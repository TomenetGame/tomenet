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

/* added this for consistency in some (unrelated) header-inclusion,
   it IS a server file, isn't it? */
#define SERVER

#include "angband.h"


/* Allow ego 'of slaying' weapons of top 2h types (SoS,MoD,TA) to gain dice or sides at all? */
#define EGO_TOP_WEAP_SLAY

/* Allow randart weapons of top 2h types (SoS,MoD,TA) to gain dice or sides at all?
   NOTE: Such randart weapons could surpass Grond even in the hands of the 6-base-bpr class 'Warrior'.
         If this is disabled, then Grond will surpass randarts even for 5-base-bpr classes.
         It is probably most sensible to keep this disabled. */
//#define RANDART_TOP_WEAP_SLAY


#ifndef EGO_TOP_WEAP_SLAY
 /* 42 cut executioner's sword some slack: allow 6d6/5d7. 45 allows 5d8 too. */
 #define slay_limit_ego(a_ptr, k_ptr)\
     (((k_ptr)->flags4 & TR4_MUST2H) ? ((a_ptr)->tval == TV_SWORD ? 42 : 54) : (((k_ptr)->flags4 & TR4_SHOULD2H) ? 42 : 30))
#else
 /* 55 allows 11d4 scythe of slicing, 60 allows 6d9 TA/MoD, 63 allows 7d8 TA/MoD */
 #define slay_limit_ego(a_ptr, k_ptr) \
     (((k_ptr)->flags4 & TR4_MUST2H) ? ((a_ptr)->tval == TV_SWORD ? 45 : 63) : (((k_ptr)->flags4 & TR4_SHOULD2H) ? 42 : 30))
#endif

#ifndef RANDART_TOP_WEAP_SLAY
 /* 42 cut executioner's sword some slack: allow 6d6/5d7. 45 allows 5d8 too. */
 #define slay_limit_randart(a_ptr, k_ptr)\
     (((k_ptr)->flags4 & TR4_MUST2H) ? ((a_ptr)->tval == TV_SWORD ? 42 : 54) : (((k_ptr)->flags4 & TR4_SHOULD2H) ? 42 : 30))
#else
 /* 55 allows 11d4 scythe of slicing, 60 allows 6d9 TA/MoD, 63 allows 7d8 TA/MoD */
 #define slay_limit_randart(a_ptr, k_ptr) \
     (((k_ptr)->flags4 & TR4_MUST2H) ? ((a_ptr)->tval == TV_SWORD ? 45 : 63) : (((k_ptr)->flags4 & TR4_SHOULD2H) ? 42 : 30))
#endif


/* How much power/curses is/are assigned to randarts.
   With [40] default value, randarts can have up to
   130 power ((40+79)*11/10), wich means even the top
   randarts still have a slim chance (10 out of 99)
   to not get AGGRAVATE - C. Blue */
#define RANDART_QUALITY 40

/* How many attempts to add abilities */
#define MAX_TRIES 200


#define abs(x)	((x) > 0 ? (x) : (-(x)))
#define sign(x)	((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))


/* Artifact/ego item return structure */
static artifact_type randart;

#if 0 /*unused*/
/*
 * Calculate the multiplier we'll get with a given bow type.
 * This is done differently in 2.8.2 than it was in 2.8.1.
 */
static int bow_multiplier (int sval) {
	switch (sval) {
	case SV_SLING: case SV_SHORT_BOW: return 2;
	case SV_LONG_BOW: case SV_LIGHT_XBOW: return 3;
	case SV_HEAVY_XBOW: return 4;
	  /*		default: msg_format ("Illegal bow sval %s\n", sval); */
	}
	return 0;
}
#endif

/*
 * We've just added an ability which uses the pval bonus.
 * Make sure it's not zero.  If it's currently negative, leave
 * it negative (heh heh).
 */
static void do_pval (artifact_type *a_ptr) {
	/* Add some pval */
	if (a_ptr->pval == 0)
		a_ptr->pval = 1 + rand_int(3);
	/* Cursed -- make it worse! */
	else if (a_ptr->pval < 0) {
		if (rand_int(2) == 0) a_ptr->pval--;
	}
	/* Bump up an existing pval */
	else if (rand_int(3) > 0)
		a_ptr->pval++;
	/* Done */
	return;
}


/*
 * Make it bad, or if it's already bad, make it worse!
 */
static void do_curse (artifact_type *a_ptr) {
	/* Some chance of picking up these flags */
	if (rand_int(3) == 0) a_ptr->flags3 |= TR3_AGGRAVATE;
	if (!is_ammo(a_ptr->tval)) {
		if (rand_int(5) == 0) a_ptr->flags3 |= TR3_DRAIN_EXP;
		if (rand_int(8) == 0) a_ptr->flags5 |= TR5_DRAIN_MANA;
		if (rand_int(11) == 0) a_ptr->flags5 |= TR5_DRAIN_HP;
		if (rand_int(7) == 0) a_ptr->flags3 |= TR3_TELEPORT;
	}

	/* Some chance or reversing good bonuses */
	if (!is_ammo(a_ptr->tval) && (a_ptr->pval > 0) && (rand_int(2) == 0)) a_ptr->pval = -a_ptr->pval;
	if ((a_ptr->to_a > 0) && (rand_int(2) == 0)) a_ptr->to_a = -a_ptr->to_a;
	if ((a_ptr->to_h > 0) && (rand_int(2) == 0)) a_ptr->to_h = -a_ptr->to_h;
	if ((a_ptr->to_d > 0) && (rand_int(4) == 0)) a_ptr->to_d = -a_ptr->to_d;

	/* Some chance of making bad bonuses worse */
	if (!is_ammo(a_ptr->tval) && (a_ptr->pval < 0) && (rand_int(2) == 0)) a_ptr->pval -= rand_int(2);
	if ((a_ptr->to_a < 0) && (rand_int(2) == 0)) a_ptr->to_a -= 3 + rand_int(10);
	if ((a_ptr->to_h < 0) && (rand_int(2) == 0)) a_ptr->to_h -= 3 + rand_int(6);
	if ((a_ptr->to_d < 0) && (rand_int(4) == 0)) a_ptr->to_d -= 3 + rand_int(6);

	/* If it is cursed, we can heavily curse it */
	if (a_ptr->flags3 & TR3_CURSED)
	{
		if (rand_int(2) == 0) a_ptr->flags3 |= TR3_HEAVY_CURSE;
		if (rand_int(15) == 0) a_ptr->flags4 |= TR4_CURSE_NO_DROP;
		return;
	}

	/* Always light curse the item */
	a_ptr->flags3 |= TR3_CURSED;
}

/*
 * Evaluate the artifact's overall power level.
 */
#define AP_JEWELRY_COMBAT /* helps +dam/+hit/+ac rings/amulets a bit, causing those values to not factor in. - C. Blue */
s32b artifact_power(artifact_type *a_ptr) { //Kurzel
	object_kind *k_ptr = &k_info[lookup_kind(a_ptr->tval, a_ptr->sval)];
	s32b p = 1;
	int immunities = 0, i;//, mult;

	/* Evaluate certain abilities based on type of object. */
	switch (a_ptr->tval) {
	case TV_BOW:
		//mult = bow_multiplier (a_ptr->sval);
		if (a_ptr->flags3 & TR3_XTRA_MIGHT) p += 30;
		if (a_ptr->flags3 & TR3_XTRA_SHOTS) p += 20;

		p += (a_ptr->to_h + 3 * sign(a_ptr->to_h)) / 4;
		p += (a_ptr->to_d + sign(a_ptr->to_d)) / 2;

		if (a_ptr->weight < k_ptr->weight) p++;
		break;
	case TV_DIGGING: //not possible atm
		p += 40;
		/* fall through! */
	case TV_BOOMERANG:
		if (a_ptr->flags3 & TR3_XTRA_SHOTS) p += 20;
		/* fall through! */
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_AXE:
		if (a_ptr->flags1 & TR1_KILL_DRAGON) p += 4;
		if (a_ptr->flags1 & TR1_KILL_DEMON) p += 4;
		if (a_ptr->flags1 & TR1_KILL_UNDEAD) p += 4;

		if (a_ptr->flags1 & TR1_SLAY_EVIL) p += 4;
		if (a_ptr->flags1 & TR1_SLAY_ANIMAL) p += 2;
		if (a_ptr->flags1 & TR1_SLAY_UNDEAD) p += 3;
		if (a_ptr->flags1 & TR1_SLAY_DRAGON) p += 3;
		if (a_ptr->flags1 & TR1_SLAY_DEMON) p += 3;
		if (a_ptr->flags1 & TR1_SLAY_TROLL) p += 1;
		if (a_ptr->flags1 & TR1_SLAY_ORC) p += 1;
		if (a_ptr->flags1 & TR1_SLAY_GIANT) p += 1;

		if (a_ptr->flags1 & TR1_BRAND_ACID) p += 4;
		if (a_ptr->flags1 & TR1_BRAND_ELEC) p += 4;
		if (a_ptr->flags1 & TR1_BRAND_FIRE) p += 3;
		if (a_ptr->flags1 & TR1_BRAND_COLD) p += 3;

		if (a_ptr->flags1 & TR1_BLOWS) p += (a_ptr->pval) * 6;
		if (a_ptr->flags1 & TR1_LIFE) p += (a_ptr->pval) * 7;

		if ((a_ptr->flags1 & TR1_TUNNEL) &&
		    (a_ptr->tval != TV_DIGGING))
			p += a_ptr->pval * 3;

		if (a_ptr->flags5 & TR5_VORPAL) p += 20;

		/* Instead of formerly base dd/ds, only increased dd/ds add to ap now.
		   This is less penalizing on 2h-weapons, making it fairer - C. Blue */
//		p += ((a_ptr->dd - k_ptr->dd + 1) * (a_ptr->ds - k_ptr->ds + 1) - 1) * 2;
		p += (a_ptr->dd * (a_ptr->ds + 1) - k_ptr->dd * (k_ptr->ds + 1)) * 1;

		/* Remember, weight is in 0.1 lb. units. */
		if (a_ptr->weight != k_ptr->weight)
			p += (k_ptr->weight - a_ptr->weight) / 20;

#ifndef AP_JEWELRY_COMBAT
		/* fall through! */
	case TV_RING:
	case TV_AMULET:
#endif
		p += (a_ptr->to_d + 2 * sign (a_ptr->to_d)) / 3;
		if (a_ptr->to_d > 15) p += (a_ptr->to_d - 14) / 2;

		p += (a_ptr->to_h + 3 * sign (a_ptr->to_h)) / 4;

		break;
	case TV_MSTAFF:	// maybe this needs another entry
		if (a_ptr->flags1 & TR1_LIFE) p += (a_ptr->pval) * 7;

		/* Remember, weight is in 0.1 lb. units. */
		if (a_ptr->weight != k_ptr->weight)
			p += (k_ptr->weight - a_ptr->weight) / 20;
		break;
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
		if (a_ptr->flags1 & TR1_BLOWS) p += (a_ptr->pval) * 8;
		if (a_ptr->flags1 & TR1_LIFE) p += (a_ptr->pval) * 10;

		if (a_ptr->flags1 & TR1_SLAY_EVIL) p += 15;
		if (a_ptr->flags1 & TR1_SLAY_ANIMAL) p += 10;
		if (a_ptr->flags1 & TR1_SLAY_UNDEAD) p += 13;
		if (a_ptr->flags1 & TR1_SLAY_DRAGON) p += 15;
		if (a_ptr->flags1 & TR1_SLAY_DEMON) p += 15;
		if (a_ptr->flags1 & TR1_SLAY_TROLL) p += 10;
		if (a_ptr->flags1 & TR1_SLAY_ORC) p += 8;
		if (a_ptr->flags1 & TR1_SLAY_GIANT) p += 6;

		if (a_ptr->flags1 & TR1_BRAND_ACID) p += 15;
		if (a_ptr->flags1 & TR1_BRAND_ELEC) p += 13;
		if (a_ptr->flags1 & TR1_BRAND_FIRE) p += 11;
		if (a_ptr->flags1 & TR1_BRAND_COLD) p += 10;

#if 0 /* hurts heavy armour */
		/* allow base ac to factor in somewhat */
		p += (a_ptr->ac + 4 * sign (a_ptr->ac)) / 5;
#endif
		p += (a_ptr->to_h + sign(a_ptr->to_h)) / 2;
		p += (a_ptr->to_d + sign(a_ptr->to_d)) / 2;
		if (a_ptr->weight != k_ptr->weight)
			p += (k_ptr->weight - a_ptr->weight) / 30;
		break;
	case TV_LITE:
//		p += 35;
		p += 25;
#if 0 /* we're already subtracting 10 from p for FUEL_LITE flag presence, further below. Also this is unfair for wooden torch/brass lantern. */
		if (!(a_ptr->flags4 & TR4_FUEL_LITE) &&
		    (k_ptr->flags4 & TR4_FUEL_LITE))
			p += 10;
#endif
		break;
	case TV_RING:
	case TV_AMULET:
		p += 20;
		/* hack -- Nazgul rings */
		if (a_ptr->sval == SV_RING_SPECIAL) p += 20;
		break;
	}

	/* Other abilities are evaluated independent of the object type. */

	/* Two notes regarding jewelry, which might be improved but aren't really important:
	   1) +AC is counted even for rings/amulets, although those retain their +AC as randarts.
	   2) We don't take into account +hit/+dam here for rings/amulets (see above, end of weapons), although we do for their +AC.
	   - C. Blue */
#ifdef AP_JEWELRY_COMBAT
	if (a_ptr->tval != TV_RING && a_ptr->tval != TV_AMULET
 #ifdef NEW_SHIELDS_NO_AC
	    && a_ptr->tval != TV_SHIELD
 #endif
	    ) {
		i = a_ptr->to_a - 10 - k_ptr->to_a / 2 - k_ptr->level / 15;
		p += (i + 3 * sign(i)) / 4;
		if (i > 20) p += (i - 20) / 2;
 #ifndef TO_AC_CAP_30
		if (a_ptr->to_a > 30) p += (a_ptr->to_a - 30) / 2; /* always costly */
		if (a_ptr->to_a > 35) p += 20000;	/* inhibit */
 #else
		if (a_ptr->to_a > 25) p += (a_ptr->to_a - 25) / 2; /* always costly */
		if (a_ptr->to_a > 30) p += 20000;	/* inhibit */
 #endif
	}
#endif

	if (a_ptr->pval > 0) {
		if (a_ptr->flags1 & TR1_STR) p += a_ptr->pval * 2;
		if (a_ptr->flags1 & TR1_INT) p += a_ptr->pval * 2;
		if (a_ptr->flags1 & TR1_WIS) p += a_ptr->pval * 2;
		if (a_ptr->flags1 & TR1_DEX) p += a_ptr->pval * 2;
		if (a_ptr->flags1 & TR1_CON) p += a_ptr->pval * 2;
		if (a_ptr->flags1 & TR1_STEALTH) p += a_ptr->pval * 4;
#ifdef ART_WITAN_STEALTH
		else if (a_ptr->tval == TV_BOOTS && a_ptr->sval == SV_PAIR_OF_WITAN_BOOTS) p += -2 * 4;
#endif
		if (a_ptr->flags1 & TR1_SEARCH) p += a_ptr->pval * 2;
	} else if (a_ptr->pval < 0) {	/* hack: don't give large negatives */
		if (a_ptr->flags1 & TR1_STR) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_INT) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_WIS) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_DEX) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_CON) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_STEALTH) p += a_ptr->pval;
#ifdef ART_WITAN_STEALTH
		else if (a_ptr->tval == TV_BOOTS && a_ptr->sval == SV_PAIR_OF_WITAN_BOOTS) p += -2 * 4;
#endif
		if (a_ptr->flags1 & TR1_SEARCH) p += a_ptr->pval;
	}
	if (a_ptr->flags1 & TR1_CHR) p += a_ptr->pval;
	if (a_ptr->flags1 & TR1_INFRA) p += (a_ptr->pval + sign (a_ptr->pval)) / 2;

	i = ((a_ptr->pval + 6) / 3);
	//i = i * i;
	//i = (i * 5) / 2;
#if 0 /* although 62/133 looks reasonable on paper, it produced mostly verylow..low pvals, and alot aggr boots */
	if (a_ptr->flags1 & TR1_SPEED) p += i * 12;
	if (a_ptr->flags1 & TR1_MANA) p += i * 10;
	if (a_ptr->flags5 & TR5_CRIT) p += i * 8;
#else
	if (a_ptr->flags1 & TR1_SPEED) p += i * 10;
	if (a_ptr->flags1 & TR1_MANA) p += i * 9;
	if (a_ptr->flags5 & TR5_CRIT) p += i * 8;
#endif

#if 0 //enable me? :/
	if (a_ptr->flags5 & TR5_LUCK) p += a_ptr->pval + 5;
