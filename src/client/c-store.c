/* $Id$ */
/*
 * Client-side store stuff.
 */

#include "angband.h"

bool leave_store;
int store_top = 0;



/*
 * ToME show_building, ripped for client	- Jir -
 */
void display_store_action() {
	int i;
	/* BIG_MAP leads to big shops */
	int spacer = (screen_hgt > SCREEN_HGT) ? 14 : 0;

	for (i = 0; i < 6; i++) {
		if (!c_store.actions[i]) continue;

		c_put_str(c_store.action_attr[i], c_store.action_name[i],
				21 + spacer + (i / 2), 20 + (30 * (i % 2)));
	}
}

static void display_entry(int pos, int entries) {
	object_type *o_ptr;
	int i, x;
	char o_name[ONAME_LEN];
	char out_val[MSG_LEN];

	int maxwid = 75;
	int wgt;

	/* Get the item */
	o_ptr = &store.stock[pos];
	wgt = o_ptr->weight;

	/* Get the "offset" */
	i = (pos % entries);

	/* Label it, clear the line --(-- */
	if (wgt >= 0 || store_num == STORE_HOME || store_num == STORE_HOME_DUN) { /* <0: player store hack */
		(void)sprintf(out_val, "%c) ", I2A(i));
		prt(out_val, i + 6, 0);
	}

	/* Describe an item in the home */
	if (store_num == STORE_HOME || store_num == STORE_HOME_DUN) {
		maxwid = 75;

		/* Leave room for weights, if necessary -DRS- */
		if (c_cfg.show_weights) maxwid -= 10;

		/* Describe the object */
		strcpy(o_name, store_names[pos]);
		o_name[maxwid] = '\0';
		c_put_str(o_ptr->attr, o_name, i + 6, 3);

		/* Show weights */
		if (c_cfg.show_weights) {
			/* Only show the weight of an individual item */
			int wgt = o_ptr->weight;
			(void)sprintf(out_val, "%3d.%d lb", wgt / 10, wgt % 10);
			put_str(out_val, i + 6, 68);
		}
	} else {
		//wgt<0: pstore 'sign' hack
		if (wgt < 0) {
			maxwid = 70;

			/* Describe the object (fully) */
			strcpy(o_name, store_names[pos]);

			o_name[maxwid] = '\0';
			c_put_str(o_ptr->attr, o_name, i + 6, 0);
		} else {
			/* Must leave room for the "price" */
			maxwid = 65;

			/* Leave room for weights, if necessary -DRS- */
			if (c_cfg.show_weights) maxwid -= 7;

			/* Describe the object (fully) */
			strcpy(o_name, store_names[pos]);

			o_name[maxwid] = '\0';
			c_put_str(o_ptr->attr, o_name, i + 6, 3);

			/* Show weights */
			if (c_cfg.show_weights) {
				/* Only show the weight of an individual item */
				(void)sprintf(out_val, "%3d.%d", wgt / 10, wgt % 10);
				put_str(out_val, i + 6, 61);
			}

			x = store_prices[pos];
			if (x >= 0) { /* <0: player store hack */
				/* Actually draw the price (not fixed) */
				(void)sprintf(out_val, "%9d  ", x);
				c_put_str(p_ptr->au < x ? TERM_L_DARK : TERM_WHITE, out_val, i + 6, 68);
			}
		}
	}
}



void display_inventory(void) {
	int i, k, entries = 12;

	/* BIG_MAP leads to big shops */
	if (screen_hgt > SCREEN_HGT) entries = 26; /* we don't have 34 letters in the alphabet =p */

	for (k = 0; k < entries; k++) {
		/* Do not display "dead" items */
		if (store_top + k >= store.stock_num) break;

		/* Display that one */
		display_entry(store_top + k, entries);
	}

	/* Erase the extra lines and the "more" prompt */
	for (i = k; i <= entries; i++) prt("", i + 6, 0);

	/* Assume "no current page" */
	put_str("             ", 5, 20);

	/* Visual reminder of "more items" */
	if (store.stock_num > entries) {
		/* Show "more" reminder (after the last item) */
		prt("-more-", k + 6, 3);

		/* Indicate the "current page" */
		put_str(format("(Page %d of %d)%s%s",
		    store_top / entries + 1, store.stock_num == 0 ? 1 : (store.stock_num + entries - 1) / entries,
		    store_top / entries + 1 >= 10 ? "" : " ", (store.stock_num + entries - 1) / entries >= 10 ? "" : " "),
		    5, 20);
	}

	/* Hack - Get rid of the cursor - mikaelh */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;
}

