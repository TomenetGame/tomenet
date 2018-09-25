/* $Id$ */
#include "angband.h"

/* reordering will allow these to be removed */
static void cmd_clear_buffer(void);
static void cmd_master(void);

static void cmd_clear_actions(void) {
	Send_clear_actions();
}

static void cmd_player_equip(void) {
	/* Set the hook */
	special_line_type = SPECIAL_FILE_PLAYER_EQUIP;

	/* Call the file perusal */
	peruse_file();
}

static bool item_tester_edible(object_type *o_ptr) {
	if (o_ptr->tval == TV_FOOD) return TRUE;
	if (o_ptr->tval == TV_FIRESTONE) return TRUE;

	return FALSE;
}

/* XXX not fully functional */
static bool item_tester_oils(object_type *o_ptr) {
	if (o_ptr->tval == TV_LITE) return TRUE;
	if (o_ptr->tval == TV_FLASK) return TRUE;

	return FALSE;
}

static void cmd_all_in_one(void) {
	int item, dir;

	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Use which item? ", (USE_EQUIP | USE_INVEN | USE_EXTRA)))
		return;

	if (INVEN_WIELD <= item) {
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

	switch (inventory[item].tval) {
	case TV_POTION:
	case TV_POTION2:
		Send_quaff(item);
		break;
	case TV_SCROLL:
	case TV_PARCHMENT:
		Send_read(item);
		break;
	case TV_WAND:
		if (!get_dir(&dir)) return;
		Send_aim(item, dir);
		break;
	case TV_STAFF:
		Send_use(item);
		break;
	case TV_ROD:
		/* Does rod require aiming? (Always does if not yet identified) */
		if (inventory[item].uses_dir == 0) {
			/* (also called if server is outdated, since uses_dir will be 0 then) */
			Send_zap(item);
		} else {
			if (!get_dir(&dir)) return;
			Send_zap_dir(item, dir);
		}
		break;
	case TV_LITE:
		if (inventory[INVEN_LITE].tval)
			Send_fill(item);
		else
			Send_wield(item);
		break;
	case TV_FLASK:
		Send_fill(item);
		break;
	case TV_FOOD:
	case TV_FIRESTONE:
		Send_eat(item);
		break;
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
		Send_wield(item);
		break;
	/* NOTE: 'item' isn't actually sent */
	case TV_SPIKE:
		if (!get_dir(&dir)) return;
		Send_spike(dir);
		break;
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
		Send_wield(item);
		break;
	case TV_TRAPKIT:
		do_trap(item);
		break;
	case TV_RUNE:
		/* (Un)Combine it */
		Send_activate(item);
		break;
	/* Presume it's sort of spellbook */
	case TV_BOOK:
	default:
	{
		int i;
		bool done = FALSE;

		for (i = 1; i < MAX_SKILLS; i++) {
			if (s_info[i].tval == inventory[item].tval &&
			    s_info[i].action_mkey && p_ptr->s_info[i].value) {
				do_activate_skill(i, item);
				done = TRUE;
				break;
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

void process_command() {
	/* Parse the command */
	switch (command_cmd) {
	/* Ignore */
	case ESCAPE:
	case '-': /* used for targetting since 4.4.6, so we ignore it to allow
		     macros that hackily use it for both directional and non-
		     directional things, so the 'key not in use' message is
		     suppressed for non-directional actions, just to look better. */
		break;
	/* Ignore mostly, but also stop automatically repeated actions */
	case ' ': cmd_clear_actions(); break;
	/* Ignore return */
	case '\r': break;
	/* Clear buffer */
	case ')': cmd_clear_buffer(); break;

	/*** Movement Commands ***/

	/* Dig a tunnel*/
	case '+': cmd_tunnel(); break;
	/* Move */
	case ';': cmd_walk(); break;
	/*** Running, Staying, Resting ***/
	case '.': cmd_run(); break;
	case ',':
	case 'g': cmd_stay(); break;
	case KTRL('G'): cmd_stay_one(); break;

	/* Get the mini-map */
	case 'M': cmd_map(0); break;
	/* Recenter map */
	case 'L': cmd_locate(); break;
	/* Search */
	case 's': cmd_search(); break;
	/* Toggle Search Mode */
	case 'S': cmd_toggle_search(); break;
	/* Rest */
	case 'R': cmd_rest(); break;

	/*** Stairs and doors and chests ***/

	/* Go up */
	case '<': cmd_go_up(); break;
	/* Go down */
	case '>': cmd_go_down(); break;

	/* Open a door */
	case 'o': cmd_open(); break;
	/* Close a door */
	case 'c': cmd_close(); break;
	/* Bash a door */
	case 'B': cmd_bash(); break;

	/* Disarm a trap or chest */
	case 'D': cmd_disarm(); break;

	/*** Inventory commands ***/

	case 'i': cmd_inven(); break;
	case 'e': cmd_equip(); break;
	case 'd': cmd_drop(USE_INVEN | USE_EQUIP); break;
	case '$': cmd_drop_gold(); break;
	case 'w': cmd_wield(); break;
	case 't': cmd_take_off(); break;
	case 'x': cmd_swap(); break;
	case 'k': cmd_destroy(USE_INVEN | USE_EQUIP); break;
#if 0 /* currently no effect on server-side */
	case 'K': cmd_king(); break;
#endif
	case 'K': cmd_force_stack(); break;
	case '{': cmd_inscribe(USE_INVEN | USE_EQUIP); break;
	case '}': cmd_uninscribe(USE_INVEN | USE_EQUIP); break;
	case 'H': cmd_apply_autoins(); break;
	case 'j': cmd_steal(); break;

	/*** Inventory "usage" commands ***/

	case 'q': cmd_quaff(); break;
	case 'r': cmd_read_scroll(); break;
	case 'a': cmd_aim_wand(); break;
	case 'u': cmd_use_staff(); break;
	case 'z': cmd_zap_rod(); break;
	case 'F': cmd_refill(); break;
	case 'E': cmd_eat(); break;
	case 'A': cmd_activate(); break;

	/*** Firing and throwing ***/

	case 'f': cmd_fire(); break;
	case 'v': cmd_throw(); break;

	/*** Spell casting ***/

	case 'b': cmd_browse(); break;
	case 'G': do_cmd_skill(); break;
	case 'm': do_cmd_activate_skill(); break;
	case 'U': cmd_ghost(); break;

	/*** Looking/Targetting ***/

	case '*': cmd_target(); break;
	case '(': cmd_target_friendly(); break;
	case 'l': cmd_look(); break;
	case 'I': cmd_observe(USE_INVEN | USE_EQUIP); break;

	/*** Information ***/

	case 'C': cmd_character(); break;
	case '~': cmd_check_misc(); break;
	case '|': cmd_uniques(); break;
	case '\'': cmd_player_equip(); break;
	case '@': cmd_players(); break;
	case '#': cmd_high_scores(); break;
	case '?': cmd_help(); break;

	/*** Miscellaneous ***/

	case ':': cmd_message(); break;
	case 'P': cmd_party(); break;

	/*** Additional/unsorted stuff */

	case '_': cmd_sip(); break;
	case 'p': cmd_telekinesis(); break;
	case 'W': cmd_wield2(); break;
	case 'V': cmd_cloak(); break;

	case ']':
		/* Dungeon master commands, normally only accessible to
		 * a valid dungeon master.  These commands only are
		 * effective for a valid dungeon master.
		 */
		/*
		   if (!strcmp(nick,DUNGEON_MASTER)) cmd_master();
		   else prompt_topline("Hit '?' for help.");
		   */
		cmd_master();
		break;

	/* Add separate buffer for chat only review (good for afk) -Zz */
	case KTRL('O'): do_cmd_messages_chatonly(); break;
	case KTRL('P'): do_cmd_messages(); break;

	case KTRL('Q'):
#ifdef RETRY_LOGIN
		rl_connection_destructible = TRUE;
		rl_connection_state = 3;
#endif
		Net_cleanup(); quit(NULL);
#ifdef RETRY_LOGIN
		return; //why aren't all of these breaks returns? :-p
#endif
	case KTRL('R'): cmd_redraw(); break;
	case 'Q': cmd_suicide(); break;
	case '=': do_cmd_options(); break;
	case '\"':
		cmd_load_pref();
		/* Resend options to server */
		Send_options();
		break;
	case '%': interact_macros(); break;
	case '&': auto_inscriptions(); break;
	case 'h': cmd_purchase_house(); break;
	case '/': cmd_all_in_one(); break;
	case KTRL('S'): cmd_spike(); break;
	case KTRL('T'): xhtml_screenshot("screenshot????"); break;
	case KTRL('I'): cmd_lagometer(); break;

#ifdef USE_SOUND_2010
	case KTRL('U'): interact_audio(); break;
	//case KTRL('M'): //this is same as '\r' and hence doesn't work..
	case KTRL('C'): toggle_music(FALSE); break;
	case KTRL('N'): toggle_master(FALSE); break;
#endif

	case '!': cmd_BBS(); break;
#if 0 /* only for debugging purpose - dump some client-side special config */
	case KTRL('C'): c_msg_format("Client FPS: %d", cfg_client_fps); break;
#endif
	default: cmd_raw_key(command_cmd); break;
	}
}




static void cmd_clear_buffer(void) {
	/* Hack -- flush the key buffer */
	Send_clear_buffer();
}



void cmd_tunnel(void) {
	int dir = command_dir;

	if (!dir && !get_dir(&dir)) return;

	Send_tunnel(dir);
}

void cmd_walk(void) {
	int dir = command_dir;

	if (!dir && !get_dir(&dir)) return;

	Send_walk(dir);
}

void cmd_run(void) {
	int dir = command_dir;

	if (!dir && !get_dir(&dir)) return;

	Send_run(dir);
}

void cmd_stay(void) {
	Send_stay();
}
void cmd_stay_one(void) {
	Send_stay_one();
}

/* Mode:
   0 = current floor or worldmap
   1 = force worldmap
*/
void cmd_map(char mode) {
	char ch, dir = 0;
	bool sel = FALSE;

	/* Hack -- if the screen is already icky, ignore this command */
	if (screen_icky && !mode) return;

	/* Save the screen */
	Term_save();

	/* Reset grid selection if any */
	//Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
	minimap_selx = -1;

	while (TRUE) {
		/* Send the request */
		Send_map(mode + dir);

		/* Reset the line counter */
		last_line_info = 0;

		while (TRUE) {
			/* Remember the grid below our grid-selection marker */
			if (sel) {
				minimap_selattr = Term->scr->a[minimap_sely][minimap_selx];
				minimap_selchar = Term->scr->c[minimap_sely][minimap_selx];
			}

			/* Wait until we get the whole thing */
			if (last_line_info == 23 + HGT_PLUS) {
				/* Abuse this for drawing selection cursor */
				int wx, wy;
				if (minimap_selx != -1) {
					wx = p_ptr->wpos.wx + minimap_selx - minimap_posx;
					wy = p_ptr->wpos.wy - minimap_sely + minimap_posy;
					Term_putstr(0, 9, -1, TERM_WHITE, " Select");
					Term_putstr(0, 10, -1, TERM_WHITE, format("(%2d,%2d)", wx, wy));
				} else {
					Term_putstr(0, 9, -1, TERM_WHITE, "        ");
					Term_putstr(0, 10, -1, TERM_WHITE, "        ");
				}

				/* Hack - Get rid of the cursor - mikaelh */
				Term->scr->cx = Term->wid;
				Term->scr->cu = 1;
			}

			/* Wait for net input, or a key */
			ch = inkey();

			/* Allow to chat, to tell exact macros to other people easily */
			if (ch == ':') cmd_message();
			/* Take a screenshot */
			else if (ch == KTRL('T')) xhtml_screenshot("screenshot????");
			/* locate map (normal / rogue-like keys) */
			else if (ch == '2' || ch == 'j') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_sely < CL_WINDOW_HGT - 2) minimap_sely++;
					continue;
				}
				dir = 0x2;
				break;
			} else if (ch == '6' || ch == 'l') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_selx < 8 + 64 - 1) minimap_selx++;
					continue;
				}
				dir = 0x4;
				break;
			} else if (ch == '8' || ch == 'k') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_sely > 1) minimap_sely--;
					continue;
				}
				dir = 0x8;
				break;
			} else if (ch == '4' || ch == 'h') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_selx > 8) minimap_selx--;
					continue;
				}
				dir = 0x10;
				break;
			} else if (ch == '1' || ch == 'b') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_sely < CL_WINDOW_HGT - 2) minimap_sely++;
					if (minimap_selx > 8) minimap_selx--;
					continue;
				}
				dir = 0x2 | 0x10;
				break;
			} else if (ch == '3' || ch == 'n') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_sely < CL_WINDOW_HGT - 2) minimap_sely++;
					if (minimap_selx < 8 + 64 - 1) minimap_selx++;
					continue;
				}
				dir = 0x2 | 0x4;
				break;
			} else if (ch == '9' || ch == 'u') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_sely > 1) minimap_sely--;
					if (minimap_selx < 8 + 64 - 1) minimap_selx++;
					continue;
				}
				dir = 0x8 | 0x10;
				break;
			} else if (ch == '7' || ch == 'y') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_sely > 1) minimap_sely--;
					if (minimap_selx > 8) minimap_selx--;
					continue;
				}
				dir = 0x8 | 0x4;
				break;
			} else if (ch == '5' || ch == ' ') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					minimap_selx = minimap_posx;
					minimap_sely = minimap_posy;
					minimap_selattr = minimap_attr;
					minimap_selchar = minimap_char;
					continue;
				}
				dir = 0;
				break;
			}
			/* manual grid selection on the map */
			else if (ch == 's') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					sel = FALSE;
					minimap_selx = -1;
				} else {
					sel = TRUE;
					minimap_selx = minimap_posx;
					minimap_sely = minimap_posy;
					minimap_selattr = minimap_attr;
					minimap_selchar = minimap_char;
				}
			} else if (ch == 'r' && sel) {
				Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
				minimap_selx = minimap_posx;
				minimap_sely = minimap_posy;
				minimap_selattr = minimap_attr;
				minimap_selchar = minimap_char;
			} else if (ch == ESCAPE && sel == TRUE) {
				Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
				sel = FALSE;
				minimap_selx = -1;
			}
			/* user abort? */
			else if (ch == ESCAPE || ch == 'M') break;
#ifdef USE_SOUND_2010
			else if (ch == KTRL('C')) toggle_music(FALSE);
			else if (ch == KTRL('N')) toggle_master(FALSE);
			else if (ch == KTRL('X')) toggle_music(FALSE);//rl
			else if (ch == KTRL('V')) toggle_master(FALSE);//rl
#endif

			continue;
		}

		/* Reset position of our own '@' */
		minimap_posx = -1;

		/* user abort? */
		if (ch == ESCAPE || ch == 'M') break;

		/* re-retrieve map */
		continue;
	}

	/* Reload the screen */
	Term_load();

	/* Flush any queued events */
	Flush_queue();
}

