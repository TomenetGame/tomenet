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

/* chance of townie respawning like other monsters, randint(chance)==0 means respawn */
 /* Default */
#define TOWNIE_RESPAWN_CHANCE	250
/* better for Halloween event */
#define HALLOWEEN_TOWNIE_RESPAWN_CHANCE	125

/* if defined, player ghost loses exp slowly. [10000]
 * see GHOST_XP_CASHBACK in xtra2.c also.
 */
#define GHOST_FADING	10000

/* How fast HP/MP regenerate when 'resting'. [3] */
#define RESTING_RATE	(cfg.resting_rate)

/* Chance of items damaged when drowning, in % [3] */
#define WATER_ITEM_DAMAGE_CHANCE	3

/* Maximum wilderness radius a player can travel with WoR [16] 
 * TODO: Add another method to allow wilderness travels */
#define RECALL_MAX_RANGE	24

/* duration of GoI when getting recalled.        [2] (Must be 0<=n<=4) */
#define RECALL_GOI_LENGTH        3


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

#define SHUTDOWN_IGNORE_IDDC(p_ptr) \
    (in_irondeepdive(&(p_ptr)->wpos) \
    && (p_ptr)->idle_char > 900) /* just after 15 minutes flat */
    //&& p_ptr->afk
    //&& p_ptr->idle_char > STARVE_KICK_TIMER) //reuse idle-starve-kick-timer for this


/*
 * Return a "feeling" (or NULL) about an item.  Method 1 (Heavy).
 */
cptr value_check_aux1(object_type *o_ptr) {
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Artifacts */
	if (artifact_p(o_ptr)) {
		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return "terrible";

		/* Normal */
		return "special";
	}

	/* Ego-Items */
	if (ego_item_p(o_ptr)) {
		/* Hack for Stormbringer, so it doesn't show as "worthless" */
		if (o_ptr->name2 == EGO_STORMBRINGER) return "terrible";

#if 0
		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return "worthless";
#else
		/* Cursed items */
		if (cursed_p(o_ptr)) return "cursed";

		/* Broken items */
		if (broken_p(o_ptr)) return "worthless";
#endif

		/* Normal */

		/* All exploding or ego-ammo is excellent.. */
		if (is_ammo(o_ptr->tval) && (o_ptr->pval || o_ptr->name2 || o_ptr->name2b)) {
			/* ..except for zero-money ego: Backbiting ammo! */
			if (o_ptr->name2 && !e_info[o_ptr->name2].cost) return "worthless";
			if (o_ptr->name2b && !e_info[o_ptr->name2b].cost) return "worthless";
			return "excellent";
		}

		if (!object_value(0, o_ptr)) return "worthless";
		if (object_value(0, o_ptr) < 4000) return "good";
		return "excellent";
	}

	/* Cursed items */
	if (cursed_p(o_ptr)) return "cursed";

	/* Broken items */
	if (broken_p(o_ptr)) return "broken";

	/* Valid "tval" codes */
	switch (o_ptr->tval) {
	case TV_DIGGING:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
	case TV_AXE:
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
	case TV_BOOMERANG:
		/* Good "armor" bonus */
		if ((o_ptr->to_a > k_ptr->to_a) &&
		    (o_ptr->to_a > 0)) return "good";
		/* Good "weapon" bonus */
		if ((o_ptr->to_h - k_ptr->to_h + o_ptr->to_d - k_ptr->to_d > 0) &&
		    (o_ptr->to_h > 0 || o_ptr->to_d > 0)) return "good";
		break;
	default:
		/* Good "armor" bonus */
		if (o_ptr->to_a > 0) return "good";
		/* Good "weapon" bonus */
		if (o_ptr->to_h + o_ptr->to_d > 0) return "good";
		break;
	}

	/* Default to "average" */
	return "average";
}

cptr value_check_aux1_magic(object_type *o_ptr) {
	object_kind *k_ptr = &k_info[o_ptr->k_idx];


	switch (o_ptr->tval) {
	/* Scrolls, Potions, Wands, Staves and Rods */
	case TV_SCROLL:
		/* hack for cheques */
		if (k_ptr->sval == SV_SCROLL_CHEQUE) return "good";
	case TV_POTION:
	case TV_POTION2:
	case TV_WAND:
	case TV_STAFF:
	case TV_ROD:
	case TV_ROD_MAIN:
		/* "Cursed" scrolls/potions have a cost of 0 */
		if (k_ptr->cost == 0) return "bad";//"terrible";

		/* Artifacts */
		if (artifact_p(o_ptr)) return "special";

		/* Scroll of Nothing, Apple Juice, etc. */
		if (k_ptr->cost < 3) return "worthless"; //"average" or "worthless"

		/*
		 * Identify, Phase Door, Cure Light Wounds, etc. are
		 * just average
		 */
		if (k_ptr->cost < 100) return "average";

		/* Enchant Armor, *Identify*, Restore Stat, etc. */
		if (k_ptr->cost < 4000) return "good";

		/* Acquirement, Deincarnation, Strength, Blood of Life, ... */
		if (k_ptr->cost >= 4000) return "excellent";

		break;
	/* Food */
	case TV_FOOD:
		/* "Cursed" food */
		if (k_ptr->cost == 0) return "bad";//"terrible";

		/* Artifacts */
		if (artifact_p(o_ptr)) return "special";

		/* Normal food (no magical properties) */
		if (k_ptr->cost <= 10) return "average";

		/* Everything else is good */
		if (k_ptr->cost > 10) return "good";

		break;
	}

	/* No feeling */
//	return "";
	return (NULL);
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 2 (Light).
 */
cptr value_check_aux2(object_type *o_ptr) {
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Cursed items (all of them) */
	if (cursed_p(o_ptr)) return "cursed";

	/* Broken items (all of them) */
	if (broken_p(o_ptr)) return "broken";

	/* Artifacts -- except cursed/broken ones */
	if (artifact_p(o_ptr)) return "good";

	/* Ego-Items -- except cursed/broken ones */
	if (!k_ptr->cost) return "broken";
	if (ego_item_p(o_ptr)) {
		if (!object_value(0, o_ptr)) return "worthless";
		return "good";
	}

	switch (o_ptr->tval) {
	case TV_DIGGING:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
	case TV_AXE:
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
	case TV_BOOMERANG:
		/* Good "armor" bonus */
		if (o_ptr->to_a > k_ptr->to_a) return "good";
		/* Good "weapon" bonus */
		if (o_ptr->to_h - k_ptr->to_h + o_ptr->to_d - k_ptr->to_d > 0) return "good";
		break;
	default:
		/* Good "armor" bonus */
		if (o_ptr->to_a > 0) return "good";
		/* Good "weapon" bonus */
		if (o_ptr->to_h + o_ptr->to_d > 0) return "good";
		break;
	}

	/* No feeling */
	return (NULL);
}

cptr value_check_aux2_magic(object_type *o_ptr) {
	object_kind *k_ptr = &k_info[o_ptr->k_idx];


	switch (o_ptr->tval) {
	/* Scrolls, Potions, Wands, Staves and Rods */
	case TV_SCROLL:
		/* hack for cheques */
		if (k_ptr->sval == SV_SCROLL_CHEQUE) return "good";
	case TV_POTION:
	case TV_POTION2:
	case TV_WAND:
	case TV_STAFF:
	case TV_ROD:
		/* "Cursed" scrolls/potions have a cost of 0 */
		if (k_ptr->cost == 0) return "bad";//"cursed";

		/* Artifacts */
		if (artifact_p(o_ptr)) return "good";

		/* Scroll of Nothing, Apple Juice, etc. */
		if (k_ptr->cost < 3) return "average";//or "worthless"

		/*
		 * Identify, Phase Door, Cure Light Wounds, etc. are
		 * just average
		 */
		if (k_ptr->cost < 100) return "average";

		/* Enchant Armor, *Identify*, Restore Stat, etc. */
		if (k_ptr->cost < 4000) return "good";

		/* Acquirement, Deincarnation, Strength, Blood of Life, ... */
		if (k_ptr->cost >= 4000) return "good";

		break;

	/* Food */
	case TV_FOOD:
		/* "Cursed" food */
		if (k_ptr->cost == 0) return "bad";//"cursed";

		/* Artifacts */
		if (artifact_p(o_ptr)) return "good";

		/* Normal food (no magical properties) */
		if (k_ptr->cost <= 10) return "average";

		/* Everything else is good */
		if (k_ptr->cost > 10) return "good";

		break;
	}

	/* No feeling */
//	return "";
	return (NULL);
}

/*
 * Sense the inventory
 */
static void sense_inventory(int Ind) {
	player_type *p_ptr = Players[Ind];

	int i, dur;

	bool heavy = FALSE, heavy_magic = FALSE, heavy_archery = FALSE;
	bool ok_combat = FALSE, ok_magic = FALSE, ok_archery = FALSE;
	bool ok_curse = FALSE, force_curse = FALSE;

	cptr feel;
	bool felt_heavy, fail;

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
	if (!rand_int(dur / (get_skill_scale(p_ptr, SKILL_COMBAT, 80) + 20) - 28)) ok_combat = TRUE;
	if (!rand_int(dur / (get_skill_scale(p_ptr, SKILL_ARCHERY, 80) + 20) - 28)) ok_archery = TRUE;
	if (!rand_int(dur / (get_skill_scale(p_ptr, SKILL_MAGIC, 80) + 20) - 28)) {
		ok_magic = TRUE;
		ok_curse = TRUE;
	}
	/* note: SKILL_PRAY is currently unused */
	if (!rand_int(dur / (get_skill_scale(p_ptr, SKILL_PRAY, 80) + 20) - 28)) ok_curse = TRUE;

	/* A powerful warrior can pseudo-id ranged weapons and ammo too,
	   even if (s)he's not good at archery in general */
	if (get_skill(p_ptr, SKILL_COMBAT) >= 31 && ok_combat) {
#if 1 /* (apply 33% chance, see below) - allow basic feelings */
		ok_archery = TRUE;
#else /* (apply 33% chance, see below) - allow more distinctive feelings */
		heavy_archery = TRUE;
#endif
	}

	/* A very powerful warrior can even distinguish magic items */
	if (get_skill(p_ptr, SKILL_COMBAT) >= 41 && ok_combat) {
#if 0 /* too much? */
		ok_magic = TRUE;
#endif
		ok_curse = TRUE;
	}

	/* extra class-specific boni */
	if (p_ptr->ptrait == TRAIT_ENLIGHTENED) force_curse = ok_curse = TRUE;
	else if (p_ptr->pclass == CLASS_PRIEST
 #ifdef ENABLE_CPRIEST
	    || p_ptr->pclass == CLASS_CPRIEST
 #endif
	    ) {
		//i = (p_ptr->lev < 35) ? (135 - (p_ptr->lev + 10) * 3) : 1;
		if (p_ptr->lev >= 35 || !rand_int(3000 / (p_ptr->lev * 2 + 30) - 29)) force_curse = ok_curse = TRUE;
	}

	/* nothing to feel? exit */
	if (!ok_combat && !ok_magic && !ok_archery && !ok_curse) return;

	heavy = (get_skill(p_ptr, SKILL_COMBAT) >= 11) ? TRUE : FALSE;
	heavy_magic = (get_skill(p_ptr, SKILL_MAGIC) >= 11) ? TRUE : FALSE;
	heavy_archery = (get_skill(p_ptr, SKILL_ARCHERY) >= 11) ? TRUE : FALSE;

	/*** Sense everything ***/

	/* Check everything */
	for (i = 0; i < INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];
		fail = FALSE;

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;
		/* It is fully known, no information needed */
		if (object_known_p(Ind, o_ptr)) continue;
		/* We know about it already, do not tell us again */
		if (o_ptr->ident & ID_SENSE_HEAVY) continue;
		else if (o_ptr->ident & ID_SENSE) fail = TRUE;
		/* Occasional failure on inventory items */
		if ((i < INVEN_WIELD) &&
		    (magik(80) || UNAWARENESS(p_ptr))) {
			/* usually fail, except if we're forced to insta-sense a curse */
			if (!force_curse || fail) continue;
			/* if we're forced to insta-sense a curse, do just that */
			ok_magic = ok_combat = ok_archery = FALSE;
		}

		feel = NULL;
		felt_heavy = FALSE;

		if (ok_curse && !fail && cursed_p(o_ptr)) feel = value_check_aux1(o_ptr);

		/* Valid "tval" codes */
		switch (o_ptr->tval) {
		case TV_DIGGING:
		case TV_BLUNT:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		case TV_AXE:
		case TV_TRAPKIT:
			if (fail && !heavy) continue; //finally fail
			if (ok_combat) {
				feel = (heavy ? value_check_aux1(o_ptr) :
						value_check_aux2(o_ptr));
				if (heavy) felt_heavy = TRUE;
			}
			break;
		case TV_MSTAFF:
			if (fail && !heavy_magic) continue; //finally fail
			if (ok_magic) {
				feel = (heavy_magic ? value_check_aux1(o_ptr) :
						value_check_aux2(o_ptr));
				if (heavy_magic) felt_heavy = TRUE;
			}
			break;
		case TV_SCROLL:
			/* hack for cheques: Don't try to pseudo-id them at all. */
			if (o_ptr->sval == SV_SCROLL_CHEQUE) continue;
		case TV_POTION:
		case TV_POTION2:
		case TV_WAND:
		case TV_STAFF:
		case TV_ROD:
		case TV_FOOD:
			if (fail && !heavy_magic) continue; //finally fail
			if (ok_magic && !object_aware_p(Ind, o_ptr)) {
				feel = (heavy_magic ? value_check_aux1_magic(o_ptr) :
				    value_check_aux2_magic(o_ptr));
				if (heavy_magic) felt_heavy = TRUE;
			}
			break;
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_BOOMERANG:
			if (fail && !heavy_archery) continue; //finally fail
			if (ok_archery || (ok_combat && magik(25))) {
				feel = (heavy_archery ? value_check_aux1(o_ptr) :
				    value_check_aux2(o_ptr));
				if (heavy_archery) felt_heavy = TRUE;
			}
			break;
		}

		/* Skip non-feelings */
		if (!feel) continue;

		/* Stop everything */
		if (p_ptr->disturb_minor) disturb(Ind, 0, 0);

		/* Get an object description */
		object_desc(Ind, o_name, o_ptr, FALSE, 0);

		/* Hack -- suppress messages */
		if (p_ptr->taciturn_messages) suppress_message = TRUE;

		/* Message (equipment) */
		if (i >= INVEN_WIELD) {
			msg_format(Ind, "You feel the %s (%c) you are %s %s %s...",
			    o_name, index_to_label(i), describe_use(Ind, i),
			    ((o_ptr->number == 1) ? "is" : "are"), feel);
		}
		/* Message (inventory) */
		else {
			msg_format(Ind, "You feel the %s (%c) in your pack %s %s...",
			    o_name, index_to_label(i),
			    ((o_ptr->number == 1) ? "is" : "are"), feel);
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
		switch(o_ptr->tval) {
		case TV_WAND:
		case TV_STAFF:
		case TV_ROD:
		case TV_ROD_MAIN:
		case TV_SCROLL:
		case TV_POTION:
		case TV_POTION2:
		case TV_FOOD:
			p_ptr->obj_felt[o_ptr->k_idx] = TRUE;
			if (felt_heavy) p_ptr->obj_felt_heavy[o_ptr->k_idx] = TRUE;
		}

#if 1
		/* new: remove previous pseudo-id tag completely, since we've bumped our p-id tier meanwhile */
		if (fail) {
			char note2[80], noteid[10];
			note_crop_pseudoid(note2, noteid, quark_str(o_ptr->note));
			if (!note2[0]) o_ptr->note = o_ptr->note_utag = 0;
			else o_ptr->note = quark_add(note2);
		}
#endif

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

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* in case the pseudo-id inscription triggers a forced auto-inscription! :)
		   Added to allow inscribing 'good' ammo !k or so (ethereal ammo) */
		handle_stuff(Ind);
		Send_apply_auto_insc(Ind, i);
	}
}

#if 0 /* the results might be too incorrect! for example 'average' can still mean enchantment, etc. */
/* for NEW_ID_SCREEN's pseudo-id handling: */

static int quality_check_aux1(object_type *o_ptr) {
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	if (artifact_p(o_ptr)) return 3;

	if (ego_item_p(o_ptr)) {
		if (o_ptr->name2 == EGO_STORMBRINGER) return 3;
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return 2;
		if (is_ammo(o_ptr->tval) && (o_ptr->pval || o_ptr->name2 || o_ptr->name2b)) return 2;
		if (object_value(0, o_ptr) < 4000) return 1;
		return 2;
	}

	if (cursed_p(o_ptr)) return -1;
	if (broken_p(o_ptr)) return -1;

	switch (o_ptr->tval) {
	case TV_DIGGING:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
	case TV_AXE:
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
	case TV_BOOMERANG:
		if ((o_ptr->ident & ID_SENSE_GOOD)) return 1;
		break;
	default:
		if ((o_ptr->ident & ID_SENSE_GOOD)) return 1;
	}

	return 0;
}

/*
 * Return a "feeling" (or NULL) about an item.  Method 2 (Light).
 */
static int quality_check_aux2(object_type *o_ptr) {
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	if (cursed_p(o_ptr)) -1;
	if (broken_p(o_ptr)) -1;
	if (artifact_p(o_ptr)) return 1;
	if (!k_ptr->cost) return -1;
	if (ego_item_p(o_ptr)) {
		return 1;
	}

	switch (o_ptr->tval) {
	case TV_DIGGING:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
	case TV_AXE:
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
	case TV_BOOMERANG:
		if ((o_ptr->ident & ID_SENSE_GOOD)) return 1;
		break;
	default:
		if ((o_ptr->ident & ID_SENSE_GOOD)) return 1;
	}

	return 0;
}

int pseudo_id_result(object_type *o_ptr) {
	int quality = 0;

	switch (o_ptr->tval) {
	case TV_DIGGING:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
	case TV_AXE:
	case TV_TRAPKIT:

	case TV_MSTAFF:

	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
	case TV_BOOMERANG:
		quality = ((o_ptr->ident & ID_SENSE_HEAVY) ? quality_check_aux1(o_ptr) :
		    quality_check_aux2(o_ptr));
		break;

	case TV_SCROLL:
	case TV_POTION:
	case TV_POTION2:
	case TV_WAND:
	case TV_STAFF:
	case TV_ROD:
	case TV_FOOD:
		return 0;
	}

	return quality;
}
#endif

/*
 * Regenerate hit points				-RAK-
 */
static void regenhp(int Ind, int percent) {
	player_type *p_ptr = Players[Ind];

	s32b        new_chp, new_chp_frac;
	int                   old_chp;
	int freeze_test_heal = p_ptr->test_heal;

	/* Save the old hitpoints */
	old_chp = p_ptr->chp;

	/* Extract the new hitpoints */
	new_chp = ((s32b)p_ptr->mhp) * percent + PY_REGEN_HPBASE;
	/* Apply the healing */
	hp_player_quiet(Ind, new_chp >> 16, TRUE);
	//p_ptr->chp += new_chp >> 16;   /* div 65536 */

	/* check for overflow -- this is probably unneccecary */
	if ((p_ptr->chp < 0) && (old_chp > 0)) p_ptr->chp = MAX_SHORT;

	/* handle fractional hit point healing */
	new_chp_frac = (new_chp & 0xFFFF) + p_ptr->chp_frac;	/* mod 65536 */
	if (new_chp_frac >= 0x10000L) {
		hp_player_quiet(Ind, 1, TRUE);
		p_ptr->chp_frac = new_chp_frac - 0x10000L;
	} else {
		p_ptr->chp_frac = new_chp_frac;
	}

	p_ptr->test_heal = freeze_test_heal;
}


/*
 * Regenerate mana points				-RAK-
 */
static void regenmana(int Ind, int percent) {
	player_type *p_ptr = Players[Ind];
	s32b        new_mana, new_mana_frac;
	int                   old_csp;

#ifdef MARTYR_NO_MANA
	if (p_ptr->martyr) return;
#endif

	old_csp = p_ptr->csp;
	new_mana = ((s32b)p_ptr->msp) * percent + PY_REGEN_MNBASE;

	p_ptr->csp += new_mana >> 16;	/* div 65536 */
	/* check for overflow */
	if ((p_ptr->csp < 0) && (old_csp > 0))
		p_ptr->csp = MAX_SHORT;

	new_mana_frac = (new_mana & 0xFFFF) + p_ptr->csp_frac;	/* mod 65536 */
	if (new_mana_frac >= 0x10000L) {
		p_ptr->csp_frac = new_mana_frac - 0x10000L;
		p_ptr->csp++;
	} else {
		p_ptr->csp_frac = new_mana_frac;
	}

	/* Must set frac to zero even if equal */
	if (p_ptr->csp >= p_ptr->msp) {
		p_ptr->csp = p_ptr->msp;
		p_ptr->csp_frac = 0;
	}

	/* Redraw mana */
	if (old_csp != p_ptr->csp) {
		/* Redraw */
		p_ptr->redraw |= (PR_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);
	}
}


#define pelpel
#ifdef pelpel

