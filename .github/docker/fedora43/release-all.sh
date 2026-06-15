#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

log() {
	echo "[release-all] $*"
}

log "Building X11 releases"
"${SCRIPT_DIR}/release-server-x11-linux.sh"
"${SCRIPT_DIR}/release-client-x11-linux.sh"

log "Building MinGW release"
"${SCRIPT_DIR}/release-client-win32.sh"

log "Building SDL2 releases"
"${SCRIPT_DIR}/release-client-sdl2-linux.sh"
"${SCRIPT_DIR}/release-client-sdl2-win32.sh"

log "Done"
