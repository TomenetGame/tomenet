/* $Id$ */
/* Client-side spell stuff */

#include "angband.h"

/* NULL for ghost - FIXME */
static void print_spells(object_type *o_ptr)
{
	int	i, col, realm, num;

	/* Hack -- handle ghosts right */
	if (p_ptr->ghost)
	{
		realm = REALM_GHOST;
		num = 0;
	}
	else
	{
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
	for (i = 0; i < 9; i++)
	{
		/* Check for end of the book */
		if (spell_info[realm][num][i][0] == '\0')
			break;

		/* Dump the info */
		prt(spell_info[realm][num][i], 2 + i, col);
	}

	/* Clear the bottom line */
	prt("", 2 + i, col);
}

static void print_mimic_spells()
{
	int	i, col, j = 2, k;
	char buf[90];

	/* Print column */
	col = 15;
	k = 0; /* next column? for forms with LOTS of spells (GWoP) */

	/* Title the list */
	prt("", 1, col);
	put_str("Name", 1, col + 5);

	prt("", j, col);
	put_str("a) Polymorph Self into next known form", j++, col);

	prt("", j, col);
	put_str("b) Polymorph Self into next known form with fitting extremities", j++, col);

	prt("", j, col);
	put_str("c) Polymorph Self into..", j++, col);

	/* Dump the spells */
	for (i = 0; i < 32; i++)
	{
		/* Check for end of the book */
	        if (!(p_ptr->innate_spells[0] & (1L << i)))
		  continue;

		/* Dump the info */
		sprintf(buf, "%c) %s", I2A(j - 2 + k * 17), monster_spells4[i].name);
		prt(buf, j++, col + k * 35);

		/* check for beginning of 2nd column */
		if (j > 21) {
			j = 5;
			k = 1;
		}
	}
	for (i = 0; i < 32; i++)
	{
		/* Check for end of the book */
	        if (!(p_ptr->innate_spells[1] & (1L << i)))
			continue;

 		/* Dump the info */
		sprintf(buf, "%c) %s", I2A(j - 2 + k * 17), monster_spells5[i].name);
		prt(buf, j++, col + k * 35);

		/* check for beginning of 2nd column */
		if (j > 21) {
			j = 5;
			k = 1;
		}
	}
	for (i = 0; i < 32; i++)
	{
		/* Check for end of the book */
	        if (!(p_ptr->innate_spells[2] & (1L << i)))
			continue;

		/* Dump the info */
		sprintf(buf, "%c) %s", I2A(j - 2 + k * 17), monster_spells6[i].name);
		prt(buf, j++, col + k * 35);

		/* check for beginning of 2nd column */
		if (j > 21) {
			j = 5;
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

int get_spell(s32b *sn, cptr prompt, int book, bool known)
{
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

	if (p_ptr->ghost)
	{
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
	for (i = 0; i < 9; i++)
	{
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

	if (c_cfg.always_show_lists)
	{
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of spells */
		print_spells(o_ptr);
	}

	/* Get a spell from the user */
	while (!flag && get_com(out_val, &choice))
	{
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?'))
		{
			/* Show the list */
			if (!redraw)
			{
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of spells */
				print_spells(o_ptr);
			}

			/* Hide the list */
			else
			{
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
                if (islower(choice))
                {
                        i = A2I(choice);
                        if (i >= num) i = -1;
                }

                /* uppercase... hope this is portable. */
                else if (isupper(choice))
                {
                        i = (choice - 'A') + 64;
                        if (i-64 >= num) i = -1;
                }
                else i = -1;

		/* Totally Illegal */
		if (i < 0)
		{
			bell();
			continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw)
	{
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
void show_browse(object_type *o_ptr)
{
	/* Save the screen */
	Term_save();

	/* Display the spells */
	print_spells(o_ptr);

	/* Clear the top line */
	prt("", 0, 0);

	/* Prompt user */
	put_str("[Press any key to continue]", 0, 23);

	/* Wait for key */
	(void)inkey();

	/* Restore the screen */
	Term_load();

	/* Flush any events */
	Flush_queue();
}

static int get_mimic_spell(int *sn)
{
	int		i, num = 3; /* 3 polymorph self spells */
	bool		flag, redraw;
	char		choice;
	char		out_val[160];
	int             corresp[200];

	/* Assume no spells available */
	(*sn) = -2;

	/* Init the Polymorph power */
	corresp[0] = 0;
	/* Init the Polymorph into fitting */
	corresp[1] = 1;
	/* Init the Polymorph into.. <#> power */
	corresp[2] = 2;

	/* Check for "okay" spells */
	for (i = 0; i < 32; i++)
	{
		/* Look for "okay" spells */
		if (p_ptr->innate_spells[0] & (1L << i))
		{
		  corresp[num] = i + 3;
		  num++;
		}
	}
	for (i = 0; i < 32; i++)
	{
		/* Look for "okay" spells */
		if (p_ptr->innate_spells[1] & (1L << i))
		{
		  corresp[num] = i + 32 + 3;
		  num++;
		}
	}
	for (i = 0; i < 32; i++)
	{
		/* Look for "okay" spells */
		if (p_ptr->innate_spells[2] & (1L << i))
		{
		  corresp[num] = i + 64 + 3;
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

	if (c_cfg.always_show_lists)
	{
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of spells */
		print_mimic_spells();
	}

	/* Get a spell from the user */
	while (!flag && get_com(out_val, &choice))
	{
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?'))
		{
			/* Show the list */
			if (!redraw)
			{
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of spells */
				print_mimic_spells();
			}

			/* Hide the list */
			else
			{
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
		else if (choice == '@')
		{
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
				if (c >= 64 + 3) {
					if (!strcmp(buf, monster_spells6[c - 64 - 3].name)) {
						flag = TRUE;
						break;
					}
				} else if (c >= 32 + 3) {
					if (!strcmp(buf, monster_spells5[c - 32 - 3].name)) {
						flag = TRUE;
						break;
					}
				} else if (c >= 3) { /* checkin for >=3 is paranoia */
					if (!strcmp(buf, monster_spells4[c - 3].name)) {
						flag = TRUE;
						break;
					}
				}
			}
			if (flag) break;

			/* illegal input */
			bell();
			continue;
		}
		else
		{
			/* extract request */
			i = (islower(choice) ? A2I(choice) : -1);
			if (i >= num) i = -1;
		}

		/* Totally Illegal */
		if (i < 0)
		{
			bell();
			continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw)
	{
		Term_load();

		/* Flush any events */
		Flush_queue();
	}


	/* Abort if needed */
	if (!flag) return (FALSE);

	if (c_cfg.other_query_flag && !i)
	{
		sprintf(out_val, "Really change the form? ");
		if (!get_check(out_val)) return(FALSE);
	}

	/* Save the choice */
	(*sn) = corresp[i];

	/* Success */
	return (TRUE);
}


/*
 * Mimic
 */
void do_mimic()
{
	int spell, j, dir;
	char out_val[6];
	bool uses_dir = FALSE;

	/* Ask for the spell */
	if (!get_mimic_spell(&spell)) return;

	/* later on maybe this can moved to server side, then no need for '20000 hack'.
	   Btw, 30000, the more logical one, doesnt work, dont ask me why */
	if (spell == 2) {
		strcpy(out_val,"");
		get_string("Which form (0 for player) ? ", out_val, 4);
		if (strlen(out_val) == 0) return;
		j = atoi(out_val);
		if ((j < 0) || (j > 2767)) return;
		spell = 20000 + j;
	}

	/* Spell requires direction? */
	else if (spell > 2 && is_newer_than(&server_version, 4, 4, 5, 10, 0, 0)) {
		j = spell - 3;
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
void do_ghost(void)
{
	s32b j;

	/* Ask for an ability, allow cancel */
	if (!get_spell(&j, "use", 0, FALSE)) return;

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
bool get_item_hook_find_spell(int *item, bool inven_first)
{
	int i, spell;
	char buf[80];
	char buf2[100];

	strcpy(buf, "Manathrust");
	if (!get_string("Spell name? ", buf, 79))
		return FALSE;
	sprintf(buf2, "return find_spell(\"%s\")", buf);
	spell = exec_lua(0, buf2);
	if (spell == -1) return FALSE;

	for (i = 0; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &inventory[i];

		if (o_ptr->tval == TV_BOOK) {
			/* A random book ? */
			if ((o_ptr->sval == 255) && (o_ptr->pval == spell))
			{
				*item = i;
				hack_force_spell = spell;
				return TRUE;
			}
			/* A normal book */
			else
			{
				sprintf(buf2, "return spell_in_book2(%d, %d, %d)", i, o_ptr->sval, spell);
				if (exec_lua(0, buf2))
				{
					*item = i;
					hack_force_spell = spell;
					return TRUE;
				}
			}
		}
		
	}

	return FALSE;
}

/*
 * Get a spell from a book
 */
s32b get_school_spell(cptr do_what, int *item_book)
{
	int i, item;
	u32b spell = -1;
	int num = 0, where = 1;
	int ask;
	bool flag, redraw;
	char choice;
	char out_val[160];
	char buf2[40];
	object_type *o_ptr;
	int tmp;
	int sval, pval;

	hack_force_spell = -1;

	if (*item_book < 0)
	{
		get_item_extra_hook = get_item_hook_find_spell;
		item_tester_tval = TV_BOOK;
		sprintf(buf2, "You have no book to %s from.", do_what);
		if (!c_get_item(&item, format("%^s from which book?", do_what), (USE_INVEN | USE_EXTRA) )) {
			if (item == -2) c_msg_format("%s", buf2);
			return -1;
		}
	}
	else
	{
		/* Already given */
		item = *item_book;
	}

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}
	else
		return(-1);

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	/* No spell to cast by default */
	spell = -1;

	/* Is it a random book, or something else ? */
	if (o_ptr->tval == TV_BOOK)
	{
		sval = o_ptr->sval;
		pval = o_ptr->pval;
	}
	else
	{
		/* Only books allowed */
		return -1;
	}

	if (hack_force_spell == -1)
	{
		num = exec_lua(0, format("return book_spells_num2(%d, %d)", item, sval));

		/* Build a prompt (accept all spells) */
		if (num)
			strnfmt(out_val, 78, "(Spells %c-%c, Descs %c-%c, *=List, ESC=exit) %^s which spell? ",
			    I2A(0), I2A(num - 1), I2A(0) - 'a' + 'A', I2A(num - 1) - 'a' + 'A',  do_what);
		else
			strnfmt(out_val, 78, "No spells available - ESC=exit");

		if (c_cfg.always_show_lists)
		{
			/* Show list */
			redraw = TRUE;

			/* Save the screen */
			Term_save();

			/* Display a list of spells */
			where = exec_lua(0, format("return print_book2(0, %d, %d, %d)", item, sval, pval));
		}

		/* Get a spell from the user */
		while (!flag && get_com(out_val, &choice))
		{
			/* Request redraw */
			if (((choice == ' ') || (choice == '*') || (choice == '?')))
			{
				/* Show the list */
				if (!redraw)
				{
					/* Show list */
					redraw = TRUE;

					/* Save the screen */
					Term_save();

					/* Display a list of spells */
					where = exec_lua(0, format("return print_book2(0, %d, %d, %d)", item, sval, pval));
				}

				/* Hide the list */
				else
				{
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
			if ((i < 0) || (i >= num))
			{
				bell();
				continue;
			}

			/* Verify it */
			if (ask)
			{
				/* Show the list */
				if (!redraw)
				{
					/* Show list */
					redraw = TRUE;

					/* Save the screen */
					Term_save();

				}

				/* Display a list of spells */
				where = exec_lua(0, format("return print_book2(0, %d, %d, %d)", item, sval, pval));
				exec_lua(0, format("print_spell_desc(spell_x2(%d, %d, %d, %d), %d)", item, sval, pval, i, where));
			}
			else
			{
				/* Save the spell index */
				spell = exec_lua(0, format("return spell_x2(%d, %d, %d, %d)", item, sval, pval, i));

				/* Require "okay" spells */
				if (!exec_lua(0, format("return is_ok_spell(0, %d)", spell)))
				{
					bell();
					c_msg_format("You may not %s that spell.", do_what);
					spell = -1;
					continue;
				}

				/* Stop the loop */
				flag = TRUE;
			}
		}
	}
	else
	{
		/* Require "okay" spells */
		if (exec_lua(0, format("return is_ok_spell(0, %d)", hack_force_spell)))
		{
			flag = TRUE;
			spell = hack_force_spell;
		}
		else
		{
			bell();
			c_msg_format("You may not %s that spell.", do_what);
			spell = -1;
		}
	}

	/* Restore the screen */
	if (redraw)
	{
		Term_load();
	}


	/* Abort if needed */
	if (!flag) return -1;

	tmp = spell;
	*item_book = item;
	return(spell);
}

/* TODO: handle blindness */
void browse_school_spell(int item, int book, int pval)
{
	int i;
	int num = 0, where = 1;
	int ask;
	char choice;
	char out_val[160];
	int sval = book;

#ifdef USE_SOUND_2010
	if (sval == SV_SPELLBOOK) sound(browse_sound_idx, SFX_TYPE_COMMAND, 100, 0);
	else sound(browsebook_sound_idx, SFX_TYPE_COMMAND, 100, 0);
#endif

        num = exec_lua(0, format("return book_spells_num2(%d, %d)", item, sval));

	/* Build a prompt (accept all spells) */
	if (num)
		strnfmt(out_val, 78, "(Spells %c-%c, ESC=exit) cast which spell? ",
	    	    I2A(0), I2A(num - 1));
	else
		strnfmt(out_val, 78, "No spells available - ESC=exit");

	/* Save the screen */
	Term_save();

	/* Display a list of spells */
	where = exec_lua(0, format("return print_book2(0, %d, %d, %d)", item, sval, pval));

	/* Get a spell from the user */
	while (get_com(out_val, &choice))
	{
		/* Display a list of spells */
		where = exec_lua(0, format("return print_book2(0, %d, %d, %d)", item, sval, pval));

		/* Note verify */
		ask = (isupper(choice));

		/* Lowercase */
		if (ask) choice = tolower(choice);

		/* Extract request */
		i = (islower(choice) ? A2I(choice) : -1);

		/* Totally Illegal */
		if ((i < 0) || (i >= num))
		{
			bell();
			continue;
		}

                /* Restore the screen */
                /* Term_load(); */

                /* Display a list of spells */
                where = exec_lua(0, format("return print_book2(0, %d, %d, %d)", item, sval, pval));
                exec_lua(0, format("print_spell_desc(spell_x2(%d, %d, %d, %d), %d)", item, sval, pval, i, where));
	}


	/* Restore the screen */
	Term_load();
}

static void print_combatstances()
{
	int col = 20, j = 2;

	/* Title the list */
	prt("", 1, col);
	put_str("Name", 1, col + 5);

	prt("", j, col);
	put_str("a) Balanced stance (normal behaviour)", j++, col);

	prt("", j, col);
	put_str("b) Defensive stance", j++, col);

	prt("", j, col);
	put_str("c) Offensive stance", j++, col);

	/* Clear the bottom line */
	prt("", j++, col);
}

static int get_combatstance(int *cs)
{
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

	if (c_cfg.always_show_lists)
	{
		/* Show list */
		redraw = TRUE;
		/* Save the screen */
		Term_save();
		/* Display a list of stances */
		print_combatstances();
	}

	/* Get a stance from the user */
	while (!flag && get_com(out_val, &choice))
	{
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?'))
		{
			/* Show the list */
			if (!redraw)
			{
				/* Show list */
				redraw = TRUE;
				/* Save the screen */
				Term_save();
				/* Display a list of stances */
				print_combatstances();
			}

			/* Hide the list */
			else
			{
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
		if (i < 0)
		{
			bell();
			continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw)
	{
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
void do_stance()
{
	int stance;
	/* Ask for the stance */
	if(!get_combatstance(&stance)) return;
	Send_activate_skill(MKEY_STANCE, stance, 0, 0, 0, 0);
}


static void print_melee_techniques()
{
	int	i, col, j = 0;
	char buf[90];
	/* Print column */
	col = 20;
	/* Title the list */
	prt("", 1, col);
	put_str("Name", 1, col + 5);
	/* Dump the techniques */
	for (i = 0; i < 16; i++)
	{
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

static int get_melee_technique(int *sn)
{
	int		i, num = 0;
	bool		flag, redraw;
	char		choice;
	char		out_val[160];
	int             corresp[32];

	/* Assume no techniques available */
	(*sn) = -2;

	/* Check for accessible techniques */
	for (i = 0; i < 32; i++)
	{
		/* Look for accessible techniques */
		if (p_ptr->melee_techniques & (1L << i))
		{
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

	if (c_cfg.always_show_lists)
	{
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of techniques */
		print_melee_techniques();
	}

	/* Get a technique from the user */
	while (!flag && get_com(out_val, &choice))
	{
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?'))
		{
			/* Show the list */
			if (!redraw)
			{
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of techniques */
				print_melee_techniques();
			}

			/* Hide the list */
			else
			{
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
                    		if (!strcmp(buf, melee_techniques[corresp[i]])) {
					flag = TRUE;
					break;
				}
			}
			if (flag) break;

			/* illegal input */
			bell();
			continue;
                }
		else {
		       	/* extract request */
			i = (islower(choice) ? A2I(choice) : -1);
		      	if (i >= num) i = -1;

			/* Totally Illegal */
			if (i < 0)
			{
				bell();
				continue;
			}

			/* Stop the loop */
			flag = TRUE;
		}
	}

	/* Restore the screen */
	if (redraw)
	{
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

void do_melee_technique()
{
  int technique;

  /* Ask for the technique */
  if(!get_melee_technique(&technique)) return;

  Send_activate_skill(MKEY_MELEE, 0, technique, 0, 0, 0);
}

static void print_ranged_techniques()
{
	int	i, col, j = 0;
	char buf[90];
	/* Print column */
	col = 20;
	/* Title the list */
	prt("", 1, col);
	put_str("Name", 1, col + 5);
	/* Dump the techniques */
	for (i = 0; i < 16; i++)
	{
		/* Check for accessible technique */
	        if (!(p_ptr->ranged_techniques & (1L << i)))
		  continue;
		/* Dump the info */
		sprintf(buf, "%c) %s", I2A(j), ranged_techniques[i]);
		prt(buf, 2 + j++, col);
	}
	/* Clear the bottom line */
	prt("", 2 + j++, col);
}

static int get_ranged_technique(int *sn)
{
	int		i, num = 0;
	bool		flag, redraw;
	char		choice;
	char		out_val[160];
	int             corresp[32];

	/* Assume no techniques available */
	(*sn) = -2;

	/* Check for accessible techniques */
	for (i = 0; i < 32; i++)
	{
		/* Look for accessible techniques */
		if (p_ptr->ranged_techniques & (1L << i))
		{
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

	if (c_cfg.always_show_lists)
	{
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of techniques */
		print_ranged_techniques();
	}

	/* Get a technique from the user */
	while (!flag && get_com(out_val, &choice))
	{
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?'))
		{
			/* Show the list */
			if (!redraw)
			{
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of techniques */
				print_ranged_techniques();
			}

			/* Hide the list */
			else
			{
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
			strcpy(buf, "Flare missile"); 
                        if (!get_string("Technique? ", buf, 79)) {
                    		if (redraw) {
                    			Term_load();
                    			Flush_queue();
                    		}
                    		return FALSE;
                	}

                        /* Find the skill it is related to */
                        for (i = 0; i < num; i++) {
                    		if (!strcmp(buf, ranged_techniques[corresp[i]])) {
					flag = TRUE;
					break;
				}
			}
			if (flag) break;

			/* illegal input */
			bell();
			continue;
                }
		else {
		       	/* extract request */
			i = (islower(choice) ? A2I(choice) : -1);
	      		if (i >= num) i = -1;

			/* Totally Illegal */
			if (i < 0)
			{
				bell();
				continue;
			}

			/* Stop the loop */
			flag = TRUE;
		}
	}

	/* Restore the screen */
	if (redraw)
	{
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

void do_ranged_technique()
{
  int technique;

  /* Ask for the technique */
  if(!get_ranged_technique(&technique)) return;

  Send_activate_skill(MKEY_RANGED, 0, technique, 0, 0, 0);
}


#ifdef ENABLE_RCRAFT

static int b_compare (const void * a, const void * b)
{
	return ( *(byte*)b - *(byte*)a );
}

static byte runes_in_flag(byte runes[], u32b flags)
{
	if ((flags & R_ACID) == R_ACID) { runes[0] = 1; } else { runes[0] = 0; }
	if ((flags & R_ELEC) == R_ELEC) { runes[1] = 1; } else { runes[1] = 0; }
	if ((flags & R_FIRE) == R_FIRE) { runes[2] = 1; } else { runes[2] = 0; }
	if ((flags & R_COLD) == R_COLD) { runes[3] = 1; } else { runes[3] = 0; }
	if ((flags & R_POIS) == R_POIS) { runes[4] = 1; } else { runes[4] = 0; }
	if ((flags & R_FORC) == R_FORC) { runes[5] = 1; } else { runes[5] = 0; }
	if ((flags & R_WATE) == R_WATE) { runes[6] = 1; } else { runes[6] = 0; }
	if ((flags & R_EART) == R_EART) { runes[7] = 1; } else { runes[7] = 0; }
	if ((flags & R_CHAO) == R_CHAO) { runes[8] = 1; } else { runes[8] = 0; }
	if ((flags & R_NETH) == R_NETH) { runes[9] = 1; } else { runes[9] = 0; }
	if ((flags & R_NEXU) == R_NEXU) { runes[10] = 1; } else { runes[10] = 0; }
	if ((flags & R_TIME) == R_TIME) { runes[11] = 1; } else { runes[11] = 0; }
	
	return 0;
}

/* rspell_type

Selects a spell based on the rspell_selector[] array 
*/
static u32b rspell_type (u32b flags)
{
	u16b i;
	
	for (i = 0; i<MAX_RSPELL_SEL; i++)
	{
		if (rspell_selector[i].flags)
		{
			if ((rspell_selector[i].flags & flags) == rspell_selector[i].flags)
			{
				return rspell_selector[i].type;
			}
		}
	}
	return RT_NONE;
}

/* rspell_skill (for client level comparison in color selector) - Kurzel

Calculates a skill average value for the requested runes.

Uses a series of formulas created by Kurzel.

Assumes a max of 3 runes in a spell. Needs to be adjusted if this ever changes.
Also assumes that the rune skill flags are in order, starting with FIRECOLD.
*/
static u16b rspell_skill_client(u32b s_type) //u16b rspell_skill_client(u32b s_flags);
{
	u16b s = 0; u16b i; u16b skill = 0; u16b value = 0;
	s16b a=0,b=0,c=0;
	
	byte rune_c = 0;
	byte skill_c = 0;
	byte skills[RCRAFT_MAX_ELEMENTS/2] = {0,0,0,0,0,0};
	byte runes[3] = {0,0,0};
	
	byte s_runes[RCRAFT_MAX_ELEMENTS];
	runes_in_flag(s_runes, s_type);
	
	for (i=0; i<RCRAFT_MAX_ELEMENTS; i++)
	{
		if (s_runes[i]==1)
		{
			skill = r_elements[i].skill;
			value = get_skill(skill);
			//Ugh. We need an index from 0-7. FIRECOLD through MINDNEXU is 96-103
			//skills[skill - SKILL_R_FIRECOLD] = 1;
			skills[skill - SKILL_R_ACIDWATE] = 1;
			//ACIDWATE through FORCTIME is 96-101.
			if (rune_c > 2)
			{
				break;
			}
			
			runes[rune_c++] = value;
		}
	}
	
	for (i=0; i<RCRAFT_MAX_ELEMENTS/2; i++)
	{
		if (skills[i]==1)
		{
			skill_c++;
		}
	}
	
	if (rune_c == 1 || (rune_c == 2 && runes[0] == runes[1]) || (rune_c == 3 && (runes[0] == runes[1] && runes[1] == runes[2])))
	{
		return runes[0];
	}
	
	qsort(runes, 3, sizeof(byte), b_compare);
	
	switch (skill_c)
	{
		case 2:
			if (rune_c == 2)
			{
				a = 50 * R_CAP / 100;
				
				s = ((a*runes[0])+((100-a)*runes[1])) / 100;
			}
			else if (rune_c == 3)
			{
				if (runes[1] == runes[2])
				{
					a = 33 * R_CAP / 100;
				}
				else // if (runes[0] == runes[1])
				{
					a = 66 * R_CAP / 100;
				}
				
				s = ((a*runes[0])+((100-a)*runes[1])) / 100;
			}
			break;
		
		case 3:
			a = 33 * R_CAP / 100;
			b = 33;
			c = 100 - a - b;
			
			s = (a*runes[0] + b*runes[1] + c*runes[2]) / 100;
			break;
		
		default:
			s = runes[0];
			break;
	}
	
	return s;
}

/* Color-selector Code - Kurzel.
   Return colour that indicates difficulty of a rune spell depending on
   the choice of certain runes, imperative (via level_mod), method (via level_mod). */
char runecraft_colourize(u32b flags, int level_mod) {
	s32b s_av = 0;
	s32b e_level = 0;

	e_level = runespell_list[rspell_type(flags)].level + level_mod;
	s_av = rspell_skill_client(flags);

	/* catch if we don't know the runes and hence can't cast this spell at all */
	if (s_av <= 0) return 'D';

	if (e_level < (s_av - 10)) return 'B';
	else if (e_level < s_av) return 'w';
	else if (e_level < (s_av +  5)) return 'y';
	else if (e_level < (s_av + 15)) return 'o';
	else return 'D';
}

static void print_runes(u32b flags)
{
	int col = 10, j = 2, i;
	char tmpbuf[80];
	
	/* Title the list */
	prt("", 1, col); put_str("Element,       Rune", 1, col);

	for (i = 0; i < RCRAFT_MAX_ELEMENTS; i++) {
		if ((flags & r_elements[i].self) != r_elements[i].self) {
			sprintf(tmpbuf, "%c) \377%c%-11s\377w '%s'",
			    'a' + i,
			    runecraft_colourize(flags | r_elements[i].self, 0),
			    r_elements[i].title,
			    r_elements[i].e_syl);
			prt("", j, col);
			put_str(tmpbuf, j++, col);
		}
	}

	prt("", j++, col);
	put_str("Select the maximum of three runes, or press \"Return\" when done.", j++, col);

	/* Clear the bottom line */
	prt("", j++, col);
}

static void print_rune_imperatives(u32b flags)
{
	int col = 10, j = 2, i;
	char tmpbuf[80];

	/* Title the list */
	prt("", 1, col);
	put_str("Name            ( Lvl, Dam%, Cost%, Fail% )", 1, col);

	for (i = 0; i < RCRAFT_MAX_IMPERATIVES; i++) {

		/* catch 'chaotic' (only one that has dam or cost of zero) */
		if (!r_imperatives[i].cost) {
			sprintf(tmpbuf, "%c) \377%c%-10s\377w   (  %s%d, ???%%,  ???%%,  +??%% )",
			    'a' + i,
			    runecraft_colourize(flags, r_imperatives[i].level),
			    r_imperatives[i].name,
			    r_imperatives[i].level >= 0 ? "+" : "", r_imperatives[i].level);
		} else {
			/* normal imperatives */
			sprintf(tmpbuf, "%c) \377%c%-10s\377w   (  %s%d, %3d%%,  %3d%%,  %s%2d%% )",
			    'a' + i,
			    runecraft_colourize(flags, r_imperatives[i].level),
			    r_imperatives[i].name,
			    r_imperatives[i].level >= 0 ? "+" : "", r_imperatives[i].level,
			    r_imperatives[i].dam * 10,
			    r_imperatives[i].cost * 10,
			    r_imperatives[i].fail >= 0 ? "+" : "" , r_imperatives[i].fail);
		}

		prt("", j, col);
		put_str(tmpbuf, j++, col);
	}

	/* Clear the bottom line */
	prt("", j++, col);
}

static void print_rune_methods(u32b flags, byte *imper)
{
	int col = 10, j = 2, i;
	char tmpbuf[80];

	/* Title the list */
	prt("", 1, col);
	put_str("Name         (  Lvl,  Cost%  )", 1, col);

	for (i = 0; i < RCRAFT_MAX_TYPES; i++) {
		sprintf(tmpbuf, "%c) \377%c%-7s\377w   (  %s%d,   %3d%%  )",
		    'a' + i,
		    runecraft_colourize(flags, r_imperatives[(*imper)].level + runespell_types[i].cost),
		    runespell_types[i].title,
		    runespell_types[i].cost >= 0 ? "+" : "-", ABS(runespell_types[i].cost),
		    runespell_types[i].pen * 10);
		prt("", j, col);
		put_str(tmpbuf, j++, col);
	}

	/* Clear the bottom line */
	prt("", j++, col);
}

static int get_rune_type(u32b s_flags, u32b *sn)
{
	int		i = 0; int num = 16;
	bool		flag, redraw;
	char		choice;
	char		out_val[160];

	/* Assume cancelled */
	(*sn) = 17;

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	/* Build a prompt (accept all techniques) */
	if (num)
		strnfmt(out_val, 78, "(Runes %c-%c, *=List, ESC=exit, ENTER=done) Which runes? ", I2A(0), I2A(num-1));
	else
		strnfmt(out_val, 78, "No elements available - ESC=exit");

	if (c_cfg.always_show_lists)
	{
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of techniques */
		print_runes(s_flags);
	}

	/* Get a technique from the user */
	while (!flag && get_com(out_val, &choice))
	{
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?'))
		{
			/* Show the list */
			if (!redraw)
			{
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of techniques */
				print_runes(s_flags);
			}

			/* Hide the list */
			else
			{
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

	       	/* extract request */
		i = (islower(choice) ? A2I(choice) : 17);
	      	if (i < 0) return FALSE;

		/* Totally Illegal */
		if (i>num)
		{
			bell();
			continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw)
	{
		Term_load();

		/* Flush any events */
		Flush_queue();
	}


	/* Abort if needed */
	if (!flag) return (FALSE);

	/* Save the choice */
	if(i >= 0 && i < num)
		(*sn) = r_elements[i].self;
	else
	{
		*sn = 0;
		return FALSE;
	}

	/* Success */
	return (TRUE);
}

static int get_rune_imperative(u32b s_flags, byte *sn)
{
	int		i, num = RCRAFT_MAX_IMPERATIVES;
	bool		flag, redraw;
	char		choice;
	char		out_val[160];
	byte            corresp[RCRAFT_MAX_IMPERATIVES];

	for (i = 0; i < RCRAFT_MAX_IMPERATIVES; i++)
		corresp[i] = r_imperatives[i].id;

	/* Assume cancelled */
	(*sn) = num + 1;

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	/* Build a prompt (accept all techniques) */
	if (num)
		strnfmt(out_val, 78, "(Spell-power %c-%c, *=List, ESC=exit) What kind of effect? ",
		    I2A(0), I2A(num-1));
	else
		strnfmt(out_val, 78, "No options available - ESC=exit");

	if (c_cfg.always_show_lists)
	{
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of techniques */
		print_rune_imperatives(s_flags);
	}

	/* Get a technique from the user */
	while (!flag && get_com(out_val, &choice))
	{
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?'))
		{
			/* Show the list */
			if (!redraw)
			{
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of techniques */
				print_rune_imperatives(s_flags);
			}

			/* Hide the list */
			else
			{
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
	      	if (i >= num) i = num+1;

		/* Totally Illegal */
		if (i>num)
		{
			bell();
			continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw)
	{
		Term_load();

		/* Flush any events */
		Flush_queue();
	}


	/* Abort if needed */
	if (!flag) return (FALSE);

	/* Save the choice */
	if (i >= 0 && i <= num)
	{
		(*sn) = corresp[i];
	}
	else
		return FALSE;

	/* Success */
	return (TRUE);
}

static int get_rune_method(u32b s_flags, u32b *sn, byte *imperative, u16b *method)
{
	int		i, num = RCRAFT_MAX_TYPES;
	bool		flag, redraw;
	char		choice;
	char		out_val[160];
	byte            corresp[RCRAFT_MAX_TYPES];
	
	//Pass imperative - Kurzel
	byte imper = (*imperative);

	for (i = 0; i < RCRAFT_MAX_TYPES; i++)
		corresp[i] = runespell_types[i].type;

	/* Assume cancelled */
	(*sn) = num + 1;

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	/* Build a prompt (accept all techniques) */
	if (num)
		strnfmt(out_val, 78, "(Spell-types %c-%c, *=List, ESC=exit) Which spell type? ",
		    I2A(0), I2A(num-1));
	else
		strnfmt(out_val, 78, "No methods available - ESC=exit");

	if (c_cfg.always_show_lists)
	{
		/* Show list */
		redraw = TRUE;

		/* Save the screen */
		Term_save();

		/* Display a list of techniques */
		print_rune_methods(s_flags, &imper);
	}

	/* Get a technique from the user */
	while (!flag && get_com(out_val, &choice))
	{
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?'))
		{
			/* Show the list */
			if (!redraw)
			{
				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				/* Display a list of techniques */
				print_rune_methods(s_flags, &imper);
			}

			/* Hide the list */
			else
			{
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
	      	if (i >= num) i = num+1;

		/* Totally Illegal */
		if (i > num)
		{
			bell();
			continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw)
	{
		Term_load();

		/* Flush any events */
		Flush_queue();
	}


	/* Abort if needed */
	if (!flag) return (FALSE);

	/* Save the choice */
	if (i >= 0 && i <= num)
	{
		(*sn) = corresp[i];
		(*method) = i;
	}
	else
		return FALSE;
	/* Success */
	return (TRUE);
}

void do_runespell()
{
	Term_load();
	u32b s_flags = 0; u16b method = 0; byte imperative = 0; int dir = 0;
	u32b flag = 0; int i = 0;
	int part1 = 0; int part2 = 0;

	for(i = 0; i<3; i++)
	{
		if(get_rune_type(s_flags,&flag))
		{
			s_flags |= flag;
			flag = 0;
		}
		else
			break;
	}
	if(!s_flags) { Term_load(); return; } //Didn't set anything, user is trying to cancel

	if(!get_rune_imperative(s_flags, &imperative)){ Term_load(); return; }

	if(!get_rune_method(s_flags, &flag, &imperative, &method)) { Term_load(); return; }
	else
		s_flags |= flag;

	if(s_flags == 0) { Term_load(); return; } //Empty spell

	if(method != 1 && method != 5 && method != 7)
		if (!get_dir(&dir)) return;

	Term_load();

	/* Split the long into two ints */
	part1 = s_flags / 10000;
	part2 = s_flags % 10000;
	Send_activate_skill(MKEY_RCRAFT, part1, part2, dir, (int)imperative, 0);
}

#endif

