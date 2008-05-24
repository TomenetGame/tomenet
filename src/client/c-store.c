/* $Id$ */
/*
 * Client-side store stuff.
 */

#include "angband.h"

bool leave_store;
static int store_top;



/*
 * ToME show_building, ripped for client	- Jir -
 */
void display_store_action()
{
	int i;

	for (i = 0; i < 6; i++)
	{
		if (!c_store.actions[i]) continue;

		c_put_str(c_store.action_attr[i], c_store.action_name[i],
				21 + (i / 2), 17 + (30 * (i % 2)));
	}
}

static void display_entry(int pos)
{
	object_type *o_ptr;
	int i, x;
	char o_name[80];
	char out_val[160];

	int maxwid = 75;

	/* Get the item */
	o_ptr = &store.stock[pos];

        /* Get the "offset" */
        i = (pos % 12);

        /* Label it, clear the line --(-- */
        (void)sprintf(out_val, "%c) ", I2A(i));
        prt(out_val, i+6, 0);

        /* Describe an item in the home */
        if (store_num == 7)
        {
                maxwid = 75;

                /* Leave room for weights, if necessary -DRS- */
                if (c_cfg.show_weights) maxwid -= 10;

                /* Describe the object */
		strcpy(o_name, store_names[pos]);
                o_name[maxwid] = '\0';
                c_put_str(o_ptr->xtra1, o_name, i+6, 3);

                /* Show weights */
                if (c_cfg.show_weights)
                {
                        /* Only show the weight of an individual item */
                        int wgt = o_ptr->weight;
                        (void)sprintf(out_val, "%3d.%d lb", wgt / 10, wgt % 10);
                        put_str(out_val, i+6, 68);
                }
        }

        else
        {
                /* Must leave room for the "price" */
                maxwid = 65;

                /* Leave room for weights, if necessary -DRS- */
                if (c_cfg.show_weights) maxwid -= 7;

                /* Describe the object (fully) */
		strcpy(o_name, store_names[pos]);
                o_name[maxwid] = '\0';
                c_put_str(o_ptr->xtra1, o_name, i+6, 3);

                /* Show weights */
                if (c_cfg.show_weights)
                {
                        /* Only show the weight of an individual item */
                        int wgt = o_ptr->weight;
                        (void)sprintf(out_val, "%3d.%d", wgt / 10, wgt % 10);
                        put_str(out_val, i+6, 61);
                }

		x = store_prices[pos];

		/* Actually draw the price (not fixed) */
		(void)sprintf(out_val, "%9ld  ", (long)x);
		c_put_str(p_ptr->au < x ? TERM_L_DARK : TERM_WHITE, out_val, i+6, 68);
        }
}



void display_inventory(void)
{
	int i, k;

	for (k = 0; k < 12; k++)
	{
		/* Do not display "dead" items */
		if (store_top + k >= store.stock_num) break;

		/* Display that one */
		display_entry(store_top + k);
	}

	/* Erase the extra lines and the "more" prompt */
	for (i = k; i < 13; i++) prt("", i + 6, 0);

	/* Assume "no current page" */
	put_str("        ", 5, 20);

	/* Visual reminder of "more items" */
	if (store.stock_num > 12)
	{
		/* Show "more" reminder (after the last item) */
		prt("-more-", k + 6, 3);

		/* Indicate the "current page" */
		put_str(format("(Page %d)", store_top/12 + 1), 5, 20);
	}
}

/*
 * Get the ID of a store item and return its value      -RAK-
 */
static int get_stock(int *com_val, cptr pmt, int i, int j)
{
        char    command;

        char    out_val[160];


        /* Paranoia XXX XXX XXX */
        c_msg_print(NULL);


        /* Assume failure */
        *com_val = (-1);

        /* Build the prompt */
        (void)sprintf(out_val, "(Items %c-%c, ESC to exit) %s",
                      I2A(i), I2A(j), pmt);

        /* Ask until done */
        while (TRUE)
        {
                int k;

                /* Escape */
                if (!get_com(out_val, &command)) break;

                /* Convert */
                k = (islower(command) ? A2I(command) : -1);

                /* Legal responses */
                if ((k >= i) && (k <= j))
                {
                        *com_val = k;
                        break;
                }

                /* Oops */
                bell();
        }

        /* Clear the prompt */
        prt("", 0, 0);

        /* Cancel */
        if (command == ESCAPE) return (FALSE);

        /* Success */
        return (TRUE);
}




/* XXX Bad design.. store code really should be rewritten.	- Jir - */
static void store_examine(void)
{
	int                     i;
	int                     item;

	object_type             *o_ptr;

	char            out_val[160];


	/* Empty? */
	if (store.stock_num <= 0)
	{
		if (store_num == 7) c_msg_print("Your home is empty.");
		else c_msg_print("I am currently out of stock.");
		return;
	}

	/* Find the number of objects on this and following pages */
	i = (store.stock_num - store_top);

	/* And then restrict it to the current page */
	if (i > 12) i = 12;

	/* Prompt */
	if (store_num == 7)
	{
		sprintf(out_val, "Which item do you want to examine? ");
	}
	else
	{
		sprintf(out_val, "Which item do you want to examine? ");
	}

	/* Get the item number to be bought */
	if (!get_stock(&item, out_val, 0, i-1)) return;

	/* Get the actual index */
	item = item + store_top;

	/* Get the actual item */
	o_ptr = &store.stock[item];

	/* Tell the server */
	if (is_book(o_ptr)) show_browse(o_ptr);
	else if (o_ptr->tval == TV_BOOK)
		browse_school_spell(o_ptr->sval, o_ptr->pval);
	else Send_store_examine(item);
}


