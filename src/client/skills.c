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

static int get_idx(int i);

/*
 * Given the name of a skill, returns skill index or -1 if no
 * such skill is found
 */
#if 0 /* not used - mikaelh */
static s16b find_skill(cptr name) {
	u16b i;

	/* Scan skill list */
	for (i = 1; i < MAX_SKILLS; i++)
		/* The name matches */
		if (streq((char*)s_info[i].name, name)) return(i);

	/* No match found */
	return(-1);
}
#endif


/*
 *
 */
s16b get_skill(int skill) {
	return(p_ptr->s_info[skill].value / SKILL_STEP);
}


/*
 *
 */
s16b get_skill_scale(player_type *pfft, int skill, u32b scale) {
	(void) pfft; /* first parameter ignored in client code */

	/* XXX XXX XXX */
	return(((p_ptr->s_info[skill].value / 10) * (scale * (SKILL_STEP / 10)) /
	    (SKILL_MAX / 10)) /
	    (SKILL_STEP / 10));
}

/*
 *
 */
static int get_idx(int i) {
	int j;

	for (j = 1; j < MAX_SKILLS; j++)
		if (s_info[j].order == i) return(j);
	return(0);
}


static bool has_child(int sel) {
	int i;

	for (i = 1; i < MAX_SKILLS; i++)
		if (s_info[i].father == sel) return(TRUE);
	return(FALSE);
}

static bool has_active_child(int sel) {
	int i;

	for (i = 1; i < MAX_SKILLS; i++)
		if ((s_info[i].father == sel) &&
		    ((p_ptr->s_info[i].mod) || (p_ptr->s_info[i].value) ||
		    has_active_child(i)))
			return(TRUE);
	return(FALSE);
}

/*
 *
 */
static void init_table_aux(int table[MAX_SKILLS][2], int *idx, int father, int lev, bool full) {
	int j, i;

	for (j = 1; j < MAX_SKILLS; j++) {
		i = get_idx(j);

		if (s_info[i].father != father) continue;
		if (p_ptr->s_info[i].flags1 & SKF1_HIDDEN) continue;

		/* new option: hide all completely unusable skill branches - C. Blue */
		if (c_cfg.hide_unusable_skills &&
		    !p_ptr->s_info[i].mod && !p_ptr->s_info[i].value &&
		    !has_active_child(i))
			continue;

		table[*idx][0] = i;
		table[*idx][1] = lev;
		(*idx)++;
		if (p_ptr->s_info[i].dev || full) init_table_aux(table, idx, i, lev + 1, full);
	}
}


static void init_table(int table[MAX_SKILLS][2], int *max, bool full) {
	*max = 0;
	init_table_aux(table, max, -1, 0, full);
}


static void print_desc_aux(cptr txt, int y, int xx) {
	int i = -1, x = xx;

	while (txt[++i] != 0) {
		if (txt[i] == '\n') {
			x = xx;
			y++;
		} else Term_putch(x++, y, TERM_YELLOW, txt[i]);
	}
}

/*
 * Dump the skill tree
 */
void dump_skills(FILE *fff) {
	int i, j, max = 0;
	int table[MAX_SKILLS][2];
	char buf[80];

	init_table(table, &max, TRUE);

	Term_clear();

	fprintf(fff, "\nSkills (points left: %d)", p_ptr->skill_points);

	for (j = 0; j < max; j++) {
		int z;

		i = table[j][0];

		/* XXX this causes strange dump when one has SKILL_DEVICE and not
		 * SKILL_MAGIC, for example.
		 * We should make sure the skill doesn't have 'valid' children!
		 */
		if ((p_ptr->s_info[i].value == 0) && (i != SKILL_MISC))
			if (p_ptr->s_info[i].mod == 0) continue;

/*		sprintf(buf, "\n");		*/
		fprintf(fff, "\n");

/*		sprintf(buf, "");		*/
		buf[0] = '\0';

		for (z = 0; z < table[j][1]; z++) strcat(buf, "    ");

		if (!has_child(i))
			strcat(buf, format(" . %s", s_info[i].name));
		else if (!has_active_child(i))
			strcat(buf, format(" o %s", s_info[i].name));
		else
			strcat(buf, format(" - %s", s_info[i].name));

		if (!(p_ptr->s_info[i].flags1 & SKF1_DUMMY)) {
			fprintf(fff, "%-50s%02d.%03d", buf, p_ptr->s_info[i].value / SKILL_STEP, p_ptr->s_info[i].value % SKILL_STEP);
			if (p_ptr->s_info[i].mod)
				fprintf(fff, " [%0d.%03d]", p_ptr->s_info[i].mod / 1000, p_ptr->s_info[i].mod % 1000);
			else
				fprintf(fff, " [-----]");
		}
	}
	fprintf(fff, "\n");
}

