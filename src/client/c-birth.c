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

/* Cut down on some of the walls of text hitting the player during login process? */
#define SIMPLE_LOGIN

/* Choose class before race */
#define CLASS_BEFORE_RACE

/* Don't display 'Trait' if traits aren't available */
#define HIDE_UNAVAILABLE_TRAIT

/* For race/class/trait descriptions */
#define DIZ_ROW 2
#define DIZ_COL 29

/* For character parameter list [15] */
#define CHAR_COL 9

/* Login screen text placement */
#ifdef ATMOSPHERIC_INTRO
 #define LOGIN_ROW 9 /*1*/
 #define TORCH_CC_X 27
 #define DRAW_TORCH \
	c_put_str(TERM_FIRE,  "^", 3, TORCH_CC_X); \
	c_put_str(TERM_FIRE,  "*", 4, TORCH_CC_X); \
	c_put_str(TERM_EMBER, "#", 5, TORCH_CC_X); \
	c_put_str(TERM_UMBER, "|", 6, TORCH_CC_X); \
	c_put_str(TERM_UMBER, "|", 7, TORCH_CC_X); \
	c_put_str(TERM_UMBER, "|", 8, TORCH_CC_X); \
	c_put_str(TERM_UMBER, "|", 9, TORCH_CC_X); \
	c_put_str(TERM_L_DARK,"~",10, TORCH_CC_X);
#else
 #define LOGIN_ROW 2
#endif

/*
 * Choose the character's name
 */
static void choose_name(void) {
	char tmp[ACCOUNTNAME_LEN], fmt[ACCOUNTNAME_LEN], *cp;

	/* Prompt and ask */
#ifndef SIMPLE_LOGIN
	c_put_str(TERM_SLATE, "If you are new to TomeNET, read this:", 7, 2);
	prt("http://www.tomenet.eu/guide.php", 8, 2);
	c_put_str(TERM_SLATE, "*** Logging in with an account ***", 12, 2);
	prt("In order to play, you need to create an account.", 14, 2);
	prt("Your account can hold a maximum of 7 different characters to play with!", 15, 2);
	prt("If you don't have an account yet, just enter one of your choice and make sure", 16, 2);
	prt("that you remember its name and password. Each player should have not more", 17, 2);
	prt("than 1 account. Ask a server administrator to 'validate' your account!", 18, 2);
	prt("If an account is not validated, it has certain restrictions to prevent abuse.", 19, 2);
#else
 #ifdef ATMOSPHERIC_INTRO /* ascii-art in the top area */
  #define LOGO_ROW 1
  #define LOGO_ATTR TERM_EMBER
	/* display a title */
  #if 0
	c_put_str(TERM_FIRETHIN, "  ^^^^^^^^^   ^^^^^^^    ^^^ ^^^      ^^^^^^^    ^^^  ^^^    ^^^^^^^   ^^^^^^^^^", LOGO_ROW, 0);
	c_put_str(TERM_LITE,     " /########/  /######/   /##|/##|     /######/   /##| /##/   /######/  /########/", LOGO_ROW + 1, 0);
	c_put_str(TERM_FIRE,     "   /##/     /##/ ##/   /###/###|    /##/       /###|/##/   /##/         /##/    ", LOGO_ROW + 2, 0);
	c_put_str(TERM_FIRE,     "  /##/     /##/ ##/   /########|   /######/   /###/###/   /######/     /##/     ", LOGO_ROW + 3, 0);
	c_put_str(TERM_FIRE,     " /##/     /##/ ##/   /##|##||##|  /##/       /##/|###/   /##/         /##/      ", LOGO_ROW + 4, 0);
	c_put_str(TERM_EMBER,    "/##/     /######/   /##/|##||##| /######/   /##/ |##/   /######/     /##/       ", LOGO_ROW + 5, 0);
  #else
	c_put_str(TERM_FIRETHIN, " ^^^^^^^^^^  ^^^^^^^^  ^^^^    ^^^^  ^^^^^^^^   ^^^  ^^^^  ^^^^^^^^  ^^^^^^^^^^", LOGO_ROW, 0);
	c_put_str(TERM_LITE,     " |########|  |######|  |###\\  /###|  |######|   |##\\ |##|  |######|  |########|", LOGO_ROW + 1, 0);
	c_put_str(TERM_FIRE,     " |########|  |##| ##|  |####\\/####|  |##|       |###\\|##|  |##|      |########|", LOGO_ROW + 2, 0);
	c_put_str(TERM_FIRE,     "    |##|     |##| ##|  |##########|  |######|   |#######|  |######|     |##|   ", LOGO_ROW + 3, 0);
	c_put_str(TERM_FIRE,     "    |##|     |##| ##|  |##|\\##/|##|  |##|       |##|\\###|  |##|         |##|   ", LOGO_ROW + 4, 0);
	c_put_str(TERM_EMBER,    "    |##|     |######|  |##| \\/ |##|  |######|   |##| \\##|  |######|     |##|   ", LOGO_ROW + 5, 0);
  #endif
 #endif
	c_put_str(TERM_SLATE, "Welcome! In order to play, you need to create an account.", LOGIN_ROW, 2);
	c_put_str(TERM_SLATE, "If you don't have an account yet, just enter one of your choice. Remember", LOGIN_ROW + 1, 2);
	c_put_str(TERM_SLATE, "your name and password. Players may only own one account each at a time.", LOGIN_ROW + 2, 2);
	c_put_str(TERM_SLATE, "If you are new to TomeNET, this guide may prove useful:", LOGIN_ROW + 9, 2);
	prt("http://www.tomenet.eu/guide.php", LOGIN_ROW + 10, 2);
#endif
#ifndef SIMPLE_LOGIN
	prt("Enter your account name above.", 21, 2);
#endif

	enter_name:

	/* Ask until happy */
	while (1) {
		/* Go to the "name" area */
#ifndef SIMPLE_LOGIN
		move_cursor(2, 15);
#else
		move_cursor(LOGIN_ROW + 4, 15);
#endif
		/* Save the player name */
		strcpy(tmp, nick);

		/* Get an input, ignore "Escape" */
#ifdef RETRY_LOGIN
		/* Name was ok, just skip to password? */
		if (rl_password) rl_password = FALSE;
		else
#endif
		if (askfor_aux(tmp, ACCOUNTNAME_LEN - 1, 0)) strcpy(nick, tmp);
		/* at this point: ESC = quit game */
		else exit(0);

		/* All done */
		break;
	}

	/* Trim spaces right away */
	strcpy(fmt, nick);
	cp = fmt;
	while (*cp == ' ') cp++;
	strcpy(nick, cp);
	cp = nick + (strlen(nick) - 1);
	while (*cp == ' ') {
		*cp = 0;
		cp--;
	}

	/* Don't allow empty name */
	if (!nick[0]) {
		rl_password = FALSE; //paranoia, effectless
		goto enter_name;
	}

	/* Always start on upper-case */
	nick[0] = toupper(nick[0]);

	/* Pad the name (to clear junk) */
	sprintf(fmt, "%%-%d.%ds", ACCOUNTNAME_LEN - 1, ACCOUNTNAME_LEN - 1);
	sprintf(tmp, fmt, nick);

	/* Re-Draw the name (in light blue) */
#ifndef SIMPLE_LOGIN
	c_put_str(TERM_L_BLUE, tmp, 2, 15);
#else
	c_put_str(TERM_L_BLUE, tmp, LOGIN_ROW + 4, 15);
#endif

	/* Erase the prompt, etc */
	clear_from(20);
}


/*
 * Choose the character's name
 */
static bool enter_password(void) {
	size_t c;
	char tmp[PASSWORD_LEN];

#ifndef SIMPLE_LOGIN
	/* Prompt and ask */
	prt("Enter your password above.", 21, 2);
#endif

	/* Default */
	strcpy(tmp, pass);

	/* Ask until happy */
	while (1) {
		/* Go to the "name" area */
#ifndef SIMPLE_LOGIN
		move_cursor(3, 15);
#else
		move_cursor(LOGIN_ROW + 5, 15);
#endif

		/* Get an input, ignore "Escape" */
		if (askfor_aux(tmp, PASSWORD_LEN - 1, ASKFOR_PRIVATE)) strcpy(pass, tmp);
		/* abort and jump back to name prompt? */
		else return FALSE;

		/* Don't allow empty password */
		if (!pass[0]) continue;

		/* All done */
		break;
	}

	/* Pad the name (to clear junk)
	sprintf(tmp, "%-15.15s", pass); */

	 /* Re-Draw the password as 'x's (in light blue) */
	for (c = 0; c < strlen(pass); c++)
#ifndef SIMPLE_LOGIN
		Term_putch(15+c, 3, TERM_L_BLUE, 'x');
#else
		Term_putch(15+c, LOGIN_ROW + 5, TERM_L_BLUE, 'x');
#endif

	/* Erase the prompt, etc */
	clear_from(20);
	return TRUE;
}


/*
 * Choose the character's sex				-JWT-
 */
static bool choose_sex(void) {
	char c = '\0';		/* pfft redesign while(1) */
	bool hazard = FALSE;
	bool parity = magik(50);

	put_str("m) Male", 21, parity ? 2 : 17);
	put_str("f) Female", 21, parity ? 17 : 2);
	put_str("      ", 4, CHAR_COL);
	if (valid_dna) {
		if (dna_sex % 2) c_put_str(TERM_SLATE, "Male", 4, CHAR_COL);
		else c_put_str(TERM_SLATE, "Female", 4, CHAR_COL);
	}

	if (auto_reincarnation) {
		hazard = TRUE;
		c = '#';
	}

#ifdef ATMOSPHERIC_INTRO
	DRAW_TORCH
#endif

	while (1) {
		if (valid_dna) c_put_str(TERM_SLATE, "Choose a sex (* for random, \377B#\377s/\377B%\377s to reincarnate, Q to quit): ", 20, 2);
		else c_put_str(TERM_SLATE, "Choose a sex (* for random, Q to quit): ", 20, 2);

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		if (!hazard) c = inkey();
		if (c == 'Q' || c == KTRL('Q')) quit(NULL);
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) return FALSE;
#endif
		if (c == 'm') {
			sex = 1;
			put_str("      ", 4, CHAR_COL);
			c_put_str(TERM_L_BLUE, "Male", 4, CHAR_COL);
			break;
		} else if (c == 'f') {
			sex = 0;
			put_str("      ", 4, CHAR_COL);
			c_put_str(TERM_L_BLUE, "Female", 4, CHAR_COL);
			break;
		} else if (c == '?') {
			/*do_cmd_help("help.hlp");*/
		} else if (c == '*') {
			switch (rand_int(2)) {
				case 0:
					c = 'f';
					break;
				case 1:
					c = 'm';
					break;
			}
			hazard = TRUE;
		}
		else if (c == '#') {
			if (valid_dna) {
				if (dna_sex % 2) c = 'm';
				else c = 'f';
				hazard = TRUE;
			} else {
				hazard = FALSE;
				auto_reincarnation = FALSE;
				continue;
			}
		} else if (c == '%' && valid_dna) {
			c = '#';
			hazard = TRUE;
			auto_reincarnation = TRUE;
			continue;
		} else bell();
	}

	clear_from(19);
	return TRUE;
}


