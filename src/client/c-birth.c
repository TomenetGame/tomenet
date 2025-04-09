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

/* Stat order limits (new way) for setting reduced/boosted stats (10 = unmodified base stat) */
#define STAT_MOD_BASE	10
#define STAT_MOD_MIN	8
#define STAT_MOD_MAX	17

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

/* Use Windows TEMP folder (acquired from environment variable) for pinging the servers in the meta server list.
   KEEP CONSISTENT WITH nclient.c!
   Note: We actually use the user's home folder still, not the os_temp_path. TODO maybe: Change that. */
#define WINDOWS_USE_TEMP



/*
 * Choose the character's name
 */
static void choose_name(void) {
	char tmp[ACCNAME_LEN], fmt[ACCNAME_LEN], *cp;

	/* Prompt and ask */
#ifndef SIMPLE_LOGIN
	c_put_str(TERM_SLATE, "If you are new to TomeNET, read this:", 7, 2);
	prt("https://www.tomenet.eu/guide.php", 8, 2);
	c_put_str(TERM_SLATE, "*** Logging in with an account ***", 12, 2);
	prt("In order to play, you need to create an account.", 14, 2);
	prt("Your account can hold a maximum of 7 different characters to play with!", 15, 2);
	prt("If you don't have an account yet, just enter one of your choice and make sure", 16, 2);
	prt("that you remember its name and password. Each player should have not more", 17, 2);
	prt("than 1 account. Ask a server administrator to 'validate' your account!", 18, 2);
	prt("If an account is not validated, it has certain restrictions to prevent abuse.", 19, 2);
#else
 #ifdef ATMOSPHERIC_INTRO /* ascii-art in the top area */
	char introline[6][MAX_CHARS] = { 0 };
  #if 0
	char introline_col[6] = { TERM_FIRETHIN, TERM_LITE, TERM_FIRE, TERM_FIRE, TERM_FIRE , TERM_EMBER };
  #elif 1
	char introline_col[6] = { TERM_FIRETHIN, TERM_LITE, TERM_FIRE, TERM_FIRE, TERM_PLAS , TERM_EMBER };
  #elif 0
	char introline_col[6] = { TERM_FIRETHIN, TERM_LITE, TERM_FIRE, TERM_PLAS, TERM_EMBER, TERM_DETO };
  #else
	char introline_col[6] = { TERM_FIRETHIN, TERM_LITE, TERM_FIRE, TERM_PLAS, TERM_EMBER, TERM_HELLFIRE };
  #endif
	int x, y;
  #define LOGO_ROW 1
  #define LOGO_ATTR TERM_EMBER
  #define LOGO_SOLID /* use 'solid-wall' char instead of '#' */
	/* display a title */
  #if 0
	strcpy(introline[0], "  ^^^^^^^^^   ^^^^^^^    ^^^ ^^^      ^^^^^^^    ^^^  ^^^    ^^^^^^^   ^^^^^^^^^");
	strcpy(introline[1], " /########/  /######/   /##|/##|     /######/   /##| /##/   /######/  /########/");
	strcpy(introline[2], "   /##/     /##/ ##/   /###/###|    /##/       /###|/##/   /##/         /##/    ");
	strcpy(introline[3], "  /##/     /##/ ##/   /########|   /######/   /###/###/   /######/     /##/     ");
	strcpy(introline[4], " /##/     /##/ ##/   /##|##||##|  /##/       /##/|###/   /##/         /##/      ");
	strcpy(introline[5], "/##/     /######/   /##/|##||##| /######/   /##/ |##/   /######/     /##/       ");
  #else
	strcpy(introline[0], " ^^^^^^^^^^  ^^^^^^^^  ^^^^    ^^^^  ^^^^^^^^   ^^^  ^^^^  ^^^^^^^^  ^^^^^^^^^^");
	strcpy(introline[1], " |########|  |######|  |###\\  /###|  |######|   |##\\ |##|  |######|  |########|");
	strcpy(introline[2], " |########|  |##| ##|  |####\\/####|  |##|       |###\\|##|  |##|      |########|");
	strcpy(introline[3], "    |##|     |##| ##|  |##########|  |######|   |#######|  |######|     |##|   ");
	strcpy(introline[4], "    |##|     |##| ##|  |##|\\##/|##|  |##|       |##|\\###|  |##|         |##|   ");
	strcpy(introline[5], "    |##|     |######|  |##| \\/ |##|  |######|   |##| \\##|  |######|     |##|   ");
  #endif
  #ifdef USE_GRAPHICS
	if (use_graphics) {
		int w;
		char c;
		char32_t c32;

		for (y = 0; y < 6; y++) {
			for (w = 0; w < 80; w++) {
				c = introline[y][w];
				switch (c) {
				case '#': c32 = 813; break;
				case '|': c32 = ' '; break;
				case '\\': c32 = (introline[y][w - 1] == '#' || introline[y][w + 1] == ' ' ? 825 : 826); break;
				case '/': c32 = (introline[y][w - 1] == '#' || introline[y][w + 1] == ' ' ? 827 : 828); break;
   #if 0 // 0'ed -> keep the flickering top parts ASCII? :)
				//case '^': c32 = ...we don't have anything for this actually^^...; break;
   #endif
				default: c32 = (char32_t)c;
				}
				Term_draw(w, LOGO_ROW + y, introline_col[y], c32);
			}
		}
	} else
  #endif
	{
  #ifdef LOGO_SOLID
		if (strcmp(ANGBAND_SYS, "gcu") && !force_cui) /* No solid chars in terminal mode for now, seems to cause glitches */
   #if defined(WINDOWS) && defined(USE_LOGFONT)
		if (!use_logfont) /* doesn't have font_map_solid_walls style characters */
   #endif
		for (y = 0; y < 6; y++) for (x = 0; x < MAX_CHARS; x++) if (introline[y][x] == '#')
   #ifdef WINDOWS
			introline[y][x] = FONT_MAP_SOLID_WIN;
   #else
			introline[y][x] = FONT_MAP_SOLID_X11;
   #endif
  #endif
		for (y = 0; y < 6; y++)
			c_put_str(introline_col[y], introline[y], LOGO_ROW + y, 0);
	}
 #endif
	c_put_str(TERM_SLATE, "Welcome! In order to play, you need to create an account.", LOGIN_ROW, 2);
	c_put_str(TERM_SLATE, "If you don't have an account yet, just enter one of your choice. Remember", LOGIN_ROW + 1, 2);
	c_put_str(TERM_SLATE, "your name and password. Players may only own one account each at a time.", LOGIN_ROW + 2, 2);

	c_put_str(TERM_SLATE, "If you are new to TomeNET, this guide may prove useful:", LOGIN_ROW + 10, 2);
	prt("https://www.tomenet.eu/guide.php", LOGIN_ROW + 11, 2);
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
		if (askfor_aux(tmp, ACCNAME_LEN - 1, ASKFOR_LIVETRIM)) strcpy(nick, tmp);
		/* at this point: ESC = quit game */
		else {
			/* Nuke each term before exit. */
			for (int j = ANGBAND_TERM_MAX - 1; j >= 0; j--) {
				if (ang_term[j]) term_nuke(ang_term[j]);
			}
			exit(0);
		}

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
	sprintf(fmt, "%%-%d.%ds", ACCNAME_LEN - 1, ACCNAME_LEN - 1);
	sprintf(tmp, fmt, nick);

	/* Re-Draw the name (in light blue) */
#ifndef SIMPLE_LOGIN
	c_put_str(TERM_L_BLUE, tmp, 2, 15);
#else
	c_put_str(TERM_L_BLUE, tmp, LOGIN_ROW + 4, 15);
#endif

	/* Erase the prompt, etc */
	clear_from(21);
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
		else return(FALSE);

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
		Term_putch(15 + c, 3, TERM_L_BLUE, 'x');
#else
		Term_putch(15 + c, LOGIN_ROW + 5, TERM_L_BLUE, 'x');
#endif

	/* Erase the prompt, etc */
	clear_from(21);
	return(TRUE);
}