/*
 * Draw the skill tree
 */
static void print_skills(int table[MAX_SKILLS][2], int max, int sel, int start) {
	int i, j;
	int wid, hgt;

	Term_clear();
	Term_get_size(&wid, &hgt);

	if (c_cfg.rogue_like_commands)
		Term_putstr(0, 0, -1, TERM_WHITE, " Character Skills   [help:\377R?\377- move:\377sj\377-,\377sk\377-,\377sg\377-,\377sG\377-,\377s#\377-,\377ss\377- fold:\377s<CR>\377-,\377sc\377-,\377so\377- train:\377sl\377-,\377sarrow-right\377-]");
	else
		Term_putstr(0, 0, -1, TERM_WHITE, " Character Skills   [help:\377R?\377- move:\377s2\377-,\377s8\377-,\377sg\377-,\377sG\377-,\377s#\377-,\377ss\377- fold:\377s<CR>\377-,\377sc\377-,\377so\377- train:\377s6\377-,\377sarrow-right\377-]");

	Term_putstr(0, 1, -1, TERM_WHITE, format(" Skill points left: \377%c%-5d   \377wType \377R/undoskills\377- in chat immediately after mistake.", (p_ptr->skill_points) ? 'B': 's', p_ptr->skill_points));
	print_desc_aux((char*)s_info[table[sel][0]].desc, 2, 0);

	for (j = start; j < start + (hgt - 4); j++) {
		byte color = TERM_WHITE;
		char deb = ' ', end = ' ';

		if (j >= max) break;

		i = table[j][0];

		if (p_ptr->s_info[i].value == 0) {
			if (p_ptr->s_info[i].mod == 0) color = TERM_L_DARK;
			else color = TERM_ORANGE;
		} else if ((p_ptr->s_info[i].value == SKILL_MAX) ||
		    ((p_ptr->s_info[i].flags1 & SKF1_MAX_1) && (p_ptr->s_info[i].value == 1000)) ||
		    ((p_ptr->s_info[i].flags1 & SKF1_MAX_10) && (p_ptr->s_info[i].value == 10000)) ||
		    ((p_ptr->s_info[i].flags1 & SKF1_MAX_20) && (p_ptr->s_info[i].value == 20000)) ||
		    ((p_ptr->s_info[i].flags1 & SKF1_MAX_25) && (p_ptr->s_info[i].value == 25000)) ||
		    (p_ptr->s_info[i].flags1 & SKF1_AUTO_MAX))
			color = TERM_L_BLUE;
		if (p_ptr->s_info[i].flags1 & SKF1_HIDDEN) color = TERM_L_RED;
		if (p_ptr->s_info[i].flags1 & SKF1_DUMMY) color = TERM_SLATE;

		if (j == sel) {
#if defined(USE_X11) || defined(WINDOWS) || defined(USE_SDL2)
 #if defined(TEST_CLIENT) && defined(EXTENDED_BG_COLOURS)
			color = TERMX_BLUE;
 #else
			color = TERM_L_GREEN;
 #endif
#else
			color = TERM_L_GREEN;
#endif
			deb = '[';
			end = ']';
		}
		if (!has_child(i)) {
			c_prt(color, format("%c.%c%s", deb, end, s_info[i].name),
			      j + 4 - start, table[j][1] * 4);
		} else if (!has_active_child(i)) {
			c_prt(color, format("%co%c%s", deb, end, s_info[i].name),
			      j + 4 - start, table[j][1] * 4);
		} else if (p_ptr->s_info[i].dev) {
			c_prt(color, format("%c-%c%s", deb, end, s_info[i].name),
			      j + 4 - start, table[j][1] * 4);
		} else {
			c_prt(color, format("%c+%c%s", deb, end, s_info[i].name),
			      j + 4 - start, table[j][1] * 4);
		}

		if (!(p_ptr->s_info[i].flags1 & SKF1_DUMMY)) {
			if (p_ptr->s_info[i].mod)
				c_prt(color,
				    format("%02d.%03d [%01d.%03d]",
				    p_ptr->s_info[i].value / SKILL_STEP, p_ptr->s_info[i].value % SKILL_STEP,
				    (int)p_ptr->s_info[i].mod / 1000, (int)p_ptr->s_info[i].mod % 1000),
				    j + 4 - start, 60);
			else
				c_prt(color,
				    format("%02d.%03d [-----]",
				    p_ptr->s_info[i].value / SKILL_STEP, p_ptr->s_info[i].value % SKILL_STEP),
				    j + 4 - start, 60);
		}
	}

	/* Hack - Get rid of the cursor - mikaelh */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;
}

