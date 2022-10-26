/* $Id$ */
/* More extra client things */

#include "angband.h"


/* When copying to clipboard, attempt to combine long chat messages that got
   split up into multiple lines together again. (msg_print() breaks them up.) */
#define COPY_MULTILINE


#ifdef COPY_MULTILINE
/* helper function */
static void copy_to_clipboard_multiline(cptr msg_raw, cptr *message_recall, int i, int s, int n, int nn) {
	if (msg_raw[0] == '\376') msg_raw++;

	/* Is this the following-up line of a multiline message? */
	if (msg_raw[0] == ' ' || (msg_raw[0] == '\377' && msg_raw[2] == ' ')) {
		int j;
		char xmsg[MSG_LEN], xmsg_reverse[MSG_LEN];
		const char *c;
		bool end = FALSE;

		c = msg_raw;
		if (*c == '\377') c += 2;
		while (*c == ' ') c++;
		strcpy(xmsg, " ");
		strcat(xmsg, c);
		/* avoid duplicate ' ' */
		if (xmsg[strlen(xmsg) - 1] == ' ') xmsg[strlen(xmsg) - 1] = 0;

		if (s == -1) { /* message_important */
			for (j = 0 + 1; (j < 20 + HGT_PLUS) && (i + j < n); j++) {
				c = message_recall[nn - 1 - (i + j)]; /* because of inverted traversal direction */
				if (!c) continue;
				if (*c == '\376') c++;

				/* End at the initial line of the multiline message */
				if (c[0] != ' ' && (c[0] != '\377' || c[2] != ' ')) end = TRUE;

				if (!end) {
					if (*c == '\377') c += 2;
					while (*c == ' ') c++;
					/* Inserting this space could break an URL, so we'll check for it later again and possibly remove it */
					strcpy(xmsg_reverse, " ");
				} else xmsg_reverse[0] = 0;

				strcat(xmsg_reverse, c);
				/* avoid duplicate ' ' */
				if (xmsg_reverse[strlen(xmsg_reverse) - 1] == ' ') xmsg_reverse[strlen(xmsg_reverse) - 1] = 0;
#if 0
				/* remove the first space too if the previous line didn't end on an alphanum char, to avoid breaking URLs.
				   Note that this can still break URLs because words might get broken down inbetween instead of at a hyphen (would need fixing in util.c) */
				if (xmsg[0] == ' ' && !isalphanum(xmsg_reverse[strlen(xmsg_reverse) - 1])) strcat(xmsg_reverse, xmsg + 1);
#else
				/* never insert spaces? (we do, but we delete it here again) - URLs are perfectly safe this way, but words might get concatinated wrongly */
				if (xmsg[0] == ' ') strcat(xmsg_reverse, xmsg + 1);
#endif
				else strcat(xmsg_reverse, xmsg);
				strcpy(xmsg, xmsg_reverse);

				if (end) break;
			}
		} else { /* message_recall */
			for (j = 0 + 1; (j < 20 + HGT_PLUS) && (i + j + s < n); j++) {
				c = message_recall[i + j + s];
				if (!c) continue;
				if (*c == '\376') c++;

				/* End at the initial line of the multiline message */
				if (c[0] != ' ' && (c[0] != '\377' || c[2] != ' ')) end = TRUE;

				if (!end) {
					if (*c == '\377') c += 2;
					while (*c == ' ') c++;
					strcpy(xmsg_reverse, " ");
				} else xmsg_reverse[0] = 0;

				strcat(xmsg_reverse, c);
				/* avoid duplicate ' ' */
				if (xmsg_reverse[strlen(xmsg_reverse) - 1] == ' ') xmsg_reverse[strlen(xmsg_reverse) - 1] = 0;
#if 0
				/* remove the first space too if the previous line didn't end on an alphanum char, to avoid breaking URLs.
				   Note that this can still break URLs because words might get broken down inbetween instead of at a hyphen (would need fixing in util.c) */
				if (xmsg[0] == ' ' && !isalphanum(xmsg_reverse[strlen(xmsg_reverse) - 1])) strcat(xmsg_reverse, xmsg + 1);
#else
				/* never insert spaces? (we do, but we delete it here again) - URLs are perfectly safe this way, but words might get concatinated wrongly */
				if (xmsg[0] == ' ') strcat(xmsg_reverse, xmsg + 1);
#endif
				else strcat(xmsg_reverse, xmsg);
				strcpy(xmsg, xmsg_reverse);

				if (end) break;
			}
		}
		(void)copy_to_clipboard(xmsg);
	} else (void)copy_to_clipboard((char*)msg_raw);
}
#endif

