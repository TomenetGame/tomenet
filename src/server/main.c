/* File: main.c */

/* Purpose: initialization, main() function and main loop */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"

#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

/*
 * Some machines have a "main()" function in their "main-xxx.c" file,
 * all the others use this file for their "main()" function.
 */

/*
 * A hook for "quit()".
 *
 * Close down, then fall back into "quit()".
 *
 * Unnecessary, as the server doesn't open any "terms".  --KLJ--
 */
static void quit_hook(cptr s) {
#ifdef UNIX_SOCKETS
	SocketCloseAll();
#endif
}



/*
 * Set the stack size (for the Amiga)
 */
#ifdef AMIGA
# include <dos.h>
__near long __stack = 32768L;
#endif


/*
 * Set the stack size and overlay buffer (see main-286.c")
 */
#ifdef USE_286
# include <dos.h>
extern unsigned _stklen = 32768U;
extern unsigned _ovrbuffer = 0x1500;
#endif

/*
 * Initialize and verify the file paths, and the score file.
 *
 * Use the ANGBAND_PATH environment var if possible, else use
 * DEFAULT_PATH, and in either case, branch off appropriately.
 *
 * First, we'll look for the ANGBAND_PATH environment variable,
 * and then look for the files in there.  If that doesn't work,
 * we'll try the DEFAULT_PATH constant.  So be sure that one of
 * these two things works...
 *
 * We must ensure that the path ends with "PATH_SEP" if needed,
 * since the "init_file_paths()" function will simply append the
 * relevant "sub-directory names" to the given path.
 *
 * Note that the "path" must be "Angband:" for the Amiga, and it
 * is ignored for "VM/ESA", so I just combined the two.
 */
static void init_paths(void) {
	char path[MAX_PATH_LENGTH];

#if defined(AMIGA) || defined(VM)

	/* Hack -- prepare "path" */
	strcpy(path, "Tomenet:");

#else /* AMIGA / VM */

	cptr tail;

	/* Get the environment variable */
	tail = getenv("TOMENET_PATH");

	/* Use the angband_path, or a default */
	strcpy(path, tail ? tail : DEFAULT_PATH);

	/* Hack -- Add a path separator (only if needed) */
	if (!suffix(path, PATH_SEP)) strcat(path, PATH_SEP);

#endif /* AMIGA / VM */

	/* Initialize */
	init_file_paths(path);
}


#ifdef COMBO_AM_IC_CAP
static void init_combo_am_ic_cap(void) {
	int min_slope = (10000 * (COMBO_AM_IC_CAP - INTERCEPT_CAP)) / ANTIMAGIC_CAP; /* max IC, n AM */
	int max_slope = 10000; /* 0 IC, n AM */

	slope_fak = (max_slope - min_slope) / INTERCEPT_CAP; /* change of slope depending on initial IC */
}
#endif


/*
 * The server config files were moved to lib/config with version 4.5.2.
 * The account file was also moved to lib/save. This code will
 * automatically create lib/config and move files to their new
 * locations. - mikaelh
 */
static void migrate_files(void) {
	char buf[1024];
	int errval;
	DIR *dp;
	FILE *fp;

	/* Check that lib/config exists */
	dp = opendir(ANGBAND_DIR_CONFIG);
	if (!dp) {
		/* Try to create lib/config */
#ifdef WINDOWS
		if (mkdir(ANGBAND_DIR_CONFIG) != 0) {
#else
		if (mkdir(ANGBAND_DIR_CONFIG, 0777) != 0) {
#endif
			errval = errno;
			fprintf(stderr, "Failed to create directory \"%s\"! (errno = %d)", ANGBAND_DIR_CONFIG, errval);
		} else {
			/* Try to move files to lib/config */
			path_build(buf, 1024, ANGBAND_DIR_CONFIG, "badnames.txt");
			rename("badnames.txt", buf);

			path_build(buf, 1024, ANGBAND_DIR_CONFIG, "nonswearing.txt");
			rename("nonswearing.txt", buf);

			path_build(buf, 1024, ANGBAND_DIR_CONFIG, "swearing.txt");
			rename("swearing.txt", buf);

			path_build(buf, 1024, ANGBAND_DIR_CONFIG, "tomenet.cfg");
			rename("tomenet.cfg", buf);
		}
	} else {
		closedir(dp);
	}

	/* Check the new location of account file */
	path_build(buf, 1024, ANGBAND_DIR_SAVE, "tomenet.acc");
	fp = fopen(buf, "r");
	if (!fp) {
		/* Check the old location */
		fp = fopen("tomenet.acc", "r");
		if (fp) {
			fclose(fp);

			/* Move from old to new location */
			if (rename("tomenet.acc", buf) != 0) {
				fprintf(stderr, "Could not move tomenet.acc to new location in %s!\n", buf);
			}
		}
	} else {
		fclose(fp);
	}
}


static void writepid(char *str) {
	FILE *fp;
	fp = fopen(str, "wb");
	if (fp) {
		fprintf(fp, "%d\n", (int) getpid());
		fclose(fp);
	}
}


/* just call a lua function in custom.lua, usually unused - C. Blue */
static void post_init_lua(void) {
	int h = 0, m = 0, s = 0, dwd = 0, dd = 0, dm = 0, dy = 0;
	time_t now;
	struct tm *tmp;

	time(&now);
	tmp = localtime(&now);
	h = tmp->tm_hour;
	m = tmp->tm_min;
	s = tmp->tm_sec;
	get_date(&dwd, &dd, &dm, &dy);
	exec_lua(0, format("server_startup_post(\"%s\", %d, %d, %d, %d, %d, %d, %d)", showtime(), h, m, s, dwd, dd, dm, dy));
}


/*
 * Some machines can actually parse command line args
 *
 * XXX XXX XXX The "-c", "-d", and "-i" options should probably require
 * that their "arguments" do NOT end in any path separator.
 *
 * The "path" options should probably be simplified into some form of
 * "-dWHAT=PATH" syntax for simplicity.
 */
int main(int argc, char *argv[]) {
	bool new_game = FALSE, all_terrains = FALSE, dry_Bree = FALSE, new_wilderness = FALSE, new_flavours = FALSE, new_houses = FALSE;
	bool config_specified = FALSE;
	char buf[1024];
	int catch_signals = TRUE;
#ifdef WINDOWS
	WSADATA wsadata;
	/* Initialize WinSock */
	WSAStartup(MAKEWORD(1, 1), &wsadata);
#endif
  

	/* Save the "program name" */
	argv0 = argv[0];


#ifdef USE_286
	/* Attempt to use XMS (or EMS) memory for swap space */
	if (_OvrInitExt(0L, 0L)) { _OvrInitEms(0, 0, 64); }
#endif


#ifdef SET_UID

	/* Default permissions on files */
	(void)umask(022);

# ifdef SECURE
	/* Authenticate */
	Authenticate();
# endif

#endif


	/* Get the file paths */
	init_paths();

	/* Possibly move the server config files and the account file */
	migrate_files();

	/* Initialize the server log file */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "tomenet.log");

	/* Open the file */
	s_setup(buf);

	/* Initialize the server rfe file */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "tomenet.rfe");

	/* Open the file */
	s_setupr(buf);

	/* Initiliaze PID file */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "tomenet.pid");

	/* Write PID */
	writepid(buf);

	/* Tell the scripts that the server is up now */
	update_check_file();

	/* Acquire the version strings */
	version_build();

