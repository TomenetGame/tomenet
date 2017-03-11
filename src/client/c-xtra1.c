/* $Id$ */
/* Handle the printing of info to the screen */



#include "angband.h"


/* make more room for potential new stuff in char dump,
   especially WINNER status line: */
#define NEW_COMPRESSED_DUMP

/* Compress both AC lines (Base AC, To AC) into one,
   to free up a line for "Trait"? */
#define NEW_COMPRESSED_DUMP_AC

/* Don't display 'Trait' if traits aren't available */
#define HIDE_UNAVAILABLE_TRAIT


/*
 * Print character info at given row, column in a 13 char field
 */
static void prt_field(cptr info, int row, int col) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	/* Dump 13 spaces to clear */
	c_put_str(TERM_WHITE, "             ", row, col);

	/* Dump the info itself */
	c_put_str(TERM_L_BLUE, info, row, col);

	/* restore cursor position */
	Term_gotoxy(x, y);
}


/*
 * Converts stat num into a six-char (right justified) string
 */
void cnv_stat(int val, char *out_val) {
	if (!c_cfg.linear_stats) {
		/* Above 18 */
		if (val > 18) {
			int bonus = (val - 18);

			if (bonus >= 220)
				sprintf(out_val, "18/%3s", "***");
			else if (bonus >= 100)
				sprintf(out_val, "18/%03d", bonus);
			else
				sprintf(out_val, " 18/%02d", bonus);
		}
		/* From 3 to 18 */
		else sprintf(out_val, "    %2d", val);
	} else {
		/* Above 18 */
		if (val > 18) {
			int bonus = (val - 18);

			if (bonus >= 220)
				sprintf(out_val, "    **");
			else
				sprintf(out_val, "    %2d", 18 + (bonus / 10));
		}
		/* From 3 to 18 */
		else sprintf(out_val, "    %2d", val);
	}
}


/*
 * Print character stat in given row, column
 */
void prt_stat(int stat, int max, int cur, int cur_base) {
	char tmp[32];
	int x, y;

	if (client_mode == CLIENT_PARTY) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (cur < max) {
		Term_putstr(0, ROW_STAT + stat, -1, TERM_WHITE, (char*)stat_names_reduced[stat]);
		cnv_stat(cur, tmp);
		Term_putstr(COL_STAT + 6, ROW_STAT + stat, -1, TERM_YELLOW, tmp);
	} else {
		Term_putstr(0, ROW_STAT + stat, -1, TERM_WHITE, (char*)stat_names[stat]);
		cnv_stat(cur, tmp);
		if(cur_base < (18 + 100))
			Term_putstr(COL_STAT + 6, ROW_STAT + stat, -1, TERM_L_GREEN, tmp);
		else
			Term_putstr(COL_STAT + 6, ROW_STAT + stat, -1, TERM_L_UMBER, tmp);
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints "title", including "wizard" or "winner" as needed.
 */
void prt_title(cptr title) {
	prt_field(title, ROW_TITLE, COL_TITLE);
}


/*
 * Prints level and experience
 */
/* Display exp bar in 5% steps instead of 10%? (Uses 2 colours to disgtinguish) */
#define EXP_BAR_FINESCALE
/* Fill rest of exp bar up with dark colour instead of leaving it blank? */
#define EXP_BAR_FILLDARK

#if 1 /* draw exp bar in blue? */
 #define EXP_BAR_HI TERM_L_BLUE
 #define EXP_BAR_LO TERM_BLUE
 #define EXP_BAR_NO TERM_L_DARK
#else /* draw exp bar in green? */
 #define EXP_BAR_HI TERM_L_GREEN
 #define EXP_BAR_LO TERM_GREEN
 #define EXP_BAR_NO TERM_L_DARK
#endif
#define EXP_BAR_HI_DRAINED TERM_YELLOW
#define EXP_BAR_LO_DRAINED TERM_L_UMBER
#define EXP_BAR_NO_DRAINED TERM_YELLOW
//#define EXP_BAR_NO_DRAINED TERM_UMBER

void prt_level(int level, int max_lev, int max_plv, s32b max, s32b cur, s32b adv, s32b adv_prev) {
	char tmp[32], exp_bar_char;
	int colour = (level < max_lev) ? TERM_YELLOW : TERM_L_GREEN;
	int x, y;
	int scale = adv - adv_prev;

	/* remember cursor position */
	Term_locate(&x, &y);

	Term_putstr(0, ROW_LEVEL, -1, TERM_WHITE, "LEVEL ");

	sprintf(tmp, "%2d", level);

	if (max_lev == max_plv) {
		Term_putstr(COL_LEVEL + 6, ROW_LEVEL, -1, colour, "    ");
		Term_putstr(COL_LEVEL + 10, ROW_LEVEL, -1, colour, tmp);
	} else {
		Term_putstr(COL_LEVEL + 6, ROW_LEVEL, -1, colour, tmp);
		sprintf(tmp, "(%2d)", max_plv);
		Term_putstr(COL_LEVEL + 8, ROW_LEVEL, -1, TERM_L_GREEN, tmp);
	}

	if (!c_cfg.exp_bar) {
		if (!c_cfg.exp_need)
			sprintf(tmp, "%9d", (int)cur);
		else {
			if (level >= PY_MAX_LEVEL || !adv)
				(void)sprintf(tmp, "*********");
			else {
				/* Hack -- display in minus (to avoid confusion chez player) */
				(void)sprintf(tmp, "%9d", (int)(cur - adv));
			}
		}
		strcat(tmp, " "); //hack to correctly clear line after player disabled 'exp_bar' option (it's 1 char 'too long')

		if (cur >= max) {
			Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "XP ");
			Term_putstr(COL_EXP + 3, ROW_EXP, -1, TERM_L_GREEN, tmp);
		} else {
			Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "Xp ");
			Term_putstr(COL_EXP + 3, ROW_EXP, -1, TERM_YELLOW, tmp);
		}
	} else { //c_cfg.exp_bar
#ifdef EXP_BAR_FINESCALE
		int got_org = 0;
#endif
#ifdef WINDOWS
		if (c_cfg.font_map_solid_walls) exp_bar_char = 127; /* :-p hack */
#else
		if (c_cfg.font_map_solid_walls) exp_bar_char = 2; /* :-p hack */
#endif
		else exp_bar_char = '#';

		if (level >= PY_MAX_LEVEL || !adv || !scale) {
			(void)sprintf(tmp, "*********");

			if (cur >= max) {
				Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "XP ");
				Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_HI, tmp);
			} else {
				Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "Xp ");
				Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_HI_DRAINED, tmp);
			}

			/* restore cursor position */
			Term_gotoxy(x, y);

			return;
		}
#ifndef EXP_BAR_FINESCALE
		else {//if (!c_cfg.exp_need) {
			int got = ((cur - adv_prev) * 10) / scale, i;

			for (i = 0; i < got; i++) tmp[i] = exp_bar_char;
			tmp[i] = 0;
 #if 0
		} else {
			int got = ((cur - adv_prev) * 10) / scale, i;

			for (i = 0; i < got; i++) tmp[i] = '-'; //10 'filled bubbles' = next level, so only need to display 9!
			for (i = got; i < 9; i++) tmp[i] = exp_bar_char;
			tmp[i] = 0;
 #endif
		}
#else /* finer double-scale 0..10 in 0.5 steps :D */
		else {//if (!c_cfg.exp_need) {
			int got = ((cur - adv_prev) * 20 + half_exp) / scale, i;
			got_org = got;

			for (i = 0; i < got / 2; i++) tmp[i] = exp_bar_char;
			tmp[i] = 0;
 #if 0
		} else {
			int got = ((cur - adv_prev) * 20 + half_exp) / scale, i;
			got_org = got;

			for (i = 0; i < got / 2; i++) tmp[i] = '-'; //10 'filled bubbles' = next level, so only need to display 9!
			for (i = got / 2; i < 9; i++) tmp[i] = exp_bar_char;
			tmp[i] = 0;
 #endif
		}
#endif

#ifdef EXP_BAR_FILLDARK
		/* Paint dark base ;) */
		if (cur >= max) Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_NO, "---------");
		else Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_NO_DRAINED, "---------");
#else
		if (cur >= max) Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_NO, "         ");//just erase any previous '-' or exp bar possibly
		else Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_NO_DRAINED, "-");
#endif
		if (cur >= max) {
			Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "XP ");
#ifndef EXP_BAR_FINESCALE
			Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_HI, tmp);
#else
			/* special hack to be able to display this distinguishdly from got == 18 (we only have 9 characters available) */
			if (got_org == 19) Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_LO, tmp);
			else {
				Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_HI, tmp);
				if (got_org % 2) Term_putch(COL_EXP + 3 + got_org / 2, ROW_EXP, EXP_BAR_LO, exp_bar_char);
			}
#endif
		} else {
			Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "Xp ");
#ifndef EXP_BAR_FINESCALE
			Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_HI_DRAINED, tmp);
#else
			/* special hack to be able to display this distinguishdly from got == 18 (we only have 9 characters available) */
			if (got_org == 19) Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_LO_DRAINED, tmp);
			else {
				Term_putstr(COL_EXP + 3, ROW_EXP, -1, EXP_BAR_HI_DRAINED, tmp);
				if (got_org % 2) Term_putch(COL_EXP + 3 + got_org / 2, ROW_EXP, EXP_BAR_LO_DRAINED, exp_bar_char);
			}
#endif
		}
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints current gold
 */
void prt_gold(int gold) {
	char tmp[32];
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	put_str("AU ", ROW_GOLD, COL_GOLD);
	sprintf(tmp, "%9d", gold);
	if (p_ptr->au < 1000000000) strcat(tmp, " "); //hack to correctly clear line for players moving huge amounts
	c_put_str(TERM_L_GREEN, tmp, ROW_GOLD, COL_GOLD + 3);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints current AC
 */
void prt_ac(int ac) {
	char tmp[32];
	int x, y;
	if (client_mode == CLIENT_PARTY) return;

	/* remember cursor position */
	Term_locate(&x, &y);

	put_str("AC:", ROW_AC, COL_AC);
	sprintf(tmp, "%5d", ac);
	c_put_str(TERM_L_GREEN, tmp, ROW_AC, COL_AC + 7);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints Max/Cur hit points
 */
void prt_hp(int max, int cur) {
	char tmp[32];
	byte color;
	int x, y; /* for remembering cursor pos */

	/* remember cursor position */
	Term_locate(&x, &y);

	if (client_mode == CLIENT_PARTY) {
		color = TERM_L_RED;
		sprintf(tmp, "HP: %4d ", max);
		c_put_str(color, tmp, CLIENT_PARTY_ROWHP, CLIENT_PARTY_COLHP);

		sprintf(tmp, "%4d", cur);

		if (cur >= max)
			color = TERM_L_RED;
		else if (cur > max / 10)
			color = TERM_YELLOW;
		else
			color = TERM_RED;
		c_put_str(color, tmp, CLIENT_PARTY_ROWHP, CLIENT_PARTY_COLHP + 9);
	}
	/* DEG Default to else since only 2 types for now */
	else {
#ifndef CONDENSED_HP_SP
		put_str("Max HP ", ROW_MAXHP, COL_MAXHP);
		sprintf(tmp, "%5d", max);
		color = TERM_L_GREEN;
		c_put_str(color, tmp, ROW_MAXHP, COL_MAXHP + 7);

		put_str("Cur HP ", ROW_CURHP, COL_CURHP);

		sprintf(tmp, "%5d", cur);

		if (cur >= max)
			color = TERM_L_GREEN;
		else if (cur > max / 10)
			color = TERM_YELLOW;
		else
			color = TERM_RED;

		c_put_str(color, tmp, ROW_CURHP, COL_CURHP + 7);
#else
		put_str("HP:    /", ROW_MAXHP, 0);
		if (max == -9999) sprintf(tmp, "   -"); /* wonder whether this will ever be used */
		else sprintf(tmp, "%4d", max);
		color = TERM_L_GREEN;
		c_put_str(color, tmp, ROW_MAXHP, COL_MAXHP);
		if (cur == -9999) sprintf(tmp, "   -"); /* wonder whether this will ever be used */
		else sprintf(tmp, "%4d", cur);
		if (cur >= max)	color = TERM_L_GREEN;
		else if (cur > max / 10) color = TERM_YELLOW;
		else color = TERM_RED;
		c_put_str(color, tmp, ROW_CURHP, COL_CURHP);
#endif

		/* restore cursor position */
		Term_gotoxy(x, y);
	}
}
void prt_stamina(int max, int cur) {
	char tmp[32];
	byte color;
	int x, y; /* for remembering cursor pos */

	/* remember cursor position */
	Term_locate(&x, &y);

#ifdef CONDENSED_HP_SP
		put_str("ST:    /", ROW_MAXST, 0);
		if (max == -9999) sprintf(tmp, "   -");
		else sprintf(tmp, "%4d", max);
		color = TERM_L_GREEN;
		c_put_str(color, tmp, ROW_MAXST, COL_MAXST);
		if (cur == -9999) sprintf(tmp, "   -");
		else sprintf(tmp, "%4d", cur);
		if (cur >= max)	color = TERM_L_GREEN;
		else if (cur > max / 10) color = TERM_YELLOW;
		else color = TERM_RED;
		c_put_str(color, tmp, ROW_CURST, COL_CURST);
#endif

	/* restore cursor position */
	Term_gotoxy(x, y);
}
/* DEG print party members hps to screen */
void prt_party_stats(int member_num, byte color, char *member_name, int member_lev, int member_chp, int member_mhp, int member_csp, int member_msp) {
	char tmp[32];
	int rowspacing = 0;

	if (member_num != 0) rowspacing += (4 * member_num);

	if (client_mode != CLIENT_PARTY) return;

	if (member_name[0] == '\0' && member_lev == 0) {
		/* Empty member? Just clear it - mikaelh */
		int i;
		for (i = CLIENT_PARTY_ROWMBR + rowspacing; i < CLIENT_PARTY_ROWMBR + rowspacing + 3; i++)
			Term_erase(0, i, 12);
		return;
	}

	sprintf(tmp, "%s", member_name);
	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing) ,CLIENT_PARTY_COLMBR);

	sprintf(tmp, "HP: %4d ", member_mhp);
	color = TERM_L_RED;
	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing + 1) ,CLIENT_PARTY_COLMBR);

	sprintf(tmp, "%4d", member_chp);

	if (member_chp >= member_mhp)
		color = TERM_L_RED;
	else if (member_chp > member_mhp / 10)
		color = TERM_YELLOW;
	else
		color = TERM_RED;

	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing + 1) ,CLIENT_PARTY_COLMBR + 9);


	sprintf(tmp, "SP: %4d ", member_msp);
	color = TERM_L_BLUE;
	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing + 2) , CLIENT_PARTY_COLMBR + 0);

	sprintf(tmp, "%4d", member_csp);

	if (member_csp >= member_msp)
		color = TERM_L_BLUE;
	else if (member_csp > member_msp / 10)
		color = TERM_YELLOW;
	else
		color = TERM_RED;

	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing + 2) , CLIENT_PARTY_COLMBR + 9);
}


/*
 * Prints Max/Cur spell points
 */
