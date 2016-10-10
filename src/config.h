/* $Id$ */
/* File: config.h */

#ifndef TOMENET_CONFIG_H
#define TOMENET_CONFIG_H

/* Purpose: Angband specific configuration stuff */

/* Note : Much of the functionality of this file is being phased into
 * tomenet.cfg.  This will allow server reconfiguration without recompilation.
 */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

/*
 * Look through the following lines, and where a comment includes the
 * tag "OPTION:", examine the associated "#define" statements, and decide
 * whether you wish to keep, comment, or uncomment them.  You should not
 * have to modify any lines not indicated by "OPTION".
 *
 * Note: Also examine the "system" configuration file "h-config.h"
 * and the variable initialization file "variable.c".  If you change
 * anything in "variable.c", you only need to recompile that file.
 *
 * And finally, remember that the "Makefile" will specify some rather
 * important compile time options, like what visual module to use.
 */


/*
 * OPTION: See the Makefile(s), where several options may be declared.
 *
 * Some popular options include "USE_GCU" (allow use with Unix "curses"),
 * "USE_X11" (allow basic use with Unix X11), "USE_XAW" (allow use with
 * Unix X11 plus the Athena Widget set), and "USE_CAP" (allow use with
 * the "termcap" library, or with hard-coded vt100 terminals).
 *
 * The old "USE_NCU" option has been replaced with "USE_GCU".
 *
 * Several other such options are available for non-unix machines,
 * such as "MACINTOSH", "WINDOWS", "USE_IBM", "USE_EMX".
 *
 * You may also need to specify the "system", using defines such as
 * "SOLARIS" (for Solaris), etc, see "h-config.h" for more info.
 */


/*
 * OPTION: Include some debugging code.  Note that this option may
 * result in a server that crashes more frequently, as a core dump
 * will be done instead of killing the bad connection.
 */
#define DEBUG

/*
 * OPTION: Break graphics on the client side.  Due to a very bad
 * network design, sending the character definitions needed for
 * graphics may cause clients to not be able to connect.  This
 * disables this, which may help in resolving connection problems.
 * Note that you will need to connect to an 0.5.4 or later server
 * for this to work.
 *
 * This option will be taken out in 0.6.0, which will have a
 * completely different network design, thus alleviating this and
 * other problems.
 */
/* #define BREAK_GRAPHICS */

/*
 * OPTION: define "SPECIAL_BSD" for using certain versions of UNIX
 * that use the 4.4BSD Lite version of Curses in "main-gcu.c"
 */
/*
 * NOTE: FreeBSD user should *NOT* define SPECIAL_BSD.
 */
/* #define SPECIAL_BSD */

/* #define NETBSD pfft -- indeed should go in makefile ;) */

/*
 * For server: can use crypt() for doing passwords
 */
#ifndef WIN32
#define HAVE_CRYPT
#endif

/*
 * OPTION: Use the POSIX "termios" methods in "main-gcu.c"
 */
/* #define USE_TPOSIX */

/*
 * OPTION: Use the "termio" methods in "main-gcu.c"
 */
/* #define USE_TERMIO */

/*
 * OPTION: Use the icky BSD "tchars" methods in "main-gcu.c"
 */
/* #define USE_TCHARS */


/*
 * OPTION: Use "blocking getch() calls" in "main-gcu.c".
 * Hack -- Note that this option will NOT work on many BSD machines
 * Currently used whenever available, if you get a warning about
 * "nodelay()" undefined, then make sure to undefine this.
 */
#if defined(SYS_V) || defined(AMIGA)
# define USE_GETCH
#endif


/*
 * OPTION: Use the "curs_set()" call in "main-gcu.c".
 * Hack -- This option will not work on most BSD machines
 */
#ifdef SYS_V
# define USE_CURS_SET
#endif


/*
 * OPTION: Include "ncurses.h" instead of "curses.h" in "main-gcu.c"
 */
#define USE_NCURSES


/*
 * OPTION: for multi-user machines running the game setuid to some other
 * user (like 'games') this SAFE_SETUID option allows the program to drop
 * its privileges when saving files that allow for user specified pathnames.
 * This lets the game be installed system wide without major security
 * concerns.  There should not be any side effects on any machines.
 *
 * This will handle "gids" correctly once the permissions are set right.
 */
