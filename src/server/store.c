/* $Id$ */
/* File: store.c */

/* Purpose: Store commands */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"

/* Minimal benefit a store should obtain via transaction,
 * in percent.	[10] */
#define STORE_BENEFIT	10

/* Extra check for ego item generation in store, in percent.	[50] */
#define STORE_EGO_CHANCE	50

static int gettown(int Ind);

void home_sell(int Ind, int item, int amt);
void home_purchase(int Ind, int item, int amt);
void home_examine(int Ind, int item);
static void display_house_entry(int Ind, int pos);
static void display_house_inventory(int Ind);
static void display_trad_house(int Ind);
void home_extend(int Ind);


#define MAX_COMMENT_1	6

static cptr comment_1[MAX_COMMENT_1] =
{
	"Okay.",
	"Fine.",
	"Accepted!",
	"Agreed!",
	"Done!",
	"Taken!"
};

#if 0

#define MAX_COMMENT_2A	2

static cptr comment_2a[MAX_COMMENT_2A] =
{
	"You try my patience.  %s is final.",
	"My patience grows thin.  %s is final."
};

#define MAX_COMMENT_2B	12

static cptr comment_2b[MAX_COMMENT_2B] =
{
	"I can take no less than %s gold pieces.",
	"I will accept no less than %s gold pieces.",
	"Ha!  No less than %s gold pieces.",
	"You knave!  No less than %s gold pieces.",
	"That's a pittance!  I want %s gold pieces.",
	"That's an insult!  I want %s gold pieces.",
	"As if!  How about %s gold pieces?",
	"My arse!  How about %s gold pieces?",
	"May the fleas of 1000 orcs molest you!  Try %s gold pieces.",
	"May your most favourite parts go moldy!  Try %s gold pieces.",
	"May Morgoth find you tasty!  Perhaps %s gold pieces?",
	"Your mother was an Ogre!  Perhaps %s gold pieces?"
};

#define MAX_COMMENT_3A	2

static cptr comment_3a[MAX_COMMENT_3A] =
{
	"You try my patience.  %s is final.",
	"My patience grows thin.  %s is final."
};


#define MAX_COMMENT_3B	12

static cptr comment_3b[MAX_COMMENT_3B] =
{
	"Perhaps %s gold pieces?",
	"How about %s gold pieces?",
	"I will pay no more than %s gold pieces.",
	"I can afford no more than %s gold pieces.",
	"Be reasonable.  How about %s gold pieces?",
	"I'll buy it as scrap for %s gold pieces.",
	"That is too much!  How about %s gold pieces?",
	"That looks war surplus!  Say %s gold pieces?",
	"Never!  %s is more like it.",
	"That's an insult!  %s is more like it.",
	"%s gold pieces and be thankful for it!",
	"%s gold pieces and not a copper more!"
};

#define MAX_COMMENT_4A	4

static cptr comment_4a[MAX_COMMENT_4A] =
{
	"Enough!  You have abused me once too often!",
	"Arghhh!  I have had enough abuse for one day!",
	"That does it!  You shall waste my time no more!",
	"This is getting nowhere!  I'm going to Londis!"
};

#define MAX_COMMENT_4B	4

static cptr comment_4b[MAX_COMMENT_4B] =
{
	"Leave my store!",
	"Get out of my sight!",
	"Begone, you scoundrel!",
	"Out, out, out!"
};

#define MAX_COMMENT_5	8

static cptr comment_5[MAX_COMMENT_5] =
{
	"Try again.",
	"Ridiculous!",
	"You will have to do better than that!",
	"Do you wish to do business or not?",
	"You've got to be kidding!",
	"You'd better be kidding!",
	"You try my patience.",
	"Hmmm, nice weather we're having."
};

#define MAX_COMMENT_6	4

static cptr comment_6[MAX_COMMENT_6] =
{
	"I must have heard you wrong.",
	"I'm sorry, I missed that.",
	"I'm sorry, what was that?",
	"Sorry, what was that again?"
};

#endif


/*
 * Successful haggle.
 */
static void say_comment_1(int Ind)
{
	msg_print(Ind, comment_1[rand_int(MAX_COMMENT_1)]);
}

/*
 * We store the current "store number" here so everyone can access it
 */


/*
 * We store the current "store page" here so everyone can access it
 */
/*static int store_top = 0;*/

/*
 * We store the current "store pointer" here so everyone can access it
 */
/*static store_type *st_ptr = NULL;*/

/*
 * We store the current "owner type" here so everyone can access it
 */
/*static owner_type *ot_ptr = NULL;*/

void alloc_stores(int townval)
{
	int i;

	/* Allocate the stores */
	/* XXX of course, it's inefficient;
	 * we should check which town has what stores
	 */
	C_MAKE(town[townval].townstore, max_st_idx, store_type);

	/* Fill in each store */
	for (i = 0; i < max_st_idx; i++)
	{       
		/* Access the store */
		store_type *st_ptr = &town[townval].townstore[i];
		store_info_type *sti_ptr = &st_info[i];

		/* sett store type */
		st_ptr->st_idx = i;

		/* Assume full stock */
		st_ptr->stock_size = sti_ptr->max_obj;

		/* Allocate the stock */
		C_MAKE(st_ptr->stock, st_ptr->stock_size, object_type);
	}
}

/*
 * Determine the price of an item (qty one) in a store.
 *
 * This function takes into account the player's charisma, and the
 * shop-keepers friendliness, and the shop-keeper's base greed, but
 * never lets a shop-keeper lose money in a transaction.
 *
 * The "greed" value should exceed 100 when the player is "buying" the
 * item, and should be less than 100 when the player is "selling" it.
 *
 * Hack -- the black market always charges twice as much as it should.
 *
 * Charisma adjustment runs from 80 to 130
 * Racial adjustment runs from 95 to 130
 *
 * Since greed/charisma/racial adjustments are centered at 100, we need
 * to adjust (by 200) to extract a usable multiplier.  Note that the
 * "greed" value is always something (?).
 */
static s32b price_item(int Ind, object_type *o_ptr, int greed, bool flip)
{
	player_type *p_ptr = Players[Ind];
	owner_type *ot_ptr;
	store_type *st_ptr;
	int     factor;
	int     adjust;
	/*s32b    price;*/
	s64b    price;
	int i;

	i=gettown(Ind);
	if(i==-1) i = 0;

	st_ptr = &town[i].townstore[p_ptr->store_num];
	ot_ptr = &ow_info[st_ptr->owner];

	/* Get the value of one of the items */
	price = object_value(flip ? Ind : 0, o_ptr);

	/* Worthless items */
	if (price <= 0) return (0L);

	/* Compute the racial factor */
	if (is_state(Ind, st_ptr, STORE_LIKED))
	{
		factor = ot_ptr->costs[STORE_LIKED];
	}
	else if (is_state(Ind, st_ptr, STORE_HATED))
	{
		factor = ot_ptr->costs[STORE_HATED];
	}
	else
	{
		factor = ot_ptr->costs[STORE_NORMAL];
	}

	/* Add in the charisma factor */
	factor += adj_chr_gold[p_ptr->stat_ind[A_CHR]];


	/* Shop is buying */
	if (flip)
	{
		/* Adjust for greed */
		adjust = 100 + (300 - (greed + factor));

		/* Hack -- Shopkeepers hate higher level-requirement */
		adjust += (20 - o_ptr->level) / 2 ;

		/* Never get "silly" */
		if (adjust > 100 - STORE_BENEFIT) adjust = 100 - STORE_BENEFIT;

		/* Mega-Hack -- Black market sucks */
		if (st_info[st_ptr->st_idx].flags1 & SF1_ALL_ITEM) price /= 4;

		/* You're not a welcomed customer.. */
		if (p_ptr->tim_blacklist) price = price / 4;

		/* To prevent cheezing */
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)){
			if(o_ptr->pval==0)
			{
//				o_ptr->level = 1; //= 0
				price = 1000; //= 100
			}
			else
			{
/*				if (r_info[o_ptr->pval].level > 0) {
					o_ptr->level = 15 + (1000 / ((2000 / r_info[o_ptr->pval].level) + 10));
				} else {
					o_ptr->level = 15;
				}
*/				price = 1000;
			}
		}
	}

	/* Shop is selling */
	else
	{
		/* Adjust for greed */
		adjust = 100 + ((greed + factor) - 300);

		/* Never get "silly" */
		if (adjust < 100 + STORE_BENEFIT) adjust = 100 + STORE_BENEFIT;

		/* some shops are extra expensive */
		if (st_info[st_ptr->st_idx].flags1 & SF1_PRICE16) price *= 16;
		if (st_info[st_ptr->st_idx].flags1 & SF1_PRICE4) price *= 4;
		if (st_info[st_ptr->st_idx].flags1 & SF1_PRICE2) price *= 2;
		if (st_info[st_ptr->st_idx].flags1 & SF1_PRICE1) price = (price * 3) / 2;

		/* You're not a welcomed customer.. */
		if (p_ptr->tim_blacklist) price = price * 4;

	}

	/* Compute the final price (with rounding) */
	price = (price * adjust + 50L) / 100L;

	/* Note -- Never become "free" */
	if (price <= 0L) return (1L);

	/* Return the price */
	return (price);
}


/*
 * Special "mass production" computation
 */
static int mass_roll(int num, int max)
{
	int i, t = 0;
	for (i = 0; i < num; i++) t += rand_int(max);
	return (t);
}


/*
 * Certain "cheap" objects should be created in "piles"
 * Some objects can be sold at a "discount" (in small piles)
 */
static void mass_produce(object_type *o_ptr, store_type *st_ptr)
{
	int size = 1;
	int discount = 0;

	s32b cost = object_value(0, o_ptr);


	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Food, Flasks, and Lites */
		case TV_FOOD:
		case TV_FLASK:
		case TV_LITE:
		case TV_PARCHEMENT:
		{
			if (cost <= 5L) size += mass_roll(3, 5);
			if (cost <= 20L) size += mass_roll(3, 5);
			break;
		}

		case TV_POTION:
		case TV_SCROLL:
		{
			if (cost <= 60L) size += mass_roll(3, 5);
			if (cost <= 240L) size += mass_roll(1, 5);
			if (st_ptr->st_idx == STORE_BTSUPPLY && o_ptr->sval == SV_POTION_STAR_HEALING) size += mass_roll(3, 5);
			break;
		}

		case TV_BOOK:
		{
			if (cost <= 50L) size += mass_roll(2, 3);
			if (cost <= 500L) size += mass_roll(1, 3);
			break;
		}

		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SHIELD:
		case TV_GLOVES:
		case TV_BOOTS:
		case TV_CLOAK:
		case TV_HELM:
		case TV_CROWN:
		case TV_SWORD:
		case TV_POLEARM:
		case TV_HAFTED:
		case TV_DIGGING:
		case TV_BOW:
		case TV_BOOMERANG:
		case TV_MSTAFF:
		case TV_AXE:
		case TV_TRAPKIT:
		case TV_INSTRUMENT:
		case TV_ROD: case TV_ROD_MAIN: case TV_STAFF: case TV_WAND:
		{
			/* No ego-stacks */
			if (o_ptr->name2) break;

			if (cost <= 10L) size += mass_roll(3, 5);
			if (cost <= 100L) size += mass_roll(3, 5);
			break;
		}
		case TV_DRAG_ARMOR:
			/* Only single items of these */
			break;

		case TV_SPIKE:
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		{
			if (cost <= 10L) size += damroll(10, 2);
			if (cost <= 100L) size += damroll(5, 3);
			size += damroll(20, 2);
			break;
		}
	}


	/* Pick a discount */
	if (cost < 5)
	{
		discount = 0;
	}
	else if (rand_int(25) == 0 && cost > 9)
	{
		discount = 10;
	}
	else if (rand_int(50) == 0)
	{
		discount = 25;
	}
	else if (rand_int(150) == 0)
	{
		discount = 50;
	}
	else if (rand_int(300) == 0)
	{
		discount = 75;
	}
	else if (rand_int(500) == 0)
	{
		discount = 90;
	}


	/* Save the discount */
	o_ptr->discount = discount;

	/* Save the total pile size */
	o_ptr->number = size - (size * discount / 100);
}

/*
 * Determine if a store item can "absorb" another item
 *
 * See "object_similar()" for the same function for the "player"
 */
/* o_ptr is one in store, j_ptr is one in new. */
static bool store_object_similar(object_type *o_ptr, object_type *j_ptr)
{
	/* Hack -- Identical items cannot be stacked */
	if (o_ptr == j_ptr) return (0);

	/* Different objects cannot be stacked */
	if (o_ptr->k_idx != j_ptr->k_idx) return (0);

	/* Different modes cannot be stacked */
	if (((o_ptr->owner_mode & MODE_IMMORTAL) && !(j_ptr->owner_mode & MODE_IMMORTAL)) ||
	    (!(o_ptr->owner_mode & MODE_IMMORTAL) && (j_ptr->owner_mode & MODE_IMMORTAL)))
		return (0);

	/* Different charges (etc) cannot be stacked */
	if (o_ptr->pval != j_ptr->pval && o_ptr->tval != TV_WAND) return (0);
	if (o_ptr->bpval != j_ptr->bpval) return (0);

	/* Require many identical values */
	if (o_ptr->to_h  !=  j_ptr->to_h) return (0);
	if (o_ptr->to_d  !=  j_ptr->to_d) return (0);
	if (o_ptr->to_a  !=  j_ptr->to_a) return (0);

	/* Require identical "artifact" names */
	/* Bad idea, randart ammo is stacked easily. (Rand)arts just
	   shouldn't stack at all - C. Blue */
#if 0
	if (o_ptr->name1 != j_ptr->name1) return (0);
#else
	if (o_ptr->name1 || j_ptr->name1) return (0);
#endif

	/* Require identical "ego-item" names */
	if (o_ptr->name2 != j_ptr->name2) return (0);
	if (o_ptr->name2b != j_ptr->name2b) return (0);

	/* Hack -- Never stack "powerful" items */
	if (o_ptr->xtra1 || j_ptr->xtra1) return (0);

	/* Hack -- Never stack recharging items */
	/* Megahack -- light sources are allowed (hoping it's
	 * not non-fuel activable one..) */
	if (o_ptr->timeout != j_ptr->timeout) return (FALSE);

	/* Require many identical values */
	if (o_ptr->ac    !=  j_ptr->ac)   return (0);
	if (o_ptr->dd    !=  j_ptr->dd)   return (0);
	if (o_ptr->ds    !=  j_ptr->ds)   return (0);

	/* Hack -- Never stack chests */
	if (o_ptr->tval == TV_CHEST) return (0);

	/* Require matching discounts */
	if (o_ptr->discount != j_ptr->discount) return (0);

	/* They match, so they must be similar */
	return (TRUE);
}


