/* $Id$ */
/* File: skills.c */

/* Purpose: player skills */

/*
 * Copyright (c) 2001 DarkGod
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"

/*
 * Given the name of a skill, returns skill index or -1 if no
 * such skill is found
 */
s16b find_skill(cptr name)
{
	u16b i;

	/* Scan skill list */
	for (i = 1; i < MAX_SKILLS; i++)
	{
		/* The name matches */
		if (streq((char*)s_info[i].name, name)) return (i);
	}

	/* No match found */
	return (-1);
}


/*
 *
 */
s16b get_skill(int skill)
{
	return (p_ptr->s_info[skill].value / SKILL_STEP);
}


/*
 *
 */
s16b get_skill_scale(int skill, u32b scale)
{
	/* XXX XXX XXX */
	return (((p_ptr->s_info[skill].value / 10) * (scale * (SKILL_STEP / 10)) /
	         (SKILL_MAX / 10)) /
	        (SKILL_STEP / 10));
}


#if 0
/*
 * Advance the skill point of the skill specified by i and
 * modify related skills
 */
void increase_skill(int i, s16b *invest)
{
	/* No skill points to be allocated */
	if (!p_ptr->skill_points) return;

	/* The skill cannot be increased */
	if (!s_info[i].mod) return;

	/* The skill is already maxed */
        if (s_info[i].value >= SKILL_MAX) return;

        /* Cannot allocate more than player level * 3 / 2 + 4 levels */
        if ((s_info[i].value / SKILL_STEP) >= ((p_ptr->lev * 3 / 2) + 4))
        {
                int hgt, wid;

		Term_get_size(&wid, &hgt);
                msg_box("Cannot raise a skill value above 4 + player level * 3/2.", (int)(hgt / 2), (int)(wid / 2));
                return;
        }

	/* Spend an unallocated skill point */
	p_ptr->skill_points--;

	/* Increase the skill */
        s_info[i].value += s_info[i].mod;
        invest[i]++;
}


/*
 * Descrease the skill point of the skill specified by i and
 * modify related skills
 */
void decrease_skill(int i, s16b *invest)
{
        /* Cannot decrease more */
        if (!invest[i]) return;

	/* The skill cannot be decreased */
	if (!s_info[i].mod) return;

	/* The skill already has minimal value */
	if (!s_info[i].value) return;

	/* Free a skill point */
	p_ptr->skill_points++;

	/* Decrease the skill */
	s_info[i].value -= s_info[i].mod;
        invest[i]--;
}

#endif

/*
 *
 */
int get_idx(int i)
{
	int j;

	for (j = 1; j < MAX_SKILLS; j++)
	{
		if (s_info[j].order == i)
			return (j);
                        }
	return (0);
}


/*
 *
 */
void init_table_aux(int table[MAX_SKILLS][2], int *idx, int father, int lev,
	bool full)
{
	int j, i;

	for (j = 1; j < MAX_SKILLS; j++)
	{
		i = get_idx(j);

		if (s_info[i].father != father) continue;
		if (p_ptr->s_info[i].hidden) continue;

		table[*idx][0] = i;
		table[*idx][1] = lev;
		(*idx)++;
		if (p_ptr->s_info[i].dev || full) init_table_aux(table, idx, i, lev + 1, full);
	}
}


void init_table(int table[MAX_SKILLS][2], int *max, bool full)
{
	*max = 0;
	init_table_aux(table, max, -1, 0, full);
}


bool has_child(int sel)
{
	int i;

	for (i = 1; i < MAX_SKILLS; i++)
	{
		if (s_info[i].father == sel)
			return (TRUE);
	}
	return (FALSE);
}

void print_desc_aux(cptr txt, int y, int xx)
{
	int i = -1, x = xx;


	while (txt[++i] != 0)
	{
		if (txt[i] == '\n')
		{
			x = xx;
			y++;
		}
		else
		{
			Term_putch(x++, y, TERM_YELLOW, txt[i]);
		}
	}
}

/*
 * Dump the skill tree
 */
