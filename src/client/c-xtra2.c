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
void do_cmd_messages(void)
{
	int i, j, k, n, nn, q, r, s, t;

	char shower[80] = "";
	char finder[80] = "";

	cptr message_recall[MESSAGE_MAX] = {0};
	cptr msg, msg2;

	/* Display messages in different colors -Zz */
	char nameA[20];
	char nameB[20];
	cptr nomsg_target = "Target Selected.";

	strcpy(nameA, "[");  strcat(nameA, nick);  strcat(nameA, ":");
	strcpy(nameB, ":");  strcat(nameB, nick);  strcat(nameB, "]");


	/* Total messages */
	n = message_num();
	nn = 0;  // number of new messages

	/* Filter message buffer for "unimportant messages" add to message_recall
	 * "Target Selected" messages are too much clutter for archers to remove 
	 * from msg recall
	 */

	j = 0;
	msg = NULL;
	for (i = 0; i < n; i++)
	{
		msg = message_str(i);

		if (strstr(msg, nomsg_target) != NULL) 
			continue;	

		message_recall[nn] = msg;	
		nn++;
	}


	/* Start on first message */
	i = 0;

	/* Start at leftmost edge */
	q = 0;

#if 0 /* evil test */
	/* Enter "icky" mode */
	screen_icky = topline_icky = TRUE;
#endif

	/* Save the screen */
	Term_save();

	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		n = nn;  // new total # of messages in our new array
		r = 0;	// how many times the message is Repeated
		s = 0;	// how many lines Saved
		k = 0;	// end of buffer flag


		/* Dump up to 20 lines of messages */
		for (j = 0; (j < 20) && (i + j + s < n); j++)
		{
			byte a = TERM_WHITE;

			msg2 = msg;
			msg = message_recall[i+j+s];

			/* Handle repeated messages */
			if (msg == msg2)
			{
				r++;
				j--;
				s++;
				if (i + j + s < n - 1) continue;
				k = 1;
			}

			if (r)
			{
//				msg = format("    (the message below repeated %d times)", r + 1);
//				Term_putstr(72, 21-j+1-k, -1, a, format(" (x%d)", r + 1));
				Term_putstr(t < 72 ? t : 72, 21-j+1-k, -1, a, format(" (x%d)", r + 1));
				r = 0;
//				continue;
//				s--;
			}

			/* Apply horizontal scroll */
			msg = (strlen(msg) >= q) ? (msg + q) : "";

			/* Handle "shower" */
			if (shower[0] && strstr(msg, shower)) a = TERM_YELLOW;

#if 0
			/* Handle message from other player */
			/* Display messages in different colors -Zz */
			if ((strstr(msg, nameA) != NULL) || (strstr(msg, nameB) != NULL))
				a = TERM_GREEN;
			else if (msg[0] == '[')
				a = TERM_L_BLUE;
			else 
				a = TERM_WHITE;
#endif

			/* Dump the messages, bottom to top */
			Term_putstr(0, 21-j, -1, a, msg);
			t = strlen(msg);
		}

		/* Display header XXX XXX XXX */
		prt(format("Message Recall (%d-%d of %d), Offset %d",
					i, i+j-1, n, q), 0, 0);

		/* Display prompt (not very informative) */
		prt("[Press 'p' for older, 'n' for newer, ..., or ESCAPE]", 23, 0);

		/* Get a command */
		k = inkey();

		/* Exit on Escape */
		if (k == ESCAPE) break;

		/* Hack -- Save the old index */
		j = i;

		/* Horizontal scroll */
		if (k == '4')
		{
			/* Scroll left */
			q = (q >= 40) ? (q - 40) : 0;

			/* Success */
			continue;
		}

		/* Horizontal scroll */
		if (k == '6')
		{
			/* Scroll right */
			q = q + 40;

			/* Success */
			continue;
		}

		/* Hack -- handle show */
		if (k == '=')
		{
			/* Prompt */
			prt("Show: ", 23, 0);

			/* Get a "shower" string, or continue */
			if (!askfor_aux(shower, 80, 0)) continue;

			/* Okay */
			continue;
		}

		/* Hack -- handle find */
		if (k == '/')
		{
			int z;

			/* Prompt */
			prt("Find: ", 23, 0);

			/* Get a "finder" string, or continue */
			if (!askfor_aux(finder, 80, 0)) continue;

			/* Scan messages */
			for (z = i + 1; z < n; z++)
			{
				cptr str = message_str(z);

				/* Handle "shower" */
				if (strstr(str, finder))
				{
					/* New location */
					i = z;

					/* Done */
					break;
				}
			}
		}

		/* Recall 1 older message */
		if ((k == '8') || (k == '\n') || (k == '\r'))
		{
			/* Go newer if legal */
			if (i + 1 < n) i += 1;
		}

		/* Recall 10 older messages */
		if (k == '+')
		{
			/* Go older if legal */
			if (i + 10 < n) i += 10;
		}

		/* Recall 20 older messages */
		if ((k == 'p') || (k == KTRL('P')) || (k == ' '))
		{
			/* Go older if legal */
			if (i + 20 < n) i += 20;
		}

		/* Recall 20 newer messages */
		if ((k == 'n') || (k == KTRL('N')))
		{
			/* Go newer (if able) */
			i = (i >= 20) ? (i - 20) : 0;
		}

		/* Recall 10 newer messages */
		if (k == '-')
		{
			/* Go newer (if able) */
			i = (i >= 10) ? (i - 10) : 0;
		}

		/* Recall 1 newer messages */
		if (k == '2')
		{
			/* Go newer (if able) */
			i = (i >= 1) ? (i - 1) : 0;
		}

		/* Hack -- Error of some kind */
		if (i == j) bell();
	}

	/* Restore the screen */
	Term_load();