void prt_sp(int max, int cur) {
	char tmp[32];
	byte color;
	int x, y; /* for remembering cursor pos */

	/* remember cursor position */
	Term_locate(&x, &y);

	if (client_mode == CLIENT_PARTY) {
		sprintf(tmp, "MP: %4d ", max);
		color = TERM_L_BLUE;
		c_put_str(color, tmp, CLIENT_PARTY_ROWSP, CLIENT_PARTY_COLSP);

		sprintf(tmp, "%4d", cur);
		color = TERM_L_GREEN;

		if (cur >= max)
			color = TERM_L_BLUE;
		else if (cur > max / 10)
			color = TERM_YELLOW;
		else
			color = TERM_RED;

		c_put_str(color, tmp, CLIENT_PARTY_ROWSP, CLIENT_PARTY_COLSP + 9);
	} else {
#ifndef CONDENSED_HP_SP
		put_str("Max MP ", ROW_MAXSP, COL_MAXSP);

		sprintf(tmp, "%5d", max);
		color = TERM_L_GREEN;

		c_put_str(color, tmp, ROW_MAXSP, COL_MAXSP + 7);


		put_str("Cur MP ", ROW_CURSP, COL_CURSP);

		sprintf(tmp, "%5d", cur);

		if (cur >= max)
			color = TERM_L_GREEN;
		else if (cur > max / 10)
			color = TERM_YELLOW;
		else
			color = TERM_RED;

		c_put_str(color, tmp, ROW_CURSP, COL_CURSP + 7);
#else
		put_str("MP:    /", ROW_MAXSP, 0);
		if (max == -9999) sprintf(tmp, "   -");
		else sprintf(tmp, "%4d", max);
		color = TERM_L_GREEN;
		c_put_str(color, tmp, ROW_MAXSP, COL_MAXSP);
		if (cur == -9999) sprintf(tmp, "   -");
		else sprintf(tmp, "%4d", cur);
		if (cur >= max) color = TERM_L_GREEN;
		else if (cur > max / 10) color = TERM_YELLOW;
		else color = TERM_RED;
		c_put_str(color, tmp, ROW_CURSP, COL_CURSP);
#endif
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints the player's current sanity.
 */
void prt_sane(byte attr, cptr buf) {
	int x, y;

	if (client_mode == CLIENT_PARTY)
		return;

#ifdef SHOW_SANITY	/* NO SANITY DISPLAY!!! */

	/* remember cursor position */
	Term_locate(&x, &y);

	put_str("SN:         ", ROW_SANITY, COL_SANITY);

	c_put_str(attr, buf, ROW_SANITY, COL_SANITY+3);

	/* restore cursor position */
	Term_gotoxy(x, y);
#endif	/* SHOW_SANITY */
}

/*
 * Prints depth into the dungeon
 */
void prt_depth(int x, int y, int z, bool town, int colour, int colour_sector, cptr name) {
	char depths[32];
	int x2, y2;

	/* Remember everything else */
	depth_town = town;
	depth_colour = colour;
	depth_colour_sector = colour_sector;
	/* if-check: may be identical pointer (!) */
	if (depth_name != name) strcpy(depth_name, name);

	/* remember cursor position */
	Term_locate(&x2, &y2);

	if (x == 127) sprintf(depths, "(?\?,?\?)");
	else sprintf(depths, "(%2d,%2d)", x, y);
	c_put_str(colour_sector, depths, ROW_XYPOS, COL_XYPOS);

	if (town)
		strcpy(depths, name);
	else if (c_cfg.depth_in_feet)
		sprintf(depths, "%dft", z * 50);
	else
		sprintf(depths, "Lev %d", z);

	/* Right-Adjust the "depth" and clear old values */
	c_prt(colour, format("%7s", depths), ROW_DEPTH, COL_DEPTH);

	/* New feeling-on-next-floor indicator after 4.5.6: */
	/* '+8' because depth string was right-aligned for width 7 in above format() */
	if (name[0] == '*') c_prt(TERM_L_BLUE, "*", ROW_DEPTH, COL_DEPTH + 8);

	/* restore cursor position */
	Term_gotoxy(x2, y2);

	/* hack: extend warning on speed colour too */
	if (colour_sector == TERM_L_DARK) no_tele_grid = TRUE;
	else no_tele_grid = FALSE;
	prt_speed(p_speed);
}

/*
 * Prints hunger information
 */
void prt_hunger(int food) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (food < PY_FOOD_FAINT)
		c_put_str(TERM_RED, "Weak  ", ROW_HUNGRY, COL_HUNGRY);
	else if (food < PY_FOOD_WEAK)
		c_put_str(TERM_ORANGE, "Weak  ", ROW_HUNGRY, COL_HUNGRY);
	else if (food < PY_FOOD_ALERT)
		c_put_str(TERM_YELLOW, "Hungry", ROW_HUNGRY, COL_HUNGRY);
	else if (food < PY_FOOD_FULL)
		c_put_str(TERM_L_GREEN, "      ", ROW_HUNGRY, COL_HUNGRY);
	else if (food < PY_FOOD_MAX)
		c_put_str(TERM_L_GREEN, "Full  ", ROW_HUNGRY, COL_HUNGRY);
	else
		c_put_str(TERM_GREEN, "Gorged", ROW_HUNGRY, COL_HUNGRY);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints blindness status
 */
void prt_blind(bool blind) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (blind)
		c_put_str(TERM_ORANGE, "Blind", ROW_BLIND, COL_BLIND);
	else
		put_str("     ", ROW_BLIND, COL_BLIND);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints confused status
 */
void prt_confused(bool confused) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (confused)
		c_put_str(TERM_ORANGE, "Confused", ROW_CONFUSED, COL_CONFUSED);
	else
		put_str("        ", ROW_CONFUSED, COL_CONFUSED);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints fear status
 */
void prt_afraid(bool fear) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (fear)
		c_put_str(TERM_ORANGE, "Afraid", ROW_AFRAID, COL_AFRAID);
	else
		put_str("      ", ROW_AFRAID, COL_AFRAID);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints poisoned status
 */
void prt_poisoned(bool poisoned) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (poisoned)
		c_put_str(TERM_ORANGE, "Poisoned", ROW_POISONED, COL_POISONED);
	else
		put_str("        ", ROW_POISONED, COL_POISONED);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints paralyzed/searching status
 */
void prt_state(bool paralyzed, bool searching, bool resting) {
	byte attr = TERM_WHITE;
	char text[16];
	int x, y;

	if (paralyzed) {
		attr = TERM_RED;
		strcpy(text, "Paralyzed!  ");
	}

	else if (searching) {
#if 0
		if (get_skill(SKILL_STEALTH) <= 10)
			strcpy(text, "Searching   ");
		else {
			attr = TERM_L_DARK;
			strcpy(text,"Stlth Mode  ");
		}
#else
		strcpy(text, "Searching   ");
#endif

	}

	else if (resting)
		strcpy(text, "Resting     ");
	else
		strcpy(text, "            ");

	/* remember cursor position */
	Term_locate(&x, &y);

	c_put_str(attr, text, ROW_STATE, COL_STATE);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints speed
 */
void prt_speed(int speed) {
	int attr = TERM_WHITE;
	char buf[32] = "";
	int x, y;

	if (speed > 0) {
		attr = TERM_L_GREEN;
		sprintf(buf, "Fast +%d", speed);
	} else if (speed < 0) {
		attr = TERM_L_UMBER;
		sprintf(buf, "Slow %d", speed);
	}

	if (no_tele_grid) {
		attr = TERM_L_DARK;
		if (!speed) sprintf(buf, "No-Teleport");
	}

	/* remember cursor position */
	Term_locate(&x, &y);

	/* Display the speed */
	c_put_str(attr, format("%-11s", buf), ROW_SPEED, COL_SPEED);

	/* restore cursor position */
	Term_gotoxy(x, y);

	/* hack: remember speed (for extra no-tele warning) */
	p_speed = speed;
}

/*
 * Prints ability to gain skillss
 * Note: Replacing this in 4.4.8.5 by a Blows/Round display, see function below - C. Blue
 */
void prt_study(bool study) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (study)
		put_str("Skill", ROW_STUDY, COL_STUDY);
	else
		put_str("     ", ROW_STUDY, COL_STUDY);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/* Prints blows/round in main window - important and changable stat! - C. Blue */
void prt_bpr(byte bpr, byte attr) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	/* hack: display active wraithform indicator instead */
	if (bpr == 255) c_put_str(attr, "Wraith", ROW_BPR, COL_BPR);
	else c_put_str(attr, format("%2d BpR", bpr), ROW_BPR, COL_BPR);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints cut status
 */
void prt_cut(int cut) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (cut > 1000)
		c_put_str(TERM_L_RED, "Mortal wound", ROW_CUT, COL_CUT);
	else if (cut > 200)
		c_put_str(TERM_RED, "Deep gash  ", ROW_CUT, COL_CUT);
	else if (cut > 100)
		c_put_str(TERM_RED, "Severe cut  ", ROW_CUT, COL_CUT);
	else if (cut > 50)
		c_put_str(TERM_ORANGE, "Nasty cut   ", ROW_CUT, COL_CUT);
	else if (cut > 25)
		c_put_str(TERM_ORANGE, "Bad cut     ", ROW_CUT, COL_CUT);
	else if (cut > 10)
		c_put_str(TERM_YELLOW, "Light cut  ", ROW_CUT, COL_CUT);
	else if (cut)
		c_put_str(TERM_YELLOW, "Graze     ", ROW_CUT, COL_CUT);
	else
		put_str("            ", ROW_CUT, COL_CUT);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints stun status
 */
void prt_stun(int stun) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (stun > 100)
		c_put_str(TERM_RED, "Knocked out ", ROW_STUN, COL_STUN);
	else if (stun > 50)
		c_put_str(TERM_ORANGE, "Heavy stun  ", ROW_STUN, COL_STUN);
	else if (stun)
		c_put_str(TERM_ORANGE, "Stun        ", ROW_STUN, COL_STUN);
	else
		put_str("            ", ROW_STUN, COL_STUN);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints race/class info
 */
void prt_basic(void) {
	cptr r = NULL, c = NULL;

        r = p_ptr->rp_ptr->title;
        c = p_ptr->cp_ptr->title;

	prt_field((char *)r, ROW_RACE, COL_RACE);
	prt_field((char *)c, ROW_CLASS, COL_CLASS);
}

/* Print AFK status */
void prt_AFK(byte afk) {
	int x, y;

	/* Remember AFK status */
	p_ptr->afk = afk;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (afk)
		c_put_str(TERM_ORANGE, "  AFK", ROW_AFK, COL_AFK);
	else
		c_put_str(TERM_ORANGE, "     ", ROW_AFK, COL_AFK);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/* Print encumberment status line */
void prt_encumberment(byte cumber_armor, byte awkward_armor, byte cumber_glove, byte heavy_wield, byte heavy_shield, byte heavy_shoot,
    byte icky_wield, byte awkward_wield, byte easy_wield, byte cumber_weight, byte monk_heavyarmor, byte rogue_heavyarmor, byte awkward_shoot)
{
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	put_str("            ", ROW_CUMBER, COL_CUMBER);
	if (cumber_armor) c_put_str(TERM_UMBER, "(", ROW_CUMBER, COL_CUMBER);
	if (heavy_wield) c_put_str(TERM_RED, "/", ROW_CUMBER, COL_CUMBER + 1);
	if (icky_wield) c_put_str(TERM_ORANGE, "\\", ROW_CUMBER, COL_CUMBER + 2);
	if (awkward_wield) c_put_str(TERM_YELLOW, "/", ROW_CUMBER, COL_CUMBER + 3);
	if (easy_wield) c_put_str(TERM_GREEN, "|", ROW_CUMBER, COL_CUMBER + 3);
	if (heavy_shield) c_put_str(TERM_RED, "[", ROW_CUMBER, COL_CUMBER + 4);
	if (heavy_shoot) c_put_str(TERM_RED, "}", ROW_CUMBER, COL_CUMBER + 5);
	if (awkward_shoot) c_put_str(TERM_YELLOW, "}", ROW_CUMBER, COL_CUMBER + 6);
	if (cumber_weight) c_put_str(TERM_L_RED, "F", ROW_CUMBER, COL_CUMBER + 7);
	if (monk_heavyarmor) c_put_str(TERM_YELLOW, "(", ROW_CUMBER, COL_CUMBER + 8);
	if (rogue_heavyarmor) c_put_str(TERM_BLUE, "(", ROW_CUMBER, COL_CUMBER + 9);
	if (awkward_armor) c_put_str(TERM_VIOLET, "(", ROW_CUMBER, COL_CUMBER + 10);
	if (cumber_glove) c_put_str(TERM_VIOLET, "]", ROW_CUMBER, COL_CUMBER + 11);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

void prt_extra_status(cptr status) {
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (ROW_EXSTA != -1) { /* paranoia: just in case we're a client 
				  without CONDENSED_HP_SP for some odd reason */
		if (!recording_macro)
			c_put_str(TERM_SLATE, status, ROW_EXSTA, COL_EXSTA);
		else
			/* hack: 'abuse' this line to display that we're in recording mode */
			c_put_str(TERM_L_RED, "*RECORDING*", ROW_EXSTA, COL_EXSTA);
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/* Update mini lag-o-meter,
   in a somewhat hacky way to keep track of packet loss:
   If pong doesn't arrive until next Send_ping(),
     assume packet loss.
   When next pong arrives, check previous ping's pong:
     was it missing? -> pretend packet loss _again_(*),
     otherwise display rtt for this pong normally.
   (*) reason is that otherwise lag-o-meter might just
   display a super short flicker of '++++++++++' that
   the user probably can't see.
   Note that this pseudo-packet loss displaying 10 '+'
   is same as if we had 450+ ms lag anyway, so no harm.*/
/* Enable bright red colour for actual packet loss? */
#define BRIGHTRED_PACKETLOSS
void prt_lagometer(int lag) {
	int attr = TERM_L_GREEN;
	int num;
	int x, y;

	/* disable(d)? */
	if (!lagometer_enabled) return;
	if (lag == -1) {
		if (screen_icky) Term_switch(0);
//		Term_putstr(COL_LAG, ROW_LAG, 12, TERM_L_DARK, "[//////////]");
		Term_putstr(COL_LAG, ROW_LAG, 12, TERM_L_DARK, "[----------]");
		if (screen_icky) Term_switch(0);
		return;
	}

#ifdef BRIGHTRED_PACKETLOSS
	y = 0;
	for (x = 0; x < 60; x++)
		if (ping_times[x] == -1) y++;

	/* Latest ping might not be lost yet */
	if (ping_times[0] == -1) y--;

	/* Real packet loss colours bright red */
	if (y) {
		attr = TERM_L_RED;
		num = 10;
	} else {
#endif
		num = lag / 50 + 1;
		if (num > 10) num = 10;

		/* hack: we have previous packet assumed to be lost? */
		if (lag == 9999) num = 10;

		if (num >= 9) attr = TERM_RED;
		else if (num >= 7) attr = TERM_ORANGE;
		else if (num >= 5) attr = TERM_YELLOW;
		else if (num >= 3) attr = TERM_GREEN;
#ifdef BRIGHTRED_PACKETLOSS
	}
#endif

	/* remember cursor position */
	Term_locate(&x, &y);

	if (screen_icky) Term_switch(0);

	/* Default to "unknown" */
	Term_putstr(COL_LAG, ROW_LAG, 12, TERM_L_DARK, "[----------]");
	/* Dump the current "lag" (use '*' symbols) */
	Term_putstr(COL_LAG + 1, ROW_LAG, num, attr, "++++++++++");

	if (screen_icky) Term_switch(0);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Redraw the monster health bar
 */
void health_redraw(int num, byte attr)
{
	int x, y;

	/* Remember monster health */
	mon_health_num = num;
	mon_health_attr = attr;

	/* remember cursor position */
	Term_locate(&x, &y);

	/* Not tracking */
	if (!attr) {
#if 0
		/* Erase the health bar */
		Term_erase(COL_INFO, ROW_INFO, 12);
#else
		/* Draw world coordinates and AFK status from memory */
		prt_depth(p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, depth_town,
		          depth_colour, depth_colour_sector, depth_name);
		prt_AFK(p_ptr->afk);
#endif
	}

	/* Tracking a monster */
	else {
		/* Default to "unknown" */
		Term_putstr(COL_INFO, ROW_INFO, 12, TERM_WHITE, "[----------]");

		/* Dump the current "health" (use '*' symbols) */
		Term_putstr(COL_INFO + 1, ROW_INFO, num, attr, "**********");
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Choice window "shadow" of the "show_inven()" function.
 */
static void display_inven(void) {
	int	i, n, z = 0;
	long int	wgt;

	object_type *o_ptr;

	char	o_name[ONAME_LEN];
	char	tmp_val[80];

	/* Find the "final" slot */
	for (i = 0; i < INVEN_PACK; i++) {
		o_ptr = &inventory[i];

		/* Track non-empty slots */
		if (o_ptr->tval) z = i + 1;
	}

	/* Display the inventory */
	for (i = 0; i < z; i++) {
		o_ptr = &inventory[i];

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Terminate - Term_putstr could otherwise read past the end of the buffer
		 * when it looks for a color code (valgrind complained about that). - mikaelh */
		tmp_val[3] = '\0';

		/* Prepare an "index" */
		tmp_val[0] = index_to_label(i);

		/* Bracket the "index" --(-- */
		tmp_val[1] = ')';

		/* Is this item acceptable? */
		if (item_tester_okay(o_ptr)) {
			/* Display the index */
			Term_putstr(0, i, 3, TERM_WHITE, tmp_val);
		} else {
			/* Grey out the index */
			Term_putstr(0, i, 3, TERM_L_DARK, tmp_val);
		}

		/* Describe the object */
		strcpy(o_name, inventory_name[i]);

		/* Obtain length of description */
		n = strlen(o_name);

#if 1 /* grey out the complete slot if item isn't eligible */
		if (item_tester_okay(o_ptr)) {
			Term_putstr(3, i, n, o_ptr->attr, o_name);
		} else {
			Term_putstr(3, i, n, TERM_L_DARK, o_name);
		}
#else /* only grey out the index */
		/* Clear the line with the (possibly indented) index */
		Term_putstr(3, i, n, o_ptr->attr, o_name);
#endif

		/* Erase the rest of the line */
		Term_erase(3 + n, i, 255);

		/* Display the weight if needed */
		if (c_cfg.show_weights && o_ptr->weight) {
			wgt = o_ptr->weight * o_ptr->number;
			if (wgt < 10000) /* still fitting into 3 digits? */
				(void)sprintf(tmp_val, "%3li.%1li lb", wgt / 10, wgt % 10);
			else
				(void)sprintf(tmp_val, "%3lik%1li lb", wgt / 10000, (wgt % 10000) / 1000);
			Term_putstr(71, i, -1, TERM_WHITE, tmp_val);
		}
	}

	/* Erase the rest of the window */
	for (i = z; i < Term->hgt; i++) {
		/* Erase the line */
		Term_erase(0, i, 255);
	}
}


/*
 * Choice window "shadow" of the "show_equip()" function.
 */
static void display_equip(void) {
	int	i, n;
	long int	wgt;
	object_type *o_ptr;
	char	o_name[ONAME_LEN];
	char	tmp_val[80];

	/* Find the "final" slot */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		o_ptr = &inventory[i];

		/* Start with an empty "index" */
		strcpy(tmp_val, "   ");

		/* Is this item acceptable? */
		if (item_tester_okay(o_ptr)) {
			/* Prepare an "index" */
			tmp_val[0] = index_to_label(i);

			/* Bracket the "index" --(-- */
			tmp_val[1] = ')';
		}

		/* Display the index (or blank space) */
		Term_putstr(0, i - INVEN_WIELD, 3, TERM_WHITE, tmp_val);

		/* Describe the object */
		strcpy(o_name, inventory_name[i]);

		/* Obtain length of the description */
		n = strlen(o_name);

		/* Clear the line with the (possibly indented) index */
		Term_putstr(3, i - INVEN_WIELD, n, o_ptr->attr, o_name);

		/* Erase the rest of the line */
		Term_erase(3 + n, i - INVEN_WIELD, 255);

		/* Display the weight if needed */
		if (c_cfg.show_weights && o_ptr->weight) {
			wgt = o_ptr->weight * o_ptr->number;
			if (wgt < 10000) /* still fitting into 3 digits? */
				(void)sprintf(tmp_val, "%3li.%1li lb", wgt / 10, wgt % 10);
			else
				(void)sprintf(tmp_val, "%3lik%1li lb", wgt / 10000, (wgt % 10000) / 1000);

			/* for 'greyed out' flexibility hack */
			if (o_ptr->attr == TERM_L_DARK)
				Term_putstr(71, i - INVEN_WIELD, -1, TERM_L_DARK, tmp_val);
			else
				Term_putstr(71, i - INVEN_WIELD, -1, TERM_WHITE, tmp_val);
		}
	}

	/* Erase the rest of the window */
	for (i = INVEN_TOTAL - INVEN_WIELD; i < Term->hgt; i++) {
		/* Erase the line */
		Term_erase(0, i, 255);
	}
}


/*
 * Display the inventory.
 *
 * Hack -- do not display "trailing" empty slots
 */
void show_inven(void) {
	int	i, j, k, l, z = 0;
	int	col, len, lim;
	long int wgt, totalwgt = 0;

	object_type *o_ptr;

	char	o_name[ONAME_LEN];
	char	tmp_val[80];

	int	out_index[23];
	byte	out_color[23];
	char	out_desc[23][ONAME_LEN];


	/* Starting column */
	col = command_gap;

	/* Default "max-length" */
	len = 79 - col;

	/* Maximum space allowed for descriptions */
	lim = 79 - 3;

	/* Require space for weight (if needed) */
	lim -= 9;


	/* Find the "final" slot */
	for (i = 0; i < INVEN_PACK; i++) {
		o_ptr = &inventory[i];

		/* Track non-empty slots */
		if (o_ptr->tval) z = i + 1;
	}

	/* Display the inventory */
	for (k = 0, i = 0; i < z; i++) {
		o_ptr = &inventory[i];

		/* Is this item acceptable? */
		if (!item_tester_okay(o_ptr)) continue;

		/* Describe the object */
		strcpy(o_name, inventory_name[i]);

		/* Hack -- enforce max length */
		o_name[lim] = '\0';

		/* Save the object index, color, and descrtiption */
		out_index[k] = i;
		out_color[k] = o_ptr->attr;
		(void)strcpy(out_desc[k], o_name);

		/* Find the predicted "line length" */
		l = strlen(out_desc[k]) + 5;

		/* Be sure to account for the weight */
		l += 9;

		/* Maintain the maximum length */
		if (l > len) len = l;

		/* Advance to next "line" */
		k++;
	}

	/* Find the column to start in */
	col = (len > 76) ? 0 : (79 - len);

	/* Output each entry */
	for (j = 0; j < k; j++) {
		/* Get the index */
		i = out_index[j];

		/* Get the item */
		o_ptr = &inventory[i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		/* Prepare and index --(-- */
		sprintf(tmp_val, "%c)", index_to_label(i));

		/* Clear the line with the (possibly indented) index */
		put_str(tmp_val, j + 1, col);

		/* Display the entry itself */
		c_put_str(out_color[j], out_desc[j], j + 1, col + 3);

		/* Display the weight if needed */
		if (c_cfg.show_weights && o_ptr->weight) {
			wgt = o_ptr->weight * o_ptr->number;
			if (wgt < 10000) /* still fitting into 3 digits? */
				(void)sprintf(tmp_val, "%3li.%1li lb", wgt / 10, wgt % 10);
			else
				(void)sprintf(tmp_val, "%3lik%1li lb", wgt / 10000, (wgt % 10000) / 1000);
			put_str(tmp_val, j + 1, 71);
			totalwgt += wgt;
		}
	}

	/* Display the weight if needed */
	if (c_cfg.show_weights && totalwgt) {
		if (totalwgt < 10000) /* still fitting into 3 digits? */
			(void)sprintf(tmp_val, "Total: %3li.%1li lb", totalwgt / 10, totalwgt % 10);
		else if (totalwgt < 10000000) /* still fitting into 3 digits? */
			(void)sprintf(tmp_val, "Total: %3lik%1li lb", totalwgt / 10000, (totalwgt % 10000) / 1000);
		else
			(void)sprintf(tmp_val, "Total: %3liM%1li lb", totalwgt / 10000000, (totalwgt % 10000000) / 1000000);
		c_put_str(TERM_L_BLUE, tmp_val, 0, 64);
	}

	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < 23)) prt("", j + 1, col ? col - 2 : col);

	/* Notify if inventory is actually empty */
	if (!k) prt("(Your inventory is empty)", 1, 13);

	/* Save the new column */
	command_gap = col;
}


/*
 * Display the equipment.
 */
void show_equip(void) {
	int	i, j, k, l;
	int	col, len, lim;
	long int wgt, totalwgt = 0, armourwgt = 0, shieldwgt = 0;

	object_type *o_ptr;

	char	o_name[ONAME_LEN];
	char	tmp_val[80];

	int	out_index[23];
	byte	out_color[23];
	char	out_desc[23][ONAME_LEN];


	/* Starting column */
	col = command_gap;

	/* Default "max-length" */
	len = 79 - col;

	/* Maximum space allowed for descriptions */
	lim = 79 - 3;

	/* Require space for weight (if needed) */
	lim -= 9;


	/* Scan the equipment list */
	for (k = 0, i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		o_ptr = &inventory[i];

		/* Is this item acceptable? */
		if (!item_tester_okay(o_ptr)) continue;

		/* Describe the object */
		strcpy(o_name, inventory_name[i]);

		/* Hack -- enforce max length */
		o_name[lim] = '\0';

		/* Save the object index, color, and descrtiption */
		out_index[k] = i;
		out_color[k] = o_ptr->attr;
		(void)strcpy(out_desc[k], o_name);

		/* Find the predicted "line length" */
		l = strlen(out_desc[k]) + 5;

		/* Be sure to account for the weight */
		l += 9;

		/* Maintain the maximum length */
		if (l > len) len = l;

		/* Advance to next "line" */
		k++;
	}

	/* Find the column to start in */
	col = (len > 76) ? 0 : (79 - len);

	/* Output each entry */
	for (j = 0; j < k; j++) {
		/* Get the index */
		i = out_index[j];

		/* Get the item */
		o_ptr = &inventory[i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		/* Prepare and index --(-- */
		sprintf(tmp_val, "%c)", index_to_label(i));

		/* Clear the line with the (possibly indented) index */
		put_str(tmp_val, j + 1, col);

		/* Display the entry itself */
		c_put_str(out_color[j], out_desc[j], j + 1, col + 3);

		/* Display the weight if needed */
		if (c_cfg.show_weights && o_ptr->weight) {
			wgt = o_ptr->weight * o_ptr->number;
			if (wgt < 10000) /* still fitting into 3 digits? */
				(void)sprintf(tmp_val, "%3li.%1li lb", wgt / 10, wgt % 10);
			else
				(void)sprintf(tmp_val, "%3lik%1li lb", wgt / 10000, (wgt % 10000) / 1000);

			/* for 'greyed out' flexibility hack */
			if (o_ptr->attr == TERM_L_DARK)
				c_put_str(TERM_L_DARK, tmp_val, j + 1, 71);
			else
				put_str(tmp_val, j + 1, 71);

			totalwgt += wgt;
			if (is_armour(o_ptr->tval)) armourwgt += wgt;
			if (o_ptr->tval == TV_SHIELD) shieldwgt += wgt;
		}
	}

	/* Display the weight if needed */
	if (c_cfg.show_weights && totalwgt) {
		if (totalwgt < 10000) /* still fitting into 3 digits? */
			(void)sprintf(tmp_val, "Total: %3li.%1li lb", totalwgt / 10, totalwgt % 10);
		else if (totalwgt < 10000000) /* still fitting into 3 digits? */
			(void)sprintf(tmp_val, "Total: %3lik%1li lb", totalwgt / 10000, (totalwgt % 10000) / 1000);
		else
			(void)sprintf(tmp_val, "Total: %3liM%1li lb", totalwgt / 10000000, (totalwgt % 10000000) / 1000000);
		c_put_str(TERM_L_BLUE, tmp_val, 0, 64);

		if (!shieldwgt) (void)sprintf(tmp_val, "Armour: %3li.%1li lb", armourwgt / 10, armourwgt % 10);
		else (void)sprintf(tmp_val, "Armour: %3li.%1li lb (%li.%1li lb)", armourwgt / 10, armourwgt % 10, (armourwgt - shieldwgt) / 10, (armourwgt - shieldwgt) % 10);
		c_put_str(TERM_L_BLUE, tmp_val, 0, col);
	}

	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < 23)) prt("", j + 1, col ? col - 2 : col);

	/* Save the new column */
	command_gap = col;
}


/*
 * Display inventory in sub-windows
 */
static void fix_inven(void) {
	int j;

	/* Scan windows */
	for (j = 0; j < ANGBAND_TERM_MAX; j++) {
		term *old = Term;

		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & PW_INVEN)) continue;

		/* Activate */
		Term_activate(ang_term[j]);

		/* Display inventory */
		display_inven();

		/* Fresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}

	if (showing_inven) show_inven();

	/* Assume that this could've been in response to a wield/takeoff/swap command we issued */
	command_confirmed = PKT_UNDEFINED; //..we don't know which one (doesn't matter)
}


/*
 * Display equipment in sub-windows
 */
static void fix_equip(void) {
	int j;

	/* Scan windows */
	for (j = 0; j < ANGBAND_TERM_MAX; j++) {
		term *old = Term;

		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & PW_EQUIP)) continue;

		/* Activate */
		Term_activate(ang_term[j]);

		/* Display inventory */
		display_equip();

		/* Fresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}

	if (showing_equip) {
		item_tester_full = TRUE; //hack for live_timeouts when pressing 'e' to look at equipment!
		show_equip();
		item_tester_full = FALSE; //unhack
	}
}


/*
 * Display character sheet in sub-windows
 */
static void fix_player(void) {
	int j;

	/* Scan windows */
	for (j = 0; j < ANGBAND_TERM_MAX; j++) {
		term *old = Term;

		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & PW_PLAYER)) continue;

		/* Activate */
		Term_activate(ang_term[j]);

		/* Display inventory */
		display_player(csheet_page);

		/* Fresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}
}


/*
 * Hack -- display recent messages in sub-windows
 *
 * XXX XXX XXX Adjust for width and split messages
 */
static void fix_message(void) {
        int j, i;
        int w, h;
        int x, y;

	cptr msg;
	byte a;

	/* Display messages in different colors -Zz */

        /* Scan windows */
        for (j = 0; j < ANGBAND_TERM_MAX; j++) {
                term *old = Term;

                /* No window */
                if (!ang_term[j]) continue;

                /* No relevant flags */
                if (!(window_flag[j] &
		    (PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT))) continue;

                /* Activate */
                Term_activate(ang_term[j]);

                /* Get size */
                Term_get_size(&w, &h);

		/* Does this terminal show the normal message_str buffer? or chat/msgnochat only?*/
		if (window_flag[j] & PW_CHAT) {
	                /* Dump messages */
	                for (i = 0; i < h; i++) {
				a = TERM_WHITE;
				msg = message_str_chat(i);

	                        /* Dump the message on the appropriate line */
	                        Term_putstr(0, (h - 1) - i, -1, a, (char*)msg);

	                        /* Cursor */
	                        Term_locate(&x, &y);

	                        /* Clear to end of line */
	                        Term_erase(x, y, 255);
	                }
		} else if (window_flag[j] & PW_MSGNOCHAT) {
	                /* Dump messages */
	                for (i = 0; i < h; i++) {
				a = TERM_WHITE;
				msg = message_str_msgnochat(i);

	                        /* Dump the message on the appropriate line */
	                        Term_putstr(0, (h - 1) - i, -1, a, (char*)msg);

	                        /* Cursor */
	                        Term_locate(&x, &y);

	                        /* Clear to end of line */
	                        Term_erase(x, y, 255);
	                }
		} else {
	                /* Dump messages */
	                for (i = 0; i < h; i++)
	                {
				a = TERM_WHITE;
				msg = message_str(i);

				/* strip remaining control codes that were left in
				   this main buffer for purpose of CTRL+O/P scrollback
				   control, before actually displaying the message. */
				if (msg[0] == '\376') msg++;

	                        /* Dump the message on the appropriate line */
	                        Term_putstr(0, (h - 1) - i, -1, a, (char*)msg);

	                        /* Cursor */
	                        Term_locate(&x, &y);

	                        /* Clear to end of line */
	                        Term_erase(x, y, 255);
	                }
		}

                /* Fresh */
                Term_fresh();

                /* Restore */
                Term_activate(old);
        }
}

static void fix_lagometer(void) {
	int j;

	/* Scan windows */
	for (j = 0; j < ANGBAND_TERM_MAX; j++) {
		term *old = Term;

		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & PW_LAGOMETER)) continue;

		/* Activate */
		Term_activate(ang_term[j]);

		/* Display lag-o-meter */
		display_lagometer(FALSE);

		/* Fresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}
}

/*
 * Hack -- pass color info around this file
 */
static byte likert_color = TERM_WHITE;


/*
 * Returns a "rating" of x depending on y
 */
static cptr likert(int x, int y, int max) {
	/* Paranoia */
	if (y <= 0) y = 1;

	/* Negative values */
	if (x < 0) {
		likert_color = TERM_RED;
		return ("Very Bad");
	}

	/* Highest possible value reached */
        if ((x >= max) && max) {
                likert_color = TERM_L_UMBER;
                return ("Legendary");
        }

	/* Analyze the value */
	switch (((x * 10) / y)) {
		case 0:
		case 1:
			likert_color = TERM_RED;
			return ("Bad");
		case 2:
			likert_color = TERM_RED;
			return ("Poor");
		case 3:
		case 4:
			likert_color = TERM_YELLOW;
			return ("Fair");
		case 5:
			likert_color = TERM_YELLOW;
			return ("Good");
		case 6:
			likert_color = TERM_YELLOW;
			return ("Very Good");
		case 7:
		case 8:
			likert_color = TERM_L_GREEN;
			return ("Excellent");
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
			likert_color = TERM_L_GREEN;
			return ("Superb");
		case 14:
		case 15:
		case 16:
		case 17:
			likert_color = TERM_L_GREEN;
			return ("Heroic");
		default:
			/* indicate that there is a maximum value */
			if (max) likert_color = TERM_GREEN;
			/* indicate that there is no maximum */
			else likert_color = TERM_L_GREEN;
			return ("Legendary");
	}
}

/*
 * Draws the lag-o-meter.
 */
#define COLOURED_LAGOMETER
void display_lagometer(bool display_commands) {
	int i, cnt, sum, cur, min, max, avg, x, y, packet_loss, height;
	char tmp[80];
	char graph[16][61];
#ifdef COLOURED_LAGOMETER
	byte attr = TERM_VIOLET; /* silyl compiler */
 #if 0 /* use same colours as mini lag-o-meter? */
//	byte attr0 = TERM_L_GREEN, attr1 = TERM_GREEN;
	char attrc0 = 'G', attrc1 = 'g';
 #else /* use white instead of green tones, for being easier on the eye maybe? */
//	byte attr0 = TERM_L_WHITE, attr1 = TERM_SLATE;
	char attrc0 = 'W', attrc1 = 's';
 #endif
	int cur_lag;
	char *underscore, *underscore2, *underscore2a, c_graph[60 * 3 + 1];
	char attrc = TERM_VIOLET; /* silyl compiler */
	char attrc_prev = attrc0; /* assume best case at the top line */
#endif

	/* Clear screen */
	Term_clear();

	/* Why are we here */
	prt("The Lag-o-meter", 1, 30);

	/* Find min and max and calculate avg */
	packet_loss = cnt = sum = 0;
	min = max = -1;
	for (i = 0; i < 60; i++) {
		if (ping_times[i] > 0) {
			if (min == -1) min = ping_times[i];
			else if (ping_times[i] < min) min = ping_times[i];

			if (max == -1) max = ping_times[i];
			else if (ping_times[i] > max) max = ping_times[i];

			cnt++;
			sum += ping_times[i];
		}
		else if (ping_times[i] == -1) {
			packet_loss++;
		}
	}

	if (cnt) avg = sum / cnt;
	else avg = -1;

	/* Latest ping might not be lost yet */
	if (ping_times[0] == -1) {
		packet_loss--;
		cur = ping_times[1];
	}
	else cur = ping_times[0];

	/* Clear the graph */
	for (y = 0; y < 16; y++) {
		for (x = 0; x < 60; x++) {
			graph[y][x] = ' ';
		}
		graph[y][60] = '\0';
	}

	/* Create the graph */
	for (i = 0; i < 60; i++) {
		/* Calculate the height */
		height = (16 * ping_times[i] + max / 2) / max;

		for (y = 0; y < height; y++) {
			graph[15 - y][59 - i] = '*';
		}

		/* Draw an underscore if a reply was received but it wouldn't be visible */
		if (height == 0 && ping_times[i] > 0) {
			graph[15][59 - i] = '_';
		}
	}

	/* Draw the graph */
	for (y = 0; y < 16; y++) {
#ifndef COLOURED_LAGOMETER
		prt(graph[y], 3 + y, 10);
#else
 #if 0 /* doesn't take care of correct '_' colouring */
		cur_lag = (max * (16 - y)) / 16;
		if (cur_lag >= 400) attr = TERM_RED;
		else if (cur_lag >= 300) attr = TERM_ORANGE;
		else if (cur_lag >= 200) attr = TERM_YELLOW;
		else if (cur_lag >= 100) attr = attr1;
		else attr = attr0;
		c_prt(attr, graph[y], 3 + y, 10);
 #else /* does take care */
		memset(c_graph, 0, sizeof(char) * (60 * 3 + 1));
		underscore = underscore2 = graph[y];
		while (underscore < graph[y] + 60) {
			/* handle non-underscore chunk (ie a chunk of asterisks),
			   their colouring can be handled in one go too */
			underscore2 = strchr(underscore, '_');
			if (!underscore2) underscore2 = graph[y] + 60;
			cur_lag = (max * (16 - y)) / 16;
			if (cur_lag >= 400) attrc = 'r';
			else if (cur_lag >= 300) attrc = 'o';
			else if (cur_lag >= 200) attrc = 'y';
			else if (cur_lag >= 100) attrc = attrc1;
			else attrc = attrc0;
			if (underscore2 - underscore) {
				/* avoid 'skipping' a colour level, for better visuals ;) */
				switch (attrc_prev) {
				case 'r':
					if (attrc == 'y' || attrc == attrc1 || attrc == attrc0) attrc = 'o';
					break;
				case 'o':
					if (attrc == attrc1 || attrc == attrc0) attrc = 'y';
					break;
				case 'y':
					if (attrc == attrc0) attrc = attrc1;
					break;
				}

				strcat(c_graph, format("\377%c", attrc));
				strncat(c_graph, underscore, underscore2 - underscore);
			}
			underscore = underscore2;

			/* handle a chunk of underscores,
			   but handle their colouring separately, one by one */
			if (underscore < graph[y] + 60) {
				underscore2 = strchr(underscore, '*');
				underscore2a = strchr(underscore, ' ');
				if (!underscore2) underscore2 = graph[y] + 60;
				if (!underscore2a) underscore2a = graph[y] + 60;
				if (underscore2a < underscore2) underscore2 = underscore2a;
				for (underscore2a = underscore; underscore2a < underscore2; underscore2a++) {
					cur_lag = ping_times[59 - (underscore2a - graph[y])];
					if (cur_lag >= 400) attrc = 'r';
					else if (cur_lag >= 300) attrc = 'o';
					else if (cur_lag >= 200) attrc = 'y';
					else if (cur_lag >= 100) attrc = attrc1;
					else attrc = attrc0;
					strcat(c_graph, format("\377%c%c", attrc, *underscore2a));
				}
				underscore = underscore2a;
			}
		}
		c_put_str(TERM_WHITE, c_graph, 3 + y, 10);
		attrc_prev = attrc;
 #endif
#endif
	}

#ifndef COLOURED_LAGOMETER
	prt("Cur:", 20, 2);
	if (cur != -1) sprintf(tmp, "%5dms", cur);
	else tmp[0] = '\0';
	prt(tmp, 20, 7);

	prt("Avg:", 20, 17);
	if (avg != -1) sprintf(tmp, "%5dms", avg);
	else tmp[0] = '\0';
	prt(tmp, 20, 22);

 #if 0 /* TMI */
	prt("10-min cleaned avg:", 21, 2);
	sprintf(tmp, "%5dms", ping_avg);
	else tmp[0] = '\0';
	prt(tmp, 21, 22);
 #endif

	prt("Min:", 20, 32);
	if (min != -1) sprintf(tmp, "%5dms", min);
	else tmp[0] = '\0';
	prt(tmp, 20, 37);

	prt("Max:", 20, 47);
	if (max != -1) sprintf(tmp, "%5dms", max);
	else tmp[0] = '\0';
	prt(tmp, 20, 52);

	prt("Packet Loss:", 20, 62);
	sprintf(tmp, "%2d", packet_loss);
	prt( tmp, 20, 75);
#else
	prt("Cur:", 20, 2);
	if (cur != -1) {
		sprintf(tmp, "%5dms", cur);
		if (cur >= 400) attr = TERM_RED;
		else if (cur >= 300) attr = TERM_ORANGE;
		else if (cur >= 200) attr = TERM_YELLOW;
		else if (cur >= 100) attr = TERM_GREEN;
		else attr = TERM_L_GREEN;
	}
	else tmp[0] = '\0';
	c_prt(attr, tmp, 20, 7);

	prt("Avg:", 20, 17);
	if (avg != -1) {
		sprintf(tmp, "%5dms", avg);
		if (avg >= 400) attr = TERM_RED;
		else if (avg >= 300) attr = TERM_ORANGE;
		else if (avg >= 200) attr = TERM_YELLOW;
		else if (avg >= 100) attr = TERM_GREEN;
		else attr = TERM_L_GREEN;
	}
	else tmp[0] = '\0';
	c_prt(attr, tmp, 20, 22);

 #if 0 /* TMI */
	prt("10-min cleaned avg:", 21, 2);
	if (ping_avg != -1) {
		sprintf(tmp, "%5dms", ping_avg);
		if (ping_avg >= 400) attr = TERM_RED;
		else if (ping_avg >= 300) attr = TERM_ORANGE;
		else if (ping_avg >= 200) attr = TERM_YELLOW;
		else if (ping_avg >= 100) attr = TERM_GREEN;
		else attr = TERM_L_GREEN;
	}
	else tmp[0] = '\0';
	c_prt(attr, tmp, 21, 22);
 #endif

	prt("Min:", 20, 32);
	if (min != -1) {
		sprintf(tmp, "%5dms", min);
		if (min >= 400) attr = TERM_RED;
		else if (min >= 300) attr = TERM_ORANGE;
		else if (min >= 200) attr = TERM_YELLOW;
		else if (min >= 100) attr = TERM_GREEN;
		else attr = TERM_L_GREEN;
	}
	else tmp[0] = '\0';
	c_prt(attr, tmp, 20, 37);

	prt("Max:", 20, 47);
	if (max != -1) {
		sprintf(tmp, "%5dms", max);
		if (max >= 400) attr = TERM_RED;
		else if (max >= 300) attr = TERM_ORANGE;
		else if (max >= 200) attr = TERM_YELLOW;
		else if (max >= 100) attr = TERM_GREEN;
		else attr = TERM_L_GREEN;
	}
	else tmp[0] = '\0';
	c_prt(attr, tmp, 20, 52);

	prt("Packet Loss:", 20, 62);
	sprintf(tmp, "%2d", packet_loss);
	c_prt(packet_loss ? TERM_RED : TERM_L_GREEN, tmp, 20, 75);
#endif

	if (display_commands) {
#ifndef COLOURED_LAGOMETER
		if (lagometer_enabled) {
			prt("(2) Disable lag-o-meter", 22, 4);
		} else {
			prt("(1) Enable lag-o-meter", 22, 4);
		}
		prt("(c) Clear lag-o-meter", 22, 40);
#else
		if (lagometer_enabled) {
			c_put_str(TERM_WHITE, "(\377U2\377w) Disable lag-o-meter", 22, 4);
		} else {
			c_put_str(TERM_WHITE, "(\377U1\377w) Enable lag-o-meter", 22, 4);
		}
		c_put_str(TERM_WHITE, "(\377Uc\377w) Clear lag-o-meter", 22, 40);
#endif
//		prt("Command: ", 23, 2);
	}
}

/*
 * Update the lag-o-meter if it's open.
 */
void update_lagometer() {
	if (lagometer_open) {
		display_lagometer(TRUE);

		/* Fresh in case there's no net input */
		Term_fresh();
	}

	/* Update any other windows */
	p_ptr->window |= PW_LAGOMETER;
}

void display_player(int hist) {
	int i;
	char buf[80];
	cptr desc;

	if (hist != 2) {
#ifndef NEW_COMPRESSED_DUMP_AC
		int y_row1 = 1, y_row2 = 8, y_row3 = 15, y_rowmode = y_row1 + 5;
#else
		int y_row1 = 1, y_row2 = 9, y_row3 = 15, y_rowmode = y_row1 + 6;
#endif

		/* Clear screen */
		clear_from(0);

		/* Name, Sex, Race, Class */
		put_str("Name        :", y_row1, 1);
		put_str("Sex         :", y_row1 + 1, 1);
		put_str("Race        :", y_row1 + 2, 1);
		put_str("Class       :", y_row1 + 3, 1);
		put_str("Body        :", y_row1 + 4, 1);
#ifdef NEW_COMPRESSED_DUMP_AC
 #ifdef HIDE_UNAVAILABLE_TRAIT
		if (trait != 0)
 #endif
		{
			put_str("Trait       :", y_row1 + 5, 1);
			c_put_str(TERM_L_BLUE, trait_info[trait].title, y_row1 + 5, 15);
		}
#endif
		put_str("Mode        :", y_rowmode, 1);

		c_put_str(TERM_L_BLUE, cname, y_row1, 15);
		c_put_str(TERM_L_BLUE, (p_ptr->male ? "Male" : "Female"), y_row1 + 1, 15);
		c_put_str(TERM_L_BLUE, race_info[race].title, y_row1 + 2, 15);
		c_put_str(TERM_L_BLUE, class_info[class].title, y_row1 + 3, 15);
		c_put_str(TERM_L_BLUE, c_p_ptr->body_name, y_row1 + 4, 15);
		if (p_ptr->mode & MODE_PVP)
			c_put_str(TERM_L_BLUE, "PvP (one life)", y_rowmode, 15);
		else if (p_ptr->mode & MODE_EVERLASTING)
			c_put_str(TERM_L_BLUE, "Everlasting (infinite lives)", y_rowmode, 15);
		else if ((p_ptr->mode & MODE_NO_GHOST) && (p_ptr->mode & MODE_HARD))
			c_put_str(TERM_L_BLUE, "Hellish (one life, extra hard)", y_rowmode, 15);
		else if (p_ptr->mode & MODE_NO_GHOST)
			c_put_str(TERM_L_BLUE, "Unworldly (one life)", y_rowmode, 15);
		else if (p_ptr->mode & MODE_HARD)
			c_put_str(TERM_L_BLUE, "Hard (3 lives, extra hard)", y_rowmode, 15);
		else /*(p_ptr->mode == MODE_NORMAL)*/
			c_put_str(TERM_L_BLUE, "Normal (3 lives)", y_rowmode, 15);

		/* Age, Height, Weight, Social */
		prt_num("Age          ", (int)p_ptr->age, y_row1, 32, TERM_L_BLUE);
		prt_num("Height       ", (int)p_ptr->ht, y_row1 + 1, 32, TERM_L_BLUE);
		prt_num("Weight       ", (int)p_ptr->wt, y_row1 + 2, 32, TERM_L_BLUE);
		prt_num("Social Class ", (int)p_ptr->sc, y_row1 + 3, 32, TERM_L_BLUE);

		/* Display the stats */
		for (i = 0; i < 6; i++) {
			/* Special treatment of "injured" stats */
			if (p_ptr->stat_use[i] < p_ptr->stat_top[i]){
				int value;

				/* Use lowercase stat name */
				put_str(stat_names_reduced[i], y_row1 + i, 61);

				/* Get the current stat */
				value = p_ptr->stat_use[i];

				/* Obtain the current stat (modified) */
				cnv_stat(value, buf);

				/* Display the current stat (modified) */
				c_put_str(TERM_YELLOW, buf, y_row1 + i, 66);

				/* Acquire the max stat */
				value = p_ptr->stat_top[i];

				/* Obtain the maximum stat (modified) */
				cnv_stat(value, buf);

				/* Display the maximum stat (modified) */
				if(p_ptr->stat_max[i] < (18 + 100))
					c_put_str(TERM_L_GREEN, buf, y_row1 + i, 73);
				else c_put_str(TERM_L_UMBER, buf, y_row1 + i, 73);
			}

			/* Normal treatment of "normal" stats */
			else {
				/* Assume uppercase stat name */
				put_str(stat_names[i], y_row1 + i, 61);

				/* Obtain the current stat (modified) */
				cnv_stat(p_ptr->stat_use[i], buf);

				/* Display the current stat (modified) */
				if (p_ptr->stat_max[i] < (18 + 100)) c_put_str(TERM_L_GREEN, buf, y_row1 + i, 66);
				else c_put_str(TERM_L_UMBER, buf, 1 + i, 66);
			}
		}

		/* Check for history */
		if (hist) {
#ifndef NEW_COMPRESSED_DUMP
			put_str("(Character Background)", y_row3 - 1, 25);
#endif
			for (i = 0; i < 4; i++)
				c_put_str(TERM_SLATE, p_ptr->history[i], y_row3 + i, 10);
		} else {
#ifndef NEW_COMPRESSED_DUMP
			put_str("(Miscellaneous Abilities)", y_row3 - 1, 25);
#endif

			/* Display "skills" */
			put_str("Fighting    :", y_row3, 1);
			desc = likert(p_ptr->skill_thn, 120, 0);
			c_put_str(likert_color, desc, y_row3, 15);

			put_str("Bows/Throw  :", y_row3 + 1, 1);
			desc = likert(p_ptr->skill_thb, 120, 0);
			c_put_str(likert_color, desc, y_row3 + 1, 15);

			put_str("Saving Throw:", y_row3 + 2, 1);
			desc = likert(p_ptr->skill_sav, 52, 95);	/*was 6.0 before x10 increase */
			c_put_str(likert_color, desc, y_row3 + 2, 15);

			put_str("Stealth     :", y_row3 + 3, 1);
			desc = likert(p_ptr->skill_stl, 10, 30);
			c_put_str(likert_color, desc, y_row3 + 3, 15);


			put_str("Perception  :", y_row3, 28);
			desc = likert(p_ptr->skill_fos, 40, 75);
			c_put_str(likert_color, desc, y_row3, 42);

			put_str("Searching   :", y_row3 + 1, 28);
			desc = likert(p_ptr->skill_srh, 60, 100);
			c_put_str(likert_color, desc, y_row3 + 1, 42);

			put_str("Disarming   :", y_row3 + 2, 28);
			desc = likert(p_ptr->skill_dis, 80, 100);
			c_put_str(likert_color, desc, y_row3 + 2, 42);

			put_str("Magic Device:", y_row3 + 3, 28);
			//desc = likert(p_ptr->skill_dev, 60, 0);
			//desc = likert(p_ptr->skill_dev, 80, 0);
			//about 161 is max mdev reachable atm (2014/07/24)
			desc = likert((p_ptr->skill_dev * (p_ptr->skill_dev + 80)) / 110, 100, 0);
			c_put_str(likert_color, desc, y_row3 + 3, 42);


			put_str("Blows/Round:", y_row3, 55);
			c_put_str(TERM_L_BLUE, format("%d", p_ptr->num_blow), y_row3, 69);

			put_str("Shots/Round:", y_row3 + 1, 55);
			c_put_str(TERM_L_BLUE, format("%d", p_ptr->num_fire), y_row3 + 1, 69);
#if 0
			put_str("Spells/Round:", y_row3 + 2, 55);
			c_put_str(TERM_L_BLUE, format("%d", p_ptr->num_spell), y_row3 + 2, 69);
#endif
			put_str("Infra-Vision:", y_row3 + 3, 55);
			c_put_str(TERM_L_BLUE, format("%d feet", p_ptr->see_infra * 10), y_row3 + 3, 69);
		}

		/* Dump the bonuses to hit/dam */
		prt_num("+To MHit    ", p_ptr->dis_to_h + p_ptr->to_h_melee, y_row2, 1, TERM_L_BLUE);
		prt_num("+To MDamage ", p_ptr->dis_to_d + p_ptr->to_d_melee, y_row2 + 1, 1, TERM_L_BLUE);
		prt_num("+To RHit    ", p_ptr->dis_to_h + p_ptr->to_h_ranged, y_row2 + 2, 1, TERM_L_BLUE);
		prt_num("+To RDamage ", p_ptr->to_d_ranged, y_row2 + 3, 1, TERM_L_BLUE);
#ifndef NEW_COMPRESSED_DUMP_AC
		/* Dump the armor class bonus */
		prt_num("+ To AC     ", p_ptr->dis_to_a, y_row2 + 4, 1, TERM_L_BLUE);
		/* Dump the total armor class */
		prt_num("  Base AC   ", p_ptr->dis_ac, y_row2 + 5, 1, TERM_L_BLUE);
#else
		prt_num("Total AC    ", p_ptr->dis_to_a + p_ptr->dis_ac, y_row2 + 4, 1, TERM_L_BLUE);
#endif

		prt_num("Level      ", (int)p_ptr->lev, y_row2, 28, TERM_L_GREEN);
		if (p_ptr->exp >= p_ptr->max_exp)
			prt_lnum("Experience ", p_ptr->exp, y_row2 + 1, 28, TERM_L_GREEN);
		else
			prt_lnum("Experience ", p_ptr->exp, y_row2 + 1, 28, TERM_YELLOW);
		prt_lnum("Max Exp    ", p_ptr->max_exp, y_row2 + 2, 28, TERM_L_GREEN);
		if (p_ptr->lev >= PY_MAX_LEVEL || !exp_adv) {
			put_str("Exp to Adv.", y_row2 + 3, 28);
			c_put_str(TERM_L_GREEN, "    *****", y_row2 + 3, 28 + 11);
		} else {
			prt_lnum("Exp to Adv.", exp_adv, y_row2 + 3, 28, TERM_L_GREEN);
		}
		prt_lnum("Gold       ", p_ptr->au, y_row2 + 4, 28, TERM_L_GREEN);
#ifndef NEW_COMPRESSED_DUMP_AC
 #ifdef HIDE_UNAVAILABLE_TRAIT
		if (trait != 0)
 #endif
		{
			put_str("Trait", y_row2 + 5, 28);
			c_put_str(TERM_L_BLUE, trait_info[trait].title, y_row2 + 5, 39);
		}
#endif

#ifndef NEW_COMPRESSED_DUMP
		if (p_ptr->mhp == -9999) {
			put_str("Max Hit Points         ", y_row2, 52);
			c_put_str(TERM_L_GREEN, "-", y_row2, 75);
		} else
			prt_num("Max Hit Points ", p_ptr->mhp, y_row2, 52, TERM_L_GREEN);

		if (p_ptr->chp == -9999) {
			put_str("Cur Hit Points         ", y_row2 + 1, 52);
			c_put_str(TERM_L_GREEN, "-", y_row2 + 1, 75);
		} else if (p_ptr->chp >= p_ptr->mhp)
			prt_num("Cur Hit Points ", p_ptr->chp, y_row2 + 1, 52, TERM_L_GREEN);
		else if (p_ptr->chp > (p_ptr->mhp) / 10)
			prt_num("Cur Hit Points ", p_ptr->chp, y_row2 + 1, 52, TERM_YELLOW);
		else
			prt_num("Cur Hit Points ", p_ptr->chp, y_row2 + 1, 52, TERM_RED);

		if (p_ptr->msp == -9999) {
			put_str("Max MP (Mana)          ", y_row2 + 2, 52);
			c_put_str(TERM_L_GREEN, "-", y_row2 + 2, 75);
		} else
			prt_num("Max MP (Mana)  ", p_ptr->msp, y_row2 + 2, 52, TERM_L_GREEN);

		if (p_ptr->csp == -9999) {
			put_str("Cur MP (Mana)          ", y_row2 + 3, 52);
			c_put_str(TERM_L_GREEN, "-", y_row2 + 3, 75);
		} else if (p_ptr->csp >= p_ptr->msp)
			prt_num("Cur MP (Mana)  ", p_ptr->csp, y_row2 + 3, 52, TERM_L_GREEN);
		else if (p_ptr->csp > (p_ptr->msp) / 10)
			prt_num("Cur MP (Mana)  ", p_ptr->csp, y_row2 + 3, 52, TERM_YELLOW);
		else
			prt_num("Cur MP (Mana)  ", p_ptr->csp, y_row2 + 3, 52, TERM_RED);
 #ifdef SHOW_SANITY
		put_str("Cur Sanity", y_row2 + 4, 52);

		c_put_str(c_p_ptr->sanity_attr, c_p_ptr->sanity, y_row2 + 4, 67);
 #endif	/* SHOW_SANITY */

#else

		if (p_ptr->mhp == -9999) {
			put_str("Hit Points", y_row2, 52);
			c_put_str(TERM_L_GREEN, "-", y_row2, 71);
		} else {
			prt_num("Hit Points     ", p_ptr->mhp, y_row2, 52, TERM_L_GREEN);
			    c_put_str(TERM_L_GREEN, "/", y_row2, 71);
			if (p_ptr->chp >= p_ptr->mhp)
				prt_num("", p_ptr->chp, y_row2, 62, TERM_L_GREEN);
			else if (p_ptr->chp > (p_ptr->mhp) / 10)
				prt_num("", p_ptr->chp, y_row2, 62, TERM_YELLOW);
			else
				prt_num("", p_ptr->chp, y_row2, 62, TERM_RED);
		}

		if (p_ptr->msp == -9999) {
			put_str("MP (Mana)", y_row2 + 1, 52);
			c_put_str(TERM_L_GREEN, "-", y_row2 + 1, 71);
		} else {
			prt_num("MP (Mana)      ", p_ptr->msp, y_row2 + 1, 52, TERM_L_GREEN);
			c_put_str(TERM_L_GREEN, "/", y_row2 + 1, 71);
			if (p_ptr->csp >= p_ptr->msp)
				prt_num("", p_ptr->csp, y_row2 + 1, 62, TERM_L_GREEN);
			else if (p_ptr->csp > (p_ptr->msp) / 10)
				prt_num("", p_ptr->csp, y_row2 + 1, 62, TERM_YELLOW);
			else
				prt_num("", p_ptr->csp, y_row2 + 1, 62, TERM_RED);
		}

 #ifdef SHOW_SANITY
		put_str("Sanity", y_row2 + 2, 52);
		c_put_str(c_p_ptr->sanity_attr, c_p_ptr->sanity, y_row2 + 2, 67);
 #endif	/* SHOW_SANITY */

		/* Display 'WINNER' status */
		put_str("Status", y_row2 + 4, 52);
		if (p_ptr->chp < 0 || c_p_ptr->sanity_attr == TERM_RED) c_put_str(TERM_L_DARK, "        DEAD", y_row2 + 4, 64);
		else if (p_ptr->ghost) c_put_str(TERM_RED, "Ghost (dead)", y_row2 + 4, 64);
		else if (p_ptr->total_winner) c_put_str(TERM_VIOLET, "***WINNER***", y_row2 + 4, 64);
		else c_put_str(TERM_L_GREEN, "       Alive", y_row2 + 4, 64);
#endif

		/* Show location (better description needed XXX) */
		if (c_cfg.depth_in_feet) {
			if (location_name2[0])
				put_str(format("You are at %s%d ft%s %s %s.",
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz * 50,
					p_ptr->wpos.wz ? "" : ")",
					location_pre, location_name2), 20, 10);//hist ? 10 : 1);
			else if (p_ptr->wpos.wx == 127)
				put_str(format("You are at %s%d ft%s of an unknown region.",
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz * 50,
					p_ptr->wpos.wz ? "" : ")"), 20, 10);//hist ? 10 : 1);
			else
				put_str(format("You are at %s%d ft%s of world map sector %d,%d.",
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz * 50,
					p_ptr->wpos.wz ? "" : ")",
					p_ptr->wpos.wx, p_ptr->wpos.wy), 20, 10);//hist ? 10 : 1);
		} else {
			if (location_name2[0])
				put_str(format("You are at %slevel %d%s %s %s.",
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz,
					p_ptr->wpos.wz ? "" : ")",
					location_pre, location_name2), 20, 10);//hist ? 10 : 1);
			else if (p_ptr->wpos.wx == 127)
				put_str(format("You are at %slevel %d%s of an unknown region.",
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz,
					p_ptr->wpos.wz ? "" : ")"), 20, 10);//hist ? 10 : 1);
			else
				put_str(format("You are at %slevel %d%s of world map sector %d,%d.",
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz,
					p_ptr->wpos.wz ? "" : ")",
					p_ptr->wpos.wx, p_ptr->wpos.wy), 20, 10);//hist ? 10 : 1);
		}
	} else { //Character sheet boni page, finally!
		int i, j, k, amfi1 = 0, amfi2 = 0;
		int header_color[4][19];

		int color;
		char symbol;
		char tmp[10];
		byte amfi_sum = 0, luck_sum = 0, life_sum = 0;

		/* Clear screen */
		clear_from(0);

		for (i = 0; i < 4; i++)
			for (j = 0; j < 19; j++)
				header_color[i][j] = TERM_L_DARK;

		/* Fill Columns */
		for (i = 0; i < 15; i++) {
			/* Header */
			color = TERM_WHITE;
			if (csheet_boni[i].cb[11] & CB12_XHIDD) color = TERM_YELLOW;
			if (csheet_boni[i].cb[12] & CB13_XSTAR) color = TERM_L_UMBER;
			if (csheet_boni[i].cb[12] & CB13_XCRSE) color = TERM_RED;
			if (csheet_boni[i].cb[12] & CB13_XNOEQ) color = TERM_L_DARK;
			if (i == 14) color = (p_ptr->ghost) ? TERM_L_DARK : (p_ptr->mode & MODE_FRUIT_BAT) ? TERM_ORANGE : TERM_WHITE;
			for (j = 0; j < 5; j++) {
				switch (i) {
				case 0: { c_put_str(color, "a", 0, 5 + j * 20 + i); break; }
				case 1: { c_put_str(color, "b", 0, 5 + j * 20 + i); break; }
				case 2: { c_put_str(color, "c", 0, 5 + j * 20 + i); break; }
				case 3: { c_put_str(color, "d", 0, 5 + j * 20 + i); break; }
				case 4: { c_put_str(color, "e", 0, 5 + j * 20 + i); break; }
				case 5: { c_put_str(color, "f", 0, 5 + j * 20 + i); break; }
				case 6: { c_put_str(color, "g", 0, 5 + j * 20 + i); break; }
				case 7: { c_put_str(color, "h", 0, 5 + j * 20 + i); break; }
				case 8: { c_put_str(color, "i", 0, 5 + j * 20 + i); break; }
				case 9: { c_put_str(color, "j", 0, 5 + j * 20 + i); break; }
				case 10: { c_put_str(color, "k", 0, 5 + j * 20 + i); break; }
				case 11: { c_put_str(color, "l", 0, 5 + j * 20 + i); break; }
				case 12: { c_put_str(color, "m", 0, 5 + j * 20 + i); break; }
				case 13: { c_put_str(color, "n", 0, 5 + j * 20 + i); break; }
				case 14: { c_put_str(color, (p_ptr->mode & MODE_FRUIT_BAT) ? "b" : "@", 0, 5 + j * 20 + i); break; }
				}
			}

			/* Blank Fill */
			color = TERM_SLATE;
			if (csheet_boni[i].cb[12] & CB13_XNOEQ) color = TERM_L_DARK;
			if (csheet_boni[i].cb[11] & CB12_XHIDD) color = TERM_YELLOW;
			for (j = 0; j < 5; j++) {
				for (k = 1; k < 20; k++)
					c_put_str(color, ".", k, 5 + j * 20 + i);
			}
			if (csheet_boni[i].cb[12] & CB13_XNOEQ) continue;

			/* Fill with boni values and track header colour. Gross hardcode! - Kurzel */
			if (csheet_boni[i].cb[0] & CB1_SFIRE) { c_put_str(TERM_RED, "-", 1, 5 + i); if (header_color[0][0] == TERM_L_DARK) header_color[0][0] = TERM_RED; }
			if (csheet_boni[i].cb[0] & CB1_RFIRE) { c_put_str(TERM_WHITE, "+", 1, 5 + i); if (header_color[0][0] == TERM_L_DARK || header_color[0][0] == TERM_RED) header_color[0][0] = TERM_WHITE; }
			if (csheet_boni[i].cb[0] & CB1_IFIRE) { c_put_str(TERM_L_UMBER, "*", 1, 5 + i); if (header_color[0][0] != TERM_L_UMBER) header_color[0][0] = TERM_L_UMBER; }
			if (csheet_boni[i].cb[0] & CB1_SCOLD) { c_put_str(TERM_RED, "-", 2, 5 + i); if (header_color[0][1] == TERM_L_DARK) header_color[0][1] = TERM_RED; }
			if (csheet_boni[i].cb[0] & CB1_RCOLD) { c_put_str(TERM_WHITE, "+", 2, 5 + i); if (header_color[0][1] == TERM_L_DARK || header_color[0][1] == TERM_RED) header_color[0][1] = TERM_WHITE; }
			if (csheet_boni[i].cb[0] & CB1_ICOLD) { c_put_str(TERM_L_UMBER, "*", 2, 5 + i); if (header_color[0][1] != TERM_L_UMBER) header_color[0][1] = TERM_L_UMBER; }
			if (csheet_boni[i].cb[0] & CB1_SELEC) { c_put_str(TERM_RED, "-", 3, 5 + i); if (header_color[0][2] == TERM_L_DARK) header_color[0][2] = TERM_RED; }
			if (csheet_boni[i].cb[0] & CB1_RELEC) { c_put_str(TERM_WHITE, "+", 3, 5 + i); if (header_color[0][2] == TERM_L_DARK || header_color[0][2] == TERM_RED) header_color[0][2] = TERM_WHITE; }
			if (csheet_boni[i].cb[1] & CB2_IELEC) { c_put_str(TERM_L_UMBER, "*", 3, 5 + i); if (header_color[0][2] != TERM_L_UMBER) header_color[0][2] = TERM_L_UMBER; }
			if (csheet_boni[i].cb[1] & CB2_SACID) { c_put_str(TERM_RED, "-", 4, 5 + i); if (header_color[0][3] == TERM_L_DARK) header_color[0][3] = TERM_RED; }
			if (csheet_boni[i].cb[1] & CB2_RACID) { c_put_str(TERM_WHITE, "+", 4, 5 + i); if (header_color[0][3] == TERM_L_DARK || header_color[0][3] == TERM_RED) header_color[0][3] = TERM_WHITE; }
			if (csheet_boni[i].cb[1] & CB2_IACID) { c_put_str(TERM_L_UMBER, "*", 4, 5 + i); if (header_color[0][3] != TERM_L_UMBER) header_color[0][3] = TERM_L_UMBER; }
			if (csheet_boni[i].cb[1] & CB2_SPOIS) { c_put_str(TERM_RED, "-", 5, 5 + i); if (header_color[0][4] == TERM_L_DARK) header_color[0][4] = TERM_RED; }
			if (csheet_boni[i].cb[1] & CB2_RPOIS) { c_put_str(TERM_WHITE, "+", 5, 5 + i); if (header_color[0][4] == TERM_L_DARK || header_color[0][4] == TERM_RED) header_color[0][4] = TERM_WHITE; }
			if (csheet_boni[i].cb[1] & CB2_IPOIS) { c_put_str(TERM_L_UMBER, "*", 5, 5 + i); if (header_color[0][4] != TERM_L_UMBER) header_color[0][4] = TERM_L_UMBER; }
			if (csheet_boni[i].cb[1] & CB2_RBLND) { c_put_str(TERM_WHITE, "+", 6, 5 + i); header_color[0][5] = TERM_WHITE; }
			if (csheet_boni[i].cb[2] & CB3_SLITE) { c_put_str(TERM_RED, "-", 7, 5 + i); if (header_color[0][6] == TERM_L_DARK) header_color[0][6] = TERM_RED; }
			if (csheet_boni[i].cb[2] & CB3_RLITE) { c_put_str(TERM_WHITE, "+", 7, 5 + i); if (header_color[0][6] == TERM_L_DARK || header_color[0][6] == TERM_RED) header_color[0][6] = TERM_WHITE; }
			if (csheet_boni[i].cb[2] & CB3_RDARK) { c_put_str(TERM_WHITE, "+", 8, 5 + i); if (header_color[0][7] == TERM_L_DARK) header_color[0][7] = TERM_WHITE; }
			if (csheet_boni[i].cb[2] & CB3_RSOUN) { c_put_str(TERM_WHITE, "+", 9, 5 + i); header_color[0][8] = TERM_WHITE; }
			if (csheet_boni[i].cb[12] & CB13_XNCUT) c_put_str(TERM_WHITE, "s", 10, 5 + i);
			if (csheet_boni[i].cb[2] & CB3_RSHRD) { c_put_str(TERM_WHITE, "+", 10, 5 + i); header_color[0][9] = TERM_WHITE; }
			if ((csheet_boni[i].cb[2] & CB3_RSHRD) && (csheet_boni[i].cb[12] & CB13_XNCUT)) c_put_str(TERM_WHITE, "*", 10, 5 + i);
			if (csheet_boni[i].cb[2] & CB3_RNEXU) { c_put_str(TERM_WHITE, "+", 11, 5 + i); header_color[0][10] = TERM_WHITE; }
			if (csheet_boni[i].cb[2] & CB3_RNETH) { c_put_str(TERM_WHITE, "+", 12, 5 + i); if (header_color[0][11] == TERM_L_DARK) header_color[0][11] = TERM_WHITE; }
			if (csheet_boni[i].cb[2] & CB3_INETH) { c_put_str(TERM_L_UMBER, "*", 12, 5 + i); if (header_color[0][11] != TERM_L_UMBER) header_color[0][11] = TERM_L_UMBER; }
			if (csheet_boni[i].cb[3] & CB4_RCONF) { c_put_str(TERM_WHITE, "+", 13, 5 + i); if (header_color[0][12] == TERM_L_DARK || header_color[0][12] == TERM_YELLOW) header_color[0][12] = TERM_WHITE; }
			if (csheet_boni[i].cb[3] & CB4_RCHAO) {
				c_put_str(TERM_WHITE, "+", 14, 5 + i); header_color[0][13] = TERM_WHITE;
				//Chaos covers confusion for whatever reason, maybe confusion breathers should breathe psi damage instead, and confusion should become non-damaging? - Kurzel
				//Could replace br_conf with a psi attack for the affected mobs, breathing psi doesn't make any sense either, some mobs could be removed even. - Kurzel
				//Bronze D / draconians could get gravity/lightning to be consistent with other metallic dragon abilities/lore? Same colour as gravity Z now anyway. - Kurzel
				if (!(csheet_boni[i].cb[3] & CB4_RCONF)) { c_put_str(TERM_YELLOW, "+", 13, 5 + i); if (header_color[0][12] == TERM_L_DARK) header_color[0][12] = TERM_YELLOW; }
			}
			if (csheet_boni[i].cb[3] & CB4_RDISE) { c_put_str(TERM_WHITE, "+", 15, 5 + i); header_color[0][14] = TERM_WHITE; }
			if (csheet_boni[i].cb[3] & CB4_RWATR) { c_put_str(TERM_WHITE, "+", 16, 5 + i); if (header_color[0][15] == TERM_L_DARK) header_color[0][15] = TERM_WHITE; }
			if (csheet_boni[i].cb[3] & CB4_IWATR) { c_put_str(TERM_L_UMBER, "*", 16, 5 + i); if (header_color[0][15] != TERM_L_UMBER) header_color[0][15] = TERM_L_UMBER; }
			if (csheet_boni[i].cb[3] & CB4_RTIME) { c_put_str(TERM_WHITE, "+", 17, 5 + i); header_color[0][16] = TERM_WHITE; }
			if (csheet_boni[i].cb[3] & CB4_RMANA) { c_put_str(TERM_WHITE, "+", 18, 5 + i); header_color[0][17] = TERM_WHITE; }
			if (csheet_boni[i].cb[3] & CB4_RMIND) { c_put_str(TERM_WHITE, "+", 19, 5 + i); header_color[0][18] = TERM_WHITE; }
			if (csheet_boni[i].cb[4] & CB5_XMIND) { c_put_str(TERM_WHITE, "*", 19, 5 + i); header_color[0][18] = TERM_WHITE; } //Similar to no_cut, for now? Level 2~ - Kurzel

			if (csheet_boni[i].cb[4] & CB5_RFEAR) { c_put_str(TERM_WHITE, "+", 1, 25 + i); header_color[1][0] = TERM_WHITE; }
			if (csheet_boni[i].cb[4] & CB5_RPARA) { c_put_str(TERM_WHITE, "+", 2, 25 + i); header_color[1][1] = TERM_WHITE; }
			if (csheet_boni[i].cb[4] & CB5_STELE) { c_put_str(TERM_RED, "-", 3, 25 + i); if (header_color[1][2] == TERM_L_DARK || header_color[1][2] == TERM_WHITE) header_color[1][2] = TERM_RED; } //overrides the resist, no items with both
			if (csheet_boni[i].cb[4] & CB5_RTELE) { c_put_str(TERM_WHITE, "+", 3, 25 + i); if (header_color[1][2] == TERM_L_DARK) header_color[1][2] = TERM_WHITE; }
			if (csheet_boni[i].cb[4] & CB5_ITELE) { c_put_str(TERM_YELLOW, "*", 3, 25 + i); if (header_color[1][2] != TERM_YELLOW) header_color[1][2] = TERM_YELLOW; }
			if (csheet_boni[i].cb[4] & CB5_RSINV) { c_put_str(TERM_WHITE, "+", 4, 25 + i); if (header_color[1][3] == TERM_L_DARK) header_color[1][3] = TERM_WHITE; }
			if (csheet_boni[i].cb[4] & CB5_RINVS) { c_put_str(TERM_WHITE, "+", 5, 25 + i); header_color[1][4] = TERM_WHITE; }
			if (csheet_boni[i].cb[5] & CB6_RLIFE) { c_put_str(TERM_WHITE, "+", 6, 25 + i); header_color[1][5] = TERM_WHITE; }
			if (csheet_boni[i].cb[13] & CB14_ILIFE) { c_put_str(TERM_L_UMBER, "*", 6, 25 + i); header_color[1][5] = TERM_L_UMBER; }
			if (csheet_boni[i].cb[5] & CB6_RWRTH) { c_put_str(TERM_WHITE, "+", 7, 25 + i); header_color[1][6] = TERM_WHITE; }
			if (csheet_boni[i].cb[5] & CB6_RFFAL) { c_put_str(TERM_WHITE, "+", 8, 25 + i); header_color[1][7] = TERM_WHITE; }
			if (csheet_boni[i].cb[12] & CB13_XSWIM) { c_put_str(TERM_L_BLUE, "~", 9, 25 + i); if (header_color[1][8] == TERM_L_DARK) header_color[1][8] = TERM_YELLOW; }
			if (csheet_boni[i].cb[12] & CB13_XTREE) { c_put_str(TERM_GREEN, "#", 9, 25 + i); if (header_color[1][8] == TERM_L_DARK) header_color[1][8] = TERM_YELLOW; }
			if ((csheet_boni[i].cb[12] & CB13_XTREE) && (csheet_boni[i].cb[12] & CB13_XSWIM)) c_put_str(TERM_YELLOW, "+", 9, 25 + i); //Almost flying~
			if (csheet_boni[i].cb[5] & CB6_RLVTN) {
				c_put_str(TERM_WHITE, "+", 9, 25 + i); header_color[1][8] = TERM_WHITE;
				//Levitation covers feather falling, similar to chaos/confusion but plausible? - Kurzel
				if (!(csheet_boni[i].cb[5] & CB6_RFFAL)) { c_put_str(TERM_YELLOW, "+", 8, 25 + i); if (header_color[1][7] == TERM_L_DARK) header_color[1][7] = TERM_YELLOW; }
			}
			if (csheet_boni[i].cb[5] & CB6_RCLMB) { c_put_str(TERM_WHITE, "+", 10, 25 + i); header_color[1][9] = TERM_WHITE; }
			if (csheet_boni[i].cb[5] & CB6_SRGHP) { c_put_str(TERM_RED, "-", 11, 25 + i); if (header_color[1][10] == TERM_L_DARK) header_color[1][10] = TERM_RED; }
			if (csheet_boni[i].cb[5] & CB6_RRGHP) { c_put_str(TERM_WHITE, "+", 11, 25 + i); if (header_color[1][10] == TERM_L_DARK) header_color[1][10] = TERM_WHITE; if (header_color[1][10] == TERM_RED || header_color[1][10] == TERM_YELLOW) header_color[1][10] = TERM_YELLOW; } //yellow if both...
			if (csheet_boni[i].cb[5] & CB6_SRGMP) { c_put_str(TERM_RED, "-", 12, 25 + i); if (header_color[1][11] == TERM_L_DARK) header_color[1][11] = TERM_RED; }
			if (csheet_boni[i].cb[6] & CB7_RRGMP) { c_put_str(TERM_WHITE, "+", 12, 25 + i); if (header_color[1][11] == TERM_L_DARK) header_color[1][11] = TERM_WHITE; if (header_color[1][11] == TERM_RED || header_color[1][11] == TERM_YELLOW) header_color[1][11] = TERM_YELLOW; } //yellow if both...
			if (csheet_boni[i].cb[6] & CB7_RFOOD) { c_put_str(TERM_WHITE, "+", 13, 25 + i); if (header_color[1][12] == TERM_L_DARK) header_color[1][12] = TERM_WHITE; }
			if (p_ptr->prace == RACE_ENT || p_ptr->prace == RACE_VAMPIRE) { c_put_str(TERM_WHITE, "*", 13, 25 + 14); if (header_color[1][12] == TERM_L_DARK) header_color[1][12] = TERM_WHITE; } //Hack: Ents/Vamps require food but do not gorge! - Kurzel
			if (csheet_boni[i].cb[6] & CB7_IFOOD) { c_put_str(TERM_L_UMBER, "*", 13, 25 + i); if (header_color[1][12] != TERM_L_UMBER) header_color[1][12] = TERM_L_UMBER; }
			if (csheet_boni[i].cb[6] & CB7_RVAMP) {
				if ((i == 0) || (i == 1) || (i == 2) || (i == 12)
				    || ((i == 14) && ((p_ptr->body_monster == 365) || (p_ptr->body_monster == 391))) //Hack: use * for 100% weapon/ammo or v-bat/mist forms
				    || (i == 14 && race == RACE_VAMPIRE && get_skill(SKILL_NECROMANCY) == 50 && get_skill(SKILL_TRAUMATURGY) == 50)) { //Nasty hack: Assume that having full trauma+necro gives 100% vamp actually
					c_put_str(TERM_WHITE, "*", 14, 25 + i); header_color[1][13] = TERM_WHITE;
				} else { c_put_str(TERM_WHITE, "+", 14, 25 + i); header_color[1][13] = TERM_WHITE; }
			}
			if (p_ptr->mode & MODE_FRUIT_BAT && p_ptr->body_monster == 0) { c_put_str(TERM_WHITE, "+", 14, 25 + 14); header_color[1][13] = TERM_WHITE; } //Mega Hack: Hardcode 50% vamp as a fruit bat! Maybe incorrect for mimics? - Kurzel
			if (csheet_boni[i].cb[6] & CB7_RAUID) { c_put_str(TERM_WHITE, "+", 15, 25 + i); header_color[1][14] = TERM_WHITE; }
			if (csheet_boni[i].cb[6] & CB7_RREFL) { c_put_str(TERM_WHITE, "+", 16, 25 + i); header_color[1][15] = TERM_WHITE; }
			if (csheet_boni[i].cb[6] & CB7_RAMSH) { c_put_str(TERM_YELLOW, "+", 17, 25 + i); header_color[1][16] = TERM_YELLOW; }
			// AMFI (Numerical) goes here at [1][17] - Kurzel
			if (csheet_boni[i].cb[6] & CB7_RAGGR) { c_put_str(TERM_RED, "+", 19, 25 + i); header_color[1][18] = TERM_RED; }

			/* Numerical Boni (WHITE if affected at, GOLD if sustained or maxxed (important), at a glance - Kurzel */
			if (csheet_boni[i].spd != 0) {
				header_color[2][0] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].spd > 9) color = TERM_GREEN;
				if (csheet_boni[i].spd > 19) color = TERM_L_UMBER; //high form/skills
				if (csheet_boni[i].spd < 0) color = TERM_L_RED;
				if (csheet_boni[i].spd < -9) color = TERM_RED;
				sprintf(tmp, "%d", abs(csheet_boni[i].spd) % 10);
				if (csheet_boni[i].spd > 29) sprintf(tmp, "*");
				c_put_str(color, tmp, 1, 45 + i);
			}
			if (csheet_boni[i].slth != 0) {
				header_color[2][1] = TERM_WHITE;
				if (p_ptr->skill_stl >= 30) header_color[2][1] = TERM_L_UMBER;
				color = TERM_L_GREEN;
				if (csheet_boni[i].slth > 9) color = TERM_GREEN;
				if (csheet_boni[i].slth > 19) color = TERM_L_UMBER; //admin items
				if (csheet_boni[i].slth < 0) color = TERM_L_RED;
				if (csheet_boni[i].slth < -9) color = TERM_RED;
				sprintf(tmp, "%d", abs(csheet_boni[i].slth) % 10);
				if (csheet_boni[i].slth > 29) sprintf(tmp, "*");
				c_put_str(color, tmp, 2, 45 + i);
			}
			if (csheet_boni[i].srch != 0) {
				header_color[2][2] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].srch > 9) color = TERM_GREEN;
				if (csheet_boni[i].srch > 19) color = TERM_L_UMBER; //admin items
				if (csheet_boni[i].srch < 0) color = TERM_L_RED;
				if (csheet_boni[i].srch < -9) color = TERM_RED;
				sprintf(tmp, "%d", abs(csheet_boni[i].srch) % 10);
				if (csheet_boni[i].srch > 29) sprintf(tmp, "*");
				c_put_str(color, tmp, 3, 45 + i);
			}
			if (csheet_boni[i].infr != 0) {
				header_color[2][3] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].infr > 9) color = TERM_GREEN;
				if (csheet_boni[i].infr > 19) color = TERM_L_UMBER; //admin items
				if (csheet_boni[i].infr < 0) color = TERM_L_RED;
				if (csheet_boni[i].infr < -9) color = TERM_RED;
				sprintf(tmp, "%d", abs(csheet_boni[i].infr) % 10);
				if (csheet_boni[i].infr > 29) sprintf(tmp, "*");
				c_put_str(color, tmp, 4, 45 + i);
			}
			if (csheet_boni[i].lite != 0) {
				header_color[2][4] = TERM_WHITE;
				color = TERM_WHITE;
				if (csheet_boni[i].cb[12] & CB13_XLITE) { color = TERM_L_UMBER; header_color[2][4] = TERM_L_UMBER; }
				sprintf(tmp, "%d", abs(csheet_boni[i].lite) % 10);
				if (csheet_boni[i].lite > 9) sprintf(tmp, "*");
				c_put_str(color, tmp, 5, 45 + i);
			}
			if (csheet_boni[i].dig != 0) {
				header_color[2][5] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].dig > 9) color = TERM_GREEN;
				if (csheet_boni[i].dig > 19) color = TERM_L_UMBER; //admin items
				if (csheet_boni[i].dig < 0) color = TERM_L_RED;
				if (csheet_boni[i].dig < -9) color = TERM_RED;
				sprintf(tmp, "%d", abs(csheet_boni[i].dig) % 10);
				if (csheet_boni[i].dig > 29 || (csheet_boni[i].cb[12] & CB13_XWALL)) sprintf(tmp, "*");
				if (csheet_boni[i].cb[12] & CB13_XWALL) color = TERM_L_UMBER;
				c_put_str(color, tmp, 6, 45 + i);
			}
			if (csheet_boni[i].blow != 0) {
				header_color[2][6] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].blow > 9) color = TERM_GREEN;
				if (csheet_boni[i].blow < 0) color = TERM_L_RED;
				if (csheet_boni[i].blow < -9) color = TERM_RED;
				sprintf(tmp, "%d", abs(csheet_boni[i].blow) % 10);
				if (csheet_boni[i].blow > 19) sprintf(tmp, "*");
				c_put_str(color, tmp, 7, 45 + i);
			}
			if (csheet_boni[i].crit != 0) {
				header_color[2][7] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].crit > 9) color = TERM_GREEN;
				if (csheet_boni[i].crit < 0) color = TERM_L_RED;
				if (csheet_boni[i].crit < -9) color = TERM_RED;
				sprintf(tmp, "%d", abs(csheet_boni[i].crit) % 10);
				if (csheet_boni[i].crit > 19) sprintf(tmp, "*");
				c_put_str(color, tmp, 8, 45 + i);
			}
			if (csheet_boni[i].shot != 0) {
				header_color[2][8] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].shot > 9) color = TERM_GREEN;
				if (csheet_boni[i].shot < 0) color = TERM_L_RED;
				if (csheet_boni[i].shot < -9) color = TERM_RED;
				sprintf(tmp, "%d", abs(csheet_boni[i].shot) % 10);
				if (csheet_boni[i].shot > 19) sprintf(tmp, "*");
				c_put_str(color, tmp, 9, 45 + i);
			}
			if (csheet_boni[i].migh != 0) {
				header_color[2][9] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].migh > 9) color = TERM_GREEN;
				if (csheet_boni[i].migh < 0) color = TERM_L_RED;
				if (csheet_boni[i].migh < -9) color = TERM_RED;
				sprintf(tmp, "%d", abs(csheet_boni[i].migh) % 10);
				if (csheet_boni[i].migh > 19) sprintf(tmp, "*");
				c_put_str(color, tmp, 10, 45 + i);
			}
			if (csheet_boni[i].mxhp != 0) {
				header_color[2][10] = TERM_WHITE;
				life_sum += csheet_boni[i].mxhp;
				if (life_sum >= 3) header_color[2][10] = TERM_L_UMBER; //max
				color = TERM_L_GREEN;
				if (csheet_boni[i].mxhp >= 3) color = TERM_L_UMBER;
				else if (csheet_boni[i].mxhp < 0) color = TERM_RED;
				sprintf(tmp, "%d", abs(csheet_boni[i].mxhp) % 10);
				if (csheet_boni[i].mxhp > 19) sprintf(tmp, "*");
				c_put_str(color, tmp, 11, 45 + i);
			}
			if (csheet_boni[i].mxmp != 0) {
				header_color[2][11] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].mxmp > 9) color = TERM_GREEN;
				if (csheet_boni[i].mxmp < 0) color = TERM_L_RED;
				if (csheet_boni[i].mxmp < -9) color = TERM_RED;
				sprintf(tmp, "%d", abs(csheet_boni[i].mxmp) % 10);
				if (csheet_boni[i].mxmp > 19) sprintf(tmp, "*");
				c_put_str(color, tmp, 12, 45 + i);
			}
			if (csheet_boni[i].luck != 0) {
				luck_sum += csheet_boni[i].luck;
				header_color[2][12] = TERM_WHITE;
				if (luck_sum >= 40) header_color[2][12] = TERM_L_UMBER; //max
				color = TERM_L_GREEN;
				if (csheet_boni[i].luck > 9) color = TERM_GREEN;
				if (csheet_boni[i].luck > 19) color = TERM_L_UMBER; //high luck sets
				if (csheet_boni[i].luck < 0) color = TERM_L_RED;
				if (csheet_boni[i].luck < -9) color = TERM_RED;
				sprintf(tmp, "%d", abs(csheet_boni[i].luck) % 10);
				if (csheet_boni[i].mxmp > 29) sprintf(tmp, "*");
				c_put_str(color, tmp, 13, 45 + i);
			}
			if (csheet_boni[i].pstr != 0) {
				if (header_color[2][13] == TERM_L_DARK) header_color[2][13] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].cb[11] & CB12_RSSTR) { color = TERM_L_UMBER; header_color[2][13] = TERM_L_UMBER; }
				if (csheet_boni[i].pstr > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSSTR) color = TERM_UMBER; }
				if (csheet_boni[i].pstr < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSSTR) color = TERM_YELLOW; }
				if (csheet_boni[i].pstr < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSSTR) color = TERM_ORANGE; }
				sprintf(tmp, "%d", abs(csheet_boni[i].pstr) % 10);
				if (csheet_boni[i].pstr > 19) sprintf(tmp, "*");
				c_put_str(color, tmp, 14, 45 + i);
			} else if (csheet_boni[i].cb[11] & CB12_RSSTR) { c_put_str(TERM_L_UMBER, "s", 14, 45 + i); header_color[2][13] = TERM_L_UMBER; }
			if (csheet_boni[i].pint != 0) {
				if (header_color[2][14] == TERM_L_DARK) header_color[2][14] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].cb[11] & CB12_RSINT) { color = TERM_L_UMBER; header_color[2][14] = TERM_L_UMBER; }
				if (csheet_boni[i].pint > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSINT) color = TERM_UMBER; }
				if (csheet_boni[i].pint < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSINT) color = TERM_YELLOW; }
				if (csheet_boni[i].pint < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSINT) color = TERM_ORANGE; }
				sprintf(tmp, "%d", abs(csheet_boni[i].pint) % 10);
				if (csheet_boni[i].pint > 19) sprintf(tmp, "*");
				c_put_str(color, tmp, 15, 45 + i);
			} else if (csheet_boni[i].cb[11] & CB12_RSINT) { c_put_str(TERM_L_UMBER, "s", 15, 45 + i); header_color[2][14] = TERM_L_UMBER; }
			if (csheet_boni[i].pwis != 0) {
				if (header_color[2][15] == TERM_L_DARK) header_color[2][15] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].cb[11] & CB12_RSWIS) { color = TERM_L_UMBER; header_color[2][15] = TERM_L_UMBER; }
				if (csheet_boni[i].pwis > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSWIS) color = TERM_UMBER; }
				if (csheet_boni[i].pwis < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSWIS) color = TERM_YELLOW; }
				if (csheet_boni[i].pwis < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSWIS) color = TERM_ORANGE; }
				sprintf(tmp, "%d", abs(csheet_boni[i].pwis) % 10);
				if (csheet_boni[i].pwis > 19) sprintf(tmp, "*");
				c_put_str(color, tmp, 16, 45 + i);
			} else if (csheet_boni[i].cb[11] & CB12_RSWIS) { c_put_str(TERM_L_UMBER, "s", 16, 45 + i); header_color[2][15] = TERM_L_UMBER; }
			if (csheet_boni[i].pdex != 0) {
				if (header_color[2][16] == TERM_L_DARK) header_color[2][16] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].cb[11] & CB12_RSDEX) { color = TERM_L_UMBER; header_color[2][16] = TERM_L_UMBER; }
				if (csheet_boni[i].pdex > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSDEX) color = TERM_UMBER; }
				if (csheet_boni[i].pdex < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSDEX) color = TERM_YELLOW; }
				if (csheet_boni[i].pdex < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSDEX) color = TERM_ORANGE; }
				sprintf(tmp, "%d", abs(csheet_boni[i].pdex) % 10);
				if (csheet_boni[i].pdex > 19) sprintf(tmp, "*");
				c_put_str(color, tmp, 17, 45 + i);
			} else if (csheet_boni[i].cb[11] & CB12_RSDEX) { c_put_str(TERM_L_UMBER, "s", 17, 45 + i); header_color[2][16] = TERM_L_UMBER; }
			if (csheet_boni[i].pcon != 0) {
				if (header_color[2][17] == TERM_L_DARK) header_color[2][17] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].cb[11] & CB12_RSCON) { color = TERM_L_UMBER; header_color[2][17] = TERM_L_UMBER; }
				if (csheet_boni[i].pcon > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSCON) color = TERM_UMBER; }
				if (csheet_boni[i].pcon < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSCON) color = TERM_YELLOW; }
				if (csheet_boni[i].pcon < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSCON) color = TERM_ORANGE; }
				sprintf(tmp, "%d", abs(csheet_boni[i].pcon) % 10);
				if (csheet_boni[i].pcon > 19) sprintf(tmp, "*");
				c_put_str(color, tmp, 18, 45 + i);
			} else if (csheet_boni[i].cb[11] & CB12_RSCON) { c_put_str(TERM_L_UMBER, "s", 18, 45 + i); header_color[2][17] = TERM_L_UMBER; }
			if (csheet_boni[i].pchr != 0) {
				if (header_color[2][18] == TERM_L_DARK) header_color[2][18] = TERM_WHITE;
				color = TERM_L_GREEN;
				if (csheet_boni[i].cb[11] & CB12_RSCHR) { color = TERM_L_UMBER; header_color[2][18] = TERM_L_UMBER; }
				if (csheet_boni[i].pchr > 9) { color = TERM_GREEN; if (csheet_boni[i].cb[11] & CB12_RSCHR) color = TERM_UMBER; }
				if (csheet_boni[i].pchr < 0) { color = TERM_L_RED; if (csheet_boni[i].cb[11] & CB12_RSCHR) color = TERM_YELLOW; }
				if (csheet_boni[i].pchr < -9) { color = TERM_RED; if (csheet_boni[i].cb[11] & CB12_RSCHR) color = TERM_ORANGE; }
				sprintf(tmp, "%d", abs(csheet_boni[i].pchr) % 10);
				if (csheet_boni[i].pchr > 19) sprintf(tmp, "*");
				c_put_str(color, tmp, 19, 45 + i);
			} else if (csheet_boni[i].cb[11] & CB12_RSCHR) { c_put_str(TERM_L_UMBER, "s", 19, 45 + i); header_color[2][18] = TERM_L_UMBER; }

			/* Track AM Field in a similar fashion */
			if (csheet_boni[i].amfi != 0) {
				if (i == 0) amfi1 = csheet_boni[i].amfi;
				else if (i == 1) amfi2 = csheet_boni[i].amfi;
				else amfi_sum += csheet_boni[i].amfi;
				header_color[1][17] = TERM_WHITE;

				/* item? (dark sword only) */
				if (i != 14) {
					if (csheet_boni[i].amfi >= 30) color = TERM_L_UMBER; //show max on darkswords, not on form/skill column though
					else color = TERM_L_GREEN;
				}
				else if (csheet_boni[i].amfi >= ANTIMAGIC_CAP) color = TERM_L_UMBER; //max AM skill, or form+skill combo
				else if ((csheet_boni[i].amfi / 10) >= 5) color = TERM_GREEN; //max AM skill, or form+skill combo
				else color = TERM_L_GREEN;

				sprintf(tmp, "%d", csheet_boni[i].amfi / 10);
				c_put_str(color, tmp, 18, 25 + i);
			}

			/* ESP and slay flags, white for anything */
			if (csheet_boni[i].cb[7] & CB8_ESPID) { c_put_str(TERM_WHITE, "e", 1, 65 + i); header_color[3][0] = TERM_WHITE; }
			if (csheet_boni[i].cb[7] & CB8_EANIM) { c_put_str(TERM_WHITE, "e", 2, 65 + i); header_color[3][1] = TERM_WHITE; }
			if (csheet_boni[i].cb[7] & CB8_SANIM) { c_put_str(TERM_WHITE, "s", 2, 65 + i); header_color[3][1] = TERM_WHITE; }
			if ((csheet_boni[i].cb[7] & CB8_EANIM) && (csheet_boni[i].cb[7] & CB8_SANIM)) c_put_str(TERM_WHITE, "+", 2, 65 + i);
			if (csheet_boni[i].cb[7] & CB8_EORCS) { c_put_str(TERM_WHITE, "e", 3, 65 + i); header_color[3][2] = TERM_WHITE; }
			if (csheet_boni[i].cb[7] & CB8_SORCS) { c_put_str(TERM_WHITE, "s", 3, 65 + i); header_color[3][2] = TERM_WHITE; }
			if ((csheet_boni[i].cb[7] & CB8_EORCS) && (csheet_boni[i].cb[7] & CB8_SORCS)) c_put_str(TERM_WHITE, "+", 3, 65 + i);
			if (csheet_boni[i].cb[7] & CB8_ETROL) { c_put_str(TERM_WHITE, "e", 4, 65 + i); header_color[3][3] = TERM_WHITE; }
			if (csheet_boni[i].cb[7] & CB8_STROL) { c_put_str(TERM_WHITE, "s", 4, 65 + i); header_color[3][3] = TERM_WHITE; }
			if ((csheet_boni[i].cb[7] & CB8_ETROL) && (csheet_boni[i].cb[7] & CB8_STROL)) c_put_str(TERM_WHITE, "+", 4, 65 + i);
			if (csheet_boni[i].cb[7] & CB8_EGIAN) { c_put_str(TERM_WHITE, "e", 5, 65 + i); header_color[3][4] = TERM_WHITE; }

			if (csheet_boni[i].cb[8] & CB9_SGIAN) { c_put_str(TERM_WHITE, "s", 5, 65 + i); header_color[3][4] = TERM_WHITE; }
			if ((csheet_boni[i].cb[7] & CB8_EGIAN) && (csheet_boni[i].cb[8] & CB9_SGIAN)) c_put_str(TERM_WHITE, "+", 5, 65 + i);
			if (csheet_boni[i].cb[8] & CB9_EDRGN) { c_put_str(TERM_WHITE, "e", 6, 65 + i); header_color[3][5] = TERM_WHITE; }
			if (csheet_boni[i].cb[8] & CB9_SDRGN) { c_put_str(TERM_WHITE, "s", 6, 65 + i); header_color[3][5] = TERM_WHITE; }
			if (csheet_boni[i].cb[8] & CB9_KDRGN) { c_put_str(TERM_WHITE, "k", 6, 65 + i); header_color[3][5] = TERM_WHITE; }
			if ((csheet_boni[i].cb[8] & CB9_EDRGN) && (csheet_boni[i].cb[8] & CB9_SDRGN)) c_put_str(TERM_WHITE, "+", 6, 65 + i);
			if ((csheet_boni[i].cb[8] & CB9_EDRGN) && (csheet_boni[i].cb[8] & CB9_KDRGN)) c_put_str(TERM_WHITE, "*", 6, 65 + i);
			if (csheet_boni[i].cb[8] & CB9_EDEMN) { c_put_str(TERM_WHITE, "e", 7, 65 + i); header_color[3][6] = TERM_WHITE; }
			if (csheet_boni[i].cb[8] & CB9_SDEMN) { c_put_str(TERM_WHITE, "s", 7, 65 + i); header_color[3][6] = TERM_WHITE; }
			if (csheet_boni[i].cb[8] & CB9_KDEMN) { c_put_str(TERM_WHITE, "k", 7, 65 + i); header_color[3][6] = TERM_WHITE; }
			if ((csheet_boni[i].cb[8] & CB9_EDEMN) && (csheet_boni[i].cb[8] & CB9_SDEMN)) c_put_str(TERM_WHITE, "+", 7, 65 + i);
			if ((csheet_boni[i].cb[8] & CB9_EDEMN) && (csheet_boni[i].cb[8] & CB9_KDEMN)) c_put_str(TERM_WHITE, "*", 7, 65 + i);
			if (csheet_boni[i].cb[8] & CB9_EUNDD) { c_put_str(TERM_WHITE, "e", 8, 65 + i); header_color[3][7] = TERM_WHITE; }

			if (csheet_boni[i].cb[9] & CB10_SUNDD) { c_put_str(TERM_WHITE, "s", 8, 65 + i); header_color[3][7] = TERM_WHITE; }
			if (csheet_boni[i].cb[9] & CB10_KUNDD) { c_put_str(TERM_WHITE, "k", 8, 65 + i); header_color[3][7] = TERM_WHITE; }
			if ((csheet_boni[i].cb[8] & CB9_EUNDD) && (csheet_boni[i].cb[9] & CB10_SUNDD)) c_put_str(TERM_WHITE, "+", 8, 65 + i);
			if ((csheet_boni[i].cb[8] & CB9_EUNDD) && (csheet_boni[i].cb[9] & CB10_KUNDD)) c_put_str(TERM_WHITE, "*", 8, 65 + i);
			if (csheet_boni[i].cb[9] & CB10_EEVIL) { c_put_str(TERM_WHITE, "e", 9, 65 + i); header_color[3][8] = TERM_WHITE; }
			if (csheet_boni[i].cb[9] & CB10_SEVIL) { c_put_str(TERM_WHITE, "s", 9, 65 + i); header_color[3][8] = TERM_WHITE; }
			if ((csheet_boni[i].cb[9] & CB10_EEVIL) && (csheet_boni[i].cb[9] & CB10_SEVIL)) c_put_str(TERM_WHITE, "+", 9, 65 + i);
			if (csheet_boni[i].cb[9] & CB10_EDGRI) { c_put_str(TERM_WHITE, "e", 10, 65 + i); header_color[3][9] = TERM_WHITE; }
			if (csheet_boni[i].cb[9] & CB10_EGOOD) { c_put_str(TERM_WHITE, "e", 11, 65 + i); header_color[3][10] = TERM_WHITE; }
			if (csheet_boni[i].cb[9] & CB10_ENONL) { c_put_str(TERM_WHITE, "e", 12, 65 + i); header_color[3][11] = TERM_WHITE; }
			if (csheet_boni[i].cb[9] & CB10_EUNIQ) { c_put_str(TERM_WHITE, "e", 13, 65 + i); header_color[3][12] = TERM_WHITE; }

			if (csheet_boni[i].cb[10] & CB11_BFIRE) { c_put_str(TERM_WHITE, "b", 14, 65 + i); header_color[3][13] = TERM_WHITE; }
			if (csheet_boni[i].cb[10] & CB11_AFIRE) { c_put_str(TERM_WHITE, "a", 14, 65 + i); header_color[3][13] = TERM_WHITE; }
			if ((csheet_boni[i].cb[10] & CB11_BFIRE) && (csheet_boni[i].cb[10] & CB11_AFIRE)) c_put_str(TERM_WHITE, "+", 14, 65 + i);
			if (csheet_boni[i].cb[10] & CB11_BCOLD) { c_put_str(TERM_WHITE, "b", 15, 65 + i); header_color[3][14] = TERM_WHITE; }
			if (csheet_boni[i].cb[10] & CB11_ACOLD) { c_put_str(TERM_WHITE, "a", 15, 65 + i); header_color[3][14] = TERM_WHITE; }
			if ((csheet_boni[i].cb[10] & CB11_BCOLD) && (csheet_boni[i].cb[10] & CB11_ACOLD)) c_put_str(TERM_WHITE, "+", 15, 65 + i);
			if (csheet_boni[i].cb[10] & CB11_BELEC) { c_put_str(TERM_WHITE, "b", 16, 65 + i); header_color[3][15] = TERM_WHITE; }
			if (csheet_boni[i].cb[10] & CB11_AELEC) { c_put_str(TERM_WHITE, "a", 16, 65 + i); header_color[3][15] = TERM_WHITE; }
			if ((csheet_boni[i].cb[10] & CB11_BELEC) && (csheet_boni[i].cb[10] & CB11_AELEC)) c_put_str(TERM_WHITE, "+", 16, 65 + i);
			if (csheet_boni[i].cb[10] & CB11_BACID) { c_put_str(TERM_WHITE, "b", 17, 65 + i); header_color[3][16] = TERM_WHITE; }
			if (csheet_boni[i].cb[10] & CB11_BPOIS) { c_put_str(TERM_WHITE, "b", 18, 65 + i); header_color[3][17] = TERM_WHITE; }
			if (csheet_boni[i].cb[11] & CB12_BVORP) { c_put_str(TERM_WHITE, "+", 19, 65 + i); header_color[3][18] = TERM_WHITE; }

			/* Footer */
			color = csheet_boni[i].color;
			symbol = (csheet_boni[i].symbol ? csheet_boni[i].symbol : ' ');
			sprintf(tmp, "%c", symbol);
			for (j = 0; j < 5; j++)
				c_put_str(color, tmp, 20, 5 + j * 20 + i);
		}

		/* AM field - body? skill or monster form */
		if (amfi1 > amfi2) amfi_sum += amfi1;
		else amfi_sum += amfi2;
		if (amfi_sum >= ANTIMAGIC_CAP) header_color[1][17] = TERM_L_UMBER;

		/* Row Headers */
		c_put_str(header_color[0][0], "Fire:", 1, 0); //row, col
		c_put_str(header_color[0][1], "Cold:", 2, 0);
		c_put_str(header_color[0][2], "Elec:", 3, 0);
		c_put_str(header_color[0][3], "Acid:", 4, 0);
		c_put_str(header_color[0][4], "Pois:", 5, 0);
		c_put_str(header_color[0][5], "Blnd:", 6, 0);
		c_put_str(header_color[0][6], "Lite:", 7, 0);
		c_put_str(header_color[0][7], "Dark:", 8, 0);
		c_put_str(header_color[0][8], "Soun:", 9, 0);
		c_put_str(header_color[0][9], "Shrd:", 10, 0);
		c_put_str(header_color[0][10], "Nexu:", 11, 0);
		c_put_str(header_color[0][11], "Neth:", 12, 0);
		c_put_str(header_color[0][12], "Conf:", 13, 0);
		c_put_str(header_color[0][13], "Chao:", 14, 0);
		c_put_str(header_color[0][14], "Dise:", 15, 0);
		c_put_str(header_color[0][15], "Watr:", 16, 0);
		c_put_str(header_color[0][16], "Time:", 17, 0);
		c_put_str(header_color[0][17], "Mana:", 18, 0);
		c_put_str(header_color[0][18], "Mind:", 19, 0);

		c_put_str(header_color[1][0], "Fear:", 1, 20);
		c_put_str(header_color[1][1], "Para:", 2, 20);
		c_put_str(header_color[1][2], "Tele:", 3, 20);
		c_put_str(header_color[1][3], "SInv:", 4, 20);
		c_put_str(header_color[1][4], "Invs:", 5, 20);
		c_put_str(header_color[1][5], "HLif:", 6, 20);
		c_put_str(header_color[1][6], "Wrth:", 7, 20);
		c_put_str(header_color[1][7], "FFal:", 8, 20);
		c_put_str(header_color[1][8], "Lvtn:", 9, 20);
		c_put_str(header_color[1][9], "Clmb:", 10, 20);
		c_put_str(header_color[1][10], "RgHP:", 11, 20);
		c_put_str(header_color[1][11], "RgMP:", 12, 20);
		c_put_str(header_color[1][12], "Food:", 13, 20);
		c_put_str(header_color[1][13], "Vamp:", 14, 20);
		c_put_str(header_color[1][14], "AuID:", 15, 20);
		c_put_str(header_color[1][15], "Refl:", 16, 20);
		c_put_str(header_color[1][16], "AMSh:", 17, 20);
		c_put_str(header_color[1][17], "AMFi:", 18, 20);
		c_put_str(header_color[1][18], "Aggr:", 19, 20);

		c_put_str(header_color[2][0], "Spd :", 1, 40);
		c_put_str(header_color[2][1], "Slth:", 2, 40);
		c_put_str(header_color[2][2], "Srch:", 3, 40);
		c_put_str(header_color[2][3], "Infr:", 4, 40);
		c_put_str(header_color[2][4], "Lite:", 5, 40);
		c_put_str(header_color[2][5], "Tunn:", 6, 40);
		c_put_str(header_color[2][6], "Blow:", 7, 40);
		c_put_str(header_color[2][7], "Crit:", 8, 40);
		c_put_str(header_color[2][8], "Shot:", 9, 40);
		c_put_str(header_color[2][9], "Mght:", 10, 40);
		c_put_str(header_color[2][10], "MxHP:", 11, 40);
		c_put_str(header_color[2][11], "MxMP:", 12, 40);
		c_put_str(header_color[2][12], "Luck:", 13, 40);
		c_put_str(header_color[2][13], "STR :", 14, 40);
		c_put_str(header_color[2][14], "INT :", 15, 40);
		c_put_str(header_color[2][15], "WIS :", 16, 40);
		c_put_str(header_color[2][16], "DEX :", 17, 40);
		c_put_str(header_color[2][17], "CON :", 18, 40);
		c_put_str(header_color[2][18], "CHR :", 19, 40);

		c_put_str(header_color[3][0], "Spid:", 1, 60);
		c_put_str(header_color[3][1], "Anim:", 2, 60);
		c_put_str(header_color[3][2], "Orcs:", 3, 60);
		c_put_str(header_color[3][3], "Trol:", 4, 60);
		c_put_str(header_color[3][4], "Gian:", 5, 60);
		c_put_str(header_color[3][5], "Drgn:", 6, 60);
		c_put_str(header_color[3][6], "Demn:", 7, 60);
		c_put_str(header_color[3][7], "Undd:", 8, 60);
		c_put_str(header_color[3][8], "Evil:", 9, 60);
		c_put_str(header_color[3][9], "Dgri:", 10, 60);
		c_put_str(header_color[3][10], "Good:", 11, 60);
		c_put_str(header_color[3][11], "Nonl:", 12, 60);
		c_put_str(header_color[3][12], "Uniq:", 13, 60);
		c_put_str(header_color[3][13], "Fire:", 14, 60);
		c_put_str(header_color[3][14], "Cold:", 15, 60);
		c_put_str(header_color[3][15], "Elec:", 16, 60);
		c_put_str(header_color[3][16], "Acid:", 17, 60);
		c_put_str(header_color[3][17], "Pois:", 18, 60);
		c_put_str(header_color[3][18], "Vorp:", 19, 60);
	}
}

