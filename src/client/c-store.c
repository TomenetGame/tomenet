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
	int i, y = 2, y2 = 0;

	/* BIG_MAP leads to big shops */
	int spacer = (screen_hgt == MAX_SCREEN_HGT) ? 14 : 0;

	for (i = 0; i < MAX_STORE_ACTIONS; i++) {
		//actually erase any remaining store action strings from previous store we entered. This could really use cleaning up.
		if (i < 8) c_put_str(c_store.action_attr[i], "                          ", y + y2 + 18 + spacer + (i / 2), 20 + (30 * (i % 2)));
		else c_put_str(c_store.action_attr[i], "              ", y + y2 + 18 + spacer + (i / 2), 20 + (30 * (i % 2)));

		if (!c_store.actions[i]) continue;

		if (i < 8) c_put_str(c_store.action_attr[i], c_store.action_name[i], y + y2 + 18 + spacer + (i / 2), 20 + (30 * (i % 2)));
		else c_put_str(c_store.action_attr[i], c_store.action_name[i], y + y2 + 18 + spacer + 11 - i, 2); //only enough space for short names for actions 9+ (especially no space for pricings!)
	}
}

static void display_entry(int pos, int entries) {
	object_type *o_ptr;
	int i, y = 2;
	s32b prc;
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
		prt(out_val, i + y + 3, 0);
	} else prt("", i + y + 3, 0); /* Just clear the line (for pieces of wood used as labels) */

	/* Describe an item in the home */
	if (store_num == STORE_HOME || store_num == STORE_HOME_DUN) {
		maxwid = 75;

		/* Leave room for weights, if necessary -DRS- */
		if (c_cfg.show_weights) maxwid -= 10;

		/* Describe the object */
		strcpy(o_name, store_names[pos]);
		o_name[maxwid] = '\0';
		c_put_str(o_ptr->attr, o_name, i + y + 3, 3);

		/* Show weights */
		if (c_cfg.show_weights) {
			/* Only show the weight of an individual item */
			int wgt = o_ptr->weight;

			(void)sprintf(out_val, "%3d.%d lb", wgt / 10, wgt % 10);
			put_str(out_val, i + y + 3, 68);
		}
	} else {
		//wgt<0: pstore 'sign' hack
		if (wgt < 0) {
			maxwid = 70;

			/* Describe the object (fully) */
			strcpy(o_name, store_names[pos]);

			o_name[maxwid] = '\0';
			c_put_str(o_ptr->attr, o_name, i + y + 3, 0);
		} else {
			/* Must leave room for the "price" */
			maxwid = 65;

			/* Leave room for weights, if necessary -DRS- */
			if (c_cfg.show_weights) maxwid -= 7;

			/* Describe the object (fully) */
			strcpy(o_name, store_names[pos]);

			o_name[maxwid] = '\0';
			c_put_str(o_ptr->attr, o_name, i + y + 3, 3);

			/* Show weights */
			if (c_cfg.show_weights) {
				/* Only show the weight of an individual item */
				(void)sprintf(out_val, "%3d.%d", wgt / 10, wgt % 10);
				put_str(out_val, i + y + 3, 61);
			}

			prc = store_prices[pos];
			if (prc >= 0) { /* <0: player store hack and mathom-house hack */
				/* Actually draw the price (not fixed) */
				if (!c_cfg.colourize_bignum) {
					/* Normal display of the price */
					(void)sprintf(out_val, "%9d  ", prc);
				} else {
					if (p_ptr->au >= prc && prc >= 0) colour_bignum(prc, -1, out_val, 0, TRUE);
					//else (void)sprintf(out_val, "%9d  ", x); //will be coloured uniformly dark aka unaffordable
					else colour_bignum(prc, -1, out_val, 0, FALSE);
				}
				c_put_str(p_ptr->au < prc ? TERM_L_DARK : TERM_WHITE, out_val, i + y + 3, 68);
			} else if (prc == -3) { /* @SB inscription and sold out: */
				(void)sprintf(out_val, "\377D SOLD OUT  ");
				c_put_str(TERM_L_DARK, out_val, i + y + 3, 68);
			}
		}
	}
}

/* Similar to AU_DURING_BROWSE: Allow 'S'elling directly from browsing a subinventory,
   but the store screen needs to refresh the displayed stock accordingly, while we're still in the icky subinventory screen. */
