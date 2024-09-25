#!/bin/bash
set -euo pipefail

# Minimal, readable toolchain setup for Fedora 43.
# Builds TomeNET SDL3 Linux and MinGW32 Windows clients, native X11 Linux client,
# native non-SDL MinGW32 Windows client, and the Linux server.
# This script is intended to be reusable both in distrobox and Docker image builds.
#
# Example commands for setting up build environment and building tomenet.
# Run these commands from the tomenet repository root folder.
#
# Preferred: build local builder image from Containerfile and create distrobox in one line.
# Docker:
# $ docker build -f .github/docker/fedora43/Containerfile -t tomenet-fedora43-builder:local . && distrobox create --name tomenet-fedora-43 --image tomenet-fedora43-builder:local
# Podman:
# $ podman build -f .github/docker/fedora43/Containerfile -t tomenet-fedora43-builder:local . && distrobox create --name tomenet-fedora-43 --image tomenet-fedora43-builder:local
#
# Alternative (manual setup in plain Fedora 43 distrobox):
# $ distrobox create --name tomenet-fedora-43 --image registry.fedoraproject.org/fedora:43
# $ distrobox enter tomenet-fedora-43 -- bash .github/docker/fedora43/install-build-requirements.sh
#
# Build tomenet and tomenet.exe executables (should appear in tomenet root folder):
# $ distrobox enter tomenet-fedora-43 -- bash -c 'cd ./src; make -f makefile.sdl3 clean install'
#
# Note: For running the linux client outside the container, install compatible runtime packages or copy needed .so files from the container to the directory where the binary is running from. To determine all the .so files you need to run the client, use the `ldd` command on the executable, or when building a release (.github/docker/fedora43/release-client-sdl3-linux.sh), the resulting archive contains all the needed .so files.
#
# Note: For running the tomenet.exe, you'll need to copy out all needed DLLs from the container to the directory where the exe file is running from. To determine all the .dll files you need to run the client, use the `i686-w64-mingw32-objdump` command on the executable, or when building a release (.github/docker/fedora43/release-client-sdl3-win32.sh), the resulting archive contains all the needed .dll files.
#

# The SDL3 library and extensions to build and install.
SDL3_VERSION=3.4.10
SDL3_TTF_VERSION=3.2.0
SDL3_IMAGE_VERSION=3.4.4
SDL3_NET_VERSION=3.2.0
SDL3_MIXER_VERSION=3.2.0

# The MINGW namespace and path to extract.
MINGW_TARGET=i686-w64-mingw32
MINGW_PREFIX=/usr/${MINGW_TARGET}/sys-root/mingw

run_as_root() {
	if [ "$(id -u)" -eq 0 ]; then
		"$@"
		return
	fi

	if command -v sudo >/dev/null 2>&1; then
		sudo "$@"
		return
	fi

	echo "Error: need root privileges for: $*" >&2
	echo "Run as root or install/use sudo." >&2
	exit 1
}

# Install toolchains and non-SDL dependencies. SDL3 itself is installed from upstream pins below.
run_as_root dnf -y --refresh install \
  @development-tools clang autoconf automake libtool cmake ninja-build wget tar unzip patchelf pkgconf-pkg-config \
  alsa-lib-devel pipewire-devel pulseaudio-libs-devel libXcursor-devel libXext-devel libXfixes-devel libXi-devel libXrandr-devel libXScrnSaver-devel libXtst-devel libxkbcommon-devel wayland-devel wayland-protocols-devel \
  SDL2_mixer-devel \
  libcurl-devel openssl-devel libarchive-devel freetype-devel harfbuzz-devel zlib-static libpng-devel libpng-static libjpeg-turbo-devel libtiff-devel libwebp-devel libavif-devel \
  flac flac-devel fluidsynth-devel libogg-devel libvorbis-devel libxmp-devel mpg123-devel opusfile-devel opus-devel wavpack-devel \
  mingw32-gcc mingw32-gcc-c++ mingw32-binutils mingw32-pkg-config \
  mingw32-SDL2_mixer mingw32-libarchive mingw32-curl mingw32-openssl mingw32-libgnurx

# The ncurses-devel package is additionally needed for server and native x11 client compilation (makefile).
run_as_root dnf -y install ncurses-devel
# Use ncursesw (wide) for building by brute force symlinking libraries.
# Most distros have libncursesw.so as default and are missing the libncurses.so file, resulting in missing libraries error when running.
run_as_root ln -sf /usr/lib64/libncursesw.so /usr/lib64/libncurses.so

# The wine package is additionally needed for native windows client compilation (makefile.mingw).
# This download can take longer.
run_as_root dnf -y install wine
# Create symbolic link for sdl2-config to avoid makefile.mingw build error.
run_as_root mkdir -p "/usr/${MINGW_TARGET}/bin"
run_as_root ln -sf "${MINGW_PREFIX}/bin/sdl2-config" "/usr/${MINGW_TARGET}/bin/sdl2-config"


