/* $Id$ */
/* Client-side spell stuff */

#include "angband.h"

/* NULL for ghost - FIXME */
//static void print_spells(int book)
static void print_spells(object_type *o_ptr)
{
	int	i, col, realm, num;
//	object_type *o_ptr = &inventory[book];

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
	int	i, col, j = 2;
	char buf[90];

	/* Print column */
	col = 20;

	/* Title the list */
	prt("", 1, col);
	put_str("Name", 1, col + 5);

	prt("", j, col);
	put_str("a) Polymorph Self", j++, col);

	/* Dump the spells */
	for (i = 0; i < 32; i++)
	{
		/* Check for end of the book */
	        if (!(p_ptr->innate_spells[0] & (1L << i)))
		  continue;

		/* Dump the info */
		sprintf(buf, "%c) %s", I2A(j - 2), monster_spells4[i]);
		prt(buf, j++, col);
	}
	for (i = 0; i < 32; i++)
	{
		/* Check for end of the book */
	        if (!(p_ptr->innate_spells[1] & (1L << i)))
			continue;

 		/* Dump the info */
		sprintf(buf, "%c) %s", I2A(j - 2), monster_spells5[i]);
		prt(buf, j++, col);
	}
	for (i = 0; i < 32; i++)
	{
		/* Check for end of the book */
	        if (!(p_ptr->innate_spells[2] & (1L << i)))
			continue;

		/* Dump the info */
		sprintf(buf, "%c) %s", I2A(j - 2), monster_spells6[i]);
		prt(buf, j++, col);
	}

	/* Clear the bottom line */
	prt("", j++, col);
}

/*
 * Allow user to choose a spell/prayer from the given book.
 */

/* modified to accept certain capital letters for priest spells. -AD- */ 
 
int get_spell(int *sn, cptr prompt, int book, bool known)
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
//		if (spell_info[realm][book][i])
		/* quite a hack.. FIXME */