void dump_skills(FILE *fff)
#if 0
{
	int i, j, max = 0;
	int table[MAX_SKILLS][2];
	char buf[80];

	init_table(table, &max, TRUE);

	Term_clear();

	fprintf(fff, "\nSkills (points left: %d)", p_ptr->skill_points);

	for (j = 0; j < max; j++)
	{
		int z;

		i = table[j][0];

		if ((p_ptr->s_info[i].value == 0) && (i != SKILL_MISC))
		{
			if (p_ptr->s_info[i].mod == 0) continue;
		}

		sprintf(buf, "\n");

		for (z = 0; z < table[j][1]; z++) strcat(buf, "	 ");

		if (!has_child(i))
		{
			strcat(buf, format(" . %s", s_info[i].name));
		}
		else
		{
			strcat(buf, format(" - %s", s_info[i].name));
		}

		fprintf(fff, "%-50s%02ld.%03ld [%01d.%03d]",
		        buf, p_ptr->s_info[i].value / SKILL_STEP, p_ptr->s_info[i].value % SKILL_STEP,
		        p_ptr->s_info[i].mod / 1000, p_ptr->s_info[i].mod % 1000);
	}

	fprintf(fff, "\n");
}
#else	// 0
{
	int i, j, max = 0;
	int table[MAX_SKILLS][2];
	char buf[80];

	init_table(table, &max, TRUE);

	Term_clear();

	fprintf(fff, "\nSkills (points left: %d)", p_ptr->skill_points);

	for (j = 0; j < max; j++)
	{
		int z;

		i = table[j][0];

		/* XXX this causes strange dump when one has SKILL_DEVICE and not
		 * SKILL_MAGIC, for example.
		 * We should make sure the skill doesn't have 'valid' children!
		 */
		if ((p_ptr->s_info[i].value == 0) && (i != SKILL_MISC))
		{
			if (p_ptr->s_info[i].mod == 0) continue;
		}

//		sprintf(buf, "\n");
		fprintf(fff, "\n");
		sprintf(buf, "");

		for (z = 0; z < table[j][1]; z++) strcat(buf, "    ");

		if (!has_child(i))
		{
			strcat(buf, format(" . %s", s_info[i].name));
		}
		else
		{
			strcat(buf, format(" - %s", s_info[i].name));
		}

		fprintf(fff, "%-50s%02ld.%03ld [%01d.%03d]",
		        buf, p_ptr->s_info[i].value / SKILL_STEP, p_ptr->s_info[i].value % SKILL_STEP,
		        p_ptr->s_info[i].mod / 1000, p_ptr->s_info[i].mod % 1000);
	}

	fprintf(fff, "\n");
}
#endif	// 0

/*
 * Draw the skill tree
 */
void print_skills(int table[MAX_SKILLS][2], int max, int sel, int start)
{
	int i, j;
	int wid, hgt;

	Term_clear();
	Term_get_size(&wid, &hgt);

//	c_prt(TERM_WHITE, "TomeNET Skills Screen", 0, 28);
//	c_prt(TERM_WHITE, " === TomeNET Skills Screen ===  [move:2,8,j,k  fold:<CR>,c,o  advance:6,l]", 0, 0);
	if (c_cfg.rogue_like_commands)
	{
		c_prt(TERM_WHITE, " === TomeNET Skills Screen ===  [move:j,k,g,G,#  fold:<CR>,c,o  advance:l]", 0, 0);
	}
	else
	{
		c_prt(TERM_WHITE, " === TomeNET Skills Screen ===  [move:2,8,g,G,#  fold:<CR>,c,o  advance:6]", 0, 0);
	}

	c_prt((p_ptr->skill_points) ? TERM_L_BLUE : TERM_L_RED,
	      format("Skill points left: %d", p_ptr->skill_points), 1, 0);
	print_desc_aux((char*)s_info[table[sel][0]].desc, 2, 0);

	for (j = start; j < start + (hgt - 4); j++)
	{
		byte color = TERM_WHITE;
		char deb = ' ', end = ' ';

		if (j >= max) break;

		i = table[j][0];

		if ((p_ptr->s_info[i].value == 0))
		{
			if (p_ptr->s_info[i].mod == 0) color = TERM_L_DARK;
			else color = TERM_ORANGE;
		}
		else if (p_ptr->s_info[i].value == SKILL_MAX) color = TERM_L_BLUE;
		if (p_ptr->s_info[i].hidden) color = TERM_L_RED;
		if (j == sel)
		{
			color = TERM_L_GREEN;
			deb = '[';
			end = ']';
		}
		if (!has_child(i))
		{
			c_prt(color, format("%c.%c%s", deb, end, s_info[i].name),
			      j + 4 - start, table[j][1] * 4);
		}
		else if (p_ptr->s_info[i].dev)
		{
			c_prt(color, format("%c-%c%s", deb, end, s_info[i].name),
			      j + 4 - start, table[j][1] * 4);
		}
		else
		{
			c_prt(color, format("%c+%c%s", deb, end, s_info[i].name),
			      j + 4 - start, table[j][1] * 4);
		}
		c_prt(color,
		      format("%02ld.%03ld [%01d.%03d]",
			         p_ptr->s_info[i].value / SKILL_STEP, p_ptr->s_info[i].value % SKILL_STEP,
			         p_ptr->s_info[i].mod / 1000, p_ptr->s_info[i].mod % 1000),
			  j + 4 - start, 60);
	}
}

