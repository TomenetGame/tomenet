/* $Id$ */
#include "angband.h"

/* reordering will allow these to be removed */
static void cmd_clear_buffer(void);
static void cmd_master(void);

static void cmd_clear_actions(void) {
	Send_clear_actions();
}

static void cmd_player_equip(void)
{
    /* Set the hook */
    special_line_type = SPECIAL_FILE_PLAYER_EQUIP;

    /* Call the file perusal */
    peruse_file();
}

static bool item_tester_edible(object_type *o_ptr)
{
	if (o_ptr->tval == TV_FOOD) return TRUE;
	if (o_ptr->tval == TV_FIRESTONE) return TRUE;

	return FALSE;
}

/* XXX not fully functional */
static bool item_tester_oils(object_type *o_ptr)
{
	if (o_ptr->tval == TV_LITE) return TRUE;
	if (o_ptr->tval == TV_FLASK) return TRUE;

	return FALSE;
}

static void cmd_all_in_one(void)
{
	int item, dir;

	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Use which item? ", (USE_EQUIP | USE_INVEN | USE_EXTRA)))
	{
		return;
	}

	if (INVEN_WIELD <= item)
	{
		/* Does item require aiming? (Always does if not yet identified) */
		if (inventory[item].uses_dir == 0) {
			/* (also called if server is outdated, since uses_dir will be 0 then) */
			Send_activate(item);
		} else {
			if (!get_dir(&dir)) return;
			Send_activate_dir(item, dir);
		}
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
		case TV_PARCHMENT:
		{
			Send_read(item);
			break;
		}

		case TV_WAND:
		{
			if (!get_dir(&dir)) return;
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
			/* Does rod require aiming? (Always does if not yet identified) */
			if (inventory[item].uses_dir == 0) {
				/* (also called if server is outdated, since uses_dir will be 0 then) */
				Send_zap(item);
			} else {
				if (!get_dir(&dir)) return;
				Send_zap_dir(item, dir);
			}
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
		case TV_FIRESTONE:
		{
			Send_eat(item);
			break;
		}

		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		{
			Send_wield(item);
			break;
		}

		/* NOTE: 'item' isn't actually sent */
		case TV_SPIKE:
		{
			if (!get_dir(&dir)) return;
			Send_spike(dir);
			break;
		}

		case TV_BOW:
		case TV_BOOMERANG:
		case TV_DIGGING:
		case TV_BLUNT:
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

		/* Presume it's sort of spellbook */
		case TV_BOOK:
		case TV_TRAPKIT:
		default:
		{
			int i;
			bool done = FALSE;
			for (i = 1; i < MAX_SKILLS; i++)
			{
				if (s_info[i].tval == inventory[item].tval)
				{
					if (s_info[i].action_mkey && p_ptr->s_info[i].value)
					{
						do_activate_skill(i, item);
						done = TRUE;
						break;
					}
					/* Now a number of skills shares same mkey */
				}
			}
			if (!done) {
				/* XXX there should be more generic 'use' command */
				/* Does item require aiming? (Always does if not yet identified) */
				if (inventory[item].uses_dir == 0) {
					/* (also called if server is outdated, since uses_dir will be 0 then) */
					Send_activate(item);
				} else {
					if (!get_dir(&dir)) return;
					Send_activate_dir(item, dir);
				}
			}
			break;
		}
	}
}

/* Handle all commands */

void process_command()
{
	/* Parse the command */
	switch (command_cmd)
	{
			/* Ignore */
		case ESCAPE:
		case '-': /* used for targetting since 4.4.6, so we ignore it to allow
			     macros that hackily use it for both directional and non-
			     directional things, so the 'key not in use' message is
			     suppressed for non-directional actions, just to look better. */
			break;

			/* Ignore mostly, but also stop automatically repeated actions */
		case ' ':
			cmd_clear_actions();
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

		case ',':
		case 'g':
			cmd_stay();
			break;

			/* Get the mini-map */
		case 'M':
			cmd_map(0);
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

		case 'x':
			cmd_swap();
			break;

		case 'k':
			cmd_destroy();
			break;

#if 0 /* currently no effect on server-side */
		case 'K':
			cmd_king();
			break;
#endif
		case 'K':
			cmd_force_stack();
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
			do_cmd_skill();
			break;
		case 'm':
			do_cmd_activate_skill();
			break;

		case 'U':
			cmd_ghost();
			break;

		case '*': /*** Looking/Targetting ***/
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

		case '_':
			cmd_sip();
			break;

		case 'p':
			cmd_telekinesis();
			break;

		case 'W':
			cmd_wield2();
			break;

		case 'V':
			cmd_cloak();
			break;

			/*** Information ***/
		case 'C':
			cmd_character();
			break;

		case '~':
			cmd_check_misc();
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

		case ']':
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
			/* Resend options to server */
			Send_options();
			break;

		case '%':
			interact_macros();
			break;

		case '&':
			auto_inscriptions();
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

		case KTRL('T'):
			xhtml_screenshot("screenshot????");
			break;

		case KTRL('I'):
			cmd_lagometer();
			break;

#ifdef USE_SOUND_2010
		case KTRL('U'):
			interact_audio();
			break;
//		case KTRL('M'): //this is same as '\r' and hence doesn't work..
		case KTRL('C'):
			toggle_music();
			break;
		case KTRL('N'):
			toggle_audio();
			break;
#endif

		case '!':
			cmd_BBS();
			break;
#if 0 /* only for debugging purpose - dump some client-side special config */
		case KTRL('C'):
			c_msg_format("Client FPS: %d", cfg_client_fps);
			break;
#endif
		default:
			cmd_raw_key(command_cmd);
			break;
	}
}




static void cmd_clear_buffer(void)
{
	/* Hack -- flush the key buffer */

	Send_clear_buffer();
}



void cmd_tunnel(void)
{
	int dir = command_dir;

	if (!dir) {
		if (!get_dir(&dir)) return;
	}

	Send_tunnel(dir);
}

void cmd_walk(void)
{
	int dir = command_dir;

	if (!dir) {
		if (!get_dir(&dir)) return;
	}

	Send_walk(dir);
}

void cmd_run(void)
{
	int dir = command_dir;

	if (!dir)
	{
		if (!get_dir(&dir)) return;
	}

	Send_run(dir);
}

void cmd_stay(void)
{
	Send_stay();
}

void cmd_map(char mode)
{
	char ch;

	/* Hack -- if the screen is already icky, ignore this command */
	if (screen_icky && !mode) return;

	/* Save the screen */
	Term_save();

	/* Send the request */
	Send_map(mode);

	/* Reset the line counter */
	last_line_info = 0;

	/* Wait until we get the whole thing */
	while (last_line_info < 23)
	{
		/* Hack - Get rid of the cursor - mikaelh */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Wait for net input, or a key */
		ch = inkey();

		/* Take a screenshot */
		if (ch == KTRL('T'))
		{
			xhtml_screenshot("screenshot????");
		}

		/* Check for user abort */
		else if (ch == ESCAPE)
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

			/* Take a screenshot */
			if (ch == KTRL('T'))
			{
				xhtml_screenshot("screenshot????");
			}

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
		if (!get_dir(&dir)) return;
	}

	Send_open(dir);
}

void cmd_close(void)
{
	int dir = command_dir;

	if (!dir)
	{
		if (!get_dir(&dir)) return;
	}

	Send_close(dir);
}

void cmd_bash(void)
{
	int dir = command_dir;

	if (!dir)
	{
		if (!get_dir(&dir)) return;
	}

	Send_bash(dir);
}

void cmd_disarm(void)
{
	int dir = command_dir;

	if (!dir)
	{
		if (!get_dir(&dir)) return;
	}

	Send_disarm(dir);
}

void cmd_inven(void)
{
	char ch; /* props to 'potato' and the korean team from sarang.net for the idea - C. Blue */
	int c;
	char buf[MSG_LEN];


	/* First, erase our current location */

	/* Then, save the screen */
	Term_save();

	command_gap = 50;

	show_inven();

	ch = inkey();
	if (ch == KTRL('T')) xhtml_screenshot("screenshot????");
	/* allow pasting item into chat */
	else if (ch >= 'A' && ch < 'A' + INVEN_PACK) { /* using capital letters to force SHIFT key usage, less accidental spam that way probably */
		c = ch - 'A';

		if (inventory[c].tval) {
			snprintf(buf, MSG_LEN - 1, "\377s%s", inventory_name[c]);
                        buf[MSG_LEN - 1] = 0;
                        Send_paste_msg(buf);
		}
	}

	/* restore the screen */
	Term_load();
	/* print our new location */

	/* Flush any events */
	Flush_queue();
}

void cmd_equip(void)
{
	char ch; /* props to 'potato' and the korean team from sarang.net for the idea - C. Blue */
	int c;
	char buf[MSG_LEN];


	Term_save();

	command_gap = 50;

	/* Hack -- show empty slots */
	item_tester_full = TRUE;

	show_equip();

	/* Undo the hack above */
	item_tester_full = FALSE;

	ch = inkey();
	if (ch == KTRL('T')) xhtml_screenshot("screenshot????");
	/* allow pasting item into chat */
	else if (ch >= 'A' && ch < 'A' + (INVEN_TOTAL - INVEN_WIELD)) { /* using capital letters to force SHIFT key usage, less accidental spam that way probably */
		c = ch - 'A';
		if (inventory[INVEN_WIELD + c].tval) {
			snprintf(buf, MSG_LEN - 1, "\377s%s", inventory_name[INVEN_WIELD + c]);
			buf[MSG_LEN - 1] = 0;
			Send_paste_msg(buf);
		}
	}

	Term_load();

	/* Flush any events */
	Flush_queue();
}

void cmd_drop(void)
{
	int item, amt;

	if (!c_get_item(&item, "Drop what? ", (USE_EQUIP | USE_INVEN))) return;

	/* Get an amount */
	if (inventory[item].number > 1) {
		if (is_cheap_misc(inventory[item].tval) && c_cfg.whole_ammo_stack && !verified_item
		    && item < INVEN_WIELD) /* <- new: ignore whole_ammo_stack for equipped ammo, so it can easily be shared */
			amt = inventory[item].number;
		else amt = c_get_quantity("How many (any letter = all)? ", inventory[item].number);
	}
	else amt = 1;

	/* Send it */
	Send_drop(item, amt);
}

void cmd_drop_gold(void)
{
	s32b amt;

	/* Get how much */
	amt = c_get_quantity("How much gold (any letter = all)? ", -1);

	/* Send it */
	if (amt)
		Send_drop_gold(amt);
}

void cmd_wield(void)
{
	int item;

	if (!c_get_item(&item, "Wear/Wield which item? ", (USE_INVEN)))
	{
		return;
	}

	/* Send it */
	Send_wield(item);
}

void cmd_wield2(void)
{
	int item;

	if (!c_get_item(&item, "Wear/Wield which item? ", (USE_INVEN)))
	{
		return;
	}

	/* Send it */
	Send_wield2(item);
}

void cmd_observe(void)
{
	int item;

	if (!c_get_item(&item, "Examine which item? ", (USE_EQUIP | USE_INVEN)))
	{
		return;
	}

	/* Send it */
	Send_observe(item);
}

void cmd_take_off(void)
{
	int item, amt = 255;

	if (!c_get_item(&item, "Takeoff which item? ", (USE_EQUIP))) return;

	/* New in 4.4.7a, for ammunition: Can take off a certain amount - C. Blue */
	if (inventory[item].number > 1 && verified_item
	    && is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
		amt = c_get_quantity("How many (any letter = all)? ", inventory[item].number);
		Send_take_off_amt(item, amt);
	} else {
		Send_take_off(item);
	}
}

void cmd_swap(void) {
	int item;

	if (!c_get_item(&item, "Swap which item? ", (USE_INVEN | USE_EQUIP | INVEN_FIRST))) return;

	if (item <= INVEN_PACK) Send_wield(item);
	else Send_take_off(item);
}


void cmd_destroy(void)
{
	int item, amt;
	char out_val[MSG_LEN];

	if (!c_get_item(&item, "Destroy what? ", (USE_EQUIP | USE_INVEN))) return;

	/* Get an amount */
	if (inventory[item].number > 1) {
		if (is_cheap_misc(inventory[item].tval) && c_cfg.whole_ammo_stack && !verified_item) amt = inventory[item].number;
		else amt = c_get_quantity("How many (any letter = all)? ", inventory[item].number);
	} else amt = 1;

	/* Sanity check */
	if (!amt) return;

	if (!c_cfg.no_verify_destroy) {
		if (inventory[item].number == amt)
			sprintf(out_val, "Really destroy %s? ", inventory_name[item]);
		else
			sprintf(out_val, "Really destroy %d of your %s? ", amt, inventory_name[item]);
		if (!get_check(out_val)) return;
	}

	/* Send it */
	Send_destroy(item, amt);
}


void cmd_inscribe(void)
{
	int item;
	char buf[1024];

	if (!c_get_item(&item, "Inscribe what? ", (USE_EQUIP | USE_INVEN | SPECIAL_REQ))) {
		if (item != -3) return;

		/* '-3' hack: Get inscription before item (SPECIAL_REQ) */
		buf[0] = '\0';

		/* Get an inscription first */
		if (!get_string("Inscription: ", buf, 59)) return;

		/* Get the item afterwards */
		if (!c_get_item(&item, "Inscribe what? ", (USE_EQUIP | USE_INVEN))) return;

		Send_inscribe(item, buf);
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

	if (!c_get_item(&item, "Uninscribe what? ", (USE_EQUIP | USE_INVEN)))
	{
		return;
	}

	/* Send it */
	Send_uninscribe(item);
}

void cmd_steal(void)
{
	int dir;

	if (!get_dir(&dir)) return;

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

	item_tester_hook = item_tester_quaffable;
	get_item_hook_find_obj_what = "Potion name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Quaff which potion? ", (USE_INVEN | USE_EXTRA)))
	{
		return;
	}

	/* Send it */
	Send_quaff(item);
}

