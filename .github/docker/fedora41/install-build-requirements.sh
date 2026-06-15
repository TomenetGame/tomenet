#!/bin/bash
set -euo pipefail

# Minimal, readable toolchain setup for Fedora 41.
# Builds TomeNET SDL2 (Linux) and MinGW32 Windows cross binary, native X11 (Linux) binary,
# native (Non-SDL2) MinGW32 (Windows) cross binary and linux server binary.
# This script is intended to be reusable both in distrobox and Docker image builds.
#
# Example commands for setting up build environment and building tomenet.
# Run these commands from the tomenet repository root folder.
#
# Preferred: build local builder image from Containerfile and create distrobox in one line.
# Docker:
# $ docker build -f .github/docker/fedora41/Containerfile -t tomenet-fedora41-builder:local . && distrobox create --name tomenet-fedora-41 --image tomenet-fedora41-builder:local
# Podman:
# $ podman build -f .github/docker/fedora41/Containerfile -t tomenet-fedora41-builder:local . && distrobox create --name tomenet-fedora-41 --image tomenet-fedora41-builder:local
#
# Alternative (manual setup in plain Fedora 41 distrobox):
# $ distrobox create --name tomenet-fedora-41 --image registry.fedoraproject.org/fedora:41
# $ distrobox enter tomenet-fedora-41 -- bash .github/docker/fedora41/install-build-requirements.sh
#
# Build tomenet and tomenet.exe executables (should appear in tomenet root folder):
# $ distrobox enter tomenet-fedora-41 -- bash -c 'cd ./src; make -f makefile.sdl2 clean install'
#
# Note: For running the linux client outside the container, install compatible runtime packages or copy needed .so files from the container to the directory where the binary is running from:
# libSDL2-2.0.so.0 libSDL2_ttf-2.0.so.0 libSDL2_net-2.0.so.0 libSDL2_mixer-2.0.so.0, libSDL2_image-2.0.so.0 libarchive.so.13 libcurl.so.4 libssl.so.3 libcrypto.so.3 libm.so.6 libc.so.6, and the dynamic loader ld-linux-x86-64.so.2.
#
# Note: For running the tomenet.exe, you'll need to copy out these DLLs from the container's /usr/i686-w64-mingw32/sys-root/mingw/bin/*.dll directory to the directory where the exe file is running from:
# iconv.dll libarchive-13.dll libbz2-1.dll libcrypto-3.dll libcurl-4.dll libfreetype-6.dll libgcc_s_dw2-1.dll libglib-2.0-0.dll libgnurx-0.dll libharfbuzz-0.dll libidn2-0.dll libintl-8.dll libjpeg-62.dll liblzma-5.dll libnettle-8.dll libpcre2-8-0.dll libpng16-16.dll libpsl-5.dll libsharpyuv-0.dll libssh2-1.dll libssl-3.dll libtiff-5.dll libunistring-2.dll libwebp-7.dll libwebpdemux-2.dll libwinpthread-1.dll libxml2-2.dll SDL2.dll SDL2_image.dll SDL2_mixer.dll SDL2_net.dll SDL2_ttf.dll SDL3.dll zlib1.dll
#

have_cmd() {
	command -v "$1" >/dev/null 2>&1
}

run_as_root() {
	if [ "$(id -u)" -eq 0 ]; then
		"$@"
		return
	fi

	if have_cmd sudo; then
		sudo "$@"
		return
	fi

	echo "Error: need root privileges for: $*" >&2
	echo "Run as root or install/use sudo." >&2
	exit 1
}

# Install necessary and optional packages for building linux and mingw32 versions of sdl2 tomenet client (and SDL2_net).
run_as_root dnf -y --refresh install \
  @development-tools clang autoconf automake libtool wget patchelf \
  SDL2-devel SDL2_net-devel SDL2_ttf-devel SDL2_mixer-devel SDL2_image-devel \
  libcurl-devel openssl-devel libarchive-devel \
  mingw32-gcc mingw32-gcc-c++ mingw32-binutils mingw32-pkg-config \
  mingw32-SDL2 mingw32-SDL2_ttf mingw32-SDL2_image mingw32-SDL2_mixer \
  mingw32-libarchive mingw32-curl mingw32-openssl mingw32-libgnurx

# Don't know why, but sdl2-config returns bad include path for mingw, which results in error 'SDL.h not found' in `src/client/snd-sdl.c`. Creating symlink fixes the issue.
run_as_root ln -s ./sys-root/mingw/include /usr/i686-w64-mingw32/include

# The ncurses-devel package is additionally needed for server and native x11 client compilation (makefile).
run_as_root dnf -y install ncurses-devel
# Use ncursesw (wide) for building by brute force symlinking libraries.
# Most distros have libncursesw.so as default and are missing the libncurses.so file, resulting in missing libraries error when running.
run_as_root ln -sf /usr/lib64/libncursesw.so /usr/lib64/libncurses.so

# The wine package is additionally needed for native windows client compilation (makefile.mingw).
# This download can take longer.
run_as_root dnf -y install wine
# Create symbolic link for sdl2-config to avoid makefile.mingw build error.
run_as_root mkdir -p '/usr/i686-w64-mingw32/bin'
run_as_root ln -sf '/usr/i686-w64-mingw32/sys-root/mingw/bin/sdl2-config' '/usr/i686-w64-mingw32/bin/sdl2-config'


# Build and install SDL_net for MinGW32 (not in Fedora repos).
tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT
(
	SDL2_NET_VERSION=2.2.0
  cd "$tmpdir"
  wget "https://github.com/libsdl-org/SDL_net/releases/download/release-${SDL2_NET_VERSION}/SDL2_net-${SDL2_NET_VERSION}.tar.gz"
  tar xf "SDL2_net-${SDL2_NET_VERSION}.tar.gz"
  cd "SDL2_net-${SDL2_NET_VERSION}"

  MINGW_TARGET=i686-w64-mingw32
  PREFIX=/usr/$MINGW_TARGET/sys-root/mingw
  export PATH=$PREFIX/bin:$PATH
  export SDL2_CONFIG=$PREFIX/bin/sdl2-config

  ./configure --host="$MINGW_TARGET" --prefix="$PREFIX" \
    --disable-static --disable-sdltest --disable-examples
  make -j"$(nproc)"
  run_as_root make install
)