#define SAFE_SETUID


/*
 * This flag enables the "POSIX" methods for "SAFE_SETUID".
 */
#ifdef _POSIX_SAVED_IDS
# define SAFE_SETUID_POSIX
#endif


/*
 * This "fix" is from "Yoshiaki KASAHARA <kasahara@csce.kyushu-u.ac.jp>"
 * It prevents problems on (non-Solaris) Suns using "SAFE_SETUID".
 */
#if defined(sun) && !defined(SOLARIS)
# undef SAFE_SETUID_POSIX
#endif




/*
 * OPTION: for the AFS distributed file system, define this to ensure that
 * the program is secure with respect to the setuid code.  This option has
 * not been tested (to the best of my knowledge).  This option may require
 * some weird tricks with "player_uid" and such involving "defines".
 * Note that this option used the AFS library routines Authenticate(),
 * bePlayer(), beGames() to enforce the proper priviledges.
 * You may need to turn "SAFE_SETUID" off to use this option.
 */
/* #define SECURE */




/*
 * OPTION: Verify savefile Checksums (Angband 2.7.0 and up)
 * This option can help prevent "corruption" of savefiles, and also
 * stop intentional modification by amateur users.
 */
#define VERIFY_CHECKSUMS


/*
 * OPTION: Forbid the use of "fiddled" savefiles.  As far as I can tell,
 * a fiddled savefile is one with an internal timestamp different from
 * the actual timestamp.  Thus, turning this option on forbids one from
 * copying a savefile to a different name.  Combined with disabling the
 * ability to save the game without quitting, and with some method of
 * stopping the user from killing the process at the tombstone screen,
 * this should prevent the use of backup savefiles.  It may also stop
 * the use of savefiles from other platforms, so be careful.
 */
/* #define VERIFY_TIMESTAMP */


/*
 * OPTION: Forbid the "savefile over-write" cheat, in which you simply
 * run another copy of the game, loading a previously saved savefile,
 * and let that copy over-write the "dead" savefile later.  This option
 * either locks the savefile, or creates a fake "xxx.lok" file to prevent
 * the use of the savefile until the file is deleted.  Not ready yet.
 */
/* #define VERIFY_SAVEFILE */


/*
 * OPTION: Set the metaserver address.  The metaserver keeps track of
 * all running servers so people can find an active server with other
 * players on.  Define this to be an empty string if you don't want to
 * report to a metaserver.
 */

/* NOTE: client uses these values.
 * server uses those in tomenet.cfg.
 * bad design.
 */

//#define	META_ADDRESS "62.210.141.11"
//#define	META_ADDRESS_2 "europe.tomenet.eu"
#define		META_ADDRESS "meta.tomenet.eu"
#define		META_ADDRESS_2 "37.187.75.24"

/*
 * Server gateway: Provide raw data for applications
 */
#define SERVER_GWPORT

/*
 * Worlds server connection
 */
/* for local testing - if you see this commented out, that means
 * I committed it by mistake.  please just make it revert back.	-Jir
 */
/* Would you make it a tomenet.cfg option? */
#ifndef WIN32
#define TOMENET_WORLDS
#endif

#if 0 /* moved to tomenet.cfg.. */
#define BIND_NAME "tomenet.eu"
#define	BIND_IP "64.53.71.115"
#endif


#if 0	/* not used for good, most likely. DELETEME */
/*
 * OPTION: Set a vhost bind address.  This is only used if you have
 * multiple IP's on a single box, and care which one the server
 * binds too, ( for name purposes, perhaps ).
 * Note that this allows multiple servers to run on a single
 * machine as well.
 * Probably almost never used.
*/

/* Define the password for the server console, used if NEW_SERVER_CONSOLE
 * is defined below.  Provides authentication for the tomenetconsole program.
 */
#define		CONSOLE_PASSWORD	"change_me"


/* Define the name of a special administration character who gets
 * special powers, and will hopefully eventually get wizard mode.
 * Better documentation of this feature is needed.
 * In the future these two characters will probably be combined.
 */

#define 	ADMIN_WIZARD	"Serverchez"
#define		DUNGEON_MASTER	"DungeonMaster"

/* for the unique respawn... */
#define COME_BACK_TIME 480

/* Base probability of a level unstaticing */
/* Roughly once an hour (10 fps, 3600 seconds in an hour) */
#define LEVEL_UNSTATIC_PROB 36000


