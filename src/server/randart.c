/* $Id$ */
/* File: randart.c */

/* 
 * Purpose: Random artifacts 
 * Adapted from Greg Wooledge's artifact randomiser.
 */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

/*
 * Things using the similar method as randarts are also
 * put in here, eg. ego-items (and randunis).		- Jir -
 */

#include "angband.h"

/* How much power is assigned to randarts */
#define RANDART_QUALITY 40

/* How many attempts to add abilities */
#define MAX_TRIES 200


#define abs(x)	((x) > 0 ? (x) : (-(x)))
#define sign(x)	((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))

/*
 * We will prototype the artifact in this:
 */

artifact_type	randart;

artifact_type	*a_ptr;
object_kind 	*k_ptr;


/* 
 * Power assigned to artifact being made.
 */
s16b	power;



/* 
 * Calculate the multiplier we'll get with a given bow type.
 * This is done differently in 2.8.2 than it was in 2.8.1.
 */
static int bow_multiplier (int sval)
{
	switch (sval)
	{
		case SV_SLING: case SV_SHORT_BOW: return 2;

		case SV_LONG_BOW: case SV_LIGHT_XBOW: return 3;

		case SV_HEAVY_XBOW: return 4;

		  /*		default: msg_format ("Illegal bow sval %s\n", sval); */
      	}

	return 0;
}


/* 
 * We've just added an ability which uses the pval bonus.
 * Make sure it's not zero.  If it's currently negative, leave
 * it negative (heh heh).
 */
static void do_pval (artifact_type *a_ptr)
{
	/* Add some pval */
	if (a_ptr->pval == 0)
	{
		a_ptr->pval = 1 + rand_int (3);
	}

	/* Cursed -- make it worse! */
	else if (a_ptr->pval < 0)
	{
		if (rand_int (2) == 0) a_ptr->pval--;
	}

	/* Bump up an existing pval */
	else if (rand_int (3) > 0)
	{
		a_ptr->pval++;
	}

	/* Done */
	return;
}


/* 
 * Make it bad, or if it's already bad, make it worse!
 */
static void do_curse (artifact_type *a_ptr)
{
	/* Some chance of picking up these flags */
	if (rand_int (3) == 0) a_ptr->flags3 |= TR3_AGGRAVATE;
	if (rand_int (5) == 0) a_ptr->flags3 |= TR3_DRAIN_EXP;
	if (rand_int (8) == 0) a_ptr->flags5 |= TR5_DRAIN_MANA;
	if (rand_int (11) == 0) a_ptr->flags5 |= TR5_DRAIN_HP;
	if (rand_int (7) == 0) a_ptr->flags3 |= TR3_TELEPORT;

	/* Some chance or reversing good bonuses */
	if ((a_ptr->pval > 0) && (rand_int (2) == 0)) a_ptr->pval = -a_ptr->pval;
	if ((a_ptr->to_a > 0) && (rand_int (2) == 0)) a_ptr->to_a = -a_ptr->to_a;
	if ((a_ptr->to_h > 0) && (rand_int (2) == 0)) a_ptr->to_h = -a_ptr->to_h;
	if ((a_ptr->to_d > 0) && (rand_int (4) == 0)) a_ptr->to_d = -a_ptr->to_d;

	/* Some chance of making bad bonuses worse */
	if ((a_ptr->pval < 0) && (rand_int (2) == 0)) a_ptr->pval -= rand_int(2);
	if ((a_ptr->to_a < 0) && (rand_int (2) == 0)) a_ptr->to_a -= 3 + rand_int(10);
	if ((a_ptr->to_h < 0) && (rand_int (2) == 0)) a_ptr->to_h -= 3 + rand_int(6);
	if ((a_ptr->to_d < 0) && (rand_int (4) == 0)) a_ptr->to_d -= 3 + rand_int(6);

	/* If it is cursed, we can heavily curse it */
	if (a_ptr->flags3 & TR3_CURSED)
	{
		if (rand_int (2) == 0) a_ptr->flags3 |= TR3_HEAVY_CURSE;
		if (rand_int (15) == 0) a_ptr->flags4 |= TR4_CURSE_NO_DROP;
		return;
	}

	/* Always light curse the item */
	a_ptr->flags3 |= TR3_CURSED;

	/* Chance of heavy curse */
	if (rand_int (4) == 0) a_ptr->flags3 |= TR3_HEAVY_CURSE;
}

/* 
 * Evaluate the artifact's overall power level.
 */
static s32b artifact_power (artifact_type *a_ptr)
{
	s32b p = 0;
	int immunities = 0;

	/* Start with a "power" rating derived from the base item's level. */
	p = (k_ptr->level + 7) / 8;

	/* Evaluate certain abilities based on type of object. */
	switch (a_ptr->tval)
	{
		case TV_BOW:
		{
			int mult;

			p += (a_ptr->to_d + sign (a_ptr->to_d)) / 2;
			mult = bow_multiplier (a_ptr->sval);
			if (a_ptr->flags3 & TR3_XTRA_MIGHT)
			{
				if (a_ptr->pval > 3)
				{
					p += 20000;	/* inhibit */
					mult = 1;	/* don't overflow */
				}
				else
					mult += a_ptr->pval;
			}
			p *= mult;
			if (a_ptr->flags3 & TR3_XTRA_SHOTS)
			{
				if (a_ptr->pval > 3)
					p += 20000;	/* inhibit */
				else if (a_ptr->pval > 0)
					p *= (2 * a_ptr->pval);
			}
			p += (a_ptr->to_h + 3 * sign (a_ptr->to_h)) / 4;
			if (a_ptr->weight < k_ptr->weight) p++;
			break;
		}
		case TV_DIGGING:
			p += 40;
		case TV_BOOMERANG:
			if (a_ptr->flags3 & TR3_XTRA_SHOTS)
			{
				if (a_ptr->pval > 3)
					p += 20000;	/* inhibit */
				else if (a_ptr->pval > 0)
					p *= (2 * a_ptr->pval);
			}
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_AXE:
		case TV_MSTAFF:	// maybe this needs another entry
		{
			p += (a_ptr->dd * a_ptr->ds + 1) / 2;
			if (a_ptr->flags1 & TR1_SLAY_EVIL) p = (p * 3) / 2;
			if (a_ptr->flags1 & TR1_KILL_DRAGON) p = (p * 3) / 2;
			if (a_ptr->flags1 & TR1_SLAY_ANIMAL) p = (p * 4) / 3;
			if (a_ptr->flags1 & TR1_SLAY_UNDEAD) p = (p * 4) / 3;
			if (a_ptr->flags1 & TR1_SLAY_DRAGON) p = (p * 4) / 3;
			if (a_ptr->flags1 & TR1_SLAY_DEMON) p = (p * 5) / 4;
			if (a_ptr->flags1 & TR1_SLAY_TROLL) p = (p * 5) / 4;
			if (a_ptr->flags1 & TR1_SLAY_ORC) p = (p * 5) / 4;
			if (a_ptr->flags1 & TR1_SLAY_GIANT) p = (p * 6) / 5;

			if (a_ptr->flags1 & TR1_BRAND_ACID) p = p * 2;
			if (a_ptr->flags1 & TR1_BRAND_ELEC) p = (p * 3) / 2;
			if (a_ptr->flags1 & TR1_BRAND_FIRE) p = (p * 4) / 3;
			if (a_ptr->flags1 & TR1_BRAND_COLD) p = (p * 4) / 3;

			p += (a_ptr->to_d + 2 * sign (a_ptr->to_d)) / 3;
			if (a_ptr->to_d > 15) p += (a_ptr->to_d - 14) / 2;


			if ((a_ptr->flags1 & TR1_TUNNEL) &&
			    (a_ptr->tval != TV_DIGGING))
				p += a_ptr->pval * 3;

			p += (a_ptr->to_h + 3 * sign (a_ptr->to_h)) / 4;

			/* Remember, weight is in 0.1 lb. units. */
			if (a_ptr->weight != k_ptr->weight)
				p += (k_ptr->weight - a_ptr->weight) / 20;

			break;
		}
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		{
			p += (a_ptr->ac + 4 * sign (a_ptr->ac)) / 5;
			p += (a_ptr->to_h + sign (a_ptr->to_h)) / 2;
			p += (a_ptr->to_d + sign (a_ptr->to_d)) / 2;
			if (a_ptr->weight != k_ptr->weight)
				p += (k_ptr->weight - a_ptr->weight) / 30;
			break;
		}
		case TV_LITE:
		{
//			p += 35;
			p += 25;
			break;
		}
		case TV_RING:
		case TV_AMULET:
		{
			p += 20;
			/* hack -- Nazgul rings */
			if (a_ptr->sval == SV_RING_SPECIAL) p += 20;
			break;
		}
	}

	/* Other abilities are evaluated independent of the object type. */
	p += (a_ptr->to_a + 3 * sign (a_ptr->to_a)) / 4;
	if (a_ptr->to_a > 20) p += (a_ptr->to_a - 19) / 2;
	if (a_ptr->to_a > 30) p += (a_ptr->to_a - 29) / 2;
	if (a_ptr->to_a > 40) p += 20000;	/* inhibit */

	if (a_ptr->pval > 0)
	{
		if (a_ptr->flags1 & TR1_STR) p += a_ptr->pval * a_ptr->pval;
		if (a_ptr->flags1 & TR1_INT) p += a_ptr->pval * a_ptr->pval;
		if (a_ptr->flags1 & TR1_WIS) p += a_ptr->pval * a_ptr->pval;
		if (a_ptr->flags1 & TR1_DEX) p += a_ptr->pval * a_ptr->pval;
		if (a_ptr->flags1 & TR1_CON) p += a_ptr->pval * a_ptr->pval;
		if (a_ptr->flags1 & TR1_STEALTH) p += a_ptr->pval * a_ptr->pval;
	}
	else if (a_ptr->pval < 0)	/* hack: don't give large negatives */
	{
		if (a_ptr->flags1 & TR1_STR) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_INT) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_WIS) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_DEX) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_CON) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_STEALTH) p += a_ptr->pval;
	}
	if (a_ptr->flags1 & TR1_CHR) p += a_ptr->pval;
	if (a_ptr->flags1 & TR1_INFRA) p += (a_ptr->pval + sign (a_ptr->pval)) / 2;
        if (a_ptr->flags1 & TR1_SPEED) p += a_ptr->pval * 2;
        if (a_ptr->flags1 & TR1_MANA) p += a_ptr->pval * 2;

	if (a_ptr->flags2 & TR2_SUST_STR) p += 6;
	if (a_ptr->flags2 & TR2_SUST_INT) p += 4;
	if (a_ptr->flags2 & TR2_SUST_WIS) p += 4;
	if (a_ptr->flags2 & TR2_SUST_DEX) p += 4;
	if (a_ptr->flags2 & TR2_SUST_CON) p += 4;
	if (a_ptr->flags2 & TR2_SUST_CHR) p += 1;
	if (a_ptr->flags2 & TR2_IM_ACID)
	{
		p += 20;
		immunities++;
	}
	if (a_ptr->flags2 & TR2_IM_ELEC)
	{
		p += 24;
		immunities++;
	}
	if (a_ptr->flags2 & TR2_IM_FIRE)
	{
		p += 36;
		immunities++;
	}
	if (a_ptr->flags2 & TR2_IM_COLD)
	{
		p += 24;
		immunities++;
	}
	if (immunities > 1) p += 16;
	if (immunities > 2) p += 16;
	if (immunities > 3) p += 20000;		/* inhibit */
	if (a_ptr->flags2 & TR2_FREE_ACT) p += 8;
	if (a_ptr->flags2 & TR2_HOLD_LIFE) p += 10;
	if (a_ptr->flags2 & TR2_RES_ACID) p += 6;
	if (a_ptr->flags2 & TR2_RES_ELEC) p += 6;
	if (a_ptr->flags2 & TR2_RES_FIRE) p += 6;
	if (a_ptr->flags2 & TR2_RES_COLD) p += 6;
	if (a_ptr->flags2 & TR2_RES_POIS) p += 12;
	if (a_ptr->flags2 & TR2_RES_LITE) p += 8;
	if (a_ptr->flags2 & TR2_RES_DARK) p += 10;
	if (a_ptr->flags2 & TR2_RES_BLIND) p += 10;
	if (a_ptr->flags2 & TR2_RES_CONF) p += 8;
	if (a_ptr->flags2 & TR2_RES_SOUND) p += 10;
	if (a_ptr->flags2 & TR2_RES_SHARDS) p += 8;
	if (a_ptr->flags2 & TR2_RES_NETHER) p += 12;
	if (a_ptr->flags2 & TR2_RES_NEXUS) p += 10;
	if (a_ptr->flags2 & TR2_RES_CHAOS) p += 12;
	if (a_ptr->flags2 & TR2_RES_DISEN) p += 12;

	if (a_ptr->flags3 & TR3_FEATHER) p += 2;
	if (a_ptr->flags3 & TR3_LITE1) p += 2;
	if (a_ptr->flags4 & TR4_LITE2) p += 4;
	if (a_ptr->flags4 & TR4_LITE3) p += 8;
	if (a_ptr->flags4 & TR4_FUEL_LITE) p -= 20;
	if (a_ptr->flags3 & TR3_SEE_INVIS) p += 8;
