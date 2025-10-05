/* $Id$ */
/* Purpose: Angband game engine */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"
#include "externs.h"

/* chance of townie respawning like other monsters, rand_int(chance)==0 means respawn */
 /* Default */
#define TOWNIE_RESPAWN_CHANCE	350
/* better for Halloween event */
#define HALLOWEEN_TOWNIE_RESPAWN_CHANCE	175

/* if defined, player ghost loses exp slowly. [10000]
 * see GHOST_XP_CASHBACK in xtra2.c also.
 */
#define GHOST_FADING	(cfg.fps * 150)

/* How fast HP/MP regenerate when 'resting'. [3] */
#define RESTING_RATE	(cfg.resting_rate)

/* Inverse of (1 - assumed density of wood relative to water), we're assuming 0.5, so it's "1 / (1 - 0.5)" = 2. */
#define WOOD_INV_DENSITY 2

/* Maximum wilderness radius a player can travel with WoR [16]
 * TODO: Add another method to allow wilderness travels */
#define RECALL_MAX_RANGE	24

/* duration of GoI when getting recalled.        [2] (Must be 0<=n<=4) */
#define RECALL_GOI_LENGTH	3

#ifdef EXTENDED_COLOURS_PALANIM
/* How often is the palette animated per in-game hour? [12 aka every 5 minutes]
   **NOTE**: Make sure (HOUR / PALANIM_HOUR_DIV) aka (((10 * 384 * cfg.fps) / 24) / PALANIM_HOUR_DIV) is integer divisable! */
 #define PALANIM_HOUR_DIV	12
#endif

/* Failing auto-retaliation with item or cmd (magic devices, spells, mimic powers) due to being out of mana/charges/energy
   and without melee-fallback will not cost the usual 1/3 energy it costs when attempting manually. */
#define AUTORET_FAIL_FREE



/* forward declarations */
static void process_firework_creation(void);
#ifndef CLIENT_SIDE_WEATHER
static void process_weather_control(void);
static void process_weather_effect_creation(void);
#else
 #ifndef CLIENT_WEATHER_GLOBAL

static void wild_weather_init(void);
static void process_wild_weather(void);
static void cloud_set_movement(int i);
static void cloud_move(int i, bool newly_created);
static void cloud_erase(int i);
 /* cdsum*87/200 : sin 60 deg */
#define CLOUD_XS(cx1, cy1, cx2, cy2, cdsum) ((cx1 - (cdsum - (cx2 - cx1)) / 2) / MAX_WID)
#define CLOUD_YS(cx1, cy1, cx2, cy2, cdsum) ((cy1 - ((cdsum * 87) / 200)) / MAX_HGT)
#define CLOUD_XD(cx1, cy1, cx2, cy2, cdsum) ((cx2 + (cdsum - (cx2 - cx1)) / 2) / MAX_WID)
#define CLOUD_YD(cx1, cy1, cx2, cy2, cdsum) ((cy2 + ((cdsum * 87) / 200)) / MAX_HGT)

/* Enable wind in general? */
#define WEATHER_WINDS

/* Activate hack that disables all cloud movement,
   as a workaround for the 'eternal rain' bug for now - C. Blue */
#define WEATHER_NO_CLOUD_MOVEMENT

 #else

static void process_weather_control(void);
 #endif
#endif

/* To make up for this, players get an IDDC_freepass ;) */
#define SHUTDOWN_IGNORE_IDDC(p_ptr) \
    ((in_irondeepdive(&(p_ptr)->wpos) || in_hallsofmandos(&(p_ptr)->wpos)) \
    && (p_ptr)->idle_char > 900) /* just after 15 minutes flat */
    //&& p_ptr->afk
    //&& p_ptr->idle_char > STARVE_KICK_TIMER) //reuse idle-starve-kick-timer for this

static int fast_sqrt_10div6[120] = { /* 0..120, with 1 decimal place resolution, with the specified index being assumed to get pseudo-int-divided by 6 before going through sqrt() calc. */
     0, 4, 6, 7, 8, 9,10,11,12,12,	13,14,14,15,15,16,16,17,17,18,
    18,19,19,20,20,20,21,21,22,22,	22,23,23,23,24,24,24,25,25,25,
    26,26,26,27,27,27,28,28,28,29,	29,29,29,30,30,30,31,31,31,31,
    32,32,32,32,33,33,33,33,34,34,	34,34,35,35,35,35,36,36,36,36,
    37,37,37,37,37,38,38,38,38,39,	39,39,39,39,40,40,40,40,40,41,
    41,41,41,41,42,42,42,42,42,43,	43,43,43,43,44,44,44,44,44,45,
};
static int fast_sqrt_10[371] = { /* 0..370, with 1 decimal place resolution. Since MMP can be much higher than 371, the value needs to be divided by 6 before feeding as index. */
     0,10,14,17,20,22,24,26,28,30,		32,33,35,36,37,39,40,41,42,44, /* <- redundant line, as up to here it's covered by fast_sqrt_10div6 already. Included just for completion's sake. */
    45,46,47,48,49,50,51,52,53,54,		55,56,57,57,58,59,60,61,62,62,			63,64,65,66,66,67,68,69,69,70,			71,71,72,73,73,74,75,75,76,77,
    77,78,79,79,80,81,81,82,82,83,		84,84,85,85,86,87,87,88,88,89,			89,90,91,91,92,92,93,93,94,94,			95,95,96,96,97,97,98,98,99,99,
    100,100,101,101,102,102,103,103,104,104,	105,105,106,106,107,107,108,108,109,109,	110,110,110,111,111,112,112,113,113,114,	114,114,115,115,116,116,117,117,117,118,
    118,119,119,120,120,120,121,121,122,122,	122,123,123,124,124,124,125,125,126,126,	126,127,127,128,128,128,129,129,130,130,	130,131,131,132,132,132,133,133,133,134,
    134,135,135,135,136,136,136,137,137,137,	138,138,139,139,139,140,140,140,141,141,	141,142,142,142,143,143,144,144,144,145,	145,145,146,146,146,147,147,147,148,148,
    148,149,149,149,150,150,150,151,151,151,	152,152,152,153,153,153,154,154,154,155,	155,155,156,156,156,157,157,157,157,158,	158,158,159,159,159,160,160,160,161,161,
    161,162,162,162,162,163,163,163,164,164,	164,165,165,165,166,166,166,166,167,167,	167,168,168,168,169,169,169,169,170,170,	170,171,171,171,171,172,172,172,173,173,
    173,173,174,174,174,175,175,175,175,176,	176,176,177,177,177,177,178,178,178,179,	179,179,179,180,180,180,181,181,181,181,	182,182,182,182,183,183,183,184,184,184,
    184,185,185,185,185,186,186,186,187,187,	187,187,188,188,188,188,189,189,189,189,	190,190,190,191,191,191,191,192,192,192,	192
};


/*
 * Return a "feeling" (or NULL) about an item.  Method 1 (Heavy).
 */

/* Feel 'combat' strongly (heavy) - always gives a result! */
cptr value_check_aux1(object_type *o_ptr) {
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	u32b v;

	/* Artifacts */
	if (artifact_p(o_ptr)) {
		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return("terrible");

		/* Normal */
		return("special");
	}

	/* Ego-Items */
	if (ego_item_p(o_ptr)) {
		/* Hack for Stormbringer, so it doesn't show as "worthless" */
		if (o_ptr->name2 == EGO_STORMBRINGER) return("terrible");

		/* Cursed items */
		if (cursed_p(o_ptr)) return("cursed");

		/* Broken items */
		if (broken_p(o_ptr)) return("worthless");

		v = object_value(0, o_ptr);
		if (!v) return("worthless");

		/* Exploding ammo counts as pseudo-ego and is excellent */
		if (is_ammo(o_ptr->tval) && o_ptr->pval) return("excellent");

		if (v < 4000) return("good");

		return("excellent");
	}

	/* Cursed items */
	if (cursed_p(o_ptr)) return("cursed");

	/* Broken items */
	if (broken_p(o_ptr)) return("worthless");

	/* All exploding or ego-ammo is excellent.. */
	if (is_ammo(o_ptr->tval) && (o_ptr->pval || o_ptr->name2 || o_ptr->name2b)) {
		/* ..except for zero-money ego: Backbiting ammo! */
		if (o_ptr->name2 && !e_info[o_ptr->name2].cost) return("worthless");
		if (o_ptr->name2b && !e_info[o_ptr->name2b].cost) return("worthless");
		return("excellent");
	}

	/* Valid "tval" codes */
	switch (o_ptr->tval) {
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_AXE:

	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:

	case TV_DIGGING:

	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
	case TV_BOOMERANG:

	case TV_TRAPKIT:
		/* Good "armor" bonus */
		if ((o_ptr->to_a > k_ptr->to_a) &&
		    (o_ptr->to_a > 0)) return("good");
		/* Good "weapon" bonus */
		if ((o_ptr->to_h - k_ptr->to_h + o_ptr->to_d - k_ptr->to_d > 0) &&
		    (o_ptr->to_h > 0 || o_ptr->to_d > 0)) return("good");
		break;
	default:
		/* Good "armor" bonus */
		if (o_ptr->to_a > 0) return("good");
		/* Good "weapon" bonus */
		if (o_ptr->to_h + o_ptr->to_d > 0) return("good");
		break;
	}

	/* Default to "average" */
	return("average");
}

/* Feel 'magic' strongly (heavy) - always gives a result! */
cptr value_check_aux1_magic(object_type *o_ptr) {
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	u32b v;

	switch (o_ptr->tval) {
	/* Scrolls, Potions, Wands, Staves and Rods */
	case TV_SCROLL:
		/* hack for cheques */
		if (k_ptr->sval == SV_SCROLL_CHEQUE) {
			v = ps_get_cheque_value(o_ptr);
			if (v < 10000) return("good");
			return("excellent");
		}
		/* Fall through */
	case TV_POTION:
	case TV_POTION2:

	case TV_WAND:
	case TV_STAFF:
	case TV_ROD:
	case TV_ROD_MAIN:
		/* "Cursed" scrolls/potions have a cost of 0 */
		if (k_ptr->cost == 0) return("bad");

		/* Artifacts */
		if (artifact_p(o_ptr)) return("special");

		/* Scroll of Nothing, Apple Juice, etc. */
		if (k_ptr->cost < 3) return("worthless"); //"average" or "worthless"

		/*
		 * Identify, Phase Door, Cure Light Wounds, etc. are
		 * just average
		 */
		if (k_ptr->cost < 100) return("average");

		/* Enchant Armor, *Identify*, Restore Stat, etc. */
		if (k_ptr->cost < 4000) return("good");

		/* Acquirement, Artifact Creation... */
		return("excellent");

	case TV_RING:
	case TV_AMULET:
		/* Artifacts */
		if (artifact_p(o_ptr)) {
			/* Cursed/Broken */
			if (cursed_p(o_ptr) || broken_p(o_ptr)) return("terrible");

			/* Normal */
			return("special");
		}

		/* Cursed items */
		if (cursed_p(o_ptr)) return("cursed");

		/* Broken items -- doesn't exist for jewelry though, as this is an ego power, not in k_info */
		if (broken_p(o_ptr)) return("worthless");

		v = object_value(0, o_ptr);
		if (!v) return("bad");
		else if (v <= 20) return("worthless"); //adornment
		else if (v < 1000) return("average");
		else if (v < 10000) return("good"); //jewelry tends to be expensivo in general
		return("excellent");

	/* Food */
	case TV_FOOD:
		/* "Cursed" food */
		if (k_ptr->cost == 0) return("bad");

		/* Artifacts */
		if (artifact_p(o_ptr)) return("special");

		/* Food with magical properties -
		   note about mushrooms: Restoring is 2000, Restore Str/Con + Hallu are 200. */
		if (k_ptr->cost >= 200) return("excellent"); /* Great word for 200 Au items, but w/e >_> */
		if (k_ptr->cost > 10) return("good");

		break;
	}

	/* Default */
	return("average");
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 2 (Light).
 */

/* Feel 'combat' lightly - 'average' in heavy becomes just <no result> here! */
cptr value_check_aux2(object_type *o_ptr) {
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Cursed items (all of them) */
	if (cursed_p(o_ptr)) return("cursed");

	/* Broken items (all of them) */
	if (broken_p(o_ptr)) return("worthless");

	/* Artifacts -- except cursed/broken ones */
	if (artifact_p(o_ptr)) return("good");

	/* Ego-Items -- except cursed/broken ones */
	if (!k_ptr->cost) return("worthless");
	if (ego_item_p(o_ptr)) {
		if (!object_value(0, o_ptr)) return("worthless");
		return("good");
	}
	/* Exploding ammo counts as pseudo-ego */
	if (is_ammo(o_ptr->tval) && o_ptr->pval) return("good");

	switch (o_ptr->tval) {
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_AXE:

	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:

	case TV_DIGGING:

	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
	case TV_BOOMERANG:

	case TV_TRAPKIT:
		/* Good "armor" bonus */
		if (o_ptr->to_a > k_ptr->to_a
		    && o_ptr->to_a > 0 /* for rusty armour....*/
		    ) return("good");
		/* Good "weapon" bonus */
		if (o_ptr->to_h - k_ptr->to_h + o_ptr->to_d - k_ptr->to_d > 0
		    && (o_ptr->to_h > 0 || o_ptr->to_d > 0) /* for broken daggers....*/
		    ) return("good");
		break;
	default:
		/* Good "armor" bonus */
		if (o_ptr->to_a > 0) return("good");
		/* Good "weapon" bonus */
		if (o_ptr->to_h + o_ptr->to_d > 0) return("good");
		break;
	}

	/* No feeling */
	return(NULL);
}

/* Feel 'magic' lightly - 'average' in heavy becomes just <no result> here! */
cptr value_check_aux2_magic(object_type *o_ptr) {
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	u32b v;

	switch (o_ptr->tval) {
	/* Scrolls, Potions, Wands, Staves and Rods */
	case TV_SCROLL:
		/* hack for cheques */
		if (k_ptr->sval == SV_SCROLL_CHEQUE) return("good");
		/* Fall through */
	case TV_POTION:
	case TV_POTION2:

	case TV_WAND:
	case TV_STAFF:
	case TV_ROD:
		/* Bad scrolls/potions have a cost of 0 */
		if (k_ptr->cost == 0) return("worthless");

		/* Artifacts (doesn't exist for these item types) */
		if (artifact_p(o_ptr)) return("good");

		if (k_ptr->cost >= 1000) return("good");

		break;

	case TV_RING:
	case TV_AMULET:
		/* Cursed items */
		if (cursed_p(o_ptr)) return("cursed");

		/* Broken items -- doesn't exist for jewelry though, as this is an ego power, not in k_info */
		if (broken_p(o_ptr)) return("worthless");

		/* Artifacts */
		if (artifact_p(o_ptr)) return("good");

		v = object_value(0, o_ptr);
		if (!v) return("worthless");
		if (ego_item_p(o_ptr)) return("good");

		break;

	/* Food */
	case TV_FOOD:
		/* "Cursed" food */
		if (k_ptr->cost == 0) return("worthless");

		/* Artifacts (doesn't exist) */
		if (artifact_p(o_ptr)) return("good");

		/* Food with magical properties -
		   note about mushrooms: Restoring is 2000, Restore Str/Con + Hallu are 200. */
		if (k_ptr->cost > 10) return("good");

		break;
	}

	/* No feeling */
	//return("");
	return(NULL);
}

/*
 * Sense the inventory aka Pseudo-ID.
 * We're called every 5/6 of a player's dungeon turn from process_player_end_aux().
 * NOTE: Currently does not affect items inside bags. (ENABLE_SUBINVEN)
 */
#define INVEN_FAIL 80 /* Occasional failure on inventory items */
static void sense_inventory(int Ind) {
	player_type *p_ptr = Players[Ind];

	int i, dur, in = -1;
#ifdef ENABLE_SUBINVEN
	int si = -2, ii = -1;
#endif

	bool heavy_combat = FALSE, heavy_magic = FALSE, heavy_archery = FALSE, heavy_traps = FALSE, heavy_food = FALSE;

	bool ok_combat_base = FALSE, ok_magic_base = FALSE, ok_archery_base = FALSE, ok_traps_base = FALSE, ok_food_base = FALSE;
	bool ok_curse_base = FALSE, force_curse_base = FALSE, force_shrooms_base = (p_ptr->prace == RACE_HOBBIT || p_ptr->prace == RACE_KOBOLD);

	bool ok_combat, ok_magic, ok_archery, ok_traps, ok_food, ok_curse, force_curse, force_shrooms;

	cptr feel;
	bool felt_heavy, fail_light;

	object_type *o_ptr;
	char o_name[ONAME_LEN + 10];


	/*** Check for "sensing" ***/

	/* No sensing when confused */
	if (p_ptr->confused) return;

#ifdef IDDC_ID_BOOST
	if (in_irondeepdive(&p_ptr->wpos)) dur = 1500; /* faster pseudo-id in IDDC */
	else
#endif
	dur = 3000; /* default */

	/* instead, allow more use at lower SKILL_COMBAT levels already,
	otherwise huge stacks of ID scrolls will remain mandatory for maybe too long a time - C. Blue */
	if (!rand_int(dur / (get_skill_scale(p_ptr, SKILL_COMBAT, 80) + 20) - 28)) ok_combat_base = TRUE;
	if (!rand_int(dur / (get_skill_scale(p_ptr, SKILL_ARCHERY, 80) + 20) - 28)) ok_archery_base = TRUE;
	if (!rand_int(dur / (get_skill_scale(p_ptr, SKILL_MAGIC, 80) + 20) - 28)) ok_magic_base = ok_curse_base = TRUE;
	if (!rand_int(dur / (get_skill_scale(p_ptr, SKILL_TRAPPING, 80) + 20) - 28)) ok_traps_base = TRUE;
	if (!rand_int(dur / (get_skill_scale(p_ptr, SKILL_HEALTH, 80) + 20) - 28)) ok_food_base = TRUE;
	/* note: SKILL_PRAY is currently unused */
	if (!rand_int(dur / (get_skill_scale(p_ptr, SKILL_PRAY, 80) + 20) - 28)) ok_curse_base = TRUE;

	/* A powerful warrior can pseudo-id ranged weapons and ammo too,
	   even if (s)he's not good at archery in general */
	if (get_skill(p_ptr, SKILL_COMBAT) >= 30 && ok_combat_base) ok_archery_base = TRUE; /* (apply 33% chance, see below) - allow basic feelings */

	/* A very powerful warrior can even distinguish magic items */
	if (get_skill(p_ptr, SKILL_COMBAT) >= 40 && ok_combat_base) ok_curse_base = TRUE;

	/* extra class-specific boni */
	if (p_ptr->ptrait == TRAIT_ENLIGHTENED) force_curse_base = ok_curse_base = TRUE;
	else if (p_ptr->pclass == CLASS_PRIEST
 #ifdef ENABLE_CPRIEST
	    || p_ptr->pclass == CLASS_CPRIEST
 #endif
	    ) {
		//i = (p_ptr->lev < 35) ? (135 - (p_ptr->lev + 10) * 3) : 1;
		if (p_ptr->lev >= 35 || !rand_int(dur / (p_ptr->lev * 2 + 30) - 29)) force_curse_base = ok_curse_base = TRUE;
	}

	/* nothing to feel? exit */
	if (!ok_combat_base && !ok_magic_base && !ok_archery_base && !ok_traps_base && !ok_curse_base && !ok_food_base && !force_shrooms_base) return;

	heavy_combat = (get_skill(p_ptr, SKILL_COMBAT) >= 10) ? TRUE : FALSE;
	heavy_magic = (get_skill(p_ptr, SKILL_MAGIC) >= 10) ? TRUE : FALSE;
	heavy_archery = (get_skill(p_ptr, SKILL_ARCHERY) >= 10) ? TRUE : FALSE;
	heavy_traps = (get_skill(p_ptr, SKILL_TRAPPING) >= 10) ? TRUE : FALSE;
	heavy_food = (get_skill(p_ptr, SKILL_HEALTH) >= 10) ? TRUE : FALSE;

	/* Check everything */
#ifdef ENABLE_SUBINVEN
	while (ii < INVEN_TOTAL) {
		if (si != -2) {
			si++;
			if (si == p_ptr->inventory[ii].bpval) {
				si = -2;
				continue;
			}
			o_ptr = &p_ptr->subinventory[ii][si];
			i = (ii + 1) * SUBINVEN_INVEN_MUL + si;
		} else {
			ii++;
			o_ptr = &p_ptr->inventory[ii];
			i = ii;
		}
#else
	for (i = 0; i < INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];
#endif
		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;
#ifdef ENABLE_SUBINVEN
		/* Enter subinventory? */
		if (o_ptr->tval == TV_SUBINVEN) {
			si = -1;
			continue;
		}
#endif

		/* It is fully known, no information needed */
		if (object_known_p(Ind, o_ptr)) continue;
		/* We have pseudo-id'ed it heavily before, do not tell us again */
		if (o_ptr->ident & ID_SENSE_HEAVY) continue;

		/* If already pseudo-id'ed normally, fail for non-heavy pseudo-id, but allow heavy one as an upgrade if we meanwhile trained the responsible skill */
		fail_light = (o_ptr->ident & ID_SENSE); /* Note for force_curse handling: This also means we already know if the item is cursed! */

		ok_combat = ok_combat_base;
		ok_magic = ok_magic_base;
		ok_archery = ok_archery_base;
		ok_traps = ok_traps_base;
		ok_food = ok_food_base;
		ok_curse = ok_curse_base;
		force_curse = force_curse_base;
		force_shrooms = force_shrooms_base;

		/* Occasional failure on inventory items */
		if ((i < INVEN_WIELD
#ifdef ENABLE_SUBINVEN
		    || i >= SUBINVEN_INVEN_MUL
#endif
		    ) && (magik(INVEN_FAIL) || UNAWARENESS(p_ptr))) {
			if ((!force_curse && !force_shrooms) || fail_light) continue; /* Either we don't need to process force_curse, or we already know the item is cursed */
			/* if we're forced to insta-sense a curse/shroom, do just that, as we'd have failed for all other actions */
			ok_magic = ok_combat = ok_archery = ok_traps = ok_food = FALSE;
			/* We're just here for the shrooms, ie if force_curse is FALSE, prevent curse-detection. */
			ok_curse = force_curse;
		}

		feel = NULL;
		felt_heavy = FALSE;

		/* Valid "tval" codes */
		switch (o_ptr->tval) {
		case TV_DIGGING:
		case TV_BLUNT:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_AXE:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		case TV_BOOMERANG:
			if (fail_light && !heavy_combat) continue; //finally fail
			if (ok_combat) {
				feel = (heavy_combat ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));
				if (heavy_combat) felt_heavy = TRUE;
			}
			break;

		case TV_MSTAFF:
			if (fail_light && !heavy_magic) continue; //finally fail
			if (ok_magic) {
				feel = (heavy_magic ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));
				if (heavy_magic) felt_heavy = TRUE;
			}
			break;

		case TV_SCROLL:
			/* hack for cheques: Don't try to pseudo-id them at all. */
			if (o_ptr->sval == SV_SCROLL_CHEQUE) continue;
			/* Fall through */
		case TV_POTION:
		case TV_POTION2:
		case TV_WAND:
		case TV_STAFF:
		case TV_ROD:
		case TV_RING: /* finally these two, in 2023 */
		case TV_AMULET:
			if (fail_light && !heavy_magic) continue; //finally fail
			if (ok_magic && !object_aware_p(Ind, o_ptr)) {
				feel = (heavy_magic ? value_check_aux1_magic(o_ptr) : value_check_aux2_magic(o_ptr));
				if (heavy_magic) felt_heavy = TRUE;
			}
			break;

		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
			/* If we can detect via archery or via trapping, choose archery (doesn't matter, but we have to pick one) */
			if (ok_traps && !(ok_archery && (heavy_archery || !heavy_traps))) {
				if (fail_light && !heavy_traps) continue; //finally fail
				feel = (heavy_traps ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));
				if (heavy_traps) felt_heavy = TRUE;
				break;
			}
			/* Fall through to Archery check */
		case TV_BOW:
			if (fail_light && !heavy_archery) continue; //finally fail
			if (ok_archery || (ok_combat && magik(25))) {
				feel = (heavy_archery ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));
				if (heavy_archery) felt_heavy = TRUE;
			}
			break;

		case TV_TRAPKIT:
			if (fail_light && !heavy_traps) continue; //finally fail
			if (ok_traps) {
				feel = (heavy_traps ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));
				if (heavy_traps) felt_heavy = TRUE;
			}
			break;

		case TV_FOOD: /* dual! Uses auxX_magic, which contains the food-specific values. Since potions are 'magic' too, food seems not misplaced.. */
			/* First check for 'force_shrooms'. Note that we use 'heavy' detection (aux1 instead of aux2) for now. */
			if (force_shrooms && o_ptr->sval <= SV_FOOD_MUSHROOMS_MAX && !object_aware_p(Ind, o_ptr)) {
				feel = value_check_aux1_magic(o_ptr);
				felt_heavy = TRUE;
				break;
			}

			/* Note: ego food powers have no influence, as these don't change the value but are just 'brand names' */
			if (fail_light && !heavy_food) continue; //finally fail
			if (ok_food && !object_aware_p(Ind, o_ptr)) {
				feel = (heavy_food ? value_check_aux1_magic(o_ptr) : value_check_aux2_magic(o_ptr));
				if (heavy_food) felt_heavy = TRUE;
			}
			break;
		}

		/* Maybe change: ok_curse isn't 'heavy' priority ie it can fail on inventory (unless forced) as can all other non-heavy pseudo-id,
		   but if it triggers then it always gives 'heavy' result values (from _aux1()), revealing that a cursed item is an artifact, via 'terrible' feeling.
		   This cheeze perhaps isn't terrible though, as we'd have found out if we tried to 'k' the item in question anyway >_>. */
		if (!feel && ok_curse && cursed_p(o_ptr)) { /* Avoid double-pseudo-ID, giving priority to any feelings gained above */
			feel = value_check_aux1(o_ptr);
			/* We have "felt" it */
			o_ptr->ident |= (ID_SENSE | ID_SENSED_ONCE | ID_SENSE_HEAVY);
		}

		/* Skip non-feelings */
		if (!feel) continue;

		/* Stop everything */
		if (p_ptr->disturb_minor) disturb(Ind, 0, 0);

		/* Get an object description */
		object_desc(Ind, o_name, o_ptr, FALSE, 0);

		/* Hack -- suppress messages */
		if (p_ptr->taciturn_messages) suppress_message = TRUE;

		/* Message (inventory) */
#ifdef ENABLE_SUBINVEN
		if (i >= SUBINVEN_INVEN_MUL) {
			msg_format(Ind, "You feel the %s (%c)(%c) in your pack %s %s...",
			    o_name, index_to_label(ii), index_to_label(si),
			    ((o_ptr->number == 1) ? "is" : "are"), feel);
		} else
#endif
		if (i < INVEN_WIELD) {
			msg_format(Ind, "You feel the %s (%c) in your pack %s %s...",
			    o_name, index_to_label(i),
			    ((o_ptr->number == 1) ? "is" : "are"), feel);
		}
		/* Message (equipment) */
		else {
			msg_format(Ind, "You feel the %s (%c) you are %s %s %s...",
			    o_name, index_to_label(i), describe_use(Ind, i),
			    ((o_ptr->number == 1) ? "is" : "are"), feel);
		}

		if (!p_ptr->warning_id && (streq(feel, "good") || streq(feel, "excellent") || streq(feel, "special"))) {
			msg_print(Ind, "\374\377yHINT: If you sense that an unknown item you found is 'good' or even better, make");
			msg_print(Ind, "\374\377y      sure to identify it, eg with a \377oScroll of Identify\377y from the alchemist shop!");
			s_printf("warning_id: %s\n", p_ptr->name);
			p_ptr->warning_id = 1;
		}

		suppress_message = FALSE;

		/* We have "felt" it */
		o_ptr->ident |= (ID_SENSE | ID_SENSED_ONCE);
		if (felt_heavy) o_ptr->ident |= ID_SENSE_HEAVY;

		/* Remember feeling of that flavour, if and only if an item is always the same!
		   For example, rings might be cursed by ego power. Wands may not.
		   Other than that, this way an interesting middle-way between RPG-style
		   remember-items-seen-in-shops and normal ID-remembrance can be created:
		   Remember static items seen in shops just by feeling before having IDed one. :)
		    - C. Blue */
		switch (o_ptr->tval) {
		case TV_SCROLL:
		case TV_POTION:
		case TV_POTION2:
		case TV_WAND:
		case TV_STAFF:
		case TV_ROD:
		case TV_ROD_MAIN:
		//case TV_RING: - no, as they might have 'cursed' ego power
		//case TV_AMULET: - no, as they might have 'cursed' ego power
		case TV_FOOD:
			p_ptr->obj_felt[o_ptr->k_idx] = TRUE;
			if (felt_heavy) p_ptr->obj_felt_heavy[o_ptr->k_idx] = TRUE;
		}

		/* new: remove previous pseudo-id tag completely, since we've bumped our p-id tier meanwhile */
		if (fail_light) {
			char note2[80], noteid[10];

			note_crop_pseudoid(note2, noteid, quark_str(o_ptr->note));
			if (!note2[0]) o_ptr->note = o_ptr->note_utag = 0;
			else o_ptr->note = quark_add(note2);
		}

		/* Inscribe it textually */
#if 0 /* make pseudo-id remove previous unique tag? */
		if (!o_ptr->note || o_ptr->note_utag) {
			o_ptr->note = quark_add(feel);
			o_ptr->note_utag = 0;
		}
#else /* keep unique tag until removed manually by player? */
		if (!o_ptr->note) o_ptr->note = quark_add(feel);
#endif
		/* Still add an inscription, so unique loot doesn't cause major annoyances - C. Blue */
		/* for items that were already pseudo-id-inscribed but then forgotten:
		   only add inscription if it doesn't exist on the item yet (*) - 2 checks */
		else if (strcmp(quark_str(o_ptr->note), feel)) { // (*) check 1 of 2 (exact match)
			strcpy(o_name, feel); /* just abusing o_name for this since it's not needed anymore anyway */
			strcat(o_name, "-");
			if (!strstr(quark_str(o_ptr->note), o_name)) { // (*) check 2 of 2 (partial match)
				strcat(o_name, quark_str(o_ptr->note));
				o_name[ONAME_LEN - 1] = 0;
				o_ptr->note = quark_add(o_name);
			}
		}

#ifdef ENABLE_SUBINVEN
		if (i >= SUBINVEN_INVEN_MUL) {
			/* Redraw subinven item */
			display_subinven_aux(Ind, ii, si);
		} else
#endif
		if (i >= INVEN_WIELD) p_ptr->window |= PW_EQUIP;
		else p_ptr->window |= PW_INVEN;
		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

		in = i;

		/* in case the pseudo-id inscription triggers a forced auto-inscription! :)
		   Added to allow inscribing 'good' ammo !k or so (ethereal ammo) */
		o_ptr->auto_insc = TRUE;
		handle_stuff(Ind);
		//Send_apply_auto_insc(Ind, i);
	}

	if (in != -1) Send_item_newest_2nd(Ind, in);
}

/*
 * Regenerate hit points				-RAK-
 */
static void regenhp(int Ind, int percent) {
	player_type *p_ptr = Players[Ind];

	s32b	new_chp, new_chp_frac;
	int	old_chp;
	int	freeze_test_heal = p_ptr->test_heal;

#ifndef NO_REGEN_ALT
	if (p_ptr->no_hp_regen) return;
#endif

	/* Save the old hitpoints */
	old_chp = p_ptr->chp;

	/* Extract the new hitpoints */
	new_chp = ((s32b)p_ptr->mhp) * percent + PY_REGEN_HPBASE;
	/* Apply the healing */
	hp_player(Ind, new_chp >> 16, TRUE, TRUE);

	/* check for overflow -- this is probably unneccecary */
	if ((p_ptr->chp < 0) && (old_chp > 0)) p_ptr->chp = MAX_SHORT;

	/* handle fractional hit point healing */
	new_chp_frac = (new_chp & 0xFFFF) + p_ptr->chp_frac;	/* mod 65536 */
	if (new_chp_frac >= 0x10000L) {
		hp_player(Ind, 1, TRUE, TRUE);
		p_ptr->chp_frac = new_chp_frac - 0x10000L;
	} else p_ptr->chp_frac = new_chp_frac;

	p_ptr->test_heal = freeze_test_heal;
	p_ptr->test_regen = p_ptr->chp - old_chp;
}

/*
 * Regenerate mana points				-RAK-
 */
static void regenmana(int Ind, int percent) {
	player_type *p_ptr = Players[Ind];
	s32b new_mana, new_mana_frac;
	int old_cmp;

#ifdef MARTYR_NO_MANA
	if (p_ptr->martyr) return;
#endif

#ifndef NO_REGEN_ALT
	if (p_ptr->no_mp_regen) return;
#endif

	old_cmp = p_ptr->cmp;
	new_mana = ((s32b)p_ptr->mmp) * percent + PY_REGEN_MNBASE;

	p_ptr->cmp += new_mana >> 16;	/* div 65536 */
	/* check for overflow */
	if ((p_ptr->cmp < 0) && (old_cmp > 0))
		p_ptr->cmp = MAX_SHORT;

	new_mana_frac = (new_mana & 0xFFFF) + p_ptr->cmp_frac;	/* mod 65536 */
	if (new_mana_frac >= 0x10000L) {
		p_ptr->cmp_frac = new_mana_frac - 0x10000L;
		p_ptr->cmp++;
	} else {
		p_ptr->cmp_frac = new_mana_frac;
	}

	/* Must set frac to zero even if equal */
	if (p_ptr->cmp >= p_ptr->mmp) {
		p_ptr->cmp = p_ptr->mmp;
		p_ptr->cmp_frac = 0;
	}

	/* Redraw mana */
	if (old_cmp != p_ptr->cmp) {
		/* Redraw */
		p_ptr->redraw |= (PR_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);
	}
}


#define pelpel
#ifdef pelpel

/* Wipeout the effects	- Jir - */
void erase_effects(int effect) {
	int i, j, l;
	effect_type *e_ptr = &effects[effect];
	worldpos *wpos = &e_ptr->wpos;
	cave_type **zcave;
	cave_type *c_ptr;
	int rad = e_ptr->rad;
	int cy = e_ptr->cy;
	int cx = e_ptr->cx;

	e_ptr->time = 0;

	e_ptr->interval = 0;
	e_ptr->type = 0;
	e_ptr->dam = 0;
	e_ptr->time = 0;
	e_ptr->flags = 0;
	e_ptr->whot = 0; // Kurzel
	e_ptr->cx = 0;
	e_ptr->cy = 0;
	e_ptr->rad = 0;
	e_ptr->cflags = 0;

	if (!(zcave = getcave(wpos))) return;

	/* XXX +1 is needed.. */
	for (l = 0; l < tdi[rad + 0]; l++) {
		j = cy + tdy[l];
		i = cx + tdx[l];
		if (!in_bounds(j, i)) continue;

		c_ptr = &zcave[j][i];
		if (c_ptr->effect == effect) {
			c_ptr->effect = 0;
			everyone_lite_spot(wpos, j, i);
		}
		if (c_ptr->effect_past == effect) c_ptr->effect_past = 0;
	}
}

/* Helper function for usage of EFF_DAMAGE_AFTER_SETTING:
   Imprint effect on a grid, redraw it for everybody, apply damage projection. */
/* Apply project() damage at the end of processing an effect, right after setting all new c_ptr->effect, instead of in
   the beginning before generating the new effect-step. This helps that monsters don't 'jump' an effect by lucky timing. */
#define EFF_DAMAGE_AFTER_SETTING
static void apply_effect(int k, int *who, struct worldpos *wpos, int x, int y, cave_type *c_ptr) {
#if 0 //#ifdef EFF_DAMAGE_AFTER_SETTING
	effect_type *e_ptr = &effects[k];
	//not sure whether the mod_ball_spell_flags() should be used actually:
	int flg = mod_ball_spell_flags(e_ptr->type, PROJECT_NORF | PROJECT_GRID | PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE | PROJECT_JUMP | PROJECT_NODO | PROJECT_NODF);

	/* Imprint on grid */
	c_ptr->effect = k;

	/* Redraw for everyone */
	everyone_lite_spot(wpos, y, x);

	/* Apply damage -- maybe efficient to add a skip here for non-damaging visual-only effects? */
	if (!(e_ptr->flags & EFF_DUMMY))
		project(*who, 0, wpos, y, x, e_ptr->dam, e_ptr->type, flg, "");

		/* The caster got ghost-killed by the projection (or just disconnected)? If it was a real player, handle it: */
		if (*who < 0 && *who > PROJECTOR_UNUSUAL &&
		    Players[0 - *who]->conn == NOT_CONNECTED) {
			/* Make the effect friendly after death - mikaelh */
			*who = PROJECTOR_PLAYER;
		}
		/* Storms end instantly though if the caster is gone or just died. (PROJECTOR_PLAYER falls through from above check.) */
		if (((e_ptr->flags & EFF_STORM) || (e_ptr->flags & EFF_VORTEX))
		    && (*who == PROJECTOR_PLAYER || Players[0 - *who]->death)) {
			erase_effects(k);
			*who = 0;
		}
	}
#else
	/* Old way - just imprint and redraw */
	c_ptr->effect = k;
	everyone_lite_spot(wpos, y, x);
#endif
}

/*
 * Handle staying spell effects once every 10 game turns
 */
/*
 * TODO:
 * - excise dead effect for efficiency
 * - allow players/monsters to 'cancel' the effect
 * - implement EFF_LAST eraser (for now, no spell has this flag)
 * - reduce the use of everyone_lite_spot (one call, one packet)
 */
/*
 * XXX Check it:
 * - what happens if the player dies?
 * - what happens to the storm/wave when player changes floor?
 *   (A. the storm is left on the original floor, and still try to follow
 *    the caster, pfft)
 */
/* Set these to animate the colours of an effect if they aren't already flickering
   colours such as TERM_POIS etc. but instead static colours that are picked randomly
   in spell_color(). - C. Blue */
#define ANIMATE_EFFECTS /* animates spell_color() randomness, costs more bandwidth */
#define FREQUENT_EFFECT_ANIMATION /* costs even more bandwidth */
static void process_effects(void) {
	int i, j, k, l;
	worldpos *wpos;
	cave_type **zcave;
	cave_type *c_ptr;
	int who;
	player_type *p_ptr;
	monster_type *m_ptr;
	effect_type *e_ptr;
	int flg;
	bool skip;
	bool is_player;

	for (k = 0; k < MAX_EFFECTS; k++) {
		e_ptr = &effects[k];
		/* Skip empty slots */
		if (e_ptr->time == 0) continue;

		wpos = &e_ptr->wpos;
		if (!(zcave = getcave(wpos))) {
			e_ptr->time = 0;
			/* TODO - excise it - see right below! */
			continue;
		}
		/* See above - It ends and for some reason getcave() was fine again? Excise it.
		   This is kinda paranoia probably? */
		if (e_ptr->time <= 0) {
			erase_effects(k);
			continue;
		}

		//not sure whether the mod_ball_spell_flags() should be used actually:
		flg = mod_ball_spell_flags(e_ptr->type, PROJECT_NORF | PROJECT_GRID | PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE | PROJECT_JUMP | PROJECT_NODO | PROJECT_NODF);

		/* New: EFF_SELF transports PROJECT_SELF aka self-harming spells, which we now translate back into PROJECT_SELF to inflict the periodic projection of this effect. */
		if (e_ptr->flags & EFF_SELF) flg |= PROJECT_SELF;

#ifdef ARCADE_SERVER
 #if 0
		if ((e_ptr->flags & EFF_CROSSHAIR_A) || (e_ptr->flags & EFF_CROSSHAIR_B) || (e_ptr->flags & EFF_CROSSHAIR_C)) {

			/* e_ptr->interval is player who controls it */
			if (e_ptr->interval >= NumPlayers) {
				p_ptr = Players[e_ptr->interval];
				if (k == p_ptr->e) {
					if (e_ptr->cy != p_ptr->arc_b || e_ptr->cx != p_ptr->arc_a) {
						if (!in_bounds(e_ptr->cy, e_ptr->cx)) continue;
						c_ptr = &zcave[e_ptr->cy][e_ptr->cx];
						c_ptr->effect = 0;
						everyone_lite_spot(wpos, e_ptr->cy, e_ptr->cx);
						e_ptr->cy = p_ptr->arc_b;
						e_ptr->cx = p_ptr->arc_a;
						if (!in_bounds(e_ptr->cy, e_ptr->cx)) continue;
						c_ptr = &zcave[e_ptr->cy][e_ptr->cx];
						c_ptr->effect = k;
						everyone_lite_spot(wpos, e_ptr->cy, e_ptr->cx);
					}

				} else {
					erase_effects(k);
				}
			} else {
				erase_effects(k);
			}
			continue;
		}
 #endif
#endif

#ifndef EXTENDED_TERM_COLOURS
 #ifdef ANIMATE_EFFECTS
  #ifdef FREQUENT_EFFECT_ANIMATION
		/* hack: animate the effect! Note that this is independant of the effect interval,
		   as opposed to the other animation code below. - C. Blue */
		if (!(turn % (cfg.fps / 5)) && e_ptr->time && spell_color_animation(e_ptr->type))
			for (l = 0; l < tdi[e_ptr->rad]; l++) {
				j = e_ptr->cy + tdy[l];
				i = e_ptr->cx + tdx[l];
				if (!in_bounds(j, i)) continue;
				everyone_lite_spot(wpos, j, i);
			}
  #endif
 #endif
#endif

		/* check if it's time to process this effect now (depends on level_speed) */
		if ((turn % (e_ptr->interval * level_speed(wpos) / (level_speeds[0] * 5))) != 0) continue;

		/* Reduce duration */
		e_ptr->time--;

		/* effect belongs to a non-player? (Assume unchanging who, aka PROJECTOR_xxx values, not m_idx indices, or we'd have to verify if the monster is actually still alive!) */
		if (e_ptr->who > 0 || e_ptr->who <= PROJECTOR_UNUSUAL) who = e_ptr->who;
		/* or to a player? Confirm his Ind then, in case it meanwhile changed! */
		else {
			/* --- At first, assume Ind is no longer valid and replace by PROJECTOR_xxx: --- */

			/* Make the effect friendly after logging out - mikaelh */
			if (!(e_ptr->flags & EFF_SELF)) who = PROJECTOR_PLAYER;
			/* Or make the effect harmful to players instead, if the original projection was harmful too (PROJECT_SELF->EFF_SELF) */
			else who = PROJECTOR_EFFECT;

			/* --- Now verify player existance and reinstantiate his Ind if valid: --- */

			/* Hack -- is the trapper online? (Why call him 'trapper' specifically?) */
			for (i = 1; i <= NumPlayers; i++) {
				p_ptr = Players[i];

				/* Check if they are in here */
				if (e_ptr->who_id != p_ptr->id) continue;

				/* additionally check if player left level --
				   avoids panic save on killing a dungeon boss with a lasting effect while
				   already having been prematurely recalled to town meanwhile (insta-res!) - C. Blue */
				if (!inarea(&p_ptr->wpos, wpos)) break;

				/* it's fine, player still exists here, verify the Ind and proceed */
				who = 0 - i;
				break;
			}
		}
		is_player = (who < 0 && who > PROJECTOR_UNUSUAL);

		/* Storm ends if the cause is gone */
		if (((e_ptr->flags & EFF_STORM) || (e_ptr->flags & EFF_VORTEX))
		    && (who == PROJECTOR_EFFECT || who == PROJECTOR_PLAYER)) {
			erase_effects(k);
			continue;
		}

		/* Snowflakes disappear if they reach end of traversed screen */
		if ((e_ptr->flags & EFF_SNOWING) && e_ptr->cy == MAX_HGT - 2) {
			erase_effects(k);
			continue;
		}
		/* Raindrops disappear if they reach end of traversed screen */
		if ((e_ptr->flags & EFF_RAINING) && e_ptr->cy == MAX_HGT - 2) {
			erase_effects(k);
			continue;
		}
		/* Falling stars disappear if they reach end of traversed screen */
		if ((e_ptr->flags & EFF_FALLING_STAR) && (e_ptr->cx == MAX_WID - 2 || e_ptr->cx == 1)) {
			erase_effects(k);
			continue;
		}

		/* Handle spell effects */
		for (l = 0; l < tdi[e_ptr->rad]; l++) {
			i = e_ptr->cx + tdx[l];
			j = e_ptr->cy + tdy[l];
			if (!in_bounds(j, i)) continue;

			c_ptr = &zcave[j][i];

			/* Don't allow multiple effects to hit the same grid */
			if (c_ptr->effect && c_ptr->effect != k) skip = TRUE;
			else skip = FALSE;

			/* Remove memories of where the effect boundary was previously */
			if (c_ptr->effect_past == k) c_ptr->effect_past = 0;

#ifndef EFF_DAMAGE_AFTER_SETTING
			/* For all currently affected grids:
			   Apply colour animation and damage projection, erase effect if timeout results from projection somehow */
			if (c_ptr->effect == k) {
				/* Apply damage */
				if (!(e_ptr->flags & EFF_DUMMY)) {
					project(who, 0, wpos, j, i, e_ptr->dam, e_ptr->type, flg, "");

					/* The caster got ghost-killed by the projection (or just disconnected)? If it was a real player, handle it: */
					if (is_player && Players[0 - who]->conn == NOT_CONNECTED) {
						/* Make the effect friendly after death - mikaelh */
						if (!(e_ptr->flags & EFF_SELF)) who = PROJECTOR_PLAYER;
						else who = PROJECTOR_EFFECT; /* Make effect harmful to players instead */
					}
					/* Storms end instantly though if the caster is gone or just died. (PROJECTOR_PLAYER falls through from above check.) */
					if (((e_ptr->flags & EFF_STORM) || (e_ptr->flags & EFF_VORTEX))
					    && (who == PROJECTOR_PLAYER || (is_player && Players[0 - who]->death))) {
						erase_effects(k);
						break;
					}
				}
 #ifndef EXTENDED_TERM_COLOURS
  #ifdef ANIMATE_EFFECTS
   #ifndef FREQUENT_EFFECT_ANIMATION
				/* C. Blue - hack: animate effects inbetween ie allow random changes in spell_color().
				   Note: animation speed depends on effect interval. */
				if (spell_color_animation(e_ptr->type))
				    everyone_lite_spot(wpos, j, i);
   #endif
  #endif
 #endif
			}
#endif

			/* If it's a changing effect, remove it from currently affected grid to prepare for next iteration */
			if (!(e_ptr->flags & EFF_LAST)
			    && c_ptr->effect == k) {
				if ((e_ptr->flags & EFF_WAVE)) {
					if (distance(e_ptr->cy, e_ptr->cx, j, i) < e_ptr->rad - 2) { //wave has thickness 3: each taget gets hit 3 times
						c_ptr->effect = 0;
						everyone_lite_spot(wpos, j, i);
					}
				} else if ((e_ptr->flags & EFF_THINWAVE)) {
					if (distance(e_ptr->cy, e_ptr->cx, j, i) < e_ptr->rad) { // thin wave has thickness 1: each target gets hit 1 time
						if (c_ptr->effect == k) c_ptr->effect_past = k; /* Don't allow a monster to 'jump' the effect */
						c_ptr->effect = 0;
						everyone_lite_spot(wpos, j, i);
					}
				} else if ((e_ptr->flags & EFF_STORM) || (e_ptr->flags & EFF_VORTEX)) {
					c_ptr->effect = 0;
					everyone_lite_spot(wpos, j, i);
				} else if ((e_ptr->flags & EFF_SNOWING)) {
					c_ptr->effect = 0;
					everyone_lite_spot(wpos, j, i);
				} else if ((e_ptr->flags & EFF_RAINING)) {
					c_ptr->effect = 0;
					everyone_lite_spot(wpos, j, i);
				} else if (e_ptr->flags & (EFF_FIREWORKS1 | EFF_FIREWORKS2 | EFF_FIREWORKS3)) {
					c_ptr->effect = 0;
					everyone_lite_spot(wpos, j, i);
#if 0 /* no need to erase inbetween, while effect is still expanding - at the same time this fixes ugly tile flickering from redrawing (lava!) */
				} else if (e_ptr->flags & (EFF_LIGHTNING1 | EFF_LIGHTNING2 | EFF_LIGHTNING3)) {
					c_ptr->effect = 0;
					everyone_lite_spot(wpos, j, i);
#endif
				} else if ((e_ptr->flags & EFF_FALLING_STAR)) {
					c_ptr->effect = 0;
					everyone_lite_spot(wpos, j, i);
				} else if ((e_ptr->flags & EFF_SEEKER)) {
					c_ptr->effect = 0;
					everyone_lite_spot(wpos, j, i);
				}
			}

			/* --- Complex effects: Create next effect iteration and imprint it on grid --- */

			/* Creates a "wave" effect*/
			if (e_ptr->flags & (EFF_WAVE | EFF_THINWAVE)) {
				if (projectable(wpos, e_ptr->cy, e_ptr->cx, j, i, MAX_RANGE)
				    //&& projectable(wpos, e_ptr->caster_y, e_ptr->caster_x, j, i, MAX_RANGE) --enable if caster_x/y are needed for special combo effects, if ever
				    && (distance(e_ptr->cy, e_ptr->cx, j, i) == e_ptr->rad))
					apply_effect(k, &who, wpos, i, j, c_ptr);
			}

			/* Generate fireworks effects */
			else if (e_ptr->flags & (EFF_FIREWORKS1 | EFF_FIREWORKS2 | EFF_FIREWORKS3)) {
				int semi = (e_ptr->time + e_ptr->rad) / 2;

				/* until half-time (or half-radius) the fireworks rise into the air */
				if (e_ptr->rad < e_ptr->time) {
					e_ptr->cflags = 1;
					if (i == e_ptr->cx && j == e_ptr->cy - e_ptr->rad)
						apply_effect(k, &who, wpos, i, j, c_ptr);
				} else { /* after that, they explode (w00t) */
					e_ptr->cflags = 2;
					/* explosion is faster than flying upwards */
					//doesn't work-   e_ptr->interval = 2;
#ifdef USE_SOUND_2010
					if (e_ptr->rad == e_ptr->time + 1 && !(e_ptr->flags & EFF_TEMP)) {
						e_ptr->flags |= EFF_TEMP; /* Play the sfx only during this one frame, not during ALL frames of this current e_ptr->rad period. Those would stack terribly (if ovl_sfx_misc isn't disabled). */
						if ((e_ptr->flags & EFF_FIREWORKS3))
							//sound_near_site(e_ptr->cy, e_ptr->cx, wpos, 0, "fireworks_big", "", SFX_TYPE_MISC, FALSE);
							sound_floor_vol(wpos, "fireworks_big", "", SFX_TYPE_MISC, randint(26) + 75);
						else if ((e_ptr->flags & EFF_FIREWORKS2))
							//sound_near_site(e_ptr->cy, e_ptr->cx, wpos, 0, "fireworks_norm", "", SFX_TYPE_MISC, FALSE);
							sound_floor_vol(wpos, "fireworks_norm", "", SFX_TYPE_MISC, randint(26) + 75);
						else
							//sound_near_site(e_ptr->cy, e_ptr->cx, wpos, 0, "fireworks_small", "", SFX_TYPE_MISC, FALSE);
							sound_floor_vol(wpos, "fireworks_small", "", SFX_TYPE_MISC, randint(26) + 75);
					}
#endif

#if 0 /* just for testing */
					if (e_ptr->flags & EFF_FIREWORKS1) { /* simple rocket (line) */
						if (i == e_ptr->cx && j == e_ptr->cy - e_ptr->rad)
							apply_effect(k, &who, wpos, i, j, c_ptr);
					}
#endif
					if (e_ptr->flags & EFF_FIREWORKS1) { /* 3-star */
						if (((i == e_ptr->cx && j >= e_ptr->cy - e_ptr->rad) && /* up */
						    (i == e_ptr->cx && j <= e_ptr->cy + 1 - e_ptr->rad)) ||
						    ((i >= e_ptr->cx + semi - e_ptr->rad && j == e_ptr->cy - semi) && /* left */
						    (i <= e_ptr->cx + semi + 1 - e_ptr->rad && j == e_ptr->cy - semi)) ||
						    ((i <= e_ptr->cx - semi + e_ptr->rad && j == e_ptr->cy - semi) && /* right */
						    (i >= e_ptr->cx - semi - 1 + e_ptr->rad && j == e_ptr->cy - semi)))
							apply_effect(k, &who, wpos, i, j, c_ptr);
					} else if (e_ptr->flags & EFF_FIREWORKS2) { /* 5-star */
						if (((i == e_ptr->cx && j >= e_ptr->cy - e_ptr->rad) && /* up */
						    (i == e_ptr->cx && j <= e_ptr->cy + 1 - e_ptr->rad)) ||
						    ((i >= e_ptr->cx + semi - e_ptr->rad && j >= e_ptr->cy - e_ptr->rad) && /* up-left */
						    (i <= e_ptr->cx + semi + 1 - e_ptr->rad && j <= e_ptr->cy + 1 - e_ptr->rad)) ||
						    ((i <= e_ptr->cx - semi + e_ptr->rad && j >= e_ptr->cy - e_ptr->rad) && /* up-right */
						    (i >= e_ptr->cx - semi - 1 + e_ptr->rad && j <= e_ptr->cy + 1 - e_ptr->rad)) ||
						    ((i >= e_ptr->cx + semi - e_ptr->rad && j <= e_ptr->cy - 2 * semi + e_ptr->rad) && /* down-left */
						    (i <= e_ptr->cx + semi + 1 - e_ptr->rad && j >= e_ptr->cy - 0 - 2 * semi + e_ptr->rad)) ||
						    ((i <= e_ptr->cx - semi + e_ptr->rad && j <= e_ptr->cy - 2 * semi + e_ptr->rad) && /* down-right */
						    (i >= e_ptr->cx - semi - 1 + e_ptr->rad && j >= e_ptr->cy - 0 - 2 * semi + e_ptr->rad)))
							apply_effect(k, &who, wpos, i, j, c_ptr);
					} else { /* EFF_FIREWORKS3 */ /* 7-star whoa */
						if (((i == e_ptr->cx && j >= e_ptr->cy - e_ptr->rad) && /* up */
						    (i == e_ptr->cx && j <= e_ptr->cy + 1 - e_ptr->rad)) ||
						    ((i >= e_ptr->cx + semi - e_ptr->rad && j == e_ptr->cy - semi) && /* left */
						    (i <= e_ptr->cx + semi + 1 - e_ptr->rad && j == e_ptr->cy - semi)) ||
						    ((i <= e_ptr->cx - semi + e_ptr->rad && j == e_ptr->cy - semi) && /* right */
						    (i >= e_ptr->cx - semi - 1 + e_ptr->rad && j == e_ptr->cy - semi)) ||
						    ((i >= e_ptr->cx + semi - e_ptr->rad && j >= e_ptr->cy - e_ptr->rad) && /* up-left */
						    (i <= e_ptr->cx + semi + 1 - e_ptr->rad && j <= e_ptr->cy + 1 - e_ptr->rad)) ||
						    ((i <= e_ptr->cx - semi + e_ptr->rad && j >= e_ptr->cy - e_ptr->rad) && /* up-right */
						    (i >= e_ptr->cx - semi - 1 + e_ptr->rad && j <= e_ptr->cy + 1 - e_ptr->rad)) ||
						    ((i >= e_ptr->cx + semi - e_ptr->rad && j <= e_ptr->cy - 2 * semi + e_ptr->rad) && /* down-left */
						    (i <= e_ptr->cx + semi + 1 - e_ptr->rad && j >= e_ptr->cy - 0 - 2 * semi + e_ptr->rad)) ||
						    ((i <= e_ptr->cx - semi + e_ptr->rad && j <= e_ptr->cy - 2 * semi + e_ptr->rad) && /* down-right */
						    (i >= e_ptr->cx - semi - 1 + e_ptr->rad && j >= e_ptr->cy - 0 - 2 * semi + e_ptr->rad)))
							apply_effect(k, &who, wpos, i, j, c_ptr);
					}
				}
			}

			/* Generate meteor effects */
			else if (e_ptr->flags & EFF_METEOR) {
				if (e_ptr->time) {
					if (i == e_ptr->cx) {
						apply_effect(k, &who, wpos, i, j, c_ptr);
						if (j == e_ptr->cy) c_ptr->effect_xtra = 0;
						else c_ptr->effect_xtra = 2;
					} else {
						if (j == e_ptr->cy) {
							apply_effect(k, &who, wpos, i, j, c_ptr);
							c_ptr->effect_xtra = 1;
						}
#if 0 /* draw diagonals too? - no, too much clutter */
						else {
							apply_effect(k, &who, wpos, i, j, c_ptr);
							c_ptr->effect_xtra = 3 + (j < e_ptr->cy ? 0 : 1) + (i < e_ptr->cx ? 0 : 2);
						}
#endif
					}
				} else if (i == e_ptr->cx && j == e_ptr->cy)
					ball_noInd(who, e_ptr->type, e_ptr->dam, wpos, j, i, 1);
			}

			/* Generate lightning effects -- effect_xtra: -1\ 0| 1/ 2_ */
			else if (e_ptr->flags & (EFF_LIGHTNING1 | EFF_LIGHTNING2 | EFF_LIGHTNING3)) {
				int mirrored = (e_ptr->dam == 0) ? -1 : 1;

				if ((e_ptr->flags & EFF_LIGHTNING1)) {
					int stage = e_ptr->rad;

					if (stage > 15) stage = 15; /* afterglow */

					switch (stage) {
					case 15:
						if (i == e_ptr->cx + mirrored * 14 && j == e_ptr->cy + 4) {///
							c_ptr->effect_xtra = -mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 14:
						if (i == e_ptr->cx + mirrored * 13 && j == e_ptr->cy + 3) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 13:
						if (i == e_ptr->cx + mirrored * 12 && j == e_ptr->cy + 3) {///
							c_ptr->effect_xtra = -mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx + mirrored * 6 && j == e_ptr->cy + 5) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 12:
						if (i == e_ptr->cx + mirrored * 11 && j == e_ptr->cy + 2) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx + mirrored * 5 && j == e_ptr->cy + 5) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 11:
						if (i == e_ptr->cx + mirrored * 10 && j == e_ptr->cy + 2) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx + mirrored * 4 && j == e_ptr->cy + 5) {///
							c_ptr->effect_xtra = -mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 10:
						if (i == e_ptr->cx + mirrored * 9 && j == e_ptr->cy + 2) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx + mirrored * 3 && j == e_ptr->cy + 4) {///
							c_ptr->effect_xtra = -mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 9:
						if (i == e_ptr->cx + mirrored * 8 && j == e_ptr->cy + 2) {///
							c_ptr->effect_xtra = -mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx + mirrored * 3 && j == e_ptr->cy + 3) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 8:
						if (i == e_ptr->cx + mirrored * 7 && j == e_ptr->cy + 1) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx + mirrored * 4 && j == e_ptr->cy + 3) {//`
							c_ptr->effect_xtra = mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 7:
						if (i == e_ptr->cx + mirrored * 6 && j == e_ptr->cy + 1) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx + mirrored * 5 && j == e_ptr->cy + 2) {//`
							c_ptr->effect_xtra = mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 6:
						if (i == e_ptr->cx + mirrored * 5 && j == e_ptr->cy + 1) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 5:
						if (i == e_ptr->cx + mirrored * 4 && j == e_ptr->cy + 1) {///
							c_ptr->effect_xtra = -mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 4:
						if (i == e_ptr->cx + mirrored * 3 && j == e_ptr->cy) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 3:
						if (i == e_ptr->cx + mirrored * 2 && j == e_ptr->cy) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 2:
						if (i == e_ptr->cx + mirrored * 1 && j == e_ptr->cy) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 1:
						if (i == e_ptr->cx && j == e_ptr->cy) {//_
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
					}
				} else if ((e_ptr->flags & EFF_LIGHTNING2)) {
					int stage = e_ptr->rad;

					if (stage > 8) stage = 8; /* afterglow */

					switch (stage) {
					case 8:
						if (i == e_ptr->cx + mirrored * 6 && j == e_ptr->cy + 5) {
							c_ptr->effect_xtra = mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 7:
						if (i == e_ptr->cx + mirrored * 6 && j == e_ptr->cy + 4) {
							c_ptr->effect_xtra = -mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 6:
						if (i == e_ptr->cx + mirrored * 5 && j == e_ptr->cy + 3) {
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx - mirrored * (4 + 1) && j == e_ptr->cy + 3) {
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 5:
						if (i == e_ptr->cx + mirrored * 4 && j == e_ptr->cy + 3) {
							c_ptr->effect_xtra = -mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx - mirrored * (3 + 1) && j == e_ptr->cy + 3) {
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 4:
						if (i == e_ptr->cx + mirrored * 3 && j == e_ptr->cy + 2) {
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx - mirrored * (2 + 1) && j == e_ptr->cy + 3) {
							c_ptr->effect_xtra = mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 3:
						if (i == e_ptr->cx + mirrored * 2 && j == e_ptr->cy + 2) {
							c_ptr->effect_xtra = -mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx - mirrored * 2 && j == e_ptr->cy + 2) {
							c_ptr->effect_xtra = 0;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 2:
						if (i == e_ptr->cx + mirrored * 1 && j == e_ptr->cy + 1) {
							c_ptr->effect_xtra = -mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx - mirrored * 1 && j == e_ptr->cy + 1) {
							c_ptr->effect_xtra = mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 1:
						if (i == e_ptr->cx && j == e_ptr->cy) {
							c_ptr->effect_xtra = 0;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
					}
				} else if ((e_ptr->flags & EFF_LIGHTNING3)) {
					int stage = e_ptr->rad;

					if (stage > 10) stage = 10; /* afterglow */

					switch (stage) {
					case 10:
						if (i == e_ptr->cx - mirrored * 8 && j == e_ptr->cy + 6) {
							c_ptr->effect_xtra = 0;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 9:
						if (i == e_ptr->cx - mirrored * 7 && j == e_ptr->cy + 5) {
							c_ptr->effect_xtra = mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 8:
						if (i == e_ptr->cx - mirrored * 6 && j == e_ptr->cy + 4) {
							c_ptr->effect_xtra = mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 7:
						if (i == e_ptr->cx - mirrored * 5 && j == e_ptr->cy + 3) {
							c_ptr->effect_xtra = 0;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 6:
						if (i == e_ptr->cx - mirrored * 5 && j == e_ptr->cy + 2) {
							c_ptr->effect_xtra = mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx - mirrored && j == e_ptr->cy + 4) {
							c_ptr->effect_xtra = 0;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 5:
						if (i == e_ptr->cx - mirrored * 4 && j == e_ptr->cy + 1) {
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx - mirrored * 2 && j == e_ptr->cy + 3) {
							c_ptr->effect_xtra = -mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 4:
						if (i == e_ptr->cx - mirrored * 3 && j == e_ptr->cy + 1) {
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						if (i == e_ptr->cx - mirrored * 3 && j == e_ptr->cy + 2) {
							c_ptr->effect_xtra = 0;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 3:
						if (i == e_ptr->cx - mirrored * 2 && j == e_ptr->cy + 1) {
							c_ptr->effect_xtra = mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 2:
						if (i == e_ptr->cx - mirrored * 1 && j == e_ptr->cy) {
							c_ptr->effect_xtra = 2;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
						/* Fall through */
					case 1:
						if (i == e_ptr->cx && j == e_ptr->cy) {
							c_ptr->effect_xtra = mirrored;
							apply_effect(k, &who, wpos, i, j, c_ptr);
						}
					}
				}
			}

#ifdef EFF_DAMAGE_AFTER_SETTING
			/* For persisting grids (newly affected grids are treated in apply_effect() calls instead) of this effect:
			   Apply colour animation and damage projection, erase effect if timeout results from projection somehow */
			if (c_ptr->effect == k) {
				/* Apply damage */
				if (!skip && !(e_ptr->flags & EFF_DUMMY)) {
					project(who, 0, wpos, j, i, e_ptr->dam, e_ptr->type, flg, "");

					/* The caster got ghost-killed by the projection (or just disconnected)? If it was a real player, handle it: */
					if (is_player && Players[0 - who]->conn == NOT_CONNECTED) {
						/* Make the effect friendly after death - mikaelh */
						if (!(e_ptr->flags & EFF_SELF)) who = PROJECTOR_PLAYER;
						else who = PROJECTOR_EFFECT; /* Make effect harmful to players instead */
					}
					/* Storms end instantly though if the caster is gone or just died. (PROJECTOR_PLAYER falls through from above check.) */
					if (((e_ptr->flags & EFF_STORM) || (e_ptr->flags & EFF_VORTEX))
					    && (who == PROJECTOR_PLAYER || (is_player && Players[0 - who]->death))) {
						erase_effects(k);
						break;
					}
				}
 #ifndef EXTENDED_TERM_COLOURS
  #ifdef ANIMATE_EFFECTS
   #ifndef FREQUENT_EFFECT_ANIMATION
				/* C. Blue - hack: animate effects inbetween ie allow random changes in spell_color().
				   Note: animation speed depends on effect interval. */
				if (spell_color_animation(e_ptr->type))
				    everyone_lite_spot(wpos, j, i);
   #endif
  #endif
 #endif
			}
#endif

			/* We lost our caster for some reason? (Died to projection) */
			if (!who) {
				is_player = FALSE;
				break;
			}
		}

		/* --- Imprint simple non-damaging effects on grid (project() was already called above in the loop).
		       Handle effects that change their origin coordinate, as these don't fit into the loop above (storms).
		       Also process modification of all changeable effects (radius/movement). --- */

		/* Expand waves */
		if (e_ptr->flags & (EFF_WAVE | EFF_THINWAVE)) e_ptr->rad++;

		/* Creates a "storm" effect*/
		else if ((e_ptr->flags & EFF_STORM) && is_player) { //todo: the player-checks are paranoia? We already checked this far above
			p_ptr = Players[0 - who];

			e_ptr->cy = p_ptr->py;
			e_ptr->cx = p_ptr->px;

			for (l = 0; l < tdi[e_ptr->rad]; l++) {
				j = e_ptr->cy + tdy[l];
				i = e_ptr->cx + tdx[l];
				if (!in_bounds(j, i)) continue;

				c_ptr = &zcave[j][i];
				if (c_ptr->effect && c_ptr->effect != k) continue; /* 'skip' */

				if (projectable(wpos, e_ptr->cy, e_ptr->cx, j, i, MAX_RANGE)
				    //&& projectable(wpos, e_ptr->caster_y, e_ptr->caster_x, j, i, MAX_RANGE) --enable if caster_x/y are needed for special combo effects, if ever
				    && (distance(e_ptr->cy, e_ptr->cx, j, i) <= e_ptr->rad)) {
					apply_effect(k, &who, wpos, i, j, c_ptr);
					if (!(e_ptr->flags & EFF_DUMMY)) {
						project(who, 0, wpos, j, i, e_ptr->dam, e_ptr->type, flg, "");
						/* The caster got ghost-killed by the projection (or just disconnected)? If it was a real player, handle it: */
						if (Players[0 - who]->conn == NOT_CONNECTED || Players[0 - who]->death) {
							erase_effects(k);
							break;
						}
					}
				}
			}
		}

		/* Maintain a "vortex" effect - Kurzel */
		else if ((e_ptr->flags & EFF_VORTEX) && who > PROJECTOR_EFFECT) {  // TODO: verify this PROJECTOR_EFFECT comparison for correctness
			e_ptr->rad = 0; //Hack: Fix strange EFF_VORTEX behavior...
			if (e_ptr->whot > 0) {
				m_ptr = &m_list[e_ptr->whot];
				if (!m_ptr->r_idx) e_ptr->whot = 0;
				else if (!inarea(wpos, &m_ptr->wpos)) e_ptr->whot = 0;
				// Hack: Check for distance, since m_ptr may be a new mob after death!
				else if (distance(e_ptr->cy, e_ptr->cx, m_ptr->fy, m_ptr->fx) > 3) e_ptr->whot = 0;
				else {
					e_ptr->cy = m_ptr->fy;
					e_ptr->cx = m_ptr->fx;
				}
			} else if (e_ptr->whot < 0) {
				p_ptr = Players[0 - e_ptr->whot];
				if (p_ptr->conn == NOT_CONNECTED) e_ptr->whot = 0;
				else if (p_ptr->death) e_ptr->whot = 0;
				else if (!inarea(wpos, &p_ptr->wpos)) e_ptr->whot = 0;
				else {
					e_ptr->cy = p_ptr->py;
					e_ptr->cx = p_ptr->px;
				}
			} else {
				// Hack: Decay rapidly without a target.
				e_ptr->time -= 1;
			}
			if (e_ptr->rad == 0) {
				c_ptr = &zcave[e_ptr->cy][e_ptr->cx];
				c_ptr->effect = k;
				if (!(e_ptr->flags & EFF_DUMMY))
					project(who, 0, wpos, e_ptr->cy, e_ptr->cx, e_ptr->dam, e_ptr->type, flg, "");
				everyone_lite_spot(wpos, e_ptr->cy, e_ptr->cx);
			} else {
				for (l = 0; l < tdi[e_ptr->rad]; l++) {
					j = e_ptr->cy + tdy[l];
					i = e_ptr->cx + tdx[l];
					if (!in_bounds(j, i)) continue;
					c_ptr = &zcave[j][i];
					if (projectable(wpos, e_ptr->cy, e_ptr->cx, j, i, MAX_RANGE)
					    //&& projectable(wpos, e_ptr->caster_y, e_ptr->caster_x, j, i, MAX_RANGE) --enable if caster_x/y are needed for special combo effects, if ever
					    && (distance(e_ptr->cy, e_ptr->cx, j, i) <= e_ptr->rad)) {
						c_ptr->effect = k;
						if (!(e_ptr->flags & EFF_DUMMY))
							project(who, 0, wpos, j, i, e_ptr->dam, e_ptr->type, flg, "");
						everyone_lite_spot(wpos, j, i);
					}
				}
			}
		}

		/* Seeker missiles */
		else if (e_ptr->flags & EFF_SEEKER) {
#if 0
			player_type *p_ptr;

			/* imprint target player coords until we get relatively close, then it's free flight and player can more easily evade */
			if (e_ptr->time > 10 && e_ptr->whot && e_ptr->whot <= NumPlayers) {
				p_ptr = Players[e_ptr->whot];
				if (inarea(&p_ptr->wpos, wpos)) {
					e_ptr->tx = p_ptr->px;
					e_ptr->ty = p_ptr->py;
				}
			}
#endif

			/* needed? */
			e_ptr->rad = 0;

			/* steer */
			if (e_ptr->tx < e_ptr->cx) e_ptr->cx--;
			if (e_ptr->tx > e_ptr->cx) e_ptr->cx++;
			if (e_ptr->ty < e_ptr->cy) e_ptr->cy--;
			if (e_ptr->ty > e_ptr->cy) e_ptr->cy++;

			/* imprint on map grid */
			c_ptr = &zcave[e_ptr->cy][e_ptr->cx];
			apply_effect(k, &who, wpos, e_ptr->cx, e_ptr->cy, c_ptr);

			/* Impact? */
			if (!e_ptr->time) {
				int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO;

				(void)project(PROJECTOR_UNUSUAL, 1, wpos, e_ptr->cy, e_ptr->cx, 10, GF_LITE, flg, "");
#ifdef USE_SOUND_2010
				//if (p_ptr->sfx_monsterattack)
				sound_near_site(e_ptr->cy, e_ptr->cx, wpos, 0, "fireworks_big", "cast_ball", SFX_TYPE_MON_SPELL, FALSE);
#endif
			}
		}

		/* Drift snowflakes */
		else if (e_ptr->flags & EFF_SNOWING) {
			e_ptr->cy++; /* for now just fall straight downwards */
			/* gusts of wind */
			if (wind_gust > 0) {
				e_ptr->cx--;
				if (e_ptr->cx < 1) e_ptr->cx = MAX_WID - 2;
			}
			if (wind_gust < 0) {
				e_ptr->cx++;
				if (e_ptr->cx >= MAX_WID - 1) e_ptr->cx = 1;
			}
			c_ptr = &zcave[e_ptr->cy][e_ptr->cx];
			apply_effect(k, &who, wpos, e_ptr->cx, e_ptr->cy, c_ptr);
		}
		/* Drift raindrops */
		else if (e_ptr->flags & EFF_RAINING) {
			e_ptr->cy++; /* for now just fall straight downwards */
			/* gusts of wind */
			if (wind_gust > 0) {
				e_ptr->cx--;
				if (e_ptr->cx < 1) e_ptr->cx = MAX_WID - 2;
			}
			if (wind_gust < 0) {
				e_ptr->cx++;
				if (e_ptr->cx >= MAX_WID - 1) e_ptr->cx = 1;
			}
			c_ptr = &zcave[e_ptr->cy][e_ptr->cx];
			apply_effect(k, &who, wpos, e_ptr->cx, e_ptr->cy, c_ptr);
		}

		/* Launch/explode fireworks */
		else if (e_ptr->flags & (EFF_FIREWORKS1 | EFF_FIREWORKS2 | EFF_FIREWORKS3)) {
			e_ptr->rad++; /* while radius < time/2 -> "rise into the air", otherwise "explode" */
		}

		/* Expand lightning */
		else if ((e_ptr->flags & (EFF_LIGHTNING1 | EFF_LIGHTNING2 | EFF_LIGHTNING3))
		    && e_ptr->rad < 15) {
			e_ptr->rad++;
		}

		/* Slide falling stars */
		else if (e_ptr->flags & EFF_FALLING_STAR) {
			if (TRUE) {
				e_ptr->cx--;
			} else {
				e_ptr->cx++;
			}
			c_ptr = &zcave[e_ptr->cy][e_ptr->cx];
			apply_effect(k, &who, wpos, e_ptr->cx, e_ptr->cy, c_ptr);
		}

		/* Thunderstorm visual */
		else if (e_ptr->flags & EFF_THUNDER_VISUAL) {
			c_ptr = &zcave[e_ptr->cy][e_ptr->cx];
			apply_effect(k, &who, wpos, e_ptr->cx, e_ptr->cy, c_ptr);
		}

		/* Excise effects that reached end of their lifetime, instead of relying only on the
		   copy of this check at the beginning of process_effects(). Reason: The effect would
		   visually linger for another project-interval then, without doing anything. So just
		   clear it up yet, so players don't get confused. */
		if (e_ptr->time <= 0) {
			erase_effects(k);
			continue;
		}
	}

#if 0 /* Done in process_player_end_aux() */
	/* Apply sustained effect in the player grid, if any */
	apply_terrain_effect(py, px);
#endif
}

#endif /* pelpel */



/*
 * Queued drawing at the beginning of a new turn.
 */
static void process_lite_later(void) {
	int i;
	struct worldspot *wspot;

	for (i = 0; i < lite_later_num; i++) {
		wspot = &lite_later[i];

		/* Draw now */
		everyone_lite_spot(&wspot->wpos, wspot->y, wspot->x);
	}

	/* All done */
	lite_later_num = 0;
}



/*
 * Regenerate the monsters (once per 100 game turns)
 *
 * XXX XXX XXX Should probably be done during monster turns.
 */
/* Note that since this is done in real time, monsters will regenerate
 * faster in game time the deeper they are in the dungeon.
 */
static void regen_monsters(void) {
	int i, frac;

	/* Regenerate everyone */
	for (i = 1; i < m_max; i++) {
		/* Check the i'th monster */
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = race_inf(m_ptr);

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Allow regeneration (if needed) */
		if (m_ptr->hp < m_ptr->maxhp) {
			/* Hack -- Base regeneration */
			frac = m_ptr->maxhp / 100;

			/* Hack -- Minimal regeneration rate */
			if (!frac) frac = 1;

#if defined(TROLL_REGENERATION) || defined(HYDRA_REGENERATION)
			if (r_ptr->flags2 & RF2_REGENERATE_TH) frac *= 4;
			else if (r_ptr->flags2 & RF2_REGENERATE_T2) frac *= 3;
			else
#endif
#ifdef HYDRA_REGENERATION
			if (r_ptr->d_char == 'M') frac *= 4;
			else
#endif
#ifdef TROLL_REGENERATION
			/* Experimental - Trolls are super-regenerators (hard-coded) */
			if (m_ptr->r_idx == RI_HALF_TROLL) frac *= 3;
			else if (r_ptr->d_char == 'T') frac *= 4;
			else
#endif
			/* Hack -- Some monsters regenerate quickly */
			if (r_ptr->flags2 & RF2_REGENERATE) frac *= 2;

			if (m_ptr->hold_hp_regen) frac = (frac * m_ptr->hold_hp_regen_perc) / 100;
			if (!frac) continue;

			if (m_ptr->r_idx == RI_BLUE) frac /= 40;

			/* Hack -- Regenerate */
			m_ptr->hp += frac;

			/* Do not over-regenerate */
			if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

			/* Update health bars */
			update_health(i);
		}
	}
}



/* update a particular player's view to daylight, assuming he's on world surface */
bool player_day(int Ind) {
	player_type *p_ptr = Players[Ind];
	cave_type **zcave = getcave(&p_ptr->wpos);
	int x, y;
	struct dun_level *l_ptr = getfloor(&p_ptr->wpos);
	bool ret = FALSE;

	/* Weather effect colouring may differ depending on daytime */
	Send_weather_colouring(Ind, TERM_WATE, TERM_WHITE, TERM_L_UMBER, '+');

	/* Shade map and darken/forget features */

	if (!zcave) return(FALSE); /* paranoia */

	if (outdoor_affects(&p_ptr->wpos)) {
		p_ptr->redraw |= (PR_MAP); /* For Cloud Planes shading */
		ret = TRUE;
	}
	if (p_ptr->wpos.wz) return(ret);

	if (p_ptr->global_event_temp & PEVF_INDOORS_00) return(FALSE);
	if (in_sector000(&p_ptr->wpos) && (sector000flags2 & LF2_INDOORS)) return(FALSE);
	if (l_ptr && (l_ptr->flags2 & LF2_INDOORS)) return(FALSE);

	//if (p_ptr->tim_watchlist) p_ptr->tim_watchlist--;

	/* Hack -- Scan the level */
	for (y = 0; y < MAX_HGT; y++)
	for (x = 0; x < MAX_WID; x++) {
		/* Hack -- Memorize lit grids if allowed */
		if (istownarea(&p_ptr->wpos, MAX_TOWNAREA)
		    && (p_ptr->view_perma_grids)) {
			p_ptr->cave_flag[y][x] |= CAVE_MARK;
		}
		note_spot(Ind, y, x);
	}

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);
	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);
	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);


	/* Lastly, handle music */

#ifdef USE_SOUND_2010
	if (p_ptr->is_day) return(FALSE);
	p_ptr->is_day = TRUE;
	handle_music(Ind);
	handle_ambient_sfx(Ind, &zcave[p_ptr->py][p_ptr->px], &p_ptr->wpos, TRUE);
#endif

	return(TRUE);
}
/* update a particular player's view to night, assuming he's on world surface */
bool player_night(int Ind) {
	player_type *p_ptr = Players[Ind];
	cave_type **zcave = getcave(&p_ptr->wpos);
	int x, y;
	struct dun_level *l_ptr = getfloor(&p_ptr->wpos);
	bool ret = FALSE;

	/* Weather effect colouring may differ depending on daytime */
	Send_weather_colouring(Ind, TERM_BLUE, TERM_WHITE, TERM_L_UMBER, '+');

	/* Shade map and darken/forget features */

	if (!zcave) return(FALSE); /* paranoia */

	if (outdoor_affects(&p_ptr->wpos)) {
		p_ptr->redraw |= (PR_MAP); /* For Cloud Planes shading */
		ret = TRUE;
	}
	if (p_ptr->wpos.wz) return(ret);

	if (p_ptr->global_event_temp & PEVF_INDOORS_00) return(FALSE);
	if (in_sector000(&p_ptr->wpos) && (sector000flags2 & LF2_INDOORS)) return(FALSE);
	if (l_ptr && (l_ptr->flags2 & LF2_INDOORS)) return(FALSE);

	//if (p_ptr->tim_watchlist) p_ptr->tim_watchlist--;

	/* Hack -- Scan the level */
	for (y = 0; y < MAX_HGT; y++)
	for (x = 0; x < MAX_WID; x++) {
		/*  Darken "boring" features */
		if (cave_plain_floor_grid(&zcave[y][x]) && !(zcave[y][x].info & CAVE_ROOM)) { /* keep house grids */
			/* Forget the grid */
			p_ptr->cave_flag[y][x] &= ~CAVE_MARK;
		/* Always remember interesting features in town areas */
		} else if (istownarea(&p_ptr->wpos, MAX_TOWNAREA)
		    && (p_ptr->view_perma_grids)) {
			p_ptr->cave_flag[y][x] |= CAVE_MARK;
		}

		note_spot(Ind, y, x);
	}

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);
	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);
	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);


	/* Lastly, handle music */

#ifdef USE_SOUND_2010
	if (!p_ptr->is_day) return(FALSE);
	p_ptr->is_day = FALSE;
	handle_music(Ind);
	handle_ambient_sfx(Ind, &zcave[p_ptr->py][p_ptr->px], &p_ptr->wpos, TRUE);
#endif

	return(TRUE);
}

/* update a particular player's view as a town looks at night, just for dungeon towns */
void player_dungeontown(int Ind) {
	player_type *p_ptr = Players[Ind];
	cave_type **zcave = getcave(&p_ptr->wpos);
	int x, y;

	if (!zcave) return; /* paranoia */
	s_printf("DUNGEON_TOWN_MAP: %s\n", p_ptr->name);

	/* Hack -- Scan the level */
	for (y = 0; y < MAX_HGT; y++)
	for (x = 0; x < MAX_WID; x++) {
		/* Always remember interesting features in town areas */
#if 0
		if (!(cave_plain_floor_grid(&zcave[y][x]) && !(zcave[y][x].info & CAVE_ROOM))
		    && p_ptr->view_perma_grids)
#endif
		if (!(cave_plain_floor_grid(&zcave[y][x])) || (zcave[y][x].info & CAVE_ROOM))
			p_ptr->cave_flag[y][x] |= CAVE_MARK;

		note_spot(Ind, y, x);
	}

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);
	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);
	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);
}

/* turn an allocated wpos to bright day and update view of players on it */
void world_surface_day(struct worldpos *wpos) {
	cave_type **zcave = getcave(wpos), *c_ptr;
	struct dun_level *l_ptr = getfloor(wpos);
	int y, x;

	if (!zcave) return; /* paranoia */

	if (sector000separation && 0 == WPOS_SECTOR000_Z &&
	    wpos->wx == WPOS_SECTOR000_X && wpos->wy == WPOS_SECTOR000_Y) return;
	if (l_ptr && (l_ptr->flags2 & LF2_INDOORS)) return;

	/* Hack -- Scan the level */
	for (y = 0; y < MAX_HGT; y++)
	for (x = 0; x < MAX_WID; x++) {
		/* Get the cave grid */
		c_ptr = &zcave[y][x];

		/* Assume lit */
		c_ptr->info |= CAVE_GLOW;
		c_ptr->info &= ~CAVE_DARKEN;
	}
}

/* turn an allocated wpos to dark night and update view of players on it */
void world_surface_night(struct worldpos *wpos) {
	cave_type **zcave = getcave(wpos), *c_ptr;
	struct dun_level *l_ptr = getfloor(wpos);
	int y, x;
	int stores = 0, y1, x1, i;
	byte sx[255], sy[255];

	if (!zcave) return; /* paranoia */

	if (sector000separation && 0 == WPOS_SECTOR000_Z &&
	    wpos->wx == WPOS_SECTOR000_X && wpos->wy == WPOS_SECTOR000_Y) return;
	if (l_ptr && (l_ptr->flags2 & LF2_INDOORS)) return;

	/* Hack -- Scan the level */
	for (y = 0; y < MAX_HGT; y++)
	for (x = 0; x < MAX_WID; x++) {
		/* Get the cave grid */
		c_ptr = &zcave[y][x];

		/* darken all */
		if (!(f_info[c_ptr->feat].flags1 & FF1_PROTECTED) &&
		    !(c_ptr->info & CAVE_ROOM) && /* keep houses' contents lit */
		    !(f_info[c_ptr->feat].flags2 & FF2_GLOW) &&
		    !(c_ptr->info & (CAVE_GLOW_HACK | CAVE_GLOW_HACK_LAMP))) {
			c_ptr->info &= ~CAVE_GLOW;
			c_ptr->info |= CAVE_DARKEN;
		}

		if ((c_ptr->feat == FEAT_SHOP || c_ptr->feat == FEAT_SICKBAY_DOOR) && stores < 254) {
			sx[stores] = x;
			sy[stores] = y;
			stores++;
		}
	}

	/* Hack -- illuminate the stores */
	for (i = 0; i < stores; i++) {
		x = sx[i];
		y = sy[i];

		for (y1 = y - 1; y1 <= y + 1; y1++)
		for (x1 = x - 1; x1 <= x + 1; x1++) {
			/* Get the grid */
			c_ptr = &zcave[y1][x1];

			/* Illuminate the store */
			//c_ptr->info |= CAVE_ROOM | CAVE_GLOW;
			c_ptr->info |= CAVE_GLOW;
		}
	}
}

/* Day starts */
static void sun_rises() {
	struct worldpos wrpos;
	int wx, wy, i;
	player_type *p_ptr;

	night_surface = FALSE;
	wrpos.wz = 0;

	/* scan through all currently allocated world surface levels */
	for (wx = 0; wx < MAX_WILD_X; wx++)
	for (wy = 0; wy < MAX_WILD_Y; wy++) {
		wrpos.wx = wx;
		wrpos.wy = wy;
		world_surface_day(&wrpos);
	}

	/* Message all players who witness switch */
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		if (p_ptr->conn == NOT_CONNECTED) continue;
		if (player_day(i)) {
			msg_print(i, "The sun has risen.");
			if (p_ptr->prace == RACE_VAMPIRE || (p_ptr->body_monster && r_info[p_ptr->body_monster].d_char == 'V')) grid_affects_player(i, p_ptr->px, p_ptr->py); /* sunlight damage */
		}
	}
}

/* Night starts */
static void night_falls() {
	struct worldpos wrpos;
	int wx, wy, i;
	player_type *p_ptr;

	night_surface = TRUE;
	wrpos.wz = 0;

	/* scan through all currently allocated world surface levels */
	for (wx = 0; wx < MAX_WILD_X; wx++)
	for (wy = 0; wy < MAX_WILD_Y; wy++) {
		wrpos.wx = wx;
		wrpos.wy = wy;
		world_surface_night(&wrpos);
	}

	/* Message all players who witness switch */
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		if (p_ptr->conn == NOT_CONNECTED) continue;
		if (player_night(i)) {
			msg_print(i, "The sun has fallen.");
			if (p_ptr->prace == RACE_VAMPIRE || (p_ptr->body_monster && r_info[p_ptr->body_monster].d_char == 'V')) grid_affects_player(i, p_ptr->px, p_ptr->py); /* sunlight damage */
		}
	}

	/* If it's new year's eve, start the fireworks! */
	if (season_newyearseve) fireworks = 1;
}

#ifdef EXTENDED_COLOURS_PALANIM
static unsigned int colour_std[BASE_PALETTE_SIZE] = {
	0x000000,      /* BLACK */
	0xffffff,      /* WHITE */
	0x9d9d9d,      /* SLATE */
	0xff8d00,      /* ORANGE */
	0xb70000,      /* RED */
	0x009d44,      /* GREEN */
 #ifndef READABILITY_BLUE
	0x0000ff,      /* BLUE */
 #else
	0x0033ff,      /* BLUE */
 #endif
	0x8d6600,      /* UMBER */
 #ifndef DISTINCT_DARK
	0x747474,      /* LDARK */
 #else
	//0x585858,      /* LDARK */
	0x666666,      /* LDARK */
 #endif
	0xcdcdcd,      /* LWHITE */
	0xaf00ff,      /* VIOLET */
	0xffff00,      /* YELLOW */
	0xff3030,      /* LRED */
	0x00ff00,      /* LGREEN */
	0x00ffff,      /* LBLUE */
	0xc79d55,      /* LUMBER */
};

#define COLOUR_R(i) ((int)((colour_std[i] & 0xff0000) >> 16))
#define COLOUR_G(i) ((int)((colour_std[i] & 0x00ff00) >> 8))
#define COLOUR_B(i) ((int)(colour_std[i] & 0x0000ff))

#define COLOUR_DIFF_R(i,j) ((int)COLOUR_R(i) - (int)COLOUR_R(j))
#define COLOUR_DIFF_G(i,j) ((int)COLOUR_G(i) - (int)COLOUR_G(j))
#define COLOUR_DIFF_B(i,j) ((int)COLOUR_B(i) - (int)COLOUR_B(j))
#define COLOUR_STEP_R(i,j,s,n) ((((1000 * COLOUR_DIFF_R(i,j)) * (n)) / (s)) / 1000)
#define COLOUR_STEP_G(i,j,s,n) ((((1000 * COLOUR_DIFF_G(i,j)) * (n)) / (s)) / 1000)
#define COLOUR_STEP_B(i,j,s,n) ((((1000 * COLOUR_DIFF_B(i,j)) * (n)) / (s)) / 1000)
#define COLOUR_CALC_R(i,j,s,n) (COLOUR_R(i) - COLOUR_STEP_R(i,j,s,n))
#define COLOUR_CALC_G(i,j,s,n) (COLOUR_G(i) - COLOUR_STEP_G(i,j,s,n))
#define COLOUR_CALC_B(i,j,s,n) (COLOUR_B(i) - COLOUR_STEP_B(i,j,s,n))

#define COLOUR_DIFF_CR(c,i) ((int)(c) - (int)COLOUR_R(i))
#define COLOUR_DIFF_CG(c,i) ((int)(c) - (int)COLOUR_G(i))
#define COLOUR_DIFF_CB(c,i) ((int)(c) - (int)COLOUR_B(i))
#define COLOUR_STEP_CR(c,i,s,n) ((((1000 * COLOUR_DIFF_CR(c,i)) * (n)) / (s)) / 1000)
#define COLOUR_STEP_CG(c,i,s,n) ((((1000 * COLOUR_DIFF_CG(c,i)) * (n)) / (s)) / 1000)
#define COLOUR_STEP_CB(c,i,s,n) ((((1000 * COLOUR_DIFF_CB(c,i)) * (n)) / (s)) / 1000)
#define COLOUR_CALC_CR(c,i,s,n) ((int)(c) - COLOUR_STEP_CR(c,i,s,n))
#define COLOUR_CALC_CG(c,i,s,n) ((int)(c) - COLOUR_STEP_CG(c,i,s,n))
#define COLOUR_CALC_CB(c,i,s,n) ((int)(c) - COLOUR_STEP_CB(c,i,s,n))

#define COLOUR_DIFF_RC(i,c) ((int)COLOUR_R(i) - (int)(c))
#define COLOUR_DIFF_GC(i,c) ((int)COLOUR_G(i) - (int)(c))
#define COLOUR_DIFF_BC(i,c) ((int)COLOUR_B(i) - (int)(c))
#define COLOUR_STEP_RC(i,c,s,n) ((((1000 * COLOUR_DIFF_RC(i,c)) * (n)) / (s)) / 1000)
#define COLOUR_STEP_GC(i,c,s,n) ((((1000 * COLOUR_DIFF_GC(i,c)) * (n)) / (s)) / 1000)
#define COLOUR_STEP_BC(i,c,s,n) ((((1000 * COLOUR_DIFF_BC(i,c)) * (n)) / (s)) / 1000)
#define COLOUR_CALC_RC(i,c,s,n) (COLOUR_R(i) - COLOUR_STEP_RC(i,c,s,n))
#define COLOUR_CALC_GC(i,c,s,n) (COLOUR_G(i) - COLOUR_STEP_GC(i,c,s,n))
#define COLOUR_CALC_BC(i,c,s,n) (COLOUR_B(i) - COLOUR_STEP_BC(i,c,s,n))

#define COLOUR_DIFF_CRC(c,d) ((int)(c) - (int)(d))
#define COLOUR_DIFF_CGC(c,d) ((int)(c) - (int)(d))
#define COLOUR_DIFF_CBC(c,d) ((int)(c) - (int)(d))
#define COLOUR_STEP_CRC(c,d,s,n) ((((1000L * COLOUR_DIFF_CRC(c,d)) * (n)) / (s)) / 1000)
#define COLOUR_STEP_CGC(c,d,s,n) ((((1000L * COLOUR_DIFF_CGC(c,d)) * (n)) / (s)) / 1000)
#define COLOUR_STEP_CBC(c,d,s,n) ((((1000L * COLOUR_DIFF_CBC(c,d)) * (n)) / (s)) / 1000)
#define COLOUR_CALC_CRC(c,d,s,n) ((int)(c) - COLOUR_STEP_CRC(c,d,s,n))
#define COLOUR_CALC_CGC(c,d,s,n) ((int)(c) - COLOUR_STEP_CGC(c,d,s,n))
#define COLOUR_CALC_CBC(c,d,s,n) ((int)(c) - COLOUR_STEP_CBC(c,d,s,n))

/* Todo: Finish un-sunset stage 4 and commented this out to enable it */
#define STRETCHEDSUNSET /* laziness/todo */

/* --- Usage: ---
 COLOUR_CALC_...(target colour, starting colour, steps, current step)
*/

/* Helper function to determine sky state and sub-state over the day time cycle.
   'fill' will return filler values for time intervals between the actual switching times.
   Note: For consistency, 'sub' is always decreasing from n..0 during any skystate's progress.
         This ensures that we arrive at the exact target colour without rounding errors. */
static void get_world_surface_palette_state(int *sky, int *sub, int *halfhours, bool fill) {
	int t = (turn / (HOUR / PALANIM_HOUR_DIV)) % (24 * PALANIM_HOUR_DIV); //5 minute intervals, ie 1/12 hour

	/* Halloween is permanently 'midnight': */
	if (season_halloween) {
		t = 0;
		fill = TRUE;
	}

	/* Check for smooth dark/light transitions every 5 minutes (in-game time). (6h..20h is daylight) */
	/* 4:00h..5:00h - getting less dark */
	if (t >= 4 * PALANIM_HOUR_DIV && t < 5 * PALANIM_HOUR_DIV) {
		(*sky) = 0;
		(*sub) = 5 * PALANIM_HOUR_DIV - t;
		(*sub) += 3 * PALANIM_HOUR_DIV; //proactive for sky 1; starting from darkest colours (night theme)
		(*halfhours) = 2 + 6; //combo with sky 1
	}
	/* 5:00h..8:00h - DAYBREAK - darkness ceases */
	else if (t >= 5 * PALANIM_HOUR_DIV && t < 8 * PALANIM_HOUR_DIV) {
		(*sky) = 1;
		(*sub) = 8 * PALANIM_HOUR_DIV - t;
		(*halfhours) = 2 + 6; //combo with sky 0
	}
	/* 13:00h..19:00h - getting yellowish/less bright */
	else if (t >= 13 * PALANIM_HOUR_DIV && t < 18 * PALANIM_HOUR_DIV + PALANIM_HOUR_DIV / 2) {
		(*sky) = 2;
		(*sub) = 18 * PALANIM_HOUR_DIV + PALANIM_HOUR_DIV / 2 - t;
		(*halfhours) = 11;
	}
	/* 19:00h..20:00h - sunsetty (reddish), getting darkish */
	else if (t >= 18 * PALANIM_HOUR_DIV + PALANIM_HOUR_DIV / 2 && t < 20 * PALANIM_HOUR_DIV) {
		(*sky) = 3;
		(*sub) = 20 * PALANIM_HOUR_DIV - t;
		(*halfhours) = 3;
	}
#ifndef STRETCHEDSUNSET /* finish more realistic, quicker sunset, then '0 this */
	/* 20:00h..20:30h - un-sunset, returning hues to normal */
	else if (t >= 20 * PALANIM_HOUR_DIV && t < 20 * PALANIM_HOUR_DIV + PALANIM_HOUR_DIV / 2) {
		(*sky) = 4;
		(*sub) = 20 * PALANIM_HOUR_DIV + PALANIM_HOUR_DIV / 2 - t;
		(*halfhours) = 1;
	}
	/* 20:30h..21:00h - move from sunset to NIGHTFALL - getting dark */
	else if (t >= 20 * PALANIM_HOUR_DIV + PALANIM_HOUR_DIV && t <= 21 * PALANIM_HOUR_DIV) {
		(*sky) = 5;
		(*sub) = 21 * PALANIM_HOUR_DIV - t;
		(*halfhours) = 1;
	}
#else
	/* 20:00h..21:00h - move from sunset to NIGHTFALL - getting dark */
	else if (t >= 20 * PALANIM_HOUR_DIV && t <= 21 * PALANIM_HOUR_DIV) {
		(*sky) = 5;
		(*sub) = 21 * PALANIM_HOUR_DIV - t;
		(*halfhours) = 2;
	}
#endif
	/* other -- fill? */
	else if (fill) {
		if (t >= 8 * PALANIM_HOUR_DIV && t < 13 * PALANIM_HOUR_DIV) {
			(*sky) = 1;
			(*sub) = 0;
			(*halfhours) = 10;
		}
		else if (t >= 21 * PALANIM_HOUR_DIV || t < 4 * PALANIM_HOUR_DIV) {
			(*sky) = 5;
			(*sub) = 0;
			(*halfhours) = 14;
		}
	}
	/* paranoia */
	else {
		(*sky) = -1; //indicate error
		(*sub) = 0;
	}
}
/* Local helper function for actually animating the palette.
   Note: For consistency we use Send_palette(<targetcol> - <targetcol - currentcol>) */
#define PALANIM_OPTIMIZED /* KEEP SAME AS CLIENT! */
/* Shade mountains aka TERM_L_UMBER too? */
#define SHADE_MOUNTAINS
static void world_surface_palette_player_do(int i, int sky, int sub, int halfhours) {
	int steps = halfhours * PALANIM_HOUR_DIV / 2;

	switch (sky) {
	case 0: //getting less dark (PALANIM_HOUR_DIV steps) (05:00<06:00)
		//black stays black
		Send_palette(i, 17,		//white		<-slate
		    COLOUR_CALC_R(TERM_WHITE, TERM_SLATE, steps, sub),
		    COLOUR_CALC_R(TERM_WHITE, TERM_SLATE, steps, sub),
		    COLOUR_CALC_R(TERM_WHITE, TERM_SLATE, steps, sub));
		Send_palette(i, 18,		//slate		<-l-dark
		    COLOUR_CALC_R(TERM_SLATE, TERM_L_DARK, steps, sub),
		    COLOUR_CALC_G(TERM_SLATE, TERM_L_DARK, steps, sub),
		    COLOUR_CALC_B(TERM_SLATE, TERM_L_DARK, steps, sub));
		Send_palette(i, 19,		//orange		<-umber
		    COLOUR_CALC_R(TERM_ORANGE, TERM_UMBER, steps, sub),
		    COLOUR_CALC_G(TERM_ORANGE, TERM_UMBER, steps, sub),
		    COLOUR_CALC_B(TERM_ORANGE, TERM_UMBER, steps, sub));
#if 0 /* unchanged */
		Send_palette(i, 20,;		//red		<-%
		    COLOUR_R(TERM_RED), COLOUR_G(TERM_RED), COLOUR_B(TERM_RED));
		Send_palette(i, 21,		//green		<-%
		    COLOUR_R(TERM_GREEN), COLOUR_G(TERM_GREEN), COLOUR_B(TERM_GREEN));
		Send_palette(i, 22,		//blue (readable)	<-%
		    COLOUR_R(TERM_BLUE), COLOUR_G(TERM_BLUE), COLOUR_B(TERM_BLUE));
		Send_palette(i, 23,		//umber		<-%
		    COLOUR_R(TERM_UMBER), COLOUR_G(TERM_UMBER), COLOUR_B(TERM_UMBER));
		Send_palette(i, 24,		//l-dark (distinct)	<-%
		    COLOUR_R(TERM_L_DARK), COLOUR_G(TERM_L_DARK), COLOUR_B(TERM_L_DARK));
#endif
		Send_palette(i, 25,		//l-white		<-slate
		    COLOUR_CALC_R(TERM_L_WHITE, TERM_SLATE, steps, sub),
		    COLOUR_CALC_G(TERM_L_WHITE, TERM_SLATE, steps, sub),
		    COLOUR_CALC_B(TERM_L_WHITE, TERM_SLATE, steps, sub));
#if 0 /* unchanged */
		Send_palette(i, 26,		//violet		<-%
		    COLOUR_R(TERM_VIOLET), COLOUR_G(TERM_VIOLET), COLOUR_B(TERM_VIOLET));
#endif
		Send_palette(i, 27,		//yellow		<-l-umber
		    COLOUR_CALC_R(TERM_YELLOW, TERM_L_UMBER, steps, sub),
		    COLOUR_CALC_G(TERM_YELLOW, TERM_L_UMBER, steps, sub),
		    COLOUR_CALC_B(TERM_YELLOW, TERM_L_UMBER, steps, sub));
		Send_palette(i, 28,		//l-red		<-red
		    COLOUR_CALC_R(TERM_L_RED, TERM_RED, steps, sub),
		    COLOUR_CALC_G(TERM_L_RED, TERM_RED, steps, sub),
		    COLOUR_CALC_B(TERM_L_RED, TERM_RED, steps, sub));
		Send_palette(i, 29,		//l-green		<-green
		    COLOUR_CALC_R(TERM_L_GREEN, TERM_GREEN, steps, sub),
		    COLOUR_CALC_G(TERM_L_GREEN, TERM_GREEN, steps, sub),
		    COLOUR_CALC_B(TERM_L_GREEN, TERM_GREEN, steps, sub));
		Send_palette(i, 30,		//l-blue		<-blue
		    COLOUR_CALC_R(TERM_L_BLUE, TERM_BLUE, steps, sub),
		    COLOUR_CALC_G(TERM_L_BLUE, TERM_BLUE, steps, sub),
		    COLOUR_B(TERM_L_BLUE)); //BLUE and L_BLUE are both 0xff
		Send_palette(i, 31,		//l-umber		<-umber
		    COLOUR_CALC_R(TERM_L_UMBER, TERM_UMBER, steps, sub),
		    COLOUR_CALC_G(TERM_L_UMBER, TERM_UMBER, steps, sub),
		    COLOUR_CALC_B(TERM_L_UMBER, TERM_UMBER, steps, sub));
		break;
	case 1: //DAYBREAK! -- darkness ceases (this entry has all correct colours) (24 steps) (06:00-08:00)
		//continuing from sky 0, ending at brightest colours (day theme)
		//we continue where skystage 0 left off, at (1 * PALANIM_HOUR_DIV) steps
		//black stays black
		Send_palette(i, 17,		//white		<-slate
		    COLOUR_CALC_R(TERM_WHITE, TERM_SLATE, steps, sub),
		    COLOUR_CALC_G(TERM_WHITE, TERM_SLATE, steps, sub),
		    COLOUR_CALC_B(TERM_WHITE, TERM_SLATE, steps, sub));
		Send_palette(i, 18,		//slate		<-l-dark
		    COLOUR_CALC_R(TERM_SLATE, TERM_L_DARK, steps, sub),
		    COLOUR_CALC_G(TERM_SLATE, TERM_L_DARK, steps, sub),
		    COLOUR_CALC_B(TERM_SLATE, TERM_L_DARK, steps, sub));
		Send_palette(i, 19,		//orange		<-umber
		    COLOUR_CALC_R(TERM_ORANGE, TERM_UMBER, steps, sub),
		    COLOUR_CALC_G(TERM_ORANGE, TERM_UMBER, steps, sub),
		    COLOUR_CALC_B(TERM_ORANGE, TERM_UMBER, steps, sub));
#if 0 /* unchanged */
		Send_palette(i, 20,;		//red		<-%
		    COLOUR_R(TERM_RED), COLOUR_G(TERM_RED), COLOUR_B(TERM_RED));
		Send_palette(i, 21,		//green		<-%
		    COLOUR_R(TERM_GREEN), COLOUR_G(TERM_GREEN), COLOUR_B(TERM_GREEN));
		Send_palette(i, 22,		//blue (readable)	<-%
		    COLOUR_R(TERM_BLUE), COLOUR_G(TERM_BLUE), COLOUR_B(TERM_BLUE));
		Send_palette(i, 23,		//umber		<-%
		    COLOUR_R(TERM_UMBER), COLOUR_G(TERM_UMBER), COLOUR_B(TERM_UMBER));
		Send_palette(i, 24,		//l-dark (distinct)	<-%
		    COLOUR_R(TERM_L_DARK), COLOUR_G(TERM_L_DARK), COLOUR_B(TERM_L_DARK));
#endif
		Send_palette(i, 25,		//l-white		<-slate
		    COLOUR_CALC_R(TERM_L_WHITE, TERM_SLATE, steps, sub),
		    COLOUR_CALC_G(TERM_L_WHITE, TERM_SLATE, steps, sub),
		    COLOUR_CALC_B(TERM_L_WHITE, TERM_SLATE, steps, sub));
#if 0 /* unchanged */
		Send_palette(i, 26,		//violet		<-%
		    COLOUR_R(TERM_VIOLET), COLOUR_G(TERM_VIOLET), COLOUR_B(TERM_VIOLET));
#endif
		Send_palette(i, 27,		//yellow		<-l-umber
		    COLOUR_CALC_R(TERM_YELLOW, TERM_L_UMBER, steps, sub),
		    COLOUR_CALC_G(TERM_YELLOW, TERM_L_UMBER, steps, sub),
		    COLOUR_CALC_B(TERM_YELLOW, TERM_L_UMBER, steps, sub));
		Send_palette(i, 28,		//l-red		<-red
		    COLOUR_CALC_R(TERM_L_RED, TERM_RED, steps, sub),
		    COLOUR_CALC_G(TERM_L_RED, TERM_RED, steps, sub),
		    COLOUR_CALC_B(TERM_L_RED, TERM_RED, steps, sub));
		Send_palette(i, 29,		//l-green		<-green
		    COLOUR_CALC_R(TERM_L_GREEN, TERM_GREEN, steps, sub),
		    COLOUR_CALC_G(TERM_L_GREEN, TERM_GREEN, steps, sub),
		    COLOUR_CALC_B(TERM_L_GREEN, TERM_GREEN, steps, sub));
		Send_palette(i, 30,		//l-blue		<-blue
		    COLOUR_CALC_R(TERM_L_BLUE, TERM_BLUE, steps, sub),
		    COLOUR_CALC_G(TERM_L_BLUE, TERM_BLUE, steps, sub),
		    COLOUR_B(TERM_L_BLUE)); //BLUE and L_BLUE are both 0xff
		Send_palette(i, 31,		//l-umber		<-umber
		    COLOUR_CALC_R(TERM_L_UMBER, TERM_UMBER, steps, sub),
		    COLOUR_CALC_G(TERM_L_UMBER, TERM_UMBER, steps, sub),
		    COLOUR_CALC_B(TERM_L_UMBER, TERM_UMBER, steps, sub));
		break;
	case 2: //getting yellowish/less bright (72 steps) (13:00<18:30)
		//going from brightest colours (day theme) to yellowish/orangeish colours
		//black stays black
		Send_palette(i, 17,		//white
		    COLOUR_CALC_CR(0xef, TERM_WHITE, steps, sub),
		    COLOUR_CALC_CG(0xef, TERM_WHITE, steps, sub),
		    COLOUR_CALC_CB(0xa0, TERM_WHITE, steps, sub));
		Send_palette(i, 18,		//slate
		    COLOUR_CALC_CR(0x8d, TERM_SLATE, steps, sub),
		    COLOUR_CALC_CG(0x8d, TERM_SLATE, steps, sub),
		    COLOUR_CALC_CB(0x70, TERM_SLATE, steps, sub));
#if 0 /* unchanged */
		Send_palette(i, 19,		//orange
		    COLOUR_R(TERM_ORANGE), COLOUR_G(TERM_ORANGE), COLOUR_B(TERM_ORANGE));
		Send_palette(i, 20,		//red
		    COLOUR_R(TERM_RED), COLOUR_G(TERM_RED), COLOUR_B(TERM_RED));
		Send_palette(i, 21,		//green
		    COLOUR_R(TERM_GREEN), COLOUR_G(TERM_GREEN), COLOUR_B(TERM_GREEN));
		Send_palette(i, 22,		//blue (readable)
		    COLOUR_R(TERM_BLUE), COLOUR_G(TERM_BLUE), COLOUR_B(TERM_BLUE));
		Send_palette(i, 23,		//umber
		    COLOUR_R(TERM_UMBER), COLOUR_G(TERM_UMBER), COLOUR_B(TERM_UMBER));
		Send_palette(i, 24,;		//l-dark (distinct)
		    COLOUR_R(TERM_L_DARK), COLOUR_G(TERM_L_DARK), COLOUR_B(TERM_L_DARK));
#endif
		Send_palette(i, 25,		//l-white
		    COLOUR_CALC_CR(0xbf, TERM_L_WHITE, steps, sub),
		    COLOUR_CALC_CG(0xbf, TERM_L_WHITE, steps, sub),
		    COLOUR_CALC_CB(0x90, TERM_L_WHITE, steps, sub));
#if 0 /* unchanged */
		Send_palette(i, 26,		//violet
		    COLOUR_R(TERM_VIOLET), COLOUR_G(TERM_VIOLET), COLOUR_B(TERM_VIOLET));
#endif
		Send_palette(i, 27,		//yellow
		    COLOUR_R(TERM_YELLOW),
		    COLOUR_CALC_CG(0xdf, TERM_YELLOW, steps, sub),
		    COLOUR_B(TERM_YELLOW));
#if 0 /* unchanged */
		Send_palette(i, 28,		//l-red
		    COLOUR_R(TERM_L_RED), COLOUR_G(TERM_L_RED), COLOUR_B(TERM_L_RED));
		Send_palette(i, 29,		//l-green
		    COLOUR_R(TERM_L_GREEN), COLOUR_G(TERM_L_GREEN), COLOUR_B(TERM_L_GREEN));
		Send_palette(i, 30,		//l-blue
		    COLOUR_R(TERM_L_BLUE), COLOUR_G(TERM_L_BLUE), COLOUR_B(TERM_L_BLUE));
#endif
#ifdef SHADE_MOUNTAINS
		Send_palette(i, 31,		//l-umber
		    COLOUR_CALC_CR(0xe0, TERM_L_UMBER, steps, sub),
		    COLOUR_CALC_CG(0xa0, TERM_L_UMBER, steps, sub),
		    COLOUR_B(TERM_L_UMBER));
#endif
		break;
	case 3: //sunsetty and darkish (PALANIM_HOUR_DIV steps) (18:30<19:00)
		//picking up sky 2, continuing to orangish/reddish/purpleish/maybe darker colours
		//black stays black
		Send_palette(i, 17,		//white
		    0xef,
		    COLOUR_CALC_CGC(0xb0, 0xef, steps, sub),
		    COLOUR_CALC_CBC(0xa0, 0xaf, steps, sub));
		Send_palette(i, 18,		//slate
		    0x8d,
		    COLOUR_CALC_CGC(0x6f, 0x8d, steps, sub),
		    COLOUR_CALC_CBC(0x5f, 0x70, steps, sub));
#if 0 /* unchanged */
		Send_palette(i, 19,		//orange
		    COLOUR_R(TERM_ORANGE), COLOUR_G(TERM_ORANGE), COLOUR_B(TERM_ORANGE));
		Send_palette(i, 20,		//red
		    COLOUR_R(TERM_RED), COLOUR_G(TERM_RED), COLOUR_B(TERM_RED));
#endif
		Send_palette(i, 21,		//green
		    COLOUR_CALC_CR(0x2f, TERM_GREEN, steps, sub),
		    COLOUR_CALC_CG(0x7f, TERM_GREEN, steps, sub),
		    COLOUR_B(TERM_GREEN)); //maybe reduce this too? would then become smaller than TERM_GREEN base value though, need to adjust it suddenly when night time hits :/
		Send_palette(i, 22,		//blue (readable)
		    COLOUR_CALC_CR(0x3f, TERM_BLUE, steps, sub),
		    COLOUR_CALC_CG(0x2f, TERM_BLUE, steps, sub),
		    COLOUR_B(TERM_BLUE));
#if 0 /* unchanged */
		Send_palette(i, 23,		//umber
		    COLOUR_R(TERM_UMBER), COLOUR_G(TERM_UMBER), COLOUR_B(TERM_UMBER));
		Send_palette(i, 24,;		//l-dark (distinct)
		    COLOUR_R(TERM_L_DARK), COLOUR_G(TERM_L_DARK), COLOUR_B(TERM_L_DARK));
#endif
		Send_palette(i, 25,		//l-white
		    0xbf,
		    COLOUR_CALC_CGC(0x90, 0xbf, steps, sub),
		    COLOUR_CALC_CBC(0x80, 0x90, steps, sub));
#if 0 /* unchanged */
		Send_palette(i, 26,		//violet
		    COLOUR_R(TERM_VIOLET), COLOUR_G(TERM_VIOLET), COLOUR_B(TERM_VIOLET));
#endif
		Send_palette(i, 27,		//yellow
		    COLOUR_R(TERM_YELLOW),
		    COLOUR_CALC_CGC(0xbf, 0xdf, steps, sub),
		    COLOUR_B(TERM_YELLOW));
#if 0 /* unchanged */
		Send_palette(i, 28,		//l-red
		    COLOUR_R(TERM_L_RED), COLOUR_G(TERM_L_RED), COLOUR_B(TERM_L_RED));
#endif
		Send_palette(i, 29,		//l-green
		    COLOUR_CALC_CR(0x3f, TERM_L_GREEN, steps, sub),
		    COLOUR_CALC_CG(0xcf, TERM_L_GREEN, steps, sub),
		    COLOUR_CALC_CB(0x2f, TERM_L_GREEN, steps, sub));
		Send_palette(i, 30,		//l-blue
		    COLOUR_CALC_CR(0x3f, TERM_L_BLUE, steps, sub),
		    COLOUR_CALC_CG(0xaf, TERM_L_BLUE, steps, sub),
		    COLOUR_B(TERM_L_BLUE));
#ifdef SHADE_MOUNTAINS
		Send_palette(i, 31,		//l-umber
		    COLOUR_CALC_CRC(0xc0, 0xe0, steps, sub),
		    COLOUR_CALC_CGC(0x70, 0xa0, steps, sub),
		    COLOUR_B(TERM_L_UMBER));
#endif
		break;
#ifndef STRETCHEDSUNSET /* todo: enable this more realistic, unstretched sunset */
	case 4: //un-sunset (remove the reddish tones, and yellowish too actually) (19:00<19:30)
		//black stays black
		Send_palette(i, 17,		//white
		    0xef,
		    COLOUR_CALC_CGC(0xb0, 0xef, steps, sub),
		    COLOUR_CALC_CBC(0xa0, 0xaf, steps, sub));
		Send_palette(i, 18,		//slate
		    0x8d,
		    COLOUR_CALC_CGC(0x6f, 0x8d, steps, sub),
		    COLOUR_CALC_CBC(0x5f, 0x70, steps, sub));
 #if 0 /* unchanged */
		Send_palette(i, 19,		//orange
		    COLOUR_R(TERM_ORANGE), COLOUR_G(TERM_ORANGE), COLOUR_B(TERM_ORANGE));
		Send_palette(i, 20,		//red
		    COLOUR_R(TERM_RED), COLOUR_G(TERM_RED), COLOUR_B(TERM_RED));
 #endif
		Send_palette(i, 21,		//green
		    COLOUR_CALC_CR(0x2f, TERM_GREEN, steps, sub),
		    COLOUR_CALC_CG(0x7f, TERM_GREEN, steps, sub),
		    COLOUR_B(TERM_GREEN)); //maybe reduce this too? would then become smaller than TERM_GREEN base value though, need to adjust it suddenly when night time hits :/
		Send_palette(i, 22,		//blue (readable)
		    COLOUR_CALC_CR(0x3f, TERM_BLUE, steps, sub),
		    COLOUR_CALC_CG(0x2f, TERM_BLUE, steps, sub),
		    COLOUR_B(TERM_BLUE));
 #if 0 /* unchanged */
		Send_palette(i, 23,		//umber
		    COLOUR_R(TERM_UMBER), COLOUR_G(TERM_UMBER), COLOUR_B(TERM_UMBER));
		Send_palette(i, 24,;		//l-dark (distinct)
		    COLOUR_R(TERM_L_DARK), COLOUR_G(TERM_L_DARK), COLOUR_B(TERM_L_DARK));
 #endif
		Send_palette(i, 25,		//l-white
		    0xbf,
		    COLOUR_CALC_CGC(0x90, 0xbf, steps, sub),
		    COLOUR_CALC_CBC(0x80, 0x90, steps, sub));
 #if 0 /* unchanged */
		Send_palette(i, 26,		//violet
		    COLOUR_R(TERM_VIOLET), COLOUR_G(TERM_VIOLET), COLOUR_B(TERM_VIOLET));
 #endif
		Send_palette(i, 27,		//yellow
		    COLOUR_R(TERM_YELLOW),
		    COLOUR_CALC_CGC(0xbf, 0xdf, steps, sub),
		    COLOUR_B(TERM_YELLOW));
 #if 0 /* unchanged */
		Send_palette(i, 28,		//l-red
		    COLOUR_R(TERM_L_RED), COLOUR_G(TERM_L_RED), COLOUR_B(TERM_L_RED));
 #endif
		Send_palette(i, 29,		//l-green
		    COLOUR_CALC_CR(0x3f, TERM_L_GREEN, steps, sub),
		    COLOUR_CALC_CG(0xcf, TERM_L_GREEN, steps, sub),
		    COLOUR_CALC_CB(0x2f, TERM_L_GREEN, steps, sub));
		Send_palette(i, 30,		//l-blue
		    COLOUR_CALC_CR(0x3f, TERM_L_BLUE, steps, sub),
		    COLOUR_CALC_CG(0xaf, TERM_L_BLUE, steps, sub),
		    COLOUR_B(TERM_L_BLUE));
#ifdef SHADE_MOUNTAINS
		Send_palette(i, 31,		//l-umber
		    COLOUR_CALC_CRC(0xc0, 0xc0, steps, sub),
		    COLOUR_CALC_CGC(0x70, 0x70, steps, sub),
		    COLOUR_B(TERM_L_UMBER));
#endif
		break;
	case 5: //NIGHTFALL! -- getting dark (PALANIM_HOUR_DIV steps) (19:30-20:00)
		//leading sky 3 final colours to darkest colours (night theme)
		//black stays black
		Send_palette(i, 17,		//white		<-slate
		    COLOUR_CALC_RC(TERM_SLATE, 0xef, steps, sub),
		    COLOUR_CALC_GC(TERM_SLATE, 0xb0, steps, sub),
		    COLOUR_CALC_BC(TERM_SLATE, 0xa0, steps, sub));
		Send_palette(i, 18,		//slate		<-l-dark
		    COLOUR_CALC_RC(TERM_L_DARK, 0x8d, steps, sub),
		    COLOUR_CALC_GC(TERM_L_DARK, 0x6f, steps, sub),
		    COLOUR_CALC_BC(TERM_L_DARK, 0x5f, steps, sub));
		Send_palette(i, 19,		//orange		<-umber
		    COLOUR_CALC_R(TERM_UMBER, TERM_ORANGE, steps, sub),
		    COLOUR_CALC_G(TERM_UMBER, TERM_ORANGE, steps, sub),
		    COLOUR_CALC_B(TERM_UMBER, TERM_ORANGE, steps, sub));
		Send_palette(i, 20,		//red		<-%
		    COLOUR_R(TERM_RED), COLOUR_G(TERM_RED), COLOUR_B(TERM_RED));
		Send_palette(i, 21,		//green		<-%
		    COLOUR_CALC_RC(TERM_GREEN, 0x2f, steps, sub),
		    COLOUR_CALC_GC(TERM_GREEN, 0x7f, steps, sub),
		    COLOUR_B(TERM_GREEN));
		Send_palette(i, 22,		//blue (readable)	<-%
		    COLOUR_CALC_RC(TERM_BLUE, 0x3f, steps, sub),
		    COLOUR_CALC_GC(TERM_BLUE, 0x2f, steps, sub),
		    COLOUR_B(TERM_BLUE));
 #if 0 /* unchanged */
		Send_palette(i, 23,		//umber		<-%
		    COLOUR_R(TERM_UMBER), COLOUR_G(TERM_UMBER), COLOUR_B(TERM_UMBER));
		Send_palette(i, 24,		//l-dark (distinct)	<-%
		    COLOUR_R(TERM_L_DARK), COLOUR_G(TERM_L_DARK), COLOUR_B(TERM_L_DARK));
 #endif
		Send_palette(i, 25,		//l-white		<-slate
		    COLOUR_CALC_RC(TERM_SLATE, 0xbf, steps, sub),
		    COLOUR_CALC_GC(TERM_SLATE, 0x90, steps, sub),
		    COLOUR_CALC_BC(TERM_SLATE, 0x80, steps, sub));
 #if 0 /* unchanged */
		Send_palette(i, 26,		//violet		<-%
		    COLOUR_R(TERM_VIOLET), COLOUR_G(TERM_VIOLET), COLOUR_B(TERM_VIOLET));
 #endif
		Send_palette(i, 27,		//yellow		<-l-umber
		    COLOUR_CALC_R(TERM_L_UMBER, TERM_YELLOW, steps, sub),
		    COLOUR_CALC_GC(TERM_L_UMBER, 0xbf, steps, sub),
		    COLOUR_CALC_B(TERM_L_UMBER, TERM_YELLOW, steps, sub));
		Send_palette(i, 28,		//l-red		<-red
		    COLOUR_CALC_R(TERM_RED, TERM_L_RED, steps, sub),
		    COLOUR_CALC_G(TERM_RED, TERM_L_RED, steps, sub),
		    COLOUR_CALC_B(TERM_RED, TERM_L_RED, steps, sub));
		Send_palette(i, 29,		//l-green		<-green
		    COLOUR_CALC_RC(TERM_GREEN, 0x3f, steps, sub),
		    COLOUR_CALC_GC(TERM_GREEN, 0xcf, steps, sub),
		    COLOUR_CALC_BC(TERM_GREEN, 0x2f, steps, sub));
		Send_palette(i, 30,		//l-blue		<-blue
		    COLOUR_CALC_RC(TERM_BLUE, 0x3f, steps, sub),
		    COLOUR_CALC_GC(TERM_BLUE, 0xaf, steps, sub),
		    COLOUR_B(TERM_BLUE)); //BLUE and L_BLUE are both 0xff
		Send_palette(i, 31,		//l-umber		<-umber
#ifdef SHADE_MOUNTAINS
		    COLOUR_CALC_RC(TERM_UMBER, 0xc0, steps, sub),
		    COLOUR_CALC_GC(TERM_UMBER, 0x70, steps, sub),
#else
		    COLOUR_CALC_R(TERM_UMBER, TERM_L_UMBER, steps, sub),
		    COLOUR_CALC_G(TERM_UMBER, TERM_L_UMBER, steps, sub),
#endif
		    COLOUR_CALC_B(TERM_UMBER, TERM_L_UMBER, steps, sub));
		break;
#else
	case 5: //NIGHTFALL! -- getting dark (PALANIM_HOUR_DIV steps) (19:00-20:00)
		//leading sky 3 final colours to darkest colours (night theme)
		//black stays black
		Send_palette(i, 17,		//white		<-slate
		    COLOUR_CALC_RC(TERM_SLATE, 0xef, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_GC(TERM_SLATE, 0xb0, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_BC(TERM_SLATE, 0xa0, PALANIM_HOUR_DIV, sub));
		Send_palette(i, 18,		//slate		<-l-dark
		    COLOUR_CALC_RC(TERM_L_DARK, 0x8d, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_GC(TERM_L_DARK, 0x6f, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_BC(TERM_L_DARK, 0x5f, PALANIM_HOUR_DIV, sub));
		Send_palette(i, 19,		//orange		<-umber
		    COLOUR_CALC_R(TERM_UMBER, TERM_ORANGE, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_G(TERM_UMBER, TERM_ORANGE, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_B(TERM_UMBER, TERM_ORANGE, PALANIM_HOUR_DIV, sub));
		Send_palette(i, 20,		//red		<-%
		    COLOUR_R(TERM_RED), COLOUR_G(TERM_RED), COLOUR_B(TERM_RED));
		Send_palette(i, 21,		//green		<-%
		    COLOUR_CALC_RC(TERM_GREEN, 0x2f, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_GC(TERM_GREEN, 0x7f, PALANIM_HOUR_DIV, sub),
		    COLOUR_B(TERM_GREEN));
		Send_palette(i, 22,		//blue (readable)	<-%
		    COLOUR_CALC_RC(TERM_BLUE, 0x3f, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_GC(TERM_BLUE, 0x2f, PALANIM_HOUR_DIV, sub),
		    COLOUR_B(TERM_BLUE));
 #if 0 /* unchanged */
		Send_palette(i, 23,		//umber		<-%
		    COLOUR_R(TERM_UMBER), COLOUR_G(TERM_UMBER), COLOUR_B(TERM_UMBER));
		Send_palette(i, 24,		//l-dark (distinct)	<-%
		    COLOUR_R(TERM_L_DARK), COLOUR_G(TERM_L_DARK), COLOUR_B(TERM_L_DARK));
 #endif
		Send_palette(i, 25,		//l-white		<-slate
		    COLOUR_CALC_RC(TERM_SLATE, 0xbf, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_GC(TERM_SLATE, 0x90, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_BC(TERM_SLATE, 0x80, PALANIM_HOUR_DIV, sub));
 #if 0 /* unchanged */
		Send_palette(i, 26,		//violet		<-%
		    COLOUR_R(TERM_VIOLET), COLOUR_G(TERM_VIOLET), COLOUR_B(TERM_VIOLET));
 #endif
		Send_palette(i, 27,		//yellow		<-l-umber
		    COLOUR_CALC_R(TERM_L_UMBER, TERM_YELLOW, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_GC(TERM_L_UMBER, 0xbf, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_B(TERM_L_UMBER, TERM_YELLOW, PALANIM_HOUR_DIV, sub));
		Send_palette(i, 28,		//l-red		<-red
		    COLOUR_CALC_R(TERM_RED, TERM_L_RED, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_G(TERM_RED, TERM_L_RED, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_B(TERM_RED, TERM_L_RED, PALANIM_HOUR_DIV, sub));
		Send_palette(i, 29,		//l-green		<-green
		    COLOUR_CALC_RC(TERM_GREEN, 0x3f, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_GC(TERM_GREEN, 0xcf, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_BC(TERM_GREEN, 0x2f, PALANIM_HOUR_DIV, sub));
		Send_palette(i, 30,		//l-blue		<-blue
		    COLOUR_CALC_RC(TERM_BLUE, 0x3f, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_GC(TERM_BLUE, 0xaf, PALANIM_HOUR_DIV, sub),
		    COLOUR_B(TERM_BLUE)); //BLUE and L_BLUE are both 0xff
		Send_palette(i, 31,		//l-umber		<-umber
#ifdef SHADE_MOUNTAINS
		    COLOUR_CALC_RC(TERM_UMBER, 0xc0, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_GC(TERM_UMBER, 0x70, PALANIM_HOUR_DIV, sub),
#else
		    COLOUR_CALC_R(TERM_UMBER, TERM_L_UMBER, PALANIM_HOUR_DIV, sub),
		    COLOUR_CALC_G(TERM_UMBER, TERM_L_UMBER, PALANIM_HOUR_DIV, sub),
#endif
		    COLOUR_CALC_B(TERM_UMBER, TERM_L_UMBER, PALANIM_HOUR_DIV, sub));
		break;
#endif
	}
#ifdef PALANIM_OPTIMIZED
	/* Send 'end of palette data' refresh marker */
	Send_palette(i, 127, 0, 0, 0);
#endif
}
/* Helper function to check whether a player is affected by palette animation in his current location. */
bool palette_affects(int Ind) {
	player_type *p_ptr = Players[Ind];

	if (is_older_than(&p_ptr->version, 4, 7, 1, 2, 0, 0)
	    || !p_ptr->palette_animation
	    || (p_ptr->global_event_temp & PEVF_INDOORS_00))
		return(FALSE);

	if (outdoor_affects(&p_ptr->wpos)) return(TRUE);

	/* Not affected */
	return(FALSE);
}
/* Re-colour the world palette depending on sky-state (time intervals of specific colour-transshading purpose)
   and the sky-state's substate (linearly interpolated 5 minute intervals within each sky-state interval) - C. Blue */
static void world_surface_palette(void) {
	int i, sky, sub, halfhours;

	get_world_surface_palette_state(&sky, &sub, &halfhours, FALSE);
	if (sky == -1) return; /* right now is not a time to change any colour */

	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (!palette_affects(i)) continue;
		world_surface_palette_player_do(i, sky, sub, halfhours);
	}
}
/* Update world palette for one player (i means Ind). */
void world_surface_palette_player(int Ind) {
	int sky, sub, halfhours;

	if (!palette_affects(Ind)) return;
	get_world_surface_palette_state(&sky, &sub, &halfhours, TRUE);
	world_surface_palette_player_do(Ind, sky, sub, halfhours);
}
/* Debugging */
void set_pal_debug(int Ind, int k) {
	msg_format(Ind, "spd: %d (%d - %d)", COLOUR_DIFF_BC(TERM_GREEN, 0xaf), COLOUR_B(TERM_GREEN), 0xaf);
	for (k = 0; k <= PALANIM_HOUR_DIV; k++) {
		msg_format(Ind, "spd: %d - %d (%d)", COLOUR_B(TERM_GREEN), COLOUR_STEP_BC(TERM_GREEN, 0xaf, PALANIM_HOUR_DIV, k), k);
		msg_format(Ind, " = %d", COLOUR_CALC_BC(TERM_GREEN, 0xaf, PALANIM_HOUR_DIV, k));
	}
	return;

	msg_format(Ind, "called set_pal_debug(%d, %d) -> CR: %d - %d = %d, CSR: %d", Ind, k, COLOUR_R(TERM_WHITE), COLOUR_R(TERM_SLATE), COLOUR_DIFF_R(TERM_WHITE, TERM_SLATE), COLOUR_STEP_R(TERM_WHITE, TERM_SLATE, 3 * PALANIM_HOUR_DIV, k));
	Send_palette(Ind, 17,//white
	    COLOUR_CALC_R(TERM_WHITE, TERM_SLATE, 3 * PALANIM_HOUR_DIV, k),
	    COLOUR_CALC_G(TERM_WHITE, TERM_SLATE, 3 * PALANIM_HOUR_DIV, k),
	    COLOUR_CALC_B(TERM_WHITE, TERM_SLATE, 3 * PALANIM_HOUR_DIV, k));
}
#endif

/* take care of day/night changes, on world surface.
   NOTE: assumes that it gets called every HOUR turns only! */
static void process_day_and_night() {
	bool sunrise, nightfall;

	/* Check for sunrise or nightfall */
	sunrise = (((turn / HOUR) % 24) == SUNRISE) && IS_DAY; /* IS_DAY checks for events, that's why it's here. - C. Blue */
	nightfall = (((turn / HOUR) % 24) == NIGHTFALL) && IS_NIGHT; /* IS_NIGHT is pointless at the time of coding this, just for consistencies sake with IS_DAY above. */

#ifdef EXTENDED_COLOURS_PALANIM /* We're called every (HOUR / PALANIM_HOUR_DIV) instead of just every hour? */
	if (!(turn % HOUR)) {
#endif
		/* Day breaks - not during Halloween {>_>} or during NEW_YEARS_EVE (fireworks)! -- covered by IS_DAY now. */
		if (sunrise)
			sun_rises();
		/* Night falls - but only if it was actually day so far:
		   During HALLOWEEN as well as NEW_YEARS_EVE it stays night all the time >:) (see above) */
		else if (nightfall && !night_surface)
			night_falls();
#ifdef EXTENDED_COLOURS_PALANIM
	}

	/* Called every (HOUR / PALANIM_HOUR_DIV) [5 min in-game time] */
	world_surface_palette();
#endif
}

/* Called when the server starts up */
static void init_day_and_night() {
	if (IS_DAY)
		sun_rises();
	else /* assume IS_NIGHT ;) */
		night_falls();
}
/* Called if time is manipulated by a god - via /settime, use with utmost care (mess up logs/stats etc), on TEST_SERVER only! */
void verify_day_and_night() {
	if (IS_DAY && night_surface) sun_rises();
	else if (IS_NIGHT && !night_surface) night_falls();
}

/*
 * Handle certain things globally in regards to the player, called every turn:
 * Monster respawn, black breath, flashbacks, ghost-fading.
 */

static void process_world_player(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i;
	//int regen_amount, NumPlayers_old = NumPlayers;


	/*** Process the monsters ***/
	/* Note : since monsters are added at a constant rate in real time,
	 * this corresponds in game time to them appearing at faster rates
	 * deeper in the dungeon.
	 */

	/* Check for creature generation */
	if (!(turn % ((cfg.fps * 5) / 6)) &&
	    ((!istown(&p_ptr->wpos) && (rand_int(MAX_M_ALLOC_CHANCE) == 0)) ||
	    (!season_halloween && istown(&p_ptr->wpos) && (rand_int(TOWNIE_RESPAWN_CHANCE) == 0)) ||
	    (season_halloween && istown(&p_ptr->wpos) &&
	    (rand_int(in_bree(&p_ptr->wpos) ? HALLOWEEN_TOWNIE_RESPAWN_CHANCE : TOWNIE_RESPAWN_CHANCE) == 0)))
	    /* avoid admins spawning stuff */
	    && !(p_ptr->admin_dm || p_ptr->admin_wiz)) {
		dun_level *l_ptr = getfloor(&p_ptr->wpos);

		/* Should we disallow those with MULTIPLY flags to spawn on surface? */
		if (!l_ptr || !(l_ptr->flags1 & LF1_NO_NEW_MONSTER)) {
			/* Set the monster generation depth */
			monster_level = getlevel(&p_ptr->wpos);

			if (p_ptr->wpos.wz) (void)alloc_monster(&p_ptr->wpos, MAX_SIGHT + 5, FALSE);
			else wild_add_monster(&p_ptr->wpos);
		}
	}

	/* Every 1500 turns, warn about any Black Breath not gotten from an equipped
	 * object, and stop any resting. -LM-
	 */
	/* Probably better done in process_player_end?	- Jir - */
	if (!(turn % (cfg.fps * 50)) && p_ptr->black_breath) {
		msg_print(Ind, "\377WThe Black Breath saps your soul!");

		/* alert to the neighbors also */
		msg_format_near(Ind, "\377WA dark aura seems to surround %s!", p_ptr->name);

		disturb(Ind, 0, 0);
	}

	/* Cold Turkey  */
	i = (p_ptr->csane << 7) / p_ptr->msane;
	if (!(turn % (cfg.fps * 70)) && !magik(i)) {
		msg_print(Ind, "\377rA flashback storms your head!");

		/* alert to the neighbors also */
		if (magik(20)) {
			switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
			case 's': case 'x': case 'z':
				msg_format_near(Ind, "You see %s' eyes bloodshot.", p_ptr->name);
				break;
			default:
				msg_format_near(Ind, "You see %s's eyes bloodshot.", p_ptr->name);
			}
		}

		set_image(Ind, p_ptr->image + 12 - i / 8);
		disturb(Ind, 0, 0);
	}

#ifdef GHOST_FADING
	if (p_ptr->ghost && !p_ptr->admin_dm &&
	    //!(turn % GHOST_FADING))
	    //!(turn % ((5100L - p_ptr->lev * 50) * GHOST_FADING)))
	    !(turn % ((GHOST_FADING * 50) / p_ptr->lev)))
		//(rand_int(GHOST_FADING) < p_ptr->lev * p_ptr->lev))
		take_xp_hit(Ind, 1 + p_ptr->lev / 5 + p_ptr->max_exp / 10000L, "fading", TRUE, TRUE, FALSE, 0);
#endif	// GHOST_FADING

}

/*
 * Quick hack to allow mimics to retaliate with innate powers	- Jir -
 * It's high time we redesign auto-retaliator	XXX
 */
static int retaliate_mimic_power(int Ind, int choice) {
	player_type *p_ptr = Players[Ind];
	int i, k, num = 3;

	/* Check for "okay" spells */
	for (k = 0; k <= 3; k++) {
		for (i = 0; i < 32; i++) {
			/* Look for "okay" spells */
			if (p_ptr->innate_spells[k] & (1U << i)) {
				if (num == choice) return(k * 32 + i);
				num++;
			}
		}
	}

	return(-1); //power not available
}

/*
 * Handle items for auto-retaliation  - Jir -
 * use_old_target is *strongly* recommended to actually make use of it.
 * If fallback is TRUE the melee weapon will be used if the intended means failed. - C. Blue
 */
static bool retaliate_item(int Ind, int item, cptr inscription, bool fallback) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	int res, choice = 0, spell = 0;
#ifdef MSTAFF_MDEV_COMBO
	int tv, item_x;
#endif

	if (item < 0) return(FALSE);
	o_ptr = &p_ptr->inventory[item];
	if (!o_ptr->k_idx) return(FALSE);

	/* 'Do nothing' inscription */
	if (inscription != NULL && *inscription == 'x') return(TRUE);

	/* Is it variant @Ot for town-only auto-retaliation? - C. Blue */
	if (*inscription == 't') {
		if (!istownarea(&p_ptr->wpos, MAX_TOWNAREA)) return(FALSE);
		inscription++;
	}

#ifndef AUTO_RET_CMD
	/* Hack -- use innate power via inscription on any item */
	if (inscription != NULL && *inscription == 'M' && get_skill(p_ptr, SKILL_MIMIC)) {
		/* Spell to cast */
		if (*(inscription + 1)) {
			choice = *(inscription + 1) - 97;

			/* valid inscription? */
			if (choice >= 0) {
				/* shape-changing for retaliation is not so nice idea, eh? */
				if (choice < 4) {	/* 3 polymorph powers + immunity preference */
 #if 1
					return(FALSE);
 #else
					/* hack: prevent 'polymorph into...' power */
					if (choice == 2) do_cmd_mimic(Ind, 1, 5);
					else do_cmd_mimic(Ind, choice, 5);
					return(TRUE);
 #endif
				} else {
					int power = retaliate_mimic_power(Ind, choice - 1); /* immunity preference */
					bool dir = FALSE;

					if (power == -1) return(!fallback || p_ptr->fail_no_melee); //tried to use unavailable power

					if (innate_powers[power].smana > p_ptr->cmp && fallback) return(p_ptr->fail_no_melee);
 #if 0
					if (power) {
						/* undirected power? */
						switch (power) {
						case 0:
						case 64: case 66:
						case 68: case 69:
						case 76:
							do_cmd_mimic(Ind, power + 3, 0);
							return(TRUE);
						}
						/* power requires direction? */
						do_cmd_mimic(Ind, power + 3, 5);
						return(TRUE);
					}
 #else
					switch (power / 32) {
					case 0: dir = monster_spells4[power].uses_dir;
						break;
					case 1: dir = monster_spells5[power - 32].uses_dir;
						break;
					case 2: dir = monster_spells6[power - 64].uses_dir;
						break;
					case 3: dir = monster_spells0[power - 96].uses_dir;
						break;
					}
					do_cmd_mimic(Ind, power + 3, dir ? 5 : 0);
					return(TRUE);
 #endif
				}
			}
		}
		return(FALSE);
	}
#endif

#ifndef AUTO_RET_NEW
	/* Only fighter classes can use various items for this */
	if (is_fighter(p_ptr)) {
 #if 0
		/* item with {@O-} is used only when in danger */
		if (*inscription == '-' && p_ptr->chp > p_ptr->mhp / 2) return(FALSE);
 #endif

		switch (o_ptr->tval) {
		/* non-directional ones */
		case TV_SCROLL:
			do_cmd_read_scroll(Ind, item);
			return(TRUE);
		case TV_POTION:
			do_cmd_quaff_potion(Ind, item);
			return(TRUE);
		case TV_STAFF:
			do_cmd_use_staff(Ind, item);
			return(TRUE);
		case TV_ROD:
			do_cmd_zap_rod(Ind, item, 5);
			return(TRUE);
		case TV_WAND:
			do_cmd_aim_wand(Ind, item, 5);
			return(TRUE);
 #ifdef MSTAFF_MDEV_COMBO
		case TV_MSTAFF:
			if (o_ptr->xtra1) {
				do_cmd_use_staff(Ind, item + 10000);
				return(TRUE);
			} else if (o_ptr->xtra2) {
				do_cmd_aim_wand(Ind, item + 10000, 5);
				return(TRUE);
			} else if (o_ptr->xtra3) {
				do_cmd_zap_rod(Ind, item + 10000, 5);
				return(TRUE);
			}
 #endif
		}
	}
#else
 #ifdef MSTAFF_MDEV_COMBO
	item_x = item;
	if (o_ptr->tval == TV_MSTAFF) {
		if (o_ptr->xtra1) {
			tv = TV_STAFF;
			item_x = item + 10000;
		} else if (o_ptr->xtra2) {
			tv = TV_WAND;
			item_x = item + 10000;
		} else if (o_ptr->xtra3) {
			tv = TV_ROD;
			item_x = item + 10000;
		} else tv = o_ptr->tval;
	} else tv = o_ptr->tval;
	switch (tv) {
 #else
	switch (o_ptr->tval) {
 #endif
	case TV_STAFF:
		/* Check if we're out of charges, to handle 'fallback' to melee  --
		   in any case suppress out-of-charges message (which would be displayed if do_cmd_use_staff() gets called) */
		if ((o_ptr->ident & ID_EMPTY) || ((o_ptr->ident & ID_KNOWN) && o_ptr->pval == 0)) {
			if (!fallback || p_ptr->fail_no_melee) {
 #ifndef AUTORET_FAIL_FREE
				p_ptr->energy -= level_speed(&p_ptr->wpos);
 #endif
				return(TRUE); //just out of charges, but no fallback
			}
			return(FALSE); //fallback to melee
		}
 #ifdef MSTAFF_MDEV_COMBO
		do_cmd_use_staff(Ind, item_x);
 #else
		do_cmd_use_staff(Ind, item);
 #endif
		return(TRUE);
	case TV_WAND:
		/* Check if we're out of charges, to handle 'fallback' to melee  --
		   in any case suppress out-of-charges message (which would be displayed if do_cmd_use_wand() gets called) */
		if ((o_ptr->ident & ID_EMPTY) || ((o_ptr->ident & ID_KNOWN) && o_ptr->pval == 0)) {
			if (!fallback || p_ptr->fail_no_melee) {
 #ifndef AUTORET_FAIL_FREE
				p_ptr->energy -= level_speed(&p_ptr->wpos);
 #endif
				return(TRUE); //just out of energy, but no fallback
			}
			return(FALSE); //fallback to melee
		}
 #ifdef MSTAFF_MDEV_COMBO
		do_cmd_aim_wand(Ind, item_x, 5);
 #else
		do_cmd_aim_wand(Ind, item, 5);
 #endif
		return(TRUE);
	case TV_ROD:
		/* Check if we're out of energy, to handle 'fallback' to melee  --
		   in any case suppress out-of-energy message (which would be displayed if do_cmd_zap_rod() gets called) */
		if (o_ptr->pval != 0) {
			if (!fallback || p_ptr->fail_no_melee) {
 #ifndef AUTORET_FAIL_FREE
				p_ptr->energy -= level_speed(&p_ptr->wpos);
 #endif
				return(TRUE); //just out of energy, but no fallback
			}
			return(FALSE); //fallback to melee
		}
 #ifdef MSTAFF_MDEV_COMBO
		do_cmd_zap_rod(Ind, item_x, 5);
 #else
		do_cmd_zap_rod(Ind, item, 5);
 #endif
		return(TRUE);
	}
#endif

	/* Accept reasonable targets:
	 * This prevents a player from getting stuck when facing a
	 * monster inside a wall.
	 * NOTE: The above statement becomes obsolete nowadays if
	 * PY_PROJ_ and similar are defined.
	 */
	if (!target_able(Ind, p_ptr->target_who)) return(FALSE);

	/* Spell to cast */
	if (inscription != NULL) {
		choice = *inscription - 96 - 1;
		if (choice < 0 || choice > 15) choice = 0; //allow a-o (15 spells, currently biggest tome has 13)
	}

	switch (o_ptr->tval) {
	/* weapon -- attack normally! */
	case TV_MSTAFF:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_AXE:
		//redundant?->	if (item == INVEN_WIELD) return(FALSE);
		return(FALSE);
		break;

	/* directional ones */
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
		if (item == INVEN_BOW || item == INVEN_AMMO) {
			if (!p_ptr->inventory[INVEN_AMMO].k_idx ||
			    !p_ptr->inventory[INVEN_AMMO].number)
				break;

			retaliating_cmd = TRUE;
			do_cmd_fire(Ind, 5);
			if (p_ptr->ranged_double) do_cmd_fire(Ind, 5);
			return(TRUE);
		}
		break;

	//case TV_INSTRUMENT:
	case TV_BOOMERANG:
		if (item == INVEN_BOW) {
			retaliating_cmd = TRUE;
			do_cmd_fire(Ind, 5);
			return(TRUE);
		}
		break;

	/* spellbooks - mikaelh */
	case TV_BOOK:
		if (o_ptr->sval == SV_SPELLBOOK) {
			/* It's a spellbook */

			/* There's only one spell in a spellbook */
			spell = o_ptr->pval;
		} else {
			/* It's a tome */

			/* Get the spell */
			if (MY_VERSION < (4 << 12 | 4 << 8 | 1U << 4 | 8)) {
				/* no longer supported! to make s_aux.lua slimmer */
				spell = exec_lua(Ind, format("return spell_x(%d, %d, %d)", o_ptr->sval, o_ptr->pval, choice));
			} else {
				spell = exec_lua(Ind, format("return spell_x2(%d, %d, %d, %d)", item, o_ptr->sval, o_ptr->pval, choice));
			}
		}

		/* Book doesn't contain a spell in the selected slot */
		if (spell == -1) break;

		/* Check if we're out of mana (or other problem), to handle 'fallback' to melee  --
		   in any case suppress OoM message (which would be displayed if do_cmd_mimic() gets called) */
		res = exec_lua(Ind, format("return test_school_spell(%d, %d, %d)", Ind, spell, item));
		if (res) {
			if (!fallback || p_ptr->fail_no_melee) {
#ifndef AUTORET_FAIL_FREE
				p_ptr->energy -= level_speed(&p_ptr->wpos) / 3;
#endif
				return(TRUE); //just out of mana (or other valid problem), but no fallback
			}
			return(FALSE); //fallback to melee
		}
		/* just let cast_school_spell() handle all failures, because it also deducts correct energy for each result case */
		cast_school_spell(Ind, item, spell, 5, -1, 0);
		return(TRUE);
	}

	/* If all fails, then melee */
	return(p_ptr->fail_no_melee);
}

#ifdef AUTO_RET_CMD
/* Check auto-retaliation set via slash command /autoretX or /arX.
   This was added for mimics to use their powers without having to inscribe
   a random item as a workaround, but is also used for runespells now.
   It returns FALSE if we want to attempt to auto-ret but can't because conditions
   aren't met, allowing us to instead try and melee-auto-ret.
   It returns TRUE if we want to auto-ret and also attempted it (no matter the
   result), so we cannot additionally perform melee-auto-ret. - C. Blue
   Also extended for runecraft, since having physical runes in the inventory
   isn't needed to actually cast runespells. */
static bool retaliate_cmd(int Ind, bool fallback) {
	player_type *p_ptr = Players[Ind];
	u16b ar = p_ptr->autoret_mu;

	/* no autoret set? */
	if (!ar) return(FALSE);

	/* Was a mimic power set for auto-ret? */
	if (!(ar & 0x8000) && p_ptr->s_info[SKILL_MIMIC].value) {
		/* Spell to cast */
		int choice, power;
		bool dir = FALSE;

		/* Accept reasonable targets:
		 * This prevents a player from getting stuck when facing a
		 * monster inside a wall.
		 * NOTE: The above statement becomes obsolete nowadays if
		 * PY_PROJ_ and similar are defined.
		 */
		if (!target_able(Ind, p_ptr->target_who)) return(FALSE);

		/* Is it variant @Ot for town-only auto-retaliation? */
		if ((ar & 0x4000) && !istownarea(&p_ptr->wpos, MAX_TOWNAREA)) return(FALSE);

		//if (ar & 0x2000) nosleep = TRUE;

		/* Fallback to melee if oom? */
		if (ar & 0x1000) fallback = TRUE;

		/* Extract mimic spell */
		ar &= ~(0x1000 | 0x2000 | 0x4000);
		choice = ar - 1l;

		/* Check for generally valid mimic power */
		if (choice < 4) return(FALSE); /* 3 polymorph powers + immunity preference */

		/* Check if we can use the selected mimic power under the current circumstances */
		power = retaliate_mimic_power(Ind, choice - 1);
		/* Check if castable. Any problem is a valid reason to keep 'fallback' option now, not just if we're out of mana (1) --
		   in any case suppress OoM message (which would be displayed if do_cmd_mimic()->do_mimic_power() gets called) */
		if (power == -1 ||
		    p_ptr->confused ||
		    innate_powers[power].smana > p_ptr->cmp) {
			if (!fallback || p_ptr->fail_no_melee) {
 #ifndef AUTORET_FAIL_FREE
				p_ptr->energy -= level_speed(&p_ptr->wpos) / 3;
 #endif
				return(TRUE); //just out of mana (or other valid problem), but no fallback
			}
			return(FALSE); //fallback to melee
		}

		/* We have a valid attempt */
		switch (power / 32) {
		case 0: dir = monster_spells4[power].uses_dir;
			break;
		case 1: dir = monster_spells5[power - 32].uses_dir;
			break;
		case 2: dir = monster_spells6[power - 64].uses_dir;
			break;
		case 3: dir = monster_spells0[power - 96].uses_dir;
			break;
		}
		do_cmd_mimic(Ind, power + 3, dir ? 5 : 0);
		return(TRUE);
	}

	/* Was a rune set for auto-ret? */
	else if ((ar & 0x8000)) {
		u32b u = 0x0;

		/* Wall safety? */
		if (!target_able(Ind, p_ptr->target_who)) return(FALSE);

		/* Is it variant @Ot for town-only auto-retaliation? */
		if ((ar & 0x4000) && !istownarea(&p_ptr->wpos, MAX_TOWNAREA)) return(FALSE);

		//if (ar & 0x2000) nosleep = TRUE;

		/* Fallback to melee if oom? */
		if (ar & 0x1000) fallback = TRUE;

		/* Decompress runespell... - Kurzel */
		ar &= ~(0x8000 | 0x4000 | 0x2000 | 0x1000);

		u |= (1 << (ar & 0x0007));                // Rune 1 (3-bit) to byte
		u |= ((1 << ((ar & 0x0038) >> 3)) <<  8); // Rune 2 (3-bit) to byte
		u |= ((1 << ((ar & 0x01C0) >> 6)) << 16); // Mode   (3-bit) to byte
		u |= ((1 << ((ar & 0x0E00) >> 9)) << 24); // Type   (3-bit) to byte

		/* Check if castable. Any problem is a valid reason to keep 'fallback' option now, not just if we're out of mana (1) --
		   in any case suppress OoM message (which would be displayed if cast_rune_spell() gets called) */
		if (exec_lua(Ind, format("return rcraft_arr_test(%d, %d)", Ind, u))) {
			if (!fallback || p_ptr->fail_no_melee) {
 #ifndef AUTORET_FAIL_FREE
				p_ptr->energy -= level_speed(&p_ptr->wpos) / 3;
 #endif
				return(TRUE); //just out of mana (or other valid problem), but no fallback
			}
			return(FALSE); //fallback to melee
		}
		/* Try to cast it, deduct proper energy in any of the different outcome cases */
		if (cast_rune_spell(Ind, (u16b)(u & 0xFFFF), (u16b)((u & 0xFFFF0000) >> 16), 5)) return(TRUE); //success
		return(TRUE); //failure
	}

	/* Neither /arm nor /arr was set, aka no 'command-retaliation' (note: /ar doesn't count for this) */
	else return(FALSE);

	/* If all fails, then melee */
	return(p_ptr->fail_no_melee);
}
#endif

/*
 * Check for nearby players or monsters and attempt to do something useful.
 *
 * This function should only be called if the player is "lagging" and helpless
 * to do anything about the situation.  This is not intended to be incredibly
 * useful, merely to prevent deaths due to extreme lag.
 */
 /* This function returns a 0 if no attack has been performed, a 1 if an attack
  * has been performed and there are still monsters around, and a 2 if an attack
  * has been performed and all of the surrounding monsters are dead.
  * (This difference between 1 and 2 isn't used anywhere... but I leave it for
  *  future use.	Jir)
  */
  /* We now intelligently try to decide which monster to autoattack.  Our current
   * algorithm is to fight first Q's, then any monster that is 20 levels higher
   * than its peers, then the most proportionatly wounded monster, then the highest
   * level monster, then the monster with the least hit points.
	 */
/* Now a player can choose the way of auto-retaliating;		- Jir -
 * Item marked with inscription {@O} will be used automatically.
 * For spellbooks this should be {@Oa} to specify which spell to use.
 * You can also choose to 'do nothing' by inscribing it on not-suitable item.
 *
 * Fighters are allowed to use {@O-}, which is only used if his HP is 1/2 or less.
 * ({@O-} feature is commented out)
 */

/* handle RF7_NO_TARGET monsters so they won't block auto-retaliation?
   This involves checking for retal-item before checking for retal-target.
   That is probably much more expensive on CPU than the other way round.
   Also see CHEAP_NO_TARGET_TEST. - C. Blue
   NOTE: I added code that prevents auto-ret with physical attacks against a
   wraithed target, this code is in EXPENSIVE_NO_TARGET_TEST only, so it won't
   work if this isn't defined. This was already the case for melee, just not for @O ranged. */
#define EXPENSIVE_NO_TARGET_TEST

/* Allow '@Ox' on any equipped item to stop melee auto-ret, if no other '@O' was found already? */
#define ALLOW_OX_ANYWHERE_HACK

static bool auto_retaliate_test(int Ind) {
	player_type *p_ptr = Players[Ind], *q_ptr, *p_target_ptr = NULL, *prev_p_target_ptr = NULL;
	int d, i, tx, ty, target, prev_target, item = -1;
	//char friends = 0;
	monster_type *m_ptr, *m_target_ptr = NULL, *prev_m_target_ptr = NULL;
	monster_race *r_ptr = NULL, *r_ptr2, *r_ptr0;
	object_type *o_ptr;
	cptr inscription = NULL, at_O_inscription = NULL;
	bool no_melee = FALSE, fallback = FALSE;
	bool skip_monsters = (p_ptr->cloaked || p_ptr->shadow_running) && !p_ptr->stormbringer;
	cave_type **zcave;
	/* are we dealing just physical damage? for wraithform checks (stormbringer ignores everything and just attacks anyway..): */
	bool physical = TRUE, wraith = (p_ptr->tim_wraith != 0) && !p_ptr->stormbringer;

	if (!(zcave = getcave(&p_ptr->wpos))) {
		p_ptr->ar_test_fail = TRUE;
		return(FALSE);
	}
	if (p_ptr->new_level_flag) {
		p_ptr->ar_test_fail = TRUE;
		return(FALSE);
	}
	/* disable auto-retaliation if we skip monsters/hostile players and blood-bonded players likewise */
	if (skip_monsters && !p_ptr->blood_bond) {
		p_ptr->ar_test_fail = TRUE;
		return(FALSE);
	}

	/* Just to kill compiler warnings */
	target = prev_target = 0;

#ifdef EXPENSIVE_NO_TARGET_TEST
	/* Pick an item with {@O} inscription */
	for (i = 0; i < INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];
		if (!o_ptr->tval) continue;

		inscription = quark_str(o_ptr->note);

		/* check for a valid inscription */
		if (inscription == NULL) continue;

		/* scan the inscription for @O */
		while (*inscription != '\0') {
			if (*inscription == '@' && inscription[1] != '\0') {
				inscription++;

				/* a valid @O has been located */
				/* @O shouldn't work on weapons or ammo in inventory - mikaelh */
				if ((*inscription == 'O' || *inscription == 'Q') && !(i < INVEN_WIELD &&
				    (is_melee_weapon(o_ptr->tval) || is_ammo(o_ptr->tval) ||
				    is_armour(o_ptr->tval) || o_ptr->tval == TV_MSTAFF ||
				    o_ptr->tval == TV_BOW || o_ptr->tval == TV_BOOMERANG))) {
					if (*inscription == 'Q') fallback = TRUE;
					inscription++;

					/* Skip this item in case it has @Ox */
					if (*inscription == 'x') {
						p_ptr->warning_autoret = 99; /* seems he knows what he's doing! */
						break;
					}
					/* Skip this item in case it has @Ot and we aren't in town */
					if (*inscription == 't') {
						p_ptr->warning_autoret = 99; /* seems he knows what he's doing! */
						if (!istownarea(&p_ptr->wpos, MAX_TOWNAREA)) break;
					}

					/* Select the first usable item with @O */
					item = i;
					i = INVEN_TOTAL;

					/* Remember the inscription */
					at_O_inscription = inscription;

					p_ptr->warning_autoret = 99; /* seems he knows what he's doing! */
					break;
				}
			}
			inscription++;
		}
	}

	/* check if we're using physical damage attacks */
	if (item != -1) physical = is_weapon(o_ptr->tval) || o_ptr->tval == TV_MSTAFF || is_ammo(o_ptr->tval);

 #ifdef ALLOW_OX_ANYWHERE_HACK
	/* Scan for @Ox to disable auto-retaliation only if no @O was found - mikaelh */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];
		if (!o_ptr->tval) continue;

		inscription = quark_str(o_ptr->note);

		/* check for a valid inscription */
		if (inscription == NULL) continue;

		/* scan the inscription for @Ox */
		while (*inscription != '\0') {
			if (inscription[0] == '@' && (inscription[1] == 'O' || inscription[1] == 'Q') && inscription[2] == 'x') {
				p_ptr->warning_autoret = 99; /* seems he knows what he's doing! */

				if (i == INVEN_WIELD || i == INVEN_ARM) {
					/* Prevent melee retaliation - mikaelh */
					no_melee = TRUE;
				}

				if (item == -1) {
					/* Select the item with @Ox */
					item = i;

					/* Make at_O_inscription point to the 'x' */
					at_O_inscription = inscription + 2;

					i = INVEN_TOTAL; /* exit the outer loop too */
					break;
				}
			}
			inscription++;
		}
	}
 #endif

	/* no specific item inscribed -> use default melee attacks */
	if (item == -1) physical = TRUE;
#endif

	/* check for monster/player targets */
	for (d = 1; d <= 9; d++) {
		if (d == 5) continue;

		tx = p_ptr->px + ddx[d];
		ty = p_ptr->py + ddy[d];

		if (!in_bounds(ty, tx)) continue;

		if (!(i = zcave[ty][tx].m_idx)) continue;
		if (i > 0) {
			m_ptr = &m_list[i];
			r_ptr0 = race_inf(m_ptr);

			/* Paranoia -- Skip dead monsters */
			if (!m_ptr->r_idx) continue;

			/* Make sure that the player can see this monster */
			if (!p_ptr->mon_vis[i]) continue;

			/* Stop annoying auto-retaliation against certain 'monsters' */
			if (r_ptr0->flags8 & RF8_NO_AUTORET) continue;

			if (m_ptr->status & M_STATUS_FRIENDLY) continue;

#ifdef EXPENSIVE_NO_TARGET_TEST
			/* Skip monsters we cannot actually target! (Sparrows) */
			if ((r_ptr0->flags7 & RF7_NO_TARGET) &&
			    is_ranged_item(Ind, &p_ptr->inventory[item]))
				continue;

			/* New - wraithform checks: Don't attack monsters that we'd deal 0 damage against and just wake up */
			if (wraith && physical && !(r_ptr0->flags2 & RF2_PASS_WALL)) continue;
#endif

			/* Protect pets/golems */
#if 0 /* Only vs our own pet/golem? */
			if (p_ptr->id == m_ptr->owner && !p_ptr->stormbringer) continue;
#else /* vs own+other players' pets/golems? */
			if (m_ptr->owner && !p_ptr->stormbringer) continue;
#endif

			/* specialty: don't auto-retaliate charmed monsters.
			   Maybe slightly inconsistent with the fact that we still auto-ret sleeping monsters ;). */
			if (m_ptr->charmedignore && !p_ptr->stormbringer) continue;

			/* Figure out if this is the best target so far */
			if (!m_target_ptr) {
				/* It's the first monster we're checking */
				prev_m_target_ptr = m_target_ptr;
				m_target_ptr = m_ptr;
				r_ptr = race_inf(m_ptr);
				prev_target = target;
				target = i;
			} else {
				/* Target dummy should always be the last one to get attacked - mikaelh */
				if (m_ptr->r_idx == RI_TARGET_DUMMY1 || m_ptr->r_idx == RI_TARGET_DUMMY2 ||
				    m_ptr->r_idx == RI_TARGET_DUMMYA1 || m_ptr->r_idx == RI_TARGET_DUMMYA2)
					continue;

				r_ptr2 = r_ptr;
				r_ptr = race_inf(m_ptr);

				/* Don't attack sleeping monsters before awake monsters */
				if (m_target_ptr->csleep && !m_ptr->csleep) {
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* If it is a Q, then make it our new target. */
				/* We don't handle the case of choosing between two
				 * Q's because if the player is standing next to two Q's
				 * he deserves whatever punishment he gets.
				 */
				else if (r_ptr->d_char == 'Q') {
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* Otherwise if it is 20 levels higher than everything
				 * else attack it.
				 */
				else if ((r_ptr->level - 20) >= r_ptr2->level) {
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* Otherwise if it is the most proportionatly wounded monster
				 * attack it.
				 */
				else if (m_ptr->hp * m_target_ptr->maxhp < m_target_ptr->hp * m_ptr->maxhp) {
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* If it is a tie attack the higher level monster */
				else if (m_ptr->hp * m_target_ptr->maxhp == m_target_ptr->hp * m_ptr->maxhp) {
					if (r_ptr->level > r_ptr2->level) {
						prev_m_target_ptr = m_target_ptr;
						m_target_ptr = m_ptr;
						prev_target = target;
						target = i;
					}
					/* If it is a tie attack the monster with less hit points */
					else if (r_ptr->level == r_ptr2->level) {
						if (m_ptr->hp < m_target_ptr->hp) {
							prev_m_target_ptr = m_target_ptr;
							m_target_ptr = m_ptr;
							prev_target = target;
							target = i;
						}
					}
				}
			}
		} else if (cfg.use_pk_rules != PK_RULES_NEVER) {
			i = -i;
			if (!(q_ptr = Players[i])) continue;

			/* Skip non-connected players */
			if (q_ptr->conn == NOT_CONNECTED) continue;

			/* Skip players we aren't hostile to */
			if (!check_hostile(Ind, i) && !p_ptr->stormbringer) continue;

			/* Skip players we cannot see */
			if (!p_ptr->play_vis[i]) continue;

			/* Figure out if this is the best target so far */
			if (p_target_ptr) {
				/* If we are 15 levels over the old target, make this
				 * player our new target.
				 */
				if ((q_ptr->lev - 15) >= p_target_ptr->lev) {
					prev_p_target_ptr = p_target_ptr;
					p_target_ptr = q_ptr;
					prev_target = target;
					target = -i;
				}
				/* Otherwise attack this player if he is more proportionatly
				 * wounded than our old target.
				 */
				else if (q_ptr->chp * p_target_ptr->mhp < p_target_ptr->chp * q_ptr->mhp) {
					prev_p_target_ptr = p_target_ptr;
					p_target_ptr = q_ptr;
					prev_target = target;
					target = -i;
				}
				/* If it is a tie attack the higher level player */
				else if (q_ptr->chp * p_target_ptr->mhp == p_target_ptr->chp * q_ptr->mhp) {
					if (q_ptr->lev > p_target_ptr->lev) {
						prev_p_target_ptr = p_target_ptr;
						p_target_ptr = q_ptr;
						prev_target = target;
						target = -i;
					}
					/* If it is a tie attack the player with less hit points */
					else if (q_ptr->lev == p_target_ptr->lev) {
						if (q_ptr->chp < p_target_ptr->chp) {
							prev_p_target_ptr = p_target_ptr;
							p_target_ptr = q_ptr;
							prev_target = target;
							target = -i;
						}
					}
				}
			} else {
				prev_p_target_ptr = p_target_ptr;
				p_target_ptr = q_ptr;
				prev_target = target;
				target = -i;
			}
		}
	}

	/* Nothing to attack */
	if (!p_target_ptr && !m_target_ptr) {
		/* Hack: If we don't have a 'continuous attack' ie are out of
		   adjacent targets, which just happened here, stop 'piercing' attacks */
		if (p_ptr->piercing && !p_ptr->piercing_charged) p_ptr->piercing = 0;

		p_ptr->ar_test_fail = TRUE;
		p_ptr->melee_crit_dual = 0;
		return(FALSE);
	}

#ifndef EXPENSIVE_NO_TARGET_TEST
	/* Pick an item with {@O} inscription */
	for (i = 0; i < INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];
		if (!o_ptr->tval) continue;

		inscription = quark_str(o_ptr->note);

		/* check for a valid inscription */
		if (inscription == NULL) continue;

		/* scan the inscription for @O */
		while (*inscription != '\0') {
			if (*inscription == '@') {
				inscription++;

				/* a valid @O has been located */
				/* @O shouldn't work on weapons or ammo in inventory - mikaelh */
				if ((*inscription == 'O' || *inscription == 'Q') && !(i < INVEN_WIELD &&
				    (is_melee_weapon(o_ptr->tval) || is_ammo(o_ptr->tval) ||
				    is_armour(o_ptr->tval) || o_ptr->tval == TV_MSTAFF ||
				    o_ptr->tval == TV_BOW || o_ptr->tval == TV_BOOMERANG))) {
					if (*inscription == 'Q') fallback = TRUE;
					inscription++;

 #if 0 //wrong here?
					/* Skip this item in case it has @Ox */
					if (*inscription == 'x') {
						p_ptr->warning_autoret = 99; /* seems he knows what he's doing! */
						break;
					}
 #endif

					/* Skip this item in case it has @Ot and we aren't in town */
					if (*inscription == 't') {
						p_ptr->warning_autoret = 99; /* seems he knows what he's doing! */
						if (!istownarea(&p_ptr->wpos, MAX_TOWNAREA)) break;
					}

					/* Select the first usable item with @O */
					item = i;
					i = INVEN_TOTAL;

					/* Remember the inscription */
					at_O_inscription = inscription;

					p_ptr->warning_autoret = 99; /* seems he knows what he's doing! */
					break;
				}
			}
			inscription++;
		}
	}

	/* Scan for @Ox to disable auto-retaliation only if no @O was found - mikaelh */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];
		if (!o_ptr->tval) continue;

		inscription = quark_str(o_ptr->note);

		/* check for a valid inscription */
		if (inscription == NULL) continue;

		/* scan the inscription for @O */
		while (*inscription != '\0') {
			if (inscription[0] == '@' && (inscription[1] == 'O' || inscription[1] == 'Q') && inscription[2] == 'x') {
				p_ptr->warning_autoret = 99; /* seems he knows what he's doing! */

				if (i == INVEN_WIELD || i == INVEN_ARM) {
					/* Prevent melee retaliation - mikaelh */
					no_melee = TRUE;
				}

				if (item == -1) {
					/* Select the item with @Ox */
					item = i;

					/* Make at_O_inscription point to the 'x' */
					at_O_inscription = inscription + 2;

					i = INVEN_TOTAL; /* exit the outer loop too */
					break;
				}
			}
			inscription++;
		}
	}
#endif


	/* save data, for subsequent auto_retaliate() call */
	p_ptr->ar_m_target_ptr = m_target_ptr;
	p_ptr->ar_prev_m_target_ptr = prev_m_target_ptr;
	p_ptr->ar_p_target_ptr = p_target_ptr;
	p_ptr->ar_prev_p_target_ptr = prev_p_target_ptr;

	p_ptr->ar_target = target;
	p_ptr->ar_item = item;
	p_ptr->ar_fallback = fallback;
	p_ptr->ar_at_O_inscription = at_O_inscription;
	p_ptr->ar_no_melee = no_melee | (p_ptr->autoret_base & 0x1);

	p_ptr->ar_test_fail = FALSE;
	/* employ forced (via @O or @Q inscription) auto-retaliation? */
	return(at_O_inscription && *at_O_inscription != 'x');
}
static int auto_retaliate(int Ind) {
	player_type *p_ptr = Players[Ind], *p_target_ptr, *prev_p_target_ptr;
	int target, item;
	//char friends = 0;
	monster_type *m_target_ptr, *prev_m_target_ptr;
	cptr at_O_inscription;
	bool no_melee, fallback;
	bool skip_monsters = (p_ptr->cloaked || p_ptr->shadow_running) && !p_ptr->stormbringer;
	cave_type **zcave;

	if (!(zcave = getcave(&p_ptr->wpos))) return(FALSE);
	if (p_ptr->new_level_flag) return(0);
	/* disable auto-retaliation if we skip monsters/hostile players and blood-bonded players likewise */
	if (skip_monsters && !p_ptr->blood_bond) return(0);


	/* load data, from preceding auto_retaliate_test() call */
	if (p_ptr->ar_test_fail) return(FALSE);

	m_target_ptr = p_ptr->ar_m_target_ptr;
	prev_m_target_ptr = p_ptr->ar_prev_m_target_ptr;
	p_target_ptr = p_ptr->ar_p_target_ptr;
	prev_p_target_ptr = p_ptr->ar_prev_p_target_ptr;

	target = p_ptr->ar_target;
	item = p_ptr->ar_item;
	fallback = p_ptr->ar_fallback;
	at_O_inscription = p_ptr->ar_at_O_inscription;
	no_melee = p_ptr->ar_no_melee;

	/* Don't melee if melee-ar is set to town-only */
	if ((p_ptr->autoret_base & 0x5) == 0x4) { //(not a typo @ 5 vs 4: 1 means that melee-ret is disabled completely (and for paranoia: 0x2 is not available))
		p_ptr->warning_autoret = 99; /* seems he knows what he's doing! */
		if (!istownarea(&p_ptr->wpos, MAX_TOWNAREA)) no_melee = TRUE;
	}


	/* If we have a player target, attack him. */
	if (p_target_ptr) {
		/* set the target */
		p_ptr->target_who = target;

		/* Attack him */
		/* Stormbringer bypasses everything!! */
		//py_attack(Ind, p_target_ptr->py, p_target_ptr->px);
		if (p_ptr->stormbringer || (
#ifdef AUTO_RET_CMD
		    !retaliate_cmd(Ind, fallback) &&
#endif
		    !retaliate_item(Ind, item, at_O_inscription, fallback) &&
		    !p_ptr->afraid && !no_melee)) {
			py_attack(Ind, p_target_ptr->py, p_target_ptr->px, FALSE);
		}

		/* Check if he is still alive or another targets exists */
		if ((!p_target_ptr->death) || (prev_p_target_ptr) || (m_target_ptr)) {
			/* We attacked something */
			return(1);
		} else {
			/* Otherwise return 2 to indicate we are no longer
			 * autoattacking anything.
			 */
			return(2);
		}
	}

	/* The dungeon master does not fight his or her offspring */
	if (p_ptr->admin_dm) return(0);

	/* If we have a target to attack, attack it! */
	if (m_target_ptr) {
		/* set the target */
		p_ptr->target_who = target;
		if (m_target_ptr->pet) { //a pet?
			return(0);
		}

		/* Attack it */
		//py_attack(Ind, m_target_ptr->fy, m_target_ptr->fx);
		if (p_ptr->stormbringer || (
#ifdef AUTO_RET_CMD
		    !retaliate_cmd(Ind, fallback) &&
#endif
		    !retaliate_item(Ind, item, at_O_inscription, fallback) &&
		    !p_ptr->afraid && !no_melee)) {
			py_attack(Ind, m_target_ptr->fy, m_target_ptr->fx, FALSE);

			/* manage autoretaliation warning for newbies (reset it) */
			if (p_ptr->warning_autoret != 99) p_ptr->warning_autoret = 0;
		}

		/* Check if it is still alive or another targets exists */
		if ((m_target_ptr->r_idx) || (prev_m_target_ptr) || (p_target_ptr)) {
			/* We attacked something */
			return(1);
		} else {
			/* Otherwise return 2 to indicate we are no longer
			 * autoattacking anything.
			 */
			return(2);
		}
	}

	/* Nothing was attacked. */
	return(0);
}

/*
 * Player processing that occurs at the beginning of a new turn (aka server frame)
 */
static void process_player_begin(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* for AT_VALINOR: */
	int i, x, y, xstart = 0, ystart = 0, ox, oy;
	cave_type **zcave;
	dun_level *l_ptr;

	p_ptr->heal_turn_20 +=  p_ptr->heal_turn[0] - p_ptr->heal_turn[20];
	p_ptr->heal_turn_10 +=  p_ptr->heal_turn[0] - p_ptr->heal_turn[10];
	p_ptr->heal_turn_5 +=  p_ptr->heal_turn[0] - p_ptr->heal_turn[5];
	p_ptr->dam_turn_20 = p_ptr->dam_turn[0] - p_ptr->dam_turn[20];
	p_ptr->dam_turn_10 = p_ptr->dam_turn[0] - p_ptr->dam_turn[10];
	p_ptr->dam_turn_5 = p_ptr->dam_turn[0] - p_ptr->dam_turn[5];
	for (i = 20; i > 0; i--) {
	/* move internal heal/turn register for C_BLUE_AI */
		p_ptr->heal_turn[i] = p_ptr->heal_turn[i - 1];
	/* move internal dam/turn register for C_BLUE_AI */
		p_ptr->dam_turn[i] = p_ptr->dam_turn[i - 1];
	}
	p_ptr->heal_turn[0] = 0; /* prepared to store healing that might happen */
	p_ptr->dam_turn[0] = 0; /* prepared to store damage dealing that might happen */

	/* Perform pending automatic transportation */
	switch (p_ptr->auto_transport) {
	case AT_BLINK: teleport_player_force(Ind, 10); p_ptr->auto_transport = 0; break;
	case AT_TPORT: teleport_player_force(Ind, 100); p_ptr->auto_transport = 0; break;
	case AT_VALINOR: /* allocate Valinor; recall player there */
#if 0
		if (players_on_depth(wpos) && !getcave(wpos)) {
			/* Allocate space for it */
			alloc_dungeon_level(wpos);
			/* Generate a dungeon level there */
			generate_cave(wpos, p_ptr);
		}
		zcave = getcave(wpos);
		l_ptr = getfloor(wpos);
#endif
		p_ptr->recall_pos.wx = valinor_wpos_x;
		p_ptr->recall_pos.wy = valinor_wpos_y;
		p_ptr->recall_pos.wz = valinor_wpos_z;
		// let's try LEVEL_OUTSIDE_RAND (5) instead of LEVEL_OUTSIDE (4) - C. Blue :)
		//p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
		p_ptr->new_level_method = LEVEL_RAND;
		recall_player(Ind, "");
		p_ptr->auto_transport = AT_VALINOR2;
		break;
	case AT_VALINOR2: /* (re-)generate Valinor from scratch; move player into position; lite it up for the show; place 'monsters' */
		zcave = getcave(&p_ptr->wpos);
		l_ptr = getfloor(&p_ptr->wpos);
		/* Get rid of annoying level flags */
		l_ptr->flags1 &= ~(LF1_NO_MAP | LF1_NO_MAGIC_MAP);
		for (y = 0; y < MAX_HGT / 3; y++) {
			for (x = 0; x < MAX_WID; x++) {
				zcave[y][x].feat = FEAT_HIGH_MOUNT_SOLID;
				zcave[y + MAX_HGT / 3][x].feat = FEAT_GLIT_WATER;
				zcave[y + (MAX_HGT * 2) / 3][x].feat = FEAT_GLIT_WATER;
			}
		}
		/* Wipe any monsters/objects */
		wipe_o_list_safely(&p_ptr->wpos);
		wipe_m_list(&p_ptr->wpos);
		/* Regenerate the level from fixed layout */
		process_dungeon_file("t_valinor.txt", &p_ptr->wpos, &ystart, &xstart, 21, 66, TRUE);
		for (x = 0; x <= 65; x++) zcave[21][x].feat = FEAT_GLIT_WATER;
		zcave[20][0].feat = FEAT_GLIT_WATER;
		zcave[20][65].feat = FEAT_GLIT_WATER;
		/* Some lil hacks */
		msg_format(Ind, "\377%cYou enter the shores of Valinor..", COLOUR_DUNGEON);
		wiz_lite_extra(Ind);
		/* Move @ to designated starting position (level_rand_x/y()) and redraw */
		oy = p_ptr->py;
		ox = p_ptr->px;
		p_ptr->py = 15;
		p_ptr->px = 16;
		zcave[oy][ox].m_idx = 0;
		zcave[p_ptr->py][p_ptr->px].m_idx = 0 - Ind;
		everyone_lite_spot(&p_ptr->wpos, oy, ox);
		everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
		/* Summon 'monsters' */
		place_monster_one(&p_ptr->wpos, 7, 10, RI_BRIGHTLANCE, 0, 0, 0, 0, 0);
		everyone_lite_spot(&p_ptr->wpos, 7, 10);
		place_monster_one(&p_ptr->wpos, 7, 15, RI_BRIGHTLANCE, 0, 0, 0, 0, 0);
		everyone_lite_spot(&p_ptr->wpos, 7, 15);
		place_monster_one(&p_ptr->wpos, 10, 25, RI_OROME, 0, 0, 0, 0, 0);
		everyone_lite_spot(&p_ptr->wpos, 10, 25);
		p_ptr->update |= PU_LITE;
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
		p_ptr->update |= (PU_DISTANCE);
		redraw_stuff(Ind);
		p_ptr->auto_transport = AT_VALINOR3;
		p_ptr->auto_transport_turn = turn;
		break;
	case AT_VALINOR3:	/* Orome mumbles */
		if (turn < p_ptr->auto_transport_turn + cfg.fps * 1) break; /* cool down.. */
		msg_print(Ind, "\374 ");
		msg_print(Ind, "\374\377oOrome, the Hunter, mumbles something about a spear..");
		p_ptr->auto_transport = AT_VALINOR4;
		p_ptr->auto_transport_turn = turn;
		break;
	case AT_VALINOR4:	/* Orome looks */
		if (turn < p_ptr->auto_transport_turn + cfg.fps * 5) break; /* cool down.. */
		msg_print(Ind, "\374 ");
		msg_print(Ind, "\374\377oOrome, the Hunter, notices you and surprise crosses his face!");
		p_ptr->auto_transport = AT_VALINOR5;
		p_ptr->auto_transport_turn = turn;
		break;
	case AT_VALINOR5:	/* Orome laughs */
		if (turn < p_ptr->auto_transport_turn + cfg.fps * 4) break; /* cool down.. */
		msg_print(Ind, "\374 ");
		msg_print(Ind, "\374\377oOrome, the Hunter, laughs out loudly!");
#if 0
		set_afraid(Ind, 1 + 2 * (1 + get_skill_scale(p_ptr, SKILL_COMBAT, 3 + 1)));
#else
		p_ptr->afraid = 1 + 2 * (1 + get_skill_scale(p_ptr, SKILL_COMBAT, 3 + 1));
		disturb(Ind, 0, 0);
		p_ptr->redraw |= (PR_AFRAID);
		handle_stuff(Ind);
#endif
		p_ptr->auto_transport = AT_VALINOR6;
		p_ptr->auto_transport_turn = turn;
		break;
	case AT_VALINOR6:	/* Orome offers */
		if (turn < p_ptr->auto_transport_turn + cfg.fps * 8) break; /* cool down.. */
		msg_print(Ind, "\374 ");
		msg_print(Ind, "\374\377oOrome, the Hunter, offers you to stay here!");
		msg_print(Ind, "\374\377y  (You may hit the suicide keys in order to retire here,");
		msg_print(Ind, "\374\377y  or take the staircase back to the mundane world.)");
		msg_print(Ind, "\374 ");
		p_ptr->auto_transport = 0;
		break;
	case AT_VALINORX:	/* Orome pats */
		if (turn < p_ptr->auto_transport_turn + cfg.fps * 3) break; /* cool down.. */
		msg_print(Ind, "\374 ");
		msg_print(Ind, "\374\377oOrome, the Hunter, heartily *pats* you on the back!");
		bypass_invuln = TRUE;
		take_hit(Ind, p_ptr->chp / 6, "a *pat* on the back", 0);
		bypass_invuln = FALSE;
		p_ptr->auto_transport = 0;
		break;
	case AT_PARTY:
		disturb(Ind, 0, 0);//stop running for a moment..
		p_ptr->auto_transport = 0;
		p_ptr->recall_pos.wx = p_ptr->wpos.wx;
		p_ptr->recall_pos.wy = p_ptr->wpos.wy;
		//switch dungeon - assume that there is no other dungeon/tower on same wpos as Death Fate!
		if (p_ptr->wpos.wz < 0) {
			p_ptr->recall_pos.wz = 1;
			//add the temporary 'mirror' dungeon -- maybe use verify_dungeon() instead
			if (!(wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags & WILD_F_UP))
				add_dungeon(&p_ptr->wpos, 1, 2, DF1_UNLISTED | DF1_NO_RECALL, DF2_IRON | DF2_NO_EXIT_MASK |
				    DF2_NO_ENTRY_MASK | DF2_RANDOM,
				    DF3_NO_SIMPLE_STORES | DF3_NO_DUNGEON_BONUS | DF3_EXP_20, TRUE, 0, DI_DEATH_FATE, 0, 0);
		} else {
			p_ptr->recall_pos.wz = -1;
			//add the temporary 'mirror' dungeon -- maybe use verify_dungeon() instead
			if (!(wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags & WILD_F_DOWN))
				add_dungeon(&p_ptr->wpos, 1, 2, DF1_UNLISTED | DF1_NO_RECALL, DF2_IRON | DF2_NO_EXIT_MASK |
				    DF2_NO_ENTRY_MASK | DF2_RANDOM,
				    DF3_NO_SIMPLE_STORES | DF3_NO_DUNGEON_BONUS | DF3_EXP_20, FALSE, 0, DI_DEATH_FATE, 0, 0);
		}
		//get him there
		p_ptr->new_level_method = LEVEL_RAND;
		recall_player(Ind, "");
		if (!getcave(&p_ptr->wpos)) {
			p_ptr->auto_transport = AT_PARTY2;
			break;
		}
		//fall through
	case AT_PARTY2:
		p_ptr->auto_transport = 0;
		//don't rely on 'P:' because we use an extra staircase now for the balcony
		oy = p_ptr->py;
		ox = p_ptr->px;
		p_ptr->py = 2;
		p_ptr->px = 1;
		zcave = getcave(&p_ptr->wpos);
		zcave[oy][ox].m_idx = 0;
		zcave[p_ptr->py][p_ptr->px].m_idx = 0 - Ind;
		everyone_lite_spot(&p_ptr->wpos, oy, ox);
		everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
		p_ptr->player_sees_dm = TRUE;
		break;
	case AT_MUMBLE:
		if (turn < p_ptr->auto_transport_turn + cfg.fps * 2) break; /* cool down.. */
		msg_print(Ind, "\374\377BYou hear some very annoyed mumbling...");
		p_ptr->auto_transport = 0;
		break;
	}

#if defined(TARGET_SWITCHING_COST) || defined(TARGET_SWITCHING_COST_RANGED)
	//Verify: This code should not require additional modifications for NEW_AUTORET_ENERGY:reserve_energy feat
	if (p_ptr->tsc_lasttarget && p_ptr->energy >= level_speed(&p_ptr->wpos) * 2 - 1) {
		if (p_ptr->tsc_idle_energy >= level_speed(&p_ptr->wpos)) {
			p_ptr->tsc_lasttarget = 0;
			p_ptr->tsc_idle_energy = 0;
		} else p_ptr->tsc_idle_energy += extract_energy[p_ptr->pspeed];
	}
#endif

	/* Give the player some energy */
	p_ptr->energy += extract_energy[p_ptr->pspeed];
	limit_energy(p_ptr);

	/* clear 'quaked' flag for p_ptr->impact limiting */
	p_ptr->quaked = FALSE;

	/* Check "resting" status */
	if (p_ptr->resting) {
		/* No energy availiable while resting */
		// This prevents us from instantly waking up.
		p_ptr->energy = 0;
	}

	/* Handle paralysis here */
#ifndef ARCADE_SERVER
	if (p_ptr->paralyzed || p_ptr->stun > 100)
		p_ptr->energy = 0;
#else
	if (p_ptr->paralyzed)
		p_ptr->energy = 0;
#endif
	/* Specialty when we want to sync events and keep players and monsters on a short hiatus for reading story or w/e */
	if (p_ptr->suspended) {
		if (p_ptr->suspended > turn) p_ptr->energy = 0;
		else {
			p_ptr->suspended = 0;
			p_ptr->redraw |= PR_STATE;
		}
	}

	/* Hack -- semi-constant hallucination */
	if (p_ptr->image && (randint(10) < 1)) p_ptr->redraw |= (PR_MAP);

	/* Mega-Hack -- Random teleportation XXX XXX XXX */
	if ((p_ptr->teleport) && (rand_int(100) < 1)
	    /* not during highlander tournament (or other events, for now) */
	    && !(in_sector000(&p_ptr->wpos) && sector000separation)) {
		/* Teleport player */
		teleport_player(Ind, 40, FALSE);
	}

	if (p_ptr->melee_timeout_crit_dual) p_ptr->melee_timeout_crit_dual--;
}


/*
 * Generate the feature effect
 */
static void apply_terrain_effect(int Ind) {
	player_type *p_ptr = Players[Ind];
	int y = p_ptr->py, x = p_ptr->px;
	cave_type *c_ptr, **zcave;
	feature_type *f_ptr;

	if (!(zcave = getcave(&p_ptr->wpos))) return;

	c_ptr = &zcave[y][x];
	f_ptr = &f_info[c_ptr->feat];

	if (f_ptr->d_frequency[0] != 0) {
		int i;

		for (i = 0; i < 4; i++) {
			/* Check the frequency */
			if (f_ptr->d_frequency[i] == 0) continue;

			if ((!rand_int(f_ptr->d_frequency[i])) &&
			    ((f_ptr->d_side[i] != 0) || (f_ptr->d_dice[i] != 0))) {
				int l, dam = 0;
				int d = f_ptr->d_dice[i], s = f_ptr->d_side[i];

				if (d == -1) d = p_ptr->lev;
				if (s == -1) s = p_ptr->lev;

				/* Roll damage */
				for (l = 0; l < d; l++) dam += randint(s);

				/* Apply damage */
				project(PROJECTOR_TERRAIN, 0, &p_ptr->wpos, y, x, dam, f_ptr->d_type[i],
				    PROJECT_NORF | PROJECT_KILL | PROJECT_HIDE | PROJECT_JUMP | PROJECT_NODO | PROJECT_NODF, "");

				/* Hack -- notice death */
				//if (!alive || death) return;
				if (p_ptr->death) return;
			}
		}
	}
}

#if 0
/* admin can summon a player anywhere he/she wants */
void summon_player(int victim, struct worldpos *wpos, char *message) {
	struct player_type *p_ptr;
	p_ptr = Players[victim];
	p_ptr->recall_pos.wx = wpos->wx;
	p_ptr->recall_pos.wy = wpos->wy;
	p_ptr->recall_pos.wz = wpos->wz;
	recall_player(victim, message);
}
#endif

/* Actually recall a player with no questions asked.
   For player-initiated recall, this function is only called from do_recall(),
   but for special purpose (admin/events..) it can be called directly. */
void recall_player(int Ind, char *message) {
	struct player_type *p_ptr;
	cave_type **zcave;
	struct worldpos old_wpos;
	char buf[MAX_CHARS_WIDE];

	p_ptr = Players[Ind];

	if (!p_ptr) return;
	if (!(zcave = getcave(&p_ptr->wpos))) {
		/* This seemed to have happened as 'double recall', leading to a panic save:
		   A poisoned player recalled to orc caves surface and died with insta-res enabled just the moment he arrived,
		   resulting in the level method getting set to LEVEL_TO_TEMPLE, but the cave not yet allocated,
		   making the 2nd recall_player() (to temple) fail exactly here and therefore return(),
		   so the subsequent process_player_change_wpos() call would attempt to apply LEVEL_TO_TEMPLE in the orc caves sector,
		   which of course fails as there is no temple there, sets p_ptr->px/py both to 0, and this in turn crashed update_lite()...
		   Solution: Ensure cave is allocated!   - C. Blue */
		s_printf("ERORR in recall_player(): <%s>(%d) at (%d,%d,%d) has no cave!\n", p_ptr->name, Ind, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
#if 0
		return;	// eww
#else
		/* Actually allocate the cave so we don't fail! */
		alloc_dungeon_level(&p_ptr->wpos);
		zcave = getcave(&p_ptr->wpos);
#endif
	}

	/* Always clear exception-flag */
	p_ptr->global_event_temp &= ~PEVF_PASS_00;

	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);

	/* Get him out of any pending quest input prompts */
	if (p_ptr->request_id >= RID_QUEST && p_ptr->request_id <= RID_QUEST_ACQUIRE + MAX_Q_IDX - 1) {
		Send_request_abort(Ind);
		p_ptr->request_id = RID_NONE;
	}

#ifdef USE_SOUND_2010
	sound(Ind, "recall", "teleport", SFX_TYPE_COMMAND, FALSE);
#endif

	/* Remove the player */
	zcave[p_ptr->py][p_ptr->px].m_idx = 0;

	/* Show everyone that he's left */
	everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

	msg_print(Ind, message);

	/* Forget his lite and view */
	forget_lite(Ind);
	forget_view(Ind);

#if 1 //todo: fix again after it broke: actually this code should instead have been done already in process_player_change_wpos() or related functions >.>
	/* sanity check on recall coordinates */
	p_ptr->recall_pos.wx %= MAX_WILD_X;
	p_ptr->recall_pos.wy %= MAX_WILD_Y;
	if (p_ptr->recall_pos.wz < 0) {
		if (wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx].dungeon) {
			if (-p_ptr->recall_pos.wz > wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx].dungeon->maxdepth)
				p_ptr->recall_pos.wz = -wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx].dungeon->maxdepth;
		} else p_ptr->recall_pos.wz = 0;
	} else if (p_ptr->recall_pos.wz > 0) {
		if (wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx].tower) {
			if (p_ptr->recall_pos.wz > wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx].tower->maxdepth)
				p_ptr->recall_pos.wz = wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx].tower->maxdepth;
		} else p_ptr->recall_pos.wz = 0;
	}
#endif

	/* Change the wpos */
	wpcopy(&old_wpos, &p_ptr->wpos);
	wpcopy(&p_ptr->wpos, &p_ptr->recall_pos);

	/* One less person here */
	new_players_on_depth(&old_wpos, -1, TRUE);

	/* Log it */
	s_printf("Recalled: %s from %d,%d,%d to %d,%d,%d.\n", p_ptr->name,
	    old_wpos.wx, old_wpos.wy, old_wpos.wz,
	    p_ptr->recall_pos.wx, p_ptr->recall_pos.wy, p_ptr->recall_pos.wz);

	/* One more person here */
	p_ptr->new_level_flag = TRUE;
	new_players_on_depth(&p_ptr->wpos, 1, TRUE);

	/* He'll be safe for some turns */
	set_invuln_short(Ind, RECALL_GOI_LENGTH);	// It runs out if attacking anyway

	/* cancel any user recalls */
	p_ptr->word_recall = 0;


	/* check for /tpto admin command that allows changing between 2 wz positions that are both != 0: */
	if (p_ptr->wpos.wz) return;


	/* Update wilderness map! This is for RECALL_MAX_RANGE:
	   We learn about the intermediate world map sectors we land on. */
	if (!p_ptr->ghost)
		p_ptr->wild_map[(p_ptr->wpos.wx + p_ptr->wpos.wy * MAX_WILD_X) / 8] |=
		    (1U << ((p_ptr->wpos.wx + p_ptr->wpos.wy * MAX_WILD_X) % 8));

	/* Did we really make it through all floors of the ironman challenge dungeon? */
	if (in_irondeepdive(&old_wpos) && !is_admin(p_ptr)) {
		int i, j;

#ifdef IRONDEEPDIVE_FIXED_TOWN_WITHDRAWAL
		bool success = TRUE;

		if (getlevel(&old_wpos) == IDDC_TOWN1_FIXED || getlevel(&old_wpos) == IDDC_TOWN2_FIXED) success = FALSE;
		if (success)
#endif
		for (i = 0; i < IDDC_HIGHSCORE_SIZE; i++) {
#ifdef IDDC_THROUGH_IS_FIRST /* 0->always replace first entry! */
			if (deep_dive_level[i] == -1) {
 #ifdef IDDC_RESTRICT_ACC_CLASS /* only allow one entry of same account+class? : discard new entry */
				if (!strcmp(deep_dive_account[i], p_ptr->accountname) &&
				    deep_dive_class[i] == p_ptr->pclass) {
					i = IDDC_HIGHSCORE_DISPLAYED;
					break;
				}
 #endif
				continue;
			}
#endif
#ifdef IDDC_RESTRICT_ACC_CLASS /* only allow one entry of same account+class? : discard previous entry */
			for (j = i; j < IDDC_HIGHSCORE_SIZE - 1; j++) {
				if (!strcmp(deep_dive_account[j], p_ptr->accountname) &&
				    deep_dive_class[j] == p_ptr->pclass) {
					int k;

					/* pull up all succeeding entries by 1  */
					for (k = j; k < IDDC_HIGHSCORE_SIZE - 1; k++) {
						deep_dive_level[k] = deep_dive_level[k + 1];
						strcpy(deep_dive_name[k], deep_dive_name[k + 1]);
						strcpy(deep_dive_char[k], deep_dive_char[k + 1]);
						strcpy(deep_dive_account[k], deep_dive_account[k + 1]);
						deep_dive_class[k] = deep_dive_class[k + 1];
					}
					break;
				}
			}
#endif
			/* push down all entries by 1, to make room for inserting new entry */
			for (j = IDDC_HIGHSCORE_SIZE - 1; j > i; j--) {
				deep_dive_level[j] = deep_dive_level[j - 1];
				strcpy(deep_dive_name[j], deep_dive_name[j - 1]);
				strcpy(deep_dive_char[j], deep_dive_char[j - 1]);
				strcpy(deep_dive_account[j], deep_dive_account[j - 1]);
				deep_dive_class[j] = deep_dive_class[j - 1];
			}
			deep_dive_level[i] = -1;
			//strcpy(deep_dive_name[i], p_ptr->name);
#ifdef IDDC_HISCORE_SHOWS_ICON
			sprintf(deep_dive_name[i], "%s, %s%s (\\{%c%c\\{s/\\{%c%d\\{s),",
			    p_ptr->name, get_prace2(p_ptr), class_info[p_ptr->pclass].title, color_attr_to_char(p_ptr->cp_ptr->color), p_ptr->fruit_bat ? 'b' : '@',
			    p_ptr->ghost ? 'r' : 's', p_ptr->max_plv);
#else
			sprintf(deep_dive_name[i], "%s, %s%s (\\{%c%d\\{s),",
			    p_ptr->name, get_prace2(p_ptr), class_info[p_ptr->pclass].title,
			    p_ptr->ghost ? 'r' : 's', p_ptr->max_plv);
#endif
			strcpy(deep_dive_char[i], p_ptr->name);
			strcpy(deep_dive_account[i], p_ptr->accountname);
			deep_dive_class[i] = p_ptr->pclass;
			break;
		}

#ifdef IRONDEEPDIVE_ALLOW_INCOMPAT
		/* need to leave party, since we might be teamed up with incompatible char mode players! */
		if (p_ptr->party && !p_ptr->admin_dm &&
		    compat_mode(p_ptr->mode, parties[p_ptr->party].cmode))
			party_leave(Ind, FALSE);
#endif

#ifdef IRONDEEPDIVE_FIXED_TOWN_WITHDRAWAL
		if (success) {
#endif
			if (p_ptr->iron_winner < 250) p_ptr->iron_winner++; /* actually count xD (byte, but need -1 for conversion from old bool type in load2.c) */
			if (p_ptr->iron_winner == 1) {
				msg_print(Ind, "\374\377L***\377aYou made it through the Ironman Deep Dive challenge!\377L***");
				sprintf(buf, "\374\377L***\377a%s made it through the Ironman Deep Dive challenge!\377L***", p_ptr->name);
				l_printf("%s \\{U%s (%d) made it through the Ironman Deep Dive challenge\n", showdate(), p_ptr->name, p_ptr->lev);
			} else if (p_ptr->iron_winner <= 99) {
				msg_format(Ind, "\374\377L***\377aYou made it through the Ironman Deep Dive challenge (x%d)!\377L***", p_ptr->iron_winner);
				sprintf(buf, "\374\377L***\377a%s made it through the Ironman Deep Dive challenge (x%d)!\377L***", p_ptr->name, p_ptr->iron_winner);
				l_printf("%s \\{U%s (%d) made it through the Ironman Deep Dive challenge (x%d)\n", showdate(), p_ptr->name, p_ptr->lev, p_ptr->iron_winner);
			} else { /* Nobody could be this insane, RIIIIIGHT? */
				msg_print(Ind, "\374\377L***\377aYou made it through the Ironman Deep Dive challenge (x99+)!\377L***");
				sprintf(buf, "\374\377L***\377a%s made it through the Ironman Deep Dive challenge (x99+)!\377L***", p_ptr->name);
				l_printf("%s \\{U%s (%d) made it through the Ironman Deep Dive challenge (x99+)\n", showdate(), p_ptr->name, p_ptr->lev);
			}
			msg_broadcast(Ind, buf);
#ifdef TOMENET_WORLDS
			if (cfg.worldd_events) world_msg(buf);
#endif
			/* enforce dedicated Ironman Deep Dive Challenge character slot usage */
			if (p_ptr->mode & MODE_DED_IDDC) {
				msg_print(Ind, "\377aYou return to town and may retire ('Q') when ready.");
				msg_print(Ind, "\377aIf you enter a dungeon or tower, \377Rretirement\377a is assumed \377Rautomatically\377a.");

				process_player_change_wpos(Ind);
				p_ptr->recall_pos = BREE_WPOS;

				wpcopy(&old_wpos, &p_ptr->wpos);
				wpcopy(&p_ptr->wpos, &p_ptr->recall_pos);
				new_players_on_depth(&old_wpos, -1, TRUE);
				s_printf("Recalled: %s from %d,%d,%d to %d,%d,%d.\n", p_ptr->name,
				    old_wpos.wx, old_wpos.wy, old_wpos.wz,
				    p_ptr->recall_pos.wx, p_ptr->recall_pos.wy, p_ptr->recall_pos.wz);
				p_ptr->new_level_flag = TRUE;
				new_players_on_depth(&p_ptr->wpos, 1, TRUE);
				set_invuln_short(Ind, RECALL_GOI_LENGTH);	// It runs out if attacking anyway
				p_ptr->word_recall = 0;
				process_player_change_wpos(Ind);

				/* Restrict character to world surface */
				if (p_ptr->iron_winner_ded < 250) p_ptr->iron_winner_ded++;
			}

			p_ptr->IDDC_flags = 0x0; //clear IDDC specialties
#ifdef IRONDEEPDIVE_FIXED_TOWN_WITHDRAWAL
		} else {
			/* enforce dedicated Ironman Deep Dive Challenge character slot usage */
			if (p_ptr->mode & MODE_DED_IDDC) {
				msg_print(Ind, "\377RYou failed to complete the Ironman Deep Dive Challenge!");
				strcpy(p_ptr->died_from, "indetermination");
				p_ptr->died_from_ridx = 0;
				p_ptr->deathblow = 0;
				p_ptr->death = TRUE;
				return;
			}

			sprintf(buf, "\374\377s%s withdrew from the Ironman Deep Dive challenge.", p_ptr->name);
			msg_broadcast(0, buf);
 #ifdef TOMENET_WORLDS
			if (cfg.worldd_events) world_msg(buf);
 #endif

			/* reduce his accumulated mimicry form knowledge somewhat
			   (keep some of the benefits of IDDC hunting) */
 #ifdef IDDC_MIMICRY_BOOST
			if (IDDC_MIMICRY_BOOST <= 2) j = 5;
			else if (IDDC_MIMICRY_BOOST == 3) j = 6;
			else if (IDDC_MIMICRY_BOOST == 4) j = 7;
			else if (IDDC_MIMICRY_BOOST <= 6) j = 10;
			else j = 13;
			for (i = 1; i < max_r_idx; i++) {
				if ((r_info[i].flags1 & RF1_UNIQUE)) continue;
				p_ptr->r_mimicry[i] = (p_ptr->r_mimicry[i] * 5) / j;
			}
			/* In case he can no longer use his current form */
			do_mimic_change(Ind, 0, TRUE);
 #endif

			p_ptr->IDDC_flags = 0x0; //clear IDDC specialties

			/* Be nice: Actually set all his true artifacts to non-iddc timeouting
			   (for IDDC_ARTIFACT_FAST_TIMEOUT) */
			for (i = 0; i < INVEN_TOTAL; i++) {
				if (!p_ptr->inventory[i].k_idx) continue;
				if (!p_ptr->inventory[i].name1 || p_ptr->inventory[i].name1 == ART_RANDART) continue;
				a_info[p_ptr->inventory[i].name1].iddc = FALSE;
			}
		}
#endif
	}

	/* Specialty: Did we make it through the Halls of Mandos?
	   Those are now ironman, so they're a 'pure', traditional ironman challenge.
	   However, this dungeon can be entered at any level, so it might be less of a challenge. */
	if (in_hallsofmandos(&old_wpos) && !is_admin(p_ptr)) {
		msg_print(Ind, "\374\377a***\377sYou made it through the Halls of Mandos!\377a***");
		sprintf(buf, "\374\377a***\377s%s made it through the Halls of Mandos!\377a***", p_ptr->name);
		msg_broadcast(Ind, buf);
		l_printf("%s \\{U%s (%d) made it through the Halls of Mandos\n", showdate(), p_ptr->name, p_ptr->lev);

#ifdef DED_IDDC_MANDOS
		/* enforce dedicated Ironman Deep Dive Challenge character slot usage */
		if (p_ptr->mode & MODE_DED_IDDC) {
			msg_print(Ind, "\377aYou return to town and may retire ('Q') when ready.");
			//msg_print(Ind, "\377aIf you enter a dungeon or tower, \377Rretirement\377a is assumed \377Rautomatically\377a.");

			process_player_change_wpos(Ind);
			p_ptr->recall_pos = BREE_WPOS;

			wpcopy(&old_wpos, &p_ptr->wpos);
			wpcopy(&p_ptr->wpos, &p_ptr->recall_pos);
			new_players_on_depth(&old_wpos, -1, TRUE);
			s_printf("Recalled: %s from %d,%d,%d to %d,%d,%d.\n", p_ptr->name,
			    old_wpos.wx, old_wpos.wy, old_wpos.wz,
			    p_ptr->recall_pos.wx, p_ptr->recall_pos.wy, p_ptr->recall_pos.wz);
			p_ptr->new_level_flag = TRUE;
			new_players_on_depth(&p_ptr->wpos, 1, TRUE);
			set_invuln_short(Ind, RECALL_GOI_LENGTH);	// It runs out if attacking anyway
			p_ptr->word_recall = 0;
			process_player_change_wpos(Ind);

			/* Restrict character to world surface */
			//p_ptr->iron_winner_ded++; --not a winner from Mandos-conquering
		}
#endif
	}

#ifdef MODULE_ALLOW_INCOMPAT
	if (in_module(&old_wpos) && !is_admin(p_ptr)) {
		/* need to leave party, since we might be teamed up with incompatible char mode players! */
		if (p_ptr->party && !p_ptr->admin_dm &&
		    compat_mode(p_ptr->mode, parties[p_ptr->party].cmode))
			party_leave(Ind, FALSE);
	}
#endif
}


/* Check if a player is unable to use Word of Recall (or teleportation) at his current location.
   Important, if sector00 check gets fully if 1'ed: p_ptr->recall_pos.. must be set. */
bool can_use_wordofrecall(player_type *p_ptr) {
	dungeon_type *d_ptr = getdungeon(&p_ptr->wpos);
	cave_type **zcave = getcave(&p_ptr->wpos);
	dun_level *l_ptr = getfloor(&p_ptr->wpos);

	/* special restriction for global events (Highlander Tournament) */
	if (sector000separation && !is_admin(p_ptr) && (
	    (p_ptr->wpos.wx == WPOS_SECTOR000_X && p_ptr->wpos.wy == WPOS_SECTOR000_Y)
#if 0 /* for now not needed, as we only call this function atm to check on start of a global event */
	    || (p_ptr->recall_pos.wx == WPOS_SECTOR000_X && p_ptr->recall_pos.wy == WPOS_SECTOR000_Y))
	    && !(p_ptr->global_event_temp & PEVF_PASS_00)
#else
	    )
#endif
	    )
		return(FALSE);

	if (d_ptr && !(getfloor(&p_ptr->wpos)->flags1 & LF1_IRON_RECALL) && (
	    (((d_ptr->flags1 & DF1_FORCE_DOWN) || (d_ptr->flags2 & DF2_IRON)) && d_ptr->maxdepth > ABS(p_ptr->wpos.wz)) ||
	    ((d_ptr->flags1 & DF1_NO_RECALL) || (d_ptr->flags2 & DF2_NO_EXIT_WOR))
	    ))
		return(FALSE);

	if (p_ptr->anti_tele) return(FALSE);
	if (zcave && (zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK)) return(FALSE);
#if 1
	/* Prevent teleporting someone onto a vault grid - this was allowed but poses the following problem:
	   A player entering a level via staircase into a vault can get teleported off the staircase.
	   Even if the vault isn't no-tele he can still fail to teleport out if the vault occupies the whole map.
	   Since this seems too harsh, let's just keep teleportation consistent in the way that you cannot tele onto icky grids in general. */
	if (zcave && (zcave[p_ptr->py][p_ptr->px].info & CAVE_ICKY)) return(FALSE);
#endif

	if ((p_ptr->global_event_temp & PEVF_NOTELE_00)) return(FALSE);

	if (l_ptr && (l_ptr->flags2 & LF2_NO_TELE)) return(FALSE);
	if (in_sector000(&p_ptr->wpos) && (sector000flags2 & LF2_NO_TELE)) return(FALSE);
	//if (l_ptr && (l_ptr->flags1 & LF1_NO_MAGIC)) return(FALSE);

	/* Space/Time Anchor */
	if (check_st_anchor(&p_ptr->wpos, p_ptr->py, p_ptr->px)) return(FALSE);

	/* It's okay */
	return(TRUE);
}

/* Handles WoR
 * XXX dirty -- REWRITEME
 * bypass : ignore any normal restrictions (ironman, fear..) - this is unused actually, instead, this function performs admin checks that ignore such restrictions.
 */
static void do_recall(int Ind, bool bypass) {
	player_type *p_ptr = Players[Ind];
	char *message = NULL;
	bool pass = (p_ptr->global_event_temp & PEVF_PASS_00);

	/* Always clear exception-flag */
	p_ptr->global_event_temp &= ~PEVF_PASS_00;

	/* sorta circumlocution? */
	worldpos new_pos;
	bool recall_ok = TRUE;

	/* Disturbing! */
	disturb(Ind, 0, 0);

	/* special restriction for global events (Highlander Tournament) */
	if (sector000separation && !is_admin(p_ptr) && (
	    (!p_ptr->recall_pos.wx && !p_ptr->recall_pos.wy) ||
	    (!p_ptr->wpos.wx && !p_ptr->wpos.wy))) {
		if (!pass) {
			msg_print(Ind, "\377oA tension leaves the air around you...");
			p_ptr->redraw |= (PR_DEPTH);
			p_ptr->recall_x = p_ptr->recall_y = 0;
			return;
		}
	}

	if (!p_ptr->wpos.wz && !bypass) {
		wilderness_type *w_ptr = &wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx];
		dungeon_type *d_ptr = NULL;

		if (p_ptr->recall_pos.wz < 0 && (w_ptr->flags & WILD_F_DOWN))
			d_ptr = w_ptr->dungeon;
		else if (p_ptr->recall_pos.wz > 0 && (w_ptr->flags & WILD_F_UP))
			d_ptr = w_ptr->tower;

#ifdef OBEY_DUNGEON_LEVEL_REQUIREMENTS
		if (d_ptr) {
			if ((d_ptr->type && d_info[d_ptr->type].min_plev > p_ptr->lev) ||
			    (!d_ptr->type && d_ptr->baselevel <= (p_ptr->lev * 3) / 2 + 7)) {
				msg_print(Ind, "\377rAs you attempt to recall, you are gripped by an uncontrollable fear.");
				msg_print(Ind, "\377oA tension leaves the air around you...");
				p_ptr->redraw |= (PR_DEPTH);
				if (!is_admin(p_ptr)) {
					p_ptr->recall_x = p_ptr->recall_y = 0;
					set_afraid(Ind, 10);// + (d_ptr->baselevel - p_ptr->max_dlv));
					return;
				}
			}
		}
#endif
		/* Nether Realm only for Kings/Queens (currently paranoia, since NR is NO_RECALL_INTO) */
		if (d_ptr && (d_ptr->type == DI_NETHER_REALM) && !p_ptr->total_winner) {
			msg_print(Ind, "\377rAs you attempt to recall, you are gripped by an uncontrollable fear.");
			if (!is_admin(p_ptr)) {
				p_ptr->recall_x = p_ptr->recall_y = 0;
				set_afraid(Ind, 10);//+(d_ptr->baselevel-p_ptr->max_dlv));
				return;
			}
		}
	}

	/* Determine the level */
	/* recalling to surface */
	if (p_ptr->wpos.wz && !bypass) {
		struct dungeon_type *d_ptr;
		d_ptr = getdungeon(&p_ptr->wpos);

		/* Messages */
		if ((((d_ptr->flags2 & DF2_IRON) || (d_ptr->flags1 & DF1_FORCE_DOWN)) && d_ptr->maxdepth > ABS(p_ptr->wpos.wz)) ||
		    (d_ptr->flags1 & DF1_NO_RECALL) || (d_ptr->flags2 & DF2_NO_EXIT_WOR)) {
			if (!(getfloor(&p_ptr->wpos)->flags1 & LF1_IRON_RECALL)) {
				msg_print(Ind, "You feel yourself being pulled toward the surface!");
				if (!is_admin(p_ptr)) {
					recall_ok = FALSE;
					/* Redraw the depth(colour) */
					p_ptr->redraw |= (PR_DEPTH);
					p_ptr->recall_x = p_ptr->recall_y = 0;
				}
			} else if ((p_ptr->mode & MODE_DED_IDDC) && in_irondeepdive(&p_ptr->wpos)) {
				msg_print(Ind, "You have dedicated yourself to the Ironman Deep Dive Challenge!");
				if (!is_admin(p_ptr)) {
					recall_ok = FALSE;
					/* Redraw the depth(colour) */
					p_ptr->redraw |= (PR_DEPTH);
					p_ptr->recall_x = p_ptr->recall_y = 0;
				}
			}
		}
		if (recall_ok) {
			msg_format(Ind, "\377%cYou are transported out of %s..", COLOUR_DUNGEON, get_dun_name(p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz > 0, d_ptr, 0, FALSE));
			if (p_ptr->wpos.wz > 0) {
				message = "You feel yourself yanked downwards!";
				msg_format_near(Ind, "%s is yanked downwards!", p_ptr->name);
			} else {
				message = "You feel yourself yanked upwards!";
				msg_format_near(Ind, "%s is yanked upwards!", p_ptr->name);
			}

			/* New location */
			new_pos.wx = p_ptr->wpos.wx;
			new_pos.wy = p_ptr->wpos.wy;
			new_pos.wz = 0;
#if 0
			p_ptr->new_level_method = (istown(&new_pos) ? LEVEL_RAND : LEVEL_OUTSIDE_RAND);
#endif
			p_ptr->new_level_method = (p_ptr->wpos.wz>0 ? LEVEL_RECALL_DOWN : LEVEL_RECALL_UP);
		}
	}
	/* beware! bugs inside! (jir) (no longer I hope) */
	/* world travel */
	/* why wz again? (jir) */
	else if ((!(p_ptr->recall_pos.wz) || !(wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags & (WILD_F_UP | WILD_F_DOWN))) && !bypass) {
		int dis;

		/* We haven't mapped the target worldmap sector yet? */
		if (((!(p_ptr->wild_map[(wild_idx(&p_ptr->recall_pos)) / 8] &
		    (1U << (wild_idx(&p_ptr->recall_pos)) % 8))) &&
		    !is_admin(p_ptr) ) ||
		    /* Or we have specified no target sector or the same sector we're already in? */
		    (inarea(&p_ptr->wpos, &p_ptr->recall_pos)
		    && p_ptr->recall_x == 0 //exception for admins: Allow precise recall within same sector
		    ))
		{
			/* back to the last town (s)he visited.
			 * (This can fail if gone too far) */
			p_ptr->recall_pos.wx = p_ptr->town_x;
			p_ptr->recall_pos.wy = p_ptr->town_y;
		}

#ifdef RECALL_MAX_RANGE
		dis = distance(p_ptr->recall_pos.wy, p_ptr->recall_pos.wx,
				p_ptr->wpos.wy, p_ptr->wpos.wx);
		if (dis > RECALL_MAX_RANGE && !is_admin(p_ptr)) {
			new_pos.wx = p_ptr->wpos.wx + (p_ptr->recall_pos.wx - p_ptr->wpos.wx) * RECALL_MAX_RANGE / dis;
			new_pos.wy = p_ptr->wpos.wy + (p_ptr->recall_pos.wy - p_ptr->wpos.wy) * RECALL_MAX_RANGE / dis;
		}
		else
#endif	// RECALL_MAX_RANGE
		{
			new_pos.wx = p_ptr->recall_pos.wx;
			new_pos.wy = p_ptr->recall_pos.wy;
		}

		new_pos.wz = 0;

		/* Messages */
		message = "You feel yourself yanked sideways!";
		msg_format_near(Ind, "%s is yanked sideways!", p_ptr->name);

		/* New location */
		p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
	}

	/* into dungeon/tower */
	/* even at runlevel 2048 players may still recall..for now */
	else if (cfg.runlevel > 4 && cfg.runlevel != 2049) {
		wilderness_type *w_ptr = &wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx];
		//wilderness_type *w_ptr = &wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx];
		/* Messages */
		if (p_ptr->recall_pos.wz < 0 && w_ptr->flags & WILD_F_DOWN) {
			dungeon_type *d_ptr = wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx].dungeon;
#ifdef SEPARATE_RECALL_DEPTHS
			int d = -get_recall_depth(&p_ptr->recall_pos, p_ptr);
#endif

			/* check character/dungeon limits */
			if (-p_ptr->recall_pos.wz > w_ptr->dungeon->maxdepth)
				p_ptr->recall_pos.wz = 0 - w_ptr->dungeon->maxdepth;
			if (p_ptr->inval && -p_ptr->recall_pos.wz > 10)
				p_ptr->recall_pos.wz = -10;
#ifdef SEPARATE_RECALL_DEPTHS
			if (d > p_ptr->recall_pos.wz) p_ptr->recall_pos.wz = d;
			if ( /* ??? */
#else
			if (p_ptr->max_dlv < w_ptr->dungeon->baselevel - p_ptr->recall_pos.wz)
				p_ptr->recall_pos.wz = w_ptr->dungeon->baselevel - p_ptr->max_dlv - 1;
			//if (d_ptr->baselevel - p_ptr->max_dlv > 2) {
			if ((!d_ptr->type && d_ptr->baselevel - p_ptr->max_dlv > 2) || /* ??? */
#endif
#ifdef OBEY_DUNGEON_LEVEL_REQUIREMENTS
			    (d_ptr->type && d_info[d_ptr->type].min_plev > p_ptr->lev) ||
#endif
			    (d_ptr->flags1 & (DF1_NO_RECALL | DF1_NO_UP | DF1_FORCE_DOWN)) ||
#ifdef RPG_SERVER /* Prevent recalling into NO_DEATH dungeons */
			    (d_ptr->flags2 & (DF2_NO_DEATH)) ||
#endif
#if 0
			    (d_ptr->flags2 & (DF2_IRON | DF2_NO_RECALL_INTO)) ||
#else /* allow recalling into town dungeons every 1000 ft now? Adjusted Moltor's suggestion */
			    (d_ptr->flags2 & (DF2_NO_RECALL_INTO)) ||
			    ((d_ptr->flags2 & DF2_IRON) && wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx].type != WILD_TOWN) ||
			    ((d_ptr->flags2 & DF2_IRON) && ((p_ptr->recall_pos.wz + 1) % 20)) ||
#endif
			    (d_ptr->flags2 & DF2_NO_ENTRY_WOR))
			{
				if (!is_admin(p_ptr))
					p_ptr->recall_pos.wz = 0;
				else if (p_ptr->recall_pos.wz != 0) /* got zero already from get_max_depth()? then don't give 'nowhere' msg twice */
					msg_print(Ind, "You feel yourself yanked toward nowhere...");
			}

			if (p_ptr->recall_pos.wz >= 0) {
				p_ptr->recall_pos.wz = 0;
			} else {
				message = "You feel yourself yanked downwards!";
				msg_format(Ind, "\377%cYou are transported into %s..", COLOUR_DUNGEON, get_dun_name(p_ptr->recall_pos.wx, p_ptr->recall_pos.wy, FALSE, d_ptr, 0, FALSE));
				msg_format_near(Ind, "%s is yanked downwards!", p_ptr->name);
			}
		}
		else if (p_ptr->recall_pos.wz > 0 && w_ptr->flags & WILD_F_UP) {
			dungeon_type *d_ptr = wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx].tower;
#ifdef SEPARATE_RECALL_DEPTHS
			int d = get_recall_depth(&p_ptr->recall_pos, p_ptr);
#endif

			/* check character/tower limits */
			if (p_ptr->recall_pos.wz > w_ptr->tower->maxdepth)
				p_ptr->recall_pos.wz = w_ptr->tower->maxdepth;
			if (p_ptr->inval && p_ptr->recall_pos.wz > 10)
				p_ptr->recall_pos.wz = 10;
#ifdef SEPARATE_RECALL_DEPTHS
			if (d < p_ptr->recall_pos.wz) p_ptr->recall_pos.wz = d;
			if ( /* ??? */
#else
			if (p_ptr->max_dlv < w_ptr->tower->baselevel + p_ptr->recall_pos.wz + 1)
				p_ptr->recall_pos.wz = 0 - w_ptr->tower->baselevel + p_ptr->max_dlv + 1;
			//if (d_ptr->baselevel - p_ptr->max_dlv > 2) {
			if ((!d_ptr->type && d_ptr->baselevel-p_ptr->max_dlv > 2) || /* ??? */
#endif
#ifdef OBEY_DUNGEON_LEVEL_REQUIREMENTS
			    (d_ptr->type && d_info[d_ptr->type].min_plev > p_ptr->lev) ||
#endif
			    (d_ptr->flags1 & (DF1_NO_RECALL | DF1_NO_UP | DF1_FORCE_DOWN)) ||
#ifdef RPG_SERVER /* Prevent recalling into NO_DEATH towers */
			    (d_ptr->flags2 & (DF2_NO_DEATH)) ||
#endif
#if 0
			    (d_ptr->flags2 & (DF2_IRON | DF2_NO_RECALL_INTO)) ||
#else /* allow recalling into town dungeons every 1000 ft now? Adjusted Moltor's suggestion */
			    (d_ptr->flags2 & (DF2_NO_RECALL_INTO)) ||
			    ((d_ptr->flags2 & DF2_IRON) && wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx].type != WILD_TOWN) ||
			    ((d_ptr->flags2 & DF2_IRON) && ((p_ptr->recall_pos.wz - 1) % 20)) ||
#endif
			    (d_ptr->flags2 & DF2_NO_ENTRY_WOR))
			{
				if (!is_admin(p_ptr))
					p_ptr->recall_pos.wz = 0;
				else if (p_ptr->recall_pos.wz != 0) /* got zero already from get_max_depth()? then don't give 'nowhere' msg twice */
					msg_print(Ind, "You feel yourself yanked toward nowhere...");
			}

			if (p_ptr->recall_pos.wz <= 0) {
				p_ptr->recall_pos.wz = 0;
			} else {
				message = "You feel yourself yanked upwards!";
				msg_format(Ind, "\377%cYou are transported into %s..", COLOUR_DUNGEON, get_dun_name(p_ptr->recall_pos.wx, p_ptr->recall_pos.wy, TRUE, d_ptr, 0, FALSE));
				msg_format_near(Ind, "%s is yanked upwards!", p_ptr->name);
			}
		} else {
			p_ptr->recall_pos.wz = 0;
		}

		/* prevent 'cross-recalling' except for admins (ie change of x,y and z at once) */
		if (p_ptr->recall_pos.wz && !is_admin(p_ptr) &&
		    (p_ptr->wpos.wx != p_ptr->recall_pos.wx || p_ptr->wpos.wy != p_ptr->recall_pos.wy)) {
			p_ptr->recall_pos.wx = p_ptr->wpos.wx;
			p_ptr->recall_pos.wy = p_ptr->wpos.wy;
			p_ptr->recall_pos.wz = 0;
		}

		new_pos.wx = p_ptr->recall_pos.wx;
		new_pos.wy = p_ptr->recall_pos.wy;
		new_pos.wz = p_ptr->recall_pos.wz;
		if (p_ptr->recall_pos.wz == 0) {
			message = "You feel yourself yanked toward nowhere...";
			p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
		}
		else p_ptr->new_level_method = LEVEL_RAND;
	} else {
		/* new_pos isn't set so recalling shouldn't be allowed - mikaelh */
		recall_ok = FALSE;
		p_ptr->redraw |= (PR_DEPTH);
		p_ptr->recall_x = p_ptr->recall_y = 0;
	}

	if (recall_ok) {
		/* back into here */
		wpcopy(&p_ptr->recall_pos, &new_pos);
		recall_player(Ind, message);
	}
}

/* Does player have ball? */
int has_ball (player_type *p_ptr) {
	int i;
	for (i = 0; i < INVEN_WIELD; i++) {
		if (p_ptr->inventory[i].tval == TV_GAME && p_ptr->inventory[i].sval == SV_GAME_BALL)
			break;
	}
	if (i == INVEN_WIELD) i = -1;
	return(i);
}

int food_consumption_legacy(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i, j;

	i = (10 + (extract_energy[p_ptr->pspeed] / 10) * 3) / 2;

	/* Adrenaline takes more food */
	if (p_ptr->adrenaline) i *= 5;

	/* Biofeedback takes more food */
	if (p_ptr->biofeedback) i *= 2;

	/* Regeneration and extra-growth takes more food */
	if (p_ptr->regenerate || p_ptr->xtrastat_tim) i += 30;

	/* Regeneration (but not Nether Sap) takes more food */
	if (p_ptr->tim_regen && p_ptr->tim_regen_pow > 0) i += p_ptr->tim_regen_pow / 10;

	j = 0;

	/* Mimics need more food if sustaining heavy forms */
	if (p_ptr->body_monster && r_info[p_ptr->body_monster].weight > 180)
		j = 15 - 7500 / (r_info[p_ptr->body_monster].weight + 320);//180:0, 260:2, 500:~5, 1000:~9, 5000:14, 7270:15

	/* Draconian and Half-Troll take more food */
	if (p_ptr->prace == RACE_DRACONIAN
	    || p_ptr->prace == RACE_HALF_TROLL) j = 15;

	/* Use either mimic form induced food consumption increase,
	   or intrinsic one, depending on which is higher. */
	i += j;

	/* Vampires consume food very quickly,
	   but old vampires don't need food frequently */
	if (p_ptr->prace == RACE_VAMPIRE) {
		if (p_ptr->lev >= 40) i += 60 / (p_ptr->lev - 37);
		else i += 20;
	}

	/* Invisibility consume a lot of food */
	i += p_ptr->invis / 2;

	/* Invulnerability consume a lot of food */
	if (p_ptr->invuln) i += 40;

	/* Wraith Form consume a lot of food */
	if (p_ptr->tim_wraith) i += 30;

	/* Hitpoints multiplier consume a lot of food */
	if (p_ptr->to_l) i += p_ptr->to_l * 5;

	/* Slow digestion takes less food */
	//if (p_ptr->slow_digest) i -= 10;
	if (p_ptr->slow_digest) i -= (i > 40) ? i / 4 : 10;

	return(i);
}

int food_consumption(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Basic digestion is '20', assuming normal speed (+0) */
	int i = 20, j;

	/* ---------- Weight vs form weight scaling of the base value ---------- */
	/* Could consider making this multiplicative same as speed-scaling, but might be too harsh in regards to player race choice. */

	/* Form vs race-intrinsic malus, stronger one overrides */

	/* Mimics need more food if sustaining heavy forms.
	   Note: No 'bonus' for light forms, as it might even be wraiths that weigh zero, wouldn't make sense.
	   However! Hidden food relief bonus for player races that weigh > 180 lbs using a monster form that weighs up to 180 lbs,
	    this way the player may still save some food by polymorphing into a 'non-heavy' form! */
	if (p_ptr->body_monster && r_info[p_ptr->body_monster].weight > 180)
		i += 3 * (15 - 7500 / (r_info[p_ptr->body_monster].weight + 320)); //180:0, 260:2, 500:~5, 1000:~9, 5000:14, 7270:15  -> *3 = +0..+45 */

	/* If not in monster form, use own weight to determine food consumption.
	   Special exception for Ents, who consume just a moderate baseline amount, allowing them a 'feeding rate' similar to humans.
	   We use the male baseline value from tables.c to not force players to create&reroll female chars to minimize food consumption -_-. - C. Blue */
	else if (!p_ptr->body_monster && p_ptr->prace != RACE_ENT) {
		if (p_ptr->rp_ptr->m_b_wt > 180) i += (p_ptr->rp_ptr->m_b_wt - 180) / 10; //255 (Draconian) or 250 (Half-Troll) +7, (note: both get extra penalized later: for breath/regen)
		else if (p_ptr->rp_ptr->m_b_wt < 180) i -= (180 - p_ptr->rp_ptr->m_b_wt) / 15;// 145 (Elf) -2, 130 (Yeek) -3, 90 (Gnome) -6, 60 (Hobbit) -8, 45 (Kobold) -9!
	}


	/* ---------- Additives ---------- */

	/* Vampires consume food very quickly, unless in bat/mist form (+2..10),
	   but old vampires don't need food frequently (+4...20) */
	if (p_ptr->prace == RACE_VAMPIRE) {
		if (p_ptr->lev >= 40) j = 60 / (p_ptr->lev - 37);
		else j = 20;
		if (p_ptr->body_monster) j /= 2;
		i += j;
	}

	/* Note: adrenaline and biofeedback are mutually exclusive, gaining one will terminate the other. */

	/* Adrenaline takes more food */
	if (p_ptr->adrenaline) i += 60; // might need balancing at very high character speed - C. Blue

	/* Biofeedback takes more food */
	if (p_ptr->biofeedback) i += 30;

#ifdef ENABLE_DRACONIAN_TRAITS
	/* Draconians' breath/element effects take extra food */
	if (p_ptr->prace == RACE_DRACONIAN) {
		if (p_ptr->ptrait == TRAIT_RED) i += 5; /* Don't double-penalise this lineage for their intrinsic 'regenerate', which gets +15 further down. */
		else i += 10;
	}
#endif

	/* Regeneration and extra-growth takes more food. (Intrinsic) super-regen takes a large amount of food. */
#if defined(TROLL_REGENERATION) || defined(HYDRA_REGENERATION)
	switch (troll_hydra_regen(p_ptr)) {
	case 1: i += 20; break;
	case 2: i += 25; break;
	}
#endif
	if (p_ptr->regenerate || p_ptr->xtrastat_tim) i += 15;
	/* Other stat-boosting effects: */
	if (p_ptr->shero || p_ptr->fury) i += 20;
	else if (p_ptr->hero) i += 10;

	/* Non-magical regeneration burns enormously more food temporarily: Fast metabolism! */
	if (p_ptr->tim_regen &&
	    p_ptr->tim_regen_pow > 0 && /* (and definitely not Nether Sap, anyway) */
	    p_ptr->tim_regen_cost) /* non-magical only, ie via shroom of fast metabolism */
		i += 30;

	/* Hitpoints multiplier consume significantly much food (+0..15) */
	if (p_ptr->to_l) i += p_ptr->to_l * 5;

	/* Invisibility consume a lot of food (+0..20, +40 for potion of invis) */
	i += p_ptr->invis / 2;

	/* Wraith Form consumes a lot of food */
	if (p_ptr->tim_wraith) i += 30;

	/* Invulnerability consume a lot of food */
	if (p_ptr->invuln) i += 40;


	/* ---------- Time scaling based on speed ---------- */

	/* Modify digestion rate based on speed */
#if 1 /* Actually this seems fine! */
	if (p_ptr->pspeed >= 110) i = (i * (15 + extract_energy[p_ptr->pspeed] / 10)) / 25; // 'fast': 1 (normal) .. x2.5~3 (end-game fast) .. x3.8 (fastest)
	else i = (i * (3 + extract_energy[p_ptr->pspeed] / 10)) / 13; // 'slow': 1/3 (slowest) .. 1 (normal)
#else // require more food at higher speed? (WiP)
	if (p_ptr->pspeed >= 110) i = (i * (10 + ((extract_energy[p_ptr->pspeed] * 3) / 10))) / 40; // 'fast': 1 (normal) .. x4.6 (end-game fast) .. x6.25 (fastest)
	else i = (i * (3 + extract_energy[p_ptr->pspeed] / 10)) / 13; // 'slow': 1/3 (slowest) .. 1 (normal)
#endif

	/* ---------- Reductions ---------- */

	/* Slow digestion takes 1/4 less food (required nutrition gets rounded up) */
	if (p_ptr->slow_digest) i = (i * 3 + 3) / 4;

	return(i);
}

/*
 * Handle misc. 'timed' things on the player.
 * returns FALSE if player no longer exists.
 *
 * TODO: find a better way for timed spells(set_*).
 *
 * Called every 5/6 of a player's dungeon turn, see DUN_TURN_56() macro.
 */
static bool process_player_end_aux(int Ind) {
	player_type	*p_ptr = Players[Ind];
	cave_type **zcave, *c_ptr;
	object_type	*o_ptr;
	int		i, j, k, autofood = 0;
	int		regen_amount; //, NumPlayers_old = NumPlayers;
	struct worldpos *wpos = &p_ptr->wpos;
	dun_level	*l_ptr = getfloor(wpos);
	dungeon_type	*d_ptr = getdungeon(wpos);
	char		o_name[ONAME_LEN];
	bool		warm_place = TRUE;
#if defined(TROLL_REGENERATION) || defined(HYDRA_REGENERATION)
	bool		intrinsic_regen = FALSE;
#endif
	bool		timeout_handled;
	bool		town = istown(wpos), townarea = istownarea(wpos, MAX_TOWNAREA), dungeontown = isdungeontown(wpos);

	int minus = 1;
	int minus_magic = 1;
	int minus_health = get_skill_scale_fine(p_ptr, SKILL_HEALTH, 2); /* was 3, but then HEALTH didn't give HP.. */
	int minus_combat = get_skill_scale_fine(p_ptr, SKILL_COMBAT, 3);

	/* form-intrinsic AM (DISBELIEVE) must be punished in the same manner or it'd be a too inconsistent advantage */
	if (p_ptr->body_monster && r_info[p_ptr->body_monster].flags7 & RF7_DISBELIEVE) {
		j = (2 * ANTIMAGIC_CAP) / 50; //scale factor: intrinsic AM could go above the max obtainable from pure skill
		i = r_info[p_ptr->body_monster].level / 2 + 10;
		i = i + get_skill(p_ptr, SKILL_ANTIMAGIC);
		if (i > ANTIMAGIC_CAP) i = ANTIMAGIC_CAP;
		minus_magic += (i * j) / ANTIMAGIC_CAP + (magik(((i * j * 100) / ANTIMAGIC_CAP) % 100) ? 1 : 0);
	} else minus_magic += get_skill_scale_fine(p_ptr, SKILL_ANTIMAGIC, 2); /* was 3 before, trying slightly less harsh 2 now */

	if (!(zcave = getcave(wpos))) return(FALSE);
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* Anything done here cannot be reduced by GoI/Manashield etc */
	bypass_invuln = TRUE;


	/*** Damage over Time ***/
#define POISON_DIV 30
#define CUT_DIV 75
#define CUT_HEAL_SPEED 50 /* [1..100] Speed in % at which cuts heal, skipping a heal-turn at (100-this)% probability. */


	/* Take damage from poison */
	if (p_ptr->poisoned) {
		k = p_ptr->mhp / POISON_DIV;
		k += (rand_int(POISON_DIV) < p_ptr->mhp % POISON_DIV) ? 1 : 0;
		if (!k) k = 1;

		if (p_ptr->slow_poison) {
			p_ptr->slow_poison = (p_ptr->slow_poison % 3) + 1; /* Take damage every n-th tick only [3] */
			if (p_ptr->slow_poison == 1) {
				p_ptr->died_from_ridx = 0;
				take_hit(Ind, k, "poison", p_ptr->poisoned_attacker);
			}
		} else {
			/* Take damage */
			p_ptr->died_from_ridx = 0;
			take_hit(Ind, k, "poison", p_ptr->poisoned_attacker);
		}
	}
	/* Suffer from disease - basically same amount of damage and duration as poison except it cannot be cured as easily */
	if (p_ptr->diseased) {
		k = p_ptr->mhp / POISON_DIV;
		k += (rand_int(POISON_DIV) < p_ptr->mhp % POISON_DIV) ? 1 : 0;
		if (!k) k = 1;

		/* Take damage */
		p_ptr->died_from_ridx = 0;
		take_hit(Ind, k, "disease", p_ptr->poisoned_attacker);
	}

	/* Take damage from cuts */
	if (p_ptr->cut) {
		k = p_ptr->mhp / CUT_DIV;
		k += (rand_int(CUT_DIV) < p_ptr->mhp % CUT_DIV) ? 1 : 0;
		if (!k) k = 1;

		if (p_ptr->cut >= CUT_MORTAL_WOUND) i = 7;	/* Mortal wound */
		if (p_ptr->cut >= CUT_DEEP_GASH) i = 6;		/* Deep gash */
		if (p_ptr->cut >= CUT_SEVERE_CUT) i = 5;	/* Severe cut */
		if (p_ptr->cut >= CUT_NASTY_CUT) i = 4;		/* Nasty cut */
		if (p_ptr->cut >= CUT_BAD_CUT) i = 3;		/* Bad cut */
		if (p_ptr->cut >= CUT_LIGHT_CUT) i = 2;		/* Light cut */
		else i = 1; /* CUT_GRAZE:			   Graze */

		/* Take damage */
		p_ptr->died_from_ridx = 0;
		take_hit(Ind, i * k, "a fatal wound", p_ptr->cut_attacker);
	}

	/* Misc. terrain effects */
	if (!p_ptr->ghost) { /* Spare dead players, even though levitation usually does NOT fully protect eg from lava fire damage? */
		/* Generic terrain effects */
		apply_terrain_effect(Ind);

		/* Drowning, but not ghosts */
		if (feat_is_deep_water(c_ptr->feat)) {
			/* Rewrote this whole routine to take into account DSMs, wood helping thanks to its relative density, subinventories etc. - C. Blue */
			if (!p_ptr->tim_wraith && !p_ptr->levitate) { /* Wraiths and levitating players are completely unaffected by water, including their items */
				bool huge_wood = FALSE, cold = cold_place(wpos), is_ent = (p_ptr->prace == RACE_ENT && !p_ptr->body_monster);
				int wood_weight, water_weight, required_wood_weight, drowning;

				/* Calculate actual weight dragging us down and amount of wood pulling us up */
				wood_weight = 0;
				water_weight = (p_ptr->can_swim ? 0 : p_ptr->wt * 10) + p_ptr->total_weight; //(is_ent implies can_swim)

				if (is_ent) {
					huge_wood = TRUE;
					wood_weight += p_ptr->wt * 10;
				}

				j = -1;
				for (i = 0; i < INVEN_PACK; i++) {
					o_ptr = &p_ptr->inventory[i];
					if (!o_ptr->k_idx) break;

#ifdef ENABLE_SUBINVEN
					ppea_subinv: /* sigh - I really don't want to add a helper function just for the contents of this loop -_- */
					if (j != -1) {
						o_ptr = &p_ptr->subinventory[i][j];
						if (!o_ptr->k_idx) {
							j = -1;
							continue;
						}
					}
#endif

					/* DSMs have neutral buoyancy */
					if (o_ptr->tval == TV_DRAG_ARMOR) water_weight -= o_ptr->weight * o_ptr->number;

					/* Wood helps, massive piece is an instant game changer - except for people CRAZILY overloaded perhaps.
					   It also helps against backpack taking damage now, as we can put our backpack on it, or keep ourselves out of the water far enough maybe ^^. */
					if (o_ptr->tval == TV_GOLEM && o_ptr->sval == SV_GOLEM_WOOD) huge_wood = TRUE;
					/* Wood helps us, but note that we shouldn't disregard that wooden weapons often also contain a significant amount of metal that would counteract the effect.
					   Trying to alleviate this a bit by checking whether it's a weapon, but that in turn will add mistakes if the weapon indeed was purely made of wood (club?) ^^.
					   Well, we'll just have to accept the approximations. */
					if (contains_significant_wood(o_ptr)) {
						if (is_melee_weapon(o_ptr->tval)) {
							water_weight -= (o_ptr->number * o_ptr->weight * 2) / 3;
							wood_weight += (o_ptr->number * o_ptr->weight * 2) / 3;
						} else {
							water_weight -= o_ptr->number * o_ptr->weight;
							wood_weight += o_ptr->number * o_ptr->weight; // massive piece: 1100, piece: 40
						}
					}

#ifdef ENABLE_SUBINVEN
					if (j != -1) {
						/* Process next item in subinventory */
						j++;
						if (j == o_ptr->bpval) {
							j = -1;
							continue;
						}
						goto ppea_subinv;
					} else if (o_ptr->tval == TV_SUBINVEN) { // assumption: There can be no subinventories inside subinventories!
						/* Process items in subinventory, starting with the first item */
						j = 0;
						goto ppea_subinv;
					}
#endif
				}

				/* Calculate required amount of wood we need to carry in order to make us float safely. */
				required_wood_weight = water_weight * WOOD_INV_DENSITY;
				drowning = required_wood_weight - wood_weight; /* If we're >=0 then we're not drowning as the wood we carry is enough to keep us floating */

				/* Aquatic monsters and Ents never drown */
				if ((p_ptr->body_monster && (r_info[p_ptr->body_monster].flags7 & RF7_AQUATIC)) || is_ent) /* They might not be able to can_swim if overburdened, but they just don't drown anyway */
					drowning = -1;

				/* We're drowning because we don't have enough wood to keep us afloat? */
				if (drowning >= 0) {
					int swim;

					/* --- Now test if we can 'swim' somehow, which nullifies 'hit'/ --- */

					/* Swimming skill:
					   Note that race weight is byte and max used is actually 255 (Ent/Draconian).
					   Maxed out swimming, therefore we must be able to at least sustain 255 lbs =p,
					   plus some extra weight we're carrying with us.
					   Normal minimum (no items) drowning at 0 wood, relative water density 0.5, depending on race: 1000 (yeek) .. 5100 (Ent/Draconian)

					   Ensure that the swimming skill (also unrealistically) doesn't create insane extra value for small/light races:
					   So we let it depend on our weight or size or both:
					   Size aka p_ptr->ht goes from 33 (Hobbit) to 180 (Draconian/Ent) and is byte,
					   weight aka p_ptr->wt goes from 50 (Hobbit) to 255 (Draconian/Ent) and is byte too.

					   Note: Typical inven for fighters is super roughly (depends on mdevs vs potions/scrolls) ~200 lbs,
					         equipment ~60 lbs (or 80 lbs with Mattock) = ~2600 _extra_ object weight just form items.
					         With real-life endgame stuff, can be more like total of ~330 lbs for fighter chars.
					         Light chars (Dodgers) can be 120..160 lbs. Mages however may hit 200 lbs from mana potions and books,
					         their mage staff is 9.0 and wooden though, offsetting the remaining ~15-20 lbs equipment. */
					/* - Light race has it easier to swim in general:
					     Scale so heavy races (16.500 swimming) need to train swimming more than light races (6.000 swimming) to carry themselves ^^.
					   - Heavy race can sustain more total armour (~38%) at maxed swimming in general:
					     Scale so heavy races can still/barely have full armour at max swimming skill, while light races can sustain somewhat less armour weight :) */
					swim = get_skill_scale(p_ptr, SKILL_SWIM, 850 * 50);
					//swim = ((r_info[p_ptr->body_monster].flags7 & RF7_CAN_SWIM) && swim < 500) ? 500 : swim;
					//swim += 390 - (adj_chr_gold[p_ptr->stat_ind[A_STR]] + adj_chr_gold[p_ptr->stat_ind[A_DEX]] * 2); //0..150
					swim += (adj_str_wgt[p_ptr->stat_ind[A_STR]] * 2 + adj_str_wgt[p_ptr->stat_ind[A_DEX]] * 4) - 30; //0..150 too :)
					/* Balance vs total character weight: This supports around 280 lbs total weight as maximum without drowning. */
					swim = (swim * 3) / 4;
					/* Try to counter drowning with swimming capabilities. */
					drowning = drowning / WOOD_INV_DENSITY - (swim * (p_ptr->ht + p_ptr->wt + 350)) / 100; //also use 'ht': It's disadvantageous to have bad BMI! =p

					/* If we're STILL drowning, we take damage */
					if (drowning >= 0) {
						int hit;

						/* Drowning damage per drowning 'tick' */
						hit = (p_ptr->mhp >> 6) + randint(p_ptr->mhp >> 5);
						/* Take CON into consideration (max 30%) */
						hit = (hit * (80 - adj_str_wgt[p_ptr->stat_ind[A_CON]])) / 75;
						/* Now regarding RACE_VAMPIRE - they cannot drown, but they take damage from 'running water'.
						   So I decided on middle grounds and just assume all rivers, oceans, ponds and even puddles (fortunately these
						   are usually FEAT_SHAL_WATER though) are 'running water' and just reduce the drowning damage somewhat. - C. Blue */
						if (p_ptr->suscep_life) hit >>= 1;
						else if (p_ptr->demon) hit >>= 1;
						/* Always take damage? */
						if (!hit) hit = rand_int(3) ? 0 : 1;

						if (hit) {
							/* Freezing place, add extra cold damage? */
							if (cold && !p_ptr->immune_cold) {
								i = hit;
								if (p_ptr->resist_cold) i = (i + 2) / 3;
								if (p_ptr->oppose_cold) i = (i + 2) / 3;
								hit += i;
#ifdef TEST_SERVER
								msg_format(Ind, "\377rYou're drowning in freezing water for %d damage!", hit);
							} else msg_format(Ind, "\377rYou're drowning for %d damage!", hit);
#else
								msg_print(Ind, "\377rYou're drowning in freezing water!");
							} else msg_print(Ind, "\377rYou're drowning!");
#endif
							take_hit(Ind, hit, "drowning", 0);
						}
					}

#if 0 /* would require hacking the stat-autorestoration duration to be much shorter for this, or it's too annoying */
					/* Swimming, whether successful or not, can drain STR. */
					if (!rand_int(adj_str_wgt[p_ptr->stat_ind[A_CON]]) && !p_ptr->sustain_str) {
						msg_print(Ind, "\377oYou are weakened by the exertion of swimming!");
						dec_stat(Ind, A_STR, 10, STAT_DEC_TEMPORARY);
					}
#else /* this is a more elegant solution perhaps? */
					/* Swimming, whether successful or not, can drain ST. */
					if (p_ptr->cst) {
						msg_print(Ind, "\377oYou are exhausted by the exertion of swimming!");
						p_ptr->cst--;
						p_ptr->redraw |= PR_STAMINA;
					}
#endif
				}

				/* If we're, after all (enough wood and/or enough swiming or aquatic/Ent) not drowning... */
				if (drowning < 0) {
					/* Special flavour message when not drowning and carrying a huge block of wood */
					if (huge_wood && !is_ent && !rand_int(5)) {
						/* Give specific message for flavour, about clinging to the massive piece of wood, being our 'main relief' ;) */
						msg_print(Ind, "You float in the water safely, clinging to the wood.");
					}

					/* Freezing place, still take cold damage? */
					if (cold && !p_ptr->immune_cold) {
						/* Same damage calc as for drowning -> consistent with the added cold damage when drowning in freezing water: */
						i = (p_ptr->mhp >> 6) + randint(p_ptr->mhp >> 5);
						if (p_ptr->resist_cold) i = (i + 2) / 3;
						if (p_ptr->oppose_cold) i = (i + 2) / 3;
#ifdef TEST_SERVER
						msg_format(Ind, "\377rYou're freezing for %d damage!", i);
#else
						msg_print(Ind, "\377rYou're freezing!");
#endif
						take_hit(Ind, i, "freezing water", 0);
					}
				}

				/* Harm items while in water (even if we're not drowning/taking damage, ie hit == 0).
				   Items should get damaged even if player can_swim
				   but this might devalue swimming too much compared to levitation. */
				if (!p_ptr->immune_water && (!p_ptr->resist_water || !rand_int(3))
				    && !(p_ptr->body_monster && (r_info[p_ptr->body_monster].flags7 & RF7_AQUATIC))
				    && magik(WATER_ITEM_DAMAGE_CHANCE)) {
					/* Trying to keep the backpack from getting soaked too much */
					if (TOOL_EQUIPPED(p_ptr) != SV_TOOL_TARPAULIN
					    && !(huge_wood && drowning < 0)
					    && !magik(get_skill_scale(p_ptr, SKILL_SWIM, 4900))) {
						/* Apply water damage to inventory. If it has no effect (so we don't harm more than 1 slot at a time here),
						   but we're in a freezing place, apply cold damage to inventory too. */
						if (!inven_damage(Ind, set_water_destroy, 1) && cold) inven_damage(Ind, set_cold_destroy, 1);
					}
					/* Can't prevent our body being inside the water though */
					equip_damage(Ind, GF_WATER);
				}
			}
		}
		/* Aquatic anoxia */
		else if ((p_ptr->body_monster) &&
		    ((r_info[p_ptr->body_monster].flags7 & RF7_AQUATIC) &&
		    !(r_info[p_ptr->body_monster].flags3 & RF3_UNDEAD))
		    && (!feat_is_shal_water(c_ptr->feat) ||
		    r_info[p_ptr->body_monster].weight > 700)
		    /* new: don't get stunned from crossing door/stair grids every time - C. Blue */
		    && !(is_always_passable(c_ptr->feat) && (c_ptr->info & CAVE_WATERY))
		    && !p_ptr->tim_wraith) {
			long hit = p_ptr->mhp >> 6; /* Take damage */

			hit += randint(p_ptr->chp >> 5);
			/* Take CON into consideration(max 30%) */
			hit = (hit * (80 - adj_str_wgt[p_ptr->stat_ind[A_CON]])) / 75;
			if (!hit) hit = 1;

			if (hit) {
				if (!feat_is_shal_water(c_ptr->feat))
					msg_print(Ind, "\377rYou cannot breathe air!");
				else
					msg_print(Ind, "\377rThere's not enough water to breathe!");
			}

			if (randint(1000) < 10) {
				msg_print(Ind, "\377rYou find it hard to stir!");
				//do_dec_stat(Ind, A_DEX, STAT_DEC_TEMPORARY);
				dec_stat(Ind, A_DEX, 10, STAT_DEC_TEMPORARY);
			}

			/* very important to actually enforce it, otherwise
			   it won't matter at all, because during a fight
			   you quaff healing potions anyway.
			   if you change the amount, compare it with continuous
			   stun reduction so it won't get neutralized by high combat skill. */
			if (p_ptr->stun < 20) set_stun_raw(Ind, p_ptr->stun + 5);

			take_hit(Ind, hit, "anoxia", 0);
		}

		/* Spectres -- take damage when moving through walls */

		/*
		 * Added: ANYBODY takes damage if inside through walls
		 * without wraith form -- NOTE: Spectres will never be
		 * reduced below 0 hp by being inside a stone wall; others
		 * WILL BE!
		 */
		/* Seemingly the comment above is wrong, dunno */
		if (!cave_floor_bold(zcave, p_ptr->py, p_ptr->px)
		    && !cave_passable(zcave, p_ptr->py, p_ptr->px)) {
			/* Player can walk through trees */
			//if ((PRACE_FLAG(PR1_PASS_TREE) || (get_skill(SKILL_DRUID) > 15)) && (cave[py][px].feat == FEAT_TREE))
#if 0
			if ((p_ptr->pass_trees || p_ptr->levitate) &&
					(c_ptr->feat == FEAT_TREE))
#endif
			if (player_can_enter(Ind, c_ptr->feat, FALSE)) {
				/* Do nothing */
				//take damage like if from hp-drain? (old way)
			}
			else if (c_ptr->feat == FEAT_HOME) {/* rien */}
			//else if (PRACE_FLAG(PR1_SEMI_WRAITH) && (!p_ptr->wraith_form) && (f_info[cave[py][px].feat].flags1 & FF1_CAN_PASS))
			else if (!p_ptr->tim_wraith &&
			    !p_ptr->master_move_hook) { /* Hack -- builder is safe */
				//int amt = 1 + ((p_ptr->lev) / 5) + p_ptr->mhp / 100;
				/* Currently it only serves to reduce 'stuck' players' HP,
				   so we might lower it a bit - C. Blue */
				int amt = 1 + ((p_ptr->lev) / 10) + p_ptr->mhp / 100;
				int amt2 = p_ptr->mmp / 35;

				/* hack: disruption shield suffers a lot if it's got to withstand the walls oO */
				if (p_ptr->tim_manashield) {
					amt += amt2;

					/* Be nice and don't let the disruption shield dissipate */
					if (p_ptr->pclass == CLASS_MAGE) {
						if (amt > p_ptr->cmp - 1) amt = p_ptr->cmp - 1;
					} else {
						/* Other classes take double damage */
						if (2 * amt > p_ptr->cmp - 1) amt = (p_ptr->cmp - 1) / 2;
					}

					bypass_invuln = FALSE; /* Disruption shield protects from this type of damage */
					take_hit(Ind, amt, " walls ...", 0);
					bypass_invuln = TRUE;
				} else {
					if (amt > p_ptr->chp - 1) amt = p_ptr->chp - 1;
					take_hit(Ind, amt, " walls ...", 0);
				}
			}
		}

		/* specialty for Cloud Planes: continuous icy winds */
		if (d_ptr && d_ptr->type == DI_CLOUD_PLANES) {
#if 0 /* questionable - same as floor terrain effects? (kills potions) */
			project(PROJECTOR_TERRAIN, 0, wpos, p_ptr->py, p_ptr->px, randint(20), GF_COLD,
			    PROJECT_NORF | PROJECT_KILL | PROJECT_HIDE | PROJECT_JUMP | PROJECT_NODO | PROJECT_NODF, "freezing winds");
#else /* seems better/cleaner */
			if (!p_ptr->immune_cold) {
				int dam;
				bool destroy = FALSE; /* note: inven damage should be same probability/extent, with or without <res xor opp>! */

				if (p_ptr->resist_cold && p_ptr->oppose_cold) dam = randint(10);
				else if (p_ptr->resist_cold || p_ptr->oppose_cold) {
					dam = randint(20);
					if (!rand_int(3)) destroy = TRUE;
				} else {
					dam = randint(40);
					if (!rand_int(3)) destroy = TRUE;
				}
				msg_format(Ind, "You are hit by freezing winds for \377o%d\377w damage.", dam);
				bypass_invuln = FALSE; /* Disruption shield protects from this type of damage */
				take_hit(Ind, dam, "freezing winds", 0);
				bypass_invuln = TRUE;
				if (destroy) inven_damage(Ind, set_cold_destroy, 1);
			}
#endif
		}
	}


	/*** Check the Food, and Regenerate ***/
	/* Ent's natural food while in 'Resting Mode' - C. Blue
	   Water helps much, natural floor helps some. */
	if (!p_ptr->ghost && p_ptr->prace == RACE_ENT && p_ptr->resting) {
		if ((feat_is_water(c_ptr->feat) && c_ptr->feat != FEAT_TAINTED_WATER) || c_ptr->feat == FEAT_MUD)
			autofood = 200; //Delicious!
		else if (c_ptr->feat == FEAT_GRASS || c_ptr->feat == FEAT_DIRT)
			autofood = 100;
		else if (c_ptr->feat == FEAT_BUSH || c_ptr->feat == FEAT_TREE)
			autofood = 70;
		else
			autofood = 0;

		if (autofood > 0 && set_food(Ind, p_ptr->food + autofood)) {
			msg_print(Ind, "You gain some nourishment from around you.");
#if 0 /* spammy in town */
			switch (autofood) {
			case 70:
				msg_format_near(Ind, "\374\377wYou hear strange sounds coming from the direction of %s.", p_ptr->name);
				break;
			case 100:
				msg_format_near(Ind, "\374\377w%s digs %s roots deep into the ground.", p_ptr->name, (p_ptr->male ? "his" : "her"));
				break;
			case 200:
				msg_format_near(Ind, "\374\377w%s absorbs all the water around %s.", p_ptr->name, (p_ptr->male ? "him" : "her"));
			}
#endif
		}
	}
	/* Ghosts don't need food */
	/* Allow AFK-hivernation if not hungry */
	else if (!p_ptr->ghost && !(p_ptr->afk && p_ptr->food >= PY_FOOD_ALERT) && !p_ptr->admin_dm &&
	    p_ptr->paralyzed <= cfg.spell_stack_limit && /* Hack for forced stasis - also prevents damage from starving badly */
	    /* Don't starve in town (but recover from being gorged) - C. Blue */
	    (!(townarea || dungeontown || safe_area(Ind) || (c_ptr->info2 & CAVE2_REFUGE)) //not in AMC either @ safe_area()
	    || p_ptr->food >= PY_FOOD_FULL)) { /* allow to digest even some in town etc to not get gorged in upcoming fights quickly - C. Blue */
		/* Digest normally */
		if (p_ptr->food < PY_FOOD_MAX) {
			/* Every 50/6 level turns */
			if (!(turn % (DUN_TURN_56(wpos) * 10))) {
				i = food_consumption(Ind);

				/* Cut vampires some slack for Nether Realm:
				   Ancient vampire lords almost don't need any food at all */
				if (p_ptr->prace == RACE_VAMPIRE && p_ptr->total_winner) i = 1;

				/* Never negative or zero. We always consume some nutrition. */
				else if (i < 1) i = 1; //actually should never happen, with the new method (food_consumption())

				/* Digest some food */
				(void)set_food(Ind, p_ptr->food - i);
			}
		}

		/* Digest quickly when gorged */
		else {
			/* Digest a lot of food */
			(void)set_food(Ind, p_ptr->food - 100);
		}

		/* Starve to death (slowly) */
		if (p_ptr->food < PY_FOOD_STARVE) {
			/* Calculate damage */
			i = (PY_FOOD_STARVE - p_ptr->food) / 10;

			/* Take damage */
			take_hit(Ind, i, "starvation", 0);
		}
	}


	/* Default regeneration */
	regen_amount = PY_REGEN_NORMAL;

	/* Amulet of immortality relieves from eating */
	if (p_ptr->admin_invuln) p_ptr->food = PY_FOOD_MAX - 1;

	/* Getting Weak */
	if (p_ptr->food < PY_FOOD_WEAK) {
		/* Lower regeneration */
		if (p_ptr->food < PY_FOOD_STARVE)
			regen_amount = 0;
		else if (p_ptr->food < PY_FOOD_FAINT)
			regen_amount = PY_REGEN_FAINT;
		else
			regen_amount = PY_REGEN_WEAK;

		/* Getting Faint */
		if (!p_ptr->ghost && p_ptr->food < PY_FOOD_FAINT) {
			/* Faint occasionally */
			if (!p_ptr->paralyzed && (rand_int(100) < 10)) {
				/* Message */
				msg_print(Ind, "You faint from the lack of food.");
				disturb(Ind, 1, 0);

				/* Hack -- faint (bypass free action) */
				(void)set_paralyzed(Ind, p_ptr->paralyzed + 1 + rand_int(5));
			}
		}
	}

	/* Resting */
	if (p_ptr->resting) {
		if (!p_ptr->searching) regen_amount = regen_amount * RESTING_RATE;

		/* Timed resting? */
		if (p_ptr->resting > 0) {
			p_ptr->resting--;
			if (!p_ptr->resting) p_ptr->redraw |= (PR_STATE);
		}
	}

	/* Regenerate the mana */
#ifdef ARCADE_SERVER
	p_ptr->regen_mana = TRUE;
#endif
	/* Hack -- regenerate mana 5/3 times faster */
	if (p_ptr->cmp < p_ptr->mmp) {
		if (in_pvparena(wpos))
			regenmana(Ind, ((regen_amount * 5 * PVP_MANA_REGEN_BOOST) * (p_ptr->regen_mana ? 2 : 1)) / 3);
		else
			regenmana(Ind, ((regen_amount * 5) * (p_ptr->regen_mana ? 2 : 1)) / 3);
	}

	/* Regeneration ability - in pvp, damage taken is greatly reduced, so regen must not nullify the remaining damage easily */
#if defined(TROLL_REGENERATION) || defined(HYDRA_REGENERATION)
	if ((i = troll_hydra_regen(p_ptr))) {
		intrinsic_regen = TRUE;
		switch (i) {
		case 1: regen_amount *= (p_ptr->mode & MODE_PVP) ? 2 : 3; break;
		case 2: regen_amount *= (p_ptr->mode & MODE_PVP) ? 2 : 4; break;
		}
	} else
#endif
	if (p_ptr->regenerate) regen_amount *= 2;

	/* Health skill improves regeneration by up to 50% */
	if (minus_health) regen_amount = (regen_amount * (4 + minus_health)) / 6;

	/* Holy curing improves regeneration by up to 100% */
	if (get_skill(p_ptr, SKILL_HCURING) >= 30) regen_amount = (regen_amount * (get_skill(p_ptr, SKILL_HCURING) - 10)) / 20;

	/* Blood Magic, aka Auras, draw from your blood and thereby slow your regeneration down to 1/4 if all 3 auras are enabled */
	regen_amount = regen_amount / (1 + (p_ptr->aura[AURA_FEAR] ? 1 : 0) + (p_ptr->aura[AURA_SHIVER] ? 1 : 0) + (p_ptr->aura[AURA_DEATH] ? 1 : 0));

	/* Poisoned or cut yields no intrinsic healing (aka regeneration) */
	if (p_ptr->poisoned || p_ptr->diseased || p_ptr->sun_burn
#if defined(TROLL_REGENERATION) || defined(HYDRA_REGENERATION)
	    /* Trolls and Hydras continue to regenerate even while cut (it's the whole point of their regen) */
	    || (p_ptr->cut && (!intrinsic_regen || !p_ptr->cut_intrinsic_regen))
#else
	    || p_ptr->cut
#endif
		)
		regen_amount = 0;

	/* But Biofeedback always helps -- TODO: Test the results a bit */
	if (p_ptr->biofeedback) regen_amount += randint(1024) + regen_amount;

	if (p_ptr->hold_hp_regen) regen_amount = (regen_amount * p_ptr->hold_hp_regen_perc) / 100;

	/* Regenerate Hit Points if needed */
	if (p_ptr->chp < p_ptr->mhp && regen_amount && !p_ptr->martyr)
		regenhp(Ind, regen_amount);


	/* Increase regeneration by flat amount from timed regeneration powers */
	if (p_ptr->tim_regen) {
		/* Regeneration spell (Nature) and mushroom of fast metabolism */
		if (p_ptr->tim_regen_pow > 0) {
			/* Actually 'quiet' HP heal, add the actual message afterwards here */
			i = hp_player(Ind, p_ptr->tim_regen_pow / 10 + (magik((p_ptr->tim_regen_pow % 10) * 10) ? 1 : 0), TRUE, TRUE);
			if (i) msg_format(Ind, "\377gYou are healed for %d points.", i);

			/* For spell: Also heal cuts */
			if (p_ptr->cut && !p_ptr->tim_regen_cost && magik(CUT_HEAL_SPEED)) {
				int nonlin, healcut; // tim_regen_pow is 134 at 50.000 Nature (0.000 SP), 183 at 50.000 SP.

#if 0
				nonlin = 10 + (1000 * (p_ptr->tim_regen_pow + 1)) / (p_ptr->cut * 10);
				healcut = (p_ptr->tim_regen_pow * 10) / nonlin;
				// cut 1:	rg 1: 1				rg 10: 1	rg 100: 1
				// cut 10:	rg 1: 1		rg  5: 1	rg 10: 1	rg 100: 1
				// cut 100:	rg 1: 1				rg 10: 4	rg 100: 9	rg 183: 9
				// cut 1000:	rg 1: 1				rg 10: 9	rg 100: 50	rg 183: 65
#else
				nonlin = 20 + (1000 * (p_ptr->tim_regen_pow + 1)) / (p_ptr->cut * 10);
				healcut = (p_ptr->tim_regen_pow * 5) / nonlin;
				// cut 1:	rg 1: 				rg 10: 		rg 100: 1
				// cut 10:	rg 1: 		rg  5: 		rg 10: 		rg 100: 1
				// cut 100:	rg 1: 				rg 10: 		rg 100: 4	rg 183: 4
				// cut 1000:	rg 1: 				rg 10: 		rg 100: 16	rg 183: 24
#endif

				if (!healcut) healcut = 1;
				(void)set_cut(Ind, p_ptr->cut - healcut, p_ptr->cut_attacker, FALSE);
			}
		}
		/* Nether Sap spell (Unlife) */
		else if (p_ptr->tim_regen_pow < 0) {
			if (p_ptr->cmp >= p_ptr->tim_regen_cost) {
				p_ptr->cmp -= p_ptr->tim_regen_cost;
				p_ptr->redraw |= PR_MANA;
				/* (Cannot be using Martyr as true vampire, so no need to check for regen inhibition, but hp_player_quiet() does check for it anyway.)
				   Actually 'quiet' and give msg afterwards */
				i = hp_player(Ind, (-p_ptr->tim_regen_pow) / 10 + (magik(((-p_ptr->tim_regen_pow) % 10) * 10) ? 1 : 0), TRUE, TRUE);
				if (i) msg_format(Ind, "\377gYou are healed for %d points.", i);
			} else (void)set_tim_mp2hp(Ind, 0, 0, 0);  /* End prematurely when OOM */
		}
	}


	/* Undiminish healing penalty in PVP mode */
	if (p_ptr->heal_effect) {
		//p_ptr->heal_effect -= (p_ptr->mhp + p_ptr->lev * 6) / 30;
		p_ptr->heal_effect -= p_ptr->lev / 10;
		if (p_ptr->heal_effect < 0) p_ptr->heal_effect = 0;
	}

	/* Countdown no-teleport rule in PVP mode */
	if (p_ptr->pvp_prevent_tele) {
		p_ptr->pvp_prevent_tele--;
		if (!p_ptr->pvp_prevent_tele) p_ptr->redraw |= PR_DEPTH;
	}

	/* Regenerate depleted Stamina */
	if ((p_ptr->cst < p_ptr->mst) && !p_ptr->shadow_running && !p_ptr->dispersion) {
		//int s = 2 * (76 + (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus_health) * 3);
		int s = regen_boost_stamina * (54 + (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus_health) * 3);

		if (p_ptr->resting && !p_ptr->searching) s *= 2;
		p_ptr->cst_frac += s;
		if (p_ptr->cst_frac >= 10000) {
			p_ptr->cst_frac -= 10000;
			p_ptr->cst++;
			if (p_ptr->cst == p_ptr->mst) p_ptr->cst_frac = 0;
			p_ptr->redraw |= (PR_STAMINA);
		}
	}
	/* Dispersion spell if it has a duration (new version) */
	if (p_ptr->dispersion_tim) {
		p_ptr->dispersion_tim--;
		if (!p_ptr->dispersion_tim) set_dispersion(Ind, 0, 0);
	}

	/* Disturb if we are done resting */
	if ((p_ptr->resting) && (p_ptr->chp == p_ptr->mhp && !p_ptr->drain_life) && (p_ptr->cmp == p_ptr->mmp) && (p_ptr->cst == p_ptr->mst)
#if 0 /* stop resting as an Ent when we reach 'full' food state? */
	    && !(p_ptr->prace == RACE_ENT && p_ptr->food < PY_FOOD_FULL))
#else /* Ents can continue to rest indefinitely to basically be afk without toggling afk state, yet still not going hungry? */
	    && (p_ptr->prace != RACE_ENT || !autofood))
#endif
		disturb(Ind, 0, 0);

	/* Finally, at the end of our turn, update certain counters. */
	/*** Timeout Various Things ***/

	/* Handle temporary stat drains */
	for (i = 0; i < C_ATTRIBUTES; i++) {
		if (p_ptr->stat_cnt[i] > 0) {
			p_ptr->stat_cnt[i] -= (1 + minus_health);
			if (p_ptr->stat_cnt[i] <= 0) {
				do_res_stat_temp(Ind, i);
			}
		}
	}

#ifdef ENABLE_BLOOD_FRENZY
	/* Blood frenzy */
	if (p_ptr->blood_frenzy_rage) {
		if (p_ptr->inventory[INVEN_WIELD].tval != TV_AXE || p_ptr->inventory[INVEN_ARM].tval != TV_AXE)
			p_ptr->blood_frenzy_rage = 0; //reset completely(!)
		else {
			p_ptr->blood_frenzy_rage -= 20 + p_ptr->num_blow;
			if (p_ptr->blood_frenzy_rage < 0) p_ptr->blood_frenzy_rage = 0;
		}

		if (p_ptr->blood_frenzy_active && p_ptr->blood_frenzy_rage <= 700) {
			p_ptr->blood_frenzy_rage = 0; //reset completely(!)
			p_ptr->blood_frenzy_active = FALSE;
			p_ptr->update |= PU_BONUS;
			msg_print(Ind, "\377WYour blood frenzy ceases.");
		}
	}
#endif

	/* Adrenaline */
	if (p_ptr->adrenaline)
		(void)set_adrenaline(Ind, p_ptr->adrenaline - 1);

	/* Biofeedback */
	if (p_ptr->biofeedback)
		(void)set_biofeedback(Ind, p_ptr->biofeedback - 1);

	/* Hack -- Bow Branding */
	if (p_ptr->ammo_brand)
		(void)set_ammo_brand(Ind, p_ptr->ammo_brand - minus_magic, p_ptr->ammo_brand_t, p_ptr->ammo_brand_d);

	/* Nimbus - Kurzel */
	if (p_ptr->nimbus)
		(void)set_nimbus(Ind, p_ptr->nimbus - minus_magic, p_ptr->nimbus_t, p_ptr->nimbus_d);

	/* weapon brand time */
	/* special hack: if both weapons run out at the same time with the same element, only give one (combined) message ^^ */
	if (p_ptr->melee_brand && p_ptr->melee_brand2 &&
	    p_ptr->melee_brand_t == p_ptr->melee_brand2_t &&
	    SGN(p_ptr->melee_brand - minus_magic) == SGN(p_ptr->melee_brand2 - minus_magic)) /* actually not just run out together but also count down together for efficiency */
		(void)set_melee_brand(Ind, p_ptr->melee_brand - minus_magic, p_ptr->melee_brand_t, p_ptr->melee_brand_flags | TBRAND_F_DUAL, FALSE, !p_ptr->melee_brand_ma);
	else {
		if (p_ptr->melee_brand)
			(void)set_melee_brand(Ind, p_ptr->melee_brand - minus_magic, p_ptr->melee_brand_t, p_ptr->melee_brand_flags, FALSE, !p_ptr->melee_brand_ma);
		if (p_ptr->melee_brand2)
			(void)set_melee_brand(Ind, p_ptr->melee_brand2 - minus_magic, p_ptr->melee_brand2_t, p_ptr->melee_brand2_flags, FALSE, !p_ptr->melee_brand_ma);
	}

	/* Hack -- Timed ESP */
	if (p_ptr->tim_esp)
		(void)set_tim_esp(Ind, p_ptr->tim_esp - minus_magic);

#if 0 /* moved to process_player_end() */
	/* Hack -- Space/Time Anchor */
	if (p_ptr->st_anchor)
		(void)set_st_anchor(Ind, p_ptr->st_anchor - minus_magic);
#endif

	/* Hack -- Prob travel */
	if (p_ptr->prob_travel)
		(void)set_prob_travel(Ind, p_ptr->prob_travel - minus_magic);

	/* Hack -- Avoid traps */
	if (p_ptr->tim_traps)
		(void)set_tim_traps(Ind, p_ptr->tim_traps - minus_magic);

	/* Temporary Mimicry from a Ring of Polymorphing or Blood Sacrifice */
	if (p_ptr->tim_mimic && p_ptr->body_monster == p_ptr->tim_mimic_what) {
#ifdef ENABLE_HELLKNIGHT
		if (p_ptr->pclass == CLASS_HELLKNIGHT
 #ifdef ENABLE_CPRIEST
		    || p_ptr->pclass == CLASS_CPRIEST
 #endif
		    )
			/* decrease time left of being polymorphed */
			(void)set_mimic(Ind, p_ptr->tim_mimic - 1, p_ptr->tim_mimic_what);
		else
#endif
			/* hack - on hold while in town */
			if (!townarea && !dungeontown) {
				/* decrease time left of being polymorphed */
				(void)set_mimic(Ind, p_ptr->tim_mimic - 1, p_ptr->tim_mimic_what);
			}
	}

	/* Hack -- Timed manashield */
	if (p_ptr->tim_manashield)
		set_tim_manashield(Ind, p_ptr->tim_manashield - minus_magic);
#ifndef KURZEL_PK
	if (cfg.use_pk_rules == PK_RULES_DECLARE) {
		if (p_ptr->tim_pkill) {
			p_ptr->tim_pkill--;
			if (!p_ptr->tim_pkill) {
				if (p_ptr->pkill & PKILL_SET) {
					msg_print(Ind, "You may now kill other players");
					p_ptr->pkill |= PKILL_KILLER;
				} else {
					msg_print(Ind, "You are now protected from other players");
					p_ptr->pkill &= ~PKILL_KILLABLE;
				}
			}
		}
	}
#endif

	/* Hack -- Meditation */
	if (p_ptr->tim_meditation)
		(void)set_tim_meditation(Ind, p_ptr->tim_meditation - minus);

	/* Hack -- Wraithform (Not for Wraithstep though, it lasts indefinitely) */
	if (p_ptr->tim_wraith && !(p_ptr->tim_wraithstep & 0x1)) (void)set_tim_wraith(Ind, p_ptr->tim_wraith - minus_magic);

	/* Wraithstep: Check if we're within the few turns of weakened immaterium, allowing us to enter solid walls if we want to */
	if ((p_ptr->tim_wraithstep & 0x1) && (p_ptr->tim_wraithstep & 0xF0)) {
		p_ptr->tim_wraithstep -= 0x10;
		if (!(p_ptr->tim_wraithstep & 0xF0)) {
			p_ptr->tim_wraithstep &= ~0x1;
			msg_print(Ind, "The boundary to the immaterium returns to normal.");
			p_ptr->redraw |= PR_BPR_WRAITH_PROB;
		}
	}

	/* Hack -- Hallucinating */
	if (p_ptr->image) {
		int adjust = 1;

		if (get_skill(p_ptr, SKILL_MIND) >= 30) adjust++;
		if (get_skill(p_ptr, SKILL_HCURING) >= 50) adjust++;
		(void)set_image(Ind, p_ptr->image - adjust);
	}

	/* Blindness */
	if (p_ptr->blind) {
		int adjust = 1 + minus_health;

		if (get_skill(p_ptr, SKILL_HCURING) >= 30) adjust++;
		(void)set_blind(Ind, p_ptr->blind - adjust);
	}

	/* Timed see-invisible */
	if (p_ptr->tim_invis)
		(void)set_tim_invis(Ind, p_ptr->tim_invis - minus_magic);

	/* Timed invisibility */
	if (p_ptr->tim_invisibility) {
		if (p_ptr->aggravate) {
			msg_print(Ind, "Your invisibility is broken by your aggravation.");
			(void)set_invis(Ind, 0, 0);
		} else {
			(void)set_invis(Ind, p_ptr->tim_invisibility - minus_magic, p_ptr->tim_invis_power2);
		}
	}
	/* Special invisibility from Shadow spell 'Shadow Shroud' */
	if (p_ptr->shrouded) {
		if (p_ptr->aggravate) set_shroud(Ind, 0, 0);
		else set_shroud(Ind, p_ptr->shrouded - minus_magic, p_ptr->shroud_power);
	}

	/* Timed infra-vision */
	if (p_ptr->tim_infra)
		(void)set_tim_infra(Ind, p_ptr->tim_infra - minus_magic);

	/* Paralysis */
	if (p_ptr->paralyzed && p_ptr->paralyzed <= cfg.spell_stack_limit) /* hack */
		(void)set_paralyzed(Ind, p_ptr->paralyzed - 1);

	/* Confinement */
	if (p_ptr->stopped) (void)set_stopped(Ind, p_ptr->stopped - 1);

	/* Confusion */
	if (p_ptr->confused) {
		int adjust = minus + minus_combat;

		if (get_skill(p_ptr, SKILL_MIND) >= 30) adjust *= 2;
		(void)set_confused(Ind, p_ptr->confused - adjust);
	}

	/* Afraid */
	if (p_ptr->afraid)
		(void)set_afraid(Ind, p_ptr->afraid - minus - minus_combat);

	/* Fast */
	if (p_ptr->fast)
		(void)set_fast(Ind, p_ptr->fast - minus_magic, p_ptr->fast_mod);

	/* Slow */
	if (p_ptr->slow)
		(void)set_slow(Ind, p_ptr->slow - minus_magic);

#ifdef ENABLE_MAIA
	if (p_ptr->divine_crit)
		(void)do_divine_crit(Ind, p_ptr->divine_crit_mod, p_ptr->divine_crit - minus_magic);

	if (p_ptr->divine_hp)
		(void)do_divine_hp(Ind, p_ptr->divine_hp_mod, p_ptr->divine_hp - minus_magic);

	if (p_ptr->divine_xtra_res)
		(void)do_divine_xtra_res(Ind, p_ptr->divine_xtra_res - minus_magic);
#endif
	/* xtra shot? - the_sandman */
	if (p_ptr->focus_time)
		(void)do_focus(Ind, p_ptr->focus_val, p_ptr->focus_time - minus_magic);

	/* xtra stats? - the_sandman */
	if (p_ptr->xtrastat_tim)
		(void)do_xtra_stats(Ind, p_ptr->xtrastat_which, p_ptr->xtrastat_pow, p_ptr->xtrastat_tim - minus_magic, p_ptr->xtrastat_demonic);

	/* Protection from evil */
	if (p_ptr->protevil)
		(void)set_protevil(Ind, p_ptr->protevil - minus_magic, p_ptr->protevil_own);

	/* Holy Zeal - EA bonus */
	if (p_ptr->zeal) {
#ifdef ENABLE_OHERETICISM
		/* Extend ongoing 'Boundless Hate' spell thanks to Traumaturgy feedback? */
		if (p_ptr->hate_prolong == 2) p_ptr->hate_prolong = 0;
//msg_print(Ind, " \377v+prolonged");
		else
//msg_print(Ind, " \377R+0");
#endif
		(void)set_zeal(Ind, p_ptr->zeal_power, p_ptr->zeal - 1);
	}

	/* Heart is still boldened */
	if (p_ptr->res_fear_temp)
		(void)set_res_fear(Ind, p_ptr->res_fear_temp - 1);

	/* Holy Martyr */
#if 0 /* moved to process_player_end() */
	if (p_ptr->martyr)
		(void)set_martyr(Ind, p_ptr->martyr - 1);
	if (p_ptr->martyr_timeout) {
		p_ptr->martyr_timeout--;
		if (!p_ptr->martyr_timeout) msg_print(Ind, "\376The heavens are ready to accept your martyrium.");
	}
#endif

	/* Mindcrafters' Willpower */
	if (p_ptr->mindboost)
		(void)set_mindboost(Ind, p_ptr->mindboost_power, p_ptr->mindboost - 1);

	/* Mindcrafters' repulsion shield */
	if (p_ptr->kinetic_shield) (void)set_kinetic_shield(Ind, p_ptr->kinetic_shield - 1);

#ifdef ENABLE_OCCULT
	/* Spritiual shields (Occult) */
	if (p_ptr->spirit_shield) (void)set_spirit_shield(Ind, p_ptr->spirit_shield_pow, p_ptr->spirit_shield - 1);
	if (p_ptr->temp_savingthrow) (void)set_savingthrow(Ind, p_ptr->temp_savingthrow - 1);
#endif

	if (p_ptr->cloak_neutralized) p_ptr->cloak_neutralized--;
	if (p_ptr->cloaked > 1) {
		if (--p_ptr->cloaked == 1) {
			msg_print(Ind, "\377DYou cloaked your appearance!");
			msg_format_near(Ind, "\377w%s disappears before your eyes!", p_ptr->name);
			p_ptr->update |= (PU_BONUS | PU_MONSTERS);
			p_ptr->redraw |= (PR_STATE | PR_SPEED);
			handle_stuff(Ind);
			/* log a hint that newbies got it */
			if (p_ptr->lev == 15) s_printf("CLOAKING-15: %s.\n", p_ptr->name);
			p_ptr->warning_cloak = 1;
		}
	}

	/* Invulnerability */
	/* Hack -- make -1 permanent invulnerability */
	if (p_ptr->invuln) {
		if (p_ptr->invuln > 0) {
			/* Hack: Don't diminish Stair-GoI by Antimagic: */
			if (p_ptr->invuln_dur >= 5) (void)set_invuln(Ind, p_ptr->invuln - minus_magic);
			else (void)set_invuln(Ind, p_ptr->invuln - 1);
		}
		//if (p_ptr->invuln == 5) msg_print(Ind, "\377vThe invulnerability shield starts to fade...");
	}

	/* Heroism */
	if (p_ptr->hero)
		(void)set_hero(Ind, p_ptr->hero - 1);

	/* Berserk */
	if (p_ptr->shero)
		(void)set_shero(Ind, p_ptr->shero - 1);

	/* Fury */
	if (p_ptr->fury)
		(void)set_fury(Ind, p_ptr->fury - 1);

	/* Sprint? melee technique */
	if (p_ptr->melee_sprint)
		(void)set_melee_sprint(Ind, p_ptr->melee_sprint - 1);

	/* Blessed */
	if (p_ptr->blessed > 0)
		(void)set_blessed(Ind, p_ptr->blessed - 1, p_ptr->blessed_own);
	else if (p_ptr->blessed < 0)
		(void)set_blessed(Ind, p_ptr->blessed + 1, p_ptr->blessed_own);

	/* Shield */
	if (p_ptr->shield)
		(void)set_shield(Ind, p_ptr->shield - minus_magic, p_ptr->shield_power, p_ptr->shield_opt, p_ptr->shield_power_opt, p_ptr->shield_power_opt2);

	/* Timed deflection */
	if (p_ptr->tim_reflect)
		(void)set_tim_reflect(Ind, p_ptr->tim_reflect - minus_magic);

	/* Timed Feather Falling */
	if (p_ptr->tim_ffall)
		(void)set_tim_ffall(Ind, p_ptr->tim_ffall - 1);
	if (p_ptr->tim_lev)
		(void)set_tim_lev(Ind, p_ptr->tim_lev - 1);

	/* Timed regen */
	if (p_ptr->tim_regen) {
		if (p_ptr->tim_regen_pow > 0) (void)set_tim_regen(Ind, p_ptr->tim_regen - 1, p_ptr->tim_regen_pow, p_ptr->tim_regen_cost); /* Regeneration */
		else if (p_ptr->tim_regen_pow < 0) (void)set_tim_mp2hp(Ind, p_ptr->tim_regen - 1, -p_ptr->tim_regen_pow, p_ptr->tim_regen_cost); /* Nether Sap */
	}

	/* Thunderstorm */
	if (p_ptr->tim_thunder) {
		int dam = damroll(p_ptr->tim_thunder_p1, p_ptr->tim_thunder_p2);
		int x, y, tries = 40;
		monster_type *m_ptr = NULL;

		while (--tries) {
			/* Pick random location within thunderstorm radius and check for creature */
			scatter(wpos, &y, &x, p_ptr->py, p_ptr->px, (MAX_RANGE * 2) / 3, FALSE);
			//(max possible grids [tries recommended]: 1 ~1020, 5/6 ~710, 4/5 ~615, 3/4 ~530, 2/3 ~450 [30], 1/2 ~255 [15])
			c_ptr = &zcave[y][x];

			/* Any creature/player here, apart from ourselves? */
			if (!c_ptr->m_idx || -c_ptr->m_idx == Ind) continue;

			/* Non-hostile player or special projector? Skip. */
			if (c_ptr->m_idx < 0 &&
			    (c_ptr->m_idx <= PROJECTOR_UNUSUAL || !check_hostile(Ind, -c_ptr->m_idx)))
				continue;

			/* Hit a monster */
			if (c_ptr->m_idx > 0) {
				/* Access the monster */
				m_ptr = &m_list[c_ptr->m_idx];

				/* Ignore "dead" monsters */
				if (!m_ptr->r_idx) continue;

				/* Don't wake up sleeping monsters unless explicitely toggled (via /tss) */
				if (m_ptr->csleep && !p_ptr->ts_sleeping) continue;
				/* Don't break charm/trance */
				if (m_ptr->charmedignore) continue;

				/* Note that we in general do not exempt NO_DEATH monsters, eg target dummies, they should still get stroke. */

				/* Don't strike friendly questors (Town Elder^^) */
				if (m_ptr->questor && !m_ptr->questor_hostile) continue;
				/* Likewise don't strike non-hostile critters (no need to check for monster ego flags here, as these flags really are basic-intrinsic only) */
				if (r_info[m_ptr->r_idx].flags7 & (RF7_PET | RF7_NEUTRAL | RF7_FRIENDLY | RF7_NO_TARGET))
					/* Panda, horses (horses are also FRIENDLY anyway ie don't seek out the player), Robin */
					continue;
				/* Spells cast by a player never hurt a friendly golem */
				if (m_ptr->owner) {
					int i;

					//look for its owner to see if he's hostile or not
					for (i = 1; i < NumPlayers; i++)
						if (Players[i]->id == m_ptr->owner) {
							if (!check_hostile(Ind, i)) continue;
							break;
						}
					//if his owner is not online, assume friendly(!)
					if (i == NumPlayers) continue;
				}
				//if (m_ptr->pet) continue;
			}
			/* else: Hit a hostile player */

			/* We found a target */
			break;
		}

		if (tries) {
			/* Hit a monster */
			if (c_ptr->m_idx > 0) {
				char m_name[MNAME_LEN];

				monster_desc(Ind, m_name, c_ptr->m_idx, 0);
				msg_format(Ind, "Lightning strikes %s.", m_name);
			}
			/* Hit a hostile player */
			else {
				char q_name[NAME_LEN];

				/* Track player health */
				//if (p_ptr->play_vis[0 - c_ptr->m_idx]) health_track(Ind, c_ptr->m_idx);

				player_desc(Ind, q_name, -c_ptr->m_idx, 0);
				msg_format(Ind, "Lightning strikes %s.", q_name);
			}
#ifdef USE_SOUND_2010
			sound_near_site_vol(y, x, wpos, 0, "lightning", "thunder", SFX_TYPE_NO_OVERLAP, FALSE, 50); //don't overlap, too silyl? also: no screen flashing
#endif
			project(0 - Ind, 0, wpos, y, x, dam, GF_THUNDER, PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID | PROJECT_JUMP | PROJECT_NODF | PROJECT_NODO, "");
			thunderstorm_visual(wpos, x, y, -1);
		}

		(void)set_tim_thunder(Ind, p_ptr->tim_thunder - minus_magic, p_ptr->tim_thunder_p1, p_ptr->tim_thunder_p2);
	}

	/* Oppose Acid */
	if (p_ptr->oppose_acid)
		(void)set_oppose_acid(Ind, p_ptr->oppose_acid - minus_magic);

	/* Oppose Electricity */
	if (p_ptr->oppose_elec)
		(void)set_oppose_elec(Ind, p_ptr->oppose_elec - minus_magic);

	/* Oppose Fire */
	if (p_ptr->oppose_fire)
		(void)set_oppose_fire(Ind, p_ptr->oppose_fire - minus_magic);

	/* Oppose Cold */
	if (p_ptr->oppose_cold)
		(void)set_oppose_cold(Ind, p_ptr->oppose_cold - minus_magic);

	/* Oppose Poison */
	if (p_ptr->oppose_pois)
		(void)set_oppose_pois(Ind, p_ptr->oppose_pois - minus_magic);

	/*** Poison and Stun and Cut ***/

	/* Poison */
	if (p_ptr->poisoned) {
		int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] / 2 + minus); //0..4->1..5

		if (get_skill(p_ptr, SKILL_HCURING) >= 30) adjust += 3; //1..8
		adjust += minus_health; //minus_health is 0..2 -> 1..10

		/* Apply some healing */
		(void)set_poisoned(Ind, p_ptr->poisoned - adjust, p_ptr->poisoned_attacker);

		if (!p_ptr->poisoned) p_ptr->slow_poison = 0;
	}

	/* Disease */
	if (p_ptr->diseased) {
		int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] / 2 + minus); //0..4->1..5

		if (get_skill(p_ptr, SKILL_HCURING) >= 30) adjust += 3; //1..8
		adjust += minus_health; //minus_health is 0..2 -> 1..10

		/* Apply some healing */
		(void)set_diseased(Ind, p_ptr->diseased - adjust, p_ptr->poisoned_attacker);
	}

	/* Stun */
	if (p_ptr->stun) {
		/* Apply some healing */
		int adjust = minus + get_skill_scale_fine(p_ptr, SKILL_COMBAT, 1);

		if (rand_int(9) < adj_con_fix[p_ptr->stat_ind[A_CON]]) adjust++;
		if (get_skill(p_ptr, SKILL_HCURING) >= 40) adjust++;
		(void)set_stun_raw(Ind, p_ptr->stun - adjust);
	}

	/* Cut - heal over time */
	if ((p_ptr->cut || p_ptr->cut_bandaged) && magik(CUT_HEAL_SPEED)) {
		int adjust = minus;// = (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus);

		/* Hack -- Truly "mortal" wound */
		if (p_ptr->cut + p_ptr->cut_bandaged >= CUT_MORTAL_WOUND) {
			/* Holiness > worldly bandages, always helps */
			if (get_skill(p_ptr, SKILL_HCURING) >= 40) adjust = 2;
			else adjust = 0;
		} else {
			if (get_skill(p_ptr, SKILL_HCURING) >= 40) adjust *= 2; //..which is also 2 (minus is always 1 here)
		}
		/* Biofeedback always helps (Draconian firestone effect) */
		if (p_ptr->biofeedback) adjust += 5;

		/* Apply some healing */
		if (p_ptr->cut) (void)set_cut(Ind, p_ptr->cut - adjust * (minus_health + 1), p_ptr->cut_attacker, FALSE);
		else if (adjust >= p_ptr->cut_bandaged) {
			p_ptr->cut_bandaged = p_ptr->cut_attacker = 0;
			msg_print(Ind, "\376Your wound seems healed, you remove the bandage.");
		} else p_ptr->cut_bandaged -= adjust;
	}

#ifdef IRRITATING_WEATHER
	if (!p_ptr->grid_house)
		switch (p_ptr->weather_influence) {
		case 0: break; //no bad weather
		case 1: break; //rainstorm - no effect
		case 2: //snowstorm
			if (p_ptr->resist_cold || p_ptr->oppose_cold || p_ptr->immune_cold
			    || worn_armour_weight(p_ptr) >= 140 + (p_ptr->inventory[INVEN_HEAD].tval ? 0 : 50) + (p_ptr->inventory[INVEN_OUTER].tval ? 0 : 50) /* surpass chain mail weight ^^ */
			    || (p_ptr->inventory[INVEN_OUTER].tval == TV_CLOAK && p_ptr->inventory[INVEN_OUTER].sval == SV_FUR_CLOAK))
				break;
			if (!rand_int(5)) {
				int dam = (p_ptr->chp * (3 + rand_int(5)) + 99) / 100;

				if (dam && p_ptr->chp > dam) {
					//msg_format(Ind, "The winds are freezing you for \377o%d\377w damage.", dam);
					if (!rand_int(3)) msg_format(Ind, "\377wThe wind is freezing you!"); //less spam
					bypass_invuln = FALSE; /* Disruption shield protects from this type of damage */
					take_hit(Ind, dam, "freezing winds", 0);
					bypass_invuln = TRUE;
					/* Note: No inventory damage =p */
				} else msg_format(Ind, "\377wThe wind is freezing you!");
			}
			break;
		case 3: //sandstorm
			if (p_ptr->resist_shard || p_ptr->resist_blind || p_ptr->no_cut
			    || get_skill(p_ptr, SKILL_EARTH) >= 30)
				break;
			if (!rand_int(15)) {
				msg_format(Ind, "\377yThe sand storm is blinding you!");
				set_blind_quiet(Ind, p_ptr->blind + rand_int(7));
			}
			break;
		}
#endif

	/* Temporary blessing of luck */
	if (p_ptr->bless_temp_luck
	    && p_ptr->wpos.wz)
		(void)bless_temp_luck(Ind, -1, p_ptr->bless_temp_luck - 1);

	/* Temporary auras */
	if (p_ptr->sh_fire_tim) (void)set_sh_fire_tim(Ind, p_ptr->sh_fire_tim - minus_magic);
	if (p_ptr->sh_cold_tim) (void)set_sh_cold_tim(Ind, p_ptr->sh_cold_tim - minus_magic);
	if (p_ptr->sh_elec_tim) (void)set_sh_elec_tim(Ind, p_ptr->sh_elec_tim - minus_magic);

	if (p_ptr->tim_lcage) set_tim_lcage(Ind, p_ptr->tim_lcage - 1);


	/* Still possible effects from another player's support spell on this player? */
	if (p_ptr->support_timer) {
		p_ptr->support_timer--;
		if (!p_ptr->support_timer) p_ptr->supp = p_ptr->supp_top = 0;
	}

	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		if (i == INVEN_LITE) i++; /* Handled further down */

		timeout_handled = FALSE;
		o_ptr = &p_ptr->inventory[i];

#if POLY_RING_METHOD == 0
		/* Check polymorph rings with timeouts */
		if (i == INVEN_LEFT || i == INVEN_RIGHT) {
			if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH) && (o_ptr->timeout_magic > 0) &&
			    (p_ptr->body_monster == o_ptr->pval)) {
				timeout_handled = TRUE;

				/* Decrease life-span */
				o_ptr->timeout_magic--;
 #ifndef LIVE_TIMEOUTS
				/* Hack -- notice interesting energy steps */
				if ((o_ptr->timeout_magic < 100) || (!(o_ptr->timeout_magic % 100)))
 #else
				if (p_ptr->live_timeouts || (o_ptr->timeout_magic < 100) || (!(o_ptr->timeout_magic % 100)))
 #endif
				{
					/* Window stuff */
					p_ptr->window |= PW_EQUIP;
				}

				/* Hack -- notice interesting fuel steps */
				if ((o_ptr->timeout_magic > 0) && (o_ptr->timeout_magic < 100) && !(o_ptr->timeout_magic % 10)) {
					if (p_ptr->disturb_minor) disturb(Ind, 0, 0);
					msg_print(Ind, "\376\377LYour ring flickers and fades, flashes of light run over its surface.");
					/* Window stuff */
					p_ptr->window |= PW_EQUIP;
				} else if (o_ptr->timeout_magic == 0) {
					disturb(Ind, 0, 0);
					msg_print(Ind, "\376\377LYour ring disintegrates!");

					if ((p_ptr->body_monster == o_ptr->pval) &&
					    ((p_ptr->r_mimicry[p_ptr->body_monster] < r_info[p_ptr->body_monster].level) ||
					    (get_skill_scale(p_ptr, SKILL_MIMIC, 100) < r_info[p_ptr->body_monster].level))) {
						/* If player hasn't got high enough kill count anymore now, poly back to player form! */
						msg_print(Ind, "You polymorph back to your normal form.");
						do_mimic_change(Ind, 0, TRUE);
					}

					/* Decrease the item, optimize. */
					inven_item_increase(Ind, i, -1);
					inven_item_optimize(Ind, i);
				}
			}
		}
#endif

		/* Yet-to-add "Ethereal" or other timed items, that live only until running out of energy. Added for ART_ANTIRIAD, the first of this sort. */
		if (o_ptr->timeout && !timeout_handled) {
			/* Decrease life-span */
			/* Decrease in in-game hours (aka 360 in-game turns: ~2/s * 3600 / 20 (20x in-game-time vs real-time)) ? */
			if (o_ptr->name1 == ART_ANTIRIAD) {
				o_ptr->xtra9++;
				if (o_ptr->xtra9 == 360) {
					o_ptr->xtra9 = 0;
					o_ptr->timeout--;
				} else continue; /* No visible change yet */
			}
			/* Decrease in in-game (character) turns */
			else o_ptr->timeout--;

#ifndef LIVE_TIMEOUTS
			/* Hack -- notice interesting energy steps */
			if ((o_ptr->timeout < 100) || (!(o_ptr->timeout % 100)))
#else
			if (p_ptr->live_timeouts || (o_ptr->timeout < 100) || (!(o_ptr->timeout % 100)))
#endif
			{
				/* Window stuff */
				p_ptr->window |= PW_EQUIP;
			}

			/* Hack -- notice interesting fuel steps */
			if ((o_ptr->timeout > 0) && (o_ptr->timeout < 100) && !(o_ptr->timeout % 10)) {
				if (p_ptr->disturb_minor) disturb(Ind, 0, 0);
				/* Window stuff */
				p_ptr->window |= PW_EQUIP;

				if (o_ptr->name1 != ART_ANTIRIAD) {
					char o_name[ONAME_LEN];

					object_desc(Ind, o_name, o_ptr, TRUE, 3);
					msg_format(Ind, "\376\377LYour %s flickers and flashes...", o_name);
				}
			} else if (o_ptr->timeout == 0) {
				disturb(Ind, 0, 0);

				if (o_ptr->name1 != ART_ANTIRIAD) {
					char o_name[ONAME_LEN];

					object_desc(Ind, o_name, o_ptr, TRUE, 3);
					msg_format(Ind, "\376\377LYour %s disintegrates!", o_name);

					/* Decrease the item, optimize. */
					inven_item_increase(Ind, i, -1);
					inven_item_optimize(Ind, i);
				} else {
					msg_format(Ind, "\376\377LYour armour is out of energy and suddenly becomes almost impossible to move!");
					o_ptr->name1 = ART_ANTIRIAD_DEPLETED;
					o_ptr->weight = a_info[o_ptr->name1].weight;
					//handle_art_i(ART_ANTIRIAD_DEPLETED);
					//handle_art_d(ART_ANTIRIAD);
					p_ptr->window |= (PW_INVEN | PW_EQUIP); //todo: weight doesn't update!
				}

				p_ptr->update |= PU_BONUS;
				handle_stuff(Ind);
			}
		}
	}

	/* Don't accidentally float after dying */
	if (p_ptr->safe_float_turns) p_ptr->safe_float_turns--;

#ifdef USE_SOUND_2010
	/* Start delayed music for players who were greeted at start via audio */
	if (p_ptr->music_start) {
		p_ptr->music_start--;
		if (p_ptr->music_start == 0) handle_music(Ind);
	}
#endif


	/*** Process Light ***/

	/* Check for light being wielded */
	o_ptr = &p_ptr->inventory[INVEN_LITE];

	/* Burn some fuel in the current lite */
	if (o_ptr->tval == TV_LITE) {
		u32b f1 = 0 , f2 = 0 , f3 = 0, f4 = 0, f5 = 0, f6 = 0, esp = 0;

		/* Hack -- Use some fuel (sometimes) */
#if 0
		if (!artifact_p(o_ptr) && !(o_ptr->sval == SV_LITE_DWARVEN)
		    && !(o_ptr->sval == SV_LITE_FEANORIAN) && (o_ptr->pval > 0) && (!o_ptr->name1))
#endif

		/* Extract the item flags */
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		/* Hack -- Use some fuel */
		if ((f4 & TR4_FUEL_LITE) && o_ptr->timeout > 0
		    && !(get_skill(p_ptr, SKILL_TEMPORAL) >= 20 && rand_int(2))
		    ) {
			/* Decrease life-span */
			o_ptr->timeout--;

			/* Hack -- notice interesting fuel steps */
#ifndef LIVE_TIMEOUTS
			if ((o_ptr->timeout < 100) || (!(o_ptr->timeout % 100)))
#else
			if (p_ptr->live_timeouts || (o_ptr->timeout < 100) || (!(o_ptr->timeout % 100)))
#endif
			{
				/* Window stuff */
				p_ptr->window |= PW_EQUIP;
			}

			/* Hack -- Special treatment when blind */
			if (p_ptr->blind) {
				/* Hack -- save some light for later */
				if (o_ptr->timeout == 0) o_ptr->timeout++;
			}

			/* The light is now out */
			else if (o_ptr->timeout == 0) {
				bool refilled = FALSE;

				disturb(Ind, 0, 0);

				/* If flint is ready, refill at once */
				if (TOOL_EQUIPPED(p_ptr) == SV_TOOL_FLINT && !p_ptr->paralyzed) {
					msg_print(Ind, "You prepare a flint...");
					refilled = do_auto_refill(Ind);
					if (!refilled)
						msg_print(Ind, "Oops, you're out of fuel!");
				}

				if (!refilled) {
					msg_print(Ind, "Your light has gone out!");
					/* Calculate torch radius */
					p_ptr->update |= (PU_TORCH | PU_BONUS);

					if (!p_ptr->warning_lite_refill) {
						p_ptr->warning_lite_refill = 1;
						msg_print(Ind, "\374\377yHINT: Press \377oSHIFT+f\377y to refill your light source. You will need a flask of");
						msg_print(Ind, "\374\377y      oil for lanterns, or another torch to combine with an extinct torch.");
						s_printf("warning_lite_refill: %s\n", p_ptr->name);
					}
				}
#if 0	/* torch of presentiment goes poof to unlight trap, taken out for now - C. Blue */
				/* Torch disappears */
				if (o_ptr->sval == SV_LITE_TORCH && !o_ptr->timeout) {
					/* Decrease the item, optimize. */
					inven_item_increase(Ind, INVEN_LITE, -1);
					//inven_item_describe(Ind, INVEN_LITE);
					inven_item_optimize(Ind, INVEN_LITE);
				}
#endif
			}

			/* The light is getting dim */
			else if ((o_ptr->timeout < 100) && (!(o_ptr->timeout % 10))) {
				if (p_ptr->disturb_minor) disturb(Ind, 0, 0);
				msg_print(Ind, HCMSG_LIGHT_FAINT);
			}
		}
	}

	/* Calculate torch radius */
	//p_ptr->update |= (PU_TORCH|PU_BONUS);

	/* Silyl fun stuff >:) - C. Blue */
	if (p_ptr->corner_turn) {
		p_ptr->corner_turn -= (p_ptr->corner_turn > 2 ? 2 : p_ptr->corner_turn);
		if (p_ptr->martyr) p_ptr->corner_turn = 0;
		if (p_ptr->corner_turn > 25) {
			p_ptr->corner_turn = 0;
			disturb(Ind, 0, 0);
			msg_print(Ind, "\377oYour head feels dizzy!");
			msg_format_near(Ind, "%s looks dizzy!", p_ptr->name);
			set_confused(Ind, 10 + rand_int(5));
			if (magik(50)) {
				if (!p_ptr->suscep_life && p_ptr->prace != RACE_ENT) {
					msg_print(Ind, "\377oYou vomit!");
					msg_format_near(Ind, "%s vomits!", p_ptr->name);
					take_hit(Ind, 1, "circulation collapse", 0);
					//? if (p_ptr->chp < p_ptr->mhp) /* *invincibility* fix */
					if (p_ptr->food > PY_FOOD_FAINT - 1) (void)set_food(Ind, PY_FOOD_FAINT - 1);
					(void)set_poisoned(Ind, 0, 0);
					if (!p_ptr->free_act && !p_ptr->slow_digest)
					(void)set_paralyzed(Ind, p_ptr->paralyzed + rand_int(10) + 10);
				}
			}
		}
	}


	/*** Process Inventory ***/

	/* Handle experience draining */
	/* (Historical comment, code was changed meanwhile- "Moltor is right, exp drain was too weak for up to quite high levels. Need to make a new formula..") */
	/* No expdrain or just less expdrain while player is on world surface,
	   no expdrain while in town or housing area,
	   to allow characters who lack a *remove curse* spell to make more use of the rings. */
	if (p_ptr->drain_exp
	    && magik((p_ptr->wpos.wz != 0 ? (dungeontown ? 0 : 50) :
	     (townarea ? 0 : 25)) / (p_ptr->prace == RACE_VAMPIRE ? 2 : 1))
	    && magik(100 - p_ptr->antimagic / 2)
	    && magik(30 - (60 / (p_ptr->drain_exp + 2))) /* 10%/15%/18%/20% probability per dungeon tick (5/6 player's dungeon turn) to lose XP (for 1/2/3/4 drain sources) */
	    ) {
		//long exploss = 1 + p_ptr->lev + p_ptr->max_exp / 2000L; /* loss is 1+35+175 (lv 35), 1+40+525 (lv 40), 1+45+1250 (lv 45), 1+50+2900 (lv 50) */
		long exploss = 1 + p_ptr->lev + p_ptr->max_exp / 3000L; /* experimental: reduce divisor to 3000...4000 (for slower-paced chars) */

		/* Reduce exploss the more difficult it is for a race/class combination
		   to aquire exp, ie depending on the character's exp% penalty.
		   Otherwise wearing rings of power on Draconian or Maiar becomes quite harsh,
		   and might require special strategies to counter such as taking ood U pits. */
		exploss = (exploss * 120L) / (50 + p_ptr->expfact / 2);

		/* Cap it against level thresholds */
#ifdef ALT_EXPRATIO
		if ((p_ptr->lev >= 2) && (player_exp[p_ptr->lev - 2] > p_ptr->exp - exploss))
			exploss = p_ptr->exp - player_exp[p_ptr->lev - 2];
#else
		if ((p_ptr->lev >= 2) && ((((s64b)player_exp[p_ptr->lev - 2]) * ((s64b)p_ptr->expfact)) / 100L > p_ptr->exp - exploss))
			exploss = p_ptr->exp - player_exp[p_ptr->lev - 2];
#endif

		/* Drain it! */
		if (exploss > 0) take_xp_hit(Ind, exploss, "Draining", TRUE, FALSE, FALSE, 0);
	}

	/* Now implemented here too ;) - C. Blue */
	/* let's say TY_CURSE lowers stats (occurs often) */
	if (p_ptr->ty_curse &&
	    (rand_int(p_ptr->wpos.wz != 0 ? 200 : (town || dungeontown ? 0 : 500)) == 1) &&
	    (get_skill(p_ptr, SKILL_HSUPPORT) < 40)) {
		if (magik(105 - p_ptr->skill_sav)) {
			msg_print(Ind, "An ancient foul curse touches you but you resist!");
		} else {
			msg_print(Ind, "An ancient foul curse shakes your body!");
#if 0
			if (rand_int(2))
			(void)do_dec_stat(Ind, rand_int(6), STAT_DEC_NORMAL);
			else if (!p_ptr->keep_life) lose_exp(Ind, (p_ptr->exp / 100) * MON_DRAIN_LIFE);
			/* take_xp_hit(Ind, 1 + p_ptr->lev / 5 + p_ptr->max_exp / 50000L, "an ancient foul curse", TRUE, TRUE, TRUE, 0); */
#else
			(void)do_dec_stat(Ind, rand_int(6), STAT_DEC_NORMAL);
#endif
		}
	}
	/* and DG_CURSE randomly summons a monster (non-unique) */
	if (p_ptr->dg_curse && (rand_int(300) == 0) && !townarea && !dungeontown &&
	    (get_skill(p_ptr, SKILL_HSUPPORT) < 40)) {
		int anti_Ind = world_check_antimagic(Ind);

		if (anti_Ind) {
#ifdef USE_SOUND_2010
			sound(Ind, "am_field", NULL, SFX_TYPE_MISC, FALSE);
#endif
			msg_format(anti_Ind, "\377%cA curse builds up but dissipates in your anti-magic field.", COLOUR_AM_GOOD);

			switch (Players[anti_Ind]->name[strlen(Players[anti_Ind]->name) - 1]) {
			case 's': case 'x': case 'z':
				msg_format_near(anti_Ind, "\377%cA curse builds up but dissipates in %s' anti-magic field.", COLOUR_AM_NEAR, Players[anti_Ind]->name);
				break;
			default:
				msg_format_near(anti_Ind, "\377%cA curse builds up but dissipates in %s's anti-magic field.", COLOUR_AM_NEAR, Players[anti_Ind]->name);
			}
		} else {
			msg_print(Ind, "An ancient morgothian curse calls out!");
#ifdef USE_SOUND_2010
			if (summon_specific(&p_ptr->wpos, p_ptr->py, p_ptr->px, -((p_ptr->lev + getlevel(&p_ptr->wpos) + 1) / 2 + 1), 100, SUMMON_MONSTER, 0, 0))
				sound_near_site(p_ptr->py, p_ptr->px, &p_ptr->wpos, 0, "summon", NULL, SFX_TYPE_MISC, FALSE);
#else
			(void)summon_specific(&p_ptr->wpos, p_ptr->py, p_ptr->px, -((p_ptr->lev + getlevel(&p_ptr->wpos) + 1) / 2 + 1), 100, SUMMON_MONSTER, 0, 0);
#endif
		}
	}

	/* Handle experience draining.  In Oangband, the effect
	 * is worse, especially for high-level characters.
	 * As per Tolkien, hobbits are resistant.
	 */
	if (p_ptr->black_breath &&
	    rand_int((get_skill(p_ptr, SKILL_HCURING) >= 50) ? 250 : 150) <
	    (p_ptr->prace == RACE_HOBBIT || p_ptr->body_monster == RI_HALFLING_SLINGER || p_ptr->body_monster == RI_SLHOBBIT || p_ptr->suscep_life ? 2 : 5)) {
		(void)do_dec_stat_time(Ind, rand_int(6), STAT_DEC_NORMAL, 25, 0, TRUE);
		take_xp_hit(Ind, 1 + p_ptr->lev * 3 + p_ptr->max_exp / 5000L,
		    "Black Breath", TRUE, TRUE, TRUE, 0);
	}

	/* Drain Mana */
	if (p_ptr->drain_mana && p_ptr->cmp) {
		p_ptr->cmp -= p_ptr->drain_mana;
		if (magik(30)) p_ptr->cmp -= p_ptr->drain_mana;

		if (p_ptr->cmp < 0) p_ptr->cmp = 0;

		/* Redraw */
		p_ptr->redraw |= (PR_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);
	}

	/* Drain Hitpoints */
	if (p_ptr->drain_life) {
		int drain = (p_ptr->drain_life) * (rand_int(p_ptr->mhp / 100) + 1);

		p_ptr->no_alert = TRUE;
		take_hit(Ind, drain < p_ptr->chp ? drain : p_ptr->chp, "life draining", 0);
		p_ptr->no_alert = FALSE;
	}

	/* Charm/Possess: Drain Mana */
	if (p_ptr->mcharming) {
		int drain, drain_frac;

		if (p_ptr->mmp < 120) drain = fast_sqrt_10div6[p_ptr->mmp];
		else drain = fast_sqrt_10[p_ptr->mmp / 6];
		drain = (drain * p_ptr->mcharming);
		drain_frac = drain % 100;
		drain = drain / 100;

		if (rand_int(100) < drain_frac) p_ptr->cmp--; /* handle first decimal place via probability */
		p_ptr->cmp -= drain;
		if (p_ptr->cmp < 0) p_ptr->cmp = 0;

		/* Redraw */
		p_ptr->redraw |= (PR_MANA);
		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		if (!p_ptr->cmp) do_mstopcharm(Ind);
	}

	/* Note changes */
	j = 0;

	/* Items carried that are susceptible to warmth can be kept cool longer by a Water-proficient mage or other means of cooling.
	   (Maybe todo: Vampire player carrying stuff while in a cold_place..) */
	k = 0;
	if (get_skill(p_ptr, SKILL_WATER) >= 8) k = get_skill(p_ptr, SKILL_WATER);
	//if (get_skill(p_ptr, SKIll_PPOWER) >= 24 && get_skill(p_ptr, SKIll_PPOWER) - 16 > k) k = get_skill(p_ptr, SKILL_PPOWER) - 16;
	if (p_ptr->sh_cold && !p_ptr->sh_fire && k < 20) k = 20;
	if (p_ptr->ptrait == TRAIT_WHITE && k < 29) k = 29;
	if (((p_ptr->aura[AURA_SHIVER] && get_skill(p_ptr, SKILL_AURA_SHIVER) >= 30) ||
	    (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST))
	     && k < 29)
	        k = 29;
	if (cold_place(&p_ptr->wpos)) {
		warm_place = FALSE;
		if (p_ptr->prace == RACE_VAMPIRE) {
			/* Vampires don't emanate as much body heat */
			if (k < 39) k = 39;
		} else {
			if (k < 20) k = 20;
		}
	}

	if (rand_int(86) > k - 8) { /* cold effects prolong the duration to up to 2x */
		int iced = 0, cooling = 0;

		/* Check if we carry ice/snow for extra backpack cooling */
		for (i = INVEN_PACK - 1; i >= 0; i--) {
			o_ptr = &p_ptr->inventory[i];
			if (!o_ptr->k_idx) continue;

			if (o_ptr->tval == TV_GAME && o_ptr->sval == SV_SNOWBALL) iced += o_ptr->number;
			if (o_ptr->tval == TV_POTION && o_ptr->sval == SV_POTION_BLOOD) cooling += o_ptr->number;
		}
		/* Calc % chance for extra preservation turn and cap it */
		if (cooling) {
			cooling = (50 * iced) / (3 * cooling); //require n snowballs to optimally cool 1 potion
			if (cooling > 50) cooling = 50;
		}
		/* Calc % chance for snow to preserve itself and cap it */
		iced = 100 - (iced * 10 + 1990) / (iced + 19);
		if (iced > 67) iced = 67;

		/* Process inventory (blood potions, snowballs).
		   We use inverse order so we can check for snowballs first,
		   which will then affect Blood Potion shelf life! */
		for (i = INVEN_PACK - 1; i >= 0; i--) {
			/* Get the object */
			o_ptr = &p_ptr->inventory[i];

			/* Skip non-objects */
			if (!o_ptr->k_idx) continue;

			/* SV_POTION_BLOOD going bad */
			if (o_ptr->tval == TV_POTION && o_ptr->sval == SV_POTION_BLOOD) {
				/* Carrying enough snow will prolong potions by another 50% */
				if (o_ptr->timeout && !magik(cooling)) {
					o_ptr->timeout--;
					/* Heat accelerates the process */
					if (o_ptr->timeout && (p_ptr->sh_fire || p_ptr->ptrait == TRAIT_RED) && !p_ptr->sh_cold && !rand_int(2)) o_ptr->timeout--;
#ifdef LIVE_TIMEOUTS
					if (p_ptr->live_timeouts) p_ptr->window |= PW_INVEN;
#endif
					/* Notice changes */
					if (!(o_ptr->timeout)) {
						char o_name[ONAME_LEN];

						object_desc(Ind, o_name, o_ptr, FALSE, 3);
						msg_format(Ind, "Your %s %s gone bad.", o_name, o_ptr->number == 1 ? "has" : "have");

						inven_item_increase(Ind, i, -o_ptr->number);
						inven_item_describe(Ind, i);
						inven_item_optimize(Ind, i);
						j++;
					}
					continue;
				}
			}

			/* SV_SNOWBALL melting */
			if (o_ptr->tval == TV_GAME && o_ptr->sval == SV_SNOWBALL) {
				if (warm_place && !magik(iced)) { /* snowballs can also preserve each other */
					o_ptr->pval--;
					/* Heat accelerates the process */
					if (o_ptr->pval && (p_ptr->sh_fire || p_ptr->ptrait == TRAIT_RED) && !p_ptr->sh_cold && !rand_int(2)) o_ptr->pval--;
#ifdef LIVE_TIMEOUTS
					if (p_ptr->live_timeouts) p_ptr->window |= PW_INVEN;
#endif
					/* Notice changes */
					if (!(o_ptr->pval)) {
						char o_name[ONAME_LEN];

						object_desc(Ind, o_name, o_ptr, FALSE, 3);
						msg_format(Ind, "Your %s %s!", o_name, o_ptr->number == 1 ? "melts" : "melt");

						inven_item_increase(Ind, i, -o_ptr->number);
						inven_item_describe(Ind, i);
						inven_item_optimize(Ind, i);
						j++;
					}
					continue;
				}
			}
		}
	}

	/* Process equipment */
	for (i = 0; i < INVEN_TOTAL; i++) {
		/* Get the object */
		o_ptr = &p_ptr->inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Recharge activatable objects */
		/* (well, light-src should be handled here too? -Jir- */
		if (o_ptr->recharging) {
			/* Recharge */
			o_ptr->recharging--;

			/* Notice changes */
			if (!(o_ptr->recharging)) {
				object_desc(Ind, o_name, o_ptr, FALSE, 256);
				msg_format(Ind, "Your %s (%c) ha%s finished charging.", o_name, index_to_label(i), o_ptr->number == 1 ? "s" : "ve");
				j++;
			}
		}
	}

	/* Recharge rods in player's inventory */
	/* this should be moved to 'timeout'? */
#ifdef WIELD_DEVICES
	for (i = 0; i <= INVEN_WIELD; i++) {
#else
	for (i = 0; i < INVEN_PACK; i++) {
#endif
		o_ptr = &p_ptr->inventory[i];

		/* Examine all charging rods */
		if ((o_ptr->tval == TV_ROD
#ifdef MSTAFF_MDEV_COMBO
		    || (o_ptr->tval == TV_MSTAFF && o_ptr->xtra3)
#endif
		    ) && o_ptr->pval) {
#ifndef NEW_MDEV_STACKING
			/* Charge it */
			o_ptr->pval--;
 #if 0 /* zap_rod() already applies individually reduced "uncharge" based on Magic Device skill, so this is redundant. And it's not used for rods on the floor. */
			/* Charge it further */
			if (o_ptr->pval && magik(p_ptr->skill_dev))
				o_ptr->pval--;
 #endif
#else
			/* Charge it */
			o_ptr->pval -= o_ptr->number;
			if (o_ptr->pval < 0) o_ptr->pval = 0; //can happen by rod-stack-splitting (divide_charged_item())
			/* Reset it from 'charging' state to charged state */
			if (!o_ptr->pval) o_ptr->bpval = 0;
#endif

			/* Notice changes */
			if (!(o_ptr->pval)) {
				if (check_guard_inscription(o_ptr->note, 'C')) {
					object_desc(Ind, o_name, o_ptr, FALSE, 256);
					msg_format(Ind, "Your %s (%c) ha%s finished charging.", o_name, index_to_label(i), o_ptr->number == 1 ? "s" : "ve");
				}
				j++;
			}
		}

#ifdef ENABLE_SUBINVEN
		if (o_ptr->tval == TV_SUBINVEN && o_ptr->sval == SV_SI_MDEVP_WRAPPING) {
			int k, size = o_ptr->bpval;

			for (k = 0; k < size; k++) {
				o_ptr = &p_ptr->subinventory[i][k];

				/* Examine all charging rods */
				if ((o_ptr->tval == TV_ROD) && (o_ptr->pval)) {
 #ifndef NEW_MDEV_STACKING
					/* Charge it */
					o_ptr->pval--;
  #if 0 /* zap_rod() already applies individually reduced "uncharge" based on Magic Device skill, so this is redundant. And it's not used for rods on the floor. */
					/* Charge it further */
					if (o_ptr->pval && magik(p_ptr->skill_dev))
						o_ptr->pval--;
  #endif
 #else
					/* Charge it */
					o_ptr->pval -= o_ptr->number;
					if (o_ptr->pval < 0) o_ptr->pval = 0; //can happen by rod-stack-splitting (divide_charged_item())
					/* Reset it from 'charging' state to charged state */
					if (!o_ptr->pval) o_ptr->bpval = 0;
 #endif

					/* Notice changes */
					if (!(o_ptr->pval)) {
						if (check_guard_inscription(o_ptr->note, 'C')) {
							object_desc(Ind, o_name, o_ptr, FALSE, 256);
							msg_format(Ind, "Your %s (%c)(%c) ha%s finished charging.", o_name, index_to_label(i), index_to_label(k), o_ptr->number == 1 ? "s" : "ve");
						}
						j++; //not needed, since we redraw subinven here directly instead of using PW_INVEN. Todo: Implement PW_SUBINVEN and all (slide inven etc).
						/* Redraw subinven item */
						display_subinven_aux(Ind, i, k);
					}
				}
			}
		}
#endif
	}

	/* Notice changes */
	if (j) {
		/* Combine pack */
		p_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);
	}

	/* Feel the inventory */
	if (!p_ptr->admin_dm) sense_inventory(Ind);

	/*** Involuntary Movement ***/


	/* Delayed mind link */
	if (p_ptr->esp_link_end) {
//msg_format(Ind, "ele: %d", p_ptr->esp_link_end);
		p_ptr->esp_link_end--;

		/* Activate the break */
		if (!p_ptr->esp_link_end) {
			player_type *p_ptr2 = NULL;
			int Ind2;

			if ((Ind2 = get_esp_link(Ind, 0x0, &p_ptr2))) {
				if (!(p_ptr->esp_link_flags & LINKF_HIDDEN)) {
					msg_format(Ind, "\377RYou break the mind link with %s.", p_ptr2->name);
					msg_format(Ind2, "\377R%s breaks the mind link with you.", p_ptr->name);
				}
				if (p_ptr->esp_link_flags & LINKF_VIEW_DEDICATED) p_ptr->update |= PU_MUSIC;
				if (p_ptr2->esp_link_flags & LINKF_VIEW_DEDICATED) p_ptr2->update |= PU_MUSIC;
				p_ptr->esp_link = 0;
				p_ptr->esp_link_type = 0;
				p_ptr->esp_link_flags = 0;
				p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
				p_ptr->update |= (PU_BONUS | PU_VIEW | PU_MANA | PU_HP);
				p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);
				p_ptr2->esp_link = 0;
				p_ptr2->esp_link_type = 0;
				p_ptr2->esp_link_flags = 0;
				p_ptr2->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
				p_ptr2->update |= (PU_BONUS | PU_VIEW | PU_MANA | PU_HP);
				p_ptr2->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);
			}
		}
	}

	/* Don't do AFK in a store */
	if (p_ptr->tim_store) {
		if (p_ptr->store_num == -1) p_ptr->tim_store = 0;
#ifdef ENABLE_GO_GAME
		else if (go_game_up && go_engine_player_id == p_ptr->id) p_ptr->tim_store = 0;
#endif
		else if (!admin_p(Ind)) {
			/* Count down towards turnout */
			p_ptr->tim_store--;

			/* Check if that town is 'crowded' */
			if (p_ptr->tim_store <= 0) {
				player_type *q_ptr;
				bool bye = FALSE;

				for (j = 1; j <= NumPlayers; j++) {
					if (Ind == j) continue;
					if (admin_p(j)) continue;
					q_ptr = Players[j];
					if (!inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;
					if (q_ptr->afk) continue;

					/* new: other players must wait in line, or at least closely nearby, to kick you out */
					if (ABS(q_ptr->py - p_ptr->py) >= 4 || ABS(q_ptr->px - p_ptr->px) >= 4) continue;

					bye = TRUE;
					break;
				}

				if (bye) store_kick(Ind, TRUE);
				else p_ptr->tim_store = STORE_TURNOUT;
			}
		}
	}

	if (p_ptr->hold_hp_regen) p_ptr->hold_hp_regen--;

	if (p_ptr->pstealing) {
		/* Count down towards turnout */
		p_ptr->pstealing--;
		//if (!p_ptr->pstealing) msg_print(Ind, "You're calm enough to steal from another player.");
		if (!p_ptr->pstealing) msg_print(Ind, "You're calm enough to attempt to steal something.");
	}

	/* Delayed Word-of-Recall */
	if (p_ptr->word_recall) {
		if ((l_ptr && (l_ptr->flags2 & LF2_NO_TELE) && !is_admin(p_ptr))
#ifdef ANTI_TELE_CHEEZE
		    || p_ptr->anti_tele
 #ifdef ANTI_TELE_CHEEZE_ANCHOR
		    || check_st_anchor(&p_ptr->wpos, p_ptr->py, p_ptr->px)
 #endif
#endif
		    ) {
			msg_print(Ind, "\377oA tension leaves the air around you...");
			p_ptr->word_recall = 0;
			p_ptr->recall_x = p_ptr->recall_y = 0;
			if (p_ptr->disturb_state) disturb(Ind, 0, 0);
			/* Redraw the depth(colour) */
			p_ptr->redraw |= (PR_DEPTH);
			handle_stuff(Ind);
		} else {
			/* Count down towards recall */
			p_ptr->word_recall--;

			/* MEGA HACK: no recall if icky, or in a shop */
			if (!p_ptr->word_recall) {
				if (
#ifndef ANTI_TELE_CHEEZE
				    p_ptr->anti_tele || (check_st_anchor(&p_ptr->wpos, p_ptr->py, p_ptr->px) && !p_ptr->admin_dm) ||
#endif
				    p_ptr->store_num != -1 ||
				    zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK)
					p_ptr->word_recall = 1;
				else
					/* Activate the recall */
					do_recall(Ind, 0);
			}
		}
	}

	bypass_invuln = FALSE;

	/* Evileye, please tell me if it's right */
	if (p_ptr->tim_wraith) {
		if (zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) {
			p_ptr->tim_wraith = 0;
			p_ptr->tim_wraithstep &= ~0x1; //hack: mark as normal wraithform, to distinguish from wraithstep
			msg_print(Ind, "You lose your wraith powers.");
			p_ptr->redraw |= PR_BPR_WRAITH_PROB;
			msg_format_near(Ind, "%s loses %s wraith powers.", p_ptr->name, p_ptr->male ? "his":"her");
		}
		/* No wraithform on NO_MAGIC levels - C. Blue */
		else if (l_ptr && (l_ptr->flags1 & LF1_NO_MAGIC)) {
			p_ptr->tim_wraith = 0;
			p_ptr->tim_wraithstep &= ~0x1; //hack: mark as normal wraithform, to distinguish from wraithstep
			msg_print(Ind, "You lose your wraith powers.");
			p_ptr->redraw |= PR_BPR_WRAITH_PROB;
			msg_format_near(Ind, "%s loses %s wraith powers.", p_ptr->name, p_ptr->male ? "his":"her");
		}
	}

	return(TRUE);
}

/* process any team games */
static void process_games(int Ind) {
	player_type *p_ptr = Players[Ind];
	cave_type **zcave;
	cave_type *c_ptr;
	char sstr[80];
	int score = 0;

	if (!(zcave = getcave(&p_ptr->wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	if (c_ptr->feat == FEAT_AGOAL || c_ptr->feat == FEAT_BGOAL) {
		int ball;

		switch (gametype) {
		/* rugby type game */
		case EEGAME_RUGBY:
			if ((ball = has_ball(p_ptr)) == -1) break;

			if (p_ptr->team == 1 && c_ptr->feat == FEAT_BGOAL) {
				teamscore[0]++;
				msg_format_near(Ind, "\377R%s scored a goal!!!", p_ptr->name);
				msg_print(Ind, "\377rYou scored a goal!!!");
				score = 1;
			}
			if (p_ptr->team == 2 && c_ptr->feat == FEAT_AGOAL) {
				teamscore[1]++;
				msg_format_near(Ind, "\377B%s scored a goal!!!", p_ptr->name);
				msg_print(Ind, "\377gYou scored a goal!!!");
				score = 1;
			}
			if (score) {
				object_type tmp_obj;
				s16b ox, oy;
				int try;

				p_ptr->energy = 0;
				snprintf(sstr, 80, "Score: \377RReds: %d  \377BBlues: %d", teamscore[0], teamscore[1]);
				msg_broadcast(0, sstr);

				for (try = 0; try < 1000; try++) {
					ox = p_ptr->px + 5 - rand_int(10);
					oy = p_ptr->py + 5 - rand_int(10);
					if (!in_bounds(oy, ox)) continue;
					if (!cave_floor_bold(zcave, oy, ox)) continue;
					tmp_obj = p_ptr->inventory[ball];
					tmp_obj.marked2 = ITEM_REMOVAL_NEVER;
					drop_near(TRUE, 0, &tmp_obj, -1, &p_ptr->wpos, oy, ox);
					s_printf("dropping at %d %d (%d)\n", ox, oy, try);
					inven_item_increase(Ind, ball, -999);
					inven_item_optimize(Ind, ball);
					break;
				}
				/* Move the player from the goal area */
				teleport_player_force(Ind, 20);
			}
			break;
		/* capture the flag */
		case EEGAME_CTF:
			break;
		default:
			break; /* gcc 3.4 actually wants this - mikaelh */
		}
	}
}

/*
 * Player processing that occurs at the end of a turn
 */
static void process_player_end(int Ind) {
	player_type *p_ptr = Players[Ind];

	//int x, y, i, j, new_depth, new_world_x, new_world_y;
	//int regen_amount, NumPlayers_old = NumPlayers;
	char attackstatus;

	//byte *w_ptr;

	/* slower 'running' movement over certain terrain */
	int real_speed = cfg.running_speed;
	cave_type *c_ptr, **zcave;

	if (!(zcave = getcave(&p_ptr->wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	if (Players[Ind]->conn == NOT_CONNECTED) return;


	/* --- Process various turn counters --- */

	/* count turns online, afk, and idle */
	p_ptr->turns_online++;
	if (p_ptr->afk) p_ptr->turns_afk++;
	if (p_ptr->idle) p_ptr->turns_idle++;
	else if (!p_ptr->afk) p_ptr->turns_active++;

	/* count how long they stay on a level (for EXTRA_LEVEL_FEELINGS) */
	p_ptr->turns_on_floor++;
	/* hack to indicate it to the player */
	if (p_ptr->turns_on_floor == TURNS_FOR_EXTRA_FEELING) Send_depth(Ind, &p_ptr->wpos);

	/* Crimes being forgotten over time */
	if (!(turn % (cfg.fps / 2))) {
		if (p_ptr->tim_blacklist) p_ptr->tim_blacklist--;
		if (p_ptr->tim_watchlist) p_ptr->tim_watchlist--;
		if (p_ptr->tim_jail && !p_ptr->wpos.wz) {
			p_ptr->tim_jail--;
			if (!p_ptr->tim_jail) {
				/* only release him from jail if he didn't already take the ironman jail dungeon escape route. */
				if (zcave[p_ptr->py][p_ptr->px].info & CAVE_JAIL) {
#ifdef JAILER_KILLS_WOR
					/* eat his WoR scrolls as suggested? */
					bool found = FALSE, one = TRUE;

					for (j = 0; j < INVEN_WIELD; j++) {
						if (!p_ptr->inventory[j].k_idx) continue;
						o_ptr = &p_ptr->inventory[j];
						if ((o_ptr->tval == TV_ROD) && (o_ptr->sval == SV_ROD_RECALL)) {
							if (found) one = FALSE;
							if (o_ptr->number > 1) one = FALSE;
							o_ptr->pval = 300;
							found = TRUE;
						}
					}
					if (found) {
						msg_format(Ind, "The jailer discharges your rod%s of recall.", one ? "" : "s");
						p_ptr->window |= PW_INVEN;
					}
#endif

					msg_print(Ind, "\377GYou are free to go!");
#ifdef JAIL_KICK
					p_ptr->tim_jail_delay = JAIL_KICK + 1;
#else
					p_ptr->tim_jail_delay = 1;
#endif
				}
			}
		}
		if (p_ptr->tim_jail_delay) {
			p_ptr->tim_jail_delay--;
			if (!p_ptr->tim_jail_delay) {
#ifdef JAIL_KICK
				if (JAIL_KICK > 0) msg_print(Ind, "\377sThe prison guards are out of patience and escort you out.");
#endif
				/* Get the jail door location */
				if (!p_ptr->house_num) teleport_player_force(Ind, 1); //should no longer happen as house_num is saved now between logins
				else {
					zcave[p_ptr->py][p_ptr->px].m_idx = 0;
					everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
					p_ptr->px = houses[p_ptr->house_num - 1].dx;
					p_ptr->py = houses[p_ptr->house_num - 1].dy;
					p_ptr->house_num = 0;
					teleport_player_force(Ind, 1);

					/* Hack: We started on the prison door, which isn't CAVE_STCK.
					   So we have to manually add a message and redraw the no-tele indicators. */
					msg_print(Ind, "\377sFresh air greets you as you leave the prison.");
					p_ptr->redraw |= PR_DEPTH; /* hack: depth colour indicates no-tele */
					p_ptr->redraw |= PR_BPR_WRAITH_PROB;
				}
			}
#ifdef JAIL_KICK
			else if (JAIL_KICK >= 12 && p_ptr->tim_jail_delay == JAIL_KICK / 2)
				msg_print(Ind, "\377sThe prison guards are waiting for you to leave the prison...");
#endif
		}
	}


	/* --- Player commands, auto-retaliation and movement --- */

	/* calculate effective running speed */
	eff_running_speed(&real_speed, p_ptr, c_ptr);

	/* inject a delayed command? */
	handle_XID(Ind);

	/* Try to execute any commands on the command queue. */
	process_pending_commands(p_ptr->conn);

#ifdef XID_REPEAT
	/* Hack to restore repeated ID command after Receive_inventory_revision() happened: */
	if (p_ptr && p_ptr->command_rep_temp) {
		/* Initiate a new delayed command that is identical to the previous one */
 #if 0
		//p_ptr->command_rep = p_ptr->command_rep_temp; <- red lag :-p
		p_ptr->delayed_index = p_ptr->delayed_index_temp;
		p_ptr->delayed_index_temp = -1;
		p_ptr->delayed_spell = p_ptr->delayed_spell_temp;
		p_ptr->current_item = p_ptr->current_item_temp;
		p_ptr->command_rep_temp = 0;
 #else
		/* Don't restore the command but reinject it from scratch */
		p_ptr->command_rep = p_ptr->command_rep_temp = p_ptr->command_rep_active = 0; //FALSE
		p_ptr->delayed_index = p_ptr->delayed_index_temp;
		p_ptr->delayed_index_temp = 0;
		p_ptr->delayed_spell = p_ptr->delayed_spell_temp;
		p_ptr->delayed_spell_temp = 0;
		p_ptr->current_item = p_ptr->current_item_temp;
		p_ptr->current_item_temp = 0;
 #endif
	}
#endif

	process_games(Ind); //todo: actually instead call this when changing grid ie moving/porting -> grid_affects_player() perhaps

	/* Mind Fusion/Control disables the char's automatic 'background' behaviour: No auto-retaliation and no running. */
	if (!(p_ptr->esp_link && p_ptr->esp_link_type && (p_ptr->esp_link_flags & LINKF_OBJ))) {
		/* Check for fire-till-kill and auto-retaliation */
		if (!p_ptr->requires_energy && /* <- new, required for allowing actions here (fire-till-kill)
						  at <= 100% energy to prevent character lock-up. - C. Blue */
		    !p_ptr->confused && !p_ptr->resting &&
		    (!p_ptr->autooff_retaliator || /* <- these conditions seem buggy/wrong/useless? */
		     !p_ptr->invuln))//&& !p_ptr->tim_manashield)))
		    {
			/* Prepare auto-ret/fire-till-kill mode energy requirements.
			   (Note: auto-ret is currently nothing special but always 1 x level_speed() as usual,
			    so for auto-ret nothing changed basically. Therefore firing with auto-ret will also
			    still have that small delay in the beginning, if player could fire multiple shots/round.
			    This cannot be changed easily because here we don't know yet with which methid the
			    player might auto-ret. - C. Blue) */
			int energy = level_speed(&p_ptr->wpos);

			/* is this line even required at all..? */
			if (p_ptr->shooting_till_kill && !target_okay(Ind)) p_ptr->shooting_till_kill = FALSE;

			/* Check for auto-retaliate */

			/* The 'old way' is actually the best way, because the initial delay of the 'new way',
			   when it happens, can be very irritating. The best way to fix perceived responsiveness
			   (of the old way) would be to not add full floor speed energy all at once, but in multiple
			   parts, to (ideally) immediately cover the energy loss for a single attack performed. */
#ifndef NEW_AUTORET_ENERGY
			/* old way - get the usual 'double initial attack' in.
			   Drawback: Have to wait for nearly a full turn (1-(1/attacksperround))
			   for FTK/meleeret to break out for performing a different action. -- this should be fixed now. May break out in 1/attacksperround now.) */
			if (p_ptr->energy >= energy) {
#else
			/* new way - allows to instantly break out and perform another action (quaff/read)
			   but doesn't give the 'double initial attack' anymore, just a normal, single attack.
			   Main drawback: Walking into a mob will not smoothly transgress into auto-ret the next turn,
			   but wait for an extra turn before it begins, ie taking 2 turns (minus '1 point of energy', to be exact) until first attack got in.
			   (Note that this e*2-1 amount of energy is also the maximum a player can store, according to limit_energy().) */
			if (p_ptr->energy >= (p_ptr->instant_retaliator ? energy : energy * 2 - 1)) {
#endif
				/* assume nothing will happen here */
				p_ptr->auto_retaliating = FALSE;

				/* New feat: @O inscription will break FTK to enter melee auto-ret instead */
				if (auto_retaliate_test(Ind)) p_ptr->shooting_till_kill = FALSE;

				if (p_ptr->shooting_till_kill) {
					/* stop shooting till kill if target is no longer available,
					   instead of casting a final time into thin air! */
					if (target_okay(Ind)) {
						p_ptr->auto_retaliating = TRUE;

#ifdef NEW_AUTORET_RESERVE_ENERGY
 #ifdef NEW_AUTORET_RESERVE_ENERGY_WORKAROUND
						if (!p_ptr->running)
 #endif
						if (!p_ptr->instant_retaliator) p_ptr->triggered_auto_attacking = TRUE;
#endif

						if (p_ptr->shoot_till_kill_spell) {
							cast_school_spell(Ind, p_ptr->shoot_till_kill_book, p_ptr->shoot_till_kill_spell - 1, 5, -1, 0);
							if (!p_ptr->shooting_till_kill) p_ptr->shoot_till_kill_spell = 0;
						} else if (p_ptr->shoot_till_kill_rcraft) {
							cast_rune_spell(Ind, p_ptr->FTK_e_flags, p_ptr->FTK_m_flags, 5);
						} else if (p_ptr->shoot_till_kill_mimic) {
							do_cmd_mimic(Ind, p_ptr->shoot_till_kill_mimic - 1 + 3, 5);
						} else if (p_ptr->shoot_till_kill_wand) {
							do_cmd_aim_wand(Ind, p_ptr->shoot_till_kill_wand, 5);
						} else if (p_ptr->shoot_till_kill_rod) {
							do_cmd_zap_rod(Ind, p_ptr->shoot_till_kill_rod, 5);
						} else {
							do_cmd_fire(Ind, 5);
#if 1 //what was the point of this hack again?..
							p_ptr->auto_retaliating = !p_ptr->auto_retaliating; /* hack, it's unset in do_cmd_fire IF it WAS successfull, ie reverse */
#endif
						}

						//not required really	if (p_ptr->ranged_double && p_ptr->shooting_till_kill) do_cmd_fire(Ind, 5);
						p_ptr->shooty_till_kill = FALSE; /* if we didn't succeed shooting till kill, then we don't intend it anymore */
					} else {
						p_ptr->shooting_till_kill = FALSE;
					}
				}

				/* Parania: Don't break fire-till-kill by starting to auto-retaliate when monster enters melee range!
				   (note: that behaviour was reported, but haven't reproduced it in local testing so far) */
				if (!p_ptr->shooting_till_kill) {
					/* Check for nearby monsters and try to kill them */
					/* If auto_retaliate returns nonzero than we attacked
					 * something and so should use energy.
					 */
					p_ptr->auto_retaliaty = TRUE; /* hack: prevent going un-AFK from auto-retaliating */
					if ((!p_ptr->auto_retaliating) /* aren't we doing fire_till_kill already? */
					    && (attackstatus = auto_retaliate(Ind))) { /* attackstatus seems to be unused! */
						p_ptr->auto_retaliating = TRUE;
#ifdef NEW_AUTORET_RESERVE_ENERGY
 #ifdef NEW_AUTORET_RESERVE_ENERGY_WORKAROUND
						if (!p_ptr->running)
 #endif
						if (!p_ptr->instant_retaliator) p_ptr->triggered_auto_attacking = TRUE;
#endif
						/* Use energy */
						//p_ptr->energy -= level_speed(p_ptr->dun_depth);
					}
					p_ptr->auto_retaliaty = FALSE;
				}

				/* Reset attack sfx counter in case player enabled half_sfx_attack or cut_sfx_attack. */
				if (!p_ptr->shooting_till_kill && !p_ptr->auto_retaliating) {
					p_ptr->count_cut_sfx_attack = 500;
					p_ptr->half_sfx_attack_state = FALSE;
				}
			} else p_ptr->auto_retaliating = FALSE; /* if no energy left, this is required to turn off the no-run-while-retaliate-hack */
		}

		/* ('Handle running' from above was originally at this place) */
		/* Handle running -- 5 times the speed of walking */
		//Verify: This code should not require additional modifications for NEW_AUTORET_ENERGY:reserve_energy feat
#ifdef NEW_AUTORET_ENERGY
		if (p_ptr->instant_retaliator || !p_ptr->shooting_till_kill) /* Required to keep allowing 'shooting on the run' */
 #if 0
 #ifdef NEW_AUTORET_RESERVE_ENERGY
    /* TODO: This currently doesn't work because nserver.c command-retrieval first checks if there is a turn of energy available,
	and THEN executes the command. However, the command might fail for different reasons than having not enough energy (or the
	command might not require the full amount of energy allocated - but this is probably not the case currently for any command,
	it's eg verified for do_cmd_fire() in nserver.c how much energy it really needs based on p_ptr->num_fire).
	That means that running will be stopped until a full turn of energy has been built up - for nothing! - and then is spent all
	at once at running, making us hop 5 grids at a time.
	Easiest way to reproduce: Hold down firing key while running without any valid target in sight -> running stops for a full turn of
	energy and then when receive_fire() doesn't complain into requires_energy but still cannot fire (as there is no target) and hence
	burn the energy, we'll run for the full worth of energy (ie 5 grids).
	The 'best' solution would be to rewrite ALL commands to report into requires_energy only if the only reason they failed to trigger
	was actually a lack of energy and nothing else (eg for do_cmd_fire(): valid target, ammo, all fine, just needing the energy now). */
		if (p_ptr->instant_retaliator || !p_ptr->requires_energy) /* While running, we'd have no energy left at all to start shooting */
 #endif
 #endif
#endif
		while (p_ptr->running && p_ptr->energy >= (level_speed(&p_ptr->wpos) * (real_speed + 1)) / real_speed) {
			char consume_full_energy;

			run_step(Ind, 0, &consume_full_energy);
			if (consume_full_energy)
				/* Consume a full turn of energy in case we have e.g. attacked a monster */
				p_ptr->energy -= level_speed(&p_ptr->wpos);
			else
				p_ptr->energy -= level_speed(&p_ptr->wpos) / real_speed;
		}
	}


	/* Notice stuff -- any reason to call it twice here in process_player_end()? Further down again, see APD's comment */
	if (p_ptr->notice) notice_stuff(Ind);

	/* XXX XXX XXX Pack Overflow */
	pack_overflow(Ind);

	/* Added for Holy Martyr, which was previously in process_player_end_aux() */
	if (!(turn % cfg.fps)) {
		/* Process Martyr independantly of level speed, in real time instead.
		   Otherwise it gives the player too much action time on deep levels at high speeds. */
		if (p_ptr->martyr) (void)set_martyr(Ind, p_ptr->martyr - 1);
		if (p_ptr->martyr_timeout) {
			p_ptr->martyr_timeout--;
			if (!p_ptr->martyr_timeout) {
#ifdef ENABLE_HELLKNIGHT
				if (p_ptr->ptrait == TRAIT_CORRUPTED && (
				    p_ptr->pclass == CLASS_HELLKNIGHT
 #ifdef ENABLE_CPRIEST
				    || p_ptr->pclass == CLASS_CPRIEST
 #endif
				    ))
					msg_print(Ind, "\376The maelstrom of chaos is ready to accept your sacrifice.");
				else
#endif
				msg_print(Ind, "\376The heavens are ready to accept your martyrium.");
			}
		}

		/* Process space/time anchor's no-tele field (used to be in process_player_end_aux()). */
		if (p_ptr->st_anchor) (void)set_st_anchor(Ind, p_ptr->st_anchor - 1);
	}

	/* Process things such as regeneration. */
	/* This used to be processed every 10 turns (check: this can't be right, too fast, must've been every 2 turns?),
	 * but I am changing it to be processed once every 5/6 of a "dungeon turn".
	 * This will make healing and poison faster with respect to real time < 1750 feet and slower > 1750 feet. - C. Blue
	 */
	if (!(turn % DUN_TURN_56(&p_ptr->wpos))) // level_speed is 750*5 at Bree town aka level 0 -> turn % 31.25, which is ~1/2s at cfg.fps 60
		if (!process_player_end_aux(Ind)) return;

	/* HACK -- redraw stuff a lot, this should reduce perceived latency. */
	/* This might not do anything, I may have been silly when I added this. -APD */
	/* Notice stuff (if needed) */
	if (p_ptr->notice) notice_stuff(Ind);

	/* Update stuff (if needed) */
	if (p_ptr->update) update_stuff(Ind);

	/* Redraw stuff (if needed) */
	if (p_ptr->redraw2) redraw2_stuff(Ind);
	if (p_ptr->redraw) redraw_stuff(Ind);

	/* Redraw stuff (if needed) */
	if (p_ptr->window) window_stuff(Ind);
}


bool stale_level(struct worldpos *wpos, int grace) {
	time_t now;
	struct dun_level *l_ptr;

	/* Hack -- towns are static for good? too spammy */
	//if (istown(wpos)) return(FALSE);
	/* Hack -- make dungeon towns static though? too cheezy */
	//if (isdungeontown(wpos)) return(FALSE);

	if ((l_ptr = getfloor(wpos)) && (l_ptr->flags2 & LF2_STATIC)) {
//s_printf("stale_level LF2_STATIC %d,%d,%d\n", wpos->wx, wpos->wy, wpos->wz);
		return(FALSE);
	}

#if 0 /* instead done in calling function! */
	/* Hack: In IDDC, all floors are stale for 2 minutes to allow logging back in if
	         someone's connection dropped without the floor going stale right away,
	         and towns to allow for easy item transfers.
	         Small drawback: If someone logs back on after having taken a break from IDDC
	         and he finds himself in a non-accessible area, he'll have to wait for
	         2 minutes instead of the usual 10 seconds till the floor regens. */
	if (in_irondeepdive(wpos) && grace < 120) grace = 120;
#endif

	now = time(&now);
	if (wpos->wz) {
		struct dungeon_type *d_ptr;
		struct dun_level *l_ptr;

		d_ptr = getdungeon(wpos);
		if (!d_ptr) return(FALSE);
		l_ptr = &d_ptr->level[ABS(wpos->wz) - 1];
#if DEBUG_LEVEL > 3
		s_printf("%s  now:%ld last:%ld diff:%ld grace:%d players:%d\n", wpos_format(0, wpos), now, l_ptr->lastused, now-l_ptr->lastused,grace, players_on_depth(wpos));
#endif
		/* Hacky: Combine checks for normal death/quit static time (lastused) and for anti-scum static time (creationtime):
		   lastused is set by death/quit staticing, but it is 0 when stair-scumming. */
		if (l_ptr->lastused) {
			if (now - l_ptr->lastused > grace) return(TRUE);
		} else if (now - l_ptr->creationtime > grace) return(TRUE);
	} else if (now - wild_info[wpos->wy][wpos->wx].surface.lastused > grace) {
#if 0
		/* Never allow dealloc where there are houses */
		/* For now at least */
		int i;

		for (i = 0; i < num_houses; i++) {
			if (inarea(wpos, &houses[i].wpos)) {
				if (!(houses[i].flags & HF_DELETED)) return(FALSE);
			}
		}
#endif
		return(TRUE);
	}
	return(FALSE);
}

static void do_unstat(struct worldpos *wpos, byte fast_unstat) {
	int j;
	struct dun_level *l_ptr = getfloor(wpos);

#if 1 /* this LF2_STATIC check isn't really needed here, the one in stale_level() is sufficient */
	/* For events or admin intervention: Floor flag 'STATIC' keeps floor artificially static until floor is deallocated or flag is cleared. */
	if (l_ptr && (l_ptr->flags2 & LF2_STATIC)) {
//s_printf("do_unstat LF2_STATIC %d,%d,%d\n", wpos->wx, wpos->wy, wpos->wz);
		return;
	}
#endif

	/* Highlander Tournament sector000 is static while players are in dungeon! */
	if (in_sector000(wpos) && sector000separation) return;

	/* Arena Monster Challenge */
	if (ge_special_sector && in_arena(wpos)) return;

	// Anyone on this depth?
	for (j = 1; j <= NumPlayers; j++)
		if (inarea(&Players[j]->wpos, wpos)) return;

	// --- If this level is static and no one is actually on it: ---

	/* limit static time in Ironman Deep Dive Challenge a lot */
	if (in_irondeepdive(wpos)) {
		if (isdungeontown(wpos)) {
			if (stale_level(wpos, 300)) new_players_on_depth(wpos, 0, FALSE);//5 min
		} else if ((getlevel(wpos) < cfg.min_unstatic_level) && (0 < cfg.min_unstatic_level)) {
			/* still 2 minutes static for very shallow levels */
			if (stale_level(wpos, 120)) new_players_on_depth(wpos, 0, FALSE);//2 min
		} else if (stale_level(wpos, 300)) new_players_on_depth(wpos, 0, FALSE);//5 min (was 10)
	} else {
		switch (fast_unstat) {
		case 1: //SAURON_FLOOR_FAST_UNSTAT
			j = 60 * 60; //1h
			break;
		case 2: //DI_DEATH_FATE
#if 0 /* Not working instantly. Done in process_player_change_wpos() instead. */
			new_players_on_depth(wpos, 0, FALSE);
			if (getcave(wpos)) dealloc_dungeon_level(wpos);
#else
			j = 0;
			break;
#endif
		default: //normal dungeon floor (fast_unstat = FALSE)
			j = cfg.level_unstatic_chance * getlevel(wpos) * 60;
			break;
		}

		/* makes levels between 50ft and min_unstatic_level unstatic on player saving/quiting game/leaving level DEG */
		if (((getlevel(wpos) < cfg.min_unstatic_level) && (0 < cfg.min_unstatic_level)) ||
		    stale_level(wpos, j))
			new_players_on_depth(wpos, 0, FALSE);
	}
}

/*
 * 24 hourly scan of houses - should the odd house be owned by
 * a non player. Hopefully never, but best to save admin work.
 */
static void scan_houses() {
	int i;
	//int lval;
	s_printf("Doing house maintenance\n");
	for (i = 0; i < num_houses; i++) {
		if (!houses[i].dna->owner) continue;
		switch (houses[i].dna->owner_type) {
			case OT_PLAYER:
				if (!lookup_player_name(houses[i].dna->owner)) {
					s_printf("Found old player houses. ID: %d\n", houses[i].dna->owner);
					kill_houses(houses[i].dna->owner, OT_PLAYER);
				}
				break;
			case OT_PARTY:
				if (!strlen(parties[houses[i].dna->owner].name)) {
					s_printf("Found old party houses. ID: %d\n", houses[i].dna->owner);
					kill_houses(houses[i].dna->owner, OT_PARTY);
				}
				break;
			case OT_GUILD:
				if (!strlen(guilds[houses[i].dna->owner].name)) {
					s_printf("Found old guild houses. ID: %d\n", houses[i].dna->owner);
					kill_houses(houses[i].dna->owner, OT_GUILD);
				}
				break;
		}
	}
	s_printf("Finished house maintenance\n");
}

/* If the level unstaticer is not disabled, try to.
 * Now it's done every 60*fps, so better raise the
 * rate as such.	- Jir -
 */
/*
 * Deallocate all non static levels. (evileye)
 */
static void purge_old() {
	int x, y, i;
	struct wilderness_type *w_ptr;
	struct dungeon_type *d_ptr;

	//if (cfg.level_unstatic_chance > 0)
	{
		struct worldpos twpos;

		twpos.wz = 0;
#if 0 /* we get called every second or even less often? */
		for (y = 0; y < MAX_WILD_Y; y++) {
#else /* we get called every frame, to distribute the workload? */
		y = turn % MAX_WILD_Y;
		{
#endif
			twpos.wy = y;
			for (x = 0; x < MAX_WILD_X; x++) {
				twpos.wx = x;
				w_ptr = &wild_info[twpos.wy][twpos.wx];

				if (cfg.level_unstatic_chance > 0 &&
				    players_on_depth(&twpos))
					do_unstat(&twpos, FALSE);

				if (!players_on_depth(&twpos) &&
				    !istown(&twpos) &&
				    !isdungeontown(&twpos) &&
				    getcave(&twpos) && stale_level(&twpos, cfg.anti_scum))
					dealloc_dungeon_level(&twpos);

				if (w_ptr->flags & WILD_F_UP) {
					d_ptr = w_ptr->tower;
					for (i = 1; i <= d_ptr->maxdepth; i++) {
						twpos.wz = i;
						if (cfg.level_unstatic_chance > 0 &&
						    players_on_depth(&twpos))
							do_unstat(&twpos, d_ptr->type == DI_DEATH_FATE ? 2 : ((d_ptr->type == DI_MT_DOOM && i == d_ptr->maxdepth) ? 1 : 0));

						if (!players_on_depth(&twpos) && getcave(&twpos) &&
						    stale_level(&twpos, cfg.anti_scum))
							dealloc_dungeon_level(&twpos);
					}
				}
				if (w_ptr->flags & WILD_F_DOWN) {
					d_ptr = w_ptr->dungeon;
					for (i = 1; i <= d_ptr->maxdepth; i++) {
						twpos.wz = -i;
						if (cfg.level_unstatic_chance > 0 &&
						    players_on_depth(&twpos))
							do_unstat(&twpos, d_ptr->type == DI_DEATH_FATE ? 2 : ((d_ptr->type == DI_MT_DOOM && i == d_ptr->maxdepth) ? 1 : 0));

						if (!players_on_depth(&twpos) && getcave(&twpos) &&
						    stale_level(&twpos, cfg.anti_scum))
							dealloc_dungeon_level(&twpos);
					}
				}
				twpos.wz = 0;
			}
		}
	}
}

/*
 * Log suspicious items inside hollow houses to tomenet.log
 * TODO: Check for OT_GUILD (or guild will be mere den of cheeze)
 */
void cheeze(object_type *o_ptr) {
#if CHEEZELOG_LEVEL > 3
	int j, owner;

	/* check for inside a house */
	for (j = 0; j < num_houses; j++) {
		if (!inarea(&houses[j].wpos, &o_ptr->wpos) || !fill_house(&houses[j], FILL_OBJECT, o_ptr)) continue;

		switch (houses[j].dna->owner_type) {
		case OT_PLAYER:
			if (o_ptr->owner != houses[j].dna->owner && o_ptr->level > lookup_player_level(houses[j].dna->owner))
				s_printf("Suspicious item: (%d,%d) Owned by %s, in %s's house. (%d,%d)\n", o_ptr->wpos.wx, o_ptr->wpos.wy, lookup_player_name(o_ptr->owner), lookup_player_name(houses[j].dna->owner), o_ptr->level, lookup_player_level(houses[j].dna->owner));
			break;
		case OT_PARTY:
			if ((owner = lookup_player_id(parties[houses[j].dna->owner].owner)) && o_ptr->owner != owner) {
				if (o_ptr->level > lookup_player_level(owner))
					s_printf("Suspicious item: (%d,%d) Owned by %s, in %s party house. (%d,%d)\n", o_ptr->wpos.wx, o_ptr->wpos.wy, lookup_player_name(o_ptr->owner), parties[houses[j].dna->owner].name, o_ptr->level, lookup_player_level(owner));
			}
			break;
		case OT_GUILD:
			if ((owner = guilds[houses[j].dna->owner].master) && o_ptr->owner != owner) {
				if (o_ptr->level > lookup_player_level(owner))
					s_printf("Suspicious item: (%d,%d) Owned by %s, in %s party house. (%d,%d)\n", o_ptr->wpos.wx, o_ptr->wpos.wy, lookup_player_name(o_ptr->owner), guilds[houses[j].dna->owner].name, o_ptr->level, lookup_player_level(owner));
			}
		}
		break;
	}
#endif // CHEEZELOG_LEVEL > 3
}


/*
 * Log suspicious items inside trad houses to tomenet.log
 * Traditional (Vanilla) houses version of cheeze()	- Jir -
 */
#ifndef USE_MANG_HOUSE_ONLY
void cheeze_trad_house() {
 #if CHEEZELOG_LEVEL > 3
	int i, j;
	house_type *h_ptr;
	object_type *o_ptr;

	/* check for inside a house */
	for (j = 0;j < num_houses; j++) {
		h_ptr = &houses[j];
		if (h_ptr->dna->owner_type == OT_PLAYER) {
			for (i = 0; i < h_ptr->stock_num; i++) {
				o_ptr = &h_ptr->stock[i];
				if (o_ptr->owner != h_ptr->dna->owner) {
					if (o_ptr->level > lookup_player_level(h_ptr->dna->owner))
						s_printf("Suspicious item: (%d,%d) Owned by %s, in %s's trad house(%d). (%d,%d)\n",
						    o_ptr->wpos.wx, o_ptr->wpos.wy,
						    lookup_player_name(o_ptr->owner),
						    lookup_player_name(h_ptr->dna->owner),
						    j, o_ptr->level,
						    lookup_player_level(h_ptr->dna->owner));
				}
			}
		}
		else if (h_ptr->dna->owner_type == OT_PARTY) {
			int owner;

			if ((owner = lookup_player_id(parties[h_ptr->dna->owner].owner))) {
				for (i = 0; i < h_ptr->stock_num; i++) {
					o_ptr = &h_ptr->stock[i];
					if (o_ptr->owner != owner) {
						if (o_ptr->level > lookup_player_level(owner))
							s_printf("Suspicious item: (%d,%d) Owned by %s, in %s party trad house(%d). (%d,%d)\n",
							    o_ptr->wpos.wx, o_ptr->wpos.wy,
							    lookup_player_name(o_ptr->owner),
							    parties[h_ptr->dna->owner].name,
							    j, o_ptr->level, lookup_player_level(owner));
					}
				}
			}
		}
		else if (h_ptr->dna->owner_type == OT_GUILD) {
			int owner;

			if ((owner = guilds[h_ptr->dna->owner].master)) {
				for (i = 0; i < h_ptr->stock_num; i++) {
					o_ptr = &h_ptr->stock[i];
					if (o_ptr->owner != owner) {
						if (o_ptr->level > lookup_player_level(owner))
							s_printf("Suspicious item: (%d,%d) Owned by %s, in %s guild trad house(%d). (%d,%d)\n",
							    o_ptr->wpos.wx, o_ptr->wpos.wy,
							    lookup_player_name(o_ptr->owner),
							    guilds[h_ptr->dna->owner].name,
							    j, o_ptr->level, lookup_player_level(owner));
					}
				}
			}
		}
	}
 #endif // CHEEZELOG_LEVEL > 3
}
#endif	// USE_MANG_HOUSE_ONLY


/* Change item mode of all items inside a house to the according house owner mode */
void house_contents_chmod(object_type *o_ptr) {
#if CHEEZELOG_LEVEL > 3
	int j, owner;

	/* check for inside a house */
	for (j = 0; j < num_houses; j++) {
		if (inarea(&houses[j].wpos, &o_ptr->wpos) && fill_house(&houses[j], FILL_OBJECT, o_ptr)) {
			switch (houses[j].dna->owner_type) {
			case OT_PLAYER:
				o_ptr->mode = lookup_player_mode(houses[j].dna->owner));
				break;
			case OT_PARTY:
				if ((owner = lookup_player_id(parties[houses[j].dna->owner].owner)))
					o_ptr->mode = lookup_player_mode(owner));
				break;
			}
			break;
		}
	}
#endif // CHEEZELOG_LEVEL > 3
}


/* Traditional (Vanilla) houses version */
#ifndef USE_MANG_HOUSE_ONLY
void tradhouse_contents_chmod() {
#if CHEEZELOG_LEVEL > 3
	int i, j;
	house_type *h_ptr;
	object_type *o_ptr;

	/* check for inside a house */
	for (j = 0; j < num_houses; j++) {
		h_ptr = &houses[j];
		if (h_ptr->dna->owner_type == OT_PLAYER) {
			for (i = 0; i < h_ptr->stock_num; i++) {
				o_ptr = &h_ptr->stock[i];
				o_ptr->mode = lookup_player_mode(houses[j].dna->owner));
			}
		} else if (h_ptr->dna->owner_type == OT_PARTY) {
			int owner;

			if ((owner = lookup_player_id(parties[h_ptr->dna->owner].owner))) {
				for (i = 0; i < h_ptr->stock_num; i++) {
					o_ptr = &h_ptr->stock[i];
					o_ptr->mode = lookup_player_mode(owner));
				}
			}
		}
	}
#endif // CHEEZELOG_LEVEL > 3
}
#endif	// USE_MANG_HOUSE_ONLY



/*
 * The purpose of this function is to scan through all the
 * game objects in the world.
 *
 * In the case for non house/inn objects, essentially a TTL is
 * set. This protects them from instant loss, should a player
 * drop something just at the wrong time, but it should get
 * rid of some town junk.
 *
 * We're called once per minute, from process_various().
 */
//TODO: Do some work distribution over frames, this function seems CPU-hungry, looping through all items when it's called. */
static void scan_objs() {
	int i, cnt = 0, dcnt = 0;
	object_type *o_ptr;
	cave_type **zcave;

	/* objects time-outing disabled? */
	if (!cfg.surface_item_removal && !cfg.dungeon_item_removal) return;

	for (i = 0; i < o_max; i++) {
		o_ptr = &o_list[i];
		if (!o_ptr->k_idx) continue;

		/* Skip monster inventory items (hm why actually) */
		if (o_ptr->held_m_idx) continue;

		/* not dropped on player death or generated on the floor? (or special stuff) */
		if (o_ptr->marked2 == ITEM_REMOVAL_NEVER ||
		    o_ptr->marked2 == ITEM_REMOVAL_HOUSE)
			continue;

		/* Take care of items of an erased player, which aren't usable by anyone anymore, inside the inn too. */
		if (o_ptr->owner == MAX_ID + 1) {
			/* Eat all non-transferrable items:
			   Starter items and level 0 items that aren't rescue-exchangeable either */
			if (((o_ptr->mode & MODE_STARTER_ITEM) || !o_ptr->level) && !exceptionally_shareable_item(o_ptr)) {
				delete_object_idx(i, TRUE, FALSE);
				cnt++;
				dcnt++;
				continue;
			}
		}

		/* Make town Inns a safe place to store (read: cheeze) items,
		   at least as long as the town level is allocated. - C. Blue */
		if ((zcave = getcave(&o_ptr->wpos))
		    && in_bounds_array(o_ptr->iy, o_ptr->ix) //paranoia, as we checked for held_m_idx above already
		    && (f_info[zcave[o_ptr->iy][o_ptr->ix].feat].flags1 & FF1_PROTECTED)) continue;

		/* check items on the world's surface */
		if (!o_ptr->wpos.wz && cfg.surface_item_removal) {
			if (in_bounds_array(o_ptr->iy, o_ptr->ix)) { //paranoia
				if (o_ptr->marked2 == ITEM_REMOVAL_QUICK) {
					if (++o_ptr->marked >= 2) {
						delete_object_idx(i, TRUE, TRUE);
						dcnt++;
					}
				} else if (o_ptr->marked2 == ITEM_REMOVAL_MONTRAP) {
					if (++o_ptr->marked >= 120) {
						delete_object_idx(i, TRUE, TRUE);
						dcnt++;
					}
				} else if (++o_ptr->marked >= ((like_artifact_p(o_ptr) || /* Stormbringer too */
				    (o_ptr->note && !o_ptr->owner))?
				    cfg.surface_item_removal * 3 : cfg.surface_item_removal) /* ITEM_REMOVAL_NORMAL [10 min, all normal surface area drops, including towns, excluding houses]  */
				    + (o_ptr->marked2 == ITEM_REMOVAL_DEATH_WILD ? cfg.death_wild_item_removal : 0)
				    + (o_ptr->marked2 == ITEM_REMOVAL_LONG_WILD ? cfg.long_wild_item_removal : 0)
				    ) {
					/* Artifacts and objects that were inscribed and dropped by
					the dungeon master or by unique monsters on their death
					stay n times as long as cfg.surface_item_removal specifies */
					delete_object_idx(i, TRUE, TRUE);
					dcnt++;
				}
			}

			/* Also perform a 'cheeze check' */
 #if CHEEZELOG_LEVEL > 1
			else
  #if CHEEZELOG_LEVEL < 4
			/* ..only once an hour. (logs would fill the hd otherwise ;( */
			if (!(turn % (cfg.fps * 3600)))
  #endif	/* CHEEZELOG_LEVEL (4) */
				cheeze(o_ptr);
 #endif	/* CHEEZELOG_LEVEL (1) */

			/* count amount of items that were checked */
			cnt++;
		}

		/* check items on dungeon/tower floors */
		else if (o_ptr->wpos.wz && cfg.dungeon_item_removal) {
			if (in_bounds_array(o_ptr->iy, o_ptr->ix)) { //paranoia
				/* Artifacts and objects that were inscribed and dropped by
				the dungeon master or by unique monsters on their death
				stay n times as long as cfg.surface_item_removal specifies */
				if (++o_ptr->marked >= ((artifact_p(o_ptr) ||
				    (o_ptr->note && !o_ptr->owner)) ?
				    cfg.dungeon_item_removal * 3 : cfg.dungeon_item_removal)) {
					delete_object_idx(i, TRUE, TRUE);
					dcnt++;
				}
			}

			/* Also perform a 'cheeze check' */
 #if CHEEZELOG_LEVEL > 1
			else
  #if CHEEZELOG_LEVEL < 4
			/* ..only once an hour. (logs would fill the hd otherwise ;( */
			if (!(turn % (cfg.fps * 3600)))
  #endif	/* CHEEZELOG_LEVEL (4) */
				cheeze(o_ptr);
 #endif	/* CHEEZELOG_LEVEL (1) */

			/* count amount of items that were checked */
			cnt++;
		}
	}

	/* log result */
	if (dcnt) s_printf("Scanned %d objects. Removed %d.\n", cnt, dcnt);

#ifndef USE_MANG_HOUSE_ONLY
	/* Additional cheeze check for all those items inside of mangband-style houses */
 #if CHEEZELOG_LEVEL > 1
  #if CHEEZELOG_LEVEL < 4
	if (!(turn % (cfg.fps * 3600)))
  #endif	/* CHEEZELOG_LEVEL (4) */
		cheeze_trad_house();
 #endif	/* CHEEZELOG_LEVEL (1) */
#endif	/* USE_MANG_HOUSE_ONLY */

}


/* NOTE: this can cause short freeze if many stores exist,
 * so it isn't called from process_various any more.
 * the store changes their stocks when a player enters a store.
 *
 * (However, this function can be called by admin characters)
 */
void store_turnover() {
	int i, n;

	for (i = 0; i < numtowns; i++) {
		/* Maintain each shop (except home and auction house) */
		//for (n = 0; n < MAX_BASE_STORES - 2; n++)
		for (n = 0; n < max_st_idx; n++) {
			/* Maintain */
			store_maint(&town[i].townstore[n]);
		}

		/* Sometimes, shuffle the shopkeepers */
		if (rand_int(STORE_SHUFFLE) == 0) {
			/* Shuffle a random shop (except home and auction house) */
			//store_shuffle(&town[i].townstore[rand_int(MAX_BASE_STORES - 2)]);
			store_shuffle(&town[i].townstore[rand_int(max_st_idx)]);
		}
	}
}

/*
 * This function handles "global" things such as the stores,
 * day/night in the town, etc.
 */
/* Added the japanese unique respawn patch -APD-
   It appears that each moment of time is equal to 10 minutes?
*/
/* called only every cfg.fps/6 turns
*/
/* WARNING: Every if-check in here should test at a resolution compatible to 'turn % (cfg.fps/6)'
   accordingly, otherwise depending on cfg.fps it might be skipped sometimes, which may or
   may not be critical depending on what it does! - C. Blue */
static void process_various(void) {
	int i, j, k;
	int h = 0, m = 0, s = 0, dwd = 0, dd = 0, dm = 0, dy = 0;
#ifndef ARCADE_SERVER
	time_t now;
	struct tm *tmp;
#endif
	//cave_type *c_ptr;
	player_type *p_ptr;

	//char buf[1024];

	/* this TomeCron stuff could be merged at some point
	   to improve efficiency. ;) */

	do_xfers(); /* handle filetransfers per second */

	/* Save the server state occasionally */
	if (!(turn % ((NumPlayers ? 10L : 1000L) * SERVER_SAVE))) {
		save_server_info();

		/* Save each player */
		for (i = 1; i <= NumPlayers; i++) {
			/* Save this player */
			save_player(i);
		}
	}

#if 0 /* might skip an hour if transition is unprecise, ie 1:59 -> 3:00 */
	/* Extra LUA function in custom.lua */
	time(&now);
	tmp = localtime(&now);
	h = tmp->tm_hour;
	m = tmp->tm_min;
	s = tmp->tm_sec;

	if (!(turn % (cfg.fps * 3600)))
		exec_lua(0, format("cron_1h(\"%s\", %d, %d, %d)", showtime(), h, m, s));
#else
 #ifndef ARCADE_SERVER
	if (!(turn % (cfg.fps * 10))) { /* call every 10 seconds instead of every 10 turns, to save some CPU time (yeah well...)*/
		/* Extra LUA function in custom.lua */
		time(&now);
		tmp = localtime(&now);
		h = tmp->tm_hour;
		m = tmp->tm_min;
		s = tmp->tm_sec;
		if (h != cron_1h_last_hour) {
			exec_lua(0, format("cron_1h(\"%s\", %d, %d, %d)", showtime(), h, m, s));
			cron_1h_last_hour = h;
		}
	}
 #endif
#endif

	/* daily maintenance */
	if (!(turn % (cfg.fps * 86400))) {
		s_printf("24 hours maintenance cycle\n");
		scan_characters();
		scan_accounts();
		scan_houses();
#ifdef IRONDEEPDIVE_MIXED_TYPES
		(void)scan_iddc();
#endif

#ifdef FIREWORK_DUNGEON
		init_firework_dungeon();
#endif

		if (cfg.auto_purge) {
			s_printf("previous server status: m_max(%d) o_max(%d)\n",
					m_max, o_max);
			compact_monsters(0, TRUE);
			compact_objects(0, TRUE);
			//compact_traps(0, TRUE);
			s_printf("current server status:  m_max(%d) o_max(%d)\n",
					m_max, o_max);
		}
#if DEBUG_LEVEL > 1
		else s_printf("Current server status:  m_max(%d) o_max(%d)\n",
				m_max, o_max);
#endif
		s_printf("Finished maintenance\n");
		get_date(&dwd, &dd, &dm, &dy);
		exec_lua(0, format("cron_24h(\"%s\", %d, %d, %d, %d, %d, %d, %d)", showtime(), h, m, s, dwd, dd, dm, dy));

		/* bbs_add_line("--- new day line ---"); */

		/* notify admins? */
		for (i = 1; i <= NumPlayers; i++) {
			if (!is_admin(Players[i])) continue;
			msg_format(i, "\374\377y[24-h maintenance/cron finished - %s]", showtime());
		}
	}
	/* Every 6 hours */
	if (!(turn % (cfg.fps * 21600))) {
		/* In case season_halloween is up - allow re-farming the pumpkin, especially important if there isn't much traffic on the server */
		great_pumpkin_killer1[0] = great_pumpkin_killer2[0] = 0;
	}

#if 0 /* disable for now - mikaelh */
	if (!(turn % (cfg.fps * 50))) {
		/* Tell the scripts that we're alive */
		update_check_file();
	}
#endif

	/* Things handled once per hour */
	if (!(turn % (cfg.fps * 3600))) {
		/* Purge leaderless guilds after a while */
		for (i = 1; i < MAX_GUILDS; i++) {
			if (!guilds[i].members) continue;
			if (!lookup_player_name(guilds[i].master) &&
			    guilds[i].timeout++ >= 24 * 7)
				guild_timeout(i);
		}
	}

	/* Handle certain things once a minute */
	if (!(turn % (cfg.fps * 60))) {
		monster_race *r_ptr;

		check_xorders();	/* check for expiry of extermination orders */
		check_banlist();	/* unban some players */
		scan_objs();		/* scan objects and houses */

		if (dungeon_store_timer) dungeon_store_timer--; /* Timeout */
		if (dungeon_store2_timer) dungeon_store2_timer--; /* Timeout */

		if (great_pumpkin_timer > 0) great_pumpkin_timer--; /* HALLOWEEN */
		if (great_pumpkin_duration > 0) {
			great_pumpkin_duration--;
			if (!great_pumpkin_duration) {
				monster_type *m_ptr;
				int m_idx;

				for (k = m_top - 1; k >= 0; k--) {
					/* Access the index */
					m_idx = m_fast[k];
					/* Access the monster */
					m_ptr = &m_list[m_idx];
					/* Excise "dead" monsters */
					if (!m_ptr->r_idx) {
						/* Excise the monster */
						m_fast[k] = m_fast[--m_top];
						/* Skip */
						continue;
					}
					/* Players of too high level cannot participate in killing attemps (anti-cheeze) */
					/* search for Great Pumpkins */
					if (m_ptr->r_idx == RI_PUMPKIN) {
						msg_print_near_monster(m_idx, "\377oThe Great Pumpkin wails and suddenly vanishes into thin air!");
						s_printf("HALLOWEEN: The Great Pumpkin despawned from %d,%d,%d.\n", m_ptr->wpos.wx, m_ptr->wpos.wy, m_ptr->wpos.wz);
						delete_monster_idx(m_idx, TRUE);
						//note_spot_depth(&p_ptr->wpos, y, x);
						great_pumpkin_timer = rand_int(2); /* fast respawn if not killed! */
					}
				}
			}
			else if (great_pumpkin_duration <= 5) {
				monster_type *m_ptr;
				int m_idx;

				for (k = m_top - 1; k >= 0; k--) {
					/* Access the index */
					m_idx = m_fast[k];
					/* Access the monster */
					m_ptr = &m_list[m_idx];
					/* Excise "dead" monsters */
					if (!m_ptr->r_idx) {
						/* Excise the monster */
						m_fast[k] = m_fast[--m_top];
						/* Skip */
						continue;
					}
					/* Players of too high level cannot participate in killing attemps (anti-cheeze) */
					/* search for Great Pumpkins */
					if (m_ptr->r_idx == RI_PUMPKIN) {
						msg_print_near_monster(m_idx, "\377oThe Great Pumpkin wails and seems to fade..");
						break;
					}
				}
			}
		}

#ifdef TEST_SERVER /* Enable NR collapse? (Possibly either teleporting everyone out or killing everyone who didn't on his own teleport out?) */
 #warning "Collapsing Nether Realm"
		/* Nether Realm completely collapses? -- To prohibit using this bottom floor as save operations base maybe */
		if (nether_realm_collapsing) {
			struct worldpos wpos = {netherrealm_wpos_x, netherrealm_wpos_y, netherrealm_end_wz};

			nether_realm_collapsing--;

			/* Give warning in the meantime */
			if (nether_realm_collapsing == 10) floor_msg_format(0, &wpos, "\377RThis plane of the nether realm seems to be in the process of collapsing..");
			else if (nether_realm_collapsing == 5) floor_msg_format(0, &wpos, "\377RThis plane of the nether realm seems to be on the verge of collapsing, beware!");
			else if (nether_realm_collapsing == 1) floor_msg_format(0, &wpos, "\377lThis plane of the nether realm will collapse any moment, get out while you can!");

			/* Eventually collapse! */
			if (!nether_realm_collapsing) {
				int i;
				player_type *p_ptr;

 #ifdef USE_SOUND_2010
				sound_floor_vol(&wpos, "destruction", NULL, SFX_TYPE_AMBIENT, 100); //ambient for impied lightning visuals, yet not weather-related
 #endif

				for (i = 1; i <= NumPlayers; i++) {
					p_ptr = Players[i];
					if (!inarea(&p_ptr->wpos, &wpos)) continue;

 #if 1 /* Recall players out? */
					p_ptr->recall_pos.wx = WPOS_SECTOR000_X;
					p_ptr->recall_pos.wy = WPOS_SECTOR000_Y;
					p_ptr->recall_pos.wz = WPOS_SECTOR000_Z;
					p_ptr->new_level_method = LEVEL_RAND;
					recall_player(i, "");
  #ifdef USE_SOUND_2010
					//handle_music(i); //superfluous? */
  #endif
 #else /* Kill players who didn't leave on their own? */
					strcpy(p_ptr->died_from, "collapsing nether plane");
					p_ptr->died_from_ridx = 0;
					p_ptr->deathblow = 0;
					player_death(i);
 #endif
				}

				/* Unstatic the bottom floor as it collapsed */
				unstatic_level(&wpos);
				if (getcave(&wpos)) dealloc_dungeon_level(&wpos);
			}
		}
#endif

		if (season_xmas) { /* XMAS */
			if (santa_claus_timer > 0) santa_claus_timer--;
			if (santa_claus_timer == 0) {
				cave_type **zcave = getcave(BREE_WPOS_P);

				if (zcave) { /* anyone in town? */
					int x, y, tries = 50;

					/* Try nine locations */
					while (--tries) {
						/* Pick location nearby hard-coded town centre */
						scatter(BREE_WPOS_P, &y, &x, 34, 96, 10, 0);

						/* Require "empty" floor grids */
						if (!cave_empty_bold(zcave, y, x)) continue;

						if (place_monster_aux(BREE_WPOS_P, y, x, RI_SANTA2, FALSE, FALSE, 0, 0) == 0) {
							s_printf("%s XMAS: Generated Santa Claus.\n", showtime());
							santa_claus_timer = -1; /* put generation on hold */
							break;
						}
					}
					/* Still no Santa? Induce fast respawn, probably paranoia */
					if (santa_claus_timer == 0) santa_claus_timer = 1;
				}
			}
		}

		for (i = 1; i <= NumPlayers; i++) {
			p_ptr = Players[i];

			/* Update the player retirement timers */
			// If our retirement timer is set
			if (p_ptr->retire_timer > 0) {
				k = p_ptr->retire_timer;

				// Decrement our retire timer
				j = k - 1;

				// Alert him
				if (j <= 60)
					msg_format(i, "\377rYou have %d minute%s of tenure left.", j, j > 1 ? "s" : "");
				else if (j <= 1440 && !(k % 60))
					msg_format(i, "\377yYou have %d hours of tenure left.", j / 60);
				else if (!(k % 1440))
					msg_format(i, "\377GYou have %d days of tenure left.", j / 1440);

				// If the timer runs out, forcibly retire
				// this character.
				if (!j) do_cmd_suicide(i);
			}
		}

		/* Reswpan for kings' joy  -Jir- */
		/* Update the unique respawn timers */
		/* I moved this out of the loop above so this may need some
		 * tuning now - mikaelh */
		if (max_rur_idx) /* Paranoia: No unique monsters available? */
		for (j = 1; j <= NumPlayers; j++) {
			p_ptr = Players[j];
			if (!p_ptr->total_winner) continue;
			if (istownarea(&p_ptr->wpos, MAX_TOWNAREA)) continue; /* allow kings idling instead of having to switch chars */

			/* Randomly pick a basically eligible unique monster. */
			k = rand_int(max_rur_idx);
			i = rur_info_map[k];
			r_ptr = &r_info[i];

			/* Unique isn't dead or fails respawn roll? Then this player is off the hook for now! */
			if (p_ptr->r_killed[i] != 1 ||
			    rand_int(cfg.unique_respawn_time * (r_ptr->level + 1)) > 9)
				continue;

			/* "Ressurect" the unique */
			p_ptr->r_killed[i] = 0;
			Send_unique_monster(j, i);

			/* Tell the player */
			/* the_sandman: added colour */
			msg_format(j, "\374\377v%s rises from the dead!",(r_name + r_ptr->name));
		}

		/* discard reserved character names that exceed their timeout */
		for (i = 0; i < MAX_RESERVED_NAMES; i++) {
			if (!reserved_name_character[i][0]) break;
			if (reserved_name_timeout[i]) {
				reserved_name_timeout[i]--;
				continue;
			}
			s_printf("RESERVED_NAMES_FREE: \"%s\" (%s) at %d.\n", reserved_name_character[i], reserved_name_account[i], i); //debug
			for (j = i + 1; j < MAX_RESERVED_NAMES; j++) {
				if (!reserved_name_character[j][0]) break;
				strcpy(reserved_name_character[j - 1], reserved_name_character[j]);
				strcpy(reserved_name_account[j - 1], reserved_name_account[j]);
				reserved_name_timeout[j - 1] = reserved_name_timeout[j];
			}
			reserved_name_character[j - 1][0] = '\0';
			i--;
			break;
		}
	}

#if 0
	/* Grow trees very occasionally */
	if (!(turn % GROW_TREE)) {
		/* Find a suitable location */
		for (i = 1; i < 1000; i++) {
			cave_type *c_ptr;

			/* Pick a location */
			y = rand_range(1, MAX_HGT - 1);
			x = rand_range(1, MAX_WID - 1);

			/* Acquire pointer */
 #ifdef NEW_DUNGEON
			c_ptr = &zcave[0][y][x];
 #else
			c_ptr = &cave[0][y][x];
 #endif

			/* Only allow "dirt" */
			if (c_ptr->feat != FEAT_DIRT) continue;

			/* Never grow on top of objects or monsters */
			if (c_ptr->m_idx) continue;
			if (c_ptr->o_idx) continue;

			/* Grow a tree here */
			c_ptr->feat = get_seasonal_tree();

			/* Show it */
			everyone_lite_spot(0, y, x);

			/* Done */
			break;
		}
	}
#endif /* if 0 */
}

/* Process things that aren't real-time dependant: Timed shutdowns, compacting objects/monsters.
   We are called once every 50 turns, independant of actual server frame speed. */
static void process_world(void) {
	int i, j, n = 0;
	player_type *p_ptr;

	if (cfg.runlevel < 6 && time(NULL) - cfg.closetime > 120)
		set_runlevel(cfg.runlevel - 1);

	if (cfg.runlevel < 5) {
		for (i = NumPlayers; i > 0 ;i--) {
			if (Players[i]->conn == NOT_CONNECTED) continue;
			if (Players[i]->wpos.wz != 0) break;
		}
		if (!i) set_runlevel(0);
	}

	/* shutdown now */
	if (cfg.runlevel == 2049) {
		shutdown_server();
	/* termination now */
	} else if (cfg.runlevel == 2052) {
		/* same as /shutdown 0, except server will return -2 instead of -1.
		   can be used by scripts to initiate maintenance downtime etc. */
		set_runlevel(-1);

		/* paranoia - set_runlevel() will call exit() */
		time(&cfg.closetime);
		return;
	/* /shutxlow */
	} else if (cfg.runlevel == 2051) {
		for (i = NumPlayers; i > 0 ;i--) {
			p_ptr = Players[i];
			if (p_ptr->conn == NOT_CONNECTED) continue;
			/* Ignore admins that are loged in */
			if (admin_p(i)) continue;
			/* count players */
			n++;

			/* Ignore characters that are afk and not in a dungeon/tower */
			//if ((p_ptr->wpos.wz == 0) && (p_ptr->afk)) continue;

			/* Ignore chars in fixed irondeepdive towns */
			if (is_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#ifdef IRONDEEPDIVE_EXTRA_FIXED_TOWNS
			if (is_extra_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#endif

			/* extra, just for /shutempty: Ignore all iddc chars who are afk/idle */
			if (SHUTDOWN_IGNORE_IDDC(p_ptr)) continue;

			/* Ignore characters that are not in a dungeon/tower */
			if (p_ptr->wpos.wz == 0) {
				/* Don't interrupt events though */
				if (p_ptr->wpos.wx != WPOS_SECTOR000_X || p_ptr->wpos.wy != WPOS_SECTOR000_Y || !sector000separation) continue;
			}
			break;
		}
		if (!i && (n <= 3)) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	/* /shutxxlow */
	} else if (cfg.runlevel == 2053) {
		for (i = NumPlayers; i > 0 ;i--) {
			p_ptr = Players[i];
			if (p_ptr->conn == NOT_CONNECTED) continue;
			/* Ignore admins that are loged in */
			if (admin_p(i)) continue;
			/* count players */
			n++;

			/* Ignore characters that are afk and not in a dungeon/tower */
			//if ((p_ptr->wpos.wz == 0) && (p_ptr->afk)) continue;

			/* Ignore chars in fixed irondeepdive towns */
			if (is_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#ifdef IRONDEEPDIVE_EXTRA_FIXED_TOWNS
			if (is_extra_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#endif

			/* extra, just for /shutempty: Ignore all iddc chars who are afk/idle */
			if (SHUTDOWN_IGNORE_IDDC(p_ptr)) continue;

			/* Ignore characters that are not in a dungeon/tower */
			if (p_ptr->wpos.wz == 0) {
				/* Don't interrupt events though */
				if (p_ptr->wpos.wx != WPOS_SECTOR000_X || p_ptr->wpos.wy != WPOS_SECTOR000_Y || !sector000separation) continue;
			}
			break;
		}
		if (!i && (n <= 2)) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	}
	/* /shutempty */
#ifdef ENABLE_GO_GAME
	else if (cfg.runlevel == 2048 && !go_game_up) {
#else
	else if (cfg.runlevel == 2048) {
#endif
		/* Operate only on characters that are actually logged in */
		for (i = NumPlayers; i > 0 ;i--) {
			p_ptr = Players[i];
			if (p_ptr->conn == NOT_CONNECTED) break; /* Specialty: Not continue but break here! */
			/* Ignore admins that are loged in */
			if (admin_p(i)) continue;

			/* Ignore chars in fixed irondeepdive towns */
			if (is_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
 #ifdef IRONDEEPDIVE_EXTRA_FIXED_TOWNS
			if (is_extra_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
 #endif

			/* extra, just for /shutempty: Ignore all iddc chars who are afk/idle */
			if (SHUTDOWN_IGNORE_IDDC(p_ptr)) continue;

			/* Ignore characters that are afk and not in a dungeon/tower */
			//if ((p_ptr->wpos.wz == 0) && (p_ptr->afk)) continue;

			/* Ignore characters that are not in a dungeon/tower */
			if (p_ptr->wpos.wz == 0) {
				/* Don't interrupt events though */
				if (p_ptr->wpos.wx != WPOS_SECTOR000_X || p_ptr->wpos.wy != WPOS_SECTOR000_Y || !sector000separation) continue;
			}
			break;
		}
		/* Operate on accounts that are logged in in the character screen or in the process of creating a character */
		for (j = 0; j < max_connections; j++) {
			connection_t *connp = Conn[j];

			/* no connection at all? */
			if (!connp) continue;

			/* connection already has a character fully logged in? ignore! */
			if (connp->state == 0x08) continue; // 0x08 = CONN_PLAYING

			if (connp->id == -1) continue;

			/* We now check all characters that aren't in-game yet, but in state 0x04 aka CONN_LOGIN:
			   That is either still in character list screen or in character creation process. */

			/* Ignore characters that are not yet in the character creation phase but just in the character list screen? */
			if (!connp->c_name) continue; /* (has not chosen a character name to login yet) */

			/* Always ignore admins no matter what stage they are in */
			if (GetInd[connp->id] && admin_p(GetInd[connp->id])) continue;
			/* Always ignore admin accounts that are about to login, too */
			if (connp->nick) {
				struct account acc;

				if (GetAccount(&acc, connp->nick, NULL, TRUE, NULL, NULL) && (acc.flags & ACC_ADMIN)) continue;
			}

			/* Don't trigger restart yet */
			break;
		}
		if (!i && j == max_connections) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	/* /shutlow */
	} else if (cfg.runlevel == 2047) {
		for (i = NumPlayers; i > 0 ;i--) {
			p_ptr = Players[i];
			if (p_ptr->conn == NOT_CONNECTED) continue;
			/* Ignore admins that are loged in */
			if (admin_p(i)) continue;
			/* count players */
			n++;

			/* Ignore characters that are afk and not in a dungeon/tower */
			//if ((p_ptr->wpos.wz == 0) && (p_ptr->afk)) continue;

			/* Ignore chars in fixed irondeepdive towns */
			if (is_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#ifdef IRONDEEPDIVE_EXTRA_FIXED_TOWNS
			if (is_extra_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#endif

			/* extra, just for /shutempty: Ignore all iddc chars who are afk/idle */
			if (SHUTDOWN_IGNORE_IDDC(p_ptr)) continue;

			/* Ignore characters that are not in a dungeon/tower */
			if (p_ptr->wpos.wz == 0) {
				/* Don't interrupt events though */
				if (p_ptr->wpos.wx != WPOS_SECTOR000_X || p_ptr->wpos.wy != WPOS_SECTOR000_Y || !sector000separation) continue;
			}
			break;
		}
		if (!i && (n <= 5)) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	/* /shutvlow */
	} else if (cfg.runlevel == 2046) {
		for (i = NumPlayers; i > 0 ;i--) {
			p_ptr = Players[i];
			if (p_ptr->conn == NOT_CONNECTED) continue;
			/* Ignore admins that are loged in */
			if (admin_p(i)) continue;
			/* count players */
			n++;

			/* Ignore characters that are afk and not in a dungeon/tower */
			//if ((p_ptr->wpos.wz == 0) && (p_ptr->afk)) continue;

			/* Ignore chars in fixed irondeepdive towns */
			if (is_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#ifdef IRONDEEPDIVE_EXTRA_FIXED_TOWNS
			if (is_extra_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#endif

			/* extra, just for /shutempty: Ignore all iddc chars who are afk/idle */
			if (SHUTDOWN_IGNORE_IDDC(p_ptr)) continue;

			/* Ignore characters that are not in a dungeon/tower */
			if (p_ptr->wpos.wz == 0) {
				/* Don't interrupt events though */
				if (p_ptr->wpos.wx != WPOS_SECTOR000_X || p_ptr->wpos.wy != WPOS_SECTOR000_Y || !sector000separation) continue;
			}
			break;
		}
		if (!i && (n <= 4)) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	/* /shutnone */
	} else if (cfg.runlevel == 2045) {
		for (i = NumPlayers; i > 0 ;i--) {
			if (Players[i]->conn == NOT_CONNECTED) continue;
			/* Ignore admins that are loged in */
			if (admin_p(i)) continue;
			/* count players */
			n++;
		}
		/* Operate on accounts that are logged in in the character screen or in the process of creating a character */
		for (j = 0; j < max_connections; j++) {
			connection_t *connp = Conn[j];

			/* no connection at all? */
			if (!connp) continue;

			/* connection already has a character fully logged in? ignore! */
			if (connp->state == 0x08) continue; // 0x08 = CONN_PLAYING

			if (connp->id == -1) continue;

#if 0 /* 0: don't restart even for those who just have logged in their account name and aren't creating a character yet */
			if (!connp->c_name) continue; //has not chosen a character name to login yet? ignore then
#endif

			//ignore admins
			if (GetInd[connp->id] && admin_p(GetInd[connp->id])) continue;
			//ignore admin accounts that are about to login, too
			if (connp->nick) {
				struct account acc;

				if (GetAccount(&acc, connp->nick, NULL, TRUE, NULL, NULL) && (acc.flags & ACC_ADMIN)) continue;
			}

			/* Don't trigger restart yet */
			break;
		}
		if (!n && j == max_connections) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	/* /shutactivevlow */
	} else if (cfg.runlevel == 2044) {
		for (i = NumPlayers; i > 0 ;i--) {
			p_ptr = Players[i];
			if (p_ptr->conn == NOT_CONNECTED) continue;

			/* Ignore admins that are loged in */
			if (admin_p(i)) continue;

			/* Ignore perma-afk players! */
			//if (p_ptr->afk &&
			if (is_inactive(i) >= 30 * 20) /* 20 minutes idle? */
				continue;

			/* count players */
			n++;

			/* Ignore characters that are afk and not in a dungeon/tower */
			//if ((p_ptr->wpos.wz == 0) && (p_ptr->afk)) continue;

			/* Ignore chars in fixed irondeepdive towns */
			if (is_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#ifdef IRONDEEPDIVE_EXTRA_FIXED_TOWNS
			if (is_extra_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#endif

			/* Ignore characters that are not in a dungeon/tower */
			if (p_ptr->wpos.wz == 0) {
				/* Don't interrupt events though */
				if (p_ptr->wpos.wx != WPOS_SECTOR000_X || p_ptr->wpos.wy != WPOS_SECTOR000_Y || !sector000separation) continue;
			}
			break;
		}
		if (!i && (n <= 4)) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	}

	/* /shutrec [terminate] */
	if (cfg.runlevel == 2043 || cfg.runlevel == 2042) {
		if (shutdown_recall_timer <= 60 && shutdown_recall_state < 3) {
			if (cfg.runlevel == 2043)
				msg_broadcast(0, "\374\377I*** \377RServer restart in max 1 minute (auto-recall). \377I***");
			else
				msg_broadcast(0, "\374\377I*** \377RServer shutdown in max 1 minute (auto-recall). \377I***");
			shutdown_recall_state = 3;
		}
		else if (shutdown_recall_timer <= 300 && shutdown_recall_state < 2) {
			if (cfg.runlevel == 2043)
				msg_broadcast(0, "\374\377I*** \377RServer restart in max 5 minutes (auto-recall). \377I***");
			else
				msg_broadcast(0, "\374\377I*** \377RServer shutdown in max 5 minutes (auto-recall). \377I***");
			shutdown_recall_state = 2;
		}
		else if (shutdown_recall_timer <= 900 && shutdown_recall_state < 1) {
			if (cfg.runlevel == 2043)
				msg_broadcast(0, "\374\377I*** \377RServer restart in max 15 minutes (auto-recall). \377I***");
			else
				msg_broadcast(0, "\374\377I*** \377RServer shutdown in max 15 minutes (auto-recall). \377I***");
			shutdown_recall_state = 1;
		}
		if (!shutdown_recall_timer) {
			for (i = NumPlayers; i > 0 ;i--) {
				p_ptr = Players[i];
				if (p_ptr->conn == NOT_CONNECTED) continue;

				/* Don't remove loot from ghosts waiting for res */
				if (admin_p(i) || p_ptr->ghost) continue;

				/* Don't free people from the Ironman Deep Dive Challenge */
				if (in_irondeepdive(&p_ptr->wpos)) continue;

				if (p_ptr->wpos.wz) {
					p_ptr->recall_pos.wx = p_ptr->wpos.wx;
					p_ptr->recall_pos.wy = p_ptr->wpos.wy;
					p_ptr->recall_pos.wz = 0;
					p_ptr->new_level_method = (p_ptr->wpos.wz > 0 ? LEVEL_RECALL_DOWN : LEVEL_RECALL_UP);
					recall_player(i, "");
				}
			}
			if (cfg.runlevel == 2043) {
				msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
				cfg.runlevel = 2049;
			} else { //if (cfg.runlevel == 2042) {
				msg_broadcast(-1, "\374\377G<<<\377oServer is now going down for maintenance.\377G>>>");
				cfg.runlevel = 2052;
			}
		} else {
			for (i = NumPlayers; i > 0 ;i--) {
				p_ptr = Players[i];
				if (p_ptr->conn == NOT_CONNECTED) continue;

				/* Ignore admins that are loged in */
				if (admin_p(i)) continue;

				/* Ignore chars in fixed irondeepdive towns */
				if (is_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#ifdef IRONDEEPDIVE_EXTRA_FIXED_TOWNS
				if (is_extra_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#endif

				/* Ignore characters that are afk and not in a dungeon/tower */
				//if ((p_ptr->wpos.wz == 0) && (p_ptr->afk)) continue;

				/* Ignore characters that are not in a dungeon/tower */
				if (p_ptr->wpos.wz == 0) {
					/* Don't interrupt events though */
					if (p_ptr->wpos.wx != WPOS_SECTOR000_X || p_ptr->wpos.wy != WPOS_SECTOR000_Y || !sector000separation) continue;
				}
				break;
			}
			if (!i) {
				if (cfg.runlevel == 2043) {
					msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
					cfg.runlevel = 2049;
				} else { //if (cfg.runlevel == 2042)
					msg_broadcast(-1, "\374\377G<<<\377oServer is now going down for maintenance.\377G>>>");
					cfg.runlevel = 2052;
				}
			}
		}
	}
	/* /shutulow */
	else if (cfg.runlevel == 2041) {
		for (i = NumPlayers; i > 0 ;i--) {
			p_ptr = Players[i];
			if (p_ptr->conn == NOT_CONNECTED) continue;
			/* Ignore admins that are loged in */
			if (admin_p(i)) continue;
			/* count players */
			n++;

			/* Ignore characters that are afk and not in a dungeon/tower */
			//if ((p_ptr->wpos.wz == 0) && (p_ptr->afk)) continue;

			/* Ignore chars in fixed irondeepdive towns */
			if (is_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#ifdef IRONDEEPDIVE_EXTRA_FIXED_TOWNS
			if (is_extra_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos))) continue;
#endif

			/* extra, just for /shutempty: Ignore all iddc chars who are afk/idle */
			if (SHUTDOWN_IGNORE_IDDC(p_ptr)) continue;

			/* Ignore characters that are not in a dungeon/tower */
			if (p_ptr->wpos.wz == 0) {
				/* Don't interrupt events though */
				if (p_ptr->wpos.wx != WPOS_SECTOR000_X || p_ptr->wpos.wy != WPOS_SECTOR000_Y || !sector000separation) continue;
			}
			break;
		}
		if (!i && (n <= 1)) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	}

	/* Hack -- Compact the object list occasionally */
	if (o_top + 160 > MAX_O_IDX) compact_objects(320, TRUE);

	/* Hack -- Compact the monster list occasionally */
	if (m_top + 320 > MAX_M_IDX) compact_monsters(640, TRUE);

	/* Hack -- Compact the trap list occasionally */
	//if (t_top + 160 > MAX_TR_IDX) compact_traps(320, TRUE);

}

#ifdef DUNGEON_VISIT_BONUS
/* Keep track of frequented dungeons, called every minute. */
static void process_dungeon_boni(void) {
	int i;
	dungeon_type *d_ptr;
	wilderness_type *w_ptr;
	player_type *p_ptr;
 #ifdef DUNGEON_VISIT_BONUS_DEPTHRANGE
	int depthrange;
 #endif

	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		if (p_ptr->conn == NOT_CONNECTED) continue;

		if (p_ptr->wpos.wz == 0) continue;
		w_ptr = &wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx];
		if (p_ptr->wpos.wz < 0) d_ptr = w_ptr->dungeon;
		else if (p_ptr->wpos.wz > 0) d_ptr = w_ptr->tower;
		else continue; //paranoia

 #if 0
		/* NOTE: We're currently counting up for -each- player in there.
		   Maybe should be changed to generic 'visited or not visited'. */
		if (dungeon_visit_frequency[d_ptr->id] < VISIT_TIME_CAP)
			dungeon_visit_frequency[d_ptr->id]++;
 #else
		/* NOTE about NOTE: Not anymore. Now # of players doesn't matter: */
		if (dungeon_visit_frequency[d_ptr->id] < VISIT_TIME_CAP)
			dungeon_visit_check[d_ptr->id] = TRUE;
 #endif

 #ifdef DUNGEON_VISIT_BONUS_DEPTHRANGE
		/* Also keep track of which depths are mostly frequented */
		depthrange = getlevel(&p_ptr->wpos) / 10;
		/* Excempt Valinor (dlv 200) */
		if (depthrange < 20
		    && depthrange_visited[depthrange] < VISIT_TIME_CAP);
			depthrange_visited[depthrange]++;
 #endif
	}

 #if 1
	for (i = 1; i <= dungeon_id_max; i++)
		if (dungeon_visit_check[i]) {
			dungeon_visit_check[i] = FALSE;
			dungeon_visit_frequency[i]++;
		}
 #endif
}
/* Decay visit frequency for all dungeons over time. */
/* Also update the 'rest bonus' each dungeon gives:
   Use <startfloor + 2/3 * (startfloor-endfloor)> as main comparison value.
   Called every 10 minutes. */
static void process_dungeon_boni_decay(void) {
	int i;

 #ifdef DUNGEON_VISIT_BONUS_DEPTHRANGE
	for (i = 0; i < 20; i++) {
		if (depthrange_visited[i])
			depthrange_visited[i]--;

		/* update the depth-range scale in regards to its most contributing dungeon */
		//todo; see below
	}
 #endif
	for (i = 1; i <= dungeon_id_max; i++) {
		if (dungeon_visit_frequency[i])
			dungeon_visit_frequency[i]--;

		/* update bonus of this dungeon */
 #ifdef DUNGEON_VISIT_BONUS_DEPTHRANGE
		/* more sophisticated algoritm.
		   Todo: find a good way how to do this :-p.
		   Idea (C. Blue):
		   players increase the depthrange_visited counter for the dlvl they are on,
		   dungeons then compare it with their average depth (which is as stated above
		   start + 2/3 * (end-start) floor), and only start counting as "not visited"
		   if the dungeon depths that ARE visited actually are comparable to the
		   avg depth of the dungeon in question.
		   Main problem: Handle multiple dungeons adding to the same depthrange in
		   pretty different scales, eg 'player A is all the time in dungeon A which
		   only is lvl 1-9, while player B spends even much more time in the very first
		   level of dungeon B which goes from lvl 1-49. Now how are the numbers scaled
		   down to determine the actual 'visitedness' of dungeon A for example? */
 #endif

		/* straightforward simple way without DUNGEON_VISIT_BONUS_DEPTHRANGE */
		set_dungeon_bonus(i, FALSE);
	}
}
#endif

#ifdef FLUENT_ARTIFACT_RESETS
/* Process timing out of artifacts.
   Handled in 1-minute resolution - assuming we have less artifacts than 60*cfg.fps. */
static void process_artifacts(void) {
	int i = turn % (cfg.fps * 60);
	player_type *p_ptr;

	if (!cfg.persistent_artifacts && i < max_a_idx && a_info[i].timeout > 0) {
 #if defined(IDDC_ARTIFACT_FAST_TIMEOUT) || defined(WINNER_ARTIFACT_FAST_TIMEOUT)
		bool double_speed = FALSE;
 #endif

 #ifdef IDDC_ARTIFACT_FAST_TIMEOUT
		if (a_info[i].iddc) {
			a_info[i].timeout -= 2;
			double_speed = TRUE;

			/* hack: don't accidentally skip erasure/warning */
			if (a_info[i].timeout == -1) a_info[i].timeout = 0;
			//Not required because of double_speed calc:
			//else if (a_info[i].timeout == FLUENT_ARTIFACT_WARNING - 1) a_info[i].timeout = FLUENT_ARTIFACT_WARNING;
		} else
 #endif
 #ifdef WINNER_ARTIFACT_FAST_TIMEOUT
		if (a_info[i].winner) {
			a_info[i].timeout -= 2;
			double_speed = TRUE;

			/* hack: don't accidentally skip erasure/warning */
			if (a_info[i].timeout == -1) a_info[i].timeout = 0;
			//Not required because of double_speed calc:
			//else if (a_info[i].timeout == FLUENT_ARTIFACT_WARNING - 1) a_info[i].timeout = FLUENT_ARTIFACT_WARNING;
		} else
 #endif
		a_info[i].timeout--;

		/* erase? */
		if (a_info[i].timeout == 0) erase_or_locate_artifact(i, TRUE);
		/* just warn? */
 #if defined(IDDC_ARTIFACT_FAST_TIMEOUT) || defined(WINNER_ARTIFACT_FAST_TIMEOUT)
		else if (a_info[i].timeout == (double_speed ? FLUENT_ARTIFACT_WARNING / 2 : FLUENT_ARTIFACT_WARNING)) {
 #else
		else if (a_info[i].timeout == FLUENT_ARTIFACT_WARNING) {
 #endif
			int j, k;
			char o_name[ONAME_LEN];

			for (j = 1; j <= NumPlayers; j++) {
				p_ptr = Players[j];
				if (p_ptr->id != a_info[i].carrier) continue;
				if (p_ptr->conn == NOT_CONNECTED) continue;
				for (k = 0; k < INVEN_TOTAL; k++) {
					if (!p_ptr->inventory[k].k_idx) continue;
					if (p_ptr->inventory[k].name1 != i) continue;
					if (!(p_ptr->inventory[k].ident & ID_MENTAL)) continue;
					object_desc(j, o_name, &p_ptr->inventory[k], TRUE, 128 + 256);
					msg_format(j, "\374\377RYour %s will vanish soon!", o_name);
					j = NumPlayers;
					break;
				}
			}
		}
	}
}
#endif

#ifdef ENABLE_MERCHANT_MAIL
static void process_merchant_mail(void) {
	int i = turn % MAX_MERCHANT_MAILS; //fast enough >_>

	if (mail_sender[i][0]) {
		if (mail_duration[i]) {
 #ifndef MERCHANT_MAIL_INFINITE
			bool erase = FALSE;
 #endif

			if (mail_duration[i] < 0) {
				/* we bounced back to sender, but now stop bouncing and erase it if he doesn't want it either, if MERCHANT_MAIL_INFINITE */
				mail_duration[i]++;
 #ifndef MERCHANT_MAIL_INFINITE
				erase = TRUE;
 #endif
			} else
				/* normal case (1st send, or infinite bouncing enabled) */
				mail_duration[i]--;

			if (!mail_duration[i]) {
				int j;

				/* set timeout for expiry */
				if (lookup_player_id(mail_target[i])) {
					/* normal */
 #ifdef MERCHANT_MAIL_INFINITE
					mail_timeout[i] = MERCHANT_MAIL_TIMEOUT;
 #else
					if (erase) mail_timeout[i] = - 2 - MERCHANT_MAIL_TIMEOUT;
					else mail_timeout[i] = MERCHANT_MAIL_TIMEOUT;
 #endif

					/* notify him */
					for (j = 1; j <= NumPlayers; j++) {
						if (strcmp(Players[j]->accountname, mail_target_acc[i])) continue;
						if (strcmp(Players[j]->name, mail_target[i])) {
							msg_format(j, "\374\377yThe merchants guild has mail for your character '%s'!", mail_target[i]);
 #ifndef MERCHANT_MAIL_INFINITE
							if (erase) msg_print(j, "\374\377yWarning - if you don't pick it up in time, the guild bureau will confiscate it!");
 #endif
 #ifdef USE_SOUND_2010
							sound(j, "store_doorbell_leave", NULL, SFX_TYPE_MISC, FALSE);
 #endif
						} else {
							if (Players[j]->store_num == STORE_MERCHANTS_GUILD) {
 #ifdef USE_SOUND_2010
								sound(j, "store_doorbell_leave", NULL, SFX_TYPE_MISC, FALSE);
 #endif
								merchant_mail_delivery(j);
							} else {
								msg_print(j, "\374\377yThe merchants guild has mail for you!");
								/* This doesn't work well right now because the receipient might have re-created a character of the same name,
								   which would cause this message to get omitted, confusingly for the sender: */
								//if (!lookup_player_id(mail_sender[i])) msg_print(j, "\374\377yAccording to the offices' information the receipient unfortunately is deceased.");
 #ifndef MERCHANT_MAIL_INFINITE
								if (erase) msg_print(j, "\374\377yWarning - if you don't pick it up in time, the guild bureau will confiscate it!");
 #endif
 #ifdef USE_SOUND_2010
								sound(j, "store_doorbell_leave", NULL, SFX_TYPE_MISC, FALSE);
 #endif
							}
						}
					}
				} else
					/* hack for players who no longer exist */
					mail_timeout[i] = -1; //This hack is no longer in use, as this is now taken care of directly in erase_player() and erase_player_name()
			}
		} else if (mail_timeout[i] != 0) {
			bool erase, quiet;

			if (mail_timeout[i] == -1) {
				/* character timed out and was deleted */
				mail_timeout[i] = 0;
				erase = TRUE;
				quiet = FALSE;
			} else if (mail_timeout[i] == -2) {
				/* player refused to pay for COD mail */
				mail_timeout[i] = 0;
				erase = FALSE;
				quiet = TRUE;
			} else if (mail_timeout[i] < -2) {
				/* we just bounced back, might result in erasure if not picked up and not MERCHANT_MAIL_INFINITE */
				mail_timeout[i]++;
				if (mail_timeout[i] == -2)
 #ifdef MERCHANT_MAIL_INFINITE
					mail_timeout[i] = 0;
 #else
				{
  #if 1
					/* notify sloth */
					int j;

					for (j = 1; j <= NumPlayers; j++) {
						if (!strcmp(Players[j]->accountname, mail_target_acc[i])) {
							if (strcmp(Players[j]->name, mail_target[i]))
								msg_print(j, "\374\377yA mail package for another character of yours just expired!");
							else
								msg_print(j, "\374\377yA mail package for you just expired!");
						}
					}
  #endif

					s_printf("MERCHANT_MAIL_ERROR_ERASED (1st bounce).\n");
					/* delete mail! */
					mail_sender[i][0] = 0;
					/* leave structure */
					mail_timeout[i] = 1;
				}
 #endif
				erase = FALSE;
				quiet = FALSE;
			} else {
				/* normal expiry countdown */
				mail_timeout[i]--;
				erase = FALSE;
				quiet = FALSE;
			}

			if (!mail_timeout[i]) {
				/* send it back */
				char tmp[NAME_LEN];
				u32b pid;
				cptr acc;
				int j;

				/* notify receipient that he didn't make it in time */
				if (!erase && !quiet)
					for (j = 1; j <= NumPlayers; j++) {
						if (!strcmp(Players[j]->accountname, mail_target_acc[i])) {
							if (strcmp(Players[j]->name, mail_target[i]))
								msg_print(j, "\374\377yA mail package for another character of yours just expired and was returned!");
							else
								msg_print(j, "\374\377yA mail package for you just expired and was returned to the sender!");
						}
					}

 #ifndef MERCHANT_MAIL_INFINITE
				erase = TRUE;
				mail_duration[i] = -MERCHANT_MAIL_DURATION;
 #else
				mail_duration[i] = MERCHANT_MAIL_DURATION;
 #endif
				strcpy(tmp, mail_sender[i]);
				strcpy(mail_sender[i], mail_target[i]);
				strcpy(mail_target[i], tmp);

				/* If it was COD mail, don't ask for a fee! (It might have been a typo and outrageous, in which case the item would now be lost) */
				mail_COD[i] = 0;

				pid = lookup_player_id(mail_target[i]);
				if (!pid) {
					s_printf("MERCHANT_MAIL_ERROR: no pid - Sender %s, Target %s\n", mail_sender[i], mail_target[i]);
					/* special: the mail bounced back to a character that no longer exists!
					   if this happens with the new receipient too, the mail gets deleted,
					   since noone can pick it up anymore. */
					if (erase) {
						s_printf("MERCHANT_MAIL_ERROR_ERASED.\n");
						/* delete mail! */
						mail_sender[i][0] = 0;
					}
				} else {
					acc = lookup_accountname(pid);
					if (acc) strcpy(mail_target_acc[i], acc);
					else s_printf("MERCHANT_MAIL_ERROR_RETURN: no acc - Sender %s, Target %s\n", mail_sender[i], mail_target[i]);
				}
			}
		}
	}
}
#endif


int find_player(s32b id) {
	int i;

	for (i = 1; i <= NumPlayers; i++) {
		player_type *p_ptr = Players[i];

		if (Players[i]->conn == NOT_CONNECTED) return(0);
		if (p_ptr->id == id) return(i);
	}

	/* assume none */
	return(0);
}

int find_player_name(cptr name) {
	int i;

	for (i = 1; i <= NumPlayers; i++)
		if (!strcasecmp(Players[i]->name, name)) return(i);

	/* assume none */
	return(0);
}

void process_player_change_wpos(int Ind) {
	player_type *p_ptr = Players[Ind];
	worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	//worldpos twpos;
	dun_level *l_ptr;
	int d, j, x, y, startx = 0, starty = 0, m_idx, my, mx, tries, emergency_x, emergency_y, dlv = getlevel(wpos);
	char o_name_short[ONAME_LEN];
	bool smooth_ambient = FALSE; //also used for flickering!
#ifdef USE_SOUND_2010
	bool travel_ambient = FALSE;
#endif
	bool df = FALSE, took_oneway_stairs, dont_end_up_in_ntvault_nestpit;
	monster_type *m_ptr;
	cave_type **mcave;
	struct dungeon_type *d_ptr = getdungeon(wpos);

	/* Prevent exploiting /undoskills by invoking it right before each level-up:
	   Discard the possibility to undoskills when we venture into a dungeon again. */
	if (!p_ptr->wpos_old.wz && p_ptr->wpos.wz) p_ptr->reskill_possible &= ~RESKILL_F_UNDO;

	if (p_ptr->steamblast) {
		msg_print(Ind, "You cancel your preparations for a steam blast charge.");
		p_ptr->steamblast = 0;
	}

	/* un-snow */
	p_ptr->temp_misc_1 &= ~0x08;
	//update_player(Ind); //un-snowing restores lack of visibility by others - required?

#ifdef USE_SOUND_2010
	if ((l_ptr = getfloor(&p_ptr->wpos_old)) && (l_ptr->flags1 & LF1_NO_GHOST))
		sound(Ind, "monster_roar", NULL, SFX_TYPE_STOP, FALSE);
#endif

	/* IDDC specialties */
	if (in_irondeepdive(&p_ptr->wpos_old)) {
		/* for obtaining statistical IDDC information: */
		if (!is_admin(p_ptr)) {
			s_printf("CVRG-IDDC: '%s' leaves floor %d:\n", p_ptr->name, p_ptr->wpos_old.wz);
			log_floor_coverage(getfloor(&p_ptr->wpos_old), &p_ptr->wpos_old);

#if 1 /* experimental (IDDC_OCCUPIED_FLOOR) */
			/* And a specialty, colour/uncolour staircases for others: */
			for (d = 1; d <= NumPlayers; d++) {
				if (d == Ind) continue;
				if (in_irondeepdive(&Players[d]->wpos)) {
					//Players[d]->update |= PU_VIEW;
					Players[d]->redraw |= PR_MAP;
					handle_stuff(d);
				}
			}
#endif
		}

#ifdef IDDC_EASY_SPEED_RINGS
 #if IDDC_EASY_SPEED_RINGS > 0
		/* IDDC_EASY_SPEED_RINGS - allow easy speed-ring finding for the first 1-2 rings,
		   from floor 60+ and optionally another one from floor 80+? */
		if (wpos->wz == IDDC_TOWN2_WILD
  #if IDDC_EASY_SPEED_RINGS > 1
		    || wpos->wz == IDDC_TOWN2_FIXED
  #endif
		    ) {
			//hack: abuse 2 flag bits as a counter, increment
			j = (p_ptr->IDDC_flags & 0x3) + 1;
			p_ptr->IDDC_flags = (p_ptr->IDDC_flags & ~0x3) | j;
		}
 #endif
#endif

		/* Allow getting an extermination order again */
		//p_ptr->IDDC_flags &= ~0xC;
		//decrement the 2-bits-number..
		if (p_ptr->IDDC_flags & 0xC) {
			j = (((p_ptr->IDDC_flags & 0xC) >> 2) - 1) << 2;
			p_ptr->IDDC_flags = (p_ptr->IDDC_flags & ~0xC) | j;
		}
	}

	/* Decide whether we stayed long enough on the previous
	   floor to get distinct floor feelings here, and also
	   start counting turns we spend on this floor. */
	//there's no scumming in RPG_SERVER!
#ifdef RPG_SERVER
	p_ptr->distinct_floor_feeling = TRUE;
#else
	if (p_ptr->turns_on_floor >= TURNS_FOR_EXTRA_FEELING)
		p_ptr->distinct_floor_feeling = TRUE;
	else
		p_ptr->distinct_floor_feeling = FALSE;
#endif
	if (p_ptr->new_level_method != LEVEL_OUTSIDE &&
	    p_ptr->new_level_method != LEVEL_OUTSIDE_RAND &&
	    p_ptr->new_level_method != LEVEL_HOUSE)
		p_ptr->turns_on_floor = 0;

#ifdef ENABLE_MAIA
	/* reset void gate coordinates */
	if (p_ptr->voidx) {
		if (getcave(&p_ptr->wpos_old))
			cave_set_feat(&p_ptr->wpos_old, p_ptr->voidy, p_ptr->voidx, twall_erosion(&p_ptr->wpos_old, p_ptr->voidy, p_ptr->voidx, FEAT_FLOOR));
		p_ptr->voidx = 0;
		p_ptr->voidy = 0;
	}
#endif

	/* being on different floors destabilizes mind fusion */
	if (p_ptr->esp_link_type && p_ptr->esp_link)
		change_mind(Ind, FALSE);

	/* Check "maximum depth" to make sure it's still correct */
	if (wpos->wz != 0 && (!p_ptr->ghost || p_ptr->admin_dm)) {
		if (dlv > p_ptr->max_dlv) p_ptr->max_dlv = dlv;

#ifdef SEPARATE_RECALL_DEPTHS
		j = recall_depth_idx(wpos, p_ptr);
		if (ABS(wpos->wz) > p_ptr->max_depth[j]) p_ptr->max_depth[j] = ABS(wpos->wz);
#endif
	}

	/* Make sure the server doesn't think the player is in a store */
	if (p_ptr->store_num != -1) {
		handle_store_leave(Ind);
		Send_store_kick(Ind);
	}

	/* Hack -- artifacts leave the queen/king */
	/* also checks the artifact list */
	if ((cfg.fallenkings_etiquette || cfg.kings_etiquette) && !is_admin(p_ptr) && cfg.strict_etiquette) {
		object_type *o_ptr;
		char o_name[ONAME_LEN];
		bool no_etiquette =
		    (!(cfg.fallenkings_etiquette && p_ptr->once_winner && !p_ptr->total_winner) &&
		    !(cfg.kings_etiquette && p_ptr->total_winner));

		for (j = 0; j < INVEN_TOTAL; j++) {
			o_ptr = &p_ptr->inventory[j];
			if (!o_ptr->k_idx || !true_artifact_p(o_ptr)) continue;

			/* fix the list */
			handle_art_inumpara(o_ptr->name1);
			if (!a_info[o_ptr->name1].known && (o_ptr->ident & ID_KNOWN))
				a_info[o_ptr->name1].known = TRUE;

			if (no_etiquette ||
			    winner_artifact_p(o_ptr) || admin_artifact_p(o_ptr)) continue;

			/* Describe the object */
			object_desc(Ind, o_name, o_ptr, TRUE, 0);
			object_desc(0, o_name_short, o_ptr, TRUE, 256);

			/* Message */
			msg_format(Ind, "\377y%s bids farewell to you...", o_name);
			handle_art_d(o_ptr->name1);

			/* Eliminate the item  */
			inven_item_increase(Ind, j, -99);
			inven_item_describe(Ind, j);
			inven_item_optimize(Ind, j);

			/* Tell everyone */
			msg_broadcast_format(Ind, "\374\377M* \377U%s has been lost once more. \377M*", o_name_short);

			j--;
		}
	}

	/* Reset bot hunting variables */
	p_ptr->silly_door_exp = 0;

	/* For Ironman Deep Dive Challenge */
	p_ptr->IDDC_logscum = 0; /* we changed floor, all fine and dandy.. */

	/* Somebody has entered an ungenerated level */
	if (players_on_depth(wpos) && !getcave(wpos)) {
		/* Allocate space for it */
		alloc_dungeon_level(wpos);

		/* Generate a dungeon level there */
		generate_cave(wpos, p_ptr);

#ifdef USE_SOUND_2010
		/* allow non-normal (interval-timed) ambient sfx, but depend on our own fast-travel-induced rythm */
		travel_ambient = TRUE;
	} else if (players_on_depth(wpos) == 1) travel_ambient = TRUE; /* exception - if we're the only one here we won't mess up someone else's ambient sfx rythm, so it's ok */
#else
	}
#endif

#ifdef USE_SOUND_2010
	if (travel_ambient) {
		/* hack for single ambient sfx while speed-travelling the wild.
		   (Random info: Travelling a sector horizontally at +4 spd takes ~20s if no obstacles.) */
		if (!wpos->wz && !p_ptr->wpos_old.wz) {
			wilderness_type *w_ptr = &wild_info[wpos->wy][wpos->wx];
 #if 0 /* require to travel lots of the same type of terrain to play hacked-rythem ambient sfx? */
			if (w_ptr->type != w_ptr->type)
				p_ptr->ambient_sfx_timer = 0;
			else
 #endif
			if (p_ptr->ambient_sfx_timer >= w_ptr->ambient_sfx_timer && !w_ptr->ambient_sfx_dummy) {
				p_ptr->ambient_sfx_timer = 0;
				w_ptr->ambient_sfx_timer = 0;
			}
		} else p_ptr->ambient_sfx_timer = 0;
	}
#endif

	zcave = getcave(wpos);
	l_ptr = getfloor(wpos);

	/* Detect wrong dungeon generation:
	   All admin-created dungeons via master_level() get DF2_RANDOM flag,
	   so do all predefined (ie non type-0) dungeons.
	   However, if the code somewhere calls add_dungeon() of type 0 but forgets to add DF2_RANDOM,
	   the result might be a dungeon that has no defined width and height.
	   We catch that here to make an admin's debugging life easier. - C. Blue */
	if (l_ptr && (!l_ptr->wid || !l_ptr->hgt)) s_printf("L_PTR WARNING: Level has dimension zero. Check for missing DF2_RANDOM flag!\n");

	/* Clear the "marked" and "lit" flags for each cave grid */
	for (y = 0; y < MAX_HGT; y++)
		for (x = 0; x < MAX_WID; x++)
			p_ptr->cave_flag[y][x] = 0;

	/* Player now starts mapping this dungeon (as far as its flags allow) */
	if (l_ptr) p_ptr->dlev_id = l_ptr->id;
	else p_ptr->dlev_id = 0;

	/* Memorize the town and all wilderness levels close to town */
	if (istownarea(wpos, MAX_TOWNAREA)) {
		p_ptr->max_panel_rows = MAX_PANEL_ROWS;
		p_ptr->max_panel_cols = MAX_PANEL_COLS;

		p_ptr->max_tradpanel_rows = MAX_TRADPANEL_ROWS;
		p_ptr->max_tradpanel_cols = MAX_TRADPANEL_COLS;

		p_ptr->cur_hgt = MAX_HGT;
		p_ptr->cur_wid = MAX_WID;

		if (istown(wpos)) {
			p_ptr->town_x = wpos->wx;
			p_ptr->town_y = wpos->wy;

			/* for PVP-mode, reset diminishing healing */
			p_ptr->heal_effect = 0;
			/* and anti-fleeing teleport prevention */
			p_ptr->pvp_prevent_tele = 0;
			p_ptr->redraw |= PR_DEPTH;
		}
	} else if (wpos->wz) {
		/* Hack -- tricky formula, but needed */
		p_ptr->max_panel_rows = MAX_PANEL_ROWS_L;
		p_ptr->max_panel_cols = MAX_PANEL_COLS_L;

		p_ptr->max_tradpanel_rows = MAX_TRADPANEL_ROWS_L;
		p_ptr->max_tradpanel_cols = MAX_TRADPANEL_COLS_L;

		p_ptr->cur_hgt = l_ptr->hgt;
		p_ptr->cur_wid = l_ptr->wid;

		/* only show 'dungeon explore feelings' when entering a dungeon, for now */
#ifdef DUNGEON_VISIT_BONUS
		show_floor_feeling(Ind, (p_ptr->wpos_old.wz == 0));
#else
		show_floor_feeling(Ind, FALSE);
#endif
	} else {
		p_ptr->max_panel_rows = MAX_PANEL_ROWS;
		p_ptr->max_panel_cols = MAX_PANEL_COLS;

		p_ptr->max_tradpanel_rows = MAX_TRADPANEL_ROWS;
		p_ptr->max_tradpanel_cols = MAX_TRADPANEL_COLS;

		p_ptr->cur_hgt = MAX_HGT;
		p_ptr->cur_wid = MAX_WID;
	}

#ifdef BIG_MAP
	if (p_ptr->max_panel_rows < 0) p_ptr->max_panel_rows = 0;
	if (p_ptr->max_panel_cols < 0) p_ptr->max_panel_cols = 0;
#endif

#ifdef ALLOW_NR_CROSS_PARTIES
	if (p_ptr->party && at_netherrealm(&p_ptr->wpos_old) && !at_netherrealm(&p_ptr->wpos)
	    && compat_mode(p_ptr->mode, parties[p_ptr->party].cmode)
	    /* actually preserve his nether realm cross party for this,
	       so he can tell everyone involved about Valinor in party chat: */
	    && p_ptr->auto_transport != AT_VALINOR
	    && !p_ptr->admin_dm)
		/* need to leave party, since we might be teamed up with incompatible char mode players! */
		party_leave(Ind, FALSE);
#endif
#ifdef IDDC_IRON_COOP
	if (p_ptr->party && at_irondeepdive(&p_ptr->wpos_old) && !at_irondeepdive(&p_ptr->wpos)
	    && compat_mode(p_ptr->mode, parties[p_ptr->party].cmode)
	    && !p_ptr->admin_dm)
		/* need to leave party, since we might be teamed up with incompatible char mode players! */
		party_leave(Ind, FALSE);
#endif
#ifdef ALLOW_NR_CROSS_ITEMS
	if (in_netherrealm(&p_ptr->wpos_old) && !in_netherrealm(&p_ptr->wpos))
		for (j = 1; j < INVEN_TOTAL; j++)
			p_ptr->inventory[j].NR_tradable = FALSE;
#endif
#ifdef IDDC_NO_TRADE_CHEEZE
	/* hack: abuse NR_tradable to mark items _untradable_ for first couple of IDDC floors */
	if (p_ptr->wpos.wx == WPOS_IRONDEEPDIVE_X &&
	    p_ptr->wpos.wy == WPOS_IRONDEEPDIVE_Y) {
		/* on entering the first floor, mark all items we already possess for anti-trade-cheeze */
		if (p_ptr->wpos.wz == WPOS_IRONDEEPDIVE_Z) {
			for (j = 1; j < INVEN_TOTAL; j++) {
				if (exceptionally_shareable_item(&p_ptr->inventory[j])) continue;
				p_ptr->inventory[j].NR_tradable = TRUE;
			}
		/* after reaching n-th floor, unmark them to make them freely available for trading again */
		} else if (ABS(p_ptr->wpos_old.wz) < IDDC_NO_TRADE_CHEEZE && ABS(p_ptr->wpos.wz) >= IDDC_NO_TRADE_CHEEZE) {
			for (j = 1; j < INVEN_TOTAL; j++)
				p_ptr->inventory[j].NR_tradable = FALSE;
		}
	}
#endif

	/* Immediately reset floor */
	if (in_deathfate(&p_ptr->wpos_old) && getcave(&p_ptr->wpos_old)
	    /* ..if no more players are on it */
	    && !players_on_depth(&p_ptr->wpos_old)
	    )
		dealloc_dungeon_level(&p_ptr->wpos_old);

	if (!in_deathfate_x(&p_ptr->wpos_old) && in_deathfate_x(&p_ptr->wpos)) {
		s_printf("DF-ENTER: %s (%s) -> %d\n", p_ptr->name, p_ptr->accountname, p_ptr->wpos_old.wz);
		df = TRUE;
	}
	if (in_deathfate_x(&p_ptr->wpos_old) && !in_deathfate_x(&p_ptr->wpos)) {
		s_printf("DF-LEAVE: %s (%s) <- %d\n", p_ptr->name, p_ptr->accountname, p_ptr->wpos_old.wz);
		df = TRUE;
	}

	wpcopy(&p_ptr->wpos_old, &p_ptr->wpos);

	/* Allow the player again to find another random IDDC town, if he hit a static IDDC town */
	if (is_fixed_irondeepdive_town(&p_ptr->wpos, dlv)) p_ptr->IDDC_found_rndtown = FALSE;
	/* Cover disallowing the same if he enters a random town someone else already generated */
	else if (in_irondeepdive(&p_ptr->wpos) && l_ptr && (l_ptr->flags1 & LF1_RANDOM_TOWN)) p_ptr->IDDC_found_rndtown = TRUE;

	/* hack -- update night/day in wilderness levels - this must be done even if previous wpos.wz was also zero, or towns would appear "unlit".
	   Note that this will currently also trigger gfx_hack_repaint as these functions also Send_weather_colouring() again. */
	if (!wpos->wz) {
		if (IS_DAY) player_day(Ind);
		else player_night(Ind);
	}
#ifdef EXTENDED_COLOURS_PALANIM
	world_surface_palette_player(Ind);
#endif

	/* Determine starting location */
	switch (p_ptr->new_level_method) {
	/* Recalled down */
	case LEVEL_RECALL_DOWN:
	/* Took staircase down */
	case LEVEL_DOWN:
		starty = level_down_y(wpos);
		startx = level_down_x(wpos);
		break;

	/* Recalled up */
	case LEVEL_RECALL_UP:
	/* Took staircase up */
	case LEVEL_UP:
		starty = level_up_y(wpos);
		startx = level_up_x(wpos);
		break;

	/* Teleported level */
	case LEVEL_RAND:
		starty = level_rand_y(wpos);
		startx = level_rand_x(wpos);
		break;

	/* Used ghostly travel */
	case LEVEL_PROB_TRAVEL:
	case LEVEL_GHOST:
		starty = p_ptr->py;
		startx = p_ptr->px;

		/* don't prob into sickbay area (also can't prob into inns) */
		if (zcave[starty][startx].feat == FEAT_SICKBAY_AREA) {
			tries = 1000;
			do {
				if (!(--tries)) break;
				starty = rand_int((l_ptr ? l_ptr->hgt : MAX_HGT) - 3) + 1;
				startx = rand_int((l_ptr ? l_ptr->wid : MAX_WID) - 3) + 1;
			} while (zcave[starty][startx].feat == FEAT_SICKBAY_AREA); /* don't recall him into sickbay areas */
			if (!tries) { /* just this one time */
				starty = p_ptr->py;
				startx = p_ptr->px;
			}
		}
		break;

	/* Over the river and through the woods */
	case LEVEL_OUTSIDE:
		smooth_ambient = TRUE; /* normal wilderness running */
		/* Fall through */
	case LEVEL_HOUSE:
		starty = p_ptr->py;
		startx = p_ptr->px;
		break;

		/* this is used instead of extending the level_rand_y/x
		into the negative direction to prevent us from
		alocing so many starting locations.  Although this does
		not make players teleport to simmilar locations, this
		could be achieved by seeding the RNG with the depth. */

	case LEVEL_OUTSIDE_RAND:
		/* make sure we aren't in an "icky" location */
		emergency_x = 0; emergency_y = 0; tries = 0;
		do {
			starty = rand_int((l_ptr ? l_ptr->hgt : MAX_HGT) - 3) + 1;
			startx = rand_int((l_ptr ? l_ptr->wid : MAX_WID) - 3) + 1;
			if (cave_floor_bold(zcave, starty, startx)) {
				emergency_x = startx;
				emergency_y = starty;
			}
		}
		while (((zcave[starty][startx].info & (CAVE_ICKY | CAVE_STCK | CAVE_NEST_PIT)) /* Don't recall into houses. Stck/Nest-pit shouldn't really happen on world surface though.. */
			|| feat_is_deep_water(zcave[starty][startx].feat)
			|| feat_is_deep_lava(zcave[starty][startx].feat)
			|| (zcave[starty][startx].feat == FEAT_SICKBAY_AREA) /* don't recall him into sickbay areas */
			|| (zcave[starty][startx].info & CAVE_PROT) /* don't recall into stables or inns */
			|| (f_info[zcave[starty][startx].feat].flags1 & FF1_PROTECTED)
			|| (zcave[starty][startx].feat == FEAT_SHOP) /* Prevent landing on a store entrance */
			|| (!cave_floor_bold(zcave, starty, startx)))
			&& (++tries < 10000));
		if (tries == 10000 && emergency_x) {
			startx = emergency_x;
			starty = emergency_y;
		}
		break;

	case LEVEL_OUTSIDE_CENTER: /* Special admin debugging, not for player transport */
		tries = 0;
		do {
			startx = (l_ptr ? l_ptr->wid : MAX_WID) / 2 + (rand_int(tries * 2 + 1) - tries) / 4;
			starty = (l_ptr ? l_ptr->hgt : MAX_HGT) / 2 + (rand_int(tries * 2 + 1) - tries) / 4;
		} while (!cave_floor_bold(zcave, starty, startx) && (++tries < 100));
		if (tries == 100) {
			startx = 1;
			starty = 1;
		}
		break;

	case LEVEL_TO_TEMPLE:
		/* Try to find a temple's sickbay area */
		for (y = 0; y < MAX_HGT; y++) {
			for (x = 0; x < MAX_WID; x++) {
				cave_type* c_ptr = &zcave[y][x];

				if (c_ptr->feat == FEAT_SICKBAY_DOOR) {
					/* Found the temple's sickbay area */
					startx = x;
					starty = y;
					break;
				}
			}
			if (startx) break;
		}

		/* Try to find a temple instead */
		if (!startx)
		for (y = 0; y < MAX_HGT; y++) {
			for (x = 0; x < MAX_WID; x++) {
				cave_type* c_ptr = &zcave[y][x];

				if (c_ptr->feat == FEAT_SHOP) {
					struct c_special *cs_ptr = GetCS(c_ptr, CS_SHOP);

					if (cs_ptr) {
						int which = cs_ptr->sc.omni;

						if (which == STORE_TEMPLE) {
							/* Found a temple */
							startx = x;
							starty = y;
							break;
						}
					}
				}
			}
			if (startx) break;
		}

		/* Go with random coordinates in case there's no temple */
		if (!startx) {
			starty = level_rand_y(wpos);
			startx = level_rand_x(wpos);
		}
		break;
	}

	/* Highlander Tournament hack: pseudo-teleport the player
	   after he took a staircase inside the highlander dungeon */
	if (sector000separation && in_highlander_dun(&p_ptr->wpos) && l_ptr)
		starty = l_ptr->hgt + 1; /* for the code right below :) ("Hack -- handle smaller floors") */

	/* Hack -- handle smaller floors */
	if (l_ptr && (starty > l_ptr->hgt || startx > l_ptr->wid)) {
		/* make sure we aren't in an "icky" location */
		emergency_x = 0; emergency_y = 0; tries = 0;
		do {
			starty = rand_int(l_ptr->hgt - 3) + 1;
			startx = rand_int(l_ptr->wid - 3) + 1;
			if (cave_floor_bold(zcave, starty, startx)
			    && !(zcave[starty][startx].info & CAVE_STCK)) {
				emergency_x = startx;
				emergency_y = starty;
			}
		}
		while (((zcave[starty][startx].info & CAVE_ICKY)
			|| (!cave_floor_bold(zcave, starty, startx)))
			&& (++tries < 10000));
		if (tries == 10000 && emergency_x) {
			startx = emergency_x;
			starty = emergency_y;
		}
	}

	/* Did we take a staircase into/inside a one-way dungeon? */
	took_oneway_stairs = ((p_ptr->new_level_method == LEVEL_UP || p_ptr->new_level_method == LEVEL_DOWN)
	    && d_ptr && ((d_ptr->flags1 & (DF1_FORCE_DOWN | DF1_NO_UP)) || (d_ptr->flags2 & DF2_IRON)));

	/* Did we use a level-changing method that should avoid any no-tele-vaults and nests/pits? */
	dont_end_up_in_ntvault_nestpit = ((p_ptr->new_level_method == LEVEL_RECALL_UP || p_ptr->new_level_method == LEVEL_RECALL_DOWN ||
	    p_ptr->new_level_method == LEVEL_RAND || p_ptr->new_level_method == LEVEL_OUTSIDE_RAND ||
	    p_ptr->new_level_method == LEVEL_PROB_TRAVEL || took_oneway_stairs)
	    && !(p_ptr->global_event_temp & PEVF_STCK_OK));

	//s_printf("finding area (%d,%d)\n", startx, starty);
	/* Place the player in an empty space */
	for (j = 0; j < 5000; j++) {
		/* Increasing distance */
		d = (j + 149) / 150;

		/* Pick a location */
		scatter(wpos, &y, &x, starty, startx, d, 1);

		/* Must have an "empty" grid */
		if (!cave_empty_bold(zcave, y, x)) continue;

		/* Not allowed to go onto a icky location (house) if Depth <= 0 */
		if (wpos->wz == 0 && !(p_ptr->global_event_temp & PEVF_ICKY_OK)) {
			if ((zcave[y][x].info & CAVE_ICKY) && (p_ptr->new_level_method != LEVEL_HOUSE)) continue;
			if (!(zcave[y][x].info & CAVE_ICKY) && (p_ptr->new_level_method == LEVEL_HOUSE)) continue;
		}

		/* Prevent recalling or prob-travelling into no-tele vaults and monster nests! - C. Blue
		   And also prevent taking staircases into these if we are in a one-way-only dungeon. */
		if ((zcave[y][x].info & (CAVE_STCK | CAVE_NEST_PIT)) && dont_end_up_in_ntvault_nestpit) continue;

		/* Prevent landing onto a store entrance */
		if (zcave[y][x].feat == FEAT_SHOP) continue;

		/* for new sickbay area: */
		if (p_ptr->new_level_method == LEVEL_TO_TEMPLE
		    && zcave[y][x].feat != FEAT_SICKBAY_AREA) continue;

		/* Must be inside the level borders - mikaelh */
		if (x < 1 || y < 1 || x > p_ptr->cur_wid - 2 || y > p_ptr->cur_hgt - 2)
			continue;

		/* should somewhat stay away from certain locations? (DK escape beacons) */
		if (p_ptr->avoid_loc)
			for (d = 0; d < p_ptr->avoid_loc; d++)
				if (distance(y, x, p_ptr->avoid_loc_y[d], p_ptr->avoid_loc_x[d]) < 8)
					continue;

		break;
	}
	/* this is required to make sense, isn't it.. */
	if (j == 5000) {
		s_printf("PPCW: No empty space (1).\n");
		x = startx;
		y = starty;

		/* REALLY don't recall/probtravel into no-tele.. | and also don't take one-way staircase now */
		if ((zcave[y][x].info & (CAVE_STCK | CAVE_NEST_PIT)) && dont_end_up_in_ntvault_nestpit) {
			for (starty = 1; starty < p_ptr->cur_hgt - 1; starty++) {
				for (startx = 1; startx < p_ptr->cur_wid - 1; startx++) {
					if (!(zcave[starty][startx].info & CAVE_STCK) &&
					    cave_empty_bold(zcave, starty, startx)) {
						x = startx;
						y = starty;
						break;
					}
				}
			}
		}
		if (starty == p_ptr->cur_hgt - 1 && startx == p_ptr->cur_wid - 1)
			s_printf("PPCW: No empty space (2).\n");
	}
	/* top paranoia check - if whole level is walled, place him in a wall instead of into another creature! */
	if (zcave[y][x].m_idx && zcave[y][x].m_idx != -Ind) {
		s_printf("PPCW: No empty space (3) checking..");
		for (starty = 1; starty < p_ptr->cur_hgt - 1; starty++) {
			for (startx = 1; startx < p_ptr->cur_wid - 1; startx++) {
				if (!(zcave[starty][startx].info & CAVE_STCK) && !zcave[starty][startx].m_idx) {
					x = startx;
					y = starty;
					s_printf("success.\n");
					j = 0;
					break;
				}
			}
			if (!j) break;
		}
		if (j) s_printf("failed!\n");
	}

	if (is_admin(p_ptr) && p_ptr->recall_x != 0 && p_ptr->recall_y != 0) {
		p_ptr->px = p_ptr->recall_x;
		p_ptr->py = p_ptr->recall_y;
		p_ptr->recall_x = 0;
		p_ptr->recall_y = 0;
	} else {
		p_ptr->px = x;
		p_ptr->py = y;
	}

	/* Update the player location */
	zcave[y][x].m_idx = 0 - Ind;
	cave_midx_debug(wpos, y, x, -Ind);

	/* Update his golem's location */
	/* Exception: Death Fate does not allow golems */
	if (!in_deathfate_x(wpos)) for (m_idx = m_top - 1; m_idx >= 0; m_idx--) {
		m_ptr = &m_list[m_fast[m_idx]];
		mcave = getcave(&m_ptr->wpos);

		if (!m_fast[m_idx]) continue;

		/* Excise "dead" monsters */
		if (!m_ptr->r_idx) continue;

		/* We're not the owner of this monster? */
		if (m_ptr->owner != p_ptr->id) continue;

		/* If the golem is set to stand guard instead of following, don't move it with us */
		if ((m_ptr->mind & GOLEM_GUARD) && !(m_ptr->mind & GOLEM_FOLLOW)) continue;

		/* XXX XXX XXX (merging) */
		starty = m_ptr->fy;
		startx = m_ptr->fx;
		starty = y;
		startx = x;

		if (!m_ptr->wpos.wz && !(m_ptr->mind & GOLEM_FOLLOW)) continue;
		/*
		   if ((m_ptr->mind & GOLEM_GUARD) && !(m_ptr->mind & GOLEM_FOLLOW)) continue;
		   */

		/* Place the golems in an empty space */
		for (j = 0; j < 1500; ++j) {
			/* Increasing distance */
			d = (j + 149) / 150;

			/* Pick a location */
			scatter(wpos, &my, &mx, starty, startx, d, 0);

			/* Must have an "empty" grid */
			if (!cave_empty_bold(zcave, my, mx)) continue;

			/* Not allowed to go onto a icky location (house) if Depth <= 0 */
			if ((wpos->wz == 0) && (zcave[my][mx].info & CAVE_ICKY))
				continue;
			break;
		}
		if (mcave) {
			mcave[m_ptr->fy][m_ptr->fx].m_idx = 0;
			everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
		}
		wpcopy(&m_ptr->wpos, wpos);
		mcave = getcave(&m_ptr->wpos);
		m_ptr->fx = mx;
		m_ptr->fy = my;
		if (mcave) {
			mcave[m_ptr->fy][m_ptr->fx].m_idx = m_fast[m_idx];
			everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
		}

		/* Update the monster (new location) */
		update_mon(m_fast[m_idx], TRUE);
	}
#if 0
	while (TRUE) {
		y = rand_range(1, ((Depth) ? (MAX_HGT - 2) : (SCREEN_HGT - 2)));
		x = rand_range(1, ((Depth) ? (MAX_WID - 2) : (SCREEN_WID - 2)));

		/* Must be a "naked" floor grid */
		if (!cave_naked_bold(zcave, y, x)) continue;

		/* Refuse to start on anti-teleport grids */
		if (zcave[y][x].info & CAVE_ICKY) continue;
		break;
	}
#endif

	panel_calculate(Ind);

	/* Hack -- remove tracked monster */
	health_track(Ind, 0);

	p_ptr->redraw |= (PR_MAP);
	p_ptr->redraw |= (PR_DEPTH);

	forget_view(Ind);
	forget_lite(Ind);
	/* original order: update_view first, then update_lite.  - C. Blue
	   Problem: can't see some grids when entering destroyed levels.
	   Fix: update lite before view.
	   New problem: Had panic save, thought maybe since lite depends on correctly
	    initialized view, but otoh view also seems to depend on lite oO.
	    Turned out that wasn't the panic save (fixed now).
	   Still new problem: Now after changing wpos, lite would be a square
	    instead of circle, until moving.
	   Fix: Add another update_view in the beginning -_-. */
	update_view(Ind);
	update_lite(Ind);
	update_view(Ind);
	update_monsters(TRUE);
	update_players();

	/* Tell him that he should beware */
	if (wpos->wz == 0 && !istown(wpos)) {
		if (wild_info[wpos->wy][wpos->wx].own) {
			cptr p = lookup_player_name(wild_info[wpos->wy][wpos->wx].own);

			if (p == NULL) p = "Someone";
			msg_format(Ind, "You enter the land of %s.", p);
		}
	}

	/* Clear the flag */
	p_ptr->new_level_flag = FALSE;

	/* Is arriving in a fixed IDDC-town noteworthy maybe? */
	if (is_fixed_irondeepdive_town(&p_ptr->wpos, dlv) && !is_admin(p_ptr)) {
		if (dlv == IDDC_TOWN1_FIXED) {
			msg_broadcast_format(0, "\374\377s%s has reached Menegroth.", p_ptr->name);
			l_printf("%s \\{s%s reached Menegroth\n", showdate(), p_ptr->name);
		} else if (dlv == IDDC_TOWN2_FIXED) {
			msg_broadcast_format(0, "\374\377s%s has reached Nargothrond.", p_ptr->name);
			l_printf("%s \\{s%s reached Nargothrond\n", showdate(), p_ptr->name);
		}
	}

	if (p_ptr->warning_depth != 2 && p_ptr->wpos.wz && !in_deathfate_x(&p_ptr->wpos)) {
		/* Too low to get any exp? */
		if (dlv < det_req_level(p_ptr->lev) - 5) {
			if (p_ptr->warning_depth != 2 ) {
				msg_print(Ind, "\377yYour level is very high compared to the dungeon floor you're currently on!");
				msg_print(Ind, "\377yFor that reason the depth is shown in grey colour (instead of white) in the");
				msg_print(Ind, "\377ybottom right corner of the main window, indicating that you will not gain any");
				msg_print(Ind, "\377yexperience from monster kills here due to the very low amount of threat.");
				s_printf("warning_depth=2: %s\n", p_ptr->name);
				p_ptr->warning_depth = 2;
			}
		}
		/* Too low to get 100% exp? */
		else if (dlv < det_req_level(p_ptr->lev)) {
			if (!p_ptr->warning_depth) {
				msg_print(Ind, "\377yYour level is quite high compared to the dungeon floor you're currently on!");
				msg_print(Ind, "\377yFor that reason the depth is shown in yellow colour (instead of white) in the");
				msg_print(Ind, "\377ybottom right corner of the main window, indicating that you will gain reduced");
				msg_print(Ind, "\377yamounts of experience from monster kills here due to the low amount of threat.");
				s_printf("warning_depth=1: %s\n", p_ptr->name);
				p_ptr->warning_depth = 1;
			}
		}
	}

	/* Did we enter/leave a no-run level? Temporarily disable/reenable warning_run */
	if ((l_ptr && (l_ptr->flags2 & LF2_NO_RUN)) || (in_sector000(&p_ptr->wpos) && (sector000flags2 & LF2_NO_RUN))) {
		if (p_ptr->warning_run < 3) p_ptr->warning_run += 10;
	} else if (p_ptr->warning_run >= 10) p_ptr->warning_run -= 10;

	/* warning messages, mostly for newbies */
	if (!p_ptr->ghost) { /* don't warn ghosts */
		if (p_ptr->warning_bpr2 != 1 && p_ptr->num_blow == 1 &&
		    /* and don't spam Martial Arts users or mage-staff wielders ;) */
		    p_ptr->inventory[INVEN_WIELD].k_idx && is_melee_weapon(p_ptr->inventory[INVEN_WIELD].tval)) {
			p_ptr->warning_bpr2 = p_ptr->warning_bpr3 = 1;
			msg_print(Ind, "\374\377yWARNING! You can currently perform only ONE 'blow per round' (attack).");
			msg_print(Ind, "\374\377y    If you rely on close combat, you should get at least 2 BpR!");
			msg_print(Ind, "\374\377y    Possible reasons: Weapon is too heavy or your strength is too low.");
			if (p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD) {
				if (p_ptr->rogue_like_commands)
					msg_print(Ind, "\374\377y    Try taking off your shield (\377oSHIFT+t\377y) and see if that helps.");
				else
					msg_print(Ind, "\374\377y    Try taking off your shield ('\377ot\377y' key) and see if that helps.");
			}
			switch (p_ptr->pclass) {
			case CLASS_WARRIOR:
				msg_print(Ind, "\374\377y    Warriors should try either a dagger, whip, spear or cleaver.");
				break;
			case CLASS_PALADIN:
				msg_print(Ind, "\374\377y    Paladins should try either a dagger, whip, spear or cleaver.");
				break;
#ifdef ENABLE_DEATHKNIGHT
			case CLASS_DEATHKNIGHT:
				msg_print(Ind, "\374\377y    Death Knights should try either a dagger, spear or cleaver.");
				break;
#endif
#ifdef ENABLE_HELLKNIGHT
			case CLASS_HELLKNIGHT:
				//msg_print(Ind, "\374\377y    Hell Knights should try either a dagger, spear or cleaver.");
				msg_print(Ind, "\374\377y    Hell Knights should try either a dagger, whip, spear or cleaver.");
				break;
#endif
			case CLASS_MIMIC:
				msg_print(Ind, "\374\377y    Mimics should try either a dagger, whip, spear or cleaver.");
				break;
			case CLASS_ROGUE:
				msg_print(Ind, "\374\377y    Rogues should try dual-wielding two daggers or main gauches.");
				break;
			case CLASS_RANGER:
				msg_print(Ind, "\374\377y    Rangers should try dual-wielding two daggers or sword & dagger.");
				break;
			}
			s_printf("warning_bpr23: %s\n", p_ptr->name);
		} else if (p_ptr->warning_wield == 0 &&
		    p_ptr->num_blow == 1 && !p_ptr->inventory[INVEN_WIELD].k_idx) {
			p_ptr->warning_wield = 1;
			msg_print(Ind, "\374\377yWARNING: You don't wield a weapon at the moment!");
			msg_print(Ind, "\374\377y    Press '\377Rw\377y' key. It lets you pick a weapon (as well as other items)");
			msg_print(Ind, "\374\377y    from your inventory to wield!");
			msg_print(Ind, "\374\377y    (If you plan to train 'Martial Arts' skill, ignore this warning.)");
			s_printf("warning_wield: %s\n", p_ptr->name);
		}
		if (p_ptr->warning_lite == 0 && p_ptr->cur_lite == 0 &&
		    (p_ptr->wpos.wz < 0 || (p_ptr->wpos.wz == 0 && night_surface))) { //Training Tower currently exempt
			if (p_ptr->wpos.wz < 0) p_ptr->warning_lite = 1;
			msg_print(Ind, "\374\377yHINT: You don't wield any light source at the moment!");
			msg_print(Ind, "\374\377y      Press '\377ow\377y' to wield a torch or lantern (or other items).");
			s_printf("warning_lite: %s\n", p_ptr->name);
		}
	}
#if 1
	if (p_ptr->warning_run < 3) {
		p_ptr->warning_run++;
		msg_print(Ind, "\374\377yHINT: To run fast, use \377oSHIFT+direction\377y keys.");
		msg_print(Ind, "\374\377y      For that, \377oNUMLOCK\377y key must be OFF and no awake monster in sight!");
		s_printf("warning_run: %s\n", p_ptr->name);
	}
#endif
	if (!p_ptr->warning_worldmap && p_ptr->wpos.wz == 0 &&
	    !in_sector000(&p_ptr->wpos) &&
	    (ABS(p_ptr->wpos.wx - 32) >= 2 || ABS(p_ptr->wpos.wy - 32) >= 2)) {
		msg_print(Ind, "\374\377yHINT: You can press '\377oM\377y' or '\377o~0\377y'to browse a worldmap.");
		msg_print(Ind, "\374\377y      Towns, for example Bree, are denoted as yellow 'T'.");
		s_printf("warning_worldmap: %s\n", p_ptr->name);
		p_ptr->warning_worldmap = 1;
	}

	if (!p_ptr->warning_dungeon && p_ptr->wpos.wz == 0 &&
	    !in_sector000(&p_ptr->wpos) &&
	    (ABS(p_ptr->wpos.wx - 32) >= 3 || ABS(p_ptr->wpos.wy - 32) >= 3)) {
		msg_print(Ind, "\374\377yHINT: Consider going to the Training Tower first, to gain some levels.");
		msg_print(Ind, "\374\377y      After that, seek out a dungeon. The tower is located in Bree.");
		s_printf("warning_dungeon: %s\n", p_ptr->name);
		p_ptr->warning_dungeon = 1;
	}

	if (!p_ptr->warning_powins) {
		for (j = 1; j < INVEN_PACK; j++) {
			if (!p_ptr->inventory[j].tval) break;
			if (p_ptr->inventory[j].tval != TV_BOOK || !is_custom_tome(p_ptr->inventory[j].sval)) continue;

			msg_print(Ind, "\374\377yHINT: Press \377o{\377- to power-inscribe your custom books, eg a codex.");
			msg_print(Ind, "\374\377y      When prompted for inscription, just enter: \377y@@@");
			s_printf("warning_powins: %s\n", p_ptr->name);
			p_ptr->warning_powins = 1;
			break;
		}
	}

	/* Hack -- jail her/him */
	if (!p_ptr->wpos.wz && p_ptr->tim_susp
#ifdef JAIL_TOWN_AREA
	    && istownarea(&p_ptr->wpos, MAX_TOWNAREA)
#endif
	    )
		imprison(Ind, JAIL_OLD_CRIMES, "old crimes");

	/* daylight problems for vampires; and nightly darkvision boost is _sometimes_ not revoked when recalling into the dungeon until first movement step is taken */
	if (p_ptr->prace == RACE_VAMPIRE
	/* temp luck blessings are on hold while on the surface - which is applied in calc_boni(): */
	    || p_ptr->bless_temp_luck) {
		calc_boni(Ind);
		if (p_ptr->prace == RACE_VAMPIRE) {
			p_ptr->update |= PU_TORCH;
			update_stuff(Ind);
		}
	}

	/* moved here, since it simplifies the wpos-changing process and
	   should keep it highly consistent and linear.
	   Note: Basically every case that sets p_ptr->new_level_flag = TRUE
	   can and should be stripped of immediate check_Morgoth() call and
	   instead rely on its occurance here. Also, there should actually be
	   no existing case of check_Morgoth() that went without setting said flag,
	   with the only exception of server join/leave in nserver.c and Morgoth
	   live spawn (ie not on level generation time) in monster2.c. - C. Blue */
	check_Morgoth(Ind);
	if (df) check_df();
	if (p_ptr->new_level_flag) return;

#ifdef CLIENT_SIDE_WEATHER
	/* update his client-side weather */
	player_weather(Ind, TRUE, TRUE, TRUE);
#endif

	/* Brightly lit Arena Monster Challenge */
	if (ge_special_sector && in_arena(&p_ptr->wpos)) {
		wiz_lite_extra(Ind);
		/* Also tell him about this place */
		for (j = 0; j < MAX_GLOBAL_EVENTS; j++) {
			if (global_event[j].getype != GE_ARENA_MONSTER) continue;
			msg_print(Ind, "\377uYou enter the Arena Monster Challenge!");
			msg_format(Ind, "\377uType '\377U/evinfo %d\377u' to learn more or '\377U/evsign %d\377u' to challenge.", j + 1, j + 1);
			break;
		}
	}

	if (in_pvparena(&p_ptr->wpos)) {
		/* teleport after entering [the PvP arena],
		   so stairs won't be camped */
		teleport_player(Ind, 200, TRUE);
		/* Brightly lit PvP Arena */
		wiz_lite_extra(Ind);
	}

	/* PvP-Mode specialties: */
	if ((p_ptr->mode & MODE_PVP) && p_ptr->wpos.wz &&
	    !in_pvparena(&p_ptr->wpos)) { /* <- not in the pvp arena actually */
		/* anti-chicken: Don't allow a high player to 'chase' a very low player */
		for (j = 1; j <= NumPlayers; j++) {
			if (Players[j]->conn == NOT_CONNECTED) continue;
			if (is_admin(Players[j])) continue;
			if (!(Players[j]->mode & MODE_PVP)) continue;
			if (!inarea(&Players[j]->wpos, &p_ptr->wpos)) continue;
			if (Players[j]->max_plv < p_ptr->max_plv - 5) {
				p_ptr->new_level_method = (p_ptr->wpos.wz > 0 ? LEVEL_RECALL_DOWN : LEVEL_RECALL_UP);
				p_ptr->recall_pos.wz = 0;
				p_ptr->recall_pos.wx = p_ptr->wpos.wx;
				p_ptr->recall_pos.wy = p_ptr->wpos.wy;
				//set_recall_timer(Ind, 1);
				p_ptr->word_recall = 1;
				msg_print(Ind, "\377yYou have sudden visions of a bawking chicken while being recalled!");
			}
		}
	}

	/* Hack: Allow players to pass trees always, while in town */
	if (istown(&p_ptr->wpos) || isdungeontown(&p_ptr->wpos)) {
		p_ptr->town_pass_trees = TRUE;

		if (!p_ptr->warning_drained) {
			if (p_ptr->exp < p_ptr->max_exp ||
			    p_ptr->stat_cur[A_STR] != p_ptr->stat_max[A_STR] ||
			    p_ptr->stat_cur[A_INT] != p_ptr->stat_max[A_INT] ||
			    p_ptr->stat_cur[A_WIS] != p_ptr->stat_max[A_WIS] ||
			    p_ptr->stat_cur[A_DEX] != p_ptr->stat_max[A_DEX] ||
			    p_ptr->stat_cur[A_CON] != p_ptr->stat_max[A_CON] ||
			    p_ptr->stat_cur[A_CHR] != p_ptr->stat_max[A_CHR]) {
				msg_print(Ind, "\374\377yHINT: When your attributes (STR/INT/WIS/DEX/CON/CHR) or experience (XP) are");
				msg_print(Ind, "\374\377y      drained they are displayed in yellow colour. Buy a potion in town to");
				msg_print(Ind, "\374\377y      restore them! The alchemist sells potions for stats, the temple for xp.");
				s_printf("warning_drained: %s\n", p_ptr->name);
				p_ptr->warning_drained = 1;
			}
		}
	} else p_ptr->town_pass_trees = FALSE;

	/* Give warning message to use word-of-recall, aimed at newbies */
	if (!p_ptr->warning_wor2 && p_ptr->wpos.wx == cfg.town_x && p_ptr->wpos.wy == cfg.town_y && p_ptr->wpos.wz == -1 && p_ptr->max_plv <= 10) {
		/* scan inventory for any scrolls */
		bool found_items = FALSE;
		int i;

		for (i = 0; i < INVEN_PACK; i++) {
			if (!p_ptr->inventory[i].k_idx) continue;
			if (!object_known_p(Ind, &p_ptr->inventory[i])) continue;
			if (p_ptr->inventory[i].tval == TV_SCROLL &&
			    p_ptr->inventory[i].sval == SV_SCROLL_WORD_OF_RECALL)
				found_items = TRUE;
		}
		if (!found_items) {
			msg_print(Ind, "\374\377yHINT: You can use scrolls of \377Rword-of-recall\377y to teleport out of a dungeon");
			msg_print(Ind, "\374\377y      or back into it, making the tedious search for stairs obsolete!");
			s_printf("warning_wor2: %s\n", p_ptr->name);
		}
		p_ptr->warning_wor2 = 1;
	}

	if (!p_ptr->warning_elder && p_ptr->wpos.wx == cfg.town_x && p_ptr->wpos.wy == cfg.town_y && p_ptr->wpos.wz == 0 && p_ptr->max_plv >= 5) {
		msg_print(Ind, "\374\377yHINT: You can ask the town elder in Bree from time to time about \"\377Uadvice\377y\"!");
		msg_print(Ind, "\374\377y      Based on your level, skills, items she might have life-saving suggestions.");
		s_printf("warning_elder: %s\n", p_ptr->name);
		p_ptr->warning_elder = 1;
	}

#if 0 /* since digging is pretty awesome now, this is too much, and we should be glad that treasure detection items have some use now actually! */
	/* High digging results in auto-treasure detection */
	if (get_skill(p_ptr, SKILL_DIG) >= 30) floor_detect_treasure(Ind);
#endif

	/* If he found a town in the dungeon, map it as much as a normal town would be at night */
	if (l_ptr && (l_ptr->flags1 & LF1_DUNGEON_TOWN)) player_dungeontown(Ind);

	quest_check_player_location(Ind);

#ifdef USE_SOUND_2010
	/* clear boss/floor-specific music */
	p_ptr->music_monster = -1;
	handle_ambient_sfx(Ind, &(getcave(&p_ptr->wpos)[p_ptr->py][p_ptr->px]), &p_ptr->wpos, smooth_ambient);
#endif

#ifdef ENABLE_SELF_FLASHING
	/* if not travelling through wilderness smoothly,
	   flicker player for a moment, to allow for easy location */
	if (!smooth_ambient && p_ptr->flash_self) p_ptr->flashing_self = cfg.fps / FLASH_SELF_DIV2;
#endif

	grid_affects_player(Ind, -1, -1);

	clockin(Ind, 7); /* Remember his wpos */

	/* Note: IDDC is already covered in cmd2.c when taking entrance staircases */
	if (in_hallsofmandos(wpos)) {
		p_ptr->warning_depth = 2;
		p_ptr->warning_wor = p_ptr->warning_wor2 = 1;
	}

	if (in_deathfate_x(wpos)) wiz_lite_extra(Ind);

	/* Display this warning at most once per floor. Once per secret area would be nice but requires some non-trivial coding... */
	p_ptr->warning_secret_area = FALSE;

	// DYNAMIC_CLONE_MAP (worldmap, while not shopping): extract code from wild_display_mini_map()
}

void steamblast_trigger(int idx) {
	cave_type **zcave = getcave(&steamblast_wpos[idx]);

	if (zcave) {
		int x = steamblast_x[idx], y = steamblast_y[idx];
		cave_type *c_ptr = &zcave[y][x];
		struct worldpos *wpos = &steamblast_wpos[idx];

		c_ptr->info &= ~CAVE_STEAMBLAST; /* colour back to normal */

		/* Closed door, locked doors, jammed doors -- works 100% for now */
		if (c_ptr->feat >= FEAT_DOOR_HEAD && c_ptr->feat <= FEAT_DOOR_TAIL) {
		    // && !(f_info[c_ptr->feat].flags2 & FF2_NO_TFORM) && !(c_ptr->info & CAVE_NO_TFORM)
			c_ptr->info &= ~CAVE_STEAMBLAST;
			cave_set_feat_live(wpos, y, x, FEAT_BROKEN);
#ifdef USE_SOUND_2010
			sound_near_site(y, x, wpos, 0, "fireworks_norm", NULL, SFX_TYPE_COMMAND, FALSE); //fireworks_small, detonation

#endif
		}
		/* Locked chest? */
		else if (c_ptr->o_idx) {
			object_type *o_ptr = &o_list[c_ptr->o_idx];

			if (o_ptr->tval == TV_CHEST && (o_ptr->temp & 0x08)) {
				trap_kind *t_ptr;
				int disarm = steamblast_disarm[idx];

				o_ptr->temp &= ~0x08;
				everyone_lite_spot(wpos, y, x);
#ifdef USE_SOUND_2010
				sound_near_site(y, x, wpos, 0, "fireworks_norm", NULL, SFX_TYPE_COMMAND, FALSE); //fireworks_small, detonation
#endif
				if (o_ptr->custom_lua_usage) exec_lua(0, format("custom_object_usage(%d,%d,%d,%d,%d)", 0, c_ptr->o_idx, -1, 10, o_ptr->custom_lua_usage));

				t_ptr = &t_info[o_ptr->pval];
				disarm = disarm - t_ptr->difficulty * 3;
				/* Always have a small chance of success */
				if (disarm < 2) disarm = 2;
				if (rand_int(100) < disarm) {
					/* Actually disarm it without setting off the trap */
					o_ptr->pval = (0 - o_ptr->pval);
					object_known(o_ptr);
					if (o_ptr->custom_lua_usage) exec_lua(0, format("custom_object_usage(%d,%d,%d,%d,%d)", 0, c_ptr->o_idx, 0, 9, o_ptr->custom_lua_usage));
				} else {
					/* Apply chest traps, if any */
					/* Some traps might destroy the chest on setting off : o_ptr->sval -> SV_CHEST_RUINED */
					/* Custom LUA hacks */
					if (o_ptr->xtra1) exec_lua(0, format("custom_chest_trap(%d,%d)", 0, o_ptr->xtra1));
					/* Don't skip normal trap routines? */
					if (!(o_ptr->xtra4 & 0x8)) {
						/* Set off trap */
						(void)generic_activate_trap_type(wpos, y, x, o_ptr, c_ptr->o_idx);
						if ((o_ptr->xtra3 & 0x1) || /* Erase chest whenever the trap was set off */
						    (o_ptr->sval == SV_CHEST_RUINED && (o_ptr->xtra3 & 0x2))) /* Erase the chest if it got ruined by the trap */
							delete_object_idx(c_ptr->o_idx, FALSE, FALSE);
						else {
							/* If chest is not ruined, also disarm + unlock */
							//o_ptr->pval = 0;
							o_ptr->pval = (0 - o_ptr->pval);
							object_known(o_ptr);
							if (o_ptr->custom_lua_usage) exec_lua(0, format("custom_object_usage(%d,%d,%d,%d,%d)", 0, c_ptr->o_idx, 0, 9, o_ptr->custom_lua_usage));
						}
					} else {
						/* If chest is not ruined, also disarm + unlock it */
						//o_ptr->pval = 0;
						o_ptr->pval = (0 - o_ptr->pval);
						object_known(o_ptr);
						if (o_ptr->custom_lua_usage) exec_lua(0, format("custom_object_usage(%d,%d,%d,%d,%d)", 0, c_ptr->o_idx, 0, 9, o_ptr->custom_lua_usage));
					}
				}
			}
		}
	}

	steamblast_timer[idx] = steamblast_timer[steamblasts - 1];
	steamblast_x[idx] = steamblast_x[steamblasts - 1];
	steamblast_y[idx] = steamblast_y[steamblasts - 1];
	steamblast_disarm[idx] = steamblast_disarm[steamblasts - 1];
	steamblast_wpos[idx] = steamblast_wpos[steamblasts - 1];
	steamblasts--;
}



/*
 * Main loop --KLJ--
 *
 * This is main loop; it is called every 1/FPS seconds.  Usually FPS is about
 * 10, so that a normal unhasted unburdened character gets 1 player turn per
 * second.  Note that we process every player and the monsters, then quit.
 * The "scheduling" code (see sched.c) is the REAL main loop, which handles
 * various inputs and timings.
 */

void dungeon(void) {
	int i, k;
	static int export_turns = 1; // Setting this to '1' makes it run on server startup
	player_type *p_ptr;

	/* Return if no one is playing */
	/* if (!NumPlayers) return; */

	/* Check for death.  Go backwards (very important!) */
	for (i = NumPlayers; i > 0; i--) {
		/* Check connection first */
		if (Players[i]->conn == NOT_CONNECTED) continue;
		/* Check for death */
		if (Players[i]->death) player_death(i);
	}

	/* Check player's depth info */
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		if (p_ptr->conn == NOT_CONNECTED || !p_ptr->new_level_flag) continue;

		/* Auto-retire iron winners */
		if (p_ptr->iron_winner_ded && p_ptr->wpos.wz != 0
		    /* Still allow dedicated-mode iron winners to reenter iddc/mandos, to actually be able to do something? */
		    && !in_irondeepdive(&p_ptr->wpos) && !in_hallsofmandos(&p_ptr->wpos)
		    ) {
			p_ptr->new_level_flag = FALSE; //clean up, needed?
			do_cmd_suicide(i);
			continue;
		}

		/* Process any wpos-change of player */
		process_player_change_wpos(i);
	}

	/* New meta client implementation */
	meta_tick();

	/* Handle any network stuff */
	Net_input();


	/* Note -- this is the END of the last turn */

	/* Do final end of turn processing for each player */
	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Actually process that player */
		process_player_end(i);
	}

	/* paranoia - It's done twice */
	/* Check for death.  Go backwards (very important!) */
	for (i = NumPlayers; i > 0; i--) {
		/* Check connection first */
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Check for death */
		if (Players[i]->death)
			player_death(i);
	}



	///*** BEGIN NEW TURN ***///
	turn += turn_plus;

	/* Check for overflow - mikaelh */
#if 0 /* avoid doing this last-minute, since some calcs will already be off */
	if (turn < 0)
#else /* give at least ~30 minutes buffer (eg login process requires that approx.) */
	if (turn == turn_overflow) /* note: turn is s32b */
#endif
	{
		/* Reset the turn counter */
		/* This will cause some weird things */
		/* Players will probably timeout */
		turn = 1;

		/* Fix/reset turn-related durations/timeouts */
		for (k = 0; k < numtowns; k++) {
			for (i = 0; i < max_st_idx; i++) {
				store_type *st_ptr = &town[k].townstore[i];

				st_ptr->last_theft = st_ptr->tim_watch = 0;
				st_ptr->last_visit = 0;
			}
		}
		for (i = 1; i <= NumPlayers; i++) {
			Players[i]->last_gold_drop_timer = 0;
			Players[i]->go_turn = 0;
			Players[i]->item_order_turn = turn + HOUR; //ew, well this should be fine
			if (Players[i]->suspended) Players[i]->suspended = cfg.fps * 2; /* hrm */

			/* Pft: */
			Players[i]->test_turn = 0;
		}
		/* Process the monsters */
		for (k = m_top - 1; k >= 0; k--) {
			i = m_fast[k];
			if (m_list[i].suspended) m_list[i].suspended = cfg.fps * 2; /* hrrm */
		}
		/* Reset to full duration again, lucky hunters */
		for (i = 0; i < MAX_XORDERS; i++)
			if (xorders[i].active && xorders[i].id) xorders[i].turn = 0;
		/* Events will have wrong timings or display absurd durations,
		   affected: ge->start_turn, ge->end_turn */
		for (i = 0; i < MAX_GLOBAL_EVENTS; i++)
			stop_global_event(0, i);

		/* Log it */
		s_printf("%s: TURN COUNTER RESET (%d)\n", showtime(), turn_overflow);

		/* Reset empty party creation times */
		for (i = 1; i < MAX_PARTIES; i++) {
			if (parties[i].members == 0) parties[i].created = 0;
		}
	}

	/* Do some queued drawing */
	process_lite_later();

#ifdef PLAYER_STORES
 #ifdef EXPORT_PLAYER_STORE_OFFERS
  #if EXPORT_PLAYER_STORE_OFFERS > 0
	/* Export player store items every n hours */
	if (!(turn % (cfg.fps * 60 * EXPORT_PLAYER_STORE_OFFERS))) export_player_store_offers(&export_turns);
	else if (export_turns) export_player_store_offers(&export_turns);
  #endif
 #endif
#endif

	/* Process solo-reking timers (if enabled) and quest (de)activations, once a minute. */
	if (!(turn % (cfg.fps * 60))) {
#ifdef SOLO_REKING
		/* Process fallen winners */
		for (i = 1; i <= NumPlayers; i++) {
			if (!Players[i]->solo_reking) continue;

			Players[i]->solo_reking -= 250; // 1 min = 250 au
			if (Players[i]->solo_reking < 0) Players[i]->solo_reking = 0;
		}
#endif

		/* Process quest (de)activation every minute */
		process_quests();
	}

	/* Process some things once each second.
	   NOTE: Some of these (global events) mustn't be handled _after_ a player got set to 'dead',
	   or gameplay might in very rare occasions get screwed up (someone dying when he shouldn't) - C. Blue */
	if (!(turn % cfg.fps)) {
		player_type *p_ptr;

		/* Process global_events */
		process_global_events();

		/* process special event timers */
		process_timers();

		/* Call a specific lua function every second */
		exec_lua(0, "second_handler()");

		/* Process weather effects */
#ifdef CLIENT_SIDE_WEATHER
 #ifndef CLIENT_WEATHER_GLOBAL
		process_wild_weather();
 #else
		process_weather_control();
 #endif
#else
		process_weather_control();
#endif

#ifdef AUCTION_SYSTEM
		/* Process auctions */
		process_auctions();
#endif

#ifdef USE_SOUND_2010
		process_ambient_sfx();
#endif

		/* process timed shutdown (/shutrec) */
		if (shutdown_recall_timer) shutdown_recall_timer--;

		/* Steam blast charge - Fighting technique */
		for (i = 0; i < steamblasts; i++) {
			steamblast_timer[i]--;
			if (!steamblast_timer[i]) steamblast_trigger(i);
		}

		/* process certain player-related timers */
		k = 0; //party-colourization
		for (i = 1; i <= NumPlayers; i++) {
			p_ptr = Players[i];

			/* CTRL+R spam control */
			if (p_ptr->redraw_cooldown) p_ptr->redraw_cooldown--;

			/* Custom timer countdowns */
			if (p_ptr->custom_timer) {
				p_ptr->custom_timer--;
				if (p_ptr->custom_timer == -86400 - 1) {
					msg_print(i, "\376\377vYour custom timer reached max count of 86400s (1 day).");
#ifdef USE_SOUND_2010
					if (p_ptr->audio_sfx) sound(i, "gong", "bell", SFX_TYPE_ALERT, FALSE);
					else Send_warning_beep(i);
#else
					if (p_ptr->paging < 3) p_ptr->paging = 3;
#endif
					p_ptr->custom_timer = 0;
				} else if (!p_ptr->custom_timer) {
					msg_print(i, "\376\377vYour custom timer finished.");
#ifdef USE_SOUND_2010
					if (p_ptr->audio_sfx) sound(i, "gong", "bell", SFX_TYPE_ALERT, FALSE);
					else Send_warning_beep(i);
#else
					if (p_ptr->paging < 3) p_ptr->paging = 3;
#endif
				}
			}

			/* Temporary public-chat-muting from spam */
			if (p_ptr->mutedtemp) {
				p_ptr->mutedtemp--;
				if (!p_ptr->mutedtemp) msg_print(i, "You are no longer muted.");
			}

#ifdef USE_SOUND_2010
			/* Arbitrary max number, just to prevent integer overflow.
			   Should just be higher than the longest interval of any ambient sfx type. */
			if (!p_ptr->wpos.wz && /* <- redundant check sort of, caught in process_player_change_wpos() anyway */
			    p_ptr->ambient_sfx_timer < 200)
				p_ptr->ambient_sfx_timer++;
#endif

#ifdef ENABLE_GO_GAME
			/* Kifu email spam control */
			if (p_ptr->go_mail_cooldown) p_ptr->go_mail_cooldown--;
#endif

			/* Party-colorization (admins only): not player-specific, but iterates over all online players anyway */
			if (p_ptr->party && parties[p_ptr->party].set_attr == TRUE) {
				parties[p_ptr->party].set_attr = FALSE;
				if (k == TERM_L_DARK - 1) k++; //reserved for iron teams
				if (k == TERM_SLATE - 1) k++; //skip too similar white tone maybe, QoL
				//if (k == TERM_ORANGE - 1) k++; //slightly cluttery with orange-coloured depths
				if (k == TERM_L_WHITE - 1) k++; //skip too similar white tone maybe, QoL
				parties[p_ptr->party].attr = ++k;
				k = k % TERM_MULTI;
			}
		}

		/* Free firework drops sometimes, in the inn in Bree :o (inn size is 5x5) */
		if (season_newyearseve && !rand_int(60 * 10)) { //on average once per 10 minutes -> ~4h to fill the inn if not unlucky x/y rolls
			struct worldpos wpos = {32, 32, 0};
			cave_type **zcave = getcave(&wpos);

			if (zcave) {
				object_type forge, *o_ptr;
				int x, y, t = 100, available = 0;

				/* Create random firework scroll */
				invcopy(&forge, lookup_kind(TV_SCROLL, SV_SCROLL_FIREWORK));
				forge.number = 1;
				apply_magic(&wpos, &forge, -1, FALSE, FALSE, FALSE, FALSE, RESF_NONE);
				forge.level = 1;

				/* The inn in Bree, hardcoded coordinates ugh */
				while (--t) {
					x = 120 + rand_int(5);
					y = 27 + rand_int(5);
					if (zcave[y][x].o_idx) {
						o_ptr = &o_list[zcave[y][x].o_idx];
						if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_FIREWORK) available++;
						continue;
					}
					drop_near(FALSE, 0, &forge, 0, &wpos, y, x);
					break;
				}

				/* In case the inn is full, drop one in Bree, but much more seldomly */
				if (!t && available < 2 && !rand_int(6)) { //on average once per 60 minutes
					t = 200;
					while (--t) {
						x = rand_int(MAX_WID - 2) + 1;
						y = rand_int(MAX_HGT - 2) + 1;
						if (drop_near(FALSE, 0, &forge, 0, &wpos, y, x) > 0) break;
					}
				}
			}
		}
	}
	if ((turn % cfg.fps) == cfg.fps - 1)
		for (i = 1; i <= NumPlayers; i++)
			parties[Players[i]->party].set_attr = TRUE;

	/* Nether Realm collapsing animations - process every 1/5th second */
	if (!(turn % (cfg.fps / 5))) {
		if (nether_realm_collapsing && !rand_int(8)) {
			struct worldpos wpos = {netherrealm_wpos_x, netherrealm_wpos_y, netherrealm_end_wz};
			int x = nrc_x, y = nrc_y - 5; /* start somewhat 'above' Zu-Aon's death spot */

			/* stay in bounds */
			if (x < 20) x = rand_int(37) + 4;//don't cut them off too much
			else if (x >= MAX_WID - 20) x = MAX_WID - 40 + rand_int(36);//don't cut them off too much
			else x = x - 20 + rand_int(41);
			if (y < 15) y = rand_int(16);
			else if (y >= MAX_HGT - 15) y = MAX_HGT - 30 + rand_int(23);//don't cut them off at the bottom border too much
			else y = y - 15 + rand_int(31);

			//cast_lightning(&wpos, rand_int(MAX_WID - 20) + 10, rand_int(MAX_HGT - 15) + 5);
			cast_lightning(&wpos, x, y);
		}
	}

#ifndef CLIENT_SIDE_WEATHER
	/* Process server-side weather */
	process_weather_effect_creation();
#endif

	/* Process server-side visual special fx */
	if (season_newyearseve && fireworks && !(turn % (cfg.fps / 4)))
		process_firework_creation();



	/* Do some beginning of turn processing for each player */
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		if (p_ptr->conn == NOT_CONNECTED) continue;

		if (p_ptr->test_turn_idle && p_ptr->test_turn) {
			p_ptr->idle_attack++;
			if (p_ptr->idle_attack >= p_ptr->test_turn_idle) {
				p_ptr->test_turn = p_ptr->test_turn + p_ptr->test_turn_idle;
				tym_evaluate(i);
				p_ptr->test_turn = p_ptr->test_turn_idle = 0;
			}
		}

		/* Print queued log messages (anti-spam feature) - C. Blue */
		if (p_ptr->last_gold_drop && turn - p_ptr->last_gold_drop_timer >= cfg.fps * 2) {
			s_printf("Gold dropped (%d by %s at %d,%d,%d) [anti-spam].\n", p_ptr->last_gold_drop, p_ptr->name, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			p_ptr->last_gold_drop = 0;
			p_ptr->last_gold_drop_timer = turn;
		}

#if 0
		{
			/* experimental water animation, trying to not be obnoxious -
			   strategy: pick a random grid, if it's water, redraw it */
			cave_type **zcave = getcave(&p_ptr->wpos);

			if (zcave) {
				int x = rand_int(p_ptr->panel_col_max - p_ptr->panel_col_min) + p_ptr->panel_col_min;
				int y = rand_int(p_ptr->panel_row_max - p_ptr->panel_row_min) + p_ptr->panel_row_min;

				lite_spot(i, y, x);
 #if 0
				struct dun_level *l_ptr = getfloor(&p_ptr->wpos);

				/* outside? */
				if (!l_ptr) {
					int x = rand_int(MAX_WID), y = rand_int(MAX_HGT);

					if (zcave[y][x].feat == FEAT_DEEP_WATER ||
					    zcave[y][x].feat == FEAT_SHAL_WATER)
						lite_spot(i, y, x);
				/* inside? */
				} else {
					int x = rand_int(l_ptr->wid), y = rand_int(l_ptr->hgt);

					if (zcave[y][x].feat == FEAT_DEEP_WATER ||
					    zcave[y][x].feat == FEAT_SHAL_WATER)
						lite_spot(i, y, x);
				}
 #endif
			}
		}
#endif

#ifdef ENABLE_SELF_FLASHING
		/* Check for hilite */
		if (p_ptr->flashing_self > 0 && !(turn % (cfg.fps / 15))) {
			p_ptr->flashing_self--;
			//everyone_lite_spot_move(i, &p_ptr->wpos, p_ptr->py, p_ptr->px);
			lite_spot(i, p_ptr->py, p_ptr->px);
		} else
#endif
		if (!(turn % (cfg.fps / 6))) {
			/* Play server-side animations (consisting of cycling/random colour choices,
			   instead of animated colours which are circled client-side) */
			/* Flicker invisible player? (includes colour animation!) */
			//if (p_ptr->invis) update_player(i);
			if (p_ptr->invis) {
				update_player_flicker(i);
				update_player(i);
			}
			/* Otherwise only take care of colour animation */
			//else { /* && p_ptr->body_monster */
				everyone_lite_spot_move(i, &p_ptr->wpos, p_ptr->py, p_ptr->px);
			//}
		}

		/* Do non-self-related animations and checks too */
		if (!(turn % (cfg.fps / 6))) {
#ifdef TELEPORT_SURPRISES
			/* Teleport-surprise-effect against monsters */
			if (p_ptr->teleported) p_ptr->teleported--;
#endif
		}

		/* Perform beeping players who are currently being paged by others */
		if (p_ptr->paging && !(turn % (cfg.fps / 4))) {
			Send_beep(i);
			p_ptr->paging--;
		}

		/* Actually process that player */
		process_player_begin(i);

		/* Process the world of that player */
		process_world_player(i);
	}

	/* Process spell effects */
	process_effects();

	/* Process projections' teleportation effects on monsters */
	if (scan_do_dist) { /* Optimization: Only check all monsters when needed */
		for (i = 0; i < m_top; i++) {
			if (m_list[m_fast[i]].do_dist) {
				teleport_away(m_fast[i], m_list[m_fast[i]].do_dist);
				m_list[m_fast[i]].do_dist = FALSE;
			}
		}
		scan_do_dist = FALSE;
	}

#ifdef FLUENT_ARTIFACT_RESETS
	process_artifacts();
#endif

#ifdef ENABLE_MERCHANT_MAIL
	process_merchant_mail();
#endif

	/* Process all of the monsters */
#ifdef ASTAR_DISTRIBUTE
	process_monsters_astar();
#endif
#ifdef PROCESS_MONSTERS_DISTRIBUTE
	process_monsters();
#else
	if (!(turn % MONSTER_TURNS)) process_monsters();
#endif

	/* Process programmable NPCs */
	if (!(turn % NPC_TURNS)) process_npcs();

	/* Process all of the objects */
	/* Currently, process_objects() only recharges rods on the floor and in trap kits.
	   It must be called as frequently as process_player_end_aux(), which recharges rods in inventory,
	   so they're both recharging at the same rate.
	   Note: the exact timing for each object is measured inside the function. */
	process_objects();

	/* Process the world */
	if (!(turn % 50)) process_world();


	/* Tell a day passed */
	//if (((turn + (DAY_START * 10L)) % (10L * DAY)) == 0)
	if (!(turn % DAY)) { /* midnight */
		char buf[20];

		snprintf(buf, 20, "%s", get_day(bst(YEAR, turn))); /* hack: abuse get_day()'s capabilities */
		msg_broadcast_format(0,
			"\377GToday it is %s of the %s year of the third age.",
			get_month_name(bst(DAY, turn), FALSE, FALSE), buf);

#ifdef MUCHO_RUMOURS
		/* the_sandman prints a rumour */
		if (NumPlayers) {
			/* Pick the 1st player arbitrarily, it's broadcast to the rest due to how fortune() works.. */
			msg_print(1, "Suddenly a thought comes to your mind:");
			fortune(1, 2);
		}
#endif
	}

	/* Additional townie monster spawn (t, ie specifically Bree), independant of any players present! */
	if (!(turn % ((cfg.fps * 5) / 6)) && !rand_int(TOWNIE_RESPAWN_CHANCE)) {
		monster_level = 0;
		(void)alloc_monster(BREE_WPOS_P, MAX_SIGHT + 5, FALSE);
	}

	/* Clean up Bree regularly to prevent too dangerous towns in which weaker characters cant move around */
	thin_surface_spawns(); //distributes workload now

	/* Used to be in process_various(), but changed it to distribute workload over all frames now. */
	purge_old();

	/* Process everything else */
	if (!(turn % (cfg.fps / 6))) {
		char buf[1024];
		FILE *fp;

		process_various();

		/* Hack -- Regenerate the monsters */
		if (!(turn % ((cfg.fps * 10) / 6))) regen_monsters();

		/* process delayed requests */
		for (i = 1; i <= NumPlayers; i++) {
			p_ptr = Players[i];
			if (p_ptr->delay_str) {
				p_ptr->delay_str -= 10;
				if (p_ptr->delay_str <= 0) {
					p_ptr->delay_str = 0;
					Send_request_str(i, p_ptr->delay_str_id, p_ptr->delay_str_prompt, p_ptr->delay_str_std);
				}
			}
			if (p_ptr->delay_cfr) {
				p_ptr->delay_cfr -= 10;
				if (p_ptr->delay_cfr <= 0) {
					p_ptr->delay_cfr = 0;
					Send_request_cfr(i, p_ptr->delay_cfr_id, p_ptr->delay_cfr_prompt, p_ptr->delay_cfr_default_choice);
				}
			}
		}

		/* EXPERIMENTAL: Poll for AI responses, output them through 8ball. - C. Blue */
#define AI_MAXLEN 4096 /* Maximum length of AI's response string to read */
#define AI_MULTILINE 2 /* Allow AI responses to be multiple [2] chat lines long */
		path_build(buf, 1024, ANGBAND_DIR_DATA, "external-response.log");
		if ((fp = fopen(buf, "r")) != NULL) {
			if (!feof(fp)) {
				char strbase[AI_MAXLEN], *str, *c, strtmp[1024], *open_parenthesis, *o, *p;
				bool within_parentheses = FALSE;
				/* Cut message of at MSG_LEN minus "\374\377y[8ball] " chat prefix length, and -6 for world-broadcast server prefix eg '\377g[1] ': */
				int maxlen = MSG_LEN - 1 - 11 - 6; /* and note that this maxlen is the real content length, not a null-terminated string length */
#if AI_MULTILINE > 0
				char strm[AI_MULTILINE][MSG_LEN], c1;
				int m_max = 0;
#endif

				if (fgets(strbase, AI_MAXLEN, fp) != NULL) {
					strbase[AI_MAXLEN - 1] = 0;
					str = strbase;
					/* Trim leading spaces */
					while (*str == ' ') str++;

					/* Change all " into ' to avoid conflict with lua eight_ball("..") command syntax. */
					c = str - 1;
					while(*(++c)) if (*c == '"') *c = '\'';
					/* Remove all linebreaks or LUA will break */
					c = str - 1;
					while(*(++c)) if (*c == '\n' || *c == '\r') *c = ' ';

					/* If str actually isn't empty (buffer overflow then on accessing strlen-1), trim trailing spaces */
					if (*c) while (c[strlen(c) - 1] == ' ') c[strlen(c) - 1] = 0;
#if AI_MULTILINE > 0
					/* Dissect -possibly very long- response string into multiple chat messages if required;
					   only treat the last one with shortening/cutting procedures. */
					while (strlen(str) > maxlen && m_max < AI_MULTILINE - 1) {
						/* Fill a chat line, cutting off the rest */
						strncpy(strm[m_max], str, maxlen);
						strm[m_max][maxlen] = 0; /* remember note: content length, no null-termination needed (so no '-1') */
						/* Try not to cut off the line within a word */
						do {
							c = strm[m_max] + strlen(strm[m_max]) - 1;
							c1 = tolower(*c);
							if (c1 < 'a' || c1 > 'z') break;
							*c = 0;
						} while (TRUE);

						/* Prepare to fill another chat line if needed */
						str = str + strlen(strm[m_max]);
						m_max++;

						/* Trim trailing spaces of the strm[] we just finished now (so we actually discard spaces completely, instead of them going into the next strm[]) */
						while (strm[m_max - 1][strlen(strm[m_max - 1]) - 1] == ' ') strm[m_max - 1][strlen(strm[m_max - 1]) - 1] = 0;
					}
#endif

					open_parenthesis = strchr(str, '(');

					/* If response exceeds our maximum message length, get rid of parentheses-structs first */
					while (strlen(str) > maxlen && open_parenthesis) {
						p = NULL;
						o = open_parenthesis;

						while (!p) {
							p = strchr(o, ')');
							if (!p) break;

							/* Skip smileys within parentheses.. */
							if (p == str || *(p - 1) != '-') break;
							o = p;
							p = NULL;
						}

						/* Crop out the parentheses struct */
						if (p) {
							strcpy(strtmp, str);
							if (*(p + 1) == ' ') p++; /* Skip a space after the closing parenthesis */
							strcpy(o, strtmp + (p - str) + 1);

							open_parenthesis = strchr(str, '(');
						} else break;
					}

					/* Truncate so we don't exceed our maximum message length! (Panic save ensues) */
					str[maxlen] = 0; /* remember note: content length, no null-termination needed (so no '-1') */

					/* Anything left to process? Or we will get buffer overflows from accessing strlen-1 positions */
					if (*str) {
						/* Special maintenance/status response given by control scripts? */
						if (!((*str == '<' && str[strlen(str) - 1] == '>') || (*str == '[' && str[strlen(str) - 1] == ']'))) {
							/* Cut off trailing remains of a sentence -_- (even required for AI response, as it also gets cut off often).
							   Try to make sure we catch at least one whole sentence, denotedly limited by according punctuation marks. */
							c = str + strlen(str) - 1;
							while(c > str && ((*c != '.' && *c != '?'  && *c != '!' && *c != ';') || within_parentheses)) {
								if (open_parenthesis) {
									if (*c == ')' && *(c - 1) != '-') within_parentheses = TRUE;
									if (*c == '(') within_parentheses = FALSE;
								}
								c--;
							}
							/* ..however, some responses have so long sentences that there is maybe only a comma, none of the above marks.. */
							if (c == str) {
								c = str + strlen(str) - 1;
								while(c > str && *c != ',') c--;
								/* Avoid sillily short results */
								if (c < str + 10) c = str;
							}
							/* ..and some crazy ones don't even have a comma :/ ..*/
							if (c == str) {
								char *c1, *c2, *c3, *c4, *c5;

								c = str + strlen(str) - 1;
								/* Beeeest effort at "language" gogo.. */
								c1 = my_strcasestr(str, "that");
								c2 = my_strcasestr(str, "what");
								c3 = my_strcasestr(str, "which");
								c4 = my_strcasestr(str, "who");
								c5 = my_strcasestr(str, "where");
								if (c2 > c1) c1 = c2;
								if (c3 > c1) c1 = c3;
								if (c4 > c1) c1 = c4;
								if (c5 > c1) c1 = c5;
								c = c1;
								/* Also strip the space before this word */
								if (c > str) c--;
								/* Avoid sillily short results */
								if (c < str + 10) c = NULL;
							}

							/* Found any valid way to somehow truncate the line? =_= */
							if (c) {
								if (*c != '?' && *c != '!') *c = '.'; /* At the end of the text, replace a comma or semicolon or space, but not an exclamation mark. */
								*(c + 1) = 0;
							}

							/* A new weirdness has popped up: It started generating [more and more] trailing dot-triplets, separated with spaces, at the end of each answer */
							if (*str && *(str + 1)) // paranoia? ensure there is some string left, so strlen-2 doesn't buffer-overflow */
								while (str[strlen(str) - 1] == ' ' || (str[strlen(str) - 1] == '.' && (str[strlen(str) - 2] == '.'
								    || str[strlen(str) - 2] == ' ' || str[strlen(str) - 2] == '?' || str[strlen(str) - 2] == '!')))
									str[strlen(str) - 1] = 0;
						}
					}
#if AI_MULTILINE > 0
					/* Add the treated 'str' to our multiline array too, just to make it look orderly ^^ */
					strcpy(strm[m_max], str);
					m_max++;
					/* ..and output all lines */
					for (c1 = 0; c1 < m_max; c1++)
						exec_lua(0, format("eight_ball(\"%s\")", strm[(int)c1])); //don't cry, compiler -_- @(int)
#else
					exec_lua(0, format("eight_ball(\"%s\")", str));
#endif
				}
			}
			fclose(fp);

			/* Clear response file after having processed the response (through 8ball), as it's no longer pending but has been processed now. */
			/* Create a backup just for debugging/checking: */
			remove(format("%s.bak", buf));
			rename(buf, format("%s.bak", buf));
		}
	}

#ifdef DUNGEON_VISIT_BONUS
	/* Keep track of frequented dungeons, every minute */
	if (!(turn % (cfg.fps * 60))) process_dungeon_boni();
	/* Decay visit frequency for all dungeons over time, every 10 minutes */
	if (!(turn % (cfg.fps * 60 * 10))) process_dungeon_boni_decay();
#endif

	/* Process day/night changes on world_surface */
#ifndef EXTENDED_COLOURS_PALANIM
	if (!(turn % HOUR)) process_day_and_night();
#else /* call it once every palette-animation-step time interval */
	if (!(turn % (HOUR / PALANIM_HOUR_DIV))) process_day_and_night();
#endif

	/* Refresh everybody's displays */
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Notice stuff */
		if (p_ptr->notice) notice_stuff(i);

		/* Update stuff */
		if (p_ptr->update) update_stuff(i);

		/* Redraw stuff */
		if (p_ptr->redraw2) redraw2_stuff(i);
		if (p_ptr->redraw) redraw_stuff(i);

		/* Window stuff */
		if (p_ptr->window) window_stuff(i);
	}

	/* Animate player's @ in his own view here on server side */
	/* This animation is only for mana shield / GOI indication */
	if (!(turn % (cfg.fps / 12)))
	for (i = 1; i <= NumPlayers; i++) {
		/* Colour animation is done in lite_spot */
		lite_spot(i, Players[i]->py, Players[i]->px);
	}

	/* Process debugging/helper functions - C. Blue */
	if (store_debug_mode &&
	    /* '/ 10 ' stands for quick motion, ie time * 10.
	       Doesn't support cfg.dun_store_turns, but instead just uses cfg.store_turns even for dungeon stores.
	       Doesn't support cfg.book_store_turns_perc. */
	    (!(turn % ((store_debug_mode * (10L * cfg.store_turns)) / store_debug_quickmotion))))
		store_debug_stock();

#ifdef ARCADE_SERVER
	/* Process special Arcade Server things */
	if (turn % (cfg.fps / 3) == 1) exec_lua(1, "firin()");
 #define tron_speed ((cfg.fps + 3) / 7)
	if (!tron_speed || !(turn % tron_speed)) exec_lua(1, "tron()");
#endif

#ifdef ENABLE_GO_GAME
	/* Process Go AI engine communication (its replies) */
	if (go_engine_processing && !(turn % (cfg.fps / 10))) go_engine_process();
#endif

	/* Send any information over the network */
	Net_output();
}

void set_runlevel(int val) {
	//static bool meta = TRUE;

	switch (val) {
		case -1:
			/* terminate: return a value that lets a script know that
			   it must not restart us, in this case that is cfg.runlevel = -1, set below... */
			s_printf("***SERVER MAINTENANCE TERMINATION***\n");
			/* Fall through */
		case 0:
			cfg.runlevel = val;
			shutdown_server();
		case 1:
			/* Logout all remaining players except admins */
			msg_broadcast(0, "\377rServer shutdown imminent!");
			break;
		case 2:
			/* Force recalling of all players to town if
			   configured */
			msg_broadcast(0, "\377rServer about to close down. Recall soon");
			break;
		case 3:
			/* Hide from the meta - server socket still open */
			Report_to_meta(META_DIE);
			//meta = FALSE;
			msg_broadcast(0, "\377rServer will close soon.");
			break;
		case 4:
			/* Dungeons are closed */
			msg_broadcast(0, "\377yThe dungeons are now closed.");
			break;
		case 5:
			/* Shutdown warning mode, automatic timer */
			//msg_broadcast(0, "\377yWarning. Server shutdown will take place in five minutes.");
			msg_broadcast(0, "\374\377yWarning. Server shutdown will take place in ten minutes.");
			break;
		case 1024:
			Report_to_meta(META_DIE);
			//meta = FALSE;
			break;
			/* Hack -- character edit (possessor) mode */
		case 2042:
		case 2043:
		case 2044:
		case 2045:
		case 2046:
		case 2047:
		case 2048:
		case 2051:
		case 2053:
			/* Shutdown as soon as server is empty (admins don't count) */
			break;
		case 2049: //neither is 2052
			/* Usually not called here - just a temporary hack value (see dungeon.c) */
			shutdown_server();
			break;
		case 6:
			/* Running */
		default:
			/* Cancelled shutdown */
			if (cfg.runlevel != 6) msg_broadcast(0, "\377GServer shutdown cancelled.");
			Report_to_meta(META_START);
			//meta = TRUE;
			val = 6;
			break;
	}
	time(&cfg.closetime);
	cfg.runlevel = val;
}

/*
 * Actually play a game
 *
 * If the "new_game" parameter is true, then, after loading the
 * server-specific savefiles, we will start anew.
 */
void play_game(bool new_game, bool all_terrains, bool dry_Bree, bool lowdun_near_Bree, bool new_wilderness, bool new_flavours, bool new_houses) {
	int h = 0, m = 0, s = 0, dwd = 0, dd = 0, dm = 0, dy = 0;
	time_t now;
	struct tm *tmp;
	//int i, n;

redo_world:

	/* Init the RNG */
	// Is it a good idea ? DGDGDGD --  maybe FIXME
	//if (Rand_quick)
	{
		u32b seed;

		/* Basic seed */
		seed = (time(NULL));

#ifdef SET_UID

		/* Mutate the seed on Unix machines */
		seed = ((seed >> 3) * (getpid() << 1));

#endif

		/* Use the complex RNG */
		Rand_quick = FALSE;

		/* Seed the "complex" RNG */
		Rand_state_init(seed);
	}

	/*** Init the wild_info array... for more information see wilderness.c ***/
	init_wild_info();

	/* Load list of banned players */
	load_banlist();

	/* Attempt to load the server state information */
	if (!load_server_info()) {
		/* Oops */
		quit("broken server savefile(s)");
	}

	/* Initialize 'runtime' tracker: Just count up each time the server gets (re)started (until overflow, then just continue counting up again).
	   Note: Since it initializes at 0, it will start at 1 after first-time increase here -
	   this might be important in the light that older player savegames are initialized to value 0 instead, and hence differ by default.
	   So to keep this behaviour consistent, we check whether we reach 0 after an overflow here and jump to 1 to skip the 0 value again. */
	runtime_server++;
	/* Hack: For consistent behaviour, always skip '0' value - here potentially happening in any case of an arithmetic overflow: */
	if (!runtime_server) runtime_server++;

	/* Nothing loaded */
	if (!server_state_loaded) {
		/* Make server state info */
		new_game = TRUE;

		/* Create a new dungeon */
		server_dungeon = FALSE;
	}
	else server_dungeon = TRUE;

	/* Roll new town */
	if (new_game) {
		/* Ignore the dungeon */
		server_dungeon = FALSE;

		/* Start in town */
		/* dlev = 0;*/

		/* Hack -- seed for flavors */
		seed_flavor = rand_int(0x10000000);

		/* Hack -- seed for town layout */
		seed_town = rand_int(0x10000000);

		/* Initialize server state information */
		server_birth();

		/* Generate the wilderness */
		genwild(all_terrains, dry_Bree);

		/* Generate the towns */
		if (!wild_spawn_towns(lowdun_near_Bree)) goto redo_world;


		/* Create dungeon index info */
		s_printf("Indexing dungeons..\n");
		reindex_dungeons();

		/* Hack -- force town surrounding types
		 * This really shouldn't be here; just a makeshift and to be
		 * replaced by multiple-town generator */
		//wild_bulldoze();

		/* Hack -- enter the world */
		turn = 1;
	} else {
		/* use with lua_forget_map() */
		if (new_wilderness) {
			int i, x, y;
			wilderness_type *w_ptr;
			struct dungeon_type *d_ptr;

			s_printf("Resetting wilderness..\n");

			/* Hack -- seed for town layout */
			seed_town = rand_int(0x10000000);

			/* erase all house information and re-init, consistent with init_other() */
			s_printf("  ..erasing old info..\n");

			/* free old objects (worldmap/houses/monster inventory) */
			for (i = 0; i < o_max; i++) {
				/* Skip dead objects */
				if (!o_list[i].k_idx) continue;
				if (true_artifact_p(&o_list[i])) handle_art_d(o_list[i].name1);
				questitem_d(&o_list[i], o_list[i].number);
			}
			C_KILL(o_list, MAX_O_IDX, object_type);
			C_MAKE(o_list, MAX_O_IDX, object_type);
			o_max = 1;
			o_nxt = 1;
			o_top = 0;

			/* free old monsters */
			C_KILL(m_list, MAX_M_IDX, monster_type);
			C_MAKE(m_list, MAX_M_IDX, monster_type);
			m_max = 1;
			m_nxt = 1;
			m_top = 0;

			/* free old houses[] info */
			for (i = 0; i < num_houses; i++) {
				KILL(houses[i].dna, struct dna_type);
				if (!(houses[i].flags & HF_RECT))
					C_KILL(houses[i].coords.poly, MAXCOORD, char);
#ifndef USE_MANG_HOUSE_ONLY
				C_KILL(houses[i].stock, houses[i].stock_size, object_type);
#endif
			}
			C_KILL(houses, house_alloc, house_type);
			C_KILL(houses_bak, house_alloc, house_type);
			house_alloc = 1024;
			C_MAKE(houses, house_alloc, house_type);
			C_MAKE(houses_bak, house_alloc, house_type);
			num_houses = 0;

			/* free old town[] and town[]->store info */
			if (town) {
				for (i = 0; i < numtowns; i++) dealloc_stores(i);
				C_KILL(town, numtowns, struct town_type);
			}
			numtowns = 0;

			/* free old dungeon/tower info */
			for (y = 0 ;y < MAX_WILD_Y; y++) {
				for (x = 0; x < MAX_WILD_X; x++) {
					w_ptr = &wild_info[y][x];
					if (w_ptr->flags & WILD_F_DOWN) {
						d_ptr = w_ptr->dungeon;
#ifndef UNIQUES_KILLED_ARRAY
						for (i = 0; i < d_ptr->maxdepth; i++)
							C_KILL(d_ptr->level[i].uniques_killed, MAX_R_IDX, char);
#endif
						C_KILL(d_ptr->level, d_ptr->maxdepth, struct dun_level);
						KILL(d_ptr, struct dungeon_type);
					}
					if (w_ptr->flags & WILD_F_UP) {
						d_ptr = w_ptr->tower;
#ifndef UNIQUES_KILLED_ARRAY
						for (i = 0; i < d_ptr->maxdepth; i++)
							C_KILL(d_ptr->level[i].uniques_killed, MAX_R_IDX, char);
#endif
						C_KILL(d_ptr->level, d_ptr->maxdepth, struct dun_level);
						KILL(d_ptr, struct dungeon_type);
					}
				}
			}

			/*** Init the wild_info array... for more information see wilderness.c ***/
			init_wild_info();

			/* Ignore the dungeon */
			server_dungeon = FALSE;

			/* Generate the wilderness */
			s_printf("  ..generating wilderness..\n");
			genwild(all_terrains, dry_Bree);

			/* Generate the towns */
			s_printf("  ..spawning towns..\n");
			if (!wild_spawn_towns(lowdun_near_Bree)) goto redo_world;

			/* Discard old dungeon index info */
			s_printf("  ..reindexing dungeons..\n");
			reindex_dungeons();

			s_printf("..done.\n");
		}
		if (new_flavours) {
			/* use with lua_forget_flavours() */
			s_printf("Resetting flavours.\n");

			/* Hack -- seed for flavors */
			seed_flavor = rand_int(0x10000000);
		}
		if (new_houses) {
			int i;

			s_printf("Reinitializing houses.\n");

			/* free old houses[] info */
			for (i = 0; i < num_houses; i++) {
				KILL(houses[i].dna, struct dna_type);
				if (!(houses[i].flags & HF_RECT))
					C_KILL(houses[i].coords.poly, MAXCOORD, char);
#ifndef USE_MANG_HOUSE_ONLY
				C_KILL(houses[i].stock, houses[i].stock_size, object_type);
#endif
			}
			C_KILL(houses, house_alloc, house_type);
			C_KILL(houses_bak, house_alloc, house_type);
			house_alloc = 1024;
			C_MAKE(houses, house_alloc, house_type);
			C_MAKE(houses_bak, house_alloc, house_type);
			num_houses = 0;
		}
	}

	/* Initialize wilderness 'level' */
	init_wild_info_aux(0,0);


	/* Flash a message */
	s_printf("Please wait...\n");

	/* Flavor the objects */
	flavor_init();
	flavor_hacks();

	s_printf("Object flavors initialized...\n");

	/* Reset the visual mappings */
	reset_visuals();

	/* Make a town if necessary */
	/*
	 * NOTE: The central town of Angband is handled in a special way;
	 * If it was static when saved, we shouldn't allocate it.
	 * If not static, or it's new game, we should allocate it.
	 */
	/*
	 * For sure, we should redesign it for real multiple-towns!
	 * - Jir -
	 */
	if (!server_dungeon) {
		/* Allocate space for it */
		alloc_dungeon_level(BREE_WPOS_P);

		/* Actually generate the town */
		generate_cave(BREE_WPOS_P, NULL);
	}

	/* Finish initializing dungeon monsters */
	setup_monsters();

	/* Detect and fix wrong monster counts. Important to keep uniques spawnable:
	   Their counts might have been corrupted by panic save or similar crash. */
	/* First set all to zero */
	for (h = 0; h < MAX_R_IDX; h++) r_info[h].cur_num = 0;
	/* Now count how many monsters there are of each race */
	for (h = 1; h < m_max; h++)
		if (m_list[h].r_idx) r_info[m_list[h].r_idx].cur_num++;

	/* Finish initializing dungeon objects */
	setup_objects();

	/* Server initialization is now "complete" */
	server_generated = TRUE;

	/* Set up the contact socket, so we can allow players to connect */
	setup_contact_socket();

	/* Set up the network server */
	if (Setup_net_server() == -1)
		quit("Couldn't set up net server");

	/* scan for inactive players */
	scan_characters();
	scan_accounts();
	scan_houses();

#if defined(CLIENT_SIDE_WEATHER) && !defined(CLIENT_WEATHER_GLOBAL)
	/* initialize weather */
	wild_weather_init();
#endif

	init_day_and_night();

#ifdef FIREWORK_DUNGEON
	init_firework_dungeon();
#endif

	cfg.runlevel = 6;		/* Server is running */

	/* Set up the main loop */
	install_timer_tick(dungeon, cfg.fps);

	/* Execute custom startup script - C. Blue */
	time(&now);
	tmp = localtime(&now);
	h = tmp->tm_hour;
	m = tmp->tm_min;
	s = tmp->tm_sec;
	get_date(&dwd, &dd, &dm, &dy);
	exec_lua(0, format("playloop_startup(\"%s\", %d, %d, %d, %d, %d, %d, %d)", showtime(), h, m, s, dwd, dd, dm, dy));

	quest_handle_disabled_on_startup();

	/* Loop forever */
	sched();

	/* This should never, ever happen */
	s_printf("sched returned!!!\n");

	/* Close stuff */
	close_game();

	/* Quit */
	quit(NULL);
}


void shutdown_server(void) {
	s_printf("Shutting down.\n");

	/* Kick every player out and save his game */
	while (NumPlayers > 0) {
		/* Note the we always save the first player */
		player_type *p_ptr = Players[1];

		/* Player gets a free IDDC anti-logscum pass, in case he was in the IDDC and not in a town, so his level got stale w/o his doing.
		   This pass is invalidated right after login in any case, even not applied. */
		if (!p_ptr->IDDC_logscum) p_ptr->IDDC_freepass = TRUE;

		/* Notify players who are afk and even pseudo-afk rogues */
#if 0 /* always send a page beep? */
		Send_beep(1);
#endif
#if 0 /* only when afk or idle? */
		if (p_ptr->afk) Send_beep(1);
		else if (p_ptr->turns_idle >= cfg.fps * AUTO_AFK_TIMER) Send_beep(1);
#endif
#if 1 /* send a warning sound (usually same as page beep) */
#ifdef USE_SOUND_2010
		//sound(1, "warning", "page", SFX_TYPE_NO_OVERLAP, FALSE);
		sound(1, "page", NULL, SFX_TYPE_NO_OVERLAP, FALSE);
#endif
#endif
		Net_output1(1);

		/* Indicate cause */
		strcpy(p_ptr->died_from, "server shutdown");
		p_ptr->died_from_ridx = 0;

		/* Try to save */
		if (!save_player(1)) Destroy_connection(p_ptr->conn, "Server shutdown (save failed)");

		/* Successful save */
		if (cfg.runlevel == -1)
			Destroy_connection(p_ptr->conn, "Server is undergoing maintenance and may take a little while to restart.");
		else
			Destroy_connection(p_ptr->conn, "Server has been updated, please login again."); /* was "Server shutdown (save succeeded)" */
	}

	/* Save list of banned players */
	save_banlist();

	/* Save dynamic quest info */
	save_quests();

	/* Hack: Erase formerly safe floors, ie Arena Monster Challenge.
	   Otherwise if it looks the same after restart and has the same monster in it,
	   people can die for real! (Because ge_special_sector is not saved.) */
	if (ge_special_sector) {
		struct worldpos wpos = { WPOS_ARENA_X, WPOS_ARENA_Y, WPOS_ARENA_Z };

		dealloc_dungeon_level(&wpos);
	}

	/* Save the server state */
	if (!save_server_info()) quit("Server state save failed!");

	/* Tell the metaserver that we're gone */
	Report_to_meta(META_DIE);

#ifdef ENABLE_GO_GAME
	/* Shut down Go AI engine and its pipes */
	go_engine_terminate();
#endif

	if (cfg.runlevel == -1) quit("Terminating");
	else quit("Server state saved");

	/* Just to be sure and to to keep the compiler happy */
	exit(-1);
}

void pack_overflow(int Ind) {
	player_type *p_ptr = Players[Ind];

	object_type *o_ptr;
	int amt, i, j = 0;
	u32b f1, f2, f3, f4, f5, f6, esp;
	char o_name[ONAME_LEN];


	/* XXX XXX XXX Pack Overflow */
	if (!p_ptr->inventory[INVEN_PACK].k_idx) return;

	/* Choose an item to spill */
	for (i = INVEN_PACK - 1; i >= 0; i--) {
		o_ptr = &p_ptr->inventory[i];
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		if (!check_guard_inscription(o_ptr->note, 'd') &&
		    !((f4 & TR4_CURSE_NO_DROP) && cursed_p(o_ptr)) &&
		    !o_ptr->questor) {
			j = 1;
			break;
		}
	}

	if (!j) i = INVEN_PACK;
#ifdef SUBINVEN_LIMIT_GROUP
	/* Be on the safe side, do NOT potentially drop a bag in use for another bag we just picked up:
	   Too annoying to lose all its contents too, for zero potential value from the new bag item. */
	else if (p_ptr->inventory[INVEN_PACK].tval == TV_SUBINVEN) i = INVEN_PACK;
#endif

	/* Access the slot to be dropped */
	o_ptr = &p_ptr->inventory[i];

	/* Drop all of that item */
	amt = o_ptr->number;

	/* Disturbing */
	disturb(Ind, 0, 0);

	/* Warning */
	//msg_print(Ind, "\376\377oYour pack overflows!");

	/* Describe */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	/* Message */
	msg_format(Ind, "\376\377oYour pack overflows! You drop %s.", o_name);

#ifdef USE_SOUND_2010
	sound_item(Ind, o_ptr->tval, o_ptr->sval, "drop_");
#endif

	/* Drop it (carefully) near the player */
	drop_near(TRUE, Ind, o_ptr, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);

#ifdef ENABLE_SUBINVEN
	/* Paranoia? - If we drop a subinventory, remove all items and place them into the player's inventory */
	if (o_ptr->tval == TV_SUBINVEN) empty_subinven(Ind, i, FALSE, FALSE);
#endif

	/* Decrease the item, optimize. */
	inven_item_increase(Ind, i, -amt);
	inven_item_optimize(Ind, i);

	/* Newest item is actually the one that caused the overflow,
	   which now moved from overflow slot INVEN_PACK down one into normal inventory slot 'w'. */
	if (o_ptr->mode & MODE_NOT_NEWEST_ITEM) {
		o_ptr->mode &= ~MODE_NOT_NEWEST_ITEM;
		Send_item_newest_2nd(Ind, INVEN_PACK - 1);
	} else Send_item_newest(Ind, INVEN_PACK - 1);
}

/* Handle special timed events that don't fit in /
   aren't related to 'Global Events' routines - C. Blue
   (To be called every second.)
   Now also used for Go minigame. */
void process_timers() {
	struct worldpos wpos;
	cave_type **zcave;
	int y, x, i;
	player_type *p_ptr;

	for (i = 1; i < CUSTOM_LUA_TIMERS; i++) { //accomodate for LUA arrays starting at index 1, just play it safe
		if (!custom_lua_timer_timeout[i]) continue;
		custom_lua_timer_timeout[i]--;
		if (!custom_lua_timer_timeout[i])
			exec_lua(0, format("custom_lua_timer(%d,\"%s\",%d,%d,%d)", i,
			    custom_lua_timer_parmstr[i], custom_lua_timer_parm1[i], custom_lua_timer_parm2[i], custom_lua_timer_parm3[i]));
	}

#ifdef ENABLE_GO_GAME
	/* Process Go AI engine communication (its replies) */
	if (go_game_up || go_wait_for_sgf) go_engine_clocks();
#endif

	/* reduce warning_rest cooldown */
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		if (p_ptr->conn == NOT_CONNECTED) continue;
		if (p_ptr->warning_rest_cooldown) {
			p_ptr->warning_rest_cooldown--;
//#if WARNING_REST_TIMES > 0 /* comment in, then move this to Receive_run() and Receive_walk()? */
			if (!p_ptr->warning_rest_cooldown &&
			    ((p_ptr->chp * 10) / p_ptr->mhp <= 5 || (p_ptr->mmp && ((p_ptr->cmp * 10) / p_ptr->mmp <= 2)))) {
				msg_print(i, "\374\377RHINT: Press \377oSHIFT+r\377R to rest, so your hit points will");
				msg_print(i, "\374\377R      regenerate faster! Also true for mana and stamina!");
				p_ptr->warning_rest_cooldown = 60;
 #if WARNING_REST_TIMES > 0
				p_ptr->warning_rest++;
				if (p_ptr->warning_rest == WARNING_REST_TIMES) acc_set_flags(p_ptr->accountname, ACC_WARN_REST, TRUE);
 #endif
				s_printf("warning_rest: %s\n", p_ptr->name);
			}
//#endif
		}
	}

	/* PvP Arena in 0,0 - Release monsters: */
	if (timer_pvparena1) {
		/* check for existence of arena */
		wpos.wx = WPOS_PVPARENA_X;
		wpos.wy = WPOS_PVPARENA_Y;
		wpos.wz = WPOS_PVPARENA_Z;
		if ((zcave = getcave(&wpos))) {

			timer_pvparena1--;
			if (!timer_pvparena1) {
				/* start next countdown */
				timer_pvparena1 = 90; //45 was too fast, disallowing ppl to leave the arena sort of

				if ((timer_pvparena3 % 2) == 0) { /* cycle preparation? */
					/* clear old monsters and items */
					wipe_m_list(&wpos);
					wipe_o_list_safely(&wpos);
					/* close all doors */
					for (i = 1; i <= 6; i++) {
						y = 2;
						x = (i * 10) - 3;
						zcave[y][x].feat = FEAT_SEALED_DOOR;
						everyone_lite_spot(&wpos, y, x);
					}
				}

				//moar monsters: 196 (wolf), 313 (uruk), 278 (frost giant), 563 (y r dragon), 547 (mumak), 609 (baron of hell)
				if (timer_pvparena3 == 0) { /* prepare first cycle */
					y = 1;
					x = (1 * 10) - 3;
					summon_override_checks = SO_ALL;
					//866 elite uruk, 563 young red dragon
					//place_monster_aux(&wpos, y, x, 866, FALSE, FALSE, 100, 0);
					//place_monster_one(&wpos, y, x, 866, FALSE, FALSE, FALSE, 100, 0);
					place_monster_one(&wpos, y, x, 102, FALSE, FALSE, FALSE, 100, 0);//large k
					x = (2 * 10) - 3;
					//487 storm giant
					//place_monster_aux(&wpos, y, x, 487, FALSE, FALSE, 100, 0);
					//place_monster_one(&wpos, y, x, 563, FALSE, FALSE, FALSE, 100, 0);
					place_monster_one(&wpos, y, x, 249, FALSE, FALSE, FALSE, 100, 0);//light Z(271) //vlasta(249)
					x = (3 * 10) - 3;
					//609 baron of hell
					//place_monster_aux(&wpos, y, x, 590, FALSE, FALSE, 100, 0);
					//place_monster_one(&wpos, y, x, 487, FALSE, FALSE, FALSE, 100, 0);
					place_monster_one(&wpos, y, x, 866, FALSE, FALSE, FALSE, 100, 0);//elite o
					x = (4 * 10) - 3;
					//590 mature gold d
					//place_monster_aux(&wpos, y, x, 720, FALSE, FALSE, 100, 0);
					//place_monster_one(&wpos, y, x, 720, FALSE, FALSE, FALSE, 100, 0);
					place_monster_one(&wpos, y, x, 563, FALSE, FALSE, FALSE, 100, 0);//young red d
					x = (5 * 10) - 3;
					//995 marilith, 558 colossus
					//place_monster_aux(&wpos, y, x, 558, FALSE, FALSE, 100, 0);
					//place_monster_one(&wpos, y, x, 558, FALSE, FALSE, FALSE, 100, 0);
					place_monster_one(&wpos, y, x, 194, FALSE, FALSE, FALSE, 100, 0);//tengu
					x = (6 * 10) - 3;
					//602 bronze D, 720 barbazu
					//place_monster_aux(&wpos, y, x, 609, FALSE, FALSE, 100, 0);
					//place_monster_one(&wpos, y, x, 609, FALSE, FALSE, FALSE, 100, 0);
					place_monster_one(&wpos, y, x, 321, FALSE, FALSE, FALSE, 100, 0);//stone P
					summon_override_checks = SO_NONE;
					timer_pvparena3++; /* start releasing cycle */
				} else if (timer_pvparena3 == 2) { /* prepare second cycle */
					summon_override_checks = SO_ALL;
					for (i = 1; i <= 6; i++) {
						y = 1;
						x = (i * 10) - 3;
						//613 hellhound is too tough, 963 aranea im_pois, 986 3-h hydra, 249 vlasta
						//440 5-h hydra, 387 4-h hydra, 341 chimaera, 301 2-h hydra, 325 gold dfly
						//place_monster_aux(&wpos, y, x, 963, FALSE, FALSE, 100, 0);
						//place_monster_one(&wpos, y, x, i % 3 ? i % 2 ? 341 : 325 : 301, FALSE, FALSE, FALSE, 100, 0);
						place_monster_one(&wpos, y, x, i % 3 ? i % 2 ? 295 : 325 : 275, FALSE, FALSE, FALSE, 100, 0);//sphinx,gold dfly,tarantula
						everyone_lite_spot(&wpos, y, x);
					}
					summon_override_checks = SO_NONE;
					timer_pvparena3++; /* start releasing cycle */
				} else {
					/* clear previous monster */
					wipe_m_list_roaming(&wpos);
					if (timer_pvparena2 > 1) {
						/* erase monster that didn't come out of its chamber */
						y = 1;
						x = ((timer_pvparena2 - 1) * 10) - 3;
						if (zcave[y][x].m_idx > 0) delete_monster_idx(zcave[y][x].m_idx, TRUE);
						/* erase monster that stopped on its chamber door */
						y = 2;
						if (zcave[y][x].m_idx > 0) delete_monster_idx(zcave[y][x].m_idx, TRUE);
					}

					/* open current door to release monster */
					y = 2;
					x = (timer_pvparena2 * 10) - 3;
					zcave[y][x].feat = FEAT_UNSEALED_DOOR;
					everyone_lite_spot(&wpos, y, x);

					/* after 6th monster, take a break and switch wave cycle */
					if (timer_pvparena2 == 6) {
						/* prepare next cycle next time */
						timer_pvparena3++;
						/* max number of cycles? */
						timer_pvparena3 = timer_pvparena3 % 4;

						/* next time, begin at monster #1 again */
						timer_pvparena2 = 1;
					} else {
						/* next time, release next monster */
						timer_pvparena2++;
					}
				}
			}
		}
	}

#ifndef ARCADE_SERVER /* no stables in that town.. */
	/* With addition of horse stables in Bree, cycle horses */
	if (((turn % (cfg.fps * 600)) / cfg.fps) <= 37) { // '<=' limit check not really needed, it's not like we're wasting much cpu cycles :-s
		wpos.wx = 32;
		wpos.wy = 32;
		wpos.wz = 0;
		if ((zcave = getcave(&wpos))) {
			int k, l;
			static int bree_i, bree_j;
			static int bree_map[5];

			/* Erase all existing horses */
			switch ((turn % (cfg.fps * 600)) / cfg.fps) {
			case 0: delete_monster(&wpos, 25, 118, TRUE); break;
			case 6: delete_monster(&wpos, 25, 120, TRUE); break;
			case 14: delete_monster(&wpos, 25, 122, TRUE); break;
			case 16: delete_monster(&wpos, 25, 124, TRUE); break;
			//case 21: delete_monster(&wpos, 27, 118, TRUE); break;
			}

			/* Randomly place 0-5 new ones */
			switch ((turn % (cfg.fps * 600)) / cfg.fps) {
			case 0:
				for (i = 0; i < 5; i++) bree_map[i] = 0;
				bree_i = 0;
				bree_j = rand_int(3) + rand_int(4);
				break;
			case 23:
			case 26:
			case 31:
			case 33:
			case 37:
				/* "split up for-loop" */
				if (bree_i == bree_j) break;
				k = rand_int(5 - bree_i);
				k += bree_map[k];
				summon_override_checks = SO_ALL;
				switch (k) {
				case 0: place_monster_one(&wpos, 25, 118, 1136 + rand_int(5), FALSE, FALSE, FALSE, 0, 0); break;
				case 1: place_monster_one(&wpos, 25, 120, 1136 + rand_int(5), FALSE, FALSE, FALSE, 0, 0); break;
				case 2: place_monster_one(&wpos, 25, 122, 1136 + rand_int(5), FALSE, FALSE, FALSE, 0, 0); break;
				case 3: place_monster_one(&wpos, 25, 124, 1136 + rand_int(5), FALSE, FALSE, FALSE, 0, 0); break;
				//case 4: place_monster_one(&wpos, 27, 118, 1136 + rand_int(5), FALSE, FALSE, FALSE, 0, 0); break;
				}
				for (l = k; l < 5; l++) bree_map[l]++;
				summon_override_checks = SO_NONE;
				bree_i++;
				break;
			}
		}
	}
#endif

	if (!timer_falling_star) {
		timer_falling_star = 2 + rand_int(60);
		wpos.wx = WPOS_DF_X;
		wpos.wy = WPOS_DF_Y;
		wpos.wz = -WPOS_DF_Z * 2;
		if ((zcave = getcave(&wpos))) {
			x = rand_int(65 - 20) + 1 + 15;
			y = rand_int(7 - 1) + 1;
			i = 7 + rand_int(22);
			cast_falling_star(&wpos, x, y, i);
		}
	} else timer_falling_star--;

	if (fake_waitpid_geo) {
		FILE *fp;

		y = 0;

		/* Check if admin caller is still present */
		for (i = 1; i <= NumPlayers; i++) {
			p_ptr = Players[i];
			if (p_ptr->conn == NOT_CONNECTED) continue;
			if (p_ptr->id != fake_waitpid_geo) continue;
			break;
		}
		/* Found him */
		if (i <= NumPlayers) {
			char buf[MAX_CHARS], buf2[MSG_LEN];

			fp = fopen("__ipinfo.tmp", "r");
			if (fp) {
				buf2[0] = 0;
				i = p_ptr->Ind;
				/* Read result and display the relevant parts */
				x = 0;
				while (!my_fgets(fp, buf, 80, FALSE)) {
					y = 1;
					if (strstr(buf, "city\":") || strstr(buf, "region\":") || strstr(buf, "country\":")) {
						x = 1;
#if 0
						/* Dump relevant lines */
						msg_print(i, buf);
#else
						/* Forge one long line from all relevant lines */
						strcat(buf2, strstr(buf, strstr(buf, ":") + 1));
#endif
					}
				}
				fclose(fp);
				//remove("__ipinfo.tmp"); /* Keep maybe, if we want to review it manually afterwards. */
				if (!y) msg_print(i, "Geo response not yet ready, please wait...");
				else if (!x) msg_print(i, "No geo information available.");
#if 1 /* Dump one long line */
				else {
					if (buf2[strlen(buf2) - 1] == ',') buf2[strlen(buf2) - 1] = 0; /* Trim trailing comma */
					msg_print(i, buf2);
				}
#endif
			}
		} else y = 1; /* Abort */
		if (y) fake_waitpid_geo = 0;
	}
	if (fake_waitpid_ping) {
		FILE *fp;

		y = 0;

		/* Check if admin caller is still present */
		for (i = 1; i <= NumPlayers; i++) {
			p_ptr = Players[i];
			if (p_ptr->conn == NOT_CONNECTED) continue;
			if (p_ptr->id != fake_waitpid_ping) continue;
			break;
		}
		/* Found him */
		if (i <= NumPlayers) {
			char buf[MAX_CHARS], *s;

			fp = fopen("__ipping.tmp", "r");
			if (fp) {
				i = p_ptr->Ind;

				/* Read result and display the relevant parts */
				x = 0;
				while (!my_fgets(fp, buf, 80, FALSE)) {
					/* File exists */
					y = 1;

					/* Catch error: Invalid IP address */
					if (strstr(buf, "Name or service not known")) {
						msg_print(i, "Ping error: Name or service not known.");
						x = 1;
						break;
					}

					if ((s = strstr(buf, " time="))) {
						/* Response time exists */
						x = 1;

						msg_print(i, s);
						break;
					}
				}
				fclose(fp);
				//remove("__ipping.tmp"); /* Keep maybe, if we want to review it manually afterwards. */
				if (!y) msg_print(i, "Ping response not yet ready, please wait...");
				else if (!x) {
					msg_print(i, "No ping information available.");
#if 1 /* Initiate fallback procedure (again POSIX-only), requires traceroute and grep: */
					msg_print(i, "Falling back to traceroute check, please wait...");
					/* Could specify optionally: '-n' (changes format though) and/or '-m 30' (max hops): */
					i = system(format("( traceroute %s | grep -oE -e \" [-.0-9a-z]+ \" -e \"\\([^ ]+\\)\" -e \"[ .0-9]+ms\" > __iproute.tmp ; echo 'DONE' >> __iproute.tmp ) &", fake_waitxxx_ipaddr)); //grep -oP should also work
					fake_waitpid_route = fake_waitpid_ping; /* Transfer report process from ping lookup to route lookup */
#endif
				}
			}
		} else y = 1; /* Abort */
		if (y) fake_waitpid_ping = 0;
	}
	if (fake_waitpid_route) {
		FILE *fp;
		bool done = FALSE;

		/* Check if admin caller is still present */
		for (i = 1; i <= NumPlayers; i++) {
			p_ptr = Players[i];
			if (p_ptr->conn == NOT_CONNECTED) continue;
			if (p_ptr->id != fake_waitpid_route) continue;
			break;
		}
		/* Found him */
		if (i <= NumPlayers) {
			char buf[MAX_CHARS], *s;

			fp = fopen("__iproute.tmp", "r");
			if (fp) {
				int ping_max = -1, ping_cur;
				char host_max[MAX_CHARS], host_cur[MAX_CHARS];
				char addr_max[MAX_CHARS], addr_cur[MAX_CHARS];
				int header = 4;

				i = p_ptr->Ind;

				/* Read result and display the relevant parts.
				   File format is: header line, { name, (ip), ping ms }, DONE line. */
				while (!my_fgets(fp, buf, 80, FALSE)) {
					/* Skip the response's header line */
					if (header) {
						header--;
						continue;
					}

					/* File is complete? */
					if (strstr(buf, "DONE")) {
						done = TRUE;
						break;
					}
					/* Skip hop index numbers (they get grep'ed too, maybe just adjust grep accordingly instead?) */
					else if (strlen(buf) <= 4) continue;
					/* Got a ping time? */
					else if (strstr(buf, " ms")) {
						ping_cur = atoi(buf);
						if (ping_cur > ping_max) {
							ping_max = ping_cur;
							strcpy(host_max, host_cur);
							strcpy(addr_max, addr_cur);
						}
					}
					/* Got a host ip? */
					else if ((s = strstr(buf, "("))) strcpy(addr_cur, s); //keep parenthesis :)
					/* Got a host name */
					else strcpy(host_cur, buf + 1);
				}
				fclose(fp);
				//if (done) remove("__iproute.tmp"); /* Keep maybe, if we want to review it manually afterwards. */
				if (!done) ; //spammy! msg_print(i, "Route-ping response not yet ready, please wait...");
				else if (ping_max == -1) msg_print(i, "No route-ping information available.");
				else msg_format(i, " %s %s: %d ms", host_max, addr_max, ping_max);
			}
		} else done = TRUE; /* Abort */
		if (done) fake_waitpid_route = 0;
	}
	if (fake_waitpid_clver) {
		/* Just time out in case client never responds */
		fake_waitpid_clver_timer--;

		/* Discard for good */
		if (!fake_waitpid_clver_timer) {
			fake_waitpid_clver = 0;
			s_printf("clver request timed out.\n");
		}
	}
}

/* during new years eve, cast fireworks! - C. Blue
   NOTE: Called four times per second (if fireworks are enabled). */
static void process_firework_creation() {
	int i;

	if (!fireworks_delay) { /* fire! */
		worldpos wpos;

		switch (rand_int(4)) {
		case 0:	fireworks_delay = 0; break;
		case 1:	fireworks_delay = 1; break;
		case 2:	fireworks_delay = 4; break;
		case 3:	fireworks_delay = 8; break;
		}

		/* Fireworks in Bree */
		wpos.wx = 32; wpos.wy = 32; wpos.wz = 0;

		/* Launch multiple rockets ;) - mikaelh */
		for (i = 0; i < fireworks; i++) {
			/* "type" determines colour and explosion style */
			cast_fireworks(&wpos, rand_int(MAX_WID - 120) + 60, rand_int(MAX_HGT - 40) + 20, -1);
		}
	} else fireworks_delay--;
}


#if defined(CLIENT_SIDE_WEATHER) && defined(CLIENT_WEATHER_GLOBAL)
/* Update all affected (ie on worldmap surface) players' client-side weather.
   NOTE: Called on opportunity of _global_ weather undergoing any change. */
static void players_weather() {
	int i;

	for (i = 1; i <= NumPlayers; i++) {
		/* Skip non-connected players */
		if (Players[i]->conn == NOT_CONNECTED) continue;
		/* Skip players not on world surface - player_weather() actually checks this too though */
		if (Players[i]->wpos.wz) continue;

		/* send weather update to him */
		player_weather(i, FALSE, TRUE, FALSE);
	}
}
#endif

#if defined(CLIENT_WEATHER_GLOBAL) || !defined(CLIENT_SIDE_WEATHER)
/* manage and toggle weather and wind state - C. Blue
   NOTE: Called once per second,
         and for CLIENT_SIDE_WEATHER only if also CLIENT_WEATHER_GLOBAL. */
static void process_weather_control() {

 #ifdef CLIENT_SIDE_WEATHER
/* NOTE: we are only supposed to get here if also CLIENT_WEATHER_GLOBAL. */

	/* during winter, it snows: */
	if (season == SEASON_WINTER) {
		if (!weather) { /* not snowing? */
			if (weather_duration <= 0 && weather_frequency > 0) { /* change weather? */
				weather = 2; /* snowing now */
				players_weather();
				weather_duration = weather_frequency * 60 * 4;
			} else if (weather_duration > 0) {
				weather_duration--;
			}
		} else { /* it's currently snowing */
			if (weather_duration <= 0 && (4 - weather_frequency) > 0) { /* change weather? */
				weather = 0; /* stop snowing */
				players_weather();
				weather_duration = (4 - weather_frequency) * 60 * 4;
			} else if (weather_duration > 0) {
				weather_duration--;
			}
		}
	/* during spring, summer and autumn, it rains: */
	} else {
		if (weather) { /* it's currently raining */
			if (weather_duration <= 0 && weather_frequency > 0) { /* change weather? */
				weather = 0; /* stop raining */
				players_weather();
//s_printf("WEATHER: Stopping rain.\n");
				weather_duration = rand_int(60 * 30 / weather_frequency) + 60 * 30 / weather_frequency;
			} else weather_duration--;
		} else { /* not raining at the moment */
			if (weather_duration <= 0 && (4 - weather_frequency) > 0) { /* change weather? */
				weather = 1; /* start raining */
				players_weather();
//s_printf("WEATHER: Starting rain.\n");
				weather_duration = rand_int(60 * 9) + 60 * weather_frequency;
			} else if (weather_duration > 0) {
				weather_duration--;
			}
		}
	}

	/* manage wind */
	if (wind_gust > 0) {
		wind_gust--; /* gust from east */
		if (!wind_gust) players_weather();
	} else if (wind_gust < 0) {
		wind_gust++; /* gust from west */
		if (!wind_gust) players_weather();
	} else if (!wind_gust_delay) { /* gust of wind coming up? */
		wind_gust_delay = 15 + rand_int(240); /* so sometimes no gust at all during 4-min snow periods */
		wind_gust = rand_int(60) + 5;
		if (rand_int(2)) {
			wind_gust = -wind_gust;
		}
		players_weather();
	} else wind_gust_delay--;

 #else /* server-side weather: */

	/* during winter, it snows: */
	if (season == SEASON_WINTER) {
		if (!weather) { /* not snowing? */
			if (weather_duration <= 0 && weather_frequency > 0) { /* change weather? */
				weather = 1; /* snowing now */
				weather_duration = weather_frequency * 60 * 4;
			} else if (weather_duration > 0) {
				weather_duration--;
			}
		} else { /* it's currently snowing */
			if (weather_duration <= 0 && (4 - weather_frequency) > 0) { /* change weather? */
				weather = 0; /* stop snowing */
				weather_duration = (4 - weather_frequency) * 60 * 4;
			} else if (weather_duration > 0) {
				weather_duration--;
			}
		}
	/* during spring, summer and autumn, it rains: */
	} else {
		if (weather) { /* it's currently raining */
			if (weather_duration <= 0 && weather_frequency > 0) { /* change weather? */
				weather = 0; /* stop raining */
//s_printf("WEATHER: Stopping rain.\n");
				weather_duration = rand_int(60 * 30 / weather_frequency) + 60 * 30 / weather_frequency;
			} else weather_duration--;
		} else { /* not raining at the moment */
			if (weather_duration <= 0 && (4 - weather_frequency) > 0) { /* change weather? */
				weather = 1; /* start raining */
//s_printf("WEATHER: Starting rain.\n");
				weather_duration = rand_int(60 * 9) + 60 * weather_frequency;
			} else if (weather_duration > 0) {
				weather_duration--;
			}
		}
	}

	/* manage wind */
	if (wind_gust > 0) wind_gust--; /* gust from east */
	else if (wind_gust < 0) wind_gust++; /* gust from west */
	else if (!wind_gust_delay) { /* gust of wind coming up? */
		wind_gust_delay = 15 + rand_int(240); /* so sometimes no gust at all during 4-min snow periods */
		wind_gust = rand_int(60) + 5;
		if (rand_int(2)) wind_gust = -wind_gust;
	} else wind_gust_delay--;

 #endif

}
#endif

/* create actual weather effects.
   (animating/excising is then done in process_effects())
   NOTE: Called each turn. */
#ifndef CLIENT_SIDE_WEATHER
static void process_weather_effect_creation() {
	/* clear skies or new years eve fireworks? do nothing */
	if (!weather || fireworks) return;

	/* winter? then it snows: */
	if (season == SEASON_WINTER) {
		if (!(turn % ((cfg.fps + 29) / 30))) {
			worldpos wpos;

			/* Create snowflakes in Bree */
			wpos.wx = 32; wpos.wy = 32; wpos.wz = 0;
			cast_snowflake(&wpos, rand_int(MAX_WID - 2) + 1, 8);
		}
	/* spring, summer or autumn? it rains: */
	} else {
		if (!(turn % ((cfg.fps + 59) / 60))) {
			worldpos wpos;

			/* Create raindrops in Bree */
			wpos.wx = 32; wpos.wy = 32; wpos.wz = 0;
			cast_raindrop(&wpos, rand_int(MAX_WID - 2) + 1);
			cast_raindrop(&wpos, rand_int(MAX_WID - 2) + 1);
		}
	}
}
#endif

#ifdef CLIENT_SIDE_WEATHER
 #ifndef CLIENT_WEATHER_GLOBAL
/* initialize some weather-related variables */
static void wild_weather_init() {
	wilderness_type *w_ptr;
	int i, x, y;

	/* initialize amount of clouds */
	season_change(season, FALSE);

	for (x = 0; x < MAX_WILD_X; x++)
	for (y = 0; y < MAX_WILD_Y; y++) {
		w_ptr = &wild_info[y][x];

		/* initialize weather vars */

		/* initialize cloud vars */
		for (i = 0; i < 10; i++) {
			w_ptr->cloud_idx[i] = -1; /* be inactive by default */
			w_ptr->cloud_x1[i] = -9999; /* hack for client: disabled */
		}
	}
}

    /* DEBUGGING NOTES - C. Blue
       There might be a bug where weather_updated is not TRUE'd for
       the player's wilderness sector when using /mkcloud, which might also
       be a bug in normal cloud creation. The player will then need to leave
       and reenter his sector for the cloud to take effect.
       Further bug info: In cloud_erase() the same cloud of a found cloud_idx
       (here in /rmcloud) will not be found to be 'touching this sector'.
       FURTHER: Seems inconsistent use of cloud_x1/x2/y1/y2:
       cloud_create() uses them as start+destination, cloud_move/erase use
       them as box-defining points for finding the cloud's covering area. oO */


/* Manange and control weather separately for each worldmap sector,
   assumed that client-side weather is used, depending on clouds.
   NOTE: Called once per second. */
/* Note: Meanings of cloud_state (not implemented):
   1..100: growing (decrementing, until reaching 1)
   -1..-100: shrinking (incrementing, until reaching -1 or critically low radius
*/
static void process_wild_weather() {
	int i;

	/* process clouds forming, dissolving
	   and moving over world map */
	for (i = 0; i < MAX_CLOUDS; i++) {
		/* does cloud exist? */
		if (cloud_dur[i]) {
			/* hack: allow dur = -1 to last forever */
			if (cloud_dur[i] > 0) {
				/* hack to not have clouds near fireworks */
				if (season_newyearseve) {
					int txm = (cloud_x1[i] + (cloud_x2[i] - cloud_x1[i]) / 2) / MAX_WID; /* central wpos of this cloud */
					int tym = cloud_y1[i] / MAX_HGT;

					if (distance(tym, txm, cfg.town_y, cfg.town_x) <= 4) cloud_dur[i] = 1;
				}
				/* lose essence from raining/snowing */
				cloud_dur[i]--;
				/* if cloud dissolves, keep track */
				if (!cloud_dur[i]) {
//s_printf("erasing %d\n", i);//TEST_SERVER
					cloud_erase(i);
					clouds--;
					/* take a break for this second ;) */
					continue;
				}
			}
			/* Move it */
			cloud_move(i, FALSE);
		}

		/* create cloud? limit value depends on season */
		else if (clouds < max_clouds_seasonal) {
 #ifdef TEST_SERVER /* hack: fixed location for easier live testing? */
			//around Bree
			cloud_create(i, 32 * MAX_HGT, 32 * MAX_WID, FALSE);
			cloud_state[i] = 1;
 #else
			/* create cloud at random starting x, y world _grid_ coords (!) */
			cloud_create(i,
			    rand_int(MAX_WILD_X * MAX_WID),
			    rand_int(MAX_WILD_Y * MAX_HGT),
			    FALSE);
 #endif
		}
	}

	/* update players' local client-side weather if required */
	local_weather_update();
}

/* Change a cloud's movement.
   (Note: Continuous movement is performed by
   clients via buffered direction & duration.)
   New note: Negative values should probably work too (inverse direction). */
static void cloud_set_movement(int i) {
 #ifdef WEATHER_NO_CLOUD_MOVEMENT
{ /* hack: Try to fix the eternal rain bug:
     Disable cloud movement for now - C. Blue */
	cloud_xm100[i] = 0;
	cloud_ym100[i] = 0;
	cloud_mdur[i] = 30 + rand_int(300);
	return;
}
 #endif

 #ifdef TEST_SERVER /* hack: fixed location for easier live testing? */
  #if 0
	cloud_xm100[i] = 100;
	cloud_ym100[i] = 100;
	cloud_mdur[i] = 10;
  #endif
  #if 1
	cloud_xm100[i] = 0;
	cloud_ym100[i] = 0;
	cloud_mdur[i] = 10;
  #endif
 #else
	if (rand_int(2)) {
		/* no pseudo-wind */
		cloud_xm100[i] = 0;
		cloud_ym100[i] = 0;
	} else {
		/* pseudo-wind */
		cloud_xm100[i] = randint(100);
		cloud_ym100[i] = randint(100);
	}
	cloud_mdur[i] = 30 + rand_int(300); /* seconds */
 #endif
}

/* remove cloud status off all previously affected wilderness sectors,
   move & resize cloud,
   imprint cloud status on all currently affected wilderness sectors.
   Set each affected sector's 'weather_updated' if there was a change.
   Note: Wild sectors have a limit of amount of concurrent clouds
         they can be affected by (10).
   'resend_direction' explanation:
   If the direction of this cloud changed. the affected wilderness
   sectors will definitely need to be set to weather_updated, so
   the clients will by synchronized to the new cloud movement.
   NOTE: 'change' means that either cloud movement changes or that
         a wild sector toggles affected/unaffected state, simply. */
/* make rain fall down slower? */
 #if 1
  #define WEATHER_GEN_TICKS 3
  #define WEATHER_SNOW_MULT 3
 #else
/* make rain fall down faster? (recommended) */
  #define WEATHER_GEN_TICKS 2
  #define WEATHER_SNOW_MULT 4
 #endif
static void cloud_move(int i, bool newly_created) {
	bool resend_dir = FALSE, sector_changed;
	wilderness_type *w_ptr;
	int x, y, xs, ys, xd, yd, j;
	int txm, tym, tgx, tgy, d, dx, dy;
	int xs_add_prev = 0, ys_add_prev = 0, xd_add_prev = 0, yd_add_prev = 0;
	bool was_affected, is_affected, can_become_affected;
	int was_affected_idx = 0; /* slaying compiler warning */
	bool final_cloud_in_sector;


/* process cloud shaping & movement  --------------------------------------- */

	/* perform growing/shrinking (todo: implement) */
	if (cloud_state[i] > 1) {
		cloud_state[i]--;
		/* grow */
	} else if (cloud_state[i] < -1) {
		cloud_state[i]++;
		/* shrink */
	}

	/* process movement
	   Note: This is done both server- and client-side,
	         and synch'ed whenever movement changes or
	         a new worldmap sector becomes (un)affected
	         which is currently visited by tbe player to
	         synch with. */
	cloud_mdur[i]--;
	/* change in movement? */
	if (!cloud_mdur[i]) {
		cloud_set_movement(i);
		/* update all players' cloud direction information */
		resend_dir = TRUE;
	}

	/* perform movement */
	cloud_xfrac[i] += cloud_xm100[i];
	cloud_yfrac[i] += cloud_ym100[i];
	x = cloud_xfrac[i] / 100;
	y = cloud_yfrac[i] / 100;
	if (x) {
		cloud_x1[i] += x;
		cloud_x2[i] += x;
		cloud_xfrac[i] -= x * 100;
		/* we possibly left a sector behind by moving on: */
		if (x > 0) xs_add_prev = -1;
		else xd_add_prev = 1;
	}
	if (y) {
		cloud_y1[i] += y;
		cloud_y2[i] += y;
		cloud_yfrac[i] += y * 100;
		/* we possibly left a sector behind by moving on: */
		if (y > 0) ys_add_prev = -1;
		else yd_add_prev = 1;
	}


/* imprint new situation on wild sectors locally --------------------------- */

	/* check all worldmap sectors affected by this cloud
	   and modify local weather situation accordingly if needed */
	/* NOTE regarding hardcoding: These calcs depend on cloud creation algo a lot */
	txm = (cloud_x1[i] + (cloud_x2[i] - cloud_x1[i]) / 2) / MAX_WID; /* central wpos of this cloud */
	tym = cloud_y1[i] / MAX_HGT;
	xs = CLOUD_XS(cloud_x1[i], cloud_y1[i], cloud_x2[i], cloud_y2[i], cloud_dsum[i]);
	xd = CLOUD_XD(cloud_x1[i], cloud_y1[i], cloud_x2[i], cloud_y2[i], cloud_dsum[i]);
	ys = CLOUD_YS(cloud_x1[i], cloud_y1[i], cloud_x2[i], cloud_y2[i], cloud_dsum[i]);
	yd = CLOUD_YD(cloud_x1[i], cloud_y1[i], cloud_x2[i], cloud_y2[i], cloud_dsum[i]);
	/* traverse all wild sectors that could maybe be affected or
	   have been affected by this cloud, very roughly calculated
	   (just a rectangle mask), fine check is done inside. */
	for (x = xs + xs_add_prev; x <= xd + xd_add_prev; x++)
	for (y = ys + ys_add_prev; y <= yd + yd_add_prev; y++) {
		/* skip sectors out of array bounds */
		if (x < 0 || x >= MAX_WILD_X || y < 0 || y >= MAX_WILD_Y) continue;

		w_ptr = &wild_info[y][x];

		sector_changed = FALSE; /* assume no change */
		was_affected = FALSE;
		is_affected = FALSE;
		can_become_affected = FALSE; /* assume no free space left in local cloud array of this wild sector */
		final_cloud_in_sector = TRUE; /* assume this cloud is the only one keeping the weather up here */

		/* was the sector affected before moving? */
		for (j = 0; j < 10; j++) {
			/* cloud occurs in this sector? so sector was already affected */
			if (w_ptr->cloud_idx[j] == i) {
				was_affected = TRUE;
				was_affected_idx = j;
			}
			/* free space in cloud array? means we could add another cloud if we want */
			else if (w_ptr->cloud_idx[j] == -1) can_become_affected = TRUE;
			else if (w_ptr->cloud_idx[j] != -1) final_cloud_in_sector = FALSE;
		}
		if (!was_affected) final_cloud_in_sector = FALSE;

		/* if this sector wasn't affected before, and we don't
		   have free space left in its local cloud array,
		   then we won't be able to imprint on it anyway, so we
		   may as well ignore and leave this sector now. */
		if (!was_affected && !can_become_affected) continue;

 #ifdef TEST_SERVER
//SPAM(after a short while) s_printf("cloud-debug 1.\n");
 #endif

		/* NOTE: Basically same code in c-xtra1.c:do_weather(), dungeon.c:cloud_move(), wild.c:pos_in_weather() */

		/* is the sector affected now after moving? */
		/* calculate coordinates for deciding test case */
		/* NOTE regarding hardcoding: These calcs depend on cloud creation algo a lot */
		if (x < txm) tgx = (x + 1) * MAX_WID - 1; /* x: left edge of current sector */
		else if (x > txm) tgx = x * MAX_WID; /* x: right edge of current sector */
		else tgx = cloud_x1[i] + (cloud_x2[i] - cloud_x1[i]) / 2; /* x: center point of current cloud */
		if (y < tym) tgy = (y + 1) * MAX_HGT - 1; /* analogous to x above */
		else if (y > tym) tgy = y * MAX_HGT;
		else tgy = cloud_y1[i];
		/* test whether cloud touches the sector (taken from distance()) */
		/* Calculate distance to cloud focus point 1: */
		dy = (tgy > cloud_y1[i]) ? (tgy - cloud_y1[i]) : (cloud_y1[i] - tgy);
		dx = (tgx > cloud_x1[i]) ? (tgx - cloud_x1[i]) : (cloud_x1[i] - tgx);
		d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
		/* Calculate distance to cloud focus point 2: */
		dy = (tgy > cloud_y2[i]) ? (tgy - cloud_y2[i]) : (cloud_y2[i] - tgy);
		dx = (tgx > cloud_x2[i]) ? (tgx - cloud_x2[i]) : (cloud_x2[i] - tgx);
		/* ..and sum them up */
		d += (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));

		/* is sector affected now? add cloud to its local cloud array */
		if (d <= cloud_dsum[i]) {
			is_affected = TRUE;
 #ifdef TEST_SERVER
//s_printf("cloud-debug 2.\n");
 #endif

			/* update old cloud data or add a new entry if it didn't exist previously */
			for (j = 0; j < 10; j++) {
				/* unchanged cloud situation leads to continuing.. */
				if (was_affected && w_ptr->cloud_idx[j] != i) continue;
				if (!was_affected && w_ptr->cloud_idx[j] != -1) continue;
 #ifdef TEST_SERVER
//s_printf("cloud-debug 3.\n");
 #endif

				/* imprint cloud data to this wild sector's local cloud array */
				w_ptr->cloud_idx[j] = i;
				w_ptr->cloud_x1[j] = cloud_x1[i];
				w_ptr->cloud_y1[j] = cloud_y1[i];
				w_ptr->cloud_x2[j] = cloud_x2[i];
				w_ptr->cloud_y2[j] = cloud_y2[i];
				w_ptr->cloud_dsum[j] = cloud_dsum[i];
				w_ptr->cloud_xm100[j] = cloud_xm100[i];
				w_ptr->cloud_ym100[j] = cloud_ym100[i];
 #if 0
				/* meta data for Send_weather() */
				if (!w_ptr->cloud_updated[j]) {
					w_ptr->cloud_updated[j] = TRUE;
					w_ptr->clouds_to_update++;
				}
 #endif
				/* define weather situation accordingly */
				switch (w_ptr->type) {
				case WILD_ICE:
					w_ptr->weather_type = 2; /* always snow in icy waste */
					break;
				case WILD_DESERT:
					w_ptr->weather_type = 3; /* always sandstorm in desert */
					break;
				default:
					/* depends on season */
					w_ptr->weather_type = (season == SEASON_WINTER ? 2 : 1);
				}
 #ifdef TEST_SERVER
//s_printf("weather_type debug: wt=%d, x,y=%d,%d.\n", w_ptr->weather_type, x, y);
  #if 0
if (NumPlayers && Players[NumPlayers]->wpos.wx == x && Players[NumPlayers]->wpos.wy == y) {
    s_printf("weather_type debug: wt=%d, x,y=%d,%d.\n", w_ptr->weather_type, x, y);
    s_printf("cloud debug all: cidx=%d, x1,y1=%d,%d, dsum=%d.\n",
	w_ptr->cloud_idx[j], w_ptr->cloud_x1[j], w_ptr->cloud_y1[j], w_ptr->cloud_dsum[j]);
}
  #endif
 #endif

 #ifndef WEATHER_WINDS /* no winds */
				w_ptr->weather_wind = 0; /* todo: change =p (implement winds, actually -- currently we're
				                            using pseudo-winds by just setting random cloud movement - C. Blue */
				w_ptr->weather_wind_vertical = 0;
 #else /* winds */
  #ifndef WEATHER_NO_CLOUD_MOVEMENT /* Cloud movemenet is disabled as a workaround for the 'eternal rain' bug.. */
				/* note- pretty provisional implementation, since winds shouldnt depend
				   on cloud movement, but actually the other way round ;) This is merely
				   so we get to see some wind already, and these 'winds' would also conflict
				   if multiple clouds moving into different directions met.. - C. Blue */
				if (cloud_xm100[i] > 40) w_ptr->weather_wind = 5 - (2 * ((cloud_xm100[i] - 40) / 21));
				else if (cloud_xm100[i] < -40) w_ptr->weather_wind = 6 - (2 * ((-cloud_xm100[i] - 40) / 21));
				else w_ptr->weather_wind = 0;
				/* also determine vertical pseudo winds, just for server-side running speed calculations */
				if (cloud_ym100[i] > 40) w_ptr->weather_wind_vertical = 5 - (2 * ((cloud_ym100[i] - 40) / 21));
				else if (cloud_ym100[i] < -40) w_ptr->weather_wind_vertical = 6 - (2 * ((-cloud_ym100[i] - 40) / 21));
				else w_ptr->weather_wind_vertical = 0;
  #else /* ..so a little hack is needed to bring back 'windy' weather^^ - C. Blue */
				/* hack - randomly assume some winds, that stay sufficiently consistent */
				{
					/* this should allow smooth/consistent winds over slight changes in a player's wilderness position: */
					int seed = (y + x + ((int)((turn / (cfg.fps * 1200)) % 90))) / 3; /* hourly change */

					switch (seed % 30) {
					case 0: case 1: case 2: case 3:		w_ptr->weather_wind = 0; break;
					case 4:	case 5:	case 6:			w_ptr->weather_wind = 5; break;
					case 7:	case 8:				w_ptr->weather_wind = 3; break;
					case 9:					w_ptr->weather_wind = 1; break;
					case 10: case 11:			w_ptr->weather_wind = 3; break;
					case 12: case 13: case 14:		w_ptr->weather_wind = 5; break;
					case 15: case 16: case 17: case 18:	w_ptr->weather_wind = 0; break;
					case 19: case 20: case 21:		w_ptr->weather_wind = 6; break;
					case 22: case 23:			w_ptr->weather_wind = 4; break;
					case 24:				w_ptr->weather_wind = 2; break;
					case 25: case 26:			w_ptr->weather_wind = 4; break;
					case 27: case 28: case 29:		w_ptr->weather_wind = 6; break;
					}
					w_ptr->weather_wind_vertical = 0;
				}
  #endif
 #endif

				w_ptr->weather_intensity =
				    ((w_ptr->weather_type == 2 || w_ptr->weather_type == 3) && (!w_ptr->weather_wind || w_ptr->weather_wind >= 3)) ?
				    5 : 8;
				w_ptr->weather_speed =
 #if 1 /* correct! (this is a different principle than 'weather_intensity' above) */
				    (w_ptr->weather_type == 2 || w_ptr->weather_type == 3) ? WEATHER_SNOW_MULT * WEATHER_GEN_TICKS :
				    /* hack: for non-windy rainfall, accelerate raindrop falling speed by 1: */
				    (w_ptr->weather_wind ? 1 * WEATHER_GEN_TICKS : WEATHER_GEN_TICKS - 1);
 #else /* just for testing stuff */
				    ((w_ptr->weather_type == 2 || w_ptr->weather_type == 3) && (!w_ptr->weather_wind || w_ptr->weather_wind >= 3)) ?
				    WEATHER_SNOW_MULT * WEATHER_GEN_TICKS : 1 * WEATHER_GEN_TICKS;
 #endif
				break;
			}
		}
		/* sector is not affected. If it was previously affected,
		   delete cloud from its local array now. */
		else if (was_affected) {
 #ifdef TEST_SERVER
//s_printf("cloud-debug 4.\n");
 #endif
			/* erase cloud locally */
			w_ptr->cloud_idx[was_affected_idx] = -1;
			w_ptr->cloud_x1[was_affected_idx] = -9999; /* hack for client: client sees this as 'disabled' */
 #if 0
			/* meta data for Send_weather() */
			if (!w_ptr->cloud_updated[was_affected_idx]) {
				w_ptr->cloud_updated[was_affected_idx] = TRUE;
				w_ptr->clouds_to_update++;
			}
 #endif

			/* if this was the last cloud in this sector,
			   define (stop) weather situation accordingly */
			if (final_cloud_in_sector) {
 #ifdef TEST_SERVER
//s_printf("cloud-debug 5.\n");
 #endif
				w_ptr->weather_type = 0; /* make weather 'run out slowly' */
			}
		}

		/* so, did this sector actually 'change' after all? */
		if (was_affected != is_affected) sector_changed = TRUE;

		/* if the local situation did actually change,
		   mark as updated for re-sending it to all players on it */
		if (sector_changed || newly_created || resend_dir)
			w_ptr->weather_updated = TRUE;
	}
}

static void cloud_erase(int i) {
	wilderness_type *w_ptr;
	int x, y, xs, ys, xd, yd, j;
	bool was_affected;
	int was_affected_idx = 0; /* slaying compiler warning */
	bool final_cloud_in_sector;

/* imprint new situation on wild sectors locally --------------------------- */

	/* check all worldmap sectors affected by this cloud
	   and modify local weather situation accordingly if needed */
	/* NOTE regarding hardcoding: These calcs depend on cloud creation algo a lot */
	xs = CLOUD_XS(cloud_x1[i], cloud_y1[i], cloud_x2[i], cloud_y2[i], cloud_dsum[i]);
	xd = CLOUD_XD(cloud_x1[i], cloud_y1[i], cloud_x2[i], cloud_y2[i], cloud_dsum[i]);
	ys = CLOUD_YS(cloud_x1[i], cloud_y1[i], cloud_x2[i], cloud_y2[i], cloud_dsum[i]);
	yd = CLOUD_YD(cloud_x1[i], cloud_y1[i], cloud_x2[i], cloud_y2[i], cloud_dsum[i]);
	/* traverse all wild sectors that could maybe be affected or
	   have been affected by this cloud, very roughly calculated
	   (just a rectangle mask), fine check is done inside. */
	for (x = xs; x <= xd; x++)
	for (y = ys; y <= yd; y++) {
		/* skip sectors out of array bounds */
		if (x < 0 || x >= MAX_WILD_X || y < 0 || y >= MAX_WILD_Y) continue;

		w_ptr = &wild_info[y][x];
//if (Players[1] && Players[1]->wpos.wx == x && Players[1]->wpos.wy == y) s_printf("p1 here\n");

		was_affected = FALSE;
		final_cloud_in_sector = TRUE; /* assume this cloud is the only one keeping the weather up here */

		/* was the sector affected before erasing? */
		for (j = 0; j < 10; j++) {
			/* cloud occurs in this sector? so sector was already affected */
			if (w_ptr->cloud_idx[j] == i) {
				was_affected = TRUE;
				was_affected_idx = j;
			}
			else if (w_ptr->cloud_idx[j] != -1) final_cloud_in_sector = FALSE;
		}
		if (!was_affected) final_cloud_in_sector = FALSE;

		if (was_affected) {
			/* erase cloud locally */
			w_ptr->cloud_idx[was_affected_idx] = -1;
			w_ptr->cloud_x1[was_affected_idx] = -9999; /* hack for client: client sees this as 'disabled' */
 #if 0
			/* meta data for Send_weather() */
			if (!w_ptr->cloud_updated[was_affected_idx]) {
				w_ptr->cloud_updated[was_affected_idx] = TRUE;
				w_ptr->clouds_to_update++;
			}
 #endif

			/* if this was the last cloud in this sector,
			   define (stop) weather situation accordingly */
			if (final_cloud_in_sector) {
				w_ptr->weather_type = 0; /* make weather 'run out slowly' */
			}

			/* so, did this sector actually 'change' after all? */
			//sector_changed -> TRUE
			/* if the local situation did actually change,
			   mark as updated for re-sending it to all players on it */
			w_ptr->weather_updated = TRUE;
		}
	}
}

/* Create a new cloud with given index and worldmap _grid_ coords of its center
   This does not check whether there will be too many clouds!
   (forced is for administrative cloud-testing.) */
void cloud_create(int i, int cx, int cy, bool forced) {
	/* cloud dimensions: */
	int sx, sy, dx, dy, dsum;
	/* worldmap coords: */
	int x, y, xs, xd, ys, yd;

	/* decide on initial size (largest ie horizontal diameter) */
	dsum = MAX_WID + rand_int(MAX_WID * 5);
	sx = cx - dsum / 4;
	sy = cy;
	dx = cx + dsum / 4;
	dy = cy;

	xs = CLOUD_XS(sx, sy, dx, dy, dsum);
	xd = CLOUD_XD(sx, sy, dx, dy, dsum);
	ys = CLOUD_YS(sx, sy, dx, dy, dsum);
	yd = CLOUD_YD(sx, sy, dx, dy, dsum);

	/* hack: not many clouds over deserts */
	if (!forced)
	for (x = xs - 1; x <= xd + 1; x++) {
		for (y = ys - 1; y <= yd + 1; y++) {
			if (in_bounds_wild(y, x) &&
			    wild_info[y][x].type == WILD_DESERT) {
				if (rand_int(10)) return;

				/* leave loops */
				x = 9999;
				break;
			}
		}
	}

	/* we have one more cloud now */
	clouds++;
	/* create cloud by setting it's life time */
	cloud_dur[i] = 30 + rand_int(600); /* seconds */
	/* set horizontal diameter */
	cloud_dsum[i] = dsum;
	/* set location/destination */
	cloud_x1[i] = sx;
	cloud_y1[i] = sy;
	cloud_x2[i] = dx;
	cloud_y2[i] = dy;
	/* decide on growing/shrinking/constant cloud state */
	cloud_state[i] = rand_int(2) ? 1 : (rand_int(2) ? rand_int(100) : -rand_int(100));
	/* decide on initial movement */
	cloud_set_movement(i);
	/* add this cloud to its initial wild sector(s) */
	cloud_move(i, TRUE);
}


/* update players' local client-side weather if required.
   called each time by process_wild_weather, aka 1/s. */
void local_weather_update(void) {
	int i;

	/* HACK part 1: play random thunderclaps if player is receiving harsh weather.
	   Note: this is synched to all players in the same worldmap sector,
	   for consistency. :) */
	int thunderstorm, thunderclap = 999;
 #ifdef USE_SOUND_2010
	int vol = rand_int(86);
 #endif

	thunderstorm = (turn / (cfg.fps * 3600)) % 6; /* n out of every 6 world map sector clusters have thunderstorms going */
	if (!(turn % (cfg.fps * 10))) thunderclap = rand_int(5); /* every 10s there is a 1 in 5 chance of thunderclap (in a thunderstorm area) */

	/* update players' local client-side weather if required */
	for (i = 1; i <= NumPlayers; i++) {
		/* Skip non-connected players */
		if (Players[i]->conn == NOT_CONNECTED) continue;
		/* Skip players not on world surface - player_weather() actually checks this too though */
		if (Players[i]->wpos.wz) continue;

		/* HACK part 2: if harsh weather, play random world-sector-synched thunderclaps */
 #if 0 /* debug */
		if (thunderclap == 0) {
			s_printf("p %d - w %d, t %d\n",
			    ((Players[i]->wpos.wy + Players[i]->wpos.wx) / 5) % 6,
			    thunderstorm,
			    wild_info[Players[i]->wpos.wy][Players[i]->wpos.wx].weather_type);
		}
 #endif
		if (thunderclap == 0 &&
		    wild_info[Players[i]->wpos.wy][Players[i]->wpos.wx].weather_type == 1 && /* no blizzards for now, just rainstorms */
		    //wild_info[Players[i]->wpos.wy][Players[i]->wpos.wx].weather_wind &&
		    ((Players[i]->wpos.wy + Players[i]->wpos.wx) / 5) % 6 == thunderstorm) {
 #ifdef USE_SOUND_2010
			sound_vol(i, "thunder", NULL, SFX_TYPE_WEATHER, FALSE, 15 + (vol + Players[i]->wpos.wy + Players[i]->wpos.wx) % 86); //weather: screen flashing implied
 #endif
		}

		/* no change in local situation? nothing to do then */
		if (!wild_info[Players[i]->wpos.wy][Players[i]->wpos.wx].weather_updated) continue;
		/* update player's local weather */
 #ifdef TEST_SERVER /* DEBUG */
  #if 0
s_printf("updating weather for player %d.\n", i);
  #endif
 #endif
		player_weather(i, FALSE, TRUE, FALSE);
	}
	/* reclear 'weather_updated' flag after all players have been updated */
	for (i = 1; i <= NumPlayers; i++)
		wild_info[Players[i]->wpos.wy][Players[i]->wpos.wx].weather_updated = FALSE;
}
 #endif /* !CLIENT_WEATHER_GLOBAL */
#endif /* CLIENT_SIDE_WEATHER */

/* calculate slow-downs of running speed due to environmental circumstances / grid features - C. Blue
   Note: a.t.m. Terrain-related slowdowns take precedence over wind-related slowdowns, cancelling them. */
void eff_running_speed(int *real_speed, player_type *p_ptr, cave_type *c_ptr) {
	int impair = 100;

	/* Shadow school: Especially fast at world surface at night, for travelling QoL, negating any terrain-induced slowdowns even! */
	if (!p_ptr->wpos.wz && night_surface && (impair = 100 - get_skill_scale(p_ptr, SKILL_OSHADOW, 108))) {
		int boost = 100 + get_skill_scale(p_ptr, SKILL_OSHADOW, 60);

		if (impair < 25) impair = 25; /* up to 75% impairment reduction, reached at skill ~35 */
		if (boost > 130) boost = 130; /* up to 30% faster running speed, reached at skill 25 already */
		*real_speed = (*real_speed * boost) / 100;
	}

#if 1 /* NEW_RUNNING_FEAT */
	if (!is_admin(p_ptr) && !p_ptr->ghost && !p_ptr->tim_wraith) {
		/* are we in fact running-levitating? */
		//if ((f_info[c_ptr->feat].flags1 & (FF1_CAN_LEVITATE | FF1_CAN_RUN)) && p_ptr->levitate) {
		if ((f_info[c_ptr->feat].flags1 & (FF1_CAN_LEVITATE | FF1_CAN_RUN))) {
			/* Allow level 50 druids to run at full speed */
			if (!(p_ptr->pclass == CLASS_DRUID &&  p_ptr->lev >= 50)) {
				if (f_info[c_ptr->feat].flags1 & FF1_SLOW_LEVITATING_1) *real_speed = (*real_speed * 100) / (100 + impair); // -50% speed
				if (f_info[c_ptr->feat].flags1 & FF1_SLOW_LEVITATING_2) *real_speed = (*real_speed * 100) / (100 + (300 * impair) / 100); // -75% speed
			}
		}
	    /* or running-swimming? */
		else if (feat_is_water(c_ptr->feat) && p_ptr->can_swim) {
			/* Allow Aquatic players run/swim at full speed */
			if (!(r_info[p_ptr->body_monster].flags7 & RF7_AQUATIC)) {
				if (f_info[c_ptr->feat].flags1 & FF1_SLOW_SWIMMING_1) *real_speed = (*real_speed * 100) / (100 + impair); // -50% speed
				if (f_info[c_ptr->feat].flags1 & FF1_SLOW_SWIMMING_2) *real_speed = (*real_speed * 100) / (100 + (300 * impair) / 100); // -75% speed
			}
		}
		/* or just normally running? */
		else {
			if (f_info[c_ptr->feat].flags1 & FF1_SLOW_RUNNING_1) *real_speed = (*real_speed * 100) / (100 + impair); // -50% speed
			if (f_info[c_ptr->feat].flags1 & FF1_SLOW_RUNNING_2) *real_speed = (*real_speed * 100) / (100 + (300 * impair) / 100); // -75% speed
		}
	}
#endif

#if 1
	/* Hinder player movement in deep, moving streams of water/lava - and speed us up if we're moving with it! */
	if (!p_ptr->ghost && !p_ptr->levitate && !p_ptr->tim_wraith &&
	    *real_speed == cfg.running_speed) { /* Only if we're not already slowed down for some reason (by above terrain slowdown code) */
		switch (c_ptr->feat) {

		/* Deep streams have major impact */
		case FEAT_ANIM_DEEP_LAVA_EAST:
		case FEAT_ANIM_DEEP_WATER_EAST:
			if (p_ptr->find_current == 1 || p_ptr->find_current == 4 || p_ptr->find_current == 7)
				*real_speed = (*real_speed * 5) / 10;
			if (p_ptr->find_current == 3 || p_ptr->find_current == 6 || p_ptr->find_current == 9)
				*real_speed = (*real_speed * 13) / 10;
			break;
		case FEAT_ANIM_DEEP_LAVA_WEST:
		case FEAT_ANIM_DEEP_WATER_WEST:
			if (p_ptr->find_current == 3 || p_ptr->find_current == 6 || p_ptr->find_current == 9)
				*real_speed = (*real_speed * 5) / 10;
			if (p_ptr->find_current == 1 || p_ptr->find_current == 4 || p_ptr->find_current == 7)
				*real_speed = (*real_speed * 13) / 10;
			break;
		case FEAT_ANIM_DEEP_LAVA_NORTH:
		case FEAT_ANIM_DEEP_WATER_NORTH:
			if (p_ptr->find_current == 1 || p_ptr->find_current == 2 || p_ptr->find_current == 3)
				*real_speed = (*real_speed * 5) / 10;
			if (p_ptr->find_current == 7 || p_ptr->find_current == 8 || p_ptr->find_current == 9)
				*real_speed = (*real_speed * 13) / 10;
			break;
		case FEAT_ANIM_DEEP_LAVA_SOUTH:
		case FEAT_ANIM_DEEP_WATER_SOUTH:
			if (p_ptr->find_current == 7 || p_ptr->find_current == 8 || p_ptr->find_current == 9)
				*real_speed = (*real_speed * 5) / 10;
			if (p_ptr->find_current == 1 || p_ptr->find_current == 2 || p_ptr->find_current == 3)
				*real_speed = (*real_speed * 13) / 10;
			break;

		/* Shallow streams have minor impact */
		case FEAT_ANIM_SHAL_LAVA_EAST:
		case FEAT_ANIM_SHAL_WATER_EAST:
			if (p_ptr->find_current == 1 || p_ptr->find_current == 4 || p_ptr->find_current == 7)
				*real_speed = (*real_speed * 8) / 10;
			if (p_ptr->find_current == 3 || p_ptr->find_current == 6 || p_ptr->find_current == 9)
				*real_speed = (*real_speed * 11) / 10;
			break;
		case FEAT_ANIM_SHAL_LAVA_WEST:
		case FEAT_ANIM_SHAL_WATER_WEST:
			if (p_ptr->find_current == 3 || p_ptr->find_current == 6 || p_ptr->find_current == 9)
				*real_speed = (*real_speed * 8) / 10;
			if (p_ptr->find_current == 1 || p_ptr->find_current == 4 || p_ptr->find_current == 7)
				*real_speed = (*real_speed * 11) / 10;
			break;
		case FEAT_ANIM_SHAL_LAVA_NORTH:
		case FEAT_ANIM_SHAL_WATER_NORTH:
			if (p_ptr->find_current == 1 || p_ptr->find_current == 2 || p_ptr->find_current == 3)
				*real_speed = (*real_speed * 8) / 10;
			if (p_ptr->find_current == 7 || p_ptr->find_current == 8 || p_ptr->find_current == 9)
				*real_speed = (*real_speed * 11) / 10;
			break;
		case FEAT_ANIM_SHAL_LAVA_SOUTH:
		case FEAT_ANIM_SHAL_WATER_SOUTH:
			if (p_ptr->find_current == 7 || p_ptr->find_current == 8 || p_ptr->find_current == 9)
				*real_speed = (*real_speed * 8) / 10;
			if (p_ptr->find_current == 1 || p_ptr->find_current == 2 || p_ptr->find_current == 3)
				*real_speed = (*real_speed * 11) / 10;
			break;
		}
	}
#endif

#ifdef IRRITATING_WEATHER /* Hinder player movement if running against the wind speed */
 #if defined(CLIENT_SIDE_WEATHER) && !defined(CLIENT_WEATHER_GLOBAL)
	if (!p_ptr->grid_house && !p_ptr->wpos.wz) {
		int wind, real_speed_vertical;

		/* hack: just 'wind' (invisible/inaudible) without actual rain/snow/sand doesn't count, since it might confuse the players */
		if (!wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_type) return;

		real_speed_vertical = *real_speed;
		/* running against strong wind is slower :) - C. Blue */
		wind = wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_wind;
		if (!wind || *real_speed != cfg.running_speed) ;
			/* if no wind, or if we're already slowed down: nothing */
		else if (wind % 2) {
			/* west wind */
			if (p_ptr->find_current == 1 || p_ptr->find_current == 4 || p_ptr->find_current == 7)
				*real_speed = (*real_speed * (wind + 3)) / 10;
		} else {
			/* east wind */
			if (p_ptr->find_current == 3 || p_ptr->find_current == 6 || p_ptr->find_current == 9)
				*real_speed = (*real_speed * (wind + 6)) / 10;
		}
		/* also check vertical winds (which are only used for exactly this purpose here) */
		wind = wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].weather_wind_vertical;
		if (!wind || real_speed_vertical != cfg.running_speed) ;
			/* if no wind, or if we're already slowed down: nothing */
		else if (wind % 2) {
			/* west wind */
			if (p_ptr->find_current == 7 || p_ptr->find_current == 8 || p_ptr->find_current == 9)
				real_speed_vertical = (real_speed_vertical * (wind + 3)) / 10;
		} else {
			/* east wind */
			if (p_ptr->find_current == 1 || p_ptr->find_current == 2 || p_ptr->find_current == 3)
				real_speed_vertical = (real_speed_vertical * (wind + 6)) / 10;
		}
		if (real_speed_vertical < *real_speed) *real_speed = real_speed_vertical;
	}
 #endif
#endif
}

/* Initiates forced shutdown with all players getting recalled automatically.
   Added this for use within LUA files, for automatic seasonal event updating. - C. Blue */
void timed_shutdown(int k, bool terminate) {
	int i;

	//msg_admins(0, format("\377w* Shutting down in %d minutes *", k));
	if (terminate)
		msg_broadcast_format(0, "\374\377I*** \377RAutomatic recall and server maintenance shutdown in max %d minute%s. \377I***", k, (k == 1) ? "" : "s");
	else
		msg_broadcast_format(0, "\374\377I*** \377RAutomatic recall and server restart in max %d minute%s. \377I***", k, (k == 1) ? "" : "s");

	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (Players[i]->paging) continue;
		Players[i]->paging = 2;
	}

	cfg.runlevel = terminate ? 2042 : 2043;
	shutdown_recall_timer = k * 60;

	/* hack: suppress duplicate message */
	if (k <= 1) shutdown_recall_state = 3;
	else if (k <= 5) shutdown_recall_state = 2;
	else if (k <= 15) shutdown_recall_state = 1;
	else shutdown_recall_state = 0;

	s_printf("AUTOSHUTREC: %d minute(s).\n", k);
}

int recall_depth_idx(struct worldpos *wpos, player_type *p_ptr) {
	int j;

	/* cannot recall in 0,0? */
	if (!wpos->wx && !wpos->wy) return(-1);

	/* no dungeon/tower here? */
	if (wpos->wz > 0 && !wild_info[wpos->wy][wpos->wx].tower) return(-1);
	if (wpos->wz <= 0 && !wild_info[wpos->wy][wpos->wx].dungeon) return(-1);/* assume basic recall prefers dungeon over tower */

	for (j = 0; j < MAX_D_IDX * 2; j++) {
		/* it's a dungeon that's new to us - add it! */
		if (!p_ptr->max_depth_wx[j] && !p_ptr->max_depth_wy[j]) {
			p_ptr->max_depth_wx[j] = wpos->wx;
			p_ptr->max_depth_wy[j] = wpos->wy;
			p_ptr->max_depth_tower[j] = (wpos->wz > 0);
			return(j);
		}
		/* check dungeons we've already been to */
		if (p_ptr->max_depth_wx[j] == wpos->wx &&
		    p_ptr->max_depth_wy[j] == wpos->wy &&
		    p_ptr->max_depth_tower[j] == (wpos->wz > 0))
			return(j);
	}
	s_printf("p_ptr->max_depth[]: TOO MANY DUNGEONS ('%s',%d,%d,%d)!\n", p_ptr->name, wpos->wx, wpos->wy, wpos->wz);
	return(-1);
}
int get_recall_depth(struct worldpos *wpos, player_type *p_ptr) {
	int i = recall_depth_idx(wpos, p_ptr);
	if (i == -1) return(0);
	return(p_ptr->max_depth[i]);
}

/* Inject a delayed command into the command queue, used for handling !X inscription:
   It simulates the player executing an ID command after the item has been picked up. */
void handle_XID(int Ind) {
	sockbuf_t *connpq = get_conn_q(Ind);
	player_type *p_ptr = Players[Ind];

	if (!p_ptr->delayed_spell) return;
#ifdef XID_REPEAT
	if (p_ptr->command_rep_active) return;
	p_ptr->command_rep_active = TRUE;
#endif

	/* hack: inject the delayed ID-spell cast command */
	switch (p_ptr->delayed_spell) {
	case 0: break; /* nothing */
#ifdef ENABLE_XID_MDEV
	case -1: /* item activation */
		Packet_printf(connpq, "%c%hd", PKT_ACTIVATE, p_ptr->delayed_index);
 #ifdef XID_REPEAT
		p_ptr->command_rep = PKT_ACTIVATE;
		p_ptr->delayed_spell_temp = p_ptr->delayed_spell;
 #endif
		p_ptr->delayed_spell = 0;
		break;
	case -2: /* perception staff use */
		Packet_printf(connpq, "%c%hd", PKT_USE, p_ptr->delayed_index);
 #ifdef XID_REPEAT
		p_ptr->command_rep = PKT_USE;
		p_ptr->delayed_spell_temp = p_ptr->delayed_spell;
 #endif
		p_ptr->delayed_spell = 0;
		break;
	case -3: /* perception rod zap */
		Packet_printf(connpq, "%c%hd", PKT_ZAP, p_ptr->delayed_index);
 #ifdef XID_REPEAT
		p_ptr->command_rep = PKT_ZAP;
		p_ptr->delayed_spell_temp = p_ptr->delayed_spell;
 #endif
		p_ptr->delayed_spell = 0;
		break;
#endif
	case -4: /* ID / *ID* scroll read (Note: This is of course a one-time thing, won't get repeated - scrolls always succeed) */
		Packet_printf(connpq, "%c%hd", PKT_READ, p_ptr->delayed_index);
		p_ptr->delayed_spell = 0;
		break;
	default: /* spell */
#ifdef ENABLE_XID_SPELL
		Packet_printf(connpq, "%c%c%hd%hd%c%hd%hd", PKT_ACTIVATE_SKILL, MKEY_SCHOOL, p_ptr->delayed_index, p_ptr->delayed_spell - 1, -1, -1, 0);
 #ifdef XID_REPEAT
		p_ptr->command_rep = PKT_ACTIVATE_SKILL;
		p_ptr->delayed_spell_temp = p_ptr->delayed_spell;
 #endif
		p_ptr->delayed_spell = 0;
#endif
	}
}

/* Helper function to determine snowballs melting, blood potions going bad and other stuff */
bool cold_place(struct worldpos *wpos) {
	bool cold;
	dungeon_type *d_ptr = NULL;

	/* not melting while it's cold */
	if (season == SEASON_WINTER) cold = TRUE;
	else cold = FALSE;

	/* not melting snowballs in always-winter regions, but melting in never-winter regions */
	switch (wild_info[wpos->wy][wpos->wx].type) {
	case WILD_ICE:
		cold = TRUE;
		break;
	case WILD_VOLCANO:
	case WILD_DESERT:
		cold = FALSE;
		break;
	}

	/* don't melt in icy dungeons, but melt in other dungeons */
	if (wpos->wz > 0) d_ptr = wild_info[wpos->wy][wpos->wx].tower;
	else if (wpos->wz < 0) d_ptr = wild_info[wpos->wy][wpos->wx].dungeon;
	if (d_ptr) {
		u32b dflags1;

#ifdef IRONDEEPDIVE_MIXED_TYPES
		if (in_irondeepdive(wpos)) dflags1 = d_info[iddc[ABS(wpos->wz)].type].flags1;
		else
#endif
		/* custom 'wilderness' (type-0) dungeon, admin-added */
		if (d_ptr->theme) {
			/* start with original flags of this type-0 dungeon */
			dflags1 = d_ptr->flags1;
			/* add 'theme' flags that don't mess up our main flags too much */
			dflags1 |= (d_info[d_ptr->theme].flags1 & DF1_THEME_MASK);
		}
		/* normal dungeon from d_info.txt */
		else dflags1 = d_ptr->flags1;

		if (dflags1 & DF1_COLD_PLACE) cold = TRUE;
		//else if (dflags1 & DF1_HOT_PLACE) cold = FALSE;
		else cold = FALSE;
	}

	return(cold);
}
#if 0 /* currently unused, todo: use^^ */
/* Helper function to determine snowballs melting, blood potions going bad and other stuff */
bool hot_place(struct worldpos *wpos) {
	bool hot;
	dungeon_type *d_ptr = NULL;

	/* not melting while it's hot */
	if (season == SEASON_SUMMER) hot = TRUE;
	else hot = FALSE;

	/* not melting snowballs in always-winter regions, but melting in never-winter regions */
	switch (wild_info[wpos->wy][wpos->wx].type) {
	case WILD_ICE:
		hot = FALSE;
		break;
	case WILD_VOLCANO:
	case WILD_DESERT:
		hot = TRUE;
		break;
	}

	/* melt faster in hot dungeons */
	if (wpos->wz > 0) d_ptr = wild_info[wpos->wy][wpos->wx].tower;
	else if (wpos->wz < 0) d_ptr = wild_info[wpos->wy][wpos->wx].dungeon;
	if (d_ptr) {
		u32b dflags1;

#ifdef IRONDEEPDIVE_MIXED_TYPES
		if (in_irondeepdive(wpos)) dflags1 = d_info[iddc[ABS(wpos->wz)].type].flags1;
		else
#endif
		/* custom 'wilderness' (type-0) dungeon, admin-added */
		if (d_ptr->theme) {
			/* start with original flags of this type-0 dungeon */
			dflags1 = d_ptr->flags1;
			/* add 'theme' flags that don't mess up our main flags too much */
			dflags1 |= (d_info[d_ptr->theme].flags1 & DF1_THEME_MASK);
		}
		/* normal dungeon from d_info.txt */
		else dflags1 = d_ptr->flags1;

		if (dflags1 & DF1_HOT_PLACE) hot = TRUE;
		//else if (dflags1 & DF1_COLD_PLACE) hot = FALSE;
		else hot = FALSE;
	}

	return(hot);
}
#endif

/* Apply flags for jail dungeons aka escape tunnels */
void apply_jail_flags(u32b *f1, u32b *f2, u32b *f3) {
	/* Base flags for any jail dungeon */
	*f1 |= DF1_UNLISTED;
	*f3 = DF3_JAIL_DUNGEON | DF3_CYCLIC_STAIRS;
	/* Make it a bit special/interesting :o */
	*f3 |= DF3_DARK;
	*f2 |= DF2_NO_MAGIC_MAP;
	//*f3 |= DF3_NO_TELE | DF3_NO_SUMMON | DF3_LIMIT_ESP; //DF3_NO_ESP |

	/* Make it more escape-tunnelish */
	*f1 |= DF1_SMALLEST | DF1_FLAT | DF1_NO_DOORS;
	*f2 |= DF2_NO_RECALL_INTO | DF2_NO_ENTRY_PROB;
	//*f3 |= DF3_NOT_EMPTY | DF3_FEW_ROOMS | DF3_NO_VAULTS; //too small/few rooms, sometimes whole floor is 100% walled, with 1 staircase grid left, ie 0 rooms.
	*f3 |= DF3_NOT_EMPTY | DF3_NO_VAULTS | DF3_NO_SIMPLE_STORES;
	//*f3 |= DF3_NO_SUMMON; //make it cheesy?^^

	/* Actually just make it force-down instead of iron */
	*f2 &= ~DF2_IRON;
	*f1 |= DF1_FORCE_DOWN;
}