/*
 * Redraw any necessary windows
 */
void window_stuff(void) {
	/* Redraw the skills menu if requested */
	if (redraw_skills)
		do_redraw_skills();

	/* Redraw the store inventory if requested */
	if (redraw_store)
		do_redraw_store();

	/* Window stuff */
	if (!p_ptr->window) return;

	/* Display inventory */
	if (p_ptr->window & PW_INVEN) {
		p_ptr->window &= ~(PW_INVEN);
		fix_inven();
	}

	/* Display equipment */
	if (p_ptr->window & PW_EQUIP) {
		p_ptr->window &= (~PW_EQUIP);
		fix_equip();
	}

	/* Display player */
	if (p_ptr->window & PW_PLAYER) {
		p_ptr->window &= (~PW_PLAYER);
		fix_player();
	}

	/* Display messages */
	if (p_ptr->window & (PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT)) {
		p_ptr->window &= (~(PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT));
		fix_message();
	}

	/* Display the lag-o-meter */
	if (p_ptr->window & (PW_LAGOMETER)) {
		p_ptr->window &= (~PW_LAGOMETER);
		fix_lagometer();
	}
}

/* Handle weather (rain and snow) client-side - C. Blue
 * Note: keep following defines in sync with nclient.c, beginning of file.
 * do_weather() is called by do_ping() which is called every frame. */