//	if (a_ptr->flags3 & TR3_TELEPATHY) p += 20;
	if (a_ptr->esp & (ESP_ORC)) p += 1;
	if (a_ptr->esp & (ESP_TROLL)) p += 1;
	if (a_ptr->esp & (ESP_DRAGON)) p += 2;
	if (a_ptr->esp & (ESP_GIANT)) p += 1;
	if (a_ptr->esp & (ESP_DEMON)) p += 1;
	if (a_ptr->esp & (ESP_UNDEAD)) p += 2;
	if (a_ptr->esp & (ESP_EVIL)) p += 8;
	if (a_ptr->esp & (ESP_ANIMAL)) p += 1;
	if (a_ptr->esp & (ESP_DRAGONRIDER)) p += 1;
	if (a_ptr->esp & (ESP_GOOD)) p += 2;
	if (a_ptr->esp & (ESP_NONLIVING)) p += 2;
	if (a_ptr->esp & (ESP_UNIQUE)) p += 8;
	if (a_ptr->esp & (ESP_SPIDER)) p += 2;
	if (a_ptr->esp & ESP_ALL) p += 20;
        if (a_ptr->flags4 & TR4_AUTO_ID) p += 20;
	if (a_ptr->flags3 & TR3_SLOW_DIGEST) p += 4;
	if (a_ptr->flags3 & TR3_REGEN) p += 8;
	if (a_ptr->flags5 & TR5_REGEN_MANA) p += 8;
	if (a_ptr->flags3 & TR3_TELEPORT) p -= 20;
	if (a_ptr->flags3 & TR3_DRAIN_EXP) p -= 16;
	if (a_ptr->flags3 & TR3_AGGRAVATE) p -= 8;
	if (a_ptr->flags3 & TR3_BLESSED) p += 4;
	if (a_ptr->flags3 & TR3_CURSED) p -= 4;
	if (a_ptr->flags3 & TR3_HEAVY_CURSE) p -= 20;
/*	if (a_ptr->flags3 & TR3_PERMA_CURSE) p -= 40; */
	if (a_ptr->flags4 & TR4_ANTIMAGIC_10) p += 8;
	if (a_ptr->flags5 & TR5_INVIS) p += 20;

	return p;
}



static void remove_contradictory (artifact_type *a_ptr)
{
	if (a_ptr->flags3 & TR3_AGGRAVATE)
	{
		a_ptr->flags1 &= ~(TR1_STEALTH);
		a_ptr->flags5 &= ~(TR5_INVIS);
	}
	if (a_ptr->flags2 & TR2_IM_ACID) a_ptr->flags2 &= ~(TR2_RES_ACID);
	if (a_ptr->flags2 & TR2_IM_ELEC) a_ptr->flags2 &= ~(TR2_RES_ELEC);
	if (a_ptr->flags2 & TR2_IM_FIRE) a_ptr->flags2 &= ~(TR2_RES_FIRE);
	if (a_ptr->flags2 & TR2_IM_COLD) a_ptr->flags2 &= ~(TR2_RES_COLD);
	if (a_ptr->pval < 0)
	{
		if (a_ptr->flags1 & TR1_STR) a_ptr->flags2 &= ~(TR2_SUST_STR);
		if (a_ptr->flags1 & TR1_INT) a_ptr->flags2 &= ~(TR2_SUST_INT);
		if (a_ptr->flags1 & TR1_WIS) a_ptr->flags2 &= ~(TR2_SUST_WIS);
		if (a_ptr->flags1 & TR1_DEX) a_ptr->flags2 &= ~(TR2_SUST_DEX);
		if (a_ptr->flags1 & TR1_CON) a_ptr->flags2 &= ~(TR2_SUST_CON);
		if (a_ptr->flags1 & TR1_CHR) a_ptr->flags2 &= ~(TR2_SUST_CHR);
		/*a_ptr->flags1 &= ~(TR1_BLOWS);*/
	}
	if (a_ptr->flags3 & TR3_CURSED) a_ptr->flags3 &= ~(TR3_BLESSED);
	if (a_ptr->flags1 & TR1_KILL_DRAGON) a_ptr->flags1 &= ~(TR1_SLAY_DRAGON);
	if (a_ptr->flags3 & TR3_DRAIN_EXP) a_ptr->flags2 &= ~(TR2_HOLD_LIFE);

	if (a_ptr->esp & (ESP_ALL)) a_ptr->esp = (ESP_ALL);
}



/* 
 * Randomly select an extra ability to be added to the artifact in question.
 * This function is way too large.
 */
static void add_ability (artifact_type *a_ptr)
{
	int r;

	
	r = rand_int (10);
	/* if r < 5 -> Pick something dependent on item type. */
	if ((r < 5) || (a_ptr->tval == TV_SHOT) ||
	(a_ptr->tval == TV_ARROW) || (a_ptr->tval == TV_BOLT))
	{
		r = rand_int (100);
		switch (a_ptr->tval)
		{
			case TV_BOW:
			{
				if (r < 15)
				{
					a_ptr->flags3 |= TR3_XTRA_SHOTS;
					do_pval (a_ptr);
				}
				else if (r < 35)
				{
					a_ptr->flags3 |= TR3_XTRA_MIGHT;
					do_pval (a_ptr);
				}
				else if (r < 65) a_ptr->to_h += 2 + rand_int (2);
				else a_ptr->to_d += 2 + rand_int (3);

				break;
			}
			case TV_DIGGING:
			case TV_HAFTED:
			case TV_POLEARM:
			case TV_SWORD:
			case TV_AXE:
			case TV_BOOMERANG:
			case TV_MSTAFF:
			{
				if (r < 4)
				{
					a_ptr->flags1 |= TR1_WIS;
					do_pval (a_ptr);
					if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_WIS;
					if (a_ptr->tval == TV_SWORD || a_ptr->tval == TV_POLEARM ||
					    a_ptr->tval == TV_HAFTED || a_ptr->tval == TV_AXE)
						a_ptr->flags3 |= TR3_BLESSED;
				}
				else if (r < 7)
				{
					a_ptr->flags1 |= TR1_BRAND_ACID;
					if (rand_int (4) > 0) a_ptr->flags2 |= TR2_RES_ACID;
				}
				else if (r < 10)
				{
					a_ptr->flags1 |= TR1_BRAND_ELEC;
					if (rand_int (4) > 0) a_ptr->flags2 |= TR2_RES_ELEC;
				}
				else if (r < 14)
				{
					a_ptr->flags1 |= TR1_BRAND_FIRE;
					if (rand_int (4) > 0) a_ptr->flags2 |= TR2_RES_FIRE;
				}
				else if (r < 18)
				{
					a_ptr->flags1 |= TR1_BRAND_COLD;
					if (rand_int (4) > 0) a_ptr->flags2 |= TR2_RES_COLD;
				}
				else if (r < 25)
				{
					a_ptr->dd += 1 + rand_int (2) + rand_int (2);
					if (a_ptr->dd > 9) a_ptr->dd = 9;
				}
				else if (r < 27)
				{ 
					a_ptr->flags1 |= TR1_KILL_DRAGON;
					a_ptr->esp |= (ESP_DRAGON);
				}
				else if (r < 29)
				{ 
					a_ptr->flags1 |= TR1_KILL_DEMON;
					a_ptr->esp |= (ESP_DEMON);
				}
				else if (r < 31)
				{ 
					a_ptr->flags1 |= TR1_KILL_UNDEAD;
					a_ptr->esp |= (ESP_UNDEAD);
				}
				else if (r < 35)
				{
					a_ptr->flags1 |= TR1_SLAY_DRAGON;
					if (magik(80)) a_ptr->esp |= (ESP_DRAGON);
				}
				else if (r < 39)
				{
					a_ptr->flags1 |= TR1_SLAY_EVIL;
					if (magik(80)) a_ptr->esp |= (ESP_EVIL);
				}

				else if (r < 43)
				{
					a_ptr->flags1 |= TR1_SLAY_ANIMAL;
					if (magik(80)) a_ptr->esp |= (ESP_EVIL);
				}
				else if (r < 47)
				{
					a_ptr->flags1 |= TR1_SLAY_UNDEAD;
					if (magik(80)) a_ptr->esp |= (ESP_UNDEAD);
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_DEMON;
						if (magik(80)) a_ptr->esp |= (ESP_DEMON);
					}
				}
				else if (r < 51)
				{
					a_ptr->flags1 |= TR1_SLAY_DEMON;
					if (magik(80)) a_ptr->esp |= (ESP_DEMON);
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_UNDEAD;
						if (magik(80)) a_ptr->esp |= (ESP_UNDEAD);
					}
				}
				else if (r < 55)
				{
					a_ptr->flags1 |= TR1_SLAY_ORC;
					if (magik(80)) a_ptr->esp |= (ESP_ORC);
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_TROLL;
						if (magik(80)) a_ptr->esp |= (ESP_TROLL);
					}
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_GIANT;
						if (magik(80)) a_ptr->esp |= (ESP_GIANT);
					}
				}
				else if (r < 59)
				{
					a_ptr->flags1 |= TR1_SLAY_TROLL;
					if (magik(80)) a_ptr->esp |= (ESP_TROLL);
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_ORC;
						if (magik(80)) a_ptr->esp |= (ESP_ORC);
					}
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_GIANT;
						if (magik(80)) a_ptr->esp |= (ESP_GIANT);
					}
				}
				else if (r < 63)
				{
					a_ptr->flags1 |= TR1_SLAY_GIANT;
					if (magik(80)) a_ptr->esp |= (ESP_GIANT);
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_ORC;
						if (magik(80)) a_ptr->esp |= (ESP_ORC);
					}
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_TROLL;
						if (magik(80)) a_ptr->esp |= (ESP_TROLL);
					}
				}
				else if (r < 68) a_ptr->flags3 |= TR3_SEE_INVIS;
				else if (r < 72)
				{
					if (a_ptr->tval == TV_BOOMERANG) {
						a_ptr->flags3 |= TR3_XTRA_SHOTS;
						do_pval (a_ptr);
						if (a_ptr->pval > 1) a_ptr->pval = 1;
					} else {
						a_ptr->flags1 |= TR1_BLOWS;
						do_pval (a_ptr);
						if (a_ptr->pval > 3) a_ptr->pval = 3;
					}
				}
				else if (r < 85)
				{
					a_ptr->to_d += 2 + rand_int (10);
					a_ptr->to_h += 2 + rand_int (10);
				}
				else if (r < 88) a_ptr->to_a += 3 + rand_int (3);
				else if (r < 91)
				{
					a_ptr->flags5 |= TR5_CRIT;
					if (a_ptr->pval < 0) break;
					if (a_ptr->pval == 0) a_ptr->pval = 3 + rand_int (8);
					else if (rand_int (2) == 0) a_ptr->pval++;
				}
				else if (r < 95)
				{
					switch(a_ptr->tval) {
					case TV_DIGGING:
						a_ptr->pval++;
						break;
					case TV_MSTAFF:
						a_ptr->pval++;
						break;
					default: /* normal weapons */
						a_ptr->flags1 |= TR1_VAMPIRIC;
					}
				}
				else a_ptr->weight = (a_ptr->weight * 9) / 10;

				break;
			}
			case TV_SHOT:
			case TV_ARROW:
			case TV_BOLT:
			{
				if (r < 4)
				{
					a_ptr->flags1 |= TR1_BRAND_ACID;
				}
				else if (r < 8)
				{
					a_ptr->flags1 |= TR1_BRAND_ELEC;
				}
				else if (r < 12)
				{
					a_ptr->flags1 |= TR1_BRAND_FIRE;
				}
				else if (r < 16)
				{
					a_ptr->flags1 |= TR1_BRAND_COLD;
				}
				else if (r < 20)
				{
					a_ptr->flags1 |= TR1_BRAND_POIS;
				}
				else if (r < 24)
				{ 
					a_ptr->flags1 |= TR1_KILL_DRAGON;
				}
				else if (r < 28)
				{ 
					a_ptr->flags1 |= TR1_KILL_DEMON;
				}
				else if (r < 32)
				{ 
					a_ptr->flags1 |= TR1_KILL_UNDEAD;
				}
				else if (r < 40)
				{
					a_ptr->flags1 |= TR1_SLAY_DRAGON;
				}
				else if (r < 44)
				{
					a_ptr->flags1 |= TR1_SLAY_EVIL;
				}
				else if (r < 52)
				{
					a_ptr->flags1 |= TR1_SLAY_ANIMAL;
				}
				else if (r < 60)
				{
					a_ptr->flags1 |= TR1_SLAY_UNDEAD;
				}
				else if (r < 68)
				{
					a_ptr->flags1 |= TR1_SLAY_DEMON;
				}
				else if (r < 72)
				{
					a_ptr->flags1 |= TR1_SLAY_ORC;
				}
				else if (r < 80)
				{
					a_ptr->flags1 |= TR1_SLAY_TROLL;
				}
				else if (r < 88)
				{
					a_ptr->flags1 |= TR1_SLAY_GIANT;
				}
				else if (r < 92)
				{
					a_ptr->flags1 |= TR1_VAMPIRIC;
				}
				else
				{
				    /* bad luck */
				}
				break;
			}
			case TV_BOOTS:
			{
				if (r < 10) a_ptr->flags3 |= TR3_FEATHER;
				else if (r < 30) a_ptr->to_a += 2 + rand_int (4);
				else if (r < 40)
				{
					a_ptr->flags4 |= TR4_FLY;
				}
				else if (r < 50) a_ptr->flags4 |= TR4_CLIMB;
				else if (r < 80)
				{
					a_ptr->flags1 |= TR1_STEALTH;
					do_pval (a_ptr);
				}
				else if (r < 90)
				{
					a_ptr->flags1 |= TR1_SPEED;
					if (a_ptr->pval < 0) break;
					if (a_ptr->pval == 0) a_ptr->pval = 3 + rand_int (8);
					else if (rand_int (2) == 0) a_ptr->pval++;
				}
				else a_ptr->weight = (a_ptr->weight * 9) / 10;
				break;
			}
			case TV_GLOVES:
			{
				if (r < 15) a_ptr->flags2 |= TR2_FREE_ACT;
				else if (r < 25)
				{
					a_ptr->flags1 |= TR1_MANA;
					if (a_ptr->pval == 0) a_ptr->pval = 5 + rand_int (6);
					else do_pval (a_ptr);
					if (a_ptr->pval < 0) a_ptr->pval = 2;
				}
				else if (r < 30)
				  {
				    a_ptr->flags4 |= TR4_AUTO_ID;
				  }
				else if (r < 41)
				{
					a_ptr->flags1 |= TR1_DEX;
					do_pval (a_ptr);
				}
				else if (r < 52)
				{
					a_ptr->flags1 |= TR1_STR;
					do_pval (a_ptr);
				}
				else if (r < 57)
				{
					a_ptr->flags1 |= TR1_BLOWS;
					if (rand_int(3)) a_ptr->pval = 1;
                                        else a_ptr->pval = 2;
				}
				else if (r < 62)
				{
					a_ptr->flags5 |= TR5_CRIT;
					if (a_ptr->pval < 0) break;
					if (a_ptr->pval == 0) a_ptr->pval = 3 + rand_int (8);
					else if (rand_int (2) == 0) a_ptr->pval++;
				}
				else if (r < 77)
				{
				int rsub = 60 + rand_int(24);
				if ((rsub < 62) && !(a_ptr->flags1 & TR1_MULTMASK))
				{
					a_ptr->flags1 |= TR1_BRAND_ACID;
				}
				else if ((rsub < 64) && !(a_ptr->flags1 & TR1_MULTMASK))
				{
					a_ptr->flags1 |= TR1_BRAND_ELEC;
				}
				else if ((rsub < 66) && !(a_ptr->flags1 & TR1_MULTMASK))
				{
					a_ptr->flags1 |= TR1_BRAND_FIRE;
				}
				else if ((rsub < 68) && !(a_ptr->flags1 & TR1_MULTMASK))
				{
					a_ptr->flags1 |= TR1_BRAND_COLD;
				}
				else if ((rsub < 70) && !(a_ptr->flags1 & TR1_MULTMASK))
				{
					a_ptr->flags1 |= TR1_BRAND_POIS;
				}
				else if ((rsub < 72) && !(a_ptr->flags1 & TR1_MULTMASK))
				{
					a_ptr->flags1 |= TR1_SLAY_DRAGON;
				}
				else if ((rsub < 74) && !(a_ptr->flags1 & TR1_MULTMASK))
				{
					a_ptr->flags1 |= TR1_SLAY_ANIMAL;
				}
				else if ((rsub < 76) && !(a_ptr->flags1 & TR1_MULTMASK))
				{
					a_ptr->flags1 |= TR1_SLAY_UNDEAD;
				}
				else if ((rsub < 78) && !(a_ptr->flags1 & TR1_MULTMASK))
				{
					a_ptr->flags1 |= TR1_SLAY_DEMON;
				}
				else if ((rsub < 80) && !(a_ptr->flags1 & TR1_MULTMASK))
				{
					a_ptr->flags1 |= TR1_SLAY_ORC;
				}
				else if ((rsub < 82) && !(a_ptr->flags1 & TR1_MULTMASK))
				{
					a_ptr->flags1 |= TR1_SLAY_TROLL;
				}
				else if ((rsub < 84) && !(a_ptr->flags1 & TR1_MULTMASK))
				{
					a_ptr->flags1 |= TR1_SLAY_GIANT;
				}
#if 0 //too powerful on gloves - Art Gloves 'soul cure' can help.
				else if (r < 71) && !(a_ptr->flags1 & TR1_MULTMASK))
				{ 
					a_ptr->flags1 |= TR1_KILL_DRAGON;
				}
				else if (r < 72) && !(a_ptr->flags1 & TR1_MULTMASK))
				{ 
					a_ptr->flags1 |= TR1_KILL_UNDEAD;
				}
				else if (r < 73) && !(a_ptr->flags1 & TR1_MULTMASK))
				{ 
					a_ptr->flags1 |= TR1_KILL_DEMON;
				}
				else if (r < 74) && !(a_ptr->flags1 & TR1_MULTMASK))
				{
					a_ptr->flags1 |= TR1_SLAY_EVIL;
				}