static void clear_diz(void) {
	int i;
	//c_put_str(TERM_UMBER, "                              ", DIZ_ROW, DIZ_COL);
	for (i = 0; i < 12; i++)
		c_put_str(TERM_L_UMBER, "                                                  ", DIZ_ROW + i, DIZ_COL);
}

static void display_race_diz(int r) {
	int i = 0;

	clear_diz();
	if (!race_diz[r][i]) return; /* server !newer_than 4.5.1.2 */

//	c_put_str(TERM_UMBER, format("--- %s ---", race_info[r].title), DIZ_ROW, DIZ_COL);
	while (i < 12 && race_diz[r][i][0]) {
		c_put_str(TERM_L_UMBER, race_diz[r][i], DIZ_ROW + i, DIZ_COL);
		i++;
	}
}

/*
 * Allows player to select a race			-JWT-
 */
static bool choose_race(void) {
	player_race *rp_ptr;
	int i, j, l, m, n, sel = 0;
	char c = '\0';
	char out_val[160];
	bool hazard = FALSE, sel_ok = FALSE;

	/* ENABLE_DEATHKNIGHT hack: If we went back here from next menu, change 'Death Knight' choice back to
	   the more generic 'Paladin', or only 'Vampire' would be highlighted as noone else can pick DK. */
	if (class == CLASS_DEATHKNIGHT) {
		class = CLASS_PALADIN;
		p_ptr->cp_ptr = &class_info[class];
		/* Redraw proper class name */
 #ifndef CLASS_BEFORE_RACE
		c_put_str(TERM_L_BLUE, "                    ", 7, CHAR_COL);
		c_put_str(TERM_L_BLUE, (char*)p_ptr->cp_ptr->title, 7, CHAR_COL);
 #else
		c_put_str(TERM_L_BLUE, "                    ", 5, CHAR_COL);
		c_put_str(TERM_L_BLUE, (char*)p_ptr->cp_ptr->title, 5, CHAR_COL);
 #endif
	}

	for (i = 18; i < 24; i++) Term_erase(1, i, 255);

race_redraw:
	l = 2;
	m = 22 - (Setup.max_race - 1) / 5;
	n = m - 1;

	for (j = 0; j < Setup.max_race; j++) {
		rp_ptr = &race_info[j];
		sprintf(out_val, "%c) %s", I2A(j), rp_ptr->title);

#ifdef CLASS_BEFORE_RACE
		if (!(rp_ptr->choice & BITS(class))) {
			c_put_str(TERM_L_DARK, out_val, m, l);
			if (!sel_ok) sel++;
		} else
#endif
		{
			sel_ok = TRUE;
			if (j == sel) c_put_str(TERM_YELLOW, out_val, m, l);
			else c_put_str(TERM_WHITE, out_val, m, l);
		}
		l += 15;
		if (l > 70) {
			l = 2;
			m++;
		}
	}
#ifndef CLASS_BEFORE_RACE
	c_put_str(TERM_L_BLUE, "                    ", 5, CHAR_COL);
	if (valid_dna && (dna_race >= 0 && dna_race < Setup.max_race)) c_put_str(TERM_SLATE, race_info[dna_race].title, 5, CHAR_COL);
#else
	c_put_str(TERM_L_BLUE, "                    ", 6, CHAR_COL);
	if (valid_dna && (dna_race >= 0 && dna_race < Setup.max_race)) c_put_str(TERM_SLATE, race_info[dna_race].title, 6, CHAR_COL);
#endif

	if (auto_reincarnation) {
		hazard = TRUE;
		c = '#';
	}

#ifdef ATMOSPHERIC_INTRO
	DRAW_TORCH
#endif

	while (1) {
		if (valid_dna) c_put_str(TERM_SLATE, "Choose a race (* random, \377B#\377s/\377B%\377s reincarnate, Q quit, BACKSPACE back, 2/4/6/8): ", n, 2);
		else c_put_str(TERM_SLATE, "Choose a race (* for random, Q to quit, BACKSPACE to go back, 2/4/6/8): ", n, 2);
		display_race_diz(sel);

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		if (!hazard) c = inkey();

		if (c == 'Q' || c == KTRL('Q')) quit(NULL);
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) return FALSE;
#endif
		if (c == '\b') {
			clear_diz();
			clear_from(n);
			return FALSE;
		}

		/* Allow 'navigating', to highlight and display the descriptive text */
		if (c == '4' || c == '-') {//j
			sel = (Setup.max_race + sel - 1) % Setup.max_race;
			while (!(race_info[sel].choice & BITS(class)))
			    sel = (Setup.max_race + sel - 1) % Setup.max_race;
			goto race_redraw;
		}
		if (c == '6' || c == '+') {//k
			sel = (sel + 1) % Setup.max_race;
			while (!(race_info[sel].choice & BITS(class)))
			    sel = (sel + 1) % Setup.max_race;
			goto race_redraw;
		}
		if (c == '8' || c == '<') {//h
			if (sel - 5 < 0) sel = sel + ((Setup.max_race - 1 - sel) / 5) * 5;
			else sel -= 5;
			while (!(race_info[sel].choice & BITS(class))) {
				if (sel - 5 < 0) sel = sel + ((Setup.max_race - 1 - sel) / 5) * 5;
				else sel -= 5;
			}
			goto race_redraw;
		}
		if (c == '2' || c == '>') {//l
			if (sel + 5 >= Setup.max_race) sel = sel % 5;
			else sel += 5;
			while (!(race_info[sel].choice & BITS(class))) {
				if (sel + 5 >= Setup.max_race) sel = sel % 5;
				else sel += 5;
			}
			goto race_redraw;
		}
		if (c == '\r' || c == '\n') c = 'a' + sel;
		
		if (c == '*') hazard = TRUE;
		if (hazard) j = rand_int(Setup.max_race);
		else j = (islower(c) ? A2I(c) : -1);
		
		if (c == '#') {
			if (valid_dna && (dna_race >= 0 && dna_race < Setup.max_race)) j = dna_race;
			else {
				valid_dna = 0;
				hazard = FALSE;
				auto_reincarnation = FALSE;
				continue;
			}
		} else if (c == '%' && valid_dna) {
			c = '#';
			hazard = TRUE;
			auto_reincarnation = TRUE;
			continue;
		}

		if ((j < Setup.max_race) && (j >= 0)) {
			rp_ptr = &race_info[j];
			if (!(rp_ptr->choice & BITS(class))) continue;

#ifdef CLASS_BEFORE_RACE
			/* ENABLE_DEATHKNIGHT */
			if (j == RACE_VAMPIRE && class == CLASS_PALADIN) {
				/* Unhack class 'Paladin' to 'Death Knight' */
				class = CLASS_DEATHKNIGHT;
				p_ptr->cp_ptr = &class_info[class];
				/* Redraw proper class name */
 #ifndef CLASS_BEFORE_RACE
				c_put_str(TERM_L_BLUE, "                    ", 7, CHAR_COL);
				c_put_str(TERM_L_BLUE, (char*)p_ptr->cp_ptr->title, 7, CHAR_COL);
 #else
				c_put_str(TERM_L_BLUE, "                    ", 5, CHAR_COL);
				c_put_str(TERM_L_BLUE, (char*)p_ptr->cp_ptr->title, 5, CHAR_COL);
 #endif
			}
			else {
				/* Need to redraw class name for non-Death Knights too, since they
				   could've been Death Knights and pressed BACKSPACE to pick another
				   race than Vampire.. */
 #ifndef CLASS_BEFORE_RACE
				c_put_str(TERM_L_BLUE, "                    ", 7, CHAR_COL);
				c_put_str(TERM_L_BLUE, (char*)class_info[class].title, 7, CHAR_COL);
 #else
				c_put_str(TERM_L_BLUE, "                    ", 5, CHAR_COL);
				c_put_str(TERM_L_BLUE, (char*)class_info[class].title, 5, CHAR_COL);
 #endif
			}
#endif

			race = j;
#ifndef CLASS_BEFORE_RACE
			c_put_str(TERM_L_BLUE, "                    ", 5, CHAR_COL);
			c_put_str(TERM_L_BLUE, (char*)rp_ptr->title, 5, CHAR_COL);
#else
			c_put_str(TERM_L_BLUE, "                    ", 6, CHAR_COL);
			c_put_str(TERM_L_BLUE, (char*)rp_ptr->title, 6, CHAR_COL);
#endif

			//Clear any dna Draconian trait being displayed
			if (race != RACE_DRACONIAN) put_str("                                      ", 7, 1);

			break;
		} else if (c == '?') {
			/*do_cmd_help("help.hlp");*/
		} else bell();
	}

	clear_diz();
	clear_from(n);
	return TRUE;
}


static void display_trait_diz(int r) {
	int i;

	/* Transform visible index back to real index */
	for (i = 0; i <= r; i++)
		if (!(trait_info[i].choice & BITS(race))) r++;

	i = 0;
	clear_diz();
	if (!trait_diz[r][i]) return; /* server !newer_than 4.5.1.2 */

//	c_put_str(TERM_UMBER, format("--- %s ---", trait_info[r].title), DIZ_ROW, DIZ_COL);
	while (i < 12 && trait_diz[r][i][0]) {
		c_put_str(TERM_L_UMBER, trait_diz[r][i], DIZ_ROW + i, DIZ_COL);
		i++;
	}
}

/*
 * Allows player to select a racial trait (introduced for Draconians) - C. Blue
 */
