/* $Id$ */
/* File: c-spell.c */
/* Client-side spell stuff */

#include "angband.h"

/* NULL for ghost - FIXME */
static void print_spells(object_type *o_ptr) {
	int	i, col, realm, num;

	/* Hack -- handle ghosts right */
	if (p_ptr->ghost) {
		realm = REALM_GHOST;
		num = 0;
	} else {
		realm = find_realm(o_ptr->tval);
		num = o_ptr->sval;
	}


	/* Print column */
	col = 20;

	/* Title the list */
	prt("", 1, col);
	put_str("Name", 1, col + 5);
	put_str("Lv Mana Fail", 1, col + 35);

	/* Dump the spells */
	for (i = 0; i < 9; i++) {
		/* Check for end of the book */
		if (spell_info[realm][num][i][0] == '\0')
			break;

		/* Dump the info */
		prt(spell_info[realm][num][i], 2 + i, col);
	}

	/* Clear the bottom line */
	prt("", 2 + i, col);
}

static void print_mimic_spells() {
	int	i, col, j = 2, k, fail;
	char buf[90];

	/* Print column */
	col = 13;
	k = 0; /* next column? for forms with LOTS of spells (GWoP) */

	/* Title the list */
	prt("", 1, col);
	c_put_str(TERM_WHITE, "Name", 1, col + 4);
	c_put_str(TERM_WHITE, "Fail%", 1, col + 4 + 22 - 2);
	c_put_str(TERM_WHITE, "Cost", 1, col + 4 + 22 + 6 - 2);

	prt("", j, col);
	c_put_str(TERM_L_WHITE, " a) Polymorph Self into next known form", j++, col);

	prt("", j, col);
	c_put_str(TERM_L_WHITE, " b) Polymorph Self into next known form with fitting extremities", j++, col);

	prt("", j, col);
	c_put_str(TERM_L_WHITE, " c) Polymorph Self into..", j++, col);

	prt("", j, col);
	c_put_str(TERM_L_WHITE, " d) Set preferred immunity", j++, col);

	/* Dump the spells */
	for (i = 0; i < 32; i++) {
		/* Check for end of the book */
		if (!(p_ptr->innate_spells[0] & (1L << i)))
		  continue;

		fail = (innate_powers[i].sfail * adj_int_pow[p_ptr->stat_ind[A_INT]]) / 100;
		if (fail < 1) fail = 1;
		if (fail > 99) fail = 99;

		/* fill right side of the list with blanks, in case we don't have 2 columns of spells - looks better */
		if (!k) put_str("                                 ", j, col + 33);

		/* Dump the info */
		sprintf(buf, " %c) %-22s \377y%2d \377B%2d ", I2A(j - 2 + k * 16), monster_spells4[i].name,
		    fail, innate_powers[i].smana);
		Term_putstr(col + k * 33, j++, -1, TERM_WHITE, buf);

		/* check for beginning of 2nd column */
		if (j > 21) {
			j = 6;
			k = 1;
		}
	}
	for (i = 0; i < 32; i++) {
		/* Check for end of the book */
		if (!(p_ptr->innate_spells[1] & (1L << i)))
			continue;

		fail = (innate_powers[i + 32].sfail * adj_int_pow[p_ptr->stat_ind[A_INT]]) / 100;
		if (fail < 1) fail = 1;
		if (fail > 99) fail = 99;

		/* fill right side of the list with blanks, in case we don't have 2 columns of spells - looks better */
		if (!k) put_str("                                 ", j, col + 33);

 		/* Dump the info */
		sprintf(buf, " %c) %-22s \377y%2d \377B%2d ", I2A(j - 2 + k * 16), monster_spells5[i].name,
		    fail, innate_powers[i + 32].smana);
		Term_putstr(col + k * 33, j++, -1, TERM_WHITE, buf);

		/* check for beginning of 2nd column */
		if (j > 21) {
			j = 6;
			k = 1;
		}
	}
	for (i = 0; i < 32; i++) {
		/* Check for end of the book */
		if (!(p_ptr->innate_spells[2] & (1L << i)))
			continue;

		fail = (innate_powers[i + 64].sfail * adj_int_pow[p_ptr->stat_ind[A_INT]]) / 100;
		if (fail < 1) fail = 1;
		if (fail > 99) fail = 99;

		/* fill right side of the list with blanks, in case we don't have 2 columns of spells - looks better */
		if (!k) put_str("                                 ", j, col + 33);

		/* Dump the info */
		sprintf(buf, " %c) %-22s \377y%2d \377B%2d ", I2A(j - 2 + k * 16), monster_spells6[i].name,
		    fail, innate_powers[i + 64].smana);
		Term_putstr(col + k * 33, j++, -1, TERM_WHITE, buf);

		/* check for beginning of 2nd column */
		if (j > 21) {
			j = 6;
			k = 1;
		}
	}

	/* Clear the bottom line(s) */
	if (k) prt("", 22, col);
	prt("", j++, col + k * 35);
}


/*
 * Allow user to choose a spell/prayer from the given book.
 */

/* modified to accept certain capital letters for priest spells. -AD- */

int get_spell(s32b *sn, cptr prompt, int book) {
	int		i, num = 0;
	bool		flag, redraw, okay;
	char		choice;
	char		out_val[160];
	cptr p;

	object_type *o_ptr = &inventory[book];
	int realm = find_realm(o_ptr->tval);
	/* see Receive_inven .. one of the dirtiest hack ever :( */
	int sval = o_ptr->sval;

	p = "spell";

	if (p_ptr->ghost) {
		p = "power";
		realm = REALM_GHOST;
		book = 0;
		sval = 0;
	}

	/* Assume no usable spells */
	okay = FALSE;

	/* Assume no spells available */
	(*sn) = -2;

	/* Check for "okay" spells */
	for (i = 0; i < 9; i++) {
		/* Look for "okay" spells */
		if (spell_info[realm][sval][i][0] &&
			/* Hack -- This presumes the spells are sorted by level */
			!strstr(spell_info[realm][sval][i],"unknown"))
		{
			okay = TRUE;
			num++;
		}
	}

	/* No "okay" spells */
	if (!okay) return (FALSE);


	/* Assume cancelled */
	(*sn) = -1;

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	/* Build a prompt (accept all spells) */
	if (num)
		strnfmt(out_val, 78, "(%^ss %c-%c, *=List, ESC=exit) %^s which %s? ",
		    p, I2A(0), I2A(num - 1), prompt, p);
	else
		strnfmt(out_val, 78, "No %^ss available - ESC=exit", p);

	if (c_cfg.always_show_lists) {
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of spells */
		print_spells(o_ptr);
	}

	/* Get a spell from the user */
	while (!flag && get_com(out_val, &choice)) {
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?')) {
			/* Show the list */
			if (!redraw) {
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of spells */
				print_spells(o_ptr);
			}
			/* Hide the list */
			else {
				/* Hide list */
				redraw = FALSE;

				/* Restore the screen */
				Term_load();

				/* Flush any events */
				Flush_queue();
			}

			/* Ask again */
			continue;
		}

		/* hack for CAPITAL prayers (heal other) */
		/* hack once more for possible expansion */
		/* lowercase */
		if (islower(choice)) {
			i = A2I(choice);
			if (i >= num) i = -1;
		}

		/* uppercase... hope this is portable. */
		else if (isupper(choice)) {
			i = (choice - 'A') + 64;
			if (i-64 >= num) i = -1;
		}
		else i = -1;

		/* Totally Illegal */
		if (i < 0) {
			bell();
			continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw) {
		Term_load();

		/* Flush any events */
		Flush_queue();
	}


	/* Abort if needed */
	if (!flag) return (FALSE);

	/* Save the choice */
	(*sn) = i;

	/* Success */
	return (TRUE);
}


/*
 * Peruse the spells/prayers in a Book
 *
 * Note that *all* spells in the book are listed
 */
void show_browse(object_type *o_ptr) {
	/* Save the screen */
	Term_save();

	/* Display the spells */
	print_spells(o_ptr);

	/* Clear the top line */
	clear_topline();

	/* Prompt user */
	put_str("[Press any key to continue]", 0, 23);

	/* Wait for key */
	(void)inkey();

	/* Restore the screen */
	Term_load();

	/* Flush any events */
	Flush_queue();
}