/*
 * Get the ID of a store item and return its value      -RAK-
 */
static int get_stock(int *com_val, cptr pmt, int i, int j) {
	char    command;
	char    out_val[160];

	/* Paranoia XXX XXX XXX */
	clear_topline_forced();

	/* Assume failure */
	*com_val = (-1);

	/* Build the prompt */
	(void)sprintf(out_val, "(Items %c-%c, ESC to exit) %s", I2A(i), I2A(j), pmt);

	/* Ask until done */
	while (TRUE) {
	        int k;

		/* Escape */
		if (!get_com(out_val, &command)) break;

		/* Convert */
		k = (islower(command) ? A2I(command) : -1);

		/* Legal responses */
		if ((k >= i) && (k <= j)) {
			*com_val = k;
			break;
		}

		/* Oops */
		bell();
	}

	/* Clear the prompt */
	clear_topline();

	/* Cancel */
	if (command == ESCAPE) return (FALSE);

	/* Success */
	return (TRUE);
}




/* XXX Bad design.. store code really should be rewritten.	- Jir - */
static void store_examine(void) {
	int                     i;
	int                     item;

	object_type             *o_ptr;
	char            out_val[160];

	/* BIG_MAP leads to big shops */
	int entries = (screen_hgt > SCREEN_HGT) ? 26 : 12;


	/* Empty? */
	if (store.stock_num <= 0) {
		if (store_num == STORE_HOME || store_num == STORE_HOME_DUN)
			c_msg_print("Your home is empty.");
		else c_msg_print("I am currently out of stock.");
		return;
	}

	/* Find the number of objects on this and following pages */
	i = (store.stock_num - store_top);

	/* And then restrict it to the current page */
	if (i > entries) i = entries;

	/* Prompt */
	if (store_num == STORE_HOME || store_num == STORE_HOME_DUN)
		sprintf(out_val, "Which item do you want to examine? ");
	else
		sprintf(out_val, "Which item do you want to examine? ");

	/* Get the item number to be bought */
	if (!get_stock(&item, out_val, 0, i-1)) return;

	/* Get the actual index */
	item = item + store_top;

	/* Get the actual item */
	o_ptr = &store.stock[item];

	/* Tell the server */
	if (is_realm_book(o_ptr)) show_browse(o_ptr);
	else if (o_ptr->tval == TV_BOOK &&
	    (!is_custom_tome(o_ptr->sval) || o_ptr->xtra1))
		browse_school_spell(-(item + 1), o_ptr->sval, o_ptr->pval);
	else Send_store_examine(item);
}


static void store_purchase(bool one) {
	int i, amt, amt_afford;
	int item;

	object_type *o_ptr;
	char out_val[160];

	/* BIG_MAP leads to big shops */
	int entries = (screen_hgt > SCREEN_HGT) ? 26 : 12;


	/* Empty? */
	if (store.stock_num <= 0) {
		if (store_num == STORE_HOME || store_num == STORE_HOME_DUN)
			c_msg_print("Your home is empty.");
		else c_msg_print("I am currently out of stock.");
		return;
	}


	/* Find the number of objects on this and following pages */
	i = (store.stock_num - store_top);

	/* And then restrict it to the current page */
	if (i > entries) i = entries;

	/* Prompt */
	if (store_num == STORE_HOME || store_num == STORE_HOME_DUN)
		sprintf(out_val, "Which item do you want to take? ");
	else
		sprintf(out_val, "Which item are you interested in? ");

	/* Get the item number to be bought */
	if (!get_stock(&item, out_val, 0, i-1)) return;

	/* Get the actual index */
	item = item + store_top;

	/* Get the actual item */
	o_ptr = &store.stock[item];

	/* Client-side check already, for item stacks: Too expensive? */
	if (store_prices[item] > p_ptr->au) {
		c_msg_print("You do not have enough gold.");
		return;
	}

	/* Assume the player wants just one of them */
	amt = 1;

	/* Find out how many the player wants */
	if (o_ptr->number > 1 && !one) {
		/* Hack -- note cost of "fixed" items */
		if (store_num != STORE_HOME || store_num == STORE_HOME_DUN) {
			c_msg_print(format("That costs %d gold per item.", store_prices[item]));

			if (store_prices[item] > 0) amt_afford = p_ptr->au / store_prices[item];
			else amt_afford = o_ptr->number;

			/* Get a quantity */
			if (o_ptr->number <= amt_afford)
				amt = c_get_quantity(NULL, o_ptr->number);
			else if (amt_afford > 1) {
				inkey_letter_all = TRUE;
				sprintf(out_val, "Quantity (1-\377y%d\377w, 'a' or spacebar for all): ", amt_afford);
				amt = c_get_quantity(out_val, amt_afford);
			} else {
				sprintf(out_val, "Quantity (\377y1\377w): ");
				amt = c_get_quantity(out_val, 1);
			}
		} else {
			/* Get a quantity */
			amt = c_get_quantity(NULL, o_ptr->number);
		}

		/* Allow user abort */
		if (amt <= 0) return;
	}

	/* Tell the server */
	Send_store_purchase(item, amt);
}