static bool choose_trait(void) {
	player_trait *tp_ptr;

	int i, j, l, m, n, sel = 0, sel_offset, shown_traits = 0;
	char c = '\0';
	char out_val[160];
	bool hazard = FALSE;

	/* Assume traits are N/A: */
	trait = -1;
	tp_ptr = &trait_info[0];


	/* Absolutely no traits, not even #0 'N/A' */
	if (Setup.max_trait == 0) return TRUE;

#ifdef HIDE_UNAVAILABLE_TRAIT
	/* If server doesn't support traits, or we only have the
	   dummy 'N/A' trait available in general, skip trait choice */
	if (Setup.max_trait == 1) return TRUE;
#endif

	/* Prepare to list, skip trait #0 'N/A' */
	for (j = 1; j < Setup.max_trait; j++) {
		tp_ptr = &trait_info[j];
		if (!(tp_ptr->choice & BITS(race))) continue;
		shown_traits++;
	}
	/* No traits available? */
	if (shown_traits == 0) {
		/* Paranoia - server bug (not even 'N/A' "available") */
		if (!(trait_info[0].choice & BITS(race))) return TRUE;
	}

	/* No traits available for this race (except for 'N/A')? Skip forward then.
	   Note: trait #0 is "N/A", which only traitless classes are supposed to 'have'. */
	if (trait_info[0].choice & BITS(race)) {
#ifndef HIDE_UNAVAILABLE_TRAIT
 #ifdef CLASS_BEFORE_RACE
		put_str("Trait :                               ", 7, 1);
		c_put_str(TERM_L_BLUE, trait_info[0].title, 7, CHAR_COL);
 #else
		put_str("Trait :                               ", 6, 1);
		c_put_str(TERM_L_BLUE, trait_info[0].title, 6, CHAR_COL);
 #endif
#endif
		return TRUE;
	}

	/* If we have traits available for the race chose, prepare to display them */
#ifdef CLASS_BEFORE_RACE
	put_str("Trait :                               ", 7, 1);
	if (valid_dna && (dna_trait > 0 && dna_trait < Setup.max_trait)) c_put_str(TERM_SLATE, trait_info[dna_trait].title, 7, CHAR_COL);
#else
	put_str("Trait :                               ", 6, 1);
	if (valid_dna && (dna_trait > 0 && dna_trait < Setup.max_trait)) c_put_str(TERM_SLATE, trait_info[dna_trait].title, 6, CHAR_COL);
#endif

	for (i = 18; i < 24; i++) Term_erase(1, i, 255);

	/* Traits are available */
	trait = 0;

trait_redraw:
	l = 2;
	m = 22 - (shown_traits - 1) / 3;
	n = m - 1;

	/* Display the legal choices */
	i = 0;
	sel_offset = 0;
	for (j = 0; j < Setup.max_trait; j++) {
		tp_ptr = &trait_info[j];
		if (!(tp_ptr->choice & BITS(race))) {
			sel_offset++;
			continue;
		}

		sprintf(out_val, "%c) %s", I2A(i), tp_ptr->title);
		if (j == sel + sel_offset) c_put_str(TERM_YELLOW, out_val, m, l);
		else c_put_str(TERM_WHITE, out_val, m, l);
		i++;

		l += 25;
		if (l > 70) {
			l = 2;
			m++;
		}
	}

	if (auto_reincarnation) {
		hazard = TRUE;
		c = '#';
	}

#ifdef ATMOSPHERIC_INTRO
	DRAW_TORCH
#endif

	/* Get a trait */
	while (1) {
		if (valid_dna) c_put_str(TERM_SLATE, "Choose a trait (* random, \377B#\377s/\377B%\377s reincarnate, Q quit, BACKSPACE back, 2/4/6/8): ", n, 2);
		else c_put_str(TERM_SLATE, "Choose a trait (* for random, Q to quit, BACKSPACE to go back, 2/4/6/8):  ", n, 2);
		display_trait_diz(sel);

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		if (!hazard) c = inkey();

		if (c == 'Q' || c == KTRL('Q')) quit(NULL);
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) return FALSE;
#endif
		if (c == '\b') {
			clear_diz();
			clear_from(n);
#ifdef CLASS_BEFORE_RACE
			put_str("                                            ", 7, 1);
			if (valid_dna && (dna_trait > 0 && dna_trait < Setup.max_trait)) {
				put_str("Trait :                               ", 7, 1);
				c_put_str(TERM_SLATE, trait_info[dna_trait].title, 7, CHAR_COL);
			}
#else
			put_str("                                            ", 6, 1);
			if (valid_dna && (dna_trait > 0 && dna_trait < Setup.max_trait)) {
				put_str("Trait :                               ", 6, 1);
				c_put_str(TERM_SLATE, trait_info[dna_trait].title, 6, CHAR_COL);
			}
#endif
			return FALSE;
		}

		/* Allow 'navigating', to highlight and display the descriptive text */
		if (c == '4' || c == '-') {//j
			sel = (shown_traits + sel - 1) % shown_traits;
			goto trait_redraw;
		}
		if (c == '6' || c == '+') {//k
			sel = (sel + 1) % shown_traits;
			goto trait_redraw;
		}
		if (c == '8' || c == '<') {//h
			if (sel - 3 < 0) sel = sel + ((shown_traits - 1 - sel) / 3) * 3;
			else sel -= 3;
			goto trait_redraw;
		}
		if (c == '2' || c == '>') {//l
			if (sel + 3 >= shown_traits) sel = sel % 3;
			else sel += 3;
			goto trait_redraw;
		}
		if (c == '\r' || c == '\n') c = 'a' + sel;

		if (c == '*') hazard = TRUE;
		if (hazard) j = rand_int(shown_traits);
		else j = (islower(c) ? A2I(c) : -1);

		if (c == '#') {
			if (valid_dna && (dna_trait > 0 && dna_trait < Setup.max_trait)) j = dna_trait;
			else {
				valid_dna = 0;
				hazard = FALSE;
				auto_reincarnation = FALSE;
				continue;
			}
		} else if (c == '%') {
			hazard = TRUE;
			c = '#';
			auto_reincarnation = TRUE;
			continue;
		} else {
			/* Transform visible index back to real index */
			for (i = 0; i <= j; i++)
				if (!(trait_info[i].choice & BITS(race))) j++;
		}

		/* Paranoia */
		if (j > shown_traits) continue;

		/* Verify if legal */
		if ((j < Setup.max_trait) && (j >= 0)) {
			tp_ptr = &trait_info[j];
			if (!(tp_ptr->choice & BITS(race))) continue;

			trait = j;
			break;
		} else if (c == '?') {
			/*do_cmd_help("help.hlp");*/
		} else {
			bell();
		}
	}

	clear_diz();
	/* hack: Draw this _after_ clear_diz(), because lineage
	   titles are too long -_- */
#ifndef CLASS_BEFORE_RACE
	put_str("Trait :                               ", 6, 1);
	c_put_str(TERM_L_BLUE, (char*)trait_info[trait].title, 6, CHAR_COL);
#else
	put_str("Trait :                               ", 7, 1);
	c_put_str(TERM_L_BLUE, (char*)trait_info[trait].title, 7, CHAR_COL);
#endif
	clear_from(n);

	return TRUE;
}


static void display_class_diz(int r) {
	int i = 0;

	clear_diz();
	if (!class_diz[r][i]) return; /* server !newer_than 4.5.1.2 */

	//c_put_str(TERM_UMBER, format("--- %s ---", class_info[r].title), DIZ_ROW, DIZ_COL);
	while (i < 12 && class_diz[r][i][0]) {
		c_put_str(TERM_L_UMBER, class_diz[r][i], DIZ_ROW + i, DIZ_COL);
		i++;
	}
}

/*
 * Gets a character class				-JWT-
 */