/* OPTION : Keep the town backwards compatible with some previous development
 * versions, specifically those that have a broken auction house.  You probably
 * don't want to enable this unless you have been running a development version of
 * the code that has a 'store 9' in it.
 */
 /* #define	DEVEL_TOWN_COMPATIBILITY */ 
#endif	/* 0 */

/*
 * OPTION: Use wider corrdiors (room for two people abreast).
 */
#define WIDE_CORRIDORS

/*
 * OPTION: Allow players to interact.  This has many effects, mainly
 * that people can hit each other to do damage, and spells will now
 * harm other players when they hit.
 */
/* obsolete - set USE_PK_RULES in tomenet.cfg instead. */
/*#define PLAYER_INTERACTION */

/*
 * OPTION: Hack -- Compile in support for "Spoiler Generation"
 */
/* #define ALLOW_SPOILERS */


/*
 * OPTION: Allow "do_cmd_colors" at run-time
 */
#define ALLOW_COLORS

/*
 * OPTION: Allow "do_cmd_visuals" at run-time
 */
#define ALLOW_VISUALS

/*
 * OPTION: Allow "do_cmd_macros" at run-time
 */
#define ALLOW_MACROS


/*
 * OPTION: Allow characteres to be "auto-rolled"
 */
/* #define ALLOW_AUTOROLLER */


/*
 * OPTION: Allow monsters to "flee" when hit hard
 */
#define ALLOW_FEAR

/*
 * OPTION: Allow monsters to "flee" from strong players
 */
#define ALLOW_TERROR


/*
 * OPTION: Allow parsing of the ascii template files in "init.c".
 * This must be defined if you do not have valid binary image files.
 * It should be usually be defined anyway to allow easy "updating".
 */
#define ALLOW_TEMPLATES

/*
 * OPTION: Allow loading of pre-2.7.0 savefiles.  Note that it takes
 * about 15K of code in "save-old.c" to parse the old savefile format.
 * Angband 2.8.0 will ignore a lot of info from pre-2.7.0 savefiles.
 */
/* #define ALLOW_OLD_SAVEFILES */


/*
 * OPTION: Delay the loading of the "f_text" array until it is actually
 * needed, saving ~1K, since "feature" descriptions are unused.
 */
/* #define DELAY_LOAD_F_TEXT */

/*
 * OPTION: Delay the loading of the "k_text" array until it is actually
 * needed, saving ~1K, since "object" descriptions are unused.
 */
/* #define DELAY_LOAD_K_TEXT */

/*
 * OPTION: Delay the loading of the "a_text" array until it is actually
 * needed, saving ~1K, since "artifact" descriptions are unused.
 */
/* #define DELAY_LOAD_A_TEXT */

/*
 * OPTION: Delay the loading of the "e_text" array until it is actually
 * needed, saving ~1K, since "ego-item" descriptions are unused.
 */
/* #define DELAY_LOAD_E_TEXT */

/*
 * OPTION: Delay the loading of the "r_text" array until it is actually
 * needed, saving ~60K, but "simplifying" the "monster" descriptions.
 */
/* #define DELAY_LOAD_R_TEXT */

/*
 * OPTION: Delay the loading of the "v_text" array until it is actually
 * needed, saving ~1K, but "destroying" the "vault" generation.
 */
/* #define DELAY_LOAD_V_TEXT */


/*
 * OPTION: Handle signals
 */
#define HANDLE_SIGNALS


/*
 * OPTION: Allow use of the "flow_by_smell" and "flow_by_sound"
 * software options, which enable "monster flowing".
 */
/* #define MONSTER_FLOW */



#if 0 /* moved to defines.h - C. Blue */
/*
 * OPTION: Maximum flow depth when using "MONSTER_FLOW"
 */
#define MONSTER_FLOW_DEPTH 32
#endif



/*
 * OPTION: Allow use of extended spell info	-DRS-
 */
#define DRS_SHOW_SPELL_INFO

/*
 * OPTION: Allow use of the monster health bar	-DRS-
 */
#define DRS_SHOW_HEALTH_BAR


