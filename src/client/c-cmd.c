/* $Id$ */
#include "angband.h"

static void cmd_clear_buffer(void);

void cmd_player_equip(void)
{
    /* Set the hook */
    special_line_type = SPECIAL_FILE_PLAYER_EQUIP;

    /* Call the file perusal */
    peruse_file();
}

static bool item_tester_magicable(object_type *o_ptr)
{
#if 0	// temporally hack for skill system		- Jir -
	if (((p_ptr->pclass == CLASS_MAGE) || (p_ptr->pclass == CLASS_RANGER)) && (o_ptr->tval == TV_MAGIC_BOOK)) return TRUE;

	if ((p_ptr->pclass == CLASS_SORCERER) && (o_ptr->tval == TV_SORCERY_BOOK)) return TRUE;

	if ((p_ptr->pclass == CLASS_ROGUE) && (o_ptr->tval == TV_SHADOW_BOOK)) return TRUE;

	if ((p_ptr->pclass == CLASS_ARCHER) && (o_ptr->tval == TV_HUNT_BOOK)) return TRUE;

	if ((p_ptr->pclass == CLASS_TELEPATH) && (o_ptr->tval ==TV_PSI_BOOK)) return TRUE;
#else
	if (o_ptr->tval == TV_MAGIC_BOOK) return TRUE;

	if (o_ptr->tval == TV_SORCERY_BOOK) return TRUE;

	if (o_ptr->tval == TV_SHADOW_BOOK) return TRUE;

	if (o_ptr->tval == TV_HUNT_BOOK) return TRUE;

	if (o_ptr->tval ==TV_PSI_BOOK) return TRUE;
#endif	// 0

	return FALSE;
}

void cmd_all_in_one(void)
{
	int item, dir;

	if (!c_get_item(&item, "Use which item? ", TRUE, TRUE, FALSE))
	{
		return;
	}

	if (INVEN_WIELD <= item)
	{
		Send_activate(item);
		return;
	}

	switch (inventory[item].tval)
	{
		case TV_POTION:
		case TV_POTION2:
		{
			Send_quaff(item);
			break;
		}

		case TV_SCROLL:
		{
			Send_read(item);
			break;
		}

		case TV_WAND:
		{
			get_dir(&dir);
			Send_aim(item, dir);
			break;
		}

		case TV_STAFF:
		{
			Send_use(item);
			break;
		}

		case TV_ROD:
		{
			Send_zap(item);
			break;
		}

		case TV_LITE:
		{
			if (inventory[INVEN_LITE].tval)
			{
				Send_fill(item);
			}
			else
			{ 
				Send_wield(item);
			}
			break;
		}

		case TV_FLASK:
		{
			Send_fill(item);
			break;
		}
	
		case TV_FOOD:
		{
			Send_eat(item);
			break;
		}

		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOOMERANG:
		{
#if 0
			if (!get_dir(&dir))
				return;

			Send_fire(item, dir);
#else	// 0
			Send_wield(item);
#endif	// 0
			break;
		}

		case TV_PSI_BOOK:
		case TV_MAGIC_BOOK:
		case TV_SORCERY_BOOK:
		case TV_SHADOW_BOOK:
		case TV_HUNT_BOOK:
		{
			if ((*item_tester_magicable)(&inventory[item]))
			{
				 do_cast(item);
			}
			else
			{
				c_msg_print("You tried to decipher the contents in vain.");
			}
			break;
		}

		case TV_FIGHT_BOOK:
		{
//			if ((*item_tester_magicable)(&inventory[item]))
			if (p_ptr->pclass == CLASS_WARRIOR)
			{
				 do_fight(item);
			}
			else
			{
//				c_msg_print("You are not strong enough.");
				c_msg_print("You find it hard to follow those acrobatic instructions.");
			}
			break;
		}



		case TV_PRAYER_BOOK:
		{
			if (class != CLASS_PRIEST && class != CLASS_PALADIN)
        	{
				c_msg_print("You pray.");
			}
			else
			{
				do_pray(item);
			}
			break;
        }

		/* NOTE: 'item' isn't actually sent */
		case TV_SPIKE:
		{
			if (!get_dir(&dir))
				return;

			Send_spike(dir);
			break;
		}

		case TV_BOW:
//		case TV_BOOMERANG:	// soon
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_AXE:
		case TV_MSTAFF:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		case TV_AMULET:
		case TV_RING:
		case TV_TOOL:
		case TV_INSTRUMENT:
		{
			Send_wield(item);
			break;
		}

		default:
		{
			prt("Sorry I cannot handle that.", 0, 0);
			break;
		}
	}

		/* prototype
		case :
		{
			
			break;
		}
		*/
}

/* Handle all commands */

void process_command()
{
	/* Parse the command */
	switch (command_cmd)
	{
		/* Ignore */
		case ESCAPE:
		case ' ':
			break;

			/* Ignore return */
		case '\r':
			break;

			/* Clear buffer */
		case ')':
			cmd_clear_buffer();
			break;

			/*** Movement Commands ***/

			/* Dig a tunnel*/
		case '+':
			cmd_tunnel();
			break;

			/* Move */
		case ';':
			cmd_walk();
			break;

			/*** Running, Staying, Resting ***/
		case '.':
			cmd_run();
			break;

                case 'X':
			do_cmd_skill();
			break;

		case ',':
		case 'g':
			cmd_stay();
			break;

			/* Get the mini-map */
		case 'M':
			cmd_map();
			break;

			/* Recenter map */
		case 'L':
			cmd_locate();
			break;

			/* Search */
		case 's':
			cmd_search();
			break;

			/* Toggle Search Mode */
		case 'S':
			cmd_toggle_search();
			break;

			/* Rest */
		case 'R':
			cmd_rest();
			break;

			/*** Stairs and doors and chests ***/

			/* Go up */
		case '<':
			cmd_go_up();
			break;

			/* Go down */
		case '>':
			cmd_go_down();
			break;

			/* Open a door */
		case 'o':
			cmd_open();
			break;

			/* Close a door */
		case 'c':
			cmd_close();
			break;

			/* Bash a door */
		case 'B':
			cmd_bash();
			break;

			/* Disarm a trap or chest */
		case 'D':
			cmd_disarm();
			break;

			/*** Inventory commands ***/
		case 'i':
			cmd_inven();
			break;

		case 'e':
			cmd_equip();
			break;

		case 'd':
			cmd_drop();
			break;

		case '$':
			cmd_drop_gold();
			break;

		case 'w':
			cmd_wield();
			break;

		case 't':
			cmd_take_off();
			break;

		case 'k':
			cmd_destroy();
			break;

		case 'K':
			cmd_king();
			break;

		case '{':
			cmd_inscribe();
			break;

		case '}':
			cmd_uninscribe();
			break;

		case 'j':
			cmd_steal();
			break;

			/*** Inventory "usage" commands ***/
		case 'q':
			cmd_quaff();
			break;

		case 'r':
			cmd_read_scroll();
			break;

		case 'a':
			cmd_aim_wand();
			break;

		case 'u':
			cmd_use_staff();
			break;

		case 'z':
			cmd_zap_rod();
			break;

		case 'F':
			cmd_refill();
			break;

		case 'E':
			cmd_eat();
			break;

		case 'A':
			cmd_activate();
			break;

			/*** Firing and throwing ***/
		case 'f':
			cmd_fire();
			break;

		case 'v':
			cmd_throw();
			break;

			/*** Spell casting ***/
		case 'b':
			cmd_browse();
			break;

		case 'G':
			cmd_study();
			break;

		case 'm':
			do_cmd_activate_skill();
			break;

		case 'p':
			cmd_pray();
			break;

		case 'n':
			cmd_fight();
			break;

		case 'N':
			cmd_mimic();
			break;

		case 'U':
			cmd_ghost();
			break;

		case 'x':
			cmd_mind();
			break;
			/*** Looking/Targetting ***/
		case '*':
			cmd_target();
			break;

		case '(':
			cmd_target_friendly();
			break;

		case 'l':
			cmd_look();
			break;

		case 'I':
			cmd_observe();
			break;

			/*** Information ***/
		case 'C':
			cmd_character();
			break;

		case '~':
			cmd_artifacts();
			break;

		case '|':
			cmd_uniques();
			break;

		case '\'':
			cmd_player_equip();
			break;

		case '@':
			cmd_players();
			break;

		case '#':
			cmd_high_scores();
			break;

		case '?':
			cmd_help();
			break;
			/*** Miscellaneous ***/
		case ':':
			cmd_message();
			break;

		case 'P':
			cmd_party();
			break;

		case '&':
			/* Dungeon master commands, normally only accessible to 
			 * a valid dungeon master.  These commands only are 
			 * effective for a valid dungeon master.
			 */

			/*
			   if (!strcmp(nick,DUNGEON_MASTER)) cmd_master(); 
			   else prt("Hit '?' for help.", 0, 0);
			   */
			cmd_master();
			break;

			/* Add separate buffer for chat only review (good for afk) -Zz */
		case KTRL('O'):
			do_cmd_messages_chatonly();
			break;

		case KTRL('P'):
			do_cmd_messages();
			break;

		case KTRL('X'):
			Net_cleanup();
			quit(NULL);

		case KTRL('R'):
			cmd_redraw();
			break;

		case 'Q':
			cmd_suicide();
			break;

		case '=':
			do_cmd_options();
			break;

			case '\"':
				cmd_load_pref();
			break;

		case '%':
			interact_macros();
			break;

		case 'h':
			cmd_purchase_house();
			break;

		case '/':
			cmd_all_in_one();
			break;

		case KTRL('S'):
			cmd_spike();
			break;

		default:
			prt("Hit '?' for help.", 0, 0);
			break;
	}
}




