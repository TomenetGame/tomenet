# $Id$
# File: Makefile

# This is not a very "nice" Makefile, but it usually works.


# --------------------------------- READ THIS ---------------------------------
#
# Currently, only 4 target builds are supported for building the client:
#
#  -DWIN32 for Windows
#  -DUSE_X11 (optionally with USE_GCU) for X11 client
#  -DUSE_X11 )optionally with USE_GCU) with -DOSX for a Mac OSX client
#  -DUSE_GCU for a linux ncurses command-line client
#
# The below paragraph that mentions a bunch of other targets such as
# Mac/IBM/LSL/ACN/XAW is long outdated.
#
# Additionally, -DSOUND_SDL and the according sdl-config call for both compiler
# and linker flags and linking against -lSDL_mixer should be added so the
# client provides SDL sound/music support.


#
# Note that you may have to make modifications below according
# to your machine, available libraries, compilation options,
# and your "visual module" of choice.  This Makefile is intended
# for use with Unix machines running X11, Curses, Ncurses, or Vt100,
# or possibly for "Atari" or "Amiga" computers with "Curses" ports,
# see below for more information.
#
# Note that "main-mac.c", the visual module for the Macintosh,
# must be compiled in a special way, see elsewhere.
#
# Note that "main-win.c", the visual module for Windows,
# must be compiled in a special way, see elsewhere.
#
# Note that "main-ibm.c", "main-emx.c", and "main-286.c",
# the visual modules for various types of IBM-PC computers,
# must be compiled with special Makefiles, see elsewhere.
#
# Note that "main-lsl.c", the visual module for ???,
# must be compiled with "Makefile.lsl", see elsewhere.
#
# Note that "main-acn.c", the visual module for Risc Acorn,
# must be compiled with "Makefile.acn", see elsewhere.
#
# Note that "Makefile.w31" and "Makefile.wat" are slight variations
# on "Makefile.win" and "Makefile.ibm", respectively, which allow
# the use of the "main-win.c" and "main-ibm.c" files, respectively,
# with special compilers.
#
# If you are able to construct "main-xxx.c" and/or "Makefile.xxx"
# files for a currently unsupported system, please send them to me
# (benh@voicenet.com) for inclusion in future versions.
#
# This Makefile comes with "default" dependancies that may be obsolete.
#
# You may want to run "make depend" if you are planning on changing
# anything.
#


#
# The "source" and "object" files.
#

SERV_SRCS = \
  common/z-util.c common/z-virt.c common/z-form.c common/z-rand.c \
  common/net-unix.c common/sockbuf.c common/common.c \
  server/variable.c server/control.c server/hooks.c \
  server/party.c server/tables.c server/util.c server/cave.c \
  server/object1.c server/object2.c server/monster2.c \
  server/xtra1.c server/xtra2.c server/spells1.c server/spells2.c \
  server/melee1.c server/melee2.c server/save.c server/files.c \
  server/cmd1.c server/cmd2.c server/cmd3.c server/cmd4.c server/cmd5.c \
  server/cmd6.c server/store.c server/birth.c server/load2.c server/randart.c \
  server/sched.c server/generate.c server/wild.c server/dungeon.c \
  server/init1.c server/init2.c server/nserver.c server/printout.c \
  server/main.c server/traps.c server/csfunc.c server/skills.c server/bldg.c \
  common/files.c common/w_z_pack.c server/world.c server/slash.c \
  server/runecraft.c server/auction.c server/quests.c common/SFMT.c \
  server/go.c server/metaclient.c common/tables.c common/md5.c

SERV_OBJS = \
  common/z-util.o common/z-virt.o common/z-form.o common/z-rand.o \
  common/net-unix.o common/sockbuf.o common/common.o \
  server/variable.o server/control.o server/hooks.o  \
  server/party.o server/tables.o server/util.o server/cave.o \
  server/object1.o server/object2.o server/monster2.o \
  server/xtra1.o server/xtra2.o server/spells1.o server/spells2.o \
  server/melee1.o server/melee2.o server/save.o server/files.o \
  server/cmd1.o server/cmd2.o server/cmd3.o server/cmd4.o server/cmd5.o \
  server/cmd6.o server/store.o server/birth.o server/load2.o server/randart.o \
  server/sched.o server/generate.o server/wild.o server/dungeon.o \
  server/init1.o server/init2.o server/nserver.o server/printout.o \
  server/main.o server/traps.o server/csfunc.o server/skills.o server/bldg.o \
  common/files.o common/w_z_pack.o server/world.o server/slash.o \
  server/runecraft.o server/auction.o server/quests.o common/SFMT.o \
  server/go.o server/metaclient.o common/tables.o common/md5.o