static int get_mimic_spell(int *sn) {
	int i, num = 3 + 1; /* 3 polymorph self spells + set immunity */
	bool flag, redraw;
	char choice;
	char out_val[160];
	int corresp[200];
	bool safe_input = FALSE;

	/* Assume no spells available */
	(*sn) = -2;

	/* Init the Polymorph power */
	corresp[0] = 0;
	/* Init the Polymorph into fitting */
	corresp[1] = 1;
	/* Init the Polymorph into.. <#> power */
	corresp[2] = 2;
	/* Init the Set-Immunity power */
	corresp[3] = 3;

	/* Check for "okay" spells */
	for (i = 0; i < 32; i++) {
		/* Look for "okay" spells */
		if (p_ptr->innate_spells[0] & (1L << i)) {
			corresp[num] = i + 4;
			num++;
		}
	}
	for (i = 0; i < 32; i++) {
		/* Look for "okay" spells */
		if (p_ptr->innate_spells[1] & (1L << i)) {
			corresp[num] = i + 32 + 4;
			num++;
		}
	}
	for (i = 0; i < 32; i++) {
		/* Look for "okay" spells */
		if (p_ptr->innate_spells[2] & (1L << i)) {
			corresp[num] = i + 64 + 4;
			num++;
		}
	}


	/* Assume cancelled */
	(*sn) = -1;

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	/* Build a prompt (accept all spells) */
	strnfmt(out_val, 78, "(Powers %c-%c, *=List, @=Name, ESC=exit) use which power? ",
		I2A(0), I2A(num - 1));

	if (c_cfg.always_show_lists) {
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of spells */
		print_mimic_spells();
	}

	/* safe_macros avoids getting stuck in an unfulfilled (item-)prompt */
	if (parse_macro && c_cfg.safe_macros) safe_input = TRUE;

	/* Get a spell from the user */
	while (!flag && get_com(out_val, &choice)) {
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?')) {
			/* Show the list */
			if (!redraw) {
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of spells */
				print_mimic_spells();
			}
			/* Hide the list */
			else {
				/* Hide list */
				redraw = FALSE;

				/* Restore the screen */
				Term_load();

				/* Flush any events */
				Flush_queue();
			}

			/* Ask again */
			continue;
		} else if (choice == '@') {
			char buf[80];
			int c;
			strcpy(buf, "");
			if (!get_string("Power? ", buf, 79)) {
				if (redraw) {
					Term_load();
					Flush_queue();
				}
				return FALSE;
			}

			/* Find the power it is related to */
			for (i = 0; i < num; i++) {
				c = corresp[i];
				if (c >= 64 + 4) {
					if (!strcasecmp(buf, monster_spells6[c - 64 - 4].name)) {
						flag = TRUE;
						break;
					}
				} else if (c >= 32 + 4) {
					if (!strcasecmp(buf, monster_spells5[c - 32 - 4].name)) {
						flag = TRUE;
						break;
					}
				} else if (c >= 4) { /* checkin for >=4 is paranoia */
					if (!strcasecmp(buf, monster_spells4[c - 4].name)) {
						flag = TRUE;
						break;
					}
				}
			}
			if (flag) break;

			/* illegal input */
			bell();
			/* hack - cancel prompt if we're in a failed macro execution */
			if (safe_input && abort_prompt) {
				flush_now();
				break;
			}
			continue;
		} else {
			/* extract request */
			i = (islower(choice) ? A2I(choice) : -1);
			if (i >= num) i = -1;
		}

		/* Totally Illegal */
		if (i < 0) {
			bell();
			/* hack - cancel prompt if we're in a failed macro execution */
			if (safe_input && abort_prompt) {
				flush_now();
				break;
			}
			continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw) {
		Term_load();

		/* Flush any events */
		Flush_queue();
	}

	/* restore macro handling hack to default state */
	abort_prompt = FALSE;


	/* Abort if needed */
	if (!flag) return (FALSE);

	if (c_cfg.other_query_flag && !i) {
		sprintf(out_val, "Really change the form?");
		if (!get_check2(out_val, FALSE)) return(FALSE);
	}

	/* Save the choice */
	(*sn) = corresp[i];

	/* Success */
	return (TRUE);
}


static void print_immunities() {
	int col, j = 2;

	/* Print column */
	col = 15;

	/* Title the list */
	prt("", 1, col);
	put_str("Immunity", 1, col + 5);


	prt("", j, col);
	put_str("a) Check (view current preference, don't set it)", j++, col);

	prt("", j, col);
	put_str("b) None (pick randomly each time)", j++, col);

	prt("", j, col);
	put_str("c) Lightning", j++, col);

	prt("", j, col);
	put_str("d) Frost", j++, col);

	prt("", j, col);
	put_str("e) Acid", j++, col);

	prt("", j, col);
	put_str("f) Fire", j++, col);

	prt("", j, col);
	put_str("g) Poison", j++, col);

	prt("", j, col);
	put_str("h) Water", j++, col);
}


/*
 * Mimic
 */
void do_mimic() {
	int spell, j, dir, c = 0;
	char out_val[41];
	bool uses_dir = FALSE;
	bool redraw = FALSE;
	char choice;

	/* Ask for the spell */
	if (!get_mimic_spell(&spell)) return;

	/* later on maybe this can moved to server side, then no need for '20000 hack'.
	   Btw, 30000, the more logical one, doesnt work, dont ask me why */
	if (spell == 2) {
		bool is_string = FALSE;
		char *cptr;

		out_val[0] = '\0';
		get_string("Which form (name or number; 0 for player; -1 for previous) ? ", out_val, 40);
		out_val[40] = '\0';

		/* empty input? */
		if (strlen(out_val) == 0) return;

		/* did user input an index number or a name? */
		cptr = out_val;
		if (*cptr == '-') cptr++; //cover minus sign for '-1' hack
		while (*cptr) {
			if (*cptr < '0' || *cptr > '9') {
				is_string = TRUE;
				break;
			}
			cptr++;
		}

		/* input is a string? */
		if (is_string) {
			/* hack for quick'n'dirty macros */
			if (out_val[0] == '@') {
				for (j = 1; out_val[j]; j++) out_val[j - 1] = out_val[j];
				out_val[j - 1] = '\0';
			}

			if (monster_list_idx == 0) {
				c_msg_print("Cannot specify monster by name: File lib/game/r_info.txt not found!");
				return;
			}

			for (j = 0; j < monster_list_idx; j++) {
				if (!strcasecmp(monster_list_name[j], out_val)) {
					spell = 20000 + monster_list_code[j];
					break;
				}
			}
			if (j == monster_list_idx) {
				c_msg_print("Form not found. Make sure the file lib/game/r_info.txt is up to date.");
				return;
			}
		}

		/* input is a number */
		else {
			j = atoi(out_val);
			if (j == -1) spell = 32767; //hack-o-mat -> 32767 is marker for 'previous form'
			else if ((j < 0) || (j > 2767)) return; //<-32767 limit intended? but we only add 20000? =P
			else spell = 20000 + j;
		}
	}

	else if (spell == 3) {
		if (!is_newer_than(&server_version, 4, 4, 9, 1, 0, 0)) {
			c_msg_print("Server version too old - this feature is not available.");
			return;
		}

		if (c_cfg.always_show_lists) {
			/* Show list */
			redraw = TRUE;

			/* Save the screen */
			Term_save();

			/* Display a list of spells */
			print_immunities();
		}

		/* Get a spell from the user */
		while (get_com("(Immunities a-h, *=List, @=Name, ESC=exit) Prefer which immunity? ", &choice)) {
			/* Request redraw */
			if ((choice == ' ') || (choice == '*') || (choice == '?')) {
				/* Show the list */
				if (!redraw) {
					/* Show list */
					redraw = TRUE;

					/* Save the screen */
					Term_save();

					/* Display a list of spells */
					print_immunities();
				}

				/* Hide the list */
				else {
					/* Hide list */
					redraw = FALSE;

					/* Restore the screen */
					Term_load();

					/* Flush any events */
					Flush_queue();
				}

				/* Ask again */
				continue;
			} else if (choice == '@') {
				char buf[80];
				strcpy(buf, "");
				if (!get_string("Immunity? ", buf, 49)) {
					if (redraw) {
						Term_load();
						Flush_queue();
					}
					return;
				}
				buf[49] = 0;

				/* Find the power it is related to */
				if (!strcasecmp(buf, "Check")) c = 1;
				else if (!strcasecmp(buf, "None")) c = 2;
				else if (!strcasecmp(buf, "Lightning")) c = 3;
				else if (!strcasecmp(buf, "Frost")) c = 4;
				else if (!strcasecmp(buf, "Acid")) c = 5;
				else if (!strcasecmp(buf, "Fire")) c = 6;
				else if (!strcasecmp(buf, "Poison")) c = 7;
				else if (!strcasecmp(buf, "Water")) c = 8;
				if (c) break;
			} else if (choice >= 'a' && choice <= 'h') {
				/* extract request */
				c = A2I(choice) + 1;
				break;
			}

			/* illegal input */
			bell();
			continue;
		}

		/* Restore the screen */
		if (redraw) {
			Term_load();

			/* Flush any events */
			Flush_queue();
		}

		if (c) Send_activate_skill(MKEY_MIMICRY, 0, 25000, c, 0, 0);
		return;
	}

	/* Spell requires direction? */
	else if (spell > 3 && is_newer_than(&server_version, 4, 4, 5, 10, 0, 0)) {
		j = spell - 4;
		spell--; //easy backward compatibility, spell == 3 has already been taken care of by 20000 hack
		if (j < 32) uses_dir = monster_spells4[j].uses_dir;
		else if (j < 64) uses_dir = monster_spells5[j - 32].uses_dir;
		else uses_dir = monster_spells6[j - 64].uses_dir;

		if (uses_dir) {
			if (get_dir(&dir))
				Send_activate_skill(MKEY_MIMICRY, 0, spell, dir, 0, 0);
			return;
		}
	}

	Send_activate_skill(MKEY_MIMICRY, 0, spell, 0, 0, 0);
}

