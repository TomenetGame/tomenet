/* $Id$ */
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
static void choose_name(void)
{
	char tmp[23];

	/* Prompt and ask */
	c_put_str(TERM_SLATE, "If you are new to TomeNET, read this:", 7, 2);
	prt("http://www.c-blue.de/rogue/tomenet-guide.txt", 8, 2);
	c_put_str(TERM_SLATE, "*** Logging in with an account ***", 12, 2);
	prt("In order to play, you need to create an account.", 14, 2);
	prt("Your account can hold a maximum of 7 different characters to play with!", 15, 2);
	prt("If you don't have an account yet, just enter one of your choice and make sure", 16, 2);
	prt("that you remember its name and password. Each player should have not more", 17, 2);
	prt("than 1 account. Ask a server administrator to 'validate' your account!", 18, 2);
	prt("If an account is not validated, it has certain restrictions to prevent abuse.", 19, 2);
	prt("Enter your account name above.", 21, 2);

	/* Ask until happy */
	while (1)
	{
		/* Go to the "name" area */
		move_cursor(2, 15);

		/* Save the player name */
		strcpy(tmp, nick);

		/* Get an input, ignore "Escape" */
		if (askfor_aux(tmp, 15, 0, 0)) strcpy(nick, tmp);

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
static void enter_password(void)
{
	int c;
	char tmp[23];

	/* Prompt and ask */
	prt("Enter your password above.", 21, 2);

	/* Default */
	strcpy(tmp, pass);

	/* Ask until happy */
	while (1)
	{
		/* Go to the "name" area */
		move_cursor(3, 15);

		/* Get an input, ignore "Escape" */
		if (askfor_aux(tmp, 15, 1, 0)) strcpy(pass, tmp);

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
	char        c='\0';		/* pfft redesign while(1) */
	bool hazard = FALSE;
	bool parity = magik(50);

	put_str("m) Male", 21, parity ? 2 : 17);
	put_str("f) Female", 21, parity ? 17 : 2);

	while (1)
	{
		c_put_str(TERM_SLATE, "Choose a sex (* for random, Q to Quit): ", 20, 2);
		if (!hazard) c = inkey();
		if (c == 'Q') quit(NULL);
		if (c == 'm')
		{
			sex = 1;
			c_put_str(TERM_L_BLUE, "Male", 4, 15);
			break;
		}
		else if (c == 'f')
		{
			sex = 0;
			c_put_str(TERM_L_BLUE, "Female", 4, 15);
			break;
		}
		else if (c == '?')
		{
			/*do_cmd_help("help.hlp");*/
		}
		else if (c == '*')
		{
			switch (rand_int(2))
			{
				case 0:
					c = 'f';
					break;
				case 1:
					c = 'm';
					break;
			}
			hazard = TRUE;
		}
		else
		{
			bell();
		}
	}

	clear_from(19);
}


/*
 * Allows player to select a race			-JWT-
 */
static void choose_race(void)
{
	player_race *rp_ptr;
	int                 j, l, m, n;

	char                c;

	char		out_val[160];


        l = 2;
        m = 22 - (Setup.max_race - 1) / 5;
        n = m - 1;

        put_str("                                                                             ", 18, l);
        put_str("                                                                             ", 19, l);
        put_str("                                                                             ", 20, 1);
        put_str("                                                                             ", 21, 1);
        put_str("                                                                             ", 22, 1);
        put_str("                                                                             ", 23, 1);


	for (j = 0; j < Setup.max_race; j++)
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
		c_put_str(TERM_SLATE, "Choose a race (* for random, Q to Quit): ", n, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);

		if (c == '*') j = rand_int(Setup.max_race);
		else j = (islower(c) ? A2I(c) : -1);

		if ((j < Setup.max_race) && (j >= 0))
		{
			race = j;
			rp_ptr = &race_info[j];
			put_str("                    ", 5, 15);
			c_put_str(TERM_L_BLUE, (char*)rp_ptr->title, 5, 15);
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

	clear_from(n);
}


/*
 * Gets a character class				-JWT-
 */
static bool choose_class(void)
{
	player_class *cp_ptr;
        player_race *rp_ptr = &race_info[race];
	int          j, l, m, n;

	char         c='\0';

	char	 out_val[160];
	bool hazard = FALSE;


        /* Prepare to list */
        l = 2;
        m = 22 - (Setup.max_class - 1) / 5;
        n = m - 1;

	c_put_str(TERM_SLATE, "--- Classes NOT recommended for beginners are: ---", n - 3, 2);
	c_put_str(TERM_SLATE, "Istar, Priest, (Archer,) Adventurer, Shaman, Runemaster", n - 2, 2);

        put_str("                                                                             ", 18, l);
        put_str("                                                                             ", 19, l);
        put_str("                                                                             ", 20, l);
        put_str("                                                                             ", 21, l);
        put_str("                                                                             ", 22, l);
        put_str("                                                                             ", 23, l);

        /* Display the legal choices */
        for (j = 0; j < Setup.max_class; j++)
        {
                cp_ptr = &class_info[j];

                if (!(rp_ptr->choice & BITS(j)))
                {
                        sprintf(out_val, "%c) %s", I2A(j), cp_ptr->title);
                        c_put_str(TERM_L_DARK, out_val, m, l);
                } else {
                        sprintf(out_val, "%c) %s", I2A(j), cp_ptr->title);
                        c_put_str(TERM_WHITE, out_val, m, l);
                }

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
                c_put_str(TERM_SLATE, "Choose a class (? for Help, * for random, Q to Quit, BACKSPACE to go back):  ", n, 2);
                if (!hazard) c = inkey();

                if (c == 'Q') quit(NULL);
                if (c == '\b') return FALSE;

		if (c == '*') hazard = TRUE;
		if (hazard) j = rand_int(Setup.max_class);
		else j = (islower(c) ? A2I(c) : -1);

		if ((j < Setup.max_class) && (j >= 0))
		{
			if (!(rp_ptr->choice & BITS(j))) continue;

			class = j;
			cp_ptr = &class_info[j];
			c_put_str(TERM_L_BLUE, (char*)cp_ptr->title, 7, 15);
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

	clear_from(n - 3); /* -3 so beginner-warnings are also cleared */
	return TRUE;
}


/*
 * Get the desired stat order.
 */
static void choose_stat_order(void)
{
	int i, j, k, avail[6], crb, maxed_stats = 0;
	char c='\0';
	char out_val[160], stats[6][4], buf[8], buf2[8];
	bool hazard = FALSE;

        player_class *cp_ptr = &class_info[class];
        player_race *rp_ptr = &race_info[race];


        /* Character stats are randomly rolled (1 time): */
        if (char_creation_flags == 0) {

		put_str("Stat order  :", 11, 1);

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

			/* Hack -- it's obvious */
			/* if (i > 4) hazard = TRUE;
			It confused too many noobiez. Taking it out for now. */

			/* Get a stat */
			while (1)
			{
				put_str("Choose your stat order (* for random, Q to Quit): ", 20, 2);
				if (hazard)
				{
					j = rand_int(6);
				}
				else
				{
					c = inkey();
					if (c == 'Q') quit(NULL);
					if (c == '*') hazard = TRUE;

					j = (islower(c) ? A2I(c) : -1);
				}

				if ((j < 6) && (j >= 0) && (avail[j]))
				{
					stat_order[i] = j;
					c_put_str(TERM_L_BLUE, stats[j], 8, 15 + i * 5);
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

        /* player can define his stats completely manually: */
        else if (char_creation_flags == 1) {
                j = 0; /* current stat to be modified */
                k = 30; /* free points left */

                clear_from(14);

                c_put_str(TERM_SLATE, "Distribute your attribute points:", 14, 5);
                c_put_str(TERM_SLATE, "Free points: ", 14, 45);
                c_put_str(TERM_L_GREEN, format("%2d", k), 14, 60);
                put_str("Use keys '+', '-', 'RETURN'", 16, 5);
                put_str("or 8/2/4/6 on the number pad", 17, 5);
                put_str("to modify and navigate.", 18, 5);
                put_str("Press ESC to proceed, after", 19, 5);
                put_str("you distributed all points.", 20, 5);
                put_str("(Press 'Q' to quit.)", 21, 5);
                c_put_str(TERM_SLATE, "No more than 1 attribute out of the 6 is allowed to be maximised.", 23, 5);

		c_put_str(TERM_L_UMBER, "Strength -    ", 3, 30);
		c_put_str(TERM_YELLOW, "  How quickly you can strike.", 4, 30);
		c_put_str(TERM_YELLOW, "  How much you can carry and wield.", 5, 30);
		c_put_str(TERM_YELLOW, "  How much damage your strikes inflict.", 6, 30);
		c_put_str(TERM_YELLOW, "  How easily you can bash, throw and dig.", 7, 30);
		c_put_str(TERM_YELLOW, "  Slightly helps you swimming.", 8, 30);

                for (i = 0; i < 6; i++) {
                        stat_order[i] = 10;

			strncpy(stats[i], stat_names[i], 3);
			stats[i][3] = '\0';
		}

		while (1)
		{
			c_put_str(TERM_L_GREEN, format("%2d", k), 14, 60);

			for (i = 0; i < 6; i++) {
				crb = stat_order[i] + cp_ptr->c_adj[i] + rp_ptr->r_adj[i];
				if (crb > 18) crb = 18 + (crb - 18) * 10;
				cnv_stat(crb, buf);
				cnv_stat(stat_order[i], buf2);
                        	sprintf(out_val, "%s: %s      (base %s)", stats[i], buf, buf2);

                        	if (j == i) {
                        		if (stat_order[i] == 10-2)
                        			c_put_str(TERM_L_RED, out_val, 16 + i, 45);
                        		else if (stat_order[i] == 17)
                        			c_put_str(TERM_L_BLUE, out_val, 16 + i, 45);
                        		else
                        			c_put_str(TERM_ORANGE, out_val, 16 + i, 45);
                        	} else {
                        		if (stat_order[i] == 10-2)
		                        	c_put_str(TERM_RED, out_val, 16 + i, 45);
                        		else if (stat_order[i] == 17)
                        			c_put_str(TERM_VIOLET, out_val, 16 + i, 45);
		                        else
		                        	c_put_str(TERM_L_UMBER, out_val, 16 + i, 45);
		                }
                	}

			c = inkey();
			crb = cp_ptr->c_adj[j] + rp_ptr->r_adj[j];
			if (c == '-' || c == '4') {
				if (stat_order[j] > 10-2 && stat_order[j]+crb > 3) {
					if (stat_order[j] <= 12) {
						/* intermediate */
						stat_order[j]--;
						k++;
					} else if (stat_order[j] <= 14) {
						/* high */
						stat_order[j]--;
						k+=2;
					} else if (stat_order[j] <= 16) {
						/* nearly max */
						stat_order[j]--;
						k+=3;
					} else {
						/* max! */
						stat_order[j]--;
						k+=4;
						maxed_stats--;
					}
				}
			}
			if (c == '+' || c == '6') {
				if (stat_order[j] < 17) {
					if (stat_order[j] < 12 && k >= 1) {
						/* intermediate */
						stat_order[j]++;
						k--;
					} else if (stat_order[j] < 14 && k >= 2) {
						/* high */
						stat_order[j]++;
						k-=2;
					} else if (stat_order[j] < 16 && k >= 3) {
						/* nearly max */
						stat_order[j]++;
						k-=3;
					} else if (k >= 4 && !maxed_stats) { /* only 1 maxed stat is allowed */
						/* max! */
						stat_order[j]++;
						k-=4;
						maxed_stats++;
					}
				}
			}
			if (c == '\r' || c == '2') j = (j+1) % 6;
			if (c == '8' || c == '\b') j = (j+5) % 6;
			if (c == '\r' || c == '2' || c == '8' || c == '\b') {
				switch (j) {
				case 0:	c_put_str(TERM_L_UMBER, "Strength -    ", 3, 30);
					c_put_str(TERM_YELLOW, "  How quickly you can strike.                  ", 4, 30);
					c_put_str(TERM_YELLOW, "  How much you can carry and wield.            ", 5, 30);
					c_put_str(TERM_YELLOW, "  How much damage your strikes inflict.        ", 6, 30);
					c_put_str(TERM_YELLOW, "  How easily you can bash, throw and dig.      ", 7, 30);
					c_put_str(TERM_YELLOW, "  Slightly helps you swimming.                 ", 8, 30);
					c_put_str(TERM_YELLOW, "                                               ", 9, 30);
					c_put_str(TERM_YELLOW, "                                               ", 10, 30);
					c_put_str(TERM_YELLOW, "                                               ", 11, 30);
					break;
				case 1:	c_put_str(TERM_L_UMBER, "Intelligence -", 3, 30);
					c_put_str(TERM_YELLOW, "  How well you can use magic                   ", 4, 30);
					c_put_str(TERM_YELLOW, "    (depending on your class and spells).      ", 5, 30);
					c_put_str(TERM_YELLOW, "  How well you can use magic devices.          ", 6, 30);
					c_put_str(TERM_YELLOW, "  Helps your disarming skill.                  ", 7, 30);
					c_put_str(TERM_YELLOW, "  Helps noticing attempts to steal from you.   ", 8, 30);
					c_put_str(TERM_YELLOW, "                                               ", 9, 30);
					c_put_str(TERM_YELLOW, "                                               ", 10, 30);
					c_put_str(TERM_YELLOW, "                                               ", 11, 30);
					break;
				case 2:	c_put_str(TERM_L_UMBER, "Wisdom -      ", 3, 30);
					c_put_str(TERM_YELLOW, "  How well you can use prayers and magic       ", 4, 30);
					c_put_str(TERM_YELLOW, "    (depending on your class and spells).      ", 5, 30);
					c_put_str(TERM_YELLOW, "  How well can you resist malicious effects    ", 6, 30);
					c_put_str(TERM_YELLOW, "    and influences on both body and mind.      ", 7, 30);
					c_put_str(TERM_YELLOW, "                                               ", 8, 30);
					c_put_str(TERM_YELLOW, "                                               ", 9, 30);
					c_put_str(TERM_YELLOW, "                                               ", 10, 30);
					c_put_str(TERM_YELLOW, "                                               ", 11, 30);
					break;
				case 3:	c_put_str(TERM_L_UMBER, "Dexterity -   ", 3, 30);
					c_put_str(TERM_YELLOW, "  How quickly you can strike.                  ", 4, 30);
					c_put_str(TERM_YELLOW, "  Increases your chance to not miss.           ", 5, 30);
					c_put_str(TERM_YELLOW, "  Opponents will miss very slightly more often.", 6, 30);
					c_put_str(TERM_YELLOW, "  Helps your stealing skills (if any).         ", 7, 30);
					c_put_str(TERM_YELLOW, "  Helps to prevent foes stealing from you.     ", 8, 30);
					c_put_str(TERM_YELLOW, "  Helps keeping your balance after bashing.    ", 9, 30);
					c_put_str(TERM_YELLOW, "  Helps your disarming skill.                  ", 10, 30);
					c_put_str(TERM_YELLOW, "  Slightly helps you swimming.                 ", 11, 30);
					break;
				case 4:	c_put_str(TERM_L_UMBER, "Constitution -", 3, 30);
					c_put_str(TERM_YELLOW, "  Determines your amout of HP                  ", 4, 30);
					c_put_str(TERM_YELLOW, "    (hit points, ie how much damage you can    ", 5, 30);
					c_put_str(TERM_YELLOW, "    take without dieing.                       ", 6, 30);
					c_put_str(TERM_YELLOW, "    Very high constitution might not show much ", 7, 30);
					c_put_str(TERM_YELLOW, "    effect until your character also reaches   ", 8, 30);
					c_put_str(TERM_YELLOW, "    an appropriate level.)                     ", 9, 30);
					c_put_str(TERM_YELLOW, "  Reduces the duration of poisonous effects.   ", 10, 30);
					c_put_str(TERM_YELLOW, "  Helps your character not to drown easily.    ", 11, 30);
					break;
				case 5:	c_put_str(TERM_L_UMBER, "Charisma -    ", 3, 30);
					c_put_str(TERM_YELLOW, "  Shops will offer you wares at better prices. ", 4, 30);
					c_put_str(TERM_YELLOW, "    (Note that shop keepers are also influenced", 5, 30);
					c_put_str(TERM_YELLOW, "    by your character's race.)                 ", 6, 30);
					c_put_str(TERM_YELLOW, "  Helps you to resist seducing attacks.        ", 7, 30);
					c_put_str(TERM_YELLOW, "                                               ", 8, 30);
					c_put_str(TERM_YELLOW, "                                               ", 9, 30);
					c_put_str(TERM_YELLOW, "                                               ", 10, 30);
					c_put_str(TERM_YELLOW, "                                               ", 11, 30);
					break;
				}
			}
			if (c == '\e') break;
			if (c == 'Q') quit(NULL);
		}

		c_put_str(TERM_L_UMBER, "              ", 3, 30);
		c_put_str(TERM_YELLOW, "                                                ", 4, 30);
		c_put_str(TERM_YELLOW, "                                                ", 5, 30);
		c_put_str(TERM_YELLOW, "                                                ", 6, 30);
		c_put_str(TERM_YELLOW, "                                                ", 7, 30);
		c_put_str(TERM_YELLOW, "                                                ", 8, 30);
		c_put_str(TERM_YELLOW, "                                                ", 9, 30);
		c_put_str(TERM_YELLOW, "                                                ", 10, 30);
		c_put_str(TERM_YELLOW, "                                                ", 11, 30);
		clear_from(14);
        }
}

/* Quick hack!		- Jir -
 * TODO: remove hard-coded things. */
static void choose_mode(void)
{
	char        c='\0';
	bool hazard = FALSE;

#if 0
	put_str("n) Normal", 20, 2);
	put_str("f) Fruit bat", 20, 15 + 2);
	put_str("g) no Ghost", 20, 30 + 2);
	put_str("h) Hard", 20, 45 + 2);
	put_str("H) Hellish", 20, 60 + 2);
	put_str("e) Everlasting", 21, 2);
#else
	put_str("n) Normal", 16, 2);
	c_put_str(TERM_SLATE, "(3 lifes)", 16, 12);
	put_str("g) No Ghost", 17, 2);
	c_put_str(TERM_SLATE, "('Unworldly' - One life only. The traditional rogue-like way)", 17, 14);
	put_str("e) Everlasting", 18, 2);
	c_put_str(TERM_SLATE, "(You may resurrect infinite times, but cannot enter highscore)", 18, 17);
	put_str("h) Hard", 19, 2);
	c_put_str(TERM_SLATE, "('Purgatorial' - like normal, with nasty additional penalties)", 19, 10);
	put_str("H) Hellish", 20, 2);
	c_put_str(TERM_SLATE, "(Combination of Hard + No Ghost)", 20, 13);
	put_str("p) PvP", 21, 2);
	c_put_str(TERM_SLATE, "(Can't beat the game, instead special 'player vs player' rules apply)", 21, 9);
#endif

	while (1)
	{
		c_put_str(TERM_SLATE, "Choose a mode (* for random, Q to Quit): ", 15, 2);
		if (!hazard) c = inkey();
		if (c == 'Q') quit(NULL);
#if 0
		if (c == 'f')
		{
			sex += 512;
			c_put_str(TERM_L_BLUE, "Fruit Bat", 9, 15);
			break;
		}
#else
		if (c == 'p')
		{
			sex += MODE_PVP;
			c_put_str(TERM_L_BLUE, "PvP", 9, 15);
			break;
		}
#endif
		else if (c == 'n')
		{
			c_put_str(TERM_L_BLUE, "Normal", 9, 15);
			break;
		}
		else if (c == 'g')
		{
			sex += MODE_NO_GHOST;
			c_put_str(TERM_L_BLUE, "No Ghost", 9, 15);
			break;
		}
		else if (c == 'h')
		{
			sex += (MODE_HARD);
			c_put_str(TERM_L_BLUE, "Hard", 9, 15);
			break;
		}
		else if (c == 'H')
		{
			sex += (MODE_NO_GHOST + MODE_HARD);
			c_put_str(TERM_L_BLUE, "Hellish", 9, 15);
			break;
		}
		else if (c == 'e')
		{
			sex += MODE_EVERLASTING;
			c_put_str(TERM_L_BLUE, "Everlasting", 9, 15);
			break;
		}
		else if (c == '?')
		{
			/*do_cmd_help("help.hlp");*/
		}
		else if (c == '*')
		{
			switch (rand_int(5))
			{
				case 0:
					c = 'n';
					break;
				case 1:
					c = 'p';
					break;
				case 2:
					c = 'g';
					break;
				case 3:
					c = 'h';
					break;
				case 4:
					c = 'H';
					break;
				case 5:
					c = 'e';
					break;
			}
			hazard = TRUE;
		}
		else
		{
			bell();
		}
	}

	clear_from(15);
}

/* Fruit bat is now a "body modification" that can be applied to all "modes" - C. Blue */
static void choose_body_modification(void)
{
	char        c='\0';
	bool hazard = FALSE;

	put_str("n) Normal body", 20, 2);
	put_str("f) Fruit bat", 21, 2);
	c_put_str(TERM_SLATE, "(Bats are faster and vampiric, but can't wear certain items)", 21, 15);

	while (1)
	{
		c_put_str(TERM_SLATE, "Choose a body modification (* for random, Q to Quit): ", 19, 2);
		if (!hazard) c = inkey();
		if (c == 'Q') quit(NULL);

		if (c == 'f')
		{
			sex += MODE_FRUIT_BAT;
			c_put_str(TERM_L_BLUE, "Fruit bat", 6, 15);
			break;
		}
		else if (c == 'n')
		{
			c_put_str(TERM_L_BLUE, "Normal body", 6, 15);
			break;
		}
		else if (c == '?')
		{
			/*do_cmd_help("help.hlp");*/
		}
		else if (c == '*')
		{
			switch (rand_int(2))
			{
				case 0:
					c = 'n';
					break;
				case 1:
					c = 'f';
					break;
			}
			hazard = TRUE;
		}
		else
		{
			bell();
		}
	}

	clear_from(19);
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
	c_put_str(TERM_SLATE, "If TomeNET quits with a login failure, check account name and password", 4, 3);
	c_put_str(TERM_SLATE, "for upper/lower case! If you are a new player, try a different name.", 5, 3);

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
	put_str("Body        :", 6, 1);
	put_str("Class       :", 7, 1);
	put_str("Mode        :", 9, 1);

	/* Clear bottom of screen */
	clear_from(20);

	/* Display some helpful information XXX XXX XXX */

	/* Choose a sex */
	choose_sex();

        do {
                /* Choose a race */
                choose_race();
		/* Choose character's body modification */
		choose_body_modification();
                /* Choose a class */
        } while (!choose_class());

	class_extra = 0;

	/* Choose stat order */
	choose_stat_order();

	/* Choose character mode */
	choose_mode();
	
	/* Clear */
	clear_from(15);

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
        strcpy(server_name, "62.210.141.11");

	/* Ask for server name */
	return askfor_aux(server_name, 80, 0, 0);
}

/*
 * Have the player choose a server from the list given by the
 * metaserver.
 */
/* TODO:
 * - wrap the words using strtok
 * - allow to scroll the screen in case
 * */

bool get_server_name(void)
{
        s32b i;
        cptr tmp;

#ifdef EXPERIMENTAL_META
	int j, bytes, socket = -1;
	char buf[80192], c;
#else
	int j, k, l, bytes, socket = -1, offsets[20], lines = 0;
	char buf[80192], *ptr, c, out_val[260];
#endif

        /* We NEED lua here, so if it aint initialized yet, do it */
        init_lua();

	/* Message */
	prt("Connecting to metaserver for server list....", 1, 1);

	/* Make sure message is shown */
	Term_fresh();

	if (strlen(meta_address) > 0) {
		/* Metaserver in config file */
		socket = CreateClientSocket(meta_address, 8801);
	}

	/* Failed to connect to metaserver in config file, connect to hard-coded address */
	if (socket == -1) {
	        /* Connect to metaserver */
		socket = CreateClientSocket(META_ADDRESS, 8801);
	}

	/* Check for failure */
	if (socket == -1)
	{
		/* Hack -- Connect to metaserver #2 */
		socket = CreateClientSocket(META_ADDRESS_2, 8801);

		/* Check for failure */
		if (socket == -1)
		{
			return enter_server_name();
		}
	}

	/* Read */
	bytes = SocketRead(socket, buf, 80192);

	/* Close the socket */
	SocketClose(socket);

	/* Check for error while reading */
	if (bytes <= 0)
	{
		return enter_server_name();
	}

        Term_clear();

#ifdef EXPERIMENTAL_META
        call_lua(0, "meta_display", "(s)", "d", buf, &i);
#else

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

		j = strlen(out_val);

		/* Strip off offending characters */
		out_val[j - 1] = '\0';
		out_val[j]='\0';

		k=0;
		while(j){
			l=strlen(&out_val[k]);
			if(j > 75){
				l=75;
				while(out_val[k+l]!=' ') l--;
				out_val[l]='\0';
			}
			prt(out_val+k, lines++, (k ? 4 : 1));
			k+=(l+1);
			j=strlen(&out_val[k]);
		}

		/* Go to next metaserver entry */
		ptr += strlen(ptr) + 1;

		/* One more entry */
		i++;

		/* We can't handle more than 20 entries -- BAD */
		if (lines > 20) break;
	}

	/* Message */
	prt(longVersion , lines + 1, 1);

	/* Prompt */
	prt("-- Choose a server to connect to (Q for manual entry): ", lines + 2, 1);
#endif

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
		else if (c == ESCAPE)
		{
			quit(NULL);
			return FALSE;
		}

		/* Index */
		j = (islower(c) ? A2I(c) : -1);

		/* Check for legality */
		if (j >= 0 && j < i)
			break;
	}

#ifdef EXPERIMENTAL_META
        call_lua(0, "meta_get", "(s,d)", "sd", buf, j, &tmp, &server_port);
        strcpy(server_name, tmp);
#else
	/* Extract server name */
        sscanf(buf + offsets[j], "%s", server_name);
#endif

	/* Success */
	return TRUE;
}

/* Human */
static char *human_syllable1[] =
{
	"Ab", "Ac", "Ad", "Af", "Agr", "Ast", "As", "Al", "Adw", "Adr", "Ar",
	"B", "Br", "C", "Cr", "Ch", "Cad", "D", "Dr", "Dw", "Ed", "Eth", "Et",
	"Er", "El", "Eow", "F", "Fr", "G", "Gr", "Gw", "Gal", "Gl", "H", "Ha",
	"Ib", "Jer", "K", "Ka", "Ked", "L", "Loth", "Lar", "Leg", "M", "Mir",
	"N", "Nyd", "Ol", "Oc", "On", "P", "Pr", "R", "Rh", "S", "Sev", "T",
	"Tr", "Th", "V", "Y", "Z", "W", "Wic",
};

static char *human_syllable2[] =
{
	"a", "ae", "au", "ao", "are", "ale", "ali", "ay", "ardo", "e", "ei",
	"ea", "eri", "era", "ela", "eli", "enda", "erra", "i", "ia", "ie",
	"ire", "ira", "ila", "ili", "ira", "igo", "o", "oa", "oi", "oe",
	"ore", "u", "y",
};

static char *human_syllable3[] =
{
	"a", "and", "b", "bwyn", "baen", "bard", "c", "ctred", "cred", "ch",
	"can", "d", "dan", "don", "der", "dric", "dfrid", "dus", "f", "g",
	"gord", "gan", "l", "li", "lgrin", "lin", "lith", "lath", "loth",
	"ld", "ldric", "ldan", "m", "mas", "mos", "mar", "mond", "n",
	"nydd", "nidd", "nnon", "nwan", "nyth", "nad", "nn", "nnor", "nd",
	"p", "r", "ron", "rd", "s", "sh", "seth", "sean", "t", "th", "tha",
	"tlan", "trem", "tram", "v", "vudd", "w", "wan", "win", "wyn", "wyr",
	"wyr", "wyth",
};

/*
 * Random Name Generator
 * based on a Javascript by Michael Hensley
 * "http://geocities.com/timessquare/castle/6274/"
 */
void create_random_name(int race, char *name)
{
	char *syl1, *syl2, *syl3;

	int idx;

	/* Paranoia */
	if (!name) return;

	idx = rand_int(sizeof(human_syllable1) / sizeof(char *));
	syl1 = human_syllable1[idx];
	idx = rand_int(sizeof(human_syllable2) / sizeof(char *));
	syl2 = human_syllable2[idx];
	idx = rand_int(sizeof(human_syllable3) / sizeof(char *));
	syl3 = human_syllable3[idx];

	/* Concatenate selected syllables */
	strnfmt(name, 32, "%s%s%s", syl1, syl2, syl3);
}

