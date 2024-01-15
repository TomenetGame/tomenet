/* $Id$ */
#include "angband.h"

#ifdef REGEX_SEARCH
 #include <regex.h>
#endif

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
	if (o_ptr->tval == TV_FOOD) return(TRUE);
	if (o_ptr->tval == TV_FIRESTONE) return(TRUE);
	if (o_ptr->tval == TV_GAME && o_ptr->sval == SV_SNOWBALL) return(TRUE);
	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT && (o_ptr->xtra3 & 0x0001)) return(TRUE);

	return(FALSE);
}

/* XXX not fully functional */
static bool item_tester_oils(object_type *o_ptr) {
	if (o_ptr->tval == TV_LITE) return(TRUE);
	if (o_ptr->tval == TV_FLASK && o_ptr->sval == SV_FLASK_OIL) return(TRUE);

	return(FALSE);
}

static void cmd_all_in_one(void) {
	int item, dir, tval;

	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

#ifdef ENABLE_SUBINVEN
	if (!c_get_item(&item, "Use which item? ", (USE_EQUIP | USE_INVEN | USE_EXTRA | USE_SUBINVEN))) {
#else
	if (!c_get_item(&item, "Use which item? ", (USE_EQUIP | USE_INVEN | USE_EXTRA))) {
#endif
		if (item == -2) c_msg_print("You don't have any items.");
		return;
	}
#ifdef ENABLE_SUBINVEN
	if (item >= 100) tval = subinventory[item / 100 - 1][item % 100].tval;
	else
#endif
	if (item >= INVEN_WIELD) {
		/* Does item require aiming? (Always does if not yet identified) */
		if (inventory[item].uses_dir == 0) {
			/* (also called if server is outdated, since uses_dir will be 0 then) */
			Send_activate(item);
		} else {
			if (!get_dir(&dir)) return;
			Send_activate_dir(item, dir);
		}
		return;
	} else tval = inventory[item].tval;

	switch (tval) {
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
#ifdef ENABLE_SUBINVEN
		if (item >= 100) Send_zap(item);
		else
#endif
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
#ifdef ENABLE_SUBINVEN
		if (item >= 100) return;
#endif
		if (inventory[item].sval == SV_FLASK_OIL) Send_fill(item);
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
#ifdef ENABLE_SUBINVEN
	case TV_SUBINVEN:
		cmd_browse(item);
		break;
#endif
	case TV_SPECIAL:
		/* Todo maybe - implement something */
		break;
	/* Presume it's sort of spellbook */
	case TV_BOOK:
	default:
	    {
		int i;
		bool done = FALSE;

#ifdef ENABLE_SUBINVEN
		if (item >= 100) return;
#endif

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

	case 'b': cmd_browse(-1); break;
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
	case '|':
		if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0)) {
			char choice;
#if 0 /* 0: for a choice, use ~ key; we just keep the 'normal' function as a shortcut on | for QoL */
			bool inkey_msg_old = inkey_msg;

			inkey_msg = TRUE;
			// TODO: "uniques_alive","List only unslain uniques for your local party" <- replaced by this choice now:
			//get_com("Which uniques? (ESC for all, SPACE for (party-)unslain, ! for bosses/top lv.)", &choice);
			get_com("Which uniques? (ESC for all, SPACE for alive, 'b' or 'e' for bosses)", &choice);
			inkey_msg = inkey_msg_old;
			if (choice == '!' || choice == 'b' || choice == 'e') choice = 2;
			else if (choice == ' ' || choice == 'a') choice = 1;
			else choice = 0;
#else
			choice = 0;
#endif
			/* Encode 'choice' in 'line' info */
			Send_special_line(SPECIAL_FILE_UNIQUE, choice * 100000, "");
		} else cmd_uniques();
		break;
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
		if (p_ptr->admin_dm || p_ptr->admin_wiz) cmd_master();
		else cmd_raw_key(command_cmd);
		break;

	/* Add separate buffer for chat only review (good for afk) -Zz */
	case KTRL('O'): do_cmd_messages_important(); break;
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
	case KTRL('T'): xhtml_screenshot("screenshot????", FALSE); break;
	case KTRL('I'): cmd_lagometer(); break;

//#define CTRLC_DEBUG /* Use CTRL-C for some special debug stuff instead of its normal function (audio shortcut)? */

#ifdef USE_SOUND_2010
	case KTRL('U'): interact_audio(); break;
	//case KTRL('M'): //this is same as '\r' and hence doesn't work..
 #ifndef CTRLC_DEBUG
	case KTRL('C'): toggle_music(FALSE); break;
 #endif
	case KTRL('N'): toggle_master(FALSE); break;
#else
	case KTRL('U'):
	//case KTRL('M'): //this is same as '\r' and hence doesn't work..
 #ifndef CTRLC_DEBUG
	case KTRL('C'):
 #endif
	case KTRL('N'):
		c_msg_print("This key is unused in clients without SDL-sound support.  Hit '?' for help.");
		break;
#endif

	case '!':
#if 1
		cmd_BBS(); break;
#else /* For debugging */
		c_msg_format("%d->%d", TERMX_BLUE, term2attr(TERMX_BLUE)); break;
#endif
#ifdef CTRLC_DEBUG /* only for debugging purpose - dump some client-side special config */
	case KTRL('C'):
		//c_msg_format("Client FPS: %d", cfg_client_fps);
		handle_process_font_file();
		break;
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
	char ch, dir = 0x00;
	bool sel = FALSE, worldmap = ((mode == 1) || !map_town);

	/* Hack -- if the screen is already icky, ignore this command */
	if (screen_icky && !mode) return;

	/* Save the screen */
	Term_save();

	/* Reset grid selection if any */
	//Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
	minimap_selx = -1;

	/* Accepting minimap network data, to be drawn over the main screen contents */
	local_map_active = TRUE;

	while (TRUE) {
		/* Send the request */
		Send_map(mode + dir);

		/* Reset the line counter */
		last_line_info = 0;

#ifdef WILDMAP_ALLOW_SELECTOR_SCROLLING
c_msg_format("cmd_map - wx,wy=%d,%d; mmsx,mmsy=%d,%d, mmpx,mmpy=%d,%d, y_offset=%d", p_ptr->wpos.wx, p_ptr->wpos.wy, minimap_selx, minimap_sely, minimap_posx, minimap_posy, minimap_yoff);
		if (sel) {
c_msg_format("cmd_map SEL - wx,wy=%d,%d; mmsx,mmsy=%d,%d, mmpx,mmpy=%d,%d, y_offset=%d", p_ptr->wpos.wx, p_ptr->wpos.wy, minimap_selx, minimap_sely, minimap_posx, minimap_posy, minimap_yoff);
			//minimap_selx = minimap_posx;
			//minimap_sely = minimap_posy;
			//minimap_selattr = minimap_attr;
			//minimap_selchar = minimap_char;
		}
#endif

		while (TRUE) {
			/* Remember the grid below our grid-selection marker */
			if (sel) {
				minimap_selattr = Term->scr->a[minimap_sely][minimap_selx];
				minimap_selchar = Term->scr->c[minimap_sely][minimap_selx];
			}

			/* Wait until we get the whole thing */
			if (last_line_info == 23 + HGT_PLUS) {
				if (minimap_selx != -1) {
					int dis;
					int wx, wy; /* For drawing selection cursor */

#ifndef WILDMAP_ALLOW_SELECTOR_SCROLLING
					wx = p_ptr->wpos.wx + minimap_selx - minimap_posx;
					wy = p_ptr->wpos.wy - minimap_sely + minimap_posy;
#else
					int yoff = p_ptr->wpos.wy - minimap_yoff; // = the 'y' world coordinate of the center point of the minimap

					wx = p_ptr->wpos.wx + minimap_selx - minimap_posx;
					/* minimap_yoff: p_ptr->wpos.wy - minimap_yoff = center point 'pseudo-wpos' of our map, to draw it around.
					   (The x offset is always 32, as the 64x64 worldmap map fits on the screen horizontally.)
					   The minimap_selx begins at left screen side, but minimap_sely (1..44 on a 46-lines screen aka big_map)
					    begins at the _top_ instead of the bottom of the screen, so we have to translate. */
					wy = yoff - 44 / 2 + 44 - minimap_sely; //44 = height of displayed map, with y_off being the wpos at the _center point_ of the displayed map
c_msg_format("wx,wy=%d,%d; mmsx,mmsy=%d,%d, mmpx,mmpy=%d,%d, y_offset=%d", p_ptr->wpos.wx, p_ptr->wpos.wy, minimap_selx, minimap_sely, minimap_posx, minimap_posy, minimap_yoff);
#endif

					Term_putstr(0,  9, -1, TERM_WHITE, "\377o Select");
					Term_putstr(0, 10, -1, TERM_WHITE, format("(%2d,%2d)", wx, wy));
					Term_putstr(0, 11, -1, TERM_WHITE, format("%+3d,%+3d", wx - p_ptr->wpos.wx, wy - p_ptr->wpos.wy));
					/* RECALL_MAX_RANGE uses distance() function, so we do the same:
					   It was originally defined as 16, but has like ever been 24, so we just colourize accordingly to catch 'em all.. */
					dis = distance(wx, wy, p_ptr->wpos.wx, p_ptr->wpos.wy);
					Term_putstr(0, 12, -1, TERM_WHITE, format("Dist:\377%c%2d", dis <= 16 ? 'w' : (dis <= 24 ? 'y' : 'o'), dis));
				} else {
					Term_putstr(0,  9, -1, TERM_WHITE, "        ");
					Term_putstr(0, 10, -1, TERM_WHITE, "        ");
					Term_putstr(0, 11, -1, TERM_WHITE, "        ");
					Term_putstr(0, 12, -1, TERM_WHITE, "        ");
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
			else if (ch == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);
			/* locate map (normal / rogue-like keys) */
			else if (ch == '2' || ch == 'j') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_sely < CL_WINDOW_HGT - 2) minimap_sely++;
#ifdef WILDMAP_ALLOW_SELECTOR_SCROLLING
					else if (is_atleast(&server_version, 4, 8, 1, 2, 0, 0)) {
						dir = 0x02;
						break;
					}
#endif
					continue;
				}
				dir = 0x02;
				break;
			} else if (ch == '6' || ch == 'l') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_selx < 8 + 64 - 1) minimap_selx++;
					continue;
				}
				dir = 0x04;
				break;
			} else if (ch == '8' || ch == 'k') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_sely > 1) minimap_sely--;
#ifdef WILDMAP_ALLOW_SELECTOR_SCROLLING
					else if (is_atleast(&server_version, 4, 8, 1, 2, 0, 0)) {
						dir = 0x08;
						break;
					}