/*
 * Use a ghost ability
 */
/* XXX XXX This doesn't work at all!! */
void do_ghost(void) {
	s32b j;

	/* Ask for an ability, allow cancel */
	if (!get_spell(&j, "use", 0)) return;

	/* Tell the server */
	Send_ghost(j);
}


/*
 * Schooled magic
 */

/*
 * Find a spell in any books/objects
 *
 * This is modified code from ToME. - mikaelh
 *
 * Note: Added bool inven_first because it's used for
 * get_item_extra_hook; it's unused here though - C. Blue
 */
static int hack_force_spell = -1;
int hack_force_spell_level = 0;
#ifndef DISCRETE_SPELL_SYSTEM /* good method before the big spell system rework */
bool get_item_hook_find_spell(int *item, int mode) {
	int i, spell;
	char buf[80];
	char buf2[100];

	(void) mode; /* suppress compiler warning */

	strcpy(buf, "Manathrust");
	if (!get_string("Spell name? ", buf, 79))
		return FALSE;
	sprintf(buf2, "return find_spell(\"%s\")", buf);
	spell = exec_lua(0, buf2);
	if (spell == -1) return FALSE;

	for (i = 0; i < INVEN_TOTAL; i++) {
		object_type *o_ptr = &inventory[i];

		if (o_ptr->tval == TV_BOOK) {
			/* A random book ? */
			if ((o_ptr->sval == SV_SPELLBOOK) && (o_ptr->pval == spell)) {
				*item = i;
				hack_force_spell = spell;
				return TRUE;
			}
			/* A normal book */
			else {
				sprintf(buf2, "return spell_in_book2(%d, %d, %d)", i, o_ptr->sval, spell);
				if (exec_lua(0, buf2)) {
					*item = i;
					hack_force_spell = spell;
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}
#else /* new method that allows to enter partial spell names, for comfortable 'I/II/III..' handling */
#define ALLOW_DUPLICATE_NAMES /* Allow multiple spells of the same name? (in different schools). It picks the spell with highest level. */
bool get_item_hook_find_spell(int *item, int mode) {
	int i, spos, highest = 0, current, spell;
#ifdef ALLOW_DUPLICATE_NAMES
	int slev, slev_max = 0;
#endif
	char buf[80], buf2[100], sname[40], *bufptr;
	object_type *o_ptr;
	bool exact_match = FALSE, combo_spell, found_something = FALSE;

	(void) mode; /* suppress compiler warning */


	strcpy(buf, "");
	if (!get_string("Spell name? ", buf, 79)) return FALSE;

	if (strlen(buf) >= 4) {
		bufptr = buf + strlen(buf);
		if (!strcasecmp(bufptr - 2, " I") ||
		    !strcasecmp(bufptr - 3, " II") ||
		    !strcasecmp(bufptr - 4, " III") ||
		    !strcasecmp(bufptr - 3, " IV") ||
		    !strcasecmp(bufptr - 2, " V") ||
		    !strcasecmp(bufptr - 3, " VI"))
			exact_match = TRUE;
	}

	for (i = 0; i < INVEN_TOTAL; i++) {
		o_ptr = &inventory[i];
		if (o_ptr->tval != TV_BOOK) continue;

		spos = 0;
		do {
			spos++;
			sprintf(buf2, "return get_spellname_in_book(%d, %d)", i, spos); //note: get_spellname_in_book() is not ready for DSS_EXPANDED_SCROLLS!
			strcpy(sname, string_exec_lua(0, buf2));
			if (!sname[0]) break;

			/* does this spell actually have different versions? */
			combo_spell = FALSE;
			current = 0;
			if (strlen(sname) >= 4) {
				bufptr = sname + strlen(sname);
				if (!strcasecmp(bufptr - 2, " I")) current = 1;
				if (!strcasecmp(bufptr - 3, " II")) current = 2;
				if (!strcasecmp(bufptr - 4, " III")) current = 3;
				if (!strcasecmp(bufptr - 3, " IV")) current = 4;
				if (!strcasecmp(bufptr - 2, " V")) current = 5;
				if (!strcasecmp(bufptr - 3, " VI")) current = 6;
				if (current) combo_spell = TRUE;
			}

			/* Only allow testing for partial match if it's a combo spell */
			if (exact_match || !combo_spell) {
				if (strcasecmp(buf, sname)) continue;
			} else {
				if (strncasecmp(buf, sname, strlen(buf))) continue;
			}

			/* get the spell's spell array index */
#ifndef ALLOW_DUPLICATE_NAMES /* old method: doesn't allow two spells of similar name (when in different schools), and is redundant as we already know item+pos of spell -> no need to search for it again */
			sprintf(buf2, "return find_spell(\"%s\")", sname);
#else
			sprintf(buf2, "return find_spell_from_item(%d, %d, \"%s\")", i, spos, buf);
#endif
			spell = exec_lua(0, buf2);
			if (spell == -1) return FALSE; /* paranoia - can't happen */

			/* hack: if this is a multi-version spell, keep it in mind but continue
			   checking, if we can not cast this particular version of it. */
			if (!exact_match && combo_spell) {
				if (!found_something) {
					/* assume that we can probably cast this spell */
					*item = i;
					hack_force_spell = spell;
					found_something = TRUE;
				}

#ifndef ALLOW_DUPLICATE_NAMES /* old method: doesn't allow two spells of similar name (when in different schools) */
				sprintf(buf2, "return is_ok_spell(0, %d)", spell);
				if (!exec_lua(0, buf2)) continue;
#else
				sprintf(buf2, "return is_ok_spell2(0, %d)", spell);
				if (!(slev = exec_lua(0, buf2))) continue;
				if (slev > slev_max && current == highest) {
					slev_max = slev;
					/* the highest spell we found so far that we can cast -
					   it's same tier, but from a different school and actually of higher spell-level! */
					*item = i;
					hack_force_spell = spell;
				}
#endif

				if (current > highest) {
					/* the highest spell we found so far that we can cast */
					*item = i;
					hack_force_spell = spell;
					highest = current;
				}

				/* continue checking, maybe we find a higher spell variant that is still usable */
				continue;
			}

#ifndef ALLOW_DUPLICATE_NAMES /* old method: doesn't allow two spells of similar name (when in different schools) */
			/* assume that we can probably cast this spell */
			*item = i;
			hack_force_spell = spell;
			found_something = TRUE;

			/* Ok, we found a spell that we can use, or we didn't find
			   an alternative compound-spell's sub-spell variant. */
			return TRUE;
#else
			sprintf(buf2, "return is_ok_spell2(0, %d)", spell);
			if (!(slev = exec_lua(0, buf2))) continue;
			if (slev > slev_max) {
				slev_max = slev;
				/* the highest spell we found so far that we can cast -
				   it's same tier, but from a different school and actually of higher spell-level! */
				*item = i;
				hack_force_spell = spell;
				found_something = TRUE;
			}
			//continue searching, maybe we find a spell of higher level and same name from another school..
#endif
		} while (TRUE);
	}

	if (found_something) return TRUE;
	return FALSE;
}
#endif

/*
 * Get a spell from a book
 */
s32b get_school_spell(cptr do_what, int *item_book) {
	int i, item;
	u32b spell = -1;
	int num = 0, where = 1;
	int ask;
	bool flag, redraw;
	char choice;
	char out_val[160], out_val2[160];
	char buf2[40];
	object_type *o_ptr;
	//int tmp;
	int sval, pval;

	hack_force_spell = -1;
	hack_force_spell_level = 0;

	if (*item_book < 0) {
		get_item_extra_hook = get_item_hook_find_spell;
		item_tester_tval = TV_BOOK;
		sprintf(buf2, "You have no book to %s from.", do_what);
		sprintf(out_val, "%s from which book?", do_what);
		out_val[0] = toupper(out_val[0]);
		if (sflags1 & SFLG1_LIMIT_SPELLS) {
			if (!c_get_item(&item, out_val, (USE_INVEN | USE_EXTRA | USE_LIMIT | NO_FAIL_MSG) )) {
				if (item == -2) c_msg_format("%s", buf2);
				return -1;
			}
		} else {
			if (!c_get_item(&item, out_val, (USE_INVEN | USE_EXTRA | NO_FAIL_MSG) )) {
				if (item == -2) c_msg_format("%s", buf2);
				return -1;
			}
		}
	} else {
		/* Already given */
		item = *item_book;
	}

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &inventory[item];
	else return(-1);

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	/* No spell to cast by default */
	spell = -1;

	/* Is it a random book, or something else ? */
	if (o_ptr->tval == TV_BOOK) {
		sval = o_ptr->sval;
		pval = o_ptr->pval;
	} else {
		/* Only books allowed */
		return -1;
	}

	if (hack_force_spell == -1) {
		sprintf(out_val, "return book_spells_num2(%d, %d)", item, sval);
		num = exec_lua(0, out_val);

		/* Build a prompt (accept all spells) */
		if (num)
			strnfmt(out_val2, 78, "(Spells %c-%c, Descs %c-%c, *=List, ESC=exit) %^s which spell? ",
			    I2A(0), I2A(num - 1), I2A(0) - 'a' + 'A', I2A(num - 1) - 'a' + 'A',  do_what);
		else
			strnfmt(out_val2, 78, "No spells available - ESC=exit");

		if (c_cfg.always_show_lists) {
			/* Show list */
			redraw = TRUE;

			/* Save the screen */
			Term_save();

			/* Display a list of spells */
			sprintf(out_val, "return print_book2(0, %d, %d, %d)", item, sval, pval);
			where = exec_lua(0, out_val);
		}

		/* Get a spell from the user */
		while (!flag && get_com(out_val2, &choice)) {
			/* Request redraw */
			if (((choice == ' ') || (choice == '*') || (choice == '?'))) {
				/* Show the list */
				if (!redraw) {
					/* Show list */
					redraw = TRUE;

					/* Save the screen */
					Term_save();

					/* Display a list of spells */
					sprintf(out_val, "return print_book2(0, %d, %d, %d)", item, sval, pval);
					where = exec_lua(0, out_val);
				}

				/* Hide the list */
				else {
					/* Hide list */
					redraw = FALSE;
					where = 1;

					/* Restore the screen */
					Term_load();
				}

				/* Redo asking */
				continue;
			}

			/* Note verify */
			ask = (isupper(choice));

			/* Lowercase */
			if (ask) choice = tolower(choice);

			/* Extract request */
			i = (islower(choice) ? A2I(choice) : -1);

			/* Totally Illegal */
			if ((i < 0) || (i >= num)) {
				bell();
				continue;
			}

			/* Verify it */
			if (ask) {
				/* Show the list */
				if (!redraw) {
					/* Show list */
					redraw = TRUE;

					/* Save the screen */
					Term_save();

				}

				/* Display a list of spells */
				sprintf(out_val, "return print_book2(0, %d, %d, %d)", item, sval, pval);
				where = exec_lua(0, out_val);
				sprintf(out_val, "print_spell_desc(spell_x2(%d, %d, %d, %d), %d)", item, sval, pval, i, where);
				exec_lua(0, out_val);
			} else {
				/* Save the spell index */
				sprintf(out_val, "return spell_x2(%d, %d, %d, %d)", item, sval, pval, i);
				spell = exec_lua(0, out_val);

				/* Require "okay" spells */
				sprintf(out_val, "return is_ok_spell(0, %d)", spell);
				if (!exec_lua(0, out_val)) {
					bell();
					c_msg_format("You may not %s that spell.", do_what);
					spell = -1;
					continue;
				}

				/* Stop the loop */
				flag = TRUE;
			}
		}
	} else {
		/* Require "okay" spells */
		sprintf(out_val, "return is_ok_spell(0, %d)", hack_force_spell);
		if (exec_lua(0, out_val)) {
			flag = TRUE;
			spell = hack_force_spell;
		} else {
			bell();
			c_msg_format("You may not %s that spell.", do_what);
			spell = -1;
		}
	}

	/* Restore the screen */
	if (redraw) Term_load();

	/* Abort if needed */
	if (!flag) return -1;

	//tmp = spell;
	*item_book = item;
	return (spell + 1000 * hack_force_spell_level);
}

/* TODO: handle blindness */
void browse_school_spell(int item, int book, int pval) {
	int i;
	int num = 0, where = 1;
	int ask;
	char choice;
	char out_val[160], out_val2[160];
	int sval = book;

#ifdef USE_SOUND_2010
	if (sval == SV_SPELLBOOK) sound(browse_sound_idx, SFX_TYPE_COMMAND, 100, 0);
	else sound(browsebook_sound_idx, SFX_TYPE_COMMAND, 100, 0);
#endif

	sprintf(out_val, "return book_spells_num2(%d, %d)", item, sval);
	num = exec_lua(0, out_val);

	/* Build a prompt (accept all spells) */
	if (num)
		strnfmt(out_val2, 78, "(Spells %c-%c, ESC=exit) which spell? ", I2A(0), I2A(num - 1));
	else
		strnfmt(out_val2, 78, "No spells available - ESC=exit");

	/* Save the screen */
	Term_save();

	/* Display a list of spells */
	sprintf(out_val, "return print_book2(0, %d, %d, %d)", item, sval, pval);
	where = exec_lua(0, out_val);

	/* Get a spell from the user */
	while (get_com(out_val2, &choice)) {
		/* Restore the screen (ie Term_load() without 'popping' it) */
		Term_restore();

		/* Display a list of spells */
		sprintf(out_val, "return print_book2(0, %d, %d, %d)", item, sval, pval);
		where = exec_lua(0, out_val);

		/* Note verify */
		ask = (isupper(choice));

		/* Lowercase */
		if (ask) choice = tolower(choice);

		/* Extract request */
		i = (islower(choice) ? A2I(choice) : -1);

		/* Totally Illegal */
		if ((i < 0) || (i >= num)) {
			bell();
			continue;
		}

		/* Restore the screen */
		/* Term_load(); */

		/* Display a list of spells */
		sprintf(out_val, "return print_book2(0, %d, %d, %d)", item, sval, pval);
		where = exec_lua(0, out_val);
		sprintf(out_val, "print_spell_desc(spell_x2(%d, %d, %d, %d), %d)", item, sval, pval, i, where);
		exec_lua(0, out_val);
	}

	/* Restore the screen */
	Term_load();
}

static void print_combatstances() {
	int col = 20, j = 2;

	/* Title the list */
	prt("", 1, col);
	put_str("Name", 1, col + 5);

	prt("", j, col);
	put_str("a) Balanced stance (standard)", j++, col);

	prt("", j, col);
	put_str("b) Defensive stance", j++, col);

	prt("", j, col);
	put_str("c) Offensive stance", j++, col);

	/* Clear the bottom line */
	prt("", j++, col);
}

static int get_combatstance(int *cs) {
	int		i = 0, num = 3; /* number of pre-defined stances here in this function */
	bool		flag, redraw;
	char		choice;
	char		out_val[160];
	int             corresp[5];

	corresp[0] = 0; /* balanced stance */
	corresp[1] = 1; /* defensive stance */
	corresp[2] = 2; /* offensive stance */

	/* Assume cancelled */
	(*cs) = -1;
	/* Nothing chosen yet */
	flag = FALSE;
	/* No redraw yet */
	redraw = FALSE;

	/* Build a prompt (accept all stances) */
	strnfmt(out_val, 78, "(Stances %c-%c, *=List, ESC=exit) enter which stance? ",
		I2A(0), I2A(num - 1));

	if (c_cfg.always_show_lists) {
		/* Show list */
		redraw = TRUE;
		/* Save the screen */
		Term_save();
		/* Display a list of stances */
		print_combatstances();
	}

	/* Get a stance from the user */
	while (!flag && get_com(out_val, &choice)) {
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?')) {
			/* Show the list */
			if (!redraw) {
				/* Show list */
				redraw = TRUE;
				/* Save the screen */
				Term_save();
				/* Display a list of stances */
				print_combatstances();
			}

			/* Hide the list */
			else {
				/* Hide list */
				redraw = FALSE;
				/* Restore the screen */
				Term_load();
				/* Flush any events */
				Flush_queue();
			}

			/* Ask again */
			continue;
		}

		/* extract request */
		i = (islower(choice) ? A2I(choice) : -1);
		if (i >= num) i = -1;

		/* Totally Illegal */
		if (i < 0) {
			bell();
			continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw) {
		Term_load();
		/* Flush any events */
		Flush_queue();
	}

	/* Abort if needed */
	if (!flag) return (FALSE);

	/* Save the choice */
	(*cs) = corresp[i];
	/* Success */
	return (TRUE);
}

/*
 * Enter a combat stance (warriors) - C. Blue
 */
void do_stance() {
	int stance;
	/* Ask for the stance */
	if (!get_combatstance(&stance)) return;
	Send_activate_skill(MKEY_STANCE, stance, 0, 0, 0, 0);
}


static void print_melee_techniques() {
	int i, col, j = 0;
	char buf[90];
	/* Print column */
	col = 20;
	/* Title the list */
	prt("", 1, col);
	put_str("Name", 1, col + 5);
	/* Dump the techniques */
	for (i = 0; i < 16; i++) {
		/* Check for accessible technique */
		if (!(p_ptr->melee_techniques & (1L << i)))
		  continue;
		/* Dump the info */
		sprintf(buf, "%c) %s", I2A(j), melee_techniques[i]);
		prt(buf, 2 + j++, col);
	}
	/* Clear the bottom line */
	prt("", 2 + j++, col);
}

static int get_melee_technique(int *sn) {
	int		i, num = 0;
	bool		flag, redraw;
	char		choice;
	char		out_val[160];
	int             corresp[32];

	/* Assume no techniques available */
	(*sn) = -2;

	/* Check for accessible techniques */
	for (i = 0; i < 32; i++) {
		/* Look for accessible techniques */
		if (p_ptr->melee_techniques & (1L << i)) {
			corresp[num] = i;
			num++;
		}
	}

	/* Assume cancelled */
	(*sn) = -1;

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	/* Build a prompt (accept all techniques) */
	if (num)
		strnfmt(out_val, 78, "(Techniques %c-%c, *=List, @=Name, ESC=exit) use which technique? ",
		    I2A(0), I2A(num - 1));
	else
		strnfmt(out_val, 78, "No techniques available - ESC=exit");

	if (c_cfg.always_show_lists) {
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of techniques */
		print_melee_techniques();
	}

	/* Get a technique from the user */
	while (!flag && get_com(out_val, &choice)) {
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?')) {
			/* Show the list */
			if (!redraw) {
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of techniques */
				print_melee_techniques();
			}
			/* Hide the list */
			else {
				/* Hide list */
				redraw = FALSE;

				/* Restore the screen */
				Term_load();

				/* Flush any events */
				Flush_queue();
			}

			/* Ask again */
			continue;
		}
		else if (choice == '@') {
			char buf[80];

			strcpy(buf, "Sprint");
			if (!get_string("Technique? ", buf, 79)) {
				if (redraw) {
					Term_load();
					Flush_queue();
				}
				return FALSE;
			}

			/* Find the skill it is related to */
			for (i = 0; i < num; i++) {
				if (!strcasecmp(buf, melee_techniques[corresp[i]])) {
					flag = TRUE;
					break;
				}
			}
			if (flag) break;

			/* illegal input */
			bell();
			continue;
		} else {
			/* extract request */
			i = (islower(choice) ? A2I(choice) : -1);
			if (i >= num) i = -1;

			/* Totally Illegal */
			if (i < 0) {
				bell();
				continue;
			}

			/* Stop the loop */
			flag = TRUE;
		}
	}

	/* Restore the screen */
	if (redraw) {
		Term_load();

		/* Flush any events */
		Flush_queue();
	}


	/* Abort if needed */
	if (!flag) return (FALSE);

	/* Save the choice */
	(*sn) = corresp[i];

	/* Success */
	return (TRUE);
}

void do_melee_technique() {
	int technique;

	/* Ask for the technique */
	if (!get_melee_technique(&technique)) return;

	Send_activate_skill(MKEY_MELEE, 0, technique, 0, 0, 0);
}

static void print_ranged_techniques() {
	int i, col, j = 0;
	char buf[90];

	/* Print column */
	col = 20;
	/* Title the list */
	prt("", 1, col);
	put_str("Name", 1, col + 5);
	/* Dump the techniques */
	for (i = 0; i < 16; i++) {
		/* Check for accessible technique */
		if (!(p_ptr->ranged_techniques & (1L << i))) continue;
		/* Dump the info */
		sprintf(buf, "%c) %s", I2A(j), ranged_techniques[i]);
		prt(buf, 2 + j++, col);
	}
	/* Clear the bottom line */
	prt("", 2 + j++, col);
}

static int get_ranged_technique(int *sn) {
	int		i, num = 0;
	bool		flag, redraw;
	char		choice;
	char		out_val[160];
	int             corresp[32];

	/* Assume no techniques available */
	(*sn) = -2;

	/* Check for accessible techniques */
	for (i = 0; i < 32; i++) {
		/* Look for accessible techniques */
		if (p_ptr->ranged_techniques & (1L << i)) {
			corresp[num] = i;
			num++;
		}
	}

	/* Assume cancelled */
	(*sn) = -1;

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	/* Build a prompt (accept all techniques) */
	if (num)
		strnfmt(out_val, 78, "(Techniques %c-%c, *=List, @=Name, ESC=exit) use which technique? ",
		    I2A(0), I2A(num - 1));
	else
		strnfmt(out_val, 78, "No techniques available - ESC=exit");

	if (c_cfg.always_show_lists) {
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of techniques */
		print_ranged_techniques();
	}

	/* Get a technique from the user */
	while (!flag && get_com(out_val, &choice)) {
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?')) {
			/* Show the list */
			if (!redraw) {
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of techniques */
				print_ranged_techniques();
			}

			/* Hide the list */
			else {
				/* Hide list */
				redraw = FALSE;

				/* Restore the screen */
				Term_load();

				/* Flush any events */
				Flush_queue();
			}

			/* Ask again */
			continue;
		}
		else if (choice == '@') {
			char buf[80];

			strcpy(buf, "Flare Missile");
			if (!get_string("Technique? ", buf, 79)) {
				if (redraw) {
					Term_load();
					Flush_queue();
				}
				return FALSE;
			}

			/* Find the skill it is related to */
			for (i = 0; i < num; i++) {
				if (!strcasecmp(buf, ranged_techniques[corresp[i]])) {
					flag = TRUE;
					break;
				}
			}
			if (flag) break;

			/* illegal input */
			bell();
			continue;
		} else {
			/* extract request */
			i = (islower(choice) ? A2I(choice) : -1);
			if (i >= num) i = -1;

			/* Totally Illegal */
			if (i < 0) {
				bell();
				continue;
			}

			/* Stop the loop */
			flag = TRUE;
		}
	}

	/* Restore the screen */
	if (redraw) {
		Term_load();

		/* Flush any events */
		Flush_queue();
	}


	/* Abort if needed */
	if (!flag) return (FALSE);

	/* Save the choice */
	(*sn) = corresp[i];

	/* Success */
	return (TRUE);
}

void do_ranged_technique() {
	int technique;

	/* Ask for the technique */
	if (!get_ranged_technique(&technique)) return;

	Send_activate_skill(MKEY_RANGED, 0, technique, 0, 0, 0);
}

/** Duplicate Code (non-static for use in c-util.c) -- /server/runecraft.c  - Kurzel - To be LUAized! **/

byte flags_to_elements(byte element[], u16b e_flags) {
	byte elements = 0;
	byte i;
	for (i = 0; i < RCRAFT_MAX_ELEMENTS; i++) {
		if ((e_flags & r_elements[i].flag) == r_elements[i].flag) {
			element[elements] = i;
			elements++;
		}
	}
	return elements;
}

byte flags_to_imperative(u16b m_flags) {
	byte i;
	for (i = 0; i < RCRAFT_MAX_IMPERATIVES; i++) {
		if ((m_flags & r_imperatives[i].flag) == r_imperatives[i].flag) return i;
	}
	return -1;
}

byte flags_to_type(u16b m_flags) {
	byte i;
	for (i = 0; i < RCRAFT_MAX_TYPES; i++)
		if ((m_flags & r_types[i].flag) == r_types[i].flag)
			return i;
	return -1;
}

byte flags_to_projection(u16b e_flags) {
	byte i;
	for (i = 0; i < RCRAFT_MAX_PROJECTIONS; i++) {
		if (e_flags == r_projections[i].flags)
			return i;
	}
	return -1;
}

byte rspell_skill(byte element[], byte elements) {
	u16b skill = 0;
	byte i;

#ifdef ENABLE_AVERAGE_SKILL //calculate an average
	for (i = 0; i < elements; i++) {
		skill += get_skill(r_elements[element[i]].skill);
	}
	skill /= elements;
#else //take the lowest value
	u16b skill_compare;

	skill = skill - 1; //overflow to largest u16b value
	for (i = 0; i < elements; i++) {
		skill_compare = get_skill(r_elements[element[i]].skill);
		if (skill_compare < skill) skill = skill_compare;
	}
#endif
	return (byte)skill;
}

byte rspell_level(byte imperative, byte type) {
	byte level = 0;

	level += r_imperatives[imperative].level;
	level += r_types[type].level;
	return level;
}

s16b rspell_diff(byte skill, byte level) {
	s16b diff = skill - level + 1;

	if (diff > S_DIFF_MAX) return S_DIFF_MAX;
	else return diff;
}

byte rspell_cost(byte imperative, byte type, byte skill) {
	u16b cost = r_types[type].c_min + rget_level(r_types[type].c_max - r_types[type].c_min);
	cost = cost * r_imperatives[imperative].cost / 10;
	if (cost < S_COST_MIN) cost = S_COST_MIN;
	if (cost > S_COST_MAX) cost = S_COST_MAX;
	return (byte)cost;
}

byte rspell_fail(byte imperative, byte type, s16b diff, u16b penalty) {
	/* Set the base failure rate; currently 50% at equal skill to level such that the range is [5,95] over 30 levels */
	s16b fail = 3 * (S_DIFF_MAX - diff) + 5;

	/* Modifier */
	fail += r_imperatives[imperative].fail;

	/* Status Penalty */
	fail += penalty; //Place before stat modifier; casters of great ability can reduce the penalty for casting while hindered?

	/* Reduce failure rate by STAT adjustment */
	fail -= ((adj_mag_stat[p_ptr->stat_ind[A_INT]] * 65 + adj_mag_stat[p_ptr->stat_ind[A_DEX]] * 35) / 100) - 3;

	/* Extract the minimum failure rate */
	s16b minfail = (adj_mag_fail[p_ptr->stat_ind[A_INT]] * 65 + adj_mag_fail[p_ptr->stat_ind[A_DEX]] * 35) / 100;

	/* Minimum failure rate */
	if (fail < minfail) fail = minfail;

	/* Always a 5 percent chance of working */
	if (fail > 95) fail = 95;

	return (byte)fail;
}

u16b rspell_damage(u32b *dx, u32b *dy, byte imperative, byte type, byte skill, byte projection) {
	u32b damage = rget_weight(r_projections[projection].weight);
	u32b d1 = r_types[type].d1min + rget_level(r_types[type].d1max - r_types[type].d1min);
	u32b d2 = r_types[type].d2min + rget_level(r_types[type].d2max - r_types[type].d2min);

	/* Weight - Always a reduction if applicable, squarer damage for low weight elements when d1 > d2. - Kurzel */
	d1 = d1 * damage / S_WEIGHT_HI;
	damage = (r_types[type].dbmin + rget_level(r_types[type].dbmax - r_types[type].dbmin)) * damage / S_WEIGHT_HI;

	/* Modifier - Usually an increase, squarer damage for high damage modifiers when d1 > d2. - Kurzel  */ 
	d2 = d2 * r_imperatives[imperative].damage / 10; 
	damage = damage * r_imperatives[imperative].damage / 10;

	/* Return */
	*dx = (byte)d1;
	*dy = (byte)d2;

	return (u16b)damage;
}

byte rspell_radius(byte imperative, byte type, byte skill, byte projection) {
	s16b radius = r_types[type].r_min + rget_level(r_types[type].r_max - r_types[type].r_min);
	radius += r_imperatives[imperative].radius;
	if (radius < S_RADIUS_MIN) radius = S_RADIUS_MIN;
	if (radius > S_RADIUS_MAX) radius = S_RADIUS_MAX;
	return (byte)radius;
}

byte rspell_duration(byte imperative, byte type, byte skill, byte projection, u16b dice) {
	s16b duration = r_types[type].d_min + rget_level(r_types[type].d_max - r_types[type].d_min);
	duration = duration * r_imperatives[imperative].duration / 10;
	if (duration < S_DURATION_MIN) duration = S_DURATION_MIN;
	if (duration > S_DURATION_MAX) duration = S_DURATION_MAX;
	return (byte)duration;
}

/** End Duplicate Code **/

/* For improved activation code Receive_item() nclient.c */
bool item_tester_hook_rune(object_type *o_ptr) {
	if ((o_ptr->tval == TV_RUNE)
	 && (o_ptr->sval < RCRAFT_MAX_ELEMENTS))
		return TRUE;
	return FALSE;
}

/* Improve this visually (grey out equipment, not inventory) in the future. See above note. - Kurzel */
static bool item_tester_hook_sigil(object_type *o_ptr) {
	/* Store tval for comparison speed */
	byte tval = o_ptr->tval;

	/* Weapons */
	if ((tval == TV_MSTAFF)
	 || (tval == TV_BLUNT)
	 || (tval == TV_POLEARM)
	 || (tval == TV_SWORD)
	 || (tval == TV_AXE)
	 || (tval == TV_BOOMERANG))
		return TRUE;

	/* Armours */
	if ((tval == TV_BOOTS)
	 || (tval == TV_GLOVES)
	 || (tval == TV_HELM)
	 || (tval == TV_CROWN)
	 || (tval == TV_SHIELD)
	 || (tval == TV_CLOAK)
	 || (tval == TV_SOFT_ARMOR)
	 || (tval == TV_HARD_ARMOR)
	 || (tval == TV_DRAG_ARMOR))
		return TRUE;

	return FALSE;
}

static void rcraft_print_types(u16b e_flags, u16b m_flags) {
	int col = 13, j = 2, i, color;
	char tmpbuf[80];

	/* Print the header */
	prt("", 1, col);
	put_str("   Form    Level Cost Fail Info", 1, col);

	/* Check for a penalty due to status */
	u16b penalty = 0;
	if (p_ptr->blind) penalty += 10;
	//if (p_ptr->confused) penalty += 10; //#ifdef ENABLE_CONFUSED_CASTING
	if (p_ptr->stun > 50) penalty += 25;
	else if (p_ptr->stun) penalty += 15;

	/* Analyze parameters */
	byte element[RCRAFT_MAX_ELEMENTS];
	byte elements = flags_to_elements(element, e_flags);
	/* Rune check for the restricted spells; enhanced, sign, glyph, sigil, boon. - Kurzel */
	byte projection = flags_to_projection(e_flags);
	bool has_rune = 0;
	for (i = 0; i < INVEN_PACK; i++) {
		if (inventory[i].tval == TV_RUNE && inventory[i].sval == projection) has_rune = 1;
	}
	byte skill = rspell_skill(element, elements);
	byte imperative = flags_to_imperative(m_flags);
	byte level, cost, fail;
	s16b diff, sdiff;

	/* Print the list */
	for (i = 0; i < RCRAFT_MAX_TYPES; i++) {

		/* Analyze parameters */
		level = rspell_level(imperative, i);
		diff = rspell_diff(skill, level);
		cost = rspell_cost(imperative, i, skill);
		fail = rspell_fail(imperative, i, diff, penalty);
		u32b dx, dy;
		u16b damage = rspell_damage(&dx, &dy, imperative, i, skill, projection);
		u16b dice = damroll(dx, dy);
		byte radius = rspell_radius(imperative, i, skill, projection);
		byte duration = rspell_duration(imperative, i, skill, projection, dice);

		/* Extra parameters */
		sdiff = skill - level + 1; //For a real 'level difference' display.

		/* Get the line color */
		if (diff > 0) {
			color = 'G';
			if (penalty) color = 'y';
			if (p_ptr->csp < cost) color = 'o';
			if (r_imperatives[imperative].flag == I_ENHA && !has_rune) color = 'R';
			//if (p_ptr->anti_magic && r_types[i].flag != T_SIGL) color = 'r'; //#define ENABLE_SHELL_ENCHANT
			if (p_ptr->anti_magic) color = 'r';
		}
		else color = 'D';

		/* Fill a line */
		switch (r_types[i].flag) {

			case T_BOLT: { //Beam
				if (r_imperatives[imperative].flag != I_ENHA) {
					sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %dd%d",
					color, 'a' + i, r_types[i].name, sdiff, cost, fail, dx, dy);
				} else {
					sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %dd%d",
					color, 'a' + i, "beam", sdiff, cost, fail, dx, dy);
				}
			break; }

			case T_CLOU: { //Storm
				if (r_imperatives[imperative].flag != I_ENHA) {
					sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d rad %d dur %d",
					color, 'a' + i, r_types[i].name, sdiff, cost, fail, rget_level(damage), radius, duration);
				} else {
					sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d rad %d dur %d",
					color, 'a' + i, "storm", sdiff, cost, fail, rget_level(damage)*15/10, 1, duration*2);
				}
			break; }

			case T_SIGN: { //Glyph
				if (!has_rune && color != 'D') color = 'R';
				if (r_imperatives[imperative].flag != I_ENHA) {
					switch (projection) {

						case SV_R_LITE: { //Illumination
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d rad %d illumination",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail, rspell_damage(&dx, &dy, imperative, flags_to_type(T_BALL), skill, projection)/4, 3+radius*2);
						break; }

						case SV_R_DARK: { //Invisibility
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dur %d+1d%d pow %d invisibility",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail, duration/2, duration/2, damage);
						break; }

						case SV_R_NEXU: { //Teleportation
							switch (r_imperatives[imperative].flag) {
								//Manual tuning; Phase Door (6-12), Blink (10), Teleport (100), Spell (100-200) - Kurzel
								case I_MINI: { radius = 12 + rget_level(12); break; }
								case I_LENG: { radius = 25 + rget_level(25); break; }
								case I_COMP: { radius =  6 + rget_level( 6); break; }
								case I_MODE: { radius = 25 + rget_level(25); break; }
								case I_EXPA: { radius = 75 + rget_level(75); break; }
								case I_BRIE: { radius = 25 + rget_level(25); break; }
								case I_MAXI: { radius = 50 + rget_level(50); break; }
							}
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% rad %d teleportation",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail, radius);
						break; }

						case SV_R_NETH: { //Annihilation
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% pow %d annihilation (bolt)",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail, 8+(damage+5)/10);
						break; }

						case SV_R_CHAO: { //Polymorph Self
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% polymorph self",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail);
						break; }

						case SV_R_MANA: { //Recharging
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% pow %d recharging",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail, damage);
						break; }

						case SV_R_CONF: { //Reflection
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dur %d+1d%d reflection",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail, duration/2, duration/2);
						break; }

						case SV_R_INER: { //Mass Stasis
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% pow %d mass stasis",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail, damage*2);
						break; }

						case SV_R_ELEC:
						case SV_R_FIRE:
						case SV_R_COLD:
						case SV_R_ACID:
						case SV_R_POIS: { //Infusion
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dur %d+1d%d %s infusion",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail, duration/2, duration/2, r_projections[projection].name);
						break; }

						case SV_R_WATE: { //Quench Thirst
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% quench thirst",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail);
						break; }

						case SV_R_GRAV: { //Mass Teleport-To
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% pow %d mass teleport-to",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail, damage*2);
						break; }

						case SV_R_SHAR: { //Dig
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dig (bolt)",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail);
						break; }

						case SV_R_SOUN: { //Disarm
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% rad %d disarm",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail, radius);
						break; }

						case SV_R_TIME: { //Haste
							damage = rget_level(15) * r_imperatives[imperative].damage / 10;
							if (damage < 1) damage = 1;
							if (damage > 10) damage = 10;
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dur %d+1d%d +%d speed",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail, duration/2, duration/2, damage);
						break; }

						case SV_R_DISE: { //Resistance
							sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dur %d+1d%d base resistance",
							color, 'a' + i, r_types[i].name, sdiff, cost, fail, duration/2, duration/2);
						break; }

						case SV_R_ICEE:
						case SV_R_PLAS: { //Shield
								damage = rget_level(20) * r_imperatives[imperative].damage / 10;
								if (damage < 1) damage = 1;
								if (damage > 20) damage = 20;
								sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %dd%d dur %d+1d%d +%d AC",
								color, 'a' + i, r_types[i].name, sdiff, cost, fail, dx, dy, duration/2, duration/2, damage);
						break; }

					}
				} else {
					sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d",
					color, 'a' + i, "glyph", sdiff, cost, fail, rspell_damage(&dx, &dy, imperative, flags_to_type(T_BALL), skill, projection));
				}
			break; }

			case T_BALL: { //Swarm
				if (r_imperatives[imperative].flag != I_ENHA) {
					sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d rad %d",
					color, 'a' + i, r_types[i].name, sdiff, cost, fail, damage, radius);
				} else {
					sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d or %d (x%d)",
					color, 'a' + i, "swarm", sdiff, cost, fail, rspell_damage(&dx, &dy, imperative, flags_to_type(T_BALL), skill, projection)/2, rspell_damage(&dx, &dy, flags_to_imperative(I_MINI), flags_to_type(T_BALL), skill, projection), (((3+(sdiff-1)/10) > 3) ? (3+(sdiff-1)/10) : 3));
				}
			break; }

			case T_WAVE: { //Surge
				if (r_imperatives[imperative].flag != I_ENHA) {
					sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d (x3) rad %d",
					color, 'a' + i, r_types[i].name, sdiff, cost, fail, rget_level(damage), radius);
				} else {
					sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d",
					color, 'a' + i, "surge", sdiff, cost, fail, damage*2);
				}
			break; }

			case T_SIGL: { //Boon
				if (!has_rune && color != 'D') color = 'R';
				if (r_imperatives[imperative].flag != I_ENHA) {
					sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% %s resistance",
					color, 'a' + i, r_types[i].name, sdiff, cost, fail, r_projections[projection].name);
				} else {
					sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% miscellaneous boni",
					color, 'a' + i, "boon", sdiff, cost, fail);
				}
			break; }

			case T_BURS: { //Flare
				if (r_imperatives[imperative].flag != I_ENHA) {
					sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d rad %d",
					color, 'a' + i, r_types[i].name, sdiff, cost, fail, damage, radius);
				} else {
					sprintf(tmpbuf, "\377%c%c) %-7s %5d %4d %3d%% dam %d (33%% backlash)",
					color, 'a' + i, "flare", sdiff, cost, fail, dx*dy);
				}
			break; }

		}

		/* Print the line */
		prt("", j, col);
		put_str(tmpbuf, j++, col);
	}

	/* Clear the bottom line */
	prt("", j, col);
}

