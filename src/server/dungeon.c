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
#define TOWNIE_RESPAWN_CHANCE	50

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


/*
 * Return a "feeling" (or NULL) about an item.  Method 1 (Heavy).
 */
static cptr value_check_aux1(object_type *o_ptr)
{
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
		return "excellent";
	}

	/* Cursed items */
	if (cursed_p(o_ptr)) return "cursed";

	/* Broken items */
	if (broken_p(o_ptr)) return "broken";

	/* Good "armor" bonus */
	if (o_ptr->to_a > 0) return "good";

	/* Good "weapon" bonus */
	if (o_ptr->to_h + o_ptr->to_d > 0) return "good";

	/* Default to "average" */
	return "average";
}

static cptr value_check_aux1_magic(object_type *o_ptr)
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
			if (k_ptr->cost == 0) return "terrible";

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
			if (k_ptr->cost < 10000) return "good";

			/* Acquirement, Deincarnation, Strength, Blood of Life, ... */
			if (k_ptr->cost >= 10000) return "excellent";

			break;
		}

		/* Food */
		case TV_FOOD:
		{
			/* "Cursed" food */
			if (k_ptr->cost == 0) return "terrible";

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
static cptr value_check_aux2(object_type *o_ptr)
{
	/* Cursed items (all of them) */
	if (cursed_p(o_ptr)) return "cursed";

	/* Broken items (all of them) */
	if (broken_p(o_ptr)) return "broken";

	/* Artifacts -- except cursed/broken ones */
	if (artifact_p(o_ptr)) return "good";

	/* Ego-Items -- except cursed/broken ones */
	if (ego_item_p(o_ptr)) return "good";

	/* Good armor bonus */
	if (o_ptr->to_a > 0) return "good";

	/* Good weapon bonuses */
	if (o_ptr->to_h + o_ptr->to_d > 0) return "good";

	/* No feeling */
	return (NULL);
}

static cptr value_check_aux2_magic(object_type *o_ptr)
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
			if (k_ptr->cost == 0) return "cursed";

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
			if (k_ptr->cost < 10000) return "good";

			/* Acquirement, Deincarnation, Strength, Blood of Life, ... */
			if (k_ptr->cost >= 10000) return "good";

			break;
		}

		/* Food */
		case TV_FOOD:
		{
			/* "Cursed" food */
			if (k_ptr->cost == 0) return "cursed";

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

	object_type *o_ptr;

	char o_name[160];


	/*** Check for "sensing" ***/

	/* No sensing when confused */
	if (p_ptr->confused) return;

	if (!rand_int(133 - get_skill_scale(p_ptr, SKILL_COMBAT, 130))) ok_combat = TRUE;
	if (!rand_int(133 - get_skill_scale(p_ptr, SKILL_ARCHERY, 130))) ok_archery = TRUE;
	if (!rand_int(133 - get_skill_scale(p_ptr, SKILL_MAGIC, 130))) ok_magic = TRUE;
	if (!rand_int(133 - get_skill_scale(p_ptr, SKILL_PRAY, 130))) ok_curse = TRUE;
	if (!ok_combat && !ok_magic && !ok_archery) return;
	heavy = (get_skill(p_ptr, SKILL_COMBAT) > 10) ? TRUE : FALSE;
	heavy_magic = (get_skill(p_ptr, SKILL_MAGIC) > 10) ? TRUE : FALSE;
	heavy_archery = (get_skill(p_ptr, SKILL_ARCHERY) > 10) ? TRUE : FALSE;


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

		if (ok_curse && cursed_p(o_ptr)) feel = value_check_aux1(o_ptr);

		/* Valid "tval" codes */
		switch (o_ptr->tval)
		{
			case TV_DIGGING:
			case TV_HAFTED:
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
				break;
			}

			case TV_MSTAFF:
			{
				if (ok_magic)
					feel = (heavy_magic ? value_check_aux1(o_ptr) :
							value_check_aux2(o_ptr));
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

		/* Inscribe it textually */
		if (!o_ptr->note) o_ptr->note = quark_add(feel);

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
	new_chp = ((long)p_ptr->mhp) * percent + PY_REGEN_HPBASE;
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
	new_mana = ((long)p_ptr->msp) * percent + PY_REGEN_MNBASE;
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
void erase_effects(int effect)
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
							PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE | PROJECT_JUMP);

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
					{
						c_ptr->effect = 0;
						everyone_lite_spot(wpos, j, i);
					}
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
						(distance(e_ptr->cy, e_ptr->cx, j, i) == e_ptr->rad))
				{
					c_ptr->effect = k;
					everyone_lite_spot(wpos, j, i);
				}
			}
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

	//object_type		*o_ptr;


	/* Every 50 game turns */
	/* Check moved */