/*
 * Choose the character's sex				-JWT-
 */
static bool choose_sex(void) {
	char c = '\0';		/* pfft redesign while (1) */
	bool hazard = FALSE;
	bool parity = magik(50);

	/* Note: 'sex' also contains all 'mode' information. This is split up and processed on server-side. */
	sex = 0x0000;

	put_str("m) Male", 21, parity ? 2 : 17);
	put_str("f) Female", 21, parity ? 17 : 2);
	put_str("      ", 4, CHAR_COL);
	if (valid_dna) {
		if (dna_sex & MODE_MALE) c_put_str(TERM_SLATE, "Male", 4, CHAR_COL);
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
		if (valid_dna) c_put_str(TERM_SLATE, "Choose sex (* for random, \377B#\377s/\377B%\377s to reincarnate, Q to quit): ", 20, 0);
		else c_put_str(TERM_SLATE, "Choose sex (* for random, Q to quit): ", 20, 0);

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		if (!hazard) c = inkey();
		if (c == 'Q' || c == KTRL('Q')) quit(NULL);
		if (c == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) return(FALSE);
#endif
		if (c == 'm') {
			sex |= MODE_MALE;
			put_str("      ", 4, CHAR_COL);
			c_put_str(TERM_L_BLUE, "Male", 4, CHAR_COL);
			break;
		} else if (c == 'f') {
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
				if (dna_sex & MODE_MALE) c = 'm';
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
	return(TRUE);
}


static void clear_diz(void) {
	int i;

	for (i = 0; i < 12; i++)
		c_put_str(TERM_L_UMBER, "                                                  ", DIZ_ROW + i, DIZ_COL);
}

static void display_race_diz(int r) {
	int i = 0;

	clear_diz();
	if (!race_diz[r][i][0]) return; /* server !newer_than 4.5.1.2 */

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
		if (valid_dna) c_put_str(TERM_SLATE, "Choose race (* random, \377B#\377s/\377B%\377s reincarnate, Q quit, BACKSPACE back, ? guide):", n, 0);
		else c_put_str(TERM_SLATE, "Choose race (* random, Q quit, BACKSPACE back, 2/4/6/8, ? guide):", n, 0);
		display_race_diz(sel);

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		if (!hazard) c = inkey();

		if (c == 'Q' || c == KTRL('Q')) quit(NULL);
		if (c == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) return(FALSE);
#endif
		if (c == '\b') {
			clear_diz();
			clear_from(n);
			return(FALSE);
		}

		if (c == '?') {
			cmd_the_guide(3, 0, "races");
			continue;
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
				valid_dna = FALSE;
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
	return(TRUE);
}


static void display_trait_diz(int r) {
	int i;

	/* Transform visible index back to real index */
	for (i = 0; i <= r; i++)
		if (!(trait_info[i].choice & BITS(race))) r++;

	i = 0;
	clear_diz();
	if (!trait_diz[r][i][0]) return; /* server !newer_than 4.5.1.2 */

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
	if (Setup.max_trait == 0) return(TRUE);

#ifdef HIDE_UNAVAILABLE_TRAIT
	/* If server doesn't support traits, or we only have the
	   dummy 'N/A' trait available in general, skip trait choice */
	if (Setup.max_trait == 1) return(TRUE);
#endif

	/* Prepare to list, skip trait #0 'N/A' */
	for (j = 1; j < Setup.max_trait; j++) {
		tp_ptr = &trait_info[j];

		/* Super-hacky: s_PVP_MAIA. Since choose_trait() is usually called before choose_mode()
		   we'll have to get called twice for this occassion as shown_traits will be 0 on first run. */
		if ((sex & (MODE_PVP | MODE_DED_PVP)) && race == RACE_MAIA && s_PVP_MAIA && (j == TRAIT_ENLIGHTENED || j == TRAIT_CORRUPTED)) tp_ptr->choice |= BITS(race);

		if (!(tp_ptr->choice & BITS(race))) continue;
		shown_traits++;
	}
	/* Super-hacky: s_PVP_MAIA. We also have to disable trait #0 (N/A) because it is an indicator that there are no available traits, overriding that there are actually available ones. */
	if ((sex & (MODE_PVP | MODE_DED_PVP)) && race == RACE_MAIA && s_PVP_MAIA) trait_info[0].choice = 0x0;

	/* No traits available? */
	if (shown_traits == 0) {
		/* Paranoia - server bug (not even 'N/A' "available") */
		if (!(trait_info[0].choice & BITS(race))) return(TRUE);
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
		return(TRUE);
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
		if (valid_dna) c_put_str(TERM_SLATE, "Choose trait (* random, \377B#\377s/\377B%\377s reincarnate, Q quit, BACKSPACE back, ? guide):", n, 0);
		else c_put_str(TERM_SLATE, "Choose trait (* for random, Q to quit, BACKSPACE to go back, 2/4/6/8, ? guide):", n, 0);
		display_trait_diz(sel);

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		if (!hazard) c = inkey();

		if (c == 'Q' || c == KTRL('Q')) quit(NULL);
		if (c == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) return(FALSE);
#endif

		if (c == '?') {
			cmd_the_guide(3, 0, "traits");
			continue;
		}

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
			return(FALSE);
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
				valid_dna = FALSE;
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

#if 0 /* Wrong - we might have eg traits 14 and 15 available, which amounts to 2 shown_traits, but 14 > 2 and 15 > 2 so they won't pass.. */
		/* Paranoia */
		if (j > shown_traits) continue;
#endif

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

	return(TRUE);
}


static void display_class_diz(int r) {
	int i = 0;

	clear_diz();
	if (!class_diz[r][i][0]) return; /* server !newer_than 4.5.1.2 */

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
		if (valid_dna) c_put_str(TERM_SLATE, "Choose class (* random, \377B#\377s/\377B%\377s reincarnate, Q quit, BACKSPACE back, ? guide):", n, 0);
		else c_put_str(TERM_SLATE, "Choose class (* for random, Q to quit, BACKSPACE to go back, 2/4/6/8, ? guide):", n, 0);
		display_class_diz(sel);

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		if (!hazard) c = inkey();

		if (c == 'Q' || c == KTRL('Q')) quit(NULL);
		if (c == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) return(FALSE);
#endif
		if (c == '\b') {
			clear_diz();
			clear_from(n - 3);
			return(FALSE);
		}

		if (c == '?') {
			cmd_the_guide(3, 0, "classes");
			continue;
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
					valid_dna = FALSE;
					hazard = FALSE;
					auto_reincarnation = FALSE;
					continue;
				}
#endif
			} else {
				valid_dna = FALSE;
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
	return(TRUE);
}


/*
 * Get the desired stat order.
 */
#define SHOW_BPR
static bool choose_stat_order(void) {
	int i, j, k, avail[6], crb, maxed_stats = 0;
	char c = '\0';
	char out_val[160], stats[6][4], buf[8], buf2[8], buf3[8];
	bool hazard = FALSE;
	s16b stat_order_tmp[6];

	player_class *cp_ptr = &class_info[class];
	player_race *rp_ptr = &race_info[race];

	char attribute_name[6][20] = { "Strength", "Intelligence", "Wisdom", "Dexterity", "Constitution", "Charisma" };


	for (i = 0; i < C_ATTRIBUTES; i++) stat_order_tmp[i] = stat_order[i];

	/* Init the 6 attributes' diz from lua */
	memset(attribute_diz, 0, sizeof(char) * 6 * 8 * 61);
	for (j = 0; j < C_ATTRIBUTES; j++)
		for (i = 0; i < 8; i++) {
			sprintf(out_val, "return get_attribute_diz(\"%s\", %d)", attribute_name[j], i);
			strcpy(attribute_diz[j][i], string_exec_lua(0, out_val));
		}

	/* Character stats are randomly rolled (1 time): */
	if (char_creation_flags == 0) {

		put_str("Stat order  :", 11, 1);

		/* All stats are initially available */
		for (i = 0; i < C_ATTRIBUTES; i++) {
			strncpy(stats[i], stat_names[i], 3);
			stats[i][3] = '\0';
			avail[i] = 1;
		}

		/* Find the ordering of all 6 stats */
		for (i = 0; i < C_ATTRIBUTES; i++) {
			/* Clear bottom of screen */
			clear_from(20);

			/* Print available stats at bottom */
			for (k = 0; k < C_ATTRIBUTES; k++) {
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
				put_str("Choose your stat order (* for random, Q to quit): ", 20, 0);
				if (hazard) {
					j = rand_int(6);
				} else {
					Term->scr->cx = Term->wid;
					Term->scr->cu = 1;

					c = inkey();
					if (c == 'Q' || c == KTRL('Q')) quit(NULL);
					if (c == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);
#ifdef RETRY_LOGIN
					if (rl_connection_destroyed) return(FALSE);
#endif
					if (c == '*') hazard = TRUE;

					j = (islower(c) ? A2I(c) : -1);
				}

				if ((j < C_ATTRIBUTES) && (j >= 0) && (avail[j])) {
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

#ifdef SHOW_BPR
		int tablesize = 0, *bpr_str = NULL, *bpr_dex = NULL, *bpr = NULL, mbpr = 0;
		char lua[MAX_CHARS], *cbpr;
		bool show_bpr = FALSE, use_formula = TRUE;

		/* Check if we have an actual formula available */
		sprintf(out_val, "return get_class_bpr2(\"%s\", 0, 0, 0)", class_info[class].title);
		strcpy(lua, string_exec_lua(0, out_val));
		if (lua[0] == 'X') { /* No. We have to fall back to using a static table. */
			use_formula = FALSE;

			/* Find out size of our str/dex/bpr table and create it */
			sprintf(out_val, "return get_class_bpr_tablesize()");
			tablesize = atoi(string_exec_lua(0, out_val));
			if (tablesize) {
				C_MAKE(bpr_str, tablesize, int);
				C_MAKE(bpr_dex, tablesize, int);
				C_MAKE(bpr, tablesize, int);

				/* Display potential BpR for super-light weapons */
				for (i = 0; i < tablesize; i++) {
					sprintf(out_val, "return get_class_bpr(\"%s\", %d)", class_info[class].title, i);
					strcpy(lua, string_exec_lua(0, out_val));
					bpr_str[i] = atoi(lua + 1);
					/* Paranoia: Make this robust, so people's clients arent suddenly broken */
					cbpr = strchr(lua, 'D');
					if (!cbpr) break;
					bpr_dex[i] = atoi(cbpr + 1);
					cbpr = strchr(lua, 'B');
					if (!cbpr) break;
					bpr[i] = atoi(cbpr + 1);
				}

				/* Finished populating the table without errors? */
				if (i == tablesize) {
					show_bpr = TRUE; /* no errors, we're able to display projected BpR just fine */
					/* Hack: For some classes BpR has no significant meaning, so we won't display it */
					if (!bpr[0]) show_bpr = FALSE;
				}
			}
		} else if (lua[0] != 'N') show_bpr = TRUE; /* Hack: For some classes BpR has no significant meaning, so we won't display it */
#endif

		j = 0; /* current stat to be modified */
		k = 30; /* free points left */

		clear_from(14);

		c_put_str(TERM_SLATE, "Distribute your attribute points (\377ouse them all!\377s):", rowA, col1);
		c_put_str(TERM_L_GREEN, format("%2d", k), rowA, col3);
		c_put_str(TERM_SLATE, "If any values are shown under 'recommended minimum', try to reach those.", rowA + 1, col1);
		if (valid_dna) c_put_str(TERM_SLATE, "Current:   (Base) (Prev) Recommended min:", 15, col2);
#ifndef SHOW_BPR
		else c_put_str(TERM_SLATE, "Current:   (Base)        Recommended min:", 15, col2);
#else
		else c_put_str(TERM_L_WHITE, "Current:   (Base)        Recommended min:", 15, col2);
#endif

		//if (valid_dna) c_put_str(TERM_SLATE, "Press \377B#\377s/\377B%\377s to reincarnate.", 15, col1);
		if (valid_dna) c_put_str(TERM_SLATE, "Press \377B#\377s/\377B%\377s to reincarnate.", 16, col1);
#if 0
		c_put_str(TERM_SLATE, "Use keys '+', '-', 'RETURN'", 16, col1);
		c_put_str(TERM_SLATE, "or 8/2/4/6 or arrow keys to", 17, col1);
		c_put_str(TERM_SLATE, "modify and navigate.", 18, col1);
		c_put_str(TERM_SLATE, "After you distributed all", 19, col1);
		c_put_str(TERM_SLATE, "points, press ESC to proceed.", 20, col1);
		c_put_str(TERM_SLATE, "'Q' = quit, BACKSPACE = back.", 21, col1);
#else
		c_put_str(TERM_WHITE, "\377w8\377s/\377w2\377s/\377wUp\377s/\377wDown\377s: Select a stat.", 17, col1);
		//c_put_str(TERM_SLATE, " Select a stat.", 17, col1);
		c_put_str(TERM_WHITE, "-\377s/\377w+\377s/\377w4\377s/\377w6\377s/\377wLeft\377s/\377wRight\377s: Modify.", 18, col1);
		//c_put_str(TERM_SLATE, " Modify the selected stat.", 19, col1);
		c_put_str(TERM_WHITE, "RETURN\377s/\377wESC\377s: Proceed when done.", 19, col1);
		c_put_str(TERM_WHITE, "Q\377s quit, \377wBACKSPC\377s back, \377w?\377s guide.", 20, col1);
#endif

#ifndef SHOW_BPR
		c_put_str(TERM_SLATE, "No more than 1 attribute out of the 6 is allowed to be maximised.", 23, col1);
#else
		c_put_str(TERM_SLATE, "No more than 1 attribute out of the 6 is allowed to be maximised.", 14, col1);
 #if 0 /* Just query best weapon type? */
		if (show_bpr) c_put_str(TERM_L_WHITE, "BpR this character gets with extremely light weapons (up to 3.0 lbs):", 23, col1);
 #else /* Actually query and display ALL four weapon types */
		if (show_bpr) c_put_str(TERM_L_WHITE, "BpR this character gets with lightest sword/blunt/axe/polearm:", 23, col1 - 1);
 #endif
#endif

		/* Start with displaying Strength diz */
		c_put_str(TERM_L_UMBER,format("   - %s -    ", attribute_name[0]), DIZ_ROW, 30);
		for (i = 0; i < 8; i++) c_put_str(TERM_YELLOW, attribute_diz[0][i], DIZ_ROW + 1 + i, 30);

		for (i = 0; i < C_ATTRIBUTES; i++) {
			stat_order[i] = STAT_MOD_BASE;
			strncpy(stats[i], stat_names[i], 3);
			stats[i][3] = '\0';
			if (valid_dna) {
				if (dna_stat_order[i] >= STAT_MOD_MIN && dna_stat_order[i] <= STAT_MOD_MAX) {
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
			/* Display remaining skill points */
			c_put_str(TERM_L_GREEN, format("%2d", k), rowA, col3);

#ifdef SHOW_BPR
			/* Display projected BpR with <= 3.0lbs weapons */
			if (show_bpr) {
				if (use_formula) {
 #if 0 /* Just query best weapon type? */
					/* Assume fixed weight of 3.0 lbs (lowest breakpoint for BpR) -
					   Note that spears and cleavers weigh 6.0 and 5.0 though. */
					sprintf(out_val, "return get_class_bpr2(\"%s\", 30, %d, %d)",
					    class_info[class].title,
					    stat_order[0] + cp_ptr->c_adj[0] + rp_ptr->r_adj[0],
					    stat_order[3] + cp_ptr->c_adj[3] + rp_ptr->r_adj[3]);
					strcpy(lua, string_exec_lua(0, out_val));
					mbpr = atoi(lua);
					if (mbpr) { //paranoia
						sprintf(out_val, "%d", mbpr);
						c_put_str(mbpr == 1 ? TERM_ORANGE : (mbpr == 2 ? TERM_YELLOW : TERM_L_GREEN), out_val, 23, 73);
					} else c_put_str(TERM_ORANGE, "  ", 23, 73); //paranoia
 #else /* Actually query and display ALL four weapon types */
					sprintf(out_val, "return get_class_bpr3(\"%s\", %d, %d)",
					    class_info[class].title,
					    stat_order[0] + cp_ptr->c_adj[0] + rp_ptr->r_adj[0],
					    stat_order[3] + cp_ptr->c_adj[3] + rp_ptr->r_adj[3]);
					strcpy(lua, string_exec_lua(0, out_val));
					if (lua[0] == '\377') //paranoia
						c_put_str(TERM_L_GREEN, lua, 23, 73 - 8);
					else c_put_str(TERM_ORANGE, "? / ? / ? / ?", 23, 73 - 8); //paranoia
 #endif
				} else {
					mbpr = 1;
					for (i = 0; i < tablesize; i++)
						if (bpr[i] /* Check that this particular entry isn't disabled */
						    && stat_order[0] + cp_ptr->c_adj[0] + rp_ptr->r_adj[0] >= bpr_str[i]
						    && stat_order[3] + cp_ptr->c_adj[3] + rp_ptr->r_adj[3] >= bpr_dex[i])
							mbpr = bpr[i];
					//sprintf(out_val, "%d%s", mbpr, mbpr == 3 ? "+" : " ");
					sprintf(out_val, "%d", mbpr);
					c_put_str(mbpr == 1 ? TERM_ORANGE : (mbpr == 2 ? TERM_YELLOW : TERM_L_GREEN), out_val, 23, 73);
				}
			}
#endif

			for (i = 0; i < C_ATTRIBUTES; i++) {
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
					if (stat_order[i] == STAT_MOD_MIN)
						c_put_str(TERM_L_RED, out_val, 16 + i, col2);
					else if (stat_order[i] == STAT_MOD_MAX)
						c_put_str(TERM_L_BLUE, out_val, 16 + i, col2);
					else
						c_put_str(TERM_ORANGE, out_val, 16 + i, col2);
				} else {
					if (stat_order[i] == STAT_MOD_MIN)
						c_put_str(TERM_RED, out_val, 16 + i, col2);
					else if (stat_order[i] == STAT_MOD_MAX)
						c_put_str(TERM_VIOLET, out_val, 16 + i, col2);
					else
						c_put_str(TERM_L_UMBER, out_val, 16 + i, col2);
				}
			}

			Term->scr->cx = Term->wid;
			Term->scr->cu = 1;

			if (!auto_reincarnation) c = inkey();

			if (c == '?') {
				cmd_the_guide(3, 0, "attributes");
				continue;
			}

			crb = cp_ptr->c_adj[j] + rp_ptr->r_adj[j];
			if (c == '-' || c == '4' || c == 'h') {
				if (stat_order[j] > STAT_MOD_MIN &&
				    /* exception: allow going below 3 if we initially were below 3 too */
				    (stat_order[j] > STAT_MOD_BASE || stat_order[j]+crb > 3)) {
					if (stat_order[j] <= STAT_MOD_BASE + 2) {
						/* intermediate */
						stat_order[j]--;
						k++;
					} else if (stat_order[j] <= STAT_MOD_BASE + 4) {
						/* high */
						stat_order[j]--;
						k += 2;
					} else if (stat_order[j] <= STAT_MOD_MAX - 1) {
						/* nearly max */
						stat_order[j]--;
						k += 3;
					} else {
						/* max! (STAT_MOD_MAX) */
						stat_order[j]--;
						k += 4;
						maxed_stats--;
					}
				}
			}
			if (c == '+' || c == '6' || c == 'l') {
				if (stat_order[j] < STAT_MOD_MAX) {
					if (stat_order[j] < STAT_MOD_BASE + 2 && k >= 1) {
						/* intermediate */
						stat_order[j]++;
						k--;
					} else if (stat_order[j] < STAT_MOD_BASE + 4 && k >= 2) {
						/* high */
						stat_order[j]++;
						k -= 2;
					} else if (stat_order[j] < STAT_MOD_MAX - 1 && k >= 3) {
						/* nearly max */
						stat_order[j]++;
						k -= 3;
					} else if (k >= 4 && !maxed_stats) { /* only 1 maxed stat is allowed */
						/* max! (STAT_MOD_MAX) */
						stat_order[j]++;
						k -= 4;
						maxed_stats++;
					}
				}
			}
			if (c == '2' || c == 'j') j = (j + 1) % 6;
			if (c == '8' || c == 'k') j = (j + 5) % 6;
			if (c == '2' || c == '8' || c == 'j' || c == 'k') {
				c_put_str(TERM_L_UMBER,format("   - %s -    ", attribute_name[j]), DIZ_ROW, 30);
				for (i = 0; i < 8; i++) c_put_str(TERM_YELLOW, attribute_diz[j][i], DIZ_ROW + 1 + i, 30);
			}
			if (c == '\e' || c == '\r') {
				if (k > 0) {
					c_put_str(TERM_NUKE, format("%2d", k), rowA, col3);
					if (get_check2(format("You still have %d stat points left to distribute! Really continue?", k), FALSE)) break;
					c_put_str(TERM_L_GREEN, format("%2d", k), rowA, col3);
				} else break;
			}
			if (c == 'Q' || c == KTRL('Q')) quit(NULL);
			if (c == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);
#ifdef RETRY_LOGIN
			if (rl_connection_destroyed) return(FALSE);
#endif
			if (c == '\b') {
				for (i = 0; i < C_ATTRIBUTES; i++) stat_order[i] = stat_order_tmp[i];

				for (i = DIZ_ROW; i <= DIZ_ROW + 8; i++) Term_erase(30, i, 255);
				clear_from(rowA);

				return(FALSE);
			}
			if (c == '#') {
				if (valid_dna) {
					for (i = 0; i < C_ATTRIBUTES; i++) {
						if (dna_stat_order[i] >= STAT_MOD_MIN && dna_stat_order[i] <= STAT_MOD_MAX) stat_order[i] = dna_stat_order[i];
						else stat_order[i] = STAT_MOD_MIN;
					}
					for (i = DIZ_ROW; i <= DIZ_ROW + 8; i++) Term_erase(30, i, 255);
					clear_from(rowA);
					return(TRUE);
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

		for (i = DIZ_ROW; i <= DIZ_ROW + 8; i++) Term_erase(30, i, 255);
		clear_from(rowA);
	}
	return(TRUE);
}

/* Quick hack!		- Jir -
 * TODO: remove hard-coded things. */
static bool choose_mode(void) {
	char c = '\0';
	bool hazard = FALSE;

	/* Note: Currently, RPG server (s_RPG) does not allow PvP/Everlasting/Normal modes. */

	/* specialty: slot-exclusive-mode char? */
	if (dedicated) {
		if (create_character_ok_iddc) {
			put_str("i) Ironman Deep Dive Challenge", 16, 2);
			c_put_str(TERM_SLATE, "(Unworldly - one life only.)", 16, 33);
			put_str("s) Ironman Deep Dive Challenge Soloist", 17, 2);
			c_put_str(TERM_SLATE, "(Cannot trade with other players)", 17, 41);
			put_str("H) Hellish Ironman Deep Dive Challenge", 18, 2);
			c_put_str(TERM_SLATE, "(Extra hard, sort of ridiculous)", 18, 41);
		}
		if (create_character_ok_pvp) {
			if (!s_RPG) {
				put_str("p) PvP", 19, 2);
				c_put_str(TERM_SLATE, "(Can't beat the game, instead special 'player vs player' rules apply)", 19, 9);
			}
		}

		c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
		if (valid_dna) {
#if 1
			if (((dna_sex & MODE_HARD) == MODE_HARD) && ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST)) c_put_str(TERM_SLATE, "Hellish", 9, CHAR_COL);
			else if (((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) && ((dna_sex & MODE_SOLO) == MODE_SOLO)) c_put_str(TERM_SLATE, "Soloist", 9, CHAR_COL);
			else if ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) c_put_str(TERM_SLATE, "No Ghost", 9, CHAR_COL);// -- redundant wording, instead:
			else if ((dna_sex & MODE_EVERLASTING) == MODE_EVERLASTING) c_put_str(TERM_SLATE, "Everlasting", 9, CHAR_COL);
			else if ((dna_sex & MODE_PVP) == MODE_PVP) c_put_str(TERM_SLATE, "PvP", 9, CHAR_COL);
			else c_put_str(TERM_SLATE, "Normal", 9, CHAR_COL);
#else
			if (dna_sex & MODE_DED_IDDC) {
				if (((dna_sex & MODE_HARD) == MODE_HARD) && ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST)) c_put_str(TERM_SLATE, "Hellish", 9, CHAR_COL);
				else if (((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) && ((dna_sex & MODE_SOLO) == MODE_SOLO)) c_put_str(TERM_SLATE, "Soloist", 9, CHAR_COL);
				else if ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) c_put_str(TERM_SLATE, "No Ghost", 9, CHAR_COL);// -- redundant wording, instead:
				else if ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) c_put_str(TERM_SLATE, "IDDC", 9, CHAR_COL);
				//invalid -- else if ((dna_sex & MODE_EVERLASTING) == MODE_EVERLASTING) c_put_str(TERM_SLATE, "Everlasting", 9, CHAR_COL);
				//invalid -- must be in ded-pvp bracket below -- else if ((dna_sex & MODE_PVP) == MODE_PVP) c_put_str(TERM_SLATE, "PvP", 9, CHAR_COL);
				//invalid -- else c_put_str(TERM_SLATE, "Normal", 9, CHAR_COL);
				else c_put_str(TERM_SLATE, "IDDC", 9, CHAR_COL); //paranoia
			} else { /* MODE_DED_PVP */
				c_put_str(TERM_SLATE, "PvP", 9, CHAR_COL); //must be MODE_PVP
			}
#endif
		}

		if (auto_reincarnation) {
			hazard = TRUE;
			c = '#';
		}

#ifdef ATMOSPHERIC_INTRO
	DRAW_TORCH
#endif

		while (1) {
			if (valid_dna) c_put_str(TERM_SLATE, "Choose mode (* random, \377B#\377s/\377B%\377s reincarnate, Q quit, BACKSPACE back, ? guide): ", 15, 0);
			else c_put_str(TERM_SLATE, "Choose mode (* for random, Q to quit, BACKSPACE to go back, ? guide): ", 15, 0);

			Term->scr->cx = Term->wid;
			Term->scr->cu = 1;

			if (!hazard) c = inkey();

			if (c == 'Q' || c == KTRL('Q')) quit(NULL);
			if (c == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);
#ifdef RETRY_LOGIN
			if (rl_connection_destroyed) return(FALSE);
#endif

			if (c == '?') {
				cmd_the_guide(3, 0, "modes");
				continue;
			}

			if (c == '\b') {
				clear_from(15);
				return(FALSE);
			} else if (c == 'p' && !s_RPG) {
				if (!create_character_ok_pvp) {
					bell();
					continue;
				}
				sex |= MODE_PVP | MODE_DED_PVP;
				c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
				c_put_str(TERM_L_BLUE, "PvP", 9, CHAR_COL);
				break;
			} else if (c == 'i') {
				if (!create_character_ok_iddc) {
					bell();
					continue;
				}
				sex |= MODE_NO_GHOST | MODE_DED_IDDC;
				c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
				c_put_str(TERM_L_BLUE, "No Ghost", 9, CHAR_COL);
				break;
			} else if (c == 'H') {
				if (!create_character_ok_iddc) {
					bell();
					continue;
				}
				sex |= (MODE_NO_GHOST | MODE_HARD | MODE_DED_IDDC);
				c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
				c_put_str(TERM_L_BLUE, "Hellish", 9, CHAR_COL);
				break;
			} else if (c == 's') {
				if (!create_character_ok_iddc) {
					bell();
					continue;
				}
				sex |= (MODE_NO_GHOST | MODE_SOLO | MODE_DED_IDDC);
				c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
				c_put_str(TERM_L_BLUE, "Soloist", 9, CHAR_COL);
				break;
			} else if (c == '?') {
				/*do_cmd_help("help.hlp");*/
			} else if (c == '*') {
				switch (rand_int(s_RPG ? 3 : 4)) {
				case 0:
					if (!create_character_ok_iddc) {
						bell();
						continue;
					}
					c = 'i';
					break;
				case 1:
					if (!create_character_ok_iddc) {
						bell();
						continue;
					}
					c = 's';
					break;
				case 2:
					if (!create_character_ok_iddc) {
						bell();
						continue;
					}
					c = 'H';
					break;
				case 3:
					if (!create_character_ok_pvp) {
						bell();
						continue;
					}
					c = 'p';
					break;
				}
				hazard = TRUE;
			} else if (c == '#') {
				if (valid_dna) {
					if (((dna_sex & MODE_HARD) == MODE_HARD) && ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST)) c = 'H';
					else if ((dna_sex & MODE_HARD) == MODE_HARD) c = 'H';
					else if (((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) && ((dna_sex & MODE_SOLO) == MODE_SOLO)) c = 's';
					else if ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) c = 'i';
					else if ((dna_sex & MODE_EVERLASTING) == MODE_EVERLASTING) c = 'i';
					else if ((dna_sex & MODE_PVP) == MODE_PVP && !s_RPG) c = 'p';
					else c = 'i';
					/* What dedicated slot types are even left? */
					if (!create_character_ok_iddc &&
					    (c == 'H' || c == 's' || c == 'i')) {
						bell();
						hazard = FALSE;
						auto_reincarnation = FALSE;
						continue;
					}
					if (!create_character_ok_pvp && c == 'p') {
						bell();
						hazard = FALSE;
						auto_reincarnation = FALSE;
						continue;
					}
					/* Fix dedicated modes */
					if (!(dna_sex & (MODE_PVP | MODE_DED_PVP))) dna_sex |= MODE_DED_IDDC;
					else dna_sex |= MODE_DED_PVP;

					hazard = TRUE;
					//prevent endless loop on RPG server
					if (s_RPG && (c == 'p')) {
						bell();
						hazard = FALSE;
						auto_reincarnation = FALSE;
						continue;
					}
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
		return(TRUE);
	}

	if (!s_RPG) {
		put_str("n) Normal", 16, 2);
		c_put_str(TERM_SLATE, "(3 lives)", 16, 12);
	}
	put_str("g) No Ghost", 17, 2);
	c_put_str(TERM_SLATE, "('Unworldly' - One life only. The traditional rogue-like way)", 17, 14);
	put_str("s) Soloist", 18, 2);
	c_put_str(TERM_SLATE, "(Same as 'No Ghost' but cannot exchange gold/items with anybody)", 18, 14);
	if (!s_RPG) {
		put_str("e) Everlasting", 19, 2);
		c_put_str(TERM_SLATE, "(You may resurrect infinite times but cannot enter highscore)", 19, 17);
	}
	/* hack: no weird modi on first client startup!
	   To find out whether it's 1st or not we check firstrun and # of existing characters.
	   However, we just don't display the choices, they're still choosable except for firstrun! */
	if (!firstrun || existing_characters) {
		put_str("H) Hellish", 20, 2);
		c_put_str(TERM_SLATE, "(Like 'Unworldly' mode but extra hard - sort of ridiculous)", 20, 13);
		if (!s_RPG) {
			put_str("p) PvP", 21, 2);
			c_put_str(TERM_SLATE, "(Can't beat the game, instead special 'player vs player' rules apply)", 21, 9);
		}
	}

	c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
	if (valid_dna) {
		if (((dna_sex & MODE_HARD) == MODE_HARD) && ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST)) c_put_str(TERM_SLATE, "Hellish", 9, CHAR_COL);
		else if (((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) && ((dna_sex & MODE_SOLO) == MODE_SOLO)) c_put_str(TERM_SLATE, "Soloist", 9, CHAR_COL);
		else if ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) c_put_str(TERM_SLATE, "No Ghost", 9, CHAR_COL);
		else if ((dna_sex & MODE_EVERLASTING) == MODE_EVERLASTING && !s_RPG) c_put_str(TERM_SLATE, "Everlasting", 9, CHAR_COL);
		else if ((dna_sex & MODE_PVP) == MODE_PVP && !s_RPG) c_put_str(TERM_SLATE, "PvP", 9, CHAR_COL);
		else if (!s_RPG) c_put_str(TERM_SLATE, "Normal", 9, CHAR_COL);
	}

	if (auto_reincarnation) {
		hazard = TRUE;
		c = '#';
	}

#ifdef ATMOSPHERIC_INTRO
	DRAW_TORCH
#endif

	while (1) {
		if (valid_dna) c_put_str(TERM_SLATE, "Choose mode (* random, \377B#\377s/\377B%\377s reincarnate, Q quit, BACKSPACE back, ? guide): ", 15, 0);
		else c_put_str(TERM_SLATE, "Choose mode (* for random, Q to quit, BACKSPACE to go back, ? guide): ", 15, 0);

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		if (!hazard) c = inkey();

		if (s_RPG) switch (c) {
			case 'n': //normal
			case 'p': //pvp
			case 'e': //everlasting
				bell();
				continue;
		}

		if (c == 'Q' || c == KTRL('Q')) quit(NULL);
		if (c == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) return(FALSE);
#endif

		if (c == '?') {
			cmd_the_guide(3, 0, "modes");
			continue;
		}

		if (c == '\b') {
			clear_from(15);
			return(FALSE);
		}
		else if (c == 'p' && !firstrun) {
			sex |= MODE_PVP;
			c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
			c_put_str(TERM_L_BLUE, "PvP", 9, CHAR_COL);
			break;
		} else if (c == 'n') {
			c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
			c_put_str(TERM_L_BLUE, "Normal", 9, CHAR_COL);
			break;
		} else if (c == 'g') {
			sex |= MODE_NO_GHOST;
			c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
			c_put_str(TERM_L_BLUE, "No Ghost", 9, CHAR_COL);
			break;
		} else if (c == 's') {
			sex |= MODE_NO_GHOST | MODE_SOLO;
			c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
			c_put_str(TERM_L_BLUE, "Soloist", 9, CHAR_COL);
			break;
		}
		else if (c == 'H' && !firstrun) {
			sex |= (MODE_NO_GHOST | MODE_HARD);
			c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
			c_put_str(TERM_L_BLUE, "Hellish", 9, CHAR_COL);
			break;
		} else if (c == 'e') {
			sex |= MODE_EVERLASTING;
			c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
			c_put_str(TERM_L_BLUE, "Everlasting", 9, CHAR_COL);
			break;
		} else if (c == '?') {
			/*do_cmd_help("help.hlp");*/
		} else if (c == '*') {
			switch (rand_int(s_RPG ? 3 : 6)) {
			case 0:
				c = 'g';
				break;
			case 1:
				c = 'H';
				break;
			case 2:
				c = 's';
				break;
			case 3:
				c = 'p';
				break;
			case 4:
				c = 'e';
				break;
			case 5:
				c = 'n';
				break;
			}
			hazard = TRUE;
		} else if (c == '#') {
			if (valid_dna) {
				hazard = TRUE;
				if (((dna_sex & MODE_HARD) == MODE_HARD) && ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST)) c = 'H';
				else if (((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) && ((dna_sex & MODE_SOLO) == MODE_SOLO)) c = 's';
				else if ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST) c = 'g';
				else if ((dna_sex & MODE_EVERLASTING) == MODE_EVERLASTING && !s_RPG) c = 'e';
				else if ((dna_sex & MODE_PVP) == MODE_PVP && !s_RPG) c = 'p';
				else if (!s_RPG) c = 'n';

				//prevent endless loop on Arcade (because s_RPG is true there too - maybe it shouldn't?) (and RPG?) server
				if ((c == '#') ||
				//prevent endless loop on RPG server
				    (s_RPG && (c == 'n' || c == 'p' || c == 'e'))) {
					bell();
					hazard = FALSE;
					auto_reincarnation = FALSE;
					continue;
				}
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
	return(TRUE);
}

/* Fruit bat is now a "body modification" that can be applied to all "modes" - C. Blue */
static bool choose_body_modification(void) {
	char c = '\0';
	bool hazard = FALSE;

	/* normal mode is the default */
	sex &= ~MODE_FRUIT_BAT;

	put_str("n) Normal body", 20, 2);
	/* hack: no weird modi on first client startup!
	   To find out whether it's 1st or not we check firstrun and # of existing characters.
	   However, we just don't display the choices, they're still choosable except on firstrun! */
	if (!firstrun || existing_characters) {
		put_str("f) Fruit bat", 21, 2);
		if (class == CLASS_ARCHER) {
			c_put_str(TERM_L_DARK, "f) Fruit bat", 21, 2);
			c_put_str(TERM_L_DARK, "(WARNING: Do not pick this as Archer, as bats cannot use bows!)", 21, 15);
		} else if (class == CLASS_MIMIC || class == CLASS_DRUID || class == CLASS_SHAMAN)
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
		if (valid_dna) c_put_str(TERM_SLATE, "Choose body mod (* random, \377B#\377s/\377B%\377s reincarnate, Q quit, BACKSPC back, ? guide):", 19, 0);
		else c_put_str(TERM_SLATE, "Choose body modification (* for random, Q quit, BACKSPACE to go back, ? guide): ", 19, 0);

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		if (!hazard) c = inkey();

		if (c == 'Q' || c == KTRL('Q')) quit(NULL);
		if (c == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) return(FALSE);
#endif

		if (c == '?') {
			cmd_the_guide(3, 0, "body mod");
			continue;
		}

		if (c == '\b') {
			clear_from(19);
			return(FALSE);
		}
		else if (c == 'f' && !firstrun) {
			sex |= MODE_FRUIT_BAT;
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
	return(TRUE);
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
			//music(exec_lua(0, "return get_music_index(\"generic\")"));
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
	c_put_str(TERM_SLATE, "Name and password are case-sensitive! Name must start on upper-case letter.", LOGIN_ROW + 7, 2);
	c_put_str(TERM_SLATE, "The name may also contain numbers, spaces and these symbols: .,-'&_$%~#", LOGIN_ROW + 8, 2);
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

#if 0 /* let's only store crecedentials after an actually successful login instead, ie in nclient.c */
//#ifdef WINDOWS
	/* erase crecedentials? */
	if (!strlen(nick) || !strlen(pass))
		store_crecedentials();
//#endif
#endif

	/* Message */
	put_str("Connecting to server....", 22, 1);

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
	dedicated = (sex & (MODE_DED_PVP | MODE_DED_IDDC)) != 0;
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
		if (dna_sex & MODE_MALE) c_put_str(TERM_SLATE, "Male", 4, CHAR_COL);
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
		else if ((dna_sex & MODE_NO_GHOST) == MODE_NO_GHOST && (dna_sex & MODE_SOLO) == MODE_SOLO) c_put_str(TERM_SLATE, "Soloist", 9, CHAR_COL);
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
#if 0 /* RPG server now supports the new 'Soloist' mode too, instead of just 'Unworldly' so far. Also should allow 'Hellish' actually. */
	if (!s_RPG || s_RPG_ADMIN) {
		if (!choose_mode()) goto cstats;
	} else {
		c_put_str(TERM_L_BLUE, "                    ", 9, CHAR_COL);
		c_put_str(TERM_L_BLUE, "No Ghost", 9, CHAR_COL);
	}
#else
	if (!choose_mode()) goto cstats;
#endif
	/* Super-hacky: PvP-Mode Maiar of starter level 20+ need to pick a trait! So we have to do it after choosing the mode: */
	if ((sex & (MODE_PVP | MODE_DED_PVP)) && race == RACE_MAIA && s_PVP_MAIA && !choose_trait()) goto cstats;

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
	save_birth_file(cname, FALSE);

	/* Message */
	put_str("Entering game...  [Hit any key]", 21, 1);

	/* Wait for key */
	i = inkey();
	if (i == KTRL('T')) xhtml_screenshot("screenshot????", FALSE); //xD

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

	prt("Enter a server's hostname or IP address you want to connect to,", 1, 1);
	prt(" or instead press ESCAPE to quit: ", 2, 1);

	/* Message */
	Term_putstr(0, 6, -1, TERM_L_BLUE, "In case the meta server seems unreachable or reports no servers, you can try");
	Term_putstr(0, 7, -1, TERM_L_BLUE, "looking in your TomeNET folder for the script files named '\377RTomeNET-direct-xxxx\377B'");
	Term_putstr(0, 8, -1, TERM_L_BLUE, "where xxxx denote the region of each script, ie \377oEU\377B for europe, \377oNA\377B for north");
	Term_putstr(0, 9, -1, TERM_L_BLUE, "america, \377oAPAC\377B for asia-pacific, and double-click one of them to connect directly");
	Term_putstr(0, 10, -1, TERM_L_BLUE, "to a TomeNET server of the region of your choice, bypassing DNS and meta server.");

	/* Move cursor */
	move_cursor(4, 3);

	/* Default */
	strcpy(server_name, "europe.tomenet.eu");

	/* Ask for server name */
	return(askfor_aux(server_name, 79, 0));
}

#ifdef EXPERIMENTAL_META
void display_experimental_meta(void) {
 #ifndef META_DISPLAYPINGS_LATER // -- now that GCU has a weird glitch where we have to redraw meta() after each getch() call from Term_inkey(), we don't want to clear because it also clears ping times display, blinking annoyingly.
	Term_clear();
 #else
  #if 0 /* We only really need this once: When refresh_meta_once is triggered. */
	/* This is the one line of code inside Term_clear() that actually "enables" the screen again to get written on again, making the meta_display visible again oO - wtf */
	Term->total_erase = TRUE;
  #endif
 #endif
	call_lua(0, "meta_display", "(s)", "d", meta_buf, &meta_i);
	Term_fresh();//added to try and debug WinXP partially-black meta screen problem
}
#endif
bool meta_read_and_close(void) {
	int retries, bytes = 0;

	/* Wipe the buffer so valgrind doesn't complain - mikaelh */
	C_WIPE(meta_buf, 80192, char);

	/* Listen for reply (try ten times in ten seconds) */
	for (retries = 0; retries < 10; retries++) {
		/* Set timeout */
		SetTimeout(1, 0);

		/* Wait for info */
		if (!SocketReadable(meta_socket)) continue;

		/* Read */
		bytes = SocketRead(meta_socket, meta_buf, 80192);
		break;
	}

	/* Close the socket */
	SocketClose(meta_socket);
	meta_socket = -1;
	meta_i = 0;

	/* Check for error while reading */
	if (bytes <= 0) return(FALSE);

	return(TRUE);
}
bool meta_connect(void) {
	if (strlen(meta_address) > 0) {
		/* Metaserver in config file */
		meta_socket = CreateClientSocket(meta_address, 8801);
	}

	/* Failed to connect to metaserver in config file, connect to hard-coded address */
	if (meta_socket == -1) {
		Term_erase(0, 0, 80);
		Term_erase(1, 0, 80);
		prt("Failed to connect to user-specified meta server.", 0, 0);
		prt("Trying to connect to default metaserver instead....", 1, 0);

		/* Connect to metaserver */
		meta_socket = CreateClientSocket(META_ADDRESS, 8801);
	}

#ifdef META_ADDRESS_2
	/* Check for failure */
	if (meta_socket == -1) {
		Term_erase(0, 0, 80);
		Term_erase(1, 0, 80);
		prt("Failed to connect to meta server.", 0, 0);
		prt("Trying to connect to default alternate metaserver instead....", 1, 0);
		/* Hack -- Connect to metaserver #2 */
		meta_socket = CreateClientSocket(META_ADDRESS_2, 8801);
	}
#endif

	/* Check for failure */
	if (meta_socket == -1) {
		Term_erase(0, 0, 80);
		Term_erase(1, 0, 80);
		prt("Failed to connect to meta server.", 0, 0);
		return(FALSE);
	}

	return(TRUE);
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
	bool meta_failed;

#ifdef EXPERIMENTAL_META
	int j;
	char buf[1024], response[1024], c;
 #ifdef META_PINGS
	int k, v, r;
	FILE *fff;
  #if 0
	char path[1024];
  #endif
  #ifdef WINDOWS
	char xpath[1024];
  #endif
 #endif

	meta_socket = -1;
#else
	int j, k, l, bytes = 0, socket = -1, offsets[20], lines = 0;
	char buf[1024], *ptr, c, out_val[260];
#endif
#ifdef META_PINGS
	int tmpport;
#endif

	/* We NEED lua here, so if it aint initialized yet, do it */
	init_lua();

	/* Message */
	prt("Connecting to metaserver for server list....", 1, 1);

	/* Make sure message is shown */
	Term_fresh();

	/* Connect to meta, and then read server list and then close the connection again!
	   If anything fails, call manual server name input request instead. */
	meta_failed = !meta_connect() || !meta_read_and_close();
#ifdef USE_SOUND_2010
	if (use_sound) {
		/* This is the first music the player can get to hear, even before title screen... */
		music(exec_lua(0, "return get_music_index(\"meta\")") + 10000 + 20000); //hack: play instantly w/o fade-in and don't repeat
	}
#endif
	if (meta_failed) return(enter_server_name());

#ifdef META_PINGS
	/* Test for 'ping' command availability (should always be there though).
	   Temporarily abuse meta_pings_servers as marker (-1 = ping disabled). */
 #if defined(WINDOWS) && defined(WINDOWS_USE_TEMP)
	/* Use official Windows TEMP folder instead? */
	if (getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
		strcpy(buf, getenv("HOMEDRIVE"));
		strcat(buf, getenv("HOMEPATH"));
		strcat(buf, "\\__ping.tmp");
	} else
 #endif
	path_build(buf, 1024, ANGBAND_DIR_USER, "__ping.tmp");
 #ifdef WINDOWS
	meta_pings_xpath[0] = 0;
	r = system(format("ping /? > %s 2>&1", buf));
 #else /* assume POSIX */
	r = system(format("ping -h > %s 2>&1", buf)); // (returns 2 on success)
 #endif
	(void)r; //slay compiler warning;

	fff = my_fopen(buf, "r");
	if (!fff) meta_pings_servers = -1;
	else {
		meta_pings_servers = -1;
		while (my_fgets(fff, response, sizeof(buf)) == 0) {
 #ifdef WINDOWS
			if (!strstr(response, "ping [-")) continue;
 #else
			if (!strstr(response, "  ping [")) continue;
 #endif
			meta_pings_servers = 0;
			break;
		}
		my_fclose(fff);
		remove(buf);

 #ifdef WINDOWS /* Try Win XP workaround */
		if (meta_pings_servers == -1) {
			strcpy(xpath, getenv("SYSTEMROOT"));
			strcat(xpath, "\\system32");
			r = system(format("%s\\ping /? > %s 2>&1", xpath, buf));
			(void)r; //slay compiler warning;

			/* second (and final) chance.. */
			fff = my_fopen(buf, "r");
			if (!fff) meta_pings_servers = -1;
			else {
				meta_pings_servers = -1;
				while (my_fgets(fff, response, sizeof(buf)) == 0) {
					if (!strstr(response, "ping [-")) continue;
					meta_pings_servers = 0;
					sprintf(meta_pings_xpath, " %s", xpath);
					break;
				}
				my_fclose(fff);
				remove(buf);
			}
		}
 #endif
	}
#endif

	/* Finally display the meta server list which we read earlier into the global variable meta_buf. */
#ifdef EXPERIMENTAL_META
 #ifdef META_DISPLAYPINGS_LATER
	Term_clear();
 #endif
	display_experimental_meta();
#else
	/* Start at the beginning */
	ptr = meta_buf;
	i = 0;

	Term_clear();

	/* Print each server */
	while (ptr - meta_buf < bytes) {
		/* Check for no entry */
		if (strlen(ptr) <= 1) {
			/* Increment */
			ptr++;
			/* Next */
			continue;
		}

		/* Save offset */
		offsets[i] = ptr - meta_buf;

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

#ifdef META_PINGS
	/* From the read metaserver list, initiate pinging all the game servers. */
	if (!meta_pings_servers) {
		/* Obtain all unique server names */
		v = 0;
		for (j = 0; j < meta_i; j++) {
			call_lua(0, "meta_get", "(s,d)", "sd", meta_buf, j, &tmp, &tmpport);
			if (!tmp[0]) continue; //paranoia

			/* Check for duplicate names */
			for (k = 0; k < v; k++)
				if (streq(meta_pings_server_name[k], tmp)) break;
			/* Mark duplicate and remember its first equivalent */
			if (k != v) meta_pings_server_duplicate[v] = k;
			else meta_pings_server_duplicate[v] = -1; /* We're the first of our name!~ */

			/* Display "pinging.." status */
			call_lua(0, "meta_add_ping", "(d,d)", "d", j, -2, &r);

			/* Add server to list */
			strcpy(meta_pings_server_name[v], tmp);
			v++;
			/* Limit of how many servers we can list */
			if (v == META_PINGS) break;
		}
		/* Redraw 'pinging..' (paranoia) */
		Term_fresh();
		/* hack: hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		(void)r; //slay compiler warning;
		meta_pings_servers = v;

 #ifdef META_PINGS_CREATEFILE
		/* Init global stuff for CreateFile()/CreateProcess() */
		for (j = 0; j < meta_pings_servers; j++) {
			ZeroMemory(&sa[j], sizeof(SECURITY_ATTRIBUTES));
			sa[j].nLength = sizeof(SECURITY_ATTRIBUTES);
			sa[j].lpSecurityDescriptor = NULL;
			sa[j].bInheritHandle = TRUE;

			ZeroMemory(&si[j], sizeof(STARTUPINFO));
			si[j].cb = sizeof(STARTUPINFO);
			si[j].dwFlags |= STARTF_USESTDHANDLES;

  #if 0 /* done in nclient.c atm? */
   #if defined(WINDOWS) && defined(WINDOWS_USE_TEMP)
			/* Use official Windows TEMP folder instead? */
			if (getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
				strcpy(path, getenv("HOMEDRIVE"));
				strcat(path, getenv("HOMEPATH"));
				strcat(path, format("\\__ping_%s.tmp", meta_pings_server_name[i]));
			} else
   #endif
			path_build(path, 1024, ANGBAND_DIR_USER, format("__ping_%s.tmp", meta_pings_server_name[j]));
			fhan[j] = CreateFile(path,
			    FILE_WRITE_DATA, //FILE_APPEND_DATA,
   #if 0
			    0, //or:
   #else
			    FILE_SHARE_WRITE | FILE_SHARE_READ, // should be 0 tho
   #endif
			    &sa[j],
			    OPEN_ALWAYS,
			    FILE_ATTRIBUTE_NORMAL,
			    NULL);

			si[j].hStdInput = NULL;
			si[j].hStdError = fhan[j];
			si[j].hStdOutput = fhan[j];
  #endif
		}
 #endif
	}
#endif

#ifdef META_DISPLAYPINGS_LATER
	/* Crazy GCU bug workaround, for first call of inkey() below ->->-> 'i = getch();' will actually blank out the screen for unknown reason! */
	refresh_meta_once = TRUE;
#endif
	/* Ask to choose a server until happy */
	while (1) {
		/* Get a key */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;
		c = inkey();
#ifdef META_DISPLAYPINGS_LATER
		/* Without this, choosing a server too quickly may result in the login screen being partially overwritten with the meta server again */
		refresh_meta_once = FALSE;
#endif

		/* Check for quit */
		if (c == 'Q' || c == KTRL('Q')) {
#ifdef META_PINGS
			char path[1024];

			/* Disable pinging. */
			for (i = 0; i < meta_pings_servers; i++) {
				/* Clean up temp files */
 #if defined(WINDOWS) && defined(WINDOWS_USE_TEMP)
				/* Use official Windows TEMP folder instead? */
				if (getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
					strcpy(path, getenv("HOMEDRIVE"));
					strcat(path, getenv("HOMEPATH"));
					strcat(path, format("\\__ping_%s.tmp", meta_pings_server_name[i]));
				} else
 #endif
				path_build(path, 1024, ANGBAND_DIR_USER, format("__ping_%s.tmp", meta_pings_server_name[i]));
				remove(path);
			}
 #if defined(WINDOWS) && defined(WINDOWS_USE_TEMP)
			/* Use official Windows TEMP folder instead? */
			if (getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
				strcpy(path, getenv("HOMEDRIVE"));
				strcat(path, getenv("HOMEPATH"));
				strcat(path, "\\__ping.tmp");
			} else
 #endif
			path_build(path, 1024, ANGBAND_DIR_USER, "__ping.tmp");
			remove(path);
			meta_pings_servers = 0;
#endif
			return(enter_server_name());
		} else if (c == ESCAPE) {
#ifdef META_PINGS
			char path[1024];

			/* Disable pinging. */
			for (i = 0; i < meta_pings_servers; i++) {
				/* Clean up temp files */
 #if defined(WINDOWS) && defined(WINDOWS_USE_TEMP)
				/* Use official Windows TEMP folder instead? */
				if (getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
					strcpy(path, getenv("HOMEDRIVE"));
					strcat(path, getenv("HOMEPATH"));
					strcat(path, format("\\__ping_%s.tmp", meta_pings_server_name[i]));
				} else
 #endif
				path_build(path, 1024, ANGBAND_DIR_USER, format("__ping_%s.tmp", meta_pings_server_name[i]));
				remove(path);
			}
 #if defined(WINDOWS) && defined(WINDOWS_USE_TEMP)
			/* Use official Windows TEMP folder instead? */
			if (getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
				strcpy(path, getenv("HOMEDRIVE"));
				strcat(path, getenv("HOMEPATH"));
				strcat(path, "\\__ping.tmp");
			} else
 #endif
			path_build(path, 1024, ANGBAND_DIR_USER, "__ping.tmp");
			remove(path);
			meta_pings_servers = 0;
#endif
			quit(NULL);
			return(FALSE);
		} else if (c == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);

		/* Index */
		j = (islower(c) ? A2I(c) : -1);

		/* Check for legality */
		if (j >= 0 && j < meta_i) break;
	}
#ifdef META_PINGS
	/* Disable pinging for the rest of the game again. */
	for (i = 0; i < meta_pings_servers; i++) {
		/* Clean up temp files */
 #if defined(WINDOWS) && defined(WINDOWS_USE_TEMP)
		/* Use official Windows TEMP folder instead? */
		if (getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
			strcpy(path, getenv("HOMEDRIVE"));
			strcat(path, getenv("HOMEPATH"));
			strcat(path, format("\\__ping_%s.tmp", meta_pings_server_name[i]));
		} else
 #endif
		path_build(path, 1024, ANGBAND_DIR_USER, format("__ping_%s.tmp", meta_pings_server_name[i]));
		remove(path);
	}
 #if defined(WINDOWS) && defined(WINDOWS_USE_TEMP)
	/* Use official Windows TEMP folder instead? */
	if (getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
		strcpy(path, getenv("HOMEDRIVE"));
		strcat(path, getenv("HOMEPATH"));
		strcat(path, "\\__ping.tmp");
	} else
 #endif
	path_build(path, 1024, ANGBAND_DIR_USER, "__ping.tmp");
	remove(path);
	meta_pings_servers = 0;
#endif

#ifdef EXPERIMENTAL_META
	call_lua(0, "meta_get", "(s,d)", "sd", meta_buf, j, &tmp, &server_port);
	strcpy(server_name, tmp);
#else
	/* Extract server name */
	sscanf(meta_buf + offsets[j], "%s", server_name);
#endif

	/* Success */
	return(TRUE);
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