static bool rcraft_get_type(u16b e_flags, u16b m_flags, u16b *tempflag) {
	int i = 0;
	bool done = FALSE, redraw = FALSE;
	char choice;
	char out_val[160];

	/* Assume cancelled */
	*tempflag = RCRAFT_MAX_TYPES + 1;

	/* Build a prompt (accept all techniques) */
	if (RCRAFT_MAX_TYPES)
		strnfmt(out_val, 78, "(Form %c-%c, *=List, ESC=exit) Which spell type? ", I2A(0), I2A(RCRAFT_MAX_TYPES - 1));
	else
		strnfmt(out_val, 78, "No options available - ESC=exit");

	if (c_cfg.always_show_lists) {
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of techniques */
		rcraft_print_types(e_flags, m_flags);
	}

	/* Get a technique from the user */
	while (!done && get_com(out_val, &choice)) {
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?')) {
			/* Show the list */
			if (!redraw) {
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of techniques */
				rcraft_print_types(e_flags, m_flags);
			}
			/* Hide the list */
			else {
				/* Hide list */
				redraw = FALSE;

				/* Restore the screen */
				Term_load();

				/* Flush any events */
				Flush_queue();
			}

			/* Ask again */
			continue;
		}

		/* Extract choice */
		i = (islower(choice) ? A2I(choice) : RCRAFT_MAX_TYPES);
		if (i < 0) return FALSE;

		/* Totally illegal */
		if (i > RCRAFT_MAX_TYPES) {
			bell();
			continue;
		}

		/* Stop the loop */
		done = TRUE;
	}

	/* Restore the screen */
	if (redraw) {
		Term_load();

		/* Flush any events */
		Flush_queue();
	}


	/* Abort if needed */
	if (!done) return (FALSE);

	/* Save the choice */
	if (i >= 0 && i < RCRAFT_MAX_TYPES) *tempflag = r_types[i].flag;
	else {
		*tempflag = 0;
		return FALSE;
	}

	/* Success */
	return (TRUE);
}