/* helper functions for store_chat(), also to be used for
   pasting store items in the middle of a chat line similar to
   new '\\x' feature for inven/equip-pasting. - C. Blue */
void store_paste_where(char *where) {
	snprintf(where, 17, "\377s(%d,%d) \377%c%c\377s:",
	    p_ptr->wpos.wx, p_ptr->wpos.wy,
	    color_attr_to_char(c_store.store_attr),
	    c_store.store_char);
}
void store_paste_item(char *out_val, int item) {
	char	price[16];

	/* Get shop price if any */
	/* Home doesn't price items */
	if (store_num == STORE_HOME || store_num == STORE_HOME_DUN) {
		sprintf(out_val, " %s", store_names[item]);
	/* Player stores with '@S-' inscription neither (museum mode) */
	} else if (store_prices[item] < 0) {
		sprintf(out_val, " %s", store_names[item]);
	/* Normal [player-]store */
	} else {
		/* Convert the price to more readable format */
		if (store_prices[item] >= 10000000)
			snprintf(price, 16, "%dM", store_prices[item] / 1000000);
		else if (store_prices[item] >= 10000)
			snprintf(price, 16, "%dk", store_prices[item] / 1000);
		else
			snprintf(price, 16, "%d", store_prices[item]);

		sprintf(out_val, "%s (%s Au)", store_names[item], price);
	}
}

static void store_chat(void) {
	int	i, item;

	char	out_val[MSG_LEN], buf[MSG_LEN];
	char	where[17];

	/* BIG_MAP leads to big shops */
	int entries = (screen_hgt > SCREEN_HGT) ? 26 : 12;


	/* Empty? */
	if (store.stock_num <= 0) {
		if (store_num == STORE_HOME || store_num == STORE_HOME_DUN)
			c_msg_print("Your home is empty.");
		else c_msg_print("I am currently out of stock.");
		return;
	}


	/* Find the number of objects on this and following pages */
	i = (store.stock_num - store_top);

	/* And then restrict it to the current page */
	if (i > entries) i = entries;

	/* Prompt */
	if (store_num == STORE_HOME || store_num == STORE_HOME_DUN)
		sprintf(out_val, "Which item? ");
	else
		sprintf(out_val, "Which item? ");

	/* Get the item number to be pasted */
	if (!get_stock(&item, out_val, 0, i - 1)) return;

	/* Get the actual index */
	item = item + store_top;

	store_paste_where(where);
	store_paste_item(out_val, item);

	/* Tell the server */
	snprintf(buf, MSG_LEN - 1, "%s %s", where, out_val);
	buf[MSG_LEN - 1] = 0;
	Send_paste_msg(buf);
}