#endif
				}
				else if (r < 81)
				{
					a_ptr->flags1 |= TR1_VAMPIRIC;
				}
				else if (r < 92)
					a_ptr->to_a += 3 + rand_int (3);
				else
				{
					a_ptr->to_h += 2 + rand_int (3);
					a_ptr->to_d += 2 + rand_int (3);
					a_ptr->flags3 |= TR3_SHOW_MODS;
				}
				break;
			}
			case TV_HELM:
			case TV_CROWN:
			{
				if (r < 15) a_ptr->flags2 |= TR2_RES_BLIND;
				if (r < 17) {
					a_ptr->flags1 |= TR1_INFRA;
					if (a_ptr->pval == 0) a_ptr->pval = randint(2);
				}
				else if (r < 20) a_ptr->flags5 |= TR5_REGEN_MANA;
				else if (r < 22) {
					a_ptr->flags1 |= TR1_MANA;
					if (a_ptr->pval == 0) a_ptr->pval = randint(5);
				}
				else if (r < 30)
				  {
				    a_ptr->flags4 |= TR4_AUTO_ID;
				  }
//				else if (r < 45) a_ptr->flags3 |= TR3_TELEPATHY;
//				else if (r < 45) a_ptr->esp |= (ESP_ALL);
				else if (r < 31) a_ptr->esp |= (ESP_ORC);
				else if (r < 32) a_ptr->esp |= (ESP_TROLL);
				else if (r < 33) a_ptr->esp |= (ESP_DRAGON);
				else if (r < 34) a_ptr->esp |= (ESP_GIANT);
				else if (r < 35) a_ptr->esp |= (ESP_DEMON);
				else if (r < 36) a_ptr->esp |= (ESP_UNDEAD);
				else if (r < 37) a_ptr->esp |= (ESP_EVIL);
				else if (r < 38) a_ptr->esp |= (ESP_ANIMAL);
				else if (r < 39) a_ptr->esp |= (ESP_DRAGONRIDER);
				else if (r < 40) a_ptr->esp |= (ESP_GOOD);
				else if (r < 41) a_ptr->esp |= (ESP_NONLIVING);
				else if (r < 42) a_ptr->esp |= (ESP_UNIQUE);
				else if (r < 43) a_ptr->esp |= (ESP_SPIDER);
				else if (r < 44) a_ptr->esp |= (ESP_ALL);
				else if (r < 60) a_ptr->flags3 |= TR3_SEE_INVIS;
				else if (r < 70)
				{
					a_ptr->flags1 |= TR1_WIS;
					do_pval (a_ptr);
				}
				else if (r < 80)
				{
					a_ptr->flags1 |= TR1_INT;
					do_pval (a_ptr);
				}
				else if (r < 85) a_ptr->flags2 |= TR2_RES_CONF;
				else if (r < 90) a_ptr->flags2 |= TR2_RES_FEAR;
				else a_ptr->to_a += 3 + rand_int (3);
				break;
			}
			case TV_SHIELD:
			{
				if (r < 18) a_ptr->flags2 |= TR2_RES_ACID;
				else if (r < 36) a_ptr->flags2 |= TR2_RES_ELEC;
				else if (r < 54) a_ptr->flags2 |= TR2_RES_FIRE;
				else if (r < 72) a_ptr->flags2 |= TR2_RES_COLD;
				else if (r < 80) a_ptr->flags5 |= TR5_REFLECT;
				else a_ptr->to_a += 3 + rand_int (3);
				break;
			}
			case TV_CLOAK:
			{
				if (r < 20)
				{
					a_ptr->flags3 |= TR3_FEATHER;
					do_pval (a_ptr);
				}
				else if (r < 28)//30
				{
					if (!(a_ptr->flags3 & TR3_SH_FIRE || a_ptr->flags3 & TR3_SH_ELEC))
					{
						if (rand_int(2)) {
							a_ptr->flags3 |= TR3_SH_FIRE;
							a_ptr->flags2 |= TR2_RES_FIRE;
						} else {
							a_ptr->flags3 |= TR3_SH_ELEC;
							a_ptr->flags2 |= TR2_RES_ELEC;
						}
					}
				}
				else if (r < 31) a_ptr->flags5 |= TR5_INVIS;//33
				else if (r < 53)//55
				{
					a_ptr->flags1 |= TR1_STEALTH;
					do_pval (a_ptr);
				}
				else if (r < 63)
				{
					a_ptr->flags2 |= TR2_RES_SHARDS;
				}
				else if (r < 70)
				{
					a_ptr->flags4 |= TR4_FLY;
				}
				else a_ptr->to_a += 3 + rand_int(3);
				break;
			}
			case TV_SOFT_ARMOR:
			case TV_HARD_ARMOR:
			{
				if (r < 8)
				{
					a_ptr->flags1 |= TR1_STEALTH;
					do_pval (a_ptr);
				}
				else if (r < 16) a_ptr->flags2 |= TR2_HOLD_LIFE;
				else if (r < 22)
				{
					a_ptr->flags1 |= TR1_CON;
					do_pval (a_ptr);
					if (rand_int (2) == 0)
						a_ptr->flags2 |= TR2_SUST_CON;
				}
				else if (r < 34) a_ptr->flags2 |= TR2_RES_ACID;
				else if (r < 46) a_ptr->flags2 |= TR2_RES_ELEC;
				else if (r < 58) a_ptr->flags2 |= TR2_RES_FIRE;
				else if (r < 70) a_ptr->flags2 |= TR2_RES_COLD;
				else if (r < 80)
					a_ptr->weight = (a_ptr->weight * 9) / 10;
				else a_ptr->to_a += 3 + rand_int (8);
				break;
			}
			case TV_LITE:
			{
				if (r < 50) a_ptr->flags3 |= TR3_LITE1;
				else if (r < 80) a_ptr->flags4 |= TR4_LITE2;
				else a_ptr->flags4 |= TR4_LITE3;

//				if (r % 2) a_ptr->flags4 &= ~TR4_FUEL_LITE;
				if (!(r % 5)) a_ptr->flags4 &= ~TR4_FUEL_LITE;
				break;
			}
		}
	}
	else			/* Pick something universally useful. */
	{
		r = rand_int(45);
		switch (r)
		{
			case 0:
				a_ptr->flags1 |= TR1_STR;
				do_pval (a_ptr);
				if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_STR;
				break;
			case 1:
				a_ptr->flags1 |= TR1_INT;
				do_pval (a_ptr);
				if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_INT;
				break;
			case 2:
				a_ptr->flags1 |= TR1_WIS;
				do_pval (a_ptr);
				if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_WIS;
				if (a_ptr->tval == TV_SWORD || a_ptr->tval == TV_POLEARM)
					a_ptr->flags3 |= TR3_BLESSED;
				break;
			case 3:
				a_ptr->flags1 |= TR1_DEX;
				do_pval (a_ptr);
				if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_DEX;
				break;
			case 4:
				a_ptr->flags1 |= TR1_CON;
				do_pval (a_ptr);
				if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_CON;
				break;
			case 5:
				a_ptr->flags1 |= TR1_CHR;
				do_pval (a_ptr);
				if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_CHR;
				break;

			case 6:
				a_ptr->flags1 |= TR1_STEALTH;
				do_pval (a_ptr);
				break;
			case 7:
				a_ptr->flags1 |= TR1_SEARCH;
				do_pval (a_ptr);
				break;
			case 8:
				a_ptr->flags1 |= TR1_INFRA;
				do_pval (a_ptr);
				break;
			case 9:
				a_ptr->flags1 |= TR1_SPEED;
				if (a_ptr->pval == 0) a_ptr->pval = 3 + rand_int (3);
				else do_pval (a_ptr);
				break;

			case 10:
				a_ptr->flags2 |= TR2_SUST_STR;
				if (rand_int (2) == 0)
				{
					a_ptr->flags1 |= TR1_STR;
					do_pval (a_ptr);
				}
				break;
			case 11:
				a_ptr->flags2 |= TR2_SUST_INT;
				if (rand_int (2) == 0)
				{
					a_ptr->flags1 |= TR1_INT;
					do_pval (a_ptr);
				}
				break;
			case 12:
				a_ptr->flags2 |= TR2_SUST_WIS;
				if (rand_int (2) == 0)
				{
					a_ptr->flags1 |= TR1_WIS;
					do_pval (a_ptr);
					if (a_ptr->tval == TV_SWORD || a_ptr->tval == TV_POLEARM)
						a_ptr->flags3 |= TR3_BLESSED;
				}
				break;
			case 13:
				a_ptr->flags2 |= TR2_SUST_DEX;
				if (rand_int (2) == 0)
				{
					a_ptr->flags1 |= TR1_DEX;
					do_pval (a_ptr);
				}
				break;
			case 14:
				a_ptr->flags2 |= TR2_SUST_CON;
				if (rand_int (2) == 0)
				{
					a_ptr->flags1 |= TR1_CON;
					do_pval (a_ptr);
				}
				break;
			case 15:
				a_ptr->flags2 |= TR2_SUST_CHR;
				if (rand_int (2) == 0)
				{
					a_ptr->flags1 |= TR1_CHR;
					do_pval (a_ptr);
				}
				break;

			case 16:
			{
				if (rand_int (3) == 0) a_ptr->flags2 |= TR2_IM_ACID;
				break;
			}
			case 17:
			{
				if (rand_int (3) == 0) a_ptr->flags2 |= TR2_IM_ELEC;
				break;
			}
			case 18:
			{
				if (rand_int (4) == 0) a_ptr->flags2 |= TR2_IM_FIRE;
				break;
			}
			case 19:
			{
				if (rand_int (3) == 0) a_ptr->flags2 |= TR2_IM_COLD;
				break;
			}
			case 20: a_ptr->flags2 |= TR2_FREE_ACT; break;
			case 21: a_ptr->flags2 |= TR2_HOLD_LIFE; break;
			case 22: a_ptr->flags2 |= TR2_RES_ACID; break;
			case 23: a_ptr->flags2 |= TR2_RES_ELEC; break;
			case 24: a_ptr->flags2 |= TR2_RES_FIRE; break;
			case 25: a_ptr->flags2 |= TR2_RES_COLD; break;

			case 26: a_ptr->flags2 |= TR2_RES_POIS; break;
			case 27: a_ptr->flags2 |= TR2_RES_LITE; break;
			case 28: a_ptr->flags2 |= TR2_RES_DARK; break;
			case 29: a_ptr->flags2 |= TR2_RES_BLIND; break;
			case 30: a_ptr->flags2 |= TR2_RES_CONF; break;
			case 31: a_ptr->flags2 |= TR2_RES_SOUND; break;
			case 32: a_ptr->flags2 |= TR2_RES_SHARDS; break;
			case 33:
				if (rand_int (2) == 0)
					a_ptr->flags2 |= TR2_RES_NETHER;
				break;
			case 34: a_ptr->flags2 |= TR2_RES_NEXUS; break;
			case 35: a_ptr->flags2 |= TR2_RES_CHAOS; break;
			case 36:
				if (rand_int (2) == 0)
					a_ptr->flags2 |= TR2_RES_DISEN;
				break;
			case 37: a_ptr->flags3 |= TR3_FEATHER; break;
			case 38: a_ptr->flags3 |= TR3_LITE1; break;
			case 39: a_ptr->flags3 |= TR3_SEE_INVIS; break;
		        case 40:
#if 0
				if (rand_int (3) == 0)
//					a_ptr->flags3 |= TR3_TELEPATHY;
					a_ptr->esp |= (ESP_ALL);
#endif	// 0
				{
					int rr = rand_int (29);
					if (rr < 1) a_ptr->esp |= (ESP_ORC);
					else if (rr < 2) a_ptr->esp |= (ESP_TROLL);
					else if (rr < 3) a_ptr->esp |= (ESP_DRAGON);
					else if (rr < 4) a_ptr->esp |= (ESP_GIANT);
					else if (rr < 5) a_ptr->esp |= (ESP_DEMON);
					else if (rr < 8) a_ptr->esp |= (ESP_UNDEAD);
					else if (rr < 12) a_ptr->esp |= (ESP_EVIL);
					else if (rr < 14) a_ptr->esp |= (ESP_ANIMAL);
					else if (rr < 16) a_ptr->esp |= (ESP_DRAGONRIDER);
					else if (rr < 19) a_ptr->esp |= (ESP_GOOD);
					else if (rr < 21) a_ptr->esp |= (ESP_NONLIVING);
					else if (rr < 24) a_ptr->esp |= (ESP_UNIQUE);
					else if (rr < 26) a_ptr->esp |= (ESP_SPIDER);
					else a_ptr->esp |= (ESP_ALL);
					break;
				}
			case 41: a_ptr->flags3 |= TR3_SLOW_DIGEST; break;

			case 42:
				a_ptr->flags5 |= TR5_REGEN_MANA; break;
			case 43:
				a_ptr->flags3 |= TR3_REGEN; break;
			case 44:
				a_ptr->flags3 |= TR3_NO_MAGIC; break;
		}
	}

	/* Now remove contradictory or redundant powers. */
	remove_contradictory (a_ptr);
}




