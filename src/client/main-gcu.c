/* File: main-gcu.c */

/*
 * Copyright (c) 1997 Ben Harrison, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

/* Purpose: Allow use of Unix "curses" with Angband -BEN- */


/*
 * To use this file, you must define "USE_GCU" in the Makefile.
 *
 * Hack -- note that "angband.h" is included AFTER the #ifdef test.
 * This was necessary because of annoying "curses.h" silliness.
 *
 * Note that this file is not "intended" to support non-Unix machines,
 * nor is it intended to support VMS or other bizarre setups.
 *
 * Also, this package assumes that the underlying "curses" handles both
 * the "nonl()" and "cbreak()" commands correctly, see the "OPTION" below.
 *
 * This code should work with most versions of "curses" or "ncurses",
 * and the "main-ncu.c" file (and USE_NCU define) are no longer used.
 *
 * See also "USE_CAP" and "main-cap.c" for code that bypasses "curses"
 * and uses the "termcap" information directly, or even bypasses the
 * "termcap" information and sends direct vt100 escape sequences.
 *
 * XXX XXX XXX This file provides only a single "term" window.
 *
 * The "init" and "nuke" hooks are built so that only the first init and
 * the last nuke actually do anything, but the other functions are not
 * "correct" for multiple windows.  Minor changes would also be needed
 * to allow the system to handle the "locations" of the various windows.
 *
 * But in theory, it should be possible to allow a 50 line screen to be
 * split into two (or more) sub-screens.
 *
 * XXX XXX XXX Consider the use of "savetty()" and "resetty()".
 */

#include "angband.h"


#ifdef USE_GCU


/*
 * Hack -- play games with "bool"
 */
#undef bool

/*
 * Include the proper "header" file
 */
#ifdef USE_NCURSES
# include <ncurses.h>
#else
# include <curses.h>
#endif

typedef struct term_data term_data;
struct term_data
{
	term t;

	int rows;
	int cols;

	WINDOW *win;
};

#define MAX_TERM_DATA 4

static term_data data[MAX_TERM_DATA];


/*
 * Hack -- try to guess which systems use what commands
 * Hack -- allow one of the "USE_Txxxxx" flags to be pre-set.
 * Mega-Hack -- try to guess when "POSIX" is available.
 * If the user defines two of these, we will probably crash.
 */
#if !defined(USE_TPOSIX)
# if !defined(USE_TERMIO) && !defined(USE_TCHARS)
#  if defined(_POSIX_VERSION)
#   define USE_TPOSIX
#  else
#   if defined(USG) || defined(linux) || defined(SOLARIS)
#    define USE_TERMIO
#   else
#    define USE_TCHARS
#   endif
#  endif
# endif
#endif

/*
 * Hack -- Amiga uses "fake curses" and cannot do any of this stuff
 */
#if defined(AMIGA)
# undef USE_TPOSIX
# undef USE_TERMIO
# undef USE_TCHARS
#endif




/*
 * POSIX stuff
 */
#ifdef USE_TPOSIX
# include <sys/ioctl.h>
# include <termios.h>
#endif

/*
 * One version needs this file
 */
#ifdef USE_TERMIO
# include <sys/ioctl.h>
# include <termio.h>
#endif

/*
 * The other needs this file
 */
#ifdef USE_TCHARS
# include <sys/ioctl.h>
# include <sys/resource.h>
# include <sys/param.h>
# include <sys/file.h>
# include <sys/types.h>
#endif


/*
 * XXX XXX Hack -- POSIX uses "O_NONBLOCK" instead of "O_NDELAY"
 *
 * They should both work due to the "(i != 1)" test below.
 */
#ifndef O_NDELAY
# define O_NDELAY O_NONBLOCK
#endif


/*
 * OPTION: some machines lack "cbreak()"
 * On these machines, we use an older definition
 */
/* #define cbreak() crmode() */


/*
 * OPTION: some machines cannot handle "nonl()" and "nl()"
 * On these machines, we can simply ignore those commands.
 */
/* #define nonl() */
/* #define nl() */


/*
 * Save the "normal" and "angband" terminal settings
 */

#ifdef USE_TPOSIX