void cmd_locate(void) {
	int dir;
	char ch;

	/* Initialize */
	Send_locate(5);

	/* Show panels until done */
	while (1) {
		/* Assume no direction */
		dir = 0;

		/* Get a direction */
		while (!dir) {
			/* Get a command (or Cancel) */
			ch = inkey();

			/* Check for cancel */
			if (ch == ESCAPE || ch == ' ' || ch == (c_cfg.rogue_like_commands ? 'W' : 'L')) break;

			/* Take a screenshot */
			if (ch == KTRL('T'))
				xhtml_screenshot("screenshot????");

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
	clear_topline_forced();
}

void cmd_search(void) {
	Send_search();
}

void cmd_toggle_search(void) {
	Send_toggle_search();
}

void cmd_rest(void) {
	Send_rest();
}

void cmd_go_up(void) {
	Send_go_up();
}

void cmd_go_down(void) {
	Send_go_down();
}

void cmd_open(void) {
	int dir = command_dir;

	if (!dir && !get_dir(&dir)) return;

	Send_open(dir);
}

void cmd_close(void) {
	int dir = command_dir;

	if (!dir && !get_dir(&dir)) return;

	Send_close(dir);
}

void cmd_bash(void) {
	int dir = command_dir;

	if (!dir && !get_dir(&dir)) return;

	Send_bash(dir);
}

void cmd_disarm(void) {
	int dir = command_dir;

	if (!dir && !get_dir(&dir)) return;

	Send_disarm(dir);
}

void cmd_inven(void) {
	char ch; /* props to 'potato' and the korean team from sarang.net for the idea - C. Blue */
	int c;
	char buf[MSG_LEN];

	showing_inven = TRUE;

	/* First, erase our current location */

	/* Then, save the screen */
	Term_save();

	command_gap = 50;

	show_inven();

	while (TRUE) {
		ch = inkey();
		if (ch == KTRL('T')) {
			xhtml_screenshot("screenshot????");
			break;
		}
		/* allow pasting item into chat */
		else if (ch >= 'A' && ch < 'A' + INVEN_PACK) { /* using capital letters to force SHIFT key usage, less accidental spam that way probably */
			c = ch - 'A';

			if (inventory[c].tval) {
				snprintf(buf, MSG_LEN - 1, "\377s%s", inventory_name[c]);
				buf[MSG_LEN - 1] = 0;
				/* if item inscriptions contains a colon we might need
				   another colon to prevent confusing it with a private message */
				if (strchr(inventory_name[c], ':')) {
					buf[strchr(inventory_name[c], ':') - inventory_name[c] + strlen(buf) - strlen(inventory_name[c]) + 1] = '\0';
					if (strchr(buf, ':') == &buf[strlen(buf) - 1])
						strcat(buf, ":");
					strcat(buf, strchr(inventory_name[c], ':') + 1);
				}
				Send_paste_msg(buf);
			}
			break;
		}
		else if (ch == 'x') {
			cmd_observe(USE_INVEN);
			continue;
		}
		else if (ch == 'd') {
			cmd_drop(USE_INVEN);
			continue;
		}
		else if (ch == 'k' || ch == KTRL('D')) {
			cmd_destroy(USE_INVEN);
			continue;
		}
		else if (ch == '{') {
			cmd_inscribe(USE_INVEN);
			continue;
		}
		else if (ch == '}') {
			cmd_uninscribe(USE_INVEN);
			continue;
		}
		else if (ch == ':') {
			cmd_message();
			continue;
		}
		break;
	}

	showing_inven = FALSE;

	/* restore the screen */
	Term_load();
	/* print our new location */

	/* Flush any events */
	Flush_queue();
}

void cmd_equip(void) {
	char ch; /* props to 'potato' and the korean team from sarang.net for the idea - C. Blue */
	int c;
	char buf[MSG_LEN];

	showing_equip = TRUE;

	Term_save();

	command_gap = 50;

	/* Hack -- show empty slots */
	item_tester_full = TRUE;

	show_equip();

	/* Undo the hack above */
	item_tester_full = FALSE;

	while (TRUE) {
		ch = inkey();
		if (ch == KTRL('T')) {
			xhtml_screenshot("screenshot????");
			break;
		}
		/* allow pasting item into chat */
		else if (ch >= 'A' && ch < 'A' + (INVEN_TOTAL - INVEN_WIELD)) { /* using capital letters to force SHIFT key usage, less accidental spam that way probably */
			c = ch - 'A';
			if (inventory[INVEN_WIELD + c].tval) {
				snprintf(buf, MSG_LEN - 1, "\377s%s", inventory_name[INVEN_WIELD + c]);
				buf[MSG_LEN - 1] = 0;
				/* if item inscriptions contains a colon we might need
				   another colon to prevent confusing it with a private message */
				if (strchr(inventory_name[INVEN_WIELD + c], ':')) {
					buf[strchr(inventory_name[INVEN_WIELD + c], ':') - inventory_name[INVEN_WIELD + c]
					    + strlen(buf) - strlen(inventory_name[INVEN_WIELD + c]) + 1] = '\0';
					if (strchr(buf, ':') == &buf[strlen(buf) - 1])
						strcat(buf, ":");
					strcat(buf, strchr(inventory_name[INVEN_WIELD + c], ':') + 1);
				}
				Send_paste_msg(buf);
			}
			break;
		}
		else if (ch == 'x') {
			cmd_observe(USE_EQUIP);
			continue;
		}
		else if (ch == '{') {
			cmd_inscribe(USE_EQUIP);
			continue;
		}
		else if (ch == '}') {
			cmd_uninscribe(USE_EQUIP);
			continue;
		}
		else if (ch == ':') {
			cmd_message();
			continue;
		}
		break;
	}

	showing_equip = FALSE;

	Term_load();

	/* Flush any events */
	Flush_queue();
}

void cmd_drop(byte flag) {
	int item, amt;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Drop what? ", (flag | USE_EXTRA))) return;

	/* Get an amount */
	if (inventory[item].number > 1) {
		if (is_cheap_misc(inventory[item].tval) && c_cfg.whole_ammo_stack && !verified_item
		    && item < INVEN_WIELD) /* <- new: ignore whole_ammo_stack for equipped ammo, so it can easily be shared */
			amt = inventory[item].number;
		else {
			inkey_letter_all = TRUE;
			amt = c_get_quantity("How many ('a' or spacebar for all)? ", inventory[item].number);
		}
	}
	else amt = 1;

	/* Send it */
	Send_drop(item, amt);
}

void cmd_drop_gold(void) {
	s32b amt;

	/* Get how much */
	inkey_letter_all = TRUE;
	amt = c_get_quantity("How much gold ('a' or spacebar for all)? ", -1);

	/* Send it */
	if (amt) Send_drop_gold(amt);
}

void cmd_wield(void) {
	int item;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Wear/Wield which item? ", (USE_INVEN | USE_EXTRA))) 
		return;

	/* Send it */
	Send_wield(item);
}

void cmd_wield2(void) {
	int item;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Wear/Wield which item? ", (USE_INVEN | USE_EXTRA)))
		return;

	/* Send it */
	Send_wield2(item);
}

void cmd_observe(byte mode) {
	int item;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Examine which item? ", (mode | USE_EXTRA)))
		return;

	/* Send it */
	Send_observe(item);
}

void cmd_take_off(void) {
	int item, amt = 255;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Takeoff which item? ", (USE_EQUIP | USE_EXTRA))) return;

	/* New in 4.4.7a, for ammunition: Can take off a certain amount - C. Blue */
	if (inventory[item].number > 1 && verified_item
	    && is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
		inkey_letter_all = TRUE;
		amt = c_get_quantity("How many ('a' or spacebar for all)? ", inventory[item].number);
		Send_take_off_amt(item, amt);
	} else
		Send_take_off(item);
}

void cmd_swap(void) {
	int item;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Swap which item? ", (USE_INVEN | USE_EQUIP | INVEN_FIRST | USE_EXTRA | CHECK_MULTI))) return;

	/* For items that can go into multiple slots (weapons and rings),
	   check @xN inscriptions on source and destination to pick slot */
	if (item < INVEN_PACK) {
		char insc[4], *c;

		/* smart handling of @xN swapping with equipped items that have alternative slot options */
		if (is_weapon(inventory[item].tval) && is_weapon(inventory[INVEN_ARM].tval) &&
		    (c = strstr(inventory_name[item], "@x"))) {
			strncpy(insc, c, 3);
			insc[3] = 0;
			if (strstr(inventory_name[INVEN_ARM], insc) && !strstr(inventory_name[INVEN_WIELD], insc)) {
				Send_wield2(item);
				return;
			}
		}
		else if (inventory[item].tval == TV_RING && inventory[INVEN_RIGHT].tval == TV_RING &&
		    (c = strstr(inventory_name[item], "@x"))) {
			strncpy(insc, c, 3);
			insc[3] = 0;
			if (strstr(inventory_name[INVEN_RIGHT], insc) && !strstr(inventory_name[INVEN_LEFT], insc)) {
				Send_wield2(item);
				return;
			}
		}
		/* new: also smart handling of @N swapping with equipped items that have alternative slot options */
		else if (is_weapon(inventory[item].tval) && is_weapon(inventory[INVEN_ARM].tval) &&
		    (c = strstr(inventory_name[item], "@")) && c[1] >= '0' && c[1] <= '9') {
			strncpy(insc, c, 2);
			insc[2] = 0;
			if (strstr(inventory_name[INVEN_ARM], insc) && !strstr(inventory_name[INVEN_WIELD], insc)) {
				Send_wield2(item);
				return;
			}
		}
		else if (inventory[item].tval == TV_RING && inventory[INVEN_RIGHT].tval == TV_RING &&
		    (c = strstr(inventory_name[item], "@")) && c[1] >= '0' && c[1] <= '9') {
			strncpy(insc, c, 2);
			insc[2] = 0;
			if (strstr(inventory_name[INVEN_RIGHT], insc) && !strstr(inventory_name[INVEN_LEFT], insc)) {
				Send_wield2(item);
				return;
			}
		}

		/* default */
		Send_wield(item);
	}
	else Send_take_off(item);
}


void cmd_destroy(byte flag) {
	int item, amt;
	char out_val[MSG_LEN];

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Destroy what? ", (flag | USE_EXTRA))) return;

	/* Get an amount */
	if (inventory[item].number > 1) {
		if (is_cheap_misc(inventory[item].tval) && c_cfg.whole_ammo_stack && !verified_item) amt = inventory[item].number;
		else {
			inkey_letter_all = TRUE;
			amt = c_get_quantity("How many ('a' or spacebar for all)? ", inventory[item].number);
		}
	} else amt = 1;

	/* Sanity check */
	if (!amt) return;

	if (!c_cfg.no_verify_destroy) {
		if (inventory[item].number == amt)
			sprintf(out_val, "Really destroy %s?", inventory_name[item]);
		else
			sprintf(out_val, "Really destroy %d of your %s?", amt, inventory_name[item]);
		if (!get_check2(out_val, FALSE)) return;
	}

	/* Send it */
	Send_destroy(item, amt);
}


void cmd_inscribe(byte flag) {
	int item;
	char buf[1024];

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Inscribe what? ", (flag | USE_EXTRA | SPECIAL_REQ))) {
		if (item != -3) return;

		/* '-3' hack: Get inscription before item (SPECIAL_REQ) */
		buf[0] = '\0';

		/* Get an inscription first */
		if (!get_string("Inscription: ", buf, 59)) return;

		/* Get the item afterwards */
		if (!c_get_item(&item, "Inscribe what? ", (USE_EQUIP | USE_INVEN | USE_EXTRA))) return;

		Send_inscribe(item, buf);
		return;
	}

	buf[0] = '\0';

	/* Get an inscription */
	if (get_string("Inscription: ", buf, 59))
		Send_inscribe(item, buf);
}

void cmd_uninscribe(byte flag) {
	int item;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Uninscribe what? ", (flag | USE_EXTRA)))
		return;

	/* Send it */
	Send_uninscribe(item);
}

void cmd_apply_autoins(void) {
	int item;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	/* Get the item */
	if (!c_get_item(&item, "Inscribe which item? ", (USE_EQUIP | USE_INVEN | USE_EXTRA))) return;

	apply_auto_inscriptions(item, TRUE);
	if (c_cfg.auto_inscribe) Send_autoinscribe(item);
}

void cmd_steal(void) {
	int dir;

	if (!get_dir(&dir)) return;

	/* Send it */
	Send_steal(dir);
}

static bool item_tester_quaffable(object_type *o_ptr) {
	if (o_ptr->tval == TV_POTION) return TRUE;
	if (o_ptr->tval == TV_POTION2) return TRUE;
	if ((o_ptr->tval == TV_FOOD) &&
			(o_ptr->sval == SV_FOOD_PINT_OF_ALE ||
			 o_ptr->sval == SV_FOOD_PINT_OF_WINE) )
			return TRUE;

	return FALSE;
}

void cmd_quaff(void) {
	int item;

	item_tester_hook = item_tester_quaffable;
	get_item_hook_find_obj_what = "Potion name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Quaff which potion? ", (USE_INVEN | USE_EXTRA | NO_FAIL_MSG))) {
		if (item == -2) c_msg_print("You don't have any potions.");
		return;
	}

	/* Send it */
	Send_quaff(item);
}

static bool item_tester_readable(object_type *o_ptr) {
	if (o_ptr->tval == TV_SCROLL) return TRUE;
	if (o_ptr->tval == TV_PARCHMENT) return TRUE;

	return FALSE;
}

void cmd_read_scroll(void) {
	int item;

	item_tester_hook = item_tester_readable;
	get_item_hook_find_obj_what = "Scroll name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Read which scroll? ", (USE_INVEN | USE_EXTRA | NO_FAIL_MSG))) {
		if (item == -2) c_msg_print("You don't have any scrolls.");
		return;
	}

	/* Send it */
	Send_read(item);
}

void cmd_aim_wand(void) {
	int item, dir;

	item_tester_tval = TV_WAND;
	get_item_hook_find_obj_what = "Wand name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Aim which wand? ", (USE_INVEN | USE_EXTRA | CHECK_CHARGED | NO_FAIL_MSG))) {
		if (item == -2) c_msg_print("You don't have any wands.");
		return;
	}

	if (!get_dir(&dir)) return;

	/* Send it */
	Send_aim(item, dir);
}

void cmd_use_staff(void) {
	int item;

	item_tester_tval = TV_STAFF;
	get_item_hook_find_obj_what = "Staff name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Use which staff? ", (USE_INVEN | USE_EXTRA | CHECK_CHARGED | NO_FAIL_MSG))) {
		if (item == -2) c_msg_print("You don't have any staves.");
		return;
	}

	/* Send it */
	Send_use(item);
}

void cmd_zap_rod(void) {
	int item, dir;

	item_tester_tval = TV_ROD;
	get_item_hook_find_obj_what = "Rod name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Zap which rod? ", (USE_INVEN | USE_EXTRA | CHECK_CHARGED | NO_FAIL_MSG))) {
		if (item == -2) c_msg_print("You don't have any rods.");
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
void cmd_refill(void) {
	int item;
	cptr p;

	if (!inventory[INVEN_LITE].tval) {
		c_msg_print("You are not wielding a light source.");
		return;
	}

	item_tester_hook = item_tester_oils;
	p = "Refill with which light? ";

	if (!c_get_item(&item, p, (USE_INVEN)))
		return;

	/* Send it */
	Send_fill(item);
}

void cmd_eat(void) {
	int item;

	item_tester_hook = item_tester_edible;
	get_item_hook_find_obj_what = "Food name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Eat what? ", (USE_INVEN | USE_EXTRA))) {
		if (item == -2) c_msg_print("You do not have anything edible.");
		return;
	}

	/* Send it */
	Send_eat(item);
}