#define STOCK_DURING_BROWSE
void display_store_inventory(void) {
	int i, k, entries = 12, y = 2;

#ifdef STOCK_DURING_BROWSE
	/* Display new stock in the actual shopping screen, not in our icky-subinventory screen */
	if (screen_icky > 1) Term_switch(1);
#endif

	/* BIG_MAP leads to big shops */
	if (screen_hgt == MAX_SCREEN_HGT) entries = 26; /* we don't have 34 letters in the alphabet =p */

	for (k = 0; k < entries; k++) {
		/* Do not display "dead" items */
		if (store_top + k >= store.stock_num) break;

		/* Display that one */
		display_entry(store_top + k, entries);
	}

	/* Erase the extra lines, +1 for the "more" prompt */
	for (i = k; i < entries + 1; i++) prt("", i + y + 3, 0);

	/* Assume "no current page" */
	put_str("             ", y + 2, 20);

	/* Visual reminder of "more items" */
	if (store.stock_num > entries) {
		/* Show "more" reminder (after the last item) */
		put_str("-more (Space/Backspace to flip page)-", k + y + 3, 3);

		/* Indicate the "current page" */
		put_str(format("(Page %d of %d)%s%s",
		    store_top / entries + 1, store.stock_num == 0 ? 1 : (store.stock_num + entries - 1) / entries,
		    store_top / entries + 1 >= 10 ? "" : " ", (store.stock_num + entries - 1) / entries >= 10 ? "" : " "),
		    y + 2, 20);
	}

#ifdef STOCK_DURING_BROWSE
	if (screen_icky > 1) {
		/* Return from actual shopping screen to our icky-subinventory screen... */
		Term_switch(1);
		/* ...but also show us what has changed in the underlying shopping screen now that we potentially sold an item from our subinventory. */
		Term_restore();
	}
#endif

	/* Hack - Get rid of the cursor - mikaelh */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;
}

/*
 * Get the ID of a store item and return its value      -RAK-
 */