static bool choose_class(void) {
	player_class *cp_ptr;
#ifndef CLASS_BEFORE_RACE
	player_race *rp_ptr = &race_info[race];
	bool sel_ok = FALSE;
#endif
	int i, j = 0, l, m, n, sel = 0, hidden = 0, cmap[MAX_CLASS + 16]; //+16 for future-proofing reserve..
	char c = '\0';
	char out_val[160];
	bool hazard = FALSE;

	/* ENABLE_DEATHKNIGHT/ENABLE_HELLKNIGHT/ENABLE_CPRIEST duplicate slot hack (Paladin vs Death Knight/Hell Knight vs Corrupted Priest) */
	if (!is_newer_than(&server_version, 4, 7, 0, 2, 0, 0)) {
		//old server: hardcoded
		if (Setup.max_class >= 14) {
			hidden = Setup.max_class - 13;
		}
	} else {
		for (i = 0; i < Setup.max_class; i++) {
			if (class_info[i].hidden) hidden++;
			else {
				cmap[j] = i;
				j++;
			}
		}
	}
	Setup.max_class -= hidden;

	/* Prepare to list */
	l = 2;
	m = 22 - (Setup.max_class - 1) / 5;
	n = m - 1;

	c_put_str(TERM_SLATE, "Important - These classes are NOT RECOMMENDED for beginners:", n - 3, 2);
	//c_put_str(TERM_SLATE, "Important: For beginners, classes easy to play are:", n - 3, 2);
	c_put_str(TERM_ORANGE, "Important", n - 3, 2);
	c_put_str(TERM_SLATE, "Istar, Priest, Archer, Ranger, Adventurer, Shaman, Runemaster.", n - 2, 2);
	//c_put_str(TERM_SLATE, "Warrior, Rogue, Paladin, Druid, and maybe Archer.", n - 2, 2);

	for (i = 18; i < 24; i++) Term_erase(1, i, 255);

class_redraw:
	l = 2;
	m = 22 - (Setup.max_class - 1) / 5;
	n = m - 1;

	/* Display the legal choices */
	for (j = 0; j < Setup.max_class; j++) {
		cp_ptr = &class_info[cmap[j]];
		sprintf(out_val, "%c) %s", I2A(j), cp_ptr->title);

#ifndef CLASS_BEFORE_RACE
		if (!(rp_ptr->choice & BITS(cmap[j]))) {
			c_put_str(TERM_L_DARK, out_val, m, l);
			if (!sel_ok) sel++;
		} else
#endif
		{
#ifndef CLASS_BEFORE_RACE
			sel_ok = TRUE;
#endif
			if (j == sel) c_put_str(TERM_YELLOW, out_val, m, l);
			else c_put_str(TERM_WHITE, out_val, m, l);
		}

		l += 15;
		if (l > 70) {
			l = 2;
			m++;
		}
	}
#ifndef CLASS_BEFORE_RACE
	c_put_str(TERM_L_BLUE, "                    ", 7, CHAR_COL);
	if (valid_dna && (dna_class >= 0 && dna_class < Setup.max_class + hidden)) c_put_str(TERM_SLATE, dna_class_title, 7, CHAR_COL);
#else
	c_put_str(TERM_L_BLUE, "                    ", 5, CHAR_COL);
	if (valid_dna && (dna_class >= 0 && dna_class < Setup.max_class + hidden)) c_put_str(TERM_SLATE, dna_class_title, 5, CHAR_COL);
#endif

	if (auto_reincarnation) {
		hazard = TRUE;
		c = '#';
	}

#ifdef ATMOSPHERIC_INTRO
	DRAW_TORCH
#endif

	/* Get a class */
	while (1) {
		if (valid_dna) c_put_str(TERM_SLATE, "Choose a class (* random, \377B#\377s/\377B%\377s reincarnate, Q quit, BACKSPACE back, 2/4/6/8):  ", n, 2);
		else c_put_str(TERM_SLATE, "Choose a class (* for random, Q to quit, BACKSPACE to go back, 2/4/6/8):  ", n, 2);
		display_class_diz(sel);

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		if (!hazard) c = inkey();

		if (c == 'Q' || c == KTRL('Q')) quit(NULL);
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) return FALSE;
#endif
		if (c == '\b') {
			clear_diz();
			clear_from(n - 3);
			return FALSE;
		}

		/* Allow 'navigating', to highlight and display the descriptive text */
		if (c == '4' || c == '-') {//j
			sel = (Setup.max_class + sel - 1) % Setup.max_class;
#ifndef CLASS_BEFORE_RACE
			while (!(race_info[race].choice & BITS(sel)))
			    sel = (Setup.max_class + sel - 1) % Setup.max_class;
#endif
			goto class_redraw;
		}
		if (c == '6' || c == '+') {//k
			sel = (sel + 1) % Setup.max_class;
#ifndef CLASS_BEFORE_RACE
			while (!(race_info[race].choice & BITS(sel)))
			    sel = (sel + 1) % Setup.max_class;
#endif
			goto class_redraw;
		}
		if (c == '8' || c == '<') {//h
			//note that this going in jumps only really works ifdef CLASS_BEFORE_RACE -_-
			if (sel - 5 < 0) {
#if 0 /*later*/
				//also go back to previus "column"
				if (sel > 0) sel--;
				else {
					//or wrap around completely
					sel = ((Setup.max_class / 5) + 1) * 5 - 1;
					while (sel > Setup.max_class) sel -= 5;
					goto class_redraw;
				}
#endif
				sel = sel + ((Setup.max_class - 1 - sel) / 5) * 5;
			}
			else sel -= 5;
#ifndef CLASS_BEFORE_RACE
			while (!(race_info[race].choice & BITS(sel))) {
				if (sel - 5 < 0) sel = sel + ((Setup.max_class - 1 - sel) / 5) * 5;
				else sel -= 5;
			}
#endif
			goto class_redraw;
		}
		if (c == '2' || c == '>') {//l
			//note that this going in jumps only really works ifdef CLASS_BEFORE_RACE -_-
			if (sel + 5 >= Setup.max_class) {
				sel = sel % 5;
#if 0 /*later*/
				//also advance to next "column"
				if (sel < 4) sel++;
				else sel = 0; //or wrap around completely
#endif
			}
			else sel += 5;
#ifndef CLASS_BEFORE_RACE
			while (!(race_info[race].choice & BITS(sel))) {
				if (sel + 5 >= Setup.max_class) sel = sel % 5;
				else sel += 5;
			}
#endif
			goto class_redraw;
		}
		if (c == '\r' || c == '\n') c = 'a' + sel;

		if (c == '*') hazard = TRUE;
		if (hazard) j = rand_int(Setup.max_class);
		else j = (islower(c) ? A2I(c) : -1);
		
		if (c == '#') {
			if (valid_dna && (dna_class >= 0 && dna_class < Setup.max_class + hidden)) {
#if 0
				j = dna_class;
#else
				for (i = 0; i < Setup.max_class; i++) {
					if (cmap[i] != dna_class) continue;
					j = i;
					break;
				}
				if (i == Setup.max_class) {
					valid_dna = 0;
					hazard = FALSE,
					auto_reincarnation = FALSE;
					continue;
				}
#endif
			} else {
				valid_dna = 0;
				hazard = FALSE;
				auto_reincarnation = FALSE;
				continue;
			}
		} else if (c == '%') {
			auto_reincarnation = TRUE;
			c = '#';
			hazard = TRUE;
			continue;
		}

		if ((j < Setup.max_class) && (j >= 0)) {
#ifndef CLASS_BEFORE_RACE
			if (!(rp_ptr->choice & BITS(cmap[j]))) continue;
#endif

			class = cmap[j];
			cp_ptr = &class_info[class];
#ifndef CLASS_BEFORE_RACE
			c_put_str(TERM_L_BLUE, "                    ", 7, CHAR_COL);
			c_put_str(TERM_L_BLUE, (char*)cp_ptr->title, 7, CHAR_COL);
#else
			c_put_str(TERM_L_BLUE, "                    ", 5, CHAR_COL);
			c_put_str(TERM_L_BLUE, (char*)cp_ptr->title, 5, CHAR_COL);
#endif
			break;
		} else if (c == '?') {
			/*do_cmd_help("help.hlp");*/
		} else bell();
	}

	/* Unhack */
	Setup.max_class += hidden;

	clear_diz();
	clear_from(n - 3); /* -3 so beginner-warnings are also cleared */
	return TRUE;
}


/*
 * Get the desired stat order.
 */