CLI_SRCS = \
  common/z-util.c common/z-virt.c common/z-form.c common/z-rand.c \
  common/net-unix.c common/sockbuf.c common/net-ibm.c common/common.c \
  client/z-term.c client/c-util.c client/c-cmd.c client/c-inven.c \
  client/c-files.c client/c-tables.c client/c-store.c client/c-init.c \
  client/variable.c client/main-ibm.c client/main-cap.c client/main-gcu.c \
  client/main-x11.c client/main-xaw.c client/nclient.c client/c-birth.c \
  client/c-xtra1.c client/c-xtra2.c client/c-spell.c client/client.c \
  client/skills.c common/files.c common/SFMT.c client/snd-sdl.c \
  common/tables.c common/md5.c

CLI_OBJS = \
  common/z-util.o common/z-virt.o common/z-form.o common/z-rand.o \
  common/net-unix.o common/sockbuf.o common/net-ibm.o common/common.o \
  client/z-term.o client/c-util.o client/c-cmd.o client/c-inven.o \
  client/c-files.o client/c-tables.o client/c-store.o client/c-init.o \
  client/variable.o client/main-ibm.o client/main-cap.o client/main-gcu.o \
  client/main-x11.o client/main-xaw.o client/nclient.o client/c-birth.o \
  client/c-xtra1.o client/c-xtra2.o client/c-spell.o client/client.o \
  client/skills.o common/files.o common/SFMT.o client/snd-sdl.o \
  common/tables.o common/md5.o


CONS_SRCS = \
  common/z-util.c common/z-virt.c common/z-form.c common/net-unix.c \
  common/sockbuf.c common/net-ibm.c common/common.c \
  console/z-term.c common/z-rand.c \
  client/main-ibm.c client/main-cap.c client/main-gcu.c \
  client/main-x11.c client/main-xaw.c client/c-tables.c \
  console/variable.c console/c-util.c console/console.c console/c-init.c \
  common/SFMT.c

CONS_OBJS = \
  common/z-util.o common/z-virt.o common/z-form.o common/net-unix.o \
  common/sockbuf.o common/net-ibm.o common/common.o \
  console/z-term.o common/z-rand.o \
  client/main-ibm.o client/main-cap.o client/main-gcu.o \
  client/main-x11.o client/main-xaw.o client/c-tables.o \
  console/variable.o console/c-util.o console/console.o console/c-init.o \
  common/SFMT.o


LUASRCS = \
  server/script.c client/lua_bind.c \
  server/w_util.c server/w_play.c server/w_spells.c

# TODO: lua directory should be under common/ and not server/
TOLUASRCS = \
  server/lua/lapi.c server/lua/lcode.c server/lua/ldebug.c server/lua/ldo.c server/lua/lfunc.c server/lua/lgc.c \
  server/lua/llex.c server/lua/lmem.c server/lua/lobject.c server/lua/lparser.c server/lua/lstate.c server/lua/lstring.c \
  server/lua/ltable.c server/lua/ltests.c server/lua/ltm.c server/lua/lundump.c server/lua/lvm.c server/lua/lzio.c \
  server/lua/lauxlib.c server/lua/lbaselib.c server/lua/ldblib.c server/lua/liolib.c server/lua/lstrlib.c \
  server/lua/tolua_lb.c server/lua/tolua_rg.c server/lua/tolua_tt.c server/lua/tolua_tm.c server/lua/tolua_gp.c \
  server/lua/tolua_eh.c server/lua/tolua_bd.c server/lua/lmathlib.c

LUAOBJS = \
  server/script.o server/lua_bind.o \
  server/w_util.o server/w_play.o server/w_spells.o

TOLUAOBJS = \
  server/lua/lapi.o server/lua/lcode.o server/lua/ldebug.o server/lua/ldo.o server/lua/lfunc.o server/lua/lgc.o \
  server/lua/llex.o server/lua/lmem.o server/lua/lobject.o server/lua/lparser.o server/lua/lstate.o server/lua/lstring.o \
  server/lua/ltable.o server/lua/ltests.o server/lua/ltm.o server/lua/lundump.o server/lua/lvm.o server/lua/lzio.o \
  server/lua/lauxlib.o server/lua/lbaselib.o server/lua/ldblib.o server/lua/liolib.o server/lua/lstrlib.o \
  server/lua/tolua_lb.o server/lua/tolua_rg.o server/lua/tolua_tt.o server/lua/tolua_tm.o server/lua/tolua_gp.o \
  server/lua/tolua_eh.o server/lua/tolua_bd.o server/lua/lmathlib.o