/*
 * Allow a store item to absorb another item
 */
/* o_ptr is item in store, j_ptr is new item bought. */
static void store_object_absorb(object_type *o_ptr, object_type *j_ptr)
{
	int total = o_ptr->number + j_ptr->number;

	if (!j_ptr->owner) o_ptr->owner = 0;
	if (j_ptr->level < o_ptr->level) o_ptr->level = j_ptr->level;
	/* I don't know why this is needed, it's bad for Nether Realm store though 
	if (o_ptr->level < 1) o_ptr->level = 1; -C. Blue */

	/* Combine quantity, lose excess items */
	o_ptr->number = (total > 99) ? 99 : total;

	/* Hack -- combine wands' charge */
	if (o_ptr->tval == TV_WAND) o_ptr->pval += j_ptr->pval;
}


/*
 * Check to see if the shop will be carrying too many objects	-RAK-
 * Note that the shop, just like a player, will not accept things
 * it cannot hold.  Before, one could "nuke" potions this way.
 */
static bool store_check_num(store_type *st_ptr, object_type *o_ptr)
{
	int        i;
	object_type *j_ptr;

	/* Free space is always usable */
	if (st_ptr->stock_num < st_ptr->stock_size) return TRUE;

	/* Normal stores do special stuff */
	else
	{
		/* Check all the items */
		for (i = 0; i < st_ptr->stock_num; i++)
		{
			/* Get the existing item */
			j_ptr = &st_ptr->stock[i];

			/* Can the new object be combined with the old one? */
			if (store_object_similar(j_ptr, o_ptr)) return (TRUE);
		}
	}

	/* But there was no room at the inn... */
	return (FALSE);
}




/*
 * Determine if the current store will purchase the given item
 *
 * Note that a shop-keeper must refuse to buy "worthless" items
 */
static bool store_will_buy(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];

	/* Switch on the store */
	switch (p_ptr->store_num)
	{
		/* General Store */
		case 0:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_FOOD:
				case TV_LITE:
				case TV_FLASK:
				case TV_SPIKE:
				case TV_SHOT:
				case TV_ARROW:
				case TV_BOLT:
				case TV_DIGGING:
				case TV_CLOAK:
				case TV_TOOL:
				case TV_PARCHEMENT:
				/* and now new... :) */
				case TV_TRAPKIT:
				break;
				default:
				return (FALSE);
			}
			break;
		}

		/* Armoury */
		case 1:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_BOOTS:
				case TV_GLOVES:
				case TV_CROWN:
				case TV_HELM:
				case TV_SHIELD:
				case TV_CLOAK:
				case TV_SOFT_ARMOR:
				case TV_HARD_ARMOR:
				case TV_DRAG_ARMOR:
				break;
				default:
				return (FALSE);
			}
			break;
		}

		/* Weapon Shop */
		case 2:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_SHOT:
				case TV_BOLT:
				case TV_ARROW:
				case TV_BOW:
				case TV_DIGGING:
				case TV_HAFTED:
				case TV_POLEARM:
				case TV_SWORD:
				case TV_AXE:
				case TV_MSTAFF:
				case TV_BOOMERANG:
				break;
				default:
				return (FALSE);
			}
			break;
		}

		/* Temple */
		case 3:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_SCROLL:
				case TV_POTION:
				case TV_POTION2:
				case TV_HAFTED:
				break;
				default:
				return (FALSE);
			}
			break;
		}

		/* Alchemist */
		case 4:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_SCROLL:
				case TV_POTION:
				case TV_POTION2:
				break;
				default:
				return (FALSE);
			}
			break;
		}

		/* Magic Shop */
		case 5:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_BOOK:
				case TV_AMULET:
				case TV_RING:
				case TV_STAFF:
				case TV_WAND:
				case TV_ROD:
				case TV_SCROLL:
				case TV_POTION:
				case TV_POTION2:
				case TV_MSTAFF:	/* naturally? */
				break;
				default:
				return (FALSE);
			}
			break;
		}
		/* Bookstore Shop */
		case 8:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_BOOK:
#if 0
				case TV_SYMBIOTIC_BOOK:
				case TV_MUSIC_BOOK:
				case TV_DAEMON_BOOK:
				case TV_DRUID_BOOK:
#endif	/* 0 */
					break;
				default:
					return (FALSE);
			}
			break;
		}
		/* Pet Shop */
		case 9:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_EGG:
					break;
				default:
					return (FALSE);
			}
			break;
		}
		/* Rare Footwear Shop */
		case 45:
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_BOOTS:
				break;
				default:
				return (FALSE);
			}
			break;
		/* Rare Jewellry Shop */
		case 42:
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_AMULET:
				case TV_RING:
				break;
				default:
				return (FALSE);
			}
			break;
		/* Mining Supply Store */
		case 59:
			switch (o_ptr->tval)
			{
				case TV_DIGGING:
				case TV_LITE:
				case TV_FLASK:
				case TV_WAND: if (o_ptr->sval != SV_WAND_STONE_TO_MUD) return(FALSE);
				case TV_POTION: if (o_ptr->sval != SV_POTION_DETONATIONS) return(FALSE);
				break;
				default:
				return (FALSE);
			}
		/* Bordertravel supplies */
		case STORE_BTSUPPLY:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_POTION:
				case TV_POTION2:
				case TV_SCROLL:
				case TV_FOOD:
				case TV_LITE:
				case TV_FLASK:
				case TV_TRAPKIT:
				break;
				case TV_RING:
					if ((o_ptr->sval != SV_RING_RES_NETHER) &&
					    (o_ptr->sval != SV_RING_FLAMES) &&
					    (o_ptr->sval != SV_RING_RESIST_FIRE)) return(FALSE);
				break;
				default:
				return (FALSE);
			}
			break;
		}
		case STORE_HERBALIST:
		{
			switch (o_ptr->tval)
			{
				case TV_FOOD:
					if ((o_ptr->sval <= 19) || (o_ptr->sval == 50) || (o_ptr->sval == 40) ||
					    (o_ptr->sval == 37) || (o_ptr->sval == 38) || (o_ptr->sval == 39))
						 break;
				case TV_POTION:
					if ((o_ptr->sval != SV_POTION_APPLE_JUICE) &&
					    (o_ptr->sval != SV_POTION_SLIME_MOLD)) return(FALSE);
					break;
				default:
				return (FALSE);
			}
			break;
		}
	}

	/* XXX XXX XXX Ignore "worthless" items */
	if (object_value(Ind, o_ptr) <= 0) return (FALSE);

	/* XXX Never OK to sell keys */
	if (o_ptr->tval == TV_KEY) return (FALSE);

	/* This prevents suicide-cheeze */
	if ((o_ptr->level < 1) && (o_ptr->owner != p_ptr->id)) return (FALSE);

	/* Assume okay */
	return (TRUE);
}



/*
 * Add the item "o_ptr" to a real stores inventory.
 *
 * If the item is "worthless", it is thrown away (except in the home).
 *
 * If the item cannot be combined with an object already in the inventory,
 * make a new slot for it, and calculate its "per item" price.  Note that
 * this price will be negative, since the price will not be "fixed" yet.
 * Adding an item to a "fixed" price stack will not change the fixed price.
 *
 * In all cases, return the slot (or -1) where the object was placed
 */
static int store_carry(store_type *st_ptr, object_type *o_ptr)
{
	int		i, slot;
	s32b	value, j_value;
	object_type	*j_ptr;

	/* Evaluate the object */
	value = object_value(0, o_ptr);

	/* Cursed/Worthless items "disappear" when sold */
	if (value <= 0) return (-1);

	/* All store items are fully *identified* */
	/* (I don't know it's too nice.. ) */
	o_ptr->ident |= ID_MENTAL;

	/* Erase the inscription */
	if (!(st_info[st_ptr->st_idx].flags1 & SF1_MUSEUM)) o_ptr->note = 0;

	/* Check each existing item (try to combine) */
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		/* Get the existing item */
		j_ptr = &st_ptr->stock[slot];

		/* Can the existing items be incremented? */
		if (store_object_similar(j_ptr, o_ptr))
		{
			/* Hack -- extra items disappear */
			store_object_absorb(j_ptr, o_ptr);

			/* All done */
			return (slot);
		}
	}

	/* No space? */
	if (st_ptr->stock_num >= st_ptr->stock_size) return (-1);


	/* Check existing slots to see if we must "slide" */
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		/* Get that item */
		j_ptr = &st_ptr->stock[slot];

		/* Objects sort by decreasing type */
		if (o_ptr->tval > j_ptr->tval) break;
		if (o_ptr->tval < j_ptr->tval) continue;

		/* Objects sort by increasing sval */
		if (o_ptr->sval < j_ptr->sval) break;
		if (o_ptr->sval > j_ptr->sval) continue;

		/* Evaluate that slot */
		j_value = object_value(0, j_ptr);

		/* Objects sort by decreasing value */
		if (value > j_value) break;
		if (value < j_value) continue;
	}

	/* Slide the others up */
	for (i = st_ptr->stock_num; i > slot; i--)
	{
		st_ptr->stock[i] = st_ptr->stock[i-1];
	}

	/* More stuff now */
	st_ptr->stock_num++;

	/* Insert the new item */
	st_ptr->stock[slot] = *o_ptr;

	/* Return the location */
	return (slot);
}


/*
 * Increase, by a given amount, the number of a certain item
 * in a certain store.  This can result in zero items.
 */
static void store_item_increase(store_type *st_ptr, int item, int num)
{
	int         cnt;
	object_type *o_ptr;

	/* Get the item */
	o_ptr = &st_ptr->stock[item];

	/* Verify the number */
	cnt = o_ptr->number + num;
	if (cnt > 255) cnt = 255;
	else if (cnt < 0) cnt = 0;
	num = cnt - o_ptr->number;

	/* Save the new number */
	o_ptr->number += num;
}


/*
 * Remove a slot if it is empty
 */
static void store_item_optimize(store_type *st_ptr, int item)
{
	int         j;
	object_type *o_ptr;

	/* Get the item */
	o_ptr = &st_ptr->stock[item];

	/* Must exist */
	if (!o_ptr->k_idx) return;

	/* Must have no items */
	if (o_ptr->number) return;

	/* One less item */
	st_ptr->stock_num--;

	/* Slide everyone */
	for (j = item; j < st_ptr->stock_num; j++)
	{
		st_ptr->stock[j] = st_ptr->stock[j + 1];
	}

	/* Nuke the final slot */
	invwipe(&st_ptr->stock[j]);
}


/*
 * This function will keep 'crap' out of the black market.
 * Crap is defined as any item that is "available" elsewhere
 * Based on a suggestion by "Lee Vogt" <lvogt@cig.mcel.mot.com>
 */
static bool black_market_crap(object_type *o_ptr)
{
	int		i, j, k;
	store_type *st_ptr;

	/* Ego items are never crap */
	if (o_ptr->name2) return (FALSE);

	/* Good items are never crap */
	if (o_ptr->to_a > 0) return (FALSE);
	if (o_ptr->to_h > 0) return (FALSE);
	if (o_ptr->to_d > 0) return (FALSE);

	/* check individual store would be better. *later* */
	for(i=0; i<max_st_idx; i++){
		if (st_info[i].flags1 & SF1_ALL_ITEM){
			for(j=0; j<st_info[i].table_num; j++){
				if(o_ptr->k_idx==st_info[i].table[j][0])
					return(FALSE);
			}
		}
	}

	for(k = 0; k < numtowns; k++){
		st_ptr=town[k].townstore;
		/* Check the other "normal" stores */
		for (i = 0; i < max_st_idx; i++)
		{
			/* Don't compare other BMs */
			if (st_info[i].flags1 & SF1_ALL_ITEM) continue;

			/* Check every item in the store */
			for (j = 0; j < st_ptr[i].stock_num; j++)
			{
				object_type *j_ptr = &st_ptr[i].stock[j];

				/* Duplicate item "type", assume crappy */
				if (o_ptr->k_idx == j_ptr->k_idx) return (TRUE);
			}
		}
	}

	/* Assume okay */
	return (FALSE);
}


/*
 * Attempt to delete (some of) a random item from the store
 * Hack -- we attempt to "maintain" piles of items when possible.
 */

static void store_delete(store_type *st_ptr)
{
	int what, num;
	object_type *o_ptr;

	/* Pick a random slot */
	what = rand_int(st_ptr->stock_num);

	/* Determine how many items are here */
	o_ptr = &st_ptr->stock[what];
	num = o_ptr->number;

	/* Hack -- sometimes, only destroy half the items */
	if (rand_int(100) < 50) num = (num + 1) / 2;

	/* Hack -- sometimes, only destroy a single item */
	if (rand_int(100) < 50) num = 1;

	/* Hack -- preserve artifacts */
	if (artifact_p(o_ptr))
	{
		/* Preserve this one */
		a_info[o_ptr->name1].cur_num = 0;
		a_info[o_ptr->name1].known = FALSE;
	}

	/*
	 * Hack -- If rods or wands are dropped, the total maximum timeout or 
	 * charges need to be allocated between the two stacks.  If all the items 
	 * are being dropped, it makes for a neater message to leave the original 
	 * stack's pval alone. -LM-
	 */
	if (o_ptr->tval == TV_WAND)
	{
		if (o_ptr->tval == TV_WAND)
		{
			(void)divide_charged_item(o_ptr, num);
		}
	}

	/* Actually destroy (part of) the item */
	store_item_increase(st_ptr, what, -num);
	store_item_optimize(st_ptr, what);
}


/* Analyze store flags and return a level */
static int return_level(store_type *st_ptr)
{
	store_info_type *sti_ptr = &st_info[st_ptr->st_idx];
	int level;

	if (sti_ptr->flags1 & SF1_RANDOM) level = 0;
	else level = rand_range(1, STORE_OBJ_LEVEL);

//	if (sti_ptr->flags1 & SF1_DEPEND_LEVEL) level += dun_level;

	if (sti_ptr->flags1 & SF1_SHALLOW_LEVEL) level += 5 + rand_int(5);
	if (sti_ptr->flags1 & SF1_MEDIUM_LEVEL) level += 25 + rand_int(25);
	if (sti_ptr->flags1 & SF1_DEEP_LEVEL) level += 45 + rand_int(45);

//	if (sti_ptr->flags1 & SF1_ALL_ITEM) level += p_ptr->lev;

	return (level);
}