#endif

	if (a_ptr->flags2 & TR2_SUST_STR) p += 5;
	if (a_ptr->flags2 & TR2_SUST_INT) p += 4;
	if (a_ptr->flags2 & TR2_SUST_WIS) p += 4;
	if (a_ptr->flags2 & TR2_SUST_DEX) p += 4;
	if (a_ptr->flags2 & TR2_SUST_CON) p += 5;
	if (a_ptr->flags2 & TR2_SUST_CHR) p += 1;

	if (a_ptr->flags2 & TR2_IM_ACID) {
		p += 26;
		immunities++;
	}
	if (a_ptr->flags2 & TR2_IM_ELEC) {
		p += 20;
		immunities++;
	}
	if (a_ptr->flags2 & TR2_IM_FIRE) {
		p += 28;
		immunities++;
	}
	if (a_ptr->flags2 & TR2_IM_COLD) {
		p += 23;
		immunities++;
	}
	if (a_ptr->flags5 & TR5_IM_POISON) { //currently not possible on randarts
		p += 20;
		immunities++;
	}
	if (immunities > 1) p += 10;
	if (immunities > 2) p += 20000;		/* inhibit */

	if (a_ptr->flags2 & TR2_RES_FEAR) p += 4;
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
	if (a_ptr->flags2 & TR2_RES_DISEN) p += 13;

	if (a_ptr->flags3 & TR3_FEATHER) p += 2;
	if (a_ptr->flags3 & TR3_LITE1) p += 2;
	if (a_ptr->flags4 & TR4_LITE2) p += 4;
	if (a_ptr->flags4 & TR4_LITE3) p += 8;
	if (a_ptr->flags4 & TR4_FUEL_LITE) p -= 15;//10
	if (a_ptr->flags3 & TR3_SEE_INVIS) p += 8;
	//if (a_ptr->flags3 & TR3_TELEPATHY) p += 20;
	if (a_ptr->esp & (ESP_ORC)) p += 1;
	if (a_ptr->esp & (ESP_TROLL)) p += 1;
	if (a_ptr->esp & (ESP_DRAGON)) p += 4;
	if (a_ptr->esp & (ESP_GIANT)) p += 2;
	if (a_ptr->esp & (ESP_DEMON)) p += 3;
	if (a_ptr->esp & (ESP_UNDEAD)) p += 3;
	if (a_ptr->esp & (ESP_EVIL)) p += 16; //p += 14; p += 18;
	if (a_ptr->esp & (ESP_ANIMAL)) p += 4;
	if (a_ptr->esp & (ESP_DRAGONRIDER)) p += 2;
	if (a_ptr->esp & (ESP_GOOD)) p += 4;
	if (a_ptr->esp & (ESP_NONLIVING)) p += 2;
	if (a_ptr->esp & (ESP_UNIQUE)) p += 8;
	if (a_ptr->esp & (ESP_SPIDER)) p += 2;
	if (a_ptr->esp & ESP_ALL) p += 40; //note: this should probably be even higher, maybe 50
	if (a_ptr->flags4 & TR4_AUTO_ID) p += 20;//maybe even higher, like 30
	if (a_ptr->flags3 & TR3_SLOW_DIGEST) p += 4;
	if (a_ptr->flags3 & TR3_REGEN) p += 8;
	if (a_ptr->flags5 & TR5_REGEN_MANA) p += 8;
	if ((a_ptr->flags3 & TR3_TELEPORT) && (a_ptr->flags3 & TR3_CURSED)) p -= 20;
	if (a_ptr->flags3 & TR3_DRAIN_EXP) p -= 16;
	if (a_ptr->flags3 & TR3_AGGRAVATE) p -= 8;
	if (a_ptr->flags3 & TR3_BLESSED) p += 4;
	if (a_ptr->flags3 & TR3_CURSED) p -= 4;
	if (a_ptr->flags3 & TR3_HEAVY_CURSE) p -= 20;
	/*if (a_ptr->flags3 & TR3_PERMA_CURSE) p -= 40; */
	/*if (a_ptr->flags4 & TR4_ANTIMAGIC_10) p += 8; */
	if (a_ptr->flags5 & TR5_INVIS) p += 20;

	if (a_ptr->flags1 & TR1_VAMPIRIC) p += 15;
	if (a_ptr->flags5 & TR5_REFLECT) p += 15;
	if (a_ptr->flags4 & TR4_LEVITATE) p += 15;
	if (a_ptr->flags4 & TR4_CLIMB) p += 15;
	if (a_ptr->flags3 & TR3_SH_FIRE) p += 5;
	if (a_ptr->flags5 & TR5_SH_COLD) p += 5;
	if (a_ptr->flags3 & TR3_SH_ELEC) p += 5;

	/* only for Ethereal DSM basically.. */
	if (a_ptr->flags3 & TR3_WRAITH) p += 20;
	/* only for WINNERS_ONLY heavy armour basically :-o (note: nice vs Zu-Aon) */
	if (a_ptr->flags5 & TR5_RES_MANA) p += 20;
	return p;
}


static void remove_contradictory(artifact_type *a_ptr, bool aggravate_me) { //Kurzel
	/* If the item is predestined to be aggravating, already get free
	   its ap from useless mods that won't make it onto the final item */
	if (aggravate_me) {
		a_ptr->flags1 &= ~(TR1_STEALTH);
		a_ptr->flags5 &= ~(TR5_INVIS);
	}

	/* Remove redundant resistances et al */
	if (a_ptr->flags2 & TR2_IM_ACID) a_ptr->flags2 &= ~(TR2_RES_ACID);
	if (a_ptr->flags2 & TR2_IM_ELEC) a_ptr->flags2 &= ~(TR2_RES_ELEC);
	if (a_ptr->flags2 & TR2_IM_FIRE) a_ptr->flags2 &= ~(TR2_RES_FIRE);
	if (a_ptr->flags2 & TR2_IM_COLD) a_ptr->flags2 &= ~(TR2_RES_COLD);
	if (a_ptr->flags2 & TR2_RES_CHAOS) a_ptr->flags2 &= ~(TR2_RES_CONF);
	if (a_ptr->flags4 & TR4_LEVITATE) a_ptr->flags3 &= ~(TR3_FEATHER);

	/* Remove redundant slay mods */
	if (a_ptr->flags1 & TR1_KILL_DRAGON) a_ptr->flags1 &= ~(TR1_SLAY_DRAGON);
	if (a_ptr->flags1 & TR1_KILL_UNDEAD) a_ptr->flags1 &= ~(TR1_SLAY_UNDEAD);
	if (a_ptr->flags1 & TR1_KILL_DEMON) a_ptr->flags1 &= ~(TR1_SLAY_DEMON);

	/* Remove accidentally given good mods on cursed object */
	if (a_ptr->pval < 0) {
		if (a_ptr->flags1 & TR1_STR) a_ptr->flags2 &= ~(TR2_SUST_STR);
		if (a_ptr->flags1 & TR1_INT) a_ptr->flags2 &= ~(TR2_SUST_INT);
		if (a_ptr->flags1 & TR1_WIS) a_ptr->flags2 &= ~(TR2_SUST_WIS);
		if (a_ptr->flags1 & TR1_DEX) a_ptr->flags2 &= ~(TR2_SUST_DEX);
		if (a_ptr->flags1 & TR1_CON) a_ptr->flags2 &= ~(TR2_SUST_CON);
		if (a_ptr->flags1 & TR1_CHR) a_ptr->flags2 &= ~(TR2_SUST_CHR);
		/*a_ptr->flags1 &= ~(TR1_BLOWS);*/
	}
	if (a_ptr->flags3 & TR3_CURSED) a_ptr->flags3 &= ~(TR3_BLESSED);
	if (a_ptr->flags3 & TR3_DRAIN_EXP) a_ptr->flags2 &= ~(TR2_HOLD_LIFE);
}

static void remove_redundant_esp(artifact_type *a_ptr) {
	if (a_ptr->esp & ESP_DRAGON) a_ptr->esp &= (~ESP_DRAGONRIDER);
	if (a_ptr->esp & ESP_EVIL) a_ptr->esp &= (~(ESP_ORC | ESP_TROLL | ESP_GIANT | ESP_DEMON | ESP_UNDEAD));
	if (a_ptr->esp & ESP_ALL) a_ptr->esp = ESP_ALL;
}



/*
 * Randomly select an extra ability to be added to the artifact in question.
 * This function is way too large.
 */