/*
 * OPTION: Enable the "smart_learn" and "smart_cheat" options.
 * They let monsters make more "intelligent" choices about attacks
 * (including spell attacks) based on their observations of the
 * player's reactions to previous attacks.  The "smart_cheat" option
 * lets the monster know how the player would react to an attack
 * without actually needing to make the attack.  The "smart_learn"
 * option requires that a monster make a "failed" attack before
 * learning that the player is not harmed by that attack.
 *
 * This adds about 3K to the memory and about 5K to the executable.
 */
/*
 * NOTE: this option will be disabled unless it covers multi-player
 * situation.. and prolly never.
 */
/* #define DRS_SMART_OPTIONS */



/*
 * OPTION: Enable the "track_follow" and "track_target" options.
 * They let monsters follow the player's foot-prints, or remember
 * the player's recent locations.  This code has been removed from
 * the current version because it is being rewritten by Billy, and
 * until it is ready, it will not work.  Do not define this option.
 */
/* #define WDT_TRACK_OPTIONS */



/*
 * OPTION: Allow the use of "sound" in various places.
 */

/* --new sound system 'USE_SOUND_2010' in place utilizing SDL, state is experimental- C. Blue */
#define USE_SOUND
#define USE_SOUND_2010

/*
 * OPTION: Allow the use of "graphics" in various places
 */
/* #define USE_GRAPHICS */


/*
 * OPTION: Hack -- Macintosh stuff
 */
#ifdef MACINTOSH

/* Do not handle signals */
# undef HANDLE_SIGNALS

#endif


/*
 * OPTION: Hack -- Windows stuff
 */
#ifdef WINDOWS

/* Do not handle signals */
/* # undef HANDLE_SIGNALS */

#endif



/*
 * OPTION: Set the "default" path to the angband "lib" directory.
 *
 * See "main.c" for usage, and note that this value is only used on
 * certain machines, primarily Unix machines.  If this value is used,
 * it will be over-ridden by the "ANGBAND_PATH" environment variable,
 * if that variable is defined and accessable.  The final slash is
 * optional, but it may eventually be required.
 *
 * Using the value "lib" below tells Angband that, by default,
 * the user will run "angband" from the same directory that contains
 * the "lib" directory.  This is a reasonable (but imperfect) default.
 *
 * If at all possible, you should change this value to refer to the
 * actual location of the "lib" folder, for example, "/tmp/angband/lib/"
 * or "/usr/games/lib/angband/", or "/pkg/angband/lib".
 */
#ifndef DEFAULT_PATH
# define DEFAULT_PATH "lib"
#endif


/*
 * On multiuser systems, add the "uid" to savefile names
 */
#ifdef SET_UID
# define SAVEFILE_USE_UID
#endif


/*
 * OPTION: Check the "time" against "lib/file/hours.txt"
 */
/* #define CHECK_TIME */

/*
 * OPTION: Check the "load" against "lib/file/load.txt"
 * This may require the 'rpcsvs' library
 */
/* #define CHECK_LOAD */


/*
 * OPTION: For some brain-dead computers with no command line interface,
 * namely Macintosh, there has to be some way of "naming" your savefiles.
 * The current "Macintosh" hack is to make it so whenever the character
 * name changes, the savefile is renamed accordingly.  But on normal
 * machines, once you manage to "load" a savefile, it stays that way.
 * Macintosh is particularly weird because you can load savefiles that
 * are not contained in the "lib:save:" folder, and if you change the
 * player's name, it will then save the savefile elsewhere.  Note that
 * this also gives a method of "bypassing" the "VERIFY_TIMESTAMP" code.
 */
#if defined(MACINTOSH) || defined(WINDOWS) || defined(AMIGA)
# define SAVEFILE_MUTABLE
#endif


/*
 * OPTION: Capitalize the "user_name" (for "default" player name)
 * This option is only relevant on SET_UID machines.
 */
#define CAPITALIZE_USER_NAME



/*
 * OPTION: Shimmer Multi-Hued monsters/objects
 */
#define SHIMMER_MONSTERS
#define SHIMMER_OBJECTS


/*
 * OPTION: Person to bother if something goes wrong.
 */
#define MAINTAINER	"c_blue@gmx.net"


/*
 * OPTION: Have the server respond to commands typed in on its tty.
 */
/*define SERVER_CONSOLE */

/*
 * OPTION: Enable a method to control the server from an external program.
 */
#define NEW_SERVER_CONSOLE