static void cmd_clear_buffer(void)
{
	Send_clear_buffer();
}



void cmd_tunnel(void)
{
	int dir = command_dir;

	if (!dir)
	{
		get_dir(&dir);
	}

	Send_tunnel(dir);
}

void cmd_walk(void)
{
	int dir = command_dir;

	if (!dir)
	{
		get_dir(&dir);
	}

	Send_walk(dir);
}

void cmd_run(void)
{
	int dir = command_dir;

	if (!dir)
	{
		get_dir(&dir);
	}

	Send_run(dir);
}

void cmd_stay(void)
{
	Send_stay();
}

void cmd_map(void)
{
	char ch;

	/* Hack -- if the screen is already icky, ignore this command */
	if (screen_icky) return;

	/* Save the screen */
	Term_save();

	/* Send the request */
	Send_map();

	/* Reset the line counter */
	last_line_info = 0;

	/* Wait until we get the whole thing */
	while (last_line_info < 23)
	{
		/* Wait for net input, or a key */
		ch = inkey();

		/* Check for user abort */
		if (ch == ESCAPE)
			break;
	}

	/* Reload the screen */
	Term_load();

	/* Flush any queued events */
	Flush_queue();
}

void cmd_locate(void)
{
	int dir;
	char ch;

	/* Initialize */
	Send_locate(5);

	/* Show panels until done */
	while (1)
	{
		/* Assume no direction */
		dir = 0;

		/* Get a direction */
		while (!dir)
		{
			/* Get a command (or Cancel) */
			ch = inkey();

			/* Check for cancel */
			if (ch == ESCAPE) break;

			/* Extract direction */
			dir = keymap_dirs[ch & 0x7F];

			/* Error */
			if (!dir) bell();
		}

		/* No direction */
		if (!dir) break;

		/* Send the command */
		Send_locate(dir);
	}

	/* Done */
	Send_locate(0);
	
	/* Clear */
	c_msg_print(NULL);
}

void cmd_search(void)
{
	Send_search();
}

void cmd_toggle_search(void)
{
	Send_toggle_search();
}

void cmd_rest(void)
{
	Send_rest();
}

void cmd_go_up(void)
{
	Send_go_up();
}

void cmd_go_down(void)
{
	Send_go_down();
}

void cmd_open(void)
{
	int dir = command_dir;

	if (!dir)
	{
		get_dir(&dir);
	}

	Send_open(dir);
}

void cmd_close(void)
{
	int dir = command_dir;

	if (!dir)
	{
		get_dir(&dir);
	}

	Send_close(dir);
}

void cmd_bash(void)
{
	int dir = command_dir;

	if (!dir)
	{
		get_dir(&dir);
	}

	Send_bash(dir);
}

void cmd_disarm(void)
{
	int dir = command_dir;

	if (!dir)
	{
		get_dir(&dir);
	}

	Send_disarm(dir);
}

void cmd_inven(void)
{
	/* First, erase our current location */

	/* Then, save the screen */
	Term_save();

	command_gap = 50;

	show_inven();

	(void)inkey();

	/* restore the screen */
	Term_load();
	/* print our new location */

	/* Flush any events */
	Flush_queue();
}

void cmd_equip(void)
{
	Term_save();

	command_gap = 50;

	/* Hack -- show empty slots */
	item_tester_full = TRUE;

	show_equip();

	/* Undo the hack above */
	item_tester_full = FALSE;

	(void)inkey();

	Term_load();

	/* Flush any events */
	Flush_queue();
}

void cmd_drop(void)
{
	int item, amt;

	if (!c_get_item(&item, "Drop what? ", TRUE, TRUE, FALSE))
	{
		return;
	}

	/* Get an amount */
	if (inventory[item].number > 1)
	{
		amt = c_get_quantity("How many? ", inventory[item].number);
	}
	else amt = 1;

	/* Send it */
	Send_drop(item, amt);
}

void cmd_drop_gold(void)
{
	s32b amt;

	/* Get how much */
	amt = c_get_quantity("How much gold? ", -1);

	/* Send it */
	if (amt)
		Send_drop_gold(amt);
}

void cmd_wield(void)
{
	int item;

	if (!c_get_item(&item, "Wear/Wield which item? ", FALSE, TRUE, FALSE))
	{
		return;
	}

	/* Send it */
	Send_wield(item);
}

void cmd_observe(void)
{
	int item;

	if (!c_get_item(&item, "Examine which item? ", TRUE, TRUE, FALSE))
	{
		return;
	}

	/* Send it */
	Send_observe(item);
}

void cmd_take_off(void)
{
	int item;

	if (!c_get_item(&item, "Takeoff which item? ", TRUE, FALSE, FALSE))
	{
		return;
	}

	/* Send it */
	Send_take_off(item);
}


void cmd_destroy(void)
{
	int item, amt;
	char out_val[160];

	if (!c_get_item(&item, "Destroy what? ", TRUE, TRUE, FALSE))
	{
		return;
	}

	/* Get an amount */
	if (inventory[item].number > 1)
	{
		amt = c_get_quantity("How many? ", inventory[item].number);
	}
	else amt = 1;

	/* Sanity check */
	if (!amt) return;
	else if (inventory[item].number == amt)
		sprintf(out_val, "Really destroy %s? ", inventory_name[item]);
	else
		sprintf(out_val, "Really destroy %d of your %s? ", amt, inventory_name[item]);
	if (!get_check(out_val)) return;

	/* Send it */
	Send_destroy(item, amt);
}