static struct termios  norm_termios;

static struct termios  game_termios;

#endif

#ifdef USE_TERMIO

static struct termio  norm_termio;

static struct termio  game_termio;

#endif

#ifdef USE_TCHARS

static struct ltchars norm_special_chars;
static struct sgttyb  norm_ttyb;
static struct tchars  norm_tchars;
static int            norm_local_chars;

static struct ltchars game_special_chars;
static struct sgttyb  game_ttyb;
static struct tchars  game_tchars;
static int            game_local_chars;

#endif



/*
 * Hack -- Number of initialized "term" structures
 */
static int active = 0;


/*
 * The main screen information
 */
/*static term term_screen_body;*/


#ifdef A_COLOR

/*
 * Hack -- define "A_BRIGHT" to be "A_BOLD", because on many
 * machines, "A_BRIGHT" produces ugly "inverse" video.
 */
#ifndef A_BRIGHT
# define A_BRIGHT A_BOLD
#endif

/*
 * Software flag -- we are allowed to use color
 */
static int can_use_color = TRUE;

/*
 * Software flag -- we are allowed to change the colors
 */
static int can_fix_color = FALSE;

/*
 * Software flag -- we are allowed to use a predefined 256 color palette
 */
static int can_use_256_color = TRUE;

/*
 * Simple Angband to Curses color conversion table
 */
static int colortable[16];

#endif

void resize_main_window_gcu(int cols, int rows);


/* Remember original terminal colours to restore on exit */
static short int cor[256], cog[256], cob[256];



/*
 * Place the "keymap" into its "normal" state
 */
static void keymap_norm(void) {

#ifdef USE_TPOSIX

	/* restore the saved values of the special chars */
	(void)tcsetattr(0, TCSAFLUSH, &norm_termios);

#endif

#ifdef USE_TERMIO

	/* restore the saved values of the special chars */
	(void)ioctl(0, TCSETA, (char *)&norm_termio);

#endif

#ifdef USE_TCHARS

	/* restore the saved values of the special chars */
	(void)ioctl(0, TIOCSLTC, (char *)&norm_special_chars);
	(void)ioctl(0, TIOCSETP, (char *)&norm_ttyb);
	(void)ioctl(0, TIOCSETC, (char *)&norm_tchars);
	(void)ioctl(0, TIOCLSET, (char *)&norm_local_chars);

#endif

}


/*
 * Place the "keymap" into the "game" state
 */
static void keymap_game(void) {

#ifdef USE_TPOSIX

	/* restore the saved values of the special chars */
	(void)tcsetattr(0, TCSAFLUSH, &game_termios);

#endif

#ifdef USE_TERMIO

	/* restore the saved values of the special chars */
	(void)ioctl(0, TCSETA, (char *)&game_termio);

#endif

#ifdef USE_TCHARS

	/* restore the saved values of the special chars */
	(void)ioctl(0, TIOCSLTC, (char *)&game_special_chars);
	(void)ioctl(0, TIOCSETP, (char *)&game_ttyb);
	(void)ioctl(0, TIOCSETC, (char *)&game_tchars);
	(void)ioctl(0, TIOCLSET, (char *)&game_local_chars);

#endif

}


/*
 * Save the normal keymap
 */
static void keymap_norm_prepare(void) {

#ifdef USE_TPOSIX

	/* Get the normal keymap */
	tcgetattr(0, &norm_termios);

#endif

#ifdef USE_TERMIO

	/* Get the normal keymap */
	(void)ioctl(0, TCGETA, (char *)&norm_termio);

#endif

#ifdef USE_TCHARS

	/* Get the normal keymap */
	(void)ioctl(0, TIOCGETP, (char *)&norm_ttyb);
	(void)ioctl(0, TIOCGLTC, (char *)&norm_special_chars);
	(void)ioctl(0, TIOCGETC, (char *)&norm_tchars);
	(void)ioctl(0, TIOCLGET, (char *)&norm_local_chars);

#endif

}


/*
 * Save the keymaps (normal and game)
 */
