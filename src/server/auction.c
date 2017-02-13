/* File: auction.c */

/*
 * Purpose: Auction system - mikaelh
 */

/* NOTE: This is still buggy and far from ready */

/* added this for consistency in some (unrelated) header-inclusion,
   it IS a server file, isn't it? */
#define SERVER
#include "angband.h"

#ifdef AUCTION_SYSTEM

/* 
 * TODO:
 * - Debugging
 */

int new_auction();
int count_auctions_player(s32b player);
bool auction_parse_money(cptr s, s32b *amount);
bool auction_parse_time(cptr s, time_t *amount);
void auction_clear(int auction_id);
void auction_add_bid(int auction_id, s32b bid, s32b bidder);
void auction_remove_bid(int auction_id, int bid_id);
void auction_list_print_item(int Ind, int auction_id);
cptr my_strcasestr(cptr haystack, cptr needle);

/* TODO: How badly can this mess up shopping? */
void process_auctions()
{
	int i, j, k;
	auction_type *auc_ptr;
	bid_type *bid_ptr;
	s32b au, balance, total_money;
	time_t now = time(NULL);
	player_type *p_ptr;
	bool found;

	for (i = 1; i < auction_alloc; i++)
	{
		auc_ptr = &auctions[i];

		switch (auc_ptr->status)
		{
			case AUCTION_STATUS_BIDDING:
				/* Check the time limit */
				if (auc_ptr->start + auc_ptr->duration < now)
				{
					/* Auction has expired */
					if (auc_ptr->starting_price && auc_ptr->bids_cnt)
					{
						/* Find the highest bidder who can actually afford the item right now */
						auc_ptr->winning_bid = -1;
						for (j = auc_ptr->bids_cnt - 1; j >= 0; j--)
						{
							bid_ptr = &auc_ptr->bids[j];

							/* Try to find the player */
							found = FALSE;
							for (k = 1; k <= NumPlayers; k++)
							{
								if (Players[k]->id == bid_ptr->bidder)
								{
									p_ptr = Players[k];
									au = p_ptr->au;
									balance = p_ptr->balance;
									found = TRUE;
									break;
								}
							}

							if (!found)
							{
								/* Player isn't on so use the hash table */
								au = lookup_player_au(bid_ptr->bidder);
								if (au == -1)
								{
									/* Player not found */
									continue;
								}
								balance = lookup_player_balance(bid_ptr->bidder);
							}

							/* Check if (s)he has enough money to pay for his/her bet right now */
							total_money = au + balance;
							if (total_money >= bid_ptr->bid)
							{
								auc_ptr->winning_bid = j;
								break;
							}
							
						}
						if (auc_ptr->winning_bid == -1)
						{
							/* No one could afford his/her bid -> cancelled */
							auc_ptr->status = AUCTION_STATUS_CANCELLED;
							/* TODO: Add more status codes? */

							/* Log it */
							s_printf("AUCTION: No valid bids for item #%d\n", i);

							C_KILL(auc_ptr->bids, auc_ptr->bids_cnt, bid_type);
							auc_ptr->bids_cnt = 0;

							/* Check if (s)he is online */
							for (j = 1; j <= NumPlayers; j++)
							{
								if (Players[j]->id == auc_ptr->owner)
								{
									msg_format(j, "\377B[@] \377yAuction for item #%d has ended but no one could pay his/her bid.", i);
									msg_print(j, "\377B[@] \377GYou can \377gretrieve \377Gthe following item:");
									msg_format(j, "\377B[@]   \377w%s", auc_ptr->desc);
									break;
								}
							}
						}
						else
						{
							/* We have a winner */
							auc_ptr->status = AUCTION_STATUS_FINISHED;

							/* Log it */
							s_printf("AUCTION: %s won item #%d from %s for %d\n", lookup_player_name(bid_ptr->bidder), i, lookup_player_name(auc_ptr->owner), bid_ptr->bid);
							s_printf("AUCTION:   %s\n", auc_ptr->desc);

							/* Check if (s)he is online */
							for (j = 1; j <= NumPlayers; j++)
							{
								if (Players[j]->id == bid_ptr->bidder)
								{
									p_ptr = Players[j];
									/* Take money from the bank first */
									if (p_ptr->balance >= bid_ptr->bid)
									{
										p_ptr->balance -= bid_ptr->bid;
									}
									else
									{
										p_ptr->au -= bid_ptr->bid - p_ptr->balance;
										p_ptr->balance = 0;
									}
									auc_ptr->flags |= AUCTION_FLAG_WINNER_PAID;

									msg_format(j, "\377B[@] \377GYou have won item #%d and can now \377gretrieve \377Git!", i);
									msg_format(j, "\377B[@]   \377w%s", auc_ptr->desc);
									msg_format(j, "\377B[@] \377G%d gold has been automatically withdrawn.", bid_ptr->bid);
									break;
								}
							}

							if (auc_ptr->owner > 0)
							{
								/* Check if the seller is online */
								for (j = 1; j <= NumPlayers; j++)
								{
									if (Players[j]->id == auc_ptr->owner)
									{
										s32b amount = bid_ptr->bid - bid_ptr->bid * AUCTION_FEE / 100;
										p_ptr = Players[j];
										p_ptr->balance += amount;
										auc_ptr->flags |= AUCTION_FLAG_SELLER_PAID;

										msg_format(j, "\377B[@] \377GAuction for the follow item #%d has ended.", i);
										msg_format(j, "\377B[@]   \377w%s", auc_ptr->desc);
										msg_format(j, "\377B[@] \377GThe winning bid was %d by %s.", bid_ptr->bid, lookup_player_name(bid_ptr->bidder));
										msg_format(j, "\377B[@] \377GYou received %d.", amount);
										break;
									}
								}
							}
						}
					}
					else
					{
						/* No bids -> cancelled */
						auc_ptr->status = AUCTION_STATUS_CANCELLED;

						/* Log it */
						if (auc_ptr->starting_price)
							s_printf("AUCTION: No bids for item #%d\n", i);

						/* Find the owner and notify him/her */
						for (j = 1; j <= NumPlayers; j++)
						{
							p_ptr = Players[j];
							if (p_ptr->id == auc_ptr->owner)
							{
								if (auc_ptr->starting_price)
									msg_format(j, "\377B[@] \377yAuction for item #%d has ended but there were no bids.", i);
								else /* buyout-only */
									msg_format(j, "\377B[@] \377yAuction for item #%d has ended but nobody bought it.", i);
								msg_print(j, "\377B[@] \377GYou can \377gretrieve \377Gthe following item:");
								msg_format(j, "\377B[@]   \377w%s", auc_ptr->desc);
								break;
							}
						}
					}
				}
				break;
		}
	}
}

