# $Id$
# File: Makefile

# This is not a very "nice" Makefile, but it usually works.

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
  server/main.c server/traps.c server/csfunc.c server/skills.c \
  common/files.o

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
  server/main.o server/traps.o server/csfunc.o server/skills.o \
  common/files.o


CLI_SRCS = \
  common/z-util.c common/z-virt.c common/z-form.c common/z-rand.c \
  common/net-unix.c common/sockbuf.c common/net-ibm.c common/common.c \
  client/z-term.c client/c-util.c client/c-cmd.c client/c-inven.c \
  client/c-files.c client/c-tables.c client/c-store.c client/c-init.c \
  client/variable.c client/main-ibm.c client/main-cap.c client/main-gcu.c \
  client/main-x11.c client/main-xaw.c client/nclient.c client/c-birth.c \
  client/c-xtra1.c client/c-xtra2.c client/c-spell.c client/client.c \
  client/skills.c common/files.c

CLI_OBJS = \
  common/z-util.o common/z-virt.o common/z-form.o common/z-rand.o \
  common/net-unix.o common/sockbuf.o common/net-ibm.o common/common.o \
  client/z-term.o client/c-util.o client/c-cmd.o client/c-inven.o \
  client/c-files.o client/c-tables.o client/c-store.o client/c-init.o \
  client/variable.o client/main-ibm.o client/main-cap.o client/main-gcu.o \
  client/main-x11.o client/main-xaw.o client/nclient.o client/c-birth.o \
  client/c-xtra1.o client/c-xtra2.o client/c-spell.o client/client.o \
  client/skills.o common/files.o


CONS_SRCS = \
  common/z-util.c common/z-virt.c common/z-form.c common/net-unix.c \
  common/sockbuf.c common/net-ibm.c common/common.c \
  client/z-term.c common/z-rand.c \
  client/main-ibm.c client/main-cap.c client/main-gcu.c \
  client/main-x11.c client/main-xaw.c \
  console/variable.c console/c-util.c console/console.c console/c-init.c

CONS_OBJS = \
  common/z-util.o common/z-virt.o common/z-form.o common/net-unix.o \
  common/sockbuf.o common/net-ibm.o common/common.o \
  client/z-term.o common/z-rand.o \
  client/main-ibm.o client/main-cap.o client/main-gcu.o \
  client/main-x11.o client/main-xaw.o \
  console/variable.o console/c-util.o console/console.o console/c-init.o


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
  server/lua/tolua_eh.c server/lua/tolua_bd.c

LUAOBJS = \
  server/script.o server/lua_bind.o \
  server/w_util.o server/w_play.o server/w_spells.o

TOLUAOBJS = \
  server/lua/lapi.o server/lua/lcode.o server/lua/ldebug.o server/lua/ldo.o server/lua/lfunc.o server/lua/lgc.o \
  server/lua/llex.o server/lua/lmem.o server/lua/lobject.o server/lua/lparser.o server/lua/lstate.o server/lua/lstring.o \
  server/lua/ltable.o server/lua/ltests.o server/lua/ltm.o server/lua/lundump.o server/lua/lvm.o server/lua/lzio.o \
  server/lua/lauxlib.o server/lua/lbaselib.o server/lua/ldblib.o server/lua/liolib.o server/lua/lstrlib.o \
  server/lua/tolua_lb.o server/lua/tolua_rg.o server/lua/tolua_tt.o server/lua/tolua_tm.o server/lua/tolua_gp.o \
  server/lua/tolua_eh.o server/lua/tolua_bd.o

CLI_LUASRCS = \
  client/c-script.c client/lua_bind.c \
  client/w_play.c client/w_util.c client/w_spells.c

#  server/lua_bind.c \
#  server/w_util.c server/w_play.c

CLI_LUAOBJS = \
  client/c-script.o server/lua_bind.o \
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


#
# Standard version (see main-x11.c and main-gcu.c)
#
# This version supports both "X11" and "curses" in a single executable.
#
# You may have to add various X11 include/library directories to the
# "CFLAGS", if your machine places files in a weird location.
#
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
#CFLAGS = -Wall -g -pipe -D"USE_X11" -D"USE_GCU"
#LIBS = -L/usr/X11R6/lib -lX11 -lcurses


X11BASE = /usr/X11R6

##
## Variation -- FreeBSD 4.3R
##
#PREFIX = /usr/local
CFLAGS = -g -pipe -D"USE_X11" -D"USE_GCU" -I${X11BASE}/include -DDEFAULT_PATH="\"./lib\"" -O3
LIBS = -L${X11BASE}/lib -L/usr/pkg/lib -lX11 -lcurses -lcrypt 


##
## Variation -- Only support "main-x11.c" (not "main-gcu.c")
##
#CFLAGS = -Wall -O1 -pipe -g -D"USE_X11"
#LIBS = -lX11


##
## Variation -- Only support "main-gcu.c" (not "main-x11.c")
##
#CFLAGS = -Wall -O1 -pipe -g -D"USE_GCU"
#LIBS = -lcurses -ltermcap


## Added by thaler, 6/28/97
## Variation -- Only support "main-gcu.c" (not "main-x11.c") on localhost
##
## NOTE -- Using this variation requires that the server and client run
## on the same machine!  Also define META_ADDRESS to be an empty string
## in config.h.  --Keldon
##
#CFLAGS = -Wall -O1 -pipe -g -D"USE_GCU" -DUNIX_SOCKETS
#LIBS = -lncurses -ltermcap