CLI_LUASRCS = \
  client/c-script.c client/lua_bind.c \
  client/w_play.c client/w_util.c client/w_spells.c

#  server/lua_bind.c \
#  server/w_util.c server/w_play.c

CLI_LUAOBJS = \
  client/c-script.o client/lua_bind.o \
  client/w_play.o client/w_util.o client/w_spells.o

##
## Following are some "system" definitions
##
## No changes are needed to compile a version that will run on both
## X11 and Curses, in debugging mode, with maximal warnings, on many
## normal Unix machines of the Sun OS variety (non-solaris).
##
## To use an "alternative" definition, simply "modify" (or "replace")
## the definition below with one that you like.  For example, you can
## change the compiler to "cc", or remove the "debugging" options, or
## remove the X11 or Curses support, etc, as desired.
##
## See also "config.h" and "h-config.h" for important information.
##
## Some "examples" are given below, they can be used by simply
## removing the FIRST column of "#" signs from the "block" of lines
## you wish to use, and commenting out "standard" block below.
##
## This is not intended to be a "good" Makefile, just a "simple" one.
##


#
# This is my compiler of choice, it seems to work most everywhere
#
CC = gcc

# For allowing #if..#else..#endif constructs in LUA files - C. Blue
# Note: The flags must contain
# 1) the option that turns off elimination of comment lines (ie cpp
#    must just copy comments 1:1). Usually this is '-C'.
# 2) the option that turns off generation of a preprocessor header
#    (lines starting on '# '). Usually this is '-P'.
# Also, if your system doesnt have a dedicated cpp command, you might
# need to use the gcc invocation below instead.
#
# cpp variant:
CPP = cpp
CPPFLAGS = -C -P
# gcc variant:
#CPP = gcc
#CPPFLAGS = -x c -E -Wp,-C,-P


# For variations with X11
X11BASE = /usr/X11R6


##
## Standard version -- supports X11 (main-x11.c)
## Standard client for X11 usage.
##
# You may have to add various X11 include/library directories to the
# "CFLAGS", if your machine places files in a weird location.
#
## With SDL
#CFLAGS = -O2 -g -pipe -Wall -DUSE_X11 -I${X11BASE}/include -D_XOPEN_SOURCE -D_BSD_SOURCE -DMEXP=19937 -std=c99 -DSOUND_SDL `sdl-config --cflags`
#LIBS = -L${X11BASE}/lib -L/usr/pkg/lib -lX11 -lcrypt -lm `sdl-config --libs` -lSDL_mixer
##
## Without SDL
#CFLAGS = -O2 -g -pipe -Wall -DUSE_X11 -I${X11BASE}/include -D_XOPEN_SOURCE -D_BSD_SOURCE -DMEXP=19937 -std=c99
#LIBS = -L${X11BASE}/lib -L/usr/pkg/lib -lX11 -lcrypt -lm

##
## Variation: main-x11.c and main-gcu.c
## This version supports both "X11" and "curses" in a single executable.
##
# You may be able to remove "-ltermcap" on some machines (ex: Solaris).
#
# You may have to replace "-lcurses" with "-lncurses" to use the
# "new curses" library instead of the "old curses" library, and
# you may have to add "-l/usr/include/ncurses" to the "CFLAGS".
#
# See "main-gcu.c" and "config.h" for some optional "curses" defines,
# including "USE_GETCH" and "USE_CURS_SET".  Note that "config.h" will
# attempt to "guess" at many of these flags based on your system.
#
## With SDL
CFLAGS = -O2 -g -pipe -Wall -DUSE_X11 -DUSE_GCU -I${X11BASE}/include -D_XOPEN_SOURCE -D_BSD_SOURCE -DMEXP=19937 -std=c99 -DSOUND_SDL `sdl-config --cflags` -D_DEFAULT_SOURCE -DACC32
LIBS = -L${X11BASE}/lib -L/usr/pkg/lib -lX11 -lncurses -lcrypt -lm `sdl-config --libs` -lSDL_mixer
##
## Without SDL
#CFLAGS = -O2 -g -pipe -Wall -DUSE_X11 -DUSE_GCU -I${X11BASE}/include -D_XOPEN_SOURCE -D_BSD_SOURCE -DMEXP=19937 -std=c99
#LIBS = -L${X11BASE}/lib -L/usr/pkg/lib -lX11 -lncurses -lcrypt -lm