/*
 * Show previous messages to the user   -BEN-
 *
 * The screen format uses line 0 and 23 for headers and prompts,
 * skips line 1 and 22, and uses line 2 thru 21 for old messages.
 *
 * This command shows you which commands you are viewing, and allows
 * you to "search" for strings in the recall.
 *
 * Note that messages may be longer than 80 characters, but they are
 * displayed using "infinite" length, with a special sub-command to
 * "slide" the virtual display to the left or right.
 *
 * Attempt to only hilite the matching portions of the string.
 */
/* TODO: those functions should be bandled in one or two generic function(s).
 */
void do_cmd_messages(void) {
	int i, j, k, n, nn, q, r, s, t = 0, p;
	byte a, ab, ap;

	char shower[80] = "";
	char finder[80] = "";

	cptr message_recall[MESSAGE_MAX] = {0};
	cptr msg = "", msg2, msg_raw = NULL, msg3;

	/* Display messages in different colors -Zz */

	cptr nomsg_target = "Target selected.";
	cptr nomsg_map = "Map sector ";

	/* Total messages */
	n = message_num();
	nn = 0;  /* number of new messages */

	/* Filter message buffer for "unimportant messages" add to message_recall
	 * "Target selected" messages are too much clutter for archers to remove
	 * from msg recall
	 */

	j = 0;
	msg = NULL;
	for (i = 0; i < n; i++) {
		msg = message_str(i);

		if (strstr(msg, nomsg_target) ||
		    strstr(msg, nomsg_map))
			continue;

		/* strip scrollback control code before processing the message */
		if (msg[0] == '\376') msg++;

		message_recall[nn] = msg;
		nn++;
	}

	/* Start on first message */
	i = 0;

	/* Start at leftmost edge */
	q = 0;

	/* Save the screen */
	Term_save();

	/* Process requests until done */
	while (1) {
		/* Clear screen */
		Term_clear();

		n = nn;  /* new total # of messages in our new array */
		r = 0;	/* how many times the message is Repeated */
		s = 0;	/* how many lines Saved */
		k = 0;	/* end of buffer flag, but used arithmetically; secondary use as key buffer */
		msg = NULL;

		/* Dump up to 20 lines of messages */
		for (j = 0; (j < 20 + HGT_PLUS) && (i + j + s < n); j++) {
			a = ab = ap = TERM_WHITE;

			msg2 = msg;
			msg = message_recall[i + j + s];
			if (!j) msg_raw = msg; //remember the bottom-most line

			/* Handle repeated messages
			   (Minor glitch note: if the first msgs after joining the server are repeated 'searching' msgs, the first one will not combine) */
			if (msg == msg2) {
				r++;
				j--;
				s++;
				if (i + j + s < n - 1) continue;
				/* We reached the final message in our scrollback buffer and it was a duplicate */
				k = 1;
			}

			if (r) {
				Term_putstr(t < 72 ? t : 72, 21 + HGT_PLUS - j + 1 - k, -1, a, format(" (x%d)", r + 1));
				r = 0;
			}

			/* Final message reached and already added to duplicates, we don't have anything more to display */
			if (k) break;

			/* Apply horizontal scroll */
			msg3 = ((int) strlen(msg) >= q) ? (msg + q) : "";

			/* For horizontal scrolling: Parse correct colour code that we might have skipped */
			if (q) {
				for (p = 0; p < q; p++) {
					if (msg[p] != '\377') continue;
					if (msg[p + 1] == '.') a = ap;
					else if (isalpha(msg[p + 1]) || isdigit(msg[p + 1])) {
						ap = ab;
						a = ab = color_char_to_attr(msg[p + 1]);
					}
				}
			}

			/* Handle "shower" */
			if (shower[0] && my_strcasestr(msg3, shower)) {
				char tmp[MSG_LEN], *tc = tmp;
				const char *tm = msg3;

				/* Lines can contain a lot of colour codes that would override this one, so we have to strip them first */
				while (*tm) {
					if (*tm == '\377') {
						tm++;
						if (*tm) tm++;
						continue;
					}
					*tc = *tm;
					tc++;
					tm++;
				}
				*tc = 0;
				Term_putstr(0, 21 + HGT_PLUS - j, -1, TERM_SELECTOR, tmp);
			} else { /* Normal line processing (ie no shower set) */
				/* Dump the messages, bottom to top */
				Term_putstr(0, 21 + HGT_PLUS - j, -1, a, (char*)msg3);
			}

			/* For bundling of subsequent identical lines:
			   Remember length of the actual line that has possibly also been vertically-scrolled */
			t = strlen(msg3);
		}

		/* Display header XXX XXX XXX */
		prt(format("Message Recall (%d-%d of %d), Offset %d",
		    i, i + j - 1, n, q), 0, 0);

		/* Display prompt (not very informative) */
		prt("[p/n/g/G/# navi, f filedump, CTRL+K copy last line, / search, = mark, ESC exit]", 23 + HGT_PLUS, 0);

		/* Get a command */
		k = inkey();

		/* Exit on Escape */
		if (k == ESCAPE || k == KTRL('Q')) break;

		/* Hack -- Save the old index */
		j = i;

		/* Hack -- go to a specific line */
		if (k == '#') {
			/* suppress hybrid macros */
			bool inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			char tmp[80];
			prt(format("Goto Line(max %d): ", n), 23 + HGT_PLUS, 0);
			strcpy(tmp, "0");
			if (askfor_aux(tmp, 79, 0)) {
				i = atoi(tmp);
				i = i > 0 ? (i < n ? i : n - 1) : 0;
			}
			inkey_msg = inkey_msg_old;
		}

		/* Horizontal scroll */
		if (k == '4' || k == '<' || k == 'h') {
			/* Scroll left */
			q = (q >= 40) ? (q - 40) : 0;

			/* Success */
			continue;
		}

		/* Horizontal scroll */
		if (k == '6' || k == '>' || k == 'l') {
			/* Scroll right */
			q = q + 40;
			if (q >= ONAME_LEN) { /* Prevent buffer overflow. We just pick an arbitrary value here, randomly picked from Receive_special_line():buf[]. */
				q -= 40;
				if (q < 0) q = 0; /* Edge case, paranoia. */
			}

			/* Success */
			continue;
		}

		/* Hack -- handle show */
		if (k == '=') {
			/* suppress hybrid macros */
			bool inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			/* Prompt */
			prt("Mark: ", 23 + HGT_PLUS, 0);

			/* Get a "shower" string, or continue */
			if (!askfor_aux(shower, 79, 0)) {
				inkey_msg = inkey_msg_old;
				continue;
			}
			inkey_msg = inkey_msg_old;

			/* Okay */
			continue;
		}

		/* Hack -- handle find */
		/* FIXME -- (x4) compressing seems to ruin it */
		if (k == '/') {
			int z;
			/* suppress hybrid macros */
			bool inkey_msg_old = inkey_msg;

			inkey_msg = TRUE;

			/* Prompt */
			prt("Find: ", 23 + HGT_PLUS, 0);

			/* Get a "finder" string, or continue */
			if (!askfor_aux(finder, 79, 0)) {
				inkey_msg = inkey_msg_old;
				continue;
			}
			inkey_msg = inkey_msg_old;

			/* Scan messages */
			for (z = i + 1; z < n; z++) {
				cptr str = message_recall[z];//message_str(z);

				/* Handle "shower" */
				if (my_strcasestr(str, finder)) {
					/* New location */
					i = z;

					/* Hack -- also show */
					//strcpy(shower, str);  --not the whole line, just the search string, or we only highlight exact duplicate lines somewhere
					strcpy(shower, finder); //make the search string also the highlight show-string

					/* Done */
					break;
				}
			}
		}

		/* Recall 1 older message */
		if ((k == '8') || (k == '\b') || k == 'k') {
			/* Go newer if legal */
			if (i + 1 < n) i += 1;
		}

		/* Recall 10 older messages */
		if (k == '+') {
			/* Go older if legal */
			if (i + 10 < n) i += 10;
		}

		/* Recall 20 older messages */
		if ((k == 'p') || (k == KTRL('P')) || (k == 'b') || k == KTRL('U')) {
			/* Go older if legal */
			if (i + 20 + HGT_PLUS < n) i += 20 + HGT_PLUS;
		}

		/* Recall 20 newer messages */
		if ((k == 'n') || (k == KTRL('N')) || k == ' ') {
			/* Go newer (if able) */
			i = (i >= 20 + HGT_PLUS) ? (i - (20 + HGT_PLUS)) : 0;
		}

		/* Recall 10 newer messages */
		if (k == '-') {
			/* Go newer (if able) */
			i = (i >= 10) ? (i - 10) : 0;
		}

		/* Recall 1 newer messages */
		if (k == '2' || k == 'j' || (k == '\n') || (k == '\r')) {
			/* Go newer (if able) */
			i = (i >= 1) ? (i - 1) : 0;
		}

		/* Recall the oldest messages */
		if (k == 'g') {
			/* Go oldest */
			i = n - (20 + HGT_PLUS);
			if (i < 0) i = 0;
		}

		/* Recall the newest messages */
		if (k == 'G') {
			/* Go newest */
			i = 0;
		}

		/* Dump */
		if ((k == 'f') || (k == 'F')) {
			char tmp[80];
			strnfmt(tmp, 79, "%s-msg.txt", cname);
			if (get_string("Filename: ", tmp, 79)) {
				if (tmp[0] && (tmp[0] != ' ')) {
					dump_messages(tmp, MESSAGE_MAX, 0);
					continue;
				}
			}
		}

		/* Copy to clipboard o_o */
		if (k == KTRL('K') && msg != NULL) {
#ifdef COPY_MULTILINE
			copy_to_clipboard_multiline(msg_raw, message_recall, i, s, n, -1);
#else
			(void)copy_to_clipboard((char*)msg_raw);
#endif
			continue;
		}

		if (k == KTRL('T')) {
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", FALSE);
			continue;
		}

		/* helpful for replying bit by bit to stuff in
		   chat that happened while you were afk.. */
		if (k == ':') {
			cmd_message();
			continue;
		}

		/* Hack -- Error of some kind */
		if (i == j) bell();
	}

	/* Restore the screen */
	Term_load();

	/* Flush any queued events */
	Flush_queue();
}