static void store_sell(void) {
	int item, amt;

	if (store_num == STORE_HOME || store_num == STORE_HOME_DUN) {
		if (!c_get_item(&item, "Drop what? ", (USE_EQUIP | USE_INVEN)))
			return;
	} else if (store_num == STORE_MATHOM_HOUSE) {
		if (!c_get_item(&item, "Donate what? ", (USE_EQUIP | USE_INVEN)))
			return;
	} else {
		if (!c_get_item(&item, "Sell what? ", (USE_EQUIP | USE_INVEN)))
			return;
	}

	/* Get an amount */
	if (inventory[item].number > 1) {
		if (is_cheap_misc(inventory[item].tval) && c_cfg.whole_ammo_stack && !verified_item) amt = inventory[item].number;
		else {
			inkey_letter_all = TRUE;
			amt = c_get_quantity("How many ('a' or spacebar for all)? ", inventory[item].number);
		}
	} else amt = 1;

	/* Hack -- verify for Museum(Mathom house) */
	if (store_num == STORE_MATHOM_HOUSE) {
		char out_val[160];

		if (inventory[item].number == amt)
			sprintf(out_val, "Really donate %s?", inventory_name[item]);
		else
			sprintf(out_val, "Really donate %d of your %s?", amt, inventory_name[item]);
		if (!get_check2(out_val, FALSE)) return;
	}

	/* Tell the server */
	Send_store_sell(item, amt);
}

static void store_do_command(int num, bool one) {
	int                     i, amt, gold;
	int                     item, item2;
	u16b action = c_store.actions[num];
	u16b bact = c_store.bact[num];
	/* BIG_MAP leads to big shops */
	int entries = (screen_hgt > SCREEN_HGT) ? 26 : 12;

	char            out_val[160];

	i = amt = gold = item = item2 = 0;

	/* lazy job for 'older' commands */
	switch (bact) {
	case BACT_SELL:
		store_sell();
		return;
		break;
	case BACT_BUY:
		store_purchase(one);
		return;
		break;
	case BACT_EXAMINE:
		store_examine();
		return;
		break;
	}

	if (c_store.flags[num] & BACT_F_STORE_ITEM) {
		/* Empty? */
		if (store.stock_num <= 0) {
			if (store_num == STORE_HOME || store_num == STORE_HOME_DUN)
				c_msg_print("Your home is empty.");
			else c_msg_print("I am currently out of stock.");
			return;
		}


		/* Find the number of objects on this and following pages */
		i = (store.stock_num - store_top);

		/* And then restrict it to the current page */
		if (i > entries) i = entries;

		/* Prompt */
		sprintf(out_val, "Which item? ");

		/* Get the item number to be bought */
		if (!get_stock(&item, out_val, 0, i - 1)) return;

		/* Get the actual index */
		item = item + store_top;
	}

	if (c_store.flags[num] & BACT_F_INVENTORY) {
		if (!c_get_item(&item, "Which item? ", (USE_EQUIP | USE_INVEN)))
			return;
	}

	if (c_store.flags[num] & BACT_F_GOLD) {
		/* Get how much */
		inkey_letter_all = TRUE;
		gold = c_get_quantity("How much gold ('a' or spacebar for all)? ", -1);

		/* Send it */
		if (!gold) return;
	}

	if (c_store.flags[num] & BACT_F_HARDCODE) {
		/* Nothing for now */
	}

	Send_store_command(action, item, item2, amt, gold);
}

