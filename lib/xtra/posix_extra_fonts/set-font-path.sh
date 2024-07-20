#!/bin/sh

# Add subdirectories that contain a fonts.dir file to X server font path by calling "xset +fp" - mikaelh

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cd "${SCRIPTPATH}"

for i in */fonts.dir; do
	SUBDIR=$(dirname "${i}"})
	#echo xset +fp "${SCRIPTPATH}/${SUBDIR}"
	xset +fp "${SCRIPTPATH}/${SUBDIR}"
done