void cmd_activate(void) {
	int item, dir;

	/* Allow all items */
	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Activatable item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Activate what? ", (USE_EQUIP | USE_INVEN | USE_EXTRA)))
		return;

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


int cmd_target(void) {
	bool done = FALSE;
	bool position = FALSE;
	int d;
	char ch;

	/* Tell the server to init targetting */
	Send_target(0);

	while (!done) {
		ch = inkey();

		if (!ch) continue;

		switch (ch) {
		case ESCAPE:
		case 'q':
			/* Clear top line */
			clear_topline();
			return FALSE;
		case 't':
		case '5':
			done = TRUE;
			break;
		case 'p':
			/* Toggle */
			position = !position;

			/* Tell the server to reset */
			Send_target(128 + 0);

			break;
		default:
			d = keymap_dirs[ch & 0x7F];
			if (!d) {
				/* APD exit if not a direction, since
				 * the player is probably trying to do
				 * something else, like stay alive...
				 */
				/* Clear the top line */
				clear_topline();
				return FALSE;
			} else {
				if (position) Send_target(d + 128);
				else Send_target(d);
			}
			break;
		}
	}

	/* Send the affirmative */
	if (position) Send_target(128 + 5);
	else Send_target(5);

	/* Print the message */
	if (!c_cfg.taciturn_messages) prt("Target selected.", 0, 0);

	return TRUE;
}


int cmd_target_friendly(void) {
	/* Tell the server to init targetting */
	Send_target_friendly(0);
	return TRUE;
}



void cmd_look(void) {
	bool done = FALSE;
	bool position = FALSE;
	int d;
	char ch;

	/* Tell the server to init looking */
	Send_look(0);

	while (!done) {
		ch = inkey();

		if (!ch) continue;

	    if (c_cfg.rogue_like_commands) {
		switch (ch) {
		case ESCAPE:
		case 'q':
			/* Clear top line */
			clear_topline();
			return;
		case KTRL('T'):
			xhtml_screenshot("screenshot????");
			break;
		case 'p':
			/* Toggle manual ground-targetting */
			position = !position;

			/* Tell the server to reset ground-target */
			Send_look(128 + 0);
			break;
		case 'x':
			/* actually look at the ground-targetted grid */
			if (position) Send_look(128 + 5);
			break;
		default:
			d = keymap_dirs[ch & 0x7F];
			if (!d) bell();
			else {
				if (position) Send_look(128 + d); /* do manual ground-targetting */
				else Send_look(d); /* do normal looking */
			}
			break;
		}
	    } else {
		switch (ch) {
		case ESCAPE:
		case 'q':
			/* Clear top line */
			clear_topline();
			return;
		case KTRL('T'):
			xhtml_screenshot("screenshot????");
			break;
		case 'p':
			/* Toggle manual ground-targetting */
			position = !position;

			/* Tell the server to reset ground-target */
			Send_look(128 + 0);
			break;
		case 'l':
			/* actually look at the ground-targetted grid */
			if (position) Send_look(128 + 5);
			break;
		default:
			d = keymap_dirs[ch & 0x7F];
			if (!d) bell();
			else {
				if (position) Send_look(128 + d); /* do manual ground-targetting */
				else Send_look(d); /* do normal looking */
			}
			break;
		}
	    }
	}
}


void cmd_character(void) {
	char ch = 0;
	int done = 0;
	char tmp[MAX_CHARS];

	/* Save screen */
	Term_save();

	while (!done) {
		/* Display player info */
		display_player(csheet_page);

		/* Window Display */
		p_ptr->window |= PW_PLAYER;
		window_stuff();
		
		/* Display message */
		prt("[ESC to quit, f to make a chardump, h to toggle history / abilities]", 22, 1);

		/* Wait for key */
		ch = inkey();

		/* specialty: allow chatting from within here -- to tell others about your lineage ;) - C. Blue */
		if (ch == ':') {
			cmd_message();
			continue;
		}

		/* Check for "display history" */
		if (ch == 'h' || ch == 'H') {
			/* Toggle */
			csheet_page++; if (csheet_page == 3) csheet_page = 0; //loop
		}

		/* Dump */
		if ((ch == 'f') || (ch == 'F')) {
			strnfmt(tmp, MAX_CHARS - 1, "%s.txt", cname);
			if (get_string("Filename(you can post it to http://angband.oook.cz/): ", tmp, MAX_CHARS - 1)) {
				if (tmp[0] && (tmp[0] != ' '))
					file_character(tmp, FALSE);
			}
		}

		/* Take a screenshot */
		if (ch == KTRL('T'))
			xhtml_screenshot("screenshot????");

		/* Check for quit */
		if (ch == 'q' || ch == 'Q' || ch == ESCAPE || ch == 'C') {
			/* Quit */
			done = 1;
		}
	}

	/* Reload screen */
	Term_load();

	/* Flush any events */
	Flush_queue();
}



void cmd_artifacts(void) {
	/* Set the hook */
	special_line_type = SPECIAL_FILE_ARTIFACT;

	/* Call the file perusal */
	peruse_file();
}

void cmd_uniques(void) {
	/* Set the hook */
	special_line_type = SPECIAL_FILE_UNIQUE;

	/* Call the file perusal */
	peruse_file();
}

void cmd_players(void) {
	/* Set the hook */
	special_line_type = SPECIAL_FILE_PLAYER;

	/* Call the file perusal */
	peruse_file();
}

void cmd_high_scores(void) {
	/* Set the hook */
	special_line_type = SPECIAL_FILE_SCORES;

	/* Call the file perusal */
	peruse_file();
}

#ifndef BUFFER_GUIDE
static char *fgets_inverse(char *buf, int max, FILE *f) {
	int c, res, pos;
	char *ress;

	/* We expect to start positioned on the '\n' of the line we want to read, aka at its very end */
	fseek(f, -1, SEEK_CUR);
	while ((c = fgetc(f)) != '\n') {
		res = fseek(f, -2, SEEK_CUR);
		/* Did we hit the beginning of the file? Emulate 'EOF' aka return 'NULL' */
		if (res == -1) return NULL;
	}
	/* We're now on the beginning of the line we want to read */
	pos = ftell(f);
	ress = fgets(buf, max, f);

	/* Rewind by this line + 1, to fulful our starting expectation for next time again */
	fseek(f, pos - 1, SEEK_SET);

	return ress;
}
#endif
void cmd_the_guide(void) {
	static int line = 0, line_before_search = 0;
	static char lastsearch[MAX_CHARS] = "";
	static char lastchapter[MAX_CHARS] = "";

	bool inkey_msg_old, within, within_col, searchwrap = FALSE, skip_redraw = FALSE, backwards = FALSE, restore_pos = FALSE;
	int bottomline = (screen_hgt > SCREEN_HGT ? 46 - 1 : 24 - 1), maxlines = (screen_hgt > SCREEN_HGT ? 46 - 4 : 24 - 4);
	int searchline = -1, within_cnt = 0, c, n, line_presearch = line;
	char search[MAX_CHARS], withinsearch[MAX_CHARS], chapter[MAX_CHARS]; //chapter[8]; -- now also used for terms
	char buf[MAX_CHARS * 2 + 1], buf2[MAX_CHARS * 2 + 1], *cp, *cp2;
#ifndef BUFFER_GUIDE
	char path[1024], bufdummy[MAX_CHARS + 1];
	FILE *fff;
#else
	static int fseek_pseudo = 0; //fake a file position, within our buffered guide string array
#endif
	int i;
	char *res;
	bool search_uppercase = FALSE;

	/* empty file? */
	if (guide_lastline == -1) return;

#ifndef BUFFER_GUIDE
	path_build(path, 1024, "", "TomeNET-Guide.txt");
	fff = my_fopen(path, "r");
	/* mysteriously can't open file anymore? */
	if (!fff) return;
#endif

	/* init searches */
	chapter[0] = 0;
	search[0] = 0;
	//lastsearch[0] = 0;

	Term_save();

	while (TRUE) {
		if (!skip_redraw) Term_clear();
		if (backwards) Term_putstr(23,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d]", (guide_lastline - line) + 1, guide_lastline + 1));
		else Term_putstr(23,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d]", line + 1, guide_lastline + 1));
		//Term_putstr(1, bottomline, -1, TERM_L_BLUE, "Up,Down,PgUp,PgDn,End navigate; s/d/c/# search/next/chapter/line; ESC to exit");
		//Term_putstr(0, bottomline, -1, TERM_L_BLUE, " Space/n,p,Enter,Back; s,S/d,D/f,c,# search,next,prev,chapter,line; ESC to exit");
		//Term_putstr(0, bottomline, -1, TERM_L_BLUE, " Space/n,p,Enter,Back; s,d,D/f,S,c,# srch,nxt,prv,reset,chapter,line; ESC exits");
		Term_putstr(8, bottomline, -1, TERM_L_BLUE, ",Space/n,p,Enter,Back,ESC; s,d,D/f,S,c,# srch,nxt,prv,reset,chapter,line");
		Term_putstr(0, bottomline, -1, TERM_YELLOW, "'?' Help");
		if (skip_redraw) goto skipped_redraw;
		restore_pos = FALSE;

#ifndef BUFFER_GUIDE
		/* Always begin at zero */
		if (backwards) fseek(fff, -1, SEEK_END);
		else fseek(fff, 0, SEEK_SET);

		/* If we're not searching for something specific, just seek forwards until reaching our current starting line */
		if (!chapter[0]) {
			if (backwards) {
				if (!searchwrap) for (n = 0; n < line; n++) res = fgets_inverse(buf, 81, fff); //res just slays non-existant compiler warning..what
			} else {
				if (!searchwrap) for (n = 0; n < line; n++) res = fgets(buf, 81, fff); //res just slays compiler warning
			}
		} else searchline = -1; //init searchline for chapter-search
#else
		/* Always begin at zero */
		if (backwards) fseek_pseudo = guide_lastline;
		else fseek_pseudo = 0;

		/* If we're not searching for something specific, just seek forwards until reaching our current starting line */
		if (!chapter[0]) {
			if (backwards) {
				if (!searchwrap) fseek_pseudo -= line;
			} else {
				if (!searchwrap) fseek_pseudo += line;
			}
		} else searchline = -1; //init searchline for chapter-search
#endif

		/* Display as many lines as fit on the screen, starting at the desired position */
		withinsearch[0] = 0;
		for (n = 0; n < maxlines; n++) {
#ifndef BUFFER_GUIDE
			if (backwards) res = fgets_inverse(buf, 81, fff);
			else res = fgets(buf, 81, fff);
#else
			if (backwards) {
				if (fseek_pseudo < 0) {
					fseek_pseudo = 0;
					res = NULL; //emulate EOF
				} else res = buf;
				strcpy(buf, guide_line[fseek_pseudo]);
				fseek_pseudo--;
			} else {
				if (fseek_pseudo > guide_lastline) {
					fseek_pseudo = guide_lastline;
					res = NULL; //emulate EOF
				} else res = buf;
				strcpy(buf, guide_line[fseek_pseudo]);
				fseek_pseudo++;
			}
#endif
			/* Reached end of file? -> No need to try and display further lines */
			if (!res) break;

			buf[strlen(buf) - 1] = 0; //strip trailing newlines

			/* Automatically add colours to "(x.yza)" formatted chapter markers */
			cp = buf;
			cp2 = buf2;
			within = FALSE;
			while (*cp) {
				/* look ahead for "(x.yza)" */
				if (*cp == '(') {
					switch (*(cp + 1)) {
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						/* sub chapter aka (x,yza) */
						if (*(cp + 2) == '.') {
							switch (*(cp + 3)) {
							case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
								if (*(cp + 4) == ')' || *(cp + 5) == ')' || *(cp + 6) == ')') {
									/* begin of colourizing */
									*cp2++ = '\377';
									*cp2++ = 'G';
									within = TRUE;
								}
							}
						}
						/* main chapter aka (x) --
						   but must be at the beginning of a line, so references to them (which currently don't exist in the text) don't count.
						   This restriction was added because many things such as lvl reqs are currently written as "(n)" in the text. */
						else if (*(cp + 2) == ')'
						    && cp == buf) { //<-'beginning of line' restriction
							/* begin of colourizing */
							*cp2++ = '\377';
							*cp2++ = 'G';
							within = TRUE;
						}
					}
				}

				/* just copy each single character over */
				*cp2++ = *cp++;

				/* end of colourizing */
				if (*(cp - 1) == ')' && within) {
					*cp2++ = '\377';
					*cp2++ = 'w';
					within = FALSE;
				}

			}
			*cp2 = 0;

			/* New chapter functionality: Search for a specific chapter keyword? */
			if (chapter[0] && chapter[0] != '(') {
				bool ok = FALSE;
				char *p, *s;

				searchline++;
				//must be beginning of line or <only spaces before it, AND NO SPACE in the actual chapter search string>
				p = strstr(buf2, chapter);
				if (p == buf2) ok = TRUE;
				else if (p && chapter[0] != ' ') { //also allow searching for various keywords, but make sure we don't pick eg 'Darkness Storm' before the actual spell diz -> SPACE check
					s = buf2;
					ok = TRUE;
					while (s < p) {
						if (*s == ' ') s++;
						else {
							ok = FALSE;
							break;
						}
					}
				}
				/* a little extra hack for '- Mind' and '- Mindcraft -' vs '- Mindcrafter': make sure a word isn't partially matched but really ends */
				if (ok && isalpha(chapter[strlen(chapter) - 1]) && isalpha(p[strlen(chapter)])) ok = FALSE;
				/* Found it? */
				if (ok) {
					/* Hack: Abuse normal 's' search to colourize */
					strcpy(withinsearch, chapter);

					chapter[0] = 0;
					line = searchline;
					/* Redraw line number display */
					if (backwards) Term_putstr(23,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d]", (guide_lastline - line) + 1, guide_lastline + 1));
					else Term_putstr(23,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d]", line + 1, guide_lastline + 1));
				} else {
					/* Skip all lines until we find the desired chapter */
					n--;
					continue;
				}
			}
			/* Search for specific chapter? */
			else if (chapter[0]) {
				searchline++;
				/* Found it? Accomodate for colour code and '.' must not follow after the chapter marker */
				if (searchline >= guide_endofcontents &&
				    strstr(buf2, chapter) == buf2 + 2 && !strchr(strchr(buf2, ')'), '.')) {
					chapter[0] = 0;
					line = searchline;
					/* Redraw line number display */
					if (backwards) Term_putstr(23,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d]", (guide_lastline - line) + 1, guide_lastline + 1));
					else Term_putstr(23,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d]", line + 1, guide_lastline + 1));
				} else {
					/* Skip all lines until we find the desired chapter */
					n--;
					continue;
				}
			}

			/* Search for specific string? */
			else if (search[0]) {
				searchline++;
				/* Search wrapped around once and still no result? Finish. */
				if (searchwrap && searchline == line) {
					search[0] = 0;
					searchwrap = FALSE;
					withinsearch[0] = 0;

					/* we cannot search for further results if there was none (would result in glitchy visuals) */
					lastsearch[0] = 0;

					/* correct our line number again */
					line = line_presearch;
					backwards = FALSE;
					/* return to that position again */
					restore_pos = TRUE;
					break;
				/* We found a result */
				} else if (my_strcasestr_skipcol(buf2, search, search_uppercase)) {
					/* Reverse again to normal direction/location */
					if (backwards) {
						backwards = FALSE;
						searchline = guide_lastline - searchline;
#ifndef BUFFER_GUIDE
						/* Skip end of line, advancing to next line */
						fseek(fff, 1, SEEK_CUR);
						/* This line has already been read too, by fgets_inverse(), so skip too */
						res = fgets(bufdummy, 81, fff); //res just slays compiler warning
#else
						fseek_pseudo += 2;
#endif
					}

					strcpy(withinsearch, search);
					search[0] = 0;
					searchwrap = FALSE;
					line = searchline;
					/* Redraw line number display */
					Term_putstr(23,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d]", line + 1, guide_lastline + 1));
				/* Still searching */
				} else {
					/* Skip all lines until we find the desired string */
					n--;
					continue;
				}
			}

			/* Colour all search finds */
			if (withinsearch[0] && my_strcasestr_skipcol(buf2, withinsearch, search_uppercase)) {
				strcpy(buf, buf2);

				cp = buf;
				cp2 = buf2;
				within = within_col = FALSE;
				while (*cp) {
					/* We're entering/exiting a (light green) coloured chapter marker? */
					if (*cp == '\377') {
						if (*(cp + 1) == 'G') within_col = TRUE;
						else within_col = FALSE;

						/* The colour-letter inside a colour code does of course not count towards the search result.
						   However, if already inside a result, don't recolour it from red to green.. */
						if (!within) {
							*cp2++ = *cp++;
							/* paranoia: broken colour code? */
							if (!(*cp)) break;
							*cp2++ = *cp++;
						} else {
							cp++;
							/* paranoia: broken colour code? */
							if (!(*cp)) break;
							cp++;
						}

						continue;
					}

					//if (!strncasecmp_skipcol(cp, withinsearch, strlen(withinsearch, search_uppercase))) { --no need to define an extra function, just reuse my_strcasestr_skipcol() simply:
					if (my_strcasestr_skipcol(cp, withinsearch, search_uppercase) == cp) {
						/* begin of colourizing */
						*cp2++ = '\377';
						*cp2++ = 'R';
						within = TRUE;
						within_cnt = 0;
					}

					/* just copy each single character over */
					*cp2++ = *cp++;

					/* end of colourizing? */
					if (within) {
						within_cnt++;
						if (within_cnt == strlen(withinsearch)) {
							within = FALSE;
							*cp2++ = '\377';
							/* restore a disrupted chapter marker? */
							if (within_col) {
								within_col = FALSE;
								*cp2++ = 'G';
							}
							/* normal text */
							else *cp2++ = 'w';
						}
					}

				}
				*cp2 = 0;
			}

			/* Display processed line, colourized by chapters and search results */
			Term_putstr(0, 2 + n, -1, TERM_WHITE, buf2);
		}

		/* failed to locate chapter? Forget about it and return to where we were */
		if (chapter[0]) {
			chapter[0] = 0;
			continue;
		}
		/* failed to find search string? wrap around and search once more,
		   this time from the beginning up to our actual posititon. */
		if (search[0]) {
			if (!searchwrap) {
				searchline = -1; //start from the beginning of the file again
				searchwrap = TRUE;
				continue;
			} else { //never reached now, since searchwrap = FALSE is set in search code above already
				/* finally end search, without any results */
				search[0] = 0;
				searchwrap = FALSE;

				/* correct our line number again */
				line = line_presearch = line;
				if (backwards) backwards = FALSE;
				continue;
			}
		}
		if (restore_pos) continue;
		/* Reverse again to normal direction/location */
		if (backwards) {
			backwards = FALSE;
			line = guide_lastline - line;
			line++;
		}

		skipped_redraw:
		skip_redraw = FALSE;

		/* hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		c = inkey();

		switch (c) {
		/* specialty: allow chatting from within here */
		case ':':
			cmd_message();
			skip_redraw = TRUE;
			continue;
		/* allow inscribing items (hm) */
		case '{':
			cmd_inscribe(USE_INVEN | USE_EQUIP);
			skip_redraw = TRUE;
			continue;
		case '}':
			cmd_uninscribe(USE_INVEN | USE_EQUIP);
			skip_redraw = TRUE;
			continue;
		case KTRL('T'):
			xhtml_screenshot("screenshot????");
			skip_redraw = TRUE;
			continue;

		/* navigate (up/down) */
		case '8': case '\010': case 0x7F: //rl:'k'
			line--;
			if (line < 0) line = guide_lastline - maxlines + 1;
			if (line < 0) line = 0;
			continue;
		case '2': case '\r': case '\n': //rl:'j'
			line++;
			if (line > guide_lastline - maxlines) line = 0;
			continue;
		/* page up/down */
		case '9': case 'p': //rl:?
			if (line == 0) line = guide_lastline - maxlines + 1;
			else line -= maxlines;
			if (line < 0) line = 0;
			continue;
		case '3': case 'n': case ' ': //rl:?
			if (line < guide_lastline - maxlines) {
				line += maxlines;
				if (line > guide_lastline - maxlines) line = guide_lastline - maxlines;
				if (line < 0) line = 0;
			} else line = 0;
			continue;
		/* home key to reset */
		case '7': //rl:?
			line = 0;
			continue;
		/* support end key too.. */
		case '1': //rl:?
			line = guide_lastline - maxlines + 1;
			if (line < 0) line = 0;
			continue;

		/* seach for 'chapter': can be either a numerical one or a main term, such as race/class/skill names. */
		case 'c':
			Term_erase(0, bottomline, 80);
			Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter chapter to jump to: ");
#if 0
			buf[0] = 0;
#else
			strcpy(buf, lastchapter); //a small life hack
#endif
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			//askfor_aux(buf, 7, 0)); //was: numerical chapters only
			askfor_aux(buf, MAX_CHARS - 1, 0); //allow entering chapter terms too
			inkey_msg = inkey_msg_old;
			if (!buf[0]) continue;

			strcpy(lastchapter, buf); //a small life hack for remembering our chapter search term for subsequent normal searching
			strcpy(lastsearch, buf); //further small life hack - not sure if a great idea or not..

			/* abuse chapter searching for extra functionality: search for chapter about a specific main term? */
			if (isalpha(buf[0])) {
				int find;
				char tmpbuf[MAX_CHARS];

				chapter[0] = 0;

				/* Expand 'AC' to 'Armour Class' */
				if (!strcasecmp(buf, "ac")) strcpy(buf, "armour class");

				/* Misc chapters, hardcoded: */
				if (!strcasecmp(buf, "Bree")
				    || my_strcasestr(buf, "Barr")
				    || my_strcasestr(buf, "Downs")
				    || my_strcasestr(buf, "Train")
				    || my_strcasestr(buf, "Tow")
				    || !strcasecmp(buf, "tt")) {
					strcpy(chapter, "Bree  ");
					continue;
				}
				if (my_strcasestr(buf, "Gond")
				    || my_strcasestr(buf, "Mord")
				    || !strcasecmp(buf, "mor")) {
					strcpy(chapter, "Gondolin  ");
					continue;
				}
				if (my_strcasestr(buf, "Minas") || my_strcasestr(buf, "Anor")
				    || my_strcasestr(buf, "Paths")
				    || my_strcasestr(buf, "Path of ")
				    || !strcasecmp(buf, "potd")) {
					strcpy(chapter, "Minas Anor  ");
					continue;
				}
				if (my_strcasestr(buf, "Loth")
				    || my_strcasestr(buf, "Angband")
				    || !strcasecmp(buf, "ang")) {
					strcpy(chapter, "Lothlorien  ");
					continue;
				}
				if (my_strcasestr(buf, "Khaz")) {
					strcpy(chapter, "Khazad-dum  ");
					continue;
				}

				if (my_strcasestr(buf, "Fate") || !strcasecmp(buf, "df")) {
					strcpy(chapter, "Death Fate   ");
					continue;
				}
				if (my_strcasestr(buf, "Mand") || my_strcasestr(buf, "Halls") || !strcasecmp(buf, "hom")) {
					strcpy(chapter, "The Halls of Mandos   ");
					continue;
				}
				if ((my_strcasestr(buf, "Orc") && my_strcasestr(buf, "Cav")) || !strcasecmp(buf, "oc")) {
					strcpy(chapter, "The Orc Cave   ");
					continue;
				}
				if (my_strcasestr(buf, "Mirk") || !strcasecmp(buf, "mw")) {
					strcpy(chapter, "Mirkwood   ");
					continue;
				}
				if ((my_strcasestr(buf, "Old") && my_strcasestr(buf, "For")) || !strcasecmp(buf, "of")) {
					strcpy(chapter, "The Old Forest   ");
					continue;
				}
				if (my_strcasestr(buf, "Helc") || !strcasecmp(buf, "hc")) {
					strcpy(chapter, "The Helcaraxe   ");
					continue;
				}
				if (my_strcasestr(buf, "Sandw") || my_strcasestr(buf, "Lair") || !strcasecmp(buf, "swl") || !strcasecmp(buf, "sl")) {
					strcpy(chapter, "The Sandworm Lair   ");
					continue;
				}
				if (my_strcasestr(buf, "Heart") || !strcasecmp(buf, "hote")) {
					strcpy(chapter, "The Heart of the Earth   ");
					continue;
				}
				if (my_strcasestr(buf, "Maze")) {
					strcpy(chapter, "The Maze   ");
					continue;
				}
				if (my_strcasestr(buf, "Ciri") || (my_strcasestr(buf, "Ung") && !my_strcasestr(buf, "Dung") && !my_strcasestr(buf, "Ungoli")) || !strcasecmp(buf, "cu")) { //Ungoliant check is just paranoia, not a dungeon boss
					strcpy(chapter, "Cirith Ungol   ");
					continue;
				}
				if (my_strcasestr(buf, "Rhun") || !strcasecmp(buf, "lor")) {
					strcpy(chapter, "The Land of Rhun   ");
					continue;
				}
				if (my_strcasestr(buf, "Mori") || my_strcasestr(buf, "Mines") || !strcasecmp(buf, "mom")) {
					strcpy(chapter, "The Mines of Moria   ");
					continue;
				}
				find = 0;
				if (my_strcasestr(buf, "Smal")) find++;
				if (my_strcasestr(buf, "Wate")) find++;
				if (my_strcasestr(buf, "Cav")) find++;
				if (find >= 2 || my_strcasestr(buf, "SWC")) {
					strcpy(chapter, "The Small Water Cave   ");
					continue;
				}
				if (my_strcasestr(buf, "Subm") || my_strcasestr(buf, "Ruin") || !strcasecmp(buf, "sr")) {
					strcpy(chapter, "Submerged Ruins   ");
					continue;
				}
				if (my_strcasestr(buf, "Illu") || my_strcasestr(buf, "Cast") || !strcasecmp(buf, "ic")) {
					strcpy(chapter, "The Illusory Castle   ");
					continue;
				}
				find = 0;
				if (my_strcasestr(buf, "Sacr")) find++;
				if (my_strcasestr(buf, "Land")) find++;
				if (my_strcasestr(buf, "Mou")) find++;
				if (find >= 2 || !strcasecmp(buf, "slom")) {
					strcpy(chapter, "The Sacred Land of Mountains   ");
					continue;
				}
				if (my_strcasestr(buf, "Ereb")) {
					strcpy(chapter, "Erebor   ");
					continue;
				}
				if (!strcasecmp(buf, "Dol") || my_strcasestr(buf, "Guld") || !strcasecmp(buf, "dg")) {
					strcpy(chapter, "Dol Guldur   ");
					continue;
				}
				if ((my_strcasestr(buf, "Moun")
				    && !my_strcasestr(buf, "Mounta")) //don't confuse with Sacred Lands of Mountains
				    || (my_strcasestr(buf, "Doom")
				    && !my_strcasestr(buf, "Doome")) //don't confuse with Doomed Grounds spell
				    || !strcasecmp(buf, "mtd")) { //note: TSLoM has been filtered out before us already
					strcpy(chapter, "Mount Doom   ");
					continue;
				}
				if ((my_strcasestr(buf, "Clou") && my_strcasestr(buf, "Pla")) || !strcasecmp(buf, "cp")) {
					strcpy(chapter, "The Cloud Planes   ");
					continue;
				}
				if ((my_strcasestr(buf, "Neth") && my_strcasestr(buf, "Rea")) || !strcasecmp(buf, "nr")) {
					strcpy(chapter, "Nether Realm   ");
					continue;
				}

				find = 0;
				if (my_strcasestr(buf, "Iron")) find++;
				if (my_strcasestr(buf, "Deep")) find++;
				if (my_strcasestr(buf, "Dive")) find++;
				if (my_strcasestr(buf, "Chal")) find++;
				if (!strcasecmp(buf, "IDDC") || !strcasecmp("Ironman Deep Dive Challenge", buf) || find >= 2) {
					strcpy(chapter, "Ironman Deep Dive Challenge (IDDC)");
					continue;
				}
				if ((!strcasecmp("HT", buf) || !strcasecmp("HL", buf) || my_strcasestr("Highland", buf) || my_strcasestr("Tournament", buf))
				    && strcasecmp("Ent", buf)) { //oops
					strcpy(chapter, "Highlander Tournament");
					continue;
				}
				if (!strcasecmp("AMC", buf) || (my_strcasestr("Arena", buf))) {// && my_strcasestr("Challenge", buf))) {
					strcpy(chapter, "Arena Monster Challenge");
					continue;
				}
				if (!strcasecmp("DK", buf) || !strcasecmp("Dungeon Keeper", buf) || my_strcasestr("Keeper", buf)) { //sorry, Death Knights
					strcpy(chapter, "Dungeon Keeper");
					continue;
				}
				if (!strcasecmp("XO", buf) || !strcasecmp("Extermination Orders", buf)
				    || my_strcasestr(buf, "Extermination") || my_strcasestr(buf, "Order")) {
					strcpy(chapter, "Extermination Orders");
					continue;
				}
				if (my_strcasestr("Halloween", buf)) {
					strcpy(chapter, "Halloween");
					continue;
				}
				if (!strcasecmp("Christmas", buf) || !strcasecmp(buf, "xmas")) {
					strcpy(chapter, "Christmas");
					continue;
				}
				find = 0;
				if (my_strcasestr(buf, "New")) find++;
				if (my_strcasestr(buf, "Year")) find++;
				if (my_strcasestr(buf, "Eve")) find++;
				if (my_strcasestr(buf, "nye") || find >= 2) {
					strcpy(chapter, "New year's eve");
					continue;
				}
				if (!strcasecmp("ma", buf)) {
					strcpy(chapter, ".Martial Arts");
					continue;
				}

				/* Lua-defined chapters */

				/* actually first do an 'exact match' search on all schools/school groups, while we check for partial match further down again -
				   reason: '- Mind' school and '- Mindcraft -' school group vs 'Mindcrafter' class name similarity.. */
				for (i = 0; i < guide_schools; i++) {
					/* account for the hacky trailing ' -' in some guide skills */
					strcpy(tmpbuf, guide_school[i]);
					if (tmpbuf[strlen(tmpbuf) - 1] == '-') tmpbuf[strlen(tmpbuf) - 2] = 0; //school group 'Mindcraft': cut off the trailing ' -'

					if (strcasecmp(tmpbuf, buf)) continue; //any match (default)
					strcpy(chapter, "- ");
					strcat(chapter, guide_school[i]); //can be prefixed by either + or . (see guide.lua)
					break;
				}
				if (chapter[0]) continue;

				/* New, for 'Curses' vs 'Remove Curses': Do an 'exact match' search on guide chapters */
				for (i = 0; i < guide_chapters; i++) {
					if (strcasecmp(guide_chapter[i], buf)) continue;
					strcpy(chapter, "(");
					strcat(chapter, guide_chapter_no[i]);
					strcat(chapter, ")");
					break;
				}
				if (chapter[0]) continue;

				//race/class prefix match
				for (i = 0; i < guide_races; i++) {
					if (my_strcasestr(guide_race[i], buf) != guide_race[i]) continue;
					strcpy(chapter, "- ");
					strcat(chapter, guide_race[i]);
					break;
				}
				if (chapter[0]) continue;
				for (i = 0; i < guide_classes; i++) {
					if (my_strcasestr(guide_class[i], buf) != guide_class[i]) continue;
					strcpy(chapter, "- ");
					strcat(chapter, guide_class[i]);
					break;
				}
				if (chapter[0]) continue;

				//race/class any match
				for (i = 0; i < guide_races; i++) {
					if (!my_strcasestr(guide_race[i], buf)) continue;
					strcpy(chapter, "- ");
					strcat(chapter, guide_race[i]);
					break;
				}
				if (chapter[0]) continue;
				for (i = 0; i < guide_classes; i++) {
					if (!my_strcasestr(guide_class[i], buf)) continue;
					strcpy(chapter, "- ");
					strcat(chapter, guide_class[i]);
					break;
				}
				if (chapter[0]) continue;

				for (i = 0; i < guide_skills; i++) {
					if (strcasecmp(guide_skill[i], buf)) continue; //exact match?
					strcpy(chapter, guide_skill[i]); //can be prefixed by either + or . (see guide.lua)
					break;
				}
				if (chapter[0]) continue;
				for (i = 0; i < guide_skills; i++) {
					if (my_strcasestr(guide_skill[i], buf) != guide_skill[i]) continue; //prefix match?
					strcpy(chapter, guide_skill[i]); //can be prefixed by either + or . (see guide.lua)
					break;
				}
				if (chapter[0]) continue;
				for (i = 0; i < guide_skills; i++) {
					if (!my_strcasestr(guide_skill[i], buf)) continue; //any match (default)
					strcpy(chapter, guide_skill[i]); //can be prefixed by either + or . (see guide.lua)
					break;
				}
				if (chapter[0]) continue;

				for (i = 0; i < guide_schools; i++) {
					if (!my_strcasestr(guide_school[i], buf)) continue;
					strcpy(chapter, "- ");
					strcat(chapter, guide_school[i]);
					break;
				}
				if (chapter[0]) continue;

				for (i = 0; i < guide_spells; i++) { //exact match?
					if (strcasecmp(guide_spell[i], buf)) continue;
					strcpy(chapter, "    ");
					strcat(chapter, guide_spell[i]);
					/* Enforce 'direct match over partial match' eg 'Lightning' vs 'Lightning Bolt' via silyl hack:
					   If spell name contains no space then after the spell's name at least 2 spaces must follow.. ehe
					   This keenly assumes that 1) spaceless spell names are short enough to not force a line break before
					   the spell detail text starts, 2) spaceless spell names are short enough that not just one space follows
					   before the spell detail text starts. */
					if (!strchr(guide_spell[i], ' ')) strcat(chapter, "  ");
					break;
				}
				if (chapter[0]) continue;
				for (i = 0; i < guide_spells; i++) { //prefix match?
					if (my_strcasestr(guide_spell[i], buf) != guide_spell[i]) continue;
					strcpy(chapter, "    ");
					strcat(chapter, guide_spell[i]);
					break;
				}
				if (chapter[0]) continue;
				for (i = 0; i < guide_spells; i++) { //any match (default)
					if (!my_strcasestr(guide_spell[i], buf)) continue;
					strcpy(chapter, "    ");
					strcat(chapter, guide_spell[i]);
					break;
				}
				if (chapter[0]) continue;

				/* Fighting/ranged techniques, hard-coded in the client */
				for (i = 0; i < 16; i++) {
					if (!my_strcasestr(melee_techniques[i], buf)) continue;
					if (strstr(melee_techniques[i], "Tech") == melee_techniques[i]) continue; //skip placeholders
					if (strstr(melee_techniques[i], "XXX") == melee_techniques[i]) continue; //skip placeholders
					strcpy(chapter, "    ");
					strcat(chapter, melee_techniques[i]);
					break;
				}
				for (i = 0; i < 16; i++) {
					if (!my_strcasestr(ranged_techniques[i], buf)) continue;
					if (strstr(ranged_techniques[i], "Tech") == ranged_techniques[i]) continue; //skip placeholders
					if (strstr(ranged_techniques[i], "XXX") == ranged_techniques[i]) continue; //skip placeholders
					strcpy(chapter, "    ");
					strcat(chapter, ranged_techniques[i]);
					break;
				}
				if (chapter[0]) continue;

				/* Additions */
				if (my_strcasestr(buf, "Line")) { //draconian lineages
					strcpy(chapter, "    Lineage");
					continue;
				}
				if (my_strcasestr(buf, "Depth")) { //min depth for exp in relation to char lv
					strcpy(chapter, "    Character level");
					continue;
				}
				if (my_strcasestr(buf, "Dung") && (my_strcasestr(buf, "typ") || my_strcasestr(buf, "Col"))) { //dungeon types/colours, same as staircases below
					strcpy(chapter, "Dungeon types");
					continue;
				}
				if (my_strcasestr(buf, "Dung")) { //dungeons
					strcpy(chapter, "Dungeon                 ");
					continue;
				}
				if (my_strcasestr(buf, "Boss")) { //dungeon bosses
					strcpy(chapter, "Dungeon, sorted by depth");
					continue;
				}
				if (my_strcasestr(buf, "Stair") || my_strcasestr(buf, "typ") ) { //staircase types, same as 'Dungeon types'
					strcpy(chapter, "Dungeon types");
					continue;
				}

				/* If not matched, lastly try to (partially) match chapter titles */
				/* First try to match beginning of title */
				for (i = 0; i < guide_chapters; i++) {
					if (my_strcasestr(guide_chapter[i], buf) == guide_chapter[i]) {
						strcpy(chapter, "(");
						strcat(chapter, guide_chapter_no[i]);
						strcat(chapter, ")");
						break;
					}
				}
				if (chapter[0]) continue;
				/* If no match, try to match anywhere in title */
				for (i = 0; i < guide_chapters; i++) {
					if (!my_strcasestr(guide_chapter[i], buf)) continue;
					strcpy(chapter, "(");
					strcat(chapter, guide_chapter_no[i]);
					strcat(chapter, ")");
					break;
				}
				if (chapter[0]) continue;

				/* Continue with failure (empty chapter[] string) */
				continue;
			}
			/* the original use of 'chapter' meant numerical chapters, which can have up to 8 characters, processed below */
			buf[8] = 0;

			/* search for numerical chapter, aka (x.yza) */
			memset(chapter, 0, sizeof(char) * 8);
			cp = buf;
			cp2 = chapter;
			if (*cp == '(') cp++;
			*cp2++ = '(';
			*cp2++ = *cp++;
			*cp2++ = *cp++;

			/* enable main chapters too? -> '(n)' */
			if (*(cp2 - 1) == ')') continue;
			else if (*(cp2 - 1) == 0) {
				*(cp2 - 1) = ')';
				continue;
			}

			*cp2++ = *cp++;
			*cp2++ = *cp++;
			if (*(cp2 - 1) == ')') continue; // (x.y)
			else if (*(cp2 - 1) == 0) {
				*(cp2 - 1) = ')';
				continue;
			}
			*cp2++ = *cp++;
			if (*(cp2 - 1) == ')') continue; // (x.yn)
			else if (*(cp2 - 1) == 0) {
				*(cp2 - 1) = ')';
				continue;
			}
			*cp2++ = *cp++;
			if (*(cp2 - 1) == ')') continue; // (x.yza)
			else if (*(cp2 - 1) == 0) {
				*(cp2 - 1) = ')';
				continue;
			}
			chapter[0] = 0; //invalid chapter specified
			continue;
		/* search for keyword */
		case 's':
			Term_erase(0, bottomline, 80);
			Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter search string: ");
#if 0
			search[0] = 0;
#else
			/* life hack: if we searched for a non-numerical chapter string, make it auto-search target
			   for a (possibly happening subsequently to the chapter search) normal search too */
 #if 0
			if (lastchapter[0] != '(') strcpy(search, lastchapter);
			else search[0] = 0;
 #else
			strcpy(search, lastchapter);
 #endif
#endif
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			askfor_aux(search, MAX_CHARS - 1, 0);
			inkey_msg = inkey_msg_old;
			if (!search[0]) continue;

			line_before_search = line;
			line_presearch = line;
			/* Skip the line we're currently in, start with the next line actually */
			line++;
			if (line > guide_lastline - maxlines) line = guide_lastline - maxlines;
			if (line < 0) line = 0;

			/* Hack: If we enter all upper-case, search for exactly that, case sensitively */
			search_uppercase = TRUE;
			for (c = 0; search[c]; c++) {
				if (search[c] == toupper(search[c])) continue;
				search_uppercase = FALSE;
				break;
			}

			strcpy(lastsearch, search);
			searchline = line - 1; //init searchline for string-search
			continue;
		/* special function now: Reset search. Means: Go to where I was before searching. */
		case 'S':
			line = line_before_search;
			continue;
		/* search for next occurance of the previously used search keyword */
		case 'd':
			if (!lastsearch[0]) continue;

			line_presearch = line;
			/* Skip the line we're currently in, start with the next line actually */
			line++;
#if 0
			if (line > guide_lastline - maxlines) line = guide_lastline - maxlines;
			if (line < 0) line = 0;
#endif

			strcpy(search, lastsearch);
			searchline = line - 1; //init searchline for string-search
			continue;
		/* search for previous occurance of the previously used search keyword */
		case 'D':
		case 'f':
			if (!lastsearch[0]) continue;

			line_presearch = line;
			/* Inverse location/direction */
			backwards = TRUE;
			line = guide_lastline - line;
			/* Skip the line we're currently in, start with the next line actually */
			line++;

#if 0
			if (line > guide_lastline - maxlines) line = guide_lastline - maxlines;
			if (line < 0) line = 0;
#endif
			strcpy(search, lastsearch);
			searchline = line - 1; //init searchline for string-search
			continue;
		/* jump to a specific line number */
		case '#':
			Term_erase(0, bottomline, 80);
			Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter line number to jump to: ");
			buf[0] = 0;
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			if (!askfor_aux(buf, 7, 0)) {
				inkey_msg = inkey_msg_old;
				continue;
			}
			inkey_msg = inkey_msg_old;

			line = atoi(buf) - 1;
			if (line > guide_lastline - maxlines) line = guide_lastline - maxlines;
			if (line < 0) line = 0;
			continue;
		case '?': //help
			Term_clear();
			Term_putstr(23,  0, -1, TERM_L_BLUE, "[The Guide - navigation keys]");
			Term_putstr( 0,  2, -1, TERM_WHITE, "At the bottom of the guide screen you will see the following line:");
			Term_putstr( 8,  4, -1, TERM_L_BLUE, ",Space/n,p,Enter,Back,ESC; s,d,D/f,S,c,# srch,nxt,prv,reset,chapter,line");
			Term_putstr( 0,  4, -1, TERM_YELLOW, "'?' Help");
			Term_putstr( 0,  6, -1, TERM_WHITE, "Those keys can be used to navigate the guide. Here's a detailed explanation:");
			Term_putstr( 0,  7, -1, TERM_WHITE, "  Space or 'n':    Move down by one page");
			Term_putstr( 0,  8, -1, TERM_WHITE, "  'p'         :    Move up by one page");
			Term_putstr( 0,  9, -1, TERM_WHITE, "  's'         :    Search for a text string (use all upper-case for strict mode)");
			Term_putstr( 0, 10, -1, TERM_WHITE, "  'd'         :    ..after 's', this jumps to the next match");
			Term_putstr( 0, 11, -1, TERM_WHITE, "  'D' or 'f'  :    ..after 's', this jumps to the previous match");
			Term_putstr( 0, 12, -1, TERM_WHITE, "  'S'         :    ..resets screen to where you were before you did a search.");
			Term_putstr( 0, 13, -1, TERM_WHITE, "  'c'         :    Chapter Search. This is a special search that will skip most");
			Term_putstr( 0, 14, -1, TERM_WHITE, "                   basic text and only match specific topics and keywords.");
			Term_putstr( 0, 15, -1, TERM_WHITE, "                   Use this when searching for races, classes, skills, spells,");
			Term_putstr( 0, 16, -1, TERM_WHITE, "                   schools, techniques, dungeons, bosses, events, lineages,");
			Term_putstr( 0, 17, -1, TERM_WHITE, "                   depths, stair types, or actual chapter titles or indices.");
			Term_putstr( 0, 18, -1, TERM_WHITE, "  '#'         :    Jump to a specific line number.");
			Term_putstr( 0, 19, -1, TERM_WHITE, "  ESC         :    The Escape key will exit the guide screen.");
			Term_putstr( 0, 20, -1, TERM_WHITE, "In addition, the arrow keys and the number pad keys can be used, and the keys");
			Term_putstr( 0, 21, -1, TERM_WHITE, "PgUp/PgDn/Home/End should work both on the main keyboard and the number pad.");
			Term_putstr( 0, 22, -1, TERM_WHITE, "This might depend on your specific OS flavour and desktop environment though.");
			Term_putstr(23, 23, -1, TERM_L_BLUE, "(Press any key to go back)");
			inkey();
			continue;

		/* exit */
		case ESCAPE: //case 'q': case KTRL('Q'):
#ifndef BUFFER_GUIDE
			my_fclose(fff);
#endif
			Term_load();
			return;
		}
	}

#ifndef BUFFER_GUIDE
	my_fclose(fff);
#endif
	Term_load();
}