static int get_stock(int *com_val, cptr pmt, int i, int j) {
	char command;
	char out_val[160];

	/* Paranoia XXX XXX XXX */
	clear_topline_forced();

	/* Assume failure */
	*com_val = (-1);

	/* Build the prompt */
	if (store_last_item == -1) (void)sprintf(out_val, "(Items %c-%c, ESC to exit) %s", I2A(i), I2A(j), pmt);
	else (void)sprintf(out_val, "(Items %c-%c, + for previous, ESC to exit) %s", I2A(i), I2A(j), pmt);

	/* Ask until done */
	while (TRUE) {
	        int k;

		/* Escape */
		if (!get_com(out_val, &command)) break;

		if (command == '+') command = 'a' + store_last_item;

		/* Convert */
		k = (islower(command) ? A2I(command) : -1);

		/* Remember this item pile in the stock for buying/stealing more of the same */
		store_last_item = k;
		store_last_item_tval = store.stock[k].tval;
		store_last_item_sval = store.stock[k].sval;
		/* ...except if this was actually the last item!
		   (We assume that this command gets through and the item _is_ really removed.
		   If it isn't, the "+" option won't be available and the player will just have to
		   operate on this last item manually, also not a big loss, since it's only 1.)
		   Again, this can fail if macro doesn't use \w waits because the stock number update
		   might not yet been received from the server for numbers > 1.
		   Potential local workaround: If client knew that we are specifically stealing,
		   it could assume each attempt successful and decrement a working copy of the stock number
		   as local counter and use that instead of the server-updated stock.number. If stealing failed
		   we'd have to re-enter the store anyway so the stock number would correctly get updated again.
		   ...Or just use \w waits in the macros. */
		if (store.stock[k].number == 1) store_last_item = -1;

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
	if (command == ESCAPE) return(FALSE);

	/* Success */
	return(TRUE);
}




/* XXX Bad design.. store code really should be rewritten.	- Jir - */
static void store_examine(bool no_browse) {
	int i, item;
	object_type *o_ptr;
	char out_val[160];

	/* BIG_MAP leads to big shops */
	int entries = (screen_hgt == MAX_SCREEN_HGT) ? 26 : 12;


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
	if (!get_stock(&item, out_val, 0, i - 1)) return;

	/* Get the actual index */
	item = item + store_top;

	/* Get the actual item */
	o_ptr = &store.stock[item];

	/* Tell the server */
	if (no_browse) { //hack: Don't browse books but truly examine them
		Send_store_examine(item);
		return;
	}
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
	int entries = (screen_hgt == MAX_SCREEN_HGT) ? 26 : 12;


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
	if (!get_stock(&item, out_val, 0, i - 1)) return;

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
		int limitG = scan_auto_inscriptions_for_limit(store_names[item]);

		/* Hack -- note cost of "fixed" items */
		if (store_num != STORE_HOME && store_num != STORE_HOME_DUN) {
			c_msg_print(format("That costs %d gold per item.", store_prices[item]));

			if (store_prices[item] > 0) amt_afford = p_ptr->au / store_prices[item];
			else amt_afford = o_ptr->number;

			/* Get a quantity */
			if (o_ptr->number <= amt_afford)
				if (limitG) amt = c_get_quantity(NULL, limitG <= amt_afford ? limitG : o_ptr->number, o_ptr->number);
				else amt = c_get_quantity(NULL, 1, o_ptr->number);
			else if (amt_afford > 1) {
				inkey_letter_all = TRUE;
				if (limitG) amt = c_get_quantity(NULL, limitG <= amt_afford ? limitG : amt_afford, amt_afford);
				else {
					sprintf(out_val, "Quantity (1-\377y%d\377w, 'a' or spacebar for all): ", amt_afford);
					amt = c_get_quantity(out_val, 1, amt_afford);
				}
			} else {
				sprintf(out_val, "Quantity (\377y1\377w): ");
				amt = c_get_quantity(out_val, 1, -1);
			}
		} else {
			/* Get a quantity */
			if (limitG) amt = c_get_quantity(NULL, limitG <= o_ptr->number ? limitG : o_ptr->number, o_ptr->number);
			else amt = c_get_quantity(NULL, 1, o_ptr->number);
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
/* price, 2x colour code, " Au": */
#define PRICE_LEN (16 + 4 + 3)
void store_paste_item(char *out_val, int item) {
	char price[PRICE_LEN];

	/* Get shop price if any */
	/* Home doesn't price items */
	if (store_num == STORE_HOME || store_num == STORE_HOME_DUN) {
		sprintf(out_val, " %s", store_names[item]);
	/* Player stores with '@S-' inscription neither (museum mode) */
	} else if (store_prices[item] < 0 && store_prices[item] != -3) {
		if (store_powers[item][0]) sprintf(out_val, " %s (%s)", store_names[item], store_powers[item]);
		else sprintf(out_val, " %s", store_names[item]);
	/* Normal [player-]store */
	} else {
		char price_col[3], price_col2[3];

		/* Indicate blacklist pricing (40x) */
		if (store_price_mul > 10) {  //we're blacklisted
			strcpy(price_col, "\377r");
			strcpy(price_col2, "\377s");
		} else if (store_price_mul < 10) { //doesn't exist actually
			strcpy(price_col, "\377g");
			strcpy(price_col2, "\377s");
		} else { //normal price
			price_col[0] = 0;
			price_col2[0] = 0;
		}

		/* @SB inscription and sold out: */
		if (store_prices[item] == -3) snprintf(price, PRICE_LEN, "\377DSOLD OUT\377s");
		/* Convert the price to more readable format */
		else if (store_prices[item] >= 10000000)
			snprintf(price, PRICE_LEN, "%s%dM Au%s", price_col, store_prices[item] / 1000000, price_col2);
		else if (store_prices[item] >= 10000)
			snprintf(price, PRICE_LEN, "%s%dk Au%s", price_col, store_prices[item] / 1000, price_col2);
		else
			snprintf(price, PRICE_LEN, "%s%d Au%s", price_col, store_prices[item], price_col2);

		/* Paste store_powers if available (randarts etc) or -specialty!- for books, their spells */
		if (store.stock[item].tval == TV_BOOK) {
			char powins[POW_INSCR_LEN];

			powins[0] = 0;
			if (is_custom_tome(store.stock[item].sval)) {
				//if (redux) {
				if (TRUE) {
					if (store.stock[item].xtra1) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name2)", store.stock[item].xtra1 - 1))));
					if (store.stock[item].xtra2) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name2)", store.stock[item].xtra2 - 1))));
					if (store.stock[item].xtra3) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name2)", store.stock[item].xtra3 - 1))));
					if (store.stock[item].xtra4) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name2)", store.stock[item].xtra4 - 1))));
					if (store.stock[item].xtra5) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name2)", store.stock[item].xtra5 - 1))));
					if (store.stock[item].xtra6) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name2)", store.stock[item].xtra6 - 1))));
					if (store.stock[item].xtra7) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name2)", store.stock[item].xtra7 - 1))));
					if (store.stock[item].xtra8) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name2)", store.stock[item].xtra8 - 1))));
					if (store.stock[item].xtra9) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name2)", store.stock[item].xtra9 - 1))));
				} else {
					if (store.stock[item].xtra1) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name)", store.stock[item].xtra1 - 1))));
					if (store.stock[item].xtra2) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name)", store.stock[item].xtra2 - 1))));
					if (store.stock[item].xtra3) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name)", store.stock[item].xtra3 - 1))));
					if (store.stock[item].xtra4) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name)", store.stock[item].xtra4 - 1))));
					if (store.stock[item].xtra5) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name)", store.stock[item].xtra5 - 1))));
					if (store.stock[item].xtra6) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name)", store.stock[item].xtra6 - 1))));
					if (store.stock[item].xtra7) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name)", store.stock[item].xtra7 - 1))));
					if (store.stock[item].xtra8) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name)", store.stock[item].xtra8 - 1))));
					if (store.stock[item].xtra9) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name)", store.stock[item].xtra9 - 1))));
				}
			} else if (store.stock[item].sval != SV_SPELLBOOK) { /* Predefined book (handbook or tome) */
				int i, num = 0, spell;
				char out_val[160];

				/* Find amount of spells in this book */
				sprintf(out_val, "return book_spells_num2(%d, %d)", -1, store.stock[item].sval);
				num = exec_lua(0, out_val);

				/* Iterate through them and display their [short] names */
				for (i = 0; i < num; i++) {
					/* Get the spell */
					if (MY_VERSION < (4 << 12 | 4 << 8 | 1U << 4 | 8)) {
						/* no longer supported! to make s_aux.lua slimmer */
						spell = exec_lua(0, format("return spell_x(%d, %d, %d)", store.stock[item].sval, -1, i));
					} else {
						spell = exec_lua(0, format("return spell_x2(%d, %d, %d, %d)", -1, store.stock[item].sval, -1, i));
					}

					/* Book doesn't contain a spell in the selected slot */
					if (spell == -1) continue;

					/* Hack: For now, always use redux format because it'd become toooooo much */
					//if (redux || TRUE) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name2)", spell))));
					if (TRUE) strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name2)", spell))));
					else strcat(powins, format("%s,", string_exec_lua(0, format("return(__tmp_spells[%d].name)", spell))));
				}
			}

			if (powins[0] && powins[strlen(powins) - 1] == ',') powins[strlen(powins) - 1] = 0;

			if (powins[0]) sprintf(out_val, "%s (%s) (%s)", store_names[item], price, powins);
			else sprintf(out_val, "%s (%s)", store_names[item], price);
			return;
		}

		if (store_powers[item][0]) sprintf(out_val, "%s (%s) (%s)", store_names[item], price, store_powers[item]);
		else sprintf(out_val, "%s (%s)", store_names[item], price);
	}
}