/* Is it an ok object ? */ /* I don't like this kinda hack.. */
static int store_tval = 0, store_level = 0;

/*
 * Hack -- determine if a template is "good"
 */
static bool kind_is_storeok(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

#if 0
	if (k_info[k_idx].flags3 & TR3_NORM_ART)
		return( FALSE );
#endif	// 0

	if (k_info[k_idx].flags3 & TR3_INSTA_ART)
		return( FALSE );

	if (!kind_is_legal(k_idx)) return FALSE;

	if (k_ptr->tval != store_tval) return (FALSE);
	if (k_ptr->level < (store_level / 2)) return (FALSE);

	return (TRUE);
}

/*
 * Creates a random item and gives it to a store
 * This algorithm needs to be rethought.  A lot.
 * Currently, "normal" stores use a pre-built array.
 *
 * Note -- the "level" given to "obj_get_num()" is a "favored"
 * level, that is, there is a much higher chance of getting
 * items with a level approaching that of the given level...
 *
 * Should we check for "permission" to have the given item?
 */
static void store_create(store_type *st_ptr)
{
	int i=0, tries, level, chance, item;

	object_type		tmp_obj;
	object_type		*o_ptr = &tmp_obj;
	int force_num = 0;
        object_kind *k_ptr;
	ego_item_type *e_ptr, *e2_ptr;
	bool good, great;


	/* Paranoia -- no room left */
	if (st_ptr->stock_num >= st_ptr->stock_size) return;


	/* Hack -- consider up to four items */
	for (tries = 0; tries < 4; tries++)
	{
		/* Black Market */

		if (st_info[st_ptr->st_idx].flags1 & SF1_ALL_ITEM) 
		{
			/* Hack -- Pick an item to sell */
			item = rand_int(st_info[st_ptr->st_idx].table_num);
			i = st_info[st_ptr->st_idx].table[item][0];
			chance = st_info[st_ptr->st_idx].table[item][1];

			/* Hack -- fake level for apply_magic() */
			level = return_level(st_ptr);

			/* Does it pass the rarity check ? */
			/* Balancing check for other items!!! */
			if (!magik(chance) || magik(60)) i=0;
			else {
				/* Hack -- mass-produce for black-market promised items */
				if (st_info[st_ptr->st_idx].flags1 & SF1_ALL_ITEM)
				{
					force_num = rand_range(2, 5);//was 3,9
				}
			}

			if(!i){
				/* Pick a level for object/magic */
				level = 60 + rand_int(25);	/* for now let's use this */

				/* Clear restriction */
				get_obj_num_hook = NULL;
	
				/* Prepare allocation table */
				get_obj_num_prep();
	
				/* Random item (usually of given level) */
				i = get_obj_num(level);
    				
				/* Handle failure */
				if (!i) continue;
			}
		}

		/* Normal Store */
		else
		{
			/* Hack -- Pick an item to sell */
			item = rand_int(st_info[st_ptr->st_idx].table_num);
			i = st_info[st_ptr->st_idx].table[item][0];
			chance = st_info[st_ptr->st_idx].table[item][1];

			/* Does it pass the rarity check ? */
			if (!magik(chance)) continue;

			/* Hack -- fake level for apply_magic() */
			level = return_level(st_ptr);

			/* Hack -- i > 10000 means it's a tval and all svals are allowed */
			/* NOTE: no store uses this */
			if (i > 10000)
			{
				obj_theme theme;

				/* No themes */
				theme.treasure = 100;
				theme.combat = 100;
				theme.magic = 100;
				theme.tools = 100;
				init_match_theme(theme);

				/* Activate restriction */
				get_obj_num_hook = kind_is_storeok;
				store_tval = i - 10000;

				/* Do we forbid too shallow items ? */
				if (st_info[st_ptr->st_idx].flags1 & SF1_FORCE_LEVEL) store_level = level;
				else store_level = 0;

				/* Prepare allocation table */
				get_obj_num_prep();

				/* Get it ! */
				i = get_obj_num(level);

				/* Invalidate the cached allocation table */
//				alloc_kind_table_valid = FALSE;
			}

			if (!i) continue;
		}

		invcopy(o_ptr, i);

		/* Apply some "low-level" magic (no artifacts) */
		/* ego flags check */
		if (st_info[st_ptr->st_idx].flags1 & SF1_GOOD) good = TRUE;
		else good = FALSE;
		if (st_info[st_ptr->st_idx].flags1 & SF1_GREAT) great = TRUE;
		else great = FALSE;
		apply_magic(&o_ptr->wpos, o_ptr, level, FALSE, good, great, FALSE);

		/* Hack -- Charge lite uniformly */
		if (o_ptr->tval == TV_LITE)
		{
			u32b f1, f2, f3, f4, f5, esp;
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

			/* Only fuelable ones! */
			if (f4 & TR4_FUEL_LITE)
			{
				if (o_ptr->sval == SV_LITE_TORCH) o_ptr->timeout = FUEL_TORCH / 2;
				if (o_ptr->sval == SV_LITE_LANTERN) o_ptr->timeout = FUEL_LAMP / 2;
			}
		}

		/* The item is "known" */
		object_known(o_ptr);

		/* Mega-Hack -- no chests in stores */
//		if (o_ptr->tval == TV_CHEST || o_ptr->tval==8) continue;
		if (o_ptr->tval == TV_CHEST) continue;

		/* Hack - Anti-cheeze =p */
		if ((o_ptr->tval == TV_POTION) && (o_ptr->sval == SV_POTION_EXPERIENCE)) continue;

		/* Prune the black market */
		if (st_info[st_ptr->st_idx].flags1 & SF1_ALL_ITEM)
	    	{
			/* Hack -- No "crappy" items */
			if (black_market_crap(o_ptr)) continue;

			/* Hack -- No "cheap" items */
			if (object_value(0, o_ptr) <= 50) continue; //was <10

			/* No "worthless" items */
			/* if (object_value(o_ptr) <= 0) continue; */

			/* Hack -- No POTION2 items */
			if (o_ptr->tval == TV_POTION2) continue;
			
			/* Hack -- Less POTION items */
			if ((o_ptr->tval == TV_POTION) && magik(80)) continue;

			/* Hack -- less athelas */
			if (o_ptr->tval == TV_FOOD && o_ptr->sval == SV_FOOD_ATHELAS &&
				magik(80)) continue;
		}

		/* Prune normal stores */
		else
		{
			/* No "worthless" items */
			if (object_value(0, o_ptr) <= 0) continue;

			/* ego rarity control for normal stores */
			if ((o_ptr->name2 || o_ptr->name2b))
			{
				if ((!(st_info[st_ptr->st_idx].flags1 & SF1_EGO)) &&
				    !magik(STORE_EGO_CHANCE))
					continue;
			}

			/* Hack -- General store shouldn't sell too much aman cloaks etc */
			if (st_ptr->st_idx == 0 && object_value(0, o_ptr) > 1000 &&
			    magik(33) && 
			    !(st_info[st_ptr->st_idx].flags1 & SF1_EGO))
				continue;
		}

		/* Shop has many egos? */
		if (!(o_ptr->name2 || o_ptr->name2b))
		{
			if ((st_info[st_ptr->st_idx].flags1 & SF1_EGO) &&
			    (magik(50)))
				continue;
		}
		/* Shop has extra rare egos? */
		if ((o_ptr->name2 || o_ptr->name2b))
		{
			if ((st_info[st_ptr->st_idx].flags1 & SF1_RARE_EGO) &&
			    (object_value(0, o_ptr) < 25000) &&
			    (magik(50)))
				continue;
		}

		/* Is the item too cheap for this shop? */
		if ((st_info[st_ptr->st_idx].flags1 & SF1_PRICY_ITEMS1) && (object_value(0, o_ptr) < 3000))
			continue;
		if ((st_info[st_ptr->st_idx].flags1 & SF1_PRICY_ITEMS2) && (object_value(0, o_ptr) < 8000))//PRICY_ITEMS1
			continue;
		if ((st_info[st_ptr->st_idx].flags1 & SF1_PRICY_ITEMS3) && (object_value(0, o_ptr) < 15000))//PRICY_ITEMS2
			continue;
		if ((st_info[st_ptr->st_idx].flags1 & SF1_PRICY_ITEMS4) && (object_value(0, o_ptr) < 20000))
			continue;

		/* Further store flag checks */
		k_ptr = &k_info[o_ptr->k_idx];
		e_ptr = &e_info[o_ptr->name2]; //unused atm
		e2_ptr = &e_info[o_ptr->name2b];//unused atm
		/* Rare enough? */
		/* Interesting numbers: 3+, 6+, 8+, 16+ */
		if ((st_info[st_ptr->st_idx].flags1 & SF1_VERY_RARE))
		{
	    		if ((k_ptr->chance[0] < 8) && (k_ptr->chance[1] < 8) &&
    			    (k_ptr->chance[2] < 8) && (k_ptr->chance[3] < 8))
				continue;
		}
		if ((st_info[st_ptr->st_idx].flags1 & SF1_RARE))
		{
    			if ((k_ptr->chance[0] < 3) && (k_ptr->chance[1] < 3) &&
    			    (k_ptr->chance[2] < 3) && (k_ptr->chance[3] < 3))
				continue;
		}

#if 0
/* MEDIUM_LEVEL and DEEP_LEVEL are already checked in apply_magic,
let's depend on SF1*RARE flags here.. */
		/* Deep enough? */
		if ((st_info[st_ptr->st_idx].flags1 & SF1_DEEP_LEVEL) &&
		    ((k_ptr->locale[0] < 50) && (k_ptr->locale[1] < 50) &&
		    (k_ptr->locale[2] < 50) && (k_ptr->locale[3] < 50)))
			continue;	
		else if ((st_info[st_ptr->st_idx].flags1 & SF1_MEDIUM_LEVEL) &&
		    ((k_ptr->locale[0] < 25) && (k_ptr->locale[1] < 25) &&
		    (k_ptr->locale[2] < 25) && (k_ptr->locale[3] < 25)))
			continue;
#endif

		/* Mass produce and/or Apply discount */
		mass_produce(o_ptr, st_ptr);

		if (st_info[st_ptr->st_idx].flags1 & SF1_NO_DISCOUNT2) {
		    /* Reduce discount */
		    if (o_ptr->discount == 10) o_ptr->discount = 0;
		    if (o_ptr->discount == 25) o_ptr->discount = 10;
		    if (o_ptr->discount == 50) o_ptr->discount = 20;
		    if (o_ptr->discount == 75) o_ptr->discount = 25;
		    if (o_ptr->discount == 90) o_ptr->discount = 25;
		    if (o_ptr->discount == 100) o_ptr->discount = 25;
		}
		if (st_info[st_ptr->st_idx].flags1 & SF1_NO_DISCOUNT) {
		    /* Reduce discount */
		    o_ptr->discount = 0;
		}

		if ((force_num) && (o_ptr->tval != TV_SHOT) &&
		    (o_ptr->tval != TV_ARROW) && (o_ptr->tval != TV_BOLT))
		{
			switch (o_ptr->tval) {
			case TV_DRAG_ARMOR:
				/* Only single items of these */
				force_num = 1;
				break;
			}

			/* Only single items of very expensive stuff */
			if (object_value(0, o_ptr) >= 200000) {
				force_num = 1;
			}

			/* No ego-stacks */
			if (o_ptr->name2) force_num = 1;

			o_ptr->number = force_num;
		}

		/* Nether Realm store items are always level 0 (or 99?) */
		if (st_ptr->st_idx == STORE_BTSUPPLY) o_ptr->level = 0;
		/* Attempt to carry the (known) item */
		(void)store_carry(st_ptr, o_ptr);

		/* Definitely done */
		break;
	}
}

/*
 * Update the bargain info
 */
#if 0
static void updatebargain(s32b price, s32b minprice)
{
	/* Hack -- auto-haggle */
	if (auto_haggle) return;

	/* Cheap items are "boring" */
	if (minprice < 10L) return;

	/* Count the successful haggles */
	if (price == minprice)
	{
		/* Just count the good haggles */
		if (st_ptr->good_buy < MAX_SHORT)
		{
			st_ptr->good_buy++;
		}
	}

	/* Count the failed haggles */
	else
	{
		/* Just count the bad haggles */
		if (st_ptr->bad_buy < MAX_SHORT)
		{
			st_ptr->bad_buy++;
		}
	}
}
#endif

/*
 * Return town index, or -1 if not in a town
 *
 * For multiple stores.
 */
static int gettown(int Ind){
	player_type *p_ptr = Players[Ind];
	int i, retval=-1;
	if(p_ptr->wpos.wz) return(-1);
	for(i=0;i<numtowns;i++){
		if(town[i].x==p_ptr->wpos.wx && town[i].y==p_ptr->wpos.wy){
			retval=i;
			break;
		}
	}
	return(retval);
}

/*
 * Re-displays a single store entry
 *
 * Actually re-sends a single store entry --KLJ--
 */