static void keymap_game_prepare(void) {

#ifdef USE_TPOSIX

	/* Acquire the current mapping */
	tcgetattr(0, &game_termios);

	/* Force "Ctrl-C" to interupt */
	game_termios.c_cc[VINTR] = (char)3;

	/* Force "Ctrl-Z" to suspend */
	game_termios.c_cc[VSUSP] = (char)26;

	/* Hack -- Leave "VSTART/VSTOP" alone */

	/* Disable the standard control characters */
	game_termios.c_cc[VQUIT] = (char)-1;
	game_termios.c_cc[VERASE] = (char)-1;
	game_termios.c_cc[VKILL] = (char)-1;
	game_termios.c_cc[VEOF] = (char)-1;
	game_termios.c_cc[VEOL] = (char)-1;

	/* Normally, block until a character is read */
	game_termios.c_cc[VMIN] = 1;
	game_termios.c_cc[VTIME] = 0;

#endif

#ifdef USE_TERMIO

	/* Acquire the current mapping */
	(void)ioctl(0, TCGETA, (char *)&game_termio);

	/* Force "Ctrl-C" to interupt */
	game_termio.c_cc[VINTR] = (char)3;

	/* Force "Ctrl-Z" to suspend */
	game_termio.c_cc[VSUSP] = (char)26;

	/* Hack -- Leave "VSTART/VSTOP" alone */

	/* Disable the standard control characters */
	game_termio.c_cc[VQUIT] = (char)-1;
	game_termio.c_cc[VERASE] = (char)-1;
	game_termio.c_cc[VKILL] = (char)-1;
	game_termio.c_cc[VEOF] = (char)-1;
	game_termio.c_cc[VEOL] = (char)-1;

	/* Normally, block until a character is read */
	game_termio.c_cc[VMIN] = 1;
	game_termio.c_cc[VTIME] = 0;

#endif

#ifdef USE_TCHARS

	/* Get the default game characters */
	(void)ioctl(0, TIOCGETP, (char *)&game_ttyb);
	(void)ioctl(0, TIOCGLTC, (char *)&game_special_chars);
	(void)ioctl(0, TIOCGETC, (char *)&game_tchars);
	(void)ioctl(0, TIOCLGET, (char *)&game_local_chars);

	/* Force suspend (^Z) */
	game_special_chars.t_suspc = (char)26;

	/* Cancel some things */
	game_special_chars.t_dsuspc = (char)-1;
	game_special_chars.t_rprntc = (char)-1;
	game_special_chars.t_flushc = (char)-1;
	game_special_chars.t_werasc = (char)-1;
	game_special_chars.t_lnextc = (char)-1;

	/* Force interupt (^C) */
	game_tchars.t_intrc = (char)3;

	/* Force start/stop (^Q, ^S) */
	game_tchars.t_startc = (char)17;
	game_tchars.t_stopc = (char)19;

	/* Cancel some things */
	game_tchars.t_quitc = (char)-1;
	game_tchars.t_eofc = (char)-1;
	game_tchars.t_brkc = (char)-1;

#endif

}




/*
 * Suspend/Resume
 */
static errr Term_xtra_gcu_alive(int v) {
	/* Suspend */
	if (!v) {
		/* Go to normal keymap mode */
		keymap_norm();

		/* Restore modes */
		nocbreak();
		echo();
		nl();

		/* Hack -- make sure the cursor is visible */
		Term_xtra(TERM_XTRA_SHAPE, 1);

		/* Flush the curses buffer */
		(void)refresh();

#ifdef SPECIAL_BSD
		/* this moves curses to bottom right corner */
		mvcur(curscr->cury, curscr->curx, LINES - 1, 0);
#else
		/* this moves curses to bottom right corner */
		mvcur(curscr->_cury, curscr->_curx, LINES - 1, 0);
#endif

		/* Exit curses */
		endwin();

		/* Flush the output */
		(void)fflush(stdout);
	}

	/* Resume */
	else {
		/* Refresh */
		/* (void)touchwin(curscr); */
		/* (void)wrefresh(curscr); */

		/* Restore the settings */
		cbreak();
		noecho();
		nonl();

		/* Go to angband keymap mode */
		keymap_game();
	}

	/* Success */
	return (0);
}




/*
 * Init the "curses" system
 */