static void store_chat(void) {
	int	i, item;

	char	out_val[MSG_LEN], buf[MSG_LEN];
	char	where[17];

	/* BIG_MAP leads to big shops */
	int entries = (screen_hgt == MAX_SCREEN_HGT) ? 26 : 12;


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

#if defined(KIND_DIZ) && defined(CLIENT_ITEM_PASTE_DIZ)
	if (sflags1 & SFLG1_CIPD) {
		int j;

		/* We don't have the local k_idx, so we have to find it from tval,sval: */
		for (j = 0; j < kind_list_idx; j++) {
			if (kind_list_tval[j] != store.stock[item].tval || kind_list_sval[j] != store.stock[item].sval) continue;
			if (kind_list_dizline[j][0]) Send_paste_msg(format("%s\377D - %s", buf, kind_list_dizline[j]));
			else j = kind_list_idx; //hack: no-diz marker
			break;
		}
		if (j == kind_list_idx) {
			Send_paste_msg(buf);
			return;
		}
	} else
#elif defined(SERVER_ITEM_PASTE_DIZ)
	if (sflags1 & SFLG1_SIPD) Send_paste_msg(format("%s\372%d,%d", buf, store.stock[item].tval, store.stock[item].sval));
	else
#endif
	Send_paste_msg(buf);
}