static void display_entry(int Ind, int pos)
{
	player_type *p_ptr = Players[Ind];
	store_type *st_ptr;
	owner_type *ot_ptr;

	object_type		*o_ptr;
	s32b		x;

	char		o_name[160];
	byte		attr;
	int		wgt;
	int		i;

	int maxwid = 75;

	i=gettown(Ind);
//	if(i==-1) return;	//DUNGEON STORES
	if(i==-1) i=0;
	
	st_ptr = &town[i].townstore[p_ptr->store_num];
//	ot_ptr = &owners[p_ptr->store_num][st_ptr->owner];
	ot_ptr = &ow_info[st_ptr->owner];

	/* Get the item */
	o_ptr = &st_ptr->stock[pos];

#if 0
	/* Get the "offset" */
	i = (pos % 12);

	/* Label it, clear the line --(-- */
	(void)sprintf(out_val, "%c) ", I2A(i));
	prt(out_val, i+6, 0);
#endif

	/* Describe an item in the home */
	if (p_ptr->store_num == 7)
	{
		maxwid = 75;

		/* Leave room for weights, if necessary -DRS- */
		/*if (show_weights) maxwid -= 10;*/

		/* Describe the object */
		object_desc(Ind, o_name, o_ptr, TRUE, 3);
		o_name[maxwid] = '\0';

		attr = tval_to_attr[o_ptr->tval];
		/* grey out if wrong mode / level */
		if ( ( ((!o_ptr->level) || (o_ptr->level > p_ptr->lev)) &&
			(o_ptr->owner) && (o_ptr->owner != p_ptr->id) ) ||
		     ((o_ptr->owner_mode == MODE_IMMORTAL) && (p_ptr->mode != MODE_IMMORTAL)) )
			attr = TERM_L_DARK;

		/* Only show the weight of an individual item */
		wgt = o_ptr->weight;

		/* Send the info */
		Send_store(Ind, pos, attr, wgt, o_ptr->number, 0, o_name, o_ptr->tval, o_ptr->sval, o_ptr->pval);
	}

	/* Describe an item (fully) in a store */
	else
	{
		/* Must leave room for the "price" */
		maxwid = 65;

		/* Leave room for weights, if necessary -DRS- */
		/*if (show_weights) maxwid -= 7;*/

		/* Describe the object (fully) */
		object_desc_store(Ind, o_name, o_ptr, TRUE, 3);
		o_name[maxwid] = '\0';

		attr = tval_to_attr[o_ptr->tval];
		/* grey out if wrong mode / level */
		if ( ( ((!o_ptr->level) || (o_ptr->level > p_ptr->lev)) &&
			(o_ptr->owner) && (o_ptr->owner != p_ptr->id) ) ||
		     ((o_ptr->owner_mode == MODE_IMMORTAL) && (p_ptr->mode != MODE_IMMORTAL)) )
			attr = TERM_L_DARK;

		/* Only show the weight of an individual item */
		wgt = o_ptr->weight;

		/* Extract the "minimum" price */
		x = price_item(Ind, o_ptr, ot_ptr->min_inflate, FALSE);

		/* Send the info */
		Send_store(Ind, pos, attr, wgt, o_ptr->number, x, o_name, o_ptr->tval, o_ptr->sval, o_ptr->pval);
	}
}


/*
 * Displays a store's inventory			-RAK-
 * All prices are listed as "per individual object".  -BEN-
 *
 * The inventory is "sent" not "displayed". -KLJ-
 */
static void display_inventory(int Ind)
{
	player_type *p_ptr = Players[Ind];
	store_type *st_ptr;
	int k;
	int i;
	
	i=gettown(Ind);
//	if(i==-1) return;	//DUNGEON STORES
	if(i==-1) i=0;

	st_ptr = &town[i].townstore[p_ptr->store_num];

	/* Display the next 48 items */
	for (k = 0; k < STORE_INVEN_MAX; k++)
	{
		/* Do not display "dead" items */
		if (k >= st_ptr->stock_num) break;

		/* Display that line */
		display_entry(Ind, k);
	}
	//if (k < 1) Send_store(Ind, 0, 0, 0, 0, 0, "", 0, 0);
}


/*
 * Displays players gold					-RAK-
 */
static void store_prt_gold(int Ind)
{
	player_type *p_ptr = Players[Ind];

	Send_gold(Ind, p_ptr->au, p_ptr->balance);
}


/*
 * Displays store (after clearing screen)		-RAK-
 */
static void display_store(int Ind)
{
	player_type *p_ptr = Players[Ind];
	store_type *st_ptr;
	owner_type *ot_ptr;
	cptr store_name;
	int i;

	i=gettown(Ind);
//	if(i==-1) return;	//DUNGEON STORES
	if(i==-1) i=0;

	st_ptr = &town[i].townstore[p_ptr->store_num];
	ot_ptr = &ow_info[st_ptr->owner];
	store_name = (st_name + st_info[st_ptr->st_idx].name);

	/* Display the current gold */
	store_prt_gold(Ind);

	/* Draw in the inventory */
	display_inventory(Ind);

	/* The "Home" is special */
	if (p_ptr->store_num == 7)	/* This shouldn't happen */
	{
		/* Send the store info */
//		Send_store_info(Ind, p_ptr->store_num, 0, st_ptr->stock_num);
		Send_store_info(Ind, p_ptr->store_num, "Your House", "", st_ptr->stock_num, st_ptr->stock_size);
	}

	/* Normal stores */
	else
	{
		cptr owner_name = (ow_name + ot_ptr->name);

		/* Send the store actions info */
		show_building(Ind, st_ptr);

		/* Hack -- Museum doesn't have owner */
		if (st_info[st_ptr->st_idx].flags1 & SF1_MUSEUM) owner_name = "";

		/* Send the store info */
//		Send_store_info(Ind, p_ptr->store_num, st_ptr->owner, st_ptr->stock_num);
		Send_store_info(Ind, p_ptr->store_num, store_name, owner_name, st_ptr->stock_num, ot_ptr->max_cost);
	}

}



/*
 * Haggling routine					-RAK-
 *
 * Return TRUE if purchase is NOT successful
 */
static bool sell_haggle(int Ind, object_type *o_ptr, s32b *price)
{
	player_type *p_ptr = Players[Ind];
	store_type *st_ptr;

	owner_type *ot_ptr;

	s32b               purse, cur_ask, final_ask;

	int			final = FALSE;

	cptr		pmt = "Offer";
	int i;

	i=gettown(Ind);
//	if(i==-1) return(FALSE);       //DUNGEON STORES
        if(i==-1) i=0;

	st_ptr = &town[i].townstore[p_ptr->store_num];
//	ot_ptr = &owners[p_ptr->store_num][st_ptr->owner];
	ot_ptr = &ow_info[st_ptr->owner];

	*price = 0;


	/* Obtain the starting offer and the final offer */
	cur_ask = price_item(Ind, o_ptr, ot_ptr->max_inflate, TRUE);
	final_ask = price_item(Ind, o_ptr, ot_ptr->min_inflate, TRUE);

	/* Get the owner's payout limit */
	purse = (s32b)(ot_ptr->max_cost);

	/* No reason to haggle */
	if (final_ask >= purse)
	{
		/* Message */
		msg_print(Ind, "You instantly agree upon the price.");

		/* Offer full purse */
		final_ask = purse;
	}

	/* No need to haggle */
	else
	{
		/* Message */
		msg_print(Ind, "You eventually agree upon the price.");
	}

	/* Final price */
	cur_ask = final_ask;

	/* Final offer */
	final = TRUE;
	pmt = "Final Offer";

	/* Hack -- Return immediately */
	*price = final_ask * o_ptr->number;
	return (FALSE);
}



/*
 * Will the owner retire?
 */
static bool retire_owner_p(store_type *st_ptr)
{
	store_info_type *sti_ptr = &st_info[st_ptr->st_idx];

	if ((sti_ptr->owners[0] == sti_ptr->owners[1]) &&
	    (sti_ptr->owners[0] == sti_ptr->owners[2]) &&
	    (sti_ptr->owners[0] == sti_ptr->owners[3]))
	{
		/* there is no other owner */
		return FALSE;
	}

	if (rand_int(STORE_SHUFFLE) != 0)
	{
		return FALSE;
	}

	return TRUE;
}



/* Is the command legal?	- Jir - */
static bool store_attest_command(int store, int bact)
{
	int i;
	for (i = 0; i < STORE_MAX_ACTION; i++)
		if (ba_info[st_info[store].actions[i]].action == bact) return (TRUE);
		//if (st_info[store].actions[i] == action) return (TRUE);

	return (FALSE);
}


/*
 * Stole an item from a store                   -DG-
 */
/* TODO: specify 'amt' */
void store_stole(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	int st = p_ptr->store_num;

	store_type *st_ptr;
	owner_type *ot_ptr;

	int			i;
	int			item_new;
	int amt = 1;
	int chance = 0;

	u32b		best, tbest, tcadd, tccompare;

	object_type		sell_obj;
	object_type		*o_ptr;

	char		o_name[160];

	if (p_ptr->store_num == 7)
	{
		msg_print(Ind, "You don't steal from your home!");
		return;
	}

	/* Level restriction (mainly anticheeze) */
	if (p_ptr->lev < 10)
	{
		msg_print(Ind, "You dare not to!");
		return;
	}

	/* I'm not saying this is the way to go, but they
	   can cheeze by attempting repeatedly */
	if(p_ptr->tim_blacklist){
		msg_print(Ind, "Bastard Thief! Get out of my shop!!!");
		if(p_ptr->tim_blacklist < 10000000)	/* 10 million turns is LONG ENOUGH */
			p_ptr->tim_blacklist += 1000;	/* add a little */
		store_kick(Ind, FALSE);
		return;
	}

	i=gettown(Ind);
//	if(i==-1) return;       //DUNGEON STORES
        if(i==-1) i=0;

	if (p_ptr->store_num==-1){
		msg_print(Ind,"You left the shop!");
		return;
	}

	st_ptr = &town[i].townstore[st];
//	ot_ptr = &owners[st][st_ptr->owner];
	ot_ptr = &ow_info[st_ptr->owner];

	if (!store_attest_command(p_ptr->store_num, BACT_STEAL))
	{
		//if (!is_admin(p_ptr)) return;
		return;
	}

	/* Get the actual item */
	o_ptr = &st_ptr->stock[item];

	/* Assume the player wants just one of them */
	/*amt = 1;*/

	/* Hack -- get a "sample" object */
	sell_obj = *o_ptr;
	sell_obj.number = amt;

	/*
	 * Hack -- If rods or wands are dropped, the total maximum timeout or 
	 * charges need to be allocated between the two stacks.  If all the items 
	 * are being dropped, it makes for a neater message to leave the original 
	 * stack's pval alone. -LM-
	 */
	if (o_ptr->tval == TV_WAND)
	{
		if (o_ptr->tval == TV_WAND)
		{
			sell_obj.pval = divide_charged_item(o_ptr, amt);
		}
	}

	/* Hack -- require room in pack */
	if (!inven_carry_okay(Ind, &sell_obj))
	{
		msg_print(Ind, "You cannot carry that many different items.");
		return;
	}

	/* shopkeepers in special shops are often especially careful */
	if (st_info[st_ptr->st_idx].flags1 & SF1_NO_STEAL)
	{
		msg_print(Ind, "The shopkeeper watches the items extremely cautiously!");
		return;
	}

	/* Determine the "best" price (per item) */
	/* NOTE: it's used to determine the penalty when stealing failed */
	best = price_item(Ind, &sell_obj, ot_ptr->min_inflate, FALSE);

	/* shopkeeper watches expensive items carefully */
	tcadd = 100;
	tbest = object_value(Ind, o_ptr);

	if (tbest > 10000000) tbest = 10000000;
	tccompare = 10;
	while((tccompare / 10) < tbest)
	{
		tcadd = (tcadd * 114) / 100;
		tccompare = (tccompare * 13) / 10;
	}
	tcadd /= 100;
	if (tcadd < 1) tcadd = 1;
	tcadd = 57000 / ((10000 / tcadd) + 50);

	/* Player tries to stole it */
#if 0	// Tome formula .. seemingly, high Stealing => more failure!
	if (rand_int((40 - p_ptr->stat_ind[A_DEX]) +
	    ((((j_ptr->weight * amt) / 2) + tcadd) / (5 + get_skill_scale(SKILL_STEALING, 15))) +
	    (get_skill_scale(SKILL_STEALING, 25))) <= 10)
#endif	// 0
	chance = (50 - p_ptr->stat_ind[A_DEX]) +
	    ((((sell_obj.weight * amt) / 2) + tcadd) /
	    (5 + get_skill_scale(p_ptr, SKILL_STEALING, 15))) -
	    (get_skill_scale(p_ptr, SKILL_STEALING, 25));

	/* Invisibility and stealth are not unimportant */
	chance = (chance * (100 - (p_ptr->tim_invisibility > 0 ? 13 : 0))) / 100;
	chance = (chance * (115 - ((p_ptr->skill_stl * 3) / 4))) / 100;

	/* shopkeepers in special shops are often especially careful */
	if (st_info[st_ptr->st_idx].flags1 & SF1_VHARD_STEAL)
		chance += 30;
	else if (st_info[st_ptr->st_idx].flags1 & SF1_HARD_STEAL)
		chance += 15;

	/* avoid div0 */
	if (chance < 1) chance = 1;

	/* always 1% chance to fail, so that ppl won't macro it */
	/* 1% pfft. 5% and rising... */
	if (rand_int(chance) <= 10 && !magik(5))
	{
		/* Hack -- buying an item makes you aware of it */
		object_aware(Ind, &sell_obj);

		/* Hack -- clear the "fixed" flag from the item */
		sell_obj.ident &= ~ID_FIXED;
		
		/* Stolen items cannot be sold */
		sell_obj.discount = 100;
		sell_obj.note = quark_add("stolen");

		/* Describe the transaction */
		object_desc(Ind, o_name, &sell_obj, TRUE, 3);

		/* Message */
		msg_format(Ind, "You stole %s.", o_name);

		/* Let the player carry it (as if he picked it up) */
		item_new = inven_carry(Ind, &sell_obj);

		/* Describe the final result */
		object_desc(Ind, o_name, &p_ptr->inventory[item_new], TRUE, 3);

		/* Message */
		msg_format(Ind, "You have %s (%c).",
				o_name, index_to_label(item_new));

		/* Handle stuff */
		handle_stuff(Ind);

		/* Note how many slots the store used to have */
		i = st_ptr->stock_num;

		/* Remove the bought items from the store */
		store_item_increase(st_ptr, item, -amt);
		store_item_optimize(st_ptr, item);

		/* Resend the basic store info */
		display_store(Ind);

		/* Store is empty */
		if (st_ptr->stock_num == 0)
		{
#if 0	/* disabled -- some stores with few stocks get silly */
			/* XXX kick him out once, since client update doesn't
			 * seem to work right */
			store_kick(Ind, FALSE);
			suppress_message = FALSE;

			/* Shuffle */
			if (rand_int(STORE_SHUFFLE) == 0)
			{
				/* Message */
				msg_print(Ind, "The shopkeeper retires.");

				/* Shuffle the store */
				store_shuffle(st_ptr);
			}

			/* Maintain */
			else
			{
				/* Message */
				msg_print(Ind, "The shopkeeper brings out some new stock.");
			}

			/* New inventory */
			for (i = 0; i < 10; i++)
			{
				/* Maintain the store */
				store_maint(st_ptr);
			}

			/* Redraw everything */
			//display_inventory(Ind);

			return;
#endif	// 0

			/* This should do a nice restock */
			//st_ptr->last_visit = 0;
			st_ptr->last_visit = -10L * STORE_TURNS;
		}

		/* The item is gone */
		//else
		if (st_ptr->stock_num != i)
		{
			/* Redraw everything */
			display_inventory(Ind);
		}

		/* Item is still here */
		else
		{
			/* Redraw the item */
			display_entry(Ind, item);
		}
		//suppress_message = FALSE;

	}

	else
	{
		/* Complain */
		// say_comment_4();
		msg_print(Ind, "\377yBastard\377L!!!");
		msg_print(Ind, "\377rNow you're on the black list of merchants..");

		/* Reset insults */
		st_ptr->insult_cur = 0;
		st_ptr->good_buy = 0;
		st_ptr->bad_buy = 0;

		/* Kicked out for a LONG time */
		p_ptr->tim_blacklist += tcadd * amt * 8 + 1000;

		/* Of course :) */
		store_kick(Ind, FALSE);
	}

	/* Not kicked out */
	return;
}