#if 0 /* evil test */
	/* Leave "icky" mode */
	screen_icky = topline_icky = FALSE;
#endif

	/* Flush any queued events */
	Flush_queue();
}


/* Show buffer of "important events" only via the CTRL-O command -Zz */
void do_cmd_messages_chatonly(void)
{
	int i, j, k, n, nn, q;

	char shower[80] = "";
	char finder[80] = "";

	/* Create array to store message buffer for important messags  */
	/* (This is an expensive hit, move to c-init.c?  But this only */
	/* occurs when user hits CTRL-O which is usally in safe place  */
	/* or after AFK) 					       */ 
	cptr message_chat[MESSAGE_MAX] = {0};

	/* Display messages in different colors */
	char nameA[20];
	char nameB[20];
	cptr msg_deadA = "You have been killed";
	cptr msg_deadB = "You die";
	cptr msg_unique = "was slain by";
	cptr msg_killed = "was killed by";
	cptr msg_destroyed = "ghost was destroyed by";
	cptr msg_suicide = "committed suicide.";
	cptr msg_telepath = "mind";


	strcpy(nameA, "[");  strcat(nameA, nick);  strcat(nameA, ":");
	strcpy(nameB, ":");  strcat(nameB, nick);  strcat(nameB, "]");


	/* Total messages */
	n = message_num();
	nn = 0;  // number of new messages

	/* Filter message buffer for "important messages" add to message_chat*/
	for (i = 0; i < n; i++)
	{
		cptr msg = message_str(i);

		if ((strstr(msg, nameA)      != NULL) || (strstr(msg, nameB)         != NULL) || (msg[0] == '[') || \
				(strstr(msg, msg_killed) != NULL) || (strstr(msg, msg_destroyed) != NULL) || \
				(strstr(msg, msg_unique) != NULL) || (strstr(msg, msg_suicide)   != NULL) || \
				(strstr(msg, msg_deadA)  != NULL) || (strstr(msg, msg_deadB)     != NULL) || \
				(strstr(msg, msg_telepath) != NULL) || (msg[2] == '['))
		{
			message_chat[nn] = msg;	
			nn++;
		}
	}

	/* Start on first message */
	i = 0;

	/* Start at leftmost edge */
	q = 0;


#if 0 /* evil test */
	/* Enter "icky" mode */
	screen_icky = topline_icky = TRUE;
#endif

	/* Save the screen */
	Term_save();

	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Use last element in message_chat as  message_num() */
		n = nn;
		/* Dump up to 20 lines of messages */
		for (j = 0; (j < 20) && (i + j < n); j++)
		{
			byte a = TERM_WHITE;
			cptr msg = message_chat[i+j];

			/* Apply horizontal scroll */
			msg = (strlen(msg) >= q) ? (msg + q) : "";

			/* Handle "shower" */
			if (shower[0] && strstr(msg, shower)) a = TERM_YELLOW;

#if 0
			/* Handle message from other player */
			/* Display messages in different colors -Zz */
			if ((strstr(msg, nameA) != NULL) || (strstr(msg, nameB) != NULL))
				a = TERM_GREEN;
			else if (msg[0] == '[')
				a = TERM_L_BLUE;
			else 
				a = TERM_WHITE;
#endif
			/* Dump the messages, bottom to top */
			Term_putstr(0, 21-j, -1, a, msg);
		}

		/* Display header XXX XXX XXX */
		prt(format("Message Recall (%d-%d of %d), Offset %d",
					i, i+j-1, n, q), 0, 0);

		/* Display prompt (not very informative) */
		prt("[Press 'p' for older, 'n' for newer, ..., or ESCAPE]", 23, 0);

		/* Get a command */
		k = inkey();

		/* Exit on Escape */
		if (k == ESCAPE) break;

		/* Hack -- Save the old index */
		j = i;

		/* Horizontal scroll */
		if (k == '4')
		{
			/* Scroll left */
			q = (q >= 40) ? (q - 40) : 0;

			/* Success */
			continue;
		}

		/* Horizontal scroll */
		if (k == '6')
		{
			/* Scroll right */
			q = q + 40;

			/* Success */
			continue;
		}

		/* Hack -- handle show */
		if (k == '=')
		{
			/* Prompt */
			prt("Show: ", 23, 0);

			/* Get a "shower" string, or continue */
			if (!askfor_aux(shower, 80, 0)) continue;

			/* Okay */
			continue;
		}

		/* Hack -- handle find */
		if (k == '/')
		{
			int z;

			/* Prompt */
			prt("Find: ", 23, 0);

			/* Get a "finder" string, or continue */
			if (!askfor_aux(finder, 80, 0)) continue;

			/* Scan messages */
			for (z = i + 1; z < n; z++)
			{
				cptr str = message_str(z);

				/* Handle "shower" */
				if (strstr(str, finder))
				{
					/* New location */
					i = z;

					/* Done */
					break;
				}
			}
		}

		/* Recall 1 older message */
		if ((k == '8') || (k == '\n') || (k == '\r'))
		{
			/* Go newer if legal */
			if (i + 1 < n) i += 1;
		}

		/* Recall 10 older messages */
		if (k == '+')
		{
			/* Go older if legal */
			if (i + 10 < n) i += 10;
		}

		/* Recall 20 older messages */
		if ((k == 'p') || (k == KTRL('P')) || (k == ' ') || (k == KTRL('O')))
		{
			/* Go older if legal */
			if (i + 20 < n) i += 20;
		}

		/* Recall 20 newer messages */
		if ((k == 'n') || (k == KTRL('N')))
		{
			/* Go newer (if able) */
			i = (i >= 20) ? (i - 20) : 0;
		}

		/* Recall 10 newer messages */
		if (k == '-')
		{
			/* Go newer (if able) */
			i = (i >= 10) ? (i - 10) : 0;
		}

		/* Recall 1 newer messages */
		if (k == '2')
		{
			/* Go newer (if able) */
			i = (i >= 1) ? (i - 1) : 0;
		}

		/* Hack -- Error of some kind */
		if (i == j) bell();
	}

	/* Restore the screen */
	Term_load();

#if 0 /* evil test */
	/* Leave "icky" mode */
	screen_icky = topline_icky = FALSE;
#endif

	/* Flush any queued events */
	Flush_queue();
}