static void store_purchase(void)
{
        int                     i, amt;
        int                     item;

        object_type             *o_ptr;

        char            out_val[160];


        /* Empty? */
        if (store.stock_num <= 0)
        {
                if (store_num == 7) c_msg_print("Your home is empty.");
                else c_msg_print("I am currently out of stock.");
                return;
        }


        /* Find the number of objects on this and following pages */
        i = (store.stock_num - store_top);

        /* And then restrict it to the current page */
        if (i > 12) i = 12;

        /* Prompt */
        if (store_num == 7)
        {
                sprintf(out_val, "Which item do you want to take? ");
        }
        else
        {
                sprintf(out_val, "Which item are you interested in? ");
        }

        /* Get the item number to be bought */
        if (!get_stock(&item, out_val, 0, i-1)) return;

        /* Get the actual index */
        item = item + store_top;

        /* Get the actual item */
        o_ptr = &store.stock[item];

        /* Assume the player wants just one of them */
        amt = 1;

        /* Find out how many the player wants */
        if (o_ptr->number > 1)
        {
                /* Hack -- note cost of "fixed" items */
                if (store_num != 7)
                {
                        c_msg_print(format("That costs %ld gold per item.", (long)(store_prices[item])));
                }

                /* Get a quantity */
                amt = c_get_quantity(NULL, o_ptr->number);

                /* Allow user abort */
                if (amt <= 0) return;
        }

	/* Tell the server */
	Send_store_purchase(item, amt);
}

static void store_sell(void)
{
	int item, amt;

	if (store_num == 7)
	{
		if (!c_get_item(&item, "Drop what? ", TRUE, TRUE, FALSE))
		{
			return;
		}
	}
	else if (store_num == 57)
	{
		if (!c_get_item(&item, "Donate what? ", TRUE, TRUE, FALSE))
		{
			return;
		}
	}
	else
	{
		if (!c_get_item(&item, "Sell what? ", TRUE, TRUE, FALSE))
		{
			return;
		}
	}

	/* Get an amount */
	if (inventory[item].number > 1)
	{
		amt = c_get_quantity("How many? ", inventory[item].number);
	}
	else amt = 1;

	/* Hack -- verify for Museum(Mathom house) */
	if (store_num == 57)
	{
		char out_val[160];

		if (inventory[item].number == amt)
			sprintf(out_val, "Really donate %s? ", inventory_name[item]);
		else
			sprintf(out_val, "Really donate %d of your %s? ", amt, inventory_name[item]);
		if (!get_check(out_val)) return;
	}

	/* Tell the server */
	Send_store_sell(item, amt);
}

static void store_do_command(int num)
{
	int                     i, amt, gold;
	int                     item, item2;
	u16b action = c_store.actions[num];
	u16b bact = c_store.bact[num];

	char            out_val[160];

	i = amt = gold = item = item2 = 0;

	/* lazy job for 'older' commands */
	switch (bact)
	{
		case BACT_SELL:
			store_sell();
			return;
			break;
		case BACT_BUY:
			store_purchase();
			return;
			break;
		case BACT_EXAMINE:
			store_examine();
			return;
			break;
	}

	if (c_store.flags[num] & BACT_F_STORE_ITEM)
	{
		/* Empty? */
		if (store.stock_num <= 0)
		{
			if (store_num == 7) c_msg_print("Your home is empty.");
			else c_msg_print("I am currently out of stock.");
			return;
		}


		/* Find the number of objects on this and following pages */
		i = (store.stock_num - store_top);

		/* And then restrict it to the current page */
		if (i > 12) i = 12;

		/* Prompt */
		sprintf(out_val, "Which item? ");

		/* Get the item number to be bought */
		if (!get_stock(&item, out_val, 0, i-1)) return;

		/* Get the actual index */
		item = item + store_top;
	}

	if (c_store.flags[num] & BACT_F_INVENTORY)
	{
		if (!c_get_item(&item, "Which item? ", TRUE, TRUE, FALSE))
		{
			return;
		}
	}

	if (c_store.flags[num] & BACT_F_GOLD)
	{
		/* Get how much */
		gold = c_get_quantity("How much gold? ", -1);

		/* Send it */
		if (!gold) return;
	}

	if (c_store.flags[num] & BACT_F_HARDCODE)
	{
		/* Nothing for now */
	}

	Send_store_command(action, item, item2, amt, gold);
}