static void Term_init_gcu(term *t) {
	term_data *td = (term_data *)(t->data);

	/* Count init's, handle first */
	if (active++ != 0) return;

	/* Erase the screen */
	(void)wclear(td->win);

	/* Reset the cursor */
	(void)wmove(td->win, 0, 0);

	/* Flush changes */
	(void)wrefresh(td->win);

	/* Game keymap */
	keymap_game();

	/* Tell select() to watch stdin - mikaelh */
	x11_socket = 0;

	/* One key may be encoded as multiple key presses */
	multi_key_macros = TRUE;
}


/*
 * Nuke the "curses" system
 */
static void Term_nuke_gcu(term *t) {
	term_data *td = (term_data *)(t->data);

	/* Delete this window */
	delwin(td->win);

	/* Count nuke's, handle last */
	if (--active != 0) return;

	/* Hack -- make sure the cursor is visible */
	Term_xtra(TERM_XTRA_SHAPE, 1);

#ifdef SPECIAL_BSD
	/* This moves curses to bottom right corner */
	mvcur(curscr->cury, curscr->curx, LINES - 1, 0);
#else
	/* This moves curses to bottom right corner */
	mvcur(curscr->_cury, curscr->_curx, LINES - 1, 0);
#endif

	/* Flush the curses buffer */
	(void)refresh();

	/* Exit curses */
	endwin();

	/* Flush the output */
	(void)fflush(stdout);

	/* Normal keymap */
	keymap_norm();
}




#ifdef USE_GETCH

/*
 * Process events, with optional wait
 */
static errr Term_xtra_gcu_event(int v) {
	int i, k;

	/* Wait */
	if (v) {
		/* Paranoia -- Wait for it */
		nodelay(stdscr, FALSE);

		/* Get a keypress */
		i = getch();

		/* Mega-Hack -- allow graceful "suspend" */
		for (k = 0; (k < 10) && (i == ERR); k++) i = getch();

		/* Broken input is special */
		if (i == ERR) exit(0);
		if (i == EOF) exit(0);
	}

	/* Do not wait */
	else {
		/* Do not wait for it */
		nodelay(stdscr, TRUE);

		/* Check for keypresses */
		i = getch();

		/* Wait for it next time */
		nodelay(stdscr, FALSE);

		/* None ready */
		if (i == ERR) return (1);
		if (i == EOF) return (1);
	}

	/* Enqueue the keypress */
	Term_keypress(i);

	/* Success */
	return (0);
}

#else	/* USE_GETCH */

/*
 * Process events (with optional wait)
 */
static errr Term_xtra_gcu_event(int v) {
	int i, k;
	char buf[2];

	/* Wait */
	if (v) {
		/* Wait for one byte */
		i = read(0, buf, 1);

		/* Hack -- Handle bizarre "errors" */
		if ((i <= 0) && (errno != EINTR)) exit(1);
	}

	/* Do not wait */
	else {
		/* Get the current flags for stdin */
		k = fcntl(0, F_GETFL, 0);

		/* Oops */
		if (k < 0) return (1);

		/* Tell stdin not to block */
		if (fcntl(0, F_SETFL, k | O_NDELAY) < 0) return (1);

		/* Read one byte, if possible */
		i = read(0, buf, 1);

		/* Replace the flags for stdin */
		if (fcntl(0, F_SETFL, k)) return (1);
	}

	/* Ignore "invalid" keys */
	if ((i != 1) || (!buf[0])) return (1);

	/* Enqueue the keypress */
	Term_keypress(buf[0]);

	/* Success */
	return (0);
}

#endif	/* USE_GETCH */


/*
 * Handle a "special request"
 */
