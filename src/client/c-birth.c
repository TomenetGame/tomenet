/* File: birth.c */

/* Purpose: create a player character */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define CLIENT

#include "angband.h"


/*
 * Choose the character's name
 */
void choose_name(void)
{
	char tmp[23];

	/* Prompt and ask */
	prt("Enter your player's name above (or hit ESCAPE).", 21, 2);

	/* Ask until happy */
	while (1)
	{
		/* Go to the "name" area */
		move_cursor(2, 15);

		/* Save the player name */
		strcpy(tmp, nick);

		/* Get an input, ignore "Escape" */
		if (askfor_aux(tmp, 15, 0)) strcpy(nick, tmp);

		/* All done */
		break;
	}

	/* Pad the name (to clear junk) */
	sprintf(tmp, "%-15.15s", nick);

	/* Re-Draw the name (in light blue) */
	c_put_str(TERM_L_BLUE, tmp, 2, 15);

	/* Erase the prompt, etc */
	clear_from(20);
}


/*
 * Choose the character's name
 */
void enter_password(void)
{
	int c;
	char tmp[23];

	/* Prompt and ask */
	prt("Enter your password above (or hit ESCAPE).", 21, 2);

	/* Default */
	strcpy(tmp, pass);

	/* Ask until happy */
	while (1)
	{
		/* Go to the "name" area */
		move_cursor(3, 15);

		/* Get an input, ignore "Escape" */
		if (askfor_aux(tmp, 15, 1)) strcpy(pass, tmp);

		/* All done */
		break;
	}

	/* Pad the name (to clear junk) 
	sprintf(tmp, "%-15.15s", pass); */

	 /* Re-Draw the name (in light blue) */
	for (c = 0; c < strlen(pass); c++)
		Term_putch(15+c, 3, TERM_L_BLUE, 'x');

	/* Erase the prompt, etc */
	clear_from(20);
}


/*
 * Choose the character's sex				-JWT-
 */
static void choose_sex(void)
{
	char        c;

	put_str("m) Male", 20, 2);
	put_str("f) Female", 20, 17);
	put_str("M) Hell Male", 21, 2);
	put_str("F) Hell Female", 21, 17);

	while (1)
	{
		put_str("Choose a sex (? for Help, Q to Quit): ", 19, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);
		if (c == 'm')
		{
			sex = 1;
			c_put_str(TERM_L_BLUE, "Male", 4, 15);
			break;
		}
		if (c == 'M')
		{
			sex = 3;
			c_put_str(TERM_L_BLUE, "Male", 4, 15);
			break;
		}
		else if (c == 'f')
		{
			sex = 0;
			c_put_str(TERM_L_BLUE, "Female", 4, 15);
			break;
		}
		else if (c == 'F')
		{
			sex = 2;
			c_put_str(TERM_L_BLUE, "Female", 4, 15);
			break;
		}
		else if (c == '?')
		{
			/*do_cmd_help("help.hlp");*/
		}
		else
		{
			bell();
		}
	}

	clear_from(20);
}


/*
 * Allows player to select a race			-JWT-
 */
static void choose_race(void)
{
	player_race *rp_ptr;
	int                 j, k, l, m;

	char                c;

	char		out_val[160];

	k = 0;
	l = 2;
	m = 21;


	for (j = 0; j < MAX_RACES; j++)
	{
		rp_ptr = &race_info[j];
		(void)sprintf(out_val, "%c) %s", I2A(j), rp_ptr->title);
		put_str(out_val, m, l);
		l += 15;
		if (l > 70)
		{
			l = 2;
			m++;
		}
	}

	while (1)
	{
		put_str("Choose a race (? for Help, Q to Quit): ", 20, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);
		j = (islower(c) ? A2I(c) : -1);
		if ((j < MAX_RACES) && (j >= 0))
		{
			race = j;
			rp_ptr = &race_info[j];
			c_put_str(TERM_L_BLUE, rp_ptr->title, 5, 15);
			break;
		}
		else if (c == '?')
		{
			/*do_cmd_help("help.hlp");*/
		}
		else
		{
			bell();
		}
	}

	clear_from(20);
}