/*
 * Buy an item from a store				-RAK-
 */
void store_purchase(int Ind, int item, int amt)
{
	player_type *p_ptr = Players[Ind];

	int st = p_ptr->store_num;

	store_type *st_ptr;
	owner_type *ot_ptr;

	int			i, choice;
	int			item_new;

	s32b		price, best;

	object_type		sell_obj;
	object_type		*o_ptr;

	char		o_name[160];

	if (amt < 1)
	{
		s_printf("$INTRUSION$ Bad amount %d! Bought by %s.\n", amt, p_ptr->name);
		msg_print(Ind, "\377RInvalid amount. Your attempt has been logged.");
		return;
	}

	if (p_ptr->store_num == 7)
	{
		home_purchase(Ind, item, amt);
		return;
	}

	i=gettown(Ind);
//	if(i==-1) return;       //DUNGEON STORES
        if(i==-1) i=0;

	if (p_ptr->store_num==-1){
		msg_print(Ind,"You left the shop!");
		return;
	}

	st_ptr = &town[i].townstore[st];
//	ot_ptr = &owners[st][st_ptr->owner];
	ot_ptr = &ow_info[st_ptr->owner];

	if (!store_attest_command(p_ptr->store_num, BACT_BUY))
	{
		/* Hack -- admin can 'buy'
		 * (it's to remove craps from the Museums) */
		if (!is_admin(p_ptr)) return;
	}

	/* Empty? */
	if (st_ptr->stock_num <= 0)
	{
		if (p_ptr->store_num == 7) msg_print(Ind, "Your home is empty.");
		else msg_print(Ind, "I am currently out of stock.");
		return;
	}


#if 0
	/* Find the number of objects on this and following pages */
	i = (st_ptr->stock_num - store_top);

	/* And then restrict it to the current page */
	if (i > 12) i = 12;

	/* Prompt */
	if (p_ptr->store_num == 7)
	{
		sprintf(out_val, "Which item do you want to take? ");
	}
	else
	{
		sprintf(out_val, "Which item are you interested in? ");
	}

	/* Get the item number to be bought */
	if (!get_stock(&item, out_val, 0, i-1)) return;
#endif

	/* Get the actual item */
	o_ptr = &st_ptr->stock[item];

	/* Check if the player is powerful enough for that item */
        if ((o_ptr->owner) && (o_ptr->owner != p_ptr->id) && (o_ptr->level > p_ptr->lev || o_ptr->level == 0))
        {
                if (cfg.anti_cheeze_pickup)
                {
                        msg_print(Ind, "You aren't powerful enough yet to pick up that item!");
                        return;
                }
                if (true_artifact_p(o_ptr) && cfg.anti_arts_pickup)
                {
                        msg_print(Ind, "You aren't powerful enough yet to pick up that artifact!");
                        return;
                }
	}

	/* Assume the player wants just one of them */
	/*amt = 1;*/

	/* Hack -- get a "sample" object */
	sell_obj = *o_ptr;
	sell_obj.number = amt;

	/*
	 * Hack -- If rods or wands are dropped, the total maximum timeout or 
	 * charges need to be allocated between the two stacks.  If all the items 
	 * are being dropped, it makes for a neater message to leave the original 
	 * stack's pval alone. -LM-
	 */
	if (o_ptr->tval == TV_WAND)
	{
		if (o_ptr->tval == TV_WAND)
		{
			sell_obj.pval = divide_charged_item(o_ptr, amt);
		}
	}

	/* Hack -- require room in pack */
	if (!inven_carry_okay(Ind, &sell_obj))
	{
		msg_print(Ind, "You cannot carry that many different items.");
		return;
	}

	/* Determine the "best" price (per item) */
	best = price_item(Ind, &sell_obj, ot_ptr->min_inflate, FALSE);

#if 0
	/* Find out how many the player wants */
	if (o_ptr->number > 1)
	{
		/* Hack -- note cost of "fixed" items */
		if ((p_ptr->store_num != 7) && (o_ptr->ident & ID_FIXED))
		{
			msg_format("That costs %ld gold per item.", (long)(best));
		}

		/* Get a quantity */
		amt = get_quantity(NULL, o_ptr->number);

		/* Allow user abort */
		if (amt <= 0) return;
	}
#endif

	/* Attempt to buy it */
	if (p_ptr->store_num != 7)
	{
		/* For now, I'm assuming everything's price is "fixed" */
		/* Fixed price, quick buy */
#if 0
		if (o_ptr->ident & ID_FIXED)
		{
#endif
			/* Assume accept */
			choice = 0;

			/* Go directly to the "best" deal */
			price = (best * sell_obj.number);
#if 0
		}

		/* Haggle for it */
		else
		{
			/* Describe the object (fully) */
			object_desc_store(o_name, &sell_obj, TRUE, 3);

			/* Message */
			msg_format("Buying %s (%c).", o_name, I2A(item));

			/* Haggle for a final price */
			choice = purchase_haggle(&sell_obj, &price);

			/* Hack -- Got kicked out */
			if (st_ptr->store_open >= turn) return;
		}
#endif


		/* Player wants it */
		if (choice == 0)
		{
			/* Fix the item price (if "correctly" haggled) */
			if (price == (best * sell_obj.number)) o_ptr->ident |= ID_FIXED;

			/* Player can afford it */
			if (p_ptr->au >= price)
			{
				if (p_ptr->taciturn_messages) suppress_message = TRUE;

				/* Say "okay" */
				say_comment_1(Ind);

				/* Be happy */
				/*decrease_insults();*/

				/* Spend the money */
				p_ptr->au -= price;

				/* Buying things lessen the distrust somewhat */
				if (p_ptr->tim_blacklist && (price > 100))
				{
					p_ptr->tim_blacklist -= price / 100;
					if (p_ptr->tim_blacklist < 1) p_ptr->tim_blacklist = 1;
				}

				/* Update the display */
				store_prt_gold(Ind);

				/* Hack -- buying an item makes you aware of it */
				object_aware(Ind, &sell_obj);

				/* Hack -- clear the "fixed" flag from the item */
				sell_obj.ident &= ~ID_FIXED;

				/* Describe the transaction */
				object_desc(Ind, o_name, &sell_obj, TRUE, 3);

				/* Message */
				msg_format(Ind, "You bought %s for %ld gold.", o_name, (long)price);

				/* Let the player carry it (as if he picked it up) */
				item_new = inven_carry(Ind, &sell_obj);

				/* Describe the final result */
				object_desc(Ind, o_name, &p_ptr->inventory[item_new], TRUE, 3);

				/* Message */
				msg_format(Ind, "You have %s (%c).",
				           o_name, index_to_label(item_new));

				/* Handle stuff */
				handle_stuff(Ind);

				/* Note how many slots the store used to have */
				i = st_ptr->stock_num;

				/* Remove the bought items from the store */
				store_item_increase(st_ptr, item, -amt);
				store_item_optimize(st_ptr, item);

				/* Resend the basic store info */
				display_store(Ind);

				/* Store is empty */
				if (st_ptr->stock_num == 0)
				{
#if 0	/* disabled -- some stores with few stocks get silly */
					/* XXX kick him out once, since client update doesn't
					 * seem to work right */
					store_kick(Ind, FALSE);
					suppress_message = FALSE;

					/* Shuffle */
					if (rand_int(STORE_SHUFFLE) == 0)
					{
						/* Message */
						msg_print(Ind, "The shopkeeper retires.");

						/* Shuffle the store */
						store_shuffle(st_ptr);
					}

					/* Maintain */
					else
					{
						/* Message */
						msg_print(Ind, "The shopkeeper brings out some new stock.");
					}

					/* New inventory */
					for (i = 0; i < 10; i++)
					{
						/* Maintain the store */
						store_maint(st_ptr);
					}

					/* Redraw everything */
					//display_inventory(Ind);

					return;
#endif	// 0
				}

				/* The item is gone */
				else if (st_ptr->stock_num != i)
				{
					/* Redraw everything */
					display_inventory(Ind);
				}

				/* Item is still here */
				else
				{
					/* Redraw the item */
					display_entry(Ind, item);
				}
				suppress_message = FALSE;
			}

			/* Player cannot afford it */
			else
			{
				/* Simple message (no insult) */
				msg_print(Ind, "You do not have enough gold.");
			}
		}
	}

	/* Home is much easier */
	else
	{
		/* Carry the item */
		item_new = inven_carry(Ind, &sell_obj);

		/* Describe just the result */
		object_desc(Ind, o_name, &p_ptr->inventory[item_new], TRUE, 3);

		/* Message */
		msg_format(Ind, "You have %s (%c).", o_name, index_to_label(item_new));

		/* Handle stuff */
		handle_stuff(Ind);

		/* Take note if we take the last one */
		i = st_ptr->stock_num;

		/* Remove the items from the home */
		store_item_increase(st_ptr, item, -amt);
		store_item_optimize(st_ptr, item);

		/* Resend the basic store info */
		display_store(Ind);

		/* Hack -- Item is still here */
		if (i == st_ptr->stock_num)
		{
			/* Redraw the item */
			display_entry(Ind, item);
		}

		/* The item is gone */
		else
		{
			/* Redraw everything */
			display_inventory(Ind);
		}
	}

	/* Not kicked out */
	return;
}


/*
 * Sell an item to the store (or home)
 */
void store_sell(int Ind, int item, int amt)
{
	player_type *p_ptr = Players[Ind];
//	store_type *st_ptr;

	int			choice;

	s32b		price;

	object_type		sold_obj;
	object_type		*o_ptr;

	char		o_name[160];

	/* Check for client-side exploit! */
	if (p_ptr->inventory[item].number < amt) {
		s_printf("$INTRUSION$ Bad amount %d of %d! (Home) Sold by %s.\n", amt, p_ptr->inventory[item].number, p_ptr->name);
		msg_print(Ind, "You don't have that many!");
		return;
	}

	if (p_ptr->store_num == 7)
	{
		home_sell(Ind, item, amt);
		return;
	}

	/* sanity check - Yakina - */
	if (p_ptr->store_num==-1){
		msg_print(Ind,"You left the shop!");
		return;
	}

//	st_ptr = &town[gettown(Ind)].townstore[p_ptr->store_num];

	if (!store_attest_command(p_ptr->store_num, BACT_SELL)) return;

	/* You can't sell 0 of something. */
	if (amt <= 0) return;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* Check for validity of sale */
	if (!store_will_buy(Ind, o_ptr))
	{
		msg_print(Ind, "I don't want that!");
		return;
	}

#if 0

	/* Not gonna happen XXX inscribe */
	if ((item >= INVEN_WIELD) && cursed_p(o_ptr))
	{
		/* Oops */
		msg_print("Hmmm, it seems to be cursed.");

		/* Stop */
		return;
	}

#endif

	/* Create the object to be sold (structure copy) */
	sold_obj = *o_ptr;
	sold_obj.number = amt;

	/* Get a full description */
	object_desc(Ind, o_name, &sold_obj, TRUE, 3);

	/* Remove any inscription for stores */
	if (p_ptr->store_num != 7) sold_obj.note = 0;

	/* Access the store */
//	i=gettown(Ind);
//	store_type *st_ptr = &town[townval].townstore[i];

	/* Is there room in the store (or the home?) */
	if (gettown(Ind)!=-1)	//DUNGEON STORES
	{
	if (!store_check_num(&town[gettown(Ind)].townstore[p_ptr->store_num], &sold_obj))
	{
		if (p_ptr->store_num == 7) msg_print(Ind, "Your home is full.");
		else msg_print(Ind, "I have not the room in my store to keep it.");
		return;
	}
	}else{
	if (!store_check_num(&town[0].townstore[p_ptr->store_num], &sold_obj))
	{
		if (p_ptr->store_num == 7) msg_print(Ind, "Your home is full.");
		else msg_print(Ind, "I have not the room in my store to keep it.");
		return;
	}
	}

	/* Museum */
//	if (p_ptr->store_num == 57)
	if (st_info[p_ptr->store_num].flags1 & SF1_MUSEUM)
	{
#if 0
		/* Describe the transaction */
		msg_format(Ind, "Selling %s (%c).", o_name, index_to_label(item));

		/* Haggle for it */
		choice = sell_haggle(Ind, &sold_obj, &price);

		/* Tell the client about the price */
		Send_store_sell(Ind, price);
#endif	// 0

		/* Save the info for the confirmation */
		p_ptr->current_selling = item;
		p_ptr->current_sell_amt = amt;
		p_ptr->current_sell_price = 0;

		/* Hack -- assume confirmed */
		store_confirm(Ind);

		/* Wait for confirmation before actually selling */
		return;
	}

	/* Real store */
	if (p_ptr->store_num != 7)
	{
		/* Describe the transaction */
		msg_format(Ind, "Selling %s (%c).", o_name, index_to_label(item));

		/* Haggle for it */
		choice = sell_haggle(Ind, &sold_obj, &price);

		/* Tell the client about the price */
		Send_store_sell(Ind, price);

		/* Save the info for the confirmation */
		p_ptr->current_selling = item;
		p_ptr->current_sell_amt = amt;
		p_ptr->current_sell_price = price;

		/* Wait for confirmation before actually selling */
		return;
	}


#if 0
	/* Player is at home */
	else
	{
		/* Describe */
		msg_format(Ind, "You drop %s.", o_name);

		/* Take it from the players inventory */
		inven_item_increase(Ind, item, -amt);
		inven_item_describe(Ind, item);
		inven_item_optimize(Ind, item);

		/* Handle stuff */
		handle_stuff(Ind);

		/* Let the store (home) carry it */
		item_pos = home_carry(&sold_obj);

		/* Update store display */
		if (item_pos >= 0)
		{
			store_top = (item_pos / 12) * 12;
			display_inventory(Ind);
		}
	}
#endif
}