/*
 * Redraw the skill menu if possible.
 */
static bool hack_do_cmd_skill = FALSE;
static int table[MAX_SKILLS][2];
static int sel = 0;
static int start = 0;
static int max;
void do_redraw_skills() {
	redraw_skills = FALSE;

	if (hack_do_cmd_skill) {
		print_skills(table, max, sel, start);
	}
}

/*
 * Interact with skills.
 */
#define SKILL_SCREEN_PAD_TOP 4		/* The top n lines of the skill screen that don't contain skill lines but some other stuff/text/diz */
void do_cmd_skill() {
	bool leave = FALSE;
	char ch;
	int i;
	int wid, hgt;

	/* Initialize global variables */
	//sel = 0;
	//start = 0;

	/* Save the screen */
	Term_save();

	/* Clear the screen */
	Term_clear();

	/* Skill menu open */
	hack_do_cmd_skill = TRUE;

	/* Initialise the skill list */
	init_table(table, &max, FALSE);

	while (!leave) {
		Term_get_size(&wid, &hgt);

		/* Display list of skills */
		print_skills(table, max, sel, start);

		ch = inkey();

		switch(ch) {
		/* Leave the skill screen */
		case ESCAPE:
		case KTRL('Q'):
			leave = TRUE;
			break;

		case '?':
			/* Exception for runecraft, since the runes are not spells/schools:
			   Just redirect to runecraft in general. */
			if (s_info[table[sel][0]].father == SKILL_SCHOOL_RUNECRAFT) cmd_the_guide(3, 0, "Runecraft");
			else cmd_the_guide(3, 0, (char*)s_info[table[sel][0]].name);
			break;

		/* Take a screenshot */
		case KTRL('T'):
			xhtml_screenshot("screenshot????", FALSE);
			break;

		/* specialty: allow chatting from within here */
		case ':':
			cmd_message();
			break;

		/* Expand / collapse list of skills */
		case '\r':
			if (p_ptr->s_info[table[sel][0]].dev) p_ptr->s_info[table[sel][0]].dev = FALSE;
			else p_ptr->s_info[table[sel][0]].dev = TRUE;
			init_table(table, &max, FALSE);
			Send_skill_dev(table[sel][0], p_ptr->s_info[table[sel][0]].dev);
			break;
		case 'c':
			for (i = 0; i < max; i++) {
				p_ptr->s_info[table[i][0]].dev = FALSE;
				Send_skill_dev(-1, FALSE);
			}

			init_table(table, &max, FALSE);
			start = sel = 0;
			break;
		case 'o':
			for (i = 0; i < max; i++) {
				p_ptr->s_info[table[i][0]].dev = TRUE;
				Send_skill_dev(-1, TRUE);
			}

			init_table(table, &max, FALSE);
			/* TODO: memorize and recover the cursor position */
			break;

		case 'g':
		case '7':
		case NAVI_KEY_POS1:
			start = sel = 0;
			break;
		case 'G':
		case '1':
		case NAVI_KEY_END:
			sel = max - 1;
			start = sel - (hgt - SKILL_SCREEN_PAD_TOP);
			if (start < 0) start = 0;
			if (sel >= start + (hgt - SKILL_SCREEN_PAD_TOP)) start = sel - (hgt - SKILL_SCREEN_PAD_TOP) + 1;
			break;

		/* Hack -- go to a specific line */
		case '#': {
			char tmp[80];

			prt(format("Goto Line(max %d): ", max), 23 + HGT_PLUS, 0);
			strcpy(tmp, "1");
			if (askfor_aux(tmp, 10, 0)) {
				sel = start = atoi(tmp) - 1;
				if (sel >= max) sel = start = max - 1;
				if (sel < 0) sel = start = 0;
			}
			break;
			}

		/* Next page */
		case 'n':
		case ' ':
		case '3':
		case NAVI_KEY_PAGEDOWN:
			//note: /4 -> actually move in half-screen increments instead of jumping a whole page
			sel += (hgt - SKILL_SCREEN_PAD_TOP + 3) / 4;
			start += (hgt - SKILL_SCREEN_PAD_TOP + 3) / 4;
			if (sel >= max) sel = max - 1;
			if (start >= max) start = max - 1;
			break;

		/* Previous page */
		case 'p':
		case 'b':
		case '9':
		case NAVI_KEY_PAGEUP:
			//note: /4 -> actually move in half-screen increments instead of jumping a whole page
			sel -= (hgt - SKILL_SCREEN_PAD_TOP + 3) / 4;
			start -= (hgt - SKILL_SCREEN_PAD_TOP + 3) / 4;
			if (sel < 0) sel = 0;
			if (start < 0) start = 0;
			break;

		/* Search for skill name */
		case 's':
		case '/':
			{
			char tmp[MAX_CHARS];
			bool inkey_msg_old = inkey_msg;
			int j, k;

			prt("Search for skill: ", 23 + HGT_PLUS, 0);
			tmp[0] = 0;
			inkey_msg = TRUE; /* suppress hybrid macros */
			if (!askfor_aux(tmp, MAX_CHARS, 0)) {
				inkey_msg = inkey_msg_old;
				continue;
			}
			inkey_msg = inkey_msg_old;

			for (j = 1; j < MAX_SKILLS; j++) {
				i = get_idx(j);

				if (p_ptr->s_info[i].flags1 & SKF1_HIDDEN) continue;
				if (!my_strcasestr((char*)s_info[i].name, tmp)) continue;

				/* If the skill we're looking for is currently not developed, develop it and all
				   parent skills recursively, so it's visible in our table and we can select it */
				k = i;
				while (!p_ptr->s_info[k].dev) {
					p_ptr->s_info[k].dev = TRUE;
					Send_skill_dev(k, TRUE);
					k = s_info[k].father;
					if (k == -1) break;
				}
				init_table(table, &max, FALSE);

				for (i = 0; i < max; i++) {
					if (!my_strcasestr((char*)s_info[table[i][0]].name, tmp)) continue;

					sel = i;
					start = i - (hgt - SKILL_SCREEN_PAD_TOP) / 2 - 1;
					if (start + hgt >= max) start = max - (hgt - SKILL_SCREEN_PAD_TOP);
					if (start < 0) start = 0;
					break;
				}

				break;
			}
			}
			break;

		/* Select / increase a skill */
		default: {
			int dir = ch;

			/* Move cursor down */
			if (dir == '2' || dir == 'j') sel++;

			/* Move cursor up */
			if (dir == '8' || dir == 'k') sel--;

			/* Increase the current skill */
			if (dir == '6' || dir == 'l') {
				/* Send a packet */
				Send_skill_mod(table[sel][0]);
			}

			/* XXX XXX XXX Wizard mode commands outside of wizard2.c */

			/* Handle boundaries and scrolling */
			if (sel < 0) sel = max - 1;
			if (sel >= max) sel = 0;
			if (sel < start) start = sel;
			if (sel >= start + (hgt - SKILL_SCREEN_PAD_TOP)) start = sel - (hgt - SKILL_SCREEN_PAD_TOP) + 1;
			}
		}
	}

	/* Load the screen */
	Term_load();

	/* Skill menu not open anymore */
	hack_do_cmd_skill = FALSE;

	/* Flush the queue */
	Flush_queue();
}