static void rcraft_print_imperatives(u16b e_flags, u16b m_flags) {
	int col = 13, j = 2, i, color;
	char tmpbuf[80];

	/* Print the header */
	prt("", 1, col);
	put_str("   Imperative Level Cost Fail Damage Radius Duration Energy", 1, col);

	/* Analyze parameters */
	byte element[RCRAFT_MAX_ELEMENTS];
	byte elements = flags_to_elements(element, e_flags);
	/* Rune check for the restricted spells; enhanced, sign, glyph, sigil, boon. - Kurzel */
	byte projection = flags_to_projection(e_flags);
	bool has_rune = 0;
	for (i = 0; i < INVEN_PACK; i++) {
		if (inventory[i].tval == TV_RUNE && inventory[i].sval == projection) has_rune = 1;
	}
	byte skill = rspell_skill(element, elements);

	/* Fill the list */
	for (i = 0; i < RCRAFT_MAX_IMPERATIVES; i++) {

		/* Get the line color */
		if (r_imperatives[i].level+4 < skill) color = (r_imperatives[i].flag == I_ENHA && !has_rune) ? 'R' : 'G'; //Ew, hardcode the 1st spell type level-1 - Kurzel
		else color = 'D';

		/* Fill a line */
		sprintf(tmpbuf, "\377%c%c) %-10s %s%d %3d%% %s%d%% %5d%% %5s%d %7d%% %5d%%",
			color, 'a' + i, r_imperatives[i].name,
			ABS(r_imperatives[i].level) >= 10 ? (r_imperatives[i].level >= 0 ? "  +" : "  ") : (r_imperatives[i].level >= 0 ? "   +" : "   "),
			r_imperatives[i].level, r_imperatives[i].cost * 10,
			ABS(r_imperatives[i].fail) >= 10 ? (r_imperatives[i].fail >= 0 ? "+" : "") : (r_imperatives[i].fail >= 0 ? " +" : " "),
			r_imperatives[i].fail, r_imperatives[i].damage * 10, r_imperatives[i].radius >= 0 ? "+" : "-", ABS(r_imperatives[i].radius),
			r_imperatives[i].duration * 10, r_imperatives[i].energy * 10);

		/* Print the line */
		prt("", j, col);
		put_str(tmpbuf, j++, col);
	}

	/* Clear the bottom line */
	prt("", j, col);
}