/* Show buffer of "important events" only via the CTRL-O command -Zz */
void do_cmd_messages_important(void) {
	int i, j, k, n, nn, q, p;
	byte a, ab, ap;

	char shower[80] = "";
	char finder[80] = "";

	cptr msg = "", msg_raw = NULL, msg3;

	/* Create array to store message buffer for important messags  */
	/* (This is an expensive hit, move to c-init.c?  But this only */
	/* occurs when user hits CTRL-O which is usally in safe place  */
	/* or after AFK) 					       */
	cptr message_important[MESSAGE_MAX] = {0};

	/* Display messages in different colors */

	/* we use a dedicated important-scrollback-buffer to allow longer scrollbacks after message buffer got flooded with lots of combat messages */
	/* Total messages */
	n = message_num_impscroll();
	nn = 0;  /* number of new messages */

	/* Filter message buffer for "important messages" add to message_important*/
	//for (i = 0; i < n; i++)
	for (i = n - 1; i >= 0; i--) { /* traverse from oldest to newest message */
		message_important[nn] = message_str_impscroll(i);
		nn++;
	}

	/* Start on first message */
	i = 0;

	/* Start at leftmost edge */
	q = 0;


	/* Save the screen */
	Term_save();

	/* Process requests until done */
	while (1) {
		/* Clear screen */
		Term_clear();

		/* Use last element in message_important as  message_num() */
		n = nn;

		/* Dump up to 20 lines of messages */
		for (j = 0; (j < 20 + HGT_PLUS) && (i + j < n); j++) {
			a = ab = ap = TERM_WHITE;

			msg = message_important[nn - 1 - (i + j)]; /* because of inverted traversal direction, see further above */
			if (!j) msg_raw = msg; //remember the bottom-most line

			/* Note: msg2 is not used in this function as we don't do bundling of identical subsequent lines here unlike in do_cmd_messages(). */

			/* Apply horizontal scroll */
			msg3 = ((int) strlen(msg) >= q) ? (msg + q) : "";

			/* For horizontal scrolling: Parse correct colour code that we might have skipped */
			if (q) {
				for (p = 0; p < q; p++) {
					if (msg[p] != '\377') continue;
					if (msg[p + 1] == '.') a = ap;
					else if (isalpha(msg[p + 1]) || isdigit(msg[p + 1])) {
						ap = ab;
						a = ab = color_char_to_attr(msg[p + 1]);
					}
				}
			}

			/* Handle "shower" */
			if (shower[0] && my_strcasestr(msg3, shower)) {
				char tmp[MSG_LEN], *tc = tmp;
				const char *tm = msg3;

				/* Lines can contain a lot of colour codes that would override this one, so we have to strip them first */
				while (*tm) {
					if (*tm == '\377') {
						tm++;
						if (*tm) tm++;
						continue;
					}
					*tc = *tm;
					tc++;
					tm++;
				}
				*tc = 0;
				Term_putstr(0, 21 + HGT_PLUS - j, -1, TERM_SELECTOR, tmp);
			} else { /* Normal line processing (ie no shower set) */
				/* Dump the messages, bottom to top */
				Term_putstr(0, 21 + HGT_PLUS - j, -1, a, (char*)msg3);
			}
		}

		/* Display header XXX XXX XXX */
		prt(format("Important-Message Recall (%d-%d of %d), Offset %d",
		    i, i + j - 1, n, q), 0, 0);

		/* Display prompt (not very informative) */
		prt("[p/n/g/G/# navi, f filedump, CTRL+K copy last line, / search, = mark, ESC exit]", 23 + HGT_PLUS, 0);

		/* Get a command */
		k = inkey();

		/* Exit on Escape */
		if (k == ESCAPE || k == KTRL('Q')) break;

		/* Hack -- Save the old index */
		j = i;

		/* Hack -- go to a specific line */
		if (k == '#') {
			/* suppress hybrid macros */
			bool inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			char tmp[80];
			prt(format("Goto Line(max %d): ", n), 23 + HGT_PLUS, 0);
			strcpy(tmp, "0");
			if (askfor_aux(tmp, 79, 0)) {
				i = atoi(tmp);
				i = i > 0 ? (i < n ? i : n - 1) : 0;
			}
			inkey_msg = inkey_msg_old;
		}

		/* Horizontal scroll */
		if (k == '4' || k == '<' || k == 'h') {
			/* Scroll left */
			q = (q >= 40) ? (q - 40) : 0;

			/* Success */
			continue;
		}

		/* Horizontal scroll */
		if (k == '6' || k == '>' || k == 'l') {
			/* Scroll right */
			q = q + 40;
			if (q >= ONAME_LEN) { /* Prevent buffer overflow. We just pick an arbitrary value here, randomly picked from Receive_special_line():buf[]. */
				q -= 40;
				if (q < 0) q = 0; /* Edge case, paranoia. */
			}

			/* Success */
			continue;
		}

		/* Hack -- handle show */
		if (k == '=') {
			/* suppress hybrid macros */
			bool inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;

			/* Prompt */
			prt("Mark: ", 23 + HGT_PLUS, 0);

			/* Get a "shower" string, or continue */
			if (!askfor_aux(shower, 79, 0)) {
				inkey_msg = inkey_msg_old;
				continue;
			}
			inkey_msg = inkey_msg_old;

			/* Okay */
			continue;
		}

		/* Hack -- handle find */
		if (k == '/') {
			int z;
			/* suppress hybrid macros */
			bool inkey_msg_old = inkey_msg;

			inkey_msg = TRUE;

			/* Prompt */
			prt("Find: ", 23 + HGT_PLUS, 0);

			/* Get a "finder" string, or continue */
			if (!askfor_aux(finder, 79, 0)) {
				inkey_msg = inkey_msg_old;
				continue;
			}
			inkey_msg = inkey_msg_old;

			/* Scan messages */
			for (z = i + 1; z < n; z++) {
				cptr str = message_important[z];//message_str_impscroll(z);

				/* Handle "shower" */
				if (my_strcasestr(str, finder)) {
					/* New location */
					i = z;

					/* Hack -- also show */
					//strcpy(shower, str);  --not the whole line, just the search string, or we only highlight exact duplicate lines somewhere
					strcpy(shower, finder); //make the search string also the highlight show-string

					/* Done */
					break;
				}
			}
		}

		/* Recall 1 older message */
		if ((k == '8') || (k == '\b') || k == 'k') {
			/* Go newer if legal */
			if (i + 1 < n) i += 1;
		}

		/* Recall 10 older messages */
		if (k == '+') {
			/* Go older if legal */
			if (i + 10 < n) i += 10;
		}

		/* Recall 20 older messages */
		if ((k == 'p') || (k == KTRL('P')) || (k == 'b') || (k == KTRL('O')) ||
		    k == KTRL('U')) {
			/* Go older if legal */
			if (i + 20 + HGT_PLUS < n) i += 20 + HGT_PLUS;
		}

		/* Recall 20 newer messages */
		if ((k == 'n') || (k == KTRL('N')) || k == ' ') {
			/* Go newer (if able) */
			i = (i >= 20 + HGT_PLUS) ? (i - (20 + HGT_PLUS)) : 0;
		}

		/* Recall 10 newer messages */
		if (k == '-') {
			/* Go newer (if able) */
			i = (i >= 10) ? (i - 10) : 0;
		}

		/* Recall 1 newer messages */
		if (k == '2' || k == 'j' || (k == '\n') || (k == '\r')) {
			/* Go newer (if able) */
			i = (i >= 1) ? (i - 1) : 0;
		}

		/* Recall the oldest messages */
		if (k == 'g') {
			/* Go oldest */
			i = n - (20 + HGT_PLUS);
			if (i < 0) i = 0;
		}

		/* Recall the newest messages */
		if (k == 'G') {
			/* Go newest */
			i = 0;
		}

		/* Dump */
		if ((k == 'f') || (k == 'F')) {
			char tmp[80];
			strnfmt(tmp, 79, "%s-important.txt", cname);
			if (get_string("Filename: ", tmp, 79)) {
				if (tmp[0] && (tmp[0] != ' ')) {
					dump_messages(tmp, MESSAGE_MAX, 1);
					continue;
				}
			}
		}

		/* Copy to clipboard o_o */
		if (k == KTRL('K') && msg_raw != NULL) {
#ifdef COPY_MULTILINE
			copy_to_clipboard_multiline(msg_raw, message_important, i, -1, n, nn);
#else
			(void)copy_to_clipboard((char*)msg_raw);
#endif
			continue;
		}

		if (k == KTRL('T')) {
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", FALSE);
			continue;
		}

		/* helpful for replying bit by bit to stuff in
		   chat that happened while you were afk.. */
		if (k == ':') {
			cmd_message();
			continue;
		}

		/* Hack -- Error of some kind */
		if (i == j) bell();
	}

	/* Restore the screen */
	Term_load();

	/* Flush any queued events */
	Flush_queue();
}