/* Wipeout the effects	- Jir - */
static void erase_effects(int effect) {
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
	e_ptr->sx = 0;
	e_ptr->sy = 0;
	e_ptr->cx = 0;
	e_ptr->cy = 0;
	e_ptr->rad = 0;

	if (!(zcave = getcave(wpos))) return;

	/* XXX +1 is needed.. */
	for (l = 0; l < tdi[rad + 0]; l++)
	{
		j = cy + tdy[l];
		i = cx + tdx[l];
		if (!in_bounds2(wpos, j, i)) continue;

		c_ptr = &zcave[j][i];

		if (c_ptr->effect == effect)
		{
			c_ptr->effect = 0;
			everyone_lite_spot(wpos, j, i);
		}
	}
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
	int who = PROJECTOR_EFFECT;
	player_type *p_ptr;

	/* Every 10 game turns */
	//if (turn % 10) return;

	/* Not in the small-scale wilderness map */
	//if (p_ptr->wild_mode) return;


	for (k = 0; k < MAX_EFFECTS; k++) {
		// int e = cave[j][i].effect;
		effect_type *e_ptr = &effects[k];

		/* Skip empty slots */
		if (e_ptr->time == 0) continue;

		wpos = &e_ptr->wpos;
		if (!(zcave = getcave(wpos))) {
			e_ptr->time = 0;
			/* TODO - excise it */
			continue;
		}

#ifdef ARCADE_SERVER
 #if 0
		if ((e_ptr->flags & EFF_CROSSHAIR_A) || (e_ptr->flags & EFF_CROSSHAIR_B) || (e_ptr->flags & EFF_CROSSHAIR_C)) {

			/* e_ptr->interval is player who controls it */
			if (e_ptr->interval >= NumPlayers) {
				p_ptr = Players[e_ptr->interval];
				if (k == p_ptr->e) {
					if (e_ptr->cy != p_ptr->arc_b || e_ptr->cx != p_ptr->arc_a) {
						if (!in_bounds2(wpos, e_ptr->cy, e_ptr->cx)) continue;
						c_ptr = &zcave[e_ptr->cy][e_ptr->cx];
						c_ptr->effect = 0;
						everyone_lite_spot(wpos, e_ptr->cy, e_ptr->cx);
						e_ptr->cy = p_ptr->arc_b;
						e_ptr->cx = p_ptr->arc_a;
						if (!in_bounds2(wpos, e_ptr->cy, e_ptr->cx)) continue;
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
				if (!in_bounds2(wpos, j, i)) continue;
				everyone_lite_spot(wpos, j, i);
			}
  #endif
 #endif
#endif

		/* check if it's time to process this effect now (depends on level_speed) */
		if ((turn % (e_ptr->interval * level_speed(wpos) / (level_speeds[0] * 5))) != 0) continue;

		/* Reduce duration */
		e_ptr->time--;

		/* It ends? */
		if (e_ptr->time <= 0) {
			erase_effects(k);
			continue;
		}

		/* effect belongs to a non-player? */
		if (e_ptr->who > 0) who = e_ptr->who;
		else { /* or to a player? */
			/* Make the effect friendly after logging out - mikaelh */
			who = PROJECTOR_PLAYER;

			/* XXX Hack -- is the trapper online? */
			for (i = 1; i <= NumPlayers; i++) {
				p_ptr = Players[i];

				/* Check if they are in here */
				if (e_ptr->who != 0 - p_ptr->id) continue;

				/* additionally check if player left level --
				   avoids panic save on killing a dungeon boss with a lasting effect while
				   already having been prematurely recalled to town meanwhile (insta-res!) - C. Blue */
				if (!inarea(&p_ptr->wpos, wpos)) break;

				/* it's fine */
				who = 0 - i;
				break;
			}
		}

		/* Storm ends if the cause is gone */
		if  (e_ptr->flags & EFF_STORM && (who == PROJECTOR_EFFECT || who == PROJECTOR_PLAYER)) {
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

		/* Handle spell effects */
		for (l = 0; l < tdi[e_ptr->rad]; l++) {
			j = e_ptr->cy + tdy[l];
			i = e_ptr->cx + tdx[l];
			if (!in_bounds2(wpos, j, i)) continue;

			c_ptr = &zcave[j][i];

			//if (c_ptr->effect != k) continue;
			if (c_ptr->effect != k) /* Nothing */;
			else {
				if (e_ptr->time) {
					int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE | PROJECT_JUMP | PROJECT_NODO | PROJECT_NODF;

					flg = mod_ball_spell_flags(e_ptr->type, flg);

					/* Apply damage */
					project(who, 0, wpos, j, i, e_ptr->dam, e_ptr->type, flg, "");

					/* Oh, destroyed? RIP */
					if (who < 0 && who != PROJECTOR_EFFECT && who != PROJECTOR_PLAYER &&
					    Players[0 - who]->conn == NOT_CONNECTED) {
						/* Make the effect friendly after death - mikaelh */
						who = PROJECTOR_PLAYER;
					}

#ifndef EXTENDED_TERM_COLOURS
 #ifdef ANIMATE_EFFECTS
  #ifndef FREQUENT_EFFECT_ANIMATION
					/* C. Blue - hack: animate effects inbetween
					   ie allow random changes in spell_color().
					   Note: animation speed depends on effect interval. */
					if (spell_color_animation(e_ptr->type))
					    everyone_lite_spot(wpos, j, i);
  #endif
 #endif
#endif
				} else {
					c_ptr->effect = 0;
					everyone_lite_spot(wpos, j, i);
				}

				/* Hack -- notice death */
				//	if (!alive || death) return;
				/* Storm ends if the cause is gone */
				if (e_ptr->flags & EFF_STORM &&
				    (who == PROJECTOR_PLAYER || Players[0 - who]->death)) {
					erase_effects(k);
					break;
				}
			}


#if 0
			if (((e_ptr->flags & EFF_WAVE) && !(e_ptr->flags & EFF_LAST)) || ((e_ptr->flags & EFF_STORM) && !(e_ptr->flags & EFF_LAST))) {
				if (distance(e_ptr->cy, e_ptr->cx, j, i) < e_ptr->rad - 2)
					c_ptr->effect = 0;
			}
#else	// 0

			if (!(e_ptr->flags & EFF_LAST)) {
				if ((e_ptr->flags & EFF_WAVE)) {
					if (distance(e_ptr->cy, e_ptr->cx, j, i) < e_ptr->rad - 2) {
						c_ptr->effect = 0;
						everyone_lite_spot(wpos, j, i);
					}
				} else if ((e_ptr->flags & EFF_STORM)) {
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
				}
			}
#endif	// 0

			/* Creates a "wave" effect*/
			if (e_ptr->flags & EFF_WAVE) {
				if (los(wpos, e_ptr->cy, e_ptr->cx, j, i) &&
				    (distance(e_ptr->cy, e_ptr->cx, j, i) == e_ptr->rad)) {
					c_ptr->effect = k;
					everyone_lite_spot(wpos, j, i);
				}
			}

			/* Generate fireworks effects */
			if (e_ptr->flags & (EFF_FIREWORKS1 | EFF_FIREWORKS2 | EFF_FIREWORKS3)) {
				int semi = (e_ptr->time + e_ptr->rad) / 2;
				/* until half-time (or half-radius) the fireworks rise into the air */
				if (e_ptr->rad < e_ptr->time) {
					if (i == e_ptr->cx && j == e_ptr->cy - e_ptr->rad) {
						c_ptr->effect = k;
						everyone_lite_spot(wpos, j, i);
					}
				} else { /* after that, they explode (w00t) */
					/* explosion is faster than flying upwards */
//doesn't work					e_ptr->interval = 2;

#ifdef USE_SOUND_2010
					if (e_ptr->rad == e_ptr->time) {
						if ((e_ptr->flags & EFF_FIREWORKS3))
							//sound_near_site(e_ptr->cy, e_ptr->cx, wpos, 0, "fireworks_big", "", SFX_TYPE_AMBIENT, FALSE);
							sound_floor_vol(wpos, "fireworks_big", "", SFX_TYPE_AMBIENT, randint(26) + 75);
						else if ((e_ptr->flags & EFF_FIREWORKS2))
							//sound_near_site(e_ptr->cy, e_ptr->cx, wpos, 0, "fireworks_norm", "", SFX_TYPE_AMBIENT, FALSE);
							sound_floor_vol(wpos, "fireworks_norm", "", SFX_TYPE_AMBIENT, randint(26) + 75);
						else
							//sound_near_site(e_ptr->cy, e_ptr->cx, wpos, 0, "fireworks_small", "", SFX_TYPE_AMBIENT, FALSE);
							sound_floor_vol(wpos, "fireworks_small", "", SFX_TYPE_AMBIENT, randint(26) + 75);
					}
#endif

#if 0
					if (e_ptr->flags & EFF_FIREWORKS1) { /* simple rocket (line) */
						if (i == e_ptr->cx && j == e_ptr->cy - e_ptr->rad) {
							c_ptr->effect = k;
							everyone_lite_spot(wpos, j, i);
						}
#endif
					if (e_ptr->flags & EFF_FIREWORKS1) { /* 3-star */
						if (((i == e_ptr->cx && j >= e_ptr->cy - e_ptr->rad) && /* up */
						    (i == e_ptr->cx && j <= e_ptr->cy + 1 - e_ptr->rad)) ||
						    ((i >= e_ptr->cx + semi - e_ptr->rad && j == e_ptr->cy - semi) && /* left */
						    (i <= e_ptr->cx + semi + 1 - e_ptr->rad && j == e_ptr->cy - semi)) ||
						    ((i <= e_ptr->cx - semi + e_ptr->rad && j == e_ptr->cy - semi) && /* right */
						    (i >= e_ptr->cx - semi - 1 + e_ptr->rad && j == e_ptr->cy - semi))) {
							c_ptr->effect = k;
							everyone_lite_spot(wpos, j, i);
						}
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
						    (i >= e_ptr->cx - semi - 1 + e_ptr->rad && j >= e_ptr->cy - 0 - 2 * semi + e_ptr->rad))) {
							c_ptr->effect = k;
							everyone_lite_spot(wpos, j, i);
						}
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
						    (i >= e_ptr->cx - semi - 1 + e_ptr->rad && j >= e_ptr->cy - 0 - 2 * semi + e_ptr->rad))) {
							c_ptr->effect = k;
							everyone_lite_spot(wpos, j, i);
						}
					}
				}
			}

			/* Generate lightning effects -- effect_xtra: -1\ 0| 1/ 2_ */
			if (e_ptr->flags & (EFF_LIGHTNING1 | EFF_LIGHTNING2 | EFF_LIGHTNING3)) {
				int mirrored = (e_ptr->dam == 0) ? -1 : 1;

				if ((e_ptr->flags & EFF_LIGHTNING1)) {
					int stage = e_ptr->rad;

					if (stage > 15) stage = 15; /* afterglow */

					switch (stage) {
					case 15:
						if (i == e_ptr->cx + mirrored * 14 && j == e_ptr->cy + 4) {///
							c_ptr->effect = k;
							c_ptr->effect_xtra = -mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 14:
						if (i == e_ptr->cx + mirrored * 13 && j == e_ptr->cy + 3) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
					case 13:
						if (i == e_ptr->cx + mirrored * 12 && j == e_ptr->cy + 3) {///
							c_ptr->effect = k;
							c_ptr->effect_xtra = -mirrored;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx + mirrored * 6 && j == e_ptr->cy + 5) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
					case 12:
						if (i == e_ptr->cx + mirrored * 11 && j == e_ptr->cy + 2) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx + mirrored * 5 && j == e_ptr->cy + 5) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
					case 11:
						if (i == e_ptr->cx + mirrored * 10 && j == e_ptr->cy + 2) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx + mirrored * 4 && j == e_ptr->cy + 5) {///
							c_ptr->effect = k;
							c_ptr->effect_xtra = -mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 10:
						if (i == e_ptr->cx + mirrored * 9 && j == e_ptr->cy + 2) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx + mirrored * 3 && j == e_ptr->cy + 4) {///
							c_ptr->effect = k;
							c_ptr->effect_xtra = -mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 9:
						if (i == e_ptr->cx + mirrored * 8 && j == e_ptr->cy + 2) {///
							c_ptr->effect = k;
							c_ptr->effect_xtra = -mirrored;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx + mirrored * 3 && j == e_ptr->cy + 3) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
					case 8:
						if (i == e_ptr->cx + mirrored * 7 && j == e_ptr->cy + 1) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx + mirrored * 4 && j == e_ptr->cy + 3) {//`
							c_ptr->effect = k;
							c_ptr->effect_xtra = mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 7:
						if (i == e_ptr->cx + mirrored * 6 && j == e_ptr->cy + 1) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx + mirrored * 5 && j == e_ptr->cy + 2) {//`
							c_ptr->effect = k;
							c_ptr->effect_xtra = mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 6:
						if (i == e_ptr->cx + mirrored * 5 && j == e_ptr->cy + 1) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
					case 5:
						if (i == e_ptr->cx + mirrored * 4 && j == e_ptr->cy + 1) {///
							c_ptr->effect = k;
							c_ptr->effect_xtra = -mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 4:
						if (i == e_ptr->cx + mirrored * 3 && j == e_ptr->cy) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
					case 3:
						if (i == e_ptr->cx + mirrored * 2 && j == e_ptr->cy) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
					case 2:
						if (i == e_ptr->cx + mirrored * 1 && j == e_ptr->cy) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
					case 1:
						if (i == e_ptr->cx && j == e_ptr->cy) {//_
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
					}
				} else if ((e_ptr->flags & EFF_LIGHTNING2)) {
					int stage = e_ptr->rad;

					if (stage > 8) stage = 8; /* afterglow */

					switch (stage) {
					case 8:
						if (i == e_ptr->cx + mirrored * 6 && j == e_ptr->cy + 5) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 7:
						if (i == e_ptr->cx + mirrored * 6 && j == e_ptr->cy + 4) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = -mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 6:
						if (i == e_ptr->cx + mirrored * 5 && j == e_ptr->cy + 3) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx - mirrored * (4+1) && j == e_ptr->cy + 3) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
					case 5:
						if (i == e_ptr->cx + mirrored * 4 && j == e_ptr->cy + 3) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = -mirrored;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx - mirrored * (3+1) && j == e_ptr->cy + 3) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
					case 4:
						if (i == e_ptr->cx + mirrored * 3 && j == e_ptr->cy + 2) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx - mirrored * (2+1) && j == e_ptr->cy + 3) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 3:
						if (i == e_ptr->cx + mirrored * 2 && j == e_ptr->cy + 2) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = -mirrored;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx - mirrored * 2 && j == e_ptr->cy + 2) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = 0;
							everyone_lite_spot(wpos, j, i);
						}
					case 2:
						if (i == e_ptr->cx + mirrored * 1 && j == e_ptr->cy + 1) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = -mirrored;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx - mirrored * 1 && j == e_ptr->cy + 1) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 1:
						if (i == e_ptr->cx && j == e_ptr->cy) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = 0;
							everyone_lite_spot(wpos, j, i);
						}
					}
				} else if ((e_ptr->flags & EFF_LIGHTNING3)) {
					int stage = e_ptr->rad;

					if (stage > 10) stage = 10; /* afterglow */

					switch (stage) {
					case 10:
						if (i == e_ptr->cx - mirrored * 8 && j == e_ptr->cy + 6) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = 0;
							everyone_lite_spot(wpos, j, i);
						}
					case 9:
						if (i == e_ptr->cx - mirrored * 7 && j == e_ptr->cy + 5) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 8:
						if (i == e_ptr->cx - mirrored * 6 && j == e_ptr->cy + 4) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 7:
						if (i == e_ptr->cx - mirrored * 5 && j == e_ptr->cy + 3) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = 0;
							everyone_lite_spot(wpos, j, i);
						}
					case 6:
						if (i == e_ptr->cx - mirrored * 5 && j == e_ptr->cy + 2) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = mirrored;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx - mirrored && j == e_ptr->cy + 4) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = 0;
							everyone_lite_spot(wpos, j, i);
						}
					case 5:
						if (i == e_ptr->cx - mirrored * 4 && j == e_ptr->cy + 1) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx - mirrored * 2 && j == e_ptr->cy + 3) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = -mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 4:
						if (i == e_ptr->cx - mirrored * 3 && j == e_ptr->cy + 1) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
						if (i == e_ptr->cx - mirrored * 3 && j == e_ptr->cy + 2) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = 0;
							everyone_lite_spot(wpos, j, i);
						}
					case 3:
						if (i == e_ptr->cx - mirrored * 2 && j == e_ptr->cy + 1) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					case 2:
						if (i == e_ptr->cx - mirrored * 1 && j == e_ptr->cy) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = 2;
							everyone_lite_spot(wpos, j, i);
						}
					case 1:
						if (i == e_ptr->cx && j == e_ptr->cy) {
							c_ptr->effect = k;
							c_ptr->effect_xtra = mirrored;
							everyone_lite_spot(wpos, j, i);
						}
					}
				}
			}
		}

		if (e_ptr->flags & EFF_WAVE) e_ptr->rad++;
		/* Creates a "storm" effect*/
		else if (e_ptr->flags & EFF_STORM && who > PROJECTOR_EFFECT) {
			p_ptr = Players[0 - who];

			e_ptr->cy = p_ptr->py;
			e_ptr->cx = p_ptr->px;

			for (l = 0; l < tdi[e_ptr->rad]; l++) {
				j = e_ptr->cy + tdy[l];
				i = e_ptr->cx + tdx[l];
				if (!in_bounds2(wpos, j, i)) continue;

				c_ptr = &zcave[j][i];

				if (los(wpos, e_ptr->cy, e_ptr->cx, j, i) &&
						(distance(e_ptr->cy, e_ptr->cx, j, i) <= e_ptr->rad))
				{
					c_ptr->effect = k;
					everyone_lite_spot(wpos, j, i);
				}
			}
		}
		/* snowflakes */
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
			c_ptr->effect = k;
			everyone_lite_spot(wpos, e_ptr->cy, e_ptr->cx);
		}
		/* raindrops */
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
			c_ptr->effect = k;
			everyone_lite_spot(wpos, e_ptr->cy, e_ptr->cx);
		}

		/* fireworks */
		else if (e_ptr->flags & (EFF_FIREWORKS1 | EFF_FIREWORKS2 | EFF_FIREWORKS3)) {
			e_ptr->rad++; /* while radius < time/2 -> "rise into the air", otherwise "explode" */
		}

		/* lightning */
		else if ((e_ptr->flags & (EFF_LIGHTNING1 | EFF_LIGHTNING2 | EFF_LIGHTNING3))
		    && e_ptr->rad < 15) {
			e_ptr->rad++;
		}

		/* thunderstorm visual */
		else if (e_ptr->flags & EFF_THUNDER_VISUAL) {
			c_ptr = &zcave[e_ptr->cy][e_ptr->cx];
			c_ptr->effect = k;
			everyone_lite_spot(wpos, e_ptr->cy, e_ptr->cx);
		}

	}



	/* Apply sustained effect in the player grid, if any */
//	apply_effect(py, px);
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

			/* Hack -- Some monsters regenerate quickly */
			if (r_ptr->flags2 & RF2_REGENERATE) frac *= 2;

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
	int x, y;
	struct dun_level *l_ptr = getfloor(&p_ptr->wpos);

	if (p_ptr->wpos.wz) return FALSE;
	if (in_sector00(&p_ptr->wpos))
		return FALSE;
	if (l_ptr && (l_ptr->flags2 & LF2_INDOORS)) return FALSE;

	//if (p_ptr->tim_watchlist) p_ptr->tim_watchlist--;
	if (p_ptr->prace == RACE_VAMPIRE ||
	    (p_ptr->body_monster && r_info[p_ptr->body_monster].d_char == 'V'))
		calc_boni(Ind); /* daylight */

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