static bool choose_stat_order(void) {
	int i, j, k, avail[6], crb, maxed_stats = 0;
	char c = '\0';
	char out_val[160], stats[6][4], buf[8], buf2[8], buf3[8];
	bool hazard = FALSE;
	s16b stat_order_tmp[6];

	player_class *cp_ptr = &class_info[class];
	player_race *rp_ptr = &race_info[race];

	for (i = 0; i < 6; i++) stat_order_tmp[i] = stat_order[i];


	/* Character stats are randomly rolled (1 time): */
	if (char_creation_flags == 0) {

		put_str("Stat order  :", 11, 1);

		/* All stats are initially available */
		for (i = 0; i < 6; i++) {
			strncpy(stats[i], stat_names[i], 3);
			stats[i][3] = '\0';
			avail[i] = 1;
		}

		/* Find the ordering of all 6 stats */
		for (i = 0; i < 6; i++) {
			/* Clear bottom of screen */
			clear_from(20);

			/* Print available stats at bottom */
			for (k = 0; k < 6; k++) {
				/* Check for availability */
				if (avail[k]) {
					sprintf(out_val, "%c) %s", I2A(k), stats[k]);
					put_str(out_val, 21, k * 9);
				}
			}

			/* Hack -- it's obvious */
			/* if (i > 4) hazard = TRUE;
			It confused too many noobiez. Taking it out for now. */

#ifdef ATMOSPHERIC_INTRO
	DRAW_TORCH
#endif

			/* Get a stat */
			while (1) {
				put_str("Choose your stat order (* for random, Q to quit): ", 20, 2);
				if (hazard) {
					j = rand_int(6);
				} else {
					Term->scr->cx = Term->wid;
					Term->scr->cu = 1;

					c = inkey();
					if (c == 'Q' || c == KTRL('Q')) quit(NULL);
#ifdef RETRY_LOGIN
					if (rl_connection_destroyed) return FALSE;
#endif
					if (c == '*') hazard = TRUE;

					j = (islower(c) ? A2I(c) : -1);
				}

				if ((j < 6) && (j >= 0) && (avail[j])) {
					stat_order[i] = j;
					c_put_str(TERM_L_BLUE, stats[j], 8, 15 + i * 5);
					avail[j] = 0;
					break;
				} else if (c == '?') {
					/*do_cmd_help("help.hlp");*/
				} else bell();
			}
		}

		clear_from(20);
	}

	/* player can define his stats completely manually: */
	else if (char_creation_flags == 1) {
		int col1 = 3, col2 = 35, col3 = 54, tmp_stat, rowA = 12;

		j = 0; /* current stat to be modified */
		k = 30; /* free points left */

		clear_from(14);

		c_put_str(TERM_SLATE, "Distribute your attribute points (\377ouse them all!\377s):", rowA, col1);
		c_put_str(TERM_L_GREEN, format("%2d", k), rowA, col3);
		c_put_str(TERM_SLATE, "If any values are shown under 'recommended minimum', try to reach those.", rowA + 1, col1);
		if (valid_dna) c_put_str(TERM_SLATE, "Current:   (Base) (Prev) Recommended min:", 15, col2);
		else c_put_str(TERM_SLATE, "Current:   (Base)        Recommended min::", 15, col2);

		if (valid_dna) c_put_str(TERM_SLATE, "Press \377B#\377s/\377B%\377s to reincarnate.", 15, col1);
#if 0
		c_put_str(TERM_SLATE, "Use keys '+', '-', 'RETURN'", 16, col1);
		c_put_str(TERM_SLATE, "or 8/2/4/6 or arrow keys to", 17, col1);
		c_put_str(TERM_SLATE, "modify and navigate.", 18, col1);
		c_put_str(TERM_SLATE, "After you distributed all", 19, col1);
		c_put_str(TERM_SLATE, "points, press ESC to proceed.", 20, col1);
		c_put_str(TERM_SLATE, "'Q' = quit, BACKSPACE = back.", 21, col1);
#else
		c_put_str(TERM_WHITE, "RETURN\377s/\377w8\377s/\377w2\377s/\377wUp\377s/\377wDown\377s:", 16, col1);
		c_put_str(TERM_SLATE, " Select a stat.", 17, col1);
		c_put_str(TERM_WHITE, "-\377s/\377w+\377s/\377w4\377s/\377w6\377s/\377wLeft\377s/\377wRight\377s:", 18, col1);
		c_put_str(TERM_SLATE, " Modify the selected stat.", 19, col1);
		c_put_str(TERM_WHITE, "ESC\377s to proceed when done.", 20, col1);
		c_put_str(TERM_WHITE, "Q\377s quits, \377wBACKSPACE\377s goes back.", 21, col1);
#endif

		c_put_str(TERM_SLATE, "No more than 1 attribute out of the 6 is allowed to be maximised.", 23, col1);

		c_put_str(TERM_L_UMBER,"   - Strength -    ", DIZ_ROW, 30);
		c_put_str(TERM_YELLOW, "   How quickly you can strike.", DIZ_ROW + 1, 30);
		c_put_str(TERM_YELLOW, "   How much you can carry and wield.", DIZ_ROW + 2, 30);
		c_put_str(TERM_YELLOW, "   How much damage your strikes inflict.", DIZ_ROW + 3, 30);
		c_put_str(TERM_YELLOW, "   How easily you can bash, throw and dig.", DIZ_ROW + 4, 30);
		c_put_str(TERM_YELLOW, "   Slightly improves your swimming.", DIZ_ROW + 5, 30);

		for (i = 0; i < 6; i++) {
			stat_order[i] = 10;
			strncpy(stats[i], stat_names[i], 3);
			stats[i][3] = '\0';
			if (valid_dna) {
				if (dna_stat_order[i] > 7 || dna_stat_order[i] < 18) {
					sprintf(buf3, "(%2d)", dna_stat_order[i]);
					c_put_str(TERM_SLATE, buf3, 16 + i, col2 + 19);
				}
			}
		}

		if (auto_reincarnation) {
			c = '#';
		}

#ifdef ATMOSPHERIC_INTRO
	DRAW_TORCH
#endif

		while (1) {
			c_put_str(TERM_L_GREEN, format("%2d", k), rowA, col3);

			for (i = 0; i < 6; i++) {
				crb = stat_order[i] + cp_ptr->c_adj[i] + rp_ptr->r_adj[i];
				if (crb > 18) crb = 18 + (crb - 18) * 10;
				cnv_stat(crb, buf);
				sprintf(buf2, "%2d", stat_order[i]);
				sprintf(out_val, "%s: %s (%s)", stats[i], buf, buf2);

				tmp_stat = cp_ptr->min_recommend[i];
				if (tmp_stat >= 100) {
					/* indicate it's a main stat of this class */
					tmp_stat -= 100;
					c_put_str(TERM_GREEN, "(main)", 16 + i, col2 + 26 + 8);
				}
				if (tmp_stat) {
					if (tmp_stat > 18) tmp_stat = 18 + (tmp_stat - 18) * 10;
					cnv_stat(tmp_stat, buf);
					if (crb >= tmp_stat)
						c_put_str(TERM_L_GREEN, buf, 16 + i, col2 + 26);
					else
						c_put_str(TERM_GREEN, buf, 16 + i, col2 + 26);
				}

				if (j == i) {
					if (stat_order[i] == 10-2)
						c_put_str(TERM_L_RED, out_val, 16 + i, col2);
					else if (stat_order[i] == 17)
						c_put_str(TERM_L_BLUE, out_val, 16 + i, col2);
					else
						c_put_str(TERM_ORANGE, out_val, 16 + i, col2);
				} else {
					if (stat_order[i] == 10-2)
						c_put_str(TERM_RED, out_val, 16 + i, col2);
					else if (stat_order[i] == 17)
						c_put_str(TERM_VIOLET, out_val, 16 + i, col2);
					else
						c_put_str(TERM_L_UMBER, out_val, 16 + i, col2);
				}
			}

			Term->scr->cx = Term->wid;
			Term->scr->cu = 1;

			if (!auto_reincarnation) c = inkey();
			crb = cp_ptr->c_adj[j] + rp_ptr->r_adj[j];
			if (c == '-' || c == '4' || c == 'h') {
				if (stat_order[j] > 10-2 &&
				    /* exception: allow going below 3 if we initially were below 3 too */
				    (stat_order[j] > 10 || stat_order[j]+crb > 3)) {
					if (stat_order[j] <= 12) {
						/* intermediate */
						stat_order[j]--;
						k++;
					} else if (stat_order[j] <= 14) {
						/* high */
						stat_order[j]--;
						k += 2;
					} else if (stat_order[j] <= 16) {
						/* nearly max */
						stat_order[j]--;
						k += 3;
					} else {
						/* max! */
						stat_order[j]--;
						k += 4;
						maxed_stats--;
					}
				}
			}
			if (c == '+' || c == '6' || c == 'l') {
				if (stat_order[j] < 17) {
					if (stat_order[j] < 12 && k >= 1) {
						/* intermediate */
						stat_order[j]++;
						k--;
					} else if (stat_order[j] < 14 && k >= 2) {
						/* high */
						stat_order[j]++;
						k -= 2;
					} else if (stat_order[j] < 16 && k >= 3) {
						/* nearly max */
						stat_order[j]++;
						k -= 3;
					} else if (k >= 4 && !maxed_stats) { /* only 1 maxed stat is allowed */
						/* max! */
						stat_order[j]++;
						k -= 4;
						maxed_stats++;
					}
				}
			}
			if (c == '\r' || c == '2' || c == 'j') j = (j+1) % 6;
			if (c == '8' || c == 'k') j = (j+5) % 6;
			if (c == '\r' || c == '2' || c == '8' || c == 'j' || c == 'k') {
				switch (j) {
				case 0:	c_put_str(TERM_L_UMBER,"   - Strength -    ", DIZ_ROW, 30);
					c_put_str(TERM_YELLOW, "   How quickly you can strike.                  ", DIZ_ROW + 1, 30);
					c_put_str(TERM_YELLOW, "   How much you can carry and wear/wield.       ", DIZ_ROW + 2, 30);
					c_put_str(TERM_YELLOW, "   How much damage your strikes inflict.        ", DIZ_ROW + 3, 30);
					c_put_str(TERM_YELLOW, "   How easily you can bash, throw and dig.      ", DIZ_ROW + 4, 30);
					c_put_str(TERM_YELLOW, "   Slightly improves your swimming.             ", DIZ_ROW + 5, 30);
					c_put_str(TERM_YELLOW, "                                                ", DIZ_ROW + 6, 30);
					c_put_str(TERM_YELLOW, "                                                ", DIZ_ROW + 7, 30);
					c_put_str(TERM_YELLOW, "                                                ", DIZ_ROW + 8, 30);
					break;
				case 1:	c_put_str(TERM_L_UMBER,"   - Intelligence -", DIZ_ROW, 30);
					c_put_str(TERM_YELLOW, "   How well you can use magic                   ", DIZ_ROW + 1, 30);
					c_put_str(TERM_YELLOW, "    (depending on your class and spells).       ", DIZ_ROW + 2, 30);
					c_put_str(TERM_YELLOW, "   How well you can use magic devices.          ", DIZ_ROW + 3, 30);
					c_put_str(TERM_YELLOW, "   Helps your disarming ability.                ", DIZ_ROW + 4, 30);
					c_put_str(TERM_YELLOW, "   Helps noticing attempts to steal from you.   ", DIZ_ROW + 5, 30);
					c_put_str(TERM_YELLOW, "                                                ", DIZ_ROW + 6, 30);
					c_put_str(TERM_YELLOW, "                                                ", DIZ_ROW + 7, 30);
					c_put_str(TERM_YELLOW, "                                                ", DIZ_ROW + 8, 30);
					break;
				case 2:	c_put_str(TERM_L_UMBER,"   - Wisdom -      ", DIZ_ROW, 30);
					c_put_str(TERM_YELLOW, "   How well you can use prayers and magic       ", DIZ_ROW + 1, 30);
					c_put_str(TERM_YELLOW, "    (depending on your class and spells).       ", DIZ_ROW + 2, 30);
					c_put_str(TERM_YELLOW, "   How well can you resist malicious effects    ", DIZ_ROW + 3, 30);
					c_put_str(TERM_YELLOW, "    and influences on both body and mind        ", DIZ_ROW + 4, 30);
					c_put_str(TERM_YELLOW, "    (saving throw).                             ", DIZ_ROW + 5, 30);
					c_put_str(TERM_YELLOW, "   How much insanity your mind can take.        ", DIZ_ROW + 6, 30);
					c_put_str(TERM_YELLOW, "                                                ", DIZ_ROW + 7, 30);
					c_put_str(TERM_YELLOW, "                                                ", DIZ_ROW + 8, 30);
					break;
				case 3:	c_put_str(TERM_L_UMBER,"   - Dexterity -   ", DIZ_ROW, 30);
					c_put_str(TERM_YELLOW, "   How quickly you can strike.                  ", DIZ_ROW + 1, 30);
					c_put_str(TERM_YELLOW, "   Reduces your chance to miss.                 ", DIZ_ROW + 2, 30);
					c_put_str(TERM_YELLOW, "   Opponents will miss very slightly more often.", DIZ_ROW + 3, 30);
					c_put_str(TERM_YELLOW, "   Helps your stealing skills (if any).         ", DIZ_ROW + 4, 30);
					c_put_str(TERM_YELLOW, "   Helps to prevent foes stealing from you.     ", DIZ_ROW + 5, 30);
					c_put_str(TERM_YELLOW, "   Helps keeping your balance after bashing.    ", DIZ_ROW + 6, 30);
					c_put_str(TERM_YELLOW, "   Helps your disarming ability.                ", DIZ_ROW + 7, 30);
					c_put_str(TERM_YELLOW, "   Slightly improves your swimming.             ", DIZ_ROW + 8, 30);
					break;
				case 4:	c_put_str(TERM_L_UMBER,"   - Constitution -", DIZ_ROW, 30);
					c_put_str(TERM_YELLOW, "   Determines your amout of HP                  ", DIZ_ROW + 1, 30);
					c_put_str(TERM_YELLOW, "    (hit points, ie how much damage you can     ", DIZ_ROW + 2, 30);
					c_put_str(TERM_YELLOW, "    take without dying. High constitution might ", DIZ_ROW + 3, 30);
					c_put_str(TERM_YELLOW, "    not show much effect until your character   ", DIZ_ROW + 4, 30);
					c_put_str(TERM_YELLOW, "    also reaches an appropriate level.)         ", DIZ_ROW + 5, 30);
					c_put_str(TERM_YELLOW, "   Reduces duration of poison and stun effects. ", DIZ_ROW + 6, 30);
					c_put_str(TERM_YELLOW, "   Increases stamina regeneration rate.         ", DIZ_ROW + 7, 30);
					c_put_str(TERM_YELLOW, "   Helps your character not to drown too easily.", DIZ_ROW + 8, 30);
					break;
				case 5:	c_put_str(TERM_L_UMBER,"   - Charisma -    ", DIZ_ROW, 30);
					c_put_str(TERM_YELLOW, "   Shops will offer you wares at better prices. ", DIZ_ROW + 1, 30);
					c_put_str(TERM_YELLOW, "    (Note that shop keepers are also influenced ", DIZ_ROW + 2, 30);
					c_put_str(TERM_YELLOW, "    by your character's race.)                  ", DIZ_ROW + 3, 30);
					c_put_str(TERM_YELLOW, "   Helps you to resist seducing attacks.        ", DIZ_ROW + 4, 30);
					c_put_str(TERM_YELLOW, "                                                ", DIZ_ROW + 5, 30);
					c_put_str(TERM_YELLOW, "                                                ", DIZ_ROW + 6, 30);
					c_put_str(TERM_YELLOW, "                                                ", DIZ_ROW + 7, 30);
					c_put_str(TERM_YELLOW, "                                                ", DIZ_ROW + 8, 30);
					break;
				}
			}
			if (c == '\e') {
				if (k > 0) {
					c_put_str(TERM_NUKE, format("%2d", k), rowA, col3);
					if (get_check2(format("You still have %d stat points left to distribute! Really continue?", k), FALSE)) break;
					c_put_str(TERM_L_GREEN, format("%2d", k), rowA, col3);
				} else break;
			}
			if (c == 'Q' || c == KTRL('Q')) quit(NULL);
#ifdef RETRY_LOGIN
			if (rl_connection_destroyed) return FALSE;
#endif
			if (c == '\b') {
				for (i = 0; i < 6; i++) stat_order[i] = stat_order_tmp[i];

				for (i = 3; i < 12; i++) Term_erase(30, i, 255);
				clear_from(rowA);

				return FALSE;
			}
			if (c == '#') {
				if (valid_dna) {
					for (i = 0; i < 6; i++) {
						if (dna_stat_order[i] > 7 || dna_stat_order[i] < 18) stat_order[i] = dna_stat_order[i];
						else stat_order[i] = 8;
					}
					for (i = 3; i < 12; i++) Term_erase(30, i, 255);
					clear_from(rowA);
					return TRUE;
				} else {
					auto_reincarnation = FALSE;
					hazard = FALSE;
					continue;
				}
			}
			if (c == '%') {
				auto_reincarnation = TRUE;
				c = '#';
				continue;
			}
		}

		for (i = 3; i < 12; i++) Term_erase(30, i, 255);
		clear_from(rowA);
	}
	return TRUE;
}

