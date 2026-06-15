#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=.github/docker/fedora43/release-common.sh
source "${SCRIPT_DIR}/release-common.sh"

release_init
MINGW_TARGET="${MINGW_TARGET:-i686-w64-mingw32}"
RELEASE_VERSION="$(release_version)"
STAGE_DIR="${RELEASE_STAGE_BASE}/client-sdl2-win32"
ROOT_NAME="tomenet-${RELEASE_VERSION}-client-sdl2-win32"
ROOT_DIR="${STAGE_DIR}/${ROOT_NAME}"
ARCHIVE_PATH="${RELEASE_OUTDIR}/tomenet-${RELEASE_VERSION}-client-sdl2-win32.zip"

make -C "$SRC_DIR" -f makefile.sdl2 clean tomenet.exe

rm -rf "$STAGE_DIR"
mkdir -p "$ROOT_DIR"
mkdir -p "$RELEASE_OUTDIR"

cp -a "${SRC_DIR}/tomenet.exe" "${ROOT_DIR}/TomeNET.exe"
cp -a "${REPO_ROOT}/tomenet.cfg" "$ROOT_DIR/"
cp -a "${REPO_ROOT}/COPYING" "${REPO_ROOT}/README.md" "${REPO_ROOT}/TomeNET-Guide.txt" "$ROOT_DIR/"
cp -a "${REPO_ROOT}/TomeNET-direct-APAC.bat" "${REPO_ROOT}/TomeNET-direct-EU.bat" "${REPO_ROOT}/TomeNET-direct-NA.bat" "${REPO_ROOT}/screenCapture.bat" "${REPO_ROOT}/Take-ScreenShot.ps1" "$ROOT_DIR/"
cp -a "${REPO_ROOT}/lib" "$ROOT_DIR/"

copy_win_dlls_recursive "${SRC_DIR}/tomenet.exe" "$ROOT_DIR" "$MINGW_TARGET"

(
	cd "$STAGE_DIR"
	zip -r -9 "$ARCHIVE_PATH" "$ROOT_NAME"
)

release_cleanup_stage "$STAGE_DIR"
