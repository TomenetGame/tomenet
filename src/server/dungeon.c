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

/* duration of GoI when getting recalled.        [2] */
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
 #else

static void process_weather_control(void);
 #endif
#endif


/*
 * Return a "feeling" (or NULL) about an item.  Method 1 (Heavy).
 */
cptr value_check_aux1(object_type *o_ptr)
{
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

		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return "worthless";

		/* Normal */
#if 0
		/* exploding ammo is excellent */
		if (is_ammo(o_ptr->tval) && (o_ptr->pval != 0)) return "excellent";
#else
		/* All exploding or ego-ammo is excellent */
		if (is_ammo(o_ptr->tval) && (o_ptr->pval || o_ptr->name2 || o_ptr->name2b)) return "excellent";
#endif

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

cptr value_check_aux1_magic(object_type *o_ptr)
{
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
cptr value_check_aux2(object_type *o_ptr)
{
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
//		if (o_ptr->name2 == 125 || o_ptr->name2a == 125) return "broken"; /* backbiting */
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

cptr value_check_aux2_magic(object_type *o_ptr)
{
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
static void sense_inventory(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		i;

	bool heavy = FALSE, heavy_magic = FALSE, heavy_archery = FALSE;
	bool ok_combat = FALSE, ok_magic = FALSE, ok_archery = FALSE;
	bool ok_curse = FALSE;

	cptr	feel;
	bool	felt_heavy;

	object_type *o_ptr;
	char o_name[ONAME_LEN];


	/*** Check for "sensing" ***/

	/* No sensing when confused */
	if (p_ptr->confused) return;

#if 0 /* no more linear ;) */
	if (!rand_int(133 - get_skill_scale(p_ptr, SKILL_COMBAT, 130))) ok_combat = TRUE;
	if (!rand_int(133 - get_skill_scale(p_ptr, SKILL_ARCHERY, 130))) ok_archery = TRUE;
	if (!rand_int(133 - get_skill_scale(p_ptr, SKILL_MAGIC, 130))) {
		ok_magic = TRUE;
		ok_curse = TRUE;
	}
	if (!rand_int(133 - get_skill_scale(p_ptr, SKILL_PRAY, 130))) ok_curse = TRUE;
#else
     /* instead, allow more use at lower SKILL_COMBAT levels already,
	otherwise huge stacks of ID scrolls will remain mandatory for maybe too long a time - C. Blue */
//	if (!rand_int(399 / (get_skill_scale(p_ptr, SKILL_COMBAT, 97) + 3))) ok_combat = TRUE;
//	if (!rand_int(2 * (101 - (get_skill_scale(p_ptr, SKILL_COMBAT, 70) + 30)))) ok_combat = TRUE;
//	if (!rand_int(102 - (get_skill_scale(p_ptr, SKILL_COMBAT, 80) + 20))) ok_combat = TRUE;
//	if (!rand_int(102 - (get_skill_scale(p_ptr, SKILL_COMBAT, 70) + 30))) ok_combat = TRUE;
//	if (!rand_int(2000 / (get_skill_scale(p_ptr, SKILL_COMBAT, 80) + 20) - 18)) ok_combat = TRUE;
	if (!rand_int(3000 / (get_skill_scale(p_ptr, SKILL_COMBAT, 80) + 20) - 28)) ok_combat = TRUE;
	if (!rand_int(3000 / (get_skill_scale(p_ptr, SKILL_ARCHERY, 80) + 20) - 28)) ok_archery = TRUE;
	if (!rand_int(3000 / (get_skill_scale(p_ptr, SKILL_MAGIC, 80) + 20) - 28)) {
		ok_magic = TRUE;
		ok_curse = TRUE;
	}
	/* note: SKILL_PRAY is currently unused */
	if (!rand_int(3000 / (get_skill_scale(p_ptr, SKILL_PRAY, 80) + 20) - 28)) ok_curse = TRUE;
#endif

	/* A powerful warrior can pseudo-id ranged weapons and ammo too,
	   even if (s)he's not good at archery in general */
	if (get_skill(p_ptr, SKILL_COMBAT) >= 31) {
#if 1 /* (apply 33% chance, see below) - allow basic feelings */
		ok_archery = TRUE;
#else /* (apply 33% chance, see below) - allow more distinctive feelings */
		heavy_archery = TRUE;
#endif
	}

	/* A very powerful warrior can even distinguish magic items */
	if (get_skill(p_ptr, SKILL_COMBAT) >= 41) {
#if 0 /* too much? */
		ok_magic = TRUE;
#endif
		ok_curse = TRUE;
	}

#if 1
	/* extra class-specific boni */
	i = 150 - ((p_ptr->lev <= 50) ? (p_ptr->lev * 2) : (p_ptr->lev + 50));
	if ((p_ptr->pclass == CLASS_PRIEST) && !rand_int(i)) ok_curse = TRUE;
 #if 0 /* out of line? */
	if ((p_ptr->pclass == CLASS_ISTAR ||
	    p_ptr->pclass == CLASS_SHAMAN) && !rand_int(i)) ok_magic = TRUE;
	if ((p_ptr->pclass == CLASS_MINDCRAFTER) && !rand_int(i)) {
		ok_curse = TRUE;
		ok_magic = TRUE;
		ok_combat = TRUE;
	}
 #endif
#endif

	/* nothing to feel? exit */
	if (!ok_combat && !ok_magic && !ok_archery) return;

	heavy = (get_skill(p_ptr, SKILL_COMBAT) >= 11) ? TRUE : FALSE;
	heavy_magic = (get_skill(p_ptr, SKILL_MAGIC) >= 11) ? TRUE : FALSE;
	heavy_archery = (get_skill(p_ptr, SKILL_ARCHERY) >= 11) ? TRUE : FALSE;

	/*** Sense everything ***/

	/* Check everything */
	for (i = 0; i < INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;
		/* We know about it already, do not tell us again */
		if (o_ptr->ident & ID_SENSE) continue;
		/* It is fully known, no information needed */
		if (object_known_p(Ind, o_ptr)) continue;
		/* Occasional failure on inventory items */
		if ((i < INVEN_WIELD) &&
		    (magik(80) || UNAWARENESS(p_ptr))) continue;

		feel = NULL;
		felt_heavy = FALSE;

		if (ok_curse && cursed_p(o_ptr)) feel = value_check_aux1(o_ptr);

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
			if (ok_combat)
				feel = (heavy ? value_check_aux1(o_ptr) :
						value_check_aux2(o_ptr));
			if (heavy) felt_heavy = TRUE;
			break;
		case TV_MSTAFF:
			if (ok_magic)
				feel = (heavy_magic ? value_check_aux1(o_ptr) :
						value_check_aux2(o_ptr));
			if (heavy_magic) felt_heavy = TRUE;
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
			if (ok_magic && !object_aware_p(Ind, o_ptr))
				feel = (heavy_magic ? value_check_aux1_magic(o_ptr) :
				    value_check_aux2_magic(o_ptr));
			if (heavy_magic) felt_heavy = TRUE;
			break;
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_BOOMERANG:
			if (ok_archery || (ok_combat && magik(25)))
				feel = (heavy_archery ? value_check_aux1(o_ptr) :
				    value_check_aux2(o_ptr));
			if (heavy_archery) felt_heavy = TRUE;
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
		else {
			strcpy(o_name, feel); /* just abusing o_name for this since it's not needed anymore anyway */
			strcat(o_name, "-");
			strcat(o_name, quark_str(o_ptr->note));
			o_ptr->note = quark_add(o_name);
		}

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);
	}
}



/*
 * Regenerate hit points				-RAK-
 */
static void regenhp(int Ind, int percent)
{
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
static void regenmana(int Ind, int percent)
{
	player_type *p_ptr = Players[Ind];

	s32b        new_mana, new_mana_frac;
	int                   old_csp;

	old_csp = p_ptr->csp;
	new_mana = ((s32b)p_ptr->msp) * percent + PY_REGEN_MNBASE;
	p_ptr->csp += new_mana >> 16;	/* div 65536 */
	/* check for overflow */
	if ((p_ptr->csp < 0) && (old_csp > 0))
	{
		p_ptr->csp = MAX_SHORT;
	}
	new_mana_frac = (new_mana & 0xFFFF) + p_ptr->csp_frac;	/* mod 65536 */
	if (new_mana_frac >= 0x10000L)
	{
		p_ptr->csp_frac = new_mana_frac - 0x10000L;
		p_ptr->csp++;
	}
	else
	{
		p_ptr->csp_frac = new_mana_frac;
	}

	/* Must set frac to zero even if equal */
	if (p_ptr->csp >= p_ptr->msp)
	{
		p_ptr->csp = p_ptr->msp;
		p_ptr->csp_frac = 0;
	}

	/* Redraw mana */
	if (old_csp != p_ptr->csp)
	{
		/* Redraw */
		p_ptr->redraw |= (PR_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);
	}
}


#define pelpel
#ifdef pelpel

/* Wipeout the effects	- Jir - */
static void erase_effects(int effect)
{
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
	e_ptr->cx = 0;
	e_ptr->cy = 0;
	e_ptr->rad = 0;

	if(!(zcave=getcave(wpos))) return;

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
static void process_effects(void)
{
	int i, j, k, l;
	worldpos *wpos;
	cave_type **zcave;
	cave_type *c_ptr;
	int who = PROJECTOR_EFFECT;
	player_type *p_ptr;

	/* Every 10 game turns */
//	if (turn % 10) return;

	/* Not in the small-scale wilderness map */
//	if (p_ptr->wild_mode) return;


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
                if(e_ptr->flags & EFF_CROSSHAIR_A || e_ptr->flags & EFF_CROSSHAIR_B || e_ptr->flags & EFF_CROSSHAIR_C)
                {

                        /* e_ptr->interval is player who controls it */
                        if(e_ptr->interval >= NumPlayers)
                        {
                                p_ptr = Players[e_ptr->interval];
                                if(k == p_ptr->e)
                                {
                                        if(e_ptr->cy != p_ptr->arc_b || e_ptr->cx != p_ptr->arc_a)
                                        {
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

                                }
                                else
                                {
                                erase_effects(k);
                                }
                        }
                        else
                        {
                        erase_effects(k);
                        }
                continue;
                }
 #endif
#endif

#ifdef ANIMATE_EFFECTS
 #ifdef FREQUENT_EFFECT_ANIMATION
		/* Hack -- Decode extra runespell info for color. - Kurzel */
		u32b typ_original = e_ptr->type;
		e_ptr->type = e_ptr->type % 1000;
		
		/* hack: animate the effect! Note that this is independant of the effect interval,
		   as opposed to the other animation code below. - C. Blue */
		if (!(turn % (cfg.fps / 5)) && e_ptr->time && spell_color_animation(e_ptr->type))
			for (l = 0; l < tdi[e_ptr->rad]; l++) {
				j = e_ptr->cy + tdy[l];
				i = e_ptr->cx + tdx[l];
				if (!in_bounds2(wpos, j, i)) continue;
				everyone_lite_spot(wpos, j, i);
			}
			
		/* Hack -- Restore runespell info. - Kurzel */
		e_ptr->type = typ_original;			
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

		if (e_ptr->who > 0) who = e_ptr->who;
		else {
			/* Make the effect friendly after logging out - mikaelh */
			who = PROJECTOR_PLAYER;

			/* XXX Hack -- is the trapper online? */
			for (i = 1; i <= NumPlayers; i++) {
				p_ptr = Players[i];

				/* Check if they are in here */
				if (e_ptr->who == 0 - p_ptr->id) {
					who = 0 - i;
					break;
				}
			}
		}

		/* Storm ends if the cause is gone */
		if (e_ptr->flags & EFF_STORM && (who == PROJECTOR_EFFECT || who == PROJECTOR_PLAYER))
		{
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
					/* Apply damage */
					if (e_ptr->type == GF_HEALINGCLOUD)
						project(who, 0, wpos, j, i, e_ptr->dam, e_ptr->type,
						    PROJECT_NORF | PROJECT_GRID | PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE | PROJECT_JUMP | PROJECT_PLAY, "");
						    //PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE | PROJECT_JUMP | PROJECT_PLAY, "");
					else //Effects should also hit grids, for runemaster EFF_WAVE/STOR functionality. - Kurzel
						project(who, 0, wpos, j, i, e_ptr->dam, e_ptr->type,
						    PROJECT_NORF | PROJECT_GRID | PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE | PROJECT_JUMP, "");
						    //PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE | PROJECT_JUMP, "");
					/* Oh, destroyed? RIP */
					if (who < 0 && who != PROJECTOR_EFFECT && who != PROJECTOR_PLAYER &&
							Players[0 - who]->conn == NOT_CONNECTED)
					{
						/* Make the effect friendly after death - mikaelh */
						who = PROJECTOR_PLAYER;
					}
					
#ifdef ANIMATE_EFFECTS
 #ifndef FREQUENT_EFFECT_ANIMATION
					/* Hack -- Decode extra runespell info for color. - Kurzel */
					u32b typ_original = e_ptr->type;
					e_ptr->type = e_ptr->type % 1000;
					
					/* C. Blue - hack: animate effects inbetween
					   ie allow random changes in spell_color().
					   Note: animation speed depends on effect interval. */
					if (spell_color_animation(e_ptr->type))
					    everyone_lite_spot(wpos, j, i);
			
					/* Hack -- Restore runespell info. - Kurzel */
					e_ptr->type = typ_original;	
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
				    (who == PROJECTOR_PLAYER || Players[0 - who]->death))
				{
					erase_effects(k);
					break;
				}
			}


#if 0
			if (((e_ptr->flags & EFF_WAVE) && !(e_ptr->flags & EFF_LAST)) || ((e_ptr->flags & EFF_STORM) && !(e_ptr->flags & EFF_LAST)))
			{
				if (distance(e_ptr->cy, e_ptr->cx, j, i) < e_ptr->rad - 2)
					c_ptr->effect = 0;
			}
#else	// 0

			if (!(e_ptr->flags & EFF_LAST))
			{
				if ((e_ptr->flags & EFF_WAVE))
				{
					if (distance(e_ptr->cy, e_ptr->cx, j, i) < e_ptr->rad - 2)
					{
						c_ptr->effect = 0;
						everyone_lite_spot(wpos, j, i);
					}
				}
				else if ((e_ptr->flags & EFF_STORM))
				{
					c_ptr->effect = 0;
					everyone_lite_spot(wpos, j, i);
				}
				else if ((e_ptr->flags & EFF_SNOWING))
				{
					c_ptr->effect = 0;
					everyone_lite_spot(wpos, j, i);
				}
				else if ((e_ptr->flags & EFF_RAINING))
				{
					c_ptr->effect = 0;
					everyone_lite_spot(wpos, j, i);
				}
				else if (e_ptr->flags & (EFF_FIREWORKS1 | EFF_FIREWORKS2 | EFF_FIREWORKS3))
				{
					c_ptr->effect = 0;
					everyone_lite_spot(wpos, j, i);
				}
			}
#endif	// 0

			/* Creates a "wave" effect*/
			if (e_ptr->flags & EFF_WAVE)
			{
				if (los(wpos, e_ptr->cy, e_ptr->cx, j, i) &&
						(distance(e_ptr->cy, e_ptr->cx, j, i) == e_ptr->rad))
				{
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
	}



	/* Apply sustained effect in the player grid, if any */
//	apply_effect(py, px);
}

#endif /* pelpel */



/*
 * Queued drawing at the beginning of a new turn.
 */
static void process_lite_later(void)
{
	int i;
	struct worldspot *wspot;

	for (i = 0; i < lite_later_num; i++)
	{
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
static void regen_monsters(void)
{
	int i, frac;

	/* Regenerate everyone */
	for (i = 1; i < m_max; i++)
	{
		/* Check the i'th monster */
		monster_type *m_ptr = &m_list[i];
                monster_race *r_ptr = race_inf(m_ptr);

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Allow regeneration (if needed) */
		if (m_ptr->hp < m_ptr->maxhp)
		{
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
void player_day(int Ind) {
	player_type *p_ptr = Players[Ind];
	int x, y;

//	if (p_ptr->tim_watchlist) p_ptr->tim_watchlist--;
	if (p_ptr->prace == RACE_VAMPIRE ||
	    (p_ptr->body_monster && r_info[p_ptr->body_monster].d_char == 'V'))
		calc_boni(Ind); /* daylight */

	/* Hack -- Scan the level */
	for (y = 0; y < MAX_HGT; y++)
	for (x = 0; x < MAX_WID; x++) {
		/* Hack -- Memorize lit grids if allowed */
		if (istownarea(&p_ptr->wpos, 2)
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
	handle_music(Ind);
#endif
}
/* update a particular player's view to night, assuming he's on world surface */
void player_night(int Ind) {
	player_type *p_ptr = Players[Ind];
	cave_type **zcave = getcave(&p_ptr->wpos);
	int x, y;

	if (!zcave) return; /* paranoia */

//	if (p_ptr->tim_watchlist) p_ptr->tim_watchlist--;
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
		} else if (istownarea(&p_ptr->wpos, 2)
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
	handle_music(Ind);
#endif
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
	int y, x;

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
	int y, x;
	int stores = 0, y1, x1, i;
	byte sx[255], sy[255];
	
	if (!zcave) return; /* paranoia */

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

		if (c_ptr->feat == FEAT_SHOP && stores < 254) {
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
//			c_ptr->info |= CAVE_ROOM | CAVE_GLOW;
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
		if (!getcave(&wrpos)) continue;
		world_surface_day(&wrpos);
	}

	/* Message all players who witness switch */
	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (Players[i]->wpos.wz) continue;
		msg_print(i, "The sun has risen.");

		player_day(i);
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
		if (!getcave(&wrpos)) continue;
		world_surface_night(&wrpos);
	}

	/* Message all players who witness switch */
	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (Players[i]->wpos.wz) continue;
		msg_print(i, "The sun has fallen.");

		player_night(i);
	}
}

/* take care of day/night changes, on world surface.
   NOTE: assumes that it gets called every HOUR turns only! */
static void process_day_and_night() {
	bool sunrise, nightfall;

	/* Check for sunrise or nightfall */
	sunrise = ((turn / HOUR) % 24) == SUNRISE;
	nightfall = ((turn / HOUR) % 24) == NIGHTFALL;

	/* Day breaks - not during Halloween {>_>} or during NEW_YEARS_EVE (fireworks)! */
	if (sunrise && !fireworks && !season_halloween)
	{
		sun_rises();
	}

	/* Night falls - but only if it was actually day so far:
	   During HALLOWEEN as well as NEW_YEARS_EVE it stays night all the time >:) (see above) */
	else if (nightfall && !night_surface) {
		night_falls();
	}
}

/* Called when the server starts up */
static void init_day_and_night() {
	int hour;

	hour = ((turn / HOUR) % 24);

	if ((hour >= SUNRISE) && (hour <= NIGHTFALL) && !season_halloween) {
		sun_rises();
	} else {
		night_falls();
	}
}

/*
 * Handle certain things once every 50 game turns
 */

static void process_world(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int	i;
//	int	regen_amount, NumPlayers_old=NumPlayers;


	/*** Process the monsters ***/
	/* Note : since monsters are added at a constant rate in real time,
	 * this corresponds in game time to them appearing at faster rates
	 * deeper in the dungeon.
	 */

#if 0 //see below o_O
	/* Check for creature generation */
	if ((!istown(&p_ptr->wpos) && (rand_int(MAX_M_ALLOC_CHANCE) == 0)) ||
 #ifndef HALLOWEEN
	    (istown(&p_ptr->wpos) && (rand_int(TOWNIE_RESPAWN_CHANCE * ((3 / NumPlayers) + 1)) == 0)))
 #else
	    (istown(&p_ptr->wpos) && (rand_int((p_ptr->wpos.wx == cfg.town_x && p_ptr->wpos.wy == cfg.town_y ?
	    HALLOWEEN_TOWNIE_RESPAWN_CHANCE : TOWNIE_RESPAWN_CHANCE) * ((3 / NumPlayers) + 1)) == 0)))
 #endif
#endif//0

	/* Check for creature generation */
#if 0 /* too many people idling all day.. ;) */
	if ((!istown(&p_ptr->wpos) && (rand_int(MAX_M_ALLOC_CHANCE) == 0)) ||
	    (!season_halloween && (istown(&p_ptr->wpos) && (rand_int(TOWNIE_RESPAWN_CHANCE * ((3 / NumPlayers) + 1)) == 0))) ||
	    (season_halloween && (istown(&p_ptr->wpos) && (rand_int((p_ptr->wpos.wx == cfg.town_x && p_ptr->wpos.wy == cfg.town_y ?
		HALLOWEEN_TOWNIE_RESPAWN_CHANCE : TOWNIE_RESPAWN_CHANCE) * ((3 / NumPlayers) + 1)) == 0))))
#else /* ..so no longer depending on amount of players in town: */
	if ((!istown(&p_ptr->wpos) && (rand_int(MAX_M_ALLOC_CHANCE) == 0)) ||
	    (!season_halloween && istown(&p_ptr->wpos) && (rand_int(TOWNIE_RESPAWN_CHANCE) == 0)) ||
	    (season_halloween && istown(&p_ptr->wpos) &&
	    (rand_int(p_ptr->wpos.wx == cfg.town_x && p_ptr->wpos.wy == cfg.town_y ?
	    HALLOWEEN_TOWNIE_RESPAWN_CHANCE : TOWNIE_RESPAWN_CHANCE) == 0)))
#endif
	{
		dun_level *l_ptr = getfloor(&p_ptr->wpos);
		/* Should we disallow those with MULTIPLY flags to spawn on surface? */
		if (!l_ptr || !(l_ptr->flags1 & LF1_NO_NEW_MONSTER))
		{
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
	if (!(turn % 3000) && (p_ptr->black_breath))
	{
		msg_print(Ind, "\377WThe Black Breath saps your soul!");

		/* alert to the neighbors also */
		msg_format_near(Ind, "\377WA dark aura seems to surround %s!", p_ptr->name);

		disturb(Ind, 0, 0);
	}

	/* Cold Turkey  */
	i = (p_ptr->csane << 7) / p_ptr->msane;
	if (!(turn % 4200) && !magik(i))
	{
		msg_print(Ind, "\377rA flashback storms your head!");

		/* alert to the neighbors also */
		if (magik(20)) msg_format_near(Ind, "You see %s's eyes bloodshot.", p_ptr->name);

		set_image(Ind, p_ptr->image + 12 - i / 8);
		disturb(Ind, 0, 0);
	}

#ifdef GHOST_FADING
		if (p_ptr->ghost && !p_ptr->admin_dm &&
//			!(turn % GHOST_FADING))
//			!(turn % ((5100L - p_ptr->lev * 50)*GHOST_FADING)))
			!(turn % (GHOST_FADING / p_ptr->lev * 50)))
//			(rand_int(10000) < p_ptr->lev * p_ptr->lev))
			take_xp_hit(Ind, 1 + p_ptr->lev / 5 + p_ptr->max_exp / 10000L, "fading", TRUE, TRUE, FALSE);
#endif	// GHOST_FADING

}

/*
 * Quick hack to allow mimics to retaliate with innate powers	- Jir -
 * It's high time we redesign auto-retaliator	XXX
 */
static int retaliate_mimic_power(int Ind, int choice)
{
	player_type *p_ptr = Players[Ind];
	int i, k, num = 3;

	/* Check for "okay" spells */
	for (k = 0; k < 3; k++) {
		for (i = 0; i < 32; i++) {
			/* Look for "okay" spells */
			if (p_ptr->innate_spells[k] & (1L << i)) {
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
static bool retaliate_item(int Ind, int item, cptr inscription, bool fallback)
{
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
		if (!istownarea(&p_ptr->wpos, 2)) return FALSE;
		inscription++;
	}

	/* Hack -- use innate power
	 * TODO: devise a generic way to activate skills for retaliation */
	if (inscription != NULL && *inscription == 'M' && get_skill(p_ptr, SKILL_MIMIC)) {
		/* Spell to cast */
		if (*(inscription + 1)) {
			choice = *(inscription + 1) - 97;

			/* valid inscription? */
			if (choice >= 0) {
				/* shape-changing for retaliation is not so nice idea, eh? */
				if (choice < 3) {	/* 3 polymorph powers */
					/* hack: prevent 'polymorph into...' power */
					if (choice == 2) do_cmd_mimic(Ind, 1, 5);
					else do_cmd_mimic(Ind, choice, 5);
					return TRUE;
				} else {
					int power = retaliate_mimic_power(Ind, choice);
					if (innate_powers[power].smana > p_ptr->csp && fallback) return (p_ptr->fail_no_melee);
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
				}
			}
		}
		return FALSE;
	}

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
		if (choice < 0 || choice > 9) choice = 0;
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
				if (MY_VERSION < (4 << 12 | 4 << 8 | 1 << 4 | 8)) {
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
#ifdef ENABLE_RCRAFT
		/*
			Auto-retaliation format:
			@Oa through to @Og
			(@O defaults to @Oa)
			@Oc casts a basic (cheap) bolt spell of the type that it is on.
			@Ob casts a self spell
			The letters correspond to the mkey spell selector
		*/
		case TV_RUNE2:
#if 0
			if (o_ptr->sval >= 0 && o_ptr->sval <= RCRAFT_MAX_ELEMENTS) {
				if (choice < 0 || choice >= 8) choice = 0;
				execute_rspell(Ind, 5, r_elements[o_ptr->sval].self | runespell_types[choice].type, 1, 1);
				//execute_rspell(Ind, 5, NULL, 0, r_elements[o_ptr->sval].self | R_BOLT, 0, 1); //choice sometimes has wrong value
				return TRUE;
			}
			break;
#else
			/* New runemaster retaliation. - Kurzel */
			/* Search for up to 2 more runes with the same inscription as the first found. */
			if (o_ptr->sval >= 0 && o_ptr->sval <= RCRAFT_MAX_ELEMENTS) {
				int i, runes = 0;
				int rune_type = 0;
				u32b rune_flags = 0;
				int imperative = 0;
				int type = 0;

				if (*inscription != '\0') {
					imperative = *inscription;
					inscription++;
					choice = imperative - 'a';
					if (choice < 0 || choice > RCRAFT_MAX_IMPERATIVES) choice = 0;
				}

				if (*inscription != '\0') {
					type = *inscription;
					rune_type = *inscription - 'a';
					if (rune_type < 0 || rune_type > RCRAFT_MAX_TYPES) rune_type = 0;
				}

				/* Pick the next rune with {@O} inscription */
				for (i = item; i < INVEN_TOTAL && runes < 3; i++) {
					o_ptr = &p_ptr->inventory[i];
					if (o_ptr->tval != TV_RUNE2) continue;

					inscription = quark_str(o_ptr->note);

					/* check for a valid inscription */
					if (inscription == NULL) continue;

					/* scan the inscription for @O */
					while (*inscription != '\0') {
						if (*inscription == '@') {
							inscription++;

							/* a valid @O has been located */
							if (*inscription == 'O' || *inscription == 'Q') {
								int type2 = 0;
								int imperative2 = 0;
								inscription++;

								if (*inscription != '\0') {
									imperative2 = *inscription;
									inscription++;
								}

								if (*inscription != '\0') {
									type2 = *inscription;
								}

								/* Skip this item in case it isn't the same */
								if (imperative != imperative2 || type != type2) break;
								
								/* Use this rune element */
								rune_flags |= r_elements[o_ptr->sval].self;
								runes++;
								
								break;
							}
						}
						inscription++;
					}
				}
				if (execute_rspell(Ind, 5, rune_flags | runespell_types[rune_type].type, choice, 1) == 2
				    && fallback)
					return (p_ptr->fail_no_melee);
				return TRUE;
			}
			break;
#endif
#endif
		/* not likely :) */
	}

	/* If all fails, then melee */
//	return FALSE;
	return (p_ptr->fail_no_melee);
}


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
   Also see CHEAP_NO_TARGET_TEST. - C. Blue */
#define EXPENSIVE_NO_TARGET_TEST
static int auto_retaliate(int Ind)
{
	player_type *p_ptr = Players[Ind], *q_ptr, *p_target_ptr = NULL, *prev_p_target_ptr = NULL;
	int d, i, tx, ty, target, prev_target, item = -1;
//	char friends = 0;
	monster_type *m_ptr, *m_target_ptr = NULL, *prev_m_target_ptr = NULL;
	monster_race *r_ptr = NULL, *r_ptr2, *r_ptr0;
	object_type *o_ptr;
	cptr inscription = NULL, at_O_inscription = NULL;
	bool no_melee = FALSE, fallback = FALSE;
	bool skip_monsters = (p_ptr->cloaked || p_ptr->shadow_running) && !p_ptr->stormbringer;
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return(FALSE);

	if (p_ptr->new_level_flag) return 0;

	/* disable auto-retaliation if we skip monsters/hostile players and blood-bonded players likewise */
	if (skip_monsters && !p_ptr->blood_bond) return 0;

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
				    (is_weapon(o_ptr->tval) || is_ammo(o_ptr->tval) ||
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
				if (m_ptr->r_idx == 1101 || m_ptr->r_idx == 1126) continue;

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

		return 0;
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
				    (is_weapon(o_ptr->tval) || is_ammo(o_ptr->tval) ||
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

	/* If we have a player target, attack him. */
	if (p_target_ptr) {
		/* set the target */
		p_ptr->target_who = target;

		/* Attack him */
		/* Stormbringer bypasses everything!! */
//		py_attack(Ind, p_target_ptr->py, p_target_ptr->px);
		if (p_ptr->stormbringer ||
		    (!retaliate_item(Ind, item, at_O_inscription, fallback) && !p_ptr->afraid && !no_melee)) {
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
		if (!retaliate_item(Ind, item, at_O_inscription, fallback) && !p_ptr->afraid && !no_melee) {
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
static void process_player_begin(int Ind)
{
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
		for (y = 0; y < MAX_HGT; y++) {
			for (x = 0; x < MAX_WID; x++) {
				if (zcave[y][x].feat == FEAT_PERM_SOLID) zcave[y][x].feat = FEAT_HIGH_MOUNT_SOLID;
			}
		}
		/* Wipe any monsters/objects */
		wipe_o_list_safely(&p_ptr->wpos);
		wipe_m_list(&p_ptr->wpos);
		/* Regenerate the level from fixed layout */
		process_dungeon_file("t_valinor.txt", &p_ptr->wpos, &ystart, &xstart, 21, 67, TRUE);
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
		place_monster_one(&p_ptr->wpos, 7, 10, 1100, 0, 0, 0, 0, 0);
		everyone_lite_spot(&p_ptr->wpos, 7, 10);
		place_monster_one(&p_ptr->wpos, 7, 15, 1100, 0, 0, 0, 0, 0);
		everyone_lite_spot(&p_ptr->wpos, 7, 15);
		place_monster_one(&p_ptr->wpos, 10, 25, 1098, 0, 0, 0, 0, 0);
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


	/* Give the player some energy */
	p_ptr->energy += extract_energy[p_ptr->pspeed];
	limit_energy(p_ptr);

	/* Check "resting" status */
	if (p_ptr->resting) {
		/* No energy availiable while resting */
		// This prevents us from instantly waking up.
		p_ptr->energy = 0;
	}

	/* Handle paralysis here */
#ifndef ARCADE_SERVER
	if (p_ptr->paralyzed || p_ptr->stun >= 100)
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
static void apply_effect(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int y = p_ptr->py, x = p_ptr->px;
	cave_type *c_ptr;
	feature_type *f_ptr;

	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return;

	c_ptr = &zcave[y][x];
	f_ptr = &f_info[c_ptr->feat];

	if (f_ptr->d_frequency[0] != 0) {
		int i;

		for (i = 0; i < 4; i++) {
			/* Check the frequency */
			if (f_ptr->d_frequency[i] == 0) continue;

			/* XXX it's no good to use 'turn' here */
//			if (((turn % f_ptr->d_frequency[i]) == 0) &&

			if ((!rand_int(f_ptr->d_frequency[i])) &&
			    ((f_ptr->d_side[i] != 0) || (f_ptr->d_dice[i] != 0)))
			{
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
				        PROJECT_NORF | PROJECT_KILL | PROJECT_HIDE | PROJECT_JUMP, "");

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
void recall_player(int Ind, char *message){
	struct player_type *p_ptr;
	cave_type **zcave;
	struct worldpos old_wpos;

	p_ptr = Players[Ind];

	if(!p_ptr) return;
	if(!(zcave = getcave(&p_ptr->wpos))) return;	// eww

	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);

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

	/* Did we really make it through all (99?) floors of the ironman challenge dungeon? */
	if (old_wpos.wx == WPOS_IRONDEEPDIVE_X &&
	    old_wpos.wy == WPOS_IRONDEEPDIVE_Y &&
	    old_wpos.wz != 0
	    && !is_admin(p_ptr)) {
		int i, j;

		for (i = 0; i < 20; i++) {
			if (deep_dive_level[i] == -1) continue;
			for (j = 20 - 1; j > i; j--) {
				deep_dive_level[j] = deep_dive_level[j - 1];
				strcpy(deep_dive_name[j], deep_dive_name[j - 1]);
			}
			deep_dive_level[i] = -1;
			//strcpy(deep_dive_name[i], p_ptr->name);
			sprintf(deep_dive_name[i], "%s the %s %s (%d)", p_ptr->name, get_prace(p_ptr), class_info[p_ptr->pclass].title, p_ptr->max_plv);
			break;
		}

		msg_broadcast_format(0, "\374\377L***\377a%s made it through the Ironman Deep Dive challenge!\377L***", p_ptr->name);
		l_printf("%s \\{U%s (%d) made it through the Ironman Deep Dive challenge\n", showdate(), p_ptr->name, p_ptr->lev);
	}
}


/* Handles WoR
 * XXX dirty -- REWRITEME
 */
static void do_recall(int Ind, bool bypass)
{
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
			msg_print(Ind, "A tension leaves the air around you...");
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
				msg_print(Ind, "A tension leaves the air around you...");
				p_ptr->redraw |= (PR_DEPTH);
				if (!is_admin(p_ptr)) {
					set_afraid(Ind, 10);// + (d_ptr->baselevel - p_ptr->max_dlv));
					return;
				}
			}
		}
#endif
        /* Nether Realm only for Kings/Queens (currently paranoia, since NR is NO_RECALL_INTO) */
        if (d_ptr && (d_ptr->type == 6) && !p_ptr->total_winner) {
            msg_print(Ind,"\377rAs you attempt to ascend, you are gripped by an uncontrollable fear.");
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
		if ((((d_ptr->flags2 & DF2_IRON || d_ptr->flags1 & DF1_FORCE_DOWN) && d_ptr->maxdepth > ABS(p_ptr->wpos.wz)) ||
		    (d_ptr->flags1 & DF1_NO_RECALL)) && !(getfloor(&p_ptr->wpos)->flags1 & LF1_IRON_RECALL)) {
			msg_print(Ind, "You feel yourself being pulled toward the surface!");
			if (!is_admin(p_ptr)) {
				recall_ok = FALSE;
				/* Redraw the depth(colour) */
				p_ptr->redraw |= (PR_DEPTH);
			}
		}
		if (recall_ok) {
			msg_format(Ind, "\377%cYou are transported out of %s..", COLOUR_DUNGEON, d_name + d_info[d_ptr->type].name);
			if(p_ptr->wpos.wz > 0) {
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
		    (1 << (wild_idx(&p_ptr->recall_pos))%8))) &&
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
		message="You feel yourself yanked sideways!";
		msg_format_near(Ind, "%s is yanked sideways!", p_ptr->name);

		/* New location */
		p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
	}

	/* into dungeon/tower */
	/* even at runlevel 2048 players may still recall..for now */
	else if ((cfg.runlevel > 4) && (cfg.runlevel <= 2048)) {
		wilderness_type *w_ptr = &wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx];
//		wilderness_type *w_ptr = &wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx];
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
			    (d_ptr->type && d_info[d_ptr->type].min_plev > p_ptr->lev) ||
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
				else
					msg_print(Ind, "You feel yourself yanked toward nowhere...");
			}

			if (p_ptr->recall_pos.wz >= 0) {
				p_ptr->recall_pos.wz = 0;
			} else {
				message = "You feel yourself yanked downwards!";
		                msg_format(Ind, "\377%cYou are transported into %s..", COLOUR_DUNGEON, d_name + d_info[d_ptr->type].name);
				msg_format_near(Ind, "%s is yanked downwards!", p_ptr->name);
			}
		}
		else if (p_ptr->recall_pos.wz > 0 && w_ptr->flags & WILD_F_UP) {
			dungeon_type *d_ptr=wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx].tower;
#ifdef SEPARATE_RECALL_DEPTHS
			int d = get_recall_depth(&p_ptr->recall_pos, p_ptr);
#endif

			/* check character/tower limits */
			if (p_ptr->recall_pos.wz > w_ptr->tower->maxdepth)
				p_ptr->recall_pos.wz = w_ptr->tower->maxdepth;
			if (p_ptr->inval && -p_ptr->recall_pos.wz > 10)
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
			    (d_ptr->type && d_info[d_ptr->type].min_plev > p_ptr->lev) ||
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
				else
					msg_print(Ind, "You feel yourself yanked toward nowhere...");
			}

			if (p_ptr->recall_pos.wz <= 0) {
				p_ptr->recall_pos.wz = 0;
			} else {
				message = "You feel yourself yanked upwards!";
		                msg_format(Ind, "\377%cYou are transported into %s..", COLOUR_DUNGEON, d_name + d_info[d_ptr->type].name);
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
	for(i = 0; i < INVEN_WIELD; i++){
		if(p_ptr->inventory[i].tval == 1 && p_ptr->inventory[i].sval == 9) {
			break;
		}
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
static bool process_player_end_aux(int Ind)
{
	player_type *p_ptr = Players[Ind];
	cave_type		*c_ptr;
	object_type		*o_ptr;
	int		i, j;
	int		regen_amount; //, NumPlayers_old=NumPlayers;
	dun_level *l_ptr = getfloor(&p_ptr->wpos);

	/* Unbelievers "resist" magic */
	//		int minus = (p_ptr->anti_magic)?3:1;
	int minus = 1;
	int minus_magic = 1 + get_skill_scale_fine(p_ptr, SKILL_ANTIMAGIC, 3); /* was 3 before, trying slightly less harsh 2 now */
	int minus_health = get_skill_scale_fine(p_ptr, SKILL_HEALTH, 2); /* was 3, but then HEALTH didn't give HP.. */
	int minus_combat = get_skill_scale_fine(p_ptr, SKILL_COMBAT, 3);

	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return(FALSE);

	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* Anything done here cannot be reduced by GoI/Manashield etc */
	bypass_invuln = TRUE;

	/*** Damage over Time ***/

	/* Take damage from poison */
	if (p_ptr->poisoned) {
		/* Take damage */
		take_hit(Ind, 1, "poison", p_ptr->poisoned_attacker);
	}

	/* Misc. terrain effects */
	if (!p_ptr->ghost) {
		/* Generic terrain effects */
		apply_effect(Ind);

		/* Drowning, but not ghosts */
		if (c_ptr->feat == FEAT_DEEP_WATER) {
			if ((!p_ptr->fly) && (!p_ptr->can_swim)) {
				/* Take damage */
				if (!(p_ptr->body_monster) || (
					!(r_info[p_ptr->body_monster].flags7 &
						(RF7_AQUATIC | RF7_CAN_SWIM)) &&
					!(r_info[p_ptr->body_monster].flags3&RF3_UNDEAD) ))
				{
					int hit = p_ptr->mhp>>6;
					int swim = get_skill_scale(p_ptr, SKILL_SWIM, 4500);
					hit += randint(p_ptr->mhp>>5);
					if(!hit) hit=1;

					/* Take CON into consideration(max 30%) */
					hit = (hit * (80 - adj_str_wgt[p_ptr->stat_ind[A_CON]])) / 75;

					/* temporary abs weight calc */
					if(p_ptr->wt+p_ptr->total_weight/10 > 170 + swim * 2)	// 190
					{
						long factor=(p_ptr->wt+p_ptr->total_weight/10)-150-swim * 2;	// 170
						/* too heavy, always drown? */
						if (factor < 300) {
							if (randint(factor) < 20) hit = 0;
						}

						/* Hack: Take STR and DEX into consideration (max 28%) */
						if (magik(adj_str_wgt[p_ptr->stat_ind[A_STR]] / 2) ||
								magik(adj_str_wgt[p_ptr->stat_ind[A_DEX]]) / 2)
							hit = 0;

						if (magik(swim)) hit = 0;

						if (hit) msg_print(Ind,"\377rYou're drowning!");

						/* harm equipments (even hit == 0) */
						if (TOOL_EQUIPPED(p_ptr) != SV_TOOL_TARPAULIN &&
						    magik(WATER_ITEM_DAMAGE_CHANCE) && !p_ptr->fly &&
						    !p_ptr->immune_water) {
							if (!p_ptr->resist_water || magik(50)) {
								if (!magik(get_skill_scale(p_ptr, SKILL_SWIM, 4900)))
									inven_damage(Ind, set_water_destroy, 1);
								equip_damage(Ind, GF_WATER);
							}
						}

						if(randint(1000 - factor) < 10) {
							if (p_ptr->prace != RACE_HALF_TROLL) {
								msg_print(Ind,"\377oYou are weakened by the exertion of swimming!");
								//							do_dec_stat(Ind, A_STR, STAT_DEC_TEMPORARY);
								dec_stat(Ind, A_STR, 10, STAT_DEC_TEMPORARY);
							}
						}
						take_hit(Ind, hit, "drowning", 0);
					}
				}
			}
		}
		/* Aquatic anoxia */
		else if((p_ptr->body_monster) &&
		    ((r_info[p_ptr->body_monster].flags7 & RF7_AQUATIC) &&
		    !(r_info[p_ptr->body_monster].flags3 & RF3_UNDEAD))
		    && ((c_ptr->feat != FEAT_SHAL_WATER) ||
		    r_info[p_ptr->body_monster].weight > 700)
		    /* new: don't get stunned from crossing door/stair grids every time - C. Blue */
		    && !(is_door(c_ptr->feat) || is_stair(c_ptr->feat)))
		{
			long hit = p_ptr->mhp>>6; /* Take damage */
			hit += randint(p_ptr->chp>>5);
			/* Take CON into consideration(max 30%) */
			hit = (hit * (80 - adj_str_wgt[p_ptr->stat_ind[A_CON]])) / 75;
			if(!hit) hit=1;

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
			if (p_ptr->stun < 20) set_stun(Ind, p_ptr->stun + 5);

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
			if ((p_ptr->pass_trees || p_ptr->fly) &&
					(c_ptr->feat == FEAT_TREE))
#endif
			if (player_can_enter(Ind, c_ptr->feat)) {
				/* Do nothing */
			}
			else if (c_ptr->feat == FEAT_HOME){/* rien */}
			//else if (PRACE_FLAG(PR1_SEMI_WRAITH) && (!p_ptr->wraith_form) && (f_info[cave[py][px].feat].flags1 & FF1_CAN_PASS))
			else if (!p_ptr->tim_wraith &&
			    !p_ptr->master_move_hook)	/* Hack -- builder is safe */
			{
//				int amt = 1 + ((p_ptr->lev)/5) + p_ptr->mhp / 100;
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
	if (!p_ptr->ghost && p_ptr->prace == RACE_ENT && p_ptr->resting &&
	    (c_ptr->feat == FEAT_SHAL_WATER ||
	    c_ptr->feat == FEAT_DEEP_WATER ||
	    c_ptr->feat == FEAT_MUD ||
	    c_ptr->feat == FEAT_GRASS ||
	    c_ptr->feat == FEAT_DIRT ||
	    c_ptr->feat == FEAT_BUSH ||
	    c_ptr->feat == FEAT_TREE)) {
		/* only very slowly in general (maybe depends on soil/floor type?) */
		i = 5;
		/* prevent overnourishment ^^ */
		if (p_ptr->food + i > PY_FOOD_FULL) i = PY_FOOD_FULL - p_ptr->food;
		if (i > 0) (void)set_food(Ind, p_ptr->food + i);
	}
	/* Ghosts don't need food */
	/* Allow AFK-hivernation if not hungry */
	else if (!p_ptr->ghost && !(p_ptr->afk && p_ptr->food >= PY_FOOD_ALERT) && !p_ptr->admin_dm &&
	    /* Don't starve in town (but recover from being gorged) - C. Blue */
//	    (!istown(&p_ptr->wpos) || p_ptr->food >= PY_FOOD_MAX))
	    (!istown(&p_ptr->wpos) || p_ptr->food >= PY_FOOD_FULL)) /* allow to digest some to not get gorged in upcoming fights quickly - C. Blue */
	{
		/* Digest normally */
		if (p_ptr->food < PY_FOOD_MAX) {
			/* Every 50/6 level turns */
//			if (!(turn%((level_speed((&p_ptr->wpos))*10)/12)))
			if (!(turn % ((level_speed((&p_ptr->wpos)) / 12) * 10))) {
				/* Basic digestion rate based on speed */
//				i = extract_energy[p_ptr->pspeed]*2;	// 1.3 (let them starve)
				i = (10 + extract_energy[p_ptr->pspeed] * 3) / 2;

				/* Adrenaline takes more food */
				if (p_ptr->adrenaline) i *= 5;

				/* Biofeedback takes more food */
				if (p_ptr->biofeedback) i *= 2;

				/* Regeneration takes more food */
				if (p_ptr->regenerate) i += 30;

				/* Regeneration takes more food */
				if (p_ptr->tim_regen) i += p_ptr->tim_regen_pow / 10;

				/* DragonRider and Half-Troll take more food */
				if (p_ptr->prace == RACE_DRACONIAN
						|| p_ptr->prace == RACE_HALF_TROLL) i += 15;

				/* Vampires consume food very quickly */
				if (p_ptr->prace == RACE_VAMPIRE) i += 20;

				/* Invisibility consume a lot of food */
				i += p_ptr->invis / 2;

				/* Invulnerability consume a lot of food */
				if (p_ptr->invuln) i += 40;

				/* Wraith Form consume a lot of food */
				if (p_ptr->tim_wraith) i += 30;

				/* Hitpoints multiplier consume a lot of food */
				if (p_ptr->to_l) i += p_ptr->to_l * 5;

				/* Slow digestion takes less food */
//				if (p_ptr->slow_digest) i -= 10;
				if (p_ptr->slow_digest) i -= (i > 40) ? i / 4 : 10;

				/* Never negative */
				if (i < 1) i = 1;

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
        o_ptr = &p_ptr->inventory[INVEN_NECK];
        /* Skip empty items */
        if (o_ptr->k_idx)
        {
            if (o_ptr->tval == TV_AMULET &&
                (o_ptr->sval == SV_AMULET_INVINCIBILITY || o_ptr->sval == SV_AMULET_INVULNERABILITY))
		p_ptr->food = PY_FOOD_MAX - 1;
	}


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
	/* Hack -- regenerate mana 5/3 times faster */
#ifdef ARCADE_SERVER
        p_ptr->regen_mana = TRUE;
#endif
	if (p_ptr->csp < p_ptr->msp)
		regenmana(Ind, ((regen_amount * 5) * ((p_ptr->regen_mana || (p_ptr->mode & MODE_PVP)) ? 2 : 1)) / 3);

	/* Regeneration ability */
	if (p_ptr->regenerate)
		regen_amount = regen_amount * 2;

	if (minus_health)
		regen_amount = regen_amount * 3 / 2;

	/* Holy curing gives improved regeneration ability */
	if (get_skill(p_ptr, SKILL_HCURING) >= 30) regen_amount = (regen_amount * (get_skill(p_ptr, SKILL_HCURING) - 10)) / 20;

	/* Increase regen by tim regen */
	if (p_ptr->tim_regen) regen_amount += p_ptr->tim_regen_pow;

	/* Poisoned or cut yields no healing */
	if (p_ptr->poisoned) regen_amount = 0;
	if (p_ptr->cut) regen_amount = 0;

	/* But Biofeedback always helps */
	if (p_ptr->biofeedback) regen_amount += randint(0x400) + regen_amount;

	/* Regenerate Hit Points if needed */
	if (p_ptr->chp < p_ptr->mhp)
		regenhp(Ind, regen_amount);

	/* Undiminish healing penalty in PVP mode */
	if (p_ptr->heal_effect) {
//		p_ptr->heal_effect -= (p_ptr->mhp + p_ptr->lev * 6) / 30;
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
//		int s = 2 * (76 + (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus_health) * 3);
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

	/* Temporary Mimicry from a Ring of Polymorphing */
	if (p_ptr->tim_mimic) {
		/* hack - on hold while in town */
		if (!istown(&p_ptr->wpos))
			/* decrease time left of being polymorphed */
			(void)set_mimic(Ind, p_ptr->tim_mimic - 1, p_ptr->tim_mimic_what);
	}

	/* Hack -- Timed manashield */
	if (p_ptr->tim_manashield)
		set_tim_manashield(Ind, p_ptr->tim_manashield - minus_magic);

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
	if (p_ptr->tim_jail && !p_ptr->wpos.wz) {
		p_ptr->tim_jail--;
		if (!p_ptr->tim_jail) {
#ifdef JAILER_KILLS_WOR
			/* eat his WoR scrolls as suggested? */
			bool found = FALSE, one = TRUE;
			for (j = 0; j < INVEN_WIELD; j++) {
				object_type *j_ptr;
				if (!p_ptr->inventory[j].k_idx) continue;
				j_ptr = &p_ptr->inventory[j];
				if ((j_ptr->tval == TV_ROD) && (j_ptr->sval == SV_ROD_RECALL)) {
					if (found) one = FALSE;
					if (o_ptr->number > 1) one = FALSE;
					j_ptr->pval = 300;
					found = TRUE;
				}
			}
			if (found) {
				msg_format(Ind, "The jailer discharges your rod%s of recall.", one ? "" : "s");
				p_ptr->window |= PW_INVEN;
			}
#endif

			/* only teleport him if he didn't take the ironman exit.
			   (could add p_ptr->in_jail to handle it 100% safely
			   or just set p_ptr->tim_jail to 0 on wpos change) */
			if (p_ptr->wpos.wz == 0 &&
			    (zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK)) {
				msg_print(Ind, "\377GYou are free to go!");
				teleport_player_force(Ind, 1);
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
			(void)set_invis(Ind, p_ptr->tim_invisibility - minus_magic, p_ptr->tim_invis_power);
		}
	}

	/* Timed infra-vision */
	if (p_ptr->tim_infra)
		(void)set_tim_infra(Ind, p_ptr->tim_infra - minus_magic);

	/* Paralysis */
	if (p_ptr->paralyzed)
		(void)set_paralyzed(Ind, p_ptr->paralyzed - 1 - minus_health);

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
		(void)do_divine_crit(Ind, p_ptr->divine_crit - minus_magic, p_ptr->divine_crit_mod);

	if (p_ptr->divine_hp)
		(void)do_divine_hp(Ind, p_ptr->divine_hp - minus_magic, p_ptr->divine_hp_mod);

	if (p_ptr->divine_xtra_res_time_mana)
		(void)do_divine_xtra_res_time_mana(Ind, p_ptr->divine_xtra_res_time_mana - minus_magic);
#endif
	/* xtra shot? - the_sandman */
	if (p_ptr->focus_time)
		(void)do_focus_shot(Ind, p_ptr->focus_time - minus_magic, p_ptr->focus_val);

	/* xtra stats? - the_sandman */
	if (p_ptr->xtrastat)
		(void)do_xtra_stats(Ind, p_ptr->xtrastat - minus_magic, p_ptr->statval);

	/* Protection from evil */
	if (p_ptr->protevil)
		(void)set_protevil(Ind, p_ptr->protevil - 1);

	/* Holy Zeal - EA bonus */
	if (p_ptr->zeal)
		(void)set_zeal(Ind, p_ptr->zeal_power, p_ptr->zeal - 1);

	/* Heart is still boldened */
	if (p_ptr->res_fear_temp)
		(void)set_res_fear(Ind, p_ptr->res_fear_temp - 1);

	/* Holy Martyr */
	if (p_ptr->martyr)
		(void)set_martyr(Ind, p_ptr->martyr - 1);
	if (p_ptr->martyr_timeout) {
		p_ptr->martyr_timeout--;
		if (!p_ptr->martyr_timeout) msg_print(Ind, "The heavens are ready to accept your martyrium.");
	}

	/* Mindcrafters' Willpower */
	if (p_ptr->mindboost)
		(void)set_mindboost(Ind, p_ptr->mindboost_power, p_ptr->mindboost - 1);

	/* Mindcrafters' repulsion shield */
	if (p_ptr->kinetic_shield) (void)set_kinetic_shield(Ind, p_ptr->kinetic_shield - 1);

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
		}
	}

	/* Invulnerability */
	/* Hack -- make -1 permanent invulnerability */
	if (p_ptr->invuln) {
		if (p_ptr->invuln > 0) (void)set_invuln(Ind, p_ptr->invuln - minus_magic);
		if (p_ptr->invuln == 5) msg_print(Ind, "\377vThe invulnerability shield starts to flicker and fade...");
	}

	/* Heroism */
	if (p_ptr->hero)
		(void)set_hero(Ind, p_ptr->hero - 1);

	/* Super Heroism */
	if (p_ptr->shero)
		(void)set_shero(Ind, p_ptr->shero - 1);

	/* Furry */
	if (p_ptr->fury)
		(void)set_fury(Ind, p_ptr->fury - 1);

	/* Berserk #2 */
	if (p_ptr->berserk)
		(void)set_berserk(Ind, p_ptr->berserk - 1);

	/* Sprint? melee technique */
	if (p_ptr->melee_sprint)
		(void)set_melee_sprint(Ind, p_ptr->melee_sprint - 1);

	/* Blessed */
	if (p_ptr->blessed)
		(void)set_blessed(Ind, p_ptr->blessed - 1);

	/* Shield */
	if (p_ptr->shield)
		(void)set_shield(Ind, p_ptr->shield - 1, p_ptr->shield_power, p_ptr->shield_opt, p_ptr->shield_power_opt, p_ptr->shield_power_opt2);

#ifdef ENABLE_RCRAFT
	/* Timed deflection */
	if (p_ptr->tim_deflect)
		(void)set_tim_deflect(Ind, p_ptr->tim_deflect - 1);

	/* New debuffs */
	if (p_ptr->temporary_am)
		(void)do_temporary_antimagic(Ind, p_ptr->temporary_am - minus_magic);

	if (p_ptr->temporary_to_l_dur)
		(void)do_life_bonus_aux(Ind, p_ptr->temporary_to_l_dur - minus_magic, p_ptr->temporary_to_l);

	if (p_ptr->temporary_speed_dur)
		(void)do_speed_bonus_aux(Ind, p_ptr->temporary_speed_dur - minus_magic, p_ptr->temporary_speed);
#endif
	
	/* Timed Levitation */
	if (p_ptr->tim_ffall)
		(void)set_tim_ffall(Ind, p_ptr->tim_ffall - 1);
	if (p_ptr->tim_fly)
		(void)set_tim_fly(Ind, p_ptr->tim_fly - 1);

	/* Timed regen */
	if (p_ptr->tim_regen)
		(void)set_tim_regen(Ind, p_ptr->tim_regen - 1, p_ptr->tim_regen_pow);
#ifdef ENABLE_RCRAFT
	/* Trauma boost */
	if (p_ptr->tim_trauma)
		(void)set_tim_trauma(Ind, p_ptr->tim_trauma - 1, p_ptr->tim_trauma_pow);
#endif
	/* Thunderstorm */
	if (p_ptr->tim_thunder) {
                int dam = damroll(p_ptr->tim_thunder_p1, p_ptr->tim_thunder_p2);
		int i, tries = 600;
		monster_type *m_ptr = NULL;

		while (tries) {
			/* Access the monster */
			m_ptr = &m_list[i = rand_range(1, m_max - 1)];

			tries--;

			/* Ignore "dead" monsters */
			if (!m_ptr->r_idx) continue;

			/* pfft. not even our level */

			if (!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;
			/* Cant see ? cant hit */
			if (!los(&p_ptr->wpos, p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx)) continue;
			if (distance(p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx) > 15) continue;

			/* Do not hurt friends! */
			/* if (is_friend(m_ptr) >= 0) continue; */
			break;
		}

		if (tries) {
			char m_name[MNAME_LEN];

			monster_desc(Ind, m_name, i, 0);
			msg_format(Ind, "Lightning strikes %s.", m_name, i, dam/3);
			project(-Ind, 0, &p_ptr->wpos, m_ptr->fy, m_ptr->fx, dam / 3, GF_ELEC,
			        PROJECT_KILL | PROJECT_ITEM, "");
			project(-Ind, 0, &p_ptr->wpos, m_ptr->fy, m_ptr->fx, dam / 3, GF_LITE,
			        PROJECT_KILL | PROJECT_ITEM, "");
			project(-Ind, 0, &p_ptr->wpos, m_ptr->fy, m_ptr->fx, dam / 3, GF_SOUND,
			        PROJECT_KILL | PROJECT_ITEM, "");
		}

		(void)set_tim_thunder(Ind, p_ptr->tim_thunder - 1, p_ptr->tim_thunder_p1, p_ptr->tim_thunder_p2);
        }

	/* Oppose Acid */
	if (p_ptr->oppose_acid)
		(void)set_oppose_acid(Ind, p_ptr->oppose_acid - 1);

	/* Oppose Lightning */
	if (p_ptr->oppose_elec)
		(void)set_oppose_elec(Ind, p_ptr->oppose_elec - 1);

	/* Oppose Fire */
	if (p_ptr->oppose_fire)
		(void)set_oppose_fire(Ind, p_ptr->oppose_fire - 1);

	/* Oppose Cold */
	if (p_ptr->oppose_cold)
		(void)set_oppose_cold(Ind, p_ptr->oppose_cold - 1);

	/* Oppose Poison */
	if (p_ptr->oppose_pois)
		(void)set_oppose_pois(Ind, p_ptr->oppose_pois - 1);

	/*** Poison and Stun and Cut ***/

	/* Poison */
	if (p_ptr->poisoned) {
		int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus);
		/* Apply some healing */
		if (get_skill(p_ptr, SKILL_HCURING) >= 30) adjust *= 2;

		//(void)set_poisoned(Ind, p_ptr->poisoned - adjust - minus_health * 2, p_ptr->poisoned_attacker);
		(void)set_poisoned(Ind, p_ptr->poisoned - (adjust + minus_health) * (minus_health + 1), p_ptr->poisoned_attacker);
	}

	/* Stun */
	if (p_ptr->stun) {
#if 0
		int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus);
		/* Apply some healing */
		//(void)set_stun(Ind, p_ptr->stun - adjust - minus_health * 2);
		(void)set_stun(Ind, p_ptr->stun - (adjust + minus_health) * (minus_health + 1));
#endif
		int adjust = minus + get_skill_scale_fine(p_ptr, SKILL_COMBAT, 2);
//		if (get_skill(p_ptr, SKILL_HCURING) >= 40) adjust = (adjust * 5) / 3;
		if (get_skill(p_ptr, SKILL_HCURING) >= 40) adjust++;

		(void)set_stun(Ind, p_ptr->stun - adjust);
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
//		(void)set_cut(Ind, p_ptr->cut - (adjust + minus_health) * (minus_health + 1), p_ptr->cut_attacker);
		(void)set_cut(Ind, p_ptr->cut - adjust * (minus_health + 1), p_ptr->cut_attacker);
	}

	/* Temporary blessing of luck */
	if (p_ptr->bless_temp_luck) (void)bless_temp_luck(Ind, p_ptr->bless_temp_luck_power, p_ptr->bless_temp_luck - 1);

	/* Temporary auras */
	if (p_ptr->sh_fire_tim) (void)set_sh_fire_tim(Ind, p_ptr->sh_fire_tim - 1);
	if (p_ptr->sh_cold_tim) (void)set_sh_cold_tim(Ind, p_ptr->sh_cold_tim - 1);
	if (p_ptr->sh_elec_tim) (void)set_sh_elec_tim(Ind, p_ptr->sh_elec_tim - 1);

	/* Still possible effects from another player's support spell on this player? */
	if (p_ptr->support_timer) {
		p_ptr->support_timer--;
		if (!p_ptr->support_timer) p_ptr->supported_by = 0;
	}

#if POLY_RING_METHOD == 0
	/* Check polymorph rings with timeouts */
	for (i = INVEN_LEFT; i <= INVEN_RIGHT; i++) {
		o_ptr = &p_ptr->inventory[i];
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH) && (o_ptr->timeout > 0) &&
		    (p_ptr->body_monster == o_ptr->pval))
		{
			/* Decrease life-span */
			o_ptr->timeout--;

			/* Hack -- notice interesting energy steps */
			if ((o_ptr->timeout < 100) || (!(o_ptr->timeout % 100))) {
				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP);
			}

			/* Hack -- notice interesting fuel steps */
			if ((o_ptr->timeout > 0) && (o_ptr->timeout < 100) && !(o_ptr->timeout % 10))
			{
				if (p_ptr->disturb_minor) disturb(Ind, 0, 0);
				msg_print(Ind, "\376\377LYour ring flickers and fades, flashes of light run over its surface.");
				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP);
			} else if (o_ptr->timeout == 0) {
				disturb(Ind, 0, 0);
				msg_print(Ind, "\376\377LYour ring disintegrates!");

    			        if ((p_ptr->body_monster == o_ptr->pval) &&
		                    ((p_ptr->r_killed[p_ptr->body_monster] < r_info[p_ptr->body_monster].level) ||
	                    	    (get_skill_scale(p_ptr, SKILL_MIMIC, 100) < r_info[p_ptr->body_monster].level)))
		                {
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

	/* Don't accidentally float after dieing */
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
		u32b f1 = 0 , f2 = 0 , f3 = 0, f4 = 0, f5 = 0, esp = 0;

		/* Hack -- Use some fuel (sometimes) */
#if 0
		if (!artifact_p(o_ptr) && !(o_ptr->sval == SV_LITE_DWARVEN)
		    && !(o_ptr->sval == SV_LITE_FEANOR) && (o_ptr->pval > 0) && (!o_ptr->name1))
#endif	// 0

			/* Extract the item flags */
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

		/* Hack -- Use some fuel */
		if ((f4 & TR4_FUEL_LITE) && (o_ptr->timeout > 0)) {
			/* Decrease life-span */
			o_ptr->timeout--;

			/* Hack -- notice interesting fuel steps */
			if ((o_ptr->timeout < 100) || (!(o_ptr->timeout % 100))) {
				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP);
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
				if (TOOL_EQUIPPED(p_ptr) == SV_TOOL_FLINT &&
						!p_ptr->paralyzed)
				{
					msg_print(Ind, "You prepare a flint...");
					refilled = do_auto_refill(Ind);
					if (!refilled)
						msg_print(Ind, "Oops, you're out of fuel!");
				}

				if (!refilled) {
					msg_print(Ind, "Your light has gone out!");
					/* Calculate torch radius */
					p_ptr->update |= (PU_TORCH|PU_BONUS);

					if (p_ptr->max_plv <= 20 && !p_ptr->warning_lite_refill) {
						p_ptr->warning_lite_refill = 1;
						msg_print(Ind, "\374\377yHINT: Press SHIFT+F to refill your light source. You will need a flask of");
						msg_print(Ind, "\374\377y      oil for lanterns, or another torch to combine with an extinct torch.");
//						s_printf("warning_lite_refill: %s\n", p_ptr->name);
					}
				}
#if 0	/* torch of presentiment goes poof to unlight trap, taken out for now - C. Blue */
				/* Torch disappears */
				if (o_ptr->sval == SV_LITE_TORCH && !o_ptr->timeout) {
					/* Decrease the item, optimize. */
					inven_item_increase(Ind, INVEN_LITE, -1);
					//						inven_item_describe(Ind, INVEN_LITE);
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
	//		p_ptr->update |= (PU_TORCH|PU_BONUS);

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
					if (p_ptr->food > PY_FOOD_FAINT - 1)
					        (void)set_food(Ind, PY_FOOD_FAINT - 1);
				(void)set_poisoned(Ind, 0, 0);
				if (!p_ptr->free_act && !p_ptr->slow_digest)
				        (void)set_paralyzed(Ind, p_ptr->paralyzed + rand_int(10) + 10);
			    }
			}
		}
	}


	/*** Process Inventory ***/

	/* Handle experience draining */
//	if (p_ptr->drain_exp && (p_ptr->wpos.wz != 0) && magik(30 - (60 / (p_ptr->drain_exp + 2))))
/* experimental: maybe no expdrain or just less expdrain while player is on world surface.
   idea: allow classes who lack a *remove curse* spell to make more use of the rings. */
//	if (p_ptr->drain_exp && (p_ptr->wpos.wz != 0) && magik(30 - (60 / (p_ptr->drain_exp + 2))))
//	if (p_ptr->drain_exp && magik(p_ptr->wpos.wz != 0 ? 50 : 0) && magik(30 - (60 / (p_ptr->drain_exp + 2))))
//	if (p_ptr->drain_exp && magik(p_ptr->wpos.wz != 0 ? 50 : (istown(&p_ptr->wpos) ? 0 : 25)) && magik(30 - (60 / (p_ptr->drain_exp + 2))))
	/* changing above line to use istownarea() so you can sort your houses without drain */
	if (p_ptr->drain_exp && magik(p_ptr->wpos.wz != 0 ? 50 : (istownarea(&p_ptr->wpos, 2) ? 0 : 25)) && magik(30 - (60 / (p_ptr->drain_exp + 2))))
//		take_xp_hit(Ind, 1 + p_ptr->lev / 5 + p_ptr->max_exp / 50000L, "Draining", TRUE, FALSE, FALSE);
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
	    (rand_int(p_ptr->wpos.wz != 0 ? 100 : (istown(&p_ptr->wpos) ? 0 : 300)) == 1) &&
	    (get_skill(p_ptr, SKILL_HSUPPORT) < 50) && magik(100 - p_ptr->antimagic)) {
		msg_print(Ind, "An ancient foul curse shakes your body!");
#if 0
		if (rand_int(2))
		(void)do_dec_stat(Ind, rand_int(6), STAT_DEC_NORMAL);
		else
		lose_exp(Ind, (p_ptr->exp / 100) * MON_DRAIN_LIFE);
		/* take_xp_hit(Ind, 1 + p_ptr->lev / 5 + p_ptr->max_exp / 50000L, "an ancient foul curse", TRUE, TRUE, TRUE); */
#else
		(void)do_dec_stat(Ind, rand_int(6), STAT_DEC_NORMAL);
#endif
	}
	/* and DG_CURSE randomly summons a monster (non-unique) */
	if (p_ptr->dg_curse && (rand_int(200) == 0) && !istown(&p_ptr->wpos) &&
	    (get_skill(p_ptr, SKILL_HSUPPORT) < 50) && magik(100 - p_ptr->antimagic)) {
		msg_print(Ind, "An ancient morgothian curse calls out!");
		summon_specific(&p_ptr->wpos, p_ptr->py, p_ptr->px, p_ptr->lev + 20, 100, 0, 0, 0);
	}

	/* Handle experience draining.  In Oangband, the effect
	 * is worse, especially for high-level characters.
	 * As per Tolkien, hobbits are resistant.
	 */
	if ((p_ptr->black_breath || p_ptr->black_breath_tmp) &&
	    rand_int((get_skill(p_ptr, SKILL_HCURING) >= 50) ? 250 : 150) < (p_ptr->prace == RACE_HOBBIT ? 2 : 5))
	{
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
	if (p_ptr->drain_life || p_ptr->runetrap_drain_life) {
		int drain = (p_ptr->drain_life + p_ptr->runetrap_drain_life) * (rand_int(p_ptr->mhp / 100) + 1);
		take_hit(Ind, drain < p_ptr->chp ? drain : p_ptr->chp, "life draining", 0);
	}

	/* Note changes */
	j = 0;

	/* Process equipment */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		/* Get the object */
		o_ptr = &p_ptr->inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Recharge activatable objects */
		/* (well, light-src should be handled here too? -Jir- */
		if ((o_ptr->timeout > 0) && ((o_ptr->tval != TV_LITE) || o_ptr->name1) &&
		    !((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)))
		{
			/* Recharge */
			o_ptr->timeout--;

			/* Notice changes */
			if (!(o_ptr->timeout)) j++;
		}
	}

	/* Recharge rods */
	/* this should be moved to 'timeout'? */
	for (i = 0; i < INVEN_PACK; i++) {
		o_ptr = &p_ptr->inventory[i];

		/* Examine all charging rods */
		if ((o_ptr->tval == TV_ROD) && (o_ptr->pval)) {
			/* Charge it */
			o_ptr->pval--;

			/* Charge it further */
			if (o_ptr->pval && magik(p_ptr->skill_dev))
				o_ptr->pval--;

			/* Notice changes */
			if (!(o_ptr->pval)) j++;
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

				for (j = 1; j < NumPlayers + 1; j++) {
					if (Ind == j) continue;
					if (admin_p(j)) continue;
					q_ptr = Players[j];
					if (!inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;
					if (q_ptr->afk) continue;

					bye = TRUE;
					break;
				}

				if (bye) store_kick(Ind, TRUE);
				else p_ptr->tim_store = STORE_TURNOUT;
			}
		}
	}

	if (p_ptr->tim_blacklist && !p_ptr->afk) {
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
//		if (!p_ptr->pstealing) msg_print(Ind, "You're calm enough to steal from another player.");
		if (!p_ptr->pstealing) msg_print(Ind, "You're calm enough to attempt to steal something.");
	}

	/* Delayed Word-of-Recall */
	if (p_ptr->word_recall) {
		/* Count down towards recall */
		p_ptr->word_recall--;

		/* MEGA HACK: no recall if icky, or in a shop */
		if( ! p_ptr->word_recall ) {
			//				if(p_ptr->anti_tele ||
			if ((p_ptr->store_num != -1) || p_ptr->anti_tele ||
			    (check_st_anchor(&p_ptr->wpos, p_ptr->py, p_ptr->px) &&
			     !p_ptr->admin_dm) ||
			    zcave[p_ptr->py][p_ptr->px].info&CAVE_STCK)
			{
				p_ptr->word_recall++;
			}
		}

		/* XXX Rework needed! */
		/* Activate the recall */
		if (!p_ptr->word_recall) do_recall(Ind, 0);
	}

	bypass_invuln = FALSE;

	/* Evileye, please tell me if it's right */
	if (p_ptr->tim_wraith) {
		if(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) {
			p_ptr->tim_wraith = 0;
			msg_print(Ind, "You lose your wraith powers.");
			msg_format_near(Ind, "%s loses %s wraith powers.", p_ptr->name, p_ptr->male ? "his":"her");
		}
	}

	/* No wraithform on NO_MAGIC levels - C. Blue */
	if (p_ptr->tim_wraith) {
		if (p_ptr->wpos.wz && l_ptr && (l_ptr->flags1 & LF1_NO_MAGIC)) {
			p_ptr->tim_wraith = 0;
			msg_print(Ind, "You lose your wraith powers.");
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

	if(c_ptr->feat==FEAT_AGOAL || c_ptr->feat==FEAT_BGOAL){
		int ball;
		switch(gametype){
			/* rugby type game */
			case EEGAME_RUGBY:
				if((ball=has_ball(p_ptr))==-1) break;

				if(p_ptr->team==1 && c_ptr->feat==FEAT_BGOAL){
					teamscore[0]++;
					msg_format_near(Ind, "\377R%s scored a goal!!!", p_ptr->name);
					msg_print(Ind, "\377rYou scored a goal!!!");
					score=1;
				}
				if(p_ptr->team==2 && c_ptr->feat==FEAT_AGOAL){
					teamscore[1]++;
					msg_format_near(Ind, "\377B%s scored a goal!!!", p_ptr->name);
					msg_print(Ind, "\377gYou scored a goal!!!");
					score=1;
				}
				if(score){
					object_type tmp_obj;
					s16b ox, oy;
					int try;
					p_ptr->energy=0;
					snprintf(sstr, 80, "Score: \377RReds: %d  \377BBlues: %d", teamscore[0], teamscore[1]); 
					msg_broadcast(0, sstr);

					for(try=0; try<1000; try++){
						ox=p_ptr->px+5-rand_int(10);
						oy=p_ptr->py+5-rand_int(10);
						if(!in_bounds(oy, ox)) continue;
						if(!cave_floor_bold(zcave, oy, ox)) continue;
						tmp_obj=p_ptr->inventory[ball];
						tmp_obj.marked2 = ITEM_REMOVAL_NEVER;
						drop_near(&tmp_obj, -1, &p_ptr->wpos, oy, ox);
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
static void process_player_end(int Ind)
{
	player_type *p_ptr = Players[Ind];

//	int		x, y, i, j, new_depth, new_world_x, new_world_y;
//	int		regen_amount, NumPlayers_old=NumPlayers;
	char		attackstatus;

//	byte			*w_ptr;

	/* slower 'running' movement over certain terrain */
	int real_speed = cfg.running_speed;
	cave_type *c_ptr, **zcave;
	if(!(zcave = getcave(&p_ptr->wpos))) return;
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

	/* Try to execute any commands on the command queue. */
	process_pending_commands(p_ptr->conn);

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

		/* test ftk type and use according energy requirements */
		if (p_ptr->shooting_till_kill) {
			if (target_okay(Ind)) {
				/* spells always require 1 turn: */
				if (p_ptr->shoot_till_kill_spell) ;
				/* note: currently doesn't take into account R_MELE bpr feature, which reduces energy: */
				else if (p_ptr->shoot_till_kill_rune_spell) energy = rspell_time(Ind, p_ptr->shoot_till_kill_rune_modifier);
				/* shooting with ranged weapon: */
				else energy = energy / p_ptr->num_fire;
			} else {
				p_ptr->shooting_till_kill = FALSE;
				/* normal auto-retaliation */
				//MIGHT NOT BE A MELEE ATTACK, SO COMMENTED OUT!  else energy = energy / p_ptr->num_blow;
			}
		}
		/* normal auto-retaliation */
		//MIGHT NOT BE A MELEE ATTACK, SO COMMENTED OUT!  else energy = energy / p_ptr->num_blow;


		/* Check for auto-retaliate */
		if (p_ptr->energy >= energy) {
			/* assume nothing will happen here */
			p_ptr->auto_retaliating = FALSE;

			if (p_ptr->shooting_till_kill) {
				/* stop shooting till kill if target is no longer available,
				   instead of casting a final time into thin air! */
				if (target_okay(Ind)) {
					p_ptr->auto_retaliating = TRUE;

					if (p_ptr->shoot_till_kill_spell) {
						cast_school_spell(Ind, p_ptr->shoot_till_kill_book, p_ptr->shoot_till_kill_spell - 1, 5, -1, 0);
						if (!p_ptr->shooting_till_kill) p_ptr->shoot_till_kill_spell = 0;
					}
#ifdef ENABLE_RCRAFT
					else if (p_ptr->shoot_till_kill_rune_spell) {
						execute_rspell(Ind, 5, p_ptr->shoot_till_kill_rune_spell, p_ptr->shoot_till_kill_rune_modifier, 0);
					}
#endif
					else {
						do_cmd_fire(Ind, 5);
					}

					p_ptr->auto_retaliating = !p_ptr->auto_retaliating; /* hack, it's unset in do_cmd_fire IF it WAS successfull, ie reverse */
					//not required really	if (p_ptr->ranged_double && p_ptr->shooting_till_kill) do_cmd_fire(Ind, 5);
					p_ptr->shooty_till_kill = FALSE; /* if we didn't succeed shooting till kill, then we don't intend it anymore */
				} else {
					p_ptr->shooting_till_kill = FALSE;
				}
			}

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
		} else {
			p_ptr->auto_retaliating = FALSE; /* if no energy left, this is required to turn off the no-run-while-retaliate-hack */
		}
	}


	/* ('Handle running' from above was originally at this place) */
	/* Handle running -- 5 times the speed of walking */
	while (p_ptr->running && p_ptr->energy >= (level_speed(&p_ptr->wpos) * (real_speed + 1))/real_speed) {
		run_step(Ind, 0);
		p_ptr->energy -= level_speed(&p_ptr->wpos) / real_speed;
	}


	/* Notice stuff */
	if (p_ptr->notice) notice_stuff(Ind);

	/* XXX XXX XXX Pack Overflow */
	pack_overflow(Ind);

	process_games(Ind);

	/* Process things such as regeneration. */
	/* This used to be processed every 10 turns, but I am changing it to be
	 * processed once every 5/6 of a "dungeon turn". This will make healing
	 * and poison faster with respect to real time < 1750 feet and slower >
	 * 1750 feet. - C. Blue
	 */
	if (!(turn % (level_speed(&p_ptr->wpos) / 12))) {
		if (!process_player_end_aux(Ind)) return;
	}


	/* HACK -- redraw stuff a lot, this should reduce perceived latency. */
	/* This might not do anything, I may have been silly when I added this. -APD */
	/* Notice stuff (if needed) */
	if (p_ptr->notice) notice_stuff(Ind);

	/* Update stuff (if needed) */
	if (p_ptr->update) update_stuff(Ind);

//	if(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) p_ptr->tim_wraith = 0;
	
	/* Redraw stuff (if needed) */
	if (p_ptr->redraw) redraw_stuff(Ind);

	/* Redraw stuff (if needed) */
	if (p_ptr->window) window_stuff(Ind);
}


bool stale_level(struct worldpos *wpos, int grace)
{
	time_t now;

	/* Hack -- towns are static for good. */
//	if (istown(wpos)) return (FALSE);

	now = time(&now);
	if(wpos->wz) {
		struct dungeon_type *d_ptr;
		struct dun_level *l_ptr;
		d_ptr = getdungeon(wpos);
		if(!d_ptr) return(FALSE);
		l_ptr = &d_ptr->level[ABS(wpos->wz) - 1];
#if DEBUG_LEVEL > 1
		s_printf("%s  now:%d last:%d diff:%d grace:%d players:%d\n", wpos_format(0, wpos), now, l_ptr->lastused, now-l_ptr->lastused,grace, players_on_depth(wpos));
#endif
		if(now - l_ptr->lastused > grace){
			return(TRUE);
		}
	}
	else if(now - wild_info[wpos->wy][wpos->wx].lastused > grace){
		/* Never allow dealloc where there are houses */
		/* For now at least */
#if 0
		int i;

		for(i = 0; i < num_houses; i++){
			if(inarea(wpos, &houses[i].wpos)){
				if(!(houses[i].flags&HF_DELETED))
					return(FALSE);
			}
		}
#endif

		return(TRUE);
	}
	/*
	else if(now-wild_info[wpos->wy][wpos->wx].lastused > grace)
		return(TRUE);
	 */
	return(FALSE);
}

static void do_unstat(struct worldpos *wpos)
{
	int num_on_depth = 0;
	int j;
	player_type *p_ptr;


	/* Highlander Tournament sector00 is static while players are in dungeon! */
	if (wpos->wx == WPOS_SECTOR00_X && wpos->wy == WPOS_SECTOR00_Y && wpos->wz == WPOS_SECTOR00_Z && sector00separation) return;

	/* Arena Monster Challenge */
	if (ge_special_sector &&
	    wpos->wx == WPOS_ARENA_X && wpos->wy == WPOS_ARENA_Y &&
	    wpos->wz == WPOS_ARENA_Z) return;


	// Count the number of players actually in game on this depth
	for (j = 1; j < NumPlayers + 1; j++) {
		p_ptr = Players[j];
		if (inarea(&p_ptr->wpos, wpos)) num_on_depth++;
	}
	// If this level is static and no one is actually on it
	if (!num_on_depth)
//			&& stale_level(wpos))
	{
		/* makes levels between 50ft and min_unstatic_level unstatic on player saving/quiting game/leaving level DEG */
		if ((( getlevel(wpos) < cfg.min_unstatic_level) && (0 < cfg.min_unstatic_level)) ||
		    stale_level(wpos, cfg.level_unstatic_chance * getlevel(wpos) * 60))
			new_players_on_depth(wpos,0,FALSE);
	}
#if 0
	// If this level is static and no one is actually on it
	if (!num_on_depth && stale_level(wpos))
	{
		/* makes levels between 50ft and min_unstatic_level unstatic on player saving/quiting game/leaving level DEG */
		if (( getlevel(wpos) < cfg.min_unstatic_level) && (0 < cfg.min_unstatic_level)) {
			new_players_on_depth(wpos, 0, FALSE);
		}
		// random chance of the level unstaticing
		// the chance is one in (base_chance * depth)/250 feet.
		if (!rand_int(((cfg.level_unstatic_chance * (getlevel(wpos) + 5)) / 5) - 1)) {
			// unstatic the level
			new_players_on_depth(wpos, 0, FALSE);
		}
	}
#endif	// 0
}

/*
 * 24 hourly scan of houses - should the odd house be owned by
 * a non player. Hopefully never, but best to save admin work.
 */
static void scan_houses()
{
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
static void purge_old()
{
	int x, y, i;

//	if (cfg.level_unstatic_chance > 0)
	{
		struct worldpos twpos;
		twpos.wz = 0;
		for(y = 0; y < MAX_WILD_Y; y++) {
			twpos.wy = y;
			for(x = 0; x < MAX_WILD_X; x++) {
				struct wilderness_type *w_ptr;
				struct dungeon_type *d_ptr;

				twpos.wx = x;
				w_ptr = &wild_info[twpos.wy][twpos.wx];

				if (cfg.level_unstatic_chance > 0 &&
				    players_on_depth(&twpos))
					do_unstat(&twpos);

				if (!players_on_depth(&twpos) && !istown(&twpos) &&
				    getcave(&twpos) && stale_level(&twpos, cfg.anti_scum))
					dealloc_dungeon_level(&twpos);

				if (w_ptr->flags & WILD_F_UP) {
					d_ptr = w_ptr->tower;
					for (i = 1; i <= d_ptr->maxdepth; i++) {
						twpos.wz = i;
						if (cfg.level_unstatic_chance > 0 &&
						    players_on_depth(&twpos))
							do_unstat(&twpos);
						
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
							do_unstat(&twpos);

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
				break;
			}
		}
	}
#endif // CHEEZELOG_LEVEL > 3
}


/* Traditional (Vanilla) houses version of cheeze()	- Jir - */
#ifndef USE_MANG_HOUSE_ONLY
void cheeze_trad_house()
{
#if CHEEZELOG_LEVEL > 3
	int i, j;
	house_type *h_ptr;
	object_type *o_ptr;

	/* check for inside a house */
	for(j = 0;j < num_houses; j++) {
		h_ptr = &houses[j];
		if(h_ptr->dna->owner_type == OT_PLAYER) {
			for (i = 0; i < h_ptr->stock_num; i++) {
				o_ptr = &h_ptr->stock[i];
				if(o_ptr->owner != h_ptr->dna->owner) {
					if(o_ptr->level > lookup_player_level(h_ptr->dna->owner))
						s_printf("Suspicious item: (%d,%d) Owned by %s, in %s's trad house(%d). (%d,%d)\n",
								o_ptr->wpos.wx, o_ptr->wpos.wy,
								lookup_player_name(o_ptr->owner),
								lookup_player_name(h_ptr->dna->owner),
								j, o_ptr->level,
								lookup_player_level(h_ptr->dna->owner));
				}
			}
		}
		else if(h_ptr->dna->owner_type == OT_PARTY) {
			int owner;
			if((owner = lookup_player_id(parties[h_ptr->dna->owner].owner))) {
				for (i = 0; i < h_ptr->stock_num; i++) {
					o_ptr = &h_ptr->stock[i];
					if(o_ptr->owner != owner) {
						if(o_ptr->level > lookup_player_level(owner))
							s_printf("Suspicious item: (%d,%d) Owned by %s, in %s party trad house(%d). (%d,%d)\n",
									o_ptr->wpos.wx, o_ptr->wpos.wy,
									lookup_player_name(o_ptr->owner),
									parties[h_ptr->dna->owner].name,
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
	for(j = 0; j < num_houses; j++){
		if(inarea(&houses[j].wpos, &o_ptr->wpos)){
			if(fill_house(&houses[j], FILL_OBJECT, o_ptr)){
				if(houses[j].dna->owner_type == OT_PLAYER){
					o_ptr->mode = lookup_player_mode(houses[j].dna->owner));
				}
				else if(houses[j].dna->owner_type == OT_PARTY){
					int owner;
					if((owner = lookup_player_id(parties[houses[j].dna->owner].owner))){
						o_ptr->mode = lookup_player_mode(owner));
					}
				}
				break;
			}
		}
	}
#endif // CHEEZELOG_LEVEL > 3
}


/* Traditional (Vanilla) houses version */
#ifndef USE_MANG_HOUSE_ONLY
void tradhouse_contents_chmod()
{
#if CHEEZELOG_LEVEL > 3
	int i, j;
	house_type *h_ptr;
	object_type *o_ptr;

	/* check for inside a house */
	for(j = 0; j < num_houses; j++)
	{
		h_ptr = &houses[j];
		if(h_ptr->dna->owner_type == OT_PLAYER)
		{
			for (i = 0; i < h_ptr->stock_num; i++)
			{
				o_ptr = &h_ptr->stock[i];
				o_ptr->mode = lookup_player_mode(houses[j].dna->owner));
			}
		}
		else if(h_ptr->dna->owner_type == OT_PARTY)
		{
			int owner;
			if((owner = lookup_player_id(parties[h_ptr->dna->owner].owner)))
			{
				for (i = 0; i < h_ptr->stock_num; i++)
				{
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

	for(i = 0; i < o_max; i++){
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
						delete_object_idx(i, TRUE);
						dcnt++;
					}
				} else if (++o_ptr->marked >= ((artifact_p(o_ptr) ||
				    (o_ptr->note && !o_ptr->owner))?
				    cfg.surface_item_removal * 3 : cfg.surface_item_removal)
				    + (o_ptr->marked2 == ITEM_REMOVAL_DEATH_WILD ? cfg.death_wild_item_removal : 0)
				    + (o_ptr->marked2 == ITEM_REMOVAL_LONG_WILD ? cfg.long_wild_item_removal : 0)
				    ) {
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
				    (o_ptr->note && !o_ptr->owner))?
				    cfg.dungeon_item_removal * 3 : cfg.dungeon_item_removal)) {
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
void store_turnover()
{
	int i, n;

	for(i = 0; i < numtowns; i++) {
		/* Maintain each shop (except home and auction house) */
//		for (n = 0; n < MAX_BASE_STORES - 2; n++)
		for (n = 0; n < max_st_idx; n++) {
			/* Maintain */
			store_maint(&town[i].townstore[n]);
		}

		/* Sometimes, shuffle the shopkeepers */
		if (rand_int(STORE_SHUFFLE) == 0) {
			/* Shuffle a random shop (except home and auction house) */
//			store_shuffle(&town[i].townstore[rand_int(MAX_BASE_STORES - 2)]);
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
static void process_various(void)
{
	int i, j;
	int h = 0, m = 0, s = 0, dwd = 0, dd = 0, dm = 0, dy = 0;
	time_t now;
	struct tm *tmp;
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

#if 0 /* might skip an hour if transition is unprecice, ie 1:59 -> 3:00 */
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
		if (cfg.auto_purge) {
			s_printf("previous server status: m_max(%d) o_max(%d)\n",
					m_max, o_max);
			compact_monsters(0, TRUE);
			compact_objects(0, TRUE);
//			compact_traps(0, TRUE);
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
		
/*		bbs_add_line("--- new day line ---"); */
	}

	if (!(turn % (cfg.fps * 10))) purge_old();

#if 0 /* disable for now - mikaelh */
	if (!(turn % (cfg.fps * 50))) {
		/* Tell the scripts that we're alive */
		update_check_file();
	}
#endif

	/* Handle certain things once a minute */
	if (!(turn % (cfg.fps * 60))) {
		monster_race *r_ptr;

		check_quests();		/* It's enough with 'once a minute', none? -Jir */

		Check_evilmeta();	/* check that evilmeta is still up */
		check_banlist();	/* unban some players */
		scan_objs();		/* scan objects and houses */

		if (dungeon_store_timer) dungeon_store_timer--; /* Timeout */
		if (dungeon_store2_timer) dungeon_store2_timer--; /* Timeout */
		if (great_pumpkin_timer > 0) great_pumpkin_timer--; /* HALLOWEEN */

		/* Update the player retirement timers */
		for (i = 1; i <= NumPlayers; i++) {
			p_ptr = Players[i];

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

		}

		/* Reswpan for kings' joy  -Jir- */
		/* Update the unique respawn timers */
		/* I moved this out of the loop above so this may need some
                 * tuning now - mikaelh */
		for (j = 1; j <= NumPlayers; j++) {
			p_ptr = Players[j];
			if (!p_ptr->total_winner) continue;

			/* Hack -- never Maggot and his dogs :) */
			i = rand_range(60,MAX_R_IDX-2);
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
//			if (i == 860 || i == 862 || i == 1032 || i == 1067 || i == 1085 || i == 1097) continue;
			if (r_ptr->level >= 98) continue; /* Not Michael either */

			if (r_ptr->flags7 & RF7_NAZGUL) continue; /* No nazguls */

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
			msg_format(j,"\377v%s rises from the dead!",(r_name + r_ptr->name));
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

int find_player(s32b id)
{
	int i;

	for (i = 1; i < NumPlayers + 1; i++) {
		player_type *p_ptr = Players[i];
		if (Players[i]->conn == NOT_CONNECTED) return 0;
		if (p_ptr->id == id) return i;
	}

	/* assume none */
	return 0;
}

int find_player_name(char *name)
{
	int i;

	for (i = 1; i < NumPlayers + 1; i++) {
		player_type *p_ptr = Players[i];

		if (!strcmp(p_ptr->name, name)) return i;
	}

	/* assume none */
	return 0;
}

static void process_player_change_wpos(int Ind)
{
	player_type *p_ptr = Players[Ind];
	worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	//worldpos twpos;
	dun_level *l_ptr;
	int d, j, x, y, startx = 0, starty = 0, m_idx, my, mx, tries, emergency_x, emergency_y, dlv = getlevel(wpos);

#ifdef ENABLE_RCRAFT
	/* remove all rune traps of this player */
	if (p_ptr->runetraps) {
		cave_type *c_ptr;
		struct c_special *cs_ptr;
		if ((zcave = getcave(&p_ptr->wpos_old))) {
			for (j = 0; j < p_ptr->runetraps; j++) {
				c_ptr = &zcave[p_ptr->runetrap_y[j]][p_ptr->runetrap_x[j]];
				if (c_ptr->feat != FEAT_RUNE_TRAP) continue; /* paranoia */
				if (!(cs_ptr = GetCS(c_ptr, CS_RUNE_TRAP))) continue; /* paranoia */

				d = cs_ptr->sc.runetrap.feat;
				cs_erase(c_ptr, cs_ptr);
				cave_set_feat_live(&p_ptr->wpos_old, p_ptr->runetrap_y[j], p_ptr->runetrap_x[j], d);
			}
		}
		remove_rune_trap_upkeep(Ind, 0, -1, -1);
	}
#endif

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
	p_ptr->voidx = 0; p_ptr->voidy = 0;
#endif

	/* being on different floors destabilizes mind fusion */
	if (p_ptr->esp_link_type && p_ptr->esp_link)
		change_mind(Ind, FALSE);

	/* Check "maximum depth" to make sure it's still correct */
	if (wpos->wz != 0 && !p_ptr->ghost) {
		if (dlv > p_ptr->max_dlv) p_ptr->max_dlv = dlv;

#ifdef SEPARATE_RECALL_DEPTHS
		j = recall_depth_idx(wpos, p_ptr);
		if (ABS(wpos->wz) > p_ptr->max_depth[j]) p_ptr->max_depth[j] = ABS(wpos->wz);
#endif
	}

	/* Make sure the server doesn't think the player is in a store */
	if (p_ptr->store_num != -1) {
#ifdef PLAYER_STORES
		if (p_ptr->store_num <= -2) {
			/* unlock the fake store again which we had occupied */
			fake_store_visited[-2 - p_ptr->store_num] = 0;
		}
#endif
		handle_store_leave(Ind);
		Send_store_kick(Ind);
	}

	/* Hack -- artifacts leave the queen/king */
	/* also checks the artifact list */
	//		if (!is_admin(p_ptr) && player_is_king(Ind))
	{
		object_type *o_ptr;
		char		o_name[ONAME_LEN];

		for (j = 0; j < INVEN_TOTAL; j++) {
			o_ptr = &p_ptr->inventory[j];
			if (!o_ptr->k_idx || !true_artifact_p(o_ptr)) continue;

			/* fix the list */
			handle_art_inumpara(o_ptr->name1);
			if (!a_info[o_ptr->name1].known && (o_ptr->ident & ID_KNOWN))
				a_info[o_ptr->name1].known = TRUE;

			if (!(cfg.fallenkings_etiquette && p_ptr->once_winner && !p_ptr->total_winner && !is_admin(p_ptr))) {
				if (!(cfg.kings_etiquette && p_ptr->total_winner && !is_admin(p_ptr))) {
					continue;
				}
			}

			if (winner_artifact_p(o_ptr)) continue;

			/* Describe the object */
			object_desc(Ind, o_name, o_ptr, TRUE, 0);

			/* Message */
			msg_format(Ind, "\377y%s bids farewell to you...", o_name);
			handle_art_d(o_ptr->name1);

			/* Eliminate the item  */
			inven_item_increase(Ind, j, -99);
			inven_item_describe(Ind, j);
			inven_item_optimize(Ind, j);

			j--;
		}
	}

	/* Reset bot hunting variables */
	p_ptr->silly_door_exp = 0;

	/* Somebody has entered an ungenerated level */
	if (players_on_depth(wpos) && !getcave(wpos)) {
		/* Allocate space for it */
		alloc_dungeon_level(wpos);

		/* Generate a dungeon level there */
		generate_cave(wpos, p_ptr);

#if 0 /* arena monster challenge - paranoia (should be generated in xtra1.c, and permanently static really) */
//copy/pasted code, non-functional as is, just put it here for later maybe
		new_players_on_depth(&wpos, 1, TRUE); /* make it static */
	        s_printf("EVENT_LAYOUT: Generating arena_tt at %d,%d,%d\n", wpos.wx, wpos.wy, wpos.wz);
	        process_dungeon_file("t_arena_tt.txt", &wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);
#endif
	}

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
	if (istownarea(wpos, 2)) {
		p_ptr->max_panel_rows = (MAX_HGT / SCREEN_HGT) * 2 - 2;
		p_ptr->max_panel_cols = (MAX_WID / SCREEN_WID) * 2 - 2;

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
#if 1
		/* Hack -- tricky formula, but needed */
		p_ptr->max_panel_rows = ((l_ptr->hgt + SCREEN_HGT / 2) / SCREEN_HGT) * 2 - 2;
		p_ptr->max_panel_cols = ((l_ptr->wid + SCREEN_WID / 2) / SCREEN_WID ) * 2 - 2;
#else
		p_ptr->max_panel_rows = (MAX_HGT / SCREEN_HGT) * 2 - 2;
		p_ptr->max_panel_cols = (MAX_WID / SCREEN_WID) * 2 - 2;
#endif	// 0

		p_ptr->cur_hgt = l_ptr->hgt;
		p_ptr->cur_wid = l_ptr->wid;

		/* only show 'dungeon explore feelings' when entering a dungeon, for now */
#ifdef DUNGEON_VISIT_BONUS
		show_floor_feeling(Ind, (p_ptr->wpos_old.wz == 0));
#else
		show_floor_feeling(Ind, FALSE);
#endif
	} else {
		p_ptr->max_panel_rows = (MAX_HGT / SCREEN_HGT) * 2 - 2;
		p_ptr->max_panel_cols = (MAX_WID / SCREEN_WID) * 2 - 2;

		p_ptr->cur_hgt = MAX_HGT;
		p_ptr->cur_wid = MAX_WID;
	}

#if (defined(ENABLE_RCRAFT) || defined(DUNGEON_VISIT_BONUS))
	wpcopy(&p_ptr->wpos_old, &p_ptr->wpos);
#endif

	/* hack -- update night/day in wilderness levels */
	if (!wpos->wz) {
		if (IS_DAY) player_day(Ind);
		else player_night(Ind);
	}

	/* Determine starting location */
	switch (p_ptr->new_level_method) {
	/* Climbed down */
	case LEVEL_RECALL_DOWN:
	case LEVEL_DOWN:  		starty = level_down_y(wpos);
					startx = level_down_x(wpos);
					break;

	/* Climbed up */
	case LEVEL_RECALL_UP:
	case LEVEL_UP:    		starty = level_up_y(wpos);
					startx = level_up_x(wpos);
					break;

	/* Teleported level */
	case LEVEL_RAND:  		starty = level_rand_y(wpos);
					startx = level_rand_x(wpos);
					break;

	/* Used ghostly travel */
	case LEVEL_PROB_TRAVEL:
	case LEVEL_GHOST: 		starty = p_ptr->py;
					startx = p_ptr->px;
					break;

	/* Over the river and through the woods */
	case LEVEL_OUTSIDE:
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
			|| (zcave[starty][startx].feat==FEAT_DEEP_WATER)
			|| (!cave_floor_bold(zcave, starty, startx)))
			&& (++tries < 10000) );
		if (tries == 10000 && emergency_x) {
			startx = emergency_x;
			starty = emergency_y;
		}
		break;
	}

	/* Highlander Tournament hack: pseudo-teleport the player
	   after he took a staircase inside the highlander dungeon */
//	if (sector00separation &&
	if (p_ptr->wpos.wx == WPOS_HIGHLANDER_DUN_X &&
	    p_ptr->wpos.wy == WPOS_HIGHLANDER_DUN_Y &&
	    (p_ptr->wpos.wz * WPOS_HIGHLANDER_DUN_Z) > 0
	    && l_ptr)
		starty = l_ptr->hgt + 1; /* for the code right below :) ("Hack -- handle smaller floors") */

	/* Hack -- handle smaller floors */
	if (l_ptr && (starty > l_ptr->hgt || startx > l_ptr->wid)) {
		/* make sure we aren't in an "icky" location */
		emergency_x = 0; emergency_y = 0; tries = 0;
		do {
			starty = rand_int(l_ptr->hgt-3)+1;
			startx = rand_int(l_ptr->wid-3)+1;
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
		if (wpos->wz == 0) {
			if((zcave[y][x].info & CAVE_ICKY) && (p_ptr->new_level_method != LEVEL_HOUSE)) continue;
			if(!(zcave[y][x].info & CAVE_ICKY) && (p_ptr->new_level_method == LEVEL_HOUSE)) continue;
		}

		/* Prevent recalling or prob-travelling into no-tele vaults and monster nests! - C. Blue */
	        if((zcave[y][x].info & (CAVE_STCK | CAVE_NEST_PIT)) &&
		    (p_ptr->new_level_method == LEVEL_RECALL_UP || p_ptr->new_level_method == LEVEL_RECALL_DOWN ||
		    p_ptr->new_level_method == LEVEL_RAND || p_ptr->new_level_method == LEVEL_OUTSIDE_RAND ||
		    p_ptr->new_level_method == LEVEL_PROB_TRAVEL))
			continue;

		/* Must be inside the level borders - mikaelh */
		if (x < 1 || y < 1 || x > p_ptr->cur_wid - 2 || y > p_ptr->cur_hgt - 2)
			continue;

		break;
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
		if ((wpos->wz==0) && (zcave[y][x].info & CAVE_ICKY))
			break;
	}
#endif /*0*/
	p_ptr->py = y;
	p_ptr->px = x;

	/* Update the player location */
	zcave[y][x].m_idx = 0 - Ind;
	cave_midx_debug(wpos, y, x, -Ind);

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

	/* Recalculate panel */
	p_ptr->panel_row = ((p_ptr->py - SCREEN_HGT / 4) / (SCREEN_HGT / 2));
	if (p_ptr->panel_row > p_ptr->max_panel_rows) p_ptr->panel_row = p_ptr->max_panel_rows;
	else if (p_ptr->panel_row < 0) p_ptr->panel_row = 0;

	p_ptr->panel_col = ((p_ptr->px - SCREEN_WID / 4) / (SCREEN_WID / 2));
	if (p_ptr->panel_col > p_ptr->max_panel_cols) p_ptr->panel_col = p_ptr->max_panel_cols;
	else if (p_ptr->panel_col < 0) p_ptr->panel_col = 0;

	/* Hack -- remove tracked monster */
	health_track(Ind, 0);

	p_ptr->redraw |= (PR_MAP);
	p_ptr->redraw |= (PR_DEPTH);

	panel_bounds(Ind);
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

	/* warning messages, mostly for newbies */
	if (p_ptr->max_plv == 1 &&
	    p_ptr->num_blow == 1 && p_ptr->warning_bpr2 != 1 &&
	    /* and don't spam Martial Arts users or mage-staff wielders ;) */
	    p_ptr->inventory[INVEN_WIELD].k_idx && is_weapon(p_ptr->inventory[INVEN_WIELD].tval)) {
		p_ptr->warning_bpr2 = p_ptr->warning_bpr3 = 1;
		msg_print(Ind, "\374\377yWARNING! You can currently perform only ONE melee attack per round.");
		msg_print(Ind, "\374\377y    If you rely on melee combat, it is strongly advised to try and");
		msg_print(Ind, "\374\377y    get AT LEAST TWO blows/round (press shift+c to check the #).");
		msg_print(Ind, "\374\377yPossible reasons are: Weapon is too heavy; too little STR or DEX; You");
		msg_print(Ind, "\374\377y    just equipped too heavy armour or a shield - depending on your class.");
		msg_print(Ind, "\374\377y    Also, some classes can dual-wield to get an extra blow/round.");
		s_printf("warning_bpr23: %s\n", p_ptr->name);
	} else if (p_ptr->max_plv == 1 &&
	    p_ptr->num_blow == 1 && !p_ptr->inventory[INVEN_WIELD].k_idx &&
	    p_ptr->warning_wield == 0) {
		p_ptr->warning_wield = 1;
		msg_print(Ind, "\374\377yWARNING: You don't wield a weapon at the moment!");
		msg_print(Ind, "\374\377y    Press '\377Rw\377y' key. It lets you pick a weapon (as well as other items)");
		msg_print(Ind, "\374\377y    from your inventory to wield!");
		msg_print(Ind, "\374\377y    (If you plan to train 'Martial Arts' skill, ignore this warning.)");
		s_printf("warning_wield: %s\n", p_ptr->name);
	}
	else if (p_ptr->max_plv <= 3 && p_ptr->warning_run == 0) {
		p_ptr->warning_run = 1;
		msg_print(Ind, "\374\377yHINT: To run fast, use \377RSHIFT + direction\377y keys.");
		msg_print(Ind, "\374\377y      For that, Numlock key must be OFF and no awake monster in sight!");
		s_printf("warning_run: %s\n", p_ptr->name);
	}
	else if (p_ptr->max_plv <= 5 && p_ptr->cur_lite == 0 &&
	    (p_ptr->wpos.wz < 0 || (p_ptr->wpos.wz == 0 && night_surface)) //Training Tower currently exempt
	    && p_ptr->warning_lite == 0) {
		if (p_ptr->wpos.wz < 0) p_ptr->warning_lite = 1;
		msg_print(Ind, "\374\377yHINT: You don't wield any light source at the moment!");
		msg_print(Ind, "\374\377y      Press '\377Rw\377y'. It lets you wield a torch or lantern (and other items).");
		s_printf("warning_lite: %s\n", p_ptr->name);
	}

	/* Hack -- jail her/him */
	if (!p_ptr->wpos.wz && p_ptr->tim_susp)
		imprison(Ind, 0, "old crimes");

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

#ifdef CLIENT_SIDE_WEATHER 
	/* update his client-side weather */
	player_weather(Ind, TRUE, TRUE, TRUE);
#endif

	/* Brightly lit Arena Monster Challenge */
	if (ge_special_sector && p_ptr->wpos.wx == WPOS_ARENA_X &&
	    p_ptr->wpos.wy == WPOS_ARENA_Y && p_ptr->wpos.wz == WPOS_ARENA_Z)
		wiz_lite(Ind);

	if (p_ptr->wpos.wx == WPOS_PVPARENA_X && p_ptr->wpos.wy == WPOS_PVPARENA_Y && p_ptr->wpos.wz == WPOS_PVPARENA_Z) {
		/* teleport after entering [the PvP arena],
		   so stairs won't be camped */
		teleport_player(Ind, 200, TRUE);
		/* Brightly lit PvP Arena */
		wiz_lite(Ind);
	}

	/* PvP-Mode specialties: */
	if ((p_ptr->mode & MODE_PVP) && p_ptr->wpos.wz &&
	    !(p_ptr->wpos.wx == WPOS_PVPARENA_X && p_ptr->wpos.wy == WPOS_PVPARENA_Y && p_ptr->wpos.wz == WPOS_PVPARENA_Z)) { /* <- not in the pvp arena actually */
		/* anti-chicken: Don't allow a high player to 'chase' a very low player */
		for (j = 1; j < NumPlayers; j++) {
			if (Players[j]->conn == NOT_CONNECTED) continue;
			if (is_admin(Players[j])) continue;
			if (!(Players[j]->mode & MODE_PVP)) continue;
			if (!inarea(&Players[j]->wpos, &p_ptr->wpos)) continue;
			if (Players[j]->max_plv < p_ptr->max_plv - 5) {
				p_ptr->new_level_method=(p_ptr->wpos.wz > 0 ? LEVEL_RECALL_DOWN : LEVEL_RECALL_UP);
				p_ptr->recall_pos.wz = 0;
				p_ptr->recall_pos.wx = p_ptr->wpos.wx;
				p_ptr->recall_pos.wy = p_ptr->wpos.wy;
//				set_recall_timer(Ind, 1);
				p_ptr->word_recall = 1;
				msg_print(Ind, "\377yYou have sudden visions of a bawking chicken while being recalled!");
			}
		}
	}

	/* Hack: Allow players to pass trees always, while in town */
	if (istown(&p_ptr->wpos)) p_ptr->town_pass_trees = TRUE;
	else p_ptr->town_pass_trees = FALSE;

#if 0 /* since digging is pretty awesome now, this is too much, and we should be glad that treasure detection items have some use now actually! */
	/* High digging results in auto-treasure detection */
	if (get_skill(p_ptr, SKILL_DIG) >= 30) floor_detect_treasure(Ind);
#endif

	/* If he found a town in the dungeon, map it as much as a normal town would be at night */
	if (l_ptr && (l_ptr->flags1 & LF1_DUNGEON_TOWN)) player_dungeontown(Ind);

	/* Did we enter a no-tele vault? */
	if (zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) {
		msg_print(Ind, "\377DThe air in here feels very still.");
		p_ptr->redraw |= PR_DEPTH; /* hack: depth colour indicates no-tele */
		/* New: Have bgm indicate no-tele too! (done below in handle_music()) */
	}

#ifdef USE_SOUND_2010
	/* clear boss/floor-specific music */
	p_ptr->music_monster = -1;
	handle_music(Ind);
#endif
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

void dungeon(void)
{
	int i;

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
	for (i = 1; i < NumPlayers + 1; i++) {
		player_type *p_ptr = Players[i]; 
		if (p_ptr->conn == NOT_CONNECTED || !p_ptr->new_level_flag)
			continue;
		process_player_change_wpos(i);
	}

	/* Handle any network stuff */
	Net_input();
#if 0
	/* Hack -- Compact the object list occasionally */
	if (o_top + 16 > MAX_O_IDX) compact_objects(32, FALSE);

	/* Hack -- Compact the monster list occasionally */
	if (m_top + 32 > MAX_M_IDX) compact_monsters(64, FALSE);

	/* Hack -- Compact the trap list occasionally */
	if (t_top + 16 > MAX_TR_IDX) compact_traps(32, FALSE);
#endif



	/* Note -- this is the END of the last turn */

	/* Do final end of turn processing for each player */
	for (i = 1; i < NumPlayers + 1; i++) {
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

		/* Log it */
		s_printf("%s: TURN COUNTER RESET (%d)\n", showtime(), turn_overflow);

		/* Reset empty party creation times */
		for (i = 1; i < MAX_PARTIES; i++) {
			if (parties[i].members == 0) parties[i].created = 0;
		}
	}

	/* Do some queued drawing */
	process_lite_later();

	/* process some things once each second.
	   NOTE: Some of these (global events) mustn't
	   be handled _after_ a player got set to 'dead',
	   or gameplay might in very rare occasions get
	   screwed up (someone dieing when he shouldn't) - C. Blue */
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

		/* process timed shutdown (/shutrec) */
		if (shutdown_recall_timer) shutdown_recall_timer--;
	}

#ifndef CLIENT_SIDE_WEATHER
	/* Process server-side weather */
	process_weather_effect_creation();
#endif

	/* Process server-side visual special fx */
	if (season_newyearseve && fireworks && !(turn % (cfg.fps / 4)))
		process_firework_creation();



	/* Do some beginning of turn processing for each player */
	for (i = 1; i < NumPlayers + 1; i++) {
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Print queued log messages (anti-spam feature) - C. Blue */
		if (Players[i]->last_gold_drop && turn - Players[i]->last_gold_drop_timer >= cfg.fps * 2) {
			s_printf("Gold dropped (%ld by %s at %d,%d,%d) [anti-spam].\n", Players[i]->last_gold_drop, Players[i]->name, Players[i]->wpos.wx, Players[i]->wpos.wy, Players[i]->wpos.wz);
			Players[i]->last_gold_drop = 0;
			Players[i]->last_gold_drop_timer = turn;
		}

		/* Play server-side animations (consisting of cycling/random colour choices,
		   instead of animated colours which are circled client-side) */
		if (!(turn % (cfg.fps / 6))) {
			/* Flicker invisible player? (includes colour animation!) */
//			if (Players[i]->invis) update_player(i);
			if (Players[i]->invis) {
				update_player_flicker(i);
				update_player(i);
			}
			/* Otherwise only take care of colour animation */
//			else { /* && Players[i]->body_monster */
				everyone_lite_spot(&Players[i]->wpos, Players[i]->py, Players[i]->px);
//			}
		}

		/* Perform beeping players who are currently being paged by others */
		if (Players[i]->paging && !(turn % (cfg.fps / 4))) {
			Send_beep(i);
			Players[i]->paging--;
		}

		/* Actually process that player */
		process_player_begin(i);
	}

	/* Process spell effects */
	process_effects();

	/* Process projections' teleportation effects on monsters */
	for (i = 0; i < m_top; i++)
		if (m_list[m_fast[i]].do_dist) {
			teleport_away(m_fast[i], m_list[m_fast[i]].do_dist);
			m_list[m_fast[i]].do_dist = FALSE;
		}

	/* Process all of the monsters */
	if (!(turn % MONSTER_TURNS))
		process_monsters();

	/* Process programmable NPCs */
	if (!(turn % NPC_TURNS))
		process_npcs();

	/* Process all of the objects */
	if ((turn % 10) == 5) process_objects();

	/* Process the world */
	if (!(turn % 50)) {
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
		}
#ifdef ENABLE_GO_GAME
		if (cfg.runlevel == 2048 && !go_game_up) {
#else
		if (cfg.runlevel == 2048) {
#endif
			for (i = NumPlayers; i > 0 ;i--) {
				if (Players[i]->conn == NOT_CONNECTED) continue;
				/* Ignore admins that are loged in */
				if (admin_p(i)) continue;
				/* Ignore characters that are afk and not in a dungeon/tower */
//				if((Players[i]->wpos.wz == 0) && (Players[i]->afk)) continue;
				/* Ignore characters that are not in a dungeon/tower */
				if (Players[i]->wpos.wz == 0) {
					/* Don't interrupt events though */
					if (Players[i]->wpos.wx != WPOS_SECTOR00_X || Players[i]->wpos.wy != WPOS_SECTOR00_Y || !sector00separation) continue;
				}
				break;
			}
			if (!i) {
				msg_broadcast(-1, "\377o<<<Server is being updated, but will be up again in no time.>>>");
				cfg.runlevel = 2049;
			}
		}
		if (cfg.runlevel == 2047) {
			int n = 0;
			for (i = NumPlayers; i > 0 ;i--) {
				if (Players[i]->conn == NOT_CONNECTED) continue;
				/* Ignore admins that are loged in */
				if (admin_p(i)) continue;
				/* count players */
				n++;
				/* Ignore characters that are afk and not in a dungeon/tower */
//				if((Players[i]->wpos.wz == 0) && (Players[i]->afk)) continue;
				/* Ignore characters that are not in a dungeon/tower */
				if (Players[i]->wpos.wz == 0) {
					/* Don't interrupt events though */
					if (Players[i]->wpos.wx != WPOS_SECTOR00_X || Players[i]->wpos.wy != WPOS_SECTOR00_Y || !sector00separation) continue;
				}
				break;
			}
			if (!i && (n <= 7)) {
				msg_broadcast(-1, "\377o<<<Server is being updated, but will be up again in no time.>>>");
				cfg.runlevel = 2049;
			}
		}
		if (cfg.runlevel == 2046) {
			int n = 0;
			for (i = NumPlayers; i > 0 ;i--) {
				if(Players[i]->conn == NOT_CONNECTED) continue;
				/* Ignore admins that are loged in */
				if(admin_p(i)) continue;
				/* count players */
				n++;

				/* Ignore characters that are afk and not in a dungeon/tower */
//				if((Players[i]->wpos.wz == 0) && (Players[i]->afk)) continue;
				/* Ignore characters that are not in a dungeon/tower */
				if (Players[i]->wpos.wz == 0) {
					/* Don't interrupt events though */
					if (Players[i]->wpos.wx != WPOS_SECTOR00_X || Players[i]->wpos.wy != WPOS_SECTOR00_Y || !sector00separation) continue;
				}
				break;
			}
			if (!i && (n <= 4)) {
				msg_broadcast(-1, "\377o<<<Server is being updated, but will be up again in no time.>>>");
				cfg.runlevel = 2049;
			}
		}
		if (cfg.runlevel == 2045) {
			int n = 0;
			for (i = NumPlayers; i > 0 ;i--) {
				if(Players[i]->conn == NOT_CONNECTED) continue;
				/* Ignore admins that are loged in */
				if(admin_p(i)) continue;
				/* count players */
				n++;
			}
			if (!n) {
				msg_broadcast(-1, "\377o<<<Server is being updated, but will be up again in no time.>>>");
				cfg.runlevel = 2049;
			}
		}
		if (cfg.runlevel == 2044) {
			int n = 0;
			for (i = NumPlayers; i > 0 ;i--) {
				if (Players[i]->conn == NOT_CONNECTED) continue;
				/* Ignore admins that are loged in */
				if (admin_p(i)) continue;
				/* Ignore perma-afk players! */
				//if (Players[i]->afk && 
				if (is_inactive(i) >= 30 * 20) /* 20 minutes idle? */
					continue;
				/* count players */
				n++;

				/* Ignore characters that are afk and not in a dungeon/tower */
//				if((Players[i]->wpos.wz == 0) && (Players[i]->afk)) continue;
				/* Ignore characters that are not in a dungeon/tower */
				if (Players[i]->wpos.wz == 0) {
					/* Don't interrupt events though */
					if (Players[i]->wpos.wx != WPOS_SECTOR00_X || Players[i]->wpos.wy != WPOS_SECTOR00_Y || !sector00separation) continue;
				}
				break;
			}
			if (!i && (n <= 4)) {
				msg_broadcast(-1, "\377o<<<Server is being updated, but will be up again in no time.>>>");
				cfg.runlevel = 2049;
			}
		}

		if (cfg.runlevel == 2043) {
			if (shutdown_recall_timer <= 60 && shutdown_recall_state < 3) {
				msg_broadcast(0, "\374\377I*** \377RServer-shutdown in max 1 minute (auto-recall). \377I***");
				shutdown_recall_state = 3;
			}
			else if (shutdown_recall_timer <= 300 && shutdown_recall_state < 2) {
				msg_broadcast(0, "\374\377I*** \377RServer-shutdown in max 5 minutes (auto-recall). \377I***");
				shutdown_recall_state = 2;
			}
			else if (shutdown_recall_timer <= 900 && shutdown_recall_state < 1) {
				msg_broadcast(0, "\374\377I*** \377RServer-shutdown in max 15 minutes (auto-recall). \377I***");
				shutdown_recall_state = 1;
			}
			if (!shutdown_recall_timer) {
				for (i = NumPlayers; i > 0 ;i--) {
					if (Players[i]->conn == NOT_CONNECTED) continue;

					/* Don't remove loot from ghosts waiting for res */
					if (admin_p(i) || Players[i]->ghost) continue;

					if (Players[i]->wpos.wz) {
						Players[i]->recall_pos.wx = Players[i]->wpos.wx;
						Players[i]->recall_pos.wy = Players[i]->wpos.wy;
						Players[i]->recall_pos.wz = 0;
						Players[i]->new_level_method = (Players[i]->wpos.wz > 0 ? LEVEL_RECALL_DOWN : LEVEL_RECALL_UP);
						recall_player(i, "");
					}
				}
				msg_broadcast(-1, "\377o<<<Server is being updated, but will be up again in no time.>>>");
				cfg.runlevel = 2049;
			} else {
				for (i = NumPlayers; i > 0 ;i--) {
					if (Players[i]->conn == NOT_CONNECTED) continue;
					/* Ignore admins that are loged in */
					if (admin_p(i)) continue;
					/* Ignore characters that are afk and not in a dungeon/tower */
//						if((Players[i]->wpos.wz == 0) && (Players[i]->afk)) continue;
					/* Ignore characters that are not in a dungeon/tower */
					if(Players[i]->wpos.wz == 0) {
						/* Don't interrupt events though */
						if (Players[i]->wpos.wx != WPOS_SECTOR00_X || Players[i]->wpos.wy != WPOS_SECTOR00_Y || !sector00separation) continue;
					}
					break;
				}
				if (!i) {
					msg_broadcast(-1, "\377o<<<Server is being updated, but will be up again in no time.>>>");
					cfg.runlevel = 2049;
				}
			}
		}

		/* Hack -- Compact the object list occasionally */
		if (o_top + 160 > MAX_O_IDX) compact_objects(320, TRUE);

		/* Hack -- Compact the monster list occasionally */
		if (m_top + 320 > MAX_M_IDX) compact_monsters(640, TRUE);

		/* Hack -- Compact the trap list occasionally */
//		if (t_top + 160 > MAX_TR_IDX) compact_traps(320, TRUE);

		/* Tell a day passed */
//		if (((turn + (DAY_START * 10L)) % (10L * DAY)) == 0)
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

		for (i = 1; i < NumPlayers + 1; i++) {
			if (Players[i]->conn == NOT_CONNECTED)
				continue;

			/* Process the world of that player */
			process_world(i);
		}
	}

	/* Clean up Bree regularly to prevent too dangerous towns in which weaker characters cant move around */
//	if (!(turn % 650000)) { /* 650k ~ 3hours */
//	if (!(turn % (cfg.fps * 3600))) { /* 1 h */
	if (!(turn % (cfg.fps * 600))) { /* new timing after function was changed to "thin_surface_spawns()" */
		thin_surface_spawns();
//spam		s_printf("%s Surface spawns thinned.\n", showtime());
	}

	/* Process everything else */
	if (!(turn % 10)) {
		process_various();

		/* Hack -- Regenerate the monsters every hundred game turns */
		if (!(turn % 100)) regen_monsters();
	}

#ifdef DUNGEON_VISIT_BONUS
	/* Keep track of frequented dungeons, every minute */
	if (!(turn % (cfg.fps * 60))) {
		dungeon_type *d_ptr;
		wilderness_type *w_ptr;
# ifdef DUNGEON_VISIT_BONUS_DEPTHRANGE
		int depthrange;
# endif

		for (i = 1; i < NumPlayers + 1; i++) {
			player_type *p_ptr = Players[i];
			if (p_ptr->conn == NOT_CONNECTED) continue;

			if (p_ptr->wpos.wz == 0) continue;
			w_ptr = &wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx];
			if (p_ptr->wpos.wz < 0) d_ptr = w_ptr->dungeon;
			else if (p_ptr->wpos.wz > 0) d_ptr = w_ptr->tower;
			else continue; //paranoia

# if 0
			/* NOTE: We're currently counting up for -each- player in there.
			   Maybe should be changed to generic 'visited or not visited'. */
			if (dungeon_visit_frequency[d_ptr->id] < VISIT_TIME_CAP)
				dungeon_visit_frequency[d_ptr->id]++;
# else
			/* NOTE about NOTE: Not anymore. Now # of players doesn't matter: */
			if (dungeon_visit_frequency[d_ptr->id] < VISIT_TIME_CAP)
				dungeon_visit_check[d_ptr->id] = TRUE;
# endif

# ifdef DUNGEON_VISIT_BONUS_DEPTHRANGE
			/* Also keep track of which depths are mostly frequented */
			depthrange = getlevel(&p_ptr->wpos) / 10;
			/* Excempt Valinor (dlv 200) */
			if (depthrange < 20
			    && depthrange_visited[depthrange] < VISIT_TIME_CAP);
				depthrange_visited[depthrange]++;
# endif
		}

# if 1
		for (i = 1; i <= dungeon_id_max; i++)
			if (dungeon_visit_check[i]) {
				dungeon_visit_check[i] = FALSE;
				dungeon_visit_frequency[i]++;
			}
# endif
	}
	/* Decay visit frequency for all dungeons over time. */
	/* Also update the 'rest bonus' each dungeon gives:
	   Use <startfloor + 2/3 * (startfloor-endfloor)> as main comparison value. */
	if (!(turn % (cfg.fps * 60 * 10))) {
# ifdef DUNGEON_VISIT_BONUS_DEPTHRANGE
		for (i = 0; i < 20; i++) {
			if (depthrange_visited[i])
				depthrange_visited[i]--;

			/* update the depth-range scale in regards to its most contributing dungeon */
			//todo; see below
		}
# endif
		for (i = 1; i <= dungeon_id_max; i++) {
			if (dungeon_visit_frequency[i])
				dungeon_visit_frequency[i]--;

			/* update bonus of this dungeon */
# ifdef DUNGEON_VISIT_BONUS_DEPTHRANGE
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
# endif

			/* straightforward simple way without DUNGEON_VISIT_BONUS_DEPTHRANGE */
			set_dungeon_bonus(i, FALSE);
		}
	}
#endif

	/* Process day/night changes on world_surface */
	if (!(turn % HOUR)) process_day_and_night();

	/* Refresh everybody's displays */
	for (i = 1; i < NumPlayers + 1; i++) {
		player_type *p_ptr = Players[i];

		if (p_ptr->conn == NOT_CONNECTED)
			continue;

		/* Notice stuff */
		if (p_ptr->notice) notice_stuff(i);

		/* Update stuff */
		if (p_ptr->update) update_stuff(i);

		/* Redraw stuff */
		if (p_ptr->redraw) redraw_stuff(i);

		/* Window stuff */
		if (p_ptr->window) window_stuff(i);
	}

	/* Animate player's @ in his own view here on server side */
	/* This animation is only for mana shield / GOI indication */
	if (!(turn % 5))
	for (i = 1; i < NumPlayers + 1; i++) {
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
	if (go_engine_processing) go_engine_process();
#endif

	/* Send any information over the network */
	Net_output();
}

void set_runlevel(int val) {
	static bool meta=TRUE;
	switch(val) {
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
			meta=FALSE;
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
			meta=FALSE;
			break;
			/* Hack -- character edit (possessor) mode */
		case 2043:
		case 2044:
		case 2045:
		case 2046:
		case 2047:
		case 2048:
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
			meta=TRUE;
			val = 6;
			break;
	}
	time(&cfg.closetime);
	cfg.runlevel=val;
}

/*
 * Actually play a game
 *
 * If the "new_game" parameter is true, then, after loading the
 * server-specific savefiles, we will start anew.
 */
void play_game(bool new_game) {
	int h = 0, m = 0, s = 0, dwd = 0, dd = 0, dm = 0, dy = 0;
	time_t now;
	struct tm *tmp;
	//int i, n;

	/* Init the RNG */
	// Is it a good idea ? DGDGDGD --  maybe FIXME
	//	if (Rand_quick)
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
		genwild();

		/* Generate the towns */
		wild_spawn_towns();

		/* Hack -- force town surrounding types
		 * This really shouldn't be here; just a makeshift and to be
		 * replaced by multiple-town generator */
		//wild_bulldoze();

		/* Hack -- enter the world */
		turn = 1;
	}

	/* Initialize wilderness 'level' */
	init_wild_info_aux(0,0);


	/* Flash a message */
	s_printf("Please wait...\n");

	/* Flavor the objects */
	flavor_init();

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
	while(NumPlayers > 0) {
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
		sound(1, "warning", "page", SFX_TYPE_NO_OVERLAP, FALSE);
#endif
		Net_output1(1);

		/* Indicate cause */
		strcpy(p_ptr->died_from, "server shutdown");

		/* Try to save */
		if (!save_player(1)) Destroy_connection(p_ptr->conn, "Server shutdown (save failed)");

		/* Successful save */
		Destroy_connection(p_ptr->conn, "Server has been updated, please login again."); /* was "Server shutdown (save succeeded)" */
	}

	/* Save the server state */
	if (!save_server_info()) quit("Server state save failed!");

	/* Tell the metaserver that we're gone */
	Report_to_meta(META_DIE);

#ifdef ENABLE_GO_GAME
	/* Shut down Go AI engine and its pipes */
	go_engine_terminate();
#endif

	quit("Server state saved");
}

void pack_overflow(int Ind) {
	player_type *p_ptr = Players[Ind];
	
	/* XXX XXX XXX Pack Overflow */
	if (p_ptr->inventory[INVEN_PACK].k_idx) {
		object_type *o_ptr;
		int amt, i, j = 0;
		u32b f1, f2, f3, f4, f5, esp;
		char	o_name[ONAME_LEN];

		/* Choose an item to spill */
		for(i = INVEN_PACK - 1; i >= 0; i--) {
			o_ptr = &p_ptr->inventory[i];
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

			if(!check_guard_inscription( o_ptr->note, 'd' ) &&
			    !((f4 & TR4_CURSE_NO_DROP) && cursed_p(o_ptr))) {
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
//		msg_print(Ind, "\376\377oYour pack overflows!");

		/* Describe */
		object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* Message */
		msg_format(Ind, "\376\377oYour pack overflows! You drop %s.", o_name);

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
		/* check for existance of arena */
		wpos.wx = WPOS_PVPARENA_X;
		wpos.wy = WPOS_PVPARENA_Y;
		wpos.wz = WPOS_PVPARENA_Z;
		if (!(zcave = getcave(&wpos))) return;

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
//				place_monster_aux(&wpos, y, x, 866, FALSE, FALSE, 100, 0);
				place_monster_one(&wpos, y, x, 866, FALSE, FALSE, FALSE, 100, 0);
				x = (2 * 10) - 3;
				//487 storm giant
//				place_monster_aux(&wpos, y, x, 487, FALSE, FALSE, 100, 0);
				place_monster_one(&wpos, y, x, 563, FALSE, FALSE, FALSE, 100, 0);
				x = (3 * 10) - 3;
				//609 baron of hell
//				place_monster_aux(&wpos, y, x, 590, FALSE, FALSE, 100, 0);
				place_monster_one(&wpos, y, x, 487, FALSE, FALSE, FALSE, 100, 0);
				x = (4 * 10) - 3;
				//590 mature gold d
//				place_monster_aux(&wpos, y, x, 720, FALSE, FALSE, 100, 0);
				place_monster_one(&wpos, y, x, 720, FALSE, FALSE, FALSE, 100, 0);
				x = (5 * 10) - 3;
				//995 marilith, 558 colossus
//				place_monster_aux(&wpos, y, x, 558, FALSE, FALSE, 100, 0);
				place_monster_one(&wpos, y, x, 558, FALSE, FALSE, FALSE, 100, 0);
				x = (6 * 10) - 3;
				//602 bronze D, 720 barbazu
//				place_monster_aux(&wpos, y, x, 609, FALSE, FALSE, 100, 0);
				place_monster_one(&wpos, y, x, 609, FALSE, FALSE, FALSE, 100, 0);
				summon_override_checks = SO_NONE;
				timer_pvparena3++; /* start releasing cycle */
				return;
			} else if (timer_pvparena3 == 2) { /* prepare second cycle */
				summon_override_checks = SO_ALL;
				for (i = 1; i <= 6; i++) {
					y = 1;
					x = (i * 10) - 3;
					//613 hellhound is too tough, 963 aranea im_pois, 986 3-h hydra, 249 vlasta
					//440 5-h hydra, 387 4-h hydra, 341 chimaera, 301 2-h hydra, 325 gold dfly
//					place_monster_aux(&wpos, y, x, 963, FALSE, FALSE, 100, 0);
					place_monster_one(&wpos, y, x, i % 3 ? i % 2 ? 341 : 325 : 301, FALSE, FALSE, FALSE, 100, 0);
					everyone_lite_spot(&wpos, y, x);
				}
				summon_override_checks = SO_NONE;
				timer_pvparena3++; /* start releasing cycle */
				return;
			}

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
				return;
			}

			/* next time, release next monster */
			timer_pvparena2++;
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
			cast_fireworks(&wpos, rand_int(MAX_WID - 120) + 60, rand_int(MAX_HGT - 40) + 20);
		}
	} else {
		fireworks_delay--;
	}
}


/* Update all affected (ie on worldmap surface) players' client-side weather.
   NOTE: Called on opportuniy of _global_ weather undergoing any change. */
#if defined CLIENT_SIDE_WEATHER && defined CLIENT_WEATHER_GLOBAL
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

/* Manange and control weather separately for each worldmap sector,
   assumed that client-side weather is used, depending on clouds.
   NOTE: Called once per second. */
/* Note: Meanings of cloud_state (not implemented):
   1..100: growing (decrementing, until reaching 1)
   -1..-100: shrinking (incrementing, until reaching -1 or critically low radius
*/
static void process_wild_weather() {
	wilderness_type *w_ptr;
	int i;

	/* process clouds forming, dissolving
	   and moving over world map */
	for (i = 0; i < MAX_CLOUDS; i++) {
		/* does cloud exist? */
		if (cloud_dur[i]) {
			/* hack: allow dur = -1 to last forever */
			if (cloud_dur[i] > 0) {
				/* lose essence from raining/snowing */
				cloud_dur[i]--;
				/* if cloud dissolves, keep track */
				if (!cloud_dur[i]) {
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
			/* we have one more cloud now */
			clouds++;
			/* create cloud by setting it's life time */
			cloud_dur[i] = 30 + rand_int(600); /* seconds */
			/* decide on initial size (largest ie horizontal diameter) */
			cloud_dsum[i] = MAX_WID + rand_int(MAX_WID * 5);
#ifdef TEST_SERVER /* hack: fixed location for easier live testing? */
			//around Bree
			cloud_x1[i] = 32 * MAX_WID - rand_int(cloud_dsum[i] / 4);
			cloud_y1[i] = 32 * MAX_HGT;
			cloud_x2[i] = 32 * MAX_WID + rand_int(cloud_dsum[i] / 4);
			cloud_y2[i] = 32 * MAX_HGT;
			cloud_state[i] = 1;
#else
			/* decide on starting x, y world _grid_ coords (!) */
			cloud_x1[i] = rand_int(MAX_WILD_X * MAX_WID - cloud_dsum[i] / 4);
			cloud_y1[i] = rand_int(MAX_WILD_Y * MAX_HGT);
			cloud_x2[i] = rand_int(MAX_WILD_X * MAX_WID + cloud_dsum[i] / 4);
			cloud_y2[i] = rand_int(MAX_WILD_Y * MAX_HGT);
			/* decide on growing/shrinking/constant cloud state */
			cloud_state[i] = rand_int(2) ? 1 : (rand_int(2) ? rand_int(100) : -rand_int(100));
#endif
			/* decide on initial movement */
			cloud_set_movement(i);
			/* add this cloud to its initial wild sector(s) */
			cloud_move(i, TRUE);
		}
	}

	/* update players' local client-side weather if required */
	for (i = 1; i <= NumPlayers; i++) {
		w_ptr = &wild_info[Players[i]->wpos.wy][Players[i]->wpos.wx];
		/* Skip non-connected players */
		if (Players[i]->conn == NOT_CONNECTED) continue;
		/* Skip players not on world surface - player_weather() actually checks this too though */
		if (Players[i]->wpos.wz) continue;
		/* no change in local situation? nothing to do then */
		if (!w_ptr->weather_updated) continue;
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

/* Change a cloud's movement.
   (Note: Continuous movement is performed by
   clients via buffered direction & duration.)
   New note: Negative values should probably work too (inverse direction). */
static void cloud_set_movement(int i) {
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
		/* no wind */
		cloud_xm100[i] = 0;
		cloud_ym100[i] = 0;
	} else {
		/* wind */
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
#define WEATHER_GEN_TICKS 3
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
	txm = (cloud_x1[i] + (cloud_x2[i] - cloud_x1[i]) / 2) / MAX_WID;
	tym = cloud_y1[i] / MAX_HGT;
	xs = (cloud_x1[i] - (cloud_dsum[i] - (cloud_x2[i] - cloud_x1[i])) / 2) / MAX_WID;
	xd = (cloud_x2[i] + (cloud_dsum[i] - (cloud_x2[i] - cloud_x1[i])) / 2) / MAX_WID;
	ys = (cloud_y1[i] - ((cloud_dsum[i] * 87) / 200)) / MAX_HGT; /* (sin 60 deg) */
	yd = (cloud_y1[i] + ((cloud_dsum[i] * 87) / 200)) / MAX_HGT;
	/* traverse all wild sectors that could maybe be affected or
	   have been affected by this cloud, very roughly calculated
	   (just a rectangle mask), fine check is done inside. */
	for (x = xs + xs_add_prev; x < xd + xd_add_prev; x++)
	for (y = ys + ys_add_prev; y < yd + yd_add_prev; y++) {
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
			else final_cloud_in_sector = FALSE;
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
		if (x < txm) tgx = (x + 1) * MAX_WID - 1;
		else if (x > txm) tgx = x * MAX_WID;
		else tgx = cloud_x1[i] + (cloud_x2[i] - cloud_x1[i]) / 2;
		if (y < tym) tgy = (y + 1) * MAX_HGT - 1;
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
				w_ptr->weather_type = (season == SEASON_WINTER ? 2 : 1);
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

#if 0 /* no winds */
				w_ptr->weather_wind = 0; /* todo: change =p (implement winds, actually) */
#else /* winds */
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
#endif

				w_ptr->weather_intensity = (season == SEASON_WINTER) ? 5 : 8;
				w_ptr->weather_speed = (season == SEASON_WINTER) ? 3 * WEATHER_GEN_TICKS : 1 * WEATHER_GEN_TICKS;
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

 #endif /* !CLIENT_WEATHER_GLOBAL */
#endif /* CLIENT_SIDE_WEATHER */

/* calculate slow-downs of running speed due to environmental circumstances / grid features - C. Blue
   Note: a.t.m. Terrain-related slowdowns take precedence over wind-related slowdowns, cancelling them. */
void eff_running_speed(int *real_speed, player_type *p_ptr, cave_type *c_ptr) {
#if 1 /* NEW_RUNNING_FEAT */
	if (!is_admin(p_ptr) && !p_ptr->ghost && !p_ptr->tim_wraith) {
		/* are we in fact running-flying? */
		//if ((f_info[c_ptr->feat].flags1 & (FF1_CAN_FLY | FF1_CAN_RUN)) && p_ptr->fly) {
		if ((f_info[c_ptr->feat].flags1 & (FF1_CAN_FLY | FF1_CAN_RUN))) {
			/* Allow level 50 druids to run at full speed */
			if (!(p_ptr->pclass == CLASS_DRUID &&  p_ptr->lev >= 50)) {
				if (f_info[c_ptr->feat].flags1 & FF1_SLOW_FLYING_1) *real_speed /= 2;
				if (f_info[c_ptr->feat].flags1 & FF1_SLOW_FLYING_2) *real_speed /= 4;
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
void timed_shutdown(int k) {
//	msg_admins(0, format("\377w* Shutting down in %d minutes *", k));
	msg_broadcast_format(0, "\374\377I*** \377RAutomatic recall and server restart in max %d minute%s. \377I***", k, (k == 1) ? "" : "s");

	cfg.runlevel = 2043;
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
	if (!wpos->wx && !wpos->wy) return (-1);

	for (j = 0; j < MAX_D_IDX * 2; j++) {
		/* it's a dungeon that's new to us - add it! */
		if (!p_ptr->max_depth_wx[j]) {
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
	s_printf("WARNING! TOO MANY DUNGEONS!\n");
	return (-1);
}
int get_recall_depth(struct worldpos *wpos, player_type *p_ptr) {
	int i = recall_depth_idx(wpos, p_ptr);
	if (i == -1) return 0;
	return p_ptr->max_depth[i];
}