/*
 * Print a batch of skills.
 */
static void print_skill_batch(int *p, int start, int max, bool mode) {
	char buff[80];
	int i = start, j = 0;

	if (mode) prt(format("         %-31s", "Name"), 1, 20);

	for (i = start; i < (start + 20); i++) {
		if (i >= max) break;

		if (p[i] > 0)
			sprintf(buff, "  %c-%3d) %-30s", I2A(j), s_info[p[i]].action_mkey, (char*)s_info[p[i]].action_desc);
		else
			sprintf(buff, "  %c-%3d) %-30s", I2A(j), 1, "Change melee style");

		if (mode) prt(buff, 2 + j, 20);
		j++;
	}
	if (mode) prt("", 2 + j, 20);
	//note: since the "skill screen" contains "skills", these 'meta' skills have been dubbed "abilities" for now,
	//just to avoid (er hopefully) confusion.. (keep consistent with tomenet*.hlp files):
	prompt_topline(format("Select an ability (a-%c), * to list, @ to select by name/No., +/- to scroll:", I2A(j - 1)));

	screen_line_icky = 2 + j + 1;
	screen_column_icky = 20 - 1;
}

static int do_cmd_activate_skill_aux() {
	char which;
	int max = 0, i, start = 0;
	int ret;
	bool mode = c_cfg.always_show_lists;
	int *p;
	bool term_saved = FALSE;

	C_MAKE(p, MAX_SKILLS, int);

	for (i = 1; i < MAX_SKILLS; i++) {
		/* At least gain one whole skill level to use it! - Kurzel */
		if (s_info[i].action_mkey && p_ptr->s_info[i].value > 999) {
			int j;
			bool next = FALSE;

			/* Already got it ? */
			for (j = 0; j < max; j++) {
				if (s_info[i].action_mkey == s_info[p[j]].action_mkey) {
					next = TRUE;
					break;
				}
			}
			if (next) continue;

			p[max++] = i;
		}
	}

	if (!max) {
		c_msg_print("You don't have any activable skills.");
		/* Stop macro execution if we're on safe_macros! */
		if (parse_macro && c_cfg.safe_macros) flush_now();
		return(-1);
	}
/*	if (max == 1 && c_cfg.quick_messages)
	{
		return(p[0]);
	}
*/

	/* Save the terminal if c_cfg.always_show_lists was on */
	if (mode && !term_saved) {
		Term_save();
		term_saved = TRUE;
	}

	topline_icky = TRUE;

	while (1) {
		print_skill_batch(p, start, max, mode);
		which = inkey();

		if (which == ESCAPE) {
			ret = -1;
			break;
		} else if (which == KTRL('T')) {
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", FALSE);
		} else if (which == '*' || which == '?' || which == ' ') {
			mode = (mode) ? FALSE : TRUE;
			if (!mode && term_saved) {
				Term_load();
				term_saved = FALSE;
			} else if (mode && !term_saved) {
				Term_save();
				term_saved = TRUE;
			}
		} else if (which == '+') {
			start += 20;
			if (start >= max) start -= 20;
			if (term_saved) {
				/* Draw the new list on a fresh copy of the screen */
				Term_load();
				Term_save();
			}
		} else if (which == '-') {
			start -= 20;
			if (start < 0) start += 20;
			if (term_saved) {
				/* Draw the new list on a fresh copy of the screen */
				Term_load();
				Term_save();
			}
		} else if (which == '@') {
			char buf[80];
			int nb;

			strcpy(buf, "Cast a spell");
			if (!get_string("Skill action? ", buf, 79)) {
				if (term_saved) Term_load();
				screen_line_icky = -1;
				screen_column_icky = -1;
				return(FALSE);
			}

			/* Can we convert to a number? */
			nb = atoi(buf) - 1;

			/* Find the skill it is related to */
			for (i = 1; i < MAX_SKILLS; i++) {
				if (s_info[i].action_desc && (!strcmp(buf, (char*)s_info[i].action_desc) && get_skill(i)))
					break;
				if ((s_info[i].action_mkey == nb + 1) && (nb != -1) && get_skill(i))
					break;
			}
			if ((i < MAX_SKILLS)) {
				ret = i;
				break;
			}
			/* If the desired skill couldn't be found, stop macro execution if we're on safe_macros! */
			if (parse_macro && c_cfg.safe_macros) {
				flush_now();
				ret = -1;
				break;
			}
		} else {
			which = tolower(which);
			if (start + A2I(which) >= max) {
				bell();
				if (parse_macro && c_cfg.safe_macros) {
					flush_now();
					ret = -1;
					break;
				}
				continue;
			}
			if (start + A2I(which) < 0) {
				bell();
				if (parse_macro && c_cfg.safe_macros) {
					flush_now();
					ret = -1;
					break;
				}
				continue;
			}

			ret = p[start + A2I(which)];
			break;
		}
	}

	screen_line_icky = -1;
	screen_column_icky = -1;

	if (term_saved) Term_load();
	topline_icky = FALSE;

	/* Clear the prompt line */
	clear_topline();

	C_FREE(p, MAX_SKILLS, int);

	return(ret);
}

