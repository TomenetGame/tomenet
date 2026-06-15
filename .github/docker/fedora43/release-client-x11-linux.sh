#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=.github/docker/fedora43/release-common.sh
source "${SCRIPT_DIR}/release-common.sh"

release_init
RELEASE_ARCH="${RELEASE_ARCH:-amd64}"
RELEASE_VERSION="$(release_version)"
STAGE_DIR="${RELEASE_STAGE_BASE}/client-x11-linux"
ROOT_NAME="tomenet-${RELEASE_VERSION}-client-x11-linux-${RELEASE_ARCH}"
ROOT_DIR="${STAGE_DIR}/${ROOT_NAME}"
ARCHIVE_PATH="${RELEASE_OUTDIR}/tomenet-${RELEASE_VERSION}-client-x11-linux-${RELEASE_ARCH}.tar.bz2"

make -C "$SRC_DIR" -f makefile clean tomenet

rm -rf "$STAGE_DIR"
mkdir -p "$ROOT_DIR"
mkdir -p "$RELEASE_OUTDIR"

cp -a "${SRC_DIR}/tomenet" "$ROOT_DIR/"
cp -a "${REPO_ROOT}/.tomenetrc" "$ROOT_DIR/"
cp -a "${REPO_ROOT}/COPYING" "${REPO_ROOT}/README.md" "${REPO_ROOT}/TomeNET-Guide.txt" "$ROOT_DIR/"
cp -a "${REPO_ROOT}/tomenet-direct-APAC.sh" "${REPO_ROOT}/tomenet-direct-EU.sh" "${REPO_ROOT}/tomenet-direct-NA.sh" "$ROOT_DIR/"
cp -a "${REPO_ROOT}/lib" "$ROOT_DIR/"

copy_linux_deps_recursive "${SRC_DIR}/tomenet" "$ROOT_DIR"

(
	cd "$STAGE_DIR"
	tar -cjf "$ARCHIVE_PATH" "$ROOT_NAME"
)

release_cleanup_stage "$STAGE_DIR"