void cmd_help(void) {
	/* Set the hook */
	special_line_type = SPECIAL_FILE_HELP;

	/* Call the file perusal */
	peruse_file();
}

#define ARTIFACT_LORE_LIST_SIZE 17
static void artifact_lore(void) {
	char s[20 + 1], tmp[80];
	int c, i, j, n, selected, selected_list, list_idx[ARTIFACT_LORE_LIST_SIZE], presorted;
	bool show_lore = TRUE, direct_match;
	int selected_line = 0;
	/* for pasting lore to chat */
	char paste_lines[18][MSG_LEN];
	/* suppress hybrid macros */
	bool inkey_msg_old;
	/* for skipping pages forward/backward if there are more matches than lines */
	int pageoffset = 0, skipped;
	bool more_results;

	Term_save();

    s[0] = '\0';
    while (TRUE) {
	Term_clear();
	Term_putstr(2,  2, -1, TERM_WHITE, "Enter (partial) artifact name to refine the search:");
	Term_putstr(2,  3, -1, TERM_WHITE, "Press ENTER to display lore about the selected artifact.");

	while (TRUE) {
		Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Artifact Lore ***");
		Term_putstr(54,  2, -1, TERM_WHITE, "                                ");
		Term_putstr(54,  2, -1, TERM_L_GREEN, s);

		/* display top 15 of all matching artifacts */
		clear_from(5);
		n = 0;
		selected = selected_list = -1;

		skipped = 0;
		more_results = FALSE;

		/* hack 1: exact match always takes top position
		   hack 2: match at beginning of name takes precedence */
		direct_match = FALSE;
		if (s[0] && !pageoffset) for (i = 0; i < MAX_A_IDX; i++) {
			/* create upper-case working copy */
			strcpy(tmp, artifact_list_name[i]);
			for (j = 0; tmp[j]; j++) tmp[j] = toupper(tmp[j]);

			/* exact match? exact match without 'The ' at the beginning maybe? */
			if (!strcmp(tmp, s) ||
			    (tmp[0] == 'T' && tmp[1] == 'H' && tmp[2] == 'E' && tmp[3] == ' ' && !strcmp(tmp + 4, s))
			    || (s[0] == '#' && atoi(s + 1) && artifact_list_code[i] == atoi(s + 1))) { /* also allow typing in the artifact's aidx directly! */
				if (direct_match) continue; //paranoia -- there should never be two artifacts of the exact same name or index number
				direct_match = TRUE;

				selected = artifact_list_code[i];
				selected_list = i;
				if (n) Term_erase(5, 5, 80); //erase old beginning-of-line match that was shown here first
				Term_putstr(5, 5, -1, selected_line == 0 ? TERM_L_UMBER : TERM_UMBER, artifact_list_name[i]);

				/* no beginning-of-line match yet? good. */
				if (!n) {
					list_idx[n] = i;
				} else { /* already got a beginning-of-line match? swap them, cause exact match always take pos #0 */
					int tmp = list_idx[0];
					list_idx[0] = i;
					list_idx[n] = tmp;

					/* redisplay the moved choice */
					Term_putstr(5, 5 + n, -1, selected_line == n ? TERM_L_UMBER : TERM_UMBER, artifact_list_name[i]);
				}

				n++;
				break;
				if (n == 2) break;//allow 1 direct + 1 beginning-of-line match
			}
			/* beginning of line match? without the 'The ' at the beginning maybe?
			   (only check for two, of which one might already be a direct match (checked above)) */
			else if ((n == 0 || (direct_match && n == 1)) &&
			    (!strncmp(tmp, s, strlen(s)) ||
			    (tmp[0] == 'T' && tmp[1] == 'H' && tmp[2] == 'E' && tmp[3] == ' ' && !strncmp(tmp + 4, s, strlen(s))))) {
				selected = artifact_list_code[i];
				selected_list = i;
				Term_putstr(5, 5 + n, -1, selected_line == n ? TERM_L_UMBER : TERM_UMBER, artifact_list_name[i]);
				list_idx[n] = i;
				n++;
				if (direct_match) break;//allow 1 direct + 1 beginning-of-line match
			}
		}
		presorted = n;

		for (i = 0; i < MAX_A_IDX && n < ARTIFACT_LORE_LIST_SIZE + 1; i++) { /* '+ 1' for 'more_results' hack */
#if 0
			/* direct match above already? */
			if (i == selected_list) continue;
#else
			/* direct match or beginning-of-line matches above already? */
			for (j = 0; j < presorted; j++) if (i == list_idx[j]) break;
			if (j != presorted) continue;
#endif

			/* create upper-case working copy */
			strcpy(tmp, artifact_list_name[i]);
			for (j = 0; tmp[j]; j++) tmp[j] = toupper(tmp[j]);

			if (artifact_list_code[i] &&
			    ((s[0] == '!' && s[1] && artifact_list_name[i][0] == s[1]) /* also allow typing in !<char> aka the artifact symbol! */
			    || strstr(tmp, s))) { /* partial string match anywhere in the name */
				/* hack - found more results than fit on currently displayed page? */
				if (n == ARTIFACT_LORE_LIST_SIZE) {
					more_results = TRUE;
					break;
				}
				if (skipped < pageoffset * 17) {
					skipped++;
					continue;
				}

				if (n == 0) {
					selected_list = i;
					selected = artifact_list_code[i];
				}
				Term_putstr(5, 5 + n, -1, n == selected_line ? TERM_L_UMBER : TERM_UMBER, artifact_list_name[i]);
				list_idx[n] = i;
				n++;
			}
		}

		/* choice outside of the list? snap back */
		if (selected_line >= n) {
			selected_line = n - 1;
			if (selected_line < 0) selected_line = 0;
			else Term_putstr(5, 5 + selected_line, -1, TERM_L_UMBER, artifact_list_name[list_idx[selected_line]]);
		}

		if (!s[0])
			Term_putstr(2,  23, -1, TERM_WHITE, "Press ESC to exit, ENTER for lore/stats, Up/Down/PgUp/PgDn/Home to navigate");
		else
			Term_putstr(2,  23, -1, TERM_WHITE, "Press ESC to clear, ENTER for lore/stats, Up/Down/PgUp/PgDn/Home to navigate");
		/* hack: place cursor at pseudo input prompt */
		Term->scr->cx = 54 + strlen(s);
		Term->scr->cy = 2;
		Term->scr->cu = 1;

		inkey_msg_old = inkey_msg;
		inkey_msg = TRUE;
		c = inkey();
		inkey_msg = inkey_msg_old;

		/* specialty: allow chatting from within here */
		if (c == ':') {
			cmd_message();
			continue;
		}
		/* allow copying the artifact details into an item inscription ;) */
		if (c == '{') {
			cmd_inscribe(USE_INVEN | USE_EQUIP);
			continue;
		}
		if (c == '}') {
			cmd_uninscribe(USE_INVEN | USE_EQUIP);
			continue;
		}

		/* navigate in the list (up/down) */
		if (s[0] != '#' && c == '8') { //rl:'k'
			if (n > 0) {
				if (selected_line > 0) selected_line--;
				else selected_line = n - 1;
			}
			continue;
		}
		if (s[0] != '#' && c == '2') { //rl:'j'
			if (n > 0) {
				if (selected_line < n - 1) selected_line++;
				else selected_line = 0;
			}
			continue;
		}

		/* page up/down in the search results */
		if (s[0] != '#' && c == '9') { //rl:?
			if (pageoffset > 0) pageoffset--;
			continue;
		}
		if (s[0] != '#' && c == '3') { //rl:?
			if (pageoffset < 100 /* silyl */
			    && more_results)
				pageoffset++;
			continue;
		}
		/* home key to reset */
		if (s[0] != '#' && c == '7') { //rl:?
			pageoffset = 0;
			continue;
		}

		/* backspace */
		if (c == '\010' || c == 0x7F) {
			if (strlen(s) > 0) {
				s[strlen(s) - 1] = '\0';
				pageoffset = 0;
			}
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
		if (c == ESCAPE) {
			if (!s[0]) break;
			else s[0] = '\0';
		}
		/* illegal char */
		if (c < 32 || c > 127) continue;
		/* name too long? */
		if (strlen(s) >= 20) continue;
		/* build name */
		c = toupper(c);
		strcat(s, format("%c", c));
		pageoffset = 0;
	}

	/* exit? */
	if (c == ESCAPE) {
		Term_load();
		return;
	}

	/* display lore! */
	selected_list = list_idx[selected_line];
	selected = artifact_list_code[selected_list];
	while (TRUE) {
		clear_from(4);
		//Term_putstr(5, 5, -1, TERM_L_UMBER, artifact_list_name[selected_list]);
		for (i = 0; i < 18; i++) paste_lines[i][0] = '\0';

		if (show_lore) {
			artifact_lore_aux(selected, selected_list, paste_lines);
			Term_putstr(5,  23, -1, TERM_WHITE, "-- press ESC/Backspace to exit, SPACE for stats, c/C to chat-paste --");
		} else {
			artifact_stats_aux(selected, selected_list, paste_lines);
			Term_putstr(5,  23, -1, TERM_WHITE, "-- press ESC/Backspace to exit, SPACE for lore, c/C to chat-paste --");
		}
		/* hack: hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		while (TRUE) {
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			c = inkey();
			inkey_msg = inkey_msg_old;

			/* specialty: allow chatting from within here */
			if (c == ':') {
				cmd_message();
				Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Artifact Lore ***");
			}
			/* allow copying the artifact details into an item inscription ;) */
			if (c == '{') {
				cmd_inscribe(USE_INVEN | USE_EQUIP);
				Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Artifact Lore ***");
			}
			if (c == '}') {
				cmd_uninscribe(USE_INVEN | USE_EQUIP);
				Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Artifact Lore ***");
			}

			if (c == '\e' || c == '\b') break;
			if (c == ' ') {
				show_lore = !show_lore;
				break;
			}
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
			if (c == 'C') {
				/* paste lore AND stats into chat */
				for (i = 0; i < 18; i++) paste_lines[i][0] = '\0';
				artifact_lore_aux(selected, selected_list, paste_lines);
				for (i = 0; i < 18; i++) {
					if (!paste_lines[i][0]) break;
					//if (i == 6 || i == 12) usleep(10000000);
					if (paste_lines[i][strlen(paste_lines[i]) - 1] == ' ')
						paste_lines[i][strlen(paste_lines[i]) - 1] = '\0';
					Send_paste_msg(paste_lines[i]);
				}
				/* don't double-post the title: skip paste line 0 */
				for (i = 0; i < 18; i++) paste_lines[i][0] = '\0';
				artifact_stats_aux(selected, selected_list, paste_lines);
				for (i = 1; i < 18; i++) {
					if (!paste_lines[i][0]) break;
					//if (i == 6 || i == 12) usleep(10000000);
					if (paste_lines[i][strlen(paste_lines[i]) - 1] == ' ')
						paste_lines[i][strlen(paste_lines[i]) - 1] = '\0';
					Send_paste_msg(paste_lines[i]);
				}
				break;
			}
		}
		/* ESC = go back and erase search term */
		if (c == '\e') {
			s[0] = '\0';
			break;
		}
		/* Backspace = go back but keep search term */
		if (c == '\b') break;
	}
  }

	Term_load();
}