/*
 * Returns pointer to randart artifact_type structure.
 *
 * o_ptr should contain the seed (in name3) plus a tval
 * and sval. It returns NULL on illegal sval and tvals.
 */
artifact_type *randart_make(object_type *o_ptr)
{
	/*u32b activates;*/
	s32b power;
	int tries;
	s32b ap;
	bool curse_me = FALSE;
	bool aggravate_me = FALSE;
	
	/* Get pointer to our artifact_type object */
	a_ptr = &randart;

	/* Get pointer to object kind */
	k_ptr = &k_info[o_ptr->k_idx];

	/* Set the RNG seed. */
	Rand_value = o_ptr->name3;
	Rand_quick = TRUE;

	/* Screen for disallowed TVALS */
	if ((k_ptr->tval!=TV_BOW) &&
//	    (k_ptr->tval!=TV_DIGGING) &&	 /* better ban it? */
//	    (k_ptr->tval!=TV_TOOL) &&
//	    (k_ptr->tval!=TV_INSTRUMENT) &&
	    (k_ptr->tval != TV_SHOT) && (k_ptr->tval != TV_ARROW) && (k_ptr->tval != TV_BOLT) &&
	    (k_ptr->tval!=TV_HAFTED) &&
	    (k_ptr->tval!=TV_POLEARM) &&
	    (k_ptr->tval!=TV_SWORD) &&
	    (k_ptr->tval!=TV_BOOMERANG) &&
	    (k_ptr->tval!=TV_AXE) &&
	    (k_ptr->tval!=TV_MSTAFF) &&
	    (k_ptr->tval!=TV_BOOTS) &&
	    (k_ptr->tval!=TV_GLOVES) &&
	    (k_ptr->tval!=TV_HELM) &&
	    (k_ptr->tval!=TV_CROWN) &&
	    (k_ptr->tval!=TV_SHIELD) &&
	    (k_ptr->tval!=TV_CLOAK) &&
	    (k_ptr->tval!=TV_SOFT_ARMOR) &&
	    (k_ptr->tval!=TV_DRAG_ARMOR) &&
	    (k_ptr->tval!=TV_HARD_ARMOR) &&
	    (k_ptr->tval!=TV_RING) &&
	    (k_ptr->tval!=TV_LITE) &&
	    (k_ptr->tval!=TV_AMULET))
	{
		/* Not an allowed type */
		return(NULL);
	}

        /* Randart ammo doesn't keep (exploding) from normal item */
        if ((k_ptr->tval == TV_SHOT) || (k_ptr->tval == TV_ARROW) ||
            (k_ptr->tval == TV_BOLT)) k_ptr->pval = 0;

	/* Mega Hack -- forbig randart polymorph rings(pval would be BAD) */
	if ((k_ptr->tval == TV_RING) && (k_ptr->sval == SV_RING_POLYMORPH))
	  {
	    return (NULL);
	  }
	
	/* Wipe the artifact_type structure */
	WIPE(&randart, artifact_type);

	/* 
	 * First get basic artifact quality
	 * 90% are good
	 */
	if (rand_int(10))
	{
		power = rand_int(80) + RANDART_QUALITY;
	}
	/* 10% are cursed */
	else
	{
		power = rand_int(40) - (2*RANDART_QUALITY);
	}
	
	if (power < 0) curse_me = TRUE;
	
	/* Really powerful items should aggravate. */
	if (power > 100)
	{
		if (rand_int (100) < (power - 100) * 3)
		{
			aggravate_me = TRUE;
		}
	}
	
	if ((k_ptr->tval == TV_SHOT) ||
	    (k_ptr->tval == TV_ARROW) ||
	    (k_ptr->tval == TV_BOLT)) {
		aggravate_me = FALSE;
//		power /= 3;
	}


	/* Default values */
	a_ptr->cur_num = 0;
	a_ptr->max_num = 1;
	a_ptr->tval = k_ptr->tval;
	a_ptr->sval = k_ptr->sval;
	a_ptr->pval = k_ptr->pval;

	/* rings of speed would become too powerful otherwise: */
/*wrong:if (o_ptr->bpval) a_ptr->pval =	(o_ptr->pval < o_ptr->bpval)?
		o_ptr->bpval : o_ptr->pval;
	o_ptr->bpval = 0;*/

	if (k_ptr->tval == TV_LITE) a_ptr->pval = 0;
	/* Amulets and Rings keep their (+hit,+dam)[+AC] instead of having it
	   reset to (+0,+0)[+0] (since those boni are hard-coded for jewelry) */
	if ((a_ptr->tval == TV_AMULET) || (a_ptr->tval == TV_RING))
	{
		a_ptr->to_h = o_ptr->to_h;
		a_ptr->to_d = o_ptr->to_d;
		a_ptr->to_a = o_ptr->to_a;
	} else {
		a_ptr->to_h = k_ptr->to_h;
		a_ptr->to_d = k_ptr->to_d;
		a_ptr->to_a = k_ptr->to_a;
	}
	a_ptr->ac = k_ptr->ac;
	a_ptr->dd = k_ptr->dd;
	a_ptr->ds = k_ptr->ds;
	a_ptr->weight = k_ptr->weight;
	a_ptr->flags1 = k_ptr->flags1;
	a_ptr->flags2 = k_ptr->flags2;
	a_ptr->flags3 = k_ptr->flags3;
	a_ptr->flags3 |= (TR3_IGNORE_ACID | TR3_IGNORE_ELEC |
			   TR3_IGNORE_FIRE | TR3_IGNORE_COLD);
	a_ptr->flags4 = k_ptr->flags4;
	a_ptr->flags5 = k_ptr->flags5;
	a_ptr->flags5 |= TR5_IGNORE_WATER;

	/* Ensure weapons have some bonus to hit & dam */
	if ((a_ptr->tval==TV_DIGGING) ||
	    (a_ptr->tval==TV_HAFTED) ||
	    (a_ptr->tval==TV_POLEARM) ||
	    (a_ptr->tval==TV_SWORD) ||
	    (a_ptr->tval==TV_AXE) ||
	    (a_ptr->tval==TV_BOW))
	{
		a_ptr->to_d += 1 + rand_int(20);
		a_ptr->to_h += 1 + rand_int(20);
	}

	/* Ammo doesn't get large bonus */
	if ((a_ptr->tval==TV_SHOT) ||
	    (a_ptr->tval==TV_ARROW) ||
	    (a_ptr->tval==TV_BOLT))
	{
		/* exploding art ammo is very rare */
		if (magik(10)) {
			int power[28]={GF_ELEC, GF_POIS, GF_ACID,
			GF_COLD, GF_FIRE, GF_PLASMA, GF_LITE,
			GF_DARK, GF_SHARDS, GF_SOUND,
			GF_CONFUSION, GF_FORCE, GF_INERTIA,
			GF_MANA, GF_METEOR, GF_ICE, GF_CHAOS,
			GF_NETHER, GF_NEXUS, GF_TIME,
			GF_GRAVITY, GF_KILL_WALL, GF_AWAY_ALL,
			GF_TURN_ALL, GF_NUKE, GF_STUN,
			GF_DISINTEGRATE, GF_HELL_FIRE};
			a_ptr->pval = power[rand_int(28)];
		}

		a_ptr->to_d = 0;
		a_ptr->to_h = 0;
		if (magik(30)) {
			a_ptr->to_d += randint(3);
			a_ptr->to_h += randint(5);
		}
		if (magik(20)) {
			a_ptr->to_d += randint(3);
			a_ptr->to_h += randint(5);
		}
		if (magik(10)) a_ptr->ds += 1;
		else if (magik(10)) a_ptr->dd += 1;
	}
	
	/* Ensure armour has some bonus to ac */
	if ((k_ptr->tval==TV_BOOTS) ||
	    (k_ptr->tval==TV_GLOVES) ||
	    (k_ptr->tval==TV_HELM) ||
	    (k_ptr->tval==TV_CROWN) ||
	    (k_ptr->tval==TV_SHIELD) ||
	    (k_ptr->tval==TV_CLOAK) ||
	    (k_ptr->tval==TV_SOFT_ARMOR) ||
	    (k_ptr->tval==TV_DRAG_ARMOR) ||
	    (k_ptr->tval==TV_HARD_ARMOR))
	{
		a_ptr->to_a += 1 + rand_int(17);
	}
		
	/* First draft: add two abilities, then curse it three times. */
	if (curse_me)
	{
		add_ability (a_ptr);
		add_ability (a_ptr);
		do_curse (a_ptr);
		do_curse (a_ptr);
		do_curse (a_ptr);
		remove_contradictory (a_ptr);
		ap = artifact_power(a_ptr);
	}

	else if ((k_ptr->tval == TV_SHOT) ||
	    (k_ptr->tval == TV_ARROW) ||
	    (k_ptr->tval == TV_BOLT)) {
		add_ability (a_ptr);
		if (magik(50)) add_ability (a_ptr);
		if (magik(25)) add_ability (a_ptr);
		if (magik(10)) add_ability (a_ptr);
		ap = artifact_power (a_ptr) + RANDART_QUALITY + 15; /* in general ~5k+0+15k value */
	}
	else
	{
		/* Select a random set of abilities which roughly matches the
		   original's in terms of overall power/usefulness. */
		for (tries = 0; tries < MAX_TRIES; tries++)
		{
			artifact_type a_old;

			/* Copy artifact info temporarily. */
			a_old = *a_ptr;
			add_ability (a_ptr);
			ap = artifact_power (a_ptr);
			
			if (ap > (power * 11) / 10 + 1)
			{	/* too powerful -- put it back */
				*a_ptr = a_old;
				continue;
			}

			else if (ap >= (power * 9) / 10)	/* just right */
			{
				break;
			}

			/* Stop if we're going negative, so we don't overload
			   the artifact with great powers to compensate. */
			else if ((ap < 0) && (ap < (-(power * 1)) / 10))
			{
				break;
			}
		}		/* end of power selection */

		if (aggravate_me)
		{
			a_ptr->flags3 |= TR3_AGGRAVATE;
			remove_contradictory (a_ptr);
			ap = artifact_power (a_ptr);
		}
	}
	a_ptr->cost = (ap + 10 - RANDART_QUALITY) * (s32b)1500;
        a_ptr->level = (curse_me ? (ap < -20 ? -ap : (ap > 20 ? ap : 15 + ABS(ap))) : ap);

	if (a_ptr->cost < 0) a_ptr->cost = 0;

#if 0
	/* One last hack: if the artifact is very powerful, raise the rarity.
	   This compensates for artifacts like (original) Bladeturner, which
	   have low artifact rarities but came from extremely-rare base
	   kinds. */
	if ((ap > 0) && ((ap / 8) > a_ptr->rarity))
		a_ptr->rarity = ap / 8;
#endif /* if 0 */

	/*if (activates) a_ptr->flags3 |= TR3_ACTIVATE;*/
	/*if (a_idx < ART_MIN_NORMAL) a_ptr->flags3 |= TR3_INSTA_ART;*/

	/* Add TR3_HIDE_TYPE to all artifacts with nonzero pval because we're
	   too lazy to find out which ones need it and which ones don't. */
	if (a_ptr->pval) a_ptr->flags3 |= TR3_HIDE_TYPE;

        /* Fix some limits */
	/* Mage staves never have NO_MAGIC but their pval always adds to MANA */
	if (a_ptr->tval == TV_MSTAFF) {
		a_ptr->flags3 &= ~TR3_NO_MAGIC;
		if (a_ptr->pval) a_ptr->flags1 |= TR1_MANA;
	}
	/* Dark Swords never add to MANA */
	if (a_ptr->tval == TV_SWORD && a_ptr->sval == SV_DARK_SWORD) a_ptr->flags1 &= ~TR1_MANA;	
        if (a_ptr->flags1 & TR1_MANA)
	{
		/* Randart mage staves may give up to +10 +1 bonus MANA */
		if ((a_ptr->tval == TV_MSTAFF) && (a_ptr->pval > 11)) a_ptr->pval = 11;
		/* Helms and crowns may not give more than +3 MANA */
		else if ((a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN) &&
		    (a_ptr->pval > 3)) a_ptr->pval = 3;
		/* Usually +10 MANA is max */
                else if (a_ptr->pval > 10) a_ptr->pval = 10;
        }
	/* No more than +4 IV on helms and crowns */
	if ((a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN) &&
	    (a_ptr->flags1 & TR1_INFRA) && (a_ptr->pval > 4)) a_ptr->pval = 4;
	/* If an item increases SPEED _and_ CRIT by over 7
	   then reduce pval to 2/3 to balance */
	if ((a_ptr->flags1 & TR1_SPEED) && (a_ptr->flags5 & TR5_CRIT) &&
	    (a_ptr->pval > 7)) a_ptr->pval = (a_ptr->pval * 2) / 3;
	/* No more than +6 stealth */
	if ((a_ptr->flags1 & TR1_STEALTH) && (a_ptr->pval > 6)) a_ptr->pval = 6;
	/* No more than +5 luck */
	if ((a_ptr->flags5 & TR5_LUCK) && (a_ptr->pval > 5)) a_ptr->pval = 5;
	/* No more than +3 disarming ability (randart picklocks aren't allowed anyways) */
	if ((a_ptr->flags5 & TR5_DISARM) && (a_ptr->pval > 3)) a_ptr->pval = 3;
	/* Never increase stats too greatly */
	if (a_ptr->flags1 & (TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR))
	{
		int c=0; /* Count how many stats are increased */
		if (a_ptr->flags1 & TR1_STR) c++;
		if (a_ptr->flags1 & TR1_INT) c++;
		if (a_ptr->flags1 & TR1_WIS) c++;
		if (a_ptr->flags1 & TR1_DEX) c++;
		if (a_ptr->flags1 & TR1_CON) c++;
		if (a_ptr->flags1 & TR1_CHR) c++;
		/* limit +stats to 15 (3*(+5) or 5*(+3)),
		   never more than +3 on amulets */
		if ((a_ptr->tval == TV_AMULET) || (c > 3))
		{
			if (a_ptr->pval > 3) a_ptr->pval /= 2;
			if (a_ptr->pval > 3) a_ptr->pval = 3;
			if (a_ptr->pval == 0) a_ptr->pval = 1;
		} else {
			if (a_ptr->pval > 5) a_ptr->pval /= 2;
			if (a_ptr->pval > 5) a_ptr->pval = 5;
			if (a_ptr->pval == 0) a_ptr->pval = 1;
		}
	}
	/* Never more than +3 EA, +2 on gloves */
        if (a_ptr->flags1 & TR1_BLOWS)
        {
		if (a_ptr->tval == TV_GLOVES)
		{
			if (a_ptr->pval > 2) a_ptr->pval /= 3;
	                if (a_ptr->pval > 2) a_ptr->pval = 2;
		} else {
			if (a_ptr->pval > 3) a_ptr->pval /= 2;
	                if (a_ptr->pval > 3) a_ptr->pval = 3;
		}
		if (a_ptr->pval == 0) a_ptr->pval = 1;
        }
	/* Never more than +3 LIFE (doesn't occur on randarts anyways, yet) */
	if ((a_ptr->flags1 & TR1_LIFE) && (a_ptr->pval > 3)) a_ptr->pval = 3;
	/* Never more than +6 +hit/+dam on gloves, +30 in general */
	if (a_ptr->tval == TV_GLOVES) {
		if (a_ptr->to_h > 6) a_ptr->to_h = 6;
		if (a_ptr->to_d > 6) a_ptr->to_d = 6;
	} else {
		if (a_ptr->to_h > 30) a_ptr->to_h = 30;
		if (a_ptr->to_d > 30) a_ptr->to_d = 30;
	}

	/* Hack -- DarkSword randarts should have this */
	if ((a_ptr->tval == TV_SWORD) && (a_ptr->sval == SV_DARK_SWORD))
	{
		/* Remove all old ANTIMAGIC flags that might have been
		set by a curse or random ability */
		a_ptr->flags4 &= ((~TR4_ANTIMAGIC_30) & (~TR4_ANTIMAGIC_20) & (~TR4_ANTIMAGIC_10));
		
		/* Start with basic Antimagic */
		a_ptr->flags4 |= TR4_ANTIMAGIC_50;

		/* If they have large tohit/dam boni they can get more AM even */

		/* If +hit/+dam cancels out base AM.. */
		if ((a_ptr->to_h + a_ptr->to_d) >= 50)
		{
			/* Reduce +hit/+dam equally so it just cancels out 50% base AM */
			if (magik(50))
				while (a_ptr->to_h + a_ptr->to_d > 50) {
					a_ptr->to_h--;
					if (a_ptr->to_h + a_ptr->to_d > 50) a_ptr->to_d--;
				}
			/* Now add 0%..50% AM to receive -10%..50% AM in total;
			   (-10% from +30+30 -50% if +hit+dam wasn't reduced above) */
			if (magik(40)) a_ptr->flags4 |= TR4_ANTIMAGIC_30 | TR4_ANTIMAGIC_20;
			else if (magik(60)) a_ptr->flags4 |= TR4_ANTIMAGIC_30 | TR4_ANTIMAGIC_10;
			else if (magik(80)) a_ptr->flags4 |= TR4_ANTIMAGIC_30;
			else if (magik(90)) a_ptr->flags4 |= TR4_ANTIMAGIC_20;
			else if (magik(95)) a_ptr->flags4 |= TR4_ANTIMAGIC_10;
		}
		else if ((a_ptr->to_h + a_ptr->to_d) >= 40)
		{
			if (magik(50))
				while (a_ptr->to_h + a_ptr->to_d > 40) {
					a_ptr->to_h--;
					if (a_ptr->to_h + a_ptr->to_d > 40) a_ptr->to_d--;
				}

			if (magik(50)) a_ptr->flags4 |= TR4_ANTIMAGIC_30 | TR4_ANTIMAGIC_10;
			else if (magik(65)) a_ptr->flags4 |= TR4_ANTIMAGIC_30;
			else if (magik(80)) a_ptr->flags4 |= TR4_ANTIMAGIC_20;
			else if (magik(90)) a_ptr->flags4 |= TR4_ANTIMAGIC_10;
		}
		else if ((a_ptr->to_h + a_ptr->to_d) >= 30)
		{
			if (magik(50))
				while (a_ptr->to_h + a_ptr->to_d > 30) {
					a_ptr->to_h--;
					if (a_ptr->to_h + a_ptr->to_d > 30) a_ptr->to_d--;
				}

			if (magik(60)) a_ptr->flags4 |= TR4_ANTIMAGIC_30;
			else if (magik(75)) a_ptr->flags4 |= TR4_ANTIMAGIC_20;
			else if (magik(90)) a_ptr->flags4 |= TR4_ANTIMAGIC_10;
		}
		else if ((a_ptr->to_h + a_ptr->to_d) >= 20)
		{
			if (magik(50))
				while (a_ptr->to_h + a_ptr->to_d > 20) {
					a_ptr->to_h--;
					if (a_ptr->to_h + a_ptr->to_d > 20) a_ptr->to_d--;
				}

			if (magik(70)) a_ptr->flags4 |= TR4_ANTIMAGIC_20;
			else if (magik(85)) a_ptr->flags4 |= TR4_ANTIMAGIC_10;
		}
		else if ((a_ptr->to_h + a_ptr->to_d) >= 10)
		{
			if (magik(50))
			        while (a_ptr->to_h + a_ptr->to_d > 10) {
					a_ptr->to_h--;
					if (a_ptr->to_h + a_ptr->to_d > 10) a_ptr->to_d--;
				}

			if (magik(80)) a_ptr->flags4 |= TR4_ANTIMAGIC_10;
		}
	}
	
	/* Restore RNG */
	Rand_quick = FALSE;

	/* Return a pointer to the artifact_type */	
	return (a_ptr);
}