static void store_process_command(void)
{
	int i;
	for (i = 0; i < 6; i++)
	{
		if (!c_store.actions[i]) continue;
		if (c_store.letter[i] == command_cmd)
		{
			store_do_command(i);
			return;
		}
	}

	/* Parse the command */
	switch (command_cmd)
	{
		/* Leave */
		case ESCAPE:
		case KTRL('X'):
		{
			leave_store = TRUE;
			break;
		}

		case KTRL('T'):
		{
			/* Take a screenshot */
			xhtml_screenshot("screenshotXXXX");
			break;
		}

		/* Browse */
		case ' ':
		{
			if (store.stock_num <= 12)
			{
				c_msg_print("Entire inventory is shown.");
			}
			else
			{
				store_top += 12;
				if (store_top >= store.stock_num) store_top = 0;
				display_inventory();
			}
			break;
		}

		/* Get (purchase) */
		case 'g':
		case 'p':
		case 'm':
		{
			store_purchase();
			break;
		}

		/* Drop (Sell) */
		case 'd':
		case 's':
		{
			store_sell();
			break;
		}

		/* eXamine */
		case 'x':
		case 'l':
		{
			store_examine();
			break;
		}

		/* Ignore return */
		case '\r':
		{
			break;
		}

		/* Equipment list */
		case 'e':
		{
			cmd_equip();
			break;
		}

		/* Inventory list */
		case 'i':
		{
			cmd_inven();
			break;
		}


		default:
		{
			cmd_raw_key(command_cmd);
			break;
		}
	}
}

void c_store_prt_gold(void)
{
	char out_val[64];

	prt("Gold Remaining: ", 19, 53);

	sprintf(out_val, "%9ld", (long) p_ptr->au);
	prt(out_val, 19, 68);

	/* Hack -- show balance (if not 0) */
	if (store_num == 56 && p_ptr->balance)
	{
		prt("Your balance  : ", 20, 53);

		sprintf(out_val, "%9ld", (long) p_ptr->balance);
		prt(out_val, 20, 68);
	}
	else
	{
		/* Erase part of the screen */
		Term_erase(0, 20, 255);
	}
}

void display_store(void)
{
	char buf[1024];

	/* Save the term */
	Term_save();

	/* We are "shopping" */
	shopping = TRUE;

	/* Clear screen */
	Term_clear();

	store_top = 0;

	/* The "Home" is special */
	if (store_num == 7)
	{
		/* Put the owner name */
		put_str("Your Home", 3, 30);

		/* Label the item descriptions */
		put_str("Item Description", 5, 3);

		/* If showing weights, show label */
		if (c_cfg.show_weights)
		{
			put_str("Weight", 5, 70);
		}
	}

	/* Normal stores */
	else
	{
		/* Put the owner name and race */
		sprintf(buf, "%s", c_store.owner_name);
		put_str(buf, 3, 10);

		/* Show the max price in the store (above prices) */
		sprintf(buf, "%s (%ld)", c_store.store_name, (long)(c_store.max_cost));
		prt(buf, 3, 50);

		/* Label the item descriptions */
		put_str("Item Description", 5, 3);

		/* If showing weights, show label */
		if (c_cfg.show_weights)
		{
			put_str("Weight", 5, 60);
		}

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
	while (!leave_store)
	{
		/* Hack -- Clear line 1 */
		prt("", 1, 0);

		/* Clear */
		clear_from(21);

		/* Prompt */
		prt("You may: ", 21, 0);

		/* Basic commands */
		prt(" ESC) Exit.", 22, 0);

		/* Browse if necessary */
		if (store.stock_num > 12)
		{
			prt(" SPACE) Next page", 23, 0);
		}

		/* Home commands */
		if (store_num == 7 && FALSE)
		{
			prt(" g) Get an item.", 22, 30);
			prt(" d) Drop an item.", 23, 30);

			prt(" x) eXamine an item.", 22, 60);
		}

		/* Shop commands XXX XXX XXX */
		else
		{
			display_store_action();
		}

		/* Get a command */
		while (!command_cmd)
		{
			/* XXX it's here since it can be changed */
			if (store_num == 7)
			{
				/* Put the house capacity */
				put_str(format("Capacity: %d", c_store.max_cost), 3, 60);
			}

			/* Re-fresh the screen */
			Term_fresh();

			/* Update our timer and send a keepalive packet if
			 * neccecary */
			update_ticks();
			do_keepalive();
			do_ping();

			if (Net_flush() == -1)
			{
				plog("Bad net flush");
				return;
			}

			/* Set the timeout */
//			SetTimeout(0, 1000000 / Setup.frames_per_second);
			SetTimeout(0, next_frame());

			/* Only take input if we got some */
			if (SocketReadable(Net_fd()))
				if (Net_input() == -1)
					return;

			/* Get a command */
			request_command(TRUE);

			/* Flush */
			flush_now();

			/* Redraw windows if necessary */
			if (p_ptr->window)
			{
				window_stuff();
			}
		}

		/* Process the command */
		store_process_command();

		/* Clear the old command */
		command_cmd = 0;
	}

	/* Tell the server that we're outta here */
	Send_store_leave();

	/* Clear the screen */
	Term_clear();

	/* We are no longer "shopping" */
	shopping = FALSE;

	/* Flush any events that happened */
	Flush_queue();

	/* reload the term */
	Term_load();
}