static bool rcraft_get_imperative(u16b e_flags, u16b m_flags, u16b *tempflag) {
	int i = 0;
	bool done = FALSE, redraw = FALSE;
	char choice;
	char out_val[160];

	/* Assume cancelled */
	*tempflag = RCRAFT_MAX_IMPERATIVES + 1;

	/* Build a prompt (accept all techniques) */
	if (RCRAFT_MAX_IMPERATIVES)
		strnfmt(out_val, 78, "(Imperative %c-%c, *=List, ESC=exit) Which spell modifier? ", I2A(0), I2A(RCRAFT_MAX_IMPERATIVES - 1));
	else
		strnfmt(out_val, 78, "No options available - ESC=exit");

	if (c_cfg.always_show_lists) {
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of techniques */
		rcraft_print_imperatives(e_flags, m_flags);
	}

	/* Get a technique from the user */
	while (!done && get_com(out_val, &choice)) {
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?')) {
			/* Show the list */
			if (!redraw) {
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of techniques */
				rcraft_print_imperatives(e_flags, m_flags);
			}
			/* Hide the list */
			else {
				/* Hide list */
				redraw = FALSE;

				/* Restore the screen */
				Term_load();

				/* Flush any events */
				Flush_queue();
			}

			/* Ask again */
			continue;
		}

		/* Extract choice */
		i = (islower(choice) ? A2I(choice) : RCRAFT_MAX_IMPERATIVES);
		if (i < 0) return FALSE;

		/* Totally illegal */
		if (i > RCRAFT_MAX_IMPERATIVES) {
			bell();
			continue;
		}

		/* Stop the loop */
		done = TRUE;
	}

	/* Restore the screen */
	if (redraw) {
		Term_load();

		/* Flush any events */
		Flush_queue();
	}


	/* Abort if needed */
	if (!done) return (FALSE);

	/* Save the choice */
	if (i >= 0 && i < RCRAFT_MAX_IMPERATIVES) *tempflag = r_imperatives[i].flag;
	else {
		*tempflag = 0;
		return FALSE;
	}

	/* Success */
	return (TRUE);
}