/*
 * Interreact with skills
 */
bool hack_do_cmd_skill_wait = FALSE;
void do_cmd_skill()
{
	int sel = 0, start = 0, max;
	char c;
	int table[MAX_SKILLS][2];
	int i;
	int wid,hgt;
	bool changed = FALSE;

#ifndef EVIL_TEST /* evil test */
	/* Screen is icky */
	screen_icky = TRUE;
#endif

	/* Save the screen */
	Term_save();

	/* Clear the screen */
	Term_clear();

	/* Initialise the skill list */
	init_table(table, &max, FALSE);

	while (TRUE)
	{
		Term_get_size(&wid, &hgt);

		/* Display list of skills */
		print_skills(table, max, sel, start);

		/* Wait for user input */
		if (!hack_do_cmd_skill_wait)
			c = inkey();
		else
		{
			int net_fd;

			/* Acquire and save maximum file descriptor */
			net_fd = Net_fd();

			/* If no network yet, just wait for a keypress */
			if (net_fd == -1)
			{
				/* Look for a keypress */
				quit("No network in do_cmd_skill() !");
			}
			else
			{
				/* Wait for keypress, while also checking for net input */
				do
				{
					int result;

					/* Update our timer and if neccecary send a keepalive packet
					*/
					update_ticks();
					do_keepalive();

					/* Flush the network output buffer */
					Net_flush();

					/* Wait for .001 sec, or until there is net input */
					SetTimeout(0, 1000);

					/* Parse net input if we got any */
					if (SocketReadable(net_fd))
					{
						if ((result = Net_input()) == -1)
						{
							quit(NULL);
						}

						/* Update the screen */
						Term_fresh();

						/* Redraw windows if necessary */
						if (p_ptr->window)
						{
							window_stuff();
						}
					}
				} while (hack_do_cmd_skill_wait);
			}
			c = 0;
		}

		/* Leave the skill screen */
		if (c == ESCAPE || c == KTRL('X')) break;

		/* Expand / collapse list of skills */
		else if (c == '\r')
		{
			if (p_ptr->s_info[table[sel][0]].dev) p_ptr->s_info[table[sel][0]].dev = FALSE;
			else p_ptr->s_info[table[sel][0]].dev = TRUE;
			init_table(table, &max, FALSE);
		}
		else if (c == 'c')
		{
			for (i = 0; i < max; i++)
				p_ptr->s_info[table[i][0]].dev = FALSE;

			init_table(table, &max, FALSE);
			start = sel = 0;
		}
		else if (c == 'o')
		{
			for (i = 0; i < max; i++)
				p_ptr->s_info[table[i][0]].dev = TRUE;

			init_table(table, &max, FALSE);
			/* TODO: memorize and recover the cursor position */
		}

		else if (c == 'g')
		{
			start = sel = 0;
		}
		else if (c == 'G')
		{
			sel = max - 1;
			start = sel - (hgt - 4);
			if (sel >= start + (hgt - 4)) start = sel - (hgt - 4) + 1;
		}
		/* Hack -- go to a specific line */
		else if (c == '#')
		{
			char tmp[80];
			prt(format("Goto Line(max %d): ", max), 23, 0);
			strcpy(tmp, "1");
			if (askfor_aux(tmp, 10, 0))
			{
				sel = start = atoi(tmp) - 1;
				if (sel >= max) sel = start = max - 1;
				if (sel < 0) sel = start = 0;
			}
		}

		/* Next page */
		else if (c == 'n' || c == ' ')
		{
			sel += (hgt - 4);
			start += (hgt - 4);
			if (sel >= max) sel = max - 1;
			if (start >= max) start = max - 1;
		}

		/* Previous page */
		else if (c == 'p' || c == 'b')
		{
			sel -= (hgt - 4);
			start -= (hgt - 4);
			if (sel < 0) sel = 0;
			if (start < 0) start = 0;
		}

		/* Select / increase a skill */
		else
		{
			int dir = c;

			/* Move cursor down */
			if (dir == '2' || dir == 'j') sel++;

			/* Move cursor up */
			if (dir == '8' || dir == 'k') sel--;

			/* Miscellaneous skills cannot be increased/decreased as a group */
			//			if (table[sel][0] == SKILL_MISC) continue;

			/* Increase the current skill */
			if (dir == '6' || dir == 'l')
			{
				/* Send a packet */
				Send_skill_mod(table[sel][0]);

				/* Now we wait the answer */
				hack_do_cmd_skill_wait = TRUE;

				/* Refresh when everything is done */
				changed = TRUE;
			}

			/* Decrease the current skill */
			//      		if (dir == '4' || dir == 'h') decrease_skill(table[sel][0], skill_invest);

			/* XXX XXX XXX Wizard mode commands outside of wizard2.c */

			/* Increase the skill */
			//			if (wizard && (c == '+')) skill_bonus[table[sel][0]] += SKILL_STEP;

			/* Decrease the skill */
			//			if (wizard && (c == '-')) skill_bonus[table[sel][0]] -= SKILL_STEP;

			/* Handle boundaries and scrolling */
			if (sel < 0) sel = max - 1;
			if (sel >= max) sel = 0;
			if (sel < start) start = sel;
			if (sel >= start + (hgt - 4)) start = sel - (hgt - 4) + 1;
		}
	}

	/* Load the screen */
	Term_load();

	/* XXX test -- redraw when done */
	if (changed) Send_redraw(1);

#ifndef EVIL_TEST /* evil test */
	/* Screen is not icky */
	screen_icky = FALSE;
#endif
}