int new_auction()
{
	int i;
	time_t now;

	now = time(NULL);

	/* Look for an empty auction slot */
	for (i = 1; i < auction_alloc; i++)
	{
		/* Empty slots can be used 1 hour after they were last used */
		if (auctions[i].status == AUCTION_STATUS_EMPTY &&
		    auctions[i].start + 3600 < now)
		{
			return i;
		}
	}

	/* Allocate more auctions */
	GROW(auctions, auction_alloc, auction_alloc + 16, auction_type);
	i = auction_alloc;
	auction_alloc += 16;
	return i;
}

int count_auctions_player(s32b player)
{
	int count = 0;
	int i;

	for (i = 1; i < auction_alloc; i++)
	{
		if (auctions[i].owner == player)
		{
			count++;
		}
	}

	return count;
}

bool auction_parse_money(cptr s, s32b *amount)
{
	int i = 0;
	char c;
	s32b price = 0;

	*amount = 0;

	while (s[i])
	{
		c = s[i];
		if (c >= '0' && c <= '9')
		{
			price *= 10;
			price += c - '0';
		}
		/* k for thousands */
		else if (c == 'k' || c == 'K')
		{
			price *= 1000;
			break;
		}
		/* m for millions */
		else if (c == 'm' || c == 'M')
		{
			price *= 1000000;
			break;
		}
		else if (c == '-')
		{
			/* Ignore, we don't want negative amounts */
		}
		else
		{
			/* Error */
			return FALSE;
		}
		i++;
	}

	*amount = price;
	return TRUE;
}

bool auction_parse_time(cptr s, time_t *amount)
{
	int i = 0;
	char c;
	int value = 0;
	time_t len = 0;

	*amount = 0;

	while (s[i])
	{
		c = s[i];
		if (c >= '0' && c <= '9')
		{
			value *= 10;
			value += c - '0';
		}
		/* d for days */
		else if (c == 'd' || c == 'D')
		{
			len += value * 86400;
			value = 0;
		}
		/* h for minutes */
		else if (c == 'h' || c == 'H')
		{
			len += value * 3600;
			value = 0;
		}
		/* m for minutes */
		else if (c == 'm' || c == 'M')
		{
			len += value * 60;
			value = 0;
		}
		/* s for seconds */
		else if (c == 's' || c == 'S')
		{
			len += value;
			value = 0;
		}
		else
		{
			/* Error */
			return FALSE;
		}
		i++;
	}

	/* Default: seconds */
	len += value;

	*amount = len;
	return TRUE;
}

/* Return the amount of time in a human readable form */
char *auction_format_time(time_t t)
{
	int days, hours, minutes, seconds;
	char *buf = C_NEW(160, char);

	if (t < 60)
	{
		/* Just seconds */
		snprintf(buf, 160, "%d seconds", (int) t);
	}
	else if (t < 3600)
	{
		/* Minutes and seconds */
		minutes = t / 60;
		seconds = t - minutes * 60;
		if (seconds)
		{
			snprintf(buf, 160, "%d minutes and %d seconds", minutes, seconds);
		}
		else
		{
			snprintf(buf, 160, "%d minutes", minutes);
		}
	}
	else if (t < 86400)
	{
		/* Hours and minutes */
		hours = t / 3600;
		minutes = (t - hours * 3600) / 60;
		if (minutes)
		{
			snprintf(buf, 160, "%d hours and %d minutes", hours, minutes);
		}
		else
		{
			snprintf(buf, 160, "%d hours", hours);
		}
	}
	else
	{
		/* Days, hours and minutes */
		days = t / 86400;
		hours = (t - days * 86400) / 3600;
		minutes = (t - days * 86400 - hours * 3600) / 60;

		if (hours)
		{
			if (minutes)
			{
				snprintf(buf, 160, "%d days, %d hours and %d minutes", days, hours, minutes);
			}
			else
			{
				snprintf(buf, 160, "%d days and %d hours", days, minutes);
			}
		}
		else
		{
			if (minutes)
			{
				snprintf(buf, 160, "%d days and %d minutes", days, minutes);
			}
			else
			{
				snprintf(buf, 160, "%d days", days);
			}
		}
	}

	return buf;
}

void auction_clear(int auction_id)
{
	int i, last_in_use = 1;
	auction_type *auc_ptr = &auctions[auction_id];

	if (auc_ptr->desc)
	{
		C_KILL(auc_ptr->desc, 160, char);
	}

	if (auc_ptr->bids_cnt)
	{
		C_KILL(auc_ptr->bids, auc_ptr->bids_cnt, bid_type);
	}

	WIPE(auc_ptr, auction_type);
	auc_ptr->status = AUCTION_STATUS_EMPTY;
	auc_ptr->start = time(NULL); /* store the time in the start field */

	/* Find the last auction slot in use */
	/* Start from 1 to make sure last_in_use won't be 0 */
	for (i = 1; i < auction_alloc; i++)
	{
		auc_ptr = &auctions[i];
		if (auc_ptr->status != AUCTION_STATUS_EMPTY)
			last_in_use = i;
	}

	/* Round up to next multiple of 16 */
	last_in_use = (last_in_use + 15) / 16 * 16;

	/* Check if the auctions array can be shrunk */
	if (last_in_use < auction_alloc)
	{
		SHRINK(auctions, auction_alloc, last_in_use, auction_type);
	}
}