void cmd_inscribe(void)
{
	int item;
	char buf[1024];

	if (!c_get_item(&item, "Inscribe what? ", TRUE, TRUE, FALSE))
	{
		return;
	}

	buf[0] = '\0';

	/* Get an inscription */
	if (get_string("Inscription: ", buf, 59))
		Send_inscribe(item, buf);
}

void cmd_uninscribe(void)
{
	int item;

	if (!c_get_item(&item, "Uninscribe what? ", TRUE, TRUE, FALSE))
	{
		return;
	}

	/* Send it */
	Send_uninscribe(item);
}

void cmd_steal(void)
{
	int dir;

	get_dir(&dir);

	/* Send it */
	Send_steal(dir);
}

static bool item_tester_quaffable(object_type *o_ptr)
{
	if (o_ptr->tval == TV_POTION) return TRUE;
	if (o_ptr->tval == TV_POTION2) return TRUE;
	if ((o_ptr->tval == TV_FOOD) &&
			(o_ptr->sval == SV_FOOD_PINT_OF_ALE ||
			 o_ptr->sval == SV_FOOD_PINT_OF_WINE) )
			return TRUE;

	return FALSE;
}

void cmd_quaff(void)
{
	int item;

//	item_tester_tval = TV_POTION;
	item_tester_hook = item_tester_quaffable;

	if (!c_get_item(&item, "Quaff which potion? ", FALSE, TRUE, FALSE))
	{
		return;
	}

	/* Send it */
	Send_quaff(item);
}

void cmd_read_scroll(void)
{
	int item;

	item_tester_tval = TV_SCROLL;

	if (!c_get_item(&item, "Read which scroll? ", FALSE, TRUE, FALSE))
	{
		return;
	}

	/* Send it */
	Send_read(item);
}

void cmd_aim_wand(void)
{
	int item, dir;

	item_tester_tval = TV_WAND;

	if (!c_get_item(&item, "Aim which wand? ", FALSE, TRUE, FALSE))
	{
		return;
	}

	get_dir(&dir);
	
	/* Send it */
	Send_aim(item, dir);
}

void cmd_use_staff(void)
{
	int item;

	item_tester_tval = TV_STAFF;

	if (!c_get_item(&item, "Use which staff? ", FALSE, TRUE, FALSE))
	{
		return;
	}

	/* Send it */
	Send_use(item);
}

void cmd_zap_rod(void)
{
	int item;

	item_tester_tval = TV_ROD;

	if (!c_get_item(&item, "Use which rod? ", FALSE, TRUE, FALSE))
	{
		return;
	}

	/* Send it */
	Send_zap(item);
}

void cmd_refill(void)
{
	int item;
	cptr p;

#if 0
	if (inventory[INVEN_LITE].tval == TV_TORCH)
	{
		item_tester_tval = TV_TORCH;
		p = "Refill with which torch? ";
	}

	else if (inventory[INVEN_LITE].tval == TV_LANTERN)
	{
		item_tester_tval = TV_FLASK;
		p = "Refill with which flask? ";
	}

	else
	{
		c_msg_print("Your light cannot be refilled.");
		return;
	}
#endif

	p = "Refill with which light? ";

	if (!c_get_item(&item, p, FALSE, TRUE, FALSE))
	{
		return;
	}

	/* Send it */
	Send_fill(item);
}

void cmd_eat(void)
{
	int item;

	item_tester_tval = TV_FOOD;

	if (!c_get_item(&item, "Eat what? ", FALSE, TRUE, FALSE))
	{
		return;
	}

	/* Send it */
	Send_eat(item);
}

void cmd_activate(void)
{
	int item;

        if (!c_get_item(&item, "Activate what? ", TRUE, TRUE, FALSE))
	{
		return;
	}

	/* Send it */
	Send_activate(item);
}


int cmd_target(void)
{
	bool done = FALSE;
	bool position = FALSE;
	int d;
	char ch;

	/* Tell the server to init targetting */
	Send_target(0);

	while (!done)
	{
		ch = inkey();

		if (!ch)
			continue;

		switch (ch)
		{
			case ESCAPE:
			case 'q':
			{
				/* Clear top line */
				prt("", 0, 0);
				return FALSE;
			}
			case 't':
			case '5':
			{
				done = TRUE;
				break;
			}
			case 'p':
			{
				/* Toggle */
				position = !position;

				/* Tell the server to reset */
				Send_target(128 + 0);

				break;
			}
			default:
			{
				d = keymap_dirs[ch & 0x7F];
				if (!d) 
				{
					/* APD exit if not a direction, since 
					 * the player is probably trying to do 
					 * something else, like stay alive...
					 */
					/* Clear the top line */
					prt("", 0, 0);
					return FALSE;
				}
				else
				{
					if (position)
						Send_target(d + 128);	
					else Send_target(d);
				}
				break;
			}
		}
	}

	/* Send the affirmative */
	if (position)
		Send_target(128 + 5);
	else Send_target(5);

	return TRUE;
}


int cmd_target_friendly(void)
{
	/* Tell the server to init targetting */
	Send_target_friendly(0);
	return TRUE;
}



void cmd_look(void)
{
	bool done = FALSE;
	int d;
	char ch;

	/* Tell the server to init looking */
	Send_look(0);

	while (!done)
	{
		ch = inkey();

		if (!ch) continue;

		switch (ch)
		{
			case ESCAPE:
			case 'q':
			{
				/* Clear top line */
				prt("", 0, 0);
				return;
			}
			default:
			{
				d = keymap_dirs[ch & 0x7F];
				if (!d) bell();
				else Send_look(d);
				break;
			}
		}
	}
}

			
void cmd_character(void)
{
	char ch = 0;
	int hist = 0, done = 0;

	/* Save screen */
	Term_save();

	while (!done)
	{
		/* Display player info */
		display_player(hist);

		/* Display message */
		prt("[ESC to quit, h to toggle history]", 21, 24);

		/* Wait for key */
		ch = inkey();

		/* Check for "display history" */
		if (ch == 'h' || ch == 'H')
		{
			/* Toggle */
			hist = !hist;
		}

		/* Check for quit */
		if (ch == 'q' || ch == 'Q' || ch == ESCAPE)
		{
			/* Quit */
			done = 1;
		}
	}

	/* Reload screen */
	Term_load();

	/* Flush any events */
	Flush_queue();
}



void cmd_artifacts(void)
{
	/* Set the hook */
	special_line_type = SPECIAL_FILE_ARTIFACT;

	/* Call the file perusal */
	peruse_file();
}

void cmd_uniques(void)
{
	/* Set the hook */
	special_line_type = SPECIAL_FILE_UNIQUE;

	/* Call the file perusal */
	peruse_file();
}

void cmd_players(void)
{
	/* Set the hook */
	special_line_type = SPECIAL_FILE_PLAYER;

	/* Call the file perusal */
	peruse_file();
}

void cmd_high_scores(void)
{
	/* Set the hook */
	special_line_type = SPECIAL_FILE_SCORES;

	/* Call the file perusal */
	peruse_file();
}

void cmd_help(void)
{
	/* Set the hook */
	special_line_type = SPECIAL_FILE_HELP;

	/* Call the file perusal */
	peruse_file();
}

void cmd_message(void)
{
	char buf[60];
	int i;

	buf[0] = '\0';

	if (get_string("Message: ", buf, 59)){
		for(i=0;i<60;i++){
			if(buf[i]=='{') buf[i]='\377';
		}
		Send_msg(buf);
	}
}