static errr Term_xtra_gcu(int n, int v) {
	term_data *td = (term_data *)(Term->data);

	/* Analyze the request */
	switch (n) {
		/* Clear screen */
		case TERM_XTRA_CLEAR:
		touchwin(td->win);
		(void)wclear(td->win);
		return (0);

		/* Make a noise */
		case TERM_XTRA_NOISE:
		n = write(1, "\007", 1);
		return (0);

		/* Flush the Curses buffer */
		case TERM_XTRA_FRESH:
		(void)wrefresh(td->win);
		return (0);

#ifdef USE_CURS_SET

		/* Change the cursor visibility */
		case TERM_XTRA_SHAPE:
		curs_set(v);
		return (0);

#endif

		/* Suspend/Resume curses */
		case TERM_XTRA_ALIVE:
		return (Term_xtra_gcu_alive(v));

		/* Process events */
		case TERM_XTRA_EVENT:
		return (Term_xtra_gcu_event(v));

		/* Flush events */
		case TERM_XTRA_FLUSH:
		while (!Term_xtra_gcu_event(FALSE));
		return (0);

		/* Delay */
		case TERM_XTRA_DELAY:
		usleep(1000 * v);
		return (0);
	}

	/* Unknown */
	return (1);
}


/*
 * Actually MOVE the hardware cursor
 */
static errr Term_curs_gcu(int x, int y) {
	term_data *td = (term_data *)(Term->data);

	/* Literally move the cursor */
	wmove(td->win, y, x);

	/* Success */
	return (0);
}


/*
 * Erase a grid of space
 * Hack -- try to be "semi-efficient".
 */
static errr Term_wipe_gcu(int x, int y, int n) {
	term_data *td = (term_data *)(Term->data);

	/* Place cursor */
	wmove(td->win, y, x);

	/* Clear to end of line */
	if (x + n >= td->cols) {
		wclrtoeol(td->win);
	}
	/* Clear some characters */
	else {
		while (n-- > 0) waddch(td->win, ' ');
	}

	/* Success */
	return (0);
}






/*
 * Place some text on the screen using an attribute
 */
static errr Term_text_gcu(int x, int y, int n, byte a, cptr s) {
	term_data *td = (term_data *)(Term->data);
	int i;
	char text[255];

	/* Obtain a copy of the text */
	for (i = 0; i < n; i++) text[i] = s[i];
	text[n] = 0;

	/* Move the cursor and dump the string */
	wmove(td->win, y, x);

#ifdef A_COLOR
	/* Set the color */
	if (can_use_color) wattrset(td->win, colortable[a & 0x0F]);
#endif

	/* Add the text */
	waddstr(td->win, text);

	/* Success */
	return (0);
}



static errr term_data_init(term_data *td, int rows, int cols, int y, int x)
{
	term *t = &td->t;

	/* Make sure the window has a positive size */
	if (rows <= 0 || cols <= 0) return (0);

	td->win = newwin(rows, cols, y, x);

	if (!td->win)
	{
		plog("Failed to setup curses window.");
		return (-1);
	}

	/* Store size */
	td->rows = rows;
	td->cols = cols;

	/* Initialize the term */
	term_init(t, cols, rows, 256);

	/* Avoid the bottom right corner */
	t->icky_corner = TRUE;

	/* Erase with "white space" */
	t->attr_blank = TERM_WHITE;
	t->char_blank = ' ';

	/* Set some hooks */
	t->init_hook = Term_init_gcu;
	t->nuke_hook = Term_nuke_gcu;

	/* Set some more hooks */
	t->text_hook = Term_text_gcu;
	t->wipe_hook = Term_wipe_gcu;
	t->curs_hook = Term_curs_gcu;
	t->xtra_hook = Term_xtra_gcu;

	/* Save the data */
	t->data = td;

	/* Activate it */
	Term_activate(t);


	/* Success */
	return (0);
}

/*
 * Prepare "curses" for use by the file "term.c"
 *
 * Installs the "hook" functions defined above, and then activates
 * the main screen "term", which clears the screen and such things.
 *
 * Someone should really check the semantics of "initscr()"
 */