/*
 * Make random artifact name.
 */
void randart_name(object_type *o_ptr, char *buffer)
{
	char tmp[80];
	
	/* Set the RNG seed. It this correct. Should it be restored??? XXX */
	Rand_value = o_ptr->name3;
	Rand_quick = TRUE;

	/* Take a random name */
	get_rnd_line("randarts.txt", 0, tmp);

	/* Capitalise first character */
	tmp[0] = toupper(tmp[0]);
	
	/* Either "sword of something" or
	 * "sword 'something'" form */
	if (rand_int(2))
	{
		sprintf(buffer, "of %s", tmp);
	}
	else
	{
		sprintf(buffer, "'%s'", tmp);
	}
	
	/* Restore RNG */
	Rand_quick = FALSE;

	return;
}
	

/*
 * Here begins the code for new ego-items.		- Jir -
 * Powers of ego-items are determined from random seed
 * just like randarts, but is controlled by ego-flags.
 *
 * This code is the mixture of PernAngband ego-item system
 * and PernMangband randarts system(as you see above).
 *
 * Our system is more efficient in memory/file-size (all we
 * need is a s32b seed), but *less* efficient in executing
 * speed (we 'generate' egos each time they are used!)
 * than that of PernA.
 */

/*
 * Returns pointer to ego-item artifact_type structure.
 *
 * o_ptr should contain the seed (in name3) plus a tval
 * and sval.
 */
