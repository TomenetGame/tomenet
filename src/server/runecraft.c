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
		msg_print(Ind, "You must be wearing that to attune the rune.");
		return(FALSE);
	}

	/* Artifact? */
	if (o_ptr->name1) {
		msg_print(Ind, "The artifact is unaffected by your attempts!");
		return(FALSE);
	}

	/* One sigil per element! */
	object_type *r_ptr = &p_ptr->inventory[p_ptr->current_activation];
	byte sval = r_ptr->sval;
	if (((p_ptr->inventory[INVEN_WIELD].sigil == sval) && item != INVEN_WIELD)
	 || ((p_ptr->inventory[INVEN_ARM].sigil == sval) && item != INVEN_ARM)
	 || ((p_ptr->inventory[INVEN_BODY].sigil == sval) && item != INVEN_BODY)
	 || ((p_ptr->inventory[INVEN_OUTER].sigil == sval) && item != INVEN_OUTER)
	 || ((p_ptr->inventory[INVEN_HEAD].sigil == sval) && item != INVEN_HEAD)
	 || ((p_ptr->inventory[INVEN_HANDS].sigil == sval) && item != INVEN_HANDS)
	 || ((p_ptr->inventory[INVEN_FEET].sigil == sval) && item != INVEN_FEET)) {
		msg_format(Ind, "You may only inscribe one sigil per element.");
		return(FALSE);
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

#ifdef USE_SOUND_2010
	/* Play a sound! */
	sound(Ind, "item_rune", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

	/* Clean up... */
	inven_item_optimize(Ind, item);

	return(TRUE);
}

/*
 * Leave a "rune of warding" which projects when broken.
 * This otherwise behaves like a normal "glyph of warding".
 * Returns TRUE if a glyph was placed, FALSE otherwise!
 */
bool warding_rune(int Ind, byte typ, int dam, byte rad) {
	player_type *p_ptr = Players[Ind];
	int y = p_ptr->py, x = p_ptr->px;
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave = getcave(wpos);
	cave_type *c_ptr = &zcave[y][x];
	struct c_special *cs_ptr;

	/* Allocate memory for a new rune */
	if (!(cs_ptr = AddCS(c_ptr, CS_RUNE))) return(FALSE);

	/* Preserve the old feature */
	cs_ptr->sc.rune.feat = c_ptr->feat;

	/* Try to place a rune */
	if (!cave_set_feat_live(&p_ptr->wpos, p_ptr->py, p_ptr->px, FEAT_RUNE)) {
		msg_print(Ind, "You cannot place a glyph here.");
		cs_erase(c_ptr, cs_ptr);
		return(FALSE);
	}

	/* Store info */
	cs_ptr->sc.rune.id = p_ptr->id;
	cs_ptr->sc.rune.dam = dam;
	cs_ptr->sc.rune.rad = rad;
	cs_ptr->sc.rune.typ = typ;

	return(TRUE);
}

/*
 * Break a "rune of warding" which explodes when broken.
 * This otherwise behaves like a normal "glyph of warding".
 * Returns TRUE if the monster died, FALSE otherwise!
 */
bool warding_rune_break(int m_idx) {
	monster_type *m_ptr = &m_list[m_idx];
	worldpos *wpos = &m_ptr->wpos;
	cave_type **zcave = getcave(wpos);
	if (!zcave) return(FALSE);
	int my = m_ptr->fy;
	int mx = m_ptr->fx;
	cave_type *c_ptr = &zcave[my][mx];
	struct c_special *cs_ptr = GetCS(c_ptr, CS_RUNE);
	if (!cs_ptr) return(FALSE);

	/* Restore the feature */
	cave_set_feat_live(wpos, my, mx, cs_ptr->sc.rune.feat);

	/* Clear cs_ptr before project() to save some lite_spot packets */
	s32b id = cs_ptr->sc.rune.id;
	s16b d = cs_ptr->sc.rune.dam;
	byte r = cs_ptr->sc.rune.rad;
	byte t = cs_ptr->sc.rune.typ;
	cs_erase(c_ptr, cs_ptr);

	/* XXX Hack -- Owner online? */
	int i, who = PROJECTOR_MON_TRAP;
	player_type *p_ptr = (player_type*)NULL;
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		if (p_ptr->conn == NOT_CONNECTED) continue;
		if (!inarea(&p_ptr->wpos, wpos)) continue;
		/* Check if they are in here */
		if (id == p_ptr->id) {
			who = i;
			break;
		}
	}

	/* Explode if owner is in area, otherwise just refresh grid view */
	if (who > 0) {
		project(0 - who, r, wpos, my, mx, d, t, (PROJECT_JUMP | PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_LODF), "");
#ifdef USE_SOUND_2010
		sound_near_site(my, mx, wpos, 0, "ball", NULL, SFX_TYPE_MISC, FALSE);
#endif
	} else {
		everyone_lite_spot(wpos, my, mx);
	}

	/* Return True if the monster died, false otherwise! */
	return(zcave[my][mx].m_idx == 0 ? TRUE : FALSE);
}
/* For PvP */
bool py_warding_rune_break(int m_idx) {
	return(FALSE);
}