void auction_add_bid(int auction_id, s32b bid, s32b bidder)
{
	auction_type *auc_ptr = &auctions[auction_id];
	bid_type *bid_ptr;
	int i, j;
	bool placed;

	if (!auc_ptr->bids)
	{
		/* MAKE a bid */
		MAKE(auc_ptr->bids, bid_type);

		bid_ptr = auc_ptr->bids;
		auc_ptr->bids_cnt = 1;
	}
	else
	{
		/* Add a bid */
		GROW(auc_ptr->bids, auc_ptr->bids_cnt, auc_ptr->bids_cnt + 1, bid_type);

		auc_ptr->bids_cnt++;

		placed = FALSE;
		
		/* Keep the bids sorted, lowest bid is first */
		for (i = 0; i < auc_ptr->bids_cnt - 1; i++)
		{
			bid_ptr = &auc_ptr->bids[i];
			if (bid_ptr->bid > bid)
			{
				/* Slide rest of the bids down */
				for (j = i + 1; j < auc_ptr->bids_cnt; j++)
				{
					COPY(&auc_ptr->bids[j], &auc_ptr->bids[j - 1], bid_type);
				}

				/* Add the bid here */
				placed = TRUE;
				break;
			}
		}

		if (!placed) {
			/* Add the bid to the end */
			bid_ptr = &auc_ptr->bids[auc_ptr->bids_cnt - 1];
		}
	}

	bid_ptr->bid = bid;
	bid_ptr->bidder = bidder;
}

void auction_remove_bid(int auction_id, int bid_id)
{
	auction_type *auc_ptr = &auctions[auction_id];
	int i;

	/* Slide the bids up */
	for (i = bid_id + 1; i < auc_ptr->bids_cnt; i++)
	{
		COPY(&auc_ptr->bids[i - 1], &auc_ptr->bids[i], bid_type);
	}

	if (auc_ptr->bids_cnt == 1)
	{
		/* Just KILL it */
		KILL(auc_ptr->bids, bid_type);
	}
	else
	{
		SHRINK(auc_ptr->bids, auc_ptr->bids_cnt, auc_ptr->bids_cnt - 1, bid_type);
	}
	auc_ptr->bids_cnt--;	
}

bool auction_mode_check(int Ind, int auction_id)
{
	auction_type *auc_ptr = &auctions[auction_id];
#if 0 /* I took the liberty of messing around ;) - C. Blue */
	player_type *p_ptr = Players[Ind];

	if ((auc_ptr->mode == MODE_EVERLASTING) && (p_ptr->mode != MODE_EVERLASTING))
		return FALSE; /* covers charmode_trading_restrictions 0 and 1 */
	if ((cfg.charmode_trading_restrictions == 2) &&
	    ((auc_ptr->mode & MODE_EVERLASTING) != (p_ptr->mode & MODE_EVERLASTING)))
		return FALSE; /* added check for charmode_trading_restrictions level 2 */

	return TRUE;

#else

	object_type forge_dummy;

	forge_dummy.owner = auc_ptr->owner; /* assuming auction_type.owner is same kind of 'id' value
					    as o_ptr->owner here; correct me if wrong please - C. Blue */
	forge_dummy.mode = auc_ptr->mode;
	return (compat_pomode(Ind, &forge_dummy) == NULL);
#endif
}

void auction_print_error(int Ind, int n)
{
	if (!n) return;
	switch (n)
	{
		case AUCTION_ERROR_INVALID_ID:
			msg_print(Ind, "\377B[@] \377rError: Invalid auction ID!");
			break;
		case AUCTION_ERROR_NOT_BIDDING:
			msg_print(Ind, "\377B[@] \377rError: Bidding has not yet begun!");
			break;
		case AUCTION_ERROR_OWN_ITEM:
			msg_print(Ind, "\377B[@] \377rError: That's your own item!");
			break;
		case AUCTION_ERROR_OVERFLOW:
			msg_print(Ind, "\377B[@] \377rError: Overflow or invalid input!");
			break;
		case AUCTION_ERROR_INVALID_SLOT:
			msg_print(Ind, "\377B[@] \377rError: Invalid inventory slot!");
			break;
		case AUCTION_ERROR_EMPTY_SLOT:
			msg_print(Ind, "\377B[@] \377rError: Empty inventory slot!");
			break;
		case AUCTION_ERROR_TOO_CHEAP:
			msg_print(Ind, "\377B[@] \377rError: Item is too cheap to be auctioned!");
			break;
		case AUCTION_ERROR_ALREADY_STARTED:
			msg_print(Ind, "\377B[@] \377rError: Bidding on this item has already started!");
			break;
		case AUCTION_ERROR_NOT_SUPPORTED:
			msg_print(Ind, "\377B[@] \377rError: Action not supported!");
			break;
		case AUCTION_ERROR_INSUFFICIENT_MONEY:
			msg_print(Ind, "\377B[@] \377rError: You cannot afford that!");
			break;
		case AUCTION_ERROR_TOO_SMALL:
			msg_print(Ind, "\377B[@] \377rError: Your bid is too small!");
			break;
		case AUCTION_ERROR_INSUFFICIENT_LEVEL:
			msg_print(Ind, "\377B[@] \377rError: Your level is too low!");
			break;
		case AUCTION_ERROR_EVERLASTING_ITEM:
			msg_print(Ind, "\377B[@] \377rError: Item is from an everlasting player!");
			break;
		case AUCTION_ERROR_NONEVERLASTING_ITEM:
			msg_print(Ind, "\377B[@] \377rError: Item is from a non-everlasting player!");
			break;
		case AUCTION_ERROR_INVALID_ACCOUNT:
			msg_print(Ind, "\377B[@] \377rError: Your account is invalid!");
			break;
		case AUCTION_ERROR_INVALID_PRICE:
			msg_print(Ind, "\377B[@] \377rError: Invalid price!");
			break;
		case AUCTION_ERROR_INVALID_DURATION:
			msg_print(Ind, "\377B[@] \377rError: Invalid duration!");
			break;
		case AUCTION_ERROR_TOO_MANY:
			msg_print(Ind, "\377B[@] \377rError: You have too many auctions already in progress!");
			break;
		case AUCTION_ERROR_NO_BIDDING:
			msg_print(Ind, "\377B[@] \377rError: This item is only available for buyout!");
			break;
		case AUCTION_ERROR_NO_BUYOUT:
			msg_print(Ind, "\377B[@] \377rError: Buyout isn't available for this item!");
			break;
		case AUCTION_ERROR_EITHER_BID_OR_BUYOUT:
			msg_print(Ind, "\377B[@] \377rError: You need to allow either bidding or buyout!");
			break;
	}
}

