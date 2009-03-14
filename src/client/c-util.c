/* $Id$ */
#include "angband.h"

#define MACRO_USE_CMD	0x01
#define MACRO_USE_STD	0x02
#define MACRO_USE_HYB	0x04

#define NR_OPTIONS_SHOWN	20

static int MACRO_WAIT = 96;

static void ascii_to_text(char *buf, cptr str);

static bool after_macro = FALSE;
static bool parse_macro = FALSE;
static bool parse_under = FALSE;
static bool parse_slash = FALSE;
static bool strip_chars = FALSE;

static bool flush_later = FALSE;

static byte macro__use[256];

static bool was_chat_buffer = FALSE;
static bool was_real_chat = FALSE;

static char octify(uint i)
{
	return (hexsym[i%8]);
}

static char hexify(uint i)
{
	return (hexsym[i%16]);
}

void move_cursor(int row, int col)
{
	Term_gotoxy(col, row);
}

void flush(void)
{
	flush_later = TRUE;
}

void flush_now(void)
{
	/* Clear various flags */
	flush_later = FALSE;

	/* Cancel "macro" info */
	parse_macro = after_macro = FALSE;

	/* Cancel "sequence" info */
	parse_under = parse_slash = FALSE;

	/* Cancel "strip" mode */
	strip_chars = FALSE;

	/* Forgot old keypresses */
	Term_flush();
}

/*
 * Check for possibly pending macros
 */