/* 18 ATTR_MULTI 'D's in the game atm, but Chaos Wyrm is also ATTR_ANY,
   so we'll just have ATTR_ANY override ATTR_MULTI and disallow both,
   hence ending up with a neat amount of 17 'pure' ATTR_ANY monsters. ;) - C. Blue */
#define MONSTER_LORE_LIST_SIZE 17
static void monster_lore(void) {
	char s[20 + 1], tmp[80];
	int c, i, j, n, selected, selected_list, list_idx[MONSTER_LORE_LIST_SIZE], presorted;
	bool show_lore = TRUE, direct_match;
	int selected_line = 0;
	/* for pasting lore to chat */
	char paste_lines[18][MSG_LEN];
	/* suppress hybrid macros */
	bool inkey_msg_old;
	/* for skipping pages forward/backward if there are more matches than lines */
	int pageoffset = 0, skipped;
	bool more_results;

	Term_save();

    s[0] = '\0'; s[1] = 0; s[2] = 0;
    while (TRUE) {
	Term_clear();
	Term_putstr(2,  2, -1, TERM_WHITE, "Enter (partial) monster name to refine the search:");
	Term_putstr(2,  3, -1, TERM_WHITE, "Or press '!' followed by a monster symbol and a colour symbol or 'm' or 'M'.");
	//Term_putstr(2,  4, -1, TERM_WHITE, "Press RETURN to display lore about the selected monster.");

	while (TRUE) {
		Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Monster Lore ***");
		Term_putstr(53,  2, -1, TERM_WHITE, "                                ");
		Term_putstr(53,  2, -1, TERM_L_GREEN, s);

		/* display top 15 of all matching monsters */
		clear_from(5);
		n = 0;
		selected = selected_list = -1;

		skipped = 0;
		more_results = FALSE;

		/* Entering a '!' first means "search for monster symbol instead" */
		if (s[0] == '!') {
			if (s[1] && !pageoffset) for (i = 1; i < monster_list_idx; i++) {
				/* match? */
				if (monster_list_symbol[i][1] == s[1] &&
				    (!s[2] ||
				     /* don't display 'v'iolet monsters that are really multi-hued, when searching for 'v'iolet monsters. */
				     (monster_list_symbol[i][0] == s[2] &&
				    /* ... extended on any colour (Chaos Butterfly was L_GREEN for example) */
				     //!(s[2] == 'v' && (monster_list_any[i] || monster_list_breath[i])))
				     !(monster_list_any[i] || monster_list_breath[i]))
				     ||
				    (s[2] == 'm' && monster_list_any[i]) ||
				    (s[2] == 'M' && monster_list_breath[i]))) {
		    			selected = monster_list_code[i];
					selected_list = i;
#if 0
					Term_putstr(5, 5, -1, selected_line == 0 ? TERM_YELLOW : TERM_UMBER, format("(%4d, \377%c%c\377%c, L%-3d)  %s",
					    monster_list_code[i], monster_list_symbol[i][0], monster_list_symbol[i][1], selected_line == 0 ? 'y' : 'u', monster_list_level[i], monster_list_name[i]));
#else
					Term_putstr(5, 5, -1, selected_line == 0 ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c)  %s",
					    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], selected_line == 0 ? 'y' : 'u', monster_list_name[i]));
#endif
					list_idx[0] = i;
					n++;
					break;
				}
			}
			/* check for more matches */
			for (i = 1; i < monster_list_idx && n < MONSTER_LORE_LIST_SIZE + 1; i++) { /* '+ 1' for 'more_results' hack */
				/* direct match above already? */
				if (i == selected_list) continue;

				if (monster_list_code[i] &&
				    monster_list_symbol[i][1] == s[1] &&
				    (!s[2] ||
				     /* don't display 'v'iolet monsters that are really multi-hued, when searching for 'v'iolet monsters */
				     (monster_list_symbol[i][0] == s[2] &&
				     /* ... extended on any colour (Chaos Butterfly was L_GREEN for example) */
				     //!(s[2] == 'v' && (monster_list_any[i] || monster_list_breath[i])))
				     !(monster_list_any[i] || monster_list_breath[i]))
				     ||
				    (s[2] == 'm' && monster_list_any[i]) ||
				    (s[2] == 'M' && monster_list_breath[i]))) {
					/* hack - found more results than fit on currently displayed page? */
					if (n == MONSTER_LORE_LIST_SIZE) {
						more_results = TRUE;
						break;
					}
					if (skipped < pageoffset * 17) {
						skipped++;
						continue;
					}

					if (n == 0) {
						selected = monster_list_code[i];
						selected_list = i;
					}
#if 0
					Term_putstr(5, 5 + n, -1, n == selected_line ? TERM_YELLOW : TERM_UMBER, format("(%4d, \377%c%c\377%c, L%-3d)  %s",
					    monster_list_code[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u', monster_list_level[i], monster_list_name[i]));
#else
					Term_putstr(5, 5 + n, -1, n == selected_line ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c)  %s",
					    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u', monster_list_name[i]));
#endif
					list_idx[n] = i;
					n++;
				}
			}
		}
		/* Classic: Search for monster name */
		else {
			/* hack 1: direct match always takes top position
			   hack 2: match at beginning of name takes precedence */
			direct_match = FALSE;
			if (s[0] && !pageoffset) for (i = 1; i < monster_list_idx; i++) {
				/* create upper-case working copy */
				strcpy(tmp, monster_list_name[i]);
				for (j = 0; tmp[j]; j++) tmp[j] = toupper(tmp[j]);

				/* exact match? */
				if (!strcmp(tmp, s)
				    || (s[0] == '#' && atoi(s + 1) && monster_list_code[i] == atoi(s + 1))) { /* also allow typing in the monster's ridx directly! */
					if (direct_match) continue; //paranoia -- there should never be two monsters of the exact same name or index number
					direct_match = TRUE;

					selected = monster_list_code[i];
					selected_list = i;

#if 0
					Term_putstr(5, 5, -1, selected_line == 0 ? TERM_YELLOW : TERM_UMBER, format("(%4d, \377%c%c\377%c, L%-3d)  %s",
					    monster_list_code[i], monster_list_symbol[i][0], monster_list_symbol[i][1], selected_line == 0 ? 'y' : 'u', monster_list_level[i], monster_list_name[i]));
#else
					if (n) Term_erase(5, 5, 80); //erase old beginning-of-line match that was shown here first
					Term_putstr(5, 5, -1, selected_line == 0 ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c)  %s",
					    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], selected_line == 0 ? 'y' : 'u', monster_list_name[i]));
#endif

					/* no beginning-of-line match yet? good. */
					if (!n) {
						list_idx[n] = i;
					} else { /* already got a beginning-of-line match? swap them, cause exact match always take pos #0 */
						int tmp = list_idx[0];
						list_idx[0] = i;
						list_idx[n] = tmp;

						/* redisplay the moved choice */
						Term_putstr(5, 5 + n, -1, selected_line == n ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c)  %s",
						    monster_list_code[list_idx[1]], monster_list_level[list_idx[1]], monster_list_symbol[list_idx[1]][0],
						    monster_list_symbol[list_idx[1]][1], selected_line == n ? 'y' : 'u', monster_list_name[list_idx[1]]));
					}
					n++;
					if (n == 2) break;//allow 1 direct + 1 beginning-of-line match
				}
				/* beginning of line match? (only check for two, of which one might already be a direct match (checked above)) */
				else if ((n == 0 || (direct_match && n == 1)) && !strncmp(tmp, s, strlen(s))) {
					selected = monster_list_code[i];
					selected_list = i;
#if 0
					Term_putstr(5, 5 + n, -1, selected_line == 0 ? TERM_YELLOW : TERM_UMBER, format("(%4d, \377%c%c\377%c, L%-3d)  %s",
					    monster_list_code[i], monster_list_symbol[i][0], monster_list_symbol[i][1], selected_line == 0 ? 'y' : 'u', monster_list_level[i], monster_list_name[i]));
#else
					Term_putstr(5, 5 + n, -1, selected_line == n ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c)  %s",
					    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], selected_line == n ? 'y' : 'u', monster_list_name[i]));