/* Quick hack!		- Jir -
 * TODO: remove hard-coded things. */
static bool choose_mode(void) {
	char c = '\0';
	bool hazard = FALSE;

	/* specialty: slot-exclusive-mode char? */
	if (dedicated) {
		put_str("i) Ironman Deep Dive Challenge", 16, 2);
		c_put_str(TERM_SLATE, "(Unworldly - one life only.)", 16, 33);
		put_str("H) Hellish Ironman Deep Dive Challenge", 17, 2);
		c_put_str(TERM_SLATE, "(Extra hard, sort of ridiculous)", 17, 41);
		put_str("p) PvP", 18, 2);
		c_put_str(TERM_SLATE, "(Can't beat the game, instead special 'player vs player' rules apply)", 18, 9);

		c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
		if (valid_dna) {
			if (((dna_sex & MODE_HARD) == MODE_HARD) && ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST)) c_put_str(TERM_SLATE, "Hellish", 9, CHAR_COL);
			else if ((dna_sex & MODE_HARD) == MODE_HARD) c_put_str(TERM_SLATE, "Hard", 9, CHAR_COL);
			else if ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) c_put_str(TERM_SLATE, "No Ghost", 9, CHAR_COL);
			else if ((dna_sex & MODE_EVERLASTING) == MODE_EVERLASTING) c_put_str(TERM_SLATE, "Everlasting", 9, CHAR_COL);
			else if ((dna_sex & MODE_PVP) == MODE_PVP) c_put_str(TERM_SLATE, "PvP", 9, CHAR_COL);
			else c_put_str(TERM_SLATE, "Normal", 9, CHAR_COL);
		}

		if (auto_reincarnation) {
			hazard = TRUE;
			c = '#';
		}

#ifdef ATMOSPHERIC_INTRO
	DRAW_TORCH
#endif

		while (1) {
			if (valid_dna) c_put_str(TERM_SLATE, "Choose a mode (* random, \377B#\377s/\377B%\377s reincarnate, Q quit, BACKSPACE back): ", 15, 2);
			else c_put_str(TERM_SLATE, "Choose a mode (* for random, Q to quit, BACKSPACE to go back): ", 15, 2);

			Term->scr->cx = Term->wid;
			Term->scr->cu = 1;

			if (!hazard) c = inkey();

			if (c == 'Q' || c == KTRL('Q')) quit(NULL);
#ifdef RETRY_LOGIN
			if (rl_connection_destroyed) return FALSE;
#endif
			if (c == '\b') {
				clear_from(15);
				return FALSE;
			}
			else if (c == 'p') {
				sex += MODE_PVP;
				c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
				c_put_str(TERM_L_BLUE, "PvP", 9, CHAR_COL);
				break;
			} else if (c == 'i') {
				sex += MODE_NO_GHOST;
				c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
				c_put_str(TERM_L_BLUE, "No Ghost", 9, CHAR_COL);
				break;
			}
			else if (c == 'H') {
				sex += (MODE_NO_GHOST + MODE_HARD);
				c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
				c_put_str(TERM_L_BLUE, "Hellish", 9, CHAR_COL);
				break;
			} else if (c == '?') {
				/*do_cmd_help("help.hlp");*/
			} else if (c == '*') {
				switch (rand_int(3 - 1)) {
					case 0:
						c = 'i';
						break;
					case 1:
						c = 'p';
						break;
					case 2:
						c = 'H';
						break;
				}
				hazard = TRUE;
			} else if (c == '#') {
				if (valid_dna) {
					if (((dna_sex & MODE_HARD) == MODE_HARD) && ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST)) c = 'H';
					else if ((dna_sex & MODE_HARD) == MODE_HARD) c = 'H';
					else if ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) c = 'i';
					else if ((dna_sex & MODE_EVERLASTING) == MODE_EVERLASTING) c = 'i';
					else if ((dna_sex & MODE_PVP) == MODE_PVP) c = 'p';
					else c = 'i';
					hazard = TRUE;
				} else {
					auto_reincarnation = FALSE;
					hazard = FALSE;
					continue;
				}
			} else if (c == '%') {
				auto_reincarnation = TRUE;
				hazard = TRUE;
				c = '#';
			} else bell();
		}

		clear_from(15);
		return TRUE;
	}

	put_str("n) Normal", 16, 2);
	c_put_str(TERM_SLATE, "(3 lifes)", 16, 12);
	put_str("g) No Ghost", 17, 2);
	c_put_str(TERM_SLATE, "('Unworldly' - One life only. The traditional rogue-like way)", 17, 14);
	put_str("e) Everlasting", 18, 2);
	c_put_str(TERM_SLATE, "(You may resurrect infinite times, but cannot enter highscore)", 18, 17);
	/* hack: no weird modi on first client startup!
	   To find out whether it's 1st or not we check bigmap_hint and # of existing characters.
	   However, we just don't display the choices, they're still choosable! */
	if (!bigmap_hint || existing_characters) {
#if 0
		put_str("h) Hard", 19, 2);
		c_put_str(TERM_SLATE, "('Purgatorial' - like normal, with nasty additional penalties)", 19, 10);
#endif
		put_str("H) Hellish", 20 - 1, 2);
		c_put_str(TERM_SLATE, "(Like 'unworldly' mode, but extra hard - sort of ridiculous)", 20 - 1, 13);
		put_str("p) PvP", 21 - 1, 2);
		c_put_str(TERM_SLATE, "(Can't beat the game, instead special 'player vs player' rules apply)", 21 - 1, 9);
	}

	c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
	if (valid_dna) {
		if (((dna_sex & MODE_HARD) == MODE_HARD) && ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST)) c_put_str(TERM_SLATE, "Hellish", 9, CHAR_COL);
		else if ((dna_sex & MODE_HARD) == MODE_HARD) c_put_str(TERM_SLATE, "Hard", 9, CHAR_COL);
		else if ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) c_put_str(TERM_SLATE, "No Ghost", 9, CHAR_COL);
		else if ((dna_sex & MODE_EVERLASTING) == MODE_EVERLASTING) c_put_str(TERM_SLATE, "Everlasting", 9, CHAR_COL);
		else if ((dna_sex & MODE_PVP) == MODE_PVP) c_put_str(TERM_SLATE, "PvP", 9, CHAR_COL);
		else c_put_str(TERM_SLATE, "Normal", 9, CHAR_COL);
	}

	if (auto_reincarnation) {
		hazard = TRUE;
		c = '#';
	}

#ifdef ATMOSPHERIC_INTRO
	DRAW_TORCH
#endif

	while (1) {
		if (valid_dna) c_put_str(TERM_SLATE, "Choose a mode (* random, \377B#\377s/\377B%\377s reincarnate, Q quit, BACKSPACE back): ", 15, 2);
		else c_put_str(TERM_SLATE, "Choose a mode (* for random, Q to quit, BACKSPACE to go back): ", 15, 2);

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		if (!hazard) c = inkey();

		if (c == 'Q' || c == KTRL('Q')) quit(NULL);
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) return FALSE;
#endif
		if (c == '\b') {
			clear_from(15);
			return FALSE;
		}
		else if (c == 'p') {
			sex += MODE_PVP;
			c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
			c_put_str(TERM_L_BLUE, "PvP", 9, CHAR_COL);
			break;
		} else if (c == 'n') {
			c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
			c_put_str(TERM_L_BLUE, "Normal", 9, CHAR_COL);
			break;
		} else if (c == 'g') {
			sex += MODE_NO_GHOST;
			c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
			c_put_str(TERM_L_BLUE, "No Ghost", 9, CHAR_COL);
			break;
		}
#if 0
		else if (c == 'h') {
			sex += (MODE_HARD);
			c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
			c_put_str(TERM_L_BLUE, "Hard", 9, CHAR_COL);
			break;
		}
#endif
		else if (c == 'H') {
			sex += (MODE_NO_GHOST + MODE_HARD);
			c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
			c_put_str(TERM_L_BLUE, "Hellish", 9, CHAR_COL);
			break;
		} else if (c == 'e') {
			sex += MODE_EVERLASTING;
			c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
			c_put_str(TERM_L_BLUE, "Everlasting", 9, CHAR_COL);
			break;
		} else if (c == '?') {
			/*do_cmd_help("help.hlp");*/
		} else if (c == '*') {
			switch (rand_int(5 - 1)) {
				case 0:
					c = 'n';
					break;
				case 1:
					c = 'p';
					break;
				case 2:
					c = 'g';
					break;
#if 0
				case 3:
					c = 'h';
					break;
#endif
				case 4 - 1:
					c = 'H';
					break;
				case 5 - 1:
					c = 'e';
					break;
			}
			hazard = TRUE;
		} else if (c == '#') {
			if (valid_dna) {
				if (((dna_sex & MODE_HARD) == MODE_HARD) && ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST)) c = 'H';
				else if ((dna_sex & MODE_HARD) == MODE_HARD) c = 'h';
				else if ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) c = 'g';
				else if ((dna_sex & MODE_EVERLASTING) == MODE_EVERLASTING) c = 'e';
				else if ((dna_sex & MODE_PVP) == MODE_PVP) c = 'p';
				else c = 'n';
				hazard = TRUE;
			} else {
				auto_reincarnation = FALSE;
				hazard = FALSE;
				continue;
			}
		} else if (c == '%') {
			auto_reincarnation = TRUE;
			hazard = TRUE;
			c = '#';
			continue;
		} else bell();
	}

	clear_from(15);
	return TRUE;
}

