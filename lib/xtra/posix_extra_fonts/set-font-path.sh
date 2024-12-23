#!/bin/sh

# Add subdirectories that contain a fonts.dir file to X server font path by calling "xset +fp" - mikaelh

#SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
# Split up the above line cause it gave me bad syntax highlighting in silyl mc ^^' - C. Blue
DIR=$(dirname "$0")
SCRIPTPATH="$( cd -- "$DIR" >/dev/null 2>&1 ; pwd -P )"

cd "${SCRIPTPATH}"

for i in */fonts.dir; do
	SUBDIR=$(dirname "${i}"})
	#echo xset +fp "${SCRIPTPATH}/${SUBDIR}"

	# Hack: remove [multiple] duplicates of this font path, that were potentially added during previous runs - C. Blue
	# Otherwise we'd eventually exceed the max string size of the font path and get an error 51:
	# X Error of failed request: BadLength (poly request too large or internal Xlib length error)
	xset -fp "${SCRIPTPATH}/${SUBDIR}"
	xset +fp "${SCRIPTPATH}/${SUBDIR}"
done