artifact_type *ego_make(object_type *o_ptr)
{
	ego_item_type *e_ptr;
	int j, rr, granted_pval;
	bool limit_blows = FALSE;
//	u32b f1, f2, f3, f4, f5, esp;
	s16b e_idx;

	/* Hack -- initialize bias */
	artifact_bias = 0;

	/* Get pointer to our artifact_type object */
	a_ptr = &randart;

	/* Get pointer to object kind */
	k_ptr = &k_info[o_ptr->k_idx];

	/* Set the RNG seed. */
	Rand_value = o_ptr->name3;
	Rand_quick = TRUE;

	/* Wipe the artifact_type structure */
	WIPE(&randart, artifact_type);

	e_idx = o_ptr->name2;

	/* Ok now, THAT is truly ugly */
try_an_other_ego:
	e_ptr = &e_info[e_idx];

	/* Hack -- extra powers */
	for (j = 0; j < 5; j++)
	{
		/* Rarity check */
		if (magik(e_ptr->rar[j]))
		{
			a_ptr->flags1 |= e_ptr->flags1[j];
			a_ptr->flags2 |= e_ptr->flags2[j];
			a_ptr->flags3 |= e_ptr->flags3[j];
			a_ptr->flags4 |= e_ptr->flags4[j];
			a_ptr->flags5 |= e_ptr->flags5[j];
			a_ptr->esp |= e_ptr->esp[j];
			add_random_ego_flag(a_ptr, e_ptr->fego[j], &limit_blows, o_ptr->level, o_ptr);
		}
	}
	
	/* Hack - Amulet of telepathic awareness (formerly of ESP) */
#if 1
	{
		/* Apply limited ESP powers, evaluated from R_ESP_ flags */
		if (a_ptr->esp & R_ESP_LOW) {
		//if (rand_int(100) < 25) {
			rr = rand_int(16) - 1;
			if (rr < 2) a_ptr->esp |= (ESP_ORC);
			else if (rr < 4) a_ptr->esp |= (ESP_TROLL);
	    		else if (rr < 6) a_ptr->esp |= (ESP_GIANT);
	    		else if (rr < 8) a_ptr->esp |= (ESP_ANIMAL);
			else if (rr < 10) a_ptr->esp |= (ESP_DRAGONRIDER);
			else if (rr < 12) a_ptr->esp |= (ESP_GOOD);
			else if (rr < 14) a_ptr->esp |= (ESP_NONLIVING);
			else a_ptr->esp |= (ESP_SPIDER);
			a_ptr->esp &= (~R_ESP_LOW);
		}
		if (a_ptr->esp & R_ESP_HIGH) {
		//if (rand_int(100) < 50) {
			rr = rand_int(10) - 1;
			if (rr < 2) a_ptr->esp |= (ESP_DRAGON);
			else if (rr < 4) a_ptr->esp |= (ESP_DEMON);
			else if (rr < 7) a_ptr->esp |= (ESP_UNDEAD);
			else if (rr < 8) a_ptr->esp |= (ESP_EVIL);
			else a_ptr->esp |= (ESP_UNIQUE);
			a_ptr->esp &= (~R_ESP_HIGH);
		}
		if (a_ptr->esp & R_ESP_ANY)
		{
			rr = rand_int(26) - 1;
			if (rr < 1) a_ptr->esp |= (ESP_ORC);
			else if (rr < 2) a_ptr->esp |= (ESP_TROLL);
			else if (rr < 3) a_ptr->esp |= (ESP_DRAGON);
			else if (rr < 4) a_ptr->esp |= (ESP_GIANT);
			else if (rr < 5) a_ptr->esp |= (ESP_DEMON);
			else if (rr < 8) a_ptr->esp |= (ESP_UNDEAD);
			else if (rr < 12) a_ptr->esp |= (ESP_EVIL);
			else if (rr < 14) a_ptr->esp |= (ESP_ANIMAL);
			else if (rr < 16) a_ptr->esp |= (ESP_DRAGONRIDER);
			else if (rr < 19) a_ptr->esp |= (ESP_GOOD);
			else if (rr < 21) a_ptr->esp |= (ESP_NONLIVING);
		        else if (rr < 24) a_ptr->esp |= (ESP_UNIQUE);
			else a_ptr->esp |= (ESP_SPIDER);
			a_ptr->esp &= (~R_ESP_ANY);
		}
		//if (rand_int(100) < 5) a_ptr->esp |= ESP_ALL;
		//if (a_ptr->esp & R_ESP_ALL)
	}
	if (a_ptr->esp & ESP_ALL) a_ptr->esp = ESP_ALL;
#endif

	/* Hack -- obtain bonuses */
	if (e_ptr->max_to_h > 0) a_ptr->to_h += randint(e_ptr->max_to_h);
	if (e_ptr->max_to_h < 0) a_ptr->to_h -= randint(-e_ptr->max_to_h);
	if (e_ptr->max_to_d > 0) a_ptr->to_d += randint(e_ptr->max_to_d);
	if (e_ptr->max_to_d < 0) a_ptr->to_d -= randint(-e_ptr->max_to_d);
	if (e_ptr->max_to_a > 0) a_ptr->to_a += randint(e_ptr->max_to_a);
	if (e_ptr->max_to_a < 0) a_ptr->to_a -= randint(-e_ptr->max_to_a);

	/* No insane number of blows */  
	if (limit_blows && (a_ptr->flags1 & TR1_BLOWS))
	{
		if (a_ptr->pval > 2)
			a_ptr->pval -= randint(a_ptr->pval - 2);
	}

	/* Obtain granted minimum pval */
	granted_pval = 0;
	if ((o_ptr->tval == TV_MSTAFF) && (o_ptr->sval == SV_MSTAFF))
	{
		if ((o_ptr->name2b == 2)||(o_ptr->name2 == 2)) granted_pval = 4;
		if ((o_ptr->name2b == 3)||(o_ptr->name2 == 3)) granted_pval = 7;
	}

	/* Hack -- obtain pval */
	if (e_ptr->max_pval > 0) a_ptr->pval += granted_pval + randint(e_ptr->max_pval - granted_pval);
	if (e_ptr->max_pval < 0) a_ptr->pval -= (granted_pval + randint(-e_ptr->max_pval - granted_pval));

	/* Hack -- apply rating bonus(it's done in apply_magic) */
	//		rating += e_ptr->rating;

#if 1	// double-ego code.. future pleasure :)
	if (o_ptr->name2b && (o_ptr->name2b != e_idx))
	{
		e_idx = o_ptr->name2b;
		goto try_an_other_ego;
	}
#endif	// 0

        /* Fix some limits */
	/* Mage Staves don't have NO_MAGIC */
	if (o_ptr->tval == TV_MSTAFF) a_ptr->flags3 &= ~TR3_NO_MAGIC;
	/* Dark Swords don't have MANA (or SPELL) flag */
	if (o_ptr->tval == TV_SWORD && o_ptr->sval == SV_DARK_SWORD)
	{
		a_ptr->flags1 &= ~TR1_MANA;
		a_ptr->flags1 &= ~TR1_SPELL;
	}
	/* Items of/with 'Magi'/'Istari' don't have NO_MAGIC: */
	if ((o_ptr->name2 == EGO_MAGI || o_ptr->name2b == EGO_MAGI) ||
	    (o_ptr->name2 == EGO_LITE_MAGI || o_ptr->name2b == EGO_LITE_MAGI) ||
	    (o_ptr->name2 == 135 || o_ptr->name2b == 135) ||
	    (o_ptr->name2 == 169 || o_ptr->name2b == 169) ||
	    (o_ptr->name2 == 185 || o_ptr->name2b == 185) ||
	    (o_ptr->name2 == 186 || o_ptr->name2b == 186)) a_ptr->flags3 &= ~TR3_NO_MAGIC;
        if (a_ptr->flags1 & TR1_MANA)
        {
                if (a_ptr->pval > 10) a_ptr->pval = 10;
        }
	if ((a_ptr->flags1 & TR1_SPEED) && (a_ptr->flags5 & TR5_CRIT))
	{
		a_ptr->pval = (a_ptr->pval * 2) / 3;
	}
	if(a_ptr->flags1 & TR1_STEALTH)
	{
		if (a_ptr->pval > 6) a_ptr->pval = 6;
	}
	if(a_ptr->flags5 & TR5_LUCK)
	{
		if (a_ptr->pval > 5) a_ptr->pval = 5;
	}
	if(a_ptr->flags5 & TR5_DISARM)
	{
		if (a_ptr->pval > 3) a_ptr->pval = 3;
	}
	if (a_ptr->flags1 & (TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR))
	{
		if (a_ptr->tval == TV_AMULET)
		{
			if (a_ptr->pval > 3) a_ptr->pval /= 2;
			if (a_ptr->pval > 3) a_ptr->pval = 3;
			if (a_ptr->pval == 0) a_ptr->pval = 1;
		} else {
			if (a_ptr->pval > 5) a_ptr->pval /= 2;
			if (a_ptr->pval > 5) a_ptr->pval = 5;
			if (a_ptr->pval == 0) a_ptr->pval = 1;
		}
	}
        if (a_ptr->flags1 & TR1_BLOWS)
        {
		if (o_ptr->tval == TV_GLOVES)
		{
			if (a_ptr->pval > 2) a_ptr->pval /= 3;
	                if (a_ptr->pval > 2) a_ptr->pval = 2;
		} else {
			if (a_ptr->pval > 3) a_ptr->pval /= 2;
	                if (a_ptr->pval > 3) a_ptr->pval = 3;
		}
		if (a_ptr->pval == 0) a_ptr->pval = 1;
        }
	if ((a_ptr->flags1 & TR1_LIFE) && (a_ptr->pval > 3))
	{
		a_ptr->pval = 3;
	}

	/* Restore RNG */
	Rand_quick = FALSE;

	/* Return a pointer to the artifact_type */	
	return (a_ptr);
}
static void add_random_esp(artifact_type *a_ptr, int all)
{
	int rr = rand_int (26 + all);
	if (rr < 1) a_ptr->esp |= (ESP_ORC);
	else if (rr < 2) a_ptr->esp |= (ESP_TROLL);
	else if (rr < 3) a_ptr->esp |= (ESP_DRAGON);
	else if (rr < 4) a_ptr->esp |= (ESP_GIANT);
	else if (rr < 5) a_ptr->esp |= (ESP_DEMON);
	else if (rr < 8) a_ptr->esp |= (ESP_UNDEAD);
	else if (rr < 12) a_ptr->esp |= (ESP_EVIL);
	else if (rr < 14) a_ptr->esp |= (ESP_ANIMAL);
	else if (rr < 16) a_ptr->esp |= (ESP_DRAGONRIDER);
	else if (rr < 19) a_ptr->esp |= (ESP_GOOD);
	else if (rr < 21) a_ptr->esp |= (ESP_NONLIVING);
	else if (rr < 24) a_ptr->esp |= (ESP_UNIQUE);
	else if (rr < 26) a_ptr->esp |= (ESP_SPIDER);
	else a_ptr->esp |= (ESP_ALL);
}