void cmd_party(void)
{
	char i;
	char buf[80];

	/* We are now in party mode */
	party_mode = TRUE;

	/* Save screen */
	Term_save();

	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Initialize buffer */
		buf[0] = '\0';

		/* Describe */
		Term_putstr(0, 2, -1, TERM_WHITE, "Party commands");

		/* Selections */
		/* See also: Receive_party() */
		Term_putstr(5, 4, -1, TERM_WHITE, "(1) Create a party");
		Term_putstr(5, 5, -1, TERM_WHITE, "(2) Add a player to party");
		Term_putstr(5, 6, -1, TERM_WHITE, "(3) Delete a player from party");
		Term_putstr(5, 7, -1, TERM_WHITE, "(4) Leave your current party");
		Term_putstr(5, 8, -1, TERM_WHITE, "(5) Specify player to attack");
		Term_putstr(5, 9, -1, TERM_WHITE, "(6) Make peace");
		Term_putstr(5, 10, -1, TERM_WHITE, "(7) Create a new guild");
		Term_putstr(5, 11, -1, TERM_WHITE, "(8) Add player to guild");
		Term_putstr(5, 12, -1, TERM_WHITE, "(9) Remove player from guild");
		Term_putstr(5, 13, -1, TERM_WHITE, "(a) Leave guild");

		/* Show current party status */
		Term_putstr(0, 16, -1, TERM_WHITE, party_info);

		/* Prompt */
		Term_putstr(0, 15, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		/* Create party */
		else if (i == '1')
		{
			/* Get party name */
			if (get_string("Party name: ", buf, 79))
				Send_party(PARTY_CREATE, buf);
		}

		/* Add player */
		else if (i == '2')
		{
			/* Get player name */
			if (get_string("Add player: ", buf, 79))
				Send_party(PARTY_ADD, buf);
		}

		/* Delete player */
		else if (i == '3')
		{
			/* Get player name */
			if (get_string("Delete player: ", buf, 79))
				Send_party(PARTY_DELETE, buf);
		}

		/* Leave party */
		else if (i == '4')
		{
			/* Send the command */
			Send_party(PARTY_REMOVE_ME, "");
		}

		/* Attack player/party */
		else if (i == '5')
		{
			/* Get player name */
			if (get_string("Player/party to attack: ", buf, 79))
				Send_party(PARTY_HOSTILE, buf);
		}

		/* Make peace with player/party */
		else if (i == '6')
		{
			/* Get player/party name */
			if (get_string("Make peace with: ", buf, 79))
				Send_party(PARTY_PEACE, buf);
		}
		else if (i == '7'){
			/* Get new guild name */
			if (get_string("Guild name: ", buf, 79))
				Send_guild(GUILD_CREATE, buf);
		}
		else if (i == '8'){
			/* Get player name */
			if (get_string("Player name: ", buf, 79))
				Send_guild(GUILD_ADD, buf);
		}
		else if (i == '9'){
			/* Get player name */
			if (get_string("Player name: ", buf, 79))
				Send_guild(GUILD_DELETE, buf);
		}
		else if (i == 'a'){
			if (get_check("Leave the guild ? "))
				Send_guild(GUILD_REMOVE_ME, "");
		}

		/* Oops */
		else
		{
			/* Ring bell */
			bell();
		}

		/* Flush messages */
		c_msg_print(NULL);
	}

	/* Reload screen */
	Term_load();

	/* No longer in party mode */
	party_mode = FALSE;

	/* Flush any events */
	Flush_queue();
}


void cmd_fire(void)
{
	int item, dir;

#if 0
	if (!c_get_item(&item, "Fire which ammo? ", FALSE, TRUE, FALSE))
	{
		return;
	}
#endif	// 0

	if (!get_dir(&dir))
		return;
	
	/* Send it */
//	Send_fire(item, dir);
	Send_fire(dir);
}

void cmd_throw(void)
{
	int item, dir;

	if (!c_get_item(&item, "Throw what? ", FALSE, TRUE, FALSE))
	{
		return;
	}

	if (!get_dir(&dir))
		return;

	/* Send it */
	Send_throw(item, dir);
}

static bool item_tester_browsable(object_type *o_ptr)
{
	if (((p_ptr->pclass == CLASS_MAGE) || (p_ptr->pclass == CLASS_RANGER)) &&
	    (o_ptr->tval == TV_MAGIC_BOOK))
	{
		return TRUE;
	}
	if (((p_ptr->pclass == CLASS_PRIEST) || (p_ptr->pclass == CLASS_PALADIN)) &&
	    (o_ptr->tval == TV_PRAYER_BOOK))
	{
		return TRUE;
	}
	
	if ((p_ptr->pclass == CLASS_SORCERER) && (o_ptr->tval == TV_SORCERY_BOOK))
	{
		return TRUE;
	}
	
	if ((p_ptr->pclass == CLASS_ROGUE) && (o_ptr->tval == TV_SHADOW_BOOK))
	{
		return TRUE;
	}
	
	if ((p_ptr->pclass == CLASS_ARCHER) && (o_ptr->tval == TV_HUNT_BOOK))
	{
		return TRUE;
	}
	
	if ((p_ptr->pclass == CLASS_WARRIOR) && (o_ptr->tval == TV_FIGHT_BOOK))
	{
		return TRUE;
	}

	if ((p_ptr->pclass == CLASS_TELEPATH) && (o_ptr->tval == TV_PSI_BOOK))
	{
		return TRUE;
	}
	
	return FALSE;
}

void cmd_browse(void)
{
	int item;

	if (p_ptr->ghost)
	{
		show_browse(0);
		return;
	}

	item_tester_hook = item_tester_browsable;

	if (!c_get_item(&item, "Browse which book? ", FALSE, TRUE, FALSE))
	{
		if (item == -2) c_msg_print("You have no books that you can read.");
		return;
	}

	/* Show it */
	show_browse(item);
}

void cmd_study(void)
{
	int item;

	item_tester_hook = item_tester_browsable;

	if (!c_get_item(&item, "Gain from which book? ", FALSE, TRUE, FALSE))
	{
		if (item == -2) c_msg_print("You have no books that you can read.");
		return;
	}

	/* Pick a spell and do it */
	do_study(item);
}

void cmd_cast(void)
{
	int item;

	if ((class != CLASS_MAGE) && (class != CLASS_ROGUE) && (class != CLASS_RANGER) && (class != CLASS_SORCERER) && (class != CLASS_ARCHER) && (class != CLASS_TELEPATH))
	{
		c_msg_print("You cannot cast spells!");
		return;
	}

	item_tester_hook = item_tester_magicable;

	if (!c_get_item(&item, "Cast from what book? ", FALSE, TRUE, FALSE))
	{
		if (item == -2) c_msg_print("You have no books that you can cast from.");
		return;
	}

	/* Pick a spell and do it */
	do_cast(item);
}

void cmd_pray(void)
{
	int item;

	if (class != CLASS_PRIEST && class != CLASS_PALADIN)
	{
		c_msg_print("Pray hard enough and your prayers may be answered.");
		return;
	}

	item_tester_tval = TV_PRAYER_BOOK;

	if (!c_get_item(&item, "Pray from what book? ", FALSE, TRUE, FALSE))
	{
		if (item == -2) c_msg_print("You have no books that you can pray from.");
		return;
	}

	/* Pick a spell and do it */
	do_pray(item);
}

void cmd_mimic(void)
{
	if (!get_skill(SKILL_MIMIC))
	{
		c_msg_print("You are too solid.");
		return;
	}

	/* Pick a spell and do it */
	do_mimic();
}

