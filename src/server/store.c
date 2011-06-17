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
#define STORE_BENEFIT		10

/* Extra check for ego item generation in store, in percent.	[50] */
#define STORE_EGO_CHANCE	50

/* Maximum amount of maintenances to perform if a shop was unvisited for too long a period of time [10]
   Adjusting it to balance art scroll rarity in EBM - C. Blue (see EBM in st_info.txt for more) */
#define MAX_MAINTENANCES	10

/* Prevent items in stores which got a power < 0 in apply_magic() yet didn't turn out CURSED?
   These items will end up with very low boni, below their k_info values, and therefore be somewhat
   useless, since they won't be bought anyway. - C. Blue */
#define NO_PSEUDO_CURSED_ITEMS


static int gettown(int Ind);
static int gettown_dun(int Ind);

void home_sell(int Ind, int item, int amt);
void home_purchase(int Ind, int item, int amt);
void home_examine(int Ind, int item);
static void display_house_entry(int Ind, int pos, house_type *h_ptr);
static void display_house_inventory(int Ind, house_type *h_ptr);
static void display_trad_house(int Ind, house_type *h_ptr);
void home_extend(int Ind);
#ifdef PLAYER_STORES
static u32b player_store_inscribed(object_type *o_ptr, u32b price);
static void player_store_handle_purchase(int Ind, object_type *o_ptr, object_type *s_ptr, u32b value);
#endif


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
		
		/* remember town assignment */
		st_ptr->town = townval;

		/* Assume full stock */
		st_ptr->stock_size = sti_ptr->max_obj;

		/* Allocate the stock */
		C_MAKE(st_ptr->stock, st_ptr->stock_size, object_type);
	}
}

/* For containing memory leaks - mikaelh */
void dealloc_stores(int townval)
{
	int i;

	/* Check that stores exist */
	if (!town[townval].townstore) return;

	for (i = 0; i < max_st_idx; i++)
	{
		store_type *st_ptr = &town[townval].townstore[i];

		/* Free stock */
		C_KILL(st_ptr->stock, st_ptr->stock_size, object_type);
	}

	/* Free stores */
	C_KILL(town[townval].townstore, max_st_idx, store_type);
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
static s64b price_item(int Ind, object_type *o_ptr, int greed, bool flip)
{
	player_type *p_ptr = Players[Ind];
	owner_type *ot_ptr;
	store_type *st_ptr;
	int     factor;
	int     adjust;
	s64b    price;
	int i;

	i = gettown(Ind);
	/* hack: non-town stores (ie dungeon, but could also be wild) are borrowed from town #0 - C. Blue */
	if (i == -1) i = gettown_dun(Ind);

	st_ptr = &town[i].townstore[p_ptr->store_num];
	ot_ptr = &ow_info[st_ptr->owner];

	/* Get the value of one of the items */
	price = object_value(flip ? Ind : 0, o_ptr);

	/* Worthless items */
	if (price <= 0) return (0L);

	/* Compute the racial factor */
	if (is_state(Ind, st_ptr, STORE_LIKED)) {
		factor = ot_ptr->costs[STORE_LIKED];
	} else if (is_state(Ind, st_ptr, STORE_HATED)) {
		factor = ot_ptr->costs[STORE_HATED];
	} else {
		factor = ot_ptr->costs[STORE_NORMAL];
	}

	/* Add in the charisma factor */
	factor += adj_chr_gold[p_ptr->stat_ind[A_CHR]];


	/* Shop is buying */
	if (flip) {
		/* Adjust for greed */
		adjust = 100 + (300 - (greed + factor));//greed+factor: 150..250 (flip=TRUE)

#if 0 /* disabled for now to make things look more consistent (eg law dsm of def selling for less than law dsm) */
		/* Hack -- Shopkeepers hate higher level-requirement */
		adjust += (20 - o_ptr->level) / 2 ;
#endif

		/* Never get "silly" */
		if (adjust > 100 - STORE_BENEFIT) adjust = 100 - STORE_BENEFIT;

		/* Mega-Hack -- Black market sucks */
		if (st_info[st_ptr->st_idx].flags1 & SF1_ALL_ITEM) price /= 4;

		/* Seasoned Tradesman doesn't pay very much either, he knows the customers can't disagree.. */
		if (st_info[st_ptr->st_idx].flags1 & SF1_BUY67) price = (price * 2) / 3;
		if (st_info[st_ptr->st_idx].flags1 & SF1_BUY50) price /= 2;

		/* You're not a welcomed customer.. */
		if (p_ptr->tim_blacklist) price = price / 4;

		/* To prevent cheezing */
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)){
			if(o_ptr->pval == 0) {
				price = 1000;
			} else {
				price = 1000 + r_info[o_ptr->pval].level * 100;
			}
		}
	}

	/* Shop is selling */
	else {
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

#ifdef PLAYER_STORES
static s64b price_item_player_store(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];
	store_type *st_ptr;
	s64b price, final_price;

	st_ptr = &fake_store[-2 - p_ptr->store_num];

	/* Get the value of one of the items */
	price = object_value_real(0, o_ptr);

	/* Player stores have an increased minimum price */
	price *= 2;

	/* Add to this any extra price the player inscribed */
	final_price = player_store_inscribed(o_ptr, price);

	/* Return the price */
	return (final_price);
}
#endif

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

	s64b cost = object_value(0, o_ptr);


	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Food, Flasks, and Lites */
		case TV_FOOD:
		case TV_FLASK:
		case TV_LITE:
		case TV_PARCHMENT:
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
			if (st_ptr->st_idx == STORE_BTSUPPLY &&
			    (o_ptr->sval == SV_POTION_STAR_HEALING ||
			    o_ptr->sval == SV_POTION_RESTORE_MANA /* for mages - mikaelh */
			    ))
				size += mass_roll(3, 5);
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
		case TV_BLUNT:
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
	if (compat_omode(o_ptr, j_ptr)) return (0);

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
	
	/* require same seed */
	if (o_ptr->name3 != j_ptr->name3) return (0);

	/* Hack -- Never stack "powerful" items */
	if (o_ptr->xtra1 || j_ptr->xtra1) return (0);

	/* Hack -- Never stack recharging items */
	/* Megahack -- light sources are allowed (hoping it's
	 * not non-fuel activable one..) */
	if (o_ptr->timeout != j_ptr->timeout) return (0);

	/* Require many identical values */
	if (o_ptr->ac    !=  j_ptr->ac)   return (0);
	if (o_ptr->dd    !=  j_ptr->dd)   return (0);
	if (o_ptr->ds    !=  j_ptr->ds)   return (0);

	/* Hack -- Never stack chests */
	if (o_ptr->tval == TV_CHEST) return (0);

	/* Hack -- Never stack 'used' custom tomes */
	if (o_ptr->tval == TV_BOOK && o_ptr->sval >= SV_CUSTOM_TOME_1 &&
	    o_ptr->sval < SV_SPELLBOOK && o_ptr->xtra1)
		return(0);
	if (j_ptr->tval == TV_BOOK && j_ptr->sval >= SV_CUSTOM_TOME_1 &&
	    j_ptr->sval < SV_SPELLBOOK && j_ptr->xtra1)
		return(0);

	/* cheques may have different value, so they must not stack */
	if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_CHEQUE) return FALSE;

	/* Require matching discounts */
	if (o_ptr->discount != j_ptr->discount) return (0);

#ifdef PLAYER_STORES
	/* Different inscriptions can be used to prevent stacking
	   and thereby customize pile sizes :) */
	if (o_ptr->note != j_ptr->note) return (0);
#endif

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

#ifdef PLAYER_STORES
	/* player stores don't buy anything */
	if (p_ptr->store_num <= -2) return FALSE;