//	if (turn % 50) return;


	/*** Check the Time and Load ***/
	/* The server will never quit --KLJ-- */

	/*** Handle the "town" (stores and sunshine) ***/

	/* While in town or wilderness */
	if (p_ptr->wpos.wz==0)
	{
		/* Hack -- Daybreak/Nighfall in town */
//		if (!(turn % ((10L * TOWN_DAWN) / 2)))
		if (!(turn % ((10L * DAY) / 2)))
		{
			bool dawn;
			struct worldpos *wpos=&p_ptr->wpos;
			cave_type **zcave;
			if(!(zcave=getcave(wpos))) return;

			/* Check for dawn */
//			dawn = (!(turn % (10L * TOWN_DAWN)));
			dawn = (!(turn % (10L * DAY)));

			/* Day breaks */
			if (dawn)
			{
				/* Message */
				msg_print(Ind, "The sun has risen.");
	
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

						/* Hack -- Memorize lit grids if allowed */
						if ((istown(wpos)) && (p_ptr->view_perma_grids)) *w_ptr |= CAVE_MARK;

						/* Hack -- Notice spot */
						/* XXX it's slow (sunrise/sunset lag) */
						note_spot(Ind, y, x);						
					}			
				}
			}

			/* Night falls */
			else
			{
				int stores = 0, y1, x1;
				byte sx[255], sy[255];

				/* Message  */
				msg_print(Ind, "The sun has fallen.");

				 /* Hack -- Scan the level */
				for (y = 0; y < MAX_HGT; y++)
				{
					for (x = 0; x < MAX_WID; x++)
					{
						/*  Get the cave grid */
						c_ptr = &zcave[y][x];
						w_ptr = &p_ptr->cave_flag[y][x];

						/*  Darken "boring" features */
//						if (c_ptr->feat <= FEAT_INVIS && !(c_ptr->info & CAVE_ROOM))
						if (cave_plain_floor_grid(c_ptr) && !(c_ptr->info & CAVE_ROOM))
						{
							  /* Forget the grid */ 
							c_ptr->info &= ~CAVE_GLOW;
							*w_ptr &= ~CAVE_MARK;

							/* Hack -- Notice spot */
							note_spot(Ind, y, x);
						}						

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


	/*** Process the monsters ***/
	/* Note : since monsters are added at a constant rate in real time,
	 * this corresponds in game time to them appearing at faster rates
	 * deeper in the dungeon.
	 */

	/* Check for creature generation */
	if ((rand_int(MAX_M_ALLOC_CHANCE) == 0) && ((!istown(&p_ptr->wpos)) || magik(TOWNIE_RESPAWN_CHANCE)))
	{
		dun_level *l_ptr = getfloor(&p_ptr->wpos);
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
	//object_type		*o_ptr;
	int spell = 0;

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
			spell = *(inscription + 1) - 96;

			/* shape-changing for retaliation is not so nice idea, eh? */
			if (spell < 2)
			{
				do_cmd_mimic(Ind, 0);
				return TRUE;
			}
			else
			{
				int power = retaliate_mimic_power(Ind, spell);
				if (power)
				{
					do_cmd_mimic(Ind, power + 1);
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

		switch (p_ptr->inventory[item].tval)
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
		spell = *inscription - 96 - 1;
		if (spell < 0 || spell > 9) spell = 0;
	}

	switch (p_ptr->inventory[item].tval)
	{
		/* weapon -- attack normally! */
		case TV_MSTAFF:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_AXE:
			if (item == INVEN_WIELD) return FALSE;
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

				do_cmd_fire(Ind, 5);
				return TRUE;
			}
			break;

		case TV_BOOMERANG:
			if (item == INVEN_BOW)
			{
				do_cmd_fire(Ind, 5);
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
					item = i;
					i = INVEN_TOTAL;
					break;
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

	/*
	int			i;
	object_type		*o_ptr;
	*/


	/* Give the player some energy */
	p_ptr->energy += extract_energy[p_ptr->pspeed];
#ifdef DUMB_WIN
        p_ptr->energy = 32000;
#endif

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
       	if (p_ptr->paralyzed || p_ptr->stun >= 100)
       		p_ptr->energy = 0;

	/* Hack -- semi-constant hallucination */
	if (p_ptr->image && (randint(10) < 1)) p_ptr->redraw |= (PR_MAP);

	/* Mega-Hack -- Random teleportation XXX XXX XXX */
	if ((p_ptr->teleport) && (rand_int(100) < 1))
	{
		/* Teleport player */
		teleport_player(Ind, 40);
	}

}


/*
 * Generate the feature effect
 */
void apply_effect(int Ind)
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
				        PROJECT_KILL | PROJECT_HIDE | PROJECT_JUMP);

				/* Hack -- notice death */
//				if (!alive || death) return;
				if (p_ptr->death) return;
			}
		}
	}
}


/* Handles WoR
 * XXX dirty -- REWRITEME
 */