##
## Variation -- Only support "main-gcu.c" (not "main-x11.c")
## This is a command-line client that relies on ncurses.
##
## With SDL
#CFLAGS = -O2 -g -pipe -Wall -DUSE_GCU -D_XOPEN_SOURCE -D_BSD_SOURCE -DMEXP=19937 -std=c99 -DSOUND_SDL `sdl-config --cflags`
#LIBS = -L/usr/pkg/lib -lncurses -lcrypt -lm `sdl-config --libs` -lSDL_mixer
##
## Without SDL
#CFLAGS = -O2 -g -pipe -Wall -DUSE_GCU -D_XOPEN_SOURCE -D_BSD_SOURCE -DMEXP=19937 -std=c99
#LIBS = -L/usr/pkg/lib -lncurses -lcrypt -lm


##
## Variation -- OS X : See makefile.osx
##


##
## Variation -- Windows : See makefile.mingw (MinGW32) or makefile.win (Cygwin)
##


##
## Variation -- Linux / FreeBSD
##
## Add -msse2 to CFLAGS to enable SSE2 support in SFMT (not needed for x86_64).
##
## You can compile the client without X11 support by removing -DUSE_X11 from
## CFLAGS and -lX11 from LIBS
##
## With SDL
#CFLAGS = -O2 -g -pipe -Wall -DUSE_X11 -DUSE_GCU -I${X11BASE}/include -D_XOPEN_SOURCE -D_BSD_SOURCE -DMEXP=19937 -std=c99 -DSOUND_SDL `sdl-config --cflags`
#LIBS = -L${X11BASE}/lib -L/usr/pkg/lib -lX11 -lncurses -lcrypt -lm `sdl-config --libs` -lSDL_mixer
#-lXtst (for numlock - obsolete)
## Uncomment this if you want to bundle libraries with the client
#LDFLAGS = -Wl,-rpath,\$$ORIGIN
##
## Without SDL
#CFLAGS = -O2 -g -pipe -Wall -DUSE_X11 -DUSE_GCU -I${X11BASE}/include -D_XOPEN_SOURCE -D_BSD_SOURCE -DMEXP=19937 -std=c99
#LIBS = -L${X11BASE}/lib -L/usr/pkg/lib -lX11 -lncurses -lcrypt -lm


## Added by thaler, 6/28/97
## Variation -- Only support "main-gcu.c" (not "main-x11.c") on localhost
##
## NOTE -- Using this variation requires that the server and client run
## on the same machine!  Also define META_ADDRESS to be an empty string
## in config.h.  --Keldon
##
#CFLAGS = -Wall -pipe -g -DUSE_GCU -DUNIX_SOCKETS
#LIBS = -lncurses -ltermcap


##
## Variation -- Use "main-xaw.c" instead of "main-x11.c"
##
#CFLAGS = -Wall -pipe -g -DUSE_XAW -DUSE_GCU
#LIBS = -lXaw -lXmu -lXt -lX11 -lcurses -ltermcap


##
## Variation -- Use "main-cap.c" instead of "main-gcu.c"
##
#CFLAGS = -Wall -pipe -g -DUSE_X11 -DUSE_CAP
#LIBS = -lX11 -ltermcap


##
## Variation -- Only work on simple vt100 terminals
##
#CFLAGS = -Wall -pipe -g -DUSE_CAP -DUSE_HARDCODE


##
## Variation -- FreeBSD 4.3R
##
#PREFIX = /usr/local
#CFLAGS = -Wall -g -DUSE_X11 -DUSE_GCU -I${X11BASE}/include -DDEFAULT_PATH="\"${PREFIX}/lib/tomenet\""
#LIBS = -L${X11BASE}/lib -lX11 -lcurses


##
## Variation -- compile for Solaris
##
#CFLAGS = -Wall -pipe -g -DUSE_X11 -DUSE_GCU -DSOLARIS -I/usr/openwin/include
#LIBS = -L/usr/openwin/lib -R/usr/openwin/lib -lX11 -lsocket -lnsl -lcurses


# Compile a client with 'test client' version/tag
tomenet.test: CFLAGS += -DTEST_CLIENT -Og -O0
tomenet.test: PPFLAGS = -DTEST_CLIENT


# Lua
SRCS += $(LUASRCS)
SRCS += $(CLI_LUASRCS)
SERV_OBJS += $(LUAOBJS)
SERV_SRCS += $(TOLUASRCS)
SERV_OBJS += $(TOLUAOBJS)
CLI_OBJS += $(CLI_LUAOBJS)
CLI_SRCS += $(TOLUASRCS)
CLI_OBJS += $(TOLUAOBJS)
CFLAGS += $(LUAFLAGS) -Iserver -Iserver/lua

TOLUA = server/tolua

all: tomenet tomenet.server accedit
	@echo Now you need to copy the binaries out of the src directory or run \"make install\".