#define SKY_ALTITUDE	20 /* assumed 'pseudo-isometric' cloud altitude */
#define PANEL_X		(SCREEN_PAD_LEFT) /* physical top-left screen position of view panel */
#define PANEL_Y		(SCREEN_PAD_TOP) /* physical top-left screen position of view panel */
void do_weather() {
	int i, j, intensity;
	static int weather_gen_ticks = 0, weather_ticks10 = 0;
	static int weather_wind_ticks = 0, weather_speed_snow_ticks = 0, weather_speed_rain_ticks = 0; /* sub-ticks when weather really is processed */
	bool wind_west_effective = FALSE, wind_east_effective = FALSE; /* horizontal movement */
	bool gravity_effective_rain = FALSE, gravity_effective_snow = FALSE; /* vertical movement */
	bool redraw = FALSE;
	int x, y, dx, dy, d; /* distance calculations (cloud centre) */
	static int cloud_movement_ticks = 0, cloud_movement_lasttick = 0;
	bool with_clouds, outside_clouds;


	/* continue animating current weather (if any) */
	if (!weather_type && !weather_elements) {
#ifdef USE_SOUND_2010
		weather_particles_seen = 0;
#endif
		return;
	}


/* perform redraw and sync hacks ------------------------------------------- */

	/* hack: instantly redraw weather without waiting for ticks and exit */
	if (weather_type >= 20000) {
		weather_type -= 20000;
		redraw = TRUE;
	}

	/* hack: reset sub-ticks to graphically synchronize horizontal
	   and vertical movement in case they use the same modulo (!) */
	if (weather_type >= 10000) {
		weather_type -= 10000;
		/* reset wind ticks and speed ticks to possibly synch them */
		weather_wind_ticks = 0;
		weather_speed_snow_ticks = weather_speed_rain_ticks = 0;
	}

	if (redraw) {
		if (screen_icky) Term_switch(0);

		for (i = 0; i < weather_elements; i++) {
#ifdef BIGMAP_MINDLINK_HACK
			/* big_map mindlink visuals hack */
			if (last_line_y <= SCREEN_HGT &&
			    weather_element_y[i] >= weather_panel_y + SCREEN_HGT)
				;
			else
#endif
			/* only for elements within visible panel screen area */
			if (weather_element_x[i] >= weather_panel_x &&
			    weather_element_x[i] < weather_panel_x + screen_wid &&
			    weather_element_y[i] >= weather_panel_y &&
			    weather_element_y[i] < weather_panel_y + screen_hgt) {
				if (weather_element_type[i] == 1) {
					/* display raindrop */
					Term_draw(PANEL_X + weather_element_x[i] - weather_panel_x,
					    PANEL_Y + weather_element_y[i] - weather_panel_y,
#ifdef EXTENDED_BG_COLOURS
					    rand_int(2) ? TERM2_BLUE : TERM_ORANGE, weather_wind == 0 ? '|' : (weather_wind % 2 == 1 ? '\\' : '/'));
#else
					    TERM_BLUE, weather_wind == 0 ? '|' : (weather_wind % 2 == 1 ? '\\' : '/'));
#endif
				} else if (weather_element_type[i] == 2) {
					/* display snowflake */
					Term_draw(PANEL_X + weather_element_x[i] - weather_panel_x,
					    PANEL_Y + weather_element_y[i] - weather_panel_y,
					    TERM_WHITE, '*');
				}
			}
		}

		if (screen_icky) Term_switch(0);
		/* Update the screen */
		Term_fresh();

		/* started to draw on a freshly updated panel? */
		weather_panel_changed = FALSE;
		/* return to 'reenter' regular timing */
		if (weather_elements) return;
	}


