#!/bin/bash


# This file installs extra fonts locally for the running user. - C. Blue


# ======================================================================================
# ------ Check the variable settings below and ensure they match with your setup: ------

# This file must be located in your tomenet/lib/xtra folder!

# path of the user's local font folder, which we will create if it doesn't exist yet
LOCALFONTPATH=~/.local/share/fonts/misc

# paths to system's font config
FONTCFG=/etc/fonts/conf.d
FONTCFGAVAIL=/usr/share/fontconfig/conf.avail

# location of TomeNET's user folder, from our working directory (which must be tomenet/lib/xtra)
TOMENETUSER=../user

# location of a folder that has subfolders with extra fonts inside (eg extra_fonts/coolfont/, extra_fonts/myfont/ ...)
# It will be scanned for subfolders and from each subfolder all prf and pcf files inside it will be installed.
# Example file structure:
# /home/tomenetplayer/extra_fonts/mylittlefont/font-custom-7x10.prf
# /home/tomenetplayer/extra_fonts/mylittlefont/font-custom-7x10.pcf
# /home/tomenetplayer/extra_fonts/hugefont/font-custom-24x36.prf
# /home/tomenetplayer/extra_fonts/hugefont/font-custom-24x36.pcf
EXTRAFONTS_PARENTFOLDER=~/extra_fonts

# ======================================================================================



# Check for correct surroundings

if [ ! -d $EXTRAFONTS_PARENTFOLDER ]; then
    echo ""
    echo "  Error: Font folder '$EXTRAFONTS_PARENTFOLDER' not found (in relation to current directory)."
    echo "  This script must be located and called in TomeNET's ./lib/xtra/ folder!"
    echo ""
    exit 1
fi
if [ ! -d $TOMENETUSER ]; then
    echo ""
    echo "  Error: TomeNET's lib/xtra folder structure not found."
    echo "  This script must be located and called in TomeNET's ./lib/xtra/ folder!"
    echo ""
    exit 1
fi

echo ""
echo "  Installing extra fonts!"
echo ""


# Ubuntu has this silly no-bitmap-fonts config

if [ -f $FONTCFG/70-no-bitmaps.conf ]; then
    echo "  Found bitmap-preventing setting: $FONTCFG/70-no-bitmaps.conf"
    echo "   Trying to delete this file, in order to enable bitmap font usage:"
    sudo rm -v $FONTCFG/70-no-bitmaps.conf
else
    echo "  Confirmed absense of $FONTCFG/70-no-bitmap.conf - good!"
fi
if [ ! -f $FONTCFG/70-yes-bitmaps.conf ]; then
    echo "  Trying to instantiate: $FONTCFG/70-yes-bitmaps.conf"
    echo "   As it is needed on some systems, in order to enable bitmap font usage:"
    sudo ln -v -s $FONTCFGAVAIL/70-yes-bitmaps.conf $FONTCFG/
else
    echo "  Confirmed presense of $FONTCFG/70-yes-bitmap.conf - good!"
fi
echo ""


# Iterate over all subfolders within the extra-fonts folder, and attempt to install all prf/pcf from each one:
for FONTDIR in $EXTRAFONTS_PARENTFOLDER/*/
do
    # remove trailing slash
    FONTDIR=${FONTDIR%*/}

    # paranoia: catch non-existant folder
    if [ "$FONTDIR" == "*" ]; then
        echo "Error: '$EXTRAFONTS_PARENTFOLDER' is not a valid directory."
        exit 1
    fi

    echo "   Installing $FONTDIR fonts:"
    mkdir -p $LOCALFONTPATH

    echo "    copying .prf files..."
    cp $FONTDIR/prf/*.prf $TOMENETUSER/

    echo "    copying .pcf files..."
    cp $FONTDIR/pcf/*.pcf $LOCALFONTPATH/

    echo "    building local fonts folder at '$LOCALFONTPATH'..."
    mkfontdir $LOCALFONTPATH

    echo "    adding that folder to fonts path ('xset q' to check)..."
    xset +fp $LOCALFONTPATH

    echo "    registering fonts in the system..."
    fc-cache -f

    echo "    These fonts from this font folder are now recognized by the system:"
    rm -f _extra_fonts_list.tmp
    ls -1  $FONTDIR/pcf/*.pcf | grep -o "[^/][0-9a-z]*.pcf$" | grep -o "^[^.]*" > _extra_fonts_list.tmp
    xlsfonts |grep -f _extra_fonts_list.tmp | tr '\n' ' '
    rm _extra_fonts_list.tmp
    echo ""
done

# Finished
echo ""