static void rcraft_print_elements(u16b e_flags) {
	int col = 13, j = 2, i;
	char tmpbuf[80];

	/* Print the header */
	prt("", 1, col);
	put_str("   Element", 1, col);

	/* Print the list */
	for (i = 0; i < RCRAFT_MAX_ELEMENTS; i++) {
		if ((e_flags & r_elements[i].flag) != r_elements[i].flag) {
			/* Fill a line */
			sprintf(tmpbuf, "\377%c%c) %-11s",
				'G',
				'a' + i,
				r_elements[i].name);
			/* Print the line */
			prt("", j, col);
			put_str(tmpbuf, j++, col);
		} else {
			/* Fill a line */
			sprintf(tmpbuf, "\377%c%c) %-11s",
				'U',
				'a' + i,
				r_elements[i].name);
			/* Print the line */
			prt("", j, col);
			put_str(tmpbuf, j++, col);
		}
	}

	/* Print an instruction */
	// sprintf(tmpbuf, "Select up to %d elements.", RSPELL_MAX_ELEMENTS);
	// prt("", j, col);
	// put_str(tmpbuf, j++, col);

	/* Clear the bottom line */
	prt("", j, col);
}

static bool rcraft_get_element(u16b e_flags, u16b *tempflag) {
	int i = 0;
	bool done = FALSE, redraw = FALSE;
	char choice;
	char out_val[160];

	/* Assume cancelled */
	*tempflag = RCRAFT_MAX_ELEMENTS + 1;

	/* Build a prompt */
	if (RCRAFT_MAX_ELEMENTS)
		strnfmt(out_val, 78, "(Elements %c-%c, *=List, ESC=exit, ENTER=done) Which component runes? ", I2A(0), I2A(RCRAFT_MAX_ELEMENTS - 1));
	else
		strnfmt(out_val, 78, "No options available - ESC=exit");

	if (c_cfg.always_show_lists) {
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of techniques */
		rcraft_print_elements(e_flags);
	}

	/* Get a technique from the user */
	while (!done && get_com(out_val, &choice)) {

		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?')) {
			/* Show the list */
			if (!redraw) {
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of techniques */
				rcraft_print_elements(e_flags);
			}
			/* Hide the list */
			else {
				/* Hide list */
				redraw = FALSE;

				/* Restore the screen */
				Term_load();

				/* Flush any events */
				Flush_queue();
			}

			/* Ask again */
			continue;
		}

		if ((choice == '\n') || (choice == '\r') || (choice == '\e')|| (choice == 'q')) {
			/* Hide list */
			redraw = FALSE;

			/* Restore the screen */
			Term_load();

			/* Flush any events */
			Flush_queue();

			return FALSE;
		}

		/* Extract choice */
		i = (islower(choice) ? A2I(choice) : (RCRAFT_MAX_ELEMENTS + 1));
		if (i < 0) return FALSE;

		/* Totally illegal */
		if (i > RCRAFT_MAX_ELEMENTS) {
			bell();
			continue;
		}

		/* Stop the loop */
		done = TRUE;
	}

	/* Restore the screen */
	if (redraw) {
		Term_load();

		/* Flush any events */
		Flush_queue();
	}


	/* Abort if needed */
	if (!done) return (FALSE);

	/* Save the choice */
	if (i >= 0 && i < RCRAFT_MAX_ELEMENTS) *tempflag = r_elements[i].flag;
	else {
		*tempflag = 0;
		return FALSE;
	}

	/* Success */
	return (TRUE);
}