static void store_process_command(int cmd) {
	int i;
	bool allow_w_t = TRUE, allow_k = TRUE;

	/* BIG_MAP leads to big shops */
	int entries = (screen_hgt > SCREEN_HGT) ? 26 : 12;

	for (i = 0; i < 6; i++) {
		if (!c_store.actions[i]) continue;

		if (c_store.letter[i] == cmd) {
			store_do_command(i, FALSE);
			return;
		}

		/* only allow wear/wield and take-off in stores that do not
		   already provide special functions of their own on those keys */
		if (c_cfg.rogue_like_commands) {
			if (c_store.letter[i] == 'w' ||
			    //c_store.letter[i] == KTRL('W') ||
			    c_store.letter[i] == 'T') allow_w_t = FALSE;
			//if (c_store.letter[i] == KTRL('W')) allow_k = FALSE;
		} else {
			if (c_store.letter[i] == 'w' ||
			    c_store.letter[i] == 'W' ||
			    c_store.letter[i] == 't') allow_w_t = FALSE;
			if (c_store.letter[i] == 'k') allow_k = FALSE;
		}
	}

	/* hack: translate p/s into d/g and vice versa, for extra comfort */
	for (i = 0; i < 6; i++) {
		if (!c_store.actions[i]) continue;

		switch (c_store.letter[i]) {
		case 'p':
			if (cmd == 'g') {
				store_do_command(i, FALSE);
				return;
			}
			if (cmd == KTRL('G') && !c_cfg.rogue_like_commands) {
				store_do_command(i, TRUE);
				return;
			}
			if (cmd == KTRL('Z') && c_cfg.rogue_like_commands) {
				store_do_command(i, TRUE);
				return;
			}
			break;
		case 's':
			if (cmd == 'd') {
				store_do_command(i, FALSE);
				return;
			}
			break;
		case 'g':
			if (cmd == 'p') {
				store_do_command(i, FALSE);
				return;
			}
			if (cmd == KTRL('G') && !c_cfg.rogue_like_commands) {
				store_do_command(i, TRUE);
				return;
			}
			if (cmd == KTRL('Z') && c_cfg.rogue_like_commands) {
				store_do_command(i, TRUE);
				return;
			}
			break;
		case 'd':
			if (cmd == 's') {
				store_do_command(i, FALSE);
				return;
			}
			break;
		case 'x':
#if 0 /* nope, change of plans for future changes: use shift+I to inspect _own_ items instead! */
		//(future change: replace x by I in ba_info, and I/s by D) -- case 'I':
			if (cmd == 'I' || cmd == 'l' || cmd == 'x') {
				store_do_command(i, FALSE);
				return;
			}
#else
			if (cmd == 'l' || cmd == 'x') {
				store_do_command(i, FALSE);
				return;
			}
#endif
			break;
		}
	}

	/* Parse the command */
	i = 0; /* for jumping to page 1/2/3/4 */
	switch (cmd) {
		/* Leave */
		case ESCAPE:
		case KTRL('Q'):
			leave_store = TRUE;
			break;

		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????");
			break;

		/* Browse */
		case ' ':
			if (store.stock_num <= entries) {
				if (store_top) {
					/*
					 * Hack - Allowing going back to first page after buying
					 * last item on the second page. - mikaelh */
					store_top = 0;
					display_inventory();
				} else {
					c_msg_print("Entire inventory is shown.");
				}
			} else {
				store_top += entries;
				if (store_top >= store.stock_num) store_top = 0;
				display_inventory();
			}
			break;

		/* Allow to browse backwards via BACKSPACE */
		case '\b':
			if (store.stock_num <= entries) {
				if (store_top) {
					/* see above - C. Blue */
					store_top = 0;
					display_inventory();
				} else {
					c_msg_print("Entire inventory is shown.");
				}
			} else {
				store_top -= entries;
				if (store_top < 0) store_top = ((store.stock_num - 1) / entries) * entries;
				display_inventory();
			}
			break;

		/* go to page n */
		case '0': i++;
		case '9': i++;
		case '8': i++;
		case '7': i++;
		case '6': i++;
		case '5': i++;
		case '4': i++;
		case '3': i++;
		case '2': i++;
		case '1':
			if (store.stock_num > entries * i) {
				store_top = entries * i;
				display_inventory();
			}
#if 0 /* suppress message, in case STORE_INVEN_MAX doesn't actually support all 10 pages. It'd look silyl. */
			else c_msg_format("Page %d is empty.", i + 1);
#endif
			break;

		/* Paste on Chat */
		case 'c':
			store_chat();
			break;

		/* allow to actually chat from within a store, might be cool for
		   notifying others of items other than just pasting into chat */
		case ':':
			cmd_message();
			break;

		/* allow inspecting _own_ items while in a store! */
		case 'I':
			cmd_observe(USE_INVEN | USE_EQUIP);
			break;

		/* Ignore return */
		case '\r':
			break;

		/* Equipment list */
		case 'e':
			cmd_equip();
			break;

		/* Inventory list */
		case 'i':
			cmd_inven();
			break;

#ifdef USE_SOUND_2010
		case KTRL('C'):
		case KTRL('X')://rl
			toggle_music();
			break;
		case KTRL('N'):
		case KTRL('V')://rl
			toggle_audio();
			break;
#endif

		case '{':
			cmd_inscribe(USE_INVEN | USE_EQUIP);
			break;
		case '}':
			cmd_uninscribe(USE_INVEN | USE_EQUIP);
			break;

		/* special feat for some stores: allow wear/wield and take-off..
		   'emulating' rogue-like key mapping option for now, sigh */
		case 'w':
			if (allow_w_t) cmd_wield();
			else cmd_raw_key(cmd);
			break;
		case 'W':
			if (c_cfg.rogue_like_commands || !allow_w_t) cmd_raw_key(cmd);
			else cmd_wield2();
			break;
		case KTRL('W'):
			if (!c_cfg.rogue_like_commands || !allow_w_t) cmd_raw_key(cmd);
			else cmd_wield2();
			break;
		case 't':
			if (c_cfg.rogue_like_commands || !allow_w_t) cmd_raw_key(cmd);
			else cmd_take_off();
			break;
		case 'T':
			if (!c_cfg.rogue_like_commands || !allow_w_t) cmd_raw_key(cmd);
			else cmd_take_off();
			break;
		case 'k':
			if (c_cfg.rogue_like_commands || !allow_k) cmd_raw_key(cmd);
			else cmd_destroy(USE_INVEN | USE_EQUIP);
			break;
		case KTRL('D'):
			if (!c_cfg.rogue_like_commands || !allow_k) cmd_raw_key(cmd);
			else cmd_destroy(USE_INVEN | USE_EQUIP);
			break;

		default:
			cmd_raw_key(cmd);
			break;
	}
}

