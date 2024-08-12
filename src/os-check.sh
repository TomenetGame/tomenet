# Helper script to determine less common OS - C. Blue

if [ "$ANDROID_ROOT" != "" ] || [ "`uname -r | grep -i android`" != "" ]; then echo "-DANDROID" # practically implies GCU atm
elif [ "`uname -s | grep -i iPhone`" != "" ]; then echo "-DIOS"
elif [ "`uname -s | grep -i iPad`" != "" ]; then echo "-DIOS"
elif [ "`uname -a |  grep -i Darwin`" != "" ]; then echo "-DOSX" # practically redudant as it is (should be) set in the makefile.osx
elif [ "`uname -s |  grep -i Linux`" != "" ]; then echo "-DLINUX" # a dummy, as we use X11/GCU for this as specializations anyway
else echo "-DANY_`uname -s`"
fi
# ... and Windows clients should be compiled with WIN32 in the makefile.win / makefile.mingw etc anyway