static void store_sell(void) {
	int item, amt, num, tval, ng;
	cptr name;

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

#ifdef ENABLE_SUBINVEN
	if (item >= SUBINVEN_INVEN_MUL) {
		num = subinventory[item / SUBINVEN_INVEN_MUL - 1][item % SUBINVEN_INVEN_MUL].number;
		tval = subinventory[item / SUBINVEN_INVEN_MUL - 1][item % SUBINVEN_INVEN_MUL].tval;
		name = subinventory_name[item / SUBINVEN_INVEN_MUL - 1][item % SUBINVEN_INVEN_MUL];
		ng = check_guard_inscription_str(subinventory_name[item / SUBINVEN_INVEN_MUL - 1][item % SUBINVEN_INVEN_MUL], 'G');
	} else
#endif
	{
		num = inventory[item].number;
		tval = inventory[item].tval;
		name = inventory_name[item];
		ng = check_guard_inscription_str(inventory_name[item], 'G');
	}

	/* Get an amount */
	if (num > 1) {
		if (is_cheap_misc(tval) && c_cfg.whole_ammo_stack && !verified_item) amt = num;
		else {
			inkey_letter_all = TRUE;
			if (ng > 1 && (ng = num - ng + 1) > 0)
				amt = c_get_quantity(format("How many (ENTER = %d, 'a' = all)? ", ng), ng, num);
			else
				amt = c_get_quantity("How many (ENTER/'a'/SPACE = all)? ", num, num);
		}
	} else amt = 1;

	/* Hack -- verify for Museum(Mathom house) */
	if (store_num == STORE_MATHOM_HOUSE) {
		char out_val[160];

		if (num == amt)
			sprintf(out_val, "Really donate %s?", name);
		else
			sprintf(out_val, "Really donate %d of your %s?", amt, name);
		if (!get_check2(out_val, FALSE)) return;
	}

	/* Tell the server */
	Send_store_sell(item, amt);
}