void store_confirm(int Ind)
{
	player_type *p_ptr = Players[Ind];
	long item, amt, price, value, price_redundance;

	object_type *o_ptr, sold_obj;
	char o_name[160];
	int item_pos;
	bool museum;

	/* Abort if we shouldn't be getting called */
	if (p_ptr->current_selling == -1)
		return;

	if (p_ptr->store_num==-1){
		msg_print(Ind,"You left the building!");
		return;
	}

	museum = (st_info[p_ptr->store_num].flags1 & SF1_MUSEUM) ? TRUE : FALSE;

	/* Restore the variables */
	item = p_ptr->current_selling;
	amt = p_ptr->current_sell_amt;
	price = p_ptr->current_sell_price;
        /* Server-side exploit checks - C. Blue */
        if (amt <= 0) {
                s_printf("$INTRUSION$ (FORCED) Bad amount %d! Sold by %s.\n", amt, p_ptr->name);
		return;
	}
        if (p_ptr->inventory[item].number < amt) {
                s_printf("$INTRUSION$ Bad amount %d of %d! Sold by %s.\n", amt, p_ptr->inventory[item].number, p_ptr->name);
                msg_print(Ind, "You don't have that many!");
		return;
	}
	sold_obj = p_ptr->inventory[item];
	sold_obj.number = amt;
	(void) sell_haggle(Ind, &sold_obj, &price_redundance);
	if (price != price_redundance) {
                s_printf("$INTRUSION$ Tried to sell %ld for %ld! Sold by %s.\n", price_redundance, price, p_ptr->name);
#if 0
                msg_print(Ind, "Wrong item!");
                return;
#else
		price = price_redundance;
		if (!price) return;
		/* Paranoia - Don't sell '(nothing)s' */
		if (!p_ptr->inventory[item].k_idx) return;
#endif
	}

	/* Trash the saved variables */
	p_ptr->current_selling = -1;
	p_ptr->current_sell_amt = -1;
	p_ptr->current_sell_price = -1;

	/* Sold... */

	/* Say "okay" */
	if (!museum) say_comment_1(Ind);

	/* Be happy */
	/*decrease_insults();*/

	/* Get some money */
	p_ptr->au += price;

	/* Update the display */
	store_prt_gold(Ind);

	/* Get the inventory item */
	o_ptr = &p_ptr->inventory[item];

	/* Become "aware" of the item */
	object_aware(Ind, o_ptr);

	/* Know the item fully */
	object_known(o_ptr);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Re-Create the now-identified object that was sold */
	sold_obj = *o_ptr;
	sold_obj.number = amt;

	/*
	 * Hack -- If rods or wands are dropped, the total maximum timeout or 
	 * charges need to be allocated between the two stacks.  If all the items 
	 * are being dropped, it makes for a neater message to leave the original 
	 * stack's pval alone. -LM-
	 */
	if (o_ptr->tval == TV_WAND)
	{
		if (o_ptr->tval == TV_WAND)
		{
			sold_obj.pval = divide_charged_item(o_ptr, amt);
		}
	}

	/* Get the "actual" value */
	value = object_value(Ind, &sold_obj) * sold_obj.number;

	/* Get the description all over again */
	object_desc(Ind, o_name, &sold_obj, TRUE, 3);

	/* Describe the result (in message buffer) */
	if (!museum) msg_format(Ind, "You sold %s for %ld gold.", o_name, (long)price);

	/* Analyze the prices (and comment verbally) */
	/*purchase_analyze(price, value, dummy);*/

	/* Take the item from the player, describe the result */
	inven_item_increase(Ind, item, -amt);
	inven_item_describe(Ind, item);
	inven_item_optimize(Ind, item);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Artifact won't be sold in a store */
	if (cfg.anti_arts_shop && true_artifact_p(&sold_obj) && !museum)
	{
		a_info[sold_obj.name1].cur_num = 0;
		a_info[sold_obj.name1].known = FALSE;
		return;
	}

	/* The store gets that (known) item */
//	if(sold_obj.tval!=8)	// What was it for.. ?
	if(gettown(Ind)!=-1) //DUNGEON STORES
		item_pos = store_carry(&town[gettown(Ind)].townstore[p_ptr->store_num], &sold_obj);
	else
		item_pos = store_carry(&town[0].townstore[p_ptr->store_num], &sold_obj);
//		item_pos = store_carry(p_ptr->store_num, &sold_obj);

	/* Resend the basic store info */
	display_store(Ind);

	/* Re-display if item is now in store */
	if (item_pos >= 0)
	{
		display_inventory(Ind);
	}
}


/*
 * Examine an item in a store			   -JDL-
 */
void store_examine(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	int st = p_ptr->store_num;

	store_type *st_ptr;
	owner_type *ot_ptr;

	int			i;

	object_type		sell_obj;
	object_type		*o_ptr;

	char		o_name[160];

	if (p_ptr->store_num == 7)
	{
		home_examine(Ind, item);
		return;
	}

	i=gettown(Ind);
//	if(i==-1) return;       //DUNGEON STORES
        if(i==-1) i=0;

	st_ptr = &town[i].townstore[st];
//	ot_ptr = &owners[st][st_ptr->owner];
	ot_ptr = &ow_info[st_ptr->owner];

	if (!store_attest_command(st, BACT_EXAMINE))
	{
		//if (!is_admin(p_ptr)) return;
		return;
	}

	/* Empty? */
	if (st_ptr->stock_num <= 0)
	{
		if (p_ptr->store_num == 7) msg_print(Ind, "Your home is empty.");
		else msg_print(Ind, "I am currently out of stock.");
		return;
	}

	/* Get the actual item */
	o_ptr = &st_ptr->stock[item];

	/* Assume the player wants just one of them */
	/*amt = 1;*/

	/* Hack -- get a "sample" object */
	sell_obj = *o_ptr;

	/* Description */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	/* Require full knowledge */
	if (!(o_ptr->ident & (ID_MENTAL)))
	{
		/* Describe */
		msg_format(Ind, "%s\n", o_name);
		if (strlen(o_name) > 77) msg_format(Ind, "%s\n", o_name + 77);

                switch(o_ptr->tval){
	        case TV_HAFTED:
    		        msg_print(Ind, "It's a hafted weapon."); break;
                case TV_POLEARM:
	        	msg_print(Ind, "It's a polearm."); break;
    	        case TV_SWORD:
        	        msg_print(Ind, "It's a sword-type weapon."); break;
                case TV_AXE:
	                msg_print(Ind, "It's an axe-type weapon."); break;
        	default:
	                if (wield_slot(Ind, o_ptr) != INVEN_WIELD) msg_print(Ind, "You have no special knowledge about that item.");
        	        break;
	        }
 		/* This can only happen in the home */
		if (wield_slot(Ind, o_ptr) == INVEN_WIELD)
		{
			int blows = calc_blows(Ind, o_ptr);
			msg_format(Ind, "With it, you can usually attack %d time%s/turn.",
					blows, blows > 1 ? "s" : "");
		}
		else msg_print(Ind, "You have no special knowledge about that item.");
		return;
	}

	/* Description */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	/* Describe */
//	msg_format(Ind, "Examining %s...", o_name);

	/* Describe it fully */
	if (!identify_fully_aux(Ind, o_ptr)) msg_print(Ind, "You see nothing special.");

#if 0
	if (o_ptr->tval < TV_BOOK)
	{
		if (!identify_fully_aux(Ind, o_ptr)) msg_print(Ind, "You see nothing special.");
		/* Books are read */
	}
	else
	{
		do_cmd_browse_aux(o_ptr);
	}
#endif	// 0

	return;
}


/*
 * Hack -- set this to leave the store
 */
static bool leave_store = FALSE;


/*
 * Enter a store, and interact with it.
 *
 * Note that we use the standard "request_command()" function
 * to get a command, allowing us to use "command_arg" and all
 * command macros and other nifty stuff, but we use the special
 * "shopping" argument, to force certain commands to be converted
 * into other commands, normally, we convert "p" (pray) and "m"
 * (cast magic) into "g" (get), and "s" (search) into "d" (drop).
 */
void do_cmd_store(int Ind)
{
	player_type *p_ptr = Players[Ind];
	store_type *st_ptr;
	int			which;
	int i, j;
	int maintain_num;

	cave_type		*c_ptr;
	struct c_special *cs_ptr;

	/* Access the player grid */
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	i=gettown(Ind);
//	if(i==-1) return;	//DUNGEON STORES
	if(i == -1) i = 0;	// INITIATING MAD HACK >:) (C. Blue)

	/* Verify a store */
#if 0
	if (!((c_ptr->feat >= FEAT_SHOP_HEAD) &&
	      (c_ptr->feat <= FEAT_SHOP_TAIL)))
#endif	// 0
	if (c_ptr->feat != FEAT_SHOP)
	{
		msg_print(Ind, "You see no store here.");
		return;
	}

	/* Extract the store code */
//	which = (c_ptr->feat - FEAT_SHOP_HEAD);

	if((cs_ptr=GetCS(c_ptr, CS_SHOP)))
	{
		which = cs_ptr->sc.omni;
	}
	else
	{
		msg_print(Ind, "You see no store here.");
		return;
	}

	if (i != -1){
		/* Hack -- Check the "locked doors" */
		if (town[i].townstore[which].store_open >= turn)
		{
			msg_print(Ind, "The doors are locked.");
			return;
		}
	}

	/* Hack -- Ignore the home */
	if (which == 7)	/* XXX It'll change */
	{
		/* msg_print(Ind, "The doors are locked."); */
		return;
	}
#if 0	// it have changed
	else if (which > 7)	/* XXX It'll change */
	{
		msg_print(Ind, "A placard reads: Closed for inventory.");
		return;
	}
#endif	// 0

	/* No command argument */
	/*command_arg = 0;*/

	/* No repeated command */
	/*command_rep = 0;*/

	/* No automatic command */
	/*command_new = 0;*/

	if (gettown(Ind) != -1){
		st_ptr = &town[i].townstore[which];

		/* Make sure if someone is already in */
		for (i = 1; i <= NumPlayers; i++)
		{
			/* Check this player */
			j=gettown(i);
			if(j!=-1){
				if(st_ptr==&town[j].townstore[Players[i]->store_num])
				{
					msg_print(Ind, "The store is full.");
					store_kick(Ind, FALSE);
					return;
				}
			}
		}
	} else {
		//DUNGEON STORES
//		st_ptr =
		st_ptr = &town[i].townstore[which];

		/* Make sure if someone is already in */
		for (i = 1; i <= NumPlayers; i++)
		{
			/* Check this player */
			j=gettown(i);
			if(j==-1){
				if(st_ptr==&town[0].townstore[Players[i]->store_num])
				{
					msg_print(Ind, "The store is full.");
					store_kick(Ind, FALSE);
					return;
				}
			}
		}
	}

	/* Set the timer */
	p_ptr->tim_store = STORE_TURNOUT;

	/* Calculate the number of store maintainances since the last visit */
	maintain_num = (turn - st_ptr->last_visit) / (10L * STORE_TURNS);

	/* Maintain the store max. 10 times */
	if (maintain_num > 10) maintain_num = 10;

	if (maintain_num)
	{
		/* Maintain the store */
		for (i = 0; i < maintain_num; i++)
		{
//			store_maint(p_ptr->town_num, which);
			store_maint(st_ptr);
			if (retire_owner_p(st_ptr)) store_shuffle(st_ptr);
		}

		/* Save the visit */
		st_ptr->last_visit = turn;
	}

	/* Save the store number */
	p_ptr->store_num = which;

	/* Save the store and owner pointers */
	/*st_ptr = &store[p_ptr->store_num];
	ot_ptr = &owners[p_ptr->store_num][st_ptr->owner];*/

	if (p_ptr->tim_blacklist > 7000)
		msg_print(Ind, "As you enter, the owner gives you a murderous look.");
	else if (p_ptr->tim_blacklist > 5000)
		msg_print(Ind, "As you enter, the owner gazes at you angrily.");
	else if (p_ptr->tim_blacklist > 3000)
		msg_print(Ind, "As you enter, the owner eyes you suspiciously.");
	else if (p_ptr->tim_blacklist > 1000)
		msg_print(Ind, "As you enter, the owner looks at you disapprovingly.");
	else if (p_ptr->tim_blacklist)
		msg_print(Ind, "As you enter, the owner gives you a cool glance.");

	/* Display the store */
	display_store(Ind);

	/* Do not leave */
	leave_store = FALSE;
}



/*
 * Shuffle one of the stores.
 */
void store_shuffle(store_type *st_ptr)
{
	int i, j;
	//owner_type *ot_ptr;
	int tries = 100;

#if 0
	/* Ignore home */
	if (which == 7) return;
#endif

	/* Make sure no one is in the store */
	for (i = 1; i <= NumPlayers; i++)
	{
		/* Check this player */
		j=gettown(i);
		if(j!=-1){
			if(st_ptr==&town[j].townstore[Players[i]->store_num])
				return;
		}
		if(j==-1){	//DUNGEON STORES
			if(st_ptr==&town[0].townstore[Players[i]->store_num])
				return;
		}
	}

	/* Pick a new owner */
	for (j = st_ptr->owner; j == st_ptr->owner; )
	{	  
//		st_ptr->owner = rand_int(MAX_OWNERS);
		st_ptr->owner = st_info[st_ptr->st_idx].owners[rand_int(4)];
		if ((!(--tries))) break;
	}

	/* Activate the new owner */
	//	ot_ptr = &owners[st_ptr->st_idx][st_ptr->owner];
	//ot_ptr = &ow_info[st_ptr->owner];


	/* Reset the owner data */
	st_ptr->insult_cur = 0;
	st_ptr->store_open = 0;
	st_ptr->good_buy = 0;
	st_ptr->bad_buy = 0;


	/* Hack -- discount all the items */
	for (i = 0; i < st_ptr->stock_num; i++)
	{
		object_type *o_ptr;

		/* Get the item */
		o_ptr = &st_ptr->stock[i];

		/* Hack -- Cheapest goods like spikes won't be 'on sale' */
		if (object_value(0, o_ptr) < 5) continue;

		/* Hack -- Sell all old items for "half price" */
		o_ptr->discount = 50;

		/* Hack -- Items are no longer "fixed price" */
		o_ptr->ident &= ~ID_FIXED;

		/* Some stores offer less or no discount */
		if (st_info[st_ptr->st_idx].flags1 & SF1_NO_DISCOUNT) {
		        o_ptr->discount = 0;
		}
		else if (st_info[st_ptr->st_idx].flags1 & SF1_NO_DISCOUNT2) {
			o_ptr->discount = 15;
		        /* Mega-Hack -- Note that the item is "on sale" */
			o_ptr->note = quark_add("15% off");
		}
		else {
			/* Mega-Hack -- Note that the item is "on sale" */
			o_ptr->note = quark_add("on sale");
		}
	}
}


