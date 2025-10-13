/* $Id$ */
/* File: c-util.c */
/* Client-side utility stuff */

#include "angband.h"

#include <sys/time.h>
#ifndef WINDOWS
 #include <glob.h>
#endif

#define ENABLE_SUBWINDOW_MENU /* allow =f menu function for setting fonts/visibility of term windows */
//#ifdef ENABLE_SUBWINDOW_MENU
 #include <dirent.h> /* we now need it for scanning for audio packs too */
//#endif

#ifdef REGEX_SEARCH
/* For extract_url(): able to utilize regexps? */
 #define REGEX_URL
#endif

#if defined(REGEX_SEARCH) || defined(REGEX_URL)
 #include <regex.h>
 #define REGEX_ARRAY_SIZE 1
#endif

#define MACRO_USE_CMD	0x01
#define MACRO_USE_STD	0x02
#define MACRO_USE_HYB	0x04

/* Have the Macro Wizard generate target code in
   the form *tXXX- instead of XXX*t? - C. Blue */
#define MACRO_WIZARD_SMART_TARGET

/* This should be extended onto all top-line clearing/message prompting during macro execution,
   to avoid erasing %:bla lines coming from the macro, by overwriting them with useless prompts: */
//#define DONT_CLEAR_TOPLINE_IF_AVOIDABLE -- turned into an option instead: c_cfg.keep_topline

#define RAINY_TOMB /* Display rainy weather for the mood? +_+ - C. Blue */

#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
static bool inkey_location_keys = FALSE;
  /* Navigation keys/special keys sequence initializer */
  #define NAVI_KEY_SEQ_START		{ 31, 0}
 #ifdef WINDOWS
  /* CTRL pressed */
  #define NAVI_KEY_SEQ_CTRL		{ 67, 0}
  /* SHIFT pressed */
  #define NAVI_KEY_SEQ_SHIFT		{ 83 , 0}
  /* ALT pressed */
  #define NAVI_KEY_SEQ_ALT		{ 65 , 0}
  /* Shiftkey terminator marker */
  #define NAVI_KEY_SEQ_SHIFTKEY_TERM	{ 120, 0 }
  /* Static middle sequence */
  #define NAVI_KEY_SEQ_SKIP		{ 0 }
  /* Navigation key codes */
  #define NAVI_KEY_SEQ_UP		{ 52,  56, 0}
  #define NAVI_KEY_SEQ_RIGHT		{ 52,  68, 0}
  #define NAVI_KEY_SEQ_DOWN		{ 53,  48, 0}
  #define NAVI_KEY_SEQ_LEFT		{ 52,  66, 0}
  #define NAVI_KEY_SEQ_POS1		{ 52,  55, 0}
  #define NAVI_KEY_SEQ_END		{ 52,  70, 0}
  #define NAVI_KEY_SEQ_PAGEUP		{ 52,  57, 0}
  #define NAVI_KEY_SEQ_PAGEDOWN		{ 53,  49, 0}
  #define NAVI_KEY_SEQ_DEL		{ 53,  51, 0}
 #else /* POSIX, at least working on Linux/X11 */
  /* CTRL pressed */
  #define NAVI_KEY_SEQ_CTRL		{ 78, 0 }
  /* SHIFT pressed */
  #define NAVI_KEY_SEQ_SHIFT		{ 83, 0 }
  /* ALT pressed */
  #define NAVI_KEY_SEQ_ALT		{ 79, 0 }
  /* Shiftkey terminator marker */
  #define NAVI_KEY_SEQ_SHIFTKEY_TERM	{ 95, 0 }
  /* Static middle sequence */
  #define NAVI_KEY_SEQ_SKIP		{ 70, 70, 0 }
  /* Navigation key codes */
  #define NAVI_KEY_SEQ_UP		{ 53,  50, 0 }
  #define NAVI_KEY_SEQ_RIGHT		{ 53,  51, 0 }
  #define NAVI_KEY_SEQ_DOWN		{ 53,  52, 0 }
  #define NAVI_KEY_SEQ_LEFT		{ 53,  49, 0 }
  #define NAVI_KEY_SEQ_POS1		{ 53,  48, 0 }
  #define NAVI_KEY_SEQ_END		{ 53,  55, 0 }
  #define NAVI_KEY_SEQ_PAGEUP		{ 53,  53, 0 }
  #define NAVI_KEY_SEQ_PAGEDOWN		{ 53,  54, 0 }
  /* Depending on system/terminal, Backspace and Delete are both ASCII 8 and cannot be distinguished by us. :/ */
  #define NAVI_KEY_SEQ_DEL		{ 0 }
 #endif
  /* The closing marker of all special key sequences */
 #define NAVI_KEY_SEQ_TERM		{ 13, 0 }

 char nks_start[] = NAVI_KEY_SEQ_START;
 char nks_term[] = NAVI_KEY_SEQ_TERM;
 char nks_ctrl[] = NAVI_KEY_SEQ_CTRL;
 char nks_shift[] = NAVI_KEY_SEQ_SHIFT;
 char nks_alt[] = NAVI_KEY_SEQ_ALT;
 char nks_skterm[] = NAVI_KEY_SEQ_SHIFTKEY_TERM;
 char nks_skip[] = NAVI_KEY_SEQ_SKIP;
 char nks_u[] = NAVI_KEY_SEQ_UP;
 char nks_r[] = NAVI_KEY_SEQ_RIGHT;
 char nks_d[] = NAVI_KEY_SEQ_DOWN;
 char nks_l[] = NAVI_KEY_SEQ_LEFT;
 char nks_p[] = NAVI_KEY_SEQ_POS1;
 char nks_e[] = NAVI_KEY_SEQ_END;
 char nks_pu[] = NAVI_KEY_SEQ_PAGEUP;
 char nks_pd[] = NAVI_KEY_SEQ_PAGEDOWN;
 char nks_del[] = NAVI_KEY_SEQ_DEL;
 /* Minimum length to even start string comparisons, or it couln't possibly be a 'special' trigger key: */
 int nks_minlen;
#endif



#ifdef SET_UID
# ifndef HAS_USLEEP
/*
 * For those systems that don't have "usleep()" but need it.
 *
 * Fake "usleep()" function grabbed from the inl netrek server -cba
 */
int usleep(huge microSeconds) {
	struct timeval		Timer;
	int			nfds = 0;

#ifdef FD_SET
	fd_set		*no_fds = NULL;
#else
	int			*no_fds = NULL;
#endif

	/* Was: int readfds, writefds, exceptfds; */
	/* Was: readfds = writefds = exceptfds = 0; */


	/* Paranoia -- No excessive sleeping */
	if (microSeconds > 4000000L) core("Illegal usleep() call");


	/* Wait for it */
	Timer.tv_sec = (microSeconds / 1000000L);
	Timer.tv_usec = (microSeconds % 1000000L);

	/* Wait for it */
	if (select(nfds, no_fds, no_fds, no_fds, &Timer) < 0) {
		/* Hack -- ignore interrupts */
		if (errno != EINTR) return(-1);
	}

	/* Success */
	return(0);
}
# endif /* HAS_USLEEP */
#endif /* SET_UID */

#ifdef WIN32
int usleep(long microSeconds) {
	Sleep(microSeconds / 1000); /* meassured in milliseconds not microseconds*/
	return(0);
}
#endif /* WIN32 */



static int MACRO_WAIT = 96; //hack: ASCII 96 ("`") is unused in the game's key layout
static int MACRO_XWAIT = 30; //hack: ASCII 30 (RS/Record Separator) which is practically unused, as it represents CTRL+^ is now abused for new client-side wait function that is indepdendant of the server, allows for long waits, and can be cancelled by keypress.
//Note that inkey() actually treats ASCII 30 special, as control-caret, but this usage seems deprecated so we don't have any collision here with MACRO_XWAIT.

static void ascii_to_text(char *buf, cptr str);

static bool after_macro = FALSE;
bool parse_macro = FALSE; /* are we inside the process of executing a macro */
bool inkey_sleep = FALSE, inkey_sleep_semaphore = FALSE;
int macro_missing_item = 0;

/* Does this 'parse_under' have any effect at all nowadays?
   Some comment refers specifically to "control-backslash" or
   "control+underscore sequence in main-x11.c" but there is no such a thing.
   What's also confusing is that ctrl+\ doesn't produce 31, but ctrl+/ does aka normal slash, not backslash. */
static bool parse_under = FALSE;
static bool parse_slash = FALSE;
static bool strip_chars = FALSE;

static bool flush_later = FALSE;

static byte macro__use[256];

static bool was_chat_buffer = FALSE;
static bool was_all_buffer = FALSE;
static bool was_important_scrollback = FALSE;

static char octify(uint i) {
	return(hexsym[i % 8]);
}

static char hexify(uint i) {
	return(hexsym[i % 16]);
}

void move_cursor(int row, int col) {
	Term_gotoxy(col, row);
}

void flush(void) {
	flush_later = TRUE;
}

void flush_now(void) {
	/* Clear various flags */
	flush_later = FALSE;

	/* Cancel "macro" info */
//#ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
	if (c_cfg.keep_topline) restore_prompt();
//#endif
	parse_macro = after_macro = FALSE;

	/* Cancel "sequence" info */
	parse_under = parse_slash = FALSE;

	/* Cancel "strip" mode */
	strip_chars = FALSE;

	/* Forgot old keypresses */
	Term_flush();
}

#ifdef RETRY_LOGIN
void RL_revert_input(void) {
	int i;

	after_macro = FALSE;
	parse_macro = FALSE; /* are we inside the process of executing a macro */
	inkey_sleep = FALSE; inkey_sleep_semaphore = FALSE;
	macro_missing_item = 0;
	parse_under = FALSE;
	parse_slash = FALSE;
	strip_chars = FALSE;

	flush_later = FALSE;

	for (i = 0; i < 256; i++)
		macro__use[i] = 0;

	was_chat_buffer = FALSE;
	was_all_buffer = FALSE;
	was_important_scrollback = FALSE;
}
#endif

/*
 * Check for possibly pending macros
 */
static int macro_maybe(cptr buf, int n) {
	int i;

	/* Scan the macros */
	for (i = n; i < macro__num; i++) {
		/* Skip inactive macros */
		if (macro__hyb[i] && ((shopping && !c_cfg.macros_in_stores) || inkey_msg)) continue;
		if (macro__cmd[i] && ((shopping && !c_cfg.macros_in_stores) || !inkey_flag || inkey_msg)) continue;

		/* Check for "prefix" */
		if (prefix(macro__pat[i], buf)) {
			/* Ignore complete macros */
			if (!streq(macro__pat[i], buf)) return(i);
		}
	}

	/* No matches */
	return(-1);
}


/*
 * Find the longest completed macro
 */
static int macro_ready(cptr buf) {
	int i, t, n = -1, s = -1;

	/* Scan the macros */
	for (i = 0; i < macro__num; i++) {
		/* Skip inactive macros */
		if (macro__cmd[i] && ((shopping && !c_cfg.macros_in_stores) || inkey_msg || !inkey_flag)) continue;
		if (macro__hyb[i] && ((shopping && !c_cfg.macros_in_stores) || inkey_msg)) continue;

		/* Check for "prefix" */
		if (!prefix(buf, macro__pat[i])) continue;

		/* Check the length of this entry */
		t = strlen(macro__pat[i]);

		/* Find the "longest" entry */
		if ((n >= 0) && (s > t)) continue;

		/* Track the entry */
		n = i;
		s = t;
	}

	/* Return the result */
	return(n);
}


/*
 * Hack -- add a macro definition (or redefinition).
 *
 * If "cmd_flag" is set then this macro is only active when
 * the user is being asked for a command (see below).
 */
void macro_add(cptr pat, cptr act, bool cmd_flag, bool hyb_flag) {
	int n;

	/* Paranoia -- require data */
	if (!pat || !act) return;


	/* Look for a re-usable slot */
	for (n = 0; n < macro__num; n++) {
		/* Notice macro redefinition */
		if (streq(macro__pat[n], pat)) {
			/* Free the old macro action */
			string_free(macro__act[n]);

			/* Save the macro action */
			macro__act[n] = string_make(act);

			/* Save the "cmd_flag" */
			macro__cmd[n] = cmd_flag;

			/* Save the hybrid flag */
			macro__hyb[n] = hyb_flag;

			/* Update the "trigger" char - mikaelh */
			if (hyb_flag) macro__use[(byte)(pat[0])] |= MACRO_USE_HYB;
			else if (cmd_flag) macro__use[(byte)(pat[0])] |= MACRO_USE_CMD;
			else macro__use[(byte)(pat[0])] |= MACRO_USE_STD;

			/* All done */
			return;
		}
	}


	/* Save the pattern */
	macro__pat[macro__num] = string_make(pat);

	/* Save the macro action */
	macro__act[macro__num] = string_make(act);

	/* Save the "cmd_flag" */
	macro__cmd[macro__num] = cmd_flag;

	/* Save the "hybrid flag" */
	macro__hyb[macro__num] = hyb_flag;

	/* One more macro */
	macro__num++;


	/* Hack -- Note the "trigger" char */
	if (hyb_flag) macro__use[(byte)(pat[0])] |= MACRO_USE_HYB;
	else if (cmd_flag) macro__use[(byte)(pat[0])] |= MACRO_USE_CMD;
	macro__use[(byte)(pat[0])] |= MACRO_USE_STD;

}

/*
 * Try to delete a macro - mikaelh
 */
bool macro_del(cptr pat) {
	int i, num = -1;

	/* Find the macro */
	for (i = 0; i < macro__num; i++) {
		if (streq(macro__pat[i], pat)) {
			num = i;
			break;
		}
	}

	if (num == -1) return(FALSE);

	/* Free it */
	string_free(macro__pat[num]);
	macro__pat[num] = NULL;
	string_free(macro__act[num]);
	macro__act[num] = NULL;

	/* Remove it from every array */
	for (i = num + 1; i < macro__num; i++) {
		macro__pat[i - 1] = macro__pat[i];
		macro__act[i - 1] = macro__act[i];
		macro__cmd[i - 1] = macro__cmd[i];
		macro__hyb[i - 1] = macro__hyb[i];
	}

#if 0 /* this can actually disable multiple keys if their macro patterns begin with the same byte - mikaelh */
	macro__use[(byte)(pat[0])] = 0;
#endif

	macro__num--;

	return(TRUE);
}

/* Returns the difference between two timevals in milliseconds */
static int diff_ms(struct timeval *begin, struct timeval *end) {
	int diff;

	diff = (end->tv_sec - begin->tv_sec) * 1000;
	diff += (end->tv_usec - begin->tv_usec) / 1000;

	return(diff);
}

/* ACCEPT_KEYS should be disabled or it'd cause the part labelled
    "Hack -- flush the output once no key is ready"
   in inkey() to trigger, flush the input and thereby clearing an
   ongoing macro, making it impossible to use \wXX constructs for
   macros that work with item-requests sent by the server
   (recharge, enchant, identify). */
//#define ACCEPT_KEYS
//#define PEEK_ONLY_KEYS /* only check for ESC key but don't touch/process key queue yet; currently broken! */
void sync_sleep(int milliseconds) {
	static char animation[4] = { '-', '\\', '|', '/' };
	int result, net_fd;
	struct timeval begin, now;
	int time_spent;
#ifdef ACCEPT_KEYS
	char ch;
#endif

	command_confirmed = -1;
	inkey_sleep = TRUE;

#ifdef WINDOWS
	/* Use the multimedia timer function */
	DWORD systime_ms = timeGetTime();
	begin.tv_sec = systime_ms / 1000;
	begin.tv_usec = (systime_ms % 1000) * 1000;
#else
	gettimeofday(&begin, NULL);
#endif
	net_fd = Net_fd();

#ifdef ACCEPT_KEYS
	/* HACK - Create a new key queue so we can receive fresh key presses */
	Term->keys_old = Term->keys;
	MAKE(Term->keys, key_queue);
	C_MAKE(Term->keys->queue, Term->key_size_orig, char);
	Term->keys->size = Term->key_size_orig;
#endif

	while (TRUE) {
#ifdef ACCEPT_KEYS
		/* Check for fresh key presses */
 #ifndef PEEK_ONLY_KEYS
		while (Term_inkey(&ch, FALSE, TRUE) == 0) {
 #else
		if (Term_inkey(&ch, FALSE, FALSE) == 0) {
 #endif
			if (ch == ESCAPE) {
				/* Forget key presses */
				Term->keys->head = 0;
				Term->keys->tail = 0;
				Term->keys->length = 0;

				if (Term->keys_old) {
					/* Destroy the old queue */
					C_KILL(Term->keys_old->queue, Term->keys_old->size, char);
					KILL(Term->keys_old, key_queue);
				}

				/* Erase the spinner */
				Term_erase(Term->wid - 1, 0, 1);

				/* Abort */
				inkey_sleep = FALSE;
				return;
			}
 #ifndef PEEK_ONLY_KEYS
			else {
				if (Term->keys_old) {
					/* Add it to the old queue */
					Term_keypress_aux(Term->keys_old, ch);
				}
			}
 #endif
		}
#endif

#ifdef WINDOWS
		/* Use the multimedia timer function */
		DWORD systime_ms = timeGetTime();
		now.tv_sec = systime_ms / 1000;
		now.tv_usec = (systime_ms % 1000) * 1000;
#else
		gettimeofday(&now, NULL);
#endif

		/* Check if we have waited long enough */
		time_spent = diff_ms(&begin, &now);
		if (time_spent >= milliseconds
		    /* hack: Net_input() might've caused command-processing that in turn calls inkey(),
		       then we'd already be continuing processing the part of the macro that comes after
		       a \wXX directive that we're still waiting for to complete here,
		       causing a inkey->sync_sleep->Net_input->inkey.. recursion from macros.
		       So in that case if a server-reply came in before we're done waiting, we can just
		       stop waiting now, because usually the purpose of \wXX is to actually wait for an
		       (item) input request from the server. - C. Blue */
		    || inkey_sleep_semaphore
		    /* hack: For commands where the server doesn't conveniently sends us another input
		       request as described above, starting at 4.6.2 the server might send us a special
		       PKT_CONFIRM though to signal that we can stop sleeping now. */
		    || command_confirmed != -1) {
#ifdef ACCEPT_KEYS
 #ifndef PEEK_ONLY_KEYS
			if (Term->keys_old) {
				/* Destroy the temporary key queue */
				C_KILL(Term->keys->queue, Term->keys->size, char);
				KILL(Term->keys, key_queue);

				/* Restore the old queue */
				Term->keys = Term->keys_old;
				Term->keys_old = NULL;
			}
 #endif
#endif

			/* Erase the spinner */
			Term_erase(Term->wid - 1, 0, 1);
			inkey_sleep = FALSE;
			/* For chained macros: Need to reset the semaphore, as we could meet another \w directive later on in the same macro we're still processing */
			inkey_sleep_semaphore = FALSE;
			return;
		}

		/* Do a little animation in the upper right corner */
		Term_putch(Term->wid - 1, 0, TERM_WHITE, animation[time_spent / 100 % 4]);

		/* Flush output - maintain flickering/multi-hued characters */
		do_flicker();

		/* Update our timer and if neccecary send a keepalive packet
		 */
		update_ticks();
		if (!c_quit) {
			do_keepalive();
			do_ping();
		}

		/* Flush the network output buffer */
		Net_flush();

		/* Wait for .001 sec, or until there is net input */
		SetTimeout(0, 1000);

		/* Update the screen */
		Term_fresh();

		if (c_quit) {
			usleep(1000);
			continue;
		}

		/* Parse net input if we got any */
		if (SocketReadable(net_fd)) {
			if ((result = Net_input()) == -1) quit("Net_input failed.");
		}

		/* Redraw windows if necessary */
		window_stuff();
	}
}
void sync_xsleep(int milliseconds) {
	static char animation[4] = { '-', '\\', '|', '/' };
	int result, net_fd;
	struct timeval begin, now;
	int time_spent;
	char ch;

	command_confirmed = -1;
	inkey_sleep = TRUE;

#ifdef WINDOWS
	/* Use the multimedia timer function */
	DWORD systime_ms = timeGetTime();
	begin.tv_sec = systime_ms / 1000;
	begin.tv_usec = (systime_ms % 1000) * 1000;
#else
	gettimeofday(&begin, NULL);
#endif
	net_fd = Net_fd();

	/* HACK - Create a new key queue so we can receive fresh key presses */
	Term->keys_old = Term->keys;

	MAKE(Term->keys, key_queue);
	C_MAKE(Term->keys->queue, Term->key_size_orig, char);

	Term->keys->size = Term->key_size_orig;

	while (TRUE) {
		/* Check for fresh key presses */
		while (Term_inkey(&ch, FALSE, TRUE) == 0) {
			if (ch == ESCAPE) {
				/* Forget key presses */
				Term->keys->head = 0;
				Term->keys->tail = 0;
				Term->keys->length = 0;

				if (Term->keys_old) {
					/* Destroy the old queue */
					C_KILL(Term->keys_old->queue, Term->keys_old->size, char);
					KILL(Term->keys_old, key_queue);
				}

				/* Erase the spinner */
				Term_erase(Term->wid - 1, 0, 1);

				/* Abort */
				inkey_sleep = FALSE;
				return;
			} else if (ch == ' ') {
				if (Term->keys_old) {
					/* Destroy the temporary key queue */
					C_KILL(Term->keys->queue, Term->keys->size, char);
					KILL(Term->keys, key_queue);

					/* Restore the old queue */
					Term->keys = Term->keys_old;
					Term->keys_old = NULL;
				}

				/* Erase the spinner */
				Term_erase(Term->wid - 1, 0, 1);
				inkey_sleep = FALSE;

				/* Cancel sleep and resume */
				return;
			} else {
				if (Term->keys_old) {
					/* Add it to the old queue */
					Term_keypress_aux(Term->keys_old, ch);
				}
			}
		}

#ifdef WINDOWS
		/* Use the multimedia timer function */
		DWORD systime_ms = timeGetTime();
		now.tv_sec = systime_ms / 1000;
		now.tv_usec = (systime_ms % 1000) * 1000;
#else
		gettimeofday(&now, NULL);
#endif

		/* Check if we have waited long enough */
		time_spent = diff_ms(&begin, &now);
		if (time_spent >= milliseconds
		    /* hack: Net_input() might've caused command-processing that in turn calls inkey(),
		       then we'd already be continuing processing the part of the macro that comes after
		       a \wXX directive that we're still waiting for to complete here,
		       causing a inkey->sync_sleep->Net_input->inkey.. recursion from macros.
		       So in that case if a server-reply came in before we're done waiting, we can just
		       stop waiting now, because usually the purpose of \wXX is to actually wait for an
		       (item) input request from the server. - C. Blue */
		    || inkey_sleep_semaphore
		    /* hack: For commands where the server doesn't conveniently sends us another input
		       request as described above, starting at 4.6.2 the server might send us a special
		       PKT_CONFIRM though to signal that we can stop sleeping now. */
		    || command_confirmed != -1) {
			if (Term->keys_old) {
				/* Destroy the temporary key queue */
				C_KILL(Term->keys->queue, Term->keys->size, char);
				KILL(Term->keys, key_queue);

				/* Restore the old queue */
				Term->keys = Term->keys_old;
				Term->keys_old = NULL;
			}

			/* Erase the spinner */
			Term_erase(Term->wid - 1, 0, 1);
			inkey_sleep = FALSE;
			/* For chained macros: Need to reset the semaphore, as we could meet another \w directive later on in the same macro we're still processing */
			inkey_sleep_semaphore = FALSE;
			return;
		}

		/* Do a little animation in the upper right corner */
		Term_putch(Term->wid - 1, 0, TERM_WHITE, animation[time_spent / 100 % 4]);

		/* Flush output - maintain flickering/multi-hued characters */
		do_flicker();

		/* Update our timer and if neccecary send a keepalive packet
		 */
		update_ticks();
		if (!c_quit) {
			do_keepalive();
			do_ping();
		}

		/* Flush the network output buffer */
		Net_flush();

		/* Wait for .001 sec, or until there is net input */
		SetTimeout(0, 1000);

		/* Update the screen */
		Term_fresh();

		if (c_quit) {
			usleep(1000);
			continue;
		}

		/* Parse net input if we got any */
		if (SocketReadable(net_fd)) {
			if ((result = Net_input()) == -1) quit("Net_input failed.");
		}

		/* Redraw windows if necessary */
		window_stuff();
	}
}

#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
/* Scan a buffer of key inputs whether it forms a special sequence of a navigational key. - C. Blue

   'string_input_relevance': Only scan for navigational keys that have concrete relevance during string input, ignore the rest.
   Currently, relevant keys for line-input are: left, right, pos1, end, del, ctrl+left, ctrl+right.

   This is especially important as it leaves shift+arrow macros untouched and working,
   which contain ESC keys and might be pressed as panic keys by the player if he gets attacked by a monster while typing.

   TODO maybe: catch the non-special key backspace here too (and shift+backspace, ctrl+backspace), just to cancel their macros during string input as well. */
int scan_navi_key(cptr buf, bool string_input_relevance) {
	int i;
	cptr bufp = buf;
	char c;
	int found = TRUE;
	bool sk_shift = FALSE, sk_ctrl = FALSE, sk_alt = FALSE;

	if (strlen(buf) < nks_minlen) return(0);

	/* Verify special key sequence starter */
	for (i = 0; (c = nks_start[i]); i++) if (*bufp++ != c) { found = FALSE; break; }

	/* Strip shiftkeys - we want ANY of the navigationalkey+shiftkeys combos to get ignored as macro, as we may process them all.
	   Also we assume that shiftkey marker are only 1 byte long and occur in the order ctrl-shift-alt in the key code sequences.
	   We actually don't really need to process shiftkeys here in general, if we use ENABLE_SHIFT_SPECIALKEYS for that instead. */
	if (*bufp == nks_ctrl[0]) {
		sk_ctrl = TRUE;
 #ifdef ENABLE_SHIFT_SPECIALKEYS
		//inkey_shift_special |= 0x2; //should be redundant, as this flag should be set already via e.s.s. functionality
 #endif
		bufp++;
	}
	if (*bufp == nks_shift[0]) {
		sk_shift = TRUE;
 #ifdef ENABLE_SHIFT_SPECIALKEYS
		//inkey_shift_special |= 0x1; //should be redundant, as this flag should be set already via e.s.s. functionality
 #endif
		bufp++;
	}
	if (*bufp == nks_alt[0]) {
		if (string_input_relevance) return(0); // Irrelevant key!
		sk_alt = TRUE;
 #ifdef ENABLE_SHIFT_SPECIALKEYS
		//inkey_shift_special |= 0x4; //should be redundant, as this flag should be set already via e.s.s. functionality
 #endif
		bufp++;
	}
	if (string_input_relevance && sk_shift && sk_ctrl) return(0); // Irrelevant key!

	/* Verify presence of end-of-shiftkeys marker */
	for (i = 0; (c = nks_skterm[i]); i++) if (*bufp++ != c) { found = FALSE; break; }
	/* At least on POSIX there are always two static chars in the middle */
	for (i = 0; (c = nks_skip[i]); i++) if (*bufp++ != c) { found = FALSE; break; }
	/* Now check for the various keys */
	if (!found) return(0);

	found = 0;

	for (i = 0; (c = nks_del[i]); i++) if (*(bufp + i) != c) break;
	if (i && !c) {
		found = NAVI_KEY_DEL;
		if (string_input_relevance && (sk_shift || sk_ctrl || sk_alt)) return(0); // Irrelevant key!
	}
	if (!found) {
		for (i = 0; (c = nks_u[i]); i++) if (*(bufp + i) != c) break;
		if (i && !c) {
			found = NAVI_KEY_UP;
			if (string_input_relevance) return(0); // Irrelevant key!
		}
	}
	if (!found) {
		for (i = 0; (c = nks_r[i]); i++) if (*(bufp + i) != c) break;
		if (i && !c) {
			found = NAVI_KEY_RIGHT;
			if (string_input_relevance && sk_shift) return(0); // Irrelevant key!
		}
	}
	if (!found) {
		for (i = 0; (c = nks_d[i]); i++) if (*(bufp + i) != c) break;
		if (i && !c) {
			found = NAVI_KEY_DOWN;
			if (string_input_relevance) return(0); // Irrelevant key!
		}
	}
	if (!found) {
		for (i = 0; (c = nks_l[i]); i++) if (*(bufp + i) != c) break;
		if (i && !c) {
			found = NAVI_KEY_LEFT;
			if (string_input_relevance && sk_shift) return(0); // Irrelevant key!
		}
	}
	if (!found) {
		for (i = 0; (c = nks_pu[i]); i++) if (*(bufp + i) != c) break;
		if (i && !c) {
			found = NAVI_KEY_PAGEUP;
			if (string_input_relevance) return(0); // Irrelevant key!
		}
	}
	if (!found) {
		for (i = 0; (c = nks_pd[i]); i++) if (*(bufp + i) != c) break;
		if (i && !c) {
			found = NAVI_KEY_PAGEDOWN;
			if (string_input_relevance) return(0); // Irrelevant key!
		}
	}
	if (!found) {
		for (i = 0; (c = nks_p[i]); i++) if (*(bufp + i) != c) break;
		if (i && !c) {
			found = NAVI_KEY_POS1;
			if (string_input_relevance && (sk_shift || sk_ctrl || sk_alt)) return(0); // Irrelevant key!
		}
	}
	if (!found) {
		for (i = 0; (c = nks_e[i]); i++) if (*(bufp + i) != c) break;
		if (i && !c) {
			found = NAVI_KEY_END;
			if (string_input_relevance && (sk_shift || sk_ctrl || sk_alt)) return(0); // Irrelevant key!
		}
	}
	/* Assume all key sequences are equally long,
	   just pick one of them (that is guaranteed to be in use in any OS variant, ie not 'DEL') to advance pointer accordingly */
	bufp += strlen(nks_u);
	/* Verify special key sequence terminator */
	for (i = 0; (c = nks_term[i]); i++) if (*bufp++ != c) { found = 0; break; }


	/* 'found' = 0 -> unknown sequence, discard */

	return(found);
}
#endif

/* Wrapped inkey() function to use with ALLOW_NAVI_KEYS_IN_PROMPT. - C. Blue */
char inkey_combo(bool modify_allowed, int *cursor_pos, cptr input_str) {
	char i;

#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
	inkey_location_keys = TRUE;
 #if 0 /* All of this isn't that cool. Instead, let's only auto-enter edit mode if user actually presses an arrow/positional key, making use of this whole feature thing. */
  #if 0 /* This also changes colour from yellow to white */
	if (modify_allowed) i = KTRL('E'); //Simulate CTRL+E press once, to enter 'edit' mode right away.
	else
  #else /* This retains the yellow starter prompt colour, better */
	if (modify_allowed
	    && cursor_pos && input_str) /* <- two paranoia checks against wrong parm usage */
		*cursor_pos = strlen(input_str);
  #endif
 #endif
#endif

	i = inkey();

#ifdef ENABLE_SHIFT_SPECIALKEYS
	if (i == '\010' || i == 0x7F) { /* BACKSPACE (or on POSIX also: DELETE) */
		if (inkey_shift_special == 0x2) i = KTRL('E'); /* CTRL+BACKSPACE = erase word */
		if (inkey_shift_special == 0x1) i = KTRL('D'); /* SHIFT+BACKSPACE = DELETE -- for POSIX: 'Emulate' non-existant DEL key. */
	}
	else if (inkey_shift_special == 0x2) switch (i) {
		case NAVI_KEY_LEFT: i = KTRL('Q'); break; /* CTRL+LEFT = jump a word left */
		case NAVI_KEY_RIGHT: i = KTRL('W'); break; /* CTRL+RIGHT = jump a word right */
	}
#endif

#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
	inkey_location_keys = FALSE;
#endif

	return(i);
}


/*
 * Helper function called only from "inkey()"
 *
 * This function does most of the "macro" processing.
 *
 * We use the "Term_key_push()" function to handle "failed" macros,
 * as well as "extra" keys read in while choosing a macro, and the
 * actual action for the macro.
 *
 * Embedded macros are illegal, although "clever" use of special
 * control chars may bypass this restriction.  Be very careful.
 *
 * The user only gets 500 (1+2+...+29+30) milliseconds for the macro.
 *
 * Note the annoying special processing to "correctly" handle the
 * special "control-backslash" codes following a "control-underscore"
 * macro sequence.  See "main-x11.c" and "main-xaw.c" for details.
 *
 *
 * Note that we NEVER wait for a keypress without also checking the network
 * for incoming packets.  This stops annoying "timeouts" and also lets
 * the screen get redrawn if need be while we are waiting for a key. --KLJ--
 */
static char inkey_aux(void) {
	int	k = 0, n, p = 0, w = 0;
	char	ch = 0;
#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
	char	ch_navi;
#endif
	cptr	pat, act;
	char	buf[1024];
	char	buf_atoi[5];
	bool	inkey_max_line_set;
	int net_fd;

	inkey_max_line_set = inkey_max_line;

	/* Acquire and save maximum file descriptor */
	net_fd = Net_fd();

	/* If no network yet, just wait for a keypress */
	if (net_fd == -1) {
#ifndef ATMOSPHERIC_INTRO /* don't block, because we still want to animate colours maybe (title screen!) */
		/* Wait for a keypress */
		(void)(Term_inkey(&ch, TRUE, TRUE));

		if (parse_macro && ch == MACRO_WAIT) {
			buf_atoi[0] = '0';
			buf_atoi[1] = '0';
			buf_atoi[2] = '\0';
			(void)(Term_inkey(&ch, TRUE, TRUE));
			if (ch) buf_atoi[0] = ch;
			(void)(Term_inkey(&ch, TRUE, TRUE));
			if (ch) buf_atoi[1] = ch;
			w = atoi(buf_atoi);
			sync_sleep(w * 100L); /* w 1/10th seconds */
			ch = 0;
			w = 0;

			/* continue with the next 'real' key */
			(void)(Term_inkey(&ch, TRUE, TRUE));
		} else if (parse_macro && ch == MACRO_XWAIT) {
			buf_atoi[0] = '0';
			buf_atoi[1] = '0';
			buf_atoi[2] = '0';
			buf_atoi[3] = '0';
			buf_atoi[4] = '\0';
			(void)(Term_inkey(&ch, TRUE, TRUE));
			if (ch) buf_atoi[0] = ch;
			(void)(Term_inkey(&ch, TRUE, TRUE));
			if (ch) buf_atoi[1] = ch;
			(void)(Term_inkey(&ch, TRUE, TRUE));
			if (ch) buf_atoi[2] = ch;
			(void)(Term_inkey(&ch, TRUE, TRUE));
			if (ch) buf_atoi[3] = ch;
			w = atoi(buf_atoi);
			sync_xsleep(w * 100L); /* w/10 seconds */
			ch = 0;
			w = 0;

			/* continue with the next 'real' key */
			(void)(Term_inkey(&ch, TRUE, TRUE));
		}
#else
		do {
			do_flicker();
			(void)(Term_inkey(&ch, FALSE, TRUE));
			if (ch) break;
			update_ticks();		// <-(x)!! in -c (terminal) mode, this is actually _the_ (only) update_ticks() that is called during meta server display and that is able to visibly redraw meta server list every 9s! (META_DISPLAYPINGS_LATER)
 #ifdef RAINY_TOMB
			do_weather(FALSE);
 #endif

			/* Don't consume 100% cpu, wait according to client fps. */
 #if 0
			/* Wait according to fps - mikaelh */
			SetTimeout(0, next_frame()); //doesn't work here
 #else
			usleep(1000); //so use this instead
 #endif
		} while (!ch);
#endif
	} else {
		/* Wait for keypress, while also checking for net input */
		do {
			int result;

			/* Flush output - maintain flickering/multi-hued characters */
			do_flicker();

			/* Animate things: Minimap '@' denoting our own position */
			if (minimap_posx != -1 && screen_icky) {
				if ((ticks % 6) < 3)
					Term_draw(minimap_posx, minimap_posy, TERM_WHITE, '@');
				else
					Term_draw(minimap_posx, minimap_posy, minimap_attr, minimap_char);
				/* New: Grid selection possible, to verify coordinates */
				if (minimap_selx != -1) {
					if ((ticks % 6) < 3)
						Term_draw(minimap_selx, minimap_sely, TERM_ORANGE, 'X');
					else
						Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
				}
			}

			/* Look for a keypress */
			(void)(Term_inkey(&ch, FALSE, TRUE));

			if (parse_macro && ch == MACRO_WAIT) {
				buf_atoi[0] = '0';
				buf_atoi[1] = '0';
				buf_atoi[2] = '\0';
				(void)(Term_inkey(&ch, TRUE, TRUE));
				if (ch) buf_atoi[0] = ch;
				(void)(Term_inkey(&ch, TRUE, TRUE));
				if (ch) buf_atoi[1] = ch;
				w = atoi(buf_atoi);
				sync_sleep(w * 100L); /* w 1/10th seconds */
				ch = 0;
				w = 0;

				/* continue with the next 'real' key */
				continue;
			} else if (parse_macro && ch == MACRO_XWAIT) {
				buf_atoi[0] = '0';
				buf_atoi[1] = '0';
				buf_atoi[2] = '0';
				buf_atoi[3] = '0';
				buf_atoi[4] = '\0';
				(void)(Term_inkey(&ch, TRUE, TRUE));
				if (ch) buf_atoi[0] = ch;
				(void)(Term_inkey(&ch, TRUE, TRUE));
				if (ch) buf_atoi[1] = ch;
				(void)(Term_inkey(&ch, TRUE, TRUE));
				if (ch) buf_atoi[2] = ch;
				(void)(Term_inkey(&ch, TRUE, TRUE));
				if (ch) buf_atoi[3] = ch;
				w = atoi(buf_atoi);
				sync_xsleep(w * 100L); /* w/10 seconds */
				ch = 0;
				w = 0;

				/* continue with the next 'real' key */
				continue;
			}

			/* Hack for auto-pressing spacebar while in player-list (SPECIAL_FILE_PLAYER) */
			if (within_cmd_player && ticks - within_cmd_player_ticks >= 50) {
				within_cmd_player_ticks = ticks;
				/* hack: -- TODO: Remove/restrict it so it doesn't interfere with inkey() checks that just _wait_ for any actual keypress,
				   as those will be auto-confirmed by this; maybe use the existing if (k == 1) check after inkey() in peruse_file(). */
				ch = -1; //refresh our current player list view
				break;
			}
			/* Also hack for auto-pressing a dummy key in jukebox screen while in automatic 'play-all' mode, in order to redraw the jukebox screen with current song selected */
			if (jukebox_screen &&
			    ((jukebox_play_all && (jukebox_play_all_prev != jukebox_playing || jukebox_play_all_prev_song != jukebox_playing_song))
			    || jukebox_play_all_done
			    || jukebox_gamemusicchanged)) {
				jukebox_play_all_done = FALSE;
				jukebox_gamemusicchanged = FALSE;
				ch = -1;
				break;
			}

			/* If we got a key, break */
			if (ch) break;

			/* If we received a 'max_line' value from the net,
			   break if inkey_max_line flag was set */
			if (inkey_max_line != inkey_max_line_set) {
				/* hack: -- TODO: Remove/restrict it so it doesn't interfere with inkey() checks that just _wait_ for any actual keypress,
				   as those will be auto-confirmed by this; maybe use the existing if (k == 1) check after inkey() in peruse_file(). */
				ch = 1; //refresh the max line number to orange colour if we reached the end of the document perused
				break;
			}

			/* Update our timer and if neccecary send a keepalive packet
			 */
			update_ticks();
			if (!c_quit) {
				do_keepalive();
				do_ping();
			}

			/* Flush the network output buffer */
			Net_flush();

			/* Wait for .001 sec, or until there is net input */
			//SetTimeout(0, 1000);

			/* Wait according to fps - mikaelh */
			SetTimeout(0, next_frame());

			/* Update the screen */
			Term_fresh();

			if (c_quit) {
				usleep(1000);
				continue;
			}

			/* Parse net input if we got any */
			if (SocketReadable(net_fd)) {
				if ((result = Net_input()) == -1) quit("Net_input failed.");
			}

			/* Hack - Leave a store */
			if (shopping && leave_store) return(ESCAPE);

			/* Are we supposed to abort a 'special input request'?
			   (Ie outside game situation has changed, eg left store.) */
			if (request_pending && request_abort) {
				request_abort = FALSE;
				return(ESCAPE);
			}

			/* Redraw windows if necessary */
			window_stuff();
		} while (!ch);
	}

	/* End of internal macro */
	if (ch == 29) {
//#ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
		if (c_cfg.keep_topline) restore_prompt();
//#endif
		parse_macro = FALSE;
	}


	/* Do not check "ascii 28" */
	if (ch == 28) return(ch);


	/* Do not check "ascii 29" */
	if (ch == 29) return(ch);

	/* Do not check macro actions */
	if (parse_macro) {
		/* Check for problems with missing items the macro was supposed to use - C. Blue */
		if (macro_missing_item == 1) {
			/* Check for discarding item called by name */
			if (ch == '@') {
				ch = 0;
				/* Prepare to ignore the (partial) item name sequence */
				macro_missing_item = 2;
			}
			/* Check for discarding item called by numeric inscription */
			else if (ch >= '0' && ch <= '9') {
				ch = 0;
				/* Case closed */
				macro_missing_item = 0;
			}
			/* Expected item choice is missing IN the macro - bad macro! Case closed though. */
			else macro_missing_item = 0;
		/* Still within call-by-name sequence? */
		} else if (macro_missing_item == 2) {
			if (ch == '\r') {
				/* Sequence finally ends here */
				macro_missing_item = 0;
			} else ch = 0;
		/* Check for problems with missing inscription-called items */
		} else if (macro_missing_item == 3) {
			if (ch >= '0' && ch <= '9') ch = 0;
			/* Case closed in any way */
			macro_missing_item = 0;
		}

		/* return next macro character */
		return(ch);
	}

	/* Do not check "control-underscore" sequences */
	if (parse_under) return(ch);

	/* Do not check "control-backslash" sequences */
	if (parse_slash) return(ch);


	/* Efficiency -- Ignore impossible macros */
	if (!macro__use[(byte)(ch)]) return(ch);

	/* Efficiency -- Ignore inactive macros */
	if ((((shopping && !c_cfg.macros_in_stores) || !inkey_flag || inkey_msg) && (macro__use[(byte)(ch)] == MACRO_USE_CMD)) || inkey_interact_macros) return(ch);
	if ((((shopping && !c_cfg.macros_in_stores) || inkey_msg) && (macro__use[(byte)(ch)] == MACRO_USE_HYB)) || inkey_interact_macros) return(ch);

	/* Save the first key, advance */
	buf[p++] = ch;
	buf[p] = '\0';

	/* Wait for a macro, or a timeout */
	while (TRUE) {
		/* Check for possible macros */
		k = macro_maybe(buf, k);

		/* Nothing matches */
		if (k < 0) break;

		/* Check for (and remove) a pending key */
		if (0 == Term_inkey(&ch, FALSE, TRUE)) {
			/* Append the key */
			buf[p++] = ch;
			buf[p] = '\0';

			/* Restart wait */
			w = 0;
		}

		/* No key ready */
		else {
			if (multi_key_macros) {
				/* Increase "wait" */
				w += 10;

				/* Excessive delay */
				if (w >= 100) break;

				/* Delay */
				Term_xtra(TERM_XTRA_DELAY, w);
			} else {
				/* No waiting */
				break;
			}
		}
	}

#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
 #ifdef SOME_NAVI_KEYS_DISABLE_MACROS_IN_PROMPTS
	/* Specifically prevent any macros on navigation keys, as we want to process these keys directly in the current situation (ie some sort of message input prompt)? */
	if (inkey_location_keys && (ch_navi = scan_navi_key(buf, TRUE))) return(ch_navi);
 #endif
#endif

	/* Check for a successful macro */
	k = macro_ready(buf);

	/* No macro available */
	if (k < 0) {
		/* Push all the keys back on the queue */
		while (p > 0) {
			/* Push the key, notice over-flow */
			if (Term_key_push(buf[--p])) return(0);
		}

		/* Wait for (and remove) a pending key */
		(void)Term_inkey(&ch, TRUE, TRUE);

		/* Return the key */
		return(ch);
	}

	/* Access the macro pattern */
	pat = macro__pat[k];

	/* Get the length of the pattern */
	n = strlen(pat);


	/* Push the "extra" keys back on the queue */
	while (p > n) {
		if (Term_key_push(buf[--p])) return(0);
	}

	/* We are now inside a macro */
	parse_macro = TRUE;
	inkey_sleep_semaphore = FALSE; //init semaphore for macros containing \wXX
	/* Assume that the macro has no problems with possibly missing items so far */
	macro_missing_item = 0;
	/* Paranoia: Make sure that the macro won't instantly fail because something failed before */
	abort_prompt = FALSE;

	/* Push the "macro complete" key */
	if (Term_key_push(29)) return(0);


	/* Access the macro action */
	act = macro__act[k];

	/* Get the length of the action */
	n = strlen(act);

#if 0
	/* Push the macro "action" onto the key queue */
	while (n > 0) {
		/* Push the key, notice over-flow */
		if (Term_key_push(act[--n])) return(0);
	}
#else
	/* Push all at once */
	Term_key_push_buf(act, n);
#endif

	/* Force "inkey()" to call us again */
	return(0);
}



/*
 * Get a keypress from the user.
 *
 * This function recognizes a few "global parameters".  These are variables
 * which, if set to TRUE before calling this function, will have an effect
 * on this function, and which are always reset to FALSE by this function
 * before this function returns.  Thus they function just like normal
 * parameters, except that most calls to this function can ignore them.
 *
 * Normally, this function will process "macros", but if "inkey_base" is
 * TRUE, then we will bypass all "macro" processing.  This allows direct
 * usage of the "Term_inkey()" function.
 *
 * Normally, this function will do something, but if "inkey_xtra" is TRUE,
 * then something else will happen.
 *
 * Normally, this function will wait until a "real" key is ready, but if
 * "inkey_scan" is TRUE, then we will return zero if no keys are ready.
 *
 * Normally, this function will show the cursor, and will process all normal
 * macros, but if "inkey_flag" is TRUE, then we will only show the cursor if
 * "hilite_player" is TRUE (--possibly deprecated info here-- after
 * hilite_player was always broken, a new implementation is tested in 2013),
 * and also, we will only process "command" macros.
 *
 * Note that the "flush()" function does not actually flush the input queue,
 * but waits until "inkey()" is called to perform the "flush".
 *
 * Refresh the screen if waiting for a keypress and no key is ready.
 *
 * Note that "back-quote" is automatically converted into "escape" for
 * convenience on machines with no "escape" key.  This is done after the
 * macro matching, so the user can still make a macro for "backquote".
 *
 * Note the special handling of a few "special" control-keys, which
 * are reserved to simplify the use of various "main-xxx.c" files,
 * or used by the "macro" code above.
 *
 * Ascii 27 is "control left bracket" -- normal "Escape" key
 * Ascii 28 is "control backslash" -- special macro delimiter
 * Ascii 29 is "control right bracket" -- end of macro action
 * Ascii 30 is "control caret" -- indicates "keypad" key
 * Ascii 31 is "control underscore" -- begin macro-trigger
 *
 * Hack -- Make sure to allow calls to "inkey()" even if "term_term_main"
 * is not the active Term, this allows the various "main-xxx.c" files
 * to only handle input when "term_term_main" is "active".
 *
 * Note the nasty code used to process the "inkey_base" flag, which allows
 * various "macro triggers" to be entered as normal key-sequences, with the
 * appropriate timing constraints, but without actually matching against any
 * macro sequences.  Most of the nastiness is to handle "ascii 28" (see below).
 *
 * The "ascii 28" code is a complete hack, used to allow "default actions"
 * to be associated with a given keypress, and used only by the X11 module,
 * it may or may not actually work.  The theory is that a keypress can send
 * a special sequence, consisting of a "macro trigger" plus a "default action",
 * with the "default action" surrounded by "ascii 28" symbols.  Then, when that
 * key is pressed, if the trigger matches any macro, the correct action will be
 * executed, and the "strip default action" code will remove the "default action"
 * from the keypress queue, while if it does not match, the trigger itself will
 * be stripped, and then the "ascii 28" symbols will be stripped as well, leaving
 * the "default action" keys in the "key queue".  Again, this may not work.
 */
char inkey(void) {
	int v;
	char kk, ch;
	bool done = FALSE;
	term *old = Term;
	int w = 0;
	int skipping = FALSE;

#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
 #define INKEY_LOCATION_KEY_SIZE (10+5) /* +5: paranoia reserve vs buffer overflow */
	static bool inkey_location_key_active = FALSE;
	static int inkey_location_key_index = 0;
	static char inkey_location_key_sequence[INKEY_LOCATION_KEY_SIZE];
	static int inkey_location_key_tick = -1;
#endif

#ifdef ENABLE_SHIFT_SPECIALKEYS
	inkey_shift_special = 0x0;
#endif


	/* Hack -- handle delayed "flush()" */
	if (flush_later) {
		/* Done */
		flush_later = FALSE;

		/* Cancel "macro" info */
//#ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
		if (c_cfg.keep_topline) restore_prompt();
//#endif
		parse_macro = after_macro = FALSE;

		/* Cancel "sequence" info */
		parse_under = parse_slash = FALSE;

		/* Cancel "strip" mode */
		strip_chars = FALSE;

		/* Forget old keypresses */
		Term_flush();
	}


	/* Access cursor state */
	(void)Term_get_cursor(&v);

	/* Show the cursor if waiting, except sometimes in "command" mode */
	if (!inkey_scan && (!inkey_flag)) {
		/* Show the cursor */
		(void)Term_set_cursor(1);
	}

	/* Hack -- Activate the screen */
	Term_activate(term_term_main);


	/* Get a (non-zero) keypress */
	for (ch = 0; !ch; ) {
		/* Flush output - maintain flickering/multi-hued characters */
		do_flicker(); //unnecessary since it's done in inkey_aux() anyway? */

		/* Nothing ready, not waiting, and not doing "inkey_base" */
		if (!inkey_base && inkey_scan && (0 != Term_inkey(&ch, FALSE, FALSE))) break;

		/* Hack -- flush the output once no key is ready */
		if (!done && (0 != Term_inkey(&ch, FALSE, FALSE))) {		// <-(1)!! in -c (terminal) mode, this causes metaserver-display to blank out (META_DISPLAYPINGS_LATER)
			/* Hack -- activate proper term */
			Term_activate(old);

			/* Flush output */
			Term_fresh();

			/* Hack -- activate the screen */
			Term_activate(term_term_main);

			/* Only once */
			done = TRUE;
		}

		/* Hack -- used to get a macro trigger (for that we need raw Term_inkey() passthrough, without any macro accidentally triggering from these keys read). */
		if (inkey_base) {
			char xh;
#if 0 /* don't block.. */
			/* Check for keypress, optional wait */
			(void)Term_inkey(&xh, !inkey_scan, TRUE);
#else /* ..but keep processing net input in the background so we don't timeout. - C. Blue */
			int net_fd = Net_fd();

			if (inkey_scan) (void)Term_inkey(&xh, FALSE, TRUE); /* don't wait */
			else if (net_fd == -1) (void)Term_inkey(&xh, TRUE, TRUE); /* wait and block, no network yet anyway */
			else do { /* do wait, but don't block; keep processing */
				/* Note that we assume here that we're ONLY called from get_macro_trigger()! */

				/* Flush output - maintain flickering/multi-hued characters */
				do_flicker();

				/* Look for a keypress */
				(void)(Term_inkey(&xh, FALSE, TRUE));

				/* If we got a key, break */
				if (xh) break;

 #if 0 /* probably should be disabled - assumption: we're ONLY called for get_macro_trigger() */
				/* If we received a 'max_line' value from the net,
				   break if inkey_max_line flag was set */
				if (inkey_max_line != inkey_max_line_set) {
					/* hack: */
					ch = 1;
					break;
				}
 #endif

				/* Update our timer and if neccecary send a keepalive packet */
				update_ticks();
				if (!c_quit) {
					do_keepalive();
					do_ping();
				}

				/* Flush the network output buffer */
				Net_flush();

				/* Wait according to fps - mikaelh */
				SetTimeout(0, next_frame());

				/* Update the screen */
				Term_fresh();

				if (c_quit) {
					usleep(1000);
					continue;
				}

				/* Parse net input if we got any */
				if (SocketReadable(net_fd)) {
					if (Net_input() == -1) quit("Net_input failed.");
				}

 #if 0 /* probably not needed - assumption: we're ONLY called for get_macro_trigger() */
				/* Hack - Leave a store */
				if (shopping && leave_store) {
					if (inkey_sleep) inkey_sleep_semaphore = TRUE;
					return(ESCAPE);
				}
 #endif

				/* Redraw windows if necessary */
				window_stuff();
			} while (1);
#endif

			/* Key ready */
			if (xh) {
				/* Reset delay */
				w = 0;

				/* Mega-Hack */
				if (xh == 28) {
					/* Toggle "skipping" */
					skipping = !skipping;
				}

				/* Use normal keys */
				else if (!skipping) {
					/* Use it */
					ch = xh;
				}
			}

			/* No key ready */
			else {
				if (multi_key_macros) {
					/* Increase "wait" */
					w += 10;

					/* Excessive delay */
					if (w >= 100) break;

					/* Delay */
					Term_xtra(TERM_XTRA_DELAY, w);
				} else {
					/* No waiting */
					break;
				}
			}

			/* Continue */
			continue;
		}

		/* Get a key (see above) */
		kk = ch = inkey_aux();//		<-(y)!! in -c (terminal) mode, this waits for keypress (META_DISPLAYPINGS_LATER)

#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
		/* Crazy hack: Distinguish between actual navi-key sequence (starting on 31) and just a 31 char input via ctrl+/ or ctrl+_ */
		if (inkey_location_keys && inkey_location_key_active
		    /* Actually wait for 2 ticks (200ms) to be safe we weren't exactly at the tick-change threshold and paranoid... */
		    && inkey_location_key_tick != ticks && inkey_location_key_tick + 1 != ticks
		    /* No further key received? Cannot be an navi-key-sequence then, but was just normal input */
		    && inkey_location_key_index == 0) {
			/* Disable navi-key-sequence checking for this 31 */
			inkey_location_key_active = FALSE;

			/* --- Enable the control-underscore sequence status as 31 would have done normally: --- */

			/* Inside a "underscore" sequence */
			parse_under = TRUE;
			/* Strip chars (always) */
			strip_chars = TRUE;
		}
#endif

		/* Finished a "control-underscore" sequence */
		if (parse_under && (ch <= 32)) {
			/* Found the edge */
			parse_under = FALSE;

			/* Stop stripping */
			strip_chars = FALSE;

			/* Strip this key */
			ch = 0;
		}


		/* Finished a "control-backslash" sequence */
		if (parse_slash && (ch == 28)) {
			/* Found the edge */
			parse_slash = FALSE;

			/* Stop stripping */
			strip_chars = FALSE;

			/* Strip this key */
			ch = 0;
		}

//c_msg_format("key %c (%d) - %d, %d, %d, %d, %d", ch, ch, kk, after_macro, parse_slash, multi_key_macros, skipping);

		/* Handle some special keys */
		switch (ch) {
		/* Hack -- convert back-quote into escape */
		case '`':
			/* Convert to "Escape" */
			ch = ESCAPE;
			break;

		/* Hack -- strip "control-backslash" special-fallback-strings */
		case 28:
			/* Strip this key */
			ch = 0;
			/* Inside a "control-backslash" sequence */
			parse_slash = TRUE;
			/* Strip chars (sometimes) */
			strip_chars = after_macro;
			break;

		/* Hack -- strip "control-right-bracket" end-of-macro-action */
		case 29:
			/* Strip this key */
			ch = 0;
			break;

		/* Hack -- strip "control-caret" special-keypad-indicator -- only used in deprecated main-xxx.c files now? otherwise it could collide with MACRO_XWAIT */
		case 30:
			/* Strip this key */
			ch = 0;
			break;

		/* Hack -- strip "control-underscore" special-macro-triggers
		   (NOTE: CTRL+_ and CTRL+/ is also key 31, and thereby can freeze up char input for the next 15 chars if user presses it manually.) */
		case 31: /* == nks_start[0] : Start marker of special key sequence */
#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
			/* Crazy hack: Enable use of arrow keys, added for askfor_aux() -- TODO: Check collision with multi-key-macro-sequence (parse_under/strip_chars)! */
			if (inkey_location_keys) {
				inkey_location_key_active = TRUE;
				inkey_location_key_index = -1;
				/* Allow to distinguish between a location-key sequence starting on 31 and a solemn 31 that was input by the user via ctrl+/ or ctrl+_ !
				   Oherwise this can be slightly lethal if it happens in chat input while in the dungeon.
				   The way we distinguish is that since a navigation-key sequence is practically atomic, we just wait for one tick after receiving a 31,
				   and if there is no further input the next tick (+100ms) we assume it was not a navigation-key sequence,
				   and we behave as if the 31 char was processed normally, ie enabling parse_under and strip_chars as seen below. */
				inkey_location_key_tick = ticks;
				break;
			}
#endif

			/* Strip this key */
			ch = 0;
			/* Inside a "underscore" sequence */
			parse_under = TRUE;
			/* Strip chars (always) */
			strip_chars = TRUE;
			break;
		}

#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
		/* Process/end crazy hack */
		if (inkey_location_key_active) {
			/* Process char, add it to sequence code */
			inkey_location_key_index++;
			if (inkey_location_key_index < INKEY_LOCATION_KEY_SIZE) /* <- Paranoia test: Prevent buffer overflow */
				inkey_location_key_sequence[inkey_location_key_index] = ch;
			else { /* This shouldn't happen ever */
				inkey_location_key_active = FALSE;
				ch = 0;
				break;
			}

			if (ch == nks_term[0]) { /* == 13 : End marker of special key sequence */
				/* Terminate character collection */
				inkey_location_key_active = FALSE;

				/* Evaluate and return hack value. Note #0 is always '31', #6 always '13', or we wouldn't be here. */
				ch = scan_navi_key(inkey_location_key_sequence, FALSE);
			}
			/* Continue receiving chars */
			else ch = 0;
		}
#endif


		/* Hack -- Set "after_macro" code */
		after_macro = ((kk == 29) ? TRUE : FALSE);


		/* Hack -- strip chars */
		if (strip_chars) ch = 0;
	}


	/* Hack -- restore the term */
	Term_activate(old);

	/* Restore the cursor */
	Term_set_cursor(v);

	/* Cancel the various "global parameters" */
	inkey_base = inkey_scan = inkey_flag = inkey_max_line = FALSE;

	/* Return the keypress */
	if (inkey_sleep) inkey_sleep_semaphore = TRUE;
	return(ch);
}

static int hack_dir = 0;

/*
 * Convert a "Rogue" keypress into an "Angband" keypress
 * Pass extra information as needed via "hack_dir"
 *
 * Note that many "Rogue" keypresses encode a direction.
 */
char roguelike_commands(char command) {
	/* Process the command */
	switch (command) {

	/* Movement (rogue keys) */
	case 'b': hack_dir = 1; return(';');
	case 'j': hack_dir = 2; return(';');
	case 'n': hack_dir = 3; return(';');
	case 'h': hack_dir = 4; return(';');
	case 'l': hack_dir = 6; return(';');
	case 'y': hack_dir = 7; return(';');
	case 'k': hack_dir = 8; return(';');
	case 'u': hack_dir = 9; return(';');

	/* Running (shift + rogue keys) */
	case 'B': hack_dir = 1; return('.');
	case 'J': hack_dir = 2; return('.');
	case 'N': hack_dir = 3; return('.');
	case 'H': hack_dir = 4; return('.');
	case 'L': hack_dir = 6; return('.');
	case 'Y': hack_dir = 7; return('.');
	case 'K': hack_dir = 8; return('.');
	case 'U': hack_dir = 9; return('.');

	/* Tunnelling (control + rogue keys) */
	case KTRL('B'): hack_dir = 1; return('+');
	case KTRL('J'): hack_dir = 2; return('+');
	case KTRL('N'): hack_dir = 3; return('+');
	case KTRL('H'): hack_dir = 4; return('+');
	case KTRL('L'): hack_dir = 6; return('+');
	case KTRL('Y'): hack_dir = 7; return('+');
	case KTRL('K'): hack_dir = 8; return('+');
	case KTRL('U'): hack_dir = 9; return('+');

	/* Oops, audio mixer */
	case KTRL('F'): return(KTRL('U'));
	case KTRL('V'): return(KTRL('N'));
	//case KTRL('X'): return(KTRL('C')); --we use ctrl+x for ghost powers and are out of keys, so this just doesn't exist in rogue-like keyset :/

	/* Hack -- Commit suicide */
	/* ..instead display fps */
	//case KTRL('C'): return('Q');
	/* Force-stack items */
	case KTRL('C'): return('K');

	/* Hack -- White-space */
	case KTRL('M'): return('\r');

	/* Allow use of the "destroy" command */
	case KTRL('D'): return('k');

	/* Ghost powers (formerly 'Undead powers', but we have Vampire race now ^^) */
	case KTRL('X'): return('U');
	/* Locate player on map */
	case KTRL('W'): return('L');
	/* Browse a book (Peruse) */
	case 'P': return('b');
	/* Steal */
	case KTRL('A'): return('j');
	/* Toggle search mode */
	case '#': return('S');
	/* Use a staff (Zap) */
	case 'Z': return('u');
	/* Take off equipment */
	case 'T': return('t');
	/* Fire an item */
	case 't': return('f');
	/* Bash a door (Force) */
	case 'f': return('B');
	/* Look around (examine) */
	case 'x': return('l');
	/* Aim a wand (Zap) */
	case 'z': return('a');
	/* Zap a rod (Activate) */
	case 'a': return('z');
	/* Party mode */
	case 'O': return('P');

	/* Secondary 'wear/wield' */
	//case 'W': return('W');
	/* Swap item */
	case 'S': return('x');
	/* House commands */
	case KTRL('E'): return('h');
	/* Reapply auto-inscriptions */
	case KTRL('G'): return('H');
	/* Pick up exactly one item from a stack of same type of items (ie from piles, containing up to 99 <number> of this item) */
	case KTRL('Z'): return(KTRL('G'));

	/* Run */
	case ',': return('.');
	/* Stay still (fake direction) */
	case '.': hack_dir = 5; return(',');
	/* Stay still (fake direction) */
	case '5': hack_dir = 5; return(',');

	/* Standard walking */
	case '1': hack_dir = 1; return(';');
	case '2': hack_dir = 2; return(';');
	case '3': hack_dir = 3; return(';');
	case '4': hack_dir = 4; return(';');
	case '6': hack_dir = 6; return(';');
	case '7': hack_dir = 7; return(';');
	case '8': hack_dir = 8; return(';');
	case '9': hack_dir = 9; return(';');
	}

	/* Default */
	return(command);
}



/*
 * Convert an "Original" keypress into an "Angband" keypress
 * Pass direction information back via "hack_dir".
 *
 * Note that "Original" and "Angband" are very similar.
 */
char original_commands(char command) {
	/* Process the command */
	switch (command) {

	/* Hack -- White space */
	case KTRL('J'): return('\r');
	case KTRL('M'): return('\r');

	/* Tunnel */
	case 'T': return('+');

	/* Run */
	case '.': return('.');

	/* Stay still (fake direction) */
	case ',': hack_dir = 5; return(',');

	/* Stay still (fake direction) */
	case '5': hack_dir = 5; return(',');

	/* Standard walking */
	case '1': hack_dir = 1; return(';');
	case '2': hack_dir = 2; return(';');
	case '3': hack_dir = 3; return(';');
	case '4': hack_dir = 4; return(';');
	case '6': hack_dir = 6; return(';');
	case '7': hack_dir = 7; return(';');
	case '8': hack_dir = 8; return(';');
	case '9': hack_dir = 9; return(';');

	//(display fps) case KTRL('C'): return('Q');
	/* Hack -- Commit suicide */
	//case KTRL('K'): return('Q'); //ctrl+q surely is enough, can leave this key free
	}

	/* Default */
	return(command);
}



/*
 * React to new value of "rogue_like_commands".
 *
 * Initialize the "keymap" arrays based on the current value of
 * "rogue_like_commands".  Note that all "undefined" keypresses
 * by default map to themselves with no direction.  This allows
 * "standard" commands to use the same keys in both keysets.
 *
 * To reset the keymap, simply set "rogue_like_commands" to -1,
 * call this function, restore its value, call this function.
 *
 * The keymap arrays map keys to "command_cmd" and "command_dir".
 *
 * It is illegal for keymap_cmds[N] to be zero, except for
 * keymaps_cmds[0], which is unused.
 *
 * You can map a key to "tab" to make it "non-functional".
 */
void keymap_init(void) {
	int i, k;

	/* Initialize every entry */
	for (i = 0; i < 128; i++) {
		/* Default to "no direction" */
		hack_dir = 0;

		/* Attempt to translate */
		if (c_cfg.rogue_like_commands)
			k = roguelike_commands(i);
		else
			k = original_commands(i);

		/* Save the keypress */
		keymap_cmds[i] = k;

		/* Save the direction */
		keymap_dirs[i] = hack_dir;
	}


#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
	nks_minlen = strlen(nks_start) + strlen(nks_skterm) + strlen(nks_skip) + strlen(nks_term) + 1;
#endif
}


/*
 * Flush the screen, make a noise
 */
void bell(void) {
	/* Mega-Hack -- Flush the output */
	Term_fresh();

	/* Make a bell noise (if allowed) */
	if (c_cfg.ring_bell) {
#ifdef USE_SOUND_2010
#ifdef SOUND_SDL
		/* Try to beep via bell sfx of the SDL audio system first */
		if (!sound_bell()
		    //&& !(c_cfg.audio_paging && sound_page())
		    )
#endif
#endif
		if (!c_cfg.quiet_os) Term_xtra(TERM_XTRA_NOISE, 0);
	}

	/* Flush the input (later!) */
	flush();

	/* hack for safe_macros item prompts
	   (Bugfix: Only set abort_prompt to TRUE if we're actually parsing a macro,
	   otherwise we end up with a lingering rogue 'abort_prompt' that will cause
	   any next macro to abort in undefined ways even if the macro was fine,
	   possibly causing the @ menu to pop up if the macro contained an '@' (eg for call-by-name).) */
	if (parse_macro) abort_prompt = TRUE;
}
/* Same as bell() except it doesn't make a sound :D */
void bell_silent(void) {
	/* Mega-Hack -- Flush the output */
	Term_fresh();

	/* Flush the input (later!) */
	flush();

	/* hack for safe_macros item prompts
	   (Bugfix: Only set abort_prompt to TRUE if we're actually parsing a macro,
	   otherwise we end up with a lingering rogue 'abort_prompt' that will cause
	   any next macro to abort in undefined ways even if the macro was fine,
	   possibly causing the @ menu to pop up if the macro contained an '@' (eg for call-by-name).) */
	if (parse_macro) abort_prompt = TRUE;
}

/* Generate a page sfx (beep) */
int page(void) {
#ifdef USE_SOUND_2010
#ifdef SOUND_SDL
	/* Try to beep via page sfx of the SDL audio system first */
	if (c_cfg.audio_paging && sound_page()) return(1);
#endif
#endif

	/* Fall back on system-specific default beeps */
	//Term_fresh();
	if (!c_cfg.quiet_os) Term_xtra(TERM_XTRA_NOISE, 0);
	//flush();

	return(1);
}
/* Generate a warning sfx (beep) or if it's missing then a page sfx */
int warning_page(void) {
#ifdef USE_SOUND_2010
#ifdef SOUND_SDL
	/* Try to beep via warning sfx of the SDL audio system first */
	if (sound_warning()) return(1);
	//if (c_cfg.audio_paging && sound_page()) return(1);
#endif
#endif

	/* Fall back on system-specific default beeps */
	//Term_fresh();
	if (!c_cfg.quiet_os) Term_xtra(TERM_XTRA_NOISE, 0);
	//flush();

	return(1);
}


/*
 * Display a string on the screen using an attribute, and clear
 * to the end of the line.
 */
void c_prt(byte attr, cptr str, int row, int col) {
	/* Hack -- fake monochrome */
	/* if (!c_cfg.use_color) attr = TERM_WHITE; */

	/* Clear line, position cursor */
	Term_erase(col, row, 255);

	/* Dump the attr/text */
	Term_addstr(-1, attr, str);
}

/* Like c_prt(), but allow the usual colour codes -- for future use in s_aux.lua instead of c_prt(), for coloured spell info */
void cc_prt(byte attr, cptr str, int row, int col) {
	/* Hack -- fake monochrome */
	/* if (!c_cfg.use_color) attr = TERM_WHITE; */

	/* Clear line, position cursor */
	Term_erase(col, row, 255);

	/* Dump the attr/text */
	Term_putstr(col, row, -1, attr, (char*)str);
}

/*
 * As above, but in "white"
 */
void prt(cptr str, int row, int col) {
	/* Spawn */
	c_prt(TERM_WHITE, str, row, col);
}


/*
 * Print the last n characters of str onto the screen.
 */
static void c_prt_last(byte attr, char *str, int y, int x, int n) {
	int len = strlen(str);

	if (n < len)
		Term_putstr(x, y, n, attr, str + (len - n));
	else
		Term_putstr(x, y, len, attr, str);
}
/* Added for new string input editability - C. Blue */
static void c_prt_n(byte attr, char *str, int y, int x, int n) {
	char tmp[strlen(str) + 1];

	strncpy(tmp, str, n);
	tmp[n] = 0;

	Term_putstr(x, y, -1, attr, tmp);
}

#if defined(WINDOWS) || defined(USE_X11)
/* Helper function for copy_to_clipboard */
static void extract_url(char *buf_esc, char *buf_prev, int end_of_name) {
	char *c, *c2, *be = NULL;

	/* Hack: Double-tapping 'copy_to_clipboard' tries to extract an URL.
	   So ignore the first tap here. */
	if (strcmp(buf_prev, buf_esc)) return;

//c_msg_format("1: %s", buf_esc);
//c_msg_format("2: %s", buf_prev);

 #if 0
c_msg_format("%c/%c/%c/%c - %c/%c/%c/%c - %c/%c/%c/%c - %c/%c/%c/%c",
    buf_esc[0], buf_esc[1], buf_esc[2], buf_esc[3], buf_esc[4], buf_esc[5], buf_esc[6], buf_esc[7],
    buf_esc[8], buf_esc[9], buf_esc[10], buf_esc[11], buf_esc[12], buf_esc[13], buf_esc[14], buf_esc[15]);
 #endif

	/* Catch chat messages (the usual case) because the player's name might have sort of "url form" :-p
	   Problem: /me messages don't have a closing ']' after the name, so we use a hack by marking the end of the name with a '{-' neutral colour code,
	   however, this hack is currently disabled as it's a bit annoying that we cannot separate forged messages from real '/me' messages anyway.
	   So let's just go with "extract_url() doesn't work for /me messages" for now.. -_- */
	if (end_of_name) be = buf_esc + end_of_name; // <- ^ disabled atm
	else {
		if (buf_esc[0] == '[') be = strchr(buf_esc, ']');
		if (be == buf_esc + strlen(buf_esc) - 1) be = NULL; //catch '/me' messages where the ']' is at the very end of the message
		if (!be) be = buf_esc; else be++;
	}
	if (!be[0]) return;

	/* First try a simple method for easy to recognize ULRs */
	if ((c = strstr(be, "http")) || (c2 = strstr(be, "www."))) {
		if (!c || (c[4] != ':' && c[5] != ':')) c = NULL; else c2 = NULL;
		if (!c) c = c2;
		if (c) {
			if (prefix(c, "https://")) c2 = c + 8; //silyl compiler warning
			else if (prefix(c, "http://")) c2 = c + 7; //silyl compiler warning
			else if (prefix(c, "www.")) c2 = c + 4;
			else c2 = NULL; //kill compiler warning

			if (c2 && strcspn(c2, ".") < strcspn(c2, " ,/\\?*:;+}{][()!\"$%&'~|<>=")) { /* only hyphen is allowed in domain names */
			// "/([\w+]+\:\/\/)?([\w\d-]+\.)*[\w-]+[\.\:]\w+([\/\?\=\&\#\.]?[\w-]+)*\/?/gm" // Find really all URLs, but also matches clock times, so...
			// "[0-9]:[0-9][0-9][^0-9]" // ..exclude clock times specifically here.

				int pos;

				pos = strcspn(c2, " "); /* not allowed in any part of the URL, thereby terminating it */
				strcpy(buf_prev, c); //swap strings so we don't do overlapping copying..
				buf_prev[pos + (c2 - c)] = 0; //wrong compiler warning (c2 uninitialized)
				strcpy(buf_esc, buf_prev);

				/* Trim trailing dot, if any - in case someone wrote an URL and 'terminated' his line grammatically with a dot, like a normal sentence. */
				if (buf_esc[strlen(buf_esc) - 1] == '.') buf_esc[strlen(buf_esc) - 1] = 0;

				/* 'Reset' double-tapping */
				buf_prev[0] = 0;
//c_msg_format("3a: %s", buf_esc);
				return;
			}
		}
	}
 #ifdef REGEX_URL
	/* Also try to catch less clear URLs */
	else {
		regmatch_t pmatch[REGEX_ARRAY_SIZE + 1]; /* take out the heavy calibre (´ `) */
		regex_t re;
		int status = -999;

		status = regcomp(&re, "[a-z0-9][a-z0-9]*\\.[a-z0-9][.a-z0-9]*(/[^ ]*)?", REG_EXTENDED|REG_ICASE);
		if (status != 0) return; //error

		status = regexec(&re, be, REGEX_ARRAY_SIZE, pmatch, 0);
		if (status) {
			regfree(&re);
			return; //not found
		}
		if (pmatch[0].rm_so == -1) {
			regfree(&re);
			return; //paranoia
		}

		/* Just take the first match of the line.. */
		strcpy(buf_prev, be); //swap strings so we don't do overlapping copying..
		strcpy(buf_esc, buf_prev + pmatch[0].rm_so);
		buf_esc[pmatch[0].rm_eo - pmatch[0].rm_so] = 0;

		/* Trim trailing dot, if any - in case someone wrote an URL and 'terminated' his line grammatically with a dot, like a normal sentence. */
		if (buf_esc[strlen(buf_esc) - 1] == '.') buf_esc[strlen(buf_esc) - 1] = 0;

		/* 'Reset' double-tapping */
		buf_prev[0] = 0;

		regfree(&re);
//c_msg_format("3b: %s", buf_esc);
		return;
	}
 #endif
}
#endif

/* For copying and pasting: Don't duplicate (or reduce again) first (or any) ':'. */
//#define NO_COLON_DUPLICATION --not implemented, no use for it atm
/* copy a line of text to clipboard - C. Blue
   'chat_input': We're copying from an active chat input prompt. */
void copy_to_clipboard(char *buf, bool chat_input) {
#ifdef WINDOWS
	size_t len;
	int pos = 0, end_of_name = 0;
	char *c, *c2, buf_esc[MSG_LEN + 10]; //+10: room for turning '/' occurances into '//'
	static char buf_prev[MSG_LEN + 10];

	/* escape all ' (up to 10 before overflow) */
	c = buf;
	c2 = buf_esc;
	while (*c) {
		switch (*c) {
		case ':':
			if (pos != 0 && pos <= NAME_LEN) {
				if (*(c + 1) == ':') c++;
			}
			/* Catch URLs pasted with double :: too */
			else if (pos != 0) {
				if (*(c + 1) == ':') c++;
			}
			break;
		case '{':
			if (chat_input)
				switch (*(c + 1)) {
				case '{':
					c++;
					break;
				case 0: /* broken colour code (paranoia) */
					c++;
					continue;
				default: /* assume colour code and discard */
					c += 2;
					continue;
				}
			break;
		/* skip special markers */
		case '\376':
		case '\375':
		case '\374':
			c++;
			continue;
		/* strip colour codes */
		case '\377':
			switch (*(c + 1)) {
#if 0 /* disabled for now, as we need to check whether a message is '/me' style but those could be forged anyway, so it's all not really clean -_- */
			case '-': /* hack: '/me' marker for end-of-name, in case the name is url-like so we know to skip it */
				end_of_name = c - buf;
				c++;
				continue;
#endif
			case 0: /* broken colour code (paranoia) */
				c++;
				continue;
			default: /* assume colour code and discard */
				c += 2;
				continue;
			}
			break;
		case '\\': *c2++ = '\\';
		}
		*c2 = *c;
		c++;
		c2++;
		pos++;
	}
	*c2 = 0;

	extract_url(buf_esc, buf_prev, end_of_name);

	len = strlen(buf_esc) + 1;
	HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, len);
	memcpy(GlobalLock(hMem), buf_esc, len);
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
	strcpy(buf_prev, buf_esc);
#endif

#ifdef USE_X11 /* relies on xclip being installed! */
	int r, pos = 0, end_of_name = 0;
	char *c, *c2, buf_esc[MSG_LEN + 10]; //+10: room for turning '/' occurances into '//'
	static char buf_prev[MSG_LEN + 10];

	/* escape all ' (up to 10 before overflow) */
	c = buf;
	c2 = buf_esc;
	while (*c && strlen(c2) < MSG_LEN + 5) {
		switch (*c) {
		case '\'':
			*c2++ = '\'';
			*c2++ = '"';
			*c2++ = '\'';
			*c2++ = '"';
			break;
		case ':':
			if (pos != 0 && pos <= NAME_LEN) {
				if (*(c + 1) == ':') c++;
			}
			/* Catch URLs pasted with double :: too */
			else if (pos != 0) {
				if (*(c + 1) == ':') c++;
			}
			break;
		case '{':
			if (chat_input)
				switch (*(c + 1)) {
				case '{':
					c++;
					break;
				case 0: /* broken colour code (paranoia) */
					c++;
					continue;
				default: /* assume colour code and discard */
					c += 2;
					continue;
				}
			break;
		/* skip special markers */
		case '\376':
		case '\375':
		case '\374':
			c++;
			continue;
		/* strip colour codes */
		case '\377':
			switch (*(c + 1)) {
#if 0 /* disabled for now, as we need to check whether a message is '/me' style but those could be forged anyway, so it's all not really clean -_- */
			case '-': /* hack: '/me' marker for end-of-name, in case the name is url-like so we know to skip it */
				end_of_name = c - buf;
				c++;
				continue;
#endif
			case 0: /* broken colour code (paranoia) */
				c++;
				continue;
			default: /* assume colour code and discard */
				c += 2;
				continue;
			}
			break;
		case '\\': *c2++ = '\\';
		}
		*c2 = *c;
		c++;
		c2++;
		pos++;
	}
	*c2 = 0;

	extract_url(buf_esc, buf_prev, end_of_name);

	r = system(format("echo -n '%s' | xclip -sel clip", buf_esc));
	if (r) c_msg_print("Copy failed, make sure xclip is installed.");
	else strcpy(buf_prev, buf_esc);
#endif
}
/* Paste current clipboard into active chat input.
   'global': paste goes to global chat (including /say and /whisper)? (not private/party/guild/floor chat) -
   For the latter four the line already started with a ':' for the chat prefix and we don't need to duplicate the first ':' anymore. */
bool paste_from_clipboard(char *buf, bool global) {
#if defined(WINDOWS) || defined(USE_X11)
	bool no_slash_command;
	int pos = 0;
	char *c, *c2, buf_esc[MSG_LEN + 15];
#else
	c_msg_print("\377yClipboard operations not available in GCU client.");
#endif

#ifdef WINDOWS
	if (OpenClipboard(NULL)) {
		HANDLE hClipboardData = GetClipboardData(CF_TEXT);
		if (hClipboardData) {
			CHAR *pchData = (CHAR*) GlobalLock(hClipboardData);

			if (pchData) {
				strncpy(buf_esc, pchData, MSG_LEN - NAME_LEN - 13); //just accomodate for some colour codes and spacing, not really calculated it
				buf_esc[MSG_LEN - NAME_LEN - 13] = 0;

				/* treat { and : and also strip away all control chars (like 0x0A aka RETURN) */
				c = buf_esc;
				c2 = buf;
				no_slash_command = buf_esc[0] != '/';
				while (*c) {
					if (*c < 32) {
						c++;
						continue;
					}

					switch (*c) {
					case ':':
						if (global && no_slash_command && pos != 0 && pos <= NAME_LEN) {
							*c2 = ':';
							c2++;
							global = FALSE; /* only the first ':' needs duplication */
						}
						break;
					case '{':
						*c2 = '{';
						c2++;
						break;
					}
					*c2 = *c;
					c++;
					c2++;
					pos++;
				}
				*c2 = 0;

				GlobalUnlock(hClipboardData);
			} else return(FALSE);
		} else return(FALSE);
		CloseClipboard();
	} else return(FALSE);
	return(TRUE);
#endif

#ifdef USE_X11 /* relies on xclip being installed! */
	FILE *fp;
	int r;
	char buf_line[MSG_LEN];
	bool max_length_reached = FALSE;

	r = system("xclip -sel clip -o > __clipboard__");
	if (r || !(fp = fopen("__clipboard__", "r"))) {
		c_msg_print("Paste failed, make sure xclip is installed.");
		return(FALSE);
	}

	/* combine multi-line text into one line, replacing the RETURNs by spaces if needed */
	buf_esc[0] = 0;
	while (!max_length_reached && fgets(buf_line, MSG_LEN - NAME_LEN - 13, fp)) { //just accomodate for some colour codes and spacing, not really calculated it
		/* limit total length of message */
		if (strlen(buf_esc) + strlen(buf_line) > MSG_LEN - 1) {
			buf_line[MSG_LEN - 1 - strlen(buf_esc)] = 0;
			max_length_reached = TRUE;
		}

		if (buf_esc[0] && buf_esc[strlen(buf_esc) - 1] != ' ') strcat(buf_esc, " ");
		strcat(buf_esc, buf_line);
	}
	//else buf_esc[strlen(buf_esc) - 1] = 0; //remove trailing newline -- there is no trailing newline!
	/* treat { and : and also strip away all control chars (like 0x0A aka RETURN) */
	c = buf_esc;
	c2 = buf;
	no_slash_command = buf_esc[0] != '/';
	while (*c) {
		if (*c < 32) {
			c++;
			continue;
		}

		switch (*c) {
		case ':':
			if (global && no_slash_command && pos != 0 && pos <= NAME_LEN) {
				*c2 = ':';
				c2++;
				global = FALSE; /* only the first ':' needs duplication */
			}
			break;
		case '{':
			*c2 = '{';
			c2++;
			break;
		}
		*c2 = *c;
		c++;
		c2++;
		pos++;
	}
	*c2 = 0;

	fclose(fp);
	return(TRUE);
#endif

	return(FALSE);
}

#define SEARCH_NOCASE /* CTRL+C chat history search: Case-insensitive? */
/* Helper function for message-history search done inside askfor_aux(),
   supporting wildcards '*'. - C. Blue */
static bool search_history_aux(const char *msg, const char *buf) {
	static char tmpbuf[MSG_LEN], *tmpc, *tmpc2, swapbuf[MSG_LEN];
	static const char *msgc, *msgc_tmp;

	/* Handle wildcard segments (or final term) on a working copy */
	strcpy(tmpbuf, buf);
	tmpc = tmpbuf;
	msgc = msg;
	while (*tmpc) {
#if 0 /* overwriting own buffer? */
		strcpy(tmpbuf, tmpc);
#else /* safe copy */
		strcpy(swapbuf, tmpc);
		strcpy(tmpbuf, swapbuf);
#endif

		if ((tmpc2 = strchr(tmpbuf, '*'))) {
			*tmpc2 = 0;
			tmpc = tmpc2 + 1;
		} else tmpc = tmpbuf + strlen(tmpbuf);
#ifdef SEARCH_NOCASE
		if (!(msgc_tmp = my_strcasestr(msgc, tmpbuf))) {
			tmpc = NULL;
			break;
		}
		msgc = msgc_tmp + strlen(tmpbuf);
#else
		if (!(msgc_tmp = strstr(msgc, tmpbuf))) {
			tmpc = NULL;
			break;
		}
		msgc = msgc_tmp + strlen(tmpbuf);
#endif
	}
	return(tmpc != NULL);
}
/*
 * Get some input at the cursor location.
 * Assume the buffer is initialized to a default string.
 * Note that this string is often "empty" (see below).
 * The default buffer is displayed in yellow until cleared.
 * Pressing RETURN right away accepts the default entry.
 * Normal chars clear the default and append the char.
 * Backspace clears the default or deletes the final char.
 * ESCAPE clears the buffer and the window and returns FALSE.
 * RETURN accepts the current buffer contents and returns TRUE.
 */

/* APD -- added private so passwords will not be displayed. */
/*
 * Jir -- added history.
 * TODO: cursor editing (fix past terminal width extending text)
 */
typedef char msg_hist_var[MSG_HISTORY_MAX][MSG_LEN];
bool askfor_aux(char *buf, int len, char mode) {
	int y, x;
	int i = 0;
	int k = 0; /* Is the end of line */
	int l = 0; /* Is the cursor location on line */
	int j, j2; /* Loop iterator */

	bool search = FALSE, search_changed;
	int sp_iter = 0, sp_size = 0, sp_end = 0;
	msg_hist_var *sp_msg = NULL;

	bool tail = FALSE;
	int l_old = l;
	bool modify_allowed = TRUE;

	/* For clipboard pasting */
	char tmpbuf[MSG_LEN], *tmpc;
	int tmpl;

	/* Terminal width */
	int wid = 80;

	/* Visible length on the screen */
	int vis_len = len;

	bool done = FALSE;

	/* Hack -- if short, don't use history */
	bool nohist = (mode & ASKFOR_PRIVATE) || len < 20;
	int cur_hist;


	if (mode & ASKFOR_CHATTING) cur_hist = hist_chat_end;
	else cur_hist = hist_end;

	/* Handle wrapping */
	if (cur_hist >= MSG_HISTORY_MAX) cur_hist = 0;

	/* Locate the cursor */
	Term_locate(&x, &y);

	/* The top line is "icky" */
	topline_icky = TRUE;

	/* Paranoia -- check len */
	if (len < 1) len = 1;

	/* Paranoia -- check column */
	if ((x < 0) || (x >= wid - 1)) x = 0;

	/* Restrict the visible length */
	if (x + vis_len > wid - 1) vis_len = wid - 1 - x;

	/* Paranoia -- Clip the default entry */
	buf[len] = '\0';

	/* Display the default answer */
	Term_erase(x, y, len);
	if (mode & ASKFOR_PRIVATE) {
		c_prt_last(TERM_YELLOW, buf[0] ? "(default)" : "", y, x, vis_len);
		modify_allowed = FALSE;
	} else c_prt_last(TERM_YELLOW, buf, y, x, vis_len);

	if (mode & ASKFOR_CHATTING) {
		strncpy(message_history_chat[hist_chat_end], buf, sizeof(*message_history_chat) - 1);
		message_history_chat[hist_chat_end][sizeof(*message_history_chat) - 1] = '\0';
	}
	else if (!nohist) {
		strncpy(message_history[hist_end], buf, sizeof(*message_history) - 1);
		message_history[hist_end][sizeof(*message_history) - 1] = '\0';
	}

	/* Process input */
	while (!done) {
		search_changed = FALSE;

		/* Place cursor */
		if (strlen(buf) > vis_len) {
			if (l > strlen(buf) - vis_len)
				Term_gotoxy(x + l + vis_len - strlen(buf), y);
			else
				Term_gotoxy(x, y); //would need a new var 'x_edit' to allow having the cursor at other positions
		} else
			Term_gotoxy(x + l, y);

		/* Get a key */
		i = inkey_combo(modify_allowed, &k, buf);

		/* Analyze the key */
		switch (i) {
		case ESCAPE:
		//case KTRL('Q'):
			/* Catch searching mode */
#if 0 /* Clear current search, but stay in searching mode */
			if (search && buf[0]) {
#else /* Clear current search and leave searching mode */
			if (search) {
				search = FALSE;
#endif
				Term_erase(x, y, strlen(buf));
				buf[0] = '\0';
				k = l = 0;
				break;
			}

			/* Leave */
			k = 0;
			done = TRUE;
			break;

#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
		/* Discard those positioning keys that are unusable for string input: */
		case NAVI_KEY_UP:
		case NAVI_KEY_DOWN:
		case NAVI_KEY_PAGEUP:
		case NAVI_KEY_PAGEDOWN:
			continue;
#endif

#if 0 /* urxvt actually seems to recognize 0x7F as BACKSPACE, unlike other terminals \
	which just ignore it, resulting in the Backspace key working like Delete key, \
	so we better keep this disabled here and move it back to '\010' below. - C. Blue */
		case 0x7F: /* DEL or ASCII 127 removes the char under cursor */
#endif
		case KTRL('D'):
		case NAVI_KEY_DEL:
			/* Navigational key pressed -> implicitely enter edit mode */
			if (modify_allowed) k = strlen(buf);

			if (k > l) {
				/* Move the rest of the line one back */
				for (j = l + 1; j < k; j++) buf[j - 1] = buf[j];
				k--;
				if (search) search_changed = TRUE; /* Search term was changed */
			}
			break;

		case 0x7F: /* POSIX (there DEL = BACKSPACE): well...not DEL but Backspace too it seems =P */
		case '\010': /* Backspace removes char before cursor */
			if (k == l && k > 0) { /* Pressed while cursor is at the end of the line */
				k--;
				l--;
				if (search) search_changed = TRUE; /* Search term was changed */
			}
			else if (k > l && l > 0) { /* Pressed while cursor is somewhere in between the line */
			  /* Move the rest of the line one back, including
			     char under cursor and cursor) */
				for (j = l; j < k; j++) buf[j - 1] = buf[j];
				l--;
				k--;
				if (search) search_changed = TRUE; /* Search term was changed */
			}
			else if (!l && k > 0) { /* Pressed while cursor is at the beginning of the line */
#if 0
			    /* Specialty: Behave like DELETE key instead of BACKSPACE key if at the start of the text */
				/* Move the rest of the line one back */
				for (j = l + 1; j < k; j++) buf[j - 1] = buf[j];
				k--;
				if (search) search_changed = TRUE; /* Search term was changed */
#else
			    /* Specialty: Erase the whole line, emulating the "usual" behaviour when any key is pressed without ALLOW_NAVI_KEYS_IN_PROMPT enabled, if user didn't explicitely enter edit mode via CTRL+E. */
				k = 0;
#endif
			}
			break;

		/* move by one */
		case KTRL('A'):
		case NAVI_KEY_LEFT:
			/* Navigational key pressed -> implicitely enter edit mode */
			if (modify_allowed) k = strlen(buf);

			if (l > 0) l--;
			break;
		case KTRL('S'):
		case NAVI_KEY_RIGHT:
			/* Navigational key pressed -> implicitely enter edit mode */
			if (modify_allowed) k = strlen(buf);

			if (l < k) l++;
			break;
		/* jump words */
		case KTRL('Q'):
			/* Navigational key pressed -> implicitely enter edit mode */
			if (modify_allowed) k = strlen(buf);

			if (!l) break;

			tail = FALSE;

			for (--l; l >= 0; l--) {
				if (!((buf[l] >= 'a' && buf[l] <= 'z') || (buf[l] >= 'A' && buf[l] <= 'Z') || (buf[l] >= '0' && buf[l] <= '9')) && tail) {
					/* stop on beginning of the word */
					l++;
					break;
				}
				tail = TRUE;
			}
			if (l < 0) l = 0;
			break;
		case KTRL('W'):
			/* Navigational key pressed -> implicitely enter edit mode */
			if (modify_allowed) k = strlen(buf);

			if (l == k) break;

			tail = FALSE;

			for (++l; l <= k; l++) {
				if (!((buf[l] >= 'a' && buf[l] <= 'z') || (buf[l] >= 'A' && buf[l] <= 'Z') || (buf[l] >= '0' && buf[l] <= '9')) && tail)
					/* stop after the end of the word, ON the separator */
					break;
				tail = TRUE;
			}
			if (l > k) l = k;
			break;
		/* end/begin (pos1) */
		case KTRL('V'):
		case NAVI_KEY_POS1:

			/* Navigational key pressed -> implicitely enter edit mode */
			if (modify_allowed) k = strlen(buf);

			l = 0;
			break;
		case KTRL('B'):
		case NAVI_KEY_END:
			/* Navigational key pressed -> implicitely enter edit mode */
			if (modify_allowed) k = strlen(buf);

			l = k;
			break;

		case KTRL('I'): /* TAB */
			if (mode & ASKFOR_CHATTING) {
				/* Change chatting mode - mikaelh */
				chat_mode++;
				if (chat_mode == CHAT_MODE_PARTY && !party_info_name[0]) chat_mode++; /* chat mode index order matters: party < guild */
				if (chat_mode == CHAT_MODE_GUILD && !guild_info_name[0]) chat_mode++;
				if (chat_mode > CHAT_MODE_GUILD) chat_mode = CHAT_MODE_NORMAL;

				/* HACK - Change the prompt */
				switch (chat_mode) {
				case CHAT_MODE_PARTY:
					c_prt(C_COLOUR_CHAT_PARTY, "Party: ", 0, 0);
					/* Recalculate visible length */
					vis_len = wid - 1 - sizeof("Party: ");
					break;
				case CHAT_MODE_LEVEL:
					c_prt(C_COLOUR_CHAT_LEVEL, "Floor: ", 0, 0);

					/* Recalculate visible length */
					vis_len = wid - 1 - sizeof("Floor: ");
					break;
				case CHAT_MODE_GUILD:
					c_prt(C_COLOUR_CHAT_GUILD, "Guild: ", 0, 0);

					/* Recalculate visible length */
					vis_len = wid - 1 - sizeof("Guild: ");
					break;
				default:
					prt("Message: ", 0, 0);

					/* Recalculate visible length */
					vis_len = wid - 1 - sizeof("Message: ");
					break;
				}

				Term_locate(&x, &y);

				if (search) search_changed = TRUE; /* Redraw search term as it was visually erased */
			}
			break;

		case KTRL('U'): /* reverse KTRL('I') */
			if (mode & ASKFOR_CHATTING) {
				/* Reverse change chatting mode */
				chat_mode--;
				if (chat_mode == CHAT_MODE_GUILD && !guild_info_name[0]) chat_mode--; /* chat mode index order matters: guild > party */
				if (chat_mode == CHAT_MODE_PARTY && !party_info_name[0]) chat_mode--;
				if (chat_mode < CHAT_MODE_NORMAL) chat_mode = CHAT_MODE_GUILD;

				/* HACK - Change the prompt */
				switch (chat_mode) {
				case CHAT_MODE_PARTY:
					c_prt(C_COLOUR_CHAT_PARTY, "Party: ", 0, 0);

					/* Recalculate visible length */
					vis_len = wid - 1 - sizeof("Party: ");
					break;
				case CHAT_MODE_LEVEL:
					c_prt(C_COLOUR_CHAT_LEVEL, "Floor: ", 0, 0);

					/* Recalculate visible length */
					vis_len = wid - 1 - sizeof("Floor: ");
					break;
				case CHAT_MODE_GUILD:
					c_prt(C_COLOUR_CHAT_GUILD, "Guild: ", 0, 0);

					/* Recalculate visible length */
					vis_len = wid - 1 - sizeof("Guild: ");
					break;
				default:
					prt("Message: ", 0, 0);

					/* Recalculate visible length */
					vis_len = wid - 1 - sizeof("Message: ");
					break;
				}

				Term_locate(&x, &y);

				if (search) search_changed = TRUE; /* Redraw search term as it was visually erased */
			}
			break;

		case KTRL('R'): /* All/general ('message') */
			if (mode & ASKFOR_CHATTING) {
				chat_mode = CHAT_MODE_NORMAL;
				prt("Message: ", 0, 0);

				/* Recalculate visible length */
				vis_len = wid - 1 - sizeof("Message: ");

				Term_locate(&x, &y);

				if (search) search_changed = TRUE; /* Redraw search term as it was visually erased */
			}
			break;

		case KTRL('Y'): /* Party */
			if (mode & ASKFOR_CHATTING) {
				chat_mode = CHAT_MODE_PARTY;
				c_prt(C_COLOUR_CHAT_PARTY, "Party: ", 0, 0);

				/* Recalculate visible length */
				vis_len = wid - 1 - sizeof("Party: ");

				Term_locate(&x, &y);

				if (search) search_changed = TRUE; /* Redraw search term as it was visually erased */
			}
			break;

		case KTRL('F'): /* Floor/Depth/Level */
			/* Alternative behaviour while in 'searching' mode:
			   Searching for text in a previously entered chat message: Get previous match. */
			if (search && !nohist) { /* We are in 'message history searching' mode (and a message history does actually exist - paranoia) */
				if (!buf[0]) continue; /* Nothing typed in yet */
				if (sp_iter == sp_size) continue; /* No match exists at all */

				/* Go reverse: Look for previous match.. */
				for (j = sp_iter - 1; j > -sp_end; j--) {
					j2 = (sp_end - j + MSG_HISTORY_MAX * 2) % MSG_HISTORY_MAX; /* Reverse direction */

					if (!search_history_aux((*sp_msg)[j2], buf)) continue;

					/* Display the result message, overwriting the real 'buf' only visually, while keeping 'buf' unchanged */
					c_prt(TERM_YELLOW, format("%s: ", buf), y, x);
					c_prt(TERM_L_BLUE, format("%s", (*sp_msg)[j2]), y, x + 2 + strlen(buf));
					sp_iter = j;
					break;
				}
				break;
			}

			/* Normal behaviour outside of search-mode: Switch to 'floor' chat mode. */
			if (mode & ASKFOR_CHATTING) {
				chat_mode = CHAT_MODE_LEVEL;
				c_prt(C_COLOUR_CHAT_LEVEL, "Floor: ", 0, 0);

				/* Recalculate visible length */
				vis_len = wid - 1 - sizeof("Floor: ");

				Term_locate(&x, &y);

				//cannot happen, as CTRL+F has other behaviour (above) while searching--   if (search) search_changed = TRUE; /* Redraw search term as it was visually erased */
			}
			break;

		case KTRL('G'): /* Guild */
			if (mode & ASKFOR_CHATTING) {
				chat_mode = CHAT_MODE_GUILD;
				c_prt(C_COLOUR_CHAT_GUILD, "Guild: ", 0, 0);

				/* Recalculate visible length */
				vis_len = wid - 1 - sizeof("Guild: ");

				Term_locate(&x, &y);

				if (search) search_changed = TRUE; /* Redraw search term as it was visually erased */
			}
			break;

#if 0 /* was: erase line. Can just hit ESC/Backspace though, so if 0'ed as it's now used to cycle chat modes instead */
		case KTRL('U'):
			k = 0;
			break;
#endif

		case KTRL('N'):
			if (nohist) break;
			cur_hist++;
			if (mode & ASKFOR_CHATTING) {
				if ((!hist_chat_looped && cur_hist >= hist_chat_end) ||
				    (hist_chat_looped && cur_hist >= MSG_HISTORY_MAX))
					cur_hist = 0;
				strncpy(buf, message_history_chat[cur_hist], len);
				buf[len] = '\0';
			} else {
				if ((!hist_looped && cur_hist >= hist_end) ||
				    (hist_looped && cur_hist >= MSG_HISTORY_MAX))
					cur_hist = 0;
				strncpy(buf, message_history[cur_hist], len);
				buf[len] = '\0';
			}

			k = l = strlen(buf);

			if (search) search_changed = TRUE; /* Search term was changed */
			break;
		case KTRL('P'):
			if (nohist) break;
			if (mode & ASKFOR_CHATTING) {
				if (cur_hist) cur_hist--;
				else cur_hist = hist_chat_looped ? MSG_HISTORY_MAX - 1 : hist_chat_end - 1;
				strncpy(buf, message_history_chat[cur_hist], len);
				buf[len] = '\0';
			} else {
				if (cur_hist) cur_hist--;
				else cur_hist = hist_looped ? MSG_HISTORY_MAX - 1 : hist_end - 1;
				strncpy(buf, message_history[cur_hist], len);
				buf[len] = '\0';
			}

			k = l = strlen(buf);

			if (search) search_changed = TRUE; /* Search term was changed */
			break;

		/* word-size backspace */
		case KTRL('E'): {
			/* hack: CTRL+E is also the designated 'edit' key to edit a predefined default string */
			if (modify_allowed) k = strlen(buf);

			/* actual 'real' CTRL+E functionality following now */

			if (!l) break;
			l_old = l;

			tail = TRUE;
			for (; l > 0; l--) {
				if (((buf[l - 1] >= 'a' && buf[l - 1] <= 'z') || (buf[l - 1] >= 'A' && buf[l - 1] <= 'Z') || (buf[l - 1] >= '0' && buf[l - 1] <= '9')) != tail) continue;
				if (tail) tail = FALSE;
				else break;
			}

			for (j = 0; j < strlen(buf) - l_old; j++) buf[l + j] = buf[l_old + j];
			k -= l_old - l;
			if (k < 0) k = l = 0;

			if (search) search_changed = TRUE; /* Search term was changed */
			break;
			}

		case KTRL('T'): /* Take a screenshot */
			xhtml_screenshot("screenshot????", FALSE);
			break;

		case '\n':
		case '\r':
			/* Catch searching mode */
			if (search) {
				/* We didn't find any match? */
				if (sp_iter == sp_size) {
#if 0
					continue;
#else /* Actually switch to normal text input instead of just having to discard the unmatched search string: */
					search = FALSE;
					buf[len] = '\0';
					k = l = strlen(buf);
					c_prt(TERM_WHITE, buf, y, x);
					/* Note: For some reason the cursor becomes invisible here */
 #if 0 /* This code works randomly. Sometimes it does NOT restore cursor visibility. */
					Term_set_cursor(1);
					Term_xtra(TERM_XTRA_SHAPE, 1);
 #endif
					continue;
#endif
				}

				/* Replace our search string by the actual match and go with that */
				//strncpy(buf, (*sp_msg)[(sp_end - sp_iter) % MSG_HISTORY_MAX], len);
				strcpy(buf, (*sp_msg)[(sp_end - sp_iter + MSG_HISTORY_MAX * 2) % MSG_HISTORY_MAX]);
				search = FALSE;
				k = l = strlen(buf);
				c_prt(TERM_WHITE, buf, y, x);
				continue;
			}
			/* Proceed normally */
			buf[len] = '\0';
			k = l = strlen(buf);
			done = TRUE;
			break;

		case KTRL('C'): /* Allow searching for text in a previously entered chat message */
			if (nohist) break;
			if (!search) {
				/* Reverse search, starting with newest entries */
				if (mode & ASKFOR_CHATTING) {
					sp_size = hist_chat_looped ? MSG_HISTORY_MAX : hist_chat_end;
					sp_end = hist_chat_looped ? (hist_chat_end - 1 + MSG_HISTORY_MAX) % MSG_HISTORY_MAX : hist_chat_end - 1;
					sp_msg = &message_history_chat;
				} else {
					sp_size = hist_looped ? MSG_HISTORY_MAX : hist_end;
					sp_end = hist_looped ? (hist_end - 1 + MSG_HISTORY_MAX) % MSG_HISTORY_MAX : hist_end - 1;
					sp_msg = &message_history;
				}
				/* No history recorded yet? */
				if (sp_end < 0) break;
				/* Begin searching mode */
				sp_iter = -1;
				search = TRUE;
				//break; - don't break, start searching right away, as we might already have entered something into this chat prompt
			}
			if (!buf[0]) continue; /* Nothing typed in yet */

			/* Continue search by looking for next match */
			if (sp_iter == sp_size) continue; /* No match exists at all */
			/* Look for another match.. */
			for (j = sp_iter + 1; j < sp_size; j++) {
				j2 = (sp_end - j + MSG_HISTORY_MAX * 2) % MSG_HISTORY_MAX; /* Reverse direction */

				if (!search_history_aux((*sp_msg)[j2], buf)) continue;

				/* Display the result message, overwriting the real 'buf' only visually, while keeping 'buf' unchanged */
				c_prt(TERM_YELLOW, format("%s: ", buf), y, x);
				c_prt(TERM_L_BLUE, format("%s", (*sp_msg)[j2]), y, x + 2 + strlen(buf));
				sp_iter = j;
				break;
			}
			/* No further match found, but we did find at least one match? */
			if (j == sp_size && sp_iter != -1) {
				/* Cycle through search results: Start from the beginning again and search up to the final match again */
				for (j = 0; j <= sp_iter; j++) {
					j2 = (sp_end - j + MSG_HISTORY_MAX * 2) % MSG_HISTORY_MAX; /* Reverse direction */

					if (!search_history_aux((*sp_msg)[j2], buf)) continue;

					/* Display the result message, overwriting the real 'buf' only visually, while keeping 'buf' unchanged */
					c_prt(TERM_YELLOW, format("%s: ", buf), y, x);
					c_prt(TERM_L_BLUE, format("%s", (*sp_msg)[j2]), y, x + 2 + strlen(buf));
					sp_iter = j;
					break;
				}
			}
			/* Absolutely no match found? Need different search term to continue. */
			else if (j > sp_iter) sp_iter = sp_size;
			continue;

		case KTRL('K'): /* copy current chat line to clipboard */
			copy_to_clipboard(buf, TRUE);
			break;
		case KTRL('L'): /* paste current clipboard to chat */
			if (!paste_from_clipboard(tmpbuf, chat_mode == CHAT_MODE_NORMAL && !((tmpc = strchr(buf, ':')) && tmpc - buf < l))) { //emulate strnstr()
				bell();
				break;
			}

			/* Ensure line length isn't exceeded */
			if (strlen(buf) >= MSG_LEN - 1) {
				bell();
				break;
			}
			tmpl = strlen(tmpbuf);
			if (strlen(buf) + tmpl >= MSG_LEN) {
				bell(); //still warn, as stuff got cut off..
				tmpbuf[MSG_LEN - strlen(buf)] = 0;
			}

			/* Paste at end of line and increment k and l */
			if (k == l) {
				if (k < len) {
					strcat(buf, tmpbuf);
					k += tmpl;
					l += tmpl;
					if (search) search_changed = TRUE; /* Search term was changed */
				}
			}
			/* Paste at current cursor position after moving
			   the rest of the line forward */
			else if (k > l) {
				if (k < len) {
					for (j = k; j >= l; j--) buf[j + tmpl] = buf[j];
					strncpy(&buf[l], tmpbuf, tmpl); //exclude terminating 0
					l += tmpl; //maybe keep cursor position instead of moving forward? but this seems better for now
					k += tmpl;
					if (search) search_changed = TRUE; /* Search term was changed */
				}
			}
			break;

		default:
			/* We entered a normal character */

			/* For account/character name input: Live-replace all invalid chars with '_' like Trim_name() would do */
			if (mode & ASKFOR_LIVETRIM) {
				/* first char... */
				if (!l) {
					/* force upper-case */
					i = toupper(i);
					/* must be a letter */
					if (i < 'A' || i > 'Z') continue;
				}
				if (!((i >= 'A' && i <= 'Z') ||
				    (i >= 'a' && i <= 'z') ||
				    (i >= '0' && i <= '9') ||
				    //strchr(" .,-'&_$%~#<>|", i))) /* chars allowed for character name, */
				    strchr(" .,-'&_$%~#", i))) /* chars allowed for character name, */
					i = '_';
			}

			if (search) search_changed = TRUE; /* Search term was changed */

			/* inkey_letter_all hack for c_get_quantity() */
			//if (inkey_letter_all && !k && ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z'))) { i = 'a';
			if (inkey_letter_all && !k && (i == 'a' || i == 'A' || i == ' ')) { /* allow spacebar too */
				buf[k++] = 'a';
				done = TRUE;
				break;
			}
			/* Place character at end of line and increment k and l */
			if (k == l) {
				if ((k < len) && (isprint(i))) {
					buf[k++] = i;
					l++;
				}
			}
			/* Place character at current cursor position after moving
			   the rest of the line one step forward */
			else if (k > l) {
				if ((k < len) && (isprint(i))) {
					for (j = k; j >= l; j--) buf[j + 1] = buf[j];
					buf[l++] = i;
					k++;
				}
			}
			else bell(); //paranoia? line length should never be < cursor position
			break;
		}

		/* can only edit the default string with first key press being the designated 'edit' key */
		modify_allowed = FALSE;

		/* Terminate */
		buf[k] = '\0';

		/* Are we in message-history-searching mode? */
		if (search) {
			/* We didn't enter a normal character that changes our search string? Nothing to do then. */
			if (!search_changed) continue;

			/* empty search string initially */
			if (!buf[0]) {
				/* just skip.. */
				Term_erase(x, y, vis_len);
				sp_iter = sp_size;
				continue;
			}
			/* Search term changed: Reset search and look for first match.. */
			sp_iter = -1;
			for (j = 0; j < sp_size; j++) {
				j2 = (sp_end - j + MSG_HISTORY_MAX * 2) % MSG_HISTORY_MAX; /* Reverse direction */

				if (!search_history_aux((*sp_msg)[j2], buf)) continue;

				/* Display the result message, overwriting the real 'buf' only visually, while keeping 'buf' unchanged */
				c_prt(TERM_YELLOW, format("%s: ", buf), y, x);
				c_prt(TERM_L_BLUE, format("%s", (*sp_msg)[j2]), y, x + 2 + strlen(buf));
				sp_iter = j;
				break;
			}
			/* No match found at all */
			if (j == sp_size) {
				c_prt(TERM_ORANGE, buf, y, x);
				sp_iter = sp_size;
			}
			continue;
		}

		/* Update the entry */
		if (!(mode & ASKFOR_PRIVATE)) {
			Term_erase(x, y, vis_len);
			if (k >= vis_len) {
				if (l > k - vis_len)
					c_prt_last(TERM_WHITE, buf + k - vis_len, y, x, vis_len);
				else
					c_prt_n(TERM_WHITE, buf + l, y, x, vis_len);
			} else
#if 0 /* This erases the entire line, instead of just vis_len (done above at Term_erase()) */
				c_prt(TERM_WHITE, buf, y, x);
#else /* This just prints without extra erasing */
				Term_putstr(x, y, -1, TERM_WHITE, buf);
#endif
		} else {
			if (len > k) Term_erase(x + k, y, len - k);
			if (k) Term_putch(x + k - 1, y, TERM_WHITE, 'x');
		}
	}

	/* c_get_quantity() hack is in any case inactive again */
	inkey_letter_all = FALSE;

	/* The top line is OK now */
	topline_icky = FALSE;
	if (!c_quit) Flush_queue();

	/* Aborted */
	if (i == ESCAPE) return(FALSE);

	/* Success */
	if (nohist) return(TRUE);

	/* Add this to the (chat) history.
	   Update (chat) history whenever we repeat an old message, removing it from its old position and adding it on top.
	   This is especially useful for the search-function, as it won't give duplicate hits over time. */
	if (mode & ASKFOR_CHATTING) {
		int i, j;

		/* Scan history for duplicate of this new message */
		if (!hist_chat_looped) {
			for (i = 0; i < hist_chat_end; i++) {
				if (!strcmp(message_history_chat[i], buf)) {
					/* Found an old duplicate. Excise it. */
					for (j = i; j < hist_chat_end - 1; j++) strcpy(message_history_chat[j], message_history_chat[j + 1]);
					/* Go back one step accordingly */
					hist_chat_end--;
					break;
				}
			}
		} else {
			for (i = hist_chat_end; i < hist_chat_end + MSG_HISTORY_MAX; i++) {
				if (!strcmp(message_history_chat[i % MSG_HISTORY_MAX], buf)) {
					/* Found an old duplicate. Excise it. */
					for (j = i; j < hist_chat_end + MSG_HISTORY_MAX - 1; j++) strcpy(message_history_chat[j % MSG_HISTORY_MAX], message_history_chat[(j + 1) % MSG_HISTORY_MAX]);
					/* Go back one step accordingly */
					if (hist_chat_end) hist_chat_end--;
					else hist_chat_end = MSG_HISTORY_MAX - 1;
					break;
				}
			}
		}
		/* Append our message to history */
		strcpy(message_history_chat[hist_chat_end], buf);
		hist_chat_end++;
		if (hist_chat_end >= MSG_HISTORY_MAX) {
			hist_chat_end = 0;
			hist_chat_looped = TRUE;
		}
	} else {
		int i, j;

		/* Scan history for duplicate of this new message */
		if (!hist_looped) {
			for (i = 0; i < hist_end; i++) {
				if (!strcmp(message_history[i], buf)) {
					/* Found an old duplicate. Excise it. */
					for (j = i; j < hist_end; j++) strcpy(message_history[j], message_history[j + 1]);
					/* Go back one step accordingly */
					hist_end--;
					break;
				}
			}
		} else {
			for (i = hist_end; i < hist_end + MSG_HISTORY_MAX; i++) {
				if (!strcmp(message_history[i % MSG_HISTORY_MAX], buf)) {
					/* Found an old duplicate. Excise it. */
					for (j = i; j < hist_end + MSG_HISTORY_MAX - 1; j++) strcpy(message_history[j % MSG_HISTORY_MAX], message_history[(j + 1) % MSG_HISTORY_MAX]);
					/* Go back one step accordingly */
					if (hist_end) hist_end--;
					else hist_end = MSG_HISTORY_MAX - 1;
					break;
				}
			}
		}
		/* Append our message to history */
		strcpy(message_history[hist_end], buf);
		hist_end++;
		if (hist_end >= MSG_HISTORY_MAX) {
			hist_end = 0;
			hist_looped = TRUE;
		}
	}

	/* Handle the additional chat modes */
#if 0
	/* Slash commands and self-chat are exceptions */
	if ((mode & ASKFOR_CHATTING) && chat_mode != CHAT_MODE_NORMAL && buf[0] != '/' && !(buf[0] == '%' && buf[1] == ':'))
#else
	/* Slash commands and aimed chat (with *: prefix) are exceptions */
	if ((mode & ASKFOR_CHATTING) && chat_mode != CHAT_MODE_NORMAL && buf[0] != '/' &&
	    (buf[1] != ':' || (buf[0] != '%' && buf[0] != '!' && buf[0] != '$' && buf[0] != '#' && buf[0] != '+')))
#endif
	{
		for (i = k; i >= 0; i--)
			buf[i + 2] = buf[i];

		if (chat_mode == CHAT_MODE_PARTY) buf[0] = '!';
		else if (chat_mode == CHAT_MODE_LEVEL) buf[0] = '#';
		else if (chat_mode == CHAT_MODE_GUILD) buf[0] = '$';
		else buf[0] = '\0';
		buf[1] = ':';
		k += 2;
		buf[k] = '\0';
	}

	/* Success */
	return(TRUE);
}


/*
 * Get a string from the user
 *
 * The "prompt" should take the form "Prompt: "
 *
 * Note that the initial contents of the string is used as
 * the default response, so be sure to "clear" it if needed.
 *
 * We clear the input, and return FALSE, on "ESCAPE".
 */
bool get_string(cptr prompt, char *buf, int len) {
	bool res;
	char askfor_mode = 0x00;

	/* suppress hybrid macros */
	bool inkey_msg_old = inkey_msg;
	inkey_msg = TRUE;

	/* Display prompt */
	Term_erase(0, 0, 255);
	Term_putstr(0, 0, -1, TERM_WHITE, (char *)prompt);

	if (streq(prompt, "Message: ")) {
		askfor_mode |= ASKFOR_CHATTING;

		/* HACK - Change the prompt according to current chat mode */
		switch (chat_mode) {
		case CHAT_MODE_PARTY:
			c_prt(C_COLOUR_CHAT_PARTY, "Party: ", 0, 0);
			break;
		case CHAT_MODE_LEVEL:
			c_prt(C_COLOUR_CHAT_LEVEL, "Floor: ", 0, 0);
			break;
		case CHAT_MODE_GUILD:
			c_prt(C_COLOUR_CHAT_GUILD, "Guild: ", 0, 0);
			break;
		}
	}

	/* Ask the user for a string */
	res = askfor_aux(buf, len, askfor_mode);

	/* Clear prompt */
	if (askfor_mode & ASKFOR_CHATTING) clear_topline_forced();
	else if (res) clear_topline(); /* exited via confirming */
	else clear_topline_forced(); /* exited via ESC */

	/* restore responsiveness to hybrid macros */
	inkey_msg = inkey_msg_old;

	/* Result */
	return(res);
}


/*
 * Prompts for a keypress
 *
 * The "prompt" should take the form "Command: "
 *
 * Returns TRUE unless the character is "Escape"
 */
bool get_com(cptr prompt, char *command) {
	/* The top line is "icky" */
	topline_icky = TRUE;

	/* Display a prompt */
	prompt_topline(prompt);

	/* Get a key */
	*command = inkey();

	/* Clear the prompt */
	clear_topline();

	/* Fix the top line */
	topline_icky = FALSE;

	/* Flush any events */
	Flush_queue();

	/* Handle "cancel" */
	if (*command == ESCAPE) return(FALSE);

	/* Success */
	return(TRUE);
}
/* Like get_com() but returns -2 if BACKSPACE was pressed */
int get_com_bk(cptr prompt, char *command) {
	/* The top line is "icky" */
	topline_icky = TRUE;

	/* Display a prompt */
	prompt_topline(prompt);

	/* Get a key */
	*command = inkey();

	/* Clear the prompt */
	clear_topline();

	/* Fix the top line */
	topline_icky = FALSE;

	/* Flush any events */
	Flush_queue();

	/* Handle "cancel" */
	if (*command == ESCAPE) return(FALSE);
	else if (*command == '\010') return(-2);

	/* Success */
	return(TRUE);
}


/*
 * Request a command from the user.
 *
 * Sets command_cmd, command_dir, command_rep, command_arg.
 *
 * Note that "caret" ("^") is treated special, and is used to
 * allow manual input of control characters.  This can be used
 * on many machines to request repeated tunneling (Ctrl-H) and
 * on the Macintosh to request "Control-Caret".
 *
 * Note that this command is used both in the dungeon and in
 * stores, and must be careful to work in both situations.
 */
void request_command() {
	char cmd;


	/* Flush the input */
	/* flush(); */

	/* Get a keypress in "command" mode */

	/* Activate "command mode" */
	inkey_flag = TRUE;

	/* Activate "scan mode" */
	inkey_scan = TRUE;

	/* Get a command */
	cmd = inkey();

	/* Return if no key was pressed */
	if (!cmd) return;

	msg_flag = FALSE;
	clear_topline();

//#ifndef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
	/* Clear top line */
	if (!c_cfg.keep_topline) clear_topline();
//#endif

	/* Bypass "keymap" */
	if (cmd == '\\') {
//#ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
		/* Clear top line */
		if (c_cfg.keep_topline) clear_topline();
//#endif

		/* Get a char to use without casting */
		(void)(get_com("Command: ", &cmd));

		/* Hack -- allow "control chars" to be entered */
		if (cmd == '^') {
			/* Get a char to "cast" into a control char */
			(void)(get_com("Command: Control: ", &cmd));

			/* Convert */
			cmd = KTRL(cmd);
		}

//#ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
		/* Clear top line */
		if (c_cfg.keep_topline) clear_topline();
//#endif

		/* Use the key directly */
		command_cmd = cmd;
	} else {
		/* Hack -- allow "control chars" to be entered */
		if (cmd == '^') {
//#ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
			/* Clear top line */
			if (c_cfg.keep_topline) clear_topline();
//#endif

			/* Get a char to "cast" into a control char */
			(void)(get_com("Control: ", &cmd));

			/* Convert */
			cmd = KTRL(cmd);

//#ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
			/* Clear top line */
			if (c_cfg.keep_topline) clear_topline();
//#endif
		}

		/* Access the array info */
		command_cmd = keymap_cmds[cmd & 0x7F];
		command_dir = keymap_dirs[cmd & 0x7F];
	}

	/* Paranoia */
	if (!command_cmd) command_cmd = ESCAPE;

//#ifndef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
	/* Hack -- erase the message line. */
	if (!c_cfg.keep_topline) clear_topline();
//#endif
}

bool get_dir(int *dp) {
	int	dir = 0;
	char	command;
	cptr	p;

	p = "Direction ('*' choose target, '-' acquired target, '+' acquired/manual)?";
	get_com(p, &command);

#if 0 /* maaaybe in future: client-side monster check \
	 for best performance (might be exploitable though). \
	 Meanwhile see below. */
	if (command == '+') {
		if (!target_okay()) {
			p = "Direction ('*' to choose a target)?";
			get_com(p, &command);
		} else command = '-';
	}
#endif

	/* Handle target request */
	if (command == '*') {
		if (cmd_target()) dir = 5;
	}
	/* Handle previously acquired target, discard cast if not available */
	else if (command == '-') dir = 10;
	else if (command == '+') dir = 11;
	/* Normal direction, including 5 for 'acquired target or self' */
	else dir = keymap_dirs[command & 0x7F];

	*dp = dir;

	if (!dir) return(FALSE);
	return(TRUE);
}


/*
 * Display a string on the screen using an attribute.
 *
 * At the given location, using the given attribute, if allowed,
 * add the given string.  Do not clear the line.
 */
void c_put_str(byte attr, cptr str, int row, int col) {
	/* Position cursor, Dump the attr/text */
	Term_putstr(col, row, -1, attr, (char*)str);
}
/* Version of c_put_str() specifically for Chh sheet in horizontal mode. */
//#define VERT_ALT_Y /* alternate y a bit for readability? */
//#define VERT_ADD_COLON /* add a colon ':' at the end? */
void c_put_str_vert(byte attr, cptr str, int row, int col) {
#if 0 /* Print it normally, from left to right, and with vertical cascading */
	Term_putstr(col, row, -1, attr, (char*)str);
#else /* Print it vertically! */
	row = 0;
	while (str[row]) {
 #ifdef VERT_ALT_Y
  #ifdef VERT_ADD_COLON
		Term_putch(col, 0 + col % 2 + row, attr, str[row]);
  #else
		Term_putch(col, 1 + col % 2 + row, attr, str[row]);
  #endif
 #else
  #ifdef VERT_ADD_COLON
		Term_putch(col, 1 + row, attr, str[row]);
  #else
		Term_putch(col, 2 + row, attr, str[row]);
  #endif
 #endif
		row++;
	}
 #ifdef VERT_ADD_COLON
	Term_putch(col, 1 + row, attr, ':');
 #endif
#endif
}


/*
 * As above, but in "white"
 */
void put_str(cptr str, int row, int col) {
	/* Spawn */
	Term_putstr(col, row, -1, TERM_WHITE, (char*)str);
}

/*
 * Verify something with the user
 *
 * The "prompt" should take the form "Query? "
 *
 * Pressing ESC will accept the default_yes value.
 * Note that "[y/n]" is appended to the prompt.
 */
bool get_check2(cptr prompt, bool default_yes) {
	int i;
	char buf[80];

	/* Hack -- Build a "useful" prompt */
	if (default_yes) strnfmt(buf, 78, "%.70s [Y/n]", prompt);
	else strnfmt(buf, 78, "%.70s [y/N]", prompt);

	/* The top line is "icky" */
	topline_icky = TRUE;

	/* Prompt for it */
	prompt_topline(buf);

#if 0
	/* Get an acceptable answer */
	while (TRUE) {
		i = inkey();
		if (c_cfg.quick_messages) break;
		if (i == ESCAPE) break;
		if (strchr("YyNn", i)) break;
		bell();
	}
#else
	/* c_cfg.quick_messages now always on */
	i = inkey();
#endif

	/* Erase the prompt */
	clear_topline();

	/* The top line is OK again */
	topline_icky = FALSE;

	/* Flush any events that came in while we were icky */
	if (!c_quit) Flush_queue();

	/* More normal */
	if (default_yes) {
		if (i == 'n' || i == 'N') return(FALSE);
		return(TRUE);
	}

	if ((i == 'Y') || (i == 'y')) return(TRUE);
	return(FALSE);
}
/* default_choice:
   0 = no preference, only accept y/Y/n/N for input
   1 = default is yes, any key besides n/N will count as yes
   2 = default is yes, any key besides y/Y will count as no
   Pressing ESC only works if default_choice isn't 0, and will accept the default_choice then. */
bool get_check3(cptr prompt, char default_choice) {
	int i;
	char buf[80];

	/* Hack -- Build a "useful" prompt */
	if (default_choice == 1) strnfmt(buf, 78, "%.70s [Y/n]", prompt);
	else if (default_choice == 2) strnfmt(buf, 78, "%.70s [y/N]", prompt);
	else strnfmt(buf, 78, "%.70s [y/n]", prompt);

	/* The top line is "icky" */
	topline_icky = TRUE;

	/* Prompt for it */
	prompt_topline(buf);

	/* Get an acceptable answer */
	while (TRUE) {
		i = inkey();
		//if (c_cfg.quick_messages) break; //always on now
		if (!default_choice && !strchr("YyNn", i)) continue;
		break;
	}

	/* Erase the prompt */
	clear_topline();

	/* The top line is OK again */
	topline_icky = FALSE;

	/* Flush any events that came in while we were icky */
	if (!c_quit) Flush_queue();

	/* More normal */
	if (default_choice == 1) {
		if (i == 'n' || i == 'N') return(FALSE);
		return(TRUE);
	}

	if ((i == 'Y') || (i == 'y')) return(TRUE);
	return(FALSE);
}

byte get_3way(cptr prompt, bool default_no) {
	int i, res = -1;
	char buf[80];

	/* Hack -- Build a "useful" prompt */
	if (default_no) strnfmt(buf, 78, "%.70s [y/a/N]", prompt);
	else strnfmt(buf, 78, "%.70s [y/a/n]", prompt);

	/* The top line is "icky" */
	topline_icky = TRUE;

	/* Prompt for it */
	prompt_topline(buf);

	while (TRUE) {
		/* c_cfg.quick_messages now always on */
		i = inkey();

		if (i == 'Y' || i == 'y') res = 1;
		else if (i == 'A' || i == 'a') res = 2;
		else if (i == 'N' || i == 'n' ||
		    /* added CTRL+Q for quickly exit in-game by double-tapping CTRL+Q,
		       and for RETRY_LOGIN to quickly exit the whole game by triple-tapping CTRL+Q. */
		    (default_no && (i == '\r' || i == '\n' || i == '\e' || i == KTRL('Q')))) /* not all keys, just ESC and RETURN */
			res = 0;
		if (res != -1) break;
	}

	/* Flush any events that came in while we were icky */
	if (!c_quit) Flush_queue();

	/* Erase the prompt */
	clear_topline();

	/* The top line is OK again */
	topline_icky = FALSE;

	return(res);
}



/* Kurzel reported that on Windows 10/11, printf() output is not shown in the terminal for unknown reason. So we need a log file, alternatively, as workaround: */
void logprint(const char *out) {
	static FILE *fp = NULL;

	/* Atomic append, in case things go really wrong (paranoia) */
	if (!fp) fp = fopen("tomenet-stdout.log", "w");
	else fp = fopen("tomenet-stdout.log", "a");

	if (fp) {
		fprintf(fp, "%s", out);
		fclose(fp);
	}

	printf("%s", out);
}



/*
 * Recall the "text" of a saved message
 */
cptr message_str(s32b age) {
	s32b x;
	s32b o;
	cptr s;

	/* Forgotten messages have no text */
	if ((age < 0) || (age >= message_num())) return("");

	/* Acquire the "logical" index */
	x = (message__next + MESSAGE_MAX - (age + 1)) % MESSAGE_MAX;

	/* Get the "offset" for the message */
	o = message__ptr[x];

	/* Access the message text */
	s = &message__buf[o];

	/* Return the message text */
	return(s);
}
cptr message_str_chat(s32b age) {
	s32b x;
	s32b o;
	cptr s;

	/* Forgotten messages have no text */
	if ((age < 0) || (age >= message_num_chat())) return("");

	/* Acquire the "logical" index */
	x = (message__next_chat + MESSAGE_MAX - (age + 1)) % MESSAGE_MAX;

	/* Get the "offset" for the message */
	o = message__ptr_chat[x];

	/* Access the message text */
	s = &message__buf_chat[o];

	/* Return the message text */
	return(s);
}
cptr message_str_msgnochat(s32b age) {
	s32b x;
	s32b o;
	cptr s;

	/* Forgotten messages have no text */
	if ((age < 0) || (age >= message_num_msgnochat())) return("");

	/* Acquire the "logical" index */
	x = (message__next_msgnochat + MESSAGE_MAX - (age + 1)) % MESSAGE_MAX;

	/* Get the "offset" for the message */
	o = message__ptr_msgnochat[x];

	/* Access the message text */
	s = &message__buf_msgnochat[o];

	/* Return the message text */
	return(s);
}
cptr message_str_impscroll(s32b age) {
	s32b x;
	s32b o;
	cptr s;

	/* Forgotten messages have no text */
	if ((age < 0) || (age >= message_num_impscroll())) return("");

	/* Acquire the "logical" index */
	x = (message__next_impscroll + MESSAGE_MAX - (age + 1)) % MESSAGE_MAX;

	/* Get the "offset" for the message */
	o = message__ptr_impscroll[x];

	/* Access the message text */
	s = &message__buf_impscroll[o];

	/* Return the message text */
	return(s);
}


/*
 * How many messages are "available"?
 */
s32b message_num(void) {
	int last, next, n;

#ifdef RETRY_LOGIN
	/* Don't prompt to 'save chat messages?' when quitting the game from within the character screen,
	   if we have previousl been in-game and therefore messages have had been added. */
	if (!in_game) return(0);
#endif

	/* Extract the indexes */
	last = message__last;
	next = message__next;

	/* Handle "wrap" */
	if (next < last) next += MESSAGE_MAX;

	/* Extract the space */
	n = (next - last);

	/* Return the result */
	return(n);
}
s32b message_num_chat(void) {
	int last, next, n;

	/* Extract the indexes */
	last = message__last_chat;
	next = message__next_chat;

	/* Handle "wrap" */
	if (next < last) next += MESSAGE_MAX;

	/* Extract the space */
	n = (next - last);

	/* Return the result */
	return(n);
}
s32b message_num_msgnochat(void) {
	int last, next, n;

	/* Extract the indexes */
	last = message__last_msgnochat;
	next = message__next_msgnochat;

	/* Handle "wrap" */
	if (next < last) next += MESSAGE_MAX;

	/* Extract the space */
	n = (next - last);

	/* Return the result */
	return(n);
}
s32b message_num_impscroll(void) {
	int last, next, n;

	/* Extract the indexes */
	last = message__last_impscroll;
	next = message__next_impscroll;

	/* Handle "wrap" */
	if (next < last) next += MESSAGE_MAX;

	/* Extract the space */
	n = (next - last);

	/* Return the result */
	return(n);
}


/*
 * Add a new message, with great efficiency
 */
void c_message_add(cptr str) {
	int i, k, x, n;


	/*** Step 1 -- Analyze the message ***/

	/* Hack -- Ignore "non-messages" */
	if (!str) return;

	/* Message length */
	n = strlen(str);

	/* Important Hack -- Ignore "long" messages */
	if (n >= MESSAGE_BUF / 4) return;


	/*** Step 2 -- Attempt to optimize ***/

	/* Limit number of messages to check */
	k = message_num() / 4;

	/* Limit number of messages to check */
	if (k > MESSAGE_MAX / 32) k = MESSAGE_MAX / 32;

	/* Check the last few messages (if any to count) */
	for (i = message__next; k; k--) {
		u32b q;
		cptr old;

		/* Back up and wrap if needed */
		if (i-- == 0) i = MESSAGE_MAX - 1;

		/* Stop before oldest message */
		if (i == message__last) break;

		/* Extract "distance" from "head" */
		q = (message__head + MESSAGE_BUF - message__ptr[i]) % MESSAGE_BUF;

		/* Do not optimize over large distance */
		if (q > MESSAGE_BUF / 2) continue;

		/* Access the old string */
		old = &message__buf[message__ptr[i]];

		/* Compare */
		if (!streq(old, str)) continue;

		/* Get the next message index, advance */
		x = message__next++;

		/* Handle wrap */
		if (message__next == MESSAGE_MAX) message__next = 0;

		/* Kill last message if needed */
		if (message__next == message__last) message__last++;

		/* Handle wrap */
		if (message__last == MESSAGE_MAX) message__last = 0;

		/* Assign the starting address */
		message__ptr[x] = message__ptr[i];

		/* Redraw - assume that all chat messages start with '[' */
#if 0
		if (str[0] == '[')
			p_ptr->window |= (PW_MESSAGE | PW_CHAT);
		else
			p_ptr->window |= (PW_MESSAGE | PW_MSGNOCHAT);
#endif
		p_ptr->window |= (PW_MESSAGE);

		/* Success */
		return;
	}


	/*** Step 3 -- Ensure space before end of buffer ***/

	/* Kill messages and Wrap if needed */
	if (message__head + n + 1 >= MESSAGE_BUF) {
		/* Kill all "dead" messages */
		for (i = message__last; TRUE; i++) {
			/* Wrap if needed */
			if (i == MESSAGE_MAX) i = 0;

			/* Stop before the new message */
			if (i == message__next) break;

			/* Kill "dead" messages */
			if (message__ptr[i] >= message__head)
			{
				/* Track oldest message */
				message__last = i + 1;
			}
		}

		/* Wrap "tail" if needed */
		if (message__tail >= message__head) message__tail = 0;

		/* Start over */
		message__head = 0;
	}


	/*** Step 4 -- Ensure space before next message ***/

	/* Kill messages if needed */
	if (message__head + n + 1 > message__tail) {
		/* Grab new "tail" */
		message__tail = message__head + n + 1;

		/* Advance tail while possible past first "nul" */
		while (message__buf[message__tail - 1]) message__tail++;

		/* Kill all "dead" messages */
		for (i = message__last; TRUE; i++) {
			/* Wrap if needed */
			if (i == MESSAGE_MAX) i = 0;

			/* Stop before the new message */
			if (i == message__next) break;

			/* Kill "dead" messages */
			if ((message__ptr[i] >= message__head) &&
			    (message__ptr[i] < message__tail))
			{
				/* Track oldest message */
				message__last = i + 1;
			}
		}
	}


	/*** Step 5 -- Grab a new message index ***/

	/* Get the next message index, advance */
	x = message__next++;

	/* Handle wrap */
	if (message__next == MESSAGE_MAX) message__next = 0;

	/* Kill last message if needed */
	if (message__next == message__last) message__last++;

	/* Handle wrap */
	if (message__last == MESSAGE_MAX) message__last = 0;



	/*** Step 6 -- Insert the message text ***/

	/* Assign the starting address */
	message__ptr[x] = message__head;

	/* Append the new part of the message */
	for (i = 0; i < n; i++)
		/* Copy the message */
		message__buf[message__head + i] = str[i];

	/* Terminate */
	message__buf[message__head + i] = '\0';

	/* Advance the "head" pointer */
	message__head += n + 1;

#if 0
//deprecated?:
	/* Window stuff - assume that all chat messages start with '[' */
	if (str[0] == '[')
		p_ptr->window |= (PW_MESSAGE | PW_CHAT);
	else
		p_ptr->window |= (PW_MESSAGE | PW_MSGNOCHAT);
//      	p_ptr->window |= PW_MESSAGE;
#else
	p_ptr->window |= PW_MESSAGE;
#endif
}
void c_message_add_chat(cptr str) {
	int i, k, x, n;


	/*** Step 1 -- Analyze the message ***/

	/* Hack -- Ignore "non-messages" */
	if (!str) return;

	/* Message length */
	n = strlen(str);

	/* Important Hack -- Ignore "long" messages */
	if (n >= MESSAGE_BUF / 4) return;


	/*** Step 2 -- Attempt to optimize ***/

	/* Limit number of messages to check */
	k = message_num_chat() / 4;

	/* Limit number of messages to check */
	if (k > MESSAGE_MAX / 32) k = MESSAGE_MAX / 32;

	/* Check the last few messages (if any to count) */
	for (i = message__next_chat; k; k--) {
		u32b q;
		cptr old;

		/* Back up and wrap if needed */
		if (i-- == 0) i = MESSAGE_MAX - 1;

		/* Stop before oldest message */
		if (i == message__last_chat) break;

		/* Extract "distance" from "head" */
		q = (message__head_chat + MESSAGE_BUF - message__ptr_chat[i]) % MESSAGE_BUF;

		/* Do not optimize over large distance */
		if (q > MESSAGE_BUF / 2) continue;

		/* Access the old string */
		old = &message__buf_chat[message__ptr_chat[i]];

		/* Compare */
		if (!streq(old, str)) continue;

		/* Get the next message index, advance */
		x = message__next_chat++;

		/* Handle wrap */
		if (message__next_chat == MESSAGE_MAX) message__next_chat = 0;

		/* Kill last message if needed */
		if (message__next_chat == message__last_chat) message__last_chat++;

		/* Handle wrap */
		if (message__last_chat == MESSAGE_MAX) message__last_chat = 0;

		/* Assign the starting address */
		message__ptr_chat[x] = message__ptr_chat[i];

		/* Redraw - assume that all chat messages start with '[' */
		p_ptr->window |= (PW_CHAT);

		/* Success */
		return;
	}


	/*** Step 3 -- Ensure space before end of buffer ***/

	/* Kill messages and Wrap if needed */
	if (message__head_chat + n + 1 >= MESSAGE_BUF) {
		/* Kill all "dead" messages */
		for (i = message__last_chat; TRUE; i++) {
			/* Wrap if needed */
			if (i == MESSAGE_MAX) i = 0;

			/* Stop before the new message */
			if (i == message__next_chat) break;

			/* Kill "dead" messages */
			if (message__ptr_chat[i] >= message__head_chat)
				/* Track oldest message */
				message__last_chat = i + 1;
		}

		/* Wrap "tail" if needed */
		if (message__tail_chat >= message__head_chat) message__tail_chat = 0;

		/* Start over */
		message__head_chat = 0;
	}


	/*** Step 4 -- Ensure space before next message ***/

	/* Kill messages if needed */
	if (message__head_chat + n + 1 > message__tail_chat) {
		/* Grab new "tail" */
		message__tail_chat = message__head_chat + n + 1;

		/* Advance tail while possible past first "nul" */
		while (message__buf_chat[message__tail_chat - 1]) message__tail_chat++;

		/* Kill all "dead" messages */
		for (i = message__last_chat; TRUE; i++) {
			/* Wrap if needed */
			if (i == MESSAGE_MAX) i = 0;

			/* Stop before the new message */
			if (i == message__next_chat) break;

			/* Kill "dead" messages */
			if ((message__ptr_chat[i] >= message__head_chat) &&
			    (message__ptr_chat[i] < message__tail_chat)) {
				/* Track oldest message */
				message__last_chat = i + 1;
			}
		}
	}


	/*** Step 5 -- Grab a new message index ***/

	/* Get the next message index, advance */
	x = message__next_chat++;

	/* Handle wrap */
	if (message__next_chat == MESSAGE_MAX) message__next_chat = 0;

	/* Kill last message if needed */
	if (message__next_chat == message__last_chat) message__last_chat++;

	/* Handle wrap */
	if (message__last_chat == MESSAGE_MAX) message__last_chat = 0;



	/*** Step 6 -- Insert the message text ***/

	/* Assign the starting address */
	message__ptr_chat[x] = message__head_chat;

	/* Append the new part of the message */
	for (i = 0; i < n; i++)
		/* Copy the message */
		message__buf_chat[message__head_chat + i] = str[i];

	/* Terminate */
	message__buf_chat[message__head_chat + i] = '\0';

	/* Advance the "head" pointer */
	message__head_chat += n + 1;

	/* Window stuff - assume that all chat messages start with '[' */
	p_ptr->window |= (PW_CHAT);

}
void c_message_add_msgnochat(cptr str) {
	int i, k, x, n;


	/*** Step 1 -- Analyze the message ***/

	/* Hack -- Ignore "non-messages" */
	if (!str) return;

	/* Message length */
	n = strlen(str);

	/* Important Hack -- Ignore "long" messages */
	if (n >= MESSAGE_BUF / 4) return;


	/*** Step 2 -- Attempt to optimize ***/

	/* Limit number of messages to check */
	k = message_num_msgnochat() / 4;

	/* Limit number of messages to check */
	if (k > MESSAGE_MAX / 32) k = MESSAGE_MAX / 32;

	/* Check the last few _msgnochatmessages (if any to count) */
	for (i = message__next_msgnochat; k; k--) {
		u32b q;
		cptr old;

		/* Back up and wrap if needed */
		if (i-- == 0) i = MESSAGE_MAX - 1;

		/* Stop before oldest message */
		if (i == message__last_msgnochat) break;

		/* Extract "distance" from "head" */
		q = (message__head_msgnochat + MESSAGE_BUF - message__ptr_msgnochat[i]) % MESSAGE_BUF;

		/* Do not optimize over large distance */
		if (q > MESSAGE_BUF / 2) continue;

		/* Access the old string */
		old = &message__buf_msgnochat[message__ptr_msgnochat[i]];

		/* Compare */
		if (!streq(old, str)) continue;

		/* Get the next message index, advance */
		x = message__next_msgnochat++;

		/* Handle wrap */
		if (message__next_msgnochat == MESSAGE_MAX) message__next_msgnochat = 0;

		/* Kill last message if needed */
		if (message__next_msgnochat == message__last_msgnochat) message__last_msgnochat++;

		/* Handle wrap */
		if (message__last_msgnochat == MESSAGE_MAX) message__last_msgnochat = 0;

		/* Assign the starting address */
		message__ptr_msgnochat[x] = message__ptr_msgnochat[i];

		/* Redraw - assume that all chat messages start with '[' */
		p_ptr->window |= (PW_MSGNOCHAT);

		/* Success */
		return;
	}


	/*** Step 3 -- Ensure space before end of buffer ***/

	/* Kill messages and Wrap if needed */
	if (message__head_msgnochat + n + 1 >= MESSAGE_BUF) {
		/* Kill all "dead" messages */
		for (i = message__last_msgnochat; TRUE; i++) {
			/* Wrap if needed */
			if (i == MESSAGE_MAX) i = 0;

			/* Stop before the new message */
			if (i == message__next_msgnochat) break;

			/* Kill "dead" messages */
			if (message__ptr_msgnochat[i] >= message__head_msgnochat)
			{
				/* Track oldest message */
				message__last_msgnochat = i + 1;
			}
		}

		/* Wrap "tail" if needed */
		if (message__tail_msgnochat >= message__head_msgnochat) message__tail_msgnochat = 0;

		/* Start over */
		message__head_msgnochat = 0;
	}


	/*** Step 4 -- Ensure space before next message ***/

	/* Kill messages if needed */
	if (message__head_msgnochat + n + 1 > message__tail_msgnochat) {
		/* Grab new "tail" */
		message__tail_msgnochat = message__head_msgnochat + n + 1;

		/* Advance tail while possible past first "nul" */
		while (message__buf_msgnochat[message__tail_msgnochat - 1]) message__tail_msgnochat++;

		/* Kill all "dead" messages */
		for (i = message__last_msgnochat; TRUE; i++) {
			/* Wrap if needed */
			if (i == MESSAGE_MAX) i = 0;

			/* Stop before the new message */
			if (i == message__next_msgnochat) break;

			/* Kill "dead" messages */
			if ((message__ptr_msgnochat[i] >= message__head_msgnochat) &&
			    (message__ptr_msgnochat[i] < message__tail_msgnochat)) {
				/* Track oldest message */
				message__last_msgnochat = i + 1;
			}
		}
	}


	/*** Step 5 -- Grab a new message index ***/

	/* Get the next message index, advance */
	x = message__next_msgnochat++;

	/* Handle wrap */
	if (message__next_msgnochat == MESSAGE_MAX) message__next_msgnochat = 0;

	/* Kill last message if needed */
	if (message__next_msgnochat == message__last_msgnochat) message__last_msgnochat++;

	/* Handle wrap */
	if (message__last_msgnochat == MESSAGE_MAX) message__last_msgnochat = 0;



	/*** Step 6 -- Insert the message text ***/

	/* Assign the starting address */
	message__ptr_msgnochat[x] = message__head_msgnochat;

	/* Append the new part of the message */
	for (i = 0; i < n; i++)
		/* Copy the message */
		message__buf_msgnochat[message__head_msgnochat + i] = str[i];

	/* Terminate */
	message__buf_msgnochat[message__head_msgnochat + i] = '\0';

	/* Advance the "head" pointer */
	message__head_msgnochat += n + 1;

	/* Window stuff - assume that all chat messages start with '[' */
	p_ptr->window |= (PW_MSGNOCHAT);
}
void c_message_add_impscroll(cptr str) {
	int i, k, x, n;


	/*** Step 1 -- Analyze the message ***/

	/* Hack -- Ignore "non-messages" */
	if (!str) return;

	/* Message length */
	n = strlen(str);

	/* Important Hack -- Ignore "long" messages */
	if (n >= MESSAGE_BUF / 4) return;


	/*** Step 2 -- Attempt to optimize ***/

	/* Limit number of messages to check */
	k = message_num_impscroll() / 4;

	/* Limit number of messages to check */
	if (k > MESSAGE_MAX / 32) k = MESSAGE_MAX / 32;

	/* Check the last few messages (if any to count) */
	for (i = message__next_impscroll; k; k--) {
		u32b q;
		cptr old;

		/* Back up and wrap if needed */
		if (i-- == 0) i = MESSAGE_MAX - 1;

		/* Stop before oldest message */
		if (i == message__last_impscroll) break;

		/* Extract "distance" from "head" */
		q = (message__head_impscroll + MESSAGE_BUF - message__ptr_impscroll[i]) % MESSAGE_BUF;

		/* Do not optimize over large distance */
		if (q > MESSAGE_BUF / 2) continue;

		/* Access the old string */
		old = &message__buf_impscroll[message__ptr_impscroll[i]];

		/* Compare */
		if (!streq(old, str)) continue;

		/* Get the next message index, advance */
		x = message__next_impscroll++;

		/* Handle wrap */
		if (message__next_impscroll == MESSAGE_MAX) message__next_impscroll = 0;

		/* Kill last message if needed */
		if (message__next_impscroll == message__last_impscroll) message__last_impscroll++;

		/* Handle wrap */
		if (message__last_impscroll == MESSAGE_MAX) message__last_impscroll = 0;

		/* Assign the starting address */
		message__ptr_impscroll[x] = message__ptr_impscroll[i];

		/* Redraw - assume that all chat messages start with '[' */
#if 0
		if (str[0] == '[')
			p_ptr->window |= (PW_MESSAGE | PW_CHAT);
		else
			p_ptr->window |= (PW_MESSAGE | PW_MSGNOCHAT);
#endif
		p_ptr->window |= (PW_MESSAGE);

		/* Success */
		return;
	}


	/*** Step 3 -- Ensure space before end of buffer ***/

	/* Kill messages and Wrap if needed */
	if (message__head_impscroll + n + 1 >= MESSAGE_BUF) {
		/* Kill all "dead" messages */
		for (i = message__last_impscroll; TRUE; i++) {
			/* Wrap if needed */
			if (i == MESSAGE_MAX) i = 0;

			/* Stop before the new message */
			if (i == message__next_impscroll) break;

			/* Kill "dead" messages */
			if (message__ptr_impscroll[i] >= message__head_impscroll)
			{
				/* Track oldest message */
				message__last_impscroll = i + 1;
			}
		}

		/* Wrap "tail" if needed */
		if (message__tail_impscroll >= message__head_impscroll) message__tail_impscroll = 0;

		/* Start over */
		message__head_impscroll = 0;
	}


	/*** Step 4 -- Ensure space before next message ***/

	/* Kill messages if needed */
	if (message__head_impscroll + n + 1 > message__tail_impscroll) {
		/* Grab new "tail" */
		message__tail_impscroll = message__head_impscroll + n + 1;

		/* Advance tail while possible past first "nul" */
		while (message__buf_impscroll[message__tail_impscroll - 1]) message__tail_impscroll++;

		/* Kill all "dead" messages */
		for (i = message__last_impscroll; TRUE; i++) {
			/* Wrap if needed */
			if (i == MESSAGE_MAX) i = 0;

			/* Stop before the new message */
			if (i == message__next_impscroll) break;

			/* Kill "dead" messages */
			if ((message__ptr_impscroll[i] >= message__head_impscroll) &&
			    (message__ptr_impscroll[i] < message__tail_impscroll)) {
				/* Track oldest message */
				message__last_impscroll = i + 1;
			}
		}
	}


	/*** Step 5 -- Grab a new message index ***/

	/* Get the next message index, advance */
	x = message__next_impscroll++;

	/* Handle wrap */
	if (message__next_impscroll == MESSAGE_MAX) message__next_impscroll = 0;

	/* Kill last message if needed */
	if (message__next_impscroll == message__last_impscroll) message__last_impscroll++;

	/* Handle wrap */
	if (message__last_impscroll == MESSAGE_MAX) message__last_impscroll = 0;



	/*** Step 6 -- Insert the message text ***/

	/* Assign the starting address */
	message__ptr_impscroll[x] = message__head_impscroll;

	/* Append the new part of the message */
	for (i = 0; i < n; i++)
		/* Copy the message */
		message__buf_impscroll[message__head_impscroll + i] = str[i];

	/* Terminate */
	message__buf_impscroll[message__head_impscroll + i] = '\0';

	/* Advance the "head" pointer */
	message__head_impscroll += n + 1;

#if 0
//deprecated?:
	/* Window stuff - assume that all chat messages start with '[' */
	if (str[0] == '[')
		p_ptr->window |= (PW_MESSAGE | PW_CHAT);
	else
		p_ptr->window |= (PW_MESSAGE | PW_MSGNOCHAT);
//		p_ptr->window |= PW_MESSAGE;
#else
	p_ptr->window |= PW_MESSAGE;
#endif
}

/*
 * Output a message to the top line of the screen.
 *
 * Break long messages into multiple pieces (40-72 chars).
 *
 * Allow multiple short messages to "share" the top line.
 *
 * Prompt the user to make sure he has a chance to read them.
 *
 * These messages are memorized for later reference (see above).
 *
 * We could do "Term_fresh()" to provide "flicker" if needed.
 *
 * The global "msg_flag" variable can be cleared to tell us to
 * "erase" any "pending" messages still on the screen.
 *
 * XXX XXX XXX Note that we must be very careful about using the
 * "msg_print()" functions without explicitly calling the special
 * "msg_print(NULL)" function, since this may result in the loss
 * of information if the screen is cleared, or if anything is
 * displayed on the top line.
 *
 * XXX XXX XXX Note that "msg_print(NULL)" will clear the top line
 * even if no messages are pending.  This is probably a hack.
 */
void c_msg_print(cptr msg) {
	int n, x, y;
	char buf[1024];
	char *t;
	bool first_line = TRUE;

	if (msg && msg[0] == '\377' && msg[1] == '\377') {
		msg += 2;
		if (c_cfg.topline_first) first_line = FALSE;
	}

	if (!c_cfg.topline_no_msg && first_line)
		/* using clear_topline() here prevents top-line clearing via c_msg_print(NULL) */
		if (!topline_icky) clear_topline();

	/* No message */
	if (!msg) {
		/* An empty msg was and is a hack to just clear the topline, done above.
		   However, now it has a second purpose (4.7.3): On relog, cause the client to call fix_message() once per relog,
		   to restore all previous messages in the message windows across logins.
		   This is important in case the server motd (admin-notes) aren't sent and no other message (party/guild/player notes) is sent either,
		   in which case the message windows would all stay blank until the client receives the first message. */
		p_ptr->window |= PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT;
		return;
	}

	/* Copy it */
	strcpy(buf, msg);
	/* mark beginning of text to output */
	t = buf;

	/* Remember cursor position */
	Term_locate(&x, &y);

	/* Message length */
	n = strlen(msg);

	/* Paranoia */
	if (n > 1000) return;

	/* Memorize the message */
	if (msg[0] == '\375') {
		was_chat_buffer = TRUE;
		was_all_buffer = FALSE;
	} else if (msg[0] == '\374') {
		was_all_buffer = TRUE;
		was_chat_buffer = FALSE;
	} else {
		was_chat_buffer = FALSE;
		was_all_buffer = FALSE;
	}

	/* strip (from here on) effectless control codes */
	if (*t == '\374') t++;
	if (*t == '\375') t++;

	/* add the message to generic, chat-only and no-chat buffers
	   accordingly, KEEPING '\376' control code (v>4.4.2.4) for main buffer */
	c_message_add(t);

	/* strip remaining control codes before displaying on screen */
	if (*t == '\376') {
		t++;
		was_important_scrollback = TRUE;
	} else was_important_scrollback = FALSE;

	if (was_chat_buffer || was_all_buffer)
		c_message_add_chat(t);
	if (!was_chat_buffer)
		c_message_add_msgnochat(t);
	if (was_chat_buffer || was_all_buffer || was_important_scrollback)
		c_message_add_impscroll(t);

	/* Extra stuff: Dump messages to stdout? */
	if (c_cfg.clone_to_stdout) {
		char buf2[MSG_LEN], *c = t, *c2 = buf2;

		while (*c) {
			switch (*c) {
			/* strip colour codes */
			case '\377':
				switch (*(c + 1)) {
				case 0: /* broken colour code (paranoia) */
					c++;
					continue;
				default: /* assume colour code and discard */
					c += 2;
					continue;
				}
				break;
			}
			*c2 = *c;
			c++;
			c2++;
		}
		*c2 = 0;
		printf("%s\n", buf2);
		fflush(stdout);
		/* Instead of flushing everytime we could also set stdout to unbuffered with either of these:
		setbuf(stdout, NULL);
		setvbuf(stdout, (char*)NULL, _IONBF, 0); //more flexible than setbuf()
		*/
	}
	/* Extra stuff: Dump messages to file? */
	if (c_cfg.clone_to_file) {
		char buf2[MSG_LEN], *c = t, *c2 = buf2;
		FILE *fp;
		char path[1024];

		/* Build the filename */
		path_build(path, 1024, ANGBAND_DIR_USER, "stdout.txt");

		fp = my_fopen(path, "a");
		/* success */
		if (fp) {
			while (*c) {
				switch (*c) {
				/* strip colour codes */
				case '\377':
					switch (*(c + 1)) {
					case 0: /* broken colour code (paranoia) */
						c++;
						continue;
					default: /* assume colour code and discard */
						c += 2;
						continue;
					}
					break;
				}
				*c2 = *c;
				c++;
				c2++;
			}
			*c2 = 0;
			fprintf(fp, "%s\n", buf2);
			my_fclose(fp);
		}
	}

	/* Don't display any messages in top line? */
	if (c_cfg.topline_no_msg || topline_icky || !first_line) return;

	/* Small length limit */
	if (n > 80) n = 80;

	/* Display the tail of the message to the topline */
	clear_topline_forced(); //extra: make sure it's completely erased so we don't overwrite stuff into each other
	Term_putstr(0, 0, n, TERM_WHITE, t);

	/* Restore cursor */
	Term_gotoxy(x, y);

	/* Remember the message */
	msg_flag = TRUE;
}

/*
 * Display a formatted message, using "vstrnfmt()" and "c_msg_print()".
 */
void c_msg_format(cptr fmt, ...) {
	va_list vp;
	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Format the args, save the length */
	(void)vstrnfmt(buf, 1024, fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Display */
	c_msg_print(buf);
}


/*
 * Request a "quantity" from the user
 * 'max' -1 means 'no maximum'.
 * Added max and predef for handling item stacks in one's inventory -- C. Blue
 */
#define QUANTITY_WIDTH 10
s32b c_get_quantity(cptr prompt, s32b predef, s32b max) {
	s32b amt;
	char tmp[80];
	char buf[80];

	char bi1[80], bi2[9 + 1];
	int n = 0, i = 0, j = 0;
	s32b i1 = 0, i2 = 0, mul = 1;

	/* Build the default */
	sprintf(buf, "%d", predef);

	/* Build a prompt if needed */
	if (!prompt) {
		/* Build a prompt */
		inkey_letter_all = TRUE;

		if (max != -1) sprintf(tmp, "Quantity (1-%d, ", max);
		else sprintf(tmp, "Quantity (");

		if (predef < max) strcat(tmp, format("ENTER = %d, 'a'/SPACE = all): ", predef));
		else strcat(tmp, "ENTER/'a'/SPACE = all): ");

		/* Use that prompt */
		prompt = tmp;
	}

	/* Ask for a quantity */
	if (!get_string(prompt, buf, QUANTITY_WIDTH)) return(0);

	if (!buf[0]) { /* ENTER = default */
		if (predef >= 0) amt = predef;
		/* hack for dropping gold (max is -1) */
		else amt = PY_MAX_GOLD;
	} else if (buf[0] == 'a' /* 'a' means "all" - C. Blue */
	    /* allow old 'type in any letter for "all"' hack again too?: */
	    || (isalpha(buf[0])
	    /* exempt the E factors */
	    && ((buf[0] != 'k' && buf[0] != 'K' && buf[0] != 'm' && buf[0] != 'M' && buf[0] != 'g' && buf[0] != 'G')
	    /* ..except if after a factor there is no actual number following, then we allow this to be 'all' */
	    || !buf[1] || (buf[1] < '0' || buf[1] > '9')))) {
		if (max >= 0) amt = max;
		/* hack for dropping gold (max is -1) */
		else amt = PY_MAX_GOLD;
	} else {
		/* special hack to enable old 'any letter = all' hack without interfering with 'k'/'M'/'G'/ for kilo/mega/giga: */
		if ((buf[0] == 'k' || buf[0] == 'K' || buf[0] == 'm' || buf[0] == 'M' || buf[0] == 'g' || buf[0] == 'G') && buf[1]
		    && (buf[1] < '0' || buf[1] > '9')) {
			//all..
			buf[0] = 'a';
			buf[1] = 0;
		}
		/* new method slightly extended: allow leading 'k' or 'M' or 'G' too */
		else if ((buf[0] == 'k' || buf[0] == 'K' || buf[0] == 'm' || buf[0] == 'M' || buf[0] == 'g' || buf[0] == 'G') && buf[1]) {
			/* add leading '0' to revert it to the usual format */
			for (i = QUANTITY_WIDTH + 1; i >= 1; i--) buf[i] = buf[i - 1];
			buf[0] = '0';
		}

		/* new method for inputting amounts of gold:  1m35 = 1,350,000  - C. Blue */
		while (buf[n] >= '0' && buf[n] <= '9') bi1[i++] = buf[n++];
		bi1[i] = '\0';
		i1 = atoi(bi1);
		if ((buf[n] == 'k' || buf[n] == 'K') && n > 0) mul = 1000;
		else if (buf[n] == 'm' || buf[n] == 'M') mul = 1000000;
		else if (buf[n] == 'g' || buf[n] == 'G') mul = 1000000000;
		if (mul > 1) {
			n++;
			i = 0;
			while (buf[n] >= '0' && buf[n] <= '9' && i < 6) bi2[i++] = buf[n++];
			bi2[i] = '\0';

			i = 0;
			while (i < 9) {
				if (bi2[i] == '\0') {
					j = i;
					while (j < 9) bi2[j++] = '0';
					break;
				}
				i++;
			}

			if (mul == 1000) bi2[3] = '\0';
			else if (mul == 1000000) bi2[6] = '\0';
			else if (mul == 1000000000) bi2[9] = '\0';

			i2 = atoi(bi2);
			amt = i1 * mul + i2;
		} else amt = i1;
	}

	/* Enforce the maximum, if maximum is defined */
	if (max >= 0 && amt > max) amt = max;

	/* Enforce the minimum */
	if (amt < 0) amt = 0;

	/* Return the result */
	return(amt);
}

s32b c_get_number(cptr prompt, s32b predef, s32b min, s32b max) {
	s32b amt;
	char tmp[80];
	char buf[80], *bufptr;

	char bi1[80], bi2[9 + 1];
	int n, i, j;
	s32b i1, i2, mul;
	bool negative;


redo:
	n = i = j = 0;
	i1 = i2 = 0;
	mul = 1;
	negative = FALSE;

	/* Build the default */
	sprintf(buf, "%d", predef);

	/* Build a prompt if needed */
	if (!prompt) {
		/* Build a prompt */
		inkey_letter_all = TRUE;

		if (max != -1) sprintf(tmp, "Number (%d-%d, ", min, max);
		else sprintf(tmp, "Number (");

		if (predef < max) strcat(tmp, format("ENTER = %d, 'a'/SPACE = all): ", predef));
		else strcat(tmp, "ENTER/'a'/SPACE = all): ");

		/* Use that prompt */
		prompt = tmp;
	}

	/* Ask for a quantity */
	if (!get_string(prompt, buf, QUANTITY_WIDTH)) return(0);

	if (!buf[0]) { /* ENTER = default */
		if (predef >= 0) amt = predef;
		/* hack for dropping gold (max is -1) */
		else amt = PY_MAX_GOLD;
	} else if (buf[0] == 'a' /* 'a' means "all" - C. Blue */
	    /* allow old 'type in any letter for "all"' hack again too?: */
	    || (isalpha(buf[0])
	    /* exempt the E factors */
	    && ((buf[0] != 'k' && buf[0] != 'K' && buf[0] != 'm' && buf[0] != 'M' && buf[0] != 'g' && buf[0] != 'G')
	    /* ..except if after a factor there is no actual number following, then we allow this to be 'all' */
	    || !buf[1] || (buf[1] < '0' || buf[1] > '9')))) {
		if (max >= 0) amt = max;
		/* hack for dropping gold (max is -1) */
		else amt = PY_MAX_GOLD;
	} else {
		bufptr = buf;
		if (*bufptr == '-') {
			negative = TRUE;
			bufptr++;
			n++;
		}

		/* special hack to enable old 'any letter = all' hack without interfering with 'k'/'M'/'G'/ for kilo/mega/giga: */
		if ((*bufptr == 'k' || *bufptr == 'K' || *bufptr == 'm' || *bufptr == 'M' || *bufptr == 'g' || *bufptr == 'G') && *(bufptr + 1)
		    && (*(bufptr + 1) < '0' || *(bufptr + 1) > '9')) {
			//all..
			*bufptr = 'a';
			*(bufptr + 1) = 0;
		}
		/* new method slightly extended: allow leading 'k' or 'M' or 'G' too */
		else if ((*bufptr == 'k' || *bufptr == 'K' || *bufptr == 'm' || *bufptr == 'M' || *bufptr == 'g' || *bufptr == 'G') && *(bufptr + 1)) {
			/* add leading '0' to revert it to the usual format */
			for (i = QUANTITY_WIDTH + 1; i >= 1; i--) buf[i] = buf[i - 1];
			*bufptr = '0';
		}

		/* new method for inputting amounts of gold:  1m35 = 1,350,000  - C. Blue */
		while (buf[n] >= '0' && buf[n] <= '9') bi1[i++] = buf[n++];
		bi1[i] = '\0';
		i1 = atoi(bi1);
		if ((buf[n] == 'k' || buf[n] == 'K') && n > 0) mul = 1000;
		else if (buf[n] == 'm' || buf[n] == 'M') mul = 1000000;
		else if (buf[n] == 'g' || buf[n] == 'G') mul = 1000000000;
		if (mul > 1) {
			n++;
			i = 0;
			while (buf[n] >= '0' && buf[n] <= '9' && i < 6) bi2[i++] = buf[n++];
			bi2[i] = '\0';

			i = 0;
			while (i < 9) {
				 if (bi2[i] == '\0') {
					j = i;
					while (j < 9) bi2[j++] = '0';
					break;
				 }
				 i++;
			}

			if (mul == 1000) bi2[3] = '\0';
			else if (mul == 1000000) bi2[6] = '\0';
			else if (mul == 1000000000) bi2[9] = '\0';

			i2 = atoi(bi2);
			amt = i1 * mul + i2;
		} else amt = i1;
	}

	if (negative) amt = -amt;

	/* Enforce the maximum */
	if (amt > max) goto redo;

	/* Enforce the minimum */
	if (amt < min) goto redo;

	/* Return the result */
	return(amt);
}

void clear_from(int row) {
	int y;

	/* Erase requested rows */
	for (y = row; y < Term->hgt; y++)
		/* Erase part of the screen */
		Term_erase(0, y, 255);
}

/* Added for clearing of casino screen */
void clear_from_to(int row_s, int row_e) {
	int y;

	if (row_e >= Term->hgt) row_e = Term->hgt;

	/* Erase requested rows */
	for (y = row_s; y < row_e; y++) {
		/* This is specifically required for rawpict image clearing, or the images will remain on screen */
		Term_fresh();

		/* Erase part of the screen */
		Term_erase(0, y, 255);
	}
}

void prt_num(cptr header, int num, int row, int col, byte color) {
	int len = strlen(header);
	char out_val[32];

	put_str(header, row, col);
	put_str("   ", row, col + len);
	(void)sprintf(out_val, "%6d", num);
	c_put_str(color, out_val, row, col + len + 3);
}

/* print large numbers, used in C screen for Xp values and Au */
void prt_lnum(cptr header, s32b num, int row, int col, byte color) {
	int len = strlen(header) + 1;
	char out_val[32];

	put_str(header, row, col);
	if (c_cfg.colourize_bignum && color != TERM_L_UMBER) {
		/* Drained value? */
		if (color == TERM_YELLOW) colour_bignum(num, -1, out_val, 3, TRUE);
		/* Normal value */
		else colour_bignum(num, -1, out_val, 1, TRUE);
	}
	else (void)sprintf(out_val, "%10d", (int)num); /* Increased form 9 to 10 just for gold (~2 billion limit) */
	c_put_str(color, out_val, row, col + len);
}


static void ascii_to_text(char *buf, cptr str) {
	char *s = buf;

	/* Analyze the "ascii" string */
	while (*str) {
		byte i = (byte)(*str++);

		if (i == ESCAPE) {
			*s++ = '\\';
			*s++ = 'e';
		} else if (i == MACRO_WAIT) {
			*s++ = '\\';
			*s++ = 'w';
		} else if (i == MACRO_XWAIT) {
			*s++ = '\\';
			*s++ = 'W';
		} else if (i == ' ') {
			*s++ = '\\';
			*s++ = 's';
		} else if (i == '\b') {
			*s++ = '\\';
			*s++ = 'b';
		} else if (i == '\t') {
			*s++ = '\\';
			*s++ = 't';
		} else if (i == '\n') {
			*s++ = '\\';
			*s++ = 'n';
		} else if (i == '\r') {
			*s++ = '\\';
			*s++ = 'r';
		} else if (i == '^') {
			*s++ = '\\';
			*s++ = '^';
		} else if (i == '\\') {
			*s++ = '\\';
			*s++ = '\\';
		} else if (i < 32) {
			*s++ = '^';
			*s++ = i + 64;
		} else if (i < 127)
			*s++ = i;
		else if (i < 64) {
			*s++ = '\\';
			*s++ = '0';
			*s++ = octify(i / 8);
			*s++ = octify(i % 8);
		} else {
			*s++ = '\\';
			*s++ = 'x';
			*s++ = hexify(i / 16);
			*s++ = hexify(i % 16);
		}
	}

	/* Terminate */
	*s = '\0';
}

static errr macro_dump(cptr fname) {
	int i, j, n;

	FILE *fff;

	char buf[1024];
	char buf2[4096];


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* Check if the file already exists */
	fff = my_fopen(buf, "r");

	if (fff) {
		fclose(fff);

		/* Attempt to rename */
		strcpy(buf2, buf);
		strncat(buf2, ".bak", 1023);
		rename(buf, buf2);
	}

	/* Write to the file */
	fff = my_fopen(buf, "w");

	/* Failure */
	if (!fff) return(-1);

	c_msg_format("Macros saved to file %s", fname);

	/* Skip space */
	fprintf(fff, "\n\n");

	/* Start dumping */
	fprintf(fff, "# Macro dump\n\n");

	/* Dump them */
	for (i = 0; i < macro__num; i++) {
		/* Start the macro */
		fprintf(fff, "# Macro '%d'\n\n", i);

#if 0
		/* Extract the action */
		ascii_to_text(buf, macro__act[i]);

		/* Dump the macro */
		fprintf(fff, "A:%s\n", buf);
#else
		/* Support really long macros - mikaelh */
		fprintf(fff, "A:");

		for (j = 0, n = strlen(macro__act[i]); j < n; j += 1023) {
			/* Take a piece of the action */
			strncpy(buf, &macro__act[i][j], 1023);
			buf[1023] = '\0';

			/* Convert it */
			ascii_to_text(buf2, buf);

			/* Dump it */
			fprintf(fff, "%s", buf2);
		}

		fprintf(fff, "\n");
#endif

		/* Extract the action */
		ascii_to_text(buf, macro__pat[i]);

		/* Dump command macro */
		if (macro__hyb[i]) fprintf(fff, "H:%s\n", buf);
		else if (macro__cmd[i]) fprintf(fff, "C:%s\n", buf);
		/* Dump normal macros */
		else fprintf(fff, "P:%s\n", buf);

		/* End the macro */
		fprintf(fff, "\n\n");
	}

	/* Finish dumping */
	fprintf(fff, "\n\n\n\n");

	/* Close */
	my_fclose(fff);

	/* Success */
	return(0);
}

static void get_macro_trigger(char *buf) {
	int i, n = 0;
	char tmp[1024];

	/* Flush */
	flush();

	/* Do not process macros */
	inkey_base = TRUE;

	/* First key */
	i = inkey();

	/* Read the pattern */
	while (i) {
		/* Save the key */
		buf[n++] = i;

		/* Do not process macros */
		inkey_base = TRUE;

		/* Do not wait for keys */
		inkey_scan = TRUE;

		/* Attempt to read a key */
		i = inkey();
	}

	/* Terminate */
	buf[n] = '\0';

	/* Flush */
	flush();


	/* Convert the trigger */
	ascii_to_text(tmp, buf);

	/* Hack -- display the trigger */
	Term_addstr(-1, TERM_WHITE, tmp);
}


/* When reinitializing macros, also reload font/graf prefs?
   Shoudln't be needed. */
//#define FORGET_MACRO_VISUALS

#ifdef TEST_CLIENT

/* Maximum amount of switchable macrofile-sets loaded at the same time */
#define MACROFILESETS_MAX 8
/* Maximum amount of switchable macrofile-set-stages */
#define MACROFILESETS_STAGES_MAX 6
/* String part that serves as marker for recognizing macrosets and their switch-type by the macros on their dedicated cycle/switch-keys */
#define MACROFILESET_MARKER_CYCLIC "Cycling\\sto\\sset"
#define MACROFILESET_MARKER_SWITCH "Switching\\sto\\sset"
#define MACROSET_NAME_LEN 20
#define MACROSET_COMMENT_LEN 20
struct macro_fileset_type {
	bool style_cyclic; // Style: cyclic (at least one trigger key was found that cycles)
	bool style_free; // Style: free-switching (at last one trigger key was found that switches freely)
	char basefilename[MACROSET_NAME_LEN]; // Base .prf filename part (excluding path) for all macro files of this set, to which stage numbers get appended
	char macro__pat__cycle[32];
	char macro__patbuf__cycle[32];
	char macro__act__cycle[160];
	char macro__actbuf__cycle[160];
	int stages; // Amount of stages to cyclic/switch between
	bool any_stage_file_exists; // just QoL shortcut derived from at least one of 'stage_file_exists[]' being TRUE
	bool all_stage_files_exist; // just QoL shortcut
	bool currently_referenced; // this macro set is referenced by at least one existing macro among all currently loaded macros

	char macro__pat__switch[MACROFILESETS_STAGES_MAX][32];
	char macro__patbuf__switch[MACROFILESETS_STAGES_MAX][32];
	char macro__act__switch[MACROFILESETS_STAGES_MAX][160];
	char macro__actbuf__switch[MACROFILESETS_STAGES_MAX][160];
	bool stage_file_exists[MACROFILESETS_STAGES_MAX]; // stage file was actually found? (eg if stage files 1,2,4 are found, we must assume there is a stage 3, but maybe the file is missing)
	char stage_comment[MACROFILESETS_STAGES_MAX][MACROSET_COMMENT_LEN];
};
typedef struct macro_fileset_type macro_fileset_type;

static int filesets_found = 0;
static int fileset_selected = -1, fileset_stage_selected = -1;
static macro_fileset_type fileset[MACROFILESETS_MAX] = { 0 };

/* Scan...
   (a) all currently loaded macros,
   (b) look on disk for files belonging to those scanned in (a),
   (c) all macro files on disk (in user/TomeNET-user folder) for filesets. - C. Blue */
int macroset_scan(void) {
	int k, m;
	bool style_cyclic, style_free;

	int f, n, stage;
	char *cc, *cf, *cfile;
	char buf_pat[32], buftxt_pat[32], buf_act[160], buftxt_act[160];
	char buf_basename[1024], tmpbuf[1024];
#ifndef WINDOWS
	size_t glob_size;
	glob_t glob_res;
	char **p;
#else
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
#endif
	char cwd[1024];


	/* Discard known filesets - fileset[] already gets zero-initialized */
#if 0
	for (k = 0; k < MACROFILESETS_MAX; k++) {
		fileset[k].stages = 0;
		fileset[k].currently_referenced = fileset[k].any_stage_file_exists = fileset[k].all_stage_files_exist = FALSE;
		for (f = 0; f < MACROFILESETS_STAGES_MAX; f++)
			fileset[k].stage_file_exists[f] = FALSE;
	}
#else
	memset(fileset, 0, sizeof(fileset));
#endif
	filesets_found = 0;
	fileset_selected = fileset_stage_selected = -1;


	/* ---------- (a) Auto-scan for all currently loaded (ie are referenced in our currently active macros) filesets ---------- */

	k = 0;
	m = -1;
	while (TRUE) {
		while (++m < macro__num) {
			style_cyclic = style_free = FALSE;

			/* Get macro in parsable format */
			strncpy(buf_act, macro__act[m], 159);
			buf_act[159] = '\0';
			ascii_to_text(buftxt_act, buf_act);

			/* Scan macro for marker text, indicating that it's a set-switch */
			// (note: MACROFILESET_MARKER_CYCLIC "Cycling\\sto\\sset")
			// (note: MACROFILESET_MARKER_SWITCH "Switching\\sto\\sset")
			if ((cc = strstr(buftxt_act, MACROFILESET_MARKER_CYCLIC))) style_cyclic = TRUE;
			if ((cf = strstr(buftxt_act, MACROFILESET_MARKER_SWITCH))) style_free = TRUE;

			if (!style_cyclic && !style_free) continue;

			/* Found one! */

			/* Determine base filename from the action text : %lFILENAME\r\e */
			cfile = strstr(buftxt_act, "%l");
			if (!cfile) continue; //broken set-switching macro (not following our known scheme)
			strcpy(buf_basename, cfile + 2);
			/* Find end of filename */
			cfile = strstr(buf_basename, "\\r\\e");
			if (!cfile) continue; //broken set-switching macro (not following our known scheme)
			*cfile = 0;
			/* Find start of 'stage' appendix of the filename, cut it off to obtain base filename.
			   Assume filename has this format "basename-FSn.prf" where n is the stage number: 0...MACROFILESETS_STAGES_MAX */
			/* If this is a cyclic macro, the number after FS won't give the # of cycles away! Only the %:... self-notification message can do that!
			   So it should follow a specific format: ":%:Cycling to set n of m\r 'comment'\r" <- the 'of m' giving away the true amount of stages for cyclic sets!
			   However, it might be better to instead scan the folder for macro files starting on the base filename instead, so we are sure to catch all. */
			if (strncmp(buf_basename + strlen(buf_basename) - 8, "-FS", 3)) continue; //broken set-switching macro (not following our known scheme)

			//TODO: Extract 'stage_comment' (eg 'water/cold spells' that is part of the switching-message)

			/* --- At this point, we confirmed a valid macro belonging to a macro set found --- */

			if (k >= MACROFILESETS_MAX) {
				/* Too many macro sets! */
				c_msg_format("\377yWarning(0): Excess macroset reference \"%s\" found! (max %d sets.).", buf_basename, MACROFILESETS_MAX);
				continue;
			}

			/* Finalize stage index */
			stage = atoi(buf_basename + strlen(buf_basename) - 5) - 1;
			/* Finalize base filename */
			buf_basename[strlen(buf_basename) - 8] = 0;

			/* Too many stages? */
			if (stage >= MACROFILESETS_STAGES_MAX) {
				c_msg_format("\377yWarning(0): Discarding excess stage file %d (maximum stage is %d)", stage + 1, MACROFILESETS_STAGES_MAX);
				c_msg_format("\377y            for macroset '%s'.", buf_basename);
				continue;
			} else if (stage < 0) {
				c_msg_format("\377yWarning(0): Discarding invalid stage file %d (stages must start at 1)", stage + 1);
				c_msg_format("\377y            for macroset '%s'.", buf_basename);
				continue;
			}

			/* Scan already known filesets for same basename,
			to either add the found key to an existing one or add a new fileset */
			for (f = 0; f < k; f++) {
				if (strcmp(fileset[f].basefilename, buf_basename)) continue;

				/* Found set! */

				/* New maximum stage registered? Then update # of stages. */
				if (stage >= fileset[f].stages) fileset[f].stages = stage + 1;

				/* Add stage to this already known set */
				break;
			}

			/* No known fileset of this basename found? Register a new set. */
			if (f == k) {
				fileset[k].style_cyclic = style_cyclic;
				fileset[k].style_free = style_free;

				fileset[k].stages = stage + 1; // We have at least this many stages, apparently
				strcpy(fileset[k].basefilename, buf_basename);

				/* Init cycle keys */
				fileset[k].macro__pat__cycle[0] = 0;
				fileset[k].macro__patbuf__cycle[0] = 0;

				/* Set is referenced by loaded macros in memory */
				fileset[k].currently_referenced = TRUE;

				/* One more registered macroset */
				k++;
			} else k = f; //unify, for subsequent code:

			/* Get trigger in parsable format */
			strncpy(buf_pat, macro__pat[m], 31);
			buf_pat[31] = '\0';
			ascii_to_text(buftxt_pat, buf_pat);

			/* Init switch keys */
			fileset[k].macro__pat__switch[stage][0] = 0;
			fileset[k].macro__patbuf__switch[stage][0] = 0;
			fileset[k].macro__act__switch[stage][0] = 0;
			fileset[k].macro__actbuf__switch[stage][0] = 0;
			/* Macro-set specific: */
			if (style_cyclic) {
				strcpy(fileset[k].macro__pat__cycle, buf_pat);
				strcpy(fileset[k].macro__patbuf__cycle, buftxt_pat);
			}
			/* Macro-set-stage specific: */
			if (style_free) {
				strcpy(fileset[k].macro__pat__switch[stage], buf_pat);
				strcpy(fileset[k].macro__patbuf__switch[stage], buftxt_pat);
				strcpy(fileset[k].macro__act__switch[stage], buf_act);
				strcpy(fileset[k].macro__actbuf__switch[stage], buftxt_act);
			}

			/* Continue scanning keys for switch-macros */
		}
		/* Scanned the last one of all loaded macros? We're done. */
		if (m >= macro__num - 1) break;
	}
	filesets_found = k;


	/* ---------- (b) Disk operations: Read macro files from TomeNET's user folder ---------- */

	getcwd(cwd, sizeof(cwd)); //Remember TomeNET working directory
	chdir(ANGBAND_DIR_USER); //Change to TomeNET user folder

	/* For cyclic sets: These don't have keys to switch to each stage, but only 1 key that switches to the next stage.
	   So to actually find all stages of a cyclic set, we therefore need to scan for all actually existing stage-filenames derived from the base filename.
	   Also, for free-switch sets we can use this anyway, just to verify whether a stage's file does actually exist or if there is a 'hole' (eg stage files 1,2,4 exist but 3 doesn't). */
	for (k = 0; k < filesets_found; k++) {
#ifndef WINDOWS
c_msg_format("(1)scan disk for set (%d) <%s>", k, fileset[k].basefilename);
		glob(format("%s-FS?.prf", fileset[k].basefilename), 0, 0, &glob_res);
		glob_size = glob_res.gl_pathc;
		if (glob_size < 1) { /* No macro files found at all, ew. */
			/* -- this warning is redundant with 'has no stage file(s)' warning further down ---
			c_msg_format("\377yWarning: Currently loaded macros refer to macroset \"%s\"", fileset[k].basefilename);
			c_msg_format("\377y         but there were no macro files \"%s-FS?.prf\" found of that name!", fileset[k].basefilename);
			*/
c_msg_print("(1)nothing");
		} else { /* Found 'n' macro files */
			fileset[k].any_stage_file_exists = TRUE;

			for (p = glob_res.gl_pathv; glob_size; p++, glob_size--) {
				/* Acquire base name and stage */
				strcpy(buf_basename, *p);
#else
		hFind = FindFirstFile("*-FS?.prf", &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE) //;
c_msg_format("(2)FindFirstFile failed (%ld)", GetLastError());
		else {
			strcpy(buf_basename, FindFileData.cFileName);
			n = 0;
			while (TRUE) {
				if (n++) {
					if (FindNextFile(hFind, &FindFileData)) strcpy(buf_basename, FindFileData.cFileName);
					else break;
				}
#endif
				/* Extract stage number */
				stage = atoi(buf_basename + strlen(fileset[k].basefilename) + 3) - 1;
				if (stage >= MACROFILESETS_STAGES_MAX) {
					c_msg_format("\377yWarning(1): Discarding excess stage file %d (maximum stage is %d)", stage + 1, MACROFILESETS_STAGES_MAX);
					c_msg_format("\377y            for macroset '%s'.", buf_basename);
					continue;
				} else if (stage < 0) {
					c_msg_format("\377yWarning(1): Discarding invalid stage file %d (stages must start at 1)", stage + 1);
					c_msg_format("\377y            for macroset '%s'.", buf_basename);
					continue;
				}
c_msg_format("(1)set (%d) <%s> registered stage %d", k, fileset[k].basefilename, stage);

				/* Register that there is an existing file to back up this stage's existance */
				fileset[k].stage_file_exists[stage] = TRUE;

				/* If stage number is higher than our highest known stage number, increase our known number to this one (new max found) */
				if (stage >= fileset[k].stages) fileset[k].stages = stage + 1;
			}
		}
#ifndef WINDOWS
		globfree(&glob_res);
#else
		if (hFind != INVALID_HANDLE_VALUE) FindClose(hFind);
#endif
	}


	/* ---------- (c) Now also scan user folder for any macro sets that aren't referenced by our currently loaded macros: ---------- */

#ifndef WINDOWS
	glob("*-FS?.prf", 0, 0, &glob_res);
	glob_size = glob_res.gl_pathc;
	if (glob_size < 1) //; /* No macro files found at all */
c_msg_print("(2)nothing");
	else { /* Found 'n' macro files */
		for (p = glob_res.gl_pathv; glob_size; p++, glob_size--) {
			/* Acquire base name and stage */
			strcpy(buf_basename, *p);
#else
	hFind = FindFirstFile("*-FS?.prf", &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) { //;
		c_msg_format("(2)FindFirstFile failed (%ld)", GetLastError());
	} else {
		strcpy(buf_basename, FindFileData.cFileName);
		n = 0;
		while (TRUE) {
			if (n++) {
				if (FindNextFile(hFind, &FindFileData)) strcpy(buf_basename, FindFileData.cFileName);
				else break;
			}
#endif
			/* Extract stage index */
			stage = atoi(buf_basename + strlen(buf_basename) - 5) - 1;
			/* Extract base name */
			buf_basename[strlen(buf_basename) - 8] = 0;

c_msg_format("(2)set <%s> / stage %d found", buf_basename, stage);
			/* Too many stages? */
			if (stage >= MACROFILESETS_STAGES_MAX) {
				c_msg_format("\377yWarning(2): Discarding excess stage file %d (maximum stage is %d)", stage + 1, MACROFILESETS_STAGES_MAX);
				c_msg_format("\377y            for macroset '%s'.", buf_basename);
				continue;
			} else if (stage < 0) {
				c_msg_format("\377yWarning(2): Discarding invalid stage file %d (stages must start at 1)", stage + 1);
				c_msg_format("\377y            for macroset '%s'.", buf_basename);
				continue;
			}

			/* Check if it's not one we already registered in (b) above, or here in (c) from a previous '*p' entry */
			for (k = 0; k < filesets_found; k++)
				if (!strcmp(fileset[k].basefilename, buf_basename)) break;

			/* It's a set we already know? */
			if (k != filesets_found) {
				/* If it was from (b) then we have fully checked all related files already and can continue.
				   To determine whether it was from (b), we utilize our knowledge that any filesets scanned in (b)
				   are those registered in (a) ie those currently referenced to by loaded macros in memory. */
				if (fileset[k].currently_referenced) continue;

				/* It's a new stage of a set we have registered already */
c_msg_format("(2)existing disk-set (%d) <%s> adds stage %d", k, fileset[k].basefilename, stage);

				/* Register that there is an existing file to back up this stage's existance */
				fileset[k].stage_file_exists[stage] = TRUE;

				/* New maximum stage registered? Then update # of stages. */
				if (stage >= fileset[k].stages) fileset[k].stages = stage + 1;
			}
			/* It's a new set that we haven't registered yet? */
			else {
				c_msg_format("Found disk-only set <%s>.", buf_basename);

				/* Register it as new set if possible (we still have space)*/

				if (k >= MACROFILESETS_MAX) {
					/* Too many macro sets! */
					c_msg_format("\377yWarning(2): Excess macroset reference \"%s\" found! (max %d sets.).", buf_basename, MACROFILESETS_MAX);
					continue;
				}

				fileset[k].stages = stage + 1; // We have at least this many stages, apparently
				strcpy(fileset[k].basefilename, buf_basename);

				/* Register that there is an existing file to back up this stage's existance */
				fileset[k].stage_file_exists[stage] = TRUE;

				fileset[k].style_cyclic = FALSE;//todo:find out- style_cyclic;
				fileset[k].style_free = FALSE;//todo:find out- style_free;

				/* Init cycle keys */
				fileset[k].macro__pat__cycle[0] = 0;
				fileset[k].macro__patbuf__cycle[0] = 0;

				/* Set is purely from disk, not referenced by loaded macros in memory */
				fileset[k].currently_referenced = FALSE;

				/* One more registered macroset */
				filesets_found++;
			}
		}
	}
#ifndef WINDOWS
	globfree(&glob_res);
#else
	if (hFind != INVALID_HANDLE_VALUE) FindClose(hFind);
#endif
	/* End of 'user' folder disk operations */
	chdir(cwd); //Return to TomeNET working directory


	/* ---------- For all known macro sets, now check whether macro files for all/some stages are missing ---------- */

	for (k = 0; k < filesets_found; k++) {
		/* Count all stage files */
		n = 0;
		for (f = 0; f < fileset[k].stages; f++) {
			if (fileset[k].stage_file_exists[f]) n++;
			else stage = f;
		}

		/* Found all of them aka fileset is complete? We're done. */
		if (n == fileset[k].stages) {
			fileset[k].all_stage_files_exist = TRUE;
			continue;
		}

		/* Found none, ie all stages are missing their file each? */
		if (!n) {
			if (fileset[k].stages == 1) {
				c_msg_format("\377yWarning(+): Macroset \"%s\" (1 stage) has no file.", fileset[k].basefilename);
			} else {
 #if 0
				c_msg_format("\377yWarning(+): Macroset \"%s\" (%d stage%s) has no files for any stage.",
				    fileset[k].basefilename, fileset[k].stages, fileset[k].stages != 1 ? "s" : "");
 #else /* Save screen space, shorter message */
				c_msg_format("\377yWarning(+): Macroset \"%s\" (%d stage%s) has no files.",
				    fileset[k].basefilename, fileset[k].stages, fileset[k].stages != 1 ? "s" : "");
 #endif
			}
		}
		/* Just one of the stages is missing a file? */
		else if (n == fileset[k].stages - 1) {
			c_msg_format("\377yWarning(+): Macroset \"%s\" (%d stage%s) has no file for stage %d.",
			    fileset[k].basefilename, fileset[k].stages, fileset[k].stages != 1 ? "s" : "", stage + 1);
		/* Several stages are missing files */
		} else {
			tmpbuf[0] = 0;
			for (f = 0; f < fileset[k].stages; f++)
				if (!fileset[k].stage_file_exists[f]) strcat(tmpbuf, format("%d, ", f + 1));
			tmpbuf[strlen(tmpbuf) - 2] = 0; //trim trailing comma

			c_msg_format("\377yWarning(+): Macroset \"%s\" (%d stage%s)",
			    fileset[k].basefilename, fileset[k].stages, fileset[k].stages != 1 ? "s" : "");
			c_msg_format("\377y            has no files for these stages: %s.", tmpbuf);
		}
	}

	/* all done, return # of sets found */
	return(filesets_found);
}

/* Prompt to enter an existing macrofileset number, stores it in 'f': */
#define GET_MACROFILESET \
	{ if (!filesets_found) continue; \
	Term_putstr(15, l, -1, TERM_L_GREEN, format("Enter number of set to select (1-%d): ", filesets_found)); \
	tmpbuf[0] = 0; \
	if (!askfor_aux(tmpbuf, 3, 0)) continue; \
	f = atoi(tmpbuf) - 1; \
	if (f < 0 || f >= filesets_found) { \
		c_msg_format("\377oError: Invalid number entered (%d).", f + 1); \
		continue; \
	} }

/* Prompt to enter an existing macrofileset-stage number of the currently selected fileset, stores it in 'f': */
#define GET_MACROFILESET_STAGE \
	{ if (fileset_selected == -1 || !fileset[fileset_selected].stages) continue; \
	Term_putstr(15, l, -1, TERM_L_GREEN, format("Enter number of stage to select (1-%d): ", fileset[fileset_selected].stages)); \
	tmpbuf[0] = 0; \
	if (!askfor_aux(tmpbuf, 3, 0)) continue; \
	f = atoi(tmpbuf) - 1; \
	if (f < 0 || f >= fileset[fileset_selected].stages) { \
		c_msg_format("\377oError: Invalid number entered (%d).", f + 1); \
		continue; \
	} }

#endif /* TEST_CLIENT */

void interact_macros(void) {
	int i, j = 0, l, l2, chain_type;
	char tmp[160], buf[1024], buf2[1024], *bptr, *b2ptr, chain_macro_buf[1024];
	char fff[1024], t_key[10], choice;
	bool m_ctrl, m_alt, m_shift, t_hyb, t_com;
	bool were_recording = FALSE;
	bool inkey_msg_old = inkey_msg; //just for cmd_message().. probably redundant and we could just remove the inkey_msg = TRUE at cmd_message() instead of doing these extra checks...
#ifdef TEST_CLIENT
	static bool rescan = TRUE; // (mw_fileset) always initially scan once on first invocation of the macro-fileset menu
#endif

	/* Save screen */
	Term_save();
	topline_icky = TRUE;

	/* No macros should work within the macro menu itself */
	inkey_interact_macros = TRUE; /* This makes setting inkey_msg = TRUE after cmd_message() redundant and therefore not needed */

	/* Did we just finish recording a macro by entering this menu? */
	if (recording_macro) {
		/* stop recording */
		were_recording = TRUE;
		recording_macro = FALSE;
		c_msg_print("...macro recording finished.");
		/* cut off the last key which was '%' to bring us back here */
		recorded_macro[strlen(recorded_macro) - 1] = '\0';
		/* use the recorded macro */
		strcpy(macro__buf, recorded_macro);
		/* redraw ie remove 'RECORDING' indicator before screen is saved */
		if (screen_icky) Term_switch(0);
		prt_extra_status(NULL);
		if (screen_icky) Term_switch(0);
	}

	/* Process requests until done */
	while (1) {
		/* Clear screen */
		Term_clear();

		/* Describe */
		Term_putstr(0, 0, -1, TERM_L_GREEN, "Interact with Macros");


		/* Selections */
		l = 2;
		Term_putstr(5, l++, -1, TERM_L_BLUE, "(\377yz\377B) Invoke macro wizard         *** Recommended ***");
		Term_putstr(5, l++, -1, TERM_L_BLUE, "(\377ys\377B/\377yS\377B/\377yF\377B/\377yA\377B) Save macros to named / global.prf / form-named / class pref file");
		Term_putstr(5, l++, -1, TERM_WHITE, "(\377yl\377w/\377yL\377w) Load macros from a pref file / load current class-specific pref file");
		Term_putstr(5, l++, -1, TERM_L_BLUE, "(\377yZ\377w) Invoke macro wizard and implicitely choose macro-set creation.");
		l++;
		Term_putstr(5, l++, -1, TERM_WHITE, "(\377yd\377w) Delete a macro from a key   (restores a key's normal behaviour)");
		Term_putstr(5, l++, -1, TERM_WHITE, "(\377yI\377w) Reinitialize all macros     (discards all unsaved macros)");
		Term_putstr(5, l++, -1, TERM_WHITE, "(\377yG\377w/\377yC\377w/\377yB\377w/\377yU\377w/\377yX\377w) Forget global.prf / <character>.prf / both / most / all");
		Term_putstr(5, l++, -1, TERM_WHITE, "(\377yt\377w/\377yi\377w) Test a key for an existing macro / list all currently defined macros");
		Term_putstr(5, l++, -1, TERM_WHITE, "(\377yw\377w) Swap the macro(s) of two keys");
		Term_putstr(5, l++, -1, TERM_SLATE, "(\377ua\377s) Enter a new macro action manually. Afterwards..");
		Term_putstr(5, l++, -1, TERM_SLATE, "(\377uh\377s) ..create a hybrid macro     (usually preferable over command/normal)");
		Term_putstr(5, l++, -1, TERM_SLATE, "(\377uc\377s) ..create a command macro    (most compatible, eg for using / and * key)");
		Term_putstr(5, l++, -1, TERM_SLATE, "(\377un\377s) ..create a normal macro     (persists everywhere, even in chat)");
		//Term_putstr(5, l++, -1, TERM_SLATE, "(\377u4\377s) Create a identity macro  (erases a macro)");
		Term_putstr(5, l++, -1, TERM_SLATE, "(\377ue\377s) Create an empty macro       (completely disables a key)");
		//Term_putstr(5, l++, -1, TERM_SLATE, "(\377uq\377s/\377yQ\377w) Enter and create a 'quick & dirty' macro / set preferences"),
		Term_putstr(5, l++, -1, TERM_SLATE, "(\377uq\377s) Enter and create a 'quick & dirty' macro"),
		//Term_putstr(5, l++, -1, TERM_SLATE, "(\377r\377w/\377yR\377w) Record a macro / set preferences");
		Term_putstr(5, l++, -1, TERM_SLATE, "(\377ur\377s) Record a macro");
		Term_putstr(5, l++, -1, TERM_SLATE, "(\377up\377s) Paste currently shown macro action to chat");
		//l++;

		/* Describe that action */
		Term_putstr(0, l + 2, -1, TERM_L_GREEN, "Current action (if any) shown below:");

		/* Analyze the current action */
		ascii_to_text(buf, macro__buf);

		/* Display the current action */
		Term_putstr(0, l + 3, -1, TERM_WHITE, buf);

		/* Prompt */
		Term_putstr(0, l, -1, TERM_L_GREEN, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		/* Allow to chat, to tell exact macros to other people easily */
		else if (i == ':') cmd_message();

		/* Take a screenshot */
		else if (i == KTRL('T')) xhtml_screenshot("screenshot????", 2);

		/* Load a pref file */
		else if (i == 'l') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Load a user pref file");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "File: ");

			/* Default filename */
			//sprintf(tmp, "user-%s.prf", ANGBAND_SYS);
			/* Use the character name by default - mikaelh */
			sprintf(tmp, "%s.prf", cname);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Process the given filename */
			(void)process_pref_file_manual(tmp);

			/* Pref files may change settings, so reload the keymap - mikaelh */
			keymap_init();
		}

		/* Load class-specific pref file */
		else if (i == 'L') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Load current class-specific pref file");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "File: ");

			sprintf(tmp, "%s.prf", class_info[class].title);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Process the given filename */
			if (process_pref_file(tmp) == -1) c_msg_format("\377yError: Can't read pref file '%s'!", tmp);

			/* Pref files may change settings, so reload the keymap - mikaelh */
			keymap_init();
		}

		/* Save a 'macro' file */
		else if (i == 's') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Save a macro file (use 'global.prf' for account-wide)");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "File: ");

			/* Default filename */
			//sprintf(tmp, "user-%s.prf", ANGBAND_SYS);
			/* Use the character name by default - mikaelh */
			sprintf(tmp, "%s.prf", cname);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Dump the macros */
			(void)macro_dump(tmp);
		}

		/* Save a 'macro' file as 'global.prf' */
		else if (i == 'S') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Save a macro file to global.prf (account-wide)");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "File: ");

			/* Default filename */
			strcpy(tmp, "global.prf");

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Dump the macros */
			(void)macro_dump(tmp);
		}

		/* Save a 'macro' file as <cname>_<form name> pref file, or revert to just cname if in player form */
		else if (i == 'F') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Save a macro file named for form");// (or 'global.prf' for account-wide)"); <- actually implement "global-<form>.prf" maybe? :O

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "File: ");

			/* Default filename */
			if (strcmp(c_p_ptr->body_name, "Player")) sprintf(tmp, "%s%c%s.prf", cname, PRF_BODY_SEPARATOR, c_p_ptr->body_name);
			/* Fall back to normal character name if not using any monster form */
			else sprintf(tmp, "%s.prf", cname);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Dump the macros */
			(void)macro_dump(tmp);
		}

		/* Save a 'macro' file as <class>.prf pref file */
		else if (i == 'A') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Save a class-specific macro file");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "File: ");

			/* Default filename */
			sprintf(tmp, "%s.prf", p_ptr->cp_ptr->title);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Dump the macros */
			(void)macro_dump(tmp);
		}

		/* Enter a new action */
		else if (i == 'a') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Enter a new action");

			/* Go to the correct location */
			Term_gotoxy(0, l + 3);

			/* Get an encoded action */
			if (!askfor_aux(buf, 159, 0)) continue;

			/* Extract an action */
			text_to_ascii(macro__buf, buf);
		}

		/* Create a command macro */
		else if (i == 'c') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Create a command macro");

			/* Prompt */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			/* Some keys aren't allowed to prevent the user
			   from locking himself out accidentally */
			if (!strcmp(buf, "\e") || !strcmp(buf, "%")) {
				c_msg_print("\377yKeys <ESC> and '%' aren't allowed to carry a macro.");
			} else {
				/* Link the macro */
				macro_add(buf, macro__buf, TRUE, FALSE);
				/* Message */
				c_msg_print("Created a new command macro.");
			}
		}

		/* Create a hybrid macro */
		else if (i == 'h') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Create a hybrid macro");

			/* Prompt */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			if (!strcmp(buf, "/") || !strcmp(buf, "*") || /* windows */
			    (strlen(buf) == 10 && (buf[8] == '/' || buf[8] == '*')) || /* x11 - to do: fix this crap */
			    (*buf >= 'a' && *buf <= 'w') || /* inventory */
			    *buf == '+' || *buf == '-' || /* the '+' and '-' keys are used in certain input prompts (targetting, drop newest item) */
			    *buf == ' ' || /* spacebar is used almost everywhere, should only be command macro */
			    (buf[0] == 1 && buf[1] == 0) /* CTRL+A glitch */
			    )
				c_msg_print("\377oWarning: This key may only work with command macros, not hybrid or normal ones!");

			/* Some keys aren't allowed to prevent the user
			   from locking himself out accidentally */
			if (!strcmp(buf, "\e") || !strcmp(buf, "%")) {
				c_msg_print("\377yKeys <ESC> and '%' aren't allowed to carry a macro.");
			} else {
				/* Link the macro */
				macro_add(buf, macro__buf, FALSE, TRUE);
				/* Message */
				c_msg_print("Created a new hybrid macro.");
			}
		}

		/* Create a normal macro */
		else if (i == 'n') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Create a normal macro");

			/* Prompt */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			if (!strcmp(buf, "/") || !strcmp(buf, "*") || /* windows */
			    (strlen(buf) == 10 && (buf[8] == '/' || buf[8] == '*')) || /* x11 - to do: fix this crap */
			    (*buf >= 'a' && *buf <= 'w') || /* inventory */
			    *buf == '+' || *buf == '-' || /* the '+' and '-' keys are used in certain input prompts (targetting, drop newest item) */
			    *buf == ' ' || /* spacebar is used almost everywhere, should only be command macro */
			    (buf[0] == 1 && buf[1] == 0) /* CTRL+A glitch */
			    )
				c_msg_print("\377oWarning: This key may only work with command macros, not hybrid or normal ones!");

			/* Some keys aren't allowed to prevent the user
			   from locking himself out accidentally */
			if (!strcmp(buf, "\e") || !strcmp(buf, "%")) {
				c_msg_print("\377yKeys <ESC> and '%' aren't allowed to carry a macro.");
			} else {
				/* Link the macro */
				macro_add(buf, macro__buf, FALSE, FALSE);
				/* Message */
				c_msg_print("Created a new normal macro.");
			}
		}

#if 0
		/* Create an identity macro */
		else if (i == 'd') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Create an identity macro");

			/* Prompt */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			/* Link the macro */
			macro_add(buf, buf, FALSE, FALSE);

			/* Message */
			c_msg_print("Created a new identity macro.");
		}
#else
		/* Delete a macro */
		else if (i == 'd') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Delete a macro");

			/* Prompt */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			/* Delete the macro */
			if (macro_del(buf)) {
				c_msg_print("The macro was deleted.");

				/* ..and actually restore the key from the predefined, non-custom macros: */
				strcpy(macro_trigger_exclusive, buf); //hack
				macro_processing_exclusive = TRUE;

				/* Access the "basic" pref file */
				strcpy(buf2, "pref.prf");
				process_pref_file(buf2);
				/* Access the "basic" system pref file */
				sprintf(buf2, "pref-%s.prf", ANGBAND_SYS);
				process_pref_file(buf2);

 #ifdef FORGET_MACRO_VISUALS
				/* Access the "visual" system pref file (if any) */
				handle_process_font_file();
 #endif

				macro_trigger_exclusive[0] = 0; //unhack
				macro_processing_exclusive = FALSE;


				/* if there's a default macro on that key, ask user if he wants to keep it */
				for (i = 0; i < macro__num; i++) {
					if (streq(macro__pat[i], buf)) {
						strncpy(macro__buf, macro__act[i], 159);
						macro__buf[159] = '\0';
						break;
					}
				}
				if (i < macro__num) {
					ascii_to_text(buf2, macro__buf);
					Term_putstr(0, l + 3, -1, TERM_YELLOW, "                                                                               ");
					Term_putstr(0, l + 3, -1, TERM_YELLOW, buf2);
					Term_putstr(0, l + 2, -1, TERM_YELLOW, format("This key resets to the following default %s macro. Delete it too? [y/N]",
					    macro__hyb[i] ? "hybrid" : (macro__cmd[i] ? "command" : "normal")));
					i = inkey();
					if (i == 'y' || i == 'Y') macro_del(buf);
				}
			} else {
				c_msg_print("No macro was found.");

				/* If there's a default macro available for this key, ask user if he wants to restore it! */
				strcpy(macro_trigger_exclusive, buf); //hack
				macro_processing_exclusive = TRUE;

				/* Access the "basic" pref file */
				strcpy(buf2, "pref.prf");
				process_pref_file(buf2);
				/* Access the "basic" system pref file */
				sprintf(buf2, "pref-%s.prf", ANGBAND_SYS);
				process_pref_file(buf2);

 #ifdef FORGET_MACRO_VISUALS
				/* Access the "visual" system pref file (if any) */
				handle_process_font_file();
 #endif

				macro_trigger_exclusive[0] = 0; //unhack
				macro_processing_exclusive = FALSE;

				for (i = 0; i < macro__num; i++) {
					if (streq(macro__pat[i], buf)) {
						strncpy(macro__buf, macro__act[i], 159);
						macro__buf[159] = '\0';
						break;
					}
				}
				if (i < macro__num) {
					ascii_to_text(buf2, macro__buf);
					Term_putstr(0, l + 3, -1, TERM_YELLOW, "                                                                               ");
					Term_putstr(0, l + 3, -1, TERM_YELLOW, buf2);
					Term_putstr(0, l + 2, -1, TERM_YELLOW, format("Do you wish to reset this key to the following default %s macro? [y/N]",
					    macro__hyb[i] ? "hybrid" : (macro__cmd[i] ? "command" : "normal")));
					i = inkey();
					if (i != 'y' && i != 'Y') macro_del(buf);
				}
			}
		}
#endif

		/* Create an empty macro */
		else if (i == 'e') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Create an empty macro");

			/* Prompt */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			/* Some keys aren't allowed to prevent the user
			   from locking himself out accidentally */
			if (!strcmp(buf, "\e") || !strcmp(buf, "%")) {
				c_msg_print("\377yKeys <ESC> and '%' aren't allowed to carry a macro.");
			} else {
				/* Link the macro */
				macro_add(buf, "", FALSE, FALSE);
				/* Message */
				c_msg_print("Created a new empty macro.");
			}
		}

		/* Query a macro */
		else if (i == 't') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Test key for a macro");

			/* Prompt */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			/* Re-using 'i' here shouldn't matter anymore */
			for (i = 0; i < macro__num; i++) {
				if (streq(macro__pat[i], buf)) {
					strncpy(macro__buf, macro__act[i], 159);
					macro__buf[159] = '\0';

					/* Message */
					ascii_to_text(tmp, macro__buf);
					if (macro__hyb[i]) c_msg_format("Found hybrid macro: %s", tmp);
					else if (macro__cmd[i]) c_msg_format("Found command macro: %s", tmp);
					else {
						if (!macro__act[i][0]) c_msg_print("Found empty macro.");
						else c_msg_format("Found normal macro: %s", tmp);
					}

					/* Update windows */
					window_stuff();

					break;
				}
			}

			if (i == macro__num) {
				/* Message */
				c_msg_print("No macro was found.");
			}
		}

		/* Switch macros */
		else if (i == 'w') {
			int mi1, mi2;
			char mpat1[1024], mpat2[1024];

			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Swap macro(s) of two keys");

			Term_putstr(0, l + 1, -1, TERM_WHITE, "Press first key: ");
			mpat1[0] = 0;
			get_macro_trigger(mpat1);
			Term_erase(18, l + 1, 255);

			for (mi1 = 0; mi1 < macro__num; mi1++)
				if (streq(macro__pat[mi1], mpat1)) break;

			Term_putstr(0, l + 1, -1, TERM_WHITE, "Press second key: ");
			mpat2[0] = 0;
			get_macro_trigger(mpat2);

			for (mi2 = 0; mi2 < macro__num; mi2++)
				if (streq(macro__pat[mi2], mpat2)) break;

			if (mi1 == macro__num && mi2 == macro__num) {
				c_msg_print("Neither key has a macro on it.");
				continue;
			}
			if (mi1 == macro__num)
				c_msg_format("Moving %s macro from second key to first key.", !macro__act[mi2][0] ? "empty" : (macro__hyb[mi2] ? "hybrid" : (macro__cmd[mi2] ? "command" : "normal")));
			else if (mi2 == macro__num)
				c_msg_format("Moving %s macro from first key to second key.", !macro__act[mi1][0] ? "empty" : (macro__hyb[mi1] ? "hybrid" : (macro__cmd[mi1] ? "command" : "normal")));
			else
				c_msg_format("Swapping macros of first key (%s) and second key (%s).",
				    !macro__act[mi1][0] ? "empty" : (macro__hyb[mi1] ? "hybrid" : (macro__cmd[mi1] ? "command" : "normal")),
				    !macro__act[mi2][0] ? "empty" : (macro__hyb[mi2] ? "hybrid" : (macro__cmd[mi2] ? "command" : "normal")));

			if (mi1 < macro__num) {
				string_free(macro__pat[mi1]);
				macro__pat[mi1] = string_make(mpat2);
			}
			if (mi2 < macro__num) {
				string_free(macro__pat[mi2]);
				macro__pat[mi2] = string_make(mpat1);
			}
		}

		/* List all macros */
		else if (i == 'i') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: List all macros");

			/* Re-using 'i' here shouldn't matter anymore */
			for (i = 0; i < macro__num; i++) {
				if (i % 20 == 0) {
					/* Clear screen */
					Term_clear();

					/* Describe */
					Term_putstr(0, 1, -1, TERM_L_UMBER, format("  *** Current Macro List (Entry %d-%d) ***", i + 1, i + 20 > macro__num ? macro__num : i + 20));
					Term_putstr(0, 22, -1, TERM_L_UMBER, "  [Press any key to continue, 'p' for previous, ESC to exit]");
				}

				/* Get trigger in parsable format */
				ascii_to_text(buf, macro__pat[i]);

				/* Get macro in parsable format */
				strncpy(macro__buf, macro__act[i], 159);
				macro__buf[159] = '\0';
				ascii_to_text(buf2, macro__buf);

#if 0 /* not a glitch maybe, just treat them @ len>2 clause.. */
#ifdef WINDOWS
				/* 'Glitch': Macros read from file have a trailing \r linefeed -_- */
				if (strlen(buf) > 2 && buf[strlen(buf) - 2] == '\\' && buf[strlen(buf) - 1] == 'r') buf[strlen(buf) - 2] = 0; //trim it
#endif
#endif

				/* Make the trigger human-readable */
				m_ctrl = m_alt = m_shift = FALSE;
#if 0 /* For debugging only: Code exclusion */
				if (TRUE) {
					sprintf(t_key, "%-9.9s", buf);
					/* ensure termination (in paranoid case of overflow) */
					t_key[9] = '\0';
				} else
#endif
				if (strlen(buf) == 1) {
					/* just a simple key */
					sprintf(t_key, "%c        ", buf[0]);
				} else if (strlen(buf) == 2) {
					/* ctrl + a simple key ('^%c' uppercase) or a special key ('\%c' or '^%c' lowercase) keypress
					 *
					 * special keys are represented by \+<normal letter> (lowercase) combo
					 * but comment from previous dev states:
					 * "special keys are represented by ^+<normal letter> combo.. >_>"
					 * so the code tries to consider both */
					m_ctrl = FALSE;
					bool is_simple = FALSE;

					switch (buf[1]) {
						/* a lowercase letter indicates special key */
						case 'b': sprintf(t_key, "Bsp/Del  "); break; // 'Backspace' and also 'Del' key! why?
						case 'r': sprintf(t_key, "Enter    "); break;
						case 's': sprintf(t_key, "Space    "); break;
						case 't': sprintf(t_key, "Tab      "); break;
						case 'w': sprintf(t_key, "`        "); break; // a grave (`) on en_US keyboard
						default: is_simple = TRUE;
					}
					if (is_simple) {
						if (buf[0] != '\\') {
							/* ctrl + a simple key */
							m_ctrl = TRUE;
							sprintf(t_key, "%c        ", buf[1]);
						} else sprintf(t_key, "\\%c       ", buf[1]); /* an unknown special key */
					}
				} else {
					/* special key, possibly with shift and/or ctrl */
					int keycode;

					bptr = buf + 2;
					/* Linux */
					if (*bptr == 'N') { m_ctrl = TRUE; bptr++; }
					if (*bptr == 'S') { m_shift = TRUE; bptr++; }
					if (*bptr == 'O') { m_alt = TRUE; bptr++; }
					/* Windows */
					if (*bptr == 'C') { m_ctrl = TRUE; bptr++; }
					if (*bptr == 'S') { m_shift = TRUE; bptr++; }
					if (*bptr == 'A') { m_alt = TRUE; bptr++; }
					bptr++;

					/* Translate keycode to some human-readable key (keyboard-layout-dependant hit or miss tho...) */
#ifdef WINDOWS
					/* Windows keycode */
					if (bptr[0] == 'F' && bptr[1] == 'F') {
						bptr[4] = 0;
						keycode = strtol(bptr + 2, NULL, 16);

						switch (keycode) {
						case 0xBE: strcpy(bptr,  "F1"); break;
						case 0xBF: strcpy(bptr,  "F2"); break;
						case 0xC0: strcpy(bptr,  "F3"); break;
						case 0xC1: strcpy(bptr,  "F4"); break;
						case 0xC2: strcpy(bptr,  "F5"); break;
						case 0xC3: strcpy(bptr,  "F6"); break;
						case 0xC4: strcpy(bptr,  "F7"); break;
						case 0xC5: strcpy(bptr,  "F8"); break;
						case 0xC6: strcpy(bptr,  "F9"); break;
						case 0xC7: strcpy(bptr,  "F10"); break;
						case 0xC8: strcpy(bptr,  "F11"); break;
						case 0xC9: strcpy(bptr,  "F12"); break;

						case 0x61: strcpy(bptr,  "PrtScr"); break;
						case 0x14: strcpy(bptr,  "Scroll"); break;
						case 0x13: strcpy(bptr,  "Pause"); break;

						/* Arrow keys et al, also number pad at numlock off */
						case 0x52: strcpy(bptr,  "Up"); break;
						case 0x54: strcpy(bptr,  "Down"); break;
						case 0x51: strcpy(bptr,  "Left"); break;
						case 0x53: strcpy(bptr,  "Right"); break;
						case 0x63: strcpy(bptr,  "Ins"); break; // 'Del' is ctrl+b!
						case 0x55: strcpy(bptr,  "PageUp"); break;
						case 0x56: strcpy(bptr,  "PageDn"); break;
						case 0x50: strcpy(bptr,  "Home"); break;
						case 0x57: strcpy(bptr,  "End"); break;

						/* Number pad (Numlock independant) */
						case 0xAF: strcpy(bptr,  "Num/"); break;
						case 0xAA: strcpy(bptr,  "Num*"); break;
						case 0xAD: strcpy(bptr,  "Num-"); break;
						case 0xAB: strcpy(bptr,  "Num+"); break;
						case 0x8D: strcpy(bptr,  "NumRet"); break;

						/* Number pad (Numlock off) */
						case 0x9F: strcpy(bptr,  "num."); break;
						case 0x9E: strcpy(bptr,  "num0"); break;
						case 0x9C: strcpy(bptr,  "num1"); break;
						case 0x99: strcpy(bptr,  "num2"); break;
						case 0x9B: strcpy(bptr,  "num3"); break;
						case 0x96: strcpy(bptr,  "num4"); break;
						case 0x9D: strcpy(bptr,  "num5"); break;
						case 0x98: strcpy(bptr,  "num6"); break;
						case 0x95: strcpy(bptr,  "num7"); break;
						case 0x97: strcpy(bptr,  "num8"); break;
						case 0x9A: strcpy(bptr,  "num9"); break;

						/* Number pad (Numlock off) */
						case 0xAC: strcpy(bptr,  "NUM."); break;
						case 0xB0: strcpy(bptr,  "NUM0"); break;
						case 0xB1: strcpy(bptr,  "NUM1"); break;
						case 0xB2: strcpy(bptr,  "NUM2"); break;
						case 0xB3: strcpy(bptr,  "NUM3"); break;
						case 0xB4: strcpy(bptr,  "NUM4"); break;
						case 0xB5: strcpy(bptr,  "NUM5"); break;
						case 0xB6: strcpy(bptr,  "NUM6"); break;
						case 0xB7: strcpy(bptr,  "NUM7"); break;
						case 0xB8: strcpy(bptr,  "NUM8"); break;
						case 0xB9: strcpy(bptr,  "NUM9"); break;

						/* Number pad duplicate, apparently numbers only, no idea.. */
						case 0xD8: strcpy(bptr,  "Num7"); break;
						case 0xD9: strcpy(bptr,  "Num8"); break;
						case 0xDA: strcpy(bptr,  "Num9"); break;
						case 0xDB: strcpy(bptr,  "Num4"); break;
						case 0xDC: strcpy(bptr,  "Num5"); break;
						case 0xDD: strcpy(bptr,  "Num6"); break;
						case 0xDE: strcpy(bptr,  "Num1"); break;
						case 0xDF: strcpy(bptr,  "Num2"); break;
						case 0xE0: strcpy(bptr,  "Num3"); break;
						}
					} else if (*(bptr - 1) == 'x') {
						bptr[2] = 0;
						keycode = strtol(bptr, NULL, 16);

						switch (keycode) {
						case 0x3B: strcpy(bptr,  "F1"); break;
						case 0x3C: strcpy(bptr,  "F2"); break;
						case 0x3D: strcpy(bptr,  "F3"); break;
						case 0x3E: strcpy(bptr,  "F4"); break;
						case 0x3F: strcpy(bptr,  "F5"); break;
						case 0x40: strcpy(bptr,  "F6"); break;
						case 0x41: strcpy(bptr,  "F7"); break;
						case 0x42: strcpy(bptr,  "F8"); break;
						case 0x43: strcpy(bptr,  "F9"); break;
						case 0x44: strcpy(bptr,  "F10"); break;
						case 0x57: strcpy(bptr,  "F11"); break;
						case 0x58: strcpy(bptr,  "F12"); break;

						//? case 0x61: strcpy(bptr,  "PrtScr"); break;
						//? case 0x14: strcpy(bptr,  "Scroll"); break;
						//? case 0x13: strcpy(bptr,  "Pause"); break;

						/* Arrow keys et al, also number pad at numlock off */
						case 0x48: strcpy(bptr,  "Up"); break;
						case 0x50: strcpy(bptr,  "Down"); break;
						case 0x4A: strcpy(bptr,  "num-"); break;
						case 0x4B: strcpy(bptr,  "Left"); break;
						case 0x4C: strcpy(bptr,  "num5"); break;
						case 0x4D: strcpy(bptr,  "Right"); break;
						case 0x4E: strcpy(bptr,  "num+"); break;
						case 0x52: strcpy(bptr,  "Ins"); break;
						case 0x53: strcpy(bptr,  "Del"); break;
						case 0x49: strcpy(bptr,  "PageUp"); break;
						case 0x51: strcpy(bptr,  "PageDn"); break;
						case 0x47: strcpy(bptr,  "Home"); break;
						case 0x4F: strcpy(bptr,  "End"); break;

						/* Number pad (Numlock off) */
						/* -- duplicates of the normal keyboard key block listed above, while numlock is off! --
						case 0x53: strcpy(bptr,  "num."); break;
						case 0x52: strcpy(bptr,  "num0"); break;
						case 0x4F: strcpy(bptr,  "num1"); break;
						case 0x50: strcpy(bptr,  "num2"); break;
						case 0x51: strcpy(bptr,  "num3"); break;
						case 0x4B: strcpy(bptr,  "num4"); break;
						//(unusable with numlock off) case 0x: strcpy(bptr,  "num5"); break;
						case 0x4D: strcpy(bptr,  "num6"); break;
						case 0x47: strcpy(bptr,  "num7"); break;
						case 0x48: strcpy(bptr,  "num8"); break;
						case 0x49: strcpy(bptr,  "num9"); break;
						---- */

						/* Number pad (Numlock on)
						   - same as the normal keyboard keys with corresponding symbols! - */

						/* Strange codes left over.. looking like numpad */
						case 0x77: strcpy(bptr,  "Num7"); break;
						case 0x8D: strcpy(bptr,  "Num8"); break;
						case 0x84: strcpy(bptr,  "Num9"); break;
						case 0x8E: strcpy(bptr,  "Num-"); break;
						case 0x73: strcpy(bptr,  "Num4"); break;
						case 0x8F: strcpy(bptr,  "Num5"); break;
						case 0x74: strcpy(bptr,  "Num6"); break;
						case 0x90: strcpy(bptr,  "Num+"); break;
						case 0x75: strcpy(bptr,  "Num1"); break;
						case 0x91: strcpy(bptr,  "Num2"); break;
						case 0x76: strcpy(bptr,  "Num3"); break;
						case 0x92: strcpy(bptr,  "Num0"); break;
						case 0x93: strcpy(bptr,  "Num."); break;
						}
					} else if (bptr[0] == '_') {
						//???
					}
#else
					/* Linux keycode */
					if (bptr[0] == 'F' && bptr[1] == 'F') {
						bptr[4] = 0;
						keycode = strtol(bptr + 2, NULL, 16);

						switch (keycode) {
						case 0xBE: strcpy(bptr,  "F1"); break;
						case 0xBF: strcpy(bptr,  "F2"); break;
						case 0xC0: strcpy(bptr,  "F3"); break;
						case 0xC1: strcpy(bptr,  "F4"); break;
						case 0xC2: strcpy(bptr,  "F5"); break;
						case 0xC3: strcpy(bptr,  "F6"); break;
						case 0xC4: strcpy(bptr,  "F7"); break;
						case 0xC5: strcpy(bptr,  "F8"); break;
						case 0xC6: strcpy(bptr,  "F9"); break;
						case 0xC7: strcpy(bptr,  "F10"); break;
						case 0xC8: strcpy(bptr,  "F11"); break;
						case 0xC9: strcpy(bptr,  "F12"); break;

						case 0x61: strcpy(bptr,  "PrtScr"); break;
						case 0x14: strcpy(bptr,  "Scroll"); break;
						case 0x13: strcpy(bptr,  "Pause"); break;

						/* Arrow keys et al, also number pad at numlock off */
						case 0x52: strcpy(bptr,  "Up"); break;
						case 0x54: strcpy(bptr,  "Down"); break;
						case 0x51: strcpy(bptr,  "Left"); break;
						case 0x53: strcpy(bptr,  "Right"); break;
						case 0x63: strcpy(bptr,  "Ins"); break; // 'Del' is ctrl+b!
						case 0x55: strcpy(bptr,  "PageUp"); break;
						case 0x56: strcpy(bptr,  "PageDn"); break;
						case 0x50: strcpy(bptr,  "Home"); break;
						case 0x57: strcpy(bptr,  "End"); break;

						/* Number pad (Numlock independant) */
						case 0xAF: strcpy(bptr,  "Num/"); break;
						case 0xAA: strcpy(bptr,  "Num*"); break;
						case 0xAD: strcpy(bptr,  "Num-"); break;
						case 0xAB: strcpy(bptr,  "Num+"); break;
						case 0x8D: strcpy(bptr,  "NumRet"); break;

						/* Number pad (Numlock off) */
						case 0x9F: strcpy(bptr,  "num."); break;
						case 0x9E: strcpy(bptr,  "num0"); break;
						case 0x9C: strcpy(bptr,  "num1"); break;
						case 0x99: strcpy(bptr,  "num2"); break;
						case 0x9B: strcpy(bptr,  "num3"); break;
						case 0x96: strcpy(bptr,  "num4"); break;
						case 0x9D: strcpy(bptr,  "num5"); break;
						case 0x98: strcpy(bptr,  "num6"); break;
						case 0x95: strcpy(bptr,  "num7"); break;
						case 0x97: strcpy(bptr,  "num8"); break;
						case 0x9A: strcpy(bptr,  "num9"); break;

						/* Number pad (Numlock off) */
						case 0xAC: strcpy(bptr,  "NUM."); break;
						case 0xB0: strcpy(bptr,  "NUM0"); break;
						case 0xB1: strcpy(bptr,  "NUM1"); break;
						case 0xB2: strcpy(bptr,  "NUM2"); break;
						case 0xB3: strcpy(bptr,  "NUM3"); break;
						case 0xB4: strcpy(bptr,  "NUM4"); break;
						case 0xB5: strcpy(bptr,  "NUM5"); break;
						case 0xB6: strcpy(bptr,  "NUM6"); break;
						case 0xB7: strcpy(bptr,  "NUM7"); break;
						case 0xB8: strcpy(bptr,  "NUM8"); break;
						case 0xB9: strcpy(bptr,  "NUM9"); break;

						/* Number pad duplicate, apparently numbers only, no idea.. */
						case 0xD8: strcpy(bptr,  "Num7"); break;
						case 0xD9: strcpy(bptr,  "Num8"); break;
						case 0xDA: strcpy(bptr,  "Num9"); break;
						case 0xDB: strcpy(bptr,  "Num4"); break;
						case 0xDC: strcpy(bptr,  "Num5"); break;
						case 0xDD: strcpy(bptr,  "Num6"); break;
						case 0xDE: strcpy(bptr,  "Num1"); break;
						case 0xDF: strcpy(bptr,  "Num2"); break;
						case 0xE0: strcpy(bptr,  "Num3"); break;
						}
					} else if (bptr[0] == '_') {
						//???
					}
#endif

					sprintf(t_key, "%-9.9s", bptr);
					/* ensure termination (in paranoid case of overflow) */
					t_key[9] = '\0';
				}

				/* determine trigger type */
				t_hyb = t_com = FALSE;
				if (macro__hyb[i]) t_hyb = TRUE;
				else if (macro__cmd[i]) t_com = TRUE;

				/* build a whole line */
				sprintf(fff, "%s %s %s %s [%s]  %-49.49s", m_shift ? "SHF" : "   ", m_ctrl ? "CTL" : "   ", m_alt ? "ALT" : "   ",
				    t_key, t_hyb ? "HY" : t_com ? "C " : "  ", buf2);

				Term_putstr(0, i % 20 + 2, -1, TERM_WHITE, fff);

				/* Wait for keypress before displaying more */
				if ((i % 20 == 19) || (i == macro__num - 1)) switch (inkey()) {
					case ESCAPE:
						i = -2; /* hack to leave for loop */
						break;
					case 'p':
					case '\010': /* backspace */
						if (i >= 39) {
							/* show previous 20 entries */
							i -= 20 + (i % 20) + 1;
						} else { /* wrap around */
							i = macro__num - (macro__num % 20) - 1;
							if (i == macro__num - 1) i -= 20;
						}
						break;
					case KTRL('T'):
						/* Take a screenshot */
						xhtml_screenshot("screenshot????", 2);
						/* keep current list */
						i -= (i % 20) + 1;
						break;
					default:
						/* show next 20 entries */
						if (i == macro__num - 1) i = -1; /* restart list at 1st macro again */
				}
				if (i == -2) break;
			}

			if (i == 0) {
				/* Message */
				c_msg_print("No macro was found.");
			}
		}

		/* Enter a 'quick & dirty' macro */
		else if (i == 'q') {
			bool call_by_name = FALSE, mimic_transform = FALSE;

			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Enter a new 'quick & dirty' macro");

			/* Go to the correct location */
			Term_gotoxy(0, l + 3);

			/* Get an encoded action */
			if (!askfor_aux(buf, 159, 0)) continue;

			/* Fix it up quick and dirty: Ability code short-cutting */
			buf2[0] = '\\'; //note: should in theory be ')e\',
			buf2[1] = 'e'; //      but doesn't work due to prompt behaviour
			buf2[2] = ')'; //      (\e will then get ignored)
			buf2[3] = 0; //paranoia
			bptr = buf;
			b2ptr = buf2 + 3;
			while (*bptr) {
				/* close an active @..\ tag? */
				if (call_by_name) {
					if (*bptr == '\\') { /* '\' is terminator for '@' call-by-name tag */
						call_by_name = mimic_transform = FALSE;
						*b2ptr++ = '\\'; *b2ptr++ = 'r';
						bptr++;
					} else *b2ptr++ = *bptr++;
					continue;
				}

				/* service (end of I-tag): mimic-transformation into monster index number automatically adds the required '\r' at the end */
				if (mimic_transform && (*bptr) != '@' && (*bptr) != '-' && ((*bptr) < '0' || (*bptr) > '9')) { /* allow '-1' for 'previous form' */
					*b2ptr++ = '\\'; *b2ptr++ = 'r';
					mimic_transform = FALSE;
				}

				/* expand q'n'd token to a real command */
				switch (*bptr) {
				case 'P': /* use innate mimic power */
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '3'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
					bptr++;	break;
				case 'I': /* use innate mimic power: transform into specific */
					mimic_transform = TRUE;
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '3'; *b2ptr++ = '\\'; *b2ptr++ = 'r'; *b2ptr++ = 'c';
					bptr++;	break;
				case 'F': /* employ fighting technique */
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '5'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
					bptr++;	break;
				case 'S': /* employ shooting technique */
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '6'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
					bptr++;	break;
				case 'T': /* set a trap */
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '1'; *b2ptr++ = '0'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
					bptr++;	break;
				case 'M': /* cast a spell */
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '1'; *b2ptr++ = '1'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
					bptr++;	break;
				case 'R': /* draw a rune */
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '1'; *b2ptr++ = '2'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
					bptr++;	break;
				case 's': /* change stance */
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '1'; *b2ptr++ = '3'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
					bptr++;	break;
#if 0 /* disabled, to allow @q */
				case '*': /* set a target */
					*b2ptr++ = '*'; *b2ptr++ = 't';
					bptr++;	break;
#endif
				case '@': /* start 'call-by-name' mode, reading the spell/item name next */
					call_by_name = TRUE;
					if (!mimic_transform) *b2ptr++ = '@'; /* does work, but is actually not required when entering a mimic form */
					bptr++;	break;
				default:
					*b2ptr++ = *bptr++;
				}
			}
			/* service (end of line): mimic-transformation into monster index number automatically adds the required '\r' at the end */
			if (mimic_transform) {
				*b2ptr++ = '\\'; *b2ptr++ = 'r';
			}

			/* terminate anyway */
			*b2ptr = '\0';

			/* Display the current action */
			Term_putstr(0, l + 3, -1, TERM_WHITE, buf2);

			/* Extract an action */
			text_to_ascii(macro__buf, buf2);

			/* Prompt */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			/* Some keys aren't allowed to prevent the user
			   from locking himself out accidentally */
			if (!strcmp(buf, "\e") || !strcmp(buf, "%")) {
				c_msg_print("\377yKeys <ESC> and '%' aren't allowed to carry a macro.");
			} else {
				/* Automatically choose usually best fitting macro type,
				   depending on chosen trigger key! */
				//[normal macros: F-keys (only keys that aren't used for any text input)]
				//command macros: / * a..w (all keys that are used in important standard prompts: inventory) and spacebar
				//hybrid macros: all others, maybe even also normal-macro-keys
				if (!strcmp(buf, "/") || !strcmp(buf, "*") || /* windows */
				    (strlen(buf) == 10 && (buf[8] == '/' || buf[8] == '*')) || /* x11 - to do: fix this crap */
#if 0
				    (*buf >= 'a' && *buf <= 'w') || /* inventory */
#else
				    (*buf >= 'a' && *buf <= 'z') || /* command keys - you shouldn't really put macros on these, but w/e.. */
				    (*buf >= 'A' && *buf <= 'Z') ||
				    (*buf >= '0' && *buf <= '9') || /* menu choices. Especially for ~ menu it is required that these are command macros so they don't trigger there.. */
				    *buf == ' ' || /* spacebar is used almost everywhere, should only be command macro */
#endif
				    *buf == '+' || *buf == '-' || /* the '+' and '-' keys are used in certain input prompts (targetting, drop newest item) */
				    (buf[0] == 1 && buf[1] == 0) /* CTRL+A glitch */
				    ) {
					/* make it a command macro */
					/* Link the macro */
					macro_add(buf, macro__buf, TRUE, FALSE);
					/* Message */
					c_msg_print("Created a new command macro.");
				} else {
					/* make it a hybrid macro */
					/* Link the macro */
					macro_add(buf, macro__buf, FALSE, TRUE);
					/* Message */
					c_msg_print("Created a new hybrid macro.");
				}
			}
		}

		/* Configure 'quick & dirty' macro functionality */
		else if (i == 'Q') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Configure 'quick & dirty' macro functionality");

			/* TODO:
			   config auto-prefix '\e)' */
		}

		/* Start recording a macro */
		else if (i == 'r') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Record a macro");

			/* Clear screen */
			Term_clear();

			/* Describe */
			Term_putstr(25, 4, -1, TERM_L_RED, "*** Recording a macro ***");
			Term_putstr(8, 8, -1, TERM_WHITE, "As soon as you press any key, the macro recorder will begin to");
			Term_putstr(15, 9, -1, TERM_WHITE, "record all further key presses you will perform.");
			Term_putstr(5, 11, -1, TERM_WHITE, "These can afterwards be saved to a macro, that you can bind to a key.");
			Term_putstr(5, 13, -1, TERM_WHITE, "To stop the recording process, just press '%' key to enter the macro");
			Term_putstr(5, 14, -1, TERM_WHITE, "menu again. You'll then be able to create a normal, command or hybrid");
			Term_putstr(4, 15, -1, TERM_WHITE, "macro from the whole recorded action by choosing the usual menu points");
			Term_putstr(16, 16, -1, TERM_WHITE, "for the different macro types: h), c) or n).");
			Term_putstr(19, 20, -1, TERM_SELECTOR, ">>>                                <<<");
			Term_putstr(19 + 3, 20, -1, TERM_L_RED,   "Press any key to start recording");

			/* Wait for confirming keypress to finally start recording */
			inkey();

			/* Reload screen */
			Term_load();
			topline_icky = FALSE;

			/* Flush the queue */
			Flush_queue();

			/* leave macro menu */
			inkey_interact_macros = FALSE;
			/* hack: Display recording status */
			Send_redraw(0);

			/* enter recording mode */
			strcpy(recorded_macro, "");
			recording_macro = TRUE;
			c_msg_print("Macro recording started...");

			/* leave the macro menu */
			inkey_msg = inkey_msg_old;
			return;
		}

		/* Configure macro recording functionality */
		else if (i == 'R') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Configure macro recording functionality");

			/* TODO: implement */
		}

		else  if (i == 'G') {
			/* Forget macros loaded from global.prf */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Forget global macros");

			for (i = 0; i < macro__num; i++) {
				string_free(macro__pat[i]);
				macro__pat[i] = NULL;
				string_free(macro__act[i]);
				macro__act[i] = NULL;
				macro__cmd[i] = FALSE;
				macro__hyb[i] = FALSE;
			}
			macro__num = 0;
			for (i = 0; i < 256; i++) macro__use[i] = 0;

			macro_processing_exclusive = TRUE;

			/* Access the "basic" pref file */
			strcpy(buf, "pref.prf");
			process_pref_file(buf);
			/* Access the "basic" system pref file */
			sprintf(buf, "pref-%s.prf", ANGBAND_SYS);
			process_pref_file(buf);
#ifdef FORGET_MACRO_VISUALS
			/* Access the "visual" system pref file (if any) */
			handle_process_font_file();
#endif
#if 0 /* skip exactly these here */
			/* Access the "global" macro file */
			sprintf(buf, "global.prf");
			process_pref_file(buf);
#endif
			/* Access the "race" pref file */
			if (race < Setup.max_race) {
				sprintf(buf, "%s.prf", race_info[race].title);
				process_pref_file(buf);
			}
			/* Access the "trait" pref file */
			if (race < Setup.max_trait) {
				sprintf(buf, "%s.prf", trait_info[trait].title);
				process_pref_file(buf);
			}
			/* Access the "class" pref file */
			if (class < Setup.max_class) {
				sprintf(buf, "%s.prf", class_info[class].title);
				process_pref_file(buf);
			}
			/* Access the "character" pref file */
			load_charspec_macros(cname);

			macro_processing_exclusive = FALSE;
			c_msg_print("Reninitialized all macros, omitting 'global.prf'");
		}

		else if (i == 'C') {
			/* Forget macros loaded from character.prf */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Forget character-specific macros");

			for (i = 0; i < macro__num; i++) {
				string_free(macro__pat[i]);
				macro__pat[i] = NULL;
				string_free(macro__act[i]);
				macro__act[i] = NULL;
				macro__cmd[i] = FALSE;
				macro__hyb[i] = FALSE;
			}
			macro__num = 0;
			for (i = 0; i < 256; i++) macro__use[i] = 0;

			macro_processing_exclusive = TRUE;

			/* Access the "basic" pref file */
			strcpy(buf, "pref.prf");
			process_pref_file(buf);
			/* Access the "basic" system pref file */
			sprintf(buf, "pref-%s.prf", ANGBAND_SYS);
			process_pref_file(buf);
#ifdef FORGET_MACRO_VISUALS
			/* Access the "visual" system pref file (if any) */
			handle_process_font_file();
#endif
			/* Access the "global" macro file */
			sprintf(buf, "global.prf");
			process_pref_file(buf);
			/* Access the "race" pref file */
			if (race < Setup.max_race) {
				sprintf(buf, "%s.prf", race_info[race].title);
				process_pref_file(buf);
			}
			/* Access the "trait" pref file */
			if (race < Setup.max_trait) {
				sprintf(buf, "%s.prf", trait_info[trait].title);
				process_pref_file(buf);
			}
			/* Access the "class" pref file */
			if (class < Setup.max_class) {
				sprintf(buf, "%s.prf", class_info[class].title);
				process_pref_file(buf);
			}
#if 0 /* skip exactly these here */
			/* Access the "character" pref file */
			load_charspec_macros(cname);
#endif

			macro_processing_exclusive = FALSE;
			c_msg_format("Reninitialized all macros, omitting '%s*.prf", cname);
		}

		else if (i == 'B') {
			/* Forget macros loaded from global.prf and character.prf */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Forget global and character-specific macros");

			for (i = 0; i < macro__num; i++) {
				string_free(macro__pat[i]);
				macro__pat[i] = NULL;
				string_free(macro__act[i]);
				macro__act[i] = NULL;
				macro__cmd[i] = FALSE;
				macro__hyb[i] = FALSE;
			}
			macro__num = 0;
			for (i = 0; i < 256; i++) macro__use[i] = 0;

			macro_processing_exclusive = TRUE;

			/* Access the "basic" pref file */
			strcpy(buf, "pref.prf");
			process_pref_file(buf);
			/* Access the "basic" system pref file */
			sprintf(buf, "pref-%s.prf", ANGBAND_SYS);
			process_pref_file(buf);
#ifdef FORGET_MACRO_VISUALS
			/* Access the "visual" system pref file (if any) */
			handle_process_font_file();
#endif
#if 0 /* skip these here */
			/* Access the "global" macro file */
			sprintf(buf, "global.prf");
			process_pref_file(buf);
#endif
			/* Access the "race" pref file */
			if (race < Setup.max_race) {
				sprintf(buf, "%s.prf", race_info[race].title);
				process_pref_file(buf);
			}
			/* Access the "trait" pref file */
			if (trait < Setup.max_trait) {
				sprintf(buf, "%s.prf", trait_info[trait].title);
				process_pref_file(buf);
			}
			/* Access the "class" pref file */
			if (class < Setup.max_class) {
				sprintf(buf, "%s.prf", class_info[class].title);
				process_pref_file(buf);
			}
#if 0 /* skip these here */
			/* Access the "character" pref file */
			load_charspec_macros(cname);
#endif

			macro_processing_exclusive = FALSE;
			c_msg_format("Reninitialized all macros, omitting 'global.prf' and '%s*.prf'", cname);
		}

		else if (i == 'U') {
			/* Forget all automatically loaded macros (global, charname, race, trait, class) */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Forget all auto-loaded macros");

			for (i = 0; i < macro__num; i++) {
				string_free(macro__pat[i]);
				macro__pat[i] = NULL;
				string_free(macro__act[i]);
				macro__act[i] = NULL;
				macro__cmd[i] = FALSE;
				macro__hyb[i] = FALSE;
			}
			macro__num = 0;
			for (i = 0; i < 256; i++) macro__use[i] = 0;

			macro_processing_exclusive = TRUE;

			/* Access the "basic" pref file */
			strcpy(buf, "pref.prf");
			process_pref_file(buf);
			/* Access the "basic" system pref file */
			sprintf(buf, "pref-%s.prf", ANGBAND_SYS);
			process_pref_file(buf);
#ifdef FORGET_MACRO_VISUALS
			/* Access the "visual" system pref file (if any) */
			handle_process_font_file();
#endif
#if 0 /* skip these here */
			/* Access the "global" macro file */
			sprintf(buf, "global.prf");
			process_pref_file(buf);
			/* Access the "race" pref file */
			if (race < Setup.max_race) {
				sprintf(buf, "%s.prf", race_info[race].title);
				process_pref_file(buf);
			}
			/* Access the "trait" pref file */
			if (trait < Setup.max_trait) {
				sprintf(buf, "%s.prf", trait_info[trait].title);
				process_pref_file(buf);
			}
			/* Access the "class" pref file */
			if (class < Setup.max_class) {
				sprintf(buf, "%s.prf", class_info[class].title);
				process_pref_file(buf);
			}
			/* Access the "character" pref file */
			load_charspec_macros(cname);
#endif

			macro_processing_exclusive = FALSE;
			c_msg_print("Reninitialized all macros, omitting all auto-loaded prf-files:");
			c_msg_format(" 'global.prf', '%s*.prf', '%s.prf', '%s.prf, '%s.prf'",
			    cname,
			    race < Setup.max_race ? race_info[race].title : "NO_RACE",
			    trait < Setup.max_trait ? trait_info[trait].title : "NO_TRAIT",
			    class < Setup.max_class ? class_info[class].title : "NO_CLASS");
		}

		else if (i == 'X') {
			/* Forget macros loaded from character.prf */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Forget all macros");

			for (i = 0; i < macro__num; i++) {
				string_free(macro__pat[i]);
				macro__pat[i] = NULL;
				string_free(macro__act[i]);
				macro__act[i] = NULL;
				macro__cmd[i] = FALSE;
				macro__hyb[i] = FALSE;
			}
			macro__num = 0;
			for (i = 0; i < 256; i++) macro__use[i] = 0;

			c_msg_print("Unloaded all macros");
		}

		else if (i == 'I') {
			/* Forget all macros, then reload all macro files */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Reinitialize all macros");

			for (i = 0; i < macro__num; i++) {
				string_free(macro__pat[i]);
				macro__pat[i] = NULL;
				string_free(macro__act[i]);
				macro__act[i] = NULL;
				macro__cmd[i] = FALSE;
				macro__hyb[i] = FALSE;
			}
			macro__num = 0;
			for (i = 0; i < 256; i++) macro__use[i] = 0;

			macro_processing_exclusive = TRUE;

			/* Access the "basic" pref file */
			strcpy(buf, "pref.prf");
			process_pref_file(buf);
			/* Access the "basic" system pref file */
			sprintf(buf, "pref-%s.prf", ANGBAND_SYS);
			process_pref_file(buf);
#ifdef FORGET_MACRO_VISUALS
			/* Access the "visual" system pref file (if any) */
			handle_process_font_file();
#endif
			/* Access the "global" macro file */
			sprintf(buf, "global.prf");
			process_pref_file(buf);
			/* Access the "race" pref file */
			if (race < Setup.max_race) {
				sprintf(buf, "%s.prf", race_info[race].title);
				process_pref_file(buf);
			}
			/* Access the "trait" pref file */
			if (trait < Setup.max_trait) {
				sprintf(buf, "%s.prf", trait_info[trait].title);
				process_pref_file(buf);
			}
			/* Access the "class" pref file */
			if (class < Setup.max_class) {
				sprintf(buf, "%s.prf", class_info[class].title);
				process_pref_file(buf);
			}
			/* Access the "character" pref file */
			load_charspec_macros(cname);


			macro_processing_exclusive = FALSE;
			c_msg_print("Reninitialized all macros.");
		}

		else if (i == 'p') {
			/* Paste macro as chat line */

			/* Analyze the current action */
			ascii_to_text(buf, macro__buf);

			if (!buf[0]) {
				c_msg_print("No macro action shown.");
				continue;
			}

			Send_paste_msg(buf);
		}

		else if (i == 'z' || i == 'Z') {
			int target_dir = '5', ystart = 6;
			bool should_wait, force_normal;
#define mw_quaff 'a'
#define mw_read 'b'
#define mw_fire 'c'
#define mw_throw 'C'
#define mw_schoolnt 'd'
#define mw_mimicnt 'D'
#define mw_schoolt 'e'
#define mw_mimict 'E'
#define mw_mimicidx 'f'
#define mw_poly 'g'
#define mw_prfimm 'G'
#define mw_rune 'h'
#define mw_fight 'i'
#define mw_stance 'I'
#define mw_shoot 'j'
#define mw_trap 'H'
#define mw_device 'J'
#define mw_any 'k'
#define mw_anydir 'K'
#define mw_abilitynt 'l'
#define mw_abilityt 'L'
#define mw_common 'm'
#define mw_prfele 'M'
#define mw_slash 'n'
#define mw_custom 'N'
#define mw_load 'o'
#define mw_option 'O'
#define mw_equip 'p'
#define mw_dir_run 'q'
#define mw_dir_tunnel 'r'
#define mw_dir_disarm 's'
#define mw_dir_bash 't'
#define mw_dir_close 'u'
#define mw_LAST 'u'
#define mw_chain 'Z'
#define mw_fileset 'S'

			/* Invoke wizard to create a macro step-by-step as easy as possible  */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Invoke macro wizard");

			/* initialise macro-chaining */
			chain_macro_buf[0] = 0;
			chain_type = 0; // 0 = command, 1 = hybrid, 2 = normal
Chain_Macro:
			should_wait = FALSE;

			/* Clear screen */
			Term_clear();

			/* Describe */
			Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
			//Term_putstr(25, 22, -1, TERM_L_UMBER, "[Press ESC to exit anytime]");
			Term_putstr(1, 4, -1, TERM_L_DARK, "Don't forget to save your macros with 's' when you are back in the macro menu!");
			Term_putstr(19, 5, -1, TERM_L_UMBER, "----------------------------------------");

			/* Initialize wizard state: First state */
			if (i == 'Z') { //shortcut
				i = 1;
				choice = mw_fileset;
			} else i = choice = 0;
			/* Paranoia */
			memset(tmp, 0, 160);
			memset(buf, 0, 1024);
			memset(buf2, 0, 1024);

			while (i != -1) {
				/* mw_fileset: Restart */
				if (i == 'Z') { //hack
					i = 1;
					choice = mw_fileset;
				}

				/* Restart wizard from a wrong choice? */
				if (i == -2) {
					/* Paranoia */
					memset(tmp, 0, 160);
					memset(buf, 0, 1024);
					memset(buf2, 0, 1024);
					/* Reinitialize */
					i = choice = chain_macro_buf[0] = tmp[0] = buf[0] = buf2[0] = 0;
					should_wait = FALSE;
				}

				/* Colour currently active step */
				Term_putstr(12, 1, -1, i == 0 ? TERM_L_GREEN : TERM_SLATE, "Step 1:  Choose an action for the macro to perform.");
				Term_putstr(12, 2, -1, i == 1 ? TERM_L_GREEN : TERM_SLATE, "Step 2:  If required, choose item, spell, and targetting method.");
				Term_putstr(12, 3, -1, i == 2 ? TERM_L_GREEN : TERM_SLATE, "Step 3:  Choose the key you want to bind the macro to.");

				clear_from(ystart);

				switch (i) {
				case 0:
					force_normal = FALSE;
					l = ystart;
					Term_putstr(1, l++, -1, TERM_GREEN, "Which of the following actions should the macro perform?");
					Term_putstr(8, l++, -1, TERM_L_GREEN, "a\377w/\377Gb) Drink a potion. \377w/\377G Read a scroll.");
					Term_putstr(8, l++, -1, TERM_L_GREEN, "c\377w/\377GC) Fire ranged weapon (including boomerangs). \377w/\377G Throw an item.");
					Term_putstr(8, l++, -1, TERM_L_GREEN, "d\377w/\377GD) Cast school \377w/\377G mimic spell without a target (or target manually).");
					Term_putstr(8, l++, -1, TERM_L_GREEN, "e\377w/\377GE) Cast school \377w/\377G mimic spell with target.");
					Term_putstr(8, l++, -1, TERM_L_GREEN, "f)   Cast a mimic spell by number (with and without target).");
					Term_putstr(8, l++, -1, TERM_L_GREEN, "g\377w/\377GG) Polymorph into monster. \377w/\377G Set preferred immunity (mimicry users).");
					Term_putstr(8, l++, -1, TERM_L_GREEN, "h\377w/\377GH) Draw runes to cast a runespell. \377w/\377G Set up a monster trap.");
					Term_putstr(8, l++, -1, TERM_L_GREEN, "i\377w/\377GI) Fighting technique. \377w/\377G Switch combat stance (most melee classes).");
					Term_putstr(8, l++, -1, TERM_L_GREEN, "j\377w/\377GJ) Shooting technique (archers, rangers). \377w/\377G Activate magic device.");
					Term_putstr(8, l++, -1, TERM_L_GREEN, "k\377w/\377GK) Use any item without \377w/\377G with a target).");
					Term_putstr(8, l++, -1, TERM_L_GREEN, "l\377w/\377GL) Use a basic ability ('m') without \377w/\377G with target (Draconian breath).");
					Term_putstr(8, l++, -1, TERM_L_GREEN, "m\377w/\377GM) Common commands and functions. \377w/\377G Pick breath element (Draconians).");
					Term_putstr(6, l++, -1, TERM_L_GREEN, "n\377w/\377GN\377w/\377GZ) Slash command. \377w/\377G Custom action ('%a'). \377w/\377G Chain existing macros.");
					Term_putstr(6, l++, -1, TERM_L_GREEN, "o\377w/\377GO\377w/\377Gp) Load a macro file. \377w/\377G Modify an option. \377w/\377G Change equipment.");
					Term_putstr(2, l++, -1, TERM_L_GREEN, "q\377w/\377Gr\377w/\377Gs\377w/\377Gt\377w/\377Gu) Directional running \377w/\377G tunneling \377w/\377G disarming \377w/\377G bashing \377w/\377G closing.");
#ifdef TEST_CLIENT
					Term_putstr(8, l++, -1, TERM_L_GREEN, "S)   Create a switchable multi-macrofile set.");
#endif

					while (TRUE) {
						/* Hack: Hide the cursor */
						Term->scr->cx = Term->wid;
						Term->scr->cu = 1;

						switch (choice = inkey()) {
						case ESCAPE:
						//case 'p': <-this is mw_equip now
						case '\010': /* backspace */
							i = -1; /* leave */
							break;
						case ':':
							/* specialty: allow chatting from within here (only in macro wizard step 1) */
							cmd_message();
							/* Restore top line */
							Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
							continue;
						case KTRL('T'):
							/* Take a screenshot */
							xhtml_screenshot("screenshot????", 2);
							continue;
						default:
							/* invalid action -> exit wizard */
							if ((choice < 'a' || choice > mw_LAST) &&
							    choice != 'C' && choice != 'D' && choice != 'E' &&
							    choice != 'G' && choice != 'H' && choice != 'I' && choice != 'J' &&
							    choice != 'K' && choice != 'L' && choice != 'M' && choice != 'N' && choice != 'O' &&
							    choice != 'Z'
#ifdef TEST_CLIENT
							    && choice != 'S'
#endif
							    ) {
								//i = -1;
								continue;
							}
						}
						break;
					}
					/* exit? */
					if (i == -1) continue;

					/* advance to next step */
					i++;
					break;
				case 1:
					l = ystart + 2;
					switch (choice) {
					case mw_quaff:
						Term_putstr(5, l++, -1, TERM_GREEN, "Please enter a distinctive part of the potion's name or inscription.");
						//Term_putstr(5, l++, -1, TERM_GREEN, "and pay attention to upper-case and lower-case letters!");
						Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377GCritical Wounds");
						Term_putstr(5, l++, -1, TERM_GREEN, "if you want to quaff a 'Potion of Cure Critical Wounds'.");
						l += 3;
						Term_putstr(5, l++, -1, TERM_L_GREEN, "Enter partial potion name or inscription:");
						break;

					case mw_read:
						Term_putstr(5, l++, -1, TERM_GREEN, "Please enter a distinctive part of the scroll's name or inscription.");
						//Term_putstr(5, l++, -1, TERM_GREEN, "and pay attention to upper-case and lower-case letters!");
						Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377GPhase Door");
						Term_putstr(5, l++, -1, TERM_GREEN, "if you want to read a 'Scroll of Phase Door'.");
						l += 3;
						Term_putstr(5, l++, -1, TERM_L_GREEN, "Enter partial scroll name or inscription:");
						should_wait = TRUE; /* recharge scrolls mostly; id/enchant scrolls.. */
						break;

					case mw_any:
						should_wait = TRUE; /* Just to be on the safe side, if the item picked is one that might require waiting (eg scroll of recharging in a chained macro). */
						__attribute__ ((fallthrough));
					case mw_anydir:
						Term_putstr(5, l++, -1, TERM_GREEN, "Please enter a distinctive part of the item's name or inscription.");
						//Term_putstr(5, l++, -1, TERM_GREEN, "and pay attention to upper-case and lower-case letters!");
						if (choice == mw_any) {
							Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377GRation");
							Term_putstr(5, l++, -1, TERM_GREEN, "if you want to use (eat) a 'Ration of Food'.");
						} else {
							/* actually a bit silyl here, we're not really using inscriptions but instead treat them as text -_-.. */
							Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377G@/0"); /* ..so 'correctly' this should just be '/0' :-p */
							Term_putstr(5, l++, -1, TERM_GREEN, "if you want to use (fire) a wand or rod inscribed '@/0'.");
						}
						l += 3;
						Term_putstr(5, l++, -1, TERM_L_GREEN, "Enter partial potion name or inscription:");
						break;

					case mw_schoolnt:
						sprintf(tmp, "return get_class_spellnt(%d)", p_ptr->pclass);
						strcpy(buf, string_exec_lua(0, tmp));
						Term_putstr(10, l++, -1, TERM_GREEN, "Please enter the exact spell name.");// and pay attention");
						//Term_putstr(10, l++, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, l++, -1, TERM_GREEN, format("For example, enter:     \377G%s", buf));
						Term_putstr(10, l++, -1, TERM_GREEN, "You must have learned a spell before you can use it!");
						l += 3;
						Term_putstr(15, l++, -1, TERM_L_GREEN, "Enter exact spell name:");
						should_wait = TRUE; /* identify/recharge spells */
						break;

					case mw_mimicnt:
						sprintf(tmp, "return get_class_mimicnt(%d)", p_ptr->pclass);
						strcpy(buf, string_exec_lua(0, tmp));
						Term_putstr(10, l++, -1, TERM_GREEN, "Please enter the exact spell name.");//and pay attention");
						//Term_putstr(10, l++, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, l++, -1, TERM_GREEN, format("For example, enter:     \377G%s", buf));
						Term_putstr(10, l++, -1, TERM_GREEN, "You must have learned a spell before you can use it!");
						l += 3;
						Term_putstr(15, l++, -1, TERM_L_GREEN, "Enter exact spell name:");
						break;

					case mw_schoolt:
						sprintf(tmp, "return get_class_spellt(%d)", p_ptr->pclass);
						strcpy(buf, string_exec_lua(0, tmp));
						Term_putstr(10, l++, -1, TERM_GREEN, "Please enter the exact spell name.");// and pay attention");
						//Term_putstr(10, l++, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, l++, -1, TERM_GREEN, format("For example, enter:     \377G%s", buf));
						Term_putstr(10, l++, -1, TERM_GREEN, "You must have learned a spell before you can use it!");
						l += 3;
						Term_putstr(15, l++, -1, TERM_L_GREEN, "Enter exact spell name:");
						break;

					case mw_mimict:
						sprintf(tmp, "return get_class_mimict(%d)", p_ptr->pclass);
						strcpy(buf, string_exec_lua(0, tmp));
						Term_putstr(10, l++, -1, TERM_GREEN, "Please enter the exact spell name.");// and pay attention");
						//Term_putstr(10, l++, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, l++, -1, TERM_GREEN, format("For example, enter:     \377G%s", buf));
						Term_putstr(10, l++, -1, TERM_GREEN, "You must have learned a spell before you can use it!");
						l += 3;
						Term_putstr(15, l++, -1, TERM_L_GREEN, "Enter exact spell name:");
						break;

					case mw_mimicidx:
						Term_putstr(10, l++, -1, TERM_GREEN, "Please enter a spell number, starting from 1, which is");
						Term_putstr(10, l++, -1, TERM_GREEN, "the first spell after the 3 basic powers and immunity");
						Term_putstr(10, l++, -1, TERM_GREEN, "preference setting, which always occupy spell slots a)-d).");
						Term_putstr(10, l++, -1, TERM_GREEN, "So \377G1\377g = spell e), \377G2\377g = f), \377G3\377g = g), \377G4\377g = h) etc.");
						Term_putstr(10, l++, -1, TERM_GREEN, "You must have learned a spell before you can use it!");
						l++;
						Term_putstr(15, l++, -1, TERM_L_GREEN, "Enter spell index number:");
						break;

					case mw_fight:
						Term_putstr(10, l++, -1, TERM_GREEN, "Please enter the exact technique name.");// and pay attention");
						//Term_putstr(10, l++, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, l++, -1, TERM_GREEN, "For example, enter:     \377GSprint");
						Term_putstr(10, l++, -1, TERM_GREEN, "You must have learned a technique before you can use it!");
						l += 3;
						Term_putstr(15, l++, -1, TERM_L_GREEN, "Enter exact technique name:");
						break;

					case mw_stance:
						Term_putstr(10, l++, -1, TERM_GREEN, "Please pick a stance:");
						Term_putstr(10, l++, -1, TERM_GREEN, "  \377Ga\377g) Balanced stance (standard)");
						Term_putstr(10, l++, -1, TERM_GREEN, "  \377Gb\377g) Defensive stance");
						Term_putstr(10, l++, -1, TERM_GREEN, "  \377Gc\377g) Offensive stance");
						/* Hack: Hide the cursor */
						Term->scr->cx = Term->wid;
						Term->scr->cu = 1;

						while (TRUE) {
							switch (choice = inkey()) {
							case ESCAPE:
							case 'p':
							case '\010': /* backspace */
								i = -2; /* leave */
								break;
							case ':': /* Allow chatting */
								cmd_message();
								/* Restore top line */
								Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
								continue;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????", 2);
								continue;
							default:
								/* invalid action -> exit wizard */
								if (choice < 'a' || choice > 'c') {
									//i = -1;
									continue;
								}
							}
							break;
						}
						/* exit? */
						if (i == -2) continue;

						buf[0] = choice;
						choice = mw_stance;
						break;

					case mw_shoot:
						Term_putstr(10, l++, -1, TERM_GREEN, "Please enter the exact technique name.");// and pay attention");
						//Term_putstr(10, l++, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, l++, -1, TERM_GREEN, "For example, enter:     \377GFlare Missile");
						Term_putstr(10, l++, -1, TERM_GREEN, "You must have learned a technique before you can use it!");
						l += 3;
						Term_putstr(15, l++, -1, TERM_L_GREEN, "Enter exact technique name:");
						break;

					case mw_poly:
						Term_putstr(10, l++, -1, TERM_GREEN, "Please enter the exact monster name OR its code. (You can find");
						Term_putstr(10, l++, -1, TERM_GREEN, "codes you have already learned by pressing  \377s~ 2 @  \377gin the game");
						Term_putstr(10, l++, -1, TERM_GREEN, "or by pressing  \377s:  \377gto chat and then typing the command:  \377s/mon @");
						Term_putstr(10, l++, -1, TERM_GREEN, "The first number on the left, in parentheses, is what you need.)");
						Term_putstr(10, l++, -1, TERM_GREEN, "For example, enter  \377GFruit bat\377g  or just  \377G37  \377gto transform into one.");
						Term_putstr(10, l++, -1, TERM_GREEN, "To return to the form you used before your current form, enter:  \377G-1\377g .");
						Term_putstr(10, l++, -1, TERM_GREEN, "To return to your normal form, use  \377GPlayer\377g  or its code  \377G0\377g  .");
						Term_putstr(10, l++, -1, TERM_GREEN, "To get asked about the form every time, just leave this blank.");
						//Term_putstr(10, l++, -1, TERM_GREEN, "You must have learned a form before you can use it!");
						l++;
						Term_putstr(1, l++, -1, TERM_L_GREEN, "Enter exact monster name/code or leave blank:");
						should_wait = TRUE;
						break;

					case mw_prfimm:
						Term_putstr(5, l++, -1, TERM_GREEN, "Please choose an immunity preference:");
						Term_putstr(5, l++, -1, TERM_GREEN, "\377Ga\377g) Just check (displays your current immunity preference)");
						Term_putstr(5, l++, -1, TERM_GREEN, "\377Gb\377g) None (pick one randomly on polymorphing)");
						Term_putstr(5, l++, -1, TERM_GREEN, "\377Gc\377g) Electricity  \377Gd\377g) Cold  \377Ge\377g) Fire  \377Gf\377g) Acid  \377Gg\377g) Poison  \377Gh\377g) Water");
						l += 2;
						Term_putstr(15, l++, -1, TERM_L_GREEN, "Pick one (a-h): ");

						while (TRUE) {
							switch (choice = inkey()) {
							case ESCAPE:
							case 'p':
							case '\010': /* backspace */
								i = -2; /* leave */
								break;
							case ':': /* Allow chatting */
								cmd_message();
								/* Restore top line */
								Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
								continue;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????", 2);
								continue;
							default:
								/* invalid action -> exit wizard */
								if (choice < 'a' || choice > 'h') {
									//i = -2;
									continue;
								}
							}
							break;
						}
						/* exit? */
						if (i == -2) continue;

						/* build macro part */
						switch (choice) {
						case 'a': strcpy(buf2, "\\e)m@3\rd@Check\r"); break;
						case 'b': strcpy(buf2, "\\e)m@3\rd@None\r"); break;
						case 'c': strcpy(buf2, "\\e)m@3\rd@Electricity\r"); break;
						case 'd': strcpy(buf2, "\\e)m@3\rd@Cold\r"); break;
						case 'e': strcpy(buf2, "\\e)m@3\rd@Fire\r"); break;
						case 'f': strcpy(buf2, "\\e)m@3\rd@Acid\r"); break;
						case 'g': strcpy(buf2, "\\e)m@3\rd@Poison\r"); break;
						case 'h': strcpy(buf2, "\\e)m@3\rd@Water\r"); break;
						}

						choice = mw_prfimm; /* hack - remember */
						break;

					case mw_rune: {
						strcpy(buf, "");
						u32b u = 0, u_prev[4] = { 0 };
						int step = 0;

						/* Hack: Hide the cursor */
						Term->scr->cx = Term->wid;
						Term->scr->cu = 1;

						/* Ask for a runespell? */
						while (!exec_lua(0, format("return rcraft_end(%d)", u))) {
							clear_from(ystart);
							Term_putstr(12, 22, -1, TERM_GREEN, "(Press Backspace to go back one step or Escape to quit)");
							exec_lua(0, format("return rcraft_prt(%d, %d)", u, 2));
							/* Hack: Hide the cursor */
							Term->scr->cx = Term->wid;
							Term->scr->cu = 1;
							switch (choice = inkey()) {
							case ESCAPE:
								i = -2; /* flag exit */
								break; /* exit switch */
							case '\010': /* backspace */
								if (!step) {
									i = -2; /* flag exit */
									break; /* exit switch */
								}
								u = u_prev[--step];
								buf[strlen(buf) - 1] = 0; /* remove newest char again from macro built so far*/
								continue;
							case ':': /* Allow chatting */
								cmd_message();
								/* Restore top line */
								Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
								break;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????", 2);
								break;
							default:
								i = (islower(choice) ? A2I(choice) : -1);
								if (i < 0 || i > exec_lua(0, format("return rcraft_max(%d)", u))) continue;
								u_prev[step++] = u;
								u |= exec_lua(0, format("return rcraft_bit(%d,%d)", u, i));
								strcat(buf, format("%c", choice)); /* build macro */
								break;
							}
							if (i == -2) break; /* exit while */
						}
						if (i == -2) continue; /* exit for */

						/* Ask for a direction? */
						if (exec_lua(0, format("return rcraft_dir(%d)", u)))
							strcat(buf, "*t");

						choice = mw_rune;
						i = 1;
						break; }

					case mw_trap:
						/* ---------- Enter trap kit name ---------- */
						Term_putstr(5, l++, -1, TERM_GREEN, "Please enter a distinctive part of the trap kit name or inscription.");
						//Term_putstr(5, l++, -1, TERM_GREEN, "and pay attention to upper-case and lower-case letters!");
						Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377GCatapult");
						Term_putstr(5, l++, -1, TERM_GREEN, "if you want to use a 'Catapult Trap Kit'.");
						l += 3;
						Term_putstr(5, l, -1, TERM_L_GREEN, "Enter partial trap kit name or inscription:");

						/* Get an item name */
						Term_gotoxy(50, l);
						strcpy(buf, "");
						if (!askfor_aux(buf, 159, 0)) {
							i = -2;
							continue;
						}
						strcat(buf, "\\r");

						/* ---------- Enter ammo/load name ---------- */
						l = ystart + 2;
						clear_from(l);
						Term_putstr(5, l++, -1, TERM_GREEN, "Please enter a distinctive part of the item name or inscription you");
						Term_putstr(5, l++, -1, TERM_GREEN, "want to load the trap kit with.");//, and pay attention to upper-case");
						//Term_putstr(5, l++, -1, TERM_GREEN, "and lower-case letters!");
						Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377GPebbl     \377gif you want");
						Term_putstr(5, l++, -1, TERM_GREEN, "to load a catapult trap kit with 'Rounded Pebbles'.");
						Term_putstr(5, l++, -1, TERM_GREEN, "If you want to choose ammo manually, just press the \377GRETURN\377g key.");
						l += 2;
						Term_putstr(5, l, -1, TERM_L_GREEN, "Enter partial ammo/load name or inscription:");

						/* Get an item name */
						Term_gotoxy(50, l);
						strcpy(buf2, "");
						if (!askfor_aux(buf2, 159, 0)) {
							i = -2;
							continue;
						}
						/* Choose ammo manually? Terminate partial macro here. */
						if (streq(buf2, "")) break;

						strcat(buf2, "\\r");
						strcat(buf, "@");
						strcat(buf, buf2);
						break;

					case mw_device:
						Term_putstr(10, l++, -1, TERM_GREEN, "Please choose the type of magic device you want to use:");
						l++;
						Term_putstr(15, l++, -1, TERM_L_GREEN, "a) a wand");
						Term_putstr(15, l++, -1, TERM_L_GREEN, "b) a staff");
						Term_putstr(15, l++, -1, TERM_L_GREEN, "c) a rod that doesn't require a target");
						Term_putstr(15, l++, -1, TERM_L_GREEN, "d) a rod that requires a target");
						Term_putstr(15, l++, -1, TERM_L_GREEN, "e) an activatable item that doesn't require a target");
						Term_putstr(15, l++, -1, TERM_L_GREEN, "f) an activatable item that requires a target");

						/* Hack: Hide the cursor */
						Term->scr->cx = Term->wid;
						Term->scr->cu = 1;

						while (TRUE) {
							switch (choice = inkey()) {
							case ESCAPE:
							case 'p':
							case '\010': /* backspace */
								i = -2; /* leave */
								break;
							case ':': /* Allow chatting */
								cmd_message();
								/* Restore top line */
								Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
								continue;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????", 2);
								continue;
							default:
								/* invalid action -> exit wizard */
								if (choice < 'a' || choice > 'f') {
									//i = -1;
									continue;
								}
							}
							break;
						}
						/* exit? */
						if (i == -2) continue;

						/* build macro part */
						j = 0; /* hack: != 1 means 'undirectional' device */
						if (c_cfg.rogue_like_commands) {
							switch (choice) {
							case 'a': strcpy(buf2, "\\e)z@"); j = 1; break;
							case 'b': strcpy(buf2, "\\e)Z@"); should_wait = TRUE; /* For staves of perception */ break;
							case 'c': strcpy(buf2, "\\e)a@"); should_wait = TRUE; /* For rods of perception */ break;
							case 'd': strcpy(buf2, "\\e)a@"); j = 1; break;
							case 'e': strcpy(buf2, "\\e)A@"); should_wait = TRUE; /* For eg stone of lore */ break;
							case 'f': strcpy(buf2, "\\e)A@"); j = 1; break;
							}
						} else {
							switch (choice) {
							case 'a': strcpy(buf2, "\\e)a@"); j = 1; break;
							case 'b': strcpy(buf2, "\\e)u@"); should_wait = TRUE; /* For staves of perception */ break;
							case 'c': strcpy(buf2, "\\e)z@"); should_wait = TRUE; /* For rods of perception */ break;
							case 'd': strcpy(buf2, "\\e)z@"); j = 1; break;
							case 'e': strcpy(buf2, "\\e)A@"); should_wait = TRUE; /* For eg stone of lore */ break;
							case 'f': strcpy(buf2, "\\e)A@"); j = 1; break;
							}
						}

						/* ---------- Enter device name ---------- */

						l = ystart + 2;
						clear_from(ystart);
						Term_putstr(5, l++, -1, TERM_GREEN, "Please enter a distinctive part of the magic device's name or");
						Term_putstr(5, l++, -1, TERM_GREEN, "inscription.");// and pay attention to upper-case and lower-case letters!");
						switch (choice) {
						case 'a': Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377GMagic Mis");
							Term_putstr(5, l++, -1, TERM_GREEN, "if you want to use a 'Wand of Magic Missiles'.");
							break;
						case 'b': Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377GTelep");
							Term_putstr(5, l++, -1, TERM_GREEN, "if you want to use a 'Staff of Teleportation'.");
							break;
						case 'c': Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377GTrap Loc");
							Term_putstr(5, l++, -1, TERM_GREEN, "if you want to use a 'Rod of Trap Location'.");
							break;
						case 'd': Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377GLightn");
							Term_putstr(5, l++, -1, TERM_GREEN, "if you want to use a 'Rod of Lightning Bolts'.");
							break;
						case 'e': Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377GFrostw");
							Term_putstr(5, l++, -1, TERM_GREEN, "if you want to use a 'Frostwoven Cloak'.");
							break;
						case 'f': Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377GSerpen");
							Term_putstr(5, l++, -1, TERM_GREEN, "if you want to use an 'Amulet of the Serpents'.");
							break;
						}
						l += 2;
						Term_putstr(5, l++, -1, TERM_L_GREEN, "Enter partial device name or inscription:");

						/* hack before we exit: remember menu choice 'magic device' */
						choice = mw_device;
						break;

					case mw_abilitynt:
						Term_putstr(10, l++, -1, TERM_GREEN, "Please enter the exact ability name.");// and pay attention");
						//Term_putstr(10, l++, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, l++, -1, TERM_GREEN, "For example, enter:     \377GSwitch between main-hand and dual-hand");
						l += 4;
						Term_putstr(15, l++, -1, TERM_L_GREEN, "Enter exact ability name:");
						break;
					case mw_abilityt:
						Term_putstr(10, l++, -1, TERM_GREEN, "Please enter the exact ability name.");// and pay attention");
						//Term_putstr(10, l++, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, l++, -1, TERM_GREEN, "For example, enter:     \377GBreathe element");
						l += 4;
						Term_putstr(15, l++, -1, TERM_L_GREEN, "Enter exact ability name:");
						break;

					case mw_throw:
						Term_putstr(5, l++, -1, TERM_GREEN, "Please enter a distinctive part of the item's name or inscription.");
						//Term_putstr(5, l++, -1, TERM_GREEN, "and pay attention to upper-case and lower-case letters!");
						Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377G{bad}");
						Term_putstr(5, l++, -1, TERM_GREEN, "if you want to throw any item that is inscribed '{bad}'.");
						Term_putstr(5, l++, -1, TERM_GREEN, "(That can for example give otherwise useless potions some use..)");
						l += 2;
						Term_putstr(5, l++, -1, TERM_L_GREEN, "Enter partial item name or inscription:");
						break;

					case mw_slash:
						Term_putstr(5, l++, -1, TERM_GREEN, "Please enter the complete slash command.");
						Term_putstr(5, l++, -1, TERM_GREEN, "For example, enter:     \377G/cough");
						l += 4;
						Term_putstr(5, l++, -1, TERM_L_GREEN, "Enter a slash command:");
						break;

					case mw_custom:
						Term_putstr(5, l++, -1, TERM_GREEN, "Please enter the custom macro action string.");
						Term_putstr(5, l++, -1, TERM_GREEN, "(You have to specify everything manually here, and won't get");
						Term_putstr(5, l++, -1, TERM_GREEN, "prompted about a targetting method or anything else either.)");
						l += 3;
						Term_putstr(5, l++, -1, TERM_L_GREEN, "Enter a new action:");
						break;

					case mw_load:
						Term_putstr(5, l++, -1, TERM_GREEN, "Please enter the macro file name.");
						Term_putstr(5, l++, -1, TERM_GREEN, "If you are on Linux or OSX it is case-sensitive! On Windows it is not.");
						Term_putstr(5, l++, -1, TERM_GREEN, format("For example, enter:     \377G%s.prf", cname));
						l += 3;
						Term_putstr(5, l++, -1, TERM_L_GREEN, "Exact file name:");
						break;

					case mw_option:
						Term_putstr(5, l++, -1, TERM_GREEN, "Do you want to enable, disable or toggle (flip) an option?");
						Term_putstr(5, l++, -1, TERM_L_GREEN, "    y\377g) enable an option");
						Term_putstr(5, l++, -1, TERM_L_GREEN, "    n\377g) disable an option");
						Term_putstr(5, l++, -1, TERM_L_GREEN, "    t\377g) toggle an option");
						Term_putstr(5, l++, -1, TERM_L_GREEN, "    Y\377g) enable an option and display feedback message");
						Term_putstr(5, l++, -1, TERM_L_GREEN, "    N\377g) disable an option and display feedback message");
						Term_putstr(5, l++, -1, TERM_L_GREEN, "    T\377g) toggle an option and display feedback message");
						/* hack: hide cursor */
						Term->scr->cx = Term->wid;
						Term->scr->cu = 1;

						while (TRUE) {
							switch (choice = inkey()) {
							case ESCAPE:
							case 'p':
							case '\010': /* backspace */
								i = -2; /* leave */
								break;
							case ':': /* Allow chatting */
								cmd_message();
								/* Restore top line */
								Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
								continue;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????", 2);
								continue;
							case 'y': case 'n': case 't':
							case 'Y': case 'N': case 'T':
								break;
							default:
								continue;
							}
							break;
						}
						/* exit? */
						if (i == -2) continue;

						l = ystart + 2;
						clear_from(l);
						Term_putstr(5, l++, -1, TERM_GREEN, "Now please enter the exact name of an option, for example: \377Gbig_map");
						l++;
						Term_putstr(5, l++, -1, TERM_L_GREEN, "Enter exact option name: ");

						j = choice;
						choice = mw_option;
						break;

					case mw_equip:
						Term_putstr(5, l++, -1, TERM_GREEN, "Do you want to wear/wield, take off or swap an item?");
						Term_putstr(5, l++, -1, TERM_L_GREEN, "    w\377g) primary wear/wield");
						Term_putstr(5, l++, -1, TERM_L_GREEN, "    W\377g) secondary wear/wield");
						if (c_cfg.rogue_like_commands) {
							Term_putstr(5, l++, -1, TERM_L_GREEN, "    T\377g) take off an item");
							Term_putstr(5, l++, -1, TERM_L_GREEN, "    S\377g) swap item(s)");
						} else {
							Term_putstr(5, l++, -1, TERM_L_GREEN, "    t\377g) take off an item");
							Term_putstr(5, l++, -1, TERM_L_GREEN, "    x\377g) swap item(s)");
						}
						Term_putstr(5, l++, -1, TERM_L_GREEN, "    d\377g) invoke '/dress' command (optionally with a tag)");
						Term_putstr(5, l++, -1, TERM_L_GREEN, "    b\377g) invoke '/bed' command (optionally with a tag)");
						l++;
						Term_putstr(5, l++, -1, TERM_GREEN, "Note: This macro depends on your current 'rogue_like_commands' option");
						Term_putstr(5, l++, -1, TERM_GREEN, "      setting and will not work anymore if you change the keyset.");
						/* Hack: Hide the cursor */
						Term->scr->cx = Term->wid;
						Term->scr->cu = 1;

						if (c_cfg.rogue_like_commands) {
							while (TRUE) {
								switch (choice = inkey()) {
								case ESCAPE:
								case 'p':
								case '\010': /* backspace */
									i = -2; /* leave */
									break;
								case ':': /* Allow chatting */
									cmd_message();
									/* Restore top line */
									Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
									continue;
								case KTRL('T'):
									/* Take a screenshot */
									xhtml_screenshot("screenshot????", 2);
									continue;
								case 'w':
								case 'W':
								case 'T':
								case 'S':
									break;
								case 'd':
								case 'b':
									break;
								default:
									continue;
								}
								break;
							}
						} else {
							while (TRUE) {
								switch (choice = inkey()) {
								case ESCAPE:
								case 'p':
								case '\010': /* backspace */
									i = -2; /* leave */
									break;
								case ':': /* Allow chatting */
									cmd_message();
									/* Restore top line */
									Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
									continue;
								case KTRL('T'):
									/* Take a screenshot */
									xhtml_screenshot("screenshot????", 2);
									continue;
								case 'w':
								case 'W':
								case 't':
								case 'x':
									break;
								case 'd':
								case 'b':
									break;
								default:
									continue;
								}
								break;
							}
						}
						/* exit? */
						if (i == -2) continue;

						l = ystart + 2;
						clear_from(l);
						Term_putstr(5, l++, -1, TERM_GREEN, "Please enter a distinctive part of the item's name or inscription.");
						l += 5;
						Term_putstr(5, l++, -1, TERM_L_GREEN, "Enter partial item name or inscription:");

						j = choice;
						choice = mw_equip;
						if (j != 'd' && j != 'b') should_wait = TRUE; /* /dress and /bed don't need waiting for execution, as they are purely server-side */
						break;

					case mw_common:
						force_normal = FALSE;
						l = ystart;
						//Term_putstr(10, l++, -1, TERM_GREEN, "Please choose one of these common commands and functions:"); --make room for one more entry instead
						Term_putstr(2, l++, -1, TERM_L_GREEN, "a) reply to last incoming whisper                            :+:");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "b) repeat previous chat command or message                   :^P\\r");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "c) toggle AFK state                                          :/afk\\r");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "d) word-of-recall (scroll/rod/spellbook must be inscribed '@R')");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "e) cough (reduces sleep of monsters nearby)                  :/cough\\r");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "f) shout (breaks sleep of monsters nearby)                   :/shout\\r");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "g) shooting, ammunition, trapping, throwing");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "h) prompt for a guide quick search                           :/? ");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "i) swap two items (eg inventory and equipment) or equip/unequip one item");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "j) enter/leave the PvP arena (PvP mode only)                 :/pvp\\r");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "k) use an item inscribed '@/1' (w/ or w/o target)            \\e)*t/1-");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "l) steal from shop more of the last interacted-with item     Z+\\wNN");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "m) display some extra information                            :/ex\\r");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "n) display in-game time (daylight is 6am-10pm)               :/time\\r");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "o) cast spell 'a)' from a book '@m1' (w/ or w/o target)      \\e)*tm@11\\r1a-");
						Term_putstr(2, l++, -1, TERM_L_GREEN, "p) cast spell 'a)' from a book '@m2' (w/ or w/o target)      \\e)*tm@11\\r2a-");
						/* Hack: Hide the cursor */
						Term->scr->cx = Term->wid;
						Term->scr->cu = 1;

						while (TRUE) {
							switch (choice = inkey()) {
							case ESCAPE:
							//case 'p': -- we have a) to p) available..
							case '\010': /* backspace */
								i = -2; /* leave */
								break;
							case ':': /* Allow chatting */
								cmd_message();
								/* Restore top line */
								Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
								continue;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????", 2);
								continue;
							default:
								/* invalid action -> exit wizard */
								if (choice < 'a' || choice > 'p') {
									//i = -1;
									continue;
								}
							}
							break;
						}
						/* exit? */
						if (i == -2) continue;

						/* build macro part */
						switch (choice) {
						case 'a': strcpy(buf2, ":+:"); break;
						case 'b': strcpy(buf2, ":^P\\r"); break;
						case 'c': strcpy(buf2, ":/afk\\r"); break;
						case 'd':
							while (TRUE) {
								clear_from(ystart);
								l = ystart + 2;
								Term_putstr(1, l++, -1, TERM_GREEN, "Please choose a type of word-of-recall:");
								l++;
								Term_putstr(1, l++, -1, TERM_L_GREEN, "a) just basic word-of-recall (in to max depth / back out again)  :/rec\\r");
								Term_putstr(1, l++, -1, TERM_L_GREEN, "b) recall to a specific, fixed depth (or back out again)         :/rec d\\r");
								Term_putstr(1, l++, -1, TERM_L_GREEN, "c) world-travel recall, ie recall across the world surface       :/rec x y\\r");
								Term_putstr(1, l++, -1, TERM_L_GREEN, "d) world-travel recall, specifically to Bree, aka (32,32)        :/rec 32 32\\r");
								Term_putstr(1, l++, -1, TERM_L_GREEN, "e) Manual recall, will prompt you for destination each time      :/rec ");
								while (TRUE) {
									switch (choice = inkey()) {
									case ESCAPE:
									case 'p':
									case '\010': /* backspace */
										i = -2; /* leave */
										break;
									case ':': /* Allow chatting */
										cmd_message();
										/* Restore top line */
										Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
										continue;
									case KTRL('T'):
										/* Take a screenshot */
										xhtml_screenshot("screenshot????", 2);
										continue;
									default:
										/* invalid action -> exit wizard */
										if (choice < 'a' || choice > 'e') {
											//i = -1;
											continue;
										}
									}
									break;
								}
								/* exit? */
								if (i == -2) {
									/* hack before we exit: remember menu choice 'common' */
									choice = mw_common;
									break;
								}

								l++;
								switch (choice) {
								case 'a': strcpy(buf2, ":/rec\\r"); break;
								case 'b':
									Term_putstr(5, l, -1, TERM_L_GREEN, "Enter a specific depth (eg '-500'): ");
									strcpy(buf, "");
									if (!askfor_aux(buf, 6, 0)) continue;
									strcpy(buf2, ":/rec ");
									strcat(buf2, buf);
									strcat(buf2, "\\r");
									break;
								case 'c':
									Term_putstr(5, l, -1, TERM_L_GREEN, "Enter world coordinates separated by space (eg '32 32'): ");
									strcpy(buf, "");
									if (!askfor_aux(buf, 6, 0)) continue;
									strcpy(buf2, ":/rec ");
									strcat(buf2, buf);
									strcat(buf2, "\\r");
									break;
								case 'd': strcpy(buf2, ":/rec 32 32\\r"); break;
								case 'e': strcpy(buf2, ":/rec "); break;
								}
								break;
							}
							break;
						case 'e': strcpy(buf2, ":/cough\\r"); break;
						case 'f': strcpy(buf2, ":/shout\\r"); break;
						case 'g':
							while (TRUE) {
								clear_from(ystart);
								l = ystart + 2;
								Term_putstr(2, l++, -1, TERM_GREEN, "Select one of the following:");
								Term_putstr(2, l++, -1, TERM_L_GREEN, "a) fire equipped shooter at closest enemy                    \\e)*tf-");
								Term_putstr(2, l++, -1, TERM_L_GREEN, "b) inscribe ammo/trap payload to auto pickup+merge+load+@m0  {-!=LM@m0\\r");
								Term_putstr(2, l++, -1, TERM_L_GREEN, "c) throw an item inscribed '@v1' at closest enemy            \\e)*tv1-");
								Term_putstr(2, l++, -1, TERM_L_GREEN, "d) inscribe throwing weapon to auto pickup+equip+@v1         {-!=L@v1\\r");
								Term_putstr(2, l++, -1, TERM_L_GREEN, "e) throw an item tagged {bad} at closest monster             \\e)*tv@{bad}\\r-");
								Term_putstr(2, l++, -1, TERM_L_GREEN, "f) quick-toggle option 'easy_disarm_montraps'                :/edmt\\r");

								while (TRUE) {
									switch (choice = inkey()) {
									case ESCAPE:
									case 'p':
									case '\010': /* backspace */
										i = -2; /* leave */
										break;
									case ':': /* Allow chatting */
										cmd_message();
										/* Restore top line */
										Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
										continue;
									case KTRL('T'):
										/* Take a screenshot */
										xhtml_screenshot("screenshot????", 2);
										continue;
									default:
										/* invalid action -> exit wizard */
										if (choice < 'a' || choice > 'f') {
											//i = -1;
											continue;
										}
									}
									break;
								}
								/* exit? */
								if (i == -2) {
									/* hack before we exit: remember menu choice 'common' */
									choice = mw_common;
									break;
								}

								l++;
								switch (choice) {
								case 'a':
									if (c_cfg.rogue_like_commands) strcpy(buf2, "\\e)*tt-");
									else strcpy(buf2, "\\e)*tf-");
									break;
								case 'b': strcpy(buf2, "{-!=LM@m0\\r"); break;
								case 'c': strcpy(buf2, "\\e)*tv1-"); break;
								case 'd': strcpy(buf2, "{-!=L@v1\\r"); break;
								case 'e': strcpy(buf2, "\\e)*tv@{bad}\r-"); break;
								case 'f': strcpy(buf2, ":/edmt\\r"); break;
								}
								break;
							}
							break;
						case 'h': strcpy(buf2, ":/? "); break;
						case 'i':
							while (TRUE) {
								clear_from(ystart);
								l = ystart + 2;
								Term_putstr(10, l++, -1, TERM_GREEN, "Pick one item-swap inscription:");
								l++;

								if (c_cfg.rogue_like_commands) {
									Term_putstr(11, l++, -1, TERM_L_GREEN, "a) swap two items that you inscribed '@x0'         \\e)S0");
									Term_putstr(11, l++, -1, TERM_L_GREEN, "b) swap two items that you inscribed '@x1'         \\e)S1");
									Term_putstr(11, l++, -1, TERM_L_GREEN, "c) swap two items that you inscribed '@x2'         \\e)S2");
									Term_putstr(11, l++, -1, TERM_L_GREEN, "d) swap two items that you inscribed '@x3'         \\e)S3");
								} else {
									Term_putstr(11, l++, -1, TERM_L_GREEN, "a) swap two items that you inscribed '@x0'         \\e)x0");
									Term_putstr(11, l++, -1, TERM_L_GREEN, "b) swap two items that you inscribed '@x1'         \\e)x1");
									Term_putstr(11, l++, -1, TERM_L_GREEN, "c) swap two items that you inscribed '@x2'         \\e)x2");
									Term_putstr(11, l++, -1, TERM_L_GREEN, "d) swap two items that you inscribed '@x3'         \\e)x3");
								}

								while (TRUE) {
									switch (choice = inkey()) {
									case ESCAPE:
									case 'p':
									case '\010': /* backspace */
										i = -2; /* leave */
										break;
									case ':': /* Allow chatting */
										cmd_message();
										/* Restore top line */
										Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
										continue;
									case KTRL('T'):
										/* Take a screenshot */
										xhtml_screenshot("screenshot????", 2);
										continue;
									default:
										/* invalid action -> exit wizard */
										if (choice < 'a' || choice > 'd') {
											//i = -1;
											continue;
										}
									}
									break;
								}
								/* exit? */
								if (i == -2) {
									/* hack before we exit: remember menu choice 'common' */
									choice = mw_common;
									break;
								}

								l++;
								switch (choice) {
								case 'a':
									if (c_cfg.rogue_like_commands) strcpy(buf2, "\\e)S0");
									else strcpy(buf2, "\\e)x0");
									break;
								case 'b':
									if (c_cfg.rogue_like_commands) strcpy(buf2, "\\e)S1");
									else strcpy(buf2, "\\e)x1");
									break;
								case 'c':
									if (c_cfg.rogue_like_commands) strcpy(buf2, "\\e)S2");
									else strcpy(buf2, "\\e)x2");
									break;
								case 'd':
									if (c_cfg.rogue_like_commands) strcpy(buf2, "\\e)S3");
									else strcpy(buf2, "\\e)x3");
									break;
								}
								break;
							}
							break;
						case 'j': strcpy(buf2, ":/pvp\\r"); break;
						case 'k': strcpy(buf2, "\\e)*t/1-"); break;
						case 'l': {
							int delay, num = 1;

							clear_from(ystart);
							l = ystart + 1;
							Term_putstr(2, l++, -1, TERM_GREEN, "Steal item(s) from shop, from the same stock slot you last interacted with:");
							Term_putstr(4, l++, -1, TERM_GREEN, "As long as the latency-delay matches, the macro will stop automatically");
							Term_putstr(4, l++, -1, TERM_GREEN, "after stealing the last item in the store slot.");
							l++;
							Term_putstr(10, l, -1, TERM_YELLOW, "Steal up to how many (1-20)?");
							/* default: suggest to just steal 1 item at a time */
							sprintf(tmp, "1");
							Term_gotoxy(40, l);
							if (askfor_aux(tmp, 50, 0)) {
								num = atoi(tmp);
								if (num < 1) num = 1;
								if (num > 20) num = 20;
							}
							l += 2;

							Term_putstr(10, l++, -1, TERM_YELLOW, "This macro needs a latency-based delay to work properly!");
							Term_putstr(10, l++, -1, TERM_YELLOW, "You can accept the suggested delay or modify it in steps");
							Term_putstr(10, l++, -1, TERM_YELLOW, "of 100 ms up to 9900 ms, or hit ESC to not use a delay.");
							Term_putstr(10, l, -1, TERM_L_GREEN, "ENTER\377g to accept, \377GESC\377g to discard (in ms):");

							/* suggest +25% reserve tolerance but at least +25 ms on the ping time */
							sprintf(tmp, "%d", ((ping_avg < 100 ? ping_avg + 25 : (ping_avg * 125) / 100) / 100 + 1) * 100);
							Term_gotoxy(52, l);
							if (askfor_aux(tmp, 50, 0)) {
								delay = atoi(tmp);
								if (delay % 100) delay += 100; //QoL hack for noobs who can't read
								delay /= 100;
								if (delay < 0) delay = 0;
								if (delay > 99) delay = 99;

								if (delay) sprintf(tmp, "\\w%c%c", '0' + delay / 10, '0' + delay % 10);
							}

							*buf2 = 0;
							while (num--) strcat(buf2, delay ? format("Z+%s", tmp) : "Z+");
							force_normal = TRUE;
							break; }
						case 'm': strcpy(buf2, ":/ex\\r"); break;
						case 'n': strcpy(buf2, ":/time\\r"); break;
						case 'o': strcpy(buf2, "\\e)*tm@11\\r1a-"); break;
						case 'p': strcpy(buf2, "\\e)*tm@11\\r2a-"); break;
						}

						/* hack before we exit: remember menu choice 'common' */
						choice = mw_common;
						/* exit? */
						if (i == -2) continue;

						break;

					case mw_prfele:
						Term_putstr(5, l++, -1, TERM_GREEN, "Please choose an elemental preference:");
						Term_putstr(5, l++, -1, TERM_GREEN, "\377Ga\377g) Just check (displays your current elemental preference)");
						Term_putstr(5, l++, -1, TERM_GREEN, "\377Gb\377g) None (random)");
						Term_putstr(5, l++, -1, TERM_GREEN, "\377Gc\377g) Lightning  \377Gd\377g) Frost  \377Ge\377g) Fire  \377Gf\377g) Acid  \377Gg\377g) Poison");
						l += 2;
						Term_putstr(15, l++, -1, TERM_L_GREEN, "Pick one (a-g): ");

						while (TRUE) {
							switch (choice = inkey()) {
							case ESCAPE:
							case 'p':
							case '\010': /* backspace */
								i = -2; /* leave */
								break;
							case ':': /* Allow chatting */
								cmd_message();
								/* Restore top line */
								Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
								continue;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????", 2);
								continue;
							default:
								/* invalid action -> exit wizard */
								if (choice < 'a' || choice > 'g') {
									//i = -2;
									continue;
								}
							}
							break;
						}
						/* exit? */
						if (i == -2) continue;

						/* build macro part */
						switch (choice) {
						case 'a': strcpy(buf2, "\\e)m@19\r@Check\r"); break;
						case 'b': strcpy(buf2, "\\e)m@19\r@None\r"); break;
						case 'c': strcpy(buf2, "\\e)m@19\r@Lightning\r"); break;
						case 'd': strcpy(buf2, "\\e)m@19\r@Frost\r"); break;
						case 'e': strcpy(buf2, "\\e)m@19\r@Fire\r"); break;
						case 'f': strcpy(buf2, "\\e)m@19\r@Acid\r"); break;
						case 'g': strcpy(buf2, "\\e)m@19\r@Poison\r"); break;
						}

						choice = mw_prfele; /* hack - remember */
						break;

					case mw_dir_bash:
						/* can be shield-bash attack, so we need to call the target-selector */
						if (c_cfg.rogue_like_commands) strcpy(buf2, "\\e)f*t");
						else strcpy(buf2, "\\e)B*t");
						break;
					case mw_dir_run:
					case mw_dir_tunnel:
					case mw_dir_disarm:
					case mw_dir_close:
						clear_from(ystart);
						Term_putstr(10, l++, -1, TERM_GREEN, "Please pick the specific, fixed direction:");
						l += 2;
						Term_putstr(25, l++, -1, TERM_L_GREEN, " 7  8  9");
						Term_putstr(25, l++, -1, TERM_GREEN, "  \\ | /");
						Term_putstr(25, l++, -1, TERM_L_GREEN, "4 \377g-\377G 5 \377g-\377G 6");
						Term_putstr(25, l++, -1, TERM_GREEN, "  / | \\");
						Term_putstr(25, l++, -1, TERM_L_GREEN, " 1  2  3");
						l += 2;
						Term_putstr(15, l++, -1, TERM_L_GREEN, "Your choice? (1 to 9) ");

						while (TRUE) {
							target_dir = inkey();
							switch (target_dir) {
							case ESCAPE:
							case 'p':
							case '\010': /* backspace */
								i = -2; /* leave */
								break;
							case ':': /* Allow chatting */
								cmd_message();
								/* Restore top line */
								Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
								continue;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????", 2);
								continue;
							default:
								/* invalid action -> exit wizard */
								if (target_dir < '1' || target_dir > '9') {
									//i = -3;
									continue;
								}
							}
							break;
						}

						/* exit? */
						if (i == -2) continue;

						/* This is the default start for running-macros, instead of "\e)", but it probably doesn't matter..
						   And since we're lazy we just use it for all of these minor functions here below. */
						strcpy(buf2, "\\e\\e\\\\");

						switch (choice) {
						case mw_dir_run:
							strcat(buf2, ".");
							break;
						case mw_dir_tunnel:
							strcat(buf2, "+"); //actually same for rogue_like or normal, just normal also offers 'T'
							break;
						case mw_dir_disarm:
							strcat(buf2, "D"); //actually same for rogue_like or normal
							break;
						case mw_dir_bash:
							if (c_cfg.rogue_like_commands) strcat(buf2, "f");
							else strcat(buf2, "B");
							break;
						case mw_dir_close:
							strcat(buf2, "c"); //actually same for rogue_like or normal
							break;
						}
						strcat(buf2, format("%c", target_dir));
						break;
					case mw_chain: {
						char tmp[1024], buf[64];
						bool bind = FALSE;

						while (TRUE) {
							clear_from(ystart);
							Term_putstr(10, l++, -1, TERM_GREEN, "Please press the key carrying the macro you want to chain.");
							Term_putstr(10, l++, -1, TERM_GREEN, "Pressing ESC will cancel and quit the macro-chaining process.");
							l++;
							Term_putstr(10, l++, -1, TERM_L_GREEN, "Trigger: ");

							get_macro_trigger(buf);

							if (buf[0] == ESCAPE && !buf[1]) {
								c_msg_print("\377yMacro-chaining cancelled.");
								i = -2;
								break;
							} else if (buf[0] == '%' && !buf[1]) {
								c_msg_print("\377yThe '%' key cannot have any macros on it. Please try again.");
								continue;
							}

							/* Re-using 'i' here shouldn't matter anymore */
							for (i = 0; i < macro__num; i++) {
								if (!streq(macro__pat[i], buf)) continue;

								strncpy(macro__buf, macro__act[i], 159);
								macro__buf[159] = '\0';

								/* Message */
								ascii_to_text(tmp, macro__buf);
								if (macro__hyb[i]) {
									Term_putstr(10, 15, -1, TERM_L_GREEN, "Found hybrid macro:");
									Term_putstr(10, 16, -1, TERM_L_BLUE, format("%s", tmp));
									if (chain_type < 1) chain_type = 1;
									break;
								} else if (macro__cmd[i]) {
									Term_putstr(10, 15, -1, TERM_L_GREEN, "Found command macro:");
									Term_putstr(10, 16, -1, TERM_L_BLUE, format("%s", tmp));
									break;
								} else {
									if (!macro__act[i][0]) c_msg_print("Found empty macro.");
									else {
										Term_putstr(10, 15, -1, TERM_L_GREEN, "Found normal macro:");
										Term_putstr(10, 16, -1, TERM_L_BLUE, format("%s", tmp));
										if (chain_type < 2) chain_type = 2;
										break;
									}
								}
							}
							if (i == macro__num) {
								c_msg_print("\377yNo valid macro found, please try another key.");
								continue;
							}
							break;
						}
						/* exit? */
						if (i == -2) continue;

						/* Chain */
						if (chain_macro_buf[0] && !strncmp(macro__buf, "\e)", 2)) { /* Skip subsequent ')' keybuffer clearing, as it would cancel the previous macro if the player doesn't have enough energy to execute it right now! */
							strcat(chain_macro_buf, "\e");
							strcat(chain_macro_buf, macro__buf + 2);
						} else
						strcat(chain_macro_buf, macro__buf);
						strcpy(macro__buf, chain_macro_buf);

						/* Echo it for convenience, to check */
						ascii_to_text(tmp, chain_macro_buf);
						c_msg_format("\377yChain: %s", tmp);

						/* max length check, rough estimate */
						if (strlen(chain_macro_buf) + strlen(macro__buf) >= 1024 - 50) {
							c_msg_print("\377oDue to excess length you cannot chain any further commands to this macro.");
							bind = TRUE;
						}
						/* Ask if we want to bind it to a key or continue chaining stuff */
						l += 4;
						l2 = l;
						Term_putstr(10, l++, -1, TERM_GREEN, "Press the key to bind the macro to. (ESC and % key cannot be used.)");
						Term_putstr(10, l++, -1, TERM_GREEN, "Most keys can be combined with \377USHIFT\377g, \377UALT\377g or \377UCTRL\377g modifiers!");
						if (!bind) {
							Term_putstr(10, l++, -1, TERM_GREEN, "If you want to \377Uchain another macro\377g, press '\377U%\377g' key.");
							Term_putstr(5, l, -1, TERM_L_GREEN, "Press the key to bind the macro to, or '%' for chaining: ");
						} else {
							l++;
							Term_putstr(5, l, -1, TERM_L_GREEN, "Press the key to bind the macro to: ");
						}

						while (TRUE) {
							/* Get a macro trigger */
							Term_putstr(67, l, -1, TERM_WHITE, "  ");//45
							Term_gotoxy(67, l);
							get_macro_trigger(buf);

							/* choose proper macro type, and bind it to key */
							if (!strcmp(buf, "\e")) {
								c_msg_print("\377yKeys <ESC> and '%' aren't allowed to carry a macro.");
								if (!strcmp(buf, "\e")) {
									i = -2; /* leave */
									break;
								}
								continue;
							} else if (!strcmp(buf, "%")) {
								int delay;

								/* max length check, rough estimate */
								if (strlen(chain_macro_buf) + strlen(macro__buf) >= 1024 - 50) {
									c_msg_print("\377oDue to excess length you cannot chain any further commands to this macro.");
									continue;
								}

								/* Some macros require a latency-based delay in order to update the client with
								   necessary reply information from the server before another command based on
								   this action might work.
								   Example: Wield an item, then activate it. The activation part needs to wait
								   until the server tells the client that the item has been successfully equipped.
								   Example: Polymorph into a form that has a certain spell available to cast.
								   The casting needs to wait until the server tells us that we successfully polymorphed. */
								clear_from(l2++);
								if (should_wait) {
									Term_putstr(10, l2++, -1, TERM_YELLOW, "This macro might need a latency-based delay to work properly!");
									Term_putstr(10, l2++, -1, TERM_YELLOW, "You can accept the suggested delay or modify it in steps");
								} else {
									Term_putstr(10, l2++, -1, TERM_YELLOW, "No need to add a latency-based delay so you can just press ESC. But");
									Term_putstr(10, l2++, -1, TERM_YELLOW, "if you want one, eg for shop interaction, you may modify it in steps");
								}
								Term_putstr(10, l2++, -1, TERM_YELLOW, "of 100 ms up to 9900 ms, or just press ESC to not use a delay.");
								l2++;
								Term_putstr(10, l2, -1, TERM_L_GREEN, "ENTER\377g to accept, \377GESC\377g to discard (in ms):");

								/* suggest +25% reserve tolerance but at least +25 ms on the ping time */
								sprintf(tmp, "%d", ((ping_avg < 100 ? ping_avg + 25 : (ping_avg * 125) / 100) / 100 + 1) * 100);
								Term_gotoxy(52, l2);
								if (askfor_aux(tmp, 50, 0)) {
									delay = atoi(tmp);
									if (delay % 100) delay += 100; //QoL hack for noobs who can't read
									delay /= 100;
									if (delay < 0) delay = 0;
									if (delay > 99) delay = 99;

									if (delay) {
										sprintf(tmp, "\\w%c%c", '0' + delay / 10, '0' + delay % 10);
										text_to_ascii(macro__buf, tmp);
										/* Chain */
										if (chain_macro_buf[0] && !strncmp(macro__buf, "\e)", 2)) { /* Skip subsequent ')' keybuffer clearing, as it would cancel the previous macro if the player doesn't have enough energy to execute it right now! */
											strcat(chain_macro_buf, "\e");
											strcat(chain_macro_buf, macro__buf + 2);
										} else
										strcat(chain_macro_buf, macro__buf);
										strcpy(macro__buf, chain_macro_buf);
									}
								}

								/* chain another macro */
								goto Chain_Macro;
							}
							break;
						}
						/* exit? */
						if (i == -2) continue;

						switch (chain_type) {
						case 0:
							/* make it a command macro */
							/* Link the macro */
							macro_add(buf, macro__buf, TRUE, FALSE);
							/* Message */
							c_msg_print("Created a new command macro.");
							break;
						case 1:
							/* make it a hybrid macro */
							/* Link the macro */
							macro_add(buf, macro__buf, FALSE, TRUE);
							/* Message */
							c_msg_print("Created a new hybrid macro.");
							break;
						case 2:
							/* make it a norma macro */
							/* Link the macro */
							macro_add(buf, macro__buf, FALSE, FALSE);
							/* Message */
							c_msg_print("Created a new normal macro.");
						}

						/* this was the final step, we're done */
						i = -2;
						continue;
						}

					case mw_fileset: {
#ifdef TEST_CLIENT
						int xoffset1 = 1, xoffset2 = 3;
						int f, k, m, n, found;
						char *cc, *cf, *cfile;
						char buf_pat[32], buftxt_pat[32], buf_act[160], buftxt_act[160];
						char buf_basename[1024], tmpbuf[1024];
						bool style_cyclic, style_free;
						bool ok_new_set, ok_new_stage, ok_swap_stages;

						if (rescan) { /* (Is statically TRUE on first invocation of mw_fileset, to ensure an initial scan) */
							rescan = FALSE;
							filesets_found = macroset_scan();
						}

						while (TRUE) {
							ok_new_set = (filesets_found < MACROFILESETS_MAX);
							ok_new_stage = (fileset[fileset_selected].stages < MACROFILESETS_STAGES_MAX);
							ok_swap_stages = (fileset[fileset_selected].stages >= 2); //need at least 2 stages in order to swap anything

							/* --- Begin of visual output --- */
							//l = ystart + 1;
							l = 0; //Actually discard the compelete, usual 4-lines macro wizard header. Doesn't apply to us here and just takes up space.
							clear_from(l);

							/* Space requirements are a lot, adjust visual layout depending on bigmap screen mode! */
							if (screen_hgt == MAX_SCREEN_HGT) {
								/* --- Bigmap screen --- */

								/* Give user a choice and wait for user selection of what to do */
								if (!filesets_found) Term_putstr(xoffset1, l++, -1, TERM_GREEN, format("Found no macrosets (max %d).", MACROFILESETS_MAX));
								else Term_putstr(xoffset1, l++, -1, TERM_GREEN, format("Found %d macroset%s (max %d sets, max %d stages each. REFD=currently referenced):",
								    filesets_found, filesets_found != 1 ? "s" : "", MACROFILESETS_MAX, MACROFILESETS_STAGES_MAX));
								for (k = 0; k < MACROFILESETS_MAX; k++)
									if (k < filesets_found) {
										/* Build stages string */
										tmpbuf[0] = 0;
										for (f = 0; f < fileset[k].stages; f++) {
											if (f) strcat(tmpbuf, "\377g/");
											if (fileset[k].stage_file_exists[f]) strcat(tmpbuf, format("\377W%d", f + 1));
											else strcat(tmpbuf, format("\377D%d", f + 1));
										}
										for (; f < MACROFILESETS_STAGES_MAX; f++) strcat(tmpbuf, "  ");

										Term_putstr(xoffset2 - 1, l++, -1, fileset_selected == k ? TERM_VIOLET : TERM_UMBER,
										    //format("%s%2d\377g) %s\377g; %sStages\377g: %s\377g, active: %s; \377s%s\377-; \"\377s%s\377-\"",
										    format("%s%2d\377g) %s\377g; %sStages\377g: %s\377g [%s]; \377s%s\377-; \"\377s%s\377-\"",
										    fileset_selected == k ? ">" : " ", k + 1, fileset[k].currently_referenced ? "\377BREFD" : "\377gdisk", "",//fileset[k].all_stage_files_exist ? "" : "\377y",
										    tmpbuf, (k != fileset_selected || fileset_stage_selected == -1) ? "\377u-\377-" : format("\377v%d\377-", fileset_stage_selected + 1),
										    (fileset[k].style_cyclic && fileset[k].style_free) ? "Cyc+Fr" : (fileset[k].style_cyclic ? "Cyclic" : (fileset[k].style_free ? "FreeSw" : "------")),
										    fileset[k].basefilename));
									} else
										Term_putstr(xoffset2, l++, -1, TERM_UMBER, format("%2d\377g) -", k + 1));

								l++;
								Term_putstr(xoffset1, l++, -1, TERM_GREEN, "\377gWith the filesets listed above, you may...");
								Term_putstr(xoffset2, l++, -1, TERM_GREEN, "\377Ga\377-) Clear list and rescan (discards any unsaved macro set)");
								Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("%s%s", ok_new_set ? "\377Gb\377-" : "\377Db", ") Initialise a new set (it will also get selected automatically)"));
								if (filesets_found) {
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("%s%s", filesets_found ? "\377Gc\377-" : "\377Dc", ") Select a set to work with (persists through leaving this menu)"));
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("%s%s", filesets_found ? "\377Gd\377-" : "\377Dd", ") Forget a set (forgets all loaded reference keys to that set)"));
								} else l += 2;

								if (fileset_selected != -1) {
									l++;
									Term_putstr(xoffset1, l++, -1, TERM_GREEN, format("And with the currently selected fileset (\377v%d\377-) you may...", fileset_selected + 1));
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, "\377GA\377-) Modify its switching keys");
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, "\377GB\377-) Change its switching method");
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, ok_swap_stages ? "\377GC\377-) Swap two stages" : "\377DC) Swap two stages");
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("%s) Purge one of the set stage files (purges its reference keys) (1-%d)",
									    fileset[fileset_selected].stages ? "\377GD\377-" : "\377DD", fileset[fileset_selected].stages));
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("%s%s", ok_new_stage ?
									    "\377GE\377-" : "\377DE", ") Initialise+activate a new stage to the set (doesn't clear active macros)"));
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("%s) Activate a stage (\377oforgets active macros\377- & loads stage macrofile) (1-%d)",
									    fileset[fileset_selected].stages ? "\377GF\377-" : "\377DF", fileset[fileset_selected].stages));
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("\377GG\377-) Write all currently active macros to the activated stage file%s",
									    fileset_stage_selected == -1 ? "" : format(" (\377v%d\377-)", fileset_stage_selected + 1)));
								}

								l++;
								Term_putstr(xoffset1, l++, -1, TERM_GREEN, "After selecting a set (and stage), you can leave this menu with \377GESC\377- to work");
								Term_putstr(xoffset1, l++, -1, TERM_GREEN, "on the macros you wish this set to contain. When done, return here to save");
								Term_putstr(xoffset1, l++, -1, TERM_GREEN, "the macros to the selected stage file with '\377GG\377-)' (this will overwrite it).");

								l++;
								Term_putstr(15, l, -1, TERM_L_GREEN, "Please choose an action: ");
							} else {
								/* Small screen */

								/* Give user a choice and wait for user selection of what to do */
								if (!filesets_found) Term_putstr(xoffset1, l++, -1, TERM_GREEN, format("Found no macrosets (max %d).", MACROFILESETS_MAX));
								else Term_putstr(xoffset1, l++, -1, TERM_GREEN, format("Found %d macroset%s (max %d sets, max %d stages each. REFD=currently referenced):",
								    filesets_found, filesets_found != 1 ? "s" : "", MACROFILESETS_MAX, MACROFILESETS_STAGES_MAX));
								for (k = 0; k < MACROFILESETS_MAX; k++)
									if (k < filesets_found) {
										/* Build stages string */
										tmpbuf[0] = 0;
										for (f = 0; f < fileset[k].stages; f++) {
											if (f) strcat(tmpbuf, "\377g/");
											if (fileset[k].stage_file_exists[f]) strcat(tmpbuf, format("\377W%d", f + 1));
											else strcat(tmpbuf, format("\377D%d", f + 1));
										}
										for (; f < MACROFILESETS_STAGES_MAX; f++) strcat(tmpbuf, "  ");

										Term_putstr(xoffset2 - 1, l++, -1, fileset_selected == k ? TERM_VIOLET : TERM_UMBER,
										    //format("%s%2d\377g) %s\377g; %sStages\377g: %s\377g, active: %s; \377s%s\377-; \"\377s%s\377-\"",
										    format("%s%2d\377g) %s\377g; %sStages\377g: %s\377g [%s]; \377s%s\377-; \"\377s%s\377-\"",
										    fileset_selected == k ? ">" : " ", k + 1, fileset[k].currently_referenced ? "\377BREFD" : "\377gdisk", "",//fileset[k].all_stage_files_exist ? "" : "\377y",
										    tmpbuf, (k != fileset_selected || fileset_stage_selected == -1) ? "\377u-\377-" : format("\377v%d\377-", fileset_stage_selected + 1),
										    (fileset[k].style_cyclic && fileset[k].style_free) ? "Cyc+Fr" : (fileset[k].style_cyclic ? "Cyclic" : (fileset[k].style_free ? "FreeSw" : "------")),
										    fileset[k].basefilename));
									} else
										Term_putstr(xoffset2, l++, -1, TERM_UMBER, format("%2d\377g) -", k + 1));

								Term_putstr(xoffset2, l++, -1, TERM_GREEN, "\377Ga\377-) Clear list and rescan (discards any unsaved macro set)");
								Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("%s%s", ok_new_set ? "\377Gb\377-" : "\377Db", ") Initialise a new set (it will also get selected automatically)"));
								if (filesets_found) {
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("%s%s", filesets_found ? "\377Gc\377-" : "\377Dc", ") Select a set to work with (persists through leaving this menu)"));
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("%s%s", filesets_found ? "\377Gd\377-" : "\377Dd", ") Forget a set (forgets all loaded reference keys to that set)"));
								} else l += 2;

								if (fileset_selected != -1) {
									Term_putstr(xoffset1, l++, -1, TERM_GREEN, format("With the currently selected fileset (\377v%d\377-) you may...", fileset_selected + 1));
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, "\377GA\377-) Modify its switching keys");
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, "\377GB\377-) Change its switching method");
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, ok_swap_stages ? "\377GC\377-) Swap two stages" : "\377DC) Swap two stages");
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("%s) Purge one of the set stage files (purges its reference keys) (1-%d)",
									    fileset[fileset_selected].stages ? "\377GD\377-" : "\377DD", fileset[fileset_selected].stages));
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("%s%s", ok_new_stage ?
									    "\377GE\377-" : "\377DE", ") Initialise+activate a new stage to the set (doesn't clear active macros)"));
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("%s) Activate a stage (\377oforgets active macros\377- & loads stage macrofile) (1-%d)",
									    fileset[fileset_selected].stages ? "\377GF\377-" : "\377DF", fileset[fileset_selected].stages));
									Term_putstr(xoffset2, l++, -1, TERM_GREEN, format("\377GG\377-) Write all currently active macros to the activated stage file%s",
									    fileset_stage_selected == -1 ? "" : format(" (\377v%d\377-)", fileset_stage_selected + 1)));
								}

								Term_putstr(xoffset1, l++, -1, TERM_GREEN, "After selecting a set (and stage), you can leave this menu with \377GESC\377- to work");
								Term_putstr(xoffset1, l++, -1, TERM_GREEN, "on the macros. Return here to save all macros to selected stage with '\377GG\377-)'");
							}

							/* Hack: Hide the cursor */
							Term->scr->cx = Term->wid;
							Term->scr->cu = 1;

							i = choice = 0;
							while (TRUE) {
								switch (choice = inkey()) {
								case ESCAPE:
								case 'p':
								case '\010': /* backspace */
									i = -2; /* leave */
									break;
								case ':': /* Allow chatting */
									cmd_message();
									i = -3;
									break;
								case KTRL('T'):
									/* Take a screenshot */
									xhtml_screenshot("screenshot????", 2);
									i = -3;
									break;
								default:
									/* invalid action? */
									if ((choice < 'a' || choice > 'd') && (fileset_selected == -1 || choice < 'A' || choice > 'G')) continue;
								}
								break;
							}
							/* Restore top line */
							if (i == -3) continue;
							/* exit? */
							if (i == -2) break;

							/* Perform selected action */
							if (screen_hgt == MAX_SCREEN_HGT) Term_putstr(40, l++, -1, TERM_GREEN, format("(%c)", choice));
							switch (choice) {
/* 	#define MACROFILESET_MARKER_CYCLIC "Cycling\\sto\\sset" #define MACROFILESET_MARKER_SWITCH "Switching\\sto\\sset"
	bool style_cyclic; // Style: cyclic (at least one trigger key was found that cycles) bool style_free; // Style: free-switching (at last one trigger key was found that switches freely)
	char basefilename[MACROSET_NAME_LEN]; // Base .prf filename part (excluding path) for all macro files of this set, to which stage numbers get appended
	char macro__pat__cycle[32]; char macro__patbuf__cycle[32]; char macro__act__cycle[160]; char macro__actbuf__cycle[160];
	int stages; // Amount of stages to cyclic/switch between
	char macro__pat__switch[MACROFILESETS_STAGES_MAX][32]; char macro__patbuf__switch[MACROFILESETS_STAGES_MAX][32];
	char macro__act__switch[MACROFILESETS_STAGES_MAX][160]; char macro__actbuf__switch[MACROFILESETS_STAGES_MAX][160];
	bool stage_file_exists[MACROFILESETS_STAGES_MAX]; // stage file was actually found? (eg if stage files 1,2,4 are found, we must assume there is a stage 3, but maybe the file is missing)
	char stage_comment[MACROFILESETS_STAGES_MAX][MACROSET_COMMENT_LEN]; */
							/* Fileset actions: */
							case 'a': // wipe memory list and rescan
								/* Init filesets */
								rescan = TRUE;
								i = -4;
								break;
							case 'b': //init new fileset (implies initialization+activation of a 1st stage too)
								if (!ok_new_set) continue;
								// new set index, gets appended to existing ones
								// get name
								Term_putstr(1, l, -1, TERM_L_GREEN, "Enter a name for the new set: ");
								tmpbuf[0] = 0;
								if (!askfor_aux(tmpbuf, MACROSET_NAME_LEN, 0) || !tmpbuf[0]) continue;
								f = filesets_found++;
								strcpy(fileset[f].basefilename, tmpbuf);

#if 1
								// get type (switching method)
								Term_putstr(1, l, -1, TERM_L_GREEN, "Set the set type aka switching method (1 cyclic, 2 free-switch, 3 both): ");
								n = -1;
								while (TRUE) {
									tmpbuf[0] = 0;
									Term_gotoxy(74, l);
									if (!askfor_aux(tmpbuf, 1, 0)) break;
									n = atoi(tmpbuf);
									if (n < 1 || n > 3) continue;
									break;
								}
								if (n == -1) continue;
								fileset[f].style_cyclic = (n % 2 != 0);
								fileset[f].style_free = (n / 2 != 0);
#else
								/* --- do the rest in a new, cleared screen perhaps, so there is room for explanations --- */

								l = 0;
								clear_from(l);
								Term_putstr(1, l++, -1, TERM_L_GREEN, format("Set \"\377s%s\377-\":", fileset[f].basefilename));
								l++;

								// get type (switching method)
								Term_putstr(1, l++, -1, TERM_L_GREEN, "Set the set type aka switching method (1 cyclic, 2 free-switch, 3 both): ");
								n = -1;
								while (TRUE) {
									tmpbuf[0] = 0;
									Term_gotoxy(74, l);
									if (!askfor_aux(tmpbuf, 1, 0)) break;
									n = atoi(tmpbuf);
									if (n < 1 || n > 3) continue;
									break;
								}
								if (n == -1) continue;
								fileset[f].style_cyclic = (n % 2 != 0);
								fileset[f].style_free = (n / 2 != 0);
								if (fileset[f].style_cyclic) {
									if (fileset[f].style_free)
										Term_putstr(15, l++, -1, TERM_L_GREEN, "Switching method: \377sCyclic+FreeSw");
									else
										Term_putstr(15, l++, -1, TERM_L_GREEN, "Switching method: \377sCyclic");
								} else if (fileset[f].style_free) Term_putstr(15, l++, -1, TERM_L_GREEN, "Switching method: \377sFreeSw");
								else Term_putstr(15, l++, -1, TERM_L_GREEN, "Switching method: \377s------");
								l++;
#endif
								// auto-select set and its first stage
								fileset_selected = f;
								fileset[f].stages = 1;
								fileset_stage_selected = 0;

								/* Init cycle keys */
								fileset[f].macro__pat__cycle[0] = 0;
								fileset[f].macro__patbuf__cycle[0] = 0;
								/* Init switch keys */
								fileset[f].macro__pat__switch[fileset_stage_selected][0] = 0;
								fileset[f].macro__patbuf__switch[fileset_stage_selected][0] = 0;
								fileset[f].macro__act__switch[fileset_stage_selected][0] = 0;
								fileset[f].macro__actbuf__switch[fileset_stage_selected][0] = 0;

								// ask for cycling-key / 1st stage switching key depending on selected type (1/2/1+2)
								if (fileset[f].style_cyclic) { //ask for set-global cycling-key
									while (TRUE) {
										Term_putstr(1, l, -1, TERM_L_GREEN, "Press the key you want to use as macro-cycling trigger: ");
										tmpbuf[0] = 0;
										get_macro_trigger(tmpbuf);
										if (!strcmp(tmpbuf, "\e") || !strcmp(buf, "%")) {
											c_msg_print("\377yKeys <ESC> and '%' aren't allowed to carry a macro.");
											if (!strcmp(buf, "\e")) break;
											continue;
										}
										break;
									}
									if (!strcmp(buf, "\e")) continue; //abort

									/* Set macro trigger */
									strcpy(buf_pat, tmpbuf);
									strcpy(fileset[k].macro__pat__cycle, buf_pat);
									/* Set macro trigger in human-readable format */
									ascii_to_text(buftxt_pat, buf_pat);
									strcpy(fileset[k].macro__patbuf__cycle, buftxt_pat);

									/* Forge macro action (in human-readable format) */
									sprintf(tmpbuf, ":%%:TEST-CYCLIC\r");
									//"\e):%:Cycling\sto\sset\sTESTSET\r\e)%ldummyset-FS3.prf\r\e"

									/* Set macro action in human-readable format */
									strcpy(buftxt_act, tmpbuf);
									strcpy(fileset[k].macro__actbuf__cycle, buftxt_act);
									/* Set macro action */
									text_to_ascii(buf_act, buftxt_act);
									strcpy(fileset[k].macro__act__cycle, buf_act);

									/* Also add it to the currently loaded macros */
									//key_autoconvert(tmp, fmt);
									macro_add(buf_pat, buf_act, FALSE, FALSE);
								}
								if (fileset[f].style_free) { //ask for stage-specific switching-key
									while (TRUE) {
										Term_putstr(1, l, -1, TERM_L_GREEN, "Press the key you want to use as stage 1-specific trigger: ");
										tmpbuf[0] = 0;
										get_macro_trigger(tmpbuf);
										if (!strcmp(tmpbuf, "\e") || !strcmp(buf, "%")) {
											c_msg_print("\377yKeys <ESC> and '%' aren't allowed to carry a macro.");
											if (!strcmp(buf, "\e")) break;
											continue;
										}
										break;
									}
									if (!strcmp(buf, "\e")) continue; //abort

									/* Set macro trigger */
									strcpy(buf_pat, tmpbuf);
									strcpy(fileset[k].macro__pat__switch[fileset_stage_selected], buf_pat);
									/* Set macro trigger in human-readable format */
									ascii_to_text(buftxt_pat, buf_pat);
									strcpy(fileset[k].macro__patbuf__switch[fileset_stage_selected], buftxt_pat);

									/* Forge macro action (in human-readable format) */
									sprintf(tmpbuf, ":%%:TEST-FREESW\r");
									//"\e):%:Switching\sto\sset\sTESTSET\r\e)%ldummyset-FS3.prf\r\e"

									/* Set macro action in human-readable format */
									strcpy(buftxt_act, tmpbuf);
									strcpy(fileset[k].macro__actbuf__switch[fileset_stage_selected], buftxt_act);
									/* Set macro action */
									text_to_ascii(buf_act, buftxt_act);
									strcpy(fileset[k].macro__act__switch[fileset_stage_selected], buf_act);

									/* Also add it to the currently loaded macros */
									//key_autoconvert(tmp, fmt);
									macro_add(buf_pat, buf_act, FALSE, FALSE);
								}

								break;

							case 'c': //select a set
								GET_MACROFILESET
								fileset_selected = f;
								break;

							case 'd': //forget a set
								GET_MACROFILESET
								if (fileset_selected == f) fileset_selected = -1; //unselect it if it was selected
								//scan all macros
								m = -1;
								found = 0;
								while (TRUE) {
									while (++m < macro__num) {
										/* Get macro in parsable format */
										strncpy(buf_act, macro__act[m], 159);
										buf_act[159] = '\0';
										ascii_to_text(buftxt_act, buf_act);

										/* Scan macro for marker text, indicating that it's a set-switch */
										// (note: MACROFILESET_MARKER_CYCLIC "Cycling\\sto\\sset")
										// (note: MACROFILESET_MARKER_SWITCH "Switching\\sto\\sset")
										if ((cc = strstr(buftxt_act, MACROFILESET_MARKER_CYCLIC))) style_cyclic = TRUE;
										if ((cf = strstr(buftxt_act, MACROFILESET_MARKER_SWITCH))) style_free = TRUE;

										if (!style_cyclic && !style_free) continue;

										/* Found one! */

										/* Determine base filename from the action text : %lFILENAME\r\e */
										cfile = strstr(buftxt_act, "%l");
										if (!cfile) continue; //broken set-switching macro (not following our known scheme)
										strcpy(buf_basename, cfile + 2);
										/* Find end of filename */
										cfile = strstr(buf_basename, "\\r\\e");
										if (!cfile) continue; //broken set-switching macro (not following our known scheme)
										*cfile = 0;
										/* Find start of 'stage' appendix of the filename, cut it off to obtain base filename.
										   Assume filename has this format "basename-FSn.prf" where n is the stage number: 0...MACROFILESETS_STAGES_MAX */
										/* If this is a cyclic macro, the number after FS won't give the # of cycles away! Only the %:... self-notification message can do that!
										   So it should follow a specific format: ":%:Cycling to set n of m\r 'comment'\r" <- the 'of m' giving away the true amount of stages for cyclic sets!
										   However, it might be better to instead scan the folder for macro files starting on the base filename instead, so we are sure to catch all. */
										if (strncmp(buf_basename + strlen(buf_basename) - 8, "-FS", 3)) continue; //broken set-switching macro (not following our known scheme)

										/* --- At this point, we confirmed a valid macro belonging to a macro set found --- */

										/* Finalize base filename */
										buf_basename[strlen(buf_basename) - 8] = 0;

										/* Compare to set we want to delete */
										if (strcmp(fileset[f].basefilename, buf_basename)) continue;

										/* --- Matches target set! --- */

										/* Delete macro */
										(void)macro_del(macro__pat[m]);
										found++;

										/* Continue scanning keys for switch-macros */
									}
									/* Scanned the last one of all loaded macros? We're done. */
									if (m >= macro__num - 1) break;
								}
								if (!found) c_msg_print("No references to the macroset were found within currently loaded macros.");
								c_msg_format("%d reference%s to the macroset were cleared within currently loaded macros.", found, found == 1 ? "" : "s");

								/* Slide the rest of the sets list one up */
								filesets_found--; /* One less registered macroset */
								for (k = f; k < filesets_found; k++)
									fileset[k] = fileset[k + 1];
								/* Erase final one */
								fileset[k].stages = 0;
								break;

							/* Note: If a fileset is activated, a stage of it will also be activated automatically. If it's a new set, it'll be stage #1 (still empty). */
							/* Fileset-stage actions (required 'fileset_selected != -1' condition was already checked directly after inkey() read) : */
							case 'A': //modify switching keys
								break;

							case 'B': //modify switching method
								break;

							case 'C': //swap two stages
								if (!ok_swap_stages) continue;
								break;

							case 'D': //purge a stage
								if (!fileset[fileset_selected].stages) continue;
								break;

							case 'E': //init additional stage; append it to or insert it into the current stages list
								if (!ok_new_stage) continue;
								break;

							case 'F': //activate a stage
								GET_MACROFILESET_STAGE
								fileset_stage_selected = f;
								break;

							case 'G': //write current macros to active stage
#if 0
								// get comment
								Term_putstr(15, l, -1, TERM_L_GREEN, "Enter a comment (optional): ");
								tmpbuf[0] = 0;
								if (!askfor_aux(tmpbuf, MACROSET_COMMENT_LEN, 0)) fileset[f].comment[0] = 0;
								strcpy(fileset[f].comment, tmpbuf);
#endif

								break;
							}

							/* Restart mw_fileset menu */
							if (i == -4) break;
						}

						/* Hack: Restart mw_fileset menu */
						if (i == -4) {
							i = 'Z';
							continue;
						}
#endif /* TEST_CLIENT */

						/* this was the final step, we're done */
						i = -1; //actually don't continue (back to macro wizard main screen) but break out (back to macro menu) for convenience!
						continue; }
					}


					/* --------------- specify item/parm if required --------------- */



					/* might input a really long line? */
					if (choice == mw_custom) {
						Term_gotoxy(5, ystart + 9);

						/* Get an item/spell name */
						strcpy(buf, "");
						if (!askfor_aux(buf, 159, 0)) {
							i = -2;
							continue;
						}
					}
					else if (choice == mw_slash) {
						Term_gotoxy(5, ystart + 9);

						/* Get an item/spell name */
						strcpy(buf, "");
						if (!askfor_aux(buf, 159, 0)) {
							i = -2;
							continue;
						}
						strcpy(buf, format(":%s\r", buf));
					}
					/* mw_mimicidx is special: it requires a number (1..n) */
					else if (choice == mw_mimicidx) {
						while (TRUE) {
							/* Get power slot */
							Term_gotoxy(47, ystart + 8);
							strcpy(buf, "");
							if (!askfor_aux(buf, 159, 0)) {
								i = -2;
								break;
							}
							/* not a number/invalid? retry (we have slots d)..z)) */
							if (atoi(buf) < 1 || atoi(buf) > 26 - 3) continue;

							/* ok (1..23) - translate into spell slot */
							strcpy(buf, format("%c", 'd' + atoi(buf)));
							break;
						}
						if (i == -2) continue;
					}
					/* no need for inputting an item/spell to use with the macro? */
					else if (choice != mw_fire && choice != mw_rune && choice != mw_trap && choice != mw_prfimm &&
					    choice != mw_stance && choice != mw_common && choice != mw_dir_run && choice != mw_dir_tunnel &&
					    choice != mw_dir_disarm && choice != mw_dir_bash && choice != mw_dir_close && choice != mw_prfele &&
					    choice != mw_fileset) {
						if (choice == mw_load) Term_gotoxy(23, ystart + 8);
						else if (choice == mw_poly) Term_gotoxy(47, ystart + 11);
						else if (choice == mw_option) Term_gotoxy(30, ystart + 4);
						else Term_gotoxy(47, ystart + 8);

						/* Get an item/spell name */
						strcpy(buf, "");
						if (!askfor_aux(buf, 159, 0)) {
							i = -2;
							continue;
						}

						/* allow 'empty' polymorph-into macro that prompts for form */
						if (choice != mw_poly || buf[0]) strcat(buf, "\\r");
					}



					/* --------------- complete the macro by glueing premade part and default part together --------------- */



					/* generate the full macro action; magic device/preferred immunity macros are already pre-made */
					if (choice != mw_device && choice != mw_prfimm && choice != mw_custom && choice != mw_common &&
					    choice != mw_dir_run && choice != mw_dir_tunnel && choice != mw_dir_disarm && choice != mw_dir_bash &&
					    choice != mw_dir_close && choice != mw_prfele && choice != mw_fileset) {
						buf2[0] = '\\'; //note: should in theory be ')e\',
						buf2[1] = 'e'; //      but doesn't work due to prompt behaviour
						buf2[2] = ')'; //      (\e will then get ignored)
					}

					switch (choice) {
					case mw_quaff:
						buf2[3] = 'q';
						buf2[4] = '@';
						strcpy(buf2 + 5, buf);
						break;
					case mw_read:
						buf2[3] = 'r';
						buf2[4] = '@';
						strcpy(buf2 + 5, buf);
						break;
					case mw_any:
						buf2[3] = '/';
						buf2[4] = '@';
						strcpy(buf2 + 5, buf);
						break;
					case mw_anydir:
						buf2[3] = '/';
						buf2[4] = '@';
						strcpy(buf2 + 5, buf);
						l = strlen(buf2);
						buf2[l] = '*';
						buf2[l + 1] = 't';
						buf2[l + 2] = 0;
						break;
					case mw_fire:
						if (c_cfg.rogue_like_commands) buf2[3] = 't';
						else buf2[3] = 'f';
						buf2[4] = '*';
						buf2[5] = 't';
						buf2[6] = 0;
						break;
					case mw_schoolnt:
					case mw_schoolt:
						buf2[3] = 'm';
						buf2[4] = '@';
						buf2[5] = '1';
						buf2[6] = '1';
						buf2[7] = '\\';
						buf2[8] = 'r';
						buf2[9] = '@';
						strcpy(buf2 + 10, buf);
						if (choice == mw_schoolt) {
							strcpy(buf, "*t");
							strcat(buf2, buf);
						}
						break;
					case mw_mimicnt:
					case mw_mimict:
						buf2[3] = 'm';
						buf2[4] = '@';
						buf2[5] = '3';
						buf2[6] = '\\';
						buf2[7] = 'r';
						buf2[8] = '@';
						strcpy(buf2 + 9, buf);
						if (choice == mw_mimict) {
							strcpy(buf, "*t");
							strcat(buf2, buf);
						}
						break;
					case mw_mimicidx:
						buf2[3] = '*';
						buf2[4] = 't';
						buf2[5] = 'm';
						buf2[6] = '@';
						buf2[7] = '3';
						buf2[8] = '\\';
						buf2[9] = 'r';
						strcpy(buf2 + 10, buf);
						/* note: targetting method '-' is the only option here,
						   for safety reasons (if spell doesn't take a target!) */
						strcat(buf2, "-");
						break;
					case mw_fight:
						buf2[3] = 'm';
						buf2[4] = '@';
						buf2[5] = '5';
						buf2[6] = '\\';
						buf2[7] = 'r';
						buf2[8] = '@';
						strcpy(buf2 + 9, buf);
						break;
					case mw_stance:
						buf2[3] = 'm';
						buf2[4] = '@';
						buf2[5] = '1';
						buf2[6] = '3';
						buf2[7] = '\\';
						buf2[8] = 'r';
						buf2[9] = buf[0];
						buf2[10] = 0;
						break;
					case mw_shoot:
						buf2[3] = 'm';
						buf2[4] = '@';
						buf2[5] = '6';
						buf2[6] = '\\';
						buf2[7] = 'r';
						buf2[8] = '@';
						strcpy(buf2 + 9, buf);
						break;
					case mw_poly:
						buf2[3] = 'm';
						buf2[4] = '@';
						buf2[5] = '3';
						buf2[6] = '\\';
						buf2[7] = 'r';
						buf2[8] = 'c';
						strcpy(buf2 + 9, buf);
						break;
					case mw_rune:
						buf2[3] = 'm';
						buf2[4] = '@';
						buf2[5] = '1';
						buf2[6] = '2';
						buf2[7] = '\\';
						buf2[8] = 'r';
						strcpy(buf2 + 9, buf);
						break;
					case mw_trap:
						buf2[3] = 'm';
						buf2[4] = '@';
						buf2[5] = '1';
						buf2[6] = '0';
						buf2[7] = '\\';
						buf2[8] = 'r';
						buf2[9] = '@';
						strcpy(buf2 + 10, buf);
						break;
					case mw_device:
						/* hack: magiv device uses direction? */
						if (j == 1) strcat(buf, "*t");

						strcat(buf2, buf);
						break;
					case mw_abilitynt:
					case mw_abilityt:
						buf2[3] = 'm';
						buf2[4] = '@';
						strcpy(buf2 + 5, buf);
						if (choice == mw_abilityt) {
							strcpy(buf, "*t");
							strcat(buf2, buf);
						}
						break;
					case mw_throw:
						buf2[3] = 'v';
						buf2[4] = '@';
						strcpy(buf2 + 5, buf);
						l = strlen(buf2);
						buf2[l] = '*';
						buf2[l + 1] = 't';
						buf2[l + 2] = 0;
						break;

					case mw_slash:
						strcat(buf2, buf);
						break;

					case mw_custom:
						strcpy(buf2, buf);
						break;

					case mw_load:
						buf2[3] = '%';
						buf2[4] = 'l';
						strcpy(buf2 + 5, buf);
						l = strlen(buf2);
						buf2[l] = '\\';
						buf2[l + 1] = 'e';
						buf2[l + 2] = 0;
						break;

					case mw_option:
#if 0 /* actually invoke the options menu? */
						buf2[3] = '%';
						buf2[4] = 'z';
						... implement locating the desired option ...
						buf2[l] = '\\';
						buf2[l + 1] = 'e';
						buf2[l + 2] = '\\';
						buf2[l + 3] = 'e';
						buf2[l + 4] = 0;
#else /* use client-side option-toggling slash command? */
						switch (j) {
						case 'y':
							strcat(buf2, ":/opty ");
							break;
						case 'n':
							strcat(buf2, ":/optn ");
							break;
						case 't':
							strcat(buf2, ":/optt ");
							break;
						case 'Y':
							strcat(buf2, ":/optvy ");
							break;
						case 'N':
							strcat(buf2, ":/optvn ");
							break;
						case 'T':
							strcat(buf2, ":/optvt ");
							break;
						}
						strcat(buf2, buf);
						strcat(buf2, "\\n");
#endif
						break;

					case mw_equip:
						if (!c_cfg.rogue_like_commands)
							switch (j) {
							case 'w':
								buf2[3] = 'w';
								break;
							case 'W':
								buf2[3] = 'W';
								break;
							case 't':
								buf2[3] = 't';
								break;
							case 'x':
								buf2[3] = 'x';
								break;
							}
						else
							switch (j) {
							case 'w':
								buf2[3] = 'w';
								break;
							case 'W':
								buf2[3] = 'W';
								break;
							case 'T':
								buf2[3] = 'T';
								break;
							case 'S':
								buf2[3] = 'S';
								break;
							}
						switch (j) {
						case 'd':
							strcat(buf2, ":/dress");
							if (buf[2]) { //buf[0+1] are "/r" if empty
								strcat(buf2, " ");
								strcat(buf2, buf);
							} else strcat(buf2, buf);
							break;
						case 'b':
							strcat(buf2, ":/bed");
							if (buf[2]) { //buf[0+1] are "/r" if empty
								strcat(buf2, " ");
								strcat(buf2, buf);
							} else strcat(buf2, buf);
							break;
						default:
							buf2[4] = '@';
							strcpy(buf2 + 5, buf);
						}
						break;
					}

					/* Convert the targetting method from XXX*t to *tXXX- ? */
#ifdef MACRO_WIZARD_SMART_TARGET
					/* ask about replacing '*t' vs '-' (4.4.6) vs '+' (4.4.6b) */
					if (strstr(buf2, "*t")
					    && choice != mw_mimicidx
					    && choice != mw_common /* *t- is used there, we'll assume it always should be method a) */
					    && choice != mw_load /* (paranoia) */
					    && choice != mw_custom
					    ) {
						while (TRUE) {
							i = 1; //clearing it in case it was set to -3 below
							clear_from(ystart);
							l = ystart + 1;
							Term_putstr(10, l++, -1, TERM_GREEN, "Please choose the targetting method:");
							l++;

							//Term_putstr(10, l++, -1, TERM_GREEN, "(\377UHINT: \377gAlso inscribe your ammo '!=' for auto-pickup!)");
							Term_putstr(10, l++, -1, TERM_L_GREEN, "a) Target closest monster if such exists,");
							Term_putstr(10, l++, -1, TERM_L_GREEN, "   otherwise cancel action. (\377URecommended in most cases!\377G)");
							Term_putstr(10, l++, -1, TERM_L_GREEN, "b) Target closest monster if such exists,");
							Term_putstr(10, l++, -1, TERM_L_GREEN, "   otherwise prompt for direction.");
							Term_putstr(10, l++, -1, TERM_L_GREEN, "c) Target closest monster if such exists,");
							Term_putstr(10, l++, -1, TERM_L_GREEN, "   otherwise target own grid.");
							Term_putstr(10, l++, -1, TERM_L_GREEN, "d) Fire into a fixed direction or prompt for direction.");
							Term_putstr(10, l++, -1, TERM_L_GREEN, "e) Target own grid (ie yourself).");
							l++;

							Term_putstr(10, l++, -1, TERM_L_GREEN, "f) Target most wounded friendly player,");
							Term_putstr(10, l++, -1, TERM_L_GREEN, "   cancel action if no player is nearby. (\377UEg for 'Cure Wounds'.\377G)");
							Term_putstr(10, l++, -1, TERM_L_GREEN, "g) Target most wounded friendly player,");
							Term_putstr(10, l++, -1, TERM_L_GREEN, "   target own grid instead if no player is nearby.");

							while (TRUE) {
								switch (choice = inkey()) {
								case ESCAPE:
								case 'p':
								case '\010': /* backspace */
									i = -2; /* leave */
									break;
								case ':': /* Allow chatting */
									cmd_message();
									inkey_msg = TRUE; /* And suppress macros again.. */
									/* Restore top line */
									Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
									continue;
								case KTRL('T'):
									/* Take a screenshot */
									xhtml_screenshot("screenshot????", 2);
									continue;
								default:
									/* invalid action -> exit wizard */
									if (choice < 'a' || choice > 'g') {
										//i = -2;
										continue;
									}
								}
								break;
							}
							/* exit? */
							if (i == -2) break;

							/* Get a specific fixed direction */
							if (choice == 'd') {
								clear_from(ystart);
								l = ystart + 2;
								Term_putstr(10, l++, -1, TERM_GREEN, "Please pick the specific, fixed direction or '%':");
								l += 2;

								Term_putstr(25, l++, -1, TERM_L_GREEN, " 7  8  9");
								Term_putstr(25, l++, -1, TERM_GREEN, "  \\ | / ");
								Term_putstr(25, l++, -1, TERM_L_GREEN, "4 \377g-\377G 5 \377g-\377G 6");
								//Term_putstr(25, l++, -1, TERM_GREEN, "  / | \\         \377G?\377g = 'Prompt for direction each time'");
								Term_putstr(25, l++, -1, TERM_GREEN, "  / | \\         \377G%\377g = 'Prompt for direction each time'");
								Term_putstr(25, l++, -1, TERM_L_GREEN, " 1  2  3");
								l += 2;

								//Term_putstr(15, l++, -1, TERM_L_GREEN, "Your choice? (1 to 9, or '?') ");
								Term_putstr(15, l++, -1, TERM_L_GREEN, "Your choice? (1 to 9, or '%') ");

//#if 0 /* No - this will break if the user has any kind of macro on the key already. Eg on his '?' key, and then presses '?' here. Need another solution. */
#if 1 /* Actually we need this, but we will replace ? by %, so it's guaranteedly a macro-free key. */
								/* hack: temporarily enable macro parsing for using numpad keys without numlock to specify a direction */
								inkey_interact_macros = FALSE;
#endif
								while (TRUE) {
									target_dir = inkey();
//c_message_add(format("target_dir = '%i' ( %c %c )", target_dir, (char)(target_dir & 0xFF), (char)((target_dir & 0xFF00) >> 8)));
									switch (target_dir) {
									case ESCAPE:
									case 'p':
									case '\010': /* backspace */
										i = -3; /* leave */
										break;
									case ':': /* Allow chatting */
										cmd_message();
										/* Restore top line */
										Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
										continue;
									case KTRL('T'):
										/* Take a screenshot */
										xhtml_screenshot("screenshot????", 2);
										continue;
									default:
										/* invalid action -> exit wizard */
										//if ((target_dir < '1' || target_dir > '9') && target_dir != '?') {
										if ((target_dir < '1' || target_dir > '9') && target_dir != '%') {
											//i = -3;
											continue;
										}
									}
									break;
								}
#if 1 /* (see above) */
								/* disable macro parsing again */
								inkey_interact_macros = TRUE;
#endif
								/* exit? */
								if (i == -3) continue;
							}
							/* successfully done this step */
							break;
						}
						if (i == -2) continue;

						if (choice != 'c') {
							/* choose initial targetting mechanics */
							if (choice == 'd') strcpy(buf, "\\e)");
							else if (choice == 'e') strcpy(buf, "\\e)*q");
							else if (choice == 'f' || choice == 'g') strcpy(buf, "\\e)(");
							else strcpy(buf, "\\e)*t");

							/* We assume that '*t' is always the last part in the macro
							   and that '\e)' is always the first part */
							strncat(buf, buf2 + 3, strlen(buf2) - 5);

							/* add new direction feature */
							if (choice == 'b') strcat(buf, "+");
							else if (choice == 'd') {
								//if (target_dir != '?') strcat(buf, format("%c", target_dir));
								if (target_dir != '%') strcat(buf, format("%c", target_dir));
							}
							else if (choice == 'e' || choice == 'g') strcat(buf, "5");
							else strcat(buf, "-");

							/* replace old macro by this one */
							strcpy(buf2, buf);
						}
					}
#endif

					/* Extract an action */
					/* Omit repeated '\e)' -- can break chain macros (todo: fix?) */
					if (chain_macro_buf[0] && choice != mw_custom) text_to_ascii(macro__buf, buf2 + 3);
					else text_to_ascii(macro__buf, buf2);
					/* Handle chained macros */
					if (chain_macro_buf[0] && !strncmp(macro__buf, "\e)", 2)) { /* Skip subsequent ')' keybuffer clearing, as it would cancel the previous macro if the player doesn't have enough energy to execute it right now! */
						strcat(chain_macro_buf, "\e");
						strcat(chain_macro_buf, macro__buf + 2);
					} else
					strcat(chain_macro_buf, macro__buf);
					strcpy(macro__buf, chain_macro_buf);

					/* advance to next step */
					i++;
					break;
				case 2:
					l = ystart + 1;
					Term_putstr(10, l++, -1, TERM_GREEN, "In this final step, press the key you would like");
					Term_putstr(10, l++, -1, TERM_GREEN, "to bind the macro to. (ESC and % key cannot be used.)");
					Term_putstr(10, l++, -1, TERM_GREEN, "You should use keys that have no other purpose!");
					Term_putstr(10, l++, -1, TERM_GREEN, "Good examples are the F-keys, F1 to F12 and unused number pad keys.");
					l++;
					//Term_putstr(10, l++, -1, TERM_GREEN, "The keys ESC and '%' are NOT allowed to be used.");
					Term_putstr(10, l++, -1, TERM_GREEN, "Most keys can be combined with \377USHIFT\377g, \377UALT\377g or \377UCTRL\377g modifiers!");
					l++;
					Term_putstr(10, l++, -1, TERM_GREEN, "If you want to \377Uchain another macro\377g, press '\377U%\377g' key.");
					Term_putstr(10, l++, -1, TERM_GREEN, "By doing this you can combine multiple macros into one hotkey.");
					l++;
					Term_putstr(5, l, -1, TERM_L_GREEN, "Press the key to bind the macro to, or '%' for chaining: ");

					while (TRUE) {
						l2 = l;

						/* Get a macro trigger */
						Term_putstr(67, l2, -1, TERM_WHITE, "  ");//45
						Term_gotoxy(67, l2);
						get_macro_trigger(buf);

						/* choose proper macro type, and bind it to key */
#if 0 /* no, because we allow remapping of this key too, here */
						if (!strcmp(buf, format("%c", KTRL('T')))) {
							/* Take a screenshot */
							xhtml_screenshot("screenshot????", 2);
							continue;
						} else
#endif
						if (!strcmp(buf, "\e")) {
							c_msg_print("\377yKeys <ESC> and '%' aren't allowed to carry a macro.");
							if (!strcmp(buf, "\e")) {
								i = -2; /* leave */
								break;
							}
							continue;
						} else if (!strcmp(buf, "%")) {
							char tmp[1024];
							int delay;

							/* max length check, rough estimate */
							if (strlen(chain_macro_buf) + strlen(macro__buf) >= 1024 - 50) {
								c_msg_print("\377oDue to excess length you cannot chain any further commands to this macro.");
								continue;
							}

							/* If not already planned to be suggested, still ask if we want to add a delay */

							/* Some macros require a latency-based delay in order to update the client with
							   necessary reply information from the server before another command based on
							   this action might work.
							   Example: Wield an item, then activate it. The activation part needs to wait
							   until the server tells the client that the item has been successfully equipped.
							   Example: Polymorph into a form that has a certain spell available to cast.
							   The casting needs to wait until the server tells us that we successfully polymorphed. */
							clear_from(l2);
							if (should_wait) {
								Term_putstr(10, l2++, -1, TERM_YELLOW, "This macro might need a latency-based delay to work properly!");
								Term_putstr(10, l2++, -1, TERM_YELLOW, "You can accept the suggested delay or modify it in steps");
							} else {
								Term_putstr(10, l2++, -1, TERM_YELLOW, "No need to add a latency-based delay so you can just press ESC. But");
								Term_putstr(10, l2++, -1, TERM_YELLOW, "if you want one, eg for shop interaction, you may modify it in steps");
							}
							Term_putstr(10, l2++, -1, TERM_YELLOW, "of 100 ms up to 9900 ms, or just press ESC to not use a delay.");
							l2++;
							Term_putstr(10, l2, -1, TERM_L_GREEN, "ENTER\377g to accept, \377GESC\377g to discard (in ms):");

							/* suggest +25% reserve tolerance but at least +25 ms on the ping time */
							sprintf(tmp, "%d", ((ping_avg < 100 ? ping_avg + 25 : (ping_avg * 125) / 100) / 100 + 1) * 100);
							Term_gotoxy(52, l2);
							if (askfor_aux(tmp, 50, 0)) {
								delay = atoi(tmp);
								if (delay % 100) delay += 100; //QoL hack for noobs who can't read
								delay /= 100;
								if (delay < 0) delay = 0;
								if (delay > 99) delay = 99;

								if (delay) {
									sprintf(tmp, "\\w%c%c", '0' + delay / 10, '0' + delay % 10);
									text_to_ascii(macro__buf, tmp);
									/* Chain */
									if (chain_macro_buf[0] && !strncmp(macro__buf, "\e)", 2)) { /* Skip subsequent ')' keybuffer clearing, as it would cancel the previous macro if the player doesn't have enough energy to execute it right now! */
										strcat(chain_macro_buf, "\e");
										strcat(chain_macro_buf, macro__buf + 2);
									} else
									strcat(chain_macro_buf, macro__buf);
									strcpy(macro__buf, chain_macro_buf);
								}
							}

							/* Echo it for convenience, to check */
							ascii_to_text(tmp, chain_macro_buf);
							c_msg_format("\377yChaining to: %s", tmp);

							/* chain another macro */
							goto Chain_Macro;
						}
						break;
					}
					/* exit? */
					if (i == -2) continue;

					/* Automatically choose usually best fitting macro type,
					   depending on chosen trigger key! */
					//[normal macros: F-keys (only keys that aren't used for any text input)]
					//command macros: / * a..w (all keys that are used in important standard prompts: inventory)
					//hybrid macros: all others, maybe even also normal-macro-keys
					if (!strcmp(buf, "/") || !strcmp(buf, "*") || /* windows */
					    (strlen(buf) == 10 && (buf[8] == '/' || buf[8] == '*')) || /* x11 - to do: fix this crap */
#if 0
					    (*buf >= 'a' && *buf <= 'w') || /* inventory */
#else
					    (*buf >= 'a' && *buf <= 'z') || /* command keys - you shouldn't really put macros on these, but w/e.. */
					    (*buf >= 'A' && *buf <= 'Z') ||
					    (*buf >= '0' && *buf <= '9') || /* menu choices. Especially for ~ menu it is required that these are command macros so they don't trigger there.. */
					    *buf == ' ' || /* spacebar is used almost everywhere, should only be command macro */
#endif
					    *buf == '+' || *buf == '-' || /* the '+' and '-' keys are used in certain input prompts (targetting, drop newest item) */
					    (buf[0] == 1 && buf[1] == 0) /* CTRL+A glitch */
					    ) {
						/* make it a command macro */
						/* Link the macro */
						macro_add(buf, macro__buf, TRUE, FALSE);
						/* Message */
						c_msg_print("Created a new command macro.");
					} else if (force_normal) {
						/* make it a normal macro */
						/* Link the macro */
						macro_add(buf, macro__buf, FALSE, FALSE);
						/* Message */
						c_msg_print("Created a new normal macro.");
					} else {
						/* make it a hybrid macro */
						/* Link the macro */
						macro_add(buf, macro__buf, FALSE, TRUE);
						/* Message */
						c_msg_print("Created a new hybrid macro.");
					}

					/* this was the final step, we're done */
					i = -1;
					break;
				}
			}
		}

		/* Oops */
		else {
			/* Oops */
			bell();
		}
	}

	/* Reload screen */
	Term_load();
	topline_icky = FALSE;

	/* Flush the queue */
	Flush_queue();

	inkey_interact_macros = FALSE;
	inkey_msg = inkey_msg_old;

	/* in case we entered this menu from recording a macro,
	   we might have to update the '*recording*' status line */
	if (were_recording) Send_redraw(0);
}

#define INTEGRATED_SELECTOR /* Allows for a match length of 55 instead of 53 */
#define AUTOINS_PAGESIZE 20
#define AUTOINS_FORCE_COL 'o'
#define AUTOINS_DIS_SUB_MATCH /* Colour the match string to indicate subinven/disabled flags, instead of just the index number */
void auto_inscriptions(void) {
	int i, max_page = (MAX_AUTO_INSCRIPTIONS + AUTOINS_PAGESIZE - 1) / AUTOINS_PAGESIZE - 1, cur_idx;
	static int cur_line = 0, cur_page = 0;
	char temp, tempbuf[MAX_CHARS]; //assuming that AUTOINS_MATCH_LEN + 8 and AUTOINS_TAG_LEN + 2 are always <= MAX_CHARS
#ifdef INTEGRATED_SELECTOR
	static int prev_line = 0;
#endif
	bool redraw = TRUE, quit = FALSE;

	char tmp[160], buf[1024], *buf_ptr, c;
	char match_buf[AUTOINS_MATCH_LEN + 8], tag_buf[AUTOINS_TAG_LEN + 2];

	char fff[1024];
	static char search[MAX_CHARS] = { 0 };

#ifdef REGEX_SEARCH
	int ires = -999;
	regex_t re_src;
	char *regptr;
#endif

	/* Save screen */
	Term_save();

	/* Prevent hybrid macros from triggering in here */
	inkey_msg = TRUE;

#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
	inkey_interact_macros = TRUE;
#endif

	/* Process requests until done */
	while (1) {
		if (redraw) {
			/* Clear screen */
			Term_clear();

			/* Describe */
			Term_putstr(15,  0, -1, TERM_L_UMBER, format("*** Current Auto-Inscriptions List, page %d/%d ***", cur_page + 1, max_page + 1));
			Term_putstr(0, 21, -1, TERM_L_UMBER, "(\377yESC\377U/\377yg\377U/\377yG\377U/\377U/\377y8\377U/\377y2\377U/\377ySPC\377U/\377yBKSP\377U/\377yp\377U) nav, (\377yP\377U) chat-paste, (\377yf\377U/\377yb\377U/\377yt\377U) force/bags-only/toggle");
			Term_putstr(0, 22, -1, TERM_L_UMBER, "(\377y?\377U/\377ye\377U,\377yRET\377U/\377yI\377U/\377yX\377U/\377yd\377U/\377yc\377U) help/edit/insert/excise/delete/CLEAR, (\377ya\377U) auto-pick/des/ig");
			Term_putstr(0, 23, -1, TERM_L_UMBER, "(\377yw\377U/\377yx\377U) move, (\377y/\377U/\377y#\377U) find, (\377yl\377U/\377yL\377U/\377ys\377U/\377yS\377U/\377yA\377U) load/save '\377u.ins\377U'/'\377uglobal.ins\377U'/'\377u<class>.ins\377U'");

			for (i = 0; i < AUTOINS_PAGESIZE; i++) {
				cur_idx = cur_page * AUTOINS_PAGESIZE + i;
#ifdef INTEGRATED_SELECTOR
				if (i == cur_line) c = '4'; //TERM_SELECTOR
				else
#endif
				c = 's'; //TERM_SLATE
				/* build a whole line */
#ifdef AUTOINS_DIS_SUB_MATCH
				strcpy(match_buf, format("\377%c>\377%c", c, auto_inscription_disabled[cur_idx] ? 'D' : (auto_inscription_subinven[cur_idx] ? 'y' : 'w')));
#else
				strcpy(match_buf, format("\377%c>\377w", c));
#endif
#ifdef REGEX_SEARCH
				if (auto_inscription_invalid[cur_idx]) strcat(match_buf, "\377R");
#endif
				strcat(match_buf, auto_inscription_match[cur_idx]);
				strcpy(tag_buf, auto_inscription_tag[cur_idx]);
				sprintf(fff, "\377%c%3d %-59s %s%s\377%c>%-19s", auto_inscription_force[cur_idx] ? AUTOINS_FORCE_COL : 'w', cur_idx + 1, match_buf, /* spacing = AUTOINS_MATCH_LEN + 7 */
#ifdef REGEX_SEARCH
				    auto_inscription_invalid[cur_idx] ? "  " : "", /* silyl sprintf %- formatting, error colour code takes up 2 'chars' */
#else
				    "",
#endif
				    auto_inscription_autodestroy[cur_idx] ? "\377RA\377-" : (auto_inscription_autopickup[cur_idx] ? "\377Ga\377-" : (auto_inscription_ignore[cur_idx] ? "\377yi\377-" : " ")),
				    c, tag_buf);

#ifdef INTEGRATED_SELECTOR
 #ifdef AUTOINS_DIS_SUB_MATCH
				Term_putstr(0, i + 1, -1, TERM_WHITE, fff);
 #else
				Term_putstr(0, i + 1, -1, auto_inscription_disabled[cur_idx] ? TERM_L_DARK : (auto_inscription_subinven[cur_idx] ? TERM_YELLOW : TERM_WHITE), fff);
 #endif
#else
 #ifdef AUTOINS_DIS_SUB_MATCH
				Term_putstr(2, i + 1, -1, TERM_WHITE, fff);
 #else
				Term_putstr(2, i + 1, -1, auto_inscription_disabled[cur_idx] ? TERM_L_DARK : (auto_inscription_subinven[cur_idx] ? TERM_YELLOW : TERM_WHITE), fff);
 #endif
#endif
			}
		}
#ifdef INTEGRATED_SELECTOR
		else {
			for (i = 0; i < AUTOINS_PAGESIZE; i++) {
				cur_idx = cur_page * AUTOINS_PAGESIZE + i;

				if (i == cur_line) c = '4'; //TERM_SELECTOR
				else if (i == prev_line) c = 's'; //TERM_SLATE
				else continue;
				/* build a whole line */
 #ifdef AUTOINS_DIS_SUB_MATCH
				strcpy(match_buf, format("\377%c>\377%c", c, auto_inscription_disabled[cur_idx] ? 'D' : (auto_inscription_subinven[cur_idx] ? 'y' : 'w')));
 #else
				strcpy(match_buf, format("\377%c>\377w", c));
 #endif
 #ifdef REGEX_SEARCH
				if (auto_inscription_invalid[cur_idx]) strcat(match_buf, "\377R");
 #endif
				strcat(match_buf, auto_inscription_match[cur_idx]);
				strcpy(tag_buf, auto_inscription_tag[cur_idx]);
				sprintf(fff, "\377%c%3d %-59s %s%s\377%c>%-19s", auto_inscription_force[cur_idx] ? AUTOINS_FORCE_COL : 'w', cur_idx + 1, match_buf, /* spacing = AUTOINS_MATCH_LEN + 7 */
 #ifdef REGEX_SEARCH
				    auto_inscription_invalid[cur_idx] ? "  " : "", /* silyl sprintf %- formatting, error colour code takes up 2 'chars'  */
 #else
				    "",
 #endif
				    auto_inscription_autodestroy[cur_idx] ? "\377RA\377-" : (auto_inscription_autopickup[cur_idx] ? "\377Ga\377-" : (auto_inscription_ignore[cur_idx] ? "\377yi\377-" : " ")),
				    c, tag_buf);

 #ifdef AUTOINS_DIS_SUB_MATCH
				Term_putstr(0, i + 1, -1, TERM_WHITE, fff);
 #else
				Term_putstr(0, i + 1, -1, auto_inscription_disabled[cur_idx] ? TERM_L_DARK : (auto_inscription_subinven[cur_idx] ? TERM_YELLOW : TERM_WHITE), fff);
 #endif
			}
		}
		prev_line = cur_line;
#endif

		cur_idx = cur_page * AUTOINS_PAGESIZE + cur_line;
		redraw = TRUE;

		/* display editing 'cursor' */
#ifndef INTEGRATED_SELECTOR
		Term_putstr(0, cur_line + 1, -1, TERM_SELECTOR, ">");
#endif
		/* make cursor invisible */
		Term_set_cursor(0);
		inkey_flag = TRUE;

		/* Wait for keypress */
		switch (inkey_combo(FALSE, NULL, NULL)) {
		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", 2);
			redraw = FALSE;
			break;
		case ':':
			/* Allow to chat, to tell exact inscription-related stuff to other people easily */
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
			break;
		case '{':
			cmd_inscribe(USE_INVEN);
			break;
		case '}':
			cmd_uninscribe(USE_INVEN);
			break;
		case '?':
		case 'h':
			Term_clear();
			topline_icky = TRUE;
			i = 0;

			Term_putstr( 0, i++, -1, TERM_SLATE, "Editing item name matches (left column) and applied inscription (right column):");
			//i++;

			Term_putstr( 0, i++, -1, TERM_YELLOW, "Editing item name matches:");
			Term_putstr( 0, i++, -1, TERM_WHITE, "The item name you enter is used as a partial match, case-sensitively.");
			Term_putstr( 0, i++, -1, TERM_WHITE, "You can use '#' as wildcards, eg \"Rod#Healing\".");
#ifdef REGEX_SEARCH
			Term_putstr( 0, i++, -1, TERM_WHITE, "If you prefix a line with '$' the string will be interpreted as regexp.");
			Term_putstr( 0, i++, -1, TERM_WHITE, "In a regexp string, the '#' will no longer have a wildcard function.");
			Term_putstr( 0, i++, -1, TERM_WHITE, "Regular expressions are parsed case-insensitively instead.");
#endif
			//i++;

			Term_putstr( 0, i++, -1, TERM_YELLOW, "Editing item inscriptions to be applied:");
			Term_putstr( 0, i++, -1, TERM_WHITE, "It works the same as manual inscriptions. So you could even specify \"@@\"");
			Term_putstr( 0, i++, -1, TERM_WHITE, "if you want a power-inscription. There is only one special rule:");
			Term_putstr( 0, i++, -1, TERM_WHITE, "A matched entry that is set to '\377yi\377w'gnore will be skipped for the purpose");
			Term_putstr( 0, i++, -1, TERM_WHITE, "of inscribing if the tag is empty, and only work for auto-pickup/destroy.");
			Term_putstr( 0, i++, -1, TERM_WHITE, "Press 'f' to toggle force-inscribing, which will overwrite an existing");
			Term_putstr( 0, i++, -1, TERM_WHITE, "Inscription always. Otherwise just trivial ones, eg discount tags.");
			Term_putstr( 0, i++, -1, TERM_WHITE, "An orange tag text colour (instead of white) indicates forced-mode.");//AUTOINS_FORCE_COL
			Term_putstr( 0, i++, -1, TERM_WHITE, "Press 'b' to toggle an inscription to ONLY apply on items inside bags.");
			Term_putstr( 0, i++, -1, TERM_WHITE, "Press 't' to toggle an inscription (disables/re-enables it).");
			//i++;

			Term_putstr( 0, i++, -1, TERM_YELLOW, "Troubleshooting:");
			Term_putstr( 0, i++, -1, TERM_WHITE, "The auto-inscription menu tries to load only the most specific .ins file");
			Term_putstr( 0, i++, -1, TERM_WHITE, "and discard all other ones, the order from most specific to least is:");
			Term_putstr( 0, i++, -1, TERM_WHITE, "<charname>.ins, <class>.ins, <trait>.ins, <race>.ins, global.ins.");

			Term_putstr(25, 23, -1, TERM_L_BLUE, "(Press any key to go back)");
			inkey();

			topline_icky = FALSE;
			break;
		case '/': // search
			redraw = TRUE;
			Term_putstr(0, 23, -1, TERM_YELLOW, "Enter search term: ");
			if (!askfor_aux(search, MAX_CHARS, 0)) continue;

			for (i = 0; i < MAX_AUTO_INSCRIPTIONS; i++)
				if (my_strcasestr(auto_inscription_match[(cur_idx + 1 + i) % MAX_AUTO_INSCRIPTIONS], search)) break;
			/* If we're not looking for subsequent results, leave the options page instead of wrapping around */
			if (i != MAX_AUTO_INSCRIPTIONS) {
				cur_idx = (cur_idx + 1 + i) % MAX_AUTO_INSCRIPTIONS;
				cur_line = (cur_idx % AUTOINS_PAGESIZE);
				cur_page = cur_idx / AUTOINS_PAGESIZE;
			}
			break;
		case '#': // jump to line #
			redraw = TRUE;
			Term_putstr(0, 23, -1, TERM_YELLOW, format("Enter line number (1-%d): ", MAX_AUTO_INSCRIPTIONS));
			if (!askfor_aux(tmp, MAX_CHARS, 0)) continue;

			i = atoi(tmp);
			if (i < 1 || i > MAX_AUTO_INSCRIPTIONS) {
				c_msg_print("\377yInvalid line number.");
				continue;
			}

			cur_idx = i - 1;
			cur_line = (cur_idx % AUTOINS_PAGESIZE);
			cur_page = cur_idx / AUTOINS_PAGESIZE;
			break;
		case 'P':
			/* Paste currently selected entry to chat */
			strcpy(match_buf, "\377s<\377w");
			strcat(match_buf, auto_inscription_match[cur_idx]);
			strcat(match_buf, "\377s>");
			strcpy(tag_buf, auto_inscription_tag[cur_idx]);
			sprintf(tmp, "\377sAuto-inscription %3d: %s%s<\377%c%s\377s>", cur_idx + 1,
			    match_buf,
			    auto_inscription_autodestroy[cur_idx] ? "\377RA\377s" : (auto_inscription_autopickup[cur_idx] ? "\377Ga\377s" : (auto_inscription_ignore[cur_idx] ? "\377yi\377s" : " ")),
			    auto_inscription_force[cur_idx] ? AUTOINS_FORCE_COL : 'w',
			    tag_buf);
			Send_paste_msg(tmp);
			redraw = FALSE;
			break;
		case ESCAPE:
			quit = TRUE; /* hack to leave loop */
			break;
		case '3': //pgdn
		case 'n':
		case ' ':
		case NAVI_KEY_PAGEDOWN:
			cur_page++;
			if (cur_page > max_page) cur_page = 0;
			redraw = TRUE;
			break;
		case '9': //pgup
		case 'p':
		case '\b':
		case NAVI_KEY_PAGEUP:
			cur_page--;
			if (cur_page < 0) cur_page = max_page;
			redraw = TRUE;
			break;
		case 'G':
		case '1': //end
		case NAVI_KEY_END:
			cur_page = max_page;
			cur_line = AUTOINS_PAGESIZE - 1;
			redraw = TRUE;
			break;
		case 'g':
		case '7': //home
		case NAVI_KEY_POS1:
			cur_page = 0;
			cur_line = 0;
			redraw = TRUE;
			break;
		case '2':
		case NAVI_KEY_DOWN:
#ifndef INTEGRATED_SELECTOR
			Term_putstr(0, cur_line + 1, -1, TERM_SELECTOR, " ");
#endif
			cur_line++;
			redraw = FALSE;
			if (cur_line >= AUTOINS_PAGESIZE) {
				cur_line = 0;
				cur_page++;
				if (cur_page > max_page) cur_page = 0;
				redraw = TRUE;
			}
			break;
		case '8':
		case NAVI_KEY_UP:
#ifndef INTEGRATED_SELECTOR
			Term_putstr(0, cur_line + 1, -1, TERM_SELECTOR, " ");
#endif
			cur_line--;
			redraw = FALSE;
			if (cur_line < 0) {
				cur_line = AUTOINS_PAGESIZE - 1;
				cur_page--;
				if (cur_page < 0) cur_page = max_page;
				redraw = TRUE;
			}
			break;
		case 'x':
			if (cur_idx >= MAX_AUTO_INSCRIPTIONS - 1) break;

			cur_line++;
			redraw = FALSE;
			if (cur_line >= AUTOINS_PAGESIZE) {
				cur_line = 0;
				cur_page++;
				if (cur_page > max_page) cur_page = 0;
				redraw = TRUE;
			}

			strcpy(tempbuf, auto_inscription_match[cur_idx]);
			strcpy(auto_inscription_match[cur_idx], auto_inscription_match[cur_idx + 1]);
			strcpy(auto_inscription_match[cur_idx + 1], tempbuf);

			strcpy(tempbuf, auto_inscription_tag[cur_idx]);
			strcpy(auto_inscription_tag[cur_idx], auto_inscription_tag[cur_idx + 1]);
			strcpy(auto_inscription_tag[cur_idx + 1], tempbuf);

			temp = auto_inscription_autodestroy[cur_idx];
			auto_inscription_autodestroy[cur_idx] = auto_inscription_autodestroy[cur_idx + 1];
			auto_inscription_autodestroy[cur_idx + 1] = temp;

			temp = auto_inscription_autopickup[cur_idx];
			auto_inscription_autopickup[cur_idx] = auto_inscription_autopickup[cur_idx + 1];
			auto_inscription_autopickup[cur_idx + 1] = temp;

			temp = auto_inscription_ignore[cur_idx];
			auto_inscription_ignore[cur_idx] = auto_inscription_ignore[cur_idx + 1];
			auto_inscription_ignore[cur_idx + 1] = temp;

			temp = auto_inscription_force[cur_idx];
			auto_inscription_force[cur_idx] = auto_inscription_force[cur_idx + 1];
			auto_inscription_force[cur_idx + 1] = temp;

#ifdef REGEX_SEARCH
			temp = auto_inscription_invalid[cur_idx];
			auto_inscription_invalid[cur_idx] = auto_inscription_invalid[cur_idx + 1];
			auto_inscription_invalid[cur_idx + 1] = temp;
#endif

			temp = auto_inscription_subinven[cur_idx]; //ENABLE_SUBINVEN
			auto_inscription_subinven[cur_idx] = auto_inscription_subinven[cur_idx + 1];
			auto_inscription_subinven[cur_idx + 1] = temp;

			temp = auto_inscription_disabled[cur_idx];
			auto_inscription_disabled[cur_idx] = auto_inscription_disabled[cur_idx + 1];
			auto_inscription_disabled[cur_idx + 1] = temp;
			break;
		case 'w':
			if (!cur_idx) break;

			cur_line--;
			redraw = FALSE;
			if (cur_line < 0) {
				cur_line = AUTOINS_PAGESIZE - 1;
				cur_page--;
				if (cur_page < 0) cur_page = max_page;
				redraw = TRUE;
			}

			strcpy(tempbuf, auto_inscription_match[cur_idx]);
			strcpy(auto_inscription_match[cur_idx], auto_inscription_match[cur_idx - 1]);
			strcpy(auto_inscription_match[cur_idx - 1], tempbuf);

			strcpy(tempbuf, auto_inscription_tag[cur_idx]);
			strcpy(auto_inscription_tag[cur_idx], auto_inscription_tag[cur_idx - 1]);
			strcpy(auto_inscription_tag[cur_idx - 1], tempbuf);

			temp = auto_inscription_autodestroy[cur_idx];
			auto_inscription_autodestroy[cur_idx] = auto_inscription_autodestroy[cur_idx - 1];
			auto_inscription_autodestroy[cur_idx - 1] = temp;

			temp = auto_inscription_autopickup[cur_idx];
			auto_inscription_autopickup[cur_idx] = auto_inscription_autopickup[cur_idx - 1];
			auto_inscription_autopickup[cur_idx - 1] = temp;

			temp = auto_inscription_ignore[cur_idx];
			auto_inscription_ignore[cur_idx] = auto_inscription_ignore[cur_idx - 1];
			auto_inscription_ignore[cur_idx - 1] = temp;

			temp = auto_inscription_force[cur_idx];
			auto_inscription_force[cur_idx] = auto_inscription_force[cur_idx - 1];
			auto_inscription_force[cur_idx - 1] = temp;

#ifdef REGEX_SEARCH
			temp = auto_inscription_invalid[cur_idx];
			auto_inscription_invalid[cur_idx] = auto_inscription_invalid[cur_idx - 1];
			auto_inscription_invalid[cur_idx - 1] = temp;
#endif

			temp = auto_inscription_subinven[cur_idx]; //ENABLE_SUBINVEN
			auto_inscription_subinven[cur_idx] = auto_inscription_subinven[cur_idx - 1];
			auto_inscription_subinven[cur_idx - 1] = temp;

			temp = auto_inscription_disabled[cur_idx];
			auto_inscription_disabled[cur_idx] = auto_inscription_disabled[cur_idx - 1];
			auto_inscription_disabled[cur_idx - 1] = temp;
			break;
		case 'l':
			/* Prompt */
			clear_from(21);
			Term_putstr(0, 22, -1, TERM_L_GREEN, "*** Load an .ins file ***");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 23, -1, TERM_WHITE, "File: ");

			sprintf(tmp, "%s.ins", cname);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Process the given filename */
			if (load_auto_inscriptions(tmp)) apply_auto_inscriptions(-1);
			break;
		case 'L':
			/* Prompt */
			clear_from(21);
			Term_putstr(0, 22, -1, TERM_L_GREEN, "*** Load account-wide 'global.ins' file ***");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 23, -1, TERM_WHITE, "File: ");

			strcpy(tmp, "global.ins");

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Process the given filename */
			if (load_auto_inscriptions(tmp)) apply_auto_inscriptions(-1);
			break;
		case 's':
			/* Prompt */
			clear_from(21);
			Term_putstr(0, 22, -1, TERM_L_GREEN, "*** Save an .ins file ('global.ins' for account-wide) ***");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 23, -1, TERM_WHITE, "File: ");

			sprintf(tmp, "%s.ins", cname);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Dump the macros */
			save_auto_inscriptions(tmp);
			break;
		case 'S':
			/* Prompt */
			clear_from(21);
			Term_putstr(0, 22, -1, TERM_L_GREEN, "*** Save to account-wide 'global.ins' file ***");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 23, -1, TERM_WHITE, "File: ");

			strcpy(tmp, "global.ins");

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Dump the macros */
			save_auto_inscriptions(tmp);
			break;
		case 'A':
			/* Prompt */
			clear_from(21);
			Term_putstr(0, 22, -1, TERM_L_GREEN, "*** Save to class-specific inscription file ***");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 23, -1, TERM_WHITE, "File: ");

			strcpy(tmp, format("%s.ins", p_ptr->cp_ptr->title));

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Dump the macros */
			save_auto_inscriptions(tmp);
			break;
		case 'e':
		case '6':
		case '\n':
		case '\r':
		case NAVI_KEY_RIGHT:
//INTEGRATED_SELECTOR: - 2
			/* Clear previous matching string */
			Term_putstr(6 - 2, cur_line + 1, -1, TERM_L_GREEN, "                                                       ");
			/* Go to the correct location */
			Term_gotoxy(7 - 2, cur_line + 1);
			strcpy(buf, auto_inscription_match[cur_idx]);
			/* Get a new matching string */
			if (!askfor_aux(buf, AUTOINS_MATCH_LEN - 1, 0)) continue;

			buf_ptr = buf;
			/* special hack: allow just a '#' match */
			if (strcmp(buf, "#") && strcmp(buf, "!#")) {
				/* hack: remove leading/trailing wild cards since they are obsolete.
				   Especially trailing ones currently make it not work. */
				while (*buf_ptr == '#') buf_ptr++;
				while (*(buf_ptr + strlen(buf_ptr) - 1) == '#') *(buf_ptr + strlen(buf_ptr) - 1) = '\0';
			}

//INTEGRATED_SELECTOR: - 2
			Term_putstr(6 - 2, cur_line + 1, -1, TERM_L_GREEN, "                                                       ");
			Term_putstr(7 - 2, cur_line + 1, -1, TERM_WHITE, buf_ptr);
			/* ok */
			strcpy(auto_inscription_match[cur_idx], buf_ptr);

#ifdef REGEX_SEARCH
			/* Actually test regexp for validity right away, so we can avoid spam/annoyance/searching later. */
			/* Check for '$' prefix, forcing regexp interpretation */
			regptr = buf_ptr;
			//legacy mode --  if (regptr[0] == '!') regptr++;
			if (regptr[0] == '$') {
				regptr++;
				ires = regcomp(&re_src, regptr, REG_EXTENDED | REG_ICASE);
				if (ires != 0) {
					auto_inscription_invalid[cur_idx] = TRUE;
					c_msg_format("\377oInvalid regular expression in auto-inscription #%d.", cur_idx + 1);
					/* Re-colour the line to indicate error */
					Term_putstr(5, cur_line + 1, -1, TERM_L_RED, buf_ptr);
				} else auto_inscription_invalid[cur_idx] = FALSE;
				regfree(&re_src);
			}
#endif

			/* Clear previous tag string */
			Term_putstr(61, cur_line + 1, -1, TERM_L_GREEN, "                  ");
			/* Go to the correct location */
			Term_gotoxy(62, cur_line + 1);
			strcpy(buf, auto_inscription_tag[cur_idx]);
			/* Get a new tag string */
			if (!askfor_aux(buf, AUTOINS_TAG_LEN - 1, 0)) {
				/* in case match was changed, we may also need to reapply */
				apply_auto_inscriptions(cur_idx);
				continue;
			}
			strcpy(auto_inscription_tag[cur_idx], buf);
			apply_auto_inscriptions(cur_idx);

			/* comfort hack - fake advancing ;) */
#ifndef INTEGRATED_SELECTOR
			Term_putstr(0, cur_line + 1, -1, TERM_SELECTOR, " ");
#endif
			cur_line++;
			if (cur_line >= AUTOINS_PAGESIZE) {
				cur_line = 0;
				cur_page++;
				if (cur_page > max_page) cur_page = 0;
				redraw = TRUE;
			}
			break;
		case 'd':
			auto_inscription_match[cur_idx][0] = auto_inscription_tag[cur_idx][0] = 0;
			auto_inscription_autopickup[cur_idx] = auto_inscription_autodestroy[cur_idx] = auto_inscription_ignore[cur_idx] = FALSE;
			auto_inscription_force[cur_idx] = FALSE;
#ifdef REGEX_SEARCH
			auto_inscription_invalid[cur_idx] = FALSE;
#endif
			auto_inscription_subinven[cur_idx] = FALSE;
			auto_inscription_disabled[cur_idx] = FALSE;
#if 0
			/* also auto-advance by 1 line, for convenience */
			cur_line++;
			if (cur_line >= AUTOINS_PAGESIZE) {
				cur_line = 0;
				cur_page++;
				if (cur_page > max_page) cur_page = 0;
				redraw = TRUE;
			}
#endif
			break;
		case 'X':
			/* Slide rest up.. */
			for (i = cur_idx; i < MAX_AUTO_INSCRIPTIONS - 1; i++) {
				strcpy(auto_inscription_match[i], auto_inscription_match[i + 1]);
				strcpy(auto_inscription_tag[i], auto_inscription_tag[i + 1]);
				auto_inscription_autopickup[i] = auto_inscription_autopickup[i + 1];
				auto_inscription_autodestroy[i] = auto_inscription_autodestroy[i + 1];
				auto_inscription_ignore[i] = auto_inscription_ignore[i + 1];
				auto_inscription_force[i] = auto_inscription_force[i + 1];
#ifdef REGEX_SEARCH
				auto_inscription_invalid[i] = auto_inscription_invalid[i + 1];
#endif
				auto_inscription_subinven[i] = auto_inscription_subinven[i + 1];
				auto_inscription_disabled[i] = auto_inscription_disabled[i + 1];
			}
			/* ..and delete the last one */
			auto_inscription_match[i][0] = auto_inscription_tag[i][0] = 0;
			auto_inscription_autopickup[i] = auto_inscription_autodestroy[i] = auto_inscription_ignore[i] = FALSE;
			auto_inscription_force[i] = FALSE;
#ifdef REGEX_SEARCH
			auto_inscription_invalid[i] = FALSE;
#endif
			auto_inscription_subinven[i] = FALSE;
			auto_inscription_disabled[i] = FALSE;
			break;
		case 'I':
			/* Check if the last inscription is free */
			if (auto_inscription_match[MAX_AUTO_INSCRIPTIONS - 1][0]) {
				c_msg_format("\377yThe last auto-inscription (#%d) must be unused in order to insert a line!", MAX_AUTO_INSCRIPTIONS);
				break;
			}
			/* Slide all down.. */
			for (i = MAX_AUTO_INSCRIPTIONS - 1; i > cur_idx; i--) {
				strcpy(auto_inscription_match[i], auto_inscription_match[i - 1]);
				strcpy(auto_inscription_tag[i], auto_inscription_tag[i - 1]);
				auto_inscription_autopickup[i] = auto_inscription_autopickup[i - 1];
				auto_inscription_autodestroy[i] = auto_inscription_autodestroy[i - 1];
				auto_inscription_ignore[i] = auto_inscription_ignore[i - 1];
				auto_inscription_force[i] = auto_inscription_force[i - 1];
#ifdef REGEX_SEARCH
				auto_inscription_invalid[i] = auto_inscription_invalid[i - 1];
#endif
				auto_inscription_subinven[i] = auto_inscription_subinven[i - 1];
				auto_inscription_disabled[i] = auto_inscription_disabled[i - 1];
			}
			/* ..and clear the current line */
			auto_inscription_match[i][0] = auto_inscription_tag[i][0] = 0;
			auto_inscription_autopickup[i] = auto_inscription_autodestroy[i] = auto_inscription_ignore[i] = FALSE;
			auto_inscription_force[i] = FALSE;
#ifdef REGEX_SEARCH
			auto_inscription_invalid[i] = FALSE;
#endif
			auto_inscription_subinven[i] = FALSE;
			auto_inscription_disabled[i] = FALSE;
			break;
		case 'c':
			for (i = 0; i < MAX_AUTO_INSCRIPTIONS; i++) {
				auto_inscription_match[i][0] = auto_inscription_tag[i][0] = 0;
				auto_inscription_autopickup[i] = auto_inscription_autodestroy[i] = auto_inscription_ignore[i] = FALSE;
				auto_inscription_force[i] = FALSE;
#ifdef REGEX_SEARCH
				auto_inscription_invalid[i] = FALSE;
#endif
				auto_inscription_subinven[i] = FALSE;
				auto_inscription_disabled[i] = FALSE;
			}
			/* comfort hack - jump to first line */
#ifndef INTEGRATED_SELECTOR
			Term_putstr(0, cur_line + 1, -1, TERM_SELECTOR, " ");
#endif
			cur_line = cur_page = 0;
			redraw = TRUE;
			break;
		case 'a':
			/* cycle: nothing / auto-pickup / auto-destroy / ignore */
			if (auto_inscription_ignore[cur_idx]) {
				auto_inscription_ignore[cur_idx] = FALSE;
				auto_inscription_autopickup[cur_idx] = auto_inscription_autodestroy[cur_idx] = FALSE; /* <- shouldn't happen (someone messed up his .ins) */
			} else if (auto_inscription_autodestroy[cur_idx]) {
				auto_inscription_autodestroy[cur_idx] = FALSE;
				auto_inscription_ignore[cur_idx] = TRUE;
				auto_inscription_autopickup[cur_idx] = FALSE; /* <- shouldn't happen (someone messed up his .ins) */
			} else if (auto_inscription_autopickup[cur_idx]) {
				auto_inscription_autopickup[cur_idx] = FALSE;
				auto_inscription_autodestroy[cur_idx] = TRUE;
				auto_inscription_ignore[cur_idx] = FALSE; /* <- shouldn't happen (someone messed up his .ins) */
			} else auto_inscription_autopickup[cur_idx] = TRUE;
			redraw = TRUE;
			break;
		case 'f':
			/* toggle force-inscribe (same as '!' prefix) */
			auto_inscription_force[cur_idx] = !auto_inscription_force[cur_idx];
			/* if we changed to 'forced', we may need to reapply - note that competing inscriptions aren't well-defined here */
			if (auto_inscription_force[cur_idx]) apply_auto_inscriptions(cur_idx);
			redraw = TRUE;
			break;
		case 'b':
			/* toggle bags-only */
			auto_inscription_subinven[cur_idx] = !auto_inscription_subinven[cur_idx];
			/* if we changed to 'all' and it was 'forced' too, we may need to reapply - note that competing inscriptions aren't well-defined here */
			if (!auto_inscription_subinven[cur_idx] && auto_inscription_force[cur_idx]) apply_auto_inscriptions(cur_idx);
			redraw = TRUE;
			break;
		case 't':
			/* toggle disabled-state */
			auto_inscription_disabled[cur_idx] = !auto_inscription_disabled[cur_idx];
			/* if we changed to 're-enable' and it was 'forced' too, we may need to reapply - note that competing inscriptions aren't well-defined here */
			if (!auto_inscription_disabled[cur_idx] && auto_inscription_force[cur_idx]) apply_auto_inscriptions(cur_idx);
			redraw = TRUE;
			break;
		default:
			/* Oops */
			bell();
		}

		/* Leave */
		if (quit) break;

	}

	/* Reload screen */
	Term_load();

	/* Flush the queue */
	Flush_queue();

	/* Re-enable hybrid macros */
	inkey_msg = FALSE;

#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
	inkey_interact_macros = FALSE;
#endif
}

/* Apply all (insc_idx = -1) or one specific (insc_idx) auto-inscription to the complete inventory */
void apply_auto_inscriptions(int insc_idx) {
	int i;
#ifdef ENABLE_SUBINVEN
	int s;
#endif

	if (c_cfg.auto_inscr_off) return;

	for (i = 0; i < INVEN_TOTAL; i++) {
		(void)apply_auto_inscriptions_aux(i, insc_idx, FALSE);
#ifdef ENABLE_SUBINVEN
		if (inventory[i].tval == TV_SUBINVEN)
			for (s = 0; s < inventory[i].bpval; s++)
				(void)apply_auto_inscriptions_aux((i + 1) * SUBINVEN_INVEN_MUL + s, insc_idx, FALSE);
#endif
	}
}


/* Helper function for option manipulation - check before and after, and refresh stuff if the changes made require it.
   init: TRUE before an option gets changed, FALSE afterwards. */
void options_immediate(bool init) {
	static bool changed1, changed2, changed3, changed3a;
	static bool changed4a, changed4b, changed4c;
	static bool changed5, changed5a, changed6;
	static bool changed7, changed8;
	static bool changed9a, changed9b, changed9c, changed9d;

#if !defined(WINDOWS) && !defined(USE_X11)
	/* Assume GCU-only client - terminal will break with "^B" visuals if font_map_solid_walls is on, so disable it always: */
	if (c_cfg.font_map_solid_walls) {
		c_msg_print("\377yOption 'font_map_solid_walls' is not supported on GCU-only client.");
		c_cfg.font_map_solid_walls = FALSE;
		(*option_info[CO_FONT_MAP_SOLID_WALLS].o_var) = FALSE;
		Client_setup.options[CO_FONT_MAP_SOLID_WALLS] = FALSE;
	}
#endif

#if defined(WINDOWS) && defined(USE_LOGFONT)
	if (use_logfont && c_cfg.font_map_solid_walls) {
		c_msg_print("\377yOption 'font_map_solid_walls' is not supported with logfont.");
		c_cfg.font_map_solid_walls = FALSE;
		(*option_info[CO_FONT_MAP_SOLID_WALLS].o_var) = FALSE;
		Client_setup.options[CO_FONT_MAP_SOLID_WALLS] = FALSE;
	}
	if (use_logfont && c_cfg.solid_bars) {
		c_msg_print("\377yOption 'solid_bars' is not supported with logfont.");
		c_cfg.solid_bars = FALSE;
		(*option_info[CO_SOLID_BARS].o_var) = FALSE;
		Client_setup.options[CO_SOLID_BARS] = FALSE;
	}
#endif

	if (init) {
		changed1 = c_cfg.exp_need; changed2 = c_cfg.exp_bar; changed3 = c_cfg.solid_bars; changed3a = c_cfg.huge_bars_gfx;
		changed4a = c_cfg.hp_bar; changed4b = c_cfg.mp_bar; changed4c = c_cfg.st_bar;
		changed5 = c_cfg.equip_text_colour;
		changed5a = c_cfg.equip_set_colour;
		changed6 = c_cfg.colourize_bignum;
		changed7 = c_cfg.load_form_macros;
		changed8 = c_cfg.auto_inscr_off;
		changed9a = c_cfg.ascii_feats; changed9b = c_cfg.ascii_items; changed9c = c_cfg.ascii_monsters; changed9d = c_cfg.ascii_uniques;
		return;
	}

	/* for exp_need option changes: */
	if (changed1 != c_cfg.exp_need || changed2 != c_cfg.exp_bar || changed3 != c_cfg.solid_bars)
		prt_level(p_ptr->lev, p_ptr->max_lev, p_ptr->max_plv, p_ptr->max_exp, p_ptr->exp, exp_adv, exp_adv_prev);
	/* in case hp/mp/st are displayed as bars,
	   or hp/mp/st have just been switched between number form and bar form */
	if (changed3 != c_cfg.solid_bars || changed3a != c_cfg.huge_bars_gfx ||
	    changed4a != c_cfg.hp_bar || changed4b != c_cfg.mp_bar || changed4c != c_cfg.st_bar) {
		if (changed4a != c_cfg.hp_bar) hp_bar = c_cfg.hp_bar;
		if (changed4b != c_cfg.mp_bar) mp_bar = c_cfg.mp_bar;
		if (changed4c != c_cfg.st_bar) st_bar = c_cfg.st_bar;
		prt_hp(hp_max, hp_cur, hp_bar, hp_boosted);
		prt_mp(mp_max, mp_cur, mp_bar);
		prt_stamina(st_max, st_cur, st_bar);
		prt_stun(-1); //redraw
	}
	if (changed3 != c_cfg.solid_bars) {
		if (screen_icky) Term_switch(0);
		prt_sane(c_p_ptr->sanity_attr, c_p_ptr->sanity);
		if (screen_icky) Term_switch(0);
	}
	if (changed5 != c_cfg.equip_text_colour) p_ptr->window |= PW_EQUIP;
	if (changed5a != c_cfg.equip_set_colour) p_ptr->window |= PW_EQUIP;
	if (changed6 != c_cfg.colourize_bignum) {
		prt_gold(p_ptr->au);
		prt_level(p_ptr->lev, p_ptr->max_lev, p_ptr->max_plv, p_ptr->max_exp, p_ptr->exp, exp_adv, exp_adv_prev);
	}
	if (changed7 != c_cfg.load_form_macros && c_cfg.load_form_macros) {
		char tmp[MAX_CHARS];

		if (strcmp(c_p_ptr->body_name, "Player")) sprintf(tmp, "%s%c%s.prf", cname, PRF_BODY_SEPARATOR, c_p_ptr->body_name);
		else sprintf(tmp, "%s.prf", cname);
		(void)process_pref_file(tmp);
	}
	if (changed8 != c_cfg.auto_inscr_off && !c_cfg.auto_inscr_off) apply_auto_inscriptions(-1);
	if (changed9a != c_cfg.ascii_feats || changed9b != c_cfg.ascii_items || changed9c != c_cfg.ascii_monsters || changed9d != c_cfg.ascii_uniques) Send_redraw(0);
}

/*
 * Interact with some options
 */
static bool do_cmd_options_aux(int page, cptr info, int select) {
	char	ch;
	int	i, k, n = 0, k_no_advance;
	static int k_lasttime[OPT_PAGES + 1] = { 0 }; //+1: page 1 is actually indexed at 1, not at 0
	int	opt[24];
	char	buf[256];
	bool	tmp;

	if (select != -1) k_lasttime[page] = select;
	k = k_no_advance = k_lasttime[page];

	/* Lookup the options on this particular page (actually it's just 22 options per page, not 24) */
	for (i = 0; i < 24; i++) opt[i] = 0;

	/* Scan the options */
	for (i = 0; option_info[i].o_desc; i++) {
		/* Notice options on this "page" */
		if (option_info[i].o_page == page &&
		    option_info[i].o_enabled) opt[n++] = i;
	}

	/* Clear screen */
	Term_clear();

	/* Interact with the player */
	while (TRUE) {
		if (select == -1)
			//sprintf(buf, "%s (\377yUp/Down\377w, \377yy\377w/\377yn\377w/\377yLeft\377w/\377yRight\377w set, \377yt\377w/\377yA\377w-\377yZ\377w toggle, \377yESC\377w accept)", info);
			sprintf(buf, "%s (\377yy\377w/\377yn\377w/\377yLeft\377w/\377yRight\377w set, \377yt\377w/\377yA\377w-\377yZ\377w toggle, \377yESC\377w accept)", info);
		else
			//sprintf(buf, "%s (\377yy\377w/\377yn\377w/\377yLeft\377w/\377yRight\377w set, \377yt\377w/\377yA\377w-\377yZ\377w toggle, \377y/\377w next, \377yESC\377w accept)", info);
			sprintf(buf, "%s (\377yy\377w/\377yn\377w/\377yLeft\377w/\377yRight\377w set, \377yt\377w/\377yA\377w-\377yZ\377w toggle, \377y/\377w next, \377yESC\377w accept)", info);
		//prompt_topline(buf);
		Term_putstr(0, 0, -1, TERM_WHITE, buf);

		/* Display the options */
		for (i = 0; i < n; i++) {
			byte a = TERM_WHITE;

			/* Color current option */
			if (i == k) a = TERM_L_BLUE;

			/* Color disabled options */
			if (!option_info[opt[i]].o_enabled)
				a = (a == TERM_L_BLUE) ? TERM_SLATE : TERM_L_DARK;

			if (option_info[opt[i]].o_var == &c_cfg.font_map_solid_walls && !strcmp(ANGBAND_SYS, "gcu")) a = TERM_L_DARK;
#if defined(WINDOWS) && defined(USE_LOGFONT)
			if (use_logfont && (
			    option_info[opt[i]].o_var == &c_cfg.font_map_solid_walls ||
			    option_info[opt[i]].o_var == &c_cfg.solid_bars))
				a = TERM_L_DARK;
#endif

			/* Display the option text */
			sprintf(buf, "%-49s: %s  (%s)",
			        option_info[opt[i]].o_desc,
			        (*option_info[opt[i]].o_var ? "yes" : "no "),
			        option_info[opt[i]].o_text);
			c_prt(a, buf, i + 2, 0);
		}

		/* Hilite current option */
		move_cursor(k + 2, 50);

		/* Hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Get a key */
		ch = inkey();

		/* Analyze */
		switch (ch) {
		case ESCAPE:
		case KTRL('Q'):
			/* Next time on this options page, restore our position */
			k_lasttime[page] = k_no_advance;
			return (FALSE);
		case '/':
			k_lasttime[page] = k_no_advance;
			return (TRUE);

		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", 2);
			break;

		case ':': {
			bool inkey_msg_old = inkey_msg;

			/* specialty: allow chatting from within here */
			cmd_message();
			inkey_msg = inkey_msg_old; /* And suppress macros again.. */
			break;
			}

		case '-':
		case '8':
		case 'k':
		case '\010': //backspace
			k = (n + k - 1) % n;
			k_no_advance = k;
			break;

		case '+':
		case '2':
		case 'j':
		case '\n':
		case '\r':
		case ' ':
			k = (k + 1) % n;
			k_no_advance = k;
			break;

		case 'y':
		case '6':
		case 'l':
		case 's': //set
			(*option_info[opt[k]].o_var) = TRUE;
			Client_setup.options[opt[k]] = TRUE;
			check_immediate_options(opt[k], TRUE, TRUE);
			k_no_advance = k;
			k = (k + 1) % n;
			break;

		case 'n':
		case '4':
		case 'h':
		case 'u': //unset
			(*option_info[opt[k]].o_var) = FALSE;
			Client_setup.options[opt[k]] = FALSE;
			check_immediate_options(opt[k], FALSE, TRUE);
			k_no_advance = k;
			k = (k + 1) % n;
			break;

		case 't':
		case '5':
		case 'w': //switch
			tmp = 1 - (*option_info[opt[k]].o_var);
			(*option_info[opt[k]].o_var) = tmp;
			Client_setup.options[opt[k]] = tmp;
			check_immediate_options(opt[k], tmp, TRUE);
			k_no_advance = k;
			k = (k + 1) % n;
			break;
		default:
			/* directly toggle a specific option */
			if (ch >= 'A' && ch <= 'A' + n - 1) {
				k = ch - 'A';
				k_no_advance = k;
				tmp = 1 - (*option_info[opt[k]].o_var);
				(*option_info[opt[k]].o_var) = tmp;
				Client_setup.options[opt[k]] = tmp;
				check_immediate_options(opt[k], tmp, TRUE);
				break;
			}
			bell();
			break;
		}
	}
}


void display_account_information(void) {
	int l = 3;

	if (!acc_opt_screen) return;

	if (acc_got_info) {
		/* (Note: Flags that aren't displayed here yet but could be, if wanted:
		   ACC_MULTI (usually all admins, never players), ACC_NOSCORE (comes with TRIAL)) */
		if (acc_flags & ACC_TRIAL) c_prt(TERM_YELLOW, "Your account hasn't been validated.", l, 2);
		else c_prt(TERM_L_GREEN, "Your account is valid.", l, 2);

#if 1 /* display account flags as account info */
		l++;
		if (acc_flags & ACC_ADMIN) c_prt(TERM_BLUE, "Your account is an administrator.", l, 2);
		else if (acc_flags & ACC_VRESTRICTED) c_prt(TERM_ORANGE, "Your account is extra-restricted.", l, 2);
		else if (acc_flags & ACC_RESTRICTED) c_prt(TERM_YELLOW, "Your account is restricted.", l, 2);
		else if (acc_flags & ACC_VPRIVILEGED) c_prt(TERM_L_BLUE, "Your account is extra-privileged.", l, 2);
		else if (acc_flags & ACC_PRIVILEGED) c_prt(TERM_L_BLUE, "Your account is privileged.", l, 2);

		l++;
		if (acc_flags & ACC_ANOPVP) c_prt(TERM_RED, "Your account is not allowed under penalty to PvP.", l, 2);
		else if (acc_flags & ACC_NOPVP) c_prt(TERM_WHITE, "Your account cannot participate in PvP.", l, 2);
		else if (acc_flags & ACC_PVP) c_prt(TERM_SLATE, "Your account may participate in PvP.", l, 2);

		l++;
		if ((acc_flags & ACC_VQUIET) && !(acc_flags & ACC_QUIET)) c_prt(TERM_ORANGE, "Your account is silenced.", l, 2);
		else if ((acc_flags & ACC_QUIET) && !(acc_flags & ACC_VQUIET)) c_prt(TERM_YELLOW, "Your account is partially silenced.", l, 2);
		else if ((acc_flags & ACC_QUIET) && (acc_flags & ACC_VQUIET)) c_prt(TERM_YELLOW, "Your account is *silenced*.", l, 2);
#endif
	} else c_prt(TERM_SLATE, "Retrieving data...", l, 2);

	/* Added in 4.7.3 */
	l = 10;
	c_prt(TERM_WHITE, "Account-based character information:", l, 0);
	l += 2;
	if (p_ptr->admin_dm) c_prt(TERM_BLUE, 			"Dungeon Master                 ", l, 2);
	else if (p_ptr->admin_wiz) c_prt(TERM_BLUE, 		"Dungeon Wizard                 ", l, 2);
#if 0 /* display account flags as character info */
	else if (p_ptr->restricted == 2) c_prt(TERM_ORANGE, 	"Extra-restricted player account", l, 2);
	else if (p_ptr->restricted == 1) c_prt(TERM_YELLOW, 	"Restricted player account      ", l, 2);
	else if (p_ptr->privileged == 2) c_prt(TERM_L_BLUE, 	"Extra-privileged player account", l, 2);
	else if (p_ptr->privileged == 1) c_prt(TERM_L_BLUE, 	"Privileged player account      ", l, 2);
	else c_prt(TERM_L_GREEN, 				"Standard player account        ", l, 2);
#else
	else c_prt(TERM_L_GREEN, 				"Player character               ", l, 2);
#endif

	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;
}


/*
 * Account options
 */
static void do_cmd_options_acc(void) {
	char ch;
	bool change_pass = FALSE;
	bool go = TRUE;
	char old_pass[PASSWORD_LEN], new_pass[PASSWORD_LEN], con_pass[PASSWORD_LEN];
	char tmp[PASSWORD_LEN];

	/* suppress hybrid macros */
	bool inkey_msg_old = inkey_msg;
	inkey_msg = TRUE;


	acc_opt_screen = TRUE;
	acc_got_info = FALSE;

	/* Get the account info */
	Send_account_info();

	/* Clear screen */
	Term_clear();

	while (go) {
		/* Clear everything except the topline */
		clear_from(1);

		if (change_pass) {
			prt("Change password", 1, 0);
			prt(format("Length must be from %d to %d characters", PASSWORD_MIN_LEN, PASSWORD_LEN - 1), 3, 2);
			c_prt(TERM_L_WHITE, "Old password:", 6, 2);
			c_prt(TERM_L_WHITE, "New password:", 8, 2);
			c_prt(TERM_L_WHITE, "Confirm:", 10, 2);
			c_prt(TERM_L_WHITE, "(by typing the new password once more)", 11, 3);

			/* Ask for old password */
			move_cursor(6, 16);
			tmp[0] = '\0';
			if (!askfor_aux(tmp, PASSWORD_LEN - 1, ASKFOR_PRIVATE)) {
				change_pass = FALSE;
				continue;
			}
			if (!tmp[0]) {
				c_prt(TERM_YELLOW, "Password must not be empty", 8, 2);
				change_pass = FALSE;
				continue;
			}
			my_memfrob(tmp, strlen(tmp));
			strncpy(old_pass, tmp, sizeof(old_pass));
			old_pass[sizeof(old_pass) - 1] = '\0';

			while (1) {
				/* Ask for new password */
				move_cursor(8, 16);
				tmp[0] = '\0';
				if (!askfor_aux(tmp, PASSWORD_LEN - 1, ASKFOR_PRIVATE)) {
					change_pass = FALSE;
					break;
				}
				my_memfrob(tmp, strlen(tmp));
				strncpy(new_pass, tmp, sizeof(new_pass));
				new_pass[sizeof(new_pass) - 1] = '\0';

				/* Ask for the confirmation */
				move_cursor(10, 16);
				tmp[0] = '\0';
				if (!askfor_aux(tmp, PASSWORD_LEN - 1, ASKFOR_PRIVATE)) {
					change_pass = FALSE;
					break;
				}
				my_memfrob(tmp, strlen(tmp));
				strncpy(con_pass, tmp, sizeof(con_pass));
				con_pass[sizeof(con_pass) - 1] = '\0';

				/* Compare */
				if (strcmp(new_pass, con_pass)) {
					c_prt(TERM_YELLOW, "Passwords don't match!", 8, 2);
					Term_erase(16, 8, 255);
					Term_erase(16, 10, 255);
				} else break;
			}

			if (!change_pass) continue;

			/* Send the request */
			if (Send_change_password(old_pass, new_pass) == 1) {
				//c_msg_print("\377wPassword change has been submitted.");
#ifdef RETRY_LOGIN
				/* update password in memory so we can relogin */
				strcpy(pass, new_pass);
#endif
			} else {
				c_msg_print("\377yFailed to send password change request.");
			}

			/* Wipe the passwords from memory */
			memset(old_pass, 0, sizeof(old_pass));
			memset(new_pass, 0, sizeof(new_pass));
			memset(con_pass, 0, sizeof(con_pass));

			/* Return to the account options screen */
			change_pass = FALSE;
			continue;
		} else {
			prt("Account information", 1, 0);
			display_account_information();
			Term_putstr(2, 19, -1, TERM_WHITE, "(\377yC\377w) Change account password");
		}

		ch = inkey();

		switch (ch) {
			case ESCAPE:
				go = FALSE;
				break;
			case 'C':
				change_pass = TRUE;
				break;
		}
	}

	/* restore responsiveness to hybrid macros */
	inkey_msg = inkey_msg_old;

	acc_opt_screen = FALSE;
}


/*
 * Modify the "window" options
 */
static void do_cmd_options_win(void) {
	int i, j, d, vertikal_offset = 4;

	int y = 0;
	int x = 1;

	char ch;

	bool go = TRUE;

	u32b old_flag[ANGBAND_TERM_MAX];
	term *old;


	/* Memorize old flags */
	for (j = 1; j < ANGBAND_TERM_MAX; j++)
		/* Acquire current flags */
		old_flag[j] = window_flag[j];

	/* Clear screen */
	Term_clear();

	/* Interact */
	while (go) {
		/* Prompt XXX XXX XXX */
		Term_putstr(0, 0, -1, TERM_WHITE, "Window flags (<\377ydir\377w>, \377yt\377w (take), \377yy\377w (set), \377yn\377w (clear), \377yENTER\377w (toggle), \377yESC\377w) ");
		Term_putstr(0, 2, -1, TERM_SLATE, "-Contents to be displayed-                      -Window names-");

		/* Display the windows */
		for (j = 1; j < ANGBAND_TERM_MAX; j++) {
			byte a = TERM_WHITE;
			cptr s = ang_term_name[j];

			/* Use color */
			if (c_cfg.use_color && (j == x)) a = TERM_L_BLUE;

			/* Window name, staggered, centered */
			Term_putstr(30 + j * 5 - strlen(s) / 2, vertikal_offset + j % 2, -1, a, (char*)s);
		}

		/* Display the options */
		for (i = 0; i < NR_OPTIONS_SHOWN; i++) {
			byte a = TERM_WHITE;
			cptr str = window_flag_desc[i];

			/* Use color */
			if (c_cfg.use_color && (i == y)) a = TERM_L_BLUE;

			/* Unused option */
			if (!str) str = "\377D(Unused option)\377w";

			/* Flag name */
			Term_putstr(0, i + vertikal_offset + 2, -1, a, (char*)str);

			/* Display the windows */
			for (j = 1; j < ANGBAND_TERM_MAX; j++) {
				byte a = TERM_SLATE;
				char c = '.';

				/* Use color */
				if (c_cfg.use_color && (i == y) && (j == x)) a = TERM_L_BLUE;

				/* Active flag */
				if (window_flag[j] & (1L << i)) {
					a = TERM_L_GREEN;
					c = 'X';
				}

				/* Flag value */
				Term_putch(30 + j * 5, i + vertikal_offset + 2, a, c);
			}
		}

		/* Place Cursor */
		Term_gotoxy(30 + x * 5, y + vertikal_offset + 2);

		/* Get key */
		ch = inkey();

		/* Analyze */
		switch (ch) {
		case ESCAPE:
			go = FALSE;
			break;

		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", 2);
			break;

		/* specialty: allow chatting from within here */
		case ':': {
			bool inkey_msg_old = inkey_msg;

			cmd_message();
			inkey_msg = inkey_msg_old;
			break;
			}

		case 'T':
		case 't':
			old = Term;
			/* Clear windows */
			for (j = 1; j < ANGBAND_TERM_MAX; j++) {
				window_flag[j] &= ~(1L << y);

				/* Clear window completely (ie no flags left) and window exists? */
				if (!window_flag[j] && ang_term[j]) {
					Term_activate(ang_term[j]);
					Term_clear();
					Term_fresh();
				}
			}
			Term_activate(old);

			/* Clear flags */
			for (i = 1; i < NR_OPTIONS_SHOWN; i++)
				window_flag[x] &= ~(1L << i);

			/* Fall through */

		case 'y':
		case 'Y':
			/* Ignore screen */
			if (x == 0) break;

			/* Set flag */
			window_flag[x] |= (1L << y);

			/* Update windows */
			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER | PW_MSGNOCHAT | PW_MESSAGE | PW_CHAT | PW_CLONEMAP | PW_SUBINVEN);//PW_LAGOMETER is called automatically, no need.
			window_stuff();
			break;

		case 'n':
		case 'N':
			/* Clear flag */
			window_flag[x] &= ~(1L << y);

			/* Clear window completely (ie no flags left) and window exists? */
			if (!window_flag[x] && ang_term[x]) {
				old = Term;
				Term_activate(ang_term[x]);
				Term_clear();
				Term_fresh();
				Term_activate(old);
			}

			/* Update windows -- doesn't do anything for clearing a window though, pointless here */
			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER | PW_MSGNOCHAT | PW_MESSAGE | PW_CHAT | PW_CLONEMAP | PW_SUBINVEN);//PW_LAGOMETER is called automatically, no need.
			window_stuff();
			break;

		case '\r':
			/* Toggle flag */
			window_flag[x] ^= (1L << y);

			/* Clear window completely (ie no flags left) and window exists? */
			if (!window_flag[x] && ang_term[x]) {
				old = Term;
				Term_activate(ang_term[x]);
				Term_clear();
				Term_fresh();
				Term_activate(old);
			}

			/* Update windows */
			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER | PW_MSGNOCHAT | PW_MESSAGE | PW_CHAT | PW_CLONEMAP | PW_SUBINVEN);//PW_LAGOMETER is called automatically, no need.
			window_stuff();
			break;

		default:
			d = keymap_dirs[ch & 0x7F];

			x = (x + ddx[d] + ANGBAND_TERM_MAX - 2) % (ANGBAND_TERM_MAX - 1) + 1;
			y = (y + ddy[d] + NR_OPTIONS_SHOWN) % NR_OPTIONS_SHOWN;

			if (!d) bell();
		}
	}

	/* Notice changes */
	for (j = 1; j < ANGBAND_TERM_MAX; j++) {
		/* Dead window */
		if (!ang_term[j]) continue;

		/* Ignore non-changes */
		if (window_flag[j] == old_flag[j]) continue;

		/* Save */
		old = Term;

		/* Activate */
		Term_activate(ang_term[j]);

		/* Erase */
		Term_clear();

		/* Refresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}

	/* Update windows */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER | PW_MSGNOCHAT | PW_MESSAGE | PW_CHAT | PW_CLONEMAP | PW_SUBINVEN);//PW_LAGOMETER is called automatically, no need.
	window_stuff();

	check_for_playerlist();
}

#ifdef ENABLE_SUBWINDOW_MENU
 #if defined(WINDOWS) || defined(USE_X11)
static int font_name_cmp(const void *a, const void *b) {
   #if 0 /* simple way */
	return(strcmp((const char*)a, (const char*)b));
   #elif 0 /* sort in single-digit numbers before double-digit ones */
	char at[256], bt[256];

	at[0] = '0';
	bt[0] = '0';
	if (atoi((char*)a) < 10) strcpy(at + 1, (char *)a);
	else strcpy(at, (char *)a);
	if (atoi((char*)b) < 10) strcpy(bt + 1, (char *)b);
	else strcpy(bt, (char *)b);
	return(strcmp(at, bt));
   #else /* Sort by width, then height */
	/* Let's sort the array by font size actually */
	int fwid1, fhgt1, fwid2, fhgt2;
	char *fmatch;

	fwid1 = atoi(a);
	fmatch = my_strcasestr(a, "x");
	if (!fmatch) return(-1);  //paranoia - anyway, push 'broken' fonts towards the end of the list, just in case..
	fhgt1 = atoi(fmatch + 1);

	fwid2 = atoi(b);
	fmatch = my_strcasestr(b, "x");
	if (!fmatch) return(-1);  //paranoia - anyway, push 'broken' fonts towards the end of the list, just in case..
	fhgt2 = atoi(fmatch + 1);

	if (fwid1 != fwid2) return(fwid1 > fwid2) ? 1 : -1;
	else if (fhgt1 != fhgt2) return(fhgt1 > fhgt2) ? 1 : -1;
	else return(0);
   #endif
}
//  #endif

static void do_cmd_options_fonts(void) {
	int j, d, vertikal_offset = 6;
	int y = 0;
	char ch;
	bool go = TRUE, inkey_msg_old;

	char font_name[MAX_FONTS][256], path[1024];
	int fonts = 0;
	char tmp_name[256];
	char graphic_font_name[MAX_FONTS][256];
	int graphic_fonts = 0;

  #ifndef WINDOWS
	int x11_refresh = 50;
	FILE *fff;
  #else
	char *cp, *cpp;
  #endif

  #ifdef WINDOWS /* Windows uses the .FON files */
	DIR *dir;
	struct dirent *ent;

	/* read all locally available fonts */
	memset(font_name, 0, sizeof(char) * (MAX_FONTS * 256));
	memset(graphic_font_name, 0, sizeof(char) * (MAX_FONTS * 256));

	path_build(path, 1024, ANGBAND_DIR_XTRA, "font");
	if (!(dir = opendir(path))) {
		c_msg_format("Couldn't open fonts directory (%s).", path);
		return;
	}

	while ((ent = readdir(dir))) {
		strcpy(tmp_name, ent->d_name);
		j = -1;
		while (tmp_name[++j]) tmp_name[j] = tolower(tmp_name[j]);
		if (strstr(tmp_name, ".fon")) {
			if (tmp_name[0] == 'g') {
				if (graphic_fonts == MAX_FONTS) continue;
				strcpy(graphic_font_name[graphic_fonts], ent->d_name);
				graphic_fonts++;
				if (graphic_fonts == MAX_FONTS) c_msg_format("Warning: Number of graphic fonts exceeds max of %d. Ignoring the rest.", MAX_FONTS);
			} else {
				if (fonts == MAX_FONTS) continue;
				strcpy(font_name[fonts], ent->d_name);
				fonts++;
				if (fonts == MAX_FONTS) c_msg_format("Warning: Number of fonts exceeds max of %d. Ignoring the rest.", MAX_FONTS);
			}
		}
	}
	closedir(dir);
  #endif

  #ifdef USE_X11 /* Linux/OSX use at least the basic system fonts (/usr/share/fonts/misc) - C. Blue */
	int misc_fonts = 0;

	if (fonts < MAX_FONTS) {
		misc_fonts = get_misc_fonts(font_name[fonts], MAX_FONTS - fonts, 256, MAX_FONTS);
		if (misc_fonts > 0) fonts += misc_fonts;
		else if (fonts + 6 <= MAX_FONTS) {
			/* We boldly assume that these exist by default somehow! - They probably don't though,
			   if the above command failed, but what else can we try at this point.. #despair */
			strcpy(font_name[0], "5x8");
			strcpy(font_name[1], "6x9");
			strcpy(font_name[2], "8x13");
			strcpy(font_name[3], "9x15");
			strcpy(font_name[4], "10x20");
			strcpy(font_name[5], "12x24");
			fonts += 6;
		}
	}

	/* Additionally, read font names from a user-edited text file 'fonts.txt' to allow adding arbitrary system fonts to the cycleable list */
	path_build(path, 1024, ANGBAND_DIR_XTRA, "fonts-x11.txt");
	fff = fopen(path, "r");
	if (fff) {
		char tmp[256], *cp;

		while (fonts < MAX_FONTS) {
			if (!fgets(tmp_name, 256, fff)) break;
			tmp_name[strlen(tmp_name) - 1] = 0; //remove trailing \n

			/* Trim spaces (abuse 'path') */
			strcpy(tmp, tmp_name);
			cp = tmp;
			while (*cp == ' ') cp++;
			strcpy(tmp_name, cp);

			cp = tmp_name + (strlen(tmp_name) - 1);
			while (cp >= tmp_name && *cp == ' ') {
				*cp = 0;
				cp--;
			}

			if (!tmp_name[0]) continue; //skip empty lines
			if (tmp_name[0] == '#') continue; //skip commented out lines ('#' character at the beginning)

			strcpy(font_name[fonts], tmp_name);
			fonts++;
			if (fonts == MAX_FONTS) c_msg_format("Warning: Number of fonts exceeds max of %d. Ignoring the rest.", MAX_FONTS);
		}
		fclose(fff);
	}
  #endif

	if (!fonts) {
		c_msg_format("No .fon font files found in directory (%s).", path);
		return;
	}

//  #ifdef WINDOWS /* actually never sort fonts on X11, because they come in a sorted manner from fonts.alias and fonts.txt files already. */
	qsort(font_name, fonts, sizeof(char[256]), font_name_cmp);
   #ifdef WINDOWS /* Windows supports graphic fonts for the clone-map */
	qsort(graphic_font_name, graphic_fonts, sizeof(char[256]), font_name_cmp);
   #endif
//  #endif

   #ifdef WINDOWS /* windows client currently saves full paths (todo: just change to filename only) */
	for (j = 0; j < fonts; j++) {
		strcpy(tmp_name, font_name[j]);
		//path_build(font_name[j], 1024, path, font_name[j]);
		//strcpy(font_name[j], ".\\");
		font_name[j][0] = 0;
		strcat(font_name[j], path);
		strcat(font_name[j], "\\");
		strcat(font_name[j], tmp_name);
	}
	for (j = 0; j < graphic_fonts; j++) {
		strcpy(tmp_name, graphic_font_name[j]);
		//strcpy(graphic_font_name[j], ".\\");
		graphic_font_name[j][0] = 0;
		strcat(graphic_font_name[j], path);
		strcat(graphic_font_name[j], "\\");
		strcat(graphic_font_name[j], tmp_name);
	}
   #endif

	/* suppress hybrid macros */
	inkey_msg_old = inkey_msg;
	inkey_msg = TRUE;

	/* Clear screen */
	Term_clear();

	/* Interact */
	while (go) {
		/* Prompt XXX XXX XXX */
  #if defined(WINDOWS) && defined(USE_LOGFONT)
		if (use_logfont)
			Term_putstr(0, 0, -1, TERM_WHITE, "  <\377yup\377w/\377ydown\377w> select window, \377yv\377w visibility, \377y-\377w,\377y+\377w/\377y,\377w,\377y.\377w height/width, \377ya\377w antialiasing");
		else
  #endif
		Term_putstr(0, 0, -1, TERM_WHITE, "  <\377yup\377w/\377ydown\377w> to select window, \377yv\377w toggle visibility, \377y-\377w/\377y+\377w,\377y=\377w smaller/bigger font");
		Term_putstr(0, 1, -1, TERM_WHITE, "  \377ySPACE\377w enter new window title, \377yr\377w reset window title to default, \377yR\377w reset all");
  #if defined(WINDOWS) && defined(USE_LOGFONT)
		if (use_logfont)
			Term_putstr(0, 2, -1, TERM_WHITE, format("  \377yENTER\377w enter new logfont size, \377yL\377w %s logfont, \377yESC\377w keep changes and exit", use_logfont_ini ? "disable" : "enable"));
		else
			Term_putstr(0, 2, -1, TERM_WHITE, format("  \377yENTER\377w enter a specific font name, \377yL\377w %s logfont, \377yESC\377w keep changes and exit", use_logfont_ini ? "disable" : "enable"));
  #else
		Term_putstr(0, 2, -1, TERM_WHITE, "  \377yENTER\377w enter a specific font name, \377yESC\377w keep changes and exit");
  #endif
		Term_putstr(0, 4, -1, TERM_WHITE, format("  %d font%s and %d graphic font%s available, \377yl\377w to list in message window", fonts, fonts == 1 ? "" : "s", graphic_fonts, graphic_fonts == 1 ? "" : "s"));

		/* Display the windows */
		for (j = 0; j < ANGBAND_TERM_MAX; j++) {
			byte a = TERM_WHITE;
			cptr s = ang_term_name[j];
			char buf[256];

			/* Use color */
			if (c_cfg.use_color && (j == y)) a = TERM_L_BLUE;

			/* Window name, staggered, centered */
			Term_putstr(1, vertikal_offset + j, -1, a, (char*)s);
			/* Titles may be up to 40 characters long, in that case, shorten */
			if (strlen(s) > 19) Term_putstr(18, vertikal_offset + j, -1, a, "..");

			/* Display the font of this window */
			if (c_cfg.use_color && !term_get_visibility(j)) a = TERM_L_DARK;
  #if defined(WINDOWS) && defined(USE_LOGFONT)
			if (use_logfont && win_logfont_get_aa(j)) sprintf(buf, "%-19s (antialiased)", get_font_name(j));
			else
  #endif
			strcpy(buf, get_font_name(j));
			buf[59] = 0;
			while (strlen(buf) < 59) strcat(buf, " ");
			Term_putstr(20, vertikal_offset + j, -1, a, buf);
		}

		/* Place Cursor */
		//Term_gotoxy(20, vertikal_offset + y);
		/* hack: hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Get key */
		ch = inkey();

		/* Analyze */
		switch (ch) {
		case ESCAPE:
			go = FALSE;
			break;

		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", 2);
			break;
		case ':':
			/* specialty: allow chatting from within here */
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
			break;

		case 'v':
			if (y == 0) {
				c_msg_print("\377yThe main window must always be visible.");
				break; /* main window cannot be invisible */
			}
			term_toggle_visibility(y);
			break;

		case ' ':
			if (y == 0) {
				c_msg_print("\377yThe main window cannot be renamed.");
				break; /* main window is always 'TomeNET' */
			}
			Term_putstr(0, 20, -1, TERM_L_GREEN, "Enter new window title:");
			Term_gotoxy(0, 21);
			strcpy(tmp_name, "");
			if (!askfor_aux(tmp_name, 39, 0)) { //the array reserves [40]
				clear_from(20);
				break;
			}
			clear_from(20);
			if (!tmp_name[0]) break;

			Term_putstr(1, vertikal_offset + y, -1, TERM_DARK, "                                       ");
			strcpy(ang_term_name[y], tmp_name);

			/* Immediately change live window title */
  #ifdef WINDOWS
			set_window_title_win(y, ang_term_name[y]);
  #else /* assume POSIX */
			set_window_title_x11(y, ang_term_name[y]);
  #endif
			break;

		case 'r':
			Term_putstr(1, vertikal_offset + y, -1, TERM_DARK, "                                       ");

			/* Keep consistent with c-tables.c! */
			switch (y) {
			case 0: strcpy(ang_term_name[y], "TomeNET"); break;
			case 1: strcpy(ang_term_name[y], "Msg/Chat"); break;
			case 2: strcpy(ang_term_name[y], "Inventory"); break;
			case 3: strcpy(ang_term_name[y], "Character"); break;
			case 4: strcpy(ang_term_name[y], "Chat"); break;
			case 5: strcpy(ang_term_name[y], "Equipment"); break;
			case 6: strcpy(ang_term_name[y], "Bags"); break;
			case 7: strcpy(ang_term_name[y], "Term-7"); break;
			case 8: strcpy(ang_term_name[y], "Term-8"); break;
			case 9: strcpy(ang_term_name[y], "Term-9"); break;
			}

			/* Immediately change live window title */
  #ifdef WINDOWS
			set_window_title_win(y, ang_term_name[y]);
  #else /* assume POSIX */
			set_window_title_x11(y, ang_term_name[y]);
  #endif
			break;

		case 'R':
			/* Keep consistent with c-tables.c! */
			strcpy(ang_term_name[0], "TomeNET");
			strcpy(ang_term_name[1], "Msg/Chat");
			strcpy(ang_term_name[2], "Inventory");
			strcpy(ang_term_name[3], "Character");
			strcpy(ang_term_name[4], "Chat");
			strcpy(ang_term_name[5], "Equipment");
			strcpy(ang_term_name[6], "Bags");
			strcpy(ang_term_name[7], "Term-7");
			strcpy(ang_term_name[8], "Term-8");
			strcpy(ang_term_name[9], "Term-9");

			/* Immediately change live window title */
			for (j = 0; j < ANGBAND_TERM_MAX; j++) {
				Term_putstr(1, vertikal_offset + j, -1, TERM_DARK, "                                       ");
  #ifdef WINDOWS
				set_window_title_win(j, ang_term_name[j]);
  #else /* assume POSIX */
				set_window_title_x11(j, ang_term_name[j]);
  #endif
			}
			break;

  #if defined(WINDOWS) && defined(USE_LOGFONT)
		case '.':
			if (use_logfont) win_logfont_inc(y, FALSE);
			else bell();
			break;
  #endif
		case '=':
		case '+':
  #if defined(WINDOWS) && defined(USE_LOGFONT)
			if (use_logfont) {
				win_logfont_inc(y, TRUE);
				break;
			}
  #endif
			/* find out which of the fonts in lib/xtra/fonts we're currently using */
			if ((window_flag[y] & PW_CLONEMAP) && graphic_fonts > 0) {
				//Include the graphic fonts, because we are cycling the clone-map
				for (j = 0; j < graphic_fonts - 1; j++) {
					if (!strcasecmp(graphic_font_name[j], get_font_name(y))) {
						/* advance to next font file in lib/xtra/font */
						set_font_name(y, graphic_font_name[j + 1]);
  #ifndef WINDOWS
						sync_sleep(x11_refresh);
  #endif
						break;
					}
				}
			} else {
				for (j = 0; j < fonts - 1; j++) {
					if (!strcasecmp(font_name[j], get_font_name(y))) {
						/* advance to next font file in lib/xtra/font */
						set_font_name(y, font_name[j + 1]);
  #ifndef WINDOWS
						sync_sleep(x11_refresh);
  #endif
						break;
					}
				}
			}
			break;

  #if defined(WINDOWS) && defined(USE_LOGFONT)
		case ',':
			if (use_logfont) win_logfont_dec(y, FALSE);
			else bell();
			break;
  #endif
		case '-':
  #if defined(WINDOWS) && defined(USE_LOGFONT)
			if (use_logfont) {
				win_logfont_dec(y, TRUE);
				break;
			}
  #endif
			/* find out which of the fonts in lib/xtra/fonts we're currently using */
			if ((window_flag[y] & PW_CLONEMAP) && graphic_fonts > 0) {
				//Include the graphic fonts, because we are cycling the clone-map
				for (j = 1; j < graphic_fonts; j++) {
					if (!strcasecmp(graphic_font_name[j], get_font_name(y))) {
						/* retreat to previous font file in lib/xtra/font */
						set_font_name(y, graphic_font_name[j - 1]);
  #ifndef WINDOWS
						sync_sleep(x11_refresh);
  #endif
						break;
					}
				}
			} else {
				for (j = 1; j < fonts; j++) {
					if (!strcasecmp(font_name[j], get_font_name(y))) {
						/* retreat to previous font file in lib/xtra/font */
						set_font_name(y, font_name[j - 1]);
  #ifndef WINDOWS
						sync_sleep(x11_refresh);
  #endif
						break;
					}
				}
			}
			break;

		case '\r':
  #if defined(WINDOWS) && defined(USE_LOGFONT)
			if (use_logfont) {
				Term_putstr(0, 20, -1, TERM_L_GREEN, "Enter new size in format '<width>x<height>' (eg \"9x15\"):");
				Term_gotoxy(0, 21);
				strcpy(tmp_name, "");
				if (!askfor_aux(tmp_name, 159, 0)) {
					clear_from(20);
					break;
				}
				clear_from(20);
				if (!tmp_name[0]) break;
				win_logfont_set(y, tmp_name);
				break;
			}
  #endif
			Term_putstr(0, 20, -1, TERM_L_GREEN, "Enter a font name:");
			Term_gotoxy(0, 21);
			strcpy(tmp_name, "");
			if (!askfor_aux(tmp_name, 159, 0)) {
				clear_from(20);
				break;
			}
			clear_from(20);
			if (!tmp_name[0]) break;
			set_font_name(y, tmp_name);
  #ifndef WINDOWS
			sync_sleep(x11_refresh);
  #endif
			break;

		case 'l':
			if (!fonts && !graphic_fonts) {
				c_message_add("No fonts and no graphical fonts found.");
				break;
			}
			if (fonts) {
				char tmp_name2[256];
				int c = 0;

				c_message_add(format("-- Fonts (%d): --", fonts));
				tmp_name2[0] = 0;
				for (j = 0; j < fonts; j++) {
  #ifdef WINDOWS
					/* Windows font names contain the whole .\lib\xtra\fonts\xxx, crop that */
					cpp = font_name[j];
					while ((cp = strchr(cpp, '\\'))) cpp = cp + 1;
					sprintf(tmp_name, "%-18s", cpp);
  #else
					sprintf(tmp_name, "%-18s", font_name[j]);
  #endif

					/* print up to 4 font names per line */
					c++;
					if (c % 4 == 0 || strlen(tmp_name2) + strlen(tmp_name) > 79 - 2) {
						c_message_add(format("  %s", tmp_name2));
						tmp_name2[0] = 0;
						c = 0;
					}

					strcat(tmp_name2, tmp_name);
				}
				c_message_add(format("  %s", tmp_name2));
			}
			if (graphic_fonts) {
				c_message_add(format("-- Graphic fonts (%d): --", graphic_fonts));
				for (j = 0; j < graphic_fonts; j++)
					c_message_add(format("  %s", graphic_font_name[j]));
			}
			c_message_add(""); //linefeed
			break;

  #if defined(WINDOWS) && defined(USE_LOGFONT)
		case 'L':
			/* We cannot live-change 'use_logfont' itself, as that'd render the client effectively frozen, just toggle the ini setting for next startup: */
			use_logfont_ini = !use_logfont_ini;
			if (use_logfont_ini) c_msg_print("\377yUsing Windows-internal font instead of FON files. Requires restart, use CTRL+Q.");
			else c_msg_print("\377yUsing FON files instead of Windows-internal font. Requires restart, use CTRL+Q.");
			break;
		case 'a':
			if (!use_logfont) {
				bell();
				continue;
			}
			win_logfont_set_aa(y, !win_logfont_get_aa(y));
			/* if (win_logfont_get_aa(y)) c_msg_format("\377yLogfont-antialiasing is now on for window #%d.", y);
			else c_msg_format("\377yLogfont-antialiasing is now off for window #%d", y); */
			break;
  #endif

		default:
			d = keymap_dirs[ch & 0x7F];
			y = (y + ddy[d] + NR_OPTIONS_SHOWN) % NR_OPTIONS_SHOWN;
			if (!d) bell();
		}
	}

	/* restore responsiveness to hybrid macros */
	inkey_msg = inkey_msg_old;

	check_for_playerlist();
}
  #ifdef USE_GRAPHICS
/* These are .bmp files in xtra/graphics, on all systems. - C. Blue
   The global vars are use_graphics (TRUE/FALSE) and graphic_tiles (string of filename, without path, without '.bmp' extension, gets inserted to "graphics-%s.prf").
   Filename convention added: "graphics-<tilesetname>[#<0..9>_<subname>].bmp" */
//#include <dirent.h> /* for do_cmd_options_tilesets() */
static void do_cmd_options_tilesets(void) {
	int j, l, l2, cur_set = -1, found_subset, t;
	char ch, old_tileset[MAX_CHARS];
	bool go = TRUE, inkey_msg_old, subset_enabled[MAX_FONTS][MAX_SUBFONTS];

	char filename_tmp[MAX_FONTS][256], tileset_name[MAX_FONTS][256], path[1024];
	char tileset_subname[MAX_FONTS][MAX_SUBFONTS][256] = { 0 };
	int filenames_tmp = 0, tilesets = 0;
	char tmp_name[256], tmp_name2[256], *csub, *csub_end;

   #ifdef WINDOWS
	char *cp, *cpp;
   #endif

	DIR *dir;
	struct dirent *ent;


	/* Paranoia: 0 tileset allowed? */
	if (!MAX_FONTS) return;

	/* read all locally available tilesets */
	memset(tileset_name, 0, sizeof(char) * (MAX_FONTS * 256));

	path_build(path, 1024, ANGBAND_DIR_XTRA, "graphics");
	if (!(dir = opendir(path))) {
		c_msg_format("Couldn't open tilesets directory (%s).", path);
		return;
	}

	/* Read all eligible filenames (*.bmp), ... */
	while ((ent = readdir(dir))) {
		strcpy(tmp_name, ent->d_name);

		/* file must end on '.bmp' */
		j = strlen(tmp_name) - 4;
		if (j < 1) continue;
		if ((tolower(tmp_name[j++]) != '.' || tolower(tmp_name[j++] != 'b') || tolower(tmp_name[j++] != 'm') || tolower(tmp_name[j] != 'p'))) continue;
		/* cut off extension */
		tmp_name[j - 3] = 0;

		strcpy(filename_tmp[filenames_tmp++], tmp_name);
	}
	closedir(dir);
	if (!filenames_tmp) {
		c_msg_format("No .bmp files found in directory (%s).", path);
		return;
	}
	/* ...sort them... */
	qsort(filename_tmp, filenames_tmp, sizeof(char[256]), font_name_cmp);
	/* ...and process them for tileset/subset type */
	for (t = 0; t < filenames_tmp; t++) {
		strcpy(tmp_name, filename_tmp[t]);

		/* Check for subset */
		if ((csub = strchr(tmp_name, '#'))) {
			*csub = 0;
			l = atoi(csub + 1);
			if (l < 0 || l >= MAX_SUBFONTS) {
				c_msg_format("Warning: Ignoring out-of-range tile-subset #%d for %s.", l, tmp_name);
				continue;
			}
			/* "#n" must be followed by "_" and at least one other character */
			if (!(csub_end = strchr(csub + 1, '_')) || *(csub_end + 1) == 0) {
				c_msg_format("Warning: Ignoring wrong tile-subset syntax for %s#%d.", tmp_name, l);
				continue;
			}
			*csub_end = 0;
			for (j = 0; j < tilesets; j++) {
				/* Already found this base set name? */
				if (!strcmp(tileset_name[j], tmp_name)) {
					strcpy(tileset_subname[j][l], csub_end + 1);
					break;
				}
			}
			/* Not a new base set? */
			if (j != tilesets) continue;
		}
		/* Check that a base tileset was not already added by one of its sub-tileset file names, in that case skip */
		for (j = 0; j < tilesets; j++)
			if (!strcmp(tileset_name[j], tmp_name)) break;
		if (j != tilesets) continue;
		/* New tileset */
		strcpy(tileset_name[tilesets], tmp_name);
		/* ..also comes with a subset '#' entry? */
		if (csub) strcpy(tileset_subname[tilesets][l], csub_end + 1);

		tilesets++;
		if (tilesets == MAX_FONTS) {
			c_msg_format("Warning: Number of tilesets reached max of %d. Ignoring the rest.", MAX_FONTS);
			break;
		}
	}
	if (!tilesets) {
		c_msg_format("No eligible .bmp tileset files found in directory (%s).", path);
		return;
	}
	/* Some post-processing ^^ */
	for (j = 0; j < tilesets; j++) {
    #ifdef WINDOWS /* windows client currently saves full paths (todo: just change to filename only) */
		strcpy(tmp_name, tileset_name[j]);
		//path_build(tileset_name[j], 1024, path, tileset_name[j]);
		//strcpy(tileset_name[j], ".\\");
		tileset_name[j][0] = 0;
		strcat(tileset_name[j], path);
		strcat(tileset_name[j], "\\");
		strcat(tileset_name[j], tmp_name);
    #endif
		/* Find index of currently used tileset */
		if (strcasecmp(tileset_name[j], graphic_tiles)) continue;
		cur_set = j;
		/* Imprint current tileset's subset config to array of all tilesets' subsets */
		for (l = 0; l < MAX_SUBFONTS; l++) subset_enabled[j][l] = graphic_subtiles[l];
		break;
	}

	/* suppress hybrid macros */
	inkey_msg_old = inkey_msg;
	inkey_msg = TRUE;

	/* Clear screen */
	Term_clear();

	strcpy(old_tileset, graphic_tiles);

	/* Interact */
	while (go) {
		clear_from(0);
		l = 0;

		/* Prompt XXX XXX XXX */
		Term_putstr(0, l++, -1, TERM_WHITE, " \377y-\377w/\377y+\377w,\377y=\377w switch tileset (requires restart), \377yENTER\377w enter a specific tileset name,");
		Term_putstr(0, l++, -1, TERM_WHITE, " \377y0\377w...\377y9\377w to enable/disable subset of currently selected tileset, if available,");
   #ifdef GRAPHICS_BG_MASK
		Term_putstr(0, l++, -1, TERM_WHITE, " \377yv\377w cycle graphics mode - requires client restart! \377yESC\377w keep changes and exit.");
   #else
		Term_putstr(0, l++, -1, TERM_WHITE, " \377yv\377w toggle graphics on/off - requires client restart! \377yESC\377w keep changes and exit.");
   #endif
		l++;
		Term_putstr(0, l++, -1, TERM_WHITE, " \377sTilesets AUTO-ZOOM to font size which you can change in Window Fonts menu (\377yf\377s).");
		Term_putstr(0, l++, -1, TERM_WHITE, " \377sSo for graphics to look good, you should selected a font size that matches it");
		Term_putstr(0, l++, -1, TERM_WHITE, " \377sas best as possible and is not significantly smaller than the selected tileset.");
		Term_putstr(0, l++, -1, TERM_WHITE, " \377sFor example for a 16x22 sized tileset a 16x22 or 16x24 font works very well.");
		l++;

		Term_putstr(1, l++, -1, TERM_WHITE, format("%d graphical tileset%s available, \377yl\377w to list in message window", tilesets, tilesets == 1 ? "" : "s"));

		//GRAPHICS_BG_MASK @ UG_2MASK:
   #ifdef GRAPHICS_BG_MASK
		Term_putstr(1, l++, -1, TERM_WHITE, format("Graphical tileset mode is %s\377w ('\377yv\377w' %s).",
		    use_graphics_new == UG_2MASK ? "\377Genabled (dual-mask mode)" : (use_graphics_new ? "\377Genabled (standard)\377-" : "\377sdisabled\377-"),
		    use_graphics_new == UG_2MASK ? "to disable" : (use_graphics_new ? "to enable 2-mask mode" : "to enable standard graphics mode")));
   #else
		Term_putstr(1, l++, -1, TERM_WHITE, format("Graphical tilesets are currently %s ('v' to toggle).", use_graphics_new == UG_2MASK ? "\377Genabled (dual)" : (use_graphics_new ? "\377Genabled\377-" : "\377sdisabled\377-")));
   #endif
		l++;

		/* Tilesets are atm a global setting, not depending on terminal window */
		l2 = l;
		Term_putstr(1, l++, -1, TERM_WHITE, format("Currently selected tileset: "));
		Term_putstr(1, l++, -1, TERM_WHITE, format("  '\377B%s\377-'", graphic_tiles));
		Term_putstr(1, l++, -1, TERM_WHITE, format("Tileset filename:           "));
		Term_putstr(1, l++, -1, TERM_WHITE, format("  '\377B%s.bmp\377-'", graphic_tiles));
		Term_putstr(1, l++, -1, TERM_WHITE, format("Optional mapping filename:  "));
		Term_putstr(1, l++, -1, TERM_WHITE, format("  '\377Bgraphics-%s.bmp\377-'", graphic_tiles));
		l += 2;

		found_subset = 0;
		l2++;
		for (j = 0; j < MAX_SUBFONTS; j++) {
			if (!tileset_subname[cur_set][j][0]) continue;
			Term_putstr(40, l2++, -1, subset_enabled[cur_set][j] ? TERM_L_WHITE : TERM_L_DARK, format("  #%d '\377B%s\377-'", j, tileset_subname[cur_set][j]));
			found_subset++;
		}
		if (found_subset) Term_putstr(40, l2 - 1 - found_subset, -1, TERM_WHITE, format("Available subsets of the selected set: "));
		else Term_putstr(40, l2 - 1, -1, TERM_WHITE, format("(No subsets availabley)"));

   #ifdef GFXERR_FALLBACK
		if (use_graphics_err) {
			Term_putstr(2, l + 0, -1, TERM_WHITE, "\377RClient was unable to startup with graphics and fell back to text mode.");
			Term_putstr(2, l + 1, -1, TERM_WHITE, "\377RIf you re-enable graphics, ensure tileset is valid and its file name correct!");
			if (!use_graphics_errstr[0]) Term_putstr(2, l + 2, -1, TERM_WHITE, "\377RNo failure reason specified.");
			else {
				char line1[MAX_CHARS], line2[MAX_CHARS] = { 0 };

				Term_putstr(2, l + 2, -1, TERM_WHITE, "\377RFailure reason on startup, also shown in terminal on startup, was:");
				if (strlen(use_graphics_errstr) < MAX_CHARS) strcpy(line1, use_graphics_errstr);
				else {
					strncpy(line1, use_graphics_errstr, MAX_CHARS - 1);
					line1[MAX_CHARS - 1] = 0;
					strcpy(line2, use_graphics_errstr + MAX_CHARS - 1);
				}
				Term_putstr(0, l + 3, -1, TERM_RED, line1);
				Term_putstr(0, l + 4, -1, TERM_RED, line2);
			}
		}
   #endif

		/* Place Cursor */
		//Term_gotoxy(20, vertikal_offset + y);
		/* hack: hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Get key */
		ch = inkey();
		/* Analyze */
		if (ch >= '0' && ch <= '9') {
			int sub = ch - '0';

			l2 = l + 1;
			for (j = 0; j < MAX_SUBFONTS; j++) {
				if (!tileset_subname[cur_set][j][0]) continue;
				l2++;
				if (j != sub) continue;

				graphic_subtiles[sub] = !graphic_subtiles[sub];
				subset_enabled[cur_set][sub] = graphic_subtiles[sub];
				break;
			}
		} else switch (ch) {
		case ESCAPE:
			go = FALSE;

			if (strcmp(old_tileset, graphic_tiles))
				c_msg_print("\377yGraphical tileset was changed. Requires client restart (use CTRL+Q).");

			break;

		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", 2);
			break;
		case ':':
			/* specialty: allow chatting from within here */
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
			break;

		case 'f':
			/* specialty: allow calling the fonts menu directly from here for QoL font-zooming */
			do_cmd_options_fonts();
			break;

		case 'v':
			/* Hack: Never switch graphics settings, especially UG_2MASK, live,
			   as it will cause instant packet corruption due to missing server-client synchronisation.
			   So we just switch the savegame-affecting 'use_graphics_new' instead of actual 'use_graphics'. */
   #ifdef GRAPHICS_BG_MASK
			use_graphics_new = (use_graphics_new + 1) % 3;
			if (use_graphics_new == UG_2MASK) c_msg_print("\377yGraphical tileset usage \377Genabled (dual)\377-. Requires client restart (use CTRL+Q).");
			else
   #else
			use_graphics_new = !use_graphics_new;
   #endif
			if (use_graphics_new) {
				c_msg_print("\377yGraphical tileset usage \377Genabled\377-. Requires client restart (use CTRL+Q).");
   #if 0 /* not really needed here, if gfx_autooff_fmsw is enabled it will be applied on next login anyway, and the warning about fmsw breaking gfx is good to read so the player knows about it. */
				if (c_cfg.gfx_autooff_fmsw) {
					c_msg_print("Option 'font_map_solid_walls' was auto-disabled as graphics are enabled.");
					c_cfg.font_map_solid_walls = FALSE;
					(*option_info[CO_FONT_MAP_SOLID_WALLS].o_var) = FALSE;
					Client_setup.options[CO_FONT_MAP_SOLID_WALLS] = FALSE;
					options_immediate(FALSE);
					Send_options();
				}
   #endif
			} else c_msg_print("\377yGraphical tileset usage \377sdisabled\377-. Requires client restart (use CTRL+Q).");
			break;

		case '=':
		case '+':
			/* find out which of the tilesets in lib/xtra/graphics we're currently using */
			for (j = 0; j < tilesets - 1; j++) {
				if (strcasecmp(tileset_name[j], graphic_tiles)) continue;
				/* advance to next tileset file in lib/xtra/graphics */
				strcpy(graphic_tiles, tileset_name[j + 1]);
				cur_set = j + 1;
				break;
			}
			break;

		case '-':
			/* find out which of the tilesets in lib/xtra/graphics we're currently using */
			for (j = 1; j < tilesets; j++) {
				if (strcasecmp(tileset_name[j], graphic_tiles)) continue;
				/* retreat to previous tileset file in lib/xtra/graphics */
				strcpy(graphic_tiles, tileset_name[j - 1]);
				cur_set = j - 1;
				break;
			}
			break;

		case '\r':
			Term_putstr(0, 20, -1, TERM_L_GREEN, "Enter a tileset name (without '.bmp' extension):");
			Term_gotoxy(0, 21);
			strcpy(tmp_name, "");
			if (!askfor_aux(tmp_name, 159, 0)) {
				clear_from(20);
				break;
			}
			clear_from(20);
			if (!tmp_name[0]) break;

			for (j = 0; j < tilesets; j++) {
   #ifdef WINDOWS
				/* Windows tileset names contain the whole .\lib\xtra\graphics\xxx, crop that */
				cpp = tileset_name[j];
				while ((cp = strchr(cpp, '\\'))) cpp = cp + 1;
				sprintf(tmp_name2, "%s", cpp);
   #else
				sprintf(tmp_name2, "%s", tileset_name[j]);
   #endif
				if (strcasecmp(tmp_name2, tmp_name)) continue;
				cur_set = j;
				break;
			}

			if (j == tilesets) {
				c_msg_format("\377yError: No tileset '%s.bmp' in the graphics folder.", tmp_name);
				break;
			}
			strcpy(graphic_tiles, tmp_name);
			break;

		case 'l':
			if (!tilesets) {
				c_message_add("No tilesets found.");
				break;
			}
			if (tilesets) {
				char tmp_name2[256];
				int c = 0;

				c_message_add(format("-- Graphical tilesets (%d): --", tilesets));
				tmp_name2[0] = 0;
				for (j = 0; j < tilesets; j++) {
   #ifdef WINDOWS
					/* Windows tileset names contain the whole .\lib\xtra\graphics\xxx, crop that */
					cpp = tileset_name[j];
					while ((cp = strchr(cpp, '\\'))) cpp = cp + 1;
					sprintf(tmp_name, "%-18s", cpp);
   #else
					sprintf(tmp_name, "%-18s", tileset_name[j]);
   #endif

					/* print up to 4 tileset names per line */
					c++;
					if (c % 4 == 0 || strlen(tmp_name2) + strlen(tmp_name) > 79 - 2) {
						c_message_add(format("  %s", tmp_name2));
						tmp_name2[0] = 0;
						c = 0;
					}

					strcat(tmp_name2, tmp_name);
				}
				c_message_add(format("  %s", tmp_name2));
			}
			c_message_add(""); //linefeed
			break;

		default:
			bell();
		}
	}

	/* restore responsiveness to hybrid macros */
	inkey_msg = inkey_msg_old;

	check_for_playerlist();
}
  #endif
 #endif /* WINDOWS || USE_X11 */
#endif /* ENABLE_SUBWINDOW_MENU */

#ifdef USE_SOUND_2010
static void do_cmd_options_sfx(void) {
 #if SOUND_SDL
	do_cmd_options_sfx_sdl();
 #endif
}
static void do_cmd_options_mus(void) {
 #if SOUND_SDL
	do_cmd_options_mus_sdl();
 #endif
}
#endif

errr options_dump(cptr fname) {
	int i, j;
	FILE *fff;
	char buf[1024];

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* Check if the file already exists */
	fff = my_fopen(buf, "r");

	if (fff) {
		char buf2[1024];

		fclose(fff);

		/* Attempt to rename */
		strcpy(buf2, buf);
		strncat(buf2, ".bak", 1023);
		rename(buf, buf2);
	}

	/* Write to the file */
	fff = my_fopen(buf, "w");

	/* Failure */
	if (!fff) return(-1);


	/* Skip space */
	fprintf(fff, "\n\n");

	/* Start dumping */
	fprintf(fff, "# Automatic option dump\n\n");

	/* Dump them */
	for (i = 0; i < OPT_MAX; i++) {
		/* Require a real option */
		if (!option_info[i].o_desc) continue;

		/* Comment */
		fprintf(fff, "# Option '%s'\n", option_info[i].o_desc);

		/* Dump the option */
		if (*option_info[i].o_var)
			fprintf(fff, "Y:%s\n", option_info[i].o_text);
		else
			fprintf(fff, "X:%s\n", option_info[i].o_text);

		/* End the option */
		fprintf(fff, "\n");
	}

	fprintf(fff, "\n");

	/* Dump window flags */
	for (i = 1; i < ANGBAND_TERM_MAX; i++) {
		/* Require a real window */
		if (!ang_term[i]) continue;

		/* Check each flag */
		for (j = 0; j < NR_OPTIONS_SHOWN; j++) {
			/* Require a real flag */
			if (!window_flag_desc[j]) continue;

			/* Dump the flag */
			if (window_flag[i] & (1L << j)) {
				/* Comment */
				fprintf(fff, "# Window '%s', Flag '%s'\n",
						ang_term_name[i], window_flag_desc[j]);

				fprintf(fff, "W:%d:%d\n", i, j);

				/* Skip a line */
				fprintf(fff, "\n");
			}
		}
	}

	/* Finish dumping */
	fprintf(fff, "\n\n\n\n");

	/* Close */
	my_fclose(fff);

	/* Success */
	return(0);
}

/* For installing audio packs via = I menu: */

#ifdef WINDOWS
 #include <winreg.h>	/* remote control of installed 7-zip via registry approach */
 #include <process.h>	/* use spawn() instead of normal system() (WINE bug/Win inconsistency even maybe) */
 #define MAX_KEY_LENGTH 255
 #define MAX_VALUE_NAME 16383
#endif

/* Check if an audio pack is password protected. pack_class is the string part identifying the pack-specific cfg file name, ie 'sound' or 'music'.
   Sometimes 'l' command works without password, sometimes it already requires a password, depending on how the archive was created. */
static bool test_for_password(cptr path_7z_quoted, cptr pack_name, cptr pack_class) {
	char out_val[1024];
	bool passworded = FALSE;
	FILE *fff;

#ifdef WINDOWS
	bool l7z = FALSE;


	/* First, test if 'l' command already requires a password: */
	fff = fopen("__tomenethelper.bat", "w");
	if (!fff) {
		c_message_add("\377oError: Couldn't write temporary file (S1).");
		return(FALSE);
	}
	fprintf(fff, format("@%s -pBadPassword l \"%s\" > __tomenet.tmp\n", path_7z_quoted, pack_name));
	fclose(fff);
	_spawnl(_P_WAIT, "__tomenethelper.bat", "__tomenethelper.bat", NULL);
	remove("__tomenethelper.bat");
	fff = fopen("__tomenet.tmp", "r");
	if (!fff) {
		c_message_add("\377oError: Couldn't scan file (S2).");
		return(FALSE);
	}
	out_val[0] = 0;
	while (!feof(fff)) {
		if (!fgets(out_val, 1024, fff)) break;
		if (out_val[0]) out_val[strlen(out_val) - 1] = 0; //strip \n
		if (suffix(out_val, ": 1")) {
			fclose(fff);
			return(TRUE);
		}
	}
	fclose(fff);

	/* Find the first file in the archive, to save time: */
	fff = fopen("__tomenethelper.bat", "w");
	if (!fff) {
		c_message_add("\377oError: Couldn't write temporary file (S3).");
		return(FALSE);
	}
	fprintf(fff, format("@%s l \"%s\" > __tomenet.tmp\n", path_7z_quoted, pack_name));
	fclose(fff);
	_spawnl(_P_WAIT, "__tomenethelper.bat", "__tomenethelper.bat", NULL);
	remove("__tomenethelper.bat");
	fff = fopen("__tomenet.tmp", "r");
	if (!fff) {
		c_message_add("\377oError: Couldn't scan file (S4).");
		return(FALSE);
	}
	out_val[0] = 0;
	while (!feof(fff)) {
		if (!fgets(out_val, 1024, fff)) break;
		if (out_val[0]) out_val[strlen(out_val) - 1] = 0; //strip \n
		if (!l7z) {
			if (prefix(out_val, "-------------------")) l7z = TRUE;
			out_val[0] = 0;
			continue;
		}
		if (out_val[20] == 'D') {
			out_val[0] = 0;
			continue;
		}
		/* Found the first real file (ie not a folder) */
		break;
	}
	fclose(fff);
	if (strlen(out_val) < 3) {
		c_msg_print("\377RERROR: Archive contains no files (S5).");
		return(FALSE); /* Paranoia: No files in this archive! */
	}

	/* Test this first file for password protection: */
	fff = fopen("__tomenethelper.bat", "w");
	if (!fff) {
		c_message_add("\377oError: Couldn't write temporary file (S6).");
		return(FALSE);
	}
	fprintf(fff, format("@%s t -pBadPassword %s \"%s\" > __tomenet.tmp\n", path_7z_quoted, pack_name, out_val + 53));
	fclose(fff);
	_spawnl(_P_WAIT, "__tomenethelper.bat", "__tomenethelper.bat", NULL);
	remove("__tomenethelper.bat");
	fff = fopen("__tomenet.tmp", "r");
	if (!fff) {
		c_message_add("\377oError: Couldn't scan file (S7).");
		return(FALSE);
	}
	out_val[0] = 0;
	while (!feof(fff)) {
		if (!fgets(out_val, 1024, fff)) break;
		if (out_val[0]) out_val[strlen(out_val) - 1] = 0; //strip \n
		if (suffix(out_val, ": 1")) {
			passworded = TRUE;
			break;
		}
	}
	fclose(fff);
#else /* assume POSIX */
	int r;

	/* First, test if 'l' command already requires a password: */
	r = system(format("7z -pBadPassword l \"%s\" | grep -o -m 1 \": 1$\" > __tomenet.tmp", pack_name));
	fff = fopen("__tomenet.tmp", "r");
	if (!fff) {
		c_message_add("\377oError: Couldn't scan file (S1).");
		return(FALSE);
	}
	out_val[0] = 0;
	if (!feof(fff)) (void)fgets(out_val, 1024, fff);
	fclose(fff);
	if (strlen(out_val) >= 3) return(TRUE);

	/* Find the first file in the archive, to save time: */
	r = system(format("7z l \"%s\" | grep -m 1 \"\\.\\.\\.\\.A\" | grep -o \" %s.*\" | grep -o \"%s.*\" > __tomenet.tmp", pack_name, pack_class, pack_class));
	fff = fopen("__tomenet.tmp", "r");
	if (!fff) {
		c_message_add("\377oError: Couldn't scan file (S2).");
		return(FALSE);
	}
	out_val[0] = 0;
	if (!feof(fff)) (void)fgets(out_val, 1024, fff);
	if (out_val[0]) out_val[strlen(out_val) - 1] = 0; //strip \n
	fclose(fff);
	if (strlen(out_val) < 3) {
		c_msg_print("\377RERROR: Archive contains no files (S3).");
		return(FALSE); /* Paranoia: No files in this archive! */
	}

	/* Test that first file for password protection: */
	r = system(format("7z t -pBadPassword \"%s\" \"%s\" | grep -o -m 1 \": 1$\" > __tomenet.tmp", pack_name, out_val));
	fff = fopen("__tomenet.tmp", "r");
	if (!fff) {
		c_message_add("\377oError: Couldn't scan file (S4).");
		return(FALSE);
	}
	out_val[0] = 0;
	if (!feof(fff)) (void)fgets(out_val, 1024, fff);
	fclose(fff);
	passworded = strlen(out_val) >= 3;

	(void)r;
#endif

	return(passworded);
}
static bool verify_password(cptr path_7z_quoted, cptr pack_name, cptr pack_class, cptr password) {
	char out_val[1024];
	bool passworded = FALSE;
	FILE *fff;

#ifdef WINDOWS
	bool l7z = FALSE;

	/* Find the first file in the archive, to save time: */
	fff = fopen("__tomenethelper.bat", "w");
	if (!fff) {
		c_message_add("\377oError: Couldn't write temporary file. (V1)");
		return(FALSE);
	}
	fprintf(fff, format("@%s -p\"%s\" l \"%s\" > __tomenet.tmp\n", path_7z_quoted, password, pack_name));
	fclose(fff);
	_spawnl(_P_WAIT, "__tomenethelper.bat", "__tomenethelper.bat", NULL);
	remove("__tomenethelper.bat");
	fff = fopen("__tomenet.tmp", "r");
	if (!fff) {
		c_message_add("\377oError: Couldn't scan file (V2).");
		return(FALSE);
	}
	out_val[0] = 0;
	while (!feof(fff)) {
		if (!fgets(out_val, 1024, fff)) break;
		if (out_val[0]) out_val[strlen(out_val) - 1] = 0; //strip \n
		if (!l7z) {
			if (prefix(out_val, "-------------------")) l7z = TRUE;
			out_val[0] = 0;
			continue;
		}
		if (out_val[20] == 'D') {
			out_val[0] = 0;
			continue;
		}
		/* Found the first real file (ie not a folder) */
		break;
	}
	fclose(fff);
	if (strlen(out_val) < 3) {
		c_msg_print("\377RERROR: Archive contains no files (V3).");
		return(FALSE); /* Paranoia: No files in this archive! */
	}

	/* Test this first file for password protection: */
	fff = fopen("__tomenethelper.bat", "w");
	if (!fff) {
		c_message_add("\377oError: Couldn't write temporary file. (V4)");
		return(FALSE);
	}
	fprintf(fff, format("@%s t -p\"%s\" %s \"%s\" > __tomenet.tmp\n", path_7z_quoted, password, pack_name, out_val + 53));
	fclose(fff);
	_spawnl(_P_WAIT, "__tomenethelper.bat", "__tomenethelper.bat", NULL);
	remove("__tomenethelper.bat");
	fff = fopen("__tomenet.tmp", "r");
	if (!fff) {
		c_message_add("\377oError: Couldn't scan file (V5).");
		return(FALSE);
	}
	out_val[0] = 0;
	while (!feof(fff)) {
		if (!fgets(out_val, 1024, fff)) break;
		if (out_val[0]) out_val[strlen(out_val) - 1] = 0; //strip \n
		if (suffix(out_val, ": 1")) {
			passworded = TRUE;
			break;
		}
	}
	fclose(fff);
#else /* assume POSIX */
	int r;

	/* Find the first file in the archive, to save time: */
	r = system(format("7z -p\"%s\" l \"%s\" | grep -m 1 \"\\.\\.\\.\\.A\" | grep -o \" %s.*\" | grep -o \"%s.*\" > __tomenet.tmp", password, pack_name, pack_class, pack_class));
	fff = fopen("__tomenet.tmp", "r");
	if (!fff) {
		c_message_add("\377oError: Couldn't scan file (V1).");
		return(FALSE);
	}
	out_val[0] = 0;
	if (!feof(fff)) (void)fgets(out_val, 1024, fff);
	if (out_val[0]) out_val[strlen(out_val) - 1] = 0; //strip \n
	fclose(fff);
	if (strlen(out_val) < 3) {
		c_msg_print("\377RERROR: Archive contains no files (V2).");
		return(FALSE); /* Paranoia: No files in this archive! */
	}

	/* Test that first file for password protection: */
	r = system(format("7z t -p\"%s\" \"%s\" \"%s\" | grep -o -m 1 \": 1$\" > __tomenet.tmp", password, pack_name, out_val));
	fff = fopen("__tomenet.tmp", "r");
	if (!fff) {
		c_message_add("\377oError: Couldn't scan file (V3).");
		return(FALSE);
	}
	out_val[0] = 0;
	if (!feof(fff)) (void)fgets(out_val, 1024, fff);
	fclose(fff);
	passworded = strlen(out_val) >= 3;

	(void)r;
#endif

	return(!passworded);
}

/* Attempt to find sound+music pack 7z files in the client's root folder
   and to install them properly. - C. Blue */
static void do_cmd_options_install_audio_packs(void) {
	FILE *fff;
	char ins_path[1024] = { 0 }, out_val[1024 + 28], password[MAX_CHARS];
	char c, ch, pack_name[1024], pack_top_folder[1024];
	int r;
	bool picked, tarfile, l7z;

#ifdef WINDOWS /* use windows registry to locate 7-zip */
	HKEY hTestKey;
	char path_7z[1024], path_7z_quoted[1024];
	LPBYTE path_7z_p = (LPBYTE)path_7z;
	int path_7z_size = 1023;
	LPDWORD path_7z_size_p = (LPDWORD)&path_7z_size;
	unsigned long path_7z_type = REG_SZ;
#endif

	bool maybe_sound_pack, maybe_music_pack, sound_pack = FALSE, music_pack = FALSE;
	//bool sound_already = (audio_sfx > 3), music_already = (audio_music > 0);
	bool passworded, password_ok;


#if 0 /* hmm why not? */
	if (quiet_mode) {
		Term_putstr(0, 1, -1, TERM_RED, "Client is running in 'quiet mode'. Cannot install audio packs!");
		Term_putstr(0, 2, -1, TERM_RED, "(Restart TomeNET client with 'Sound=1' and without '-q'.)");
		Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...");
		inkey();
		return;
	}
#endif

	/* test for availability of unarchiver */
#ifdef WINDOWS
	/* check registry for 7zip (note that for example WinRAR could cause problems with 7z files) */
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\7-Zip\\"), 0, KEY_READ, &hTestKey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(hTestKey, "Path", NULL, &path_7z_type, path_7z_p, path_7z_size_p) == ERROR_SUCCESS) {
			path_7z[path_7z_size] = '\0';
		} else {
			// odd case
			RegCloseKey(hTestKey);
			Term_putstr(0, 1, -1, TERM_RED, "7-zip not properly installed. Please reinstall it. (www.7-zip.org)");
			Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...");
			inkey();
			return;
		}
	} else {
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\7-Zip\\"), 0, KEY_READ | 0x0200, &hTestKey) == ERROR_SUCCESS) {//KEY_WOW64_32KEY (0x0200)
			if (RegQueryValueEx(hTestKey, "Path", NULL, &path_7z_type, path_7z_p, path_7z_size_p) == ERROR_SUCCESS) {
				path_7z[path_7z_size] = '\0';
			} else {
				// odd case
				RegCloseKey(hTestKey);
				Term_putstr(0, 1, -1, TERM_RED, "7-zip not properly installed. Please reinstall it. (www.7-zip.org)");
				Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...");
				inkey();
				return;
			}
		} else {
			/* This case should work on 64-bit Windows (w/ 32-bit client) */
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\7-Zip\\"), 0, KEY_READ | 0x0100, &hTestKey) == ERROR_SUCCESS) {//KEY_WOW64_64KEY (0x0100)
				if (RegQueryValueEx(hTestKey, "Path", NULL, &path_7z_type, path_7z_p, path_7z_size_p) == ERROR_SUCCESS) {
					path_7z[path_7z_size] = '\0';
				} else {
					// odd case
					RegCloseKey(hTestKey);
					Term_putstr(0, 1, -1, TERM_RED, "7-zip not properly installed. Please reinstall it. (www.7-zip.org)");
					Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...");
					inkey();
					return;
				}
			} else {
				Term_putstr(0, 1, -1, TERM_RED, "7-zip not found. Please install it first: www.7-zip.org");
				Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...");
				inkey();
				return;
			}
		}
	}
	RegCloseKey(hTestKey);
	/* enclose full path in quotes, to handle possible spaces */
	strcpy(path_7z_quoted, "\"");
	strcat(path_7z_quoted, path_7z);
	strcat(path_7z_quoted, "\\7z.exe\"");
	strcat(path_7z, "\\7z.exe");

	/* 7zG doesn' support 'l' command so we also need to base '7z.exe', and we boldly assume that it is just here in the same location as 7zG.exe ... */
	/* Unfortunately, 7zG seems to be no longer supported, especially on POSIX.
	   To avoid any problems, I unify all 7z processing to just use the basic 7z(.exe) binary instead,
	   and test for password via hack. - C. Blue */

	/* do the same tests once more as for posix clients */
	fff = fopen("tmp", "w");
	fprintf(fff, "mh");
	fclose(fff);

	_spawnl(_P_WAIT, path_7z, path_7z_quoted, "a", "tmp.7z", "tmp", NULL); /* supposed to work on WINE, yet crashes if not exit(0)ing next oO */
	remove("tmp");

	if (!(fff = fopen("tmp.7z", "r"))) { /* paranoia? */
		Term_putstr(0, 1, -1, TERM_RED, "7-zip wasn't installed properly. Please reinstall it. (www.7-zip.org)");
		Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...");
		inkey();
		return;
	} else if (fgetc(fff) == EOF) { /* normal */
		Term_putstr(0, 1, -1, TERM_RED, "7-zip wasn't installed properly. Please reinstall it. (www.7-zip.org)");
		fclose(fff);
		Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...");
		inkey();
		return;
	}
#else /* assume posix */
	fff = fopen("tmp", "w");
	fclose(fff);
	r = system("7z a tmp.7z tmp");
	remove("tmp");
	if (!(fff = fopen("tmp.7z", "r"))) { /* paranoia? */
		Term_putstr(0, 1, -1, TERM_RED, "7-zip not found ('7z'). Install it first. (Package name is 'p7zip'.)");
		Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...");
		inkey();
		return;
	} else if (fgetc(fff) == EOF) { /* normal */
		Term_putstr(0, 1, -1, TERM_RED, "7-zip not found ('7z'). Install it first. (Package name is 'p7zip'.)");
		fclose(fff);
		Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...");
		inkey();
		return;
	}
#endif
	Term_fresh();
	fclose(fff);
	remove("tmp.7z");

#ifdef WINDOWS
	{ /* Use native Windows functions from dirent to query directory structure directly */
	struct dirent *de;
	DIR *dr;

	if (!(dr = opendir("."))) {
		c_message_add("\377oError: Couldn't scan TomeNET folder.");
		return;
	}
	while ((de = readdir(dr))) {
		/* Clear screen */
		Term_clear();
		Term_putstr(0, 0, -1, TERM_WHITE, "Install a sound or music pack from \377y7z\377w files within your \377yTomeNET\377w folder...");
		Term_putstr(0, 1, -1, TERM_WHITE, "Unarchiver 7-Zip (7z.exe) found.");

		strcpy(pack_name, de->d_name);

		pack_top_folder[0] = 0;
		passworded = FALSE;
		/* Scan for official pack file names and for any 'music*.7z' and 'sound*.7z' file names for 3rd party packs */
		maybe_sound_pack = !strncmp(pack_name, "TomeNET-soundpack", 17) || prefix_case(pack_name, "sound");
		maybe_music_pack = !strncmp(pack_name, "TomeNET-musicpack", 17) || prefix_case(pack_name, "music");
		if (!maybe_sound_pack && !maybe_music_pack) continue;

		/* Test for password protection */
		passworded = test_for_password(path_7z_quoted, pack_name, maybe_sound_pack ? "sound" : "music");

		/* If passworded, ask user for the password: */
		if (passworded) {
			bool retry_pw = TRUE;

			Term_putstr(0, 3, -1, TERM_ORANGE, format("Found file '\377y%s\377o'", pack_name));
			Term_putstr(0, 4, -1, TERM_ORANGE, "which is password-protected. Enter the password:                                ");
			while (retry_pw) {
				Term_gotoxy(49, 4);
				password[0] = 0;
				if (!askfor_aux(password, MAX_CHARS - 1, 0) || !password[0]) {
					c_msg_format("\377yNo password entered for '%s', skipping.", pack_name);
					retry_pw = FALSE;
					continue;
				}
				if (!verify_password(path_7z_quoted, pack_name, maybe_sound_pack ? "sound" : "music", password)) {
					Term_putstr(0, 5, -1, TERM_L_RED, "You entered a wrong password. Please try again.");
					continue;
				}
				Term_putstr(0, 5, -1, TERM_L_RED, "                                               ");
				break;
			}
			/* No password specified, skip this file? */
			if (!retry_pw) continue;
		}

		/* Scan contents for 'tar' file */
		fff = fopen("__tomenethelper.bat", "w");
		if (!fff) {
			c_message_add("\377oError: Couldn't write temporary file (I1).");
			return;
		}
		if (passworded) fprintf(fff, format("@%s -p\"%s\" l \"%s\" > __tomenet.tmp\n", path_7z_quoted, password, pack_name));
		else fprintf(fff, format("@%s l \"%s\" > __tomenet.tmp\n", path_7z_quoted, pack_name));
		fclose(fff);
		_spawnl(_P_WAIT, "__tomenethelper.bat", "__tomenethelper.bat", NULL);
		remove("__tomenethelper.bat");
		fff = fopen("__tomenet.tmp", "r");
		if (!fff) {
			c_message_add("\377oError: Couldn't scan file (1).");
			return;
		}
		l7z = FALSE;
		tarfile = FALSE;
		//format:   '------------------- -----'
		//and then: '2023-08-04 01:12:26 D....            0            0  music' (D=directory)
		while (!feof(fff)) {
			if (!fgets(out_val, 1024, fff)) break;
			if (out_val[0]) out_val[strlen(out_val) - 1] = 0; //strip \n
			else continue;
			/* Skip everything until file contents info begins */
			if (!l7z) {
				if (prefix(out_val, "-------------------")) l7z = TRUE;
				continue;
			}

			/* Scan for '.tar' archive */
			if (!tarfile && suffix_case(out_val, ".tar")) {
				fclose(fff);
				remove("__tomenet.tmp");
				tarfile = TRUE;


				/* Relist the archive, this time correctly as tar-archive */
				l7z = FALSE;
				fff = fopen("__tomenethelper.bat", "w");
				if (!fff) {
					c_message_add("\377oError: Couldn't write temporary file (I2).");
					return;
				}
				if (passworded) fprintf(fff, format("@%s x -p\"%s\" -so \"%s\" | %s l -si -ttar > __tomenet.tmp\n", path_7z_quoted, password, pack_name, path_7z_quoted));
				else fprintf(fff, format("@%s x -so \"%s\" | %s l -si -ttar > __tomenet.tmp\n", path_7z_quoted, pack_name, path_7z_quoted));
				fclose(fff);
				_spawnl(_P_WAIT, "__tomenethelper.bat", "__tomenethelper.bat", NULL);
				remove("__tomenethelper.bat");
				fff = fopen("__tomenet.tmp", "r");
				if (!fff) {
					c_message_add("\377oError: Couldn't scan file (2).");
					return;
				}
				continue;
			}

			/* Scan contents for 'music' or 'sound' folder at top level to identify any kind of compatible archive formats */
			if (strlen(out_val) < 54) continue;
			if (out_val[20] != 'D') continue;
			if (prefix_case(out_val + 53, "music")) {
				music_pack = TRUE;
				strcpy(pack_top_folder, out_val + 53);
				break;
			}
			if (prefix_case(out_val + 53, "sound")) {
				sound_pack = TRUE;
				strcpy(pack_top_folder, out_val + 53);
				break;
			}
		}

		//maybe todo, but not that feasible...
		//c_message_add("\377oError: Invalid sound pack file. Has no top folder 'sound*'.");
		//c_message_add("\377oError: Invalid music pack file. Has no top folder 'music*'.");
		//c_message_add("\377oError: Invalid audio pack file. Has no top folder.");

		if (passworded) Term_putstr(0, 5, -1, TERM_ORANGE, "File is eligible. Install it? [Y/n]");
		else Term_putstr(0, 5, -1, TERM_ORANGE, format("Found file '\377y%s\377o'. Install? [Y/n]", pack_name));
		picked = FALSE;
		while (!picked) {
			c = inkey();
			switch (c) {
			case 'n': case 'N':
				c = 0;
				picked = TRUE;
				break;
			case 'y': case 'Y': case ' ': case '\r': case '\n':
				picked = TRUE;
				break;
			}
		}
		if (c) break;
		picked = FALSE;
	}
	closedir(dr);
	}
#else /* assume POSIX */
	{
	FILE *fff_ls;

	r = system("ls . > __tomenet.tmp");
	fff_ls = fopen("__tomenet.tmp", "r");
	if (!fff_ls) {
		c_message_add("\377oError: Couldn't scan TomeNET folder.");
		return;
	}
	while (!feof(fff_ls)) {
		/* Clear screen */
		Term_clear();
		Term_putstr(0, 0, -1, TERM_WHITE, "Install a sound or music pack from \377y7z\377w files within your \377yTomeNET\377w folder...");
		Term_putstr(0, 1, -1, TERM_WHITE, "Unarchiver (7z) found.");

		if (!fgets(pack_name, 1024, fff_ls)) break;
		if (pack_name[0]) pack_name[strlen(pack_name) - 1] = 0; //'ls' command outputs trailing '/' on each line

		pack_top_folder[0] = 0;
		passworded = FALSE;
		/* Scan for official pack file names and for any 'music*.7z' and 'sound*.7z' file names for 3rd party packs */
		maybe_sound_pack = !strncmp(pack_name, "TomeNET-soundpack", 17) || prefix_case(pack_name, "sound");
		maybe_music_pack = !strncmp(pack_name, "TomeNET-musicpack", 17) || prefix_case(pack_name, "music");
		if (!maybe_sound_pack && !maybe_music_pack) continue;

		/* Test for password protection */
		passworded = test_for_password(NULL, pack_name, maybe_sound_pack ? "sound" : "music");

		/* If passworded, ask user for the password: */
		if (passworded) {
			bool retry_pw = TRUE;

			Term_putstr(0, 3, -1, TERM_ORANGE, format("Found file '\377y%s\377o'", pack_name));
			Term_putstr(0, 4, -1, TERM_ORANGE, "which is password-protected. Enter the password:                                ");
			while (retry_pw) {
				Term_gotoxy(49, 4);
				password[0] = 0;
				if (!askfor_aux(password, MAX_CHARS - 1, 0) || !password[0]) {
					c_msg_format("\377yNo password entered for '%s', skipping.", pack_name);
					retry_pw = FALSE;
					continue;
				}
				if (!verify_password(NULL, pack_name, maybe_sound_pack ? "sound" : "music", password)) {
					Term_putstr(0, 5, -1, TERM_L_RED, "You entered a wrong password. Please try again.");
					continue;
				}
				Term_putstr(0, 5, -1, TERM_L_RED, "                                               ");
				break;
			}
			/* No password specified, skip this file? */
			if (!retry_pw) continue;
		}

		/* Scan contents for 'tar' file */
		if (passworded) r = system(format("7z -p\"%s\" l \"%s\" > __tomenet.tmp", password, pack_name));
		else r = system(format("7z l \"%s\" > __tomenet.tmp", pack_name));
		fff = fopen("__tomenet.tmp", "r");
		if (!fff) {
			c_message_add("\377oError: Couldn't scan file (1).");
			return;
		}
		l7z = FALSE;
		tarfile = FALSE;
		//format:   '------------------- -----'
		//and then: '2023-08-04 01:12:26 D....            0            0  music' (D=directory)
		while (!feof(fff)) {
			if (!fgets(out_val, 1024, fff)) break;
			if (out_val[0]) out_val[strlen(out_val) - 1] = 0; //strip \n
			else continue;
			/* Skip everything until file contents info begins */
			if (!l7z) {
				if (prefix(out_val, "-------------------")) l7z = TRUE;
				continue;
			}

			/* Scan for '.tar' archive */
			if (!tarfile && suffix_case(out_val, ".tar")) {
				fclose(fff);
				remove("__tomenet.tmp");
				tarfile = TRUE;

				/* Relist the archive, this time correctly as tar-archive */
				l7z = FALSE;
				if (passworded) r = system(format("7z x -p\"%s\" -so \"%s\" | 7z l -si -ttar > __tomenet.tmp", password, pack_name));
				else r = system(format("7z x -so \"%s\" | 7z l -si -ttar > __tomenet.tmp", pack_name));
				fff = fopen("__tomenet.tmp", "r");
				if (!fff) {
					c_message_add("\377oError: Couldn't scan file (2).");
					return;
				}
				continue;
			}

			/* Scan contents for 'music' or 'sound' folder at top level to identify any kind of compatible archive formats */
			if (strlen(out_val) < 54) continue;
			if (out_val[20] != 'D') continue;
			if (prefix_case(out_val + 53, "music")) {
				music_pack = TRUE;
				strcpy(pack_top_folder, out_val + 53);
				break;
			}
			if (prefix_case(out_val + 53, "sound")) {
				sound_pack = TRUE;
				strcpy(pack_top_folder, out_val + 53);
				break;
			}
		}

		//maybe todo, but not that feasible...
		//c_message_add("\377oError: Invalid sound pack file. Has no top folder 'sound*'.");
		//c_message_add("\377oError: Invalid music pack file. Has no top folder 'music*'.");
		//c_message_add("\377oError: Invalid audio pack file. Has no top folder.");

		if (passworded) Term_putstr(0, 5, -1, TERM_ORANGE, "File is eligible. Install it? [Y/n]");
		else Term_putstr(0, 5, -1, TERM_ORANGE, format("Found file '\377y%s\377o'. Install? [Y/n]", pack_name));
		picked = FALSE;
		while (!picked) {
			c = inkey();
			switch (c) {
			case 'n': case 'N':
				c = 0;
				picked = TRUE;
				break;
			case 'y': case 'Y': case ' ': case '\r': case '\n':
				picked = TRUE;
				break;
			}
		}
		if (c) break;
		picked = FALSE;
	}
	fclose(fff_ls);
	remove("__tomenet.tmp");
	}
#endif

	if (!sound_pack && !music_pack) {
		Term_putstr(0, 3, -1, TERM_ORANGE, "  No eligible files '\377yTomeNET-soundpack*.7z\377o' nor '\377yTomeNET-musicpack*.7z\377o'   ");
		Term_putstr(0, 4, -1, TERM_ORANGE, "  nor alternative 3rd party archive files '\377ysound*\377o' or '\377ymusic*\377o' were   ");
		Term_putstr(0, 5, -1, TERM_ORANGE, "  found in your TomeNET folder. Aborting audio pack installation.     ");
		Term_putstr(0, 9, -1, TERM_WHITE,  "  Press any key to return to options menu...                          ");
		inkey();
		return;
	}
	if (!picked) {
		Term_putstr(0, 3, -1, TERM_ORANGE, "  You skipped all available audio packs, hence none were installed.   ");
		Term_putstr(0, 4, -1, TERM_ORANGE, "  Aborting audio pack installation.                                   ");
		Term_putstr(0, 9, -1, TERM_WHITE,  "  Press any key to return to options menu...                          ");
		inkey();
		return;
	}

	/* Check what folder name the pack wants to extract to, whether that folder already exists, and allow to cancel the process. */
	strcpy(ins_path, pack_top_folder);
	Term_putstr(0, 6, -1, TERM_ORANGE, "That pack wants to install to this target folder. Is that ok? (y/n):");
#ifdef WINDOWS
	Term_putstr(0, 7, -1, TERM_YELLOW, format(" '%s\\%s'", ANGBAND_DIR_XTRA, ins_path));
#else
	Term_putstr(0, 7, -1, TERM_YELLOW, format(" '%s/%s'", ANGBAND_DIR_XTRA, ins_path));
#endif
	while (TRUE) {
		c = inkey();
		if (c == 'n' || c == 'N') {
			c_message_add("\377yInstallation process has been cancelled.");
			return;
		}
		if (c == 'y' || c == 'Y') break;
	}

#ifdef SOUND_SDL
	/* Windows OS: Need to close all related files so they can actually be overwritten, esp. the .cfg files */
	if (!quiet_mode) close_audio_sdl();
#endif

	/* install sound pack */
	if (sound_pack) {
		Term_putstr(0, 9, -1, TERM_WHITE, "Installing sound pack...                                                    ");
		Term_putstr(0,10, -1, TERM_WHITE, "                                                                            ");
		Term_fresh();
		Term_flush();

#if defined(WINDOWS)
		if (passworded) /* Note: We assume that the password does NOT contain '"' -_- */
			_spawnl(_P_WAIT, path_7z, path_7z_quoted, "x", format("-p\"%s\"", password), format("-o%s", ANGBAND_DIR_XTRA), format("\"%s\"", pack_name), NULL);
		else
			_spawnl(_P_WAIT, path_7z, path_7z_quoted, "x", format("-o%s", ANGBAND_DIR_XTRA), format("\"%s\"", pack_name), NULL);
#else /* assume posix */
		if (passworded) /* Note: We assume that the password does NOT contain '"' -_- */
			r = system(format("7z -p\"%s\" x -o%s \"%s\"", password, ANGBAND_DIR_XTRA, pack_name));
		else
			r = system(format("7z x -o%s \"%s\"", ANGBAND_DIR_XTRA, pack_name));
#endif

		/* Verify installation */
		password_ok = TRUE;
		if (!(fff = fopen(format("%s/%s/sound.cfg", ANGBAND_DIR_XTRA, pack_top_folder), "r"))) password_ok = FALSE;
		else if (fgetc(fff) == EOF) { //paranoia
			password_ok = FALSE;
			fclose(fff);
		} else fclose(fff);
		if (password_ok) {
			Term_putstr(0, 12, -1, TERM_L_GREEN, "Sound pack has been installed. You can select it via '\377gX\377G' in the '=' menu.   ");
			Term_putstr(0, 13, -1, TERM_L_GREEN, "YOU NEED TO RESTART TomeNET FOR THIS TO TAKE EFFECT.                        ");
		} else Term_putstr(0, 12, -1, TERM_L_RED, "Error: sound.cfg not found! Sound pack not correctly installed!");
	}

	/* install music pack */
	if (music_pack) {
		Term_putstr(0, 9, -1, TERM_WHITE, "Installing music pack...                                                    ");
		Term_putstr(0,10, -1, TERM_WHITE, "                                                                            ");
		Term_fresh();
		Term_flush();

#if defined(WINDOWS)
		if (passworded) /* Note: We assume that the password does NOT contain '"' -_- */
			_spawnl(_P_WAIT, path_7z, path_7z_quoted, "x", format("-p\"%s\"", password), format("-o%s", ANGBAND_DIR_XTRA), format("\"%s\"", pack_name), NULL);
		else
			_spawnl(_P_WAIT, path_7z, path_7z_quoted, "x", format("-o%s", ANGBAND_DIR_XTRA), format("\"%s\"", pack_name), NULL);
#else /* assume posix */
		if (passworded) /* Note: We assume that the password does NOT contain '"' -_- */
			r = system(format("7z -p\"%s\" x -o%s \"%s\"", password, ANGBAND_DIR_XTRA, pack_name));
		else
			r = system(format("7z x -o%s \"%s\"", ANGBAND_DIR_XTRA, pack_name));
#endif

		/* Verify installation */
		password_ok = TRUE;
		if (!(fff = fopen(format("%s/%s/music.cfg", ANGBAND_DIR_XTRA, pack_top_folder), "r"))) password_ok = FALSE;
		else if (fgetc(fff) == EOF) { //paranoia
			password_ok = FALSE;
			fclose(fff);
		} else fclose(fff);
		if (password_ok) {
			Term_putstr(0, 12, -1, TERM_L_GREEN, "musc pack has been installed. You can select it via '\377gX\377G' in the '=' menu.   ");
			Term_putstr(0, 13, -1, TERM_L_GREEN, "YOU NEED TO RESTART TomeNET FOR THIS TO TAKE EFFECT.                        ");
		} else Term_putstr(0, 12, -1, TERM_L_RED, "Error: music.cfg not found! Music pack not correctly installed!");
	}

	//slay compiler warning;
	(void)r;

	Term_putstr(0, 15, -1, TERM_WHITE, "Press any key to return to options menu...");
	Term_fresh();
	/* inkey() will react to client timeout after long extraction time, terminating
	   near-instantly with hardly a chance to see the green 'installed' messages. */
	Term_inkey(&ch, TRUE, TRUE);
	//inkey();
}

/* I don't know how well these palette values work. I tried to make them as distinguishable as possible
   while maintaining most colours at their 'original hue'.. Not sure how much sense that makes. - C. Blue */
#define OCB_CMD_Y 21
static void do_cmd_options_colourblindness(void) {
	char ch;
	bool go = TRUE;
	int i, l;
	int r, g, b;
	char buf[MAX_CHARS];
#ifdef WINDOWS
	char bufc[MAX_CHARS];
	long unsigned int c;
#endif

	/* Clear screen */
	Term_clear();

	/* Interact */
	while (go) {
		Term_putstr(0,  0, -1, TERM_L_BLUE, "Colour palette and colour blindness options");
		l = 2;

		Term_putstr(0, l++, -1, TERM_WHITE, "Feedback welcome! Currently, deuteranopia and");
		Term_putstr(0, l++, -1, TERM_WHITE, "protanopia just use the same palette values.");
		Term_putstr(0, l++, -1, TERM_WHITE, "If you use a colourblind setting or custom colours");
		Term_putstr(0, l++, -1, TERM_WHITE, "you probably want to disable palette_animation in \377s=3\377w.");
		l++;

		Term_putstr(0, l++, -1, TERM_WHITE, "Note that you can view a named list of all colours");
		Term_putstr(0, l++, -1, TERM_WHITE, "at any time by typing the '\377s/colours\377w' command in the");
#if 1
		Term_putstr(0, l++, -1, TERM_WHITE, "chat prompt.");
#else
		Term_putstr(0, l++, -1, TERM_WHITE, "chat prompt. The currently used config file is:");
 #ifdef WINDOWS
		Term_putstr(0, l++, -1, TERM_L_WHITE, format("%s", ini_file));
 #else
		Term_putstr(0, l++, -1, TERM_L_WHITE, format("%s", mangrc_filename));
 #endif
#endif
		l++;

		if (c_cfg.palette_animation) Term_putstr(0, l++, -1, TERM_WHITE, "(\377yc\377w) Set a specific palette entry");
		else l++;
		l++;

		Term_putstr(60, 1, -1, TERM_L_WHITE, " Current palette:");
		/* Note: colour 0 is always fixed, unchangeable 0x000000, so we just skip it */
#ifndef CUSTOMIZE_COLOURS_FREELY
		for (i = 1; i < BASE_PALETTE_SIZE; i++) {
#else
		for (i = 0; i < BASE_PALETTE_SIZE; i++) {
			if (!i) Term_putstr(55, 2 + i, -1, 1, format("%2d %s", i, colour_name[i]));
			else
#endif
			Term_putstr(55, 2 + i, -1, i, format("%2d %s", i, colour_name[i]));
			Term_putstr(66, 2 + i, -1, TERM_WHITE, format("%3d, %3d, %3d",
			    (client_color_map[i] & 0xFF0000) >> 16,
			    (client_color_map[i] & 0x00FF00) >> 8,
			    client_color_map[i] & 0x0000FF));
		}

		if (c_cfg.palette_animation) {
			Term_putstr(0, l++, -1, TERM_WHITE, "(\377yn\377w) Set palette to normal colours");
			Term_putstr(0, l++, -1, TERM_WHITE, "(\377yd\377w) Set palette to Deuteranopia colours");
			Term_putstr(0, l++, -1, TERM_WHITE, "(\377yp\377w) Set palette to Protanopia colours");
			Term_putstr(0, l++, -1, TERM_WHITE, "(\377yt\377w) Set palette to Tritanopia colours");
			l++;

#ifdef WINDOWS
			Term_putstr(0, l++, -1, TERM_WHITE, "(\377ys\377w) Save (modified) palette to current INI file");
			Term_putstr(0, l++, -1, TERM_WHITE, "(\377yr\377w) Reset palette to values from current INI file");
			Term_putstr(0, l, -1, TERM_SLATE, format("    (Filename: %s)", ini_file));
#elif USE_X11
			Term_putstr(0, l++, -1, TERM_WHITE, "(\377ys\377w) Save (modified) palette to current rc-file");
			Term_putstr(0, l++, -1, TERM_WHITE, "(\377yr\377w) Reset palette to values from current rc-file");
			Term_putstr(0, l, -1, TERM_SLATE, format("    (Filename: %s)", mangrc_filename));
#else
			Term_putstr(0, l++, -1, TERM_L_DARK, "(This menu is not supported in current OS/mode)");
#endif
		} else {
			Term_putstr(0, l++, -1, TERM_L_RED, "Sorry, palette-colours and related options are not");
			Term_putstr(0, l++, -1, TERM_L_RED, "modifiable in command-line client mode (GCU) or if");
			Term_putstr(0, l++, -1, TERM_L_RED, "the 'palette_animation' option (=1) is disabled.");
			l += 5;
		}

		/* Hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Get key */
		ch = inkey();

		l = OCB_CMD_Y;
		Term_putstr(0, l, -1, TERM_WHITE, "                                       ");

		/* Analyze */
		switch (ch) {
		case ESCAPE:
			go = FALSE;
			break;

		/* specialty: allow chatting from within here */
		case ':': {
			bool inkey_msg_old = inkey_msg;

			cmd_message();
			inkey_msg = inkey_msg_old; /* And suppress macros again.. */
			continue;
			}

		case 'c':
			if (!c_cfg.palette_animation) continue;
			l = OCB_CMD_Y;
#ifndef CUSTOMIZE_COLOURS_FREELY
			Term_putstr(0, l, -1, TERM_L_WHITE, "Enter the colour index (1-15): ");
			strcpy(buf, "1");
			if (!askfor_aux(buf, 2, 0)) {
				Term_putstr(0, l, -1, TERM_WHITE, "                                     ");
				continue;
			}
			i = atoi(buf);
			if (i < 1 || i > 15) {
				Term_putstr(0, l, -1, TERM_WHITE, "                                     ");
				continue;
			}
#else
			Term_putstr(0, l, -1, TERM_L_WHITE, "Enter the colour index (0-15): ");
			strcpy(buf, "1");
			if (!askfor_aux(buf, 2, 0)) {
				Term_putstr(0, l, -1, TERM_WHITE, "                                     ");
				continue;
			}
			i = atoi(buf);
			if (i < 0 || i > 15) {
				Term_putstr(0, l, -1, TERM_WHITE, "                                     ");
				continue;
			}
#endif
			Term_putstr(0, l, -1, TERM_WHITE, "                                     ");

			Term_putstr(0, l, -1, TERM_L_RED, "Enter new RED value (0-255)  : ");
			strcpy(buf, "255");
			if (!askfor_aux(buf, 3, 0)) {
				Term_putstr(0, l, -1, TERM_WHITE, "                                     ");
				continue;
			}
			r = atoi(buf);
			if (r < 0 || r > 255) {
				Term_putstr(0, l, -1, TERM_WHITE, "                                     ");
				continue;
			}
			Term_putstr(0, l, -1, TERM_WHITE, "                                     ");

			Term_putstr(0, l, -1, TERM_L_GREEN, "Enter new GREEN value (0-255): ");
			strcpy(buf, "255");
			if (!askfor_aux(buf, 3, 0)) {
				Term_putstr(0, l, -1, TERM_WHITE, "                                     ");
				continue;
			}
			g = atoi(buf);
			if (g < 0 || g > 255) {
				Term_putstr(0, l, -1, TERM_WHITE, "                                     ");
				continue;
			}
			Term_putstr(0, l, -1, TERM_WHITE, "                                     ");

			Term_putstr(0, l, -1, TERM_L_BLUE, "Enter new BLUE value (0-255) : ");
			strcpy(buf, "255");
			if (!askfor_aux(buf, 3, 0)) {
				Term_putstr(0, l, -1, TERM_WHITE, "                                     ");
				continue;
			}
			b = atoi(buf);
			if (b < 0 || b > 255) {
				Term_putstr(0, l, -1, TERM_WHITE, "                                     ");
				continue;
			}
			Term_putstr(0, l, -1, TERM_WHITE, "                                     ");

#ifndef CUSTOMIZE_COLOURS_FREELY
			/* Black is not allowed (to prevent the user from locking himself out visually =p not a great help but pft) */
			if (!r && !g && !b) {
				Term_putstr(0, l, -1, TERM_YELLOW, "Sorry, setting any colour to black (0,0,0) is not allowed.");
				c_message_add("\377ySorry, setting any colour to black (0,0,0) is not allowed.");
				continue;
			}
#endif

			client_color_map[i] = (r << 16) | (g << 8) | b;
#ifdef CUSTOMIZE_COLOURS_FREELY
			/* Safe-fail: Never allow colours 0 (bg) and 1 (fg) to be identical */
			if (client_color_map[0] == client_color_map[1]) {
				if (i == 0) client_color_map[1] = 0xFFFFFF - client_color_map[0];
				else if (i == 1) client_color_map[0] = 0xFFFFFF - client_color_map[1];
			}
			/* Until live-'bg'-modification to new colour #0 values is fixed in set_palette(), hint to user to save and restart ASAP to fix the visuals */
			if (!i) {
				c_msg_print("\377RYou changed colour #0! To avoid messed up visuals, press 's' now");
				c_msg_print("\377R to save this palette to your current config file and restart your client!");
			}
#endif
			set_palette(i, r, g, b);
			refresh_palette();
			break;

		case 'n':
			if (!c_cfg.palette_animation) continue;
			for (i = 0; i < CLIENT_PALETTE_SIZE; i++) {
				client_color_map[i] = client_color_map_org[i];
				if ((i == 6 || i == BASE_PALETTE_SIZE + 6) && lighterdarkblue && client_color_map[i] == 0x0000ff)
#ifdef WINDOWS
					enable_readability_blue_win();
#else
 #ifdef USE_X11
					enable_readability_blue_x11();
 #else
					enable_readability_blue_gcu();
 #endif
#endif
				set_palette(i, (client_color_map[i] & 0xFF0000) >> 16, (client_color_map[i] & 0x00FF00) >> 8, (client_color_map[i] & 0x0000FF));
			}
			refresh_palette();
			break;

		case 'd':
			if (!c_cfg.palette_animation) continue;
			for (i = 0; i < CLIENT_PALETTE_SIZE; i++) {
				client_color_map[i] = client_color_map_deu[i];
				set_palette(i, (client_color_map[i] & 0xFF0000) >> 16, (client_color_map[i] & 0x00FF00) >> 8, client_color_map[i] & 0x0000FF);
			}
			refresh_palette();
			break;

		case 'p':
			if (!c_cfg.palette_animation) continue;
			for (i = 0; i < CLIENT_PALETTE_SIZE; i++) {
				client_color_map[i] = client_color_map_pro[i];
				set_palette(i, (client_color_map[i] & 0xFF0000) >> 16, (client_color_map[i] & 0x00FF00) >> 8, client_color_map[i] & 0x0000FF);
			}
			refresh_palette();
			break;

		case 't':
			if (!c_cfg.palette_animation) continue;
			for (i = 0; i < CLIENT_PALETTE_SIZE; i++) {
				client_color_map[i] = client_color_map_tri[i];
				set_palette(i, (client_color_map[i] & 0xFF0000) >> 16, (client_color_map[i] & 0x00FF00) >> 8, client_color_map[i] & 0x0000FF);
			}
			refresh_palette();
			break;

		case 's':
			if (!c_cfg.palette_animation) continue;
#ifdef WINDOWS
 #ifndef CUSTOMIZE_COLOURS_FREELY
			for (i = 1; i < BASE_PALETTE_SIZE; i++) {
 #else
			for (i = 0; i < BASE_PALETTE_SIZE; i++) {
 #endif
				sprintf(buf, "Colormap_%d", i);
				c = client_color_map[i];
				sprintf(bufc,  "#%06lx", c);
				WritePrivateProfileString("Base", buf, bufc, ini_file);
			}
#else
			write_mangrc_colourmap();
#endif
			l = OCB_CMD_Y;
			Term_putstr(0, l, -1, TERM_L_WHITE, "Configuration file has been updated.  ");
			c_message_add("Configuration file has been updated.  ");
			break;

		case 'r':
			if (!c_cfg.palette_animation) continue;
			for (i = 0; i < CLIENT_PALETTE_SIZE; i++) {
				client_color_map[i] = client_color_map_org[i];
				if ((i == 6 || i == BASE_PALETTE_SIZE + 6) && lighterdarkblue && client_color_map[i] == 0x0000ff)
#ifdef WINDOWS
					enable_readability_blue_win();
#else
 #ifdef USE_X11
					enable_readability_blue_x11();
 #else
					enable_readability_blue_gcu();
 #endif
#endif
				set_palette(i, (client_color_map[i] & 0xFF0000) >> 16, (client_color_map[i] & 0x00FF00) >> 8, (client_color_map[i] & 0x0000FF));
			}
			l = OCB_CMD_Y;
			Term_putstr(0, l, -1, TERM_L_WHITE, "Colours reset from Configuration file.");
			c_message_add("Colours reset from configuration file.");
			refresh_palette();
			break;

		default:
			bell();
			continue;
		}

		/* Exit */
		if (!go) break;

		/* Redraw ALL windows with new palette colours */
		//if (!c_cfg.palette_animation) refresh_palette();
	}
}

/*
 * Set or unset various options.
 *
 * The user must use the "Ctrl-R" command to "adapt" to changes
 * in any options which control "visual" aspects of the game.
 */
void do_cmd_options(void) {
	int k, l;
	char tmp[1024];
	static char src[1024] = { 0 };

	options_immediate(TRUE);

	/* Save the screen */
	Term_save();

	/* suppress hybrid macros */
	bool inkey_msg_old = inkey_msg;
	inkey_msg = TRUE;

	/* Interact */
	while (1) {
		/* Clear screen */
		Term_clear();

		/* Why are we here */
		Term_putstr(0, 0, -1, TERM_L_GREEN, "TomeNET options - press '\377y/\377G' to search for an option");

		/* Give some choices */
		l = 2;
		Term_putstr(1, l++, -1, TERM_WHITE, "(\377y1\377w-\377y5\377w)   User interface options (Base+Vis/Visuals/Format/Notifications/Messages)");
		Term_putstr(1, l++, -1, TERM_WHITE, "(\377y6\377w-\377y7\377w)   Audio options (SFX+Music/Paging+OS)");
		Term_putstr(1, l++, -1, TERM_WHITE, "(\377y8\377w-\377y0\377w)   Gameplay options (Actions+Safety/Disturbances/Items)");
#ifdef WINDOWS
		Term_putstr(1, l++, -1, TERM_WHITE, format("(\377yw\377w/\377yE\377w)  Window flags / %s%s",
		    disable_CS_IME ? "Force IME on" : (enable_CS_IME ? "Auto-IME" : "Force IME off"),
		    (disable_CS_IME || enable_CS_IME) ? "" : (suggest_IME ? " (Currently Auto-ON)" : " (Currently Auto-OFF)")));
#else
		Term_putstr(1, l++, -1, TERM_WHITE, "(\377yw\377w)     Window flags");
#endif
#ifdef GLOBAL_BIG_MAP
		if (strcmp(ANGBAND_SYS, "gcu"))
			Term_putstr(1, l++, -1, TERM_WHITE, "(\377yb\377w/\377yM\377w/\377ym\377w) Toggle/enable/disable big_map option (double screen height)");
		else
			Term_putstr(1, l++, -1, TERM_L_DARK, "(\377sb\377D/\377sM\377D/\377sm\377D) Toggle/enable/disable big_map (double size) - NOT AVAILABLE ON GCU");
#endif
		l++;
		Term_putstr(1, l++, -1, TERM_WHITE, "(\377os\377w/\377oS\377w/\377oa\377w) Save options & flags / to global.opt (account) / to class-specific file");
		Term_putstr(1, l++, -1, TERM_WHITE, "(\377ol\377w)     Load all options & flags");
		if (strcmp(ANGBAND_SYS, "gcu")) {
			Term_putstr(1, l++, -1, TERM_WHITE, "(\377oT\377w)     Save current window, positions and sizes to current config file");

#ifdef WINDOWS
			Term_putstr(1, l++, -1, TERM_SLATE, format("        (Filename: %s)", ini_file));
#elif USE_X11
			Term_putstr(1, l++, -1, TERM_SLATE, format("        (Filename: %s)", mangrc_filename));
#else
			l++; //paranoia
#endif
		} else {
			Term_putstr(1, l++, -1, TERM_L_DARK, "(\377oT\377D)       Save current windows, positions and sizes - NOT AVAILABLE ON GCU");
			l++;
		}

		Term_putstr(1, l++, -1, TERM_L_DARK, "-----------------------------------------------------------------------------");

		Term_putstr(1, l++, -1, TERM_SLATE, "The following settings are mostly saved automatically on quitting via CTRL+Q:");
		l++;
#ifdef USE_SOUND_2010
		if (c_cfg.rogue_like_commands)
			Term_putstr(1, l++, -1, TERM_WHITE, "(\377yx\377w/\377yX\377w) Audio mixer (also accessible via CTRL+F hotkey) / Audio pack selector");
		else
			Term_putstr(1, l++, -1, TERM_WHITE, "(\377yx\377w/\377yX\377w) Audio mixer (also accessible via CTRL+U hotkey) / Audio pack selector");

		Term_putstr(1, l++, -1, TERM_WHITE, "(\377yn\377w/\377yN\377w) Jukebox, listen to and disable/reenable specific sound effects/music");
#endif

#if defined(WINDOWS) || defined(USE_X11)
		/* Font (and window) settings aren't available in command-line mode */
		if (strcmp(ANGBAND_SYS, "gcu")) {
 #ifdef ENABLE_SUBWINDOW_MENU
  #ifdef USE_GRAPHICS
			Term_putstr(1, l++, -1, TERM_WHITE, "(\377yf\377w/\377yg\377w) Window Fonts and Visibility / Graphical tilesets");
  #else
			Term_putstr(1, l++, -1, TERM_WHITE, "(\377yf\377w)   Window Fonts and Visibility");
  #endif
 #endif
			/* CHANGE_FONTS_X11 */
			Term_putstr(1, l++, -1, TERM_WHITE, "(\377yF\377w)   Cycle all font sizes at once (can be tapped multiple times)");
		}
#endif
		Term_putstr(1, l++, -1, TERM_WHITE, "(\377yc\377w)   Colour palette and colour blindness options");
		l++;

#ifdef WINDOWS
		Term_putstr(1, l++, -1, TERM_WHITE, "(\377UA\377w/\377UY\377w) Account Options / Re-copy user/scpt folders to user home folder.");
#else
		Term_putstr(1, l++, -1, TERM_WHITE, "(\377UA\377w)   Account Options");
#endif
		Term_putstr(1, l++, -1, TERM_WHITE, "(\377UI\377w)   Install sound/music pack from 7z-file you placed in your TomeNET folder");
#ifdef WINDOWS
		Term_putstr(1, l++, -1, TERM_WHITE, "(\377UC\377w/\377UU\377w) Check / Update the TomeNET Guide (downloads and reinits the Guide)");
#else
		Term_putstr(1, l++, -1, TERM_WHITE, "(\377UC\377w/\377UU\377w) Check / Update the TomeNET Guide (requires 'wget' package installed)");
#endif

		/* hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Get command */
		k = inkey();

		/* Exit */
		if (k == ESCAPE || k == KTRL('Q')) break;

		/* Take a screenshot */
		if (k == KTRL('T')) xhtml_screenshot("screenshot????", 2);
		/* specialty: allow chatting from within here */
		else if (k == ':') {
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
		}

		else if (k == '1') {
			do_cmd_options_aux(1, "UI Opt. 1 (Base/Vis)", -1);
		} else if (k == '2') {
			do_cmd_options_aux(4, "UI Opt. 2 (Visuals)", -1);
		} else if (k == '3') {
			do_cmd_options_aux(6, "UI Opt. 3 (Formatting)", -1);
		} else if (k == '4') {
			do_cmd_options_aux(7, "UI Opt. 4 (Notifications)", -1);
		} else if (k == '5') {
			do_cmd_options_aux(0, "UI Opt. 5 (Messages)", -1);
		} else if (k == '6') {
			do_cmd_options_aux(5, "Audio Opt. 1 (SFX+Music)", -1);
		} else if (k == '7') {
			do_cmd_options_aux(9, "Audio Opt. 2 (Paging+OS)", -1);
		} else if (k == '8') {
			do_cmd_options_aux(2, "Play Opt. 1 (Actions/Safety)", -1);
		} else if (k == '9') {
			do_cmd_options_aux(3, "Play Opt. 2 (Disturbances)", -1);
		} else if (k == '0') {
			do_cmd_options_aux(8, "Play Opt. 3 (Items)", -1);
		}

		/* Search for an option (by name) */
		else if (k == '/') {
			int m;
			bool found = FALSE;

			Term_putstr(0, 23, -1, TERM_YELLOW, "Enter (partial) option name: ");
			if (!askfor_aux(src, 70, 0)) continue;

			while (TRUE) {
				for (l = 0; l < OPT_MAX; l++) {
					if (!option_info[l].o_desc) continue; //option exists?
					if (!option_info[l].o_enabled) continue; //option is disabled? not eligible for searching then
					if (!my_strcasestr(option_info[l].o_text, src)) continue; //(partial) match?

					found = TRUE;
					k = 0;
					for (m = 0; m < l; m++) {
						if (!option_info[m].o_enabled) continue; //option is disabled? not listed then
						if (option_info[m].o_page == option_info[l].o_page) k++;
					}

					switch(option_info[l].o_page) {
					case 1: m = do_cmd_options_aux(1, "UI Opt. 1 (Base/Vis)", k); break;
					case 4: m = do_cmd_options_aux(4, "UI Opt. 2 (Visuals)", k); break;
					case 6: m = do_cmd_options_aux(6, "UI Opt. 3 (Formatting)", k); break;
					case 7: m = do_cmd_options_aux(7, "UI Opt. 4 (Notifications)", k); break;
					case 0: m = do_cmd_options_aux(0, "UI Opt. 5 (Messages)", k); break;
					case 5: m = do_cmd_options_aux(5, "Audio Opt. 1 (SFX+Music)", k); break;
					case 9: m = do_cmd_options_aux(9, "Audio Opt. 2 (Paging+OS)", k); break;
					case 2: m = do_cmd_options_aux(2, "Play Opt. 1 (Actions/Safety)", k); break;
					case 3: m = do_cmd_options_aux(3, "Play Opt. 2 (Disturbances)", k); break;
					case 8: m = do_cmd_options_aux(8, "Play Opt. 3 (Items)", k); break;
					default: m = found = FALSE; c_msg_print("Option not found.");
					}

					/* Don't continue searching? */
					if (!m) {
						found = FALSE;
						break;
					}
				}
				/* If we're not looking for subsequent results, leave the options page instead of wrapping around */
				if (!found) {
					if (l == OPT_MAX) c_msg_format("\377yOption '%s' not found.", src);
					break;
				}
				/* ..otherwise start from the beginning again */
			}
		}

		/* Save a 'option' file */
		else if (k == 's') {
			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 23, -1, TERM_YELLOW, "Save to file ('global.opt' for account-wide): ");

			/* Default filename */
			sprintf(tmp, "%s.opt", cname);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Dump the macros */
			(void)options_dump(tmp);
		} else if (k == 'S') {
			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 23, -1, TERM_YELLOW, "Save to file 'global.opt' (account-wide): ");

			/* Default filename */
			strcpy(tmp, "global.opt");

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Dump the macros */
			(void)options_dump(tmp);
		} else if (k == 'a') {
			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 23, -1, TERM_YELLOW, "Save to class-specific .opt file: ");

			/* Default filename */
			strcpy(tmp, format("%s.opt", p_ptr->cp_ptr->title));

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Dump the macros */
			(void)options_dump(tmp);
		} else if (k == 'T') {
			if (!strcmp(ANGBAND_SYS, "gcu")) {
				c_message_add("\377ySorry, windows are not available in the GCU (command-line) client.");
				continue;
			}
#ifdef WINDOWS
			save_term_data_to_term_prefs();
			c_msg_format("\377wSaved current configuration to %s.", ini_file);
#elif USE_X11
			all_term_data_to_term_prefs();
			write_mangrc(FALSE, FALSE, FALSE);
			c_msg_format("\377wSaved current configuration to %s.", mangrc_filename);
#else
			c_msg_print("\377wCannot save configuration from menu, please quit to save.");
#endif
		}
		/* Load a pref file */
		else if (k == 'l') {
			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 23, -1, TERM_YELLOW, "Load file: ");

			/* Default filename */
			//sprintf(tmp, "global.opt");
			sprintf(tmp, "%s.opt", cname);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Process the given filename */
			if (process_pref_file(tmp) == -1) c_msg_format("\377yError: Can't read options file '%s'!", tmp);
		}

		/* Account options */
		else if (k == 'A') do_cmd_options_acc();

		/* Check the TomeNET Guide for outdatedness */
		else if (k == 'C') {
			if (check_guide_checksums(TRUE)) c_msg_print("\377yCannot check whether Guide is outdated or not.");
			else if (guide_outdated) c_msg_print("\377yYour Guide is outdated. You can update it right now by pressing: \377sU");
			else c_msg_print("\377gYour Guide is up to date!");
		}

		/* Update the TomeNET Guide */
		else if (k == 'U') {
			int res;
#ifdef WINDOWS
			char _latest_install[1024];

			remove("TomeNET-Guide.txt.old");
			rename("TomeNET-Guide.txt", "TomeNET-Guide.txt.old");
			/* Check if renaming failed -> we don't have write access! */
			if (my_fexists("TomeNET-Guide.txt")) {
				c_msg_print("\377oFailed to backup current Guide. Maybe file is write-protected.");
				c_msg_print("\377o Although bad practice, you could try running TomeNET as administrator.");
				continue;
			}

			strcpy(_latest_install, "https://www.tomenet.eu/TomeNET-Guide.txt"); //argh, note that wget.exe doesn't support https protocol! need to use http
			res = _spawnl(_P_WAIT, "updater\\wget.exe", "wget.exe", "--dot-style=mega", _latest_install, NULL);
			if (res != 0) {
				c_msg_print("\377oFailed to download the Guide.");
				c_msg_print("\377o Although bad practice, you could try running TomeNET as administrator.");
				/* Reinstantiate our version */
				rename("TomeNET-Guide.txt.old", "TomeNET-Guide.txt");
			} else {
				c_msg_print("\377gSuccessfully downloaded the Guide.");
				init_guide();
				/* correct way would be: First download the checksum file, then download the guide, then verify if checksum fits;
				   or what 'shouldn't' happen could actually happen, if timing is very bad: */
				if (check_guide_checksums(TRUE)) c_msg_print("\377yCannot check whether your Guide is outdated or not.");
				else if (guide_outdated) c_msg_print("\377yYour Guide is still outdated. This shouldn't happen.");
				else c_msg_print("\377gYour Guide is now up to date!");
				//c_msg_format("Guide reinitialized. (errno %d,lastline %d,endofcontents %d,chapters %d)", guide_errno, guide_lastline, guide_endofcontents, guide_chapters);
			}
#else
			FILE *fp;
			char out_val[3];

			remove("TomeNET-Guide.txt.old");
			rename("TomeNET-Guide.txt", "TomeNET-Guide.txt.old");
			/* Check if renaming failed -> we don't have write access! */
			if (my_fexists("TomeNET-Guide.txt")) {
				c_msg_print("\377oFailed to backup current Guide. Maybe file is write-protected.");
				continue;
			}

			(void)system("wget --timeout=3 https://www.tomenet.eu/TomeNET-Guide.txt"); //something changed in the web server's cfg; curl still works fine, but now wget needs the timeout setting; wget.exe for Windows still works!

			fp = fopen("TomeNET-Guide.txt", "r");
			if (fp) {//~paranoia?
				out_val[0] = 0;
				if (!fgets(out_val, 2, fp)) { //paranoia: guide is write-protected?
					fclose(fp);
					c_msg_print("\377oFailed to download the Guide contents. Maybe file is write-protected.");
					continue;
				}
				fclose(fp);
				res = (out_val[0] < 32);
				if (res != 0) {
					c_msg_print("\377oFailed to update the Guide.");
					/* Reinstantiate our version */
					rename("TomeNET-Guide.txt.old", "TomeNET-Guide.txt");
				} else {
					c_msg_print("\377gSuccessfully downloaded the Guide.");
					init_guide();
					/* correct way would be: First download the checksum file, then download the guide, then verify if checksum fits;
					   or what 'shouldn't' happen could actually happen, if timing is very bad: */
					if (check_guide_checksums(TRUE)) c_msg_print("\377yCannot check whether your Guide is outdated or not.");
					else if (guide_outdated) c_msg_print("\377yYouy Guide is still outdated. This shouldn't happen.");
					else c_msg_print("\377gYour Guide is now up to date!");
					//c_msg_format("Guide reinitialized. (errno %d,lastline %d,endofcontents %d,chapters %d)", guide_errno, guide_lastline, guide_endofcontents, guide_chapters);
				}
			} else {
				c_msg_print("\377oFailed to download the Guide.");
				/* Reinstantiate our version */
				rename("TomeNET-Guide.txt.old", "TomeNET-Guide.txt");
			}
#endif
		}

		/* Window flags */
		else if (k == 'w') {
			/* Spawn */
			do_cmd_options_win();
		}
#ifdef GLOBAL_BIG_MAP
		/* Toggle big_map */
		else if (k == 'b') set_bigmap(-1, FALSE);
		/* Enable big_map */
		else if (k == 'M') set_bigmap(1, FALSE);
		/* Disable big_map */
		else if (k == 'm') set_bigmap(0, FALSE);
#endif

#ifdef WINDOWS
		else if (k == 'E') {
			/* Cycle 3 steps in order: */
			if (disable_CS_IME) { // force off? -> force on
				disable_CS_IME = FALSE;
				enable_CS_IME = TRUE;
				c_msg_print("\377yIME support is now forced-on. Requires a client restart (use CTRL+Q).");
			} else if (enable_CS_IME) { // force on? -> force nothing (accept auto-choice based on OS language)
				enable_CS_IME = FALSE;
				disable_CS_IME = FALSE;
				c_msg_print("\377yIME support is now automatic (standard). Requires a client restart (use CTRL+Q).");
			} else { // force nothing? -> force on
				disable_CS_IME = TRUE;
				enable_CS_IME = FALSE;
				c_msg_print("\377yIME support is now forced-off. Requires a client restart (use CTRL+Q).");
			}
			INI_disable_CS_IME = disable_CS_IME;
			INI_enable_CS_IME = enable_CS_IME;
		}
#endif

#if defined(WINDOWS) || defined(USE_X11)
 #ifdef ENABLE_SUBWINDOW_MENU
		/* Change fonts separately and manually */
		else if (k == 'f') do_cmd_options_fonts();
  #ifdef USE_GRAPHICS
		/* Enable/disable and select graphical tilesets */
		else if (k == 'g') do_cmd_options_tilesets();
  #endif
 #endif
		/* Cycle all fonts */
		else if (k == 'F') change_font(-1);
#endif

#ifdef USE_SOUND_2010
		/* Access audio mixer */
		else if (k == 'x') interact_audio();
		else if (k == 'X') audio_pack_selector();
		/* Toggle single sfx/song from a list of all */
		else if (k == 'n') do_cmd_options_sfx();
		else if (k == 'N') do_cmd_options_mus();
#endif
		else if (k == 'I') do_cmd_options_install_audio_packs();

		else if (k == 'c') do_cmd_options_colourblindness();

#ifdef WINDOWS
		else if (k == 'Y') {
			if (getenv("HOMEDRIVE") && getenv("HOMEPATH")) { //ignore win_dontmoveuser here, as user asked for it deliberately
				//char out_val[1024];

				if (!strcmp(format("%sscpt", ANGBAND_DIR), ANGBAND_DIR_SCPT)) {
					c_msg_format("\377ySource and target scpt folder are the same: '%s'. Skipping.", ANGBAND_DIR_SCPT);
				} else {
					/* Copy to 'scpt' folder */
					/* make sure it exists (paranoia except if user did something very silyl) */
					if (!check_dir(ANGBAND_DIR_SCPT)) {
						mkdir(ANGBAND_DIR_SCPT);
						c_msg_format("\377wCreated target folder %s.", ANGBAND_DIR_SCPT);
					}
					/* copy over the default files from the installation folder */
					c_msg_format("\377wCopying %sscpt to %s.", ANGBAND_DIR, ANGBAND_DIR_SCPT);
 #if 1 /* Copy in the background? */
					WinExec(format("xcopy /I /E /Y /H /C %s%s \"%s\"", ANGBAND_DIR, "scpt", ANGBAND_DIR_SCPT), SW_NORMAL); //SW_HIDE);
 #else /* Will timeout the client if LOTS of screenshots etc ^^ */
					sprintf(out_val, "xcopy /I /E /Y /H /C %s%s \"%s\"", ANGBAND_DIR, "scpt", ANGBAND_DIR_SCPT);
					system(out_val);
 #endif
				}


				if (!strcmp(format("%suser", ANGBAND_DIR), ANGBAND_DIR_USER)) {
					c_msg_format("\377ySource and target user folder are the same: '%s'. Skipping.", ANGBAND_DIR_USER);
				} else {
					/* Copy to 'user' folder */
					/* make sure it exists (paranoia except if user did something very silyl) */
					if (!check_dir(ANGBAND_DIR_USER)) {
						mkdir(ANGBAND_DIR_USER);
						c_msg_format("\377wCreated target folder %s.", ANGBAND_DIR_USER);
					}
					/* copy over the default files from the installation folder */
					c_msg_format("\377wCopying %suser to %s.", ANGBAND_DIR, ANGBAND_DIR_USER);
 #if 1 /* Copy in the background? */
					WinExec(format("xcopy /I /E /Y /H /C %s%s \"%s\"", ANGBAND_DIR, "user", ANGBAND_DIR_USER), SW_NORMAL); //SW_HIDE);
 #else /* Will timeout the client if LOTS of screenshots etc ^^ */
					sprintf(out_val, "xcopy /I /E /Y /H /C %s%s \"%s\"", ANGBAND_DIR, "user", ANGBAND_DIR_USER);
					system(out_val);
 #endif
				}
			} else c_msg_print("\377yFailure: Environment variables HOMEDRIVE and/or HOMEPATH not available.");
		}
#endif

		/* Unknown option */
		else {
			/* Oops */
			bell();
		}
	}

	/* Restore the screen */
	Term_load();

	Flush_queue();

	/* Verify the keymap */
	keymap_init();

	options_immediate(FALSE);

	inkey_msg = inkey_msg_old;

	/* hide cursor -- just for when the main screen font has been changed, it seems after
	   that the cursor is now visible for some reason (and jumping around annoyingly) */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	/* Resend options to server */
	Send_options();

	if (custom_font_warning) {
		custom_font_warning = FALSE;
		if (c_cfg.font_map_solid_walls) c_msg_print("\377wCustom font loaded. If visuals seem wrong, disable '\377yfont_map_solid_walls\377w' in =2.");
	}
}


/*
 * For print_tomb() only - Centers a string within a 31 character string		-JWT-
 */
static void center_string(char *buf, cptr str) {
	int i, j;

	/* Total length */
	i = strlen(str);

	/* Necessary border */
	j = 15 - (i - 1) / 2;

	/* Mega-Hack */
	(void)sprintf(buf, "%*s%s%*s", j, "", str, 31 - i - j, "");
}
static void center_string_short(char *buf, cptr str) {
	int i, j;

	/* Total length */
	i = strlen(str);

	/* Necessary border */
	j = 10 - (i + 1) / 2;

	/* Mega-Hack */
	(void)sprintf(buf, "%*s%s%*s", j, "", str, 20 - i - j, "");
}


/*
 * Display a "tomb-stone"
 */
/* ToME parts. */
#define STONE_COL 11
#define STONE_COL_SHORT (11 + 6)
static void print_tomb(cptr reason) {
	bool done = FALSE;

#ifdef USE_SOUND_2010
	/* Fade out any previous ambient and weather sfx, we're at the graveyard now */
	if (use_sound) {
		sound_ambient(-1);
		sound_weather(-1);
	}
#endif

#ifdef ATMOSPHERIC_INTRO /* also extro =P */
 #ifdef USE_SOUND_2010
  #if 1
	if (use_sound) {
   #if 0
		/* switch to login screen music if available, or fade music out */
		if (insanity_death) {
			if (!music(exec_lua(0, "return get_music_index(\"tomb_insanity\")")))
				if (!music(exec_lua(0, "return get_music_index(\"tomb\")")))
					music(-4);
		} else {
			if (!music(exec_lua(0, "return get_music_index(\"tomb\")"))) music(-4);
		}
   #else
		/* switch to login screen music if available; otherwise just continue playing the current in-game music */
		if (insanity_death) {
			if (!music(exec_lua(0, "return get_music_index(\"tomb_insanity\")")))
				music(exec_lua(0, "return get_music_index(\"tomb\")"));
		} else music(exec_lua(0, "return get_music_index(\"tomb\")"));
   #endif
	}
  #endif
 #endif
#endif

	/* Print the text-tombstone */
	if (!done) {
		char	tmp[160], reason2[MAX_CHARS];
		char	buf[1024], buf2[1024 * 2];
		FILE	*fp;
		time_t	ct = time(NULL);

		/* Clear screen */
		Term_clear();

		/* Build the filename */
		path_build(buf, 1024, ANGBAND_DIR_TEXT, (ct % 2) == 0 ? "dead.txt" : "dead2.txt");

		/* Open the News file */
		fp = my_fopen(buf, "r");

		/* Dump */
		if (fp) {
			int i = 0;

			/* Dump the file to the screen */
			while (0 == my_fgets(fp, buf, 1024)) {
#if 1 /* allow colour code shortcut: '\{' = colour, '{' = normal */
				char *t = buf, *t2 = buf2;

				while (*t) {
					if (*t != '\\') *t2++ = *t++;
					else {
						/* double code ? -> colour */
						if (*(t + 1) == '{') {
							*t2++ = '\377';
							t += 2;
						}
						/* single '{' ? keep */
						else *t2++ = *t++;
					}
				}
				*t2 = 0;
				/* Display and advance */
				Term_putstr(0, i++, -1, TERM_WHITE, buf2);
#else
				/* Display and advance */
				Term_putstr(0, i++, -1, TERM_WHITE, buf);
#endif
			}

			/* Close */
			my_fclose(fp);
		}

		center_string(buf, cname);
		c_put_str(TERM_L_UMBER, buf, 6, STONE_COL);

		center_string(buf, format("the %s %s", race_info[race].title, class_info[class].title));
		c_put_str(TERM_L_UMBER, buf, 7, STONE_COL);

#if 0
		center_string(buf, race_info[race].title);
		c_put_str(TERM_L_UMBER, buf, 8, STONE_COL);

		center_string(buf, class_info[class].title);
		c_put_str(TERM_L_UMBER, buf, 9, STONE_COL);
#endif

#if 0
		(void)sprintf(tmp, "Level: %d", (int)p_ptr->lev);
		center_string_short(buf, tmp);
		c_put_str(TERM_L_UMBER, buf, 11 - 2, STONE_COL_SHORT);

		(void)sprintf(tmp, "Exp: %d", p_ptr->exp);
		center_string_short(buf, tmp);
		c_put_str(TERM_L_UMBER, buf, 12 - 2, STONE_COL_SHORT);
#else
		(void)sprintf(tmp, "Lv: %d, Exp: %d", (int)p_ptr->lev, p_ptr->exp);
		center_string(buf, tmp);
		c_put_str(TERM_L_UMBER, buf, 11 - 2, STONE_COL);
#endif
		/* XXX usually 0 */
		(void)sprintf(tmp, "AU: %d", p_ptr->au);
		center_string_short(buf, tmp);
		c_put_str(TERM_L_UMBER, buf, 12 - 2, STONE_COL_SHORT);

		/* Location */
		if (c_cfg.depth_in_feet)
			(void)sprintf(tmp, "Died on %dft %s", p_ptr->wpos.wz * 50, location_pre);
		else
			(void)sprintf(tmp, "Died on Lv %d %s", p_ptr->wpos.wz, location_pre);
		center_string(buf, tmp);
		c_put_str(TERM_L_UMBER, buf, 13 - 1, STONE_COL);

		if (location_name2[0])
			sprintf(tmp, "%s", location_name2);
		else if (p_ptr->wpos.wx == 127)
			sprintf(tmp, "an unknown location");
		else
			sprintf(tmp, "world map region (%d,%d)", p_ptr->wpos.wx, p_ptr->wpos.wy);
		center_string(buf, tmp);
		c_put_str(TERM_L_UMBER, buf, 14 - 1, STONE_COL);

		/* Time of death */
		(void)sprintf(tmp, "%-.24s", ctime(&ct));
		center_string(buf, tmp);
		c_put_str(TERM_L_UMBER, buf, 17 - 3, STONE_COL);

		/* Death cause */
		strcpy(reason2, reason);
		if (strchr(reason2, '(')) *(strchr(reason2, '(') - 1) = 0; //also eat the space before the '('

		if (strstr(reason2, "Killed by")) {
			/* Monster name too long? Try to extract shorter name from it. */
			if (strlen(reason2) - 10 > 31) {
				char *cp;

				strcpy(buf, reason2);

				/* 1st: Try to end name at a comma, skipping the additional description.
				        "Gothmog, the High Captain of Balrogs" -> "Gothmog". */
				if ((cp = strchr(buf, ','))) *cp = 0;

				/* 2nd: Try to end name at a relative word such as "that", "which" etc.
				        "The disembodied hand that strangled people" -> "The disembodied hand". */
				else if ((cp = strstr(buf, " that "))) *cp = 0;
				else if ((cp = strstr(buf, " who "))) *cp = 0;
				else if ((cp = strstr(buf, " which "))) *cp = 0;
				else if ((cp = strstr(buf, " whose "))) *cp = 0;
				else if ((cp = strstr(buf, " what "))) *cp = 0;

				/* 3rd: Try to end name at lineage descriptions aka " of ".
				        "Angamaite of Umbar" -> "Angamaite" (Note that Angamaite's name is actually not too long, just taken as example here). */
				else if ((cp = strstr(buf, " of "))) *cp = 0;

				strcpy(reason2, buf);
			}
			center_string(buf, "Killed by");
			c_put_str(TERM_L_UMBER, buf, 18 - 3, STONE_COL);
			center_string(buf, reason2 + 10);
			c_put_str(TERM_L_UMBER, buf, 21 - 5, STONE_COL);
		} else if (strstr(reason2, "Committed suicide") || strstr(reason2, "Retired")) {
			if (p_ptr->total_winner) {
				switch (p_ptr->prace) {
				case RACE_VAMPIRE:
				case RACE_MAIA:
				case RACE_HIGH_ELF:
				case RACE_ELF:
					center_string(buf, "Unexplained Disappearance");
					break;
				case RACE_HALF_ELF:
					if (magik(75)) center_string(buf, "Unexplained Disappearance");
					else center_string(buf, "Died from ripe old age");
					break;
				default:
					center_string(buf, "Died from ripe old age");
				}
				c_put_str(TERM_L_UMBER, buf, 19 - 3, STONE_COL);
			} else {
				center_string(buf, "Committed suicide");
				c_put_str(TERM_L_UMBER, buf, 19 - 3, STONE_COL);
			}
		} else {
			center_string(buf, reason2);
			c_put_str(TERM_L_UMBER, buf, 21 - 5, STONE_COL);
		}
	}
}

/*
 * Display some character info	- Jir -
 * For now, only when losing the character.
 */
void c_close_game(cptr reason) {
	int k;
#ifdef RAINY_TOMB
	int x, y;
	byte *scr_aa;
	char32_t *scr_cc;
 #ifdef GRAPHICS_BG_MASK
	byte *scr_aa_back;
	char32_t *scr_cc_back;
 #endif
#endif
	char tmp[MAX_CHARS];
	bool c_cfg_tmp = c_cfg.topline_no_msg;

	/* Let the player view the last scene */
	c_cfg.topline_no_msg = FALSE;
	c_msg_format("%s ...Press '0' key to proceed", reason);

	while (inkey() != '0');
	c_cfg.topline_no_msg = c_cfg_tmp;

	/* You are dead */
	fullscreen_weather = TRUE;
	print_tomb(reason);

	/* Remember deceased char's name if we will just recreate the same.. */
	strcpy(prev_cname, cname);

#if 0
	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	/* Display tomb screen without an input prompt, allow taking a 'clear' screenshot! */
	//while (inkey() == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);
	if (inkey() == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);
#endif

	put_str("ESC to quit, 'f' to dump the record or any other key to proceed", 23, 0);
	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

#ifdef RAINY_TOMB
 #define WEATHER_GEN_TICKS 3
 #define WEATHER_SNOW_MULT 3
	/* Bad weather today? */
	weather_elements = 0;
	if (magik(50)) { //sometimes it rains or snows~ UwU
		weather_panel_x = 0;
		weather_panel_y = 0;
		weather_type = 1 + rand_int(2) + 50;
		weather_wind = rand_int(5);
		if (weather_wind) wind_noticable = TRUE;
		weather_gen_speed = WEATHER_GEN_TICKS;
		weather_intensity = (weather_type % 10 == 2 && !(weather_wind || weather_wind >= 3) ? 5 : 8);
		weather_speed_snow = weather_speed_rain = (weather_type % 10 == 2) ? WEATHER_SNOW_MULT * WEATHER_GEN_TICKS : (weather_wind ? 1 * WEATHER_GEN_TICKS : WEATHER_GEN_TICKS - 1);
		/* Set currently visible screen as 'weather background'. */
		for (y = 0; y < screen_hgt + SCREEN_PAD_TOP + SCREEN_PAD_BOTTOM - 1; y++) {
			scr_aa = Term->scr->a[y + 1]; //+1 : leave first line blank for message prompts
			scr_cc = Term->scr->c[y + 1];
 #ifdef GRAPHICS_BG_MASK
			scr_aa_back = Term->scr_back->a[y + 1]; //+1 : leave first line blank for message prompts
			scr_cc_back = Term->scr_back->c[y + 1];
 #endif
			for (x = 0; x < screen_wid + SCREEN_PAD_LEFT + SCREEN_PAD_RIGHT; x++) {
				panel_map_a[x][y] = scr_aa[x];
				panel_map_c[x][y] = scr_cc[x];
 #ifdef GRAPHICS_BG_MASK
				panel_map_a_back[x][y] = scr_aa_back[x];
				panel_map_c_back[x][y] = scr_cc_back[x];
 #endif
			}
		}

 #ifdef USE_SOUND_2010
		if (use_sound) {
			if (weather_type % 10 == 1) { //rain
				if (weather_wind >= 1 && weather_wind <= 2) sound_weather(rain2_sound_idx);
				else sound_weather(rain1_sound_idx);
			} else if (weather_type % 10 == 2) { //snow
				if (weather_wind >= 1 && weather_wind <= 2) sound_weather(snow2_sound_idx);
				else sound_weather(snow1_sound_idx);
			}
		}
 #endif
	} else weather_type = 0;
#endif
	/* TODO: bandle them in one loop instead of 2 */
	while (1) {
		/* Get command */
		k = inkey();

		/* Exit */
		if (k == ESCAPE || k == KTRL('Q') || k == 'q' || k == 'Q') {
#ifdef RAINY_TOMB
			weather_elements = 0;
			weather_type = 0;
 #if 0 /* not working correctly */
			/* restore display behind weather particles */
			for (k = 0; k < weather_elements; k++) {
				/* only for elements within visible panel screen area */
				if (weather_element_x[k] >= weather_panel_x &&
				    weather_element_x[k] < weather_panel_x + screen_wid &&
				    weather_element_y[k] >= weather_panel_y &&
				    weather_element_y[k] < weather_panel_y + screen_hgt) {
					/* restore original grid content */
#ifdef GRAPHICS_BG_MASK
					if (use_graphics == 2 && !c_cfg.ascii_weather && !c_cfg.no2mask_weather)
						Term_draw_2mask(0 + weather_element_x[k] - weather_panel_x,
						    1 + weather_element_y[k] - weather_panel_y,
						    panel_map_a[weather_element_x[k] - weather_panel_x][weather_element_y[k] - weather_panel_y],
						    panel_map_c[weather_element_x[k] - weather_panel_x][weather_element_y[k] - weather_panel_y],
						    panel_map_a_back[weather_element_x[k] - weather_panel_x][weather_element_y[k] - weather_panel_y],
						    panel_map_c_back[weather_element_x[k] - weather_panel_x][weather_element_y[k] - weather_panel_y]);
					else
#endif
					Term_draw(0 + weather_element_x[k] - weather_panel_x,
					    1 + weather_element_y[k] - weather_panel_y,
					    panel_map_a[weather_element_x[k] - weather_panel_x][weather_element_y[k] - weather_panel_y],
					    panel_map_c[weather_element_x[k] - weather_panel_x][weather_element_y[k] - weather_panel_y]);
				}
			}
			Term_fresh();
 #endif
			for (y = 1; y < screen_hgt + SCREEN_PAD_TOP + SCREEN_PAD_BOTTOM; y++) {
				for (x = 0; x < screen_wid + SCREEN_PAD_LEFT + SCREEN_PAD_RIGHT; x++) {
#ifdef GRAPHICS_BG_MASK
					Term_draw_2mask(x, y,
					    panel_map_a[x][y - 1],
					    panel_map_c[x][y - 1],
					    panel_map_a_back[x][y - 1],
					    panel_map_c_back[x][y - 1]);
#else
					Term_draw(x, y,
					    panel_map_a[x][y - 1],
					    panel_map_c[x][y - 1]);
#endif
				}
			}
 #ifdef USE_SOUND_2010
			if (use_sound) sound_weather(-1); //fade out
 #endif
#endif
			return;
		}

		else if (k == KTRL('T')) {
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", 2);
		}

		/* Dump */
		else if ((k == 'f') || (k == 'F')) {
			strnfmt(tmp, MAX_CHARS - 1, "%s.txt", cname);
			if (get_string("Filename(you can post it to http://angband.oook.cz/): ", tmp, MAX_CHARS - 1)) {
				if (tmp[0] && (tmp[0] != ' ')) {
					file_character(tmp, FALSE);
					break;
				}
			} else {
				/* hack: hide cursor */
				Term->scr->cx = Term->wid;
				Term->scr->cu = 1;
			}
			continue;
		}
		/* Safeguard */
		else if (k == '0') continue;

		else if (k) break;
	}
#ifdef RAINY_TOMB
	weather_elements = 0;
	weather_type = 0;
 #ifdef USE_SOUND_2010
	if (use_sound) sound_weather(-1); //fade out
 #endif
#endif

	/* Interact */
	while (1) {
		/* Clear screen */
		Term_clear();

		/* Why are we here */
		prt("Your tracks", 2, 0);

		/* Give some choices */
		c_put_str(TERM_WHITE, "(\377y1\377w) Character", 4, 5);
		c_put_str(TERM_WHITE, "(\377y2\377w) Inventory", 5, 5);
		c_put_str(TERM_WHITE, "(\377y3\377w) Equipments", 6, 5);
		c_put_str(TERM_WHITE, "(\377y4\377w) Messages", 7, 5);
		c_put_str(TERM_WHITE, "(\377y5\377w) Chat messages", 8, 5);
		c_put_str(TERM_WHITE, "Press a number to review or press \377yESC\377w to continue", 10, 5);

		/* What a pity no score list here :-/ */

		/* Prompt */
		prt("Command: ", 17, 0);

		/* Get command */
		k = inkey();

		/* Exit */
		if (k == ESCAPE || k == KTRL('Q')) break;

		/* Take a screenshot */
		if (k == KTRL('T'))
			xhtml_screenshot("screenshot????", FALSE);

		/* Character screen */
		else if (k == '1' || k == 'C')
			cmd_character();

		/* Inventory */
		else if (k == '2' || k == 'i')
			cmd_inven();

		/* Equipments */
		else if (k == '3' || k == 'e')
			/* Process the running options */
			cmd_equip();

		/* Message history */
		else if (k == '4' || k == KTRL('P'))
			do_cmd_messages();

		/* Chat history */
		else if (k == '5' || k == KTRL('O'))
			do_cmd_messages_important();

#if 0
		/* Skill browsing ... is not available for now */
		else if (k == '6' || k == KTRL('G')) {
			do_cmd_skill();
		}
#endif	/* 0 */

		/* Unknown option */
		else {
			/* Oops */
			bell();
		}
	}

	fullscreen_weather = FALSE;
}



/*
 * Since only GNU libc has memfrob, we use our own.
 */
void my_memfrob(void *s, int n) {
	int i;
	char *str;

	/* Ancient servers don't know this */
	if (server_protocol < 2) return;

	str = (char*) s;

	for (i = 0; i < n; i++)
		/* XOR every byte with 42 */
		str[i] ^= 42;
}

/* dummy */
void msg_format(int Ind, cptr fmt, ...) {
	(void) Ind; /* suppress compiler warning */
	(void) fmt; /* suppress compiler warning */
	return;
}

#ifdef USE_SOUND_2010
bool sound(int val, int type, int vol, s32b player_id, int dist_x, int dist_y) {
	if (!use_sound) return(TRUE);

	/* play a sound */
	if (sound_hook) return(sound_hook(val, type, vol, player_id, dist_x, dist_y));
	else return(FALSE);
}

void sound_weather(int val) {
	if (!use_sound) return;

	/* play a sound */
	if (sound_weather_hook) sound_weather_hook(val);
}

void sound_weather_vol(int val, int vol) {
	if (!use_sound) return;

	/* play a sound */
	if (sound_weather_hook_vol) sound_weather_hook_vol(val, vol);
}

bool music(int val) {
	if (!use_sound) return(TRUE);

	/* play a sound */
	if (music_hook) return(music_hook(val));
	else return(FALSE);
}
bool music_volume(int val, char vol) {
	if (!use_sound) return(TRUE);

	/* play a sound */
	if (music_hook_vol) return(music_hook_vol(val, vol));
	else return(FALSE);
}

void sound_ambient(int val) {
	if (!use_sound) return;

	/* play a sound */
	if (sound_ambient_hook) sound_ambient_hook(val);
}

void set_mixing(void) {
	if (!use_sound) return;

	/* Set the mixer levels */
	if (mixing_hook) mixing_hook();
}

void interact_audio(void) {
	int i, j, item_x[8] = {2, 12, 22, 32, 42, 52, 62, 72}, k, l;
	static int cur_item = 0;
	int y_label = 20, y_toggle = 12, y_slider = 18;
	bool redraw = TRUE, quit = FALSE;

	/* Save screen */
	Term_save();

	/* suppress hybrid macros */
	bool inkey_msg_old = inkey_msg;
	inkey_msg = TRUE;
#if 0 /* moved 'max' from ctrl+x to ctrl+g which should be fine */
	/* suppress even normal macros, for the CTRL+X shortcut for example which is often set to \e\e^X to quit the game in any case.
	   Drawback in case ALLOW_NAVI_KEYS_IN_PROMPT isn't on: Cannot use up/down while numlock is off. */
	inkey_interact_macros = TRUE;
#endif

	/* Process requests until done */
	while (1) {
		if (redraw) {
			/* Clear screen */
			Term_clear();

			/* Describe */
			Term_putstr(30, (l = 0), -1, TERM_L_UMBER, "*** Audio Mixer ***");
			//Term_putstr(3, ++l, -1, TERM_L_UMBER, "Press arrow keys to navigate/modify, RETURN/SPACE to toggle, ESC to leave.");
#if 0 /* enter/space on a slider increase it */
			Term_putstr(1, ++l, -1, TERM_L_UMBER, "Navigate/modify: \377yArrow keys\377U. Toggle/modify: \377yRETURN\377U/\377ySPACE\377U. Reset: \377yr\377U. Exit: \377yESC\377U.");
#else /* enter/space on a slider toggle it */
			Term_putstr(1, ++l, -1, TERM_L_UMBER, "Navigate/modify: \377yArrows\377U/\377yp\377U/\377yn\377U/\377y+\377U/\377y-\377U/\377yg\377U/\377yG\377U/\377yh\377U Toggle: \377yRET\377U/\377ySPACE\377U Reset2cfg: \377yr\377U Exit: \377yESC\377U");
#endif
			Term_putstr(1, ++l, -1, TERM_L_UMBER, "Sfx only/Sfx+wea/Sfx+mus/All: \377yCTRL+S\377U/\377yW\377U/\377yE\377U/\377yA\377U Max/75%/50%/25%/Min: \377yCTRL+G\377U/\377yB\377U/\377yH\377U/\377yL\377U/\377yI");

			//Term_putstr(6, ++l, -1, TERM_L_UMBER, "Shortcuts: 'a': master, 'w': weather, 's': sound, 'c' or 'm': music.");
			//Term_putstr(7, ++l, -1, TERM_L_UMBER, "Jump to volume slider: SHIFT + according shortcut key given above.");
			//Term_putstr(6, ++l, -1, TERM_L_UMBER, "Shortcuts: 'a','w','s','c'/'m'. Shift + shortcut to jump to a slider.");
			//Term_putstr(1, ++l, -1, TERM_L_UMBER, "Shortcuts: \377ya\377U,\377yw\377U,\377ys\377U,\377yc\377U/\377ym\377U. Sliders: \377ySHIFT+shortcut\377U. Reload packs & re-init: \377yCTRL+R\377U.");
			Term_putstr(1, ++l, -1, TERM_L_UMBER, "Shortcuts: \377ya\377U,\377yw\377U,\377ys\377U,\377yc\377U/\377ym\377U   Sliders: \377ySHIFT+a\377U/\377ym\377U/\377ys\377U/\377yw\377U   Reload packs & re-init: \377yCTRL+R\377U ");

			if (quiet_mode) Term_putstr(12, ++l, -1, TERM_L_RED,                              "  Client is running in 'quiet mode': Audio is disabled.  ");
			else if (audio_sfx > 3 && audio_music > 0) Term_putstr(12, ++l, -1, TERM_L_GREEN, "     Sound pack and music pack have been detected.      ");
			//else if (audio_sfx > 3 && audio_music == 0) Term_putstr(12, ++l, -1, TERM_YELLOW, "Sound pack detected. No music pack seems to be installed.");
			else if (audio_sfx > 3 && audio_music == 0) {
				Term_putstr(12, ++l, -1, TERM_L_GREEN, "Sound pack detected.");
				Term_putstr(34, l, -1, TERM_L_RED, "No music pack seems to be installed.");
			}
			//else if (audio_sfx <= 3 && audio_music > 0) Term_putstr(12, 4, -1, TERM_YELLOW, "Music pack detected. No sound pack seems to be installed.");
			else if (audio_sfx <= 3 && audio_music > 0) {
				Term_putstr(12, ++l, -1, TERM_L_GREEN, "Music pack detected.");
				Term_putstr(34, ++l, -1, TERM_L_RED, "No sound pack seems to be installed.");
			}
			else Term_putstr(12, ++l, -1, TERM_L_RED,                                         "Neither sound pack nor music pack seems to be installed. ");

			if (!quiet_mode && noweather_mode) Term_putstr(2, ++l, -1, TERM_L_RED, "Client is in no-weather-mode (-w).");
			else if (c_cfg.no_weather) Term_putstr(2, ++l, -1, TERM_L_RED, "Weather disabled by 'no_weather' option.");

			if (c_cfg.rogue_like_commands)
				//Term_putstr(3, y_label + 2, -1, TERM_SLATE, "Outside of this mixer you can toggle audio and music via CTRL+V and CTRL+X.");
				Term_putstr(3, y_label + 2, -1, TERM_SLATE, "Outside of this mixer you can toggle audio via CTRL+V.");//we needed ctrl+x for ghost powers, out of keys... -_-
			else
				Term_putstr(3, y_label + 2, -1, TERM_SLATE, "Outside of this mixer you can toggle audio and music via CTRL+N and CTRL+C.");

			/* draw mixer */
			Term_putstr(item_x[0], y_toggle, -1, TERM_WHITE, format(" [%s]", cfg_audio_master ? "\377GX\377w" : " "));
			if (c_cfg.rogue_like_commands)
				Term_putstr(item_x[0], y_toggle + 3, -1, TERM_SLATE, "CTRL+V");
			else
				Term_putstr(item_x[0], y_toggle + 3, -1, TERM_SLATE, "CTRL+N");
			Term_putstr(item_x[1], y_toggle, -1, TERM_WHITE, format(" [%s]", cfg_audio_music ? "\377GX\377w" : " "));
			if (c_cfg.rogue_like_commands)
				//Term_putstr(item_x[1], y_toggle + 3, -1, TERM_SLATE, "CTRL+X"); out of keys, we need this for ghost powers -_-
				Term_putstr(item_x[1], y_toggle + 3, -1, TERM_SLATE, "CTRL+C"); //just display the normal-set key, as it actually works inside the mixer in rl-keyset mode too
			else
				Term_putstr(item_x[1], y_toggle + 3, -1, TERM_SLATE, "CTRL+C");
			Term_putstr(item_x[2], y_toggle, -1, TERM_WHITE, format(" [%s]", cfg_audio_sound ? "\377GX\377w" : " "));
			if (!quiet_mode && (noweather_mode || c_cfg.no_weather))
				Term_putstr(item_x[3], y_toggle, -1, TERM_L_RED, format(" [%s]", cfg_audio_weather ? "\377rX\377R" : " "));
			else
				Term_putstr(item_x[3], y_toggle, -1, TERM_WHITE, format(" [%s]", cfg_audio_weather ? "\377GX\377w" : " "));

			for (i = 4; i <= 7; i++) {
				Term_putstr(item_x[i - 4], y_toggle + 1, -1, TERM_SLATE, "on/off");
				Term_putstr(item_x[i], y_slider - 12, -1, TERM_WHITE, "  ^");
				Term_putstr(item_x[i], y_slider, -1, TERM_WHITE, "  V");
				for (j = y_slider - 1; j >= y_slider - 11; j--)
					Term_putstr(item_x[i], j, -1, TERM_SLATE, "  |");
			}
			if (!quiet_mode && (noweather_mode || c_cfg.no_weather)) {
				Term_putstr(item_x[7], y_slider - 12, -1, TERM_L_RED, "  ^");
				Term_putstr(item_x[7], y_slider, -1, TERM_L_RED, "  V");
				for (j = y_slider - 1; j >= y_slider - 11; j--)
					Term_putstr(item_x[7], j, -1, TERM_RED, "  |");
			}

			Term_putstr(item_x[4], y_slider - cfg_audio_master_volume / 10 - 1, -1, TERM_L_GREEN, "  =");
			Term_putstr(item_x[5], y_slider - cfg_audio_music_volume / 10 - 1, -1, TERM_L_GREEN, "  =");
			Term_putstr(item_x[6], y_slider - cfg_audio_sound_volume / 10 - 1, -1, TERM_L_GREEN, "  =");
			if (!quiet_mode && (noweather_mode || c_cfg.no_weather))
				Term_putstr(item_x[7], y_slider - cfg_audio_weather_volume / 10 - 1, -1, TERM_L_RED, "  =");
			else
				Term_putstr(item_x[7], y_slider - cfg_audio_weather_volume / 10 - 1, -1, TERM_L_GREEN, "  =");
		}
		redraw = TRUE;

		/* display editing 'cursor' */
		//Term_putstr(item_x[cur_item], y_label + 2, -1, TERM_ORANGE, "  ^");
		//Term_putstr(item_x[cur_item], y_label + 3, -1, TERM_ORANGE, "  |");
		Term_putstr(item_x[0], y_label, -1, cur_item == 0 ? TERM_ORANGE : TERM_WHITE, "Master");
		Term_putstr(item_x[1], y_label, -1, cur_item == 1 ? TERM_ORANGE : TERM_WHITE, "Music");
		Term_putstr(item_x[2], y_label, -1, cur_item == 2 ? TERM_ORANGE : TERM_WHITE, "Sound");
		Term_putstr(item_x[3], y_label, -1, cur_item == 3 ? TERM_ORANGE : TERM_WHITE, "Weather");
		Term_putstr(item_x[4], y_label, -1, cur_item == 4 ? TERM_ORANGE : TERM_WHITE, "Master");
		Term_putstr(item_x[5], y_label, -1, cur_item == 5 ? TERM_ORANGE : TERM_WHITE, "Music");
		Term_putstr(item_x[6], y_label, -1, cur_item == 6 ? TERM_ORANGE : TERM_WHITE, "Sound");
		Term_putstr(item_x[7], y_label, -1, cur_item == 7 ? TERM_ORANGE : TERM_WHITE, "Weather");


		/* make cursor invisible */
		Term_set_cursor(0);
		inkey_flag = TRUE;

		/* Wait for keypress */
		k = inkey();
		switch (k) {
		case KTRL('A'):
			cfg_audio_master = cfg_audio_music = cfg_audio_sound = cfg_audio_weather = TRUE;
			set_mixing();
			break;
		case KTRL('S'):
			cfg_audio_master = cfg_audio_sound = TRUE;
			cfg_audio_music = cfg_audio_weather = FALSE;
			set_mixing();
			break;
		case KTRL('W'):
			cfg_audio_master = cfg_audio_sound = cfg_audio_weather = TRUE;
			cfg_audio_music = FALSE;
			set_mixing();
			break;
		case KTRL('E'):
			cfg_audio_master = cfg_audio_music = cfg_audio_sound = TRUE;
			cfg_audio_weather = FALSE;
			set_mixing();
			break;
		case KTRL('G'): //case KTRL('X'): <- not good, as this is usually a normal-type macro.
			cfg_audio_master_volume = cfg_audio_music_volume = cfg_audio_sound_volume = cfg_audio_weather_volume = 100;
			set_mixing();
			break;
		case KTRL('H'):
			cfg_audio_master_volume = cfg_audio_music_volume = cfg_audio_sound_volume = cfg_audio_weather_volume = 50;
			set_mixing();
			break;
		case KTRL('L'):
			cfg_audio_master_volume = cfg_audio_music_volume = cfg_audio_sound_volume = cfg_audio_weather_volume = 25;
			set_mixing();
			break;
		case KTRL('B'):
			cfg_audio_master_volume = cfg_audio_music_volume = cfg_audio_sound_volume = cfg_audio_weather_volume = 75;
			set_mixing();
			break;
		case KTRL('I'):
			cfg_audio_master_volume = cfg_audio_music_volume = cfg_audio_sound_volume = cfg_audio_weather_volume = 0;
			set_mixing();
			break;
		/* allow chatting from within here */
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
			//redraw = FALSE; -- header 'mixer' gets overwritten -_-
			break;
		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", 2);
			redraw = FALSE;
			break;
		case KTRL('U'):
		case KTRL('F'): /* <- rogue-like keyset */
		case ESCAPE:
			quit = TRUE; /* hack to leave loop */
			break;
		case 'n':
		case '6':
			//Term_putstr(item_x[cur_item], y_label + 2, -1, TERM_ORANGE, "   ");
			//Term_putstr(item_x[cur_item], y_label + 3, -1, TERM_ORANGE, "   ");
			cur_item++;
			if (cur_item > 7) cur_item = 0;
			redraw = FALSE;
			break;
		case 'p':
		case '4':
			//Term_putstr(item_x[cur_item], y_label + 2, -1, TERM_ORANGE, "   ");
			//Term_putstr(item_x[cur_item], y_label + 3, -1, TERM_ORANGE, "   ");
			cur_item--;
			if (cur_item < 0) cur_item = 7;
			redraw = FALSE;
			break;
		case '8':
		case '+':
			switch (cur_item) {
			case 0:
			case 4: if (cfg_audio_master_volume <= 90) cfg_audio_master_volume += 10; else cfg_audio_master_volume = 100; break;
			case 1:
			case 5: if (cfg_audio_music_volume <= 90) cfg_audio_music_volume += 10; else cfg_audio_music_volume = 100; break;
			case 2:
			case 6: if (cfg_audio_sound_volume <= 90) cfg_audio_sound_volume += 10; else cfg_audio_sound_volume = 100; break;
			case 3:
			case 7: if (cfg_audio_weather_volume <= 90) cfg_audio_weather_volume += 10; else cfg_audio_weather_volume = 100; break;
			}
			set_mixing();
			break;
		case '2':
		case '-':
			switch (cur_item) {
			case 0:
			case 4: if (cfg_audio_master_volume >= 10) cfg_audio_master_volume -= 10; else cfg_audio_master_volume = 0; break;
			case 1:
			case 5: if (cfg_audio_music_volume >= 10) cfg_audio_music_volume -= 10; else cfg_audio_music_volume = 0; break;
			case 2:
			case 6: if (cfg_audio_sound_volume >= 10) cfg_audio_sound_volume -= 10; else cfg_audio_sound_volume = 0; break;
			case 3:
			case 7: if (cfg_audio_weather_volume >= 10) cfg_audio_weather_volume -= 10; else cfg_audio_weather_volume = 0; break;
			}
			set_mixing();
			break;
		case 'g':
			switch (cur_item) {
			case 0:
			case 4: cfg_audio_master_volume = 0; break;
			case 1:
			case 5: cfg_audio_music_volume = 0; break;
			case 2:
			case 6: cfg_audio_sound_volume = 0; break;
			case 3:
			case 7: cfg_audio_weather_volume = 0; break;
			}
			set_mixing();
			break;
		case 'G':
			switch (cur_item) {
			case 0:
			case 4: cfg_audio_master_volume = 100; break;
			case 1:
			case 5: cfg_audio_music_volume = 100; break;
			case 2:
			case 6: cfg_audio_sound_volume = 100; break;
			case 3:
			case 7: cfg_audio_weather_volume = 100; break;
			}
			set_mixing();
			break;
		case 'h':
			switch (cur_item) {
			case 0:
			case 4: cfg_audio_master_volume = 50; break;
			case 1:
			case 5: cfg_audio_music_volume = 50; break;
			case 2:
			case 6: cfg_audio_sound_volume = 50; break;
			case 3:
			case 7: cfg_audio_weather_volume = 50; break;
			}
			set_mixing();
			break;
		case KTRL('N'):
		case KTRL('V'): //rl
		case 'a':
			toggle_master(TRUE);
			break;
		case KTRL('C'):
		//case KTRL('X'): //rl --out of keys, used for ghost powers instead -_-
		case 'c':
		case 'm':
			toggle_music(TRUE);
			break;
		case 's':
			toggle_sound();
			break;
		case 'w':
			toggle_weather();
			break;
		case 'A':
			cur_item = 0 + 4;
			break;
		case 'C':
		case 'M':
			cur_item = 1 + 4;
			break;
		case 'S':
			cur_item = 2 + 4;
			break;
		case 'W':
			cur_item = 3 + 4;
			break;
		case 'r':
#if 0
			/* reset all settings to default */
			cfg_audio_master_volume = cfg_audio_music_volume = cfg_audio_sound_volume = cfg_audio_weather_volume = AUDIO_VOLUME_DEFAULT;
			cfg_audio_master = cfg_audio_music = cfg_audio_sound = cfg_audio_weather = TRUE;
#else
			/* reset all settings to what we loaded from the config file on client startup originally */
			cfg_audio_master = cfg_a;
			cfg_audio_music = cfg_m;
			cfg_audio_sound = cfg_s;
			cfg_audio_weather = cfg_w;
			cfg_audio_master_volume = cfg_va;
			cfg_audio_music_volume = cfg_vm;
			cfg_audio_sound_volume = cfg_vs;
			cfg_audio_weather_volume = cfg_vw;
#endif
			set_mixing();
			break;
		case '\n':
		case '\r':
		case ' ':
			switch (cur_item) {
			case 0: toggle_master(TRUE); break;
			case 1: toggle_music(TRUE); break;
			case 2: toggle_sound(); break;
			case 3: toggle_weather(); break;
#if 0 /* if on a volume slider, increase it */
			case 4: if (cfg_audio_master_volume <= 90) cfg_audio_master_volume += 10; else cfg_audio_master_volume = 0;
				set_mixing();
				break;
			case 5: if (cfg_audio_music_volume <= 90) cfg_audio_music_volume += 10; else cfg_audio_music_volume = 0;
				set_mixing();
				break;
			case 6: if (cfg_audio_sound_volume <= 90) cfg_audio_sound_volume += 10; else cfg_audio_sound_volume = 0;
				set_mixing();
				break;
			case 7: if (cfg_audio_weather_volume <= 90) cfg_audio_weather_volume += 10; else cfg_audio_weather_volume = 0;
				set_mixing();
				break;
#else /* if on a volume slider, toggle it on/off too */
			case 4: toggle_master(TRUE); break;
			case 5: toggle_music(TRUE); break;
			case 6: toggle_sound(); break;
			case 7: toggle_weather(); break;
#endif
			}
			break;
		case KTRL('R'):
			if (re_init_sound() == 0) c_message_add("Audio packs have been reloaded and audio been reinitialized successfully.");
			break;
		default:
			/* Oops */
			bell();
		}

		/* Leave */
		if (quit) break;
	}

	/* Reload screen */
	Term_load();

	/* Flush the queue */
	Flush_queue();

	/* Re-enable hybrid macros */
	inkey_msg = inkey_msg_old;
#if 0
	/* Re-enable normal macros */
	inkey_interact_macros = FALSE;
#endif
}
void toggle_master(bool gui) {
	cfg_audio_master = !cfg_audio_master;
	if (!gui) c_message_add(format("\377yAudio is now %s.", cfg_audio_master ? (quiet_mode ? "ON (but client is running in quiet mode)" : "ON") : "OFF"));
	set_mixing();
}
void toggle_music(bool gui) {
	cfg_audio_music = !cfg_audio_music;
	if (!gui)
		c_message_add(format("\377yMusic is now %s.",
		    cfg_audio_music ?
		    (quiet_mode ? "ON (but client is running in quiet mode)" :
		    (cfg_audio_master ? "ON" : "ON (but master mixer is set to off)"))
		    : "OFF"));
	set_mixing();
}
void toggle_sound(void) {
	cfg_audio_sound = !cfg_audio_sound;
	//c_message_add(format("\377ySound is now %s.", cfg_audio_sound ? "ON" : "OFF"));
	set_mixing();
}
void toggle_weather(void) {
	cfg_audio_weather = !cfg_audio_weather;
	//c_message_add(format("\377yWeather sound effects are now %s.", cfg_audio_weather ? "ON" : "OFF"));
	set_mixing();
}
#endif

#ifdef USE_SOUND_2010
/* Select folders for music/sound pack to load, from a selection of all eligible folders within lib/xtra */
#define MAX_PACKS 100
#define PACKS_SCREEN 10
void audio_pack_selector(void) {
	int k, soundpacks = 0, musicpacks = 0;
	static int cur_sp = 0, cur_mp = 0, cur_sy = 0, cur_my = 0;
	bool redraw = TRUE, quit = FALSE;
	char buf[1024], path[1024];
	char sp_dir[MAX_PACKS][MAX_CHARS], mp_dir[MAX_PACKS][MAX_CHARS];
	char sp_name[MAX_PACKS][MAX_CHARS], mp_name[MAX_PACKS][MAX_CHARS];
	char sp_author[MAX_PACKS][MAX_CHARS], mp_author[MAX_PACKS][MAX_CHARS];
	char sp_diz[MAX_PACKS][MAX_CHARS * 3], mp_diz[MAX_PACKS][MAX_CHARS * 3];
	char sp_version[MAX_PACKS][MAX_CHARS], mp_version[MAX_PACKS][MAX_CHARS];
	char *ckey, *cval;

	bool inkey_msg_old = inkey_msg;

	FILE *fff, *fff2;
#ifndef WINDOWS
	int r;
#endif


	/* Get list of all folders starting on 'music' or 'sound' within lib/xtra */
	fff = fopen("__tomenet.tmp", "w"); //just make sure the file always exists, for easier file-reading handling.. pft */
	if (!fff) {
		c_message_add("\377oError: Couldn't write temporary file (P1).");
		return;
	}
	fclose(fff);
#ifdef WINDOWS
 #if 0 /* use system calls - easy, but has drawback of cmd shell window popup -_- */
	fff = fopen("__tomenethelper.bat", "w");
	if (!fff) {
		c_message_add("\377oError: Couldn't write temporary file (P2).");
		return;
	}
	fprintf(fff, "@dir %s /a:d /b > __tomenet.tmp\n", ANGBAND_DIR_XTRA);
	fclose(fff);
	/* Nasty path-guessing hardcoding here =p. */
	//strcpy(buf, getenv("ComSpec"));
	//_spawnl(_P_WAIT, buf, buf, "/c", "__tomenethelper.bat", NULL);
	_spawnl(_P_WAIT, "__tomenethelper.bat", "__tomenethelper.bat", NULL);
 #else
	{ /* Use native Windows functions from dirent to query directory structure directly */
		struct dirent *de;
		DIR *dr = opendir(ANGBAND_DIR_XTRA);

		if (dr == NULL) {
			c_message_add("\377oError: Couldn't scan audio pack folders.");
			return;
		}

		fff = fopen("__tomenet.tmp", "w"); //o lazy
		while ((de = readdir(dr)) != NULL) fprintf(fff, "%s\n", de->d_name);
		fclose(fff);

		closedir(dr);
	}
 #endif
#else /* assume POSIX */
	r = system(format("ls %s/*/ -d -1 > __tomenet.tmp", ANGBAND_DIR_XTRA)); // or "ls -d */ | cat > __tomenet.tmp" in case -1 isnt supported?
#endif
	fff = fopen("__tomenet.tmp", "r");
	if (!fff) {
		c_message_add("\377oError: Couldn't read temporary file.");
		return;
	}
	while (!feof(fff)) {
		if (!fgets(buf, 1024, fff)) break;
#ifndef WINDOWS
		buf[strlen(buf) - 2] = 0; //'ls' command outputs trailing '/' on each line
#else
		buf[strlen(buf) - 1] = 0; //trailing newline
#endif

#ifndef WINDOWS
		/* crop xtra path */
		strcpy(path, buf + strlen(ANGBAND_DIR_XTRA) + 1);
		strcpy(buf, path);
#endif

		/* Found a sound pack folder? */
		if (!strncmp(buf, "sound", 5)) {
			if (soundpacks < MAX_PACKS) {
				if (!strcmp(buf, cfg_soundpackfolder)) {
					cur_sp = soundpacks; //currently used pack
					cur_sy = cur_sp > PACKS_SCREEN ? PACKS_SCREEN : cur_sp;
				}
				strcpy(sp_dir[soundpacks], buf);
				/* read [Title] metadata */
				strcpy(sp_name[soundpacks], "No name");
				strcpy(sp_author[soundpacks], "nobody");
				strcpy(sp_diz[soundpacks], "No description");
				strcpy(sp_version[soundpacks], "");
#ifdef WINDOWS
				path_build(path, 1024, ANGBAND_DIR_XTRA, format("%s\\sound.cfg", buf));
#else
				path_build(path, 1024, ANGBAND_DIR_XTRA, format("%s/sound.cfg", buf));
#endif
				fff2 = fopen(path, "r");
				if (!fff2) continue;
				while (!feof(fff2)) {
					if (!fgets(buf, 1024, fff2)) break;
					buf[strlen(buf) - 1] = 0;

					/* Trim leading spaces/tabs */
					ckey = buf;
					while (*ckey == ' ' || *ckey == '\t') ckey++;

					/* Search for key separator */
					if (!(cval = strchr(ckey, '='))) continue;
					*cval = 0;
					cval++;
					/* Trim spaces/tabs */
					while (strlen(ckey) && (ckey[strlen(ckey) - 1] == ' ' || ckey[strlen(ckey) - 1] == '\t')) ckey[strlen(ckey) - 1] = 0;
					while (*cval == ' ' || *cval == '\t') cval++;
					while (strlen(cval) && (cval[strlen(cval) - 1] == ' ' || cval[strlen(cval) - 1] == '\t')) cval[strlen(cval) - 1] = 0;

					/* Scan for pack info */
					if (!strcmp(ckey, "packname")) {
						strncpy(sp_name[soundpacks], cval, MAX_CHARS);
						continue;
					}
					if (!strcmp(ckey, "author")) {
						strncpy(sp_author[soundpacks], cval, MAX_CHARS);
						continue;
					}
					if (!strcmp(ckey, "description")) {
						strncpy(sp_diz[soundpacks], cval, MAX_CHARS * 3);
						continue;
					}
					if (!strcmp(ckey, "version")) {
						strncpy(sp_version[soundpacks], cval, MAX_CHARS);
						continue;
					}
				}
				fclose(fff2);
				soundpacks++;
			}
			continue;
		}
		/* Found a music pack folder? */
		if (!strncmp(buf, "music", 5)) {
			if (musicpacks < MAX_PACKS) {
				if (!strcmp(buf, cfg_musicpackfolder)) {
					cur_mp = musicpacks; //currently used pack
					cur_my = cur_mp > PACKS_SCREEN ? PACKS_SCREEN : cur_mp;
				}
				strcpy(mp_dir[musicpacks], buf);
				/* read [Title] metadata */
				strcpy(mp_name[musicpacks], "No name");
				strcpy(mp_author[musicpacks], "nobody");
				strcpy(mp_diz[musicpacks], "No description");
				strcpy(mp_version[musicpacks], "");
#ifdef WINDOWS
				path_build(path, 1024, ANGBAND_DIR_XTRA, format("%s\\music.cfg", buf));
#else
				path_build(path, 1024, ANGBAND_DIR_XTRA, format("%s/music.cfg", buf));
#endif
				fff2 = fopen(path, "r");
				if (!fff2) continue;
				while (!feof(fff2)) {
					if (!fgets(buf, 1024, fff2)) break;
					buf[strlen(buf) - 1] = 0;

					/* Trim leading spaces/tabs */
					ckey = buf;
					while (*ckey == ' ' || *ckey == '\t') ckey++;

					/* Search for key separator */
					if (!(cval = strchr(ckey, '='))) continue;
					*cval = 0;
					cval++;
					/* Trim spaces/tabs */
					while (strlen(ckey) && (ckey[strlen(ckey) - 1] == ' ' || ckey[strlen(ckey) - 1] == '\t')) ckey[strlen(ckey) - 1] = 0;
					while (*cval == ' ' || *cval == '\t') cval++;
					while (strlen(cval) && (cval[strlen(cval) - 1] == ' ' || cval[strlen(cval) - 1] == '\t')) cval[strlen(cval) - 1] = 0;

					/* Scan for pack info */
					if (!strcmp(ckey, "packname")) {
						strncpy(mp_name[musicpacks], cval, MAX_CHARS);
						continue;
					}
					if (!strcmp(ckey, "author")) {
						strncpy(mp_author[musicpacks], cval, MAX_CHARS);
						continue;
					}
					if (!strcmp(ckey, "description")) {
						strncpy(mp_diz[musicpacks], cval, MAX_CHARS * 3);
						continue;
					}
					if (!strcmp(ckey, "version")) {
						strncpy(mp_version[musicpacks], cval, MAX_CHARS);
						continue;
					}
				}
				fclose(fff2);
				musicpacks++;
			}
			continue;
		}
	}
	fclose(fff);
	remove("__tomenet.tmp");
#ifndef WINDOWS
	(void)r; //slay compiler warning -_-;;;
#else
	remove("__tomenethelper.bat");
#endif

	/* Save screen */
	Term_save();

	/* suppress hybrid macros */
	inkey_msg = TRUE;

	while (1) {
		if (redraw) {
			/* Clear screen */
			Term_clear();

			/* Describe */
			Term_putstr(25,  0, -1, TERM_L_UMBER, "*** Audio Pack Selector ***");
			Term_putstr(1, 1, -1, TERM_L_WHITE, "Press \377yq\377w/\377ya\377w to navigate sound packs, \377yw\377w/\377ys\377w to navigate music packs, \377yESC\377w to accept.");
			Term_putstr(5, 3, -1, TERM_L_UMBER, "Available sound packs:");
			Term_putstr(45, 3, -1, TERM_L_UMBER, "Available music packs:");

			for (k = 0; k < PACKS_SCREEN; k++) {
				if (k - cur_sy + cur_sp >= soundpacks) break;
				if (k == cur_sy)
					Term_putstr(0, 4 + k, -1, TERM_SELECTOR, sp_dir[cur_sp + k - cur_sy]);
				else
					Term_putstr(0, 4 + k, -1, TERM_WHITE, sp_dir[cur_sp + k - cur_sy]);
			}
			for (k = 0; k < PACKS_SCREEN; k++) {
				if (k - cur_my + cur_mp >= musicpacks) break;
				if (k == cur_my)
					Term_putstr(40, 4 + k, -1, TERM_SELECTOR, mp_dir[cur_mp + k - cur_my]);
				else
					Term_putstr(40, 4 + k, -1, TERM_WHITE, mp_dir[cur_mp + k - cur_my]);
			}

			Term_putstr(0, 15, -1, TERM_L_UMBER, "Selected SP:");
			Term_putstr(13, 15, -1, TERM_YELLOW, format("%s [by %s%s%s]", sp_name[cur_sp], sp_author[cur_sp], *sp_version[cur_sp] ? ", v" : "", sp_version[cur_sp]));
			Term_putstr(0, 16, -1, TERM_L_WHITE, sp_diz[cur_sp]);
			if (strlen(sp_diz[cur_sp]) >= 80) Term_putstr(0, 17, -1, TERM_L_WHITE, &sp_diz[cur_sp][80]);
			if (strlen(sp_diz[cur_sp]) >= 160) Term_putstr(0, 18, -1, TERM_L_WHITE, &sp_diz[cur_sp][160]);
			Term_putstr(0, 20, -1, TERM_L_UMBER, "Selected MP:");
			Term_putstr(13, 20, -1, TERM_YELLOW, format("%s [by %s%s%s]", mp_name[cur_mp], mp_author[cur_mp], *mp_version[cur_mp] ? ", v" : "", mp_version[cur_mp]));
			Term_putstr(0, 21, -1, TERM_L_WHITE, mp_diz[cur_mp]);
			if (strlen(mp_diz[cur_mp]) >= 80) Term_putstr(0, 22, -1, TERM_L_WHITE, &mp_diz[cur_mp][80]);
			if (strlen(mp_diz[cur_mp]) >= 160) Term_putstr(0, 23, -1, TERM_L_WHITE, &mp_diz[cur_mp][160]);
		}
		redraw = TRUE;

		/* make cursor invisible */
		Term_set_cursor(0);
		inkey_flag = TRUE;

		/* Wait for keypress */
		k = inkey();
		switch (k) {
		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", 2);
			redraw = FALSE;
			break;
		case ESCAPE:
			quit = TRUE; /* hack to leave loop */
			break;
		case 'a':
			if (cur_sp < soundpacks - 1) {
				cur_sp++;
				if (cur_sy < PACKS_SCREEN - 1) cur_sy++;
			}
			//redraw = FALSE;
			break;
		case 'q':
			if (cur_sp) {
				cur_sp--;
				if (cur_sy) cur_sy--;
			}
			//redraw = FALSE;
			break;
		case 's':
			if (cur_mp < musicpacks - 1) {
				cur_mp++;
				if (cur_my < PACKS_SCREEN - 1) cur_my++;
			}
			//redraw = FALSE;
			break;
		case 'w':
			if (cur_mp) {
				cur_mp--;
				if (cur_my) cur_my--;
			}
			//redraw = FALSE;
			break;
		default:
			/* Oops */
			bell();
		}

		/* Leave */
		if (quit) break;
	}

	/* No changes were made? */
	if (!strcmp(cfg_soundpackfolder, sp_dir[cur_sp]) && !strcmp(cfg_musicpackfolder, mp_dir[cur_mp])) {
		/* Reload screen */
		Term_load();

		/* Flush the queue */
		Flush_queue();

		/* Re-enable hybrid macros */
		inkey_msg = inkey_msg_old;

		return;
	}

	/* Sound or music pack were changed - notify, apply and re-init audio system (if SDL): */
	if (strcmp(cfg_soundpackfolder, sp_dir[cur_sp])) {
		c_message_add(format("Switched sound pack to '%s'.", sp_dir[cur_sp]));
		strcpy(cfg_soundpackfolder, sp_dir[cur_sp]);
		strcpy(cfg_soundpack_name, sp_name[cur_sp]);
		strcpy(cfg_soundpack_version, sp_version[cur_sp]);
	}
	if (strcmp(cfg_musicpackfolder, mp_dir[cur_mp])) {
		c_message_add(format("Switched music pack to '%s'.", mp_dir[cur_mp]));
		strcpy(cfg_musicpackfolder, mp_dir[cur_mp]);
		strcpy(cfg_musicpack_name, mp_name[cur_mp]);
		strcpy(cfg_musicpack_version, mp_version[cur_mp]);
	}

#ifdef WINDOWS
	store_audiopackfolders();
#else /* assume POSIX */
	write_mangrc(FALSE, FALSE, TRUE);
#endif

	/* Reload screen */
	Term_load();

	/* Flush the queue */
	Flush_queue();

	/* Re-enable hybrid macros */
	inkey_msg = inkey_msg_old;

	/* Switch it live! */
	if (re_init_sound() == 0) c_message_add("Audio packs have been reloaded and audio been reinitialized successfully.");

	//No longer true (for SDL, our only sound sytem at this point basically):
	//c_message_add("\377RAfter changing audio packs, a game client restart is required!");
}
#endif

/* For pasting monster lore into chat, also usable for item-pasting. - C. Blue
   Important feature: Replaces first ':' by '::' if sending to normal chat. */
void Send_paste_msg(char *msg) {
	/* replace first : by :: in case we're going for normal chat mode */
	char buf[MSG_LEN], *c;

	/* Only needed for non-global chat: If first msg char is a ':' it'd screw up the chat mode tag.
	   Hack for this: Just insert a pointless colour code '\377-' as a separator of the two ':'.. */
	if (chat_mode == CHAT_MODE_PARTY)
		if (msg[0] == ':') Send_msg(format("!:\377-%s", msg));
		else Send_msg(format("!: %s", msg));
	else if (chat_mode == CHAT_MODE_LEVEL)
		if (msg[0] == ':') Send_msg(format("#:\377-%s", msg));
		else Send_msg(format("#: %s", msg));
	else if (chat_mode == CHAT_MODE_GUILD)
		if (msg[0] == ':') Send_msg(format("$:\377-%s", msg));
		else Send_msg(format("$: %s", msg));
	else {
		strcpy(buf, msg);
		if ((c = strchr(msg, ':')) &&
		    *(c + 1) != ':') {
			buf[c - msg + 1] = ':';
			strcpy(&buf[c - msg + 2], c + 1);
		}
		Send_msg(format("%s", buf));
	}
}

/* Check if certain options have an immediate effect when toggled,
   for switching BIG_MAP feature on/off live. - C. Blue */
#define PANEL_X	(SCREEN_PAD_LEFT)
#define PANEL_Y	(SCREEN_PAD_TOP)
void check_immediate_options(int i, bool yes, bool playing) {
#ifdef USE_GCU
	if (!strcmp(ANGBAND_SYS, "gcu")) {
		/* Hack for now: Palette animation seems to cause segfault on login in command-line client */
		if (option_info[i].o_var == &c_cfg.palette_animation) {
			c_cfg.palette_animation = FALSE;
			(*option_info[i].o_var) = FALSE;
			Client_setup.options[i] = FALSE;
		}
		if (option_info[i].o_var == &c_cfg.disable_lightning) {
			c_cfg.disable_lightning = FALSE;
			(*option_info[i].o_var) = FALSE;
			Client_setup.options[i] = FALSE;
		}
		/* terminal will break with "^B" visuals if font_map_solid_walls is on, so disable it always: */
		if (option_info[i].o_var == &c_cfg.font_map_solid_walls) {
			if (playing) c_msg_print("\377yOption 'font_map_solid_walls' is not supported on GCU client."); //playing: otherwise 4x spam on login, like this only 1x
			c_cfg.font_map_solid_walls = FALSE;
			(*option_info[i].o_var) = FALSE;
			Client_setup.options[i] = FALSE;
		}
	}

 #ifndef GLOBAL_BIG_MAP
	/* BIG_MAP is currently not supported in GCU client */
	if (!strcmp(ANGBAND_SYS, "gcu") && option_info[i].o_var == &c_cfg.big_map) {
		c_cfg.big_map = FALSE;
		(*option_info[i].o_var) = FALSE;
		Client_setup.options[i] = FALSE;
		screen_hgt = SCREEN_HGT;
		if (playing) Send_screen_dimensions();
	} else
 #endif
#endif
#if defined(WINDOWS) && defined(USE_LOGFONT)
	if (use_logfont && option_info[i].o_var == &c_cfg.font_map_solid_walls) {
		if (playing) c_msg_print("\377yOption 'font_map_solid_walls' is not supported with logfont.");
		c_cfg.font_map_solid_walls = FALSE;
		(*option_info[i].o_var) = FALSE;
		Client_setup.options[i] = FALSE;
	}
	if (use_logfont && option_info[i].o_var == &c_cfg.solid_bars) {
		if (playing) c_msg_print("\377yOption 'solid_bars' is not supported with logfont.");
		c_cfg.solid_bars = FALSE;
		(*option_info[i].o_var) = FALSE;
		Client_setup.options[i] = FALSE;
	}
#endif
#ifndef GLOBAL_BIG_MAP
	/* Not yet. First, process all the option files before doing this */
	if (option_info[i].o_var == &c_cfg.big_map && global_big_map_hold) return;

	if (option_info[i].o_var == &c_cfg.big_map
	    && is_newer_than(&server_version, 4, 4, 9, 1, 0, 1) /* redundant */
	    && (sflags1 & SFLG1_BIG_MAP)) {
		if (!yes && screen_hgt != SCREEN_HGT) {
			screen_hgt = SCREEN_HGT;
			resize_main_window(CL_WINDOW_WID, CL_WINDOW_HGT);
			/* too early, connection not ready yet? (otherwise done in Input_loop()) */
			if (playing) {
				if (screen_icky) Term_switch(0);
				Term_clear(); /* get rid of map tiles where now status bars go instead */
				if (screen_icky) Term_switch(0);
				Send_screen_dimensions();
			}
		}
		if (yes && screen_hgt <= SCREEN_HGT) {
			screen_hgt = MAX_SCREEN_HGT;
			resize_main_window(CL_WINDOW_WID, CL_WINDOW_HGT);
			/* too early, connection not ready yet? (otherwise done in Input_loop()) */
			if (playing) {
				if (screen_icky) Term_switch(0);
				Term_clear(); /* paranoia ;) */
				if (screen_icky) Term_switch(0);
				Send_screen_dimensions();
			}
		}
	}
#endif

	/* Terminate all weather visuals and sounds via 'no_weather' option? */
	if (!noweather_mode && option_info[i].o_var == &c_cfg.no_weather && c_cfg.no_weather) {
		int i;

		/* restore tiles on display that were overwritten by weather */
		if (screen_icky) Term_switch(0);
		for (i = 0; i < weather_elements; i++) {
			/* only for elements within visible panel screen area */
			if (weather_element_x[i] >= weather_panel_x &&
			    weather_element_x[i] < weather_panel_x + screen_wid &&
			    weather_element_y[i] >= weather_panel_y &&
			    weather_element_y[i] < weather_panel_y + screen_hgt) {
#ifdef GRAPHICS_BG_MASK
				if (use_graphics == 2 && !c_cfg.ascii_weather && !c_cfg.no2mask_weather)
					/* restore original grid content */
					Term_draw_2mask(PANEL_X + weather_element_x[i] - weather_panel_x,
					    PANEL_Y + weather_element_y[i] - weather_panel_y,
					    panel_map_a[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y],
					    panel_map_c[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y],
					    panel_map_a_back[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y],
					    panel_map_c_back[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y]);
				else
#endif
				/* restore original grid content */
				Term_draw(PANEL_X + weather_element_x[i] - weather_panel_x,
				    PANEL_Y + weather_element_y[i] - weather_panel_y,
				    panel_map_a[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y],
				    panel_map_c[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y]);
			}
		}
		if (screen_icky) Term_switch(0);
		/* terminate weather */
		weather_elements = 0;
		weather_type = 0;

#ifdef USE_SOUND_2010
		if (use_sound) sound_weather(-2); //stop
#endif
	}

	/* Hide the cursor again after disabling self-highlighting */
	if (option_info[i].o_var == &c_cfg.hilite_player && !c_cfg.hilite_player) {
		if (screen_icky) Term_switch(0); //should always be icky since we're in = menu..
		Term_set_cursor(0);
		if (screen_icky) Term_switch(0);
	}

#ifdef USE_SOUND_2010
	/* Refresh music when shuffle_music or play_all is toggled */
	if (option_info[i].o_var == &c_cfg.shuffle_music || option_info[i].o_var == &c_cfg.play_all) {
		/* ..but only if we're not already in the process of changing music! */
		if (music_next == -1
		    && in_game /* Not when we're in the account overview screen and start the character creation process */
		    )
			music(-3); //refresh!
	}
#endif

	if ((option_info[i].o_var == &c_cfg.mp_huge_bar ||
	    option_info[i].o_var == &c_cfg.sn_huge_bar ||
	    option_info[i].o_var == &c_cfg.hp_huge_bar ||
	    option_info[i].o_var == &c_cfg.st_huge_bar) ||
	    ((option_info[i].o_var == &c_cfg.solid_bars || option_info[i].o_var == &c_cfg.huge_bars_gfx) &&
	    (c_cfg.mp_huge_bar || c_cfg.sn_huge_bar || c_cfg.hp_huge_bar || c_cfg.st_huge_bar))) {
		if (screen_icky) Term_switch(0);

		clear_huge_bars();

		/* Actually redraw any stun "background bar" first, as it's in the background visually... */
		if (p_ptr->stun) prt_stun(p_ptr->stun);

		/* Reset static vars for hp/sp/mp for drawing huge bars to enforce redrawing */
		prev_huge_cmp = prev_huge_csn = prev_huge_chp = -1;

		/* Avoid div/0 if client just logged in with a character, which also initializes the options and calls us */
		if (p_ptr->mmp) draw_huge_bar(0, &prev_huge_cmp, p_ptr->cmp, &prev_huge_mmp, p_ptr->mmp);
		if (p_ptr->msane) draw_huge_bar(1, &prev_huge_csn, p_ptr->csane, &prev_huge_msn, p_ptr->msane);
		if (p_ptr->mhp) draw_huge_bar(2, &prev_huge_chp, p_ptr->chp, &prev_huge_mhp, p_ptr->mhp);
		if (p_ptr->mst) draw_huge_bar(3, &prev_huge_cst, p_ptr->cst, &prev_huge_mst, p_ptr->mst);

		if (screen_icky) Term_switch(0);
	}
	if (option_info[i].o_var == &c_cfg.sn_huge_bar && c_cfg.sn_huge_bar && is_older_than(&server_version, 4, 8, 1, 3, 0, 0))
		c_message_add("Server version 4.8.1.3.0.0 or higher required for the 'huge sanity bar' feature.");

	if (option_info[i].o_var == &c_cfg.show_newest) redraw_newest(); //show or actually clear the marker
}

/* Helper functions for DONT_CLEAR_TOPLINE_IF_AVOIDABLE - C. Blue */
void prompt_topline(cptr prompt) {
#if 0
 #ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
	/* store prompt in case macro fails at an item prompt etc */
	strncpy(last_prompt, prompt, MSG_LEN);
	last_prompt[MSG_LEN - 1] = 0;
	last_prompt_macro = parse_macro;
	if (!parse_macro)
 #endif
	prt(prompt, 0, 0);
#else
 if (c_cfg.keep_topline) {
	/* store prompt in case macro fails at an item prompt etc */
	strncpy(last_prompt, prompt, MSG_LEN);
	last_prompt[MSG_LEN - 1] = 0;
	last_prompt_macro = parse_macro;
	if (!parse_macro)
		prt(prompt, 0, 0);
 } else
	prt(prompt, 0, 0);
#endif
}
void clear_topline(void) {
#if 0
 #ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
	/* clear stored prompt */
	last_prompt[0] = 0;
	if (!parse_macro || !last_prompt_macro)
 #endif
	prt("", 0, 0);
	//last_prompt_macro = parse_macro; not needed (it's just done in next prompt_topline call that follows)
#else
 if (c_cfg.keep_topline) {
	/* clear stored prompt */
	last_prompt[0] = 0;
	if (!parse_macro || !last_prompt_macro)
		prt("", 0, 0);
 } else
	prt("", 0, 0);
#endif
}
void clear_topline_forced(void) {
	last_prompt[0] = 0;
	prt("", 0, 0);
}
//#ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
/* If a macro failed while waiting for input at topline prompt,
   then restore the prompt, so the user can manually respond. */
void restore_prompt(void) {
	if (last_prompt[0] && parse_macro)
		prt(last_prompt, 0, 0);
	/* clear for next time (paranoia) */
	last_prompt[0] = 0;
}
//#endif

/* Parse a CSS style color code - mikaelh */
u32b parse_color_code(const char *str) {
	unsigned long c = 0xffffffff; /* 0xffffffff signals failure */

	if (str && strlen(str) >= 7) {
		sscanf(str + 1, "%lx", &c);
	}

	return(c);
}

#ifdef USE_GRAPHICS
/* Load the graphics pref file "graphics-{graphic_tiles}.pref" aka
   "Access the "graphic visual" system pref file (if any)". */
static void handle_process_graphics_file(void) {
	char fname[255 + 13 + 1];

	/* Figure out graphics prefs file name to be loaded. */
	sprintf(fname, "graphics-%s.prf", graphic_tiles);
	/* Access the "graphic visual" system pref file (if any). */

	/* During the graphics file parsing the offset needs to be MAX_FONT_CHAR + 1.
	 * The grafics redefinition can use tile indexing from 0 and
	 * there is no need to update graphics files after MAX_FONT_CHAR is changed. */
	char_map_offset = MAX_FONT_CHAR + 1;
	if (process_pref_file(fname) == -1) logprint(format("ERROR: Can't read graphics preferences file: %s\n", fname));
	char_map_offset = 0;

	/* Initialize pseudo-features and pseudo-objects that don't exist in the game world but are just used for graphical tilesets */
	/* --- Init pseudo-objects --- */
	/* Weather particles */
	kidx_po_rain_char = Client_setup.k_char[KIDX_PO_RAIN];
	kidx_po_rain_attr = Client_setup.k_attr[KIDX_PO_RAIN];
	kidx_po_rain_e1_char = Client_setup.k_char[KIDX_PO_RAIN_E1];
	kidx_po_rain_e1_attr = Client_setup.k_attr[KIDX_PO_RAIN_E1];
	kidx_po_rain_e2_char = Client_setup.k_char[KIDX_PO_RAIN_E2];
	kidx_po_rain_e2_attr = Client_setup.k_attr[KIDX_PO_RAIN_E2];
	kidx_po_rain_w1_char = Client_setup.k_char[KIDX_PO_RAIN_W1];
	kidx_po_rain_w1_attr = Client_setup.k_attr[KIDX_PO_RAIN_W1];
	kidx_po_rain_w2_char = Client_setup.k_char[KIDX_PO_RAIN_W2];
	kidx_po_rain_w2_attr = Client_setup.k_attr[KIDX_PO_RAIN_W2];
	kidx_po_snow_char = Client_setup.k_char[KIDX_PO_SNOW];
	kidx_po_snow_attr = Client_setup.k_attr[KIDX_PO_SNOW];
	kidx_po_sand_char = Client_setup.k_char[KIDX_PO_SAND];
	kidx_po_sand_attr = Client_setup.k_attr[KIDX_PO_SAND];
	kidx_po_d10f_tl = Client_setup.k_char[KIDX_PO_D10F_TL];
	kidx_po_d10f_t = Client_setup.k_char[KIDX_PO_D10F_T];
	kidx_po_d10f_tr = Client_setup.k_char[KIDX_PO_D10F_TR];
	kidx_po_d10f_bl = Client_setup.k_char[KIDX_PO_D10F_BL];
	kidx_po_d10f_b = Client_setup.k_char[KIDX_PO_D10F_B];
	kidx_po_d10f_br = Client_setup.k_char[KIDX_PO_D10F_BR];
	/* --- Init pseudo-features --- */
	/* Currently there are no pseudo-features used on client-side here; go board in the casino is handled server-side */
}
#endif

/* Load the default font's or a custom font's pref file aka
    "Access the "visual" system pref file (if any)". */
/* If graphics is enabled, load graphic's pref file (if any). */
#define CUSTOM_FONT_PRF /* enable custom pref files? */
void handle_process_font_file(void) {
	char buf[1024 + 17];
#ifdef CUSTOM_FONT_PRF
	char fname[1024] = { 0 }; //init for GCU-only mode: there is no font name available
	int i;

 #if 1
	/* Delete all font info we currently know */
	//feat info (terrain tiles)
	for (i = 0; i < MAX_F_IDX; i++) {
		Client_setup.f_attr[i] = 0;
		Client_setup.f_char[i] = 0;
	}
	floor_mapping_mod = u32b_char_dict_free(floor_mapping_mod);
	//race info (monsters)
	for (i = 0; i < MAX_R_IDX; i++) {
		Client_setup.r_attr[i] = 0;
		Client_setup.r_char[i] = 0;
	}
	monster_mapping_mod = u32b_char_dict_free(monster_mapping_mod);
	//kind info (objects)
	for (i = 0; i < MAX_K_IDX; i++) {
		Client_setup.k_attr[i] = 0;
		Client_setup.k_char[i] = 0;
	}
	//unknown items (objects)
	for (i = 0; i < TV_MAX; i++) {
		Client_setup.u_attr[i] = 0;
		Client_setup.u_char[i] = 0;
	}
 #endif

	/* Just to be sure. During the font parsing the offset needs to be 0.*/
	char_map_offset = 0; //paranoia

	/* Actually try to load a custom font-xxx.prf file, depending on the main screen font */
 #if defined(WINDOWS) || defined(USE_X11)
	get_term_main_font_name(fname);
 #endif
	if (fname[0]) {
		FILE *fff;
		/* Linux: fname has this format: '5x8' */
		/* Windows: fname has this format: '.\lib\xtra\font\16X24X.FON' */
 #ifdef WINDOWS
		char *p;

		strcpy(buf, fname);
		p = (buf + strlen(buf)) - 1;
		/* Look for last '\' */
		while (*p != '\\' && p > buf) p--;
		/* Found a path\file separator? Cut off file name, discard path */
		if (p > buf) strcpy(fname, p + 1);
		/* Cut off extension even, not necessary, but looks better:
		   'font-custom-5X8.prf' instead of 'font-custom-5X8.FON.prf' */
		if (!strcasecmp(&fname[strlen(fname) - 4], ".FON")) fname[strlen(fname) - 4] = 0;
 #endif
 #if defined(WINDOWS) && defined(USE_LOGFONT)
		if (!use_logfont) /* logfont has no custom mapping, don't accidentally use the one from the currently configured non-logfont-font */
 #endif
		{
			/* Create prf file name from font file name */
			sprintf(buf, "font-custom-%s.prf", fname);
			/* Abuse fname to build the file path */
			path_build(fname, 1024, ANGBAND_DIR_USER, buf);
			fff = my_fopen(fname, "r");
			/* If custom file doesn't exist, fallback to normal font pref file: */
			custom_font_warning = FALSE;
			if (!fff) sprintf(buf, "font-%s.prf", ANGBAND_SYS);
			else {
				fclose(fff);
				if (c_cfg.font_map_solid_walls) custom_font_warning = TRUE;
			}
			process_pref_file(buf);
		}
 #ifdef USE_GRAPHICS
		if (use_graphics) handle_process_graphics_file();
 #endif

		/* Resend definitions to the server */
		if (in_game) Send_client_setup();
	} else {
#endif
		/* Access the "visual" system pref file (if any) */
		sprintf(buf, "font-%s.prf", ANGBAND_SYS);
		process_pref_file(buf);
#ifdef USE_GRAPHICS
		if (use_graphics) handle_process_graphics_file();
#endif
#ifdef CUSTOM_FONT_PRF
	}
#endif
	Send_font();
}

#ifdef RETRY_LOGIN
void clear_macros(void) {
	int i;

	for (i = 0; i < macro__num; i++) {
		string_free(macro__pat[i]);
		macro__pat[i] = NULL;
		string_free(macro__act[i]);
		macro__act[i] = NULL;
		macro__cmd[i] = FALSE;
		macro__hyb[i] = FALSE;
	}
	macro__num = 0;
	for (i = 0; i < 256; i++) macro__use[i] = 0;
}
#endif

/* Colourize 3-digit clusters a bit to make reading big numbers more comfortable. - C. Blue
   method:
   0 - price tags inside stores (white/l-umber)
   1 - AU/XP line in main window (l-green/slate), also Au/Xp in C screen
   2 - like 1 but make it a length 9 number instead of length 10
   afford: can we afford the price? if false, use dark colouring.
 */
#define COLBIGNUM_A1S "\377y"
#define COLBIGNUM_A2S "\377w"
#define COLBIGNUM_B1S "\377W"
#define COLBIGNUM_B2S "\377G"
#define COLBIGNUM_M1S "\377u"
#define COLBIGNUM_M2S "\377U"
#define COLBIGNUM_C1S "\377W"
#define COLBIGNUM_C2S "\377y"

#define COLBIGNUM_DARK_A1S "\377s"
#define COLBIGNUM_DARK_A2S "\377D"
#define COLBIGNUM_DARK_B1S "\377s"
#define COLBIGNUM_DARK_B2S "\377D"
#define COLBIGNUM_DARK_M1S "\377s"
#define COLBIGNUM_DARK_M2S "\377D"

#define FIXED_PY_MAX_XXX
void colour_bignum(s32b bn, s32b bn_max, char *out_val, byte method, bool afford) {
	out_val[0] = 0;

	if (afford) {
		switch (method) {
		case 0:
			/* No billion prices in npc stores but could be in pstores */
			if (bn >= 1000000000) (void)sprintf(out_val, COLBIGNUM_A1S "%d" COLBIGNUM_A2S "%03d" COLBIGNUM_A1S "%03d" COLBIGNUM_A2S "%03d", bn / 1000000000, (bn % 1000000000) / 1000000, (bn % 1000000) / 1000, bn % 1000);
			else if (bn >= 1000000) (void)sprintf(out_val, COLBIGNUM_A2S "%3d" COLBIGNUM_A1S "%03d" COLBIGNUM_A2S "%03d", bn / 1000000, (bn % 1000000) / 1000, bn % 1000);
			else if (bn >= 1000) (void)sprintf(out_val, COLBIGNUM_A1S "%6d" COLBIGNUM_A2S "%03d", bn / 1000, bn % 1000);
			else (void)sprintf(out_val, COLBIGNUM_A2S "%9d", bn);
			break;
		case 1:
			/* Doesn't look thaaat nice colour-wise maybe, despite being easier to interpret :/ */
			if (bn != bn_max) {
				/* Assume max of 4 triplets aka 12-digit number */
				if (bn >= 1000000000) sprintf(out_val, COLBIGNUM_B1S "%1d" COLBIGNUM_B2S "%03d" COLBIGNUM_B1S "%03d" COLBIGNUM_B2S "%03d", bn / 1000000000, (bn % 1000000000) / 1000000, (bn % 1000000) / 1000, bn % 1000);
				else if (bn >= 1000000) sprintf(out_val, COLBIGNUM_B2S "%4d" COLBIGNUM_B1S "%03d" COLBIGNUM_B2S "%03d", bn / 1000000, (bn % 1000000) / 1000, bn % 1000);
				else if (bn >= 1000) sprintf(out_val, COLBIGNUM_B1S "%7d" COLBIGNUM_B2S "%03d", bn / 1000, bn % 1000);
				else sprintf(out_val, COLBIGNUM_B2S "%10d", bn);
			} else {
#ifndef FIXED_PY_MAX_XXX
				/* Alternating umber tones for PY_MAX_GOLD/PY_MAX_EXP */
				sprintf(out_val, COLBIGNUM_M2S "%1d" COLBIGNUM_M1S "%03d" COLBIGNUM_M2S "%03d" COLBIGNUM_M1S "%03d", bn / 1000000000, (bn % 1000000000) / 1000000, (bn % 1000000) / 1000, bn % 1000);
#else
				/* Since they are constants, just display it all in one colour, cannot be misinterpreted */
				sprintf(out_val, COLBIGNUM_M2S "%10d", bn);
#endif
			}
			break;
		case 2:
			if (bn != bn_max) {
				/* Assume max of 4 triplets aka 12-digit number */
				if (bn >= 1000000) sprintf(out_val, COLBIGNUM_B2S "%3d" COLBIGNUM_B1S "%03d" COLBIGNUM_B2S "%03d", bn / 1000000, (bn % 1000000) / 1000, bn % 1000);
				else if (bn >= 1000) sprintf(out_val, COLBIGNUM_B1S "%6d" COLBIGNUM_B2S "%03d", bn / 1000, bn % 1000);
				else sprintf(out_val, COLBIGNUM_B2S "%9d", bn);
			} else {
#ifndef FIXED_PY_MAX_XXX
				/* Alternating umber tones for PY_MAX_GOLD/PY_MAX_EXP */
				sprintf(out_val, COLBIGNUM_M1S "%3d" COLBIGNUM_M2S "%03d" COLBIGNUM_M1S "%03d", bn / 1000000, (bn % 1000000) / 1000, bn % 1000);
#else
				/* Since they are constants, just display it all in one colour, cannot be misinterpreted */
				sprintf(out_val, COLBIGNUM_M2S "%9d", bn);
#endif
			}
			break;
		case 3:
			/* Like 1, but for drained values */
			if (bn != bn_max) {
				/* Assume max of 4 triplets aka 12-digit number */
				if (bn >= 1000000000) sprintf(out_val, COLBIGNUM_C1S "%1d" COLBIGNUM_C2S "%03d" COLBIGNUM_C1S "%03d" COLBIGNUM_C2S "%03d", bn / 1000000000, (bn % 1000000000) / 1000000, (bn % 1000000) / 1000, bn % 1000);
				else if (bn >= 1000000) sprintf(out_val, COLBIGNUM_C2S "%4d" COLBIGNUM_C1S "%03d" COLBIGNUM_C2S "%03d", bn / 1000000, (bn % 1000000) / 1000, bn % 1000);
				else if (bn >= 1000) sprintf(out_val, COLBIGNUM_C1S "%7d" COLBIGNUM_C2S "%03d", bn / 1000, bn % 1000);
				else sprintf(out_val, COLBIGNUM_C2S "%10d", bn);
			} else {
#ifndef FIXED_PY_MAX_XXX
				/* Alternating umber tones for PY_MAX_GOLD/PY_MAX_EXP */
				sprintf(out_val, COLBIGNUM_M2S "%1d" COLBIGNUM_M1S "%03d" COLBIGNUM_M2S "%03d" COLBIGNUM_M1S "%03d", bn / 1000000000, (bn % 1000000000) / 1000000, (bn % 1000000) / 1000, bn % 1000);
#else
				/* Since they are constants, just display it all in one colour, cannot be misinterpreted */
				sprintf(out_val, COLBIGNUM_M2S "%10d", bn);
#endif
			}
			break;
		}
	} else {
		switch (method) {
		case 0:
			/* No billion prices in npc stores but could be in pstores */
			if (bn >= 1000000000) (void)sprintf(out_val, COLBIGNUM_DARK_A1S "%d" COLBIGNUM_DARK_A2S "%03d" COLBIGNUM_DARK_A1S "%03d" COLBIGNUM_DARK_A2S "%03d", bn / 1000000000, (bn % 1000000000) / 1000000, (bn % 1000000) / 1000, bn % 1000);
			else if (bn >= 1000000) (void)sprintf(out_val, COLBIGNUM_DARK_A2S "%3d" COLBIGNUM_DARK_A1S "%03d" COLBIGNUM_DARK_A2S "%03d", bn / 1000000, (bn % 1000000) / 1000, bn % 1000);
			else if (bn >= 1000) (void)sprintf(out_val, COLBIGNUM_DARK_A1S "%6d" COLBIGNUM_DARK_A2S "%03d", bn / 1000, bn % 1000);
			else (void)sprintf(out_val, COLBIGNUM_DARK_A2S "%9d", bn);
			break;
		case 1:
			/* Doesn't look thaaat nice colour-wise maybe, despite being easier to interpret :/ */
			if (bn != bn_max) {
				/* Assume max of 4 triplets aka 12-digit number */
				if (bn >= 1000000000) sprintf(out_val, COLBIGNUM_DARK_B1S "%1d" COLBIGNUM_DARK_B2S "%03d" COLBIGNUM_DARK_B1S "%03d" COLBIGNUM_DARK_B2S "%03d", bn / 1000000000, (bn % 1000000000) / 1000000, (bn % 1000000) / 1000, bn % 1000);
				else if (bn >= 1000000) sprintf(out_val, COLBIGNUM_DARK_B2S "%4d" COLBIGNUM_DARK_B1S "%03d" COLBIGNUM_DARK_B2S "%03d", bn / 1000000, (bn % 1000000) / 1000, bn % 1000);
				else if (bn >= 1000) sprintf(out_val, COLBIGNUM_DARK_B1S "%7d" COLBIGNUM_DARK_B2S "%03d", bn / 1000, bn % 1000);
				else sprintf(out_val, COLBIGNUM_DARK_B2S "%10d", bn);
			} else {
#ifndef FIXED_PY_MAX_XXX
				/* Alternating umber tones for PY_MAX_GOLD/PY_MAX_EXP */
				sprintf(out_val, COLBIGNUM_DARK_M2S "%1d" COLBIGNUM_DARK_M1S "%03d" COLBIGNUM_DARK_M2S "%03d" COLBIGNUM_DARK_M1S "%03d", bn / 1000000000, (bn % 1000000000) / 1000000, (bn % 1000000) / 1000, bn % 1000);
#else
				/* Since they are constants, just display it all in one colour, cannot be misinterpreted */
				sprintf(out_val, COLBIGNUM_DARK_M2S "%10d", bn);
#endif
			}
			break;
		case 2:
			if (bn != bn_max) {
				/* Assume max of 4 triplets aka 12-digit number */
				if (bn >= 1000000) sprintf(out_val, COLBIGNUM_DARK_B2S "%3d" COLBIGNUM_DARK_B1S "%03d" COLBIGNUM_DARK_B2S "%03d", bn / 1000000, (bn % 1000000) / 1000, bn % 1000);
				else if (bn >= 1000) sprintf(out_val, COLBIGNUM_DARK_B1S "%6d" COLBIGNUM_DARK_B2S "%03d", bn / 1000, bn % 1000);
				else sprintf(out_val, COLBIGNUM_DARK_B2S "%9d", bn);
			} else {
#ifndef FIXED_PY_MAX_XXX
				/* Alternating umber tones for PY_MAX_GOLD/PY_MAX_EXP */
				sprintf(out_val, COLBIGNUM_DARK_M1S "%3d" COLBIGNUM_DARK_M2S "%03d" COLBIGNUM_DARK_M1S "%03d", bn / 1000000, (bn % 1000000) / 1000, bn % 1000);
#else
				/* Since they are constants, just display it all in one colour, cannot be misinterpreted */
				sprintf(out_val, COLBIGNUM_DARK_M2S "%9d", bn);
#endif
			}
			break;
		}
	}

}

/* key codes: ^__XXXX\r and ^_XXXX\r */
char key_map_dos_unix[][3][10] = {
	{"FFBE", "F1", ""},
	{"FFBF", "F2", ""},
	{"FFC0", "F3", ""},
	{"FFC1", "F4", ""},
	{"FFC2", "F5", ""},
	{"FFC3", "F6", ""},
	{"FFC4", "F7", ""},
	{"FFC5", "F8", ""},
	{"FFC6", "F9", ""},
	{"FFC7", "F10", ""},
	{"FFC8", "F11", ""},
	{"FFC9", "F12", ""},

	{"FF61", "PrtScr", ""},
	{"FF14", "Scroll", ""},
	{"FF13", "Pause", ""},

	/* Arrow keys et al, also number pad at numlock off */
	{"FF52", "Up", ""},
	{"FF54", "Down", ""},
	{"FF51", "Left", ""},
	{"FF53", "Right", ""},
	{"FF63", "Ins", ""}, // 'Del' is ctrl+b!
	{"FF55", "PageUp", ""},
	{"FF56", "PageDn", ""},
	{"FF50", "Home", ""},
	{"FF57", "End", ""},

	/* Number pad (Numlock independant) */
	{"FFAF", "Num/", ""},
	{"FFAA", "Num*", ""},
	{"FFAD", "Num-", ""},
	{"FFAB", "Num+", ""},
	{"FF8D", "NumRet", ""},

	/* Number pad (Numlock off) */
	{"FF9F", "num.", ""},
	{"FF9E", "num0", ""},
	{"FF9C", "num1", ""},
	{"FF99", "num2", ""},
	{"FF9B", "num3", ""},
	{"FF96", "num4", ""},
	{"FF9D", "num5", ""},
	{"FF98", "num6", ""},
	{"FF95", "num7", ""},
	{"FF97", "num8", ""},
	{"FF9A", "num9", ""},

	/* Number pad (Numlock off) */
	{"FFAC", "NUM.", ""},
	{"FFB0", "NUM0", ""},
	{"FFB1", "NUM1", ""},
	{"FFB2", "NUM2", ""},
	{"FFB3", "NUM3", ""},
	{"FFB4", "NUM4", ""},
	{"FFB5", "NUM5", ""},
	{"FFB6", "NUM6", ""},
	{"FFB7", "NUM7", ""},
	{"FFB8", "NUM8", ""},
	{"FFB9", "NUM9", ""},

	/* Number pad duplicate, apparently numbers only, no idea.. */
	{"FFD8", "Num7", ""},
	{"FFD9", "Num8", ""},
	{"FFDA", "Num9", ""},
	{"FFDB", "Num4", ""},
	{"FFDC", "Num5", ""},
	{"FFDD", "Num6", ""},
	{"FFDE", "Num1", ""},
	{"FFDF", "Num2", ""},
	{"FFE0", "Num3", ""},

	{"x3B", "F1", ""},
	{"x3C", "F2", ""},
	{"x3D", "F3", ""},
	{"x3E", "F4", ""},
	{"x3F", "F5", ""},
	{"x40", "F6", ""},
	{"x41", "F7", ""},
	{"x42", "F8", ""},
	{"x43", "F9", ""},
	{"x44", "F10", ""},
	{"x57", "F11", ""},
	{"x58", "F12", ""},

	//? 	{"x61", "PrtScr", ""},
	//? 	{"x14", "Scroll", ""},
	//? 	{"x13", "Pause", ""},

	/* Arrow keys et al, also number pad at numlock off */
	{"x48", "Up", ""},
	{"x50", "Down", ""},
	{"x4A", "num-", ""},
	{"x4B", "Left", ""},
	{"x4C", "num5", ""},
	{"x4D", "Right", ""},
	{"x4E", "num+", ""},
	{"x52", "Ins", ""},
	{"x53", "Del", ""},
	{"x49", "PageUp", ""},
	{"x51", "PageDn", ""},
	{"x47", "Home", ""},
	{"x4F", "End", ""},

	/* Number pad (Numlock off) */
	/* -- duplicates of the normal keyboard key block listed above, while numlock is off! --
	{"x53", "num.", ""},
	{"x52", "num0", ""},
	{"x4F", "num1", ""},
	{"x50", "num2", ""},
	{"x51", "num3", ""},
	{"x4B", "num4", ""},
	//(unusable with numlock off) 	{"x", "num5", ""},
	{"x4D", "num6", ""},
	{"x47", "num7", ""},
	{"x48", "num8", ""},
	{"x49", "num9", ""}, */

	/* Number pad (Numlock on)
	   - same as the normal keyboard keys with corresponding symbols! - */

	/* Strange codes left over.. looking like numpad */
	{"x77", "Num7", ""},
	{"x8D", "Num8", ""},
	{"x84", "Num9", ""},
	{"x8E", "Num-", ""},
	{"x73", "Num4", ""},
	{"x8F", "Num5", ""},
	{"x74", "Num6", ""},
	{"x90", "Num+", ""},
	{"x75", "Num1", ""},
	{"x91", "Num2", ""},
	{"x76", "Num3", ""},
	{"x92", "Num0", ""},
	{"x93", "Num.", ""},
	//} else if (bptr[0] == '_') {

	/* Linux keycode */
	//if (bptr[0] == 'F' && bptr[1] == 'F') {
	{"FFBE", "F1", ""},
	{"FFBF", "F2", ""},
	{"FFC0", "F3", ""},
	{"FFC1", "F4", ""},
	{"FFC2", "F5", ""},
	{"FFC3", "F6", ""},
	{"FFC4", "F7", ""},
	{"FFC5", "F8", ""},
	{"FFC6", "F9", ""},
	{"FFC7", "F10", ""},
	{"FFC8", "F11", ""},
	{"FFC9", "F12", ""},

	{"FF61", "PrtScr", ""},
	{"FF14", "Scroll", ""},
	{"FF13", "Pause", ""},

	/* Arrow keys et al, also number pad at numlock off */
	{"FF52", "Up", ""},
	{"FF54", "Down", ""},
	{"FF51", "Left", ""},
	{"FF53", "Right", ""},
	{"FF63", "Ins", ""}, // 'Del' is ctrl+b!
	{"FF55", "PageUp", ""},
	{"FF56", "PageDn", ""},
	{"FF50", "Home", ""},
	{"FF57", "End", ""},

	/* Number pad (Numlock independant) */
	{"FFAF", "Num/", ""},
	{"FFAA", "Num*", ""},
	{"FFAD", "Num-", ""},
	{"FFAB", "Num+", ""},
	{"FF8D", "NumRet", ""},

	/* Number pad (Numlock off) */
	{"FF9F", "num.", ""},
	{"FF9E", "num0", ""},
	{"FF9C", "num1", ""},
	{"FF99", "num2", ""},
	{"FF9B", "num3", ""},
	{"FF96", "num4", ""},
	{"FF9D", "num5", ""},
	{"FF98", "num6", ""},
	{"FF95", "num7", ""},
	{"FF97", "num8", ""},
	{"FF9A", "num9", ""},

	/* Number pad (Numlock off) */
	{"FFAC", "NUM.", ""},
	{"FFB0", "NUM0", ""},
	{"FFB1", "NUM1", ""},
	{"FFB2", "NUM2", ""},
	{"FFB3", "NUM3", ""},
	{"FFB4", "NUM4", ""},
	{"FFB5", "NUM5", ""},
	{"FFB6", "NUM6", ""},
	{"FFB7", "NUM7", ""},
	{"FFB8", "NUM8", ""},
	{"FFB9", "NUM9", ""},

	/* Number pad duplicate, apparently numbers only, no idea.. */
	{"FFD8", "Num7", ""},
	{"FFD9", "Num8", ""},
	{"FFDA", "Num9", ""},
	{"FFDB", "Num4", ""},
	{"FFDC", "Num5", ""},
	{"FFDD", "Num6", ""},
	{"FFDE", "Num1", ""},
	{"FFDF", "Num2", ""},
	{"FFE0", "Num3", ""},
	//} else if (bptr[0] == '_') {
};

/* -1: toggle, 0 = disable, 1 = enable */
void set_bigmap(int bm, bool verbose) {
	/* BIG_MAP is currently not supported in GCU client */
	if (!strcmp(ANGBAND_SYS, "gcu")) {
		c_message_add("\377ySorry, big_map is not available in the GCU (command-line) client.");
		return;
	}

	switch (bm) {
	case -1:
		global_c_cfg_big_map = !global_c_cfg_big_map;
		if (verbose) {
			if (global_c_cfg_big_map) c_message_add("Toggled big_map mode to 'enabled'.");
			else c_message_add("Toggled big_map mode to 'disabled'.");
		}
		break;
	case 1:
		if (global_c_cfg_big_map) {
			c_message_add("The screen is already in big_map mode.");
			return;
		}
		if (verbose) c_message_add("Set big_map mode to 'enabled'.");
		global_c_cfg_big_map = TRUE;
		break;
	case 0:
		if (!global_c_cfg_big_map) {
			c_message_add("The screen is already not in big_map mode.");
			return;
		}
		if (verbose) c_message_add("Set big_map mode to 'disabled'.");
		global_c_cfg_big_map = FALSE;
		break;
	}

	if (is_newer_than(&server_version, 4, 4, 9, 1, 0, 1) /* redundant */
	    && (sflags1 & SFLG1_BIG_MAP)) {
		if (!global_c_cfg_big_map && screen_hgt != SCREEN_HGT) {
			screen_hgt = SCREEN_HGT;
			resize_main_window(CL_WINDOW_WID, CL_WINDOW_HGT);
			if (screen_icky) Term_switch(0);
			Term_clear(); /* get rid of map tiles where now status bars go instead */
			if (screen_icky) Term_switch(0);
			Send_screen_dimensions();
		}
		if (global_c_cfg_big_map && screen_hgt <= SCREEN_HGT) {
			screen_hgt = MAX_SCREEN_HGT;
			resize_main_window(CL_WINDOW_WID, CL_WINDOW_HGT);
			if (screen_icky) Term_switch(0);
			Term_clear(); /* paranoia ;) */
			if (screen_icky) Term_switch(0);
			Send_screen_dimensions();
		}
	}
}

/* Client-side version of check_guard_inscription(), without use of quarks:
 * look for "!*Erm" type, and "!* !A !f" type.
 * New (2023): Encode TRUE directly as -1 instead, and if TRUE and there's a number behind
 *             the inscription still within this same !-'segment', return that number + 1 (to encode a value of 0 too),
 *             just negative values aren't possible as -1 would interfere with 'FALSE'. -C. Blue
 *             Added this for !M and !G handling.
 * Returns <-1> if TRUE, <0> if FALSE, >0 if TRUE and a number is specified, with return value being <number+1>. */
int check_guard_inscription_str(cptr ax, char what) {
	int n = 0; //paranoia initialization

	if (ax == NULL) return(FALSE);

	while ((ax = strchr(ax, '!')) != NULL) {
		while (++ax) {
			if (*ax == 0) return(FALSE); /* end of quark, exit */
			if (*ax == ' ' || *ax == '@' || *ax == '#' || *ax == '-') break; /* end of segment, stop */
			if (*ax == what) { /* exact match, accept */
				/* Additionally scan for any 'amount' in case this inscription uses one */
				while (*(++ax)) {
					if (*ax == ' ' || *ax == '@' || *ax == '#' || *ax == '-') return(-1); /* end of segment, accepted, so exit */
					/* Check for number (Note: Evaluate atoi first, in case it's a number != 0 but with leading '0'. -0 and +0 will also be caught fine as simply 0.) */
					if ((n = atoi(ax)) || *ax == '0') return(n + 1); /* '+1' hack: Allow specifying '0' too, still distinguishing it from pure inscription w/o a number specified. */
				}
				return(-1); /* end of quark, exit */
			}
			/* '!*' special combo inscription */
			if (*ax == '*') {
				/* why so much hassle? * = all, that's it */
				/*return(TRUE); -- well, !'B'ash if it's on the ground sucks ;) */

				switch (what) { /* check for paranoid tags */
				case 'd': /* no drop */
				case 'h': /* (obsolete) no house ( sell a a key ) */
				case 'k': /* no destroy */
				case 's': /* no sell */
				case 'v': /* no thowing */
				case '=': /* force pickup */
#if 0
				case 'w': /* no wear/wield */
				case 't': /* no take off */
#endif
					return(-1);
				}
				//return(FALSE);
			}
			/* '!+' special combo inscription */
			if (*ax == '+') {
				/* why so much hassle? * = all, that's it */
				/*return(TRUE); -- well, !'B'ash if it's on the ground sucks ;) */

				switch (what) { /* check for paranoid tags */
				case 'h': /* (obsolete) no house ( sell a a key ) */
				case 'k': /* no destroy */
				case 's': /* no sell */
				case '=': /* force pickup */
#if 0
				case 'w': /* no wear/wield */
				case 't': /* no take off */
#endif
					return(-1);
				}
				//return(FALSE);
			}
		}
	}
	return(FALSE);
}

#include <sys/stat.h>
/* Specifically checks if a file exists and it is a file and not a folder - C. Blue */
bool my_fexists(const char *fname) {
	int fd;
	struct stat statbuf;

	fd = open(fname, O_RDONLY);
	if (fd == -1) return(FALSE);
	if (fstat(fd, &statbuf)) {
		close(fd);
		return(FALSE);
	}
	close(fd);
	switch (statbuf.st_mode & S_IFMT) {
	case S_IFDIR: return(FALSE);
	case S_IFREG: return(TRUE); //Note: This also works fine with symlinks
	default: return(FALSE);
	}
}
