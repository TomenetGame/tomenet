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
        int i, j, k, n, q;

        char shower[80] = "";
        char finder[80] = "";


        /* Total messages */
        n = message_num();

        /* Start on first message */
        i = 0;

        /* Start at leftmost edge */
        q = 0;


        /* Enter "icky" mode */
        screen_icky = topline_icky = TRUE;

        /* Save the screen */
        Term_save();

        /* Process requests until done */
        while (1)
        {
                /* Clear screen */
                Term_clear();

                /* Dump up to 20 lines of messages */
                for (j = 0; (j < 20) && (i + j < n); j++)
                {
                        byte a = TERM_WHITE;

                        cptr str = message_str(i+j);

                        /* Apply horizontal scroll */
                        str = (strlen(str) >= q) ? (str + q) : "";

                        /* Handle "shower" */
                        if (shower[0] && strstr(str, shower)) a = TERM_YELLOW;

			/* Handle message from other player */
			if (str[0] == '[')
				a = TERM_L_BLUE;

                        /* Dump the messages, bottom to top */
                        Term_putstr(0, 21-j, -1, a, str);
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

        /* Leave "icky" mode */
        screen_icky = topline_icky = FALSE;

	/* Flush any queued events */
	Flush_queue();
}