void auction_player_joined(int Ind)
{
	player_type *p_ptr = Players[Ind];
	auction_type *auc_ptr;
	bid_type *bid_ptr;
	int i;

	/* Check auctions */
	for (i = 1; i < auction_alloc; i++)
	{
		auc_ptr = &auctions[i];
		switch (auc_ptr->status)
		{
			case AUCTION_STATUS_SETUP:
				if (auc_ptr->owner == p_ptr->id)
				{
					msg_format(Ind, "\377B[@] \377yYou haven't yet \377gstart\377yed bidding for item #%d:", i);
					msg_format(Ind, "\377B[@]   %s%s", auc_ptr->desc);
				}
				break;
			case AUCTION_STATUS_FINISHED:
				bid_ptr = &auc_ptr->bids[auc_ptr->winning_bid];
				if (bid_ptr->bidder == p_ptr->id)
				{
					if ((auc_ptr->flags & AUCTION_FLAG_WINNER_PAID))
					{
						if (auc_ptr->item.k_idx)
						{
							msg_format(Ind, "\377B[@] \377yYou haven't yet \377Gretrieve\377yd the following item #%d:", i);
							msg_format(Ind, "\377B[@]   \377w%s", auc_ptr->desc);
						}
					}
					else
					{
						/* Take money from the bank first */
						if (p_ptr->balance >= bid_ptr->bid)
						{
							p_ptr->balance -= bid_ptr->bid;
						}
						else
						{
							p_ptr->au -= bid_ptr->bid - p_ptr->balance;
							p_ptr->balance = 0;
						}

						msg_format(Ind, "\377B[@] \377GYou have won item #%d and can now \377gretrieve \377Git!", i);
						msg_format(Ind, "\377B[@]   \377w%s", auc_ptr->desc);
						msg_format(Ind, "\377B[@] \377G%d gold has been automatically withdrawn.", bid_ptr->bid);
					}
				}
				else if (auc_ptr->owner == p_ptr->id)
				{
					/* Check if the seller has been paid */
					if (!(auc_ptr->flags & AUCTION_FLAG_SELLER_PAID))
					{
						s32b amount = bid_ptr->bid - bid_ptr->bid * AUCTION_FEE / 100;

						p_ptr->balance += amount;
						auc_ptr->flags |= AUCTION_FLAG_SELLER_PAID;

						msg_format(Ind, "\377B[@] \377GAuction for item #%d for the follow item has ended.", i);
						msg_format(Ind, "\377B[@]   \377w%s", auc_ptr->desc);
						msg_format(Ind, "\377B[@] \377GThe winning bid was %d by %s", bid_ptr->bid, lookup_player_name(bid_ptr->bidder));
						msg_format(Ind, "\377B[@] \377GYou received %d.", amount);

						/* Check if the auction can be cleared */
						if ((auc_ptr->flags & AUCTION_FLAG_WINNER_PAID) && !auc_ptr->item.k_idx)
						{
							auction_clear(i);
						}
					}
				}
				break;
			case AUCTION_STATUS_CANCELLED:
				if (auc_ptr->owner == p_ptr->id)
				{
					msg_format(Ind, "\377B[@] \377yBidding for item #%d has been cancelled or no one wanted it.", i);
					msg_print(Ind, "\377B[@] \377GYou can retrieve the following item:");
					msg_format(Ind, "\377B[@]   \377w%s", auc_ptr->desc);
				}
				break;
		}
	}
}

void auction_player_death(s32b id)
{
	auction_type *auc_ptr;
	bid_type *bid_ptr;
	int i, j;

	/* Check auctions */
	for (i = 1; i < auction_alloc; i++)
	{
		auc_ptr = &auctions[i];
		switch (auc_ptr->status)
		{
			case AUCTION_STATUS_EMPTY:
				break;
			case AUCTION_STATUS_BIDDING:
				if (auc_ptr->owner == id)
				{
					/* Seller dead */
					auc_ptr->owner = -1;
				}
				else
				{
					for (j = 0; j < auc_ptr->bids_cnt; j++)
					{
						bid_ptr = &auc_ptr->bids[j];
						if (bid_ptr->bidder == id)
						{
							/* Bidder dead */
							auction_remove_bid(i, j);
							break;
						}
					}
				}
				break;
			default:
				if (auc_ptr->owner == id)
				{
					/* Clear it, item will be lost */
					auction_clear(i);
				}
				break;
		}
	}
}

/* Little overflow check macro for auction_set
 * If X is negative we assume it has overflowed
 * If an overflow occurs the item will be returned
 */
#define auction_overflow_check(X) \
	if (X < 0) { \
		inven_carry(Ind, o_ptr); \
		auction_clear(auction_id); \
		return AUCTION_ERROR_OVERFLOW; \
	}