build_and_install_sdl_project() {
	local repo="$1"
	local asset_prefix="$2"
	local pc_module="$3"
	local version="$4"
	local archive="${asset_prefix}-${version}.tar.gz"
	local url="https://github.com/libsdl-org/${repo}/releases/download/release-${version}/${archive}"
	local -a cmake_args

	if PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig:/usr/local/lib/pkgconfig pkg-config --atleast-version="$version" "$pc_module" 2>/dev/null; then
		echo "${pc_module} >= ${version} is already installed; skipping ${asset_prefix} source build."
		return
	fi

	cmake_args=(
		-DCMAKE_BUILD_TYPE=Release
		-DCMAKE_INSTALL_PREFIX=/usr/local
		-DBUILD_SHARED_LIBS=ON
		-DSDL_SHARED=ON
		-DSDL_STATIC=OFF
		-DSDL_TEST_LIBRARY=OFF
	)

	case "$repo" in
		SDL_ttf)
			cmake_args+=(-DSDLTTF_VENDORED=OFF)
			;;
		SDL_image)
			cmake_args+=(-DSDLIMAGE_VENDORED=OFF)
			;;
		SDL_mixer)
			cmake_args+=(-DSDLMIXER_VENDORED=OFF)
			;;
	esac

	wget -O "$archive" "$url"
	tar xf "$archive"
	cd "${asset_prefix}-${version}"
	cmake -S . -B build "${cmake_args[@]}"
	cmake --build build --parallel "$(nproc)"
	run_as_root cmake --install build
	cd ..
}

install_mingw_devel_archive() {
	local repo="$1"
	local asset_prefix="$2"
	local pc_module="$3"
	local version="$4"
	local archive="${asset_prefix}-devel-${version}-mingw.tar.gz"
	local url="https://github.com/libsdl-org/${repo}/releases/download/release-${version}/${archive}"
	local extracted

	# Check if package is already installed.
	if PKG_CONFIG_PATH="${MINGW_PREFIX}/lib/pkgconfig:${MINGW_PREFIX}/share/pkgconfig" "${MINGW_TARGET}-pkg-config" --atleast-version="$version" "$pc_module" 2>/dev/null; then
		echo "${pc_module} >= ${version} is already installed for ${MINGW_TARGET}; skipping ${archive}."
		return
	fi

	# Download and extract.
	wget -O "$archive" "$url"
	tar xf "$archive"

	# Check if extracted and everything is as it should be.
	extracted="$(find . -mindepth 2 -maxdepth 3 -type d -path "*/${MINGW_TARGET}" | head -n 1)"
	if [ -z "$extracted" ]; then
		echo "Error: ${archive} does not contain ${MINGW_TARGET}" >&2
		exit 1
	fi

	# Copy extracted files to proper destination and fix pkgconfig module file.
	run_as_root cp -a "${extracted}/." "$MINGW_PREFIX/"
	run_as_root sh -c "[ -f ${MINGW_PREFIX}/lib/pkgconfig/${pc_module}.pc ] && sed -i 's|^prefix=.*|prefix=\${pcfiledir}/../..|' ${MINGW_PREFIX}/lib/pkgconfig/${pc_module}.pc"

	# Check if pkgconfig sees the installed library.
	if ! PKG_CONFIG_PATH="${MINGW_PREFIX}/lib/pkgconfig:${MINGW_PREFIX}/share/pkgconfig" "${MINGW_TARGET}-pkg-config" --atleast-version="$version" "$pc_module"; then
		echo "Error: ${pc_module} was not installed for ${MINGW_TARGET} from ${archive}" >&2
		exit 1
	fi

	# Need to remove the extracted dir, so next install does not find it when searching for extracted dir.
	rm -r ${extracted}
}

# Build and install all needed SDL3 libraries.
tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT
(
	cd "$tmpdir"
	build_and_install_sdl_project SDL SDL3 sdl3 "$SDL3_VERSION"
	build_and_install_sdl_project SDL_ttf SDL3_ttf sdl3-ttf "$SDL3_TTF_VERSION"
	build_and_install_sdl_project SDL_image SDL3_image sdl3-image "$SDL3_IMAGE_VERSION"
	build_and_install_sdl_project SDL_net SDL3_net sdl3-net "$SDL3_NET_VERSION"
	build_and_install_sdl_project SDL_mixer SDL3_mixer sdl3-mixer "$SDL3_MIXER_VERSION"

	install_mingw_devel_archive SDL SDL3 sdl3 "$SDL3_VERSION"
	install_mingw_devel_archive SDL_ttf SDL3_ttf sdl3-ttf "$SDL3_TTF_VERSION"
	install_mingw_devel_archive SDL_image SDL3_image sdl3-image "$SDL3_IMAGE_VERSION"
	install_mingw_devel_archive SDL_net SDL3_net sdl3-net "$SDL3_NET_VERSION"
	install_mingw_devel_archive SDL_mixer SDL3_mixer sdl3-mixer "$SDL3_MIXER_VERSION"
)

run_as_root ldconfig