void do_runespell() {
	Term_load();
	u16b e_flags = 0, m_flags = 0, tempflag = 0;
	int dir = 0, item = 0;

	/* Request the Elements */
	byte i;
	for(i = 0; i < RSPELL_MAX_ELEMENTS; i++) {
		if(rcraft_get_element(e_flags, &tempflag)) {
			e_flags |= tempflag;
		}
		else break;
	}
	if(!e_flags) { Term_load(); return; }

	/* Request the Modifiers */
	if(rcraft_get_imperative(e_flags, m_flags, &tempflag)) m_flags |= tempflag;
	else { Term_load(); return; }
	if(rcraft_get_type(e_flags, m_flags, &tempflag)) m_flags |= tempflag;
	else { Term_load(); return; }

	/* Request the Direction -- Hardcoded - Kurzel */
	byte projection = flags_to_projection(e_flags);
	if (((m_flags & T_BOLT) == T_BOLT)
	 || (((m_flags & T_CLOU) == T_CLOU) && !((m_flags & I_ENHA) == I_ENHA)) //Until (if) Storm can be targeted! - Kurzel
	 || (((m_flags & T_SIGN) == T_SIGN) && (((projection == SV_R_NETH) && ((m_flags & I_ENHA) != I_ENHA))))
	 || (((m_flags & T_SIGN) == T_SIGN) && (((projection == SV_R_SHAR) && ((m_flags & I_ENHA) != I_ENHA))))
	 || ((m_flags & T_BALL) == T_BALL)
	 || ((m_flags & T_BURS) == T_BURS))
		if (!get_dir(&dir)) return;

	/* Request the Item */
	if ((m_flags & T_SIGL) == T_SIGL) {
		item_tester_hook = item_tester_hook_sigil;
		if (!c_get_item(&item, "Place a rune on which item? ", (USE_EQUIP))) return;
	}

	Term_load();
	Send_activate_skill(MKEY_RCRAFT, (int)e_flags, (int)m_flags, (int)dir, item, 0);
}

void do_breath() {
	int dir = 0;
	if (!get_dir(&dir)) return;
	Send_activate_skill(MKEY_BREATH, 0, 0, dir, 0, 0);
}