int auction_set(int Ind, int slot, cptr starting_price_string, cptr buyout_price_string, cptr duration_string)
{
	int auction_id;
	auction_type *auc_ptr;
	object_type *o_ptr;
	s64b real_value;
	s32b starting_price, buyout_price;
	player_type *p_ptr = Players[Ind];
	time_t duration;
	time_t now = time(NULL);
	char *time_string;

	/* Verify slot */
	if (slot < 0 || slot >= INVEN_PACK)
	{
		/* Illegal slot */
		return AUCTION_ERROR_INVALID_SLOT;
	}

	if (!p_ptr->inventory[slot].k_idx)
	{
		/* Empty slot */
		return AUCTION_ERROR_EMPTY_SLOT;
	}

	if (count_auctions_player(p_ptr->id) > AUCTION_MAX_ITEMS_PLAYER)
	{
		/* Too many auctions */
		return AUCTION_ERROR_TOO_MANY;
	}

	auction_id = new_auction();

	auc_ptr = &auctions[auction_id];
	auc_ptr->status = AUCTION_STATUS_SETUP;
	p_ptr->current_auction = auction_id;
	auc_ptr->owner = p_ptr->id;
	auc_ptr->mode = p_ptr->mode;
	o_ptr = &auc_ptr->item;

	/* Grab one item */
	COPY(o_ptr, &p_ptr->inventory[slot], object_type);
	o_ptr->number = 1;
	if (is_magic_device(o_ptr->tval)) divide_charged_item(o_ptr, &p_ptr->inventory[slot], 1);

	inven_item_increase(Ind, slot, -1);
	inven_item_describe(Ind, slot);
	inven_item_optimize(Ind, slot);

	real_value = object_value_real(0, o_ptr);

#ifdef AUCTION_MINIMUM_VALUE
	if (real_value < AUCTION_MINIMUM_VALUE)
	{
		/* Return the item */
		inven_carry(Ind, o_ptr);

		auction_clear(auction_id);

		/* Too cheap */
		return AUCTION_ERROR_TOO_CHEAP;
	}
#endif

	if (!auction_parse_money(starting_price_string, &starting_price))
	{
		/* Return the item */
		inven_carry(Ind, o_ptr);

		auction_clear(auction_id);

		/* Too cheap */
		return AUCTION_ERROR_INVALID_PRICE;
	}
	auction_overflow_check(starting_price);

	if (!auction_parse_money(buyout_price_string, &buyout_price))
	{
		/* Return the item */
		inven_carry(Ind, o_ptr);

		auction_clear(auction_id);

		/* Too cheap */
		return AUCTION_ERROR_INVALID_PRICE;
	}
	auction_overflow_check(buyout_price);

	/* Apply restrictions to the prices */

#ifdef AUCTION_MINIMUM_STARTING_PRICE
	if (starting_price && starting_price < AUCTION_MINIMUM_STARTING_PRICE * real_value / 100)
	{
		starting_price = AUCTION_MINIMUM_STARTING_PRICE * real_value / 100;
	}
#endif

#ifdef AUCTION_MAXIMUM_STARTING_PRICE
	if (starting_price > AUCTION_MAXIMUM_STARTING_PRICE * real_value / 100)
	{
		starting_price = AUCTION_MAXIMUM_STARTING_PRICE * real_value / 100;
	}
#endif

#ifdef AUCTION_MINIMUM_BUYOUT_PRICE
	if (buyout_price && buyout_price < AUCTION_MINIMUM_BUYOUT_PRICE * real_value / 100)
	{
		buyout_price = AUCTION_MINIMUM_BUYOUT_PRICE * real_value / 100;
	}
#endif

#ifdef AUCTION_MAXIMUM_BUYOUT_PRICE
	if (buyout_price > AUCTION_MAXIMUM_BUYOUT_PRICE * real_value / 100)
	{
		buyout_price = AUCTION_MAXIMUM_BUYOUT_PRICE * real_value / 100;
	}
#endif

	if (buyout_price && buyout_price < starting_price)
	{
		/* Buyout price is smaller than the starting price */
		/* No buyout */
		buyout_price = 0;
	}

	if (!starting_price && !buyout_price)
	{
		/* Return the item */
		inven_carry(Ind, o_ptr);

		auction_clear(auction_id);

		/* Too cheap */
		return AUCTION_ERROR_EITHER_BID_OR_BUYOUT;		
	}

	if (!auction_parse_time(duration_string, &duration))
	{
		/* Return the item */
		inven_carry(Ind, o_ptr);

		auction_clear(auction_id);

		/* Too cheap */
		return AUCTION_ERROR_INVALID_DURATION;
	}
	auction_overflow_check(duration);

	/* Duration restrictions */

#ifdef AUCTION_MINIMUM_DURATION
	if (duration < AUCTION_MINIMUM_DURATION)
	{
		duration = AUCTION_MINIMUM_DURATION;
	}
#endif

#ifdef AUCTION_MAXIMUM_DURATION
	if (duration > AUCTION_MAXIMUM_DURATION)
	{
		duration = AUCTION_MAXIMUM_DURATION;
	}
#endif

	/* Final check for overflows */
	auction_overflow_check(starting_price);
	auction_overflow_check(buyout_price);
	auction_overflow_check(duration);

	auc_ptr->starting_price = starting_price;
	auc_ptr->buyout_price = buyout_price;
	auc_ptr->start = now;
	auc_ptr->duration = duration;

	/* Identify */
//	object_aware(Ind, o_ptr);
//	object_known(o_ptr);
	/* Hmm.. too exploitable */

	/* Reset the owner of the item */
	o_ptr->owner = 0;

	/* Describe it */
	C_MAKE(auc_ptr->desc, 160, char);
	object_desc(Ind, auc_ptr->desc, o_ptr, TRUE, 3);

	msg_format(Ind, "\377B[@] \377wAuction item #%d:", auction_id);
	msg_format(Ind, "\377B[@]   \377w%s", auc_ptr->desc);
	if (auc_ptr->buyout_price) msg_format(Ind, "\377B[@] \377wBuyout price: %d", auc_ptr->buyout_price);
	if (auc_ptr->starting_price) msg_format(Ind, "\377B[@] \377yStarting price: %d", auc_ptr->starting_price);

	time_string = auction_format_time(auc_ptr->duration);
	msg_format(Ind, "\377B[@] \377wDuration: %s", time_string);
	C_FREE(time_string, 160, char);
	msg_print(Ind, "\377B[@] ");
	msg_print(Ind, "\377B[@] \377wIf you want to start this auction, type \377G/auction start");
	msg_print(Ind, "\377B[@] \377wIf not, type \377G/auction cancel");

	return 0;
}

/* Player confirms that (s)he wants to start the auction */
int auction_start(int Ind)
{
	player_type *p_ptr = Players[Ind];
	auction_type *auc_ptr;

	if (p_ptr->current_auction)
	{
		auc_ptr = &auctions[p_ptr->current_auction];
		if (auc_ptr->status == AUCTION_STATUS_SETUP)
		{
			auc_ptr->status = AUCTION_STATUS_BIDDING;

			/* Log it */
			s_printf("AUCTION: %s started auction for item #%d:\n", p_ptr->name, p_ptr->current_auction);
			s_printf("AUCTION:   %s\n", auc_ptr->desc);

			msg_format(Ind, "\377B[@] \377GAuction #%d has been started!", p_ptr->current_auction);

			p_ptr->current_auction = 0;
		}
		else
		{
			/* Cannot start */
			return AUCTION_ERROR_ALREADY_STARTED;
		}
	}
	else
	{
		/* No auction to be started */
		return AUCTION_ERROR_INVALID_ID;
	}

	return 0;
}