errr init_gcu(void) {
	int i;
	/*term *t = &term_screen_body;*/
	int num_term = 4, next_win = 0;


	/* hack -- work on Xfce4's 'Terminal' without requiring the user to set this */
	if (!getenv("TERM") ||
	    (!strcmp(getenv("TERM"), "xterm") &&
	    !getenv("XTERM_VERSION")))
		setenv("TERM", "xterm-16color", -1);


	/* BIG_MAP is currently not supported in GCU client */
	c_cfg.big_map = FALSE;
	Client_setup.options[CO_BIGMAP] = FALSE;
	(*option_info[CO_BIGMAP].o_var) = FALSE;
	screen_hgt = SCREEN_HGT;


	/* Extract the normal keymap */
	keymap_norm_prepare();


#if defined(USG)
	/* Initialize for USG Unix */
	if (initscr() == NULL) return (-1);
#else
	/* Initialize for others systems */
	if (initscr() == (WINDOW*)ERR) return (-1);
#endif


	/* Hack -- Require large screen, or Quit with message */
	i = ((LINES < 24) || (COLS < 80));
	if (i) quit("Angband needs an 80x24 'curses' screen");


	/* set OS-specific resize_main_window() hook */
	resize_main_window = resize_main_window_gcu;


#ifdef A_COLOR

	/*** Init the Color-pairs and set up a translation table ***/

	/* Do we have color, and enough color, available? */
	can_use_color = ((start_color() != ERR) && has_colors() &&
	                 (COLORS >= 8) && (COLOR_PAIRS >= 8));

	/* Can we change colors? */
	can_fix_color = (can_use_color && can_change_color() &&
	                 (COLOR_PAIRS >= 16) && (COLORS >= 16));

	/* Can we use 256 colors? */
	can_use_256_color = (can_use_color && (COLOR_PAIRS >= 16) &&
	                     (COLORS >= 256));

	/* Attempt to use customized colors */
	if (can_fix_color) {
		/* Prepare the color pairs */
		init_pair(0, 0, COLOR_BLACK);	/*black */
		init_pair(1, 1, COLOR_BLACK);	/*white */
		init_pair(2, 2, COLOR_BLACK);	/*grey */
		init_pair(3, 3, COLOR_BLACK);	/*orange */
		init_pair(4, 4, COLOR_BLACK);	/*red */
		init_pair(5, 5, COLOR_BLACK);	/*green */
		init_pair(6, 6, COLOR_BLACK);	/*blue */
		init_pair(7, 7, COLOR_BLACK);	/*umber */
		init_pair(8, 8, COLOR_BLACK);	/*dark grey */
		init_pair(9, 9, COLOR_BLACK);	/*light grey */
		init_pair(10, 10, COLOR_BLACK);	/*violet */
		init_pair(11, 11, COLOR_BLACK);	/*yellow */
		init_pair(12, 12, COLOR_BLACK);	/*light red */
		init_pair(13, 13, COLOR_BLACK);	/*light green */
		init_pair(14, 14, COLOR_BLACK);	/*light blue */
		init_pair(15, 15, COLOR_BLACK);	/*light umber */

		/* XXX XXX XXX Take account of "gamma correction" */

		for (i = 0; i < 16; i++) color_content(i, &cor[i], &cog[i], &cob[i]);

		/* Using the real colours if terminal supports redefining -  thanks Pepe for the patch */
		/* Prepare the "Angband Colors" */
		/* Use common colors */
		#define RED(i)   (((client_color_map[i] >> 16 & 0xff) * 1000 + 127) / 255)
		#define GREEN(i) (((client_color_map[i] >> 8 & 0xff) * 1000 + 127) / 255)
		#define BLUE(i)  (((client_color_map[i] & 0xff) * 1000 + 127) / 255)
		for (i = 0; i < 16; i++) {
			init_color(i, RED(i), GREEN(i), BLUE(i));
		}

		/* Prepare the "Angband Colors" */
		colortable[0] = (COLOR_PAIR(0) | A_NORMAL);	/* Black */
		colortable[1] = (COLOR_PAIR(1) | A_NORMAL);	/* White */
		colortable[2] = (COLOR_PAIR(2) | A_NORMAL);	/* Grey XXX */
		colortable[3] = (COLOR_PAIR(3) | A_NORMAL);	/* Orange XXX */
		colortable[4] = (COLOR_PAIR(4) | A_NORMAL);	/* Red */
		colortable[5] = (COLOR_PAIR(5) | A_NORMAL);	/* Green */
		colortable[6] = (COLOR_PAIR(6) | A_NORMAL);	/* Blue */
		colortable[7] = (COLOR_PAIR(7) | A_NORMAL);	/* Umber */

		colortable[8] = (COLOR_PAIR(8) | A_NORMAL);	/* Dark-grey XXX */
		colortable[9] = (COLOR_PAIR(9) | A_NORMAL);	/* Light-grey XXX */
		colortable[10] = (COLOR_PAIR(10) | A_NORMAL);	/* Purple */
		colortable[11] = (COLOR_PAIR(11) | A_NORMAL);	/* Yellow */
		colortable[12] = (COLOR_PAIR(12) | A_NORMAL);	/* Light Red XXX */
		colortable[13] = (COLOR_PAIR(13) | A_NORMAL);	/* Light Green */
		colortable[14] = (COLOR_PAIR(14) | A_NORMAL);	/* Light Blue */
		colortable[15] = (COLOR_PAIR(15) | A_NORMAL);	/* Light Umber XXX */
	}

	else if (can_use_256_color) {
		int j;
		int color_palette[16] = { 0 };
		
		/* Read the fixed color palette */
		for (i = 0; i < 256; i++) {
			color_content(i, &cor[i], &cog[i], &cob[i]);
		}

		/* Find the closest match in the palette for each desired color */
		for (i = 0; i < 16; i++) {
			int want_red = RED(i);
			int want_green = GREEN(i);
			int want_blue = BLUE(i);
			int best_idx = COLOR_WHITE;
			int best_distance = 3 * 256;
			for (j = 0; j < 256; j++) {
				int distance = abs(want_red - cor[j]) + abs(want_green - cog[j]) + abs(want_blue - cob[j]);
				if (distance < best_distance) {
					best_distance = distance;
					best_idx = j;
				}
			}
			color_palette[i] = best_idx;
		}

		/* Prepare the color pairs */
		for (i = 0; i < 16; i++) {
			init_pair(i, color_palette[i], COLOR_BLACK);
		}

		/* Prepare the "Angband Colors" */
		colortable[0] = (COLOR_PAIR(0) | A_NORMAL);	/* Black */
		colortable[1] = (COLOR_PAIR(1) | A_NORMAL);	/* White */
		colortable[2] = (COLOR_PAIR(2) | A_NORMAL);	/* Grey XXX */
		colortable[3] = (COLOR_PAIR(3) | A_NORMAL);	/* Orange XXX */
		colortable[4] = (COLOR_PAIR(4) | A_NORMAL);	/* Red */
		colortable[5] = (COLOR_PAIR(5) | A_NORMAL);	/* Green */
		colortable[6] = (COLOR_PAIR(6) | A_NORMAL);	/* Blue */
		colortable[7] = (COLOR_PAIR(7) | A_NORMAL);	/* Umber */

		colortable[8] = (COLOR_PAIR(8) | A_NORMAL);	/* Dark-grey XXX */
		colortable[9] = (COLOR_PAIR(9) | A_NORMAL);	/* Light-grey XXX */
		colortable[10] = (COLOR_PAIR(10) | A_NORMAL);	/* Purple */
		colortable[11] = (COLOR_PAIR(11) | A_NORMAL);	/* Yellow */
		colortable[12] = (COLOR_PAIR(12) | A_NORMAL);	/* Light Red XXX */
		colortable[13] = (COLOR_PAIR(13) | A_NORMAL);	/* Light Green */
		colortable[14] = (COLOR_PAIR(14) | A_NORMAL);	/* Light Blue */
		colortable[15] = (COLOR_PAIR(15) | A_NORMAL);	/* Light Umber XXX */
	}

	/* Attempt to use colors */
	else if (can_use_color) {
		/* Color-pair 0 is *always* WHITE on BLACK */
		/* Prepare the color pairs */
		init_pair(1, COLOR_RED,     COLOR_BLACK);
		init_pair(2, COLOR_GREEN,   COLOR_BLACK);
		init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
		init_pair(4, COLOR_BLUE,    COLOR_BLACK);
		init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(6, COLOR_CYAN,    COLOR_BLACK);
		init_pair(7, COLOR_BLACK,   COLOR_BLACK);

		/* Prepare the "Angband Colors" */
		colortable[0] = (COLOR_PAIR(7) | A_NORMAL);	/* Black */
		colortable[1] = (COLOR_PAIR(0) | A_BRIGHT);
		colortable[2] = (COLOR_PAIR(0) | A_NORMAL);
		colortable[3] = (COLOR_PAIR(3) | A_NORMAL);
		colortable[4] = (COLOR_PAIR(1) | A_NORMAL);	/* Red */
		colortable[5] = (COLOR_PAIR(2) | A_NORMAL);	/* Green */
		colortable[6] = (COLOR_PAIR(4) | A_NORMAL);	/* Blue */
		colortable[7] = (COLOR_PAIR(3) | A_NORMAL);	/* Umber */
		colortable[8] = (COLOR_PAIR(7) | A_BRIGHT);	/* Dark-grey XXX */
		colortable[9] = (COLOR_PAIR(0) | A_BRIGHT);	/* Light-grey as pure White */
		colortable[10] = (COLOR_PAIR(5) | A_NORMAL);	/* Purple */
		colortable[11] = (COLOR_PAIR(3) | A_BRIGHT);	/* Yellow */
		colortable[12] = (COLOR_PAIR(1) | A_BRIGHT);
		colortable[13] = (COLOR_PAIR(2) | A_BRIGHT);	/* Light Green */
		colortable[14] = (COLOR_PAIR(4) | A_BRIGHT);	/* Light Blue */
		colortable[15] = (COLOR_PAIR(3) | A_NORMAL);	/* Light Umber XXX */
	}
#endif


	/*** Low level preparation ***/

#ifdef USE_GETCH

	/* Paranoia -- Assume no waiting */
	nodelay(stdscr, FALSE);

#endif

	/* Prepare */
	cbreak();
	noecho();
	nonl();

	/* Extract the game keymap */
	keymap_game_prepare();


	/*** Now prepare the term(s) ***/

	for (i = 0; i < num_term; i++) {
		int rows, cols;
		int y, x;

		switch (i) {
			case 0: rows = 24;
				cols = 80;
				y = x = 0;
				break;
			case 1: rows = LINES - 25;
				cols = 80;
				y = 25;
				x = 0;
				break;
			case 2: rows = 24;
				cols = COLS - 81;
				y = 0;
				x = 81;
				break;
			case 3: rows = LINES - 25;
				cols = COLS - 81;
				y = 25;
				x = 81;
				break;
			default: rows = cols = 0;
				 y = x = 0;
				 break;
		}

		if (rows <= 0 || cols <= 0) continue;

		term_data_init(&data[next_win], rows, cols, y, x);

		ang_term[next_win] = Term;

		next_win++;
	}

	/* Activate the "Angband" window screen */
	Term_activate(&data[0].t);

	term_screen = &data[0].t;

	/* Success */
	return (0);
}