##
## Variation -- Use "main-xaw.c" instead of "main-x11.c"
##
#CFLAGS = -Wall -O1 -pipe -g -D"USE_XAW" -D"USE_GCU"
#LIBS = -lXaw -lXmu -lXt -lX11 -lcurses -ltermcap


##
## Variation -- Use "main-cap.c" instead of "main-gcu.c"
##
#CFLAGS = -Wall -O1 -pipe -g -D"USE_X11" -D"USE_CAP"
#LIBS = -lX11 -ltermcap


##
## Variation -- Only work on simple vt100 terminals
##
#CFLAGS = -Wall -O1 -pipe -g -D"USE_CAP" -D"USE_HARDCODE"


##
## Variation -- FreeBSD 4.3R
##
#PREFIX = /usr/local
#CFLAGS = -Wall -g -D"USE_X11" -D"USE_GCU" -I${X11BASE}/include -DDEFAULT_PATH="\"${PREFIX}/lib/pmangband\""
#LIBS = -L${X11BASE}/lib -lX11 -lcurses


##
## Variation -- this might work for Linux 1.1.93 using ncurses-1.8.7.
##
#CFLAGS = -I/usr/X11R6/include -I/usr/include/ncurses \
#         -Wall -O2 -fomit-frame-pointer -m486 \
#         -D"USE_X11" -D"USE_GCU"
#LIBS = -L/usr/X11R6/lib -lX11 -lncurses -ltermcap

##
## Variation -- this might work better than the suggestion above
##
#CFLAGS = -I/usr/include/ncurses \
#         -Wall -O2 -fomit-frame-pointer \
#         -D"USE_X11" -D"USE_GCU" \
#         -D"USE_TPOSIX" -D"USE_CURS_SET"
#LIBS = -lX11 -lncurses
#LDFLAGS = -s


##
## Variation -- compile for Solaris
##
#CFLAGS = -Wall -O1 -pipe -g -D"USE_X11" -D"USE_GCU" -D"SOLARIS" -I/usr/openwin/include
#LIBS = -L/usr/openwin/lib -R/usr/openwin/lib -lX11 -lsocket -lnsl -lcurses


##
## Variation -- compile for SGI Indigo runnig Irix
##
#CFLAGS = -Wall -O1 -pipe -g -D"USE_X11" -D"USE_GCU" -D"SGI"
#LIBS = -lX11 -lcurses -ltermcap -lsun



##
## Variation -- compile for Dec ALPHA OSF/1 v2.0
##
#CC     = cc
##CFLAGS = -std -O -g3 -Olimit 4000 -D"USE_X11" -D"USE_GCU"
#CFLAGS = -std -g -D"USE_X11" -D"USE_GCU"
#LIBS   = -lX11 -lcurses -ltermcap -lrpcsvc


##
## Variation -- compile for Interactive Unix (ISC) systems
##
#CFLAGS = -Wall -O1 -pipe -g -D"USE_X11" -D"USE_GCU" -D"ISC"
#LIBS = -lX11 -lcurses -lnsl_s -linet -lcposix


##
## Variation -- Support fat binaries under NEXTSTEP
##
#CFLAGS = -Wall -O1 -pipe -g -D"USE_GCU" -arch m68k -arch i386
#LIBS = -lcurses -ltermcap


##
## Variation -- Support for DJGPP with TCPLIB
##
#CFLAGS = -Wall -O2 -g -D"USE_IBM" -DMSDOS
#LIBS = -ltcp


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

all: date tomenet tomenet.console tomenet.server accedit

# sorry. install on every make is a real pain ;)

install: all
	install -b tomenet ..
	install -b tomenet.console ..
	install -b tomenet.server ..


mini_install: install

date:
	touch common/common.c

update_use:
	touch client/client.c
	touch client/main-x11.c
#	touch client/main-gcu.c
	touch console/c-init.c

accedit: account/accedit.o
	$(CC) $(CFLAGS) -o accedit account/accedit.o -lcurses

#
# Build the "MAngband" server
#

tomenet.server: $(SERV_OBJS) $(TOLUA)
	$(CC) $(CFLAGS) -o tomenet.server $(SERV_OBJS) $(LDFLAGS) $(LIBS)


#
# Build the "MAngband" client
#

tomenet: $(CLI_OBJS)
	$(CC) $(CFLAGS) -o tomenet $(CLI_OBJS) $(LDFLAGS) $(LIBS)


#
# Build the "MAngband" remote console
#

tomenet.console: $(CONS_OBJS)
	$(CC) $(CFLAGS) -o tomenet.console $(CONS_OBJS) $(LDFLAGS) $(LIBS)


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
	cd server; rm -f *.o
	cd client; rm -f *.o
	cd common; rm -f *.o
	cd console; rm -f *.o

re: clean all

#
# Lua library compilation rules
#

server/w_util.c: server/util.pkg $(TOLUA)
	cd server; ./tolua -n util -o w_util.c util.pkg; cd ..

server/w_play.c: server/player.pkg $(TOLUA)
	cd server; ./tolua -n player -o w_play.c player.pkg; cd ..

server/w_spells.c: server/spells.pkg $(TOLUA)
	cd server; ./tolua -n spells -o w_spells.c spells.pkg; cd ..

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