static bool item_tester_readable(object_type *o_ptr)
{
	if (o_ptr->tval == TV_SCROLL) return TRUE;
	if (o_ptr->tval == TV_PARCHMENT) return TRUE;

	return FALSE;
}

void cmd_read_scroll(void)
{
	int item;

	item_tester_hook = item_tester_readable;
	get_item_hook_find_obj_what = "Scroll name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Read which scroll? ", (USE_INVEN | USE_EXTRA)))
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
	get_item_hook_find_obj_what = "Wand name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Aim which wand? ", (USE_INVEN | USE_EXTRA)))
	{
		return;
	}

	if (!get_dir(&dir)) return;

	/* Send it */
	Send_aim(item, dir);
}

void cmd_use_staff(void)
{
	int item;

	item_tester_tval = TV_STAFF;
	get_item_hook_find_obj_what = "Staff name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Use which staff? ", (USE_INVEN | USE_EXTRA)))
	{
		return;
	}

	/* Send it */
	Send_use(item);
}

void cmd_zap_rod(void)
{
	int item, dir;

	item_tester_tval = TV_ROD;
	get_item_hook_find_obj_what = "Rod name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Zap which rod? ", (USE_INVEN | USE_EXTRA)))
	{
		return;
	}

	/* Send it */
	if (inventory[item].uses_dir == 0) {
		/* (also called if server is outdated, since uses_dir will be 0 then) */
		Send_zap(item);
	} else {
		if (!get_dir(&dir)) return;
		Send_zap_dir(item, dir);
	}
}