#endif
					continue;
				}
				dir = 0x08;
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
				dir = 0x02 | 0x10;
				break;
			} else if (ch == '3' || ch == 'n') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_sely < CL_WINDOW_HGT - 2) minimap_sely++;
					if (minimap_selx < 8 + 64 - 1) minimap_selx++;
					continue;
				}
				dir = 0x02 | 0x04;
				break;
			} else if (ch == '9' || ch == 'u') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_sely > 1) minimap_sely--;
					if (minimap_selx < 8 + 64 - 1) minimap_selx++;
					continue;
				}
				dir = 0x08 | 0x10;
				break;
			} else if (ch == '7' || ch == 'y') {
				if (sel) {
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					if (minimap_sely > 1) minimap_sely--;
					if (minimap_selx > 8) minimap_selx--;
					continue;
				}
				dir = 0x08 | 0x04;
				break;
			} else if (ch == '5' || ch == ' ' || ch == 'r') {
				if (sel) {
					/* Return selector-X cursor onto the player's wpos sector */
					Term_draw(minimap_selx, minimap_sely, minimap_selattr, minimap_selchar);
					minimap_selx = minimap_posx;
					minimap_sely = minimap_posy;
					minimap_selattr = minimap_attr;
					minimap_selchar = minimap_char;
					continue;
				}
				/* Center map on player's wpos again (inital map state) */
				dir = 0x00;
				break;
			}
			/* manual grid selection on the map */
			else if (ch == 's' && worldmap) {
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

	local_map_active = FALSE;

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
			if (ch == KTRL('T')) xhtml_screenshot("screenshot????", FALSE);

			if (ch == ':') {
				cmd_message();
				inkey_msg = TRUE; /* And suppress macros again.. */
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

#ifdef ENABLE_SUBINVEN
/* This will (on server-side)..
   a) automatically pick the fitting subinventories
   b) automatically move the most possible amount
*/
static void cmd_subinven_move(void) {
	int item;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	for (item = 0; item < INVEN_PACK; item++)
		if (inventory[item].tval == TV_SUBINVEN) break;
	if (item == INVEN_PACK) {
		c_message_add("You don't carry any containers.");
		return;
	}

	if (!c_get_item(&item, "Stow what? ", (USE_INVEN | USE_EXTRA))) return;

	Send_subinven_move(item);
}
/* Note: islot can be ignored, since it's actually using_subinven, which is checked in c_get_item() anyway. */
static void cmd_subinven_remove(int islot) {
	int item;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Unstow what? ", (USE_SUBINVEN | USE_EXTRA))) return;

	Send_subinven_remove(item);
}
#endif

void cmd_inven(void) {
	char ch; /* props to 'potato' and the korean team from sarang.net for the idea - C. Blue */
	int c;
	char buf[MSG_LEN];

	/* First, erase our current location */

	/* Then, save the screen */
	Term_save();
	showing_inven = screen_icky;

	command_gap = 50;

#ifdef USE_SOUND_2010
	/* Note: We don't play the sound in show_inven() itself because that will be too spammy. */
	sound(browseinven_sound_idx, SFX_TYPE_COMMAND, 100, 0, 0, 0);
#endif

	show_inven();
#ifdef ENABLE_SUBINVEN
	/* Do this after we called show_inven() so total weight is displayed too, in the topline. */
	topline_icky = TRUE;
#endif

	while (TRUE) {
		/* Redraw these in case some command from below has erased the topline */
		show_inven_header();
#ifdef ENABLE_SUBINVEN
topline_icky = TRUE; /* Needed AGAIN. A failed 'stow' command causes topline to get partially overwritten by the server's error response msg. -_- so probably c_get_item unsets ickiness. */
#endif

		ch = inkey();
		/* allow pasting item into chat */
		if (ch >= 'A' && ch < 'A' + INVEN_PACK) { /* using capital letters to force SHIFT key usage, less accidental spam that way probably */
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
		else switch (ch) {
		case KTRL('T'):
			xhtml_screenshot("screenshot????", FALSE);
			break;
		case 'x':
			cmd_observe(USE_INVEN);
			continue;
		case 'd':
			cmd_drop(USE_INVEN);
			continue;
		case 'k':
		case KTRL('D'):
			cmd_destroy(USE_INVEN);
			continue;
		case '{':
			cmd_inscribe(USE_INVEN);
			continue;
		case '}':
			cmd_uninscribe(USE_INVEN);
			continue;
		case ':':
			cmd_message();
			continue;
		case 'b': /* Added for ENABLE_SUBINVEN, but it's fine in general, just need to clean up first, which is done in cmd_browse().. */
			cmd_browse(-1);
			/* ..and cmd_browse() also knows to restore our state */
			continue;
#ifdef ENABLE_SUBINVEN
		/* Additional commands specifically replacing/allowing backpack-related commands for subinventories: */
		/* Move item to backpack inventory - let's abuse equipment-related commands for the heck of it */
		case 's':
			cmd_subinven_move();
			continue;
#endif
		}

		/* Default, eg ESC: */
		break;
	}

	screen_line_icky = -1;
	screen_column_icky = -1;

#ifdef ENABLE_SUBINVEN
	topline_icky = FALSE;
#endif
	showing_inven = FALSE;

	/* restore the screen */
	Term_load();
	/* print our new location */

#ifdef ENABLE_SUBINVEN
	/* Get rid of the header displaying s/b keys from show_inven_header() again: */
	clear_topline();
#endif

	/* Flush any events */
	Flush_queue();
}

#ifdef ENABLE_SUBINVEN
/* Display the subinventory inside a 'pouch' in a specific inventory slot */
void cmd_subinven(int islot) {
	char ch;
	int c;
	char buf[MSG_LEN];
	object_type *i_ptr = &inventory[islot];
	int subinven_size = i_ptr->pval;
	bool leave = FALSE;

	Term_save();
	showing_inven = screen_icky;
	command_gap = 50;

 #ifdef USE_SOUND_2010
	/* Note: We don't play the sound in show_inven() itself because that will be too spammy. */
	sound(browseinven_sound_idx, SFX_TYPE_COMMAND, 100, 0, 0, 0);
 #endif

	//show_subinven(islot); --moved into the loop

	/* Redirect all inventory-related commands to this subinventory */
	using_subinven = islot;
	using_subinven_size = subinven_size;
	using_subinven_item = -1;

	while (TRUE) {
		topline_icky = TRUE; /* Server replies after commands like drop, destroy.. will overwrite our header line otherwise, when they arrive. */
		show_subinven(islot);

		ch = inkey();
		/* allow pasting item into chat */
		if (ch >= 'A' && ch < 'A' + subinven_size) { /* using capital letters to force SHIFT key usage, less accidental spam that way probably */
			c = ch - 'A';

			if (subinventory[islot][c].tval) {
				snprintf(buf, MSG_LEN - 1, "\377s%s", subinventory_name[islot][c]);
				buf[MSG_LEN - 1] = 0;
				/* if item inscriptions contains a colon we might need
				   another colon to prevent confusing it with a private message */
				if (strchr(subinventory_name[islot][c], ':')) {
					buf[strchr(subinventory_name[islot][c], ':') - subinventory_name[islot][c] + strlen(buf) - strlen(subinventory_name[islot][c]) + 1] = '\0';
					if (strchr(buf, ':') == &buf[strlen(buf) - 1])
						strcat(buf, ":");
					strcat(buf, strchr(subinventory_name[islot][c], ':') + 1);
				}
				Send_paste_msg(buf);
			}
			break;
		}
		else switch (ch) {
		case ESCAPE:
			/* Leave subinventory */
			leave = TRUE;
			break;
		case KTRL('T'):
			xhtml_screenshot("screenshot????", FALSE);
			break;
		case 'x':
			cmd_observe(USE_INVEN);
			continue;
		case 'd':
			cmd_drop(USE_INVEN);
			break; /* Need to refresh the subinven list, as soon as the server replies with the subinventory-update, so leave for now */
		case 'k':
		case KTRL('D'):
			cmd_destroy(USE_INVEN);
			break; /* Need to refresh the subinven list, as soon as the server replies with the subinventory-update, so leave for now */
		case '{':
			cmd_inscribe(USE_INVEN);
			break; /* Need to refresh the subinven list, as soon as the server replies with the subinventory-update, so leave for now */
		case '}':
			cmd_uninscribe(USE_INVEN);
			break; /* Need to refresh the subinven list, as soon as the server replies with the subinventory-update, so leave for now */
		case ':':
			cmd_message();
			continue;

		/* Additional commands specifically replacing/allowing backpack-related commands for subinventories: */

		/* Move item to backpack inventory */
		case 's': cmd_subinven_remove(using_subinven); continue;

		/* More basic functions */
		//postponed
		//case 'K': cmd_force_stack(); continue;
		//case 'H': cmd_apply_autoins(); continue;

		/* Specifically required for DEMOLITIONIST chemicals */
		case 'a':
			/* Restricted now to satchel only, no good really for other cases */
			if (i_ptr->sval != SV_SI_SATCHEL) break;

			cmd_activate(); // 'A' is item-to-chat pasting, oops	//todo
			continue;
			/* Hmm, we might need or not need to refresh the subinven list, as soon as the server replies with the subinventory-update,
			   depending on whether we Receive_item() for a 2nd item to participate, or not.
			   While just break; would be required to update our inventory for 1-item-activations, we should stay within cmd_subinven()
			   context to process the Receive_item() request within the same satchel.. Let's introduce 'using_subinven_item' maybe. */

		/* Misc stuff that could be considered */
#if 0
		case 'F': cmd_refill(); continue;
		case 'E': cmd_eat(); continue;
		case 'v': cmd_throw(); continue;
#endif

		/* Ohoho.. mada mada - super experimental hypothetical */
#if 0
		case 'a': cmd_aim_wand(); continue;
		case 'u': cmd_use_staff(); continue;
		case 'z': cmd_zap_rod(); continue;
#endif
		}

		if (leave) break;
	}

	/* End redirection of inventory-related commands, as we left the subinventory */
	using_subinven = -1;
	using_subinven_item = -1;

	screen_line_icky = -1;
	screen_column_icky = -1;

	showing_inven = FALSE;

	/* restore the screen */
	Term_load();
	/* print our new location */

	topline_icky = FALSE;
	clear_topline_forced();

	/* Flush any events */
	Flush_queue();
}
#endif

void cmd_equip(void) {
	char ch; /* props to 'potato' and the korean team from sarang.net for the idea - C. Blue */
	int c;
	char buf[MSG_LEN];

	Term_save();
	showing_equip = screen_icky;

	command_gap = 50;

	show_equip();

	while (TRUE) {
		ch = inkey();
		/* allow pasting item into chat */
		if (ch >= 'A' && ch < 'A' + (INVEN_TOTAL - INVEN_WIELD)) { /* using capital letters to force SHIFT key usage, less accidental spam that way probably */
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
		else switch (ch) {
		case KTRL('T'):
			xhtml_screenshot("screenshot????", FALSE);
			break;
		case 'x':
			cmd_observe(USE_EQUIP);
			continue;
		/* collides! ((c_cfg.rogue_like_commands && ch == 'T') || (!c_cfg.rogue_like_commands && ch == 't')) */
		case 't':
			cmd_take_off();
			continue;
		case '{':
			cmd_inscribe(USE_EQUIP);
			continue;
		case '}':
			cmd_uninscribe(USE_EQUIP);
			continue;
		case ':':
			cmd_message();
			continue;
		}

		/* Default, eg ESC: */
		break;
	}

	screen_line_icky = -1;
	screen_column_icky = -1;

	showing_equip = FALSE;

	Term_load();

	/* Flush any events */
	Flush_queue();
}

void cmd_drop(byte flag) {
	int item, amt, num, tval;
	bool equipped;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Drop what? ", (flag | USE_EXTRA))) return;

#ifdef ENABLE_SUBINVEN
	if (using_subinven != -1) {
		equipped = FALSE;

		/* Get an amount */
		if (subinventory[using_subinven][item % 100].number > 1) {
			if (is_cheap_misc(subinventory[using_subinven][item % 100].tval) && c_cfg.whole_ammo_stack && !verified_item
			    && (item % 100) < INVEN_WIELD) /* <- new: ignore whole_ammo_stack for equipped ammo, so it can easily be shared */
				amt = subinventory[using_subinven][item % 100].number;
			else {
				inkey_letter_all = TRUE;
				amt = c_get_quantity("How many ('a' or spacebar for all)? ", subinventory[using_subinven][item % 100].number);
			}
		}
		else amt = 1;

		Send_drop(item, amt);
		return;
	} else if (item >= 100) {
		equipped = FALSE;
		num = subinventory[item / 100 - 1][item % 100].number;
		tval = subinventory[item / 100 - 1][item % 100].tval;
	} else
#endif
	{
		equipped = item >= INVEN_WIELD;
		num = inventory[item].number;
		tval = inventory[item].tval;
	}

	/* Get an amount */
	if (num > 1) {
		if (is_cheap_misc(tval) && c_cfg.whole_ammo_stack && !verified_item
		    && !equipped) /* <- new: ignore whole_ammo_stack for equipped ammo, so it can easily be shared */
			amt = num;
		else {
			inkey_letter_all = TRUE;
			amt = c_get_quantity("How many ('a' or spacebar for all)? ", num);
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

	if (!c_get_item(&item, "Wear/Wield which item? ", (USE_INVEN | USE_EXTRA))) return;

	/* Send it */
	Send_wield(item);
}

void cmd_wield2(void) {
	int item;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Wear/Wield which item? ", (USE_INVEN | USE_EXTRA))) return;

	/* Send it */
	Send_wield2(item);
}

void cmd_observe(byte mode) {
	int item;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Examine which item? ", (mode | USE_EXTRA))) return;

	/* Send it */
	Send_observe(item);
}

void cmd_take_off(void) {
	int item, amt = 255, num;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Takeoff which item? ", (USE_EQUIP | USE_EXTRA))) return;
#ifdef ENABLE_SUBINVEN
	if (item >= 100) num = subinventory[item / 100 - 1][item % 100].number;
	else
#endif
	num = inventory[item].number;

	/* New in 4.4.7a, for ammunition: Can take off a certain amount - C. Blue */
	if (num > 1 && verified_item
	    && is_newer_than(&server_version, 4, 4, 7, 0, 0, 0)) {
		inkey_letter_all = TRUE;
		amt = c_get_quantity("How many ('a' or spacebar for all)? ", num);
		Send_take_off_amt(item, amt);
	} else
		Send_take_off(item);
}

void cmd_swap(void) {
	int item;
	char insc[4], *c, swap_insc[3];

	strcpy(swap_insc, "@x");

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Swap which item? ", (USE_INVEN | USE_EQUIP | INVEN_FIRST | USE_EXTRA | CHECK_MULTI))) return;

	/* For items that can go into multiple slots (weapons and rings),
	   check @xN inscriptions on source and destination to pick slot */
	if (item < INVEN_PACK) {
		/* smart handling of @xN swapping with equipped items that have alternative slot options */
		if (is_weapon(inventory[item].tval) && is_weapon(inventory[INVEN_ARM].tval) &&
		    (c = strstr(inventory_name[item], swap_insc))) {
			strncpy(insc, c, 3);
			insc[3] = 0;
			if (strstr(inventory_name[INVEN_ARM], insc) && !strstr(inventory_name[INVEN_WIELD], insc)) {
				Send_wield2(item);
				return;
			}
		}
		else if (inventory[item].tval == TV_RING && inventory[INVEN_RIGHT].tval == TV_RING &&
		    (c = strstr(inventory_name[item], swap_insc))) {
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
	/* special case for DUAL_WIELD: Swap two dual-wielded weapons, useful for main-hand mode: */
	} else if (item == INVEN_WIELD && inventory[INVEN_ARM].tval && (c = strstr(inventory_name[INVEN_WIELD], swap_insc))) {
		strncpy(insc, c, 3);
		insc[3] = 0;
		if (strstr(inventory_name[INVEN_ARM], insc)) {
			Send_wield3();
			return;
		} else Send_take_off(item); //fallback
	} else Send_take_off(item);
}


void cmd_destroy(byte flag) {
	int item, amt, num, tval;
	char out_val[MSG_LEN];
	cptr name;

	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Destroy what? ", (flag | USE_EXTRA))) return;

#ifdef ENABLE_SUBINVEN
	if (using_subinven != -1) {
		/* Get an amount */
		if (subinventory[using_subinven][item % 100].number > 1) {
			if (is_cheap_misc(subinventory[using_subinven][item % 100].tval) && c_cfg.whole_ammo_stack && !verified_item) amt = subinventory[using_subinven][item % 100].number;
			else {
				inkey_letter_all = TRUE;
				amt = c_get_quantity("How many ('a' or spacebar for all)? ", subinventory[using_subinven][item % 100].number);
			}
		} else amt = 1;

		/* Sanity check */
		if (!amt) return;

		if (!c_cfg.no_verify_destroy) {
			if (subinventory[using_subinven][item % 100].number == amt)
				sprintf(out_val, "Really destroy %s?", subinventory_name[using_subinven][item % 100]);
			else
				sprintf(out_val, "Really destroy %d of your %s?", amt, subinventory_name[using_subinven][item % 100]);
			if (!get_check2(out_val, FALSE)) return;
		}

		/* Send it */
		Send_destroy(item, amt);
		return;
	} else if (item >= 100) {
		num = subinventory[item / 100 - 1][item % 100].number;
		tval = subinventory[item / 100 - 1][item % 100].tval;
		name = subinventory_name[item / 100 - 1][item % 100];
	} else
#endif
	{
		num = inventory[item].number;
		tval = inventory[item].tval;
		name = inventory_name[item];
	}

	/* Get an amount */
	if (num > 1) {
		if (is_cheap_misc(tval) && c_cfg.whole_ammo_stack && !verified_item) amt = num;
		else {
			inkey_letter_all = TRUE;
			amt = c_get_quantity("How many ('a' or spacebar for all)? ", num);
		}
	} else amt = 1;

	/* Sanity check */
	if (!amt) return;

	if (!c_cfg.no_verify_destroy) {
		if (num == amt)
			sprintf(out_val, "Really destroy %s?", name);
		else
			sprintf(out_val, "Really destroy %d of your %s?", amt, name);
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

	if (!c_get_item(&item, "Uninscribe what? ", (flag | USE_EXTRA))) return;

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

	apply_auto_inscriptions_aux(item, -1, TRUE);
	if (c_cfg.auto_inscribe) Send_autoinscribe(item);
}

void cmd_steal(void) {
	int dir;

	if (!get_dir(&dir)) return;

	/* Send it */
	Send_steal(dir);
}

static bool item_tester_quaffable(object_type *o_ptr) {
	if (o_ptr->tval == TV_POTION) return(TRUE);
	if (o_ptr->tval == TV_POTION2) return(TRUE);
	if ((o_ptr->tval == TV_FOOD) &&
			(o_ptr->sval == SV_FOOD_PINT_OF_ALE ||
			 o_ptr->sval == SV_FOOD_PINT_OF_WINE) )
			return(TRUE);
	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT && (o_ptr->xtra3 & 0x0002)) return(TRUE);

	return(FALSE);
}

void cmd_quaff(void) {
	int item;

	item_tester_hook = item_tester_quaffable;
	get_item_hook_find_obj_what = "Potion name? ";
	get_item_extra_hook = get_item_hook_find_obj;

#ifdef ENABLE_SUBINVEN
	if (!c_get_item(&item, "Quaff which potion? ", (USE_INVEN | USE_EXTRA | NO_FAIL_MSG | USE_SUBINVEN))) {
#else
	if (!c_get_item(&item, "Quaff which potion? ", (USE_INVEN | USE_EXTRA | NO_FAIL_MSG))) {
#endif
		if (item == -2) c_msg_print("You don't have any potions.");
		return;
	}

	/* Send it */
	Send_quaff(item);
}

static bool item_tester_readable(object_type *o_ptr) {
	if (o_ptr->tval == TV_SCROLL) return(TRUE);
	if (o_ptr->tval == TV_PARCHMENT) return(TRUE);
	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT && (o_ptr->xtra3 & 0x0004)) return(TRUE);

	return(FALSE);
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

	if (!c_get_item(&item, "Aim which wand? ", (USE_INVEN | USE_EXTRA | CHECK_CHARGED | NO_FAIL_MSG
	    | USE_EQUIP))) { //WIELD_DEVICES
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

#ifdef ENABLE_SUBINVEN
	if (!c_get_item(&item, "Use which staff? ", (USE_INVEN | USE_EXTRA | CHECK_CHARGED | NO_FAIL_MSG | USE_SUBINVEN
	    | USE_EQUIP))) { //WIELD_DEVICES
#else
	if (!c_get_item(&item, "Use which staff? ", (USE_INVEN | USE_EXTRA | CHECK_CHARGED | NO_FAIL_MSG
	    | USE_EQUIP))) { //WIELD_DEVICES
#endif
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

#ifdef ENABLE_SUBINVEN
	if (!c_get_item(&item, "Zap which rod? ", (USE_INVEN | USE_EXTRA | CHECK_CHARGED | NO_FAIL_MSG | USE_SUBINVEN
	    | USE_EQUIP))) { //WIELD_DEVICES
#else
	if (!c_get_item(&item, "Zap which rod? ", (USE_INVEN | USE_EXTRA | CHECK_CHARGED | NO_FAIL_MSG
	    | USE_EQUIP))) { //WIELD_DEVICES
#endif
		if (item == -2) c_msg_print("You don't have any rods.");
		return;
	}
#ifdef ENABLE_SUBINVEN
	if (item >= 100) {
		/* Send it */
		/* Does item require aiming? (Always does if not yet identified) */
		if (subinventory[item / 100 - 1][item % 100].uses_dir == 0) {
			/* (also called if server is outdated, since uses_dir will be 0 then) */
			Send_zap(item);
		} else { /* Actually directional rods are not allowed to be used from within a subinventory, but anyway.. */
			if (!get_dir(&dir)) return;
			Send_zap_dir(item, dir);
		}
		return;
	}
#endif

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

	if (!c_get_item(&item, p, (USE_INVEN))) return;

	/* Send it */
	Send_fill(item);
}

void cmd_eat(void) {
	int item;

	item_tester_hook = item_tester_edible;
	get_item_hook_find_obj_what = "Food name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	if (!c_get_item(&item, "Eat what? ", (USE_INVEN | USE_EXTRA | NO_FAIL_MSG))) {
		if (item == -2) c_msg_print("You do not have anything edible.");
		return;
	}

	/* Send it */
	Send_eat(item);
}

void cmd_activate(void) {
	int item, dir;
#ifdef ENABLE_SUBINVEN
	int sub = -1;
#endif

	/* Allow all items */
	item_tester_hook = NULL;
	get_item_hook_find_obj_what = "Activatable item name? ";
	get_item_extra_hook = get_item_hook_find_obj;

	/* Currently, most activatable items must be worn as they are equippable items.
	   Exceptions are: Custom books, book of the dead, golem scrolls and runes. */
#ifdef ENABLE_SUBINVEN
	if (using_subinven == -1) {
		if (!c_get_item(&item, "Activate what? ", (USE_EQUIP | USE_INVEN | USE_EXTRA | USE_SUBINVEN))) return;
		if (item >= 100) sub = item / 100 - 1;
	} else
#endif
	//if (!c_get_item(&item, "Activate what? ", (USE_EQUIP | USE_INVEN | EQUIP_FIRST | USE_EXTRA)))
	if (!c_get_item(&item, "Activate what? ", (USE_EQUIP | USE_INVEN | USE_EXTRA))) return;

#ifdef ENABLE_SUBINVEN
	if (using_subinven != -1 || sub != -1) {
		if (sub == -1) sub = using_subinven;
		/* Send it */
		/* Does item require aiming? (Always does if not yet identified) */
		if (subinventory[sub][item % 100].uses_dir == 0) {
			/* (also called if server is outdated, since uses_dir will be 0 then) */
			using_subinven_item = item; /* allow mixing chemicals directly from satchels */
			Send_activate(item);
		} else {
			if (!get_dir(&dir)) return;
			Send_activate_dir(item, dir);
		}
		return;
	}
#endif

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
			return(FALSE);
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
				return(FALSE);
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

	return(TRUE);
}


int cmd_target_friendly(void) {
	/* Tell the server to init targetting */
	Send_target_friendly(0);
	return(TRUE);
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
			xhtml_screenshot("screenshot????", FALSE);
			break;
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
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
			xhtml_screenshot("screenshot????", FALSE);
			break;
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
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


#define CS_SEL_COL TERM_SELECTOR
void cmd_character(void) {
	char ch = 0;
	int done = 0;
	char tmp[MAX_CHARS];
	static int sel = 1; //start at player race actually, cause it's probably the most interesting point (together with class)

	/* Save screen */
	Term_save();

	/* Init and re-init between relogs*/
	//if (sel == 0 && !p_ptr->fruit_bat) sel++;
	if (sel == 3 && !strcasecmp(c_p_ptr->body_name, "Player")) sel++;
	if (sel == 4 && !p_ptr->ptrait) sel++;

	while (!done) {
		/* Display player info */
		display_player(csheet_page);

		if (csheet_page == 1 && sel >= 12) sel = 0; /* History is displayed instead of misc abilities? */
		if (csheet_page != 2) { /* Equipment flag screen must not be active */
			switch (sel) {
			case 0: c_put_str(CS_SEL_COL, ">", 2, 0); break;
			case 1: c_put_str(CS_SEL_COL, ">", 3, 0); break;
			case 2: c_put_str(CS_SEL_COL, ">", 4, 0); break;
			case 3: c_put_str(CS_SEL_COL, ">", 5, 0); break;
			case 4: c_put_str(CS_SEL_COL, ">", 6, 0); break;
			case 5: c_put_str(CS_SEL_COL, ">", 7, 0); break;

			case 6: c_put_str(CS_SEL_COL, ">", 1, 60); break;
			case 7: c_put_str(CS_SEL_COL, ">", 2, 60); break;
			case 8: c_put_str(CS_SEL_COL, ">", 3, 60); break;
			case 9: c_put_str(CS_SEL_COL, ">", 4, 60); break;
			case 10: c_put_str(CS_SEL_COL, ">", 5, 60); break;
			case 11: c_put_str(CS_SEL_COL, ">", 6, 60); break;

			case 12: c_put_str(CS_SEL_COL, ">", 15, 0); break;
			case 13: c_put_str(CS_SEL_COL, ">", 16, 0); break;
			case 14: c_put_str(CS_SEL_COL, ">", 17, 0); break;
			case 15: c_put_str(CS_SEL_COL, ">", 18, 0); break;
			case 16: c_put_str(CS_SEL_COL, ">", 15, 27); break;
			case 17: c_put_str(CS_SEL_COL, ">", 16, 27); break;
			case 18: c_put_str(CS_SEL_COL, ">", 17, 27); break;
			case 19: c_put_str(CS_SEL_COL, ">", 18, 27); break;
			case 20: c_put_str(CS_SEL_COL, ">", 15, 54); break;
			case 21: c_put_str(CS_SEL_COL, ">", 16, 54); break;
			case 22: c_put_str(CS_SEL_COL, ">", 18, 54); break;
			}
		}

		/* Window Display */
		p_ptr->window |= PW_PLAYER;
		window_stuff();

		/* Display message */
		if (csheet_page != 2) prt("[ESC: quit, f: chardump, h: history/abilities, up/down: select -> ?: help]", 22, 3);
		//else prt("[ESC: quit, f: chardump, h: history/abilities]", 22, 17);
		else prt("[ESC: quit, f: chardump, h: history/abilities, v: toggle horizontal view]", 22, 3);

		/* Hack: Hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Wait for key */
		ch = inkey();

		switch (ch) {
		case '2':
			if (csheet_page == 2) break;
			sel++;
			if (sel > 22) sel = 0;
			//if (sel == 0 && !p_ptr->fruit_bat) sel++;
			//if (sel == 3 && !strcasecmp(c_p_ptr->body_name, "Player")) sel++;
			if (sel == 4 && !p_ptr->ptrait) sel++;
			if (sel > 22) sel = 0;
			break;
		case '8':
			if (csheet_page == 2) break;
			sel--;
			if (sel < 0) sel = 22;
			//if (sel == 0 && !p_ptr->fruit_bat) sel--;
			if (sel == 4 && !p_ptr->ptrait) sel--;
			//if (sel == 3 && !strcasecmp(c_p_ptr->body_name, "Player")) sel--;
			if (sel < 0) sel = 22;
			if (csheet_page == 1 && sel >= 12) sel = 11;
			break;
		//case '\n': case '\r':
		case '?':
			if (csheet_page == 2) break;
			switch (sel) {
			case 0: cmd_the_guide(3, 0, "body mod"); break;
			case 1: cmd_the_guide(3, 0, (char*)race_info[race].title); break;
			case 2: cmd_the_guide(3, 0, (char*)class_info[class].title); break;
			case 3: cmd_the_guide(3, 0, "mimicry details"); break;
			case 4: cmd_the_guide(3, 0, (char*)trait_info[trait].title); break;
			case 5: cmd_the_guide(3, 0, "character modes"); break;

			case 6: cmd_the_guide(3, 0, "Strength (STR)"); break;
			case 7: cmd_the_guide(3, 0, "Intelligence (INT)"); break;
			case 8: cmd_the_guide(3, 0, "Wisdom (WIS)"); break;
			case 9: cmd_the_guide(3, 0, "Dexterity (DEX)"); break;
			case 10: cmd_the_guide(3, 0, "Constitution (CON)"); break;
			case 11: cmd_the_guide(3, 0, "Charisma (CHR)"); break;

			case 12: cmd_the_guide(3, 0, "FIGHTING$$"); break;
			case 13: cmd_the_guide(3, 0, "BOWS/THROW$$"); break;
			case 14: cmd_the_guide(3, 0, "SAVING THROW$$"); break;
			case 15: cmd_the_guide(3, 0, "STEALTH$$"); break;
			case 16: cmd_the_guide(3, 0, "PERCEPTION$$"); break;
			case 17: cmd_the_guide(3, 0, "SEARCHING$$"); break;
			case 18: cmd_the_guide(3, 0, "DISARMING$$"); break;
			case 19: cmd_the_guide(3, 0, "MAGIC DEVICE$$"); break;
			case 20: cmd_the_guide(3, 0, "BLOWS/ROUND$$"); break;
			case 21: cmd_the_guide(3, 0, "SHOTS/ROUND$$"); break;
			case 22: cmd_the_guide(3, 0, "INFRA-VISION$$"); break;
			}
			break;
		case ':':
			/* specialty: allow chatting from within here -- to tell others about your lineage ;) - C. Blue */
			cmd_message();
			break;
		case 'h': case 'H':
			/* Check for "display history" */
			/* Toggle/loop */
			csheet_page = (csheet_page + 1) % 3;
			break;
		case 'f': case 'F':
			/* Dump */
			strnfmt(tmp, MAX_CHARS - 1, "%s.txt", cname);
			if (get_string("Filename(you can post it to http://angband.oook.cz/): ", tmp, MAX_CHARS - 1)) {
				if (tmp[0] && (tmp[0] != ' '))
					file_character(tmp, FALSE);
			}
			break;
		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", 2);
			break;
		case 'q': case 'Q': case ESCAPE: case 'C':
			/* Quit */
			done = 1;
			break;
		case 'v': /* Toggle 'vertical' view of the equipment stats screen */
			if (csheet_page != 2) break;
			csheet_horiz = !csheet_horiz;
			break;
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

static char *fgets_inverse(char *buf, int max, FILE *f) {
	int c, res, pos;
	char *ress;

	/* We expect to start positioned on the '\n' of the line we want to read, aka at its very end */
	if (fseek(f, -1, SEEK_CUR)) {
		/* Did we try to go beyond the beginning of the file? Emulate 'EOF' aka return 'NULL' */
		return(NULL);
	}
	while ((c = fgetc(f)) != '\n') {
		/* Did we hit the beginning of the file? */
		if (ftell(f) == 1) {
			/* Position on the very first character of the file */
			fseek(f, 0, SEEK_SET);
			ress = fgets(buf, max, f);
			fseek(f, 0, SEEK_SET);
			return(ress);
		}
		/* Move backwards from end to beginning of the line, character by character */
		res = fseek(f, -2, SEEK_CUR);
		if (res == -1) return(NULL); //paranoia, can't hit begining of line here
	}
	/* We're now on the beginning of the line we want to read */
	pos = ftell(f);
	ress = fgets(buf, max, f);

	/* Rewind by this line + 1, to fulfil our starting expectation for next time again */
	fseek(f, pos - 1, SEEK_SET);

	return(ress);
}

/* Local Guide invocation -
   search_type: 1 = search, 2 = strict search (all upper-case), 3 = chapter search, 4 = line number,
                0 = no pre-defined search, we're browsing it normally. */
/* '/?' command switches between search types on failure and retries? */
#define TOPICSEARCH_ALT
/* '/?' command after failing to switch even attempts just a basic text search as last resort? */
#define TOPICSEARCH_ALT_BASIC
void cmd_the_guide(byte init_search_type, int init_lineno, char* init_search_string) {
	static int line = 0, line_before_search = 0, jumped_to_line = 0;
	static char lastsearch[MAX_CHARS] = { 0 };
	static char lastchapter[MAX_CHARS] = { 0 };

	bool inkey_msg_old, within, within_col, searchwrap = FALSE, skip_redraw = FALSE, backwards = FALSE, restore_pos = FALSE, marking = FALSE, marking_after = FALSE;
	int bottomline = (screen_hgt > SCREEN_HGT ? 46 - 1 : 24 - 1), maxlines = (screen_hgt > SCREEN_HGT ? 46 - 4 : 24 - 4);
	int searchline = -1, within_cnt = 0, c = 0, n, line_presearch = line;
	char searchstr[MAX_CHARS], withinsearch[MAX_CHARS], chapter[MAX_CHARS]; //chapter[8]; -- now also used for terms
	char buf[MAX_CHARS * 2 + 1], buf2[MAX_CHARS * 2 + 1], *cp, *cp2, tempstr[MAX_CHARS + 1];
#ifndef BUFFER_GUIDE
	char path[1024], bufdummy[MAX_CHARS + 1];
	FILE *fff;
#else
	static int fseek_pseudo = 0; //fake a file position, within our buffered guide string array
#endif
	int i, j;
	char *res;
	byte search_uppercase = 0, search_uppercase_ok, fallback = FALSE, fallback_uppercase = 0, force_uppercase = FALSE;

	int c_override = 0, c_temp;
	char buf_override[MAX_CHARS];

#ifdef REGEX_SEARCH
	bool search_regexp = FALSE;
	int ires = -999;
	regex_t re_src;
	char searchstr_re[MAX_CHARS];
#endif

	int first_result = -1; //for QoL hack to drop from /? guide search to basic text search tier


	/* empty file? */
	if (guide_lastline == -1) {
		if (guide_errno <= 0) {
			c_message_add("\377yThe file TomeNET-Guide.txt seems to be empty.");
			c_message_add("\377y Try updating it via =U or the TomeNET-Updater or download it manually.");
		} else {
			if (guide_errno == ENOENT) {
				c_msg_format("\377yThe file TomeNET-Guide.txt wasn't found in your TomeNET folder.");
				c_message_add("\377y Try updating it via =U or the TomeNET-Updater or download it manually.");
			} else c_msg_format("\377yThe file TomeNET-Guide.txt couldn't be opened from your TomeNET folder (%d).", guide_errno);
		}
		return;
	}

#ifndef BUFFER_GUIDE
	path_build(path, 1024, "", "TomeNET-Guide.txt");
	errno = 0;
	fff = my_fopen(path, "r");
	/* mysteriously can't open file anymore? */
	if (!fff) {
		c_msg_format("\377yThe file TomeNET-Guide.txt was not found in your TomeNET folder anymore (%d).", errno);
		if (errno == ENOENT) c_message_add("\377y Try updating it via =U or the TomeNET-Updater or download it manually.");
		return;
	}
#endif

	/* init searches */
	chapter[0] = 0;
	searchstr[0] = 0;
	//lastsearch[0] = 0;

	/* In case we're invoked by /? command, always start searching from the beginning */
	if (!init_lineno && init_search_string) line = 0;

	/* invoked for a specific topic? */
	*buf_override = 0;

#ifdef GUIDE_BOOKMARKS
	/* Hack: Quick bookmark access */
	if (init_search_type == 3 && init_search_string && strlen(init_search_string) == 1 && init_search_string[0] >= 'a' && init_search_string[0] <= 't') {
		i = init_search_string[0] - 'a';
		/* Jump to bookmark */
		if (!bookmark_line[i]) {
			c_message_add(format("\377yBookmark %c) does not exist.", init_search_string[0]));
			return;
		}
		jumped_to_line = bookmark_line[i] + 1; //Remember, just as a small QoL thingy
 #if 0
		line = jumped_to_line - 1;
		if (line > guide_lastline - maxlines) line = guide_lastline - maxlines;
		if (line < 0) line = 0;
 #else
		init_search_type = 4; //We jump to the bookmark's line # now
		init_lineno = jumped_to_line;
		if (init_lineno > guide_lastline - maxlines) init_lineno = guide_lastline - maxlines;
		if (init_lineno < 0) init_lineno = 0;
 #endif
	}
#endif

	/* Hack: Translate specific searches from all-uppercase to chapter: */
	if ((init_search_type == 2 || init_search_type == 3) && init_search_string) {
		strcpy(buf, init_search_string);

		/* 'Troubleshooting' section, directly access it */
		if (!strncasecmp(buf, "prob", 4) || !strncasecmp(buf, "prb", 3)) {
			char *x = buf;
			int p;

			while (*x && !((*x < 'a' || *x > 'z') && (*x < 'A' || *x > 'Z'))) x++;
			p = atoi(x);
			*x = 0;

			if (my_strcasestr("problem", buf) || my_strcasestr("prblem", buf)) {
				if (!p) {
					init_search_type = 3;
					strcpy(init_search_string, "Troubleshooting");
				} else {
					init_search_type = 2;
					strcpy(init_search_string, "PROBLEM ");
					strcat(init_search_string, format("%d", p));
				}
			}
		}
		/* Guild setting[s]/config/cfg/opt[s]/options -> /guild_cfg */
		else if ((my_strcasestr(buf, "setting") || my_strcasestr(buf, "config") || my_strcasestr(buf, "cfg") || my_strcasestr(buf, "opt")) && my_strcasestr(buf, "guild")) {
			init_search_type = 2;
			strcpy(init_search_string, "/GUILD_CFG");
		}
		/* Undo/reset -> /undoskills (which also mentions the new experimental way in The Mirror) */
		else if (((my_strcasestr(buf, "undo") || my_strcasestr(buf, "reset")) && (my_strcasestr(buf, "skil") || my_strcasestr(buf, "spec"))) ||
		    !strcasecmp("undo", buf) || !strcasecmp("reset", buf) || !strcasecmp(buf, "respec") || !strcasecmp(buf, "respecc") || !strcasecmp(buf, "reskill") || !strcasecmp(buf, "reskil")) {
			init_search_type = 2;
			strcpy(init_search_string, "/UNDOSKILLS");
		}
		/* Timeout/delet(e|ion)/eras(e|ure) -> character/account timeout */
		else if (my_strcasestr(buf, "timeout") || my_strcasestr(buf, "time out") ||
		    ((my_strcasestr(buf, "delet") || my_strcasestr(buf, "eras")) && (my_strcasestr(buf, "char") || my_strcasestr(buf, "acc")))) {
			init_search_type = 3;
			strcpy(init_search_string, "timeout");
		}
		/* 'form[s]' becomes "Druid Forms" chapter if we're a druid */
		else if (((!strcasecmp(buf, "form") || !strcasecmp(buf, "forms")) && p_ptr->pclass == CLASS_DRUID)||
		    (my_strcasestr(buf, "form") && my_strcasestr(buf, "druid"))) {
			init_search_type = 3;
			strcpy(init_search_string, "Druid forms");
		}
		/* race / class stats tables */
		else if (my_strcasestr(buf, "race") && my_strcasestr(buf, "tab")) {
			init_search_type = 2;
			strcpy(init_search_string, "Race        STR");
		}
		else if (my_strcasestr(buf, "clas") && my_strcasestr(buf, "tab")) {
			init_search_type = 2;
			strcpy(init_search_string, "Class         STR");
		}
		/* Cloned from chapter search shortcuts */
		else if ((my_strcasestr(buf, "race") || my_strcasestr(buf, "racial"))
		    && (my_strcasestr(buf, "bonus") || my_strcasestr(buf, "boni") || my_strcasestr(buf, "malus") || my_strcasestr(buf, "mali") || my_strcasestr(buf, "tab"))) {//(table)
			init_search_type = 2;
			strcpy(init_search_string, "Race        STR");
		}
		/* Since there is nothing significant starting on 'table', actually accept this shortcut too */
		else if (init_search_type == 3 && my_strcasestr("table", buf) && strlen(buf) >= 3) {
			init_search_type = 2;
			strcpy(init_search_string, "Race        STR");
		}
		else if (my_strcasestr(buf, "class")
		    && (my_strcasestr(buf, "bonus") || my_strcasestr(buf, "boni") || my_strcasestr(buf, "malus") || my_strcasestr(buf, "mali") || my_strcasestr(buf, "tab"))) {//(table)
			init_search_type = 2;
			strcpy(init_search_string, "Class         STR");
		}
		/* Also allow directly jumping to any attribute */
		else if (!strcmp(buf, "STR")) {
			init_search_type = 2;
			strcpy(init_search_string, "Strength (STR)");
		} else if (!strcmp(buf, "INT")) {
			init_search_type = 2;
			strcpy(init_search_string, "Intelligence (INT)");
		} else if (!strcmp(buf, "WIS")) {
			init_search_type = 2;
			strcpy(init_search_string, "Wisdom (WIS)");
		} else if (!strcmp(buf, "DEX")) {
			init_search_type = 2;
			strcpy(init_search_string, "Dexterity (DEX)");
		} else if (!strcmp(buf, "CON")) {
			init_search_type = 2;
			strcpy(init_search_string, "Constitution (CON)");
		} else if (!strcmp(buf, "CHR")) {
			init_search_type = 2;
			strcpy(init_search_string,  "Charisma (CHR)");
		}
		else if (my_strcasestr(buf, "auto") && my_strcasestr(buf, "reincar")) strcpy(init_search_string, "reincarnation");
		else if (!strcasecmp(buf, "go")) strcpy(init_search_string, "Go challenge");
		/* Pft, inconsistency - basically, data uses 'color' while text uses 'colour'.. */
		else if (init_search_type == 3 && !strcasecmp("color", init_search_string)) strcpy(init_search_string, "colour");
		/* "(7.9) Parties, Iron Teams, Guilds" */
		else if (!strcasecmp(buf, "team") || !strcasecmp(buf, "iron team") || !strcasecmp(buf, "ironteam")) strcpy(init_search_string, "teams");
		else if (!strcasecmp(buf, "guild")) strcpy(init_search_string, "guilds");
		else if (!strcasecmp(buf, "party")) strcpy(init_search_string, "parties");
		else if (my_strcasestr(buf, "magical") && my_strcasestr(buf, "dev")) strcpy(init_search_string, "magic devices"); //NOT -> magic device (skill name)
		else if ((my_strcasestr(buf, "die") || my_strcasestr(buf, "dice")) && my_strcasestr(buf, "hp")) strcpy(init_search_string, "hp die");
		else if (!strcasecmp(buf, "death") || !strcasecmp(buf, "dead")) strcpy(init_search_string, "Death, Ghosts"); //chapter (3.11) about when you died
		else if (!strcasecmp("rescue", buf)) strcpy(init_search_string, "Getting Help");
		else if (!strcasecmp("mirror", buf)) strcpy(init_search_string, "The Mirror");
		else if (!strcasecmp("change fate", buf)) strcpy(init_search_string, "The Mirror");
		else if (my_strcasestr(buf, "shop") && my_strcasestr(buf, "serv")) strcpy(init_search_string, "services");
		else if (!strcasecmp(buf, "stor") || !strcasecmp(buf, "store") || !strcasecmp(buf, "stores")) strcpy(init_search_string, "Shops");
		else if (my_strcasestr(buf, "shop") && my_strcasestr(buf, "player")) strcpy(init_search_string, "Player stores");
		else if (my_strcasestr(buf, "store") && my_strcasestr(buf, "player")) strcpy(init_search_string, "Player stores");
		else if (!strcasecmp("rw", buf)) strcpy(init_search_string, "Nazgul");
		else if (!strcasecmp(buf, "win") || !strcasecmp(buf, "winning") || !strcasecmp(buf, "winner")|| !strcasecmp(buf, "winners")) strcpy(init_search_string, "goal");
		else if (!strcasecmp(buf, "king") || !strcasecmp(buf, "queen") || !strcasecmp(buf, "emperor")|| !strcasecmp(buf, "empress")) strcpy(init_search_string, "goal");
		else if (!strcasecmp("OoD", buf)) strcpy(init_search_string, "OOD  "); //uh hacky: avoid overlap with OOD_xx flag
		else if ((cp = my_strcasestr(buf, "town")) && (cp2 = my_strcasestr(buf, "dun")) && cp < cp2) strcpy(init_search_string, "town dungeons");
		else if ((cp = my_strcasestr(buf, "town")) && (cp2 = my_strcasestr(buf, "dun")) && cp > cp2) strcpy(init_search_string, "IDDC");// dungeon towns
		else if (my_strcasestr(buf, "dun") && my_strcasestr(buf, "list")) strcpy(init_search_string, "Dungeon"); //which in turn gets expanded to "Dungeon         " sth. instead of chapter "Dungeons"
		else if (my_strcasestr(buf, "death") && my_strcasestr(buf, "msg")) strcpy(init_search_string, "death messages");
		else if (!strcasecmp("speed", buf)) strcpy(init_search_string, "time"); //for now redirect 'speed' to chapter about in-game time systems
		else if (!strcasecmp("city", buf)) strcpy(init_search_string, "town");
		else if (!strcasecmp("cities", buf)) strcpy(init_search_string, "towns");
		else if (!strcasecmp("dual wield", buf) || !strcasecmp("dual wielding", buf) || !strcasecmp("dual-wielding", buf)) strcpy(init_search_string, "dual-wield");
		else if (!strcasecmp("spell power", buf) || !strcasecmp("spell pow", buf)) strcpy(init_search_string, "spell-power");
		else if (!strcasecmp("crit", buf) || !strcasecmp("crits", buf) || !strcasecmp("critical", buf) ||
		    (my_strcasestr(buf, "crit") && (my_strcasestr(buf, "hit") || my_strcasestr(buf, "strike") || my_strcasestr(buf, "mon")))) {
			if (my_strcasestr(buf, "mon")) strcpy(init_search_string, "Critical hits by monsters"); //monster crits
			else strcpy(init_search_string, "critical-strike"); //player crits
		}
		else if (my_strcasestr(buf, "tech")) {
			int old = init_search_type;

			init_search_type = 2;
			 if (my_strcasestr(buf, "lev")) strcpy(init_search_string, "Technique levels");
			 else if (my_strcasestr(buf, "fight") || my_strcasestr(buf, "melee")) strcpy(init_search_string, "Fighting techniques");
			 else if (my_strcasestr(buf, "shoot") || my_strcasestr(buf, "range") || my_strcasestr(buf, "firing")) strcpy(init_search_string, "Shooting techniques");
			 else if (my_strcasestr(buf, "rogue")) strcpy(init_search_string, "Rogue-specific fighting techniques");
			 else if (my_strcasestr(buf, "other")) strcpy(init_search_string, "Other fighting techniques");
			 else init_search_type = old;
		}
		else if (!strcasecmp("encumbrance", buf)) strcpy(init_search_string, "encumberment");
		else if (my_strcasestr(buf, "auto") && my_strcasestr(buf, "destr")) strcpy(init_search_string, "auto-destr"); //auto-destroy
		else if (my_strcasestr(buf, "auto") && my_strcasestr(buf, "pick")) strcpy(init_search_string, "auto-pick"); //auto-pickup
		else if (my_strcasestr(buf, "auto") && my_strcasestr(buf, "ins")) strcpy(init_search_string, "auto-inscri"); //auto-inscription
		else if (my_strcasestr(buf, "forc") && my_strcasestr(buf, "stack")) strcpy(init_search_string, "force-stack");
		else if (!strcasecmp("key", buf) || !strcasecmp("keys", buf)) strcpy(init_search_string, "Command keys");
		else if (!strcasecmp("command", buf) || !strcasecmp("commands", buf)) strcpy(init_search_string, "Slash commands");
		else if (!strcasecmp("macro", buf) || !strcasecmp("macros", buf)) strcpy(init_search_string, "basic macros"); //3.6
		else if (!strcasecmp("UI", buf) || !strcasecmp("GUI", buf)) strcpy(init_search_string, "user interface");
		else if (!strcasecmp("fight", buf) || !strcasecmp("fighting", buf)) { //note: just search for 'melee' for chapter about melee combat/weapons
			init_search_type = 2;
			strcpy(init_search_string, "FIGHTING");
		} else if (!strcasecmp("shoot", buf) || !strcasecmp("shooting", buf)) strcpy(init_search_string, "Ranged");
		else if (!strcasecmp(buf, "bl")) strcpy(init_search_string, "blacklist");
		else if (!strcasecmp(buf, "!")) strcpy(init_search_string, "! inscription");
		else if (!strcasecmp(buf, "@")) strcpy(init_search_string, "macro");
		else if (!strcasecmp(buf, "slays") || !strcasecmp(buf, "brands") || !strcasecmp(buf, "brand")) strcpy(init_search_string, "slay"); //will actually invoke "Slaying vs brands"

		/* clean up */
		buf[0] = 0;
	}

	/* Searching for a slash command? Always translate to uppercase 's'earch */
	if (init_search_string && init_search_string[0] == '/') init_search_type = 2;
#if 0
	/* Searching for a predefined inscriptions? Always translate to uppercase 's'earch  */
	if (init_search_string && (init_search_string[0] == '@' || init_search_string[0] == '!') && !init_search_string[2]) {
		init_search_type = 2;
		force_uppercase = TRUE; //insc might not be alphanum, so force anyway, eg !*
	}
#else
	/* Hack: Inscriptions: Find either !<lowercase> or !<uppercase>, as specified */
	if (init_search_string && (init_search_string[0] == '@' || init_search_string[0] == '!') && !init_search_string[2]) {
		if (init_search_string[1] == toupper(init_search_string[1])) {
			init_search_type = 2;
			force_uppercase = TRUE;
		} else {
			init_search_type = 1;
			force_uppercase = FALSE;
			search_uppercase = 1;
		}
	}
#endif

#if 0 /* go to 'command-line options' instead */
	/* Searching for client commandline args? Always translate to uppercase 's'earch  */
	if (init_search_string && init_search_string[0] == '-' && !init_search_string[2]) {
		init_search_type = 2;
		force_uppercase = TRUE; //insc might not be alphanum, so force anyway, eg !*
	}
#else
	if (init_search_string && init_search_string[0] == '-' && !init_search_string[2]) {
 #ifdef WINDOWS
		strcpy(init_search_string, "WINDOWS COMMAND-LINE OPTIONS");
 #else
		strcpy(init_search_string, "POSIX COMMAND-LINE OPTIONS");
 #endif
		init_search_type = 2;
		force_uppercase = TRUE; //insc might not be alphanum, so force anyway, eg !*
		//fallback = TRUE;
		//fallback_uppercase = 4;
	}
#endif

	switch (init_search_type) {
	case 1:
		c_override = 's';
		strcpy(buf_override, init_search_string);
		break;
	case 2:
		c_override = 's';
		strcpy(buf_override, init_search_string);
		res = buf_override; //abuse res
		while (*res) { *res = toupper(*res); res++; }
		break;
	case 3:
		c_override = 'c';
		strcpy(buf_override, init_search_string);
		break;
	case 4:
		c_override = '#';
		strcpy(buf_override, format("%d", init_lineno));
		break;
	default: ; //0
	}

	Term_save();

	inkey_interact_macros = TRUE; /* Advantage: Non-command macros on Backspace etc won't interfere; Drawback: Cannot use up/down while numlock is off - unless ALLOW_NAVI_KEYS_IN_PROMPT is enabled! */

	while (TRUE) {
		/* We just wanted to do a chapter search which resulted a hard-coded substitution that isn't a real chapter but instead falls back to normal search? */
		if (fallback) {
			strcpy(searchstr, buf);
			fallback = FALSE;

			/* Hack: Perform a normal search */

			line_before_search = line;
			line_presearch = line;
			/* Skip the line we're currently in, start with the next line actually */
			line++;
			if (line > guide_lastline - maxlines) line = guide_lastline - maxlines;
			if (line < 0) line = 0;

			if (!fallback_uppercase) search_uppercase = FALSE;
			else {
				search_uppercase = fallback_uppercase;
				fallback_uppercase = FALSE;
			}

			strcpy(lastsearch, searchstr);
			searchline = line - 1; //init searchline for string-search
		}

		if (!skip_redraw) Term_clear();
		if (backwards) Term_putstr(23,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d]", (guide_lastline - line) + 1, guide_lastline + 1));
		else Term_putstr(23,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d]", line + 1, guide_lastline + 1));
#ifdef REGEX_SEARCH
		Term_putstr(26, bottomline, -1, TERM_L_BLUE, " s/r/R,d,D/f,a/A,S,c,#:src,nx,pv,mark,rs,chpt,line");
 #ifdef GUIDE_BOOKMARKS
		Term_putstr(77, bottomline, -1, TERM_L_UMBER, "b/B");
 #endif
#else
		Term_putstr(26, bottomline, -1, TERM_L_BLUE, " s,d,D/f,S,c,#:srch,nxt,prv,reset,chapter,line");
 #ifdef GUIDE_BOOKMARKS
		Term_putstr(74, bottomline, -1, TERM_L_UMBER, "b/B");
 #endif
#endif
		Term_putstr( 7, bottomline, -1, TERM_SLATE,  "SPC/n,p,ENT,BCK,ESC");
		Term_putstr( 0, bottomline, -1, TERM_YELLOW, "?:Help");
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
								if (*(cp + 4) == ')' || *(cp + 5) == ')' ||
								    (*(cp + 6) == ')' && *(cp + 5) > '9') || /* > 9 checks: ignore "(0.500)" skill stuff */
								    (*(cp + 6) == ')' && *(cp + 4) > '9')) {
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

			/* Hack: keep last search results marked all the time, even while just navigating (pg)up/dn etc.. */
			if (marking && !searchstr[0]) strcpy(searchstr, lastsearch);

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
					if (first_result == -1) first_result = searchline;

					/* Hack: Abuse normal 's' search to colourize */
					strcpy(withinsearch, chapter);

					chapter[0] = 0;
					line = searchline;
					/* Redraw line number display */
					if (backwards) Term_putstr(23,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d]", (guide_lastline - line) + 1, guide_lastline + 1));
					else Term_putstr(23,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d]", line + 1, guide_lastline + 1));

#if 0
					/* Hack: Automatically mark all subsequent results too */
					strcpy(searchstr, lastsearch);
					marking_after = TRUE;
#endif
#if 1
					/* Hack: Prevent auto-marking all subsequent results on any next display refresh (eg from a simple navigational keypress).
					   User may just press 'a' to re-mark everything if wanted. */
					marking_after = marking = FALSE;
#endif
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
					if (first_result == -1) first_result = searchline;

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

			/* Just mark last search's results within currently visible guide piece */
			else if (marking) {
#if 0 /* 0'ed is a hack: keep last search results marked all the time, even while just navigating (pg)up/dn etc.. */
				marking = FALSE;
#endif
				strcpy(withinsearch, searchstr);
				searchstr[0] = 0;
			}

			/* Search for specific string? */
			else if (searchstr[0]) {
				searchline++;

				/* Search wrapped around once and still no result? Finish. */
				if (searchwrap && searchline == line) {
					/* Hack: search_uppercase searches have multiple tiers of strictness, that result in another search with lowered strictness before giving up. */
					if (search_uppercase > 1) {
						search_uppercase--;
						searchwrap = FALSE;
						n--;
						continue;
					}

					/* We're done (unsuccessfully), clean up.. */
					searchstr[0] = 0;
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
				}

#ifdef REGEX_SEARCH
				/* We found a regexp result */
				else if (search_regexp) {
					if (my_strregexp_skipcol(buf2, re_src, searchstr_re, withinsearch, &i)) {
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

						if (first_result == -1) first_result = searchline;

						searchstr[0] = 0;
						searchwrap = FALSE;
						line = searchline;
						/* Redraw line number display */
						Term_putstr(23,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d]", line + 1, guide_lastline + 1));
					}
					/* Still searching */
					else {
						/* Skip all lines until we find the desired string */
						n--;
						continue;
					}
				}
#endif

				/* We found a result (non-regexp) */
				else if (my_strcasestr_skipcol(buf2, searchstr, search_uppercase)) {
					/* QoL hack for guide search via /? command:
					   Reduce searching tier once we wrapped around and are back to our first hit.
					   Maybe todo: Reduce chapter-tier to uppercase-4 tier first, and slowly reduce tier further,
					   instead of going down to basic text search level right away. Maybe this is better though. */
					if (first_result == -1) first_result = searchline;
					else if (searchline == first_result && (init_search_type == 2 || init_search_type == 3)) { /* we came here via /? command for upper-case or chapter search */
						/* Inform user what's going on, ie why he suddenly gets so many search results */
						//c_message_add("Guide-search wrapped around, dropping search tier to basic text search.");

						/* Drop search tier */
						search_uppercase = 0;
						//if (lastchapter[0] != '(')
						//if (lastchapter[0]) strcpy(searchstr, lastchapter);

						/* Continue search from anew in this tier! */
						line = searchline;
						searchwrap = FALSE;
						/* Disable checks to avoid loop:
						   Ie we're not going to need this hack anymore, as we already arrived at the bottom tier (basic text search). */
						first_result = guide_lastline + 1;

						/* Ignore this repeated match and continue renewed search */
						n--;
						continue;
					}

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

					strcpy(withinsearch, searchstr);
					searchstr[0] = 0;
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
#ifdef REGEX_SEARCH
			if (search_regexp) {
				if (withinsearch[0] && my_strregexp_skipcol(buf2, re_src, searchstr_re, withinsearch, &i)) {
					/* Hack for '^' regexp. Problem is that we call my_.. below 'at the beginning' every time, as we just advance cp!
					   So a regexp starting on '^' will match any results regardless of their real position: */
					bool hax = searchstr_re[0] == '^', hax_done = FALSE;

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

						/* A bit hacky: Only accept matches at the beginning of our cp string,
						   to do this we check whether match length (strlen) is same as the end-of-match pointer (i) */
						if (!hax_done && my_strregexp_skipcol(cp, re_src, searchstr_re, withinsearch, &i) && i == strlen(withinsearch)) {
							if (hax) hax_done = TRUE;

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
			} else
#endif
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
		if (searchstr[0]) {
			if (!searchwrap) {
				searchline = -1; //start from the beginning of the file again
				searchwrap = TRUE;
				continue;
			} else { //never reached now, since searchwrap = FALSE is set in search code above already
				/* finally end search, without any results */
				searchstr[0] = 0;
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

		if (marking_after) {
			marking = TRUE;
			marking_after = FALSE;
		}

		/* hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

#ifdef ENABLE_SHIFT_SPECIALKEYS
		inkey_shift_special = 0x0;
#endif
		/* we're here via /? command? */
		if (c_override) {
			if (c_override == 255) {
				/* we didn't find the topic we wanted to access via /? command? then just quit */
				if (init_search_string && !line) {
#ifdef TOPICSEARCH_ALT /* last chance: switch between search types and retry */
					char *x;

					/* change an all-caps search (2) to a chapter search (3) */
					if (init_search_type == 2) {
						strcpy(buf_override, init_search_string);
						x = buf_override;
						while (*x) { *x = tolower(*x); x++; }
 #ifdef TOPICSEARCH_ALT_BASIC /* even fall back to a standard text search, instead of clinging to any sort of 'topic'? */
						/* if we did that already, demote to standard text search instead */
						if (c == 'c') {
							c = 's';
							init_search_type = 0; /* no further attemps */
						}
						/* otherwise perform chapter search now */
						else
 #else
						init_search_type = 0; /* no further attemps */
 #endif
						c = 'c';
					}
					/* change a chapter search (3) to an all-caps search (2) */
					else if (init_search_type == 3) {
						strcpy(buf_override, init_search_string);
						x = buf_override;
 #ifdef TOPICSEARCH_ALT_BASIC /* even fall back to a standard text search, instead of clinging to any sort of 'topic'? */
						/* if we did that already, demote to standard text search instead */
						if (c == 's') {
							while (*x) { *x = tolower(*x); x++; }
							init_search_type = 0; /* no further attemps */
						} else
 #else
						init_search_type = 0; /* no further attemps */
 #endif
						{
							c = 's';
							while (*x) { *x = toupper(*x); x++; }
						}
					/* finally failed to find relevant text */
					} else
#endif
					{
						c = ESCAPE;
#ifndef TOPICSEARCH_ALT_BASIC
						c_message_add(format("Topic '%s' not found.", init_search_string));
#else
						c_message_add(format("Search term '%s' not found.", init_search_string));
#endif
					}
				} else c_override = 0;
			} else {
				c = c_override;
				c_override = 255;
			}
		}
		/* normal manual operation (resumed) */
		if (!c_override) {
#ifdef BUFFER_GUIDE
			/* Hack: Backtrack to find out in which chapter we currently are, and display it in the top line */
			int lc;
			char chapter_header[MAX_CHARS] = { 0 }, *lcc;

			/* Search for chapter headers */
			for (lc = line; lc >= guide_endofcontents; lc--) {
				if (guide_line[lc][0] == '(' && guide_line[lc][1] >= '0' && guide_line[lc][1] <= '9') {
					/* Major chapter */
					if (guide_line[lc][2] == ')') { /* (x) */
						strcpy(chapter_header, guide_line[lc]);
						chapter_header[3] = 0;
						break;
					}
					/* Minor chapter */
					else if (guide_line[lc][2] == '.' && guide_line[lc][3] >= '0' && guide_line[lc][3] <= '9'
					    && (lcc = strchr(guide_line[lc], ')')) <= guide_line[lc] + 8) { /* (x.y...), max (x.yzAbX) */
						strcpy(chapter_header, guide_line[lc]);
						*(chapter_header + (lcc - guide_line[lc]) + 1) = 0;
						break;
					}
				}
			}
			/* Found one? */
			if (chapter_header[0]) Term_putstr(14,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d - chapter %s]", line + 1, guide_lastline + 1, chapter_header));
			/* Assume we're still in the guide contents listing, around the beginning of the guide */
			else Term_putstr(14,  0, -1, TERM_L_BLUE, format("[The Guide - line %5d of %5d - contents listing]", line + 1, guide_lastline + 1));
			/* hide cursor */
			Term->scr->cx = Term->wid;
			Term->scr->cu = 1;
#endif

			c = inkey_combo(FALSE, NULL, NULL);
		}

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
			xhtml_screenshot("screenshot????", FALSE);
			skip_redraw = TRUE;
			continue;

		/* navigate line up */
		case NAVI_KEY_UP:
		case '8': case '\010': case 0x7F: //rl:'k'
			line--;
			if (line < 0) line = guide_lastline - maxlines + 1;
			if (line < 0) line = 0;
			continue;

		/* navigate line down */
		case NAVI_KEY_DOWN:
		case '2': case '\r': case '\n': //rl:'j'
			line++;
			if (line > guide_lastline - maxlines + 1) line = 0;
			continue;

		/* page up */
		case NAVI_KEY_PAGEUP:
		case '9': case 'p': //rl:?
		case 'P':
			if (line == 0) line = guide_lastline - maxlines + 1;
#ifdef ENABLE_SHIFT_SPECIALKEYS
			else if (inkey_shift_special & 0x1) line -= maxlines / 2;
#endif
			else line -= maxlines;
			if (line < 0) line = 0;
			continue;

		/* page down */
		case NAVI_KEY_PAGEDOWN:
		case '3': case 'n': case ' ': //rl:?
		case 'N':
			if (line < guide_lastline - maxlines) {
#ifdef ENABLE_SHIFT_SPECIALKEYS
				if (inkey_shift_special & 0x1) line += maxlines / 2;
				else
#endif
				line += maxlines;
				if (line > guide_lastline - maxlines + 1) line = guide_lastline - maxlines + 1;
				if (line < 0) line = 0;
			} else line = 0;
			continue;

		/* home key to reset */
		case NAVI_KEY_POS1:
		case '7': //rl:?
			line = 0;
			continue;

		/* support end key too.. */
		case NAVI_KEY_END:
		case '1': //rl:?
			line = guide_lastline - maxlines + 1;
			if (line < 0) line = 0;
			continue;

		/* Initiate '/?' command from client-side, for chapter-and-more combo search */
		case 'C':
			marking = marking_after = FALSE; //clear hack
#ifdef REGEX_SEARCH
			search_regexp = FALSE;
#endif
			Term_erase(0, bottomline, 80);
			Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter topic to search for: ");
#if 0
			buf[0] = 0;
#else
			strcpy(buf, lastchapter); //a small life hack
#endif
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			if (*buf_override) {
				strcpy(buf, buf_override);
				*buf_override = 0;
			} else {
				//askfor_aux(buf, 7, 0)); //was: numerical chapters only
				askfor_aux(buf, MAX_CHARS - 1, 0); //allow entering chapter terms too
			}
			inkey_msg = inkey_msg_old;
			if (!buf[0]) continue;

			/* Emulate /? */
			Send_msg(format("/? %s", buf));

#if 1 /* Close the guide screen before it gets reinvoked by the server, like for '/?' ? */
 #ifndef BUFFER_GUIDE
			my_fclose(fff);
 #endif
			Term_load();
 #ifdef REGEX_SEARCH
			if (!ires) regfree(&re_src);
 #endif
			return;
#else
			continue;
#endif

		/* seach for 'chapter': can be either a numerical one or a main term, such as race/class/skill names. */
		case 'c':
			marking = marking_after = FALSE; //clear hack
#ifdef REGEX_SEARCH
			search_regexp = FALSE;
#endif
			Term_erase(0, bottomline, 80);
			Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter chapter to jump to: ");
#if 0
			buf[0] = 0;
#else
			strcpy(buf, lastchapter); //a small life hack
#endif
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			if (*buf_override) {
				strcpy(buf, buf_override);
				*buf_override = 0;
			} else {
				//askfor_aux(buf, 7, 0)); //was: numerical chapters only
				askfor_aux(buf, MAX_CHARS - 1, 0); //allow entering chapter terms too
			}
			inkey_msg = inkey_msg_old;
			if (!buf[0]) continue;

			strcpy(lastchapter, buf); //a small life hack for remembering our chapter search term for subsequent normal searching
			strcpy(lastsearch, buf); //further small life hack - not sure if a great idea or not..

			/* Hack: keep last search results marked all the time, even while just navigating (pg)up/dn etc.. */
			marking_after = TRUE;

			/* abuse chapter searching for extra functionality: search for chapter about a specific main term? */
			if (isalpha(buf[0])) {
				int find;
				char tmpbuf[MAX_CHARS], temp_priority[MAX_CHARS] = { 0 }, *s;

				chapter[0] = 0;

				/* 'Troubleshooting' section, directly access it */
				if (!strncasecmp(buf, "prob", 4) || !strncasecmp(buf, "prb", 3)) {
					char *x = tmpbuf;
					int p;

					strcpy(tmpbuf, buf);
					while (*x && !((*x < 'a' || *x > 'z') && (*x < 'A' || *x > 'Z'))) x++;
					p = atoi(x);
					*x = 0;

					if (my_strcasestr("problem", tmpbuf) || my_strcasestr("prblem", tmpbuf)) {
						if (!p) strcpy(buf, "Troubleshooting");
						else {
							strcpy(buf, "PROBLEM ");
							strcat(buf, format("%d", p));
							fallback = TRUE;
							fallback_uppercase = 4;
							continue;
						}
					}

					/* clean up */
					tmpbuf[0] = 0;
				}

				if (!strcasecmp(buf, "about")) strcpy(buf, "introduction");

				/* Expand 'AC' to 'Armour Class' */
				if (!strcasecmp(buf, "ac")) strcpy(buf, "armour class");
				else if (my_strcasestr(buf, "armo") && my_strcasestr(buf, "clas")) strcpy(buf, "armour class"); //translate armor class to armour class
				/* Expand 'tc' to 'Treasure Class' */
				if (!strcasecmp(buf, "tc")) strcpy(buf, "treasure class");
				/* Expand 'am' to 'Anti-Magic' */
				if (!strcasecmp(buf, "am")) strcpy(buf, "anti-magic");
				/* Bag/Bags redirect to 'Subinventory' */
				if (!strcasecmp(buf, "bag") || !strcasecmp(buf, "bags") || my_strcasestr(buf, "container") ||
				    my_strcasestr(buf, "subinv") || my_strcasestr(buf, "sub-inv") || my_strcasestr(buf, "sub inv")) {
					strcpy(buf, "Subinventory:");
					fallback = TRUE;
					fallback_uppercase = 4;
					continue;
				}
				/* Expand 'Update' to 'Updating' */
				if (!strcasecmp(buf, "update")) strcpy(buf, "updating");


				if (my_strcasestr(buf, "auto") && !my_strcasestr(buf, "/auto") && my_strcasestr(buf, "ret")) strcpy(buf, "auto-ret"); //auto-retaliation, but not slash command

				/* Note: Class/race shortcuts such as hk, rm, mc, cp / ho, ht, he/he, de
				   aren't feasible because they are already partially in use for other things or colliding. */

				/* Melee weapon classes */
				if (my_strcasestr(buf, "weap") && my_strcasestr(buf, "clas")) strcpy(buf, "weapon types");
				else if (my_strcasestr(buf, "weap") && my_strcasestr(buf, "typ")) strcpy(buf, "weapon types");

				/* ENABLE_DEMOLITIONIST */
				if ((my_strcasestr("obtaining ", buf) && strlen(buf) >= 3) ||
				    my_strcasestr(buf, "chemic")) {
					strcpy(buf, "OBTAINING CHEMICALS/INGREDIENTS");
					fallback = TRUE;
					fallback_uppercase = 4;
					continue;
				}
				if ((my_strcasestr("charges", buf) && strlen(buf) >= 5 && !my_strcasestr(buf, "re")) ||
				    (my_strcasestr(buf, "reci") && !my_strcasestr(buf, "preci"))) { //recipes
					strcpy(buf, "CHARGE TYPE");
					fallback = TRUE;
					fallback_uppercase = 4;
					continue;
				}
				if (my_strcasestr(buf, "xp") && (my_strcasestr(buf, "table") || my_strcasestr(buf, "tabl") || my_strcasestr(buf, "tab"))) strcpy(buf, "Experience Points Table");
				if (my_strcasestr(buf, "demo") && my_strcasestr(buf, "char")) strcpy(buf, "Demolition Charges");
				/* Demolitionist perk */
				if (!strncasecmp(buf, "demol", 5) && !my_strcasestr(buf, "char")) { /* don't overlook chapter 'Demolition Charges'! */
					strcpy(buf, "DEMOLITIONIST");
					fallback = TRUE;
					fallback_uppercase = 4;
					line = 0; /* paranoia? */
					continue;
				}

				if (my_strcasestr(buf, "load") && (my_strcasestr(buf, "trap") || my_strcasestr(buf, "kit"))) strcpy(buf, "Trap kit load");
				else if (my_strcasestr(buf, "trap") && ((my_strcasestr(buf, "kit") && !my_strcasestr(buf, "bag")) || my_strcasestr(buf, "mon"))) { /* not 'trap kit bag'! */
					strcpy(buf, "ABOUT TRAP KITS:");
					fallback = TRUE;
					fallback_uppercase = 4;
					continue;
				} else if ((my_strcasestr(buf, "trap") || my_strcasestr(buf, "kit")) && my_strcasestr(buf, "bag")) {
					strcpy(buf, "Trap Kit Bag");
					fallback = TRUE;
					continue;
				}

				/* Race/class boni/mali table */
				if ((my_strcasestr(buf, "race") || my_strcasestr(buf, "racial"))
				    && (my_strcasestr(buf, "bonus") || my_strcasestr(buf, "boni") || my_strcasestr(buf, "malus") || my_strcasestr(buf, "mali") || my_strcasestr(buf, "tab"))) {//(table)
					//strcpy(buf, "boni/mali of the different races");
					strcpy(buf, "Race        STR");
					fallback = TRUE;
					continue;
				}
				/* Since there is nothing significant starting on 'table', actually accept this shortcut too */
				if (my_strcasestr("table", buf) && strlen(buf) >= 3) {
					//strcpy(buf, "boni/mali of the different races");
					strcpy(buf, "Race        STR");
					fallback = TRUE;
					continue;
				}
				if (my_strcasestr(buf, "class")
				    && (my_strcasestr(buf, "bonus") || my_strcasestr(buf, "boni") || my_strcasestr(buf, "malus") || my_strcasestr(buf, "mali") || my_strcasestr(buf, "tab"))) {//(table)
					//strcpy(buf, "boni/mali of the different classes");
					strcpy(buf, "Class         STR");
					fallback = TRUE;
					continue;
				}

				/* Expand 'pfe' to 'Protection from evil' */
				if (!strcasecmp(buf, "pfe")) strcpy(buf, "Protection from evil");
				/* Expand 'rll' to 'Restore Life Levels' and fall back to caps-search */
				if (!strcasecmp(buf, "rll")) {
					strcpy(buf, "RESTORE LIFE LEVELS");
					fallback = TRUE;
					fallback_uppercase = 4;
					continue;
				}
				/* Expand 'wor' to 'Word of Recall' and fall back to caps-search */
				if (!strcasecmp(buf, "wor")) {
					strcpy(buf, "WORD OF RECALL");
					fallback = TRUE;
					fallback_uppercase = 4;
					continue;
				}
				/* Note: 'rop' is already ring of power (slang paragraph) */
				/* Slaying/Nothingness/Morgul weapons */
				if (!strcasecmp("slaying", buf)) {
					strcpy(buf, "WEAPONS OF SLAYING");
					fallback = TRUE;
					fallback_uppercase = 4;
					continue;
				}
				if (!strcasecmp("nothing", buf)) {
					strcpy(buf, "WEAPONS OF NOTHINGNESS");
					fallback = TRUE;
					fallback_uppercase = 4;
					continue;
				}
				if (!strcasecmp("morgul", buf)) {
					strcpy(buf, "WEAPONS OF MORGUL");
					fallback = TRUE;
					fallback_uppercase = 4;
					continue;
				}
				/* Ethereal/magic/silver ammo */
				if (my_strcasestr("amm", buf)) {
					if (my_strcasestr(buf, "mag")) {
						strcpy(buf, "MAGIC AMMUNITION");
						fallback = TRUE;
						fallback_uppercase = 4;
						continue;
					} else if (my_strcasestr(buf, "sil")) {
						strcpy(buf, "SILVER AMMUNITION");
						fallback = TRUE;
						fallback_uppercase = 4;
						continue;
					} else if (my_strcasestr(buf, "eth")) {
						strcpy(buf, "ETHEREAL AMMUNITION");
						fallback = TRUE;
						fallback_uppercase = 4;
						continue;
					}
				}

				/* Maia initiation (could just chapter-search "init", but somehow this seems more intuitive..) */
				if (!strncasecmp(buf, "enl", 3) && my_strcasestr("Enlightened:", buf)) {
					strcpy(buf, "ENLIGHTENED:");
					fallback = TRUE;
					fallback_uppercase = 4;
					line = 0; /* The correct chapter currently has the first hit from the beginning, while there are more 'wrong' hits coming up afterwards.. */
					continue;
				}
				if (!strncasecmp(buf, "cor", 3) && my_strcasestr("Corrupted:", buf)) {
					strcpy(buf, "CORRUPTED:");
					fallback = TRUE;
					fallback_uppercase = 4;
					line = 0; /* The correct chapter currently has the first hit from the beginning, while there are more 'wrong' hits coming up afterwards.. */
					continue;
				}
				/* Draconian traits */
				if (!strncasecmp(buf, "trai", 4) && my_strcasestr("traits:", buf)) {
					strcpy(buf, "Table of their traits:");
					fallback = TRUE;
					fallback_uppercase = 4;
					line = 0; /* The correct chapter currently has the first hit from the beginning, while there are more 'wrong' hits coming up afterwards.. */
					continue;
				}

				/* Rogue 'Cloaking' ability has no dedicated paragraph, use key list for it */
				if (!strcasecmp(buf, "cloak") || !strcasecmp(buf, "cloaking")) {
					strcpy(buf, "'cloaking mode'");
					fallback = TRUE;
					fallback_uppercase = 0;
					line = 0; /* The correct chapter currently has the first hit from the beginning, while there are more 'wrong' hits coming up afterwards.. */
					continue;
				}

				/* There is no Black Breath chapter but it is explained fully in the chapter containing Nazgul info */
				if ((my_strcasestr(buf, "black") == buf && my_strcasestr(buf, "brea")) || !strcasecmp(buf, "bb")) strcpy(buf, "nazgul");

				if (my_strcasestr(buf, "art") && my_strcasestr(buf, "cre")) strcpy(buf, "Artifact creation");

				/* The chapter explaining 'stats' is actually titled 'Attributes' */
				if (!strcasecmp(buf, "stats") || !strcasecmp(buf, "stat")) strcpy(buf, "Attributes");
				/* Also allow directly jumping to any attribute */
				if (!strcasecmp(buf, "str") || !strcasecmp(buf, "strength")) {
					strcpy(buf, "Strength (STR)");
					fallback = TRUE;
					continue;
				}
				if (!strcasecmp(buf, "int") || !strcasecmp(buf, "intelligence")) {
					strcpy(buf, "Intelligence (INT)");
					fallback = TRUE;
					continue;
				}
				if (!strcasecmp(buf, "wis") || !strcasecmp(buf, "wisdom")) {
					strcpy(buf, "Wisdom (WIS)");
					fallback = TRUE;
					continue;
				}
				if (!strcasecmp(buf, "dex") || !strcasecmp(buf, "dexterity")) {
					strcpy(buf, "Dexterity (DEX)");
					fallback = TRUE;
					continue;
				}
				if (!strcasecmp(buf, "con") || !strcasecmp(buf, "constitution")) {
					strcpy(buf, "Constitution (CON)");
					fallback = TRUE;
					continue;
				}
				if (!strcasecmp(buf, "chr") || !strcasecmp(buf, "charisma")) {
					strcpy(buf, "Charisma (CHR)");
					fallback = TRUE;
					continue;
				}

				/* Make searches for abilities not end up in 'probability travel'.. */
				if (my_strcasestr(buf, "abi") == buf) strcpy(buf, "Abilities");

				/* Expand 'pxx' and 'Pxx' to 'PROBLEM xx' */
				if ((buf[0] == 'p' || buf[0] == 'P') && buf[1] && buf[1] >= '0' && buf[1] <= '9') {
					sprintf(tmpbuf, "PROBLEM %d:", atoi(buf + 1));
					strcpy(buf, tmpbuf);
					fallback = TRUE;
					continue;
				}

				/* Misc chapters, hardcoded: */
				if ((my_strcasestr(buf, "inst") && !my_strcasestr(buf, "install"))
				    || (my_strcasestr(buf, "auto") && my_strcasestr(buf, "res"))
				    ) { /* mustn't overlap with 'install'; "res" is already taken by "resurrection" spell but we can accept 'auto-res' */
					strcpy(buf, "Temple  ");
					fallback = TRUE;
					continue;
				}

				if (!strcasecmp(buf, "Bree")
				    || my_strcasestr(buf, "Barrow") /* "Barr" is too short, collides with Frost Barrier */
				    //|| my_strcasestr(buf, "Downs")
				    || (!my_strcasestr(buf, "strain") && my_strcasestr(buf, "Train") && my_strcasestr(buf, "Tow"))
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
				if (my_strcasestr(buf, "Mandos") || my_strcasestr(buf, "Halls") || !strcasecmp(buf, "hom")) {
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
				if (my_strcasestr(buf, "Sandw") || (my_strcasestr(buf, "Lair") && !my_strcasestr(buf, "clair")) || !strcasecmp(buf, "swl") || !strcasecmp(buf, "sl")) {
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
				if (my_strcasestr(buf, "Ciri") || ((s = my_strcasestr(buf, "Ung")) && (s == buf || s[-1] == ' ') && !my_strcasestr(buf, "Ungoli")) || !strcasecmp(buf, "cu")) { //Ungoliant check is just paranoia, not a dungeon boss
					strcpy(chapter, "Cirith Ungol   ");
					continue;
				}
				if (my_strcasestr(buf, "Rhun") || !strcasecmp(buf, "lor")) {
					strcpy(chapter, "The Land of Rhun   ");
					continue;
				}
				if (my_strcasestr(buf, "Moria") || my_strcasestr(buf, "Mines") || !strcasecmp(buf, "mom")) { //was Mori, but collides with "memories"
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
				if (my_strcasestr(buf, "Subm") || (my_strcasestr(buf, "Ruin") && !my_strcasestr(buf, "Ruina")) || !strcasecmp(buf, "sr")) {//potion of ruination
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
				if (!strcasecmp(buf, "IDDC") || !strcasecmp(buf, "Ironman Deep Dive Challenge") || find >= 2) {
					strcpy(chapter, "Ironman Deep Dive Challenge (IDDC)");
					continue;
				}
				if ((!strcasecmp(buf, "HT") || !strcasecmp(buf, "HL") || my_strcasestr(buf, "Highla") || my_strcasestr(buf, "Tourn"))
				    || (my_strcasestr(buf, "Hi") && my_strcasestr(buf, "Tou"))) {
					strcpy(chapter, "Highlander Tournament");
					continue;
				}
				if (!strcasecmp(buf, "AMC") || (my_strcasestr(buf, "Arena"))) {// && my_strcasestr("Challenge", buf))) {
					strcpy(chapter, "Arena Monster Challenge");
					continue;
				}
				if (!strcasecmp(buf, "DK") || !strcasecmp(buf, "Dungeon Keeper") || my_strcasestr(buf, "Keeper")
				    || (my_strcasestr(buf, "Dun") && my_strcasestr(buf, "Kee"))) { //sorry, Death Knights
					strcpy(chapter, "Dungeon Keeper");
					continue;
				}
				if (!strcasecmp(buf, "XO") || !strcasecmp(buf, "Extermination Orders")
				    || my_strcasestr(buf, "Extermination")
				    || (my_strcasestr(buf, "Ex") && my_strcasestr(buf, "ord"))) {
					strcpy(chapter, "Extermination orders");
					continue;
				}
				if (my_strcasestr(buf, "Halloween")) {
					strcpy(chapter, "Halloween");
					continue;
				}
				if (!strcasecmp(buf, "Christmas") || !strcasecmp(buf, "xmas")) {
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

				if (!strcasecmp(buf, "ma")) {
					strcpy(chapter, ".Martial Arts");
					continue;
				}
				if (!strcasecmp(buf, "Io") || my_strcasestr(buf, "orders")
				    || (my_strcasestr(buf, "ord") && my_strcasestr(buf, "Item"))) {
					strcpy(buf, "Item orders");
					fallback = TRUE;
					continue;
				}
				if (!strcasecmp("macro", buf) || !strcasecmp("macros", buf))
					strcpy(buf, "basic macros"); //3.6

				/* -- Lua-defined chapters -- */

				/* Exact matches */

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

				/* moved up here, so Digging skill is selected before Digging chapter */
				for (i = 0; i < guide_skills; i++) {
					if (strcasecmp(guide_skill[i], buf) && //super exact match? user even entered correct + or .? heh!
					    (guide_skill[i][0] == '+' || guide_skill[i][0] == '.') && strcasecmp(guide_skill[i] + 1, buf)) continue; //exact match? (note: leading + or . must be skipped)
					strcpy(chapter, guide_skill[i]); //can be prefixed by either + or . (see guide.lua)
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

				/* Partial matches - Prefix */

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

				for (i = 0; i < guide_skills; i++) {
					if (my_strcasestr(guide_skill[i], buf) != guide_skill[i]) continue; //prefix match?
					strcpy(chapter, guide_skill[i]); //can be prefixed by either + or . (see guide.lua)
					break;
				}
				if (chapter[0]) continue;
				for (i = 0; i < guide_spells; i++) { //prefix match?
					if (my_strcasestr(guide_spell[i], buf) != guide_spell[i]) continue;
					strcpy(chapter, "    ");
					strcat(chapter, guide_spell[i]);
					break;
				}

#if 1 /* prefix partial match moved up to here, was just the last thing further down before */
				/* First try to match beginning of title */
				for (i = 0; i < guide_chapters; i++) {
					if (my_strcasestr(guide_chapter[i], buf) == guide_chapter[i]) {
						/* Keep for now, as we want to give this priority over any partial matches EXCEPT for some special shortcuts, see below.. */
						strcpy(temp_priority, "(");
						strcat(temp_priority, guide_chapter_no[i]);
						strcat(temp_priority, ")");
						break;
					}
				}
#endif

				/* Partial matches - Any */

			    if (!temp_priority[0]) {
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

				/* (Note: Partial chapter check comes last, further below.) */

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
			    } /* <temp_priority[] overrode these.> */

				/* Additions -- can override temp_priority[] (guide chapter prefix match) again. */
				if (my_strcasestr(buf, "Line") && !(my_strcasestr(buf, "command") || my_strcasestr(buf, "cmd"))) { //draconian lineages
					strcpy(chapter, "    Lineage");
					continue;
				}
				if (my_strcasestr(buf, "Line") && (my_strcasestr(buf, "command") || my_strcasestr(buf, "cmd"))) {
#ifdef WINDOWS
					strcpy(buf, "WINDOWS COMMAND-LINE OPTIONS");
#else
					strcpy(buf, "POSIX COMMAND-LINE OPTIONS");
#endif
					fallback = TRUE;
					fallback_uppercase = 4;
					continue;
				}
				if (!strcasecmp(buf, "option") || !strcasecmp(buf, "options")) strcpy(buf, "Client options");
				if (my_strcasestr(buf, "Depth")) { //min depth for exp in relation to char lv
					strcpy(chapter, "    Character level");
					continue;
				}
				if (my_strcasestr(buf, "Dung") && (my_strcasestr(buf, "typ") || my_strcasestr(buf, "Col"))) { //dungeon types/colours, same as staircases below
					strcpy(chapter, "Dungeon types");
					continue;
				}
				if (my_strcasestr(buf, "Boss")) { //dungeon bosses
					strcpy(chapter, "Dungeon, sorted by depth");
					continue;
				} else if (my_strcasestr("Dungeons", buf) && strlen(buf) >= 3) { //dungeons
					strcpy(chapter, "Dungeon                 ");
					continue;
				}
				if (my_strcasestr(buf, "Stair") && !my_strcasestr(buf, "goi")) { //staircase types, same as 'Dungeon types'
					strcpy(chapter, "Dungeon types");
					continue;
				}
				if (!strcasecmp(buf, "goi")) { //staircase types, same as 'Dungeon types'
					strcpy(buf, "GOI ");
					fallback = TRUE;
					fallback_uppercase = 4;
					continue;
				}
				if (my_strcasestr(buf, "pow") && my_strcasestr(buf, "ins")) {
					strcpy(buf, "ITEM POWER INSCRIPTION");
					fallback = TRUE;
					fallback_uppercase = 4;
					continue;
				}


			    /* Handle temp_priority[] at this point: */
			    if (temp_priority[0]) {
				strcpy(chapter, temp_priority);
				continue;
			    }

				/* If not matched, lastly try to (partially) match chapter titles */

#if 0 /* prefix partial match moved up */
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
#endif
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
				marking_after = FALSE;//unhack
				continue;
			}

			/* Search terms that don' start on alphanum char, but are not chapter numbers either: */

			/* 'Item power inscription' */
			if (!strcmp(buf, "@@") || !strcmp(buf, "@@@")) {
				strcpy(buf, "ITEM POWER INSCRIPTION");
				fallback = TRUE;
				fallback_uppercase = 4;
				line = 0; /* (just clean, not required) */
				continue;
			}

			/* -- From here on we assume a chapter number was specified -- */

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
			marking_after = FALSE;//unhack
			continue;
		/* search for keyword */
		case 's':
			marking = marking_after = FALSE; //clear hack
#ifdef REGEX_SEARCH
			search_regexp = FALSE;
#endif
			Term_erase(0, bottomline, 80);
			Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter search string: ");
#if 0
			searchstr[0] = 0;
#else
			/* life hack: if we searched for a non-numerical chapter string, make it auto-search target
			   for a (possibly happening subsequently to the chapter search) normal search too */
 #if 0
			if (lastchapter[0] != '(') strcpy(searchstr, lastchapter);
			else searchstr[0] = 0;
 #else
			strcpy(searchstr, lastchapter);
 #endif
#endif
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			if (*buf_override) {
				strcpy(searchstr, buf_override);
				*buf_override = 0;
			} else askfor_aux(searchstr, MAX_CHARS - 1, 0);
			inkey_msg = inkey_msg_old;
			if (!searchstr[0]) continue;

			line_before_search = line;
			line_presearch = line;
			/* Skip the line we're currently in, start with the next line actually */
			line++;
			if (line > guide_lastline - maxlines) line = guide_lastline - maxlines;
			if (line < 0) line = 0;

			/* Hack: If we enter all upper-case, search for exactly that, case sensitively */
			search_uppercase = 4;
			search_uppercase_ok = FALSE;
			for (n = 0; searchstr[n]; n++) {
				if (!search_uppercase_ok && isalpha(searchstr[n])) search_uppercase_ok = TRUE; /* Need at least one alpha char to initiate an upper-case search scenario */
				if (searchstr[n] == toupper(searchstr[n])) continue;
				search_uppercase = FALSE;
				break;
			}
			/* Exception: If first char is not alpha-num, don't do uppercase restriction (for "(STR)" etc);
			   exception from exception: Allow upper-case search for slash commands!
			   exception 2: Allow upper-case search for predefined inscriptions: */
			if (!isalphanum(searchstr[0]) && searchstr[0] != '/' && searchstr[0] != '*' && !((searchstr[0] == '!' || searchstr[0] == '@') && !searchstr[2])) search_uppercase_ok = FALSE; /* slash cmd, *destruction*, !x / @x inscription */
#if 0
			/* Hack: Inscriptions: Find both !<lowercase> and !<uppercase> */
			if ((searchstr[0] == '!' || searchstr[0] == '@') && !searchstr[2])) search_uppercase = 1; /* skip tier 4 and 3 and 2 (all-uppercase in actual text), start with 1 instead */
#else
			/* Hack: Inscriptions: Find either !<lowercase> or !<uppercase>, as specified */
			if ((searchstr[0] == '!' || searchstr[0] == '@') && !searchstr[2]) {
				if (searchstr[1] == toupper(searchstr[1])) search_uppercase = 2; /* skip tier 4 and 3 (all-uppercase in actual text), start with 2 instead */
				else search_uppercase = 1;
			}
#endif
#if 0 /* go to 'command-line options' instead */
			/* Hack: Commandline parm: Find both -<lowercase> and -<uppercase> */
			if (searchstr[0] == '-' && !searchstr[2]) search_uppercase = 2; /* skip tier 4 and 3 (all-uppercase in actual text), start with 2 instead */
#endif

			/* Check and go */
			if (!search_uppercase_ok) search_uppercase = FALSE; /* must contain at least 1 (upper-case) letter */
			if (force_uppercase) { //override? for non-alphanum inscriptions
				force_uppercase = FALSE;
				search_uppercase = 2;
			}

			/* Hack: Skip 'Going..' etc */
			if (!strcmp(searchstr, "GOI")) strcpy(searchstr, "GOI ");

			strcpy(lastsearch, searchstr);
			searchline = line - 1; //init searchline for string-search

			/* Hack: keep last search results marked all the time, even while just navigating (pg)up/dn etc.. */
			marking_after = TRUE;

			continue;
#ifdef REGEX_SEARCH
		/* search for regexp ^^ why not! */
		case 'r': /* <- case-insensitive */
			__attribute__ ((fallthrough));
		case 'R':
			marking = marking_after = FALSE; //clear hack

			if (c == 'r') i = REG_EXTENDED | REG_ICASE;// | REG_NEWLINE;
			else i = REG_EXTENDED;// | REG_NEWLINE;

			Term_erase(0, bottomline, 80);
			Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter search regexp: ");

			searchstr[0] = 0;

			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			askfor_aux(searchstr, MAX_CHARS - 1, 0);
			inkey_msg = inkey_msg_old;
			if (!searchstr[0]) continue;

			if (!ires) regfree(&re_src); /* release any previously compiled regexp first! */
			ires = regcomp(&re_src, searchstr, i);
			if (ires != 0) {
				c_msg_format("\377yInvalid regular expression (%d).", ires);
				searchstr[0] = 0;
				continue;
			}

			line_before_search = line;
			line_presearch = line;
			/* Skip the line we're currently in, start with the next line actually */
			line++;
			if (line > guide_lastline - maxlines) line = guide_lastline - maxlines;
			if (line < 0) line = 0;

			strcpy(lastsearch, searchstr);
			searchline = line - 1; //init searchline for string-search

			search_uppercase = FALSE;
			search_regexp = TRUE;
			strcpy(searchstr_re, searchstr); //clone just for ^/& evaluation later..

			/* Hack: keep last search results marked all the time, even while just navigating (pg)up/dn etc.. */
			marking_after = TRUE;

			continue;
#endif

		/* special function now: Reset search. Means: Go to where I was before searching. */
		case 'S':
			marking = marking_after = FALSE; //clear hack

			line = line_before_search;
			continue;
		/* search for next occurance of the previously used search keyword */
		case 'd':
			marking = marking_after = FALSE; //clear hack

			if (!lastsearch[0]) continue;

			line_presearch = line;
			/* Skip the line we're currently in, start with the next line actually */
			line++;
#if 0
			if (line > guide_lastline - maxlines) line = guide_lastline - maxlines;
			if (line < 0) line = 0;
#endif

			strcpy(searchstr, lastsearch);
			searchline = line - 1; //init searchline for string-search

			/* Hack: keep last search results marked all the time, even while just navigating (pg)up/dn etc.. */
			marking_after = TRUE;

			continue;
		/* Mark current search results on currently visible guide part */
		case 'a':
#if 1 /* causes erratic behaviour: 'a' again realigns document position */
			/* Specialty: Unhack, unmarking an active marking */
			if (marking) {
				marking = FALSE;
				continue;
			}
#endif

			if (!lastsearch[0]) continue;

			strcpy(searchstr, lastsearch);
			marking_after = TRUE;
			continue;
		/* Enter a new (non-regexp) mark string and mark it on currently visible guide part */
		case 'A':
#ifdef REGEX_SEARCH
			search_regexp = FALSE;
#endif
			search_uppercase = FALSE;
			chapter[0] = 0;

			Term_erase(0, bottomline, 80);
			Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter mark string: ");
#if 0
			searchstr[0] = 0;
#else
			/* life hack: if we searched for a non-numerical chapter string, make it auto-search target
			   for a (possibly happening subsequently to the chapter search) normal search too */
 #if 0
			if (lastchapter[0] != '(') strcpy(searchstr, lastchapter);
			else searchstr[0] = 0;
 #else
			strcpy(searchstr, lastchapter);
 #endif
#endif
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			askfor_aux(searchstr, MAX_CHARS - 1, 0);
			inkey_msg = inkey_msg_old;
			if (!searchstr[0]) continue;

			strcpy(lastsearch, searchstr);
			marking_after = TRUE;
			continue;
		/* search for previous occurance of the previously used search keyword */
		case 'D':
		case 'f':
			marking = marking_after = FALSE; //clear hack

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
			strcpy(searchstr, lastsearch);
			searchline = line - 1; //init searchline for string-search

			/* Hack: keep last search results marked all the time, even while just navigating (pg)up/dn etc.. */
			marking_after = TRUE;

			continue;
		/* jump to a specific line number */
		case '#':
			Term_erase(0, bottomline, 80);
			Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter line number to jump to: ");
			sprintf(buf, "%d", jumped_to_line);
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			if (*buf_override) {
				strcpy(buf, buf_override);
				*buf_override = 0;
			} else {
				if (!askfor_aux(buf, 7, 0)) {
					inkey_msg = inkey_msg_old;
					continue;
				}
			}
			inkey_msg = inkey_msg_old;
			jumped_to_line = atoi(buf); //Remember, just as a small QoL thingy
			line = jumped_to_line - 1;
			if (line > guide_lastline - maxlines) line = guide_lastline - maxlines;
			if (line < 0) line = 0;
			continue;

#ifdef GUIDE_BOOKMARKS
		case 'B': /* Add bookmark at current line */
			if (!line) {
				c_message_add(format("\377ySorry, cannot add a bookmark at the very first line."));
				continue;
			}
			j = 0;
			for (i = 0; i < GUIDE_BOOKMARKS; i++) {
				if (bookmark_line[i] == line && !j) {
					j = 1;
					c_message_add(format("\377y(Note: You have already bookmarked line %d in bookmark %c)", line + 1, 'a' + i));
				}
				if (bookmark_line[i]) continue;
				Term_erase(0, bottomline, 80);
				Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter description: ");
				inkey_msg_old = inkey_msg;
				inkey_msg = TRUE;
				tempstr[0] = 0;
				askfor_aux(tempstr, 60 - 1, 0);
				inkey_msg = inkey_msg_old;
				if (!tempstr[0]) break;
				strcpy(bookmark_name[i], tempstr);
				bookmark_line[i] = line;
				c_message_add(format("\377wAdded guide bookmark #%d at line %d ('%s')", i + 1, line + 1, tempstr));
				break;
			}
			if (i == GUIDE_BOOKMARKS) c_message_add(format("\377yMaximum amount of %d bookmarks reached, delete one first to add a new one.", GUIDE_BOOKMARKS));
			continue;
		case 'b': /* Bookmarks menu */
			while (TRUE) {
				Term_clear();
				Term_putstr(23,  0, -1, TERM_L_BLUE, "[The Guide - Bookmarks Menu]");
				//Term_putstr(14,  2, -1, TERM_WHITE, format("(You can set a maximum amount of %d bookmarks)", GUIDE_BOOKMARKS));
				Term_putstr(1,  2, -1, TERM_WHITE, format("(You can set a maximum amount of %d bookmarks. Use '\377s/? a-t\377w' for quick access.)", GUIDE_BOOKMARKS));
				for (i = 0; i < GUIDE_BOOKMARKS; i++) {
					if (!bookmark_line[i]) {
						//break;
						Term_putstr(4,  3 + i, -1, TERM_YELLOW, format("\377y%c\377w)", 'a' + i));
						continue;
					}
					Term_putstr(4,  3 + i, -1, TERM_YELLOW, format("\377y%c\377w) line %5d: %s", 'a' + i, bookmark_line[i] + 1, bookmark_name[i]));
				}
				Term_putstr( 0, 23, -1, TERM_L_BLUE, "   Press letter to jump to bookmark, SHIFT+letter to delete it, ESC to leave.");

				c_temp = inkey();
				if (c_temp == ESCAPE) break;
				if (c_temp >= 'a' && c_temp <= 'a' + GUIDE_BOOKMARKS - 1) {
					/* Jump to bookmark */
					if (!bookmark_line[c_temp - 'a']) {
						c_message_add(format("\377yBookmark %c) does not exist.", c_temp));
						continue;
					}
					jumped_to_line = bookmark_line[c_temp - 'a'] + 1; //Remember, just as a small QoL thingy
					line = jumped_to_line - 1;
					if (line > guide_lastline - maxlines) line = guide_lastline - maxlines;
					if (line < 0) line = 0;
					/* Hack exit */
					c_temp = ESCAPE;
				}
				if (c_temp >= 'A' && c_temp <= 'A' + GUIDE_BOOKMARKS - 1) {
					/* Delete bookmark */
					if (!bookmark_line[c_temp - 'A']) {
						c_message_add(format("\377yBookmark %c) does not exist.", c_temp - 'A' + 'a'));
						continue;
					}
					c_message_add(format("\377wDeleted guide bookmark #%d at line %d ('%s')", c_temp - 'A', bookmark_line[c_temp - 'A'] + 1, bookmark_name[c_temp - 'A']));
					for (i = c_temp - 'A'; i < GUIDE_BOOKMARKS - 1; i++) {
						bookmark_line[i] = bookmark_line[i + 1];
						strcpy(bookmark_name[i], bookmark_name[i + 1]);
					}
					bookmark_line[GUIDE_BOOKMARKS - 1] = 0;
				}
				if (c_temp == ESCAPE) break;
			}
			continue;
#endif

		case '?': //help
			Term_clear();
			Term_putstr(23,  0, -1, TERM_L_BLUE, "[The Guide - Navigation Keys]");
			i = 1;
			Term_putstr( 0, i++, -1, TERM_WHITE, "At the bottom of the guide screen you will see the following line:");
#ifdef REGEX_SEARCH
			Term_putstr(26,   i, -1, TERM_L_BLUE, " s/r/R,d,D/f,a/A,S,c,#:src,nx,pv,mark,rs,chpt,line");
 #ifdef GUIDE_BOOKMARKS
			Term_putstr(77,   i, -1, TERM_L_UMBER, "b/B");
 #endif
#else
			Term_putstr(26,   i, -1, TERM_L_BLUE, " s,d,D/f,S,c,#:srch,nxt,prv,reset,chapter,line");
 #ifdef GUIDE_BOOKMARKS
			Term_putstr(74,   i, -1, TERM_L_UMBER, "b/B");
 #endif
#endif
			Term_putstr( 7,   i, -1, TERM_SLATE,  "SPC/n,p,ENT,BCK,ESC");
			Term_putstr( 0, i++, -1, TERM_YELLOW, "?:Help");
			Term_putstr( 0, i++, -1, TERM_WHITE, "Those keys can be used to navigate the guide. Here's a detailed explanation:");
			Term_putstr( 0, i++, -1, TERM_WHITE, " Space,'n' / 'p': Move down / up by one page (ENTER/BACKSPACE move by one line).");
			Term_putstr( 0, i++, -1, TERM_WHITE, " 's'            : Search for a text string (use all uppercase for strict mode).");
#ifdef REGEX_SEARCH
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'r' / 'R'      : Search for a regular expression ('R' = case-sensitive).");
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'd'            : ..after 's/r/R', this jumps to the next match.");
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'D' or 'f'     : ..after 's/r/R', this jumps to the previous match.");
#else
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'd'            : ..after 's', this jumps to the next match.");
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'D' or 'f'     : ..after 's', this jumps to the previous match.");
#endif
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'a' / 'A'      : Mark old/new search results on currently visible guide part.");
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'S'            : ..resets screen to where you were before you did a search.");
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'c' / 'C'      : Chapter Search. A special search that skips most basic text");
			Term_putstr( 0, i++, -1, TERM_WHITE, "                  to only match specific topics and keywords. 'C' is like '/?'");
			Term_putstr( 0, i++, -1, TERM_WHITE, "                  command, encompassing more. Use 'c' for race, classe, skill,");
			Term_putstr( 0, i++, -1, TERM_WHITE, "                  spell, school, technique, dungeon, bosses, events, lineage,");
			Term_putstr( 0, i++, -1, TERM_WHITE, "                  depths, stair types, or actual chapter titles or indices.");
			Term_putstr( 0, i++, -1, TERM_WHITE, " '#'            : Jump to a specific line number.");
#ifdef GUIDE_BOOKMARKS
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'b' / 'B'      : Open bookmark menu / add bookmark at current position.");
#endif
			Term_putstr( 0, i++, -1, TERM_WHITE, " ESC            : The Escape key will exit the guide screen.");
			Term_putstr( 0, i++, -1, TERM_WHITE, "In addition, the arrow keys and the number pad keys can be used, and the keys");
			Term_putstr( 0, i++, -1, TERM_WHITE, "PgUp/PgDn/Home/End should work both on the main keyboard and the number pad.");
			Term_putstr( 0, i++, -1, TERM_WHITE, "This might depend on your specific OS flavour and desktop environment though.");
			Term_putstr( 0, i++, -1, TERM_WHITE, "Searching for all-caps only gives 'emphasized' results first in a line (flags).");
			Term_putstr(25, 23, -1, TERM_L_BLUE, "(Press any key to go back)");
			inkey();
			continue;

		/* exit */
		case ESCAPE: //case 'q': case KTRL('Q'):
#ifndef BUFFER_GUIDE
			my_fclose(fff);
#endif
			Term_load();
#ifdef REGEX_SEARCH
			if (!ires) regfree(&re_src);
#endif
			inkey_interact_macros = FALSE;
			return;
		}
	}

#ifndef BUFFER_GUIDE
	my_fclose(fff);
#endif
	Term_load();
	inkey_interact_macros = FALSE;
}

/* Added based on cmd_the_guide() to similarly browse the client-side spoiler files. - C. Blue
   Remember things such as last line visited, last search term.. for up to this - 1 different fixed files (0 is reserved for 'not saved'). */
#define MAX_REMEMBERANCE_INDICES 11
void browse_local_file(const char* angband_path, char* fname, int rememberance_index, bool reverse) {
	static int line_cur[MAX_REMEMBERANCE_INDICES] = { 0 }, line_before_search[MAX_REMEMBERANCE_INDICES] = { 0 }, jumped_to_line[MAX_REMEMBERANCE_INDICES] = { 0 }, file_lastline[MAX_REMEMBERANCE_INDICES] = { -1 };
	static char lastsearch[MAX_REMEMBERANCE_INDICES][MAX_CHARS] = { 0 };

	bool inkey_msg_old, within, within_col, searchwrap = FALSE, skip_redraw = FALSE, backwards = FALSE, restore_pos = FALSE, marking = FALSE, marking_after = FALSE;
	int bottomline = (screen_hgt > SCREEN_HGT ? 46 - 1 : 24 - 1), maxlines = (screen_hgt > SCREEN_HGT ? 46 - 4 : 24 - 4);
	int searchline = -1, within_cnt = 0, c = 0, n, line_presearch = line_cur[rememberance_index];
	char searchstr[MAX_CHARS], withinsearch[MAX_CHARS];
	char buf[MAX_CHARS * 2 + 1], buf2[MAX_CHARS * 2 + 1], *cp, *cp2, *buf2ptr;
	char path[1024];
	char *res;
#ifndef BUFFER_LOCAL_FILE
	char bufdummy[MAX_CHARS + 1];
#else
	static int fseek_pseudo = 0; //fake a file position, within our buffered guide string array
#endif
	FILE *fff;
	byte attr;

	int i;

#ifdef REGEX_SEARCH
	bool search_regexp = FALSE;
	int ires = -999;
	regex_t re_src;
	char searchstr_re[MAX_CHARS];
#endif

	bool syntax_W = (!streq(fname, "re_info.txt") && !streq(fname, "d_info.txt") && !streq(fname, "st_info.txt"));
	bool syntax_W2 = streq(fname, "re_info.txt");
	bool syntax_G = !streq(fname, "re_info.txt");
	bool syntax_S = streq(fname, "f_info.txt");
	char *tag_y, *tag_n, *tag_src;
	bool tag_ok;


	if (!rememberance_index || rememberance_index >= MAX_REMEMBERANCE_INDICES) {
		rememberance_index = 0;

		line_cur[rememberance_index] = 0;
		line_before_search[rememberance_index] = 0;
		jumped_to_line[rememberance_index] = 0;
		//lastsearch[rememberance_index][0] = 0; //maybe just keep this, doesn't hurt
	}

	file_lastline[rememberance_index] = -1; //always try anew whether the file is valid
	path_build(path, 1024, angband_path, fname);

	/* init the file */
	errno = 0;
	fff = my_fopen(path, "r");
	if (!fff) {
		if (errno == ENOENT) {
			c_msg_format("\377yThe file %s wasn't found in your lib/game folder.", fname);
			c_message_add("\377y Try updating with the TomeNET-Updater or download it manually.");
		} else c_msg_format("\377yThe file %s couldn't be opened from your lib/game folder (%d).", fname, errno);
		return;
	}

#ifdef BUFFER_LOCAL_FILE
	if (fgets(buf, 81 , fff)) file_lastline[rememberance_index] = 0;
#else
	/* count lines */
	while (fgets(buf, 81 , fff)) file_lastline[rememberance_index]++;
#endif
	my_fclose(fff);

	/* empty file? */
	if (file_lastline[rememberance_index] == -1) {
		if (errno <= 0) {
			c_msg_format("\377yThe file %s seems to be empty.", fname);
			c_message_add("\377y Try updating with the TomeNET-Updater or download it manually.");
		} else c_msg_format("\377yThe file %s couldn't be opened from your lib/game folder (%d).", fname, errno);
		return;
	}

	fff = my_fopen(path, "r");
#ifdef BUFFER_LOCAL_FILE
	/* buffer file, for easy reverse browsing */
	i = 0;
	if (reverse) {
		fseek(fff, -1, SEEK_END);
		while (fgets_inverse(buf, 81 , fff)) {
			//buf[strlen(buf) - 1] = 0; //strip trailing newlines -- done later
			strcpy(local_file_line[i++], buf);
			if (i == LOCAL_FILE_LINES_MAX) {
				c_msg_format("\377The file %s is too big to get buffered, cut off at line %d.", fname, LOCAL_FILE_LINES_MAX);
				break;
			}
		}
		file_lastline[rememberance_index] = i - 1;
	} else {
		while (fgets(buf, 81 , fff)) {
			//buf[strlen(buf) - 1] = 0; //strip trailing newlines -- done later
			strcpy(local_file_line[i++], buf);
			if (i == LOCAL_FILE_LINES_MAX) {
				c_msg_format("\377The file %s is too big to get buffered, cut off at line %d.", fname, LOCAL_FILE_LINES_MAX);
				break;
			}
		}
		file_lastline[rememberance_index] = i - 1;
	}
	my_fclose(fff);
#endif

	/* init searches */
	searchstr[0] = 0;
	//lastsearch[rememberance_index][0] = 0;

	Term_save();

	inkey_interact_macros = TRUE; /* Advantage: Non-command macros on Backspace etc won't interfere; Drawback: Cannot use up/down while numlock is off - unless ALLOW_NAVI_KEYS_IN_PROMPT is enabled! */

	while (TRUE) {
		if (!skip_redraw) Term_clear();

		if (line_cur[rememberance_index] == file_lastline[rememberance_index] - maxlines
		    || file_lastline[rememberance_index] < maxlines)
			attr = TERM_ORANGE;
		else attr = TERM_L_BLUE;

		if (backwards) Term_putstr(23,  0, -1, attr, format("[%s - line %5d of %5d]", fname, (file_lastline[rememberance_index] - line_cur[rememberance_index]) + 1, file_lastline[rememberance_index] + 1));
		else Term_putstr(23,  0, -1, attr, format("[%s - line %5d of %5d]", fname, line_cur[rememberance_index] + 1, file_lastline[rememberance_index] + 1));
#ifdef REGEX_SEARCH
		Term_putstr(26, bottomline, -1, TERM_L_BLUE, " s/r/R,d,D/f,a/A,S,#:src,nx,pv,mark,rs,chpt,line");
#else
		Term_putstr(26, bottomline, -1, TERM_L_BLUE, " s,d,D/f,S,#:srch,nxt,prv,reset,chapter,line");
#endif
		Term_putstr( 7, bottomline, -1, TERM_SLATE,  "SPC/n,p,ENT,BCK,ESC");
		Term_putstr( 0, bottomline, -1, TERM_YELLOW, "?:Help");
		if (skip_redraw) goto skipped_redraw;
		restore_pos = FALSE;

#ifndef BUFFER_LOCAL_FILE
		/* Always begin at zero */
		if (backwards) fseek(fff, -1, SEEK_END);
		else fseek(fff, 0, SEEK_SET);

		/* If we're not searching for something specific, just seek forwards until reaching our current starting line */
		if (backwards) {
			if (!searchwrap) for (n = 0; n < line_cur[rememberance_index]; n++) res = fgets_inverse(buf, 81, fff); //res just slays non-existant compiler warning..what
		} else {
			if (!searchwrap) for (n = 0; n < line_cur[rememberance_index]; n++) res = fgets(buf, 81, fff); //res just slays compiler warning
		}
#else
		/* Always begin at zero */
		if (backwards) fseek_pseudo = file_lastline[rememberance_index];
		else fseek_pseudo = 0;
		if (backwards) {
			if (!searchwrap) fseek_pseudo -= line_cur[rememberance_index];
		} else {
			if (!searchwrap) fseek_pseudo += line_cur[rememberance_index];
		}
#endif

		/* Display as many lines as fit on the screen, starting at the desired position */
		withinsearch[0] = 0;
		for (n = 0; n < maxlines; n++) {
#ifndef BUFFER_LOCAL_FILE
			if (backwards) res = fgets_inverse(buf, 81, fff);
			else res = fgets(buf, 81, fff);
#else
			if (backwards) {
				if (fseek_pseudo < 0) {
					fseek_pseudo = 0;
					res = NULL; //emulate EOF
				} else res = buf;
				strcpy(buf, local_file_line[fseek_pseudo]);
				fseek_pseudo--;
			} else {
				if (fseek_pseudo > file_lastline[rememberance_index]) {
					fseek_pseudo = file_lastline[rememberance_index];
					res = NULL; //emulate EOF
				} else res = buf;
				strcpy(buf, local_file_line[fseek_pseudo]);
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
				/* just copy each single character over */
				*cp2++ = *cp++;
			}
			*cp2 = 0;

			/* Hack: keep last search results marked all the time, even while just navigating (pg)up/dn etc.. */
			if (marking && !searchstr[0]) strcpy(searchstr, lastsearch[rememberance_index]);

			/* Just mark last search's results within currently visible file piece */
			if (marking) {
#if 0 /* 0'ed is a hack: keep last search results marked all the time, even while just navigating (pg)up/dn etc.. */
				marking = FALSE;
#endif
				strcpy(withinsearch, searchstr);
				searchstr[0] = 0;
			}

			/* Search for specific string? */
			else if (searchstr[0]) {
				searchline++;

				/* Search wrapped around once and still no result? Finish. */
				if (searchwrap && searchline == line_cur[rememberance_index]) {
					/* We're done (unsuccessfully), clean up.. */
					searchstr[0] = 0;
					searchwrap = FALSE;
					withinsearch[0] = 0;

					/* we cannot search for further results if there was none (would result in glitchy visuals) */
					//lastsearch[rememberance_index][0] = 0;

					/* correct our line number again */
					line_cur[rememberance_index] = line_presearch;
					backwards = FALSE;
					/* return to that position again */
					restore_pos = TRUE;
					break;
				}

#ifdef REGEX_SEARCH
				/* We found a regexp result */
				else if (search_regexp) {
					if (my_strregexp_skipcol(buf2, re_src, searchstr_re, withinsearch, &i)) {
						/* Reverse again to normal direction/location */
						if (backwards) {
							backwards = FALSE;
							searchline = file_lastline[rememberance_index] - searchline;
 #ifndef BUFFER_LOCAL_FILE
							/* Skip end of line, advancing to next line */
							fseek(fff, 1, SEEK_CUR);
							/* This line has already been read too, by fgets_inverse(), so skip too */
							res = fgets(bufdummy, 81, fff); //res just slays compiler warning
 #else
							fseek_pseudo += 2;
 #endif
						}

						searchstr[0] = 0;
						searchwrap = FALSE;
						line_cur[rememberance_index] = searchline;
						/* Redraw line number display */
						Term_putstr(23,  0, -1, TERM_L_BLUE, format("[%s - line %5d of %5d]", fname, line_cur[rememberance_index] + 1, file_lastline[rememberance_index] + 1));
					}
					/* Still searching */
					else {
						/* Skip all lines until we find the desired string */
						n--;
						continue;
					}
				}
#endif

				/* We found a result (non-regexp) */
				else if (my_strcasestr_skipcol(buf2, searchstr, FALSE)) {
					/* Reverse again to normal direction/location */
					if (backwards) {
						backwards = FALSE;
						searchline = file_lastline[rememberance_index] - searchline;
 #ifndef BUFFER_LOCAL_FILE
						/* Skip end of line, advancing to next line */
						fseek(fff, 1, SEEK_CUR);
						/* This line has already been read too, by fgets_inverse(), so skip too */
						res = fgets(bufdummy, 81, fff); //res just slays compiler warning
 #else
						fseek_pseudo += 2;
 #endif
					}

					strcpy(withinsearch, searchstr);
					searchstr[0] = 0;
					searchwrap = FALSE;
					line_cur[rememberance_index] = searchline;
					/* Redraw line number display */
					Term_putstr(23,  0, -1, TERM_L_BLUE, format("[%s - line %5d of %5d]", fname, line_cur[rememberance_index] + 1, file_lastline[rememberance_index] + 1));

				/* Still searching */
				} else {
					/* Skip all lines until we find the desired string */
					n--;
					continue;
				}
			}

			/* Colour all search finds */
#ifdef REGEX_SEARCH
			if (search_regexp) {
				if (withinsearch[0] && my_strregexp_skipcol(buf2, re_src, searchstr_re, withinsearch, &i)) {
					/* Hack for '^' regexp. Problem is that we call my_.. below 'at the beginning' every time, as we just advance cp!
					   So a regexp starting on '^' will match any results regardless of their real position: */
					bool hax = searchstr_re[0] == '^', hax_done = FALSE;

					strcpy(buf, buf2);

					cp = buf;
					cp2 = buf2;
					within = within_col = FALSE;
					while (*cp) {
						/* A bit hacky: Only accept matches at the beginning of our cp string,
						   to do this we check whether match length (strlen) is same as the end-of-match pointer (i) */
						if (!hax_done && my_strregexp_skipcol(cp, re_src, searchstr_re, withinsearch, &i) && i == strlen(withinsearch)) {
							if (hax) hax_done = TRUE;

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
								/* normal text */
								*cp2++ = 'w';
							}
						}

					}
					*cp2 = 0;
				}
			} else
#endif
			if (withinsearch[0] && my_strcasestr_skipcol(buf2, withinsearch, FALSE)) {
				strcpy(buf, buf2);

				cp = buf;
				cp2 = buf2;
				within = within_col = FALSE;
				while (*cp) {
					if (my_strcasestr_skipcol(cp, withinsearch, FALSE) == cp) {
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
							/* normal text */
							*cp2++ = 'w';
						}
					}

				}
				*cp2 = 0;
			}


			/* Some sort of weak 'syntax highlighting' for slightly better readability ^^" */
			buf2ptr = buf2;

			/* $..$ and $..! lines: Display these toggles in red ($!) and green ($$) */
			memset(buf, 0, sizeof(buf));
			cp = buf2ptr;
			if (buf2ptr[0] == '\377' && buf2ptr[1] == 'R') {
				strcat(buf, "\377R");
				tag_src = buf2ptr;

				buf2ptr += 2;
				cp = buf2ptr;
			} else tag_src = NULL;

			while (*buf2ptr == '$') {
				tag_y = strchr(buf2ptr + 1, '$');
				tag_n = strchr(buf2ptr + 1, '!');
				if (!tag_y && !tag_n) break; //catch broken $... tag
				if (tag_y && tag_n) {
					if (tag_y < tag_n) {
						tag_ok = TRUE;
						cp = tag_y + 1;
						if (tag_src > tag_y) tag_src = NULL;
						if (!tag_src) strcat(buf, "\377g");
					} else {
						tag_ok = FALSE;
						cp = tag_n + 1;
						if (tag_src > tag_n) tag_src = NULL;
						if (!tag_src) strcat(buf, "\377r");
					}
				} else if (tag_y) {
					tag_ok = TRUE;
					cp = tag_y + 1;
					if (tag_src > tag_y) tag_src = NULL;
					if (!tag_src) strcat(buf, "\377g");
				} else {
					tag_ok = FALSE;
					cp = tag_n + 1;
					if (tag_src > tag_n) tag_src = NULL;
					if (!tag_src) strcat(buf, "\377r");
				}

				/* If the $-tag contains (part of) a search result, we must replace the \377w marker accordingly */
				if ((cp2 = strstr(buf2ptr, "\377w")) && cp2 < cp)
					*(cp2 + 1) = (tag_ok ? 'g' : 'r'); /* Comments are dark grey instead of white */

				/* Add the colour-treated tag to the output line */
				strncat(buf, buf2ptr, (int)(cp - buf2ptr));
				// (no need to terminate buf after strncat cause we memset everything in buf to 0 at the start)

				tag_src = strstr(buf2ptr, "\377R");
				if (cp2 && cp2 < cp && tag_src < cp2) tag_src = NULL;
				if (tag_src > cp) tag_src = NULL;
				if (!tag_src) strcat(buf, "\377w"); /* Terminate $.. tag colour and start with standard colour again, for next tag/rest of the line */

				buf2ptr = cp;
			}
			i = strlen(cp);
			strcat(buf, cp);
			strcpy(buf2, buf);
			/* For the rest of the checks, statart AFTER the $.. tags, ie with buf2ptr instead of buf2. */
			buf2ptr = buf2 + (strlen(buf2) - i);

			/* W: lines - Level/depth in l-blue, Price/xp in yellow (re_info/d_info have different W-lines though.) */
			if (syntax_W && buf2ptr[0] == 'W' && buf2ptr[1] == ':') {
				/* Level/depth is at first ':' */
				cp = buf2ptr + 2;

				/* If the level/depth contains (part of) a search result, we must replace the \377w marker accordingly */
				while ((cp2 = strstr(cp, "\377w"))) {
					//if (atoi(cp2) == 127)
				//TODO: fix this so it treats search results correctly colourwise (tag_src)
					*(cp2 + 1) = 'B'; /* Level/depth are l-blue instead of white */
				}

				strncpy(buf, buf2ptr, (int)(cp - buf2ptr));
				buf[(int)(cp - buf2ptr)] = 0;
				cp2 = strchr(cp, ':');
				if (atoi(cp) == 127)
					strcat(buf, "\377s"); /* Level/depth 127 = not findable */
				else
					strcat(buf, "\377B"); /* Level/depth are l-blue instead of white */
				strncat(buf, cp, (int)(cp2 - cp));
				strcat(buf, "\377w"); // TODO: don't go back to white if we're within a search result! (tag_src)
				strcat(buf, cp2);

				strcpy(buf2ptr, buf);


				/* For price/xp find the last ':', store its location in 'cp' */
				cp2 = NULL;
				cp = buf2ptr;
				while ((cp = strchr(cp + 1, ':'))) cp2 = cp;
				if (cp2) {
					cp = cp2 + 1;

					/* If the comment contains (part of) a search result, we must replace the \377w marker accordingly */
					while ((cp2 = strstr(cp, "\377w")))
						*(cp2 + 1) = 'y'; /* Prices are yellow instead of white */

					strncpy(buf, buf2ptr, (int)(cp - buf2ptr));
					buf[(int)(cp - buf2ptr)] = 0;
					strcat(buf, "\377y"); /* Prices are yellow instead of white */
					strcat(buf, cp);

					strcpy(buf2ptr, buf);
				}
			}
			/* W: lines - +Level in l-blue, +xp in yellow (re_info only.) */
			if (syntax_W2 && buf2ptr[0] == 'W' && buf2ptr[1] == ':') {
			}

			/* G: lines (visuals) - have the symbol coloured in the given attr (re_info can have '*' for colour ie "don't change".) */
			if (syntax_G && buf2ptr[0] == 'W' && buf2ptr[1] == ':') {
			}

			/* S: lines (visuals) - shimmer colours are displayed in their actual colour (ONLY in f_info.txt! Different in some others (spells)) */
			if (syntax_S && buf2ptr[0] == 'W' && buf2ptr[1] == ':') {
			}

			/* Maybe todo: indicate by colour things that have 0 probability of being generated normally. */

			/* Automatically add colour to "#" comments */
			if ((cp = strchr(buf2ptr, '#')) && (cp == buf2ptr || *(cp -1) != ':')) {
				/* If the comment contains (part of) a search result, we must replace the \377w marker accordingly */
				while ((cp2 = strstr(cp, "\377w")))
					*(cp2 + 1) = 'D'; /* Comments are dark grey instead of white */

				strncpy(buf, buf2ptr, (int)(cp - buf2ptr));
				buf[(int)(cp - buf2ptr)] = 0;
				strcat(buf, "\377D"); /* Comments are dark grey instead of white */
				strcat(buf, cp);

				strcpy(buf2ptr, buf);
			}


			/* Display processed line, colourized by chapters and search results (and 'syntax' highlighting) */
			Term_putstr(0, 2 + n, -1, TERM_WHITE, buf2);
		}

		/* failed to find search string? wrap around and search once more,
		   this time from the beginning up to our actual posititon. */
		if (searchstr[0]) {
			if (!searchwrap) {
				searchline = -1; //start from the beginning of the file again
				searchwrap = TRUE;
				continue;
			} else { //never reached now, since searchwrap = FALSE is set in search code above already
				/* finally end search, without any results */
				searchstr[0] = 0;
				searchwrap = FALSE;

				/* correct our line number again */
				line_cur[rememberance_index] = line_presearch = line_cur[rememberance_index];
				if (backwards) backwards = FALSE;
				continue;
			}
		}
		if (restore_pos) continue;
		/* Reverse again to normal direction/location */
		if (backwards) {
			backwards = FALSE;
			line_cur[rememberance_index] = file_lastline[rememberance_index] - line_cur[rememberance_index];
			line_cur[rememberance_index]++;
		}

		skipped_redraw:
		skip_redraw = FALSE;

		if (marking_after) {
			marking = TRUE;
			marking_after = FALSE;
		}

		/* hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		c = inkey_combo(FALSE, NULL, NULL);

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
			xhtml_screenshot("screenshot????", FALSE);
			skip_redraw = TRUE;
			continue;

		/* navigate up */
		case NAVI_KEY_UP:
		case '8': case '\010': case 0x7F: //rl:'k'
			line_cur[rememberance_index]--;
			if (line_cur[rememberance_index] < 0) line_cur[rememberance_index] = file_lastline[rememberance_index] - maxlines + 1;
			if (line_cur[rememberance_index] < 0) line_cur[rememberance_index] = 0;
			continue;

		/* navigate down */
		case NAVI_KEY_DOWN:
		case '2': case '\r': case '\n': //rl:'j'
			line_cur[rememberance_index]++;
			if (line_cur[rememberance_index] > file_lastline[rememberance_index] - maxlines + 1) line_cur[rememberance_index] = 0;
			continue;

		/* page up */
		case NAVI_KEY_PAGEUP:
		case '9': case 'p': //rl:?
			if (line_cur[rememberance_index] == 0) line_cur[rememberance_index] = file_lastline[rememberance_index] - maxlines + 1;
			else line_cur[rememberance_index] -= maxlines;
			if (line_cur[rememberance_index] < 0) line_cur[rememberance_index] = 0;
			continue;

		/* page down */
		case NAVI_KEY_PAGEDOWN:
		case '3': case 'n': case ' ': //rl:?
			if (line_cur[rememberance_index] < file_lastline[rememberance_index] - maxlines + 1) {
				line_cur[rememberance_index] += maxlines;
				if (line_cur[rememberance_index] > file_lastline[rememberance_index] - maxlines + 1) line_cur[rememberance_index] = file_lastline[rememberance_index] - maxlines + 1;
				if (line_cur[rememberance_index] < 0) line_cur[rememberance_index] = 0;
			} else line_cur[rememberance_index] = 0;
			continue;

		/* home key to reset */
		case NAVI_KEY_POS1:
		case '7': //rl:?
			line_cur[rememberance_index] = 0;
			continue;

		/* support end key too.. */
		case NAVI_KEY_END:
		case '1': //rl:?
			line_cur[rememberance_index] = file_lastline[rememberance_index] - maxlines + 1;
			if (line_cur[rememberance_index] < 0) line_cur[rememberance_index] = 0;
			continue;

		/* search for keyword */
		case 's':
			marking = marking_after = FALSE; //clear hack
#ifdef REGEX_SEARCH
			search_regexp = FALSE;
#endif
			Term_erase(0, bottomline, 80);
			Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter search string: ");
			if (lastsearch[rememberance_index][0]) strcpy(searchstr, lastsearch[rememberance_index]);
			else searchstr[0] = 0;
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			askfor_aux(searchstr, MAX_CHARS - 1, 0);
			inkey_msg = inkey_msg_old;
			if (!searchstr[0]) continue;

			line_before_search[rememberance_index] = line_cur[rememberance_index];
			line_presearch = line_cur[rememberance_index];
			/* Skip the line we're currently in, start with the next line actually */
			line_cur[rememberance_index]++;
			if (line_cur[rememberance_index] > file_lastline[rememberance_index] - maxlines) line_cur[rememberance_index] = file_lastline[rememberance_index] - maxlines;
			if (line_cur[rememberance_index] < 0) line_cur[rememberance_index] = 0;

			strcpy(lastsearch[rememberance_index], searchstr);
			searchline = line_cur[rememberance_index] - 1; //init searchline for string-search

			/* Hack: keep last search results marked all the time, even while just navigating (pg)up/dn etc.. */
			marking_after = TRUE;

			continue;
#ifdef REGEX_SEARCH
		/* search for regexp ^^ why not! */
		case 'r': /* <- case-insensitive */
			__attribute__ ((fallthrough));
		case 'R':
			marking = marking_after = FALSE; //clear hack

			if (c == 'r') i = REG_EXTENDED | REG_ICASE;// | REG_NEWLINE;
			else i = REG_EXTENDED;// | REG_NEWLINE;

			Term_erase(0, bottomline, 80);
			Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter search regexp: ");

			if (lastsearch[rememberance_index][0]) strcpy(searchstr, lastsearch[rememberance_index]);
			else searchstr[0] = 0;

			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			askfor_aux(searchstr, MAX_CHARS - 1, 0);
			inkey_msg = inkey_msg_old;
			if (!searchstr[0]) continue;

			if (!ires) regfree(&re_src); /* release any previously compiled regexp first! */
			ires = regcomp(&re_src, searchstr, i);
			if (ires != 0) {
				c_msg_format("\377yInvalid regular expression (%d).", ires);
				searchstr[0] = 0;
				continue;
			}

			line_before_search[rememberance_index] = line_cur[rememberance_index];
			line_presearch = line_cur[rememberance_index];
			/* Skip the line we're currently in, start with the next line actually */
			line_cur[rememberance_index]++;
			if (line_cur[rememberance_index] > file_lastline[rememberance_index] - maxlines) line_cur[rememberance_index] = file_lastline[rememberance_index] - maxlines;
			if (line_cur[rememberance_index] < 0) line_cur[rememberance_index] = 0;

			strcpy(lastsearch[rememberance_index], searchstr);
			searchline = line_cur[rememberance_index] - 1; //init searchline for string-search

			search_regexp = TRUE;
			strcpy(searchstr_re, searchstr); //clone just for ^/& evaluation later..

			/* Hack: keep last search results marked all the time, even while just navigating (pg)up/dn etc.. */
			marking_after = TRUE;

			continue;
#endif
		/* special function now: Reset search. Means: Go to where I was before searching. */
		case 'S':
			marking = marking_after = FALSE; //clear hack

			line_cur[rememberance_index] = line_before_search[rememberance_index];
			continue;
		/* search for next occurance of the previously used search keyword */
		case 'd':
			marking = marking_after = FALSE; //clear hack

			if (!lastsearch[rememberance_index][0]) continue;

			line_presearch = line_cur[rememberance_index];
			/* Skip the line we're currently in, start with the next line actually */
			line_cur[rememberance_index]++;
#if 0
			if (line_cur[rememberance_index] > file_lastline[rememberance_index] - maxlines) line_cur[rememberance_index] = file_lastline[rememberance_index] - maxlines;
			if (line_cur[rememberance_index] < 0) line_cur[rememberance_index] = 0;
#endif

			strcpy(searchstr, lastsearch[rememberance_index]);
			searchline = line_cur[rememberance_index] - 1; //init searchline for string-search

			/* Hack: keep last search results marked all the time, even while just navigating (pg)up/dn etc.. */
			marking_after = TRUE;

			continue;
		/* Mark current search results on currently visible file part */
		case 'a':
#if 1 /* causes erratic behaviour: 'a' again realigns document position */
			/* Specialty: Unhack, unmarking an active marking */
			if (marking) {
				marking = FALSE;
				continue;
			}
#endif

			if (!lastsearch[rememberance_index][0]) continue;

			strcpy(searchstr, lastsearch[rememberance_index]);
			marking = TRUE;
			continue;
		/* Enter a new (non-regexp) mark string and mark it on currently visible file part */
		case 'A':
#ifdef REGEX_SEARCH
			search_regexp = FALSE;
#endif

			Term_erase(0, bottomline, 80);
			Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter mark string: ");
			if (lastsearch[rememberance_index][0]) strcpy(searchstr, lastsearch[rememberance_index]);
			else searchstr[0] = 0;
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			askfor_aux(searchstr, MAX_CHARS - 1, 0);
			inkey_msg = inkey_msg_old;
			if (!searchstr[0]) continue;

			strcpy(lastsearch[rememberance_index], searchstr);
			marking = TRUE;
			continue;
		/* search for previous occurance of the previously used search keyword */
		case 'D':
		case 'f':
			marking = marking_after = FALSE; //clear hack

			if (!lastsearch[rememberance_index][0]) continue;

			line_presearch = line_cur[rememberance_index];
			/* Inverse location/direction */
			backwards = TRUE;
			line_cur[rememberance_index] = file_lastline[rememberance_index] - line_cur[rememberance_index];
			/* Skip the line we're currently in, start with the next line actually */
			line_cur[rememberance_index]++;

#if 0
			if (line_cur[rememberance_index] > file_lastline[rememberance_index] - maxlines) line_cur[rememberance_index] = file_lastline[rememberance_index] - maxlines;
			if (line_cur[rememberance_index] < 0) line_cur[rememberance_index] = 0;
#endif
			strcpy(searchstr, lastsearch[rememberance_index]);
			searchline = line_cur[rememberance_index] - 1; //init searchline for string-search

			/* Hack: keep last search results marked all the time, even while just navigating (pg)up/dn etc.. */
			marking_after = TRUE;

			continue;
		/* jump to a specific line number */
		case '#':
			Term_erase(0, bottomline, 80);
			Term_putstr(0, bottomline, -1, TERM_YELLOW, "Enter line number to jump to: ");
			sprintf(buf, "%d", jumped_to_line[rememberance_index]);
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			if (!askfor_aux(buf, 7, 0)) {
				inkey_msg = inkey_msg_old;
				continue;
			}
			inkey_msg = inkey_msg_old;
			jumped_to_line[rememberance_index] = atoi(buf); //Remember, just as a small QoL thingy
			line_cur[rememberance_index] = jumped_to_line[rememberance_index] - 1;
			if (line_cur[rememberance_index] > file_lastline[rememberance_index] - maxlines) line_cur[rememberance_index] = file_lastline[rememberance_index] - maxlines;
			if (line_cur[rememberance_index] < 0) line_cur[rememberance_index] = 0;
			continue;

		case '?': //help
			Term_clear();
			Term_putstr(23,  0, -1, TERM_L_BLUE, "[File browsing - Navigation Keys]");
			i = 1;
			Term_putstr( 0, i++, -1, TERM_WHITE, "At the bottom of the file screen you will see the following line:");
#ifdef REGEX_SEARCH
			Term_putstr(26,   i, -1, TERM_L_BLUE, " s/r/R,d,D/f,a/A,S,#:src,nx,pv,mark,rs,chpt,line");
#else
			Term_putstr(26,   i, -1, TERM_L_BLUE, " s,d,D/f,S,#:srch,nxt,prv,reset,chapter,line");
#endif
			Term_putstr( 7,   i, -1, TERM_SLATE,  "SPC/n,p,ENT,BCK,ESC");
			Term_putstr( 0, i++, -1, TERM_YELLOW, "?:Help");
			Term_putstr( 0, i++, -1, TERM_WHITE, "Those keys can be used to navigate the file. Here's a detailed explanation:");
			Term_putstr( 0, i++, -1, TERM_WHITE, " Space,'n' / 'p': Move down / up by one page (ENTER/BACKSPACE move by one line).");
			Term_putstr( 0, i++, -1, TERM_WHITE, " 's'            : Search for a text string (use all uppercase for strict mode).");
#ifdef REGEX_SEARCH
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'r' / 'R'      : Search for a regular expression ('R' = case-sensitive).");
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'd'            : ..after 's/r/R', this jumps to the next match.");
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'D' or 'f'     : ..after 's/r/R', this jumps to the previous match.");
#else
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'd'            : ..after 's', this jumps to the next match.");
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'D' or 'f'     : ..after 's', this jumps to the previous match.");
#endif
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'a' / 'A'      : Mark old/new search results on currently visible file part.");
			Term_putstr( 0, i++, -1, TERM_WHITE, " 'S'            : ..resets screen to where you were before you did a search.");
			Term_putstr( 0, i++, -1, TERM_WHITE, " '#'            : Jump to a specific line number.");
			Term_putstr( 0, i++, -1, TERM_WHITE, " ESC            : The Escape key will exit the file screen.");
			Term_putstr( 0, i++, -1, TERM_WHITE, "In addition, the arrow keys and the number pad keys can be used, and the keys");
			Term_putstr( 0, i++, -1, TERM_WHITE, "PgUp/PgDn/Home/End should work both on the main keyboard and the number pad.");
			Term_putstr( 0, i++, -1, TERM_WHITE, "This might depend on your specific OS flavour and desktop environment though.");
			Term_putstr( 0, i++, -1, TERM_WHITE, "Searching for all-caps only gives 'emphasized' results first in a line (flags).");
			Term_putstr(25, 23, -1, TERM_L_BLUE, "(Press any key to go back)");
			inkey();
			continue;

		/* exit */
		case ESCAPE: //case 'q': case KTRL('Q'):
#ifndef BUFFER_LOCAL_FILE
			my_fclose(fff);
#endif
			Term_load();
#ifdef REGEX_SEARCH
			if (!ires) regfree(&re_src);
#endif
			inkey_interact_macros = FALSE;
			return;
		}
	}

#ifndef BUFFER_LOCAL_FILE
	my_fclose(fff);
#endif
	Term_load();
	inkey_interact_macros = FALSE;
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
	//Term_putstr(2,  3, -1, TERM_WHITE, "Press ENTER to display lore about the selected artifact.");
	Term_putstr(2,  3, -1, TERM_WHITE, "Note that some rings and amulets have a random colour and are just \377sgrey\377w here.");

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
		if (c == KTRL('T')) {
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", 2);
			Term_fresh();
#ifdef WINDOWS
			Sleep(2000);
#else
			usleep(2000000);
#endif
			Term_putstr(0,  0, -1, TERM_L_UMBER, "                                                  ");
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
			if (c == KTRL('T')) {
				/* Take a screenshot */
				xhtml_screenshot("screenshot????", 3);
				Term_fresh();
#ifdef WINDOWS
				Sleep(2000);
#else
				usleep(2000000);
#endif
				Term_putstr(0,  0, -1, TERM_L_UMBER, "                                                  ");
				Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Artifact Lore ***");
				/* hack: hide cursor */
				Term->scr->cx = Term->wid;
				Term->scr->cu = 1;
				continue;
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
	//char *hyphen;

	Term_save();

    s[0] = '\0'; s[1] = 0; s[2] = 0;
    while (TRUE) {
	Term_clear();
	Term_putstr(2,  2, -1, TERM_WHITE, "Enter (partial) monster name to refine the search:");
	//Term_putstr(2,  3, -1, TERM_WHITE, "Or type '#' and its index, or '!' and its symbol and colour or 'm' or 'M'.");
	//Term_putstr(2,  3, -1, TERM_WHITE, "Or press '!' followed by a monster symbol and a colour symbol or 'm' or 'M'.");
	//Term_putstr(2,  4, -1, TERM_WHITE, "Press RETURN to display lore about the selected monster.");
	//Term_putstr(2,  3, -1, TERM_WHITE, "Or type '#' and its index, or '!' and its symbol and colour (m=ANY, M=MULTI).");
	Term_putstr(2,  3, -1, TERM_WHITE, "Or #<index> or !<symbol><colour> (m=ANY, M=MULTI) or $LLL-LLL (level range).");

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

		/* Entering a '$' first means "search for monster level range" */
		if (s[0] == '$') {
			int levs = atoi(s + 1), leve;
			char *h = strchr(s, '-');

			if (levs < 0) levs = 0; /* levs was omitted? start at 0 */
			if (h) {
				leve = atoi(h + 1);
				if (!leve) leve = 999; /* '-' was given but no end level -> open end */
			} else leve = levs;

			if (s[1] && !pageoffset) for (i = 1; i < monster_list_idx; i++) {
				/* match? */
				if (monster_list_level[i] >= levs && monster_list_level[i] <= leve) {
					selected = monster_list_code[i];
					selected_list = i;
#if 0
					Term_putstr(5, 5, -1, selected_line == 0 ? TERM_YELLOW : TERM_UMBER, format("(%4d, \377%c%c\377%c, L%-3d)  %s",
					    monster_list_code[i], monster_list_symbol[i][0], monster_list_symbol[i][1], selected_line == 0 ? 'y' : 'u', monster_list_level[i], monster_list_name[i]));
#else
					if (Client_setup.r_char[monster_list_code[i]]) {
						Term_putstr(5, 5, -1, selected_line == 0 ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c/\377%c%c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u',
						    monster_list_symbol[i][0], Client_setup.r_char[monster_list_code[i]], n == selected_line ? 'y' : 'u', monster_list_name[i]));
					} else {
						Term_putstr(5, 5, -1, selected_line == 0 ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%-3c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], selected_line == 0 ? 'y' : 'u', monster_list_name[i]));
					}
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

				if (monster_list_level[i] >= levs && monster_list_level[i] <= leve) {
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
					if (Client_setup.r_char[monster_list_code[i]]) {
						Term_putstr(5, 5 + n, -1, n == selected_line ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c/\377%c%c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u',
						    monster_list_symbol[i][0], Client_setup.r_char[monster_list_code[i]], n == selected_line ? 'y' : 'u', monster_list_name[i]));
					} else {
						Term_putstr(5, 5 + n, -1, n == selected_line ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%-3c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u', monster_list_name[i]));
					}
#endif
					list_idx[n] = i;
					n++;
				}
			}
		}

		/* Entering a '!' first means "search for monster symbol instead" */
		else if (s[0] == '!') {
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
					if (Client_setup.r_char[monster_list_code[i]]) {
						Term_putstr(5, 5, -1, selected_line == 0 ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c/\377%c%c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u',
						    monster_list_symbol[i][0], Client_setup.r_char[monster_list_code[i]], n == selected_line ? 'y' : 'u', monster_list_name[i]));
					} else {
						Term_putstr(5, 5, -1, selected_line == 0 ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%-3c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], selected_line == 0 ? 'y' : 'u', monster_list_name[i]));
					}
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
					if (Client_setup.r_char[monster_list_code[i]]) {
						Term_putstr(5, 5 + n, -1, n == selected_line ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c/\377%c%c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u',
						    monster_list_symbol[i][0], Client_setup.r_char[monster_list_code[i]], n == selected_line ? 'y' : 'u', monster_list_name[i]));
					} else {
						Term_putstr(5, 5 + n, -1, n == selected_line ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%-3c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u', monster_list_name[i]));
					}
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
					if (Client_setup.r_char[monster_list_code[i]]) {
						Term_putstr(5, 5, -1, selected_line == 0 ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c/\377%c%c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u',
						    monster_list_symbol[i][0], Client_setup.r_char[monster_list_code[i]], n == selected_line ? 'y' : 'u', monster_list_name[i]));
					} else {
						Term_putstr(5, 5, -1, selected_line == 0 ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%-3c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], selected_line == 0 ? 'y' : 'u', monster_list_name[i]));
					}
#endif

					/* no beginning-of-line match yet? good. */
					if (!n) {
						list_idx[n] = i;
					} else { /* already got a beginning-of-line match? swap them, cause exact match always take pos #0 */
						int tmp = list_idx[0];

						list_idx[0] = i;
						list_idx[n] = tmp;

						/* redisplay the moved choice */
						if (Client_setup.r_char[monster_list_code[i]]) {
							Term_putstr(5, 5 + n, -1, selected_line == n ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c/\377%c%c\377%c)  %s",
								monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u',
								monster_list_symbol[i][0], Client_setup.r_char[monster_list_code[i]], n == selected_line ? 'y' : 'u', monster_list_name[i]));
						} else {
							Term_putstr(5, 5 + n, -1, selected_line == n ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%-3c\377%c)  %s",
								monster_list_code[list_idx[1]], monster_list_level[list_idx[1]], monster_list_symbol[list_idx[1]][0], monster_list_symbol[list_idx[1]][1], selected_line == n ? 'y' : 'u', monster_list_name[list_idx[1]]));
						}
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
					if (Client_setup.r_char[monster_list_code[i]]) {
						Term_putstr(5, 5 + n, -1, selected_line == n ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c/\377%c%c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u',
						    monster_list_symbol[i][0], Client_setup.r_char[monster_list_code[i]], n == selected_line ? 'y' : 'u', monster_list_name[i]));
					} else {
						Term_putstr(5, 5 + n, -1, selected_line == n ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%-3c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], selected_line == n ? 'y' : 'u', monster_list_name[i]));
					}
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
					if (Client_setup.r_char[monster_list_code[i]]) {
						Term_putstr(5, 5 + n, -1, n == selected_line ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%c\377%c/\377%c%c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u',
						    monster_list_symbol[i][0], Client_setup.r_char[monster_list_code[i]], n == selected_line ? 'y' : 'u', monster_list_name[i]));
					} else {
						Term_putstr(5, 5 + n, -1, n == selected_line ? TERM_YELLOW : TERM_UMBER, format("(%4d, L%-3d, \377%c%-3c\377%c)  %s",
						    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u', monster_list_name[i]));
					}
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
			if (Client_setup.r_char[monster_list_code[i]]) {
				Term_putstr(5, 5 + selected_line, -1, TERM_YELLOW, format("(%4d, L%-3d, \377%c%c\377%c/\377%c%c\377%c)  %s",
				    monster_list_code[i], monster_list_level[i], monster_list_symbol[i][0], monster_list_symbol[i][1], n == selected_line ? 'y' : 'u', monster_list_symbol[i][0], Client_setup.r_char[monster_list_code[i]], n == selected_line ? 'y' : 'u', monster_list_name[i]));
			} else {
				Term_putstr(5, 5 + selected_line, -1, TERM_YELLOW, format("(%4d, L%-3d, \377%c%-3c\377y)  %s",
				    monster_list_code[list_idx[selected_line]], monster_list_level[list_idx[selected_line]], monster_list_symbol[list_idx[selected_line]][0],
				    monster_list_symbol[list_idx[selected_line]][1], monster_list_name[list_idx[selected_line]]));
			}
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
		if (c == KTRL('T')) {
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", 2);
			Term_fresh();
#ifdef WINDOWS
			Sleep(2000);
#else
			usleep(2000000);
#endif
			Term_putstr(0,  0, -1, TERM_L_UMBER, "                                                  ");
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
		//if (s[0] == '$') hyphen = strchr(s, '-');
		if (s[0] != '#' && (s[0] != '$' || strlen(s) >= 8) && c == '8') { //rl:'k'
			if (n > 0) {
				if (selected_line > 0) selected_line--;
				else selected_line = n - 1;
			}
			continue;
		}
		if (s[0] != '#' && (s[0] != '$' || strlen(s) >= 8) && c == '2') { //rl:'j'
			if (n > 0) {
				if (selected_line < n - 1) selected_line++;
				else selected_line = 0;
			}
			continue;
		}

		/* page up/down in the search results */
		if (s[0] != '#' && (s[0] != '$' || strlen(s) >= 8) && c == '9') { //rl:?
			if (pageoffset > 0) pageoffset--;
			continue;
		}
		if (s[0] != '#' && (s[0] != '$' || strlen(s) >= 8) && c == '3') { //rl:?
			if (pageoffset < 100 /* silyl */
			    && more_results)
				pageoffset++;
			continue;
		}
		/* home key to reset */
		if (s[0] != '#' && (s[0] != '$' || strlen(s) >= 8) && c == '7') { //rl:?
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
		if (s[0] != '!' && s[0] != '$') c = toupper(c); /* We're looking for a name, not a symbol */
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
			monster_lore_aux(selected, selected_list, paste_lines, FALSE);
			Term_putstr(5,  23, -1, TERM_WHITE, "-- press ESC/Backspace to exit, SPACE for stats, c/C to chat-paste --");
		} else {
			monster_stats_aux(selected, selected_list, paste_lines, FALSE);
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
			if (c == KTRL('T')) {
				/* Take a screenshot */
				xhtml_screenshot("screenshot????", 3);
				Term_fresh();
#ifdef WINDOWS
				Sleep(2000);
#else
				usleep(2000000);
#endif
				Term_putstr(0,  0, -1, TERM_L_UMBER, "                                                  ");
				Term_putstr(5,  0, -1, TERM_L_UMBER, "*** Monster Lore ***");
				/* hack: hide cursor */
				Term->scr->cx = Term->wid;
				Term->scr->cu = 1;
				continue;
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
				/* paste currently displayed monster information into chat */
				if (show_lore) {
					for (i = 0; i < 18; i++) paste_lines[i][0] = '\0';
					monster_lore_aux(selected, selected_list, paste_lines, TRUE);
					for (i = 0; i < 18; i++) {
						if (!paste_lines[i][0]) break;
						if (paste_lines[i][strlen(paste_lines[i]) - 1] == ' ')
							paste_lines[i][strlen(paste_lines[i]) - 1] = '\0';
						Send_paste_msg(paste_lines[i]);
					}
				} else {
					for (i = 0; i < 18; i++) paste_lines[i][0] = '\0';
					monster_stats_aux(selected, selected_list, paste_lines, TRUE);
					for (i = 0; i < 18; i++) {
						if (!paste_lines[i][0]) break;
						if (paste_lines[i][strlen(paste_lines[i]) - 1] == ' ')
							paste_lines[i][strlen(paste_lines[i]) - 1] = '\0';
						Send_paste_msg(paste_lines[i]);
					}
				}
				break;
			}
			if (c == 'C') {
				/* paste lore AND stats into chat */
				for (i = 0; i < 18; i++) paste_lines[i][0] = '\0';
				monster_lore_aux(selected, selected_list, paste_lines, TRUE);
				for (i = 0; i < 18; i++) {
					if (!paste_lines[i][0]) break;
					//if (i == 6 || i == 12) usleep(10000000);
					if (paste_lines[i][strlen(paste_lines[i]) - 1] == ' ')
						paste_lines[i][strlen(paste_lines[i]) - 1] = '\0';
					Send_paste_msg(paste_lines[i]);
				}
				/* don't double-post the title: skip paste line 0 */
				for (i = 0; i < 18; i++) paste_lines[i][0] = '\0';
				monster_stats_aux(selected, selected_list, paste_lines, TRUE);
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

//#define SCREENSHOT_TARGET "tomenet-screenshot.png"
#define SCREENSHOT_TARGET (format("%s.png", screenshot_filename))
bool png_screenshot(void) {
#ifdef WINDOWS
	char path[1024], *c = path, *c2, tmp[1024], executable[1024];
#endif
#if !defined(WINDOWS) && !defined(USE_X11)
	/* Neither WINDOWS nor USE_X11 */
	c_msg_print("\377ySorry, creating a PNG file from a screenshot requires an X11 or Windows system.");
	return(FALSE);
#else
	char buf[1024], file_name[1024], command[1024];
	int k;
	FILE *fp;
	char height[5];

	if (!screenshot_filename[0]) {
		c_msg_print("\377yYou have not made a screenshot yet this session (CTRL+T).");
		return(FALSE);
	}
#endif

#ifdef WINDOWS
	// When NULL is passed to GetModuleHandle, the handle of the exe itself is returned
	HMODULE hModule = GetModuleHandle(NULL);
	if (hModule != NULL) {
		// Use GetModuleFileName() with module handle to get the path
		GetModuleFileName(hModule, path, (sizeof(path)));
	} else {
		c_message_add("Screenshot: Module handle is NULL");
		return(FALSE);
	}
	while ((c2 = strchr(c, '\\'))) c = c2 + 1;
	if (c == path) return(FALSE); /* Error: No valid path. */
	*c = 0; //remove 'TomeNET.exe' from path

	/* Build source xthml filename with full path */
	strcpy(buf, "file://");
	strcat(buf, path);
	path_build(file_name, 1024, ANGBAND_DIR_USER, screenshot_filename);
	strcat(buf, file_name);
	strcat(buf, ".xhtml");

	/* Put into quotations */
	strcpy(tmp, "\"");
	strcat(tmp, buf);
	strcat(tmp, "\"");
	strcpy(buf, tmp);

	/* Build target image file name with full path */
	path_build(file_name, 1024, ANGBAND_DIR_USER, SCREENSHOT_TARGET);
	strcat(path, file_name);
	strcpy(file_name, path);

	/* Put into quotations */
	strcpy(tmp, "\"");
	strcat(tmp, file_name);
	strcat(tmp, "\"");
	strcpy(file_name, tmp);

	/* Try chromium, then chrome, then firefox */
 #if 1 /* skip Chromium on Windows OS for now */
	if (FALSE) {
 #else
	k = system("chromium.exe --version"); // - ?
	//k = system("reg query \"HKEY_CURRENT_USER\\Software\\Google\\Chrome\\BLBeacon\" /v version");
	if (!k) {
 #endif
		/* Obtain path to browser */
		system("reg query \"HKEY_CLASSES_ROOT\\ChromiumHTML\\shell\\open\\command\" /ve > __temp__"); //<- just guessed the reg value, VERIFY!
		fp = fopen("__temp__", "r");
		fgets(tmp, 1024, fp);
		fgets(tmp, 1024, fp);
		fgets(tmp, 1024, fp);
		fclose(fp);
		c = strchr(tmp, '"');
		if (!c) return(FALSE); //error
		strcpy(executable, "\"");
		strcat(executable, c + 1);
		c = strchr(executable + 1, '"');
		if (!c) return(FALSE); //error
		*c = 0;
		strcat(executable, "\"");
		remove("__temp__");

		/* Bigmap screenshot */
		if (screenshot_height == MAX_WINDOW_HGT) strcpy(height, "650");
		/* Non-bigmap screenshot */
		else if (screenshot_height == DEFAULT_TERM_HGT) strcpy(height, "365");
		/* Custom size screenshot (new 2022) */
		else sprintf(height, "%d", 52 + screenshot_height * 13);

		//bug with webdriver_manager.chrome: requires --disable-gpu to work with headless, or "ERROR:gpu_init.cc(426) Passthrough is not supported, GL is disabled" heppens.
			//--disable-software-rasterizer
		sprintf(command, "%s --headless --disable-gpu --default-background-color=00000000 --window-size=640x%s --screenshot=%s %s",
		    executable,
		    height,
		    file_name,
		    buf);

		fp = fopen("__temp__.bat", "w");
		fputs(command, fp);
		fclose(fp);
		k = system("__temp__.bat");
		remove("__temp__.bat");

		if (k) c_msg_format("PNG screenshot via 'Chromium' failed (%d).", k);
		else {
			c_msg_format("PNG screenshot via 'Chromium' saved to %s.", SCREENSHOT_TARGET);
			return(TRUE);
		}
	} else {
		//k = system("chrome.exe --version");  -- chrome is buggy and doesn't report but just starts up
		k = system("reg query \"HKEY_CURRENT_USER\\Software\\Google\\Chrome\\BLBeacon\" /v version");
		if (!k) {
			/* Obtain path to browser */
			system("reg query \"HKEY_CLASSES_ROOT\\ChromeHTML\\shell\\open\\command\" /ve > __temp__");
			fp = fopen("__temp__", "r");
			fgets(tmp, 1024, fp);
			fgets(tmp, 1024, fp);
			fgets(tmp, 1024, fp);
			fclose(fp);
			c = strchr(tmp, '"');
			if (!c) return(FALSE); //error
			strcpy(executable, "\"");
			strcat(executable, c + 1);
			c = strchr(executable + 1, '"');
			if (!c) return(FALSE); //error
			*c = 0;
			strcat(executable, "\"");
			remove("__temp__");

			/* Bigmap screenshot */
			if (screenshot_height == MAX_WINDOW_HGT) strcpy(height, "650");
			/* Non-bigmap screenshot */
			else if (screenshot_height == DEFAULT_TERM_HGT) strcpy(height, "365");
			/* Custom size screenshot (new 2022) */
			else sprintf(height, "%d", 52 + screenshot_height * 13);

			//bug with webdriver_manager.chrome: requires --disable-gpu to work with headless, or "ERROR:gpu_init.cc(426) Passthrough is not supported, GL is disabled" heppens.
			//--disable-software-rasterizer
			sprintf(command, "%s --headless --disable-gpu --default-background-color=00000000 --window-size=640x%s --screenshot=%s %s",
			    executable,
			    height,
			    file_name,
			    buf);

			fp = fopen("__temp__.bat", "w");
			fputs(command, fp);
			fclose(fp);
			k = system("__temp__.bat");
			remove("__temp__.bat");

			if (k) c_msg_format("Automatic PNG screenshot with 'Chrome' failed (%d).", k);
			else {
				c_msg_format("PNG screenshot via 'Chrome' saved to %s.", SCREENSHOT_TARGET);
				return(TRUE);
			}
		} else {
			//k = system("firefox --version --headless");  -- error 9009 "file not found" wut? due to bad path escaping/quoting without using a .bat file probably
			k = system("reg query \"HKEY_CURRENT_USER\\Software\\Mozilla\\Firefox\" /v OldDefaultBrowserCommand");
			//HKEY_CURRENT_USER\Software\Mozilla\Firefox\Launcher
			if (!k) {
				/* Obtain path to browser */
				system("reg query \"HKEY_CLASSES_ROOT\\Applications\\firefox.exe\\shell\\open\\command\" /ve > __temp__");
				fp = fopen("__temp__", "r");
				fgets(tmp, 1024, fp);
				fgets(tmp, 1024, fp);
				fgets(tmp, 1024, fp);
				fclose(fp);
				c = strchr(tmp, '"');
				if (!c) return(FALSE); //error
				strcpy(executable, "\"");
				strcat(executable, c + 1);
				c = strchr(executable + 1, '"');
				if (!c) return(FALSE); //error
				*c = 0;
				strcat(executable, "\"");
				remove("__temp__");

				/* Bigmap screenshot */
				if (screenshot_height == MAX_WINDOW_HGT) strcpy(height, "835");
				/* Non-bigmap screenshot */
				else if (screenshot_height == DEFAULT_TERM_HGT) strcpy(height, "470");
				/* Custom size screenshot (new 2022) */
				else sprintf(height, "%d", 60 + screenshot_height * 17);

				sprintf(command, "%s --headless --window-size 660,%s --screenshot %s %s",
				    executable,
				    height,
				    file_name,
				    buf);

				fp = fopen("__temp__.bat", "w");
				fputs(command, fp);
				fclose(fp);
				k = system("__temp__.bat");
				remove("__temp__.bat");

				if (k) c_msg_format("Automatic PNG screenshot with 'Firefox' failed (%d).", k);
				else {
					c_msg_format("PNG screenshot via 'Firefox' saved to %s.", SCREENSHOT_TARGET);
					return(TRUE);
				}
			}
			/* failure */
			else c_msg_print("\377yFor PNG creation, either Chrome or Firefox must be installed!");
		}
	}
	return(FALSE);
#endif

#ifdef USE_X11
	/* Use chrome, chromium or firefox to create a png screenshot from xhtml
	   We prefer chrome/chromium since it allows setting background to transparent (or black) instead of white.
	   BIG_MAP: 640x750, normal map: 640x420.  - C. Blue */
	path_build(buf, 1024, ANGBAND_DIR_USER, screenshot_filename);

	/* Get full path of xhtml screenshot file (the browsers suck and won't work with a relative path) */
	k = system(format("readlink -f %s > __tmp__", buf));
	if (k) return(FALSE); //error
	if (!(fp = fopen("__tmp__", "r"))) return(FALSE); //error
	if (!fgets(buf, 1024, fp)) {
		fclose(fp);
		return(FALSE); //error
	}
	fclose(fp);
	buf[strlen(buf) - 1] = 0; //remove trailing newline
	strcat(buf, ".xhtml");

	/* Get relative path of target image file (suddenly the browsers are fine with it) */
	path_build(file_name, 1024, ANGBAND_DIR_USER, SCREENSHOT_TARGET);
	if (k) return(FALSE); //error

	/* Try chromium, then chrome, then firefox */
	k = system("chromium --version");
	if (!k) {
		/* Bigmap screenshot */
		if (screenshot_height == MAX_WINDOW_HGT) strcpy(height, "750");
		/* Non-bigmap screenshot */
		else if (screenshot_height == DEFAULT_TERM_HGT) strcpy(height, "420");
		/* Custom size screenshot (new 2022) */
		else sprintf(height, "%d", 60 + screenshot_height * 15);

		//bug with webdriver_manager.chrome: requires --disable-gpu to work with headless, or "ERROR:gpu_init.cc(426) Passthrough is not supported, GL is disabled" heppens.
		//--disable-software-rasterizer
		sprintf(command, "chromium --headless --disable-gpu --default-background-color=00000000 --window-size=640x%s --screenshot=%s file://%s",
		    height,
		    file_name,
		    buf);
		//c_msg_print(command);
		/* Randomly hangs, sometimes just works, wut */
		k = system(command);
		if (k) c_msg_format("PNG screenshot with 'chromium' failed (%d).", k);
		else {
			c_msg_format("PNG screenshot via 'chromium' saved to %s.", SCREENSHOT_TARGET);
			return(TRUE);
		}
	} else {
		k = system("chrome --version");
		if (!k) {
			/* Bigmap screenshot */
			if (screenshot_height == MAX_WINDOW_HGT) strcpy(height, "750");
			/* Non-bigmap screenshot */
			else if (screenshot_height == DEFAULT_TERM_HGT) strcpy(height, "420");
			/* Custom size screenshot (new 2022) */
			else sprintf(height, "%d", 60 + screenshot_height * 15);

			//bug with webdriver_manager.chrome: requires --disable-gpu to work with headless, or "ERROR:gpu_init.cc(426) Passthrough is not supported, GL is disabled" heppens.
			//--disable-software-rasterizer
			sprintf(command, "chrome --headless --disable-gpu --default-background-color=00000000 --window-size=640x%s --screenshot=%s file://%s",
			    height,
			    file_name,
			    buf);
			//c_msg_print(command);
			/* Untested */
			k = system(command);
			if (k) c_msg_format("PNG screenshot with 'chrome' failed (%d).", k);
			else {
				c_msg_format("PNG screenshot via 'chrome' saved to %s.", SCREENSHOT_TARGET);
				return(TRUE);
			}
		} else {
			k = system("firefox --version");
			if (!k) {
				/* Bigmap screenshot */
				if (screenshot_height == MAX_WINDOW_HGT) strcpy(height, "750");
				/* Non-bigmap screenshot */
				else if (screenshot_height == DEFAULT_TERM_HGT) strcpy(height, "420");
				/* Custom size screenshot (new 2022) */
				else sprintf(height, "%d", 60 + screenshot_height * 15);

				sprintf(command, "firefox --headless --window-size 640,%s --screenshot %s file://%s",
				    height,
				    file_name,
				    buf);
				//c_msg_print(command);
				k = system(command);
				if (k) c_msg_format("PNG screenshot with 'firefox' failed (%d).", k);
				else {
					c_msg_format("PNG screenshot via 'firefox' saved to %s.", SCREENSHOT_TARGET);
					return(TRUE);
				}
			}
			/* failure */
			else c_msg_print("\377yFor PNG creation, either Chromium, Chrome or Firefox must be installed!");
		}
	}
	return(FALSE);
#endif
}

/* Browse spoiler files, ie *_info.txt in TomeNET's lib/game/ folder. - C. Blue */
static void cmd_spoilers(void) {
	char i = 0;
	int row = 2, col = 10;

	Term_clear();
	topline_icky = TRUE;

	Term_putstr(col, row++, -1, TERM_WHITE, "(\377yk\377w) k_info.txt  - Items (aka objects)");
	Term_putstr(col, row++, -1, TERM_WHITE, "(\377ye\377w) e_info.txt  - Ego item powers");
	Term_putstr(col, row++, -1, TERM_WHITE, "(\377yr\377w) r_info.txt  - Monsters (races)");
	Term_putstr(col, row++, -1, TERM_WHITE, "(\377yE\377w) re_info.txt - Ego monster types (ego races)");
	Term_putstr(col, row++, -1, TERM_WHITE, "(\377ya\377w) a_info.txt  - Artifacts (predefined aka true ones)");
	Term_putstr(col, row++, -1, TERM_WHITE, "(\377yt\377w) tr_info.txt - Traps");
	Term_putstr(col, row++, -1, TERM_WHITE, "(\377yv\377w) v_info.txt  - Vaults");
	Term_putstr(col, row++, -1, TERM_WHITE, "(\377yf\377w) f_info.txt  - Floor tile types");
	Term_putstr(col, row++, -1, TERM_WHITE, "(\377yd\377w) d_info.txt  - Dungeons (predefined ones)");
	Term_putstr(col, row++, -1, TERM_WHITE, "(\377ys\377w) st_info.txt - Stores");

	while (i != ESCAPE) {
		Term_putstr(0,  0, -1, TERM_BLUE, "Browse spoiler files (in TomeNET's lib/game folder)");
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Hack to disable macros: Macros on SHIFT+X for example prohibits 'X' menu choice here.. */
		inkey_interact_macros = TRUE;
		i = inkey();
		/* ..and reenable macros right away again, so navigation via arrow keys works. */
		inkey_interact_macros = FALSE;

		switch (i) {
		case 'k':
			browse_local_file(ANGBAND_DIR_GAME, "k_info.txt", 1, FALSE);
			break;
		case 'e':
			browse_local_file(ANGBAND_DIR_GAME, "e_info.txt", 2, FALSE);
			break;
		case 'r':
			browse_local_file(ANGBAND_DIR_GAME, "r_info.txt", 3, FALSE);
			break;
		case 'E':
			browse_local_file(ANGBAND_DIR_GAME, "re_info.txt", 4, FALSE);
			break;
		case 'a':
			browse_local_file(ANGBAND_DIR_GAME, "a_info.txt", 5, FALSE);
			break;
		case 'v':
			browse_local_file(ANGBAND_DIR_GAME, "v_info.txt", 6, FALSE);
			break;
		case 'd':
			browse_local_file(ANGBAND_DIR_GAME, "d_info.txt", 7, FALSE);
			break;
		case 'f':
			browse_local_file(ANGBAND_DIR_GAME, "f_info.txt", 8, FALSE);
			break;
		case 't':
			browse_local_file(ANGBAND_DIR_GAME, "tr_info.txt", 9, FALSE);
			break;
		case 's':
			browse_local_file(ANGBAND_DIR_GAME, "st_info.txt", 10, FALSE);
			break;
		case KTRL('Q'):
			i = ESCAPE;
			/* fall through */
		case ESCAPE:
			break;
		case KTRL('T'):
			xhtml_screenshot("screenshot????", FALSE);
			break;
		case ':':
			cmd_message();
			break;
		default:
			bell();
		}
	}
}

#include <dirent.h> /* for cmd_notes() */
static void cmd_notes(void) {
	static int y = 0, j_sel = 0;
	int i = 0, d, vertikal_offset = 3, horiz_offset = 5;
	int row, col = 1;

	char ch = 0;
	bool inkey_msg_old;

	char notes_fname[1024][MAX_CHARS], path[1024];
	int notes_files = 0;
	char tmp_name[256];

	DIR *dir;
	struct dirent *ent;


	Term_clear();
	topline_icky = TRUE;

	/* read all locally available fonts */
	path_build(path, 1024, ANGBAND_DIR_USER, "");
	if (!(dir = opendir(path))) {
		c_msg_format("Couldn't open user directory (%s).", path);
		return;
	}

	while ((ent = readdir(dir))) {
		strcpy(tmp_name, ent->d_name);
		if (!strncmp(tmp_name, "notes-", 6)) {
			strcpy(notes_fname[notes_files], tmp_name);
			notes_files++;
		}
	}
	closedir(dir);

	/* suppress hybrid macros */
	inkey_msg_old = inkey_msg;
	inkey_msg = TRUE;

	while (ch != ESCAPE) {
		Term_putstr(0,  0, -1, TERM_BLUE, "Browse notes-*.txt files (in TomeNET's lib/user folder)");

		row = 1;
		if (!notes_files) Term_putstr(col, row++, -1, TERM_WHITE, format("You have not receveid any notes yet. (To write a note, use the \377y/note\377w command.)"));
		else {
			//Term_putstr(col, row++, -1, TERM_WHITE, format("Found %d notes-files from other players (Press \377yENTER\377w to browse selected):", notes_files));
			Term_putstr(col, row++, -1, TERM_WHITE, format("Found %d notes-files from other players:", notes_files));
#ifdef ENABLE_SHIFT_SPECIALKEYS
			Term_putstr(col, row++, -1, TERM_WHITE, "\377ydirectional keys\377w, \377ys\377w search, \377yESC\377w leave, \377BRETURN\377w reverse browse, \377BSHIFT+RET\377w browse");
#else
			Term_putstr(col, row++, -1, TERM_WHITE, "\377ydirectional keys\377w, \377ys\377w search, \377yESC\377w leave, \377BRETURN\377w reverse browse");
#endif
		}

		/* Display the notes */
		for (i = y - 10 ; i <= y + 10 ; i++) {
			if (i < 0 || i >= notes_files) {
				Term_putstr(horiz_offset + 5, vertikal_offset + i + 10 - y, -1, TERM_WHITE, "                                                          ");
				continue;
			}

			if (i == y) j_sel = i;

			Term_putstr(horiz_offset + 5, vertikal_offset + i + 10 - y, -1, TERM_WHITE, format("  %3d", i + 1));
			Term_putstr(horiz_offset + 12, vertikal_offset + i + 10 - y, -1, TERM_WHITE, "                                                   ");
			Term_putstr(horiz_offset + 12, vertikal_offset + i + 10 - y, -1, TERM_WHITE, notes_fname[i]);
		}

		/* display static selector */
		Term_putstr(horiz_offset + 1, vertikal_offset + 10, -1, TERM_SELECTOR, ">>>");
		Term_putstr(horiz_offset + 1 + 12 + 50 + 1, vertikal_offset + 10, -1, TERM_SELECTOR, "<<<");

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Hack to disable macros: Macros on SHIFT+X for example prohibits 'X' menu choice here.. */
#ifdef ENABLE_SHIFT_SPECIALKEYS
		inkey_shift_special = 0x0;
#endif
		ch = inkey();
		/* ..and reenable macros right away again, so navigation via arrow keys works. */

		switch (ch) {
		case '\n': case '\r':
#ifdef ENABLE_SHIFT_SPECIALKEYS
			browse_local_file(ANGBAND_DIR_USER, notes_fname[j_sel], 0, (inkey_shift_special == 0x1) ? FALSE : TRUE);
#else
			browse_local_file(ANGBAND_DIR_USER, notes_fname[j_sel], 0, TRUE);
#endif
			break;
		case KTRL('Q'):
			i = ESCAPE;
			/* fall through */
		case ESCAPE:
			break;
		case KTRL('T'):
			xhtml_screenshot("screenshot????", FALSE);
			break;
		case ':':
			cmd_message();
			break;
		case 's': /* Search for receipient name */
			{
			char searchstr[MAX_CHARS] = { 0 };

			Term_putstr(0, 0, -1, TERM_WHITE, "  Enter (partial) note receipient name: ");
			askfor_aux(searchstr, MAX_CHARS - 1, 0);
			if (!searchstr[0]) break;

			for (i = 0; i < notes_files; i++)
				if (my_strcasestr(notes_fname[i], searchstr)) break;
			if (i < notes_files) j_sel = y = i;
			break;
			}
		case '9':
		case 'p':
			y = (y - 10 + notes_files) % notes_files;
			break;
		case '3':
		case ' ':
			y = (y + 10 + notes_files) % notes_files;
			break;
		case '1':
			y = notes_files - 1;
			break;
		case '7':
			y = 0;
			break;
		case '8':
		case '2':
			d = keymap_dirs[ch & 0x7F];
			y = (y + ddy[d] + notes_files) % notes_files;
			break;
		case '\010':
			y = (y - 1 + notes_files) % notes_files;
			break;
		default:
			bell();
		}
	}

	inkey_msg = inkey_msg_old;
	Term_clear();
}

/*
 * NOTE: the usage of Send_special_line is quite a hack;
 * it sends a letter in place of cur_line...		- Jir -
 */
//(Linux file managers: Dolphin, Konqueror, Thunar, Caja, Nautilus, Nemo /// generic desktop environments: xdg-open)
#if defined(USE_X11)
 /* '&' for async - actually not needed on X11 though, program will still continue to execute
    because xdg-open spawns the file manager asynchronously and returns right away */
 #ifdef OSX
  #define FILEMAN(p) (res = system(format("open \"%s\" &", p)));
  #define URLMAN(p) (res = system(format("open \"%s\" &", p)));
 #else
  #define FILEMAN(p) (res = system(format("xdg-open \"%s\" &", p)));
  #define URLMAN(p) (res = system(format("xdg-open \"%s\" &", p)));
 #endif
#elif defined(WINDOWS)
 /* 'start' for async */
 #define FILEMAN(p) (res = system(format("start explorer \"%s\"", p)));
 //#define URLMAN(p) (res = system(format("cmd /c start \"\" \"%s\"", p)));

 /* According to The Scar in Firefox on Win 11 this does open the website but also causes it to start in 'diagnostics mode' (and was 100% fine in Win 7): */
 #define URLMAN(p) ShellExecute(NULL, "open", p, NULL, NULL, SW_SHOWNORMAL);
 /* ..and according to him this works fine - but it doesn't work in Wine actually -_- : */
 //#define URLMAN(p) (res = system(format("start \"%s\"", p)));
#endif
void cmd_check_misc(void) {
	char i = 0, choice;
	int row, res;
	/* suppress hybrid macros in some submenus */
	bool inkey_msg_old, uniques, redraw = TRUE;
#if defined(USE_X11) || defined(WINDOWS)
	char path[1024];
#endif
#ifdef USE_X11
	FILE *fp;
	char buf[MAX_CHARS];
#endif
#ifdef WINDOWS
 #include <winreg.h>	/* remote control of installed 7-zip via registry approach */
 #include <process.h>	/* use spawn() instead of normal system() (WINE bug/Win inconsistency even maybe) */
 #define MAX_KEY_LENGTH 255
 #define MAX_VALUE_NAME 16383
#endif

	Term_save();
	topline_icky = TRUE;

	while (i != ESCAPE) {
		if (redraw) {
			Term_clear();
			row = 1;
			redraw = FALSE;

			Term_putstr( 5, row + 0, -1, TERM_WHITE, "(\377y1\377w) Artifacts found");
			Term_putstr( 5, row + 1, -1, TERM_WHITE, "(\377y2\377w) Monsters killed/learnt");
			Term_putstr( 5, row + 2, -1, TERM_WHITE, "(\377y3\377w) Unique monsters");
			Term_putstr( 5, row + 3, -1, TERM_WHITE, "(\377y4\377w) Objects");
			Term_putstr( 5, row + 4, -1, TERM_WHITE, "(\377y5\377w) Traps");
			Term_putstr(40, row + 0, -1, TERM_WHITE, "(\377y6\377w) Artifact lore");
			Term_putstr(40, row + 1, -1, TERM_WHITE, "(\377y7\377w) Monster lore");
			Term_putstr(40, row + 2, -1, TERM_WHITE, "(\377y8\377w) Recall depths and towns");
			Term_putstr(40, row + 3, -1, TERM_WHITE, "(\377y9\377w) Houses");
			Term_putstr(40, row + 4, -1, TERM_WHITE, "(\377y0\377w) Wilderness map");
			row += 5;

			Term_putstr( 5, row + 0, -1, TERM_WHITE, "(\377ya\377w) Players online");
			Term_putstr( 5, row + 1, -1, TERM_WHITE, "(\377yb\377w) Other players' equipments");
			Term_putstr( 5, row + 2, -1, TERM_WHITE, "(\377yc\377w) High Scores");
			Term_putstr( 5, row + 3, -1, TERM_WHITE, "(\377yd\377w) Recent Deaths");
			Term_putstr( 5, row + 4, -1, TERM_WHITE, "(\377ye\377w) Lag-o-meter");
			Term_putstr(40, row + 0, -1, TERM_WHITE, "(\377yf\377w) News (Message of the day)");
			Term_putstr(40, row + 1, -1, TERM_WHITE, "(\377yh\377w) Intro screen");
			Term_putstr(40, row + 2, -1, TERM_WHITE, "(\377yi\377w) Server settings");
			Term_putstr(40, row + 3, -1, TERM_WHITE, "(\377yj\377w) Message history");
			Term_putstr(40, row + 4, -1, TERM_WHITE, "(\377yk\377w) Chat history");
			row += 5;

			Term_putstr( 5, row + 0, -1, TERM_WHITE, "(\377y?\377w) Help");
			Term_putstr( 5, row + 1, -1, TERM_WHITE, "(\377yg\377w) The Guide (also use \377y/?\377w)");
			Term_putstr( 5, row + 2, -1, TERM_WHITE, "(\377yx\377w) Extra info (same as \377y/ex\377w)");
			Term_putstr(40, row + 0, -1, TERM_WHITE, "(\377yl\377w) Spoiler files (in lib/game)");
			Term_putstr(40, row + 1, -1, TERM_WHITE, "(\377yn\377w) Notes you received from others");
			row += 4;

			/* Folders */
			Term_putstr( 5, row + 0, -1, TERM_WHITE, "(\377UT\377w) Open program folder");
			Term_putstr(40, row + 0, -1, TERM_WHITE, "(\377UU\377w) Open user folder");
			Term_putstr( 5, row + 1, -1, TERM_WHITE, "(\377US\377w) Open sound folder");
			Term_putstr(40, row + 1, -1, TERM_WHITE, "(\377UM\377w) Open music folder");
			Term_putstr( 5, row + 2, -1, TERM_WHITE, "(\377UX\377w) Open xtra folder (fonts/audio)");
			Term_putstr(40, row + 2, -1, TERM_WHITE, "(\377UP\377w) Open Player Stores page");
			/* URLs */
			row += 3;
			Term_putstr( 5, row + 0, -1, TERM_WHITE, "(\377UW\377w) Open TomeNET website");
			Term_putstr(40, row + 0, -1, TERM_WHITE, "(\377UR\377w) Open Mikael's monster search");
			Term_putstr( 5, row + 1, -1, TERM_WHITE, "(\377UG\377w) Open git repository site");
			Term_putstr(40, row + 1, -1, TERM_WHITE, "(\377UL\377w) Open oook.cz ladder site");
			row += 3;
			Term_putstr( 5, row, -1, TERM_WHITE, "(\377o~\377w) Convert last screenshot to");
			Term_putstr( 5, row + 1,   -1, TERM_WHITE, "    a PNG and leave this menu:");
			Term_putstr( 5, row + 2,   -1, TERM_WHITE, format("    %s", screenshot_filename[0] ? screenshot_filename : "- no screenshot taken -"));
			Term_putstr(40, row, -1, TERM_WHITE, "(\377oC\377w) Edit the current config file:");
#ifdef USE_X11
			Term_putstr(40, row + 1,   -1, TERM_WHITE, format("    %s", mangrc_filename));
			Term_putstr(40, row + 2, -1, TERM_WHITE, "    (Requires 'grep' to be installed.)");
#endif
#ifdef WINDOWS
			/* The ini file contains a long path (unlike mangrc_filename), so use two lines for it.. */
			if (strlen(ini_file) <= 35)
				Term_putstr(40, row + 1,   -1, TERM_WHITE, format("    %s", ini_file));
			else {
				/* Try to cut line at backslash */
				i = 35;
				while (i && ini_file[(int)i] != '\\') i--;
				if (i != 35) i++;
				/* ..but don't accept unreasonable lenght */
				if (i < 20) {
					/* Cut without regard, ouch */
					Term_putstr(40, row + 1,   -1, TERM_WHITE, format("    %-.35s", ini_file));
					Term_putstr(40, row + 2,   -1, TERM_WHITE, format("    %-.35s", ini_file + 35));
				} else {
					/* Cut nicely & cleanly at backslash */
					Term_putstr(40, row + 1,   -1, TERM_WHITE, format("    %-.*s", i, ini_file));
					Term_putstr(40, row + 2,   -1, TERM_WHITE, format("    %-.35s", ini_file + i));
				}
			}
#endif
		}

		Term_putstr(0,  0, -1, TERM_BLUE, "Display current knowledge");

		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Hack to disable macros: Macros on SHIFT+X for example prohibits 'X' menu choice here.. */
		inkey_interact_macros = TRUE; //Note: If this line isn't commented out, peruse_file() will need another '= FALSE' or macros won't work for navigation keys, because the perused file is received WHILE we are waiting in exactly THIS inkey() here ^^.
		i = inkey();
		/* ..and reenable macros right away again, so navigation via arrow keys works. */
		inkey_interact_macros = FALSE;

		choice = 0;
		switch (i) {
		case '3':
			if (is_atleast(&server_version, 4, 8, 1, 0, 0, 0)) {
				inkey_msg_old = inkey_msg;
				inkey_msg = TRUE;
				// TODO: "uniques_alive","List only unslain uniques for your local party" <- replaced by this choice now:
				//get_com("Which uniques? (ESC for all, SPACE for (party-)unslain, ! for bosses/top lv.)", &choice);
				get_com("Which uniques? (ESC for all, SPACE for alive, 'b' or 'e' for bosses)", &choice);
				inkey_msg = inkey_msg_old;
				if (choice == '!' || choice == 'b' || choice == 'e') choice = 2;
				else if (choice == ' ' || choice == 'a') choice = 1;
				else choice = 0;
				/* Encode 'choice' in 'line' info */
				Send_special_line(SPECIAL_FILE_UNIQUE, choice * 100000, "");
			} else {
				/* Send it */
				cmd_uniques();
			}
			break;
		case '1':
			cmd_artifacts();
			break;
		case '2':
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			get_com("What kind of monsters? (ESC for all, @ for learnt, SPACE for uniques):", &choice);
			inkey_msg = inkey_msg_old;
			if (choice <= ESCAPE) choice = 0;
			if (choice == ' ') {
				choice = 0;
				uniques = TRUE;
			} else uniques = FALSE;

			/* allow specifying minlev? */
			if (is_newer_than(&server_version, 4, 5, 4, 0, 0, 0)) {
				row = c_get_quantity("Specify minimum level? (ESC for none): ", 255);
				if (is_atleast(&server_version, 4, 7, 3, 0, 0, 0))
					Send_special_line(SPECIAL_FILE_MONSTER, choice + row * 100000 + (uniques ? 100000000 : 0), "");
				else
					Send_special_line(SPECIAL_FILE_MONSTER, choice + row * 100000, "");
			} else
				Send_special_line(SPECIAL_FILE_MONSTER, choice, "");
			break;
		case '4':
			inkey_msg_old = inkey_msg;
			inkey_msg = TRUE;
			get_com("What type of objects? (ESC for all):", &choice);
			inkey_msg = inkey_msg_old;
			if (choice <= ESCAPE) choice = 0;
			Send_special_line(SPECIAL_FILE_OBJECT, choice, "");
			break;
		case '5':
			Send_special_line(SPECIAL_FILE_TRAP, 0, "");
			break;
		case '9':
			Send_special_line(SPECIAL_FILE_HOUSE, 0, "");
			break;
		case '8':
			Send_special_line(SPECIAL_FILE_RECALL, 0, "");
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
			Send_special_line(SPECIAL_FILE_DEATHS, 0, "");
			break;
		case 'e':
			cmd_lagometer();
			break;
		case 'f':
			Send_special_line(SPECIAL_FILE_MOTD2, 0, "");
			break;
		case 'h':
			show_motd(0);
			break;
		case 'i':
			Send_special_line(SPECIAL_FILE_SERVER_SETTING, 0, "");
			break;
		case 'j':
			do_cmd_messages();
			break;
		case 'k':
			do_cmd_messages_important();
			break;
		case '?':
			cmd_help();
			break;
		case 'g':
			cmd_the_guide(0, 0, NULL);
			break;
		case 'l':
			cmd_spoilers();
			redraw = TRUE;
			break;
		case 'x':
			//Send_special_line(SPECIAL_FILE_EXTRAINFO, 0, ""); --not implemented
			Send_msg("/ex");
			break;
		case 'n':
			cmd_notes();
			redraw = TRUE;
			break;
		case KTRL('Q'):
			i = ESCAPE;
			/* fall through */
		case ESCAPE:
			break;
		case KTRL('T'):
			xhtml_screenshot("screenshot????", 2);
			/* Redraw new screenshot filename: */
			//Term_putstr( 5, row + 2,   -1, TERM_WHITE, format("    %s", screenshot_filename[0] ? screenshot_filename : "- no screenshot taken -"));
			redraw = TRUE;
			break;
		case ':':
			cmd_message();
			break;

#if defined(USE_X11) || defined(WINDOWS)
		case 'T': FILEMAN("."); break;
		case 'U': FILEMAN(ANGBAND_DIR_USER); break;
		case 'S':
			path_build(path, 1024, ANGBAND_DIR_XTRA, "sound");
			if (!check_dir2(path)) {
				c_message_add("\377yA folder 'sound' doesn't exist. Press 'X' instead to go to the 'xtra' folder.");
				break;
			}
			FILEMAN(path);
			break;
		case 'M':
			path_build(path, 1024, ANGBAND_DIR_XTRA, "music");
			if (!check_dir2(path)) {
				c_message_add("\377yA folder 'music' doesn't exist. Press 'X' instead to go to the 'xtra' folder.");
				break;
			}
			FILEMAN(path);
			break;
		case 'X':
			FILEMAN(ANGBAND_DIR_XTRA);
			break;
		case 'G':
			URLMAN("https://github.com/TomenetGame/");
			break;
		case 'W':
			/* Le quality de liferino~ */
			URLMAN(format("https://www.tomenet.eu/?mode=%s&server=%s",
			    (p_ptr->mode & MODE_EVERLASTING) ? "el" : "nel", server_name));
			break;
		case 'P':
			/* Le quality de liferino~ */
			URLMAN(format("https://www.tomenet.eu/pstores.php?mode=%s&server=%s",
			    (p_ptr->mode & MODE_EVERLASTING) ? "el" : "nel", server_name));
			break;
		case 'R':
			URLMAN("https://muuttuja.org/tomenet/monsters/index.php");
			break;
		case 'L': //ow http
			URLMAN("http://angband.oook.cz/ladder-browse.php?v=TomeNET"); //https fails, NET::ERR_CERT_COMMON_NAME_INVALID
			break;
#else
		/* USE_GCU (without USE_X11) and any other unknown OS.. */
		case 'T': case 'U': case 'S': case 'M': case 'X':
			c_message_add("Sorry, cannot open file manager in terminal-mode.");
			break;
		case 'G': case 'W': case 'P': case 'R': case 'L':
			c_message_add("Sorry, cannot open browser in terminal-mode.");
			break;
#endif

		//case 'I':
		case '~':
			if (png_screenshot()) i = ESCAPE; /* quit knowledge menu on success */
			break;
		case 'C':
#ifdef WINDOWS
			//FILEMAN(ini_file);
		    {
			/* check registry for default text editor: HKEY_CLASSES_ROOT\txtfile\shell\open\command */
			HKEY hTestKey;
			char regentry[1024], *c;
			LPBYTE regentry_p = (LPBYTE)regentry;
			int regentry_size = 1023;
			LPDWORD regentry_size_p = (LPDWORD)&regentry_size;
			unsigned long regentry_type = REG_SZ;

			if (RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("txtfile\\shell\\open\\command\\"), 0, KEY_READ, &hTestKey) == ERROR_SUCCESS) {
				if (RegQueryValueEx(hTestKey, "", NULL, &regentry_type, regentry_p, regentry_size_p) == ERROR_SUCCESS) {
					regentry[regentry_size] = '\0';
				} else {
					// odd case
					RegCloseKey(hTestKey);
					Term_putstr(0, 1, -1, TERM_YELLOW, "Registry key for default text editor not found.");
				}
				/* Cut off "%1" parameter at the end, if any */
				if ((c = strchr(regentry, '%'))) *c = 0;
 #if 0 /* pops up a terminal window */
				system(format("start %s %s", regentry, ini_file));
 #else /* ..hopefully doesn't */
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));

				CreateProcess( NULL, format("%s %s", regentry, ini_file),
				    NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
 #endif
			} else {
				c_message_add("\377w(Couldn't find default app for opening text files, falling back to notepad.)");
				system(format("start notepad %s", ini_file));
			}
		    }
#endif
#ifdef USE_X11
		    {
			//system(format("xdg-open %s &", mangrc_filename));
			//FILEMAN(mangrc_filename);
			//system("cat /usr/share/applications/`xdg-mime query default text/plain` | grep -o 'Exec.*' | head -n 1 | grep -o '=.*' | grep -o '[0-9a-z]*' > __tmp__");
			int r = system("cat /usr/share/applications/`xdg-mime query default text/plain` | grep -o 'Exec.*' | grep -o '=.*' | grep -o '[0-9a-z]*' > __tmp__");
			char *c;

			fp = fopen("__tmp__", "r");
			if (fp) {
				c = fgets(buf, MAX_CHARS, fp);
				//todo maybe: error handling if xdg-mime fails
				fclose(fp);
				buf[strlen(buf) - 1] = 0;
				r = system(format("%s %s &", buf, mangrc_filename));
			}
			(void)r;
			(void)c;
		    }
#endif
			break;

		default:
			bell();
		}
	}

	topline_icky = FALSE;
	Term_load();
	(void)res;
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
	bool big_shop = (screen_hgt == MAX_SCREEN_HGT);//14 more lines, so alphabet is fully used a)..z)

	/* Wipe the whole buffer to stop valgrind from complaining about the color code conversion - mikaelh */
	C_WIPE(buf, sizeof(buf), char);

	inkey_msg = TRUE;

	if (get_string("Message: ", buf, sizeof(buf) - 1)) {
		/* hack - screenshot - mikaelh */
		if (prefix(buf, "/shot") || prefix(buf, "/screenshot")) {
			char *space = strchr(buf, ' ');

			if (space && space[1]) {
				/* Use given parameter as filename */
				xhtml_screenshot(space + 1, FALSE);
			} else {
				/* Default filename pattern */
				xhtml_screenshot("screenshot????", FALSE);
			}
			inkey_msg = FALSE;
			return;
		}

		/* Convert {'s into \377's */
		for (i = 0; i < sizeof(buf); i++) {
			if (buf[i] == '{') {
				buf[i] = '\377';

				/* remember last colour; for item pasting below */
				//c = buf[i + 1];
			}
		}

		/* Allow to paste items in chat via shortcut symbol - C. Blue */
		for (i = 0; i < strlen(buf); i++) {
			/* paste inven/equip item:  \\<slot>  */
			if ((i == 0 || buf[i - 1] != '\\') && /* don't confuse with '\\\' store pasting */
			    buf[i] == '\\' &&
			    buf[i + 1] == '\\' &&
			    ((buf[i + 2] >= 'a' && buf[i + 2] < 'a' + INVEN_PACK) || /* paste inventory item */
			    (buf[i + 2] >= 'A' && buf[i + 2] < 'A' + (INVEN_TOTAL - INVEN_WIELD)) || /* paste equipment item */
			    buf[i + 2] == '_')) { /* paste floor item, or rather: what's under your feet */
				if (buf[i + 2] == '_') strcpy(item, whats_under_your_feet);
				else if (buf[i + 2] >= 'a') strcpy(item, inventory_name[buf[i + 2] - 'a']);
				else strcpy(item, inventory_name[(buf[i + 2] - 'A') + INVEN_WIELD]);

				/* prevent buffer overflow */
				if (strlen(buf) - 3 + strlen(item) + 2 + 1 + 1 < MSG_LEN) {
					strcpy(tmp, &buf[i + 3]);
					strcpy(&buf[i], "\377s");

					/* if item inscriptions contains a colon we might need
					   another colon to prevent confusing it with a private message */
					strcat(buf, item);
					if (strchr(item, ':')) {
						buf[strchr(item, ':') - item + strlen(buf) - strlen(item) + 1] = '\0';
						if (strchr(buf, ':') == &buf[strlen(buf) - 1])
							strcat(buf, ":");
						strcat(buf, strchr(item, ':') + 1);
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
		/* Handle local slash command - C. Blue */
		else if (!strcasecmp(buf, "/ctime")) {
			time_t ct = time(NULL);
			struct tm* ctl = localtime(&ct);

			c_msg_format("\374\376\377yYour current local time: %04d/%02d/%02d - %02d:%02d:%02dh", 1900 + ctl->tm_year, ctl->tm_mon + 1, ctl->tm_mday, ctl->tm_hour, ctl->tm_min, ctl->tm_sec);
			inkey_msg = FALSE;
			return;
		} else if (!strcasecmp(buf, "/cver") || !strcasecmp(buf, "/cversion")) {
			c_msg_format("Client version: %d.%d.%d.%d.%d.%d%s, OS %d.", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_EXTRA, VERSION_BRANCH, VERSION_BUILD, CLIENT_VERSION_TAG, VERSION_OS);
			inkey_msg = FALSE;
			return;
		} else if (!strcasecmp(buf, "/apickup")) {
			c_cfg.auto_pickup = !c_cfg.auto_pickup;
			c_msg_format("Auto-pickup mode is %s.", c_cfg.auto_pickup ? "on" : "off");
			inkey_msg = FALSE;
			return;
		} else if (!strcasecmp(buf, "/adestroy")) {
			c_cfg.auto_destroy = !c_cfg.auto_destroy;
			c_msg_format("Auto-destroy mode is %s.", c_cfg.auto_destroy ? "on" : "off");
			inkey_msg = FALSE;
			return;
		} else if (!strcasecmp(buf, "/daunmatched")) {
			c_cfg.destroy_all_unmatched = !c_cfg.destroy_all_unmatched;
			c_msg_format("Destroy-all-unmatched mode (requires auto_destroy) is %s.", c_cfg.destroy_all_unmatched ? "on" : "off");
			inkey_msg = FALSE;
			return;
		} else if (!strncasecmp(buf, "/opty", 5) || !strncasecmp(buf, "/optvy", 6)) {
			bool redundant = FALSE, verbose;
			int offset;

			if (!strncasecmp(buf, "/opty", 5)) {
				verbose = FALSE;
				offset = 6;
				if (buf[offset - 1] != ' ' || !buf[offset]) {
					c_msg_print("Usage: /opty <option name>");
					inkey_msg = FALSE;
					return;
				}
			} else {
				verbose = TRUE;
				offset = 7;
				if (buf[offset - 1] != ' ' || !buf[offset]) {
					c_msg_print("Usage: /optvy <option name>");
					inkey_msg = FALSE;
					return;
				}
			}

			/* Hack for backward compatibility: Allow specifying 'big_map' although it's no longer a valid option but instead in = menu! */
			if (!strcmp(buf + offset, "big_map")) {
				set_bigmap(1, verbose);
				inkey_msg = FALSE;
				return;
			}

			for (i = 0; i < OPT_MAX; i++) {
				if (!option_info[i].o_desc) continue;
				if (strcmp(buf + offset, option_info[i].o_text)) continue;

				if (*option_info[i].o_var) redundant = TRUE;
				else {
					options_immediate(TRUE);
					*option_info[i].o_var = TRUE;
					Client_setup.options[i] = TRUE;
					options_immediate(FALSE);
				}
				break;
			}
			if (i == OPT_MAX) c_msg_format("Option '%s' does not exist.", buf + offset);
			else if (redundant) {
				if (verbose) c_msg_format("Option '%s' is already enabled.", buf + offset);
			} else {
				if (verbose) c_msg_format("Option '%s' has been enabled.", buf + offset);
				check_immediate_options(i, *option_info[i].o_var, TRUE);
				Send_options();
			}
			inkey_msg = FALSE;
			return;
		} else if (!strncasecmp(buf, "/optn", 5) || !strncasecmp(buf, "/optvn", 6)) {
			bool redundant = FALSE, verbose;
			int offset;

			if (!strncasecmp(buf, "/optn", 5)) {
				verbose = FALSE;
				offset = 6;
				if (buf[offset - 1] != ' ' || !buf[offset]) {
					c_msg_print("Usage: /optn <option name>");
					inkey_msg = FALSE;
					return;
				}
			} else {
				verbose = TRUE;
				offset = 7;
				if (buf[offset - 1] != ' ' || !buf[offset]) {
					c_msg_print("Usage: /optvn <option name>");
					inkey_msg = FALSE;
					return;
				}
			}

			/* Hack for backward compatibility: Allow specifying 'big_map' although it's no longer a valid option but instead in = menu! */
			if (!strcmp(buf + offset, "big_map")) {
				set_bigmap(0, verbose);
				inkey_msg = FALSE;
				return;
			}

			for (i = 0; i < OPT_MAX; i++) {
				if (!option_info[i].o_desc) continue;
				if (strcmp(buf + offset, option_info[i].o_text)) continue;

				if (!*option_info[i].o_var) redundant = TRUE;
				else {
					options_immediate(TRUE);
					*option_info[i].o_var = FALSE;
					Client_setup.options[i] = FALSE;
					options_immediate(FALSE);
				}
				break;
			}
			if (i == OPT_MAX) c_msg_format("Option '%s' does not exist.", buf + offset);
			else if (redundant) {
				if (verbose) c_msg_format("Option '%s' is already disabled.", buf + offset);
			} else {
				if (verbose) c_msg_format("Option '%s' has been disabled.", buf + offset);
				check_immediate_options(i, *option_info[i].o_var, TRUE);
				Send_options();
			}
			inkey_msg = FALSE;
			return;
		} else if (!strncasecmp(buf, "/optt", 5) || !strncasecmp(buf, "/optvt", 6)) {
			bool verbose;
			int offset;

			if (!strncasecmp(buf, "/optt", 5)) {
				verbose = FALSE;
				offset = 6;
				if (buf[offset - 1] != ' ' || !buf[offset]) {
					c_msg_print("Usage: /optt <option name>");
					inkey_msg = FALSE;
					return;
				}
			} else {
				verbose = TRUE;
				offset = 7;
				if (buf[offset - 1] != ' ' || !buf[offset]) {
					c_msg_print("Usage: /optt <option name>");
					inkey_msg = FALSE;
					return;
				}
			}

			/* Hack for backward compatibility: Allow specifying 'big_map' although it's no longer a valid option but instead in = menu! */
			if (!strcmp(buf + offset, "big_map")) {
				set_bigmap(-1, verbose);
				inkey_msg = FALSE;
				return;
			}

			for (i = 0; i < OPT_MAX; i++) {
				if (!option_info[i].o_desc) continue;
				if (strcmp(buf + offset, option_info[i].o_text)) continue;
				options_immediate(TRUE);
				*option_info[i].o_var = !(*option_info[i].o_var);
				Client_setup.options[i] = *option_info[i].o_var;
				options_immediate(FALSE);
				break;
			}
			if (i == OPT_MAX) c_msg_format("Option '%s' does not exist.", buf + offset);
			else {
				if (verbose) c_msg_format("Option '%s' has been toggled (%s).", buf + offset, *option_info[i].o_var ? "enabled" : "disabled");
				check_immediate_options(i, *option_info[i].o_var, TRUE);
				Send_options();
			}
			inkey_msg = FALSE;
			return;
		} else if (!strcasecmp(buf, "/know")) { /* Someone claimed that he doesn't have a '~' key ">_>.. */
			cmd_check_misc();
			inkey_msg = FALSE;
			return;
		} else if (!strcasecmp(buf, "/png")) { /* Someone claimed that he doesn't have a '~' key ">_>.. */
			(void)png_screenshot();
			inkey_msg = FALSE;
			return;
#ifdef GUIDE_BOOKMARKS
		} else if (prefix(buf, "/? ") && strlen(buf) == 4 && buf[3] >= 'a' && buf[3] <= 't') { /* Quick bookmark access */
			cmd_the_guide(3, 0, format("%c", buf[3]));
			inkey_msg = FALSE;
			return;
#endif
		} else if (!strcasecmp(buf, "/reinit_guide")) { /* Undocumented: Re-init the guide (update buffered guide info if guide was changed while client is still running). */
			init_guide();
			c_msg_format("Guide reinitialized. (errno %d,lastline %d,endofcontents %d,chapters %d)", guide_errno, guide_lastline, guide_endofcontents, guide_chapters);
			inkey_msg = FALSE;
			return;
		} else if (!strcasecmp(buf, "/reinit_audio")) { /* Undocumented: Re-init the audio system. Same as pressing CTRL+R in the mixer UI. */
			if (re_init_sound() == 0) c_message_add("Audio packs have been reloaded and audio been reinitialized successfully.");
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
				Term_putstr(5, 15, -1, TERM_WHITE, "(\377U1\377w) Toggle 'enable-adders' flag");
				Term_putstr(5, 16, -1, TERM_WHITE, "(\377U2\377w) Toggle 'auto-re-add' flag");
				Term_putstr(5, 17, -1, TERM_WHITE, "(\377U3\377w) Set minimum level required to join the guild");
				Term_putstr(5, 18, -1, TERM_WHITE, "(\377Ua\377w) View list of 'adders' who may add players in your stead");
				Term_putstr(5, 19, -1, TERM_WHITE, "(\377Ub\377w) Add/remove 'adders' who may add players in your stead");
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
			xhtml_screenshot("screenshot????", 2);

		else if (i == ':') {
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
		}

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
		} else if (guild_master && i == 'a') {
			Send_msg("/xguild_adders");
		} else if (guild_master && i == 'b') {
			strcpy(buf0, "");
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
			Term_putstr(5, 12, -1, TERM_WHITE, "(\377Ue\377w) Set/view guild options and adders");

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
			xhtml_screenshot("screenshot????", 2);

		else if (i == ':') {
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
		}

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

	if (!c_get_item(&item, "Throw what? ", (USE_INVEN | USE_EQUIP | USE_EXTRA))) return;

	if (!get_dir(&dir)) return;

	/* Send it */
	Send_throw(item, dir);
}

static bool item_tester_browsable(object_type *o_ptr) {
	return(is_realm_book(o_ptr) || o_ptr->tval == TV_BOOK
#ifdef ENABLE_SUBINVEN
	    || o_ptr->tval == TV_SUBINVEN
#endif
	    );
}

static void cmd_browse_aux_restore(void) {
	/* For unknown reason the top line text from show_subinven() remained on screen even after Term_load() */
	clear_topline();

	Term_save();
	showing_inven = screen_icky;
	command_gap = 50;
	show_inven();
}
/* Normal usage: If item is -1 the user gets prompted.
   'item' is for usage with cmd_all_in_one(). */
void cmd_browse(int item) {
	object_type *o_ptr;
	bool from_inven = showing_inven;

	/* For some reason, cmd_browse() always shows the list when called form within inventory screen,
	   so we need this here in front instead of after the c_get_item() prompt below.. */
	/* Hack: We were called form inventory? */
	if (from_inven) {
		/* Leave inventory screen first */
		screen_line_icky = -1;
		screen_column_icky = -1;
		showing_inven = FALSE;
		Term_load();
		Flush_queue();
	}

	/* commented out because first, admins are usually ghosts;
	   second, we might want a 'ghost' tome or something later,
	   kind of to bring back 'undead powers',
	   as 'ghost powers' though since the regular vampire race is undead too, pft. :) - C. Blue */
#if 0
	if (p_ptr->ghost) {
		show_browse(NULL);
		return;
	}
#endif

	if (item == -1) {
#ifdef ENABLE_SUBINVEN
		get_item_hook_find_obj_what = "Book/container name? ";
		get_item_extra_hook = get_item_hook_find_obj;

		item_tester_hook = item_tester_browsable;

		if (!c_get_item(&item, "Browse which book/container? ", (USE_INVEN |
		    USE_EQUIP | /* for WIELD_BOOKS */
		    USE_EXTRA | NO_FAIL_MSG))) {
			if (item == -2) c_msg_print("You have no books that you can read, nor containers to peruse.");
			return;
		}
#else
		get_item_hook_find_obj_what = "Book name? ";
		get_item_extra_hook = get_item_hook_find_obj;

		item_tester_hook = item_tester_browsable;

		if (!c_get_item(&item, "Browse which book? ", (USE_INVEN |
		    USE_EQUIP | /* for WIELD_BOOKS */
		    USE_EXTRA | NO_FAIL_MSG))) {
			if (item == -2) c_msg_print("You have no books that you can read.");
			return;
		}
#endif
	}

	o_ptr = &inventory[item];

	if (o_ptr->tval != TV_BOOK
#ifdef ENABLE_SUBINVEN
	    && o_ptr->tval != TV_SUBINVEN
#endif
	    ) {
#ifdef ENABLE_SUBINVEN
		c_msg_print("That is not a book to browse, nor a container to peruse.");
#else
		c_msg_print("That is not a book to browse.");
#endif
		return;
	}


#ifdef ENABLE_SUBINVEN
	if (o_ptr->tval == TV_SUBINVEN) {
		cmd_subinven(item);
		if (from_inven) cmd_browse_aux_restore();
		return;
	}
#endif
	if (o_ptr->tval == TV_BOOK) {
		browse_school_spell(item, o_ptr->sval, o_ptr->pval);
		if (from_inven) cmd_browse_aux_restore();
		return;
	}

	/* Show it */
	show_browse(o_ptr);
	if (from_inven) cmd_browse_aux_restore();
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
	char buf[1024];
	FILE *fp;
	char *buf2;
	byte fmt;
	int n, err;

	buf[0] = '\0';
	if (!get_string("Load pref: ", buf, 1023)) return;

	fp = my_fopen(buf, "r");
	if (!fp) {
		c_message_add(format("\377yCould not open file %s", buf));
		return;
	}

	/* Process the file */
	while (0 == (err = my_fgets2(fp, &buf2, &n, &fmt))) {
		/* Process the line */
		if (process_pref_file_aux(buf2, fmt)) //printf("Error in '%s' parsing '%s'.\n", buf2, buf);
			c_message_add(format("\377yError in '%s' parsing '%s'.\n", buf2, buf));
		mem_free(buf2);
	}
	if (err == 2) {
		printf("Grave error: Couldn't allocate memory when parsing '%s'.\n", buf);
		//plog(format("!!! GRAVE ERROR: Couldn't allocate memory when parsing file '%s' !!!\n", name));
		/* Maybe this is safer than plog(), if the player is in the dungeon and in a dire situation when this happens.. */
		c_message_add(format("\377y!!! GRAVE ERROR: Couldn't allocate memory when parsing file '%s' !!!\n", buf));
	}
	my_fclose(fp);

	/* Success */
	c_message_add(format("Loaded file %s", buf));
	return;
}

void cmd_redraw(void) {
	Send_redraw(0);

	/* Reset static vars for hp/sp/mp for drawing huge bars to enforce redrawing */
	prev_huge_cmp = prev_huge_csn = prev_huge_chp = -1;

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
			xhtml_screenshot("screenshot????", 2);
			break;
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
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

static void cmd_house_tag(int dir) {
	char buf[80];

	get_string("Enter a tag for the houses list (max 19 characters): ", &buf[1], 20 - 1);
	buf[0] = 'T';
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
	Term_putstr(5, 7, -1, TERM_WHITE, "(4) Paint house");/* new in 4.4.6 */
	Term_putstr(5, 8, -1, TERM_WHITE, "(5) Add houses-list tag");/* new in 4.7.3.1 */
	Term_putstr(5, 10, -1, TERM_WHITE, "(s) Enter player store");/* new in 4.4.6 */
	Term_putstr(5, 11, -1, TERM_WHITE, "(k) Knock on house door");
	/* display in dark colour since only admins can do this really */
	if (p_ptr->admin_dm || p_ptr->admin_wiz)
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
		case '5':
			cmd_house_tag(dir);
			i = ESCAPE;
			break;
		case 'D':
			if (!p_ptr->admin_dm && !p_ptr->admin_wiz) bell();
			else {
				cmd_house_kill(dir);
				i = ESCAPE;
			}
			break;
		case 's':
			cmd_house_store(dir);
			/* Note that Term_load() at the end of this function, followed shortly after by
			   Term_save() in display_store(), leads to that quick visual flickering -_- no
			   good way to skip those two though, since we don't know whether house_admin()
			   will fail on server-side..  - C. Blue */
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
			xhtml_screenshot("screenshot????", 2);
			break;
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
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
#ifdef DM_MODULES
		Term_putstr(5, 9, -1, TERM_WHITE, "(6) Save level to module file");
		Term_putstr(5, 10, -1, TERM_WHITE, "(7) Load level from module file");
		Term_putstr(5, 11, -1, TERM_WHITE, "(8) Generate a blank level");
		Term_putstr(5, 12, -1, TERM_WHITE, "(9) Set entry position");

		/* Prompt */
		Term_putstr(0, 14, -1, TERM_WHITE, "Command: ");
#else
		Term_putstr(0, 10, -1, TERM_WHITE, "Command: ");
#endif

		/* Get a key */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		/* Take a screenshot */
		else if (i == KTRL('T')) xhtml_screenshot("screenshot????", 2);
		else if (i == ':') {
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
		}
		/* static the current level */
		else if (i == '1') Send_master(MASTER_LEVEL, "s");
		/* unstatic the current level */
		else if (i == '2') Send_master(MASTER_LEVEL, "u");
		else if (i == '3') {	/* create dungeon stair here */
			buf[0] = 'D';
			buf[4] = 0x01;//hack: avoid 0 byte
			buf[5] = 0x01;//hack: avoid 0 byte
			buf[6] = 0x01;//hack: avoid 0 byte
			if (is_newer_than(&server_version, 4, 5, 6, 0, 0, 1)) {
				char ts[4];
				int t; //hooray for signed char..

				strcpy(ts, "0");
				if (!get_string("Theme (ESC/0 = default vanilla, +100 to set type instead): ", ts, 3)) t = 0;
				else t = atoi(ts);

				if (t >= 100) {
					buf[7] = 100 - t;
					/* Predefined dungeon -> nothing left to do then. */
					buf[1] = 1;
					buf[2] = 127;
					buf[3] = 'd';
					buf[8] = '\0';
					Send_master(MASTER_LEVEL, buf);
					clear_topline_forced();
					return;
				} else {
					if (is_atleast(&server_version, 4, 9, 0, 5, 0, 0)) buf[7] = t + 1; //hack: avoid 0 byte
					else buf[7] = t; //max len in this version is 8 anyway, so no info is lost even if this is 0.
				}
			} else buf[7] = 0;
			buf[1] = c_get_quantity("Base level: ", 127);
			if (!buf[1]) buf[1] = 1; //pressed ESC? Apply a default value (or dungeon creation will fail)
			buf[2] = c_get_quantity("Max depth (1-127): ", 127);
			if (!buf[2]) buf[2] = 3; //pressed ESC? Apply a default value (or dungeon creation will fail)
			buf[3] = (get_check2("Is it a tower?", FALSE) ? 't' : 'd');
			/*
			 * FIXME: flags are u32b while buf[] is char!
			 * This *REALLY* should be rewritten	- Jir -
			 */
			//--removed as currently ALL dungeons are RANDOM (or panic save occurs due to undefined dungeon size!):
			// if (get_check2("Random dungeon (default)?", TRUE)) buf[5] |= 0x02;//DF2_RANDOM
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
			/* Allow any custom flags */
			if (is_atleast(&server_version, 4, 9, 0, 5, 0, 0)) {
				char fshex[9], fshextmp[9];

				fshextmp[0] = 0;
				(void)get_string("Custom DF1 flags (string of 8 hex chars, logical OR): ", fshextmp, 8);
				memset(fshex, '0', 8);
				strcpy(fshex + 8 - strlen(fshextmp), fshextmp);
				strncpy(buf + 8, fshex, 8);

				fshextmp[0] = 0;
				(void)get_string("Custom DF2 flags (string of 8 hex chars, logical OR): ", fshextmp, 8);
				memset(fshex, '0', 8);
				strcpy(fshex + 8 - strlen(fshextmp), fshextmp);
				strncpy(buf + 16, fshex, 8);

				fshextmp[0] = 0;
				(void)get_string("Custom DF3 flags (string of 8 hex chars, logical OR): ", fshextmp, 8);
				memset(fshex, '0', 8);
				strcpy(fshex + 8 - strlen(fshextmp), fshextmp);
				strncpy(buf + 24, fshex, 8);

				buf[32] = '\0'; /* Terminate */
			} else buf[8] = '\0'; /* Terminate */
			Send_master(MASTER_LEVEL, buf);
		}
		else if (i == '4') {
			buf[0] = 'R';
			buf[1] = '\0';
			Send_master(MASTER_LEVEL, buf);
		}
		else if (i == '5') {
			buf[0] = 'T';
			buf[1] = c_get_quantity("Base level: ", 127);
			Send_master(MASTER_LEVEL, buf);
		}

#ifdef DM_MODULES
		/* Kurzel - save/load a module file (or create a blank to begin with) */
		else if (i == '6') {
			buf[0] = 'S';
			get_string("Save module name (max 19 char):", &buf[1], 19);
			Send_master(MASTER_LEVEL, buf);
		}
		else if (i == '7') {
			buf[0] = 'L';
			get_string("Load module name (max 19 char):", &buf[1], 19);
			Send_master(MASTER_LEVEL, buf);
		}
		else if (i == '8') {
			buf[0] = 'B';
			get_string("WxH string (eg. 1x1-5x5):", &buf[1], 19);
			Send_master(MASTER_LEVEL, buf);
		}
		else if (i == '9') {
			get_string("Set level entry (> < or +):", &buf[0], 1);
			Send_master(MASTER_LEVEL, buf);
		}
#endif

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
		if (i == KTRL('T')) xhtml_screenshot("screenshot????", 2);
		else if (i == ':') {
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
		}
		/* Generate by number */
		else if (i == '1') {
			buf[1] = '#';
			buf[2] = c_get_quantity("Vault number? ", 255) - 127;
			if (!buf[2]) redo_hack = 1;
			buf[3] = 0;
		}
		/* Generate by name */
		else if (i == '2') {
			buf[1] = 'n';
			get_string("Enter vault name: ", &buf[2], 77);
			if (!buf[2]) redo_hack = 1;
		}
		/* Oops */
		else {
			/* Ring bell */
			bell();
			redo_hack = 1;
		}

		/* hack -- don't do this if we hit an invalid key previously */
		if (redo_hack) continue;

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
		else if (i == KTRL('T')) xhtml_screenshot("screenshot????", 2);
		/* Chat */
		else if (i == ':') {
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
		}
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
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
			break;
		case KTRL('T'):
			xhtml_screenshot("screenshot????", 2);
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
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
			break;
		case KTRL('T'):
			xhtml_screenshot("screenshot????", 2);
			break;
		case '1': strcpy(buf, "Snaga"); break;
		case '2': strcpy(buf, "Cave orc"); break;
		case '3': strcpy(buf, "Hill orc"); break;
		case '4': strcpy(buf, "Black orc"); break;
		case '5': strcpy(buf, "Half-orc"); break;
		case '6': strcpy(buf, "Uruk"); break;
		case '7': strcpy(buf, "random"); break;
		default : bell(); break;
		}

		/* if we got an orc type, return it */
		if (buf[0]) return(buf);

		/* Flush messages */
		clear_topline_forced();
	}

	/* escape was pressed, no valid orcs types specified */
	return(NULL);
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
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
			break;
		case KTRL('T'):
			xhtml_screenshot("screenshot????", 2);
			break;
		case '1': strcpy(buf, "Poltergeist"); break;
		case '2': strcpy(buf, "Green glutton ghost"); break;
		case '3': strcpy(buf, "Loust soul"); break;
		case '4': strcpy(buf, "Skeleton kobold"); break;
		case '5': strcpy(buf, "Skeleton orc"); break;
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
		if (buf[0]) return(buf);

		/* Flush messages */
		clear_topline_forced();
	}

	/* escape was pressed, no valid types specified */
	return(NULL);
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
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
			break;
		case KTRL('T'):
			xhtml_screenshot("screenshot????", 2);
			break;
		case '1': strcpy(buf, "Vampire"); break;
		case '2': strcpy(buf, "Giant skeleton troll"); break;
		case '3': strcpy(buf, "Lich"); break;
		case '4': strcpy(buf, "Master vampire"); break;
		case '5': strcpy(buf, "Dread"); break;
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
		if (buf[0]) return(buf);

		/* Flush messages */
		clear_topline_forced();
	}

	/* escape was pressed, no valid orcs types specified */
	return(NULL);
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
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
			break;
		/* Take a screenshot */
		case KTRL('T'):
			xhtml_screenshot("screenshot????", 2);
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
			case ':':
				cmd_message();
				inkey_msg = TRUE; /* And suppress macros again.. */
				break;
			case KTRL('T'):
				xhtml_screenshot("screenshot????", 2);
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
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
			break;
		case KTRL('T'):
			xhtml_screenshot("screenshot????", 2);
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
		case '8': {
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
	if (is_newer_than(&server_version, 4, 6, 1, 1, 0, 1))
		chunksize = 1024;
	else
		chunksize = 256;

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
		Term_putstr(5, 7, -1, TERM_WHITE, "(e) Execute script command");
		Term_putstr(5, 8, -1, TERM_WHITE, "(u) Upload script file");
		Term_putstr(5, 9, -1, TERM_WHITE, "(c) Execute local script command");

		Term_putstr(0, 12, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();
		switch (i) {
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
			break;
		case KTRL('T'):
			xhtml_screenshot("screenshot????", 2);
			break;
		case '1':
			Send_special_line(SPECIAL_FILE_LOG, 0, "");
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
#ifdef TEST_CLIENT
		Term_putstr(5,10, -1, TERM_SLATE, "(a) Test 1");
		Term_putstr(5,11, -1, TERM_SLATE, "(b) Test 2");
		Term_putstr(5,12, -1, TERM_SLATE, "(c) Test 3");
#endif

		/* Prompt */
		Term_putstr(0, 14, -1, TERM_WHITE, "Command: ");

		/* Get a key */
		i = inkey();

		switch (i) {
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
			break;
		case KTRL('T'):
			xhtml_screenshot("screenshot????", 2);
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

#ifdef TEST_CLIENT
		/* Extra: Variable random stuff that gets hard-coded into the client for hacking.. */
		case 'a':
			c_message_add(format("sizeof(object_type)=%d", sizeof(object_type)));
			break;
		case 'b':
			c_message_add(format("admin_dm=%d", p_ptr->admin_dm));
			c_message_add(format("admin_wiz=%d", p_ptr->admin_wiz));
			c_message_add(format("total_winner=%d", p_ptr->total_winner));
			c_message_add(format("ghost=%d", p_ptr->ghost));
			break;
		case 'c':
			break;
#endif

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
		case ':':
			cmd_message();
			inkey_msg = TRUE; /* And suppress macros again.. */
			break;
		/* Take a screenshot */
		case KTRL('T'):
			xhtml_screenshot("screenshot????", 2);
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