void do_recall(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* sorta circumlocution? */
	worldpos new_pos;
	bool recall_ok=TRUE;

	/* Disturbing! */
	disturb(Ind, 0, 0);

	/* Determine the level */
	/* recalling to surface */
	if(p_ptr->wpos.wz)
	{
		struct dungeon_type *d_ptr;
		d_ptr=getdungeon(&p_ptr->wpos);

		/* Messages */
		if(d_ptr->flags2 & DF2_IRON && d_ptr->maxdepth>ABS(p_ptr->wpos.wz)){
			msg_print(Ind, "You feel yourself being pulled toward the surface!");
			recall_ok=FALSE;
		}
		else{
			if(p_ptr->wpos.wz > 0)
			{
				msg_print(Ind, "You feel yourself yanked downwards!");
				msg_format_near(Ind, "%s is yanked downwards!", p_ptr->name);
			}
			else
			{
				msg_print(Ind, "You feel yourself yanked upwards!");
				msg_format_near(Ind, "%s is yanked upwards!", p_ptr->name);
			}

			/* New location */
			new_pos.wx = p_ptr->wpos.wx;
			new_pos.wy = p_ptr->wpos.wy;
			new_pos.wz = 0;
#if 0
			p_ptr->new_level_method = ( istown(&new_pos) ? LEVEL_RAND : LEVEL_OUTSIDE_RAND );
#endif
			p_ptr->new_level_method = (p_ptr->wpos.wz>0 ? LEVEL_DOWN : LEVEL_UP);
		}
	}
	/* beware! bugs inside! (jir) (no longer I hope) */
	/* world travel */
	/* why wz again? (jir) */
	else if (!(p_ptr->recall_pos.wz) || !(wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags & (WILD_F_UP|WILD_F_DOWN) ))
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
		msg_print(Ind, "You feel yourself yanked sideways!");
		msg_format_near(Ind, "%s is yanked sideways!", p_ptr->name);

		/* New location */
		p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;												
	}

	/* into dungeon/tower */
	else if(cfg.runlevel>4)
	{
		wilderness_type *w_ptr = &wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx];
		/* Messages */
		if(p_ptr->recall_pos.wz < 0 && w_ptr->flags & WILD_F_DOWN)
		{
			dungeon_type *d_ptr=wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].dungeon;

			//if(d_ptr->baselevel-p_ptr->max_dlv>2){
			if((!d_ptr->type && d_ptr->baselevel-p_ptr->max_dlv > 2) ||
					(d_ptr->type && d_info[d_ptr->type].min_plev > p_ptr->lev))
			{
				p_ptr->recall_pos.wz = 0;
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
				msg_print(Ind, "You feel yourself yanked downwards!");
				msg_format_near(Ind, "%s is yanked downwards!", p_ptr->name);
			}
		}
		else if(p_ptr->recall_pos.wz > 0 && w_ptr->flags & WILD_F_UP)
		{
			dungeon_type *d_ptr=wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].tower;

			//if(d_ptr->baselevel-p_ptr->max_dlv>2){
			if((!d_ptr->type && d_ptr->baselevel-p_ptr->max_dlv > 2) ||
					(d_ptr->type && d_info[d_ptr->type].min_plev > p_ptr->lev))
			{
				p_ptr->recall_pos.wz = 0;
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
				msg_print(Ind, "You feel yourself yanked upwards!");
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
			msg_print(Ind, "You feel yourself yanked toward nowhere...");
			p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
		}
		else p_ptr->new_level_method = LEVEL_RAND;
	}

	if(recall_ok)
	{
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

		/* He'll be safe for 2 turns */
		set_invuln_short(Ind, 5);	// It runs out if attacking anyway
	}
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
	int		regen_amount, NumPlayers_old=NumPlayers;

	/* Unbelievers "resist" magic */
	//		int minus = (p_ptr->anti_magic)?3:1;
	int minus = 1 + get_skill_scale(p_ptr, SKILL_ANTIMAGIC, 3);
	int recovery = magik(get_skill_scale(p_ptr, SKILL_HEALTH, 100))?3:0;

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
		take_hit(Ind, 1, "poison");
	}

	/* Misc. terrain effects */
	if (!p_ptr->ghost)
	{
		/* Generic terrain effects */
		apply_effect(Ind);

		/* Drowning, but not ghosts */
		if(c_ptr->feat==FEAT_DEEP_WATER)
		{
			if(!p_ptr->fly)
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

					/* temporary abs weight calc */
					if(p_ptr->wt+p_ptr->total_weight/10 > 170 + swim * 2)	// 190
					{
						int factor=(p_ptr->wt+p_ptr->total_weight/10)-150-swim * 2;	// 170
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
								magik(WATER_ITEM_DAMAGE_CHANCE))
						{
							inven_damage(Ind, set_water_destroy, 1);
							if (magik(20)) minus_ac(Ind, 1);
						}

						if(randint(1000-factor)<10)
						{
							msg_print(Ind,"\377rYou are weakened by the exertion of swimming!");
							//							do_dec_stat(Ind, A_STR, STAT_DEC_TEMPORARY);
							dec_stat(Ind, A_STR, 10, STAT_DEC_TEMPORARY);
						}
						take_hit(Ind, hit, "drowning");
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
				int hit = p_ptr->mhp>>6;
				hit += randint(p_ptr->mhp>>5);
				if(!hit) hit=1;

				/* Take CON into consideration(max 30%) */
				if (!magik(adj_str_wgt[p_ptr->stat_ind[A_CON]])) hit = 0;
				if (hit) msg_print(Ind,"\377rYou cannot breathe air!");

				if(randint(1000)<10)
				{
					msg_print(Ind,"\377rYou find it hard to stir!");
					//					do_dec_stat(Ind, A_DEX, STAT_DEC_TEMPORARY);
					dec_stat(Ind, A_DEX, 10, STAT_DEC_TEMPORARY);
				}
				take_hit(Ind, hit, "anoxia");
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
			//if ((PRACE_FLAG(PR1_PASS_TREE) || (get_skill(SKILL_DRUID) > 15)) && (cave[py][px].feat == FEAT_TREES))
#if 0
			if ((p_ptr->prace == RACE_ENT || p_ptr->fly) &&
					(c_ptr->feat == FEAT_TREES))
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
				int amt = 1 + ((p_ptr->lev)/5) + p_ptr->mhp / 100;

				//			cave_no_regen = TRUE;
				if (amt > p_ptr->chp - 1) amt = p_ptr->chp - 1;
				take_hit(Ind, amt, " walls ...");
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
		take_hit(Ind, i, "a fatal wound");
	}

	/*** Check the Food, and Regenerate ***/

	/* Ghosts don't need food */
	/* Allow AFK-hivernation if not hungry */
	if (!p_ptr->ghost && !(p_ptr->afk && p_ptr->food > PY_FOOD_ALERT))
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

				/* Hack -- check to see if we have been kicked off
				 * due to starvation 
				 */

				if (NumPlayers != NumPlayers_old) return (FALSE);
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
			take_hit(Ind, i, "starvation");
		}
	}

	/* Default regeneration */
	regen_amount = PY_REGEN_NORMAL;

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
	if (p_ptr->csp < p_ptr->msp)
	{
		regenmana(Ind, (regen_amount * 5) * (p_ptr->regen_mana ? 2 : 1) / 3);
	}

	/* Regeneration ability */
	if (p_ptr->regenerate)
	{
		regen_amount = regen_amount * 2;
	}

	if (recovery)
	{
		regen_amount = regen_amount * 3 / 2;
	}

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

	/* Disturb if we are done resting */
	if ((p_ptr->resting) && (p_ptr->chp == p_ptr->mhp) && (p_ptr->csp == p_ptr->msp))
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
			p_ptr->stat_cnt[i] -= (1 + recovery);
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
		(void)set_bow_brand(Ind, p_ptr->bow_brand - minus, p_ptr->bow_brand_t, p_ptr->bow_brand_d);
	}

	/* Hack -- Timed ESP */
	if (p_ptr->tim_esp)
	{
		(void)set_tim_esp(Ind, p_ptr->tim_esp - minus);
	}

	/* Hack -- Space/Time Anchor */
	if (p_ptr->st_anchor)
	{
		(void)set_st_anchor(Ind, p_ptr->st_anchor - minus);
	}

	/* Hack -- Prob travel */
	if (p_ptr->prob_travel)
	{
		(void)set_prob_travel(Ind, p_ptr->prob_travel - minus);
	}

	/* Hack -- Avoid traps */
	if (p_ptr->tim_traps)
	{
		(void)set_tim_traps(Ind, p_ptr->tim_traps - minus);
	}

	/* Hack -- Mimicry */
	if (p_ptr->tim_mimic)
	{
		(void)set_mimic(Ind, p_ptr->tim_mimic - 1, p_ptr->tim_mimic_what);
	}

	/* Hack -- Timed manashield */
	if (p_ptr->tim_manashield)
	{
		set_tim_manashield(Ind, p_ptr->tim_manashield - minus);
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
			teleport_player(Ind, 1);
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
		(void)set_tim_wraith(Ind, p_ptr->tim_wraith - minus);
	}

	/* Hack -- Hallucinating */
	if (p_ptr->image)
	{
		(void)set_image(Ind, p_ptr->image - 1 - recovery);
	}

	/* Blindness */
	if (p_ptr->blind)
	{
		(void)set_blind(Ind, p_ptr->blind - 1 - recovery);
	}

	/* Times see-invisible */
	if (p_ptr->tim_invis)
	{
		(void)set_tim_invis(Ind, p_ptr->tim_invis - minus);
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
			(void)set_invis(Ind, p_ptr->tim_invisibility - minus, p_ptr->tim_invis_power);
		}
	}

	/* Timed infra-vision */
	if (p_ptr->tim_infra)
	{
		(void)set_tim_infra(Ind, p_ptr->tim_infra - minus);
	}

	/* Paralysis */
	if (p_ptr->paralyzed)
	{
		(void)set_paralyzed(Ind, p_ptr->paralyzed - 1 - recovery);
	}

	/* Confusion */
	if (p_ptr->confused)
	{
		(void)set_confused(Ind, p_ptr->confused - minus - recovery);
	}

	/* Afraid */
	if (p_ptr->afraid)
	{
		(void)set_afraid(Ind, p_ptr->afraid - minus);
	}

	/* Fast */
	if (p_ptr->fast)
	{
		(void)set_fast(Ind, p_ptr->fast - minus, p_ptr->fast_mod);
	}

	/* Slow */
	if (p_ptr->slow)
	{
		(void)set_slow(Ind, p_ptr->slow - minus - recovery);
	}

	/* Protection from evil */
	if (p_ptr->protevil)
	{
		(void)set_protevil(Ind, p_ptr->protevil - 1);
	}

	/* Invulnerability */
	/* Hack -- make -1 permanent invulnerability */
	if (p_ptr->invuln)
	{
		if (p_ptr->invuln > 0)
			(void)set_invuln(Ind, p_ptr->invuln - minus);
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
#if 0 // DGDGDG make it work :)
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

			/* Cant see ? cant hit */
			if (!los(py, px, m_ptr->fy, m_ptr->fx)) continue;

			/* Do not hurt friends! */
			if (is_friend(m_ptr) >= 0) continue;
			break;
		}

		if (tries)
		{
			char m_name[80];

			monster_desc(m_name, m_ptr, 0);
			msg_format("Lightning strikes %s.", m_name);
			project(0, 0, m_ptr->fy, m_ptr->fx, dam / 3, GF_ELEC,
			        PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE);
			project(0, 0, m_ptr->fy, m_ptr->fx, dam / 3, GF_LITE,
			        PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE);
			project(0, 0, m_ptr->fy, m_ptr->fx, dam / 3, GF_SOUND,
			        PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE);
		}

