/* $Id$ */
/* Handle the printing of info to the screen */



#include "angband.h"

/* make more room for potential new stuff in char dump,
   especially WINNER status line: */
#define NEW_COMPRESSED_DUMP

/*
 * Print character info at given row, column in a 13 char field
 */
static void prt_field(cptr info, int row, int col)
{
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
void cnv_stat(int val, char *out_val)
{
	if (!c_cfg.linear_stats)
	{
		/* Above 18 */
		if (val > 18)
		{
			int bonus = (val - 18);

			if (bonus >= 220)
			{
				sprintf(out_val, "18/%3s", "***");
			}
			else if (bonus >= 100)
			{
				sprintf(out_val, "18/%03d", bonus);
			}
			else
			{
				sprintf(out_val, " 18/%02d", bonus);
			}
		}

		/* From 3 to 18 */
		else
		{
			sprintf(out_val, "    %2d", val);
		}
	}
	else
	{
		/* Above 18 */
		if (val > 18)
		{
			int bonus = (val - 18);

			if (bonus >= 220)
			{
				sprintf(out_val, "    40");
			}
			else
			{
				sprintf(out_val, "    %2d", 18 + (bonus / 10));
			}
		}

		/* From 3 to 18 */
		else
		{
			sprintf(out_val, "    %2d", val);
		}
	}
}


/*
 * Print character stat in given row, column
 */
void prt_stat(int stat, int max, int cur, int cur_base)
{
	char tmp[32];
	int x, y;

	if (client_mode == CLIENT_PARTY)
	{
		return;
	}

	/* remember cursor position */
	Term_locate(&x, &y);

	if (cur < max)
	{
		Term_putstr(0, ROW_STAT + stat, -1, TERM_WHITE, (char*)stat_names_reduced[stat]);
		cnv_stat(cur, tmp);
		Term_putstr(COL_STAT + 6, ROW_STAT + stat, -1, TERM_YELLOW, tmp);
	}

	else
	{
		Term_putstr(0, ROW_STAT + stat, -1, TERM_WHITE, (char*)stat_names[stat]);
		cnv_stat(cur, tmp);
		if(cur_base < (18 + 100))
		{
			Term_putstr(COL_STAT + 6, ROW_STAT + stat, -1, TERM_L_GREEN, tmp);
		}
		else
		{
			Term_putstr(COL_STAT + 6, ROW_STAT + stat, -1, TERM_L_UMBER, tmp);
		}
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints "title", including "wizard" or "winner" as needed.
 */
void prt_title(cptr title)
{
	prt_field(title, ROW_TITLE, COL_TITLE);
}


/*
 * Prints level and experience
 */
void prt_level(int level, int max_lev, int max_plv, s32b max, s32b cur, s32b adv)
{
	char tmp[32];
	int colour = (level < max_lev) ? TERM_YELLOW : TERM_L_GREEN;
	int x, y;

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

	if (!c_cfg.exp_need)
	{
		sprintf(tmp, "%9d", (int)cur);
	}
	else
	{
		if (level >= PY_MAX_LEVEL || !adv)
		{
			(void)sprintf(tmp, "*********");
		}
		else
		{
			/* Hack -- display in minus (to avoid confusion chez player) */
			(void)sprintf(tmp, "%9d", (int)(cur - adv));
		}
	}

	if (cur >= max)
	{
                Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "XP ");
                Term_putstr(COL_EXP + 3, ROW_EXP, -1, TERM_L_GREEN, tmp);
	}
	else
	{
                Term_putstr(0, ROW_EXP, -1, TERM_WHITE, "Xp ");
                Term_putstr(COL_EXP + 3, ROW_EXP, -1, TERM_YELLOW, tmp);
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints current gold
 */
void prt_gold(int gold)
{
	char tmp[32];
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	put_str("AU ", ROW_GOLD, COL_GOLD);
	sprintf(tmp, "%9ld", (long)gold);
	c_put_str(TERM_L_GREEN, tmp, ROW_GOLD, COL_GOLD + 3);

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints current AC
 */
void prt_ac(int ac)
{
	char tmp[32];
	int x, y;
	if (client_mode == CLIENT_PARTY)
	{
		return;
	}

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
void prt_hp(int max, int cur)
{
	char tmp[32];
	byte color;
	int x, y; /* for remembering cursor pos */

	/* remember cursor position */
	Term_locate(&x, &y);

	if (client_mode == CLIENT_PARTY)
	{
		color = TERM_L_RED;
		sprintf(tmp, "HP: %4d ", max);
		c_put_str(color, tmp, CLIENT_PARTY_ROWHP, CLIENT_PARTY_COLHP);

		sprintf(tmp, "%4d", cur);

		if (cur >= max)
		{
			color = TERM_L_RED;
		}
		else if (cur > max / 10)
		{
			color = TERM_YELLOW;
		}
		else
		{
			color = TERM_RED;
		}
		c_put_str(color, tmp, CLIENT_PARTY_ROWHP, CLIENT_PARTY_COLHP + 9);
	}
	/* DEG Default to else since only 2 types for now */
	else
	{
#ifndef CONDENSED_HP_SP
		put_str("Max HP ", ROW_MAXHP, COL_MAXHP);
		sprintf(tmp, "%5d", max);
		color = TERM_L_GREEN;
		c_put_str(color, tmp, ROW_MAXHP, COL_MAXHP + 7);

		put_str("Cur HP ", ROW_CURHP, COL_CURHP);

		sprintf(tmp, "%5d", cur);

		if (cur >= max)
		{
			color = TERM_L_GREEN;
		}
		else if (cur > max / 10)
		{
			color = TERM_YELLOW;
		}
		else
		{
			color = TERM_RED;
		}

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
void prt_stamina(int max, int cur)
{
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
void prt_party_stats(int member_num, byte color, char *member_name, int member_lev, int member_chp, int member_mhp, int member_csp, int member_msp)
{
	char tmp[32];
	int rowspacing = 0;

	if (member_num != 0)
	{
		rowspacing += (4 * member_num);
	}

	if (client_mode != CLIENT_PARTY)
	{
		return;
	}

	if (member_name[0] == '\0' && member_lev == 0) {
		/* Empty member? Just clear it - mikaelh */
		int i;
		for (i = CLIENT_PARTY_ROWMBR + rowspacing; i < CLIENT_PARTY_ROWMBR + rowspacing + 3; i++) {
			Term_erase(0, i, 12);
		}
		return;
	}

	sprintf(tmp, "%s", member_name);
	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing) ,CLIENT_PARTY_COLMBR);

	sprintf(tmp, "HP: %4d ", member_mhp);
	color = TERM_L_RED;
	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing + 1) ,CLIENT_PARTY_COLMBR);

	sprintf(tmp, "%4d", member_chp);

	if (member_chp >= member_mhp)
	{
		color = TERM_L_RED;
	}
	else if (member_chp > member_mhp / 10)
	{
		color = TERM_YELLOW;
	}
	else
	{
		color = TERM_RED;
	}

	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing + 1) ,CLIENT_PARTY_COLMBR + 9);


	sprintf(tmp, "SP: %4d ", member_msp);
	color = TERM_L_BLUE;
	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing + 2) , CLIENT_PARTY_COLMBR + 0);

	sprintf(tmp, "%4d", member_csp);

	if (member_csp >= member_msp)
	{
		color = TERM_L_BLUE;
	}
	else if (member_csp > member_msp / 10)
	{
		color = TERM_YELLOW;
	}
	else
	{
		color = TERM_RED;
	}

	c_put_str(color, tmp, (CLIENT_PARTY_ROWMBR + rowspacing + 2) , CLIENT_PARTY_COLMBR + 9);
}


/*
 * Prints Max/Cur spell points
 */
void prt_sp(int max, int cur)
{
	char tmp[32];
	byte color;
	int x, y; /* for remembering cursor pos */

	/* remember cursor position */
	Term_locate(&x, &y);

	if (client_mode == CLIENT_PARTY)
	{
		sprintf(tmp, "MP: %4d ", max);
		color = TERM_L_BLUE;
		c_put_str(color, tmp, CLIENT_PARTY_ROWSP, CLIENT_PARTY_COLSP);

		sprintf(tmp, "%4d", cur);
		color = TERM_L_GREEN;

		if (cur >= max)
		{
			color = TERM_L_BLUE;
		}
		else if (cur > max / 10)
		{
			color = TERM_YELLOW;
		}
		else
		{
			color = TERM_RED;
		}

		c_put_str(color, tmp, CLIENT_PARTY_ROWSP, CLIENT_PARTY_COLSP + 9);
	}
	else
	{
#ifndef CONDENSED_HP_SP
		put_str("Max MP ", ROW_MAXSP, COL_MAXSP);

		sprintf(tmp, "%5d", max);
		color = TERM_L_GREEN;

		c_put_str(color, tmp, ROW_MAXSP, COL_MAXSP + 7);


		put_str("Cur MP ", ROW_CURSP, COL_CURSP);

		sprintf(tmp, "%5d", cur);

		if (cur >= max)
		{
			color = TERM_L_GREEN;
		}
		else if (cur > max / 10)
		{
			color = TERM_YELLOW;
		}
		else
		{
			color = TERM_RED;
		}

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
void prt_sane(byte attr, cptr buf)
{
	int x, y;

	if (client_mode == CLIENT_PARTY)
	{
		return;
	}

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
void prt_depth(int x, int y, int z, bool town, int colour, int colour_sector, cptr name)
{
	char depths[32];
	int x2, y2;

	/* Remember coordinates */
	p_ptr->wpos.wx = x;
	p_ptr->wpos.wy = y;
	p_ptr->wpos.wz = z;

	/* Remember everything else */
	depth_town = town;
	depth_colour = colour;
	depth_colour_sector = colour_sector;
	strcpy(depth_name, name);

	/* remember cursor position */
	Term_locate(&x2, &y2);

	sprintf(depths, "(%-2d,%-2d)",x,y);
	c_put_str(colour_sector, depths, ROW_XYPOS, COL_XYPOS);

	if(town)
		strcpy(depths, name);
	else if (c_cfg.depth_in_feet)
		sprintf(depths, "%dft", z*50);
	else
		sprintf(depths, "Lev %d", z);

	/* Right-Adjust the "depth" and clear old values */
	c_prt(colour, format("%7s", depths), ROW_DEPTH, COL_DEPTH);

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
void prt_hunger(int food)
{
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (food < PY_FOOD_FAINT)
	{
		c_put_str(TERM_RED, "Weak  ", ROW_HUNGRY, COL_HUNGRY);
	}

	else if (food < PY_FOOD_WEAK)
	{
		c_put_str(TERM_ORANGE, "Weak  ", ROW_HUNGRY, COL_HUNGRY);
	}

	else if (food < PY_FOOD_ALERT)
	{
		c_put_str(TERM_YELLOW, "Hungry", ROW_HUNGRY, COL_HUNGRY);
	}

	else if (food < PY_FOOD_FULL)
	{
		c_put_str(TERM_L_GREEN, "      ", ROW_HUNGRY, COL_HUNGRY);
	}

	else if (food < PY_FOOD_MAX)
	{
		c_put_str(TERM_L_GREEN, "Full  ", ROW_HUNGRY, COL_HUNGRY);
	}

	else
	{
		c_put_str(TERM_GREEN, "Gorged", ROW_HUNGRY, COL_HUNGRY);
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints blindness status
 */
void prt_blind(bool blind)
{
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (blind)
	{
		c_put_str(TERM_ORANGE, "Blind", ROW_BLIND, COL_BLIND);
	}
	else
	{
		put_str("     ", ROW_BLIND, COL_BLIND);
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints confused status
 */
void prt_confused(bool confused)
{
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (confused)
	{
		c_put_str(TERM_ORANGE, "Confused", ROW_CONFUSED, COL_CONFUSED);
	}
	else
	{
		put_str("        ", ROW_CONFUSED, COL_CONFUSED);
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints fear status
 */
void prt_afraid(bool fear)
{
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (fear)
	{
		c_put_str(TERM_ORANGE, "Afraid", ROW_AFRAID, COL_AFRAID);
	}
	else
	{
		put_str("      ", ROW_AFRAID, COL_AFRAID);
	}
	
	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints poisoned status
 */
void prt_poisoned(bool poisoned)
{
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (poisoned)
	{
		c_put_str(TERM_ORANGE, "Poisoned", ROW_POISONED, COL_POISONED);
	}
	else
	{
		put_str("        ", ROW_POISONED, COL_POISONED);
	}
	
	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints paralyzed/searching status
 */
void prt_state(bool paralyzed, bool searching, bool resting)
{
	byte attr = TERM_WHITE;
	char text[16];
	int x, y;

	if (paralyzed)
	{
		attr = TERM_RED;

		strcpy(text, "Paralyzed!  ");
	}

	else if (searching)
	{
#if 0
		if (get_skill(SKILL_STEALTH) <= 10)
		{
			strcpy(text, "Searching   ");
		}

		else
		{
			attr = TERM_L_DARK;
			strcpy(text,"Stlth Mode  ");
		}
#else
		strcpy(text, "Searching   ");
#endif

	}

	else if (resting)
	{
		strcpy(text, "Resting     ");
	}
	else
	{
		strcpy(text, "            ");
	}

	/* remember cursor position */
	Term_locate(&x, &y);

	c_put_str(attr, text, ROW_STATE, COL_STATE);
	
	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints speed
 */
void prt_speed(int speed)
{
	int attr = TERM_WHITE;
	char buf[32] = "";
	int x, y;

	if (speed > 0) {
		attr = TERM_L_GREEN;
		sprintf(buf, "Fast (+%d)", speed);
	} else if (speed < 0) {
		attr = TERM_L_UMBER;
		sprintf(buf, "Slow (%d)", speed);
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
 */
void prt_study(bool study)
{
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (study)
	{
		put_str("Skill", ROW_STUDY, COL_STUDY);
	}
	else
	{
		put_str("     ", ROW_STUDY, COL_STUDY);
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints cut status
 */
void prt_cut(int cut)
{
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (cut > 1000)
	{
		c_put_str(TERM_L_RED, "Mortal wound", ROW_CUT, COL_CUT);
	}
	else if (cut > 200)
	{
		c_put_str(TERM_RED, "Deep gash  ", ROW_CUT, COL_CUT);
	}
	else if (cut > 100)
	{
		c_put_str(TERM_RED, "Severe cut  ", ROW_CUT, COL_CUT);
	}
	else if (cut > 50)
	{
		c_put_str(TERM_ORANGE, "Nasty cut   ", ROW_CUT, COL_CUT);
	}
	else if (cut > 25)
	{
		c_put_str(TERM_ORANGE, "Bad cut     ", ROW_CUT, COL_CUT);
	}
	else if (cut > 10)
	{
		c_put_str(TERM_YELLOW, "Light cut  ", ROW_CUT, COL_CUT);
	}
	else if (cut)
	{
		c_put_str(TERM_YELLOW, "Graze     ", ROW_CUT, COL_CUT);
	}
	else
	{
		put_str("            ", ROW_CUT, COL_CUT);
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints stun status
 */
void prt_stun(int stun)
{
	int x, y;

	/* remember cursor position */
	Term_locate(&x, &y);

	if (stun > 100)
	{
		c_put_str(TERM_RED, "Knocked out ", ROW_STUN, COL_STUN);
	}
	else if (stun > 50)
	{
		c_put_str(TERM_ORANGE, "Heavy stun  ", ROW_STUN, COL_STUN);
	}
	else if (stun)
	{
		c_put_str(TERM_ORANGE, "Stun        ", ROW_STUN, COL_STUN);
	}
	else
	{
		put_str("            ", ROW_STUN, COL_STUN);
	}

	/* restore cursor position */
	Term_gotoxy(x, y);
}

/*
 * Prints race/class info
 */
void prt_basic(void)
{
	cptr r=NULL, c=NULL;

        r = p_ptr->rp_ptr->title;

        c = p_ptr->cp_ptr->title;

	prt_field((char *)r, ROW_RACE, COL_RACE);
	prt_field((char *)c, ROW_CLASS, COL_CLASS);
}

/* Print AFK status */
void prt_AFK(byte afk)
{
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

void prt_extra_status(cptr status)
{
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

	num = lag / 50 + 1;
	if (num > 10) num = 10;

	/* hack: we have previous packet assumed to be lost? */
	if (lag == 9999) num = 10;
	
	if (num >= 9) attr = TERM_RED;
	else if (num >= 7) attr = TERM_ORANGE;
	else if (num >= 5) attr = TERM_YELLOW;
	else if (num >= 3) attr = TERM_GREEN;

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
	if (!attr)
	{
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
	else
	{
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
static void display_inven(void)
{
	int	i, n, z = 0;
	long int	wgt;

	object_type *o_ptr;

	char	o_name[80];

	char	tmp_val[80];

	/* Find the "final" slot */
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &inventory[i];

		/* Track non-empty slots */
		if (o_ptr->tval) z = i + 1;
	}

	/* Display the inventory */
	for (i = 0; i < z; i++)
	{
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
		if (item_tester_okay(o_ptr))
		{
			/* Display the index */
			Term_putstr(0, i, 3, TERM_WHITE, tmp_val);
		}
		else
		{
			/* Grey out the index */
			Term_putstr(0, i, 3, TERM_L_DARK, tmp_val);
		}

		/* Describe the object */
		strcpy(o_name, inventory_name[i]);

		/* Obtain length of description */
		n = strlen(o_name);

		/* Clear the line with the (possibly indented) index */
		Term_putstr(3, i, n, o_ptr->attr, o_name);

		/* Erase the rest of the line */
		Term_erase(3+n, i, 255);

		/* Display the weight if needed */
		if (c_cfg.show_weights && o_ptr->weight)
		{
			wgt = o_ptr->weight * o_ptr->number;
			if (wgt < 10000) /* still fitting into 3 digits? */
				(void)sprintf(tmp_val, "%3li.%1li lb", wgt / 10, wgt % 10);
			else
				(void)sprintf(tmp_val, "%3lik%1li lb", wgt / 10000, (wgt % 10000) / 1000);
			Term_putstr(71, i, -1, TERM_WHITE, tmp_val);
		}
	}

	/* Erase the rest of the window */
	for (i = z; i < Term->hgt; i++)
	{
		/* Erase the line */
		Term_erase(0, i, 255);
	}
}


/*
 * Choice window "shadow" of the "show_equip()" function.
 */
static void display_equip(void)
{
	int	i, n;
	long int	wgt;

	object_type *o_ptr;

	char	o_name[80];

	char	tmp_val[80];

	/* Find the "final" slot */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		o_ptr = &inventory[i];

		/* Start with an empty "index" */
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

		/* Is this item acceptable? */
		if (item_tester_okay(o_ptr))
		{
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
		Term_erase(3+n, i - INVEN_WIELD, 255);

		/* Display the weight if needed */
		if (c_cfg.show_weights && o_ptr->weight)
		{
			wgt = o_ptr->weight * o_ptr->number;
			if (wgt < 10000) /* still fitting into 3 digits? */
				(void)sprintf(tmp_val, "%3li.%1li lb", wgt / 10, wgt % 10);
			else
				(void)sprintf(tmp_val, "%3lik%1li lb", wgt / 10000, (wgt % 10000) / 1000);
			Term_putstr(71, i - INVEN_WIELD, -1, TERM_WHITE, tmp_val);
		}
	}

	/* Erase the rest of the window */
	for (i = INVEN_TOTAL - INVEN_WIELD; i < Term->hgt; i++)
	{
		/* Erase the line */
		Term_erase(0, i, 255);
	}
}


/*
 * Display the inventory.
 *
 * Hack -- do not display "trailing" empty slots
 */
void show_inven(void)
{
	int	i, j, k, l, z = 0;
	int	col, len, lim;
	long int wgt, totalwgt = 0;

	object_type *o_ptr;

	char	o_name[80];

	char	tmp_val[80];

	int	out_index[23];
	byte	out_color[23];
	char	out_desc[23][80];


	/* Starting column */
	col = command_gap;

	/* Default "max-length" */
	len = 79 - col;

	/* Maximum space allowed for descriptions */
	lim = 79 - 3;

	/* Require space for weight (if needed) */
	lim -= 9;


	/* Find the "final" slot */
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &inventory[i];

		/* Track non-empty slots */
		if (o_ptr->tval) z = i + 1;
	}

	/* Display the inventory */
	for (k = 0, i = 0; i < z; i++)
	{
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
	for (j = 0; j < k; j++)
	{
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
		if (c_cfg.show_weights && o_ptr->weight)
		{
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
	if (c_cfg.show_weights && totalwgt)
	{
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

	/* Save the new column */
	command_gap = col;
}


/*
 * Display the equipment.
 */
void show_equip(void)
{
	int	i, j, k, l;
	int	col, len, lim;
	long int wgt, totalwgt = 0;

	object_type *o_ptr;

	char	o_name[80];

	char	tmp_val[80];

	int	out_index[23];
	byte	out_color[23];
	char	out_desc[23][80];


	/* Starting column */
	col = command_gap;

	/* Default "max-length" */
	len = 79 - col;

	/* Maximum space allowed for descriptions */
	lim = 79 - 3;

	/* Require space for weight (if needed) */
	lim -= 9;


	/* Scan the equipment list */
	for (k = 0, i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
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
	for (j = 0; j < k; j++)
	{
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
		if (c_cfg.show_weights && o_ptr->weight)
		{
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
	if (c_cfg.show_weights && totalwgt)
	{
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

	/* Save the new column */
	command_gap = col;
}


/*
 * Display inventory in sub-windows
 */
static void fix_inven(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
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
}


/*
 * Display equipment in sub-windows
 */
static void fix_equip(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
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
}


/*
 * Display character sheet in sub-windows
 */
static void fix_player(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		term *old = Term;

		/* No window */
		if (!ang_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & PW_PLAYER)) continue;

		/* Activate */
		Term_activate(ang_term[j]);

		/* Display inventory */
		display_player(FALSE);

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
static void fix_message(void)
{
        int j, i;
        int w, h;
        int x, y;

	cptr msg;
	byte a;

	/* Display messages in different colors -Zz */

        /* Scan windows */
        for (j = 0; j < 8; j++)
        {
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
	                for (i = 0; i < h; i++)
	                {
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
	                for (i = 0; i < h; i++)
	                {
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
	for (j = 0; j < 8; j++)
	{
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
static cptr likert(int x, int y, int max)
{
	/* Paranoia */
	if (y <= 0) y = 1;

	/* Negative values */
	if (x < 0)
	{
		likert_color = TERM_RED;
		return ("Very Bad");
	}

	/* Highest possible value reached */
        if ((x >= max) && max)
        {
                likert_color = TERM_L_UMBER;
                return ("Legendary");
        }

	/* Analyze the value */
	switch (((x * 10) / y))
	{
		case 0:
		case 1:
		{
			likert_color = TERM_RED;
			return ("Bad");
		}
		case 2:
		{
			likert_color = TERM_RED;
			return ("Poor");
		}
		case 3:
		case 4:
		{
			likert_color = TERM_YELLOW;
			return ("Fair");
		}
		case 5:
		{
			likert_color = TERM_YELLOW;
			return ("Good");
		}
		case 6:
		{
			likert_color = TERM_YELLOW;
			return ("Very Good");
		}
		case 7:
		case 8:
		{
			likert_color = TERM_L_GREEN;
			return ("Excellent");
		}
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		{
			likert_color = TERM_L_GREEN;
			return ("Superb");
		}
		case 14:
		case 15:
		case 16:
		case 17:
		{
			likert_color = TERM_L_GREEN;
			return ("Heroic");
		}
		default:
		{
			/* indicate that there is a maximum value */
			if (max) likert_color = TERM_GREEN;
			/* indicate that there is no maximum */
			else likert_color = TERM_L_GREEN;
			return ("Legendary");
		}
	}
}

/*
 * Draws the lag-o-meter.
 */
void display_lagometer(bool display_commands)
{
	int i, cnt, sum, cur, min, max, avg, x, y, packet_loss, height;
	char tmp[80];
	char graph[16][61];

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
		prt(graph[y], 3 + y, 10);
	}

	prt("Cur:", 19, 2);
	if (cur != -1) sprintf(tmp, "%5dms", cur);
	else tmp[0] = '\0';
	prt(tmp, 19, 7);

	prt("Avg:", 19, 17);
	if (avg != -1) sprintf(tmp, "%5dms", avg);
	else tmp[0] = '\0';
	prt(tmp, 19, 22);

	prt("Min:", 19, 32);
	if (min != -1) sprintf(tmp, "%5dms", min);
	else tmp[0] = '\0';
	prt(tmp, 19, 37);

	prt("Max:", 19, 47);
	if (max != -1) sprintf(tmp, "%5dms", max);
	else tmp[0] = '\0';
	prt(tmp, 19, 52);

	prt("Packet Loss:", 19, 62);
	sprintf(tmp, "%2d", packet_loss);
	prt(tmp, 19, 75);

	if (display_commands) {
		if (lagometer_enabled) {
			prt("(2) Disable lag-o-meter", 21, 4);
		}
		else {
			prt("(1) Enable lag-o-meter", 21, 4);
		}

		prt("Command: ", 22, 2);
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
	window_stuff();
}

void display_player(int hist)
{
	int i;
	char buf[80];
	cptr desc;

        /* Clear screen */
	clear_from(0);

        /* Name, Sex, Race, Class */
        put_str("Name        :", 1, 1);
        put_str("Sex         :", 2, 1);
        put_str("Race        :", 3, 1);
        put_str("Class       :", 4, 1);
        put_str("Body        :", 5, 1);
	put_str("Mode        :", 6, 1);

        c_put_str(TERM_L_BLUE, cname, 1, 15);
        c_put_str(TERM_L_BLUE, (p_ptr->male ? "Male" : "Female"), 2, 15);
        c_put_str(TERM_L_BLUE, race_info[race].title, 3, 15);
        c_put_str(TERM_L_BLUE, class_info[class].title, 4, 15);
        c_put_str(TERM_L_BLUE, c_p_ptr->body_name, 5, 15);
	if (p_ptr->mode & MODE_EVERLASTING)
	    	c_put_str(TERM_L_BLUE, "Everlasting (infinite lives)", 6, 15);
	else if ((p_ptr->mode & MODE_NO_GHOST) && (p_ptr->mode & MODE_HARD))
	    	c_put_str(TERM_L_BLUE, "Hellish (one life, extra hard)", 6, 15);
	else if (p_ptr->mode & MODE_NO_GHOST)
	    	c_put_str(TERM_L_BLUE, "Unworldly (one life)", 6, 15);
        else if (p_ptr->mode & MODE_HARD)
        	c_put_str(TERM_L_BLUE, "Hard (3 lives, extra hard)", 6, 15);
        else /*(p_ptr->mode == MODE_NORMAL)*/
	    	c_put_str(TERM_L_BLUE, "Normal (3 lives)", 6, 15);

        /* Age, Height, Weight, Social */
        prt_num("Age          ", (int)p_ptr->age, 1, 32, TERM_L_BLUE);
        prt_num("Height       ", (int)p_ptr->ht, 2, 32, TERM_L_BLUE);
        prt_num("Weight       ", (int)p_ptr->wt, 3, 32, TERM_L_BLUE);
        prt_num("Social Class ", (int)p_ptr->sc, 4, 32, TERM_L_BLUE);

        /* Display the stats */
        for (i = 0; i < 6; i++)
        {
                /* Special treatment of "injured" stats */
                if (p_ptr->stat_use[i] < p_ptr->stat_top[i])
                {
                        int value;

                        /* Use lowercase stat name */
                        put_str(stat_names_reduced[i], 1 + i, 61);

                        /* Get the current stat */
                        value = p_ptr->stat_use[i];

                        /* Obtain the current stat (modified) */
                        cnv_stat(value, buf);

                        /* Display the current stat (modified) */
                        c_put_str(TERM_YELLOW, buf, 1 + i, 66);

                        /* Acquire the max stat */
                        value = p_ptr->stat_top[i];

                        /* Obtain the maximum stat (modified) */
                        cnv_stat(value, buf);

                        /* Display the maximum stat (modified) */
			if(p_ptr->stat_cur[i] < (18 + 100))
			{
                    		c_put_str(TERM_L_GREEN, buf, 1 + i, 73);
			}
			else
			{
                    		c_put_str(TERM_L_UMBER, buf, 1 + i, 73);
			}
                }

                /* Normal treatment of "normal" stats */
                else
                {
                        /* Assume uppercase stat name */
                        put_str(stat_names[i], 1 + i, 61);

                        /* Obtain the current stat (modified) */
                        cnv_stat(p_ptr->stat_use[i], buf);

                        /* Display the current stat (modified) */
			if(p_ptr->stat_cur[i] < (18 + 100))
			{
    		                c_put_str(TERM_L_GREEN, buf, 1 + i, 66);
			}
			else
			{
    		                c_put_str(TERM_L_UMBER, buf, 1 + i, 66);
			}
                }
        }

	/* Check for history */
	if (hist)
	{
		put_str("(Character Background)", 14, 25);

		for (i = 0; i < 4; i++)
		{
			put_str(p_ptr->history[i], i + 15, 10);
		}
	}
	else
	{
#ifndef NEW_COMPRESSED_DUMP
		put_str("(Miscellaneous Abilities)", 14, 25);
#endif

		/* Display "skills" */
		put_str("Fighting    :", 15, 1);
		desc = likert(p_ptr->skill_thn, 120, 0);
		c_put_str(likert_color, desc, 15, 15);

		put_str("Bows/Throw  :", 16, 1);
		desc = likert(p_ptr->skill_thb, 120, 0);
		c_put_str(likert_color, desc, 16, 15);

		put_str("Saving Throw:", 17, 1);
		desc = likert(p_ptr->skill_sav, 52, 95);	/*was 6.0 before x10 increase */
		c_put_str(likert_color, desc, 17, 15);

		put_str("Stealth     :", 18, 1);
		desc = likert(p_ptr->skill_stl, 10, 30);
		c_put_str(likert_color, desc, 18, 15);


		put_str("Perception  :", 15, 28);
		desc = likert(p_ptr->skill_fos, 40, 75);
		c_put_str(likert_color, desc, 15, 42);

		put_str("Searching   :", 16, 28);
		desc = likert(p_ptr->skill_srh, 60, 100);
		c_put_str(likert_color, desc, 16, 42);

		put_str("Disarming   :", 17, 28);
		desc = likert(p_ptr->skill_dis, 80, 100);
		c_put_str(likert_color, desc, 17, 42);

		put_str("Magic Device:", 18, 28);
		desc = likert(p_ptr->skill_dev, 60, 0);
		c_put_str(likert_color, desc, 18, 42);


		put_str("Blows/Round:", 15, 55);
		c_put_str(TERM_L_BLUE, format("%d", p_ptr->num_blow), 15, 69);

		put_str("Shots/Round:", 16, 55);
		c_put_str(TERM_L_BLUE, format("%d", p_ptr->num_fire), 16, 69);
#if 0
		put_str("Spells/Round:", 17, 55);
		c_put_str(TERM_L_BLUE, format("%d", p_ptr->num_spell), 17, 69);
#endif
		put_str("Infra-Vision:", 18, 55);
		c_put_str(TERM_L_BLUE, format("%d feet", p_ptr->see_infra * 10), 18, 69);
	}

	/* Dump the bonuses to hit/dam */
	prt_num("+To MHit    ", p_ptr->dis_to_h + p_ptr->to_h_melee, 8, 1, TERM_L_BLUE);
	prt_num("+To MDamage ", p_ptr->dis_to_d + p_ptr->to_d_melee, 9, 1, TERM_L_BLUE);
	prt_num("+To RHit    ", p_ptr->dis_to_h + p_ptr->to_h_ranged, 10, 1, TERM_L_BLUE);
	prt_num("+To RDamage ", p_ptr->to_d_ranged, 11, 1, TERM_L_BLUE);
	/* Dump the armor class bonus */
	prt_num("+ To AC     ", p_ptr->dis_to_a, 12, 1, TERM_L_BLUE);
	/* Dump the total armor class */
	prt_num("  Base AC   ", p_ptr->dis_ac, 13, 1, TERM_L_BLUE);

	prt_num("Level      ", (int)p_ptr->lev, 8, 28, TERM_L_GREEN);
	if (p_ptr->exp >= p_ptr->max_exp)
	{
		prt_lnum("Experience ", p_ptr->exp, 9, 28, TERM_L_GREEN);
	}
	else
	{
		prt_lnum("Experience ", p_ptr->exp, 9, 28, TERM_YELLOW);
	}
	prt_lnum("Max Exp    ", p_ptr->max_exp, 10, 28, TERM_L_GREEN);
	if (p_ptr->lev >= PY_MAX_LEVEL || !exp_adv)
	{
		put_str("Exp to Adv.", 11, 28);
		c_put_str(TERM_L_GREEN, "    *****", 11, 28+11);
	}
	else
	{
		prt_lnum("Exp to Adv.", exp_adv, 11, 28, TERM_L_GREEN);
	}
	prt_lnum("Gold       ", p_ptr->au, 12, 28, TERM_L_GREEN);

#ifndef NEW_COMPRESSED_DUMP
	if (p_ptr->mhp == -9999) {
		put_str("Max Hit Points         ", 8, 52);
		c_put_str(TERM_L_GREEN, "-", 8, 75);
	} else {
		prt_num("Max Hit Points ", p_ptr->mhp, 8, 52, TERM_L_GREEN);
	}

	if (p_ptr->chp == -9999) {
		put_str("Cur Hit Points         ", 9, 52);
		c_put_str(TERM_L_GREEN, "-", 9, 75);
	} else if (p_ptr->chp >= p_ptr->mhp)
	{
		prt_num("Cur Hit Points ", p_ptr->chp, 9, 52, TERM_L_GREEN);
	}
	else if (p_ptr->chp > (p_ptr->mhp) / 10)
	{
		prt_num("Cur Hit Points ", p_ptr->chp, 9, 52, TERM_YELLOW);
	}
	else
	{
		prt_num("Cur Hit Points ", p_ptr->chp, 9, 52, TERM_RED);
	}

	if (p_ptr->msp == -9999) {
		put_str("Max MP (Mana)          ", 10, 52);
		c_put_str(TERM_L_GREEN, "-", 10, 75);
	} else {
		prt_num("Max MP (Mana)  ", p_ptr->msp, 10, 52, TERM_L_GREEN);
	}

    	if (p_ptr->csp == -9999) {
		put_str("Cur MP (Mana)          ", 11, 52);
		c_put_str(TERM_L_GREEN, "-", 11, 75);
	} else if (p_ptr->csp >= p_ptr->msp)
	{
		prt_num("Cur MP (Mana)  ", p_ptr->csp, 11, 52, TERM_L_GREEN);
	}
	else if (p_ptr->csp > (p_ptr->msp) / 10)
	{
		prt_num("Cur MP (Mana)  ", p_ptr->csp, 11, 52, TERM_YELLOW);
	}
	else
	{
		prt_num("Cur MP (Mana)  ", p_ptr->csp, 11, 52, TERM_RED);
	}
 #ifdef SHOW_SANITY
	put_str("Cur Sanity", 12, 52);

	c_put_str(c_p_ptr->sanity_attr, c_p_ptr->sanity, 12, 67);
 #endif	/* SHOW_SANITY */

#else

	if (p_ptr->mhp == -9999) {
		put_str("Hit Points", 8, 52);
		c_put_str(TERM_L_GREEN, "-", 8, 71);
	} else {
		prt_num("Hit Points     ", p_ptr->mhp, 8, 52, TERM_L_GREEN);
		c_put_str(TERM_L_GREEN, "/", 8, 71);
		if (p_ptr->chp >= p_ptr->mhp) {
			prt_num("", p_ptr->chp, 8, 62, TERM_L_GREEN);
		} else if (p_ptr->chp > (p_ptr->mhp) / 10) {
			prt_num("", p_ptr->chp, 8, 62, TERM_YELLOW);
		} else {
			prt_num("", p_ptr->chp, 8, 62, TERM_RED);
		}
	}

	if (p_ptr->msp == -9999) {
		put_str("MP (Mana)", 9, 52);
		c_put_str(TERM_L_GREEN, "-", 9, 71);
	} else {
		prt_num("MP (Mana)      ", p_ptr->msp, 9, 52, TERM_L_GREEN);
		c_put_str(TERM_L_GREEN, "/", 9, 71);
		if (p_ptr->csp >= p_ptr->msp) {
			prt_num("", p_ptr->csp, 9, 62, TERM_L_GREEN);
		} else if (p_ptr->csp > (p_ptr->msp) / 10) {
			prt_num("", p_ptr->csp, 9, 62, TERM_YELLOW);
		} else {
			prt_num("", p_ptr->csp, 9, 62, TERM_RED);
		}
	}

 #ifdef SHOW_SANITY
	put_str("Sanity", 10, 52);
	c_put_str(c_p_ptr->sanity_attr, c_p_ptr->sanity, 10, 67);
 #endif	/* SHOW_SANITY */

	/* Display 'WINNER' status */
	put_str("Status", 12, 52);
	if (p_ptr->chp < 0 || c_p_ptr->sanity_attr == TERM_RED) c_put_str(TERM_L_DARK, "        DEAD", 12, 64);
	else if (p_ptr->ghost) c_put_str(TERM_RED, "Ghost (dead)", 12, 64);
	else if (p_ptr->total_winner) c_put_str(TERM_VIOLET, "***WINNER***", 12, 64);
	else c_put_str(TERM_L_GREEN, "       Alive", 12, 64);
#endif

	/* Show location (better description needed XXX) */
	if (c_cfg.depth_in_feet)
		put_str(format("You are at %s%d ft%s of world map sector %d,%d.", 
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz * 50,
					p_ptr->wpos.wz ? "" : ")",
					p_ptr->wpos.wx, p_ptr->wpos.wy), 20, hist ? 10 : 1);
	else
		put_str(format("You are at %slevel %d%s of world map sector %d,%d.",
					p_ptr->wpos.wz ? "" : "surface (", p_ptr->wpos.wz,
					p_ptr->wpos.wz ? "" : ")",
					p_ptr->wpos.wx, p_ptr->wpos.wy), 20, hist ? 10 : 1);
}

/*
 * Redraw any necessary windows
 */
void window_stuff(void)
{
	/* Window stuff */
	if (!p_ptr->window) return;

	/* Display inventory */
	if (p_ptr->window & PW_INVEN)
	{
		p_ptr->window &= ~(PW_INVEN);
		fix_inven();
	}

	/* Display equipment */
	if (p_ptr->window & PW_EQUIP)
	{
		p_ptr->window &= (~PW_EQUIP);
		fix_equip();
	}

	/* Display player */
	if (p_ptr->window & PW_PLAYER)
	{
		p_ptr->window &= (~PW_PLAYER);
		fix_player();
	}

	/* Display messages */
	if (p_ptr->window & (PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT))
	{
		p_ptr->window &= (~(PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT));
		fix_message();
	}

	/* Display the lag-o-meter */
	if (p_ptr->window & (PW_LAGOMETER))
	{
		p_ptr->window &= (~PW_LAGOMETER);
		fix_lagometer();
	}
}

/* Handle weather (rain and snow) client-side - C. Blue */
/* note: keep following defines in sync with nclient.c, beginning of file */
#define SKY_ALTITUDE	20 /* assumed 'pseudo-isometric' cloud altitude */
#define PANEL_X		13 /* physical top-left screen position of view panel */
#define PANEL_Y		1 /* physical top-left screen position of view panel */
void do_weather() {
	int i, j, intensity;
	static int weather_gen_ticks = 0, weather_ticks10 = 0;
	static int weather_wind_ticks = 0, weather_speed_ticks = 0; /* sub-ticks when weather really is processed */
	bool wind_west_effective = FALSE, wind_east_effective = FALSE; /* horizontal movement */
	bool gravity_effective = FALSE; /* vertical movement */
	bool redraw = FALSE;
	int x, y, dx, dy, d; /* distance calculations (cloud centre) */
	static int cloud_movement_ticks = 0, cloud_movement_lasttick = 0;
	bool with_clouds, outside_clouds;


	/* continue animating current weather (if any) */
	if (!weather_type && !weather_elements) return;


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
		weather_speed_ticks = 0;
	}

	if (redraw) {
	        if (screen_icky) Term_switch(0);
		for (i = 0; i < weather_elements; i++) {
			/* only for elements within visible panel screen area */
			if (weather_element_x[i] >= weather_panel_x &&
    	        	    weather_element_x[i] < weather_panel_x + SCREEN_WID &&
        	    	    weather_element_y[i] >= weather_panel_y &&
        	    	    weather_element_y[i] < weather_panel_y + SCREEN_HGT) {
        	    		if (weather_element_type[i] == 1) {
	    	        	    	/* display raindrop */
    				        Term_draw(PANEL_X + weather_element_x[i] - weather_panel_x,
	        		    	    PANEL_Y + weather_element_y[i] - weather_panel_y,
		        	    	    TERM_BLUE, weather_wind == 0 ? '|' : (weather_wind % 2 == 1 ? '\\' : '/'));
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
		if (!screen_icky) Term_fresh();
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
				weather_element_type[weather_elements] = (weather_type == 3 ? (rand_int(2) ? 1 : 2) : weather_type);
				x = rand_int(MAX_WID - 2) + 1;
				y = rand_int(MAX_HGT - 1 + SKY_ALTITUDE) - SKY_ALTITUDE;
				weather_element_ydest[weather_elements] = weather_element_y[weather_elements] + SKY_ALTITUDE;

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
	weather_speed_ticks++;
	if (weather_speed_ticks == weather_speed) {
		gravity_effective = TRUE;
		weather_speed_ticks = 0;
	}

	/* nothing to do this time? - exit */
	if (!gravity_effective && !wind_west_effective && !wind_east_effective)
		return;


/* move weather elements --------------------------------------------------- */

	/* display and advance currently existing weather elements */
        if (screen_icky) Term_switch(0);
	for (i = 0; i < weather_elements; i++) {
		/* restore old tile before moving the weather element */
		/* if panel view was freshly updated from server then no need */
		if (!weather_panel_changed) {
			/* only for elements within visible panel screen area */
    	        	if (weather_element_x[i] >= weather_panel_x &&
    	        	    weather_element_x[i] < weather_panel_x + SCREEN_WID &&
        	    	    weather_element_y[i] >= weather_panel_y &&
        	    	    weather_element_y[i] < weather_panel_y + SCREEN_HGT) {
	            		/* restore original grid content */
	    		        Term_draw(PANEL_X + weather_element_x[i] - weather_panel_x,
	    		    	    PANEL_Y + weather_element_y[i] - weather_panel_y,
    	        	    	    panel_map_a[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y],
    	        	    	    panel_map_c[weather_element_x[i] - weather_panel_x][weather_element_y[i] - weather_panel_y]);
	            	}
	        }

		/* advance raindrops */
		if (weather_element_type[i] == 1) {
			/* perform movement (y:speed, x:wind) */
			if (gravity_effective) weather_element_y[i]++;
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
			/* only for elements within visible panel screen area */
			else if (weather_element_x[i] >= weather_panel_x &&
    	        	    weather_element_x[i] < weather_panel_x + SCREEN_WID &&
        	    	    weather_element_y[i] >= weather_panel_y &&
        	    	    weather_element_y[i] < weather_panel_y + SCREEN_HGT) {
    	        	    	/* display raindrop */
        		        Term_draw(PANEL_X + weather_element_x[i] - weather_panel_x,
        		    	    PANEL_Y + weather_element_y[i] - weather_panel_y,
	        	    	    TERM_BLUE, weather_wind == 0 ? '|' : (weather_wind % 2 == 1 ? '\\' : '/'));
			}
		}
		/* advance snowflakes - falling slowly (assumed weather_speed isn't
		   set to silyl value 1 - well, maybe that could be a 'hail storm') */
		else if (weather_element_type[i] == 2) {
			/* perform movement (y:speed, x:wind) */
			if (gravity_effective) weather_element_y[i]++;
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
			/* only for elements within visible panel screen area */
			else if (weather_element_x[i] >= weather_panel_x &&
    	        	    weather_element_x[i] < weather_panel_x + SCREEN_WID &&
        	    	    weather_element_y[i] >= weather_panel_y &&
        	    	    weather_element_y[i] < weather_panel_y + SCREEN_HGT) {
    	        	    	/* display snowflake */
        		        Term_draw(PANEL_X + weather_element_x[i] - weather_panel_x,
        		    	    PANEL_Y + weather_element_y[i] - weather_panel_y,
        		    	    TERM_WHITE, '*');
			}
		}
	}
        if (screen_icky) Term_switch(0);

	/* Update the screen */
	if (!screen_icky) Term_fresh();


/* clean up and exit ------------------------------------------------------- */

	/* started to draw on a freshly updated panel? */
	weather_panel_changed = FALSE;
}