void cmd_fight(void)
{
	int item;

	if (class != CLASS_WARRIOR)
	{
		c_msg_print("You are not strong enough.");
		return;
	}

	item_tester_tval = TV_FIGHT_BOOK;

	if (!c_get_item(&item, "Fight from what book? ", FALSE, TRUE, FALSE))
	{
		if (item == -2) c_msg_print("You have no books that you can fight from.");
		return;
	}

	/* Pick a spell and do it */
	do_fight(item);
}

void cmd_ghost(void)
{
	if (p_ptr->ghost)
		do_ghost();
	else
	{
		c_msg_print("You are not undead.");
	}
}

void cmd_mind(void)
{
	Send_mind();
}

void cmd_load_pref(void)
{
	char buf[80];

	buf[0] = '\0';

	if (get_string("Action: ", buf, 79))
		process_pref_file_aux(buf);
}

void cmd_redraw(void)
{
	Send_redraw();
	keymap_init();
}

void cmd_house_chown(int dir)
{
	char i=0;
	char buf[80];

	Term_clear();
	Term_putstr(0, 2, -1, TERM_BLUE, "Select owner type");
	Term_putstr(5, 4, -1, TERM_WHITE, "(1) Player");
	Term_putstr(5, 5, -1, TERM_WHITE, "(2) Party");
	Term_putstr(5, 6, -1, TERM_WHITE, "(3) Class");
	Term_putstr(5, 7, -1, TERM_WHITE, "(4) Race");
	Term_putstr(5, 8, -1, TERM_WHITE, "(5) Guild");
	while(i!=ESCAPE){
		i=inkey();
		switch(i){
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
				buf[0]='O';
				buf[1]=i;
				buf[2]=0;
				get_string("Enter new name:",&buf[2],79);
				Send_admin_house(dir,buf);
				return;
			default:
				bell();
		}
		c_msg_print(NULL);
	}
}

void cmd_house_chmod(int dir){
	char buf[80];
	char mod=ACF_NONE;
	u16b minlev=0;
	Term_clear();
	Term_putstr(0, 2, -1, TERM_BLUE, "Set new permissions");
	if (get_check("Allow party access? ")) mod |= ACF_PARTY;
	if (get_check("Allow class access? ")) mod |= ACF_CLASS;
	if (get_check("Allow race access? ")) mod |= ACF_RACE;
	minlev=c_get_quantity("Minimum level: ", 127);
	if(minlev>1) mod |= ACF_LEVEL;
	buf[0]='M';
	if((buf[1]=mod))
		sprintf(&buf[2],"%hd",minlev);
	Send_admin_house(dir,buf);
}

void cmd_house_kill(int dir){
	Send_admin_house(dir,"K");
}
void cmd_purchase_house(void)
{
	char i=0;
	int dir;

	if (!get_dir(&dir)) return;

	Term_save();
	Term_clear();
	Term_putstr(0, 2, -1, TERM_BLUE, "House commands");
	Term_putstr(5, 4, -1, TERM_WHITE, "(1) Buy/Sell house");
	Term_putstr(5, 5, -1, TERM_WHITE, "(2) Change house owner");
	Term_putstr(5, 6, -1, TERM_WHITE, "(3) Change house permissions");
	Term_putstr(5, 7, -1, TERM_WHITE, "(4) Delete house");

	while(i!=ESCAPE){
		i=inkey();
		switch(i){
			case '1':
	
				/* Send it */
				Send_purchase_house(dir);
				i=ESCAPE;
				break;
			case '2':
				cmd_house_chown(dir);
				i=ESCAPE;
				break;
			case '3':
				cmd_house_chmod(dir);
				i=ESCAPE;
				break;
			case '4':
				cmd_house_kill(dir);
				i=ESCAPE;
				break;
			default:
				bell();
		}
		c_msg_print(NULL);
	}
	Term_load();
}

void cmd_suicide(void)
{
	int i;

	/* Verify */
	if (!get_check("Do you really want to commit suicide? ")) return;

	/* Check again */
	prt("Please verify SUICIDE by typing the '@' sign: ", 0, 0);
	flush();
	i = inkey();
	prt("", 0, 0);
	if (i != '@') return;

	/* Send it */
	Send_suicide();
}

void cmd_master_aux_level(void)
{
	char i;
	char buf[80];

	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Initialize buffer */
		buf[0] = '\0';

		/* Describe */
		Term_putstr(0, 2, -1, TERM_WHITE, "Level commands");

		/* Selections */
		Term_putstr(5, 4, -1, TERM_WHITE, "(1) Static your current level");
		Term_putstr(5, 5, -1, TERM_WHITE, "(2) Unstatic your current level");
		Term_putstr(5, 6, -1, TERM_WHITE, "(3) Add dungeon");
		Term_putstr(5, 7, -1, TERM_WHITE, "(4) Remove dungeon");
		Term_putstr(5, 8, -1, TERM_WHITE, "(5) Town generation");



		/* Prompt */
		Term_putstr(0, 9, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		/* static the current level */
		else if (i == '1')
		{
			Send_master(MASTER_LEVEL, "s");
		}

		/* unstatic the current level */
		else if (i == '2')
		{
			Send_master(MASTER_LEVEL, "u");
		}
		else if (i == '3'){	/* create dungeon stair here */
			buf[0]='D';
			buf[4]=0;
			buf[1]=c_get_quantity("Base level: ", 127);
			buf[2]=c_get_quantity("Max depth (1-127): ",127);
			buf[3]=(get_check("Is it a tower? ") ? 't':'d');
			if(get_check("Random dungeon?")) buf[4] |= DUNGEON_RANDOM;
			if(get_check("Hellish?")) buf[4] |= DUNGEON_HELL;
			if(get_check("Not mappable?")) buf[4] |= DUNGEON_NOMAP;
			if(get_check("Ironman?")) buf[4] |= DUNGEON_IRON;
			Send_master(MASTER_LEVEL, buf);
		}
		else if (i == '4'){
			buf[0]='R';
			buf[1]='\0';
			Send_master(MASTER_LEVEL, buf);
		}
		else if (i == '5'){
			buf[0]='T';
			buf[1]=c_get_quantity("Base level: ", 127);
			Send_master(MASTER_LEVEL, buf);
		}

		/* Oops */
		else
		{
			/* Ring bell */
			bell();
		}

		/* Flush messages */
		c_msg_print(NULL);
	}
}