#endif
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

		/* Apply some healing */
		//(void)set_poisoned(Ind, p_ptr->poisoned - adjust - recovery * 2);
		(void)set_poisoned(Ind, p_ptr->poisoned - (adjust + recovery) * (recovery + 1));
	}

	/* Stun */
	if (p_ptr->stun)
	{
		int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus);

		/* Apply some healing */
		//(void)set_stun(Ind, p_ptr->stun - adjust - recovery * 2);
		(void)set_stun(Ind, p_ptr->stun - (adjust + recovery) * (recovery + 1));
	}

	/* Cut */
	if (p_ptr->cut)
	{
		int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + minus);

		/* Hack -- Truly "mortal" wound */
		if (p_ptr->cut > 1000) adjust = 0;

		/* Biofeedback always helps */
		if (p_ptr->biofeedback) adjust += adjust + 10;

		/* Apply some healing */
		//(void)set_cut(Ind, p_ptr->cut - adjust - recovery * 2);
		(void)set_cut(Ind, p_ptr->cut - (adjust + recovery) * (recovery + 1));
	}

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

				/* Torch disappears */
				if (o_ptr->sval == SV_LITE_TORCH && !o_ptr->timeout)
				{
					/* Decrease the item, optimize. */
					inven_item_increase(Ind, INVEN_LITE, -1);
					//						inven_item_describe(Ind, INVEN_LITE);
					inven_item_optimize(Ind, INVEN_LITE);
				}
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

	/*** Process Inventory ***/

	/* Handle experience draining */
	if (p_ptr->exp_drain && magik(10))
		//			take_xp_hit(Ind, 1, "Draining", TRUE, FALSE);
		take_xp_hit(Ind, 1 + p_ptr->lev / 5 + p_ptr->max_exp / 50000L,
				"Draining", TRUE, FALSE);
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

	/* Handle experience draining.  In Oangband, the effect
	 * is worse, especially for high-level characters.
	 * As per Tolkien, hobbits are resistant.
	 */
	if ((p_ptr->black_breath || p_ptr->black_breath_tmp) &&
			rand_int(200) < (p_ptr->prace == RACE_HOBBIT ? 2 : 5))
	{
		(void)do_dec_stat(Ind, rand_int(6), STAT_DEC_NORMAL);
		take_xp_hit(Ind, 1 + p_ptr->lev / 5 + p_ptr->max_exp / 50000L,
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
		int drain = p_ptr->drain_life + rand_int(p_ptr->mhp / 100);

		take_hit(Ind, drain < p_ptr->chp ? drain : p_ptr->chp, "life draining");
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
		if ((o_ptr->timeout > 0) && ((o_ptr->tval != TV_LITE) || o_ptr->name1))
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

					msg_format(Ind, "\377RYou break the mind link with %s.", p_ptr2->name);
					msg_format(Ind2, "\377R%s breaks the mind link with you.", p_ptr->name);
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
			do_recall(Ind);
		}
	}

	bypass_invuln = FALSE;

	/* Evileye, please tell me if it's right */
	if(zcave[p_ptr->py][p_ptr->px].info&CAVE_STCK) p_ptr->tim_wraith=0;

	return (TRUE);
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

	if (Players[Ind]->conn == NOT_CONNECTED)
		return;

	/* Try to execute any commands on the command queue. */
	process_pending_commands(p_ptr->conn);

	/* Mind Fusion/Control disable the char */
	if (p_ptr->esp_link && p_ptr->esp_link_type && (p_ptr->esp_link_flags & LINKF_OBJ)) return;

	/* Check for auto-retaliate */
	if ((p_ptr->energy >= level_speed(&p_ptr->wpos)) && !p_ptr->confused &&
			(!p_ptr->autooff_retaliator ||
			 !(p_ptr->invuln || p_ptr->tim_manashield)))
	{
		/* Check for nearby monsters and try to kill them */
		/* If auto_retaliate returns nonzero than we attacked
		 * something and so should use energy.
		 */
		if ((attackstatus = auto_retaliate(Ind)))
		{
			/* Use energy */
//			p_ptr->energy -= level_speed(p_ptr->dun_depth);
		}
	}

	/* Handle running -- 5 times the speed of walking */
	while (p_ptr->running && p_ptr->energy >= (level_speed(&p_ptr->wpos)*(cfg.running_speed + 1))/cfg.running_speed)
	{
		run_step(Ind, 0);
		p_ptr->energy -= level_speed(&p_ptr->wpos) / cfg.running_speed;
	}


	/* Notice stuff */
	if (p_ptr->notice) notice_stuff(Ind);

	/* XXX XXX XXX Pack Overflow */
	pack_overflow(Ind);


	/* Process things such as regeneration. */
	/* This used to be processed every 10 turns, but I am changing it to be
	 * processed once every 5/6 of a "dungeon turn". This will make healing
	 * and poison faster with respect to real time < 1750 feet and slower >
	 * 1750 feet.
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
void scan_houses()
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
}


/* Traditional (Vanilla) houses version of cheeze()	- Jir - */
#ifndef USE_MANG_HOUSE_ONLY
void cheeze_trad_house()
{
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
void scan_objs(){
	int i, cnt=0, dcnt=0;
	object_type *o_ptr;
	cave_type **zcave;
//	for(i=0;i<MAX_O_IDX;i++){
	for(i=0;i<o_max;i++){	// minimal job :)
		/* We leave non owned objects */
		o_ptr=&o_list[i];
		if(o_ptr->k_idx && o_ptr->owner && !o_ptr->name1){
			if(!o_ptr->wpos.wz && (zcave=getcave(&o_ptr->wpos))){
				/* XXX noisy warning, eh? */
				if (!in_bounds_array(o_ptr->iy, o_ptr->ix) &&
					in_bounds_array(255 - o_ptr->iy, o_ptr->ix))
				{
					o_ptr->iy = 255 - o_ptr->iy;
				}
				if (in_bounds_array(o_ptr->iy, o_ptr->ix)) // monster trap?
				{
					/* ick suggests a store, so leave) */
					if(!(zcave[o_ptr->iy][o_ptr->ix].info & CAVE_ICKY)){
						if(++o_ptr->marked==2){
							delete_object_idx(zcave[o_ptr->iy][o_ptr->ix].o_idx);
							dcnt++;
						}
					}
				}
	/* Once hourly cheeze check. (logs would fill the hd otherwise ;( */
#if CHEEZELOG_LEVEL > 1
				else
#if CHEEZELOG_LEVEL < 4
					if (!(turn % (cfg.fps * 3600)))
#endif	// CHEEZELOG_LEVEL (4)
						cheeze(o_ptr);
#endif	// CHEEZELOG_LEVEL (1)
				cnt++;
			}
		}
	}
	if(dcnt) s_printf("Scanned %d objects. Removed %d.\n", cnt, dcnt);

#ifndef USE_MANG_HOUSE_ONLY
#if CHEEZELOG_LEVEL > 1
#if CHEEZELOG_LEVEL < 4
	if (!(turn % (cfg.fps * 3600)))
#endif	// CHEEZELOG_LEVEL (4)
		cheeze_trad_house();
#endif	// CHEEZELOG_LEVEL (1)
#endif	// USE_MANG_HOUSE_ONLY

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
/* called only every 10 turns */
static void process_various(void)
{
	int i, j;
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
	}

	if (!(turn % (cfg.fps * 10))){
		purge_old();
	}

	/* Handle certain things once a minute */
	if (!(turn % (cfg.fps * 60)))
	{
		monster_race *r_ptr;

		check_quests();		/* It's enough with 'once a minute', none? -Jir */

		check_banlist();	/* unban some players */
		scan_objs();		/* scan objects and houses */

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

			/* Reswpan for kings' joy  -Jir- */
			/* Update the unique respawn timers */
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

				/* Hack -- Sauron and Morgoth are exceptions */
				if (r_ptr->flags1 & RF1_QUESTOR) continue;

				//				if (r_ptr->max_num > 0) continue;
				if (rand_int(cfg.unique_respawn_time * (r_ptr->level + 1)) > 9)
					continue;

				/* "Ressurect" the unique */
				p_ptr->r_killed[i] = 0;
				// 				r_ptr->max_num = 1;
				//				r_ptr->respawn_timer = -1;

				/* Tell the player */
				msg_format(j,"%s rises from the dead!",(r_name + r_ptr->name));
				//				msg_broadcast(0,buf); 
			}
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
				sprintf(buf,"%s rises from the dead!",(r_name + r_ptr->name));
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
			c_ptr->feat = FEAT_TREE;

			/* Show it */
			everyone_lite_spot(0, y, x);

			/* Done */
			break;
		}
	}