/* perform buffered cloud movement ----------------------------------------- */

	/* limited cloud movement, buffered client-side */
	if (ticks != cloud_movement_lasttick) {
		cloud_movement_lasttick = ticks;
		cloud_movement_ticks++;
		/* perform movement once per second */
		if (cloud_movement_ticks >= 10) {
			cloud_movement_ticks = 0;
			for (i = 0; i < 10; i++) {
				/* cloud exists? */
				if (cloud_x1[i] != -9999) {
					/* move a tiny bit, virtually */
					cloud_xfrac[i] += cloud_xm100[i];
					cloud_yfrac[i] += cloud_ym100[i];
					x = cloud_xfrac[i] / 100;
					y = cloud_yfrac[i] / 100;
					/* finally advanced a grid? */
					if (x) {
						cloud_x1[i] += x;
						cloud_x2[i] += x;
						cloud_xfrac[i] -= x * 100;
					}
					if (y) {
						cloud_y1[i] += y;
						cloud_y2[i] += y;
						cloud_yfrac[i] -= y * 100;
					}
				}
			}
		}
	}


/* process weather every 10ms at most -------------------------------------- */

	/* attempt to keep track of 'deci-ticks' (10ms resolution) */
	if (ticks10 == weather_ticks10) return;
	weather_ticks10 = ticks10;
	/* note: the second limit is the frame rate, cfg.fps,
	   so if it's < 100, it will limit the speed instead. */