#ifdef USE_SOUND_2010
	if (p_ptr->is_day) return FALSE;
	p_ptr->is_day = TRUE;
	handle_music(Ind);
	{
		cave_type **zcave;
		if (!(zcave = getcave(&p_ptr->wpos))) {
			s_printf("DEBUG_DAY: Ind %d, wpos %d,%d,%d\n", Ind, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			return FALSE; /* paranoia */
		}
		handle_ambient_sfx(Ind, &zcave[p_ptr->py][p_ptr->px], &p_ptr->wpos, TRUE);
	}
#endif

	return TRUE;
}
/* update a particular player's view to night, assuming he's on world surface */
bool player_night(int Ind) {
	player_type *p_ptr = Players[Ind];
	cave_type **zcave = getcave(&p_ptr->wpos);
	int x, y;
	struct dun_level *l_ptr = getfloor(&p_ptr->wpos);

	if (!zcave) return FALSE; /* paranoia */

	if (p_ptr->wpos.wz) return FALSE;
	if (in_sector00(&p_ptr->wpos))
		return FALSE;
	if (l_ptr && (l_ptr->flags2 & LF2_INDOORS)) return FALSE;

	//if (p_ptr->tim_watchlist) p_ptr->tim_watchlist--;
	if (p_ptr->prace == RACE_VAMPIRE ||
	    (p_ptr->body_monster && r_info[p_ptr->body_monster].d_char == 'V'))
		calc_boni(Ind); /* no more daylight */

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

#ifdef USE_SOUND_2010
	if (!p_ptr->is_day) return FALSE;
	p_ptr->is_day = FALSE;
	handle_music(Ind);
 #if 0 /*done above already*/
	{
		cave_type **zcave;
		if (!(zcave = getcave(&p_ptr->wpos))) {
			s_printf("DEBUG_NIGHT: Ind %d, wpos %d,%d,%d\n", Ind, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			return FALSE; /* paranoia */
		}
		handle_ambient_sfx(Ind, &zcave[p_ptr->py][p_ptr->px], &p_ptr->wpos, TRUE);
	}
 #else
	handle_ambient_sfx(Ind, &zcave[p_ptr->py][p_ptr->px], &p_ptr->wpos, TRUE);
 #endif
#endif

	return TRUE;
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

	if (sector00separation && 0 == WPOS_SECTOR00_Z &&
	    wpos->wx == WPOS_SECTOR00_X && wpos->wy == WPOS_SECTOR00_Y) return;
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

	if (sector00separation && 0 == WPOS_SECTOR00_Z &&
	    wpos->wx == WPOS_SECTOR00_X && wpos->wy == WPOS_SECTOR00_Y) return;
	if (l_ptr && (l_ptr->flags2 & LF2_INDOORS)) return;

	/* Hack -- Scan the level */
	for (y = 0; y < MAX_HGT; y++)
	for (x = 0; x < MAX_WID; x++) {
		/* Get the cave grid */
		c_ptr = &zcave[y][x];

		/* darken all */
		if (!(f_info[c_ptr->feat].flags1 & FF1_PROTECTED) &&
		    !(c_ptr->info & CAVE_ROOM)) { /* keep houses' contents lit */
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
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (player_day(i)) msg_print(i, "The sun has risen.");
	}
}

/* Night starts */
static void night_falls() {
	struct worldpos wrpos;
	int wx, wy, i;

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
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (player_night(i)) msg_print(i, "The sun has fallen.");
	}

	/* If it's new year's eve, start the fireworks! */
	if (season_newyearseve) fireworks = 1;
}

/* take care of day/night changes, on world surface.
   NOTE: assumes that it gets called every HOUR turns only! */
static void process_day_and_night() {
	bool sunrise, nightfall;

	/* Check for sunrise or nightfall */
	sunrise = (((turn / HOUR) % 24) == SUNRISE) && IS_DAY; /* IS_DAY checks for events, that's why it's here. - C. Blue */
	nightfall = (((turn / HOUR) % 24) == NIGHTFALL) && IS_NIGHT; /* IS_NIGHT is pointless at the time of coding this, just for consistencies sake with IS_DAY above. */

	/* Day breaks - not during Halloween {>_>} or during NEW_YEARS_EVE (fireworks)! -- covered by IS_DAY now. */
	if (sunrise)
		sun_rises();
	/* Night falls - but only if it was actually day so far:
	   During HALLOWEEN as well as NEW_YEARS_EVE it stays night all the time >:) (see above) */
	else if (nightfall && !night_surface)
		night_falls();
}

/* Called when the server starts up */
static void init_day_and_night() {
	if (IS_DAY)
		sun_rises();
	else /* assume IS_NIGHT ;) */
		night_falls();
}

/*
 * Handle certain things once every 50 game turns
 */

static void process_world_player(int Ind) {
	player_type *p_ptr = Players[Ind];
	int	i;
//	int	regen_amount, NumPlayers_old = NumPlayers;


	/*** Process the monsters ***/
	/* Note : since monsters are added at a constant rate in real time,
	 * this corresponds in game time to them appearing at faster rates
	 * deeper in the dungeon.
	 */

	/* Check for creature generation */
#if 0 /* too many people idling all day.. ;) */
	if ((!istown(&p_ptr->wpos) && (rand_int(MAX_M_ALLOC_CHANCE) == 0)) ||
	    (!season_halloween && (istown(&p_ptr->wpos) && (rand_int(TOWNIE_RESPAWN_CHANCE * ((3 / NumPlayers) + 1)) == 0))) ||
	    (season_halloween && (istown(&p_ptr->wpos) && (rand_int((in_bree(&p_ptr->wpos) ?
		HALLOWEEN_TOWNIE_RESPAWN_CHANCE : TOWNIE_RESPAWN_CHANCE) * ((3 / NumPlayers) + 1)) == 0))))
#else /* ..so no longer depending on amount of players in town: */
	if (((!istown(&p_ptr->wpos) && (rand_int(MAX_M_ALLOC_CHANCE) == 0)) ||
	    (!season_halloween && istown(&p_ptr->wpos) && (rand_int(TOWNIE_RESPAWN_CHANCE) == 0)) ||
	    (season_halloween && istown(&p_ptr->wpos) &&
	    (rand_int(in_bree(&p_ptr->wpos) ?
	    HALLOWEEN_TOWNIE_RESPAWN_CHANCE : TOWNIE_RESPAWN_CHANCE) == 0)))
	    /* avoid admins spawning stuff */
	    && !(p_ptr->admin_dm || p_ptr->admin_wiz))
#endif
	{
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
	if (!(turn % 3000) && (p_ptr->black_breath)) {
		msg_print(Ind, "\377WThe Black Breath saps your soul!");

		/* alert to the neighbors also */
		msg_format_near(Ind, "\377WA dark aura seems to surround %s!", p_ptr->name);

		disturb(Ind, 0, 0);
	}

	/* Cold Turkey  */
	i = (p_ptr->csane << 7) / p_ptr->msane;
	if (!(turn % 4200) && !magik(i)) {
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
//	    !(turn % GHOST_FADING))
//	    !(turn % ((5100L - p_ptr->lev * 50)*GHOST_FADING)))
	    !(turn % (GHOST_FADING / p_ptr->lev * 50)))
//		(rand_int(10000) < p_ptr->lev * p_ptr->lev))
		take_xp_hit(Ind, 1 + p_ptr->lev / 5 + p_ptr->max_exp / 10000L, "fading", TRUE, TRUE, FALSE);
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
	for (k = 0; k < 3; k++) {
		for (i = 0; i < 32; i++) {
			/* Look for "okay" spells */
			if (p_ptr->innate_spells[k] & (1U << i)) {
				if (num == choice) return (k * 32 + i);
				num++;
			}
		}
	}

	return (0);
}

/*
 * Handle items for auto-retaliation  - Jir -
 * use_old_target is *strongly* recommended to actually make use of it.
 * If fallback is TRUE the melee weapon will be used if the intended means failed. - C. Blue
 */
static bool retaliate_item(int Ind, int item, cptr inscription, bool fallback) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;;
	int cost, choice = 0, spell = 0;

	if (item < 0) return FALSE;
	o_ptr = &p_ptr->inventory[item];
	if (!o_ptr->k_idx) return FALSE;

	/* 'Do nothing' inscription */
	if (inscription != NULL && *inscription == 'x') return TRUE;

	/* Is it variant @Ot for town-only auto-retaliation? - C. Blue */
	if (*inscription == 't') {
		if (!istownarea(&p_ptr->wpos, MAX_TOWNAREA)) return FALSE;
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
					return FALSE;
#else
					/* hack: prevent 'polymorph into...' power */
					if (choice == 2) do_cmd_mimic(Ind, 1, 5);
					else do_cmd_mimic(Ind, choice, 5);
					return TRUE;
#endif
				} else {
					int power = retaliate_mimic_power(Ind, choice - 1); /* immunity preference */
					bool dir = FALSE;
					if (innate_powers[power].smana > p_ptr->csp && fallback) return (p_ptr->fail_no_melee);
#if 0
					if (power) {
						/* undirected power? */
						switch (power) {
						case 0:
						case 64: case 66:
						case 68: case 69:
						case 76:
							do_cmd_mimic(Ind, power + 3, 0);
							return TRUE;
						}
						/* power requires direction? */
						do_cmd_mimic(Ind, power + 3, 5);
						return TRUE;
					}
#else
					switch (power / 32) {
					case 0: dir = monster_spells4[power].uses_dir;
						break;
					case 1: dir = monster_spells5[power - 32].uses_dir;
						break;
					case 2: dir = monster_spells6[power - 64].uses_dir;
						break;
					}
					do_cmd_mimic(Ind, power + 3, dir ? 5 : 0);
					return TRUE;
#endif
				}
			}
		}
		return FALSE;
	}
#endif

#ifndef AUTO_RET_NEW
	/* Only fighter classes can use various items for this */
	if (is_fighter(p_ptr)) {
#if 0
		/* item with {@O-} is used only when in danger */
		if (*inscription == '-' && p_ptr->chp > p_ptr->mhp / 2) return FALSE;
#endif

		switch (o_ptr->tval) {
		/* non-directional ones */
		case TV_SCROLL:
			do_cmd_read_scroll(Ind, item);
			return TRUE;
		case TV_POTION:
			do_cmd_quaff_potion(Ind, item);
			return TRUE;
		case TV_STAFF:
			do_cmd_use_staff(Ind, item);
			return TRUE;
		case TV_ROD:
			do_cmd_zap_rod(Ind, item, 5);
			return TRUE;
		case TV_WAND:
			do_cmd_aim_wand(Ind, item, 5);
			return TRUE;
		}
	}
#else
	switch (o_ptr->tval) {
	case TV_STAFF:
		if (((o_ptr->ident & ID_EMPTY) || ((o_ptr->ident & ID_KNOWN) && o_ptr->pval == 0))
		    && fallback)
			return (p_ptr->fail_no_melee);
		do_cmd_use_staff(Ind, item);
		return TRUE;
	case TV_ROD:
		if (o_ptr->pval != 0 && fallback)
			return (p_ptr->fail_no_melee);
		do_cmd_zap_rod(Ind, item, 5);
		return TRUE;
	case TV_WAND:
		if (((o_ptr->ident & ID_EMPTY) || ((o_ptr->ident & ID_KNOWN) && o_ptr->pval == 0))
		    && fallback)
			return (p_ptr->fail_no_melee);
		do_cmd_aim_wand(Ind, item, 5);
		return TRUE;
	}
#endif

	/* Accept reasonable targets:
	 * This prevents a player from getting stuck when facing a
	 * monster inside a wall.
	 * NOTE: The above statement becomes obsolete nowadays if
	 * PY_PROJ_ and similar are defined.
	 */
	if (!target_able(Ind, p_ptr->target_who)) return FALSE;

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
//redundant?->		if (item == INVEN_WIELD) return FALSE;
			return FALSE;
			break;

		/* directional ones */
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
//		case TV_BOOMERANG:
//		case TV_INSTRUMENT:
			if (item == INVEN_BOW || item == INVEN_AMMO) {
				if (!p_ptr->inventory[INVEN_AMMO].k_idx ||
					!p_ptr->inventory[INVEN_AMMO].number)
					break;

				retaliating_cmd = TRUE;
				do_cmd_fire(Ind, 5);
				if (p_ptr->ranged_double) do_cmd_fire(Ind, 5);
				return TRUE;
			}
			break;

		case TV_BOOMERANG:
			if (item == INVEN_BOW)
			{
				retaliating_cmd = TRUE;
				do_cmd_fire(Ind, 5);
				return TRUE;
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

			cost = exec_lua(Ind, format("return get_mana(%d, %d)", Ind, spell));
			if (cost > p_ptr->csp && fallback) return (p_ptr->fail_no_melee);

			/* Check that it's ok... more checks needed here? */
			/* Limit amount of mana used? */
			if (!p_ptr->blind && !no_lite(Ind) && !p_ptr->confused && cost <= p_ptr->csp && 
				exec_lua(Ind, format("return is_ok_spell(%d, %d)", Ind, spell)))
			{
				cast_school_spell(Ind, item, spell, 5, -1, 0);
				return TRUE;
			}
			break;


		case TV_RUNE: { //Format: @O<t?><imperative><form> {!R}

			/* Validate Rune */
			if (o_ptr->sval < 0 || o_ptr->sval > RCRAFT_MAX_PROJECTIONS) break;
			u16b e_flags = r_projections[o_ptr->sval].flags, m_flags = 0;
			byte m_index = 0;

			/* Validate Imperative */
			if (*inscription != '\0') {
				m_index = *inscription - 'a';
				if (m_index < 0 || m_index > RCRAFT_MAX_IMPERATIVES) return (p_ptr->fail_no_melee); //m_flags |= I_MINI;
				else m_flags |= r_imperatives[m_index].flag;
			}
			else {
				//if (cast_rune_spell(Ind, 5, e_flags, I_MINI | T_BOLT, 0, 1) == 2)
				return (p_ptr->fail_no_melee);
				//return TRUE;
			}

			/* Next Letter */
			inscription++;

			/* Validate Form */
			if (*inscription != '\0') {
				m_index = *inscription - 'a';
				if (m_index < 0 || m_index > RCRAFT_MAX_TYPES || r_types[m_index].flag == T_SIGL //Sigil spells aren't suitable.
				|| (r_types[m_index].flag == T_SIGN && (m_flags & I_ENHA) == I_ENHA)  //Glyphs too.
				|| (r_types[m_index].flag == T_SIGN && !((o_ptr->sval == SV_R_LITE) || (o_ptr->sval == SV_R_NETH)))) //And all non-damaging Signs. - Kurzel
					return (p_ptr->fail_no_melee);
				else m_flags |= r_types[m_index].flag;
			}
			else {
				//if (cast_rune_spell(Ind, 5, e_flags, I_MINI | T_BOLT, 0, 1) == 2)
				return (p_ptr->fail_no_melee);
				//return TRUE;
			}

			/* Retaliate or Melee */
			if (cast_rune_spell(Ind, 5, e_flags, m_flags, 0, 1) == 2) return (p_ptr->fail_no_melee);
			return TRUE;

		break; }
	}

	/* If all fails, then melee */
	return (p_ptr->fail_no_melee);
}

#ifdef AUTO_RET_CMD
/* Check auto-retaliation set via slash command /autoret or /ar.
   This was added for mimics to use their powers without having to inscribe
   a random item as a workaround. - C. Blue */
static bool retaliate_cmd(int Ind, bool fallback) {
	player_type *p_ptr = Players[Ind];
	int ar = p_ptr->autoret;

	/* Is it variant @Ot for town-only auto-retaliation? - C. Blue */
	if (ar >= 100) {
		if (!istownarea(&p_ptr->wpos, MAX_TOWNAREA)) return FALSE;
		ar -= 100;
	}

	/* no autoret set? */
	if (!ar) return FALSE;

	/* Hack -- use innate power via inscription on any item */
	if (get_skill(p_ptr, SKILL_MIMIC)) {
		/* Spell to cast */
		int choice = ar - 1;

		if (choice < 4)	/* 3 polymorph powers + immunity preference */
			return FALSE;

		int power = retaliate_mimic_power(Ind, choice - 1);
		bool dir = FALSE;
		if (innate_powers[power].smana > p_ptr->csp && fallback) return (p_ptr->fail_no_melee);
		switch (power / 32) {
		case 0: dir = monster_spells4[power].uses_dir;
			break;
		case 1: dir = monster_spells5[power - 32].uses_dir;
			break;
		case 2: dir = monster_spells6[power - 64].uses_dir;
			break;
		}

		/* Accept reasonable targets:
		 * This prevents a player from getting stuck when facing a
		 * monster inside a wall.
		 * NOTE: The above statement becomes obsolete nowadays if
		 * PY_PROJ_ and similar are defined.
		 */
		if (!target_able(Ind, p_ptr->target_who)) return FALSE;

		do_cmd_mimic(Ind, power + 3, dir ? 5 : 0);
		return TRUE;
	}

	/* If all fails, then melee */
	return (p_ptr->fail_no_melee);
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
		return FALSE;
	}
	if (p_ptr->new_level_flag) {
		p_ptr->ar_test_fail = TRUE;
		return FALSE;
	}
	/* disable auto-retaliation if we skip monsters/hostile players and blood-bonded players likewise */
	if (skip_monsters && !p_ptr->blood_bond) {
		p_ptr->ar_test_fail = TRUE;
		return FALSE;
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

				/* If it is a Q, then make it our new target. */
				/* We don't handle the case of choosing between two
				 * Q's because if the player is standing next to two Q's
				 * he deserves whatever punishment he gets.
				 */
				if (r_ptr->d_char == 'Q') {
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
		return FALSE;
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

					/* Skip this item in case it has @Ox */
					if (*inscription == 'x') {
						p_ptr->warning_autoret = 99; /* seems he knows what he's doing! */
						break;
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
	p_ptr->ar_no_melee = no_melee;

	p_ptr->ar_test_fail = FALSE;
	/* employ forced (via @O or @Q inscription) auto-retaliation? */
	return (at_O_inscription && *at_O_inscription != 'x');
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
	if (p_ptr->new_level_flag) return 0;
	/* disable auto-retaliation if we skip monsters/hostile players and blood-bonded players likewise */
	if (skip_monsters && !p_ptr->blood_bond) return 0;


	/* load data, from preceding auto_retaliate_test() call */
	if (p_ptr->ar_test_fail) return FALSE;

	m_target_ptr = p_ptr->ar_m_target_ptr;
	prev_m_target_ptr = p_ptr->ar_prev_m_target_ptr;
	p_target_ptr = p_ptr->ar_p_target_ptr;
	prev_p_target_ptr = p_ptr->ar_prev_p_target_ptr;

	target = p_ptr->ar_target;
	item = p_ptr->ar_item;
	fallback = p_ptr->ar_fallback;
	at_O_inscription = p_ptr->ar_at_O_inscription;
	no_melee = p_ptr->ar_no_melee;


	/* If we have a player target, attack him. */
	if (p_target_ptr) {
		/* set the target */
		p_ptr->target_who = target;

		/* Attack him */
		/* Stormbringer bypasses everything!! */
//		py_attack(Ind, p_target_ptr->py, p_target_ptr->px);
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
			return 1;
		} else {
			/* Otherwise return 2 to indicate we are no longer
			 * autoattacking anything.
			 */
			return 2;
		}
	}

	/* The dungeon master does not fight his or her offspring */
	if (p_ptr->admin_dm) return 0;

	/* If we have a target to attack, attack it! */
	if (m_target_ptr) {
		/* set the target */
		p_ptr->target_who = target;
		if (m_target_ptr->pet) { //a pet?
			return 0;
		}

		/* Attack it */
//		py_attack(Ind, m_target_ptr->fy, m_target_ptr->fx);
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
			return 1;
		} else {
			/* Otherwise return 2 to indicate we are no longer
			 * autoattacking anything.
			 */
			return 2;
		}
	}

	/* Nothing was attacked. */
	return 0;
}

/*
 * Player processing that occurs at the beginning of a new turn
 */
static void process_player_begin(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* for AT_VALINOR: */
	int i, x, y, xstart = 0, ystart = 0, ox, oy;
	dungeon_type *d_ptr;
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
#if 1
		for (y = 0; y < MAX_WILD_Y; y++) {
			for (x = 0; x < MAX_WILD_X; x++) {
				if ((d_ptr = wild_info[y][x].tower) && (!strcmp(d_name + d_info[d_ptr->type].name, "The Shores of Valinor"))) {
					p_ptr->recall_pos.wx = x;
					p_ptr->recall_pos.wy = y;
					p_ptr->recall_pos.wz = 1;
					p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
					recall_player(Ind, "");
					break;
				}
				if ((d_ptr = wild_info[y][x].dungeon) && (!strcmp(d_name + d_info[d_ptr->type].name, "The Shores of Valinor"))) {
					p_ptr->recall_pos.wx = x;
					p_ptr->recall_pos.wy = y;
					p_ptr->recall_pos.wz = -1;
					// let's try LEVEL_OUTSIDE_RAND (5) instead of LEVEL_OUTSIDE (4) - C. Blue :)
					p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
					recall_player(Ind, "");
					break;
				}
			}
		}
		p_ptr->auto_transport = AT_VALINOR2;
		break;
#endif
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
		wiz_lite(Ind);
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
		break;
	case AT_VALINOR3:	/* Orome mumbles */
		if (turn % 300) break; /* cool down.. */
		msg_print(Ind, "\374 ");
		msg_print(Ind, "\374\377oOrome, The Hunter, mumbles something about a spear..");
		p_ptr->auto_transport = AT_VALINOR4;
		break;
	case AT_VALINOR4:	/* Orome looks */
		if (turn % 500) break; /* cool down.. */
		msg_print(Ind, "\374 ");
		msg_print(Ind, "\374\377oOrome, The Hunter, notices you and surprise crosses his face!");
		p_ptr->auto_transport = AT_VALINOR5;
		break;
	case AT_VALINOR5:	/* Orome laughs */
		if (turn % 500) break; /* cool down.. */
		msg_print(Ind, "\374 ");
		msg_print(Ind, "\374\377oOrome, The Hunter, laughs out loudly!");
		set_afraid(Ind, 8);
		p_ptr->auto_transport = AT_VALINOR6;
		break;
	case AT_VALINOR6:	/* Orome offers */
		if (turn % 500) break; /* cool down.. */
		msg_print(Ind, "\374 ");
		msg_print(Ind, "\374\377oOrome, The Hunter, offers you to stay here!");
		msg_print(Ind, "\374\377y  (You may hit the suicide keys in order to retire here,");
		msg_print(Ind, "\374\377y  or take the staircase back to the mundane world.)");
		msg_print(Ind, "\374 ");
		p_ptr->auto_transport = 0;
		break;
	}

#if defined(TARGET_SWITCHING_COST) || defined(TARGET_SWITCHING_COST_RANGED)
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


	/* Hack -- semi-constant hallucination */
	if (p_ptr->image && (randint(10) < 1)) p_ptr->redraw |= (PR_MAP);

	/* Mega-Hack -- Random teleportation XXX XXX XXX */
	if ((p_ptr->teleport) && (rand_int(100) < 1)
	    /* not during highlander tournament */
	    && (p_ptr->wpos.wx || p_ptr->wpos.wy || !sector00separation))
	{
		/* Teleport player */
		teleport_player(Ind, 40, FALSE);
	}

}


/*
 * Generate the feature effect
 */