#if 0
/*
 * List of melee skills
 */
s16b melee_skills[MAX_MELEE] =
{
	SKILL_MASTERY,
	SKILL_HAND,
	SKILL_BEAR,
};
char *melee_names[MAX_MELEE] =
{
	"Weapon combat",
	"Barehanded combat",
	"Bearform combat",
};
static bool melee_bool[MAX_MELEE];
static int melee_num[MAX_MELEE];

s16b get_melee_skill()
{
	int i;

	for (i = 0; i < MAX_MELEE; i++)
	{
		if (p_ptr->melee_style == melee_skills[i])
			return (i);
	}
	return (0);
}

s16b get_melee_skills()
{
	int i, j = 0;

	for (i = 0; i < MAX_MELEE; i++)
	{
		if (s_info[melee_skills[i]].value && (!s_info[melee_skills[i]].hidden))
		{
			melee_bool[i] = TRUE;
			j++;
		}
		else
			melee_bool[i] = FALSE;
	}

	return (j);
}

static void choose_melee()
{
	int i, j, z = 0;

	character_icky = TRUE;
	Term_save();
	Term_clear();

	j = get_melee_skills();
	prt("Choose a melee style:", 0, 0);
	for (i = 0; i < MAX_MELEE; i++)
	{
		if (melee_bool[i])
		{
			prt(format("%c) %s", I2A(z), melee_names[i]), z + 1, 0);
			melee_num[z] = i;
			z++;
		}
	}

	while (TRUE)
	{
		char c = inkey();

		if (c == ESCAPE) break;
		if (A2I(c) < 0) continue;
		if (A2I(c) >= j) continue;

		for (i = 0, z = 0; z < A2I(c); i++)
			if (melee_bool[i]) z++;
		p_ptr->melee_style = melee_skills[melee_num[z]];
		for (i = INVEN_WIELD; p_ptr->body_parts[i - INVEN_WIELD] == INVEN_WIELD; i++)
			inven_takeoff(i, 255, FALSE);
		energy_use = 100;
		break;
	}

	Term_load();
	character_icky = FALSE;
}