//		if (!spell_info[realm][book][i][0])
//		if (!strstr(spell_info[realm][book][i],"unknown"))
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
	strnfmt(out_val, 78, "(%^ss %c-%c, *=List, ESC=exit) %^s which %s? ",
		p, I2A(0), I2A(num - 1), prompt, p);

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
//				print_spells(book);
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
//void show_browse(int book)
void show_browse(object_type *o_ptr)
{
	/* Save the screen */
	Term_save();

	/* Display the spells */
//	print_spells(book);
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

#if 0
/*
 * Study a book to gain a new spell/prayer
 */
void do_study(int book)
{
	int j;
	cptr p = "spell";

	/* Priest -- Learn random spell */
	if (!strcmp(p, "prayer"))
	{
		j = -1;
	}
	/* Mage -- Learn a selected spell */
	else
	{
		/* Ask for a spell, allow cancel */
		if (!get_spell(&j, "study", book, FALSE)) return;
	}

	/* Tell the server */
	/* Note that if we are a priest, the server ignores the spell parameter */
	Send_gain(book, j);
}

/*
 * Cast a spell
 */
void do_cast(int book)
{
	int j;

	/* Ask for a spell, allow cancel */
	if (!get_spell(&j, "cast", book, FALSE)) return;

	/* Tell the server */
	Send_cast(book, j);
}

/*
 * Pray a spell
 */
void do_pray(int book)
{
	int j;

	/* Ask for a spell, allow cancel */
	if (!get_spell(&j, "pray", book, FALSE)) return;

	/* Tell the server */
	Send_pray(book, j);
}
#endif	// 0

static int get_mimic_spell(int *sn)
{
	int		i, num = 1;
	bool		flag, redraw;
	char		choice;
	char		out_val[160];
	int             corresp[200];

	/* Assume no spells available */
	(*sn) = -2;

	/* Init the Polymorph power */
	corresp[0] = 0;

	/* Check for "okay" spells */
	for (i = 0; i < 32; i++)
	{
		/* Look for "okay" spells */
		if (p_ptr->innate_spells[0] & (1L << i)) 
		{
		  corresp[num] = i + 1;
		  num++;
		}
	}
	for (i = 0; i < 32; i++)
	{
		/* Look for "okay" spells */
		if (p_ptr->innate_spells[1] & (1L << i)) 
		{
		  corresp[num] = i + 32 + 1;
		  num++;
		}
	}
	for (i = 0; i < 32; i++)
	{
		/* Look for "okay" spells */
		if (p_ptr->innate_spells[2] & (1L << i)) 
		{
		  corresp[num] = i + 64 + 1;
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
		if (!get_check(out_val)) return;
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
  int spell;

  /* Ask for the spell */
  if(!get_mimic_spell(&spell)) return;

  /* Tell the server */
//  Send_mimic(spell);
  Send_activate_skill(MKEY_MIMICRY, 0, spell, 0);
}


#if 0
/*
 * Use a technic
 */
void do_fight(int book)
{
	int j;

	/* Ask for a spell, allow cancel */
	if (!get_spell(&j, "use", book, FALSE)) return;

	/* Tell the server */
	Send_fight(book, j);
}
#endif	// 0

/*
 * Use a ghost ability
 */
void do_ghost(void)
{
	int j;

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
			sprintf(buf2, "return spell_in_book(%d, %d)", o_ptr->sval, spell);
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
s32b get_school_spell(cptr do_what)
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

//        hack_force_spell = -1;
// DGDGDG -- someone fix it please        get_item_extra_hook = get_item_hook_find_spell;
        item_tester_tval = TV_BOOK;
	sprintf(buf2, "You have no book to %s from", do_what);
#if 0	/* not sure if this is what you want ;/ */
        if (!get_item(&item, format("%^s from which book?", do_what), buf2, USE_INVEN | USE_EQUIP )) return -1;
#else
        if (!c_get_item(&item, format("%^s from which book?", do_what), TRUE, TRUE, FALSE )) return -1;
#endif

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

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
/* DGDGDG        else
        {
                sval = 255;
                pval = o_ptr->pval2;
        }*/

//        if (hack_force_spell == -1)
        {
                num = exec_lua(0, format("return book_spells_num(%d)", sval));

                /* Build a prompt (accept all spells) */
                strnfmt(out_val, 78, "(Spells %c-%c, Descs %c-%c, *=List, ESC=exit) %^s which spell? ",
                        I2A(0), I2A(num - 1), I2A(0) - 'a' + 'A', I2A(num - 1) - 'a' + 'A',  do_what);

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
                                        where = exec_lua(0, format("return print_book(%d, %d)", sval, pval));
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
                                /* Rstore the screen */
                                else
                                {
                                        /* Restore the screen */
                                        Term_load();
                                }

                                /* Display a list of spells */
                                where = exec_lua(0, format("return print_book(0, %d, %d)", sval, pval));
                                exec_lua(0, format("print_spell_desc(spell_x(%d, %d, %d), %d)", sval, pval, i, where));
                        }
                        else
                        {
                                /* Save the spell index */
                                spell = exec_lua(0, format("return spell_x(%d, %d, %d)", sval, pval, i));

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
//        repeat_push(tmp);
	printf("spell: %d\n",spell);
	return(spell);
}

void browse_school_spell(int book, int pval)
{
	int i;
	int num = 0, where = 1;
	int ask;
	char choice;
	char out_val[160];

#if 0
	/* Show choices */
	if (show_choices)
	{
		/* Update */
		p_ptr->window |= (PW_SPELL);

		/* Window stuff */
		window_stuff();
	}
#endif	// 0

        num = exec_lua(0, format("return book_spells_num(%d)", book));

	/* Build a prompt (accept all spells) */
	strnfmt(out_val, 78, "(Spells %c-%c, ESC=exit) cast which spell? ",
	        I2A(0), I2A(num - 1));

	/* Save the screen */
//	character_icky = TRUE;
	Term_save();

	/* Display a list of spells */
	where = exec_lua(0, format("return print_book(0, %d, %d)", book, pval));

	/* Get a spell from the user */
	while (get_com(out_val, &choice))
	{
		/* Display a list of spells */
		where = exec_lua(0, format("return print_book(0, %d, %d)", book, pval));

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
                where = exec_lua(0, format("return print_book(0, %d, %d)", book, pval));
                exec_lua(0, format("print_spell_desc(spell_x(%d, %d, %d), %d)", book, pval, i, where));
	}


	/* Restore the screen */
	Term_load();
//	character_icky = FALSE;

#if 0
	/* Show choices */
	if (show_choices)
	{
		/* Update */
		p_ptr->window |= (PW_SPELL);

		/* Window stuff */
		window_stuff();
	}
#endif	// 0
}