static void apply_effect(int Ind) {
	player_type *p_ptr = Players[Ind];
	int y = p_ptr->py, x = p_ptr->px;
	cave_type *c_ptr;
	feature_type *f_ptr;

	cave_type **zcave;
	if (!(zcave = getcave(&p_ptr->wpos))) return;

	c_ptr = &zcave[y][x];
	f_ptr = &f_info[c_ptr->feat];

	if (f_ptr->d_frequency[0] != 0) {
		int i;

		for (i = 0; i < 4; i++) {
			/* Check the frequency */
			if (f_ptr->d_frequency[i] == 0) continue;

			/* XXX it's no good to use 'turn' here */
			//if (((turn % f_ptr->d_frequency[i]) == 0) &&

			if ((!rand_int(f_ptr->d_frequency[i])) &&
			    ((f_ptr->d_side[i] != 0) || (f_ptr->d_dice[i] != 0))) {
				int l, dam = 0;
				int d = f_ptr->d_dice[i], s = f_ptr->d_side[i];

				if (d == -1) d = p_ptr->lev;
				if (s == -1) s = p_ptr->lev;

				/* Roll damage */
				for (l = 0; l < d; l++) {
					dam += randint(s);
				}

				/* Apply damage */
				project(PROJECTOR_TERRAIN, 0, &p_ptr->wpos, y, x, dam, f_ptr->d_type[i],
				    PROJECT_NORF | PROJECT_KILL | PROJECT_HIDE | PROJECT_JUMP | PROJECT_NODO | PROJECT_NODF, "");

				/* Hack -- notice death */
//				if (!alive || death) return;
				if (p_ptr->death) return;
			}
		}
	}
}

#if 0
/* admin can summon a player anywhere he/she wants */
void summon_player(int victim, struct worldpos *wpos, char *message){
	struct player_type *p_ptr;
	p_ptr = Players[victim];
	p_ptr->recall_pos.wx = wpos->wx;
	p_ptr->recall_pos.wy = wpos->wy;
	p_ptr->recall_pos.wz = wpos->wz;
	recall_player(victim, message);
}
#endif