/* generate new weather elements ------------------------------------------- */

	/* hack: pre-generate extra elements (usually SKY_ALTITUDE)?
	   Used if player enters an already weather-active sector */
	intensity = ((weather_type / 10) + 1);
	weather_type = weather_type % 10;

	/* weather creation: check whether elements are to be generated this time */
	weather_gen_ticks++;
	/* hack: exception for pre-generated elements (intensity > 1): insta-gen! */
	if (weather_gen_ticks == weather_gen_speed || intensity > 1) {
		/* this check might (partially) not be very important */
		if (weather_gen_ticks == weather_gen_speed)
			weather_gen_ticks = 0;
		else
			intensity--;

		/* factor in received intensity */
		intensity *= weather_intensity;

		/* create weather elements, ie rain drops or snow flakes */
		if (weather_elements <= 1024 - intensity) {
			for (i = 0; i < intensity; i++) {
				/* generate random starting pos */
				x = rand_int(MAX_WID - 2) + 1;
				y = rand_int(MAX_HGT - 1 + SKY_ALTITUDE) - SKY_ALTITUDE;

				/* test pos for validity in regards to cloud */
				with_clouds = FALSE; /* assume no clouds exist */
				outside_clouds = TRUE; /* assume not within any clouds area */
				for (j = 0; j < 10; j++) {
					if (cloud_x1[j] != -9999) { /* does cloud exist? */
						/* at least one cloud restriction applies */
						with_clouds = TRUE;

						/* note: distance calc code is taken from server's distance() */
						/* Calculate distance to cloud focus point 1: */
						dy = (y > cloud_y1[j]) ? (y - cloud_y1[j]) : (cloud_y1[j] - y);
						dx = (x > cloud_x1[j]) ? (x - cloud_x1[j]) : (cloud_x1[j] - x);
						d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
						/* Calculate distance to cloud focus point 2: */
						dy = (y > cloud_y2[j]) ? (y - cloud_y2[j]) : (cloud_y2[j] - y);
						dx = (x > cloud_x2[j]) ? (x - cloud_x2[j]) : (cloud_x2[j] - x);
						/* ..and sum them up */
						d += (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));

						/* distance within cloud? */
						if (d <= cloud_dsum[j] &&
						/* distance near cloud borders? plus, chance to thin
						   out when getting closer to the border. */
						    (rand_int(100) >= (d - ((cloud_dsum[j] * 3) / 4)) * 4))
							outside_clouds = FALSE;
					}
				}
				/* clouds apply but we're not within their areas? discard */
				if (with_clouds && outside_clouds) continue;

				/* (use pos) */
				weather_element_x[weather_elements] = x;
				weather_element_y[weather_elements] = y;

				weather_element_type[weather_elements] = (weather_type == 3 ? (rand_int(2) ? 1 : 2) : weather_type);
				weather_element_ydest[weather_elements] = y + SKY_ALTITUDE;

				/* since pos passed any checks, increase counter to acknowledge */
				weather_elements++;
			}
		}
	}