/* Multi-purpose cancel function */
int auction_cancel(int Ind, int auction_id)
{
	player_type *p_ptr = Players[Ind];
	auction_type *auc_ptr;
	bid_type *bid_ptr;
	int i, j;

	if (auction_id <= 0 || auction_id >= auction_alloc)
	{
		/* Invalid id */
		return AUCTION_ERROR_INVALID_ID;
	}

	auc_ptr = &auctions[auction_id];

	switch (auc_ptr->status)
	{
		case AUCTION_STATUS_SETUP:
			if (auc_ptr->owner == p_ptr->id)
			{
				auc_ptr->status = AUCTION_STATUS_CANCELLED;
				msg_print(Ind, "\377B[@] \377GAuction has been cancelled!");
				msg_print(Ind, "\377B[@] \377wYou can retrieve the item by typing \377G/auction retrieve");
				p_ptr->current_auction = 0;
			}
			else
			{
				/* Not owner, wrong id */
				return AUCTION_ERROR_INVALID_ID;
			}
			break;
		case AUCTION_STATUS_BIDDING:
#ifdef AUCTION_ALLOW_CANCEL_OWN_ITEM
/* Not allowed by default */	
			if (auc_ptr->owner == p_ptr->id)
			{
				auc_ptr->status = AUCTION_STATUS_CANCELLED;

				/* Log it */
				s_printf("AUCTION: %s has cancelled bidding for his/her item #%d:\n", p_ptr->name, auction_id);
				s_printf("AUCTION:   %s\n", auc_ptr->desc);

				msg_format(Ind, "\377B[@] \377GYou have cancelled bidding for your item #%d.", auction_id);
				msg_format(Ind, "\377B[@]  \377w%s", auc_ptr->desc);

				/* Notify bidders */
				for (i = 0; i < auc_ptr->bids_cnt; i++)
				{
					bid_ptr = &auc_ptr->bids[i];
					for (j = 1; j <= NumPlayers; j++)
					{
						if (Players[j]->id == bid_ptr->bidder)
						{
							msg_format(j, "\377B[@] \377w%^s has cancelled bidding for item #%d.", p_ptr->name, auction_id);
							msg_format(j, "\377B[@]   \377w%s", auc_ptr->desc);
						}
					}
				}
				C_KILL(auc_ptr->bids, auc_ptr->bids_cnt, bid_type);
				auc_ptr->bids_cnt = 0;
			}
			else
#endif
			if (is_admin(p_ptr))
			{
				auc_ptr->status = AUCTION_STATUS_CANCELLED;

				/* Log it */
				s_printf("AUCTION: Admin (%s) cancelled bidding for item #%d (owned by %s):\n", p_ptr->name, auction_id, lookup_player_name(auc_ptr->owner));
				s_printf("AUCTION:   %s\n", auc_ptr->desc);

				msg_format(Ind, "\377B[@] \377GYou have cancelled bidding for item #%d.", auction_id);
				msg_format(Ind, "\377B[@]  \377w%s", auc_ptr->desc);

				for (i = 1; i <= NumPlayers; i++)
				{
					if (Players[i]->id == auc_ptr->owner)
					{
						msg_format(i, "\377B[@] \377wBidding for item #%d has been cancelled by the authorities.", auction_id);
						msg_format(i, "\377B[@]   \377w%s", auc_ptr->desc);
					}
				}

				/* Notify bidders */
				for (i = 0; i < auc_ptr->bids_cnt; i++)
				{
					bid_ptr = &auc_ptr->bids[i];
					for (j = 1; j <= NumPlayers; j++)
					{
						if (Players[j]->id == bid_ptr->bidder)
						{
							msg_format(j, "\377B[@] \377wAuthorities have cancelled bidding for item #%d.", auction_id);
							msg_format(j, "\377B[@]   \377w%s", auc_ptr->desc);
						}
					}
				}
				C_KILL(auc_ptr->bids, auc_ptr->bids_cnt, bid_type);
				auc_ptr->bids_cnt = 0;
			}
			else
			{
				/* Lastly: Try to remove any bid (s)he may have placed */
				for (i = 0; i < auc_ptr->bids_cnt; i++)
				{
					bid_ptr = &auc_ptr->bids[i];
					if (bid_ptr->bidder == p_ptr->id)
					{
						msg_format(Ind, "\377B[@] \377GYour bid of %d on item #%d has been cancelled.", bid_ptr->bid, auction_id);
						msg_format(Ind, "\377B[@]  \377w%s", auc_ptr->desc);

						/* Remove bid */
						auction_remove_bid(auction_id, i);

						return 0;
					}
				}

				/* No bid found, (s)he cannot do anything */
				return AUCTION_ERROR_NOT_SUPPORTED;
			}
			break;
		default:
			/* Not supported */
			return AUCTION_ERROR_NOT_SUPPORTED;
	}

	return 0;
}

