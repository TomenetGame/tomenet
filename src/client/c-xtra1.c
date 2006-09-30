/* $Id$ */
/* Handle the printing of info to the screen */



#include "angband.h"

/*
 * Print character info at given row, column in a 13 char field
 */
static void prt_field(cptr info, int row, int col)
{
	/* Dump 13 spaces to clear */
	c_put_str(TERM_WHITE, "             ", row, col);

	/* Dump the info itself */
	c_put_str(TERM_L_BLUE, info, row, col);
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
void prt_level(int level, s32b max, s32b cur, s32b adv)
{
	char tmp[32];

	sprintf(tmp, "%6d", level);

	Term_putstr(0, ROW_LEVEL, -1, TERM_WHITE, "LEVEL ");
	Term_putstr(COL_LEVEL + 6, ROW_LEVEL, -1, TERM_L_GREEN, tmp);


	if (!c_cfg.exp_need)
	{
		sprintf(tmp, "%9ld", (long)cur);
	}
	else
	{
		if (level >= PY_MAX_LEVEL)
		{
			(void)sprintf(tmp, "*********");
		}
		else
		{
			/* Hack -- display in minus (to avoid confusion chez player) */
			(void)sprintf(tmp, "%9ld", cur - adv);
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
}

/*
 * Prints current gold
 */
void prt_gold(int gold)
{
	char tmp[32];

	put_str("AU ", ROW_GOLD, COL_GOLD);
	sprintf(tmp, "%9ld", (long)gold);
	c_put_str(TERM_L_GREEN, tmp, ROW_GOLD, COL_GOLD + 3);
}

/*
 * Prints current AC
 */
void prt_ac(int ac)
{
	char tmp[32];

	put_str("Cur AC ", ROW_AC, COL_AC);
	sprintf(tmp, "%5d", ac);
	c_put_str(TERM_L_GREEN, tmp, ROW_AC, COL_AC + 7);
}

/*
 * Prints Max/Cur hit points
 */
void prt_hp(int max, int cur)
{
	char tmp[32];
	byte color;

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
}

/*
 * Prints Max/Cur spell points
 */
void prt_sp(int max, int cur)
{
	char tmp[32];
	byte color;

        put_str("Max SP ", ROW_MAXSP, COL_MAXSP);

	sprintf(tmp, "%5d", max);
	color = TERM_L_GREEN;

	c_put_str(color, tmp, ROW_MAXSP, COL_MAXSP + 7);


        put_str("Cur SP ", ROW_CURSP, COL_CURSP);

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
}

/*
 * Prints the player's current sanity.
 */
void prt_sane(byte attr, cptr buf)
{
#ifdef SHOW_SANITY	/* NO SANITY DISPLAY!!! */

  put_str("SN:         ", ROW_SANITY, COL_SANITY);

  c_put_str(attr, buf, ROW_SANITY, COL_SANITY+3);
#endif	/* SHOW_SANITY */
}

/*
 * Prints depth into the dungeon
 */
void prt_depth(int x, int y, int z, bool town, int recall, cptr name)
{
	char depths[32];

	sprintf(depths, "(%-2d,%-2d)",x,y);
	c_put_str(TERM_L_GREEN, depths, ROW_XYPOS, COL_XYPOS);

	if(town)
		sprintf(depths, name);
	else if (c_cfg.depth_in_feet)
		sprintf(depths, "%dft", z*50);
	else
		sprintf(depths, "Lev %d", z);

	/* Right-Adjust the "depth" and clear old values */
	if (recall) c_prt(TERM_ORANGE, format("%7s", depths), ROW_DEPTH, COL_DEPTH);
	else prt(format("%7s", depths), ROW_DEPTH, COL_DEPTH);
}

/*
 * Prints hunger information
 */
void prt_hunger(int food)
{
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
}

/*
 * Prints blindness status
 */
void prt_blind(bool blind)
{
	if (blind)
	{
		c_put_str(TERM_ORANGE, "Blind", ROW_BLIND, COL_BLIND);
	}
	else
	{
		put_str("     ", ROW_BLIND, COL_BLIND);
	}
}

/*
 * Prints confused status
 */
void prt_confused(bool confused)
{
	if (confused)
	{
		c_put_str(TERM_ORANGE, "Confused", ROW_CONFUSED, COL_CONFUSED);
	}
	else
	{
		put_str("        ", ROW_CONFUSED, COL_CONFUSED);
	}
}

/*
 * Prints fear status
 */
void prt_afraid(bool fear)
{
	if (fear)
	{
		c_put_str(TERM_ORANGE, "Afraid", ROW_AFRAID, COL_AFRAID);
	}
	else
	{
		put_str("      ", ROW_AFRAID, COL_AFRAID);
	}
}

/*
 * Prints poisoned status
 */
void prt_poisoned(bool poisoned)
{
	if (poisoned)
	{
		c_put_str(TERM_ORANGE, "Poisoned", ROW_POISONED, COL_POISONED);
	}
	else
	{
		put_str("        ", ROW_POISONED, COL_POISONED);
	}
}

/*
 * Prints paralyzed/searching status
 */
void prt_state(bool paralyzed, bool searching, bool resting)
{
	byte attr = TERM_WHITE;

	char text[16];

	if (paralyzed)
	{
		attr = TERM_RED;

		strcpy(text, "Paralyzed!");
	}

	else if (searching)
	{
#if 0
		if (get_skill(SKILL_STEALTH) <= 10)
		{
			strcpy(text, "Searching ");
		}

		else
		{
			attr = TERM_L_DARK;
			strcpy(text,"Stlth Mode");
		}
#else
		strcpy(text, "Searching ");
#endif

	}

	else if (resting)
	{
		strcpy(text, "Resting   ");
	}
	else
	{
		strcpy(text, "          ");
	}

	c_put_str(attr, text, ROW_STATE, COL_STATE);
}

/*
 * Prints speed
 */
void prt_speed(int speed)
{
	int attr = TERM_WHITE;
	char buf[32] = "";

	if (speed > 0)
	{
		attr = TERM_L_GREEN;
		sprintf(buf, "Fast (+%d)", speed);
	}

	else if (speed < 0)
	{
		attr = TERM_L_UMBER;
		sprintf(buf, "Slow (%d)", speed);
	}

	/* Display the speed */
	c_put_str(attr, format("%-11s", buf), ROW_SPEED, COL_SPEED);
}

/*
 * Prints ability to gain skillss
 */
void prt_study(bool study)
{
	if (study)
	{
		put_str("Skill", ROW_STUDY, COL_STUDY);
	}
	else
	{
		put_str("     ", ROW_STUDY, COL_STUDY);
	}
}

/*
 * Prints cut status
 */
void prt_cut(int cut)
{
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
}

/*
 * Prints stun status
 */
void prt_stun(int stun)
{
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
	if (afk)
		c_put_str(TERM_ORANGE, "AFK", 22, 0);
	else
		c_put_str(TERM_ORANGE, "   ", 22, 0);
}

/* Print encumberment status line */
void prt_encumberment(byte cumber_armor,byte awkward_armor,byte cumber_glove,byte heavy_wield,byte heavy_shoot,
                        byte icky_wield, byte awkward_wield, byte easy_wield, byte cumber_weight, byte monk_heavyarmor, byte awkward_shoot)
{
	put_str("           ", 7, 0);
	if (cumber_armor) c_put_str(TERM_UMBER, "[", 7, 0);
	if (heavy_wield) c_put_str(TERM_RED, "/", 7, 1);
	if (icky_wield) c_put_str(TERM_ORANGE, "\\", 7, 2);
	if (awkward_wield) c_put_str(TERM_YELLOW, "/", 7, 3);
	if (easy_wield) c_put_str(TERM_GREEN, "|", 7, 4);
	if (heavy_shoot) c_put_str(TERM_RED, "}", 7, 5);
	if (awkward_shoot) c_put_str(TERM_YELLOW, "}", 7, 6);
	if (cumber_weight) c_put_str(TERM_L_RED, "F", 7, 7);
	if (monk_heavyarmor) c_put_str(TERM_YELLOW, "[", 7, 8);
	if (awkward_armor) c_put_str(TERM_VIOLET, "[", 7, 9);
	if (cumber_glove) c_put_str(TERM_VIOLET, "]", 7, 10);
}

/*
 * Redraw the monster health bar
 */
void health_redraw(int num, byte attr)
{
	/* Not tracking */
	if (!attr)
	{
		/* Erase the health bar */
		Term_erase(COL_INFO, ROW_INFO, 12);
	}

	/* Tracking a monster */
	else
	{
		/* Default to "unknown" */
		Term_putstr(COL_INFO, ROW_INFO, 12, TERM_WHITE, "[----------]");

		/* Dump the current "health" (use '*' symbols) */
		Term_putstr(COL_INFO + 1, ROW_INFO, num, attr, "**********");
	}
}

/*
 * Choice window "shadow" of the "show_inven()" function.
 */
static void display_inven(void)
{
	int	i, n, z = 0;
	int	wgt;

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

		/* Is this item acceptable? */
		if (item_tester_okay(o_ptr))
		{
			/* Prepare an "index" */
			tmp_val[0] = index_to_label(i);

			/* Bracket the "index" --(-- */
			tmp_val[1] = ')';
		}

		/* Display the index (or blank space) */
		Term_putstr(0, i, 3, TERM_WHITE, tmp_val);

		/* Describe the object */
		strcpy(o_name, inventory_name[i]);

		/* Obtain length of description */
		n = strlen(o_name);

		/* Clear the line with the (possibly indented) index */
		Term_putstr(3, i, n, o_ptr->xtra1, o_name);

		/* Erase the rest of the line */
		Term_erase(3+n, i, 255);

		/* Display the weight if needed */
		if (c_cfg.show_weights && o_ptr->weight)
		{
			wgt = o_ptr->weight;
			(void)sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
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
	int	wgt;

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
		Term_putstr(3, i - INVEN_WIELD, n, o_ptr->xtra1, o_name);

		/* Erase the rest of the line */
		Term_erase(3+n, i - INVEN_WIELD, 255);

		/* Display the weight if needed */
		if (c_cfg.show_weights && o_ptr->weight)
		{
			wgt = o_ptr->weight;
			(void)sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
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
	int	col, len, lim, wgt, totalwgt = 0;

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
		out_color[k] = o_ptr->xtra1;
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
			wgt = o_ptr->weight;
			(void)sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
			put_str(tmp_val, j + 1, 71);
			totalwgt += wgt;
		}
	}

	/* Display the weight if needed */
	if (c_cfg.show_weights && totalwgt)
	{
		(void)sprintf(tmp_val, "Total: %3d.%1d lb", totalwgt / 10, totalwgt % 10);
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
	int	col, len, lim, wgt, totalwgt = 0;

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
		out_color[k] = o_ptr->xtra1;
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
			wgt = o_ptr->weight;//multiplied server-side: * o_ptr->number;
			(void)sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
			put_str(tmp_val, j + 1, 71);
			totalwgt += wgt;
		}
	}

	/* Display the weight if needed */
	if (c_cfg.show_weights && totalwgt)
	{
		(void)sprintf(tmp_val, "Total: %3d.%1d lb", totalwgt / 10, totalwgt % 10);
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
	bool msgtarget;

	cptr msg;
	byte a;

	/* Display messages in different colors -Zz */
	char nameA[20];
	char nameB[20];

	strcpy(nameA, "[");  strcat(nameA, cname);  strcat(nameA, ":");
	strcpy(nameB, ":");  strcat(nameB, cname);  strcat(nameB, "]");


        /* Scan windows */
        for (j = 0; j < 8; j++)
        {
                term *old = Term;

                /* No window */
                if (!ang_term[j]) continue;

                /* No relevant flags */
                if (!(window_flag[j] &
		    (PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT))) continue;
		msgtarget = TRUE;

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

				/* Display messages in different colors -Zz */
				if ((strstr(msg, nameA) != NULL) || (strstr(msg, nameB) != NULL)) {
					a = TERM_GREEN;
				} else if (msg[2] == '[') {
					a = TERM_L_BLUE;
				}

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

				/* Display messages in different colors -Zz */
				if ((strstr(msg, nameA) != NULL) || (strstr(msg, nameB) != NULL)) {
					if (!(window_flag[j] & (PW_MESSAGE | PW_CHAT))) msgtarget = FALSE;
					a = TERM_GREEN;
				} else if (msg[2] == '[') {
					if (!(window_flag[j] & (PW_MESSAGE | PW_CHAT))) msgtarget = FALSE;
					a = TERM_L_BLUE;
				} else {
					if (!(window_flag[j] & (PW_MESSAGE | PW_MSGNOCHAT))) msgtarget = FALSE;
					a = TERM_WHITE;
				}
#if 0
				if (!msgtarget) break;
#endif
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


void display_player(int hist)
{
	int i;
	char buf[80];
	cptr desc;

        /* Clear screen */
	clear_from(0);

        /* Name, Sex, Race, Class */
        put_str("Name        :", 2, 1);
        put_str("Sex         :", 3, 1);
        put_str("Race        :", 4, 1);
        put_str("Class       :", 5, 1);
        put_str("Body        :", 6, 1);
	put_str("Mode        :", 7, 1);

        c_put_str(TERM_L_BLUE, cname, 2, 15);
        c_put_str(TERM_L_BLUE, (p_ptr->male ? "Male" : "Female"), 3, 15);
        c_put_str(TERM_L_BLUE, race_info[race].title, 4, 15);
        c_put_str(TERM_L_BLUE, class_info[class].title, 5, 15);
        c_put_str(TERM_L_BLUE, c_p_ptr->body_name, 6, 15);
	if (p_ptr->mode & MODE_IMMORTAL)
	    	c_put_str(TERM_L_BLUE, "Everlasting (infinite lives)", 7, 15);
	else if ((p_ptr->mode & MODE_NO_GHOST) && (p_ptr->mode & MODE_HELL))
	    	c_put_str(TERM_L_BLUE, "Hellish (one life, extra hard)", 7, 15);
	else if (p_ptr->mode & MODE_NO_GHOST)
	    	c_put_str(TERM_L_BLUE, "Unworldly (one life)", 7, 15);
        else if (p_ptr->mode & MODE_HELL)
        	c_put_str(TERM_L_BLUE, "Hard (3 lives, extra hard)", 7, 15);
        else /*(p_ptr->mode == MODE_NORMAL)*/
	    	c_put_str(TERM_L_BLUE, "Normal (3 lives)", 7, 15);

        /* Age, Height, Weight, Social */
        prt_num("Age          ", (int)p_ptr->age, 2, 32, TERM_L_BLUE);
        prt_num("Height       ", (int)p_ptr->ht, 3, 32, TERM_L_BLUE);
        prt_num("Weight       ", (int)p_ptr->wt, 4, 32, TERM_L_BLUE);
        prt_num("Social Class ", (int)p_ptr->sc, 5, 32, TERM_L_BLUE);

        /* Display the stats */
        for (i = 0; i < 6; i++)
        {
                /* Special treatment of "injured" stats */
                if (p_ptr->stat_use[i] < p_ptr->stat_top[i])
                {
                        int value;

                        /* Use lowercase stat name */
                        put_str(stat_names_reduced[i], 2 + i, 61);

                        /* Get the current stat */
                        value = p_ptr->stat_use[i];

                        /* Obtain the current stat (modified) */
                        cnv_stat(value, buf);

                        /* Display the current stat (modified) */
                        c_put_str(TERM_YELLOW, buf, 2 + i, 66);

                        /* Acquire the max stat */
                        value = p_ptr->stat_top[i];

                        /* Obtain the maximum stat (modified) */
                        cnv_stat(value, buf);

                        /* Display the maximum stat (modified) */
			if(p_ptr->stat_cur[i] < (18 + 100))
			{
                    		c_put_str(TERM_L_GREEN, buf, 2 + i, 73);
			}
			else
			{
                    		c_put_str(TERM_L_UMBER, buf, 2 + i, 73);
			}
                }

                /* Normal treatment of "normal" stats */
                else
                {
                        /* Assume uppercase stat name */
                        put_str(stat_names[i], 2 + i, 61);

                        /* Obtain the current stat (modified) */
                        cnv_stat(p_ptr->stat_use[i], buf);

                        /* Display the current stat (modified) */
			if(p_ptr->stat_cur[i] < (18 + 100))
			{
    		                c_put_str(TERM_L_GREEN, buf, 2 + i, 66);
			}
			else
			{
    		                c_put_str(TERM_L_UMBER, buf, 2 + i, 66);
			}
                }
        }

	/* Check for history */
	if (hist)
	{
		put_str("(Character Background)", 15, 25);

		for (i = 0; i < 4; i++)
		{
			put_str(p_ptr->history[i], i + 16, 10);
		}
	}
	else
	{
		put_str("(Miscellaneous Abilities)", 15, 25);

		/* Display "skills" */
		put_str("Fighting    :", 16, 1);
		desc = likert(p_ptr->skill_thn, 120, 0);
		c_put_str(likert_color, desc, 16, 15);

		put_str("Bows/Throw  :", 17, 1);
		desc = likert(p_ptr->skill_thb, 120, 0);
		c_put_str(likert_color, desc, 17, 15);

		put_str("Saving Throw:", 18, 1);
		desc = likert(p_ptr->skill_sav, 52, 95);	/*was 6.0 before x10 increase */
		c_put_str(likert_color, desc, 18, 15);

		put_str("Stealth     :", 19, 1);
		desc = likert(p_ptr->skill_stl, 10, 30);
		c_put_str(likert_color, desc, 19, 15);


		put_str("Perception  :", 16, 28);
		desc = likert(p_ptr->skill_fos, 40, 75);
		c_put_str(likert_color, desc, 16, 42);

		put_str("Searching   :", 17, 28);
		desc = likert(p_ptr->skill_srh, 60, 100);
		c_put_str(likert_color, desc, 17, 42);

		put_str("Disarming   :", 18, 28);
		desc = likert(p_ptr->skill_dis, 80, 100);
		c_put_str(likert_color, desc, 18, 42);

		put_str("Magic Device:", 19, 28);
		desc = likert(p_ptr->skill_dev, 60, 0);
		c_put_str(likert_color, desc, 19, 42);


		put_str("Blows/Round:", 16, 55);
		put_str(format("%d", p_ptr->num_blow), 16, 69);

		put_str("Shots/Round:", 17, 55);
		put_str(format("%d", p_ptr->num_fire), 17, 69);

		put_str("Spells/Round:", 18, 55);
		put_str(format("%d", p_ptr->num_spell), 18, 69);

		put_str("Infra-Vision:", 19, 55);
		put_str(format("%d feet", p_ptr->see_infra * 10), 19, 69);
	}

	/* Dump the bonuses to hit/dam */
	prt_num("+To MHit    ", p_ptr->dis_to_h + p_ptr->to_h_melee, 9, 1, TERM_L_BLUE);
	prt_num("+To MDamage ", p_ptr->dis_to_d + p_ptr->to_d_melee, 10, 1, TERM_L_BLUE);
	prt_num("+To RHit    ", p_ptr->dis_to_h + p_ptr->to_h_ranged, 11, 1, TERM_L_BLUE);
	prt_num("+To RDamage ", p_ptr->to_d_ranged, 12, 1, TERM_L_BLUE);

	/* Dump the armor class bonus */
	prt_num("+ To AC     ", p_ptr->dis_to_a, 13, 1, TERM_L_BLUE);

	/* Dump the total armor class */
	prt_num("  Base AC   ", p_ptr->dis_ac, 14, 1, TERM_L_BLUE);

	prt_num("Level      ", (int)p_ptr->lev, 9, 28, TERM_L_GREEN);

	if (p_ptr->exp >= p_ptr->max_exp)
	{
		prt_lnum("Experience ", p_ptr->exp, 10, 28, TERM_L_GREEN);
	}
	else
	{
		prt_lnum("Experience ", p_ptr->exp, 10, 28, TERM_YELLOW);
	}

	prt_lnum("Max Exp    ", p_ptr->max_exp, 11, 28, TERM_L_GREEN);

	if (p_ptr->lev >= PY_MAX_LEVEL)
	{
		put_str("Exp to Adv.", 12, 28);
		c_put_str(TERM_L_GREEN, "    *****", 12, 28+11);
	}
	else
	{
		prt_lnum("Exp to Adv.", exp_adv, 12, 28, TERM_L_GREEN);
	}

	prt_lnum("Gold       ", p_ptr->au, 13, 28, TERM_L_GREEN);

	prt_num("Max Hit Points ", p_ptr->mhp, 9, 52, TERM_L_GREEN);

	if (p_ptr->chp >= p_ptr->mhp)
	{
		prt_num("Cur Hit Points ", p_ptr->chp, 10, 52, TERM_L_GREEN);
	}
	else if (p_ptr->chp > (p_ptr->mhp) / 10)
	{
		prt_num("Cur Hit Points ", p_ptr->chp, 10, 52, TERM_YELLOW);
	}
	else
	{
		prt_num("Cur Hit Points ", p_ptr->chp, 10, 52, TERM_RED);
	}

	prt_num("Max SP (Mana)  ", p_ptr->msp, 11, 52, TERM_L_GREEN);

	if (p_ptr->csp >= p_ptr->msp)
	{
		prt_num("Cur SP (Mana)  ", p_ptr->csp, 12, 52, TERM_L_GREEN);
	}
	else if (p_ptr->csp > (p_ptr->msp) / 10)
	{
		prt_num("Cur SP (Mana)  ", p_ptr->csp, 12, 52, TERM_YELLOW);
	}
	else
	{
		prt_num("Cur SP (Mana)  ", p_ptr->csp, 12, 52, TERM_RED);
	}
#ifdef SHOW_SANITY
	put_str("Cur Sanity", 13, 52);

	c_put_str(c_p_ptr->sanity_attr, c_p_ptr->sanity, 13, 67);
#endif	/* SHOW_SANITY */


	/* Show location (better description needed XXX) */
	if (c_cfg.depth_in_feet)
		put_str(format("You are at %dft of (%d, %d).", p_ptr->wpos.wz * 50,
					p_ptr->wpos.wx, p_ptr->wpos.wy), 20, hist ? 10 : 1);
	else
		put_str(format("You are at level %d of (%d, %d).", p_ptr->wpos.wz,
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
}