#endif

	/* Hack: The Mathom House */
	if (st_info[p_ptr->store_num].flags1 & SF1_MUSEUM) {
		/* Museums won't buy true artifacts, since they'd be
		   conserved and out of reach for players thereby. */
		if (true_artifact_p(o_ptr)) return FALSE;
		/* Museum does accept even worthless donations.. */
		return TRUE;
	}

	/* Switch on the store */
	switch (p_ptr->store_num) {
		/* General Store */
		case STORE_GENERAL:
			/* Analyze the type */
			switch (o_ptr->tval) {
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
				case TV_PARCHMENT:
				/* and now new... :) */
				case TV_TRAPKIT:
				case TV_BOOMERANG:
				break;
				default:
				return (FALSE);
			}
			break;

		/* Armoury */
		case STORE_ARMOURY:
			/* Analyze the type */
			switch (o_ptr->tval) {
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

		/* Weapon Shop */
		case STORE_WEAPON:
			/* Analyze the type */
			switch (o_ptr->tval) {
				case TV_SHOT:
				case TV_BOLT:
				case TV_ARROW:
				case TV_BOW:
				case TV_BLUNT:
				case TV_POLEARM:
				case TV_SWORD:
				case TV_AXE:
//				case TV_MSTAFF:
				case TV_BOOMERANG:
				break;
				default:
				return (FALSE);
			}
			break;

		/* Temple */
		case STORE_TEMPLE:
			/* Analyze the type */
			switch (o_ptr->tval) {
				case TV_BOOK:
					if (get_book_name_color(Ind, o_ptr) != TERM_GREEN &&
					    get_book_name_color(Ind, o_ptr) != TERM_WHITE) /* unused custom books */
						 return FALSE;
				case TV_SCROLL:
				case TV_POTION:
				case TV_POTION2:
				case TV_BLUNT:
				break;
				default:
				return (FALSE);
			}
			break;

		/* Alchemist */
		case STORE_ALCHEMIST:
			/* Analyze the type */
			switch (o_ptr->tval) {
				case TV_SCROLL:
				case TV_POTION:
				case TV_POTION2:
				break;
				default:
				return (FALSE);
			}
			break;

		/* Magic Shop */
		case STORE_MAGIC:
			/* Analyze the type */
			switch (o_ptr->tval) {
				case TV_BOOK:
					if (get_book_name_color(Ind, o_ptr) != TERM_L_BLUE &&
					    get_book_name_color(Ind, o_ptr) != TERM_WHITE) /* unused custom books */
						return FALSE;
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

		/* Bookstore Shop */
		case STORE_BOOK:
			/* Analyze the type */
			switch (o_ptr->tval) {
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

		/* Pet Shop */
		case STORE_PET:
			/* Analyze the type */
			switch (o_ptr->tval) {
#ifndef ENABLE_RUNEMASTER /* Are we using this space...? */
				case TV_EGG:
#else
				case TV_RUNE1:
				case TV_RUNE2: 
#endif
					break;
				default:
					return (FALSE);
			}
			break;

		/* Rare Footwear Shop */
		case STORE_SHOESX:
			/* Analyze the type */
			switch (o_ptr->tval) {
				case TV_BOOTS:
				break;
				default:
				return (FALSE);
			}
			break;

		/* Rare Jewellry Shop */
		case STORE_JEWELX:
			/* Analyze the type */
			switch (o_ptr->tval) {
				case TV_AMULET:
				case TV_RING:
				break;
				default:
				return (FALSE);
			}
			break;

		/* Mining Supply Store */
		case STORE_MINING:
			switch (o_ptr->tval) {
				case TV_DIGGING:
				case TV_LITE:
				case TV_FLASK: break;
				case TV_WAND: if (o_ptr->sval != SV_WAND_STONE_TO_MUD) return(FALSE); else break;
				case TV_POTION: if (o_ptr->sval != SV_POTION_DETONATIONS) return(FALSE); else break;
				default:
				return (FALSE);
			}
			break;
		/* Bordertravel supplies */
		case STORE_BTSUPPLY:
			/* Analyze the type */
			switch (o_ptr->tval) {
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

		case STORE_HERBALIST:
			switch (o_ptr->tval) {
				case TV_BOOK: if (get_book_name_color(Ind, o_ptr) != TERM_L_GREEN) return FALSE;
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

		case STORE_SPEC_POTION:
			/* Analyze the type */
			switch (o_ptr->tval) {
				case TV_POTION:
				case TV_POTION2:
				break;
				default:
				return (FALSE);
			}
			break;
		case STORE_SPEC_SCROLL:
			/* Analyze the type */
			switch (o_ptr->tval) {
				case TV_SCROLL:
				break;
				default:
				return (FALSE);
			}
			break;
		case STORE_SPEC_CLOSECOMBAT:
			/* Analyze the type */
			switch (o_ptr->tval) {
				case TV_BOOTS:
				case TV_GLOVES:
				case TV_CROWN:
				case TV_HELM:
				case TV_SHIELD:
				case TV_CLOAK:
				case TV_SOFT_ARMOR:
				case TV_HARD_ARMOR:
				case TV_DRAG_ARMOR:
				case TV_SHOT:
				case TV_BOLT:
				case TV_ARROW:
				case TV_BOW:
				case TV_BLUNT:
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

		case STORE_SPEC_ARCHER:
			/* Analyze the type */
			switch (o_ptr->tval) {
				case TV_SHOT:
				case TV_BOLT:
				case TV_ARROW:
				case TV_BOW:
				break;
				default:
				return (FALSE);
			}
			break;

		case STORE_HIDDENLIBRARY:
			/* Analyze the type */
			switch (o_ptr->tval) {
				case TV_BOOK:
				break;
				default:
				return (FALSE);
			}
			break;

		case STORE_STRADER: /* For ironman dungeons */
			/* doesn't like very cheap items */
			if (object_value(Ind, o_ptr) < 10) return (FALSE);
			break;
	}

	/* XXX XXX XXX Ignore "worthless" items */
	if (object_value(Ind, o_ptr) <= 0) return (FALSE);

	/* XXX Never OK to sell keys */
	if (o_ptr->tval == TV_KEY) return (FALSE);

	/* This prevents suicide-cheeze */
#if 0 /* changed to allow selling own level 0 rewards that weren't discounted */
 #if STARTEQ_TREATMENT == 3
	if (o_ptr->level < 1) return (FALSE);
 #endif
 #if STARTEQ_TREATMENT == 2
	if ((o_ptr->level < 1) && (o_ptr->owner != p_ptr->id)) return (FALSE);
 #endif
#else
 #if STARTEQ_TREATMENT > 1
	if ((o_ptr->level < 1) && (o_ptr->owner != p_ptr->id)) return (FALSE);
 #endif
#endif
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
	s64b	value, j_value;
	object_type	*j_ptr;

	/* Evaluate the object */
	value = object_value(0, o_ptr);

	/* Cursed/Worthless items "disappear" when sold */
#ifdef PLAYER_STORES
	if (!st_ptr->player_owner) /* allow 100% off items in player stores */
#endif
	if (value <= 0 &&
	    !(st_info[st_ptr->st_idx].flags1 & SF1_MUSEUM))
		return (-1);

	/* All store items are fully *identified* */
	/* (I don't know it's too nice.. ) */
#ifdef PLAYER_STORES
//	if (!(o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_CHEQUE))
	if (!st_ptr->player_owner)
#endif
	o_ptr->ident |= ID_MENTAL;

	/* Erase the inscription */
#ifdef PLAYER_STORES
	if (!st_ptr->player_owner) /* don't erase inscriptions in player stores */
#endif
	if (!(st_info[st_ptr->st_idx].flags1 & SF1_MUSEUM)) {
		o_ptr->note = 0;
		o_ptr->note_utag = 0;
	}

#ifdef PLAYER_STORES
	/* don't combine items in player stores!
	   Otherwise, referring to the original item will fail for mang-houses. */
	if (!st_ptr->player_owner)
#endif
	/* Check each existing item (try to combine) */
	for (slot = 0; slot < st_ptr->stock_num; slot++) {
		/* Get the existing item */
		j_ptr = &st_ptr->stock[slot];

		/* Can the existing items be incremented? */
		if (store_object_similar(j_ptr, o_ptr)) {
			/* Hack: Can't have more than 1 of certain items at a time!
			   Added for Artifact Creation - C. Blue */
			if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_ARTIFACT_CREATION)
				return (-1);

			/* Hack -- extra items disappear */
			store_object_absorb(j_ptr, o_ptr);

			/* All done */
			return (slot);
		}
	}

	/* No space? */
	if (st_ptr->stock_num >= st_ptr->stock_size) return (-1);


    if (!(st_info[st_ptr->st_idx].flags1 & SF1_MUSEUM)) {

	/* Check existing slots to see if we must "slide" */
	for (slot = 0; slot < st_ptr->stock_num; slot++) {
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
		st_ptr->stock[i] = st_ptr->stock[i-1];

    } else { /* is museum -> don't order items! */

	slot = st_ptr->stock_num;

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
 * 
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
	int j;
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
		st_ptr->stock[j] = st_ptr->stock[j + 1];

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

#if 1 /* not that big impact on the game, so, optional */
	/* No golem items? */
	if (o_ptr->tval == TV_GOLEM) return (TRUE);
#endif

	/* No Talismans in the BM (can only be found! >:) */
	if (o_ptr->tval == TV_AMULET && o_ptr->sval == SV_AMULET_LUCK) return (TRUE);

	/* No magic ammos either =) the_sandman */
	if (is_ammo(o_ptr->tval) && o_ptr->sval == SV_AMMO_MAGIC) return (TRUE);

	/* No runes at all, actually... */
	if ((o_ptr->tval == TV_RUNE1 || o_ptr->tval == TV_RUNE2))
		return (TRUE);

	/* No "Handbook"s in the BM (can only be found) - C. Blue */
	if (o_ptr->tval == TV_BOOK && o_ptr->sval >= SV_BOOK_COMBO && o_ptr->sval < SV_CUSTOM_TOME_1) return (TRUE);
	
	/* no ethereal ammo */
	if (o_ptr->name2 == EGO_ETHEREAL || o_ptr->name2b == EGO_ETHEREAL) return(TRUE);

	/* No items that can be used by WINNERS_ONLY in the BM - C. Blue */
	if (k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) return (TRUE);

	/* Ego items are never crap */
	if (o_ptr->name2) return (FALSE);

	/* Good items are never crap */
	if (o_ptr->to_a > 0) return (FALSE);
	if (o_ptr->to_h > 0) return (FALSE);
	if (o_ptr->to_d > 0) return (FALSE);

	/* Don't allow items in black markets that are listed in other BMs' templates already */
	/* check individual store would be better. *later* */
	for (i = 0; i < max_st_idx; i++) {
		if (st_info[i].flags1 & SF1_ALL_ITEM) {
			for (j = 0; j < st_info[i].table_num; j++) {
				if (st_info[i].table[j][0] >= 10000) {
					if (o_ptr->tval == st_info[i].table[j][0] - 10000)
						return(FALSE);
				} else {
					if (o_ptr->k_idx == st_info[i].table[j][0])
						return(FALSE);
				}
			}
		}
	}

	/* Don't allow items that are currently offered in any non-BM.
	   NOTE: Should maybe be changed to just check vs items in that store's template!
	         Otherwise selling an item could block same item type being generated in a BM.. */
#if 1 /* note: ego items are never bm crap (see above), so checking all base stores at all does make sense */
	for (k = 0; k < numtowns; k++) {
		st_ptr = town[k].townstore;
 #if 0 /* according to Mikael's find, this causes bad bugs, ie items that seemingly for no reason wont show up in BMs anymore */
		/* Check the other "normal" stores */
		for (i = 0; i < max_st_idx; i++)
 #else /* using light-weight version that shouldn't pose problems */
		/* Check the other basic normal stores */
		for (i = 0; i < 6; i++)
 #endif
		{
			/* Don't compare other BMs */
			if (st_info[i].flags1 & SF1_ALL_ITEM) continue;

			/* Check every item in the store */
			for (j = 0; j < st_ptr[i].stock_num; j++) {
				object_type *j_ptr = &st_ptr[i].stock[j];

				/* Duplicate item "type", assume crappy */
				if (!j_ptr->owner && o_ptr->k_idx == j_ptr->k_idx) return (TRUE);
			}
		}
	}
#endif

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

	char o_name[ONAME_LEN];
	cptr s_name;

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
		handle_art_d(o_ptr->name1);
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

#ifndef TEST_SERVER
	/* keep track of artifact creation scrolls in log */
	if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_ARTIFACT_CREATION
	    && o_ptr->number == num) {
		object_desc(0, o_name, o_ptr, TRUE, 3);
		s_name = st_name + st_info[st_ptr->st_idx].name;
		s_printf("%s: STORE_DELETE: %d/%d - %d, %s (%s).\n", showtime(), st_ptr->town, town[st_ptr->town].type, st_ptr->st_idx, o_name, s_name);
	}
#else
	/* keep track of artifact creation scrolls in log */
	if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_ARTIFACT_CREATION
	    && o_ptr->number == num) {
		object_desc(0, o_name, o_ptr, TRUE, 3);
		s_name = st_name + st_info[st_ptr->st_idx].name;
		s_printf("%s: STORE_DELETE: %d/%d - %d, %s (%s).\n", showtime(), st_ptr->town, town[st_ptr->town].type, st_ptr->st_idx, o_name, s_name);
	}
#endif

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
	else level = rand_range(1, STORE_OBJ_LEVEL); //usually 5

//	if (sti_ptr->flags1 & SF1_DEPEND_LEVEL) level += dun_level;

	/* used for rare dungeon stores */
	if (sti_ptr->flags1 & SF1_SHALLOW_LEVEL) level += 5 + rand_int(5);
	if (sti_ptr->flags1 & SF1_MEDIUM_LEVEL) level += 25 + rand_int(25);
	if (sti_ptr->flags1 & SF1_DEEP_LEVEL) level += 45 + rand_int(45); /* if < 50 will prevent tomes in XBM, but occurs only rarely */

//	if (sti_ptr->flags1 & SF1_ALL_ITEM) level += p_ptr->lev;

	/* Better books in bookstores outside of Bree */
	if (st_ptr->st_idx == STORE_BOOK) {
		level += town[st_ptr->town].baselevel;
	}

	return (level);
}

/* Is it an ok object ? */ /* I don't like this kinda hack.. */
static int store_tval = 0, store_level = 0;

/*
 * Hack -- determine if a template is "good"
 */
static int kind_is_storeok(int k_idx, u32b resf)
{
	object_kind *k_ptr = &k_info[k_idx];

	int p;

#if 0
	if (k_info[k_idx].flags3 & TR3_NORM_ART)
		return 0;
#endif	// 0

	if (k_info[k_idx].flags3 & TR3_INSTA_ART)
		return 0;

	p = kind_is_legal(k_idx, resf);

	if (k_ptr->tval != store_tval) p = 0;
	if (k_ptr->level < (store_level / 2)) p = 0;

	/* Return the percentage */
	return p;
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
	int i = 0, tries, level, chance, item;
	int value, rarity; /* for ego power checks */

	object_type tmp_obj;
	object_type *o_ptr = &tmp_obj;
	int force_num = 0;
        object_kind *k_ptr;
	ego_item_type *e_ptr, *e2_ptr;
	bool good, great;
	u32b resf = RESF_STORE;
	obj_theme theme;
	bool black_market = (st_info[st_ptr->st_idx].flags1 & SF1_ALL_ITEM);
	bool carry_ok;

#if 0 /* see below where they're used */
	int boots_witan_idx = lookup_kind(TV_BOOTS, SV_PAIR_OF_WITAN_BOOTS);
	int boots_soft_idx = lookup_kind(TV_BOOTS, SV_PAIR_OF_SOFT_LEATHER_BOOTS);
	int boots_hard_idx = lookup_kind(TV_BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS);
#endif

	/* Paranoia -- no room left */
	if (st_ptr->stock_num >= st_ptr->stock_size) return;

	if (black_market) resf = RESF_STOREBM;

	/* Hack -- consider up to n items */
	for (tries = 0; tries < (black_market ? 60 : 4); tries++) /* 20:4, 40:4, 60:4, 100:4 !
	    for some reason using the higher number instead of 4 for normal stores will result in many times more ego items! ew */
	{
		/* Black Market */

		if (black_market) {
			/* Hack -- Pick an item to sell */
			item = rand_int(st_info[st_ptr->st_idx].table_num);
			i = st_info[st_ptr->st_idx].table[item][0];
			chance = st_info[st_ptr->st_idx].table[item][1];

			/* Does it pass the rarity check ? */
#if 0
			/* Balancing check for other items!!! */
			if (!magik(chance) || magik(60)) i = 0;
#else
			if (!magik(chance)) i = 0;
#endif
			/* Hack -- mass-produce for black-market promised items */
			else if (black_market) force_num = rand_range(2, 5);//was 3,9

			/* Hack -- fake level for apply_magic() */
			level = return_level(st_ptr); /* note: it's margin is random! */

			/* Hack -- i > 10000 means it's a tval and all svals are allowed */
			if (i > 10000) {
				store_tval = i - 10000;

				/* Do we forbid too shallow items ? */
				if (st_info[st_ptr->st_idx].flags1 & SF1_FORCE_LEVEL) store_level = level;
				else store_level = 0;
				/* Hack -- for later, in case of getting only a tval, but no sval */
				/* No themes */
				theme.treasure = 100;
				theme.combat = 100;
				theme.magic = 100;
				theme.tools = 100;
				init_match_theme(theme);
				/* Activate restriction */
				get_obj_num_hook = kind_is_storeok;

				/* Prepare allocation table */
				get_obj_num_prep(resf);

				/* Get it ! */
				i = get_obj_num(level, resf);
			}

			if (!i) {
				/* Pick a level for object/magic */
				level = 60 + rand_int(25);	/* for now let's use this */

				/* Clear restriction */
				get_obj_num_hook = NULL;
	
				/* Prepare allocation table */
				get_obj_num_prep(resf);
	
				/* Random item (usually of given level) */
				i = get_obj_num(level, resf);
    				
				/* Handle failure */
				if (!i) continue;
			}
		}

		/* Normal Store */
		else {
			/* Hack -- Pick an item to sell */
			item = rand_int(st_info[st_ptr->st_idx].table_num);
			i = st_info[st_ptr->st_idx].table[item][0];
			chance = st_info[st_ptr->st_idx].table[item][1];

			/* Does it pass the rarity check ? */
			if (!magik(chance)) continue;

			/* Hack -- fake level for apply_magic() */
			level = return_level(st_ptr); /* note: it's margin is random!
			    note: if this is moved into the i > 10000 part, book stores will mass-produce pval=0 books (manathrust) */

			/* Hack -- i > 10000 means it's a tval and all svals are allowed */
			if (i > 10000) {
				store_tval = i - 10000;

				/* Do we forbid too shallow items ? */
				if (st_info[st_ptr->st_idx].flags1 & SF1_FORCE_LEVEL) store_level = level;
				else store_level = 0;
				/* Hack -- for later, in case of getting only a tval, but no sval */
				/* No themes */
				theme.treasure = 100;
				theme.combat = 100;
				theme.magic = 100;
				theme.tools = 100;
				init_match_theme(theme);
				/* Activate restriction */
				get_obj_num_hook = kind_is_storeok;

				/* Prepare allocation table */
				get_obj_num_prep(resf);

				/* Get it ! */
				i = get_obj_num(level, resf);

				/* Invalidate the cached allocation table */
//				alloc_kind_table_valid = FALSE;

#if 0 /* done in st_info.txt instead by specifying boots probabilities directly */
				/* Mad hack for Rare Footwear Store (STORE_SHOESX): Less Witans */
				if ((st_info[st_ptr->st_idx].flags1 & SF1_RARE_EGO) &&
				    (st_info[st_ptr->st_idx].flags1 & SF1_DEEP_LEVEL) &&
				    i == boots_witan_idx && rand_int(2)) {
					if (rand_int(2)) i = boots_soft_idx;
					else i = boots_hard_idx;
				}
#endif
			}

			if (!i) continue;
		}

		/* Create the selected base object */
		invcopy(o_ptr, i);

		/* Apply some "low-level" magic (no artifacts) */
		/* ego flags check */
		if (st_info[st_ptr->st_idx].flags1 & SF1_GOOD) good = TRUE;
		else good = FALSE;
		if (st_info[st_ptr->st_idx].flags1 & SF1_GREAT) great = TRUE;
		else great = FALSE;
		apply_magic(&o_ptr->wpos, o_ptr, level, FALSE, good, great, FALSE, resf);

#ifdef PREVENT_CURSED_TOOLS
		/* prevent 'cursed' diggers/tools which aren't really cursed (see a_m_aux_1) - C. Blue */
		if ((o_ptr->tval == TV_DIGGING && o_ptr->tval == TV_TOOL) &&
		    (o_ptr->to_h + o_ptr->to_d < 0))
			continue;
#endif

		k_ptr = &k_info[o_ptr->k_idx];

#ifdef NO_PSEUDO_CURSED_ITEMS
		/* Prevent items with boni that are below their k_info boni */
		if (!o_ptr->name2 && (
		    o_ptr->to_h < k_ptr->to_h ||
		    o_ptr->to_d < k_ptr->to_d ||
		    o_ptr->to_a < k_ptr->to_a))
			continue;
#endif

		/* Hack -- Charge lite uniformly */
		if (o_ptr->tval == TV_LITE) {
			u32b f1, f2, f3, f4, f5, esp;
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

			/* Only fuelable ones! */
			if (f4 & TR4_FUEL_LITE) {
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
		if (black_market) {
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
		else {
			/* No "worthless" items */
			if (object_value(0, o_ptr) <= 0) continue;

			/* ego rarity control for normal stores */
			if (o_ptr->name2) {
				if ((!(st_info[st_ptr->st_idx].flags1 & SF1_EGO)) &&
				    !magik(STORE_EGO_CHANCE) && st_ptr->st_idx != STORE_GENERAL) /* more ego lamps in general store please */
					continue;
			}

			/* Hack -- General store shouldn't sell too much aman cloaks etc */
			/* Softened it up to allow brightness lamps! (for RPG mainly) - C. Blue */
			if (st_ptr->st_idx == STORE_GENERAL && object_value(0, o_ptr) > 18000 && /* ~13000 probably max */
			    magik(33) &&
			    !(st_info[st_ptr->st_idx].flags1 & SF1_EGO))
				continue;
		}

		/* Let's not allow 'Cure * Insanity', 'Augmentation', 'Learning', 'Experience',
		 * and 'Invulnerability' potions - the_sandman */
		if (st_ptr->st_idx == STORE_SPEC_POTION) {
			switch (o_ptr->tval) {
				case TV_POTION:
					switch (o_ptr->sval) {
						case SV_POTION_AUGMENTATION:
						case SV_POTION_EXPERIENCE:
						case SV_POTION_INVULNERABILITY:
							continue;
							break;
						default:
							break;
					} break;
				case TV_POTION2:
					switch (o_ptr->sval) {
						case SV_POTION2_CURE_LIGHT_SANITY:
						case SV_POTION2_CURE_SERIOUS_SANITY:
						case SV_POTION2_CURE_CRITICAL_SANITY:
						case SV_POTION2_CURE_SANITY:
						case SV_POTION2_LEARNING:
							continue;
							break;
						default:
							break;
					} break;
				default: break;	//shouldn't happen anyway
			}//o_ptr->tval switch
		}

		e_ptr = &e_info[o_ptr->name2];
		e2_ptr = &e_info[o_ptr->name2b];

		/* Shop has many egos? */
		if ((st_info[st_ptr->st_idx].flags1 & SF1_EGO) &&
		    !(o_ptr->name2) && magik(50))
			continue;

		/* Shop has extra rare egos? */
		if ((st_info[st_ptr->st_idx].flags1 & SF1_RARE_EGO) && o_ptr->name2) {
			value = e_ptr->cost + flag_cost(o_ptr, o_ptr->pval);
			rarity = e_ptr->mrarity / e_ptr->rarity;
			if (o_ptr->name2b) {
				value += e2_ptr->cost;
				/* Take rarer of both powers */
				if (e2_ptr->mrarity / e2_ptr->rarity > rarity)
					rarity = e2_ptr->mrarity / e2_ptr->rarity;
			}
			if ((rarity < 7 || value < 9000) && magik(75)) continue;
		}

		/* Is the item too cheap for this shop? */
		if ((st_info[st_ptr->st_idx].flags1 & SF1_PRICY_ITEMS1) && (object_value(0, o_ptr) < 3000))
			continue;
		if ((st_info[st_ptr->st_idx].flags1 & SF1_PRICY_ITEMS2) && (object_value(0, o_ptr) < 8000))//PRICY_ITEMS1
			continue;
		if ((st_info[st_ptr->st_idx].flags1 & SF1_PRICY_ITEMS3) && (object_value(0, o_ptr) < 15000))//PRICY_ITEMS2
			continue;
		if ((st_info[st_ptr->st_idx].flags1 & SF1_PRICY_ITEMS4) && (object_value(0, o_ptr) < 25000))//20000
			continue;

		/* Further store flag checks */
		/* Rare enough? */
		/* Interesting numbers: 3+, 6+, 8+, 16+ */
		if ((st_info[st_ptr->st_idx].flags1 & SF1_VERY_RARE)) {
			/* Note that items generated in the non-magik(99)-case will still profit from the shop flags. */
			if (magik(99)) { /* just for some diversification, no real reason though unlike for SF1_RARE flag */
				if ((k_ptr->chance[0] && k_ptr->chance[0] < 8) ||
				    (k_ptr->chance[1] && k_ptr->chance[1] < 8) ||
				    (k_ptr->chance[2] && k_ptr->chance[2] < 8) ||
				    (k_ptr->chance[3] && k_ptr->chance[3] < 8))
					continue;

				if (black_market) {
					/* players need to actually find their items! Allowing weapons though. */
					if (is_rare_armour(k_ptr->tval, k_ptr->sval)) continue;

					/* prevent books in SBM too? */
					//else if (k_ptr->tval == TV_BOOK) continue;

					/* make uber rods harder to get */
					else if (k_ptr->tval == TV_ROD) continue;

					/* no easy golem parts */
					else if (k_ptr->tval == TV_GOLEM) continue;

					/* no amulet spam, we have a secret jewelry for that */
					else if (k_ptr->tval == TV_AMULET && magik(90)) continue;
				}
			}
		}
		if ((st_info[st_ptr->st_idx].flags1 & SF1_RARE)) {
			/* no need for stat potions in a rare store, just wasting space.
			   They occur fine in normal BMs or dedicated potion store (RPG). */
			if (k_ptr->tval == TV_POTION &&
			    (k_ptr->sval == SV_POTION_INC_STR ||
			    k_ptr->sval == SV_POTION_INC_INT ||
			    k_ptr->sval == SV_POTION_INC_WIS ||
			    k_ptr->sval == SV_POTION_INC_DEX ||
			    k_ptr->sval == SV_POTION_INC_CON ||
			    k_ptr->sval == SV_POTION_INC_CHR)
			    && magik(99))
				continue;

			/* experimental: allow the SF1_RARE flag to 'not kick in' periodically! - C. Blue
			   explanation: top-class ego armour was easily obtainable here..too easily!
					the whole point of actually finding l00t was partially nullified by this,
					however, completely preventing top armour to show up in a black market
					cannot be the solution. so this store now sometimes acts as "normal bm",
					allowing any kind of top armour, while in most cases tries hard to
					_additionally_ generate the RARE stuff that we expect/want from it.
					note that these 'normal bm' fits still profit from all other shop flags,
					that normal bms don't have, so cheap/low items won't appear. :D
			   (note: it should (for mages) probably be kept ensured that soft leather boots can appear in boot store too.) */
			/* Note that items generated in the non-magik(95)-case will still profit from the shop flags. */
			if (magik(95)) {
				if ((k_ptr->chance[0] && k_ptr->chance[0] < 3) ||
				    (k_ptr->chance[1] && k_ptr->chance[1] < 3) ||
				    (k_ptr->chance[2] && k_ptr->chance[2] < 3) ||
				    (k_ptr->chance[3] && k_ptr->chance[3] < 3))
					continue;

				if (black_market) {
					/* Important: Don't allow easy armour gathering. */
					if (is_armour(k_ptr->tval)) continue;

					/* otherwise the rare weapons occur too often with nice ego powers for too low price */
					else if (is_weapon(k_ptr->tval)) continue;

					/* don't make rare jewelry store unemployed */
					else if (k_ptr->tval == TV_AMULET || k_ptr->tval == TV_RING) continue;

					/* make uber rods harder to get */
					else if (k_ptr->tval == TV_ROD) continue;

					/* keep custom books out of XBM, so they may appear more often in BM/SBM */
					else if (k_ptr->tval == TV_BOOK &&
					    k_ptr->sval >= SV_CUSTOM_TOME_1 && k_ptr->sval <= SV_CUSTOM_TOME_3)
						continue;

					/* XBM must not make Khazad Mining Supply Store unemployed! */
					else if (o_ptr->tval == TV_LITE) {
						if (magik(90)) continue;
					} else if (o_ptr->tval == TV_DIGGING) {
						if (magik(75)) continue;
					}
				}
			}
		}

#if 0
/* MEDIUM_LEVEL and DEEP_LEVEL are used in return_level() */
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

		if (st_info[st_ptr->st_idx].flags1 & SF1_NO_DISCOUNT3) {
		    /* Reduce discount */
		    if (o_ptr->discount == 10) o_ptr->discount = 10;
		    if (o_ptr->discount == 25) o_ptr->discount = 20;
		    if (o_ptr->discount == 50) o_ptr->discount = 30;
		    if (o_ptr->discount == 75) o_ptr->discount = 40;
		    if (o_ptr->discount == 90) o_ptr->discount = 50;
		    if (o_ptr->discount == 100) o_ptr->discount = 50;
		}
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

		if ((force_num) && !is_ammo(o_ptr->tval)) {
			/* Only single items of these */
			switch (o_ptr->tval) {
			case TV_DRAG_ARMOR:
				force_num = 1;
				break;
			case TV_SOFT_ARMOR:
				if (o_ptr->sval == SV_COSTUME) force_num = 1;
				break;
			case TV_RING:
				if (o_ptr->sval == SV_RING_POLYMORPH) force_num = 1;
				break;
			case TV_TOOL:
				if (o_ptr->sval == SV_TOOL_WRAPPING) force_num = 1;
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

		if (st_info[st_ptr->st_idx].flags1 & SF1_ZEROLEVEL) o_ptr->level = 0;

		/* Attempt to carry the (known) item */
		carry_ok = (store_carry(st_ptr, o_ptr) != -1);

#if 0 /* debug herbalist store */
		if (st_ptr->st_idx == STORE_HERBALIST) {
			char o_name[ONAME_LEN];
			cptr s_name;
 #if 0
			object_desc(0, o_name, o_ptr, TRUE, 3);
			s_name = st_name + st_info[st_ptr->st_idx].name;
			s_printf("%s: STORE_CARRY: %d/%d - %d, %s (%s)", showtime(), st_ptr->town, town[st_ptr->town].type, st_ptr->st_idx, o_name, s_name);
			if (carry_ok) s_printf(" OK.\n");
			else s_printf(" FAILED.\n");
 #else
			if (!carry_ok) {
				object_desc(0, o_name, o_ptr, TRUE, 3);
				s_name = st_name + st_info[st_ptr->st_idx].name;
				s_printf("%s: STORE_CARRY_FAILED: %d/%d - %d, %s (%s)\n", showtime(), st_ptr->town, town[st_ptr->town].type, st_ptr->st_idx, o_name, s_name);
			}
 #endif
		}
#endif

#if 0
		/* Log occurances of special items */
 #ifndef TEST_SERVER
//		if ((o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_ARTIFACT_CREATION && st_ptr->st_idx != 60) || /* avoid spam from SBM which offers lots of these */
		if ((o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_ARTIFACT_CREATION) ||
		    (o_ptr->tval == TV_LITE && (o_ptr->name2 == EGO_LITE_MAGI || o_ptr->name2b == EGO_LITE_MAGI)) ||
  #if 0
		    (o_ptr->tval == TV_BOOK && st_ptr->st_idx == 48) ||
  #endif
		    (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_POWER && st_ptr->st_idx != 60)) {
 #else
//		if ((o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_ARTIFACT_CREATION && st_ptr->st_idx != 60) || /* avoid spam from SBM which offers lots of these */
		if ((o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_ARTIFACT_CREATION) ||
		    (o_ptr->tval == TV_LITE && (o_ptr->name2 == EGO_LITE_MAGI || o_ptr->name2b == EGO_LITE_MAGI)) ||
		    (o_ptr->tval == TV_LITE && (o_ptr->name2 == EGO_LITE_MAGI || o_ptr->name2b == EGO_LITE_MAGI))) {
 #endif
			char o_name[ONAME_LEN];
			cptr s_name;
			object_desc(0, o_name, o_ptr, TRUE, 3);
			s_name = st_name + st_info[st_ptr->st_idx].name;
			s_printf("%s: STORE_CARRY: %d/%d - %d, %s (%s)", showtime(), st_ptr->town, town[st_ptr->town].type, st_ptr->st_idx, o_name, s_name);
			if (carry_ok) s_printf(" OK.\n");
			else s_printf(" FAILED.\n");
		}
#endif

		/* Definitely done */
		break;
	}
}

/*
 * Update the bargain info
 */
#if 0
static void updatebargain(s64b price, s64b minprice)
{
	/* Hack -- auto-haggle */
	if (auto_haggle) return;

	/* Cheap items are "boring" */
	if (minprice < 10L) return;

	/* Count the successful haggles */
	if (price == minprice) {
		/* Just count the good haggles */
		if (st_ptr->good_buy < MAX_SHORT)
			st_ptr->good_buy++;
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
static int gettown(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i, retval =- 1;
	if (p_ptr->wpos.wz) return(-1);
	for (i = 0; i < numtowns; i++){
		if (town[i].x == p_ptr->wpos.wx && town[i].y == p_ptr->wpos.wy) {
			retval = i;
			break;
		}
	}
	return(retval);
}
/* For abusing towns to provide dungeon stores - C. Blue
   (Use dlev_id and fake_town_num to bind a town's dungeon stores to a dungeon floor.) */
static int gettown_dun(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i;
	dun_level *l_ptr;

	/* formerly 0 was the default town for all non-regular-town stores (ie wild/dungeon stores) */
	//return 0;

	/* Use first normal town (Bree) for rogue stores in the wilderness
	   (don't exist though, maybe in the future) */
	if (p_ptr->wpos.wz == 0) return 0;

	/* For dungeon/tower floor stores, cycle through towns to allow
	   multiple stores of same type to occur which aren't linked */
	l_ptr = getfloor(&p_ptr->wpos);

	/* Fake town has already been set for this floor? */
	if (l_ptr->fake_town_num) return(l_ptr->fake_town_num - 1);

	/* Set a fake town to be used for this floor */
	for (i = 0; i < numtowns; i++) {
		if (town[i].dlev_id == 0) {
			/* link town and floor to each other for easy reference */
			town[i].dlev_id = l_ptr->id;
			l_ptr->fake_town_num = i + 1;

			return(i);
		}
	}

	/* All towns in use, then just re-use a random town,
	   but make it fixed for this dungeon floor! */

	/* Fix the RNG */
	bool rand_old = Rand_quick;
	u32b old_seed = Rand_value;
	Rand_quick = TRUE;
	Rand_value = l_ptr->id;

	i = rand_int(numtowns);

	/* Restore the RNG */
	Rand_quick = rand_old;
	Rand_value = old_seed;

	return(i);
}

/* Display a non-shop store (newly added SPECIAL flag) - C. Blue
   (Note: Must not be a player store - illegal) */
static void display_special_store(Ind) {
	player_type *p_ptr = Players[Ind];
	store_type *st_ptr;
	owner_type *ot_ptr;
	cptr store_name;
	int i;

	/* Get store and owner information */
	i = gettown(Ind);
	/* hack: non-town stores (ie dungeon, but could also be wild) are borrowed from town #0 - C. Blue */
	if (i == -1) i = gettown_dun(Ind);

	st_ptr = &town[i].townstore[p_ptr->store_num];
	ot_ptr = &ow_info[st_ptr->owner];
	store_name = (st_name + st_info[st_ptr->st_idx].name);

	/* Display contents, if any.
	   Note: Coordinates should start at +5, +5 to look good (pseudo-border). */
	switch (st_ptr->st_idx) {
	case STORE_CASINO:
		Send_store_special_str(Ind, 5, 5, TERM_YELLOW, "Welcome to the casino, where the lucky makes a fortune!");
		break;
	}
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

	object_type	*o_ptr;
	s64b		x;

	char		o_name[ONAME_LEN];
	byte		attr;
	int		wgt;
	int		i;

	int maxwid = 75;

	i = gettown(Ind);
	/* hack: non-town stores (ie dungeon, but could also be wild) are borrowed from town #0 - C. Blue */
	if (i == -1) i = gettown_dun(Ind);

#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2) /* it's a player's private store! */
		st_ptr = &fake_store[-2 - p_ptr->store_num];
	else
#endif
	st_ptr = &town[i].townstore[p_ptr->store_num];
//	ot_ptr = &owners[p_ptr->store_num][st_ptr->owner];
	ot_ptr = &ow_info[st_ptr->owner];

	/* Get the item */
	o_ptr = &st_ptr->stock[pos];

#if 0
	/* Get the "offset" */
	i = (pos % 12);

	/* Label it, clear the line --(-- */
	(void)snprintf(out_val, sizeof(out_val), "%c) ", I2A(i));
	prt(out_val, i+6, 0);
#endif

	/* Describe an item in the home */
	if (p_ptr->store_num == STORE_HOME) {
		maxwid = 75;

		/* Leave room for weights, if necessary -DRS- */
		/*if (show_weights) maxwid -= 10;*/

		/* Describe the object */
		/* hack for wand charges to not get displayed accumulated (less comfortable) */
		if (o_ptr->tval == TV_WAND) {
			i = o_ptr->pval;
			o_ptr->pval = i / o_ptr->number;
		}
		object_desc(Ind, o_name, o_ptr, TRUE, 3);
		if (o_ptr->tval == TV_WAND) o_ptr->pval = i; /* hack clean-up */
		o_name[maxwid] = '\0';

		attr = get_attr_from_tval(o_ptr);

		if (o_ptr->tval == TV_BOOK) attr = get_book_name_color(Ind, o_ptr);

		/* grey out if level requirements don't meet */
		if (((!o_ptr->level) || (o_ptr->level > p_ptr->lev)) &&
		    (o_ptr->owner) && (o_ptr->owner != p_ptr->id))
			attr = TERM_L_DARK;

		/* grey out if mode doesn't meet */
		if (compat_pomode(Ind, o_ptr)) attr = TERM_L_DARK;

		/* Only show the weight of an individual item */
		wgt = o_ptr->weight;

		/* Send the info */
		if (is_newer_than(&p_ptr->version, 4, 4, 3, 0, 0, 4)) {
                        if (o_ptr->tval != TV_BOOK || o_ptr->sval < SV_CUSTOM_TOME_1 || o_ptr->sval == SV_SPELLBOOK) {
                                Send_store(Ind, pos, attr, wgt, o_ptr->number, 0, o_name, o_ptr->tval, o_ptr->sval, o_ptr->pval);
			} else {
                                Send_store_wide(Ind, pos, attr, wgt, o_ptr->number, 0, o_name, o_ptr->tval, o_ptr->sval, o_ptr->pval,
                                    o_ptr->xtra1, o_ptr->xtra2, o_ptr->xtra3, o_ptr->xtra4, o_ptr->xtra5, o_ptr->xtra6, o_ptr->xtra7, o_ptr->xtra8, o_ptr->xtra9);
                        }
                } else {
			Send_store(Ind, pos, attr, wgt, o_ptr->number, 0, o_name, o_ptr->tval, o_ptr->sval, o_ptr->pval);
		}
	}
	/* Describe an item (fully) in a store */
	else {
		/* Must leave room for the "price" */
		maxwid = 65;

		/* Leave room for weights, if necessary -DRS- */
		/*if (show_weights) maxwid -= 7;*/

		/* Describe the object (fully) */
		/* hack for wand charges to not get displayed accumulated (less comfortable) */
		if (o_ptr->tval == TV_WAND) {
			i = o_ptr->pval;
			o_ptr->pval = i / o_ptr->number;
		}
#ifdef PLAYER_STORES
		/* Don't display items as fake *ID*ed in player stores! */
		if (p_ptr->store_num <= -2)
			object_desc(Ind, o_name, o_ptr, TRUE, 3);
		else
#endif
		object_desc_store(Ind, o_name, o_ptr, TRUE, 3);
		if (o_ptr->tval == TV_WAND) o_ptr->pval = i; /* hack clean-up */
		o_name[maxwid] = '\0';

		attr = get_attr_from_tval(o_ptr);

		if (o_ptr->tval == TV_BOOK) attr = get_book_name_color(Ind, o_ptr);

		/* grey out if level requirements don't meet */
		if (((!o_ptr->level) || (o_ptr->level > p_ptr->lev)) &&
		    (o_ptr->owner) && (o_ptr->owner != p_ptr->id))
			attr = TERM_L_DARK;

		/* grey out if mode doesn't meet */
		if (compat_pomode(Ind, o_ptr)) attr = TERM_L_DARK;

		/* Only show the weight of an individual item */
		wgt = o_ptr->weight;

		/* Extract the "minimum" price */
#ifdef PLAYER_STORES
		if (p_ptr->store_num <= -2)
			x = price_item_player_store(Ind, o_ptr);
		else
#endif
		x = price_item(Ind, o_ptr, ot_ptr->min_inflate, FALSE);

#ifdef PLAYER_STORES
		/* HACK: Cut out the @S pricing information from the inscription.
		   Note: o_ptr->note can only occur if we're in a player-store here.
		   Pricing tag format:  @Snnnnnnnnn.  <- with dot for termination. */
		if (o_ptr->note) player_stores_cut_inscription(o_name);
#endif
		/* Send the info */
		if (is_newer_than(&p_ptr->version, 4, 4, 3, 0, 0, 4)) {
			if (o_ptr->tval != TV_BOOK || o_ptr->sval < SV_CUSTOM_TOME_1 || o_ptr->sval == SV_SPELLBOOK) {
				Send_store(Ind, pos, attr, wgt, o_ptr->number, x, o_name, o_ptr->tval, o_ptr->sval, o_ptr->pval);
			} else {
				Send_store_wide(Ind, pos, attr, wgt, o_ptr->number, x, o_name, o_ptr->tval, o_ptr->sval, o_ptr->pval,
				    o_ptr->xtra1, o_ptr->xtra2, o_ptr->xtra3, o_ptr->xtra4, o_ptr->xtra5, o_ptr->xtra6, o_ptr->xtra7, o_ptr->xtra8, o_ptr->xtra9);
			}
		} else {
			Send_store(Ind, pos, attr, wgt, o_ptr->number, x, o_name, o_ptr->tval, o_ptr->sval, o_ptr->pval);
		}
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
	
	i = gettown(Ind);
	/* hack: non-town stores (ie dungeon, but could also be wild) are borrowed from town #0 - C. Blue */
	if (i == -1) i = gettown_dun(Ind);

#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2) /* it's a player's private store! */
		st_ptr = &fake_store[-2 - p_ptr->store_num];
	else
#endif
	st_ptr = &town[i].townstore[p_ptr->store_num];

	/* Display the next 48 items */
	for (k = 0; k < STORE_INVEN_MAX; k++) {
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
	int i, j;

	i = gettown(Ind);
	/* hack: non-town stores (ie dungeon, but could also be wild) are borrowed from town #0 - C. Blue */
	if (i == -1) i = gettown_dun(Ind);

#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2) /* it's a player's private store! */
		st_ptr = &fake_store[-2 - p_ptr->store_num];
	else
#endif
	st_ptr = &town[i].townstore[p_ptr->store_num];
	ot_ptr = &ow_info[st_ptr->owner];
	store_name = (st_name + st_info[st_ptr->st_idx].name);

	/* Display the current gold */
	store_prt_gold(Ind);

	if (!(st_info[st_ptr->st_idx].flags1 & SF1_SPECIAL)) {
		/* Draw in the inventory */
		display_inventory(Ind);
	}

	/* The "Home" is special */
	if (p_ptr->store_num == STORE_HOME) { /* This shouldn't happen */
		/* Send the store info */
		Send_store_info(Ind, p_ptr->store_num, "Your House", "", st_ptr->stock_num, st_ptr->stock_size, TERM_L_UMBER, '+');
	}
	/* Normal stores */
	else {
#ifdef PLAYER_STORES
		char owner_name_ps[40];
#endif
		byte a = st_info[st_ptr->st_idx].d_attr;
		char c = st_info[st_ptr->st_idx].d_char;

		cptr owner_name = (ow_name + ot_ptr->name);

#ifdef PLAYER_STORES
		if (p_ptr->store_num <= -2) {
			/* Send fake building template, hard-coded index 66 in st_info. */
			show_building(Ind, &town[0].townstore[66]);
		} else
#endif
		/* Send the store actions info */
		show_building(Ind, st_ptr);

		/* Hack -- Museum doesn't have owner */
		if (st_info[st_ptr->st_idx].flags1 & SF1_MUSEUM) owner_name = "";

#ifdef PLAYER_STORES
		if (p_ptr->store_num <= -2) {
			/* Hack: Player stores display the owner's name */
			store_name = "";
			strcpy(owner_name_ps, lookup_player_name(st_ptr->player_owner));
			if (owner_name_ps[strlen(owner_name_ps) - 1] == 's')
				strcat(owner_name_ps, "' Private Store");
			else
				strcat(owner_name_ps, "'s Private Store");
			owner_name = owner_name_ps;

			/* Send modified private store info */
			Send_store_info(Ind, p_ptr->store_num, store_name, owner_name, st_ptr->stock_num, 0, TERM_SLATE, '+');
		} else
#endif
		/* Send the store info */
#if 0 /* maybe not neccesary? */
		if (is_newer_than(&p_ptr->version, 4, 4, 6, 1, 0, 0))
			Send_store_info(Ind, p_ptr->store_num, store_name, owner_name,
			    (st_info[st_ptr->st_idx].flags1 & SF1_SPECIAL) ? -1 : st_ptr->stock_num,
			    ot_ptr->max_cost, a, c);
		else
			Send_store_info(Ind, p_ptr->store_num, store_name, owner_name,
			    (st_info[st_ptr->st_idx].flags1 & SF1_SPECIAL) ? 0 : st_ptr->stock_num,
			    ot_ptr->max_cost, a, c);
#else
			Send_store_info(Ind, p_ptr->store_num, store_name, owner_name,
			    (st_info[st_ptr->st_idx].flags1 & SF1_SPECIAL) ? -1 : st_ptr->stock_num,
			    ot_ptr->max_cost, a, c);
#endif

		/* Hack - Items in [common town] shops, which are specified in st_info.txt,
		   are added to the player's aware-list when he sees them in such a shop. -C. Blue */
		if (cfg.item_awareness != 0) {
			bool noticed = FALSE, default_stock;
			bool base_store = (0 <= p_ptr->store_num) && (p_ptr->store_num <= 5);

			if (base_store || cfg.item_awareness >= 2) {
				for (i = 0; i < st_ptr->stock_num; i++) {
					/* does the store offer this item on its own? */
					default_stock = FALSE;
					for (j = 0; j < st_info[st_ptr->st_idx].table_num; j++) {
						if (st_info[st_ptr->st_idx].table[j][0] >= 10000) {
							if (k_info[st_ptr->stock[i].k_idx].tval == st_info[st_ptr->st_idx].table[j][0] - 10000) {
								default_stock = TRUE;
								break;
							} else if (st_ptr->stock[i].k_idx == st_info[st_ptr->st_idx].table[j][0]) {
								default_stock = TRUE;
								break;
							}
						}
					}
					if (!default_stock && cfg.item_awareness != 4) continue;

					if (base_store) {
						object_aware(Ind, &st_ptr->stock[i]);
						noticed = TRUE;
					} else if (cfg.item_awareness == 2) {
						for (j = 0; j < INVEN_TOTAL; j++)
							if (p_ptr->inventory[j].k_idx == st_ptr->stock[i].k_idx) {
								object_aware(Ind, &st_ptr->stock[i]);
								noticed = TRUE;
							}
					} else { /* it's >= 3 */
						object_aware(Ind, &st_ptr->stock[i]);
						noticed = TRUE;
					}
				}
			}

			/* did we 'id' anything? -> update char */
			if (noticed) {
				p_ptr->update |= PU_BONUS;
				p_ptr->redraw |= PR_PLUSSES;
				p_ptr->notice |= PN_COMBINE | PN_REORDER;
				p_ptr->window |= PW_INVEN | PW_EQUIP | PW_PLAYER;
			}
		}

		/* Needs to be done after Send_store_info() so the client
		   is already in shopping = TRUE mode. - C. Blue */
		if (st_info[st_ptr->st_idx].flags1 & SF1_SPECIAL)
			display_special_store(Ind);
	}
}


/*
 * Haggling routine					-RAK-
 *
 * Return TRUE if purchase is NOT successful
 */
static bool sell_haggle(int Ind, object_type *o_ptr, s64b *price)
{
	player_type *p_ptr = Players[Ind];
	store_type *st_ptr;
	owner_type *ot_ptr;
	s64b purse, cur_ask, final_ask;
	int final = FALSE;
	cptr pmt = "Offer";
	int i;

	i = gettown(Ind);
	/* hack: non-town stores (ie dungeon, but could also be wild) are borrowed from town #0 - C. Blue */
	if (i == -1) i = gettown_dun(Ind);

#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2) /* it's a player's private store! */
		st_ptr = &fake_store[-2 - p_ptr->store_num];
	else
#endif
	st_ptr = &town[i].townstore[p_ptr->store_num];
//	ot_ptr = &owners[p_ptr->store_num][st_ptr->owner];
	ot_ptr = &ow_info[st_ptr->owner];

	*price = 0;


	/* Obtain the starting offer and the final offer */
#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2)
		cur_ask = final_ask = price_item_player_store(Ind, o_ptr);
	else
#endif
	{
		cur_ask = price_item(Ind, o_ptr, ot_ptr->max_inflate, TRUE);
		final_ask = price_item(Ind, o_ptr, ot_ptr->min_inflate, TRUE);
	}

	/* Get the owner's payout limit */
	purse = (s64b)(ot_ptr->max_cost);

	/* No reason to haggle */
	if (final_ask >= purse) {
		/* Message */
		msg_print(Ind, "You instantly agree upon the price.");

		/* Offer full purse */
		final_ask = purse;
	}

	/* No need to haggle */
	else {
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
	    (sti_ptr->owners[0] == sti_ptr->owners[3]) &&
	    (sti_ptr->owners[0] == sti_ptr->owners[4]) &&
	    (sti_ptr->owners[0] == sti_ptr->owners[5])) /* MAX_STORE_OWNERS */
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

#ifdef PLAYER_STORES
	/* Hack: Assume that in player stores all actions are legal
	         that work for a certain 'normal' store too */
	if (store <= -2) store = 0;
#endif

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

	int i, item_new, amt = 1;
	long chance = 0;

	s64b best, tbest, tcadd, tccompare;

	object_type sell_obj, *o_ptr;
	char o_name[ONAME_LEN];

	byte old_discount;
	u16b old_note;

	/* Get town or dungeon the store is located within */
	i = gettown(Ind);
	/* hack: non-town stores (ie dungeon, but could also be wild) are borrowed from town #0 - C. Blue */
	if (i == -1) i = gettown_dun(Ind);

#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2) /* it's a player's private store! */
		st_ptr = &fake_store[-2 - p_ptr->store_num];
	else
#endif
	/* Store in which town? Or in the dungeon? */
	st_ptr = &town[i].townstore[st];

	/* Sanity check - mikaelh */
	if (item < 0 || item >= st_ptr->stock_size)
		return;

	/* Get the actual item */
	o_ptr = &st_ptr->stock[item];

	/* Check that it's a real item - mikaelh */
	if (!o_ptr->tval) return;

	if (p_ptr->store_num == STORE_HOME) {
		msg_print(Ind, "You don't steal from your home!");
		return;
	}

	if (compat_pomode(Ind, o_ptr)) {
		msg_format(Ind, "You cannot take items of %s players!", compat_pomode(Ind, o_ptr));
		return;
	}

	if (p_ptr->pstealing) {
		msg_print(Ind, "You're still not calm enough to steal again..");
		return;
	}

	/* Level restriction (mainly anticheeze) */
#ifndef RPG_SERVER
	if (p_ptr->lev < 5) {
		msg_print(Ind, "You dare not to! Your level needs to be 5 at least.");
		return;
	} else if ((p_ptr->lev < 10) && (object_value(Ind, o_ptr) >= 200)) {
		msg_print(Ind, "You dare not to steal this expensive item! Hit level 10 first.");
		return;
	}
/*
	else if ((p_ptr->lev < 15) && (object_value(Ind, o_ptr) >= 1000))
	{
		msg_print(Ind, "You dare not to steal this expensive item! Hit level 10 first.");
		return;
	}
*/
#else
	if (p_ptr->lev < 5 && object_value(Ind, o_ptr) >= 200) {
		msg_print(Ind, "You dare not to steal this expensive item! Hit level 5 first.");
		return;
	}
#endif

	/* I'm not saying this is the way to go, but they
	   can cheeze by attempting repeatedly */
	if (p_ptr->tim_blacklist || p_ptr->tim_watchlist || st_ptr->tim_watch) {
		msg_print(Ind, "\"Bastard Thief! Get out of my shop!!!\"");
		msg_print_near(Ind, "You hear loud shouting..");
		msg_format_near(Ind, "an angry shopkeeper kicks %s out of the shop!", p_ptr->name);
#if 0
		/* increase/set player's blacklist timer */
		if (p_ptr->tim_blacklist < 10000000)	/* 10 million turns is LONG ENOUGH */
			p_ptr->tim_blacklist += 1000;	/* add a little */
//			p_ptr->tim_watchlist += 1;	/* add a little (a day/night period) */
			p_ptr->tim_watchlist += 1000;

 #if 0
		/* increase owner's extra attentiveness timer */
		st_ptr->tim_watch += 100;
		if (st_ptr->tim_watch > 300) st_ptr->tim_watch = 300;
 #endif
#endif
		/* player gets kicked out */
		store_kick(Ind, FALSE);
		return;
	}

	if (p_ptr->store_num == -1) {
		msg_print(Ind,"You left the shop!");
		return;
	}

//	ot_ptr = &owners[st][st_ptr->owner];
	ot_ptr = &ow_info[st_ptr->owner];

	if (!store_attest_command(p_ptr->store_num, BACT_STEAL)) {
		//if (!is_admin(p_ptr)) return;
		return;
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
	if (o_ptr->tval == TV_WAND) {
		sell_obj.pval = divide_charged_item(o_ptr, amt);
	}

	/*
	 * Hack -- Mark the item temporarily as stolen so that the
	 * inven_carry_okay check works properly - mikaelh
         */
	old_discount = sell_obj.discount;
	old_note = sell_obj.note;
	sell_obj.discount = 100;
	sell_obj.note = quark_add("stolen");

	/* Hack -- require room in pack */
	if (!inven_carry_okay(Ind, &sell_obj)) {
		msg_print(Ind, "You cannot carry that many different items.");
		return;
	}

	/* Restore the item */
	sell_obj.discount = old_discount;
	sell_obj.note = old_note;

	/* Describe the transaction */
	object_desc(0, o_name, &sell_obj, TRUE, 3);

	/* Determine the "best" price (per item) */
	/* NOTE: it's used to determine the penalty when stealing failed */
#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2)
		best = price_item_player_store(Ind, &sell_obj);
	else
#endif
	best = price_item(Ind, &sell_obj, ot_ptr->min_inflate, FALSE);

	/* shopkeeper watches expensive items carefully */
	tcadd = 100;
	tbest = object_value(Ind, o_ptr);/* Note- object_value_real would not take
					    into account the discount of the item */
	if (tbest < 1) tbest = 1; /* shops shouldn't offer items worth less,
				     but maybe this object_value call returns <= 0,
				     at least we had a panic save in ../tbest somewhere below */

	if (tbest > 10000000) tbest = 10000000;
	tccompare = 10;
	while((tccompare / 10) < tbest) /* calculate square root - C. Blue */
	{
		tcadd = (tcadd * 114) / 100;
		tccompare = (tccompare * 13) / 10;
	}
	tcadd /= 100;
	if (tcadd < 1) tcadd = 1;
	tcadd = 57000 / ((10000 / tcadd) + 50);

	/* Player tries to steal it */
	chance = ((50) - p_ptr->stat_ind[A_DEX]) +
	    ((((sell_obj.weight * amt) / 2) + tcadd) /
	    (2 + get_skill_scale(p_ptr, SKILL_STEALING, 15))) -
	    (get_skill_scale(p_ptr, SKILL_STEALING, 25));

	/* Very simple items (value <= stealingskill * 10)
	   [that occur in stacks? -> no for now, too nasty]
	   get less attention from the shopkeeper, so you don't
	   get blacklisted all the time for snatching some basic stuff - C. Blue */
	if (get_skill_scale(p_ptr, SKILL_STEALING, 50) >= 1) {
		if (tbest <= get_skill_scale(p_ptr, SKILL_STEALING, 500))
			chance = ((chance * (long)(tbest)) / get_skill_scale(p_ptr, SKILL_STEALING, 500));
	}

	/* Invisibility and stealth are not unimportant */
	chance = (chance * (100 - (p_ptr->invis > 0 ? 10 : 0))) / 100;
	chance = (chance * (115 - ((p_ptr->skill_stl * 3) / 4))) / 100;

	/* avoid div0 */
	if (chance < 1) chance = 1;

	/* shopkeepers in special shops are often especially careful */
	if (st_info[st_ptr->st_idx].flags1 & SF1_VHARD_STEAL) {
		if (chance < 10) chance = 10;
		chance = chance * 2 + 20;
	} else if (st_info[st_ptr->st_idx].flags1 & SF1_HARD_STEAL) {
		if (chance < 10) chance = 10;
		chance *= 2;
	}

	/* limit steal-back cheeze for loot that you (or someone else) sold previously */
	if (sell_obj.owner) chance = chance * 2 + 20;

	/* always 1% chance to fail, so that ppl won't macro it */
	/* 1% pfft. 5% and rising... */
	if (rand_int(chance) <= 10 && !magik(5)) {
		/* Hack -- buying an item makes you aware of it */
		object_aware(Ind, &sell_obj);

		/* Hack -- clear the "fixed" flag from the item */
		sell_obj.ident &= ~ID_FIXED;

		/* Stolen items cannot be sold */
		sell_obj.discount = 100;
		sell_obj.note = quark_add("stolen");

#ifdef STEAL_CHEEZEREDUCTION
//		if (!magik((5000000 / tbest) + 5))
		if (!magik((5000000 / object_value_real(0, &sell_obj)) + 5))
			sell_obj.level = 0;
#endif

		/* Message */
		msg_format(Ind, "You stole %s.", o_name);
#ifdef USE_SOUND_2010
		sound_item(Ind, sell_obj.tval, sell_obj.sval, "pickup_");
#endif

		/* Let the player carry it (as if he picked it up) */
		can_use(Ind, &sell_obj);//##UNH
		item_new = inven_carry(Ind, &sell_obj);

		/* Describe the final result */
		object_desc(Ind, o_name, &p_ptr->inventory[item_new], TRUE, 3);

//		s_printf("Stealing: %s (%d) succ. %s (chance %d%% (%d)).\n", p_ptr->name, p_ptr->lev, o_name, 950 / (chance<10?10:chance), chance);
		/* let's instead display the chance without regards to 5% chance to fail, since very small % numbers become more accurate! */
		if (chance > 10)
		s_printf("Stealing: %s (%d) succ. %s (chance %d%%0 (%d) %d,%d,%d).\n", p_ptr->name, p_ptr->lev, o_name, 10000 / (chance < 10 ? 10 : chance), chance, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
		if (sell_obj.tval == TV_SCROLL && sell_obj.sval == SV_SCROLL_ARTIFACT_CREATION)
		        s_printf("ARTSCROLL stolen by %s.\n", p_ptr->name);

		/* Message */
		msg_format(Ind, "You have %s (%c).", o_name, index_to_label(item_new));

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
		if (st_ptr->stock_num == 0) {
			/* This should do a nice restock */
			//st_ptr->last_visit = 0;
			st_ptr->last_visit = -10L * cfg.store_turns;
		}

		/* The item is gone */
		//else
		if (st_ptr->stock_num != i) {
			/* Redraw everything */
			display_inventory(Ind);
		}

		/* Item is still here */
		else {
			/* Redraw the item */
			display_entry(Ind, item);
		}
		//suppress_message = FALSE;

	}

	else {
//s_printf("Stealing: %s (%d) fail. %s (chance %d%% (%d)).\n", p_ptr->name, p_ptr->lev, o_name, 950 / (chance<10?10:chance), chance);
s_printf("Stealing: %s (%d) fail. %s (chance %d%%0 (%d) %d,%d,%d).\n", p_ptr->name, p_ptr->lev, o_name, 10000 / (chance<10?10:chance), chance, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
		/* Complain */
		// say_comment_4();
		msg_print(Ind, "\"\377yBastard\377L!!!'\377w\" - The angry shopkeeper throws you out!");
		msg_print(Ind, "\377rNow you'll be on the black list of merchants for a while..");
		msg_print_near(Ind, "You hear loud shouting..");
		msg_format_near(Ind, "an angry shopkeeper kicks %s out of the store!", p_ptr->name);

		/* Reset insults */
		st_ptr->insult_cur = 0;
		st_ptr->good_buy = 0;
		st_ptr->bad_buy = 0;

		/* Kicked out for a LONG time */
		i = tcadd * amt * 8 + 1000;
		/* Reduce blacklist time for very cheap stuff (value <= skill * 10): */
		if (get_skill_scale(p_ptr, SKILL_STEALING, 50) >= 1) {
			if (tbest <= get_skill_scale(p_ptr, SKILL_STEALING, 500)) {
				/* Limits for ultra-cheap items: */
				if (get_skill_scale(p_ptr, SKILL_STEALING, 500) / tbest <= 5)
					i = (i * tbest) / get_skill_scale(p_ptr, SKILL_STEALING, 500);
				else
					i /= 5;
			}
		}
		/* Paranoia, currently not needed */
		if (i < 100) i = 100;

		/* Put him on the blacklist or increase it if already on */
		p_ptr->tim_blacklist += i;

		/* watchlist - the more known a character is, the longer he remains on it */
#if 1
//		p_ptr->tim_watchlist += i * 2;
		p_ptr->tim_watchlist += i + 50; //300?
#endif
#if 0 /* use normal turns */
		if (p_ptr->max_lev <= 20)
		    i = 0;// seconds (fps % 60)
		else if (p_ptr->max_lev <= 34)
		    i = 250;
		else if (p_ptr->max_lev <= 43)
		    i = 500;
		else
		    i = 750;
		p_ptr->tim_watchlist += i;
#endif
#if 0 /* use day/night cycles */
		if (p_ptr->max_lev <= 20)
		    i = 0;
		else if (p_ptr->max_lev <= 34)
		    i = 1;
		else if (p_ptr->max_lev <= 43)
		    i = 2;
		else
		    i = 3;
		p_ptr->tim_watchlist += i;
#endif

		/* store owner is more careful from now on, for a while */
#if 0 /* not that urgent anymore after Minas' XBM was set to NO_STEAL */
		i = tcadd * 8 + 1000;
		st_ptr->tim_watch += i / 20;
		if (st_ptr->tim_watch > 300) st_ptr->tim_watch = 300;
		st_ptr->last_theft = turn;
#endif

		/* Of course :) */
		store_kick(Ind, FALSE);
	}

	/* set timeout before attempting to shoplift again */
	p_ptr->pstealing = 0; /* 10 turns aka 6 seconds */

	/* Not kicked out */
	return;
}


/*
 * Buy an item from a store				-RAK-
 */
void store_purchase(int Ind, int item, int amt)
{
	player_type *p_ptr = Players[Ind];

	store_type *st_ptr;
	owner_type *ot_ptr;

	int		i, choice;
	int		item_new;

	s64b		price, best;

	object_type	sell_obj;
	object_type	*o_ptr;

	char		o_name[ONAME_LEN];

	if (amt < 1) {
		s_printf("$INTRUSION$ Bad amount %d! Bought by %s.\n", amt, p_ptr->name);
//		msg_print(Ind, "\377RInvalid amount. Your attempt has been logged.");
		return;
	}

	if (p_ptr->store_num == STORE_HOME) /* in defines.h */
	//if (p_ptr->store_num == STORE_HOME)
	{
		home_purchase(Ind, item, amt);
		return;
	}

	i = gettown(Ind);
	/* hack: non-town stores (ie dungeon, but could also be wild) are borrowed from town #0 - C. Blue */
        if (i == -1) i = gettown_dun(Ind);

	if (p_ptr->store_num == -1) {
		msg_print(Ind,"You left the shop!");
		return;
	}

#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2) /* it's a player's private store! */
		st_ptr = &fake_store[-2 - p_ptr->store_num];
	else
#endif
	st_ptr = &town[i].townstore[p_ptr->store_num];
//	ot_ptr = &owners[st][st_ptr->owner];
	ot_ptr = &ow_info[st_ptr->owner];

	if (!store_attest_command(p_ptr->store_num, BACT_BUY)) {
		/* Hack -- admin can 'buy'
		 * (it's to remove crap from the Museums) */
		if (!is_admin(p_ptr)) return;
	}

	/* Empty? */
	if (st_ptr->stock_num <= 0) {
		if (p_ptr->store_num == STORE_HOME) msg_print(Ind, "Your home is empty.");
		else msg_print(Ind, "I am currently out of stock.");
		return;
	}


#if 0
	/* Find the number of objects on this and following pages */
	i = (st_ptr->stock_num - store_top);

	/* And then restrict it to the current page */
	if (i > 12) i = 12;

	/* Prompt */
	if (p_ptr->store_num == STORE_HOME) {
		snprintf(out_val, sizeof(out_val), "Which item do you want to take? ");
	} else {
		snprintf(out_val, sizeof(out_val), "Which item are you interested in? ");
	}

	/* Get the item number to be bought */
	if (!get_stock(&item, out_val, 0, i-1)) return;
#endif

	/* Sanity check - mikaelh */
	if (item < 0 || item >= st_ptr->stock_size)
		return;

	/* Get the actual item */
	o_ptr = &st_ptr->stock[item];

	/* Check that it's a real item - mikaelh */
	if (!o_ptr->tval) return;

#ifdef PLAYER_STORES
	/* Consistency check: Make sure noone inside a mang-house store
	   has actually picked up or otherwise modified the item we're buying. */
	if (p_ptr->store_num <= -2) {
		object_type *ho_ptr = NULL;
		house_type *h_ptr;
		int h_idx;
		cave_type **zcave, *c_ptr;

		if (!(zcave = getcave(&p_ptr->wpos))) {
			s_printf("PLAYER_STORE_PURCHASE: NO ZCAVE!\n");
			return; /* can't happen, paranoia */
		}

		/* Try to locate house linked to this store */
		h_idx = pick_house(&p_ptr->wpos, p_ptr->ps_house_y, p_ptr->ps_house_x);
		if (h_idx == -1) {
			s_printf("PLAYER_STORE_PURCHASE: NO HOUSE!\n");
			return; /* oops, he sold his house while we were shopping? :) */
		}
		h_ptr = &houses[h_idx];

		/* Access original item in the house */
		if (h_ptr->flags & HF_TRAD) {
			if (h_ptr->stock_num <= o_ptr->ps_idx_x) {
				s_printf("PLAYER_STORE_PURCHASE: BAD stock_num!\n");
				msg_print(Ind, "The shopkeeper just modified the store, please re-enter!");
				return; /* oops, the owner took out some item(s) */
			}
			ho_ptr = &h_ptr->stock[o_ptr->ps_idx_x];
		} else {
			/* ALL houses are currently rectangular, so this check seems obsolete.. */
			if (h_ptr->flags & HF_RECT) {
				c_ptr = &zcave[o_ptr->ps_idx_y][o_ptr->ps_idx_x];
				if (!c_ptr->o_idx) {
					s_printf("PLAYER_STORE_PURCHASE: BAD STOCK x,y!\n");
					msg_print(Ind, "The shopkeeper just modified the store, please re-enter!");
					return; /* oops, the owner picked up our item */
				}
				ho_ptr = &o_list[c_ptr->o_idx];
			}
		}

		/* Some item was still at the position we want to buy from,
		   so now verify that it's still the SAME item. */
		if (ho_ptr == NULL) {
			s_printf("PLAYER_STORE_PURCHASE: BAD ho_ptr!\n");
			return; /* shouldn't happen (except if we add non-rectangular mang houses.. */
		}
		if (ho_ptr->tval != o_ptr->tval || ho_ptr->sval != o_ptr->sval) {
			s_printf("PLAYER_STORE_PURCHASE: BAD STOCK tval/sval!\n");
			msg_print(Ind, "The shopkeeper just modified the store, please re-enter!");
			return; /* oops, the owner swapped our item for something else */
		}
		if (ho_ptr->number != o_ptr->number) {
			s_printf("PLAYER_STORE_PURCHASE: BAD STOCK number! (%d vs %d)\n", ho_ptr->number, o_ptr->number);
			msg_print(Ind, "The shopkeeper just modified the store, please re-enter!");
			return; /* oops, the owner took away or added some of that item type.
			           Actually this could be handled, but we're strict for now. */
		}
	}
#endif

	/* check whether client tries to buy more than the store has */
	if (o_ptr->number < amt) {
		s_printf("$INTRUSION$ Too high amount %d of %d! Bought by %s.\n", amt, o_ptr->number, p_ptr->name);
//		msg_print(Ind, "\377RInvalid amount. Your attempt has been logged.");
		return;
	}

	if (compat_pomode(Ind, o_ptr)) {
		msg_format(Ind, "You cannot take items of %s players!", compat_pomode(Ind, o_ptr));
		return;
	}

	/* prevent winners picking up true arts accidentally */
	if (true_artifact_p(o_ptr) && !winner_artifact_p(o_ptr) &&
	    p_ptr->total_winner && cfg.kings_etiquette) {
		msg_print(Ind, "Royalties may not own true artifacts!");
		if (!is_admin(p_ptr)) return;
	}

	/* Check if the player is powerful enough for that item */
	if ((o_ptr->owner) && (o_ptr->owner != p_ptr->id) &&
	    (o_ptr->level > p_ptr->lev || o_ptr->level == 0)) {
		if (cfg.anti_cheeze_pickup) {
			msg_print(Ind, "You aren't powerful enough yet to pick up that item!");
			return;
		}
		if (true_artifact_p(o_ptr) && cfg.anti_arts_pickup) {
			msg_print(Ind, "You aren't powerful enough yet to pick up that artifact!");
			return;
		}
	}
	
	if ((k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) && !p_ptr->once_winner
	    && !p_ptr->total_winner) { /* <- obsolete. Added that one just for testing when admin char sets .total_winner=1 */
		msg_print(Ind, "Only royalties are powerful enough to pick up that item!");
		if (!is_admin(p_ptr)) return;
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
	if (o_ptr->tval == TV_WAND) {
		if (o_ptr->tval == TV_WAND) {
			sell_obj.pval = divide_charged_item(o_ptr, amt);
		}
	}

	/* Hack -- require room in pack */
	if (!inven_carry_okay(Ind, &sell_obj)) {
		msg_print(Ind, "You cannot carry that many different items.");
		return;
	}

	/* Determine the "best" price (per item) */
#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2)
		best = price_item_player_store(Ind, &sell_obj);
	else
#endif
	best = price_item(Ind, &sell_obj, ot_ptr->min_inflate, FALSE);

#if 0
	/* Find out how many the player wants */
	if (o_ptr->number > 1) {
		/* Hack -- note cost of "fixed" items */
		if ((p_ptr->store_num != 7) && (o_ptr->ident & ID_FIXED))
		{
			msg_format("That costs %ld gold per item.", (long int)(best));
		}

		/* Get a quantity */
		amt = get_quantity(NULL, o_ptr->number);

		/* Allow user abort */
		if (amt <= 0) return;
	}
#endif

	/* Attempt to buy it */
	if (p_ptr->store_num != 7) {
		/* For now, I'm assuming everything's price is "fixed" */
		/* Fixed price, quick buy */
#if 0
		if (o_ptr->ident & ID_FIXED) {
#endif
			/* Assume accept */
			choice = 0;

			/* Go directly to the "best" deal */
			price = (best * sell_obj.number);
#if 0
		}

		/* Haggle for it */
		else {
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
		if (choice == 0) {
			/* Fix the item price (if "correctly" haggled) */
			if (price == (best * sell_obj.number)) o_ptr->ident |= ID_FIXED;

			/* Player can afford it */
			if (p_ptr->au >= price) {
				if (p_ptr->taciturn_messages) suppress_message = TRUE;

				/* Say "okay" */
				say_comment_1(Ind);

				/* Be happy */
				/*decrease_insults();*/

				/* Spend the money */
				p_ptr->au -= price;

				/* Buying things lessen the distrust somewhat */
				if (p_ptr->tim_blacklist && (price > 100)) {
					p_ptr->tim_blacklist -= price / 100;
					if (p_ptr->tim_blacklist < 1) p_ptr->tim_blacklist = 1;
				}

				/* Update the display */
				store_prt_gold(Ind);

				/* Hack -- buying an item makes you aware of it */
				object_aware(Ind, &sell_obj);

				/* Hack -- clear the "fixed" flag from the item */
				sell_obj.ident &= ~ID_FIXED;

#ifdef PLAYER_STORES
				/* If we buy from a player store, erase the item inscription! */
				if (p_ptr->store_num <= -2) {
					sell_obj.note = 0;
					/* handle the original items and cheque processing */
					player_store_handle_purchase(Ind, o_ptr, &sell_obj, price);
				}
#endif

				/* Describe the transaction */
				object_desc(Ind, o_name, &sell_obj, TRUE, 3);

				/* Message */
				msg_format(Ind, "You bought %s for %ld gold.", o_name, (long int)price);
if (sell_obj.tval == TV_SCROLL && sell_obj.sval == SV_SCROLL_ARTIFACT_CREATION)
	s_printf("ARTSCROLL bought by %s for %ld gold (#=%d).\n", p_ptr->name, (long int)price, sell_obj.number);
#ifdef USE_SOUND_2010
				sound_item(Ind, sell_obj.tval, sell_obj.sval, "pickup_");
#endif

				/* Let the player carry it (as if he picked it up) */
				can_use(Ind, &sell_obj);//##UNH
				item_new = inven_carry(Ind, &sell_obj);

				/* Describe the final result */
				object_desc(Ind, o_name, &p_ptr->inventory[item_new], TRUE, 3);

				/* Message */
				msg_format(Ind, "You have %s (%c).", o_name, index_to_label(item_new));

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
				if (st_ptr->stock_num == 0) {
					/* nothing (was maybe once: shuffle) */
				}

				/* The item is gone */
				else if (st_ptr->stock_num != i) {
					/* Redraw everything */
					display_inventory(Ind);
				}

				/* Item is still here */
				else {
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
	else {
		/* Carry the item */
		can_use(Ind, &sell_obj);//##UNH correct?
		item_new = inven_carry(Ind, &sell_obj);

		/* Describe just the result */
		object_desc(Ind, o_name, &p_ptr->inventory[item_new], TRUE, 3);

		/* Message */
		msg_format(Ind, "You have %s (%c).", o_name, index_to_label(item_new));
#ifdef USE_SOUND_2010
		sound_item(Ind, sell_obj.tval, sell_obj.sval, "pickup_");
#endif

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
		if (i == st_ptr->stock_num) {
			/* Redraw the item */
			display_entry(Ind, item);
		}
		/* The item is gone */
		else {
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

	s64b		price;

	object_type		sold_obj;
	object_type		*o_ptr;

	char		o_name[ONAME_LEN];

	/* Sanity check */
	if (item < 0 || item >= INVEN_TOTAL) return;

	/* Check for client-side exploit! */
	if (p_ptr->inventory[item].number < amt) {
		s_printf("$INTRUSION$ Bad amount %d of %d! (Home) Sold by %s.\n", amt, p_ptr->inventory[item].number, p_ptr->name);
		msg_print(Ind, "You don't have that many!");
		return;
	}

	if (p_ptr->store_num == STORE_HOME) {
		home_sell(Ind, item, amt);
		return;
	}

	/* sanity check - Yakina - */
	if (p_ptr->store_num == -1) {
		msg_print(Ind,"You left the shop!");
		return;
	}

//	st_ptr = &town[gettown(Ind)].townstore[p_ptr->store_num];

	if (!store_attest_command(p_ptr->store_num, BACT_SELL)) return;

	/* You can't sell 0 of something. */
	if (amt <= 0) return;

	/* Get the item (in the pack) */
	if (item >= 0) {
		/* Sanity check - mikaelh */
		if (item < 0 || item >= INVEN_TOTAL)
			return;

		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else {
		o_ptr = &o_list[0 - item];
	}

	/* Check that it's a real item - mikaelh */
	if (!o_ptr->tval) return;

	/* Check for validity of sale */
	if (!store_will_buy(Ind, o_ptr)) {
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

	/* Wands get their charges divided - mikaelh */
	if (o_ptr->tval == TV_WAND) {
		sold_obj.pval = o_ptr->pval * amt / o_ptr->number;
	}

	/* Get a full description */
	object_desc(Ind, o_name, &sold_obj, TRUE, 3);

	/* Remove any inscription for stores */
	if (p_ptr->store_num != 7) {
		sold_obj.note = 0;
		sold_obj.note_utag = 0;
	}

	/* Access the store */
//	i=gettown(Ind);
//	store_type *st_ptr = &town[townval].townstore[i];

	/* Is there room in the store (or the home?) */
	if (gettown(Ind) != -1) { //DUNGEON STORES
		if (!store_check_num(&town[gettown(Ind)].townstore[p_ptr->store_num], &sold_obj)) {
			if (p_ptr->store_num == STORE_HOME) msg_print(Ind, "Your home is full.");
			else msg_print(Ind, "I have not the room in my store to keep it.");
			return;
		}
	} else {
		if (!store_check_num(&town[gettown_dun(Ind)].townstore[p_ptr->store_num], &sold_obj)) {
			if (p_ptr->store_num == STORE_HOME) msg_print(Ind, "Your home is full.");
			else msg_print(Ind, "I have not the room in my store to keep it.");
			return;
		}
	}

	/* Museum */
//	if (p_ptr->store_num == 57)
	if (st_info[p_ptr->store_num].flags1 & SF1_MUSEUM) {
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
	long item, amt;
	s64b price, price_redundance, value;

	object_type *o_ptr, sold_obj;
	char o_name[ONAME_LEN];
	int item_pos = -1;
	bool museum;

	/* Abort if we shouldn't be getting called */
	if (p_ptr->current_selling == -1)
		return;

	if (p_ptr->store_num == -1) {
		msg_print(Ind,"You left the building!");
		return;
	}

	museum = (st_info[p_ptr->store_num].flags1 & SF1_MUSEUM) ? TRUE : FALSE;

	/* Restore the variables */
	item = p_ptr->current_selling;
	amt = p_ptr->current_sell_amt;
	price = p_ptr->current_sell_price;

	/* -------------------Repeated checks in case inven changed (rods recharging->restacking) */
	/* Get the item (in the pack) */
	if (item >= 0) {
		/* Sanity check - mikaelh */
		if (item < 0 || item >= INVEN_TOTAL)
			return;

		o_ptr = &p_ptr->inventory[item];
	}
	/* Get the item (on the floor) */
	else {
#if 0
		o_ptr = &o_list[0 - item];
#else
		return;
#endif
	}
	/* Check that it's a real item - mikaelh */
	if (!o_ptr->tval) return;
	/* Check for validity of sale */
	if (!store_will_buy(Ind, o_ptr)) {
		msg_print(Ind, "I don't want that!");
		return;
	}
	/* -------------------------------------------------------------------------------------- */

#if 0 /* done above already */
	/* Get the inventory item */
	o_ptr = &p_ptr->inventory[item];
#endif

	/* Server-side exploit checks - C. Blue */
	if (amt <= 0) {
		s_printf("$INTRUSION$ (FORCED) Bad amount %d! Sold by %s.\n", amt, p_ptr->name);
		return;
	}
	if (o_ptr->number < amt) {
		s_printf("$INTRUSION$ Bad amount %d of %d! Sold by %s.\n", amt, o_ptr->number, p_ptr->name);
		msg_print(Ind, "You don't have that many!");
		return;
	}
	sold_obj = *o_ptr;
	sold_obj.number = amt;

	/* Wands get their charges divided - mikaelh */
	if (o_ptr->tval == TV_WAND) {
		sold_obj.pval = o_ptr->pval * amt / o_ptr->number;
	}

	(void) sell_haggle(Ind, &sold_obj, &price_redundance);
	if (price != price_redundance) {
		s_printf("$INTRUSION$ Tried to sell %ld for %ld! Sold by %s.\n", (long int)price_redundance, (long int)price, p_ptr->name);
#if 0
		msg_print(Ind, "Wrong item!");
		return;
#else
		price = price_redundance;
		if (!price) return;
		/* Paranoia - Don't sell '(nothing)s' */
		if (!o_ptr->k_idx) return;
#endif
	}
	
	/* Add '!s' inscription, w00t - C. Blue */
	if (check_guard_inscription(o_ptr->note, 's')) {
		msg_print(Ind, "The item's inscription prevents it");
		return;
	}

	/* hack: prevent s32b overflow */
	if (2000000000 - o_ptr->pval < p_ptr->au) {
		msg_format(Ind, "\377yYou cannot carry more than 2 billion worth of gold!");
		return;
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
	if (o_ptr->tval == TV_WAND) sold_obj.pval = divide_charged_item(o_ptr, amt);

	/* Get the "actual" value */
	value = object_value(Ind, &sold_obj) * sold_obj.number;

	/* Get the description all over again */
	object_desc(Ind, o_name, &sold_obj, TRUE, 3);

	/* Describe the result (in message buffer) */
	if (!museum) msg_format(Ind, "You sold %s for %ld gold.", o_name, (long int)price);
	else msg_format(Ind, "You donate %s.", o_name);
#ifdef USE_SOUND_2010
	sound_item(Ind, o_ptr->tval, o_ptr->sval, "drop_");
#endif

	/* Analyze the prices (and comment verbally) */
	/*purchase_analyze(price, value, dummy);*/

	/* Take the item from the player, describe the result */
	inven_item_increase(Ind, item, -amt);
	inven_item_describe(Ind, item);
	inven_item_optimize(Ind, item);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Artifact won't be sold in a store */
	if ((cfg.anti_arts_shop || p_ptr->total_winner) && true_artifact_p(&sold_obj) && !museum) {
		handle_art_d(sold_obj.name1);
		return;
	}

	/* The store gets that (known) item */
//	if(sold_obj.tval!=8)	// What was it for.. ?
	if (gettown(Ind) != -1) //DUNGEON STORES
		item_pos = store_carry(&town[gettown(Ind)].townstore[p_ptr->store_num], &sold_obj);
#if 0 /* have dungeon shops eat the item to prevent cheezy transfers */
	else
		/* mostly for RPG_SERVER */
		if (p_ptr->store_num != STORE_STRADER)
			item_pos = store_carry(&town[gettown_dun(Ind)].townstore[p_ptr->store_num], &sold_obj);
		else
			if (true_artifact_p(&sold_obj)) {
				handle_art_d(sold_obj.name1);
			}
//		item_pos = store_carry(p_ptr->store_num, &sold_obj);
#else
	else
		/* Make artifact findable - mikaelh */
		if (true_artifact_p(&sold_obj)) {
			handle_art_d(sold_obj.name1);
		}
#endif
	/* Resend the basic store info */
	display_store(Ind);

	/* Re-display if item is now in store */
	if (item_pos >= 0) display_inventory(Ind);
}


/*
 * Examine an item in a store			   -JDL-
 */
void store_examine(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	store_type *st_ptr;
	owner_type *ot_ptr;

	int			i;

	object_type		*o_ptr;

	if (p_ptr->store_num == STORE_HOME) {
		home_examine(Ind, item);
		return;
	}

	i = gettown(Ind);
	/* hack: non-town stores (ie dungeon, but could also be wild) are borrowed from town #0 - C. Blue */
        if (i == -1) i = gettown_dun(Ind);

#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2) /* it's a player's private store! */
		st_ptr = &fake_store[-2 - p_ptr->store_num];
	else
#endif
	st_ptr = &town[i].townstore[p_ptr->store_num];
//	ot_ptr = &owners[st][st_ptr->owner];
	ot_ptr = &ow_info[st_ptr->owner];

	if (!store_attest_command(p_ptr->store_num, BACT_EXAMINE)) {
		//if (!is_admin(p_ptr)) return;
		return;
	}

	/* Empty? */
	if (st_ptr->stock_num <= 0) {
		if (p_ptr->store_num == STORE_HOME) msg_print(Ind, "Your home is empty.");
		else msg_print(Ind, "I am currently out of stock.");
		return;
	}

	/* Sanity check - mikaelh */
	if (item < 0 || item >= st_ptr->stock_size) return;

	/* Get the actual item */
	o_ptr = &st_ptr->stock[item];

	/* Check that it's a real item - mikaelh */
	if (!o_ptr->tval) return;

	/* Require full knowledge */
	if (!(o_ptr->ident & ID_MENTAL) && !is_admin(p_ptr)) observe_aux(Ind, o_ptr);
	/* Describe it fully */
	else if (!identify_fully_aux(Ind, o_ptr)) msg_print(Ind, "You see nothing special.");

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
	int which;
	int i, j;
	int maintain_num;

	cave_type *c_ptr;
	struct c_special *cs_ptr;

	/* Access the player grid */
	cave_type **zcave;
	if(!(zcave = getcave(&p_ptr->wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	i = gettown(Ind);
	/* hack: non-town stores are borrowed from town #0 - C. Blue */
	if (i == -1) i = gettown_dun(Ind);

	/* Verify a store */
#if 0
	if (!((c_ptr->feat >= FEAT_SHOP_HEAD) &&
	      (c_ptr->feat <= FEAT_SHOP_TAIL)))
#endif
	if (c_ptr->feat != FEAT_SHOP) {
		msg_print(Ind, "You see no store here.");
		return;
	}

	/* Extract the store code */
//	which = (c_ptr->feat - FEAT_SHOP_HEAD);

	if ((cs_ptr = GetCS(c_ptr, CS_SHOP))) {
		which = cs_ptr->sc.omni;
	} else {
		msg_print(Ind, "You see no store here.");
		return;
	}

	if (gettown(Ind) != -1) {
		/* Hack -- Check the "locked doors" */
		if (town[i].townstore[which].store_open >= turn) {
			msg_print(Ind, "The doors are locked.");
			return;
		}
	}

	/* Hack -- Ignore the home */
	if (which == STORE_HOME) {	/* XXX It'll change */
		/* msg_print(Ind, "The doors are locked."); */
		return;
	}
#if 0	/* it have changed */
	else if (which > 7) {	/* XXX It'll change */
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

#if 0 /* This assumes that the same store index does not occur in both town and dungeon at once! */
	if (gettown(Ind) != -1) {
		st_ptr = &town[i].townstore[which];

		/* Make sure if someone is already in */
		for (i = 1; i <= NumPlayers; i++) {
			/* Check this player */
			j = gettown(i);
			if (j != -1) {
				if (st_ptr == &town[j].townstore[Players[i]->store_num]) {
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
		for (i = 1; i <= NumPlayers; i++) {
			/* Check this player */
			j = gettown(i);
			if (j == -1) {
				if (st_ptr == &town[gettown_dun(i)].townstore[Players[i]->store_num]) {
					msg_print(Ind, "The store is full.");
					store_kick(Ind, FALSE);
					return;
				}
			}
		}
	}
#else /* This assumes that the same store index could occur in both town and dungeon at once! */
	st_ptr = &town[i].townstore[which];

	/* Make sure if someone is already in */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this player */
		j = gettown(i);
		//DUNGEON STORES
		if (j == -1) j = gettown_dun(i);

		if (st_ptr == &town[j].townstore[Players[i]->store_num]) {
			if (Players[i]->admin_dm && !p_ptr->admin_dm) {
				store_kick(i, FALSE);
				msg_print(i, "Someone has entered the store.");
				/* hack: repair grid's player index again */
				c_ptr->m_idx = -Ind;
			} else {
				msg_print(Ind, "The store is full.");
				store_kick(Ind, FALSE);
				return;
			}
		}
	}
#endif

	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
	handle_stuff(Ind); /* update stealth/search display now */

	/* process theft watch list of the store owner */
	if (st_ptr->tim_watch) {
		if ((turn - st_ptr->last_theft) / cfg.fps > st_ptr->tim_watch)
			st_ptr->tim_watch = 0;
		else
			st_ptr->tim_watch -= (turn - st_ptr->last_theft) / cfg.fps;
	}

	/* Set the timer */
	p_ptr->tim_store = STORE_TURNOUT;

	/* Calculate the number of store maintainances since the last visit */
	maintain_num = (turn - st_ptr->last_visit) / (10L * cfg.store_turns);

	/* Maintain the store max. 10 times.
	   Note: this value could probably be reduced down to 4, with
	   current values for STORE_TURNOVER_DIV and max/min turnover;
	   changing it down from 10 might be required in order to balance
	   item rarities for scummers, with eye on scumming periods which
	   could go from 1-maintenance-period up to this max value: It
	   shouldn't be too easy for the 1-period-scummer, nor too hard
	   for the max-period-scummer. The higher maintain_num's cap is,
	   the bigger the difference in scumming difficulty for rare items. - C. Blue */
	if (maintain_num > MAX_MAINTENANCES) maintain_num = MAX_MAINTENANCES;

	if (maintain_num) {
		/* Maintain the store */
		for (i = 0; i < maintain_num; i++) {
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

	/* Display the store before the blacklist or watchlist messages
	 * (requires 441a) - this was Sav's request - mikaelh */
	if (is_newer_than(&p_ptr->version, 4, 4, 1, 1, 0, 0)) {
		/* Display the store */
		display_store(Ind);
	}

	/* Temple cures some maladies and gives some bread if starving ;-o */
	if (!p_ptr->tim_blacklist && which == STORE_TEMPLE && !p_ptr->suscep_life) {
		if (p_ptr->food < PY_FOOD_ALERT) {
			msg_print(Ind, "The temple priest hands you a slice of bread.");
			set_food(Ind, (PY_FOOD_FULL - PY_FOOD_ALERT) / 2);
		}

		if (p_ptr->blind || p_ptr->confused) msg_print(Ind, "The temple priest cures you.");
		if (p_ptr->blind) set_blind(Ind, 0);
		if (p_ptr->confused) set_confused(Ind, 0);

		if (p_ptr->chp < p_ptr->mhp / 2) {
			msg_print(Ind, "The temple priest applies a bandage.");
			hp_player_quiet(Ind, p_ptr->mhp / 2, TRUE);
		}
	}

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
	else if (p_ptr->tim_watchlist)
		msg_print(Ind, "The owner keeps a sharp eye on you.");
	else if (st_ptr->tim_watch)
		msg_print(Ind, "The owner seems especially attentive right now.");

	if (!is_newer_than(&p_ptr->version, 4, 4, 1, 1, 0, 0)) {
		/* Display the store */
		display_store(Ind);
	}

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
	if (which == STORE_HOME) return;
#endif

	/* Make sure no one is in the store */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this player */
		j = gettown(i);
		if (j != -1) {
			if (st_ptr == &town[j].townstore[Players[i]->store_num])
				return;
		}
		if (j == -1) {	//DUNGEON STORES
			if (st_ptr == &town[gettown_dun(i)].townstore[Players[i]->store_num])
				return;
		}
	}

	/* Pick a new owner */
	for (j = st_ptr->owner; j == st_ptr->owner; ) {
//		st_ptr->owner = rand_int(MAX_STORE_OWNERS);
		st_ptr->owner = st_info[st_ptr->st_idx].owners[rand_int(MAX_STORE_OWNERS)];
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
			o_ptr->discount = 15; /* 'on sale' */
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
	if (which == STORE_HOME) return;
#endif

#if 0 /* disabled, since we have gettown_dun() now which may use ALL towns for dungeon stores */
	 /* HACK: Ignore non-occuring stores (dungeon stores outside of hack-town-index '0' - C. Blue */
	switch (st_ptr->st_idx) {
	/* unused stores that would just waste cpu time: */
	case 50:
		return;
	/* non-town stores (which are borrowd from town #0 exclusively): */
	case 52: case 53: case 55:
	case 61: case 62: case 63: case 64: case 65:
	case 42: case 45: case 60:
		if (st_ptr->town != 0) return; else break;
	/* stores that don't occur in every town */
	case 57: case 58: if (town[st_ptr->town].type != 1) return; else break;//bree
	case 59: if (town[st_ptr->town].type != 5) return; else break;//khazad
	case 48: if (town[st_ptr->town].type != 3) return; else break;//minas anor - but it's town 0, type 1?!
	}
#else
	/* stores that don't occur in every town */
	switch (st_ptr->st_idx) {
	case 57: case 58: if (town[st_ptr->town].type != 1) return; else break;//bree
	case 59: if (town[st_ptr->town].type != 5) return; else break;//khazad
	case 48: if (town[st_ptr->town].type != 3) return; else break;//minas anor - but it's town 0, type 1?!
	}
#endif

	/* Ignore Museum */
	if (st_info[st_ptr->st_idx].flags1 & SF1_MUSEUM) return;

	/* Make sure no one is in the store */
#if 0 /* not used (cf.st_ptr->last_visit) */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this player */
		j = gettown(i);
		if (j != -1) {
			if (st_ptr == &town[j].townstore[Players[i]->store_num]) {
				/* Somewhat hackie -- don't turn her/him out at once */
				if (magik(40) && Players[i]->tim_store < STORE_TURNOUT / 2)
					store_kick(i, TRUE);
				else return;
			}
		}
		if (j == -1) {	//DUNGEON STORES
			j = gettown_dun(i);
			if (st_ptr == &town[j].townstore[Players[i]->store_num]) {
				if (magik(40) && Players[i]->tim_store < STORE_TURNOUT / 2)
					store_kick(i, TRUE);
				else return;
			}
	}
#endif	// 0

	/* Activate the owner */
//	ot_ptr = &owners[st_ptr->st_idx][st_ptr->owner];
	ot_ptr = &ow_info[st_ptr->owner];

	/* Store keeper forgives the player */
	st_ptr->insult_cur = 0;

	/* Mega-Hack -- prune the black market */
//	if (st_ptr->st_idx == 6)
	if (st_info[st_ptr->st_idx].flags1 & SF1_ALL_ITEM) {
		/* Destroy crappy black market items */
		for (j = st_ptr->stock_num - 1; j >= 0; j--) {
			object_type *o_ptr = &st_ptr->stock[j];

			/* Destroy crappy items */
			if (black_market_crap(o_ptr)) {
				/* Destroy the item */
				store_item_increase(st_ptr, j, 0 - o_ptr->number);
				store_item_optimize(st_ptr, j);
			}
		}
	}


	/* Choose the number of slots to keep */
	j = st_ptr->stock_num;

	/* Sell a few items */
	j = j - randint(1 + st_ptr->stock_size / STORE_TURNOVER_DIV);

#if 0 /* making it dependant on shop size instead - C. Blue */
	/* Never keep more than "STORE_MAX_KEEP" slots */
	if (j > STORE_MAX_KEEP) j = STORE_MAX_KEEP;
#else
	if (j > (st_ptr->stock_size * 7) / 8) j = (st_ptr->stock_size * 7) / 8;
#endif

#if 0 /* making it dependant on shop size instead - C. Blue */
	/* Always "keep" at least "STORE_MIN_KEEP" items */
	if (j < STORE_MIN_KEEP) j = STORE_MIN_KEEP;
#else
	if (j < st_ptr->stock_size / 4) j = st_ptr->stock_size / 4;
#endif
	/* Hack -- prevent "underflow" */
	if (j < 0) j = 0;

	/* Destroy objects until only "j" slots are left */
	while (st_ptr->stock_num > j) store_delete(st_ptr);


	/* Choose the number of slots to fill */
	j = st_ptr->stock_num;

	/* Buy some more items */
	j = j + randint(1 + st_ptr->stock_size / STORE_TURNOVER_DIV);

#if 0 /* making it dependant on shop size instead - C. Blue */
	/* Never keep more than "STORE_MAX_KEEP" slots */
	if (j > STORE_MAX_KEEP) j = STORE_MAX_KEEP;
#else
	if (j > (st_ptr->stock_size * 7) / 8) j = (st_ptr->stock_size * 7) / 8;
#endif

#if 0 /* making it dependant on shop size instead - C. Blue */
	/* Always "keep" at least "STORE_MIN_KEEP" items */
	if (j < STORE_MIN_KEEP) j = STORE_MIN_KEEP;
#else
	if (j < st_ptr->stock_size / 4) j = st_ptr->stock_size / 4;
#endif

	/* Hack -- prevent "overflow" */
	if (j >= st_ptr->stock_size) j = st_ptr->stock_size - 1;

	/* Hack for Libraries: Keep especially many books, which fluctuate often.
	   Reason: It's probably a bit too annoying to wait for specific books. - C. Blue */
	if (st_ptr->st_idx == STORE_BOOK || st_ptr->st_idx == STORE_HIDDENLIBRARY ||
	    st_ptr->st_idx == STORE_LIBRARY || st_ptr->st_idx == STORE_FORBIDDENLIBRARY)
		j = st_ptr->stock_size - rand_int((st_ptr->stock_size * 3) / 8);

	/* Acquire some new items */
	/* We want speed & healing & mana pots in the BM */
	while (st_ptr->stock_num < j) {
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
//	st_ptr->owner = rand_int(MAX_STORE_OWNERS);
	st_ptr->owner = st_info[st_ptr->st_idx].owners[rand_int(MAX_STORE_OWNERS)];

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
//	st_ptr->last_visit = -100L * cfg.store_turns;

	/* Clear any old items */
	for (k = 0; k < st_ptr->stock_size; k++)
		invwipe(&st_ptr->stock[k]);
}

		
void store_kick(int Ind, bool say)
{
	player_type *p_ptr = Players[Ind];
	if (say) msg_print(Ind, "The shopkeeper asks you to leave the store once.");
	//				store_leave(Ind);
	handle_store_leave(Ind);
	Send_store_kick(Ind);
#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2) {
		/* unlock the fake store again which we had occupied */
		fake_store_visited[-2 - p_ptr->store_num] = 0;
	} else
#endif
	teleport_player_force(Ind, 1);
}

void store_exec_command(int Ind, int action, int item, int item2, int amt, int gold)
{
	player_type *p_ptr = Players[Ind];
	store_type *st_ptr;
	int i;

	/* MEGAHACK -- accept house extension command */
	if (p_ptr->store_num == 7) {
		if (ba_info[action].action == BACT_EXTEND_HOUSE) home_extend(Ind);
		return;
	}

	i = gettown(Ind);
	/* hack: non-town stores (ie dungeon, but could also be wild) are borrowed from town #0 - C. Blue */
	if (i == -1) i = gettown_dun(Ind);

	/* sanity check - Yakina - */
	if (p_ptr->store_num == -1) {
		msg_print(Ind,"You left the building!");
		return;
	}

#ifdef PLAYER_STORES
	if (p_ptr->store_num <= -2) /* it's a player's private store! */
		st_ptr = &fake_store[-2 - p_ptr->store_num];
	else
#endif
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

#if 0	/* Sanity checks - not possible since item could be
	   either from player or from store inventory - C. Blue */
	if (item < 0 || item >= st_ptr->stock_size) return;
	if (!st_ptr->stock[item].tval) return;
#endif

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

/* Added this to circumvent annoying stacking behaviour of party houses - C. Blue
   Note: j_ptr must be the house item, o_ptr the item to drop into the house! */
static int home_object_similar(int Ind, object_type *j_ptr, object_type *o_ptr, s16b tolerance) {
	player_type *p_ptr = NULL;
	int total = o_ptr->number + j_ptr->number;
	int qlev = 100;
	if (o_ptr->owner) qlev = lookup_player_level(j_ptr->owner);

/* Hack -- gold always merge */
//	if (o_ptr->tval == TV_GOLD && j_ptr->tval == TV_GOLD) return(TRUE);

	/* Require identical object types */
	if (o_ptr->k_idx != j_ptr->k_idx) return (FALSE);

	/* Level 0 items and other items won't merge, since level 0 can't be sold to shops */
	if (!(tolerance & 0x2) &&
	    (!o_ptr->level || !j_ptr->level) &&
	    (o_ptr->level != j_ptr->level))
		return (FALSE);

	/* Require same owner or convertable to same owner */
//
/*		if (o_ptr->owner != j_ptr->owner) return (FALSE); */
	if (Ind) {
		p_ptr = Players[Ind];
		if (((o_ptr->owner != j_ptr->owner)
		    && ((qlev < j_ptr->level)
		    || (j_ptr->level < 1)))
		    && (j_ptr->owner)) return (FALSE);
#if 0
		if ((o_ptr->owner != p_ptr->id)
		    && (o_ptr->owner != j_ptr->owner)) return (FALSE);
#endif

		/* Require objects from the same modus! */
		/* A non-everlasting player won't have his items stacked w/ everlasting stuff */
		if (compat_pomode(Ind, j_ptr)) return(FALSE);
	} else {
		if (o_ptr->owner != j_ptr->owner) return (FALSE);
		/* no stacks of unowned everlasting items in shops after a now-dead
		   everlasting player sold an item to the shop before he died :) */
		if (compat_omode(o_ptr, j_ptr)) return(FALSE);
	}

	/* Analyze the items */
	switch (o_ptr->tval) {
	/* Chests */
	case TV_KEY:
	case TV_CHEST:
		/* Never okay */
		return (FALSE);

	/* Food and Potions and Scrolls */
	case TV_SCROLL:
		/* cheques may have different value, so they must not stack */
		if (o_ptr->sval == SV_SCROLL_CHEQUE) return FALSE;
	case TV_FOOD:
	case TV_POTION:
	case TV_POTION2:
		/* Hack for ego foods :) */
		if (o_ptr->name2 != j_ptr->name2) return (FALSE);
		if (o_ptr->name2b != j_ptr->name2b) return (FALSE);

		/* Assume okay */
		break;

	/* Staffs and Wands */
	case TV_WAND:
		/* Require either knowledge or known empty for both wands. */
		if ((!(o_ptr->ident & (ID_EMPTY)) &&
		    !object_known_p(Ind, o_ptr)) ||
		    (!(j_ptr->ident & (ID_EMPTY)) &&
		    !object_known_p(Ind, j_ptr))) return(FALSE);

		/* Beware artifatcs should not combine with "lesser" thing */
		if (o_ptr->name1 != j_ptr->name1) return (FALSE);

		/* Do not combine recharged ones with non recharged ones. */
//		if ((f4 & TR4_RECHARGED) != (f14 & TR4_RECHARGED)) return (FALSE);

		/* Do not combine different ego or normal ones */
		if (o_ptr->name2 != j_ptr->name2) return (FALSE);

		/* Do not combine different ego or normal ones */
		if (o_ptr->name2b != j_ptr->name2b) return (FALSE);

		/* Assume okay */
		break;

	case TV_STAFF:
		/* Require knowledge */
		if (!Ind || !object_known_p(Ind, o_ptr) || !object_known_p(Ind, j_ptr)) return (FALSE);

		if (!Ind || !p_ptr->stack_allow_wands) return (FALSE);

		/* Require identical charges */
		if (o_ptr->pval != j_ptr->pval) return (FALSE);

		if (o_ptr->name2 != j_ptr->name2) return (FALSE);
		if (o_ptr->name2b != j_ptr->name2b) return (FALSE);

		/* Probably okay */
		break;

		/* Fall through */
		/* NO MORE FALLING THROUGH! MUHAHAHA the_sandman */

	/* Staffs and Wands and Rods */
	case TV_ROD:
		/* Overpoweredness, Hello! - the_sandman */
		if (o_ptr->sval == SV_ROD_HAVOC) return (FALSE);

		/* Require permission */
		if (!Ind || !p_ptr->stack_allow_wands) return (FALSE);

		/* this is only for rods... the_sandman */
		if (o_ptr->pval == 0 && j_ptr->pval != 0) return (FALSE); //lol :)

		if (o_ptr->name2 != j_ptr->name2) return (FALSE);
		if (o_ptr->name2b != j_ptr->name2b) return (FALSE);

		/* Probably okay */
		break;

	/* Weapons and Armor */
	case TV_DRAG_ARMOR:	return(FALSE);
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
	case TV_TRAPKIT: /* so they don't stack carelessly - the_sandman */
		/* Require permission */
		if (!Ind || !p_ptr->stack_allow_items) return (FALSE);

		/* XXX XXX XXX Require identical "sense" status */
		/* if ((o_ptr->ident & ID_SENSE) != */
		/*     (j_ptr->ident & ID_SENSE)) return (FALSE); */

		/* Costumes must be for same monster */
		if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_COSTUME)) {
			if (o_ptr->bpval != j_ptr->bpval) return(FALSE);
		}

		/* Fall through */

	/* Rings, Amulets, Lites */
	case TV_RING:
		/* no more, due to their 'timeout' ! */
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)) return (FALSE);
	case TV_AMULET:
	case TV_LITE:
	case TV_TOOL:
	case TV_BOOK:	/* Books can be 'fireproof' */
#if 0 /* custom tomes which aren't blank can't be stacked */
		/* hack: 'used' custom tomes can't be stacked */
		if (o_ptr->tval == TV_BOOK &&
		    o_ptr->sval >= SV_CUSTOM_TOME_1 &&
		    o_ptr->sval < SV_SPELLBOOK &&
		    o_ptr->xtra1) /* not 'empty' anymore, ie already written into? */
			return(FALSE);
		if (j_ptr->tval == TV_BOOK &&
		    j_ptr->sval >= SV_CUSTOM_TOME_1 &&
		    j_ptr->sval < SV_SPELLBOOK &&
		    j_ptr->xtra1) /* not 'empty' anymore, ie already written into? */
			return(FALSE);
#else /* custom tomes which appear identical, spell-wise too, may stack */
		if (o_ptr->tval == TV_BOOK &&
		    o_ptr->sval >= SV_CUSTOM_TOME_1 &&
		    o_ptr->sval < SV_SPELLBOOK &&
		    ((o_ptr->xtra1 != j_ptr->xtra1) ||
		    (o_ptr->xtra2 != j_ptr->xtra2) ||
		    (o_ptr->xtra3 != j_ptr->xtra3) ||
		    (o_ptr->xtra4 != j_ptr->xtra4) ||
		    (o_ptr->xtra5 != j_ptr->xtra5) ||
		    (o_ptr->xtra6 != j_ptr->xtra6) ||
		    (o_ptr->xtra7 != j_ptr->xtra7) ||
		    (o_ptr->xtra8 != j_ptr->xtra8) ||
		    (o_ptr->xtra9 != j_ptr->xtra9)))
			return(FALSE);
#endif
		/* Require full knowledge of both items */
//			if (o_ptr->tval == TV_BOOK) {
		if (!Ind || !object_known_p(Ind, o_ptr) ||
//			    !object_known_p(Ind, j_ptr) || (o_ptr->name3)) return (FALSE);
		    !object_known_p(Ind, j_ptr))
			return (FALSE);
//			}

		/* different bpval? */
		if (o_ptr->bpval != j_ptr->bpval) return(FALSE);

		/* Fall through */

	/* Missiles */
	case TV_BOLT:
	case TV_ARROW:
	case TV_SHOT:
		/* Require identical "bonuses" -
		except for ammunition which carries special inscription (will merge!) - C. Blue */
		if (!((tolerance & 0x1) && !(cursed_p(o_ptr) || cursed_p(j_ptr) ||
		    artifact_p(o_ptr) || artifact_p(j_ptr))) ||
		    (!is_ammo(o_ptr->tval) ||
		    (!check_guard_inscription(o_ptr->note, 'M') && !check_guard_inscription(j_ptr->note, 'M')))) {
			if (o_ptr->to_h != j_ptr->to_h) return (FALSE);
			if (o_ptr->to_d != j_ptr->to_d) return (FALSE);
		}
		if (o_ptr->to_a != j_ptr->to_a) return (FALSE);

		/* Require identical "pval" code */
		if (o_ptr->pval != j_ptr->pval) return (FALSE);

		/* Require identical "artifact" names <- this shouldnt happen right? */
		if (o_ptr->name1 != j_ptr->name1) return (FALSE);

		/* Require identical "ego-item" names.
		Allow swapped ego powers: Ie Arrow (SlayDragon,Ethereal) combines with Arrow (Ethereal,SlayDragon).
		Note: This code assumes there's no ego power which can be both prefix and postfix. */
#if 0
		/* This is buggy, it allows stacking of normal items with items that only have one ego power - mikaelh */
		if ((o_ptr->name2 != j_ptr->name2) && (o_ptr->name2 != j_ptr->name2b)) return (FALSE);
		if ((o_ptr->name2b != j_ptr->name2) && (o_ptr->name2b != j_ptr->name2b)) return (FALSE);
#else
		/* This one only allows name2 and name2b to be swapped */
		if (! ((o_ptr->name2 == j_ptr->name2b) && (o_ptr->name2b == j_ptr->name2))) {
			if (o_ptr->name2 != j_ptr->name2) return (FALSE);
			if (o_ptr->name2b != j_ptr->name2b) return (FALSE);
		}
#endif

		/* Require identical random seeds */
		if (o_ptr->name3 != j_ptr->name3) return (FALSE);

		/* Hack -- Never stack "powerful" items */
//			if (o_ptr->xtra1 || j_ptr->xtra1) return (FALSE);

		/* Hack -- Never stack recharging items */
		if (o_ptr->timeout != j_ptr->timeout) return (FALSE);
#if 0
		if (o_ptr->timeout || j_ptr->timeout) {
			if ((o_ptr->timeout != j_ptr->timeout) ||
			(o_ptr->tval != TV_LITE)) return (FALSE);
		}
#endif	// 0

		/* Require identical "values" */
		if (o_ptr->ac != j_ptr->ac) return (FALSE);
		if (o_ptr->dd != j_ptr->dd) return (FALSE);
		if (o_ptr->ds != j_ptr->ds) return (FALSE);

		/* Probably okay */
		break;

	case TV_GOLEM:
		if (o_ptr->pval != j_ptr->pval) return(FALSE);
		break;

	/* Various */
	default:
		/* Require knowledge */
		if (Ind && (!object_known_p(Ind, o_ptr) ||
		    !object_known_p(Ind, j_ptr))) return (FALSE);

		/* Probably okay */
		break;
	}


	/* Hack -- Require identical "cursed" status */
	if ((o_ptr->ident & ID_CURSED) != (j_ptr->ident & ID_CURSED)) return (FALSE);

	/* Hack -- Require identical "broken" status */
	if ((o_ptr->ident & ID_BROKEN) != (j_ptr->ident & ID_BROKEN)) return (FALSE);


	/* Hack -- require semi-matching "inscriptions" */
	/* Hack^2 -- books do merge.. it's to prevent some crashes */
	if (o_ptr->note && j_ptr->note && (o_ptr->note != j_ptr->note)
	    && strcmp(quark_str(o_ptr->note), "on sale")
	    && strcmp(quark_str(j_ptr->note), "on sale")
	    && !is_realm_book(o_ptr)
	    && !check_guard_inscription(o_ptr->note, 'M')
	    && !check_guard_inscription(j_ptr->note, 'M'))
		return (FALSE);

	/* Hack -- normally require matching "inscriptions" */
	if ((!Ind || !p_ptr->stack_force_notes) && (o_ptr->note != j_ptr->note)) return (FALSE);

	/* Hack -- normally require matching "discounts" */
	if ((!Ind || !p_ptr->stack_force_costs) && (o_ptr->discount != j_ptr->discount)) return (FALSE);

	/* Maximal "stacking" limit */
	if (total >= MAX_STACK_SIZE) return (FALSE);

	/* An everlasting player will have _his_ items stack w/ non-everlasting stuff
	(especially new items bought in the shops) and convert them all to everlasting */
	if (Ind && (p_ptr->mode & MODE_EVERLASTING)) {
		o_ptr->mode = MODE_EVERLASTING;
		j_ptr->mode = MODE_EVERLASTING;
	}

	/* A PvP-player will get his items convert to pvp-mode */
	if (Ind && (p_ptr->mode & MODE_PVP)) {
		o_ptr->mode = MODE_PVP;
		j_ptr->mode = MODE_PVP;
	}

	/* They match, so they must be similar */
	return (TRUE);
}

static int home_carry(int Ind, house_type *h_ptr, object_type *o_ptr)
{
	int slot;
	s64b value, j_value;
	int i;
	object_type *j_ptr;


	/* Check each existing item (try to combine) */
	for (slot = 0; slot < h_ptr->stock_num; slot++) {
		/* Get the existing item */
		j_ptr = &h_ptr->stock[slot];

		/* The home acts just like the player */
		if (home_object_similar(Ind, j_ptr, o_ptr, 0x0)) {
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
	for (slot = 0; slot < h_ptr->stock_num; slot++) {
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
		if (o_ptr->tval == TV_ROD_MAIN) {
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
		h_ptr->stock[i] = h_ptr->stock[i-1];

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
		h_ptr->stock[j] = h_ptr->stock[j + 1];

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
		for (i = 0; i < h_ptr->stock_num; i++) {
			/* Get the existing item */
			j_ptr = &h_ptr->stock[i];

			/* Can the new object be combined with the old one? */
			if (home_object_similar(Ind, j_ptr, o_ptr, 0x0)) return (TRUE);
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
	char			o_name[ONAME_LEN];
	house_type		*h_ptr;

	/* This should never happen */
	if (p_ptr->store_num != 7) {
		msg_print(Ind,"You left the house!");
		return;
	}

	h_idx = pick_house(&p_ptr->wpos, p_ptr->py, p_ptr->px);
	if (h_idx == -1) return;

	h_ptr = &houses[h_idx];


	/* You can't sell 0 of something. */
	if (amt <= 0) return;

	/* Get the item (in the pack) */
	if (item >= 0) {
		o_ptr = &p_ptr->inventory[item];
	}
	/* Get the item (on the floor) */
	else { /* Never.. */
		o_ptr = &o_list[0 - item];
	}

#if 1

	/* Not gonna happen XXX inscribe */
	/* Nah, gonna happen */
	/* TODO: CURSE_NO_DROP */
	if (cursed_p(o_ptr) && !is_admin(p_ptr)) {
		u32b f1, f2, f3, f4, f5, esp;
		if (item >= INVEN_WIELD) {
			msg_print(Ind, "Hmmm, it seems to be cursed.");
			return;
		}

		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

		if (f4 & TR4_CURSE_NO_DROP) {
			msg_print(Ind, "Hmmm, you seem to be unable to drop it.");
			return;
		}
	}

#endif

	if (cfg.anti_arts_house && undepositable_artifact_p(o_ptr)) {
		msg_print(Ind, "You cannot stock this artifact.");
		return;
	}

	/* Create the object to be sold (structure copy) */
	sold_obj = *o_ptr;
	sold_obj.number = amt;

	/* Get a full description */
	object_desc(Ind, o_name, &sold_obj, TRUE, 3);

	/* Is there room in the store (or the home?) */
	if (!home_check_num(Ind, h_ptr, &sold_obj)) {
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
	if (o_ptr->tval == TV_WAND) sold_obj.pval = divide_charged_item(o_ptr, amt);

	/* Get the description all over again */
	object_desc(Ind, o_name, &sold_obj, TRUE, 3);

	/* Describe the result (in message buffer) */
//		msg_format("You drop %s (%c).", o_name, index_to_label(item));
	msg_format(Ind, "You drop %s.", o_name);
#ifdef USE_SOUND_2010
	sound_item(Ind, o_ptr->tval, o_ptr->sval, "drop_");
#endif

	/* Analyze the prices (and comment verbally) */
	/*purchase_analyze(price, value, dummy);*/

	/* Take the item from the player, describe the result */
	inven_item_increase(Ind, item, -amt);
	inven_item_describe(Ind, item);
	inven_item_optimize(Ind, item);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Artifact won't be deposited in your home */
	if (undepositable_artifact_p(&sold_obj) &&
	    (cfg.anti_arts_house || (p_ptr->total_winner && !winner_artifact_p(o_ptr)))) {
		handle_art_d(sold_obj.name1);
		return;
	}

	/* The store gets that (known) item */
//	if(sold_obj.tval!=8)	// What was it for.. ?
	item_pos = home_carry(Ind, h_ptr, &sold_obj);
//		item_pos = store_carry(p_ptr->store_num, &sold_obj);

	/* Resend the basic store info */
	display_trad_house(Ind, h_ptr);

	/* Re-display if item is now in store */
	if (item_pos >= 0) display_house_inventory(Ind, h_ptr);
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

	char		o_name[ONAME_LEN];

	house_type *h_ptr;

	if (amt < 1) {
		s_printf("$INTRUSION(HOME)$ Bad amount %d! (Home) Bought by %s.", amt, p_ptr->name);
//		msg_print(Ind, "\377RInvalid amount. Your attempt has been logged.");
		return;
	}

	if (p_ptr->store_num == -1) {
		msg_print(Ind,"You left the shop!");
		return;
	}

	/* This should never happen */
	if (p_ptr->store_num != 7) return;

	h_idx = pick_house(&p_ptr->wpos, p_ptr->py, p_ptr->px);
	if (h_idx == -1) return;

	h_ptr = &houses[h_idx];

	/* Empty? */
	if (h_ptr->stock_num <= 0) {
		msg_print(Ind, "Your home is empty.");
		return;
	}

	/* Sanity check - mikaelh */
	if (item < 0 || item >= h_ptr->stock_size)
		return;

	/* Get the actual item */
	o_ptr = &h_ptr->stock[item];

	/* Check that it's a real item - mikaelh */
	if (!o_ptr->tval) return;

	/* check whether client tries to buy more than the store has */
	if (o_ptr->number < amt) {
		s_printf("$INTRUSION(HOME)$ Too high amount %d of %d! Bought by %s.\n", amt, o_ptr->number, p_ptr->name);
//		msg_print(Ind, "\377RInvalid amount. Your attempt has been logged.");
		return;
	}

	if (compat_pomode(Ind, o_ptr)) {
		msg_format(Ind, "You cannot take items of %s players!", compat_pomode(Ind, o_ptr));
		return;
	}

#if 0 /* if a true art was really store in a house, this should probably stay allowed.. */
	/* prevent winners picking up true arts accidentally */
	if (true_artifact_p(o_ptr) && !winner_artifact_p(o_ptr) &&
	    p_ptr->total_winner && cfg.kings_etiquette) {
		msg_print(Ind, "Royalties may not own true artifacts!");
		if (!is_admin(p_ptr)) return;
	}
#endif

	/* Check if the player is powerful enough for that item */
	if ((o_ptr->owner) && (o_ptr->owner != p_ptr->id) &&
	    (o_ptr->level > p_ptr->lev || o_ptr->level == 0)) {
		if (cfg.anti_cheeze_pickup) {
			msg_print(Ind, "You aren't powerful enough yet to pick up that item!");
			return;
		}
		if (true_artifact_p(o_ptr) && cfg.anti_arts_pickup) {
			msg_print(Ind, "You aren't powerful enough yet to pick up that artifact!");
			return;
		}
	}

	if ((k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) && !p_ptr->once_winner
	    && !p_ptr->total_winner) { /* <- Obsolete. Added that one just for testing when admin char sets .total_winner=1 */
		msg_print(Ind, "Only royalties are powerful enough to pick up that item!");
		if (!is_admin(p_ptr)) return;
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
	if (o_ptr->tval == TV_WAND) {
		if (o_ptr->tval == TV_WAND) {
			sell_obj.pval = divide_charged_item(o_ptr, amt);
		}
	}

	/* Hack -- require room in pack */
	if (!inven_carry_okay(Ind, &sell_obj)) {
		msg_print(Ind, "You cannot carry that many different items.");
		return;
	}



	/* Home is much easier */
	{

#if CHEEZELOG_LEVEL > 2
		/* Take cheezelog
		 */
		if (p_ptr->id != o_ptr->owner && 
		   (o_ptr->tval != 1 && o_ptr->sval != 9) /* Heavy ball */ )
		{
			cptr 	name = lookup_player_name(o_ptr->owner);
			int 	lev = lookup_player_level(o_ptr->owner);
			cptr	acc_name = lookup_accountname(o_ptr->owner);
			object_desc_store(Ind, o_name, o_ptr, TRUE, 3);
			/* If level diff. is too large, target player is too low,
			   and items aren't loot of a dead player, this might be cheeze! */
 #if 0
			if ((lev > p_ptr->lev + 7) && (p_ptr->lev < 40) && (name)) {
			s_printf("%s -CHEEZY- Item transaction from %s(%d) to %s(%d) at (%d,%d,%d):\n  %s\n",
					showtime(), name ? name : "(Dead player)", lev,
					p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz,
					o_name);
			c_printf("%s ITEM %s(%d,%s) %s(%d,%s) %lld(%d%%) %s\n",
					showtime(), name ? name : "(---)", lev, acc_name,
					p_ptr->name, p_ptr->lev, p_ptr->accountname,
					object_value_real(0, o_ptr), o_ptr->discount, o_name);
			} else {
			s_printf("%s Item transaction from %s(%d) to %s(%d) at (%d,%d,%d):\n  %s\n",
					showtime(), name ? name : "(Dead player)", lev,
					p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz,
					o_name);
			c_printf("%s item %s(%d,%s) %s(%d,%s) %lld(%d%%) %s\n",
					showtime(), name ? name : "(---)", lev, acc_name,
					p_ptr->name, p_ptr->lev, p_ptr->accountname,
					object_value_real(0, o_ptr), o_ptr->discount, o_name);
			}
 #else
			s_printf("%s Item transaction from %s(%d) to %s(%d%s) at (%d,%d,%d):\n  %s\n",
					showtime(), name ? name : "(Dead player)", lev,
					p_ptr->name, p_ptr->lev, p_ptr->total_winner ? ",W" : (p_ptr->once_winner ? ",O" : ""),
					p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz,
					o_name);
			c_printf("%s ITEM %s(%d,%s) : %s(%d,%s%s) %lld(%d%%) : %s\n",
					showtime(), name ? name : "(---)", lev, acc_name,
					p_ptr->name, p_ptr->lev, p_ptr->accountname,
					p_ptr->total_winner ? ",W" : (p_ptr->once_winner ? ",O" : ""),
					object_value_real(0, o_ptr), o_ptr->discount, o_name);
 #endif
			/* Highlander Tournament: Don't allow transactions before it begins */
			if (!p_ptr->max_exp) gain_exp(Ind, 1);
		}
#endif  // CHEEZELOG_LEVEL

		/* Carry the item */
		can_use(Ind, &sell_obj);//##UNH correct?
		item_new = inven_carry(Ind, &sell_obj);

		/* Describe just the result */
		object_desc(Ind, o_name, &p_ptr->inventory[item_new], TRUE, 3);

		/* Message */
		msg_format(Ind, "You have %s (%c).", o_name, index_to_label(item_new));
#ifdef USE_SOUND_2010
		sound_item(Ind, sell_obj.tval, sell_obj.sval, "pickup_");
#endif

		/* Handle stuff */
		handle_stuff(Ind);

		/* Take note if we take the last one */
		i = h_ptr->stock_num;

		/* Remove the items from the home */
		home_item_increase(h_ptr, item, -amt);
		home_item_optimize(h_ptr, item);

		/* Resend the basic store info */
		display_trad_house(Ind, h_ptr);

		/* Hack -- Item is still here */
		if (i == h_ptr->stock_num && i) {
			/* Redraw the item */
			display_house_entry(Ind, item, h_ptr);
		}
		/* The item is gone */
		else {
			/* Redraw everything */
			display_house_inventory(Ind, h_ptr);
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
	object_type		*o_ptr;
	char		o_name[ONAME_LEN];
	house_type *h_ptr;

	/* This should never happen */
	if (p_ptr->store_num != 7) return;

	h_idx = pick_house(&p_ptr->wpos, p_ptr->py, p_ptr->px);
	if (h_idx == -1) return;

	h_ptr = &houses[h_idx];

	/* Empty? */
	if (h_ptr->stock_num <= 0) {
		msg_print(Ind, "Your home is empty.");
		return;
	}

	/* Sanity check - mikaelh */
	if (item < 0 || item >= h_ptr->stock_size)
		return;

	/* Get the actual item */
	o_ptr = &h_ptr->stock[item];

	/* Check that it's a real item - mikaelh */
	if (!o_ptr->tval) return;

	/* Description */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	/* Require full knowledge */
	if (!(o_ptr->ident & ID_MENTAL) && !is_admin(p_ptr)) observe_aux(Ind, o_ptr);
	/* Describe it fully */
	else if (!identify_fully_aux(Ind, o_ptr)) msg_print(Ind, "You see nothing special.");

#if 0
	if (o_ptr->tval < TV_BOOK) {
		if (!identify_fully_aux(Ind, o_ptr)) msg_print(Ind, "You see nothing special.");
	} else { /* Books are read */
		do_cmd_browse_aux(o_ptr);
	}
#endif

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
	if (h_ptr->stock_size >= STORE_INVEN_MAX) {
		msg_print(Ind, "Your house cannot be extended any more.");
		return;
	}

	cost = h_ptr->dna->price * 2 / (h_ptr->stock_size + 1);

	if (p_ptr->au < cost) {
		msg_print(Ind, "You couldn't afford it..");
		return;
	}

	GROW(h_ptr->stock, h_ptr->stock_size, h_ptr->stock_size + 1, object_type);
	h_ptr->stock_size++;
	p_ptr->au -= cost;
	h_ptr->dna->price += cost;

	msg_format(Ind, "You extend your house for %dau.", cost);

	display_trad_house(Ind, h_ptr);

	/* Display the current gold */
	store_prt_gold(Ind);
}


static void display_house_entry(int Ind, int pos, house_type *h_ptr)
{
	player_type *p_ptr = Players[Ind];
	object_type		*o_ptr;

	char		o_name[ONAME_LEN];
	byte		attr;
	int		wgt;

	int maxwid = 75;

	/* This should never happen */
	if (p_ptr->store_num != 7) return;


	/* Get the item */
	o_ptr = &h_ptr->stock[pos];

#if 0
	/* Get the "offset" */
	i = (pos % 12);

	/* Label it, clear the line --(-- */
	(void)snprintf(out_val, sizeof(out_val), "%c) ", I2A(i));
	prt(out_val, i+6, 0);
#endif


	/* Describe the object */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);
	o_name[maxwid] = '\0';

	attr = get_attr_from_tval(o_ptr);

	/* Get the proper book colour */
	if (o_ptr->tval == TV_BOOK) attr = get_book_name_color(Ind, o_ptr);

	/* Only show the weight of an individual item */
	wgt = o_ptr->weight;

	/* Send the info */
	if (is_newer_than(&p_ptr->version, 4, 4, 3, 0, 0, 4)) {
		if (o_ptr->tval != TV_BOOK || o_ptr->sval < SV_CUSTOM_TOME_1 || o_ptr->sval == SV_SPELLBOOK) {
			Send_store(Ind, pos, attr, wgt, o_ptr->number, 0, o_name, o_ptr->tval, o_ptr->sval, o_ptr->pval);
		} else {
			Send_store_wide(Ind, pos, attr, wgt, o_ptr->number, 0, o_name, o_ptr->tval, o_ptr->sval, o_ptr->pval,
			    o_ptr->xtra1, o_ptr->xtra2, o_ptr->xtra3, o_ptr->xtra4, o_ptr->xtra5, o_ptr->xtra6, o_ptr->xtra7, o_ptr->xtra8, o_ptr->xtra9);
		}
	} else {
		Send_store(Ind, pos, attr, wgt, o_ptr->number, 0, o_name, o_ptr->tval, o_ptr->sval, o_ptr->pval);
	}
}


/*
 * Displays a store's inventory			-RAK-
 * All prices are listed as "per individual object".  -BEN-
 *
 * The inventory is "sent" not "displayed". -KLJ-
 */
static void display_house_inventory(int Ind, house_type *h_ptr)
{
	player_type *p_ptr = Players[Ind];
	int k;

	/* This should never happen */
	if (p_ptr->store_num != 7) return;

	/* Display the next 48 items */
	for (k = 0; k < STORE_INVEN_MAX; k++) {
		/* Do not display "dead" items */
		/* But tell the client that it's empty now */
		if (k && k >= h_ptr->stock_num) break;

		/* Display that line */
		display_house_entry(Ind, k, h_ptr);
	}
}

/*
 * Displays store (after clearing screen)		-RAK-
 */
static void display_trad_house(int Ind, house_type *h_ptr)
{
	player_type *p_ptr = Players[Ind];

	/* This should never happen */
	if (p_ptr->store_num != 7) return;

	/* Send the house info */
	/* our own house */
	if (!h_ptr->dna->owner || h_ptr->dna->owner == p_ptr->id)
		Send_store_info(Ind, p_ptr->store_num, "Your House", "", h_ptr->stock_num, h_ptr->stock_size, TERM_L_UMBER, '+');
	/* someone else's house (can't happen at the moment) */
	else {
		char owner[40];
		/* get name of real owner of this house */
		strcpy(owner, lookup_player_name(h_ptr->dna->owner));
		if (owner[strlen(owner) - 1] == 's') strcat(owner, "' House");
		else strcat(owner, "'s House");

		Send_store_info(Ind, p_ptr->store_num, owner, "", h_ptr->stock_num, h_ptr->stock_size, TERM_L_UMBER, '+');
	}

	/* Display the current gold */
	store_prt_gold(Ind);

	/* Draw in the inventory */
	display_house_inventory(Ind, h_ptr);

	/* Hack -- Send the store actions info */
	/* XXX it's dirty hack -- the aim is to avoid
	 * hard-coded stuffs in the client */
	show_building(Ind, &town[0].townstore[7]);
}

/* Enter a house, and interact with it.	- Jir - */
void do_cmd_trad_house(int Ind)
{
	player_type *p_ptr = Players[Ind];

	cave_type *c_ptr;
	struct c_special *cs_ptr;
	house_type *h_ptr;
	int h_idx;

	/* Access the player grid */
	cave_type **zcave;
	if (!(zcave = getcave(&p_ptr->wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* Verify a store */
	if (c_ptr->feat == FEAT_HOME) {
		/* Check access like former move_player */
		if ((cs_ptr = GetCS(c_ptr, CS_DNADOOR))) /* orig house failure */
		{
			if (!access_door(Ind, cs_ptr->sc.ptr, TRUE) && !admin_p(Ind)) {
				msg_print(Ind, "\377oThe door is locked.");
				return;
			}
		}
		else return;
	}
	/* Open door == free access */
	else if (c_ptr->feat == FEAT_HOME_OPEN) {
		/* Check access like former move_player */
		if (!(cs_ptr = GetCS(c_ptr, CS_DNADOOR))) return;
	}
	/* Not a house */
	else return;

	/* Check if it's traditional house */
	h_idx = pick_house(&p_ptr->wpos, p_ptr->py, p_ptr->px);

	if (h_idx == -1) return;

	h_ptr = &houses[h_idx];
	if (!(h_ptr->flags & HF_TRAD)) return;

	if (p_ptr->inval) {
		msg_print(Ind, "You may not use a house. Ask an admin to validate your account.");
		return;
	}


	/* Save the store number */
	p_ptr->store_num = 7;

	/* Set the timer */
	/* XXX well, don't kick her out of her own house :) */
	p_ptr->tim_store = 30000;

	/* Display the store */
	display_trad_house(Ind, h_ptr);

	/* Do not leave */
	leave_store = FALSE;
}


#endif	// USE_MANG_HOUSE_ONLY

/* Mayor's office: View list of top cheezers */
void view_cheeze_list(int Ind) {
	char    path[MAX_PATH_LENGTH];

//	cptr name = "cheeze.log";
//	(void)do_cmd_help_aux(Ind, name, NULL, line, FALSE);

	path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "cheeze-pub.log");
	do_cmd_check_other_prepare(Ind, path);
}

void reward_deed_item(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];
	object_type forge, *o_ptr = &forge;
	object_type *o2_ptr = &p_ptr->inventory[item];
	int lev = 0, dis = 100;

	WIPE(o_ptr, object_type);

	if (o2_ptr->tval != TV_PARCHMENT || o2_ptr->sval < SV_DEED_HIGHLANDER) {
		msg_print(Ind, "That's not a deed.");
		return;
	}
//	if (!o2_ptr->level && o2_ptr->owner != p_ptr->id) {
	if (o2_ptr->owner != p_ptr->id) {
		msg_print(Ind, "\377oYou can only redeem your own deeds for items.");
		return;
	}

	switch (o2_ptr->sval) {
	case SV_DEED_HIGHLANDER: /* winner's deed */
		create_reward(Ind, o_ptr, 95, 95, TRUE, TRUE, RESF_LOW2, 3000); /* 95 is default depth for highlander tournament */
		o_ptr->note = quark_add("Highlander reward");
		msg_print(Ind, "\377GThe mayor's secretary hands you a reward, while everyone applaudes!");
		msg_print_near(Ind, "You hear some applause coming out of the mayor's office!");
		break;
	case SV_DEED2_HIGHLANDER: /* participant's deed */
		msg_print(Ind, "\377yAfter examining the deed, the mayor tells you that they don't have any");
		msg_print(Ind, "\377yitems for rewards, but he suggests that you get a blessing instead!");
		return;
	case SV_DEED_PVP_MAX:
		create_reward(Ind, o_ptr, 95, 95, TRUE, TRUE, RESF_HIGH, 3000);
		o_ptr->note = quark_add("PvP reward");
		msg_print(Ind, "\377GThe mayor's secretary hands you a reward, while everyone applaudes!");
		msg_print_near(Ind, "You hear some applause coming out of the mayor's office!");
		dis = 0;
		break;
	case SV_DEED_PVP_MID:
		create_reward(Ind, o_ptr, 95, 95, TRUE, TRUE, RESF_MID, 3000);
		o_ptr->note = quark_add("PvP reward");
		msg_print(Ind, "\377GThe mayor's secretary hands you a reward, while everyone applaudes!");
		msg_print_near(Ind, "You hear some applause coming out of the mayor's office!");
		dis = 0;
		break;
	case SV_DEED_PVP_MASS:
		create_reward(Ind, o_ptr, 95, 95, TRUE, TRUE, RESF_MID, 3000);
		o_ptr->note = quark_add("PvP reward");
		msg_print(Ind, "\377GThe mayor's secretary hands you a reward, while everyone applaudes!");
		msg_print_near(Ind, "You hear some applause coming out of the mayor's office!");
		dis = 0;
		break;
	case SV_DEED_PVP_START:
		create_reward(Ind, o_ptr, 95, 95, TRUE, TRUE, RESF_LOW2, 3000);
		o_ptr->note = quark_add("");
		msg_print(Ind, "\377GThe mayor's secretary hands you an item and gives you a supportive pat.");
		lev = 1; dis = 0;
		break;
	default:
		msg_print(Ind, "\377oAfter examining the deed, the mayor tells you that they don't have any");
		msg_print(Ind, "\377oappropriate rewards. With a sorry gesture, he returns the deed to you.");
		return;
	}

	/* Take the item from the player, describe the result */
	inven_item_increase(Ind, item, -1);
	inven_item_describe(Ind, item);
	inven_item_optimize(Ind, item);

	/* finalize reward */
	object_aware(Ind, o_ptr);
	object_known(o_ptr);
	o_ptr->discount = dis;
	o_ptr->level = lev;
	o_ptr->ident |= ID_MENTAL;
	/* give him the reward item after taking the deed(!) */
	if (o_ptr->k_idx) inven_carry(Ind, o_ptr);

        /* Handle stuff */
	handle_stuff(Ind);
}

void reward_deed_blessing(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];
	object_type *o2_ptr = &p_ptr->inventory[item];

	if (o2_ptr->tval != TV_PARCHMENT || o2_ptr->sval < SV_DEED_HIGHLANDER) {
		msg_print(Ind, "That's not a deed.");
		return;
	}
//	if (!o2_ptr->level && o2_ptr->owner != p_ptr->id) {
	if (o2_ptr->owner != p_ptr->id) {
		switch (o2_ptr->sval) {
		/* all contender's deeds: */
		case SV_DEED2_HIGHLANDER:
			msg_print(Ind, "\377oYou can only redeem your own contender's deeds.");
			return;
		/* not a contender's deed, but a winner's deed! */
		default: 
			msg_print(Ind, "\377yThe mayor frowns at the deed, but accepts it.");
		}
	}

	switch (o2_ptr->sval) {
	case SV_DEED_HIGHLANDER: /* winner's deed */
		msg_print(Ind, "\377GThe town priest speaks a blessing, while everyone applaudes!");
		msg_print_near(Ind, "You hear some applause coming out of the mayor's office!");
		/* it's inaccurate, due to hack-like process_player_end_aux call timing
		   (about once every 31 turns on wz=0), but who cares :) */
#ifdef RPG_SERVER /* longer duration since dungeons are all ironman; also you can hardly trade parchments on RPG */
		bless_temp_luck(Ind, 4, (30 * 60 * cfg.fps) / 31); /* somewhere around 30 minutes */
#else
		bless_temp_luck(Ind, 4, (20 * 60 * cfg.fps) / 31); /* somewhere around 20 minutes */
#endif
		break;
	case SV_DEED2_HIGHLANDER: /* participant's deed */
		msg_print(Ind, "\377GThe town priest speaks a blessing.");
#ifdef RPG_SERVER /* longer duration since dungeons are all ironman; also you can hardly trade parchments on RPG */
		bless_temp_luck(Ind, 1, (30 * 60 * cfg.fps) / 31); /* somewhere around 30 minutes */
#else
		bless_temp_luck(Ind, 1, (20 * 60 * cfg.fps) / 31); /* somewhere around 20 minutes */
#endif
		break;
	default:
		msg_print(Ind, "\377oAfter examining the deed, the mayor tells you that they don't have any");
		msg_print(Ind, "\377oappropriate rewards. With a sorry gesture, he returns the deed to you.");
		return;
	}

	/* Take the item from the player, describe the result */
	inven_item_increase(Ind, item, -1);
	inven_item_describe(Ind, item);
	inven_item_optimize(Ind, item);

	/* Handle stuff */
	handle_stuff(Ind);
}

/* Set store_debugging_mode and put stores into a special mode that helps debugging their stocks - C. Blue
   note: may be called every 10 turns (since cfg.store_turns is multiplied by 10). */
void store_debug_stock()
{
	int i, j, n, maintain_num, what;
	store_type *st_ptr;
	object_type *o_ptr;

	/* process all stores in all towns */

	for (i = 0; i < numtowns; i++)
	for (n = 0; n < max_st_idx; n++) {

		/* process each store */
		st_ptr = &town[i].townstore[n];

		/* hack: fix old stores' <town> value, which would still be zero otherwise */
		st_ptr->town = i;

#if 1		/* HACK: Ignore non-occuring stores (dungeon stores outside of hack-town-index '0' - C. Blue */
		switch (st_ptr->st_idx) {
		/* unused stores that would just waste cpu time: */
		case 50:
			continue;
		/* non-town stores (which are borrowd from town #0 exclusively): */
		case 52: case 53: case 55:
		case 61: case 62: case 63: case 64: case 65:
		case 42: case 45: case 60:
			if (st_ptr->town != 0) continue; else break;
		/* stores that don't occur in every town */
		case 57: case 58: if (town[st_ptr->town].type != 1) continue; else break;//bree
		case 59: if (town[st_ptr->town].type != 5) continue; else break;//khazad
		case 48: if (town[st_ptr->town].type != 3) continue; else break;//minas anor - but it's town 0, type 1?!
		}
#endif
		/* Calculate the number of store maintainances since the last visit */
		maintain_num = (turn - st_ptr->last_visit) / ((10L * cfg.store_turns) / store_debug_quickmotion);

		/* Maintain the store max. 10 times */
		if (maintain_num > MAX_MAINTENANCES) maintain_num = MAX_MAINTENANCES;

		if (maintain_num) {
			/* Maintain the store */
			for (j = 0; j < maintain_num; j++) {
				store_maint(st_ptr);
				if (retire_owner_p(st_ptr)) store_shuffle(st_ptr);
			}

			/* Save the visit */
			st_ptr->last_visit = turn;

			/* cycle through items in stock now */
			for (what = 0; what < st_ptr->stock_num; what++) {
				o_ptr = &st_ptr->stock[what];

				/* hack: mention items only once, after they were generated for this store */			
				if (o_ptr->xtra1 != 222) continue;
				o_ptr->xtra1 = 0;

#if 0
				/* Log occurances of special items */
				if ((o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_ARTIFACT_CREATION && st_ptr->st_idx != 60) || /* avoid spam from SBM which offers lots of these */
				    (o_ptr->tval == TV_LITE && (o_ptr->name2 == EGO_LITE_MAGI || o_ptr->name2b == EGO_LITE_MAGI)) ||
 #if 0
				    (o_ptr->tval == TV_BOOK && st_ptr->st_idx == 48) ||
 #endif
				    (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_POWER && st_ptr->st_idx != 60)) {
					char o_name[ONAME_LEN];
					cptr s_name;
					object_desc(0, o_name, o_ptr, TRUE, 3);
					s_name = st_name + st_info[st_ptr->st_idx].name;
					s_printf("%s: REAL_STORE_CARRY: %d/%d - %d, %s (%s).\n", showtime(), st_ptr->town, town[st_ptr->town].type, st_ptr->st_idx, o_name, s_name);
				}
#endif
			}
		}
	}
}

#ifdef PLAYER_STORES
/* Is an item inscribed correctly to be sold in a player-run store? - C. Blue
   Returns price or 0 if not for sale.
   Pricing tag format:  @Snnnnnnnnn.  <- with dot for termination. */
static u32b player_store_inscribed(object_type *o_ptr, u32b price) {
	char buf[10], *p;
	u32b final_price;
	bool increase = FALSE;

	/* no item? */
	if (!o_ptr->k_idx) return -1;

	/* does it carry an inscription? */
	if (!o_ptr->note) return -1;

	/* is it a player-store inscription? */
	if (!(p = strstr(quark_str(o_ptr->note), "@S"))) return -1;

	/* is it an increase of the default price instead of a fixed price? */
	if (p[2] == '+') {
		increase = TRUE;
		p++;
	}

	/* is it a valid price tag? */
	strncpy(buf, p + 2, 9);
	buf[9] = '\0';
	/* if no number follows, we'll assume no price change */
	if (buf[0] < '0' || buf[0] > '9') return price;

	/* price limit is 2*10^9 */
	final_price = atol(buf);
	if (final_price > 2000000000) final_price = 2000000000;
	/* cannot be negative (paranoia, '-' cought above) */
	if (final_price <= 0) return price;

	/* do we want to set or to increase the base price? */
	if (increase) {
		/* never overflow (cap at 2*10^9) */
		if (price <= 2000000000 - final_price) final_price += price;
		else final_price = 2000000000;
	} else if (final_price < price)
		final_price = price;

	/* everything ok: valid price between 1 and 2,000,000,000 */
	return final_price;
}

/* Player enters a player-run store - C. Blue */
bool do_cmd_player_store(int Ind, int x, int y) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;

	cave_type **zcave, *c_ptr;

	house_type *h_ptr;
	int h_idx, i, j;
	bool is_store = FALSE; /* found at least one item for sale? */
	int fsidx = -1; /* index of a free fake store to use */

	/* access house door */
	if (!(zcave = getcave(&p_ptr->wpos))) return FALSE;

	/* Check that noone else is already using the store! */
	for (i = 1; i <= NumPlayers; i++) {
		/* Player is in a player store? */
		if (Players[i]->store_num <= -2) {
			/* Check whether he occupies this store */
			if (Players[i]->ps_house_x == x &&
			    Players[i]->ps_house_y == y) {
				/* Player is in THIS store! */
				if (Players[i]->admin_dm && !p_ptr->admin_dm) {
					/* Actually superfluous atm, because you do not 'enter'
					   a player store as you do a normal store, but instead
					   remain standing besides it, physically. */
					store_kick(i, FALSE);
					msg_print(i, "Someone has entered the store.");
					/* hack: repair grid's player index again */
					zcave[p_ptr->py][p_ptr->px].m_idx = -Ind;
				} else {
					msg_print(Ind, "The store is full.");
					return TRUE;
				}
			}
		}
	}

	/* Get house pointer and verify that it's a player store */
	h_idx = pick_house(&p_ptr->wpos, y, x);
	if (h_idx == -1) return FALSE;
	h_ptr = &houses[h_idx];


	disturb(Ind, 0, 0);

	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
	handle_stuff(Ind); /* update stealth/search display now */

	command_new = '_';


	/* Already grab a free fake store to store items in it which are
	   for sale while searching for them, in the verify-code below */
	for (i = 0; i < MAX_VISITED_PLAYER_STORES; i++) {
		/* loop until we find a free store */
		if (fake_store_visited[i]) continue;

		/* found a free one, remember its index and initialize it */
		fake_store[i].stock_num = 0;
		fake_store[i].player_owner = h_ptr->dna->owner;

		/* would need to set up a fake info template with legal
		   commands and all.. see hack in show_building().
		   So currently, setting st_idx is actually obsolete. */
		fake_store[i].st_idx = 66;
		fsidx = i;
		break;
	}

	/* Reset player's flags for accessing a player store and
	   for accessing a mass-cheque when buying piled wares */
	p_ptr->ps_house_x = p_ptr->ps_house_y = -1;
	p_ptr->ps_mcheque_x = p_ptr->ps_mcheque_y = -1;

	/* Traditional 'solid' house */
	if (h_ptr->flags & HF_TRAD) {
		for (i = 0; i < h_ptr->stock_num; i++) {
			o_ptr = &h_ptr->stock[i];

			/* test item if it's an existing mass-cheque */
			if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_CHEQUE &&
			    (!o_ptr->note || !strcmp(quark_str(o_ptr->note), "various piled items"))) {
				/* make sure inscription is consistent with player_store_handle_purchase() */
				p_ptr->ps_mcheque_x = i;
			}

			/* test item for being for sale */
			if (player_store_inscribed(o_ptr, 0) == -1) continue;

			/* found an item for sale */
			is_store = TRUE;
			if (fsidx == -1) break; /* no available fake store? */

			/* also add it to the fake store already */
			o_ptr->ps_idx_x = i;
			j = store_carry(&fake_store[fsidx], o_ptr);
//			if (j == -1) s_printf("PLAYER_STORE_CMD: store_carry -1!\n");
			if (fake_store[fsidx].stock_num == STORE_INVEN_MAX) break;
		}
	}
	/* Mang-style 'hollow' house */
	else {
		/* ALL houses are currently rectangular, so this check seems obsolete.. */
		if (h_ptr->flags & HF_RECT) {
			int sy, sx, ey, ex, cx, cy;
			sy = h_ptr->y + 1;
			sx = h_ptr->x + 1;
			ey = h_ptr->y + h_ptr->coords.rect.height - 1;
			ex = h_ptr->x + h_ptr->coords.rect.width - 1;
			for (cy = sy; cy < ey; cy++) {
			for (cx = sx; cx < ex; cx++) {
				c_ptr = &zcave[cy][cx];

				/* is there an object on the house floor grid?
				   If not, also remember this spot to create a mass cheque if needed. */
				if (!c_ptr->o_idx) {
					if (p_ptr->ps_mcheque_x == -1) {
						p_ptr->ps_mcheque_x = -2 - cx;
						p_ptr->ps_mcheque_y = -2 - cy;
					}
					continue;
				}
				o_ptr = &o_list[c_ptr->o_idx];

				/* test item if it's an existing mass-cheque */
				if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_CHEQUE &&
				    (!o_ptr->note || !strcmp(quark_str(o_ptr->note), "various piled items"))) {
					/* make sure inscription is consistent with player_store_handle_purchase() */
					p_ptr->ps_mcheque_x = cx;
					p_ptr->ps_mcheque_y = cy;
				}

				/* test it for being for sale */
				if (player_store_inscribed(o_ptr, 0) == -1) continue;

				/* found an item for sale */
				is_store = TRUE;
				if (fsidx == -1) break; /* no available fake store? */

				/* also add it to the fake store already */
				o_ptr->ps_idx_x = cx;
				o_ptr->ps_idx_y = cy;
				j = store_carry(&fake_store[fsidx], o_ptr);
//				if (j == -1) s_printf("PLAYER_STORE_CMD: store_carry -1!\n");
				/* if store is full (usually 48 items), we can stop */
				if (fake_store[fsidx].stock_num == STORE_INVEN_MAX) break;
			}
			/* Did we 'break;' inside the inner loop? Exit then. */
			if (is_store && fsidx == -1) break;
			if (fsidx != -1 && fake_store[fsidx].stock_num == STORE_INVEN_MAX) break;
			}
		}
	}

	/* if this house isn't a player-store, exit quietly */
	if (!is_store) return FALSE;

	if (p_ptr->inval) {
		msg_print(Ind, "\377UYou may not use a player store. Ask an admin to validate your account.");
		return FALSE;
	}

	/* if we're currently out of fake stores to use for this, display an excuse */
	if (fsidx == -1) {
		msg_print(Ind, "\377UThe shop is temporarily closed, please try again in a minute.");
		return FALSE;
	}

	/* HACK: Save fake store number.
	   NOTE: Client currently can handle negative store_num, since it only tests for == 7. */
	p_ptr->store_num = - 2 - fsidx;
	p_ptr->ps_house_x = x;
	p_ptr->ps_house_y = y;
	/* Reserve store for this player */
	fake_store_visited[i] = Ind;

	/* Set the timer (30000 used for homes aka "don't" kick out) */
	p_ptr->tim_store = STORE_TURNOUT;

	/* Display the store */
	display_store(Ind);

	/* Do not leave (unused?) */
	leave_store = FALSE;

	return TRUE;
}

/* Cut away the @S.. part of item inscriptions before
   displaying/selling them. - C. Blue */
void player_stores_cut_inscription(char *o_name) {
	char new_o_name[ONAME_LEN];
	cptr p, p2;

	/* hack around in new_o_name */
	strcpy(new_o_name, o_name);

	p = strstr(o_name, "@S");
	/* only if there's actually a '@S' inscription present */
	if (p) {
		new_o_name[p - o_name] = '\0';
		p2 = strstr(p, ".");
		if (p2) strcat(new_o_name, ++p2);

		/* write it back */
		strcpy(o_name, new_o_name);

		/* remove beginning of inscription if it's empty */
		if (o_name[strlen(o_name) - 1] == '{') {
			o_name[strlen(o_name) - 1] = '\0';
			/* remove one SPACE too */
			if (o_name[strlen(o_name) - 1] == ' ')
				o_name[strlen(o_name) - 1] = '\0';
		} else if (o_name[strlen(o_name) - 1] != '}') {
			/* If the inscription end is missing due to
			   player not specifying the terminating dot,
			   add a } bracket to fix that here. */
			if (strstr(o_name, "{")) strcat(o_name, "}");
		} else if (o_name[strlen(o_name) - 2] == '{') {
			/* if the last two chars are just {}, remove both */
			o_name[strlen(o_name) - 1] = '\0';
			o_name[strlen(o_name) - 1] = '\0';
			/* remove one SPACE too */
			if (o_name[strlen(o_name) - 1] == ' ')
				o_name[strlen(o_name) - 1] = '\0';
		}
	}
}

/* Get 4-Byte cheque value from 4 Bytes xtra1..xtra4 - C. Blue */
u32b ps_get_cheque_value(object_type *o_ptr) {
	u32b value = 0x0;
	value |= o_ptr->xtra1;
	value |= o_ptr->xtra2 << 8;
	value |= o_ptr->xtra3 << 16;
	value |= o_ptr->xtra4 << 24;
	return value;
}

/* Set 4-Byte cheque value from 4 Bytes xtra1..xtra4 - C. Blue */
static void ps_set_cheque_value(object_type *o_ptr, u32b value) {
	o_ptr->xtra1 = value & 0xFF;
	o_ptr->xtra2 = (value & 0xFF00) >> 8;
	o_ptr->xtra3 = (value & 0xFF0000) >> 16;
	o_ptr->xtra4 = (value & 0xFF000000) >> 24;
}

/* When an item is bought from a player store, locate the linked house
   and play back the changes, by reducing/removing the bought item from
   the house and inserting/modifying a cheque of the correct value.
   Note: Since we might run out of space easily if we create cheques for
         every potion that is sold off a stack, we will create one special
         cheque, dubbed 'mass cheque' that handles all stacked items. - C. Blue */
static void player_store_handle_purchase(int Ind, object_type *o_ptr, object_type *s_ptr, u32b value) {
	player_type *p_ptr = Players[Ind];
	object_type cheque, *cheque_ptr = &cheque;
	bool mass_cheque = FALSE;
	house_type *h_ptr;
	int h_idx, i, x, y;
	object_type *ho_ptr = NULL;
	cave_type **zcave, *c_ptr = NULL;
	char o_name[ONAME_LEN], o0_name[ONAME_LEN];
	u32b old_value;
	char owner_name[MAX_CHARS];

	/* paranoia */
	if (!o_ptr->number || !s_ptr->number) return;

	/* create a blanco cheque ;) */
	invcopy(&cheque, lookup_kind(TV_SCROLL, SV_SCROLL_CHEQUE));
	/* init its value */
	ps_set_cheque_value(&cheque, 0);
	/* bind it to the correct mode to avoid exploiting;
	   as a side effect, this tells us who bought our wares ;) */
	cheque.owner = p_ptr->id; /* set owner to buyer */
	cheque.mode = p_ptr->mode;
	cheque.level = 1; /* as long as owner is set to the buyer's id, this must be > 0 */

	/* take a cut from the sale, for the bank's erm cheque fee :-p
	      oh and for the dude who kept the shop in player's stead. Yeah. */
	if (value >= 20000000) {
		value = (value / 10) * 9;
	} else {
		value = (value * 9) / 10;
	}

	/* Try to locate house linked to this store */
	h_idx = pick_house(&p_ptr->wpos, p_ptr->ps_house_y, p_ptr->ps_house_x);
	if (h_idx == -1) {
		s_printf("PLAYER_STORE_HANDLE: NO HOUSE! (value %d, buyer %s)\n", value, p_ptr->name);
		return; /* oops? */
	}
	h_ptr = &houses[h_idx];
	/* Get house owner's name for misc purpose and notification */
	strcpy(owner_name, lookup_player_name(h_ptr->dna->owner));

	if (!(zcave = getcave(&p_ptr->wpos))) {
		s_printf("PLAYER_STORE_HANDLE: NO ZCAVE! (owner %s (%d), value %d, buyer %s)\n",
		    owner_name, h_ptr->dna->owner, value, p_ptr->name);
		return; /* oops? */
	}

	/* If it was completely bought out (ie the whole stack), we create a
	   normal cheque to replace the item in the house.
	   Otherwise we look for a mass-cheque or create one if none exists
	   yet which will be added to the house, so the store owner should
	   keep 1 slot free for this or his money goes poof! */
	if (s_ptr->number < o_ptr->number) mass_cheque = TRUE;
	object_desc(0, o0_name, s_ptr, TRUE, 3);


	/* -----------------------------------------------------------------------------------------------
	   reduce/remove the house items accordingly to how many were sold */

	/* Access original item in the house */
	if (h_ptr->flags & HF_TRAD) {
		ho_ptr = &h_ptr->stock[o_ptr->ps_idx_x];
	} else {
		/* ALL houses are currently rectangular, so this check seems obsolete.. */
		if (h_ptr->flags & HF_RECT) {
			c_ptr = &zcave[o_ptr->ps_idx_y][o_ptr->ps_idx_x];
			ho_ptr = &o_list[c_ptr->o_idx];
		}
	}

	/* We only need to reduce the items, if we're going to need a mass cheque,
	   because normal cheques just replace the items completely. */
	if (mass_cheque) {
		if (h_ptr->flags & HF_TRAD) {
			home_item_increase(h_ptr, o_ptr->ps_idx_x, -s_ptr->number);
			home_item_optimize(h_ptr, o_ptr->ps_idx_x);
		} else {
			/* Reduce amount of items in stock by how many were bought */
			floor_item_increase(c_ptr->o_idx, -s_ptr->number);
			floor_item_optimize(c_ptr->o_idx);
		}
	}


	/* -----------------------------------------------------------------------------------------------
	   Create/modify cheque */

	/* Create/modify a mass cheque? */
	if (mass_cheque) {
		/* Do we already have a mass-cheque? */
		if (p_ptr->ps_mcheque_x >= 0) {
			/* Access existing mass-cheque in the house */
			if (h_ptr->flags & HF_TRAD) {
				cheque_ptr = &h_ptr->stock[p_ptr->ps_mcheque_x];
s_printf("PLAYER_STORE: Mass Cheque (trad; owner %s (%d), %s, value %d, buyer %s)\n",
    owner_name, h_ptr->dna->owner, o0_name, value, p_ptr->name);
			} else {
				/* ALL houses are currently rectangular, so this check seems obsolete.. */
				if (h_ptr->flags & HF_RECT) {
					c_ptr = &zcave[p_ptr->ps_mcheque_y][p_ptr->ps_mcheque_x];
					cheque_ptr = &o_list[c_ptr->o_idx];
s_printf("PLAYER_STORE: Mass Cheque (mang; owner %s (%d), %s, value %d, buyer %s)\n",
    owner_name, h_ptr->dna->owner, o0_name, value, p_ptr->name);
				}
			}
		}
		/* Create a new mass cheque */
		else {
			/* drop it into the house */
			if (h_ptr->flags & HF_TRAD) {
				if (h_ptr->stock_num >= h_ptr->stock_size) {
					/* ouch, no room for dropping a cheque,
					   money goes poof :( */
					s_printf("PLAYER_STORE_HANDLE: NO ROOM! (owner %s (%d), value %d, buyer %s)\n",
					    owner_name, h_ptr->dna->owner, value, p_ptr->name);
					return;
				}
				/* add it to the stock and point to it.
				   Note: Admitted Ind should be the owner and not the
				         customer, but it shouldn't matter really. */
				i = home_carry(Ind, h_ptr, cheque_ptr);
				cheque_ptr = &h_ptr->stock[i];
s_printf("PLAYER_STORE: New Mass Cheque (trad; owner %s (%d), %s, value %d, buyer %s)\n",
    owner_name, h_ptr->dna->owner, o0_name, value, p_ptr->name);
			} else {
				/* ALL houses are currently rectangular, so this check seems obsolete.. */
				if (h_ptr->flags & HF_RECT) {
					if (p_ptr->ps_mcheque_x == -1) {
						/* ouch, no room for dropping a cheque,
						   money goes poof :( */
						s_printf("PLAYER_STORE_HANDLE: NO ROOM! (owner %s (%d), value %d, buyer %s)\n",
						    owner_name, h_ptr->dna->owner, value, p_ptr->name);
						return;
					}
					/* Access free spot and verify its freeness.. */
					x = -2 - p_ptr->ps_mcheque_x;
					y = -2 - p_ptr->ps_mcheque_y;
					c_ptr = &zcave[y][x];
					if (c_ptr->o_idx) {
						/* ouch, no room for dropping a cheque,
						   money goes poof :( */
						s_printf("PLAYER_STORE_HANDLE: NO ROOM! (owner %s (%d), value %d, buyer %s)\n",
						    owner_name, h_ptr->dna->owner, value, p_ptr->name);
						return;
					}
					/* Drop the cheque there */
					drop_near(cheque_ptr, -1, &p_ptr->wpos, y, x);
					/* Access the new item and point to it */
					cheque_ptr = &o_list[c_ptr->o_idx];
s_printf("PLAYER_STORE: New Mass Cheque (mang; owner %s (%d), %s, value %d, buyer %s)\n",
    owner_name, h_ptr->dna->owner, o0_name, value, p_ptr->name);
				}
			}
		}
		/* make sure the inscription is good (and consistent with do_cmd_player_store()) */
		cheque_ptr->note = quark_add("various piled items");

		/* Write back additional value to the cheque */
		old_value = ps_get_cheque_value(cheque_ptr);
		if (old_value <= 2000000000 - value) {
			ps_set_cheque_value(cheque_ptr, old_value + value);
		} else {
			/* erm, our cheque is already worth 2 bill? oO
			   Well.. can't save much more to it, we just
			   let it poof at this time.. */
		}
		/* We're done. */
	}
	/* Create a normal cheque? */
	else {
		/* inscribe the cheque with the full item name and its
		   sale-inscription, so the seller is well-informed. */
		object_desc(Ind, o_name, s_ptr, TRUE, 3);
		cheque_ptr->note = quark_add(o_name);

		/* Imprint value into the cheque. */
		ps_set_cheque_value(cheque_ptr, value);

		/* normal cheques replace the item they were created for,
		   easy as that. */
		if (h_ptr->flags & HF_TRAD) {
			/* For list house, overwriting is enough */
			object_copy(ho_ptr, cheque_ptr);
s_printf("PLAYER_STORE: Cheque (trad; owner %s (%d), %s, value %d, buyer %s)\n",
    owner_name, h_ptr->dna->owner, o0_name, value, p_ptr->name);
		} else {
			if (h_ptr->flags & HF_RECT) {
				/* Access free spot and verify its freeness.. */
				x = o_ptr->ps_idx_x;
				y = o_ptr->ps_idx_y;
				c_ptr = &zcave[y][x];
				/* Remove item that was sold */
				floor_item_increase(c_ptr->o_idx, -s_ptr->number);
				floor_item_optimize(c_ptr->o_idx);
				/* Drop the cheque there */
				drop_near(cheque_ptr, -1, &p_ptr->wpos, y, x);
s_printf("PLAYER_STORE: Cheque (mang; owner %s (%d), %s, value %d, buyer %s)\n",
    owner_name, h_ptr->dna->owner, o0_name, value, p_ptr->name);
			}
			//TODO: might require some everyone_lite_spot() etc stuff to redraw mang-houses..
		}
		/* We're done. */
	}

	/* Notify the store owner about the sale now or next time he logs in */
	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (strcmp(owner_name, Players[i]->name)) continue;

		/* Notify the owner now that he's online */
		msg_format(i, "\374\377yYour store at (%d,%d) just sold something!", p_ptr->wpos.wx, p_ptr->wpos.wy);
		break;
	}
	/* If he's not online, store account notification for later */
	if (i > NumPlayers) acc_set_flags_id(h_ptr->dna->owner, ACC_WARN_SALE, TRUE);
}
#endif

/* For normal shops (including player stores), it just sets store_num to -1.
   For SF1_SPECIAL buildings, it additionally cancels pending PKT_REQUEST..
   requests and currently ongoing interactions. - C. Blue */
void handle_store_leave(int Ind) {
	int i = gettown(Ind);
	store_type *st_ptr;
	player_type *p_ptr = Players[Ind];

	/* hack: non-town stores (ie dungeon, but could also be wild) */
	if (i == -1) i = gettown_dun(Ind);
	st_ptr = &town[i].townstore[p_ptr->store_num];

	/* Leave */
	p_ptr->store_num = -1;

	/* We're no longer busy with anything */
	p_ptr->store_action = 0;

	/* Not a special store? Nothing to do then */
	if (!(st_info[st_ptr->st_idx].flags1 & SF1_SPECIAL)) return;

	/* Pending requests to clear  */
	if (p_ptr->request_id != RID_NONE) {
		Send_request_abort(Ind);
	}

	/* Clean up SPECIAL store's status/features */
#ifdef ENABLE_GO_GAME
	if (go_game_up && p_ptr->id == go_engine_player_id) go_challenge_cancel();
#endif

	/* Done (no more pending request) */
	p_ptr->request_id = RID_NONE;
}