void c_store_prt_gold(void) {
	char out_val[64];
	/* BIG_MAP leads to big shops */
	int spacer = (screen_hgt > SCREEN_HGT) ? 14 : 0;

	prt("Gold Remaining: ", 19 + spacer, 53);

	sprintf(out_val, "%9d", p_ptr->au);
	if (p_ptr->au < 1000000000) strcat(out_val, " "); //hack to correctly clear line for players moving huge amounts
	prt(out_val, 19 + spacer, 68);

	/* Hack -- show balance (if not 0) */
	if (store_num == STORE_MERCHANTS_GUILD && p_ptr->balance) {
		prt("Your balance  : ", 20 + spacer, 53);

		sprintf(out_val, "%9d", p_ptr->balance);
		if (p_ptr->au < 1000000000) strcat(out_val, " ");
		prt(out_val, 20 + spacer, 68);
	} else {
		/* Erase part of the screen */
		Term_erase(0, 20 + spacer, 255);
	}
}

void do_redraw_store(void) {
	if (perusing) return; /* don't overwrite inspect-item screen when home inventory updates (due to an item timing out) */

	redraw_store = FALSE;

	if (shopping) {
		/* hack: Display changed capacity if we just extended the house */
		if ((store_num == STORE_HOME || store_num == STORE_HOME_DUN)
		    && c_store.max_cost) {
			char buf[1024];
			sprintf(buf, "%s (Stock: %d/%d)", c_store.store_name, store.stock_num, c_store.max_cost);
			prt(buf, 3, 50);
		}
		display_inventory();
	}
}

