#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="${REPO_ROOT:-$(cd -- "${SCRIPT_DIR}/../../.." && pwd)}"
SRC_DIR="${SRC_DIR:-${REPO_ROOT}/src}"

release_init() {
	RELEASE_WORKSPACE="${RELEASE_WORKSPACE:-${REPO_ROOT}/.github/workspace}"
	RELEASE_STAGE_BASE="${RELEASE_STAGE_BASE:-${RELEASE_WORKSPACE}/fedora41/.release-stage}"
	RELEASE_OUTDIR="${RELEASE_OUTDIR:-${RELEASE_WORKSPACE}}/fedora41"
	RELEASE_KEEP_STAGE="${RELEASE_KEEP_STAGE:-no}"

	if [[ "${RELEASE_STAGE_BASE}" != /* ]]; then
		RELEASE_STAGE_BASE="${REPO_ROOT}/${RELEASE_STAGE_BASE}"
	fi
	if [[ "${RELEASE_OUTDIR}" != /* ]]; then
		RELEASE_OUTDIR="${REPO_ROOT}/${RELEASE_OUTDIR}"
	fi
}

release_version() {
	awk '
		BEGIN { ext=0; br=0; bld=0 }
		/^#define[ \t]+VERSION_MAJOR[ \t]+[0-9]+/  { maj=$3 }
		/^#define[ \t]+VERSION_MINOR[ \t]+[0-9]+/  { min=$3 }
		/^#define[ \t]+VERSION_PATCH[ \t]+[0-9]+/  { pat=$3 }
		/^#define[ \t]+VERSION_EXTRA[ \t]+[0-9]+/  { ext=$3 }
		/^#define[ \t]+VERSION_BRANCH[ \t]+[0-9]+/ { br=$3 }
		/^#define[ \t]+VERSION_BUILD[ \t]+[0-9]+/  { bld=$3 }
		END {
			if (ext == 0 && br == 0 && bld == 0) print maj "." min "." pat
			else print maj "." min "." pat "." ext "." br "." bld
		}
	' "${SRC_DIR}/common/defines.h"
}

copy_linux_deps_recursive() {
	local binary="$1"
	local stage_dir="$2"
	local deps_file
	local seen_file
	local idx
	local lib
	local base

	deps_file="$(mktemp)"
	seen_file="$(mktemp)"
	idx=1

	ldd "$binary" | awk '/=> \// { print $3 }' > "$deps_file"
	while :; do
		lib="$(sed -n "${idx}p" "$deps_file")"
		[[ -n "$lib" ]] || break
		idx=$((idx + 1))
		[[ -f "$lib" ]] || continue
		base="$(basename "$lib")"
		case "$base" in
			ld-linux*.so*|libc.so.*|libm.so.*|libdl.so.*|libpthread.so.*|librt.so.*|libresolv.so.*|libtinfo.so.*)
				continue
				;;
		esac
		grep -Fqx "$lib" "$seen_file" && continue
		echo "$lib" >> "$seen_file"
		cp -aL "$lib" "$stage_dir/"
		ldd "$lib" | awk '/=> \// { print $3 }' >> "$deps_file"
		patchelf --set-rpath '$ORIGIN' "${stage_dir}/${base}"
	done

	rm -f "$deps_file" "$seen_file"
}

resolve_win_dll_for_target() {
	local dll="$1"
	local mingw_target="$2"
	local file_name
	local sysroot
	local candidate
	local -a search_roots
	local -a candidates

	file_name="$("${mingw_target}-gcc" -print-file-name="$dll" 2>/dev/null || true)"
	if [[ -n "$file_name" && "$file_name" != "$dll" && -f "$file_name" ]]; then
		printf '%s\n' "$file_name"
		return 0
	fi

	sysroot="$("${mingw_target}-gcc" -print-sysroot 2>/dev/null || true)"
	search_roots=()
	for candidate in \
		"${sysroot}/mingw" \
		"/usr/${mingw_target}/sys-root/mingw" \
		"/usr/${mingw_target}" \
		"/usr/lib/gcc/${mingw_target}" \
		"/usr/local/${mingw_target}/sys-root/mingw" \
		"/usr/local/${mingw_target}" \
		"/usr/local/lib/gcc/${mingw_target}"; do
		[[ -d "$candidate" ]] && search_roots+=("$candidate")
	done

	(( ${#search_roots[@]} > 0 )) || return 1

	mapfile -t candidates < <(find "${search_roots[@]}" -type f -iname "$dll" 2>/dev/null | LC_ALL=C sort -u)
	(( ${#candidates[@]} > 0 )) || return 1

	printf '%s\n' "${candidates[0]}"
	return 0
}

copy_win_dlls_recursive() {
	local binary="$1"
	local stage_dir="$2"
	local mingw_target="${3:-i686-w64-mingw32}"
	local deps_file
	local seen_file
	local idx
	local dll
	local lower
	local src

	deps_file="$(mktemp)"
	seen_file="$(mktemp)"
	idx=1

	"${mingw_target}-objdump" -p "$binary" | awk '/DLL Name:/ { print $3 }' > "$deps_file"
	while :; do
		dll="$(sed -n "${idx}p" "$deps_file")"
		[[ -n "$dll" ]] || break
		idx=$((idx + 1))
		lower="$(printf '%s' "$dll" | tr '[:upper:]' '[:lower:]')"
		case "$lower" in
			kernel32.dll|user32.dll|gdi32.dll|shell32.dll|advapi32.dll|ws2_32.dll|wsock32.dll|msvcrt.dll|winmm.dll|comdlg32.dll|crypt32.dll|secur32.dll|iphlpapi.dll|ole32.dll|oleaut32.dll|imm32.dll|setupapi.dll|version.dll|uxtheme.dll|shcore.dll|bcrypt.dll|wldap32.dll|dnsapi.dll|rpcrt4.dll|api-ms-win-*|ext-ms-*|ucrtbase.dll)
				continue
				;;
		esac
		grep -Fqx "$lower" "$seen_file" && continue
		echo "$lower" >> "$seen_file"
		src="$(resolve_win_dll_for_target "$dll" "$mingw_target" || true)"
		if [[ -z "$src" ]]; then
			echo "warning: missing DLL dependency $dll" >&2
			continue
		fi
		cp -aL "$src" "$stage_dir/"
		"${mingw_target}-objdump" -p "$src" | awk '/DLL Name:/ { print $3 }' >> "$deps_file"
	done

	rm -f "$deps_file" "$seen_file"
}

release_cleanup_stage() {
	local stage_dir="$1"

	if [[ "$RELEASE_KEEP_STAGE" != "yes" ]]; then
		rm -rf "$stage_dir"
		rmdir "$RELEASE_STAGE_BASE" 2>/dev/null || true
	fi
}