/*
 * Hook to determine if an object is a device, for the purpose of recharging
 */
bool item_tester_hook_device(object_type *o_ptr) {
	if (o_ptr->tval == TV_ROD ||
	    o_ptr->tval == TV_STAFF ||
	    o_ptr->tval == TV_WAND
#ifdef MSTAFF_MDEV_COMBO
	    || (o_ptr->tval == TV_MSTAFF && o_ptr->xtra4) //hack: marker as 'rechargable' for ITH_RECHARGE
#endif
	    )
		return(TRUE);

	/* Assume not */
	return(FALSE);
}
/* For device trap kits that also take demo charges (experimental) -- ENABLE_DEMOLITIONIST */
bool item_tester_hook_device_charge(object_type *o_ptr) {
	if (o_ptr->tval == TV_ROD ||
	    o_ptr->tval == TV_STAFF ||
	    o_ptr->tval == TV_WAND ||
	    o_ptr->tval == TV_CHARGE)
	    //Note: No MSTAFF_MDEV_COMBO allowed here, we cannot put mstaves into traps... */
		return(TRUE);

	/* Assume not */
	return(FALSE);
}

/*
 * Hook to determine if an object is a potion
 */
static bool item_tester_hook_potion(object_type *o_ptr) {
	if (o_ptr->tval == TV_POTION ||
	    o_ptr->tval == TV_POTION2 ||
	    o_ptr->tval == TV_FLASK)
		return(TRUE);

	/* Assume not */
	return(FALSE);
}