/* Fruit bat is now a "body modification" that can be applied to all "modes" - C. Blue */
static bool choose_body_modification(void) {
	char c = '\0';
	bool hazard = FALSE;

	/* normal mode is the default */
	sex &= ~MODE_FRUIT_BAT;

	put_str("n) Normal body", 20, 2);
	/* hack: no weird modi on first client startup!
	   To find out whether it's 1st or not we check bigmap_hint and # of existing characters.
	   However, we just don't display the choices, they're still choosable! */
	if (!bigmap_hint || existing_characters) {
		put_str("f) Fruit bat", 21, 2);
		if (class == CLASS_MIMIC || class == CLASS_DRUID || class == CLASS_SHAMAN)
			c_put_str(TERM_SLATE, "(not recommended for shapeshifters: Mimics, Druids, Shamans!)", 21, 15);
		else
			c_put_str(TERM_SLATE, "(Bats are faster and vampiric, but can't wear certain items)", 21, 15);
	}

	c_put_str(TERM_L_BLUE, "                    ", 8, CHAR_COL);
	if (valid_dna) {
		if ((dna_sex & MODE_FRUIT_BAT) == MODE_FRUIT_BAT) c_put_str(TERM_SLATE, "Fruit bat", 8, CHAR_COL);
		else c_put_str(TERM_SLATE, "Normal body", 8, CHAR_COL);
	}

	if (auto_reincarnation) {
		hazard = TRUE;
		c = '#';
	}

#ifdef ATMOSPHERIC_INTRO
	DRAW_TORCH
#endif

	while (1) {
		if (valid_dna) c_put_str(TERM_SLATE, "Choose body modification (* random, \377B#\377s/\377B%\377s reincarnate, Q quit, BACKSPACE back): ", 19, 2);
		else c_put_str(TERM_SLATE, "Choose body modification (* for random, Q to quit, BACKSPACE to go back): ", 19, 2);

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		if (!hazard) c = inkey();

		if (c == 'Q' || c == KTRL('Q')) quit(NULL);
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) return FALSE;
#endif
		if (c == '\b') {
			clear_from(19);
			return FALSE;
		}
		else if (c == 'f') {
			sex += MODE_FRUIT_BAT;
			c_put_str(TERM_L_BLUE, "                    ", 8, CHAR_COL);
			c_put_str(TERM_L_BLUE, "Fruit bat", 8, CHAR_COL);
			break;
		} else if (c == 'n') {
			c_put_str(TERM_L_BLUE, "                    ", 8, CHAR_COL);
			c_put_str(TERM_L_BLUE, "Normal body", 8, CHAR_COL);
			break;
		} else if (c == '?') {
			/*do_cmd_help("help.hlp");*/
		} else if (c == '*') {
			/* hack: no fruit bat archers */
			if (class == CLASS_ARCHER) c = 'n';
			else switch (rand_int(2)) {
				case 0:
					c = 'n';
					break;
				case 1:
					c = 'f';
					break;
			}
			hazard = TRUE;
		} else if (c == '#') {
			if (valid_dna) {
				if ((dna_sex & MODE_FRUIT_BAT) == MODE_FRUIT_BAT) c = 'f';
				else c = 'n';
				hazard = TRUE;
			} else {
				auto_reincarnation = FALSE;
				hazard = FALSE;
				continue;
			}
		} else if (c == '%') {
			auto_reincarnation = TRUE;
			hazard = TRUE;
			c = '#';
			continue;
		} else bell();
	}

	clear_from(19);
	return TRUE;
}

/*
 * Get the name/pass for this character.
 */
void get_char_name(void) {
	/* Clear screen */
	Term_clear();

#ifdef ATMOSPHERIC_INTRO
 #ifdef USE_SOUND_2010
	if (use_sound) {
		/* Play back a fitting, non-common music piece, abused as 'intro theme'.. */
		if (!music(exec_lua(0, "return get_music_index(\"title\")")))
			music(exec_lua(0, "return get_music_index(\"town_generic\")"));
		/* Play background ambient sound effect (if enabled) */
		sound_ambient(exec_lua(0, "return get_sound_index(\"ambient_fire\")"));
	}
 #endif
#endif

	/* Title everything */
#ifndef SIMPLE_LOGIN
	put_str("Name        :", 2, 1);
	put_str("Password    :", 3, 1);
	c_put_str(TERM_SLATE, "Make sure you enter letters in correct upper/lower case!", 5, 3);
#else
	put_str("Name        :", LOGIN_ROW + 4, 1);
	put_str("Password    :", LOGIN_ROW + 5, 1);
	c_put_str(TERM_SLATE, "Name and password are case-sensitive! Name always starts with upper-case.", LOGIN_ROW + 7, 2);
#endif
	/* Dump the default name */
#ifndef SIMPLE_LOGIN
	c_put_str(TERM_L_BLUE, nick, 2, 15);
#else
	c_put_str(TERM_L_BLUE, nick, LOGIN_ROW + 4, 15);
#endif


	/* Display some helpful information XXX XXX XXX */

	/* Enter both name and password */
	while (TRUE) {
		/* Choose a name */
		choose_name();
		/* Enter password */
		if (enter_password()) break;
	}

//#ifdef WINDOWS
	/* erase crecedentials? */
	if (!strlen(nick) || !strlen(pass))
		store_crecedentials();
//#endif

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

#ifdef ATMOSPHERIC_INTRO /* ascii-art in the top area */
	/* erase title, in case it gets half-way overwritten with a login-failure message (looks bad) */
	c_put_str(TERM_DARK, "                                                                                ", LOGO_ROW, 0);
	c_put_str(TERM_DARK, "                                                                                ", LOGO_ROW + 1, 0);
	c_put_str(TERM_DARK, "                                                                                ", LOGO_ROW + 2, 0);
	c_put_str(TERM_DARK, "                                                                                ", LOGO_ROW + 3, 0);
	c_put_str(TERM_DARK, "                                                                                ", LOGO_ROW + 4, 0);
	c_put_str(TERM_DARK, "                                                                                ", LOGO_ROW + 5, 0);
#endif
}

/*
 * Get the other info for this character.
 */
void get_char_info(void) {
	int i, j;
	char out_val[160];
	dedicated = sex & (MODE_DED_PVP | MODE_DED_IDDC);
	sex &= ~(MODE_DED_PVP | MODE_DED_IDDC);

	/* Hack -- display the nick */
	put_str("Name  :", 2, 1);
	c_put_str(TERM_WHITE, cname, 2, CHAR_COL);

	/* Load tables from LUA into memory --
	   could just request it from LUA on demand instead. - C. Blue */
	   //the +16 are just for some future-proofing, to avoid needing to update the client
	memset(race_diz, 0, sizeof(char) * (MAX_RACE + 16) * 12 * 61);
	memset(class_diz, 0, sizeof(char) * (MAX_CLASS + 16) * 12 * 61);
	memset(trait_diz, 0, sizeof(char) * (MAX_TRAIT + 16) * 12 * 61);
	if (is_newer_than(&server_version, 4, 5, 1, 1, 0, 0)) {
		for (j = 0; j < 12; j++) {
			for (i = 0; i < Setup.max_race; i++) {
				if (!race_info[i].title) continue;
				sprintf(out_val, "return get_race_diz(\"%s\", %d)", race_info[i].title, j);
				strcpy(race_diz[i][j], string_exec_lua(0, out_val));
			}
			for (i = 0; i < Setup.max_class; i++) {
				if (!class_info[i].title) continue;
				sprintf(out_val, "return get_class_diz(\"%s\", %d)", class_info[i].title, j);
				strcpy(class_diz[i][j], string_exec_lua(0, out_val));
			}
			for (i = 0; i < Setup.max_trait; i++) {
				if (!trait_info[i].title) continue;
				sprintf(out_val, "return get_trait_diz(\"%s\", %d)", trait_info[i].title, j);
				strcpy(trait_diz[i][j], string_exec_lua(0, out_val));
			}
		}
	}

	/* Did this character have a previous incarnation? */
	load_birth_file(cname); //valid_dna if we have a valid one

	/* Title everything */
	put_str("Sex   :", 4, 1);
	if (valid_dna) {
		if (dna_sex % 2) c_put_str(TERM_SLATE, "Male", 4, CHAR_COL);
		else c_put_str(TERM_SLATE, "Female", 4, CHAR_COL);
	}
#ifndef CLASS_BEFORE_RACE
	put_str("Race  :", 5, 1);
	if (valid_dna && (dna_race >= 0 && dna_race < Setup.max_race)) c_put_str(TERM_SLATE, race_info[dna_race].title, 5, CHAR_COL);
 #ifndef HIDE_UNAVAILABLE_TRAIT
	/* If server doesn't support traits, or we only have the dummy
	   'N/A' trait available, we don't want to display traits at all */
	put_str("Trait :                               ", 6, 1);
	if (valid_dna && (dna_trait > 0 && dna_trait < Setup.max_trait)) c_put_str(TERM_SLATE, trait_info[dna_trait].title, 6, CHAR_COL);
 #else
	//Show the trait if we previously selected draconian
	if (valid_dna && (dna_trait > 0 && dna_trait < Setup.max_trait)) {
		put_str("Trait :                               ", 6, 1);
		c_put_str(TERM_SLATE, trait_info[dna_trait].title, 6, CHAR_COL);
	}
 #endif
	put_str("Class :", 7, 1);
	if (valid_dna && (dna_class >= 0 && dna_class < Setup.max_class)) c_put_str(TERM_SLATE, dna_class_title, 7, CHAR_COL);
	put_str("Body  :", 8, 1);
	if (valid_dna) {
		if ((dna_sex & MODE_FRUIT_BAT) == MODE_FRUIT_BAT) c_put_str(TERM_SLATE, "Fruit bat", 8, CHAR_COL);
		else c_put_str(TERM_SLATE, "Normal body", 8, CHAR_COL);
	}
#else
	put_str("Class :", 5, 1);
	if (valid_dna && (dna_class >= 0 && dna_class < Setup.max_class)) c_put_str(TERM_SLATE, dna_class_title, 5, CHAR_COL);
	put_str("Race  :", 6, 1);
	if (valid_dna && (dna_race >= 0 && dna_race < Setup.max_race)) c_put_str(TERM_SLATE, race_info[dna_race].title, 6, CHAR_COL);
 #ifndef HIDE_UNAVAILABLE_TRAIT
	/* If server doesn't support traits, or we only have the dummy
	   'N/A' trait available, we don't want to display traits at all */
	put_str("Trait :                               ", 7, 1);
	if (valid_dna && (dna_trait > 0 && dna_trait < Setup.max_trait)) c_put_str(TERM_SLATE, trait_info[dna_trait].title, 7, CHAR_COL);
 #else
	//Show the trait if we previously selected draconian
	if (valid_dna && (dna_trait > 0 && dna_trait < Setup.max_trait)) {
		put_str("Trait :                               ", 7, 1);
		c_put_str(TERM_SLATE, trait_info[dna_trait].title, 7, CHAR_COL);
	}
 #endif
	put_str("Body  :", 8, 1);
	if (valid_dna) {
		if ((dna_sex & MODE_FRUIT_BAT) == MODE_FRUIT_BAT) c_put_str(TERM_SLATE, "Fruit bat", 8, CHAR_COL);
		else c_put_str(TERM_SLATE, "Normal body", 8, CHAR_COL);
	}
#endif
	put_str("Mode  :", 9, 1);
	if (valid_dna) {
		if (((dna_sex & MODE_HARD) == MODE_HARD) && ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST)) c_put_str(TERM_SLATE, "Hellish", 9, CHAR_COL);
		else if ((dna_sex & MODE_HARD) == MODE_HARD) c_put_str(TERM_SLATE, "Hard", 9, CHAR_COL);
		else if ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) c_put_str(TERM_SLATE, "No Ghost", 9, CHAR_COL);
		else if ((dna_sex & MODE_EVERLASTING) == MODE_EVERLASTING) c_put_str(TERM_SLATE, "Everlasting", 9, CHAR_COL);
		else if ((dna_sex & MODE_PVP) == MODE_PVP) c_put_str(TERM_SLATE, "PvP", 9, CHAR_COL);
		else c_put_str(TERM_SLATE, "Normal", 9, CHAR_COL);
	}
	/* Clear bottom of screen */
	clear_from(20);