void gcu_restore_colours(void) {
	int i;
	for (i = 0; i < 16; i++) init_color(i, cor[i], cog[i], cob[i]);
}

/* for big_map mode */
void resize_main_window_gcu(int cols, int rows) {
#if 0 /* copy/pasted from USE_X11 -- todo: implement for ncurses */
	int wid, hgt;
	term_data *td = term_idx_to_term_data(0);
	term *t = ang_term[0]; //&screen

	term_prefs[0].columns = cols; //screen_wid + (SCREEN_PAD_X);
	term_prefs[0].lines = rows; //screen_hgt + (SCREEN_PAD_Y);

	wid = cols * td->fnt->wid;
	hgt = rows * td->fnt->hgt;

	/* Resize the windows if any "change" is needed */
	Infowin_set(td->outer);
	if ((Infowin->w != wid + 2) || (Infowin->h != hgt + 2)) {
		Infowin_set(td->outer);
		Infowin_resize(wid + 2, hgt + 2);
		Infowin_set(td->inner);
		Infowin_resize(wid, hgt);

		Term_activate(t);
		Term_resize(cols, rows);
	}
#else
	(void) cols; /* suppress compiler warning */
	(void) rows; /* suppress compiler warning */
#endif
}

#ifndef USE_X11
/* automatically store name+password to ini file if we're a new player? */
void store_crecedentials(void) {
	write_mangrc(TRUE);
}
#endif
#endif /* USE_GCU */