#endif /* if 0 */

#if 0	// no longer
	/* Update the stores */
	if (!(turn % (10L * cfg.store_turns)))
	{
		store_turnover();
	}
#endif	// 0

#if 0
	/* Hack -- Daybreak/Nightfall outside the dungeon */
	if (!(turn % ((10L * TOWN_DAWN) / 2)))
	{
		bool dawn;

		/* Check for dawn */
		dawn = (!(turn % (10L * TOWN_DAWN)));
		/* Day breaks */
		if (dawn)
		{
			/* Mega-Hack -- erase all wilderness monsters.
			 * This should prevent wilderness monster "buildup",
			 * massive worm infestations, and uniques getting
			 * lost out there.
			 */
			struct worldpos twpos;
			twpos.wz=0;
			for(y=0;y<MAX_WILD_Y;y++){
				twpos.wy=y;
				for(x=0;x<MAX_WILD_X;x++){
					twpos.wx=x;
					if(!players_on_depth(&twpos)) wipe_m_list(&twpos);
					wild_info[twpos.wy][twpos.wx].flags&=~(WILD_F_INHABITED);
				}
			}
		
			/* Hack -- Scan the town */
			for (y = 0; y < MAX_HGT; y++)
			{
				for (x = 0; x < MAX_WID; x++)
				{
					 /* Get the cave grid */
					c_ptr = &zcave[0][y][x];

					 /* Assume lit */
					c_ptr->info |= CAVE_GLOW;

					 /* Hack -- Notice spot */
					note_spot_depth(townpos, y, x);
				}
			} 
		}	
		else
		{
			/* Hack -- Scan the town */
			for (y = 0; y < MAX_HGT; y++)
			{
				for (x = 0; x < MAX_WID; x++)
				{
					 /* Get the cave grid */
					c_ptr = &zcave[0][y][x];

					 /* Darken "boring" features */
					if (c_ptr->feat <= FEAT_INVIS && !(c_ptr->info & CAVE_ROOM))
					{
						 /* Darken the grid */
						c_ptr->info &= ~CAVE_GLOW;
					}
				}
			}
			/* hack -- make fruit bat wear off */
			/* No more -- DG */
#if 0			
			for (x = 1; x < NumPlayers + 1; x++)
			{
				if (Players[x]->fruit_bat)
				{
					Players[x]->fruit_bat--;
					if (!Players[x]->fruit_bat)
					msg_print(x, "Your form feels much more familliar.");
				}
			}
#endif
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
	//		if (!p_ptr->admin_dm && !p_ptr->admin_wiz && player_is_king(Ind))
	{
		object_type *o_ptr;
		char		o_name[160];

		for (j = 0; j < INVEN_TOTAL; j++)
		{
			o_ptr = &p_ptr->inventory[j];
			if (!o_ptr->k_idx || !true_artifact_p(o_ptr)) continue;

			/* fix the list */
			if (!a_info[o_ptr->name1].cur_num)
				a_info[o_ptr->name1].cur_num = 1;

			if (!a_info[o_ptr->name1].known && (o_ptr->ident & ID_KNOWN))
				a_info[o_ptr->name1].known = TRUE;

			if (!(cfg.kings_etiquette && p_ptr->total_winner && !is_admin(p_ptr)))
				continue;

			if (strstr((a_name + a_info[o_ptr->name1].name),"Grond") ||
					strstr((a_name + a_info[o_ptr->name1].name),"Morgoth"))
				continue;

			/* Describe the object */
			object_desc(Ind, o_name, o_ptr, TRUE, 0);

			/* Message */
			msg_format(Ind, "\377y%s bids farewell to you...", o_name);
			a_info[o_ptr->name1].cur_num = 0;
			a_info[o_ptr->name1].known = FALSE;

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
		generate_cave(wpos);
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
//		bool dawn = ((turn % (10L * TOWN_DAWN)) < (10L * TOWN_DAWN / 2)); 
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
		case LEVEL_DOWN:  starty = level_down_y(wpos);
						  startx = level_down_x(wpos);
						  break;

						  /* Climbed up */
		case LEVEL_UP:    starty = level_up_y(wpos);
						  startx = level_up_x(wpos);
						  break;

						  /* Teleported level */
		case LEVEL_RAND:  starty = level_rand_y(wpos);
						  startx = level_rand_x(wpos);
						  break;

						  /* Used ghostly travel */
		case LEVEL_GHOST: starty = p_ptr->py;
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
	for (j = 0; j < 1500; j++)
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

	/* Hack -- jail her/him */
	if(!p_ptr->wpos.wz && p_ptr->tim_susp){
		imprison(Ind, 0, "old crimes");
	}
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

	/* Do some beginning of turn processing for each player */
	for (i = 1; i < NumPlayers + 1; i++)
	{
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Actually process that player */
		process_player_begin(i);
	}

	/* Process spell effects */
	if ((turn % 10) == 3) process_effects();

	/* Process all of the monsters */
	if (!(turn % MONSTER_TURNS))
		process_monsters();

	/* Process programmable NPCs */
	if (!(turn % NPC_TURNS))
		process_npcs();

	/* Process all of the objects */
	if ((turn % 10) == 5) process_objects();

	/* Probess the world */
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

			sprintf(buf, get_day(bst(YEAR, turn) + START_YEAR));
			msg_broadcast_format(0,
					"\377GToday it is %s of the %s year of the third age.",
					get_month_name(bst(DAY, turn), FALSE, FALSE), buf);
		}

		for (i = 1; i < NumPlayers + 1; i++)
		{
			if (Players[i]->conn == NOT_CONNECTED)
				continue;

			/* Process the world of that player */
			process_world(i);
		}
	}

	/* Process everything else */
	if (!(turn % 10))
	{
		process_various();

		/* Hack -- Regenerate the monsters every hundred game turns */
		if (!(turn % 100)) regen_monsters();
	}

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
			msg_broadcast(0, "\377yWarning. Server shutdown will take place in five minutes.");
			break;
		case 1024:
			Report_to_meta(META_DIE);
			meta=FALSE;
			break;
			/* Hack -- character edit (possessor) mode */
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

	/* Attempt to load the server state information */
	if (!load_server_info())
	{
		/* Oops */
		quit("broken server savefile(s)");
	}

	/* UltraHack -- clear each wilderness levels inhabited flag, so
	   monsters will respawn.
	   hack -- clear the wild_f_in_memory flag, so house objects are added
	   once and only once.
	
	   I believe this is no longer neccecary.
	for (i = 1; i < MAX_WILD; i++) wild_info[-i].flags &= ~(WILD_F_IN_MEMORY);
	*/

	/* Nothing loaded */
	if (!server_state_loaded)
	{
		/* Make server state info */
		new_game = TRUE;

		/* Create a new dungeon */
		server_dungeon = FALSE;
	}
	else server_dungeon = TRUE;

	/* Process old character */
	if (!new_game)
	{
		/* Process the player name */
		/*process_player_name(FALSE);*/
	}

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

		//initwild();

		/*** Init the wild_info array... for more information see wilderness.c ***/
		//init_wild_info(TRUE);

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

#if 0
		/* Initialize the stores */
		for (n = 0; n < MAX_STORES; n++)
		{
			/* Initialize */
			store_init(n);
	
			/* Ignore home and auction house */
			if ((n == MAX_STORES - 2) || (n == MAX_STORES - 1)) continue;
	
			/* Maintain the shop */
			for (i = 0; i < 10; i++) store_maint(n);
		}
#endif
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
		generate_cave(&twpos);
	}
#if 0
//	else if (!central_town_loaded)
	else 
	{
		struct worldpos twpos;
		twpos.wx=cfg.town_x;
		twpos.wy=cfg.town_y;
		twpos.wz=0;

		/* Actually generate the town */
		generate_cave(&twpos);

		/* Hack -- Build the buildings/stairs (from memory) */
//		town_gen_hack(&twpos);
	}
#endif	// 0


	/* Finish initializing dungeon monsters */
	setup_monsters();

	/* Finish initializing dungeon objects */
	setup_objects();

	/* Finish initializing dungeon objects */
//	setup_traps();

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
	//int i;

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
//		i = INVEN_PACK;

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