void select_default_melee()
{
	int i;

        get_melee_skills();
        p_ptr->melee_style = SKILL_MASTERY;
	for (i = 0; i < MAX_MELEE; i++)
	{
		if (melee_bool[i])
		{
                        p_ptr->melee_style = melee_skills[i];
                        break;
		}
	}
}

#endif

/*
 * Print a batch of skills.
 */
static void print_skill_batch(int *p, int start, int max, bool mode)
{
	char buff[80];
	int i = start, j = 0;

	if (mode) prt(format("         %-31s", "Name"), 1, 20);

	for (i = start; i < (start + 20); i++)
	{
		if (i >= max) break;

                /* Hack -- only able to learn spells when learning is required */
//                if ((p[i] == SKILL_LEARN) && (!must_learn_spells()))
//                        continue;
                else if (p[i] > 0)
			sprintf(buff, "  %c-%3d) %-30s", I2A(j), p[i] + 1, s_info[p[i]].action_desc);
//			sprintf(buff, "  %c-%3d) %-30s", I2A(j), s_info[p[i]].action_mkey + 1, s_info[p[i]].action_desc);
		else
			sprintf(buff, "  %c-%3d) %-30s", I2A(j), 1, "Change melee style");

		if (mode) prt(buff, 2 + j, 20);
		j++;
	}
	if (mode) prt("", 2 + j, 20);
	prt(format("Select a skill (a-%c), * to list, @ to select by name/No., +/- to scroll:", I2A(j - 1)), 0, 0);
}

int do_cmd_activate_skill_aux()
{
	char which;
	int max = 0, i, start = 0;
	int ret;
//	bool mode = FALSE;
	bool mode = c_cfg.always_show_lists;
	int *p;

	C_MAKE(p, MAX_SKILLS, int);

	/* Count the max */

	/* More than 1 melee skill ? */
	//	if (get_melee_skills() > 1)
	//	{
	//		p[max++] = 0;
	//	}

	for (i = 1; i < MAX_SKILLS; i++)
	{
		if (s_info[i].action_mkey && p_ptr->s_info[i].value)
		{
			int j;
			bool next = FALSE;

			/* Already got it ? */
			for (j = 0; j < max; j++)
			{
				if (s_info[i].action_mkey == s_info[p[j]].action_mkey)
				{
					next = TRUE;
					break;
				}
			}
			if (next) continue;

			/* Hack -- only able to learn spells when learning is required */
			//                      if ((i == SKILL_LEARN) && (!must_learn_spells()))
			//                              continue;
			p[max++] = i;
		}
	}

	if (!max)
	{
		c_msg_print("You dont have any activable skills.");
		return -1;
	}
	if (max == 1 && c_cfg.quick_messages)
	{
		return p[0];
	}

#ifndef EVIL_TEST /* evil test */
	screen_icky = TRUE;
#endif
	Term_save();

	while (1)
	{
		print_skill_batch(p, start, max, mode);
		which = inkey();

		if (which == ESCAPE)
		{
			ret = -1;
			break;
		}
		else if (which == '*' || which == '?' || which == ' ')
		{
			mode = (mode)?FALSE:TRUE;
			Term_load();
			Term_save();
#ifndef EVIL_TEST /* evil test */
			screen_icky = FALSE;
#endif
		}
		else if (which == '+')
		{
			start += 20;
			if (start >= max) start -= 20;
			Term_load();
			Term_save();
#ifndef EVIL_TEST /* evil test */
			screen_icky = FALSE;
#endif
		}
		else if (which == '-')
		{
			start -= 20;
			if (start < 0) start += 20;
			Term_load();
			Term_save();
#ifndef EVIL_TEST /* evil test */
			screen_icky = FALSE;
#endif
		}
		else if (which == '@')
		{
			char buf[80];
			int nb;

			strcpy(buf, "Cast sorcery spell");
			if (!get_string("Skill action? ", buf, 79))
				return FALSE;

			/* Can we convert to a number? */
			nb = atoi(buf) - 1;

			/* Find the skill it is related to */
			for (i = 1; i < MAX_SKILLS; i++)
			{
				if (s_info[i].action_desc && (!strcmp(buf, (char*)s_info[i].action_desc) && get_skill(i)))
					break;
				if (i == nb)
//				if (i == s_info[nb].action_mkey)
					break;
			}
			if ((i < MAX_SKILLS))
			{
				ret = i;
				break;
			}

		}
		else
		{
			which = tolower(which);
			if (start + A2I(which) >= max)
			{
				bell();
				continue;
			}
			if (start + A2I(which) < 0)
			{
				bell();
				continue;
			}

			ret = p[start + A2I(which)];
			break;
		}
	}
	Term_load();
#ifndef EVIL_TEST /* evil test */
	screen_icky = FALSE;
#endif

	C_FREE(p, MAX_SKILLS, int);

	return ret;
}