/*
 * Maintain the inventory at the stores.
 */
void store_maint(store_type *st_ptr)
{
//	int         i, j;
	int j;

	owner_type *ot_ptr;

	int tries = 200;

#if 0
	/* Ignore home */
	if (which == 7) return;
#endif

	/* Make sure no one is in the store */
#if 0 /* not used (cf.st_ptr->last_visit) */
	for (i = 1; i <= NumPlayers; i++)
	{
		/* Check this player */
		j=gettown(i);
		if(j!=-1){
			if(st_ptr==&town[j].townstore[Players[i]->store_num])
			{
				/* Somewhat hackie -- don't turn her/him out at once */
				if (magik(40) && Players[i]->tim_store < STORE_TURNOUT / 2)
					store_kick(i, TRUE);
				else return;
			}
		}
		if(j==-1){	//DUNGEON STORES
			if(st_ptr==&town[0].townstore[Players[i]->store_num])
			{
				if (magik(40) && Players[i]->tim_store < STORE_TURNOUT / 2)
					store_kick(i, TRUE);
				else return;
			}
	}
#endif	// 0

	/* Ignore Museum */
	if (st_info[st_ptr->st_idx].flags1 & SF1_MUSEUM) return;

	/* Activate the owner */
//	ot_ptr = &owners[st_ptr->st_idx][st_ptr->owner];
	ot_ptr = &ow_info[st_ptr->owner];


	/* Store keeper forgives the player */
	st_ptr->insult_cur = 0;


	/* Mega-Hack -- prune the black market */
//	if (st_ptr->st_idx == 6)
	if (st_info[st_ptr->st_idx].flags1 & SF1_ALL_ITEM)
	{
		/* Destroy crappy black market items */
		for (j = st_ptr->stock_num - 1; j >= 0; j--)
		{
			object_type *o_ptr = &st_ptr->stock[j];

			/* Destroy crappy items */
			if (black_market_crap(o_ptr))
			{
				/* Destroy the item */
				store_item_increase(st_ptr, j, 0 - o_ptr->number);
				store_item_optimize(st_ptr, j);
			}
		}
	}


	/* Choose the number of slots to keep */
	j = st_ptr->stock_num;

	/* Sell a few items */
	j = j - randint(STORE_TURNOVER);

	/* Never keep more than "STORE_MAX_KEEP" slots */
	if (j > STORE_MAX_KEEP) j = STORE_MAX_KEEP;

	/* Always "keep" at least "STORE_MIN_KEEP" items */
	if (j < STORE_MIN_KEEP) j = STORE_MIN_KEEP;

	/* Hack -- prevent "underflow" */
	if (j < 0) j = 0;

	/* Destroy objects until only "j" slots are left */
	while (st_ptr->stock_num > j) store_delete(st_ptr);


	/* Choose the number of slots to fill */
	j = st_ptr->stock_num;

	/* Buy some more items */
	j = j + randint(STORE_TURNOVER);

	/* Never keep more than "STORE_MAX_KEEP" slots */
	if (j > STORE_MAX_KEEP) j = STORE_MAX_KEEP;

	/* Always "keep" at least "STORE_MIN_KEEP" items */
	if (j < STORE_MIN_KEEP) j = STORE_MIN_KEEP;

	/* Hack -- prevent "overflow" */
	if (j >= st_ptr->stock_size) j = st_ptr->stock_size - 1;

	/* Acquire some new items */
	/* We want speed & healing & mana pots in the BM */
	while (st_ptr->stock_num < j)
	  {
	    store_create(st_ptr);
       	    tries--;
	      if (!tries) break;
	  }
}


/*
 * Initialize the stores
 */
void store_init(store_type *st_ptr)
{
	int         k;
	owner_type *ot_ptr;


	/* Pick an owner */
//	st_ptr->owner = rand_int(MAX_OWNERS);
	st_ptr->owner = st_info[st_ptr->st_idx].owners[rand_int(4)];

	/* Activate the new owner */
//	ot_ptr = &owners[st_ptr->st_idx][st_ptr->owner];
	ot_ptr = &ow_info[st_ptr->owner];


	/* Initialize the store */
	st_ptr->store_open = 0;
	st_ptr->insult_cur = 0;
	st_ptr->good_buy = 0;
	st_ptr->bad_buy = 0;

	/* Nothing in stock */
	st_ptr->stock_num = 0;

	/*
	 * MEGA-HACK - Last visit to store is
	 * BEFORE player birth to enable store restocking
	 */
	/* so let's not employ it :) */
//	st_ptr->last_visit = -100L * STORE_TURNS;

	/* Clear any old items */
	for (k = 0; k < st_ptr->stock_size; k++)
	{
		invwipe(&st_ptr->stock[k]);
	}
}

		
void store_kick(int Ind, bool say)
{
	player_type *p_ptr = Players[Ind];
	if (say) msg_print(Ind, "The shopkeeper asks you to leave the store once.");
	//				store_leave(Ind);
	p_ptr->store_num = -1;
	Send_store_kick(Ind);
	teleport_player(Ind, 1);
}

void store_exec_command(int Ind, int action, int item, int item2, int amt, int gold)
{
	player_type *p_ptr = Players[Ind];
	store_type *st_ptr;
	int i;

	/* MEGAHACK -- accept house extension command */
	if (p_ptr->store_num==7)
	{
		if (ba_info[action].action == BACT_EXTEND_HOUSE) home_extend(Ind);
		return;
	}


	i=gettown(Ind);
//	if(i==-1) return;       //DUNGEON STORES
        if(i==-1) i=0;

	/* sanity check - Yakina - */
	if (p_ptr->store_num==-1){
		msg_print(Ind,"You left the building!");
		return;
	}

	st_ptr = &town[i].townstore[p_ptr->store_num];

	/* Mockup */
#if DEBUG_LEVEL > 2
	//msg_format(Ind, "Store command received: %d, %d, %d, %d, %d", action, item, item2, amt, gold);
	s_printf("Store command received: %d, %d, %d, %d, %d\n", action, item, item2, amt, gold);
#endif	// DEBUG_LEVEL

	/* Is the action legal? */
	//if (!store_attest_command(p_ptr->store_num, action)) return;
	if (!store_attest_command(p_ptr->store_num, ba_info[action].action))
		return;

	bldg_process_command(Ind, st_ptr, action, item, item2, amt, gold);
}

#ifndef USE_MANG_HOUSE_ONLY
/*
 * TomeNET functions for 'Vanilla-like' houses.		- Jir -
 *
 * Since hundreds of houses exist for dozens of players, we cannot handle
 * houses as 'one of stores'; these functions are reimplementation of store
 * functions using different arrays in houses[].
 *
 * Another possble solution is to make houses[] have store_type arrays, and
 * rewrite store functions to use store pointer;
 * this should be reconsidered after new ToME store code comes.
 *
 * (Yet another is to make struct like
 * store_contents
 * {
 *		s16b stock_num;
 *		s16b stock_size;
 *		object_type *stock;
 * };
 * and to make both house_type and store_type contain this)
 *
 * Unlike server-side, client will handle house as (almost) normal store.
 */
/*
 * Also, probably it's not so good design to bind store arrays to towns
 * (town_type).  This will eventually be changed, so that we can place stores
 * everywhere in the wilderness/dungeons.
 */

/*
 * Add the item "o_ptr" to the inventory of the "Home"
 *
 * In all cases, return the slot (or -1) where the object was placed
 *
 * Note that this is a hacked up version of "inven_carry()".
 *
 * Also note that it may not correctly "adapt" to "knowledge" bacoming
 * known, the player may have to pick stuff up and drop it again.
 */
/* It's ToME code */
static int home_carry(int Ind, house_type *h_ptr, object_type *o_ptr)
{
	int 				slot;
	s32b			   value, j_value;
	int 	i;
	object_type *j_ptr;


	/* Check each existing item (try to combine) */
	for (slot = 0; slot < h_ptr->stock_num; slot++)
	{
		/* Get the existing item */
		j_ptr = &h_ptr->stock[slot];

		/* The home acts just like the player */
		if (object_similar(Ind, j_ptr, o_ptr))
		{
			/* Save the new number of items */
			object_absorb(Ind, j_ptr, o_ptr);

			/* All done */
			return (slot);
		}
	}

	/* No space? */
	if (h_ptr->stock_num >= h_ptr->stock_size) return (-1);


	/* Determine the "value" of the item */
	value = object_value(Ind, o_ptr);

	/* Check existing slots to see if we must "slide" */
	for (slot = 0; slot < h_ptr->stock_num; slot++)
	{
		/* Get that item */
		j_ptr = &h_ptr->stock[slot];

		/* Hack -- readable books always come first */
#if 0
		if ((o_ptr->tval == cp_ptr->spell_book) &&
			(j_ptr->tval != cp_ptr->spell_book)) break;
		if ((j_ptr->tval == cp_ptr->spell_book) &&
			(o_ptr->tval != cp_ptr->spell_book)) continue;
#endif	// 0

		/* Objects sort by decreasing type */
		if (o_ptr->tval > j_ptr->tval) break;
		if (o_ptr->tval < j_ptr->tval) continue;

		/* Can happen in the home */
		if (!object_aware_p(Ind, o_ptr)) continue;
		if (!object_aware_p(Ind, j_ptr)) break;

		/* Objects sort by increasing sval */
		if (o_ptr->sval < j_ptr->sval) break;
		if (o_ptr->sval > j_ptr->sval) continue;

		/* Objects in the home can be unknown */
		if (!object_known_p(Ind, o_ptr)) continue;
		if (!object_known_p(Ind, j_ptr)) break;


		/*
		 * Hack:  otherwise identical rods sort by
		 * increasing recharge time --dsb
		 */
#if 0
		if (o_ptr->tval == TV_ROD_MAIN)
		{
			if (o_ptr->timeout < j_ptr->timeout) break;
			if (o_ptr->timeout > j_ptr->timeout) continue;
		}
#endif	// 0

		/* Objects sort by decreasing value */
		j_value = object_value(Ind, j_ptr);
		if (value > j_value) break;
		if (value < j_value) continue;
	}

	/* Slide the others up */
	for (i = h_ptr->stock_num; i > slot; i--)
	{
		h_ptr->stock[i] = h_ptr->stock[i-1];
	}

	/* More stuff now */
	h_ptr->stock_num++;

	/* Insert the new item */
	h_ptr->stock[slot] = *o_ptr;

	/* Return the location */
	return (slot);
}

/*
 * Increase, by a given amount, the number of a certain item
 * in a certain store.  This can result in zero items.
 */
static void home_item_increase(house_type *h_ptr, int item, int num)
{
	int         cnt;
	object_type *o_ptr;

	/* Get the item */
	o_ptr = &h_ptr->stock[item];

	/* Verify the number */
	cnt = o_ptr->number + num;
	if (cnt > 255) cnt = 255;
	else if (cnt < 0) cnt = 0;
	num = cnt - o_ptr->number;

	/* Save the new number */
	o_ptr->number += num;
}


/*
 * Remove a slot if it is empty
 */
static void home_item_optimize(house_type *h_ptr, int item)
{
	int         j;
	object_type *o_ptr;

	/* Get the item */
	o_ptr = &h_ptr->stock[item];

	/* Must exist */
	if (!o_ptr->k_idx) return;

	/* Must have no items */
	if (o_ptr->number) return;

	/* One less item */
	h_ptr->stock_num--;

	/* Slide everyone */
	for (j = item; j < h_ptr->stock_num; j++)
	{
		h_ptr->stock[j] = h_ptr->stock[j + 1];
	}

	/* Nuke the final slot */
	invwipe(&h_ptr->stock[j]);
}

/*
 * Re-displays a single store entry
 *
 * Actually re-sends a single store entry --KLJ--
 */
//static bool store_check_num_house(store_type *st_ptr, object_type *o_ptr)
static bool home_check_num(int Ind, house_type *h_ptr, object_type *o_ptr)
{
	int        i;
	object_type *j_ptr;

	/* Free space is always usable */
	if (h_ptr->stock_num < h_ptr->stock_size) return TRUE;

	/* The "home" acts like the player */
	{
		/* Check all the items */
		for (i = 0; i < h_ptr->stock_num; i++)
		{
			/* Get the existing item */
			j_ptr = &h_ptr->stock[i];

			/* Can the new object be combined with the old one? */
			if (object_similar(Ind, j_ptr, o_ptr)) return (TRUE);
		}
	}
	return (FALSE);
}

/*
 * Sell an item to the store (or home)
 */
void home_sell(int Ind, int item, int amt)
{
	player_type *p_ptr = Players[Ind];

	int			h_idx, item_pos;

	object_type		sold_obj;
	object_type		*o_ptr;

	char		o_name[160];

	house_type *h_ptr;

	/* This should never happen */
	if (p_ptr->store_num != 7)
	{
		msg_print(Ind,"You left the house!");
		return;
	}

	h_idx = pick_house(&p_ptr->wpos, p_ptr->py, p_ptr->px);
	if (h_idx == -1) return;

	h_ptr = &houses[h_idx];


	/* You can't sell 0 of something. */
	if (amt <= 0) return;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else /* Never.. */
	{
		o_ptr = &o_list[0 - item];
	}

#if 1

	/* Not gonna happen XXX inscribe */
	/* Nah, gonna happen */
	/* TODO: CURSE_NO_DROP */
	if (cursed_p(o_ptr) && !is_admin(p_ptr))
	{
		u32b f1, f2, f3, f4, f5, esp;
		if (item >= INVEN_WIELD)
		{
			/* Oops */
			msg_print(Ind, "Hmmm, it seems to be cursed.");

			/* Stop */
			return;
		}

		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

		if (f4 & TR4_CURSE_NO_DROP)
		{
			/* Oops */
			msg_print(Ind, "Hmmm, you seem to be unable to drop it.");

			/* Nope */
			return;
		}
	}

#endif

	if (cfg.anti_arts_house && true_artifact_p(o_ptr))
	{
		/* Oops */
		msg_print(Ind, "You cannot stock artifacts.");

		/* Stop */
		return;
	}

	/* Create the object to be sold (structure copy) */
	sold_obj = *o_ptr;
	sold_obj.number = amt;

	/* Get a full description */
	object_desc(Ind, o_name, &sold_obj, TRUE, 3);

	/* Is there room in the store (or the home?) */
	if (!home_check_num(Ind, h_ptr, &sold_obj))
	{
		msg_print(Ind, "Your home is full.");
		return;
	}



	/* Get the inventory item */
	o_ptr = &p_ptr->inventory[item];

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/*
	 * Hack -- If rods or wands are dropped, the total maximum timeout or 
	 * charges need to be allocated between the two stacks.  If all the items 
	 * are being dropped, it makes for a neater message to leave the original 
	 * stack's pval alone. -LM-
	 */
	if (o_ptr->tval == TV_WAND)
	{
		if (o_ptr->tval == TV_WAND)
		{
			sold_obj.pval = divide_charged_item(o_ptr, amt);
		}
	}

	/* Get the description all over again */
	object_desc(Ind, o_name, &sold_obj, TRUE, 3);

	/* Describe the result (in message buffer) */
//		msg_format("You drop %s (%c).", o_name, index_to_label(item));
	msg_format(Ind, "You drop %s.", o_name);

	/* Analyze the prices (and comment verbally) */
	/*purchase_analyze(price, value, dummy);*/

	/* Take the item from the player, describe the result */
	inven_item_increase(Ind, item, -amt);
	inven_item_describe(Ind, item);
	inven_item_optimize(Ind, item);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Artifact won't be sold in a store */
	if (cfg.anti_arts_shop && true_artifact_p(&sold_obj))
	{
		a_info[sold_obj.name1].cur_num = 0;
		a_info[sold_obj.name1].known = FALSE;
		return;
	}

	/* The store gets that (known) item */
//	if(sold_obj.tval!=8)	// What was it for.. ?
	item_pos = home_carry(Ind, h_ptr, &sold_obj);
//		item_pos = store_carry(p_ptr->store_num, &sold_obj);

	/* Resend the basic store info */
	display_trad_house(Ind);

	/* Re-display if item is now in store */
	if (item_pos >= 0)
	{
		display_house_inventory(Ind);
	}
}


/*
 * Buy an item from a store				-RAK-
 */
void home_purchase(int Ind, int item, int amt)
{
	player_type *p_ptr = Players[Ind];


	int			i;
	int			item_new;
	int h_idx;

	object_type		sell_obj;
	object_type		*o_ptr;

	char		o_name[160];

	house_type *h_ptr;

	if (amt < 1)
	{
		s_printf("$INTRUSION$ Bad amount %d! (Home) Bought by %s.", amt, p_ptr->name);
		msg_print(Ind, "\377RInvalid amount. Your attempt has been logged.");
		return;
	}

	if (p_ptr->store_num==-1){
		msg_print(Ind,"You left the shop!");
		return;
	}

	/* This should never happen */
	if (p_ptr->store_num != 7) return;

	h_idx = pick_house(&p_ptr->wpos, p_ptr->py, p_ptr->px);
	if (h_idx == -1) return;

	h_ptr = &houses[h_idx];

	/* Empty? */
	if (h_ptr->stock_num <= 0)
	{
		msg_print(Ind, "Your home is empty.");
		return;
	}


	/* Get the actual item */
	o_ptr = &h_ptr->stock[item];

	/* Check if the player is powerful enough for that item */
        if ((o_ptr->owner) && (o_ptr->owner != p_ptr->id) && (o_ptr->level > p_ptr->lev || o_ptr->level == 0))
        {
                if (cfg.anti_cheeze_pickup)
                {
                        msg_print(Ind, "You aren't powerful enough yet to pick up that item!");
                        return;
                }
                if (true_artifact_p(o_ptr) && cfg.anti_arts_pickup)
                {
                        msg_print(Ind, "You aren't powerful enough yet to pick up that artifact!");
                        return;
                }
	}

	/* Assume the player wants just one of them */
	/*amt = 1;*/

	/* Hack -- get a "sample" object */
	sell_obj = *o_ptr;
	sell_obj.number = amt;

	/*
	 * Hack -- If rods or wands are dropped, the total maximum timeout or 
	 * charges need to be allocated between the two stacks.  If all the items 
	 * are being dropped, it makes for a neater message to leave the original 
	 * stack's pval alone. -LM-
	 */
	if (o_ptr->tval == TV_WAND)
	{
		if (o_ptr->tval == TV_WAND)
		{
			sell_obj.pval = divide_charged_item(o_ptr, amt);
		}
	}

	/* Hack -- require room in pack */
	if (!inven_carry_okay(Ind, &sell_obj))
	{
		msg_print(Ind, "You cannot carry that many different items.");
		return;
	}



	/* Home is much easier */
	{
		/* Carry the item */
		item_new = inven_carry(Ind, &sell_obj);

		/* Describe just the result */
		object_desc(Ind, o_name, &p_ptr->inventory[item_new], TRUE, 3);

		/* Message */
		msg_format(Ind, "You have %s (%c).", o_name, index_to_label(item_new));

		/* Handle stuff */
		handle_stuff(Ind);

		/* Take note if we take the last one */
		i = h_ptr->stock_num;

		/* Remove the items from the home */
		home_item_increase(h_ptr, item, -amt);
		home_item_optimize(h_ptr, item);

		/* Resend the basic store info */
		display_trad_house(Ind);

		/* Hack -- Item is still here */
		if (i == h_ptr->stock_num && i)
		{
			/* Redraw the item */
			display_house_entry(Ind, item);
		}

		/* The item is gone */
		else
		{
			/* Redraw everything */
			display_house_inventory(Ind);
		}
	}

	/* Not kicked out */
	return;
}


/*
 * Examine an item in a store			   -JDL-
 */
void home_examine(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

//	int st = p_ptr->store_num;

	int	h_idx;

	object_type		sell_obj;
	object_type		*o_ptr;

	char		o_name[160];

	house_type *h_ptr;

	/* This should never happen */
	if (p_ptr->store_num != 7) return;

	h_idx = pick_house(&p_ptr->wpos, p_ptr->py, p_ptr->px);
	if (h_idx == -1) return;

	h_ptr = &houses[h_idx];

	/* Empty? */
	if (h_ptr->stock_num <= 0)
	{
		msg_print(Ind, "Your home is empty.");
		return;
	}

	/* Get the actual item */
	o_ptr = &h_ptr->stock[item];

	/* Assume the player wants just one of them */
	/*amt = 1;*/

	/* Hack -- get a "sample" object */
	sell_obj = *o_ptr;

	/* Description */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	/* Require full knowledge */
	if (!(o_ptr->ident & (ID_MENTAL)))
	{
		/* Describe */
		msg_format(Ind, "%s\n", o_name);
		if (strlen(o_name) > 77) msg_format(Ind, "%s\n", o_name + 77);

                switch(o_ptr->tval){
	        case TV_HAFTED:
    		        msg_print(Ind, "It's a hafted weapon."); break;
                case TV_POLEARM:
	        	msg_print(Ind, "It's a polearm."); break;
    	        case TV_SWORD:
        	        msg_print(Ind, "It's a sword-type weapon."); break;
                case TV_AXE:
	                msg_print(Ind, "It's an axe-type weapon."); break;
        	default:
	                if (wield_slot(Ind, o_ptr) != INVEN_WIELD) msg_print(Ind, "You have no special knowledge about that item.");
        	        break;
	        }
		/* This can only happen in the home */
		if (wield_slot(Ind, o_ptr) == INVEN_WIELD)
		{
			int blows = calc_blows(Ind, o_ptr);
			msg_format(Ind, "With it, you can usually attack %d time%s/turn.",
					blows, blows > 1 ? "s" : "");
		}
		else msg_print(Ind, "You have no special knowledge about that item.");
		return;
	}

	/* Describe it fully */
	if (!identify_fully_aux(Ind, o_ptr)) msg_print(Ind, "You see nothing special.");

#if 0
	if (o_ptr->tval < TV_BOOK)
	{
		if (!identify_fully_aux(Ind, o_ptr)) msg_print(Ind, "You see nothing special.");
		/* Books are read */
	}
	else
	{
		do_cmd_browse_aux(o_ptr);
	}
#endif	// 0

	return;
}

/* Extend the house!	- Jir - */
/* TODO:
 * - remove some dirty hacks
 * - tell players of the cost in advance
 */
void home_extend(int Ind)
{
	player_type *p_ptr = Players[Ind];

//	int st = p_ptr->store_num;

	int	h_idx, cost = 0;

	house_type *h_ptr;

	/* This should never happen */
	if (p_ptr->store_num != 7) return;

	h_idx = pick_house(&p_ptr->wpos, p_ptr->py, p_ptr->px);
	if (h_idx == -1) return;

	h_ptr = &houses[h_idx];

	/* Already too large? */
	if (h_ptr->stock_size >= STORE_INVEN_MAX)
	{
		msg_print(Ind, "Your house cannot be extended any more.");
		return;
	}

	cost = h_ptr->dna->price * 2 / (h_ptr->stock_size + 1);

	if (p_ptr->au < cost)
	{
		msg_print(Ind, "You couldn't afford it..");
		return;
	}

	GROW(h_ptr->stock, h_ptr->stock_size, h_ptr->stock_size + 1, object_type);
	h_ptr->stock_size++;
	p_ptr->au -= cost;
	h_ptr->dna->price += cost;

	msg_format(Ind, "You extend your house for %dau.", cost);

	display_trad_house(Ind);

	/* Display the current gold */
	store_prt_gold(Ind);
}


static void display_house_entry(int Ind, int pos)
{
	player_type *p_ptr = Players[Ind];

	object_type		*o_ptr;

	char		o_name[160];
	byte		attr;
	int		wgt;
	int		h_idx;

	int maxwid = 75;

	house_type *h_ptr;

	/* This should never happen */
	if (p_ptr->store_num != 7) return;

	h_idx = pick_house(&p_ptr->wpos, p_ptr->py, p_ptr->px);
	if (h_idx == -1) return;

	h_ptr = &houses[h_idx];



	/* Get the item */
	o_ptr = &h_ptr->stock[pos];

#if 0
	/* Get the "offset" */
	i = (pos % 12);

	/* Label it, clear the line --(-- */
	(void)sprintf(out_val, "%c) ", I2A(i));
	prt(out_val, i+6, 0);
#endif


	/* Describe the object */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);
	o_name[maxwid] = '\0';

	attr = tval_to_attr[o_ptr->tval];

	/* Only show the weight of an individual item */
	wgt = o_ptr->weight;

	/* Send the info */
	Send_store(Ind, pos, attr, wgt, o_ptr->number, 0, o_name, o_ptr->tval, o_ptr->sval, o_ptr->pval);
}