/* FIXME: filter doesn't work nicely */
void cmd_refill(void)
{
	int item;
	cptr p;

	if (!inventory[INVEN_LITE].tval)
	{
		c_msg_print("You are not wielding a light source.");
		return;
	}

	item_tester_hook = item_tester_oils;
	p = "Refill with which light? ";

	if (!c_get_item(&item, p, (USE_INVEN)))
	{
		return;
	}

	/* Send it */
	Send_fill(item);
}

void cmd_eat(void)
{
	int item;

	item_tester_hook = item_tester_edible;
	get_item_hook_find_obj_what = "Food name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Eat what? ", (USE_INVEN | USE_EXTRA)))
	{
		return;
	}

	/* Send it */
	Send_eat(item);
}

void cmd_activate(void)
{
	int item, dir;

	/* Allow all items */
	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Activatable item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

        if (!c_get_item(&item, "Activate what? ", (USE_EQUIP | USE_INVEN | USE_EXTRA)))
	{
		return;
	}

	/* Send it */
	/* Does item require aiming? (Always does if not yet identified) */
	if (inventory[item].uses_dir == 0) {
		/* (also called if server is outdated, since uses_dir will be 0 then) */
		Send_activate(item);
	} else {
		if (!get_dir(&dir)) return;
		Send_activate_dir(item, dir);
	}
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

	/* Print the message */
	if (!c_cfg.taciturn_messages)
	{
		prt("Target selected.", 0, 0);
	}

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
			case KTRL('T'):
			{
				xhtml_screenshot("screenshot????");
				break;
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
        char tmp[100];

	/* Save screen */
	Term_save();

	while (!done)
	{
		/* Display player info */
		display_player(hist);

		/* Display message */
		prt("[ESC to quit, f to make a chardump, h to toggle history]", 22, 10);

		/* Wait for key */
		ch = inkey();

		/* Check for "display history" */
		if (ch == 'h' || ch == 'H')
		{
			/* Toggle */
			hist = !hist;
		}

		/* Dump */
		if ((ch == 'f') || (ch == 'F'))
		{
			strnfmt(tmp, 160, "%s.txt", cname);
			if (get_string("Filename(you can post it to http://angband.oook.cz/): ", tmp, 80))
			{
				if (tmp[0] && (tmp[0] != ' '))
				{
					file_character(tmp, FALSE);
				}
			}
		}

		/* Take a screenshot */
		if (ch == KTRL('T'))
		{
			xhtml_screenshot("screenshot????");
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

static void artifact_lore(void) {
	char s[20 + 1], tmp[80];
	int c, i, j, n, selected, selected_list;

	/* for pasting lore to chat */
	char paste_lines[18][MSG_LEN];

	Term_save();

  while (TRUE) {

	s[0] = '\0';
	Term_clear();
	Term_putstr(2,  2, -1, TERM_WHITE, "Enter (partial) artifact name to refine the search:");
	Term_putstr(2,  3, -1, TERM_WHITE, "Press RETURN to display lore about the first artifact.");

	while (TRUE) {
		Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Artifact Lore ***");
		Term_putstr(54,  2, -1, TERM_WHITE, "                                ");
		Term_putstr(54,  2, -1, TERM_L_GREEN, s);

		/* display top 15 of all matching artifacts */
		clear_from(5);
		n = 0;
		selected = selected_list = -1;

		/* hack: direct match always takes top position */
		if (s[0]) for (i = 0; i < MAX_A_IDX; i++) {
			/* create upper-case working copy */
			strcpy(tmp, artifact_list_name[i]);
			for (j = 0; tmp[j]; j++) tmp[j] = toupper(tmp[j]);

			if (!strcmp(tmp, s)) {
				selected = artifact_list_code[i];
				selected_list = i;
				Term_putstr(5, 5, -1, TERM_L_UMBER, artifact_list_name[i]);
				n++;
				break;
			}
		}

		for (i = 0; i < MAX_A_IDX && n < 15; i++) {
			/* direct match above already? */
			if (i == selected_list) continue;

			/* create upper-case working copy */
			strcpy(tmp, artifact_list_name[i]);
			for (j = 0; tmp[j]; j++) tmp[j] = toupper(tmp[j]);

			if (strstr(tmp, s)) {
				if (n == 0) {
					selected = artifact_list_code[i];
					selected_list = i;
				}
				Term_putstr(5, 5 + n, -1, n == 0 ? TERM_L_UMBER : TERM_UMBER, artifact_list_name[i]);
				n++;
			}
		}

		Term_putstr(28,  23, -1, TERM_WHITE, "-- press ESC to exit --");
		c = inkey();
		/* specialty: allow chatting from within here */
                if (c == ':') {
            		cmd_message();
            		continue;
            	}

		/* backspace */
		if (c == '\010' || c == 0x7F) {
			if (strlen(s) > 0) s[strlen(s) - 1] = '\0';
			continue;
		}
		/* Mustn't start on a SPACE */
		if (c == ' ' && !strlen(s)) continue;
		/* return */
		if (c == '\n' || c == '\r') {
			if (selected_list == -1) continue;
			break;
		}
		/* escape */
		if (c == ESCAPE) break;
		/* illegal char */
		if (c < 32 || c > 127) continue;
		/* name too long? */
		if (strlen(s) >= 20) continue;
		/* build name */
		c = toupper(c);
		strcat(s, format("%c", c));
	}

	/* exit? */
	if (c == ESCAPE) {
		Term_load();
		return;
	}

	/* display lore! */
	clear_from(5);
	for (i = 0; i < 18; i++) paste_lines[i][0] = '\0';
	artifact_lore_aux(selected, selected_list, paste_lines);

	Term_putstr(20,  23, -1, TERM_WHITE, "-- press ESC to exit, c to chat-paste --");
	while (TRUE) {
		c = inkey();
		/* specialty: allow chatting from within here */
                if (c == ':') {
            		cmd_message();
			Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Artifact Lore ***");
            	}
		if (c == '\e') break;
		if (c == 'c') {
			/* paste currently displayed artifact information into chat:
			   scan line #5 for title, lines 7..22 for info. */
			for (i = 0; i < 18; i++) {
				if (!paste_lines[i][0]) break;
				//if (i == 6 || i == 12) usleep(10000000);
				if (paste_lines[i][strlen(paste_lines[i]) - 1] == ' ')
					paste_lines[i][strlen(paste_lines[i]) - 1] = '\0';
				Send_paste_msg(paste_lines[i]);
			}
		}
	}
	//if (c == '\e') break;
  }

	Term_load();
}

static void monster_lore(void) {
	char s[20 + 1], tmp[80];
	int c, i, j, n, selected, selected_list;
	bool show_lore = TRUE;

	/* for pasting lore to chat */
	char paste_lines[18][MSG_LEN];

	Term_save();

    while (TRUE) {

	s[0] = '\0';
	Term_clear();
	Term_putstr(2,  2, -1, TERM_WHITE, "Enter (partial) monster name to refine the search:");
	Term_putstr(2,  3, -1, TERM_WHITE, "Press RETURN to display lore about the first monster.");

	while (TRUE) {
		Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Monster Lore ***");
		Term_putstr(53,  2, -1, TERM_WHITE, "                                ");
		Term_putstr(53,  2, -1, TERM_L_GREEN, s);

		/* display top 15 of all matching monsters */
		clear_from(5);
		n = 0;
		selected = selected_list = -1;

		/* hack 1: direct match always takes top position
		   hack 2: match at beginning of name takes precedence */
		if (s[0]) for (i = 1; i < MAX_R_IDX; i++) {
			/* create upper-case working copy */
			strcpy(tmp, monster_list_name[i]);
			for (j = 0; tmp[j]; j++) tmp[j] = toupper(tmp[j]);

			/* exact match? */
			if (!strcmp(tmp, s)) {
				selected = monster_list_code[i];
				selected_list = i;
				Term_putstr(5, 5, -1, TERM_YELLOW, format("(%4d)  %s", monster_list_code[i], monster_list_name[i]));
				n++;
				break;
			}
			/* beginning of line match? */
			else if (!strncmp(tmp, s, strlen(s))) {
				selected = monster_list_code[i];
				selected_list = i;
				Term_putstr(5, 5, -1, TERM_YELLOW, format("(%4d)  %s", monster_list_code[i], monster_list_name[i]));
				n++;
				break;
			}
		}

		for (i = 1; i < MAX_R_IDX && n < 15; i++) {
			/* direct match above already? */
			if (i == selected_list) continue;

			/* create upper-case working copy */
			strcpy(tmp, monster_list_name[i]);
			for (j = 0; tmp[j]; j++) tmp[j] = toupper(tmp[j]);

			if (strstr(tmp, s)) {
				if (n == 0) {
					selected = monster_list_code[i];
					selected_list = i;
				}
				Term_putstr(5, 5 + n, -1, n == 0 ? TERM_YELLOW : TERM_UMBER, format("(%4d)  %s", monster_list_code[i], monster_list_name[i]));
				n++;
			}
		}

		Term_putstr(28,  23, -1, TERM_WHITE, "-- press ESC to exit --");
		c = inkey();
		/* specialty: allow chatting from within here */
                if (c == ':') {
            		cmd_message();
            		continue;
            	}

		/* backspace */
		if (c == '\010' || c == 0x7F) {
			if (strlen(s)) s[strlen(s) - 1] = '\0';
			continue;
		}
		/* Mustn't start on a SPACE */
		if (c == ' ' && !strlen(s)) continue;
		/* return */
		if (c == '\n' || c == '\r') {
			if (selected_list == -1) continue;
			break;
		}
		/* escape */
		if (c == ESCAPE) break;
		/* illegal char */
		if (c < 32 || c > 127) continue;
		/* name too long? */
		if (strlen(s) >= 20) continue;
		/* build name */
		c = toupper(c);
		strcat(s, format("%c", c));
	}

	/* exit? */
	if (c == ESCAPE) {
		Term_load();
		return;
	}

	/* display lore (default) or stats */
	while (TRUE) {
		clear_from(5);
		for (i = 0; i < 18; i++) paste_lines[i][0] = '\0';
		if (show_lore) {
			monster_lore_aux(selected, selected_list, paste_lines);
			Term_putstr(11,  23, -1, TERM_WHITE, "-- press ESC to exit, SPACE for stats, c to chat-paste --");
		} else {
			monster_stats_aux(selected, selected_list, paste_lines);
			Term_putstr(11,  23, -1, TERM_WHITE, "-- press ESC to exit, SPACE for lore, c to chat-paste --");
		}

		while (TRUE) {
			c = inkey();
			/* specialty: allow chatting from within here */
	                if (c == ':') {
	            		cmd_message();
				Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Monster Lore ***");
	            	}
			if (c == '\e') break;
			if (c == ' ') {
				show_lore = !show_lore;
				break;
			}
			if (c == 'c') {
				/* paste currently displayed monster information into chat:
				   scan line #5 for title, lines 7..22 for info. */
				for (i = 0; i < 18; i++) {
					if (!paste_lines[i][0]) break;
					//if (i == 6 || i == 12) usleep(10000000);
					if (paste_lines[i][strlen(paste_lines[i]) - 1] == ' ')
						paste_lines[i][strlen(paste_lines[i]) - 1] = '\0';
					Send_paste_msg(paste_lines[i]);
				}
			}
		}
		if (c == '\e') break;
	}
    }

	Term_load();
}


/*
 * NOTE: the usage of Send_special_line is quite a hack;
 * it sends a letter in place of cur_line...		- Jir -
 */
void cmd_check_misc(void)
{
	char i = 0, choice;
	int second = 10;

	Term_save();
	Term_clear();
//	Term_putstr(0,  0, -1, TERM_BLUE, "Display current knowledge");

	Term_putstr( 5,  3, -1, TERM_WHITE, "(\377y1\377w) Artifacts");
	Term_putstr( 5,  4, -1, TERM_WHITE, "(\377y2\377w) Monsters");
	Term_putstr( 5,  5, -1, TERM_WHITE, "(\377y3\377w) Unique monsters");
	Term_putstr( 5,  6, -1, TERM_WHITE, "(\377y4\377w) Objects");
	Term_putstr( 5,  7, -1, TERM_WHITE, "(\377y5\377w) Traps");
	Term_putstr(40,  3, -1, TERM_WHITE, "(\377y6\377w) Artifact lore");
	Term_putstr(40,  4, -1, TERM_WHITE, "(\377y7\377w) Monster lore");
	Term_putstr(40,  5, -1, TERM_WHITE, "(\377y8\377w) Recall depths and towns");
	Term_putstr(40,  6, -1, TERM_WHITE, "(\377y9\377w) Houses");
	Term_putstr(40,  7, -1, TERM_WHITE, "(\377y0\377w) Wilderness map");

	Term_putstr( 5, second + 0, -1, TERM_WHITE, "(\377ya\377w) Players online");
	Term_putstr( 5, second + 1, -1, TERM_WHITE, "(\377yb\377w) Other players' equipments");
	Term_putstr( 5, second + 2, -1, TERM_WHITE, "(\377yc\377w) Score list");
	Term_putstr( 5, second + 3, -1, TERM_WHITE, "(\377yd\377w) Server settings");
	Term_putstr( 5, second + 4, -1, TERM_WHITE, "(\377ye\377w) Opinions (if available)");
	Term_putstr(40, second + 0, -1, TERM_WHITE, "(\377yf\377w) News (Message of the day)");
	Term_putstr(40, second + 1, -1, TERM_WHITE, "(\377yg\377w) Intro screen");
	Term_putstr(40, second + 2, -1, TERM_WHITE, "(\377yh\377w) Message history");
	Term_putstr(40, second + 3, -1, TERM_WHITE, "(\377yi\377w) Chat history");
	Term_putstr(40, second + 4, -1, TERM_WHITE, "(\377yl\377w) Lag-o-meter");
	Term_putstr( 5, 17, -1, TERM_WHITE, "(\377y?\377w) Help");

	Term_putstr(0, 22, -1, TERM_BLUE, "Command: ");

	while (i != ESCAPE) {
		Term_putstr(0,  0, -1, TERM_BLUE, "Display current knowledge");

		i = inkey();
		choice = 0;
		switch(i) {
			case '3':
				/* Send it */
				cmd_uniques();
				break;
			case '1':
				cmd_artifacts();
				break;
			case '2':
				get_com("What kind of monsters? (ESC for all):", &choice);
				if (choice <= ESCAPE) choice = 0;
				Send_special_line(SPECIAL_FILE_MONSTER, choice);
				break;
			case '4':
				get_com("What type of objects? (ESC for all):", &choice);
				if (choice <= ESCAPE) choice = 0;
				Send_special_line(SPECIAL_FILE_OBJECT, choice);
				break;
			case '5':
				Send_special_line(SPECIAL_FILE_TRAP, 0);
				break;
			case '9':
				Send_special_line(SPECIAL_FILE_HOUSE, 0);
				break;
			case '8':
				Send_special_line(SPECIAL_FILE_RECALL, 0);
				break;
			case '0':
				cmd_map(1);
				break;
			case '6':
				artifact_lore();
				break;
			case '7':
				monster_lore();
				break;
			case 'a':
				cmd_players();
				break;
			case 'b':
				cmd_player_equip();
				break;
			case 'c':
				cmd_high_scores();
				break;
			case 'd':
				Send_special_line(SPECIAL_FILE_SERVER_SETTING, 0);
				break;
			case 'e':
				/* Set the hook */
				special_line_type = SPECIAL_FILE_RFE;

				/* Call the file perusal */
				peruse_file();
				break;
			case 'f':
				show_motd(0);
				break;
			case 'g':
				Send_special_line(SPECIAL_FILE_MOTD2, 0);
				break;
			case 'h':
				do_cmd_messages();
				break;
			case 'i':
				do_cmd_messages_chatonly();
				break;
			case 'l':
				cmd_lagometer();
				break;
			case '?':
				cmd_help();
				break;
			case ESCAPE:
			case KTRL('X'):
				break;
			case KTRL('T'):
				xhtml_screenshot("screenshot????");
				break;
			default:
				bell();
		}
		c_msg_print(NULL);
	}
	Term_load();
}

void cmd_message(void)
{
	/* _hacky_: A note INCLUDES the sender name, the brackets, spaces/newlines? Ouch. - C. Blue
	   Note: the -8 are additional world server tax (8 chars are used for world server line prefix etc.) */
	char buf[MSG_LEN - strlen(cname) - 5 - 3 - 8];
	char tmp[MSG_LEN];//, c = 'B';
	int i;
	int j;
	/* for store-item pasting: */
	char where[17], item[MSG_LEN];
	bool store_item = FALSE;

	/* Wipe the whole buffer to stop valgrind from complaining about the color code conversion - mikaelh */
	C_WIPE(buf, sizeof(buf), char);

	inkey_msg = TRUE;

	if (get_string("Message: ", buf, sizeof(buf) - 1)) {
		/* hack - screenshot - mikaelh */
		if (prefix(buf, "/shot") || prefix(buf, "/screenshot")) {
			char *space = strchr(buf, ' ');
			if (space && space[1]) {
				/* Use given parameter as filename */
				xhtml_screenshot(space + 1);
			} else {
				/* Default filename pattern */
				xhtml_screenshot("screenshot????");
			}
			inkey_msg = FALSE;
			return;
		}

		/* Convert {'s into \377's */
		for (i = 0; i < sizeof(buf); i++) {
			if (buf[i] == '{') {
				buf[i] = '\377';

				/* remember last colour; for item pasting below */
//				c = buf[i + 1];
			}
		}

		/* Allow to paste items in chat via shortcut symbol - C. Blue */
		for (i = 0; i < sizeof(buf); i++) {
			/* paste inven/equip item:  \\<slot>  */
			if (buf[i] == '\\' &&
			    buf[i + 1] == '\\' &&
			    ((buf[i + 2] >= 'a' && buf[i + 2] < 'a' + INVEN_PACK) ||
			    (buf[i + 2] >= 'A' && buf[i + 2] < 'A' + (INVEN_TOTAL - INVEN_WIELD)))) {
				if (buf[i + 2] >= 'a') j = buf[i + 2] - 'a';
				else j = (buf[i + 2] - 'A') + INVEN_WIELD;
				strcpy(tmp, &buf[i + 3]);
				strcpy(&buf[i], "\377s");
				strcat(buf, inventory_name[j]);

				if (tmp[0]) {
                			/* if someone just spams \\ shortcuts, at least add spaces */
                    			if (tmp[0] == '\\') strcat(buf, " ");

					strcat(buf, "\377-");
					strncat(buf, tmp, sizeof(buf) - strlen(buf));
				}
			}
			/* paste store item:  \\\<slot>  */
			else if (buf[i] == '\\' && buf[i + 1] == '\\' && buf[i + 2] == '\\' &&
			    buf[i + 3] >= 'a' && buf[i + 3] < 'a' + store.stock_num
			    && buf[i + 3] <= 'l') {
				j = buf[i + 3] - 'a' + store_top;
                    		strcpy(tmp, &buf[i + 4]);

				/* add store location/symbol, only once per chat message */
                                if (!store_item) {
                            		store_paste_where(where);
                            		store_item = TRUE;
                            		strcpy(&buf[i], where);

                            		/* need double-colon to prevent confusing it with a private message? */
                            		if (strchr(buf, ':') >= &buf[i + strlen(where) - 1])
                                		strcat(buf, ":");

                                	strcat(buf, " ");
                            	} else strcpy(&buf[i], "\377s");

				/* add actual item */
                    		store_paste_item(item, j);
                    		strcat(buf, item);

                    		/* any more string to append? */
                    		if (tmp[0]) {
                			/* if someone just spams \\\ shortcuts, at least add spaces */
                    			if (tmp[0] == '\\') strcat(buf, " ");

                        		strcat(buf, "\377-");
	                		strncat(buf, tmp, sizeof(buf) - strlen(buf));
				}
                                buf[MSG_LEN - 1] = 0;
			}
		}

		/* Handle messages to '%' (self) in the client - mikaelh */
		if (prefix(buf, "%:") && !prefix(buf, "%::"))
		{
			c_msg_format("\377o<%%> \377w%s", buf + 2);
			inkey_msg = FALSE;
			return;
		}

		Send_msg(buf);
	}

	inkey_msg = FALSE;
}

/* set flags and options such as minlvl / auto-rejoin */
static void cmd_guild_options() {

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
		Term_putstr(0, 0, -1, TERM_L_GREEN, "============== Party/Guild commands ==============");

		/* Selections */
		/* See also: Receive_party() */
		Term_putstr(5, 2, -1, TERM_WHITE, "(\377G1\377w) Create or rename a party");
		Term_putstr(5, 3, -1, TERM_WHITE, "(\377G2\377w) Create or rename an 'Iron Team' party");
		Term_putstr(5, 4, -1, TERM_WHITE, "(\377G3\377w) Add a player to party");
		Term_putstr(5, 5, -1, TERM_WHITE, "(\377G4\377w) Delete a player from party");
		Term_putstr(5, 6, -1, TERM_WHITE, "(\377G5\377w) Leave your current party");
		Term_putstr(5, 8, -1, TERM_WHITE, "(\377Ua\377w) Create a new guild");
		Term_putstr(5, 9, -1, TERM_WHITE, "(\377Ub\377w) Add player to guild");
		Term_putstr(5, 10, -1, TERM_WHITE, "(\377Uc\377w) Remove player from guild");
		Term_putstr(5, 11, -1, TERM_WHITE, "(\377Ud\377w) Leave guild");
//		Term_putstr(5, 12, -1, TERM_WHITE, "(\377Ue\377w) Set guild options");
		Term_putstr(5, 14, -1, TERM_WHITE, "(\377RA\377w) Declare war on player (not recommended!)");
		Term_putstr(5, 15, -1, TERM_WHITE, "(\377gP\377w) Make peace with player");

		/* Show current party status */
		if (is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
			if (strlen(party_info_name)) Term_putstr(0, 21, -1, TERM_WHITE, format("%s (%s, %s)", party_info_name, party_info_members, party_info_owner));
			else Term_putstr(0, 21, -1, TERM_SLATE, "(You are not in a party.)");
			if (strlen(guild_info_name)) Term_putstr(0, 22, -1, TERM_WHITE, format("%s (%s, %s)", guild_info_name, guild_info_members, guild_info_owner));
			else Term_putstr(0, 22, -1, TERM_SLATE, "(You are not in a guild.)");
		} else {
			Term_putstr(0, 20, -1, TERM_WHITE, party_info_name);
			Term_putstr(0, 21, -1, TERM_WHITE, party_info_members);
			Term_putstr(0, 22, -1, TERM_WHITE, party_info_owner);
		}

		/* Prompt */
		Term_putstr(0, 18, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE || i == KTRL('X')) break;

		/* Take a screenshot */
		else if (i == KTRL('T'))
		{
			xhtml_screenshot("screenshot????");
		}

		/* Create party */
		else if (i == '1')
		{
			/* Get party name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Enter party name: ");
			if (askfor_aux(buf, 79, 0)) Send_party(PARTY_CREATE, buf);
		}

		/* Create 'Iron Team' party */
		else if (i == '2')
		{
			/* Get party name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Enter party name: ");
			if (askfor_aux(buf, 79, 0)) Send_party(PARTY_CREATE_IRONTEAM, buf);
		}

		/* Add player */
		else if (i == '3')
		{
			/* Get player name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Enter player name: ");
			if (askfor_aux(buf, 79, 0)) Send_party(PARTY_ADD, buf);
		}

		/* Delete player */
		else if (i == '4')
		{
			/* Get player name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Enter player name: ");
			if (askfor_aux(buf, 79, 0)) Send_party(PARTY_DELETE, buf);
		}

		/* Leave party */
		else if (i == '5')
		{
			/* Send the command */
			Send_party(PARTY_REMOVE_ME, "");
		}

		/* Attack player/party */
		else if (i == 'A')
		{
			/* Get player name */
			Term_putstr(0, 19, -1, TERM_L_RED, "Enter player/party to attack: ");
			if (askfor_aux(buf, 79, 0)) Send_party(PARTY_HOSTILE, buf);
		}

		/* Make peace with player/party */
		else if (i == 'P')
		{
			/* Get player/party name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Make peace with: ");
			if (askfor_aux(buf, 79, 0)) Send_party(PARTY_PEACE, buf);
		}
		else if (i == 'a'){
			/* Get new guild name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Enter guild name: ");
			if (askfor_aux(buf, 79, 0)) Send_guild(GUILD_CREATE, buf);
		}
		else if (i == 'b'){
			/* Get player name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Enter player name: ");
			if (askfor_aux(buf, 79, 0)) Send_guild(GUILD_ADD, buf);
		}
		else if (i == 'c'){
			/* Get player name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Enter player name: ");
			if (askfor_aux(buf, 79, 0)) Send_guild(GUILD_DELETE, buf);
		}
		else if (i == 'd'){
			if (get_check("Leave the guild ? "))
				Send_guild(GUILD_REMOVE_ME, "");
		}
		else if (i == 'e'){
			/* Set guild flags/options */
			cmd_guild_options();
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
	int dir;

	if (!get_dir(&dir))
		return;

	/* Send it */
	Send_fire(dir);
}

void cmd_throw(void)
{
	int item, dir;

	if (!c_get_item(&item, "Throw what? ", (USE_INVEN)))
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
	return (is_realm_book(o_ptr) || o_ptr->tval == TV_BOOK);
}

void cmd_browse(void)
{
	int item;
	object_type *o_ptr;

/* commented out because first, admins are usually ghosts;
   second, we might want a 'ghost' tome or something later,
   kind of to bring back 'undead powers' :) - C. Blue */
#if 0
	if (p_ptr->ghost)
	{
		show_browse(NULL);
		return;
	}
#endif

	item_tester_hook = item_tester_browsable;

	if (!c_get_item(&item, "Browse which book? ", (USE_INVEN)))
	{
		if (item == -2) c_msg_print("You have no books that you can read.");
		return;
	}

	o_ptr = &inventory[item];

	if (o_ptr->tval == TV_BOOK)
	{
		browse_school_spell(item, o_ptr->sval, o_ptr->pval);
		return;
	}

	/* Show it */
	show_browse(o_ptr);
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

#if 0 /* done by cmd_telekinesis */
void cmd_mind(void)
{
	Send_mind();
}
#endif

void cmd_load_pref(void)
{
	char buf[80];

	buf[0] = '\0';

	if (get_string("Load pref: ", buf, 79))
		process_pref_file_aux(buf);
}

void cmd_redraw(void)
{
	Send_redraw(0);

#if 0 /* I think this is useless here - mikaelh */
	keymap_init();
#endif
}

static void cmd_house_chown(int dir)
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
				if(get_string("Enter new name:",&buf[2],78))
					Send_admin_house(dir,buf);
				return;
			case ESCAPE:
			case KTRL('X'):
				break;
			case KTRL('T'):
				xhtml_screenshot("screenshot????");
				break;
			default:
				bell();
		}
		c_msg_print(NULL);
	}
}

static void cmd_house_chmod(int dir){
	char buf[80];
	char mod=ACF_NONE;
	u16b minlev=0;
	Term_clear();
	Term_putstr(0, 2, -1, TERM_BLUE, "Set new permissions");
	if (get_check("Allow party access? ")) mod |= ACF_PARTY;
	if (get_check("Restrict access to class? ")) mod |= ACF_CLASS;
	if (get_check("Restrict access to race? ")) mod |= ACF_RACE;
	if (get_check("Restrict access to winners? ")) mod |= ACF_WINNER;
	if (get_check("Restrict access to fallen winners? ")) mod |= ACF_FALLENWINNER;
	if (get_check("Restrict access to no-ghost players? ")) mod |= ACF_NOGHOST;
	minlev=c_get_quantity("Minimum level: ", 127);
	if(minlev>1) mod |= ACF_LEVEL;
	buf[0]='M';
	if((buf[1]=mod))
		sprintf(&buf[2],"%hd",minlev);
	Send_admin_house(dir,buf);
}

static void cmd_house_kill(int dir){
	if (get_check("Are you sure you really want to destroy the house?"))
		Send_admin_house(dir, "K");
}

static void cmd_house_store(int dir){
	Send_admin_house(dir, "S");
}

static void cmd_house_paint(int dir){
	char buf[80];
	int item;

	item_tester_hook = item_tester_quaffable;
	get_item_hook_find_obj_what = "Potion name? ";
	get_item_extra_hook = get_item_hook_find_obj;
	if (!c_get_item(&item, "Use which potion for colouring? ", (USE_INVEN | USE_EXTRA))) {
		c_msg_print("You need a potion to extract the colour from!");
		return;
	}

	buf[0] = 'P';
	sprintf(&buf[1],"%hd", item);
	Send_admin_house(dir, buf);
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
	/* display in dark colour since only admins can do this really */
	Term_putstr(5, 7, -1, TERM_SLATE, "(4) Delete house");
	/* new in 4.4.6: */
	Term_putstr(5, 8, -1, TERM_WHITE, "(5) Enter player store");
	Term_putstr(5, 9, -1, TERM_WHITE, "(6) Paint house");

	while(i!=ESCAPE){
		i=inkey();
		switch(i){
			case '1':
				/* Confirm */
				if (get_check("Are you sure you really want to buy or sell the house?")) {
					/* Send it */
					Send_purchase_house(dir);
					i=ESCAPE;
				}
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
			case '5':
				cmd_house_store(dir);
				i=ESCAPE;
				break;
			case '6':
				cmd_house_paint(dir);
				i=ESCAPE;
				break;
			case ESCAPE:
			case KTRL('X'):
				break;
			case KTRL('T'):
				xhtml_screenshot("screenshot????");
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

static void cmd_master_aux_level(void)
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

		/* Take a screenshot */
		else if (i == KTRL('T')) xhtml_screenshot("screenshot????");
		/* static the current level */
		else if (i == '1') Send_master(MASTER_LEVEL, "s");
		/* unstatic the current level */
		else if (i == '2') Send_master(MASTER_LEVEL, "u");
		else if (i == '3'){	/* create dungeon stair here */
			buf[0] = 'D';
			buf[4] = 0x01;//hack: avoid 0 byte
			buf[5] = 0x01;//hack: avoid 0 byte
			buf[6] = 0x01;//hack: avoid 0 byte
			buf[1] = c_get_quantity("Base level: ", 127);
			buf[2] = c_get_quantity("Max depth (1-127): ", 127);
			buf[3] = (get_check("Is it a tower? ") ? 't' : 'd');
			/*
			 * FIXME: flags are u32b while buf[] is char!
			 * This *REALLY* should be rewritten	- Jir -
			 */
			if (get_check("Random dungeon (default)?")) buf[5] |= 0x02;//DF2_RANDOM
			if (get_check("Hellish?")) buf[5] |= 0x04;//DF2_HELL
			if (get_check("Not mappable?")) {
				buf[4] |= 0x02;//DF1_FORGET
				buf[5] |= 0x08;//DF2_NO_MAGIC_MAP
			}
			if (get_check("Ironman?")) {
				buf[5] |= 0x10;//DF2_IRON
				i = 0;
				if (get_check("Recallable from, before reaching its end?")) {
					if (get_check("Random recall depth intervals (y) or fixed ones (n) ?")) buf[6] |= 0x08;
					i = c_get_quantity("Frequency (random)? (1=often..4=rare): ", 4);
					switch (i) {
					case 1: buf[6] |= 0x10; break;//DF2_IRONRNDn / DF2_IRONFIXn
					case 2: buf[6] |= 0x20; break;
					case 3: buf[6] |= 0x40; break;
					default: buf[6] |= 0x80;
					}
					i = 1; // hack for towns below
				}
				if (get_check("Generate towns inbetween?")) {
					if (i == 1 && get_check("Generate towns when premature recall is allowed?")) {
						buf[5] |= 0x20;//DF2_TOWNS_IRONRECALL
					} else if (get_check("Generate towns randomly (y) or in fixed intervals (n) ?")) {
						buf[5] |= 0x40;//DF2_TOWNS_RND
					} else buf[5] |= 0x80;//DF2_TOWNS_FIX
				}
			}
			if (get_check("Disallow generation of simple stores (misc iron + low level)?")) {
				buf[4] |= 0x08;//DF3_NO_SIMPLE_STORES
				if (get_check("Generate at least the hidden library?")) buf[4] |= 0x04;//DF3_HIDDENLIB
			} else if (get_check("Generate misc iron stores (RPG rules style)?")) {
				buf[6] |= 0x04;//DF2_MISC_STORES
			} else if (get_check("Generate at least the hidden library?")) buf[4] |= 0x04;//DF3_HIDDENLIB
			buf[7] = '\0';
			Send_master(MASTER_LEVEL, buf);
		}
		else if (i == '4'){
			buf[0] = 'R';
			buf[1] = '\0';
			Send_master(MASTER_LEVEL, buf);
		}
		else if (i == '5'){
			buf[0] = 'T';
			buf[1] = c_get_quantity("Base level: ", 127);
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

static void cmd_master_aux_generate_vault(void)
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

		/* Take a screenshot */
		if (i == KTRL('T'))
		{
			xhtml_screenshot("screenshot????");
		}

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

static void cmd_master_aux_generate(void)
{
	char i;

	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

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

		/* Take a screenshot */
		else if (i == KTRL('T'))
		{
			xhtml_screenshot("screenshot????");
		}

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



static void cmd_master_aux_build(void)
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
		Term_putstr(5, 13, -1, TERM_WHITE, "(0) Any feature");

		Term_putstr(5, 15, -1, TERM_WHITE, "(a) Build Mode Off");

		/* Prompt */
		Term_putstr(0, 18, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		buf[1] = 'T';
		buf[2] = '\0';

		switch (i)
		{
			/* Take a screenshot */
			case KTRL('T'):
				xhtml_screenshot("screenshot????");
				break;
			/* Granite mode on */
			case '1': buf[0] = FEAT_WALL_EXTRA; break;
			/* Perm mode on */
			case '2': buf[0] = FEAT_PERM_EXTRA; break;
			/* Tree mode on */
			case '3': buf[0] = magik(80)?FEAT_TREE:FEAT_BUSH; break;
			/* Evil tree mode on */
			case '4': buf[0] = FEAT_DEAD_TREE; break;
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
			/* Ask for feature */
			case '0':
				buf[0] = c_get_quantity("Enter feature value:",0xff);
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

static char * cmd_master_aux_summon_orcs(void)
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
			case KTRL('T'):
				xhtml_screenshot("screenshot????");
				break;
			case '1': strcpy(buf,"Snaga"); break;
			case '2': strcpy(buf,"Cave orc"); break;
			case '3': strcpy(buf,"Hill orc"); break;
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

static char * cmd_master_aux_summon_undead_low(void)
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
			case KTRL('T'):
				xhtml_screenshot("screenshot????");
				break;
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


static char * cmd_master_aux_summon_undead_high(void)
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
			case KTRL('T'):
				xhtml_screenshot("screenshot????");
				break;
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


static void cmd_master_aux_summon(void)
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
		Term_putstr(5, 10, -1, TERM_WHITE, "(7) Summoning mode off");



		/* Prompt */
		Term_putstr(0, 13, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		/* get the type of monster to summon */
		switch (i)
		{
			/* Take a screenshot */
			case KTRL('T'):
			{
				xhtml_screenshot("screenshot????");
				break;
			}
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
				/* if (!buf[3]) redo_hack = 1; - Allow depth 0 hereby. */
				buf[4] = 0; /* terminate the string */
				break;
			}
			/* summon a specific monster or character */
			case '5':
			{
				buf[2] = 's';
				buf[3] = 0;
				get_string("Summon which monster or character? ", &buf[3], 79 - 3);
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
				case KTRL('T'):
					xhtml_screenshot("screenshot????");
					break;
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

static void cmd_master_aux_player()
{
	char i=0;
	static char buf[80];
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
		buf[0] = '\0';
		switch(i){
			case KTRL('T'):
				xhtml_screenshot("screenshot????");
				break;
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

/*
 * Upload/execute scripts
 */
/* TODO: up-to-date check and download facility */
static void cmd_script_upload()
{
	char name[81];

	name[0]='\0';

	if (!get_string("Script name: ", name, 30)) return;

	remote_update(0, name);
}

/*
 * Upload/execute scripts
 */
static void cmd_script_exec()
{
	char buf[81];

	buf[0]='\0';
	if (!get_string("Script> ", buf, 80)) return;

	Send_master(MASTER_SCRIPTS, buf);
}

static void cmd_script_exec_local()
{
	char buf[81];

	buf[0]='\0';
	if (!get_string("Script> ", buf, 80)) return;

	exec_lua(0, buf);
}


/* Dirty implementation.. FIXME		- Jir - */
static void cmd_master_aux_system()
{
	char i=0;

	while(i!=ESCAPE){
		Term_clear();
		Term_putstr(0, 2, -1, TERM_BLUE, "System commands");
		Term_putstr(5, 4, -1, TERM_WHITE, "(1) View tomenet.log");
		Term_putstr(5, 5, -1, TERM_WHITE, "(2) View tomenet.rfe");
		Term_putstr(5, 7, -1, TERM_WHITE, "(e) Execute script command");
		Term_putstr(5, 8, -1, TERM_WHITE, "(u) Upload script file");
		Term_putstr(5, 9, -1, TERM_WHITE, "(c) Execute local script command");

		Term_putstr(0, 12, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();
		switch(i){
			case KTRL('T'):
				xhtml_screenshot("screenshot????");
				break;
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
			case 'e':
				cmd_script_exec();
				break;
			case 'u':
				cmd_script_upload();
				break;
			case 'c':
				cmd_script_exec_local();
				break;
			case ESCAPE:
			case KTRL('X'):
				break;
			default:
				bell();
		}

		/* Flush messages */
		c_msg_print(NULL);
	}
}

/* Dungeon Master commands */
static void cmd_master(void)
{
	char i=0;

	party_mode = TRUE;

	/* Save screen */
	Term_save();

	/* Process requests until done */
	while (i!=ESCAPE)
	{
		/* Clear screen */
		Term_clear();

		/* Describe */
		Term_putstr(0, 2, -1, TERM_WHITE, "Dungeon Master commands");

		/* Selections */
		Term_putstr(5, 4, -1, TERM_WHITE, "(1) Level Commands");
		Term_putstr(5, 5, -1, TERM_WHITE, "(2) Building Commands");
		Term_putstr(5, 6, -1, TERM_WHITE, "(3) Summoning Commands");
		Term_putstr(5, 7, -1, TERM_WHITE, "(4) Generation Commands");
		Term_putstr(5, 8, -1, TERM_WHITE, "(5) Player Commands");
		Term_putstr(5, 9, -1, TERM_WHITE, "(6) System Commands");

		/* Prompt */
		Term_putstr(0, 14, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		switch(i){
			case KTRL('T'):
				xhtml_screenshot("screenshot????");
				break;
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
			case ESCAPE:
			case KTRL('X'):
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
		if (!get_dir(&dir)) return;
	}

	Send_spike(dir);
}

/*
 * formality :)
 * Nice if we can use LUA hooks here..
 */
void cmd_raw_key(int key)
{
	/* Send it */
	Send_raw_key(key);
}

void cmd_sip() {
	Send_sip();
}

void cmd_BBS() {
	Send_BBS();
}

void cmd_telekinesis() {
	Send_telekinesis();
}

void cmd_cloak() {
	Send_cloak();
}

void cmd_lagometer(void)
{
	int k;

	/* Save the screen */
	Term_save();

	lagometer_open = TRUE;

	/* Interact */
	while (1)
	{
		display_lagometer(TRUE);

		/* Get command */
		k = inkey();

		/* Exit */
		if (k == ESCAPE || k == KTRL('X') || k == KTRL('I')) break;

		switch (k) {
			/* Take a screenshot */
			case KTRL('T'):
				xhtml_screenshot("screenshot????");
				break;

			/* Enable */
			case '1':
				lagometer_enabled = TRUE;
				break;

			/* Disable */
			case '2':
				/* disable mini lag-o-meter too */
				prt_lagometer(-1);

				lagometer_enabled = FALSE;
				break;
		}
	}

	lagometer_open = FALSE;

	/* Restore the screen */
	Term_load();

	Flush_queue();
}

void cmd_force_stack()
{
	int item;
	if (!c_get_item(&item, "Forcibly stack what? ", USE_INVEN)) return;
	Send_force_stack(item);
}
