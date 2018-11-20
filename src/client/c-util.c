/* $Id$ */
/* File: c-util.c */
/* Client-side utility stuff */

#include "angband.h"

#include <sys/time.h>

#define ENABLE_SUBWINDOW_MENU /* allow =f menu function for setting fonts/visibility of term windows */
//#ifdef ENABLE_SUBWINDOW_MENU
 #include <dirent.h> /* we now need it for scanning for audio packs too */
//#endif

#define MACRO_USE_CMD	0x01
#define MACRO_USE_STD	0x02
#define MACRO_USE_HYB	0x04

#define NR_OPTIONS_SHOWN	8 /* apparently # of term windows (old comment: was 32 when there were 32 window_flag_desc[]) */

/* Have the Macro Wizard generate target code in
   the form *tXXX- instead of XXX*t? - C. Blue */
#define MACRO_WIZARD_SMART_TARGET

/* This should be extended onto all top-line clearing/message prompting during macro execution,
   to avoid erasing %:bla lines coming from the macro, by overwriting them with useless prompts: */
//#define DONT_CLEAR_TOPLINE_IF_AVOIDABLE -- turned into an option instead: c_cfg.keep_topline



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
		if (errno != EINTR) return -1;
	}

	/* Success */
	return 0;
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

static void ascii_to_text(char *buf, cptr str);

static bool after_macro = FALSE;
bool parse_macro = FALSE; /* are we inside the process of executing a macro */
bool inkey_sleep = FALSE, inkey_sleep_semaphore = FALSE;
int macro_missing_item = 0;
static bool parse_under = FALSE;
static bool parse_slash = FALSE;
static bool strip_chars = FALSE;

static bool flush_later = FALSE;

static byte macro__use[256];

static bool was_chat_buffer = FALSE;
static bool was_all_buffer = FALSE;
static bool was_important_scrollback = FALSE;

static char octify(uint i) {
	return (hexsym[i%8]);
}

static char hexify(uint i) {
	return (hexsym[i%16]);
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
if (c_cfg.keep_topline)
	restore_prompt();
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
		if (macro__hyb[i] && (shopping || inkey_msg)) continue;
		if (macro__cmd[i] && (shopping || !inkey_flag || inkey_msg)) continue;

		/* Check for "prefix" */
		if (prefix(macro__pat[i], buf))
		{
			/* Ignore complete macros */
			if (!streq(macro__pat[i], buf)) return (i);
		}
	}

	/* No matches */
	return (-1);
}


/*
 * Find the longest completed macro
 */