/* test for weather element movement---------------------------------------- */

	/* horizontal movement: check whether elements are to be shifted by wind this time */
	if (weather_wind) {
		weather_wind_ticks++;
		if (weather_wind % 2 == 1 && weather_wind_ticks == ((weather_wind + 1) / 2)) {
			wind_west_effective = TRUE;
			weather_wind_ticks = 0;
		}
		else if (weather_wind % 2 == 0 && weather_wind_ticks == (weather_wind / 2)) {
			wind_east_effective = TRUE;
			weather_wind_ticks = 0;
		}
	}
	/* vertical movement: check whether elements are to be pulled by gravity this time */
	weather_speed_rain_ticks++;
	weather_speed_snow_ticks++;

	if (weather_speed_rain_ticks == weather_speed_rain) {
		gravity_effective_rain = TRUE;
		weather_speed_rain_ticks = 0;
	}
	if (weather_speed_snow_ticks == weather_speed_snow) {
		gravity_effective_snow = TRUE;
		weather_speed_snow_ticks = 0;
	}

	/* nothing to do this time? - exit */
	if (!gravity_effective_rain && !gravity_effective_snow && !wind_west_effective && !wind_east_effective)
		return;


/* move weather elements --------------------------------------------------- */

#ifdef USE_SOUND_2010
	weather_particles_seen = 0;