# sorry. install on every make is a real pain ;)

install: all
	install -b tomenet ..
	install -b tomenet.server ..
	install -b accedit ..
	touch ../tomenet.acc


mini_install: install

date:
	touch common/common.c


#
# Build the account editor
#

accedit: account/accedit.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o accedit account/accedit.o -lcurses -lcrypt


#
# Build the custom LUA preprocessor
#

preproc/preproc: preproc/preproc.o
	$(CC) $(CFLAGS) -o preproc/preproc preproc/preproc.o


#
# Build the TomeNET server
#

tomenet.server: $(SERV_OBJS) $(TOLUA)
	$(CC) $(CFLAGS) $(LDFLAGS) -o tomenet.server $(SERV_OBJS) $(LIBS)


#
# Build the evileye meta client
# Note: evilmeta is outdated, server has a built-in meta client
#
evilmeta: server/evilmeta.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o evilmeta server/evilmeta.c


#
# Build the TomeNET client
#

tomenet: $(CLI_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o tomenet $(CLI_OBJS) $(LIBS)

tomenet.test: $(CLI_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o tomenet $(CLI_OBJS) $(LIBS)


#
# Build the TomeNET remote console
#

tomenet.console: $(CONS_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o tomenet.console $(CONS_OBJS) $(LIBS)


$(TOLUA): $(TOLUAOBJS) server/lua/tolua.c server/lua/tolualua.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(TOLUAOBJS) server/lua/tolua.c server/lua/tolualua.c $(LIBS)

#
# Rule for making a '.o'
#
.c.o:
	$(CC) $(CFLAGS) -o $*.o -c $*.c


#
# Clean up old junk
#

clean:
	cd server/lua; rm -f *.o
	cd server; rm -f *.o w_play.c w_util.c w_spells.c *.pkg
	cd client; rm -f *.o w_play.c w_util.c w_spells.c *.pkg
	cd common; rm -f *.o w_z_pack.c
	cd console; rm -f *.o
	rm -f account/accedit.o preproc/preproc.o

re: clean all

#
# Lua library compilation rules
#

server/util.pkg: server/util.pre preproc/preproc
	cd server; ../preproc/preproc util.pre util.pkg $(CPP) $(CPPFLAGS) $(PPFLAGS); cd ..

server/player.pkg: server/player.pre preproc/preproc
	cd server; ../preproc/preproc player.pre player.pkg $(CPP) $(CPPFLAGS) $(PPFLAGS); cd ..

server/spells.pkg: server/spells.pre preproc/preproc
	cd server; ../preproc/preproc spells.pre spells.pkg $(CPP) $(CPPFLAGS) $(PPFLAGS); cd ..

client/util.pkg: client/util.pre preproc/preproc
	cd client; ../preproc/preproc util.pre util.pkg $(CPP) $(CPPFLAGS) $(PPFLAGS); cd ..

client/player.pkg: client/player.pre preproc/preproc
	cd client; ../preproc/preproc player.pre player.pkg $(CPP) $(CPPFLAGS) $(PPFLAGS); cd ..

client/spells.pkg: client/spells.pre preproc/preproc
	cd client; ../preproc/preproc spells.pre spells.pkg $(CPP) $(CPPFLAGS) $(PPFLAGS); cd ..

server/w_util.c: server/util.pkg $(TOLUA)
	cd server; ./tolua -n util -o w_util.c util.pkg; cd ..

server/w_play.c: server/player.pkg $(TOLUA)
	cd server; ./tolua -n player -o w_play.c player.pkg; cd ..

server/w_spells.c: server/spells.pkg $(TOLUA)
	cd server; ./tolua -n spells -o w_spells.c spells.pkg; cd ..

common/w_z_pack.c: common/z_pack.pkg $(TOLUA)
	cd common; ../server/tolua -n z_pack -o w_z_pack.c z_pack.pkg; cd ..

client/w_play.c: client/player.pkg $(TOLUA)
	cd client; ../server/tolua -n player -o w_play.c player.pkg; cd ..

client/w_util.c: client/util.pkg $(TOLUA)
	cd client; ../server/tolua -n util -o w_util.c util.pkg; cd ..

client/w_spells.c: client/spells.pkg $(TOLUA)
	cd client; ../server/tolua -n spells -o w_spells.c spells.pkg; cd ..


#
# Generate dependancies automatically
#

depend:
	makedepend -- $(CFLAGS) -D__MAKEDEPEND__ -- $(SERV_SRCS) $(CLI_SRCS)


# DO NOT ADD ANYTHING BELOW THIS LINE.

# DO NOT DELETE THIS LINE -- make depend depends on it.