static bool item_tester_hook_scroll_rune(object_type *o_ptr) {
	if (o_ptr->tval == TV_SCROLL ||
	    o_ptr->tval == TV_RUNE)
		return(TRUE);

	/* Assume not */
	return(FALSE);
}

bool item_tester_hook_armour(object_type *o_ptr) {
	return(is_armour(o_ptr->tval));
}
bool item_tester_hook_nonart_armour(object_type *o_ptr) {
	return(is_armour(o_ptr->tval) && !o_ptr->name1);
}
bool item_tester_hook_armour_no_shield(object_type *o_ptr) {
	return(is_armour(o_ptr->tval) && o_ptr->tval != TV_SHIELD);
}
bool item_tester_hook_nonart_armour_no_shield(object_type *o_ptr) {
	return(is_armour(o_ptr->tval) && o_ptr->tval != TV_SHIELD && !o_ptr->name1);
}

bool item_tester_hook_armour_no_cloak(object_type *o_ptr) {
	return(is_armour(o_ptr->tval) && o_ptr->tval != TV_CLOAK);
}
bool item_tester_hook_nonart_armour_no_cloak(object_type *o_ptr) {
	return(is_armour(o_ptr->tval) && !o_ptr->name1 && o_ptr->tval != TV_CLOAK);
}
bool item_tester_hook_armour_no_shield_no_cloak(object_type *o_ptr) {
	return(is_armour(o_ptr->tval) && o_ptr->tval != TV_SHIELD && o_ptr->tval != TV_CLOAK);
}
bool item_tester_hook_nonart_armour_no_shield_no_cloak(object_type *o_ptr) {
	return(is_armour(o_ptr->tval) && o_ptr->tval != TV_SHIELD && !o_ptr->name1 && o_ptr->tval != TV_CLOAK);
}

bool item_tester_hook_cloak(object_type *o_ptr) {
	return(o_ptr->tval == TV_CLOAK);
}
bool item_tester_hook_nonart_cloak(object_type *o_ptr) {
	return(o_ptr->tval == TV_CLOAK && !o_ptr->name1);
}

bool item_tester_hook_weapon(object_type *o_ptr) {
	return((is_melee_weapon(o_ptr->tval) || is_ammo(o_ptr->tval) ||
	    o_ptr->tval == TV_BOW || o_ptr->tval == TV_BOOMERANG ||
	    (o_ptr->tval == TV_TRAPKIT && is_firearm_trapkit(o_ptr->sval)) ||
	    o_ptr->tval == TV_DIGGING || o_ptr->tval == TV_MSTAFF));
}
bool item_tester_hook_nonart_weapon(object_type *o_ptr) {
	return((is_melee_weapon(o_ptr->tval) || is_ammo(o_ptr->tval) ||
	    o_ptr->tval == TV_BOW || o_ptr->tval == TV_BOOMERANG ||
	    (o_ptr->tval == TV_TRAPKIT && is_firearm_trapkit(o_ptr->sval)) ||
	    o_ptr->tval == TV_DIGGING || o_ptr->tval == TV_MSTAFF)
	    && !o_ptr->name1);
}