static int macro_maybe(cptr buf, int n)
{
	int i;

	/* Scan the macros */
	for (i = n; i < macro__num; i++)
	{
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
static int macro_ready(cptr buf)
{
	int i, t, n = -1, s = -1;

	/* Scan the macros */
	for (i = 0; i < macro__num; i++)
	{
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
bool macro_del(cptr pat)
{
	int i, num = -1;

	/* Find the macro */
	for (i = 0; i < macro__num; i++)
	{
		if (streq(macro__pat[i], pat))
		{
			num = i;
			break;
		}
	}

	if (num == -1) return FALSE;

	/* Free it */
	string_free(macro__pat[num]);
	string_free(macro__act[num]);

	/* Remove it from every array */
	for (i = num + 1; i < macro__num; i++)
	{
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

static void sync_sleep(int milliseconds)
{
	int n;

	int result;
	int net_fd;
	net_fd = Net_fd();

	for (n = 0; n < milliseconds / 100; n++) {

		usleep(1000);

		/* Flush output - maintain flickering/multi-hued characters */
		do_flicker();
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
//		SetTimeout(0, 1000);

		/* Wait according to fps - mikaelh */
		SetTimeout(0, next_frame());

		if(c_quit) continue;

		/* Parse net input if we got any */
		if (SocketReadable(net_fd))
		{
			if ((result = Net_input()) == -1)
			{
				quit(NULL);
			}

			/* Update the screen */
			Term_fresh();

			/* Redraw windows if necessary */
			if (p_ptr->window)
			{
				window_stuff();
			}
		}

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
static char inkey_aux(void)
{
	int	k = 0, n, p = 0, w = 0;

	char	ch = 0;

	cptr	pat, act;

	char	buf[1024];

	char	buf_atoi[3];

	int net_fd;

	/* Acquire and save maximum file descriptor */
	net_fd = Net_fd();

	/* If no network yet, just wait for a keypress */
	if (net_fd == -1)
	{
		/* Look for a keypress */
		(void)(Term_inkey(&ch, TRUE, TRUE));
	}
	else
	{
		/* Wait for keypress, while also checking for net input */
		do
		{
			int result;

			/* Flush output - maintain flickering/multi-hued characters */
			do_flicker();

			/* Look for a keypress */
			(void)(Term_inkey(&ch, FALSE, TRUE));

			/* If we got a key, break */
			if (ch) break;

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

			if(c_quit) continue;

			/* Parse net input if we got any */
			if (SocketReadable(net_fd))
			{
				if ((result = Net_input()) == -1)
				{
					quit(NULL);
				}

				/* Update the screen */
				Term_fresh();

				/* Redraw windows if necessary */
				if (p_ptr->window)
				{
					window_stuff();
				}
			}
		} while (!ch);
	}


	/* End of internal macro */
	if (ch == 29) parse_macro = FALSE;


	/* Do not check "ascii 28" */
	if (ch == 28) return (ch);


	/* Do not check "ascii 29" */
	if (ch == 29) return (ch);

	if (parse_macro && (ch == MACRO_WAIT)) {
		buf_atoi[0] = '0';
		buf_atoi[1] = '0';
		buf_atoi[2] = '\0';
		(void)(Term_inkey(&ch, FALSE, TRUE));
		if (ch) buf_atoi[0] = ch;
		(void)(Term_inkey(&ch, FALSE, TRUE));
		if (ch) buf_atoi[1] = ch;
		w = atoi(buf_atoi);
		sync_sleep(w * 100L); /* w 1/10th seconds */
		ch = 0;
		w = 0;
	}

	/* Do not check macro actions */
	if (parse_macro) return (ch);

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
			/* Increase "wait" */
			w += 10;

			/* Excessive delay */
			if (w >= 100) break;

			/* Delay */
			Term_xtra(TERM_XTRA_DELAY, w);
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

	/* Push the "macro complete" key */
	if (Term_key_push(29)) return (0);


	/* Access the macro action */
	act = macro__act[k];

	/* Get the length of the action */
	n = strlen(act);

	/* Push the macro "action" onto the key queue */
	while (n > 0)
	{
		/* Push the key, notice over-flow */
		if (Term_key_push(act[--n])) return (0);
	}

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
 * "hilite_player" is TRUE, and also, we will only process "command" macros.
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
char inkey(void)
{
	int v;

	char kk, ch;

	bool done = FALSE;

	term *old = Term;

	int w = 0;

	int skipping = FALSE;


        /* Hack -- handle delayed "flush()" */
        if (flush_later)
        {
                /* Done */
                flush_later = FALSE;

                /* Cancel "macro" info */
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
	if (!inkey_scan && (!inkey_flag))
	{
		/* Show the cursor */
		(void)Term_set_cursor(1);
	}


	/* Hack -- Activate the screen */
	Term_activate(term_screen);


	/* Get a (non-zero) keypress */
	for (ch = 0; !ch; )
	{
		/* Flush output - maintain flickering/multi-hued characters */
		do_flicker();

		/* Nothing ready, not waiting, and not doing "inkey_base" */
		if (!inkey_base && inkey_scan && (0 != Term_inkey(&ch, FALSE, FALSE))) break;


		/* Hack -- flush the output once no key is ready */
		if (!done && (0 != Term_inkey(&ch, FALSE, FALSE)))
		{
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
		if (inkey_base)
		{
			char xh;

			/* Check for keypress, optional wait */
			(void)Term_inkey(&xh, !inkey_scan, TRUE);

			/* Key ready */
			if (xh)
			{
				/* Reset delay */
				w = 0;

				/* Mega-Hack */
				if (xh == 28)
				{
					/* Toggle "skipping" */
					skipping = !skipping;
				}

				/* Use normal keys */
				else if (!skipping)
				{
					/* Use it */
					ch = xh;
				}
			}

			/* No key ready */
			else
			{
				/* Increase "wait" */
				w += 10;

				/* Excessive delay */
				if (w >= 100) break;

				/* Delay */
				Term_xtra(TERM_XTRA_DELAY, w);
			}

			/* Continue */
			continue;
		}


		/* Get a key (see above) */
		kk = ch = inkey_aux();


		/* Finished a "control-underscore" sequence */
		if (parse_under && (ch <= 32))
		{
			/* Found the edge */
			parse_under = FALSE;

			/* Stop stripping */
			strip_chars = FALSE;

			/* Strip this key */
			ch = 0;
		}


		/* Finished a "control-backslash" sequence */
		if (parse_slash && (ch == 28))
		{
			/* Found the edge */
			parse_slash = FALSE;

			/* Stop stripping */
			strip_chars = FALSE;

			/* Strip this key */
			ch = 0;
		}


		/* Handle some special keys */
		switch (ch)
		{
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
	inkey_base = inkey_scan = inkey_flag = FALSE;


	/* Return the keypress */
	return (ch);
}

static int hack_dir = 0;

/*
 * Convert a "Rogue" keypress into an "Angband" keypress
 * Pass extra information as needed via "hack_dir"
 *
 * Note that many "Rogue" keypresses encode a direction.
 */
static char roguelike_commands(char command)
{
        /* Process the command */
        switch (command)
        {
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

                /* Hack -- White-space */
                case KTRL('M'): return ('\r');

                /* Allow use of the "destroy" command */
                case KTRL('D'): return ('k');

                /* Hack -- Commit suicide */
                case KTRL('C'): return ('Q');

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
static char original_commands(char command)
{
	/* Process the command */
	switch(command)
	{
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

		/* Hack -- Commit suicide */
		case KTRL('K'): return ('Q');
		case KTRL('C'): return ('Q');
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
void keymap_init(void)
{
	int i, k;

	/* Initialize every entry */
	for (i = 0; i < 128; i++)
	{
		/* Default to "no direction" */
		hack_dir = 0;

		/* Attempt to translate */
		if (c_cfg.rogue_like_commands)
		{
			k = roguelike_commands(i);
		}
		else
		{
			k = original_commands(i);
		}

		/* Save the keypress */
		keymap_cmds[i] = k;

		/* Save the direction */
		keymap_dirs[i] = hack_dir;
	}
}


/*
 * Flush the screen, make a noise
 */
void bell(void)
{
	/* Mega-Hack -- Flush the output */
	Term_fresh();

	/* Make a bell noise (if allowed) */
	if (c_cfg.ring_bell) Term_xtra(TERM_XTRA_NOISE, 0);

	/* Flush the input (later!) */
	flush();
}

/*
 * Display a string on the screen using an attribute, and clear
 * to the end of the line.
 */
void c_prt(byte attr, cptr str, int row, int col)
{
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
void prt(cptr str, int row, int col)
{
	/* Spawn */
	c_prt(TERM_WHITE, str, row, col);
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
 * TODO: cursor editing
 */
bool askfor_aux(char *buf, int len, char private, char chatting)
{
	int y, x;

	int i = 0;

	int k = 0;

	bool done = FALSE;
	
	/* Hack -- if short, don't use history */
	bool nohist = private || len < 20;
	byte cur_hist;

	if (chatting)
		cur_hist = hist_chat_end;
	else
		cur_hist = hist_end;


	/* Handle wrapping */
	if (cur_hist >= MSG_HISTORY_MAX) cur_hist = 0;

	if (chatting && chat_mode)
	{
		/* HACK - Change the prompt */
		switch (chat_mode)
		{
			case CHAT_MODE_PARTY:
				c_prt(TERM_L_GREEN, "Party: ", 0, 0);
				break;
			case CHAT_MODE_LEVEL:
				c_prt(TERM_YELLOW, "Level: ", 0, 0);
				break;
			default:
				prt("Message: ", 0, 0);
				break;
		}
	}

	/* Locate the cursor */
	Term_locate(&x, &y);

	/* The top line is "icky" */
	topline_icky = TRUE;

	/* Paranoia -- check len */
	if (len < 1) len = 1;

	/* Paranoia -- check column */
	if ((x < 0) || (x >= 80)) x = 0;

	/* Restrict the length */
	if (x + len > 80) len = 80 - x;


	/* Paranoia -- Clip the default entry */
	buf[len] = '\0';

	/* Display the default answer */
	Term_erase(x, y, len);
	if (private) Term_putstr(x, y, -1, TERM_YELLOW, "(default)");
	else Term_putstr(x, y, -1, TERM_YELLOW, buf);

	if (chatting) strcpy(message_history_chat[hist_chat_end], buf);
	else if (!nohist) strcpy(message_history[hist_end], buf);


	/* Process input */
	while (!done)
	{
		/* Place cursor */
		Term_gotoxy(x + k, y);

		/* Get a key */
		i = inkey();

		/* Analyze the key */
		switch (i)
		{
			case ESCAPE:
			case KTRL('X'):
			k = 0;
			done = TRUE;
			break;

			case '\n':
			case '\r':
			k = strlen(buf);
			done = TRUE;
			break;

			case 0x7F:
			case '\010':
			if (k > 0) k--;
			break;

			case KTRL('I'): /* TAB */
			if (chatting)
			{
				/* Change chatting mode - mikaelh */
				chat_mode++;
				if (chat_mode > CHAT_MODE_LEVEL)
					chat_mode = CHAT_MODE_NORMAL;

				/* HACK - Change the prompt */
				switch (chat_mode)
				{
					case CHAT_MODE_PARTY:
						c_prt(TERM_L_GREEN, "Party: ", 0, 0);
						break;
					case CHAT_MODE_LEVEL:
						c_prt(TERM_YELLOW, "Level: ", 0, 0);
						break;
					default:
						prt("Message: ", 0, 0);
						break;
				}

				Term_locate(&x, &y);
			}
			break;

			case KTRL('U'):
			k = 0;
			break;

			case KTRL('N'):
			if (nohist) break;
			cur_hist++;

			if (chatting && !hist_chat_looped && hist_chat_end < cur_hist) cur_hist = 0;
			else if (!hist_looped && hist_end < cur_hist) cur_hist = 0;

			if (chatting)
				strcpy(buf, message_history_chat[cur_hist]);
			else
				strcpy(buf, message_history[cur_hist]);
			k = strlen(buf);
			break;

			case KTRL('P'):
			if (nohist) break;
			if (cur_hist) cur_hist--;
			else if (chatting) cur_hist = hist_chat_looped ? MSG_HISTORY_MAX - 1 : hist_chat_end;
			else cur_hist = hist_looped ? MSG_HISTORY_MAX - 1 : hist_end;
			if (chatting)
				strcpy(buf, message_history_chat[cur_hist]);
			else
				strcpy(buf, message_history[cur_hist]);
			k = strlen(buf);
			break;

			case KTRL('W'):
			{
				bool tail = FALSE;
				for (--k; k>=0; k--)
				{
					if ((buf[k] == ' ' || buf[k] == ':' || buf[k] == ',') && tail)
					{
						/* leave the last separator */
						k++;
						break;
					}
					tail = TRUE;
				}
				if (k < 0) k = 0;
				break;
			}

			default:
			if ((k < len) && (isprint(i)))
			{
				buf[k++] = i;
			}
			else
			{
				bell();
			}
			break;
		}

		/* Terminate */
		buf[k] = '\0';

		/* Update the entry */
		if (!private)
		{
			Term_erase(x, y, len);
			Term_putstr(x, y, -1, TERM_WHITE, buf);
		}
		else
		{
			Term_erase(x+k, y, len-k);
			if(k) Term_putch(x+k-1, y, TERM_WHITE, 'x');
		}
	}

	/* The top line is OK now */
	topline_icky = FALSE;
	if(!c_quit)
		Flush_queue();

	/* Aborted */
	if (i == ESCAPE) return (FALSE);

	/* Success */
	if (nohist) return (TRUE);

	/* Add this to the history */
	if (chatting)
	{
		strcpy(message_history_chat[hist_chat_end], buf);
		hist_chat_end++;
		if (hist_chat_end >= MSG_HISTORY_MAX)
		{
			hist_chat_end = 0;
			hist_chat_looped = TRUE;
		}
	}
	else
	{
		strcpy(message_history[hist_end], buf);
		hist_end++;
		if (hist_end >= MSG_HISTORY_MAX)
		{
			hist_end = 0;
			hist_looped = TRUE;
		}
	}

	/* Handle the additional chat modes */
	/* Slash commands are an exception */
	if (chatting && chat_mode != CHAT_MODE_NORMAL && buf[0] != '/')
	{
		for (i = k; i >= 0; i--)
		{
			buf[i + 2] = buf[i];
		}

		if (chat_mode == CHAT_MODE_PARTY)
			buf[0] = '!';
		else if (chat_mode == CHAT_MODE_LEVEL)
			buf[0] = '#';
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
bool get_string(cptr prompt, char *buf, int len)
{
	bool res;
	bool chatting = FALSE;

	/* Display prompt */
	prt(prompt, 0, 0);

	if (streq(prompt, "Message: "))
		chatting = TRUE;

	/* Ask the user for a string */
	res = askfor_aux(buf, len, 0, chatting);

	/* Clear prompt */
	prt("", 0, 0);

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
bool get_com(cptr prompt, char *command)
{
	/* The top line is "icky" */
	topline_icky = TRUE;

	/* Display a prompt */
	prt(prompt, 0, 0);

	/* Get a key */
	*command = inkey();

	/* Clear the prompt */
	prt("", 0, 0);

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
void request_command(bool shopping)
{
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
	c_msg_print(NULL);

	/* Clear top line */
	prt("", 0, 0);

	/* Bypass "keymap" */
	if (cmd == '\\')
	{
		/* Get a char to use without casting */
		(void)(get_com("Command: ", &cmd));

		/* Hack -- allow "control chars" to be entered */
		if (cmd == '^')
		{
			/* Get a char to "cast" into a control char */
			(void)(get_com("Command: Control: ", &cmd));

			/* Convert */
			cmd = KTRL(cmd);
		}

		/* Use the key directly */
		command_cmd = cmd;
	}

	else
	{
		/* Hack -- allow "control chars" to be entered */
		if (cmd == '^')
		{
			/* Get a char to "cast" into a control char */
			(void)(get_com("Control: ", &cmd));

			/* Convert */
			cmd = KTRL(cmd);
		}

		/* Access the array info */
		command_cmd = keymap_cmds[cmd & 0x7F];
		command_dir = keymap_dirs[cmd & 0x7F];

		/* Hack -- always raw key when in a store */
		if (shopping) command_cmd = cmd;
	}

	/* Paranoia */
	if (!command_cmd) command_cmd = ESCAPE;

	/* Hack -- erase the message line. */
	prt("", 0, 0);
}

bool get_dir(int *dp)
{
	int	dir = 0;

	char	command;

	cptr	p;

	p = "Direction ('*' to choose a target, non-direction cancels) ? ";

	get_com(p, &command);

	/* Handle target request */
	if (command == '*')
	{
		if (cmd_target())
			dir = 5;
	}

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
void c_put_str(byte attr, cptr str, int row, int col)
{
	/* Position cursor, Dump the attr/text */
	Term_putstr(col, row, -1, attr, (char*)str);
}


/*
 * As above, but in "white"
 */
void put_str(cptr str, int row, int col)
{
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
bool get_check(cptr prompt)
{
	int i;

	char buf[80];

	/* Hack -- Build a "useful" prompt */
	strnfmt(buf, 78, "%.70s[y/n] ", prompt);

	/* The top line is "icky" */
	topline_icky = TRUE;

	/* Prompt for it */
	prt(buf, 0, 0);

	/* Get an acceptable answer */
	while (TRUE)
	{
		i = inkey();
		if (c_cfg.quick_messages) break;
		if (i == ESCAPE) break;
		if (strchr("YyNn", i)) break;
		bell();
	}

	/* Erase the prompt */
	prt("", 0, 0);

	/* The top line is OK again */
	topline_icky = FALSE;

	/* Flush any events that came in while we were icky */
	if(!c_quit)
		Flush_queue();

	/* Normal negation */
	if ((i != 'Y') && (i != 'y')) return (FALSE);

	/* Success */
	return (TRUE);
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


/*
 * How many messages are "available"?
 */
s32b message_num(void)
{
        int last, next, n;

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


/*
 * Add a new message, with great efficiency
 */
void c_message_add(cptr str)
{
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
		if (str[0]=='[')
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

	/* Window stuff - assume that all chat messages start with '[' */
        if (str[0]=='[')
        p_ptr->window |= (PW_MESSAGE | PW_CHAT);
        else
        p_ptr->window |= (PW_MESSAGE | PW_MSGNOCHAT);
//      p_ptr->window |= PW_MESSAGE;

}
void c_message_add_chat(cptr str)
{
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
void c_message_add_msgnochat(cptr str)
{
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
        for (i = message__next; k; k--)
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
void c_msg_print(cptr msg)
{
	static int p = 0;

	int n;

	char *t;

	char buf[1024];
	/* Copy it */
	if (msg) strcpy(buf, msg);

	/* For message separation (chat/non-chat): */
	char nameA[20];
	char nameB[20];
	cptr msg_deadA = "You have been killed";
	cptr msg_deadB = "You die";
	cptr msg_unique = "was slain by ";
	cptr msg_killed = "was killed ";
	cptr msg_killed2 = "was annihilated ";
	cptr msg_killed3 = "was vaporized ";
	cptr msg_destroyed = "was destroyed ";
	cptr msg_killedF = "by Morgoth, Lord of Darkness"; /* for fancy death messages */
	cptr msg_suicide = "committed suicide.";
	cptr msg_entered = "has entered the game.";
	cptr msg_left = "has left the game.";
	cptr msg_event = " wins ";
	cptr msg_winner = "is henceforth known as";
	cptr msg_quest = "has won the";
	cptr msg_dice = "dice and get";
	cptr msg_level = "Welcome to level";
	cptr msg_level2 = "has attained level";
	cptr msg_gained_ability = "\377G*";
/* don't flood the 5th chat-only window with destroy-msgs,
   ctrl+o in main window should be sufficient */
/*	cptr msg_inven_destroy1 = "\377oYour ";
        cptr msg_inven_destroy2 = "\377oOne of your ";
        cptr msg_inven_destroy3 = "\377oSome of your ";
        cptr msg_inven_destroy4 = "\377oAll of your "; */
/*	cptr msg_inven_destroy1 = "was destroyed!";
        cptr msg_inven_destroyx = "were destroyed!";*/
	cptr msg_nopkfight = "You have beaten";
	cptr msg_nopkfight2 = "has beaten you";
	cptr msg_bloodbond = "blood bonds";
	cptr msg_bloodbond2 = "ou blood bond";
	cptr msg_bloodbond3 = "won the blood bond";
	cptr msg_challenge = "challenges";
	cptr msg_defeat = "has defeated";
	cptr msg_retire = "has retired";
	cptr msg_fruitbat = "turned into a fruit bat";
	cptr msg_afk1 = "seems to be AFK now";
        cptr msg_afk2 = "has returned from AFK";

	strcpy(nameA, "[");  strcat(nameA, cname);  strcat(nameA, ":");
	strcpy(nameB, ":");  strcat(nameB, cname);  strcat(nameB, "]");


	/* Hack -- Reset */
	if (!msg_flag) p = 0;

	/* Keldon-Hack -- Always reset */
	p = 0;
	if (!topline_icky) prt("", 0, 0);

	/* Message length */
	n = (msg ? strlen(msg) : 0);

	/* Hack -- flush when requested or needed */
	if (p && (!msg || ((p + n) > 72)))
	{
		/* Flush */
		/*msg_flush(p);*/

		/* Forget it */
		msg_flag = FALSE;

		/* Reset */
		p = 0;
	}


	/* No message */
	if (!msg) return;

	/* Paranoia */
	if (n > 1000) return;

#if 0 //we have PKT_AFK now (4.4.0) - C. Blue
	/* Ok, bad hack - Sorry ;) - C. Blue */
	if (strstr(msg, "AFK mode is turned \377rON\377w.") - msg == 0) {
		p_ptr->afk = TRUE;
		c_put_str(TERM_ORANGE, "AFK", 22, 0);
	}
	if (strstr(msg, "AFK mode is turned \377GOFF\377w.") - msg == 0) {
		p_ptr->afk = FALSE;
		put_str("   ", 22, 0);
	}
#endif

	/* Memorize the message */
#if 1
	if ((strstr(msg, nameA) != NULL) || (strstr(msg, nameB) != NULL) || (msg[0] == '[') ||
	    (strstr(msg, msg_killed) != NULL) || (strstr(msg, msg_killed2) != NULL) ||
	    (strstr(msg, msg_killed3) != NULL) || (strstr(msg, msg_destroyed) != NULL) ||
	    (strstr(msg, msg_killedF) != NULL) ||
	    (strstr(msg, msg_unique) != NULL) || (strstr(msg, msg_suicide) != NULL) ||
	    (strstr(msg, msg_entered) != NULL) || (strstr(msg, msg_left) != NULL) ||
	    (strstr(msg, msg_event) != NULL) || (strstr(msg, msg_winner) != NULL) ||
	    (strstr(msg, msg_quest) != NULL) || (strstr(msg, msg_dice) != NULL) ||
	    (strstr(msg, msg_level) != NULL) || (strstr(msg, msg_level2) != NULL) ||
	    (strstr(msg, msg_gained_ability) != NULL) ||
	    (strstr(msg, msg_deadA) != NULL) || (strstr(msg, msg_deadB) != NULL) ||
/* don't flood the 5th window with destroy-msgs */
/*	    (strstr(msg, msg_inven_destroy1) != NULL) || (strstr(msg, msg_inven_destroy2) != NULL) ||
	    (strstr(msg, msg_inven_destroy3) != NULL) || (strstr(msg, msg_inven_destroy4) != NULL) ||
//	    (strstr(msg, msg_inven_destroy1) != NULL) || (strstr(msg, msg_inven_destroyx) != NULL) ||
	    (strstr(msg, msg_inven_steal1) != NULL) || (strstr(msg, msg_inven_steal2) != NULL) ||
*/
	    (strstr(msg, msg_nopkfight) != NULL) || (strstr(msg, msg_nopkfight2) != NULL) ||
	    (strstr(msg, msg_bloodbond) != NULL) || (strstr(msg, msg_bloodbond2) != NULL) ||
	    (strstr(msg, msg_bloodbond3) != NULL) ||
	    (strstr(msg, msg_challenge) != NULL) || (strstr(msg, msg_defeat) != NULL) || 
	    (strstr(msg, msg_retire) != NULL) ||
	    (strstr(msg, msg_afk1) != NULL) || (strstr(msg, msg_afk2) != NULL) ||
	    (strstr(msg, msg_fruitbat) != NULL) || (msg[2] == '[') ||
	    (msg[0] == '~' && was_chat_buffer)) {
/*	if ((strstr(msg, nameA) != NULL) || (strstr(msg, nameB) != NULL) || (msg[2] == '[')) {*/
		if (msg[2] == '[') was_real_chat = TRUE;
		else if (msg[0] != '~') was_real_chat = FALSE;

		if (msg[0] == '~') buf[0] = ' ';
		c_message_add_chat(buf);

		was_chat_buffer = TRUE;
	} else {
		was_real_chat = FALSE;
		was_chat_buffer = FALSE;
	}
/*#if 0*/
	if (!was_real_chat) {
		if (msg[0] == '~') buf[0] = ' ';
		c_message_add_msgnochat(buf);
	}
#endif
	if (msg[0] == '~') buf[0] = ' ';
	c_message_add(buf);


	/* Analyze the buffer */
	t = buf;

#if 0 /* Done on server-side now - mikaelh */
	/* Split message */
	while (n > 72)
	{
		char oops;

		int check, split;

		/* Default split */
		split = 72;

		/* Find the "best" split point */
		for (check = 40; check < 72; check++)
		{
			/* Found a valid split point */
			if (t[check] == ' ') split = check;
		}

		/* Save the split character */
		oops = t[split];

		/* Split the message */
		t[split] = '\0';

		/* Display part of the message */
		Term_putstr(0, 0, split, TERM_WHITE, t);

		/* Flush it */
		/*msg_flush(split + 1);*/

		/* Restore the split character */
		t[split] = oops;

		/* Insert a space */
		t[--split] = ' ';

		/* Prepare to recurse on the rest of "buf" */
		t += split; n -= split;
	}
#else
	/* Small length limit */
	if (n > 80) n = 80;
#endif


	/* Display the tail of the message */
	if (!topline_icky) Term_putstr(p, 0, n, TERM_WHITE, t);

	/* Remember the message */
	msg_flag = TRUE;

	/* Remember the position */
	p += n + 1;
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
 */
s32b c_get_quantity(cptr prompt, int max)
{
	int amt;

	char tmp[80];

	char buf[80];
	
	char bi1[80], bi2[6 + 1];
	int n = 0, i = 0, j = 0;
	s32b i1 = 0, i2 = 0, mul = 1;

	/* Build a prompt if needed */
	if (!prompt)
	{
		/* Build a prompt */
		sprintf(tmp, "Quantity (1-%d): ", max);

		/* Use that prompt */
		prompt = tmp;
	}


	/* Default to one */
	amt = 1;

	/* Build the default */
	sprintf(buf, "%d", amt);

	/* Ask for a quantity */
	if (!get_string(prompt, buf, 8)) return (0);


#if 1
	/* new method for inputting amounts of gold:  1m35 = 1,350,000  - C. Blue */
	while(buf[n] >= '0' && buf[n] <= '9') bi1[i++] = buf[n++];
	bi1[n] = '\0';
	i1 = atoi(bi1);
	if (buf[n] == 'k' || buf[n] == 'K') mul = 1000;
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
	else
		if (strchr(buf,'m'))
			amt=amt * 1000000;
#endif


	/* A letter means "all" */
	if (isalpha(buf[0])) amt = max;
	/* hack for drop gold: - C. Blue */
	if (buf[0] == '*') {
		if (max > 0) amt = max;
		else amt = 100000000; /* 1 more than you could actually type :) (8 chars limit) */
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
	(void)sprintf(out_val, "%6ld", (long)num);
	c_put_str(color, out_val, row, col + len + 3);
}

void prt_lnum(cptr header, s32b num, int row, int col, byte color)
{
	int len = strlen(header);
	char out_val[32];
	put_str(header, row, col);
	(void)sprintf(out_val, "%9ld", (long)num);
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

void interact_macros(void)
{
	int i;

	char tmp[160], buf[1024], buf2[1024], *bptr, *b2ptr;

	/* Save screen */
	Term_save();

	/* No macros should work within the macro menu itself */
	inkey_interact_macros = TRUE;

	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Describe */
		Term_putstr(0, 1, -1, TERM_L_GREEN, "Interact with Macros");


		/* Describe that action */
		Term_putstr(0, 19, -1, TERM_L_GREEN, "Current action (if any) shown below:");

		/* Analyze the current action */
		ascii_to_text(buf, macro__buf);

		/* Display the current action */
		Term_putstr(0, 21, -1, TERM_WHITE, buf);

		/* Selections */
		Term_putstr(5,  3, -1, TERM_WHITE, "(1) Load macros from a pref file");
		Term_putstr(5,  4, -1, TERM_WHITE, "(2) Save macros to a pref file");
		Term_putstr(5,  5, -1, TERM_WHITE, "(3) Enter a new macro action");
		Term_putstr(5,  6, -1, TERM_SLATE, "(4) Create a normal macro       (persists everywhere)");
		Term_putstr(5,  7, -1, TERM_WHITE, "(5) Create a hybrid macro       (recommended for most cases)");
		Term_putstr(5,  8, -1, TERM_SLATE, "(6) Create a command macro      (eg for using / and * key)");
//		Term_putstr(5,  9, -1, TERM_SLATE, "(7) Create a identity macro  (erases a macro)");
		Term_putstr(5,  9, -1, TERM_SLATE, "(7) Delete a macro from a key");
		Term_putstr(5, 10, -1, TERM_SLATE, "(8) Create an empty macro       (completely disables a key)");
		Term_putstr(5, 11, -1, TERM_WHITE, "(9) Query an existing macro on a key");
		Term_putstr(5, 12, -1, TERM_SLATE, "(q/Q) Enter and create a 'quick & dirty' macro / set preferences"),
//TODO		Term_putstr(5, 13, -1, TERM_WHITE, "(r/R) Record a macro / set preferences");

		/* Prompt */
		Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		/* Take a screenshot */
		else if (i == KTRL('T'))
		{
			xhtml_screenshot("screenshotXXXX");
		}

		/* Load a pref file */
		else if (i == '1')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Load a user pref file");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 17, -1, TERM_WHITE, "File: ");

			/* Default filename */
//			sprintf(tmp, "user-%s.prf", ANGBAND_SYS);
			/* Use the character name by default - mikaelh */
			sprintf(tmp, "%s.prf", cname);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0, 0)) continue;

			/* Process the given filename */
			(void)process_pref_file(tmp);

			/* Pref files may change settings, so reload the keymap - mikaelh */
			keymap_init();
		}

		/* Save a 'macro' file */
		else if (i == '2')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Save a macro file");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 17, -1, TERM_WHITE, "File: ");

			/* Default filename */
//			sprintf(tmp, "user-%s.prf", ANGBAND_SYS);
			/* Use the character name by default - mikaelh */
			sprintf(tmp, "%s.prf", cname);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0, 0)) continue;

			/* Dump the macros */
			(void)macro_dump(tmp);
		}

		/* Enter a new action */
		else if (i == '3')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Enter a new action");

			/* Go to the correct location */
			Term_gotoxy(0, 21);

			/* Hack -- limit the value */
			tmp[80] = '\0';

			/* Get an encoded action */
			if (!askfor_aux(buf, 80, 0, 0)) continue;

			/* Extract an action */
			text_to_ascii(macro__buf, buf);
		}

		/* Create a command macro */
		else if (i == '6')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Create a command macro");

			/* Prompt */
			Term_putstr(0, 17, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);
			
			/* Some keys aren't allowed to prevent the user 
			   from locking himself out accidentally */
			if (!strcmp(buf, "\e") || !strcmp(buf, "%")) {
				c_msg_print("Keys <ESC> and '%' aren't allowed to carry a macro.");
			} else {
				/* Link the macro */
				macro_add(buf, macro__buf, TRUE, FALSE);
				/* Message */
				c_msg_print("Created a new command macro.");
			}
		}

		/* Create a hybrid macro */
		else if (i == '5')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Create a hybrid macro");

			/* Prompt */
			Term_putstr(0, 17, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			/* Some keys aren't allowed to prevent the user 
			   from locking himself out accidentally */
			if (!strcmp(buf, "\e") || !strcmp(buf, "%")) {
				c_msg_print("Keys <ESC> and '%' aren't allowed to carry a macro.");
			} else {
				/* Link the macro */
				macro_add(buf, macro__buf, FALSE, TRUE);
				/* Message */
				c_msg_print("Created a new hybrid macro.");
			}
		}

		/* Create a normal macro */
		else if (i == '4')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Create a normal macro");

			/* Prompt */
			Term_putstr(0, 17, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			/* Some keys aren't allowed to prevent the user 
			   from locking himself out accidentally */
			if (!strcmp(buf, "\e") || !strcmp(buf, "%")) {
				c_msg_print("Keys <ESC> and '%' aren't allowed to carry a macro.");
			} else {
				/* Link the macro */
				macro_add(buf, macro__buf, FALSE, FALSE);
				/* Message */
				c_msg_print("Created a new normal macro.");
			}
		}

#if 0
		/* Create an identity macro */
		else if (i == '7')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Create an identity macro");

			/* Prompt */
			Term_putstr(0, 17, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			/* Link the macro */
			macro_add(buf, buf, FALSE, FALSE);

			/* Message */
			c_msg_print("Created a new identity macro.");
		}
#else
		/* Delete a macro */
		else if (i == '7')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Delete a macro");

			/* Prompt */
			Term_putstr(0, 17, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			/* Delete the macro */
			if (macro_del(buf))
				c_msg_print("The macro was deleted.");
			else
				c_msg_print("No macro was found.");
		}
#endif

		/* Create an empty macro */
		else if (i == '8')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Create an empty macro");

			/* Prompt */
			Term_putstr(0, 17, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			/* Some keys aren't allowed to prevent the user 
			   from locking himself out accidentally */
			if (!strcmp(buf, "\e") || !strcmp(buf, "%")) {
				c_msg_print("Keys <ESC> and '%' aren't allowed to carry a macro.");
			} else {
				/* Link the macro */
				macro_add(buf, "", FALSE, FALSE);
				/* Message */
				c_msg_print("Created a new empty macro.");
			}
		}

		/* Query a macro */
		else if (i == '9')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Query a macro");

			/* Prompt */
			Term_putstr(0, 17, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			/* Re-using 'i' here shouldn't matter anymore */
			for (i = 0; i < macro__num; i++)
			{
				if (streq(macro__pat[i], buf))
				{
					strncpy(macro__buf, macro__act[i], 159);
					macro__buf[159] = '\0';

					/* Message */
					if (macro__hyb[i]) c_msg_print("A hybrid macro was found.");
					else if (macro__cmd[i]) c_msg_print("A command macro was found.");
					else c_msg_print("A normal macro was found.");

					/* Update windows */
					window_stuff();

					break;
				}
			}

			if (i == macro__num)
			{
				/* Message */
				c_msg_print("No macro was found.");
			}
		}

		/* Enter a 'quick & dirty' macro */
		else if (i == 'q')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Enter a new 'quick & dirty' macro");

			/* Go to the correct location */
			Term_gotoxy(0, 21);

			/* Hack -- limit the value */
			tmp[80] = '\0';

			/* Get an encoded action */
			if (!askfor_aux(buf, 80, 0, 0)) continue;
			
			/* Fix it up quick and dirty: Ability code short-cutting */
			buf2[0] = '\\'; //note: should in theory be ')e\',
			buf2[1] = 'e'; //      but doesn't work due to prompt behaviour 
			buf2[2] = ')'; //      (\e will then get ignored)
			bptr = buf;
			b2ptr = buf2 + 3;
			while (*bptr) {
				switch (*bptr) {
				case 'M': /* use innate mimic power */
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '3'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
					bptr++;	break;
				case 'F': /* employ fighting technique */
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '5'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
					bptr++;	break;
				case 'S': /* employ shooting technique */
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '6'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
					bptr++;	break;
				case 'p': /* set a trap */
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '1'; *b2ptr++ = '0'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
					bptr++;	break;
				case 'm': /* cast a spell */
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '1'; *b2ptr++ = '1'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
					bptr++;	break;
				case 's': /* change stance */
					*b2ptr++ = 'm'; *b2ptr++ = '@'; *b2ptr++ = '1'; *b2ptr++ = '3'; *b2ptr++ = '\\'; *b2ptr++ = 'r';
					bptr++;	break;
				case '*': /* set a target */
					*b2ptr++ = '*'; *b2ptr++ = 't';
					bptr++;	break;
				default:
					*b2ptr++ = *bptr++;
				}
			}
			/* terminate anyway */
			*b2ptr = '\0';

			/* Display the current action */
        		Term_putstr(0, 21, -1, TERM_WHITE, buf2);

			/* Extract an action */
			text_to_ascii(macro__buf, buf2);
			
			/* Prompt */
			Term_putstr(0, 17, -1, TERM_WHITE, "Trigger: ");

			/* Get a macro trigger */
			get_macro_trigger(buf);

			/* Some keys aren't allowed to prevent the user 
			   from locking himself out accidentally */
			if (!strcmp(buf, "\e") || !strcmp(buf, "%")) {
				c_msg_print("Keys <ESC> and '%' aren't allowed to carry a macro.");
			} else {
				/* Automatically choose usually best fitting macro type,
				   depending on chosen trigger key! */
				//[normal macros: F-keys (only keys that aren't used for any text input)]
				//command macros: / * a..w (all keys that are used in important standard prompts)
				//hybrid macros: all others, maybe even also normal-macro-keys
				if (!strcmp(buf, "/") || !strcmp(buf, "*") || (*buf >= 'a' && *buf <= 'w')) {
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
		else if (i == 'Q')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Configure 'quick & dirty' macro functionality");
			
			/* TODO:
			   config auto-prefix '\e)' */
		}

		/* Start recording a macro */
		else if (i == 'r')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Record a macro");
			
			/* TODO: implement ^^ */

		}
		/* Configure macro recording functionality */
		else if (i == 'R')
		{
			/* Prompt */
			Term_putstr(0, 15, -1, TERM_L_GREEN, "Command: Configure macro recording functionality");

			/* TODO: implement */
		}

		/* Oops */
		else
		{
			/* Oops */
			bell();
		}
	}

	/* Reload screen */
	Term_load();

	/* Flush the queue */
	Flush_queue();

	inkey_interact_macros = FALSE;
}


/*
 * Interact with some options
 */
static void do_cmd_options_aux(int page, cptr info)
{
	char	ch;

	int		i, k = 0, n = 0;

	int		opt[24];

	char	buf[80];


	/* Lookup the options */
	for (i = 0; i < 24; i++) opt[i] = 0;

	/* Scan the options */
	for (i = 0; option_info[i].o_desc; i++)
	{
		/* Notice options on this "page" */
		if (option_info[i].o_page == page) opt[n++] = i;
	}


	/* Clear screen */
	Term_clear();

	/* Interact with the player */
	while (TRUE)
	{
		/* Prompt XXX XXX XXX */
		sprintf(buf, "%s (RET to advance, y/n to set, ESC to accept) ", info);
		prt(buf, 0, 0);

		/* Display the options */
		for (i = 0; i < n; i++)
		{
			byte a = TERM_WHITE;

			/* Color current option */
			if (i == k) a = TERM_L_BLUE;

			/* Display the option text */
			sprintf(buf, "%-48s: %s  (%s)",
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
		switch (ch)
		{
			case ESCAPE:
			case KTRL('X'):
			{
				return;
			}

			case KTRL('T'):
			{
				/* Take a screenshot */
				xhtml_screenshot("screenshotXXXX");
				break;
			}

			case '-':
			case '8':
			case 'k':
			{
				k = (n + k - 1) % n;
				break;
			}

			case ' ':
			case '\n':
			case '\r':
			case '2':
			case 'j':
			{
				k = (k + 1) % n;
				break;
			}

			case 'y':
			case 'Y':
			case '6':
			case 'l':
			{
				(*option_info[opt[k]].o_var) = TRUE;
				Client_setup.options[opt[k]] = TRUE;
				k = (k + 1) % n;
				break;
			}

			case 'n':
			case 'N':
			case '4':
			case 'h':
			{
				(*option_info[opt[k]].o_var) = FALSE;
				Client_setup.options[opt[k]] = FALSE;
				k = (k + 1) % n;
				break;
			}

			case 't':
			case 'T':
			case '5':
			case 's':
			{
				bool tmp = 1 - (*option_info[opt[k]].o_var);
				(*option_info[opt[k]].o_var) = tmp;
				Client_setup.options[opt[k]] = tmp;
				k = (k + 1) % n;
				break;
			}
			default:
			{
				bell();
				break;
			}
		}
	}
}


/*
 * Modify the "window" options
 */
static void do_cmd_options_win(void)
{
	int i, j, d, vertikal_offset = 1;

	int y = 0;
	int x = 0;

	char ch;

	bool go = TRUE;

	u32b old_flag[8];


	/* Memorize old flags */
	for (j = 0; j < 8; j++)
	{
		/* Acquire current flags */
		old_flag[j] = window_flag[j];
	}


	/* Clear screen */
	Term_clear();

	/* Interact */
	while (go)
	{
		/* Prompt XXX XXX XXX */
		prt("Window flags (<dir>, t, y, n, ESC) ", 0, 0);

		/* Display the windows */
		for (j = 0; j < 8; j++)
		{
			byte a = TERM_WHITE;

			cptr s = ang_term_name[j];

			/* Use color */
			if (c_cfg.use_color && (j == x)) a = TERM_L_BLUE;

			/* Window name, staggered, centered */
			Term_putstr(35 + j * 5 - strlen(s) / 2, vertikal_offset + j % 2, -1, a, (char*)s);
		}

		/* Display the options */
		for (i = 0; i < NR_OPTIONS_SHOWN; i++)
		{
			byte a = TERM_WHITE;

			cptr str = window_flag_desc[i];

			/* Use color */
			if (c_cfg.use_color && (i == y)) a = TERM_L_BLUE;

			/* Unused option */
			if (!str) str = "(Unused option)";

			/* Flag name */
			Term_putstr(0, i + vertikal_offset + 2, -1, a, (char*)str);

			/* Display the windows */
			for (j = 0; j < 8; j++)
			{
				byte a = TERM_WHITE;

				char c = '.';

				/* Use color */
				if (c_cfg.use_color && (i == y) && (j == x)) a = TERM_L_BLUE;

				/* Active flag */
				if (window_flag[j] & (1L << i)) c = 'X';

				/* Flag value */
				Term_putch(35 + j * 5, i + vertikal_offset + 2, a, c);
			}
		}

		/* Place Cursor */
		Term_gotoxy(35 + x * 5, y + vertikal_offset + 2);

		/* Get key */
		ch = inkey();

		/* Analyze */
		switch (ch)
		{
			case ESCAPE:
			{
				go = FALSE;
				break;
			}

			case KTRL('T'):
			{
				/* Take a screenshot */
				xhtml_screenshot("screenshotXXXX");
				break;
			}

			case 'T':
			case 't':
			{
				/* Clear windows */
				for (j = 0; j < 8; j++)
				{
					window_flag[j] &= ~(1L << y);
				}

				/* Clear flags */
				for (i = 0; i < NR_OPTIONS_SHOWN; i++)
				{
					window_flag[x] &= ~(1L << i);
				}

				/* Fall through */
			}

			case 'y':
			case 'Y':
			{
				/* Ignore screen */
				if (x == 0) break;

				/* Set flag */
				window_flag[x] |= (1L << y);
				break;
			}

			case 'n':
			case 'N':
			{
				/* Clear flag */
				window_flag[x] &= ~(1L << y);
				break;
			}

			default:
			{
				d = keymap_dirs[ch & 0x7F];

				x = (x + ddx[d] + 8) % 8;
				y = (y + ddy[d] + NR_OPTIONS_SHOWN) % NR_OPTIONS_SHOWN;

				if (!d) bell();
			}
		}
	}

	/* Notice changes */
	for (j = 0; j < 8; j++)
	{
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
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_MESSAGE | PW_PLAYER | PW_CHAT | PW_MSGNOCHAT);

	/* Update windows */
	window_stuff();
}


static errr options_dump(cptr fname)
{
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
	for (i = 0; i < OPT_MAX; i++)
	{
		/* Require a real option */
		if (!option_info[i].o_desc) continue;

		/* Comment */
		fprintf(fff, "# Option '%s'\n", option_info[i].o_desc);

		/* Dump the option */
		if (*option_info[i].o_var)
		{
			fprintf(fff, "Y:%s\n", option_info[i].o_text);
		}
		else
		{
			fprintf(fff, "X:%s\n", option_info[i].o_text);
		}

		/* End the option */
		fprintf(fff, "\n");
	}

	fprintf(fff, "\n");

	/* Dump window flags */
#if 0
	for (i = 1; i < ANGBAND_TERM_MAX; i++)
#endif
	for (i = 1; i < 8; i++)
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


/*
 * Set or unset various options.
 *
 * The user must use the "Ctrl-R" command to "adapt" to changes
 * in any options which control "visual" aspects of the game.
 */
void do_cmd_options(void)
{
	int k;
	char tmp[1024];


	/* Save the screen */
	Term_save();


	/* Interact */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Why are we here */
		prt("Angband options", 2, 0);

		/* Give some choices */
		prt("(1) User Interface Options", 4, 5);
		prt("(2) Disturbance Options", 5, 5);
		prt("(3) Game-Play Options", 6, 5);
		prt("(4) Efficiency Options", 7, 5);
		prt("(5) TomeNET Additional Options", 8, 5);

		prt("(8) Check Server Options", 10, 5);

		prt("(9) Save Options", 12, 5);
		prt("(0) Load Options", 13, 5);

		/* Window flags */
		prt("(W) Window flags", 15, 5);

		/* Prompt */
		prt("Command: ", 17, 0);

		/* Get command */
		k = inkey();

		/* Exit */
		if (k == ESCAPE || k == KTRL('X')) break;

		/* Take a screenshot */
		if (k == KTRL('T'))
		{
			xhtml_screenshot("screenshotXXXX");
		}

		/* General Options */
		if (k == '1')
		{
			/* Process the general options */
			do_cmd_options_aux(1, "User Interface Options");
		}

		/* Disturbance Options */
		else if (k == '2')
		{
			/* Process the running options */
			do_cmd_options_aux(2, "Disturbance Options");
		}

		/* Inventory Options */
		else if (k == '3')
		{
			/* Process the running options */
			do_cmd_options_aux(3, "Game-Play Options");
		}

		/* Efficiency Options */
		else if (k == '4')
		{
			/* Process the efficiency options */
			do_cmd_options_aux(4, "Efficiency Options");
		}

		/* Efficiency Options */
		else if (k == '5')
		{
			/* Process the efficiency options */
			do_cmd_options_aux(5, "TomeNET Options");
		}

		/* Server Options */
		else if (k == '8')
		{
			Send_special_line(SPECIAL_FILE_SERVER_SETTING, 0);
		}

		/* Save a 'option' file */
		else if (k == '9')
		{
			/* Prompt */
			Term_putstr(0, 17, -1, TERM_WHITE, "Command: Save an option file");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 19, -1, TERM_WHITE, "File: ");

			/* Default filename */
			sprintf(tmp, "user.prf");

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0, 0)) continue;

			/* Dump the macros */
			(void)options_dump(tmp);
		}
		/* Load a pref file */
		else if (k == '0')
		{
			/* Prompt */
			Term_putstr(0, 17, -1, TERM_WHITE, "Command: Load a user pref file");

			/* Get a filename, handle ESCAPE */
			Term_putstr(0, 19, -1, TERM_WHITE, "File: ");

			/* Default filename */
			sprintf(tmp, "user-%s.prf", ANGBAND_SYS);

			/* Ask for a file */
			if (!askfor_aux(tmp, 70, 0, 0)) continue;

			/* Process the given filename */
			(void)process_pref_file(tmp);
		}

		/* Window flags */
		else if (k == 'W')
		{
			/* Spawn */
			do_cmd_options_win();
		}

		/* Unknown option */
		else
		{
			/* Oops */
			bell();
		}
	}

	/* Restore the screen */
	Term_load();

	Flush_queue();

	/* Verify the keymap */
	keymap_init();

	/* Resend options to server */
	Send_options();
}


/*
 * Centers a string within a 31 character string		-JWT-
 */
static void center_string(char *buf, cptr str)
{
	int i, j;

	/* Total length */
	i = strlen(str);

	/* Necessary border */
	j = 15 - i / 2;

	/* Mega-Hack */
	(void)sprintf(buf, "%*s%s%*s", j, "", str, 31 - i - j, "");
}


/*
 * Display a "tomb-stone"
 */
/* ToME parts. */
static void print_tomb(cptr reason)
{
	bool done = FALSE;

	/* Print the text-tombstone */
	if (!done)
	{
		char	tmp[160];

		char	buf[1024];

		FILE        *fp;

		time_t	ct = time((time_t)0);


		/* Clear screen */
		Term_clear();

		/* Build the filename */
		path_build(buf, 1024, ANGBAND_DIR_HELP, ct % 2 ? "dead.txt" : "dead2.txt");

		/* Open the News file */
		fp = my_fopen(buf, "r");

		/* Dump */
		if (fp)
		{
			int i = 0;

			/* Dump the file to the screen */
			while (0 == my_fgets(fp, buf, 1024))
			{
				/* Display and advance */
				Term_putstr(0, i++, -1, TERM_WHITE, buf);
			}

			/* Close */
			my_fclose(fp);
		}

#if 0	/* make the server send those info! */
		/* King or Queen */
		if (total_winner || (p_ptr->lev > PY_MAX_LEVEL))
		{
			p = "Magnificent";
		}

		/* Normal */
		else
		{
			p =  cp_ptr->titles[(p_ptr->lev-1)/5] + c_text;
		}
#endif	/* 0 */

		center_string(buf, cname);
		put_str(buf, 6, 11);

		center_string(buf, "the");
		put_str(buf, 7, 11);

        	center_string(buf, race_info[race].title);
		put_str(buf, 8, 11);

        	center_string(buf, class_info[class].title);
		put_str(buf, 9, 11);

		(void)sprintf(tmp, "Level: %d", (int)p_ptr->lev);
		center_string(buf, tmp);
		put_str(buf, 11, 11);

		(void)sprintf(tmp, "Exp: %ld", (long)p_ptr->exp);
		center_string(buf, tmp);
		put_str(buf, 12, 11);

		/* XXX usually 0 */
		(void)sprintf(tmp, "AU: %ld", (long)p_ptr->au);
		center_string(buf, tmp);
		put_str(buf, 13, 11);

		if (c_cfg.depth_in_feet)
			(void)sprintf(tmp, "Died on %dft of [%2d, %2d]", p_ptr->wpos.wz * 50, p_ptr->wpos.wy, p_ptr->wpos.wx);
		else
			(void)sprintf(tmp, "Died on Level %d of [%2d, %2d]", p_ptr->wpos.wz, p_ptr->wpos.wy, p_ptr->wpos.wx);
		center_string(buf, tmp);
		put_str(buf, 14, 11);

#if 0
		(void)sprintf(tmp, "Killed on Level %d", dun_level);
		center_string(buf, tmp);
		put_str(buf, 14, 11);


		if (strlen(died_from) > 24)
		{
			strncpy(dummy, died_from, 24);
			dummy[24] = '\0';
			(void)sprintf(tmp, "by %s.", dummy);
		}
		else
			(void)sprintf(tmp, "by %s.", died_from);

		center_string(buf, tmp);
		put_str(buf, 15, 11);
#endif	/* 0 */


		(void)sprintf(tmp, "%-.24s", ctime(&ct));
		center_string(buf, tmp);
		put_str(buf, 17, 11);

		put_str(reason, 21, 10);
	}
}

/*
 * Display some character info	- Jir -
 * For now, only when losing the character.
 */
void c_close_game(cptr reason)
{
	int k;
	char tmp[1024];

	/* Let the player view the last scene */
	c_msg_format("%s ...Press '0' key to proceed", reason);

	while (inkey() != '0');

	/* You are dead */
	print_tomb(reason);

	put_str("ESC to quit, 'f' to dump the record or any other key to proceed", 23, 0);

	/* TODO: bandle them in one loop instead of 2 */
	while (1)
	{
		/* Get command */
		k = inkey();

		/* Exit */
		if (k == ESCAPE || k == KTRL('X') || k == 'q' || k == 'Q') return;

		else if (k == KTRL('T'))
		{
			/* Take a screenshot */
			xhtml_screenshot("screenshotXXXX");
		}

		/* Dump */
		else if ((k == 'f') || (k == 'F'))
		{
			strnfmt(tmp, 160, "%s.txt", cname);
			if (get_string("Filename(you can post it to http://angband.oook.cz/): ", tmp, 80))
			{
				if (tmp[0] && (tmp[0] != ' '))
				{
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
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Why are we here */
		prt("Your tracks", 2, 0);

		/* Give some choices */
		prt("(1) Character", 4, 5);
		prt("(2) Inventory", 5, 5);
		prt("(3) Equipments", 6, 5);
		prt("(4) Messages", 7, 5);
		prt("(5) Chat messages", 8, 5);

		/* What a pity no score list here :-/ */

		/* Prompt */
		prt("Command: ", 17, 0);

		/* Get command */
		k = inkey();

		/* Exit */
		if (k == ESCAPE || k == KTRL('X')) break;

		/* Take a screenshot */
		if (k == KTRL('T'))
		{
			xhtml_screenshot("screenshotXXXX");
		}

		/* Character screen */
		else if (k == '1' || k == 'C')
		{
			cmd_character();
		}

		/* Inventory */
		else if (k == '2' || k == 'i')
		{
			cmd_inven();
		}

		/* Equipments */
		else if (k == '3' || k == 'e')
		{
			/* Process the running options */
			cmd_equip();
		}

		/* Message history */
		else if (k == '4' || k == KTRL('P'))
		{
			do_cmd_messages();
		}

		/* Chat history */
		else if (k == '5' || k == KTRL('O'))
		{
			do_cmd_messages_chatonly();
		}

#if 0
		/* Skill browsing ... is not available for now */
		else if (k == '6' || k == KTRL('G'))
		{
			do_cmd_skill();
		}
#endif	/* 0 */

		/* Unknown option */
		else
		{
			/* Oops */
			bell();
		}
	}
}



/*
 * Since only GNU libc has memfrob, we use our own.
 */
void my_memfrob(void *s, int n)
{
        int i;
        char *str;

        str = (char*) s;

        for (i = 0; i < n; i++)
        {
                /* XOR every byte with 42 */
                str[i] ^= 42;
        }
}



#ifdef SET_UID

# ifndef HAS_USLEEP

/*
 * For those systems that don't have "usleep()" but need it.
 *
 * Fake "usleep()" function grabbed from the inl netrek server -cba
 */
int usleep(huge microSeconds)
{
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
	if (select(nfds, no_fds, no_fds, no_fds, &Timer) < 0)
	{
		/* Hack -- ignore interrupts */
		if (errno != EINTR) return -1;
	}

	/* Success */
	return 0;
}

# endif /* HAS_USLEEP */

#endif /* SET_UID */

#ifdef WIN32
int usleep(long microSeconds)
{
	Sleep(microSeconds/10); /* meassured in milliseconds not microseconds*/
	return(0);
}
#endif /* WIN32 */

/*
 * Check if the server version fills the requirements.
 * Copied from the server code.
 *
 * Branch has to be an exact match.
 */
bool is_newer_than(version_type *version, int major, int minor, int patch, int extra, int branch, int build)
{
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