#endif
					list_idx[n] = i;
					n++;
					if (direct_match) break;//allow 1 direct + 1 beginning-of-line match
				}
			}
			presorted = n;

			for (i = 1; i < monster_list_idx && n < MONSTER_LORE_LIST_SIZE + 1; i++) { /* '+ 1' for 'more_results' hack */
#if 0
				/* direct match above already? */
				if (i == selected_list) continue;
#else
				/* direct match or beginning-of-line matches above already? */
				for (j = 0; j < presorted; j++) if (i == list_idx[j]) break;
				if (j != presorted) continue;
#endif

				/* create upper-case working copy */
				strcpy(tmp, monster_list_name[i]);
				for (j = 0; tmp[j]; j++) tmp[j] = toupper(tmp[j]);

				if (monster_list_code[i] && strstr(tmp, s)) {
					/* hack - found more results than fit on currently displayed page? */
					if (n == MONSTER_LORE_LIST_SIZE) {
						more_results = TRUE;
						break;
					}
					if (skipped < pageoffset * 17) {
						skipped++;
						continue;
					}

					if (n == 0) {
						selected = monster_list_code[i];
						selected_list = i;
					}
#if 0
					Term_putstr(5, 5 + n, -1, n == selected_line ? TERM_YELLOW : TERM_UMBER, format("(%4d, \377%c%c\377%c, L%-3d)  %s",
					    monster_list_code[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u', monster_list_level[i], monster_list_name[i]));
#else
					Term_putstr(5, 5 + n, -1, n == selected_line ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c)  %s",
					    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u', monster_list_name[i]));
#endif
					list_idx[n] = i;
					n++;
				}
			}
		}

		/* choice outside of the list? snap back */
		if (selected_line >= n) {
			selected_line = n - 1;
			if (selected_line < 0) selected_line = 0;
			else
#if 0
				Term_putstr(5, 5 + selected_line, -1, TERM_YELLOW, format("(%4d, \377%c%c\377y, L%-3d)  %s",
				    monster_list_code[list_idx[selected_line]], monster_list_symbol[list_idx[selected_line]][0],
				    monster_list_symbol[list_idx[selected_line]][1], monster_list_level[list_idx[selected_line]], monster_list_name[list_idx[selected_line]]));
#else
				Term_putstr(5, 5 + selected_line, -1, TERM_YELLOW, format("(%4d, L%-3d, \377%c%c\377y)  %s",
				    monster_list_code[list_idx[selected_line]], monster_list_level[list_idx[selected_line]], monster_list_symbol[list_idx[selected_line]][0],
				    monster_list_symbol[list_idx[selected_line]][1], monster_list_name[list_idx[selected_line]]));
#endif
		}

		if (!s[0])
			Term_putstr(2,  23, -1, TERM_WHITE, "Press ESC to exit, ENTER for lore/stats, Up/Down/PgUp/PgDn/Home to navigate");
		else
			Term_putstr(2,  23, -1, TERM_WHITE, "Press ESC to clear, ENTER for lore/stats, Up/Down/PgUp/PgDn/Home to navigate");
		/* hack: place cursor at pseudo input prompt */
		Term->scr->cx = 53 + strlen(s);
		Term->scr->cy = 2;
		Term->scr->cu = 1;

		inkey_msg_old = inkey_msg;
		inkey_msg = TRUE;
		c = inkey();
		inkey_msg = inkey_msg_old;

		/* specialty: allow chatting from within here */
		if (c == ':') {
			cmd_message();
			continue;
		}
		/* just because you can do this in artifact lore too.. */
		if (c == '{') {
			cmd_inscribe(USE_INVEN | USE_EQUIP);
			continue;
		}
		if (c == '}') {
			cmd_uninscribe(USE_INVEN | USE_EQUIP);
			continue;
		}

		/* navigate in the list (up/down) */
		if (s[0] != '#' && c == '8') { //rl:'k'
			if (n > 0) {
				if (selected_line > 0) selected_line--;
				else selected_line = n - 1;
			}
			continue;
		}
		if (s[0] != '#' && c == '2') { //rl:'j'
			if (n > 0) {
				if (selected_line < n - 1) selected_line++;
				else selected_line = 0;
			}
			continue;
		}

		/* page up/down in the search results */
		if (s[0] != '#' && c == '9') { //rl:?
			if (pageoffset > 0) pageoffset--;
			continue;
		}
		if (s[0] != '#' && c == '3') { //rl:?
			if (pageoffset < 100 /* silyl */
			    && more_results)
				pageoffset++;
			continue;
		}
		/* home key to reset */
		if (s[0] != '#' && c == '7') { //rl:?
			pageoffset = 0;
			continue;
		}

		/* backspace */
		if (c == '\010' || c == 0x7F) {
			if (strlen(s)) {
				s[strlen(s) - 1] = '\0';
				pageoffset = 0;
			}
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
		if (c == ESCAPE) {
			if (!s[0]) break;
			else s[0] = '\0';
		}
		/* illegal char */
		if (c < 32 || c > 127) continue;
		/* name too long? */
		if (strlen(s) >= (s[0] == '!' ? 3 : 20)) continue;
		/* build name */
		if (s[0] != '!') c = toupper(c); /* We're looking for a name, not a symbol */
		strcat(s, format("%c", c));
		pageoffset = 0;
	}

	/* exit? */
	if (c == ESCAPE) {
		Term_load();
		return;
	}

	/* display lore (default) or stats */
	selected_list = list_idx[selected_line];
	selected = monster_list_code[selected_list];
	while (TRUE) {
		clear_from(4);
		//Term_putstr(5, 5, -1, TERM_YELLOW, format("(%4d)  %s", monster_list_code[selected_list], monster_list_name[selected_list]));
		for (i = 0; i < 18; i++) paste_lines[i][0] = '\0';

		if (show_lore) {
			monster_lore_aux(selected, selected_list, paste_lines);
			Term_putstr(5,  23, -1, TERM_WHITE, "-- press ESC/Backspace to exit, SPACE for stats, c/C to chat-paste --");
		} else {
			monster_stats_aux(selected, selected_list, paste_lines);
			Term_putstr(5,  23, -1, TERM_WHITE, "-- press ESC/Backspace to exit, SPACE for lore, c/C to chat-paste --");
		}
		/* hack: hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		while (TRUE) {
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			c = inkey();
			inkey_msg = inkey_msg_old;

			/* specialty: allow chatting from within here */
			if (c == ':') {
				cmd_message();
				Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Monster Lore ***");
			}
			/* just because you can do this in artifact lore too.. */
			if (c == '{') {
				cmd_inscribe(USE_INVEN | USE_EQUIP);
				Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Monster Lore ***");
			}
			if (c == '}') {
				cmd_uninscribe(USE_INVEN | USE_EQUIP);
				Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Monster Lore ***");
			}

			if (c == '\e' || c == '\b') break;
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
			if (c == 'C') {
				/* paste lore AND stats into chat */
				for (i = 0; i < 18; i++) paste_lines[i][0] = '\0';
				monster_lore_aux(selected, selected_list, paste_lines);
				for (i = 0; i < 18; i++) {
					if (!paste_lines[i][0]) break;
					//if (i == 6 || i == 12) usleep(10000000);
					if (paste_lines[i][strlen(paste_lines[i]) - 1] == ' ')
						paste_lines[i][strlen(paste_lines[i]) - 1] = '\0';
					Send_paste_msg(paste_lines[i]);
				}
				/* don't double-post the title: skip paste line 0 */
				for (i = 0; i < 18; i++) paste_lines[i][0] = '\0';
				monster_stats_aux(selected, selected_list, paste_lines);
				for (i = 1; i < 18; i++) {
					if (!paste_lines[i][0]) break;
					//if (i == 6 || i == 12) usleep(10000000);
					if (paste_lines[i][strlen(paste_lines[i]) - 1] == ' ')
						paste_lines[i][strlen(paste_lines[i]) - 1] = '\0';
					Send_paste_msg(paste_lines[i]);
				}
				break;
			}
		}
		/* ESC = go back and erase search term */
		if (c == '\e') {
			s[0] = '\0'; s[1] = 0; s[2] = 0;
			break;
		}
		/* Backspace = go back but keep search term */
		if (c == '\b') break;
	}
    }

	Term_load();
}


/*
 * NOTE: the usage of Send_special_line is quite a hack;
 * it sends a letter in place of cur_line...		- Jir -
 */
void cmd_check_misc(void) {
	char i = 0, choice;
	int second = 10;
	/* suppress hybrid macros in some submenus */
	bool inkey_msg_old;

	Term_save();
	Term_clear();
	//Term_putstr(0,  0, -1, TERM_BLUE, "Display current knowledge");

	Term_putstr( 5,  3, -1, TERM_WHITE, "(\377y1\377w) Artifacts found");
	Term_putstr( 5,  4, -1, TERM_WHITE, "(\377y2\377w) Monsters killed");
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
	Term_putstr( 5, second + 2, -1, TERM_WHITE, "(\377yc\377w) High Scores");
	Term_putstr( 5, second + 3, -1, TERM_WHITE, "(\377yd\377w) Server settings");
	Term_putstr( 5, second + 4, -1, TERM_WHITE, "(\377ye\377w) Opinions (if available)");
	Term_putstr(40, second + 0, -1, TERM_WHITE, "(\377yf\377w) News (Message of the day)");
	Term_putstr(40, second + 1, -1, TERM_WHITE, "(\377yh\377w) Intro screen");
	Term_putstr(40, second + 2, -1, TERM_WHITE, "(\377yi\377w) Message history");
	Term_putstr(40, second + 3, -1, TERM_WHITE, "(\377yj\377w) Chat history");
	Term_putstr(40, second + 4, -1, TERM_WHITE, "(\377yl\377w) Lag-o-meter");
	Term_putstr( 5, second + 6, -1, TERM_WHITE, "(\377y?\377w) Help");
	Term_putstr(40, second + 6, -1, TERM_WHITE, "(\377yg\377w) The Guide");
	Term_putstr(20, second + 8, -1, TERM_WHITE, "\377s(Type \377y/ex\377s in chat to view extra character information)");

	Term_putstr(0, 22, -1, TERM_BLUE, "Command: ");

	while (i != ESCAPE) {
		Term_putstr(0,  0, -1, TERM_BLUE, "Display current knowledge");
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		i = inkey();
		choice = 0;
		switch (i) {
			case '3':
				/* Send it */
				cmd_uniques();
				break;
			case '1':
				cmd_artifacts();
				break;
			case '2':
				inkey_msg_old = inkey_msg;
				inkey_msg = TRUE;
				get_com("What kind of monsters? (ESC for all, @ for learnt):", &choice);
				inkey_msg = inkey_msg_old;
				if (choice <= ESCAPE) choice = 0;

				/* allow specifying minlev? */
				if (is_newer_than(&server_version, 4, 5, 4, 0, 0, 0)) {
					second = c_get_quantity("Specify minimum level? (ESC for none): ", -1);
					Send_special_line(SPECIAL_FILE_MONSTER, choice + second * 100000);
				} else
					Send_special_line(SPECIAL_FILE_MONSTER, choice);
				break;
			case '4':
				inkey_msg_old = inkey_msg;
				inkey_msg = TRUE;
				get_com("What type of objects? (ESC for all):", &choice);
				inkey_msg = inkey_msg_old;
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
				Send_special_line(SPECIAL_FILE_MOTD2, 0);
				break;
			case 'h':
				show_motd(0);
				break;
			case 'i':
				do_cmd_messages();
				break;
			case 'j':
				do_cmd_messages_chatonly();
				break;
			case 'l':
				cmd_lagometer();
				break;
			case '?':
				cmd_help();
				break;
			case 'g':
				cmd_the_guide();
				break;
			case ESCAPE:
			case KTRL('Q'):
				break;
			case KTRL('T'):
				xhtml_screenshot("screenshot????");
				break;
			case ':':
				cmd_message();
				break;
			default:
				bell();
		}
	}
	Term_load();
}