int auction_place_bid(int Ind, int auction_id, cptr bid_string)
{
	player_type *p_ptr = Players[Ind];
#ifndef RPG_SERVER
	object_type *o_ptr;
#endif
	auction_type *auc_ptr;
	bid_type *bid_ptr;
	s32b bid;
	int i;

	if (auction_id <= 0 || auction_id >= auction_alloc)
	{
		/* Invalid id */
		return AUCTION_ERROR_INVALID_ID;
	}

	if (p_ptr->inval)
	{
		/* Invalid account */
		return AUCTION_ERROR_INVALID_ACCOUNT;
	}

	auc_ptr = &auctions[auction_id];

	if (auc_ptr->status != AUCTION_STATUS_BIDDING)
	{
		/* Not in bidding phase */
		return AUCTION_ERROR_NOT_BIDDING;
	}

	if (auc_ptr->owner == p_ptr->id || lookup_player_account(auc_ptr->owner) == p_ptr->account)
	{
		/* Own item, go cancel it */
		return AUCTION_ERROR_OWN_ITEM;
	}

	if (!auc_ptr->starting_price)
	{
		/* Not open for bidding */
		return AUCTION_ERROR_NO_BIDDING;
	}

	if (!auction_mode_check(Ind, auction_id))
	{
		/* Non-everlasting / everlasting mode restrictions */
		if (p_ptr->mode & MODE_EVERLASTING) return AUCTION_ERROR_NONEVERLASTING_ITEM;
		else return AUCTION_ERROR_EVERLASTING_ITEM;
	}

#ifndef RPG_SERVER
	o_ptr = &auc_ptr->item;

	if (o_ptr->level > p_ptr->lev)
	{
		/* The player's level is too low */
		return AUCTION_ERROR_INSUFFICIENT_LEVEL;
	}
#endif

	if (!auction_parse_money(bid_string, &bid))
	{
		/* Invalid price */
		return AUCTION_ERROR_INVALID_PRICE;
	}
	if (bid < 0)
	{
		/* Overflow */
		return AUCTION_ERROR_OVERFLOW;
	}

	if (bid < auc_ptr->starting_price)
	{
		/* Bid is too small */
		return AUCTION_ERROR_TOO_SMALL;
	}

	if (auc_ptr->buyout_price && bid >= auc_ptr->buyout_price)
	{
		/* Buy-out */
		return auction_buyout(Ind, auction_id);
	}

	for (i = 0; i < auc_ptr->bids_cnt; i++)
	{
		bid_ptr = &auc_ptr->bids[i];
		if (bid_ptr->bidder == p_ptr->id)
		{
			/* Remove old bid */
			auction_remove_bid(auction_id, i);
		}
	}

	auction_add_bid(auction_id, bid, p_ptr->id);

	msg_format(Ind, "\377B[@] \377GYour bid of %d on item #%d has been placed.", bid, auction_id);
	msg_format(Ind, "\377B[@]  \377w%s", auc_ptr->desc);

	return 0;
}

int auction_buyout(int Ind, int auction_id)
{
	player_type *p_ptr = Players[Ind];
#ifndef RPG_SERVER
	object_type *o_ptr;
#endif
	s32b total_money = p_ptr->au + p_ptr->balance;
	auction_type *auc_ptr;
	int i;
	player_type *q_ptr;

	if (auction_id <= 0 || auction_id >= auction_alloc)
	{
		/* Invalid id */
		return AUCTION_ERROR_INVALID_ID;
	}

	if (p_ptr->inval)
	{
		/* Invalid account */
		return AUCTION_ERROR_INVALID_ACCOUNT;
	}

	auc_ptr = &auctions[auction_id];

	if (auc_ptr->status != AUCTION_STATUS_BIDDING)
	{
		/* Not in bidding phase */
		return AUCTION_ERROR_NOT_BIDDING;
	}

	if (auc_ptr->owner == p_ptr->id || lookup_player_account(auc_ptr->owner) == p_ptr->account)
	{
		/* Own item, go cancel it */
		return AUCTION_ERROR_OWN_ITEM;
	}

	if (!auc_ptr->buyout_price)
	{
		/* Buyout isn't allowed */
		return AUCTION_ERROR_NO_BUYOUT;
	}

	if (!auction_mode_check(Ind, auction_id))
	{
		/* Non-everlasting / everlasting mode restrictions */
		if (p_ptr->mode & MODE_EVERLASTING) return AUCTION_ERROR_NONEVERLASTING_ITEM;
		else return AUCTION_ERROR_EVERLASTING_ITEM;
	}

#ifndef RPG_SERVER
	o_ptr = &auc_ptr->item;

	if (o_ptr->level > p_ptr->lev)
	{
		/* The player's level is too low */
		return AUCTION_ERROR_INSUFFICIENT_LEVEL;
	}
#endif

	if (total_money < auc_ptr->buyout_price)
	{
		/* Can't afford it */
		return AUCTION_ERROR_INSUFFICIENT_MONEY;
	}

	/* Use cash from the bank first */
	if (p_ptr->balance >= auc_ptr->buyout_price)
	{
		p_ptr->balance -= auc_ptr->buyout_price;
	}
	else
	{
		p_ptr->au -= auc_ptr->buyout_price - p_ptr->balance;
		p_ptr->balance = 0;
	}

	/* Add it to bids so (s)he can retrieve the item */
	auction_add_bid(auction_id, auc_ptr->buyout_price, p_ptr->id);
	auc_ptr->winning_bid = auc_ptr->bids_cnt - 1;
	auc_ptr->status = AUCTION_STATUS_FINISHED;
	auc_ptr->flags |= AUCTION_FLAG_WINNER_PAID;

	/* Log it */
	s_printf("AUCTION: %s has bought-out item #%d from %s for %d:\n", p_ptr->name, auction_id, lookup_player_name(auc_ptr->owner), auc_ptr->buyout_price);
	s_printf("AUCTION:   %s\n", auc_ptr->desc);

	if (auc_ptr->owner > 0)
	{
		/* See if the seller is online */
		for (i = 1; 1 <= NumPlayers; i++)
		{
			if (Players[i]->id == auc_ptr->owner)
			{
				s32b amount = auc_ptr->buyout_price - auc_ptr->buyout_price * AUCTION_FEE / 100;
				q_ptr = Players[i];

				q_ptr->balance += amount;
				auc_ptr->flags |= AUCTION_FLAG_SELLER_PAID;

				msg_format(i, "\377B[@] \377G%^s has bought the following item #%d for %d", p_ptr->name, auction_id, auc_ptr->buyout_price);
				msg_format(i, "\377B[@]   \377w%s", auc_ptr->desc);
				msg_format(i, "\377B[@] \377GYou received %d.", amount);
				break;
			}
		}
	}

	msg_format(Ind, "\377B[@] \377GYou have bought auction item #%d for %d:", auction_id, auc_ptr->buyout_price);
	msg_format(Ind, "\377B[@]   \377w%s", auc_ptr->desc);

	return 0;
}

void auction_list_print_item(int Ind, int auction_id)
{
	auction_type *auc_ptr = &auctions[auction_id];

	msg_format(Ind, "\377B[@] %d: %s", auction_id, auc_ptr->desc);
	if (auc_ptr->bids_cnt)
	{
		msg_format(Ind, "\377B[@] - Highest bid: %8d    - Buyout price: %8d", auc_ptr->bids[auc_ptr->bids_cnt - 1], auc_ptr->buyout_price);
	}
	else
	{
		msg_format(Ind, "\377B[@] - Starting price: %8d - Buyout price: %8d", auc_ptr->starting_price, auc_ptr->buyout_price);
	}
}