void cmd_master_aux_generate_vault(void)
{
	char i, redo_hack;
	char buf[80];

	/* Process requests until done */
	while (1)
	{
		redo_hack = 0;
		
		/* Clear screen */
		Term_clear();

		/* Initialize buffer */
		buf[0] = 'v';

		/* Describe */
		Term_putstr(0, 2, -1, TERM_WHITE, "Generate Vault");

		/* Selections */
		Term_putstr(5, 4, -1, TERM_WHITE, "(1) By number");
		Term_putstr(5, 5, -1, TERM_WHITE, "(2) By name");

		/* Prompt */
		Term_putstr(0, 8, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		/* Generate by number */
		else if (i == '1')
		{
			buf[1] = '#';
			buf[2] = c_get_quantity("Vault number? ", 255) - 127;
			if(!buf[2]) redo_hack = 1;
			buf[3] = 0;
		}
		
		/* Generate by name */
		else if (i == '2')
		{
			buf[1] = 'n';
			get_string("Enter vault name: ", &buf[2], 79);
			if(!buf[2]) redo_hack = 1;
		}

		/* Oops */
		else
		{
			/* Ring bell */
			bell(); redo_hack = 1;
		}

		/* hack -- don't do this if we hit an invalid key previously */
		if(redo_hack) continue;

		Send_master(MASTER_GENERATE, buf);

		/* Flush messages */
		c_msg_print(NULL);
	}
}

void cmd_master_aux_generate(void)
{
	char i;
	char buf[80];

	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Initialize buffer */
		buf[0] = '\0';

		/* Describe */
		Term_putstr(0, 2, -1, TERM_WHITE, "Generation commands");

		/* Selections */
		Term_putstr(5, 4, -1, TERM_WHITE, "(1) Vault");

		/* Prompt */
		Term_putstr(0, 7, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		/* Generate a vault */
		else if (i == '1')
		{
			cmd_master_aux_generate_vault();
		}

		/* Oops */
		else
		{
			/* Ring bell */
			bell();
		}

		/* Flush messages */
		c_msg_print(NULL);
	}
}



void cmd_master_aux_build(void)
{
	char i;
	char buf[80];

	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Initialize buffer */
		buf[0] = FEAT_FLOOR;

		/* Describe */
		Term_putstr(0, 2, -1, TERM_WHITE, "Building commands");

		/* Selections */
		Term_putstr(5, 4, -1, TERM_WHITE, "(1) Granite Mode");
		Term_putstr(5, 5, -1, TERM_WHITE, "(2) Permanent Mode");
		Term_putstr(5, 6, -1, TERM_WHITE, "(3) Tree Mode");
		Term_putstr(5, 7, -1, TERM_WHITE, "(4) Evil Tree Mode");
		Term_putstr(5, 8, -1, TERM_WHITE, "(5) Grass Mode");
		Term_putstr(5, 9, -1, TERM_WHITE, "(6) Dirt Mode");
		Term_putstr(5, 10, -1, TERM_WHITE, "(7) Floor Mode");
		Term_putstr(5, 11, -1, TERM_WHITE, "(8) Special Door Mode");
		Term_putstr(5, 12, -1, TERM_WHITE, "(9) Signpost");
		Term_putstr(5, 13, -1, TERM_WHITE, "(a) Build Mode Off");

		/* Prompt */
		Term_putstr(0, 14, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		buf[1] = 'T';
		buf[2] = '\0';

		switch (i)
		{
			/* Granite mode on */
			case '1': buf[0] = FEAT_WALL_EXTRA; break;
			/* Perm mode on */
			case '2': buf[0] = FEAT_PERM_EXTRA; break;
			/* Tree mode on */
			case '3': buf[0] = FEAT_TREE; break;
			/* Evil tree mode on */
			case '4': buf[0] = FEAT_EVIL_TREE; break;
			/* Grass mode on */
			case '5': buf[0] = FEAT_GRASS; break;
			/* Dirt mode on */
			case '6': buf[0] = FEAT_DIRT; break;
			/* Floor mode on */
			case '7': buf[0] = FEAT_FLOOR; break;
			/* House door mode on */
			case '8':
				buf[0] = FEAT_HOME_HEAD;
				{
					u16b keyid;
					keyid=c_get_quantity("Enter key pval:",0xffff);
					sprintf(&buf[2],"%d",keyid);
				}
				break;
			/* Build mode off */
			case '9':
				buf[0] = FEAT_SIGN;
				get_string("Sign:", &buf[2], 77);
				break;
			case 'a': buf[0] = FEAT_FLOOR; buf[1] = 'F'; break;
			/* Oops */
			default : bell(); break;		
		}

		/* If we got a valid command, send it */
		if (buf[0]) Send_master(MASTER_BUILD, buf);

		/* Flush messages */
		c_msg_print(NULL);
	}
}

char * cmd_master_aux_summon_orcs(void)
{
	char i;
	static char buf[80];

	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Initialize buffer */
		buf[0] = '\0';

		/* Describe */
		Term_putstr(0, 2, -1, TERM_WHITE, "Summon which orcs?");

		/* Selections */
		Term_putstr(5, 4, -1, TERM_WHITE, "(1) Snagas");
		Term_putstr(5, 5, -1, TERM_WHITE, "(2) Cave Orcs");
		Term_putstr(5, 6, -1, TERM_WHITE, "(3) Hill Orcs");
		Term_putstr(5, 7, -1, TERM_WHITE, "(4) Black Orcs");
		Term_putstr(5, 8, -1, TERM_WHITE, "(5) Half-Orcs");
		Term_putstr(5, 9, -1, TERM_WHITE, "(6) Uruks");
		Term_putstr(5, 10, -1, TERM_WHITE, "(7) Random");

		/* Prompt */
		Term_putstr(0, 13, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		buf[0] = '\0';

		/* get the type of orc */
		switch (i)
		{
			case '1': strcpy(buf,"Snaga"); break;
			case '2': strcpy(buf,"Cave orc"); break;
			case '3': strcpy(buf,"Cave orc"); break;
			case '4': strcpy(buf,"Black orc"); break;
			case '5': strcpy(buf,"Half-orc"); break;
			case '6': strcpy(buf, "Uruk"); break;
			case '7': strcpy(buf, "random"); break;
			default : bell(); break;
		}

		/* if we got an orc type, return it */
		if (buf[0]) return buf;

		/* Flush messages */
		c_msg_print(NULL);
	}

	/* escape was pressed, no valid orcs types specified */
	return NULL;
}

char * cmd_master_aux_summon_undead_low(void)
{
	char i;
	static char buf[80];

	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Initialize buffer */
		buf[0] = '\0';

		/* Describe */
		Term_putstr(0, 2, -1, TERM_WHITE, "Summon which low undead?");

		/* Selections */
		Term_putstr(5, 4, -1, TERM_WHITE, "(1) Poltergeist");
		Term_putstr(5, 5, -1, TERM_WHITE, "(2) Green glutton ghost");
		Term_putstr(5, 6, -1, TERM_WHITE, "(3) Lost soul");
		Term_putstr(5, 7, -1, TERM_WHITE, "(4) Skeleton kobold");
		Term_putstr(5, 8, -1, TERM_WHITE, "(5) Skeleton orc");
		Term_putstr(5, 9, -1, TERM_WHITE, "(6) Skeleton human");
		Term_putstr(5, 10, -1, TERM_WHITE, "(7) Zombified orc");
		Term_putstr(5, 11, -1, TERM_WHITE, "(8) Zombified human");
		Term_putstr(5, 12, -1, TERM_WHITE, "(9) Mummified orc");
		Term_putstr(5, 13, -1, TERM_WHITE, "(a) Moaning spirit");
		Term_putstr(5, 14, -1, TERM_WHITE, "(b) Vampire bat");
		Term_putstr(5, 15, -1, TERM_WHITE, "(c) Random");

		/* Prompt */
		Term_putstr(0, 18, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		buf[0] = '\0';

		/* get the type of undead */
		switch (i)
		{
			case '1': strcpy(buf,"Poltergeist"); break;
			case '2': strcpy(buf,"Green glutton ghost"); break;
			case '3': strcpy(buf,"Loust soul"); break;
			case '4': strcpy(buf,"Skeleton kobold"); break;
			case '5': strcpy(buf,"Skeleton orc"); break;
			case '6': strcpy(buf, "Skeleton human"); break;
			case '7': strcpy(buf, "Zombified orc"); break;
			case '8': strcpy(buf, "Zombified human"); break;
			case '9': strcpy(buf, "Mummified orc"); break;
			case 'a': strcpy(buf, "Moaning spirit"); break;
			case 'b': strcpy(buf, "Vampire bat"); break;
			case 'c': strcpy(buf, "random"); break;

			default : bell(); break;
		}

		/* if we got an undead type, return it */
		if (buf[0]) return buf;

		/* Flush messages */
		c_msg_print(NULL);
	}

	/* escape was pressed, no valid types specified */
	return NULL;
}


char * cmd_master_aux_summon_undead_high(void)
{
	char i;
	static char buf[80];

	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Initialize buffer */
		buf[0] = '\0';

		/* Describe */
		Term_putstr(0, 2, -1, TERM_WHITE, "Summon which high undead?");

		/* Selections */
		Term_putstr(5, 4, -1, TERM_WHITE, "(1) Vampire");
		Term_putstr(5, 5, -1, TERM_WHITE, "(2) Giant Skeleton troll");
		Term_putstr(5, 6, -1, TERM_WHITE, "(3) Lich");
		Term_putstr(5, 7, -1, TERM_WHITE, "(4) Master vampire");
		Term_putstr(5, 8, -1, TERM_WHITE, "(5) Dread");
		Term_putstr(5, 9, -1, TERM_WHITE, "(6) Nether wraith");
		Term_putstr(5, 10, -1, TERM_WHITE, "(7) Night mare");
		Term_putstr(5, 11, -1, TERM_WHITE, "(8) Vampire lord");
		Term_putstr(5, 12, -1, TERM_WHITE, "(9) Archpriest");
		Term_putstr(5, 13, -1, TERM_WHITE, "(a) Undead beholder");
		Term_putstr(5, 14, -1, TERM_WHITE, "(b) Dreadmaster");
		Term_putstr(5, 15, -1, TERM_WHITE, "(c) Nightwing");
		Term_putstr(5, 16, -1, TERM_WHITE, "(d) Nightcrawler");
		Term_putstr(5, 17, -1, TERM_WHITE, "(e) Random");

		/* Prompt */
		Term_putstr(0, 20, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		buf[0] = '\0';

		/* get the type of undead */
		switch (i)
		{
			case '1': strcpy(buf,"Vampire"); break;
			case '2': strcpy(buf,"Giant skeleton troll"); break;
			case '3': strcpy(buf,"Lich"); break;
			case '4': strcpy(buf,"Master vampire"); break;
			case '5': strcpy(buf,"Dread"); break;
			case '6': strcpy(buf, "Nether wraith"); break;
			case '7': strcpy(buf, "Night mare"); break;
			case '8': strcpy(buf, "Vampire lord"); break;
			case '9': strcpy(buf, "Archpriest"); break;
			case 'a': strcpy(buf, "Undead beholder"); break;
			case 'b': strcpy(buf, "Dreadmaster"); break;
			case 'c': strcpy(buf, "Nightwing"); break;
			case 'd': strcpy(buf, "Nightcrawler"); break;
			case 'e': strcpy(buf, "random"); break;

			default : bell(); break;
		}

		/* if we got an undead type, return it */
		if (buf[0]) return buf;

		/* Flush messages */
		c_msg_print(NULL);
	}

	/* escape was pressed, no valid orcs types specified */
	return NULL;
}


void cmd_master_aux_summon(void)
{
	char i, redo_hack;
	char buf[80];
	char * race_name;

	/* Process requests until done */
	while (1)
	{
		redo_hack = 0;

		/* Clear screen */
		Term_clear();

		/* Describe */
		Term_putstr(0, 2, -1, TERM_WHITE, "Summon . . .");

		/* Selections */
		Term_putstr(5, 4, -1, TERM_WHITE, "(1) Orcs");
		Term_putstr(5, 5, -1, TERM_WHITE, "(2) Low Undead");
		Term_putstr(5, 6, -1, TERM_WHITE, "(3) High Undead");
		Term_putstr(5, 7, -1, TERM_WHITE, "(4) Depth");
		Term_putstr(5, 8, -1, TERM_WHITE, "(5) Specific");
		Term_putstr(5, 9, -1, TERM_WHITE, "(6) Mass Genocide");
		Term_putstr(5, 10, -1, TERM_WHITE, "(7) Level *Genocide*");
		Term_putstr(5, 11, -1, TERM_WHITE, "(8) Summoning mode off");



		/* Prompt */
		Term_putstr(0, 13, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		/* get the type of monster to summon */
		switch (i)
		{
			/* orc menu */
			case '1': 
			{
				/* get the specific kind of orc */
				race_name = cmd_master_aux_summon_orcs();
				/* if no string was specified */
				if (!race_name) 
				{
					redo_hack = 1;
					break;
				}
				buf[2] = 'o';
				strcpy(&buf[3], race_name);
				break;
			}
			/* low undead menu */
			case '2': 
			{	/* get the specific kind of low undead */
				race_name = cmd_master_aux_summon_undead_low();
				/* if no string was specified */
				if (!race_name)
				{
					redo_hack = 1;
					break;
				}
				buf[2] = 'u';
				strcpy(&buf[3], race_name);
				break;
			}/* high undead menu */
			case '3': 
			{	/* get the specific kind of low undead */
				race_name = cmd_master_aux_summon_undead_high();
				/* if no string was specified */
				if (!race_name)
				{
					redo_hack = 1;
					break;
				}
				buf[2] = 'U';
				strcpy(&buf[3], race_name);
				break;
			}
			/* summon from a specific depth */
			case '4':
			{
				buf[2] = 'd';
				buf[3] = c_get_quantity("Summon from which depth? ", 127);
				if (!buf[3]) redo_hack = 1;
				buf[4] = 0; /* terminate the string */
				break;
			}
			/* summon a specific monster or character */
			case '5':
			{
				buf[2] = 's';
				buf[3] = 0;
				get_string("Summon which monster or character? ", &buf[3], 79);
				if (!buf[3]) redo_hack = 1;
				break;
			}

			case '6':
			{
				/* delete all the monsters near us */
				/* turn summoning mode on */
				buf[0] = 'T';
				buf[1] = 1;
				buf[2] = '0';
				buf[3] = '\0'; /* null terminate the monster name */
				Send_master(MASTER_SUMMON, buf);

				redo_hack = 1;
				break;
			}

			case '7':
				/* Mass mass genocide. Clear a level */
				/* Only does town & dungeon ATM */
				buf[0] = 'Q';
				buf[1] = c_get_quantity("What depth?", 127);
				buf[2] = '\0';
				Send_master(MASTER_SUMMON, buf);
				redo_hack = 1;
				break;

			case '8':
			{
				/* disable summoning mode */
				buf[0] = 'F';
				buf[3] = '\0'; /* null terminate the monster name */
				Send_master(MASTER_SUMMON, buf);

				redo_hack = 1;
				break;
			}	

			
			/* Oops */
			default : bell(); redo_hack = 1; break;
		}

		/* get how it should be summoned */

		/* hack -- make sure our method is unset so we only send 
		 * a monster summon request if we get a valid summoning type
		 */

		/* hack -- don't do this if we hit an invalid key previously */
		if (redo_hack) continue;

		while (1)
		{
			/* make sure we get a valid summoning type before summoning */
			buf[0] = 0;

			/* Clear screen */
			Term_clear();

			/* Describe */
			Term_putstr(0, 2, -1, TERM_WHITE, "Summon . . .");

			/* Selections */
			Term_putstr(5, 4, -1, TERM_WHITE, "(1) X here");
			Term_putstr(5, 5, -1, TERM_WHITE, "(2) X at random locations");
			Term_putstr(5, 6, -1, TERM_WHITE, "(3) Group here");
			Term_putstr(5, 7, -1, TERM_WHITE, "(4) Group at random location");
			Term_putstr(5, 8, -1, TERM_WHITE, "(5) Summoning mode");

			/* Prompt */
			Term_putstr(0, 10, -1, TERM_WHITE, "Command: ");

			/* Get a key */
			i = inkey();

			/* Leave */
			if (i == ESCAPE) break;

			/* get the type of summoning */
			switch (i)
			{
				/* X here */
				case '1': 
					buf[0] = 'x';
					buf[1] = c_get_quantity("Summon how many? ", 127);
					break;
					/* X in different places */
				case '2':
					buf[0] = 'X';
					buf[1] = c_get_quantity("Summon how many? ", 127);
					break;
					/* Group here */
				case '3':
					buf[0] = 'g';
					break;
					/* Group at random location */
				case '4':
					buf[0] = 'G';
					break;
					/* summoning mode on */
				case '5':
					buf[0] = 'T';
					buf[1] = 1;
					break;

					/* Oops */
				default : bell(); redo_hack = 1; break;
			}
			/* if we have a valid summoning type (escape was not just pressed)
			 * then summon the monster */
			if (buf[0]) Send_master(MASTER_SUMMON, buf);
		}

		/* Flush messages */
		c_msg_print(NULL);
	}
}

void cmd_master_aux_player()
{
	char i=0;
	char buf[80];
	Term_clear();
	Term_putstr(0, 2, -1, TERM_BLUE, "Player commands");
	Term_putstr(5, 4, -1, TERM_WHITE, "(1) Editor (offline)");
	Term_putstr(5, 5, -1, TERM_WHITE, "(2) Acquirement");
	Term_putstr(5, 6, -1, TERM_WHITE, "(3) Invoke wrath");
	Term_putstr(5, 7, -1, TERM_WHITE, "(4) Static player");
	Term_putstr(5, 8, -1, TERM_WHITE, "(5) Unstatic player");
	Term_putstr(5, 9, -1, TERM_WHITE, "(6) Delete player");
	Term_putstr(5, 10, -1, TERM_WHITE, "(7) Telekinesis");
	Term_putstr(5, 11, -1, TERM_WHITE, "(8) Broadcast");
	
	Term_putstr(0, 13, -1, TERM_WHITE, "Command: ");

	while(i!=ESCAPE){
		/* Get a key */
		i = inkey();
		buf[0]='\0';
		switch(i){
			case '1':
				buf[0]='E';
				get_string("Enter player name:",&buf[1],15);
				break;
			case '2':
				buf[0]='A';
				get_string("Enter player name:",&buf[1],15);
				break;
			case '3':
				buf[0]='k';
				get_string("Enter player name:",&buf[1],15);
				break;
			case '4':
				buf[0]='S';
				get_string("Enter player name:",&buf[1],15);
				break;
			case '5':
				buf[0]='U';
				get_string("Enter player name:",&buf[1],15);
				break;
			case '6':
				buf[0]='r';
				get_string("Enter player name:",&buf[1],15);
				break;
			case '7':
				/* DM to player telekinesis */
				buf[0]='t';
				get_string("Enter player name:",&buf[1],15);
				break;
			case '8':
				{
					int j;
					buf[0]='B';
					get_string("Message:",&buf[1],69);
					for(j=0;j<60;j++)
						if(buf[j]=='{') buf[j]='\377';
				}
				break;
			case ESCAPE:
				break;
			default:
				bell();
		}
		if (buf[0]) Send_master(MASTER_PLAYER, buf);

		/* Flush messages */
		c_msg_print(NULL);
	}
}

/* Dirty implementation.. FIXME		- Jir - */
void cmd_master_aux_system()
{
	char i=0;

	while(i!=ESCAPE){
		Term_clear();
		Term_putstr(0, 2, -1, TERM_BLUE, "System commands");
		Term_putstr(5, 4, -1, TERM_WHITE, "(1) View mangband.log");
		Term_putstr(5, 5, -1, TERM_WHITE, "(2) View mangband.rfe");
	
		Term_putstr(0, 12, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();
		switch(i){
			case '1':
				/* Set the hook */
				special_line_type = SPECIAL_FILE_LOG;

				/* Call the file perusal */
				peruse_file();

				break;
			case '2':
				/* Set the hook */
				special_line_type = SPECIAL_FILE_RFE;

				/* Call the file perusal */
				peruse_file();

				break;
			case ESCAPE:
				break;
			default:
				bell();
		}

		/* Flush messages */
		c_msg_print(NULL);
	}
}
/*
 * Upload/execute scripts
 */
void cmd_script_upload()
{
        char buf[1025], name[81];
        FILE *fff;
	
	name[0]='\0';

        if (!get_string("Script name: ", name + 1, 79)) return;

        /* Build the filename */
        path_build(buf, 1024, ANGBAND_DIR_SCPT, name + 1);
        fff = my_fopen(buf, "r");
        if (fff == NULL) return;

        name[0] = MASTER_SCRIPTB_W;
        Send_master(MASTER_SCRIPTB, name);

        /* Process the file */
        while (0 == my_fgets(fff, buf, 1024))
        {
                Send_master(MASTER_SCRIPTL, buf);
        }

        buf[0] = '\0';
        Send_master(MASTER_SCRIPTE, buf);

        my_fclose(fff);
        c_msg_print("Sent.");
}

/*
 * Upload/execute scripts
 */
void cmd_script_exec()
{
        char buf[81];

	buf[0]='\0';
        if (!get_string("Script> ", buf, 80)) return;

       	Send_master(MASTER_SCRIPTS, buf);
}

/* Dungeon Master commands */
void cmd_master(void)
{
	char i=0;
	char buf[80];

	party_mode = TRUE;

	/* Save screen */
	Term_save();

	/* Process requests until done */
	while (i!=ESCAPE)
	{
		/* Clear screen */
		Term_clear();

		/* Initialize buffer */
		buf[0] = '\0';

		/* Describe */
		Term_putstr(0, 2, -1, TERM_WHITE, "Dungeon Master commands");

		/* Selections */
		Term_putstr(5, 4, -1, TERM_WHITE, "(1) Level Commands");
		Term_putstr(5, 5, -1, TERM_WHITE, "(2) Building Commands");
		Term_putstr(5, 6, -1, TERM_WHITE, "(3) Summoning Commands");
		Term_putstr(5, 7, -1, TERM_WHITE, "(4) Generation Commands");
		Term_putstr(5, 8, -1, TERM_WHITE, "(5) Player Commands");
		Term_putstr(5, 9, -1, TERM_WHITE, "(6) System Commands");
		Term_putstr(5, 10, -1, TERM_WHITE, "(e) Execute script command");
		Term_putstr(5, 11, -1, TERM_WHITE, "(u) Upload script file");

		/* Prompt */
		Term_putstr(0, 13, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		switch(i){
			case '1':
				cmd_master_aux_level();
				break;
			case '2':
				cmd_master_aux_build();
				break;
			case '3':
				cmd_master_aux_summon();
				break;
			case '4':
				cmd_master_aux_generate();
				break;
			case '5':
				cmd_master_aux_player();
				break;
			case '6':
				cmd_master_aux_system();
				break;
			case 'e':
				cmd_script_exec();
				break;
			case 'u':
				cmd_script_upload();
				break;
			case ESCAPE:
				break;
			default:
				bell();
		}
		
		/* Flush messages */
		c_msg_print(NULL);
	}

	/* Reload screen */
	Term_load();

	/* No longer in party mode */
	party_mode = FALSE;

	/* Flush any events */
	Flush_queue();
}

void cmd_king()
{
	if (!get_check("Do you really want to own this land ?")) return;
	
	Send_King(KING_OWN);
}

void cmd_spike()
{
	int dir = command_dir;

	if (!dir)
	{
		get_dir(&dir);
	}

	Send_spike(dir);
}