/* actually recall a player with no questions asked */
void recall_player(int Ind, char *message) {
	struct player_type *p_ptr;
	cave_type **zcave;
	struct worldpos old_wpos;
	char buf[MAX_CHARS_WIDE];

	p_ptr = Players[Ind];

	if(!p_ptr) return;
	if(!(zcave = getcave(&p_ptr->wpos))) return;	// eww

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
	sound(Ind, "teleport", NULL, SFX_TYPE_COMMAND, FALSE);
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
	new_players_on_depth(&p_ptr->wpos, 1, TRUE);

	p_ptr->new_level_flag = TRUE;

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
	if (in_irondeepdive(&old_wpos)
	    && !is_admin(p_ptr)) {
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
			sprintf(deep_dive_name[i], "%s, %s %s (\\{%c%c\\{s/\\{%c%d\\{s),",
			    p_ptr->name, get_prace(p_ptr), class_info[p_ptr->pclass].title, color_attr_to_char(p_ptr->cp_ptr->color), p_ptr->fruit_bat ? 'b' : '@',
			    p_ptr->ghost ? 'r' : 's', p_ptr->max_plv);
#else
			sprintf(deep_dive_name[i], "%s, %s %s (\\{%c%d\\{s),",
			    p_ptr->name, get_prace(p_ptr), class_info[p_ptr->pclass].title,
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
			p_ptr->iron_winner = TRUE;
			msg_print(Ind, "\374\377L***\377aYou made it through the Ironman Deep Dive challenge!\377L***");
			sprintf(buf, "\374\377L***\377a%s made it through the Ironman Deep Dive challenge!\377L***", p_ptr->name);
			msg_broadcast(Ind, buf);
			l_printf("%s \\{U%s (%d) made it through the Ironman Deep Dive challenge\n", showdate(), p_ptr->name, p_ptr->lev);
#ifdef TOMENET_WORLDS
			if (cfg.worldd_events) world_msg(buf);
#endif
			/* enforce dedicated Ironman Deep Dive Challenge character slot usage */
			if (p_ptr->mode & MODE_DED_IDDC) {
				msg_print(Ind, "\377aYou return to town and may retire ('Q') when ready.");
				msg_print(Ind, "\377aIf you enter a dungeon or tower, \377Rretirement\377a is assumed \377Rautomatically\377a.");

				process_player_change_wpos(Ind);
				p_ptr->recall_pos.wx = cfg.town_x;
				p_ptr->recall_pos.wy = cfg.town_y;

				wpcopy(&old_wpos, &p_ptr->wpos);
				wpcopy(&p_ptr->wpos, &p_ptr->recall_pos);
				new_players_on_depth(&old_wpos, -1, TRUE);
				s_printf("Recalled: %s from %d,%d,%d to %d,%d,%d.\n", p_ptr->name,
				    old_wpos.wx, old_wpos.wy, old_wpos.wz,
				    p_ptr->recall_pos.wx, p_ptr->recall_pos.wy, p_ptr->recall_pos.wz);
				new_players_on_depth(&p_ptr->wpos, 1, TRUE);
				p_ptr->new_level_flag = TRUE;
				set_invuln_short(Ind, RECALL_GOI_LENGTH);	// It runs out if attacking anyway
				p_ptr->word_recall = 0;
				process_player_change_wpos(Ind);

				/* Restrict character to world surface */
				p_ptr->iron_winner_ded = TRUE;
			}

			p_ptr->IDDC_flags = 0x0; //clear IDDC specialties
#ifdef IRONDEEPDIVE_FIXED_TOWN_WITHDRAWAL
		} else {
			/* enforce dedicated Ironman Deep Dive Challenge character slot usage */
			if (p_ptr->mode & MODE_DED_IDDC) {
				msg_print(Ind, "\377RYou failed to complete the Ironman Deep Dive Challenge!");
				strcpy(p_ptr->died_from, "indetermination");
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
	if (in_hallsofmandos(&old_wpos)
	    && !is_admin(p_ptr)) {
		msg_print(Ind, "\374\377a***\377sYou made it through the Halls of Mandos!\377a***");
		sprintf(buf, "\374\377a***\377s%s made it through the Halls of Mandos!\377a***", p_ptr->name);
		msg_broadcast(Ind, buf);
		l_printf("%s \\{U%s (%d) made it through the Halls of Mandos\n", showdate(), p_ptr->name, p_ptr->lev);
	}
}


/* Handles WoR
 * XXX dirty -- REWRITEME
 */
static void do_recall(int Ind, bool bypass) {
	player_type *p_ptr = Players[Ind];
	char *message = NULL;

	/* sorta circumlocution? */
	worldpos new_pos;
	bool recall_ok = TRUE;

	/* Disturbing! */
	disturb(Ind, 0, 0);

	/* special restriction for global events (Highlander Tournament) */
	if (sector00separation && !is_admin(p_ptr) && (
	    (!p_ptr->recall_pos.wx && !p_ptr->recall_pos.wy) ||
	    (!p_ptr->wpos.wx && !p_ptr->wpos.wy))) {
		if (p_ptr->global_event_temp & PEVF_PASS_00) {
			p_ptr->global_event_temp &= ~PEVF_PASS_00;
		} else {
			msg_print(Ind, "\377oA tension leaves the air around you...");
			p_ptr->redraw |= (PR_DEPTH);
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
		if (d_ptr){
			if ((d_ptr->type && d_info[d_ptr->type].min_plev > p_ptr->lev) ||
			    (!d_ptr->type && d_ptr->baselevel <= (p_ptr->lev * 3) / 2 + 7)) {
				msg_print(Ind,"\377rAs you attempt to recall, you are gripped by an uncontrollable fear.");
				msg_print(Ind, "\377oA tension leaves the air around you...");
				p_ptr->redraw |= (PR_DEPTH);
				if (!is_admin(p_ptr)) {
					set_afraid(Ind, 10);// + (d_ptr->baselevel - p_ptr->max_dlv));
					return;
				}
			}
		}
#endif
		/* Nether Realm only for Kings/Queens (currently paranoia, since NR is NO_RECALL_INTO) */
		if (d_ptr && (d_ptr->type == DI_NETHER_REALM) && !p_ptr->total_winner) {
			msg_print(Ind,"\377rAs you attempt to recall, you are gripped by an uncontrollable fear.");
			if (!is_admin(p_ptr)) {
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
		if (((d_ptr->flags2 & DF2_IRON || d_ptr->flags1 & DF1_FORCE_DOWN) && d_ptr->maxdepth > ABS(p_ptr->wpos.wz)) ||
		    (d_ptr->flags1 & DF1_NO_RECALL)) {
			if (!(getfloor(&p_ptr->wpos)->flags1 & LF1_IRON_RECALL)) {
				msg_print(Ind, "You feel yourself being pulled toward the surface!");
				if (!is_admin(p_ptr)) {
					recall_ok = FALSE;
					/* Redraw the depth(colour) */
					p_ptr->redraw |= (PR_DEPTH);
				}
			} else if ((p_ptr->mode & MODE_DED_IDDC) && in_irondeepdive(&p_ptr->wpos)) {
				msg_print(Ind, "You have dedicated yourself to the Ironman Deep Dive Challenge!");
				if (!is_admin(p_ptr)) {
					recall_ok = FALSE;
					/* Redraw the depth(colour) */
					p_ptr->redraw |= (PR_DEPTH);
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
			p_ptr->new_level_method = ( istown(&new_pos) ? LEVEL_RAND : LEVEL_OUTSIDE_RAND );
#endif
			p_ptr->new_level_method = (p_ptr->wpos.wz>0 ? LEVEL_RECALL_DOWN : LEVEL_RECALL_UP);
		}
	}
	/* beware! bugs inside! (jir) (no longer I hope) */
	/* world travel */
	/* why wz again? (jir) */
	else if ((!(p_ptr->recall_pos.wz) || !(wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags & (WILD_F_UP|WILD_F_DOWN))) && !bypass) {
		int dis;

		if (((!(p_ptr->wild_map[(wild_idx(&p_ptr->recall_pos))/8] &
		    (1U << (wild_idx(&p_ptr->recall_pos))%8))) &&
		    !is_admin(p_ptr) ) ||
		    inarea(&p_ptr->wpos, &p_ptr->recall_pos))
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
			//if(d_ptr->baselevel-p_ptr->max_dlv>2){
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
			//if(d_ptr->baselevel-p_ptr->max_dlv>2){
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

/*
 * Handle misc. 'timed' things on the player.
 * returns FALSE if player no longer exists.
 *
 * TODO: find a better way for timed spells(set_*).
 */
static bool process_player_end_aux(int Ind) {
	player_type *p_ptr = Players[Ind];
	cave_type		*c_ptr;
	object_type		*o_ptr;
	int		i, j;
	int		regen_amount; //, NumPlayers_old = NumPlayers;
	dun_level *l_ptr = getfloor(&p_ptr->wpos);
	dungeon_type *d_ptr = getdungeon(&p_ptr->wpos);
	char o_name[ONAME_LEN];

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

	cave_type **zcave;
	if (!(zcave = getcave(&p_ptr->wpos))) return(FALSE);

	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* Anything done here cannot be reduced by GoI/Manashield etc */
	bypass_invuln = TRUE;

	/*** Damage over Time ***/

	/* Take damage from poison */
	if (p_ptr->poisoned) {
		if (p_ptr->slow_poison == 1) {
			p_ptr->slow_poison = 2;
			/* Take damage */
			take_hit(Ind, 1, "poison", p_ptr->poisoned_attacker);
		} else if (p_ptr->slow_poison == 2) {
			p_ptr->slow_poison = 1;
		} else {
			/* Take damage */
			take_hit(Ind, 1, "poison", p_ptr->poisoned_attacker);
		}
	}

	/* Misc. terrain effects */
	if (!p_ptr->ghost) {
		/* Generic terrain effects */
		apply_effect(Ind);

		/* Drowning, but not ghosts */
		if (c_ptr->feat == FEAT_DEEP_WATER) {
			bool carry_wood = FALSE;

			for (i = 0; i < INVEN_PACK; i++) {
				o_ptr = &p_ptr->inventory[i];
				if (!o_ptr->k_idx) break;
				if (o_ptr->tval == TV_GOLEM && o_ptr->sval == SV_GOLEM_WOOD) {
					carry_wood = TRUE;
					break;
				}
			}

			/* note: TODO (bug): items should get damaged even if player can_swim,
			   but this might devalue swimming too much compared to levitation. Dunno. */
			if ((!p_ptr->tim_wraith) && (!p_ptr->levitate) && (!p_ptr->can_swim) &&
			    !(p_ptr->body_monster && (r_info[p_ptr->body_monster].flags7 & (RF7_AQUATIC | RF7_CAN_SWIM)))) {
				int hit = p_ptr->mhp >> 6;
				int swim = get_skill_scale(p_ptr, SKILL_SWIM, 4500);
				hit += randint(p_ptr->mhp >> 5);
				if (!hit) hit = 1;

				/* Take CON into consideration(max 30%) */
				//note: if hit was 1, this can each result in 0 aka no hit!
				hit = (hit * (80 - adj_str_wgt[p_ptr->stat_ind[A_CON]])) / 75;
				if (p_ptr->suscep_life) hit >>= 1;

				/* temporary abs weight calc */
				if (p_ptr->wt + p_ptr->total_weight / 10 > 170 + swim * 2) { // 190
					if (carry_wood) {
						/* Clinging to the massive piece of wood, we're safe */
						if (!rand_int(5)) msg_print(Ind, "You cling to the wood!");
					} else { /* Take damage from drowning */
						long factor = (p_ptr->wt + p_ptr->total_weight / 10) - 150 - swim * 2; // 170
						/* too heavy, always drown? */
						if (factor < 300 && randint(factor) < 20) hit = 0;

						/* Hack: Take STR and DEX into consideration (max 28%) */
						if (magik(adj_str_wgt[p_ptr->stat_ind[A_STR]] / 2) ||
						    magik(adj_str_wgt[p_ptr->stat_ind[A_DEX]]) / 2)
							hit = 0;

						if (magik(swim)) hit = 0;

						if (hit) msg_print(Ind,"\377rYou're drowning!");

						/* harm equipments (even hit == 0) */
						if (TOOL_EQUIPPED(p_ptr) != SV_TOOL_TARPAULIN &&
						    magik(WATER_ITEM_DAMAGE_CHANCE) && !p_ptr->levitate &&
						    !p_ptr->immune_water) {
							if (!p_ptr->resist_water || magik(50)) {
								if (!magik(get_skill_scale(p_ptr, SKILL_SWIM, 4900)))
									inven_damage(Ind, set_water_destroy, 1);
								equip_damage(Ind, GF_WATER);
							}
						}

						if (randint(1000 - factor) < 10 && !p_ptr->sustain_str) {
							msg_print(Ind,"\377oYou are weakened by the exertion of swimming!");
							dec_stat(Ind, A_STR, 10, STAT_DEC_TEMPORARY);
						}
						take_hit(Ind, hit, "drowning", 0);
					}
				}
			}
		}
		/* Aquatic anoxia */
		else if ((p_ptr->body_monster) &&
		    ((r_info[p_ptr->body_monster].flags7 & RF7_AQUATIC) &&
		    !(r_info[p_ptr->body_monster].flags3 & RF3_UNDEAD))
		    && ((c_ptr->feat != FEAT_SHAL_WATER) ||
		    r_info[p_ptr->body_monster].weight > 700)
		    /* new: don't get stunned from crossing door/stair grids every time - C. Blue */
		    && !is_always_passable(c_ptr->feat)
		    && !p_ptr->tim_wraith)
		{
			long hit = p_ptr->mhp>>6; /* Take damage */
			hit += randint(p_ptr->chp>>5);
			/* Take CON into consideration(max 30%) */
			hit = (hit * (80 - adj_str_wgt[p_ptr->stat_ind[A_CON]])) / 75;
			if (!hit) hit = 1;

			if (hit) {
				if (c_ptr->feat != FEAT_SHAL_WATER)
					msg_print(Ind,"\377rYou cannot breathe air!");
				else
					msg_print(Ind,"\377rThere's not enough water to breathe!");
			}

			if (randint(1000) < 10) {
				msg_print(Ind,"\377rYou find it hard to stir!");
//				do_dec_stat(Ind, A_DEX, STAT_DEC_TEMPORARY);
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
			else if (c_ptr->feat == FEAT_HOME){/* rien */}
			//else if (PRACE_FLAG(PR1_SEMI_WRAITH) && (!p_ptr->wraith_form) && (f_info[cave[py][px].feat].flags1 & FF1_CAN_PASS))
			else if (!p_ptr->tim_wraith &&
			    !p_ptr->master_move_hook) { /* Hack -- builder is safe */
				//int amt = 1 + ((p_ptr->lev)/5) + p_ptr->mhp / 100;
				/* Currently it only serves to reduce 'stuck' players' HP,
				   so we might lower it a bit - C. Blue */
				int amt = 1 + ((p_ptr->lev)/10) + p_ptr->mhp / 100;
				int amt2 = p_ptr->msp / 35;

				/* hack: disruption shield suffers a lot if it's got to withstand the walls oO */
				if (p_ptr->tim_manashield) {
					amt += amt2;

					/* Be nice and don't let the disruption shield dissipate */
					if (p_ptr->pclass == CLASS_MAGE) {
						if (amt > p_ptr->csp - 1) amt = p_ptr->csp - 1;
					} else {
						/* Other classes take double damage */
						if (2 * amt > p_ptr->csp - 1) amt = (p_ptr->csp - 1) / 2;
					}

					/* Disruption shield protects from this type of damage */
					bypass_invuln = FALSE;
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
			project(PROJECTOR_TERRAIN, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, randint(20), GF_COLD,
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


	/* Take damage from cuts */
	if (p_ptr->cut) {
		/* Mortal wound or Deep Gash */
		if (p_ptr->cut > 200) i = 3;
		/* Severe cut */
		else if (p_ptr->cut > 100) i = 2;
		/* Other cuts */
		else i = 1;

		/* Take damage */
		take_hit(Ind, i, "a fatal wound", p_ptr->cut_attacker);
	}

	/*** Check the Food, and Regenerate ***/
	/* Ent's natural food while in 'Resting Mode' - C. Blue
	   Water helps much, natural floor helps some. */
	if (!p_ptr->ghost && p_ptr->prace == RACE_ENT && p_ptr->resting) {
		if (c_ptr->feat == FEAT_SHAL_WATER || c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_MUD) {
			i = 200; //Delicious!
		} else if (c_ptr->feat == FEAT_GRASS || c_ptr->feat == FEAT_DIRT) {
			i = 100;
		} else if (c_ptr->feat == FEAT_BUSH || c_ptr->feat == FEAT_TREE) {
			i = 70;
		} else {
			i = 0;
		}
		if (i > 0 && set_food(Ind, p_ptr->food + i)) {
			msg_print(Ind, "You gain some nourishment from around you.");
#if 0 /* spammy in town */
			switch(i) {
				case 70:
					msg_format_near(Ind, "\374\377wYou hear strange sounds coming from the direction of %s.", p_ptr->name);
					break;
				case 100:
					msg_format_near(Ind, "\374\377w%s digs %s roots deep into the ground.", p_ptr->name, (p_ptr->male?"his":"her"));
					break;
				case 200:
					msg_format_near(Ind, "\374\377w%s absorbs all the water around %s.", p_ptr->name, (p_ptr->male?"him":"her"));
			}
#endif
		}
	}
	/* Ghosts don't need food */
	/* Allow AFK-hivernation if not hungry */
	else if (!p_ptr->ghost && !(p_ptr->afk && p_ptr->food >= PY_FOOD_ALERT) && !p_ptr->admin_dm &&
	    /* Don't starve in town (but recover from being gorged) - C. Blue */
	    //(!istown(&p_ptr->wpos) || p_ptr->food >= PY_FOOD_MAX))
	    (!(istownarea(&p_ptr->wpos, MAX_TOWNAREA) || isdungeontown(&p_ptr->wpos))
	    || p_ptr->food >= PY_FOOD_FULL)) /* allow to digest some to not get gorged in upcoming fights quickly - C. Blue */
	{
		/* Digest normally */
		if (p_ptr->food < PY_FOOD_MAX) {
			/* Every 50/6 level turns */
			//if (!(turn % ((level_speed((&p_ptr->wpos)) * 10) / 12)))
			if (!(turn % ((level_speed((&p_ptr->wpos)) / 120) * 10))) {
				/* Basic digestion rate based on speed */
				//i = (extract_energy[p_ptr->pspeed] / 10) * 2;	// 1.3 (let them starve)
				i = (10 + (extract_energy[p_ptr->pspeed] / 10) * 3) / 2;

				/* Adrenaline takes more food */
				if (p_ptr->adrenaline) i *= 5;

				/* Biofeedback takes more food */
				if (p_ptr->biofeedback) i *= 2;

				/* Regeneration takes more food */
				if (p_ptr->regenerate) i += 30;

				/* Regeneration takes more food */
				if (p_ptr->tim_regen) i += p_ptr->tim_regen_pow / 10;

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

				/* Never negative */
				if (i < 1) i = 1;

				/* Cut vampires some slack for Nether Realm:
				   Ancient vampire lords almost don't need any food at all */
				if (p_ptr->prace == RACE_VAMPIRE && p_ptr->total_winner) i = 1;

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
	if (p_ptr->resting && !p_ptr->searching)
		regen_amount = regen_amount * RESTING_RATE;

	/* Regenerate the mana */
#ifdef ARCADE_SERVER
	p_ptr->regen_mana = TRUE;
#endif
	/* Hack -- regenerate mana 5/3 times faster */
	if (p_ptr->csp < p_ptr->msp) {
		if (in_pvparena(&p_ptr->wpos))
			regenmana(Ind, ((regen_amount * 5 * PVP_MANA_REGEN_BOOST) * (p_ptr->regen_mana ? 2 : 1)) / 3);
		else
			regenmana(Ind, ((regen_amount * 5) * (p_ptr->regen_mana ? 2 : 1)) / 3);
	}

	/* Regeneration ability */
	if (p_ptr->regenerate)
		regen_amount = regen_amount * 2;

	/* Health skill improves regen_amount */
	if (minus_health)
		regen_amount = regen_amount * 3 / 2;

	/* Holy curing gives improved regeneration ability */
	if (get_skill(p_ptr, SKILL_HCURING) >= 30) regen_amount = (regen_amount * (get_skill(p_ptr, SKILL_HCURING) - 10)) / 20;

	/* Blood Magic, aka Auras, draw from your blood and thereby slow your regen */
	regen_amount = regen_amount / (1 + (p_ptr->aura[0] ? 1 : 0) + (p_ptr->aura[1] ? 1 : 0) + (p_ptr->aura[2] ? 1 : 0));

	/* Increase regen by tim regen */
	if (p_ptr->tim_regen) regen_amount += p_ptr->tim_regen_pow;

	/* Poisoned or cut yields no healing */
	if (p_ptr->poisoned || p_ptr->cut || p_ptr->sun_burn)
		regen_amount = 0;

	/* But Biofeedback always helps */
	if (p_ptr->biofeedback) regen_amount += randint(1024) + regen_amount;

	/* Regenerate Hit Points if needed */
	if (p_ptr->chp < p_ptr->mhp && regen_amount && !p_ptr->martyr)
		regenhp(Ind, regen_amount);

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
	if ((p_ptr->cst < p_ptr->mst) && !p_ptr->shadow_running) {
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

	/* Disturb if we are done resting */
	if ((p_ptr->resting) && (p_ptr->chp == p_ptr->mhp) && (p_ptr->csp == p_ptr->msp) && (p_ptr->cst == p_ptr->mst)
	    && !(p_ptr->prace == RACE_ENT && p_ptr->food < PY_FOOD_FULL))
		disturb(Ind, 0, 0);

	/* Finally, at the end of our turn, update certain counters. */
	/*** Timeout Various Things ***/

	/* Handle temporary stat drains */
	for (i = 0; i < 6; i++) {
		if (p_ptr->stat_cnt[i] > 0) {
			p_ptr->stat_cnt[i] -= (1 + minus_health);
			if (p_ptr->stat_cnt[i] <= 0) {
				do_res_stat_temp(Ind, i);
			}
		}
	}

	/* Adrenaline */
	if (p_ptr->adrenaline)
		(void)set_adrenaline(Ind, p_ptr->adrenaline - 1);

	/* Biofeedback */
	if (p_ptr->biofeedback)
		(void)set_biofeedback(Ind, p_ptr->biofeedback - 1);

	/* Hack -- Bow Branding */
	if (p_ptr->bow_brand)
		(void)set_bow_brand(Ind, p_ptr->bow_brand - minus_magic, p_ptr->bow_brand_t, p_ptr->bow_brand_d);

	/* weapon brand time */
	if (p_ptr->brand) {
		(void)set_brand(Ind, p_ptr->brand - minus_magic, p_ptr->brand_t, p_ptr->brand_d);
	}

	/* Hack -- Timed ESP */
	if (p_ptr->tim_esp)
		(void)set_tim_esp(Ind, p_ptr->tim_esp - minus_magic);

	/* Hack -- Space/Time Anchor */
	if (p_ptr->st_anchor)
		(void)set_st_anchor(Ind, p_ptr->st_anchor - minus_magic);

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
			if (!istownarea(&p_ptr->wpos, MAX_TOWNAREA) && !isdungeontown(&p_ptr->wpos)) {
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
	if (p_ptr->tim_jail && !p_ptr->wpos.wz) {
		p_ptr->tim_jail--;
		if (!p_ptr->tim_jail) {
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

			/* only teleport him if he didn't take the ironman exit. */
			if (zcave[p_ptr->py][p_ptr->px].info & CAVE_JAIL) {
				msg_print(Ind, "\377GYou are free to go!");

				/* Get the jail door location */
				if (!p_ptr->house_num) teleport_player_force(Ind, 1);
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
					p_ptr->redraw |= PR_BPR_WRAITH;
				}
			}
		}
	}

	/* Hack -- Tunnel */
#if 0	// not used
	if (p_ptr->auto_tunnel)
		p_ptr->auto_tunnel--;
#endif	// 0
	/* Hack -- Meditation */
	if (p_ptr->tim_meditation)
		(void)set_tim_meditation(Ind, p_ptr->tim_meditation - minus);

	/* Hack -- Wraithform */
	if (p_ptr->tim_wraith)
		(void)set_tim_wraith(Ind, p_ptr->tim_wraith - minus_magic);

	/* Hack -- Hallucinating */
	if (p_ptr->image) {
		int adjust = 1 + minus_health;
		if (get_skill(p_ptr, SKILL_HCURING) >= 50) adjust++;
		(void)set_image(Ind, p_ptr->image - adjust);
	}

	/* Blindness */
	if (p_ptr->blind) {
		int adjust = 1 + minus_health;
		if (get_skill(p_ptr, SKILL_HCURING) >= 30) adjust++;
		(void)set_blind(Ind, p_ptr->blind - adjust);
	}

	/* Times see-invisible */
	if (p_ptr->tim_invis)
		(void)set_tim_invis(Ind, p_ptr->tim_invis - minus_magic);

	/* Times invisibility */
	if (p_ptr->tim_invisibility) {
		if (p_ptr->aggravate) {
			msg_print(Ind, "Your invisibility shield is broken by your aggravation.");
			(void)set_invis(Ind, 0, 0);
		} else {
			(void)set_invis(Ind, p_ptr->tim_invisibility - minus_magic, p_ptr->tim_invis_power2);
		}
	}

	/* Timed infra-vision */
	if (p_ptr->tim_infra)
		(void)set_tim_infra(Ind, p_ptr->tim_infra - minus_magic);

	/* Paralysis */
	if (p_ptr->paralyzed)
		(void)set_paralyzed(Ind, p_ptr->paralyzed - 1);// - minus_health

	/* Confusion */
	if (p_ptr->confused)
		(void)set_confused(Ind, p_ptr->confused - minus - minus_combat - minus_health);

	/* Afraid */
	if (p_ptr->afraid)
		(void)set_afraid(Ind, p_ptr->afraid - minus - minus_combat);

	/* Fast */
	if (p_ptr->fast)
		(void)set_fast(Ind, p_ptr->fast - minus_magic, p_ptr->fast_mod);

	/* Slow */
	if (p_ptr->slow)
		(void)set_slow(Ind, p_ptr->slow - minus_magic); // - minus_health

#ifdef ENABLE_MAIA
	if (p_ptr->divine_crit)
		(void)do_divine_crit(Ind, p_ptr->divine_crit_mod, p_ptr->divine_crit - minus_magic);

	if (p_ptr->divine_hp)
		(void)do_divine_hp(Ind, p_ptr->divine_hp_mod, p_ptr->divine_hp - minus_magic);

	if (p_ptr->divine_xtra_res_time)
		(void)do_divine_xtra_res_time(Ind, p_ptr->divine_xtra_res_time - minus_magic);
#endif
	/* xtra shot? - the_sandman */
	if (p_ptr->focus_time)
		(void)do_focus(Ind, p_ptr->focus_val, p_ptr->focus_time - minus_magic);

	/* xtra stats? - the_sandman */
	if (p_ptr->xtrastat_tim)
		(void)do_xtra_stats(Ind, p_ptr->xtrastat_which, p_ptr->xtrastat_pow, p_ptr->xtrastat_tim - minus_magic);

	/* Protection from evil */
	if (p_ptr->protevil)
		(void)set_protevil(Ind, p_ptr->protevil - minus_magic);

	/* Holy Zeal - EA bonus */
	if (p_ptr->zeal)
		(void)set_zeal(Ind, p_ptr->zeal_power, p_ptr->zeal - 1);

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
	if (p_ptr->blessed)
		(void)set_blessed(Ind, p_ptr->blessed - 1);

	/* Shield */
	if (p_ptr->shield)
		(void)set_shield(Ind, p_ptr->shield - minus_magic, p_ptr->shield_power, p_ptr->shield_opt, p_ptr->shield_power_opt, p_ptr->shield_power_opt2);

	/* Timed deflection */
	if (p_ptr->tim_deflect)
		(void)set_tim_deflect(Ind, p_ptr->tim_deflect - minus_magic);

	/* Timed Feather Falling */
	if (p_ptr->tim_ffall)
		(void)set_tim_ffall(Ind, p_ptr->tim_ffall - 1);
	if (p_ptr->tim_lev)
		(void)set_tim_lev(Ind, p_ptr->tim_lev - 1);

	/* Timed regen */
	if (p_ptr->tim_regen)
		(void)set_tim_regen(Ind, p_ptr->tim_regen - 1, p_ptr->tim_regen_pow);

	/* Thunderstorm */
	if (p_ptr->tim_thunder) {
		int dam = damroll(p_ptr->tim_thunder_p1, p_ptr->tim_thunder_p2);
		int x, y, tries = 40;
		monster_type *m_ptr = NULL;

		while (--tries) {
			/* Pick random location within thunderstorm radius and check for creature */
			scatter(&p_ptr->wpos, &y, &x, p_ptr->py, p_ptr->px, (MAX_RANGE * 2) / 3, FALSE);
			//(max possible grids [tries recommended]: 1 ~1020, 5/6 ~710, 4/5 ~615, 3/4 ~530, 2/3 ~450 [30], 1/2 ~255 [15])
			c_ptr = &zcave[y][x];

			/* Any creature/player here, apart from ourselves? */
			if (!c_ptr->m_idx || -c_ptr->m_idx == Ind) continue;

			/* Non-hostile player or special projector? Skip. */
			if (c_ptr->m_idx < 0 &&
			    (c_ptr->m_idx <= PROJECTOR_UNUSUAL || !check_hostile(Ind, -c_ptr->m_idx)))
				continue;

			/* Access the monster */
			m_ptr = &m_list[c_ptr->m_idx];

			/* Ignore "dead" monsters */
			if (!m_ptr->r_idx) continue;

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

			/* We found a target */
			break;
		}

		if (tries) {
			char m_name[MNAME_LEN];

			monster_desc(Ind, m_name, c_ptr->m_idx, 0);
			msg_format(Ind, "Lightning strikes %s.", m_name);
#ifdef USE_SOUND_2010
			sound_near_site_vol(y, x, &p_ptr->wpos, 0, "lightning", "thunder", SFX_TYPE_NO_OVERLAP, FALSE, 50); //don't overlap, too silyl?
#endif
			project(0 - Ind, 0, &p_ptr->wpos, y, x, dam, GF_THUNDER, PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID | PROJECT_JUMP | PROJECT_NODF | PROJECT_NODO, "");
			thunderstorm_visual(&p_ptr->wpos, x, y);
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
		int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus);

		/* Apply some healing */
		if (get_skill(p_ptr, SKILL_HCURING) >= 30) adjust *= 2;

		//(void)set_poisoned(Ind, p_ptr->poisoned - adjust - minus_health * 2, p_ptr->poisoned_attacker);
		(void)set_poisoned(Ind, p_ptr->poisoned - (adjust + minus_health) * (minus_health + 1), p_ptr->poisoned_attacker);

		if (!p_ptr->poisoned) p_ptr->slow_poison = 0;
	}

	/* Stun */
	if (p_ptr->stun) {
		/* Apply some healing */
		int adjust = minus + get_skill_scale_fine(p_ptr, SKILL_COMBAT, 1);

		if (rand_int(9) < adj_con_fix[p_ptr->stat_ind[A_CON]]) adjust++;
		if (get_skill(p_ptr, SKILL_HCURING) >= 40) adjust++;
		(void)set_stun_raw(Ind, p_ptr->stun - adjust);
	}

	/* Cut */
	if (p_ptr->cut) {
		int adjust = minus;// = (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus);

		/* Biofeedback always helps */
		if (p_ptr->biofeedback) adjust += 5;

		/* Hack -- Truly "mortal" wound */
		if (p_ptr->cut > 1000) adjust = 0;

		if (get_skill(p_ptr, SKILL_HCURING) >= 40) adjust *= 2;

		/* Apply some healing */
		//(void)set_cut(Ind, p_ptr->cut - adjust - minus_health * 2, p_ptr->cut_attacker);
		//(void)set_cut(Ind, p_ptr->cut - (adjust + minus_health) * (minus_health + 1), p_ptr->cut_attacker);
		(void)set_cut(Ind, p_ptr->cut - adjust * (minus_health + 1), p_ptr->cut_attacker);
	}

	/* Temporary blessing of luck */
	if (p_ptr->bless_temp_luck) (void)bless_temp_luck(Ind, p_ptr->bless_temp_luck_power, p_ptr->bless_temp_luck - 1);

	/* Temporary auras */
	if (p_ptr->sh_fire_tim) (void)set_sh_fire_tim(Ind, p_ptr->sh_fire_tim - minus_magic);
	if (p_ptr->sh_cold_tim) (void)set_sh_cold_tim(Ind, p_ptr->sh_cold_tim - minus_magic);
	if (p_ptr->sh_elec_tim) (void)set_sh_elec_tim(Ind, p_ptr->sh_elec_tim - minus_magic);

	/* Still possible effects from another player's support spell on this player? */
	if (p_ptr->support_timer) {
		p_ptr->support_timer--;
		if (!p_ptr->support_timer) p_ptr->supp = p_ptr->supp_top = 0;
	}

#if POLY_RING_METHOD == 0
	/* Check polymorph rings with timeouts */
	for (i = INVEN_LEFT; i <= INVEN_RIGHT; i++) {
		o_ptr = &p_ptr->inventory[i];
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH) && (o_ptr->timeout_magic > 0) &&
		    (p_ptr->body_monster == o_ptr->pval)) {
			/* Decrease life-span */
			o_ptr->timeout_magic--;
 #ifndef LIVE_TIMEOUTS
			/* Hack -- notice interesting energy steps */
			if ((o_ptr->timeout_magic < 100) || (!(o_ptr->timeout_magic % 100)))
 #else
			if (p_ptr->live_timeouts_magic || (o_ptr->timeout_magic < 100) || (!(o_ptr->timeout_magic % 100)))
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
		    && !(o_ptr->sval == SV_LITE_FEANOR) && (o_ptr->pval > 0) && (!o_ptr->name1))
#endif

			/* Extract the item flags */
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		/* Hack -- Use some fuel */
		if ((f4 & TR4_FUEL_LITE) && (o_ptr->timeout > 0)) {
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
						//s_printf("warning_lite_refill: %s\n", p_ptr->name);
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
				msg_print(Ind, "Your light is growing faint.");
			}
		}
	}

	/* Calculate torch radius */
	//p_ptr->update |= (PU_TORCH|PU_BONUS);

	/* Silyl fun stuff >:) - C. Blue */
	if (p_ptr->corner_turn) {
		p_ptr->corner_turn -= (p_ptr->corner_turn > 2 ? 2 : p_ptr->corner_turn);
		if (p_ptr->corner_turn > 25) {
			p_ptr->corner_turn = 0;
			disturb(Ind, 0, 0);
			msg_print(Ind, "\377oYour head feels dizzy!");
			msg_format_near(Ind, "%s looks dizzy!", p_ptr->name);
			set_confused(Ind, 10 + rand_int(5));
			if (magik(50)) {
				msg_print(Ind, "\377oYou vomit!");
				msg_format_near(Ind, "%s vomits!", p_ptr->name);
				take_hit(Ind, 1, "circulation collapse", 0);
				if (!p_ptr->suscep_life) {
					if (p_ptr->chp < p_ptr->mhp) /* *invincibility* fix */
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
	//if (p_ptr->drain_exp && (p_ptr->wpos.wz != 0) && magik(30 - (60 / (p_ptr->drain_exp + 2))))
/* experimental: maybe no expdrain or just less expdrain while player is on world surface.
   idea: allow classes who lack a *remove curse* spell to make more use of the rings. */
	//if (p_ptr->drain_exp && (p_ptr->wpos.wz != 0) && magik(30 - (60 / (p_ptr->drain_exp + 2))))
	//if (p_ptr->drain_exp && magik(p_ptr->wpos.wz != 0 ? 50 : 0) && magik(30 - (60 / (p_ptr->drain_exp + 2))))
	//if (p_ptr->drain_exp && magik(p_ptr->wpos.wz != 0 ? 50 : (istown(&p_ptr->wpos) ? 0 : 25)) && magik(30 - (60 / (p_ptr->drain_exp + 2))))
	/* changing above line to use istownarea() so you can sort your houses without drain */
	if (p_ptr->drain_exp && magik((
	    p_ptr->wpos.wz != 0 ? (isdungeontown(&p_ptr->wpos) ? 0 : 50) :
	    (istownarea(&p_ptr->wpos, MAX_TOWNAREA) ? 0 : 25)) / (p_ptr->prace == RACE_VAMPIRE ? 2 : 1)
	    ) && magik(30 - (60 / (p_ptr->drain_exp + 2))))
		//take_xp_hit(Ind, 1 + p_ptr->lev / 5 + p_ptr->max_exp / 50000L, "Draining", TRUE, FALSE, FALSE);
		/* Moltor is right, exp drain was too weak for up to quite high levels. Need to make a new formula.. */
	{
		long exploss = 1 + p_ptr->lev + p_ptr->max_exp / 2000L;

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
		if (exploss > 0) take_xp_hit(Ind, exploss, "Draining", TRUE, FALSE, FALSE);
	}

#if 0
	{
		if ((rand_int(100) < 10) && (p_ptr->exp > 0)) {
			p_ptr->exp--;
			p_ptr->max_exp--;
			check_experience(Ind);
		}
	}
#endif	// 0

	/* Now implemented here too ;) - C. Blue */
	/* let's say TY_CURSE lowers stats (occurs often) */
	if (p_ptr->ty_curse &&
	    (rand_int(p_ptr->wpos.wz != 0 ? 200 : (istown(&p_ptr->wpos) || isdungeontown(&p_ptr->wpos) ? 0 : 500)) == 1) &&
	    (get_skill(p_ptr, SKILL_HSUPPORT) < 40)) {
		if (magik(105 - p_ptr->skill_sav)) {
			msg_print(Ind, "An ancient foul curse touches you but you resist!");
		} else {
			msg_print(Ind, "An ancient foul curse shakes your body!");
#if 0
			if (rand_int(2))
			(void)do_dec_stat(Ind, rand_int(6), STAT_DEC_NORMAL);
			else if (!p_ptr->keep_life) lose_exp(Ind, (p_ptr->exp / 100) * MON_DRAIN_LIFE);
			/* take_xp_hit(Ind, 1 + p_ptr->lev / 5 + p_ptr->max_exp / 50000L, "an ancient foul curse", TRUE, TRUE, TRUE); */
#else
			(void)do_dec_stat(Ind, rand_int(6), STAT_DEC_NORMAL);
#endif
		}
	}
	/* and DG_CURSE randomly summons a monster (non-unique) */
	if (p_ptr->dg_curse && (rand_int(300) == 0) && !istown(&p_ptr->wpos) && !isdungeontown(&p_ptr->wpos) &&
	    (get_skill(p_ptr, SKILL_HSUPPORT) < 40)) {
		int anti_Ind = world_check_antimagic(Ind);

		if (anti_Ind) {
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
			if (summon_specific(&p_ptr->wpos, p_ptr->py, p_ptr->px, p_ptr->lev + 20, 100, SUMMON_MONSTER, 0, 0))
				sound_near_site(p_ptr->py, p_ptr->px, &p_ptr->wpos, 0, "summon", NULL, SFX_TYPE_MISC, FALSE);
#else
			(void)summon_specific(&p_ptr->wpos, p_ptr->py, p_ptr->px, p_ptr->lev + 20, 100, SUMMON_MONSTER, 0, 0);
#endif
		}
	}

	/* Handle experience draining.  In Oangband, the effect
	 * is worse, especially for high-level characters.
	 * As per Tolkien, hobbits are resistant.
	 */
	if ((p_ptr->black_breath || p_ptr->black_breath_tmp) &&
	    rand_int((get_skill(p_ptr, SKILL_HCURING) >= 50) ? 250 : 150) < (p_ptr->prace == RACE_HOBBIT || p_ptr->suscep_life ? 2 : 5)) {
		(void)do_dec_stat_time(Ind, rand_int(6), STAT_DEC_NORMAL, 25, 0, TRUE);
		take_xp_hit(Ind, 1 + p_ptr->lev * 3 + p_ptr->max_exp / 5000L,
		    "Black Breath", TRUE, TRUE, TRUE);
	}

	/* Drain Mana */
	if (p_ptr->drain_mana && p_ptr->csp) {
		p_ptr->csp -= p_ptr->drain_mana;
		if (magik(30)) p_ptr->csp -= p_ptr->drain_mana;

		if (p_ptr->csp < 0) p_ptr->csp = 0;

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

	/* Note changes */
	j = 0;

	/* Process inventory (blood potions, snowballs) */
	for (i = 0; i < INVEN_PACK; i++) {
		/* Get the object */
		o_ptr = &p_ptr->inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* SV_POTION_BLOOD going bad */
		if ((o_ptr->tval == TV_POTION || o_ptr->tval == TV_FOOD) && o_ptr->timeout) {
			o_ptr->timeout--;
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

		/* SV_SNOWBALL melting */
		if (o_ptr->tval == TV_GAME && o_ptr->pval && !cold_place(&p_ptr->wpos)) {
			o_ptr->pval--;
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
	for (i = 0; i < INVEN_PACK; i++) {
		o_ptr = &p_ptr->inventory[i];

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
					msg_format(Ind, "Your %s (%c) ha%s finished charging.", o_name, index_to_label(i), o_ptr->number == 1 ? "s" : "ve");
				}
				j++;
			}
		}
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

	if (p_ptr->tim_blacklist) {// && !p_ptr->afk) {
		/* Count down towards turnout */
		p_ptr->tim_blacklist--;
	}

#if 1 /* use turns instead of day/night cycles */
	if (p_ptr->tim_watchlist) {
		/* Count down towards turnout */
		p_ptr->tim_watchlist--;
	}
#endif

	if (p_ptr->pstealing) {
		/* Count down towards turnout */
		p_ptr->pstealing--;
		//if (!p_ptr->pstealing) msg_print(Ind, "You're calm enough to steal from another player.");
		if (!p_ptr->pstealing) msg_print(Ind, "You're calm enough to attempt to steal something.");
	}

	/* Delayed Word-of-Recall */
	if (p_ptr->word_recall) {
#ifdef ANTI_TELE_CHEEZE
		if (p_ptr->anti_tele) {
			msg_print(Ind, "\377oA tension leaves the air around you...");
			p_ptr->word_recall = 0;
			if (p_ptr->disturb_state) disturb(Ind, 0, 0);
			/* Redraw the depth(colour) */
			p_ptr->redraw |= (PR_DEPTH);
			handle_stuff(Ind);
		} else
#endif
		{
			/* Count down towards recall */
			p_ptr->word_recall--;

			/* MEGA HACK: no recall if icky, or in a shop */
			if (!p_ptr->word_recall) {
				if (((p_ptr->anti_tele ||
				    (check_st_anchor(&p_ptr->wpos, p_ptr->py, p_ptr->px) && !p_ptr->admin_dm))
				     && !(l_ptr && (l_ptr->flags2 & LF2_NO_TELE))) ||
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
			msg_print(Ind, "You lose your wraith powers.");
			p_ptr->redraw |= PR_BPR_WRAITH;
			msg_format_near(Ind, "%s loses %s wraith powers.", p_ptr->name, p_ptr->male ? "his":"her");
		}
		/* No wraithform on NO_MAGIC levels - C. Blue */
		else if (p_ptr->wpos.wz && l_ptr && (l_ptr->flags1 & LF1_NO_MAGIC)) {
			p_ptr->tim_wraith = 0;
			msg_print(Ind, "You lose your wraith powers.");
			p_ptr->redraw |= PR_BPR_WRAITH;
			msg_format_near(Ind, "%s loses %s wraith powers.", p_ptr->name, p_ptr->male ? "his":"her");
		}
	}

	return (TRUE);
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
		switch(gametype){
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
						drop_near(0, &tmp_obj, -1, &p_ptr->wpos, oy, ox);
						printf("dropping at %d %d (%d)\n", ox, oy, try);
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

	/* count turns online, afk, and idle */
	p_ptr->turns_online++;
	if (p_ptr->afk) p_ptr->turns_afk++;
	if (p_ptr->idle) p_ptr->turns_idle++;
	else if (!p_ptr->afk) p_ptr->turns_active++;

	/* count how long they stay on a level (for EXTRA_LEVEL_FEELINGS) */
	p_ptr->turns_on_floor++;
	/* hack to indicate it to the player */
	if (p_ptr->turns_on_floor == TURNS_FOR_EXTRA_FEELING) Send_depth(Ind, &p_ptr->wpos);

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

	/* Mind Fusion/Control disable the char */
	if (p_ptr->esp_link && p_ptr->esp_link_type && (p_ptr->esp_link_flags & LINKF_OBJ)) return;


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
#if 1 /* old way - get the usual 'double initial attack' in. \
	 Drawback: Have to wait for nearly a full turn (1-(1/attacksperround)) \
	 for FTK/meleeret to break out for performing a different action. -- this should be fixed now. May break out in 1/attacksperround now.) */
		if (p_ptr->energy >= energy) {
#else /* new way - allows to instantly break out and perform another action (quaff/read) \
	but doesn't give the 'double initial attack' anymore, just a normal, single attack. \
	Main drawback: Walking into a mob will not smoothly transgress into auto-ret the next turn, \
	but wait for an extra turn before it begins, ie taking 2 turns until first attack got in. */
		if (p_ptr->energy >= energy * 2 - 1) {
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

					if (p_ptr->shoot_till_kill_spell) {
						cast_school_spell(Ind, p_ptr->shoot_till_kill_book, p_ptr->shoot_till_kill_spell - 1, 5, -1, 0);
						if (!p_ptr->shooting_till_kill) p_ptr->shoot_till_kill_spell = 0;
					} else if (p_ptr->shoot_till_kill_rcraft) {
						(void)cast_rune_spell(Ind, 5, p_ptr->FTK_e_flags, p_ptr->FTK_m_flags, 0, 1);
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
				    && (attackstatus = auto_retaliate(Ind))) /* attackstatus seems to be unused! */
				{
					p_ptr->auto_retaliating = TRUE;
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
		} else {
			p_ptr->auto_retaliating = FALSE; /* if no energy left, this is required to turn off the no-run-while-retaliate-hack */
		}
	}


	/* ('Handle running' from above was originally at this place) */
	/* Handle running -- 5 times the speed of walking */
	while (p_ptr->running && p_ptr->energy >= (level_speed(&p_ptr->wpos) * (real_speed + 1)) / real_speed) {
		char consume_full_energy;
		run_step(Ind, 0, &consume_full_energy);
		if (consume_full_energy) {
			/* Consume a full turn of energy in case we have e.g. attacked a monster */
			p_ptr->energy -= level_speed(&p_ptr->wpos);
		} else {
			p_ptr->energy -= level_speed(&p_ptr->wpos) / real_speed;
		}
	}


	/* Notice stuff */
	if (p_ptr->notice) notice_stuff(Ind);

	/* XXX XXX XXX Pack Overflow */
	pack_overflow(Ind);

	process_games(Ind);

	/* Added for Holy Martyr, which was previously in process_player_end_aux():
	   Process it independantly of level speed, in real time instead.
	   Otherwise it gives the player too much action time on deep levels at high speeds. */
	if (!(turn % cfg.fps)) {
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
	}

	/* Process things such as regeneration. */
	/* This used to be processed every 10 turns, but I am changing it to be
	 * processed once every 5/6 of a "dungeon turn". This will make healing
	 * and poison faster with respect to real time < 1750 feet and slower >
	 * 1750 feet. - C. Blue
	 */
	if (!(turn % (level_speed(&p_ptr->wpos) / 120)))
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

	/* Hack -- towns are static for good? too spammy */
	//if (istown(wpos)) return FALSE;
	/* Hack -- make dungeon towns static though? too cheezy */
	//if (isdungeontown(wpos)) return FALSE;

	/* Hack: In IDDC, all floors are stale for 2 minutes to allow logging back in if
	         someone's connection dropped without the floor going stale right away,
	         and towns to allow for easy item transfers.
	         Small drawback: If someone logs back on after having taken a break from IDDC
	         and he finds himself in a non-accessible area, he'll have to wait for
	         2 minutes instead of the usual 10 seconds till the floor regens. */
	if (in_irondeepdive(wpos) && grace < 120) grace = 120;

	now = time(&now);
	if (wpos->wz) {
		struct dungeon_type *d_ptr;
		struct dun_level *l_ptr;
		d_ptr = getdungeon(wpos);
		if (!d_ptr) return FALSE;
		l_ptr = &d_ptr->level[ABS(wpos->wz) - 1];
#if DEBUG_LEVEL > 3
		s_printf("%s  now:%ld last:%ld diff:%ld grace:%d players:%d\n", wpos_format(0, wpos), now, l_ptr->lastused, now-l_ptr->lastused,grace, players_on_depth(wpos));
#endif
		/* Hacky: Combine checks for normal death/quit static time (lastused) and for anti-scum static time (creationtime):
		   lastused is set by death/quit staticing, but it is 0 when stair-scumming. */
		if (l_ptr->lastused) {
			if (now - l_ptr->lastused > grace) return TRUE;
		} else if (now - l_ptr->creationtime > grace) return TRUE;
	} else if (now - wild_info[wpos->wy][wpos->wx].lastused > grace) {
#if 0
		/* Never allow dealloc where there are houses */
		/* For now at least */
		int i;

		for (i = 0; i < num_houses; i++) {
			if (inarea(wpos, &houses[i].wpos)) {
				if (!(houses[i].flags & HF_DELETED)) return FALSE;
			}
		}
#endif
		return TRUE;
	}
	return FALSE;
}

static void do_unstat(struct worldpos *wpos, bool fast_unstat) {
	int j;

	/* Highlander Tournament sector00 is static while players are in dungeon! */
	if (in_sector00(wpos) && sector00separation) return;

	/* Arena Monster Challenge */
	if (ge_special_sector && in_arena(wpos)) return;

	// Anyone on this depth?
	for (j = 1; j <= NumPlayers; j++)
		if (inarea(&Players[j]->wpos, wpos)) return;

	// If this level is static and no one is actually on it
	//if (stale_level(wpos)) {
		/* limit static time in Ironman Deep Dive Challenge a lot */
		if (in_irondeepdive(wpos)) {
			if (isdungeontown(wpos)) {
				if (stale_level(wpos, 300)) new_players_on_depth(wpos, 0, FALSE);//5 min
			} else if ((getlevel(wpos) < cfg.min_unstatic_level) && (0 < cfg.min_unstatic_level)) {
				/* still 2 minutes static for very shallow levels */
				if (stale_level(wpos, 120)) new_players_on_depth(wpos, 0, FALSE);//2 min
			} else if (stale_level(wpos, 300)) new_players_on_depth(wpos, 0, FALSE);//5 min (was 10)
		} else {
#ifdef SAURON_FLOOR_FAST_UNSTAT
			if (fast_unstat) j = 60 * 60; //1h
			else
#endif
			j = cfg.level_unstatic_chance * getlevel(wpos) * 60;

			/* makes levels between 50ft and min_unstatic_level unstatic on player saving/quiting game/leaving level DEG */
			if (((getlevel(wpos) < cfg.min_unstatic_level) && (0 < cfg.min_unstatic_level)) ||
			    stale_level(wpos, j))
				new_players_on_depth(wpos, 0, FALSE);
		}
	//}
}

/*
 * 24 hourly scan of houses - should the odd house be owned by
 * a non player. Hopefully never, but best to save admin work.
 */
static void scan_houses() {
	int i;
	//int lval;
	s_printf("Doing house maintenance\n");
	for(i = 0; i < num_houses; i++) {
		if(!houses[i].dna->owner) continue;
		switch(houses[i].dna->owner_type) {
			case OT_PLAYER:
				if(!lookup_player_name(houses[i].dna->owner)) {
					s_printf("Found old player houses. ID: %d\n", houses[i].dna->owner);
					kill_houses(houses[i].dna->owner, OT_PLAYER);
				}
				break;
			case OT_PARTY:
				if(!strlen(parties[houses[i].dna->owner].name)) {
					s_printf("Found old party houses. ID: %d\n", houses[i].dna->owner);
					kill_houses(houses[i].dna->owner, OT_PARTY);
				}
				break;
			case OT_GUILD:
				if(!strlen(guilds[houses[i].dna->owner].name)) {
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

				if (!players_on_depth(&twpos) && !istown(&twpos) &&
				    getcave(&twpos) && stale_level(&twpos, cfg.anti_scum))
					dealloc_dungeon_level(&twpos);

				if (w_ptr->flags & WILD_F_UP) {
					d_ptr = w_ptr->tower;
					for (i = 1; i <= d_ptr->maxdepth; i++) {
						twpos.wz = i;
						if (cfg.level_unstatic_chance > 0 &&
						    players_on_depth(&twpos))
							do_unstat(&twpos, d_ptr->type == DI_MT_DOOM && i == d_ptr->maxdepth);

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
							do_unstat(&twpos, d_ptr->type == DI_MT_DOOM && i == d_ptr->maxdepth);

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
 * TODO: Check for OT_GUILD (or guild will be mere den of cheeze)
 */
void cheeze(object_type *o_ptr){
#if CHEEZELOG_LEVEL > 3
	int j;
	/* check for inside a house */
	for(j = 0; j < num_houses; j++){
		if(inarea(&houses[j].wpos, &o_ptr->wpos)){
			if(fill_house(&houses[j], FILL_OBJECT, o_ptr)){
				if(houses[j].dna->owner_type == OT_PLAYER){
					if(o_ptr->owner != houses[j].dna->owner){
						if(o_ptr->level > lookup_player_level(houses[j].dna->owner))
							s_printf("Suspicious item: (%d,%d) Owned by %s, in %s's house. (%d,%d)\n", o_ptr->wpos.wx, o_ptr->wpos.wy, lookup_player_name(o_ptr->owner), lookup_player_name(houses[j].dna->owner), o_ptr->level, lookup_player_level(houses[j].dna->owner));
					}
				}
				else if(houses[j].dna->owner_type == OT_PARTY){
					int owner;
					if((owner = lookup_player_id(parties[houses[j].dna->owner].owner))){
						if(o_ptr->owner != owner){
							if(o_ptr->level > lookup_player_level(owner))
								s_printf("Suspicious item: (%d,%d) Owned by %s, in %s party house. (%d,%d)\n", o_ptr->wpos.wx, o_ptr->wpos.wy, lookup_player_name(o_ptr->owner), parties[houses[j].dna->owner].name, o_ptr->level, lookup_player_level(owner));
						}
					}
				}
				else if(houses[j].dna->owner_type == OT_GUILD){
					int owner;
					if((owner = guilds[houses[j].dna->owner].master)){
						if(o_ptr->owner != owner){
							if(o_ptr->level > lookup_player_level(owner))
								s_printf("Suspicious item: (%d,%d) Owned by %s, in %s party house. (%d,%d)\n", o_ptr->wpos.wx, o_ptr->wpos.wy, lookup_player_name(o_ptr->owner), guilds[houses[j].dna->owner].name, o_ptr->level, lookup_player_level(owner));
						}
					}
				}
				break;
			}
		}
	}
#endif // CHEEZELOG_LEVEL > 3
}


/* Traditional (Vanilla) houses version of cheeze()	- Jir - */
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
void house_contents_chmod(object_type *o_ptr){
#if CHEEZELOG_LEVEL > 3
	int j;

	/* check for inside a house */
	for (j = 0; j < num_houses; j++){
		if (inarea(&houses[j].wpos, &o_ptr->wpos)){
			if (fill_house(&houses[j], FILL_OBJECT, o_ptr)){
				if (houses[j].dna->owner_type == OT_PLAYER)
					o_ptr->mode = lookup_player_mode(houses[j].dna->owner));
				else if (houses[j].dna->owner_type == OT_PARTY) {
					int owner;

					if ((owner = lookup_player_id(parties[houses[j].dna->owner].owner)))
						o_ptr->mode = lookup_player_mode(owner));
				}
				break;
			}
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
 * game objects on the surface of the world. Objects which
 * are not owned will be ignored. Owned items will be checked.
 * If they are in a house, they will be compared against the
 * house owner. If this fails, the level difference will be
 * used to calculate some chance for the object's deletion.
 *
 * In the case for non house objects, essentially a TTL is
 * set. This protects them from instant loss, should a player
 * drop something just at the wrong time, but it should get
 * rid of some town junk. Artifacts should probably resist
 * this clearing.
 */
/*
 * TODO:
 * - this function should handle items in 'traditional' houses too
 * - maybe rename this function (scan_objects and scan_objs...)
 */
static void scan_objs() {
	int i, cnt = 0, dcnt = 0;
	bool sj;
	object_type *o_ptr;
	cave_type **zcave;

	/* objects time-outing disabled? */
	if (!cfg.surface_item_removal && !cfg.dungeon_item_removal) return;

	for (i = 0; i < o_max; i++) {
		o_ptr = &o_list[i];
		if (!o_ptr->k_idx) continue;

		sj = FALSE;
		/* not dropped on player death or generated on the floor? (or special stuff) */
		if (o_ptr->marked2 == ITEM_REMOVAL_NEVER ||
		    o_ptr->marked2 == ITEM_REMOVAL_HOUSE)
			continue;

		/* hack for monster trap items */
		/* XXX noisy warning, eh? */ /* This is unsatisfactory. */
		if (!in_bounds_array(o_ptr->iy, o_ptr->ix) &&
		    /* There was an old woman who swallowed a fly... */
		    in_bounds_array(255 - o_ptr->iy, o_ptr->ix)) {
			sj = TRUE;
			o_ptr->iy = 255 - o_ptr->iy;
		}

		/* Make town Inns a safe place to store (read: cheeze) items,
		   at least as long as the town level is allocated. - C. Blue */
		if ((zcave = getcave(&o_ptr->wpos))
		    && in_bounds_array(o_ptr->iy, o_ptr->ix) //paranoia or monster trap? or tradhouse?
		    && (f_info[zcave[o_ptr->iy][o_ptr->ix].feat].flags1 & FF1_PROTECTED))
			continue;

		/* check items on the world's surface */
		if (!o_ptr->wpos.wz && cfg.surface_item_removal) {
			if (in_bounds_array(o_ptr->iy, o_ptr->ix)) { //paranoia or monster trap? or tradhouse?
				/* Artifacts and objects that were inscribed and dropped by
				the dungeon master or by unique monsters on their death
				stay n times as long as cfg.surface_item_removal specifies */
				if (o_ptr->marked2 == ITEM_REMOVAL_QUICK) {
					if (++o_ptr->marked >= 10) {
						/* handle monster trap, if this item was part of one */
						if (sj) {
							erase_mon_trap(&o_ptr->wpos, o_ptr->iy, o_ptr->ix);
							sj = FALSE;
						} else

						/* normal object */
						delete_object_idx(i, TRUE);

						dcnt++;
					}
				} else if (o_ptr->marked2 == ITEM_REMOVAL_MONTRAP) {
					if (++o_ptr->marked >= 120) {
						/* handle monster trap, if this item was part of one */
						if (sj) {
							erase_mon_trap(&o_ptr->wpos, o_ptr->iy, o_ptr->ix);
							sj = FALSE;
						} else

						/* normal object */
						delete_object_idx(i, TRUE);

						dcnt++;
					}
				} else if (++o_ptr->marked >= ((like_artifact_p(o_ptr) || /* stormy too */
				    (o_ptr->note && !o_ptr->owner))?
				    cfg.surface_item_removal * 3 : cfg.surface_item_removal)
				    + (o_ptr->marked2 == ITEM_REMOVAL_DEATH_WILD ? cfg.death_wild_item_removal : 0)
				    + (o_ptr->marked2 == ITEM_REMOVAL_LONG_WILD ? cfg.long_wild_item_removal : 0)
				    ) {
					/* handle monster trap, if this item was part of one */
					if (sj) {
						erase_mon_trap(&o_ptr->wpos, o_ptr->iy, o_ptr->ix);
						sj = FALSE;
					} else

					/* normal object */
					delete_object_idx(i, TRUE);

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
			if (in_bounds_array(o_ptr->iy, o_ptr->ix)) { //paranoia or monster trap? or tradhouse?
				/* Artifacts and objects that were inscribed and dropped by
				the dungeon master or by unique monsters on their death
				stay n times as long as cfg.surface_item_removal specifies */
				if (++o_ptr->marked >= ((artifact_p(o_ptr) ||
				    (o_ptr->note && !o_ptr->owner)) ?
				    cfg.dungeon_item_removal * 3 : cfg.dungeon_item_removal)) {
					/* handle monster trap, if this item was part of one */
					if (sj) {
						erase_mon_trap(&o_ptr->wpos, o_ptr->iy, o_ptr->ix);
						sj = FALSE;
					} else

					/* normal object */
					delete_object_idx(i, TRUE);

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

		/* restore monster trap hack */
		if (sj) o_ptr->iy = 255 - o_ptr->iy; /* mega-hack: never inbounds */
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
/* called only every 10 turns
*/
/* WARNING: Every if-check in here that tests for turns % cfg.fps
   should instead check for turns % cfg.fps*10, because this function
   is only called every 10 turns as stated above, otherwise
   depending on cfg.fps it might be skipped sometimes, which may or
   may not be critical depending on what it does! - C. Blue */
static void process_various(void) {
	int i, j;
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

	if (!(turn % 2)) {
		do_xfers();	/* handle filetransfers per second
				 * FOR NOW!!! DO NOT TOUCH/CHANGE!!!
				 */
	}

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
		scan_players();
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

#if 0 /* disable for now - mikaelh */
	if (!(turn % (cfg.fps * 50))) {
		/* Tell the scripts that we're alive */
		update_check_file();
	}
#endif

	/* Things handled once per hour */
	if (!(turn % (cfg.fps * 3600))) {
		/* Purge leaderless guilds after a while */
		for (i = 0; i < MAX_GUILDS; i++) {
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
				int k, m_idx;

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
					if (m_ptr->r_idx == RI_PUMPKIN1 || m_ptr->r_idx == RI_PUMPKIN2 || m_ptr->r_idx == RI_PUMPKIN3) {
						msg_print_near_monster(m_idx, "\377oThe Great Pumpkin wails and suddenly vanishes into thin air!");
						s_printf("HALLOWEEN: The Great Pumpkin despawned.\n");
						delete_monster_idx(k, TRUE);
						//note_spot_depth(&p_ptr->wpos, y, x);
						great_pumpkin_timer = rand_int(2); /* fast respawn if not killed! */
					}
				}
			}
			else if (great_pumpkin_duration <= 5) {
				monster_type *m_ptr;
				int k, m_idx;

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
					if (m_ptr->r_idx == RI_PUMPKIN1 || m_ptr->r_idx == RI_PUMPKIN2 || m_ptr->r_idx == RI_PUMPKIN3) {
						msg_print_near_monster(m_idx, "\377oThe Great Pumpkin wails and seems to fade..");
						break;
					}
				}
			}
		}

		if (season_xmas) { /* XMAS */
			if (santa_claus_timer > 0) santa_claus_timer--;
			if (santa_claus_timer == 0) {
				struct worldpos wpos = {cfg.town_x, cfg.town_y, 0};
				cave_type **zcave = getcave(&wpos);
				if (zcave) { /* anyone in town? */
					int x, y, tries = 50;

					/* Try nine locations */
					while (--tries) {
						/* Pick location nearby hard-coded town centre */
						scatter(&wpos, &y, &x, 34, 96, 10, 0);

						/* Require "empty" floor grids */
						if (!cave_empty_bold(zcave, y, x)) continue;

						if (place_monster_aux(&wpos, y, x, RI_SANTA2, FALSE, FALSE, 0, 0) == 0) {
							s_printf("%s XMAS: Generated Santa Claus.\n", showtime());
							santa_claus_timer = -1; /* put generation on hold */
							break;
						}
					}
					if (!tries) santa_claus_timer = 1; /* fast respawn, probably paranoia */
				}
			}
		}

		for (i = 1; i <= NumPlayers; i++) {
			p_ptr = Players[i];

			/* Update the player retirement timers */
			// If our retirement timer is set
			if (p_ptr->retire_timer > 0) {
				int k = p_ptr->retire_timer;

				// Decrement our retire timer
				j = k - 1;

				// Alert him
				if (j <= 60) {
					msg_format(i, "\377rYou have %d minute%s of tenure left.", j, j > 1 ? "s" : "");
				}
				else if (j <= 1440 && !(k % 60)) {
					msg_format(i, "\377yYou have %d hours of tenure left.", j / 60);
				}
				else if (!(k % 1440)) {
					msg_format(i, "\377GYou have %d days of tenure left.", j / 1440);
				}


				// If the timer runs out, forcibly retire
				// this character.
				if (!j) do_cmd_suicide(i);
			}

			/* reduce warning_rest cooldown */
			if (p_ptr->warning_rest_cooldown) p_ptr->warning_rest_cooldown--;
		}

		/* Reswpan for kings' joy  -Jir- */
		/* Update the unique respawn timers */
		/* I moved this out of the loop above so this may need some
		 * tuning now - mikaelh */
		for (j = 1; j <= NumPlayers; j++) {
			p_ptr = Players[j];
			if (!p_ptr->total_winner) continue;
			if (istownarea(&p_ptr->wpos, MAX_TOWNAREA)) continue; /* allow kings idling instead of having to switch chars */

			/* Hack -- never Maggot and his dogs :) (also includes joke monsters Martti Ihrasaari and The Greater hell-beast) */
			i = rand_range(57, max_r_idx - 1);
			r_ptr = &r_info[i];

			/* Make sure we are looking at a dead unique */
			if (!(r_ptr->flags1 & RF1_UNIQUE)) {
				j--;
				continue;
			}

			if (p_ptr->r_killed[i] != 1) continue;

			/* Hack -- Sauron and Morgoth are exceptions (and all > Morgy-uniques)
			   --- QUESTOR is currently NOT used!! - C. Blue */
			if (r_ptr->flags1 & RF1_QUESTOR) continue;
			/* ..hardcoding them instead: */
			if (r_ptr->level >= 98) continue; /* Not Michael either */
			/* Redundant, since he's a dungeon boss, but anyway: (He's linked to Sauron) */
			if (i == RI_DOL_GULDUR) continue;

			if (r_ptr->flags7 & RF7_NAZGUL) continue; /* No nazguls */

			/* Dungeon bosses probably shouldn't respawn */
			if (r_ptr->flags0 & RF0_FINAL_GUARDIAN) continue;

			/* Special-dropping uniques too! */
			/* if (r_ptr->flags1 & RF1_DROP_CHOSEN) continue; */

			//				if (r_ptr->max_num > 0) continue;
			if (rand_int(cfg.unique_respawn_time * (r_ptr->level + 1)) > 9)
				continue;

			/* "Ressurect" the unique */
			p_ptr->r_killed[i] = 0;
			Send_unique_monster(j, i);

			/* Tell the player */
			/* the_sandman: added colour */
			msg_format(j,"\374\377v%s rises from the dead!",(r_name + r_ptr->name));
		}

		/* discard reserved character names that exceed their timeout */
		for (i = 0; i < MAX_RESERVED_NAMES; i++) {
			if (!reserved_name_character[i][0]) break;
			if (reserved_name_timeout[i]) {
				reserved_name_timeout[i]--;
				continue;
			}
			for (j = i + 1; j < MAX_RESERVED_NAMES; j++) {
				if (!reserved_name_character[j][0]) {
					reserved_name_character[j - 1][0] = '\0';
					break;
				}
				strcpy(reserved_name_character[j - 1], reserved_name_character[j]);
				strcpy(reserved_name_account[j - 1], reserved_name_account[j]);
				reserved_name_timeout[j - 1] = reserved_name_timeout[j];
			}
			break;
		}
	}

#if 0
	/* Grow trees very occasionally */
	if (!(turn % (10L * GROW_TREE))) {
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

/* Process timed shutdowns, compacting objects/monsters and
   call process_world_player() for each player.
   We are called once every 50 turns. */
static void process_world(void) {
	int i;
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

	if (cfg.runlevel == 2049) {
		shutdown_server();
	} else if (cfg.runlevel == 2052) {
		/* same as /shutdown 0, except server will return -2 instead of -1.
		   can be used by scripts to initiate maintenance downtime etc. */
		set_runlevel(-1);

		/* paranoia - set_runlevel() will call exit() */
		time(&cfg.closetime);
		return;
	} else if (cfg.runlevel == 2051) {
		int n = 0;
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
				if (p_ptr->wpos.wx != WPOS_SECTOR00_X || p_ptr->wpos.wy != WPOS_SECTOR00_Y || !sector00separation) continue;
			}
			break;
		}
		if (!i && (n <= 3)) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	}
#ifdef ENABLE_GO_GAME
	else if (cfg.runlevel == 2048 && !go_game_up) {
#else
	else if (cfg.runlevel == 2048) {
#endif
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

			/* extra, just for /shutempty: Ignore all iddc chars who are afk/idle */
			if (SHUTDOWN_IGNORE_IDDC(p_ptr)) continue;

			/* Ignore characters that are afk and not in a dungeon/tower */
			//if((p_ptr->wpos.wz == 0) && (p_ptr->afk)) continue;

			/* Ignore characters that are not in a dungeon/tower */
			if (p_ptr->wpos.wz == 0) {
				/* Don't interrupt events though */
				if (p_ptr->wpos.wx != WPOS_SECTOR00_X || p_ptr->wpos.wy != WPOS_SECTOR00_Y || !sector00separation) continue;
			}
			break;
		}
		if (!i) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	} else if (cfg.runlevel == 2047) {
		int n = 0;
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
				if (p_ptr->wpos.wx != WPOS_SECTOR00_X || p_ptr->wpos.wy != WPOS_SECTOR00_Y || !sector00separation) continue;
			}
			break;
		}
		if (!i && (n <= 8)) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	} else if (cfg.runlevel == 2046) {
		int n = 0;
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
				if (p_ptr->wpos.wx != WPOS_SECTOR00_X || p_ptr->wpos.wy != WPOS_SECTOR00_Y || !sector00separation) continue;
			}
			break;
		}
		if (!i && (n <= 5)) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	} else if (cfg.runlevel == 2045) {
		int n = 0;
		for (i = NumPlayers; i > 0 ;i--) {
			if (Players[i]->conn == NOT_CONNECTED) continue;
			/* Ignore admins that are loged in */
			if (admin_p(i)) continue;
			/* count players */
			n++;
		}
		if (!n) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	} else if (cfg.runlevel == 2044) {
		int n = 0;
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
				if (p_ptr->wpos.wx != WPOS_SECTOR00_X || p_ptr->wpos.wy != WPOS_SECTOR00_Y || !sector00separation) continue;
			}
			break;
		}
		if (!i && (n <= 6)) {
			msg_broadcast(-1, "\374\377G<<<\377oServer is being updated, but will be up again in no time.\377G>>>");
			cfg.runlevel = 2049;
		}
	}

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
					if (p_ptr->wpos.wx != WPOS_SECTOR00_X || p_ptr->wpos.wy != WPOS_SECTOR00_Y || !sector00separation) continue;
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

	/* Hack -- Compact the object list occasionally */
	if (o_top + 160 > MAX_O_IDX) compact_objects(320, TRUE);

	/* Hack -- Compact the monster list occasionally */
	if (m_top + 320 > MAX_M_IDX) compact_monsters(640, TRUE);

	/* Hack -- Compact the trap list occasionally */
	//if (t_top + 160 > MAX_TR_IDX) compact_traps(320, TRUE);

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
			msg_print(1, "Suddenly a thought comes to your mind:");
			fortune(1, TRUE);
		}
#endif
	}

	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Process the world of that player */
		process_world_player(i);
	}
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
		if (a_info[i].timeout == 0) erase_artifact(i);
		/* just warn? */
 #if defined(IDDC_ARTIFACT_FAST_TIMEOUT) || defined(WINNER_ARTIFACT_FAST_TIMEOUT)
		else if ((double_speed ? a_info[i].timeout / 2 : a_info[i].timeout) == FLUENT_ARTIFACT_WARNING) {
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
				if (lookup_player_id(mail_target[i]))
					/* normal */
 #ifdef MERCHANT_MAIL_INFINITE
					mail_timeout[i] = MERCHANT_MAIL_TIMEOUT;
 #else
				{
					if (erase) mail_timeout[i] = - 2 - MERCHANT_MAIL_TIMEOUT;
					else mail_timeout[i] = MERCHANT_MAIL_TIMEOUT;
				}
 #endif
				else
					/* hack for players who no longer exist */
					mail_timeout[i] = -1;

				/* notify him */
				for (j = 1; j <= NumPlayers; j++) {
					if (!strcmp(Players[j]->accountname, mail_target_acc[i])) {
						if (strcmp(Players[j]->name, mail_target[i])) {
							msg_print(j, "\374\377yThe merchants guild has mail for another character of yours!");
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
 #ifndef MERCHANT_MAIL_INFINITE
								if (erase) msg_print(j, "\374\377yWarning - if you don't pick it up in time, the guild bureau will confiscate it!");
 #endif
 #ifdef USE_SOUND_2010
								sound(j, "store_doorbell_leave", NULL, SFX_TYPE_MISC, FALSE);
 #endif
							}
						}
					}
				}
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

					s_printf("MAIL_ERROR_ERASED (1st bounce).\n");
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


				pid = lookup_player_id(mail_target[i]);
				if (!pid) {
					s_printf("MAIL_ERROR: no pid - Sender %s, Target %s\n", mail_sender[i], mail_target[i]);
					/* special: the mail bounced back to a character that no longer exists!
					   if this happens with the new receipient too, the mail gets deleted,
					   since noone can pick it up anymore. */
					if (erase) {
						s_printf("MAIL_ERROR_ERASED.\n");
						/* delete mail! */
						mail_sender[i][0] = 0;
					}
				} else {
					acc = lookup_accountname(pid);
					if (acc) strcpy(mail_target_acc[i], acc);
					else s_printf("MAIL_ERROR_RETURN: no acc - Sender %s, Target %s\n", mail_sender[i], mail_target[i]);
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
		if (Players[i]->conn == NOT_CONNECTED) return 0;
		if (p_ptr->id == id) return i;
	}

	/* assume none */
	return 0;
}

int find_player_name(char *name) {
	int i;

	for (i = 1; i <= NumPlayers; i++) {
		player_type *p_ptr = Players[i];

		if (!strcmp(p_ptr->name, name)) return i;
	}

	/* assume none */
	return 0;
}

void process_player_change_wpos(int Ind) {
	player_type *p_ptr = Players[Ind];
	worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	//worldpos twpos;
	dun_level *l_ptr;
	int d, j, x, y, startx = 0, starty = 0, m_idx, my, mx, tries, emergency_x, emergency_y, dlv = getlevel(wpos);
	char o_name_short[ONAME_LEN];
	bool smooth_ambient = FALSE, travel_ambient = FALSE;

	/* un-snow */
	p_ptr->dummy_option_8 = FALSE;
	//update_player(Ind); //un-snowing restores lack of visibility by others - required?

	/* IDDC specialties */
	if (in_irondeepdive(&p_ptr->wpos_old)) {
		/* for obtaining statistical IDDC information: */
		if (!is_admin(p_ptr)) {
			s_printf("CVRG-IDDC: '%s' leaves floor %d:\n", p_ptr->name, p_ptr->wpos_old.wz);
			log_floor_coverage(getfloor(&p_ptr->wpos_old), &p_ptr->wpos_old);
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
			j = (p_ptr->IDDC_flags & 0xC) - 1;
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
			cave_set_feat(&p_ptr->wpos_old, p_ptr->voidy, p_ptr->voidx, twall_erosion(&p_ptr->wpos_old, p_ptr->voidy, p_ptr->voidx));
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

		/* allow non-normal (interval-timed) ambient sfx, but depend on our own fast-travel-induced rythm */
		travel_ambient = TRUE;
	} else if (players_on_depth(wpos) == 1) travel_ambient = TRUE; /* exception - if we're the only one here we won't mess up someone else's ambient sfx rythm, so it's ok */

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

	/* Clear the "marked" and "lit" flags for each cave grid */
	for (y = 0; y < MAX_HGT; y++) {
		for (x = 0; x < MAX_WID; x++) {
			p_ptr->cave_flag[y][x] = 0;
		}
	}
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

	wpcopy(&p_ptr->wpos_old, &p_ptr->wpos);

	/* Allow the player again to find another random IDDC town, if he hit a static IDDC town */
	if (is_fixed_irondeepdive_town(&p_ptr->wpos, dlv)) p_ptr->IDDC_found_rndtown = FALSE;
	/* Cover disallowing the same if he enters a random town someone else already generated */
	else if (in_irondeepdive(&p_ptr->wpos) && l_ptr && (l_ptr->flags1 & LF1_RANDOM_TOWN)) p_ptr->IDDC_found_rndtown = TRUE;

	/* hack -- update night/day in wilderness levels */
	if (!wpos->wz) {
		if (IS_DAY) player_day(Ind);
		else player_night(Ind);
	}

	/* Determine starting location */
	switch (p_ptr->new_level_method) {
	/* Climbed down */
	case LEVEL_RECALL_DOWN:
	case LEVEL_DOWN:
		starty = level_down_y(wpos);
		startx = level_down_x(wpos);
		break;

	/* Climbed up */
	case LEVEL_RECALL_UP:
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
			starty = rand_int((l_ptr ? l_ptr->hgt : MAX_HGT)-3)+1;
			startx = rand_int((l_ptr ? l_ptr->wid : MAX_WID)-3)+1;
			if (cave_floor_bold(zcave, starty, startx)) {
				emergency_x = startx;
				emergency_y = starty;
			}
		}
		while (  ((zcave[starty][startx].info & CAVE_ICKY)
			|| (zcave[starty][startx].feat == FEAT_DEEP_WATER)
			|| (zcave[starty][startx].feat == FEAT_SICKBAY_AREA) /* don't recall him into sickbay areas */
			|| (!cave_floor_bold(zcave, starty, startx)))
			&& (++tries < 10000) );
		if (tries == 10000 && emergency_x) {
			startx = emergency_x;
			starty = emergency_y;
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
	if (sector00separation && in_highlander_dun(&p_ptr->wpos) && l_ptr)
		starty = l_ptr->hgt + 1; /* for the code right below :) ("Hack -- handle smaller floors") */

	/* Hack -- handle smaller floors */
	if (l_ptr && (starty > l_ptr->hgt || startx > l_ptr->wid)) {
		/* make sure we aren't in an "icky" location */
		emergency_x = 0; emergency_y = 0; tries = 0;
		do {
			starty = rand_int(l_ptr->hgt - 3) + 1;
			startx = rand_int(l_ptr->wid - 3) + 1;
			if (cave_floor_bold(zcave, starty, startx)) {
				emergency_x = startx;
				emergency_y = starty;
			}
		}
		while (  ((zcave[starty][startx].info & CAVE_ICKY)
			|| (!cave_floor_bold(zcave, starty, startx)))
			&& (++tries < 10000) );
		if (tries == 10000 && emergency_x) {
			startx = emergency_x;
			starty = emergency_y;
		}
	}

	//printf("finding area (%d,%d)\n",startx,starty);
	/* Place the player in an empty space */
	//for (j = 0; j < 1500; ++j)
	for (j = 0; j < 5000; j++) {
		/* Increasing distance */
		d = (j + 149) / 150;

		/* Pick a location */
		scatter(wpos, &y, &x, starty, startx, d, 1);

		/* Must have an "empty" grid */
		if (!cave_empty_bold(zcave, y, x)) continue;

		/* Not allowed to go onto a icky location (house) if Depth <= 0 */
		if (wpos->wz == 0 && !(p_ptr->global_event_temp & PEVF_ICKY_OK)) {
			if((zcave[y][x].info & CAVE_ICKY) && (p_ptr->new_level_method != LEVEL_HOUSE)) continue;
			if(!(zcave[y][x].info & CAVE_ICKY) && (p_ptr->new_level_method == LEVEL_HOUSE)) continue;
		}

		/* Prevent recalling or prob-travelling into no-tele vaults and monster nests! - C. Blue */
		if ((zcave[y][x].info & (CAVE_STCK | CAVE_NEST_PIT)) &&
		    (p_ptr->new_level_method == LEVEL_RECALL_UP || p_ptr->new_level_method == LEVEL_RECALL_DOWN ||
		    p_ptr->new_level_method == LEVEL_RAND || p_ptr->new_level_method == LEVEL_OUTSIDE_RAND ||
		    p_ptr->new_level_method == LEVEL_PROB_TRAVEL)
		    && !(p_ptr->global_event_temp & PEVF_STCK_OK))
			continue;

		/* Prevent landing onto a store entrance */
		if (zcave[y][x].feat == FEAT_SHOP) continue;

		/* for new sickbay area: */
		if (p_ptr->new_level_method == LEVEL_TO_TEMPLE
		    && zcave[y][x].feat != FEAT_SICKBAY_AREA) continue;

		/* Must be inside the level borders - mikaelh */
		if (x < 1 || y < 1 || x > p_ptr->cur_wid - 2 || y > p_ptr->cur_hgt - 2)
			continue;

		/* should somewhat stay away from certain locations? */
		if (p_ptr->avoid_loc)
			for (d = 0; d < p_ptr->avoid_loc; d++)
				if (distance(y, x, p_ptr->avoid_loc_y[d], p_ptr->avoid_loc_x[d]) < 8)
					continue;

		break;
	}
	/* this is required to make sense, isn't it.. */
	if (j == 5000) {
		x = startx;
		y = starty;
	}

#if 0
	/* Place the player in an empty space */
	for (j = 0; j < 1500; ++j) {
		/* Increasing distance */
		d = (j + 149) / 150;

		/* Pick a location */
		scatter(wpos, &y, &x, starty, startx, d, 1);

		/* Must have an "empty" grid */
		if (!cave_empty_bold(zcave, y, x)) continue;

		/* Not allowed to go onto a icky location (house) if Depth <= 0 */
		if ((wpos->wz == 0) && (zcave[y][x].info & CAVE_ICKY))
			break;
	}
#endif /*0*/
	p_ptr->py = y;
	p_ptr->px = x;

	/* Update the player location */
	zcave[y][x].m_idx = 0 - Ind;
	cave_midx_debug(wpos, y, x, -Ind);

	/* Update his golem's location */
	for (m_idx = m_top - 1; m_idx >= 0; m_idx--) {
		monster_type *m_ptr = &m_list[m_fast[m_idx]];
		cave_type **mcave;
		mcave = getcave(&m_ptr->wpos);

		if (!m_fast[m_idx]) continue;

		/* Excise "dead" monsters */
		if (!m_ptr->r_idx) continue;

		if (m_ptr->owner != p_ptr->id) continue;

		if (m_ptr->mind & GOLEM_GUARD && !(m_ptr->mind & GOLEM_FOLLOW)) continue;

		/* XXX XXX XXX (merging) */
		starty = m_ptr->fy;
		startx = m_ptr->fx;
		starty = y;
		startx = x;

		if (!m_ptr->wpos.wz && !(m_ptr->mind & GOLEM_FOLLOW)) continue;
		/*
		   if (m_ptr->mind & GOLEM_GUARD && !(m_ptr->mind&GOLEM_FOLLOW)) continue;
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
		wpcopy(&m_ptr->wpos,wpos);
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

	/* Did we enter/leave a no-run level? Temporarily disable/reenable warning_run */
	if ((l_ptr && (l_ptr->flags2 & LF2_NO_RUN)) || (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_RUN))) {
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
				msg_print(Ind, "\374\377y    Hell Knights should try either a dagger, spear or cleaver.");
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
	    !in_sector00(&p_ptr->wpos) &&
	    (ABS(p_ptr->wpos.wx - 32) >= 2 || ABS(p_ptr->wpos.wy - 32) >= 2)) {
		msg_print(Ind, "\374\377yHINT: You can press '\377oM\377y' or '\377o~0\377y'to browse a worldmap.");
		msg_print(Ind, "\374\377y      Towns, for example Bree, are denoted as yellow 'T'.");
		s_printf("warning_worldmap: %s\n", p_ptr->name);
		p_ptr->warning_worldmap = 1;
	}

	if (!p_ptr->warning_dungeon && p_ptr->wpos.wz == 0 &&
	    !in_sector00(&p_ptr->wpos) &&
	    (ABS(p_ptr->wpos.wx - 32) >= 3 || ABS(p_ptr->wpos.wy - 32) >= 3)) {
		msg_print(Ind, "\374\377yHINT: Consider going to the Training Tower first, to gain some levels.");
		msg_print(Ind, "\374\377y      After that, seek out a dungeon. The tower is located in Bree.");
		s_printf("warning_dungeon: %s\n", p_ptr->name);
		p_ptr->warning_dungeon = 1;
	}

	/* Hack -- jail her/him */
	if (!p_ptr->wpos.wz && p_ptr->tim_susp
#ifdef JAIL_TOWN_AREA
	    && istownarea(&p_ptr->wpos, MAX_TOWNAREA)
#endif
	    )
		imprison(Ind, JAIL_OLD_CRIMES, "old crimes");

	/* daylight problems for vampires */
	if (!p_ptr->wpos.wz && p_ptr->prace == RACE_VAMPIRE) calc_boni(Ind);

	/* moved here, since it simplifies the wpos-changing process and
	   should keep it highly consistent and linear.
	   Note: Basically every case that sets p_ptr->new_level_flag = TRUE
	   can and should be stripped of immediate check_Morgoth() call and
	   instead rely on its occurance here. Also, there should actually be
	   no existing case of check_Morgoth() that went without setting said flag,
	   with the only exception of server join/leave in nserver.c and Morgoth
	   live spawn (ie not on level generation time) in monster2.c. - C. Blue */
	check_Morgoth(Ind);
	if (p_ptr->new_level_flag) return;

#ifdef CLIENT_SIDE_WEATHER 
	/* update his client-side weather */
	player_weather(Ind, TRUE, TRUE, TRUE);
#endif

	/* Brightly lit Arena Monster Challenge */
	if (ge_special_sector && in_arena(&p_ptr->wpos)) {
		wiz_lite(Ind);
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
		wiz_lite(Ind);
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
	if (!smooth_ambient && p_ptr->flash_self >= 0) p_ptr->flash_self = cfg.fps / FLASH_SELF_DIV2; //todo: make client option
#endif

	grid_affects_player(Ind, -1, -1);

	clockin(Ind, 7); /* Remember his wpos */
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
	int i;
	static int export_turns = 1; // Setting this to '1' makes it run on server startup
	player_type *p_ptr;

	/* Return if no one is playing */
	/* if (!NumPlayers) return; */

	/* Check for death.  Go backwards (very important!) */
	for (i = NumPlayers; i > 0; i--) {
		/* Check connection first */
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Check for death */
		if (Players[i]->death)
			player_death(i);
	}

	/* Check player's depth info */
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i]; 
		if (p_ptr->conn == NOT_CONNECTED || !p_ptr->new_level_flag)
			continue;
		if (p_ptr->iron_winner_ded && p_ptr->wpos.wz != 0) {
			p_ptr->new_level_flag = FALSE;
			do_cmd_suicide(i);
			continue;
		}
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
	turn++;

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

#if 0
		//fix:
		st_ptr->last_theft = st_ptr->tim_watch = 0;
		p_ptr->test_turn =
		st_ptr->last_visit =
		p_ptr->item_order_turn =
		p_ptr->last_gold_drop_timer = 0;
		xorders[].turn =
		p_ptr->go_turn = 0;
		ge->start_turn
		ge->end_turn
#endif

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
		}
#endif

		/* Process quest (de)activation every minute */
		process_quests();
	}

	/* Process some things once each second.
	   NOTE: Some of these (global events) mustn't be handled _after_ a player got set to 'dead',
	   or gameplay might in very rare occasions get screwed up (someone dying when he shouldn't) - C. Blue */
	if (!(turn % cfg.fps)) {
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

		/* process certain player-related timers */
		for (i = 1; i <= NumPlayers; i++) {
			/* CTRL+R spam control */
			if (Players[i]->redraw_cooldown) Players[i]->redraw_cooldown--;

			/* Arbitrary max number, just to prevent integer overflow.
			   Should just be higher than the longest interval of any ambient sfx type. */
			if (!Players[i]->wpos.wz && /* <- redundant check sort of, caught in process_player_change_wpos() anyway */
			    Players[i]->ambient_sfx_timer < 200)
				Players[i]->ambient_sfx_timer++;

#ifdef ENABLE_GO_GAME
			/* Kifu email spam control */
			if (Players[i]->go_mail_cooldown) Players[i]->go_mail_cooldown--;
#endif
		}
	}

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
		if (p_ptr->conn == NOT_CONNECTED)
			continue;

		/* Print queued log messages (anti-spam feature) - C. Blue */
		if (p_ptr->last_gold_drop && turn - p_ptr->last_gold_drop_timer >= cfg.fps * 2) {
			s_printf("Gold dropped (%d by %s at %d,%d,%d) [anti-spam].\n", p_ptr->last_gold_drop, p_ptr->name, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			p_ptr->last_gold_drop = 0;
			p_ptr->last_gold_drop_timer = turn;
		}

#if 0
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
#endif

#ifdef ENABLE_SELF_FLASHING
		/* Check for hilite */
		if (p_ptr->flash_self > 0 && !(turn % (cfg.fps / 15))) {
			p_ptr->flash_self--;
			everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
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
				everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
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
	if (!(turn % MONSTER_TURNS))
		process_monsters();
#endif

	/* Process programmable NPCs */
	if (!(turn % NPC_TURNS))
		process_npcs();

	/* Process all of the objects */
	/* Currently, process_objects() only recharges rods on the floor and in trap kits.
	   It must be called as frequently as process_player_end_aux(), which recharges rods in inventory,
	   so they're both recharging at the same rate.
	   Note: the exact timing for each object is measured inside the function. */
	process_objects();

	/* Process the world */
	if (!(turn % 50)) process_world();

	/* Clean up Bree regularly to prevent too dangerous towns in which weaker characters cant move around */
	//if (!(turn % 650000)) { /* 650k ~ 3hours */
	//if (!(turn % (cfg.fps * 3600))) { /* 1 h */
#if 0
	if (!(turn % (cfg.fps * 600))) { /* new timing after function was changed to "thin_surface_spawns()" */
		thin_surface_spawns();
		//spam	s_printf("%s Surface spawns thinned.\n", showtime());
	}
#else
	thin_surface_spawns(); //distributes workload now
#endif

	/* Used to be in process_various(), but changed it to distribute workload over all frames now. */
	purge_old();

	/* Process everything else */
	if (!(turn % 10)) {
		process_various();

		/* Hack -- Regenerate the monsters every hundred game turns */
		if (!(turn % 100)) regen_monsters();

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
	}

#ifdef DUNGEON_VISIT_BONUS
	/* Keep track of frequented dungeons, every minute */
	if (!(turn % (cfg.fps * 60))) process_dungeon_boni();
	/* Decay visit frequency for all dungeons over time, every 10 minutes */
	if (!(turn % (cfg.fps * 60 * 10))) process_dungeon_boni_decay();
#endif

	/* Process day/night changes on world_surface */
	if (!(turn % HOUR)) process_day_and_night();

	/* Refresh everybody's displays */
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		if (p_ptr->conn == NOT_CONNECTED)
			continue;

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
	    /* '/ 10 ' stands for quick motion, ie time * 10 */
	    (!(turn % ((store_debug_mode * (10L * cfg.store_turns)) / store_debug_quickmotion))))
		store_debug_stock();

#ifdef ARCADE_SERVER
	/* Process special Arcade Server things */
	if (turn % (cfg.fps / 3) == 1) exec_lua(1, "firin()");
	if (turn % tron_speed == 1) exec_lua(1, "tron()");
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

	switch(val) {
		case -1:
			/* terminate: return a value that lets a script know that
			   it must not restart us. */
			cfg.runlevel = -1;
			s_printf("***SERVER MAINTENANCE TERMINATION***\n");
		case 0:
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
//			msg_broadcast(0, "\377yWarning. Server shutdown will take place in five minutes.");
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
			/* Shutdown as soon as server is empty (admins don't count) */
			break;
		case 2049:
			/* Usually not called here - just a temporary hack value (see dungeon.c) */
			shutdown_server();
			break;
		case 6:
			/* Running */
		default:
			/* Cancelled shutdown */
			if (cfg.runlevel != 6)
				msg_broadcast(0, "\377GServer shutdown cancelled.");
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
void play_game(bool new_game, bool all_terrains, bool dry_Bree, bool new_wilderness, bool new_flavours, bool new_houses) {
	int h = 0, m = 0, s = 0, dwd = 0, dd = 0, dm = 0, dy = 0;
	time_t now;
	struct tm *tmp;
	//int i, n;

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
		wild_spawn_towns();

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
						for (i = 0; i < d_ptr->maxdepth; i++)
							C_KILL(d_ptr->level[i].uniques_killed, MAX_R_IDX, char);
						C_KILL(d_ptr->level, d_ptr->maxdepth, struct dun_level);
						KILL(d_ptr, struct dungeon_type);
					}
					if (w_ptr->flags & WILD_F_UP) {
						d_ptr = w_ptr->tower;
						for (i = 0; i < d_ptr->maxdepth; i++)
							C_KILL(d_ptr->level[i].uniques_killed, MAX_R_IDX, char);
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
			wild_spawn_towns();

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
		struct worldpos twpos;
		twpos.wx = cfg.town_x;
		twpos.wy = cfg.town_y;
		twpos.wz = 0;
		/* Allocate space for it */
		alloc_dungeon_level(&twpos);

		/* Actually generate the town */
		generate_cave(&twpos, NULL);
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
	scan_players();
	scan_accounts();
	scan_houses();
	
#if defined CLIENT_SIDE_WEATHER && !defined CLIENT_WEATHER_GLOBAL
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

		/* Notify players who are afk and even pseudo-afk rogues */
#if 0 /* always send a page beep? */
		Send_beep(1);
#endif
#if 0 /* only when afk or idle? */
		if (p_ptr->afk) Send_beep(1);
		else if (p_ptr->turns_idle >= cfg.fps * AUTO_AFK_TIMER) Send_beep(1);
#endif
#if 1 /* send a warning sound (usually same as page beep) */
		//sound(1, "warning", "page", SFX_TYPE_NO_OVERLAP, FALSE);
		sound(1, "page", NULL, SFX_TYPE_NO_OVERLAP, FALSE);
#endif
		Net_output1(1);

		/* Indicate cause */
		strcpy(p_ptr->died_from, "server shutdown");

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

	if (cfg.runlevel == -1)
		quit("Terminating");
	else
		quit("Server state saved");
}

void pack_overflow(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* XXX XXX XXX Pack Overflow */
	if (p_ptr->inventory[INVEN_PACK].k_idx) {
		object_type *o_ptr;
		int amt, i, j = 0;
		u32b f1, f2, f3, f4, f5, f6, esp;
		char o_name[ONAME_LEN];

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
		sound_item(Ind, o_ptr->tval, o_ptr->sval, "pickup_");
#endif

		/* Drop it (carefully) near the player */
		drop_near_severe(Ind, o_ptr, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);

		/* Decrease the item, optimize. */
		inven_item_increase(Ind, i, -amt);
		inven_item_optimize(Ind, i);
	}
}

/* Handle special timed events that don't fit in /
   aren't related to 'Global Events' routines - C. Blue
   (To be called every second.)
   Now also used for Go minigame. */
void process_timers() {
	struct worldpos wpos;
	cave_type **zcave;
	int y, x, i;

#ifdef ENABLE_GO_GAME
	/* Process Go AI engine communication (its replies) */
	if (go_game_up) go_engine_clocks();
#endif

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
}

/* during new years eve, cast fireworks! - C. Blue
   NOTE: Called four times per second (if fireworks are enabled). */
static void process_firework_creation() {
	int i;

	if (!fireworks_delay) { /* fire! */
		worldpos wpos;
		switch(rand_int(4)) {
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


#if defined CLIENT_SIDE_WEATHER && defined CLIENT_WEATHER_GLOBAL
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

#if defined CLIENT_WEATHER_GLOBAL || !defined CLIENT_SIDE_WEATHER
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
			cloud_create(i, 32 * MAX_HGT, 32 * MAX_WID);
			cloud_state[i] = 1;
#else
			/* create cloud at random starting x, y world _grid_ coords (!) */
			cloud_create(i,
			    rand_int(MAX_WILD_X * MAX_WID),
			    rand_int(MAX_WILD_Y * MAX_HGT));
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
					w_ptr->weather_type = 1; /* never snow in desert */
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
				    (w_ptr->weather_type == 2 && (!w_ptr->weather_wind || w_ptr->weather_wind >= 3)) ?
				    5 : 8;
				w_ptr->weather_speed =
#if 1 /* correct! (this is a different principle than 'weather_intensity' above) */
				    (w_ptr->weather_type == 2) ? WEATHER_SNOW_MULT * WEATHER_GEN_TICKS :
				    /* hack: for non-windy rainfall, accelerate raindrop falling speed by 1: */
				    (w_ptr->weather_wind ? 1 * WEATHER_GEN_TICKS : WEATHER_GEN_TICKS - 1);
#else /* just for testing stuff */
				    (w_ptr->weather_type == 2 && (!w_ptr->weather_wind || w_ptr->weather_wind >= 3)) ?
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
   This does not check whether there will be too many clouds! */
void cloud_create(int i, int cx, int cy) {
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
	int thunderstorm, thunderclap = 999, vol = rand_int(86);
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
			sound_vol(i, "thunder", NULL, SFX_TYPE_WEATHER, FALSE, 15 + (vol + Players[i]->wpos.wy + Players[i]->wpos.wx) % 86);
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
#if 1 /* NEW_RUNNING_FEAT */
	if (!is_admin(p_ptr) && !p_ptr->ghost && !p_ptr->tim_wraith) {
		/* are we in fact running-levitating? */
		//if ((f_info[c_ptr->feat].flags1 & (FF1_CAN_LEVITATE | FF1_CAN_RUN)) && p_ptr->levitate) {
		if ((f_info[c_ptr->feat].flags1 & (FF1_CAN_LEVITATE | FF1_CAN_RUN))) {
			/* Allow level 50 druids to run at full speed */
			if (!(p_ptr->pclass == CLASS_DRUID &&  p_ptr->lev >= 50)) {
				if (f_info[c_ptr->feat].flags1 & FF1_SLOW_LEVITATING_1) *real_speed /= 2;
				if (f_info[c_ptr->feat].flags1 & FF1_SLOW_LEVITATING_2) *real_speed /= 4;
			}
		}
	    /* or running-swimming? */
		else if ((c_ptr->feat == 84 || c_ptr->feat == 103 || c_ptr->feat == 174 || c_ptr->feat == 187) && p_ptr->can_swim) {
			/* Allow Aquatic players run/swim at full speed */
			if (!r_info[p_ptr->body_monster].flags7&RF7_AQUATIC) {
				if (f_info[c_ptr->feat].flags1 & FF1_SLOW_SWIMMING_1) *real_speed /= 2;
				if (f_info[c_ptr->feat].flags1 & FF1_SLOW_SWIMMING_2) *real_speed /= 4;
			}
		}
		/* or just normally running? */
		else {
			if (f_info[c_ptr->feat].flags1 & FF1_SLOW_RUNNING_1) *real_speed /= 2;
			if (f_info[c_ptr->feat].flags1 & FF1_SLOW_RUNNING_2) *real_speed /= 4;
		}
	}
#endif

#if 0 /* enable? */
 #if defined CLIENT_SIDE_WEATHER && !defined CLIENT_WEATHER_GLOBAL
    {	int wind, real_speed_vertical;
	/* hack: wind without rain doesn't count, since it might confuse the players */
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
		msg_broadcast_format(0, "\374\377I*** \377RAutomatic recall and server maintenance shutdown in %d minute%s. \377I***", k, (k == 1) ? "" : "s");
	else
		msg_broadcast_format(0, "\374\377I*** \377RAutomatic recall and server restart in %d minute%s. \377I***", k, (k == 1) ? "" : "s");

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
	if (!wpos->wx && !wpos->wy) return (-1);

	/* no dungeon/tower here? */
	if (wpos->wz > 0 && !wild_info[wpos->wy][wpos->wx].tower) return (-1);
	if (wpos->wz <= 0 && !wild_info[wpos->wy][wpos->wx].dungeon) return (-1);/* assume basic recall prefers dungeon over tower */

	for (j = 0; j < MAX_D_IDX * 2; j++) {
		/* it's a dungeon that's new to us - add it! */
		if (!p_ptr->max_depth_wx[j] && !p_ptr->max_depth_wy[j]) {
			p_ptr->max_depth_wx[j] = wpos->wx;
			p_ptr->max_depth_wy[j] = wpos->wy;
			p_ptr->max_depth_tower[j] = (wpos->wz > 0);
			return j;
		}
		/* check dungeons we've already been to */
		if (p_ptr->max_depth_wx[j] == wpos->wx &&
		    p_ptr->max_depth_wy[j] == wpos->wy &&
		    p_ptr->max_depth_tower[j] == (wpos->wz > 0))
			return j;
	}
	s_printf("p_ptr->max_depth[]: TOO MANY DUNGEONS ('%s',%d,%d,%d)!\n", p_ptr->name, wpos->wx, wpos->wy, wpos->wz);
	return (-1);
}
int get_recall_depth(struct worldpos *wpos, player_type *p_ptr) {
	int i = recall_depth_idx(wpos, p_ptr);
	if (i == -1) return 0;
	return p_ptr->max_depth[i];
}

/* Inject a delayed command into the command queue, used for handling !X inscription:
   It simulates the player executing an ID command after the item has been picked up. */
void handle_XID(int Ind) {
#if defined(ENABLE_XID_SPELL) || defined(ENABLE_XID_MDEV)
 #ifdef XID_REPEAT
	{
	sockbuf_t *connpq = get_conn_q(Ind);
	player_type *p_ptr = Players[Ind];

	if (!p_ptr->delayed_spell) return;
	if (p_ptr->command_rep_active) return;
	p_ptr->command_rep_active = TRUE;

	/* hack: inject the delayed ID-spell cast command */
	switch (p_ptr->delayed_spell) {
	case 0: break; /* nothing */
	case -1: /* item activation */
		p_ptr->command_rep = PKT_ACTIVATE;
		Packet_printf(connpq, "%c%hd", PKT_ACTIVATE, p_ptr->delayed_index);
		p_ptr->delayed_spell_temp = p_ptr->delayed_spell;
		p_ptr->delayed_spell = 0;
		break;
	case -2: /* perception staff use */
		p_ptr->command_rep = PKT_USE;
		Packet_printf(connpq, "%c%hd", PKT_USE, p_ptr->delayed_index);
		p_ptr->delayed_spell_temp = p_ptr->delayed_spell;
		p_ptr->delayed_spell = 0;
		break;
	case -3: /* perception rod zap */
		p_ptr->command_rep = PKT_ZAP;
		Packet_printf(connpq, "%c%hd", PKT_ZAP, p_ptr->delayed_index);
		p_ptr->delayed_spell_temp = p_ptr->delayed_spell;
		p_ptr->delayed_spell = 0;
		break;
	case -4: /* ID / *ID* scroll read (Note: This is of course a one-time thing, won't get repeated - scrolls always succeed) */
		//p_ptr->command_rep = PKT_READ;
		Packet_printf(connpq, "%c%hd", PKT_READ, p_ptr->delayed_index);
		//p_ptr->delayed_spell_temp = p_ptr->delayed_spell;
		p_ptr->delayed_spell = 0;
		break;
	default: /* spell */
		p_ptr->command_rep = PKT_ACTIVATE_SKILL;
		Packet_printf(connpq, "%c%c%hd%hd%c%hd%hd", PKT_ACTIVATE_SKILL, MKEY_SCHOOL, p_ptr->delayed_index, p_ptr->delayed_spell, -1, -1, 0);
		p_ptr->delayed_spell_temp = p_ptr->delayed_spell;
		p_ptr->delayed_spell = 0;
	}
	}
 #endif
#endif
}

/* Helper function to determine snowballs melting */
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
	case WILD_DESERT:
		cold = FALSE;
		break;
	}

	/* don't melt in icy dungeons, but melt in other dungeons */
	if (wpos->wz > 0) d_ptr = wild_info[wpos->wy][wpos->wx].tower;
	else if (wpos->wz < 0) d_ptr = wild_info[wpos->wy][wpos->wx].dungeon;
	if (d_ptr) {
		int dun_type;

#ifdef IRONDEEPDIVE_MIXED_TYPES
		if (in_irondeepdive(wpos)) dun_type = iddc[ABS(wpos->wz)].type;
		else
#endif
		dun_type = d_ptr->theme ? d_ptr->theme : d_ptr->type;

		switch (dun_type) {
		case DI_HELCARAXE:
		case DI_CLOUD_PLANES:
			cold = TRUE;
			break;
		default:
			cold = FALSE;
		}
	}

	return cold;
}
