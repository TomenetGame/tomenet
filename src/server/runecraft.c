/* File: runecraft.c */

/* Purpose: Runecraft Spell System
 * Flexible spells with an m-key interface (and macro wizard).
 * Combines two runes with a mode and type to create a spell.
 * Most code is now maintained in lib/scpt/runecraft.lua for easier updates.
 *
 * Runespells may be 'traced' while blind, and depend somewhat on dexterity.
 * Rather than failing a runespell, the caster receives some elemental damage.
 *
 * By Relsiet/Andy Dubbeld (andy@foisdin.com)
 * Maintained by Kurzel (kurzel.tomenet@gmail.com)
 *
 * Credits: Mark, Adam, Relsiet, C.Blue, Kurzel
 */

#define SERVER
#include "angband.h"

/*
 * Activate runes to apply boni via equipment in the form of a sigil.
 * Only allow activation after a set runecraft skill level... 40
 * Allow one sigil per element, scaling with skill variety.
 */
bool rune_enchant(int Ind, int item) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr = &p_ptr->inventory[item];

	/* Not worn? */
	if (INVEN_WIELD > item) {
		msg_print(Ind, "You must be wearing that to attune the rune...");
		return FALSE;
	}

	/* Artifact? */
	if (o_ptr->name1) {
		msg_print(Ind, "The artifact is unaffected by your attempts!");
		return FALSE;
	}

	/* One sigil per element! */
	object_type *r_ptr = &p_ptr->inventory[p_ptr->current_activation];
	byte sval = r_ptr->sval;
	if ((p_ptr->inventory[INVEN_WIELD].sigil == sval)
	 || (p_ptr->inventory[INVEN_ARM].sigil == sval)
	 || (p_ptr->inventory[INVEN_BODY].sigil == sval)
	 || (p_ptr->inventory[INVEN_OUTER].sigil == sval)
	 || (p_ptr->inventory[INVEN_HEAD].sigil == sval)
	 || (p_ptr->inventory[INVEN_HANDS].sigil == sval)
	 || (p_ptr->inventory[INVEN_FEET].sigil == sval)) {
		msg_format(Ind, "You may only inscribe one sigil per element.");
		return FALSE;
	}

	/* Store the SVAL and SSEED */
	o_ptr = &p_ptr->inventory[item];
	o_ptr->sigil = sval;
	/* Save RNG with a fresh cycle, see LCRNG() */
	o_ptr->sseed = Rand_value * 1103515245 + 12345 + turn;

	/* Fully ID and examine immediately */
	identify_fully_item(Ind, item);

	/* Erase the rune */
	inven_item_increase(Ind, p_ptr->current_activation, -1);

	/* Play a sound! */
	sound(Ind, "item_rune", NULL, SFX_TYPE_COMMAND, FALSE);

	/* Clean up... */
	inven_item_optimize(Ind, item);

	return TRUE;
}

/*
 * Leave a "rune of warding" which projects when broken.
 * This otherwise behaves like a normal "glyph of warding".
 */