/* this actually tests for spell scrolls, not the custom tome itself */
bool item_tester_hook_custom_tome(object_type *o_ptr) {
	/* check for correct book type */
	if (o_ptr->tval != TV_BOOK || o_ptr->sval != SV_SPELLBOOK) return(FALSE);
	return(TRUE);
}
bool item_tester_hook_ammo(object_type *o_ptr) {
	return(is_ammo(o_ptr->tval));
}
bool item_tester_hook_nonart_ammo(object_type *o_ptr) {
	return(is_ammo(o_ptr->tval) && !o_ptr->name1);
}
bool item_tester_hook_melee(object_type *o_ptr) {
	return(is_melee_weapon(o_ptr->tval));
}
bool item_tester_hook_nonart_melee(object_type *o_ptr) {
	return(is_melee_weapon(o_ptr->tval) && !o_ptr->name1);
}
bool item_tester_hook_ranged(object_type *o_ptr) {
	return(o_ptr->tval == TV_BOW || o_ptr->tval == TV_BOOMERANG);
}
bool item_tester_hook_nonart_ranged(object_type *o_ptr) {
	return((o_ptr->tval == TV_BOW || o_ptr->tval == TV_BOOMERANG) && !o_ptr->name1);
}
bool item_tester_hook_trapkit_fa(object_type *o_ptr) {
	return(o_ptr->tval == TV_TRAPKIT && is_firearm_trapkit(o_ptr->sval));
}
bool item_tester_hook_nonart_trapkit_fa(object_type *o_ptr) {
	return(o_ptr->tval == TV_TRAPKIT && is_firearm_trapkit(o_ptr->sval) && !o_ptr->name1);
}

//ENABLE_DEMOLITIONIST
bool item_tester_hook_chemical(object_type *o_ptr) {
	return(o_ptr->tval == TV_CHEMICAL
	    || o_ptr->tval == TV_BOTTLE
	    || o_ptr->tval == TV_POTION
	    || o_ptr->tval == TV_POTION2
	    || o_ptr->tval == TV_SCROLL
	    || o_ptr->tval == TV_FLASK);
}

/*
 * set a trap .. it's out of place somewhat.	- Jir -
 * (item_kit is -1 when called normally via m skills menu.)
 */
void do_trap(int item_kit) {
	int item_load;
	object_type *o_ptr = NULL;

	if (item_kit < 0) {
		item_tester_tval = TV_TRAPKIT;
		get_item_hook_find_obj_what = "Trap kit name? ";
		get_item_extra_hook = get_item_hook_find_obj;
#ifdef ENABLE_SUBINVEN
		if (!c_get_item(&item_kit, "Use which trapping kit? ", (USE_INVEN | USE_EXTRA | NO_FAIL_MSG | USE_SUBINVEN))) {
			if (item_kit == -2) c_msg_print("You have no trapping kits.");
			if (parse_macro && c_cfg.safe_macros) flush_now();//Term_flush();
			return;
		}
#else
		if (!c_get_item(&item_kit, "Use which trapping kit? ", (USE_INVEN | USE_EXTRA | NO_FAIL_MSG))) {
			if (item_kit == -2) c_msg_print("You have no trapping kits.");
			if (parse_macro && c_cfg.safe_macros) flush_now();//Term_flush();
			return;
		}
#endif
	}

#ifdef ENABLE_SUBINVEN
	if (item_kit >= SUBINVEN_INVEN_MUL) o_ptr = &subinventory[item_kit / SUBINVEN_INVEN_MUL - 1][item_kit % SUBINVEN_INVEN_MUL];
	else
#endif
	if (!o_ptr) o_ptr = &inventory[item_kit];

	/* Trap kits need a second object */
	switch (o_ptr->sval) {
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
	case SV_TRAPKIT_SCROLL_RUNE:
		item_tester_hook = item_tester_hook_scroll_rune;
		break;
	case SV_TRAPKIT_DEVICE:
		item_tester_hook = item_tester_hook_device_charge; //ENABLE_DEMOLITIONIST
		break;
	default:
		c_msg_print("Unknown trapping kit type!");
		if (parse_macro && c_cfg.safe_macros) flush_now();//Term_flush();
		break;
	}

	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;
#ifdef ENABLE_SUBINVEN
	/* Allow getting magic devices from a device bag (and any load item from chests, as a consequence too) */
	if (!c_get_item(&item_load, "Load with what? ", (USE_EQUIP | USE_INVEN | USE_EXTRA | NO_FAIL_MSG | USE_SUBINVEN))) {
		if (item_load == -2) c_msg_print("You have nothing to load that trap with.");
		if (parse_macro && c_cfg.safe_macros) flush_now();//Term_flush();
		return;
	}
#else
	if (!c_get_item(&item_load, "Load with what? ", (USE_EQUIP | USE_INVEN | USE_EXTRA | NO_FAIL_MSG))) {
		if (item_load == -2) c_msg_print("You have nothing to load that trap with.");
		if (parse_macro && c_cfg.safe_macros) flush_now();//Term_flush();
		return;
	}
#endif

	/* Send it */
	Send_activate_skill(MKEY_TRAP, item_kit, item_load, 0, 0, 0);
}

