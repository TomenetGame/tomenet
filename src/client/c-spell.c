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
	put_str("b) Polymorph Self into next known form with same extremities", j++, col);

	prt("", j, col);
	put_str("c) Polymorph Self into..", j++, col);

	/* Dump the spells */
	for (i = 0; i < 32; i++)
	{
		/* Check for end of the book */
	        if (!(p_ptr->innate_spells[0] & (1L << i)))
		  continue;

		/* Dump the info */
		sprintf(buf, "%c) %s", I2A(j - 2 + k * 17), monster_spells4[i]);
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
		sprintf(buf, "%c) %s", I2A(j - 2 + k * 17), monster_spells5[i]);
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
		sprintf(buf, "%c) %s", I2A(j - 2 + k * 17), monster_spells6[i]);
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
	strnfmt(out_val, 78, "(Powers %c-%c, *=List, ESC=exit) use which power? ",
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
  int spell, j;
  char out_val[6];

  /* Ask for the spell */
  if(!get_mimic_spell(&spell)) return;

  /* later on maybe this can moved to server side, then no need for '20000 hack'.
  Btw, 30000, the more logical one, doesnt work, dont ask me why */
  if(spell == 2){
    strcpy(out_val,"");
    get_string("Which form (0 for player) ? ", out_val, 4);
    if(strlen(out_val) == 0) return;
    j = atoi(out_val);

    if((j < 0) || (j > 2767)) return;
    spell = 20000 + j;
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
/* Backported from ToME -- beware, this can explode!	- Jir - */

#if 0
/*
 * Find a spell in any books/objects
 */
static int hack_force_spell = -1;
bool get_item_hook_find_spell(int *item)
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

		/* Must we wield it ? */
		if ((wield_slot(o_ptr) != -1) && (i < INVEN_WIELD)) continue;

		/* Is it a non-book? */
		if ((o_ptr->tval != TV_BOOK))
		{
			u32b f1, f2, f3, f4, f5, esp;

			/* Extract object flags */
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

			if ((f5 & TR5_SPELL_CONTAIN) && (o_ptr->pval2 == spell))
			{
				*item = i;
				hack_force_spell = spell;
				return TRUE;
			}
		}
		/* A random book ? */
		else if ((o_ptr->sval == 255) && (o_ptr->pval == spell))
		{
			*item = i;
			hack_force_spell = spell;
			return TRUE;
		}
		/* A normal book */
		else if (o_ptr->sval != 255)
		{
			sprintf(buf2, "return spell_in_book2(%d, %d)", o_ptr->sval, spell);
			if (exec_lua(0, buf2))
			{
				*item = i;
				hack_force_spell = spell;
				return TRUE;
			}
		}
	}
	return FALSE;
}
#endif

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

	if (*item_book < 0)
	{
		//        hack_force_spell = -1;
		// DGDGDG -- someone fix it please        get_item_extra_hook = get_item_hook_find_spell;
		item_tester_tval = TV_BOOK;
		sprintf(buf2, "You have no book to %s from", do_what);
		if (!c_get_item(&item, format("%^s from which book?", do_what), TRUE, TRUE, FALSE )) return -1;
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

#if 0 //someome fix ;)
	/* If it can be wielded, it must */
	if ((wield_slot(o_ptr) != -1) && (item < INVEN_WIELD))
	{
		msg_format("You cannot %s from that object, it must be wielded first.", do_what);
		return -1;
	}

	if (repeat_pull(&tmp))
	{
		return tmp;
	}
#endif
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
	/* DGDGDG        else
	   {
	   sval = 255;
	   pval = o_ptr->pval2;
	   }*/

	//        if (hack_force_spell == -1)
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
#if 0 /* would leave garbage behind - mikaelh */
				/* Restore the screen */
				else
				{
					/* Restore the screen */
					Term_load();
				}
#endif

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
#if 0 // DGDGDGDG
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
			msg_format("You may not %s that spell.", do_what);
			spell = -1;
		}
	}
#endif

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
		strnfmt(out_val, 78, "(Techniques %c-%c, *=List, ESC=exit) use which technique? ",
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
		strnfmt(out_val, 78, "(Techniques %c-%c, *=List, ESC=exit) use which technique? ",
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