/*
 * Gets a character class				-JWT-
 */
static void choose_class(void)
{
	player_class *cp_ptr;
        player_race *rp_ptr = &race_info[race];
	int          j, k, l, m;

	char         c;

	char	 out_val[160];


	/* Prepare to list */
	k = 0;
	l = 2;
	m = 21;

	/* Display the legal choices */
	for (j = 0; j < MAX_CLASS; j++)
	{
		cp_ptr = &class_info[j];

                if (!(rp_ptr->choice & BITS(j)))
                {
                        l += 15;
                        continue;
                }

		sprintf(out_val, "%c) %s", I2A(j), cp_ptr->title);
		put_str(out_val, m, l);
		l += 15;
		if (l > 70)
		{
			l = 2;
			m++;
		}
	}

	/* Get a class */
	while (1)
	{
		put_str("Choose a class (? for Help, Q to Quit): ", 20, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);
		j = (islower(c) ? A2I(c) : -1);
		if ((j < MAX_CLASS) && (j >= 0))
		{
                        if (!(rp_ptr->choice & BITS(j))) continue;

			class = j;
			cp_ptr = &class_info[j];
			c_put_str(TERM_L_BLUE, cp_ptr->title, 6, 15);
			break;
		}
		else if (c == '?')
		{
			/*do_cmd_help("help.hlp");*/
		}
		else
		{
			bell();
		}
	}

	clear_from(20);
}


/*
 * Get the desired stat order.
 */
void choose_stat_order(void)
{
	int i, j, k, avail[6];
	char c;
	char out_val[160], stats[6][4];

	/* All stats are initially available */
	for (i = 0; i < 6; i++)
	{
		strncpy(stats[i], stat_names[i], 3);
		stats[i][3] = '\0';
		avail[i] = 1;
	}

	/* Find the ordering of all 6 stats */
	for (i = 0; i < 6; i++)
	{
		/* Clear bottom of screen */
		clear_from(20);

		/* Print available stats at bottom */
		for (k = 0; k < 6; k++)
		{
			/* Check for availability */
			if (avail[k])
			{
				sprintf(out_val, "%c) %s", I2A(k), stats[k]);
				put_str(out_val, 21, k * 9);
			}
		}

		/* Get a stat */
		while (1)
		{
			put_str("Choose your stat order (? for Help, Q to Quit): ", 20, 2);
			c = inkey();
			if (c == 'Q') quit(NULL);
			j = (islower(c) ? A2I(c) : -1);
			if ((j < 6) && (j >= 0) && (avail[j]))
			{
				stat_order[i] = j;
				c_put_str(TERM_L_BLUE, stats[j], 8 + i, 15);
				avail[j] = 0;
				break;
			}
			else if (c == '?')
			{
				/*do_cmd_help("help.hlp");*/
			}
			else
			{
				bell();
			}
		}
	}

	clear_from(20);
}

/*
 * Gets a character class				-JWT-
 */
static void choose_class_mage(void)
{
	player_class *cp_ptr;
	int          j, k, l, m;

	char         c;

	char	 out_val[160];


	/* Get a class */
	while (1)
	{
		put_str("Choose the max number of blows you want(1-4) (? for Help, Q to Quit)", 20, 2);
		put_str("The less blows you have the fastest your mana regenerate :", 21, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);
		j = A2I(c);
		if ((j <= 4) && (j >= 1))
		{
			class_extra = j;
			break;
		}
		else if (c == '?')
		{
			/*do_cmd_help("help.hlp");*/
		}
		else
		{
			bell();
		}
	}

	clear_from(20);
}


/*
 * Get the name/pass for this character.
 */