/*
 * dump message history to a file.	- Jir -
 *
 * XXX The beginning of dump can be corrupted. FIXME
 */
/* FIXME: result can be garbled if contains '%' */
/* important if mode != 0 */
void dump_messages_aux(FILE *fff, int lines, int mode, bool ignore_color) {
	int i, j, k, n, nn, q, r, s, t = 0;

	cptr message_recall[MESSAGE_MAX] = {0};
	cptr msg = "", msg2;

	char buf[MSG_LEN];

	cptr nomsg_target = "Target selected.";
	cptr nomsg_map = "Map sector ";

	/* Total messages */
	n = message_num();
	nn = 0;  /* number of new messages */

	/* Filter message buffer for "unimportant messages" add to message_recall
	 * "Target selected" messages are too much clutter for archers to remove
	 * from msg recall
	 */

	j = 0;
	msg = NULL;
	for (i = 0; i < n; i++) {
		msg = message_str(i);

		if (strstr(msg, nomsg_target) || strstr(msg, nomsg_map))
			continue;

		if (mode) { /* important, ie 'important messages' only? */
			if (msg[0] == '\376') {
				/* strip control code */
				msg++;

				message_recall[nn] = msg;
				nn++;
			} else continue;
		} else {
			/* strip control code */
			if (msg[0] == '\376') msg++;

			message_recall[nn] = msg;
			nn++;
		}
	}


	/* Start on first message */
	i = nn - lines;
	if (i < 0) i = 0;

	/* Start at leftmost edge */
	q = 0;

	n = nn;  /* new total # of messages in our new array */
	r = 0;	/* how many times the message is Repeated */
	s = 0;	/* how many lines Saved */
	k = 0;	/* end of buffer flag */


	/* Dump up to 20 lines of messages */
	for (j = 0; j + s < MIN(n, lines); j++) {
		msg2 = msg;
		msg = message_recall[MIN(n, lines) - (j+s) - 1];

		/* Handle repeated messages */
		if (msg == msg2) {
			r++;
			j--;
			s++;
			if (j + s < MIN(n, lines) - 1) continue;
			k = 1;
		}

		if (r) {
			fprintf(fff, " (x%d)", r + 1);
			r = 0;
		}
		fprintf(fff, "\n");
		if (k) break;

		q = 0;
		for (t = 0; t < (int)strlen(msg); t++) {
			if (msg[t] == '\377') {
				if (!ignore_color)
					buf[q++] = '{';
				else {
					t++;
					if (msg[t] == '\0')
						break;
				}
				continue;
			}
			if (msg[t] == '\n') {
				buf[q++] = '\n';
				continue;
			}
			if (msg[t] == '\r') {
				buf[q++] = '\n';
				continue;
			}
			buf[q++] = msg[t];
		}
		buf[q] = '\0';

		/* Dump the messages, bottom to top */
		fputs(buf, fff);
	}
	fprintf(fff, "\n\n");
}

errr dump_messages(cptr name, int lines, int mode) {
	int			fd = -1;
	FILE		*fff = NULL;
	char		buf[1024];

	cptr what = mode ? "Chat" : "Message";

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, name);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the non-existing file */
	if (fd < 0) fff = my_fopen(buf, "w");


	/* Invalid file */
	if (!fff) {
		/* Message */
		c_msg_print(format("%s dump failed!", what));
		clear_topline_forced();

		/* Error */
		return(-1);
	}

	/* Begin dump */
	fprintf(fff, "  [TomeNET %d.%d.%d%s @ %s %s Dump]\n\n",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, CLIENT_VERSION_TAG, server_name, what);

	/* Do it */
	dump_messages_aux(fff, lines, mode, FALSE);//FALSE

	fprintf(fff, "\n\n");

	/* Close it */
	my_fclose(fff);


	/* Message */
	c_msg_print(format("%s dump successful.", what));
	clear_topline_forced();

	/* Success */
	return(0);
}