#ifdef SET_UID

	/* Initialize the "time" checker */
	if (check_time_init() || check_time())
		quit("The gates to Angband are closed (bad time).");

	/* Initialize the "load" checker */
	if (check_load_init() || check_load())
		quit("The gates to Angband are closed (bad load).");

#endif

	path_build(buf, 1024, ANGBAND_DIR_CONFIG, "tomenet.cfg");
	MANGBAND_CFG = string_make(buf);

	/* Process the command line arguments */
	for (--argc, ++argv; argc > 0; --argc, ++argv) {
		/* Require proper options */
		if (argv[0][0] != '-') goto usage;

		/* Analyze option */
		switch (argv[0][1]) {
			case 'c':
			ANGBAND_DIR_USER = &argv[0][2];
			break;

#ifndef VERIFY_SAVEFILE
			case 'd':
			ANGBAND_DIR_SAVE = &argv[0][2];
			break;
#endif

			case 'i':
			ANGBAND_DIR_TEXT = &argv[0][2];
			break;

			case 'r':
			new_game = TRUE;
			break;

			case 'b':
			dry_Bree = TRUE;
			break;

			case 'a':
			all_terrains = TRUE;
			break;

			case 'w':
			new_wilderness = TRUE;
			break;

			case 'f':
			new_flavours = TRUE;
			break;

			case 'h':
			new_houses = TRUE;
			break;

			case 'z':
			catch_signals = FALSE;
			break;

			case 'm':
			MANGBAND_CFG = &argv[0][2];
			config_specified = TRUE;
			break;


			default:
			usage:

			/* Note -- the Term is NOT initialized */
			puts(longVersion);
			puts("Usage: tomenet.server [options]");
			puts("  -r        Reset the server (implies -w and -f)");
			puts("  -w        Reset the server partially: New wilderness");
			puts("  -f        Reset the server partially: New flavours");
			puts("  -a        (On server creation!) Ensure that all terrain types are created");
			puts("  -b        (On server creation!) Don't allow watery wilderness around Bree");
			puts("  -h        Reinitialize houses");
			puts("  -z        Don't catch signals");
			puts("  -c<path>  Look for pref files in the directory <path>");
			puts("  -d<path>  Look for save files in the directory <path>");
			puts("  -i<path>  Look for info files in the directory <path>");
			puts("  -m<file>  Specify configuration <file>");

			/* Actually abort the process */
			quit(NULL);
		}
	}

	/* Tell "quit()" to call "Term_nuke()" */
	quit_aux = quit_hook;


	/* Catch nasty signals */
	if (catch_signals == TRUE)
		signals_init();
	
	/* Catch nasty "signals" on Windows */
#ifdef WINDOWS
#ifndef HANDLE_SIGNALS
	setup_exit_handler();
#endif
#endif

	/* Display the 'news' file */
	show_news();

	/* Load the tomenet.cfg options */
	if (!load_server_cfg() && config_specified)
		quit(NULL);

	/* Init turn overflow protection */
	/* At least 30 minutes buffer for login process. */
	turn_overflow = 2147483647 - (cfg.fps * 60 * 60);

	init_players();

	/* Initialize the arrays */
	init_some_arrays();

	/* After init_alloc() has been called: */
	init_treasure_classes();

#ifdef COMBO_AM_IC_CAP
	/* Initialize various stuff */
	init_combo_am_ic_cap();
#endif

	/* Load dynamic quest data */
	load_quests();

#ifdef ENABLE_GO_GAME
	/* Initialize & power up the Go AI */
	go_engine_init();
#endif

	post_init_lua();


	/* Play the game */
	play_game(new_game, all_terrains, dry_Bree, new_wilderness, new_flavours, new_houses);

	/* Quit */
	quit(NULL);

	/* Exit */
	return (0);
}

#ifdef DUMB_WIN
int FAR PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrevInst,
                       LPSTR lpCmdLine, int nCmdShow)
{
	MSG      msg;

//        main(0, NULL);

	/* Process messages forever */
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

        main(0, NULL);

	return (0);
}
#endif