/*
 * OPTION: Default font (when using X11).
 */
#define DEFAULT_X11_FONT		"8x13"

/*
 * OPTION: Default fonts (when using X11)
 */
#define DEFAULT_X11_FONT_SCREEN		DEFAULT_X11_FONT
#define DEFAULT_X11_FONT_MIRROR		DEFAULT_X11_FONT
#define DEFAULT_X11_FONT_RECALL		DEFAULT_X11_FONT
#define DEFAULT_X11_FONT_CHOICE		DEFAULT_X11_FONT
#define DEFAULT_X11_FONT_TERM_4		DEFAULT_X11_FONT
#define DEFAULT_X11_FONT_TERM_5		DEFAULT_X11_FONT
#define DEFAULT_X11_FONT_TERM_6		DEFAULT_X11_FONT
#define DEFAULT_X11_FONT_TERM_7		DEFAULT_X11_FONT



/*
 * Hack -- Special "ancient machine" versions
 */
#if defined(USE_286) || defined(ANGBAND_LITE_MAC)
# ifndef ANGBAND_LITE
#  define ANGBAND_LITE
# endif
#endif

/*
 * OPTION: Attempt to minimize the size of the game
 */
#ifndef ANGBAND_LITE
/* #define ANGBAND_LITE */
#endif

/*
 * Hack -- React to the "ANGBAND_LITE" flag
 */
#ifdef ANGBAND_LITE
# undef ALLOW_COLORS
# undef ALLOW_VISUALS
# undef ALLOW_MACROS
# undef MONSTER_FLOW
# undef WDT_TRACK_OPTIONS
# undef DRS_SMART_OPTIONS
# undef ALLOW_OLD_SAVEFILES
# undef ALLOW_BORG
# undef ALLOW_WIZARD
# undef ALLOW_SPOILERS
# undef ALLOW_TEMPLATES
# undef DELAY_LOAD_R_TEXT
# define DELAY_LOAD_R_TEXT
#endif



/*
 * OPTION: Attempt to prevent all "cheating"
 */
/* #define VERIFY_HONOR */


/*
 * React to the "VERIFY_HONOR" flag
 */
#ifdef VERIFY_HONOR
# define VERIFY_SAVEFILE
# define VERIFY_CHECKSUMS
# define VERIFY_TIMESTAMPS
#endif


#define NEW_DUNGEON

#define USE_LUA

/*
 * OPTION: Random Uniques and Ego Monsters.
 * not fully implemented yet.	-Jir-
 * (3.2.2)
 * 
 * Don't remove this; sure it won't compile! :-/
 * To disable this, pls set MEGO_CHANCE to 0 instead.
 * TODO: make this option valid
 */
#define RANDUNIS
/* % chances of getting ego monsters(server/monster2.c) [18]*/
#define MEGO_CHANCE             18


/* Randart rarity (server/object2.c) [80] */
#define RANDART_RARITY  80


/*
 * Size of radius-tables, used to optimize blasts/AI etc. [16]
 *
 * 16 should be able to cover all the spells handled in project().
 * Code by the old way if the radius surpasses this(eg.*Destruction*).
 * it will occupy approximately (2x(r)^2x3.14+r+1) bytes.
 */
#define PREPARE_RADIUS	16

/*
 * OPTION: verbosity of server for debug msgs in stdout/tomenet.log.
 *
 * 0 - no debug msgs
 * 1 - very recent debug msgs
 * 2 - most of recent debug msgs
 * 3 - most of debug msgs (noisy)
 * 4 - everything
 */
#define DEBUG_LEVEL 1

/*
 * OPTION: verbosity of server for anti-cheeze msgs in stdout/tomenet.log.
 *
 * 0 - no cheeze msgs
 * 1 - very limited cheeze msgs
 *     (nothing so far)
 * 2 - most of cheeze msgs
 *     hourly cheeze() log, money transaction
 * 3 - most of cheeze msgs (noisy)
 *     item transaction
 * 4 - everything
 *     cheeze() log every minute
 */
#define CHEEZELOG_LEVEL 3

