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

/* chance of townie respawning like other monsters, in % [50] */
#ifndef HALLOWEEN
 /* Default */
 #define TOWNIE_RESPAWN_CHANCE	250
#else
 /* better for Halloween event */
 #define TOWNIE_RESPAWN_CHANCE	50
#endif

/* if defined, player ghost loses exp slowly. [10000]
 * see GHOST_XP_CASHBACK in xtra2.c also.
 */
#define GHOST_FADING	10000

/* How fast HP/SP regenerate when 'resting'. [3] */
#define RESTING_RATE	(cfg.resting_rate)

/* Chance of items damaged when drowning, in % [3] */
#define WATER_ITEM_DAMAGE_CHANCE	3

/* Maximum wilderness radius a player can travel with WoR [16] 
 * TODO: Add another method to allow wilderness travels */
#define RECALL_MAX_RANGE	24

/* duration of GoI when getting recalled.        [2] */
#define RECALL_GOI_LENGTH        3

/*
 * Return a "feeling" (or NULL) about an item.  Method 1 (Heavy).
 */
cptr value_check_aux1(object_type *o_ptr)
{
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Artifacts */
	if (artifact_p(o_ptr))
	{
		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return "terrible";

		/* Normal */
		return "special";
	}

	/* Ego-Items */
	if (ego_item_p(o_ptr))
	{
		/* Cursed/Broken */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) return "worthless";

		/* Normal */
		/* exploding ammo is excellent */
		if (((o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_ARROW) ||
		    (o_ptr->tval == TV_BOLT)) && (o_ptr->pval != 0)) return "excellent";
		
		if (object_value(0, o_ptr) < 4000) return "good";
		return "excellent";
	}

	/* Cursed items */
	if (cursed_p(o_ptr)) return "cursed";

	/* Broken items */
	if (broken_p(o_ptr)) return "broken";

	/* Valid "tval" codes */
	switch (o_ptr->tval)
	{
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


	switch (o_ptr->tval)
	{
		/* Scrolls, Potions, Wands, Staves and Rods */
		case TV_SCROLL:
		case TV_POTION:
		case TV_POTION2:
		case TV_WAND:
		case TV_STAFF:
		case TV_ROD:
		case TV_ROD_MAIN:
		{
			/* "Cursed" scrolls/potions have a cost of 0 */
			if (k_ptr->cost == 0) return "bad";//"terrible";

			/* Artifacts */
			if (artifact_p(o_ptr)) return "special";

			/* Scroll of Nothing, Apple Juice, etc. */
			if (k_ptr->cost < 3) return "worthless";

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
		}

		/* Food */
		case TV_FOOD:
		{
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

	switch (o_ptr->tval)
	{
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


	switch (o_ptr->tval)
	{
		/* Scrolls, Potions, Wands, Staves and Rods */
		case TV_SCROLL:
		case TV_POTION:
		case TV_POTION2:
		case TV_WAND:
		case TV_STAFF:
		case TV_ROD:
		{
			/* "Cursed" scrolls/potions have a cost of 0 */
			if (k_ptr->cost == 0) return "bad";//"cursed";

			/* Artifacts */
			if (artifact_p(o_ptr)) return "good";

			/* Scroll of Nothing, Apple Juice, etc. */
			if (k_ptr->cost < 3) return "average";

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
		}

		/* Food */
		case TV_FOOD:
		{
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
	char o_name[160];


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
	if (!rand_int(3000 / (get_skill_scale(p_ptr, SKILL_PRAY, 80) + 20) - 28)) ok_curse = TRUE;
#endif
	if (!ok_combat && !ok_magic && !ok_archery) return;

	heavy = (get_skill(p_ptr, SKILL_COMBAT) >= 11) ? TRUE : FALSE;
	heavy_magic = (get_skill(p_ptr, SKILL_MAGIC) >= 11) ? TRUE : FALSE;
	heavy_archery = (get_skill(p_ptr, SKILL_ARCHERY) >= 11) ? TRUE : FALSE;

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


	/*** Sense everything ***/

	/* Check everything */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
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
		switch (o_ptr->tval)
		{
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
			{
				if (ok_combat)
					feel = (heavy ? value_check_aux1(o_ptr) :
							value_check_aux2(o_ptr));
				if (heavy) felt_heavy = TRUE;
				break;
			}

			case TV_MSTAFF:
			{
				if (ok_magic)
					feel = (heavy_magic ? value_check_aux1(o_ptr) :
							value_check_aux2(o_ptr));
				if (heavy_magic) felt_heavy = TRUE;
				break;
			}

			case TV_POTION:
			case TV_POTION2:
			case TV_SCROLL:
			case TV_WAND:
			case TV_STAFF:
			case TV_ROD:
			case TV_FOOD:
			{
				if (ok_magic && !object_aware_p(Ind, o_ptr))
					feel = (heavy_magic ? value_check_aux1_magic(o_ptr) :
							value_check_aux2_magic(o_ptr));
				if (heavy_magic) felt_heavy = TRUE;
				break;
			}

			case TV_SHOT:
			case TV_ARROW:
			case TV_BOLT:
			case TV_BOW:
			case TV_BOOMERANG:
			{
				if (ok_archery || (ok_combat && magik(33)))
					feel = (heavy_archery ? value_check_aux1(o_ptr) :
							value_check_aux2(o_ptr));
				if (heavy_archery) felt_heavy = TRUE;
				break;
			}
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
		if (i >= INVEN_WIELD)
		{
			msg_format(Ind, "You feel the %s (%c) you are %s %s %s...",
			           o_name, index_to_label(i), describe_use(Ind, i),
			           ((o_ptr->number == 1) ? "is" : "are"), feel);
		}

		/* Message (inventory) */
		else
		{
			msg_format(Ind, "You feel the %s (%c) in your pack %s %s...",
			           o_name, index_to_label(i),
			           ((o_ptr->number == 1) ? "is" : "are"), feel);
		}

		suppress_message = FALSE;

		/* We have "felt" it */
		o_ptr->ident |= (ID_SENSE);
		
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
		if (!o_ptr->note || o_ptr->note_priority) o_ptr->note = quark_add(feel);
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

	/* Save the old hitpoints */
	old_chp = p_ptr->chp;

	/* Extract the new hitpoints */
	new_chp = ((s32b)p_ptr->mhp) * percent + PY_REGEN_HPBASE;
	/* Apply the healing */
	hp_player_quiet(Ind, new_chp >> 16);
	//p_ptr->chp += new_chp >> 16;   /* div 65536 */

	/* check for overflow -- this is probably unneccecary */
	if ((p_ptr->chp < 0) && (old_chp > 0)) p_ptr->chp = MAX_SHORT;

	/* handle fractional hit point healing */
	new_chp_frac = (new_chp & 0xFFFF) + p_ptr->chp_frac;	/* mod 65536 */
	if (new_chp_frac >= 0x10000L)
	{
		hp_player_quiet(Ind, 1);
		p_ptr->chp_frac = new_chp_frac - 0x10000L;
	}
	else
	{
		p_ptr->chp_frac = new_chp_frac;
	}
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


	for (k = 0; k < MAX_EFFECTS; k++)
	{
		// int e = cave[j][i].effect;
		effect_type *e_ptr = &effects[k];

		/* Skip empty slots */
		if (e_ptr->time == 0) continue;

		wpos = &e_ptr->wpos;
		if(!(zcave=getcave(wpos)))
		{
			e_ptr->time = 0;
			/* TODO - excise it */
			continue;
		}

//#ifdef ARCADE_SERVER
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

		
		/* check if it's time to process this effect now (depends on level_speed) */
		if ((turn % (e_ptr->interval * level_speed(wpos) / (level_speeds[0] * 5))) != 0) continue;

		/* Reduce duration */
		e_ptr->time--;

		/* It ends? */
		if (e_ptr->time <= 0)
		{
			erase_effects(k);
			continue;
		}

		if (e_ptr->who > 0) who = e_ptr->who;
		else
		{
			/* XXX Hack -- is the trapper online? */
			for (i = 1; i <= NumPlayers; i++)
			{
				p_ptr = Players[i];

				/* Check if they are in here */
				if (e_ptr->who == 0 - p_ptr->id)
				{
					who = 0 - i;
					break;
				}
			}
		}

		/* Storm ends if the cause is gone */
		if (e_ptr->flags & EFF_STORM && who == PROJECTOR_EFFECT)
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
		for (l = 0; l < tdi[e_ptr->rad]; l++)
		{
			j = e_ptr->cy + tdy[l];
			i = e_ptr->cx + tdx[l];
			if (!in_bounds2(wpos, j, i)) continue;

			c_ptr = &zcave[j][i];

			//if (c_ptr->effect != k) continue;
			if (c_ptr->effect != k)	/* Nothing */;
			else
			{

				if (e_ptr->time)
				{
					/* Apply damage */
					project(who, 0, wpos, j, i, e_ptr->dam, e_ptr->type,
							PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE | PROJECT_JUMP, "");

					/* Oh, destroyed? RIP */
					if (who < 0 && who != PROJECTOR_EFFECT &&
							Players[0 - who]->conn == NOT_CONNECTED)
					{
						who = PROJECTOR_EFFECT;
					}
				}
				else
				{
					c_ptr->effect = 0;
					everyone_lite_spot(wpos, j, i);
				}

				/* Hack -- notice death */
				//	if (!alive || death) return;
				/* Storm ends if the cause is gone */
				if (e_ptr->flags & EFF_STORM &&
						(who == PROJECTOR_EFFECT || Players[0 - who]->death))
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
		else if (e_ptr->flags & EFF_STORM && who > PROJECTOR_EFFECT)
		{
			p_ptr = Players[0 - who];

			e_ptr->cy = p_ptr->py;
			e_ptr->cx = p_ptr->px;

			for (l = 0; l < tdi[e_ptr->rad]; l++)
			{
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



/* turn an allocated wpos to dark night */
static void world_surface_day(struct worldpos *wpos) {
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

/* turn an allocated wpos to bright day */
static void world_surface_night(struct worldpos *wpos) {
	cave_type **zcave = getcave(wpos), *c_ptr;
	int y, x;
	int stores = 0, y1, x1, i;
	byte sx[255], sy[255];

	/* Hack -- Scan the level */
	for (y = 0; y < MAX_HGT; y++)
	for (x = 0; x < MAX_WID; x++) {
		/* Get the cave grid */
		c_ptr = &zcave[y][x];

		/* darken all */
		c_ptr->info &= ~CAVE_GLOW;
		c_ptr->info |= CAVE_DARKEN;

		if (c_ptr->feat == FEAT_SHOP) {
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

/* take care of day/night changes */
static void process_day_and_night() {

return; /* DEBUG */

	/*** Handle the "town" (stores and sunshine) ***/
	/* Hack -- Daybreak/Nighfall in town */
	bool dawn;
	struct worldpos wrpos;
	int wx, wy;
	
	/* Check for dawn */
	dawn = (!(turn % (10L * DAY)));

	wrpos.wz = 0;

#ifndef HALLOWEEN
	/* Day breaks */
	if (dawn && !fireworks) /* remain night during NEW_YEARS_EVE !*/
#else
	/* not during Halloween {>_>} */
	if (FALSE)
#endif
	{
		night_surface = FALSE;
		
		/* scan through all currently allocated world surface levels */
		for (wx = 0; wx <= 63; wx++)
		for (wy = 0; wy <= 63; wy++) {
			wrpos.wx = wx;
			wrpos.wy = wy;
			if (!getcave(&wrpos)) continue;
			world_surface_day(&wrpos);
		}
	}
		/* Night falls - but only if it was actually day so far:
		   During HALLOWEEN as well as NEW_YEARS_EVE it stays night all the time >:) (see above) */
	else if (!dawn && !night_surface) {
		night_surface = TRUE;

		/* scan through all currently allocated world surface levels */
		for (wx = 0; wx <= 63; wx++)
		for (wy = 0; wy <= 63; wy++) {
			wrpos.wx = wx;
			wrpos.wy = wy;
			if (!getcave(&wrpos)) continue;
			world_surface_night(&wrpos);
		}
	}
}

/* update a particular player's view according to day/night status of the level (must be on world surface) */
static void player_day_and_night(int Ind) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave, *c_ptr;
	byte *w_ptr;
	int x, y;

	/* Only valid if in town or wilderness, ie on world surface */
	if (wpos->wz) return;

	/* get player's level; the if-check is paranoia */
	if(!(zcave=getcave(wpos))) return;

	if (night_surface) {
				/* Message */
//				msg_print(Ind, "The sun has risen.");
//				if (p_ptr->tim_watchlist) p_ptr->tim_watchlist--;
				if (p_ptr->prace == RACE_VAMPIRE) calc_bonuses(Ind); /* daylight */
		
				/* Hack -- Scan the level */
				for (y = 0; y < MAX_HGT; y++)
				{
					for (x = 0; x < MAX_WID; x++)
					{
						/* Get the cave grid */
						c_ptr = &zcave[y][x];
						w_ptr = &p_ptr->cave_flag[y][x];

						/* Hack -- Memorize lit grids if allowed */
						if ((istown(wpos)) && (p_ptr->view_perma_grids)) *w_ptr |= CAVE_MARK;

						/* Hack -- Notice spot */
						/* XXX it's slow (sunrise/sunset lag) */
						note_spot(Ind, y, x);						
					}			
				}

			/* Night falls - but only if it was actually day so far:
			   During HALLOWEEN as well as NEW_YEARS_EVE it stays night all the time >:) (see above) */
	} else {
				/* Message  */
//				msg_print(Ind, "The sun has fallen.");
//				if (p_ptr->tim_watchlist) p_ptr->tim_watchlist--;
				if (p_ptr->prace == RACE_VAMPIRE) calc_bonuses(Ind); /* no more daylight */

				/* Hack -- Scan the level */
				for (y = 0; y < MAX_HGT; y++)
				{
					for (x = 0; x < MAX_WID; x++)
					{
						/*  Get the cave grid */
						c_ptr = &zcave[y][x];
						w_ptr = &p_ptr->cave_flag[y][x];

						/*  Darken "boring" features */
						if (cave_plain_floor_grid(c_ptr) && !(c_ptr->info & CAVE_ROOM))
						{
							/* Forget the grid */ 
							*w_ptr &= ~CAVE_MARK;
						}

						/* Hack -- Notice spot */
						note_spot(Ind, y, x);

					}
				}
	}

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);
}


/*
 * Handle certain things once every 50 game turns
 */

static void process_world(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		x, y, i;

//	int		regen_amount, NumPlayers_old=NumPlayers;

	cave_type		*c_ptr;
	byte			*w_ptr;


#if 1 /* now in 'process_day_and_night' and 'check_day_and_night' */
	/*** Handle the "town" (stores and sunshine) ***/

	/* While in town or wilderness */
	if (p_ptr->wpos.wz==0)
	{
		/* Hack -- Daybreak/Nighfall in town */
		if (!(turn % ((10L * DAY) / 2)))
		{
			bool dawn;
			struct worldpos *wpos=&p_ptr->wpos;
			cave_type **zcave;
			if(!(zcave=getcave(wpos))) return;

			/* Check for dawn */
			dawn = (!(turn % (10L * DAY)));

#ifndef HALLOWEEN
			/* Day breaks */
			if (dawn && !fireworks) /* remain night during NEW_YEARS_EVE !*/
#else
			/* not during Halloween {>_>} */
			if (FALSE)
#endif
			{
				night_surface = FALSE;

				/* Message */
				msg_print(Ind, "The sun has risen.");
//				if (p_ptr->tim_watchlist) p_ptr->tim_watchlist--;
				if (p_ptr->prace == RACE_VAMPIRE) calc_bonuses(Ind); /* daylight */
		
				/* Hack -- Scan the level */
				for (y = 0; y < MAX_HGT; y++)
				{
					for (x = 0; x < MAX_WID; x++)
					{
						/* Get the cave grid */
						c_ptr = &zcave[y][x];
						w_ptr = &p_ptr->cave_flag[y][x];

						/* Assume lit */
						c_ptr->info |= CAVE_GLOW;
						c_ptr->info &= ~CAVE_DARKEN;

						/* Hack -- Memorize lit grids if allowed */
						if ((istown(wpos)) && (p_ptr->view_perma_grids)) *w_ptr |= CAVE_MARK;

						/* Hack -- Notice spot */
						/* XXX it's slow (sunrise/sunset lag) */
						note_spot(Ind, y, x);						
					}			
				}
			}

			/* Night falls - but only if it was actually day so far:
			   During HALLOWEEN as well as NEW_YEARS_EVE it stays night all the time >:) (see above) */
			else if (!dawn && !night_surface)
			{
				int stores = 0, y1, x1;
				byte sx[255], sy[255];

				night_surface = TRUE;

				/* Message  */
				msg_print(Ind, "The sun has fallen.");
//				if (p_ptr->tim_watchlist) p_ptr->tim_watchlist--;
				if (p_ptr->prace == RACE_VAMPIRE) calc_bonuses(Ind); /* no more daylight */


				 /* Hack -- Scan the level */
				for (y = 0; y < MAX_HGT; y++)
				{
					for (x = 0; x < MAX_WID; x++)
					{
						/*  Get the cave grid */
						c_ptr = &zcave[y][x];
						w_ptr = &p_ptr->cave_flag[y][x];

						/*  Darken "boring" features */
						if (cave_plain_floor_grid(c_ptr) && !(c_ptr->info & CAVE_ROOM))
						{
							/* Forget the grid */ 
							*w_ptr &= ~CAVE_MARK;
						}
						/* darken all */
						c_ptr->info &= ~CAVE_GLOW;

						/* Hack -- Notice spot */
						note_spot(Ind, y, x);

						c_ptr->info |= CAVE_DARKEN;

						if (c_ptr->feat == FEAT_SHOP)
						{
							sx[stores] = x;
							sy[stores] = y;
							stores++;
						}
					}
					
				}  				

				/* Hack -- illuminate the stores */
				for (i = 0; i < stores; i++)
				{
					x = sx[i];
					y = sy[i];

					for (y1 = y - 1; y1 <= y + 1; y1++)
					{
						for (x1 = x - 1; x1 <= x + 1; x1++)
						{
							/* Get the grid */
							c_ptr = &zcave[y1][x1];

							/* Illuminate the store */
							//				c_ptr->info |= CAVE_ROOM | CAVE_GLOW;
							c_ptr->info |= CAVE_GLOW;
						}
					}
				}
			}

			/* Update the monsters */
			p_ptr->update |= (PU_MONSTERS);

			/* Redraw map */
			p_ptr->redraw |= (PR_MAP);

			/* Window stuff */
			p_ptr->window |= (PW_OVERHEAD);
		}
	}

	/* While in the dungeon */
	else
	{
		/*** Update the Stores ***/
		/*  Don't do this for each player.  In fact, this might be */
		/*  taken out entirely for now --KLJ-- */
	}
#endif


	/*** Process the monsters ***/
	/* Note : since monsters are added at a constant rate in real time,
	 * this corresponds in game time to them appearing at faster rates
	 * deeper in the dungeon.
	 */

	/* Check for creature generation */
	if ((!istown(&p_ptr->wpos) && (rand_int(MAX_M_ALLOC_CHANCE) == 0)) ||
	    (istown(&p_ptr->wpos) && (rand_int(TOWNIE_RESPAWN_CHANCE * ((3 / NumPlayers) + 1)) == 0)))
	{
		dun_level *l_ptr = getfloor(&p_ptr->wpos);
		/* Should we disallow those with MULTIPLY flags to spawn on surface? */
		if (!l_ptr || !(l_ptr->flags1 & LF1_NO_NEW_MONSTER))
		{
			/* Set the monster generation depth */
			monster_level = getlevel(&p_ptr->wpos);
			if (p_ptr->wpos.wz)
				(void)alloc_monster(&p_ptr->wpos, MAX_SIGHT + 5, FALSE);
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
//				!(turn % GHOST_FADING))
//				!(turn % ((5100L - p_ptr->lev * 50)*GHOST_FADING)))
				!(turn % (GHOST_FADING / p_ptr->lev * 50)))
//				(rand_int(10000) < p_ptr->lev * p_ptr->lev))
			take_xp_hit(Ind, 1 + p_ptr->lev / 5 + p_ptr->max_exp / 10000L,
					"fading", TRUE, TRUE);
#endif	// GHOST_FADING

}

/*
 * Quick hack to allow mimics to retaliate with innate powers	- Jir -
 * It's high time we redesign auto-retaliator	XXX
 */
static int retaliate_mimic_power(int Ind, int power)
{
	player_type *p_ptr = Players[Ind];

	int		i, k, num = 1;

	/* Check for "okay" spells */
	for (k = 0; k < 3; k++)
	{
		for (i = 0; i < 32; i++)
		{
			/* Look for "okay" spells */
			if (p_ptr->innate_spells[k] & (1L << i)) 
			{
				num++;
				if (num == power) return (k * 32 + i);
			}
		}
	}

	return (0);
}

/*
 * Handle items for auto-retaliation  - Jir -
 * use_old_target is *strongly* recommended to actually make use of it.
 */
static bool retaliate_item(int Ind, int item, cptr inscription)
{
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr = &p_ptr->inventory[item];
	int cost, choice = 0, spell = 0;

	if (item < 0) return FALSE;

	/* 'Do nothing' inscription */
	if (*inscription == 'x') return TRUE;

	/* Hack -- use innate power
	 * TODO: devise a generic way to activate skills for retaliation */
	if (*inscription == 'M' && get_skill(p_ptr, SKILL_MIMIC))
	{
		/* Spell to cast */
		if (*(inscription + 1))
		{
			choice = *(inscription + 1) - 96;

			/* shape-changing for retaliation is not so nice idea, eh? */
			if (choice < 3)	/* 2 polymorph powers */
			{
				do_cmd_mimic(Ind, 0, 5);//w0t0w
				return TRUE;
			}
			else
			{
				int power = retaliate_mimic_power(Ind, choice);
				if (power)
				{
					do_cmd_mimic(Ind, power + 2, 5); /* 2 polymorph powers *///w0t0w
					return TRUE;
				}
			}
		}
		return FALSE;
	}

	/* Fighter classes can use various items for this */
	if (is_fighter(p_ptr))
	{
#if 0
		/* item with {@O-} is used only when in danger */
		if (*inscription == '-' && p_ptr->chp > p_ptr->mhp / 2) return FALSE;
#endif

		switch (o_ptr->tval)
		{
			/* non-directional ones */
			case TV_SCROLL:
			{
				do_cmd_read_scroll(Ind, item);
				return TRUE;
			}

			case TV_POTION:
			{
				do_cmd_quaff_potion(Ind, item);
				return TRUE;
			}

			case TV_STAFF:
			{
				do_cmd_use_staff(Ind, item);
				return TRUE;
			}

			/* ambiguous one */
			/* basically using rod for retaliation is not so good idea :) */
			case TV_ROD:
			{
				p_ptr->current_rod = item;
				do_cmd_zap_rod(Ind, item);
				return TRUE;
			}

			case TV_WAND:
			{
				do_cmd_aim_wand(Ind, item, 5);
				return TRUE;
			}
		}
	}

	/* Accept reasonable targets:
	 * This prevents a player from getting stuck when facing a
	 * monster inside a wall.
	 */
	if (!target_able(Ind, p_ptr->target_who)) return FALSE;

	/* Spell to cast */
	if (inscription != NULL)
	{
		choice = *inscription - 96 - 1;
		if (choice < 0 || choice > 9) choice = 0;
	}

	switch (o_ptr->tval)
	{
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
			if (item == INVEN_BOW || item == INVEN_AMMO)
			{
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
			if (o_ptr->sval == SV_SPELLBOOK)
			{
				/* It's a spellbook */

				/* There's only one spell in a spellbook */
				spell = o_ptr->pval;
			}
			else
			{
				/* It's a tome */

				/* Get the spell */
				spell = exec_lua(Ind, format("return spell_x(%d, %d, %d)", o_ptr->sval, o_ptr->pval, choice));
			}

			cost = exec_lua(Ind, format("return get_mana(%d, %d)", Ind, spell));

			/* Check that it's ok... more checks needed here? */
			/* Limit amount of mana used? */
			if (!p_ptr->blind && !no_lite(Ind) && !p_ptr->confused && cost <= p_ptr->csp && 
				exec_lua(Ind, format("return is_ok_spell(%d, %d)", Ind, spell)))
			{
				cast_school_spell(Ind, item, spell, 5, -1, 0);
				return TRUE;
			}
			break;

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
static int auto_retaliate(int Ind)
{
	player_type *p_ptr = Players[Ind], *q_ptr, *p_target_ptr = NULL, *prev_p_target_ptr = NULL;
	int d, i, tx, ty, target, prev_target, item = -1;
//	char friends = 0;
	monster_type *m_ptr, *m_target_ptr = NULL, *prev_m_target_ptr = NULL;
	monster_race *r_ptr = NULL, *r_ptr2;
	cptr inscription = NULL;
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return(FALSE);

	if (p_ptr->new_level_flag) return 0;
	
	if (p_ptr->cloaked && !p_ptr->stormbringer) return 0;

	/* Just to kill compiler warnings */
	target = prev_target = 0;

	for (d = 1; d <= 9; d++)
	{
		if (d == 5) continue;

		tx = p_ptr->px + ddx[d];
		ty = p_ptr->py + ddy[d];

		if (!in_bounds(ty, tx)) continue;

		if (!(i = zcave[ty][tx].m_idx)) continue;
		if (i > 0)
		{
			m_ptr = &m_list[i];

			/* Paranoia -- Skip dead monsters */
			if (!m_ptr->r_idx) continue;

			/* Make sure that the player can see this monster */
			if (!p_ptr->mon_vis[i]) continue;

                if (p_ptr->id == m_ptr->owner && !p_ptr->stormbringer) continue;

			/* Figure out if this is the best target so far */
			if (!m_target_ptr)
			{
				prev_m_target_ptr = m_target_ptr;
				m_target_ptr = m_ptr;
				r_ptr = race_inf(m_ptr);
				prev_target = target;
				target = i;
			}
			else
			{
				/* Target dummy should always be the last one to get attacked - mikaelh */
				if (m_ptr->r_idx == 1101) continue;

				r_ptr2 = r_ptr;
				r_ptr = race_inf(m_ptr);

				/* If it is a Q, then make it our new target. */
				/* We don't handle the case of choosing between two
				 * Q's because if the player is standing next to two Q's
				 * he deserves whatever punishment he gets.
				 */
                                if (r_ptr->d_char == 'Q')
				{
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* Otherwise if it is 20 levels higher than everything
				 * else attack it.
				 */
				else if ((r_ptr->level - 20) >= r_ptr2->level)
				{
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* Otherwise if it is the most proportionatly wounded monster
				 * attack it.
				 */
				else if (m_ptr->hp * m_target_ptr->maxhp < m_target_ptr->hp * m_ptr->maxhp)
				{
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* If it is a tie attack the higher level monster */
				else if (m_ptr->hp * m_target_ptr->maxhp == m_target_ptr->hp * m_ptr->maxhp)
				{
                                        if (r_ptr->level > r_ptr2->level)
					{
						prev_m_target_ptr = m_target_ptr;
						m_target_ptr = m_ptr;
						prev_target = target;
						target = i;
					}
					/* If it is a tie attack the monster with less hit points */
                                        else if (r_ptr->level == r_ptr2->level)
					{
						if (m_ptr->hp < m_target_ptr->hp)
						{
							prev_m_target_ptr = m_target_ptr;
							m_target_ptr = m_ptr;
							prev_target = target;
							target = i;
						}
					}
				}
			}
		}
		else if (cfg.use_pk_rules != PK_RULES_NEVER)
		{
			i = -i;
			if (!(q_ptr = Players[i])) continue;

			/* Skip non-connected players */
			if (q_ptr->conn == NOT_CONNECTED) continue;

			/* Skip players we aren't hostile to */
			if (!check_hostile(Ind, i) && !p_ptr->stormbringer) continue;

			/* Skip players we cannot see */
			if (!p_ptr->play_vis[i]) continue;

			/* Figure out if this is the best target so far */
			if (p_target_ptr)
			{
				/* If we are 15 levels over the old target, make this
				 * player our new target.
				 */
				if ((q_ptr->lev - 15) >= p_target_ptr->lev)
				{
					prev_p_target_ptr = p_target_ptr;
					p_target_ptr = q_ptr;
					prev_target = target;
					target = -i;
				}
				/* Otherwise attack this player if he is more proportionatly
				 * wounded than our old target.
				 */
				else if (q_ptr->chp * p_target_ptr->mhp < p_target_ptr->chp * q_ptr->mhp)
				{
					prev_p_target_ptr = p_target_ptr;
					p_target_ptr = q_ptr;
					prev_target = target;
					target = -i;
				}
				/* If it is a tie attack the higher level player */
				else if (q_ptr->chp * p_target_ptr->mhp == p_target_ptr->chp * q_ptr->mhp)
				{
					if (q_ptr->lev > p_target_ptr->lev)
					{
						prev_p_target_ptr = p_target_ptr;
						p_target_ptr = q_ptr;
						prev_target = target;
						target = -i;
					}
					/* If it is a tie attack the player with less hit points */
					else if (q_ptr->lev == p_target_ptr->lev)
					{
						if (q_ptr->chp < p_target_ptr->chp)
						{
							prev_p_target_ptr = p_target_ptr;
							p_target_ptr = q_ptr;
							prev_target = target;
							target = -i;
						}
					}
				}
			}
			else
			{
				prev_p_target_ptr = p_target_ptr;
				p_target_ptr = q_ptr;
				prev_target = target;
				target = -i;
			}
		}
	}
	
#if 0
	/* This code is not so efficient when many monsters are on the floor..
	 * However it can be 'recycled' for automatons in the future.	- Jir -
	 */
	/* Check each monster */
	for (i = 1; i < m_max; i++)
	{
		m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip monsters that aren't at this depth */
		if(!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;

		/* Make sure that the player can see this monster */
		if (!p_ptr->mon_vis[i]) continue;

                if (p_ptr->id == m_ptr->owner) continue;

		/* A monster is next to us */
		if (distance(p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx) == 1)
		{
			/* Figure out if this is the best target so far */
			if (m_target_ptr)
			{
				/* If it is a Q, then make it our new target. */
				/* We don't handle the case of choosing between two
				 * Q's because if the player is standing next to two Q's
				 * he deserves whatever punishment he gets.
				 */
                                if (race_inf(m_ptr)->d_char == 'Q')
				{
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* Otherwise if it is 20 levels higher than everything
				 * else attack it.
				 */
                                else if ((race_inf(m_ptr)->level - 20) >=
                                                R_INFO(m_target_ptr)->level)
				{
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* Otherwise if it is the most proportionatly wounded monster
				 * attack it.
				 */
				else if (m_ptr->hp * m_target_ptr->maxhp < m_target_ptr->hp * m_ptr->maxhp)
				{
					prev_m_target_ptr = m_target_ptr;
					m_target_ptr = m_ptr;
					prev_target = target;
					target = i;
				}
				/* If it is a tie attack the higher level monster */
				else if (m_ptr->hp * m_target_ptr->maxhp == m_target_ptr->hp * m_ptr->maxhp)
				{
                                        if (race_inf(m_ptr)->level > R_INFO(m_target_ptr)->level)
					{
						prev_m_target_ptr = m_target_ptr;
						m_target_ptr = m_ptr;
						prev_target = target;
						target = i;
					}
					/* If it is a tie attack the monster with less hit points */
                                        else if (race_inf(m_ptr)->level == R_INFO(m_target_ptr)->level)
					{
						if (m_ptr->hp < m_target_ptr->hp)
						{
							prev_m_target_ptr = m_target_ptr;
							m_target_ptr = m_ptr;
							prev_target = target;
							target = i;
						}
					}
				}
			}
			else
			{
				prev_m_target_ptr = m_target_ptr;
				m_target_ptr = m_ptr;
				prev_target = target;
				target = i;
			}
		}
	}

	/* Check each player */
	for (i = 1; i <= NumPlayers; i++)
	{
		q_ptr = Players[i];

		/* Skip non-connected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;

		/* Skip players not at this depth */
		if(!inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;

		/* Skip ourselves */
		if (Ind == i) continue;

		/* Skip players we aren't hostile to */
		if (!check_hostile(Ind, i)) continue;

		/* Skip players we cannot see */
		if (!p_ptr->play_vis[i]) continue;

		/* A hostile player is next to us */
		if (distance(p_ptr->py, p_ptr->px, q_ptr->py, q_ptr->px) == 1)
		{
			/* Figure out if this is the best target so far */
			if (p_target_ptr)
			{
				/* If we are 15 levels over the old target, make this
				 * player our new target.
				 */
				if ((q_ptr->lev - 15) >= p_target_ptr->lev)
				{
					prev_p_target_ptr = p_target_ptr;
					p_target_ptr = q_ptr;
					prev_target = target;
					target = -i;
				}
				/* Otherwise attack this player if he is more proportionatly
				 * wounded than our old target.
				 */
				else if (q_ptr->chp * p_target_ptr->mhp < p_target_ptr->chp * q_ptr->mhp)
				{
					prev_p_target_ptr = p_target_ptr;
					p_target_ptr = q_ptr;
					prev_target = target;
					target = -i;
				}
				/* If it is a tie attack the higher level player */
				else if (q_ptr->chp * p_target_ptr->mhp == p_target_ptr->chp * q_ptr->mhp)
				{
					if (q_ptr->lev > p_target_ptr->lev)
					{
						prev_p_target_ptr = p_target_ptr;
						p_target_ptr = q_ptr;
						prev_target = target;
						target = -i;
					}
					/* If it is a tie attack the player with less hit points */
					else if (q_ptr->lev == p_target_ptr->lev)
					{
						if (q_ptr->chp < p_target_ptr->chp)
						{
							prev_p_target_ptr = p_target_ptr;
							p_target_ptr = q_ptr;
							prev_target = target;
							target = -i;
						}
					}
				}
			}
			else
			{
				prev_p_target_ptr = p_target_ptr;
				p_target_ptr = q_ptr;
				prev_target = target;
				target = -i;
			}
		}
	}
#endif	// 0

	/* Nothing to attack */
	if (!p_target_ptr && !m_target_ptr) return 0;

	/* Pick an item with {@O} inscription */
//	for (i = 0; i < INVEN_PACK; i++)
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		if (!p_ptr->inventory[i].tval) continue;

//		cptr inscription = (unsigned char *) quark_str(o_ptr->note);
		inscription = quark_str(p_ptr->inventory[i].note);

		/* check for a valid inscription */
		if (inscription == NULL) continue;

		/* scan the inscription for @O */
		while (*inscription != '\0')
		{
			if (*inscription == '@')
			{
				inscription++;

				/* a valid @O has been located */
				if (*inscription == 'O')
				{                       
					inscription++;
					/* Don't let @Ox stop auto-retaliation if it's
					   not on an equipped weapon at all! */
					/* Changed @Ox to work on entire equipment because
					   batty players can't wield weapons - mikaelh */
					if ((*inscription != 'x') /* || (i == INVEN_WIELD) ||
					    (i == INVEN_BOW) || (i == INVEN_AMMO) */
					    || (i >= INVEN_WIELD)) {
						item = i;
						i = INVEN_TOTAL;
						break;
					}
				}
			}
			inscription++;
		}
	}

	/* If we have a player target, attack him. */
	if (p_target_ptr)
	{
		/* set the target */
		p_ptr->target_who = target;

		/* Attack him */
		/* Stormbringer bypasses everything!! */
//		py_attack(Ind, p_target_ptr->py, p_target_ptr->px);
		if (p_ptr->stormbringer ||
				(!retaliate_item(Ind, item, inscription) && !p_ptr->afraid))
		{
			py_attack(Ind, p_target_ptr->py, p_target_ptr->px, FALSE);
		}

		/* Check if he is still alive or another targets exists */
		if ((!p_target_ptr->death) || (prev_p_target_ptr) || (m_target_ptr))
		{
			/* We attacked something */
			return 1;
		}
		else
		{
			/* Otherwise return 2 to indicate we are no longer
			 * autoattacking anything.
			 */
			return 2;
		}
	}

	/* The dungeon master does not fight his or her offspring */
	if (p_ptr->admin_dm) return 0;

	/* If we have a target to attack, attack it! */
	if (m_target_ptr)
	{
		/* set the target */
		p_ptr->target_who = target;
		if (m_target_ptr->pet) { //a pet?
			return 0;
		}

		/* Attack it */
//		py_attack(Ind, m_target_ptr->fy, m_target_ptr->fx);
		if (!retaliate_item(Ind, item, inscription) && !p_ptr->afraid)
		{
			py_attack(Ind, m_target_ptr->fy, m_target_ptr->fx, FALSE);
		}

		/* Check if it is still alive or another targets exists */
		if ((m_target_ptr->r_idx) || (prev_m_target_ptr) || (p_target_ptr))
		{
			/* We attacked something */
			return 1;
		}
		else
		{
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
		if (players_on_depth(wpos) && !getcave(wpos))
		{
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
	        zcave=getcave(&p_ptr->wpos);
                l_ptr = getfloor(&p_ptr->wpos);
		/* Get rid of annoying level flags */
		l_ptr->flags1 &= ~(LF1_NOMAP | LF1_NO_MAGIC_MAP);
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
		/* Some lil hacks */
		msg_format(Ind, "\377uYou enter the shores of Valinor..");
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
		place_monster_aux(&p_ptr->wpos, 7, 10, 1100, FALSE, FALSE, 0, 0);
		everyone_lite_spot(&p_ptr->wpos, 7, 10);
		place_monster_aux(&p_ptr->wpos, 7, 15, 1100, FALSE, FALSE, 0, 0);
		everyone_lite_spot(&p_ptr->wpos, 7, 15);
		place_monster_aux(&p_ptr->wpos, 10, 25, 1098, FALSE, FALSE, 0, 0);
		everyone_lite_spot(&p_ptr->wpos, 10, 25);
		p_ptr->update |= PU_LITE;
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
		p_ptr->update |= (PU_DISTANCE);
		redraw_stuff(Ind);
		p_ptr->auto_transport = AT_VALINOR3;
		break;
	case AT_VALINOR3:	/* Orome mumbles */
		if (turn % 300) break; /* cool down.. */
		msg_print(Ind, "\377oOrome, The Hunter, mumbles something about a spear..");
		p_ptr->auto_transport = AT_VALINOR4;
		break;
	case AT_VALINOR4:	/* Orome looks */
		if (turn % 500) break; /* cool down.. */
		msg_print(Ind, "\377oOrome, The Hunter, notices you and surprise crosses his face!");
		p_ptr->auto_transport = AT_VALINOR5;
		break;
	case AT_VALINOR5:	/* Orome laughs */
		if (turn % 500) break; /* cool down.. */
		msg_print(Ind, "\377oOrome, The Hunter, laughs out loudly!");
		set_afraid(Ind, 8);
		p_ptr->auto_transport = AT_VALINOR6;
		break;
	case AT_VALINOR6:	/* Orome offers */
		if (turn % 500) break; /* cool down.. */
		msg_print(Ind, "\377oOrome, The Hunter, offers you to stay here!");
		msg_print(Ind, "\377y(You may hit the suicide keys in order to retire here,");
		msg_print(Ind, "\377yor take the staircase back to the mundane world.)");
		p_ptr->auto_transport = 0;
		break;
	}


	/* Give the player some energy */
	p_ptr->energy += extract_energy[p_ptr->pspeed];

	/* Make sure they don't have too much */
	/* But let them store up some extra */
	/* Storing up extra energy lets us perform actions while we are running */
	//if (p_ptr->energy > (level_speed(p_ptr->dun_depth)*6)/5)
	//	p_ptr->energy = (level_speed(p_ptr->dun_depth)*6)/5;
	if (p_ptr->energy > (level_speed(&p_ptr->wpos)*2) - 1)
		p_ptr->energy = (level_speed(&p_ptr->wpos)*2) - 1;

	/* Check "resting" status */
	if (p_ptr->resting)
	{
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
		teleport_player(Ind, 40);
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

	if (f_ptr->d_frequency[0] != 0)
	{
		int i;

		for (i = 0; i < 4; i++)
		{
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
				for (l = 0; l < d; l++)
				{
					dam += randint(s);
				}

				/* Apply damage */
				project(PROJECTOR_TERRAIN, 0, &p_ptr->wpos, y, x, dam, f_ptr->d_type[i],
				        PROJECT_KILL | PROJECT_HIDE | PROJECT_JUMP, "");

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
	p_ptr=Players[victim];
	p_ptr->recall_pos.wx=wpos->wx;
	p_ptr->recall_pos.wy=wpos->wy;
	p_ptr->recall_pos.wz=wpos->wz;
	recall_player(victim, message);
}
#endif

/* actually recall a player with no questions asked */
void recall_player(int Ind, char *message){
	struct player_type *p_ptr;
	cave_type **zcave;

	p_ptr=Players[Ind];
	
	if(!p_ptr) return;
	if(!(zcave=getcave(&p_ptr->wpos))) return;	// eww

	break_cloaking(Ind);

	/* One less person here */
	new_players_on_depth(&p_ptr->wpos,-1,TRUE);

	/* Remove the player */
	zcave[p_ptr->py][p_ptr->px].m_idx = 0;
	/* Show everyone that he's left */
	everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

	msg_print(Ind, message);
	/* Forget his lite and view */
	forget_lite(Ind);
	forget_view(Ind);

	/* Log it */
	s_printf("Recalled: %s from %d,%d,%d to %d,%d,%d.\n", p_ptr->name,
	    p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz,
	    p_ptr->recall_pos.wx, p_ptr->recall_pos.wy, p_ptr->recall_pos.wz);

	wpcopy(&p_ptr->wpos, &p_ptr->recall_pos);

	/* One more person here */
	new_players_on_depth(&p_ptr->wpos,1,TRUE);

	p_ptr->new_level_flag = TRUE;

	/* He'll be safe for some turns */
	set_invuln_short(Ind, RECALL_GOI_LENGTH);	// It runs out if attacking anyway

	check_Morgoth();

	/* cancel any user recalls */
	p_ptr->word_recall=0;

        /* Did we enter a no-tele vault? */
#if 0 /* moved to process_player_change_wpos */
        if(!(zcave=getcave(&p_ptr->wpos))) return;
        if(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) msg_print(Ind, "\377DThe air in here feels very still.");
#endif
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
	bool recall_ok=TRUE;

	/* Disturbing! */
	disturb(Ind, 0, 0);

	/* special restriction for global events (Highlander Tournament) */
	if (sector00separation && !is_admin(p_ptr) && (
	    (!p_ptr->recall_pos.wx && !p_ptr->recall_pos.wy) ||
	    (!p_ptr->wpos.wx && !p_ptr->wpos.wy))) {
		if (p_ptr->global_event_temp & 0x1) {
			p_ptr->global_event_temp &= ~0x1;
		} else {
			msg_print(Ind, "A tension leaves the air around you...");
			p_ptr->redraw |= (PR_DEPTH);
			return;
		}
	}

	/* Determine the level */
	/* recalling to surface */
	if(p_ptr->wpos.wz && !bypass)
	{
		struct dungeon_type *d_ptr;
		d_ptr=getdungeon(&p_ptr->wpos);

		/* Messages */
		if((((d_ptr->flags2 & DF2_IRON || d_ptr->flags1 & DF1_FORCE_DOWN) && d_ptr->maxdepth>ABS(p_ptr->wpos.wz)) ||
		    (d_ptr->flags1 & DF1_NO_RECALL)) && !(getfloor(&p_ptr->wpos)->flags1 & LF1_IRON_RECALL)) {
			msg_print(Ind, "You feel yourself being pulled toward the surface!");
			if (!is_admin(p_ptr)) {
				recall_ok=FALSE;
				/* Redraw the depth(colour) */
	    			p_ptr->redraw |= (PR_DEPTH);
			}
		}
		if (recall_ok) {
	                msg_format(Ind, "\377uYou are transported out of %s..", d_name + d_info[d_ptr->type].name);
			if(p_ptr->wpos.wz > 0)
			{
				message="You feel yourself yanked downwards!";
				msg_format_near(Ind, "%s is yanked downwards!", p_ptr->name);
			}
			else
			{
				message="You feel yourself yanked upwards!";
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
	else if ((!(p_ptr->recall_pos.wz) || !(wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags & (WILD_F_UP|WILD_F_DOWN))) && !bypass)
	{
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
		if (dis > RECALL_MAX_RANGE && !is_admin(p_ptr))
		{
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
	else if((cfg.runlevel>4) && (cfg.runlevel <= 2048))
	{
		wilderness_type *w_ptr = &wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx];
//		wilderness_type *w_ptr = &wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx];
		/* Messages */
		if(p_ptr->recall_pos.wz < 0 && w_ptr->flags & WILD_F_DOWN)
		{
			dungeon_type *d_ptr=wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx].dungeon;

			//if(d_ptr->baselevel-p_ptr->max_dlv>2){
			if((!d_ptr->type && d_ptr->baselevel-p_ptr->max_dlv > 2) ||
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
					msg_print(Ind, "(You feel yourself yanked toward nowhere...)");
			}

			if (p_ptr->max_dlv < w_ptr->dungeon->baselevel - p_ptr->recall_pos.wz)
				p_ptr->recall_pos.wz = w_ptr->dungeon->baselevel - p_ptr->max_dlv - 1;

			if (-p_ptr->recall_pos.wz > w_ptr->dungeon->maxdepth)
				p_ptr->recall_pos.wz = 0 - w_ptr->dungeon->maxdepth;
			if(p_ptr->inval && -p_ptr->recall_pos.wz > 10)
				p_ptr->recall_pos.wz = -10;

			if (p_ptr->recall_pos.wz >= 0)
			{
				p_ptr->recall_pos.wz = 0;
			}
			else
			{
				message="You feel yourself yanked downwards!";
		                msg_format(Ind, "\377uYou are transported into %s..", d_name + d_info[d_ptr->type].name);
				msg_format_near(Ind, "%s is yanked downwards!", p_ptr->name);
			}
		}
		else if(p_ptr->recall_pos.wz > 0 && w_ptr->flags & WILD_F_UP)
		{
			dungeon_type *d_ptr=wild_info[p_ptr->recall_pos.wy][p_ptr->recall_pos.wx].tower;

			//if(d_ptr->baselevel-p_ptr->max_dlv>2){
			if((!d_ptr->type && d_ptr->baselevel-p_ptr->max_dlv > 2) ||
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
					msg_print(Ind, "(You feel yourself yanked toward nowhere...)");
			}

			if (p_ptr->max_dlv < w_ptr->tower->baselevel + p_ptr->recall_pos.wz + 1)
				p_ptr->recall_pos.wz = 0 - w_ptr->tower->baselevel + p_ptr->max_dlv + 1;

			if (p_ptr->recall_pos.wz > w_ptr->tower->maxdepth)
				p_ptr->recall_pos.wz = w_ptr->tower->maxdepth;
			if(p_ptr->inval && -p_ptr->recall_pos.wz > 10)
				p_ptr->recall_pos.wz = 10;

			if (p_ptr->recall_pos.wz <= 0)
			{
				p_ptr->recall_pos.wz = 0;
			}
			else
			{
				message="You feel yourself yanked upwards!";
		                msg_format(Ind, "\377uYou are transported into %s..", d_name + d_info[d_ptr->type].name);
				msg_format_near(Ind, "%s is yanked upwards!", p_ptr->name);
			}
		}
		else
		{
			p_ptr->recall_pos.wz = 0;
		}

		new_pos.wx = p_ptr->wpos.wx;
		new_pos.wy = p_ptr->wpos.wy;
		new_pos.wz = p_ptr->recall_pos.wz;
		if (p_ptr->recall_pos.wz == 0)
		{
			message="You feel yourself yanked toward nowhere...";
			p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
		}
		else p_ptr->new_level_method = LEVEL_RAND;
	}

	if(recall_ok)
	{
		/* back into here */
		wpcopy(&p_ptr->recall_pos, &new_pos);
		recall_player(Ind, message);
#if 0
		cave_type **zcave;
		if(!(zcave=getcave(&p_ptr->wpos))) return;	// eww

		/* One less person here */
		new_players_on_depth(&p_ptr->wpos,-1,TRUE);

		/* Remove the player */
		zcave[p_ptr->py][p_ptr->px].m_idx = 0;
		/* Show everyone that he's left */
		everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

		/* Forget his lite and view */
		forget_lite(Ind);
		forget_view(Ind);

		wpcopy(&p_ptr->wpos, &new_pos);

		/* One more person here */
		new_players_on_depth(&p_ptr->wpos,1,TRUE);

		p_ptr->new_level_flag = TRUE;

		/* He'll be safe for some turns */
		set_invuln_short(Ind, RECALL_GOI_LENGTH);	// It runs out if attacking anyway
		
		check_Morgoth();
#endif
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
	if (p_ptr->poisoned)
	{
		/* Take damage */
		take_hit(Ind, 1, "poison", p_ptr->poisoned_attacker);
	}

	/* Misc. terrain effects */
	if (!p_ptr->ghost)
	{
		/* Generic terrain effects */
		apply_effect(Ind);

		/* Drowning, but not ghosts */
		if(c_ptr->feat==FEAT_DEEP_WATER)
		{
			if((!p_ptr->fly) && (!p_ptr->can_swim))
			{
				/* Take damage */
				if (!(p_ptr->body_monster) || (
							!(r_info[p_ptr->body_monster].flags7 &
								(RF7_AQUATIC | RF7_CAN_SWIM)) &&
							!(r_info[p_ptr->body_monster].flags3&RF3_UNDEAD) ))
				{
					int hit = p_ptr->mhp>>6;
					int swim = get_skill_scale(p_ptr, SKILL_SWIM, 90);
					hit += randint(p_ptr->mhp>>5);
					if(!hit) hit=1;

					/* Take CON into consideration(max 30%) */
					hit = (hit * (80 - adj_str_wgt[p_ptr->stat_ind[A_CON]])) / 75;

					/* temporary abs weight calc */
					if(p_ptr->wt+p_ptr->total_weight/10 > 170 + swim * 2)	// 190
					{
						long factor=(p_ptr->wt+p_ptr->total_weight/10)-150-swim * 2;	// 170
						/* too heavy, always drown? */
						if(factor<300)
						{
							if(randint(factor)<20) hit=0;
						}

						/* Hack: Take STR and DEX into consideration (max 28%) */
						if (magik(adj_str_wgt[p_ptr->stat_ind[A_STR]] / 2) ||
								magik(adj_str_wgt[p_ptr->stat_ind[A_DEX]]) / 2)
							hit = 0;

						if (magik(swim)) hit = 0;

						if (hit)
						{
							msg_print(Ind,"\377rYou're drowning!");
						}

						/* harm equipments (even hit == 0) */
						if (TOOL_EQUIPPED(p_ptr) != SV_TOOL_TARPAULIN &&
								magik(WATER_ITEM_DAMAGE_CHANCE) && !p_ptr->fly &&
								!p_ptr->immune_water)
						{
						    if (!p_ptr->resist_water || magik(50)) {
							inven_damage(Ind, set_water_destroy, 1);
							equip_damage(Ind, GF_WATER);
						    }
						}

						if(randint(1000-factor)<10)
						{
							msg_print(Ind,"\377rYou are weakened by the exertion of swimming!");
							//							do_dec_stat(Ind, A_STR, STAT_DEC_TEMPORARY);
							dec_stat(Ind, A_STR, 10, STAT_DEC_TEMPORARY);
						}
						take_hit(Ind, hit, "drowning", 0);
					}
				}
			}
		}
		/* Aquatic anoxia */
		else if (c_ptr->feat!=FEAT_SHAL_WATER)
		{
			/* Take damage */
			if((p_ptr->body_monster) && (
						(r_info[p_ptr->body_monster].flags7&RF7_AQUATIC) &&
						!(r_info[p_ptr->body_monster].flags3&RF3_UNDEAD) ))
			{
				long hit = p_ptr->mhp>>6;
				hit += randint(p_ptr->chp>>5);
				if(!hit) hit=1;

				/* Take CON into consideration(max 30%) */
				hit = (hit * (80 - adj_str_wgt[p_ptr->stat_ind[A_CON]])) / 75;
				if (hit) msg_print(Ind,"\377rYou cannot breathe air!");

				if(randint(1000)<10)
				{
					msg_print(Ind,"\377rYou find it hard to stir!");
					//					do_dec_stat(Ind, A_DEX, STAT_DEC_TEMPORARY);
					dec_stat(Ind, A_DEX, 10, STAT_DEC_TEMPORARY);
				}
/*				if (p_ptr->stun < 20) set_stun(Ind, 20);
				else set_stun(Ind, p_ptr->stun + 5);
*/				take_hit(Ind, hit, "anoxia", 0);
			}
		}

		/* Spectres -- take damage when moving through walls */

		/*
		 * Added: ANYBODY takes damage if inside through walls
		 * without wraith form -- NOTE: Spectres will never be
		 * reduced below 0 hp by being inside a stone wall; others
		 * WILL BE!
		 */
		/* Seemingly the comment above is wrong, dunno */
		if (!cave_floor_bold(zcave, p_ptr->py, p_ptr->px))
		{
			/* Player can walk through trees */
			//if ((PRACE_FLAG(PR1_PASS_TREE) || (get_skill(SKILL_DRUID) > 15)) && (cave[py][px].feat == FEAT_TREE))
#if 0
			if ((p_ptr->pass_trees || p_ptr->fly) &&
					(c_ptr->feat == FEAT_TREE))
#endif	// 0
			if (player_can_enter(Ind, c_ptr->feat))
			{
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

				//			cave_no_regen = TRUE;
				if (amt > p_ptr->chp - 1) amt = p_ptr->chp - 1;
				take_hit(Ind, amt, " walls ...", 0);
			}
		}
	}


	/* Take damage from cuts */
	if (p_ptr->cut)
	{
		/* Mortal wound or Deep Gash */
		if (p_ptr->cut > 200)
		{
			i = 3;
		}

		/* Severe cut */
		else if (p_ptr->cut > 100)
		{
			i = 2;
		}

		/* Other cuts */
		else
		{
			i = 1;
		}

		/* Take damage */
		take_hit(Ind, i, "a fatal wound", p_ptr->cut_attacker);
	}

	/*** Check the Food, and Regenerate ***/

	/* Ghosts don't need food */
	/* Allow AFK-hivernation if not hungry */
	if (!p_ptr->ghost && !(p_ptr->afk && p_ptr->food > PY_FOOD_ALERT) && !p_ptr->admin_dm &&
	    /* Don't starve in town (but recover from being gorged) - C. Blue */
//	    (!istown(&p_ptr->wpos) || p_ptr->food >= PY_FOOD_MAX))
	    (!istown(&p_ptr->wpos) || p_ptr->food >= PY_FOOD_FULL)) /* allow to digest some to not get gorged in upcoming fights quickly - C. Blue */
	{
		/* Digest normally */
		if (p_ptr->food < PY_FOOD_MAX)
		{
			/* Every 50/6 level turns */
			//			        if (!(turn%((level_speed((&p_ptr->wpos))*10)/12)))
			if (!(turn%((level_speed((&p_ptr->wpos))/12)*10)))
			{
				/* Basic digestion rate based on speed */
//				i = extract_energy[p_ptr->pspeed]*2;	// 1.3 (let them starve)
				i = (10 + extract_energy[p_ptr->pspeed]*3) / 2;

				/* Adrenaline takes more food */
				if (p_ptr->adrenaline) i *= 5;

				/* Biofeedback takes more food */
				if (p_ptr->biofeedback) i *= 2;

				/* Regeneration takes more food */
				if (p_ptr->regenerate) i += 30;

                                /* Regeneration takes more food */
                                if (p_ptr->tim_regen) i += p_ptr->tim_regen_pow / 10;

				/* DragonRider and Half-Troll take more food */
				if (p_ptr->prace == RACE_DRIDER
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
		else
		{
			/* Digest a lot of food */
			(void)set_food(Ind, p_ptr->food - 100);
		}

		/* Starve to death (slowly) */
		if (p_ptr->food < PY_FOOD_STARVE)
		{
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
	if (p_ptr->food < PY_FOOD_WEAK)
	{
		/* Lower regeneration */
		if (p_ptr->food < PY_FOOD_STARVE)
		{
			regen_amount = 0;
		}
		else if (p_ptr->food < PY_FOOD_FAINT)
		{
			regen_amount = PY_REGEN_FAINT;
		}
		else
		{
			regen_amount = PY_REGEN_WEAK;
		}

		/* Getting Faint */
		if (!p_ptr->ghost && p_ptr->food < PY_FOOD_FAINT)
		{
			/* Faint occasionally */
			if (!p_ptr->paralyzed && (rand_int(100) < 10))
			{
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
	{
		regen_amount = regen_amount * RESTING_RATE;
	}

	/* Regenerate the mana */
	/* Hack -- regenerate mana 5/3 times faster */
#ifdef ARCADE_SERVER
        p_ptr->regen_mana = TRUE;
#endif
	if (p_ptr->csp < p_ptr->msp)
	{
		regenmana(Ind, (regen_amount * 5) * (p_ptr->regen_mana ? 2 : 1) / 3);
	}

	/* Regeneration ability */
	if (p_ptr->regenerate)
	{
		regen_amount = regen_amount * 2;
	}

	if (minus_health)
	{
		regen_amount = regen_amount * 3 / 2;
	}

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
	{
		regenhp(Ind, regen_amount);
	}

	/* Regenerate depleted Stamina */
	if (p_ptr->cst < p_ptr->mst)
	{
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
	if ((p_ptr->resting) && (p_ptr->chp == p_ptr->mhp) && (p_ptr->csp == p_ptr->msp) && (p_ptr->cst == p_ptr->mst))
	{
		disturb(Ind, 0, 0);
	}

	/* Finally, at the end of our turn, update certain counters. */
	/*** Timeout Various Things ***/

	/* Handle temporary stat drains */
	for (i = 0; i < 6; i++)
	{
		if (p_ptr->stat_cnt[i] > 0)
		{
			p_ptr->stat_cnt[i] -= (1 + minus_health);
			if (p_ptr->stat_cnt[i] <= 0)
			{
				do_res_stat_temp(Ind, i);
			}
		}
	}

	/* Adrenaline */
	if (p_ptr->adrenaline)
	{
		(void)set_adrenaline(Ind, p_ptr->adrenaline - 1);
	}

	/* Biofeedback */				
	if (p_ptr->biofeedback)
	{
		(void)set_biofeedback(Ind, p_ptr->biofeedback - 1);
	}

	/* Hack -- Bow Branding */
	if (p_ptr->bow_brand)
	{
		(void)set_bow_brand(Ind, p_ptr->bow_brand - minus_magic, p_ptr->bow_brand_t, p_ptr->bow_brand_d);
	}

	/* weapon brand time */
	if(p_ptr->brand){
		(void)set_brand(Ind, p_ptr->brand - minus_magic, p_ptr->brand_t, p_ptr->brand_d);
	}

	/* Hack -- Timed ESP */
	if (p_ptr->tim_esp)
	{
		(void)set_tim_esp(Ind, p_ptr->tim_esp - minus_magic);
	}

	/* Hack -- Space/Time Anchor */
	if (p_ptr->st_anchor)
	{
		(void)set_st_anchor(Ind, p_ptr->st_anchor - minus_magic);
	}

	/* Hack -- Prob travel */
	if (p_ptr->prob_travel)
	{
		(void)set_prob_travel(Ind, p_ptr->prob_travel - minus_magic);
	}

	/* Hack -- Avoid traps */
	if (p_ptr->tim_traps)
	{
		(void)set_tim_traps(Ind, p_ptr->tim_traps - minus_magic);
	}

	/* Hack -- Mimicry */
	if (p_ptr->tim_mimic)
	{
		(void)set_mimic(Ind, p_ptr->tim_mimic - 1, p_ptr->tim_mimic_what);
	}

	/* Hack -- Timed manashield */
	if (p_ptr->tim_manashield)
	{
		set_tim_manashield(Ind, p_ptr->tim_manashield - minus_magic);
		if (p_ptr->tim_manashield == 10) msg_print(Ind, "\377vThe disruption shield starts to flicker and fade...");
	}
	if (cfg.use_pk_rules == PK_RULES_DECLARE)
	{
		if(p_ptr->tim_pkill){
			p_ptr->tim_pkill--;
			if(!p_ptr->tim_pkill){
				if(p_ptr->pkill&PKILL_SET){
					msg_print(Ind, "You may now kill other players");
					p_ptr->pkill|=PKILL_KILLER;
				}
				else{
					msg_print(Ind, "You are now protected from other players");
					p_ptr->pkill&=~PKILL_KILLABLE;
				}
			}
		}
	}
	if(p_ptr->tim_jail && !p_ptr->wpos.wz){
		int jy, jx;
		p_ptr->tim_jail--;
		if(!p_ptr->tim_jail){
			msg_print(Ind, "\377GYou are free to go!");
			jy=p_ptr->py;
			jx=p_ptr->px;
			zcave[jy][jx].info &= ~CAVE_STCK;
			teleport_player_force(Ind, 1);
			zcave[jy][jx].info |= CAVE_STCK;
		}
	}
#if 0	// moved to process_player_change_wpos
	if(!p_ptr->wpos.wz && p_ptr->tim_susp){
		imprison(Ind, 0, "old crimes");
		return;
	}
#endif	// 0

	/* Hack -- Tunnel */
#if 0	// not used
	if (p_ptr->auto_tunnel)
	{
		p_ptr->auto_tunnel--;
	}
#endif	// 0

	/* Hack -- Meditation */
	if (p_ptr->tim_meditation)
	{
		(void)set_tim_meditation(Ind, p_ptr->tim_meditation - minus);
	}

	/* Hack -- Wraithform */
	if (p_ptr->tim_wraith)
	{
		(void)set_tim_wraith(Ind, p_ptr->tim_wraith - minus_magic);
	}

	/* Hack -- Hallucinating */
	if (p_ptr->image)
	{
		int adjust = 1 + minus_health;
		if (get_skill(p_ptr, SKILL_HCURING) >= 50) adjust++;
		(void)set_image(Ind, p_ptr->image - adjust);
	}

	/* Blindness */
	if (p_ptr->blind)
	{
		int adjust = 1 + minus_health;
		if (get_skill(p_ptr, SKILL_HCURING) >= 30) adjust++;
		(void)set_blind(Ind, p_ptr->blind - adjust);
	}

	/* Times see-invisible */
	if (p_ptr->tim_invis)
	{
		(void)set_tim_invis(Ind, p_ptr->tim_invis - minus_magic);
	}

	/* Times invisibility */
	if (p_ptr->tim_invisibility)
	{
		if (p_ptr->aggravate)
		{
			msg_print(Ind, "Your invisibility shield is broken by your aggravation.");
			(void)set_invis(Ind, 0, 0);
		}		    
		else
		{
			(void)set_invis(Ind, p_ptr->tim_invisibility - minus_magic, p_ptr->tim_invis_power);
		}
	}

	/* Timed infra-vision */
	if (p_ptr->tim_infra)
	{
		(void)set_tim_infra(Ind, p_ptr->tim_infra - minus_magic);
	}

	/* Paralysis */
	if (p_ptr->paralyzed)
	{
		(void)set_paralyzed(Ind, p_ptr->paralyzed - 1 - minus_health);
	}

	/* Confusion */
	if (p_ptr->confused)
	{
		(void)set_confused(Ind, p_ptr->confused - minus - minus_combat - minus_health);
	}

	/* Afraid */
	if (p_ptr->afraid)
	{
		(void)set_afraid(Ind, p_ptr->afraid - minus - minus_combat);
	}

	/* Fast */
	if (p_ptr->fast)
	{
		(void)set_fast(Ind, p_ptr->fast - minus_magic, p_ptr->fast_mod);
	}

	/* Slow */
	if (p_ptr->slow)
	{
		(void)set_slow(Ind, p_ptr->slow - minus_magic); // - minus_health
	}

	/* xtra shot? - the_sandman */
	if (p_ptr->focus_time)
	{
		(void)do_focus_shot(Ind, p_ptr->focus_time - minus_magic, p_ptr->focus_val);
	}

	/* xtra stats? - the_sandman */
	if (p_ptr->xtrastat)
	{
		(void)do_xtra_stats(Ind, p_ptr->xtrastat - minus_magic, p_ptr->statval);
	}

	/* Protection from evil */
	if (p_ptr->protevil)
	{
		(void)set_protevil(Ind, p_ptr->protevil - 1);
	}

	/* Holy Zeal - EA bonus */
	if (p_ptr->zeal)
	{
		(void)set_zeal(Ind, p_ptr->zeal_power, p_ptr->zeal - 1);
	}

	/* Heart is still boldened */	
	if (p_ptr->res_fear_temp) p_ptr->res_fear_temp--;

	/* Holy Martyr */
	if (p_ptr->martyr)
	{
		(void)set_martyr(Ind, p_ptr->martyr - 1);
	}
	if (p_ptr->martyr_timeout)
	{
		p_ptr->martyr_timeout--;
		if (!p_ptr->martyr_timeout) msg_print(Ind, "The heavens are ready to accept your martyrium.");
	}

	if (p_ptr->cloaked > 1) {
#if 0 /* done in un_afk_idle now */
		if (!p_ptr->idle_char) stop_cloaking(Ind); /* stop preparations if we perform any action */
		else 
#endif
		if (--p_ptr->cloaked == 1) {
			msg_print(Ind, "\377sYou cloaked your appearance!");
			msg_format_near(Ind, "\377w%s disappears before your eyes!", p_ptr->name);
			p_ptr->update |= (PU_BONUS | PU_MONSTERS);
			p_ptr->redraw |= (PR_STATE | PR_SPEED);
		        handle_stuff(Ind);
		}
	}

	/* Invulnerability */
	/* Hack -- make -1 permanent invulnerability */
	if (p_ptr->invuln)
	{
		if (p_ptr->invuln > 0) (void)set_invuln(Ind, p_ptr->invuln - minus_magic);
		if (p_ptr->invuln == 5) msg_print(Ind, "\377vThe invulnerability shield starts to flicker and fade...");
	}

	/* Heroism */
	if (p_ptr->hero)
	{
		(void)set_hero(Ind, p_ptr->hero - 1);
	}

	/* Super Heroism */
	if (p_ptr->shero)
	{
		(void)set_shero(Ind, p_ptr->shero - 1);
	}

	/* Furry */
	if (p_ptr->fury)
	{
		(void)set_fury(Ind, p_ptr->fury - 1);
	}

	/* Berserk #2 */
	if (p_ptr->berserk)
	{
		(void)set_berserk(Ind, p_ptr->berserk - 1);
	}

	/* Sprint? melee technique */
	if (p_ptr->melee_sprint)
	{
		(void)set_melee_sprint(Ind, p_ptr->melee_sprint - 1);
	}

	/* Blessed */
	if (p_ptr->blessed)
	{
		(void)set_blessed(Ind, p_ptr->blessed - 1);
	}

	/* Shield */
	if (p_ptr->shield)
	{
		(void)set_shield(Ind, p_ptr->shield - 1, p_ptr->shield_power, p_ptr->shield_opt, p_ptr->shield_power_opt, p_ptr->shield_power_opt2);
	}

	/* Timed Levitation */
	if (p_ptr->tim_ffall)
	{
		(void)set_tim_ffall(Ind, p_ptr->tim_ffall - 1);
	}
	if (p_ptr->tim_fly)
	{
		(void)set_tim_fly(Ind, p_ptr->tim_fly - 1);
	}

	/* Timed regen */
	if (p_ptr->tim_regen)
	{
		(void)set_tim_regen(Ind, p_ptr->tim_regen - 1, p_ptr->tim_regen_pow);
	}

	/* Thunderstorm */
	if (p_ptr->tim_thunder)
	{
                int dam = damroll(p_ptr->tim_thunder_p1, p_ptr->tim_thunder_p2);
		int i, tries = 600;
		monster_type *m_ptr = NULL;

		while (tries)
		{
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

		if (tries)
		{
			char m_name[80];

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
	{
		(void)set_oppose_acid(Ind, p_ptr->oppose_acid - 1);
	}

	/* Oppose Lightning */
	if (p_ptr->oppose_elec)
	{
		(void)set_oppose_elec(Ind, p_ptr->oppose_elec - 1);
	}

	/* Oppose Fire */
	if (p_ptr->oppose_fire)
	{
		(void)set_oppose_fire(Ind, p_ptr->oppose_fire - 1);
	}

	/* Oppose Cold */
	if (p_ptr->oppose_cold)
	{
		(void)set_oppose_cold(Ind, p_ptr->oppose_cold - 1);
	}

	/* Oppose Poison */
	if (p_ptr->oppose_pois)
	{
		(void)set_oppose_pois(Ind, p_ptr->oppose_pois - 1);
	}

	/*** Poison and Stun and Cut ***/

	/* Poison */
	if (p_ptr->poisoned)
	{
		int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus);
		if (get_skill(p_ptr, SKILL_HCURING) >= 30) adjust *= 2;

		/* Apply some healing */
		//(void)set_poisoned(Ind, p_ptr->poisoned - adjust - minus_health * 2, p_ptr->poisoned_attacker);
		(void)set_poisoned(Ind, p_ptr->poisoned - (adjust + minus_health) * (minus_health + 1), p_ptr->poisoned_attacker);
	}

	/* Stun */
	if (p_ptr->stun)
	{
#if 0
		int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus);
		/* Apply some healing */
		//(void)set_stun(Ind, p_ptr->stun - adjust - minus_health * 2);
		(void)set_stun(Ind, p_ptr->stun - (adjust + minus_health) * (minus_health + 1));
#endif
#if 0
		int adjust = minus + minus_combat;
		if (get_skill(p_ptr, SKILL_HCURING) >= 40) adjust++; /* '*= 2' would be a bit too fast w/ max combat I think */
#endif
#if 0
		int adjust = minus + minus_combat / 2 + ((minus_combat == 1 || minus_combat == 3) && magik(50) ? 1 : 0);
		if (get_skill(p_ptr, SKILL_HCURING) >= 40) adjust++;
#endif
		int adjust = minus + get_skill_scale_fine(p_ptr, SKILL_COMBAT, 2);
//		if (get_skill(p_ptr, SKILL_HCURING) >= 40) adjust = (adjust * 5) / 3;
		if (get_skill(p_ptr, SKILL_HCURING) >= 40) adjust++;

		(void)set_stun(Ind, p_ptr->stun - adjust);
	}

	/* Cut */
	if (p_ptr->cut)
	{
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

	/* Still possible effects from another player's support spell on this player? */
	if (p_ptr->support_timer)
	{
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
			if ((o_ptr->timeout < 100) || (!(o_ptr->timeout % 100)))
			{
				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP);
			}

			/* Hack -- notice interesting fuel steps */
			if ((o_ptr->timeout > 0) && (o_ptr->timeout < 100) && !(o_ptr->timeout % 10))
			{
				if (p_ptr->disturb_minor) disturb(Ind, 0, 0);
				msg_print(Ind, "Your ring flickers and fades, flashes of light run over its surface.");
				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP);
			}
			else if (o_ptr->timeout == 0)
			{
				disturb(Ind, 0, 0);
				msg_print(Ind, "Your ring disintegrates!");

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

	/*** Process Light ***/

	/* Check for light being wielded */
	o_ptr = &p_ptr->inventory[INVEN_LITE];

	/* Burn some fuel in the current lite */
	if (o_ptr->tval == TV_LITE)
	{
		u32b f1 = 0 , f2 = 0 , f3 = 0, f4 = 0, f5 = 0, esp = 0;
		/* Hack -- Use some fuel (sometimes) */
#if 0
		if (!artifact_p(o_ptr) && !(o_ptr->sval == SV_LITE_DWARVEN)
				&& !(o_ptr->sval == SV_LITE_FEANOR) && (o_ptr->pval > 0) && (!o_ptr->name3))
#endif	// 0

			/* Extract the item flags */
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

		/* Hack -- Use some fuel */
		if ((f4 & TR4_FUEL_LITE) && (o_ptr->timeout > 0))
		{
			/* Decrease life-span */
			o_ptr->timeout--;

			/* Hack -- notice interesting fuel steps */
			if ((o_ptr->timeout < 100) || (!(o_ptr->timeout % 100)))
			{
				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP);
			}

			/* Hack -- Special treatment when blind */
			if (p_ptr->blind)
			{
				/* Hack -- save some light for later */
				if (o_ptr->timeout == 0) o_ptr->timeout++;
			}

			/* The light is now out */
			else if (o_ptr->timeout == 0)
			{
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

				if (!refilled)
				{
					msg_print(Ind, "Your light has gone out!");
					/* Calculate torch radius */
					p_ptr->update |= (PU_TORCH|PU_BONUS);
				}
#if 0	/* torch of presentiment goes poof to unlight trap, taken out for now - C. Blue */
				/* Torch disappears */
				if (o_ptr->sval == SV_LITE_TORCH && !o_ptr->timeout)
				{
					/* Decrease the item, optimize. */
					inven_item_increase(Ind, INVEN_LITE, -1);
					//						inven_item_describe(Ind, INVEN_LITE);
					inven_item_optimize(Ind, INVEN_LITE);
				}
#endif
			}

			/* The light is getting dim */
			else if ((o_ptr->timeout < 100) && (!(o_ptr->timeout % 10)))
			{
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
	if (p_ptr->drain_exp && magik(p_ptr->wpos.wz != 0 ? 50 : (istown(&p_ptr->wpos) ? 0 : 25)) && magik(30 - (60 / (p_ptr->drain_exp + 2))))
//		take_xp_hit(Ind, 1 + p_ptr->lev / 5 + p_ptr->max_exp / 50000L, "Draining", TRUE, FALSE);
		/* Moltor is right, exp drain was too weak for up to quite high levels. Need to make a new formula.. */
	{
		long exploss = 1 + p_ptr->lev + p_ptr->max_exp / 2000L;
#ifdef ALT_EXPRATIO
		if ((p_ptr->lev >= 2) && (player_exp[p_ptr->lev - 2] > p_ptr->exp - exploss))
			exploss = p_ptr->exp - player_exp[p_ptr->lev - 2];
#else
		if ((p_ptr->lev >= 2) && ((((s64b)player_exp[p_ptr->lev - 2]) * ((s64b)p_ptr->exp_fact)) / 100L > p_ptr->exp - exploss))
			exploss = p_ptr->exp - player_exp[p_ptr->lev - 2];
#endif
		if (exploss > 0) take_xp_hit(Ind, exploss, "Draining", TRUE, FALSE);
	}

#if 0
	{
		if ((rand_int(100) < 10) && (p_ptr->exp > 0))
		{
			p_ptr->exp--;
			p_ptr->max_exp--;
			check_experience(Ind);
		}
	}
#endif	// 0

	/* Now implemented here too ;) - C. Blue */
	/* let's say TY_CURSE lowers stats (occurs often) */
	if (p_ptr->ty_curse && (rand_int(30) == 0) && (get_skill(p_ptr, SKILL_HSUPPORT) < 50)) {
		msg_print(Ind, "An ancient foul curse shakes your body!");
#if 0
		if (rand_int(2))
		(void)do_dec_stat(Ind, rand_int(6), STAT_DEC_NORMAL);
		else
		lose_exp(Ind, (p_ptr->exp / 100) * MON_DRAIN_LIFE);
		/* take_xp_hit(Ind, 1 + p_ptr->lev / 5 + p_ptr->max_exp / 50000L, "an ancient foul curse", TRUE, TRUE); */
#else
		(void)do_dec_stat(Ind, rand_int(6), STAT_DEC_NORMAL);
#endif
	}
	/* and DG_CURSE randomly summons a monster (non-unique) */
	if (p_ptr->dg_curse && (rand_int(60) == 0) && (get_skill(p_ptr, SKILL_HSUPPORT) < 50) && !istown(&p_ptr->wpos)) {
		msg_print(Ind, "An ancient morgothian curse calls out!");
		summon_specific(&p_ptr->wpos, p_ptr->py, p_ptr->px, p_ptr->lev * 2, 100, 0, 0, 0);
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
				"Black Breath", TRUE, TRUE);
#if 0
		byte chance = (p_ptr->prace == RACE_HOBBIT) ? 2 : 5;
		int plev = p_ptr->lev;

		//                if (PRACE_FLAG(PR1_RESIST_BLACK_BREATH)) chance = 2;

		//                if ((rand_int(100) < chance) && (p_ptr->exp > 0))
		/* Toned down a little, considering it's realtime. */
		if ((rand_int(200) < chance))
			take_xp_hit(Ind, 1 + plev / 5 + p_ptr->max_exp / 10000L,
					"Black Breath", TRUE, TRUE);
		{
			int reduce = 1 + plev / 5 + p_ptr->max_exp / 10000L;
			p_ptr->exp -= reduce;
			p_ptr->max_exp -= reduce;
			//                        (void)do_dec_stat(Ind, randint(6)+1, STAT_DEC_NORMAL);
			(void)do_dec_stat(Ind, rand_int(6), STAT_DEC_NORMAL);
			check_experience(Ind);

			/* Now you can die from this :) */
			if (p_ptr->exp < 1 && !p_ptr->ghost)
			{
				p_ptr->chp=-3;
				strcpy(p_ptr->died_from, "Black Breath");
				strcpy(p_ptr->really_died_from, "Black Breath");
				p_ptr->deathblow = 0;
				player_death(Ind);
			}
		}
#endif	// 0
	}

	/* Drain Mana */
	if (p_ptr->drain_mana && p_ptr->csp)
	{
		p_ptr->csp -= p_ptr->drain_mana;
		if (magik(30)) p_ptr->csp -= p_ptr->drain_mana;

		if (p_ptr->csp < 0) p_ptr->csp = 0;

		/* Redraw */
		p_ptr->redraw |= (PR_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);
	}

	/* Drain Hitpoints */
	if (p_ptr->drain_life)
	{
		int drain = p_ptr->drain_life * (rand_int(p_ptr->mhp / 100) + 1);
		take_hit(Ind, drain < p_ptr->chp ? drain : p_ptr->chp, "life draining", 0);
#if 0
		p_ptr->chp -= (drain < p_ptr->chp ? drain : p_ptr->chp);

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);
#endif	// 0
	}

	/* Note changes */
	j = 0;

	/* Process equipment */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
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
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &p_ptr->inventory[i];

		/* Examine all charging rods */
		if ((o_ptr->tval == TV_ROD) && (o_ptr->pval))
		{
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
	if (j)
	{
		/* Combine pack */
		p_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);
	}

	/* Feel the inventory */
	sense_inventory(Ind);

	/*** Involuntary Movement ***/


	/* Delayed mind link */
	if (p_ptr->esp_link_end)
	{
		p_ptr->esp_link_end--;

		/* Activate the break */
		if (!p_ptr->esp_link_end)
		{
			int Ind2;

			if (p_ptr->esp_link_type && p_ptr->esp_link)
			{
				Ind2 = find_player(p_ptr->esp_link);

				if (!Ind2)
					end_mind(Ind, TRUE);
				else
				{
					player_type *p_ptr2 = Players[Ind2];

					if (!(p_ptr->esp_link_flags & LINKF_HIDDEN)) {
						msg_format(Ind, "\377RYou break the mind link with %s.", p_ptr2->name);
						msg_format(Ind2, "\377R%s breaks the mind link with you.", p_ptr->name);
					}
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
	}

	/* Don't do AFK in a store */
	if (p_ptr->tim_store)
	{
		if (p_ptr->store_num < 0) p_ptr->tim_store = 0;
		else
		{
			/* Count down towards turnout */
			p_ptr->tim_store--;

			/* Check if that town is 'crowded' */
			if (p_ptr->tim_store <= 0)
			{
				player_type *q_ptr;
				bool bye = FALSE;

				for (j = 1; j < NumPlayers + 1; j++)
				{
					q_ptr = Players[j];
					if (Ind == j) continue;
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

	if (p_ptr->tim_blacklist && !p_ptr->afk)
	{
		/* Count down towards turnout */
		p_ptr->tim_blacklist--;
	}

#if 1 /* use turns instead of day/night cycles */
	if (p_ptr->tim_watchlist)
	{
		/* Count down towards turnout */
		p_ptr->tim_watchlist--;
	}
#endif

	if (p_ptr->pstealing)
	{
		/* Count down towards turnout */
		p_ptr->pstealing--;
//		if (!p_ptr->pstealing) msg_print(Ind, "You're calm enough to steal from another player.");
		if (!p_ptr->pstealing) msg_print(Ind, "You're calm enough to attempt to steal something.");
	}

	/* Delayed Word-of-Recall */
	if (p_ptr->word_recall)
	{
		/* Count down towards recall */
		p_ptr->word_recall--;

		/* MEGA HACK: no recall if icky, or in a shop */
		if( ! p_ptr->word_recall ) 
		{
			//				if(p_ptr->anti_tele ||
			if((p_ptr->store_num > 0) || p_ptr->anti_tele ||
					(check_st_anchor(&p_ptr->wpos, p_ptr->py, p_ptr->px) &&
					 !p_ptr->admin_dm) ||
					zcave[p_ptr->py][p_ptr->px].info&CAVE_STCK)
			{
				p_ptr->word_recall++;
			}
		}

		/* XXX Rework needed! */
		/* Activate the recall */
		if (!p_ptr->word_recall)
		{
			do_recall(Ind, 0);
		}
	}

	bypass_invuln = FALSE;

	/* Evileye, please tell me if it's right */
	if(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) p_ptr->tim_wraith=0;

	/* No wraithform on NO_MAGIC levels - C. Blue */
	if (p_ptr->wpos.wz && l_ptr && (l_ptr->flags1 & LF1_NO_MAGIC)) p_ptr->tim_wraith=0;

	return (TRUE);
}

/* process any team games */
static void process_games(int Ind){
	player_type *p_ptr=Players[Ind];
	cave_type **zcave;
	cave_type *c_ptr;
	char sstr[80];
	int score=0;
	if(!(zcave=getcave(&p_ptr->wpos))) return;
	c_ptr=&zcave[p_ptr->py][p_ptr->px];

	if(c_ptr->feat==FEAT_AGOAL || c_ptr->feat==FEAT_BGOAL){
		int ball;
		switch(gametype){
			/* rugby type game */
			case EEGAME_RUGBY:
				if((ball=has_ball(p_ptr))==-1) break;

				if(p_ptr->team==1 && c_ptr->feat==FEAT_BGOAL){
					teamscore[0]++;
					msg_format_near(Ind, "\377R%s scored a goal!!!", p_ptr->name);
					msg_format(Ind, "\377rYou scored a goal!!!");
					score=1;
				}
				if(p_ptr->team==2 && c_ptr->feat==FEAT_AGOAL){
					teamscore[1]++;
					msg_format_near(Ind, "\377B%s scored a goal!!!", p_ptr->name);
					msg_format(Ind, "\377gYou scored a goal!!!");
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
	cave_type *c_ptr;
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];
#if 1 /* NEW_RUNNING_FEAT */
	if (!is_admin(p_ptr) && !p_ptr->ghost && !p_ptr->tim_wraith) {
		/* are we in fact running-flying? */
		if ((f_info[c_ptr->feat].flags1 & (FF1_CAN_FLY | FF1_CAN_RUN)) && p_ptr->fly) {
			if (f_info[c_ptr->feat].flags1 & FF1_SLOW_FLYING_1) real_speed /= 2;
			if (f_info[c_ptr->feat].flags1 & FF1_SLOW_FLYING_2) real_speed /= 4;
		}
    	    /* or running-swimming? */
		else if ((c_ptr->feat == 84 || c_ptr->feat == 103 || c_ptr->feat == 174 || c_ptr->feat == 187) && p_ptr->can_swim) {
		        if (f_info[c_ptr->feat].flags1 & FF1_SLOW_SWIMMING_1) real_speed /= 2;
	    	    if (f_info[c_ptr->feat].flags1 & FF1_SLOW_SWIMMING_2) real_speed /= 4;
	        }
		/* or just normally running? */
		else {
			if (f_info[c_ptr->feat].flags1 & FF1_SLOW_RUNNING_1) real_speed /= 2;
			if (f_info[c_ptr->feat].flags1 & FF1_SLOW_RUNNING_2) real_speed /= 4;
		}
	}
#endif

	if (Players[Ind]->conn == NOT_CONNECTED)
		return;

	/* Try to execute any commands on the command queue. */
	process_pending_commands(p_ptr->conn);

	/* Mind Fusion/Control disable the char */
	if (p_ptr->esp_link && p_ptr->esp_link_type && (p_ptr->esp_link_flags & LINKF_OBJ)) return;

	/* Check for auto-retaliate */
#if 0
	if ((p_ptr->energy >= level_speed(&p_ptr->wpos)) && !p_ptr->confused &&
			(!p_ptr->autooff_retaliator ||
			 !(p_ptr->invuln || p_ptr->tim_manashield)))
#else
	if ((p_ptr->energy >= level_speed(&p_ptr->wpos)) && !p_ptr->confused &&
			(!p_ptr->autooff_retaliator ||
			 !p_ptr->invuln))
#endif
	{
		if (p_ptr->shooting_till_kill && !p_ptr->shoot_till_kill_book && !p_ptr->shoot_till_kill_spell) { /* did we shoot till kill last turn, and used bow-type item? */
	                do_cmd_fire(Ind, 5);
    	        	if (p_ptr->ranged_double) do_cmd_fire(Ind, 5);
			p_ptr->shooty_till_kill = FALSE; /* if we didn't succeed shooting till kill, then we don't intend it anymore */
		}

		/* Check for nearby monsters and try to kill them */
		/* If auto_retaliate returns nonzero than we attacked
		 * something and so should use energy.
		 */
		if ((attackstatus = auto_retaliate(Ind)))
		{
			p_ptr->auto_retaliating = TRUE;
			/* Use energy */
//			p_ptr->energy -= level_speed(p_ptr->dun_depth);
		} else {
			p_ptr->auto_retaliating = FALSE;
		}
	} else {
		p_ptr->auto_retaliating = FALSE; /* if no energy left, this is required to turn off the no-run-while-retaliate-hack */
	}

	/* ('Handle running' from above was originally at this place) */
	/* Handle running -- 5 times the speed of walking */
	while (p_ptr->running && p_ptr->energy >= (level_speed(&p_ptr->wpos)*(real_speed + 1))/real_speed)
	{
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
	if (!(turn%(level_speed(&p_ptr->wpos)/12)))
	{
		if (!process_player_end_aux(Ind)) return;
	}


	/* HACK -- redraw stuff a lot, this should reduce perceived latency. */
	/* This might not do anything, I may have been silly when I added this. -APD */
	/* Notice stuff (if needed) */
	if (p_ptr->notice) notice_stuff(Ind);

	/* Update stuff (if needed) */
	if (p_ptr->update) update_stuff(Ind);

//	if(zcave[p_ptr->py][p_ptr->px].info&CAVE_STCK) p_ptr->tim_wraith=0;
	
	/* Redraw stuff (if needed) */
	if (p_ptr->redraw) redraw_stuff(Ind);

	/* Redraw stuff (if needed) */
	if (p_ptr->window) window_stuff(Ind);
}


static bool stale_level(struct worldpos *wpos, int grace)
{
	time_t now;

	/* Hack -- towns are static for good. */
//	if (istown(wpos)) return (FALSE);

	now=time(&now);
	if(wpos->wz)
	{
		struct dungeon_type *d_ptr;
		struct dun_level *l_ptr;
		d_ptr=getdungeon(wpos);
		if(!d_ptr) return(FALSE);
		l_ptr=&d_ptr->level[ABS(wpos->wz)-1];
#if DEBUG_LEVEL > 1
		s_printf("%s  now:%d last:%d diff:%d grace:%d players:%d\n", wpos_format(0, wpos), now, l_ptr->lastused, now-l_ptr->lastused,grace, players_on_depth(wpos));
#endif
		if(now-l_ptr->lastused > grace){
			return(TRUE);
		}
	}
	else if(now-wild_info[wpos->wy][wpos->wx].lastused > grace){
		/* Never allow dealloc where there are houses */
		/* For now at least */
#if 0
		int i;

		for(i=0;i<num_houses;i++){
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
	if (!wpos->wx && !wpos->wy && !wpos->wz && sector00separation) return;

	/* Arena Monster Challenge */
	if (ge_training_tower &&
	    wpos->wx == cfg.town_x && wpos->wy == cfg.town_y &&
	    wpos->wz == wild_info[cfg.town_y][cfg.town_y].tower->maxdepth) return;


	// Count the number of players actually in game on this depth
	for (j = 1; j < NumPlayers + 1; j++)
	{
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
		if (( getlevel(wpos) < cfg.min_unstatic_level) && (0 < cfg.min_unstatic_level))
		{
			new_players_on_depth(wpos,0,FALSE);
		}
		// random chance of the level unstaticing
		// the chance is one in (base_chance * depth)/250 feet.
		if (!rand_int(((cfg.level_unstatic_chance * (getlevel(wpos)+5))/5)-1))
		{
			// unstatic the level
			new_players_on_depth(wpos,0,FALSE);
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
	for(i=0;i<num_houses;i++)
	{
		if(!houses[i].dna->owner) continue;
		switch(houses[i].dna->owner_type)
		{
			case OT_PLAYER:
				if(!lookup_player_name(houses[i].dna->owner))
				{
					s_printf("Found old player houses. ID: %d\n", houses[i].dna->owner);
					kill_houses(houses[i].dna->owner, OT_PLAYER);
				}
				break;
			case OT_PARTY:
				if(!strlen(parties[houses[i].dna->owner].name))
				{
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
		twpos.wz=0;
		for(y=0;y<MAX_WILD_Y;y++)
		{
			twpos.wy=y;
			for(x=0;x<MAX_WILD_X;x++)
			{
				struct wilderness_type *w_ptr;
				struct dungeon_type *d_ptr;

				twpos.wx=x;
				w_ptr=&wild_info[twpos.wy][twpos.wx];

				if (cfg.level_unstatic_chance > 0 &&
					players_on_depth(&twpos))
					do_unstat(&twpos);

				if(!players_on_depth(&twpos) && !istown(&twpos) &&
						getcave(&twpos) && stale_level(&twpos, cfg.anti_scum))
					dealloc_dungeon_level(&twpos);

				if(w_ptr->flags & WILD_F_UP)
				{
					d_ptr=w_ptr->tower;
					for(i=1;i<=d_ptr->maxdepth;i++)
					{
						twpos.wz=i;
						if (cfg.level_unstatic_chance > 0 &&
							players_on_depth(&twpos))
						do_unstat(&twpos);
						
						if(!players_on_depth(&twpos) && getcave(&twpos) &&
								stale_level(&twpos, cfg.anti_scum))
							dealloc_dungeon_level(&twpos);
					}
				}
				if(w_ptr->flags & WILD_F_DOWN)
				{
					d_ptr=w_ptr->dungeon;
					for(i=1;i<=d_ptr->maxdepth;i++)
					{
						twpos.wz=-i;
						if (cfg.level_unstatic_chance > 0 &&
							players_on_depth(&twpos))
							do_unstat(&twpos);

						if(!players_on_depth(&twpos) && getcave(&twpos) &&
								stale_level(&twpos, cfg.anti_scum))
							dealloc_dungeon_level(&twpos);
					}
				}
				twpos.wz=0;
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
	for(j=0;j<num_houses;j++){
		if(inarea(&houses[j].wpos, &o_ptr->wpos)){
			if(fill_house(&houses[j], FILL_OBJECT, o_ptr)){
				if(houses[j].dna->owner_type==OT_PLAYER){
					if(o_ptr->owner != houses[j].dna->owner){
						if(o_ptr->level > lookup_player_level(houses[j].dna->owner))
							s_printf("Suspicious item: (%d,%d) Owned by %s, in %s's house. (%d,%d)\n", o_ptr->wpos.wx, o_ptr->wpos.wy, lookup_player_name(o_ptr->owner), lookup_player_name(houses[j].dna->owner), o_ptr->level, lookup_player_level(houses[j].dna->owner));
					}
				}
				else if(houses[j].dna->owner_type==OT_PARTY){
					int owner;
					if((owner=lookup_player_id(parties[houses[j].dna->owner].owner))){
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
	for(j=0;j<num_houses;j++)
	{
		h_ptr = &houses[j];
		if(h_ptr->dna->owner_type==OT_PLAYER)
		{
			for (i = 0; i < h_ptr->stock_num; i++)
			{
				o_ptr = &h_ptr->stock[i];
				if(o_ptr->owner != h_ptr->dna->owner)
				{
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
		else if(h_ptr->dna->owner_type==OT_PARTY)
		{
			int owner;
			if((owner=lookup_player_id(parties[h_ptr->dna->owner].owner)))
			{
				for (i = 0; i < h_ptr->stock_num; i++)
				{
					o_ptr = &h_ptr->stock[i];
					if(o_ptr->owner != owner)
					{
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
	for(j=0;j<num_houses;j++){
		if(inarea(&houses[j].wpos, &o_ptr->wpos)){
			if(fill_house(&houses[j], FILL_OBJECT, o_ptr)){
				if(houses[j].dna->owner_type==OT_PLAYER){
					o_ptr->owner_mode = lookup_player_mode(houses[j].dna->owner));
				}
				else if(houses[j].dna->owner_type==OT_PARTY){
					int owner;
					if((owner=lookup_player_id(parties[houses[j].dna->owner].owner))){
						o_ptr->owner_mode = lookup_player_mode(owner));
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
	for(j=0;j<num_houses;j++)
	{
		h_ptr = &houses[j];
		if(h_ptr->dna->owner_type==OT_PLAYER)
		{
			for (i = 0; i < h_ptr->stock_num; i++)
			{
				o_ptr = &h_ptr->stock[i];
				o_ptr->owner_mode = lookup_player_mode(houses[j].dna->owner));
			}
		}
		else if(h_ptr->dna->owner_type==OT_PARTY)
		{
			int owner;
			if((owner=lookup_player_id(parties[h_ptr->dna->owner].owner)))
			{
				for (i = 0; i < h_ptr->stock_num; i++)
				{
					o_ptr = &h_ptr->stock[i];
					o_ptr->owner_mode = lookup_player_mode(owner));
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
static void scan_objs(){
	int i, cnt=0, dcnt=0;
	object_type *o_ptr;
	cave_type **zcave;
	if (!cfg.surface_item_removal && !cfg.dungeon_item_removal) return;
	for(i=0; i<o_max; i++){
		o_ptr=&o_list[i];
		if(o_ptr->k_idx){
			bool sj=FALSE;
			/* not dropped on player death or generated on the floor? (or special stuff) */
			if (o_ptr->marked2 == ITEM_REMOVAL_NEVER) continue;

			/* check items on the world's surface */
			if(!o_ptr->wpos.wz && (zcave=getcave(&o_ptr->wpos)) && cfg.surface_item_removal) {
				/* XXX noisy warning, eh? */
				/* This is unsatisfactory. */
				if (!in_bounds_array(o_ptr->iy, o_ptr->ix) &&
				    /* There was an old woman who swallowed a fly... */
				    in_bounds_array(255 - o_ptr->iy, o_ptr->ix)) {
					sj=TRUE;
					o_ptr->iy = 255 - o_ptr->iy;
				}
				if (in_bounds_array(o_ptr->iy, o_ptr->ix)) // monster trap?
				{
					/* ick suggests a store, so leave) */
					if(!(zcave[o_ptr->iy][o_ptr->ix].info & CAVE_ICKY)){
						/* Artifacts and objects that were inscribed and dropped by
						the dungeon master or by unique monsters on their death
						stay n times as long as cfg.surface_item_removal specifies */
#if 1
						if(++o_ptr->marked>=((artifact_p(o_ptr) ||
						    (o_ptr->note && !o_ptr->owner))?
						    cfg.surface_item_removal * 3 : cfg.surface_item_removal))
#else
						if(++o_ptr->marked>=((artifact_p(o_ptr) ||
						    o_ptr->note)?
						    cfg.surface_item_removal * 3 : cfg.surface_item_removal))
#endif
						{
							delete_object_idx(zcave[o_ptr->iy][o_ptr->ix].o_idx, TRUE);
							dcnt++;
						}
					}
				}
	/* Once hourly cheeze check. (logs would fill the hd otherwise ;( */
#if CHEEZELOG_LEVEL > 1
				else
#if CHEEZELOG_LEVEL < 4
					if (!(turn % (cfg.fps * 3600)))
#endif	/* CHEEZELOG_LEVEL (4) */
						cheeze(o_ptr);
#endif	/* CHEEZELOG_LEVEL (1) */
				cnt++;
			}
			/* check items on dungeon/tower floors */
			else if(o_ptr->wpos.wz && (zcave=getcave(&o_ptr->wpos)) && cfg.dungeon_item_removal) {
				if (!in_bounds_array(o_ptr->iy, o_ptr->ix) &&
					in_bounds_array(255 - o_ptr->iy, o_ptr->ix)){
					sj=TRUE;
					o_ptr->iy = 255 - o_ptr->iy;
				}
				if (in_bounds_array(o_ptr->iy, o_ptr->ix)) // monster trap?
				{
					/* ick suggests a store, so leave) */
					if(!(zcave[o_ptr->iy][o_ptr->ix].info & CAVE_ICKY)){
						/* Artifacts and objects that were inscribed and dropped by
						the dungeon master or by unique monsters on their death
						stay n times as long as cfg.surface_item_removal specifies */
#if 1
						if(++o_ptr->marked>=((artifact_p(o_ptr) ||
						    (o_ptr->note && !o_ptr->owner))?
						    cfg.dungeon_item_removal * 3 : cfg.dungeon_item_removal))
#else
						if(++o_ptr->marked>=((artifact_p(o_ptr) ||
						    o_ptr->note)?
						    cfg.dungeon_item_removal * 3 : cfg.dungeon_item_removal))
#endif
						{
							delete_object_idx(zcave[o_ptr->iy][o_ptr->ix].o_idx, TRUE);
							dcnt++;
						}
					}
				}
	/* Once hourly cheeze check. (logs would fill the hd otherwise ;( */
#if CHEEZELOG_LEVEL > 1
				else
#if CHEEZELOG_LEVEL < 4
					if (!(turn % (cfg.fps * 3600)))
#endif	/* CHEEZELOG_LEVEL (4) */
						cheeze(o_ptr);
#endif	/* CHEEZELOG_LEVEL (1) */
				cnt++;
			}
			if(sj) o_ptr->iy = 255 - o_ptr->iy;
		}
	}
	if(dcnt) s_printf("Scanned %d objects. Removed %d.\n", cnt, dcnt);

#ifndef USE_MANG_HOUSE_ONLY
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

	for(i=0;i<numtowns;i++)
	{
		/* Maintain each shop (except home and auction house) */
//		for (n = 0; n < MAX_STORES - 2; n++)
		for (n = 0; n < max_st_idx; n++)
		{
			/* Maintain */
			store_maint(&town[i].townstore[n]);
		}

		/* Sometimes, shuffle the shopkeepers */
		if (rand_int(STORE_SHUFFLE) == 0)
		{
			/* Shuffle a random shop (except home and auction house) */
//			store_shuffle(&town[i].townstore[rand_int(MAX_STORES - 2)]);
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
   should also multiply the cfg.fps by a multiple of 10, otherwise
   depending on cfg.fps it might be skipped sometimes, which may or
   may not be critical depending on what it does! - C. Blue */
static void process_various(void)
{
	int i, j;
	int h = 0, m = 0, s = 0;
	time_t now;
	struct tm *tmp;
	//cave_type *c_ptr;
	player_type *p_ptr;

	//char buf[1024];

	/* this TomeCron stuff could be merged at some point
	   to improve efficiency. ;) */

	if(!(turn % 2)){
		do_xfers();	/* handle filetransfers per second
				 * FOR NOW!!! DO NOT TOUCH/CHANGE!!!
				 */
	}

	/* Save the server state occasionally */
	if (!(turn % ((NumPlayers ? 10L : 1000L) * SERVER_SAVE)))
	{
		save_server_info();

		/* Save each player */
		for (i = 1; i <= NumPlayers; i++)
		{
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
	if (!(turn % (cfg.fps * 86400)))
	{
		s_printf("24 hours maintenance cycle\n");
		scan_players();
		scan_houses();
		if (cfg.auto_purge)
		{
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
		exec_lua(0, format("cron_24h(\"%s\", %d, %d, %d)", showtime(), h, m, s));
		
/*		bbs_add_line("--- new day line ---"); */
	}

	if (!(turn % (cfg.fps * 10))){
		purge_old();
	}

#if 0 /* disable for now - mikaelh */
	if (!(turn % (cfg.fps * 50))) {
		/* Tell the scripts that we're alive */
		update_check_file();
	}
#endif

	/* Handle certain things once a minute */
	if (!(turn % (cfg.fps * 60)))
	{
		monster_race *r_ptr;

		check_quests();		/* It's enough with 'once a minute', none? -Jir */

		Check_evilmeta();	/* check that evilmeta is still up */
		check_banlist();	/* unban some players */
		scan_objs();		/* scan objects and houses */

		if (dungeon_store_timer) dungeon_store_timer--; /* Timeout */
		if (dungeon_store2_timer) dungeon_store2_timer--; /* Timeout */
#ifdef HALLOWEEN
		if (great_pumpkin_timer > 0) great_pumpkin_timer--;
#endif

		/* Update the player retirement timers */
		for (i = 1; i <= NumPlayers; i++)
		{
			p_ptr = Players[i];

			// If our retirement timer is set
			if (p_ptr->retire_timer > 0)
			{
				int k = p_ptr->retire_timer;

				// Decrement our retire timer
				j = k - 1;

				// Alert him
				if (j <= 60)
				{
					msg_format(i, "\377rYou have %d minute%s of tenure left.", j, j > 1 ? "s" : "");
				}
				else if (j <= 1440 && !(k % 60))
				{
					msg_format(i, "\377yYou have %d hours of tenure left.", j / 60);
				}
				else if (!(k % 1440))
				{
					msg_format(i, "\377GYou have %d days of tenure left.", j / 1440);
				}


				// If the timer runs out, forcibly retire
				// this character.
				if (!j)
				{
					do_cmd_suicide(i);
				}
			}

		}

		/* Reswpan for kings' joy  -Jir- */
		/* Update the unique respawn timers */
		/* I moved this out of the loop above so this may need some
                 * tuning now - mikaelh */
		for (j = 1; j <= NumPlayers; j++)
		{
			p_ptr = Players[j];
			if (!p_ptr->total_winner) continue;

			/* Hack -- never Maggot and his dogs :) */
			i = rand_range(60,MAX_R_IDX-2);
			r_ptr = &r_info[i];

			/* Make sure we are looking at a dead unique */
			if (!(r_ptr->flags1 & RF1_UNIQUE))
			{
				j--;
				continue;
			}

			if (!p_ptr->r_killed[i]) continue;

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
			// 				r_ptr->max_num = 1;
			//				r_ptr->respawn_timer = -1;

			/* Tell the player */
			/* the_sandman: added colour */
			msg_format(j,"\377v%s rises from the dead!",(r_name + r_ptr->name));
			//				msg_broadcast(0,buf); 
		}

#if 0 /* No more reswpan */
		/* Update the unique respawn timers */
		for (i = 1; i < MAX_R_IDX-1; i++)
		{
			monster_race *r_ptr = &r_info[i];

			/* Make sure we are looking at a dead unique */
			if (!(r_ptr->flags1 & RF1_UNIQUE)) continue;
			if (r_ptr->max_num > 0) continue;

			/* Hack -- Initially set a newly killed uniques respawn timer */
			/* -1 denotes that the respawn timer is unset */
			if (r_ptr->respawn_timer < 0) 
			{
				r_ptr->respawn_timer = cfg_unique_respawn_time * (r_ptr->level + 1);
				if (r_ptr->respawn_timer > cfg_unique_max_respawn_time)
					r_ptr->respawn_timer = cfg_unique_max_respawn_time;
			}
			// Decrament the counter 
			else r_ptr->respawn_timer--; 
			// Once the timer hits 0, ressurect the unique.
			if (!r_ptr->respawn_timer)
			{
				/* "Ressurect" the unique */
				r_ptr->max_num = 1;
				r_ptr->respawn_timer = -1;

				/* Tell every player */
				/* the_sandman: added colour */
				snprintf(buf, sizeof(buf), "\377v%s rises from the dead!",(r_name + r_ptr->name));
				msg_broadcast(0,buf); 
			}
		}
#endif
	}

#if 0
	/* Grow trees very occasionally */
	if (!(turn % (10L * GROW_TREE)))
	{
		/* Find a suitable location */
		for (i = 1; i < 1000; i++)
		{
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
			c_ptr->feat = (magik(80)?FEAT_TREE:FEAT_BUSH);

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

	for (i = 1; i < NumPlayers + 1; i++)
	{
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

	for (i = 1; i < NumPlayers + 1; i++)
	{
		player_type *p_ptr = Players[i];
		
		if (!strcmp(p_ptr->name, name)) return i;
	}
	
	/* assume none */
	return 0;
}		

static void process_player_change_wpos(int Ind)
{
	player_type *p_ptr = Players[Ind];
	worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	//worldpos twpos;
	dun_level *l_ptr;
	int d, j, x, y, startx = 0, starty = 0, m_idx, my, mx;
	byte *w_ptr;
	cave_type *c_ptr;


	/* Check "maximum depth" to make sure it's still correct */
	if (wpos->wz != 0)
	if ((!p_ptr->ghost) && (getlevel(wpos) > p_ptr->max_dlv))
		p_ptr->max_dlv = getlevel(wpos);

	/* Make sure the server doesn't think the player is in a store */
//	p_ptr->store_num = -1;
	if (p_ptr->store_num > -1)
	{
		p_ptr->store_num = -1;
		Send_store_kick(Ind);
	}

	/* Hack -- artifacts leave the queen/king */
	/* also checks the artifact list */
	//		if (!is_admin(p_ptr) && player_is_king(Ind))
	{
		object_type *o_ptr;
		char		o_name[160];

		for (j = 0; j < INVEN_TOTAL; j++)
		{
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

			if (o_ptr->name1 == ART_MORGOTH || o_ptr->name1 == ART_GROND || o_ptr->name1 == ART_PHASING ||
			    k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY)
				continue;

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

	/* Somebody has entered an ungenerated level */
	if (players_on_depth(wpos) && !getcave(wpos))
	{
		/* Allocate space for it */
		alloc_dungeon_level(wpos);

		/* Generate a dungeon level there */
		generate_cave(wpos, p_ptr);
	}

	zcave = getcave(wpos);
	l_ptr = getfloor(wpos);

	/* Clear the "marked" and "lit" flags for each cave grid */
	for (y = 0; y < MAX_HGT; y++)
	{
		for (x = 0; x < MAX_WID; x++)
		{
			w_ptr = &p_ptr->cave_flag[y][x];

			*w_ptr = 0;
		}
	}

	/* hack -- update night/day in wilderness levels */
	if ((!wpos->wz) && (IS_DAY)) wild_apply_day(wpos); 
	if ((!wpos->wz) && (IS_NIGHT)) wild_apply_night(wpos);

	/* Memorize the town and all wilderness levels close to town */
	if (istown(wpos) || (wpos->wz==0 && wild_info[wpos->wy][wpos->wx].radius <=2))
	{
		bool dawn = IS_DAY; 

		p_ptr->max_panel_rows = (MAX_HGT / SCREEN_HGT) * 2 - 2;
		p_ptr->max_panel_cols = (MAX_WID / SCREEN_WID) * 2 - 2;

		p_ptr->cur_hgt = MAX_HGT;
		p_ptr->cur_wid = MAX_WID;

		if(istown(wpos)){
			p_ptr->town_x = wpos->wx;
			p_ptr->town_y = wpos->wy;
		}

		/* Memorize the town for this player (if daytime) */
		for (y = 0; y < MAX_HGT; y++)
		{
			for (x = 0; x < MAX_WID; x++)
			{
				w_ptr = &p_ptr->cave_flag[y][x];
				c_ptr = &zcave[y][x];

				/* Memorize if daytime or "interesting" */
//				if (dawn || c_ptr->feat > FEAT_INVIS || c_ptr->info & CAVE_ROOM)
				if (dawn || !cave_plain_floor_grid(c_ptr) || c_ptr->info & CAVE_ROOM)
					*w_ptr |= CAVE_MARK;
			}
		}
	}
	else if (wpos->wz)
	{
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

		show_floor_feeling(Ind);
	}

	else
	{
		p_ptr->max_panel_rows = (MAX_HGT / SCREEN_HGT) * 2 - 2;
		p_ptr->max_panel_cols = (MAX_WID / SCREEN_WID) * 2 - 2;

		p_ptr->cur_hgt = MAX_HGT;
		p_ptr->cur_wid = MAX_WID;
	}

	/* Determine starting location */
	switch (p_ptr->new_level_method)
	{
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
							could be achieved by seeding the RNG with the depth.
							*/
		case LEVEL_OUTSIDE_RAND:

						/* make sure we aren't in an "icky" location */
						do
						{
							starty = rand_int((l_ptr ? l_ptr->hgt : MAX_HGT)-3)+1;
							startx = rand_int((l_ptr ? l_ptr->wid : MAX_WID)-3)+1;
						}
						while (  (zcave[starty][startx].info & CAVE_ICKY)
								|| (zcave[starty][startx].feat==FEAT_DEEP_WATER)
								|| (!cave_floor_bold(zcave, starty, startx)) );
						break;
	}

	/* Hack -- handle smaller floors */
	if (l_ptr && (starty > l_ptr->hgt || startx > l_ptr->wid))
	{
		/* make sure we aren't in an "icky" location */
		do
		{
			starty = rand_int(l_ptr->hgt-3)+1;
			startx = rand_int(l_ptr->wid-3)+1;
		}
		while (  (zcave[starty][startx].info & CAVE_ICKY)
				|| (!cave_floor_bold(zcave, starty, startx)) );
	}

	//printf("finding area (%d,%d)\n",startx,starty);
	/* Place the player in an empty space */
	//for (j = 0; j < 1500; ++j)
	for (j = 0; j < 5000; j++)
	{
		/* Increasing distance */
		d = (j + 149) / 150;

		/* Pick a location */
		scatter(wpos, &y, &x, starty, startx, d, 1);

		/* Must have an "empty" grid */
		if (!cave_empty_bold(zcave, y, x)) continue;

		/* Not allowed to go onto a icky location (house) if Depth <= 0 */
		if(wpos->wz==0){
			if((zcave[y][x].info & CAVE_ICKY) && (p_ptr->new_level_method!=LEVEL_HOUSE)) continue;
			if(!(zcave[y][x].info & CAVE_ICKY) && (p_ptr->new_level_method==LEVEL_HOUSE)) continue;
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
	for (j = 0; j < 1500; ++j)
	{
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

	for (m_idx = m_top - 1; m_idx >= 0; m_idx--)
	{
		monster_type *m_ptr = &m_list[m_fast[m_idx]];
		cave_type **mcave;
		mcave=getcave(&m_ptr->wpos);

		if (!m_fast[m_idx]) continue;

		/* Excise "dead" monsters */
		if (!m_ptr->r_idx) continue;

		if (m_ptr->owner != p_ptr->id) continue;

		if (m_ptr->mind & GOLEM_GUARD && !(m_ptr->mind&GOLEM_FOLLOW)) continue;

		/* XXX XXX XXX (merging) */
		starty = m_ptr->fy;
		startx = m_ptr->fx;
		starty = y;
		startx = x;

		if (!m_ptr->wpos.wz && !(m_ptr->mind&GOLEM_FOLLOW)) continue;
		/*
		   if (m_ptr->mind & GOLEM_GUARD && !(m_ptr->mind&GOLEM_FOLLOW)) continue;
		   */

		/* Place the golems in an empty space */
		for (j = 0; j < 1500; ++j)
		{
			/* Increasing distance */
			d = (j + 149) / 150;

			/* Pick a location */
			scatter(wpos, &my, &mx, starty, startx, d, 0);

			/* Must have an "empty" grid */
			if (!cave_empty_bold(zcave, my, mx)) continue;

			/* Not allowed to go onto a icky location (house) if Depth <= 0 */
			if ((wpos->wz==0) && (zcave[my][mx].info & CAVE_ICKY))
				continue;
			break;
		}
		if(mcave)
		{
			mcave[m_ptr->fy][m_ptr->fx].m_idx = 0;
			everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
		}
		wpcopy(&m_ptr->wpos,wpos);
		mcave=getcave(&m_ptr->wpos);
		m_ptr->fx = mx;
		m_ptr->fy = my;
		if(mcave)
		{
			mcave[m_ptr->fy][m_ptr->fx].m_idx = m_fast[m_idx];
			everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
		}

		/* Update the monster (new location) */
		update_mon(m_fast[m_idx], TRUE);
	}
#if 0
	while (TRUE)
	{
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

	p_ptr->redraw |= (PR_MAP);
	p_ptr->redraw |= (PR_DEPTH);

	panel_bounds(Ind);
	forget_view(Ind);
	forget_lite(Ind);
	update_view(Ind);
	update_lite(Ind);
	update_monsters(TRUE);

	/* Tell him that he should beware */
	if (wpos->wz==0 && !istown(wpos))
	{
		if (wild_info[wpos->wy][wpos->wx].own)
		{
			cptr p = lookup_player_name(wild_info[wpos->wy][wpos->wx].own);
			if (p == NULL) p = "Someone";

			msg_format(Ind, "You enter the land of %s.", p);
		}
	}

	/* Clear the flag */
	p_ptr->new_level_flag = FALSE;

	/* Did we enter a no-tele vault? */
        if(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK)
		msg_print(Ind, "\377DThe air in here feels very still.");

	/* Hack -- jail her/him */
	if(!p_ptr->wpos.wz && p_ptr->tim_susp){
		imprison(Ind, 0, "old crimes");
	}
	
	/* daylight problems for vampires */
	if (!p_ptr->wpos.wz && p_ptr->prace == RACE_VAMPIRE) calc_bonuses(Ind);
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

#ifdef ARCADE_SERVER
        if (turn % (cfg.fps / 3) == 1) exec_lua(1, "firin()");
        if (turn % tron_speed == 1) exec_lua(1, "tron()");
#endif

	int i;
	/* Return if no one is playing */
	/* if (!NumPlayers) return; */

	/* Check for death.  Go backwards (very important!) */
	for (i = NumPlayers; i > 0; i--)
	{
		/* Check connection first */
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Check for death */
		if (Players[i]->death)
			player_death(i);
	}

	/* Check player's depth info */
	for (i = 1; i < NumPlayers + 1; i++)
	{
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

	// Note -- this is the END of the last turn

	/* Do final end of turn processing for each player */
	for (i = 1; i < NumPlayers + 1; i++)
	{
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Actually process that player */
		process_player_end(i);
	}

	/* paranoia - It's done twice */
	/* Check for death.  Go backwards (very important!) */
	for (i = NumPlayers; i > 0; i--)
	{
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
	if (turn < 0)
	{
		/* Reset the turn counter */
		/* This will cause some weird things */
		/* Players will probably timeout */
		turn = 1;

		/* Log it */
		s_printf("%s: TURN COUNTER RESET\n", showtime());

		/* Reset empty party creation times */
		for (i = 1; i < MAX_PARTIES; i++) {
			if (parties[i].members == 0) parties[i].created = 0;
		}
	}

	/* Do some beginning of turn processing for each player */
	for (i = 1; i < NumPlayers + 1; i++)
	{
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Play server-side animations (consisting of cycling/random colour choices,
		   instead of animated colours which are circled client-side) */
		if (!(turn % 10)) /* && Players[i]->body_monster */
			everyone_lite_spot(&Players[i]->wpos, Players[i]->py, Players[i]->px);

		/* Perform beeping players who are currently being paged by others */
		if (Players[i]->paging && !(turn % 15)) {
			Send_beep(i);
			Players[i]->paging--;
		}

		/* Actually process that player */
		process_player_begin(i);
	}

	/* Process spell effects */
	process_effects();

	/* Process all of the monsters */
	if (!(turn % MONSTER_TURNS))
		process_monsters();

	/* Process programmable NPCs */
	if (!(turn % NPC_TURNS))
		process_npcs();

	/* Process all of the objects */
	if ((turn % 10) == 5) process_objects();

	/* Process the world */
	if (!(turn % 50))
	{
		if(cfg.runlevel<6 && time(NULL)-cfg.closetime>120)
			set_runlevel(cfg.runlevel-1);

		if(cfg.runlevel<5)
		{
			for(i=NumPlayers; i>0 ;i--)
			{
				if(Players[i]->conn==NOT_CONNECTED) continue;
				if(Players[i]->wpos.wz!=0) break;
			}
			if(!i) set_runlevel(0);
		}

		if(cfg.runlevel == 2049) {
			shutdown_server();
		}
		if(cfg.runlevel == 2048)
		{
			for(i = NumPlayers; i > 0 ;i--)
			{
				if(Players[i]->conn==NOT_CONNECTED) continue;
				/* Ignore admins that are loged in */
				if(admin_p(i)) continue;
				/* Ignore characters that are afk and not in a dungeon/tower */
//				if((Players[i]->wpos.wz == 0) && (Players[i]->afk)) continue;
				/* Ignore characters that are not in a dungeon/tower */
				if(Players[i]->wpos.wz == 0) {
					/* Don't interrupt events though */
					if (Players[i]->wpos.wx || Players[i]->wpos.wy || !sector00separation) continue;
				}
				break;
			}
			if(!i) {
				msg_broadcast(-1, "\377o<<<Server is being updated, but will be up again in no time.>>>");
				cfg.runlevel = 2049;
			}
		}
		if(cfg.runlevel == 2047)
		{
			int n = 0;
			for(i = NumPlayers; i > 0 ;i--)
			{
				if(Players[i]->conn==NOT_CONNECTED) continue;
				/* Ignore admins that are loged in */
				if(admin_p(i)) continue;
				/* count players */
				n++;
				/* Ignore characters that are afk and not in a dungeon/tower */
//				if((Players[i]->wpos.wz == 0) && (Players[i]->afk)) continue;
				/* Ignore characters that are not in a dungeon/tower */
				if(Players[i]->wpos.wz == 0) {
					/* Don't interrupt events though */
					if (Players[i]->wpos.wx || Players[i]->wpos.wy || !sector00separation) continue;
				}
				break;
			}
			if(!i && (n <= 5)) {
				msg_broadcast(-1, "\377o<<<Server is being updated, but will be up again in no time.>>>");
				cfg.runlevel = 2049;
			}
		}
		if(cfg.runlevel == 2046)
		{
			int n = 0;
			for(i = NumPlayers; i > 0 ;i--)
			{
				if(Players[i]->conn==NOT_CONNECTED) continue;
				/* Ignore admins that are loged in */
				if(admin_p(i)) continue;
				/* count players */
				n++;
				/* Ignore characters that are afk and not in a dungeon/tower */
//				if((Players[i]->wpos.wz == 0) && (Players[i]->afk)) continue;
				/* Ignore characters that are not in a dungeon/tower */
				if(Players[i]->wpos.wz == 0) {
					/* Don't interrupt events though */
					if (Players[i]->wpos.wx || Players[i]->wpos.wy || !sector00separation) continue;
				}
				break;
			}
			if(!i && (n <= 2)) {
				msg_broadcast(-1, "\377o<<<Server is being updated, but will be up again in no time.>>>");
				cfg.runlevel = 2049;
			}
		}
		if(cfg.runlevel == 2045)
		{
			int n = 0;
			for(i = NumPlayers; i > 0 ;i--)
			{
				if(Players[i]->conn==NOT_CONNECTED) continue;
				/* Ignore admins that are loged in */
				if(admin_p(i)) continue;
				/* count players */
				n++;
			}
			if(!n) {
				msg_broadcast(-1, "\377o<<<Server is being updated, but will be up again in no time.>>>");
				cfg.runlevel = 2049;
			}
		}

		/* Hack -- Compact the object list occasionally */
		if (o_top + 160 > MAX_O_IDX) compact_objects(320, TRUE);

		/* Hack -- Compact the monster list occasionally */
		if (m_top + 320 > MAX_M_IDX) compact_monsters(640, TRUE);

		/* Hack -- Compact the trap list occasionally */
//		if (t_top + 160 > MAX_TR_IDX) compact_traps(320, TRUE);

		/* Tell a day passed */
		if (((turn + (DAY_START * 10L)) % (10L * DAY)) == 0)
		{
			char buf[20];

			snprintf(buf, 20, get_day(bst(YEAR, turn) + START_YEAR));
			msg_broadcast_format(0,
					"\377GToday it is %s of the %s year of the third age.",
					get_month_name(bst(DAY, turn), FALSE, FALSE), buf);

	        	/* the_sandman prints a rumour */
		        if (NumPlayers)
			{
				msg_print(1, "Suddenly a thought comes to your mind:");
				fortune(1, TRUE);
			}
		}

		for (i = 1; i < NumPlayers + 1; i++)
		{
			if (Players[i]->conn == NOT_CONNECTED)
				continue;

			/* Process the world of that player */
			process_world(i);
		}
	}

	/* Clean up Bree regularly to prevent too dangerous towns in which weaker characters cant move around */
	if (!(turn % 650000)) { /* 650k ~ 3hours */
		worldpos wp;
		wp.wx=cfg.town_x;wp.wy=cfg.town_y;wp.wz=0;
		wipe_m_list_chance(&wp, 70);
		s_printf("%s Bree auto-genocided.\n", showtime());
	}

	/* Process everything else */
	if (!(turn % 10))
	{
		process_various();

		/* Hack -- Regenerate the monsters every hundred game turns */
		if (!(turn % 100)) regen_monsters();
	}
	
	/* Process day/night changes on world_surface */
	if (!(turn % ((10L * DAY) / 2))) process_day_and_night();
	
	/* Refresh everybody's displays */
	for (i = 1; i < NumPlayers + 1; i++)
	{
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
	for (i = 1; i < NumPlayers + 1; i++)
	{
		/* Colour animation is done in lite_spot */
		lite_spot(i, Players[i]->py, Players[i]->px);
	}

	if (!(turn % cfg.fps))
	{
		/* Process global_events each second */
		process_global_events();
		
#ifdef WINTER_SEASON
		if (!weather) { /* not snowing? */
			if (weather_duration <= 0 && WINTER_SEASON > 0) { /* change weather? */
				weather = 1; /* snowing now */
				weather_duration = WINTER_SEASON * 60 * 4;
			} else if (weather_duration > 0) {
				weather_duration--;
			}
		} else { /* it's currently snowing */
			if (weather_duration <= 0 && (4 - WINTER_SEASON) > 0) { /* change weather? */
				weather = 0; /* stop snowing */
				weather_duration = (4 - WINTER_SEASON) * 60 * 4;
			} else if (weather_duration > 0) {
				weather_duration--;
			}
		}
#else
		if (weather) { /* it's currently raining */
			if (weather_duration <= 0) { /* change weather? */
				weather = 0; /* stop raining */
s_printf("WEATHER: Stopping rain.\n");
				weather_duration = rand_int(1800) + 1800;
			} else weather_duration--;
		} else { /* not raining at the moment */
			if (weather_duration <= 0) { /* change weather? */
				weather = 1; /* start raining */
s_printf("WEATHER: Starting rain.\n");
				weather_duration = rand_int(540) + 60;
			} else weather_duration--;
		}
#endif
		if (wind_gust > 0) wind_gust--; /* gust from east */
		if (wind_gust < 0) wind_gust++; /* gust from west */
		if (!wind_gust_delay) { /* gust of wind coming up? */
			wind_gust_delay = 15 + rand_int(240); /* so sometimes no gust at all during 4-min snow periods */
			wind_gust = rand_int(60) + 5;
			if (rand_int(2)) wind_gust = -wind_gust;
		} else wind_gust_delay--;

#ifdef AUCTION_SYSTEM
		/* Process auctions */
		process_auctions();
#endif

		/* Call a specific lua function every second */
		exec_lua(0, "second_handler()");
	}

#ifdef WINTER_SEASON
	/* if it's snowing, create snowflakes */
	if (weather && !(turn % (cfg.fps / 30)) && !fireworks) {
		worldpos wpos;
		/* Snow in Bree */
		wpos.wx = 32; wpos.wy = 32; wpos.wz = 0;
		cast_snowflake(&wpos, rand_int(MAX_WID - 2) + 1, 8);
	}
#else
	/* if it's raining, create raindrops */
	if (weather && !(turn % (cfg.fps / 60)) && !fireworks) {
		worldpos wpos;
		/* Rain in Bree */
		wpos.wx = 32; wpos.wy = 32; wpos.wz = 0;
		cast_raindrop(&wpos, rand_int(MAX_WID - 2) + 1);
		cast_raindrop(&wpos, rand_int(MAX_WID - 2) + 1);
	}
#endif

#ifdef NEW_YEARS_EVE
	if (fireworks && !(turn % (cfg.fps / 4))) {
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
#endif

	/* Send any information over the network */
	Net_output();
}

void set_runlevel(int val)
{
	static bool meta=TRUE;
	switch(val)
	{
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
			msg_broadcast(0, "\377yWarning. Server shutdown will take place in ten minutes.");
			break;
		case 1024:
			Report_to_meta(META_DIE);
			meta=FALSE;
			break;
			/* Hack -- character edit (possessor) mode */
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
void play_game(bool new_game)
{
	//int i, n;

	/*** Init the wild_info array... for more information see wilderness.c ***/
	init_wild_info();

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

	/* Attempt to load the server state information */
	if (!load_server_info())
	{
		/* Oops */
		quit("broken server savefile(s)");
	}

	/* Nothing loaded */
	if (!server_state_loaded)
	{
		/* Make server state info */
		new_game = TRUE;

		/* Create a new dungeon */
		server_dungeon = FALSE;
	}
	else server_dungeon = TRUE;

	/* Roll new town */
	if (new_game)
	{
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
	if (!server_dungeon)
	{
		struct worldpos twpos;
		twpos.wx=cfg.town_x;
		twpos.wy=cfg.town_y;
		twpos.wz=0;
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
	scan_houses();

	cfg.runlevel=6;		/* Server is running */

	/* Set up the main loop */
	install_timer_tick(dungeon, cfg.fps);

	/* Execute custom startup script - C. Blue */
	exec_lua(0, format("server_startup(\"%s\")", showtime()));

	/* Loop forever */
	sched();

	/* This should never, ever happen */
	s_printf("sched returned!!!\n");

	/* Close stuff */
	close_game();

	/* Quit */
	quit(NULL);
}


void shutdown_server(void)
{
	s_printf("Shutting down.\n");

	/* Kick every player out and save his game */
	while(NumPlayers > 0)
	{
		/* Note the we always save the first player */
		player_type *p_ptr = Players[1];


		/* Indicate cause */
		strcpy(p_ptr->died_from, "server shutdown");

		/* Try to save */
		if (!save_player(1)) Destroy_connection(p_ptr->conn, "Server shutdown (save failed)");

		/* Successful save */
		Destroy_connection(p_ptr->conn, "Server shutdown (save succeeded)");
	}

	/* Stop the timer */
	teardown_timer();

	/* Save the server state */
	if (!save_server_info()) quit("Server state save failed!");

	/* Tell the metaserver that we're gone */
	Report_to_meta(META_DIE);

	quit("Server state saved");
}

void pack_overflow(int Ind)
{
	player_type *p_ptr = Players[Ind];
	
	/* XXX XXX XXX Pack Overflow */
	if (p_ptr->inventory[INVEN_PACK].k_idx)
	{
		object_type *o_ptr;
		int amt, i, j = 0;
		u32b f1, f2, f3, f4, f5, esp;
		char	o_name[160];

		/* Choose an item to spill */
		for(i = INVEN_PACK - 1; i >= 0; i--)
		{
			o_ptr = &p_ptr->inventory[i];
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

			if(!check_guard_inscription( o_ptr->note, 'd' ) &&
				!((f4 & TR4_CURSE_NO_DROP) && cursed_p(o_ptr)))
			{
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
		msg_print(Ind, "Your pack overflows!");

		/* Describe */
		object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* Message */
		msg_format(Ind, "You drop %s.", o_name);

		/* Drop it (carefully) near the player */
		drop_near_severe(Ind, o_ptr, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);

		/* Decrease the item, optimize. */
		inven_item_increase(Ind, i, -amt);
		inven_item_optimize(Ind, i);
	}
}