void cmd_message(void) {
	/* _hacky_: A note INCLUDES the sender name, the brackets, spaces/newlines? Ouch. - C. Blue
	   Note: the -8 are additional world server tax (8 chars are used for world server line prefix etc.) */
	char buf[MSG_LEN - strlen(cname) - 5 - 3 - 8];
	char tmp[MSG_LEN];//, c = 'B';
	int i;
	int j;
	/* for store-item pasting: */
	char where[17], item[MSG_LEN];
	bool store_item = FALSE;

	/* BIG_MAP leads to big shops */
	bool big_shop = (screen_hgt > SCREEN_HGT);//14 more lines, so alphabet is fully used a)..z)

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
		for (i = 0; i < strlen(buf); i++) {
			/* paste inven/equip item:  \\<slot>  */
			if ((i == 0 || buf[i - 1] != '\\') && /* don't confuse with '\\\' store pasting */
			    buf[i] == '\\' &&
			    buf[i + 1] == '\\' &&
			    ((buf[i + 2] >= 'a' && buf[i + 2] < 'a' + INVEN_PACK) ||
			    (buf[i + 2] >= 'A' && buf[i + 2] < 'A' + (INVEN_TOTAL - INVEN_WIELD)))) {
				if (buf[i + 2] >= 'a') j = buf[i + 2] - 'a';
				else j = (buf[i + 2] - 'A') + INVEN_WIELD;

				/* prevent buffer overflow */
				if (strlen(buf) - 3 + strlen(inventory_name[j]) + 2 + 1 + 1 < MSG_LEN) {
					strcpy(tmp, &buf[i + 3]);
					strcpy(&buf[i], "\377s");

					/* if item inscriptions contains a colon we might need
					   another colon to prevent confusing it with a private message */
					strcat(buf, inventory_name[j]);
					if (strchr(inventory_name[j], ':')) {
						buf[strchr(inventory_name[j], ':') - inventory_name[j] + strlen(buf) - strlen(inventory_name[j]) + 1] = '\0';
						if (strchr(buf, ':') == &buf[strlen(buf) - 1])
							strcat(buf, ":");
						strcat(buf, strchr(inventory_name[j], ':') + 1);
					}

					if (tmp[0]) {
						/* if someone just spams \\ shortcuts, at least add spaces */
						if (tmp[0] == '\\') strcat(buf, " ");

						strcat(buf, "\377-");
						strncat(buf, tmp, sizeof(buf) - strlen(buf) - 1);
					}
				}
				/* just discard */
				else {
					strcpy(tmp, &buf[i + 3]);
					buf[i] = '\0';
					strcat(buf, tmp);
					i--;
				}
			}
			/* paste store item:  \\\<slot>  */
			else if (buf[i] == '\\' && buf[i + 1] == '\\' && buf[i + 2] == '\\' &&
			    buf[i + 3] >= 'a' && buf[i + 3] <= (big_shop ? 'z' : 'l')) {
				j = buf[i + 3] - 'a' + store_top;
				store_paste_item(item, j);
				store_paste_where(where);

				/* just discard if we're not in a shop */
				if (buf[i + 3] < 'a' + store.stock_num &&
				    /* prevent buffer overflow */
				    strlen(buf) - 4 + strlen(where) + 2 + 1 + 1 + strlen(item) + 1 < MSG_LEN) {
					strcpy(tmp, &buf[i + 4]);

					/* add store location/symbol, only once per chat message */
					if (!store_item) {
						store_item = TRUE;
						strcpy(&buf[i], where);

						/* need double-colon to prevent confusing it with a private message? */
						if (strchr(buf, ':') >= &buf[i + strlen(where) - 1])
							strcat(buf, ":");

						strcat(buf, " ");
					} else strcpy(&buf[i], "\377s");

					/* add actual item */
					strcat(buf, item);

					/* any more string to append? */
					if (tmp[0]) {
						/* if someone just spams \\\ shortcuts, at least add spaces */
						if (tmp[0] == '\\') strcat(buf, " ");

						strcat(buf, "\377-");
						strncat(buf, tmp, sizeof(buf) - strlen(buf) - 1);
					}
				}
				/* just discard */
				else {
					strcpy(tmp, &buf[i + 4]);
					buf[i] = '\0';
					strcat(buf, tmp);
					i--;
				}
			}
		}

		/* Handle messages to '%' (self) in the client - mikaelh */
		if (prefix(buf, "%:") && !prefix(buf, "%::")) {
			c_msg_format("\377o<%%>\377w %s", buf + 2);
			inkey_msg = FALSE;
			return;
		}

		Send_msg(buf);
	}

	inkey_msg = FALSE;
}

/* set flags and options such as minlvl / auto-rejoin */
static void cmd_guild_options() {
	int i, acnt;
	char buf[(NAME_LEN + 1) * 5 + 1], buf0[NAME_LEN + 1];

	/* We are now in party mode */
	guildcfg_mode = TRUE;

	/* Save screen */
	Term_save();

	/* Process requests until done */
	while (1) {
		/* Clear screen */
		Term_clear();

		/* Initialize buffer */
		buf[0] = '\0';
		acnt = 0;

		/* Describe */
		Term_putstr(0, 0, -1, TERM_L_UMBER, "============== Guild configuration ===============");

		if (!guild_info_name[0]) {
			Term_putstr(5, 2, -1, TERM_L_UMBER, "You are not in a guild.");
		} else {
			/* Selections */
			if (guildhall_wx == -1) Term_putstr(5, 2, -1, TERM_SLATE, "The guild does not own a guild hall.");
			else if (guildhall_wx >= 0) Term_putstr(5, 2, -1, TERM_L_UMBER, format("The guild hall is located in the %s area of (%d,%d).",
			    guildhall_pos, guildhall_wx, guildhall_wy));
			Term_putstr(5, 4, -1, TERM_WHITE,  format("adders     : %s", guild.flags & GFLG_ALLOW_ADDERS ? "\377GYES" : "\377rno "));
			Term_putstr(5, 5, -1, TERM_L_WHITE,       "    Allows players designated via /guild_adder command to add others.");
			Term_putstr(5, 6, -1, TERM_WHITE,  format("autoreadd  : %s", guild.flags & GFLG_AUTO_READD ? "\377GYES" : "\377rno "));
			Term_putstr(5, 7, -1, TERM_L_WHITE,      "    If a guild mate ghost-dies then the next character he logs on with");
			Term_putstr(5, 8, -1, TERM_L_WHITE,      "    - if it is newly created - is automatically added to the guild again.");
			Term_putstr(5, 9, -1, TERM_WHITE, format("minlev     : \377%c%d   ", guild.minlev <= 1 ? 'w' : (guild.minlev <= 10 ? 'G' : (guild.minlev < 20 ? 'g' :
			    (guild.minlev < 30 ? 'y' : (guild.minlev < 40 ? 'o' : (guild.minlev <= 50 ? 'r' : 'v'))))), guild.minlev));
			Term_putstr(5, 10, -1, TERM_L_WHITE,      "    Minimum character level required to get added to the guild.");

			Term_erase(5, 11, 69);
			Term_erase(5, 12, 69);

			for (i = 0; i < 5; i++) if (guild.adder[i][0] != '\0') {
				sprintf(buf, "Adders are: ");
				strcat(buf, guild.adder[i]);
				acnt++;
				for (i++; i < 5; i++) {
					if (guild.adder[i][0] == '\0') continue;
					if (acnt != 3) strcat(buf, ", ");
					strcat(buf, guild.adder[i]);
					acnt++;
					if (acnt == 3) {
						Term_putstr(5, 11, -1, TERM_SLATE, buf);
						buf[0] = 0;
					}
				}
			}
			Term_putstr(5 + (acnt <= 3 ? 0 : 12), acnt <= 3 ? 11 : 12, -1, TERM_SLATE, buf);

			/* Display commands for changing options */
			if (guild_master) {
				Term_putstr(5, 16, -1, TERM_WHITE, "(\377U1\377w) Toggle 'enable-adders' flag");
				Term_putstr(5, 17, -1, TERM_WHITE, "(\377U2\377w) Toggle 'auto-re-add' flag");
				Term_putstr(5, 18, -1, TERM_WHITE, "(\377U3\377w) Set minimum level required to join the guild");
				Term_putstr(5, 19, -1, TERM_WHITE, "(\377U4\377w) Add/remove 'adders' who may add players in your stead");
			}
		}

		/* Prompt */
		Term_putstr(0, 21, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE || i == KTRL('Q')) break;

		/* Take a screenshot */
		else if (i == KTRL('T'))
			xhtml_screenshot("screenshot????");

		else if (guild_master && i == '1') {
			if (guild.flags & GFLG_ALLOW_ADDERS) {
				Send_guild_config(0, guild.flags & ~GFLG_ALLOW_ADDERS, "");
			} else {
				Send_guild_config(0, guild.flags | GFLG_ALLOW_ADDERS, "");
			}
		} else if (guild_master && i == '2') {
			if (guild.flags & GFLG_AUTO_READD) {
				Send_guild_config(0, guild.flags & ~GFLG_AUTO_READD, "");
			} else {
				Send_guild_config(0, guild.flags | GFLG_AUTO_READD, "");
			}
		} else if (guild_master && i == '3') {
			strcpy(buf0, "0");
			if (!get_string("Specify new minimum level: ", buf0, 4)) continue;
			i = atoi(buf0);
			Send_guild_config(1, i, "");
		} else if (guild_master && i == '4') {
			if (!get_string("Specify player name: ", buf0, NAME_LEN)) continue;
			Send_guild_config(2, -1, buf0);
		}

		/* Oops */
		else {
			/* Ring bell */
			bell();
		}

		/* Flush messages */
		clear_topline_forced();
	}

	/* Reload screen */
	Term_load();

	/* No longer in party mode */
	guildcfg_mode = FALSE;

	/* Flush any events */
	Flush_queue();

	return;
}