/* Add a random flag to the ego item */
/*
 * Hack -- 'dlev' is needed to determine some values;
 * I diverted lv-req of item for this purpose, thus changes in o_ptr->level
 * will result in changes of ego-item powers themselves!!	- Jir -
 */
// void add_random_ego_flag(object_type *o_ptr, int fego, bool *limit_blows)
void add_random_ego_flag(artifact_type *a_ptr, int fego, bool *limit_blows, s16b dlev, object_type *o_ptr)
{
	if (fego & ETR4_SUSTAIN)
	{
		/* Make a random sustain */
		switch(randint(6))
		{
			case 1: a_ptr->flags2 |= TR2_SUST_STR; break;
			case 2: a_ptr->flags2 |= TR2_SUST_INT; break;
			case 3: a_ptr->flags2 |= TR2_SUST_WIS; break;
			case 4: a_ptr->flags2 |= TR2_SUST_DEX; break;
			case 5: a_ptr->flags2 |= TR2_SUST_CON; break;
			case 6: a_ptr->flags2 |= TR2_SUST_CHR; break;
		}
	}

	if (fego & ETR4_OLD_RESIST)
	{
		/* Make a random resist, equal probabilities */ 
		switch (randint(11))
		{
			case  1: a_ptr->flags2 |= (TR2_RES_BLIND);  break;
			case  2: a_ptr->flags2 |= (TR2_RES_CONF);   break;
			case  3: a_ptr->flags2 |= (TR2_RES_SOUND);  break;
			case  4: a_ptr->flags2 |= (TR2_RES_SHARDS); break;
			case  5: a_ptr->flags2 |= (TR2_RES_NETHER); break;
			case  6: a_ptr->flags2 |= (TR2_RES_NEXUS);  break;
			case  7: a_ptr->flags2 |= (TR2_RES_CHAOS);  break;
			case  8: a_ptr->flags2 |= (TR2_RES_DISEN);  break;
			case  9: a_ptr->flags2 |= (TR2_RES_POIS);   break;
			case 10: a_ptr->flags2 |= (TR2_RES_DARK);   break;
			case 11: a_ptr->flags2 |= (TR2_RES_LITE);   break;
		}
	}

	if (fego & ETR4_ABILITY)
	{
		/* Choose an ability */
		switch (randint(10))
		{
			case 1: a_ptr->flags3 |= (TR3_FEATHER);     break;
			case 2: a_ptr->flags3 |= (TR3_LITE1);        break;
			case 3: a_ptr->flags3 |= (TR3_SEE_INVIS);   break;
//			case 4: a_ptr->esp |= (ESP_ALL);   break;
			case 4:
			{
				add_random_esp(a_ptr, 3);
				break;
			}
			case 5: a_ptr->flags3 |= (TR3_SLOW_DIGEST); break;
			case 6: a_ptr->flags3 |= (TR3_REGEN);       break;
			case 7: a_ptr->flags2 |= (TR2_FREE_ACT);    break;
			case 8: a_ptr->flags2 |= (TR2_HOLD_LIFE);   break;
			case 9: a_ptr->flags3 |= (TR3_NO_MAGIC);   break;
			case 10: a_ptr->flags5 |= (TR5_REGEN_MANA);   break;
		}
	}

	if (fego & ETR4_R_ELEM)
	{
		/* Make an acid/elec/fire/cold/poison resist */
		random_resistance(a_ptr, FALSE, randint(14) + 4);                                        
	}
	if (fego & ETR4_R_LOW)
	{
		/* Make an acid/elec/fire/cold resist */
		random_resistance(a_ptr, FALSE, randint(12) + 4);                                        
	}

	if (fego & ETR4_R_HIGH)
	{
		/* Make a high resist */
		random_resistance(a_ptr, FALSE, randint(22) + 16);                                       
	}
	if (fego & ETR4_R_ANY)
	{
		/* Make any resist */
		random_resistance(a_ptr, FALSE, randint(34) + 4);                                        
	}

	if (fego & ETR4_R_DRAGON)
	{
		/* Make "dragon resist" */
		dragon_resist(a_ptr);
	}

	if (fego & ETR4_SLAY_WEAP)
	{
		/* Make a Weapon of Slaying */

		if (randint(3) == 1) /* double damage */
		{
#if 0 /* isn't the following line buggy and meant to be..*/
			a_ptr->dd *= 2;
#else /* this instead?: */
			a_ptr->dd = o_ptr->dd * 2;
#endif
			while ((a_ptr->dd + o_ptr->dd) * (a_ptr->ds + o_ptr->ds) > ((a_ptr->flags4 & TR4_MUST2H)?75:50))
				a_ptr->dd -= 1; /* No overpowered slaying weapons */
			/* fix lower limit */
			if (a_ptr->dd < 0) a_ptr->dd = 0;
		} else {
			do
			{
				a_ptr->dd++;
			}
			while ((randint(a_ptr->dd) == 1) &&
				((a_ptr->dd + o_ptr->dd) * (a_ptr->ds + o_ptr->ds) < ((a_ptr->flags4 & TR4_MUST2H)?65:43)));
				/* No overpowered slaying weapons */
			do
			{
				a_ptr->ds++;
			}
			while ((randint(a_ptr->ds) == 1) &&
				((a_ptr->dd + o_ptr->dd) * (a_ptr->ds + o_ptr->ds) < (a_ptr->flags4 & TR4_MUST2H)?65:45));
				/* No overpowered slaying weapons */
		}
		if (randint(5) == 1)
		{
			a_ptr->flags1 |= TR1_BRAND_POIS;
		}
/*		if (a_ptr->tval == TV_SWORD && (randint(4) == 1))*/
		if ((a_ptr->tval != TV_HAFTED) && (randint(4) == 1))
		{
			a_ptr->flags5 |= TR5_VORPAL;
		}
	}

	if (fego & ETR4_DAM_DIE)
	{
		/* Increase damage dice */
		a_ptr->dd++;                             
	}

	if (fego & ETR4_DAM_SIZE)
	{
		/* Increase damage dice size */
		a_ptr->ds++; 
	}

	if (fego & ETR4_LIMIT_BLOWS)
	{
		/* Swap this flag */
		*limit_blows = !(*limit_blows);
	}

	if (fego & ETR4_PVAL_M1)
	{
		/* Increase pval */
		a_ptr->pval++;
	}

	if (fego & ETR4_PVAL_M2)
	{
		/* Increase pval */
		a_ptr->pval += m_bonus(2, dlev);
	}

	if (fego & ETR4_PVAL_M3)
	{
		/* Increase pval */
		a_ptr->pval += m_bonus(3, dlev);
	}

	if (fego & ETR4_PVAL_M5)
	{
		/* Increase pval */
		a_ptr->pval += m_bonus(5, dlev);
	}
	if (fego & ETR4_AC_M1)
	{
		/* Increase ac */
		a_ptr->to_a++;
	}

	if (fego & ETR4_AC_M2)
	{
		/* Increase ac */
		a_ptr->to_a += m_bonus(2, dlev);
	}

	if (fego & ETR4_AC_M3)
	{
		/* Increase ac */
		a_ptr->to_a += m_bonus(3, dlev);
	}

	if (fego & ETR4_AC_M5)
	{
		/* Increase ac */
		a_ptr->to_a += m_bonus(5, dlev);
	}
	if (fego & ETR4_TH_M1)
	{
		/* Increase to hit */
		a_ptr->to_h++;
	}

	if (fego & ETR4_TH_M2)
	{
		/* Increase to hit */
		a_ptr->to_h += m_bonus(2, dlev);
	}

	if (fego & ETR4_TH_M3)
	{
		/* Increase to hit */
		a_ptr->to_h += m_bonus(3, dlev);
	}

	if (fego & ETR4_TH_M5)
	{
		/* Increase to hit */
		a_ptr->to_h += m_bonus(5, dlev);
	}
#if 0
	if (fego & ETR4_TD_M1)
	{
		/* Increase to dam */
		a_ptr->to_d++;
	}

	if (fego & ETR4_TD_M2)
	{
		/* Increase to dam */
		a_ptr->to_d += m_bonus(2, dlev);
	}

#endif

	if (fego & ETR4_R_ESP)
	{
		add_random_esp(a_ptr, 1);
	}

	if (fego & ETR4_NO_SEED)
	{
		/* Nothing */
	}

	if (fego & ETR4_TD_M3)
	{
		/* Increase to dam */
		a_ptr->to_d += m_bonus(3, dlev);
	}

	if (fego & ETR4_TD_M5)
	{
		/* Increase to dam */
		a_ptr->to_d += m_bonus(5, dlev);
	}
	if (fego & ETR4_R_P_ABILITY)
	{
		/* Add a random pval-affected ability */
		/* This might cause boots with + to blows */
		switch (randint(6))
		{
			case 1:a_ptr->flags1 |= TR1_STEALTH; break;
			case 2:a_ptr->flags1 |= TR1_SEARCH; break;
			case 3:a_ptr->flags1 |= TR1_INFRA; break;
			case 4:a_ptr->flags1 |= TR1_TUNNEL; break;
			case 5:a_ptr->flags1 |= TR1_SPEED; break;
			case 6:a_ptr->flags1 |= TR1_BLOWS; break;                                            
		}

	}
	if (fego & ETR4_R_STAT)
	{
		/* Add a random stat */
		switch (randint(6))
		{
			case 1:a_ptr->flags1 |= TR1_STR; break;
			case 2:a_ptr->flags1 |= TR1_INT; break;
			case 3:a_ptr->flags1 |= TR1_WIS; break;
			case 4:a_ptr->flags1 |= TR1_DEX; break;
			case 5:a_ptr->flags1 |= TR1_CON; break;
			case 6:a_ptr->flags1 |= TR1_CHR; break;                                              
		}
	}
	if (fego & ETR4_R_STAT_SUST)
	{
		/* Add a random stat and sustain it */
		switch (randint(6))
		{
			case 1:
				{
					a_ptr->flags1 |= TR1_STR;
					a_ptr->flags2 |= TR2_SUST_STR;
					break;
				}

			case 2:
				{
					a_ptr->flags1 |= TR1_INT;
					a_ptr->flags2 |= TR2_SUST_INT;
					break;
				}

			case 3:
				{
					a_ptr->flags1 |= TR1_WIS;
					a_ptr->flags2 |= TR2_SUST_WIS;
					break;
				}

			case 4:
				{
					a_ptr->flags1 |= TR1_DEX;
					a_ptr->flags2 |= TR2_SUST_DEX;
					break;
				}

			case 5:
				{
					a_ptr->flags1 |= TR1_CON;
					a_ptr->flags2 |= TR2_SUST_CON;
					break;
				}
			case 6:
				{
					a_ptr->flags1 |= TR1_CHR;
					a_ptr->flags2 |= TR2_SUST_CHR;
					break;
				}                                                
		}
	}
	if (fego & ETR4_R_IMMUNITY)
	{
		/* Give a random immunity */
		switch (randint(4))
		{
			case 1:
				{
					a_ptr->flags2 |= TR2_IM_FIRE;
					a_ptr->flags3 |= TR3_IGNORE_FIRE;
					break;
				}
			case 2:
				{
					a_ptr->flags2 |= TR2_IM_ACID;
					a_ptr->flags3 |= TR3_IGNORE_ACID;
					a_ptr->flags5 |= TR5_IGNORE_WATER;
					break;
				}
			case 3:
				{
					a_ptr->flags2 |= TR2_IM_ELEC;
					a_ptr->flags3 |= TR3_IGNORE_ELEC;
					break;
				}
			case 4:
				{
					a_ptr->flags2 |= TR2_IM_COLD;
					a_ptr->flags3 |= TR3_IGNORE_COLD;
					break;
				}
		}
	}
}