#ifdef ATMOSPHERIC_INTRO
	DRAW_TORCH
#endif


	/* Display some helpful information XXX XXX XXX */

csex:
#ifdef RETRY_LOGIN
	if (rl_connection_destroyed) return;
#endif
	/* Choose a sex */
	choose_sex();

#if 0
	/* Clean up the selections */
	Term_erase(15, 5, 255);
	Term_erase(15, 6, 255);
	Term_erase(15, 7, 255);
#endif

#ifndef CLASS_BEFORE_RACE
crace:
 #ifdef RETRY_LOGIN
	if (rl_connection_destroyed) return;
 #endif
	/* Choose a race */
	//Show the trait if we previously selected draconian
	if (valid_dna && (dna_trait > 0 && dna_trait < Setup.max_trait)) {
		put_str("Trait :                               ", 6, 1);
		c_put_str(TERM_SLATE, trait_info[dna_trait].title, 6, CHAR_COL);
	} else put_str("             ", 6, 1);
	if (!choose_race()) goto csex;
ctrait:
 #ifdef RETRY_LOGIN
	if (rl_connection_destroyed) return;
 #endif
	/* Choose a trait */
	if (!choose_trait()) goto crace;
cbody:
 #ifdef RETRY_LOGIN
	if (rl_connection_destroyed) return;
 #endif
	/* Choose character's body modification */
	if (!choose_body_modification()) {
		if (trait == -1) goto crace;
		else goto ctrait;
	}
cclass:
 #ifdef RETRY_LOGIN
	if (rl_connection_destroyed) return;
 #endif
	/* Choose a class */
	if (!choose_class()) goto cbody;


cstats:
 #ifdef RETRY_LOGIN
	if (rl_connection_destroyed) return;
 #endif
	class_extra = 0;

	/* Choose stat order */
	if (!choose_stat_order()) goto cclass;
#else
cclass:
 #ifdef RETRY_LOGIN
	if (rl_connection_destroyed) return;
 #endif
	/* Choose a class */
	if (!choose_class()) goto csex;
crace:
 #ifdef RETRY_LOGIN
	if (rl_connection_destroyed) return;
 #endif
	/* Choose a race */
	//Show the trait if we previously selected draconian
	if (valid_dna && (dna_trait > 0 && dna_trait < Setup.max_trait)) {
		put_str("Trait :                               ", 7, 1);
		c_put_str(TERM_SLATE, trait_info[dna_trait].title, 7, CHAR_COL);
	} else put_str("             ", 7, 1);
	if (!choose_race()) goto cclass;
ctrait:
 #ifdef RETRY_LOGIN
	if (rl_connection_destroyed) return;
 #endif
	/* Choose a trait */
	if (!choose_trait()) goto crace;
cbody:
 #ifdef RETRY_LOGIN
	if (rl_connection_destroyed) return;
 #endif
	/* Choose character's body modification */
	if (!choose_body_modification()) {
		if (trait == -1) goto crace;
		else goto ctrait;
	}


cstats:
 #ifdef RETRY_LOGIN
	if (rl_connection_destroyed) return;
 #endif
	class_extra = 0;

	/* Choose stat order */
	if (!choose_stat_order()) goto cbody;
#endif


#ifdef RETRY_LOGIN
	if (rl_connection_destroyed) return;
#endif
	/* Fix trait hack: '-1' meant "no traits available" */
	if (trait == -1) trait = 0;


	/* Choose character mode */
	if (!s_RPG || s_RPG_ADMIN) {
		if (!choose_mode()) goto cstats;
	} else {
		c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
		c_put_str(TERM_L_BLUE, "No Ghost", 9, CHAR_COL);
	}
#ifdef RETRY_LOGIN
	if (rl_connection_destroyed) return;
#endif


#if 0 //why?
	/* Clear */
	clear_from(15);
#endif

#ifdef ATMOSPHERIC_INTRO
	DRAW_TORCH
#endif

	/* Hack: Apply slot-exclusive mode on user demand */
	if (dedicated) {
		if (sex & MODE_PVP) sex |= MODE_DED_PVP;
		else sex |= MODE_DED_IDDC;
	}

	/* Save Birth DNA */
	save_birth_file(cname);

	/* Message */
	put_str("Entering game...  [Hit any key]", 21, 1);

	/* Wait for key */
	inkey();

	/* Clear */
	clear_from(20);

#ifdef ATMOSPHERIC_INTRO
 #ifdef USE_SOUND_2010
  #if 0 /* don't stop the sfx, since we'll end up in Bree's inn anyway, which has fireplace sfx too! */
	/* Stop background ambient sound effect (if enabled) */
	if (use_sound) sound_ambient(-1);
  #endif
 #endif
#endif
}

static bool enter_server_name(void) {
	/* Clear screen */
	Term_clear();

	/* Message */
	prt("Enter the server name you want to connect to (ESCAPE to quit): ", 3, 1);

	/* Move cursor */
	move_cursor(5, 1);

	/* Default */
	strcpy(server_name, "europe.tomenet.eu");

	/* Ask for server name */
	return askfor_aux(server_name, 80, 0);
}

/*
 * Have the player choose a server from the list given by the
 * metaserver.
 */
/* TODO:
 * - wrap the words using strtok
 * - allow to scroll the screen in case
 * */

bool get_server_name(void) {
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
		prt("Failed to connect to used-specified meta server.", 2, 1);
		prt("Trying to connect to default metaserver instead....", 3, 1);

		/* Connect to metaserver */
		socket = CreateClientSocket(META_ADDRESS, 8801);
	}

#ifdef META_ADDRESS_2
	/* Check for failure */
	if (socket == -1) {
		prt("Failed to connect to meta server.", 2, 1);
		prt("Trying to connect to default alternate metaserver instead....", 3, 1);
		/* Hack -- Connect to metaserver #2 */
		socket = CreateClientSocket(META_ADDRESS_2, 8801);
	}
#endif

	/* Check for failure */
	if (socket == -1) {
		prt("Failed to connect to meta server.", 2, 1);
		return enter_server_name();
	}

	/* Wipe the buffer so valgrind doesn't complain - mikaelh */
	C_WIPE(buf, 80192, char);

	/* Read */
	bytes = SocketRead(socket, buf, 80192);

	/* Close the socket */
	SocketClose(socket);

	/* Check for error while reading */
	if (bytes <= 0) return enter_server_name();

	Term_clear();

#ifdef EXPERIMENTAL_META
	call_lua(0, "meta_display", "(s)", "d", buf, &i);
#else

	/* Start at the beginning */
	ptr = buf;
	i = 0;

	/* Print each server */
	while (ptr - buf < bytes) {
		/* Check for no entry */
		if (strlen(ptr) <= 1) {
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
		out_val[j] = '\0';

		k = 0;
		while (j) {
			l = strlen(&out_val[k]);
			if (j > 75) {
				l = 75;
				while (out_val[k + l] != ' ') l--;
				out_val[l] = '\0';
			}
			prt(out_val + k, lines++, (k ? 4 : 1));
			k += (l + 1);
			j = strlen(&out_val[k]);
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
	while (1) {
		/* Get a key */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;
		c = inkey();

		/* Check for quit */
		if (c == 'Q' || c == KTRL('Q')) return enter_server_name();
		else if (c == ESCAPE) {
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
static char *human_syllable1[] = {
	"Ab", "Ac", "Ad", "Af", "Agr", "Ast", "As", "Al", "Adw", "Adr", "Ar",
	"B", "Br", "C", "Cr", "Ch", "Cad", "D", "Dr", "Dw", "Ed", "Eth", "Et",
	"Er", "El", "Eow", "F", "Fr", "G", "Gr", "Gw", "Gal", "Gl", "H", "Ha",
	"Ib", "Jer", "K", "Ka", "Ked", "L", "Loth", "Lar", "Leg", "M", "Mir",
	"N", "Nyd", "Ol", "Oc", "On", "P", "Pr", "R", "Rh", "S", "Sev", "T",
	"Tr", "Th", "V", "Y", "Z", "W", "Wic",
};

static char *human_syllable2[] = {
	"a", "ae", "au", "ao", "are", "ale", "ali", "ay", "ardo", "e", "ei",
	"ea", "eri", "era", "ela", "eli", "enda", "erra", "i", "ia", "ie",
	"ire", "ira", "ila", "ili", "ira", "igo", "o", "oa", "oi", "oe",
	"ore", "u", "y",
};

static char *human_syllable3[] = {
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
void create_random_name(int race, char *name) {
	char *syl1, *syl2, *syl3;
	int idx;

	(void) race; /* suppress compiler warning */

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