/*
 * Hook to determine if an object is a device
 */
static bool item_tester_hook_device(object_type *o_ptr)
{
	if ((o_ptr->tval == TV_ROD) ||
	    (o_ptr->tval == TV_STAFF) ||
	    (o_ptr->tval == TV_WAND)) return (TRUE);

	/* Assume not */
	return (FALSE);
}

/*
 * Hook to determine if an object is a potion
 */
static bool item_tester_hook_potion(object_type *o_ptr)
{
	if ((o_ptr->tval == TV_POTION) ||
	    (o_ptr->tval == TV_POTION2)) return (TRUE);

	/* Assume not */
	return (FALSE);
}

/*
 * set a trap .. it's out of place somewhat.	- Jir -
 */
void do_trap(int item_kit)
{
//	int item_kit, item_load;
	int item_load;
	object_type *o_ptr, *j_ptr, *i_ptr;

	if (item_kit < 0)
	{
		item_tester_tval = TV_TRAPKIT;
		if (!c_get_item(&item_kit, "Use which trapping kit? ", FALSE, TRUE, FALSE))
		{
			if (item_kit == -2)
				c_msg_print("You have no trapping kits.");
			return;
		}
	}

	o_ptr = &inventory[item_kit];

	/* Trap kits need a second object */
	switch (o_ptr->sval)
	{
		case SV_TRAPKIT_BOW:
			item_tester_tval = TV_ARROW;
			break;
		case SV_TRAPKIT_XBOW:
			item_tester_tval = TV_BOLT;
			break;
		case SV_TRAPKIT_SLING:
			item_tester_tval = TV_SHOT;
			break;
		case SV_TRAPKIT_POTION:
			item_tester_hook = item_tester_hook_potion;
			break;
		case SV_TRAPKIT_SCROLL:
			item_tester_tval = TV_SCROLL;
			break;
		case SV_TRAPKIT_DEVICE:
			item_tester_hook = item_tester_hook_device;
			break;
		default:
			c_msg_print("Unknown trapping kit type!");
			break;
	}

	if (!c_get_item(&item_load, "Load with what? ", TRUE, TRUE, FALSE))
	{
		if (item_load == -2)
			c_msg_print("You have nothing to load that trap with.");
		return;
	}


	/* Send it */
	Send_activate_skill(MKEY_TRAP, item_kit, item_load, 0);
}

/*
 * Handle the mkey according to the types.
 * if item is less than zero, ask for an item if needed.
 */