bool warding_rune(int Ind, byte typ, byte dam) {
	player_type *p_ptr = Players[Ind];
	int y = p_ptr->py, x = p_ptr->px;
	struct worldpos *wpos = &p_ptr->wpos;

	/* Emulate cave_set_feat_live but handle cs_ptr too! */
	cave_type **zcave = getcave(wpos);
	cave_type *c_ptr;
	struct c_special *cs_ptr;

	/* Boundary check */
	if (!(zcave = getcave(wpos))) { msg_print(Ind, "You cannot place a glyph here."); return FALSE; }
	if (!in_bounds(y, x)) { msg_print(Ind, "You cannot place a glyph here."); return FALSE; }
	c_ptr = &zcave[y][x];

	/* Allowed? */
	if ((!allow_terraforming(wpos, FEAT_RUNE) || istown(wpos)) && !is_admin(p_ptr)) { msg_print(Ind, "You cannot place a glyph here."); return FALSE; }

	/* No runes of protection / glyphs of warding on non-empty grids - C. Blue */
	if (!(cave_clean_bold(zcave, y, x) && /* cave_clean_bold also checks for object absence */
	    ((c_ptr->feat == FEAT_NONE) ||
	    (c_ptr->feat == FEAT_FLOOR) ||
	    (c_ptr->feat == FEAT_DIRT) ||
	    (c_ptr->feat == FEAT_LOOSE_DIRT) || /* used for gardens (fields) in wild.c */
	    (c_ptr->feat == FEAT_CROP) || /* used for gardens (fields) in wild.c */
	    (c_ptr->feat == FEAT_GRASS) ||
	    (c_ptr->feat == FEAT_SNOW) ||
	    (c_ptr->feat == FEAT_ICE) ||
	    (c_ptr->feat == FEAT_SAND) ||
	    (c_ptr->feat == FEAT_ASH) ||
	    (c_ptr->feat == FEAT_MUD) ||
	    (c_ptr->feat == FEAT_FLOWER) ||
	    (c_ptr->feat == FEAT_NETHER_MIST)))) {
		msg_print(Ind, "You cannot place a glyph here."); return FALSE;
	}

	/* Don't mess with inns please! */
	if (f_info[c_ptr->feat].flags1 & FF1_PROTECTED) { msg_print(Ind, "You cannot place a glyph here."); return FALSE; }

	/* Allocate memory for a new rune */
	if (!(cs_ptr = AddCS(c_ptr, CS_RUNE))) return FALSE;

	/* Preserve Terrain Feature */
	cs_ptr->sc.rune.feat = c_ptr->feat;

	/* Store minimal runecraft info */
	cs_ptr->sc.rune.typ = typ;
	cs_ptr->sc.rune.lev = dam;
	cs_ptr->sc.rune.id = p_ptr->id;

	/* Change the feature */
	if (c_ptr->feat != FEAT_RUNE) c_ptr->info &= ~CAVE_NEST_PIT; /* clear teleport protection for nest grid if it gets changed */
	c_ptr->feat = FEAT_RUNE;

	int i;
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		/* Only works for players on the level */
		if (!inarea(wpos, &p_ptr->wpos)) continue;

		/* Notice */
		note_spot(i, y, x);

		/* Redraw */
		lite_spot(i, y, x);

		/* Update some things */
		p_ptr->update |= (PU_VIEW | PU_DISTANCE);
	}

	return TRUE;
}

/*
 * Break a "rune of warding" which explodes when broken.
 * This otherwise behaves like a normal "glyph of warding".
 */
bool warding_rune_break(int m_idx) {
	monster_type *m_ptr = &m_list[m_idx];

	/* Location info */
	int mx = m_ptr->fx;
	int my = m_ptr->fy;

	/* Get the relevant level */
	cave_type **zcave;
	worldpos *wpos = &m_ptr->wpos;
	zcave = getcave(wpos);
	/* Paranoia */
	if (!zcave) return(FALSE);

	/* Get the stored rune info */
	struct c_special *cs_ptr;
	cave_type *c_ptr;
	c_ptr = &zcave[my][mx];

	cs_ptr = GetCS(c_ptr, CS_RUNE);
	/* Paranoia */
	if (!cs_ptr) return(FALSE);

	/* XXX Hack -- Owner online? */
	int i, who = PROJECTOR_MON_TRAP;
	player_type *p_ptr = (player_type*)NULL;
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		if (p_ptr->conn == NOT_CONNECTED) continue;
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Check if they are in here */
		if (cs_ptr->sc.rune.id == p_ptr->id) {
			who = i;
			break;
		}
	}

	/* Fire if ready */
	if (who > 0) {

		/* Create the Effect */
		project(0 - who, 0, wpos, my, mx, cs_ptr->sc.rune.lev, cs_ptr->sc.rune.typ, (PROJECT_JUMP | PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_LODF), "");

#ifdef USE_SOUND_2010
		/* Sound */
		sound_near_site(my, mx, wpos, 0, "ball", NULL, SFX_TYPE_MISC, FALSE);
#endif
	}

	/* Restore the original cave feature */
	cave_set_feat_live(wpos, my, mx, cs_ptr->sc.rune.feat);

	/* Cleanup */
	cs_erase(c_ptr, cs_ptr);

	/* Return TRUE if m_idx still alive */
	return (zcave[my][mx].m_idx == 0 ? TRUE : FALSE);
}