#endif

	/* display and advance currently existing weather elements */
	if (screen_icky) Term_switch(0);

	for (i = 0; i < weather_elements; i++) {
		/* restore old tile before moving the weather element */
		/* if panel view was freshly updated from server then no need */
		if (!weather_panel_changed) {
#ifdef BIGMAP_MINDLINK_HACK
			/* big_map mindlink visuals hack */
			if (last_line_y <= SCREEN_HGT &&
			    weather_element_y[i] >= weather_panel_y + SCREEN_HGT)
				;
			else
#endif
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

#ifdef USE_SOUND_2010
 #ifdef BIGMAP_MINDLINK_HACK
		/* big_map mindlink visuals hack */
		if (last_line_y <= SCREEN_HGT &&
		    weather_element_y[i] >= weather_panel_y + SCREEN_HGT)
			;
		else
 #endif
		/* register weather element, if it is currently supposed to be visible on screen */
		if (weather_element_type[i] != 0 &&
		    (weather_element_x[i] >= weather_panel_x &&
		    weather_element_x[i] < weather_panel_x + screen_wid &&
		    weather_element_y[i] >= weather_panel_y &&
		    weather_element_y[i] < weather_panel_y + screen_hgt))
			weather_particles_seen++;
#endif

		/* advance raindrops */
		if (weather_element_type[i] == 1) {
			/* perform movement (y:speed, x:wind) */
			if (gravity_effective_rain) weather_element_y[i]++;
			if (wind_west_effective) weather_element_x[i]++;
			else if (wind_east_effective) weather_element_x[i]--;

			/* pac-man effect for leaving screen to the left/right */
			if (weather_element_x[i] < 1) weather_element_x[i] = MAX_WID - 2;
			else if (weather_element_x[i] > MAX_WID - 2) weather_element_x[i] = 1;

			/* left screen or reached destination? terminate it */
			if (weather_element_y[i] >= MAX_HGT - 2 ||
			    weather_element_y[i] >= weather_element_ydest[i]) {
				/* excise this effect */
				for (j = i + 1; j < weather_elements; j++) {
					weather_element_x[j - 1] = weather_element_x[j];
					weather_element_y[j - 1] = weather_element_y[j];
					weather_element_ydest[j - 1] = weather_element_ydest[j];
					weather_element_type[j - 1] = weather_element_type[j];
				}
				weather_elements--;
				i--;
			}
#ifdef BIGMAP_MINDLINK_HACK
			/* big_map mindlink visuals hack */
			else if (last_line_y <= SCREEN_HGT &&
			    weather_element_y[i] >= weather_panel_y + SCREEN_HGT)
				;
#endif
			/* only for elements within visible panel screen area */
			else if (weather_element_x[i] >= weather_panel_x &&
			    weather_element_x[i] < weather_panel_x + screen_wid &&
			    weather_element_y[i] >= weather_panel_y &&
			    weather_element_y[i] < weather_panel_y + screen_hgt) {
				/* display raindrop */
				Term_draw(PANEL_X + weather_element_x[i] - weather_panel_x,
				    PANEL_Y + weather_element_y[i] - weather_panel_y,
#ifdef EXTENDED_BG_COLOURS
				    rand_int(2) ? TERM2_BLUE : TERM_ORANGE, weather_wind == 0 ? '|' : (weather_wind % 2 == 1 ? '\\' : '/'));
#else
				    TERM_BLUE, weather_wind == 0 ? '|' : (weather_wind % 2 == 1 ? '\\' : '/'));
#endif
			}
		}
		/* advance snowflakes - falling slowly (assumed weather_speed isn't
		   set to silyl value 1 - well, maybe that could be a 'hail storm') */
		else if (weather_element_type[i] == 2) {
			/* perform movement (y:speed, x:wind) */
			if (gravity_effective_snow) weather_element_y[i]++;
			if (wind_west_effective && !rand_int(weather_wind)) weather_element_x[i]++;
			else if (wind_east_effective && !rand_int(weather_wind)) weather_element_x[i]--;

			/* pac-man effect for leaving screen to the left/right */
			if (weather_element_x[i] < 1) weather_element_x[i] = MAX_WID - 2;
			else if (weather_element_x[i] > MAX_WID - 2) weather_element_x[i] = 1;

			/* left screen or reached destination? terminate it */
			if (weather_element_y[i] > MAX_HGT - 2 ||
			    weather_element_y[i] > weather_element_ydest[i]) {
				/* excise this effect */
				for (j = i + 1; j < weather_elements; j++) {
					weather_element_x[j - 1] = weather_element_x[j];
					weather_element_y[j - 1] = weather_element_y[j];
					weather_element_ydest[j - 1] = weather_element_ydest[j];
					weather_element_type[j - 1] = weather_element_type[j];
				}
				weather_elements--;
				i--;
			}
#ifdef BIGMAP_MINDLINK_HACK
			/* big_map mindlink visuals hack */
			else if (last_line_y <= SCREEN_HGT &&
			    weather_element_y[i] >= weather_panel_y + SCREEN_HGT)
				;
#endif
			/* only for elements within visible panel screen area */
			else if (weather_element_x[i] >= weather_panel_x &&
			    weather_element_x[i] < weather_panel_x + screen_wid &&
			    weather_element_y[i] >= weather_panel_y &&
			    weather_element_y[i] < weather_panel_y + screen_hgt) {
				/* display snowflake */
				Term_draw(PANEL_X + weather_element_x[i] - weather_panel_x,
				    PANEL_Y + weather_element_y[i] - weather_panel_y,
				    TERM_WHITE, '*');
			}
		}
	}

	if (screen_icky) Term_switch(0);
	/* Update the screen */
	Term_fresh();


/* clean up and exit ------------------------------------------------------- */

	/* started to draw on a freshly updated panel? */
	weather_panel_changed = FALSE;
}