static int macro_ready(cptr buf) {
	int i, t, n = -1, s = -1;

	/* Scan the macros */
	for (i = 0; i < macro__num; i++) {
		/* Skip inactive macros */
		if (macro__cmd[i] && (shopping || inkey_msg || !inkey_flag)) continue;
		if (macro__hyb[i] && (shopping || inkey_msg)) continue;

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
	return (n);
}


/*
 * Hack -- add a macro definition (or redefinition).
 *
 * If "cmd_flag" is set then this macro is only active when
 * the user is being asked for a command (see below).
 */
void macro_add(cptr pat, cptr act, bool cmd_flag, bool hyb_flag)
{
        int n;

        /* Paranoia -- require data */
        if (!pat || !act) return;


        /* Look for a re-usable slot */
        for (n = 0; n < macro__num; n++)
        {
                /* Notice macro redefinition */
                if (streq(macro__pat[n], pat))
                {
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

	if (num == -1) return FALSE;

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

	return TRUE;
}

/* Returns the difference between two timevals in milliseconds */
static int diff_ms(struct timeval *begin, struct timeval *end) {
	int diff;

	diff = (end->tv_sec - begin->tv_sec) * 1000;
	diff += (end->tv_usec - begin->tv_usec) / 1000;

	return diff;
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
	cptr	pat, act;
	char	buf[1024];
	char	buf_atoi[3];
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

		if (parse_macro && (ch == MACRO_WAIT)) {
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
		}
#else
		do {
			do_flicker();
			(void)(Term_inkey(&ch, FALSE, TRUE));
			if (ch) break;
			update_ticks();

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

			if (parse_macro && (ch == MACRO_WAIT)) {
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
			}

			/* If we got a key, break */
			if (ch) break;

			/* If we received a 'max_line' value from the net,
			   break if inkey_max_line flag was set */
			if (inkey_max_line != inkey_max_line_set) {
				/* hack: */
				ch = 1;
				break;
			}

			/* Update our timer and if neccecary send a keepalive packet
			 */
			update_ticks();
			if(!c_quit) {
				do_keepalive();
				do_ping();
			}

			/* Flush the network output buffer */
			Net_flush();

			/* Wait for .001 sec, or until there is net input */
//			SetTimeout(0, 1000);

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
			if (shopping && leave_store) return ESCAPE;

			/* Are we supposed to abort a 'special input request'?
			   (Ie outside game situation has changed, eg left store.) */
			if (request_pending && request_abort) {
				request_abort = FALSE;
				return ESCAPE;
			}

			/* Redraw windows if necessary */
			window_stuff();
		} while (!ch);
	}

	/* End of internal macro */
	if (ch == 29) {
//#ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
if (c_cfg.keep_topline)
		restore_prompt();
//#endif
		parse_macro = FALSE;
	}


	/* Do not check "ascii 28" */
	if (ch == 28) return (ch);


	/* Do not check "ascii 29" */
	if (ch == 29) return (ch);

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
		return (ch);
	}

	/* Do not check "control-underscore" sequences */
	if (parse_under) return (ch);

	/* Do not check "control-backslash" sequences */
	if (parse_slash) return (ch);


	/* Efficiency -- Ignore impossible macros */
	if (!macro__use[(byte)(ch)]) return (ch);

	/* Efficiency -- Ignore inactive macros */
	if (((shopping || !inkey_flag || inkey_msg) && (macro__use[(byte)(ch)] == MACRO_USE_CMD)) || inkey_interact_macros) return (ch);
	if (((shopping || inkey_msg) && (macro__use[(byte)(ch)] == MACRO_USE_HYB)) || inkey_interact_macros) return (ch);


	/* Save the first key, advance */
	buf[p++] = ch;
	buf[p] = '\0';

	/* Wait for a macro, or a timeout */
	while (TRUE)
	{
		/* Check for possible macros */
		k = macro_maybe(buf, k);

		/* Nothing matches */
		if (k < 0) break;

		/* Check for (and remove) a pending key */
		if (0 == Term_inkey(&ch, FALSE, TRUE))
		{
			/* Append the key */
			buf[p++] = ch;
			buf[p] = '\0';

			/* Restart wait */
			w = 0;
		}

		/* No key ready */
		else
		{
			if (multi_key_macros)
			{
				/* Increase "wait" */
				w += 10;

				/* Excessive delay */
				if (w >= 100) break;

				/* Delay */
				Term_xtra(TERM_XTRA_DELAY, w);
			}
			else
			{
				/* No waiting */
				break;
			}
		}
	}

	/* Check for a successful macro */
	k = macro_ready(buf);

	/* No macro available */
	if (k < 0)
	{
		/* Push all the keys back on the queue */
		while (p > 0)
		{
			/* Push the key, notice over-flow */
			if (Term_key_push(buf[--p])) return (0);
		}

		/* Wait for (and remove) a pending key */
		(void)Term_inkey(&ch, TRUE, TRUE);

		/* Return the key */
		return (ch);
	}


	/* Access the macro pattern */
	pat = macro__pat[k];

	/* Get the length of the pattern */
	n = strlen(pat);


	/* Push the "extra" keys back on the queue */
	while (p > n)
	{
		if (Term_key_push(buf[--p])) return (0);
	}

	/* We are now inside a macro */
	parse_macro = TRUE;
	inkey_sleep_semaphore = FALSE; //init semaphore for macros containing \wXX
	/* Assume that the macro has no problems with possibly missing items so far */
	macro_missing_item = 0;
	/* Paranoia: Make sure that the macro won't instantly fail because something failed before */
	abort_prompt = FALSE;

	/* Push the "macro complete" key */
	if (Term_key_push(29)) return (0);


	/* Access the macro action */
	act = macro__act[k];

	/* Get the length of the action */
	n = strlen(act);

#if 0
	/* Push the macro "action" onto the key queue */
	while (n > 0)
	{
		/* Push the key, notice over-flow */
		if (Term_key_push(act[--n])) return (0);
	}
#else
	/* Push all at once */
	Term_key_push_buf(act, n);
#endif

	/* Force "inkey()" to call us again */
	return (0);
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
 * Hack -- Make sure to allow calls to "inkey()" even if "term_screen"
 * is not the active Term, this allows the various "main-xxx.c" files
 * to only handle input when "term_screen" is "active".
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


	/* Hack -- handle delayed "flush()" */
	if (flush_later) {
		/* Done */
		flush_later = FALSE;

		/* Cancel "macro" info */
//#ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
if (c_cfg.keep_topline)
		restore_prompt();
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
	Term_activate(term_screen);


	/* Get a (non-zero) keypress */
	for (ch = 0; !ch; ) {
		/* Flush output - maintain flickering/multi-hued characters */
		do_flicker(); //unnecessary since it's done in inkey_aux() anyway? */

		/* Nothing ready, not waiting, and not doing "inkey_base" */
		if (!inkey_base && inkey_scan && (0 != Term_inkey(&ch, FALSE, FALSE))) break;

		/* Hack -- flush the output once no key is ready */
		if (!done && (0 != Term_inkey(&ch, FALSE, FALSE))) {
			/* Hack -- activate proper term */
			Term_activate(old);

			/* Flush output */
			Term_fresh();

			/* Hack -- activate the screen */
			Term_activate(term_screen);

			/* Only once */
			done = TRUE;
		}

		/* Hack */
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
					return ESCAPE;
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
		kk = ch = inkey_aux();

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


		/* Handle some special keys */
		switch (ch) {
			/* Hack -- convert back-quote into escape */
			case '`':

			/* Convert to "Escape" */
			ch = ESCAPE;

			/* Done */
			break;

			/* Hack -- strip "control-right-bracket" end-of-macro-action */
			case 29:

			/* Strip this key */
			ch = 0;

			/* Done */
			break;

			/* Hack -- strip "control-caret" special-keypad-indicator */
			case 30:

			/* Strip this key */
			ch = 0;

			/* Done */
			break;

			/* Hack -- strip "control-underscore" special-macro-triggers */
			case 31:

			/* Strip this key */
			ch = 0;

			/* Inside a "underscore" sequence */
			parse_under = TRUE;

			/* Strip chars (always) */
			strip_chars = TRUE;

			/* Done */
			break;

			/* Hack -- strip "control-backslash" special-fallback-strings */
			case 28:

			/* Strip this key */
			ch = 0;

			/* Inside a "control-backslash" sequence */
			parse_slash = TRUE;

			/* Strip chars (sometimes) */
			strip_chars = after_macro;

			/* Done */
			break;
		}


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
	return (ch);
}

static int hack_dir = 0;

/*
 * Convert a "Rogue" keypress into an "Angband" keypress
 * Pass extra information as needed via "hack_dir"
 *
 * Note that many "Rogue" keypresses encode a direction.
 */
static char roguelike_commands(char command) {
	/* Process the command */
	switch (command) {

	/* Movement (rogue keys) */
	case 'b': hack_dir = 1; return (';');
	case 'j': hack_dir = 2; return (';');
	case 'n': hack_dir = 3; return (';');
	case 'h': hack_dir = 4; return (';');
	case 'l': hack_dir = 6; return (';');
	case 'y': hack_dir = 7; return (';');
	case 'k': hack_dir = 8; return (';');
	case 'u': hack_dir = 9; return (';');

	/* Running (shift + rogue keys) */
	case 'B': hack_dir = 1; return ('.');
	case 'J': hack_dir = 2; return ('.');
	case 'N': hack_dir = 3; return ('.');
	case 'H': hack_dir = 4; return ('.');
	case 'L': hack_dir = 6; return ('.');
	case 'Y': hack_dir = 7; return ('.');
	case 'K': hack_dir = 8; return ('.');
	case 'U': hack_dir = 9; return ('.');

	/* Tunnelling (control + rogue keys) */
	case KTRL('B'): hack_dir = 1; return ('+');
	case KTRL('J'): hack_dir = 2; return ('+');
	case KTRL('N'): hack_dir = 3; return ('+');
	case KTRL('H'): hack_dir = 4; return ('+');
	case KTRL('L'): hack_dir = 6; return ('+');
	case KTRL('Y'): hack_dir = 7; return ('+');
	case KTRL('K'): hack_dir = 8; return ('+');
	case KTRL('U'): hack_dir = 9; return ('+');

	/* Oops, audio mixer */
	case KTRL('F'): return (KTRL('U'));
	case KTRL('V'): return (KTRL('N'));
	case KTRL('X'): return (KTRL('C'));

	/* Hack -- Commit suicide */
	/* ..instead display fps */
	//case KTRL('C'): return ('Q');
	/* Force-stack items */
	case KTRL('C'): return ('K');

	/* Hack -- White-space */
	case KTRL('M'): return ('\r');

	/* Allow use of the "destroy" command */
	case KTRL('D'): return ('k');

	/* Locate player on map */
	case 'W': return ('L');
	/* Browse a book (Peruse) */
	case 'P': return ('b');
	/* Steal */
	case 'S': return ('j');
	/* Toggle search mode */
	case '#': return ('S');
	/* Use a staff (Zap) */
	case 'Z': return ('u');
	/* Take off equipment */
	case 'T': return ('t');
	/* Fire an item */
	case 't': return ('f');
	/* Bash a door (Force) */
	case 'f': return ('B');
	/* Look around (examine) */
	case 'x': return ('l');
	/* Aim a wand (Zap) */
	case 'z': return ('a');
	/* Zap a rod (Activate) */
	case 'a': return ('z');
	/* Party mode */
	case 'O': return ('P');

	/* Secondary 'wear/wield' */
	case KTRL('W'): return ('W');
	/* Swap item */
	case KTRL('A'): return ('x');
	/* House commands */
	case KTRL('E'): return ('h');
	/* Reapply auto-inscriptions */
	case KTRL('G'): return ('H');
	/* Pick up exactly one item from a stack of same type of items (ie from piles, containing up to 99 <number> of this item) */
	case KTRL('Z'): return (KTRL('G'));

	/* Run */
	case ',': return ('.');
	/* Stay still (fake direction) */
	case '.': hack_dir = 5; return (',');
	/* Stay still (fake direction) */
	case '5': hack_dir = 5; return (',');

	/* Standard walking */
	case '1': hack_dir = 1; return (';');
	case '2': hack_dir = 2; return (';');
	case '3': hack_dir = 3; return (';');
	case '4': hack_dir = 4; return (';');
	case '6': hack_dir = 6; return (';');
	case '7': hack_dir = 7; return (';');
	case '8': hack_dir = 8; return (';');
	case '9': hack_dir = 9; return (';');
	}

	/* Default */
	return (command);
}



/*
 * Convert an "Original" keypress into an "Angband" keypress
 * Pass direction information back via "hack_dir".
 *
 * Note that "Original" and "Angband" are very similar.
 */
static char original_commands(char command) {
	/* Process the command */
	switch(command) {

	/* Hack -- White space */
	case KTRL('J'): return ('\r');
	case KTRL('M'): return ('\r');

	/* Tunnel */
	case 'T': return ('+');

	/* Run */
	case '.': return ('.');

	/* Stay still (fake direction) */
	case ',': hack_dir = 5; return (',');

	/* Stay still (fake direction) */
	case '5': hack_dir = 5; return (',');

	/* Standard walking */
	case '1': hack_dir = 1; return (';');
	case '2': hack_dir = 2; return (';');
	case '3': hack_dir = 3; return (';');
	case '4': hack_dir = 4; return (';');
	case '6': hack_dir = 6; return (';');
	case '7': hack_dir = 7; return (';');
	case '8': hack_dir = 8; return (';');
	case '9': hack_dir = 9; return (';');

	//(display fps) case KTRL('C'): return ('Q');
	/* Hack -- Commit suicide */
	case KTRL('K'): return ('Q');
	}

	/* Default */
	return (command);
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
		Term_xtra(TERM_XTRA_NOISE, 0);
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

/* Generate a page sfx (beep) */
int page(void) {
#ifdef USE_SOUND_2010
#ifdef SOUND_SDL
	/* Try to beep via page sfx of the SDL audio system first */
	if (c_cfg.audio_paging && sound_page()) return 1;
#endif
#endif

	/* Fall back on system-specific default beeps */
	//Term_fresh();
	Term_xtra(TERM_XTRA_NOISE, 0);
	//flush();

	return 1;
}
/* Generate a warning sfx (beep) or if it's missing then a page sfx */
int warning_page(void) {
#ifdef USE_SOUND_2010
#ifdef SOUND_SDL
	/* Try to beep via warning sfx of the SDL audio system first */
	if (sound_warning()) return 1;
	//if (c_cfg.audio_paging && sound_page()) return 1;
#endif
#endif

	/* Fall back on system-specific default beeps */
	//Term_fresh();
	Term_xtra(TERM_XTRA_NOISE, 0);
	//flush();

	return 1;
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
bool askfor_aux(char *buf, int len, char mode) {
	int y, x;
	int i = 0;
	int k = 0; /* Is the end of line */
	int l = 0; /* Is the cursor location on line */
	int j = 0; /* Loop iterator */

	bool search = FALSE;
	int s = 0, s2; //todo: check if s=0 is correct

	bool tail = FALSE;
	int l_old = l;
	bool modify_ok = TRUE;

	/* Terminal width */
	int wid = 80;

	/* Visible length on the screen */
	int vis_len = len;

	bool done = FALSE;

	/* Hack -- if short, don't use history */
	bool nohist = (mode & ASKFOR_PRIVATE) || len < 20;
	int cur_hist;

	if (mode & ASKFOR_CHATTING) {
		cur_hist = hist_chat_end;
	} else {
		cur_hist = hist_end;
	}

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
		modify_ok = FALSE;
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
		/* Place cursor */
		if (strlen(buf) > vis_len) {
			if (l > strlen(buf) - vis_len)
				Term_gotoxy(x + l + vis_len - strlen(buf), y);
			else
				Term_gotoxy(x, y); //would need a new var 'x_edit' to allow having the cursor at other positions
		} else
			Term_gotoxy(x + l, y);

		/* Get a key */
		i = inkey();

		/* Analyze the key */
		switch (i) {
		case ESCAPE:
		//case KTRL('Q'):
			k = 0;
			done = TRUE;
			break;

		case KTRL('C'): /* Allow searching for text in a previously entered chat message */
			if (nohist) break;
			if (!search) {
				/* Begin searching mode */
				search = TRUE;
				break;
			}

			/* Continue search by looking for next match */
			if (s == MSG_HISTORY_MAX) continue; /* No match exists at all */
			if (mode & ASKFOR_CHATTING) {
				/* Look for further match.. */
				for (s2 = s + 1; s2 < MSG_HISTORY_MAX; s2++) {
					if (!strstr(message_history_chat[s2], buf)) continue;
					/* Display the result message, overwriting the real 'buf' only visually, while keeping 'buf' unchanged */
					c_prt(TERM_YELLOW, format("%s: %s", buf, message_history_chat[s2]), y, x);
					s = s2;
					break;
				}
				if (s2 == MSG_HISTORY_MAX) { /* No further match found */
					/* Start from the beginning again, at this point we know there must be at least one match we can find now (again) */
					for (s2 = 0; s2 <= s; s2++) {
						if (!strstr(message_history_chat[s2], buf)) continue;
						/* Display the result message, overwriting the real 'buf' only visually, while keeping 'buf' unchanged */
						c_prt(TERM_YELLOW, format("%s: %s", buf, message_history_chat[s2]), y, x);
						s = s2;
						break;
					}
				}
			} else {
				/* Look for further match.. */
				for (s2 = s + 1; s2 < MSG_HISTORY_MAX; s2++) {
					if (!strstr(message_history[s2], buf)) continue;
					/* Display the result message, overwriting the real 'buf' only visually, while keeping 'buf' unchanged */
					c_prt(TERM_YELLOW, format("%s: %s", buf, message_history[s2]), y, x);
					s = s2;
					break;
				}
				if (s2 == MSG_HISTORY_MAX) { /* No further match found */
					/* Start from the beginning again, at this point we know there must be at least one match we can find now (again) */
					for (s2 = 0; s2 <= s; s2++) {
						if (!strstr(message_history[s2], buf)) continue;
						/* Display the result message, overwriting the real 'buf' only visually, while keeping 'buf' unchanged */
						c_prt(TERM_YELLOW, format("%s: %s", buf, message_history[s2]), y, x);
						s = s2;
						break;
					}
				}
			}
			continue;
		case '\n':
		case '\r':
			/* Catch searching mode */
			if (search) {
				/* We didn't find any match? */
				if (s == MSG_HISTORY_MAX) {
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
				if (mode & ASKFOR_CHATTING) strncpy(buf, message_history_chat[s], len);
				else strncpy(buf, message_history[s], len);
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

#if 0 /* urxvt actually seems to recognize 0x7F as BACKSPACE, unlike other terminals \
	which just ignore it, resulting in the Backspace key working like Delete key, \
	so we better keep this disabled here and move it back to '\010' below. - C. Blue */
		case 0x7F: /* DEL or ASCII 127 removes the char under cursor */
#endif
		case KTRL('D'):
			if (k > l) {
				/* Move the rest of the line one back */
				for (j = l + 1; j < k; j++) buf[j - 1] = buf[j];
				k--;
			}
			break;

		case 0x7F: /* well...not DEL but Backspace too it seems =P */
		case '\010': /* Backspace removes char before cursor */
			if (k == l && k > 0) {
				k--;
				l--;
			}
			if (k > l && l > 0) {
			  /* Move the rest of the line one back, including
			     char under cursor and cursor) */
				for (j = l; j < k; j++) buf[j - 1] = buf[j];
				l--;
				k--;
			}
			break;

		/* move by one */
		case KTRL('A'):
			if (l > 0) l--;
			break;
		case KTRL('S'):
			if (l < k) l++;
			break;
		/* jump words */
		case KTRL('Q'):
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
			l = 0;
			break;
		case KTRL('B'):
			l = k;
			break;

		case KTRL('I'): /* TAB */
			if (mode & ASKFOR_CHATTING) {
				/* Change chatting mode - mikaelh */
				chat_mode++;
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
			}
			break;

		case KTRL('U'): /* reverse KTRL('I') */
			if (mode & ASKFOR_CHATTING) {
				/* Reverse change chatting mode */
				chat_mode--;
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
			}
			break;

		case KTRL('R'): /* All/general ('message') */
			if (mode & ASKFOR_CHATTING) {
				chat_mode = CHAT_MODE_NORMAL;
				prt("Message: ", 0, 0);

				/* Recalculate visible length */
				vis_len = wid - 1 - sizeof("Message: ");

				Term_locate(&x, &y);
			}
			break;

		case KTRL('Y'): /* Party */
			if (mode & ASKFOR_CHATTING) {
				chat_mode = CHAT_MODE_PARTY;
				c_prt(C_COLOUR_CHAT_PARTY, "Party: ", 0, 0);

				/* Recalculate visible length */
				vis_len = wid - 1 - sizeof("Party: ");

				Term_locate(&x, &y);
			}
			break;

		case KTRL('F'): /* Floor/Depth/Level */
			if (mode & ASKFOR_CHATTING) {
				chat_mode = CHAT_MODE_LEVEL;
				c_prt(C_COLOUR_CHAT_LEVEL, "Floor: ", 0, 0);

				/* Recalculate visible length */
				vis_len = wid - 1 - sizeof("Floor: ");

				Term_locate(&x, &y);
			}
			break;

		case KTRL('G'): /* Guild */
			if (mode & ASKFOR_CHATTING) {
				chat_mode = CHAT_MODE_GUILD;
				c_prt(C_COLOUR_CHAT_GUILD, "Guild: ", 0, 0);

				/* Recalculate visible length */
				vis_len = wid - 1 - sizeof("Guild: ");

				Term_locate(&x, &y);
			}
			break;

#if 0 /* was: erase line. Can just hit ESC though, so if 0'ed. */
		case KTRL('U'):
			k = 0;
			break;
#endif

		case KTRL('N'):
			if (nohist) break;
			cur_hist++;

			if (mode & ASKFOR_CHATTING) {
				if ((!hist_chat_looped && hist_chat_end < cur_hist) ||
				    (hist_chat_looped && cur_hist >= MSG_HISTORY_MAX))
					cur_hist = 0;
				strncpy(buf, message_history_chat[cur_hist], len);
				buf[len] = '\0';
			} else {
				if ((!hist_looped && hist_end < cur_hist) ||
				    (hist_looped && cur_hist >= MSG_HISTORY_MAX))
					cur_hist = 0;
				strncpy(buf, message_history[cur_hist], len);
				buf[len] = '\0';
			}

			k = l = strlen(buf);
			break;
		case KTRL('P'):
			if (nohist) break;
			if (mode & ASKFOR_CHATTING) {
				if (cur_hist) cur_hist--;
				else cur_hist = hist_chat_looped ? MSG_HISTORY_MAX - 1 : hist_chat_end;
				strncpy(buf, message_history_chat[cur_hist], len);
				buf[len] = '\0';
			} else {
				if (cur_hist) cur_hist--;
				else cur_hist = hist_looped ? MSG_HISTORY_MAX - 1 : hist_end;
				strncpy(buf, message_history[cur_hist], len);
				buf[len] = '\0';
			}

			k = l = strlen(buf);
			break;

		/* word-size backspace */
		case KTRL('E'): {
			/* hack: CTRL+E is also the designated 'edit' key to edit a predefined default string */
			if (modify_ok) k = strlen(buf);

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
			break;
			}

		case KTRL('T'): /* Take a screenshot */
			xhtml_screenshot("screenshot????");
			break;

		default:
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
			/* Place character at currect cursor position after moving
			   the rest of the line one step forward */
			else if (k > l) {
				if ((k < len) && (isprint(i))) {
					for (j = k; j >= l; j--) buf[j + 1] = buf[j];
					buf[l++] = i;
					k++;
				}
			}
			else bell();
			break;
		}

		/* can only edit the default string with first key press being the designated 'edit' key */
		modify_ok = FALSE;

		/* Terminate */
		buf[k] = '\0';

		/* Are we in message-history-searching mode? */
		if (search) {
			/* empty search string initially */
			if (!buf[0]) {
				/* just skip.. */
				Term_erase(x, y, vis_len);
				s = MSG_HISTORY_MAX;
				continue;
			}
			if (mode & ASKFOR_CHATTING) {
				for (s = 0; s < MSG_HISTORY_MAX; s++) {
					if (!strstr(message_history_chat[s], buf)) continue;
					/* Display the result message, overwriting the real 'buf' only visually, while keeping 'buf' unchanged */
					c_prt(TERM_YELLOW, format("%s: %s", buf, message_history_chat[s]), y, x);
					break;
				}
				if (s == MSG_HISTORY_MAX) c_prt(TERM_ORANGE, buf, y, x);
			} else {
				for (s = 0; s < MSG_HISTORY_MAX; s++) {
					if (!strstr(message_history[s], buf)) continue;
					/* Display the result message, overwriting the real 'buf' only visually, while keeping 'buf' unchanged */
					c_prt(TERM_YELLOW, format("%s: %s", buf, message_history[s]), y, x);
					break;
				}
				if (s == MSG_HISTORY_MAX) c_prt(TERM_ORANGE, buf, y, x);
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
				c_prt(TERM_WHITE, buf, y, x);
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
	if (i == ESCAPE) return (FALSE);

	/* Success */
	if (nohist) return (TRUE);

	/* Add this to the history */
	if (mode & ASKFOR_CHATTING) {
		/* Only add to history if it isn't a repetition of our previous message */
		if (!hist_chat_end || strncmp(message_history_chat[hist_chat_end - 1], buf, sizeof(*message_history_chat) - 1)) {
			strncpy(message_history_chat[hist_chat_end], buf, sizeof(*message_history_chat) - 1);
			message_history_chat[hist_chat_end][sizeof(*message_history_chat) - 1] = '\0';
			hist_chat_end++;
			if (hist_chat_end >= MSG_HISTORY_MAX) {
				hist_chat_end = 0;
				hist_chat_looped = TRUE;
			}
		}
	} else {
		/* Only add to history if it isn't a repetition of our previous message */
		if (!hist_end || strncmp(message_history_chat[hist_end - 1], buf, sizeof(*message_history) - 1)) {
			strncpy(message_history[hist_end], buf, sizeof(*message_history) - 1);
			message_history[hist_end][sizeof(*message_history) - 1] = '\0';
			hist_end++;
			if (hist_end >= MSG_HISTORY_MAX) {
				hist_end = 0;
				hist_looped = TRUE;
			}
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

		if (chat_mode == CHAT_MODE_PARTY)
			buf[0] = '!';
		else if (chat_mode == CHAT_MODE_LEVEL)
			buf[0] = '#';
		else if (chat_mode == CHAT_MODE_GUILD)
			buf[0] = '$';
		else
			buf[0] = '\0';
		buf[1] = ':';
		k += 2;
		buf[k] = '\0';
	}

	/* Success */
	return (TRUE);
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
	return (res);
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
	if (*command == ESCAPE) return (FALSE);

	/* Success */
	return (TRUE);
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
if (!c_cfg.keep_topline)
	/* Clear top line */
	clear_topline();
//#endif

	/* Bypass "keymap" */
	if (cmd == '\\') {
//#ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
if (c_cfg.keep_topline)
		/* Clear top line */
		clear_topline();
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
if (c_cfg.keep_topline)
		/* Clear top line */
		clear_topline();
//#endif

		/* Use the key directly */
		command_cmd = cmd;
	} else {
		/* Hack -- allow "control chars" to be entered */
		if (cmd == '^') {
//#ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
if (c_cfg.keep_topline)
			/* Clear top line */
			clear_topline();
//#endif

			/* Get a char to "cast" into a control char */
			(void)(get_com("Control: ", &cmd));

			/* Convert */
			cmd = KTRL(cmd);

//#ifdef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
if (c_cfg.keep_topline)
			/* Clear top line */
			clear_topline();
//#endif
		}

		/* Access the array info */
		command_cmd = keymap_cmds[cmd & 0x7F];
		command_dir = keymap_dirs[cmd & 0x7F];
	}

	/* Paranoia */
	if (!command_cmd) command_cmd = ESCAPE;

//#ifndef DONT_CLEAR_TOPLINE_IF_AVOIDABLE
if (!c_cfg.keep_topline)
	/* Hack -- erase the message line. */
	clear_topline();
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

	if (!dir) return (FALSE);
	return (TRUE);
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
	if (!c_quit)
		Flush_queue();

	/* More normal */
	if (default_yes) {
		if (i == 'n' || i == 'N' || i == '\e') return FALSE;
		return TRUE;
	}

	if ((i == 'Y') || (i == 'y')) return (TRUE);
	return (FALSE);
}
/* default_choice:
   0 = no preference, only accept y/Y/n/N for input
   1 = default is yes, any key besides n/N will count as yes
   2 = default is yes, any key besides y/Y will count as no */
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
		if (i == 'n' || i == 'N' || i == '\e') return FALSE;
		return TRUE;
	}

	if ((i == 'Y') || (i == 'y')) return (TRUE);
	return (FALSE);
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

	return res;
}


/*
 * Recall the "text" of a saved message
 */
cptr message_str(s32b age)
{
        s32b x;
        s32b o;
        cptr s;

        /* Forgotten messages have no text */
        if ((age < 0) || (age >= message_num())) return ("");

        /* Acquire the "logical" index */
        x = (message__next + MESSAGE_MAX - (age + 1)) % MESSAGE_MAX;

        /* Get the "offset" for the message */
        o = message__ptr[x];

        /* Access the message text */
        s = &message__buf[o];

        /* Return the message text */
        return (s);
}
cptr message_str_chat(s32b age)
{
        s32b x;
        s32b o;
        cptr s;

        /* Forgotten messages have no text */
        if ((age < 0) || (age >= message_num_chat())) return ("");

        /* Acquire the "logical" index */
        x = (message__next_chat + MESSAGE_MAX - (age + 1)) % MESSAGE_MAX;

        /* Get the "offset" for the message */
        o = message__ptr_chat[x];

        /* Access the message text */
        s = &message__buf_chat[o];

        /* Return the message text */
        return (s);
}
cptr message_str_msgnochat(s32b age)
{
        s32b x;
        s32b o;
        cptr s;

        /* Forgotten messages have no text */
        if ((age < 0) || (age >= message_num_msgnochat())) return ("");

        /* Acquire the "logical" index */
        x = (message__next_msgnochat + MESSAGE_MAX - (age + 1)) % MESSAGE_MAX;

        /* Get the "offset" for the message */
        o = message__ptr_msgnochat[x];

        /* Access the message text */
        s = &message__buf_msgnochat[o];

        /* Return the message text */
        return (s);
}
cptr message_str_impscroll(s32b age)
{
        s32b x;
        s32b o;
        cptr s;

        /* Forgotten messages have no text */
        if ((age < 0) || (age >= message_num_impscroll())) return ("");

        /* Acquire the "logical" index */
        x = (message__next_impscroll + MESSAGE_MAX - (age + 1)) % MESSAGE_MAX;

        /* Get the "offset" for the message */
        o = message__ptr_impscroll[x];

        /* Access the message text */
        s = &message__buf_impscroll[o];

        /* Return the message text */
        return (s);
}


/*
 * How many messages are "available"?
 */
s32b message_num(void) {
        int last, next, n;

#ifdef RETRY_LOGIN
	/* Don't prompt to 'save chat messages?' when quitting the game from within the character screen,
	   if we have previousl been in-game and therefore messages have had been added. */
	if (!in_game) return 0;
#endif

        /* Extract the indexes */
        last = message__last;
        next = message__next;

        /* Handle "wrap" */
        if (next < last) next += MESSAGE_MAX;

        /* Extract the space */
        n = (next - last);

        /* Return the result */
        return (n);
}
s32b message_num_chat(void)
{
        int last, next, n;

        /* Extract the indexes */
        last = message__last_chat;
        next = message__next_chat;

        /* Handle "wrap" */
        if (next < last) next += MESSAGE_MAX;

        /* Extract the space */
        n = (next - last);

        /* Return the result */
        return (n);
}
s32b message_num_msgnochat(void)
{
        int last, next, n;

        /* Extract the indexes */
        last = message__last_msgnochat;
        next = message__next_msgnochat;

        /* Handle "wrap" */
        if (next < last) next += MESSAGE_MAX;

        /* Extract the space */
        n = (next - last);

        /* Return the result */
        return (n);
}
s32b message_num_impscroll(void)
{
        int last, next, n;

        /* Extract the indexes */
        last = message__last_impscroll;
        next = message__next_impscroll;

        /* Handle "wrap" */
        if (next < last) next += MESSAGE_MAX;

        /* Extract the space */
        n = (next - last);

        /* Return the result */
        return (n);
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
        for (i = message__next; k; k--)
        {
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
        if (message__head + n + 1 >= MESSAGE_BUF)
        {
                /* Kill all "dead" messages */
                for (i = message__last; TRUE; i++)
                {
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
        if (message__head + n + 1 > message__tail)
        {
                /* Grab new "tail" */
                message__tail = message__head + n + 1;

                /* Advance tail while possible past first "nul" */
                while (message__buf[message__tail-1]) message__tail++;

                /* Kill all "dead" messages */
                for (i = message__last; TRUE; i++)
                {
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
        {
                /* Copy the message */
                message__buf[message__head + i] = str[i];
        }

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
        for (i = message__next_chat; k; k--)
        {
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
        if (message__head_chat + n + 1 >= MESSAGE_BUF)
        {
                /* Kill all "dead" messages */
                for (i = message__last_chat; TRUE; i++)
                {
                        /* Wrap if needed */
                        if (i == MESSAGE_MAX) i = 0;

                        /* Stop before the new message */
                        if (i == message__next_chat) break;

                        /* Kill "dead" messages */
                        if (message__ptr_chat[i] >= message__head_chat)
                        {
                                /* Track oldest message */
                                message__last_chat = i + 1;
                        }
                }

                /* Wrap "tail" if needed */
                if (message__tail_chat >= message__head_chat) message__tail_chat = 0;

                /* Start over */
                message__head_chat = 0;
        }


        /*** Step 4 -- Ensure space before next message ***/

        /* Kill messages if needed */
        if (message__head_chat + n + 1 > message__tail_chat)
        {
                /* Grab new "tail" */
                message__tail_chat = message__head_chat + n + 1;

                /* Advance tail while possible past first "nul" */
                while (message__buf_chat[message__tail_chat-1]) message__tail_chat++;

                /* Kill all "dead" messages */
                for (i = message__last_chat; TRUE; i++)
                {
                        /* Wrap if needed */
                        if (i == MESSAGE_MAX) i = 0;

                        /* Stop before the new message */
                        if (i == message__next_chat) break;

                        /* Kill "dead" messages */
                        if ((message__ptr_chat[i] >= message__head_chat) &&
                            (message__ptr_chat[i] < message__tail_chat))
                        {
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
        {
                /* Copy the message */
                message__buf_chat[message__head_chat + i] = str[i];
        }

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
        for (i = message__next_msgnochat; k; k--)
        {
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
        if (message__head_msgnochat + n + 1 >= MESSAGE_BUF)
        {
                /* Kill all "dead" messages */
                for (i = message__last_msgnochat; TRUE; i++)
                {
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
        if (message__head_msgnochat + n + 1 > message__tail_msgnochat)
        {
                /* Grab new "tail" */
                message__tail_msgnochat = message__head_msgnochat + n + 1;

                /* Advance tail while possible past first "nul" */
                while (message__buf_msgnochat[message__tail_msgnochat-1]) message__tail_msgnochat++;

                /* Kill all "dead" messages */
                for (i = message__last_msgnochat; TRUE; i++)
                {
                        /* Wrap if needed */
                        if (i == MESSAGE_MAX) i = 0;

                        /* Stop before the new message */
                        if (i == message__next_msgnochat) break;

                        /* Kill "dead" messages */
                        if ((message__ptr_msgnochat[i] >= message__head_msgnochat) &&
                            (message__ptr_msgnochat[i] < message__tail_msgnochat))
                        {
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
        {
                /* Copy the message */
                message__buf_msgnochat[message__head_msgnochat + i] = str[i];
        }

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
        for (i = message__next_impscroll; k; k--)
        {
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
        if (message__head_impscroll + n + 1 >= MESSAGE_BUF)
        {
                /* Kill all "dead" messages */
                for (i = message__last_impscroll; TRUE; i++)
                {
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
        if (message__head_impscroll + n + 1 > message__tail_impscroll)
        {
                /* Grab new "tail" */
                message__tail_impscroll = message__head_impscroll + n + 1;

                /* Advance tail while possible past first "nul" */
                while (message__buf_impscroll[message__tail_impscroll-1]) message__tail_impscroll++;

                /* Kill all "dead" messages */
                for (i = message__last_impscroll; TRUE; i++)
                {
                        /* Wrap if needed */
                        if (i == MESSAGE_MAX) i = 0;

                        /* Stop before the new message */
                        if (i == message__next_impscroll) break;

                        /* Kill "dead" messages */
                        if ((message__ptr_impscroll[i] >= message__head_impscroll) &&
                            (message__ptr_impscroll[i] < message__tail_impscroll))
                        {
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
        {
                /* Copy the message */
                message__buf_impscroll[message__head_impscroll + i] = str[i];
        }

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
//      	p_ptr->window |= PW_MESSAGE;
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

	if (!c_cfg.topline_no_msg)
		/* using clear_topline() here prevents top-line clearing via c_msg_print(NULL) */
		if (!topline_icky) clear_topline();

	/* No message */
	if (!msg) return;

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

	/* Don't display any messages in top line? */
	if (c_cfg.topline_no_msg) return;

	/* Small length limit */
	if (n > 80) n = 80;

	/* Display the tail of the message */
	if (!topline_icky) Term_putstr(0, 0, n, TERM_WHITE, t);

	/* Restore cursor */
	Term_gotoxy(x, y);

	/* Remember the message */
	msg_flag = TRUE;
}

/*
 * Display a formatted message, using "vstrnfmt()" and "c_msg_print()".
 */
void c_msg_format(cptr fmt, ...)
{
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
 *
 * Hack -- allow "command_arg" to specify a quantity
 * Hack: if max is < -1 this changes the default amount to -max.
 *       Added for handling item stacks in one's inventory -- C. Blue
 */
#define QUANTITY_WIDTH 10
s32b c_get_quantity(cptr prompt, s32b max) {
	s32b amt;
	char tmp[80];
	char buf[80];

	char bi1[80], bi2[6 + 1];
	int n = 0, i = 0, j = 0;
	s32b i1 = 0, i2 = 0, mul = 1;

	if (max < -1) {
		max = -max;
		/* Default to all */
		amt = max;
	} else if (!max) amt = 0; /* max is 0 */
	else amt = 1; /* Default to one */

	/* Build the default */
	sprintf(buf, "%d", amt);

	/* Build a prompt if needed */
	if (!prompt) {
		/* Build a prompt */
		inkey_letter_all = TRUE;
		sprintf(tmp, "Quantity (1-%d, 'a' or spacebar for all): ", max);

		/* Use that prompt */
		prompt = tmp;
	}

	/* Ask for a quantity */
	if (!get_string(prompt, buf, QUANTITY_WIDTH)) return (0);

#if 1
	/* special hack to enable old 'any letter = all' hack without interfering with 'k'/'M' for kilo/mega: */
	if ((buf[0] == 'k' || buf[0] == 'K' || buf[0] == 'm' || buf[0] == 'M') && buf[1]
	    && (buf[1] < '0' || buf[1] > '9')) {
		//all..
		buf[0] = 'a';
		buf[1] = 0;
	} else

	/* new method slightly extended: allow leading 'k' or 'm' too */
	if ((buf[0] == 'k' || buf[0] == 'K' || buf[0] == 'm' || buf[0] == 'M') && buf[1]) {
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
	if (mul > 1) {
		n++;
		i = 0;
		while(buf[n] >= '0' && buf[n] <= '9' && i < 6) bi2[i++] = buf[n++];
		bi2[i] = '\0';
//Send_msg(format("%s-%s", bi1, bi2));

		i = 0;
		while (i < 6) {
			if (bi2[i] == '\0') {
				j = i;
				while (j < 6) bi2[j++] = '0';
				break;
			}
			i++;
		}
//Send_msg(format("%s-%d", bi2, mul));

		if (mul == 1000) bi2[3] = '\0';
		else if (mul == 1000000) bi2[6] = '\0';

		i2 = atoi(bi2);
		amt = i1 * mul + i2;
	} else amt = i1;

#else

	/* Extract a number */
	amt = atoi(buf);

	/* Analyse abreviation like '15k' */
	if (strchr(buf, 'k'))
		amt = amt * 1000;
	else if (strchr(buf, 'm') || strchr(buf, 'M')
		amt = amt * 1000000;
#endif


	/* 'a' means "all" - C. Blue */
	if (buf[0] == 'a'
	    /* allow old 'type in any letter for "all"' hack again too?: */
	    || isalpha(buf[0])
	    ) {
		if (max >= 0) amt = max;
		/* hack for dropping gold (max is -1) */
		else amt = 1000000000;
	}

	/* Enforce the maximum, if maximum is defined */
	if ((max >= 0) && (amt > max)) amt = max;

	/* Enforce the minimum */
	if (amt < 0) amt = 0;

	/* Return the result */
	return (amt);
}

void clear_from(int row)
{
	int y;

	/* Erase requested rows */
	for (y = row; y < Term->hgt; y++)
	{
		/* Erase part of the screen */
		Term_erase(0, y, 255);
	}
}

void prt_num(cptr header, int num, int row, int col, byte color)
{
	int len = strlen(header);
	char out_val[32];
	put_str(header, row, col);
	put_str("   ", row, col + len);
	(void)sprintf(out_val, "%6d", num);
	c_put_str(color, out_val, row, col + len + 3);
}

void prt_lnum(cptr header, s32b num, int row, int col, byte color)
{
	int len = strlen(header);
	char out_val[32];
	put_str(header, row, col);
	(void)sprintf(out_val, "%9d", (int)num);
	c_put_str(color, out_val, row, col + len);
}


static void ascii_to_text(char *buf, cptr str)
{
	char *s = buf;

	/* Analyze the "ascii" string */
	while (*str)
	{
		byte i = (byte)(*str++);

		if (i == ESCAPE)
		{
			*s++ = '\\';
			*s++ = 'e';
		}
		else if (i == MACRO_WAIT)
		{
			*s++ = '\\';
			*s++ = 'w';
		}
		else if (i == ' ')
		{
			*s++ = '\\';
			*s++ = 's';
		}
		else if (i == '\b')
		{
			*s++ = '\\';
			*s++ = 'b';
		}
		else if (i == '\t')
		{
			*s++ = '\\';
			*s++ = 't';
		}
		else if (i == '\n')
		{
			*s++ = '\\';
			*s++ = 'n';
		}
		else if (i == '\r')
		{
			*s++ = '\\';
			*s++ = 'r';
		}
		else if (i == '^')
		{
			*s++ = '\\';
			*s++ = '^';
		}
		else if (i == '\\')
		{
			*s++ = '\\';
			*s++ = '\\';
		}
		else if (i < 32)
		{
			*s++ = '^';
			*s++ = i + 64;
		}
		else if (i < 127)
		{
			*s++ = i;
		}
		else if (i < 64)
		{
			*s++ = '\\';
			*s++ = '0';
			*s++ = octify(i / 8);
			*s++ = octify(i % 8);
		}
		else
		{
			*s++ = '\\';
			*s++ = 'x';
			*s++ = hexify(i / 16);
			*s++ = hexify(i % 16);
		}
	}

	/* Terminate */
	*s = '\0';
}

static errr macro_dump(cptr fname)
{
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
	if (!fff) return (-1);


	/* Skip space */
	fprintf(fff, "\n\n");

	/* Start dumping */
	fprintf(fff, "# Automatic macro dump\n\n");

	/* Dump them */
	for (i = 0; i < macro__num; i++)
	{
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

		for (j = 0, n = strlen(macro__act[i]); j < n; j += 1023)
		{
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
	return (0);
}

static void get_macro_trigger(char *buf)
{
	int i, n = 0;

	char tmp[1024];

	/* Flush */
	flush();

	/* Do not process macros */
	inkey_base = TRUE;

	/* First key */
	i = inkey();

	/* Read the pattern */
	while (i)
	{
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
void interact_macros(void) {
	int i, j = 0, l;

	char tmp[160], buf[1024], buf2[1024], *bptr, *b2ptr, chain_macro_buf[1024];

	char fff[1024], t_key[10], choice;
	bool m_ctrl, m_alt, m_shift, t_hyb, t_com;

	bool were_recording = FALSE;


	/* Save screen */
	Term_save();

	/* No macros should work within the macro menu itself */
	inkey_interact_macros = TRUE;

	/* Did we just finish recording a macro by entering this menu? */
	if (recording_macro) {
		/* stop recording */
		were_recording = TRUE;
		recording_macro = FALSE;
		/* cut off the last key which was '%' to bring us back here */
		recorded_macro[strlen(recorded_macro) - 1] = '\0';
		/* use the recorded macro */
		strcpy(macro__buf, recorded_macro);
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
		Term_putstr(5, l++, -1, TERM_L_BLUE, "(\377ys\377B) Save macros to a pref file");
		Term_putstr(5, l++, -1, TERM_WHITE, "(\377yl\377w) Load macros from a pref file");
		l++;
		Term_putstr(5, l++, -1, TERM_WHITE, "(\377yd\377w) Delete a macro from a key   (restores a key's normal behaviour)");
		Term_putstr(5, l++, -1, TERM_WHITE, "(\377yI\377w) Reinitialize all macros     (discards all unsaved macros)");
		Term_putstr(5, l++, -1, TERM_WHITE, "(\377yG\377w/\377yC\377w/\377yB\377w/\377yU\377w/\377yA\377w) Forget global.prf / <character>.prf / both / most / all");
		Term_putstr(5, l++, -1, TERM_WHITE, "(\377yt\377w/\377yi\377w) Test a key for an existing macro / list all currently defined macros");
		Term_putstr(5, l++, -1, TERM_WHITE, "(\377yw\377w) Switch the macro(s) of two keys");
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
		l++;

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
		else if (i == KTRL('T')) xhtml_screenshot("screenshot????");

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
			(void)process_pref_file(tmp);

			/* Pref files may change settings, so reload the keymap - mikaelh */
			keymap_init();
		}

		/* Save a 'macro' file */
		else if (i == 's') {
			/* Prompt */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Save a macro file");

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
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Save a macro file to global.prf");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, l + 1, -1, TERM_WHITE, "File: ");

			/* Default filename */
			strcpy(tmp, "global.prf");

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
				strcpy(buf, "pref.prf");
				process_pref_file(buf);
				/* Access the "basic" system pref file */
				sprintf(buf, "pref-%s.prf", ANGBAND_SYS);
				process_pref_file(buf);

 #ifdef FORGET_MACRO_VISUALS
				/* Access the "visual" system pref file (if any) */
				handle_process_font_file();
 #endif

				macro_trigger_exclusive[0] = 0; //unhack
				macro_processing_exclusive = FALSE;
			} else
				c_msg_print("No macro was found.");
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
					if (macro__hyb[i]) c_msg_print("A hybrid macro was found.");
					else if (macro__cmd[i]) c_msg_print("A command macro was found.");
					else {
						if (!macro__act[i][0]) c_msg_print("An empty macro was found.");
						else c_msg_print("A normal macro was found.");
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

			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Switch macro(s) of two keys");

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
				c_msg_format("Switching macros of first key (%s) and second key (%s).",
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
					/* '^%c' : ctrl + a simple key */
					m_ctrl = TRUE; /* not testing for the '^' actually, we assume it must be there.. */
					sprintf(t_key, "%c        ", buf[1]);

					/* Some special keys are represented by CTRL+<normal letter> combo.. >_> */
					if (buf[1] == 'b') strcat(t_key, " (BACK)"); // and also 'Del' key!
					if (buf[1] == 'r') strcat(t_key, " (ENTER)");
					if (buf[1] == 's') strcat(t_key, " (SPACE)");
					if (buf[1] == 't') strcat(t_key, " (TAB)");
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

						/* Numper pad (Numlock independant) */
						case 0xAF: strcpy(bptr,  "Num/"); break;
						case 0xAA: strcpy(bptr,  "Num*"); break;
						case 0xAD: strcpy(bptr,  "Num-"); break;
						case 0xAB: strcpy(bptr,  "Num+"); break;
						case 0x8D: strcpy(bptr,  "NumRet"); break;

						/* Numper pad (Numlock off) */
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

						/* Numper pad (Numlock off) */
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

						/* Numper pad (Numlock off) */
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

						/* Numper pad (Numlock on)
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

						/* Numper pad (Numlock independant) */
						case 0xAF: strcpy(bptr,  "Num/"); break;
						case 0xAA: strcpy(bptr,  "Num*"); break;
						case 0xAD: strcpy(bptr,  "Num-"); break;
						case 0xAB: strcpy(bptr,  "Num+"); break;
						case 0x8D: strcpy(bptr,  "NumRet"); break;

						/* Numper pad (Numlock off) */
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

						/* Numper pad (Numlock off) */
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
						xhtml_screenshot("screenshot????");
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
			bool call_by_name = FALSE, mimic_transform = FALSE, mimic_transform_by_name = FALSE;

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
			bptr = buf;
			b2ptr = buf2 + 3;
			while (*bptr) {
				if (call_by_name) {
					if (*bptr == '\\') {
						call_by_name = FALSE;
						*b2ptr++ = '\\'; *b2ptr++ = 'r';
						bptr++;
					} else {
						*b2ptr++ = *bptr++;
					}
				} else {
					/* service: mimic-transformation automatically adds the required '\r' at the end */
					switch (*bptr) {
					case 'P': case 'I': case 'F': case 'S': case 'T': case 'M': case 'R': case 's':
						if (mimic_transform) {
							if (!mimic_transform_by_name) {
								*b2ptr++ = '\\'; *b2ptr++ = 'r';
							}
							mimic_transform = mimic_transform_by_name = FALSE;
						}
						break;
					default:
						if (mimic_transform && ((*bptr) < '0' || (*bptr) > '9')) {
							*b2ptr++ = '\\'; *b2ptr++ = 'r';
							mimic_transform = FALSE;
						}
					}

					switch (*bptr) {
					case 'P': /* use innate mimic power */
						*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '3'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
						bptr++;	break;
					case 'I': /* use innate mimic power: transform into specific */
						mimic_transform = TRUE;
						if (*(bptr + 1) == '@') mimic_transform_by_name = TRUE;
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
						*b2ptr++ = '@';
						bptr++;	break;
					default:
						*b2ptr++ = *bptr++;
					}
				}
			}

			/* service: mimic-transformation automatically adds the required '\r' at the end */
			if (mimic_transform) {
				if (!mimic_transform_by_name) {
					*b2ptr++ = '\\'; *b2ptr++ = 'r';
				}
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
			Term_putstr(19, 20, -1, TERM_L_RED, ">>>Press any key to start recording<<<");

			/* Wait for confirming keypress to finally start recording */
			inkey();

			/* Reload screen */
			Term_load();

			/* Flush the queue */
			Flush_queue();

			/* leave macro menu */
			inkey_interact_macros = FALSE;
			/* hack: Display recording status */
			Send_redraw(0);

			/* enter recording mode */
			strcpy(recorded_macro, "");
			recording_macro = TRUE;

			/* leave the macro menu */
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

			//initialize_main_pref_files();
			//initialize_player_pref_files();

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
			sprintf(buf, "%s.prf", cname);
			process_pref_file(buf);

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

			//initialize_main_pref_files();
			//initialize_player_pref_files();

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
			sprintf(buf, "%s.prf", cname);
			process_pref_file(buf);
#endif

			macro_processing_exclusive = FALSE;
			c_msg_format("Reninitialized all macros, omitting '%s.prf'", cname);
		}

		else if (i == 'B') {
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

			//initialize_main_pref_files();
			//initialize_player_pref_files();

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
			sprintf(buf, "%s.prf", cname);
			process_pref_file(buf);
#endif

			macro_processing_exclusive = FALSE;
			c_msg_format("Reninitialized all macros, omitting 'global.prf' and '%s.prf'", cname);
		}

		else if (i == 'U') {
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

			//initialize_main_pref_files();
			//initialize_player_pref_files();

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
			sprintf(buf, "%s.prf", cname);
			process_pref_file(buf);
#endif

			macro_processing_exclusive = FALSE;
			c_msg_print("Reninitialized all macros, omitting all usually customized prf-files:");
			c_msg_format(" 'global.prf', '%s.prf', '%s.prf', '%s.prf, '%s.prf'", cname,
			    race < Setup.max_race ? race_info[race].title : "NO_RACE",
			    trait < Setup.max_trait ? trait_info[trait].title : "NO_TRAIT",
			    class < Setup.max_class ? class_info[class].title : "NO_CLASS");
		}

		else if (i == 'A') {
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

			//initialize_main_pref_files();
			//initialize_player_pref_files();

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
			sprintf(buf, "%s.prf", cname);
			process_pref_file(buf);

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

		else if (i == 'z') {
			int target_dir = '5';
			bool should_wait;
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
#define mw_device 'l'
#define mw_anydir 'k'
#define mw_any 'K'
#define mw_abilitynt 'm'
#define mw_abilityt 'M'
#define mw_custom 'n'
#define mw_load 'o'
#define mw_equip 'p'

#define mw_LAST 'p'

			/* Invoke wizard to create a macro step-by-step as easy as possible  */
			Term_putstr(0, l, -1, TERM_L_GREEN, "Command: Invoke macro wizard");

			/* initialise macro-chaining */
			chain_macro_buf[0] = 0;
Chain_Macro:
			should_wait = FALSE;

			/* Clear screen */
			Term_clear();

			/* Describe */
			Term_putstr(29, 0, -1, TERM_L_UMBER, "*** Macro Wizard ***");
			//Term_putstr(25, 22, -1, TERM_L_UMBER, "[Press ESC to exit anytime]");
			Term_putstr(1, 6, -1, TERM_L_DARK, "Don't forget to save your macros with 's' when you are back in the macro menu!");
			Term_putstr(19, 7, -1, TERM_L_UMBER, "----------------------------------------");

			/* Initialize wizard state: First state */
			i = choice = 0;

			while (i != -1) {
				Term_putstr(12, 2, -1, i == 0 ? TERM_L_GREEN : TERM_SLATE, "Step 1:  Choose an action for the macro to perform.");
				Term_putstr(12, 3, -1, i == 1 ? TERM_L_GREEN : TERM_SLATE, "Step 2:  If required, choose item, spell, and targetting method.");
				Term_putstr(12, 4, -1, i == 2 ? TERM_L_GREEN : TERM_SLATE, "Step 3:  Choose the key you want to bind the macro to.");

				clear_from(8);

				switch (i) {
				case 0:
					Term_putstr( 5, 9, -1, TERM_GREEN, "Which of the following actions should the macro perform?");
					Term_putstr(8, 10, -1, TERM_L_GREEN, "a/b) Drink a potion/read a scroll");
					Term_putstr(8, 11, -1, TERM_L_GREEN, "c/C) Fire ranged weapon/throw an item");
					Term_putstr(8, 12, -1, TERM_L_GREEN, "d/D) Cast school/mimic spell without a target (or target manually)");
					Term_putstr(8, 13, -1, TERM_L_GREEN, "e/E) Cast school/mimic spell with target");
					Term_putstr(8, 14, -1, TERM_L_GREEN, "f)   Cast a mimic spell by number (with and without target)");
					Term_putstr(8, 15, -1, TERM_L_GREEN, "g/G) Polymorph into monster/set preferred immunity (mimicry users)");
					Term_putstr(8, 16, -1, TERM_L_GREEN, "h/H) Draw runes to cast a runespell / set up a monster trap");
					Term_putstr(8, 17, -1, TERM_L_GREEN, "i/I) Use a fighting technique/switch combat stance (most melee classes)");
					Term_putstr(8, 18, -1, TERM_L_GREEN, "j)   Use a shooting technique (archers and rangers)");
					Term_putstr(8, 19, -1, TERM_L_GREEN, "k/K) Use any item with / without a target)");
					Term_putstr(8, 20, -1, TERM_L_GREEN, "l)   Use a magic device or activate an item");
					Term_putstr(8, 21, -1, TERM_L_GREEN, "m/M) Use a basic ability ('m') without/with target (Draconian breath)");
					Term_putstr(8, 22, -1, TERM_L_GREEN, "n)   Enter a custom action (same as pressing 'a' in macro screen)");
					Term_putstr(8, 23, -1, TERM_L_GREEN, "o/p) Load a macro file / change equipment (wield/takeoff/swap)");

					while (TRUE) {
						switch (choice = inkey()) {
						case ESCAPE:
						case 'p':
						case '\010': /* backspace */
							i = -1; /* leave */
							break;
						case KTRL('T'):
							/* Take a screenshot */
							xhtml_screenshot("screenshot????");
							continue;
						default:
							/* invalid action -> exit wizard */
							if ((choice < 'a' || choice > mw_LAST) &&
							    choice != 'C' && choice != 'D' && choice != 'E' && choice != 'M' &&
							    choice != 'G' && choice != 'I' && choice != 'K' && choice != 'H') {
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
					switch (choice) {
					case mw_quaff:
						Term_putstr(5, 10, -1, TERM_GREEN, "Please enter a distinctive part of the potion's name or inscription.");
						//Term_putstr(5, 11, -1, TERM_GREEN, "and pay attention to upper-case and lower-case letters!");
						Term_putstr(5, 11, -1, TERM_GREEN, "For example, enter:     \377GCritical Wounds");
						Term_putstr(5, 12, -1, TERM_GREEN, "if you want to quaff a 'Potion of Cure Critical Wounds'.");
						Term_putstr(5, 16, -1, TERM_L_GREEN, "Enter partial potion name or inscription:");
						break;
					case mw_read:
						Term_putstr(5, 10, -1, TERM_GREEN, "Please enter a distinctive part of the scroll's name or inscription.");
						//Term_putstr(5, 11, -1, TERM_GREEN, "and pay attention to upper-case and lower-case letters!");
						Term_putstr(5, 11, -1, TERM_GREEN, "For example, enter:     \377GPhase Door");
						Term_putstr(5, 12, -1, TERM_GREEN, "if you want to read a 'Scroll of Phase Door'.");
						Term_putstr(5, 16, -1, TERM_L_GREEN, "Enter partial scroll name or inscription:");
						break;
					case mw_any:
					case mw_anydir:
						Term_putstr(5, 10, -1, TERM_GREEN, "Please enter a distinctive part of the item's name or inscription.");
						//Term_putstr(5, 11, -1, TERM_GREEN, "and pay attention to upper-case and lower-case letters!");
						if (choice == mw_any) {
							Term_putstr(5, 11, -1, TERM_GREEN, "For example, enter:     \377GRation");
							Term_putstr(5, 12, -1, TERM_GREEN, "if you want to use (eat) a 'Ration of Food'.");
						} else {
							/* actually a bit silyl here, we're not really using inscriptions but instead treat them as text -_-.. */
							Term_putstr(5, 11, -1, TERM_GREEN, "For example, enter:     \377G@/0"); /* ..so 'correctly' this should just be '/0' :-p */
							Term_putstr(5, 12, -1, TERM_GREEN, "if you want to use (fire) a wand or rod inscribed '@/0'.");
						}
						Term_putstr(5, 16, -1, TERM_L_GREEN, "Enter partial potion name or inscription:");
						break;
					case mw_schoolnt:
						Term_putstr(10, 10, -1, TERM_GREEN, "Please enter the exact spell name.");// and pay attention");
						//Term_putstr(10, 11, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, 11, -1, TERM_GREEN, "For example, enter:     \377GPhase Door");
						Term_putstr(10, 12, -1, TERM_GREEN, "You must have learned a spell before you can use it!");
						Term_putstr(15, 16, -1, TERM_L_GREEN, "Enter exact spell name:");
						break;
					case mw_mimicnt:
						Term_putstr(10, 10, -1, TERM_GREEN, "Please enter the exact spell name.");//and pay attention");
						//Term_putstr(10, 11, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, 11, -1, TERM_GREEN, "For example, enter:     \377GBlink");
						Term_putstr(10, 12, -1, TERM_GREEN, "You must have learned a spell before you can use it!");
						Term_putstr(15, 16, -1, TERM_L_GREEN, "Enter exact spell name:");
						break;
					case mw_schoolt:
						Term_putstr(10, 10, -1, TERM_GREEN, "Please enter the exact spell name.");// and pay attention");
						//Term_putstr(10, 11, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, 11, -1, TERM_GREEN, "For example, enter:     \377GManathrust");
						Term_putstr(10, 12, -1, TERM_GREEN, "You must have learned a spell before you can use it!");
						Term_putstr(15, 16, -1, TERM_L_GREEN, "Enter exact spell name:");
						break;
					case mw_mimict:
						Term_putstr(10, 10, -1, TERM_GREEN, "Please enter the exact spell name.");// and pay attention");
						//Term_putstr(10, 11, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, 11, -1, TERM_GREEN, "For example, enter:     \377GMagic Missile");
						Term_putstr(10, 12, -1, TERM_GREEN, "You must have learned a spell before you can use it!");
						Term_putstr(15, 16, -1, TERM_L_GREEN, "Enter exact spell name:");
						break;
					case mw_mimicidx:
						Term_putstr(10, 10, -1, TERM_GREEN, "Please enter a spell number, starting from 1, which is");
						Term_putstr(10, 11, -1, TERM_GREEN, "the first spell after the 3 basic powers and immunity");
						Term_putstr(10, 12, -1, TERM_GREEN, "preference setting, which always occupy spell slots a)-d).");
						Term_putstr(10, 13, -1, TERM_GREEN, "So \377G1\377g = spell e), \377G2\377g = f), \377G3\377g = g), \377G4\377g = h) etc.");
						Term_putstr(10, 14, -1, TERM_GREEN, "You must have learned a spell before you can use it!");
						Term_putstr(15, 16, -1, TERM_L_GREEN, "Enter spell index number:");
						break;
					case mw_fight:
						Term_putstr(10, 10, -1, TERM_GREEN, "Please enter the exact technique name.");// and pay attention");
						//Term_putstr(10, 11, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, 11, -1, TERM_GREEN, "For example, enter:     \377GSprint");
						Term_putstr(10, 12, -1, TERM_GREEN, "You must have learned a technique before you can use it!");
						Term_putstr(15, 16, -1, TERM_L_GREEN, "Enter exact technique name:");
						break;
					case mw_stance:
						Term_putstr(10, 10, -1, TERM_GREEN, "Please pick a stance:");
						Term_putstr(10, 11, -1, TERM_GREEN, "  \377Ga\377g) Balanced stance (standard)");
						Term_putstr(10, 12, -1, TERM_GREEN, "  \377Gb\377g) Defensive stance");
						Term_putstr(10, 13, -1, TERM_GREEN, "  \377Gc\377g) Offensive stance");

						while (TRUE) {
							switch (choice = inkey()) {
							case ESCAPE:
							case 'p':
							case '\010': /* backspace */
								i = -1; /* leave */
								break;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????");
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
						if (i == -1) continue;

						buf[0] = choice;
						choice = mw_stance;
						break;
					case mw_shoot:
						Term_putstr(10, 10, -1, TERM_GREEN, "Please enter the exact technique name.");// and pay attention");
						//Term_putstr(10, 11, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, 11, -1, TERM_GREEN, "For example, enter:     \377GFlare Missile");
						Term_putstr(10, 12, -1, TERM_GREEN, "You must have learned a technique before you can use it!");
						Term_putstr(15, 16, -1, TERM_L_GREEN, "Enter exact technique name:");
						break;
					case mw_poly:
						Term_putstr(10, 10, -1, TERM_GREEN, "Please enter the exact monster name OR its code. (You can find");
						Term_putstr(10, 11, -1, TERM_GREEN, "codes you have already learned by pressing  \377s~ 2  \377gin the game");
						Term_putstr(10, 12, -1, TERM_GREEN, "or by pressing  \377s:  \377gto chat and then typing the command:  \377s/mon");
						Term_putstr(10, 13, -1, TERM_GREEN, "The first number on the left, in parentheses, is what you need.)");
						Term_putstr(10, 14, -1, TERM_GREEN, "For example, enter  \377GFruit bat\377g  or just  \377G37  \377gto transform into one.");
						Term_putstr(10, 15, -1, TERM_GREEN, "To return to your normal form, use  \377GPlayer\377g  or its code  \377G0\377g  .");
						//Term_putstr(10, 15, -1, TERM_GREEN, "You must have learned a form before you can use it!");
						Term_putstr(1, 17, -1, TERM_L_GREEN, "Enter exact monster name/code or leave blank:");
						should_wait = TRUE;
						break;
					case mw_prfimm:
						Term_putstr(5, 10, -1, TERM_GREEN, "Please choose an immunity preference:");
						Term_putstr(5, 11, -1, TERM_GREEN, "\377Ga\377g) Lightning  \377Gb\377g) Frost  \377Gc\377g) Acid  \377Gd\377g) Fire  \377Ge\377g) Poison  \377Gf\377g) Water");
						Term_putstr(5, 12, -1, TERM_GREEN, "\377G*\377g) None (pick one randomly on polymorphing)");
						Term_putstr(5, 13, -1, TERM_GREEN, "\377G?\377g) Just check (displays your current immunity preference)");
						Term_putstr(15, 16, -1, TERM_L_GREEN, "Pick one (a-f,*,?):");

						while (TRUE) {
							switch (choice = inkey()) {
							case ESCAPE:
							case 'p':
							case '\010': /* backspace */
								i = -1; /* leave */
								break;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????");
								continue;
							default:
								/* invalid action -> exit wizard */
								if ((choice < 'a' || choice > 'f') && choice != '?' && choice != '*') {
									//i = -1;
									continue;
								}
							}
							break;
						}
						/* exit? */
						if (i == -1) continue;

						/* build macro part */
						switch (choice) {
						case 'a': strcpy(buf2, "\\e)m@3\rd@Lightning\r"); break;
						case 'b': strcpy(buf2, "\\e)m@3\rd@Frost\r"); break;
						case 'c': strcpy(buf2, "\\e)m@3\rd@Acid\r"); break;
						case 'd': strcpy(buf2, "\\e)m@3\rd@Fire\r"); break;
						case 'e': strcpy(buf2, "\\e)m@3\rd@Poison\r"); break;
						case 'f': strcpy(buf2, "\\e)m@3\rd@Water\r"); break;
						case '*': strcpy(buf2, "\\e)m@3\rd@None\r"); break;
						case '?': strcpy(buf2, "\\e)m@3\rd@Check\r"); break;
						}

						choice = mw_prfimm; /* hack - remember */
						break;
					case mw_rune: //Hardcoded, so must be maintained. - Kurzel
						strcpy(buf2, "");
						strcpy(buf, "");
						u16b e_flags = 0;
						u16b m_flags = 0;

						/* ---------- Draw runes ---------- */

						Term_putstr(10, 10, -1, TERM_GREEN, "Please choose the component runes of the spell.");
						Term_putstr(10, 11, -1, TERM_GREEN, "ie. Select the spell runes to combine:");

						Term_putstr(4, 13, -1, TERM_GREEN, "(Select the \377Usame \377grune twice or press \377sENTER \377gto cast a single rune spell!)");

						i = 0;
						for (j = 0; j < 2; j++) {
							if (i == -1 || i == -2) continue; //invalid action -OR- drawing less than the maximum number of runes

							clear_from(14);
							//Term_putstr(10, 14, -1, TERM_GREEN, format("Selected runes: \377s%s", buf2)); //Redundant unless RCRAFT_MAX_ELEMENTS > 2
							for (i = 0; i < RCRAFT_MAX_ELEMENTS; i++) {
								/* Show duplicate rune(s) in umber (or press ENTER; allow both!) */
								if (e_flags & r_elements[i].flag) {
									Term_putstr(10, 15 + i % 6, -1, TERM_L_UMBER, format("%c) \377%c%s", 'a' + i, 'U', r_elements[i].name));
								}
								else {
									Term_putstr(10, 15 + i % 6, -1, TERM_L_GREEN, format("%c) \377%c%s", 'a' + i, 'G', r_elements[i].name));
								}
							}

							switch (choice = inkey()) {
							case '\r':
								if (j > 0) i = -2; //Are we on the second iteration? (Don't exit with 0 runes!)
								else j--;
								break;
							case ESCAPE:
							case '\010': /* backspace */
								i = -1; /* leave */
								continue;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????");
								j--;
								continue;
							default: /* invalid action -> exit wizard */
								if (choice < 'a' || choice > 'a' + RCRAFT_MAX_ELEMENTS - 1) {
									j--;
									//i = -1; //Just ignore it. (ESC to exit)
									continue;
								}

								/* add this rune.. */
								e_flags |= r_elements[choice - 'a'].flag;
								/* Enable below for old 'rune list' display - Redundant at the moment, but cool~ +_+ */
								//strcat(buf2, r_elements[choice - 'a'].name);
								//strcat(buf2, " ");
							}

							/* build macro part: 'a'..'p' or '\r' */
							strcat(buf, format("%c", choice));
						}
						if (i == -1) continue; //invalid action -> exit

						/* ---------- Select imperative ---------- */

						clear_from(10);
						Term_putstr(10, 10, -1, TERM_GREEN, "Please choose the imperative of the spell.");
						Term_putstr(10, 11, -1, TERM_GREEN, "ie. Select the spell modifier:");

						/* Print the header */
						Term_putstr(10, 13, -1, TERM_L_GREEN, "   Imperative Level Cost Fail Damage Radius Duration Energy");

						/* Analyze parameters */
						byte element[RCRAFT_MAX_ELEMENTS];
						byte elements = flags_to_elements(element, e_flags);
						/* Rune check for the restricted spells; enhanced, sign, glyph, sigil, boon. - Kurzel */
						byte projection = flags_to_projection(e_flags);
						bool has_rune = 0;
						for (i = 0; i < INVEN_PACK; i++) {
							if (inventory[i].tval == TV_RUNE && inventory[i].sval == projection) has_rune = 1;
						}
						byte skill = rspell_skill(element, elements);

						/* Fill the list */
						int color;
						char tmpbuf[80];
						for (i = 0; i < RCRAFT_MAX_IMPERATIVES; i++) {

							/* Get the line color */
							if (r_imperatives[i].level+4 < skill) color = 'G'; //Ew, hardcode the 1st spell type level-1 - Kurzel
							else color = 'D';

							/* Fill a line */
							sprintf(tmpbuf, "\377%c%c) %-10s %s%d %3d%% %s%d%% %5d%% %5s%d %7d%% %5d%%",
							    color, 'a' + i, r_imperatives[i].name,
								ABS(r_imperatives[i].level) >= 10 ? (r_imperatives[i].level >= 0 ? "  +" : "  ") : (r_imperatives[i].level >= 0 ? "   +" : "   "),
								r_imperatives[i].level, r_imperatives[i].cost * 10,
							    ABS(r_imperatives[i].fail) >= 10 ? (r_imperatives[i].fail >= 0 ? "+" : "") : (r_imperatives[i].fail >= 0 ? " +" : " "),
							    r_imperatives[i].fail, r_imperatives[i].damage * 10, r_imperatives[i].radius >= 0 ? "+" : "-", ABS(r_imperatives[i].radius),
							    r_imperatives[i].duration * 10, r_imperatives[i].energy * 10);

							/* Print the line */
							Term_putstr(10, 14 + i, -1, TERM_L_GREEN, tmpbuf);
						}

						while (TRUE) {
							switch (choice = inkey()) {
							case ESCAPE:
							case 'p':
							case '\010': /* backspace */
								i = -1; /* leave */
								break;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????");
								continue;
							default:
								/* invalid action -> exit wizard */
								if (choice < 'a' || choice > ('a'+RCRAFT_MAX_IMPERATIVES-1)) {
									//i = -1; //Just ignore it. (ESC to exit)
									continue;
								}
							}
							break;
						}
						/* exit? */
						if (i == -1) continue;

						/* Store the imperative */
						m_flags |= r_imperatives[choice - 'a'].flag;

						/* build macro part: 'a'..'h' */
						strcat(buf, format("%c", choice));

						/* ---------- Select form ---------- */

						clear_from(10);
						Term_putstr(10, 10, -1, TERM_GREEN, "Please choose the form of the spell.");
						Term_putstr(10, 11, -1, TERM_GREEN, "ie. Select the spell type:");

						/* Print the header */
						Term_putstr(10, 13, -1, TERM_L_GREEN, "   Form    Level Cost Fail Info");

						/* Check for a penalty due to status */
						u16b penalty = 0;
						if (p_ptr->blind) penalty += 10;
						//if (p_ptr->confused) penalty += 10; //#ifdef ENABLE_CONFUSED_CASTING
						if (p_ptr->stun > 50) penalty += 25;
						else if (p_ptr->stun) penalty += 15;

						/* Analyze more parameters */
						byte imperative = flags_to_imperative(m_flags);
						byte level, cost, fail;
						s16b diff, sdiff;

						/* Print the list */
						//int color;
						//char tmpbuf[80];
						for (i = 0; i < RCRAFT_MAX_TYPES; i++) {

							/* Analyze list parameters */
							level = rspell_level(imperative, i);
							diff = rspell_diff(skill, level);
							cost = rspell_cost(imperative, i, skill);
							fail = rspell_fail(imperative, i, diff, penalty);
							u32b dx, dy;
							u16b damage = rspell_damage(&dx, &dy, imperative, i, skill, projection);
							u16b dice = damroll(dx, dy);
							byte radius = rspell_radius(imperative, i, skill, projection);
							byte duration = rspell_duration(imperative, i, skill, projection, dice);

							/* Extra parameters */
							sdiff = skill - level + 1; //For a real 'level difference' display.

							/* Get the line color */
							if (diff > 0) {
								color = 'G';
								if (penalty) color = 'y';
								if (p_ptr->msp < cost) color = 'o';
								//if (p_ptr->anti_magic && r_types[i].flag != T_GLPH) color = 'r'; //#define ENABLE_SHELL_ENCHANT
								if (p_ptr->anti_magic) color = 'r';
							}
							else color = 'D';

							/* Fill a line */
							switch (r_types[i].flag) {

								case T_BOLT: { //Beam
									if (r_imperatives[imperative].flag != I_ENHA) {
										sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %dd%d",
										color, 'a' + i, r_types[i].name, sdiff, cost, fail, dx, dy);
									} else {
										sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %dd%d",
										color, 'a' + i, "beam", sdiff, cost, fail, dx, dy);
									}
								break; }

								case T_CLOU: { //Storm
									if (r_imperatives[imperative].flag != I_ENHA) {
										sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d rad %d dur %d",
										color, 'a' + i, r_types[i].name, sdiff, cost, fail, rget_level(damage), radius, duration);
									} else {
										sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d rad %d dur %d",
										color, 'a' + i, "storm", sdiff, cost, fail, rget_level(damage)*15/10, 1, duration*2);
									}
								break; }

								case T_SIGN: { //Seal
									if (!has_rune && color != 'D') color = 'R';
									if (r_imperatives[imperative].flag != I_ENHA) {
										switch (projection) {

											case SV_R_LITE: { //Illumination
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d rad %d illumination",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail, rspell_damage(&dx, &dy, imperative, flags_to_type(T_BALL), skill, projection)/4, 3+radius*2);
											break; }

											case SV_R_DARK: { //Invisibility
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dur %d+1d%d pow %d invisibility",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail, duration/2, duration/2, damage);
											break; }

											case SV_R_NEXU: { //Teleportation
												switch (r_imperatives[imperative].flag) {
													//Manual tuning; Phase Door (6-12), Blink (10), Teleport (100), Spell (100-200) - Kurzel
													case I_MINI: { radius = 12 + rget_level(12); break; }
													case I_LENG: { radius = 25 + rget_level(25); break; }
													case I_COMP: { radius =  6 + rget_level( 6); break; }
													case I_MODE: { radius = 25 + rget_level(25); break; }
													case I_EXPA: { radius = 75 + rget_level(75); break; }
													case I_BRIE: { radius = 25 + rget_level(25); break; }
													case I_MAXI: { radius = 50 + rget_level(50); break; }
												}
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% rad %d teleportation",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail, radius);
											break; }

											case SV_R_NETH: { //Annihilation
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% pow %d annihilation (bolt)",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail, 8+(damage+5)/10);
											break; }

											case SV_R_CHAO: { //Polymorph Self
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% polymorph self",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail);
											break; }

											case SV_R_MANA: { //Recharging
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% pow %d recharging",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail, damage);
											break; }

											case SV_R_CONF: { //Reflection
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dur %d+1d%d reflection",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail, duration/2, duration/2);
											break; }

											case SV_R_INER: { //Mass Stasis
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% pow %d mass stasis",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail, damage*2);
											break; }

											case SV_R_ELEC:
											case SV_R_FIRE:
											case SV_R_COLD:
											case SV_R_ACID:
											case SV_R_POIS: { //Infusion
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dur %d+1d%d %s infusion",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail, duration/2, duration/2, r_projections[projection].name);
											break; }

											case SV_R_WATE: { //Quench Thirst
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% quench thirst",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail);
											break; }

											case SV_R_GRAV: { //Mass Teleport-To
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% pow %d mass teleport-to",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail, damage*2);
											break; }

											case SV_R_SHAR: { //Dig
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dig (bolt)",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail);
											break; }

											case SV_R_SOUN: { //Disarm
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% rad %d disarm",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail, radius);
											break; }

											case SV_R_TIME: { //Haste
												damage = rget_level(15) * r_imperatives[imperative].damage / 10;
												if (damage < 1) damage = 1;
												if (damage > 10) damage = 10;
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dur %d+1d%d +%d speed",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail, duration/2, duration/2, damage);
											break; }

											case SV_R_DISE: { //Resistance
												sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dur %d+1d%d base resistance",
												color, 'a' + i, r_types[i].name, sdiff, cost, fail, duration/2, duration/2);
											break; }

											case SV_R_ICEE:
											case SV_R_PLAS: { //Shield
													damage = rget_level(20) * r_imperatives[imperative].damage / 10;
													if (damage < 1) damage = 1;
													if (damage > 20) damage = 20;
													sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %dd%d dur %d+1d%d +%d AC",
													color, 'a' + i, r_types[i].name, sdiff, cost, fail, dx, dy, duration/2, duration/2, damage);
											break; }

										}
									} else {
                    // Hack - Describe elements without an according resist! - Kurzel
                    switch (projection) {
                      case SV_R_INER: { // TR2_FREE_ACT
                        sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% free action",
                        color, 'a' + i, "seal", sdiff, cost, fail);
                      break; }
                      case SV_R_GRAV: { // TR3_FEATHER
                        sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% feather falling",
                        color, 'a' + i, "seal", sdiff, cost, fail);
                      break; }
                      case SV_R_ICEE:   // TR2_RES_COLD | TR2_RES_SHARDS
                      case SV_R_PLAS: { // TR2_RES_ELEC | TR2_RES_FIRE | TR2_RES_SOUND
                        sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% multiple resists",
                        color, 'a' + i, "seal", sdiff, cost, fail);
                      break; }
                      default: {
                        sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% %s resistance",
                        color, 'a' + i, "seal", sdiff, cost, fail, r_projections[projection].name);
                      break; }
                    }
									}
								break; }

								case T_BALL: { //Swarm
									if (r_imperatives[imperative].flag != I_ENHA) {
										sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d rad %d",
										color, 'a' + i, r_types[i].name, sdiff, cost, fail, damage, radius);
									} else {
										sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d or %d (x%d)",
										color, 'a' + i, "swarm", sdiff, cost, fail, rspell_damage(&dx, &dy, flags_to_imperative(I_MINI), flags_to_type(T_BALL), skill, projection)/2, rspell_damage(&dx, &dy, flags_to_imperative(I_MINI), flags_to_type(T_BALL), skill, projection), (((3+(sdiff-1)/10) > 3) ? (3+(sdiff-1)/10) : 3));
									}
								break; }

								case T_WAVE: { //Surge
									if (r_imperatives[imperative].flag != I_ENHA) {
										sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d (x3) rad %d",
										color, 'a' + i, r_types[i].name, sdiff, cost, fail, rget_level(damage), radius);
									} else {
										sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d dur 2",
										color, 'a' + i, "surge", sdiff, cost, fail, rget_level(damage)/2);
									}
								break; }

								case T_GLPH: { //Sigil
									if (!has_rune && color != 'D') color = 'R';
									if (r_imperatives[imperative].flag != I_ENHA) {
                    sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d",
										color, 'a' + i, "glyph", sdiff, cost, fail, rspell_damage(&dx, &dy, imperative, flags_to_type(T_BALL), skill, projection)/2);
									} else {
										sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% miscellaneous boni",
										color, 'a' + i, "sigil", sdiff, cost, fail);
									}
								break; }

								case T_BURS: { //Flare
									if (r_imperatives[imperative].flag != I_ENHA) {
										sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d rad %d",
										color, 'a' + i, r_types[i].name, sdiff, cost, fail, damage, radius);
									} else {
										sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d (33%% backlash)",
										color, 'a' + i, "flare", sdiff, cost, fail, dx*dy);
									}
								break; }

							}
							Term_putstr(10, 14 + i, -1, TERM_L_GREEN, tmpbuf);
						}

						while (TRUE) {
							switch (choice = inkey()) {
							case ESCAPE:
							case 'p':
							case '\010': /* backspace */
								i = -1; /* leave */
								break;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????");
								continue;
							default:
								/* invalid action -> exit wizard */
								if (choice < 'a' || choice > ('a'+RCRAFT_MAX_TYPES-1)) {
									//i = -1; //Just ignore it. (ESC to exit)
									continue;
								}
							}
							break;
						}
						/* exit? */
						if (i == -1) continue;

						/* Store the type */
						m_flags |= r_types[choice - 'a'].flag;

						/* build macro part: 'a'..'h' */
						strcat(buf, format("%c", choice));

						/* ---------- Determine if a direction is needed (hard-coded) ---------- */

						/* Request the Direction -- Hardcoded - Kurzel */
						if (((m_flags & T_BOLT) == T_BOLT)
						 || (((m_flags & T_CLOU) == T_CLOU) && !((m_flags & I_ENHA) == I_ENHA)) //Until (if) Storm can be targeted! - Kurzel
						 || (((m_flags & T_SIGN) == T_SIGN) && (((projection == SV_R_NETH) && ((m_flags & I_ENHA) != I_ENHA))))
						 || (((m_flags & T_SIGN) == T_SIGN) && (((projection == SV_R_SHAR) && ((m_flags & I_ENHA) != I_ENHA))))
						 || ((m_flags & T_BALL) == T_BALL)
						 || ((m_flags & T_BURS) == T_BURS))
							strcat(buf, "*t");

						/* hack before we exit: remember menu choice 'runespell' */
						choice = mw_rune;
						i = 1;
						break;
					case mw_trap:
						/* ---------- Enter trap kit name ---------- */
						Term_putstr(5, 10, -1, TERM_GREEN, "Please enter a distinctive part of the trap kit name or inscription.");
						//Term_putstr(5, 11, -1, TERM_GREEN, "and pay attention to upper-case and lower-case letters!");
						Term_putstr(5, 11, -1, TERM_GREEN, "For example, enter:     \377GCatapult");
						Term_putstr(5, 12, -1, TERM_GREEN, "if you want to use a 'Catapult Trap Kit'.");
						Term_putstr(5, 16, -1, TERM_L_GREEN, "Enter partial trap kit name or inscription:");

						/* Get an item name */
						Term_gotoxy(50, 16);
						strcpy(buf, "");
						if (!askfor_aux(buf, 159, 0)) {
							i = -1;
							continue;
						}
						strcat(buf, "\\r");

						/* ---------- Enter ammo/load name ---------- */
						clear_from(10);
						Term_putstr(5, 10, -1, TERM_GREEN, "Please enter a distinctive part of the item name or inscription you");
						Term_putstr(5, 11, -1, TERM_GREEN, "want to load the trap kit with.");//, and pay attention to upper-case");
						//Term_putstr(5, 12, -1, TERM_GREEN, "and lower-case letters!");
						Term_putstr(5, 12, -1, TERM_GREEN, "For example, enter:     \377GPebbl     \377gif you want");
						Term_putstr(5, 13, -1, TERM_GREEN, "to load a catapult trap kit with 'Rounded Pebbles'.");
						Term_putstr(5, 14, -1, TERM_GREEN, "If you want to choose ammo manually, just press the \377GRETURN\377g key.");
						Term_putstr(5, 17, -1, TERM_L_GREEN, "Enter partial ammo/load name or inscription:");

						/* Get an item name */
						Term_gotoxy(50, 17);
						strcpy(buf2, "");
						if (!askfor_aux(buf2, 159, 0)) {
							i = -1;
							continue;
						}
						/* Choose ammo manually? Terminate partial macro here. */
						if (streq(buf2, "")) break;

						strcat(buf2, "\\r");
						strcat(buf, "@");
						strcat(buf, buf2);
						break;
					case mw_device:
						Term_putstr(10, 10, -1, TERM_GREEN, "Please choose the type of magic device you want to use:");
						Term_putstr(15, 12, -1, TERM_L_GREEN, "a) a wand");
						Term_putstr(15, 13, -1, TERM_L_GREEN, "b) a staff");
						Term_putstr(15, 14, -1, TERM_L_GREEN, "c) a rod that doesn't require a target");
						Term_putstr(15, 15, -1, TERM_L_GREEN, "d) a rod that requires a target");
						Term_putstr(15, 16, -1, TERM_L_GREEN, "e) an activatable item that doesn't require a target");
						Term_putstr(15, 17, -1, TERM_L_GREEN, "f) an activatable item that requires a target");

						while (TRUE) {
							switch (choice = inkey()) {
							case ESCAPE:
							case 'p':
							case '\010': /* backspace */
								i = -1; /* leave */
								break;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????");
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
						if (i == -1) continue;

						/* build macro part */
						j = 0; /* hack: != 1 means 'undirectional' device */
						if (c_cfg.rogue_like_commands) {
							switch (choice) {
							case 'a': strcpy(buf2, "\\e)z@"); j = 1; break;
							case 'b': strcpy(buf2, "\\e)Z@"); break;
							case 'c': strcpy(buf2, "\\e)a@"); break;
							case 'd': strcpy(buf2, "\\e)a@"); j = 1; break;
							case 'e': strcpy(buf2, "\\e)A@"); break;
							case 'f': strcpy(buf2, "\\e)A@"); j = 1; break;
							}
						} else {
							switch (choice) {
							case 'a': strcpy(buf2, "\\e)a@"); j = 1; break;
							case 'b': strcpy(buf2, "\\e)u@"); break;
							case 'c': strcpy(buf2, "\\e)z@"); break;
							case 'd': strcpy(buf2, "\\e)z@"); j = 1; break;
							case 'e': strcpy(buf2, "\\e)A@"); break;
							case 'f': strcpy(buf2, "\\e)A@"); j = 1; break;
							}
						}

						/* ---------- Enter device name ---------- */

						clear_from(10);
						Term_putstr(5, 10, -1, TERM_GREEN, "Please enter a distinctive part of the magic device's name or");
						Term_putstr(5, 11, -1, TERM_GREEN, "inscription.");// and pay attention to upper-case and lower-case letters!");
						switch (choice) {
						case 'a': Term_putstr(5, 12, -1, TERM_GREEN, "For example, enter:     \377GMagic Mis");
							Term_putstr(5, 13, -1, TERM_GREEN, "if you want to use a 'Wand of Magic Missiles'.");
							break;
						case 'b': Term_putstr(5, 12, -1, TERM_GREEN, "For example, enter:     \377GTelep");
							Term_putstr(5, 13, -1, TERM_GREEN, "if you want to use a 'Staff of Teleportation'.");
							break;
						case 'c': Term_putstr(5, 12, -1, TERM_GREEN, "For example, enter:     \377GTrap Loc");
							Term_putstr(5, 13, -1, TERM_GREEN, "if you want to use a 'Rod of Trap Location'.");
							break;
						case 'd': Term_putstr(5, 12, -1, TERM_GREEN, "For example, enter:     \377GLightn");
							Term_putstr(5, 13, -1, TERM_GREEN, "if you want to use a 'Rod of Lightning Bolts'.");
							break;
						case 'e': Term_putstr(5, 12, -1, TERM_GREEN, "For example, enter:     \377GFrostw");
							Term_putstr(5, 13, -1, TERM_GREEN, "if you want to use a 'Frostwoven Cloak'.");
							break;
						case 'f': Term_putstr(5, 12, -1, TERM_GREEN, "For example, enter:     \377GSerpen");
							Term_putstr(5, 13, -1, TERM_GREEN, "if you want to use an 'Amulet of the Serpents'.");
							break;
						}
						Term_putstr(5, 16, -1, TERM_L_GREEN, "Enter partial device name or inscription:");

						/* hack before we exit: remember menu choice 'magic device' */
						choice = mw_device;

						break;

					case mw_abilitynt:
						Term_putstr(10, 10, -1, TERM_GREEN, "Please enter the exact ability name.");// and pay attention");
						//Term_putstr(10, 11, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, 11, -1, TERM_GREEN, "For example, enter:     \377GSwitch between main-hand and dual-hand");
						Term_putstr(15, 16, -1, TERM_L_GREEN, "Enter exact ability name:");
						break;
					case mw_abilityt:
						Term_putstr(10, 10, -1, TERM_GREEN, "Please enter the exact ability name.");// and pay attention");
						//Term_putstr(10, 11, -1, TERM_GREEN, "to upper-case and lower-case letters and spaces!");
						Term_putstr(10, 11, -1, TERM_GREEN, "For example, enter:     \377GBreathe element");
						Term_putstr(15, 16, -1, TERM_L_GREEN, "Enter exact ability name:");
						break;

					case mw_throw:
						Term_putstr(5, 10, -1, TERM_GREEN, "Please enter a distinctive part of the item's name or inscription.");
						//Term_putstr(5, 11, -1, TERM_GREEN, "and pay attention to upper-case and lower-case letters!");
						Term_putstr(5, 11, -1, TERM_GREEN, "For example, enter:     \377G{bad}");
						Term_putstr(5, 12, -1, TERM_GREEN, "if you want to throw any item that is inscribed '{bad}'.");
						Term_putstr(5, 13, -1, TERM_GREEN, "(That can for example give otherwise useless potions some use..)");
						Term_putstr(5, 16, -1, TERM_L_GREEN, "Enter partial item name or inscription:");
						break;

					case mw_custom:
						Term_putstr(5, 10, -1, TERM_GREEN, "Please enter the custom macro action string.");
						Term_putstr(5, 11, -1, TERM_GREEN, "(You have to specify everything manually here, and won't get");
						Term_putstr(5, 12, -1, TERM_GREEN, "prompted about a targetting method or anything else either.)");
						Term_putstr(5, 16, -1, TERM_L_GREEN, "Enter a new action:");
						break;

					case mw_load:
						Term_putstr(5, 10, -1, TERM_GREEN, "Please enter the macro file name.");
						Term_putstr(5, 11, -1, TERM_GREEN, "If you are on Linux or OSX it is case-sensitive! On Windows it is not.");
						Term_putstr(5, 12, -1, TERM_GREEN, format("For example, enter:     \377G%s.prf", cname));
						Term_putstr(5, 16, -1, TERM_L_GREEN, "Exact file name:");
						break;

					case mw_equip:
						Term_putstr(5, 10, -1, TERM_GREEN, "Do you want to wear/wield, take off or swap an item?");
						Term_putstr(5, 11, -1, TERM_L_GREEN, "    w\377g) primary wear/wield");
						Term_putstr(5, 12, -1, TERM_L_GREEN, "    W\377g) secondary wear/wield");
						Term_putstr(5, 13, -1, TERM_L_GREEN, "    t\377g) take off an item");
						Term_putstr(5, 14, -1, TERM_L_GREEN, "    x\377g) swap item(s)");
						Term_putstr(5, 16, -1, TERM_GREEN, "Note: This macro depends on your current 'rogue_like_commands' option");
						Term_putstr(5, 17, -1, TERM_GREEN, "      setting and will not work anymore if you change the keymap.");

						while (TRUE) {
							switch (choice = inkey()) {
							case ESCAPE:
							case 'p':
							case '\010': /* backspace */
								i = -1; /* leave */
								break;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????");
								continue;
							case 'w':
							case 'W':
							case 't':
							case 'x':
								break;
							default:
								continue;
							}
							break;
						}
						/* exit? */
						if (i == -1) continue;

						clear_from(10);
						Term_putstr(5, 10, -1, TERM_GREEN, "Please enter a distinctive part of the item's name or inscription.");
						Term_putstr(5, 16, -1, TERM_L_GREEN, "Enter partial item name or inscription:");

						j = choice;
						choice = mw_equip;
						should_wait = TRUE;
						break;
					}

					/* --------------- specify item/parm if required --------------- */

					/* might input a really long line? */
					if (choice == mw_custom) {
						Term_gotoxy(5, 17);

						/* Get an item/spell name */
						strcpy(buf, "");
						if (!askfor_aux(buf, 159, 0)) {
							i = -1;
							continue;
						}
					}
					/* mw_mimicidx is special: it requires a number (1..n) */
					else if (choice == mw_mimicidx) {
						while (TRUE) {
							/* Get power slot */
							Term_gotoxy(47, 16);
							strcpy(buf, "");
							if (!askfor_aux(buf, 159, 0)) {
								i = -1;
								break;
							}
							/* not a number/invalid? retry (we have slots d)..z)) */
							if (atoi(buf) < 1 || atoi(buf) > 26 - 3) continue;

							/* ok (1..23) - translate into spell slot */
							strcpy(buf, format("%c", 'd' + atoi(buf)));
							break;
						}
						if (i == -1) continue;
					}
					/* no need for inputting an item/spell to use with the macro? */
					else if (choice != mw_fire && choice != mw_rune && choice != mw_trap && choice != mw_prfimm && choice != mw_stance) {
						if (choice == mw_load) Term_gotoxy(23, 16);
						else if (choice == mw_poly) Term_gotoxy(47, 17);
						else Term_gotoxy(47, 16);

						/* Get an item/spell name */
						strcpy(buf, "");
						if (!askfor_aux(buf, 159, 0)) {
							i = -1;
							continue;
						}

						/* allow 'empty' polymorph-into macro that prompts for form */
						if (choice != mw_poly || buf[0]) strcat(buf, "\\r");
					}

					/* --------------- complete the macro by glueing premade part and default part together --------------- */

					/* generate the full macro action; magic device/preferred immunity macros are already pre-made */
					if (choice != mw_device && choice != mw_prfimm && choice != mw_custom) {
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
								buf2[3] = KTRL('W');
								break;
							case 't':
								buf2[3] = 'T';
								break;
							case 'x':
								buf2[3] = KTRL('A');
								break;
							}
						buf2[4] = '@';
						strcpy(buf2 + 5, buf);
						break;
					}

					/* Convert the targetting method from XXX*t to *tXXX- ? */
#ifdef MACRO_WIZARD_SMART_TARGET
					/* ask about replacing '*t' vs '-' (4.4.6) vs '+' (4.4.6b) */
					if (strstr(buf2, "*t") && choice != mw_mimicidx
					    && choice != mw_load /* (paranoia) */
					    && choice != mw_custom
					    ) {
						clear_from(8);
						Term_putstr(10, 8, -1, TERM_GREEN, "Please choose the targetting method:");

						//Term_putstr(10, 11, -1, TERM_GREEN, "(\377UHINT: \377gAlso inscribe your ammo '!=' for auto-pickup!)");
						Term_putstr(10, 10, -1, TERM_L_GREEN, "a) Target closest monster if such exists,");
						Term_putstr(10, 11, -1, TERM_L_GREEN, "   otherwise cancel action. (\377URecommended in most cases!\377G)");
						Term_putstr(10, 12, -1, TERM_L_GREEN, "b) Target closest monster if such exists,");
						Term_putstr(10, 13, -1, TERM_L_GREEN, "   otherwise prompt for direction.");
						Term_putstr(10, 14, -1, TERM_L_GREEN, "c) Target closest monster if such exists,");
						Term_putstr(10, 15, -1, TERM_L_GREEN, "   otherwise target own grid.");
						Term_putstr(10, 16, -1, TERM_L_GREEN, "d) Fire into a fixed direction or prompt for direction.");
						Term_putstr(10, 17, -1, TERM_L_GREEN, "e) Target own grid (ie yourself).");

						Term_putstr(10, 19, -1, TERM_L_GREEN, "f) Target most wounded friendly player,");
						Term_putstr(10, 20, -1, TERM_L_GREEN, "   cancel action if no player is nearby. (\377UEg for 'Cure Wounds'.\377G)");
						Term_putstr(10, 21, -1, TERM_L_GREEN, "g) Target most wounded friendly player,");
						Term_putstr(10, 22, -1, TERM_L_GREEN, "   target own grid instead if no player is nearby.");

						while (TRUE) {
							switch (choice = inkey()) {
							case ESCAPE:
							case 'p':
							case '\010': /* backspace */
								i = -1; /* leave */
								break;
							case KTRL('T'):
								/* Take a screenshot */
								xhtml_screenshot("screenshot????");
								continue;
							default:
								/* invalid action -> exit wizard */
								if (choice < 'a' || choice > 'g') {
									//i = -1;
									continue;
								}
							}
							break;
						}
						/* exit? */
						if (i == -1) continue;

						/* Get a specific fixed direction */
						if (choice == 'd') {
							clear_from(8);
							Term_putstr(10, 10, -1, TERM_GREEN, "Please pick the specific, fixed direction or '?':");

							Term_putstr(25, 13, -1, TERM_L_GREEN, " 7  8  9");
							Term_putstr(25, 14, -1, TERM_GREEN, "  \\ | / ");
							Term_putstr(25, 15, -1, TERM_L_GREEN, "4 \377g-\377G 5 \377g-\377G 6");
							Term_putstr(25, 16, -1, TERM_GREEN, "  / | \\         \377G?\377g = 'Prompt for direction each time'");
							Term_putstr(25, 17, -1, TERM_L_GREEN, " 1  2  3");

							Term_putstr(15, 20, -1, TERM_L_GREEN, "Your choice? (1 to 9, or '?') ");

							/* hack: temporarily enable macro parsing for using numpad keys without numlock to specify a direction */
							inkey_interact_macros = FALSE;
							while (TRUE) {
								switch (target_dir = inkey()) {
								case ESCAPE:
								case 'p':
								case '\010': /* backspace */
									i = -1; /* leave */
									break;
								case KTRL('T'):
									/* Take a screenshot */
									xhtml_screenshot("screenshot????");
									continue;
								default:
									/* invalid action -> exit wizard */
									if ((target_dir < '1' || target_dir > '9') && target_dir != '?') {
										//i = -1;
										continue;
									}
								}
								break;
							}
							/* disable macro parsing again */
							inkey_interact_macros = TRUE;
							/* exit? */
							if (i == -1) continue;
						}

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
								if (target_dir != '?') strcat(buf, format("%c", target_dir));
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
					strcat(chain_macro_buf, macro__buf);
					strcpy(macro__buf, chain_macro_buf);

					/* advance to next step */
					i++;
					break;
				case 2:
					Term_putstr(10, 9, -1, TERM_GREEN, "In this final step, press the key you would like");
					Term_putstr(10, 10, -1, TERM_GREEN, "to bind the macro to. (ESC and % key cannot be used.)");
					Term_putstr(10, 11, -1, TERM_GREEN, "You should use keys that have no other purpose!");
					Term_putstr(10, 12, -1, TERM_GREEN, "Good examples are the F-keys, F1 to F12 and unused number pad keys.");
					//Term_putstr(10, 14, -1, TERM_GREEN, "The keys ESC and '%' are NOT allowed to be used.");
					Term_putstr(10, 14, -1, TERM_GREEN, "Most keys can be combined with \377USHIFT\377g, \377UALT\377g or \377UCTRL\377g modifiers!");
					Term_putstr(10, 16, -1, TERM_GREEN, "If you want to \377Uchain another macro\377g, press '\377U%\377g' key.");
					Term_putstr(10, 17, -1, TERM_GREEN, "By doing this you can combine multiple macros into one hotkey.");
					Term_putstr(5, 19, -1, TERM_L_GREEN, "Press the key to bind the macro to, or '%' for chaining: ");

					while (TRUE) {
						/* Get a macro trigger */
						Term_putstr(67, 19, -1, TERM_WHITE, "  ");//45
						Term_gotoxy(67, 19);
						get_macro_trigger(buf);

						/* choose proper macro type, and bind it to key */
#if 0 /* no, because we allow remapping of this key too, here */
						if (!strcmp(buf, format("%c", KTRL('T')))) {
							/* Take a screenshot */
							xhtml_screenshot("screenshot????");
							continue;
						} else
#endif
						if (!strcmp(buf, "\e")) {
							c_msg_print("\377yKeys <ESC> and '%' aren't allowed to carry a macro.");
							if (!strcmp(buf, "\e")) {
								i = -1; /* leave */
								break;
							}
							continue;
						} else if (!strcmp(buf, "%")) {
							char tmp[1024];
							int delay;

							/* max length check, rough estimate */
							if (strlen(chain_macro_buf) + strlen(macro__buf) >= 1024 - 50) {
								c_msg_print("\377oYou cannot chain any further commands to this macro.");
								continue;
							}

							/* Some macros require a latency-based delay in order to update the client with
							   necessary reply information from the server before another command based on
							   this action might work.
							   Example: Wield an item, then activate it. The activation part needs to wait
							   until the server tells the client that the item has been successfully equipped.
							   Example: Polymorph into a form that has a certain spell available to cast.
							   The casting needs to wait until the server tells us that we successfully polymorphed. */
							if (should_wait) {
								clear_from(19);
								Term_putstr(10, 19, -1, TERM_YELLOW, "This macro might need a latency-based delay to work properly!");
								Term_putstr(10, 20, -1, TERM_YELLOW, "You can accept the suggested delay or modify it in steps");
								Term_putstr(10, 21, -1, TERM_YELLOW, "of 100 ms up to 9900 ms, or hit ESC to not use a delay.");
								Term_putstr(10, 23, -1, TERM_L_GREEN, "ENTER\377g to accept, \377GESC\377g to discard (in ms):");

								/* suggest +25% reserve tolerance but at least +25 ms on the ping time */
								sprintf(tmp, "%d", ((ping_avg < 100 ? ping_avg + 25 : (ping_avg * 125) / 100) / 100 + 1) * 100);
								Term_gotoxy(52, 23);
								if (askfor_aux(tmp, 50, 0)) {
									delay = atoi(tmp);
									if (delay % 100) delay += 100; //QoL hack for noobs who can't read
									delay /= 100;
									if (delay < 0) delay = 0;
									if (delay > 99) delay = 99;

									if (delay) {
										sprintf(tmp, "\\w%c%c", '0' + delay / 10, '0' + delay % 10);
										text_to_ascii(macro__buf, tmp);
										strcat(chain_macro_buf, macro__buf);
										strcpy(macro__buf, chain_macro_buf);
									}
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
					if (i == -1) continue;

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

	/* Flush the queue */
	Flush_queue();

	inkey_interact_macros = FALSE;

	/* in case we entered this menu from recording a macro,
	   we might have to update the '*recording*' status line */
	if (were_recording) Send_redraw(0);
}


void auto_inscriptions(void) {
	int i, cur_line = 0;
	bool redraw = TRUE, quit = FALSE;

	char tmp[160], buf[1024], *buf_ptr;
	char match_buf[80], tag_buf[40];

	char fff[1024];

	/* Save screen */
	Term_save();

	/* Prevent hybrid macros from triggering in here */
	inkey_msg = TRUE;

	/* Process requests until done */
	while (1) {
		if (redraw) {
			/* Clear screen */
			Term_clear();

			/* Describe */
			Term_putstr(20,  0, -1, TERM_L_UMBER, "*** Current Auto-Inscriptions List ***");
			Term_putstr(5, 21, -1, TERM_L_UMBER, "[Press 'n' for next, 'p' for previous, 'P' to chat-paste, ESC to exit]");
			Term_putstr(12, 22, -1, TERM_L_UMBER, "(l/s) Load/save auto-inscriptions from/to an '.ins' file");
			Term_putstr(4, 23, -1, TERM_L_UMBER, "(e/d/c) Edit current ('#' wildcard, '!' forces)/delete current/CLEAR ALL");

			for (i = 0; i < MAX_AUTO_INSCRIPTIONS; i++) {
				/* build a whole line */
				strcpy(match_buf, "\377s<\377w");
				strcat(match_buf, auto_inscription_match[i]);
				strcat(match_buf, "\377s>");
				strcpy(tag_buf, "\377y");
				strcat(tag_buf, auto_inscription_tag[i]);
				sprintf(fff, "%2d) %-46s      <%s\377s>", i + 1, match_buf, tag_buf);

				Term_putstr(5, i + 1, -1, TERM_WHITE, fff);
			}
		}
		redraw = TRUE;

		/* display editing 'cursor' */
		Term_putstr(1, cur_line + 1, -1, TERM_ORANGE, ">>>");

		/* make cursor invisible */
		Term_set_cursor(0);
		inkey_flag = TRUE;

		/* Wait for keypress */
		switch (inkey()) {
		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????");
			redraw = FALSE;
			break;
		case ':':
			/* Allow to chat, to tell exact inscription-related stuff to other people easily */
			cmd_message();
			break;
		case 'P':
			/* Paste currently selected entry to chat */
			strcpy(match_buf, "\377s<\377w");
			strcat(match_buf, auto_inscription_match[cur_line]);
			strcat(match_buf, "\377s>");
			strcpy(tag_buf, "\377y");
			strcat(tag_buf, auto_inscription_tag[cur_line]);
			sprintf(tmp, "\377sAuto-inscription %2d: %s <%s\377s>", cur_line + 1, match_buf, tag_buf);
			Send_paste_msg(tmp);
			redraw = FALSE;
			break;
		case ESCAPE:
			quit = TRUE; /* hack to leave loop */
			break;
		case 'n':
		case '2':
			Term_putstr(1, cur_line + 1, -1, TERM_ORANGE, "   ");
			cur_line++;
			if (cur_line >= 20) cur_line = 0;
			redraw = FALSE;
			break;
		case 'p':
		case '8':
			Term_putstr(1, cur_line + 1, -1, TERM_ORANGE, "   ");
			cur_line--;
			if (cur_line < 0) cur_line = 19;
			redraw = FALSE;
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
			load_auto_inscriptions(tmp);
			for (i = 0; i <= INVEN_TOTAL; i++) apply_auto_inscriptions(i, FALSE);
			break;
		case 's':
			/* Prompt */
			clear_from(21);
			Term_putstr(0, 22, -1, TERM_L_GREEN, "*** Save an .ins file ***");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 23, -1, TERM_WHITE, "File: ");

			sprintf(tmp, "%s.ins", cname);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0)) continue;

			/* Dump the macros */
			save_auto_inscriptions(tmp);
			break;
		case 'e':
		case '6':
		case '\n':
		case '\r':
			/* Clear previous matching string */
			Term_putstr(9, cur_line + 1, -1, TERM_L_GREEN, "                                         ");
			/* Go to the correct location */
			Term_gotoxy(10, cur_line + 1);
			strcpy(buf, auto_inscription_match[cur_line]);
			/* Get a new matching string */
			if (!askfor_aux(buf, 40, 0)) continue;
			/* hack: remove leading/trailing wild cards since they are obsolete.
			   Especially trailing ones currently make it not work. */
			buf_ptr = buf;
			while (*buf_ptr == '#') buf_ptr++;
			while (*(buf_ptr + strlen(buf_ptr) - 1) == '#') *(buf_ptr + strlen(buf_ptr) - 1) = '\0';
			Term_putstr(9, cur_line + 1, -1, TERM_L_GREEN, "                                         ");
			Term_putstr(9, cur_line + 1, -1, TERM_WHITE, buf_ptr);
			/* ok */
			strcpy(auto_inscription_match[cur_line], buf_ptr);

			/* Clear previous tag string */
			Term_putstr(55, cur_line + 1, -1, TERM_L_GREEN, "                    ");
			/* Go to the correct location */
			Term_gotoxy(55, cur_line + 1);
			strcpy(buf, auto_inscription_tag[cur_line]);
			/* Get a new tag string */
			if (!askfor_aux(buf, 20, 0)) {
				/* in case match was changed, we may also need to reapply */
				for (i = 0; i <= INVEN_TOTAL; i++) apply_auto_inscriptions(i, FALSE);
				continue;
			}
			strcpy(auto_inscription_tag[cur_line], buf);
			for (i = 0; i <= INVEN_TOTAL; i++) apply_auto_inscriptions(i, FALSE);

			/* comfort hack - fake advancing ;) */
			Term_putstr(1, cur_line + 1, -1, TERM_ORANGE, "   ");
			cur_line++;
			if (cur_line >= 20) cur_line = 0;
			break;
		case 'd':
		case '\b':
			strcpy(auto_inscription_match[cur_line], "");
			strcpy(auto_inscription_tag[cur_line], "");
#if 0
			/* also auto-advance by 1 line, for convenience */
			cur_line++;
			if (cur_line >= 20) cur_line = 0;
#endif
			break;
		case 'c':
			for (i = 0; i < MAX_AUTO_INSCRIPTIONS; i++) {
				strcpy(auto_inscription_match[i], "");
				strcpy(auto_inscription_tag[i], "");
			}
			/* comfort hack - jump to first line */
			Term_putstr(1, cur_line + 1, -1, TERM_ORANGE, "   ");
			cur_line = 0;
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
}


/*
 * Interact with some options
 */
static void do_cmd_options_aux(int page, cptr info) {
	char	ch;
	int	i, k = 0, n = 0;
	int	opt[24];
	char	buf[256];
	bool	tmp;


	/* Lookup the options */
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
		/* Prompt XXX XXX XXX */
		sprintf(buf, "%s (\377yUp/Down\377w, \377yy\377w/\377yn\377w/\377yLeft\377w/\377yRight\377w set, \377yt\377w/\377yA\377w-\377yZ\377w\377w toggle, \377yESC\377w accept)", info);
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

			/* Display the option text */
			sprintf(buf, "%-49s: %s  (%s)",
			        option_info[opt[i]].o_desc,
			        (*option_info[opt[i]].o_var ? "yes" : "no "),
			        option_info[opt[i]].o_text);
			c_prt(a, buf, i + 2, 0);
		}

		/* Hilite current option */
		move_cursor(k + 2, 50);

		/* Get a key */
		ch = inkey();

		/* Analyze */
		switch (ch) {
			case ESCAPE:
			case KTRL('Q'):
				return;

			case KTRL('T'):
				/* Take a screenshot */
				xhtml_screenshot("screenshot????");
				break;

			case ':':
				/* specialty: allow chatting from within here */
				cmd_message();
				break;

			case '-':
			case '8':
			case 'k':
				k = (n + k - 1) % n;
				break;

			case ' ':
			case '\n':
			case '\r':
			case '2':
			case 'j':
				k = (k + 1) % n;
				break;

			case 'y':
			//case 'Y':
			case '6':
			case 'l':
				(*option_info[opt[k]].o_var) = TRUE;
				Client_setup.options[opt[k]] = TRUE;
				check_immediate_options(opt[k], TRUE, TRUE);
				k = (k + 1) % n;
				break;

			case 'n':
			//case 'N':
			case '4':
			case 'h':
				(*option_info[opt[k]].o_var) = FALSE;
				Client_setup.options[opt[k]] = FALSE;
				check_immediate_options(opt[k], FALSE, TRUE);
				k = (k + 1) % n;
				break;

			case 't':
			//case 'T':
			case '5':
			case 's':
				tmp = 1 - (*option_info[opt[k]].o_var);
				(*option_info[opt[k]].o_var) = tmp;
				Client_setup.options[opt[k]] = tmp;
				check_immediate_options(opt[k], tmp, TRUE);
				k = (k + 1) % n;
				break;
			default:
				/* directly toggle a specific option */
				if (ch >= 'A' && ch <= 'A' + n - 1) {
					k = ch - 'A';
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
	if (acc_opt_screen) {
		if (acc_got_info) {
			if (acc_flags & ACC_TRIAL) {
				c_prt(TERM_YELLOW, "Your account hasn't been validated.", 3, 2);
			} else {
				c_prt(TERM_L_GREEN, "Your account is valid.", 3, 2);
			}
		} else {
			c_prt(TERM_SLATE, "Retrieving data...", 3, 2);
		}
	}
}


/*
 * Account options
 */
static void do_cmd_options_acc(void) {
	char ch;
	bool change_pass = FALSE;
	bool go = TRUE;
	char old_pass[16], new_pass[16], con_pass[16];
	char tmp[16];

	acc_opt_screen = TRUE;
	acc_got_info = FALSE;

	/* suppress hybrid macros */
	bool inkey_msg_old = inkey_msg;
	inkey_msg = TRUE;

	/* Get the account info */
	Send_account_info();

	/* Clear screen */
	Term_clear();

	while (go) {
		/* Clear everything except the topline */
		clear_from(1);

		if (change_pass) {
			prt("Change password", 1, 0);
			prt("Maximum length is 15 characters", 3, 2);
			c_prt(TERM_L_WHITE, "Old password:", 6, 2);
			c_prt(TERM_L_WHITE, "New password:", 8, 2);
			c_prt(TERM_L_WHITE, "Confirm:", 10, 2);
			c_prt(TERM_L_WHITE, "(by typing the new password once more)", 11, 3);

			/* Ask for old password */
			move_cursor(6, 16);
			tmp[0] = '\0';
			if (!askfor_aux(tmp, 15, ASKFOR_PRIVATE)) {
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
				if (!askfor_aux(tmp, 15, ASKFOR_PRIVATE)) {
					change_pass = FALSE;
					break;
				}
				my_memfrob(tmp, strlen(tmp));
				strncpy(new_pass, tmp, sizeof(new_pass));
				new_pass[sizeof(new_pass) - 1] = '\0';

				/* Ask for the confirmation */
				move_cursor(10, 16);
				tmp[0] = '\0';
				if (!askfor_aux(tmp, 15, ASKFOR_PRIVATE)) {
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
				c_msg_print("\377gPassword change has been submitted.");
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

	u32b old_flag[8];


	/* Memorize old flags */
	for (j = 0; j < ANGBAND_TERM_MAX; j++) {
		/* Acquire current flags */
		old_flag[j] = window_flag[j];
	}


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
			Term_putstr(35 + j * 5 - strlen(s) / 2, vertikal_offset + j % 2, -1, a, (char*)s);
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
			for (j = 1; j < ANGBAND_TERM_MAX; j++)
			{
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
				Term_putch(35 + j * 5, i + vertikal_offset + 2, a, c);
			}
		}

		/* Place Cursor */
		Term_gotoxy(35 + x * 5, y + vertikal_offset + 2);

		/* Get key */
		ch = inkey();

		/* Analyze */
		switch (ch) {
			case ESCAPE:
				go = FALSE;
				break;

			case KTRL('T'):
				/* Take a screenshot */
				xhtml_screenshot("screenshot????");
				break;

			/* specialty: allow chatting from within here */
			case ':':
				cmd_message();
				break;

			case 'T':
			case 't':
				/* Clear windows */
				for (j = 1; j < ANGBAND_TERM_MAX; j++)
					window_flag[j] &= ~(1L << y);

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
				p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER | PW_MSGNOCHAT | PW_MESSAGE | PW_CHAT | PW_MINIMAP);//PW_LAGOMETER is called automatically, no need.
				window_stuff();
				break;

			case 'n':
			case 'N':
				/* Clear flag */
				window_flag[x] &= ~(1L << y);

				/* Update windows */
				p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER | PW_MSGNOCHAT | PW_MESSAGE | PW_CHAT | PW_MINIMAP);//PW_LAGOMETER is called automatically, no need.
				window_stuff();
				break;

			case '\r':
				/* Toggle flag */
				window_flag[x] ^= (1L << y);

				/* Update windows */
				p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER | PW_MSGNOCHAT | PW_MESSAGE | PW_CHAT | PW_MINIMAP);//PW_LAGOMETER is called automatically, no need.
				window_stuff();
				break;

			default:
				d = keymap_dirs[ch & 0x7F];

				x = (x + ddx[d] + 6) % 7 + 1;
				y = (y + ddy[d] + NR_OPTIONS_SHOWN) % NR_OPTIONS_SHOWN;

				if (!d) bell();
		}
	}

	/* Notice changes */
	for (j = 0; j < ANGBAND_TERM_MAX; j++) {
		term *old = Term;

		/* Dead window */
		if (!ang_term[j]) continue;

		/* Ignore non-changes */
		if (window_flag[j] == old_flag[j]) continue;

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
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER | PW_MSGNOCHAT | PW_MESSAGE | PW_CHAT | PW_MINIMAP);//PW_LAGOMETER is called automatically, no need.
	window_stuff();
}

#ifdef ENABLE_SUBWINDOW_MENU
 #if defined(WINDOWS) || defined(USE_X11)
#define MAX_FONTS 50
  #ifdef WINDOWS

static int font_name_cmp(const void *a, const void *b) {
   #if 0 /* simple way */
	return strcmp((const char*)a, (const char*)b);
   #else /* sort in single-digit numbers before double-digit ones */
	char at[256], bt[256];
	at[0] = '0';
	bt[0] = '0';
	if (atoi((char*)a) < 10) strcpy(at + 1, (char *)a);
	else strcpy(at, (char *)a);
	if (atoi((char*)b) < 10) strcpy(bt + 1, (char *)b);
	else strcpy(bt, (char *)b);
	return strcmp(at, bt);
   #endif
}
  #endif

static void do_cmd_options_fonts(void) {
	int j, d, vertikal_offset = 3;
	int y = 0;
	char ch;
	bool go = TRUE, inkey_msg_old;

	char font_name[MAX_FONTS][256], path[1024];
	int fonts = 0;
	char tmp_name[256];
	char graphic_font_name[MAX_FONTS][256];
	int graphic_fonts=0;

  #ifndef WINDOWS
	int x11_refresh = 50;
	FILE *fff;
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
		while(tmp_name[++j]) tmp_name[j] = tolower(tmp_name[j]);
		if (strstr(tmp_name, ".fon")) {
			if (tmp_name[0] == 'g') {
				strcpy(graphic_font_name[graphic_fonts], ent->d_name);
				graphic_fonts++;
			} else {
				strcpy(font_name[fonts], ent->d_name);
				fonts++;
			}
		}
	}
	closedir(dir);
  #endif

  #ifdef USE_X11 /* Linux/OSX use at least the basic system fonts (/usr/share/fonts/misc) - C. Blue */
	/* Get bitmap font folder ('misc') and scan fonts.alias in it for basic bitmap fonts:
	   'cat `xset q | grep -o "[/a-z]*misc"`/fonts.alias | grep -o "^[0-9][0-9]*x[0-9]*\(bold\)\? " | grep -o "[^ ]*"' <- note the trailing space! */
	j = system("cat `xset q | grep -o \"[/a-z]*misc\"`/fonts.alias | grep -o \"^[0-9][0-9]*x[0-9]*\\(bold\\)\\? \" | grep -o \"[^ ]*\" > /tmp/tomenet-fonts.tmp");
	fff = fopen("/tmp/tomenet-fonts.tmp", "r");
	if (fff) {
		while (fonts < MAX_FONTS) {
			if (!fgets(tmp_name, 256, fff)) break;
			tmp_name[strlen(tmp_name) - 1] = 0; //remove trailing \n
			strcpy(font_name[fonts], tmp_name);
			fonts++;
		}
		remove("/tmp/tomenet-fonts.tmp"); //clean up
	} else {
		/* We boldly assume that these exist by default somehow! - They probably don't though,
		   if the above command failed, but what else can we try at this point.. #despair */
		strcpy(font_name[0], "5x8");
		strcpy(font_name[1], "6x9");
		strcpy(font_name[2], "8x13");
		strcpy(font_name[3], "9x15");
		strcpy(font_name[4], "10x20");
		strcpy(font_name[5], "12x24");
		fonts = 6;
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
			while (*cp == ' ') {
				*cp = 0;
				cp--;
			}

			if (!tmp_name[0]) continue; //skip empty lines
			if (tmp_name[0] == '#') continue; //skip commented out lines ('#' character at the beginning)

			strcpy(font_name[fonts], tmp_name);
			fonts++;
		}
		fclose(fff);
	}
  #endif

	if (!fonts) {
		c_msg_format("No .fon font files found in directory (%s).", path);
		return;
	}

  #ifdef WINDOWS /* actually never sort fonts on X11, because they come in a sorted manner from fonts.alias and fonts.txt files already. */
	qsort(font_name, fonts, sizeof(char[256]), font_name_cmp);
   #ifdef WINDOWS /* Windows supports graphic fonts for the mini map */
	qsort(graphic_font_name, graphic_fonts, sizeof(char[256]), font_name_cmp);
   #endif
  #endif

  #ifdef WINDOWS /* windows client currently saves full paths (todo: just change to filename only) */
	for (j = 0; j < fonts; j++) {
		strcpy(tmp_name, font_name[j]);
		//path_build(font_name[j], 1024, path, font_name[j]);
		strcpy(font_name[j], ".\\");
		strcat(font_name[j], path);
		strcat(font_name[j], "\\");
		strcat(font_name[j], tmp_name);
	}
	for (j = 0; j < graphic_fonts; j++) {
		strcpy(tmp_name, graphic_font_name[j]);
		strcpy(graphic_font_name[j], ".\\");
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
		Term_putstr(0, 0, -1, TERM_WHITE, "  (<\377ydir\377w>, \377yv\377w (visibility), \377y-\377w/\377y+\377w (smaller/bigger), \377yENTER\377w (enter font name), \377yESC\377w)");
		Term_putstr(0, 1, -1, TERM_WHITE, format("  (%d fonts available)", fonts));

		/* Display the windows */
		for (j = 0; j < ANGBAND_TERM_MAX; j++) {
			byte a = TERM_WHITE;
			cptr s = ang_term_name[j];
			char buf[256];

			/* Use color */
			if (c_cfg.use_color && (j == y)) a = TERM_L_BLUE;

			/* Window name, staggered, centered */
			Term_putstr(1, vertikal_offset + j, -1, a, (char*)s);

			/* Display the font of this window */
			if (c_cfg.use_color && !term_get_visibility(j)) a = TERM_L_DARK;
			strcpy(buf, get_font_name(j));
			buf[59] = 0;
			while(strlen(buf) < 59) strcat(buf, " ");
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
			xhtml_screenshot("screenshot????");
			break;
		case ':':
			/* specialty: allow chatting from within here */
			cmd_message();
			break;

		case 'v':
			if (y == 0) break; /* main window cannot be invisible */
			term_toggle_visibility(y);
			Term_putstr(0, 15, -1, TERM_YELLOW, "-- Changes to window visibilities require a restart of the client --");
			break;

		case '+':
			/* find out which of the fonts in lib/xtra/fonts we're currently using */
			if ((window_flag[y] & PW_MINIMAP) && graphic_fonts > 0)
			{
				//Include the graphic fonts, because we are cycling the mini-map
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

		case '-':
			/* find out which of the fonts in lib/xtra/fonts we're currently using */
			if ((window_flag[y] & PW_MINIMAP) && graphic_fonts > 0)
			{
				//Include the graphic fonts, because we are cycling the mini-map
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

		default:
			d = keymap_dirs[ch & 0x7F];
			y = (y + ddy[d] + NR_OPTIONS_SHOWN) % NR_OPTIONS_SHOWN;
			if (!d) bell();
		}
	}

	/* restore responsiveness to hybrid macros */
	inkey_msg = inkey_msg_old;
}
 #endif /* WINDOWS || USE_X11 */
#endif /* ENABLE_SUBWINDOW_MENU */

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
	if (!fff) return (-1);


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
	for (i = 1; i < ANGBAND_TERM_MAX; i++)
	{
		/* Require a real window */
		if (!ang_term_name[i]) continue;

		/* Check each flag */
		for (j = 0; j < NR_OPTIONS_SHOWN; j++)
		{
			/* Require a real flag */
			if (!window_flag_desc[j]) continue;

			/* Dump the flag */
			if (window_flag[i] & (1L << j))
			{
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
	return (0);
}


/* Attempt to find sound+music pack 7z files in the client's root folder
   and to install them properly. - C. Blue
   Notes: Uses the GUI version of 7z, 7zG. Reason is that a password prompt
          might be required. That's why non-X11 (ie command-line clients)
          are currently not supported. */

#ifdef WINDOWS
 #include <winreg.h>	/* remote control of installed 7-zip via registry approach */
 #include <process.h>	/* use spawn() instead of normal system() (WINE bug/Win inconsistency even maybe) */
 #define MAX_KEY_LENGTH 255
 #define MAX_VALUE_NAME 16383
#endif

static void do_cmd_options_install_audio_packs(void) {
	FILE *fff;
	char path[1024], out_val[1024+28];
	char c, ch, pack_name[1024];
	int r;
	bool picked = FALSE;

#ifdef WINDOWS /* use windows registry to locate 7-zip */
	HKEY hTestKey;
	char path_7z[1024], path_7z_quoted[1024];
	LPBYTE path_7z_p = (LPBYTE)path_7z;
	int path_7z_size = 1023;
	LPDWORD path_7z_size_p = (LPDWORD)&path_7z_size;
	unsigned long path_7z_type = REG_SZ;
#endif

	bool sound_pack = FALSE, music_pack = FALSE;
	//bool sound_already = (audio_sfx > 3), music_already = (audio_music > 0);
	bool password_ok = TRUE;

	/* Clear screen */
	Term_clear();
	Term_putstr(0, 0, -1, TERM_WHITE, "Install a sound or music pack from \377y7z\377w files within your \377yTomeNET\377w folder...");
	Term_fresh();
	if (quiet_mode) {
		Term_putstr(0, 1, -1, TERM_RED, "Client is running in 'quiet mode'. Cannot install audio packs!");
		Term_putstr(0, 2, -1, TERM_RED, "(Restart TomeNET client with 'Sound=1' and without '-q'.)");
		Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...");
		inkey();
		return;
	}

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
	strcat(path_7z_quoted, "\\7zG.exe\"");
	strcat(path_7z, "\\7zG.exe");

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
	Term_putstr(0, 1, -1, TERM_WHITE, "Unarchiver 7-Zip (7zG.exe) found.");
#else /* assume posix */
    if (!strcmp(ANGBAND_SYS, "x11")) {
 #if 0	/* command-line 7z */
	r = system("7z > tmp.7z");
 #else	/* GUI 7z (for password prompts) */
 	fff = fopen("tmp", "w");
 	fclose(fff);
	r = system("7zG a tmp.7z tmp");
	remove("tmp");
 #endif
        if (!(fff = fopen("tmp.7z", "r"))) { /* paranoia? */
		Term_putstr(0, 1, -1, TERM_RED, "7-zip GUI not found ('7zG'). Install it first. (Package name is 'p7zip'.)");
		Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...");
		inkey();
		return;
	} else if (fgetc(fff) == EOF) { /* normal */
		Term_putstr(0, 1, -1, TERM_RED, "7-zip GUI not found ('7zG'). Install it first. (Package name is 'p7zip'.)");
		fclose(fff);
		Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...");
		inkey();
		return;
	}
	Term_putstr(0, 1, -1, TERM_WHITE, "Unarchiver (7zG) found.");
    } else { /* gcu */
	/* assume posix; ncurses commandline */
	r = system("7z > tmp.7z");
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
	Term_putstr(0, 1, -1, TERM_WHITE, "Unarchiver (7z) found.");
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
		strcpy(pack_name, de->d_name);
		if (!strncmp(pack_name, "TomeNET-soundpack", 17) || !strncmp(pack_name, "TomeNET-musicpack", 17)) {
			if (!strncmp(pack_name, "TomeNET-soundpack", 17)) sound_pack = TRUE;
			if (!strncmp(pack_name, "TomeNET-musicpack", 17)) music_pack = TRUE;

			Term_putstr(0, 3, -1, TERM_ORANGE, format("Found file '%s'. Install this one? [Y/n]", pack_name));
			while (TRUE) {
				c = inkey();
				if (c == 'n' || c == 'N') break;
				if (c == 'y' || c == 'Y' || c == ' ' || c == '\r') break;
			}
			if (c == 'n' || c == 'N') continue;

			if (!strncmp(pack_name, "TomeNET-soundpack", 17)) music_pack = FALSE;
			if (!strncmp(pack_name, "TomeNET-musicpack", 17)) sound_pack = FALSE;
			picked = TRUE;
			break;
		}
	}
	closedir(dr);
    }
#else /* assume POSIX */
	r = system("ls TomeNET-soundpack*.7z TomeNET-musicpack*.7z > __tomenet.tmp");
	fff = fopen("__tomenet.tmp", "r");
	if (!fff) {
		c_message_add("\377oError: Couldn't scan TomeNET folder.");
		return;
	}
	while (!feof(fff)) {
		if (!fgets(pack_name, 1024, fff)) break;
		pack_name[strlen(pack_name) - 1] = 0; //'ls' command outputs trailing '/' on each line
		if (!strncmp(pack_name, "TomeNET-soundpack", 17) || !strncmp(pack_name, "TomeNET-musicpack", 17)) {
			if (!strncmp(pack_name, "TomeNET-soundpack", 17)) sound_pack = TRUE;
			if (!strncmp(pack_name, "TomeNET-musicpack", 17)) music_pack = TRUE;

			Term_putstr(0, 3, -1, TERM_ORANGE, format("Found file '%s'. Install this one? [Y/n]", pack_name));
			while (TRUE) {
				c = inkey();
				if (c == 'n' || c == 'N') break;
				if (c == 'y' || c == 'Y' || c == ' ' || c == '\r') break;
			}
			if (c == 'n' || c == 'N') continue;

			if (!strncmp(pack_name, "TomeNET-soundpack", 17)) music_pack = FALSE;
			if (!strncmp(pack_name, "TomeNET-musicpack", 17)) sound_pack = FALSE;
			picked = TRUE;
			break;
		}
	}
	fclose(fff);
	remove("__tomenet.tmp");
#endif

	if (!sound_pack && !music_pack) {
		Term_putstr(0, 3, -1, TERM_ORANGE, "Neither files 'TomeNET-soundpack*.7z' nor 'TomeNET-musicpack*.7z' were");
		Term_putstr(0, 4, -1, TERM_ORANGE, "found in your TomeNET folder. Aborting audio pack installation.       ");
		Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...                             ");
		inkey();
		return;
	}
	if (!picked) {
		Term_putstr(0, 3, -1, TERM_ORANGE, "You skipped all available audio packs, hence none were installed.     ");
		Term_putstr(0, 4, -1, TERM_ORANGE, "Aborting audio pack installation.                                     ");
		Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...                             ");
		inkey();
		return;
	}

	/* Check what folder name the pack wants to extract to, whether that folder already exists, and allow to cancel the process. */
#ifdef WINDOWS
	/* uhoh: 7zG actually does NOT support 'l' command, lol. Need to trust that we can run 7z in the same folder for that, I guess */
	strcpy(out_val, path_7z);
	out_val[strlen(out_val) - 5] = 0;
	strcat(out_val, ".exe"); //'''>_> {cough}
 #if 0
	_spawnl(_P_WAIT, out_val, format("\"%s\"", out_val), "l", pack_name, ">", "__tomenet.tmp", NULL);
 #else
	fff = fopen("__tomenethelper.bat", "w");
	if (!fff) {
		c_message_add("\377oError: Couldn't write temporary file.");
		return;
	}
	fprintf(fff, format("@\"%s\" l %s > __tomenet.tmp\n", out_val, pack_name));
	fclose(fff);
	_spawnl(_P_WAIT, "__tomenethelper.bat", "__tomenethelper.bat", NULL);
	remove("__tomenethelper.bat");
 #endif
#else /* assume POSIX */
    if (!strcmp(ANGBAND_SYS, "x11")) {
	r = system(format("7z l %s > __tomenet.tmp", pack_name));
    } else { /* gcu */
	r = system(format("7z l %s > __tomenet.tmp", pack_name));
    }
#endif
	fff = fopen("__tomenet.tmp", "r");
	if (!fff) {
		c_message_add("\377oError: Couldn't scan 7z file.");
		return;
	}
	while (!feof(fff)) {
		if (!fgets(out_val, 1024, fff)) break;
		/* Scan for folder names */
		if (!strstr(out_val, "D....")) continue;
		if (sound_pack) {
			/* The first folder must start on 'sound' */
			if (!strstr(out_val, " sound") || strchr(out_val, '/')) {
				c_message_add("\377oError: Invalid sound pack file. Has no top folder 'sound*'.");
				fclose(fff);
				remove("__tomenet.tmp");
				return;
			}
			picked = FALSE;//abuse
			strcpy(path, strstr(out_val, "sound"));
		}
		if (music_pack) {
			/* The first folder must start on 'music' */
			if (!strstr(out_val, " music") || strchr(out_val, '/')) {
				c_message_add("\377oError: Invalid music pack file. Has no top folder 'music*'.");
				fclose(fff);
				remove("__tomenet.tmp");
				return;
			}
			picked = FALSE;//abuse
			strcpy(path, strstr(out_val, "music"));
		}
		if (!picked) break;
	}
	fclose(fff);
	remove("__tomenet.tmp");
	if (picked) {
		c_message_add("\377oError: Invalid audio pack file. Has no top folder.");
		return;
	}
	Term_putstr(0, 3, -1, TERM_YELLOW, "That pack wants to install to this target folder. Is that ok? (y/n):");
#ifdef WINDOWS
	path[strlen(path) - 1] = 0; //trailing newline
	Term_putstr(0, 4, -1, TERM_YELLOW, format(" '%s\\%s'", ANGBAND_DIR_XTRA, path));
#else
	path[strlen(path) - 1] = 0; //trailing newline
	Term_putstr(0, 4, -1, TERM_YELLOW, format(" '%s/%s'", ANGBAND_DIR_XTRA, path));
#endif
	while (TRUE) {
		c = inkey();
		if (c == 'n' || c == 'N') {
			c_message_add("\377yInstallation process has been cancelled.");
			return;
		}
		if (c == 'y' || c == 'Y') break;
	}

	/* install sound pack */
	if (sound_pack) {
		Term_putstr(0, 3, -1, TERM_WHITE, "Installing sound pack...                                                    ");
		Term_putstr(0, 4, -1, TERM_WHITE, "                                                                            ");
		Term_fresh();
		Term_flush();
#if defined(WINDOWS)
		_spawnl(_P_WAIT, path_7z, path_7z_quoted, "x", format("-o%s", ANGBAND_DIR_XTRA), pack_name, NULL);
#else /* assume posix */
    if (!strcmp(ANGBAND_SYS, "x11")) {
		r = system(format("7zG x -o%s %s", ANGBAND_DIR_XTRA, pack_name));
    } else { /* gcu */
		r = system(format("7z x -o%s %s", ANGBAND_DIR_XTRA, pack_name));
    }
#endif
		Term_putstr(0, 3, -1, TERM_L_GREEN, "Sound pack has been installed. You can select it via '\377gX\377G' in the '=' menu.   ");
		Term_putstr(0, 4, -1, TERM_L_GREEN, "YOU NEED TO RESTART TomeNET FOR THIS TO TAKE EFFECT.                        ");
	}

	/* install music pack */
	if (music_pack) {
		Term_putstr(0, 6, -1, TERM_WHITE, "Installing music pack...                                                    ");
		Term_putstr(0, 7, -1, TERM_WHITE, "                                                                            ");
		Term_fresh();
		Term_flush();
#if defined(WINDOWS)
		//todo: custom foldernames-- remove("music/music.cfg"); //just for password_ok check below

		_spawnl(_P_WAIT, path_7z, path_7z_quoted, "x", format("-o%s", ANGBAND_DIR_XTRA), pack_name, NULL);

		/*todo: custom foldernames password check--
		* actually check for successful password, by checking whether at least music.cfg was extracted *
		if (!(fff = fopen("music/music.cfg", "r"))) password_ok = FALSE;
		else if (fgetc(fff) == EOF) { //paranoia
			password_ok = FALSE;
			fclose(fff);
		} else fclose(fff);
		*/
#else /* assume posix */
    if (!strcmp(ANGBAND_SYS, "x11")) {
		//todo: custom foldernames-- remove("music/music.cfg"); //just for password_ok check below
		r = system(format("7zG x -o%s %s", ANGBAND_DIR_XTRA, pack_name));
		/*todo: custom foldernames password check... (see above) */
    } else { /* gcu */
		//todo: custom foldernames-- remove("music/music.cfg"); //just for password_ok check below
		r = system(format("7z x -o%s %s", ANGBAND_DIR_XTRA, pack_name));
		/*todo: custom foldernames password check... (see above) */
    }
#endif

		if (password_ok) {
			Term_putstr(0, 6, -1, TERM_L_GREEN, "Music pack has been installed. You can select it via '\377gX\377G' in the '=' menu.   ");
			Term_putstr(0, 7, -1, TERM_L_GREEN, "YOU NEED TO RESTART TomeNET FOR THIS TO TAKE EFFECT.                        ");
		} else {
			Term_putstr(0, 6, -1, TERM_L_RED, "You entered a wrong password. Music pack could not be installed.");
		}
	}

	//slay compiler warning;
	(void)r;

	Term_putstr(0, 9, -1, TERM_WHITE, "Press any key to return to options menu...");
	Term_fresh();
	/* inkey() will react to client timeout after long extraction time, terminating
	   near-instantly with hardly a chance to see the green 'installed' messages. */
	Term_inkey(&ch, TRUE, TRUE);
	//inkey();
}

/*
 * Set or unset various options.
 *
 * The user must use the "Ctrl-R" command to "adapt" to changes
 * in any options which control "visual" aspects of the game.
 */
void do_cmd_options(void) {
	int k;
	char tmp[1024];

	bool changed1 = c_cfg.exp_need, changed2 = c_cfg.exp_bar, changed3 = c_cfg.font_map_solid_walls;

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
		c_prt(TERM_L_GREEN, "TomeNET options", 0, 0);

		/* Give some choices */
		Term_putstr(3, 2, -1, TERM_WHITE, "(\377y1\377w) User Interface Options 1");
		Term_putstr(3, 3, -1, TERM_WHITE, "(\377y2\377w) User Interface Options 2");
		Term_putstr(3, 4, -1, TERM_WHITE, "(\377y3\377w) User Interface Options 3");
		Term_putstr(3, 5, -1, TERM_WHITE, "(\377y4\377w) Audio Options");
		Term_putstr(3, 6, -1, TERM_WHITE, "(\377y5\377w) Game-Play Options 1");
		Term_putstr(3, 7, -1, TERM_WHITE, "(\377y6\377w) Game-Play Options 2");
		Term_putstr(3, 8, -1, TERM_WHITE, "(\377yw\377w) Window Flags");
		Term_putstr(3, 9, -1, TERM_WHITE, "(\377os\377w) Save Options & Flags");
		Term_putstr(3,10, -1, TERM_WHITE, "(\377ol\377w) Load Options & Flags");

		Term_putstr(3,12, -1, TERM_SLATE, "The following options are all saved automatically on quitting via CTRL+Q:");
		if (c_cfg.rogue_like_commands)
			Term_putstr(3,13, -1, TERM_WHITE, "(\377yx\377w/\377yX\377w) Audio mixer (also accessible via CTRL+F hotkey) / Audio pack selector");
		else
			Term_putstr(3,13, -1, TERM_WHITE, "(\377yx\377w/\377yX\377w) Audio mixer (also accessible via CTRL+U hotkey) / Audio pack selector");

		Term_putstr(3,14, -1, TERM_WHITE, "(\377yn\377w/\377yN\377w) Disable/reenable specific sound effects/music");

#if defined(WINDOWS) || defined(USE_X11)
		/* Font (and window) settings aren't available in command-line mode */
		if (strcmp(ANGBAND_SYS, "gcu")) {
 #ifdef ENABLE_SUBWINDOW_MENU
			Term_putstr(3,15, -1, TERM_WHITE, "(\377yf\377w) Window Fonts and Visibility");
 #endif
			/* CHANGE_FONTS_X11 */
			Term_putstr(3,16, -1, TERM_WHITE, "(\377yc\377w) Cycle all font sizes at once (can be tapped multiple times)");
		}
#endif

		Term_putstr(3,18, -1, TERM_WHITE, "(\377UA\377w) Account Options");
		Term_putstr(3,19, -1, TERM_WHITE, "(\377UI\377w) Install sound/music pack from 7z-file you placed in your TomeNET folder");

		/* Prompt */
		c_prt(TERM_L_GREEN, "Command: ", 21, 0);

		/* Get command */
		k = inkey();

		/* Exit */
		if (k == ESCAPE || k == KTRL('Q')) break;

		/* Take a screenshot */
		if (k == KTRL('T')) xhtml_screenshot("screenshot????");
		/* specialty: allow chatting from within here */
		else if (k == ':') cmd_message();

		else if (k == '1') {
			do_cmd_options_aux(1, "User Interface Options 1");
		} else if (k == '2') {
			do_cmd_options_aux(4, "User Interface Options 2");
		} else if (k == '3') {
			do_cmd_options_aux(6, "User Interface Options 3");
		} else if (k == '4') {
			do_cmd_options_aux(5, "Audio Options");
		} else if (k == '5') {
			do_cmd_options_aux(2, "Game-Play Options 1");
		} else if (k == '6') {
			do_cmd_options_aux(3, "Game-Play Options 2");
		}

		/* Save a 'option' file */
		else if (k == 's') {
			/* Prompt */
			Term_putstr(0, 20, -1, TERM_L_GREEN, "Command: Save an options file");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 21, -1, TERM_YELLOW, "File: ");

			/* Default filename */
			//sprintf(tmp, "global.opt");
			sprintf(tmp, "%s.opt", cname);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0))
				continue;

			/* Dump the macros */
			(void)options_dump(tmp);
		}
		/* Load a pref file */
		else if (k == 'l') {
			/* Prompt */
			Term_putstr(0, 20, -1, TERM_L_GREEN, "Command: Load an options file");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 21, -1, TERM_YELLOW, "File: ");

			/* Default filename */
			//sprintf(tmp, "global.opt");
			sprintf(tmp, "%s.opt", cname);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0))
				continue;

			/* Process the given filename */
			(void)process_pref_file(tmp);
		}

		/* Account options */
		else if (k == 'A') do_cmd_options_acc();

		/* Window flags */
		else if (k == 'w') {
			/* Spawn */
			do_cmd_options_win();
		}

#if defined(WINDOWS) || defined(USE_X11)
 #ifdef ENABLE_SUBWINDOW_MENU
		/* Change fonts separately and manually */
		else if (k == 'f') do_cmd_options_fonts();
 #endif
		/* Cycle all fonts */
		else if (k == 'c') change_font(-1);
#endif

		/* Access audio mixer */
		else if (k == 'x') interact_audio();

		else if (k == 'X') audio_pack_selector();

		else if (k == 'I') do_cmd_options_install_audio_packs();

		/* Toggle single sfx/song from a list of all */
		else if (k == 'n') do_cmd_options_sfx();
		else if (k == 'N') do_cmd_options_mus();

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

	/* for exp_need option changes: */
	if (changed1 != c_cfg.exp_need || changed2 != c_cfg.exp_bar || changed3 != c_cfg.font_map_solid_walls)
		prt_level(p_ptr->lev, p_ptr->max_lev, p_ptr->max_plv, p_ptr->max_exp, p_ptr->exp, exp_adv, exp_adv_prev);
	/* in case hp/mp/st are displayed as bars */
	if (changed3 != c_cfg.font_map_solid_walls) {
		prt_hp(hp_max, hp_cur, hp_bar);
		prt_sp(sp_max, sp_cur, sp_bar);
		prt_stamina(st_max, st_cur, st_bar);
	}

	inkey_msg = inkey_msg_old;

	/* hide cursor -- just for when the main screen font has been changed, it seems after
	   that the cursor is now visible for some reason (and jumping around annoyingly) */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	/* Resend options to server */
	Send_options();
}


/*
 * Centers a string within a 31 character string		-JWT-
 */
static void center_string(char *buf, cptr str) {
	int i, j;

	/* Total length */
	i = strlen(str);

	/* Necessary border */
	j = 16 - (i + 1) / 2;

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
#define STONE_COL_SHORT (11+6)
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
		if (!music(exec_lua(0, "return get_music_index(\"tomb\")"))) music(-4);
   #else
		/* switch to login screen music if available; otherwise just continue playing the current in-game music */
		music(exec_lua(0, "return get_music_index(\"tomb\")"));
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

#if 0	/* make the server send those info! */
		/* King or Queen */
		if (total_winner || (p_ptr->lev > PY_MAX_LEVEL))
			p = "Magnificent";
		/* Normal */
		else
			p =  cp_ptr->titles[(p_ptr->lev-1)/5] + c_text;
#endif	/* 0 */

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
		c_put_str(TERM_L_UMBER, buf, 11-2, STONE_COL_SHORT);

		(void)sprintf(tmp, "Exp: %d", p_ptr->exp);
		center_string_short(buf, tmp);
		c_put_str(TERM_L_UMBER, buf, 12-2, STONE_COL_SHORT);
#else
		(void)sprintf(tmp, "Lv: %d, Exp: %d", (int)p_ptr->lev, p_ptr->exp);
		center_string_short(buf, tmp);
		c_put_str(TERM_L_UMBER, buf, 11-2, STONE_COL_SHORT);
#endif
		/* XXX usually 0 */
		(void)sprintf(tmp, "AU: %d", p_ptr->au);
		center_string_short(buf, tmp);
		c_put_str(TERM_L_UMBER, buf, 12-2, STONE_COL_SHORT);

		/* Location */
		if (c_cfg.depth_in_feet)
			(void)sprintf(tmp, "Died on %dft %s", p_ptr->wpos.wz * 50, location_pre);
		else
			(void)sprintf(tmp, "Died on Lv %d %s", p_ptr->wpos.wz, location_pre);
		center_string(buf, tmp);
		c_put_str(TERM_L_UMBER, buf, 13-1, STONE_COL);

		if (location_name2[0])
			sprintf(tmp, "%s", location_name2);
		else if (p_ptr->wpos.wx == 127)
			sprintf(tmp, "an unknown location");
		else
			sprintf(tmp, "world map region (%d,%d)", p_ptr->wpos.wx, p_ptr->wpos.wy);
		center_string(buf, tmp);
		c_put_str(TERM_L_UMBER, buf, 14-1, STONE_COL);

		/* Time of death */
		(void)sprintf(tmp, "%-.24s", ctime(&ct));
		center_string(buf, tmp);
		c_put_str(TERM_L_UMBER, buf, 17-3, STONE_COL);

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
			c_put_str(TERM_L_UMBER, buf, 18-3, STONE_COL);
			center_string(buf, reason2 + 10);
			c_put_str(TERM_L_UMBER, buf, 21-5, STONE_COL);
		} else if (strstr(reason2, "Committed suicide")) {
			if (p_ptr->total_winner) {
				center_string(buf, "Died from ripe old age");
				c_put_str(TERM_L_UMBER, buf, 19-3, STONE_COL);
			} else {
				center_string(buf, "Committed suicide");
				c_put_str(TERM_L_UMBER, buf, 19-3, STONE_COL);
			}
		} else {
			center_string(buf, reason2);
			c_put_str(TERM_L_UMBER, buf, 21-5, STONE_COL);
		}
	}
}

/*
 * Display some character info	- Jir -
 * For now, only when losing the character.
 */
void c_close_game(cptr reason) {
	int k;
	char tmp[MAX_CHARS];
	bool c_cfg_tmp = c_cfg.topline_no_msg;

	/* Let the player view the last scene */
	c_cfg.topline_no_msg = FALSE;
	c_msg_format("%s ...Press '0' key to proceed", reason);

	while (inkey() != '0');
	c_cfg.topline_no_msg = c_cfg_tmp;

	/* You are dead */
	print_tomb(reason);

	/* Remember deceased char's name if we will just recreate the same.. */
	strcpy(prev_cname, cname);

#if 0
	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	/* Display tomb screen without an input prompt, allow taking a 'clear' screenshot! */
	//while (inkey() == KTRL('T')) xhtml_screenshot("screenshot????");
	if (inkey() == KTRL('T')) xhtml_screenshot("screenshot????");
#endif

	put_str("ESC to quit, 'f' to dump the record or any other key to proceed", 23, 0);
	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;

	/* TODO: bandle them in one loop instead of 2 */
	while (1) {
		/* Get command */
		k = inkey();

		/* Exit */
		if (k == ESCAPE || k == KTRL('Q') || k == 'q' || k == 'Q') return;

		else if (k == KTRL('T')) {
			/* Take a screenshot */
			xhtml_screenshot("screenshot????");
		}

		/* Dump */
		else if ((k == 'f') || (k == 'F')) {
			strnfmt(tmp, MAX_CHARS - 1, "%s.txt", cname);
			if (get_string("Filename(you can post it to http://angband.oook.cz/): ", tmp, MAX_CHARS - 1)) {
				if (tmp[0] && (tmp[0] != ' ')) {
					file_character(tmp, FALSE);
					break;
				}
			}
			continue;
		}
		/* Safeguard */
		else if (k == '0') continue;

		else if (k) break;
	}

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
			xhtml_screenshot("screenshot????");

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
			do_cmd_messages_chatonly();

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
}



/*
 * Since only GNU libc has memfrob, we use our own.
 */
void my_memfrob(void *s, int n) {
	int i;
	char *str;

	str = (char*) s;

	for (i = 0; i < n; i++) {
		/* XOR every byte with 42 */
		str[i] ^= 42;
	}
}



/*
 * Check if the server version fills the requirements.
 * Copied from the server code.
 *
 * Branch has to be an exact match.
 */
bool is_newer_than(version_type *version, int major, int minor, int patch, int extra, int branch, int build) {
#ifdef ATMOSPHERIC_INTRO
	/* hack for animating colours before we even have network contact to server */
	if (!version->major) return TRUE;
#endif

	if (version->major < major)
		return FALSE; /* very old */
	else if (version->major > major)
		return TRUE; /* very new */
	else if (version->minor < minor)
		return FALSE; /* pretty old */
	else if (version->minor > minor)
		return TRUE; /* pretty new */
	else if (version->patch < patch)
		return FALSE; /* somewhat old */
	else if (version->patch > patch)
		return TRUE; /* somewhat new */
	else if (version->extra < extra)
		return FALSE; /* a little older */
	else if (version->extra > extra)
		return TRUE; /* a little newer */
	/* Check that the branch is an exact match */
	else if (version->branch == branch)
	{
		/* Now check the build */
		if (version->build < build)
			return FALSE;
		else if (version->build > build)
			return TRUE;
	}

	/* Default */
	return FALSE;
}

bool is_same_as(version_type *version, int major, int minor, int patch, int extra, int branch, int build)
{
	if (version->major == major
	    && version->minor == minor
	    && version->patch == patch
	    && version->extra == extra
	    && version->branch == branch
	    && version->build == build)
		return TRUE;

	return FALSE;
}

/* dummy */
void msg_format(int Ind, cptr fmt, ...) {
	(void) Ind; /* suppress compiler warning */
	(void) fmt; /* suppress compiler warning */
	return;
}

#ifdef USE_SOUND_2010
bool sound(int val, int type, int vol, s32b player_id) {
	if (!use_sound) return TRUE;

	/* play a sound */
	if (sound_hook) return sound_hook(val, type, vol, player_id);
	else return FALSE;
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
	if (!use_sound) return TRUE;

	/* play a sound */
	if (music_hook) return music_hook(val);
	else return FALSE;
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
	int i, j, item_x[8] = {2, 12, 22, 32, 42, 52, 62, 72}, k;
	static int cur_item = 0;
	int y_label = 20, y_toggle = 12, y_slider = 18;
	bool redraw = TRUE, quit = FALSE;

	/* Save screen */
	Term_save();

	/* suppress hybrid macros */
	bool inkey_msg_old = inkey_msg;
	inkey_msg = TRUE;

	/* Process requests until done */
	while (1) {
		if (redraw) {
			/* Clear screen */
			Term_clear();

			/* Describe */
			Term_putstr(30,  0, -1, TERM_L_UMBER, "*** Audio Mixer ***");
			//Term_putstr(3, 1, -1, TERM_L_UMBER, "Press arrow keys to navigate/modify, RETURN/SPACE to toggle, ESC to leave.");
			Term_putstr(3, 1, -1, TERM_L_UMBER, "Press \377yarrow keys\377U to navigate/modify, \377yRETURN\377U/\377ySPACE\377U to toggle, \377yESC\377U to leave.");
			//Term_putstr(6, 2, -1, TERM_L_UMBER, "Shortcuts: 'a': master, 'w': weather, 's': sound, 'c' or 'm': music.");
			//Term_putstr(7, 3, -1, TERM_L_UMBER, "Jump to volume slider: SHIFT + according shortcut key given above.");
			//Term_putstr(6, 2, -1, TERM_L_UMBER, "Shortcuts: 'a','w','s','c'/'m'. Shift + shortcut to jump to a slider.");
			Term_putstr(2, 2, -1, TERM_L_UMBER, "Shortcuts: '\377ya\377U','\377yw\377U','\377ys\377U','\377yc\377U'/'\377ym\377U'. \377yShift+shortcut\377U to jump. \377yCTRL+R\377U reloads packs.");

			if (quiet_mode) Term_putstr(12, 4, -1, TERM_L_RED,                              "  Client is running in 'quiet mode': Audio is disabled.  ");
			else if (audio_sfx > 3 && audio_music > 0) Term_putstr(12, 4, -1, TERM_L_GREEN, "     Sound pack and music pack have been detected.      ");
			//else if (audio_sfx > 3 && audio_music == 0) Term_putstr(12, 4, -1, TERM_YELLOW, "Sound pack detected. No music pack seems to be installed.");
			else if (audio_sfx > 3 && audio_music == 0) {
				Term_putstr(12, 4, -1, TERM_L_GREEN, "Sound pack detected.");
				Term_putstr(34, 4, -1, TERM_L_RED, "No music pack seems to be installed.");
			}
			//else if (audio_sfx <= 3 && audio_music > 0) Term_putstr(12, 4, -1, TERM_YELLOW, "Music pack detected. No sound pack seems to be installed.");
			else if (audio_sfx <= 3 && audio_music > 0) {
				Term_putstr(12, 4, -1, TERM_L_GREEN, "Music pack detected.");
				Term_putstr(34, 4, -1, TERM_L_RED, "No sound pack seems to be installed.");
			}
			else Term_putstr(12, 4, -1, TERM_L_RED,                                         "Neither sound pack nor music pack seems to be installed. ");

			if (c_cfg.rogue_like_commands)
				Term_putstr(3, y_label + 2, -1, TERM_SLATE, "Outside of this mixer you can toggle audio and music by CTRL+V and CTRL+Q.");
			else
				Term_putstr(3, y_label + 2, -1, TERM_SLATE, "Outside of this mixer you can toggle audio and music by CTRL+N and CTRL+C.");

			/* draw mixer */
			Term_putstr(item_x[0], y_toggle, -1, TERM_WHITE, format(" [%s]", cfg_audio_master ? "\377GX\377w" : " "));
			if (c_cfg.rogue_like_commands)
				Term_putstr(item_x[0], y_toggle + 3, -1, TERM_SLATE, "CTRL+V");
			else
				Term_putstr(item_x[0], y_toggle + 3, -1, TERM_SLATE, "CTRL+N");
			Term_putstr(item_x[1], y_toggle, -1, TERM_WHITE, format(" [%s]", cfg_audio_music ? "\377GX\377w" : " "));
			if (c_cfg.rogue_like_commands)
				Term_putstr(item_x[1], y_toggle + 3, -1, TERM_SLATE, "CTRL+Q");
			else
				Term_putstr(item_x[1], y_toggle + 3, -1, TERM_SLATE, "CTRL+C");
			Term_putstr(item_x[2], y_toggle, -1, TERM_WHITE, format(" [%s]", cfg_audio_sound ? "\377GX\377w" : " "));
			Term_putstr(item_x[3], y_toggle, -1, TERM_WHITE, format(" [%s]", cfg_audio_weather ? "\377GX\377w" : " "));

			for (i = 4; i <= 7; i++) {
				Term_putstr(item_x[i - 4], y_toggle + 1, -1, TERM_SLATE, "on/off");
				Term_putstr(item_x[i], y_slider - 12, -1, TERM_WHITE, "  ^");
				Term_putstr(item_x[i], y_slider, -1, TERM_WHITE, "  V");
				for (j = y_slider - 1; j >= y_slider - 11; j--)
					Term_putstr(item_x[i], j, -1, TERM_SLATE, "  |");
			}

			Term_putstr(item_x[4], y_slider - cfg_audio_master_volume / 10 - 1, -1, TERM_L_GREEN, "  =");
			Term_putstr(item_x[5], y_slider - cfg_audio_music_volume / 10 - 1, -1, TERM_L_GREEN, "  =");
			Term_putstr(item_x[6], y_slider - cfg_audio_sound_volume / 10 - 1, -1, TERM_L_GREEN, "  =");
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
		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????");
			redraw = FALSE;
			break;
		case KTRL('U'):
		case KTRL('F'): /* <- rogue-like keymap */
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
		case KTRL('N'):
		case KTRL('V'): //rl
		case 'a':
			toggle_master(TRUE);
			break;
		case KTRL('C'):
		case KTRL('X'): //rl
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
			cur_item = 0;
			break;
		case 'C':
		case 'M':
			cur_item = 1;
			break;
		case 'S':
			cur_item = 2;
			break;
		case 'W':
			cur_item = 3;
			break;
		case '\n':
		case '\r':
		case ' ':
			switch (cur_item) {
			case 0: toggle_master(TRUE); break;
			case 1: toggle_music(TRUE); break;
			case 2: toggle_sound(); break;
			case 3: toggle_weather(); break;
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
			}
			break;
		case KTRL('R'):
			if (re_init_sound() == 0) c_message_add("Audio packs have been reloaded successfully.");
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
}
void toggle_master(bool gui) {
	cfg_audio_master = !cfg_audio_master;
	//if (!gui) c_message_add(format("\377yAudio is now %s.", cfg_audio_master ? "ON" : "OFF"));
	set_mixing();
}
void toggle_music(bool gui) {
	cfg_audio_music = !cfg_audio_music;
	//if (!gui) c_message_add(format("\377yMusic is now %s.", cfg_audio_music ? "ON" : "OFF"));
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

	FILE *fff, *fff2;
#ifndef WINDOWS
	int r;
#endif

	/* suppress hybrid macros */
	bool inkey_msg_old = inkey_msg;
	inkey_msg = TRUE;

	/* Save screen */
	Term_save();

	/* Get list of all folders starting on 'music' or 'sound' within lib/xtra */
	fff = fopen("__tomenet.tmp", "w"); //just make sure the file always exists, for easier file-reading handling.. pft */
	if (!fff) {
		c_message_add("\377oError: Couldn't write temporary file.");
		return;
	}
	fclose(fff);
#ifdef WINDOWS
 #if 0 /* use system calls - easy, but has drawback of cmd shell window popup -_- */
	fff = fopen("__tomenethelper.bat", "w");
	if (!fff) {
		c_message_add("\377oError: Couldn't write temporary file.");
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
		c_message_add("\377oError: Couldn't scan audio pack folders.");
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
					if (!strncmp(buf, "packname = ", 11)) {
						strcpy(sp_name[soundpacks], buf + 11);
						continue;
					}
					if (!strncmp(buf, "author = ", 9)) {
						strcpy(sp_author[soundpacks], buf + 9);
						continue;
					}
					if (!strncmp(buf, "description = ", 14)) {
						strcpy(sp_diz[soundpacks], buf + 14);
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
					if (!strncmp(buf, "packname = ", 11)) {
						strcpy(mp_name[musicpacks], buf + 11);
						continue;
					}
					if (!strncmp(buf, "author = ", 9)) {
						strcpy(mp_author[musicpacks], buf + 9);
						continue;
					}
					if (!strncmp(buf, "description = ", 14)) {
						strcpy(mp_diz[musicpacks], buf + 14);
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
					Term_putstr(0, 4 + k, -1, TERM_ORANGE, sp_dir[cur_sp + k - cur_sy]);
				else
					Term_putstr(0, 4 + k, -1, TERM_L_WHITE, sp_dir[cur_sp + k - cur_sy]);
			}
			for (k = 0; k < PACKS_SCREEN; k++) {
				if (k - cur_my + cur_mp >= musicpacks) break;
				if (k == cur_my)
					Term_putstr(40, 4 + k, -1, TERM_ORANGE, mp_dir[cur_mp + k - cur_my]);
				else
					Term_putstr(40, 4 + k, -1, TERM_L_WHITE, mp_dir[cur_mp + k - cur_my]);
			}

			Term_putstr(0, 15, -1, TERM_L_UMBER, "Selected SP:");
			Term_putstr(13, 15, -1, TERM_YELLOW, format("%s [by %s]", sp_name[cur_sp], sp_author[cur_sp]));
			Term_putstr(0, 16, -1, TERM_WHITE, sp_diz[cur_sp]);
			if (strlen(sp_diz[cur_sp]) >= 80) Term_putstr(0, 17, -1, TERM_WHITE, &sp_diz[cur_sp][80]);
			if (strlen(sp_diz[cur_sp]) >= 160) Term_putstr(0, 18, -1, TERM_WHITE, &sp_diz[cur_sp][160]);
			Term_putstr(0, 20, -1, TERM_L_UMBER, "Selected MP:");
			Term_putstr(13, 20, -1, TERM_YELLOW, format("%s [by %s]", mp_name[cur_mp], mp_author[cur_mp]));
			Term_putstr(0, 21, -1, TERM_WHITE, mp_diz[cur_mp]);
			if (strlen(mp_diz[cur_mp]) >= 80) Term_putstr(0, 22, -1, TERM_WHITE, &mp_diz[cur_mp][80]);
			if (strlen(mp_diz[cur_mp]) >= 160) Term_putstr(0, 23, -1, TERM_WHITE, &mp_diz[cur_mp][160]);
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
			xhtml_screenshot("screenshot????");
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
	}
	if (strcmp(cfg_musicpackfolder, mp_dir[cur_mp])) {
		c_message_add(format("Switched music pack to '%s'.", mp_dir[cur_mp]));
		strcpy(cfg_musicpackfolder, mp_dir[cur_mp]);
	}

#ifdef WINDOWS
	store_audiopackfolders();
#else /* assume POSIX */
	write_mangrc(FALSE, TRUE);
#endif

	/* Reload screen */
	Term_load();

	/* Flush the queue */
	Flush_queue();

	/* Re-enable hybrid macros */
	inkey_msg = inkey_msg_old;

	/* Switch it live! */
	(void)re_init_sound();

	//No longer true (for SDL, our only sound sytem at this point basically):
	//c_message_add("\377RAfter changing audio packs, a game client restart is required!");
}

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
	/* BIG_MAP is currently not supported in GCU client */
	if (!strcmp(ANGBAND_SYS, "gcu") && option_info[i].o_var == &c_cfg.big_map) {
		c_cfg.big_map = FALSE;
		(*option_info[i].o_var) = FALSE;
		Client_setup.options[i] = FALSE;
		screen_hgt = SCREEN_HGT;
		if (playing) Send_screen_dimensions();
	} else
#endif

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

	/* Refresh music when shuffle_music is toggled */
	if (option_info[i].o_var == &c_cfg.shuffle_music) {
		/* ..but only if we're not already in the process of changing music! */
		if (music_next == -1)
			music(-3); //refresh!
	}
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

	return c;
}

/* Load the default font's or a custom font's pref file aka
    "Access the "visual" system pref file (if any)". */
#define CUSTOM_FONT_PRF /* enable custom pref files? */
void handle_process_font_file(void) {
	char buf[1024 + 17];
#ifdef CUSTOM_FONT_PRF
	char fname[1024];
	int i;

 #if 1
	/* Delete all font info we currently know */
	//feat info (terrain tiles)
	for (i = 0; i < MAX_F_IDX; i++) {
		Client_setup.f_attr[i] = 0;
		Client_setup.f_char[i] = 0;
	}
	for (i = 0; i < 256; i++) floor_mapping_mod[i] = 0;
	//race info (monsters)
	for (i = 0; i < MAX_R_IDX; i++) {
		Client_setup.r_attr[i] = 0;
		Client_setup.r_char[i] = 0;
	}
	for (i = 0; i < 256; i++) monster_mapping_mod[i] = 0;
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

	/* Actually try to load a custom font-xxx.prf file, depending on the main screen font */
	get_screen_font_name(fname);
	if (!use_graphics && fname[0]) {
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
		/* Create prf file name from font file name */
		sprintf(buf, "font-custom-%s.prf", fname);
		/* Abuse fname to build the file path */
		path_build(fname, 1024, ANGBAND_DIR_USER, buf);
		fff = my_fopen(fname, "r");
		/* If custom file doesn't exist, fallback to normal font pref file: */
		if (!fff) sprintf(buf, "font-%s.prf", ANGBAND_SYS);
		else fclose(fff);
		process_pref_file(buf);

		/* Resend definitions to the server */
		if (in_game) Send_client_setup();
	} else {
#endif
	/* Access the "visual" system pref file (if any) */
	sprintf(buf, "%s-%s.prf", (use_graphics ? "graf" : "font"), ANGBAND_SYS);
	process_pref_file(buf);
#ifdef CUSTOM_FONT_PRF
	}
#endif
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