void cmd_party(void) {
	char i;
	char buf[80];

	/* suppress hybrid macros */
	inkey_msg = TRUE;

	/* We are now in party mode */
	party_mode = TRUE;

	/* Save screen */
	Term_save();

	/* Process requests until done */
	while (1) {
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
		if (is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
			if (party_info_name[0]) Term_putstr(5, 4, -1, TERM_WHITE, "(\377G3\377w) Add a player to party");
			else Term_putstr(5, 4, -1, TERM_WHITE, "(\377G3\377w) Add yourself to party");
		} else Term_putstr(5, 4, -1, TERM_WHITE, "(\377G3\377w) Add a player to party");
		Term_putstr(5, 5, -1, TERM_WHITE, "(\377G4\377w) Delete a player from party");
		Term_putstr(5, 6, -1, TERM_WHITE, "(\377G5\377w) Leave your current party");
		if ((party_info_mode & PA_IRONTEAM) && !(party_info_mode & PA_IRONTEAM_CLOSED))
			Term_putstr(40, 6, -1, TERM_WHITE, "(\377G0\377w) Close your iron team");
		else
			Term_putstr(40, 6, -1, TERM_WHITE, "                              ");

		Term_putstr(5, 8, -1, TERM_WHITE, "(\377Ua\377w) Create a new guild");
		if (is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
			if (guild_info_name[0]) Term_putstr(5, 9, -1, TERM_WHITE, "(\377Ub\377w) Add a player to guild");
			else Term_putstr(5, 9, -1, TERM_WHITE, "(\377Ub\377w) Add yourself to guild");
		} else Term_putstr(5, 9, -1, TERM_WHITE, "(\377Ub\377w) Add a player to guild");
		Term_putstr(5, 10, -1, TERM_WHITE, "(\377UC\377w) Remove player from guild");
		Term_putstr(5, 11, -1, TERM_WHITE, "(\377UD\377w) Leave guild");
		if (is_newer_than(&server_version, 4, 5, 2, 0, 0, 0))
			Term_putstr(5, 12, -1, TERM_WHITE, "(\377Ue\377w) Set/view guild options");

		if (!s_NO_PK) {
			Term_putstr(5, 14, -1, TERM_WHITE, "(\377RA\377w) Declare war on player/party (not recommended!)");
			Term_putstr(5, 15, -1, TERM_WHITE, "(\377gP\377w) Make peace with player");
		}

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
		if (i == ESCAPE || i == KTRL('Q')) break;

		/* Take a screenshot */
		else if (i == KTRL('T'))
			xhtml_screenshot("screenshot????");

		/* Create party */
		else if (i == '1') {
			/* Get party name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Enter party name: ");
			if (askfor_aux(buf, 79, 0)) Send_party(PARTY_CREATE, buf);
		}

		/* Create 'Iron Team' party */
		else if (i == '2') {
			/* Get party name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Enter party name: ");
			if (askfor_aux(buf, 79, 0)) Send_party(PARTY_CREATE_IRONTEAM, buf);
		}

		/* Add player */
		else if (i == '3') {
			if (party_info_name[0]) {
				/* Get player name */
				Term_putstr(0, 19, -1, TERM_YELLOW, "Enter player name: ");
				if (askfor_aux(buf, 79, 0)) Send_party(PARTY_ADD, buf);
			} else {
				/* Get party name */
				Term_putstr(0, 19, -1, TERM_YELLOW, "Enter party name: ");
				if (askfor_aux(buf, 79, 0)) Send_party(PARTY_ADD, buf);
			}
		}

		/* Delete player */
		else if (i == '4') {
			/* Get player name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Enter player name: ");
			if (askfor_aux(buf, 79, 0)) Send_party(PARTY_DELETE, buf);
		}

		/* Leave party */
		else if (i == '5') {
			/* Send the command */
			Send_party(PARTY_REMOVE_ME, "");
		}

		else if (i == '0' && (party_info_mode & PA_IRONTEAM) && !(party_info_mode & PA_IRONTEAM_CLOSED)) {
			if (get_check2("Close your Iron Team?", FALSE))
				Send_party(PARTY_CLOSE, "");
		}

		/* Attack player/party */
		else if (i == 'A' && !s_NO_PK) {
			/* Get player name */
			Term_putstr(0, 19, -1, TERM_L_RED, "Enter player/party to attack: ");
			if (askfor_aux(buf, 79, 0)) Send_party(PARTY_HOSTILE, buf);
		}
		/* Make peace with player/party */
		else if (i == 'P' && !s_NO_PK) {
			/* Get player/party name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Make peace with: ");
			if (askfor_aux(buf, 79, 0)) Send_party(PARTY_PEACE, buf);
		}

		/* Guild options */
		else if (i == 'a') {
			/* Get new guild name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Enter guild name: ");
			if (askfor_aux(buf, 79, 0)) Send_guild(GUILD_CREATE, buf);
		} else if (i == 'b') {
			if (guild_info_name[0]) {
				/* Get player name */
				Term_putstr(0, 19, -1, TERM_YELLOW, "Enter player name: ");
				if (askfor_aux(buf, 79, 0)) Send_guild(GUILD_ADD, buf);
			} else {
				/* Get guild name */
				Term_putstr(0, 19, -1, TERM_YELLOW, "Enter guild name: ");
				if (askfor_aux(buf, 79, 0)) Send_guild(GUILD_ADD, buf);
			}
		} else if (i == 'C') {
			/* Get player name */
			Term_putstr(0, 19, -1, TERM_YELLOW, "Enter player name: ");
			if (askfor_aux(buf, 79, 0)) Send_guild(GUILD_DELETE, buf);
		} else if (i == 'D') {
			if (get_check2("Leave the guild?", FALSE))
				Send_guild(GUILD_REMOVE_ME, "");
		} else if (i == 'e' && is_newer_than(&server_version, 4, 5, 2, 0, 0, 0)) {
			/* Set guild flags/options */
			cmd_guild_options();
		}

		/* Oops */
		else {
			/* Ring bell */
			bell();
		}

		/* Flush messages */
		clear_topline_forced();
	}

	/* Reload screen */
	Term_load();

	/* No longer in party mode */
	party_mode = FALSE;

	/* restore responsiveness to hybrid macros */
	inkey_msg = FALSE;

	/* Flush any events */
	Flush_queue();
}


void cmd_fire(void) {
	int dir;

	if (!get_dir(&dir))
		return;

	/* Send it */
	Send_fire(dir);
}

void cmd_throw(void) {
	int item, dir;

	item_tester_hook = NULL;
	get_item_extra_hook = get_item_hook_find_obj;
	get_item_hook_find_obj_what = "Item name? ";

	if (!c_get_item(&item, "Throw what? ", (USE_INVEN | USE_EXTRA)))
		return;

	if (!get_dir(&dir))
		return;

	/* Send it */
	Send_throw(item, dir);
}

static bool item_tester_browsable(object_type *o_ptr) {
	return (is_realm_book(o_ptr) || o_ptr->tval == TV_BOOK);
}

void cmd_browse(void) {
	int item;
	object_type *o_ptr;

/* commented out because first, admins are usually ghosts;
   second, we might want a 'ghost' tome or something later,
   kind of to bring back 'undead powers' :) - C. Blue */
#if 0
	if (p_ptr->ghost) {
		show_browse(NULL);
		return;
	}
#endif

	get_item_hook_find_obj_what = "Book name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	item_tester_hook = item_tester_browsable;

	if (!c_get_item(&item, "Browse which book? ", (USE_INVEN | USE_EXTRA | NO_FAIL_MSG))) {
		if (item == -2) c_msg_print("You have no books that you can read.");
		return;
	}

	o_ptr = &inventory[item];

	if (o_ptr->tval == TV_BOOK) {
		browse_school_spell(item, o_ptr->sval, o_ptr->pval);
		return;
	}

	/* Show it */
	show_browse(o_ptr);
}

void cmd_ghost(void) {
	if (p_ptr->ghost) do_ghost();
	else c_msg_print("You are not undead.");
}

#if 0 /* done by cmd_telekinesis */
void cmd_mind(void) {
	Send_mind();
}
#endif

void cmd_load_pref(void) {
	char buf[80];

	buf[0] = '\0';

	if (get_string("Load pref: ", buf, 79))
		process_pref_file_aux(buf);
}

void cmd_redraw(void) {
	Send_redraw(0);

#if 0 /* I think this is useless here - mikaelh */
	keymap_init();
#endif
}

static void cmd_house_chown(int dir) {
	char i = 0;
	char buf[80];

	Term_clear();
	Term_putstr(0, 2, -1, TERM_BLUE, "Select owner type");
	Term_putstr(5, 4, -1, TERM_WHITE, "(1) Player");
	Term_putstr(5, 5, -1, TERM_WHITE, "(2) Guild");

	while (i != ESCAPE) {
		i = inkey();
		switch (i) {
		case '1':
			buf[0] = 'O';
			buf[1] = i;
			buf[2] = 0;
			if (get_string("Enter new name:", &buf[2], 60))
				Send_admin_house(dir, buf);
			return;
		case '2':
			buf[0] = 'O';
			buf[1] = '2';
			buf[2] = 0;
			Send_admin_house(dir, buf);
			return;
		case ESCAPE:
		case KTRL('Q'):
			break;
		case KTRL('T'):
			xhtml_screenshot("screenshot????");
			break;
		default:
			bell();
		}
		clear_topline_forced();
	}
}

static void cmd_house_chmod(int dir) {
	char buf[80];
	char mod = ACF_NONE;
	u16b minlev = 0;
	Term_clear();
	Term_putstr(0, 2, -1, TERM_BLUE, "Set new permissions");
	if (get_check2("Allow party access?", FALSE)) mod |= ACF_PARTY;
	if (get_check2("Restrict access to class?", FALSE)) mod |= ACF_CLASS;
	if (get_check2("Restrict access to race?", FALSE)) mod |= ACF_RACE;
	if (get_check2("Restrict access to winners?", FALSE)) mod |= ACF_WINNER;
	if (get_check2("Restrict access to fallen winners?", FALSE)) mod |= ACF_FALLENWINNER;
	if (get_check2("Restrict access to no-ghost players?", FALSE)) mod |= ACF_NOGHOST;
	minlev = c_get_quantity("Minimum level: ", 127);
	if (minlev > 1) mod |= ACF_LEVEL;
	buf[0] = 'M';
	if ((buf[1] = mod))
		sprintf(&buf[2], "%hd", minlev);
	Send_admin_house(dir, buf);
}

static void cmd_house_kill(int dir) {
	if (get_check2("Are you sure you really want to destroy the house?", FALSE))
		Send_admin_house(dir, "K");
}

static void cmd_house_store(int dir) {
	Send_admin_house(dir, "S");
}

static void cmd_house_paint(int dir) {
	char buf[80];
	int item;

	item_tester_hook = item_tester_quaffable;
	get_item_hook_find_obj_what = "Potion name? ";
	get_item_extra_hook = get_item_hook_find_obj;
	if (!c_get_item(&item, "Use which potion for colouring? ", (USE_INVEN | USE_EXTRA | NO_FAIL_MSG))) {
		if (item == -2) c_msg_print("You need a potion to extract the colour from!");
		return;
	}

	buf[0] = 'P';
	sprintf(&buf[1], "%d", item);
	Send_admin_house(dir, buf);
}

static void cmd_house_knock(int dir) {
	char buf[80];

	buf[0] = 'H';
	buf[1] = '\0';
	Send_admin_house(dir, buf);
}

void cmd_purchase_house(void) {
	char i = 0;
	int dir;

	if (!get_dir(&dir)) return;

	Term_save();
	Term_clear();
	Term_putstr(0, 2, -1, TERM_BLUE, "House commands");
	Term_putstr(5, 4, -1, TERM_WHITE, "(1) Buy/Sell house");
	Term_putstr(5, 5, -1, TERM_WHITE, "(2) Change house owner");
	Term_putstr(5, 6, -1, TERM_WHITE, "(3) Change house permissions");
	Term_putstr(5, 7, -1, TERM_WHITE, "(4) Paint house");/* new in 4.4.6: */
	/* display in dark colour since only admins can do this really */
	Term_putstr(5, 9, -1, TERM_WHITE, "(s) Enter player store");/* new in 4.4.6: */
	Term_putstr(5, 10, -1, TERM_WHITE, "(k) Knock on house door");
	Term_putstr(5, 20, -1, TERM_L_DARK, "(D) Delete house (server administrators only)");

	while (i != ESCAPE) {
		i = inkey();
		switch (i) {
		case '1':
			/* Confirm */
			if (get_check2("Are you sure you really want to buy or sell the house?", FALSE)) {
				/* Send it */
				Send_purchase_house(dir);
				i = ESCAPE;
			}
			break;
		case '2':
			cmd_house_chown(dir);
			i = ESCAPE;
			break;
		case '3':
			cmd_house_chmod(dir);
			i = ESCAPE;
			break;
		case '4':
			cmd_house_paint(dir);
			i = ESCAPE;
			break;
		case 'D':
			cmd_house_kill(dir);
			i = ESCAPE;
			break;
		case 's':
			cmd_house_store(dir);
			i = ESCAPE;
			break;
		case 'k':
			cmd_house_knock(dir);
			i = ESCAPE;
			break;
		case ESCAPE:
		case KTRL('Q'):
			break;
		case KTRL('T'):
			xhtml_screenshot("screenshot????");
			break;
		default:
			bell();
		}
		clear_topline_forced();
	}
	Term_load();
}

void cmd_suicide(void) {
	int i;

	/* Verify */
	if (!get_check2("Do you really want to commit suicide?", FALSE)) return;

	/* Check again */
	prt("Please verify SUICIDE by typing the '@' sign: ", 0, 0);
	flush();
	i = inkey();
	clear_topline();
	if (i != '@') return;

	/* Send it */
	Send_suicide();
}

static void cmd_master_aux_level(void) {
	char i;
	char buf[80];

	/* Process requests until done */
	while (1) {
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
			buf[3] = (get_check2("Is it a tower?", FALSE) ? 't' : 'd');
			/*
			 * FIXME: flags are u32b while buf[] is char!
			 * This *REALLY* should be rewritten	- Jir -
			 */
			if (get_check2("Random dungeon (default)?", TRUE)) buf[5] |= 0x02;//DF2_RANDOM
			if (get_check2("Hellish?", FALSE)) buf[5] |= 0x04;//DF2_HELL
			if (get_check2("Not mappable?", FALSE)) {
				buf[4] |= 0x02;//DF1_FORGET
				buf[5] |= 0x08;//DF2_NO_MAGIC_MAP
			}
			if (get_check2("Ironman?", TRUE)) {
				buf[5] |= 0x10;//DF2_IRON
				i = 0;
				if (get_check2("Recallable from, before reaching its end?", FALSE)) {
					if (get_check2("Random recall depth intervals (y) or fixed ones (n) ?", TRUE)) buf[6] |= 0x08;
					i = c_get_quantity("Frequency (random)? (1=often..4=rare): ", 4);
					switch (i) {
					case 1: buf[6] |= 0x10; break;//DF2_IRONRNDn / DF2_IRONFIXn
					case 2: buf[6] |= 0x20; break;
					case 3: buf[6] |= 0x40; break;
					default: buf[6] |= 0x80;
					}
					i = 1; // hack for towns below
				}
				if (get_check2("Generate towns inbetween?", FALSE)) {
					if (i == 1 && get_check2("Generate towns when premature recall is allowed?", FALSE)) {
						buf[5] |= 0x20;//DF2_TOWNS_IRONRECALL
					} else if (get_check2("Generate towns randomly (y) or in fixed intervals (n) ?", TRUE)) {
						buf[5] |= 0x40;//DF2_TOWNS_RND
					} else buf[5] |= 0x80;//DF2_TOWNS_FIX
				}
			}
			if (get_check2("Disallow generation of simple stores (misc iron + low level)?", FALSE)) {
				buf[4] |= 0x08;//DF3_NO_SIMPLE_STORES
				if (get_check2("Generate at least the hidden library?", FALSE)) buf[4] |= 0x04;//DF3_HIDDENLIB
			} else if (get_check2("Generate misc iron stores (RPG rules style)?", FALSE)) {
				buf[6] |= 0x04;//DF2_MISC_STORES
			} else if (get_check2("Generate at least the hidden library?", FALSE)) buf[4] |= 0x04;//DF3_HIDDENLIB
			if (is_newer_than(&server_version, 4, 5, 6, 0, 0, 1))
				buf[7] = c_get_quantity("Theme (0 = default vanilla): ", -1);
			else	buf[7] = 0;
			buf[8] = '\0';
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
		else {
			/* Ring bell */
			bell();
		}

		/* Flush messages */
		clear_topline_forced();
	}
}

static void cmd_master_aux_generate_vault(void) {
	char i, redo_hack;
	char buf[80];

	/* Process requests until done */
	while (1) {
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
		if (i == KTRL('T')) xhtml_screenshot("screenshot????");

		/* Generate by number */
		else if (i == '1') {
			buf[1] = '#';
			buf[2] = c_get_quantity("Vault number? ", 255) - 127;
			if(!buf[2]) redo_hack = 1;
			buf[3] = 0;
		}

		/* Generate by name */
		else if (i == '2') {
			buf[1] = 'n';
			get_string("Enter vault name: ", &buf[2], 77);
			if(!buf[2]) redo_hack = 1;
		}

		/* Oops */
		else {
			/* Ring bell */
			bell();
			redo_hack = 1;
		}

		/* hack -- don't do this if we hit an invalid key previously */
		if(redo_hack) continue;

		Send_master(MASTER_GENERATE, buf);

		/* Flush messages */
		clear_topline_forced();
	}
}

static void cmd_master_aux_generate(void) {
	char i;

	/* Process requests until done */
	while (1) {
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
		else if (i == KTRL('T')) xhtml_screenshot("screenshot????");

		/* Generate a vault */
		else if (i == '1') cmd_master_aux_generate_vault();

		/* Oops */
		else
			/* Ring bell */
			bell();

		/* Flush messages */
		clear_topline_forced();
	}
}



static void cmd_master_aux_build(void) {
	char i;
	char buf[80];

	/* Process requests until done */
	while (1) {
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

		switch (i) {
		/* Take a screenshot */
		case KTRL('T'):
			xhtml_screenshot("screenshot????");
			break;
		/* Granite mode on */
		case '1': buf[0] = FEAT_WALL_EXTRA; break;
		/* Perm mode on */
		case '2': buf[0] = FEAT_PERM_EXTRA; break;
		/* Tree mode on */
		case '3': buf[0] = magik(80)?FEAT_TREE:(char)FEAT_BUSH; break;
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
				keyid = c_get_quantity("Enter key pval:", 0xffff);
				sprintf(&buf[2], "%d", keyid);
			}
			break;
		/* Sign post */
		case '9':
			buf[0] = FEAT_SIGN;
			get_string("Sign:", &buf[2], 77);
			break;
		/* Ask for feature */
		case '0':
			buf[0] = c_get_quantity("Enter feature value:",0xff);
			break;
		/* Build mode off */
		case 'a': buf[0] = FEAT_FLOOR; buf[1] = 'F'; break;
		/* Oops */
		default : bell(); break;
		}

		/* If we got a valid command, send it */
		if (buf[0]) Send_master(MASTER_BUILD, buf);

		/* Flush messages */
		clear_topline_forced();
	}
}

static char * cmd_master_aux_summon_orcs(void) {
	char i;
	static char buf[80];

	/* Process requests until done */
	while (1) {
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
		switch (i) {
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
		clear_topline_forced();
	}

	/* escape was pressed, no valid orcs types specified */
	return NULL;
}

static char * cmd_master_aux_summon_undead_low(void) {
	char i;
	static char buf[80];

	/* Process requests until done */
	while (1) {
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
		switch (i) {
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
		clear_topline_forced();
	}

	/* escape was pressed, no valid types specified */
	return NULL;
}


static char * cmd_master_aux_summon_undead_high(void) {
	char i;
	static char buf[80];

	/* Process requests until done */
	while (1) {
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
		switch (i) {
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
		clear_topline_forced();
	}

	/* escape was pressed, no valid orcs types specified */
	return NULL;
}


static void cmd_master_aux_summon(void) {
	char i, redo_hack;
	char buf[80];
	char * race_name;

	/* Process requests until done */
	while (1) {
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
		Term_putstr(5, 9, -1, TERM_WHITE, "(6) Obliteration");
		Term_putstr(5, 10, -1, TERM_WHITE, "(7) Summoning mode off");



		/* Prompt */
		Term_putstr(0, 13, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		/* get the type of monster to summon */
		switch (i) {
		/* Take a screenshot */
		case KTRL('T'):
			xhtml_screenshot("screenshot????");
			break;
		/* orc menu */
		case '1':
			/* get the specific kind of orc */
			race_name = cmd_master_aux_summon_orcs();
			/* if no string was specified */
			if (!race_name) {
				redo_hack = 1;
				break;
			}
			buf[2] = 'o';
			strcpy(&buf[3], race_name);
			break;
		/* low undead menu */
		case '2':
			/* get the specific kind of low undead */
			race_name = cmd_master_aux_summon_undead_low();
			/* if no string was specified */
			if (!race_name) {
				redo_hack = 1;
				break;
			}
			buf[2] = 'u';
			strcpy(&buf[3], race_name);
			break;
		/* high undead menu */
		case '3':
			/* get the specific kind of low undead */
			race_name = cmd_master_aux_summon_undead_high();
			/* if no string was specified */
			if (!race_name) {
				redo_hack = 1;
				break;
			}
			buf[2] = 'U';
			strcpy(&buf[3], race_name);
			break;
		/* summon from a specific depth */
		case '4':
			buf[2] = 'd';
			buf[3] = c_get_quantity("Summon from which depth? ", 127);
			/* if (!buf[3]) redo_hack = 1; - Allow depth 0 hereby. */
			buf[4] = 0; /* terminate the string */
			break;
		/* summon a specific monster or character */
		case '5':
			buf[2] = 's';
			buf[3] = 0;
			get_string("Summon which monster or character? ", &buf[3], 79 - 3);
			if (!buf[3]) redo_hack = 1;
			break;

		case '6':
			/* delete all the monsters near us */
			/* turn summoning mode on */
			buf[0] = 'T';
			buf[1] = 1;
			buf[2] = '0';
			buf[3] = '\0'; /* null terminate the monster name */
			Send_master(MASTER_SUMMON, buf);

			redo_hack = 1;
			break;

		case '7':
			/* disable summoning mode */
			buf[0] = 'F';
			buf[3] = '\0'; /* null terminate the monster name */
			Send_master(MASTER_SUMMON, buf);

			redo_hack = 1;
			break;

		/* Oops */
		default : bell(); redo_hack = 1; break;
		}

		/* get how it should be summoned */

		/* hack -- make sure our method is unset so we only send
		 * a monster summon request if we get a valid summoning type
		 */

		/* hack -- don't do this if we hit an invalid key previously */
		if (redo_hack) continue;

		while (1) {
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
			switch (i) {
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
		clear_topline_forced();
	}
}

static void cmd_master_aux_player() {
	char i = 0;
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

	while (i != ESCAPE) {
		/* Get a key */
		i = inkey();
		buf[0] = '\0';
		switch (i) {
		case KTRL('T'):
			xhtml_screenshot("screenshot????");
			break;
		case '1':
			buf[0] = 'E';
			get_string("Enter player name:", &buf[1], 15);
			break;
		case '2':
			buf[0] = 'A';
			get_string("Enter player name:", &buf[1], 15);
			break;
		case '3':
			buf[0] = 'k';
			get_string("Enter player name:", &buf[1], 15);
			break;
		case '4':
			buf[0] = 'S';
			get_string("Enter player name:", &buf[1], 15);
			break;
		case '5':
			buf[0] = 'U';
			get_string("Enter player name:", &buf[1], 15);
			break;
		case '6':
			buf[0] = 'r';
			get_string("Enter player name:", &buf[1], 15);
			break;
		case '7':
			/* DM to player telekinesis */
			buf[0] = 't';
			get_string("Enter player name:", &buf[1], 15);
			break;
		case '8':
			{
				int j;
				buf[0] = 'B';
				get_string("Message:", &buf[1], 69);
				for (j = 0; j < 60; j++)
					if (buf[j] == '{') buf[j] = '\377';
			}
			break;
		case ESCAPE:
			break;
		default:
			bell();
		}
		if (buf[0]) Send_master(MASTER_PLAYER, buf);

		/* Flush messages */
		clear_topline_forced();
	}
}

/*
 * Upload/execute scripts
 */
/* TODO: up-to-date check and download facility */
static void cmd_script_upload() {
	char name[81];
	unsigned short chunksize;

	name[0] = '\0';

	if (!get_string("Script name: ", name, 30)) return;

	/* Starting from protocol version 4.6.1.2, the client can receive 1024 bytes in one packet */
	if (is_newer_than(&server_version, 4, 6, 1, 1, 0, 1)) {
		chunksize = 1024;
	} else {
		chunksize = 256;
	}

	remote_update(0, name, chunksize);
}

/*
 * Upload/execute scripts
 */
static void cmd_script_exec() {
	char buf[81];

	buf[0] = '\0';
	if (!get_string("Script> ", buf, 80)) return;

	Send_master(MASTER_SCRIPTS, buf);
}

static void cmd_script_exec_local() {
	char buf[81];

	buf[0] = '\0';
	if (!get_string("Script> ", buf, 80)) return;

	exec_lua(0, buf);
}


/* Dirty implementation.. FIXME		- Jir - */
static void cmd_master_aux_system() {
	char i = 0;

	while (i != ESCAPE) {
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
		switch (i) {
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
		case KTRL('Q'):
			break;
		default:
			bell();
		}

		/* Flush messages */
		clear_topline_forced();
	}
}

/* Dungeon Master commands */
static void cmd_master(void) {
	char i = 0;

	//party_mode = TRUE;

	/* Save screen */
	Term_save();

	/* Process requests until done */
	while (i != ESCAPE) {
		/* Clear screen */
		Term_clear();

		/* Describe */
		Term_putstr(0, 2, -1, TERM_L_DARK, "Dungeon Master commands (server administrators only)");

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

		switch(i) {
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
		case KTRL('Q'):
			break;
		default:
			bell();
		}

		/* Flush messages */
		clear_topline_forced();
	}

	/* Reload screen */
	Term_load();

	/* No longer in party mode */
	//party_mode = FALSE;

	/* Flush any events */
	Flush_queue();
}

void cmd_king() {
	if (!get_check2("Do you really want to own this land?", FALSE)) return;

	Send_King(KING_OWN);
}

void cmd_spike() {
	int dir = command_dir;

	if (!dir && !get_dir(&dir)) return;

	Send_spike(dir);
}

/*
 * formality :)
 * Nice if we can use LUA hooks here..
 */
void cmd_raw_key(int key) {
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

void cmd_lagometer(void) {
	int k;

	/* Save the screen */
	Term_save();

	lagometer_open = TRUE;

	/* Interact */
	while (1) {
		display_lagometer(TRUE);

		/* Get command */
		k = inkey();

		/* Exit */
		if (k == ESCAPE || k == KTRL('Q') || k == KTRL('I')) break;

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
		/* Clear ping memory */
		case 'c':
			for (k = 0; k < 60; k++) ping_times[k] = 0;
			break;
		}
	}

	lagometer_open = FALSE;

	/* Restore the screen */
	Term_load();

	Flush_queue();
}

void cmd_force_stack() {
	int item;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Forcibly stack what? ", USE_INVEN | USE_EXTRA)) return;
	Send_force_stack(item);
}