/*
 * Handle the mkey according to the types.
 * if item is less than zero, ask for an item if needed.
 */
void do_activate_skill(int x_idx, int item) {
	char out_val[160];
	int dir = 0;
	s32b spell = 0L;

	if (s_info[x_idx].flags1 & SKF1_MKEY_HARDCODE) {
		switch (s_info[x_idx].action_mkey) {
		case MKEY_MIMICRY:
			do_mimic();
			break;
		case MKEY_TRAP:
			do_trap(item);
			break;
		case MKEY_RCRAFT:
			do_runecraft();
			break;
		case MKEY_STANCE:
			do_stance();
			break;
		case MKEY_MELEE:
			do_melee_technique();
			break;
		case MKEY_RANGED:
			do_ranged_technique();
			break;
		case MKEY_BREATH:
			do_breath();
			break;
		case MKEY_PICK_BREATH:
			do_pick_breath();
			break;
		default:
			c_msg_print("Very sorry, you need more recent client.");
			break;
		}
		return;
	} else if (s_info[x_idx].action_mkey == MKEY_SCHOOL) {
		int item_obj = -1, aux = 0;

		/* Ask for a spell, allow cancel */
		if ((spell = get_school_spell("cast", &item)) == -1) {
			if (parse_macro && c_cfg.safe_macros) flush_now();//Term_flush();
			return;
		}

		/* LIMIT_SPELLS - unhack */
		if (spell >= 1000) {
			aux = spell / 1000;
			spell = spell % 1000;
		}

		/* Ask for a direction? */
		dir = -1;
		sprintf(out_val, "return pre_exec_spell_dir(%d)", spell);
		if (exec_lua(0, out_val))
			if (!get_dir(&dir)) return;

		/* Ask for something? */
		sprintf(out_val, "return pre_exec_spell_extra(%d)", spell);
		if (exec_lua(0, out_val))
			aux = exec_lua(0, "return __pre_exec_extra");

		/* Ask for an item? */
		sprintf(out_val, "return pre_exec_spell_item(%d)", spell);
		if (exec_lua(0, out_val))
			item_obj = exec_lua(0, "return __pre_exec_item");

		/* Send it */
		Send_activate_skill(MKEY_SCHOOL, item, spell, dir, item_obj, aux);

		return;
	} else if (s_info[x_idx].flags1 & SKF1_MKEY_SPELL) {
		if (item < 0) {
			item_tester_tval = s_info[x_idx].tval;
			if (!c_get_item(&item, "Cast from which book? ", (USE_INVEN |
			    USE_EQUIP | /* for WIELD_BOOKS */
			    NO_FAIL_MSG))) {
				if (item == -2) c_msg_print("You have no books that you can cast from.");
				return;
			}
		}

		/* Ask for a spell, allow cancel */
		if (!get_spell(&spell, "cast", item)) return;

		/* Send it */
		Send_activate_skill(s_info[x_idx].action_mkey, item, spell, dir, 0, 0);

		return;
	}

	if (item < 0 && s_info[x_idx].flags1 & SKF1_MKEY_ITEM) {
		item_tester_tval = s_info[x_idx].tval;
		if (!c_get_item(&item, "Which item? ", (USE_EQUIP | USE_INVEN)))
			return;
	}

	if (s_info[x_idx].flags1 & SKF1_MKEY_DIRECTION) {
		if (!get_dir(&dir))
			return;
	}

	/* Send it */
	Send_activate_skill(s_info[x_idx].action_mkey, item, spell, dir, 0, 0);
}

/* Ask & execute a skill */
void do_cmd_activate_skill() {
	int x_idx = -1;

	/* Get the skill, if available */
	x_idx = do_cmd_activate_skill_aux();
	if (x_idx == -1) return;

	do_activate_skill(x_idx, -1);
}