void store_do_command(int num, bool one) {
	int i, amt, gold;
	int item, item2;
	u16b action = c_store.actions[num];
	u16b bact = c_store.bact[num];
	char out_val[160];
	/* BIG_MAP leads to big shops */
	int entries = (screen_hgt == MAX_SCREEN_HGT) ? 26 : 12;
	int get_item_mode = 0;

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
		store_examine(one); //one: hack -> don't browse books but truly examine them
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
		get_item_mode = (USE_EQUIP | USE_INVEN);

		if (c_store.flags[num] & BACT_F_STAR_ID) {
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_starid;
			get_item_hook_find_obj_what = "Which item? "; /* Seems it's not needed, but just in case */
			get_item_mode |= USE_EXTRA;
			if (!c_get_item(&item, "Which item? ", get_item_mode)) return;
		} else if (c_store.flags[num] & BACT_F_ID) {
			get_item_extra_hook = get_item_hook_find_obj;
			item_tester_hook = item_tester_hook_id;
			get_item_hook_find_obj_what = "Which item? "; /* Seems it's not needed, but just in case */
			get_item_mode |= USE_EXTRA;
			if (!c_get_item(&item, "Which item? ", get_item_mode)) return;
		} else if (c_store.flags[num] & BACT_F_ARMOUR) {
			if (c_store.flags[num] & BACT_F_NO_SHIELDS) {
				if (c_store.flags[num] & BACT_F_TOGGLE_CLOAKS) {
					if (c_store.flags[num] & BACT_F_NO_ART) item_tester_hook = item_tester_hook_nonart_armour_no_shield_no_cloak;
					else item_tester_hook = item_tester_hook_armour_no_shield_no_cloak;
				} else {
					if (c_store.flags[num] & BACT_F_NO_ART) item_tester_hook = item_tester_hook_nonart_armour_no_shield;
					else item_tester_hook = item_tester_hook_armour_no_shield;
				}
			} else {
				if (c_store.flags[num] & BACT_F_TOGGLE_CLOAKS) {
					if (c_store.flags[num] & BACT_F_NO_ART) item_tester_hook = item_tester_hook_nonart_armour_no_cloak;
					else item_tester_hook = item_tester_hook_armour_no_cloak;
				} else {
					if (c_store.flags[num] & BACT_F_NO_ART) item_tester_hook = item_tester_hook_nonart_armour;
					else item_tester_hook = item_tester_hook_armour;
				}
			}
			get_item_extra_hook = get_item_hook_find_obj;
			get_item_hook_find_obj_what = "Which piece of armour? "; /* Seems it's not needed, but just in case */
			get_item_mode |= USE_EXTRA;
			if (!c_get_item(&item, "Which armour? ", get_item_mode)) return;
		} else if (c_store.flags[num] & BACT_F_TOGGLE_CLOAKS) {
			if (c_store.flags[num] & BACT_F_NO_ART) item_tester_hook = item_tester_hook_nonart_cloak;
			else item_tester_hook = item_tester_hook_cloak;
			get_item_extra_hook = get_item_hook_find_obj;
			get_item_hook_find_obj_what = "Which piece of armour? "; /* Seems it's not needed, but just in case */
			get_item_mode |= USE_EXTRA;
			if (!c_get_item(&item, "Which armour? ", get_item_mode)) return;
		} else if (c_store.flags[num] & BACT_F_WEAPON) {
			get_item_extra_hook = get_item_hook_find_obj;
			if (c_store.flags[num] & BACT_F_NO_ART) item_tester_hook = item_tester_hook_nonart_weapon;
			else item_tester_hook = item_tester_hook_weapon;
			get_item_hook_find_obj_what = "Which weapon? "; /* Seems it's not needed, but just in case */
			get_item_mode |= USE_EXTRA;
			if (!c_get_item(&item, "Which weapon? ", get_item_mode)) return;
		} else if (c_store.flags[num] & BACT_F_AMMO) {
			get_item_extra_hook = get_item_hook_find_obj;
			if (c_store.flags[num] & BACT_F_NO_ART) item_tester_hook = item_tester_hook_nonart_ammo;
			else item_tester_hook = item_tester_hook_ammo;
			get_item_hook_find_obj_what = "What ammunition? "; /* Seems it's not needed, but just in case */
			get_item_mode |= USE_EXTRA;
			if (!c_get_item(&item, "What ammunition? ", get_item_mode)) return;
		} else if (c_store.flags[num] & BACT_F_RANGED) {
			get_item_extra_hook = get_item_hook_find_obj;
			if (c_store.flags[num] & BACT_F_NO_ART) item_tester_hook = item_tester_hook_nonart_ranged;
			else item_tester_hook = item_tester_hook_ranged;
			get_item_hook_find_obj_what = "Which ranged weapon? "; /* Seems it's not needed, but just in case */
			get_item_mode |= USE_EXTRA;
			if (!c_get_item(&item, "Which ranged weapon? ", get_item_mode)) return;
		} else if (c_store.flags[num] & BACT_F_MELEE) {
			get_item_extra_hook = get_item_hook_find_obj;
			if (c_store.flags[num] & BACT_F_NO_ART) item_tester_hook = item_tester_hook_nonart_melee;
			else item_tester_hook = item_tester_hook_melee;
			get_item_hook_find_obj_what = "Which melee weapon? "; /* Seems it's not needed, but just in case */
			get_item_mode |= USE_EXTRA;
			if (!c_get_item(&item, "Which melee weapon? ", get_item_mode)) return;
		} else if (c_store.flags[num] & BACT_F_TRAPKIT_FA) {
			get_item_extra_hook = get_item_hook_find_obj;
			if (c_store.flags[num] & BACT_F_NO_ART) item_tester_hook = item_tester_hook_nonart_trapkit_fa;
			else item_tester_hook = item_tester_hook_trapkit_fa;
			get_item_hook_find_obj_what = "Which firearm-trapkit? "; /* Seems it's not needed, but just in case */
			get_item_mode |= USE_EXTRA;
			if (!c_get_item(&item, "Which firearm-trapkit? ", get_item_mode)) return;
		} else if (!c_get_item(&item, "Which item? ", get_item_mode)) return;
	}

	if (c_store.flags[num] & BACT_F_GOLD) {
		/* Get how much */
		inkey_letter_all = TRUE;
		gold = c_get_quantity("How much gold ('a' or spacebar for all)? ", 1, -1);

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
	int entries = (screen_hgt == MAX_SCREEN_HGT) ? 26 : 12;

	for (i = 0; i < MAX_STORE_ACTIONS; i++) {
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

	/* If action was not already consumed by store-defined command keys above:
	   Hack - translate p/s into d/g and vice versa, for extra comfort. */
	for (i = 0; i < MAX_STORE_ACTIONS; i++) {
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
			if (cmd == 'l' || cmd == 'x') {
				store_do_command(i, FALSE);
				return;
			}
			/* Hack (abuse 'bool one') - Allow examing with SHIFT+x specifically for spell books to NOT browse them but truly examine the actual book object */
			if (cmd == 'X') {
				store_do_command(i, TRUE);
				return;
			}
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
			clear_pstore_visuals();
			break;

		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", FALSE);
			break;

		/* Browse */
		case ' ':
			if (store.stock_num <= entries) {
				if (store_top) {
					/*
					 * Hack - Allowing going back to first page after buying
					 * last item on the second page. - mikaelh */
					store_top = 0;
					display_store_inventory();
				} else {
					c_msg_print("Entire inventory is shown.");
				}
			} else {
				store_top += entries;
				if (store_top >= store.stock_num) store_top = 0;
				display_store_inventory();
			}
			break;

		/* Allow to browse backwards via BACKSPACE */
		case '\b':
			if (store.stock_num <= entries) {
				if (store_top) {
					/* see above - C. Blue */
					store_top = 0;
					display_store_inventory();
				} else {
					c_msg_print("Entire inventory is shown.");
				}
			} else {
				store_top -= entries;
				if (store_top < 0) store_top = ((store.stock_num - 1) / entries) * entries;
				display_store_inventory();
			}
			break;

		/* go to page n */
		case '0': i++; __attribute__ ((fallthrough));
		case '9': i++; __attribute__ ((fallthrough));
		case '8': i++; __attribute__ ((fallthrough));
		case '7': i++; __attribute__ ((fallthrough));
		case '6': i++; __attribute__ ((fallthrough));
		case '5': i++; __attribute__ ((fallthrough));
		case '4': i++; __attribute__ ((fallthrough));
		case '3': i++; __attribute__ ((fallthrough));
		case '2': i++; __attribute__ ((fallthrough));
		case '1':
			if (store.stock_num > entries * i) {
				store_top = entries * i;
				display_store_inventory();
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
		/* Added rather for QoL subinventory browsing (instead of having to go 'i' -> '!') than for actual books */
		case 'b':
			cmd_browse(-1);
			break;

#ifdef USE_SOUND_2010
		case KTRL('C'):
		case KTRL('X')://rl
			toggle_music(FALSE);
			break;
		case KTRL('N'):
		case KTRL('V')://rl
			toggle_master(FALSE);
			break;
#endif

		case '{':
			cmd_inscribe(USE_INVEN | USE_EQUIP);
			break;
		case '}':
			cmd_uninscribe(USE_INVEN | USE_EQUIP);
			break;

		/* special feat for some stores: allow wear/wield and take-off..
		   'emulating' rogue-like keyset option for now, sigh */
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

#define AU_DURING_BROWSE
void c_store_prt_gold(void) {
	char out_val[64];
	/* BIG_MAP leads to big shops */
	int spacer = (screen_hgt == MAX_SCREEN_HGT) ? 14 : 0, x = 51, y = 2;

	/* While we're inside a store, the screen is icky by default.
	   Now if we're also browsing an info file (eg exploration history in the mathom house), we're in screen 2 (0 being map window, 1 being the store).
	   So we need to switch to store screen to redraw the gold properly, then switch back to browse screen.
	   (If we instead just return, we'll see the wrong gold value listed back in the shop screen after exiting the file-browsing.) */
#ifdef AU_DURING_BROWSE /* If player is browsing a spell scroll, this will still overwrite Au live. Should be no problem, since spell-browsing will never go that far down to get overwritten by it visually */
	if (special_line_type) Term_switch(1);
#else /* If player is browsing a spell scroll, this will still write Au in the background as screen_icky is 2. So Au will not update until spell-browsing view is closed. */
	if (special_line_type || screen_icky > 1) Term_switch(1);
#endif

	/* 2024-07-09: Support up to 9 store actions in ba_info.txt maybe -> no empty spacer line between stock and gold? */
	//spacer--;

	prt("Gold Remaining: ", y + 16 + spacer, x);

	if (c_cfg.colourize_bignum) colour_bignum(p_ptr->au, -1, out_val, 0, TRUE);
	else sprintf(out_val, "%10d", p_ptr->au);
	put_str(out_val, y + 16 + spacer, x + 16);

	/* Hack -- show balance (if not 0) */
	if (store_num == STORE_MERCHANTS_GUILD && p_ptr->balance) {
		prt("Your balance  : ", y + 17 + spacer, x);

		if (c_cfg.colourize_bignum) colour_bignum(p_ptr->balance, -1, out_val, 0, TRUE);
		else sprintf(out_val, "%10d", p_ptr->balance);
		put_str(out_val, y + 17 + spacer, x + 16);
	} else {
		/* Erase part of the screen */
		Term_erase(x, y + 17 + spacer, 255);
	}

#ifdef AU_DURING_BROWSE
	if (special_line_type) Term_switch(1);
#else
	if (special_line_type || screen_icky > 1) Term_switch(1);
#endif

	/* hack: hide cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;
}

void do_redraw_store(void) {
	int y = 2;

	if (perusing) return; /* don't overwrite inspect-item screen when home inventory updates (due to an item timing out) */

	redraw_store = FALSE;

	if (shopping) {
		/* hack: Display changed capacity if we just extended the house */
		if ((store_num == STORE_HOME || store_num == STORE_HOME_DUN)
		    && c_store.max_cost) {
			char buf[1024];

			sprintf(buf, "%s (Stock: %d/%d)", c_store.store_name, store.stock_num, c_store.max_cost);
			prt(buf, y, 50);
		}
		display_store_inventory();
	}
}

void display_store(void) {
	int i, y = 2, y2 = 1;
	char buf[1024];
	/* BIG_MAP leads to big shops */
	int spacer = (screen_hgt == MAX_SCREEN_HGT) ? 14 : 0;

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
		put_str(buf, y, 10);

		/* Show the store name (usually blank for houses) and -hack- max_cost is the capacity */
		if (c_store.max_cost)
			sprintf(buf, "%s (Stock: %d/%d)", c_store.store_name, store.stock_num, c_store.max_cost);
		else
			sprintf(buf, "%s", c_store.store_name);
		prt(buf, y, 50);

		/* Label the item descriptions */
		put_str("Item Description", y + 2, 3);

		/* If showing weights, show label */
		if (c_cfg.show_weights) put_str("Weight", y + 2, 70);
	}

	/* Normal stores */
	else {
		/* Put the owner name and race */
		sprintf(buf, "%s", c_store.owner_name);
		put_str(buf, y, 10);

		/* Show the max price in the store (above prices) */
		if (!c_store.max_cost) {
			/* improve it a bit visually for player stores who don't buy anything */
			sprintf(buf, "%s", c_store.store_name);
		} else {
			sprintf(buf, "%s (%d)", c_store.store_name, c_store.max_cost);
		}
		prt(buf, y, 50);

		/* Label the item descriptions */
		put_str("Item Description", y + 2, 3);

		/* If showing weights, show label */
		if (c_cfg.show_weights) put_str("Weight", y + 2, 60);

		/* Label the asking price (in stores) */
		put_str("Price", y + 2, 72);

		/* Display the players remaining gold */
		c_store_prt_gold();
	}

	/* Display the inventory */
	display_store_inventory();

	/* Don't leave */
	leave_store = FALSE;

	/* Start at the top */
	store_top = 0;

	/* Interact with player */
	while (!leave_store) {
		/* Hack -- Clear line 1 */
		prt("", 1, 0);

		/* Clear */
		clear_from(y + 18 + spacer);

		/* Prompt */
		if (store_num == STORE_HOME || store_num == STORE_HOME_DUN) prt(" ESC) Exit house", y + y2 + 17 + spacer, 0);
		else prt(" ESC) Exit store", y + y2 + 17 + spacer, 0);
		if (store.stock_num) prt("   c) Paste to chat", y + y2 + 18 + spacer, 0);
		if (store.stock_num > 12 + spacer)
			prt(format("%s1-%d) Go to page", (store.stock_num - 1) / (12 + spacer) + 1 >= 10 ? "" : " ", (store.stock_num - 1) / (12 + spacer) + 1), y + y2 + 19 + spacer, 0);

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
	store_last_item = -1;

	/* Flush any events that happened */
	Flush_queue();

	/* reload the term */
	Term_load();
}

/* For the new SPECIAL flag for non-inven stores - C. Blue */
void display_store_special(void) {
	int i, y = 2, y2 = 1;
	char buf[1024];
	/* BIG_MAP leads to big shops */
	int spacer = (screen_hgt == MAX_SCREEN_HGT) ? 14 : 0;

	/* Save the term */
	Term_save();

	/* We are "shopping" */
	shopping = TRUE;

	/* Clear screen */
	Term_clear();

	/* Put the owner name and race */
	sprintf(buf, "%s", c_store.owner_name);
	put_str(buf, y, 10);

	sprintf(buf, "%s", c_store.store_name);
	prt(buf, y, 50);

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
		clear_from(y + 18 + spacer);

		/* Prompt */
		if (store_num == STORE_HOME || store_num == STORE_HOME_DUN) prt("ESC) Exit house", y + y2 + 18 + spacer, 0);
		else prt("ESC) Exit store", y + y2 + 18 + spacer, 0);

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
	store_last_item = -1;

	/* Flush any events that happened */
	Flush_queue();

	/* reload the term */
	Term_load();
}

/* Fix visuals a bit: When entering a pstore after entering another pstore or npc store, the old contents might flicker
   up for a moment until they get loaded from the new pstore. Clear them instead on leaving a pstore to prevent that. */
void clear_pstore_visuals(void) {
	int i;

	if (store_num > -2) return;
	for (i = 0; i < STORE_INVEN_MAX; i++)
		store_names[i][0] = 0;
}