void get_char_name(void)
{
	/* Clear screen */
	Term_clear();

	/* Title everything */
	put_str("Name        :", 2, 1);
	put_str("Password    :", 3, 1);

	/* Dump the default name */
	c_put_str(TERM_L_BLUE, nick, 2, 15);


	/* Display some helpful information XXX XXX XXX */

	/* Choose a name */
	choose_name();

	/* Enter password */
	enter_password();

	/* Message */
	put_str("Connecting to server....", 21, 1);

	/* Make sure the message is shown */
	Term_fresh();

	/* Note player birth in the message recall */
	c_message_add(" ");
	c_message_add("  ");
	c_message_add("====================");
	c_message_add("  ");
	c_message_add(" ");
}

/*
 * Get the other info for this character.
 */
void get_char_info(void)
{
	/* Title everything */
	put_str("Sex         :", 4, 1);
	put_str("Race        :", 5, 1);
	put_str("Class       :", 6, 1);
	put_str("Stat order  :", 8, 1);


	/* Clear bottom of screen */
	clear_from(20);

	/* Display some helpful information XXX XXX XXX */

	/* Choose a sex */
	choose_sex();

	/* Choose a race */
	choose_race();

	/* Choose a class */
	choose_class();

	class_extra = 0;	
	switch (class)
	{
		case CLASS_MAGE:
		{
//			choose_class_mage();
			break;
		}
	}
		
	/* Choose stat order */
	choose_stat_order();

	/* Clear */
	clear_from(20);

	/* Message */
	put_str("Entering game...  [Hit any key]", 21, 1);

	/* Wait for key */
	inkey();

	/* Clear */
	clear_from(20);
}

static bool enter_server_name(void)
{
	/* Clear screen */
	Term_clear();

	/* Message */
	prt("Enter the server name you want to connect to (ESCAPE to quit): ", 3, 1);

	/* Move cursor */
	move_cursor(5, 1);

	/* Default */
        strcpy(server_name, "127.0.0.1");

	/* Ask for server name */
	return askfor_aux(server_name, 80, 0);
}

/*
 * Have the player choose a server from the list given by the
 * metaserver.
 */
bool get_server_name(void)
{
	int i, j, bytes, socket, offsets[20];
	char buf[8192], *ptr, c, out_val[160];

	/* Message */
	prt("Connecting to metaserver for server list....", 1, 1);

	/* Make sure message is shown */
	Term_fresh();

	/* Connect to metaserver */
	socket = CreateClientSocket(META_ADDRESS, 8801);

	/* Check for failure */
	if (socket == -1)
	{
		return enter_server_name();
	}

	/* Read */
	bytes = SocketRead(socket, buf, 8192);

	/* Close the socket */
	SocketClose(socket);

	/* Check for error while reading */
	if (bytes <= 0)
	{
		return enter_server_name();
	}

	/* Start at the beginning */
	ptr = buf;
	i = 0;

	/* Print each server */
	while (ptr - buf < bytes)
	{
		/* Check for no entry */
		if (strlen(ptr) <= 1)
		{
			/* Increment */
			ptr++;

			/* Next */
			continue;
		}

		/* Save offset */
		offsets[i] = ptr - buf;

		/* Format entry */
		sprintf(out_val, "%c) %s", I2A(i), ptr);

		/* Strip off offending characters */
		out_val[strlen(out_val) - 1] = '\0';

		/* Print this entry */
		prt(out_val, i + 1, 1);

		/* Go to next metaserver entry */
		ptr += strlen(ptr) + 1;

		/* One more entry */
		i++;

		/* We can't handle more than 20 entries -- BAD */
		if (i > 20) break;
	}

	/* Prompt */
	prt("Choose a server to connect to (Q for manual entry): ", i + 2, 1);

	/* Ask until happy */
	while (1)
	{
		/* Get a key */
		c = inkey();

		/* Check for quit */
		if (c == 'Q')
		{
			return enter_server_name();
		}

		/* Index */
		j = (islower(c) ? A2I(c) : -1);

		/* Check for legality */
		if (j >= 0 && j < i)
			break;
	}

	/* Extract server name */
	sscanf(buf + offsets[j], "%s", server_name);

	/* Success */
	return TRUE;
}