/*
 * Borrowed from spells2.c of PernAngband.
 * erm.. btw.. what's that 'is_scroll'?		- Jir -
 */

#define WEIRD_LUCK      12
#define BIAS_LUCK       20
/*
 * Bias luck needs to be higher than weird luck,
 * since it is usually tested several times...
 */

//void random_resistance (object_type * o_ptr, bool is_scroll, int specific)
void random_resistance (artifact_type * a_ptr, bool is_scroll, int specific)
{
  if (!specific) /* To avoid a number of possible bugs */
  {
    if (artifact_bias == BIAS_ACID)
    {
	if (!(a_ptr->flags2 & TR2_RES_ACID))
	{
	    a_ptr->flags2 |= TR2_RES_ACID;
	    if (randint(2)==1) return;
	}
    if (randint(BIAS_LUCK)==1 && !(a_ptr->flags2 & TR2_IM_ACID))
	{
	    a_ptr->flags2 |= TR2_IM_ACID;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_ELEC)
    {
	if (!(a_ptr->flags2 & TR2_RES_ELEC))
	{
	    a_ptr->flags2 |= TR2_RES_ELEC;
	    if (randint(2)==1) return;
	}
    if (a_ptr->tval >= TV_CLOAK && a_ptr->tval <= TV_HARD_ARMOR &&
        ! (a_ptr->flags3 & TR3_SH_ELEC))
        {
            a_ptr->flags2 |= TR3_SH_ELEC;
            if (randint(2)==1) return;
        }
    if (randint(BIAS_LUCK)==1 && !(a_ptr->flags2 & TR2_IM_ELEC))
	{
	    a_ptr->flags2 |= TR2_IM_ELEC;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_FIRE)
    {
	if (!(a_ptr->flags2 & TR2_RES_FIRE))
	{
	    a_ptr->flags2 |= TR2_RES_FIRE;
	    if (randint(2)==1) return;
	}
    if (a_ptr->tval >= TV_CLOAK && a_ptr->tval <= TV_HARD_ARMOR &&
        ! (a_ptr->flags3 & TR3_SH_FIRE))
        {
            a_ptr->flags2 |= TR3_SH_FIRE;
            if (randint(2)==1) return;
        }
    if (randint(BIAS_LUCK)==1 && !(a_ptr->flags2 & TR2_IM_FIRE))
	{
	    a_ptr->flags2 |= TR2_IM_FIRE;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_COLD)
    {
	if (!(a_ptr->flags2 & TR2_RES_COLD))
	{
	    a_ptr->flags2 |= TR2_RES_COLD;
	    if (randint(2)==1) return;
	}
    if (randint(BIAS_LUCK)==1 && !(a_ptr->flags2 & TR2_IM_COLD))
	{
	    a_ptr->flags2 |= TR2_IM_COLD;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_POIS)
    {
	if (!(a_ptr->flags2 & TR2_RES_POIS))
	{
	    a_ptr->flags2 |= TR2_RES_POIS;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_WARRIOR)
    {
	if (randint(3)!=1 && (!(a_ptr->flags2 & TR2_RES_FEAR)))
	{
	    a_ptr->flags2 |= TR2_RES_FEAR;
	    if (randint(2)==1) return;
	}
    if ((randint(3)==1) && (!(a_ptr->flags3 & TR3_NO_MAGIC)))
    {
        a_ptr->flags3 |= TR3_NO_MAGIC;
        if (randint(2)==1) return;
    }
    }
    else if (artifact_bias == BIAS_NECROMANTIC)
    {
	if (!(a_ptr->flags2 & TR2_RES_NETHER))
	{
	    a_ptr->flags2 |= TR2_RES_NETHER;
	    if (randint(2)==1) return;
	}
	if (!(a_ptr->flags2 & TR2_RES_POIS))
	{
	    a_ptr->flags2 |= TR2_RES_POIS;
	    if (randint(2)==1) return;
	}
	if (!(a_ptr->flags2 & TR2_RES_DARK))
	{
	    a_ptr->flags2 |= TR2_RES_DARK;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_CHAOS)
    {
	if (!(a_ptr->flags2 & TR2_RES_CHAOS))
	{
	    a_ptr->flags2 |= TR2_RES_CHAOS;
	    if (randint(2)==1) return;
	}
	if (!(a_ptr->flags2 & TR2_RES_CONF))
	{
	    a_ptr->flags2 |= TR2_RES_CONF;
	    if (randint(2)==1) return;
	}
	if (!(a_ptr->flags2 & TR2_RES_DISEN))
	{
	    a_ptr->flags2 |= TR2_RES_DISEN;
	    if (randint(2)==1) return;
	}
    }
  }

    switch (specific?specific:randint(41))
    {
    case 1:
    if (randint(WEIRD_LUCK)!=1)
        random_resistance(a_ptr, is_scroll, specific);
	else
	{
	a_ptr->flags2 |= TR2_IM_ACID;
/*  if (is_scroll) msg_print("It looks totally incorruptible."); */
	if (!(artifact_bias))
	    artifact_bias = BIAS_ACID;
	}
	break;
    case 2:
    if (randint(WEIRD_LUCK)!=1)
	    random_resistance(a_ptr, is_scroll, specific);
	else
	{
	a_ptr->flags2 |= TR2_IM_ELEC;
/*  if (is_scroll) msg_print("It looks completely grounded."); */
	if (!(artifact_bias))
	    artifact_bias = BIAS_ELEC;
	}
	break;
    case 3:
    if (randint(WEIRD_LUCK)!=1)
	    random_resistance(a_ptr, is_scroll, specific);
	else
	{
	a_ptr->flags2 |= TR2_IM_COLD;
/*  if (is_scroll) msg_print("It feels very warm."); */
	if (!(artifact_bias))
	    artifact_bias = BIAS_COLD;
	}
	break;
    case 4:
    if (randint(WEIRD_LUCK)!=1)
	    random_resistance(a_ptr, is_scroll, specific);
	else
	{
	a_ptr->flags2 |= TR2_IM_FIRE;
/*  if (is_scroll) msg_print("It feels very cool."); */
	if (!(artifact_bias))
	    artifact_bias = BIAS_FIRE;
	}
	break;
    case 5: case 6: case 13:
	a_ptr->flags2 |= TR2_RES_ACID;
/*  if (is_scroll) msg_print("It makes your stomach rumble."); */
	if (!(artifact_bias))
	    artifact_bias = BIAS_ACID;
	break;
    case 7: case 8: case 14:
	a_ptr->flags2 |= TR2_RES_ELEC;
/*  if (is_scroll) msg_print("It makes you feel grounded."); */
    if (!(artifact_bias))
	    artifact_bias = BIAS_ELEC;
	break;
    case 9: case 10: case 15:
	a_ptr->flags2 |= TR2_RES_FIRE;
/*  if (is_scroll) msg_print("It makes you feel cool!");*/
	if (!(artifact_bias))
	    artifact_bias = BIAS_FIRE;
	break;
    case 11: case 12: case 16:
	a_ptr->flags2 |= TR2_RES_COLD;
/*  if (is_scroll) msg_print("It makes you feel full of hot air!");*/
	if (!(artifact_bias))
	    artifact_bias = BIAS_COLD;
	break;
    case 17: case 18:
	a_ptr->flags2 |= TR2_RES_POIS;
/*  if (is_scroll) msg_print("It makes breathing easier for you."); */
	if (!(artifact_bias) && randint(4)!=1)
	    artifact_bias = BIAS_POIS;
	else if (!(artifact_bias) && randint(2)==1)
	    artifact_bias = BIAS_NECROMANTIC;
	else if (!(artifact_bias) && randint(2)==1)
	    artifact_bias = BIAS_ROGUE;
	break;
    case 19: case 20:
	a_ptr->flags2 |= TR2_RES_FEAR;
/*  if (is_scroll) msg_print("It makes you feel brave!"); */
	if (!(artifact_bias) && randint(3)==1)
	    artifact_bias = BIAS_WARRIOR;
	break;
    case 21:
	a_ptr->flags2 |= TR2_RES_LITE;
/*  if (is_scroll) msg_print("It makes everything look darker.");*/
	break;
    case 22:
	a_ptr->flags2 |= TR2_RES_DARK;
/*  if (is_scroll) msg_print("It makes everything look brigher.");*/
	break;
    case 23: case 24:
	a_ptr->flags2 |= TR2_RES_BLIND;
/*  if (is_scroll) msg_print("It makes you feel you are wearing glasses.");*/
	break;
    case 25: case 26:
	a_ptr->flags2 |= TR2_RES_CONF;
/*  if (is_scroll) msg_print("It makes you feel very determined.");*/
	if (!(artifact_bias) && randint(6)==1)
	    artifact_bias = BIAS_CHAOS;
	break;
    case 27: case 28:
	a_ptr->flags2 |= TR2_RES_SOUND;
/*  if (is_scroll) msg_print("It makes you feel deaf!");*/
	break;
    case 29: case 30:
	a_ptr->flags2 |= TR2_RES_SHARDS;
/*  if (is_scroll) msg_print("It makes your skin feel thicker.");*/
	break;
    case 31: case 32:
	a_ptr->flags2 |= TR2_RES_NETHER;
/*  if (is_scroll) msg_print("It makes you feel like visiting a graveyard!");*/
	if (!(artifact_bias) && randint(3)==1)
	    artifact_bias = BIAS_NECROMANTIC;
	break;
    case 33: case 34:
	a_ptr->flags2 |= TR2_RES_NEXUS;
/*  if (is_scroll) msg_print("It makes you feel normal.");*/
	break;
    case 35: case 36:
	a_ptr->flags2 |= TR2_RES_CHAOS;
/*  if (is_scroll) msg_print("It makes you feel very firm.");*/
	if (!(artifact_bias) && randint(2)==1)
	    artifact_bias = BIAS_CHAOS;
	break;
    case 37: case 38:
	a_ptr->flags2 |= TR2_RES_DISEN;
/*  if (is_scroll) msg_print("It is surrounded by a static feeling.");*/
	break;
    case 39:
    if (a_ptr->tval >= TV_CLOAK && a_ptr->tval <= TV_HARD_ARMOR)
        a_ptr->flags3 |= TR3_SH_ELEC;
    else
	    random_resistance(a_ptr, is_scroll, specific);
    if (!(artifact_bias))
	    artifact_bias = BIAS_ELEC;
    break;
    case 40:
    if (a_ptr->tval >= TV_CLOAK && a_ptr->tval <= TV_HARD_ARMOR)
        a_ptr->flags3 |= TR3_SH_FIRE;
    else
	    random_resistance(a_ptr, is_scroll, specific);
    if (!(artifact_bias))
        artifact_bias = BIAS_FIRE;
    break;
    case 41:
    if (a_ptr->tval == TV_SHIELD || a_ptr->tval == TV_CLOAK ||
        a_ptr->tval == TV_HELM || a_ptr->tval == TV_HARD_ARMOR)
        a_ptr->flags5 |= TR5_REFLECT;
    else
	    random_resistance(a_ptr, is_scroll, specific);
    break;
    }
}

/* Borrowed from object2.c of PernAngband w/o even knowing
 * what it is :)		- Jir -
 */

void dragon_resist(artifact_type * a_ptr)
{
	do
	{
		artifact_bias = 0;

		if (randint(4)==1)
			random_resistance(a_ptr, FALSE, ((randint(14))+4));
		else
			random_resistance(a_ptr, FALSE, ((randint(22))+16));
	}
	while (randint(2)==1);
}