/*
 * Displays a store's inventory			-RAK-
 * All prices are listed as "per individual object".  -BEN-
 *
 * The inventory is "sent" not "displayed". -KLJ-
 */
static void display_house_inventory(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int k;
	int h_idx;
	
	house_type *h_ptr;

	/* This should never happen */
	if (p_ptr->store_num != 7) return;

	h_idx = pick_house(&p_ptr->wpos, p_ptr->py, p_ptr->px);
	if (h_idx == -1) return;

	h_ptr = &houses[h_idx];


	/* Display the next 48 items */
	for (k = 0; k < STORE_INVEN_MAX; k++)
	{
		/* Do not display "dead" items */
		/* But tell the client that it's empty now */
		if (k && k >= h_ptr->stock_num) break;

		/* Display that line */
		display_house_entry(Ind, k);
	}
}

/*
 * Displays store (after clearing screen)		-RAK-
 */
static void display_trad_house(int Ind)
{
	player_type *p_ptr = Players[Ind];
	house_type *h_ptr;
	int h_idx;

	/* This should never happen */
	if (p_ptr->store_num != 7) return;

	h_idx = pick_house(&p_ptr->wpos, p_ptr->py, p_ptr->px);
	if (h_idx == -1) return;

	h_ptr = &houses[h_idx];

	/* Display the current gold */
	store_prt_gold(Ind);

	/* Draw in the inventory */
	display_house_inventory(Ind);

	/* Hack -- Send the store actions info */
	/* XXX it's dirty hack -- the aim is to avoid
	 * hard-coded stuffs in the client */
	show_building(Ind, &town[0].townstore[7]);

	/* Send the store info */
	//Send_store_info(Ind, p_ptr->store_num, 0, h_ptr->stock_num);
	Send_store_info(Ind, p_ptr->store_num, "Your House", "", h_ptr->stock_num, h_ptr->stock_size);
}

/* Enter a house, and interact with it.	- Jir - */
void do_cmd_trad_house(int Ind)
{
	player_type *p_ptr = Players[Ind];

	cave_type		*c_ptr;
	struct c_special *cs_ptr;
	house_type *h_ptr;
	int h_idx;

	/* Access the player grid */
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* Verify a store */
#if 0
	if (!((c_ptr->feat >= FEAT_SHOP_HEAD) &&
	      (c_ptr->feat <= FEAT_SHOP_TAIL)))
#endif	// 0
#if 0
	if (c_ptr->feat != FEAT_HOME)
	{
		msg_print(Ind, "You see no house here.");
		return;
	}
#endif	// 0

	if (c_ptr->feat == FEAT_HOME)
	{
		/* Check access like former move_player */
		if((cs_ptr=GetCS(c_ptr, CS_DNADOOR))) /* orig house failure */
		{
			if(!access_door(Ind, cs_ptr->sc.ptr) && !admin_p(Ind))
			{
				msg_print(Ind, "\377oThe door is locked.");
				return;
			}
		}
		else return;
	}
	/* Open door == free access */
	else if (c_ptr->feat == FEAT_HOME_OPEN)
	{
		/* Check access like former move_player */
		if(!(cs_ptr=GetCS(c_ptr, CS_DNADOOR))) return;
	}
	/* Not a house */
	else return;

	/* Check if it's traditional house */
	h_idx = pick_house(&p_ptr->wpos, p_ptr->py, p_ptr->px);

	if (h_idx == -1) return;

	h_ptr = &houses[h_idx];
	if (!(h_ptr->flags & HF_TRAD)) return;

	if(p_ptr->inval){
		msg_print(Ind, "You may not use a house. Ask an admin to validate your account.");
		return;
	}


	/* Save the store number */
	p_ptr->store_num = 7;

	/* Set the timer */
	/* XXX well, don't kick her out of her own house :) */
	p_ptr->tim_store = 30000;

	/* Display the store */
	display_trad_house(Ind);

	/* Do not leave */
	leave_store = FALSE;
}



#endif	// USE_MANG_HOUSE_ONLY