/*
 * OPTION: verbosity of server for players.
 * (XXX This should be handled by client-side option;
 *  cf. taciturn_messages, last_words, speak_unique)
 *
 * 0 - deadly quiet [The first message you'll receive might be 'You die.']
 * 1 - seldom speaks ['You hit ..!' etc. are surpressed.]
 * 2 - (default)
 * 3 - chatterbox [dying msg, monster speach etc.]
 *
 * NOTE: chatterbox levels 0-1 are not implemented yet;
 *       set this to 3 basically.
 */
#define CHATTERBOX_LEVEL	3

/*
 * OPTION: suppress visual effects in project()	[3, 3]
 * To disable, comment it out.	(melee2.c, spells1.c, variables.c)
 *
 * If you recalled into a pack of hounds, the visual effects of breathes
 * slows the server/client and make it almost impossible to control.
 * This option prevents this by limiting the maximum # of blasts
 * per (PROJECTION_FLUSH_LIMIT_TURNS * MONSTER_TURNS) turns.
 *
 * NOTE: of course, it can happen that you cannot see the attacks made.
 *
 * XXX: Visual effects like this should be done in 100% client-side,
 * so that it doesn't affect connection.
 * This can be done by adjusting TERM_XTRA_DELAY, for example.
 * (see also: thin_down_flush)
 */
#define PROJECTION_FLUSH_LIMIT 9
#define PROJECTION_FLUSH_LIMIT_TURNS 3

/* 
 * OPTION: the interval of monster turns.	[6]
 * If the value is 6 and FPS is 60, monsters are processed 10 times/second.
 * This should be of *great* help for slow servers :)
 */
#define MONSTER_TURNS	6

/* Smoother load distribution:
   Handle monsters actually once per turn but only 1/MONSTER_TURNS of them: */
#define PROCESS_MONSTERS_DISTRIBUTE

#define NPC_TURNS	6	/* Programmable NPC Turns */

/*
 * OPTION: allow sanity display.
 *
 * affects both client and server, so be warned. TBH, I'm tired.
 */
#define SHOW_SANITY

/* spells1.c, cmd2.c */
/* Chance of bolt/ball harming party-member by accident. [10] */
#define FRIEND_FIRE_CHANCE	0
/* Chance of bolt/ball harming neutral-player by accident. [50] */
#define NEUTRAL_FIRE_CHANCE	0

/* OPTION: allow monsters to carry objects. */
#define MONSTER_INVENTORY

/* 
 * OPTION: default radii used for some kinds of magic (like detection).
 * artifacts uses (DEFAULT_RADIUS * 2) instead.
 *
 * TODO: DEFAULT_RADIUS_SPELL should be based on skills!
 */
#define DEFAULT_RADIUS			18
/*#define DEFAULT_RADIUS_SPELL(p_ptr)	(DEFAULT_RADIUS - 5 + p_ptr->lev / 5) */
#define DEFAULT_RADIUS_DEV(p_ptr)	(DEFAULT_RADIUS - 5 + p_ptr->skill_dev / 8)
/* NOTE: skill_dev is already affected by SKILL_DEVICE
 * get_skill_scale(p_ptr, SKILL_DEVICE, 100) */

/*
 * Monster themes above this value in percent are told to the players
 * via '/ver 1'. (If below ... that's "spice" ;)
 * (server/cmd4.c, server/monster2.c)
 */
#define TELL_MONSTER_ABOVE	15

/*
 * OPTION: Use 'old' update_view function (init2.c, cave.c)
 * The older one proved out to be more effective and fast, so define it.
 * (Newer one allows more excellent LOS handling, and it should be faster
 *  if update_lite is integrated with it.)
 */
#define USE_OLD_UPDATE_VIEW

/* OPTION: Use mangband-style houses in place of Vanilla/ToME ones.
 * (make clean!)
 */
/* #define USE_MANG_HOUSE_ONLY	*//* Not tested yet */
#define USE_MANG_HOUSE
#define MANG_HOUSE_RATE	85	/* 80 is important to allow newbies without */
				/* 300k cash to spend to buy a house too! (C. Blue) */


/*
 * Below this line are client-only options.
 * Probably we'd better separate them to another file?	- Jir -
 */

/* 
 * OPTION: max # of history for chat, slash-cmd etc.
 */
#define MSG_HISTORY_MAX	1000

#define EVIL_METACLIENT

#define CLIENT_SHIMMER
#endif

/*
 * Use the new meta scheme to do neater things
 */
#define EXPERIMENTAL_META