void display_store(void) {
	int i;
	char buf[1024];
	/* BIG_MAP leads to big shops */
	int spacer = (screen_hgt > SCREEN_HGT) ? 14 : 0;

	/* Save the term */
	Term_save();

	/* We are "shopping" */
	shopping = TRUE;

	/* Clear screen */
	Term_clear();

	store_top = 0;

	/* The "Home" is special */
	if (store_num == STORE_HOME || store_num == STORE_HOME_DUN) {
		/* Put the owner name (usually "Your House/Home") */
		sprintf(buf, "%s", c_store.owner_name);
		put_str(buf, 3, 10);

		/* Show the store name (usually blank for houses) and -hack- max_cost is the capacity */
		if (c_store.max_cost)
			sprintf(buf, "%s (Stock: %d/%d)", c_store.store_name, store.stock_num, c_store.max_cost);
		else
			sprintf(buf, "%s", c_store.store_name);
		prt(buf, 3, 50);

		/* Label the item descriptions */
		put_str("Item Description", 5, 3);

		/* If showing weights, show label */
		if (c_cfg.show_weights) put_str("Weight", 5, 70);
	}

	/* Normal stores */
	else {
		/* Put the owner name and race */
		sprintf(buf, "%s", c_store.owner_name);
		put_str(buf, 3, 10);

		/* Show the max price in the store (above prices) */
		if (!c_store.max_cost) {
			/* improve it a bit visually for player stores who don't buy anything */
			sprintf(buf, "%s", c_store.store_name);
		} else {
			sprintf(buf, "%s (%d)", c_store.store_name, c_store.max_cost);
		}
		prt(buf, 3, 50);

		/* Label the item descriptions */
		put_str("Item Description", 5, 3);

		/* If showing weights, show label */
		if (c_cfg.show_weights) put_str("Weight", 5, 60);

		/* Label the asking price (in stores) */
		put_str("Price", 5, 72);

		/* Display the players remaining gold */
		c_store_prt_gold();
	}

	/* Display the inventory */
	display_inventory();

	/* Don't leave */
	leave_store = FALSE;

	/* Start at the top */
	store_top = 0;

	/* Interact with player */
	while (!leave_store) {
		/* Hack -- Clear line 1 */
		prt("", 1, 0);

		/* Clear */
		clear_from(21 + spacer);

		/* Prompt */
#if 0
		prt("You may: ", 21 + spacer, 0);

		/* Basic commands */
		prt(" ESC) Exit.", 22 + spacer, 0);

		/* Browse if necessary */
		if (store.stock_num > 12 + spacer) prt(" SPACE) Next page", 23 + spacer, 0);
#else /* new: also show 1..n for jumping directly to a page */
		prt("ESC)   Exit store", 21 + spacer, 0);
		if (store.stock_num > 12 + spacer) {
			prt("SPACE) Next page", 22 + spacer, 0);
			prt(format("1-%d)%s  Go to page", (store.stock_num - 1) / (12 + spacer) + 1, (store.stock_num - 1) / (12 + spacer) + 1 >= 10 ? "" : " "), 23 + spacer, 0);
		}
#endif

#if 0
		/* Home commands */
		if ((store_num == STORE_HOME || store_num == STORE_HOME_DUN) && FALSE) {
			prt(" g) Get an item.", 22 + spacer, 30);
			prt(" d) Drop an item.", 23 + spacer, 30);

			prt(" x) eXamine an item.", 22 + spacer, 60);
		}
		/* Shop commands XXX XXX XXX */
		else
#endif
			display_store_action();

		/* Hack - Get rid of the cursor - mikaelh */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		i = inkey();

		if (i) {
			/* Process the command */
			store_process_command(i);
		}
	}

	/* Tell the server that we're outta here */
	Send_store_leave();

	/* Clear the screen */
	Term_clear();

	/* We are no longer "shopping" */
	shopping = FALSE;
	store.stock_num = 0;

	/* Flush any events that happened */
	Flush_queue();

	/* reload the term */
	Term_load();
}

/* For the new SPECIAL flag for non-inven stores - C. Blue */
void display_store_special(void) {
	int i;
	char buf[1024];
	/* BIG_MAP leads to big shops */
	int spacer = (screen_hgt > SCREEN_HGT) ? 14 : 0;

	/* Save the term */
	Term_save();

	/* We are "shopping" */
	shopping = TRUE;

	/* Clear screen */
	Term_clear();

	/* Put the owner name and race */
	sprintf(buf, "%s", c_store.owner_name);
	put_str(buf, 3, 10);

	sprintf(buf, "%s", c_store.store_name);
	prt(buf, 3, 50);

	/* Display the players remaining gold */
	c_store_prt_gold();

	/* Don't leave */
	leave_store = FALSE;

	/* Start at the top */
	store_top = 0;

	/* Interact with player */
	while (!leave_store) {
		/* Hack -- Clear line 1 */
		prt("", 1, 0);

		/* Clear */
		clear_from(21 + spacer);

		/* Prompt */
#if 0 /* keep consistent with display_store() */
		prt("You may: ", 21 + spacer, 0);

		/* Basic commands */
		prt(" ESC) Exit.", 22 + spacer, 0);
#else
		prt("ESC)   Exit store", 21 + spacer, 0);
#endif

#if 0
		/* Browse if necessary */
		if (store.stock_num > 12 + spacer) prt(" SPACE) Next page", 23 + spacer, 0);
#endif

		/* Shop commands XXX XXX XXX */
		display_store_action();

		/* Hack - Get rid of the cursor - mikaelh */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		i = inkey();

		if (i) {
			/* Process the command */
			store_process_command(i);
		}
	}

	/* Tell the server that we're outta here */
	Send_store_leave();

	/* Clear the screen */
	Term_clear();

	/* We are no longer "shopping" */
	shopping = FALSE;
	store.stock_num = 0;

	/* Flush any events that happened */
	Flush_queue();

	/* reload the term */
	Term_load();
}
