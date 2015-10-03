/* $Id$ */
/* More extra client things */

#include "angband.h"

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
	int i, j, k, n, nn, q, r, s, t = 0;

	char shower[80] = "";
	char finder[80] = "";

	cptr message_recall[MESSAGE_MAX] = {0};
	cptr msg = "", msg2;

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
		k = 0;	/* end of buffer flag */


		/* Dump up to 20 lines of messages */
		for (j = 0; (j < 20 + HGT_PLUS) && (i + j + s < n); j++) {
			byte a = TERM_WHITE;

			msg2 = msg;
			msg = message_recall[i+j+s];

			/* Handle repeated messages */
			if (msg == msg2) {
				r++;
				j--;
				s++;
				if (i + j + s < n - 1) continue;
				k = 1;
			}

			if (r) {
				Term_putstr(t < 72 ? t : 72, 21 + HGT_PLUS-j+1-k, -1, a, format(" (x%d)", r + 1));
				r = 0;
			}

			/* Apply horizontal scroll */
			msg = ((int) strlen(msg) >= q) ? (msg + q) : "";

			/* Handle "shower" */
			if (shower[0] && strstr(msg, shower)) a = TERM_YELLOW;

			/* Dump the messages, bottom to top */
			Term_putstr(0, 21 + HGT_PLUS-j, -1, a, (char*)msg);
			t = strlen(msg);
		}

		/* Display header XXX XXX XXX */
		prt(format("Message Recall (%d-%d of %d), Offset %d",
					i, i+j-1, n, q), 0, 0);

		/* Display prompt (not very informative) */
		prt("[Press 'p' for older, 'n' for newer, 'f' for filedump, ..., or ESCAPE]", 23 + HGT_PLUS, 0);

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
			if (askfor_aux(tmp, 80, 0)) {
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

			/* Success */
			continue;
		}

		/* Hack -- handle show */
		if (k == '=') {
			/* suppress hybrid macros */
			bool inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			/* Prompt */
			prt("Show: ", 23 + HGT_PLUS, 0);

			/* Get a "shower" string, or continue */
			if (!askfor_aux(shower, 80, 0)) {
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
			/* suppress hybrid macros */
			bool inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			int z;

			/* Prompt */
			prt("Find: ", 23 + HGT_PLUS, 0);

			/* Get a "finder" string, or continue */
			if (!askfor_aux(finder, 80, 0)) {
				inkey_msg = inkey_msg_old;
				continue;
			}
			inkey_msg = inkey_msg_old;

			/* Scan messages */
			for (z = i + 1; z < n; z++) {
				cptr str = message_str(z);

				/* Handle "shower" */
				if (strstr(str, finder)) {
					/* New location */
					i = z;

					/* Hack -- also show */
					strcpy(shower, str);

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

		if (k == KTRL('T')) {
			/* Take a screenshot */
			xhtml_screenshot("screenshot????");
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


/* Show buffer of "important events" only via the CTRL-O command -Zz
   Note that nowadays the name 'chatonly' is slightly misleading:
   It simply is a buffer of 'very important' messages, as already
   stated above by Zz ;) - C. Blue */
void do_cmd_messages_chatonly(void) {
	int i, j, k, n, nn, q;

	char shower[80] = "";
	char finder[80] = "";

	/* Create array to store message buffer for important messags  */
	/* (This is an expensive hit, move to c-init.c?  But this only */
	/* occurs when user hits CTRL-O which is usally in safe place  */
	/* or after AFK) 					       */
	cptr message_chat[MESSAGE_MAX] = {0};

	/* Display messages in different colors */

#if 0 /* before 4.6.0: we filter all eligible messages from the generic 'message' buffer to display here */
	/* Total messages */
	n = message_num();
	nn = 0;  /* number of new messages */

	/* Filter message buffer for "important messages" add to message_chat*/
//	for (i = 0; i < n; i++)
	for (i = n - 1; i >= 0; i--) { /* traverse from oldest to newest message */
		cptr msg = message_str(i);

		if (msg[0] == '\376') {
			/* strip control code */
			if (msg[0] == '\376') msg++;
			message_chat[nn] = msg;
			nn++;
		}
	}
#else /* we use a dedicated important-scrollback-buffer to allow longer scrollbacks after message buffer got flooded with lots of combat messages */
	/* Total messages */
	n = message_num_impscroll();
	nn = 0;  /* number of new messages */

	/* Filter message buffer for "important messages" add to message_chat*/
//	for (i = 0; i < n; i++)
	for (i = n - 1; i >= 0; i--) { /* traverse from oldest to newest message */
		message_chat[nn] = message_str_impscroll(i);
		nn++;
	}
#endif

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

		/* Use last element in message_chat as  message_num() */
		n = nn;
		/* Dump up to 20 lines of messages */
		for (j = 0; (j < 20 + HGT_PLUS) && (i + j < n); j++) {
			byte a = TERM_WHITE;
			cptr msg = message_chat[nn - 1 - (i+j)]; /* because of inverted traversal direction, see further above */
//			cptr msg = message_chat[i+j];

			/* Apply horizontal scroll */
			msg = ((int) strlen(msg) >= q) ? (msg + q) : "";

			/* Handle "shower" */
			if (shower[0] && strstr(msg, shower)) a = TERM_YELLOW;

			/* Dump the messages, bottom to top */
			Term_putstr(0, 21 + HGT_PLUS-j, -1, a, (char*)msg);
		}

		/* Display header XXX XXX XXX */
		prt(format("Message Recall (%d-%d of %d), Offset %d",
					i, i+j-1, n, q), 0, 0);

		/* Display prompt (not very informative) */
		prt("[Press 'p' for older, 'n' for newer, 'f' for filedump, ..., or ESCAPE]", 23 + HGT_PLUS, 0);

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
			if (askfor_aux(tmp, 80, 0)) {
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

			/* Success */
			continue;
		}

		/* Hack -- handle show */
		if (k == '=') {
			/* suppress hybrid macros */
			bool inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;

			/* Prompt */
			prt("Show: ", 23 + HGT_PLUS, 0);

			/* Get a "shower" string, or continue */
			if (!askfor_aux(shower, 80, 0)) {
				inkey_msg = inkey_msg_old;
				continue;
			}
			inkey_msg = inkey_msg_old;

			/* Okay */
			continue;
		}

		/* Hack -- handle find */
		if (k == '/') {
			/* suppress hybrid macros */
			bool inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;

			int z;

			/* Prompt */
			prt("Find: ", 23 + HGT_PLUS, 0);

			/* Get a "finder" string, or continue */
			if (!askfor_aux(finder, 80, 0)) {
				inkey_msg = inkey_msg_old;
				continue;
			}
			inkey_msg = inkey_msg_old;

			/* Scan messages */
			for (z = i + 1; z < n; z++) {
#if 0
				cptr str = message_str(z);
#else
				cptr str = message_str_impscroll(z);
#endif

				/* Handle "shower" */
				if (strstr(str, finder)) {
					/* New location */
					i = z;

					/* Hack -- also show */
					strcpy(shower, str);

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
			strnfmt(tmp, 79, "%s-chat.txt", cname);
			if (get_string("Filename: ", tmp, 79)) {
				if (tmp[0] && (tmp[0] != ' ')) {
					dump_messages(tmp, MESSAGE_MAX, 1);
					continue;
				}
			}
		}

		if (k == KTRL('T')) {
			/* Take a screenshot */
			xhtml_screenshot("screenshot????");
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
/* chatonly if mode != 0 */
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

		if (mode) { /* chatonly, ie 'important messages' only? */
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
		return (-1);
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
	return (0);
}