/* Simply list all items (for now) */
void auction_list(int Ind)
{
	auction_type *auc_ptr;
	int i;

	int printed = 0;

	msg_print(Ind, "\377B[@] \377GAuctions in progress:");

	for (i = 1; i < auction_alloc; i++)
	{
		auc_ptr = &auctions[i];

		if ((auc_ptr->status == AUCTION_STATUS_BIDDING) && auction_mode_check(Ind, i))
		{
			auction_list_print_item(Ind, i);
			printed++;
		}
	}

	if (!printed) msg_print(Ind, "\377B[@]  \377w<none>");
}

#if 0 /* common.c has an implementation already, since this one is currently TEST_SERVER only */
cptr my_strcasestr(cptr haystack, cptr needle)
{
	int i = 0, len;

	len = strlen(needle);

	while (haystack[i])
	{
		if (!strncasecmp(haystack + i, needle, len))
		{
			return haystack + i;
		}
		i++;
	}

	return NULL;
}
#endif

void auction_search(int Ind, cptr search)
{
	auction_type *auc_ptr;
	int i;
	int found = 0;

	msg_format(Ind, "\377B[@] \377GAuction items containing \"%s\"", search);

	for (i = 1; i < auction_alloc; i++)
	{
		auc_ptr = &auctions[i];

		if ((auc_ptr->status == AUCTION_STATUS_BIDDING) && auction_mode_check(Ind, i))
		{
			if (my_strcasestr(auc_ptr->desc, search))
			{
				auction_list_print_item(Ind, i);
				found++;
			}
		}
	}

	if (!found) msg_print(Ind, "\377B[@]  \377w<none>");
}

/* Retrieve won items and items from cancelled auctions */
void auction_retrieve_items(int Ind, int *retrieved, int *unretrieved)
{
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	auction_type *auc_ptr;
	bid_type *bid_ptr;
	int i;

	*retrieved = *unretrieved = 0;

	for (i = 1; i < auction_alloc; i++)
	{
		auc_ptr = &auctions[i];
		switch (auc_ptr->status)
		{
			case AUCTION_STATUS_FINISHED:
				bid_ptr = &auc_ptr->bids[auc_ptr->winning_bid];
				/* Check if the item belongs to this player now */
				if (bid_ptr->bidder == p_ptr->id)
				{
					o_ptr = &auc_ptr->item;
					if (inven_carry_okay(Ind, o_ptr, 0x0))
					{
						o_ptr->owner = p_ptr->id;
						o_ptr->mode = p_ptr->mode;
						inven_carry(Ind, o_ptr);
						(*retrieved)++;
						WIPE(o_ptr, object_type);

						/* Check if we're done with this auction now */
						if ((auc_ptr->flags & AUCTION_FLAG_WINNER_PAID) && (auc_ptr->flags & AUCTION_FLAG_SELLER_PAID))
						{
							auction_clear(i);
						}
					}
					else
					{
						(*unretrieved)++;
					}
				}
				break;
			case AUCTION_STATUS_CANCELLED:
				if (auc_ptr->owner == p_ptr->id)
				{
					o_ptr = &auc_ptr->item;
					if (inven_carry_okay(Ind, o_ptr, 0x0))
					{
						o_ptr->owner = p_ptr->id;
						inven_carry(Ind, o_ptr);
						(*retrieved)++;
						auction_clear(i);
					}
				}
				else
				{
					(*unretrieved)++;
				}
				break;
		}
	}
}

/* Show some information about the auction */
int auction_show(int Ind, int auction_id)
{
	int i;
	auction_type *auc_ptr;
	bid_type *bid_ptr;
	time_t now = time(NULL);
	char *time_string;

	if (auction_id <= 0 || auction_id >= auction_alloc)
	{
		/* Invalid id */
		return AUCTION_ERROR_INVALID_ID;
	}

	if (auctions[auction_id].status == AUCTION_STATUS_EMPTY)
	{
		/* Invalid id */
		return AUCTION_ERROR_INVALID_ID;
	}

	auc_ptr = &auctions[auction_id];

	msg_format(Ind, "\377B[@] \377wAuction item #%d from %s:", auction_id, lookup_player_name(auc_ptr->owner));
	msg_format(Ind, "\377B[@]   \377w%s", auc_ptr->desc);
	if (auc_ptr->buyout_price) msg_format(Ind, "\377B[@] \377wBuyout price: %d", auc_ptr->buyout_price);
	if (auc_ptr->starting_price)
	{
		if (auc_ptr->bids_cnt)
		{
			msg_print(Ind, "\377B[@] \377GBids:");
			for (i = auc_ptr->bids_cnt - 1; i >= 0; i--)
			{
				bid_ptr = &auc_ptr->bids[i];
				msg_format(Ind, "\377B[@] \377w%d by %s", bid_ptr->bid, lookup_player_name(bid_ptr->bidder));
			}
		}
		else
		{
			msg_print(Ind, "\377B[@] \377rNo bids.");
		}
		msg_format(Ind, "\377B[@] \377yStarting price: %d", auc_ptr->starting_price);
	}

	time_string = auction_format_time(auc_ptr->duration - (now - auc_ptr->start));
	msg_format(Ind, "\377B[@] \377wTime left: %s", time_string);
	C_FREE(time_string, 160, char);

	return 0;
}

/* Examine the item */
int auction_examine(int Ind, int auction_id)
{
	object_type *o_ptr;
	auction_type *auc_ptr;

	if (auction_id <= 0 || auction_id >= auction_alloc)
	{
		/* Invalid id */
		return AUCTION_ERROR_INVALID_ID;
	}

	if (auctions[auction_id].status == AUCTION_STATUS_EMPTY)
	{
		/* Invalid id */
		return AUCTION_ERROR_INVALID_ID;
	}

	auc_ptr = &auctions[auction_id];
	o_ptr = &auc_ptr->item;

	/* Check if it has been *identified* */
	if (!(o_ptr->ident & ID_MENTAL))
		msg_print(Ind, "No information available.");

	/* Describe it fully */
	if (!identify_fully_aux(Ind, o_ptr, FALSE, -1))
		msg_print(Ind, "You see nothing special.");

	return 0;
}

#endif