void do_activate_skill(int x_idx, int item)
{
	int dir, spell;

	dir = spell = 0;

	if (s_info[x_idx].flags1 & SKF1_MKEY_HARDCODE)
	{
		switch (s_info[x_idx].action_mkey)
		{
			case MKEY_MIMICRY:
				do_mimic();
//				cmd_mimic();
				break;
			case MKEY_TRAP:
				do_trap(item);
//				cmd_mimic();
				break;
			default:
				c_msg_print("Very sorry, you need more recent client.");
				break;
		}
		return;
	}
	else if (s_info[x_idx].flags1 & SKF1_MKEY_SPELL)
	{
		if (item < 0)
		{
			item_tester_tval = s_info[x_idx].tval;
			if (!c_get_item(&item, "Cast from which book? ", FALSE, TRUE, FALSE))
			{
				if (item == -2)
					c_msg_print("You have no books that you can cast from.");
				return;
			}
		}

		/* Ask for a spell, allow cancel */
		if (!get_spell(&spell, "cast", item, FALSE)) return;

		/* Send it */
		Send_activate_skill(s_info[x_idx].action_mkey, item, spell, dir);

		return;
	}
	else if (s_info[x_idx].action_mkey == MKEY_SCHOOL)
	{
		/* Ask for a spell, allow cancel */
		if ((spell = get_school_spell("cast") == -1)) return;

                /* Ask for a direction? */
                dir = -1;
                if (exec_lua(0, format("return pre_exec_spell_dir(%d)", spell)))
                        if (!get_dir(&dir))
                                return;

                /* Ask for an item */
                item = 0;

		/* Send it */
		Send_activate_skill(MKEY_SCHOOL, item, spell, dir);

		return;
	}

	if (item < 0 && s_info[x_idx].flags1 & SKF1_MKEY_ITEM)
	{
		item_tester_tval = s_info[x_idx].tval;
		if (!c_get_item(&item, "Which item? ", TRUE, TRUE, FALSE))
		{
			return;
		}
	}

	if (s_info[x_idx].flags1 & SKF1_MKEY_DIRECTION)
	{
		if (!get_dir(&dir))
			return;
	}

	/* Send it */
	Send_activate_skill(s_info[x_idx].action_mkey, item, spell, dir);
}

/* Ask & execute a skill */
void do_cmd_activate_skill()
{
	int x_idx = -1;

	/* Get the skill, if available */
	x_idx = do_cmd_activate_skill_aux();
#if 0
        else
	{
		x_idx = command_arg - 1;
		if ((x_idx < 0) || (x_idx >= MAX_SKILLS)) return;
                if ((!s_info[x_idx].value) || (!s_info[x_idx].action_mkey))
                {
                        c_msg_print("You cannot use this skill.");
                        return;
                }
	}
#endif
	if (x_idx == -1) return;

	do_activate_skill(x_idx, -1);

//	if (!x_idx)
//	{
//		choose_melee();
//		return;
//	}

#if 0
	switch (s_info[x_idx].action_mkey)
	{
		case MKEY_SORCERY:
			item_tester_tval = TV_SORCERY_BOOK;
			cmd_cast_skill();
			break;
		case MKEY_MAGERY:
			item_tester_tval = TV_MAGIC_BOOK;
			cmd_cast_skill();
			break;
		case MKEY_SHADOW:
			item_tester_tval = TV_SHADOW_BOOK;
			cmd_cast_skill();
			break;
		case MKEY_ARCHERING:
			item_tester_tval = TV_HUNT_BOOK;
			cmd_cast_skill();
			break;
		case MKEY_PRAY:
			item_tester_tval = TV_PRAYER_BOOK;
			cmd_cast_skill();
			break;
#if 0	// not implemeted yet.
		case MKEY_PSI:
			item_tester_tval = TV_PSI_BOOK;
			cmd_cast_skill();
			break;
#endif	// 0
		case MKEY_MIMICRY:
			cmd_mimic();
			break;
		case MKEY_FIGHTING:
			/* Note - cmd_cast() will do */
#if 0
			item_tester_tval = TV_FIGHT_BOOK;
			cmd_cast_skill();
#else	// 0
			cmd_fight();
#endif	// 0
			break;
		default:
			break;
	}
#endif	// 0
}