static void add_ability (artifact_type *a_ptr) {
	int r = rand_int(100);
	bool type_dependant_mod = FALSE;
	object_kind *k_ptr = &k_info[lookup_kind(a_ptr->tval, a_ptr->sval)];

	switch (a_ptr->tval) {
	case TV_BOOMERANG:
	case TV_BOW:
		if (r < 48) type_dependant_mod = TRUE; /* lolwut? */
		break;
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
		type_dependant_mod = TRUE;
		break;
	case TV_DRAG_ARMOR:
		if (r < 67) type_dependant_mod = TRUE;
		break;
	default: /* usually 50%-50% whether a mod is type-dependant or general */
		if (r < 50) type_dependant_mod = TRUE;
	}

	if (type_dependant_mod) {
		r = rand_int(100);

		switch (a_ptr->tval) {
		case TV_BOW:
			if (r < 25) {
				if (a_ptr->flags3 & TR3_XTRA_SHOTS) a_ptr->flags3 |= TR3_XTRA_MIGHT;
				a_ptr->flags3 |= TR3_XTRA_SHOTS;
			} else if (r < 50) {
				if (a_ptr->flags3 & TR3_XTRA_MIGHT) a_ptr->flags3 |= TR3_XTRA_SHOTS;
				a_ptr->flags3 |= TR3_XTRA_MIGHT;
			} else if (r < 70) a_ptr->to_h += 4 + rand_int(4);
			else if (r < 90) a_ptr->to_d += 4 + rand_int(4);
			else {
				a_ptr->to_h += 4 + rand_int(4);
				a_ptr->to_d += 4 + rand_int(4);
			}
			break;
		case TV_DIGGING: /* <- can't be arted atm */
		case TV_BLUNT:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_AXE:
		case TV_BOOMERANG:
			if (r < 1) { /* SPLIT FLAG: see r < 68 -_- */
				a_ptr->flags1 |= TR1_BRAND_POIS;
				if (rand_int(4) > 0) a_ptr->flags2 |= TR2_RES_POIS;
			} else if (r < 4) {
				a_ptr->flags1 |= TR1_WIS;
				do_pval (a_ptr);
				if (rand_int(2) == 0) a_ptr->flags2 |= TR2_SUST_WIS;
				/* chaotic and blessed are exclusive atm */
				if (!(a_ptr->flags5 & TR5_CHAOTIC) &&
				    (is_melee_weapon(a_ptr->tval) || a_ptr->tval == TV_BOOMERANG))
					a_ptr->flags3 |= TR3_BLESSED;
			} else if (r < 7) {
				a_ptr->flags1 |= TR1_BRAND_ACID;
				if (rand_int(4) > 0) a_ptr->flags2 |= TR2_RES_ACID;
			} else if (r < 10) {
				a_ptr->flags1 |= TR1_BRAND_ELEC;
				if (rand_int(4) > 0) a_ptr->flags2 |= TR2_RES_ELEC;
			} else if (r < 14) {
				a_ptr->flags1 |= TR1_BRAND_FIRE;
				if (rand_int(4) > 0) a_ptr->flags2 |= TR2_RES_FIRE;
			} else if (r < 18) {
				a_ptr->flags1 |= TR1_BRAND_COLD;
				if (rand_int(4) > 0) a_ptr->flags2 |= TR2_RES_COLD;
			} else if (r < 21) {
				a_ptr->ds += 1 + rand_int(2) + rand_int(2);
			} else if (r < 25) {
				a_ptr->dd += 1 + rand_int(2) + rand_int(2);
			} else if (r < 27) {
				a_ptr->flags1 |= TR1_KILL_DRAGON;
				a_ptr->esp |= (ESP_DRAGON);
			} else if (r < 29) {
				a_ptr->flags1 |= TR1_KILL_DEMON;
				a_ptr->esp |= (ESP_DEMON);
			} else if (r < 31) {
				a_ptr->flags1 |= TR1_KILL_UNDEAD;
				a_ptr->esp |= (ESP_UNDEAD);
			} else if (r < 35) {
				a_ptr->flags1 |= TR1_SLAY_DRAGON;
				if (magik(80)) a_ptr->esp |= (ESP_DRAGON);
			} else if (r < 39) {
				a_ptr->flags1 |= TR1_SLAY_EVIL;
				if (magik(80)) a_ptr->esp |= (ESP_EVIL);
			} else if (r < 43) {
				a_ptr->flags1 |= TR1_SLAY_ANIMAL;
				if (magik(80)) a_ptr->esp |= (ESP_EVIL);
			} else if (r < 47) {
				a_ptr->flags1 |= TR1_SLAY_UNDEAD;
				if (magik(80)) a_ptr->esp |= (ESP_UNDEAD);
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_SLAY_DEMON;
					if (magik(80)) a_ptr->esp |= (ESP_DEMON);
				}
			} else if (r < 51) {
				a_ptr->flags1 |= TR1_SLAY_DEMON;
				if (magik(80)) a_ptr->esp |= (ESP_DEMON);
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_SLAY_UNDEAD;
					if (magik(80)) a_ptr->esp |= (ESP_UNDEAD);
				}
			} else if (r < 55) {
				a_ptr->flags1 |= TR1_SLAY_ORC;
				if (magik(80)) a_ptr->esp |= (ESP_ORC);
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_SLAY_TROLL;
					if (magik(80)) a_ptr->esp |= (ESP_TROLL);
				}
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_SLAY_GIANT;
					if (magik(80)) a_ptr->esp |= (ESP_GIANT);
				}
			} else if (r < 59) {
				a_ptr->flags1 |= TR1_SLAY_TROLL;
				if (magik(80)) a_ptr->esp |= (ESP_TROLL);
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_SLAY_ORC;
					if (magik(80)) a_ptr->esp |= (ESP_ORC);
				}
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_SLAY_GIANT;
					if (magik(80)) a_ptr->esp |= (ESP_GIANT);
				}
			} else if (r < 63) {
				a_ptr->flags1 |= TR1_SLAY_GIANT;
				if (magik(80)) a_ptr->esp |= (ESP_GIANT);
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_SLAY_ORC;
					if (magik(80)) a_ptr->esp |= (ESP_ORC);
				}
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_SLAY_TROLL;
					if (magik(80)) a_ptr->esp |= (ESP_TROLL);
				}
			} else if (r < 66) {
#if 1
				/* Swords only: VORPAL flag */
				if (a_ptr->tval == TV_SWORD) a_ptr->flags5 |= TR5_VORPAL;
#else
				a_ptr->flags3 |= TR3_SEE_INVIS; //maybe in the future: replace [partially] with TR5_VORPAL
#endif
			} else if (r < 68) { /* SPLIT FLAG: see r < 1 -_- */
				a_ptr->flags1 |= TR1_BRAND_POIS;
				if (rand_int(4) > 0) a_ptr->flags2 |= TR2_RES_POIS;
			} else if (r < 72) {
				/* +EA turns into xshots on boomies */
				if (a_ptr->tval == TV_BOOMERANG) a_ptr->flags3 |= TR3_XTRA_SHOTS;
				else {
					a_ptr->flags1 |= TR1_BLOWS;
					do_pval (a_ptr);
					if (a_ptr->pval > 3) a_ptr->pval = 3;
				}
			} else if (r < 74) {
				/* no +LIFE on boomerangs! turn into xshots instead */
				if (a_ptr->tval == TV_BOOMERANG) {
					a_ptr->flags3 |= TR3_XTRA_SHOTS;
				} else if (a_ptr->tval != TV_DIGGING) { /* no +LIFE on diggers! */
					//melee weapons only:
					a_ptr->flags1 |= TR1_LIFE;
					if (k_ptr->flags4 & (TR4_SHOULD2H | TR4_MUST2H)) {
						do_pval (a_ptr);
						if (a_ptr->pval > 2) a_ptr->pval = 2;
					} else a_ptr->pval = 1;
				}
			} else if (r < 87) {
				a_ptr->to_d += 2 + rand_int(10);
				a_ptr->to_h += 2 + rand_int(10);
			} else if (r < 90) {
				a_ptr->to_a += 3 + rand_int(3);
			} else if (r < 93) {
				a_ptr->flags5 |= TR5_CRIT;
				if (a_ptr->pval < 0) break;
				if (a_ptr->pval == 0) a_ptr->pval = 3 + rand_int(8);
				else if (rand_int(2) == 0) a_ptr->pval++;
			} else if (r < 97) {
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
#if 1 /* activate any time you like :) - note: REMOVE THE "-2" at the start of add_ability() then! */
			else if (r < 98 && a_ptr->tval != TV_DIGGING) {
				/* chaotic and blessed are exclusive atm */
				if (!(a_ptr->flags3 & TR3_BLESSED)) {
					a_ptr->flags5 |= TR5_CHAOTIC;
					a_ptr->flags2 |= TR2_RES_CHAOS;
				}
			}
#endif
			else a_ptr->weight = (a_ptr->weight * 9) / 10;
			break;
		case TV_MSTAFF:
			if (r < 5) a_ptr->flags2 |= TR2_SUST_WIS;
			else if (r < 15) a_ptr->flags2 |= TR2_SUST_INT;
			else if (r < 25) a_ptr->flags3 |= TR3_SEE_INVIS;
			else if (r < 35) {
				a_ptr->to_d += 2 + rand_int(10);
				a_ptr->to_h += 2 + rand_int(10);
			}
			else if (r < 40) {
				int rr = rand_int(29);
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
			else a_ptr->pval += randint(5);
			break;
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
			if (r < 4) a_ptr->flags1 |= TR1_BRAND_ACID;
			else if (r < 8) a_ptr->flags1 |= TR1_BRAND_ELEC;
			else if (r < 12) a_ptr->flags1 |= TR1_BRAND_FIRE;
			else if (r < 16) a_ptr->flags1 |= TR1_BRAND_COLD;
			else if (r < 20) a_ptr->flags1 |= TR1_BRAND_POIS;
			else if (r < 24) a_ptr->flags1 |= TR1_KILL_DRAGON;
			else if (r < 28) a_ptr->flags1 |= TR1_KILL_DEMON;
			else if (r < 32) a_ptr->flags1 |= TR1_KILL_UNDEAD;
			else if (r < 40) a_ptr->flags1 |= TR1_SLAY_DRAGON;
			else if (r < 44) a_ptr->flags1 |= TR1_SLAY_EVIL;
			else if (r < 52) a_ptr->flags1 |= TR1_SLAY_ANIMAL;
			else if (r < 60) a_ptr->flags1 |= TR1_SLAY_UNDEAD;
			else if (r < 68) a_ptr->flags1 |= TR1_SLAY_DEMON;
			else if (r < 72) a_ptr->flags1 |= TR1_SLAY_ORC;
			else if (r < 80) a_ptr->flags1 |= TR1_SLAY_TROLL;
			else if (r < 88) a_ptr->flags1 |= TR1_SLAY_GIANT;
			else if (r < 92) a_ptr->flags1 |= TR1_VAMPIRIC;
			else
			{
				/* bad luck */
			}
			break;
		case TV_BOOTS:
			if (r < 10) a_ptr->flags3 |= TR3_FEATHER;
#ifndef TO_AC_CAP_30
			else if (r < 30) a_ptr->to_a += 3 + rand_int(5);
#else
			else if (r < 30) a_ptr->to_a += 3 + rand_int(4);
#endif
			else if (r < 40) a_ptr->flags4 |= TR4_LEVITATE;
			else if (r < 50) a_ptr->flags4 |= TR4_CLIMB;
			else if (r < 65) {
#ifdef ART_WITAN_STEALTH
				if (a_ptr->tval == TV_BOOTS && a_ptr->sval == SV_PAIR_OF_WITAN_BOOTS) break;
#endif
				a_ptr->flags1 |= TR1_STEALTH;
				do_pval (a_ptr);
			} else if (r < 95) {
				a_ptr->flags1 |= TR1_SPEED;
				if (a_ptr->pval < 0) break;
				if (a_ptr->pval == 0) a_ptr->pval = 3 + rand_int(8);
				else {
					a_ptr->pval++;
					if (!rand_int(2)) a_ptr->pval++;
					if (!rand_int(2)) a_ptr->pval++;
				}
			}
			else a_ptr->weight = (a_ptr->weight * 9) / 10;
			break;
		case TV_GLOVES:
			if (r < 13) a_ptr->flags2 |= TR2_FREE_ACT;
			else if (r < 21)
			{
				a_ptr->flags1 |= TR1_MANA;
				if (a_ptr->pval == 0) a_ptr->pval = 5 + rand_int(6);
				else do_pval (a_ptr);
				if (a_ptr->pval < 0) a_ptr->pval = 2;
			} else if (r < 26) a_ptr->flags4 |= TR4_AUTO_ID;
			else if (r < 36) {
				a_ptr->flags1 |= TR1_DEX;
				do_pval (a_ptr);
			} else if (r < 46) {
				a_ptr->flags1 |= TR1_STR;
				do_pval (a_ptr);
			} else if (r < 51) {
				a_ptr->flags1 |= TR1_BLOWS;
				if (rand_int(3)) a_ptr->pval = 1;
				else a_ptr->pval = 2;
			} else if (r < 53) {
				a_ptr->flags1 |= TR1_LIFE;
				do_pval (a_ptr);
				if (a_ptr->pval > 3) a_ptr->pval = 3;
			} else if (r < 58) {
				a_ptr->flags5 |= TR5_CRIT;
				if (a_ptr->pval < 0) break;
				if (a_ptr->pval == 0) a_ptr->pval = 3 + rand_int(8);
				else if (rand_int(2) == 0) a_ptr->pval++;
			}
			else if (r < 73) {
				int rsub = rand_int(24);
				if ((rsub < 2) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_BRAND_ACID;
				else if ((rsub < 4) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_BRAND_ELEC;
				else if ((rsub < 6) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_BRAND_FIRE;
				else if ((rsub < 8) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_BRAND_COLD;
				else if ((rsub < 10) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_BRAND_POIS;
				else if ((rsub < 12) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_SLAY_DRAGON;
				else if ((rsub < 14) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_SLAY_ANIMAL;
				else if ((rsub < 16) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_SLAY_UNDEAD;
				else if ((rsub < 18) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_SLAY_DEMON;
				else if ((rsub < 20) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_SLAY_ORC;
				else if ((rsub < 22) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_SLAY_TROLL;
				else if ((rsub < 24) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_SLAY_GIANT;
#if 0 /*too powerful on gloves - Art Gloves 'soul cure' can help.*/
				else if (r < 25) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_KILL_DRAGON;
				else if (r < 26) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_KILL_UNDEAD;
				else if (r < 27) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_KILL_DEMON;
				else if (r < 28) && !(a_ptr->flags1 & TR1_MULTMASK))
					a_ptr->flags1 |= TR1_SLAY_EVIL;
#endif
			} else if (r < 77) a_ptr->flags1 |= TR1_VAMPIRIC;
#ifndef TO_AC_CAP_30
			else if (r < 95) a_ptr->to_a += 3 + rand_int(5);
#else
			else if (r < 95) a_ptr->to_a += 3 + rand_int(4);
#endif
			else {
				a_ptr->to_h = 2 + rand_int(7);
				a_ptr->to_d = 2 + rand_int(7);
				a_ptr->flags3 |= TR3_SHOW_MODS;
			}
			break;
		case TV_HELM:
			if (r < 2) {
				a_ptr->flags1 |= TR1_LIFE;
				do_pval (a_ptr);
				if (a_ptr->pval > 3) a_ptr->pval = 3;
			} else if (r < 12) a_ptr->flags2 |= TR2_RES_BLIND;
			else if (r < 17) {
				a_ptr->flags1 |= TR1_INFRA;
				if (a_ptr->pval == 0) a_ptr->pval = randint(2);
			} else if (r < 25) a_ptr->flags4 |= TR4_AUTO_ID;
			else if (r < 30) {
				a_ptr->flags1 |= TR1_WIS;
				do_pval (a_ptr);
			}
			//else if (r < 45) a_ptr->flags3 |= TR3_TELEPATHY;
			//else if (r < 45) a_ptr->esp |= (ESP_ALL);
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
			else if (r < 54) a_ptr->flags3 |= TR3_SEE_INVIS;
			else if (r < 63) {
				a_ptr->flags1 |= TR1_INT;
				do_pval (a_ptr);
			}
			else if (r < 70) a_ptr->flags2 |= TR2_RES_CONF;
			else if (r < 75) a_ptr->flags2 |= TR2_RES_FEAR;
#ifndef TO_AC_CAP_30
			else a_ptr->to_a += 3 + rand_int(5);
#else
			else a_ptr->to_a += 3 + rand_int(4);
#endif
			break;
		case TV_CROWN:
			if (r < 2) {
				a_ptr->flags1 |= TR1_LIFE;
				do_pval (a_ptr);
				if (a_ptr->pval > 3) a_ptr->pval = 3;
			} else if (r < 14) a_ptr->flags2 |= TR2_RES_BLIND;
			else if (r < 17) {
				a_ptr->flags1 |= TR1_INFRA;
				if (a_ptr->pval == 0) a_ptr->pval = randint(2);
			} else if (r < 20) a_ptr->flags5 |= TR5_REGEN_MANA;
			else if (r < 22) {
				a_ptr->flags1 |= TR1_MANA;
				if (a_ptr->pval == 0) a_ptr->pval = randint(5);
			} else if (r < 30) a_ptr->flags4 |= TR4_AUTO_ID;
			//else if (r < 45) a_ptr->flags3 |= TR3_TELEPATHY;
			//else if (r < 45) a_ptr->esp |= (ESP_ALL);
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
			else if (r < 53) a_ptr->flags3 |= TR3_SEE_INVIS;
			else if (r < 62) {
				a_ptr->flags1 |= TR1_WIS;
				do_pval (a_ptr);
			} else if (r < 71) {
				a_ptr->flags1 |= TR1_INT;
				do_pval (a_ptr);
			} else if (r < 77) a_ptr->flags2 |= TR2_RES_CONF;
			else if (r < 81) a_ptr->flags2 |= TR2_RES_FEAR;
#ifndef TO_AC_CAP_30
			else a_ptr->to_a += 3 + rand_int(5);
#else
			else a_ptr->to_a += 3 + rand_int(4);
#endif
			break;
		case TV_SHIELD:
#ifndef USE_NEW_SHIELDS
			if (r < 18) a_ptr->flags2 |= TR2_RES_ACID;
			else if (r < 36) a_ptr->flags2 |= TR2_RES_ELEC;
			else if (r < 54) a_ptr->flags2 |= TR2_RES_FIRE;
			else if (r < 72) a_ptr->flags2 |= TR2_RES_COLD;
			else if (r < 80) a_ptr->flags5 |= TR5_REFLECT;
			else if (r < 90) a_ptr->weight = (a_ptr->weight * 9) / 10;
 #ifndef TO_AC_CAP_30
			else a_ptr->to_a += 3 + rand_int(5);
 #else
			else a_ptr->to_a += 3 + rand_int(4);
 #endif
#else
			if (r < 20) a_ptr->flags2 |= TR2_RES_ACID;
			else if (r < 40) a_ptr->flags2 |= TR2_RES_ELEC;
			else if (r < 60) a_ptr->flags2 |= TR2_RES_FIRE;
			else if (r < 80) a_ptr->flags2 |= TR2_RES_COLD;
			else if (r < 90) a_ptr->flags5 |= TR5_REFLECT;
			else a_ptr->weight = (a_ptr->weight * 9) / 10;
#endif
			break;
		case TV_CLOAK:
			if (r < 5) {
				a_ptr->flags3 |= TR3_FEATHER;
				do_pval (a_ptr);
			} else if (r < 10) { //30
				if (!(a_ptr->flags3 & TR3_SH_FIRE ||
				     a_ptr->flags5 & TR5_SH_COLD ||
				     a_ptr->flags3 & TR3_SH_ELEC)) {
					switch(rand_int(3)) {
					case 0:	a_ptr->flags3 |= TR3_SH_FIRE;
						a_ptr->flags2 |= TR2_RES_FIRE;
						break;
					case 1:	a_ptr->flags3 |= TR3_SH_ELEC;
						a_ptr->flags2 |= TR2_RES_ELEC;
						break;
					case 2: a_ptr->flags5 |= TR5_SH_COLD;
						a_ptr->flags2 |= TR2_RES_COLD;
						break;
					}
				}
			} else if (r < 20) a_ptr->flags5 |= TR5_INVIS;//33
			else if (r < 34) { //55
				a_ptr->flags1 |= TR1_STEALTH;
				do_pval (a_ptr);
			} else if (r < 45) a_ptr->flags2 |= TR2_RES_SHARDS;
			else if (r < 50) a_ptr->flags4 |= TR4_LEVITATE;
			else if (r < 55) a_ptr->flags2 |= TR2_HOLD_LIFE;
			else if (r < 61) a_ptr->flags2 |= TR2_RES_FIRE;
			else if (r < 68) a_ptr->flags2 |= TR2_RES_COLD;
			else if (r < 71) a_ptr->flags2 |= TR2_RES_ACID;
			else if (r < 75) a_ptr->flags2 |= TR2_RES_ELEC;
#ifndef TO_AC_CAP_30
			else a_ptr->to_a += 3 + rand_int(3);
#else
			else a_ptr->to_a += 3 + rand_int(2);
#endif
			break;
		case TV_DRAG_ARMOR:
			/*if (r < 55) ; --changed into 67% hack above */
			if (r < 15) a_ptr->flags2 |= TR2_HOLD_LIFE;
			else if (r < 30) {
				a_ptr->flags1 |= TR1_CON;
				do_pval (a_ptr);
				if (rand_int(2) == 0)
					a_ptr->flags2 |= TR2_SUST_CON;
			} else if (r < 45) {
				a_ptr->flags1 |= TR1_STR;
				do_pval (a_ptr);
				if (rand_int(2) == 0)
					a_ptr->flags2 |= TR2_SUST_STR;
			} else if (r < 50) {
				a_ptr->flags1 |= TR1_LIFE;
				do_pval (a_ptr);
				if (a_ptr->pval > 3) a_ptr->pval = 3;
#ifndef TO_AC_CAP_30
			} else a_ptr->to_a += 1 + rand_int(4);
#else
			} else a_ptr->to_a += 1 + rand_int(3);
#endif
			break;
		case TV_HARD_ARMOR:
			/* extra mods for royal armour */
			if ((a_ptr->flags5 & TR5_WINNERS_ONLY) && !rand_int(10)) {
				int rr = rand_int(100);
				if (rr < 20) a_ptr->flags5 |= TR5_REFLECT;
				else if (rr < 30) {
					a_ptr->flags5 |= TR5_RES_MANA;
					a_ptr->flags5 |= TR5_IGNORE_DISEN;
				} else if (rr < 50) a_ptr->flags2 |= TR2_IM_FIRE;
				else if (rr < 65) a_ptr->flags2 |= TR2_IM_ELEC;
				else if (rr < 80) a_ptr->flags2 |= TR2_IM_ACID;
				else if (rr < 95) a_ptr->flags2 |= TR2_IM_COLD;
				else {
					a_ptr->flags1 |= TR1_LIFE;
					do_pval (a_ptr);
					if (a_ptr->pval > 3) a_ptr->pval = 3;
				}
				break;
			}
			/* fall through! */
		case TV_SOFT_ARMOR:
			if (r < 8) {
				a_ptr->flags1 |= TR1_STEALTH;
				do_pval (a_ptr);
			} else if (r < 16) a_ptr->flags2 |= TR2_HOLD_LIFE;
			else if (r < 22) {
				a_ptr->flags1 |= TR1_CON;
				do_pval (a_ptr);
				if (rand_int(2) == 0)
					a_ptr->flags2 |= TR2_SUST_CON;
			} else if (r < 34) a_ptr->flags2 |= TR2_RES_ACID;
			else if (r < 46) a_ptr->flags2 |= TR2_RES_ELEC;
			else if (r < 58) a_ptr->flags2 |= TR2_RES_FIRE;
			else if (r < 70) a_ptr->flags2 |= TR2_RES_COLD;
			else if (r < 72) {
				a_ptr->flags1 |= TR1_LIFE;
				do_pval (a_ptr);
				if (a_ptr->pval > 3) a_ptr->pval = 3;
			} else if (r < 80) a_ptr->weight = (a_ptr->weight * 9) / 10;
#ifndef TO_AC_CAP_30
			else a_ptr->to_a += 3 + rand_int(8);
#else
			else a_ptr->to_a += 3 + rand_int(7);
#endif
			break;
		case TV_RING:
			if (r < 5) {
				if (k_ptr->flags2 & TR2_RES_ELEC) a_ptr->flags2 |= TR2_IM_ELEC;
				else if (k_ptr->flags2 & TR2_RES_COLD) a_ptr->flags2 |= TR2_IM_COLD;
				else if (k_ptr->flags2 & TR2_RES_FIRE) a_ptr->flags2 |= TR2_IM_FIRE;
				else if (k_ptr->flags2 & TR2_RES_ACID) a_ptr->flags2 |= TR2_IM_ACID;
			}
			break;
		case TV_LITE:
			if (r < 50) a_ptr->flags3 |= TR3_LITE1;
			else if (r < 80) a_ptr->flags4 |= TR4_LITE2;
			else a_ptr->flags4 |= TR4_LITE3;
			//if (r % 2) a_ptr->flags4 &= ~TR4_FUEL_LITE;
			if (!(r % 5)) a_ptr->flags4 &= ~TR4_FUEL_LITE;
			break;
		default:
			break;
		}
	} else {		/* Pick something universally useful. */
		switch (a_ptr->tval) {
		case TV_BOOMERANG:
		case TV_BOW:
			/*if (magik(33)) break;*/
		default:
			r = rand_int(44);

			switch (r) {
			case 0:
				a_ptr->flags1 |= TR1_STR;
				do_pval (a_ptr);
				if (rand_int(2) == 0) a_ptr->flags2 |= TR2_SUST_STR;
				break;
			case 1:
				a_ptr->flags1 |= TR1_INT;
				do_pval (a_ptr);
				if (rand_int(2) == 0) a_ptr->flags2 |= TR2_SUST_INT;
				break;
			case 2:
				a_ptr->flags1 |= TR1_WIS;
				do_pval (a_ptr);
				if (rand_int(2) == 0) a_ptr->flags2 |= TR2_SUST_WIS;
				if (a_ptr->tval == TV_SWORD || a_ptr->tval == TV_POLEARM)
					a_ptr->flags3 |= TR3_BLESSED;
				break;
			case 3:
				a_ptr->flags1 |= TR1_DEX;
				do_pval (a_ptr);
				if (rand_int(2) == 0) a_ptr->flags2 |= TR2_SUST_DEX;
				break;
			case 4:
				a_ptr->flags1 |= TR1_CON;
				do_pval (a_ptr);
				if (rand_int(2) == 0) a_ptr->flags2 |= TR2_SUST_CON;
				break;
			case 5:
				a_ptr->flags1 |= TR1_CHR;
				do_pval (a_ptr);
				if (rand_int(2) == 0) a_ptr->flags2 |= TR2_SUST_CHR;
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
				/* Nazgul rings never have NO_MAGIC, wouldn't make sense lore-wise */
				if (a_ptr->tval == TV_RING && a_ptr->sval == SV_RING_SPECIAL) break;
				/* cut chance in half again -> approx. 1/20..1/25 over whole item base - C. Blue */
				if (!rand_int(2)) a_ptr->flags3 |= TR3_NO_MAGIC;
				break;
			case 9:
				/* hack: no +speed on shields, this puts 2h weapons at a severe disadvantage,
				   because even though they may get +6 speed, seemingly equal to +3/+3 (1h+shield),
				   this has 2 big flaws:
				   1) getting +5 or +6 speed is RARE, 2) this means the weapon won't have +EA! */
				if (a_ptr->tval == TV_SHIELD) {
					a_ptr->flags5 |= TR5_REFLECT;
					break;
				}
				/* hack: no +speed on boomerangs
				   (mainly for ENABLE_MA_BOOMERANG in regards to MA mimics -> +speed overkill) */
				if (a_ptr->tval == TV_BOOMERANG) {
					//a_ptr->flags |= TR_; -- no good filler available atm. so just retry.
					break;
				}

				a_ptr->flags1 |= TR1_SPEED;
				if (a_ptr->pval == 0) a_ptr->pval = 3 + rand_int(3);
				else do_pval (a_ptr);
				break;
			case 10:
				a_ptr->flags2 |= TR2_SUST_STR;
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_STR;
					do_pval (a_ptr);
				}
				break;
			case 11:
				a_ptr->flags2 |= TR2_SUST_INT;
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_INT;
					do_pval (a_ptr);
				}
				break;
			case 12:
				a_ptr->flags2 |= TR2_SUST_WIS;
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_WIS;
					do_pval (a_ptr);
					if (a_ptr->tval == TV_SWORD || a_ptr->tval == TV_POLEARM)
						a_ptr->flags3 |= TR3_BLESSED;
				}
				break;
			case 13:
				a_ptr->flags2 |= TR2_SUST_DEX;
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_DEX;
					do_pval (a_ptr);
				}
				break;
			case 14:
				a_ptr->flags2 |= TR2_SUST_CON;
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_CON;
					do_pval (a_ptr);
				}
				break;
			case 15:
				a_ptr->flags2 |= TR2_SUST_CHR;
				if (rand_int(2) == 0) {
					a_ptr->flags1 |= TR1_CHR;
					do_pval (a_ptr);
				}
				break;
			case 16:
				if (rand_int(3) == 0) a_ptr->flags2 |= TR2_IM_ACID;
				break;
			case 17:
				if (rand_int(3) == 0) a_ptr->flags2 |= TR2_IM_ELEC;
				break;
			case 18:
				if (rand_int(4) == 0) a_ptr->flags2 |= TR2_IM_FIRE;
				break;
			case 19:
				if (rand_int(3) == 0) a_ptr->flags2 |= TR2_IM_COLD;
				break;
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
				if (rand_int(2) == 0) a_ptr->flags2 |= TR2_RES_NETHER;
				break;
			case 34: a_ptr->flags2 |= TR2_RES_NEXUS; break;
			case 35: a_ptr->flags2 |= TR2_RES_CHAOS; break;
			case 36:
				if (rand_int(2) == 0) a_ptr->flags2 |= TR2_RES_DISEN;
				break;
			case 37: a_ptr->flags3 |= TR3_FEATHER; break;
			case 38: a_ptr->flags3 |= TR3_LITE1; break;
			case 39: a_ptr->flags3 |= TR3_SEE_INVIS; break;
		        case 40:
#if 0
				if (rand_int(3) == 0)
					//a_ptr->flags3 |= TR3_TELEPATHY;
					a_ptr->esp |= (ESP_ALL);
#endif	// 0
				{
					int rr = rand_int(29);
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
#if 0 /* only for helms/crowns */
			case 44:
				a_ptr->flags1 |= TR1_INFRA;
				do_pval (a_ptr);
				break;
#endif
			}
		}
	}
}



/* Fix various artifact limits and contradictions */
static void artifact_fix_limits_inbetween(artifact_type *a_ptr, object_kind *k_ptr) {
	int c = 0; /* used to count how many stats the artifact increases */

/* -------------------------------------- Initial min/max limit -------------------------------------- */

	/* Never have more than +11 bonus */
	if (!is_ammo(a_ptr->tval) && a_ptr->pval > 11) a_ptr->pval = 11;

	/* Ensure a bonus for certain items whose base types always have a (b)pval */
	if (((k_ptr->flags1 & TR1_PVAL_MASK) || (k_ptr->flags5 & TR5_PVAL_MASK)) && !a_ptr->pval) {
		a_ptr->pval = randint(3); /* do_pval */
		if (k_ptr->flags3 & TR3_CURSED) a_ptr->pval = -a_ptr->pval;
	}

/* -------------------------------------- pval-independant limits -------------------------------------- */

	/* Don't exaggerate at weapon dice (2h: 5d6, 6d8, 6d8, 10d4; 1.5h: 5d5, 6d3, 1h: 2d8/3d5 */
	while (a_ptr->dd * (a_ptr->ds + 1) > slay_limit_randart(a_ptr, k_ptr)
//	    || ((k_ptr->flags4 & (TR4_MUST2H | TR4_SHOULD2H)) && a_ptr->dd * (a_ptr->ds + 1) >= (k_ptr->dd * (k_ptr->ds + 1)) << 1)
	    ) {
		if (a_ptr->dd <= k_ptr->dd) a_ptr->ds--;
		else if (a_ptr->ds <= k_ptr->ds) a_ptr->dd--;
		else {
			if (rand_int(2)) a_ptr->ds--;
			else a_ptr->dd--;
		}
	}
	/* fix lower limit (paranoia) */
	if (a_ptr->dd < 1) a_ptr->dd = 1;
	if (a_ptr->ds < 1) a_ptr->ds = 1;
	/* Don't increase it too much, ie less than for 'of slaying' ego weapons */
	if (k_ptr->dd >= 7) {
		if (a_ptr->dd > k_ptr->dd + 3) a_ptr->dd = k_ptr->dd + 3;
	} else {
		if (a_ptr->dd > k_ptr->dd + 2) a_ptr->dd = k_ptr->dd + 2;
	}
	if (a_ptr->ds > k_ptr->ds + 2) a_ptr->ds = k_ptr->ds + 2;

	/* Never more than +6 +hit/+dam on gloves, +30 in general */
	switch (a_ptr->tval) { /* CAP_ITEM_BONI */
	case TV_GLOVES:
		if (a_ptr->to_h > 6) a_ptr->to_h = 6;
		if (a_ptr->to_d > 6) a_ptr->to_d = 6;
#ifndef TO_AC_CAP_30
		if (a_ptr->to_a > 35) a_ptr->to_a = 35;
#else
		if (a_ptr->to_a > 30) a_ptr->to_a = 30;
#endif
		break;
	case TV_SHIELD:
#ifdef USE_NEW_SHIELDS  /* should actually be USE_BLOCKING, but could be too */
                        /* dramatic a change if it gets enabled temporarily - C. Blue */
 #ifndef NEW_SHIELDS_NO_AC
		if (a_ptr->to_a > 15) a_ptr->to_a = 15;
 #else
		a_ptr->to_a = 0;
 #endif
		break;
#endif
	case TV_SOFT_ARMOR: case TV_HARD_ARMOR: case TV_DRAG_ARMOR:
	case TV_CLOAK: case TV_HELM: case TV_CROWN: case TV_BOOTS:
//		if (a_ptr->to_a > 50) a_ptr->to_a = 50;
#ifndef TO_AC_CAP_30
		if (a_ptr->to_a > 35) a_ptr->to_a = 35;
#else
		if (a_ptr->to_a > 30) a_ptr->to_a = 30;
#endif
		break;
	case TV_BOW:
	case TV_BOOMERANG:
	default: /* all melee weapons */
		if (a_ptr->to_h > 30) a_ptr->to_h = 30;
		if (a_ptr->to_d > 30) a_ptr->to_d = 30;
		break;
	}

	/* Mage staves never have NO_MAGIC but their pval always adds to MANA */
	if (a_ptr->tval == TV_MSTAFF) {
		a_ptr->flags3 &= ~TR3_NO_MAGIC;
		if (a_ptr->pval) a_ptr->flags1 |= TR1_MANA;

#if 0 /* these aren't counted into AP for mage staves actually */
		/* keep it neutral for now, to keep AP unaffected */
		a_ptr->to_h = 0;
		a_ptr->to_d = 0;
#endif
	}
	/* Dark Swords never add to MANA */
	if (a_ptr->tval == TV_SWORD && a_ptr->sval == SV_DARK_SWORD) a_ptr->flags1 &= ~TR1_MANA;

	/* If an item gives +MANA, remove NO_MAGIC property */
	if ((a_ptr->flags1 & TR1_MANA) && !(k_ptr->flags3 & TR3_NO_MAGIC)) a_ptr->flags3 &= ~TR3_NO_MAGIC;
	/* If an item gives REGEN_MANA, remove NO_MAGIC property */
	if ((a_ptr->flags5 & TR5_REGEN_MANA) && !(k_ptr->flags3 & TR3_NO_MAGIC)) a_ptr->flags3 &= ~TR3_NO_MAGIC;
#if 0 /* would also need to disallow egos w/ BLESSED then, which is problematic in some ways */
	/* Don't allow BLESSED on Dark Swords */
	if (k_ptr->tval == TV_SWORD && k_ptr->sval == SV_DARK_SWORD) a_ptr->flags3 &= ~TR3_BLESSED;
#endif
	/* If an item is BLESSED, remove NO_MAGIC property */
	if ((a_ptr->flags3 & TR3_BLESSED) && !(k_ptr->flags3 & TR3_NO_MAGIC)) a_ptr->flags3 &= ~TR3_NO_MAGIC;

/* -------------------------------------- Flag-killing limits -------------------------------------- */

	/* Not more than +5 stealth */
	 /* exception for cloaks to get on par with elven cloaks of the bat */
	if (a_ptr->flags1 & TR1_STEALTH) {
		if (k_ptr->tval != TV_CLOAK) {
			if (a_ptr->pval > 5) {
				if (((a_ptr->flags1 & TR1_SPEED) || (a_ptr->flags5 & TR5_CRIT) || (a_ptr->flags1 & TR1_MANA))
				    && !(k_ptr->flags1 & TR1_STEALTH))
					a_ptr->flags1 &= ~TR1_STEALTH;
				else
					a_ptr->pval = 5;
			}
		}
		else if (a_ptr->pval > 6) a_ptr->pval = 6;
	}

	/* Not more than +4 searching, but remove searching if it blocks highly valued flags instead */
	if ((a_ptr->flags1 & TR1_SEARCH) && (a_ptr->pval > 4)) {
		if (((a_ptr->flags1 & TR1_SPEED) || (a_ptr->flags5 & TR5_CRIT) || (a_ptr->flags1 & TR1_MANA))
		    && !(k_ptr->flags1 & TR1_SEARCH))
			a_ptr->flags1 &= ~TR1_SEARCH;
		else
			a_ptr->pval = 4;
	}

	/* Preparation for erasure of heaviest speed/mana killing flags, see further below.. */
	if (a_ptr->flags1 & (TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR)) {
		/* Count how many stats are increased */
		if (a_ptr->flags1 & TR1_STR) c++;
		if (a_ptr->flags1 & TR1_INT) c++;
		if (a_ptr->flags1 & TR1_WIS) c++;
		if (a_ptr->flags1 & TR1_DEX) c++;
		if (a_ptr->flags1 & TR1_CON) c++;
		if (a_ptr->flags1 & TR1_CHR) c++;
	}

	/* Speed is of primary importance on boots! */
	if ((a_ptr->flags1 & TR1_SPEED) && (k_ptr->tval == TV_BOOTS)) {
		/* Erase the heaviest speed-killing flags! */
		if (a_ptr->pval > 5) { /* differ from post-check, to match reduction below */
			a_ptr->flags1 &= ~(TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR);
		} else if (a_ptr->pval > 3 && c > 3) { /* differ from post-check, to match reduction below */
			a_ptr->flags1 &= ~(TR1_INT | TR1_WIS | TR1_CHR); /* quite nice selectivity */
		}
	}
	/* And I guess mana is similarly important on mage staves */
	if ((a_ptr->flags1 & TR1_MANA) && (k_ptr->tval == TV_MSTAFF)) {
		/* Erase the heaviest mana-killing flags! */
		if (a_ptr->pval > 5) { /* differ from post-check, to match reduction below */
			a_ptr->flags1 &= ~(TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR);
		} else if (a_ptr->pval > 3 && c > 3) { /* differ from post-check, to match reduction below */
			a_ptr->flags1 &= ~(TR1_INT | TR1_WIS | TR1_CHR); /* quite nice selectivity */
		}
	}
	/* Note: Crit on weapons actually isn't that big of a deal, since
	   it doesn't scale in a linear way anyway, so leaving it out here. - C. Blue */

	/* Update count */
	c = 0;
	if (a_ptr->flags1 & (TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR)) {
		/* Count how many stats are increased */
		if (a_ptr->flags1 & TR1_STR) c++;
		if (a_ptr->flags1 & TR1_INT) c++;
		if (a_ptr->flags1 & TR1_WIS) c++;
		if (a_ptr->flags1 & TR1_DEX) c++;
		if (a_ptr->flags1 & TR1_CON) c++;
		if (a_ptr->flags1 & TR1_CHR) c++;
	}

/* -------------------------------------- pval-dividing limits -------------------------------------- */

	/* Never more than +3 EA, +2 on gloves (also done below in pval-fixing limits */
        if (a_ptr->flags1 & TR1_BLOWS) {
		if (a_ptr->tval == TV_GLOVES) {
			//if (a_ptr->pval > 2) a_ptr->pval /= 3;
	                if (a_ptr->pval > 2) a_ptr->pval = 2;
		} else {
			//if (a_ptr->pval > 3) a_ptr->pval /= 2;
	                if (a_ptr->pval > 3) a_ptr->pval = 3;
		}
		if (a_ptr->pval == 0) a_ptr->pval = 1;
        }

/* -------------------------------------- pval-fixing limits -------------------------------------- */

	/* Never more than +3 LIFE (currently some armour only) or +3 EA */
	if ((a_ptr->flags1 & (TR1_LIFE | TR1_BLOWS)) && (a_ptr->pval > 3)) a_ptr->pval = 3;
	if ((a_ptr->tval == TV_GLOVES) && (a_ptr->flags1 & TR1_BLOWS) && (a_ptr->pval > 2)) a_ptr->pval = 2;
	/* Never have super EA _and_ LIFE at the same time o_o */
	if ((a_ptr->flags1 & TR1_LIFE) && (a_ptr->flags1 & TR1_BLOWS) && (a_ptr->pval > 1)) a_ptr->pval = 1;

	if ((a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN)) {
		/* Not more than +6 IV on helms and crowns */
		if ((a_ptr->flags1 & TR1_INFRA) && (a_ptr->pval > 6)) a_ptr->pval = 6;
		/* Not more than +3 speed on helms/crowns */
		if ((a_ptr->flags1 & TR1_SPEED) && (a_ptr->pval > 3)) a_ptr->pval = 3;
	}

	/* Limits for +MANA */
        if (a_ptr->flags1 & TR1_MANA) {
		/* Randart mage staves may give up to +10 +1 bonus MANA */
		if ((a_ptr->tval == TV_MSTAFF) && (a_ptr->pval >= 11)) a_ptr->pval = 11;
		/* Helms and crowns may not give more than +3 MANA */
		else if ((a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN) &&
		    (a_ptr->pval > 3)) a_ptr->pval = 3;
		/* Usually +10 MANA is max */
                else if (a_ptr->pval > 10) a_ptr->pval = 10;
        }

	/* Limit speed on 1-hand weapons and shields (balances both, dual-wiel and 2-handed weapons) */
	/* Limit +LIFE to +1 under same circumstances */
	if (k_ptr->tval == TV_SHIELD || is_melee_weapon(k_ptr->tval)) {
		if (!(k_ptr->flags4 & (TR4_SHOULD2H | TR4_MUST2H))) {
			if ((a_ptr->flags1 & TR1_SPEED) && (a_ptr->pval > 3)) a_ptr->pval = 3;
			if ((a_ptr->flags1 & TR1_LIFE) && (a_ptr->pval > 1)) a_ptr->pval = 1;
		} else { //1.5h/2h melee weapons
			if ((a_ptr->flags1 & TR1_SPEED) && (a_ptr->pval > 6)) a_ptr->pval = 6;
			if ((a_ptr->flags1 & TR1_LIFE) && (a_ptr->pval > 2)) a_ptr->pval = 2;
		}
	}

	/* Note: Neither luck nor disarm can actually newly appear on a randart except if coming from k_info. */

	/* Not more than +6 luck (was +5, increased for randart elven cloaks!) */
	if ((a_ptr->flags5 & TR5_LUCK) && (a_ptr->pval > 6)) a_ptr->pval = 6;

	/* Not more than +3 disarming ability (randart picklocks aren't allowed anyways) */
	if ((a_ptr->flags5 & TR5_DISARM) && (a_ptr->pval > 3)) {
		if (!(k_ptr->flags5 & TR5_DISARM)) a_ptr->flags5 &= ~TR5_DISARM;
		else a_ptr->pval = 3;
	}

/* -------------------------------------- Misc unaffecting boni/limits -------------------------------------- */

}

/* Fix various artifact limits and contradictions */
static void artifact_fix_limits_afterwards(artifact_type *a_ptr, object_kind *k_ptr) {
	int c = 0; /* used to count how many stats the artifact increases */

/* -------------------------------------- Initial min/max limit -------------------------------------- */

	/* Never have more than +11 bonus */
	if (!is_ammo(a_ptr->tval) && a_ptr->pval > 11) a_ptr->pval = 11;

	/* Ensure a bonus for certain items whose base types always have a (b)pval */
	if (((k_ptr->flags1 & TR1_PVAL_MASK) || (k_ptr->flags5 & TR5_PVAL_MASK)) && !a_ptr->pval) {
		a_ptr->pval = randint(3); /* do_pval */
		if (k_ptr->flags3 & TR3_CURSED) a_ptr->pval = -a_ptr->pval;
	}

/* -------------------------------------- pval-independant limits -------------------------------------- */

	/* Don't exaggerate at weapon dice (2h: 5d6, 6d8, 6d8, 10d4; 1.5h: 5d5, 6d3, 1h: 2d8/3d5 */
	while (a_ptr->dd * (a_ptr->ds + 1) > slay_limit_randart(a_ptr, k_ptr)
	    //|| ((k_ptr->flags4 & (TR4_MUST2H | TR4_SHOULD2H)) && a_ptr->dd * (a_ptr->ds + 1) >= (k_ptr->dd * (k_ptr->ds + 1)) << 1)
	    ) {
		if (a_ptr->dd <= k_ptr->dd) a_ptr->ds--;
		else if (a_ptr->ds <= k_ptr->ds) a_ptr->dd--;
		else {
			if (rand_int(2)) a_ptr->ds--;
			else a_ptr->dd--;
		}
	}
	/* fix lower limit (paranoia) */
	if (a_ptr->dd < 1) a_ptr->dd = 1;
	if (a_ptr->ds < 1) a_ptr->ds = 1;
	/* Don't increase it too much, ie less than for 'of slaying' ego weapons */
	if (k_ptr->dd >= 7) {
		if (a_ptr->dd > k_ptr->dd + 3) a_ptr->dd = k_ptr->dd + 3;
	} else {
		if (a_ptr->dd > k_ptr->dd + 2) a_ptr->dd = k_ptr->dd + 2;
	}
	if (a_ptr->ds > k_ptr->ds + 2) a_ptr->ds = k_ptr->ds + 2;

	/* Never more than +6 +hit/+dam on gloves, +30 in general */
	switch (a_ptr->tval) { /* CAP_ITEM_BONI */
	case TV_GLOVES:
		if (a_ptr->to_h > 6) a_ptr->to_h = 6;
		if (a_ptr->to_d > 6) a_ptr->to_d = 6;
#ifndef TO_AC_CAP_30
		if (a_ptr->to_a > 35) a_ptr->to_a = 35;
#else
		if (a_ptr->to_a > 30) a_ptr->to_a = 30;
#endif
		break;
	case TV_SHIELD:
#ifdef USE_NEW_SHIELDS  /* should actually be USE_BLOCKING, but could be too */
                        /* dramatic a change if it gets enabled temporarily - C. Blue */
 #ifndef NEW_SHIELDS_NO_AC
		if (a_ptr->to_a > 15) a_ptr->to_a = 15;
 #else
		a_ptr->to_a = 0;
 #endif
		break;
#endif
	case TV_SOFT_ARMOR: case TV_HARD_ARMOR: case TV_DRAG_ARMOR:
	case TV_CLOAK: case TV_HELM: case TV_CROWN: case TV_BOOTS:
//		if (a_ptr->to_a > 50) a_ptr->to_a = 50;
#ifndef TO_AC_CAP_30
		if (a_ptr->to_a > 35) a_ptr->to_a = 35;
#else
		if (a_ptr->to_a > 30) a_ptr->to_a = 30;
#endif
		break;
	case TV_BOW:
	case TV_BOOMERANG:
	default: /* all melee weapons */
		if (a_ptr->to_h > 30) a_ptr->to_h = 30;
		if (a_ptr->to_d > 30) a_ptr->to_d = 30;
		break;
	}

	/* Mage staves never have NO_MAGIC but their pval always adds to MANA */
	if (a_ptr->tval == TV_MSTAFF) {
		a_ptr->flags3 &= ~TR3_NO_MAGIC;
		if (a_ptr->pval) a_ptr->flags1 |= TR1_MANA;
#if 0
		/* reduce +hit/+dam depending on +MANA bonus */
		a_ptr->to_h = -(a_ptr->pval + rand_int(5)) * 3;
		a_ptr->to_d = -(a_ptr->pval + rand_int(5)) * 3;
		if (a_ptr->to_h > 10) a_ptr->to_h = 10;
		if (a_ptr->to_d > 10) a_ptr->to_d = 10;
#else
		if (a_ptr->to_h > 15) a_ptr->to_h = 15;
		if (a_ptr->to_d > 15) a_ptr->to_d = 15;
#endif
	}
	/* Dark Swords never add to MANA */
	if (a_ptr->tval == TV_SWORD && a_ptr->sval == SV_DARK_SWORD) a_ptr->flags1 &= ~TR1_MANA;

	/* If an item gives +MANA, remove NO_MAGIC property */
	if ((a_ptr->flags1 & TR1_MANA) && !(k_ptr->flags3 & TR3_NO_MAGIC)) a_ptr->flags3 &= ~TR3_NO_MAGIC;
	/* If an item gives REGEN_MANA, remove NO_MAGIC property */
	if ((a_ptr->flags5 & TR5_REGEN_MANA) && !(k_ptr->flags3 & TR3_NO_MAGIC)) a_ptr->flags3 &= ~TR3_NO_MAGIC;
#if 0 /* would also need to disallow egos w/ BLESSED then, which is problematic in some ways */
	/* Don't allow BLESSED on Dark Swords */
	if (k_ptr->tval == TV_SWORD && k_ptr->sval == SV_DARK_SWORD) a_ptr->flags3 &= ~TR3_BLESSED;
#endif
	/* If an item is BLESSED, remove NO_MAGIC property */
	if ((a_ptr->flags3 & TR3_BLESSED) && !(k_ptr->flags3 & TR3_NO_MAGIC)) a_ptr->flags3 &= ~TR3_NO_MAGIC;

/* -------------------------------------- Flag-killing limits -------------------------------------- */

	/* Not more than +5 stealth */
	 /* exception for cloaks to get on par with elven cloaks of the bat */
	if (a_ptr->flags1 & TR1_STEALTH) {
		if (k_ptr->tval != TV_CLOAK) {
			if (a_ptr->pval > 5) {
				if (((a_ptr->flags1 & TR1_SPEED) || (a_ptr->flags5 & TR5_CRIT) || (a_ptr->flags1 & TR1_MANA))
				    && !(k_ptr->flags1 & TR1_STEALTH))
					a_ptr->flags1 &= ~TR1_STEALTH;
				else
					a_ptr->pval = 5;
			}
		}
		else if (a_ptr->pval > 6) a_ptr->pval = 6;
	}

	/* Not more than +4 searching, but remove searching if it blocks highly valued flags instead */
	if ((a_ptr->flags1 & TR1_SEARCH) && (a_ptr->pval > 4)) {
		if (((a_ptr->flags1 & TR1_SPEED) || (a_ptr->flags5 & TR5_CRIT) || (a_ptr->flags1 & TR1_MANA))
		    && !(k_ptr->flags1 & TR1_SEARCH))
			a_ptr->flags1 &= ~TR1_SEARCH;
		else
			a_ptr->pval = 4;
	}

	/* Preparation for erasure of heaviest speed/mana killing flags, see further below.. */
	if (a_ptr->flags1 & (TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR)) {
		/* Count how many stats are increased */
		if (a_ptr->flags1 & TR1_STR) c++;
		if (a_ptr->flags1 & TR1_INT) c++;
		if (a_ptr->flags1 & TR1_WIS) c++;
		if (a_ptr->flags1 & TR1_DEX) c++;
		if (a_ptr->flags1 & TR1_CON) c++;
		if (a_ptr->flags1 & TR1_CHR) c++;
	}

	/* Speed is of primary importance on boots! */
	if ((a_ptr->flags1 & TR1_SPEED) && (k_ptr->tval == TV_BOOTS)) {
		/* Erase the heaviest speed-killing flags! */
		if (a_ptr->pval > 6) {
			a_ptr->flags1 &= ~(TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR);
		} else if (a_ptr->pval > 4 && c > 3) {
			a_ptr->flags1 &= ~(TR1_INT | TR1_WIS | TR1_CHR); /* quite nice selectivity */
		}
	}
	/* And I guess mana is similarly important on mage staves */
	if ((a_ptr->flags1 & TR1_MANA) && (k_ptr->tval == TV_MSTAFF)) {
		/* Erase the heaviest mana-killing flags! */
		if (a_ptr->pval > 6) {
			a_ptr->flags1 &= ~(TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR);
		} else if (a_ptr->pval > 4 && c > 3) {
			a_ptr->flags1 &= ~(TR1_INT | TR1_WIS | TR1_CHR); /* quite nice selectivity */
		}
	}
	/* Note: Crit on weapons actually isn't that big of a deal, since
	   it doesn't scale in a linear way anyway, so leaving it out here. - C. Blue */

	/* Update count */
	c = 0;
	if (a_ptr->flags1 & (TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR)) {
		/* Count how many stats are increased */
		if (a_ptr->flags1 & TR1_STR) c++;
		if (a_ptr->flags1 & TR1_INT) c++;
		if (a_ptr->flags1 & TR1_WIS) c++;
		if (a_ptr->flags1 & TR1_DEX) c++;
		if (a_ptr->flags1 & TR1_CON) c++;
		if (a_ptr->flags1 & TR1_CHR) c++;
	}

/* -------------------------------------- pval-dividing limits -------------------------------------- */

	/* If an item increases all three, SPEED, CRIT, MANA,
	   then reduce pval to 1/2 to balance */
	if ((a_ptr->flags1 & TR1_SPEED) && (a_ptr->flags5 & TR5_CRIT) && (a_ptr->flags1 & TR1_MANA)) {
		a_ptr->pval /= 2;
		if (!a_ptr->pval) a_ptr->pval = 1;
	}
	/* If an item increases two of SPEED, CRIT, MANA by over 7
	   then reduce pval to 2/3 to balance */
	else if ((((a_ptr->flags1 & TR1_SPEED) && (a_ptr->flags5 & TR5_CRIT)) ||
	    ((a_ptr->flags1 & TR1_SPEED) && (a_ptr->flags1 & TR1_MANA)) ||
	    ((a_ptr->flags1 & TR1_MANA) && (a_ptr->flags5 & TR5_CRIT)))) {
		a_ptr->pval = (a_ptr->pval * 2) / 3;
		if (!a_ptr->pval) a_ptr->pval = 1;
	}

	/* Never more than +3 EA, +2 on gloves */
	if (a_ptr->flags1 & TR1_BLOWS) {
		if (a_ptr->tval == TV_GLOVES) {
			//if (a_ptr->pval > 2) a_ptr->pval /= 3;
			if (a_ptr->pval > 2) a_ptr->pval = 2;
		} else {
			//if (a_ptr->pval > 3) a_ptr->pval /= 2;
			if (a_ptr->pval > 3) a_ptr->pval = 3;
		}
		if (a_ptr->pval == 0) a_ptr->pval = 1;
	}

	/* Never increase stats too greatly */
	if (c) {
		/* limit +stats to 15 (3*(+5) or 5*(+3)),
		   never more than +3 on amulets */
		/* Items with only 1 stat may greatly increase it */
		if ((a_ptr->tval == TV_AMULET) || (c > 3)) {
			if (a_ptr->pval > 3) a_ptr->pval = (a_ptr->pval + 1) / 2;
			if (a_ptr->pval > 3) a_ptr->pval = 3;
			if (a_ptr->pval == 0) a_ptr->pval = 1 + rand_int(2);
		} else {
			if (a_ptr->pval > 5) a_ptr->pval = (a_ptr->pval + 2) / 2;
			if (a_ptr->pval > 5) a_ptr->pval = 5;
			if (a_ptr->pval == 0) a_ptr->pval = 1 + rand_int(3);
		}
	}

/* -------------------------------------- pval-fixing limits -------------------------------------- */

	/* Never more than +3 LIFE (currently some armour only) or +3 EA */
	if ((a_ptr->flags1 & (TR1_LIFE | TR1_BLOWS)) && (a_ptr->pval > 3)) a_ptr->pval = 3;
	/* Never have super EA _and_ LIFE at the same time o_o */
	if ((a_ptr->flags1 & TR1_LIFE) && (a_ptr->flags1 & TR1_BLOWS) && (a_ptr->pval > 1)) a_ptr->pval = 1;

	if ((a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN)) {
		/* Not more than +6 IV on helms and crowns */
		if ((a_ptr->flags1 & TR1_INFRA) && (a_ptr->pval > 6)) a_ptr->pval = 6;
		/* Not more than +3 speed on helms/crowns */
		if ((a_ptr->flags1 & TR1_SPEED) && (a_ptr->pval > 3)) a_ptr->pval = 3;
	}
	
	/* Limits for +MANA */
	if (a_ptr->flags1 & TR1_MANA) {
		/* Randart mage staves may give up to +10 +1 bonus MANA */
		if ((a_ptr->tval == TV_MSTAFF) && (a_ptr->pval >= 11)) a_ptr->pval = 11;
		/* Helms and crowns may not give more than +3 MANA */
		else if ((a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN) &&
		    (a_ptr->pval > 3)) a_ptr->pval = 3;
		/* Usually +10 MANA is max */
		else if (a_ptr->pval > 10) a_ptr->pval = 10;
	}

	/* Limit speed on 1-hand weapons and shields (balances both, dual-wiel and 2-handed weapons) */
	/* Limit +LIFE to +1 under same circumstances */
	if (k_ptr->tval == TV_SHIELD || is_melee_weapon(k_ptr->tval)) {
		if (!(k_ptr->flags4 & (TR4_SHOULD2H | TR4_MUST2H))) {
			if ((a_ptr->flags1 & TR1_SPEED) && (a_ptr->pval > 3)) a_ptr->pval = 3;
			if ((a_ptr->flags1 & TR1_LIFE) && (a_ptr->pval > 1)) a_ptr->pval = 1;
		} else { //must be a 1.5h/2h weapon
			if ((a_ptr->flags1 & TR1_SPEED) && (a_ptr->pval > 6)) a_ptr->pval = 6;
			/* Limit big weapons to +2 LIFE */
			if ((a_ptr->flags1 & TR1_LIFE) && (a_ptr->pval > 2)) a_ptr->pval = 2;
		}
	}

	/* Note: Neither luck nor disarm can actually newly appear on a randart except if coming from k_info. */

	/* Not more than +6 luck (was +5, increased for randart elven cloaks!) */
	if ((a_ptr->flags5 & TR5_LUCK) && (a_ptr->pval > 6)) a_ptr->pval = 6;

	/* Not more than +3 disarming ability (randart picklocks aren't allowed anyways) */
	if ((a_ptr->flags5 & TR5_DISARM) && (a_ptr->pval > 3)) {
		if (!(k_ptr->flags5 & TR5_DISARM)) a_ptr->flags5 &= ~TR5_DISARM;
		else a_ptr->pval = 3;
	}

/* -------------------------------------- Misc unaffecting boni/limits -------------------------------------- */

	/* Hack -- DarkSword randarts should have this */
	if ((a_ptr->tval == TV_SWORD) && (a_ptr->sval == SV_DARK_SWORD)) {
		/* Remove all old ANTIMAGIC flags that might have been
		set by a curse or random ability */
		a_ptr->flags4 &= ((~TR4_ANTIMAGIC_30) & (~TR4_ANTIMAGIC_20) & (~TR4_ANTIMAGIC_10));

		/* Start with basic Antimagic */
		a_ptr->flags4 |= TR4_ANTIMAGIC_50;

		/* If they have large tohit/dam boni they can get more AM even */

		/* If +hit/+dam cancels out base AM.. */
		if ((a_ptr->to_h + a_ptr->to_d) >= 50) {
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
		} else if ((a_ptr->to_h + a_ptr->to_d) >= 40) {
			if (magik(50))
				while (a_ptr->to_h + a_ptr->to_d > 40) {
					a_ptr->to_h--;
					if (a_ptr->to_h + a_ptr->to_d > 40) a_ptr->to_d--;
				}

			if (magik(50)) a_ptr->flags4 |= TR4_ANTIMAGIC_30 | TR4_ANTIMAGIC_10;
			else if (magik(65)) a_ptr->flags4 |= TR4_ANTIMAGIC_30;
			else if (magik(80)) a_ptr->flags4 |= TR4_ANTIMAGIC_20;
			else if (magik(90)) a_ptr->flags4 |= TR4_ANTIMAGIC_10;
		} else if ((a_ptr->to_h + a_ptr->to_d) >= 30) {
			if (magik(50))
				while (a_ptr->to_h + a_ptr->to_d > 30) {
					a_ptr->to_h--;
					if (a_ptr->to_h + a_ptr->to_d > 30) a_ptr->to_d--;
				}

			if (magik(60)) a_ptr->flags4 |= TR4_ANTIMAGIC_30;
			else if (magik(75)) a_ptr->flags4 |= TR4_ANTIMAGIC_20;
			else if (magik(90)) a_ptr->flags4 |= TR4_ANTIMAGIC_10;
		} else if ((a_ptr->to_h + a_ptr->to_d) >= 20) {
			if (magik(50))
				while (a_ptr->to_h + a_ptr->to_d > 20) {
					a_ptr->to_h--;
					if (a_ptr->to_h + a_ptr->to_d > 20) a_ptr->to_d--;
				}

			if (magik(70)) a_ptr->flags4 |= TR4_ANTIMAGIC_20;
			else if (magik(85)) a_ptr->flags4 |= TR4_ANTIMAGIC_10;
		} else if ((a_ptr->to_h + a_ptr->to_d) >= 10) {
			if (magik(50))
			        while (a_ptr->to_h + a_ptr->to_d > 10) {
					a_ptr->to_h--;
					if (a_ptr->to_h + a_ptr->to_d > 10) a_ptr->to_d--;
				}

			if (magik(80)) a_ptr->flags4 |= TR4_ANTIMAGIC_10;
		}
	}

	/* rings that give a base resistance in their base version should prioritize the
	   according type of immunity over going with a different immunity _instead_.
	   Note: we are a bit rough and kill the case of a ring having double-immunity that _both_
	         didn't fit - instead of rearraging only one of them we just discard both.. */
	if (a_ptr->tval == TV_RING && (a_ptr->flags2 & (TR2_IM_ELEC | TR2_IM_COLD | TR2_IM_FIRE | TR2_IM_ACID))) {
		if ((k_ptr->flags2 & TR2_RES_ELEC) && !(a_ptr->flags2 & TR2_IM_ELEC))
			a_ptr->flags2 = TR2_IM_ELEC;
		else if ((k_ptr->flags2 & TR2_RES_COLD) && !(a_ptr->flags2 & TR2_IM_COLD))
			a_ptr->flags2 = TR2_IM_COLD;
		else if ((k_ptr->flags2 & TR2_RES_FIRE) && !(a_ptr->flags2 & TR2_IM_FIRE))
			a_ptr->flags2 = TR2_IM_FIRE;
		else if ((k_ptr->flags2 & TR2_RES_ACID) && !(a_ptr->flags2 & TR2_IM_ACID))
			a_ptr->flags2 = TR2_IM_ACID;
	}
}

/*
 * Returns pointer to randart artifact_type structure.
 *
 * o_ptr should contain the seed (in name3) plus a tval
 * and sval. It returns NULL on illegal sval and tvals.
 */
artifact_type *randart_make(object_type *o_ptr) {
	/*u32b activates;*/
	s32b power;
	int tries, quality_boost = 0;
	s32b ap;
	bool curse_me = FALSE;
	bool aggravate_me = FALSE;

	/* Get pointer to our artifact_type object */
	artifact_type *a_ptr = &randart;

	/* Get pointer to object kind */
	object_kind *k_ptr = &k_info[o_ptr->k_idx];


	/* Set the RNG seed. */
	Rand_value = o_ptr->name3;
	Rand_quick = TRUE;

	/* Screen for disallowed TVALS */
	if ((k_ptr->tval != TV_BOW) &&
	    (k_ptr->tval != TV_BOOMERANG) &&
	    !is_ammo(k_ptr->tval) &&
	    !is_melee_weapon(k_ptr->tval) &&
	    !is_armour(k_ptr->tval) &&
	    (k_ptr->tval != TV_MSTAFF) &&
	    (k_ptr->tval != TV_LITE) &&
	    (k_ptr->tval != TV_RING) &&
	    (k_ptr->tval != TV_AMULET) &&
	    //(k_ptr->tval != TV_DIGGING) &&	 /* better ban it? */
	    //(k_ptr->tval != TV_TOOL) &&
	    //(k_ptr->tval != TV_INSTRUMENT) &&
	    (k_ptr->tval != TV_SPECIAL)) { /* <- forgot this one, else panic save if randart becomes seal, since a_ptr becomes NULL! - C. Blue */
		/* Not an allowed type */
		return(NULL);
	}

	/* Randart ammo doesn't keep (exploding) from normal item */
	if (is_ammo(k_ptr->tval)) k_ptr->pval = 0;

	/* Mega Hack -- forbid randart polymorph rings(pval would be BAD) */
	if ((k_ptr->tval == TV_RING) && (k_ptr->sval == SV_RING_POLYMORPH))
		return (NULL);

	/* Forbid amulets of Telepathic Awareness */
	if (k_ptr->tval == TV_AMULET && k_ptr->sval == SV_AMULET_ESP)
		return (NULL);

	/* Forbid costumes too */
	if ((k_ptr->tval == TV_SOFT_ARMOR) && (k_ptr->sval == SV_COSTUME))
		return (NULL);

/* taken out the quality boosts again, since those weapons already deal insane damage.
   alternatively their damage could be lowered so they don't rival grond (7d8 weapon w/ kill flags..). */
#if 0
	if (k_ptr->flags4 & TR4_SHOULD2H) quality_boost += 15;
	if (k_ptr->flags4 & TR4_MUST2H) quality_boost += 30;
	if (k_ptr->flags4 & TR4_COULD2H) quality_boost += 0;
#endif

	/* Wipe the artifact_type structure */
	WIPE(&randart, artifact_type);


	/*
	 * First get basic artifact quality
	 * 90% are good
	 */

	/* Hack - make nazgul rings of power more useful
	   (and never 'cursed randarts' in the sense of sucking really badly): */
	if ((k_ptr->tval == TV_RING) && (k_ptr->sval == SV_RING_SPECIAL)) {
		power = 65 + rand_int(15) + RANDART_QUALITY; /* 60+rnd(20)+RQ should be maximum */
	} else if (!rand_int(10) || (k_ptr->flags3 & TR3_CURSED)) { /* 10% are cursed */
		power = rand_int(40) - (2 * RANDART_QUALITY);
	} else if (k_ptr->flags5 & TR5_WINNERS_ONLY) {
		power = 30 + rand_int(50) + RANDART_QUALITY; /* avoid very useless WINNERS_ONLY randarts */
	} else {
		/* maybe move quality_boost to become added to randart_quality.. */
		/* note: quality_boost is constantly 0 atm, ie effectless */
		power = rand_int(80 + quality_boost) + RANDART_QUALITY;
	}
	if (power < 0) curse_me = TRUE;

	/* Really powerful items should aggravate. */
	if (power > 100 + quality_boost) {
		if (rand_int(100) < (power - 100 - quality_boost) * 3) {
			aggravate_me = TRUE;
		}
	}

	if (is_ammo(k_ptr->tval) || /* ammo never aggravates */
	    ((k_ptr->tval == TV_RING) && (k_ptr->sval == SV_RING_SPECIAL))) { /* rings of power would lose their granted invisibility! */
		aggravate_me = FALSE;
		//power /= 3;
	}

	/* Default values */
	a_ptr->cur_num = 0;
	a_ptr->max_num = 1;
	a_ptr->tval = k_ptr->tval;
	a_ptr->sval = k_ptr->sval;
	//a_ptr->pval = k_ptr->pval;	/* unsure about this */

	/* 'Merge' pval and bpval into a single value, by just discarding
	   o_ptr->bpval, and using only a_ptr->pval, which becomes o_ptr->pval,
	   otherwise we get things like ring of speed (+7)(+6):
	   Some items that use bpval in a special way are exempt and need to
	   keep their bpval: Rings of Power. */
	if ((k_ptr->tval != TV_RING) || (k_ptr->sval != SV_RING_SPECIAL)) {
		//if (o_ptr->bpval) a_ptr->pval = (o_ptr->pval < o_ptr->bpval)? o_ptr->bpval : o_ptr->pval;
		o_ptr->bpval = 0;
	}

	/* Amulets and Rings keep their (+hit,+dam)[+AC] instead of having it
	   reset to (+0,+0)[+0] (since those boni are hard-coded for jewelry) */
	if ((a_ptr->tval == TV_AMULET) || (a_ptr->tval == TV_RING)) {
		a_ptr->to_h = o_ptr->to_h;
		a_ptr->to_d = o_ptr->to_d;
		a_ptr->to_a = o_ptr->to_a;
#ifdef RANDART_WEAPON_BUFF
	} else if (is_melee_weapon(a_ptr->tval) || a_ptr->tval == TV_BOOMERANG) {
		/* normalise +hit,+dam to somewhat more buffed values for all art weapons */
		a_ptr->to_h = 0;//k_ptr->to_h / 2;
		a_ptr->to_d = 0;//k_ptr->to_d / 2;

		a_ptr->to_a = k_ptr->to_a;
#endif
	} else {
		/* Get base +hit,+dam,+ac from k_info for any item type, to start out with this. */
#ifdef RANDART_WEAPON_BUFF
		/* note: we assume that shooters don't have big k_info +hit/+dam,
		   or we'd have to move them up there to is_melee_weapon() block probably. */
#endif
		a_ptr->to_h = k_ptr->to_h;
		a_ptr->to_d = k_ptr->to_d;
		a_ptr->to_a = k_ptr->to_a;
	}

	/* keep all other base attributes */
	a_ptr->ac = k_ptr->ac;
	a_ptr->dd = k_ptr->dd;
	a_ptr->ds = k_ptr->ds;
	a_ptr->weight = k_ptr->weight;
	a_ptr->flags1 = k_ptr->flags1;
	a_ptr->flags2 = k_ptr->flags2;
	a_ptr->flags3 = k_ptr->flags3;
	a_ptr->flags3 |= (TR3_IGNORE_ACID | TR3_IGNORE_ELEC | TR3_IGNORE_FIRE | TR3_IGNORE_COLD);
	a_ptr->flags4 = k_ptr->flags4;
	a_ptr->flags5 = k_ptr->flags5;
	a_ptr->flags5 |= TR5_IGNORE_WATER;
	a_ptr->flags6 = k_ptr->flags6;
#ifdef ART_WITAN_STEALTH
	if (o_ptr->tval == TV_BOOTS && o_ptr->sval == SV_PAIR_OF_WITAN_BOOTS)
		a_ptr->flags1 &= ~(TR1_STEALTH);
#endif


	/* Ensure weapons have some bonus to hit & dam */
#ifdef RANDART_WEAPON_BUFF
	if (is_melee_weapon(a_ptr->tval) || a_ptr->tval == TV_BOOMERANG) {
		/* emphasise non-low values */
 #if 0		/* relatively steep start -- y=20-(125/(5+x)-5), 0<=x<=20 */
		a_ptr->to_d += 20 - (125 / (5 + rand_int(21)) - 5);
		a_ptr->to_h += 20 - (125 / (5 + rand_int(21)) - 5);
 #endif
 #if 1 /* not so steep start -- y=21-(560/(20+x)-7), 0<=x<=60 */
		a_ptr->to_d += 21 - (560 / (20 + rand_int(61)) - 7);
		a_ptr->to_h += 21 - (560 / (20 + rand_int(61)) - 7);
 #endif
	} else if (a_ptr->tval == TV_DIGGING || a_ptr->tval == TV_BOW) {
		a_ptr->to_d += rand_int(21);
		a_ptr->to_h += rand_int(21);
	}
#else
	if (is_melee_weapon(a_ptr->tval) || a_ptr->tval == TV_BOOMERANG ||
	    a_ptr->tval == TV_DIGGING || a_ptr->tval == TV_BOW) {
		a_ptr->to_d += 1 + rand_int(20);
		a_ptr->to_h += 1 + rand_int(20);
	}
#endif

	/* Ensure armour has some decent bonus to ac - C. Blue */
	if (is_armour(k_ptr->tval)) {
		/* Fixed bonus to avoid useless randarts */
		a_ptr->to_a = 10 + rand_int(6);

		/* pay respect to k_info +ac bonus */
		a_ptr->to_a += k_ptr->to_a / 2;

		/* higher level armour will get additional bonus :-o (mith110, adam120) */
		a_ptr->to_a += k_ptr->level / 15;

		/* fix limit */
#ifndef TO_AC_CAP_30
		if (a_ptr->to_a > 35) a_ptr->to_a = 35;
#else
		if (a_ptr->to_a > 30) a_ptr->to_a = 30;
#endif

		/* Hack: Account for the o_ptr immunities of MHDSMs: */
		if (k_ptr->tval == TV_DRAG_ARMOR && k_ptr->sval == SV_DRAGON_MULTIHUED) {
			if ((o_ptr->xtra2 & 0x1)) a_ptr->flags2 |= TR2_IM_FIRE;
			if ((o_ptr->xtra2 & 0x2)) a_ptr->flags2 |= TR2_IM_COLD;
			if ((o_ptr->xtra2 & 0x4)) a_ptr->flags2 |= TR2_IM_ELEC;
			if ((o_ptr->xtra2 & 0x8)) a_ptr->flags2 |= TR2_IM_ACID;
			if ((o_ptr->xtra2 & 0x10)) a_ptr->flags5 |= TR5_IM_POISON;
		}
	}

#ifdef USE_NEW_SHIELDS
 #ifndef NEW_SHIELDS_NO_AC
	/* Shields always get maximum of 15 */
	if (k_ptr->tval == TV_SHIELD) a_ptr->to_a = 15;
 #else
	if (k_ptr->tval == TV_SHIELD) a_ptr->to_a = 0;
 #endif
#endif

	/* Art ammo doesn't get great hit/dam in general. */
	if (is_ammo(a_ptr->tval)) {
		o_ptr->xtra9 = 0; //clear 'mark as unsellable' hack.. as if^^
		a_ptr->to_d = 0;
		a_ptr->to_h = 0;
		if (magik(50)) {
			a_ptr->to_d += randint(3);
			if (magik(50)) a_ptr->to_d += randint(3);
		}
		if (magik(50)) {
			a_ptr->to_h += randint(6);
			if (magik(50)) a_ptr->to_h += randint(6);
		}
		if (magik(20)) a_ptr->ds += 1;
		else if (magik(20)) a_ptr->dd += 1;

		/* exploding art ammo is very rare - note: magic ammo can't explode */
		if (magik(10) && (a_ptr->sval != SV_AMMO_MAGIC)) {
			int power[27]= { GF_ELEC, GF_POIS, GF_ACID,
			GF_COLD, GF_FIRE, GF_PLASMA, GF_LITE,
			GF_DARK, GF_SHARDS, GF_SOUND,
			GF_CONFUSION, GF_FORCE, GF_INERTIA,
			GF_MANA, GF_METEOR, GF_ICE, GF_CHAOS,
			GF_NETHER, GF_NEXUS, GF_TIME,
			GF_GRAVITY, GF_KILL_WALL, GF_AWAY_ALL,
			GF_TURN_ALL, GF_NUKE, //GF_STUN,
			GF_DISINTEGRATE, GF_HELL_FIRE };
			a_ptr->pval = power[rand_int(27)];
		}
	}

	/* First draft: add two abilities, then curse it three times. */
	if (curse_me) {
		add_ability(a_ptr);
		add_ability(a_ptr);
		do_curse(a_ptr);
		do_curse(a_ptr);
		do_curse(a_ptr);
		remove_contradictory(a_ptr, (a_ptr->flags3 & TR3_AGGRAVATE) != 0);
		remove_redundant_esp(a_ptr);
		ap = artifact_power(a_ptr);
	} else if (is_ammo(k_ptr->tval)) {
		add_ability (a_ptr);
		if (magik(50)) add_ability(a_ptr);
		if (magik(25)) add_ability(a_ptr);
		if (magik(10)) add_ability(a_ptr);
		remove_contradictory(a_ptr, FALSE);
		remove_redundant_esp(a_ptr);
		ap = artifact_power(a_ptr) + RANDART_QUALITY + 15; /* in general ~5k+(40-40)+15k value */
	} else { /* neither cursed, nor ammo: */
		artifact_type a_old;

		/* Select a random set of abilities which roughly matches the
		   original's in terms of overall power/usefulness. */
		for (tries = 0; tries < MAX_TRIES; tries++) {
			/* Copy artifact info temporarily. */
			a_old = *a_ptr;
			add_ability(a_ptr);

			remove_contradictory(a_ptr, aggravate_me);
			remove_redundant_esp(a_ptr);
			/* Moved limit-fixing here experimentally! */
			artifact_fix_limits_inbetween(a_ptr, k_ptr);

			ap = artifact_power(a_ptr);

			if (ap > (power * 11) / 10 + 1) {
				/* too powerful -- put it back */
				*a_ptr = a_old;
				continue;
			} else if (ap >= (power * 9) / 10) break; /* just right */
			/* Stop if we're going negative, so we don't overload
			   the artifact with great powers to compensate: */
			else if ((ap < 0) && (ap < (-(power * 1)) / 10)) break;
		} /* end of power selection */

#if 0 /* disallow such randarts that haven't gained any extra powers over base item version? */
		/* should almost never happen: Rolled a 'too powerful' artifact on _every_ attempt.
		   This would require a base item that is already extremely powerful (eg PDSM). */
		if (tries == MAX_TRIES)
			/* fail randart generation completely */
			return (NULL);
#else /* allow them */
		if (tries == MAX_TRIES)
			/* just use the absolute base randart, ie without additional ability mods
			   except for those hard-wired like for ammo or cursed ones. - C. Blue
			   I wonder whether a cursed PDSM randart could thereby get extra mods.. :) */
			*a_ptr = a_old;
#endif

		if (aggravate_me) {
			a_ptr->flags3 |= TR3_AGGRAVATE;
			a_ptr->flags1 &= ~(TR1_STEALTH);
			a_ptr->flags5 &= ~(TR5_INVIS);
			ap = artifact_power(a_ptr); /* recalculate, to calculate proper price and level below.. */
		}
	}

	/* Fixing final limits */
	artifact_fix_limits_afterwards(a_ptr, k_ptr);

	a_ptr->cost = (ap - RANDART_QUALITY + 50);
	if (a_ptr->cost < 0) {
		a_ptr->cost = 0;
	} else {
		a_ptr->cost = (ap * ap * ap) / 15;
	}

	/* NOTE: a_ptr->level is only the base level. Apply_magic as well as
	   create_artifact_aux execute a 'determine_level_req' on randarts
	   after calling make_artifact/creating the seed, so the _real_ level
	   reqs will base on this value, but not be the same. Just FYI ^^ - C. Blue */
	a_ptr->level = (curse_me ? (ap < -20 ? -ap : (ap > 20 ? ap : 15 + ABS(ap))) : ap);

	/* hack for randart ammunition: usually, its _final_ level would be around 35 if artscroll'ed.
	   however, let's use a more specific routine for ammo (note that it's _base_ level): */
	if (is_ammo(k_ptr->tval)) {
		int cost = 0;
		/* in general: SLAYs, BRANDs, +dam, +dice all weigh much */
		if (a_ptr->flags1 & TR1_VAMPIRIC) {a_ptr->level += 6; cost += 30000;}
		if (a_ptr->flags1 & TR1_SLAY_EVIL) {a_ptr->level += 6; cost += 40000;}
		if (a_ptr->flags1 & TR1_SLAY_ANIMAL) {a_ptr->level += 3; cost += 15000;}
		if (a_ptr->flags1 & TR1_KILL_UNDEAD) {a_ptr->level += 6; cost += 35000;}
		else if (a_ptr->flags1 & TR1_SLAY_UNDEAD) {a_ptr->level += 2; cost += 15000;}
		if (a_ptr->flags1 & TR1_KILL_DRAGON) {a_ptr->level += 6; cost += 35000;}
		else if (a_ptr->flags1 & TR1_SLAY_DRAGON) {a_ptr->level += 2; cost += 20000;}
		if (a_ptr->flags1 & TR1_KILL_DEMON) {a_ptr->level += 7; cost += 50000;}
		else if (a_ptr->flags1 & TR1_SLAY_DEMON) {a_ptr->level += 4; cost += 30000;}
		if (a_ptr->flags1 & TR1_SLAY_TROLL) {a_ptr->level += 1; cost += 3500;}
		if (a_ptr->flags1 & TR1_SLAY_ORC) {a_ptr->level += 1; cost += 2000;}
		if (a_ptr->flags1 & TR1_SLAY_GIANT) {a_ptr->level += 2; cost += 6000;}
		if (a_ptr->flags1 & TR1_BRAND_ACID) {a_ptr->level += 4; cost += 25000;}
		if (a_ptr->flags1 & TR1_BRAND_ELEC) {a_ptr->level += 4; cost += 25000;}
		if (a_ptr->flags1 & TR1_BRAND_FIRE) {a_ptr->level += 3; cost += 15000;}
		if (a_ptr->flags1 & TR1_BRAND_COLD) {a_ptr->level += 2; cost += 15000;}
		if (a_ptr->flags1 & TR1_BRAND_POIS) {a_ptr->level += 2; cost += 10000;}
		if (a_ptr->dd > k_ptr->dd) {a_ptr->level += 4; cost += 20000;}
		if (a_ptr->ds > k_ptr->ds) {a_ptr->level += 3; cost += 15000;}
		a_ptr->level += a_ptr->to_h / 2;
		cost += a_ptr->to_h * 3000;
		a_ptr->level += a_ptr->to_d;
		cost += a_ptr->to_d * 8000;
		switch(a_ptr->pval) {
		case GF_POIS: case GF_COLD: a_ptr->level += 2; cost += 10000; break;
		case GF_FIRE: case GF_ELEC: case GF_ACID: a_ptr->level += 2; cost += 15000; break;
		case GF_LITE: case GF_DARK: case GF_ICE: case GF_SHARDS: case GF_NUKE: a_ptr->level += 2; cost += 20000; break;
		case GF_MANA: case GF_METEOR: case GF_CHAOS: case GF_NETHER: case GF_HELL_FIRE: case GF_TIME: a_ptr->level += 5; cost += 30000; break;
		case GF_CONFUSION: case GF_INERTIA: a_ptr->level += 4; cost += 20000; break;
		case GF_NEXUS: case GF_GRAVITY: case GF_AWAY_ALL: case GF_TURN_ALL: a_ptr->level += 5; cost += 15000; break;
		case GF_PLASMA: case GF_SOUND: case GF_FORCE: case GF_STUN: a_ptr->level += 6; cost += 35000; break;
		case GF_KILL_WALL: a_ptr->level += 6; cost += 15000; break;
		case GF_DISINTEGRATE: a_ptr->level += 7; cost += 30000; break;
		}
		/* also hack their value */
		if (a_ptr->cost) a_ptr->cost = (cost * 4) / 5;
        }

	if (a_ptr->cost < 0) a_ptr->cost = 0;

#if 0
	/* One last hack: if the artifact is very powerful, raise the rarity.
	   This compensates for artifacts like (original) Bladeturner, which
	   have low artifact rarities but came from extremely-rare base
	   kinds. */
	if ((ap > 0) && ((ap / 8) > a_ptr->rarity))
		a_ptr->rarity = ap / 8;

	/*if (activates) a_ptr->flags3 |= TR3_ACTIVATE;*/
	/*if (a_idx < ART_MIN_NORMAL) a_ptr->flags3 |= TR3_INSTA_ART;*/
#endif /* if 0 */

	/* Add TR3_HIDE_TYPE to all artifacts with nonzero pval because we're
	   too lazy to find out which ones need it and which ones don't. */
	if (a_ptr->pval) a_ptr->flags3 |= TR3_HIDE_TYPE;

	/* Restore RNG */
	Rand_quick = FALSE;

	/* Return a pointer to the artifact_type */
	return (a_ptr);
}


/*
 * Make random artifact name.
 */
void randart_name(object_type *o_ptr, char *buffer, char *raw_buffer) {
	char tmp[MAX_CHARS];

	/* Set the RNG seed. It this correct. Should it be restored??? XXX */
	Rand_value = o_ptr->name3;
	Rand_quick = TRUE;

	/* Take a random name */
	//o_ptr->name4 = get_rnd_line("randarts.txt", 0, tmp, MAX_CHARS);
	/* Faster version */
	o_ptr->name4 = get_rnd_line_from_memory(randart_names, num_randart_names, tmp, MAX_CHARS);

	/* Capitalise first character */
	tmp[0] = toupper(tmp[0]);

	/* for normal object description */
	if (buffer) {
		/* Either "sword of something" or
		 * "sword 'something'" form */
		if (rand_int(2)) sprintf(buffer, "of %s", tmp);
		else sprintf(buffer, "'%s'", tmp);
	}
	/* for true arts in EQUIPMENT_SET_BONUS */
	if (raw_buffer) strcpy(raw_buffer, tmp);

	/* Restore RNG */
	Rand_quick = FALSE;

	return;
}


void apply_enchantment_limits(object_type *o_ptr) {
	/* not too high to-hit/to-dam boni. No need to check gloves. */
	switch (o_ptr->tval) { /* CAP_ITEM_BONI */
	case TV_SHIELD:
#ifdef USE_NEW_SHIELDS	/* should actually be USE_BLOCKING, but could be too */
			/* dramatic a change if it gets enabled temporarily - C. Blue */
 #ifndef NEW_SHIELDS_NO_AC
		if (o_ptr->to_a > 15) o_ptr->to_a = 15;
 #else
		o_ptr->to_a = 0;
 #endif
		return;
#endif
	case TV_SOFT_ARMOR: case TV_HARD_ARMOR: case TV_DRAG_ARMOR:
	case TV_CLOAK: case TV_HELM: case TV_CROWN: case TV_GLOVES: case TV_BOOTS:
#ifndef TO_AC_CAP_30
		if (o_ptr->to_a > 35) o_ptr->to_a = 35;
#else
		if (o_ptr->to_a > 30) o_ptr->to_a = 30;
#endif
		return;

	case TV_BOLT:
	case TV_ARROW:
	case TV_SHOT:
		if (o_ptr->to_h > 15) o_ptr->to_h = 15;
		if (o_ptr->to_d > 15) o_ptr->to_d = 15;
		return;

	case TV_TRAPKIT:
		if (!is_firearm_trapkit(o_ptr->sval)) {
			o_ptr->to_h = o_ptr->to_d = 0;
			return;
		}
	case TV_BOW:
	case TV_BOOMERANG:
	default: /* all melee weapons */
		if (o_ptr->to_h > 30) o_ptr->to_h = 30;
		if (o_ptr->to_d > 30) o_ptr->to_d = 30;
		return;
	}
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
artifact_type *ego_make(object_type *o_ptr) {
	ego_item_type *e_ptr;
	int j, rr, granted_pval;
	bool limit_blows = FALSE;
	//u32b f1, f2, f3, f4, f5, f6, esp;
	s16b e_idx;

	/* Hack -- initialize bias */
	artifact_bias = 0;

	/* Get pointer to our artifact_type object */
	artifact_type *a_ptr = &randart;

	/* Get pointer to object kind */
	object_kind *k_ptr = &k_info[o_ptr->k_idx];


	/* Set the RNG seed. */
	Rand_value = o_ptr->name3;
	Rand_quick = TRUE;

	/* Wipe the artifact_type structure */
	WIPE(&randart, artifact_type);
	a_ptr->tval = k_ptr->tval;
	a_ptr->sval = k_ptr->sval;

	e_idx = o_ptr->name2;

	/* Ok now, THAT is truly ugly */
try_an_other_ego:
	e_ptr = &e_info[e_idx];

	/* Hack -- extra powers */
	for (j = 0; j < 5; j++) {
		/* Rarity check */
		if (magik(e_ptr->rar[j])) {
			a_ptr->flags1 |= e_ptr->flags1[j];
			a_ptr->flags2 |= e_ptr->flags2[j];
			a_ptr->flags3 |= e_ptr->flags3[j];
			a_ptr->flags4 |= e_ptr->flags4[j];
			a_ptr->flags5 |= e_ptr->flags5[j];
			a_ptr->flags6 |= e_ptr->flags6[j];
			a_ptr->esp |= e_ptr->esp[j];
			add_random_ego_flag(a_ptr, e_ptr->fego1[j], e_ptr->fego2[j], &limit_blows, o_ptr->level, o_ptr);
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
		if (a_ptr->esp & R_ESP_ANY) {
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
#endif

	/* Hack -- obtain bonuses */
	if (e_ptr->max_to_h > 0) a_ptr->to_h += randint(e_ptr->max_to_h);
	if (e_ptr->max_to_h < 0) a_ptr->to_h -= randint(-e_ptr->max_to_h);
	if (e_ptr->max_to_d > 0) a_ptr->to_d += randint(e_ptr->max_to_d);
	if (e_ptr->max_to_d < 0) a_ptr->to_d -= randint(-e_ptr->max_to_d);
	if (e_ptr->max_to_a > 0) a_ptr->to_a += randint(e_ptr->max_to_a);
	if (e_ptr->max_to_a < 0) a_ptr->to_a -= randint(-e_ptr->max_to_a);

	/* No insane number of blows */
	if (limit_blows && (a_ptr->flags1 & TR1_BLOWS)) {
		if (a_ptr->pval > 2)
			a_ptr->pval -= randint(a_ptr->pval - 2);
	}


	/* --- Obtain granted minimum pval --- */
	granted_pval = 0;
	/* Mage staves have pvals minima */
	if ((o_ptr->tval == TV_MSTAFF) && (o_ptr->sval == SV_MSTAFF)) {
		if ((o_ptr->name2b == 2)||(o_ptr->name2 == 2)) granted_pval = 4;
		if ((o_ptr->name2b == 3)||(o_ptr->name2 == 3)) granted_pval = 7;
	}
	/* Elvenkind boots are more likely to get good pval, ugh */
	if (e_idx == EGO_ELVENKIND2) granted_pval = rand_int(e_ptr->max_pval - 2);
	/* Enchanted lanterns shouldn't be terrible */
	if (e_idx == EGO_ENCHANTED) granted_pval = 2;


	/* Hack -- obtain pval */
	if (e_ptr->max_pval > 0) a_ptr->pval += granted_pval + randint(e_ptr->max_pval - granted_pval);
	if (e_ptr->max_pval < 0) a_ptr->pval -= (granted_pval + randint(-e_ptr->max_pval - granted_pval));

	/* Remove redundant ESP and contradictory flags */
	remove_contradictory(a_ptr, (a_ptr->flags3 & TR3_AGGRAVATE) != 0);
	remove_redundant_esp(a_ptr);

	/* Hack -- apply rating bonus(it's done in apply_magic) */
	//rating += e_ptr->rating;

#if 1	/* double-ego code.. future pleasure :) */
	if (o_ptr->name2b && (o_ptr->name2b != e_idx)) {
		e_idx = o_ptr->name2b;
		goto try_an_other_ego;
	}
#endif

	/* Fix some limits */
	/* Never have more than +15 bonus */
	if (!is_ammo(a_ptr->tval) && a_ptr->pval > 15) a_ptr->pval = 15;
	/* Mage Staves don't have NO_MAGIC */
	if (o_ptr->tval == TV_MSTAFF) a_ptr->flags3 &= ~TR3_NO_MAGIC;
	/* Dark Swords don't have MANA (or SPELL) flag */
	if (o_ptr->tval == TV_SWORD && o_ptr->sval == SV_DARK_SWORD) {
		a_ptr->flags1 &= ~TR1_MANA;
		a_ptr->flags1 &= ~TR1_SPELL;
	}
	/* Items of/with 'Magi'/'Istari' and BLESSED/REGEN_MANA don't have NO_MAGIC: */
	if ((a_ptr->flags3 & TR3_NO_MAGIC) && !(k_ptr->flags3 & TR3_NO_MAGIC)) {
		/* If an item gives BLESSED, remove NO_MAGIC property */
		if (a_ptr->flags3 & TR3_BLESSED) a_ptr->flags3 &= ~TR3_NO_MAGIC;
		/* If an item gives REGEN_MANA, remove NO_MAGIC property */
		else if (a_ptr->flags5 & TR5_REGEN_MANA) a_ptr->flags3 &= ~TR3_NO_MAGIC;
		/* Ego powers which are named in a way so that you shoudldn't expect no-magic on them */
		else if ((o_ptr->name2 == EGO_MAGI || o_ptr->name2b == EGO_MAGI) || /* crowns */
		    (o_ptr->name2 == EGO_LITE_MAGI || o_ptr->name2b == EGO_LITE_MAGI) ||
		    (o_ptr->name2 == EGO_CLOAK_MAGI || o_ptr->name2b == EGO_CLOAK_MAGI) ||
		    (o_ptr->name2 == EGO_ISTARI || o_ptr->name2b == EGO_ISTARI) || /* gloves */
		    (o_ptr->name2 == EGO_OFTHEMAGI || o_ptr->name2b == EGO_OFTHEMAGI) || /* gloves */
		    (o_ptr->name2 == EGO_ROBE_MAGI || o_ptr->name2b == EGO_ROBE_MAGI))
			a_ptr->flags3 &= ~TR3_NO_MAGIC;
	}
	/* If an item increases all three, SPEED, CRIT, MANA,
	   then reduce pval to 1/2 to balance */
	if ((a_ptr->flags1 & TR1_SPEED) && (a_ptr->flags5 & TR5_CRIT) && (a_ptr->flags1 & TR1_MANA)) {
		a_ptr->pval /= 2;
		if (!a_ptr->pval) a_ptr->pval = 1;
	}
	/* If an item increases two of SPEED, CRIT, MANA by over 7
	   then reduce pval to 2/3 to balance */
	else if ((((a_ptr->flags1 & TR1_SPEED) && (a_ptr->flags5 & TR5_CRIT)) ||
	    ((a_ptr->flags1 & TR1_SPEED) && (a_ptr->flags1 & TR1_MANA)) ||
	    ((a_ptr->flags1 & TR1_MANA) && (a_ptr->flags5 & TR5_CRIT)))) {
		a_ptr->pval = (a_ptr->pval * 2) / 3;
		if (!a_ptr->pval) a_ptr->pval = 1;
	}
	/* While +MANA is capped at 10 for randarts, it's 12 for egos(!) */
	if ((a_ptr->flags1 & TR1_MANA) && a_ptr->pval > 12) a_ptr->pval = 12;
	/* Stealth cap; stealth/speed cap on all items except boots (for 'of Swiftness') */
	if (a_ptr->flags1 & TR1_STEALTH) {
		/* Don't limit elvenkind boots */
		if ((a_ptr->flags1 & TR1_SPEED) && o_ptr->tval != TV_BOOTS) {
			if (a_ptr->pval > 4) a_ptr->pval = 3 + rand_int(2);
		} else {
			if (a_ptr->pval > 6) a_ptr->pval = 6;
		}
	}
	/* Speed cap for gnomish cloaks of magi/bat (check for consistency with above check) */
	if ((a_ptr->flags1 & TR1_SPEED)
	    && is_armour(a_ptr->tval) && a_ptr->tval != TV_BOOTS
	    && a_ptr->pval > 4)
		a_ptr->pval = 4;
	if ((a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN)) {
		/* Not more than +6 IV on helms and crowns */
		if ((a_ptr->flags1 & TR1_INFRA) && (a_ptr->pval > 6)) a_ptr->pval = 6;
		/* Not more than +3 speed on helms/crowns */
		if ((a_ptr->flags1 & TR1_SPEED) && (a_ptr->pval > 3)) a_ptr->pval = 3;
	}
	/* Luck/Disarm caps */
	if(a_ptr->flags5 & TR5_LUCK) {
		if (a_ptr->pval > 5) a_ptr->pval = 5;
	}
	if(a_ptr->flags5 & TR5_DISARM) {
		if (a_ptr->pval > 3) a_ptr->pval = 3;
	}
	/* +Attribute caps */
	if (a_ptr->flags1 & (TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR)) {
		if (a_ptr->tval == TV_AMULET) {
			if (a_ptr->pval > 3) a_ptr->pval = (a_ptr->pval + 1) / 2;
			if (a_ptr->pval > 3) a_ptr->pval = 3;
			if (a_ptr->pval == 0) a_ptr->pval = 1;
		} else {
			if (a_ptr->pval > 5) a_ptr->pval = (a_ptr->pval + 2) / 2;
			if (a_ptr->pval > 5) a_ptr->pval = 5;
			if (a_ptr->pval == 0) a_ptr->pval = 1;
		}
	}
	/* +EA caps */
	if (a_ptr->flags1 & TR1_BLOWS) {
		if (o_ptr->tval == TV_GLOVES) {
			//if (a_ptr->pval > 2) a_ptr->pval /= 3;
			if (a_ptr->pval > 2) a_ptr->pval = 2;
		} else {
			//if (a_ptr->pval > 3) a_ptr->pval /= 2;
			if (a_ptr->pval > 3) a_ptr->pval = 3;
		}
		if (a_ptr->pval == 0) a_ptr->pval = 1;
	}
	/* LIFE and BLOWS on same item cap pval at +1 */
	if ((a_ptr->flags1 & TR1_LIFE) && (a_ptr->flags1 & TR1_BLOWS) && a_ptr->pval > 1) a_ptr->pval = 1;

	apply_enchantment_limits(o_ptr);
#if 0 /* removed LIFE from VAMPIRIC items for now */
	/* Back Hack :( */
	if ((o_ptr->name2 == EGO_VAMPIRIC || o_ptr->name2b == EGO_VAMPIRIC) &&
	    o_ptr->pval >= 0 && o_ptr->bpval >= 0) {
		if (o_ptr->bpval > 0) o_ptr->bpval = o_ptr->bpval > 2 ? -2 : 0 - o_ptr->bpval;
		else o_ptr->pval = o_ptr->pval > 2 ? -2 : 0 - o_ptr->pval;
	}
#endif
	/* Limit +EA to +3 */
	if ((a_ptr->flags1 & TR1_BLOWS) && (a_ptr->pval > 3)) a_ptr->pval = 3;
	/* Limit +LIFE on 1-hand weapons +2 (balances both, dual-wiel and 2-handed weapons) */
	if (a_ptr->flags1 & TR1_LIFE) {
		if (o_ptr->tval == TV_SHIELD ||
		    (is_melee_weapon(o_ptr->tval) &&
		    !(k_ptr->flags4 & TR4_SHOULD2H) && !(k_ptr->flags4 & TR4_MUST2H))
		    /* Limit +LIFE on armour (including shields) to +1 (in case of shields balancing dual/2h wield) */
		    || is_armour(o_ptr->tval)) {
			if (a_ptr->pval > 1) a_ptr->pval = 1;
		/* 2-/1.5-handed weapons may get +2 LIFE */
		} else if (a_ptr->pval > 2) a_ptr->pval = 2;
	}

	/* Restore RNG */
	Rand_quick = FALSE;

	/* Return a pointer to the artifact_type */
	return (a_ptr);
}
static void add_random_esp(artifact_type *a_ptr, int all) {
	int rr = rand_int(25 + all);
	if (rr < 1) a_ptr->esp |= (ESP_SPIDER);
	else if (rr < 2) a_ptr->esp |= (ESP_ORC);
	else if (rr < 3) a_ptr->esp |= (ESP_TROLL);
	else if (rr < 4) a_ptr->esp |= (ESP_GIANT);
	else if (rr < 5) a_ptr->esp |= (ESP_DRAGONRIDER);
	else if (rr < 7) a_ptr->esp |= (ESP_UNIQUE);
	else if (rr < 9) a_ptr->esp |= (ESP_NONLIVING);
	else if (rr < 11) a_ptr->esp |= (ESP_UNDEAD);
	else if (rr < 13) a_ptr->esp |= (ESP_DRAGON);
	else if (rr < 15) a_ptr->esp |= (ESP_DEMON);
	else if (rr < 18) a_ptr->esp |= (ESP_GOOD);
	else if (rr < 21) a_ptr->esp |= (ESP_ANIMAL);
	else if (rr < 25) a_ptr->esp |= (ESP_EVIL);
	else a_ptr->esp |= (ESP_ALL);
}

/* Add a random flag to the ego item */
/*
 * Hack -- 'dlev' is needed to determine some values;
 * I diverted lv-req of item for this purpose, thus changes in o_ptr->level
 * will result in changes of ego-item powers themselves!!	- Jir -
 */
void add_random_ego_flag(artifact_type *a_ptr, u32b fego1, u32b fego2, bool *limit_blows, s16b dlev, object_type *o_ptr) {
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* ----- fego1 flags ----- */

	if (fego1 & ETR1_SUSTAIN) {
		/* Make a random sustain */
		switch(randint(6)) {
		case 1: a_ptr->flags2 |= TR2_SUST_STR; break;
		case 2: a_ptr->flags2 |= TR2_SUST_INT; break;
		case 3: a_ptr->flags2 |= TR2_SUST_WIS; break;
		case 4: a_ptr->flags2 |= TR2_SUST_DEX; break;
		case 5: a_ptr->flags2 |= TR2_SUST_CON; break;
		case 6: a_ptr->flags2 |= TR2_SUST_CHR; break;
		}
	}

	if (fego1 & ETR1_OLD_RESIST) {
		/* Make a random resist, equal probabilities */
		switch (randint(11)) {
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

	if (fego1 & ETR1_ABILITY) {
		/* Choose an ability */
		switch (randint(10)) {
		case 1: a_ptr->flags3 |= (TR3_FEATHER);     break;
		case 2: a_ptr->flags3 |= (TR3_LITE1);        break;
		case 3: a_ptr->flags3 |= (TR3_SEE_INVIS);   break;
		//case 4: a_ptr->esp |= (ESP_ALL);   break;
		case 4: add_random_esp(a_ptr, 5); break;
		case 5: a_ptr->flags3 |= (TR3_SLOW_DIGEST); break;
		case 6: a_ptr->flags3 |= (TR3_REGEN);       break;
		case 7: a_ptr->flags2 |= (TR2_FREE_ACT);    break;
		case 8: a_ptr->flags2 |= (TR2_HOLD_LIFE);   break;
		case 9: a_ptr->flags3 |= (TR3_NO_MAGIC);   break;
		case 10: a_ptr->flags5 |= (TR5_REGEN_MANA);   break;
		}
	}

	if (fego1 & ETR1_LOW_ABILITY) {
		/* Choose an ability */
		switch (randint(10)) {
		case 1: a_ptr->flags3 |= (TR3_FEATHER);     break;
		case 2: a_ptr->flags3 |= (TR3_LITE1);        break;
		case 3: a_ptr->flags3 |= (TR3_SEE_INVIS);   break;
		//case 4: a_ptr->esp |= (ESP_ALL);   break;
		case 4: add_random_esp(a_ptr, -4); break;
		case 5: a_ptr->flags3 |= (TR3_SLOW_DIGEST); break;
		case 6: a_ptr->flags3 |= (TR3_REGEN);       break;
		case 7: a_ptr->flags2 |= (TR2_FREE_ACT);    break;
		case 8: a_ptr->flags2 |= (TR2_HOLD_LIFE);   break;
		case 9: a_ptr->flags3 |= (TR3_NO_MAGIC);   break;
		case 10: a_ptr->flags5 |= (TR5_REGEN_MANA);   break;
		}
	}

	if (fego1 & ETR1_R_ELEM) {
		/* Make an acid/elec/fire/cold/poison resist */
		random_resistance(a_ptr, FALSE, randint(14) + 4);
	}
	if (fego1 & ETR1_R_LOW) {
		/* Make an acid/elec/fire/cold resist */
		random_resistance(a_ptr, FALSE, randint(12) + 4);
	}
	if (fego1 & ETR1_R_HIGH) {
		/* Make a high resist */
		random_resistance(a_ptr, FALSE, randint(22) + 16);
	}
	if (fego1 & ETR1_R_ANY) {
		/* Make any resist */
		random_resistance(a_ptr, FALSE, randint(34) + 4);
	}
	if (fego1 & ETR1_R_DRAGON) {
		/* Make "dragon resist" */
		dragon_resist(a_ptr);
	}
	if (fego1 & ETR1_DAM_DIE) {
		/* Increase damage dice */
		a_ptr->dd++;
	}
	if (fego1 & ETR1_DAM_SIZE) {
		/* Increase damage dice size */
		a_ptr->ds++;
	}
	if (fego1 & ETR1_SLAY_WEAP) {
		/* Make a Weapon of Slaying */

		if (randint(3) == 1) { /* double damage */
			a_ptr->dd = k_ptr->dd;
			while ((a_ptr->dd + k_ptr->dd) * (a_ptr->ds + k_ptr->ds + 1) > slay_limit_ego(a_ptr, k_ptr)
			    && (a_ptr->dd > 0))
				a_ptr->dd -= 1; /* No overpowered slaying weapons */
		} else if (randint(2) == 1) {
			while (randint(a_ptr->dd + 1) == 1 &&
			    (1 + a_ptr->dd + k_ptr->dd) * (a_ptr->ds + k_ptr->ds + 1) <= slay_limit_ego(a_ptr, k_ptr))
				/* No overpowered slaying weapons */
				a_ptr->dd++;

			while (randint(a_ptr->ds + 1) == 1 &&
			    (a_ptr->dd + k_ptr->dd) * (1 + a_ptr->ds + k_ptr->ds + 1) <= slay_limit_ego(a_ptr, k_ptr))
				/* No overpowered slaying weapons */
				a_ptr->ds++;
		} else {
			while (randint(a_ptr->ds + 1) == 1 &&
			    (a_ptr->dd + k_ptr->dd) * (1 + a_ptr->ds + k_ptr->ds + 1) <= slay_limit_ego(a_ptr, k_ptr))
				/* No overpowered slaying weapons */
				a_ptr->ds++;

			while (randint(a_ptr->dd + 1) == 1 &&
			    (1 + a_ptr->dd + k_ptr->dd) * (a_ptr->ds + k_ptr->ds + 1) <= slay_limit_ego(a_ptr, k_ptr))
				/* No overpowered slaying weapons */
				a_ptr->dd++;
		}

		if (randint(5) == 1) a_ptr->flags1 |= TR1_BRAND_POIS;

		/*if (k_ptr->tval == TV_SWORD && (randint(4) == 1))*/
		if ((k_ptr->tval != TV_BLUNT) &&
		    !(k_ptr->tval == TV_POLEARM &&
			k_ptr->sval != 3 && k_ptr->sval != 6 && k_ptr->sval != 9 && k_ptr->sval != 13 && k_ptr->sval != 17 && k_ptr->sval != 30
		    ) && (randint(3) == 1))
			a_ptr->flags5 |= TR5_VORPAL;
	}

	if (fego1 & ETR1_LIMIT_BLOWS) {
		/* Swap this flag */
		*limit_blows = !(*limit_blows);
	}
	if (fego1 & ETR1_PVAL_M1) {
		/* Increase pval */
		a_ptr->pval++;
	}
	if (fego1 & ETR1_PVAL_M2) {
		/* Increase pval */
		a_ptr->pval += m_bonus(2, dlev);
	}
	if (fego1 & ETR1_PVAL_M3) {
		/* Increase pval */
		a_ptr->pval += m_bonus(3, dlev);
	}
	if (fego1 & ETR1_PVAL_M5) {
		/* Increase pval */
		a_ptr->pval += m_bonus(5, dlev);
	}
#if 0
	if (fego1 & ETR1_AC_M1) {
		/* Increase ac */
		a_ptr->to_a++;
	}
	if (fego1 & ETR1_AC_M2) {
		/* Increase ac */
		a_ptr->to_a += m_bonus(2, dlev);
	}
	if (fego1 & ETR1_AC_M3) {
		/* Increase ac */
		a_ptr->to_a += m_bonus(3, dlev);
	}
#endif
	if (fego1 & ETR1_AC_M5) {
		/* Increase ac */
		a_ptr->to_a += m_bonus(5, dlev);
	}
#if 0
	if (fego1 & ETR1_TH_M1) {
		/* Increase to hit */
		a_ptr->to_h++;
	}
	if (fego1 & ETR1_TH_M2) {
		/* Increase to hit */
		a_ptr->to_h += m_bonus(2, dlev);
	}
	if (fego1 & ETR1_TH_M3) {
		/* Increase to hit */
		a_ptr->to_h += m_bonus(3, dlev);
	}
	if (fego1 & ETR1_TH_M5) {
		/* Increase to hit */
		a_ptr->to_h += m_bonus(5, dlev);
	}
#endif
#if 0
	if (fego1 & ETR1_TD_M1) {
		/* Increase to dam */
		a_ptr->to_d++;
	}
	if (fego1 & ETR1_TD_M2) {
		/* Increase to dam */
		a_ptr->to_d += m_bonus(2, dlev);
	}

#endif
	if (fego1 & ETR1_R_ESP) {
		add_random_esp(a_ptr, 1);
	}
	if (fego1 & ETR1_NO_SEED) {
		/* Nothing */
	}
#if 0
	if (fego1 & ETR1_TD_M3) {
		/* Increase to dam */
		a_ptr->to_d += m_bonus(3, dlev);
	}
	if (fego1 & ETR1_TD_M5) {
		/* Increase to dam */
		a_ptr->to_d += m_bonus(5, dlev);
	}
#endif
	if (fego1 & ETR1_R_P_ABILITY) {
		/* Add a random pval-affected ability */
		/* This might cause boots with + to blows */
		switch (randint(6)) {
		case 1:a_ptr->flags1 |= TR1_STEALTH; break;
		case 2:a_ptr->flags1 |= TR1_SEARCH; break;
		case 3:a_ptr->flags1 |= TR1_INFRA; break;
		case 4:a_ptr->flags1 |= TR1_TUNNEL; break;
		case 5:a_ptr->flags1 |= TR1_SPEED; break;
		case 6:a_ptr->flags1 |= TR1_BLOWS; break;
		}

	}
	if (fego1 & ETR1_R_STAT) {
		/* Add a random stat */
		switch (randint(6)) {
		case 1:a_ptr->flags1 |= TR1_STR; break;
		case 2:a_ptr->flags1 |= TR1_INT; break;
		case 3:a_ptr->flags1 |= TR1_WIS; break;
		case 4:a_ptr->flags1 |= TR1_DEX; break;
		case 5:a_ptr->flags1 |= TR1_CON; break;
		case 6:a_ptr->flags1 |= TR1_CHR; break;
		}
	}
	if (fego1 & ETR1_R_STAT_SUST) {
		/* Add a random stat and sustain it */
		switch (randint(6)) {
		case 1:
			a_ptr->flags1 |= TR1_STR;
			a_ptr->flags2 |= TR2_SUST_STR;
			break;
		case 2:
			a_ptr->flags1 |= TR1_INT;
			a_ptr->flags2 |= TR2_SUST_INT;
			break;
		case 3:
			a_ptr->flags1 |= TR1_WIS;
			a_ptr->flags2 |= TR2_SUST_WIS;
			break;
		case 4:
			a_ptr->flags1 |= TR1_DEX;
			a_ptr->flags2 |= TR2_SUST_DEX;
			break;
		case 5:
			a_ptr->flags1 |= TR1_CON;
			a_ptr->flags2 |= TR2_SUST_CON;
			break;
		case 6:
			a_ptr->flags1 |= TR1_CHR;
			a_ptr->flags2 |= TR2_SUST_CHR;
			break;
		}
	}
	if (fego1 & ETR1_R_IMMUNITY) {
		/* Give a random immunity */
		switch (randint(4)) {
		case 1:
			a_ptr->flags2 |= TR2_IM_FIRE;
			a_ptr->flags3 |= TR3_IGNORE_FIRE;
			break;
		case 2:
			a_ptr->flags2 |= TR2_IM_ACID;
			a_ptr->flags3 |= TR3_IGNORE_ACID;
			a_ptr->flags5 |= TR5_IGNORE_WATER;
			break;
		case 3:
			a_ptr->flags2 |= TR2_IM_ELEC;
			a_ptr->flags3 |= TR3_IGNORE_ELEC;
			break;
		case 4:
			a_ptr->flags2 |= TR2_IM_COLD;
			a_ptr->flags3 |= TR3_IGNORE_COLD;
			break;
		}
	}

	/* ----- fego2 flags ----- */

	if (fego2 & ETR2_R_SLAY) {
		switch (rand_int(8)) {
		case 0:
			a_ptr->flags1 |= TR1_SLAY_ANIMAL;
			break;
		case 1:
			a_ptr->flags1 |= TR1_SLAY_EVIL;
			break;
		case 2:
			a_ptr->flags1 |= TR1_SLAY_UNDEAD;
			break;
		case 3:
			a_ptr->flags1 |= TR1_SLAY_DEMON;
			break;
		case 4:
			a_ptr->flags1 |= TR1_SLAY_ORC;
			break;
		case 5:
			a_ptr->flags1 |= TR1_SLAY_TROLL;
			break;
		case 6:
			a_ptr->flags1 |= TR1_SLAY_GIANT;
			break;
		case 7:
			a_ptr->flags1 |= TR1_SLAY_DRAGON;
			break;
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
void random_resistance (artifact_type * a_ptr, bool is_scroll, int specific) {
	if (!specific) { /* To avoid a number of possible bugs */
		if (artifact_bias == BIAS_ACID) {
			if (!(a_ptr->flags2 & TR2_RES_ACID)) {
				a_ptr->flags2 |= TR2_RES_ACID;
				if (randint(2) == 1) return;
			}
			if (randint(BIAS_LUCK) == 1 && !(a_ptr->flags2 & TR2_IM_ACID)) {
				a_ptr->flags2 |= TR2_IM_ACID;
				if (randint(2) == 1) return;
			}
		} else if (artifact_bias == BIAS_ELEC) {
			if (!(a_ptr->flags2 & TR2_RES_ELEC)) {
				a_ptr->flags2 |= TR2_RES_ELEC;
				if (randint(2) == 1) return;
			}
			if (a_ptr->tval >= TV_CLOAK && a_ptr->tval <= TV_HARD_ARMOR &&
			    !(a_ptr->flags3 & TR3_SH_ELEC)) {
				a_ptr->flags3 |= TR3_SH_ELEC;
				if (randint(2) == 1) return;
			}
			if (randint(BIAS_LUCK) == 1 && !(a_ptr->flags2 & TR2_IM_ELEC)) {
				a_ptr->flags2 |= TR2_IM_ELEC;
				if (randint(2) == 1) return;
			}
		} else if (artifact_bias == BIAS_FIRE) {
			if (!(a_ptr->flags2 & TR2_RES_FIRE)) {
				a_ptr->flags2 |= TR2_RES_FIRE;
				if (randint(2) == 1) return;
			}
			if (a_ptr->tval >= TV_CLOAK && a_ptr->tval <= TV_HARD_ARMOR &&
			    !(a_ptr->flags3 & TR3_SH_FIRE)) {
				a_ptr->flags3 |= TR3_SH_FIRE;
				if (randint(2) == 1) return;
			}
			if (randint(BIAS_LUCK) == 1 && !(a_ptr->flags2 & TR2_IM_FIRE)) {
				a_ptr->flags2 |= TR2_IM_FIRE;
				if (randint(2) == 1) return;
			}
		} else if (artifact_bias == BIAS_COLD) {
			if (!(a_ptr->flags2 & TR2_RES_COLD)) {
				a_ptr->flags2 |= TR2_RES_COLD;
				if (randint(2) == 1) return;
			}
			if (a_ptr->tval >= TV_CLOAK && a_ptr->tval <= TV_HARD_ARMOR &&
			    !(a_ptr->flags5 & TR5_SH_COLD)) {
				a_ptr->flags5 |= TR5_SH_COLD;
				if (randint(2) == 1) return;
			}
			if (randint(BIAS_LUCK) == 1 && !(a_ptr->flags2 & TR2_IM_COLD)) {
				a_ptr->flags2 |= TR2_IM_COLD;
				if (randint(2) == 1) return;
			}
		} else if (artifact_bias == BIAS_POIS) {
			if (!(a_ptr->flags2 & TR2_RES_POIS)) {
				a_ptr->flags2 |= TR2_RES_POIS;
				if (randint(2) == 1) return;
			}
		} else if (artifact_bias == BIAS_WARRIOR) {
			if (randint(3) != 1 && (!(a_ptr->flags2 & TR2_RES_FEAR))) {
				a_ptr->flags2 |= TR2_RES_FEAR;
				if (randint(2) == 1) return;
			}
			if ((randint(3) == 1) && (!(a_ptr->flags3 & TR3_NO_MAGIC))) {
				a_ptr->flags3 |= TR3_NO_MAGIC;
				if (randint(2) == 1) return;
			}
		} else if (artifact_bias == BIAS_NECROMANTIC) {
			if (!(a_ptr->flags2 & TR2_RES_NETHER)) {
				a_ptr->flags2 |= TR2_RES_NETHER;
				if (randint(2) == 1) return;
			}
			if (!(a_ptr->flags2 & TR2_RES_POIS)) {
				a_ptr->flags2 |= TR2_RES_POIS;
				if (randint(2) == 1) return;
			}
			if (!(a_ptr->flags2 & TR2_RES_DARK)) {
				a_ptr->flags2 |= TR2_RES_DARK;
				if (randint(2) == 1) return;
			}
		} else if (artifact_bias == BIAS_CHAOS) {
			if (!(a_ptr->flags2 & TR2_RES_CHAOS)) {
				a_ptr->flags2 |= TR2_RES_CHAOS;
				if (randint(2) == 1) return;
			}
			if (!(a_ptr->flags2 & TR2_RES_CONF)) {
				a_ptr->flags2 |= TR2_RES_CONF;
				if (randint(2) == 1) return;
			}
			if (!(a_ptr->flags2 & TR2_RES_DISEN)) {
				a_ptr->flags2 |= TR2_RES_DISEN;
				if (randint(2) == 1) return;
			}
		}
	}

	switch (specific ? specific : randint(41)) {
	case 1:
		if (randint(WEIRD_LUCK) != 1)
			random_resistance(a_ptr, is_scroll, specific);
		else {
			a_ptr->flags2 |= TR2_IM_ACID;
			/*  if (is_scroll) msg_print("It looks totally incorruptible."); */
			if (!(artifact_bias)) artifact_bias = BIAS_ACID;
		}
		break;
	case 2:
		if (randint(WEIRD_LUCK) != 1)
			random_resistance(a_ptr, is_scroll, specific);
		else {
			a_ptr->flags2 |= TR2_IM_ELEC;
			/*  if (is_scroll) msg_print("It looks completely grounded."); */
			if (!(artifact_bias)) artifact_bias = BIAS_ELEC;
		}
		break;
	case 3:
		if (randint(WEIRD_LUCK) != 1)
			random_resistance(a_ptr, is_scroll, specific);
		else {
			a_ptr->flags2 |= TR2_IM_COLD;
			/*  if (is_scroll) msg_print("It feels very warm."); */
			if (!(artifact_bias)) artifact_bias = BIAS_COLD;
		}
		break;
	case 4:
		if (randint(WEIRD_LUCK) != 1)
			random_resistance(a_ptr, is_scroll, specific);
		else {
			a_ptr->flags2 |= TR2_IM_FIRE;
			/*  if (is_scroll) msg_print("It feels very cool."); */
			if (!(artifact_bias)) artifact_bias = BIAS_FIRE;
		}
		break;
	case 5: case 6: case 13:
		a_ptr->flags2 |= TR2_RES_ACID;
		/*  if (is_scroll) msg_print("It makes your stomach rumble."); */
		if (!(artifact_bias)) artifact_bias = BIAS_ACID;
		break;
	case 7: case 8: case 14:
		a_ptr->flags2 |= TR2_RES_ELEC;
		/*  if (is_scroll) msg_print("It makes you feel grounded."); */
		if (!(artifact_bias)) artifact_bias = BIAS_ELEC;
		break;
	case 9: case 10: case 15:
		a_ptr->flags2 |= TR2_RES_FIRE;
		/*  if (is_scroll) msg_print("It makes you feel cool!");*/
		if (!(artifact_bias)) artifact_bias = BIAS_FIRE;
		break;
	case 11: case 12: case 16:
		a_ptr->flags2 |= TR2_RES_COLD;
		/*  if (is_scroll) msg_print("It makes you feel full of hot air!");*/
		if (!(artifact_bias)) artifact_bias = BIAS_COLD;
		break;
	case 17: case 18:
		a_ptr->flags2 |= TR2_RES_POIS;
		/*  if (is_scroll) msg_print("It makes breathing easier for you."); */
		if (!(artifact_bias) && randint(4) != 1) artifact_bias = BIAS_POIS;
		else if (!(artifact_bias) && randint(2) == 1) artifact_bias = BIAS_NECROMANTIC;
		else if (!(artifact_bias) && randint(2) == 1) artifact_bias = BIAS_ROGUE;
		break;
	case 19: case 20:
		a_ptr->flags2 |= TR2_RES_FEAR;
		/*  if (is_scroll) msg_print("It makes you feel brave!"); */
		if (!(artifact_bias) && randint(3) == 1) artifact_bias = BIAS_WARRIOR;
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
		if (!(artifact_bias) && randint(6) == 1) artifact_bias = BIAS_CHAOS;
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
		if (!(artifact_bias) && randint(3) == 1) artifact_bias = BIAS_NECROMANTIC;
		break;
	case 33: case 34:
		a_ptr->flags2 |= TR2_RES_NEXUS;
		/*  if (is_scroll) msg_print("It makes you feel normal.");*/
		break;
	case 35: case 36:
		a_ptr->flags2 |= TR2_RES_CHAOS;
		/*  if (is_scroll) msg_print("It makes you feel very firm.");*/
		if (!(artifact_bias) && randint(2) == 1) artifact_bias = BIAS_CHAOS;
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
		if (!(artifact_bias)) artifact_bias = BIAS_ELEC;
		break;
	case 40:
		if (a_ptr->tval >= TV_CLOAK && a_ptr->tval <= TV_HARD_ARMOR)
			a_ptr->flags3 |= TR3_SH_FIRE;
		else
			random_resistance(a_ptr, is_scroll, specific);
		if (!(artifact_bias)) artifact_bias = BIAS_FIRE;
		break;
	case 41:
		if (a_ptr->tval >= TV_CLOAK && a_ptr->tval <= TV_HARD_ARMOR)
			a_ptr->flags5 |= TR5_SH_COLD;
		else
			random_resistance(a_ptr, is_scroll, specific);
		if (!(artifact_bias)) artifact_bias = BIAS_COLD;
		break;
	case 42: /* currently not possible since switch goes only up to 41. also buggy/wrong to give reflect to these items. */
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

void dragon_resist(artifact_type * a_ptr) {
	do {
		artifact_bias = 0;

		if (randint(4) == 1)
			random_resistance(a_ptr, FALSE, ((randint(14)) + 4));
		else
			random_resistance(a_ptr, FALSE, ((randint(22)) + 16));
	}
	while (randint(2) == 1);
}
