/* $Id$ */
/* File: spells1.c */

/* Purpose: Spell code (part 1) */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"


/* 1/x chance of reducing stats (for elemental attacks) */
#define HURT_CHANCE 16

/* chance of equipments getting hurt from attacks, in percent [2] */
#define HARM_EQUIP_CHANCE	0

/* macro to determine the way stat gets reduced by element attacks */
#define	DAM_STAT_TYPE(inv) \
	(magik(inv*25) ? STAT_DEC_NORMAL : STAT_DEC_TEMPORARY)

/*
 * Maximum lower limit for player teleportation.	[30]
 * This should make teleportation of high-lv players look 'more random'.
 * To disable, comment it out.
 */
#define TELEPORT_MIN_LIMIT	30

/* Default radius of Space-Time anchor.	[12] */
#define ST_ANCHOR_DIS	12

/* Limitation for teleport radius on the wilderness.	[20] */
#define WILDERNESS_TELEPORT_RADIUS	40

/* Is a hurt monster allowed to teleport out of (non no-tele) vaults? */
#define HURT_MONSTER_ICKY_TELEPORT

/* Macro to test in project_p() whether we are hurt by a PvP
   (player vs player) or a normal PvM (player vs monster) attack */
#define IS_PVP	(who < 0 && who >= -NumPlayers)
/* Similar purpose macro (also for take_hit)
   (Note: While i > 0 can also mean special PROJECTOR_... causes,
   i < 0 is exclusively reserved for monsters.) */
#define IS_PLAYER(i)	(i > 0 && i <= NumPlayers)
#define IS_MONSTER(i)	(i < 0)

/* Take damage before applying polymorph effect?
   Traditionally, polymorph would cancel damage instead. - C. Blue */
#define DAMAGE_BEFORE_POLY

/* Don't allow (ball spell) explosions to extend the effective
   reach of the caster over MAX_RANGE? */
#define NO_EXPLOSION_OUT_OF_MAX_RANGE

/* Typical resistance check for all GF_OLD_ and GF_TURN_ attacks */
#define RES_OLD(lev, dam) ((lev) > randint(((dam) - 5) < 1 ? 1 : ((dam) - 5)) + 5)

/* Sleep power of any GF_OLD_SLEEP spell [500] */
#define GF_OLD_SLEEP_DUR 300


 /*
  * Potions "smash open" and cause an area effect when
  * (1) they are shattered while in the player's inventory,
  * due to cold (etc) attacks;
  * (2) they are thrown at a monster, or obstacle;
  * (3) they are shattered by a "cold ball" or other such spell
  * while lying on the floor.
  *
  * Arguments:
  *	who   ---  who caused the potion to shatter (0=player)
  *		  potions that smash on the floor are assumed to
  *		  be caused by no-one (who = 1), as are those that
  *		  shatter inside the player inventory.
  *		  (Not anymore -- I changed this; TY)
  *	y, x  --- coordinates of the potion (or player if
  *		  the potion was in her inventory);
  *	o_ptr --- pointer to the potion object.
  */
/*
 * Copy & Pasted from ToME :)	- Jir -
 * NOTE:
 * seemingly TV_POTION2 are not haldled right (ToME native bug?).
 */
bool potion_smash_effect(int who, worldpos *wpos, int y, int x, int o_sval) {
	int	 radius = 2;
	int	 dt = 0;
	int	 dam = 0;
	bool	ident = FALSE;
	bool	angry = FALSE;
	int	flg = (PROJECT_NORF | PROJECT_JUMP | PROJECT_ITEM | PROJECT_KILL | PROJECT_SELF | PROJECT_NODO);

	switch (o_sval) {
		case SV_POTION_SLIME_MOLD:
		case SV_POTION_WATER:   /* perhaps a 'water' attack? */
		case SV_POTION_APPLE_JUICE:
			return TRUE;

		case SV_POTION_INFRAVISION:
		case SV_POTION_DETECT_INVIS:
		case SV_POTION_SLOW_POISON:
		case SV_POTION_CURE_POISON:
		case SV_POTION_RESIST_HEAT:
		case SV_POTION_RESIST_COLD:
		case SV_POTION_RESTORE_EXP:
		case SV_POTION_ENLIGHTENMENT:
		case SV_POTION_STAR_ENLIGHTENMENT:
		case SV_POTION_SELF_KNOWLEDGE:
		case SV_POTION_RESISTANCE:
		case SV_POTION_INVULNERABILITY:
		//case SV_POTION_NEW_LIFE:
			/* All of the above potions have no effect when shattered */
			return FALSE;

		case SV_POTION_EXPERIENCE:
			dt = GF_EXP;
			dam = 1; /* level */
			ident = TRUE;
			break;
		case SV_POTION_BOLDNESS:
			radius = 3;
			dt = GF_REMFEAR;
			//ident = TRUE;
			dam = 1; /* dummy */
			break;
		case SV_POTION_SALT_WATER:
			dt = GF_OLD_CONF;
			dam = damroll(10, 5);
			ident = TRUE;
			angry = TRUE;

			/* terraform hack: melt ice^^ (Ding's suggestion) */
			{
				cave_type **zcave;
				if (!(zcave = getcave(wpos))) return TRUE; //paranoia
				if (zcave[y][x].feat == FEAT_ICE_WALL) { //100% chance for now, beats fire magic o_O
					if (who < 0 && who > PROJECTOR_UNUSUAL) msg_print(-who, "The ice wall melts.");
					cave_set_feat_live(wpos, y, x, FEAT_SHAL_WATER);
				}
			}

			break;
		case SV_POTION_LOSE_MEMORIES:
			dt = GF_OLD_CONF;
			dam = damroll(10, 11);
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_DEC_STR:
			radius = 1;
			dt = GF_DEC_STR;
			dam = 1; /* dummy */
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_DEC_INT:
			angry = TRUE;
			break;
		case SV_POTION_DEC_WIS:
			angry = TRUE;
			break;
		case SV_POTION_DEC_DEX:
			radius = 1;
			dt = GF_DEC_DEX;
			dam = 1; /* dummy */
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_DEC_CON:
			radius = 1;
			dt = GF_DEC_CON;
			dam = 1; /* dummy */
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_DEC_CHR:
			angry = TRUE;
			break;
		case SV_POTION_RES_STR:
			radius = 1;
			dt = GF_RES_STR;
			dam = 1; /* dummy */
			break;
		case SV_POTION_RES_INT:
			break;
		case SV_POTION_RES_WIS:
			break;
		case SV_POTION_RES_DEX:
			radius = 1;
			dt = GF_RES_DEX;
			dam = 1; /* dummy */
			break;
		case SV_POTION_RES_CON:
			radius = 1;
			dt = GF_RES_CON;
			dam = 1; /* dummy */
			break;
		case SV_POTION_RES_CHR:
			break;
		case SV_POTION_INC_STR:
			radius = 1;
			dt = GF_INC_STR;
			dam = 1; /* dummy */
			ident = TRUE;
			break;
		case SV_POTION_INC_INT:
			break;
		case SV_POTION_INC_WIS:
			break;
		case SV_POTION_INC_DEX:
			radius = 1;
			dt = GF_INC_DEX;
			dam = 1; /* dummy */
			ident = TRUE;
			break;
		case SV_POTION_INC_CON:
			radius = 1;
			dt = GF_INC_CON;
			dam = 1; /* dummy */
			ident = TRUE;
			break;
		case SV_POTION_INC_CHR:
			break;
		case SV_POTION_AUGMENTATION:
			radius = 1;
			dt = GF_AUGMENTATION;
			dam = 1; /* dummy */
			ident = TRUE;
			break;
		case SV_POTION_HEROISM:
		case SV_POTION_BERSERK_STRENGTH:
			radius = 1;
			dt = GF_HERO_MONSTER;
			dam = damroll(2, 10);
			ident = TRUE;
			break;
		case SV_POTION_SLOWNESS:
			radius = 1;
			dt = GF_OLD_SLOW;
			dam = damroll(10, 5);
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_POISON:
			dt = GF_POIS;
			dam = 7;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_BLINDNESS:
			radius = 1;
			dam = damroll(3, 5);
			dt = GF_BLIND;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_CONFUSION: /* Booze */
			radius = 1;
			dam = damroll(10, 8);
			dt = GF_OLD_CONF;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_SLEEP:
			dt = GF_OLD_SLEEP;
			dam = damroll(10, 8);
			angry = TRUE;
			ident = TRUE;
			break;
		case SV_POTION_RUINATION:
			radius = 1;
			dt = GF_RUINATION;
			ident = TRUE;
			angry = TRUE;
			dam = 1; /* dummy */
			break;
		case SV_POTION_DETONATIONS:
			radius = 3;
			//dt = GF_DISINTEGRATE; /* GF_ROCKET;/* was GF_SHARDS */
			dt = GF_DETONATION; /* GF_DETONATION like GF_ROCKET does partially DISI ;) - C. Blue */
			dam = damroll(45, 25);
			aggravate_monsters_floorpos(wpos, y, x);
			angry = TRUE;
			ident = TRUE;
			flg |= PROJECT_LODF; /* obsolete because of PROJECT_JUMP, but just in case */
#ifdef USE_SOUND_2010
			if (who < 0 && who > PROJECTOR_UNUSUAL) sound(-who, "detonation", NULL, SFX_TYPE_MISC, FALSE);
#endif
			break;
		case SV_POTION_DEATH:
			//dt = GF_DEATH_RAY;	/* !! */	/* not implemented yet. */
			dt = GF_NETHER_WEAK; /* special damage type solemnly for potion smash effect */
			dam = damroll(30,30);
			angry = TRUE;
			radius = 2;
			ident = TRUE;
			break;
		case SV_POTION_SPEED:
			dt = GF_OLD_SPEED;
			dam = damroll(5, 3);
			ident = TRUE;
			break;
		case SV_POTION_CURE_LIGHT:
			dt = GF_OLD_HEAL;
			dam = damroll(2,3);
			ident = TRUE;
			break;
		case SV_POTION_CURE_SERIOUS:
			dt = GF_OLD_HEAL;
			dam = damroll(4,3);
			ident = TRUE;
			break;
		case SV_POTION_CURE_CRITICAL:
			dt = GF_OLD_HEAL;
			dam = damroll(6,3);
			ident = TRUE;
			break;
		case SV_POTION_CURING:
			dt = GF_CURING; //GF_OLD_HEAL;
			dam = 0x4 + 0x8 + 0x10 + 0x20 + 0x100; //damroll(5,10);
			ident = TRUE;
			break;
		case SV_POTION_HEALING:
			dt = GF_OLD_HEAL;
			dam = damroll(10,10);
			ident = TRUE;
			break;
		case SV_POTION_STAR_HEALING:
			dt = GF_OLD_HEAL;
			dam = damroll(30,20);
			radius = 1;
			ident = TRUE;
			break;
		case SV_POTION_LIFE:
			dt = GF_LIFEHEAL;
			dam = damroll(30,20);
			radius = 1;
			ident = TRUE;
			break;
#if 0 /* silly. people DRINK these */
		case SV_POTION_RESTORE_MANA:   /* MANA */
			dt = GF_MANA;
			dam = damroll(8,10);
			radius = 1;
			ident = TRUE;
			break;
		case SV_POTION_STAR_RESTORE_MANA:   /* MANA */
			dt = GF_MANA;
			dam = damroll(12,10);
			radius = 1;
			ident = TRUE;
			break;
#endif
		default:
			/* Do nothing */  ;
			return FALSE;
	}

	/* doh! project halves the dam ?! */
	if (dt != GF_CURING) dam *= 2;

	(void) project(who, radius, wpos, y, x, dam, dt,
	    flg, "");

	if (ident && who < 0 && who > PROJECTOR_UNUSUAL) {
		player_type *p_ptr = Players[-who];
		object_type forge;
		int lev;
		
		invcopy(&forge, lookup_kind(TV_POTION, o_sval));
		lev = k_info[forge.k_idx].level;

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

		/* The player is now aware of the object */
		if (!object_aware_p(-who, &forge)) {
			object_aware(-who, &forge);
			if (!(p_ptr->mode & MODE_PVP)) gain_exp(-who, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
		}

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	}

	/* XXX	those potions that explode need to become "known" */
	return angry;
}


/*
 * Helper function -- return a "nearby" race for polymorphing
 *
 * Note that this function is one of the more "dangerous" ones...
 */
s16b poly_r_idx(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];
	int i, r, lev1, lev2;

	/* Hack -- Uniques never polymorph */
	if (r_ptr->flags1 & RF1_UNIQUE) return (r_idx);

	get_mon_num_hook = dungeon_aux;
	get_mon_num_prep(0, reject_uniques);

	/* Allowable range of "levels" for resulting monster */
	lev1 = r_ptr->level - ((randint(20)/randint(9))+1);
	lev2 = r_ptr->level + ((randint(20)/randint(9))+1);

	/* Pick a (possibly new) non-unique race */
	for (i = 0; i < 1000; i++) {
		/* Pick a new race, using a level calculation */
		/* Don't base this on "dlev" */
		/*r = get_mon_num((dlev + r_ptr->level) / 2 + 5, (dlev + r_ptr->level) / 2);*/
		r = get_mon_num(r_ptr->level + 5, r_ptr->level);

		/* Handle failure */
		if (!r) break;

		/* Obtain race */
		r_ptr = &r_info[r];

		/* Ignore unique monsters */
		//if (r_ptr->flags1 & RF1_UNIQUE) continue;

		/* Ignore monsters with incompatible levels */
		if ((r_ptr->level < lev1) || (r_ptr->level > lev2)) continue;

		/* Use that index */
		r_idx = r;

		/* Done */
		break;
	}

	/* Result */
	return (r_idx);
}

/*
 * It is radius-based now.
 *
 * TODO:
 * - give some messages
 * - allow monsters to have st-anchor?
 */
//bool check_st_anchor(struct worldpos *wpos)
bool check_st_anchor(struct worldpos *wpos, int y, int x) {
	int i;
	//dun_level *l_ptr = getfloor(wpos);

	for (i = 1; i <= NumPlayers; i++) {
		player_type *q_ptr = Players[i];

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;

		/* Skip players not on this depth */
		if (!inarea(&q_ptr->wpos, wpos)) continue;

		/* Maybe CAVE_ICKY/CAVE_STCK can be checked here */

		//if (!q_ptr->st_anchor) continue;
		//if (!q_ptr->anti_tele) continue;
		//if ((!q_ptr->res_tele) && (rand_int(100) < 67)) continue;
		if (!q_ptr->resist_continuum) continue;

		/* Compute distance */
		if (distance(y, x, q_ptr->py, q_ptr->px) > ST_ANCHOR_DIS)
			continue;

		//if(istown(wpos) && randint(100)>q_ptr->lev) continue;

		return TRUE;
	  }

	/* Assume no st_anchor */
	return FALSE;
}


/*
 * Teleport a monster, normally up to "dis" grids away.
 *
 * Attempt to move the monster at least "dis/2" grids away.
 *
 * But allow variation to prevent infinite loops.
 */
bool teleport_away(int m_idx, int dis) {
	int		oy, ox, d, i, min;
	int		ny = 0, nx = 0, max_dis = 200, tries = 5000;
#ifdef USE_SOUND_2010
	int org_dis = dis;
#endif

	bool		look = TRUE;

	monster_type	*m_ptr = &m_list[m_idx];
	monster_race	*r_ptr = race_inf(m_ptr);
	dun_level *l_ptr;
	struct worldpos *wpos;
	cave_type **zcave;

	/* Paranoia */
	if (!m_ptr->r_idx) return FALSE;

	if (r_ptr->flags9 & RF9_IM_TELE) return FALSE;

	/* Save the old location */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

	/* Space/Time Anchor */
	if (check_st_anchor(&m_ptr->wpos, oy, ox)) return FALSE;

	wpos = &m_ptr->wpos;
	if (!(zcave = getcave(wpos))) return FALSE;
	l_ptr = getfloor(wpos);

#ifndef HURT_MONSTER_ICKY_TELEPORT
	/* No teleporting/blinking out of any vaults (!) */
	if (zcave[oy][ox].info & CAVE_ICKY) return FALSE;
#else
	/* actually allow teleporting out of a vault, if monster took damage; but still prevent for no-tele vaults */
	if ((zcave[oy][ox].info & CAVE_ICKY) &&
	    /* teleport out when hurt - except for Qs, who stay to summon for most of the fight, since it's their way of fighting! */
	    (m_ptr->hp == m_ptr->maxhp || (r_ptr->d_char == 'Q' && m_ptr->hp >= m_ptr->maxhp / 5)))
		return FALSE;
	if (zcave[oy][ox].info & CAVE_STCK) return FALSE;
#endif

	/* anti-teleport floors apply to monsters too? */
	if (l_ptr && (l_ptr->flags2 & LF2_NO_TELE)) return FALSE;
	if (in_sector00(wpos) && (sector00flags2 & LF2_NO_TELE)) return FALSE;

	/* set distance according to map size, to avoid 'No empty field' failures for very small maps! */
	if (l_ptr && distance(1, 1, l_ptr->wid, l_ptr->hgt) < max_dis)
		max_dis = distance(1, 1, l_ptr->wid, l_ptr->hgt);

	/* Verify max distance */
	if (dis > max_dis) dis = max_dis;

	/* Minimum distance */
	min = dis / 2;

	/* Look until done */
	while (look) {
		/* Verify max distance once here */
		if (dis > max_dis) dis = max_dis;

		/* Try several locations */
		for (i = 0; i < 500; i++) {
			/* Pick a (possibly illegal) location */
			while (--tries) {
				ny = rand_spread(oy, dis);
				nx = rand_spread(ox, dis);
				d = distance(oy, ox, ny, nx);
				if ((d >= min) && (d <= dis)) break;
			}
			if (!tries) return FALSE;

			/* Ignore illegal locations */
			if (!in_bounds4(l_ptr, ny, nx)) continue;

			/* Require "empty" floor space */
			if (!cave_empty_bold(zcave, ny, nx)) continue;

			/* Hack -- no teleport onto glyph of warding */
			if (zcave[ny][nx].feat == FEAT_GLYPH) continue;
			if (zcave[ny][nx].feat == FEAT_RUNE) continue;
			/* No teleporting into vaults and such */
			if (zcave[ny][nx].info & CAVE_ICKY) continue;

			/* No teleportation onto protected grid (8-town-houses) */
			if (zcave[ny][nx].info & CAVE_PROT) continue;
			/* For instant-resurrection into sickbay: avoid ppl blinking into there on purpose, disturbing the patients -_- */
			if (f_info[zcave[ny][nx].feat].flags1 & FF1_PROTECTED) continue;
			/* Not onto (dungeon) stores */
			if (zcave[ny][nx].feat == FEAT_SHOP) continue;

			/* Space/Time Anchor */
			if (check_st_anchor(&m_ptr->wpos, ny, nx)) return FALSE;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;
	}

	/* Update the new location */
	zcave[ny][nx].m_idx = m_idx;
	cave_midx_debug(wpos, ny, nx, m_idx);

	/* Update the old location */
	zcave[oy][ox].m_idx = 0;

	/* Move the monster */
	m_ptr->fy = ny;
	m_ptr->fx = nx;

	/* Update the monster (new location) */
	update_mon(m_idx, TRUE);

	/* Redraw the old grid */
	everyone_lite_spot(wpos, oy, ox);

	/* Redraw the new grid */
	everyone_lite_spot(wpos, ny, nx);

#ifdef USE_SOUND_2010
	if (org_dis <= 20 && org_dis >= 10) sound_near_monster(m_idx, "blink", NULL, SFX_TYPE_COMMAND);
	else if (org_dis > 20) sound_near_monster(m_idx, "teleport", NULL, SFX_TYPE_COMMAND);
#endif

	/* Succeeded. */
	return TRUE;
}


/*
 * Teleport monster next to the player
 */
void teleport_to_player(int Ind, int m_idx) {
	player_type *p_ptr = Players[Ind];
	int ny = 0, nx = 0, oy, ox, d, i, min;
	int dis = 1, max_dis = 200; //If dis = 2, monsters don't appear next to the player. - Kurzel
	bool look = TRUE;

	monster_type *m_ptr = &m_list[m_idx];
	//int attempts = 200;
	int attempts = 5000;

	struct worldpos *wpos = &m_ptr->wpos;
	dun_level *l_ptr;
	cave_type **zcave;
	//if(p_ptr->resist_continuum) {msg_print("The space-time continuum can't be disrupted."); return;}

	/* Paranoia */
	if (!m_ptr->r_idx) return;

	/* "Skill" test */
	//if (randint(100) > m_ptr->level) return;	/* not here */

	if (!(zcave = getcave(wpos))) return;
	l_ptr = getfloor(wpos);

	/* Save the old location */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

	/* Hrm, I cannot remember/see why it's commented out..
	 * maybe pets and golems need it? */
	//if (check_st_anchor(wpos)) return;
	if (check_st_anchor(wpos, oy, ox)) return;

	/* don't teleport out of no-tele */
	if (zcave[oy][ox].info & CAVE_STCK) return;

	/* anti-teleport floors apply to monsters too? */
	if (l_ptr && (l_ptr->flags2 & LF2_NO_TELE)) return;
	if (in_sector00(wpos) && (sector00flags2 & LF2_NO_TELE)) return;

	/* set distance according to map size, to avoid 'No empty field' failures for very small maps! */
	if (l_ptr && distance(1, 1, l_ptr->wid, l_ptr->hgt) < max_dis)
		max_dis = distance(1, 1, l_ptr->wid, l_ptr->hgt);

	/* Verify max distance */
	if (dis > max_dis) dis = max_dis;

	/* Minimum distance */
	min = dis / 2;

	/* Look until done */
	while (look) {
		/* Verify max distance */
		if (dis > max_dis) dis = max_dis;

		/* Try several locations */
		for (i = 0; i < 500; i++) {
			/* Pick a (possibly illegal) location */
			while (--attempts) {
				ny = rand_spread(p_ptr->py, dis);
				nx = rand_spread(p_ptr->px, dis);
				d = distance(p_ptr->py, p_ptr->px, ny, nx);
				if ((d >= min) && (d <= dis)) break;
			}
			if (!attempts) return;

			/* Ignore illegal locations */
			if (!in_bounds(ny, nx)) continue;

			/* Require "empty" floor space */
			if (!cave_empty_bold(zcave, ny, nx)) continue;

			/* Hack -- no teleport onto glyph of warding */
			if (zcave[ny][nx].feat == FEAT_GLYPH) continue;
			if (zcave[ny][nx].feat == FEAT_RUNE) continue;
#if 0
			if (cave[ny][nx].feat == FEAT_MINOR_GLYPH) continue;

			/* ...nor onto the Pattern */
			if ((cave[ny][nx].feat >= FEAT_PATTERN_START) &&
				(cave[ny][nx].feat <= FEAT_PATTERN_XTRA2)) continue;
#endif	/* 0 */

			/* No teleporting into vaults and such */
			/* if (cave[ny][nx].info & (CAVE_ICKY)) continue; */
			if (zcave[ny][nx].info & CAVE_ICKY) continue;

			/* No teleportation onto protected grid (8-town-houses) */
			/* For instant-resurrection into sickbay: avoid ppl blinking into there on purpose, disturbing the patients -_- */
			if (zcave[ny][nx].info & CAVE_PROT) continue;
			if (f_info[zcave[ny][nx].feat].flags1 & FF1_PROTECTED) continue;
			/* Not onto (dungeon) stores */
			if (zcave[ny][nx].feat == FEAT_SHOP) continue;

			if (check_st_anchor(wpos, ny, nx)) return;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;
	}

#ifdef USE_SOUND_2010
	sound(Ind, "blink", NULL, SFX_TYPE_COMMAND, FALSE);
#else
//	sound(SOUND_TPOTHER);
#endif

	/* Update the new location */
	zcave[ny][nx].m_idx = m_idx;
	cave_midx_debug(wpos, ny, nx, m_idx);

	/* Update the old location */
	zcave[oy][ox].m_idx = 0;

	/* Move the monster */
	m_ptr->fy = ny;
	m_ptr->fx = nx;

	/* Update the monster (new location) */
	update_mon(m_idx, TRUE);

	/* Redraw the old grid */
	everyone_lite_spot(wpos, oy, ox);

	/* Redraw the new grid */
	everyone_lite_spot(wpos, ny, nx);
}


/*
 * Teleport the player to a location up to "dis" grids away.
 *
 * If no such spaces are readily available, the distance may increase.
 * Try very hard to move the player at least a quarter that distance.
 */
bool teleport_player(int Ind, int dis, bool ignore_pvp) {
	player_type *p_ptr = Players[Ind];
#if defined(USE_SOUND_2010) || defined(ENABLE_SELF_FLASHING)
	int org_dis = dis;
#endif
	int d, i, min, ox, oy, x = p_ptr->py, y = p_ptr->px;
	int xx , yy, m_idx, max_dis = 150, tries = 3000; /* (max_dis was 200 at some point) */
	worldpos *wpos = &p_ptr->wpos;
	dun_level *l_ptr;

	bool look = TRUE;
	bool left_shop = (dis == 1) || istown(wpos) //istown hack: prevent teleporting people who can't swim into the lake in Bree
	    || isdungeontown(wpos); //for IDDC random towns to avoid lava/water item destruction on logging in

	/* Space/Time Anchor */
	cave_type **zcave;
	if(!(zcave = getcave(wpos))) return FALSE;
	l_ptr = getfloor(wpos);

	if ((p_ptr->global_event_temp & PEVF_NOTELE_00)) return FALSE;
	if (l_ptr && (l_ptr->flags2 & LF2_NO_TELE)) return FALSE;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_TELE)) return FALSE;

	/* Hack -- Teleportation when died is always allowed */
	if (!p_ptr->death) {
		if (p_ptr->mode & MODE_PVP) {
#ifdef HOSTILITY_ABORTS_RUNNING
 #if 1
			/* hack: cut down phase door range in pvp, since rules dont allow running anymore */
			if (dis == 10) dis = 5;
//			if (dis <= 20) dis /= 2;
 #else
			if (ignore_pvp) dis /= 2; /* phase door spell by default works in pvp */
 #endif
#endif

			if (!ignore_pvp && p_ptr->pvp_prevent_tele) {
				msg_print(Ind, "\377yThere's no easy way out of this fight!");
				s_printf("%s TELEPORT_FAIL: pvp_prevent_tele for %s.\n", showtime(), p_ptr->name);
				return FALSE;
			}
		}
		if (p_ptr->anti_tele || check_st_anchor(wpos, p_ptr->py, p_ptr->px)) {
			msg_print(Ind, "\377oYou are surrounded by an anti-teleportation field!");
			s_printf("%s TELEPORT_FAIL: Anti-Tele for %s.\n", showtime(), p_ptr->name);
			return FALSE;
		}
		if (zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) {
			msg_print(Ind, "\377RThis location suppresses teleportation!");
			s_printf("%s TELEPORT_FAIL: Cave-Stck for %s.\n", showtime(), p_ptr->name);
			return FALSE;
		}

#ifdef AUTO_RET_NEW
		/* Don't allow phase/teleport for auto-retaliation methods */
		if (p_ptr->auto_retaliaty) {
			msg_print(Ind, "\377yYou cannot use means of self-translocation for auto-retaliation.");
			return FALSE;
		}
#endif

		//if (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC)) return;
		/* Hack -- on the wilderness one cannot teleport very far */
		/* Double death isnt nice */
		if (!wpos->wz && !istown(wpos) && dis > WILDERNESS_TELEPORT_RADIUS)
			dis = WILDERNESS_TELEPORT_RADIUS;
	}

	/* set distance according to map size, to avoid 'No empty field' failures for very small maps! */
	if (l_ptr && distance(1, 1, l_ptr->wid, l_ptr->hgt) < max_dis)
		max_dis = distance(1, 1, l_ptr->wid, l_ptr->hgt);

	/* Verify max distance */
	if (dis > max_dis) dis = max_dis;

	/* Verify max distance once here */
	if (dis > max_dis) dis = max_dis;

	/* Minimum distance */
	min = dis / 2;

#ifdef TELEPORTATION_MIN_LIMIT
	if (min > TELEPORT_MIN_LIMIT) min = TELEPORT_MIN_LIMIT;
#endif	// TELEPORTATION_MIN_LIMIT

	/* Look until done */
	while (look && tries) {
		/* Verify max distance */
		if (dis > max_dis) dis = max_dis;

		/* Try several locations */
		for (i = 0; i < 500; i++) {
			/* Pick a (possibly illegal) location */
			while (--tries) {
				y = rand_spread(p_ptr->py, dis);
				x = rand_spread(p_ptr->px, dis);

				d = distance(p_ptr->py, p_ptr->px, y, x);
				//plog(format("y%d x%d d%d min%d dis%d", y, x, d, min, dis));
				if ((d >= min) && (d <= dis)) break;
			}
			/* Avoid server hang-up on 100%-tree-maps */
			if (!tries) break;

			/* Ignore illegal locations */
			if (!in_bounds4(l_ptr, y, x)) continue;

			if (!left_shop) {
				/* Require floor space if not ghost */
				if (!p_ptr->ghost && !cave_free_bold(zcave, y, x)) continue;

				/* never teleport onto perma-walls (happens to ghosts in khazad) */
				if (cave_perma_bold(zcave, y, x)) continue;
			} else {
				/* Require floor space if not ghost */
				if (!p_ptr->ghost && !cave_free_bold(zcave, y, x)
				    && !player_can_enter(Ind, zcave[y][x].feat, TRUE))
					continue;

				/* never teleport onto perma-walls (happens to ghosts in khazad) */
				if (cave_perma_bold(zcave, y, x)
				    && !player_can_enter(Ind, zcave[y][x].feat, TRUE))
					continue;
			}

			/* Require empty space if a ghost */
			if (p_ptr->ghost && zcave[y][x].m_idx) continue;

			/* No teleporting into vaults and such */
			if (zcave[y][x].info & CAVE_ICKY) continue;
			/* This prevents teleporting into broken vaults where layouts overlap :/ annoying bug */
			if (zcave[y][x].info & CAVE_STCK) continue;
			/* No teleporting into monster nests (experimental, 2008-05-26) */
			if (zcave[y][x].info & CAVE_NEST_PIT) continue;
			/* For instant-resurrection into sickbay: avoid ppl blinking into there on purpose, disturbing the patients -_- */
			if (zcave[y][x].info & CAVE_PROT) continue;
			if (f_info[zcave[y][x].feat].flags1 & FF1_PROTECTED) continue;

			/* Prevent landing onto a store entrance */
			if (zcave[y][x].feat == FEAT_SHOP) continue;

			if (left_shop) {
				/* Copy/paste from player_can_enter(), could replace it with that (with comfortably=TRUE flag) */
				if (zcave[y][x].feat == FEAT_SHAL_LAVA ||
				    zcave[y][x].feat == FEAT_DEEP_LAVA)
					if (!(p_ptr->immune_fire || (p_ptr->resist_fire && p_ptr->oppose_fire)))
						continue;
				if (zcave[y][x].feat == FEAT_DEEP_WATER)
					//if (!(p_ptr->immune_water || p_ptr->res_water ||
					if (!(p_ptr->can_swim || p_ptr->levitate || p_ptr->ghost || p_ptr->tim_wraith))
						continue;
			}

			/* Never break into st-anchor */
			if (!p_ptr->death && check_st_anchor(wpos, y, x)) return FALSE;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;
	}

	/* No empty field on this map o_O */
	if (!tries) {
		s_printf("%s TELEPORT_FAIL: No empty field found for %s.\n", showtime(), p_ptr->name);
		msg_print(Ind, "oThe teleportation spell strangely fizzles!");
		return FALSE;
	}

	break_cloaking(Ind, 7);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);

	store_exit(Ind);

	/* Get him out of any pending quest input prompts */
	if (p_ptr->request_id >= RID_QUEST && p_ptr->request_id <= RID_QUEST_ACQUIRE + MAX_Q_IDX - 1) {
		Send_request_abort(Ind);
		p_ptr->request_id = RID_NONE;
	}

	/* Save the old location */
	oy = p_ptr->py;
	ox = p_ptr->px;

#ifdef USE_SOUND_2010
	/* note: currently 10 is the usual phase door distance, and spells can get it up to 12. */
	if (org_dis <= 20 && org_dis >= 7) sound(Ind, "phase_door", NULL, SFX_TYPE_COMMAND, TRUE);
	else if (org_dis > 20) {
		sound(Ind, "teleport", NULL, SFX_TYPE_COMMAND, TRUE);
 #ifdef TELEPORT_SURPRISES
		p_ptr->teleported = 3;
 #endif
	}
#else
 #ifdef TELEPORT_SURPRISES
	if (org_dis > 20) p_ptr->teleported = 3;
 #endif
#endif

	/* Move the player */
	p_ptr->py = y;
	p_ptr->px = x;

	grid_affects_player(Ind, ox, oy);

	/* The player isn't on his old spot anymore */
	zcave[oy][ox].m_idx = 0;

	/* The player is on his new spot */
	zcave[y][x].m_idx = 0 - Ind;
	cave_midx_debug(wpos, y, x, -Ind);

	/* Redraw the old spot */
	everyone_lite_spot(wpos, oy, ox);

	/* Don't leave me alone Daisy! */
	if (!p_ptr->ghost) {
		for (d = 1; d <= 9; d++) {
			if (d == 5) continue;

			xx = ox + ddx[d];
			yy = oy + ddy[d];

			if (!in_bounds4(l_ptr, yy, xx)) continue;

			if ((m_idx = zcave[yy][xx].m_idx) > 0) {
				monster_race *r_ptr = race_inf(&m_list[m_idx]);

				if ((r_ptr->flags6 & RF6_TPORT) &&
				    !((r_ptr->flags3 & RF3_RES_TELE) || (r_ptr->flags9 & RF9_IM_TELE))) {
					/*
					 * The latter limitation is to avoid
					 * totally unkillable suckers...
					 */
					if (!(m_list[m_idx].csleep) && mon_will_run(Ind, m_idx) == FALSE) {
						/* "Skill" test */
						if (randint(100) < r_ptr->level)
							teleport_to_player(Ind, m_idx);
					}
				}
			}
		}
	}

#ifdef ENABLE_SELF_FLASHING
	/* flicker player for a moment, to allow for easy location */
	/* not for phase door, except when our view panel has changed from it */
	if (p_ptr->flash_self >= 0 &&
	    (org_dis > 20 || !local_panel(Ind)))
		p_ptr->flash_self = cfg.fps / FLASH_SELF_DIV;
#endif

	/* Redraw the new spot */
	everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);

	/* Check for new panel (redraw map) */
	verify_panel(Ind);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

	/* Update the monsters */
	p_ptr->update |= (PU_DISTANCE);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Handle stuff XXX XXX XXX */
	if (!p_ptr->death) handle_stuff(Ind);

	return TRUE;
}


/*
 * Force teleportation of a player
 *  - mikaelh
 */
void teleport_player_force(int Ind, int dis) {
	bool anti_tele, death;
	player_type *p_ptr = Players[Ind];

	/* Turn off anti-tele */
	anti_tele = p_ptr->anti_tele;
	/* set death flag as hack to escape no-tele vault grids */
	death = p_ptr->death;

	/* hacks */
	p_ptr->anti_tele = FALSE; /* actually already covered by p_ptr->death below */
	p_ptr->death = TRUE;

	teleport_player(Ind, dis, TRUE);

	/* Restore anti-tele */
	p_ptr->anti_tele = anti_tele;
	/* restore death flag */
	p_ptr->death = death;
}


/*
 * Teleport player to a grid near the given location
 *
 * This function is slightly obsessive about correctness.
 * (Not anymore: This function allows teleporting into vaults (!))
 */
void teleport_player_to(int Ind, int ny, int nx) {
	player_type *p_ptr = Players[Ind];

	int y, x, oy, ox, dis = 1, ctr = 0;
	struct worldpos *wpos = &p_ptr->wpos;
	int tries = 3000;
	dun_level *l_ptr;
	cave_type **zcave;
	bool town = istown(wpos);//prevent teleporting people who can't swim into the lake in Bree

	if (!(zcave = getcave(wpos))) return;
	if (p_ptr->anti_tele) return;
	if (zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) return;
	l_ptr = getfloor(wpos);

	if ((p_ptr->global_event_temp & PEVF_NOTELE_00)) return;
	if (l_ptr && (l_ptr->flags2 & LF2_NO_TELE)) return;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_TELE)) return;
	//if (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC)) return;

	if (ny < 1) ny = 1;
	if (nx < 1) nx = 1;
	if (ny > MAX_HGT - 2) ny = MAX_HGT - 2;
	if (nx > MAX_WID - 2) nx = MAX_WID - 2;

	/* Space/Time Anchor */
	if (check_st_anchor(wpos, ny, nx)) return;

	/* Find a usable location */
	while (tries) {
		/* Pick a nearby legal location */
		while (--tries) {
			y = rand_spread(ny, dis);
			x = rand_spread(nx, dis);
			if (in_bounds4(l_ptr, y, x)) break;

			/* Occasionally advance the distance */
			if (++ctr > (4 * dis * dis + 4 * dis + 1)) {
				ctr = 0;
				dis++;
			}
		}
		if (!tries) return;

		if (town) {
			if (zcave[y][x].feat == FEAT_SHAL_LAVA ||
			    zcave[y][x].feat == FEAT_DEEP_LAVA)
				if (!(p_ptr->immune_fire || (p_ptr->resist_fire && p_ptr->oppose_fire))) {
					/* Occasionally advance the distance */
					if (++ctr > (4 * dis * dis + 4 * dis + 1)) {
						ctr = 0;
						dis++;
					}
					continue;
				}
			if (zcave[y][x].feat == FEAT_DEEP_WATER)
				//if (!(p_ptr->immune_water || p_ptr->res_water ||
				if (!(p_ptr->can_swim || p_ptr->levitate || p_ptr->ghost || p_ptr->tim_wraith)) {
					/* Occasionally advance the distance */
					if (++ctr > (4 * dis * dis + 4 * dis + 1)) {
						ctr = 0;
						dis++;
					}
					continue;
				}
		}

		/* Cant telep into houses on world surface..*/
		/* ..and for instant-resurrection into sickbay: avoid ppl blinking into there on purpose, disturbing the patients -_- */
		if (wpos->wz || (!(zcave[y][x].info & (CAVE_ICKY | CAVE_PROT)) && !(f_info[zcave[y][x].feat].flags1 & FF1_PROTECTED))) {
			/* No tele-to into no-tele vaults */
			if (cave_free_bold(zcave, y, x) &&
			    !(zcave[y][x].info & CAVE_STCK)) {
				/* Never break into st-anchor */
				if (!check_st_anchor(wpos, y, x)) {
					/* tele-to success */
					break;
				}
			}
		}

		/* Occasionally advance the distance */
		if (++ctr > (4 * dis * dis + 4 * dis + 1)) {
			ctr = 0;
			dis++;
		}
	}

	break_cloaking(Ind, 7);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
	store_exit(Ind);

	/* Get him out of any pending quest input prompts */
	if (p_ptr->request_id >= RID_QUEST && p_ptr->request_id <= RID_QUEST_ACQUIRE + MAX_Q_IDX - 1) {
		Send_request_abort(Ind);
		p_ptr->request_id = RID_NONE;
	}

	/* Log, to distinguish MOVE_BODY vs TELE_TO related kills just in case */
	s_printf("TELE_TO: '%s' was teleported to %d,%d", p_ptr->name, x, y);
	if ((zcave[y][x].info & CAVE_ICKY)) s_printf(" (ICKY)");
	if ((zcave[y][x].info & CAVE_STCK)) s_printf(" (STCK)");
	s_printf(".\n");

	/* Save the old location */
	oy = p_ptr->py;
	ox = p_ptr->px;

	/* Move the player */
	p_ptr->py = y;
	p_ptr->px = x;

	grid_affects_player(Ind, ox, oy);

	/* The player isn't here anymore */
	zcave[oy][ox].m_idx = 0;

	/* The player is now here */
	zcave[y][x].m_idx = 0 - Ind;
	cave_midx_debug(wpos, y, x, -Ind);

	/* Redraw the old spot */
	everyone_lite_spot(wpos, oy, ox);

#ifdef ENABLE_SELF_FLASHING
#if 0 /* not for tele-to - let's regarded it as a 'blink' here */
	/* flicker player for a moment, to allow for easy location */
	if (p_ptr->flash_self >= 0) p_ptr->flash_self = cfg.fps / FLASH_SELF_DIV;
#else /* exception when our view panel has changed from this tele-to */
	if (p_ptr->flash_self >= 0 && !local_panel(Ind)) p_ptr->flash_self = cfg.fps / FLASH_SELF_DIV;
#endif
#endif

	/* Redraw the new spot */
	everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);

	/* Check for new panel (redraw map) */
	verify_panel(Ind);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

	/* Update the monsters */
	p_ptr->update |= (PU_DISTANCE);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Handle stuff XXX XXX XXX */
	handle_stuff(Ind);

#ifdef USE_SOUND_2010
	sound(Ind, "blink", NULL, SFX_TYPE_COMMAND, TRUE);
#endif
}
void teleport_player_to_force(int Ind, int ny, int nx) {
	bool anti_tele, death;
	player_type *p_ptr = Players[Ind];

	/* Turn off anti-tele */
	anti_tele = p_ptr->anti_tele;
	/* set death flag as hack to escape no-tele vault grids */
	death = p_ptr->death;

	/* hacks */
	p_ptr->anti_tele = FALSE; /* actually already covered by p_ptr->death below */
	p_ptr->death = TRUE;

	teleport_player_to(Ind, ny, nx);

	/* Restore anti-tele */
	p_ptr->anti_tele = anti_tele;
	/* restore death flag */
	p_ptr->death = death;
}



/*
 * Teleport the player one level up or down (random when legal)
 *
 * Note that keeping the "players_on_depth" array correct is VERY important,
 * otherwise levels with players still on them might be destroyed, or empty
 * levels could be kept around, wasting memory.
 */

 /* in the wilderness, teleport to a neighboring wilderness level.
  * 'force' : TRUE only for level-unstaticing, ie administration work.
  *           (can transport players out of dungeons in ways that are not 'legal' (ironman etc).
  * New: Disallow entering/exiting dungeons this way.
  */
void teleport_player_level(int Ind, bool force) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	struct worldpos new_depth, old_wpos;
	dun_level *l_ptr = getfloor(&p_ptr->wpos);
	char *msg = "\377rCritical bug!";
	cave_type **zcave;
	int tries = 100;

	if (!(zcave = getcave(wpos))) return;
	if ((zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) && !force) return;
	//if (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC)) return;

	if ((p_ptr->global_event_temp & PEVF_NOTELE_00)) return;
	if (l_ptr && (l_ptr->flags2 & LF2_NO_TELE)) return;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_TELE)) return;

	/* Space/Time Anchor */
	if (p_ptr->anti_tele && !force) return;
	if (check_st_anchor(&p_ptr->wpos, p_ptr->py, p_ptr->px) && !force) return;

	wpcopy(&old_wpos, wpos);
	wpcopy(&new_depth, wpos);

	/* If in the wilderness, teleport to a random neighboring level.
	   This is especially important to prevent player from entering dungeons
	   that have NO_ENTRY_WOR or similar flags (eg Valinor)! - C. Blue */
	if (wpos->wz == 0) {
		/* get a valid neighbor */
		do {
			switch (rand_int(4)) {
			case DIR_NORTH:
				if (new_depth.wy < MAX_WILD_Y - 1)
					new_depth.wy++;
				msg = "A gust of wind blows you north.";
				break;
			case DIR_EAST:
				if (new_depth.wx < MAX_WILD_X - 1)
					new_depth.wx++;
				msg = "A gust of wind blows you east.";
				break;
			case DIR_SOUTH:
				if (new_depth.wy > 0)
					new_depth.wy--;
				msg = "A gust of wind blows you south.";
				break;
			case DIR_WEST:
				if (new_depth.wx > 0)
					new_depth.wx--;
				msg = "A gust of wind blows you west.";
				break;
			}
		} while (inarea(wpos, &new_depth) && --tries);
		if (!tries) {
			msg = "There is a large magical discharge in the air.";
			return;
		}

		p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
		/* update the players new wilderness location */

		/* update the players wilderness map */
		if (!p_ptr->ghost)
			p_ptr->wild_map[(new_depth.wx + new_depth.wy * MAX_WILD_X) / 8] |=
			    (1U << ((new_depth.wx + new_depth.wy * MAX_WILD_X) % 8));
	/* sometimes go down */
	} else if ((can_go_down(wpos, 0x1) && wpos->wz > 1 &&
	    ((!can_go_up(wpos, 0x1) || wpos->wz >= -1 || (rand_int(100) < 50)) ||
	     (wpos->wz < 0 && wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_IRON)))
	    || (force && can_go_down_simple(wpos))) {
		new_depth.wz--;
		msg = "You sink through the floor.";
		p_ptr->new_level_method = (new_depth.wz || (istown(&new_depth)) ? LEVEL_RAND : LEVEL_OUTSIDE_RAND);
	}
	/* else go up */
	else if ((can_go_up(wpos, 0x1) && wpos->wz < -1 &&
	    !(wpos->wz > 0 && wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_IRON))
	    || (force && can_go_up_simple(wpos))) {
		new_depth.wz++;
		msg = "You rise up through the ceiling.";
		p_ptr->new_level_method = (new_depth.wz || (istown(&new_depth)) ? LEVEL_RAND : LEVEL_OUTSIDE_RAND);
	} else {
		msg = "There is a large magical discharge in the air.";
		return;
	}

	break_cloaking(Ind, 7);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
	store_exit(Ind);

	/* Get him out of any pending quest input prompts */
	if (p_ptr->request_id >= RID_QUEST && p_ptr->request_id <= RID_QUEST_ACQUIRE + MAX_Q_IDX - 1) {
		Send_request_abort(Ind);
		p_ptr->request_id = RID_NONE;
	}

	/* Tell the player */
	msg_print(Ind, msg);

#ifdef USE_SOUND_2010
	sound(Ind, "teleport", NULL, SFX_TYPE_COMMAND, TRUE);
#endif

	/* Remove the player */
	zcave[p_ptr->py][p_ptr->px].m_idx = 0;

	/* Show that he's left */
	everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);

	/* Forget his lite and viewing area */
	forget_lite(Ind);
	forget_view(Ind);

	/* Change the wpos */
	wpcopy(wpos, &new_depth);

	/* One less player here */
	new_players_on_depth(&old_wpos, -1, TRUE);

	/* One more player here */
	new_players_on_depth(wpos, 1, TRUE);

	p_ptr->new_level_flag = TRUE;
}

/* Like teleport_player_level() but does this for ALL players on a wpos at once
   so they end up together again. Added for quest_prepare_zcave(). - C. Blue
   Note: This bypasses all teleportation-preventing aspects.
   New: Disallow entering/exiting dungeons this way. */
void teleport_players_level(struct worldpos *wpos) {
	int i, method = LEVEL_OUTSIDE_RAND, tries = 100;
	player_type *p_ptr;
	struct worldpos new_wpos, old_wpos;
	char *msg = "\377rCritical bug!";
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return;

	wpcopy(&old_wpos, wpos);
	wpcopy(&new_wpos, wpos);

	/* If in the wilderness, teleport to a random neighboring level.
	   This is especially important to prevent player from entering dungeons
	   that have NO_ENTRY_WOR or similar flags (eg Valinor)! - C. Blue */
	if (wpos->wz == 0) {
		/* get a valid neighbor */
		do {
			switch (rand_int(4)) {
			case DIR_NORTH:
				if (new_wpos.wy < MAX_WILD_Y - 1)
					new_wpos.wy++;
				msg = "A sudden magical gust of wind blows you north.";
				break;
			case DIR_EAST:
				if (new_wpos.wx < MAX_WILD_X - 1)
					new_wpos.wx++;
				msg = "A sudden magical gust of wind blows you east.";
				break;
			case DIR_SOUTH:
				if (new_wpos.wy > 0)
					new_wpos.wy--;
				msg = "A sudden magical gust of wind blows you south.";
				break;
			case DIR_WEST:
				if (new_wpos.wx > 0)
					new_wpos.wx--;
				msg = "A sudden magical gust of wind blows you west.";
				break;
			}
		} while (inarea(wpos, &new_wpos) && --tries);
		if (!tries) {
			msg = "There is a large magical discharge in the air.";
			s_printf("Warning: teleport_players_level() failed.");
			return;
		}

		method = LEVEL_OUTSIDE_RAND;
	/* sometimes go down */
	} else if ((can_go_down(wpos, 0x1) && wpos->wz > 1 &&
	    ((!can_go_up(wpos, 0x1) || wpos->wz >= -1 || (rand_int(100) < 50)) ||
	     (wpos->wz < 0 && wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_IRON)))
	    || (can_go_down_simple(wpos))) {
		new_wpos.wz--;
		msg = "Some arcane magic suddenly makes you sink through the floor.";
		method = (new_wpos.wz || (istown(&new_wpos)) ? LEVEL_RAND : LEVEL_OUTSIDE_RAND);
	}
	/* else go up */
	else if ((can_go_up(wpos, 0x1) && wpos->wz < -1 &&
	    !(wpos->wz > 0 && wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_IRON))
	    || (can_go_up_simple(wpos))) {
		new_wpos.wz++;
		msg = "Some arcane magic suddenly makes you rise up through the ceiling.";
		method = (new_wpos.wz || (istown(&new_wpos)) ? LEVEL_RAND : LEVEL_OUTSIDE_RAND);
	} else {
		msg = "There is a large magical discharge in the air.";
		s_printf("Warning: teleport_players_level() failed.");
		return;
	}

	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		p_ptr->new_level_method = method;

		/* update the players new wilderness location */

		/* update the players wilderness map */
		if (!p_ptr->ghost)
			p_ptr->wild_map[(new_wpos.wx + new_wpos.wy * MAX_WILD_X) / 8] |=
			    (1U << ((new_wpos.wx + new_wpos.wy * MAX_WILD_X) % 8));

		break_cloaking(i, 7);
		stop_precision(i);
		stop_shooting_till_kill(i);
		store_exit(i);

		/* Get him out of any pending quest input prompts */
		if (p_ptr->request_id >= RID_QUEST && p_ptr->request_id <= RID_QUEST_ACQUIRE + MAX_Q_IDX - 1) {
			Send_request_abort(i);
			p_ptr->request_id = RID_NONE;
		}

		/* Tell the player */
		msg_print(i, msg);

#ifdef USE_SOUND_2010
		sound(i, "teleport", NULL, SFX_TYPE_COMMAND, TRUE);
#endif

		/* Remove the player */
		zcave[p_ptr->py][p_ptr->px].m_idx = 0;

		/* Show that he's left */
		everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);

		/* Forget his lite and viewing area */
		forget_lite(i);
		forget_view(i);

		/* One less player here */
		new_players_on_depth(&old_wpos, -1, TRUE);

		/* Change the wpos */
		wpcopy(&p_ptr->wpos, &new_wpos);

		p_ptr->new_level_flag = TRUE;

		/* One more player here */
		new_players_on_depth(&new_wpos, 1, TRUE);
	}
}

#ifndef EXTENDED_TERM_COLOURS
/*
 * Return a color to use for the bolt/ball spells
 */
byte spell_color(int type) {
	/* Hack -- fake monochrome */
	if (!use_color) return (TERM_WHITE);

	/* Analyze */
	switch (type) { /* colourful ToME ones :) */
	case GF_MISSILE:	return (TERM_SLATE);
	case GF_ACID:		return (TERM_ACID);
	case GF_ELEC:		return (TERM_ELEC);
	case GF_FIRE:		return (TERM_FIRE);
	case GF_COLD:		return (TERM_COLD);
	case GF_POIS:		return (TERM_POIS);
	case GF_UNBREATH:	return (randint(7) < 3 ? TERM_L_GREEN : TERM_GREEN);
	//case GF_HOLY_ORB:	return (TERM_L_DARK);
	case GF_HOLY_ORB:	return (randint(6) == 1 ? TERM_ORANGE : TERM_L_DARK);
	case GF_HOLY_FIRE:	return (randint(3) != 1 ? TERM_ORANGE : (randint(2) == 1 ? TERM_YELLOW : TERM_WHITE));
	case GF_HELL_FIRE:	return (randint(5) == 1 ? TERM_RED : TERM_L_DARK);
	case GF_MANA:		return (randint(5) != 1 ? TERM_VIOLET : TERM_L_BLUE);
	case GF_ARROW:		return (TERM_L_UMBER);
	case GF_WATER:		return (randint(4) == 1 ? TERM_L_BLUE : TERM_BLUE);
	case GF_WAVE:		return (randint(4) == 1 ? TERM_L_BLUE : TERM_BLUE);
	case GF_VAPOUR:		return (randint(10) == 1 ? TERM_BLUE : TERM_L_BLUE);
	case GF_NETHER_WEAK:
	case GF_NETHER:		return (randint(4) == 1 ? TERM_L_GREEN : TERM_L_DARK);
	case GF_CHAOS:		return (TERM_MULTI);
	case GF_DISENCHANT:	return (randint(4) != 1 ? TERM_ORANGE : TERM_BLUE);
	case GF_NEXUS:		return (randint(5) < 3 ? TERM_L_RED : TERM_VIOLET);
	case GF_CONFUSION:	return (TERM_CONF);
	case GF_SOUND:		return (TERM_SOUN);
	case GF_SHARDS:		return (TERM_SHAR);
	case GF_FORCE:		return (randint(5) < 3 ? TERM_L_WHITE : TERM_ORANGE);
	case GF_INERTIA:	return (randint(5) < 3 ? TERM_SLATE : TERM_L_WHITE);
	case GF_GRAVITY:	return (randint(3) == 1? TERM_L_UMBER : TERM_UMBER);
	case GF_TIME:		return (randint(3) == 1? TERM_GREEN : TERM_L_BLUE);
	case GF_LITE_WEAK:	return (TERM_LITE);
	case GF_LITE:		return (TERM_LITE);
	case GF_DARK_WEAK:	return (TERM_DARKNESS);
	case GF_DARK:		return (TERM_DARKNESS);
	case GF_PLASMA:		return (randint(5) == 1? TERM_RED : TERM_L_RED);
	case GF_METEOR:		return (randint(3) == 1? TERM_RED : TERM_UMBER);
	case GF_ICE:		return (randint(4) == 1? TERM_L_BLUE : TERM_WHITE);
	case GF_HAVOC: //too much hassle to simulate the correct colours, this is just replacement code anyway
	case GF_INFERNO:
	case GF_DETONATION:
	case GF_ROCKET:		return (randint(6) < 4 ? TERM_L_RED : (randint(4) == 1 ? TERM_RED : TERM_L_UMBER));
	case GF_NUKE:		return (mh_attr(2));
	case GF_DISINTEGRATE:   return (randint(3) != 1 ? TERM_L_DARK : (randint(2) == 1 ? TERM_ORANGE : TERM_VIOLET));
	case GF_PSI:		return (randint(5) != 1 ? (rand_int(2) ? (rand_int(2) ? TERM_YELLOW : TERM_L_BLUE) : 127) : TERM_WHITE);
	/* new spell - the_sandman */
	case GF_CURSE:		return (randint(2) == 1 ? TERM_DARKNESS : TERM_L_DARK);
	case GF_OLD_DRAIN:	return (TERM_DARKNESS);
	/* Druids stuff */
	case GF_HEALINGCLOUD:	return (TERM_LITE);//return (randint(5) > 1 ? TERM_WHITE : TERM_L_BLUE);
	case GF_WATERPOISON:	return (TERM_COLD);//return (randint(2) == 1 ? TERM_L_BLUE : (randint(2) == 1 ? TERM_BLUE : (randint(2) == 1 ? TERM_GREEN : TERM_L_GREEN)));
	case GF_ICEPOISON:	return (TERM_SHAR);//return (randint(3) > 1 ? TERM_UMBER : (randint(2) == 1 ? TERM_GREEN : TERM_SLATE));
	/* To remove some hacks? */
	case GF_THUNDER:	return (randint(3) != 1 ? TERM_ELEC : (randint(2) == 1 ? TERM_YELLOW : TERM_LITE));
	case GF_ANNIHILATION:	return (randint(2) == 1 ? TERM_DARKNESS : TERM_L_DARK);
	case GF_OLD_SLEEP:	return TERM_L_DARK; /* Occult: for Veil of Night as wave */
	}

	/* Standard "color" */
	return (TERM_WHITE);
}

/* returns whether a spell type's colours require server-side animation or not.
   (for efficient animations in process_effects()) - C. Blue */
bool spell_color_animation(int type) {
	/* Hack -- fake monochrome */
	if (!use_color) return FALSE;

	/* Analyze */
	switch (type) { /* colourful ToME ones :) */
	case GF_MISSILE:	return FALSE;
	case GF_ACID:		return FALSE;
	case GF_ELEC:		return FALSE;
	case GF_FIRE:		return FALSE;
	case GF_COLD:		return FALSE;
	case GF_POIS:		return FALSE;
	case GF_UNBREATH:	return TRUE;//(randint(7)<3?TERM_L_GREEN:TERM_GREEN);
	//case GF_HOLY_ORB:	return FALSE;
	case GF_HOLY_ORB:	return TRUE;//(randint(6)==1?TERM_ORANGE:TERM_L_DARK);
	case GF_HOLY_FIRE:	return TRUE;//(randint(3)!=1?TERM_ORANGE:(randint(2)==1?TERM_YELLOW:TERM_WHITE));
	case GF_HELL_FIRE:	return TRUE;//(randint(5)==1?TERM_RED:TERM_L_DARK);
	case GF_MANA:		return TRUE;//(randint(5)!=1?TERM_VIOLET:TERM_L_BLUE);
	case GF_ARROW:		return FALSE;
	case GF_WATER:		return TRUE;//(randint(4)==1?TERM_L_BLUE:TERM_BLUE);
	case GF_WAVE:		return TRUE;//(randint(4)==1?TERM_L_BLUE:TERM_BLUE);
	case GF_VAPOUR:		return TRUE;
	case GF_NETHER_WEAK:
	case GF_NETHER:		return TRUE;//(randint(4)==1?TERM_SLATE:TERM_L_DARK);
	case GF_CHAOS:		return FALSE;
	case GF_DISENCHANT:	return TRUE;//(randint(4)==1?TERM_ORANGE:TERM_BLUE;//TERM_L_BLUE:TERM_VIOLET);
	case GF_NEXUS:		return TRUE;//(randint(5)<3?TERM_L_RED:TERM_VIOLET);
	case GF_CONFUSION:	return FALSE;
	case GF_SOUND:		return FALSE;//(randint(4)==1?TERM_VIOLET:TERM_WHITE);
	case GF_SHARDS:		return FALSE;//(randint(5)<3?TERM_UMBER:TERM_SLATE);
	case GF_FORCE:		return TRUE;//(randint(5)<3?TERM_L_WHITE:TERM_ORANGE);
	case GF_INERTIA:	return TRUE;//(randint(5)<3?TERM_SLATE:TERM_L_WHITE);
	case GF_GRAVITY:	return TRUE;//(randint(3)==1?TERM_L_UMBER:TERM_UMBER);
	case GF_TIME:		return TRUE;//(randint(3)==1?TERM_GREEN:TERM_L_BLUE);
	case GF_LITE_WEAK:	return FALSE;
	case GF_LITE:		return FALSE;
	case GF_DARK_WEAK:	return FALSE;
	case GF_DARK:		return FALSE;
	case GF_PLASMA:		return TRUE;//(randint(5)==1?TERM_RED:TERM_L_RED);
	case GF_METEOR:		return TRUE;//(randint(3)==1?TERM_RED:TERM_UMBER);
	case GF_ICE:		return TRUE;//(randint(4)==1?TERM_L_BLUE:TERM_WHITE);
	case GF_HAVOC:		return TRUE;//--complex shits
	case GF_INFERNO:
	case GF_DETONATION:
	case GF_ROCKET:		return TRUE;//(randint(6)<4?TERM_L_RED:(randint(4)==1?TERM_RED:TERM_L_UMBER));
	case GF_NUKE:		return TRUE;//(mh_attr(2));
	case GF_DISINTEGRATE:   return TRUE;//(randint(3)!=1?TERM_L_DARK:(randint(2)==1?TERM_VIOLET:TERM_L_ORANGE));//TERM_ORANGE:TERM_L_UMBER));
	case GF_PSI:		return TRUE;//(randint(5)!=1?(rand_int(2)?(rand_int(2)?TERM_YELLOW:TERM_L_BLUE):127):TERM_WHITE);
	/* new spell - the_sandman */
	case GF_CURSE:		return TRUE;//(randint(2)==1?TERM_DARKNESS:TERM_L_DARK);
	case GF_OLD_DRAIN:	return FALSE;
	/* Druids stuff */
	case GF_HEALINGCLOUD:	return FALSE;//return (randint(5)>1?TERM_WHITE:TERM_L_BLUE);
	case GF_WATERPOISON:	return FALSE;//return (randint(2)==1?TERM_L_BLUE:(randint(2)==1?TERM_BLUE:(randint(2)==1?TERM_GREEN:TERM_L_GREEN)));
	case GF_ICEPOISON:	return FALSE;//return (randint(3)>1?TERM_UMBER:(randint(2)==1?TERM_GREEN:TERM_SLATE));
	/* To remove some hacks? */
	case GF_THUNDER:	return TRUE;//(randint(3)!=1?TERM_ELEC:(randint(2)==1?TERM_YELLOW:TERM_LITE));
	case GF_ANNIHILATION:	return TRUE;//(randint(2)==1?TERM_DARKNESS:TERM_L_DARK);
	case GF_OLD_SLEEP:	return FALSE;/* Occult: For Veil of Night as wave */
	}

	/* Standard "color" */
	return FALSE;
}
#else
/*
 * Return a color to use for the bolt/ball spells
 */
byte spell_color(int type) {
	/* Hack -- fake monochrome */
	if (!use_color) return (TERM_WHITE);

	/* Analyze */
	switch (type) { /* colourful ToME ones :) */
	case GF_MISSILE:	return (TERM_SLATE);
	case GF_ACID:		return (TERM_ACID);
	case GF_ELEC:		return (TERM_ELEC);
	case GF_FIRE:		return (TERM_FIRE);
	case GF_COLD:		return (TERM_COLD);
	case GF_POIS:		return (TERM_POIS);
	case GF_UNBREATH:	return (TERM_UNBREATH);
	//case GF_HOLY_ORB:	return (TERM_L_DARK);
	case GF_HOLY_ORB:	return (TERM_HOLYORB);
	case GF_HOLY_FIRE:	return (TERM_HOLYFIRE);
	case GF_HELL_FIRE:	return (TERM_HELLFIRE);
	case GF_MANA:		return (TERM_MANA);
	case GF_ARROW:		return (TERM_L_UMBER);
	case GF_VAPOUR:		return (TERM_L_BLUE);//animate with some dark blue maybe?
	case GF_WATER:		return (TERM_WATE);
	case GF_WAVE:		return (TERM_WATE);
	case GF_NETHER_WEAK:
	case GF_NETHER:		return (TERM_NETH);
	case GF_CHAOS:		return (TERM_MULTI);
	case GF_DISENCHANT:	return (TERM_DISE);
	case GF_NEXUS:		return (TERM_NEXU);
	case GF_CONFUSION:	return (TERM_CONF);
	case GF_SOUND:		return (TERM_SOUN);
	case GF_SHARDS:		return (TERM_SHAR);
	case GF_FORCE:		return (TERM_FORC);
	case GF_INERTIA:	return (TERM_INER);
	case GF_GRAVITY:	return (TERM_GRAV);
	case GF_TIME:		return (TERM_TIME);
	case GF_STARLITE:	return (TERM_STARLITE);
	case GF_LITE_WEAK:	return (TERM_LITE);
	case GF_LITE:		return (TERM_LITE);
	case GF_DARK_WEAK:	return (TERM_DARKNESS);
	case GF_DARK:		return (TERM_DARKNESS);
	case GF_PLASMA:		return (TERM_PLAS);
	case GF_METEOR:		return (TERM_METEOR);
	case GF_ICE:		return (TERM_ICE);
	case GF_HAVOC:
 /* just until next server update keep some compat code for exactly the current version */
 #if VERSION_MAJOR == 4 && VERSION_MINOR == 7 && VERSION_PATCH == 0 && VERSION_EXTRA == 2
				return TERM_DETO;
 #else
				return TERM_HAVOC;
 #endif
	case GF_INFERNO:
	case GF_DETONATION:
	case GF_ROCKET:		return (TERM_DETO);
	case GF_NUKE:		return (TERM_NUKE);
	case GF_DISINTEGRATE:   return (TERM_DISI);
	case GF_PSI:		return (TERM_PSI);
	/* new spell - the_sandman */
	case GF_CURSE:		return (TERM_CURSE);
	case GF_OLD_DRAIN:	return (TERM_DARKNESS);
	/* Druids stuff */
	case GF_HEALINGCLOUD:	return (TERM_LITE);//return (randint(5)>1?TERM_WHITE:TERM_L_BLUE);
	case GF_WATERPOISON:	return (TERM_COLD);//return (randint(2)==1?TERM_L_BLUE:(randint(2)==1?TERM_BLUE:(randint(2)==1?TERM_GREEN:TERM_L_GREEN)));
	case GF_ICEPOISON:	return (TERM_SHAR);//return (randint(3)>1?TERM_UMBER:(randint(2)==1?TERM_GREEN:TERM_SLATE));
	/* To remove some hacks? */
	case GF_THUNDER:	return (TERM_THUNDER);
	case GF_ANNIHILATION:	return (TERM_ANNI);
	case GF_OLD_SLEEP:	return TERM_L_DARK; /* Occult: for Veil of Night as wave */
	}

	/* Standard "color" */
	return (TERM_WHITE);
}
#endif

/*
 * Decreases players hit points and sets death flag if necessary
 *
 * XXX XXX XXX Invulnerability needs to be changed into a "shield"
 *
 * XXX XXX XXX Hack -- this function allows the user to save (or quit)
 * the game when he dies, since the "You die." message is shown before
 * setting the player to "dead".
 */
/* Poison/Cut timed damages, curse damages etc can bypass the shield.
 * It's hack; in the future, this function should be called with flags
 * that tells 'what kind of damage'.
 */
bool bypass_invuln = FALSE;
void take_hit(int Ind, int damage, cptr hit_from, int Ind_attacker) {
	char hit_from_real[MNAME_LEN];
	player_type *p_ptr = Players[Ind];
	bool drain = !strcmp(hit_from, "life draining");

	// The "number" that the character is displayed as before the hit
	int old_num, new_num;


	/* Amulet of Immortality */
	if (p_ptr->admin_invuln) return;

	/* Heavenly invulnerability? */
	if (p_ptr->martyr && !bypass_invuln) {
		if (-Ind_attacker != PROJECTOR_TERRAIN && !drain) break_cloaking(Ind, 0); /* still notice! paranoia though, rogues can't use martyr */
		return;
	}

	/* Paranoia */
	if (p_ptr->death) return;

	/* towns are safe-zones from ALL hostile actions - except blood bond */
	if (IS_PLAYER(Ind_attacker)) {
		if ((istown(&p_ptr->wpos) || istown(&Players[Ind_attacker]->wpos)) &&
		    (!check_blood_bond(Ind_attacker, Ind) ||
		     p_ptr->afk || Players[Ind_attacker]->afk))
			return;
	}

	strcpy(hit_from_real, hit_from); /* non-hallucinated attacker */

	if (!p_ptr->no_alert) {
		if (((p_ptr->alert_afk_dam && p_ptr->afk)
#ifdef ALERT_OFFPANEL_DAM
		    /* new: alert when we're off-panel (cmd_locate) or in a sticky screen (menus) */
		    || (p_ptr->alert_offpanel_dam && (p_ptr->panel_row_old != p_ptr->panel_row || p_ptr->panel_col_old != p_ptr->panel_col))
#endif
		    )
		    /* don't alert about 0-damage terrain effect */
		    && (damage || -Ind_attacker != PROJECTOR_TERRAIN)
		    /* don't alert about life drain effects (equipment/sunburn) */
		    && !drain
#ifdef USE_SOUND_2010
		    ) {
			Send_warning_beep(Ind);
			//sound(Ind, "warning", "page", SFX_TYPE_MISC, FALSE);
#else
		    && p_ptr->paging == 0) {
			p_ptr->paging = 1;
#endif
		}

#if 0 /* Since 4.6.2 covered by 'alert_offpanel_dam' */
		/* warn if taking (continuous) damage while inside a store! */
		if (p_ptr->store_num != -1) {
 #ifdef USE_SOUND_2010
			Send_warning_beep(Ind);
			//sound(Ind, "warning", "page", SFX_TYPE_MISC, FALSE);
 #else
			if (p_ptr->paging == 0) p_ptr->paging = 1;
 #endif
			/* continuous-damage message only */
			if (bypass_invuln) msg_print(Ind, "\377RWarning - you are taking damage!");
		}
#endif
	}

	// The "number" that the character is displayed as before the hit
	/* Now displays corresponding to mana amount if disruption shield
	is acitavted (C. Blue) */
/*	if (!p_ptr->tim_manashield)
	{
*/		old_num = (p_ptr->chp * 95) / (p_ptr->mhp * 10);
		if (old_num >= 7) old_num = 10;
/*	}
	else if (p_ptr->msp > 0)
	{

		old_num = (p_ptr->csp * 95) / (p_ptr->msp*10);
		if (old_num >= 7) old_num = 10;
	} */

	/* for MODE_PVP only: prevent easy fleeing from PvP encounter >=) */
	if (IS_PLAYER(Ind_attacker) || in_pvparena(&p_ptr->wpos)) {
		p_ptr->pvp_prevent_tele = PVP_COOLDOWN_TELEPORT;
		p_ptr->redraw |= PR_DEPTH;
	}

	// This is probably unused
	// int warning = (p_ptr->mhp * hitpoint_warn / 10);

	/* Hack -- player is secured inside a store/house except in dungeons */
	/* XXX make sure it doesn't "leak"!
	 * Glitchy side-effect: Player stops suffering from poison/cuts while in a store!
	 * ..to fix that, bypass_invuln check has been added. */
	if (p_ptr->store_num != -1 && !p_ptr->wpos.wz && !bypass_invuln) return;

	/* Silyl admin games -- only in totally safe environment! */
	if (p_ptr->admin_set_defeat &&
	    -Ind_attacker != PROJECTOR_TERRAIN && !drain &&
	    ge_special_sector &&
	    in_arena(&p_ptr->wpos)) {
		p_ptr->admin_set_defeat--;
		p_ptr->chp = -1;
		p_ptr->deathblow = 1;
		break_cloaking(Ind, 0);
		goto destined_defeat;
	}

	/* Disturb - except when in PvP! */
	if (!drain && !IS_PLAYER(Ind_attacker)
	    /* hack: no longer disturb on crossing terrain that we're immune to: */
	    && (damage || -Ind_attacker != PROJECTOR_TERRAIN))
		disturb(Ind, 1, 0);

	/* Mega-Hack -- Apply "invulnerability" */
	if (p_ptr->invuln && (!bypass_invuln) && !p_ptr->invuln_applied) {
		/* 1 in 2 chance to fully deflect the damage */
		if (magik(40)) {
			msg_print(Ind, "The attack is fully deflected by a magic shield.");
			if (-Ind_attacker != PROJECTOR_TERRAIN && !drain) break_cloaking(Ind, 0);
			return;
		}

		/* Otherwise damage is reduced by the shield */
		damage = (damage + 1) / 2;
	}

	if (IS_PLAYER(Ind_attacker)) {
		Players[Ind_attacker]->test_count++;
		Players[Ind_attacker]->test_dam += damage;
	}

	/* Re allowed by evileye for power */
#if 1
	//if (p_ptr->tim_manashield)
	if (p_ptr->tim_manashield && (!bypass_invuln)) {
		if (p_ptr->csp > 0) {
			int taken;

			switch(p_ptr->pclass) {
			case CLASS_MAGE:
			taken = (damage * 2) / 2;//mana shield works with a ratio of SP<->damage points
			break;
			default:
			taken = (damage * 2) / 1;
			break;
			}

			if (p_ptr->csp < taken) {
				switch(p_ptr->pclass) {
				case CLASS_MAGE:
				damage = ((taken - p_ptr->csp) / 2) * 2;
				break;
				default:
				damage = ((taken - p_ptr->csp) / 2) * 1;
				break;
				}

				p_ptr->csp = 0;
				/* mana shield stops on empty mana! */
				set_tim_manashield(Ind, 0);
			} else {
				damage = 0;
				p_ptr->csp -= taken;
			}

			/* Display the spellpoints */
			p_ptr->redraw |= (PR_MANA);
		}
	}
#endif

	/* Admin silly stuff */
	if (IS_PLAYER(Ind_attacker) && Players[Ind_attacker]->admin_godly_strike) {
		Players[Ind_attacker]->admin_godly_strike--;
		damage = p_ptr->chp + 1;
	}

	/* Hurt the player */
	p_ptr->chp -= damage;
	p_ptr->deathblow = damage;
	if (!drain) p_ptr->hp_drained = FALSE;

	/* for cloaking as well as shadow running:
	   floor damage (like in nether realm) ain't supposed to break it! - C. Blue */
//both ifs should work properly.
	if (-Ind_attacker != PROJECTOR_TERRAIN && !drain) break_cloaking(Ind, 0);
	//if (strcmp(hit_from, "hazardous environment")) break_cloaking(Ind, 0);

destined_defeat:

	/* Update health bars */
	update_health(0 - Ind);

	/* Display the hitpoints */
	p_ptr->redraw |= (PR_HP);

	/* Figure out of if the player's "number" has changed */
	/* Now displays corresponding to mana amount if disruption shield
	is acitavted (C. Blue) */
/*	if (!p_ptr->tim_manashield)
	{
*/		new_num = (p_ptr->chp * 95) / (p_ptr->mhp * 10);
		if (new_num > TURN_CHAR_INTO_NUMBER) new_num = 10;
/*	}
	else
	{
		new_num = (p_ptr->csp * 95) / (p_ptr->msp*10); 
		if (new_num >= 7) new_num = 10;
	}
*/
	/* If so then refresh everyone's view of this player */
	if (new_num != old_num)
		everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Dead player */
	if (p_ptr->chp < 0) {
		if (IS_PLAYER(Ind_attacker)) {
			if (check_blood_bond(Ind, Ind_attacker)) {
				player_type *p2_ptr = Players[Ind_attacker];
				p_ptr->chp = p_ptr->mhp;
				p2_ptr->chp = p2_ptr->mhp;
#ifdef MARTYR_NO_MANA
				if (!p_ptr->martyr) p_ptr->csp = p_ptr->msp;
				if (!p2_ptr->martyr) p2_ptr->csp = p2_ptr->msp;
#else
				p_ptr->csp = p_ptr->msp;
				p2_ptr->csp = p2_ptr->msp;
#endif
				p_ptr->redraw |= PR_HP | PR_MANA;
				p2_ptr->redraw |= PR_HP | PR_MANA;

				s_printf("BLOOD_BOND: %s won the blood bond against %s\n", p2_ptr->name, p_ptr->name);
				msg_broadcast_format(0, "\374\377c*** %s won the blood bond against %s. ***", p2_ptr->name, p_ptr->name);

				remove_hostility(Ind, p2_ptr->name);
				remove_hostility(Ind_attacker, p_ptr->name);

				remove_blood_bond(Ind, Ind_attacker);
				remove_blood_bond(Ind_attacker, Ind);

				target_set(Ind, 0);
				target_set(Ind_attacker, 0);

				/* Remove stun/KO */
				set_stun(Ind, 0);
				set_stun(Ind_attacker, 0);

				teleport_player(Ind, 4, TRUE);

				return;
			}
		/* get real killer (for log file and scoreboard) in case we're hallucinating */
		} else if (p_ptr->image && IS_MONSTER(Ind_attacker)) {
			/* get real monster name, in case we're hallucinating */
			monster_desc(0, hit_from_real, -Ind_attacker, 0x88);
		}

		/* Note cause of death */
		/* To preserve the players original (pre-ghost) cause
		   of death, use died_from_list.  To preserve the original
		   depth, use died_from_depth. */

		(void)strcpy(p_ptr->died_from, hit_from); /* todo: blindness/no esp (fuzzy like for spell hits) */
		(void)strcpy(p_ptr->really_died_from, hit_from_real);
		if (!p_ptr->ghost) {
			strcpy(p_ptr->died_from_list, hit_from_real);
			p_ptr->died_from_depth = getlevel(&p_ptr->wpos);
			/* Hack to remember total winning */
			if (p_ptr->total_winner) strcat(p_ptr->died_from_list, "\001");
		}

		/* No longer a winner */
		//p_ptr->total_winner = FALSE;

		/* Note death */
		p_ptr->death = TRUE;
		p_ptr->deathblow = damage;

		/* Dead */
		return;
	}

	if (p_ptr->warning_rest != 3 && !p_ptr->warning_rest_cooldown
	    && p_ptr->max_plv >= 3
	    && p_ptr->chp * 10 / p_ptr->mhp <= 5) {
		msg_print(Ind, "\374\377oHINT: Press \377RSHIFT+r\377o to rest, so your hit points will");
		msg_print(Ind, "\374\377o      regenerate faster. (Also true for mana and stamina.)");
		p_ptr->warning_rest++;
		p_ptr->warning_rest_cooldown = 3;//minutes
		acc_set_flags(p_ptr->accountname, ACC_WARN_REST, TRUE);
		s_printf("warning_rest: %s\n", p_ptr->name);
	}
}


/* Decrease player's sanity. This is a copy of the function above. */
void take_sanity_hit(int Ind, int damage, cptr hit_from) {
	player_type *p_ptr = Players[Ind];
#if 0
	int old_csane = p_ptr->csane;
	int warning = (p_ptr->msane * hitpoint_warn / 10);
#endif	// 0

	/* Heavenly invulnerability? */
	if (p_ptr->martyr && !bypass_invuln) return;

	/* Amulet of Immortality */
	if (p_ptr->admin_invuln) return;

	/* Paranoia */
	if (p_ptr->death) return;

	/* Hack -- player is secured inside a store/house except in dungeons */
	if (p_ptr->store_num != -1 && !p_ptr->wpos.wz && !bypass_invuln) return;

	/* For 'Arena Monster Challenge' event: */
	if (safe_area(Ind)) {
		msg_print(Ind, "\377wYou feel disturbed, but the feeling passes.");
		return;
	}

#if 0 //Ind_attacker not available - players cannot cast insanity-draining effects on other players anyway
	/* towns are safe-zones from ALL hostile actions - except blood bond */
	if (IS_PLAYER(Ind_attacker)) {
		if ((istown(&p_ptr->wpos) || istown(&Players[Ind_attacker]->wpos)) &&
		    (!check_blood_bond(Ind_attacker, Ind) ||
		     p_ptr->afk || Players[Ind_attacker]->afk))
			return;
	}
#endif

	if (((p_ptr->alert_afk_dam && p_ptr->afk)
#ifdef ALERT_OFFPANEL_DAM
	    /* new: alert when we're off-panel (cmd_locate) */
	    || (p_ptr->alert_offpanel_dam && (p_ptr->panel_row_old != p_ptr->panel_row || p_ptr->panel_col_old != p_ptr->panel_col))
#endif
	    )
#ifdef USE_SOUND_2010
	    ) {
		Send_warning_beep(Ind);
		//sound(Ind, "warning", "page", SFX_TYPE_MISC, FALSE);
#else
	    && p_ptr->paging == 0) {
		p_ptr->paging = 1;
#endif
	}

#if 0 /* Since 4.6.2 covered by 'alert_offpanel_dam' */
	/* warn if taking (continuous) damage while inside a store! */
	if (p_ptr->store_num != -1) {
 #ifdef USE_SOUND_2010
		Send_warning_beep(Ind);
		//sound(Ind, "warning", "page", SFX_TYPE_MISC, FALSE);
 #else
		if (p_ptr->paging == 0) p_ptr->paging = 1;
 #endif
		//there's already a message given, usually..
		//msg_print(Ind, "\377RWarning - you are taking sanity damage!");
	}
#endif

#ifdef USE_SOUND_2010
	sound(Ind, "insanity", NULL, SFX_TYPE_MISC, FALSE);
#endif

	/* Mega-Hack -- Apply "invulnerability" */
	if (p_ptr->invuln && (!bypass_invuln) && !p_ptr->invuln_applied) {
		/* 1 in 2 chance to fully deflect the damage */
		if (magik(40)) {
			msg_print(Ind, "The attack is fully deflected by a magic shield.");
			return;
		}

		/* Otherwise damage is reduced by the shield */
		damage = (damage + 1) / 2;
	}


	/* Disturb */
	disturb(Ind, 1, 0);

	/* Having WEIRD_MIND or EMPTY_MIND form helps, as well as being mindcrafter */
	if(p_ptr->reduce_insanity == 1) damage = (damage * (9 + rand_int(4))) / 12;
	if(p_ptr->reduce_insanity == 2) damage = (damage * (6 + rand_int(7))) / 12;

	/* Hurt the player */
	p_ptr->csane -= damage;

	/* Display the hitpoints */
	p_ptr->redraw |= (PR_SANITY);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Dead player */
	if (p_ptr->csane < 0) {
		/* Hack -- Note death */
		msg_print(Ind, "\377vYou turn into an unthinking vegetable.");
		/*msg_print(Ind, "\377RYou die.");
		msg_print(Ind, NULL); - already in xtra2.c (C. Blue)*/


		/* Note cause of death */
		/* To preserve the players original (pre-ghost) cause
		   of death, use died_from_list.  To preserve the original
		   depth, use died_from_depth. */

		(void)strcpy(p_ptr->died_from, "insanity");
		(void)strcpy(p_ptr->really_died_from, hit_from);
		if (!p_ptr->ghost) {
			strcpy(p_ptr->died_from_list, "insanity");
			p_ptr->died_from_depth = getlevel(&p_ptr->wpos);
			/* Hack to remember total winning */
			if (p_ptr->total_winner) strcat(p_ptr->died_from_list, "\001");
		}

		/* No longer a winner */
		//p_ptr->total_winner = FALSE;

		/* Note death */
		p_ptr->death = TRUE;
		p_ptr->deathblow = damage;

		/* This way, player dies without becoming a ghost. */
		//ptr->ghost = 1;

		/* Dead */
		break_cloaking(Ind, 0);
		break_shadow_running(Ind);
		stop_precision(Ind);
		stop_shooting_till_kill(Ind);
		return;
	}

	/* Insanity warning (better message needed!) */
	if (p_ptr->csane < p_ptr->msane / 8) {
		/* Message */
		msg_print(Ind, "\377fYou can hardly suppress screaming out insane laughters!");
		msg_print(Ind, NULL);
		break_cloaking(Ind, 0);
		break_shadow_running(Ind);
		stop_precision(Ind);
	} else if (p_ptr->csane < p_ptr->msane / 4) {
		/* Message */
		msg_print(Ind, "\377RYou feel severly disturbed and paranoid..");
		msg_print(Ind, NULL);
		stop_precision(Ind);
	} else if (p_ptr->csane < p_ptr->msane / 2) {
		/* Message */
		msg_print(Ind, "\377rYou feel insanity creep into your mind..");
		msg_print(Ind, NULL);
	}
}

/* Decrease player's exp. This is another copy of the function above.
 * if mode is 'TRUE', it's permanent.
 * if fatal, player dies if runs out of exp.
 *
 * if not permanent nor fatal, use lose_exp instead.
 * - Jir -
 */
void take_xp_hit(int Ind, int damage, cptr hit_from, bool mode, bool fatal, bool disturb) {
	player_type *p_ptr = Players[Ind];

	/* Amulet of Immortality */
	if (p_ptr->admin_invuln) return;

	/* Paranoia */
	if (p_ptr->death) return;

	if (disturb) {
		break_cloaking(Ind, 0);
		stop_precision(Ind);
	}

	if (p_ptr->keep_life) {
		//msg_print(Ind, "You are impervious to life force drain!");
		return;
	}

	/* arena is safe, although this may be doubtful */
	if (safe_area(Ind)) return;

#if 0
	/* Hack -- player is secured inside a store/house except in dungeons */
	if (p_ptr->store_num != -1 && !p_ptr->wpos.wz && !bypass_invuln) return;

	if (((p_ptr->alert_afk_dam && p_ptr->afk)
 #ifdef ALERT_OFFPANEL_DAM
	    /* new: alert when we're off-panel (cmd_locate) */
	    || (p_ptr->alert_offpanel_dam && (p_ptr->panel_row_old != p_ptr->panel_row || p_ptr->panel_col_old != p_ptr->panel_col))
 #endif
	    )
 #ifdef USE_SOUND_2010
	    ) {
		Send_warning_beep(Ind);
		//sound(Ind, "warning", "page", SFX_TYPE_MISC, FALSE);
 #else
	    && p_ptr->paging == 0) {
		p_ptr->paging = 1;
 #endif
	}

	/* warn if taking (continuous) damage while inside a store! */
	if (p_ptr->store_num != -1) {
 #ifdef USE_SOUND_2010
		Send_warning_beep(Ind);
		//sound(Ind, "warning", "page", SFX_TYPE_MISC, FALSE);
 #else
		if (p_ptr->paging == 0) p_ptr->paging = 1;
 #endif
		/* continuous-damage message only */
		if (bypass_invuln) msg_print(Ind, "\377RWarning - your experience is getting drained!");
	}
#endif

	/* Disturb */
	//disturb(Ind, 1, 0);

	/* Hurt the player */
	p_ptr->exp -= damage;
	if (mode && p_ptr->max_exp) {
		p_ptr->max_exp -= damage;
		/* don't reduce max_exp to zero, in case someone insane
		   tries to cheeze events requiring 0 exp this way ;) */
		if (p_ptr->max_exp < 1) p_ptr->max_exp = 1;
	}

	check_experience(Ind);

	/* Dead player */
	if (fatal && p_ptr->exp == 0) {
		/* Hack -- Note death */
		/*msg_print(Ind, "\377RYou die.");
		msg_print(Ind, NULL); - what's so 'hacky' about this?
		It's in xtra2.c anyways, so gone here now. C. Blue */

		/* Note cause of death */
		/* To preserve the players original (pre-ghost) cause
		   of death, use died_from_list.  To preserve the original
		   depth, use died_from_depth. */

		(void)strcpy(p_ptr->died_from, hit_from);
		if (!p_ptr->ghost) {
			strcpy(p_ptr->died_from_list, hit_from);
			p_ptr->died_from_depth = getlevel(&p_ptr->wpos);
			/* Hack to remember total winning */
			if (p_ptr->total_winner) strcat(p_ptr->died_from_list, "\001");
		}

		/* No longer a winner */
		//p_ptr->total_winner = FALSE;

		/* Note death */
		p_ptr->death = TRUE;
		p_ptr->deathblow = 0;

		/* Dead */
		break_cloaking(Ind, 0);
		break_shadow_running(Ind);
		stop_precision(Ind);
		stop_shooting_till_kill(Ind);
		return;
	}
}



/*
 * Note that amulets, rods, and high-level spell books are immune
 * to "inventory damage" of any kind.  Also sling ammo and shovels.
 */


/*
 * Does a given class of objects (usually) hate acid?
 * Note that acid can either melt or corrode something.
 */
static bool hates_acid(object_type *o_ptr) {
	/* Analyze the type */
	switch (o_ptr->tval) {
	/* Wearable items */
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
	case TV_BOOMERANG:
	case TV_SWORD:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_AXE:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:

	case TV_LITE:
		return (TRUE);

	/* Staffs/Scrolls are wood/paper */
	case TV_STAFF:
	case TV_SCROLL:
	case TV_PARCHMENT:
	case TV_BOOK:
		return (TRUE);

	/* Ouch */
	case TV_CHEST:
		return (TRUE);

	/* Junk is useless */
	case TV_SKELETON:
	case TV_JUNK:
		return (TRUE);

	case TV_GAME:
		if (o_ptr->sval == SV_SNOWBALL) return TRUE;
		break;
	}

	return (FALSE);
}

/*
 * Does a given object (usually) hate electricity?
 */
static bool hates_elec(object_type *o_ptr) {
	switch (o_ptr->tval) {
	case TV_RING:
	case TV_WAND:
		return (TRUE);
	}

	return (FALSE);
}

/*
 * Does a given object (usually) hate fire?
 * Blunt/Polearm weapons have wooden shafts.
 * Arrows/Bows are mostly wooden.
 */
bool hates_fire(object_type *o_ptr) {
	/* Analyze the type */
	switch (o_ptr->tval) {
	/* Wearable */
	case TV_GLOVES:
		if (o_ptr->sval == SV_SET_OF_GAUNTLETS) return FALSE;
	case TV_ARROW:
	case TV_BOW:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_AXE:
	case TV_BOOTS:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
		return (TRUE);
	case TV_HELM:
		if (o_ptr->sval == SV_HARD_LEATHER_CAP || o_ptr->sval == SV_CLOTH_CAP) return TRUE;
		return FALSE;
	case TV_BOOMERANG:
		if (o_ptr->sval == SV_BOOM_S_METAL || o_ptr->sval == SV_BOOM_METAL) return(FALSE);
		return (TRUE);

	/* Chests */
	case TV_CHEST:
		if (o_ptr->sval == SV_CHEST_RUINED ||
		    o_ptr->sval == SV_CHEST_SMALL_WOODEN || o_ptr->sval == SV_CHEST_LARGE_WOODEN) return(TRUE);
		return (FALSE);

	/* Staffs/Scrolls burn */
	case TV_STAFF:
	case TV_SCROLL:
	case TV_PARCHMENT:
	case TV_BOOK:
		return (TRUE);

	/* Potions evaporate */
	case TV_POTION:
	case TV_POTION2:
	case TV_FLASK:
	case TV_BOTTLE: //just melts
		return (TRUE);

	/* Junk, partially */
	case TV_SKELETON:
		return TRUE;
	case TV_JUNK:
		if (o_ptr->sval == SV_WOODEN_STICK || o_ptr->sval == SV_WOOD_PIECE) return TRUE;
		break;

	case TV_GAME:
		if (o_ptr->sval == SV_SNOWBALL) return TRUE;
		break;
	}

	return (FALSE);
}

/*
 * Does a given object (usually) hate cold?
 */
static bool hates_cold(object_type *o_ptr) {
	switch (o_ptr->tval) {
	case TV_POTION:
	case TV_POTION2:
	case TV_FLASK:
	//case TV_BOTTLE:  <- empty! unlike potions..
		return (TRUE);
	}

	return (FALSE);
}

/*
 * Does a given object (usually) hate impact?
 */
static bool hates_impact(object_type *o_ptr) {
	switch (o_ptr->tval) {
	case TV_POTION:
	case TV_POTION2:
	case TV_FLASK:
	case TV_BOTTLE:
	case TV_EGG:
		return (TRUE);
	}

	return (FALSE);
}

/*
 * Does a given object (usually) hate water?
 */
bool hates_water(object_type *o_ptr) {
	switch (o_ptr->tval) {
	//case TV_POTION:	/* dilutes */
	//case TV_POTION2:	/* dilutes */
	case TV_SCROLL:		/* fades */
	case TV_PARCHMENT:	/* fades */
	case TV_BOOK:
		return (TRUE);
	case TV_GAME:
		if (o_ptr->sval == SV_SNOWBALL) return TRUE;
		break;
	}

	return (FALSE);
}

/*
 * Does a given object rust from water? (for equip_damage()) - C. Blue
 */
static bool can_rust(object_type *o_ptr) {
	switch (o_ptr->tval) {
#if 0
	case TV_GLOVES: if (o_ptr->sval == SV_LEATHER_GLOVES || o_ptr->sval == SV_SET_OF_ELVEN_GLOVES) return FALSE;
	case TV_BOOMERANG: if (o_ptr->sval == SV_BOOM_WOOD || o_ptr->sval == SV_BOOM_S_WOOD) return(FALSE); else return(TRUE);
	case TV_CROWN: if (o_ptr->sval == SV_IRON_CROWN) return(TRUE); else return(FALSE);
	case TV_SHIELD: if (o_ptr->sval == SV_SMALL_LEATHER_SHIELD || o_ptr->sval == SV_LARGE_LEATHER_SHIELD) return(FALSE); else return(TRUE);
	case TV_HARD_ARMOR: if (o_ptr->sval == SV_MITHRIL_CHAIN_MAIL || o_ptr->sval == SV_MITHRIL_PLATE_MAIL || o_ptr->sval == SV_ADAMANTITE_PLATE_MAIL) return (FALSE); else return(TRUE);
	case TV_HELM: if (o_ptr->sval == SV_HARD_LEATHER_CAP) return(FALSE); else return(TRUE);
#else
	case TV_GLOVES:
	case TV_BOOMERANG:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_HARD_ARMOR:
	case TV_HELM:
#endif
	case TV_SWORD:
	// :)	case TV_BLUNT:
	case TV_POLEARM:
	case TV_AXE:
		return (TRUE);
	}

	return (FALSE);
}



/*
 * Melt something
 */
static int set_acid_destroy(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	if (!hates_acid(o_ptr)) return (FALSE);
	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
	if (f3 & TR3_IGNORE_ACID) return (FALSE);
	return (TRUE);
}

/*
 * Electrical damage
 */
static int set_elec_destroy(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	if (!hates_elec(o_ptr)) return (FALSE);
	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
	if (f3 & TR3_IGNORE_ELEC) return (FALSE);
	return (TRUE);
}

/*
 * Burn something
 */
static int set_fire_destroy(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	if (!hates_fire(o_ptr)) return (FALSE);
	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
	if (f3 & TR3_IGNORE_FIRE) return (FALSE);
	return (TRUE);
}

/*
 * Freeze things
 */
int set_cold_destroy(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	if (!hates_cold(o_ptr)) return (FALSE);
	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
	if (f3 & TR3_IGNORE_COLD) return (FALSE);
	return (TRUE);
}

/*
 * Crash things
 */
int set_impact_destroy(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	if (!hates_impact(o_ptr)) return (FALSE);
	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
	/* Hack -- borrow flag */
//	if (f3 & TR3_IGNORE_COLD) return (FALSE);
	return (TRUE);
}

/*
 * Soak something
 */
int set_water_destroy(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	if (!hates_water(o_ptr)) return (FALSE);
	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
	if ((f5 & TR5_IGNORE_WATER) || (f3 & TR3_IGNORE_ACID)) return (FALSE);
	return (TRUE);
}

/*
 * Rust
 */
static int set_rust_destroy(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	if (!can_rust(o_ptr)) return (FALSE);
	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
	if ((f3 & TR3_IGNORE_ACID) || (f5 & TR5_IGNORE_WATER)) return (FALSE);
	return (TRUE);
}

/*
 * Burn/Crash things
 */
/*
 * Does a given object (usually) hate GF_ROCKET damage,
 * ie hates_fire() or hates_impact()?
 * (Note: Add shards too in case hates_shards() is ever added to the game)
 */
int set_rocket_destroy(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	if (!hates_impact(o_ptr)) {
		if (!hates_fire(o_ptr)) return (FALSE);
		/* Extract the flags */
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
		if (f3 & TR3_IGNORE_FIRE) return (FALSE);
	}
	return (TRUE);
}

/*
 * Every things
 */
int set_all_destroy(object_type *o_ptr) {
	if (artifact_p(o_ptr)) return (FALSE);
//	if (is_realm_book(o_ptr) && o_ptr->sval >= SV_BOOK_MIN_GOOD) return (FALSE);
	if (is_realm_book(o_ptr)) {
		u32b f1, f2, f3, f4, f5, f6, esp;
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
		/* Hack^2 -- use this as a sign of being 'high books' */
		if (f3 & TR3_IGNORE_ELEC) return (FALSE);
	}
	return (TRUE);
}



/*
 * This seems like a pretty standard "typedef"
 */
//typedef int (*inven_func)(object_type *);

/*
 * Destroys a type of item on a given percent chance
 * Note that missiles are no longer necessarily all destroyed
 * Destruction taken from "melee.c" code for "stealing".
 * Returns number of items destroyed.
 */
int inven_damage(int Ind, inven_func typ, int perc) {
	player_type	*p_ptr = Players[Ind];
	int		i, j, k, amt;
	object_type	*o_ptr;
	char		o_name[ONAME_LEN];

	if (safe_area(Ind)) return(FALSE);
	/* secret dungeon master is unaffected (potion shatter effects might be bad) */
	if (p_ptr->admin_dm) return(FALSE);

	/* Count the casualties */
	k = 0;

	/* Scan through the slots backwards */
	for (i = 0; i < INVEN_TOTAL; i++) {
		/* Hack -- equipments are harder to be harmed */
		if (i >= INVEN_PACK && i != INVEN_AMMO && !magik(HARM_EQUIP_CHANCE))
			continue;

		/* Get the item in that slot */
		o_ptr = &p_ptr->inventory[i];

		/* Hack -- for now, skip artifacts */
		if (artifact_p(o_ptr)) continue;

		/* Give this item slot a shot at death */
		if ((*typ)(o_ptr)) {
			/* Count the casualties */
			for (amt = j = 0; j < o_ptr->number; ++j) {
				if (rand_int(100) < perc) amt++;
			}

			/* Some casualities */
			if (amt) {
				/* Get a description */
				object_desc(Ind, o_name, o_ptr, FALSE, 3);

				/* Message */
				msg_format(Ind, "\376\377o%sour %s (%c) %s destroyed!",
						   ((o_ptr->number > 1) ?
							((amt == o_ptr->number) ? "All of y" :
							 (amt > 1 ? "Some of y" : "One of y")) : "Y"),
						   o_name, index_to_label(i),
						   ((amt > 1) ? "were" : "was"));

				/* Potions smash open */
				if (k_info[o_ptr->k_idx].tval == TV_POTION &&
				    typ != set_water_destroy)	/* MEGAHACK */
				    //&& typ != set_cold_destroy)
				{
					//(void)potion_smash_effect(0, &p_ptr->wpos, p_ptr->py, p_ptr->px, o_ptr->sval);
					bypass_invuln = TRUE;
					(void)potion_smash_effect(PROJECTOR_POTION, &p_ptr->wpos, p_ptr->py, p_ptr->px, o_ptr->sval);
					bypass_invuln = FALSE;
				}

				/* Destroy "amt" items */
				if (is_magic_device(o_ptr->tval)) divide_charged_item(NULL, o_ptr, amt);
				inven_item_increase(Ind, i, -amt);
				inven_item_optimize(Ind, i);

				/* Count the casualties */
				k += amt;
			}
		}
	}

	/* Return the casualty count */
	return (k);
}




/*
 * Acid, water or fire has hit the player, attempt to affect some armor. - C. Blue
 * Note that the "base armor" of an object never changes.
 * If any armor is damaged (or resists), the player takes less damage. <- not implemented atm
 */
int equip_damage(int Ind, int typ) {
	player_type	*p_ptr = Players[Ind];
	object_type	*o_ptr = NULL;
	char		o_name[ONAME_LEN];
	int		shield_bonus = 0;
	u32b dummy, f2, f5;

	if (safe_area(Ind)) return(FALSE);
	if (p_ptr->admin_dm) return(FALSE);

	/* Pick a (possibly empty) inventory slot */
#if !defined(NEW_SHIELDS_NO_AC) || !defined(USE_NEW_SHIELDS)
//	switch (rand_int(is_melee_weapon(p_ptr->inventory[INVEN_ARM].tval) ? 5 : 6)) { /* in case of DUAL_WIELD */
	switch (rand_int(p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD ? 6 : 5)) { /* in case of DUAL_WIELD */
#else
	switch (rand_int(5)) {
#endif
	case 0: o_ptr = &p_ptr->inventory[INVEN_BODY]; break;
	case 1: o_ptr = &p_ptr->inventory[INVEN_OUTER]; break;
	case 2: o_ptr = &p_ptr->inventory[INVEN_HANDS]; break;
	case 3: o_ptr = &p_ptr->inventory[INVEN_HEAD]; break;
	case 4: o_ptr = &p_ptr->inventory[INVEN_FEET]; break;
	case 5: o_ptr = &p_ptr->inventory[INVEN_ARM];
#ifdef USE_NEW_SHIELDS
		shield_bonus = o_ptr->to_a;
#endif
		break;
	}

	/* Nothing to damage */
	if (!o_ptr->k_idx) return (FALSE);

	switch (typ) {
	case GF_WATER:
		if (!set_rust_destroy(o_ptr)) return(FALSE); else break; /* for some equipped items set_rust_destroy and set_water_destroy may be different */
	case GF_ACID:
		if (!set_acid_destroy(o_ptr)) return(FALSE); else break; 
	case GF_FIRE:
		if (!set_fire_destroy(o_ptr)) return(FALSE); else break;
	default: return(FALSE);
	}

	/* hack: not disenchantable -> cannot be damaged either */
	object_flags(o_ptr, &dummy, &f2, &dummy, &dummy, &f5, &dummy, &dummy);
	if ((f2 & TR2_RES_DISEN) || (f5 & TR5_IGNORE_DISEN)) return FALSE;

	/* No damage left to be done */
	if (o_ptr->ac + o_ptr->to_a + shield_bonus <= 0) return (FALSE);

	/* Describe */
	object_desc(Ind, o_name, o_ptr, FALSE, 0);

	/* Message */
	msg_format(Ind, "\376\377oYour %s is damaged!", o_name);

	/* Damage the item */
	o_ptr->to_a--;

	/* Calculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Item was damaged */
	return (TRUE);
}

/*
 * Acid or water has hit the player but he blocked it. Damage the shield! - C. Blue
 * Note that the "base armor" of an object never changes.
 */
#ifndef NEW_SHIELDS_NO_AC
int shield_takes_damage(int Ind, int typ) {
	player_type	*p_ptr = Players[Ind];
	object_type	*o_ptr = &p_ptr->inventory[INVEN_ARM];
	char		o_name[ONAME_LEN];
	u32b dummy, f2, f5;

	if (safe_area(Ind)) return(FALSE);
	if (p_ptr->admin_dm) return(FALSE);

	/* Nothing to damage */
	if (!o_ptr->k_idx) return (FALSE);

	switch (typ) {
	case GF_WATER:
		if (p_ptr->immune_water || !set_rust_destroy(o_ptr))
			return(FALSE);
		else break; /* for some equipped items set_rust_destroy and set_water_destroy may be different */
	case GF_ACID:
		if (p_ptr->immune_acid || !set_acid_destroy(o_ptr))
			return(FALSE);
		else break;
	case GF_FIRE:
		if (p_ptr->immune_fire || !set_fire_destroy(o_ptr))
			return(FALSE);
		else break;
	default: return(FALSE);
	}

	/* hack: not disenchantable -> cannot be damaged either */
	object_flags(o_ptr, &dummy, &f2, &dummy, &dummy, &f5, &dummy, &dummy);
	if ((f2 & TR2_RES_DISEN) || (f5 & TR5_IGNORE_DISEN)) return FALSE;

	/* No damage left to be done */
 #ifdef USE_NEW_SHIELDS
	if (o_ptr->ac + o_ptr->to_a * 2 <= 0) return (FALSE);
 #else
	if (o_ptr->ac + o_ptr->to_a <= 0) return (FALSE);
 #endif

	/* Describe */
	object_desc(Ind, o_name, o_ptr, FALSE, 0);

	/* Message */
	msg_format(Ind, "\376\377oYour %s is damaged!", o_name);

	/* Damage the item */
	o_ptr->to_a--;

	/* Calculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Item was damaged */
	return (TRUE);
}
#endif

int weapon_takes_damage(int Ind, int typ, int slot) {
	player_type	*p_ptr = Players[Ind];
	object_type	*o_ptr = &p_ptr->inventory[slot];
	char		o_name[ONAME_LEN];
	u32b dummy, f2, f5;

	if (safe_area(Ind)) return(FALSE);
	if (p_ptr->admin_dm) return(FALSE);

	/* Nothing to damage */
	if (!o_ptr->k_idx) return (FALSE);

	switch (typ) {
	case GF_WATER:
		/* hack -- decrease the variety of damaging attacks a bit, mercifully */
		if (o_ptr->tval == TV_BLUNT || o_ptr->tval == TV_POLEARM) return(FALSE);

		if (p_ptr->immune_water || !set_rust_destroy(o_ptr))
			return(FALSE);
		break; /* for some equipped items set_rust_destroy and set_water_destroy may be different */
	case GF_ACID:
		/* hack -- decrease the variety of damaging attacks a bit, mercifully */
		if (o_ptr->tval == TV_BLUNT || o_ptr->tval == TV_POLEARM) return(FALSE);

		if (p_ptr->immune_acid || !set_acid_destroy(o_ptr))
			return(FALSE);
		break;
	case GF_FIRE:
		/* hack -- decrease the variety of damaging attacks a bit, mercifully */
		if (o_ptr->tval == TV_SWORD || o_ptr->tval == TV_AXE) return(FALSE);

		if (p_ptr->immune_fire || !set_fire_destroy(o_ptr))
			return(FALSE);
		break;
	default: return(FALSE);
	}

	/* hack: not disenchantable -> cannot be damaged either */
	object_flags(o_ptr, &dummy, &f2, &dummy, &dummy, &f5, &dummy, &dummy);
	if ((f2 & TR2_RES_DISEN) || (f5 & TR5_IGNORE_DISEN)) return FALSE;

	/* No damage left to be done */
	if (o_ptr->to_d <= -10) return (FALSE);

	/* Describe */
	object_desc(Ind, o_name, o_ptr, FALSE, 0);

	/* Message */
	msg_format(Ind, "\376\377oYour %s is damaged!", o_name);

	/* Damage the item */
	o_ptr->to_d--;

	/* Calculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Item was damaged */
	return (TRUE);
}


/*
 * Hurt the player with Acid
 */
int acid_dam(int Ind, int dam, cptr kb_str, int Ind_attacker) {
	player_type *p_ptr = Players[Ind];
	int inv, hurt_eq;

	dam -= (p_ptr->reduc_acid * dam) / 100;
	inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* this is SO much softer than MAngband, heh. I think it's good tho - C. Blue */
	hurt_eq = (dam < 30) ? 15 : (dam < 60) ? 33 : 100;
	if (p_ptr->resist_acid && p_ptr->oppose_acid) hurt_eq = (hurt_eq + 2) / 3;
	else if (p_ptr->resist_acid || p_ptr->oppose_acid) hurt_eq = (hurt_eq + 1) / 2;

	/* Total Immunity */
	if (p_ptr->immune_acid || (dam <= 0)) return(0);

	/* super leeway new in 2016 - cannot penetrate backpack: */
	if (dam < 10) inv = 0;

	/* Resist the damage */
	if (p_ptr->resist_acid) dam = (dam + 2) / 3;
	if (p_ptr->oppose_acid) dam = (dam + 2) / 3;
	if (p_ptr->suscep_acid && !(p_ptr->resist_acid || p_ptr->oppose_acid)) dam = dam * 2;

	/* Don't kill inventory in bloodbond... */
	int breakable = 1;
	if (IS_PLAYER(Ind_attacker)) {
		if (check_blood_bond(Ind, Ind_attacker)) {
			breakable = 0;
		}
	}

	if ((!(p_ptr->oppose_acid || p_ptr->resist_acid)) &&
		randint(HURT_CHANCE) == 1 && breakable)
		(void) do_dec_stat(Ind, A_CHR, DAM_STAT_TYPE(inv));

	/* If any armor gets hit, defend the player */
	/* let's tone it down a bit; since immunity completely protects all eq,
	   let's have resistance work similar - C. Blue */
	//if (magik(33) && equip_damage(Ind, GF_ACID)) dam = (dam + 1) / 2;
	//if (magik(50) && equip_damage(Ind, GF_ACID)) dam = (dam + 1) / 2;
	//if (equip_damage(Ind, GF_ACID)) dam = (dam + 1) / 2;
	if (magik(hurt_eq) && breakable) equip_damage(Ind, GF_ACID);

	/* Take damage */
	//take_hit(Ind, dam, kb_str, Ind_attacker);

	/* Inventory damage */
	if (!(p_ptr->oppose_acid && p_ptr->resist_acid) && breakable)
		inven_damage(Ind, set_acid_destroy, inv);

	return(dam);
}

/*
 * Hurt the player with electricity
 */
int elec_dam(int Ind, int dam, cptr kb_str, int Ind_attacker) {
	player_type *p_ptr = Players[Ind];
	int inv;

	dam -= (p_ptr->reduc_elec * dam) / 100;
	inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total immunity */
	if (p_ptr->immune_elec || (dam <= 0)) return(0);

	/* super leeway new in 2016 - cannot penetrate backpack: */
	if (dam < 10) inv = 0;

	/* Resist the damage */
	if (p_ptr->oppose_elec) dam = (dam + 2) / 3;
	if (p_ptr->resist_elec) dam = (dam + 2) / 3;
	if (p_ptr->suscep_elec && !(p_ptr->resist_elec || p_ptr->oppose_elec)) dam = dam * 2;

	/* Don't kill inventory in bloodbond... */
	int breakable = 1;
	if (IS_PLAYER(Ind_attacker)) {
		if (check_blood_bond(Ind, Ind_attacker)) {
			breakable = 0;
		}
	}

	if ((!(p_ptr->oppose_elec || p_ptr->resist_elec)) &&
		randint(HURT_CHANCE) == 1 && breakable)
		(void) do_dec_stat(Ind, A_DEX, DAM_STAT_TYPE(inv));

	/* Take damage */
	//take_hit(Ind, dam, kb_str, Ind_attacker);

	/* Inventory damage */
	if (!(p_ptr->oppose_elec && p_ptr->resist_elec) && breakable)
		inven_damage(Ind, set_elec_destroy, inv);

	return(dam);
}

/*
 * Hurt the player with Fire
 */
int fire_dam(int Ind, int dam, cptr kb_str, int Ind_attacker) {
	player_type *p_ptr = Players[Ind];
	int inv, hurt_eq;

	dam -= (p_ptr->reduc_fire * dam) / 100;
	inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	hurt_eq = (dam < 30) ? 15 : (dam < 60) ? 33 : 100;
	if (p_ptr->resist_fire && p_ptr->oppose_fire) hurt_eq = (hurt_eq + 2) / 3;
	else if (p_ptr->resist_fire || p_ptr->oppose_fire) hurt_eq = (hurt_eq + 1) / 2;

	/* Totally immune */
	if (p_ptr->immune_fire || (dam <= 0)) return(0);

	/* super leeway new in 2016 - cannot penetrate backpack: */
	if (dam < 10) inv = 0;

	/* Resist the damage */
	if (p_ptr->resist_fire) dam = (dam + 2) / 3;
	if (p_ptr->oppose_fire) dam = (dam + 2) / 3;
	if (p_ptr->suscep_fire && !(p_ptr->resist_fire || p_ptr->oppose_fire)) dam = dam * 2;

	/* Don't kill inventory in bloodbond... */
	int breakable = 1;
	if (IS_PLAYER(Ind_attacker)) {
		if (check_blood_bond(Ind, Ind_attacker)) {
			breakable = 0;
		}
	}

	if ((!(p_ptr->oppose_fire || p_ptr->resist_fire)) &&
		randint(HURT_CHANCE) == 1 && breakable)
		(void) do_dec_stat(Ind, A_STR, DAM_STAT_TYPE(inv));

	if (magik(hurt_eq) && breakable) {
		/* This check is currently only needed for fire damage (lava) as there are no other terrain tiles to inflict item damage (just nether) */
		if (-Ind_attacker != PROJECTOR_TERRAIN) equip_damage(Ind, GF_FIRE);
		else {
			/* only damage boots, as the damage is coming from the floor - maybe sometimes cloak too.
			   (from lava, we also do the same for fires though, too complicated to distinguish those here..) */
			object_type *o_ptr = &p_ptr->inventory[rand_int(4) ? INVEN_FEET : INVEN_OUTER];
			if (o_ptr->k_idx && set_fire_destroy(o_ptr)) {
				u32b dummy, f2, f5;

				object_flags(o_ptr, &dummy, &f2, &dummy, &dummy, &f5, &dummy, &dummy);
				if (!((f2 & TR2_RES_DISEN) || (f5 & TR5_IGNORE_DISEN)) &&
				    /* No damage left to be done? */
				    o_ptr->ac + o_ptr->to_a > 0) {
					char o_name[ONAME_LEN];

					object_desc(Ind, o_name, o_ptr, FALSE, 0);
					msg_format(Ind, "\376\377oYour %s is damaged!", o_name);

					/* Damage the item */
					o_ptr->to_a--;

					p_ptr->update |= (PU_BONUS);
					p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
				}
			}
		}
	}

	/* Take damage */
	//take_hit(Ind, dam, kb_str, Ind_attacker);

	/* Inventory damage */
	if (!(p_ptr->resist_fire && p_ptr->oppose_fire) && breakable)
		inven_damage(Ind, set_fire_destroy, inv);

	return(dam);
}

/*
 * Hurt the player with Cold
 */
int cold_dam(int Ind, int dam, cptr kb_str, int Ind_attacker) {
	player_type *p_ptr = Players[Ind];
	int inv;

	dam -= (p_ptr->reduc_cold * dam) / 100;
	inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total immunity */
	if (p_ptr->immune_cold || (dam <= 0)) return(0);

	/* super leeway new in 2016 - cannot penetrate backpack: */
	if (dam < 10) inv = 0;

	/* Resist the damage */
	if (p_ptr->resist_cold) dam = (dam + 2) / 3;
	if (p_ptr->oppose_cold) dam = (dam + 2) / 3;
	if (p_ptr->suscep_cold && !(p_ptr->resist_cold || p_ptr->oppose_cold)) dam = dam * 2;

	/* Don't kill inventory in bloodbond... */
	int breakable = 1;
	if (IS_PLAYER(Ind_attacker)) {
		if (check_blood_bond(Ind, Ind_attacker)) {
			breakable = 0;
		}
	}

	if ((!(p_ptr->oppose_cold || p_ptr->resist_cold)) &&
		randint(HURT_CHANCE) == 1 && breakable)
		(void) do_dec_stat(Ind, A_STR, DAM_STAT_TYPE(inv));

	/* Take damage */
//	take_hit(Ind, dam, kb_str, Ind_attacker);

	/* Inventory damage */
	if (!(p_ptr->resist_cold && p_ptr->oppose_cold) && breakable)
		inven_damage(Ind, set_cold_destroy, inv);

	return(dam);
}





/*
 * Increases a stat by one randomized level		-RAK-
 *
 * Note that this function (used by stat potions) now restores
 * the stat BEFORE increasing it.
 */
/* Fix visual glitch of stats maxing out at <= 18 not switching to TERM_L_UMBER;
   also it was a bit annoying that non-fractionally displayed stats (ie stats <= 18)
   weren't advancing visibly (ie by a full point) when quaffing stat potions, when
   the stat in question was internally (stat_cur) already at 18+;
   possible downside: This buffs stat gain tremendously for low-stat races (hi Yeeks),
   since stats <= 18 will always increase by a full point, even though they'd usually
   only increase by a fraction of a point if the stat_cur was really already above 18. */
#define FIX_STATLOWMAX
bool inc_stat(int Ind, int stat) {
	player_type *p_ptr = Players[Ind];
	int value, gain;

	/* Then augment the current/max stat */
	value = p_ptr->stat_cur[stat];

	/* Cannot go above 18/100 */
	if (value < 18 + 100) {
		/* Gain one (sometimes two) points */
#ifndef FIX_STATLOWMAX
		if (value < 18) {
#else
		if (p_ptr->stat_top[stat] < 18) {
			if (value >= 18) gain = 10;
			else
#endif
			gain = ((rand_int(100) < 75) ? 1 : 2);
			value += gain;
#ifdef FIX_STATLOWMAX
			//paranoia - only needed for old characters who might've had their stats partially increased already:
			if (value > 18) value = 18 + ((value - 18) / 10) * 10;
#endif
		}
		/* Gain 1/6 to 1/3 of distance to 18/100 */
		else if (value < 18+98) {
			/* Approximate gain value */
			gain = (((18+100) - value) / 2 + 3) / 2;

			/* Paranoia */
			if (gain < 1) gain = 1;

			/* 4 points at once is too much */
			if (gain > 17) gain = 17;

			/* Apply the bonus */
			value += randint(gain) + gain / 2;

			/* Maximal value */
			if (value > 18+99) value = 18 + 99;
		}

		/* Gain one point at a time */
		else value++;

		/* Save the new value */
		p_ptr->stat_cur[stat] = value;

		/* Bring up the maximum too */
		if (value > p_ptr->stat_max[stat])
			p_ptr->stat_max[stat] = value;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Success */
		return (TRUE);
	}

	/* Nothing to gain */
	return (FALSE);
}



/*
 * Decreases a stat by an amount indended to vary from 0 to 100 percent.
 *
 * Amount could be a little higher in extreme cases to mangle very high
 * stats from massive assaults.  -CWS
 *
 * Note that "permanent" means that the *given* amount is permanent,
 * not that the new value becomes permanent.  This may not work exactly
 * as expected, due to "weirdness" in the algorithm, but in general,
 * if your stat is already drained, the "max" value will not drop all
 * the way down to the "cur" value.
 */
bool dec_stat(int Ind, int stat, int amount, int mode) {
	player_type *p_ptr = Players[Ind];
	int cur, max, loss = 0, same, res = FALSE;

	if (safe_area(Ind)) return(FALSE);

	/* Acquire current value */
	cur = p_ptr->stat_cur[stat];
	max = p_ptr->stat_max[stat];

	/* Note when the values are identical */
	same = (cur == max);

	/* Damage "current" value */
	if (cur > 3) {
		/* Handle "low" values */
		if (cur <= 18) {
			if (amount > 90) loss++;
			if (amount > 50) loss++;
			if (amount > 20) loss++;
			loss++;
			cur -= loss;
		}
		/* Handle "high" values */
		else {
			/* Hack -- Decrement by a random amount between one-quarter */
			/* and one-half of the stat bonus times the percentage, with a */
			/* minimum damage of half the percentage. -CWS */
			loss = (((cur-18) / 2 + 1) / 2 + 1);

			/* Paranoia */
			if (loss < 1) loss = 1;

			/* Randomize the loss */
			loss = ((randint(loss) + loss) * amount) / 100;

			/* Maximal loss */
			if (loss < amount/2) loss = amount/2;

			/* Lose some points */
			cur = cur - loss;

			/* Hack -- Only reduce stat to 17 sometimes */
			if (cur < 18) cur = (amount <= 20) ? 18 : 17;
		}

		/* Prevent illegal values */
		if (cur < 3) cur = 3;

		/* Something happened */
		if (cur != p_ptr->stat_cur[stat]) res = TRUE;
	}

	/* Damage "max" value */
	if ((mode == STAT_DEC_PERMANENT) && (max > 3)) {
		/* Handle "low" values */
		if (max <= 18) {
			if (amount > 90) max--;
			if (amount > 50) max--;
			if (amount > 20) max--;
			max--;
		}
		/* Handle "high" values */
		else {
			/* Hack -- Decrement by a random amount between one-quarter */
			/* and one-half of the stat bonus times the percentage, with a */
			/* minimum damage of half the percentage. -CWS */
			loss = (((max-18) / 2 + 1) / 2 + 1);
			loss = ((randint(loss) + loss) * amount) / 100;
			if (loss < amount/2) loss = amount/2;

			/* Lose some points */
			max = max - loss;

			/* Hack -- Only reduce stat to 17 sometimes */
			if (max < 18) max = (amount <= 20) ? 18 : 17;
		}

		/* Hack -- keep it clean */
		if (same || (max < cur)) max = cur;

		/* Something happened */
		if (max != p_ptr->stat_max[stat]) res = TRUE;
	}

	/* Apply changes */
	if (res) {
		/* Actually set the stat to its new value. */
		p_ptr->stat_cur[stat] = cur;
		p_ptr->stat_max[stat] = max;

		if (mode == STAT_DEC_TEMPORARY) {
			u16b dectime;

			/* a little crude, perhaps */
			dectime = rand_int(getlevel(&p_ptr->wpos)*50) + 50;

			/* prevent overflow, stat_cnt = u16b */
			/* or add another temporary drain... */
			if ( ((p_ptr->stat_cnt[stat]+dectime)<p_ptr->stat_cnt[stat]) ||
				(p_ptr->stat_los[stat]>0) )

			{
				p_ptr->stat_cnt[stat] += dectime;
				p_ptr->stat_los[stat] += loss;
			}
			else
			{
				p_ptr->stat_cnt[stat] = dectime;
				p_ptr->stat_los[stat] = loss;
			}
		}

		/* Recalculate bonuses */
//WIS drain -> Sanity changes;	p_ptr->update |= (PU_BONUS); */
		p_ptr->update |= (PU_BONUS | PU_MANA | PU_HP | PU_SANITY);
	}

	/* Done */
	return (res);
}


/*
 * Restore a stat.  Return TRUE only if this actually makes a difference.
 */
bool res_stat(int Ind, int stat) {
	player_type *p_ptr = Players[Ind];

	/* temporary drain is gone */
	p_ptr->stat_los[stat] = 0;
	p_ptr->stat_cnt[stat] = 0;

	/* Restore if needed */
	if (p_ptr->stat_cur[stat] != p_ptr->stat_max[stat]) {
		/* Restore */
		p_ptr->stat_cur[stat] = p_ptr->stat_max[stat];

		/* Recalculate bonuses */
//		p_ptr->update |= (PU_BONUS);
		p_ptr->update |= (PU_BONUS | PU_MANA | PU_HP | PU_SANITY);

		/* Success */
		return (TRUE);
	}

	/* Nothing to restore */
	return (FALSE);
}




/*
 * Apply disenchantment to the player's stuff
 *
 * XXX XXX XXX This function is also called from the "melee" code
 *
 *
 * If "mode is set to 0 then a random slot will be used, if not the "mode"
 * slot will be used.
 *
 * Return "TRUE" if the player notices anything
 */
bool apply_disenchant(int Ind, int mode) {
	player_type *p_ptr = Players[Ind];
	int t = mode;
	object_type *o_ptr;
	char o_name[ONAME_LEN];
	u32b f1, f2, f3, f4, f5, f6, esp;

	if (safe_area(Ind)) return(FALSE);

	/* Unused */
	//mode = mode;

	if(!mode) mode = randint(9);
	else if(mode < 1 || mode > 9) return (FALSE);

	/* Pick a random slot */
	switch (mode) {
	case 1: t = INVEN_WIELD; break;
	case 2: t = INVEN_BOW; break;
	case 3: t = INVEN_BODY; break;
	case 4: t = INVEN_OUTER; break;
	case 5: t = INVEN_ARM; break;
	case 6: t = INVEN_HEAD; break;
	case 7: t = INVEN_HANDS; break;
	case 8: t = INVEN_FEET; break;
	case 9: t = INVEN_AMMO; break;
	}

	/* Get the item */
	o_ptr = &p_ptr->inventory[t];

	/* No item, nothing happens */
	if (!o_ptr->k_idx) return (FALSE);

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* Describe the object */
	object_desc(Ind, o_name, o_ptr, FALSE, 0);

	/* Special treatment for new ego-type: Ethereal (ammunition):
	   Ethereal items don't get disenchanted, but rather disappear completely! */
	if (o_ptr->name2 == EGO_ETHEREAL || o_ptr->name2b == EGO_ETHEREAL) {
		int i = randint(o_ptr->number / 5);
		if (!i) i = 1;
		msg_format(Ind, "\376\377o%s of your %s (%c) %s away!",
		    (o_ptr->number == i ? "All" : (i != 1 ? "Some" : "One")), o_name, index_to_label(t),
		    ((i != 1) ? "fade" : "fades"));
		inven_item_increase(Ind, t, -i);
		inven_item_optimize(Ind, t);
		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);
		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
		/* Notice */
		return (TRUE);
	}

	/* Nothing to disenchant */
	if ((o_ptr->to_h <= 0) && (o_ptr->to_d <= 0) && (o_ptr->to_a <= 0)) {
		/* Nothing to notice */
		return (FALSE);
	}

	if ((f2 & TR2_RES_DISEN) || (f5 & TR5_IGNORE_DISEN)) {
		msg_format(Ind, "Your %s (%c) %s unaffected!",
				   o_name, index_to_label(t),
				   ((o_ptr->number != 1) ? "are" : "is"));
		/* Notice */
		return (TRUE);
	}

	/* Artifacts have 70%(randart) or 80%(trueart) chance to resist */
	if ((artifact_p(o_ptr) && (rand_int(100) < 70)) ||
	    (true_artifact_p(o_ptr) && (rand_int(100) < 80))) {
		/* Message */
		msg_format(Ind, "Your %s (%c) resist%s!",
				   o_name, index_to_label(t),
				   ((o_ptr->number != 1) ? "" : "s"));

		/* Notice */
		return (TRUE);
	}

	/* Disenchant tohit */
	if (o_ptr->to_h > 0) {
		if (o_ptr->to_h > 0) o_ptr->to_h--;
		if ((o_ptr->to_h > 7) && (rand_int(100) < 33)) o_ptr->to_h--;
		if ((o_ptr->to_h > 15) && (rand_int(100) < 25)) o_ptr->to_h--;
	}
	/* Disenchant todam */
	if (o_ptr->to_d > 0) {
		if (o_ptr->to_d > 0) o_ptr->to_d--;
		if ((o_ptr->to_d > 7) && (rand_int(100) < 33)) o_ptr->to_d--;
		if ((o_ptr->to_d > 15) && (rand_int(100) < 25)) o_ptr->to_d--;
	}
	/* Disenchant toac */
	if (o_ptr->to_a > 0) {
		if (o_ptr->to_a > 0) o_ptr->to_a--;
		if ((o_ptr->to_a > 7) && (rand_int(100) < 33)) o_ptr->to_a--;
		if ((o_ptr->to_a > 15) && (rand_int(100) < 25)) o_ptr->to_a--;
	}
	/* Message */
	msg_format(Ind, "\376\377oYour %s (%c) %s disenchanted!",
			   o_name, index_to_label(t),
			   ((o_ptr->number != 1) ? "were" : "was"));

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* ? */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Notice */
	return (TRUE);
}


/*
 * Apply 'discharging' ie electricity damage to the player's stuff - C. Blue
 * the reason for adding this is, to bring electricity on par with other base elements.
 *
 * Return "TRUE" if the player notices anything
 */
bool apply_discharge(int Ind, int dam) {
	player_type *p_ptr = Players[Ind];

	int	i, chance = 95;
	bool	damaged_any = FALSE, damaged;
	object_type *o_ptr;
	char	o_name[ONAME_LEN];
	u32b fx, f2, f3;

	if (safe_area(Ind)) return(FALSE);

	if ((p_ptr->oppose_elec && p_ptr->resist_elec) ||
	    p_ptr->immune_elec) return(FALSE);

	/* note: used same values as in elec_dam */
	if (dam >= 30) chance -= 10;
	if (dam >= 60) chance -= 10;

	/* Scan through the slots backwards */
//	for (i = 0; i < INVEN_PACK; i++)
	for (i = 0; i < INVEN_TOTAL; i++) { /* Let's see what will happen.. */
		/* Hack -- equipments are harder to be harmed */
		if (i >= INVEN_PACK && i != INVEN_AMMO && !magik(HARM_EQUIP_CHANCE))
			continue;

		/* Get the item */
		o_ptr = &p_ptr->inventory[i];
		/* No item, nothing happens */
		if (!o_ptr->k_idx) return (FALSE);

		if (magik(chance)) continue;
		//if (o_ptr->tval == TV_AMULET && magik(50)) continue; /* further reduce chance? */

		object_flags(o_ptr, &fx, &f2, &f3, &fx, &fx, &fx, &fx);

		/* Hack -- for now, skip artifacts */
		if (artifact_p(o_ptr) ||
		    (f3 & TR3_IGNORE_ELEC) ||
		    (f2 & TR2_IM_ELEC) ||
		    (f2 & TR2_RES_ELEC)) continue;

		damaged = FALSE;

		/* Get the item in that slot */
		o_ptr = &p_ptr->inventory[i];

		/* Describe the object */
		object_desc(Ind, o_name, o_ptr, FALSE, 0);

		/* damage it */
		if (o_ptr->timeout_magic) {
			damaged = TRUE;
			if (o_ptr->timeout_magic > 1000) o_ptr->timeout_magic -= 80 + rand_int(41);
			else if (o_ptr->timeout_magic > 100) o_ptr->timeout_magic -= 15 + rand_int(11);
			else if (o_ptr->timeout_magic > 10) o_ptr->timeout_magic -= 3 + rand_int(3);
			else if (o_ptr->timeout_magic) o_ptr->timeout_magic--;
		} else if (o_ptr->tval == TV_ROD) {
			discharge_rod(o_ptr, 5 + rand_int(5));
			damaged = TRUE;
		}

		if (o_ptr->pval) switch (o_ptr->tval) {
		case TV_AMULET: if (o_ptr->pval < 2) break; /* stop at +1 */
		case TV_STAFF:
		case TV_WAND:
			o_ptr->pval--;
			damaged = TRUE;
			break;
		} else if (o_ptr->bpval > 1 && o_ptr->tval == TV_AMULET) {
			o_ptr->bpval--;
			damaged = TRUE;
		}

		/* Special treatment for new ego-type: Ethereal (ammunition): */
		if (is_ammo(o_ptr->tval) && (o_ptr->name2 == EGO_ETHEREAL || o_ptr->name2b == EGO_ETHEREAL)) {
			msg_format(Ind, "\376\377o%s %s (%c) fades away!",
			    o_ptr->number == 1 ? "Your" : "One of your", o_name, index_to_label(i));
			inven_item_increase(Ind, i, -1);
			inven_item_optimize(Ind, i);
			p_ptr->update |= (PU_BONUS);
			damaged_any = TRUE;
		}
		/* Message */
		else if (damaged) {
			msg_format(Ind, "\376\377oYour %s (%c) %s discharged!",
			    o_name, index_to_label(i), ((o_ptr->number != 1) ? "were" : "was"));
			damaged_any = TRUE;
		}
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* ? */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	return (damaged_any);
}
bool apply_discharge_item(int o_idx, int dam) {
	int	chance = 95;
	bool	damaged = FALSE;
	object_type *o_ptr = &o_list[o_idx];
	u32b fx, f2, f3;

	/* No item, nothing happens */
	if (!o_ptr->k_idx) return (FALSE);

	/* note: used same values as in elec_dam */
	if (dam >= 30) chance -= 10;
	if (dam >= 60) chance -= 10;

	if (magik(chance)) return(FALSE);
	//if (o_ptr->tval == TV_AMULET && magik(50)) return(FALSE); /* further reduce chance? */

	object_flags(o_ptr, &fx, &f2, &f3, &fx, &fx, &fx, &fx);

	/* Hack -- for now, skip artifacts */
	if (artifact_p(o_ptr) ||
	    (f3 & TR3_IGNORE_ELEC) ||
	    (f2 & TR2_IM_ELEC) ||
	    (f2 & TR2_RES_ELEC)) return(FALSE);

	/* damage it */
	if (o_ptr->timeout_magic) {
		damaged = TRUE;
		if (o_ptr->timeout_magic > 1000) o_ptr->timeout_magic -= 80 + rand_int(41);
		else if (o_ptr->timeout_magic > 100) o_ptr->timeout_magic -= 15 + rand_int(11);
		else if (o_ptr->timeout_magic > 10) o_ptr->timeout_magic -= 3 + rand_int(3);
		else if (o_ptr->timeout_magic) o_ptr->timeout_magic--;
	} else if (o_ptr->tval == TV_ROD) {
		discharge_rod(o_ptr, 5 + rand_int(5));
		damaged = TRUE;
	}

	if (o_ptr->pval) switch (o_ptr->tval) {
	case TV_AMULET: if (o_ptr->pval < 2) break; /* stop at +1 */
	case TV_STAFF:
	case TV_WAND:
		o_ptr->pval--;
		damaged = TRUE;
		break;
	} else if (o_ptr->bpval > 1 && o_ptr->tval == TV_AMULET) {
		o_ptr->bpval--;
		damaged = TRUE;
	}

	/* Special treatment for new ego-type: Ethereal (ammunition): */
	if (is_ammo(o_ptr->tval) && (o_ptr->name2 == EGO_ETHEREAL || o_ptr->name2b == EGO_ETHEREAL)) {
		if (o_ptr->number == 1) delete_object_idx(o_idx, TRUE);
		else o_ptr->number--;
		damaged = TRUE;
	}

	return (damaged);
}


/*
 * Apply Nexus
 */
static void apply_nexus(int Ind, monster_type *m_ptr, int Ind_attacker) {
	player_type *p_ptr = Players[Ind];
	int max1, cur1, max2, cur2, ii, jj;

	int chance = (195 - p_ptr->skill_sav) / 2;
	if (p_ptr->res_tele) chance = 50; //equal to 95% saving throw (max)

	if (p_ptr->martyr) return;

	if (IS_PLAYER(Ind_attacker))
		s_printf("APPLY_NEXUS_PY: %s by %s\n", p_ptr->name, Players[Ind_attacker]->name);

	switch (randint((safe_area(Ind) || (p_ptr->mode & MODE_PVP)) ? 5 : 8)) { /* don't do baaad things in Monster Arena Challenge */
	case 4: case 5:
		if (p_ptr->anti_tele || magik(chance)) {
			msg_print(Ind, "You resist the effects!");
			break;
		}
		if (m_ptr) teleport_player_to(Ind, m_ptr->fy, m_ptr->fx);
		else teleport_player(Ind, 200, TRUE);
		break;
	case 1: case 2: case 3:
		if (p_ptr->anti_tele || magik(chance)) {
			msg_print(Ind, "You resist the effects!");
			break;
		}
		teleport_player(Ind, 200, TRUE);
		break;
	case 6:
		if (magik(chance)) {
			msg_print(Ind, "You resist the effects!");
			break;
		}
		/* Teleport Level */
		teleport_player_level(Ind, FALSE);
		break;
	case 7:
		/* for now full saving throw for this one, since it's especially nasty */
		if (rand_int(100) < p_ptr->skill_sav) {
			msg_print(Ind, "You resist the effects!");
			break;
		}
		msg_print(Ind, "\376\377oYour body starts to scramble...");
		s_printf("NEXUS_SCRAMBLE: %s\n", p_ptr->name);

		/* Pick a pair of stats */
		ii = rand_int(6);
		for (jj = ii; jj == ii; jj = rand_int(6)) /* loop */;
		max1 = p_ptr->stat_max[ii];
		cur1 = p_ptr->stat_cur[ii];
		max2 = p_ptr->stat_max[jj];
		cur2 = p_ptr->stat_cur[jj];
		p_ptr->stat_max[ii] = max2;
		p_ptr->stat_cur[ii] = cur2;
		p_ptr->stat_max[jj] = max1;
		p_ptr->stat_cur[jj] = cur1;
		p_ptr->update |= (PU_BONUS | PU_MANA | PU_HP | PU_SANITY);
		break;
	case 8:
		if (check_st_anchor(&p_ptr->wpos, p_ptr->py, p_ptr->px)) break;
		if (p_ptr->anti_tele || magik(chance)) {
			msg_print(Ind, "You resist the effects!");
			break;
		}
		msg_print(Ind, "\376\377oYour backpack starts to scramble...");
		if (m_ptr) {
			ii = 10 + m_ptr->level / 3;
			if (ii > 50) ii = 50;
		} else ii = 25;
		do_player_scatter_items(Ind, 5, ii);
		break;
	}
}


/*
 * Apply Polymorph
 */

/* Ho Ho Ho... I really want this to have a chance of turning people into
   a fruit bat,  but something tells me I should put that off until the next version...

   I CANT WAIT. DOING FRUIT BAT RIGHT NOW!!! 
   -APD-
*/
/* note: apply_morph() is only called by polymorph-rod/wand usage.
   disable body-scrambling, to make it more usable for un-bat'ing someone? */
#define DISABLE_MORPH_SCRAMBLE
static void apply_morph(int Ind, int power, char * killer, int Ind_attacker) {
	player_type *p_ptr = Players[Ind];
#ifndef DISABLE_MORPH_SCRAMBLE
	int max1, cur1, max2, cur2, ii, jj;
#endif

	if (p_ptr->martyr) return;

	if (IS_PLAYER(Ind_attacker))
		s_printf("APPLY_MORPH_PY: %s by %s\n", p_ptr->name, Players[Ind_attacker]->name);

#ifndef DISABLE_MORPH_SCRAMBLE
	switch (randint(2)) {
	case 1:
		if (rand_int(40 + power * 2) < p_ptr->skill_sav) {
			msg_print(Ind, "You resist the effects!");
			break;
		}

		msg_print(Ind, "\376\377oYour body starts to scramble...");
		s_printf("MORPH_SCRAMBLE: %s\n", p_ptr->name);

		/* Pick a pair of stats */
		ii = rand_int(6);
		for (jj = ii; jj == ii; jj = rand_int(6)) /* loop */;

		max1 = p_ptr->stat_max[ii];
		cur1 = p_ptr->stat_cur[ii];
		max2 = p_ptr->stat_max[jj];
		cur2 = p_ptr->stat_cur[jj];

		p_ptr->stat_max[ii] = max2;
		p_ptr->stat_cur[ii] = cur2;
		p_ptr->stat_max[jj] = max1;
		p_ptr->stat_cur[jj] = cur1;

		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SANITY);

		break;
	case 2:
#endif
		if (!p_ptr->fruit_bat) {
			if (rand_int(10 + power * 4) < p_ptr->skill_sav) {
				msg_print(Ind, "You resist the effects!");
			} else {
				s_printf("MORPH_FRUITBAT: %s\n", p_ptr->name);
				/* FRUIT BAT!!!!!! */
				if (p_ptr->body_monster) do_mimic_change(Ind, 0, TRUE);
				msg_print(Ind, "\377yYou have been turned into a fruit bat!");
				strcpy(p_ptr->died_from, killer);
				strcpy(p_ptr->really_died_from, killer);
				p_ptr->fruit_bat = -1;
				p_ptr->deathblow = 0;
				player_death(Ind);
			}
		} else if (p_ptr->fruit_bat == 2) {
			/* no saving throw for being restored..... */
			msg_print(Ind, "You have been restored!");
			p_ptr->fruit_bat = 0;
			p_ptr->update |= (PU_BONUS | PU_HP);
		} else {
			msg_print(Ind, "You feel certain you are a fruit bat!");
		}
#ifndef DISABLE_MORPH_SCRAMBLE
	}
#endif
}



/* For project_..():
   Decrease the damage over the radius. */
static int radius_damage(int dam, int div, int typ) {
	switch (typ) {
	/* When these are cast as 'ball spells' they'd be gimped too much probably: */
	case GF_TELEPORT_PLAYER: //Kurzel - This and many others (buffs) could go here (pending approval..)!
	case GF_OLD_SLOW:
	case GF_OLD_CONF:
	case GF_OLD_SLEEP:
	case GF_TURN_ALL: //fear
	case GF_TERROR:

	/* For priest spell (Doomed Grounds) -- carries hack (9999) or percentual damage (2%) */
	case GF_OLD_DRAIN:
	case GF_ANNIHILATION:

	/* These must not be reduced, since 'dam' stores the functionality */
	case GF_RECALL_PLAYER: /* not for recall (dam is timeout) - mikaelh */
	case GF_CURE_PLAYER:
	case GF_RESTORE_PLAYER:
	case GF_CURING:
	case GF_RESURRECT_PLAYER:
	case GF_EXTRA_STATS:
	//case GF_ZEAL_PLAYER:

		return (dam);

	case GF_DARK_WEAK:
		/* May carry special flag for occult shadow darkness spell */
		return (dam & 0x2000) + (dam & 0x1FFF) / div;
	}

	/* default: half damage per grid of distance */
	return (dam / div);
}



/*
 * Hack -- track "affected" monsters
 */
static int project_m_n;
static int project_m_x;
static int project_m_y;



/*
 * We are called from "project()" to "damage" terrain features
 *
 * We are called both for "beam" effects and "ball" effects.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 *
 * XXX XXX XXX We also "see" grids which are "memorized", probably a hack
 *
 * XXX XXX XXX Perhaps we should affect doors?
 */
static bool project_f(int Ind, int who, int r, struct worldpos *wpos, int y, int x, int dam, int typ, int flg) {
	bool obvious = FALSE;
	//bool quiet = ((Ind <= 0) ? TRUE : FALSE);
	bool quiet = ((Ind <= 0 || who <= PROJECTOR_UNUSUAL) ? TRUE : FALSE);
	int div, i;

	player_type *p_ptr = (quiet ? NULL : Players[Ind]);

	byte *w_ptr = (quiet ? NULL : &p_ptr->cave_flag[y][x]);
	cave_type *c_ptr;
	cave_type **zcave;


	if (!(zcave = getcave(wpos))) return(FALSE);
	c_ptr = &zcave[y][x];

	/* Extract radius */
	div = r + 1;
	/* Decrease damage */
	dam = radius_damage(dam, div, typ);

	/* XXX XXX */
	who = who ? who : 0;


	/* Analyze the type */
	switch (typ) {
		/* Ignore most effects */
		case GF_ACID:
			if (!allow_terraforming(wpos, FEAT_TREE)) break;
			/* Destroy trees */
			if (c_ptr->feat == FEAT_TREE || c_ptr->feat == FEAT_BUSH) {
#if 0 /* no msg spam maybe */
				/* Hack -- special message */
				if (!quiet && player_can_see_bold(Ind, y, x)) {
					msg_print(Ind, "The tree decays!");
					/* Notice */
					note_spot(Ind, y, x);
					obvious = TRUE;
				}
#endif
				/* Destroy the tree */
				cave_set_feat_live(wpos, y, x, FEAT_DEAD_TREE);
			}

			/* Burn grass et al */
			if (c_ptr->feat == FEAT_GRASS || c_ptr->feat == FEAT_IVY ||
			    c_ptr->feat == FEAT_WEB ||
			    c_ptr->feat == FEAT_CROP || c_ptr->feat == FEAT_FLOWER /* :( */)
				cave_set_feat_live(wpos, y, x, FEAT_DIRT);//(was FEAT_MUD) or maybe FEAT_ASH?

			break;

		case GF_NUKE:
			if (!allow_terraforming(wpos, FEAT_TREE)) break;
			/* damage organic material */
			if (c_ptr->feat == FEAT_GRASS || c_ptr->feat == FEAT_IVY ||
			    /*c_ptr->feat == FEAT_WEB ||*/
			    c_ptr->feat == FEAT_CROP || c_ptr->feat == FEAT_FLOWER /* :( */)
				cave_set_feat_live(wpos, y, x, FEAT_DIRT);
			break;

		case GF_ICE:
		case GF_ICEPOISON:
		case GF_SHARDS:
		case GF_FORCE:
			if (!allow_terraforming(wpos, FEAT_TREE)) break;
			/* Shred certain feats */
			if (c_ptr->feat == FEAT_WEB || c_ptr->feat == FEAT_FLOWER /* :( */)
				cave_set_feat_live(wpos, y, x, FEAT_DIRT);
			break;
		case GF_ELEC:
		case GF_COLD:
		case GF_THUNDER:	/* a gestalt now, to remove hacky tri-bolt on thunderstorm */
		case GF_SOUND:
		case GF_MANA: /* <- no web/flower destructing abilities at this time? :-p */
		case GF_HOLY_ORB:
		case GF_CURSE:		/* new spell - the_sandman */
		/* Druidry: */
		case GF_HEALINGCLOUD:
			break;

		case GF_EARTHQUAKE:
			earthquake(wpos, y, x, 0);
			break;

		case GF_STONE_WALL:
			/* Require a "naked" floor grid */
			if (!cave_naked_bold(zcave, y, x)) break;
			if (!allow_terraforming(wpos, FEAT_WALL_EXTRA)) break;
			/* Beware of the houses in town */
			if ((wpos->wz == 0) && (zcave[y][x].info & CAVE_ICKY)) break;
			/* Not if it's an open house door - mikaelh */
			if (c_ptr->feat == FEAT_HOME_OPEN) break;

			/* Place a wall */
			if (c_ptr->feat != FEAT_WALL_EXTRA) c_ptr->info &= ~CAVE_NEST_PIT; /* clear teleport protection for nest grid if changed */
			cave_set_feat_live(wpos, y, x, FEAT_WALL_EXTRA);

			/* Notice */
			if (!quiet) note_spot(Ind, y, x);

			/* Redraw - the walls might block view and cause wall shading etc! */
			for (i = 1; i <= NumPlayers; i++) {
				/* If he's not playing, skip him */
				if (Players[i]->conn == NOT_CONNECTED) continue;
				/* If he's not here, skip him */
				if (!inarea(wpos, &Players[i]->wpos)) continue;

				Players[i]->update |= (PU_VIEW | PU_LITE | PU_FLOW); //PU_DISTANCE, PU_TORCH, PU_MONSTERS??; PU_FLOW needed? both VIEW and LITE needed?
			}
			break;

		/* Burn trees, grass, etc. depending on specific fire type */
		case GF_HOLY_FIRE:
			if (!allow_terraforming(wpos, FEAT_TREE)) break;
			/* Holy Fire doesn't destroy trees! */
			/* spider webs (Cirith Ungol!) */
			if (c_ptr->feat == FEAT_WEB)
				cave_set_feat_live(wpos, y, x, FEAT_ASH);

			if (c_ptr->feat == FEAT_DEAD_TREE) { //dead trees only^^
				/* Destroy the tree */
				cave_set_feat_live(wpos, y, x, FEAT_ASH);
			}

#if 1			/* FEAT_ICE_WALL are tunneable, so probably no harm in making them meltable too */
			if (c_ptr->feat == FEAT_ICE_WALL && !rand_int((410 - (dam < 370 ? dam : 370)) / 4))
				cave_set_feat_live(wpos, y, x, FEAT_SHAL_WATER);
#endif
			break;

		case GF_HAVOC:
		case GF_FIRE:
		case GF_METEOR:
		case GF_PLASMA:
		case GF_HELL_FIRE:
		case GF_INFERNO:
		//GF_DETONATION and GF_ROCKET disintegrate anyway
			if (!allow_terraforming(wpos, FEAT_TREE)) break;
			/* Destroy trees */
			if (c_ptr->feat == FEAT_TREE ||
			    c_ptr->feat == FEAT_BUSH ||
			    c_ptr->feat == FEAT_DEAD_TREE) {
#if 0 /* no need for message spam maybe - and spot is note'd in cave_set_feat_live already */
				/* Hack -- special message */
				if (!quiet && player_can_see_bold(Ind, y, x)) {
					msg_print(Ind, "The tree burns to the ground!");
					/* Notice */
					note_spot(Ind, y, x);
					obvious = TRUE;
				}
#endif
				/* Destroy the tree */
				cave_set_feat_live(wpos, y, x, FEAT_ASH);
			}

			/* Burn grass, spider webs (Cirith Ungol!) and more.. */
			if (c_ptr->feat == FEAT_GRASS || c_ptr->feat == FEAT_IVY ||
			    c_ptr->feat == FEAT_WEB ||
			    c_ptr->feat == FEAT_CROP || c_ptr->feat == FEAT_FLOWER /* :( */)
				cave_set_feat_live(wpos, y, x, FEAT_ASH);

			/* misc flavour: turn mud to dirt and/or shallow water into nothing (steam)? */
			if (c_ptr->feat == FEAT_MUD && rand_int(10 + dam / 100) > 7) cave_set_feat_live(wpos, y, x, FEAT_DIRT);
			if (c_ptr->feat == FEAT_SHAL_WATER && rand_int(10 + dam / 100) > 7) cave_set_feat_live(wpos, y, x, FEAT_MUD);

#if 1
			/* FEAT_ICE_WALL are tunneable, so probably no harm in making them meltable too */
			if (c_ptr->feat == FEAT_ICE_WALL && !rand_int((410 - (dam < 370 ? dam : 370)) / 4))
				cave_set_feat_live(wpos, y, x, FEAT_SHAL_WATER);
#endif
			break;

		/* Destroy Traps (and Locks) */
		case GF_KILL_TRAP:
		{
			struct c_special *cs_ptr;
			/* Destroy invisible traps */
			//if (c_ptr->feat == FEAT_INVIS)
			if ((cs_ptr = GetCS(c_ptr, CS_TRAPS))) {
				/* Hack -- special message */
				if (!quiet && player_can_see_bold(Ind, y, x)) {
					msg_print(Ind, "There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Destroy the trap */
//				c_ptr->feat = FEAT_FLOOR;
				cs_erase(c_ptr, cs_ptr);

				if (!quiet) {
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					if (c_ptr->o_idx && !c_ptr->m_idx)
						/* Make sure no traps are displayed on the screen anymore - mikaelh */
						everyone_clear_ovl_spot(wpos, y, x);
					else
						everyone_lite_spot(wpos, y, x);
				}
			}

			/* Secret / Locked doors are found and unlocked */
			else if ((c_ptr->feat == FEAT_SECRET) ||
			    ((c_ptr->feat >= FEAT_DOOR_HEAD + 0x01) &&
			    (c_ptr->feat <= FEAT_DOOR_HEAD + 0x07))) {
				/* Notice */
				if (!quiet && (*w_ptr & CAVE_MARK)) {
					msg_print(Ind, "Click!");
					obvious = TRUE;
				}

				/* Unlock the door */
				c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

				/* Clear mimic feature */
				if ((cs_ptr = GetCS(c_ptr, CS_MIMIC)))
					cs_erase(c_ptr, cs_ptr);

				if (!quiet) {
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					if (c_ptr->o_idx && !c_ptr->m_idx)
						/* Make sure no traps are displayed on the screen anymore - mikaelh */
						everyone_clear_ovl_spot(wpos, y, x);
					else
						everyone_lite_spot(wpos, y, x);
				}
			}

			break;
		}

		/* Destroy Doors (and traps on them) */
		case GF_KILL_DOOR:
		{
			byte feat = twall_erosion(wpos, y, x);
			struct c_special *cs_ptr;
			bool door = FALSE;

			/* Destroy all visible traps and open doors */
			if ((c_ptr->feat == FEAT_OPEN) ||
			    (c_ptr->feat == FEAT_BROKEN)) {
				door = TRUE;

				/* Hack -- special message */
				if (!quiet && (*w_ptr & CAVE_MARK)) {
					msg_print(Ind, "There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Destroy the feature */
				cave_set_feat_live(wpos, y, x, feat);
				/* Forget the wall */
				everyone_forget_spot(wpos, y, x);

				if (!quiet) {
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					if (c_ptr->o_idx && !c_ptr->m_idx)
						/* Make sure no traps are displayed on the screen anymore - mikaelh */
						everyone_clear_ovl_spot(wpos, y, x);
					else
						everyone_lite_spot(wpos, y, x);
				}
			}

			/* Destroy all closed doors */
			if ((c_ptr->feat >= FEAT_DOOR_HEAD) &&
			    (c_ptr->feat <= FEAT_DOOR_TAIL)) {
				door = TRUE;

				/* Hack -- special message */
				if (!quiet && (*w_ptr & CAVE_MARK)) {
					msg_print(Ind, "There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Destroy the feature */
				cave_set_feat_live(wpos, y, x, feat);

				/* Forget the wall */
				everyone_forget_spot(wpos, y, x);

				if (!quiet) {
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					everyone_lite_spot(wpos, y, x);

					/* Update some things */
					p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
				}
			}

			/* Destroy any traps that were on this door */
			if (door && (cs_ptr = GetCS(c_ptr, CS_TRAPS))) {
				/* Hack -- special message */
				if (!quiet && player_can_see_bold(Ind, y, x)) {
					msg_print(Ind, "There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Destroy the feature */
				//c_ptr->feat = FEAT_FLOOR;
				cs_erase(c_ptr, cs_ptr);

				/* Forget the wall */
				everyone_forget_spot(wpos, y, x);

				if (!quiet) {
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					if (c_ptr->o_idx && !c_ptr->m_idx)
						/* Make sure no traps are displayed on the screen anymore - mikaelh */
						everyone_clear_ovl_spot(wpos, y, x);
					else
						everyone_lite_spot(wpos, y, x);
				}
			}
			break;
		}

		/* Destroy Traps and Doors */
		case GF_KILL_TRAP_DOOR:
		{
			byte feat = twall_erosion(wpos, y, x);
			struct c_special *cs_ptr;

			/* Destroy any traps */
			if ((cs_ptr = GetCS(c_ptr, CS_TRAPS))) {
				/* Hack -- special message */
				if (!quiet && player_can_see_bold(Ind, y, x)) {
					msg_print(Ind, "There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Destroy the feature */
				//c_ptr->feat = FEAT_FLOOR;
				cs_erase(c_ptr, cs_ptr);

				/* Forget the wall */
				everyone_forget_spot(wpos, y, x);

				if (!quiet) {
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					if (c_ptr->o_idx && !c_ptr->m_idx)
						/* Make sure no traps are displayed on the screen anymore - mikaelh */
						everyone_clear_ovl_spot(wpos, y, x);
					else
						everyone_lite_spot(wpos, y, x);
				}
			}

			/* Destroy all visible traps and open doors */
			if ((c_ptr->feat == FEAT_OPEN) ||
			    (c_ptr->feat == FEAT_BROKEN)) {
				/* Hack -- special message */
				if (!quiet && (*w_ptr & CAVE_MARK)) {
					msg_print(Ind, "There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Destroy the feature */
				cave_set_feat_live(wpos, y, x, feat);
				/* Forget the wall */
				everyone_forget_spot(wpos, y, x);

				if (!quiet) {
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					if (c_ptr->o_idx && !c_ptr->m_idx)
						/* Make sure no traps are displayed on the screen anymore - mikaelh */
						everyone_clear_ovl_spot(wpos, y, x);
					else
						everyone_lite_spot(wpos, y, x);
				}
			}

			/* Destroy all closed doors */
			if ((c_ptr->feat >= FEAT_DOOR_HEAD) &&
			    (c_ptr->feat <= FEAT_DOOR_TAIL)) {
				/* Hack -- special message */
				if (!quiet && (*w_ptr & CAVE_MARK)) {
					msg_print(Ind, "There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Destroy the feature */
				cave_set_feat_live(wpos, y, x, feat);

				/* Forget the wall */
				everyone_forget_spot(wpos, y, x);

				if (!quiet) {
					/* Notice */
					note_spot(Ind, y, x);

					/* Redraw */
					everyone_lite_spot(wpos, y, x);

					/* Update some things */
					p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
				}
			}
			break;
		}

		/* Destroy walls (and doors) */
		case GF_KILL_WALL:
		{
			byte feat = twall_erosion(wpos, y, x);

			if (!allow_terraforming(wpos, FEAT_TREE)) break;
			/* Non-walls (etc) */
			if (cave_los(zcave, y, x)) break;
			/* Permanent walls */
			if ((c_ptr->feat >= FEAT_PERM_EXTRA || c_ptr->feat == FEAT_PERM_CLEAR)
			    && !(c_ptr->feat >= FEAT_SANDWALL && c_ptr->feat <= FEAT_SANDWALL_K)) break;

			/* the_sandman: sandwalls are stm-able too? */
			/* fixed it and also added the treasures in sandwalls - mikaelh */
			/* Sandwall */
			if (c_ptr->feat == FEAT_SANDWALL) {
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK)) {
					msg_print(Ind, "The sandwall collapses!");
					obvious = TRUE;
				}

				/* Destroy the wall */
				cave_set_feat_live(wpos, y, x, (feat == FEAT_FLOOR) ? FEAT_SAND : feat);
			}
			/* Sandwall with treasure */
			else if (c_ptr->feat == FEAT_SANDWALL_H || c_ptr->feat == FEAT_SANDWALL_K) {
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK)) {
					msg_print(Ind, "The sandwall collapses!");
					if (!istown(wpos)) msg_print(Ind, "You have found something!");
					obvious = TRUE;
				}

				/* Destroy the wall */
				cave_set_feat_live(wpos, y, x, (feat == FEAT_FLOOR) ? FEAT_SAND : feat);

				/* Place some gold */
				if (!istown(wpos)) place_gold(wpos, y, x, 0);
			}
			/* Granite */
			else if (c_ptr->feat >= FEAT_WALL_EXTRA) {
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK)) {
					msg_print(Ind, "The wall turns into mud!");
					obvious = TRUE;
				}

				/* Destroy the wall */
				cave_set_feat_live(wpos, y, x, (feat == FEAT_FLOOR) ? FEAT_MUD : feat);
			}
			/* Quartz / Magma with treasure */
			else if (c_ptr->feat >= FEAT_MAGMA_H) {
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK)) {
					msg_print(Ind, "The vein turns into mud!");
					if (!istown(wpos)) msg_print(Ind, "You have found something!");
					obvious = TRUE;
				}

				/* Destroy the wall */
				cave_set_feat_live(wpos, y, x, (feat == FEAT_FLOOR) ? FEAT_MUD : feat);

				/* Place some gold */
				if (!istown(wpos)) place_gold(wpos, y, x, 0);
			}

			/* Quartz / Magma */
			else if (c_ptr->feat >= FEAT_MAGMA) {
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK)) {
					msg_print(Ind, "The vein turns into mud!");
					obvious = TRUE;
				}

				/* Destroy the wall */
				cave_set_feat_live(wpos, y, x, (feat == FEAT_FLOOR) ? FEAT_MUD : feat);
			}

			/* Rubble */
			else if (c_ptr->feat == FEAT_RUBBLE) {
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK)) {
					msg_print(Ind, "The rubble turns into mud!");
					obvious = TRUE;
				}

				/* Destroy the rubble */
				cave_set_feat_live(wpos, y, x, (feat == FEAT_FLOOR) ? FEAT_MUD : feat);

				/* Hack -- place an object */
				if (rand_int(100) < 10 && !quiet) {
					/* Found something */
					if (player_can_see_bold(Ind, y, x) && !istown(wpos)) {
						msg_print(Ind, "There was something buried in the rubble!");
						obvious = TRUE;
					}

					/* Place object */
					if (!istown(wpos)) {
						place_object_restrictor = RESF_NONE;
						place_object(wpos, y, x, FALSE, FALSE, FALSE, make_resf(p_ptr) | RESF_LOW, default_obj_theme, p_ptr->luck, ITEM_REMOVAL_NORMAL);
					}
				}
			}

			/* House doors are immune */
			else if (c_ptr->feat == FEAT_HOME) {
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK)) {
					msg_print(Ind, "The door resists.");
					obvious = TRUE;
				}
			}

			/* Destroy doors (and secret doors) */
			else if (c_ptr->feat >= FEAT_DOOR_HEAD) {
				/* Hack -- special message */
				if (!quiet && (*w_ptr & CAVE_MARK)) {
					msg_print(Ind, "The door turns into mud!");
					obvious = TRUE;
				}

				/* Destroy the feature */
				cave_set_feat_live(wpos, y, x, (feat == FEAT_FLOOR) ? FEAT_MUD : feat);
			}

			/* Forget the wall */
			everyone_forget_spot(wpos, y, x);

			if (!quiet) {
				/* Notice */
				note_spot(Ind, y, x);

				/* Redraw */
				everyone_lite_spot(wpos, y, x);

				/* Update some things */
				p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
			}

			break;
		}

		/* Make doors */
		case GF_MAKE_DOOR:
			if (!allow_terraforming(wpos, FEAT_TREE)) break;
			/* Require a "naked" floor grid */
			if (!cave_naked_bold(zcave, y, x)) break;

			/* Create a closed door */
			c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

			if (!quiet) {
				/* Notice */
				note_spot(Ind, y, x);
				/* Redraw */
				everyone_lite_spot(wpos, y, x);
				/* Observe */
				if (*w_ptr & CAVE_MARK) obvious = TRUE;
				/* Update some things */
				p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
			}
			break;

		/* Make traps */
		case GF_MAKE_TRAP:
			if (!allow_terraforming(wpos, FEAT_TREE)) break;
			/* Require a "naked" floor grid */
			if ((zcave[y][x].feat != FEAT_MORE && zcave[y][x].feat != FEAT_LESS) && cave_perma_bold(zcave, y, x)) break;

			/* Place a trap */
			place_trap(wpos, y, x, dam);

			if (!quiet) {
				/* Notice */
				note_spot(Ind, y, x);
				/* Redraw */
				everyone_lite_spot(wpos, y, x);
			}
			break;

		/* Lite up the grid */
		case GF_STARLITE:
		case GF_LITE_WEAK:
		case GF_LITE:
			/* Exception: Bolt-type spells have no special effect */
			if (!(flg & (PROJECT_NORF | PROJECT_JUMP))) break;

			/* don't ruin the mood :> (allow turning on light inside houses though) */
			if ((!wpos->wz && (season_halloween || season_newyearseve)) && !(c_ptr->info & CAVE_ICKY)) break;

			/* Turn on the light */
			c_ptr->info |= CAVE_GLOW;

			if (!quiet) {
				/* Notice */
				note_spot_depth(wpos, y, x);
				/* Redraw */
				everyone_lite_spot(wpos, y, x);
				/* Observe */
				if (player_can_see_bold(Ind, y, x)) obvious = TRUE;
			}

			/* Mega-Hack -- Update the monster in the affected grid */
			/* This allows "spear of light" (etc) to work "correctly" */
			if (c_ptr->m_idx > 0) update_mon(c_ptr->m_idx, FALSE);

			break;

		/* Darken the grid */
		case GF_DARK_WEAK:
		case GF_DARK:
			/* Exception: Bolt-type spells have no special effect */
			if (!(flg & (PROJECT_NORF | PROJECT_JUMP))) break;

			/* don't ruin the mood :> (allow turning on light inside houses though) */
			if ((!wpos->wz && (season_halloween || season_newyearseve)) && !(c_ptr->info & CAVE_ICKY)) break;

			/* Notice */
			if (!quiet && player_can_see_bold(Ind, y, x)) obvious = TRUE;

			/* Turn off the light. */
			c_ptr->info &= ~CAVE_GLOW;

			/* Hack -- Forget "boring" grids */
			    //if (c_ptr->feat <= FEAT_INVIS)
			if (cave_plain_floor_grid(c_ptr)) {
				/* Forget the wall */
				everyone_forget_spot(wpos, y, x);
				if (!quiet)
					/* Notice */
					note_spot(Ind, y, x);
			}
			if (!quiet)
				/* Redraw */
				everyone_lite_spot(wpos, y, x);

			/* Mega-Hack -- Update the monster in the affected grid */
			/* This allows "spear of light" (etc) to work "correctly" */
			if (c_ptr->m_idx > 0) update_mon(c_ptr->m_idx, FALSE);

			break;

		case GF_KILL_GLYPH:
		{
			byte feat = twall_erosion(wpos, y, x);
			if (!allow_terraforming(wpos, FEAT_TREE)) break;
			if (c_ptr->feat == FEAT_GLYPH || c_ptr->feat == FEAT_RUNE)
				cave_set_feat_live(wpos, y, x, (feat == FEAT_FLOOR) ? FEAT_DIRT : feat);
		}

		/* from PernA	- Jir - */
#if 0
		case GF_MAKE_GLYPH:
			/* Require a "naked" floor grid */
			if (!cave_clean_bold(zcave, y, x)) break;
			if((f_info[c_ptr->feat].flags1 & FF1_PERMANENT)) break;
			if (allow_terraforming(&p_ptr->wpos, FEAT_GLYPH))
				cave_set_feat_live(wpos, y, x, FEAT_GLYPH);
			break;
#endif

		// GF_ROCKET and GF_DISINTEGRATE are handled in project()

		case GF_WATER:
		case GF_WAVE:
		case GF_VAPOUR:
		case GF_WATERPOISON:	//New druid stuff
		{
			int p1 = 0;
			int p2 = 0;
			int f1 = FEAT_NONE;
			int f2 = FEAT_NONE;
			int f = FEAT_NONE;
			int k;

			if (!allow_terraforming(wpos, FEAT_SHAL_WATER)) break;
			/* "Permanent" features will stay */
			if ((f_info[c_ptr->feat].flags1 & FF1_PERMANENT)) break;
			/* Needs more than 30 damage */
			if (dam < 30) break;

			bool old_rand = Rand_quick;
			u32b tmp_seed = Rand_value;

			if ((c_ptr->feat == FEAT_FLOOR) ||
			    (c_ptr->feat == FEAT_DIRT) ||
			    (c_ptr->feat == FEAT_GRASS)) {
				/* p1 % chance to create this feat */
				p1 = 20; f1 = FEAT_SHAL_WATER;

				if (c_ptr->feat == FEAT_DIRT) {
					/* reduce feat1 chance */
					p1 = 10;
					/* p2 % chance to create this feat */
					p2 = 15; f2 = FEAT_MUD;
				}
			}
			else if (c_ptr->feat == FEAT_SHAL_LAVA) {
				/* 15% chance to convert it to normal floor */
				p1 = 15; f1 = FEAT_VOLCANIC;//FEAT_ROCKY, FEAT_ASH
			}

			/* Use the stored/quick RNG */
			Rand_quick = TRUE;
			//using fixed seed here to make sure that repeated casting won't cover 100% of the area with water:
			Rand_value = (3623 * wpos->wy + 29753) * (2843 * wpos->wx + 48869) + (1741 * y + 22109) * y * x + (x + 96779) * x + 42 + wpos->wz;

			k = rand_int(100);

			/* Restore RNG */
			Rand_quick = old_rand;
			Rand_value = tmp_seed;

			if (k < p1) f = f1;
			else if (k < p1 + p2) f = f2;

			if (f) {
//uses static array set in generate.c, fix!	if (f == FEAT_FLOOR) place_floor_live(wpos, y, x);
				//else
				cave_set_feat_live(wpos, y, x, f);
				/* Notice */
				if (!quiet) note_spot(Ind, y, x);
				/* Redraw */
				everyone_lite_spot(wpos, y, x);
//				if (seen) obvious = TRUE;
			}
			break;
		}
	}

	/* Return "Anything seen?" */
	return (obvious);
}



/*
 * We are called from "project()" to "damage" objects
 *
 * We are called both for "beam" effects and "ball" effects.
 *
 * Perhaps we should only SOMETIMES damage things on the ground.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * XXX XXX XXX We also "see" grids which are "memorized", probably a hack
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 */
static bool project_i(int Ind, int who, int r, struct worldpos *wpos, int y, int x, int dam, int typ) {
	player_type *p_ptr = NULL;
	u16b this_o_idx, next_o_idx = 0;
	bool obvious = FALSE;
	bool quiet = ((Ind <= 0 || Ind >= 0 - PROJECTOR_UNUSUAL) ? TRUE : FALSE);
	u32b f1, f2, f3, f4, f5, f6, esp;
	char o_name[ONAME_LEN];
	int o_sval = 0;
	bool is_potion = FALSE, is_basic_potion = FALSE, is_meltable = FALSE;

	int div;
	cave_type **zcave;
	cave_type *c_ptr;
	object_type *o_ptr;
	object_kind *k_ptr;

	if (!(zcave = getcave(wpos))) return(FALSE);
	c_ptr = &zcave[y][x];
	//o_ptr = &o_list[c_ptr->o_idx];

	/* Nothing here */
	if (!c_ptr->o_idx) return (FALSE);

	/* Set the player pointer */
	if (!quiet) p_ptr = Players[Ind];

	/* Extract radius */
	div = r + 1;

	/* Adjust damage */
	dam = radius_damage(dam, div, typ);


	/* XXX XXX */
	who = who ? who : 0;

	/* Scan all objects in the grid */
    for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
	bool	is_art = FALSE;
	bool	ignore = FALSE;
	bool	plural = FALSE;
	bool	do_kill = FALSE;
	bool	do_smash_effect = FALSE;

	cptr	note_kill = NULL;

#ifdef SMELTING
	bool	melt = FALSE;
#endif


	/* Acquire object */
	o_ptr = &o_list[this_o_idx];
	k_ptr = &k_info[o_ptr->k_idx];
	/* Check for (nothing), execute hack to protect such items */
	if (nothing_test(o_ptr, NULL, wpos, x, y, 3)) {
		//s_printf("NOTHINGHACK: spell doesn't meet item at wpos %d,%d,%d.\n", wpos->wx, wpos->wy, wpos->wz);
		return(FALSE);
	}

	/* Acquire next object */
	next_o_idx = o_ptr->next_o_idx;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* get object name */
	if ((Ind >= 0) && ((0 - Ind) > PROJECTOR_UNUSUAL))
		object_desc(Ind, o_name, o_ptr, FALSE, 0);

	/* Get the "plural"-ness */
	if (o_ptr->number > 1) plural = TRUE;

	/* Check for artifact */
	if (artifact_p(o_ptr)) is_art = TRUE;

	/* Stormbringer is artifact-like - C. Blue */
	if (o_ptr->name2 == EGO_STORMBRINGER) is_art = TRUE;

	o_sval = o_ptr->sval;
	/* potion_smash_effect only takes sval as parameter, so it can't handle TV_POTION2 at this time.
	   For this reason, add is_basic_potion: It determines whether smash effect is applied. */
	is_potion = ((k_info[o_ptr->k_idx].tval == TV_POTION) || (k_info[o_ptr->k_idx].tval == TV_POTION2));
	is_basic_potion = (k_info[o_ptr->k_idx].tval == TV_POTION);
	is_meltable = (k_info[o_ptr->k_idx].tval == TV_BOTTLE || (k_info[o_ptr->k_idx].tval == TV_GAME && k_info[o_ptr->k_idx].sval == SV_SNOWBALL));

	/* Analyze the type */
	switch (typ) {
		/* Identify */
		case GF_IDENTIFY:
			do_kill = FALSE;

			/* Identify it fully */
			if (!quiet) object_aware(Ind, o_ptr);
			object_known(o_ptr);
			break;

		/* Acid -- Lots of things */
		case GF_ACID:
			if (hates_acid(o_ptr)) {
				do_kill = TRUE;
				note_kill = (plural ? " melt!" : " melts!");
				if (f3 & TR3_IGNORE_ACID) ignore = TRUE;
			}
			break;

		/* Elec -- Rings and Wands, and now much more to make IM_ELEC better, and elec less laughed at */
		case GF_ELEC:
			if (hates_elec(o_ptr)) {
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
				if (f3 & TR3_IGNORE_ELEC) ignore = TRUE;
			}
			else apply_discharge_item(this_o_idx, dam);
			break;

		/* Fire -- Flammable objects */
		case GF_FIRE:
			do_smash_effect = TRUE;
			if (hates_fire(o_ptr)) {
				do_kill = TRUE;
				if (is_meltable)
					note_kill = (plural ? " melt!" : " melts!");
				else if (is_potion)
					note_kill = (plural ? " evaporate!" : " evaporates!");
				else
					note_kill = (plural ? " burn up!" : " burns up!");
				if (f3 & TR3_IGNORE_FIRE) ignore = TRUE;
			}
#ifdef SMELTING
			melt = TRUE;
#endif
			break;

		/* Cold -- potions and flasks */
		case GF_COLD:
			if (hates_cold(o_ptr)) {
				note_kill = (plural ? " shatter!" : " shatters!");
				do_kill = TRUE;
				if (f3 & TR3_IGNORE_COLD) ignore = TRUE;
#ifdef USE_SOUND_2010
				else if (!quiet) sound(Ind, "shatter_potion", NULL, SFX_TYPE_MISC, FALSE);
#endif
			}
			break;

		/* Cold -- potions and flasks */
		case GF_WATER:
		case GF_WAVE:
		//case GF_VAPOUR:
		case GF_WATERPOISON:
			if (hates_water(o_ptr)) {
				note_kill = (plural ? " are soaked!" : " is soaked!");
				do_kill = TRUE;
				if (f5 & TR5_IGNORE_WATER) ignore = TRUE;
			}
			break;

		/* Fire + Elec + a bit Force */
		case GF_PLASMA:
			do_smash_effect = TRUE;
			ignore = TRUE;

			if (hates_fire(o_ptr)) {
				do_kill = TRUE;
				if (!(f3 & TR3_IGNORE_FIRE)) {
					ignore = FALSE;
					if (is_meltable)
						note_kill = (plural ? " melt!" : " melts!");
					else if (is_potion)
						note_kill = (plural ? " evaporate!" : " evaporates!");
					else
						note_kill = (plural ? " burn up!" : " burns up!");
				}
			}

			if (hates_elec(o_ptr)) {
				do_kill = TRUE;
				if (!(f3 & TR3_IGNORE_ELEC)) {
					ignore = FALSE;
					note_kill = (plural ? " are destroyed!" : " is destroyed!");
				}
			}
			else apply_discharge_item(this_o_idx, dam / 2);

			/* Note: Force item destruction is probably already covered
			   by applied fire item destruction above */

#ifdef SMELTING
			melt = TRUE;
#endif
			break;

		/* Fire + Impact */
		case GF_METEOR:
			do_smash_effect = TRUE;
			ignore = TRUE;
			if (hates_fire(o_ptr)) {
				do_kill = TRUE;
				if (!(f3 & TR3_IGNORE_FIRE)) {
					ignore = FALSE;
					if (is_meltable)
						note_kill = (plural ? " melt!" : " melts!");
					else if (is_potion)
						note_kill = (plural ? " evaporate!" : " evaporates!");
					else
						note_kill = (plural ? " burn up!" : " burns up!");
				}
			}
			if (hates_impact(o_ptr)) {
				do_kill = TRUE;
				if (!(f3 & TR3_IGNORE_COLD)) {
					ignore = FALSE;
					note_kill = (plural ? " shatter!" : " shatters!");
#ifdef USE_SOUND_2010
					if (!quiet) sound(Ind, "shatter_potion", NULL, SFX_TYPE_MISC, FALSE);
#endif
				}
			}
			break;

		/* Hack -- break potions and such */
		case GF_ICE:
		case GF_ICEPOISON:
			if (hates_cold(o_ptr) || hates_impact(o_ptr)) {
				note_kill = (plural ? " shatter!" : " shatters!");
				do_kill = TRUE;
#ifdef USE_SOUND_2010
				if (!quiet) sound(Ind, "shatter_potion", NULL, SFX_TYPE_MISC, FALSE);
#endif
			} else if (o_ptr->tval == TV_SCROLL || o_ptr->tval == TV_PARCHMENT) { /* not TV_BOOK atm.. */
				note_kill = (plural ? " are shredded!" : " is shredded!");
				do_kill = TRUE;
#ifdef USE_SOUND_2010
				if (!quiet) sound(Ind, "item_scroll", NULL, SFX_TYPE_MISC, FALSE); //todo maybe: new sfx?
#endif
			}
			break;

		case GF_SHARDS:
		case GF_FORCE:
		case GF_SOUND:
		case GF_THUNDER:
			if (hates_impact(o_ptr)) {
				note_kill = (plural ? " shatter!" : " shatters!");
				do_kill = TRUE;
#ifdef USE_SOUND_2010
				if (!quiet) sound(Ind, "shatter_potion", NULL, SFX_TYPE_MISC, FALSE);
#endif
			} else if (typ == GF_SHARDS &&
			    (o_ptr->tval == TV_SCROLL || o_ptr->tval == TV_PARCHMENT)) { /* not TV_BOOK atm.. */
				note_kill = (plural ? " are shredded!" : " is shredded!");
				do_kill = TRUE;
#ifdef USE_SOUND_2010
				if (!quiet) sound(Ind, "item_scroll", NULL, SFX_TYPE_MISC, FALSE); //todo maybe: new sfx?
#endif
			}
			break;

		/* Mana -- destroys everything -- except IGNORE_MANA items :p */
		case GF_ANNIHILATION:
			/* Hack: Doomed Grounds is too weak to kill items */
			if (dam < 7) break;
		case GF_MANA:
			if (!(f5 & (TR5_IGNORE_MANA | TR5_RES_MANA))) {
				do_smash_effect = TRUE;
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
			}
			break;

		case GF_DISINTEGRATE:
			do_smash_effect = FALSE;
			do_kill = TRUE;
			note_kill = is_potion ? (plural ? " evaporate!" : " evaporates!")
						: (plural ? " are pulverized!" : " is pulverized!");
			//note_kill = (plural ? " evaporate!" : " evaporates!");
			break;

		case GF_DISENCHANT:
			if ((f2 & TR2_RES_DISEN) || (f5 & TR5_IGNORE_DISEN)) break;
			if (artifact_p(o_ptr) && magik(100)) break;

			if (o_ptr->timeout_magic) o_ptr->timeout_magic /= 2;

			/* Special treatment for new ego-type: Ethereal (ammunition):
			   Ethereal items don't get disenchanted, but rather disappear completely! */
			if (o_ptr->name2 == EGO_ETHEREAL || o_ptr->name2b == EGO_ETHEREAL) {
				int i = randint(o_ptr->number / 5);

				if (!i) i = 1;
				if (i < o_ptr->number) o_ptr->number -= i;
				else {
					do_kill = TRUE;
					note_kill = plural ? " fade!" : " fades!";
				}
			}

			if (o_ptr->to_h > k_ptr->to_h) o_ptr->to_h--;
			if (o_ptr->to_d > k_ptr->to_d) o_ptr->to_d--;
			if (o_ptr->to_a > k_ptr->to_a) o_ptr->to_a--;

			switch (o_ptr->tval) {
			case TV_ROD:
				discharge_rod(o_ptr, 10 + rand_int(10));
				break;
			case TV_WAND:
			case TV_STAFF:
				o_ptr->pval = 0;
				break;
			default:
				/* changed from >0 to >1 to make items retain a slice of their actual abilities */
				if ((o_ptr->pval > 1 || o_ptr->bpval > 1)
				    && o_ptr->tval != TV_GAME
				    && !is_ammo(o_ptr->tval)
				    && o_ptr->tval != TV_CHEST
				    && o_ptr->tval != TV_BOOK
				    && (o_ptr->tval != TV_RING || o_ptr->sval != SV_RING_POLYMORPH)) {
					if (o_ptr->pval > 1) {
						if (o_ptr->bpval > 1 && rand_int(2)) o_ptr->bpval--;
						else o_ptr->pval--;
					} else o_ptr->bpval--;
				}
				break;
			}
			break;

		case GF_HAVOC:
		case GF_INFERNO:
		case GF_DETONATION:
		case GF_ROCKET:
			do_smash_effect = TRUE; /* allow coolness, while preventing exploiting via m_ptr->hit_proj_id; */
			do_kill = TRUE;
			note_kill = is_potion ? (plural ? " evaporate!" : " evaporates!")
						: (plural ? " are pulverized!" : " is pulverized!");
			//note_kill = (plural ? " evaporate!" : " evaporates!");
			break;

		/* Holy Orb -- destroys cursed non-artifacts */
		case GF_HOLY_FIRE:
		case GF_HOLY_ORB:
			do_smash_effect = TRUE;
			if (cursed_p(o_ptr)
//should be enabled maybe?    || hates_fire(o_ptr)
			    ) {
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
			}
			break;

		case GF_HELL_FIRE:
			do_smash_effect = TRUE;
			if (hates_fire(o_ptr)) {
				do_kill = TRUE;
				if (is_meltable)
					note_kill = (plural ? " melt!" : " melts!");
				else if (is_potion)
					note_kill = (plural ? " evaporate!" : " evaporates!");
				else
					note_kill = (plural ? " burn up!" : " burns up!");
				if (f3 & TR3_IGNORE_FIRE) ignore = TRUE;
			} else if (!cursed_p(o_ptr) && magik(10)) {
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
				do_kill = TRUE;
			}
#ifdef SMELTING
			melt = TRUE;
#endif
			break;

		/* Unlock chests */
		case GF_KILL_TRAP:
		//case GF_KILL_DOOR:
			/* Chests are noticed only if trapped or locked */
			if (o_ptr->tval == TV_CHEST) {
				/* Disarm/Unlock traps */
				if (o_ptr->pval > 0) {
					/* Disarm or Unlock */
					o_ptr->pval = (0 - o_ptr->pval);

					/* Identify */
					object_known(o_ptr);

					/* Notice */
					//if (!quiet && p_ptr->obj_vis[c_ptr->o_idx])
					if (!quiet && p_ptr->obj_vis[this_o_idx]) {
						msg_print(Ind, "Click!");
						obvious = TRUE;
					}
				}
			}
			break;

		/* Nexus, Gravity -- teleports the object away */
		case GF_NEXUS:
		case GF_GRAVITY:
		{
			int j, dist = (typ == GF_NEXUS ? 80 : 15);
			s16b cx, cy;
			object_type tmp_obj = *o_ptr;

			if (check_st_anchor(wpos, y, x)) break;
			//if (seen) obvious = TRUE;

			do_smash_effect = TRUE;

			/* no tricks to get stuff out of suspended guild halls ;> */
			if ((c_ptr->info & CAVE_GUILD_SUS)) break;

			note_kill = (plural ? " disappear!" : " disappears!");

			for (j = 0; j < 10; j++) {
				cx = x + dist - rand_int(dist * 2);
				cy = y + dist - rand_int(dist * 2);

				if (!in_bounds(cy, cx)) continue;
				if (!cave_floor_bold(zcave, cy, cx) ||
				    cave_perma_bold(zcave, cy, cx)) continue;

				//(void)floor_carry(cy, cx, &tmp_obj);
				drop_near(0, &tmp_obj, 0, wpos, cy, cx);

				/* XXX not working? */
				if (!quiet && note_kill)
					msg_format_near_site(y, x, wpos, 0, TRUE, "\377oThe %s%s", o_name, note_kill);

				delete_object_idx(this_o_idx, FALSE);
				break;
			}
			break;
		}

	}


	/* Attempt to destroy the object */
	//if (is_basic_potion && do_smash_effect) potion_smash_effect(who, wpos, y, x, o_sval);

	//if(do_kill && (wpos->wz))
	if (do_kill) {
		/* Effect "observed" */
		//if (!quiet && p_ptr->obj_vis[c_ptr->o_idx])
		if (!quiet && p_ptr->obj_vis[this_o_idx]) obvious = TRUE;

		/* Artifacts, and other objects, get to resist */
		if (is_art || ignore) {
			/* Observe the resist */
			//if (!quiet && p_ptr->obj_vis[c_ptr->o_idx])
			if (!quiet && p_ptr->obj_vis[this_o_idx])
				msg_format(Ind, "The %s %s unaffected!", o_name, (plural ? "are" : "is"));
		}

		/* Kill it */
		else {
			/* Describe if needed */
			//if (!quiet && p_ptr->obj_vis[c_ptr->o_idx] && note_kill)
			if (!quiet && p_ptr->obj_vis[this_o_idx] && note_kill)
				msg_format(Ind, "\377oThe %s%s", o_name, note_kill);

			/* Delete the object */
			//delete_object(wpos, y, x);
			delete_object_idx(this_o_idx, TRUE);

			/* Potions produce effects when 'shattered' */
			if (is_basic_potion && do_smash_effect) {
				/* prevent mass deto exploit */
				if (mon_hit_proj_id == mon_hit_proj_id2) {
					mon_hit_proj_id++;
					(void)potion_smash_effect(who, wpos, y, x, o_sval);
					mon_hit_proj_id2++;
				} else (void)potion_smash_effect(who, wpos, y, x, o_sval);
			}

			/* Redraw */
			if (!quiet) everyone_lite_spot(wpos, y, x);
		}
	}
#ifdef SMELTING
	else if (melt && this_o_idx == c_ptr->o_idx) { /* only apply to raw material on top of a pile, not inside */
		if (o_ptr->tval == TV_GOLD) {
			object_type forge;

			//hardcoded massive piece value and money svals..
			switch (o_ptr->sval) {
			case 1: //copper
				if (dam < 1085 / 2 || o_ptr->pval < 1400 * SMELTING) break;
				invcopy(&forge, lookup_kind(TV_GOLEM, SV_GOLEM_COPPER));
				forge.owner = o_ptr->owner;
				forge.mode = o_ptr->mode;
				forge.number = 1;
				s_printf("MELTING: Copper for %d by %s.\n", o_ptr->pval, p_ptr->name);
				*o_ptr = forge;
				break;
			case 2: //silver
				if (dam < 962 / 2 || o_ptr->pval < 4500 * SMELTING) break;
				invcopy(&forge, lookup_kind(TV_GOLEM, SV_GOLEM_SILVER));
				forge.owner = o_ptr->owner;
				forge.mode = o_ptr->mode;
				forge.number = 1;
				s_printf("MELTING: Silver for %d by %s.\n", o_ptr->pval, p_ptr->name);
				*o_ptr = forge;
				break;
			case 10: //gold
				if (dam < 1064 / 2 || o_ptr->pval < 20000 * SMELTING) break;
				invcopy(&forge, lookup_kind(TV_GOLEM, SV_GOLEM_GOLD));
				forge.owner = o_ptr->owner;
				forge.mode = o_ptr->mode;
				forge.number = 1;
				s_printf("MELTING: Gold for %d by %s.\n", o_ptr->pval, p_ptr->name);
				*o_ptr = forge;
				break;
			}
			//consider mithril/adamantite too hard to melt
		}
		//iron (heavy armour, swords, crowns)
		else if (o_ptr->tval == TV_SWORD || (o_ptr->tval == TV_HARD_ARMOR && o_ptr->sval <= SV_RIBBED_PLATE_ARMOUR) || (o_ptr->tval == TV_CROWN && o_ptr->sval == SV_IRON_CROWN)) {
			int val = object_value_real(0, o_ptr) * o_ptr->number, wgt = o_ptr->weight * o_ptr->number;

			//hardcoded massive piece value...
			if (dam >= 1538 / 2 && val >= 800 && wgt >= 7900) {
				object_type forge;

				invcopy(&forge, lookup_kind(TV_GOLEM, SV_GOLEM_IRON));
				forge.owner = o_ptr->owner;
				forge.mode = o_ptr->mode;
				forge.number = 1;
				s_printf("MELTING: Iron '%s' for %d by %s.\n", o_name, val, p_ptr->name);
				*o_ptr = forge;
				break;
			}
		}
		//silver (crowns)
		else if (o_ptr->tval == TV_CROWN && o_ptr->sval == SV_SILVER_CROWN) {
			int val = object_value_real(0, o_ptr) * o_ptr->number, wgt = o_ptr->weight * o_ptr->number;

			//hardcoded massive piece value...
			if (dam >= 962 / 2 && val >= 4500 && wgt >= 10000) {
				object_type forge;

				invcopy(&forge, lookup_kind(TV_GOLEM, SV_GOLEM_SILVER));
				forge.owner = o_ptr->owner;
				forge.mode = o_ptr->mode;
				forge.number = 1;
				s_printf("MELTING: Silver '%s' for %d by %s.\n", o_name, val, p_ptr->name);
				*o_ptr = forge;
				break;
			}
		}
		//gold (crowns)
		else if (o_ptr->tval == TV_CROWN && o_ptr->sval == SV_GOLDEN_CROWN) {
			int val = object_value_real(0, o_ptr) * o_ptr->number, wgt = o_ptr->weight * o_ptr->number;

			//hardcoded massive piece value...
			if (dam >= 1064 / 2 && val >= 20000 && wgt >= 19000) {
				object_type forge;

				invcopy(&forge, lookup_kind(TV_GOLEM, SV_GOLEM_IRON));
				forge.owner = o_ptr->owner;
				forge.mode = o_ptr->mode;
				forge.number = 1;
				s_printf("MELTING: Gold '%s' for %d by %s.\n", o_name, val, p_ptr->name);
				*o_ptr = forge;
				break;
			}
		}
		//consider mithril/adamantite too hard to melt
	}
	//gold (crown!)
#endif
    }

	/* Return "Anything seen?" */
	return (obvious);
}


#if 0 /* commented out in project_m */
/*
 * Psionic attacking of high level demons/undead can backlash
 */
static bool psi_backlash(int Ind, int m_idx, int dam) {
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);
	char m_name[MNAME_LEN];
	player_type *p_ptr;

	if (!Ind) return FALSE;

	p_ptr = Players[Ind];

	if ((r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
	    (r_ptr->level > p_ptr->lev/2) && !rand_int(2)) {
		monster_desc(Ind, m_name, m_idx, 0);
		switch (m_name[strlen(m_name) - 1]) {
		case 's': case 'x': case 'z':
			msg_format(Ind, "%^s' corrupted mind backlashes your attack!", m_name);
			break;
		default:
			msg_format(Ind, "%^s's corrupted mind backlashes your attack!", m_name);
		}
		project(0 - Ind, m_idx, 0, p_ptr->py, p_ptr->px ,dam / 3, GF_PSI, 0, "");
		return TRUE;
	}
	return FALSE;
}
#endif


/*
 * Helper function for "project()" below.
 *
 * Handle a beam/bolt/ball causing damage to a monster.
 *
 * This routine takes a "source monster" (by index) which is mostly used to
 * determine if the player is causing the damage, and a "radius" (see below),
 * which is used to decrease the power of explosions with distance, and a
 * location, via integers which are modified by certain types of attacks
 * (polymorph and teleport being the obvious ones), a default damage, which
 * is modified as needed based on various properties, and finally a "damage
 * type" (see below).
 *
 * Note that this routine can handle "no damage" attacks (like teleport) by
 * taking a "zero" damage, and can even take "parameters" to attacks (like
 * confuse) by accepting a "damage", using it to calculate the effect, and
 * then setting the damage to zero.  Note that the "damage" parameter is
 * divided by the radius, so monsters not at the "epicenter" will not take
 * as much damage (or whatever)...
 *
 * Note that "polymorph" is dangerous, since a failure in "place_monster()"'
 * may result in a dereference of an invalid pointer.  XXX XXX XXX
 *
 * Various messages are produced, and damage is applied.
 *
 * Just "casting" a substance (i.e. plasma) does not make you immune, you must
 * actually be "made" of that substance, or "breathe" big balls of it.
 *
 * We assume that "Plasma" monsters, and "Plasma" breathers, are immune
 * to plasma.
 *
 * We assume "Nether" is an evil, necromantic force, so it doesn't hurt undead,
 * and hurts evil less.  If can breath nether, then it resists it as well.
 *
 * Damage reductions use the following formulas:
 *   Note that "dam = dam * 6 / (randint(6) + 6);"
 *	 gives avg damage of .655, ranging from .858 to .500
 *   Note that "dam = dam * 5 / (randint(6) + 6);"
 *	 gives avg damage of .544, ranging from .714 to .417
 *   Note that "dam = dam * 4 / (randint(6) + 6);"
 *	 gives avg damage of .444, ranging from .556 to .333
 *   Note that "dam = dam * 3 / (randint(6) + 6);"
 *	 gives avg damage of .327, ranging from .427 to .250
 *   Note that "dam = dam * 2 / (randint(6) + 6);"
 *	 gives something simple.
 *
 * In this function, "result" messages are postponed until the end, where
 * the "note" string is appended to the monster name, if not NULL.  So,
 * to make a spell have "no effect" just set "note" to NULL.  You should
 * also set "notice" to FALSE, or the player will learn what the spell does.
 *
 * We attempt to return "TRUE" if the player saw anything "useful" happen.
 *
 * IMPORTANT: Keep approx_damage() in sync with this.
 */
static bool project_m(int Ind, int who, int y_origin, int x_origin, int r, struct worldpos *wpos, int y, int x, int dam, int typ, int flg) {
	int i = 0, div, k, k_elec, k_sound, k_lite;

	monster_type *m_ptr;
	monster_race *r_ptr;

	cptr name;

	/* Is the monster "seen"? */
	bool seen;
	/* Were the "effects" obvious (if seen)? */
	bool obvious = FALSE;

	/* do not notice things the player did to themselves by ball spell */
	/* fix me XXX XXX XXX */

	bool priest_spell = FALSE, brief_rune_spell = FALSE, dark_spell = FALSE;

	/* Polymorph setting (true or false) */
	int do_poly = 0;
	/* Teleport setting (max distance) */
	int do_dist = 0;
	/* Blindness setting (amount to confuse) */
	int do_blind = 0;
	/* Confusion setting (amount to confuse) */
	int do_conf = 0;
	/* Stunning setting (amount to stun) */
	int do_stun = 0;
	/* Sleep amount (amount to sleep) */
	int do_sleep = 0;
	/* Fear amount (amount to fear) */
	int do_fear = 0;

	/* Hold the monster name */
	char m_name[MNAME_LEN];

	bool resist = FALSE;
	/* Assume no note */
	cptr note = NULL;

	/* Assume a default death */
	cptr note_dies = " dies";
	bool quiet; /* no message output */
	bool quiet_dam = FALSE; /* no damage message output */

	int plev = 25; /* replacement dummy for when a monster isn't
			  affected by a real player but by eg a trap */

	cave_type **zcave;
	cave_type *c_ptr;
	player_type *p_ptr = NULL;


	if (!(zcave = getcave(wpos))) return(FALSE);
	c_ptr = &zcave[y][x];

	/* the_sandman: the priest spell? (0 or 1) - doomed grounds hack for applying proper heal feedback */
	if (typ == GF_OLD_DRAIN && (dam & 4096)) { //currently unused since doomed grounds deals annihilation instead
		priest_spell = TRUE;
		dam &= ~4096; // the real damage (percentage drain)
	}
	//marker to apply reduced damage cap, since it's a 'Brief' runecraft spell
	if (typ == GF_ANNIHILATION && (dam & 8192)) {
		brief_rune_spell = TRUE;
		dam &= ~8192;
	}
	//marker to handle blindness power differently
	if (typ == GF_DARK_WEAK && (dam & 8192)) {
		dark_spell = TRUE;
		dam &= ~8192;
	}

	/* hack -- by trap */
	quiet = ((Ind <= 0 || who <= PROJECTOR_UNUSUAL) ? TRUE : (0 - Ind == c_ptr->m_idx ? TRUE : FALSE));

	//if (quiet) return(FALSE);
	if (Ind <= 0 || 0 - Ind == c_ptr->m_idx) return(FALSE);
	if (!quiet) {
		p_ptr = Players[Ind];
		plev = p_ptr->lev;
	}

	/* Nobody here */
	if (c_ptr->m_idx <= 0) return (FALSE);

	/* Acquire monster pointer */
	m_ptr = &m_list[c_ptr->m_idx];

	/* Prevent recursively afflicted potion_smash_effect() hits */
	if (mon_hit_proj_id != mon_hit_proj_id2) {
		if (m_ptr->hit_proj_id == mon_hit_proj_id) return(FALSE);
		else m_ptr->hit_proj_id = mon_hit_proj_id;
	}

	/* Acquire race pointer */
	r_ptr = race_inf(m_ptr);

	/* Acquire name */
	name = r_name_get(m_ptr);

	/* Never affect projector */
	if ((who > 0) && (c_ptr->m_idx == who)) return (FALSE);

	/* Spells cast by a player never hurt a friendly golem */
	//if (IS_PVP)
	if (m_ptr->owner && who < 0 && who > PROJECTOR_UNUSUAL) {
		//look for its owner to see if he's hostile or not
		for (i = 1; i < NumPlayers; i++)
			if (Players[i]->id == m_ptr->owner) {
				if (!check_hostile(0 - who, i)) return FALSE;
				break;
			}
		//if his owner is not online, assume friendly(!)
		if (i == NumPlayers) return FALSE;
	}

	/* Set the "seen" flag */
	if (!quiet) {
		seen = p_ptr->mon_vis[c_ptr->m_idx];
		/* Cannot 'hear' if too far */
		if (!seen && distance(p_ptr->py, p_ptr->px, y, x) > MAX_SIGHT)
			quiet = TRUE;
	}
	else seen = FALSE;


	/* Get the monster name (BEFORE polymorphing) */
	if (!quiet) monster_desc(Ind, m_name, c_ptr->m_idx, 0);


	/* Handle reflection - it's back, though weaker - C. Blue */
	if ((r_ptr->flags2 & RF2_REFLECTING) &&
	    (flg & PROJECT_KILL) && !(flg & (PROJECT_NORF | PROJECT_JUMP)) /* only for fire_bolt() */
	    && magik(50)) {
		if (seen) msg_print(Ind, "Your attack was deflected.");
		return TRUE; /* notice */
	}

#ifdef USE_BLOCKING
	/* handle blocking (deflection) */
	if (strchr("hHJkpPtyn", r_ptr->d_char) && /* leaving out Yeeks (else Serpent Man 'J') */
	    !(r_ptr->flags3 & RF3_ANIMAL) && !(r_ptr->flags8 & RF8_NO_BLOCK) &&
	    (flg & PROJECT_KILL) && !(flg & (PROJECT_NORF | PROJECT_JUMP | PROJECT_NODF)) /* only for fire_bolt() */
	    && !rand_int(52 - r_ptr->level / 3)) { /* small chance to block spells */
		if (seen) {
			char hit_desc[MAX_CHARS];

			sprintf(hit_desc, "\377%c%s blocks.", COLOUR_BLOCK_MON, m_name);
			hit_desc[2] = toupper(hit_desc[2]);
			msg_print(Ind, hit_desc);
		}
		return TRUE; /* notice */
	}
#endif


	/* Extract radius */
	div = r + 1;

	/* Decrease damage */
	dam = radius_damage(dam, div, typ);


	/* Mega-Hack */
	project_m_n++;
	project_m_x = x;
	project_m_y = y;


	/* Some monsters get "destroyed" */
	if ((r_ptr->flags3 & RF3_DEMON) || (r_ptr->flags3 & RF3_UNDEAD) ||
	    (r_ptr->flags2 & RF2_STUPID) || (strchr("AEvg", r_ptr->d_char))) {
		/* Special note at death */
		note_dies = " is destroyed";
	}

	/* Hack: GF_LIFEHEAL might heal or kill a monster */
	if (typ == GF_LIFEHEAL) {
		if (r_ptr->flags3 & RF3_UNDEAD) typ = GF_HOLY_FIRE;
		else typ = GF_OLD_HEAL;
	}

	/* Analyze the damage type */
	switch (typ) {
		/* psionic mana drain */
		case GF_SILENCE:
			/* hacks: extract power (probability for succeeding) */
			i = dam % 100; /* 1..51 (school 10..50) */
			k = dam - (i * 100);
			/* calculate resistance % chance based on skill level and monster level */
			i += 18;
			i *= i;
			i = i / (r_ptr->level ? r_ptr->level : 1);
			if (i > 69) i = 69; /* cap at 95% */
			i = (i * 138) / 100; /* finalize calculation */
			i = 100 - i;
#ifdef TEST_SERVER
			s_printf("GF_SILENCE: chance i=%d, duration k=%d\n", i, k);
#endif
			/* test resistance */
			if (!((r_ptr->flags4 & RF4_SPELLCASTER_MASK) |
			    (r_ptr->flags5 & RF5_SPELLCASTER_MASK) |
			    (r_ptr->flags6 & RF6_SPELLCASTER_MASK) |
			    (r_ptr->flags0 & RF0_SPELLCASTER_MASK)) ||
			    (r_ptr->level >= 98 && (r_ptr->flags1 & RF1_UNIQUE)) ||
			     m_ptr->silenced != 0) { /* successful attempt also leads to cooldown! */
				note = " is unaffected";
			} else if (((r_ptr->flags1 & RF1_UNIQUE) && magik(50)) ||
			    ((r_ptr->flags2 & RF2_POWERFUL) && magik(50)) ||
			    magik(i)) {
				/* hack: A few turns of immunity from another attempt! */
				m_ptr->silenced = -k;
				note = " resists the effect";
			} else {
				/* extract and apply duration */
				m_ptr->silenced = k;
				note = " loses psychic energy";
			}
			/* no real HP damage */
			dam = 0; quiet_dam = TRUE;
			break;
		/* PSIONICS */
		case GF_PSI:
			if (seen) obvious = TRUE;
			note_dies = " collapses, a mindless husk,";

			/* Check resist/immunity */
			if ((r_ptr->flags9 & RF9_IM_PSI) || (r_ptr->flags2 & RF2_EMPTY_MIND) ||
			    (r_ptr->flags3 & RF3_NONLIVING))
			{
				note = " is unaffected";
				dam = 0;
				quiet_dam = TRUE;
				break;
			} else if ((r_ptr->flags9 & RF9_RES_PSI) && rand_int(3)) {
				resist = TRUE;
			} else if ((r_ptr->flags3 & RF3_UNDEAD) && rand_int(2)) {
				resist = TRUE;
			} else if ((r_ptr->flags2 & RF2_STUPID) ||
			    ((r_ptr->flags3 & RF3_ANIMAL) && !(r_ptr->flags2 & RF2_CAN_SPEAK))) {
				resist = TRUE;
			} else if (r_ptr->flags2 & RF2_WEIRD_MIND) {
				if (rand_int(5)) resist = TRUE;
				else {
					note = " resists somewhat";
					dam = (dam * 3 + 1) / 4;
				}
			} else if (!rand_int(20)) {
				resist = TRUE;
			}

			/* Check backlash vs caster */
			//if (psi_backlash(Ind, c_ptr->m_idx, dam)) resist = TRUE;

			/* Check susceptibility */
#if 0
/* vs f-im evil: similar dam to ood, somewhat more than (holy) ff (at 0 s.pow).
   however: max mana cost is only 15 instead of 25(ff:30). */
	/* note: could be && !(uni && powerful), but some hi-lv
	   uniques might need powerful flag, strangely lacking it:
	   nodens, mephistopheles, kronos, mouth of sauron */
			if ((r_ptr->flags2 & RF2_SMART) &&
			    !(r_ptr->flags1 & RF1_UNIQUE)) {
			if ((r_ptr->flags2 & RF2_SMART) &&
			    !((r_ptr->flags1 & RF1_UNIQUE) &&
			    (r_ptr->flags2 & RF2_POWERFUL))) {
#endif /* fine for now maybe: */
			if (r_ptr->flags2 & RF2_SMART) {
				if (!resist) {
					note = " is hit hard";
					dam += dam / 2;
				}
			}
			if (((m_ptr->confused > 0) && !rand_int(3)) ||
			    ((m_ptr->confused > 20) && !rand_int(3)) ||
			    ((m_ptr->confused > 50) && !rand_int(3))) {
				resist = FALSE;
				dam = dam * (3 + randint(7)) / 4;
			}

			/* Apply resistance, damage, and effects */
			if (resist) {
				note = " resists";
				dam /= 2;
				if (!(r_ptr->flags3 & RF3_NO_CONF) && !rand_int(10)) do_conf = randint(8);
			} else if (randint(dam > 20 ? 20 : dam) > randint(r_ptr->level)) {
				do_stun = randint(6);
				do_conf = randint(20);
				if (!(r_ptr->flags3 & RF3_NO_SLEEP)) do_sleep = rand_int(2) ? 5 + randint(randint(60)) : 0;
				if (!(r_ptr->flags3 & RF3_NO_FEAR)) do_fear = randint(15);
			}
			break;

		/* Mindcrafter's charm spell, makes monsters ignore you and your teammates (mostly..) */
		case GF_CHARMIGNORE:
		{
			int res = 1;

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;

			/* paranoia */
			if (quiet) break;

			/* don't affect sleeping targets maybe */
			if (m_ptr->csleep) break;

			/* don't affect hurt monsters! */
			if (m_ptr->hp < m_ptr->maxhp) {
				note = " seems too agitated to be charmed";
				break;
			}

			/* not successfully charmed? */
			if ((r_ptr->flags9 & RF9_IM_PSI) ||
			    (r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags3 & RF3_UNDEAD) ||
			    (r_ptr->flags2 & RF2_EMPTY_MIND) ||
			    (r_ptr->flags3 & RF3_NONLIVING)) {
				note = " is unaffected";
				break;
			}
			if (r_ptr->flags2 & RF2_SMART) res <<= 1;
			if (r_ptr->flags3 & RF3_NO_CONF) res <<= 1;
			if (r_ptr->flags1 & RF1_UNIQUE) res <<= 1;
			if (r_ptr->flags2 & RF2_POWERFUL) res <<= 1;
			if (magik(100 - 100 / res)) {
				note = " resists the effect";
				break;
			}

			/* remember who charmed us */
			m_ptr->charmedignore = Ind;

			/* count our victims, just for optimization atm */
			p_ptr->mcharming++;
			note = " seems to forget you were an enemy";
			break;
		}

		/* Earthquake the area */
		case GF_EARTHQUAKE:
			dam = 0;
			break;

			/* Magic Missile -- pure damage */
		case GF_MISSILE:
			if (seen) obvious = TRUE;
			break;

		/* Acid */
		case GF_ACID:
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_ACID) {
				note = " is immune";
				dam = 0;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_IM_ACID;
#endif
			} else if (r_ptr->flags9 & RF9_RES_ACID) {
				note = " resists";
				dam /= 4;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= RF9_RES_ACID;
#endif
			} else if (r_ptr->flags9 & RF9_SUSCEP_ACID) {
				note = " is hit hard";
				dam *= 2;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= RF9_SUSCEP_ACID;
#endif
			}
			break;

		/* Electricity */
		case GF_ELEC:
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_ELEC) {
				note = " is immune";
				dam = 0;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_IM_ELEC;
#endif
			} else if (r_ptr->flags9 & RF9_RES_ELEC) {
				note = " resists";
				dam /= 4;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= RF9_RES_ELEC;
#endif
			} else if (r_ptr->flags9 & RF9_SUSCEP_ELEC) {
				note = " is hit hard";
				dam *= 2;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= RF9_SUSCEP_ELEC;
#endif
			}
			break;

		/* Fire damage */
		case GF_FIRE:
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_FIRE) {
				note = " is immune";
				dam = 0;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_IM_FIRE;
#endif
			} else if (r_ptr->flags9 & RF9_RES_FIRE) {
				note = " resists";
				dam /= 4;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= RF9_RES_FIRE;
#endif
			} else if (r_ptr->flags3 & RF3_SUSCEP_FIRE) {
				note = " is hit hard";
				dam *= 2;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_SUSCEP_FIRE;
#endif
			}
			break;

		/* Cold */
		case GF_COLD:
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_COLD) {
				note = " is immune";
				dam = 0;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_IM_COLD;
#endif
			} else if (r_ptr->flags9 & RF9_RES_COLD) {
				note = " resists";
				dam /= 4;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= RF9_RES_COLD;
#endif
			} else if (r_ptr->flags3 & RF3_SUSCEP_COLD) {
				note = " is hit hard";
				dam *= 2;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_SUSCEP_COLD;
#endif
			}
			break;

		/* Poison */
		case GF_POIS:
			if (seen) obvious = TRUE;
			if ((r_ptr->flags3 & RF3_IM_POIS) ||
			    (r_ptr->flags3 & (RF3_NONLIVING)) || (r_ptr->flags3 & (RF3_UNDEAD)) ||
			    (r_ptr->d_char == 'A') || (r_ptr->d_char == 'E') || ((r_ptr->d_char == 'U') && (r_ptr->flags3 & RF3_DEMON))) {
				note = " is immune";
				dam = 0;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_IM_POIS;
#endif
			} else if (r_ptr->flags9 & RF9_RES_POIS) {
				note = " resists";
				dam /= 4;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= RF9_RES_POIS;
#endif
			} else if (r_ptr->flags9 & RF9_SUSCEP_POIS) {
				note = " is hit hard";
				dam *= 2;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= RF9_SUSCEP_POIS;
#endif
			}
			break;

		/* Thick Poison */
		case GF_UNBREATH:
			if (seen) obvious = TRUE;
			//if (magik(15)) do_pois = (10 + randint(11) + r) / (r + 1);
			if ((r_ptr->flags3 & (RF3_NONLIVING)) || (r_ptr->flags3 & (RF3_UNDEAD)) ||
			    (r_ptr->d_char == 'A') || (r_ptr->d_char == 'E') || ((r_ptr->d_char == 'U') && (r_ptr->flags3 & RF3_DEMON)) ||
			    (m_ptr->r_idx == RI_MORGOTH)) {
				note = " is immune";
				dam = 0;
				//do_pois = 0;
			} else if (r_ptr->flags3 & RF3_IM_POIS) {
				note = " resists";
				dam = (dam * 2) / 4;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_IM_POIS;
#endif
			} else if (r_ptr->flags9 & RF9_RES_POIS) {
				note = " resists somewhat";
				dam = (dam * 3) / 4;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= RF9_RES_POIS;
#endif
			}
#if 0
			else if (r_ptr->flags9 & RF9_SUSCEP_POIS) {
				note = " is hit hard";
				dam *= 2;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= RF9_SUSCEP_POIS;
#endif
			}
#endif
			break;

		case GF_HELL_FIRE:
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & (RF3_GOOD)) {
				if (r_ptr->flags3 & RF3_IM_FIRE) {
					note = " resists";
					dam *= 2; dam /= 2;//(randint(4)+3);
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_IM_FIRE;
#endif
				} else if (r_ptr->flags9 & RF9_RES_FIRE) {
					note = " is hit";
					dam = (dam * 3) / 2;
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags9 |= RF9_RES_FIRE;
#endif
				}
#if 0
				else if (r_ptr->flags3 & RF3_SUSCEP_FIRE) {
					note = " is hit hard";
					dam *= 2;
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_SUSCEP_FIRE;
#endif
				}
#endif
				else {
					dam *= 2;
					note = " is hit hard";
					//note = " is hit";
				}
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= (RF3_GOOD);
#endif
			} else {
				if (r_ptr->flags3 & RF3_IM_FIRE) {
					note = " resists a lot";
					dam *= 2; dam /= 4;//(randint(6)+10);
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_IM_FIRE;
#endif
				}
#if 1
				else if (r_ptr->flags3 & RF3_SUSCEP_FIRE) {
					note = " is hit hard";
					dam *= 2;
 #ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_SUSCEP_FIRE;
 #endif
				}
#endif
				else if ((r_ptr->flags9 & RF9_RES_FIRE) || (r_ptr->flags3 & RF3_DEMON)) {
					note = " resists somewhat";
					dam = (dam * 3) / 4;
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags9 |= RF9_RES_FIRE;
#endif
				}
				else {
					//note = " is hit";
					//dam *= 5; dam /= (randint(3)+4);
				}
			}
			//if (r_ptr->flags3 & (RF3_EVIL)) dam = (dam * 2) / 3;
			break;

		/* Holy Orb -- hurts Evil */
		case GF_HOLY_ORB:
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & (RF3_GOOD)) {
				dam = 0;
				note = " is immune";
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= (RF3_GOOD);
#endif
			}
			if (r_ptr->flags3 & RF3_EVIL) {
				note = " is hit hard";
				dam *= 2;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_EVIL;
#endif
			}
			break;

		/* Holy Fire -- hurts Evil, Good are immune, others _resist_ --
		   note: the holy component strongly outweighs the fire component */
		case GF_HOLY_FIRE:
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & (RF3_GOOD)) {
				dam = 0;
				note = " is immune";
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= (RF3_GOOD);
#endif
			} else if (r_ptr->flags3 & (RF3_EVIL)) {
				if (r_ptr->flags3 & RF3_IM_FIRE) {
					note = " resists";
					dam = (dam * 4) / 3;//(randint(4)+3);
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_IM_FIRE;
#endif
				} else if (r_ptr->flags9 & RF9_RES_FIRE) {
					note = " is hit";
					dam = (dam * 3) / 2;
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags9 |= RF9_RES_FIRE;
#endif
				}
#if 0
				else if (r_ptr->flags3 & RF3_SUSCEP_FIRE) {
					note = " is hit hard";
					dam *= 2;
 #ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_SUSCEP_FIRE;
 #endif
				}
#endif
				else {
					dam *= 2;
					note = " is hit hard";
					//note = " is hit";
				}
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= (RF3_EVIL);
#endif
			} else {
				if (r_ptr->flags3 & RF3_IM_FIRE) {
					note = " resists a lot";
					dam *= 2; dam /= 4;//(randint(6)+10);
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_IM_FIRE;
#endif
				} else if (r_ptr->flags9 & RF9_RES_FIRE) {
					note = " resists";
					dam = (dam * 3) / 4;
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags9 |= RF9_RES_FIRE;
#endif
				}
#if 1
				else if (r_ptr->flags3 & RF3_SUSCEP_FIRE) {
					note = " is hit hard";
					dam = (dam * 3) / 2;
 #ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_SUSCEP_FIRE;
 #endif
				}
#endif
				else {
					//note = " resists somewhat";
					//dam *= 5; dam /= (randint(3)+4);
				}
			}
			break;

		/* Arrow -- XXX no defense */
		case GF_ARROW:
			if (seen) obvious = TRUE;
			break;

		/* Plasma -- Fire/Elec/Force */
		case GF_PLASMA: {
			if (seen) obvious = TRUE;

			/* 50% fire */
			k = (dam + 1) / 2;
			if (r_ptr->flags3 & RF3_IM_FIRE) k = 0;
			else if (r_ptr->flags9 & RF9_RES_FIRE) k = (k + 3) / 4;
			else if (r_ptr->flags3 & RF3_SUSCEP_FIRE) k *= 2;
			/* 25% elec */
			k_elec = (dam + 3) / 4;
			if (r_ptr->flags3 & RF3_IM_ELEC) k = 0;
			else if (r_ptr->flags9 & RF9_RES_ELEC) k = (k + 3) / 4;
			else if (r_ptr->flags9 & RF9_SUSCEP_ELEC) k *= 2;
			/* 25% force */
			k_sound = (dam + 3) / 4;
			if ((r_ptr->flags4 & RF4_BR_SOUN) || (r_ptr->flags9 & RF9_RES_SOUND)) k_sound = (k + 1) / 2;
			else if (!(r_ptr->flags3 & RF3_NO_STUN)) do_stun = randint(15) / div;

			k += k_elec + k_sound;
			if (k < dam / 4) note = " resists a lot";
			else if (k < dam / 2) note = " resists";
			else if (k < dam) note = " resists somewhat";
			else if (k > dam) note = " is hit hard";
			dam = k;
			break;
		}

		/* Nether -- see above */
		case GF_NETHER_WEAK:
		case GF_NETHER:
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_UNDEAD) {
				note = " is immune";
				dam = 0;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_UNDEAD;
#endif
			} else if ((r_ptr->flags4 & RF4_BR_NETH) || (r_ptr->flags3 & RF3_RES_NETH)) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);
			}
			//else if (r_ptr->flags3 & RF3_EVIL)
			else if (r_ptr->flags3 & RF3_DEMON) {
				dam /= 2;
				note = " resists somewhat";
#ifdef OLD_MONSTER_LORE
				//if (seen) r_ptr->r_flags3 |= RF3_EVIL;
				if (seen) r_ptr->r_flags3 |= RF3_DEMON;
#endif
			}
			break;

		/* Water (acid) damage -- Water spirits/elementals are immune */
		case GF_WATER:
		case GF_VAPOUR:
			if (seen) obvious = TRUE;
			if (r_ptr->flags9 & RF9_IM_WATER) {
				note = " is immune";
				dam = 0;
			} else if (r_ptr->flags7 & RF7_AQUATIC) {
				note = " resists a lot";
				dam /= 9;
			} else if (r_ptr->flags3 & RF3_RES_WATE) {
				note = " resists";
				dam /= 4;
			}
			break;

		/* Wave = Water + Force */
		case GF_WAVE:
			if (seen) obvious = TRUE;
			if (r_ptr->flags9 & RF9_IM_WATER) {
				note = " is immune";
				dam = 0;
			} else if (r_ptr->flags7 & RF7_AQUATIC) {
				note = " resists a lot";
				dam /= 9;
			} else if (r_ptr->flags3 & RF3_RES_WATE) {
				note = " resists";
				dam /= 4;
			} else do_stun = randint(15) / div;
			break;

		/* Chaos -- Chaos breathers resist */
		case GF_CHAOS:
			if (seen) obvious = TRUE;
			if (rand_int(r_ptr->level) < 15) do_poly = TRUE;
			do_conf = (5 + randint(11)) / div;
			if ((r_ptr->flags4 & RF4_BR_CHAO) || (r_ptr->flags9 & RF9_RES_CHAOS)) {
				note = " resists";
				dam *= 3; dam /= randint(6) + 6;
				do_poly = FALSE;
			}
			break;

		/* Shards -- Shard breathers resist */
		case GF_SHARDS:
			if (seen) obvious = TRUE;
			if ((r_ptr->flags4 & RF4_BR_SHAR) || (r_ptr->flags9 & RF9_RES_SHARDS)) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);
			} else if (r_ptr->flags8 & RF8_NO_CUT) {
				note = " resists somewhat";
				dam /= 2;
			}
			break;

		case GF_HAVOC: {
			/* Inferno/Mana/Chaos */
			int res1 = 0, res2 = 0, res3 = 0, res4 = 0, res5 = 0; //shard,sound,fire,mana,chaos

			if ((r_ptr->flags4 & RF4_BR_SHAR) || (r_ptr->flags9 & RF9_RES_SHARDS))
				res1 = 2;
			//RF8_NO_CUT/RF3_NO_STUN don't help here
			if ((r_ptr->flags4 & RF4_BR_SOUN)  || (r_ptr->flags9 & RF9_RES_SOUND))
				res2 = 2;
			if (r_ptr->flags3 & RF3_IM_FIRE)
				res3 = 4;
			else if (r_ptr->flags9 & RF9_RES_FIRE)
				res3 = 2;
			//no SUSCEP_FIRE check
			if ((r_ptr->flags4 & RF4_BR_MANA) || (r_ptr->flags9 & RF9_RES_MANA))
				res4 = 3;
			if ((r_ptr->flags4 & RF4_BR_CHAO) || (r_ptr->flags9 & RF9_RES_CHAOS))
				res5 = 3;

			switch (res1 + res2 + res3 + res4 + res5) {
			case 0: case 1: case 2: case 3: break;
			case 4: case 5: case 6: case 7:
				note = " resists somewhat";
				dam = (dam * 3 + 3) / 4;
				//do_cut = 0;
				break;
			case 8: case 9: case 10:
				note = " resists";
				dam /= 2;
				break;
			default: //11,12,13,14
				note = " resists a lot";
				dam /= 3;
				break;
			}
			if (seen) obvious = TRUE;
			break; }

		/* Rocket: Shard resistance helps (PernA) */
		//(Note that the sound part doesn't cause any stun effect. - intended) */
		case GF_INFERNO:
		case GF_DETONATION:
		case GF_ROCKET: {
			int res1 = 0, res2 = 0, res3 = 0; //shard,sound,fire

			if ((r_ptr->flags4 & RF4_BR_SHAR) || (r_ptr->flags9 & RF9_RES_SHARDS))
				res1 = 1;
			//RF8_NO_CUT/RF3_NO_STUN don't help here
			if ((r_ptr->flags4 & RF4_BR_SOUN)  || (r_ptr->flags9 & RF9_RES_SOUND))
				res2 = 1;
			if (r_ptr->flags3 & RF3_IM_FIRE)
				res3 = 3;
			else if (r_ptr->flags9 & RF9_RES_FIRE)
				res3 = 1;
			//no SUSCEP_FIRE check

			switch (res1 + res2 + res3) {
			case 0: break;
			case 1: case 2:
				note = " resists somewhat";
				dam = (dam * 3 + 3) / 4;
				//do_cut = 0;
				break;
			default:
				note = " resists";
				dam /= 2;
				break;
			}
			if (seen) obvious = TRUE;
			break; }

		/* Sound -- Sound breathers resist */
		case GF_STUN:
			do_stun = (10 + randint(15)) / div;
			break;

		/* Sound -- Sound breathers resist */
		case GF_SOUND:
			if (seen) obvious = TRUE;
			do_stun = randint(15) / div; //RF3_NO_STUN is checked later
			if ((r_ptr->flags4 & RF4_BR_SOUN) || (r_ptr->flags9 & RF9_RES_SOUND)) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);
			}
			break;

		/* Confusion */
		case GF_CONFUSION:
			if (seen) obvious = TRUE;
			do_conf = (10 + randint(15)) / div;
			if ((r_ptr->flags4 & RF4_BR_CONF) ||
			    (r_ptr->flags4 & RF4_BR_CHAO) || (r_ptr->flags9 & RF9_RES_CHAOS)) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);
			} else if (r_ptr->flags3 & RF3_NO_CONF) {
				note = " resists somewhat";
				dam /= 2;
			}
			break;

		/* Disenchantment -- Breathers and Disenchanters resist */
		case GF_DISENCHANT:
			if (seen) obvious = TRUE;
			if ((r_ptr->flags4 & RF4_BR_DISE) || prefix(name, "Disen") || (r_ptr->flags3 & RF3_RES_DISE)) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);
			}
			break;

		/* Nexus -- Breathers and Existers resist */
		case GF_NEXUS:
			if (seen) obvious = TRUE;
			if ((r_ptr->flags4 & RF4_BR_NEXU) || prefix(name, "Nexus") || (r_ptr->flags3 & RF3_RES_NEXU)) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);
			}
			break;

		/* Force */
		case GF_FORCE:
			if (seen) obvious = TRUE;
			do_stun = randint(15) / div;
			if (r_ptr->flags4 & RF4_BR_WALL) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);
			}
			break;

		/* Inertia -- breathers resist */
		case GF_INERTIA: //Slowing effect -- NOTE: KEEP CONSISTENT WITH GF_CURSE AND GF_OLD_SLOW
			if (seen) obvious = TRUE;
			if (r_ptr->flags4 & RF4_BR_INER) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);
			}
			/* Powerful monsters can resist */
			else if (r_ptr->flags1 & RF1_UNIQUE) {
			} else if (r_ptr->level > ((dam - 10) < 1 ? 1 : (dam - 10)) + 10) { /* cannot randint higher? (see 'resist' branch below) */
			} else if (RES_OLD(r_ptr->level, dam)) {
			} else if (m_ptr->mspeed >= 100 && m_ptr->mspeed > m_ptr->speed - 10) /* Normal monsters slow down */
			//else if (m_ptr->mspeed >= 100) /* Normal monsters slow down */
			{
				//if (m_ptr->mspeed > 100) m_ptr->mspeed -= 10;
				m_ptr->mspeed -= 10;
				note = " starts moving slower";
			}

			break;

		/* Time -- breathers resist */
		case GF_TIME:
			if (seen) obvious = TRUE;
			if ((r_ptr->flags4 & RF4_BR_TIME) || (r_ptr->flags9 & RF9_RES_TIME)
			    || (r_ptr->flags3 & RF3_DEMON) || (r_ptr->flags3 & RF3_NONLIVING)
			    || (r_ptr->flags3 & RF3_UNDEAD) || (r_ptr->d_char == 'A')) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);
			}
#if 1 /* fixed & sanified */
			else if (!quiet && rand_int(3) == 0) { /* only occur if a player cast this */
				long t = m_ptr->hp / 10, tp = damroll(2, plev);

				note = " loses precious seconds to you";
				m_ptr->energy -= m_ptr->energy / 4;

				if (!quiet) {
					if (t > dam) t = dam;
					if (t > tp) t = tp;
					p_ptr->energy += (t * level_speed(&p_ptr->wpos)) / 500;
					/* Prevent too much energy, preventing overflow too. */
					limit_energy(p_ptr);
				}
			}
#endif
			break;

		/* Gravity -- breathers resist */
		case GF_GRAVITY:
		{
			bool resist_tele = FALSE;
			//dun_level *l_ptr = getfloor(wpos);

			if (seen) obvious = TRUE;

			if ((r_ptr->flags3 & RF3_RES_TELE) || (r_ptr->flags9 & RF9_IM_TELE)) {
				if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flags9 & RF9_IM_TELE)) {
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
#endif
					note = " resists";
					resist_tele = TRUE;
				} else if (m_ptr->level > randint(100)) {
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
#endif
					note = " resists";
					resist_tele = TRUE;
				}
			}

			if (!resist_tele) do_dist = 10;
			else do_dist = 0;

			if ((r_ptr->flags4 & RF4_BR_GRAV) ||
			    (r_ptr->flags7 & RF7_CAN_FLY)) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);
				do_dist = 0;
			}

			//todo maybe: stun effect? might be op?

			break;
		}

		/* Pure damage */
		case GF_MANA:
			if (r_ptr->flags9 & RF9_RES_MANA) {
				dam /= 3;
				note = " resists";
			} else if (r_ptr->flags4 & RF4_BR_MANA) {
				dam /= 2;
				note = " resists somewhat";
			}
			if (seen) obvious = TRUE;
			break;

			/* Meteor -- powerful magic missile */
		case GF_METEOR:
			if (seen) obvious = TRUE;
			do_stun = randint(15) / div;
			break;

		/* Ice -- Cold + Cuts + Stun */
		case GF_ICE:
			if (seen) obvious = TRUE;
			//do_stun = randint(15) / div;
			k = dam;

			dam = (k * 3) / 5;/* 60% COLD damage */
			if (r_ptr->flags3 & RF3_IM_COLD) {
				note = " is immune to cold";
				dam = 0;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_IM_COLD;
#endif
			} else if (r_ptr->flags9 & RF9_RES_COLD) {
				note = " resists cold";
				dam /= 4;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= RF9_RES_COLD;
#endif
			} else if (r_ptr->flags3 & RF3_SUSCEP_COLD) {
				note = " is hit hard by cold";
				dam *= 2;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_SUSCEP_COLD;
#endif
			}

			k = (k * 2) / 5;/* 40% SHARDS damage */
			if ((r_ptr->flags4 & RF4_BR_SHAR) || (r_ptr->flags9 & RF9_RES_SHARDS)) {
				//note = " resists";
				k = (k * 3) / (randint(6) + 6);
			} else if (r_ptr->flags8 & RF8_NO_CUT) {
				//note = " resists somewhat";
				k /= 2;
			}

			dam = dam + k;
			break;

		/* Thunder -- Elec + Sound + Light */
		case GF_THUNDER:
			if (seen) obvious = TRUE;

			k_elec = dam / 3; /* 33% ELEC damage */
			if (r_ptr->flags3 & RF3_IM_ELEC) {
				note = " is immune to lightning";
				k_elec = 0;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_IM_ELEC;
#endif
			} else if (r_ptr->flags9 & RF9_RES_ELEC) {
				note = " resists lightning";
				k_elec /= 4;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= RF9_RES_ELEC;
#endif
			} else if (r_ptr->flags9 & RF9_SUSCEP_ELEC) {
				note = " is hit hard by lightning";
				k_elec *= 2;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= RF9_SUSCEP_ELEC;
#endif
			}

			k_sound = dam / 3; /* 33% SOUND damage */
			do_stun = randint(15) / div;
			if ((r_ptr->flags4 & RF4_BR_SOUN) || (r_ptr->flags9 & RF9_RES_SOUND)) {
				//note = " resists";
				k_sound *= 3;
				k_sound /= (randint(6) + 6);
			}

			k_lite = dam / 3; /* 33% LIGHT damage */
			do_blind = damroll(3, (k_lite / 20)) + 1;
			if (r_ptr->d_char == 'A') {
				//note = " is immune";
				k_lite = do_blind = 0;
			} else if (r_ptr->flags3 & RF3_HURT_LITE) {
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_HURT_LITE;
#endif
				//note = " cringes from the light";
				//note_dies = " shrivels away in the light";
				dam *= 2;
				if (r_ptr->flags2 & RF2_REFLECTING) dam /= 2;
			} else if ((r_ptr->flags4 & RF4_BR_LITE) || (r_ptr->flags9 & RF9_RES_LITE) || (r_ptr->flags2 & RF2_REFLECTING)) {
				//note = " resists";
				k_lite *= 2;
				k_lite /= (randint(6) + 6);
				do_blind = 0;
			}

			dam = k_elec + k_sound + k_lite;
			break;

		/* Drain Life */
		case GF_OLD_DRAIN:
			if (seen) obvious = TRUE;

			/* Prevent internal overflow (int) - allow up to 35% leech */
			if (m_ptr->hp > 9362) dam = (m_ptr->hp / 100) * dam;
			else if (m_ptr->hp > 936) dam = ((m_ptr->hp / 10) * dam) / 10;
			else dam = (m_ptr->hp * dam) / 100;

			/* Make it have an effect on low-HP monsters such as townies */
			if (!dam) dam = 1;
			/* Cap */
			if (dam > 900) dam = 900;

			if (!quiet) {
				if (typ == GF_OLD_DRAIN) p_ptr->ret_dam = dam;
				else p_ptr->ret_dam = 0; /* paranoia */
			}

			if ((r_ptr->flags3 & RF3_UNDEAD) ||
			    //(r_ptr->flags3 & RF3_DEMON) ||
			    (r_ptr->flags3 & RF3_NONLIVING) ||
			    strchr("Egv", r_ptr->d_char)) {
#ifdef OLD_MONSTER_LORE
				if ((r_ptr->flags3 & RF3_UNDEAD) && seen) r_ptr->r_flags3 |= RF3_UNDEAD;
				//if ((r_ptr->flags3 & RF3_DEMON) && seen) r_ptr->r_flags3 |= RF3_DEMON;
				if ((r_ptr->flags3 & RF3_NONLIVING) && seen) r_ptr->r_flags3 |= RF3_NONLIVING;
#endif
				note = " is unaffected";
				obvious = FALSE;
				dam = 0;
				quiet_dam = TRUE;
				if (!quiet) p_ptr->ret_dam = 0;
			} else if (r_ptr->flags1 & RF1_UNIQUE) {
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags1 |= RF1_UNIQUE;
#endif
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);

				/* Make it have an effect on low-HP monsters such as townies */
				if (!dam) dam = 1;
			}

			/* the_sandman: return 15% of the damage to player. This is special
			   since (we're not using the upto 35% rule because for the spell
			   (drain cloud) it will be too much) we aim for balance
			   HACK: the priest_spell variable is defined above.
			*/
			if (priest_spell) {
				//msg_format(Ind, "\377gYou are healed for %d hit points", (dam * 15) / 100);
				if (dam && !quiet) hp_player_quiet(Ind, (dam * 15) / 100, TRUE);
				p_ptr->ret_dam = 0;
			}

			break;

		case GF_ANNIHILATION:
			if (seen) obvious = TRUE;
			if (m_ptr->hp > 9362) dam = (m_ptr->hp / 100) * dam;
			else if (m_ptr->hp > 936) dam = ((m_ptr->hp / 10) * dam) / 10;
			else dam = (m_ptr->hp * dam) / 100;

			if (r_ptr->flags1 & RF1_UNIQUE) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);
			}

			/* Make it have an effect on low-HP monsters such as townies */
			if (!dam) dam = 1;
			/* Cap */
			if (brief_rune_spell) {
				if (dam > 600) dam = 600;
			} else {
				if (dam > 1200) dam = 1200;
			}
			break;

		/* Polymorph monster (Use "dam" as "power") */
		case GF_OLD_POLY:
			if (seen) obvious = TRUE;

			/* Attempt to polymorph (see below) */
			do_poly = TRUE;

			/* Powerful monsters can resist */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10)) {
				note = " is unaffected";
				do_poly = FALSE;
				obvious = FALSE;
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;

			break;

		/* Clone monsters (Ignore "dam") */
		case GF_OLD_CLONE:
			if (r_ptr->flags7 & RF7_NO_DEATH) { /* don't haste these.. */
				note = " is unaffected";
			} else {
				if (seen) obvious = TRUE;

				/* Heal fully */
				m_ptr->hp = m_ptr->maxhp;

				/* Speed up */
				if (m_ptr->mspeed < 150) m_ptr->mspeed += 10;

				/* Attempt to clone. */
				if (multiply_monster(c_ptr->m_idx)) note = " spawns";
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;

			break;

		/* Heal Monster (use "dam" as amount of healing) */
		case GF_OLD_HEAL:
			if (seen) obvious = TRUE;

			/* Wake up */
			m_ptr->csleep = 0;
			/* Heal */
			m_ptr->hp += dam;
			/* No overflow */
			if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

			/* Redraw (later) if needed */
			update_health(c_ptr->m_idx);

			/* Message */
			note = " looks healthier";

			/* :) */
			if (m_ptr->r_idx == RI_LEPER) {
				int clone = m_list[c_ptr->m_idx].clone, clone_summoning = m_list[c_ptr->m_idx].clone_summoning;
				delete_monster_idx(c_ptr->m_idx, TRUE);
				switch (rand_int(10)) {
				case 0: i = 1; break;
				case 1: i = 6; break;
				case 2: i = 9; break;
				case 3: i = 10; break;
				case 4: i = 11; break;
				case 5: i = 12; break;
				case 6: i = 14; break;
				case 7: i = 16; break;
				case 8: i = 17; break;
				case 9: i = 18; break;
				}
				(void)place_monster_aux(wpos, y, x, i, FALSE, FALSE, clone, clone_summoning);
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;

		/* Heroism/Berserk Strength for monsters */
		case GF_HERO_MONSTER:
			if (seen) obvious = TRUE;

			/* Wake up */
			m_ptr->csleep = 0;
			/* Heal */
			m_ptr->hp += dam;
			/* No overflow */
			if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

			/* Redraw (later) if needed */
			update_health(c_ptr->m_idx);

			/* Message */
			note = " looks healthier";

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;

			/* No 'break;' here =) */

		/* Remove monster's fear */
		case GF_REMFEAR:
			if (m_ptr->monfear) {
				m_ptr->monfear = 0;
				/* Message */
				note = " becomes courageous again";
				if (seen) obvious = TRUE;
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;

		/* Speed Monster (Ignore "dam") */
		case GF_OLD_SPEED:
			if ((r_ptr->flags7 & RF7_NO_DEATH) || m_ptr->mspeed >= 150) {
				note = " is unaffected";
			} else {
				if (seen) obvious = TRUE;

				/* Speed up */
				m_ptr->mspeed += 10;
				note = " starts moving faster";
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;


		/* Slow Monster (Use "dam" as "power") */
		case GF_OLD_SLOW: //Slowing effect -- NOTE: KEEP CONSISTENT WITH GF_INERTIA AND GF_CURSE
			if (seen) obvious = TRUE;

			/* Powerful monsters can resist */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags4 & RF4_BR_INER)) {
				note = " is unaffected";
				obvious = FALSE;
			} else if (r_ptr->level > ((dam - 10) < 1 ? 1 : (dam - 10)) + 10) { /* cannot randint higher? (see 'resist' branch below) */
				note = " resists easily"; /* vs damaging it's "resists a lot" and vs effects it's "resists easily" :-o */
				obvious = FALSE;
			} else if (RES_OLD(r_ptr->level, dam)) {
				note = " resists";
				obvious = FALSE;
			}
			else if (m_ptr->mspeed >= 100 && m_ptr->mspeed > m_ptr->speed - 10) /* Normal monsters slow down */
			//else if (m_ptr->mspeed >= 100) /* Normal monsters slow down */
			{
				//if (m_ptr->mspeed > 100) m_ptr->mspeed -= 10;
				m_ptr->mspeed -= 10;
				note = " starts moving slower";
			} else {
				note = " is unaffected";
				obvious = FALSE;
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;

		/* Sleep (Use "dam" as "power") */
		case GF_OLD_SLEEP:
#ifdef ENABLE_OCCULT
			/* hack for Occult school's "Trance" spell */
			if (dam & 0x400) {
				/* fails? (not a ghost, spirit or elemental) */
				if (r_ptr->d_char != 'G' && r_ptr->d_char != 'E' && r_ptr->d_char != 'X') {
					dam = 0;
					quiet_dam = TRUE;
					quiet = TRUE;
					break;
				}
				dam -= 0x400;

				if (RES_OLD(r_ptr->level, dam)) {
					note = " resists";
					if (r_ptr->flags1 & RF1_UNIQUE) note = " is unaffected";
					/* No obvious effect */
					obvious = FALSE;
				} else {
					/* Go to sleep (much) later */
					note = " falls asleep";
					do_sleep = GF_OLD_SLEEP_DUR;
				}
				/* No "real" damage */
				dam = 0;
				quiet_dam = TRUE;
				break;
			}
#endif

			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags3 & RF3_NO_SLEEP) ||
			    RES_OLD(r_ptr->level, dam))
			{
				note = " resists";
				if (r_ptr->flags1 & RF1_UNIQUE) note = " is unaffected";
				/* Memorize a flag */
				if (r_ptr->flags3 & RF3_NO_SLEEP) {
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_NO_SLEEP;
#endif
					note = " is unaffected";
				}

				/* No obvious effect */
				obvious = FALSE;
			} else {
				/* Go to sleep (much) later */
				note = " falls asleep";
				do_sleep = GF_OLD_SLEEP_DUR;
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;

		/* Confusion (Use "dam" as "power") */
		case GF_OLD_CONF:
			if (seen) obvious = TRUE;

			/* Get confused later */
			do_conf = damroll(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags3 & RF3_NO_CONF) ||
			    RES_OLD(r_ptr->level, dam))
			{
				/* No obvious effect */
				note = " resists";
				obvious = FALSE;

				if (r_ptr->flags1 & RF1_UNIQUE) note = " is unaffected";

				/* Memorize a flag */
				if (r_ptr->flags3 & RF3_NO_CONF) {
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_NO_CONF;
#endif
					note = " is unaffected";
				}

				/* Resist */
				do_conf = 0;
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;

		case GF_TERROR:
			if (seen) obvious = TRUE;

			/* Apply some fear */
			do_fear = damroll(3, (dam / 2)) + 1;
			/* Get confused later */
			do_conf = damroll(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    ((r_ptr->flags3 & RF3_NO_CONF) && (r_ptr->flags3 & RF3_NO_FEAR))) {
				/* No obvious effect */
				note = " is unaffected";
				obvious = FALSE;
				do_fear = do_conf = 0;
			} else if (RES_OLD(r_ptr->level, dam)) {
				note = " resists the effect";
				obvious = FALSE;
				do_fear = do_conf = 0;
			} else {
				if (r_ptr->flags3 & RF3_NO_CONF) do_conf = 0;
				if (r_ptr->flags3 & RF3_NO_FEAR) do_fear = 0;
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;

		/* Confusion (Use "dam" as "power") */
		case GF_BLIND:
			if (seen) obvious = TRUE;

			/* Get blinded later */
			do_blind = dam;

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;

		/* The new cursing spell - basically slow/blind/conf all in one. the_sandman */
		/* Following the pattern, use "dam" as "power". */
		case GF_CURSE:
			/* Assume no obvious effect */
			obvious = FALSE;
			int curse = randint(3);
			if (curse == 1) { //Slowing effect -- NOTE: KEEP CONSISTENT WITH GF_INERTIA AND GF_OLD_SLOW
				/*if (((r_ptr->flags1 & RF1_UNIQUE) && magik(r_ptr->level * 4)) ||
					magik(r_ptr->level * 2)) {*/
				if ((r_ptr->flags1 & RF1_UNIQUE) ||
				    (r_ptr->flags4 & RF4_BR_INER)) {
					note = " is unaffected";
				} else if (RES_OLD(r_ptr->level, dam / 3)) {
					note = " resists";
				} else if (m_ptr->mspeed > 100 && m_ptr->mspeed > m_ptr->speed - 10) {
					m_ptr->mspeed -= 10;
					note = " starts moving slower";
					if (seen) obvious = TRUE;
				} else {
					note = " is unaffected";
				}
				dam = 0;
				quiet_dam = TRUE;
				break;
			} else if (curse == 2) { //Conf
				/* Get confused later */
				do_conf = damroll(3, (dam / 2)) + 1;

				/* Attempt a saving throw */
				if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags3 & RF3_NO_CONF) ||
				  RES_OLD(r_ptr->level, dam / 3))
				{
					note = " resists";
					if (r_ptr->flags1 & RF1_UNIQUE) note = " is unaffected";


					/* Memorize a flag */
					if (r_ptr->flags3 & RF3_NO_CONF) {
#ifdef OLD_MONSTER_LORE
						if (seen) r_ptr->r_flags3 |= RF3_NO_CONF;
#endif
						note = " is unaffected";
					}
					/* Resist */
					do_conf = 0;
				}

				//let's do some actual damage, too?
				//dam = 0;
				//quiet_dam = TRUE;
				/* then apply proper confusion resistance damage reduction (same as for GF_CONF) */
				if ((r_ptr->flags4 & RF4_BR_CONF) ||
				    (r_ptr->flags4 & RF4_BR_CHAO) || (r_ptr->flags9 & RF9_RES_CHAOS)) {
					dam *= 3; dam /= (randint(6) + 6);
				} else if (r_ptr->flags3 & RF3_NO_CONF)
					dam /= 2;
				break;
			} else { //Blind
				do_blind = dam;
				dam = 0;
				quiet_dam = TRUE;
				/* No obvious effect */
				obvious = FALSE;
			}
			break;

		/* Healing Cloud damages undead beings - the_sandman */
		case GF_HEALINGCLOUD:
			if (r_ptr->flags3 & RF3_UNDEAD) {
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_UNDEAD;
#endif
				if (seen) obvious = TRUE;
				note = " crackles in the light";
				note_dies = " evaporates into thin air";
			} else {
				dam = 0;
				quiet = TRUE;
			}
			break;

		/* Vapour + poison ownage - the_sandman */
		case GF_WATERPOISON:
			switch (randint(2)) {
			case 1: // Poison!
				if (seen) obvious = TRUE;
				if ((r_ptr->flags3 & RF3_IM_POIS) ||
				  (r_ptr->flags3 & (RF3_NONLIVING)) || (r_ptr->flags3 & (RF3_UNDEAD)) ||
				  (r_ptr->d_char == 'A') || (r_ptr->d_char == 'E') || ((r_ptr->d_char == 'U') && (r_ptr->flags3 & RF3_DEMON))) {
					note = " is immune";
					dam = 0;
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_IM_POIS;
#endif
				} else if (r_ptr->flags9 & RF9_RES_POIS) {
					note = " resists";
					dam /= 4;
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->flags9 |= RF9_RES_POIS;
#endif
				} else if (r_ptr->flags9 & RF9_SUSCEP_POIS) {
					note = " is hit hard";
					dam *= 2;
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->flags9 |= RF9_SUSCEP_POIS;
#endif
				}
				break;
			default: // Water
				if (seen) obvious = TRUE;
				if (r_ptr->flags9 & RF9_IM_WATER) {
					note = " is immune";
					dam = 0;
				} else if (r_ptr->flags7 & RF7_AQUATIC) {
					note = " resists a lot";
					dam /= 9;
				} else if (r_ptr->flags3 & RF3_RES_WATE) {
					note = " resists";
					dam /= 4;
				}
			}
			break;

		/* Random between ice (shards+water) and poison. At the moment its 3:1:1 shards:water:poison chance 
				- the_sandman */
		case GF_ICEPOISON:
			switch (randint(5)) {
			case 1:  // Shards
			case 3:
			case 5:
				if (seen) obvious = TRUE;
				if ((r_ptr->flags4 & RF4_BR_SHAR) || (r_ptr->flags9 & RF9_RES_SHARDS)) {
					note = " resists";
					dam *= 3; dam /= (randint(6) + 6);
				} else if (r_ptr->flags8 & RF8_NO_CUT) {
					note = " resists somewhat";
					dam /= 2;
				}
				break;
			case 2: // Water
				if (seen) obvious = TRUE;
				if (r_ptr->flags9 & RF9_IM_WATER) {
					note = " is immune";
					dam = 0;
				} else if (r_ptr->flags7 & RF7_AQUATIC) {
					note = " resists a lot";
					dam /= 9;
				} else if (r_ptr->flags3 & RF3_RES_WATE) {
					note = " resists";
					dam /= 4;
				}
				break;
			default: // Poison
				if (seen) obvious = TRUE;
				if ((r_ptr->flags3 & RF3_IM_POIS) ||
				  (r_ptr->flags3 & (RF3_NONLIVING)) || (r_ptr->flags3 & (RF3_UNDEAD)) ||
				  (r_ptr->d_char == 'A') || (r_ptr->d_char == 'E') || ((r_ptr->d_char == 'U') && (r_ptr->flags3 & RF3_DEMON)))
				{
					note = " is immune";
					dam = 0;
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_IM_POIS;
#endif
				} else if (r_ptr->flags9 & RF9_RES_POIS) {
					note = " resists";
					dam /= 4;
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags9 |= RF9_RES_POIS;
#endif
				} else if (r_ptr->flags9 & RF9_SUSCEP_POIS) {
					note = " is hit hard";
					dam *= 2;
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags9 |= RF9_SUSCEP_POIS;
#endif
				}
			}
			break;

		/* Lite, but only hurts susceptible creatures */
		case GF_LITE_WEAK:
			/* Hurt by light */
			if (r_ptr->flags3 & RF3_HURT_LITE) {
				/* Obvious effect */
				if (seen) obvious = TRUE;

#ifdef OLD_MONSTER_LORE
				/* Memorize the effects */
				if (seen) r_ptr->r_flags3 |= RF3_HURT_LITE;
#endif

				/* Special effect */
				note = " cringes from the light";
				note_dies = " shrivels away in the light";

				/* Specialty: Reflecting monsters take reduced damage from light, even if it was a ball or beam style attack! */
				if (r_ptr->flags2 & RF2_REFLECTING) dam /= 2;
			}
			/* Normally no damage */
			else {
				/* No damage */
				dam = 0;
				quiet = TRUE;
			}
			break;

		/* Lite -- opposite of Dark */
		case GF_LITE:
		case GF_STARLITE:
			if (seen) obvious = TRUE;

			/* Get blinded later */
			do_blind = damroll(3, (dam / 20)) + 1;

			if (r_ptr->d_char == 'A') {
				do_blind = 0;
				if (r_ptr->flags3 & RF3_EVIL) break; /* normal damage: fallen/corrupted angel */
				note = " is immune";
				dam = 0;
			} else if (r_ptr->flags3 & RF3_HURT_LITE) {
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_HURT_LITE;
#endif
				note = " cringes from the light";
				note_dies = " shrivels away in the light";
				dam *= 2;
				if (r_ptr->flags2 & RF2_REFLECTING) dam /= 2;
			} else if ((r_ptr->flags4 & RF4_BR_LITE) || (r_ptr->flags9 & RF9_RES_LITE) || (r_ptr->flags2 & RF2_REFLECTING)) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);
				do_blind = 0;
			}
			break;

		case GF_DARK_WEAK:
			if (seen) obvious = TRUE;

			/* Exception: Bolt-type spells have no special effect */
			if (flg & (PROJECT_NORF | PROJECT_JUMP)) {
				/* Get blinded later */
				if (dark_spell) do_blind = dam; //occult shadow spell
				else do_blind = damroll(3, (dam / 20)) + 1; //normal
			}

			if ((r_ptr->flags4 & RF4_BR_DARK) || (r_ptr->flags9 & RF9_RES_DARK)
			    || (r_ptr->flags3 & RF3_UNDEAD)) {
				note = " resists";
				do_blind = 0;
			}

			//TODO: Light Hounds could get damaged. Currently, all GF_DARK_WEAK causes have been changed to do GF_DARK though, anyway.
			dam = 0;
			quiet_dam = TRUE;
			break;

		case GF_DARK:
			//TODO: Light Hounds could be susceptible.
			if (seen) obvious = TRUE;

			/* Exception: Bolt-type spells have no special effect */
			if (flg & (PROJECT_NORF | PROJECT_JUMP)) {
				/* Get blinded later */
				do_blind = damroll(3, (dam / 20)) + 1;
			}

			if ((r_ptr->flags4 & RF4_BR_DARK) || (r_ptr->flags9 & RF9_RES_DARK)
			    || (r_ptr->flags3 & RF3_UNDEAD)) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6);
				do_blind = 0;
			}
			break;

		/* Stone to Mud */
		case GF_KILL_WALL:
			/* Hurt by rock remover */
			if (r_ptr->flags3 & RF3_HURT_ROCK) {
				/* Notice effect */
				if (seen) obvious = TRUE;

#ifdef OLD_MONSTER_LORE
				/* Memorize the effects */
				if (seen) r_ptr->r_flags3 |= RF3_HURT_ROCK;
#endif

				/* Cute little message */
				note = " loses some skin";
				note_dies = " dissolves";
			}

			/* Usually, ignore the effects */
			else {
				/* No damage */
				dam = 0;
				quiet = TRUE;
			}
			break;

		/* Teleport undead (Use "dam" as "power") -- unused */
		case GF_AWAY_UNDEAD:
			/* Only affect undead */
			if (r_ptr->flags3 & (RF3_UNDEAD)) {
				bool resists_tele = FALSE;

				if ((r_ptr->flags3 & (RF3_RES_TELE)) || (r_ptr->flags9 & RF9_IM_TELE)
				    || (r_ptr->flags1 & (RF1_UNIQUE))) {
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
#endif
					note = " is unaffected";
					resists_tele = TRUE;
				} else if (m_ptr->level > randint(100)) {
					note = " resists";
					resists_tele = TRUE;
				}

				if (!resists_tele) {
					if (seen) obvious = TRUE;
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= (RF3_UNDEAD);
#endif
					do_dist = dam;
				}
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;

		/* Teleport evil (Use "dam" as "power") -- unused */
		case GF_AWAY_EVIL:
			/* Only affect evil */
			if (r_ptr->flags3 & (RF3_EVIL)) {
				bool resists_tele = FALSE;

				if ((r_ptr->flags3 & (RF3_RES_TELE)) || (r_ptr->flags9 & RF9_IM_TELE)
				    || (r_ptr->flags1 & (RF1_UNIQUE))) {
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
#endif
					note = " is unaffected";
					resists_tele = TRUE;
				} else if (m_ptr->level > randint(100)) {
					note = " resists";
					resists_tele = TRUE;
				}

				if (!resists_tele) {
					if (seen) obvious = TRUE;
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= (RF3_EVIL);
#endif
					do_dist = dam;
				}
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;

		/* Teleport monster (Use "dam" as "power") */
		case GF_AWAY_ALL:
		{
			bool resists_tele = FALSE;
			//dun_level *l_ptr = getfloor(wpos);

			if ((r_ptr->flags9 & RF9_IM_TELE) ||
			    (r_ptr->flags3 & (RF3_RES_TELE)) ||
			    (r_ptr->flags1 & (RF1_UNIQUE))) {
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
#endif
				note = " is unaffected";
				resists_tele = TRUE;
			} else if (m_ptr->level > randint(100)) {
				note = " resists";
				resists_tele = TRUE;
			}

			if (!resists_tele) {
				/* Obvious */
				if (seen) obvious = TRUE;
				/* Prepare to teleport */
				do_dist = dam;
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;
		}


		/* Turn undead (Use "dam" as "power") -- overrides NO_FEAR, UNIQUE -- unused */
		case GF_TURN_UNDEAD:
			/* Only affect undead */
			if (r_ptr->flags3 & RF3_UNDEAD) {
#ifdef OLD_MONSTER_LORE
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= RF3_UNDEAD;
#endif
				/* Obvious */
				if (seen) obvious = TRUE;
				/* Apply some fear */
				do_fear = damroll(3, (dam / 2)) + 1;

				/* Attempt a saving throw */
				if (RES_OLD(r_ptr->level, dam)) {
					/* No obvious effect */
					note = " is unaffected";
					obvious = FALSE;
					do_fear = 0;
				}
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;

		/* Turn evil (Use "dam" as "power") -- overrides NO_FEAR -- unused */
		case GF_TURN_EVIL:
			/* Only affect evil */
			if (r_ptr->flags3 & RF3_EVIL) {
#ifdef OLD_MONSTER_LORE
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= RF3_EVIL;
#endif
				/* Obvious */
				if (seen) obvious = TRUE;
				/* Apply some fear */
				do_fear = damroll(3, (dam / 2)) + 1;

				if (r_ptr->flags1 & RF1_UNIQUE) {
					/* No obvious effect */
					note = " is unaffected";
					obvious = FALSE;
					do_fear = 0;
				}
				/* Attempt a saving throw */
				else if (RES_OLD(r_ptr->level, dam)) {
					/* No obvious effect */
					note = " is unaffected";
					obvious = FALSE;
					do_fear = 0;
				}
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;

		/* Turn monster (Use "dam" as "power") */
		case GF_TURN_ALL:
			/* Obvious */
			if (seen) obvious = TRUE;

			/* Apply some fear */
			do_fear = damroll(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags3 & RF3_NO_FEAR)) {
				/* No obvious effect */
				note = " is unaffected";
				obvious = FALSE;
				do_fear = 0;
			} else if (RES_OLD(r_ptr->level, dam)) {
				note = " resists the effect";
				obvious = FALSE;
				do_fear = 0;
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;

		/* Dispel undead */
		case GF_DISP_UNDEAD:
			/* Only affect undead */
			if (r_ptr->flags3 & RF3_UNDEAD) {
#ifdef OLD_MONSTER_LORE
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= RF3_UNDEAD;
#endif

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Message */
				note = " shudders"; 
				note_dies = " dissolves";
			}
			/* Ignore other monsters */
			else {
				/* No damage */
				dam = 0;
				quiet = TRUE;
			}
			break;

		/* Dispel evil */
		case GF_DISP_EVIL:
			/* Only affect evil */
			if (r_ptr->flags3 & RF3_EVIL) {
#ifdef OLD_MONSTER_LORE
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= RF3_EVIL;
#endif

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Message */
				note = " shudders";
				note_dies = " dissolves";
			}
			/* Ignore other monsters */
			else {
				/* No damage */
				dam = 0;
				quiet = TRUE;
			}
			break;

		case GF_DISP_DEMON:
			/* Only affect demons */
			if (r_ptr->flags3 & RF3_DEMON) {
#ifdef OLD_MONSTER_LORE
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= RF3_DEMON;
#endif

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Message */
				note = " shudders";
				note_dies = " dissolves";
			}
			/* Ignore other monsters */
			else {
				/* No damage */
				dam = 0;
				quiet = TRUE;
			}
			break;

		/* Dispel monster */
		case GF_DISP_ALL:
			/* Obvious */
			if (seen) obvious = TRUE;
			/* Message */
			note = " shudders";
			note_dies = " dissolves";
			break;

		/* mimic spell 'Cause Wounds' aka monsters' 'Curse' */
		case GF_CAUSE:
			/* Obvious */
			if (seen) obvious = TRUE;

			if (r_ptr->d_char == 'A' || (r_ptr->flags8 & RF8_NO_CUT)) {
				note = " is unaffected";
				dam = 0;
			} else {
				int chance = 100;

				if (!quiet) chance += p_ptr->lev * 2;
				if (r_ptr->flags3 & RF3_EVIL) chance <<= 1; //reverse pseudo-savingthrow vs 'neutral' monsters (aka 'non-evil')
				if (r_ptr->flags3 & RF3_GOOD) chance >>= 1; //pseudo-savingthrow especially for 'good' monsters
				if (rand_int(chance) < r_ptr->level) {
					note = " resists the effect";
					dam = 0;
				}
			}
			break;

		/* Nuclear waste */
		case GF_NUKE:
			if (seen) obvious = TRUE;

			if ((r_ptr->flags3 & RF3_IM_POIS) ||
			    (r_ptr->flags3 & (RF3_NONLIVING)) || (r_ptr->flags3 & (RF3_UNDEAD)) ||
			    (r_ptr->d_char == 'A') || ((r_ptr->d_char == 'U') && (r_ptr->flags3 & RF3_DEMON))) {
				note = " is immune";
				dam = 0;
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= (RF3_IM_POIS);
#endif
			} else if (r_ptr->flags9 & (RF9_RES_POIS)) {
				note = " resists";
				dam *= 3; dam /= (randint(6) + 6 );
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags9 |= (RF9_RES_POIS);
#endif
			}
			else if (randint(3) == 1) do_poly = TRUE;
			break;

		/* Pure damage */
		case GF_DISINTEGRATE:
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & (RF3_HURT_ROCK)) {
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= (RF3_HURT_ROCK);
#endif
				note = " loses some skin";
				note_dies = " evaporates";
				dam *= 2;
			}

			if (r_ptr->flags1 & RF1_UNIQUE) {
				if (rand_int(m_ptr->level + 10) > rand_int(plev)) {
					note = " resists";
					dam >>= 3;
				}
			}
			break;

		case GF_HOLD:
		case GF_DOMINATE:
			if (!quiet) {
				if ((!(r_ptr->flags1 & (RF1_UNIQUE|RF1_NEVER_MOVE)) &&
				    !(r_ptr->flags9 & RF9_IM_PSI) && !(r_ptr->flags7 & RF7_MULTIPLY)) ||
				    is_admin(p_ptr))
					m_ptr->owner = p_ptr->id;
				note = " starts following you";
			}
			dam = 0;
			quiet_dam = TRUE;
			break;

		/* Teleport monster TO */
		case GF_TELE_TO:
		{
			bool resists_tele = FALSE;
			//dun_level *l_ptr = getfloor(wpos);

			/* Teleport to nowhere..? */
			if (quiet) break;

			if (!(r_ptr->flags9 & RF9_IM_TELE) &&
			    !(r_ptr->flags3 & (RF3_RES_TELE))) {
				if (r_ptr->flags1 & (RF1_UNIQUE)) {
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
#endif
					note = " is unaffected";
					resists_tele = TRUE;
				} else if (m_ptr->level > randint(100)) {
#ifdef OLD_MONSTER_LORE
					if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
#endif
					note = " resists";
					resists_tele = TRUE;
				}
			} else {
#ifdef OLD_MONSTER_LORE
				if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
#endif
				note = " is unaffected";
				resists_tele = TRUE;
			}

			if (!resists_tele) {
				/* Obvious */
				if (seen) obvious = TRUE;

				/* Prepare to teleport */
				//do_dist = dam;
				teleport_to_player(Ind, c_ptr->m_idx);

				/* Hack -- get new location */
				y = m_ptr->fy;
				x = m_ptr->fx;

				/* Hack -- get new grid */
				c_ptr = &zcave[y][x];
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;
		}

		/* Hand of Doom */
		case GF_HAND_DOOM:
#if 0	/* okay, do that! ;) */
			if (r_ptr->r_flags1 & RF1_UNIQUE) {
				note = " resists";
				dam = 0;
			} else
#endif	// 0
			{
				int dummy = (((s32b) ((65 + randint(25)) * (m_ptr->hp))) / 100);
				//msg_print(Ind, "You feel your life fade away!");

				if (m_ptr->hp - dummy < 1) dummy = m_ptr->hp - 1;

				dam = dummy;
			}
			break;

		case GF_STOP: /* prevent monster from moving */
			dam = (dam - r_ptr->level - rand_int(10)) / 6;
			if (dam <= 0) {
				dam = 0;
				//note: we don't differentiate between 'unaffected' and 'resists' here, a dilemma added by the random factor above.
				note = " is unaffected";
			} else {
				note = " is frozen to the ground";
				m_ptr->no_move = dam;
				dam = 0;
			}
			quiet_dam = TRUE;
			break;

		/* Sleep (Use "dam" as "power") */
		case GF_STASIS:
			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    (r_ptr->flags9 & RF9_RES_TIME) ||
			    RES_OLD(r_ptr->level, dam)) {
				note = " is unaffected";
				obvious = FALSE;
			} else {
				/* Go to sleep (much) later */
				note = " is suspended";
				do_sleep = GF_OLD_SLEEP_DUR;
			}

			/* No "real" damage */
			dam = 0;
			quiet_dam = TRUE;
			break;

		/* Decrease strength */
		case GF_DEC_STR:
			/* hack */
			dam = 0;

			if (((r_ptr->flags1 & RF1_UNIQUE) && r_ptr->level >= 40) || (r_ptr->flags7 & RF7_NO_DEATH) ||
			    (r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON | RF3_DRAGON |  RF3_NONLIVING | RF3_TROLL | RF3_GIANT)) ||
			    !((r_ptr->flags3 & RF3_ANIMAL) || strchr("hHJkpPtn", r_ptr->d_char)) ||
			    (m_ptr->blow[0].effect == RBE_LOSE_STR || m_ptr->blow[1].effect == RBE_LOSE_STR || m_ptr->blow[2].effect == RBE_LOSE_STR || m_ptr->blow[3].effect == RBE_LOSE_STR
			    || m_ptr->blow[0].effect == RBE_LOSE_ALL || m_ptr->blow[1].effect == RBE_LOSE_ALL || m_ptr->blow[2].effect == RBE_LOSE_ALL || m_ptr->blow[3].effect == RBE_LOSE_ALL)) {
				//msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
			} else {
				for (i = 0; i < 4; i++) {
					/* prevent div0 if monster has no damaging attack */
					if (!m_ptr->blow[i].org_d_dice) continue;

					/* if dice are bigger than sides or if sides are just not further reducible, reduce the dice if they're still reducible */
					if ((m_ptr->blow[i].d_dice > m_ptr->blow[i].d_side || (100 * m_ptr->blow[i].d_side) / m_ptr->blow[i].org_d_side <= 72) &&
					    (100 * m_ptr->blow[i].d_dice) / m_ptr->blow[i].org_d_dice > 72) {
						/* reduce by 14% */
						m_ptr->blow[i].d_dice -= (14 * m_ptr->blow[i].org_d_dice) / 100;
						/* cap at 72% total reduction */
						if ((100 * m_ptr->blow[i].d_dice) / m_ptr->blow[i].org_d_dice < 72)
							m_ptr->blow[i].d_dice = (72 * m_ptr->blow[i].org_d_dice) / 100;
						dam = 1;
					}
					/* otherwise reduce the sides, if still reducible */
					else if ((100 * m_ptr->blow[i].d_side) / m_ptr->blow[i].org_d_side > 72) {
						/* reduce by 14% */
						m_ptr->blow[i].d_side -= (14 * m_ptr->blow[i].org_d_side) / 100;
						/* cap at 72% total reduction */
						if ((100 * m_ptr->blow[i].d_side) / m_ptr->blow[i].org_d_side < 72)
							m_ptr->blow[i].d_side = (72 * m_ptr->blow[i].org_d_side) / 100;
						dam = 1;
					}
				}
				if (dam) msg_print_near_monster(c_ptr->m_idx, "appears weaker!");
				//else msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
			}

			dam = 0;
			quiet = TRUE;
			break;

		/* Decrease dexterity */
		case GF_DEC_DEX:
			if (((r_ptr->flags1 & RF1_UNIQUE) && r_ptr->level >= 40) || (r_ptr->flags7 & RF7_NO_DEATH) ||
			    (r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON | RF3_DRAGON |  RF3_NONLIVING)) ||
			    !((r_ptr->flags3 & RF3_ANIMAL) || strchr("hHJkpPtn", r_ptr->d_char)) ||
			    (m_ptr->r_idx == 74 || m_ptr->r_idx == 539) ||
			    (m_ptr->blow[0].effect == RBE_LOSE_DEX || m_ptr->blow[1].effect == RBE_LOSE_DEX || m_ptr->blow[2].effect == RBE_LOSE_DEX || m_ptr->blow[3].effect == RBE_LOSE_DEX
			    || m_ptr->blow[0].effect == RBE_LOSE_ALL || m_ptr->blow[1].effect == RBE_LOSE_ALL || m_ptr->blow[2].effect == RBE_LOSE_ALL || m_ptr->blow[3].effect == RBE_LOSE_ALL)) {
				//msg_print_near_monster(c_ptr->m_idx, "is unaffected");
			} else {
				if (m_ptr->org_ac - m_ptr->ac < m_ptr->org_ac / 2) {
					if (m_ptr->ac) {
						msg_print_near_monster(c_ptr->m_idx, "appears clumsy!");
						m_ptr->ac = (m_ptr->ac * 7) / 8;
						if (m_ptr->ac > 10) m_ptr->ac -= 2;
						else if (m_ptr->ac > 5) m_ptr->ac -= 1;
					}
				}
				//else msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
			}
			dam = 0;
			quiet = TRUE;
			break;

		/* Decrease dexterity */
		case GF_DEC_CON:
			if (((r_ptr->flags1 & RF1_UNIQUE) && r_ptr->level >= 40) || (r_ptr->flags7 & RF7_NO_DEATH) ||
			    (r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON | RF3_DRAGON |  RF3_NONLIVING)) ||
			    !((r_ptr->flags3 & RF3_ANIMAL) || strchr("hHJkpPtn", r_ptr->d_char)) ||
			    (m_ptr->blow[0].effect == RBE_LOSE_CON || m_ptr->blow[1].effect == RBE_LOSE_CON || m_ptr->blow[2].effect == RBE_LOSE_CON || m_ptr->blow[3].effect == RBE_LOSE_CON
			    || m_ptr->blow[0].effect == RBE_LOSE_ALL || m_ptr->blow[1].effect == RBE_LOSE_ALL || m_ptr->blow[2].effect == RBE_LOSE_ALL || m_ptr->blow[3].effect == RBE_LOSE_ALL)) {
				//msg_print_near_monster(c_ptr->m_idx, "is unaffected");
			} else {
				if (m_ptr->org_maxhp - m_ptr->maxhp < m_ptr->org_maxhp / 2) {
					if (m_ptr->maxhp > 3) {
						msg_print_near_monster(c_ptr->m_idx, "appears less healthy!");
						m_ptr->maxhp = (m_ptr->maxhp * 7) / 8;
						if (m_ptr->maxhp > 10) m_ptr->maxhp -= 2;
						else if (m_ptr->maxhp > 5) m_ptr->maxhp -= 1;
						if (m_ptr->maxhp < m_ptr->hp) m_ptr->hp = m_ptr->maxhp;
					}
					//else msg_print_near_monster(c_ptr->m_idx, "is unaffected");
				}
			}
			dam = 0;
			quiet = TRUE;
			break;

		/* Restore strength */
		case GF_RES_STR:
			dam = 0; /* hack :) */
			for (i = 0; i < 4; i++) {
			/*	  if (m_ptr->blow[i].d_dice < r_ptr->blow[i].d_dice) m_ptr->blow[i].d_dice = r_ptr->blow[i].d_dice;
					if (m_ptr->blow[i].d_side < r_ptr->blow[i].d_side) m_ptr->blow[i].d_side = r_ptr->blow[i].d_side;
			*/	if (m_ptr->blow[i].d_dice < r_ptr->blow[i].org_d_dice) {
					m_ptr->blow[i].d_dice = r_ptr->blow[i].org_d_dice;
					dam = 1;
				}
				if (m_ptr->blow[i].d_side < r_ptr->blow[i].org_d_side) {
					m_ptr->blow[i].d_side = r_ptr->blow[i].org_d_side;
					dam = 1;
				}
			}
			if (dam) msg_print_near_monster(c_ptr->m_idx, "appears less weak.");
			dam = 0;
			quiet = TRUE;
			break;

		/* Restore dexterity */
		case GF_RES_DEX:
			/*if (m_ptr->ac < r_ptr->ac) {
				m_ptr->ac = r_ptr->ac;*/
			if (m_ptr->ac < m_ptr->org_ac) {
				m_ptr->ac = m_ptr->org_ac;
				msg_print_near_monster(c_ptr->m_idx, "appears less clumsy.");
			}
			dam = 0;
			quiet = TRUE;
			break;

		/* Restore constitution */
		case GF_RES_CON:
			/*if (m_ptr->hp < r_ptr->hside * r_ptr->hdice) {
				m_ptr->hp = r_ptr->hside * r_ptr->hdice;*/
			if (m_ptr->maxhp < m_ptr->org_maxhp) {
				m_ptr->hp += m_ptr->org_maxhp - m_ptr->maxhp;
				m_ptr->maxhp = m_ptr->org_maxhp;
				msg_print_near_monster(c_ptr->m_idx, "appears less sick.");
			}
			dam = 0;
			quiet = TRUE;
			break;

		/* Increase strength! */
		case GF_INC_STR:
			if ((r_ptr->flags7 & RF7_NO_DEATH)) {
				//msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
			} else {
				/* hack */
				dam = 0;

				for (i = 0; i < 4; i++) { /* increase the value which has less effect on total damage output - C. Blue :) */
					if (!m_ptr->blow[i].org_d_dice) continue; /* monster has no damaging attack? */

					/* do a 'restore strenght' before increasing it */
					if (m_ptr->blow[i].d_dice < r_ptr->blow[i].org_d_dice) m_ptr->blow[i].d_dice = r_ptr->blow[i].org_d_dice;
					if (m_ptr->blow[i].d_side < r_ptr->blow[i].org_d_side) m_ptr->blow[i].d_side = r_ptr->blow[i].org_d_side;

					if (m_ptr->blow[i].d_dice > m_ptr->blow[i].d_side
					    && m_ptr->blow[i].d_dice < m_ptr->blow[i].org_d_dice + 3) {
						m_ptr->blow[i].d_dice++;
						dam = 1;
					}
					else if (m_ptr->blow[i].d_side > 0 && m_ptr->blow[i].d_dice > 0
					    && m_ptr->blow[i].d_side < m_ptr->blow[i].org_d_side + 3) {
						m_ptr->blow[i].d_side++;
						dam = 1;
					}

					if (dam) msg_print_near_monster(c_ptr->m_idx, "appears stronger!");
					//else msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
				}
			}

			dam = 0;
			quiet = TRUE;
			break;

		/* Increase dexterity! */
		case GF_INC_DEX:
			if ((r_ptr->flags7 & RF7_NO_DEATH)) {
				//msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
			} else {
				if (m_ptr->ac < m_ptr->org_ac) m_ptr->ac = m_ptr->org_ac; /* include restore dex */
				if (m_ptr->ac < m_ptr->org_ac + 50) {
					m_ptr->ac += 5;
					msg_print_near_monster(c_ptr->m_idx, "appears more dextrous!");
				}
				//else msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
			}
			dam = 0;
			quiet = TRUE;
			break;

		/* Increase constitution! */
		case GF_INC_CON:
			if ((r_ptr->flags7 & RF7_NO_DEATH)) {
				//msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
			} else {
				if (m_ptr->maxhp < m_ptr->org_maxhp) { /* include a restore con here */
					m_ptr->hp += m_ptr->org_maxhp - m_ptr->maxhp;
					m_ptr->maxhp = m_ptr->org_maxhp;
				}

				if (m_ptr->maxhp < (m_ptr->org_maxhp * 3) / 2) {
					m_ptr->hp += ((r_ptr->hside * r_ptr->hdice) / 10) + 2;
					m_ptr->maxhp += ((r_ptr->hside * r_ptr->hdice) / 10) + 2;
					msg_print_near_monster(c_ptr->m_idx, "appears healthier!");
				}
				//else msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
			}

			dam = 0;
			quiet = TRUE;
			break;

		/* Augmentation! (who'd do such a thing, silyl..) */
		case GF_AUGMENTATION:
			if ((r_ptr->flags7 & RF7_NO_DEATH)) {
				//msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
			} else {
				/* hack */
				dam = 0;

				for (i = 0; i < 4; i++) { /* increase the value which has less effect on total damage output - C. Blue :) */
					if (!m_ptr->blow[i].org_d_dice) continue; /* monster has no damaging attack? */

					/* res */
					if (m_ptr->blow[i].d_dice < r_ptr->blow[i].org_d_dice) m_ptr->blow[i].d_dice = r_ptr->blow[i].org_d_dice;
					if (m_ptr->blow[i].d_side < r_ptr->blow[i].org_d_side) m_ptr->blow[i].d_side = r_ptr->blow[i].org_d_side;
					/* inc */
					if (m_ptr->blow[i].d_dice > m_ptr->blow[i].d_side
					    && m_ptr->blow[i].d_dice < m_ptr->blow[i].org_d_dice + 3) {
						m_ptr->blow[i].d_dice++;
						dam = 1;
					}
					else if (m_ptr->blow[i].d_side > 0 && m_ptr->blow[i].d_dice > 0
					    && m_ptr->blow[i].d_side < m_ptr->blow[i].org_d_side + 3) {
						m_ptr->blow[i].d_side++;
						dam = 1;
					}
				}

				/* res */
				if (m_ptr->ac < m_ptr->org_ac) m_ptr->ac = m_ptr->org_ac;
				/* inc */
				if (m_ptr->ac < m_ptr->org_ac + 50) {
					m_ptr->ac += 5;
					dam = 1;
				}

				/* res */
				if (m_ptr->maxhp < m_ptr->org_maxhp) {
					m_ptr->hp += m_ptr->org_maxhp - m_ptr->maxhp;
					m_ptr->maxhp = m_ptr->org_maxhp;
				}
				/* inc */
				if (m_ptr->maxhp < (m_ptr->org_maxhp * 3) / 2) {
					m_ptr->hp += ((r_ptr->hside * r_ptr->hdice) / 10) + 2;
					m_ptr->maxhp += ((r_ptr->hside * r_ptr->hdice) / 10) + 2;
					dam = 1;
				}

				if (dam) msg_print_near_monster(c_ptr->m_idx, "appears more powerful!");
				//else msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
			}

			dam = 0;
			quiet = TRUE;
			break;

		/* Ruination! (now we're talking) */
		case GF_RUINATION:
			if (((r_ptr->flags1 & RF1_UNIQUE) && r_ptr->level >= 40) || (r_ptr->flags7 & RF7_NO_DEATH) ||
			    (r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON | RF3_DRAGON |  RF3_NONLIVING)) ||
			    !((r_ptr->flags3 & RF3_ANIMAL) || strchr("hHJkpPtn", r_ptr->d_char))) {
				//msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
			} else {
				/* hack */
				dam = 0;

				/* -STR */
				if (!(r_ptr->flags3 & (RF3_TROLL | RF3_GIANT)) &&
				    !(m_ptr->blow[0].effect == RBE_LOSE_STR || m_ptr->blow[1].effect == RBE_LOSE_STR || m_ptr->blow[2].effect == RBE_LOSE_STR || m_ptr->blow[3].effect == RBE_LOSE_STR
				    || m_ptr->blow[0].effect == RBE_LOSE_ALL || m_ptr->blow[1].effect == RBE_LOSE_ALL || m_ptr->blow[2].effect == RBE_LOSE_ALL || m_ptr->blow[3].effect == RBE_LOSE_ALL))
				for (i = 0; i < 4; i++) {
					/* prevent div0 if monster has no damaging attack */
					if (!m_ptr->blow[i].org_d_dice) continue;

					/* if dice are bigger than sides or if sides are just not further reducible, reduce the dice if they're still reducible */
					if ((m_ptr->blow[i].d_dice > m_ptr->blow[i].d_side || (100 * m_ptr->blow[i].d_side) / m_ptr->blow[i].org_d_side <= 72) &&
					    (100 * m_ptr->blow[i].d_dice) / m_ptr->blow[i].org_d_dice > 72) {
						/* reduce by 14% */
						m_ptr->blow[i].d_dice -= (14 * m_ptr->blow[i].org_d_dice) / 100;
						/* cap at 72% total reduction */
						if ((100 * m_ptr->blow[i].d_dice) / m_ptr->blow[i].org_d_dice < 72)
							m_ptr->blow[i].d_dice = (72 * m_ptr->blow[i].org_d_dice) / 100;
						dam = 1;
					}
					/* otherwise reduce the sides, if still reducible */
					else if ((100 * m_ptr->blow[i].d_side) / m_ptr->blow[i].org_d_side > 72) {
						/* reduce by 14% */
						m_ptr->blow[i].d_side -= (14 * m_ptr->blow[i].org_d_side) / 100;
						/* cap at 72% total reduction */
						if ((100 * m_ptr->blow[i].d_side) / m_ptr->blow[i].org_d_side < 72)
							m_ptr->blow[i].d_side = (72 * m_ptr->blow[i].org_d_side) / 100;
						dam = 1;
					}
				}

				/* -DEX */
				if (!(m_ptr->r_idx == 74 || m_ptr->r_idx == 539) &&
				    !(m_ptr->blow[0].effect == RBE_LOSE_DEX || m_ptr->blow[1].effect == RBE_LOSE_DEX || m_ptr->blow[2].effect == RBE_LOSE_DEX || m_ptr->blow[3].effect == RBE_LOSE_DEX
				    || m_ptr->blow[0].effect == RBE_LOSE_ALL || m_ptr->blow[1].effect == RBE_LOSE_ALL || m_ptr->blow[2].effect == RBE_LOSE_ALL || m_ptr->blow[3].effect == RBE_LOSE_ALL) &&
				    (m_ptr->org_ac - m_ptr->ac < m_ptr->org_ac / 2)) {
					if (m_ptr->ac) {
						m_ptr->ac = (m_ptr->ac * 7) / 8;
						if (m_ptr->ac > 10) m_ptr->ac -= 2;
						else if (m_ptr->ac > 5) m_ptr->ac -= 1;
						dam = 1;
					}
				}

				/* -CON */
				if (!(m_ptr->blow[0].effect == RBE_LOSE_CON || m_ptr->blow[1].effect == RBE_LOSE_CON || m_ptr->blow[2].effect == RBE_LOSE_CON || m_ptr->blow[3].effect == RBE_LOSE_CON
				    || m_ptr->blow[0].effect == RBE_LOSE_ALL || m_ptr->blow[1].effect == RBE_LOSE_ALL || m_ptr->blow[2].effect == RBE_LOSE_ALL || m_ptr->blow[3].effect == RBE_LOSE_ALL) &&
				    (m_ptr->org_maxhp - m_ptr->maxhp < m_ptr->org_maxhp / 2)) {
					if (m_ptr->maxhp > 3) {
						m_ptr->maxhp = (m_ptr->maxhp * 7) / 8;
						if (m_ptr->maxhp > 10) m_ptr->maxhp -= 2;
						else if (m_ptr->maxhp > 5) m_ptr->maxhp -= 1;
						if (m_ptr->maxhp < m_ptr->hp) m_ptr->hp = m_ptr->maxhp;
						dam = 1;
					}
				}

				if (dam) msg_print_near_monster(c_ptr->m_idx, "appears less powerful!");
				//else msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
			}

			dam = 0;
			quiet = TRUE;
			break;

		/* Give experience! (dam -> +levels) */
		case GF_EXP:
			if ((r_ptr->flags7 & RF7_NO_DEATH) && m_ptr->level < MONSTER_LEVEL_MAX) {
				//msg_print_near_monster(c_ptr->m_idx, "is unaffected.");
			} else {
				msg_print_near_monster(c_ptr->m_idx, "appears more experienced!");
				if (dam < 1) dam = 1;
				m_ptr->exp = MONSTER_EXP(m_ptr->level + dam);
				monster_check_experience(c_ptr->m_idx, FALSE);
			}
			dam = 0;
			quiet = TRUE;
			break;

		case GF_CURING:
			/* hack - avoid crazy message spam under special circumstances */
			dam = 0;

			if (m_ptr->confused) {
				m_ptr->confused = 0;
				//msg_print_near_monster(c_ptr->m_idx, "is no longer confused.");
				dam = 1;
			}
			if (m_ptr->stunned) {
				m_ptr->stunned = 0;
				//msg_print_near_monster(c_ptr->m_idx, "is no longer stunned.");
				dam = 1;
			}

			dam = 0; /* hack :) */
			for (i = 0; i < 4; i++) {
				if (m_ptr->blow[i].d_dice < r_ptr->blow[i].org_d_dice) {
					m_ptr->blow[i].d_dice = r_ptr->blow[i].org_d_dice;
					dam = 1;
				}
				if (m_ptr->blow[i].d_side < r_ptr->blow[i].org_d_side) {
					m_ptr->blow[i].d_side = r_ptr->blow[i].org_d_side;
					dam = 1;
				}
			}
			//if (dam) msg_print_near_monster(c_ptr->m_idx, "appears less weak");

			if (m_ptr->ac < m_ptr->org_ac) {
				m_ptr->ac = m_ptr->org_ac;
				//msg_print_near_monster(c_ptr->m_idx, "appears less clumsy");
				dam = 1;
			}

			if (m_ptr->maxhp < m_ptr->org_maxhp) {
				m_ptr->hp += m_ptr->org_maxhp - m_ptr->maxhp;
				m_ptr->maxhp = m_ptr->org_maxhp;
				//msg_print_near_monster(c_ptr->m_idx, "appears less sick");
				dam = 1;
			}

			if (dam) msg_print_near_monster(c_ptr->m_idx, "appears cured.");

			dam = 0;
			quiet = TRUE;
			break;

		/* Default */
		default:
			/* No damage */
			dam = 0;
			quiet_dam = TRUE;
			break;
	}

	/* "Unique" monsters cannot be polymorphed */
	if (r_ptr->flags1 & RF1_UNIQUE) do_poly = FALSE;
	/* nor IM_TELE monsters.. */
	if (r_ptr->flags9 & RF9_IM_TELE) do_poly = FALSE;

	/* No polymorphing in Bree - mikaelh */
	if (in_bree(wpos)) do_poly = FALSE;


	/* "Unique" monsters can only be "killed" by the player */
	if (r_ptr->flags1 & RF1_UNIQUE) {
		/* Uniques may only be killed by the player */
		if ((who > 0) && (dam > m_ptr->hp)) dam = m_ptr->hp;
	}


	/* Check for death */
	if ((dam > m_ptr->hp) &&
	    /* Some mosnters are immune to death */
	    !(r_ptr->flags7 & RF7_NO_DEATH)) {
		/* Extract method of death */
		note = note_dies;
	}


	/* hack: No polymorphing in IDDC, because live-spawning isn't possible.
	   So polymorphing would just remove the monsters completely everytime. */
	if (do_poly && in_irondeepdive(wpos)) do_poly = FALSE;

#ifndef DAMAGE_BEFORE_POLY
	/* Mega-Hack -- Handle "polymorph" -- monsters get a saving throw */
	if (do_poly && (randint(90) > r_ptr->level)) {
		/* Default -- assume no polymorph */
		if (!do_sleep) note = " is unaffected";

		/* Pick a "new" monster race */
		i = poly_r_idx(m_ptr->r_idx);

		/* Handle polymorh */
		if (i != m_ptr->r_idx) {
			int clone, clone_summoning;

			/* Obvious */
			if (seen) obvious = TRUE;

			/* Monster polymorphs */
			note = " changes";

			/* Turn off the damage */
			dam = 0;
			quiet_dam = TRUE;

			/* Save clone status - mikaelh */
			clone = m_list[c_ptr->m_idx].clone;
			clone_summoning = m_list[c_ptr->m_idx].clone_summoning;

			/* "Kill" the "old" monster */
			delete_monster_idx(c_ptr->m_idx, TRUE);

			/* Create a new monster (no groups) */
			(void)place_monster_aux(wpos, y, x, i, FALSE, FALSE, clone, clone_summoning);

			/* XXX XXX XXX Hack -- Assume success */
			if (!quiet && c_ptr->m_idx == 0) {
				msg_format(Ind, "%^s disappears!", m_name);
				return(FALSE);
			}

			/* Hack -- Get new monster */
			m_ptr = &m_list[c_ptr->m_idx];

			/* Hack -- Get new race */
			r_ptr = race_inf(m_ptr);
		}
	}
#endif

	/* Handle "teleport" */
	if (do_dist) {
		/* Obvious */
		if (seen) obvious = TRUE;

		/* Message */
		note = " disappears";

		/* Teleport */
		/* TODO: handle failure (eg. st-anchor)	*/

		//teleport_away(c_ptr->m_idx, do_dist);
		m_ptr->do_dist = do_dist;
		scan_do_dist = TRUE;

		/* Hack -- get new location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Hack -- get new grid */
		c_ptr = &zcave[y][x];
	}

	/* Sound and Impact breathers never stun */
	if (do_stun &&
	    !(r_ptr->flags4 & RF4_BR_SOUN) &&
	    !(r_ptr->flags9 & RF9_RES_SOUND) &&
	    !(r_ptr->flags4 & RF4_BR_PLAS) &&
	    !(r_ptr->flags4 & RF4_BR_WALL) &&
	    !(r_ptr->flags3 & RF3_NO_STUN)) {
		/* Obvious */
		if (seen) obvious = TRUE;

#if 0
		/* Get stunned */
		if (m_ptr->stunned) {
			if (!do_sleep) note = " is more dazed";
			i = m_ptr->stunned + (do_stun / 2);
		} else {
			if (!do_sleep) note = " is dazed";
			i = do_stun;
		}
#else
		if (m_ptr->stunned > 100) {
			note = " is knocked out";
			i = m_ptr->stunned + (do_stun / 4);
		} else if (m_ptr->stunned > 50) {
			if (!do_sleep) note = " is heavily dazed";
			i = m_ptr->stunned + (do_stun / 3);
		} else if (m_ptr->stunned) {
			if (!do_sleep) note = " is more dazed";
			i = m_ptr->stunned + (do_stun / 2);
		} else {
			if (!do_sleep) note = " is dazed";
			i = do_stun;
		}
#endif

		/* Apply stun */
		m_ptr->stunned = (i < 200) ? i : 200;
	}

	/* Confusion and Chaos breathers (and sleepers) never confuse */
	if (do_conf &&
	    !(r_ptr->flags3 & RF3_NO_CONF) &&
	    !(r_ptr->flags4 & RF4_BR_CONF) &&
	    !(r_ptr->flags4 & RF4_BR_CHAO) &&
	    !(r_ptr->flags9 & RF9_RES_CHAOS)) {
		/* Obvious */
		if (seen) obvious = TRUE;

		/* Already partially confused */
		if (m_ptr->confused) {
			if (!do_sleep) note = " looks more confused";
			i = m_ptr->confused + (do_conf / 2);
		}
		/* Was not confused */
		else {
			if (!do_sleep) note = " looks confused";
			i = do_conf;
		}

		/* Apply confusion */
		m_ptr->confused = (i < 200) ? i : 200;
	}

	/* Blindness (confusion): Not for uniques or powerful monsters */
	if (do_blind &&
	    !strchr("AEX|,.mj$?*", r_ptr->d_char) &&
	    !(r_ptr->flags2 & RF2_POWERFUL) &&
	    !(r_ptr->flags2 & RF2_PASS_WALL) && /* Ethereal monsters */
	    !(r_ptr->flags4 & RF4_BR_LITE) && !(r_ptr->flags4 & RF4_BR_DARK) && /* light/dark hounds.. */
	    !(r_ptr->flags3 & RF3_UNDEAD) &&
	    !(r_ptr->flags3 & RF3_NONLIVING) &&
	    !(r_ptr->flags3 & RF3_DEMON) &&
	    !(r_ptr->flags3 & RF3_DRAGON) &&
	    !(r_ptr->flags3 & RF3_DRAGONRIDER) &&
	    !(r_ptr->flags1 & RF1_UNIQUE) &&
	    (m_ptr->level < 50))
/*	    !((r_ptr->flags3 & RF3_DEMON) && (m_ptr->level >= 40)) &&
	    !((r_ptr->flags3 & RF3_DRAGON) && (m_ptr->level >= 40)) &&
	    !((r_ptr->flags3 & RF3_DRAGONRIDER) && (m_ptr->level >= 40)) &&
	    !(r_ptr->flags1 & RF1_UNIQUE))*/
/*	    !(((r_ptr->flags3 & RF3_DEMON) ||
	    (r_ptr->flags3 & RF3_DRAGON) ||
	    (r_ptr->flags3 & RF3_DRAGONRIDER) ||
	    (r_ptr->flags1 & RF1_UNIQUE)) &&
	    RES_OLD(r_ptr->level, dam))  <- this line would require a " resists." note btw. */
	{
		/* Obvious */
		if (seen) obvious = TRUE;

		/* Not too confused yet? */
		if (m_ptr->confused < do_blind) {
			i = do_blind;
			if (!do_sleep) {
				/* Already confused */
				if (m_ptr->confused) {
					note = " looks more confused";
				}
				/* Was not confused */
				else {
					note = " gropes around blindly";
				}
			}
		}

		/* Apply confusion */
		m_ptr->confused = (i < 200) ? i : 200;
	} else if (do_blind) {
		if (note == NULL) {
		/* No obvious effect */
			note = " is unaffected";
			obvious = FALSE;
		}
	}

	/* Fear */
	if (do_fear) {
		/* Increase fear */
		i = m_ptr->monfear + do_fear;

		/* Set fear */
		m_ptr->monfear = (i < 200) ? i : 200;
		m_ptr->monfear_gone = 0;
	}


	/* If another monster did the damage, hurt the monster by hand */
	if (who > 0 || who <= PROJECTOR_UNUSUAL) {
		/* Redraw (later) if needed */
		update_health(c_ptr->m_idx);

		/* Some mosnters are immune to death */
		if (r_ptr->flags7 & RF7_NO_DEATH) dam = 0;

		/* Wake the monster up */
		m_ptr->csleep = 0;

		/* Hurt the monster */
		m_ptr->hp -= dam;

		/* Dead monster */
		if (m_ptr->hp < 0) {
			/* Since a non-player killed it, there will be no reward/loot */
			if (r_ptr->flags1 & RF1_UNIQUE) m_ptr->clone = 90; /* still allow some experience to be gained */
			else m_ptr->clone = 75;

			/* Generate treasure, etc */
			if (!quiet) monster_death(Ind, c_ptr->m_idx);

			/* Delete the monster */
			delete_monster_idx(c_ptr->m_idx, FALSE);

			/* Give detailed messages if destroyed */
			/* DEG Death message with damage. */
			if ((r_ptr->flags1 & RF1_UNIQUE) && (!quiet && note))
				msg_format(Ind, "%^s%s by \377e%d \377wdamage.", m_name, note, dam);
			else if (!quiet && note)
				msg_format(Ind, "%^s%s by \377g%d \377wdamage.", m_name, note, dam);

#ifdef DAMAGE_BEFORE_POLY
			do_poly = FALSE;
#endif
		}

		/* Damaged monster */
		else {
			/* Give detailed messages if visible or destroyed */
			if (!quiet && note && seen) msg_format(Ind, "%^s%s.", m_name, note);

			/* Hack -- Pain message */
			else if (!quiet && dam > 0) message_pain(Ind, c_ptr->m_idx, dam);

			/* Hack -- handle sleep */
			if (do_sleep) m_ptr->csleep = do_sleep;
		}
	}

	/* If the player did it, give him experience, check fear */
	else {
		bool fear = FALSE;

		if (m_ptr->questor && (m_ptr->questor_invincible || (r_ptr->flags7 & RF7_NO_DEATH) || !(m_ptr->questor_hostile & 0x1))) return obvious;

		if (p_ptr->admin_godly_strike) {
			p_ptr->admin_godly_strike--;
			if (!(r_ptr->flags1 & RF1_UNIQUE)) dam = m_ptr->hp + 1;
		}

		/* Hurt the monster, check for fear and death */
		if (!quiet && mon_take_hit(Ind, c_ptr->m_idx, dam, &fear, note_dies)) {
			/* Dead monster */
#ifdef DAMAGE_BEFORE_POLY
			do_poly = FALSE;
#endif
		}

		/* Damaged monster */
		else {
			/* Hack -- handle sleep */
			if (do_sleep) m_ptr->csleep = do_sleep;

			/* Give detailed messages if visible or destroyed */
			/* DEG Changed for added damage message. */
			if (!quiet && seen) {
				if (note) {
					if (!quiet_dam) {
						if (r_ptr->flags1 & RF1_UNIQUE) {
							if (p_ptr->r_killed[m_ptr->r_idx] == 1) {
								msg_format(Ind, "\377D%^s%s and takes \377e%d \377Ddamage.", m_name, note, dam);
								if (p_ptr->warn_unique_credit) Send_beep(Ind);
							} else
								msg_format(Ind, "%^s%s and takes \377e%d \377wdamage.", m_name, note, dam);
						} else
							msg_format(Ind, "%^s%s and takes \377g%d \377wdamage.", m_name, note, dam);
					} else {
						msg_format(Ind, "%^s%s.", m_name, note);
					}
				}
				/* Hack -- Pain message */
				else if (dam > 0) message_pain(Ind, c_ptr->m_idx, dam);

				/* Take note */
				if ((fear || do_fear) && !(m_ptr->csleep || m_ptr->stunned > 100)) {
#ifdef USE_SOUND_2010
#else
					sound(Ind, SOUND_FLEE);
#endif
					/* Message */
					if (m_ptr->r_idx != RI_MORGOTH)
						msg_format(Ind, "%^s flees in terror!", m_name);
					else
						msg_format(Ind, "%^s retreats!", m_name);
				}
			}

#if 0 /* see above */
			/* Take note */
			if (!quiet && (fear || do_fear) && (p_ptr->mon_vis[c_ptr->m_idx]) && !(m_ptr->csleep || m_ptr->stunned > 100)) {
 #ifdef USE_SOUND_2010
 #else
				sound(Ind, SOUND_FLEE);
 #endif

				/* Message */
				if (m_ptr->r_idx != RI_MORGOTH)
					msg_format(Ind, "%^s flees in terror!", m_name);
				else
					msg_format(Ind, "%^s retreats!", m_name);
			}
#endif

#ifdef ANTI_SEVEN_EXPLOIT /* code part: 'monster gets hit by a projection' */
			/* isn't there any player in our casting-los? */
			/* only ball spells/explosions, NOT clouds/walls/beams/bolts/traps/etc */
			//if ((flg & PROJECT_JUMP) && (flg & PROJECT_KILL) && !(flg & PROJECT_STAY) &&
			//if ((flg & PROJECT_KILL) && !(flg & PROJECT_STAY) &&
			if ((flg & PROJECT_KILL) && /* well, why not for lasting effects too actually */
			    /* some sanity/efficiency checks:.. */
			    y_origin && x_origin && dam) {
				/* first, make sure we don't have another potential target in our LOS.
				   If we have, we can just go for him and ignore whoever shot us previously.
				   NOTE: this stuff should probably be evaluated in monster target choosing
				         code already, for efficiency. */
				bool got_potential_target = FALSE;
				int p;
				/* make sure only monsters who NEED to use this actually do use it.. (Morgoth doesn't) */
				if ((r_ptr->flags2 & RF2_PASS_WALL) && (r_ptr->flags2 & RF2_KILL_WALL)) {
					got_potential_target = TRUE;
				}
				else if ((r_ptr->flags2 & RF2_PASS_WALL) || (r_ptr->flags2 & RF2_KILL_WALL)) {
					if (!m_ptr->closest_player || m_ptr->closest_player > NumPlayers ||
					    !inarea(&Players[m_ptr->closest_player]->wpos, &m_ptr->wpos))
						got_potential_target = TRUE;
					else if (projectable_wall_perm(&m_ptr->wpos, m_ptr->fy, m_ptr->fx,
					    Players[m_ptr->closest_player]->py, Players[m_ptr->closest_player]->px, MAX_RANGE)) {
						got_potential_target = TRUE;
					}
				}
				if (!got_potential_target) {
					for (p = 1; p <= NumPlayers; p++) {
						if (inarea(&Players[p]->wpos, &m_ptr->wpos) &&
						    projectable_wall(&m_ptr->wpos, m_ptr->fy, m_ptr->fx, Players[p]->py, Players[p]->px, MAX_RANGE)) {
							got_potential_target = TRUE;
							break;
						}
					}
				}
				/* Ok, noone else to go after, so we have to go for the one who actually hit us */
				if (!got_potential_target) {
					/* if we got hit by anything closer than last time, go for this one instead */
					p = distance(m_ptr->fy, m_ptr->fx, y_origin, x_origin);
					/* paranoia probably, but doesn't cost much: avoid silliness */
					if (!p) m_ptr->previous_direction = 0;
					/* start ANTI_SEVEN_EXPLOIT: */
					else {
						if (m_ptr->previous_direction == 0) {
							/* note: instead of closest dis to ANY player we just save m_ptr->cdis
							   at this time - relatively ineffective but also not mattering much probably.  */
							m_ptr->cdis_on_damage = m_ptr->cdis;
							/* start behaving special */
							m_ptr->previous_direction = -1;
							/* accept any epicenter of damage (which is closer than 999,999: always TRUE) */
							m_ptr->damage_tx = 999;
							m_ptr->damage_ty = 999;
							m_ptr->damage_dis = 999;
						}
						if (p < m_ptr->damage_dis) {
							m_ptr->damage_ty = y_origin;
							m_ptr->damage_tx = x_origin;
							m_ptr->damage_dis = p;
							m_ptr->p_tx = p_ptr->px;
							m_ptr->p_ty = p_ptr->py;
						}
					}
				}
			}
#endif
		}
	}

#ifdef DAMAGE_BEFORE_POLY
	/* Mega-Hack -- Handle "polymorph" -- monsters get a saving throw */
	if (do_poly && (randint(90) > r_ptr->level)) {
		/* Default -- assume no polymorph */
		if (!do_sleep) note = " is unaffected";

		/* Pick a "new" monster race */
		i = poly_r_idx(m_ptr->r_idx);

		/* Handle polymorh */
		if (i != m_ptr->r_idx) {
			int clone, clone_summoning;

			/* Obvious */
			if (seen) obvious = TRUE;

			/* Monster polymorphs */
			note = " changes";

			/* Save clone status - mikaelh */
			clone = m_list[c_ptr->m_idx].clone;
			clone_summoning = m_list[c_ptr->m_idx].clone_summoning;

			/* "Kill" the "old" monster */
			delete_monster_idx(c_ptr->m_idx, TRUE);

			/* Create a new monster (no groups) */
			(void)place_monster_aux(wpos, y, x, i, FALSE, FALSE, clone, clone_summoning);

			/* XXX XXX XXX Hack -- Assume success */
			if (!quiet) {
				if (c_ptr->m_idx == 0) {
					msg_format(Ind, "%^s disappears!", m_name);
					return (FALSE);
				} else msg_format(Ind, "%^s changes!", m_name);
			}
		}
	}
#endif

	/* Update the monster XXX XXX XXX */
	update_mon(c_ptr->m_idx, FALSE);

	if (!quiet) {
		/* Hack -- Redraw the monster grid anyway */
		everyone_lite_spot(wpos, y, x);
	}

	/* Return "Anything seen?" */
	return (obvious);
}






/*
 * Helper function for "project()" below.
 *
 * Handle a beam/bolt/ball causing damage to the player.
 *
 * This routine takes a "source monster" (by index), a "distance", a default
 * "damage", and a "damage type".  See "project_m()" above.
 *
 * If "rad" is non-zero, then the blast was centered elsewhere, and the damage
 * is reduced (see "project_m()" above).  This can happen if a monster breathes
 * at the player and hits a wall instead.
 *
 * We return "TRUE" if any "obvious" effects were observed.  XXX XXX Actually,
 * we just assume that the effects were obvious, for historical reasons.
 */
/*
 * Megahack -- who < -999 means 'by something strange'.		- Jir -
 * 
 * NOTE: unlike the note above, 'rad' doesn't seem to be used for damage-
 * reducing purpose, I don't know why.
 */
//static bool project_p(int Ind, int who, int r, struct worldpos *wpos, int y, int x, int dam, int typ, int rad)
static bool project_p(int Ind, int who, int r, struct worldpos *wpos, int y, int x, int dam, int typ, int rad, int flg, char *attacker) {
	player_type *p_ptr;
	monster_race *r_ptr;

	int k = 0;
	int div, k_elec, k_sound, k_lite;
	bool physical_shield;

	/* Hack -- assume obvious */
	bool obvious = TRUE;
	/* Player blind-ness */
	bool blind;
	/* Player needs a "description" (he is blind) */
	bool fuzzy = FALSE;
	/* Player is damaging herself (eg. by shattering potion) */
	bool self = FALSE;

	/* Source monster */
	monster_type *m_ptr = NULL;
	/* Monster name (for attacks) */
	char m_name[MNAME_LEN], m_name_gen[MNAME_LEN];
	/* Monster name (for damage) */
	char killer[MNAME_LEN];
	/* Colour of the damage, either r (standard) or e (unique monster) */
	char damcol = 'o';
	int psi_resists = 0, hack_dam = 0;
	/* Hack -- messages */
	//cptr act = NULL;
	/* For resist_time: Limit randomization of effect */
	int time_influence_choices;
	/* Another player casting attack spell on us? */
	bool friendly_player = FALSE;

	dun_level *l_ptr = getfloor(wpos);

	int priest_spell = FALSE, brief_rune_spell = FALSE, dark_spell = FALSE;


	/* Bad player number */
	if (Ind <= 0) return (FALSE);

	p_ptr = Players[Ind];

	/* Player has already been hit, return - mikaelh */
	if (p_ptr->got_hit) return (FALSE);

	r_ptr = &r_info[p_ptr->body_monster];

	/* Catch healing hacks */
	if (typ == GF_HEAL_PLAYER) {
#ifdef AUTO_RET_NEW
		/* Don't allow phase/teleport for auto-retaliation methods */
		if (p_ptr->auto_retaliaty) {
			msg_print(Ind, "\377yYou cannot use means of healing for auto-retaliation.");
			return FALSE;
		}
#endif
		hack_dam = dam & 0x3C00;
		dam = dam & 0x03FF;
	}

	/* the_sandman: the priest spell? (0 or 1) - doomed grounds hack for applying proper heal feedback */
	if (typ == GF_OLD_DRAIN && (dam & 4096)) { //currently unused since doomed grounds deals annihilation instead
		priest_spell = TRUE;
		dam &= ~4096; // the real damage (percentage drain)
	}
	//marker to apply reduced damage cap, since it's a 'Brief' runecraft spell
	if (typ == GF_ANNIHILATION && (dam & 8192)) {
		brief_rune_spell = TRUE;
		dam &= ~8192;
	}
	//marker to handle blindness power differently
	if (typ == GF_DARK_WEAK && (dam & 8192)) {
		dark_spell = TRUE;
		dam &= ~8192;
	}

	/* shadow running protects from taking ranged damage - C. Blue
	   note: currently thereby also affecting friendly effects' 'damage'. */
	if (p_ptr->shadow_running) dam /= 3;

	blind = (p_ptr->blind ? TRUE : FALSE);

	/* Player is not here */
	if ((x != p_ptr->px) || (y != p_ptr->py) || (!inarea(wpos,&p_ptr->wpos))) return (FALSE);

	/* Player cannot hurt himself */
	if (0 - who == Ind) {
		if (flg & (PROJECT_SELF | PROJECT_PLAY)) {
			/* todo: certain spell effects should not be cast on oneself,
			   they're only meant for projecting onto team mates. The reason
			   is that the player usually calls even better direct functions
			   in the lua files that already apply the spell effect to him.
			   Example: Detect traps spell in Divination. */
			if (typ == GF_WRAITH_PLAYER || typ == GF_CURE_PLAYER ||
			    typ == GF_RESFIRE_PLAYER || typ == GF_RESCOLD_PLAYER ||
			    typ == GF_RESELEC_PLAYER || typ == GF_RESACID_PLAYER ||
			    typ == GF_SEEMAP_PLAYER || typ == GF_DETECTTRAP_PLAYER ||
			    typ == GF_SEEINVIS_PLAYER || typ == GF_DETECTDOOR_PLAYER ||
			    typ == GF_DETECTCREATURE_PLAYER  || typ == GF_SPEED_PLAYER ||
			    typ == GF_SATHUNGER_PLAYER || typ == GF_REMFEAR_PLAYER ||
			    typ == GF_HERO_PLAYER ||
			    typ == GF_ZEAL_PLAYER || typ == GF_RESTORE_PLAYER ||
			    typ == GF_SANITY_PLAYER || typ == GF_SOULCURE_PLAYER ||
			    typ == GF_BLESS_PLAYER || typ == GF_RESPOIS_PLAYER ||
			    typ == GF_MINDBOOST_PLAYER || typ == GF_REMIMAGE_PLAYER ||
			    typ == GF_REMCONF_PLAYER)
				return FALSE;
			/* ok */
			self = TRUE;
		}
		else return (FALSE);
	}

	/* Mega-Hack -- Players cannot hurt other players */
	if (cfg.use_pk_rules == PK_RULES_NEVER && who <= 0 &&
	    who > PROJECTOR_UNUSUAL
	    && !(flg & PROJECT_PLAY)) /* except if it's an explicitely player-affecting spell! */
		return (FALSE);

	/* Store/house is safe -- NOT in dungeon! */
	if (p_ptr->store_num != -1 && istown(wpos)) return(FALSE);

	/* Extract radius */
	div = r + 1;

	/* Damage decrease over radius */
	dam = radius_damage(dam, div, typ);

	/* Hack -- always do at least one point of damage */
	if (dam <= 0) dam = 1;

	/* Hack -- Never do excessive damage */
	if (dam > MAGICAL_CAP) dam = MAGICAL_CAP;

	/* If the player is blind, be more descriptive */
	if (blind) fuzzy = TRUE;

	/* If the player is hit by a trap, be more descritive */
	if (who <= PROJECTOR_UNUSUAL) fuzzy = TRUE;

	if (who > 0) {
		/* Get the source monster */
		m_ptr = &m_list[who];

		/* Get the monster name for actions */
		monster_desc(Ind, m_name, who, 0);
		monster_desc(Ind, m_name_gen, who, 0x02);

		/* Get the monster's killing name */
		monster_desc(Ind, killer, who, 0x88);

		/* Get the monster's real name */
		monster_desc(Ind, p_ptr->really_died_from, who, 0x188);

		/* Unique monsters cause different damage message colour */
		if (race_inf(m_ptr)->flags1 & RF1_UNIQUE) damcol = 'L';
	}
	/* hack -- by trap */
	else if (who == PROJECTOR_TRAP) {
		if (attacker) {
			sprintf(killer, "a%s %s", is_a_vowel(attacker[0]) ? "n" : "", attacker);
			sprintf(m_name, "a%s %s", is_a_vowel(attacker[0]) ? "n" : "", attacker);
			sprintf(m_name_gen, "the %s", attacker);
		} else {
			/* Hopefully never. */
			/* OUTDATED: Actually this can happen if player's not on the trap (eg. door traps) */
			sprintf(killer, "a mysterious accident");
			sprintf(m_name, "something");
			sprintf(m_name_gen, "the");
		}
	}
	/* hack -- by shattering potion */
	else if (who == PROJECTOR_POTION) {
		/* TODO: add potion name */
		sprintf(killer, "an evaporating potion");
		sprintf(m_name, "an evaporating potion");
		sprintf(m_name_gen, "the");
	}
	/* hack -- by malformed invocation (runespell backlash) */
	else if (who == PROJECTOR_RUNE) {
		sprintf(killer, "a malformed invocation");
		sprintf(m_name, "a malformed invocation");
		sprintf(m_name_gen, "the");
	}
	/* hack -- another player who has logged out */
	else if (who == PROJECTOR_PLAYER) {
		sprintf(killer, "another player");
		sprintf(m_name, "another player");
		sprintf(m_name_gen, "its");

		/* Assume they were friendly */
		friendly_player = TRUE;

		/* Will hurt other players if it was a damaging spell!
		   So we should just exit here. Only drawback might be,
		   that in PvP a player cannot die from his opponent's
		   remaining nox cloud after he killed him ;). - C. Blue */
		return FALSE;
	}
	else if (who == PROJECTOR_TERRAIN) {
		if ((l_ptr && (l_ptr->flags2 & LF2_FAIR_TERRAIN_DAM)) ||
		    (in_sector00(wpos) && (sector00flags2 & LF2_FAIR_TERRAIN_DAM)))
			dam = (p_ptr->mhp * (8 + rand_int(5))) / 15 + 1;
			/* (4hp is lvl 1 char's min); maybe TODO: give high level players a slight advantage (cause higher loss if they die) */

		sprintf(killer, "hazardous environment");
		sprintf(m_name, "hazardous environment");
		sprintf(m_name_gen, "the");
	}
	/* hack -- by shattering potion */
	else if (who <= PROJECTOR_UNUSUAL) {
		/* TODO: add potion name */
		sprintf(killer, "something weird");
		sprintf(m_name, "something");
		sprintf(m_name_gen, "some");
	}
	else if (self) {
		sprintf(killer, p_ptr->male ? "himself" : "herself");
		sprintf(m_name, "It's yourself who");
		sprintf(m_name_gen, "your own");

		/* Do not apply Stair-GoI/deflection etc. on one's own helpful self-affecting spells */
		if ((typ == GF_HEAL_PLAYER) || (typ == GF_AWAY_ALL) ||
		    (typ == GF_WRAITH_PLAYER) || (typ == GF_SPEED_PLAYER) ||
		    (typ == GF_SHIELD_PLAYER) || (typ == GF_RECALL_PLAYER) ||
		    (typ == GF_BLESS_PLAYER) || (typ == GF_REMFEAR_PLAYER) ||
		    (typ == GF_REMCONF_PLAYER) || (typ == GF_REMIMAGE_PLAYER) ||
		    (typ == GF_SATHUNGER_PLAYER) || (typ == GF_RESFIRE_PLAYER) ||
		    (typ == GF_RESCOLD_PLAYER) || (typ == GF_CUREPOISON_PLAYER) ||
		    (typ == GF_SEEINVIS_PLAYER) || (typ == GF_SEEMAP_PLAYER) ||
		    (typ == GF_CURECUT_PLAYER) || (typ == GF_CURESTUN_PLAYER) ||
		    (typ == GF_DETECTCREATURE_PLAYER) || (typ == GF_DETECTDOOR_PLAYER) ||
		    (typ == GF_DETECTTRAP_PLAYER) || (typ == GF_TELEPORTLVL_PLAYER) ||
		    (typ == GF_RESPOIS_PLAYER) || (typ == GF_RESELEC_PLAYER) ||
		    (typ == GF_RESACID_PLAYER) || (typ == GF_HPINCREASE_PLAYER) ||
		    (typ == GF_HERO_PLAYER) || (typ == GF_SHERO_PLAYER) || (typ == GF_MINDBOOST_PLAYER) ||
		    (typ == GF_TELEPORT_PLAYER) || (typ == GF_ZEAL_PLAYER) ||
		    (typ == GF_RESTORE_PLAYER) || (typ == GF_REMCURSE_PLAYER) ||
		    (typ == GF_CURE_PLAYER) || (typ == GF_RESURRECT_PLAYER) ||
		    (typ == GF_SANITY_PLAYER) || (typ == GF_SOULCURE_PLAYER) ||
		    (typ == GF_OLD_HEAL) || (typ == GF_OLD_SPEED) || (typ == GF_PUSH) ||
		    (typ == GF_HEALINGCLOUD) || /* Also not a hostile spell */
		    (typ == GF_EXTRA_STATS) ||
		    (typ == GF_MINDBOOST_PLAYER) || (typ == GF_IDENTIFY) ||
		    (typ == GF_SLOWPOISON_PLAYER) || (typ == GF_CURING) ||
		    (typ == GF_OLD_POLY)) /* may (un)polymorph himself */
			friendly_player = TRUE;
	}
	else if (IS_PVP) {
		//strcpy(killer, p_ptr->play_vis[0 - who] ? Players[0 - who]->name : "It");
		//strcpy(m_name, p_ptr->play_vis[0 - who] ? Players[0 - who]->name : "It");
		sprintf(killer, "%s", p_ptr->play_vis[0 - who] ? Players[0 - who]->name : "It");
		sprintf(m_name, "%s", p_ptr->play_vis[0 - who] ? Players[0 - who]->name : "It");
		if (p_ptr->play_vis[0 - who]) {
			switch (Players[0 - who]->name[strlen(Players[0 - who]->name) - 1]) {
			case 's': case 'x': case 'z':
				sprintf(m_name_gen, "%s'", Players[0 - who]->name);
				break;
			default:
				sprintf(m_name_gen, "%s's", Players[0 - who]->name);
			}
		} else
			sprintf(m_name_gen, "its");
		strcpy(p_ptr->really_died_from, Players[0 - who]->name);

		/* Do not become hostile if it was a friendly spell */
		if ((typ != GF_HEAL_PLAYER) && (typ != GF_AWAY_ALL) &&
		    (typ != GF_WRAITH_PLAYER) && (typ != GF_SPEED_PLAYER) &&
		    (typ != GF_SHIELD_PLAYER) && (typ != GF_RECALL_PLAYER) &&
		    (typ != GF_BLESS_PLAYER) && (typ != GF_REMFEAR_PLAYER) &&
		    (typ != GF_REMCONF_PLAYER) && (typ != GF_REMIMAGE_PLAYER) &&
		    (typ != GF_SATHUNGER_PLAYER) && (typ != GF_RESFIRE_PLAYER) &&
		    (typ != GF_RESCOLD_PLAYER) && (typ != GF_CUREPOISON_PLAYER) &&
		    (typ != GF_SEEINVIS_PLAYER) && (typ != GF_SEEMAP_PLAYER) &&
		    (typ != GF_CURECUT_PLAYER) && (typ != GF_CURESTUN_PLAYER) &&
		    (typ != GF_DETECTCREATURE_PLAYER) && (typ != GF_DETECTDOOR_PLAYER) &&
		    (typ != GF_DETECTTRAP_PLAYER) && (typ != GF_TELEPORTLVL_PLAYER) &&
		    (typ != GF_RESPOIS_PLAYER) && (typ != GF_RESELEC_PLAYER) &&
		    (typ != GF_RESACID_PLAYER) && (typ != GF_HPINCREASE_PLAYER) &&
		    (typ != GF_HERO_PLAYER) && (typ != GF_SHERO_PLAYER) && (typ != GF_MINDBOOST_PLAYER) &&
		    (typ != GF_TELEPORT_PLAYER) && (typ != GF_ZEAL_PLAYER) &&
		    (typ != GF_RESTORE_PLAYER) && (typ != GF_REMCURSE_PLAYER) &&
		    (typ != GF_CURE_PLAYER) && (typ != GF_RESURRECT_PLAYER) &&
		    (typ != GF_SANITY_PLAYER) && (typ != GF_SOULCURE_PLAYER) &&
		    (typ != GF_OLD_HEAL) && (typ != GF_OLD_SPEED) && (typ != GF_PUSH) &&
		    (typ != GF_HEALINGCLOUD) && /* Also not a hostile spell */
		    (typ != GF_MINDBOOST_PLAYER) && (typ != GF_IDENTIFY) &&
		    (typ != GF_SLOWPOISON_PLAYER) && (typ != GF_CURING) &&
		    (typ != GF_EXTRA_STATS) &&
		    (typ != GF_OLD_POLY)) /* Non-hostile players may (un)polymorph each other */
		{ /* If this was intentional, make target hostile */
			if (check_hostile(0 - who, Ind)) {
				/* Make target hostile if not already */
				if (!check_hostile(Ind, 0 - who)) {
					bool result = FALSE;

					if (Players[Ind]->pvpexception < 2)
						result = add_hostility(Ind, killer, FALSE);

					/* Log it if no blood bond - mikaelh */
					if (!player_list_find(p_ptr->blood_bond, Players[0 - who]->id)) {
						s_printf("%s attacked %s (spell; result %d).\n", p_ptr->name, Players[0 - who]->name, result);
					}
				}
			}

			/* Hostile players hit each other */
			if (check_hostile(Ind, 0 - who)) {
#ifdef EXPERIMENTAL_PVP_SPELL_DAM
				int __dam = randint(dam);
				if (randint(1)) __dam *= -1;
				dam += __dam;
#else
				/* XXX Reduce damage to 1/3 */
				dam = (dam + PVP_SPELL_DAM_REDUCTION - 1) / PVP_SPELL_DAM_REDUCTION;
#endif
			}

			/* people not in the same party hit each other */
			else if (!Players[0 - who]->party || !p_ptr->party ||
				(!player_in_party(Players[0 - who]->party, Ind)))
			{
#if 0			/* NO - C. Blue */
#else			/* changed it to YES..  - still C. Blue - but added an if..*/
			if (check_hostile(0 - who, Ind)) {
#ifdef EXPERIMENTAL_PVP_SPELL_DAM
				int __dam = randint(dam); 
				if (randint(1)) __dam *= -1;
				dam += __dam;
#else
				/* XXX Reduce damage by 1/3 */
				dam = (dam + PVP_SPELL_DAM_REDUCTION - 1) / PVP_SPELL_DAM_REDUCTION;
#endif
#ifdef KURZEL_PK
				if (!magik(NEUTRAL_FIRE_CHANCE))
#else
				if ((cfg.use_pk_rules == PK_RULES_DECLARE &&
				    !(p_ptr->pkill & PKILL_KILLER)) &&
				    !magik(NEUTRAL_FIRE_CHANCE))
#endif
//#endif (<- use this endif, if above #if 0 becomes #if 1 again)			/* Just return without harming: */
					return(FALSE);
			} else { return(FALSE); }
#endif /* ..and remove this one accordingly */
			} else {
				/* Players in the same party can't harm each others */
#if FRIEND_FIRE_CHANCE
 #ifdef EXPERIMENTAL_PVP_SPELL_DAM
				int __dam = randint(dam); 
				if (randint(1)) __dam *= -1;
				dam += __dam;
 #else
				dam = (dam + PVP_SPELL_DAM_REDUCTION - 1) / PVP_SPELL_DAM_REDUCTION;
 #endif
				if (!magik(FRIEND_FIRE_CHANCE))
#endif
				return(FALSE);
			}
		/* If it's a support spell (friendly), remember the caster's level for henc anti-cheeze
		   to prevent him from getting exp until the supporting effects surely have run out: */
		} else if (typ != GF_OLD_POLY) {
			if (p_ptr->supp < Players[0 - who]->max_lev) p_ptr->supp = Players[0 - who]->max_lev;
			if (p_ptr->supp_top < Players[0 - who]->max_plv) p_ptr->supp_top = Players[0 - who]->max_plv;
			p_ptr->support_timer = cfg.spell_stack_limit ? cfg.spell_stack_limit : 200;

			friendly_player = TRUE;
		/* It's a non-hostile, yet possibly damaging spell which isn't
		   affected by 'friendly fire' rules or pvp rules, but counts
		   more like 'an accident' if it really damages someone. Usually
		   it is probably a supportive spell though. (eg GF_HEALINGCLOUD) */
		} else {
			friendly_player = TRUE;
		}
	}


	/* PvP often gives same message output as fuzzy */
	if (!strcmp(attacker,"") || !strcmp(m_name,"")) fuzzy = TRUE;

	/* Ghost-check (also checks for admin status) */
	/* GHOST CHECK */
	if ((p_ptr->ghost && ((typ == GF_HEAL_PLAYER) || /*(typ == GF_AWAY_ALL) ||*/
	    (typ == GF_WRAITH_PLAYER) || (typ == GF_SPEED_PLAYER) ||
	    /*(typ == GF_SHIELD_PLAYER) || (typ == GF_RECALL_PLAYER) ||*/
	    (typ == GF_BLESS_PLAYER) || (typ == GF_REMFEAR_PLAYER) ||
	    (typ == GF_REMCONF_PLAYER) || (typ == GF_REMIMAGE_PLAYER) ||
	    (typ == GF_SATHUNGER_PLAYER) || /*(typ == GF_RESFIRE_PLAYER) ||
	    (typ == GF_RESCOLD_PLAYER) ||*/ (typ == GF_CUREPOISON_PLAYER) ||
	    (typ == GF_SEEINVIS_PLAYER) || (typ == GF_SEEMAP_PLAYER) ||
	    (typ == GF_CURECUT_PLAYER) || /*(typ == GF_CURESTUN_PLAYER) ||*/
	    (typ == GF_DETECTCREATURE_PLAYER) || (typ == GF_DETECTDOOR_PLAYER) ||
	    (typ == GF_DETECTTRAP_PLAYER) || /*(typ == GF_TELEPORTLVL_PLAYER) ||
	    (typ == GF_RESPOIS_PLAYER) || (typ == GF_RESELEC_PLAYER) ||
	    (typ == GF_RESACID_PLAYER) ||*/ (typ == GF_HPINCREASE_PLAYER) ||
	    (typ == GF_HERO_PLAYER) || (typ == GF_SHERO_PLAYER) ||
	    /*(typ == GF_TELEPORT_PLAYER) ||*/ (typ == GF_ZEAL_PLAYER) || (typ == GF_REMCURSE_PLAYER) ||
	    (typ == GF_RESTORE_PLAYER) || (typ == GF_CURING) ||
	    (typ == GF_CURE_PLAYER) || /*(typ == GF_RESURRECT_PLAYER) ||
	    (typ == GF_SANITY_PLAYER) || (typ == GF_SOULCURE_PLAYER) ||*/
	    (typ == GF_OLD_HEAL) || (typ == GF_OLD_SPEED) ||
	    (typ == GF_HEALINGCLOUD) || /* shoo ghost, shoo */
	    (typ == GF_EXTRA_STATS) ||
	    (typ == GF_IDENTIFY) || (typ == GF_SLOWPOISON_PLAYER) ||
	    (typ == GF_OLD_POLY) || (typ == GF_MINDBOOST_PLAYER)))
	    ||
	    /* ADMIN CHECK */
	    (is_admin(p_ptr) && ((typ == GF_HEAL_PLAYER) || (typ == GF_AWAY_ALL) ||
	    (typ == GF_WRAITH_PLAYER) || (typ == GF_SPEED_PLAYER) ||
	    (typ == GF_SHIELD_PLAYER) || (typ == GF_RECALL_PLAYER) ||
	    (typ == GF_BLESS_PLAYER) || (typ == GF_REMFEAR_PLAYER) ||
	    (typ == GF_REMCONF_PLAYER) || (typ == GF_REMIMAGE_PLAYER) ||
	    (typ == GF_SATHUNGER_PLAYER) || (typ == GF_RESFIRE_PLAYER) ||
	    (typ == GF_RESCOLD_PLAYER) || (typ == GF_CUREPOISON_PLAYER) ||
	    (typ == GF_SEEINVIS_PLAYER) || (typ == GF_SEEMAP_PLAYER) ||
	    (typ == GF_CURECUT_PLAYER) || (typ == GF_CURESTUN_PLAYER) ||
	    (typ == GF_DETECTCREATURE_PLAYER) || (typ == GF_DETECTDOOR_PLAYER) ||
	    (typ == GF_DETECTTRAP_PLAYER) || (typ == GF_TELEPORTLVL_PLAYER) ||
	    (typ == GF_RESPOIS_PLAYER) || (typ == GF_RESELEC_PLAYER) ||
	    (typ == GF_RESACID_PLAYER) || (typ == GF_HPINCREASE_PLAYER) ||
	    (typ == GF_HERO_PLAYER) || (typ == GF_SHERO_PLAYER) ||
	    (typ == GF_TELEPORT_PLAYER) || (typ == GF_ZEAL_PLAYER) ||
	    (typ == GF_RESTORE_PLAYER) || (typ == GF_REMCURSE_PLAYER) ||
	    (typ == GF_CURE_PLAYER) || (typ == GF_RESURRECT_PLAYER) ||
	    (typ == GF_SANITY_PLAYER) || (typ == GF_SOULCURE_PLAYER) ||
	    (typ == GF_OLD_HEAL) || (typ == GF_OLD_SPEED) ||
	    (typ == GF_HEALINGCLOUD) || (typ == GF_MINDBOOST_PLAYER) ||
	    (typ == GF_SLOWPOISON_PLAYER) || (typ == GF_CURING) ||
	    (typ == GF_EXTRA_STATS) ||
	    (typ == GF_OLD_POLY) || (typ == GF_IDENTIFY))))
	{ /* No effect on ghosts / admins */
#if 0 //redundant?
		/* Skip non-connected players */
		if (p_ptr->conn != NOT_CONNECTED) {
			/* Disturb */
			disturb(Ind, 1, 0);
		}
#endif
		/* Return "Anything seen?" */
		return (obvious);
	}



#ifndef NEW_DODGING
	/* Bolt attack from a monster, a player or a trap */
	//if ((!rad) && get_skill(p_ptr, SKILL_DODGE) && (who > 0))
	/* Hack -- HIDE(direct) spell cannot be dodged */
	if (!friendly_player &&
	    get_skill(p_ptr, SKILL_DODGE) && !(flg & PROJECT_HIDE | PROJECT_JUMP | PROJECT_NODO))
	{
		if ((!rad) && (who >= PROJECTOR_TRAP)) {
			//		int chance = (p_ptr->dodge_level - ((r_info[who].level * 5) / 6)) / 3;
			/* Hack -- let's use 'dam' for now */
			int chance = (p_ptr->dodge_level - dam / 6) / 3;

			if ((chance > 0) && magik(chance)) {
				//msg_print(Ind, "You dodge a magical attack!");
				msg_format(Ind, "\377%cYou dodge the projectile!", COLOUR_DODGE_GOOD);
				return (TRUE);
			}
		}
		/* MEGAHACK -- allow to dodge 'bolt' traps */
		else if ((rad < 2) && (who == PROJECTOR_TRAP)) {
			/* Hack -- let's use 'dam' for now */
			int chance = (p_ptr->dodge_level - dam / 4) / 4;

			if ((chance > 0) && magik(chance)) {
				msg_format(Ind, "\377%cYou dodge a magical attack!", COLOUR_DODGE_GOOD);
				return (TRUE);
			}
		}
	}
#else
	/* Bolt attack from a monster, a player or a trap */
	//if (!p_ptr->blind && !(flg & (PROJECT_HIDE | PROJECT_GRID | PROJECT_JUMP)) && magik(apply_dodge_chance(Ind, getlevel(wpos) + 1000))) { /* hack - more difficult to dodge ranged attacks */
	if (!friendly_player &&
	    !p_ptr->blind && !(flg & (PROJECT_HIDE | PROJECT_JUMP | PROJECT_STAY | PROJECT_NODO)) && magik(apply_dodge_chance(Ind, getlevel(wpos)))) {
		if ((!rad) && (who >= PROJECTOR_TRAP)) {
			msg_format(Ind, "\377%cYou dodge %s projectile!", COLOUR_DODGE_GOOD, m_name_gen);
			return (TRUE);
		}
		/* MEGAHACK -- allow to dodge 'bolt' traps */
		else if ((rad < 2) && (who == PROJECTOR_TRAP)) {
			msg_format(Ind, "\377%cYou dodge %s magical attack!", COLOUR_DODGE_GOOD, m_name_gen);
			return (TRUE);
		}
	}
#endif


	/* Reflection */

	/* Physical-attack shield spells don't reflect all the time..! */
#ifdef ENABLE_OCCULT
	physical_shield = (p_ptr->kinetic_shield ? (rand_int(2) == 0) : FALSE) || (p_ptr->spirit_shield ? magik(p_ptr->spirit_shield_pow) : FALSE);
#else
	physical_shield = (p_ptr->kinetic_shield ? (rand_int(2) == 0) : FALSE);
#endif
	/* pre-calc kinetic/spirit shield mana tax before doing the reflection check below */
	if (physical_shield) {
		if (!friendly_player && (typ == GF_ARROW || typ == GF_MISSILE)
		    && p_ptr->csp >= dam / 7 &&
		    !rad && who != PROJECTOR_POTION && who != PROJECTOR_TERRAIN &&
		    (flg & PROJECT_KILL) && !(flg & (PROJECT_NORF | PROJECT_JUMP | PROJECT_STAY | PROJECT_NODF | PROJECT_NODO))) {
			/* drain mana */
			p_ptr->csp -= dam / 7;
			p_ptr->redraw |= PR_MANA;
		} else physical_shield = FALSE; /* failure to apply the shield to this particular attack */
	}

	/* Effects done by the plane cannot bounce,
	   balls / clouds / storms / walls (and beams atm too) cannot bounce - C. Blue */
	if (!friendly_player && (
	    /* kinetic/spirit shield? (same as reflect basically) */
	    physical_shield
	    /* reflect? */
	    || (p_ptr->reflect &&
	     !rad && who != PROJECTOR_POTION && who != PROJECTOR_TERRAIN &&
	     (flg & PROJECT_KILL) && !(flg & (PROJECT_NORF | PROJECT_JUMP | PROJECT_STAY | PROJECT_NODF | PROJECT_NODO)) &&
	     rand_int(20) < ((typ == GF_ARROW || typ == GF_MISSILE) ? 15 : 9))
#ifdef USE_BLOCKING
	    /* using a shield? requires USE_BLOCKING */
	    || (magik(apply_block_chance(p_ptr, p_ptr->shield_deflect / 5)) &&
	     !rad && who != PROJECTOR_POTION && who != PROJECTOR_TERRAIN &&
	     (flg & PROJECT_KILL) && !(flg & (PROJECT_NORF | PROJECT_JUMP | PROJECT_STAY | PROJECT_NODF | PROJECT_NODO)) &&
	     rand_int(20) < ((typ == GF_ARROW || typ == GF_MISSILE) ? 15 : 9))
#endif
	    ))
	{
		int t_y, t_x, ay, ax;
		int max_attempts = 10;

		if (blind) msg_print(Ind, "Something bounces!");
		else {
			if (p_ptr->play_vis[0 - who])
				msg_format(Ind, "%^s attack bounces!", m_name_gen);
			else
				msg_print(Ind, "Its attack bounces!");
		}

		/*if ((who != PROJECTOR_TRAP) && who)  isn't this wrong? */
		if (who && (who > PROJECTOR_UNUSUAL)) { /* can only hit a monster or a player when bouncing */
			if (who < 0) {
				ay = Players[-who]->py;
				ax = Players[-who]->px;
			} else {
				ay = m_list[who].fy;
				ax = m_list[who].fx;
			}


			/* Choose 'new' target */
			do {
				t_y = ay - 1 + randint(3);
				t_x = ax - 1 + randint(3);
				max_attempts--;
			}
#if 0
			while (max_attempts && in_bounds2(wpos, t_y, t_x) &&
					!(player_has_los_bold(Ind, t_y, t_x)));
#else	// 0
			while (max_attempts && (!in_bounds2(wpos, t_y, t_x) ||
					!(player_has_los_bold(Ind, t_y, t_x))));
#endif	// 0

			if (max_attempts < 1) {
				t_y = ay;
				t_x = ax;
			}

			//project(0, 0, wpos, t_y, t_x, dam, typ, (PROJECT_STOP|PROJECT_KILL));
			project(0 - Ind, 0, wpos, t_y, t_x, dam, typ, (PROJECT_STOP|PROJECT_KILL), "");
		}

		disturb(Ind, 1, 0);
		return TRUE;
	}


#ifdef USE_BLOCKING
	/* Bolt attacks: Took cover behind a shield? requires USE_BLOCKING */
	if (!friendly_player &&  /* cannot take cover from clouds or LOS projections (latter might be subject to change?) - C. Blue */
	     /* jump for LOS projecting, stay for clouds; !norf was already checked above -- not sure if fire_beam was covered (PROJECT_BEAM)! */
	    !rad && (flg & PROJECT_KILL) &&
	    !(flg & (PROJECT_NORF | PROJECT_JUMP | PROJECT_STAY | PROJECT_NODF))
	    ) /* requires stances to * 2 etc.. post-king -> best stance */
	{
		if (p_ptr->shield_deflect && magik(apply_block_chance(p_ptr, p_ptr->shield_deflect) / ((flg & PROJECT_LODF) ? 2 : 1))) {
			if (blind) msg_format(Ind, "\377%cSomething glances off your shield!", COLOUR_BLOCK_GOOD);
			else msg_format(Ind, "\377%cYou deflect %s attack!", COLOUR_BLOCK_GOOD, m_name_gen);
 #ifdef USE_SOUND_2010
			if (p_ptr->sfx_defense) sound(Ind, "block_shield", NULL, SFX_TYPE_ATTACK, FALSE);//not block_shield_projectile (silly for magical attacks)
 #endif

 #ifndef NEW_SHIELDS_NO_AC
			/* if we hid behind the shield from an acidic attack, damage the shield probably! */
			if (magik((dam < 5 ? 5 : (dam < 50 ? 10 : 25)))) shield_takes_damage(Ind, typ);
 #endif

			disturb(Ind, 1, 0);
			return TRUE;
		}
	}
	/* Ball attacks: Took cover behind a shield? requires USE_BLOCKING */
	else if (!friendly_player && /* cannot take cover from clouds or LOS projections (latter might be subject to change?) - C. Blue */
	     /* jump for LOS projecting, stay for clouds; !norf was already checked above -- not sure if fire_beam was covered (PROJECT_BEAM)! */
	    (flg & PROJECT_KILL) && (flg & PROJECT_NORF) && !(flg & (PROJECT_JUMP | PROJECT_STAY | PROJECT_NODF))) /* PROJECT_STAY to exempt 'cloud' spells! */
	    /* requires stances to * 2 etc.. post-king -> best stance */
	{
		if (p_ptr->shield_deflect && magik(apply_block_chance(p_ptr, p_ptr->shield_deflect) / ((flg & PROJECT_LODF) ? 4 : 2))) {
			if (blind) msg_format(Ind, "\377%cSomething hurls along your shield!", COLOUR_BLOCK_GOOD);
			else msg_format(Ind, "\377%cYou cover before %s attack!", COLOUR_BLOCK_GOOD, m_name_gen);

 #ifndef NEW_SHIELDS_NO_AC
			/* if we hid behind the shield from an acidic attack, damage the shield probably! */
			if (magik((dam < 5 ? 5 : (dam < 50 ? 10 : 25)))) shield_takes_damage(Ind, typ);
 #endif

			disturb(Ind, 1, 0);
			return TRUE;
		}
	}
#endif

	/* Test for stair GOI already, to prevent not only damage but according effects too! - C. Blue */
	/* Mega-Hack -- Apply "invulnerability" */
	if (!friendly_player && p_ptr->invuln && (!bypass_invuln)) {
		/* 1 in 2 chance to fully deflect the damage */
		if (magik(40)) {
			msg_print(Ind, "The attack is fully deflected by a magic shield.");
			if (who != PROJECTOR_TERRAIN) break_cloaking(Ind, 0);
			return(TRUE);
		}
		dam = (dam + 1) / 2;

		/* prevent multiple damage cutting */
		p_ptr->invuln_applied = TRUE;
	}


	/* Analyze the damage */
	switch (typ) {
	/* Psionics */
	case GF_PSI:
		if (rand_int(100) < p_ptr->skill_sav && !(p_ptr->esp_link_flags & LINKF_OPEN)) /* An open mind invites psi attacks */
			psi_resists++;
		if (p_ptr->mindboost && magik(p_ptr->mindboost_power)) psi_resists++;

		if ((p_ptr->shero) && (rand_int(100) >= p_ptr->skill_sav)) psi_resists--;
		if (p_ptr->confused) psi_resists--;
		if (p_ptr->image) psi_resists--;
		if (p_ptr->stun) psi_resists--;
		if ((p_ptr->afraid) && (rand_int(100) >= p_ptr->skill_sav)) psi_resists--;

		if (psi_resists > 0) {
			/* No side effects */

			/* Reduce damage */
			for (k = 0; k < psi_resists ; k++) {
				dam = dam * 4 / (4 + randint(5));
			}

			/* Telepathy reduces damage */
			if (p_ptr->telepathy) dam = dam * (3 + randint(6)) / 9;
		} else {
			/* Telepathy increases damage */
			if (p_ptr->telepathy) dam = dam * (6 + randint(6)) / 6;
		}
		if (p_ptr->pclass == CLASS_MINDCRAFTER) dam /= 2;

		if (fuzzy) msg_format(Ind, "Your mind is hit by mental energy for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);

		if (psi_resists <= 0) {
			/* Unresisted */
			if (!p_ptr->resist_conf &&
			    !(p_ptr->mindboost && magik(p_ptr->mindboost_power)))
				//set_confused(Ind, p_ptr->confused + rand_int(20) + 10);    too long for pvp?
				set_confused(Ind, p_ptr->confused + rand_int(5) + 5);

			/* At the moment:
			   No sleep/stun/fear effects (like it has on monsters), or image(hallu)
			*/
		}
		take_hit(Ind, dam, killer, -who);
		if (!IS_PVP) take_sanity_hit(Ind, dam / 8, killer); /* note: traps deal ~130..320 damage (depth 0..100) */
		break;

	/* Standard damage -- hurts inventory too */
	case GF_ACID:
		dam = acid_dam(Ind, dam, killer, -who);
		if (who == PROJECTOR_TERRAIN && dam == 0) ;
		else if (fuzzy) msg_format(Ind, "You are hit by acid for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		take_hit(Ind, dam, killer, -who);
		break;

	/* Standard damage -- hurts inventory too */
	case GF_FIRE:
		dam = fire_dam(Ind, dam, killer, -who);
		if (who == PROJECTOR_TERRAIN && dam == 0) ;
		else if (fuzzy) msg_format(Ind, "You are hit by fire for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		take_hit(Ind, dam, killer, -who);
		break;

	/* Standard damage -- hurts inventory too */
	case GF_COLD:
		dam = cold_dam(Ind, dam, killer, -who);
		if (who == PROJECTOR_TERRAIN && dam == 0) ;
		else if (fuzzy) msg_format(Ind, "You are hit by cold for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		take_hit(Ind, dam, killer, -who);
		break;

	/* Standard damage -- hurts inventory too */
	case GF_ELEC:
		dam = elec_dam(Ind, dam, killer, -who);
		apply_discharge(Ind, dam);
		if (who == PROJECTOR_TERRAIN && dam == 0) ;
		else if (fuzzy) msg_format(Ind, "You are hit by lightning for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		take_hit(Ind, dam, killer, -who);
		break;

	/* Standard damage -- also poisons player */
	case GF_POIS:
		if (p_ptr->immune_poison) {
			dam = 0;
			if (who == PROJECTOR_TERRAIN) ;
			else if (fuzzy) msg_format(Ind, "You are hit by poison for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		} else {
			if (p_ptr->resist_pois) dam = (dam + 2) / 3;
			if (p_ptr->oppose_pois) dam = (dam + 2) / 3;
			if (p_ptr->suscep_pois && !(p_ptr->resist_pois || p_ptr->oppose_pois)) dam = (dam + 2) * 2;

			if (fuzzy) msg_format(Ind, "You are hit by poison for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			take_hit(Ind, dam, killer, -who);
			if (!(p_ptr->resist_pois || p_ptr->oppose_pois) && dam > 0) {
				/* don't poison for too long in pvp */
				if (IS_PVP) {
					if (p_ptr->poisoned < 10) (void)set_poisoned(Ind, p_ptr->poisoned + rand_int(4), -who);
				} else {
					(void)set_poisoned(Ind, p_ptr->poisoned + rand_int(dam) + 10, -who);
				}
			}
		}
		break;

		/* Standard damage */
	case GF_MISSILE:
		if (p_ptr->biofeedback) dam /= 2;
		if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		take_hit(Ind, dam, killer, -who);
		break;

#ifdef ARCADE_SERVER
		case GF_PUSH:
	  //    msg_print(Ind, "You are pushed by something!");
		msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		(void)set_pushed(Ind, dam);
		break;
#endif

	case GF_HELL_FIRE:
		if (p_ptr->body_monster && (r_ptr->flags3 & RF3_GOOD)) dam *= 2;
		if (p_ptr->suscep_good) dam = (dam * 3) / 4;
		if (p_ptr->suscep_evil) dam *= 2;
		if (p_ptr->immune_fire) dam /= 2;
		else {
			if (p_ptr->resist_fire) dam = ((dam + 2) * 3) / 4;
			if (p_ptr->oppose_fire) dam = ((dam + 2) * 3) / 4;
			if (p_ptr->suscep_fire) dam = ((dam + 2) * 5) / 3;
		}
		if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		take_hit(Ind, dam, killer, -who);
		break;

		/* Holy Orb -- Player only takes partial damage */
	case GF_HOLY_ORB:
		if (p_ptr->suscep_evil) dam = (dam * 3) / 4;
		if (p_ptr->suscep_good) dam *= 2;
		if (p_ptr->body_monster && (r_ptr->flags3 & RF3_GOOD)) dam /= 4;
		if (p_ptr->pclass == CLASS_PRIEST) dam = 0;
		if (p_ptr->pclass == CLASS_PALADIN) dam /= 2;
		if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		take_hit(Ind, dam, killer, -who);
		break;

	case GF_HOLY_FIRE:
		if (p_ptr->suscep_evil) dam = (dam * 3) / 4;
		if (p_ptr->suscep_good) dam *= 2;
		if (p_ptr->body_monster && (r_ptr->flags3 & RF3_GOOD)) dam /= 4;
		if (p_ptr->immune_fire) dam /= 2;
		else {
			if (p_ptr->resist_fire) dam = ((dam + 2) * 3) / 4;
			if (p_ptr->oppose_fire) dam = ((dam + 2) * 3) / 4;
			if (p_ptr->suscep_fire && !(p_ptr->resist_fire || p_ptr->oppose_fire)) dam = ((dam + 2) * 4) / 3;
		}
		if (p_ptr->pclass == CLASS_PRIEST) dam = 0;
		if (p_ptr->pclass == CLASS_PALADIN) dam /= 2;
		if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		take_hit(Ind, dam, killer, -who);
		break;

		/* Arrow -- XXX no dodging */
	case GF_ARROW:
		if (p_ptr->biofeedback) dam /= 2;
		if (fuzzy) msg_format(Ind, "You are hit by something sharp for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		take_hit(Ind, dam, killer, -who);
		break;

		/* Plasma -- Fire/Lightning/Force */
	case GF_PLASMA:
		{
			bool ignore_fire = p_ptr->oppose_fire || p_ptr->resist_fire || p_ptr->immune_fire;
			bool ignore_elec = p_ptr->oppose_elec || p_ptr->resist_elec || p_ptr->immune_elec;
			bool inven_fire = (p_ptr->oppose_fire && p_ptr->resist_fire) || p_ptr->immune_fire;
			bool inven_elec = (p_ptr->oppose_elec && p_ptr->resist_elec) || p_ptr->immune_elec;

			/* 50% fire */
			k = (dam + 1) / 2;
			if (p_ptr->immune_fire) k = 0;
			else if (p_ptr->resist_fire || p_ptr->oppose_fire) {
				if (p_ptr->resist_fire) k = (k + 2) / 3;
				if (p_ptr->oppose_fire) k = (k + 2) / 3;
			} else if (p_ptr->suscep_fire) k *= 2;
			/* 25% elec */
			k_elec = (dam + 3) / 4;
			if (p_ptr->immune_elec) k_elec = 0;
			else if (p_ptr->resist_elec || p_ptr->oppose_elec) {
				if (p_ptr->resist_elec) k_elec = (k_elec + 2) / 3;
				if (p_ptr->oppose_elec) k_elec = (k_elec + 2) / 3;
			} else if (p_ptr->suscep_elec) k_elec *= 2;
			/* 25% force */
			k_sound = (dam + 3) / 4;
			if (p_ptr->resist_sound) k_sound = (k_sound + 1) / 2;
			else (void)set_stun(Ind, p_ptr->stun + (randint((k_sound > 22) ? 35 : (k_sound * 4 / 3 + 5)))); //strong stun
			dam = k + k_elec + k_sound;

			if (fuzzy) msg_format(Ind, "You are hit by something hot for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);

			take_hit(Ind, dam, killer, -who);

			/* Reduce stats */
			if (!ignore_fire && !ignore_elec) {
				if (randint(HURT_CHANCE) == 1) {
					if (rand_int(3)) (void) do_dec_stat(Ind, A_STR, DAM_STAT_TYPE((dam < 30) ? 1 : (dam < 60) ? 2 : 3));
					else (void) do_dec_stat(Ind, A_DEX, DAM_STAT_TYPE((dam < 30) ? 1 : (dam < 60) ? 2 : 3));
				}
			} else if (ignore_elec) {
				if (randint(HURT_CHANCE) == 1)
					(void) do_dec_stat(Ind, A_STR, DAM_STAT_TYPE((dam < 30) ? 1 : (dam < 60) ? 2 : 3));
			} else if (ignore_fire) {
				if (randint(HURT_CHANCE) == 1)
					(void) do_dec_stat(Ind, A_DEX, DAM_STAT_TYPE((dam < 30) ? 1 : (dam < 60) ? 2 : 3));
			}

			/* Don't kill inventory in bloodbond... */
			if (IS_PVP && check_blood_bond(Ind, -who)) break;

			/* Inventory damage */
			if (inven_fire && inven_elec) break;
			if (!inven_fire && !inven_elec) {
				if (rand_int(3)) inven_damage(Ind, set_fire_destroy, (dam < 30) ? 1 : (dam < 60) ? 2 : 3);
				else inven_damage(Ind, set_elec_destroy, (dam < 30) ? 1 : (dam < 60) ? 2 : 3);
			} else if (inven_elec) inven_damage(Ind, set_fire_destroy, (dam < 30) ? 1 : (dam < 60) ? 2 : 3);
			else inven_damage(Ind, set_elec_destroy, (dam < 30) ? 1 : (dam < 60) ? 2 : 3);

			break;
		}

		/* Nether -- drain experience */
	case GF_NETHER_WEAK:
		/* potion smash effect of a Potion of Death */
		if (p_ptr->suscep_life || p_ptr->ghost) {
			dam = 0;
			if (who == PROJECTOR_TERRAIN) ;
			else if (fuzzy) msg_format(Ind, "You are hit by something strange for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			break;
		}
	case GF_NETHER:
		if (p_ptr->immune_neth) {
			dam = 0;
			if (who == PROJECTOR_TERRAIN) ;
			else if (fuzzy) msg_format(Ind, "You are hit by something strange for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		} else if (p_ptr->resist_neth) {
			dam *= 6; dam /= (randint(6) + 6);
			if (fuzzy) msg_format(Ind, "You are hit by something strange for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			take_hit(Ind, dam, killer, -who); /* Prevent going down to level 1 and then taking full damage -> instakill */
		} else {
			if (fuzzy) msg_format(Ind, "You are hit by something strange for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			take_hit(Ind, dam, killer, -who); /* Prevent going down to level 1 and then taking full damage -> instakill */

			if (p_ptr->keep_life || (p_ptr->mode & MODE_PVP))
				msg_print(Ind, "You are unaffected!");
			else if (p_ptr->hold_life && (rand_int(100) < 75))
				msg_print(Ind, "You keep hold of your life force!");
			else if (p_ptr->hold_life) {
				msg_print(Ind, "You feel your life slipping away!");
				lose_exp(Ind, 200 + (p_ptr->exp/1000) * MON_DRAIN_LIFE);
			} else {
				msg_print(Ind, "You feel your life draining away!");
				lose_exp(Ind, 200 + (p_ptr->exp/100) * MON_DRAIN_LIFE);
			}
		}
		break;

		/* Water -- stun/confuse */
	case GF_WATER:
	case GF_WAVE:
		if (p_ptr->immune_water) {
			dam = 0;
			//doesn't make much sense--  if (who == PROJECTOR_TERRAIN) ; else
			if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			take_hit(Ind, dam, killer, -who);
		} else {
			if (p_ptr->body_monster && (r_ptr->flags7 & RF7_AQUATIC)) dam = (dam + 3) / 4;
			else if (p_ptr->resist_water) dam = (dam + 2) / 3;
			if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			if (TOOL_EQUIPPED(p_ptr) != SV_TOOL_TARPAULIN && magik(20 + dam / 20) &&
			    !(p_ptr->body_monster && (r_ptr->flags7 & RF7_AQUATIC))) {
				if (!p_ptr->resist_water || magik(50)) {
					/* Don't kill inventory in bloodbond... */
					int breakable = 1;
					if (IS_PVP && check_blood_bond(Ind, -who)) breakable = 0;
					if (breakable) {
						inven_damage(Ind, set_water_destroy, 1);
						if (magik(50)) equip_damage(Ind, GF_WATER);
					}
				}
			}
			take_hit(Ind, dam, killer, -who);
			if ((!p_ptr->resist_water) && !(p_ptr->body_monster && (r_ptr->flags7 & RF7_AQUATIC))) {
				if (!p_ptr->resist_sound) {
					if (IS_PVP) {
						(void)set_stun(Ind, p_ptr->stun + randint(10));
					} else { /* pvm */
						(void)set_stun(Ind, p_ptr->stun + randint(40));
					}
				}
				if (!p_ptr->resist_conf &&
				    !(p_ptr->mindboost && magik(p_ptr->mindboost_power))) {
					if (IS_PVP) {
						//(void)set_confused(Ind, p_ptr->confused + randint(2));
					} else { /* pvm */
						(void)set_confused(Ind, p_ptr->confused + randint(5) + 5);
					}
				}
			}
		}
		break;

	case GF_VAPOUR:
		if (p_ptr->immune_water) {
			dam = 0;
			//not really needed-- if (who == PROJECTOR_TERRAIN) ; else
			if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			take_hit(Ind, dam, killer, -who);
		} else {
			if (p_ptr->body_monster && (r_ptr->flags7 & RF7_AQUATIC))
				dam = (dam + 3) / 4;
			else if (p_ptr->resist_water)
				dam = (dam + 2) / 3;
			if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);

			/* no item damage */

			take_hit(Ind, dam, killer, -who);

			/* no stun/conf */
		}
		break;

		/* Stun */
	case GF_STUN:
		if (fuzzy) msg_print(Ind, "You are hit by something!");
		if (!p_ptr->resist_sound) (void)set_stun(Ind, p_ptr->stun + randint(40));
		dam = 0;
		break;

		/* Chaos -- many effects */
	case GF_CHAOS:
		if (p_ptr->resist_chaos) {
			//dam *= 6; dam /= (randint(7) + 8);
			dam *= 6; dam /= (randint(6) + 6);
		}
		if (fuzzy) msg_format(Ind, "You are hit by something strange for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);

		take_hit(Ind, dam, killer, -who); /* Prevent going down to level 1 and then taking full damage -> instakill */

		if (!p_ptr->resist_conf)
			(void)set_confused(Ind, p_ptr->confused + rand_int(20) + 10);
		if (!p_ptr->resist_chaos && !(p_ptr->mode & MODE_PVP))
			(void)set_image(Ind, p_ptr->image + randint(10));
		if (!p_ptr->resist_neth && !p_ptr->resist_chaos) {
			if (p_ptr->keep_life || (p_ptr->mode & MODE_PVP))
				msg_print(Ind, "You are unaffected!");
			else if (p_ptr->hold_life && (rand_int(100) < 75))
				msg_print(Ind, "You keep hold of your life force!");
			else if (p_ptr->hold_life) {
				msg_print(Ind, "You feel your life slipping away!");
				lose_exp(Ind, 500 + (p_ptr->exp/1000) * MON_DRAIN_LIFE);
			} else {
				msg_print(Ind, "You feel your life draining away!");
				lose_exp(Ind, 5000 + (p_ptr->exp/100) * MON_DRAIN_LIFE);
			}
		}
		break;

		/* Shards -- mostly cutting */
	case GF_SHARDS:
		if (p_ptr->biofeedback) dam /= 2;
		if (p_ptr->resist_shard) { dam *= 6; dam /= (randint(6) + 6); }
		if (fuzzy) msg_format(Ind, "You are hit by something sharp for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		take_hit(Ind, dam, killer, -who);
		if ((!p_ptr->resist_shard) && (!p_ptr->no_cut))
			(void)set_cut(Ind, p_ptr->cut + dam, -who);
		break;

		/* Sound -- mostly stunning */
	case GF_SOUND:
		/* Making the stun effect's power depend on body mass of the monster, giving gold dragons more respect again;
		   or more dependant on the damage to better fit certain monstes - C. Blue
		   body masses: 90k kavlax, 110k mature gold d, 170k mature d turtle / ancient d, 190k ancient d turtle / wyrms
		   hp: 500 drakes + mature turtle, 620 mature gold d + ancient turtle, 700 ancient turtle, 1.3k kavlax, 1.5k ancient d, 5k wyrms */
		if (p_ptr->biofeedback) dam /= 2;
		if (p_ptr->resist_sound) {
			dam *= 5; dam /= (randint(6) + 6);
			if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		} else {
			int k = (randint((dam > 90) ? 35 : (dam / 3 + 5)));
			/* for medium dragons/turtles */
			if (dam > 100 && p_ptr->stun < 35) k = 35;
			/* for kavlax, ancient things, wyrms, and (unfortunately? dunno..) also aether vortex/hound */
			if (dam > 200 && p_ptr->stun < 50) k = 50;
			if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			(void)set_stun(Ind, p_ptr->stun + k);
		}
		take_hit(Ind, dam, killer, -who);
		break;

		/* Pure confusion */
	case GF_CONFUSION:
		if (p_ptr->resist_conf || p_ptr->resist_chaos) {
			dam *= 5; dam /= (randint(6) + 6);
			if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		} else {
			if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			(void)set_confused(Ind, p_ptr->confused + randint(20) + 10);
		}
		take_hit(Ind, dam, killer, -who);
		break;

		/* Disenchantment -- see above */
	case GF_DISENCHANT:
		if (p_ptr->resist_disen) {
			dam *= 6; dam /= (randint(6) + 6);
			if (fuzzy) msg_format(Ind, "You are hit by something strange for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		} else {
			if (fuzzy) msg_format(Ind, "You are hit by something strange for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			(void)apply_disenchant(Ind, 0);
		}
		take_hit(Ind, dam, killer, -who);
		break;

		/* Nexus -- see above */
	case GF_NEXUS:
		if (p_ptr->resist_nexus) {
			dam *= 6; dam /= (randint(6) + 6);
			if (fuzzy) msg_format(Ind, "You are hit by something strange for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		} else {
			if (fuzzy) msg_format(Ind, "You are hit by something strange for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			apply_nexus(Ind, m_ptr, -who);
		}
		take_hit(Ind, dam, killer, -who);
		break;

		/* Force -- mostly stun */
	case GF_FORCE:
		if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		if (!p_ptr->resist_sound) (void)set_stun(Ind, p_ptr->stun + randint(20));
		take_hit(Ind, dam, killer, -who);
		break;

		/* Inertia -- slowness */
	case GF_INERTIA:
		if (fuzzy) msg_format(Ind, "You are hit by something strange for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		if (p_ptr->mindboost && magik(p_ptr->mindboost_power)) /* resist the effect */ ;
		else if (!p_ptr->free_act) (void)set_slow(Ind, p_ptr->slow + rand_int(4) + 4);
		else (void)set_slow(Ind, p_ptr->slow + rand_int(3) + 2);
		take_hit(Ind, dam, killer, -who);
		break;

		/* Lite -- blinding */
	case GF_LITE:
		if (p_ptr->resist_lite) { dam *= 4; dam /= (randint(6) + 6); }
		else if (p_ptr->suscep_lite) dam *= 2;
		if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		if (!p_ptr->resist_lite && !blind && !p_ptr->resist_blind)
			(void)set_blind(Ind, p_ptr->blind + randint(5) + 2);
		take_hit(Ind, dam, killer, -who);
		break;

	/* Dark -- blinding */
	case GF_DARK_WEAK:
		if (!p_ptr->resist_dark && !blind && !p_ptr->resist_blind)
			/* Exception: Bolt-type spells have no special effect */
			if (flg & (PROJECT_NORF | PROJECT_JUMP)) {
				//note: we make no difference for now here, both do 1..5+2, since pvp-effects are supposed to be greatly diminished anyway
				if (dark_spell) (void)set_blind(Ind, p_ptr->blind + randint(5) + 2); //occult shadow spell in pvp?
				else (void)set_blind(Ind, p_ptr->blind + randint(5) + 2); //normal
			}
		dam = 0;
		break;
	case GF_DARK:
		if (p_ptr->resist_dark) { dam *= 4; dam /= (randint(6) + 6); }
		if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
		else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
		if (!p_ptr->resist_dark && !blind && !p_ptr->resist_blind)
			/* Exception: Bolt-type spells have no special effect */
			if (flg & (PROJECT_NORF | PROJECT_JUMP))
				(void)set_blind(Ind, p_ptr->blind + randint(5) + 2);
		take_hit(Ind, dam, killer, -who);
		break;

		/* Time -- bolt fewer effects XXX */
	case GF_TIME:
			if (p_ptr->resist_time) { dam *= 6; dam /= (randint(6) + 6); }
			if (fuzzy) msg_format(Ind, "You are hit by something strange for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			take_hit(Ind, dam, killer, -who); /* Prevent going down to level 1 and then taking full damage -> instakill */

			if ((p_ptr->mode & MODE_PVP)) break;
			else if (p_ptr->resist_time) {
				/* a little hack for high-elven runemasters: super low time damage has no drain effects */
				if (flg & PROJECT_RNAF) break;

				time_influence_choices = randint(9);
			}
			/* hack: very little time damage won't result in ruination */
			else if (dam < 10) time_influence_choices = randint(9);
			else time_influence_choices = randint(10);

			switch (time_influence_choices) {
			case 1: case 2: case 3: case 4: case 5:
				if (p_ptr->resist_time) {
					/* let's disable it for now to improve time resistance: */
					if (magik(25)) {
						if (p_ptr->keep_life) msg_print(Ind, "You are unaffected!");
						else {
							msg_print(Ind, "You feel life has clocked back.");
							lose_exp(Ind, (p_ptr->exp / 100) * MON_DRAIN_LIFE / 4);
						}
					} else {
						msg_print(Ind, "You feel as if life has clocked back, but the feeling passes.");
					}
				} else {
					if (p_ptr->keep_life) msg_print(Ind, "You are unaffected.");
					else {
						msg_print(Ind, "You feel life has clocked back.");
						lose_exp(Ind, 100 + (p_ptr->exp / 100) * MON_DRAIN_LIFE);
					}
				}
				break;

			case 6: case 7: case 8: case 9:
				/* Sustenance slightly helps (50%) */
				do_dec_stat_time(Ind, rand_int(6), STAT_DEC_NORMAL, 50, p_ptr->resist_time ? 1 : 2, TRUE);
				break;

			case 10:
				msg_print(Ind, "You're not as powerful as you used to be...");
				for (k = 0; k < 6; k++) {
					/* Sustenance slightly helps (50%) */
					do_dec_stat_time(Ind, rand_int(6), STAT_DEC_NORMAL, 50, p_ptr->resist_time ? 1 : 3, FALSE);
				}
				break;
			}

			break;

		/* Gravity -- stun plus slowness plus teleport */
	case GF_GRAVITY:
			/* Feather fall (flying implies it, so flying is covered too) lets us resist gravity */
			if (p_ptr->feather_fall) { dam *= 6; dam /= (randint(6) + 6); }
			if (fuzzy) msg_format(Ind, "You are hit by something strange for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			msg_print(Ind, "Gravity warps around you.");
			if (!p_ptr->martyr) teleport_player(Ind, 5, TRUE);
			(void)set_slow(Ind, p_ptr->slow + rand_int(4) + 3);
			if (!p_ptr->resist_sound) {
				int k = (randint((dam > 90) ? 35 : (dam / 3 + 5)));
				(void)set_stun(Ind, p_ptr->stun + k);
			}
			take_hit(Ind, dam, killer, -who);
			break;

		/* Pure damage */
	case GF_MANA:
			if (p_ptr->resist_mana) { dam *= 6; dam /= (randint(6) + 6); }
			if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			take_hit(Ind, dam, killer, -who);
			break;

		/* Pure damage */
	case GF_METEOR:
			if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			take_hit(Ind, dam, killer, -who);
			if (!p_ptr->resist_sound) (void)set_stun(Ind, p_ptr->stun + 25 + randint(15));
			break;

		/* Ice -- cold plus stun plus cuts */
	case GF_ICE:
			k = dam;
			dam = (k * 3) / 5;/* 60% COLD damage, total cold damage is saved in 'dam' */
			k = (k * 2) / 5;/* 40% SHARDS damage, total shard damage is saved in 'k' */
			if (p_ptr->biofeedback) k = (k * 2) / 3;
			if (p_ptr->resist_shard) { k *= 6; k /= (randint(6) + 6); }
			dam = cold_dam(Ind, dam, killer, -who);
			dam = dam + k;
			if (fuzzy) msg_format(Ind, "You are hit by something cold and sharp for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			take_hit(Ind, dam, killer, -who);
			if ((!p_ptr->resist_shard) && (!p_ptr->no_cut))
				(void)set_cut(Ind, p_ptr->cut + damroll(5, 8), -who);
			/* if (!p_ptr->resist_sound)
				(void)set_stun(Ind, p_ptr->stun + randint(15)); */
			break;

		/* Thunder -- elec plus sound plus light */
	case GF_THUNDER:
			k_elec = dam / 3; /* 33% ELEC damage, total elec damage is saved in 'k_elec' */
			k_elec = elec_dam(Ind, k_elec, killer, -who);
			k_sound = dam / 3; /* 33% SOUND damage, total sound damage is saved in 'k_sound' */
			if (p_ptr->biofeedback) k_sound /= 2;
			if (p_ptr->resist_sound) {
				k_sound *= 5;
				k_sound /= (randint(6) + 6);
			}
			k_lite = dam / 3; /* 33% LIGHT damage, total light damage is saved in 'k_site' */
			if (p_ptr->resist_lite) {
				k_lite *= 4; k_lite /= (randint(6) + 6);
			} else if (p_ptr->suscep_lite) {
				k_lite *= 2;
			}
			dam = k_elec + k_sound + k_lite;
			if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			take_hit(Ind, dam, killer, -who);
			if (!p_ptr->resist_sound)
				(void)set_stun(Ind, p_ptr->stun + randint(15));
			if (!p_ptr->resist_lite && !blind && !p_ptr->resist_blind)
				(void)set_blind(Ind, p_ptr->blind + randint(5) + 2);
			break;

		/* Druid's early tox spell: 50% poison, 50% water. No side effects from the water attk */
	case GF_WATERPOISON:
			switch (randint(2)) {
				case 1: { // Poison!
					if (p_ptr->immune_poison) {
						dam = 0;
						//not really needed-- if (who == PROJECTOR_TERRAIN) ; else
						if (fuzzy) msg_format(Ind, "You are hit by poison for \377%c%d \377wdamage!", damcol, dam);
						else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
					} else {
						if (p_ptr->resist_pois) dam = (dam + 2) / 3;
						if (p_ptr->oppose_pois) dam = (dam + 2) / 3;
						if (p_ptr->suscep_pois && !(p_ptr->resist_pois || p_ptr->oppose_pois)) dam = (dam + 2) * 2;
						if (fuzzy) msg_format(Ind, "You are hit by poison for \377%c%d \377wdamage!", damcol, dam);
						else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
						take_hit(Ind, dam, killer, -who);
						if (!(p_ptr->resist_pois || p_ptr->oppose_pois))
						{
							(void)set_poisoned(Ind, p_ptr->poisoned + rand_int(dam) + 10, -who);
						}
					}
				}
				default: { // Water
					if (p_ptr->immune_water) {
						dam = 0;
						//not really needed-- if (who == PROJECTOR_TERRAIN) ; else
						if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
						else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
					} else {
						if (p_ptr->body_monster && (r_ptr->flags7 & RF7_AQUATIC)) {
							dam = (dam + 3) / 4;
						} else if (p_ptr->resist_water) {
							dam = (dam + 2) / 3;
						}
						if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
						else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
						if (TOOL_EQUIPPED(p_ptr) != SV_TOOL_TARPAULIN && magik(20 + dam / 20) &&
							!(p_ptr->body_monster && (r_ptr->flags7 & RF7_AQUATIC)))
						{
							if (!p_ptr->resist_water || magik(50)) {
								/* Don't kill inventory in bloodbond... */
								int breakable = 1;
								if (IS_PVP) {
									if (check_blood_bond(Ind, -who)) {
										breakable = 0;
									}
								}
								if (breakable) {
									inven_damage(Ind, set_water_destroy, 1);
									if (magik(50)) equip_damage(Ind, GF_WATER);
								}
							}
						}
					}
					take_hit(Ind, dam, killer, -who);
				}
			}
			break;

		/* Druid's higher tox spell: 60%shards, 10%water, 10%poison. No side effects from water/shards. */
	case GF_ICEPOISON:
			switch (randint(5)) {
				case 1:  // Shards
				case 3:
				case 5: {
					if (p_ptr->biofeedback) dam /= 2;
					if (p_ptr->resist_shard) { dam *= 6; dam /= (randint(6) + 6); }
					if (fuzzy) msg_format(Ind, "You are hit by something sharp for \377%c%d \377wdamage!", damcol, dam);
					else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
					take_hit(Ind, dam, killer, -who);
					break;
				}
				case 2: { // Water
					if (p_ptr->body_monster && (r_ptr->flags7 & RF7_AQUATIC)) dam = (dam + 3) / 4;
					if (p_ptr->immune_water) {
						dam = 0;
						//not really needed-- if (who == PROJECTOR_TERRAIN) ; else
						if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
						else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
					} else {
						if (p_ptr->resist_water) dam = (dam + 2) / 3;
						if (fuzzy) msg_format(Ind, "You are hit by something for \377%c%d \377wdamage!", damcol, dam);
						else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
						if (TOOL_EQUIPPED(p_ptr) != SV_TOOL_TARPAULIN && magik(20 + dam / 20) &&
						    !(p_ptr->body_monster && (r_ptr->flags7 & RF7_AQUATIC))) {
							if (!p_ptr->resist_water || magik(50)) {
								/* Don't kill inventory in bloodbond... */
								int breakable = 1;
								if (IS_PVP && check_blood_bond(Ind, -who)) breakable = 0;
								if (breakable) {
									inven_damage(Ind, set_water_destroy, 1);
									if (magik(25)) equip_damage(Ind, GF_WATER);
								}
							}
						}
						take_hit(Ind, dam, killer, -who);
					}
					break;
				}
				default: { // Poison
					if (p_ptr->immune_poison) {
						dam = 0;
						//not really needed-- if (who == PROJECTOR_TERRAIN) ; else
						if (fuzzy) msg_format(Ind, "You are hit by poison for \377%c%d \377wdamage!", damcol, dam);
						else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
					} else {
						if (p_ptr->resist_pois) dam = (dam + 2) / 3;
						if (p_ptr->oppose_pois) dam = (dam + 2) / 3;
						if (p_ptr->suscep_pois && !(p_ptr->resist_pois || p_ptr->oppose_pois)) dam = (dam + 2) * 2;
						if (fuzzy) msg_format(Ind, "You are hit by poison for \377%c%d \377wdamage!", damcol, dam);
						else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
						take_hit(Ind, dam, killer, -who);
						if (!(p_ptr->resist_pois || p_ptr->oppose_pois)) {
							(void)set_poisoned(Ind, p_ptr->poisoned + rand_int(dam) + 10, -who);
						}
					}
					break;
				}
			}
			break;

	case GF_CURSE: {
			int curse = randint(3);
			if (curse == 1) { //Slow
				if (fuzzy) msg_print(Ind, "Your body seems difficult to move!");
				else msg_format(Ind, "%s curses at you, slowing your movements!", killer);
				if (p_ptr->pspeed <= 100) {
					/* unaffected */
				} else if (p_ptr->mindboost && magik(p_ptr->mindboost_power)) {
					/* resist the effect */
				} else if (!p_ptr->free_act) {
					(void)set_slow(Ind, p_ptr->slow + rand_int(3) + 4);
				} else {
					(void)set_slow(Ind, p_ptr->slow + rand_int(3) + 2);
				}
				dam = 0;
			} else if (curse == 2) { //Conf
				dam = damroll(3, (dam / 2)) + 1;
				if (p_ptr->resist_conf || p_ptr->resist_chaos){
					if (fuzzy) msg_format(Ind, "Your mind is hit by confusion for \377%c%d \377wdamage!", damcol, dam);
					else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
					dam *= 5; dam /= (randint(6) + 6);
				}else{
					if (fuzzy) msg_format(Ind, "Your mind is hit by confusion for \377%c%d \377wdamage!", damcol, dam);
					else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
					if (!(p_ptr->mindboost && magik(p_ptr->mindboost_power)))
						(void)set_confused(Ind, p_ptr->confused + randint(5) + 5);
				}
				take_hit(Ind, dam, killer, -who);
			} else { //Blind
				if (fuzzy) msg_print(Ind, "Your eyes suddenly burn!");
				else msg_format(Ind, "%s casts a spell, burning your eyes!", killer);
				if (!blind && !p_ptr->resist_blind) (void)set_blind(Ind, p_ptr->blind + randint(5) + 2);
				dam = 0;
			}
			break;
			}

		/* Teleport other -- Teleports */
	case GF_AWAY_ALL:
			if (p_ptr->martyr) break;

			if (IS_PVP) {
				/* protect players in inns */
				cave_type **zcave;
				cave_type *c_ptr;
				if ((zcave = getcave(wpos))) {
					c_ptr = &zcave[p_ptr->py][p_ptr->px];
					if (f_info[c_ptr->feat].flags1 & FF1_PROTECTED) break;
				}

				/* protect AFK players */
				if (p_ptr->afk) break;

				/* Log PvP teleports - mikaelh */
				s_printf("%s teleported (GF_AWAY_ALL) %s at (%d,%d,%d).\n", Players[0 - who]->name, p_ptr->name,
					p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);

				/* Tell the one who caused it */
				msg_format(0 - who, "You teleport %s away.", Players[0 - who]->play_vis[Ind] ? p_ptr->name : "It");
			}

			if (fuzzy) msg_print(Ind, "Someone teleports you away!");
			else msg_format(Ind, "%^s teleports you away!", killer);
			teleport_player(Ind, 200, TRUE);
			break;

	case GF_WRAITH_PLAYER:
			if (fuzzy) msg_print(Ind, "You feel less constant!");
			else msg_format(Ind, "%^s turns you into a wraith!", killer);
			set_tim_wraith(Ind, dam);
			break;

	case GF_BLESS_PLAYER:
			if (dam < 18) p_ptr->blessed_power = 8;
			else if (dam < 33) p_ptr->blessed_power = 14;
			else p_ptr->blessed_power = 20;
			(void)set_blessed(Ind, dam);
			break;

		/* a special family, actually cumulative.. hacky, yeah */
	case GF_REMFEAR_PLAYER:		/* just fear */
			set_afraid(Ind, 0);
			/* Stay bold for some turns */
			(void)set_res_fear(Ind, dam);
			break;
	case GF_REMCONF_PLAYER:		/* confusion, and ALSO fear */
			set_afraid(Ind, 0);
			/* Stay bold for some turns */
			(void)set_res_fear(Ind, dam);

			set_confused(Ind, 0);
			break;
	case GF_REMIMAGE_PLAYER:	/* hallu, and ALSO conf/fear */
			set_afraid(Ind, 0);
			/* Stay bold for some turns */
			(void)set_res_fear(Ind, dam);

			set_confused(Ind, 0);

			set_image(Ind, 0);
			break;

	case GF_REMCURSE_PLAYER: /* only remove one curse */
#ifdef NEW_REMOVE_CURSE
		if (dam) remove_curse_aux(Ind, 0x1 + 0x2, -who);
		else remove_curse_aux(Ind, 0x2, -who);
#endif
		break;

	case GF_HPINCREASE_PLAYER:
			(void)hp_player(Ind, dam);
			break;

	case GF_HERO_PLAYER:
			(void)set_hero(Ind, dam); /* removed stacking */
			break;

	case GF_SHERO_PLAYER:
			(void)set_shero(Ind, dam); /* removed stacking */
			break;

	case GF_SATHUNGER_PLAYER:
		if (!p_ptr->suscep_life) {
			(void)set_food(Ind, PY_FOOD_MAX - 1);
			//if (p_ptr->male) msg_format_near(Ind, "\377y%s looks like he is going to explode.", p_ptr->name);
			//else msg_format_near(Ind, "\377y%s looks like she is going to explode.", p_ptr->name);
		}
			break;

	case GF_RESFIRE_PLAYER:
			(void)set_oppose_fire(Ind, dam); /* removed stacking */
			break;

	case GF_RESELEC_PLAYER:
			(void)set_oppose_elec(Ind, dam); /* removed stacking */
			break;

	case GF_RESPOIS_PLAYER:
			(void)set_oppose_pois(Ind, dam); /* removed stacking */
			break;

	case GF_RESACID_PLAYER:
			(void)set_oppose_acid(Ind, dam); /* removed stacking */
			break;

	case GF_RESCOLD_PLAYER:
			(void)set_oppose_cold(Ind, dam); /* removed stacking */
			break;

	case GF_CUREPOISON_PLAYER:
			(void)set_poisoned(Ind, 0, 0);
			break;

	case GF_SLOWPOISON_PLAYER:
			if (p_ptr->poisoned && !p_ptr->slow_poison) p_ptr->slow_poison = 1;
			break;

	case GF_SEEINVIS_PLAYER:
			(void)set_tim_invis(Ind, dam); /* removed stacking */
			break;

	case GF_ZEAL_PLAYER:
			(void)set_zeal(Ind, dam, 9 + randint(5));
			break;
	case GF_SEEMAP_PLAYER:
			map_area(Ind);
			break;
	case GF_DETECTCREATURE_PLAYER:
 			(void)detect_creatures(Ind);
			break;

	case GF_CURESTUN_PLAYER:
			(void)set_stun(Ind, p_ptr->stun - dam);
			break;

	case GF_DETECTDOOR_PLAYER:
			(void)detect_sdoor(Ind, DEFAULT_RADIUS);
			break;

	case GF_DETECTTRAP_PLAYER:
			(void)detect_trap(Ind, DEFAULT_RADIUS);
			break;

	case GF_CURECUT_PLAYER:
			(void)set_cut(Ind, p_ptr->cut - dam, -who);
			break;

	case GF_TELEPORTLVL_PLAYER:
			if (p_ptr->martyr) break;
			teleport_player_level(Ind, FALSE);
			break;

	case GF_HEAL_PLAYER:
			/* hacks */
			if (hack_dam & 0x2000) /* CCW */
				(void)set_stun(Ind, 0);
			if (hack_dam & 0x1000) /* CSW */
				(void)set_confused(Ind, 0);
			if (hack_dam & 0x800) { /* CLW */
				(void)set_blind(Ind, 0);
				(void)set_cut(Ind, 0, 0);
			}
			if (hack_dam & 0x400) { /* holy curing PBAoE */
				if (Ind != -who) dam = (dam * 3) / 2; /* heals allies for 3/4 of the self-heal amount */
			}

#if 0 /* causes a prob with 'Cure Wounds' spell - targetting cannot determine if to skip vampires or not -_- */
			if (p_ptr->ghost || (r_ptr->flags3 & RF3_UNDEAD) || p_ptr->suscep_life) {
				if (rand_int(100) < p_ptr->skill_sav)
					msg_print(Ind, "You shudder, but you resist the effect!");
				else {
					/* Message */
					msg_print(Ind, "You somehow feel.. cleaned!");
					dam /= 3; /* full dam is too harsh */

					/* Don't let the player get killed by it - mikaelh */
					if (p_ptr->chp <= dam) dam = p_ptr->chp - 1;
					take_hit(Ind, dam, killer, -who);
				}
			} else
#endif
			{
				//(spammy) msg_format_near(Ind, "\377g%s has been healed for %d hit points!.", p_ptr->name, dam);
				if (IS_PLAYER(-who) /* paranoia? */
				     && -who != Ind) { /* don't notify ourselves about healing ourselves */
					msg_format(-who, "\377w%s has been healed for \377g%d\377w hit points.", p_ptr->name, dam);
					msg_format(Ind, "\377g%s heals you for %d hit points", Players[-who]->name, dam);
				} else msg_format(Ind, "\377gYou are healed for %d hit points", dam);

				hp_player_quiet(Ind, dam, FALSE);
				dam = 0;
			}
			break;

		/* The druid spell; Note that it _damages_ undead-players instead of healing :-) - the_sandman */
	case GF_HEALINGCLOUD:
			if (p_ptr->ghost || p_ptr->suscep_good || p_ptr->suscep_life) {
				if (rand_int(100) < p_ptr->skill_sav)
					msg_print(Ind, "You shudder, but you resist the effect!");
				else {
					/* Message */
					msg_print(Ind, "You somehow feel.. cleaned!");
					dam /= 3; /* full dam is too harsh */

					/* Don't let the player get killed by it - mikaelh */
					if (p_ptr->chp <= dam) dam = p_ptr->chp - 1;
					take_hit(Ind, dam, killer, -who);
				}
			} else {
				/* Healed */
				msg_format(Ind, "\377gYou are healed for %d points.", dam);
				//(spammy) msg_format_near(Ind, "\377g%s has been healed for %d hit points.", p_ptr->name, dam);

				hp_player_quiet(Ind, dam, FALSE);
				dam = 0;
			}
			break;

	case GF_SPEED_PLAYER:
			if (dam > 10) dam = 10; /* cap speed bonus for others */
			if (dam > p_ptr->lev / 3) dam = p_ptr->lev / 3; /* works less well for low level targets */
			if (dam < 1) dam = 1;

			if (fuzzy) msg_print(Ind, "You feel faster!");
			else msg_format(Ind, "%^s speeds you up!", killer);
			(void)set_fast(Ind, 10 + (dam * 5), dam); /* removed stacking */
			break;

	case GF_EXTRA_STATS: //unused
			do_xtra_stats(Ind, dam / 100, dam % 100, 10 + rand_int(5) + (dam % 100) * 2);
			break;

	case GF_EXTRA_SPR: //unused
			dam = dam - 30;
			if (dam < 0) break;
			do_focus(Ind, dam / 6, 30 + dam);
			break;

	case GF_SHIELD_PLAYER: //unused
			if (dam > 10) dam = 10; /* cap AC bonus for others - Kurzel */
			if (fuzzy) msg_print(Ind, "You feel protected!");
			else msg_format(Ind, "%^s shields you!", killer);

			(void)set_shield(Ind, 10 + (dam * 5), dam, SHIELD_NONE, 0, 0); /* removed stacking */
			break;

	case GF_TELEPORT_PLAYER:
			if (p_ptr->martyr) break;

			if (IS_PVP) {
				/* protect players in inns */
				cave_type **zcave;
				cave_type *c_ptr;
				if ((zcave = getcave(wpos))) {
					c_ptr = &zcave[p_ptr->py][p_ptr->px];
					if (f_info[c_ptr->feat].flags1 & FF1_PROTECTED) break;
				}

				/* protect AFK players */
				if (p_ptr->afk) break;

				/* Log PvP teleports - mikaelh */
				s_printf("%s teleported (GF_TELEPORT_PLAYER) %s at (%d,%d,%d).\n", Players[0 - who]->name, p_ptr->name,
					p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);

				/* Tell the one who caused it */
				msg_format(0 - who, "You teleport %s away.", Players[0 - who]->play_vis[Ind] ? p_ptr->name : "It");
			}

			if (fuzzy) msg_print(Ind, "Someone teleports you away!");
			else msg_format(Ind, "%^s teleports you away!", killer);

			teleport_player(Ind, dam, TRUE);
			break;

	case GF_RECALL_PLAYER:
			dam = 0;
			if (p_ptr->afk) break;

			/* prevent silly things */
			if (iddc_recall_fail(p_ptr, l_ptr)
#ifdef ANTI_TELE_CHEEZE
			    || p_ptr->anti_tele
#endif
			    ) {
				msg_print(Ind, "\377oThere is some static discharge in the air around you, but nothing happens.");
				break;
			}

			if (!p_ptr->word_recall) {
#ifdef SEPARATE_RECALL_DEPTHS
				p_ptr->recall_pos.wz = get_recall_depth(&p_ptr->wpos, p_ptr);
#else
				p_ptr->recall_pos.wz = p_ptr->max_dlv;
#endif
				p_ptr->word_recall = dam;
				if (fuzzy) msg_print(Ind, "\377oThe air around you suddenly becomes charged!");
				else msg_format(Ind, "\377o%^s charges the air around you!", killer);
			} else {
				p_ptr->word_recall = 0;
				if (fuzzy) msg_print(Ind, "\377oA tension leaves the air around you!");
				else msg_format(Ind, "\377o%^s dispels the tension from the air around you!", killer);
			}
			p_ptr->redraw |= (PR_DEPTH);
			break;

	case GF_RESTORE_PLAYER:
			if (dam & 0x1) { /* Restore food */
				if (!p_ptr->suscep_life) {
					set_food(Ind, PY_FOOD_MAX - 1);
					//if (p_ptr->male) msg_format_near(Ind, "\377y%s looks like he is going to explode.", p_ptr->name);
					//else msg_format_near(Ind, "\377y%s looks like she is going to explode.", p_ptr->name);
				}
			}
			if (dam & 0x2) { /* Restore exp */
				(void)restore_level(Ind);
			}
			if (dam & 0x4) { /* Restore stats */
				(void)do_res_stat(Ind, A_STR);
				(void)do_res_stat(Ind, A_CON);
				(void)do_res_stat(Ind, A_DEX);
				(void)do_res_stat(Ind, A_WIS);
				(void)do_res_stat(Ind, A_INT);
				(void)do_res_stat(Ind, A_CHR);
			}
			if (dam & 0x8) { /* Black breath (herbal tea) */
				if (p_ptr->black_breath) {
					msg_print(Ind, "The hold of the Black Breath on you is broken!");
					p_ptr->black_breath = FALSE;
				}
			}
			break;
	case GF_CURING:
	case GF_CURE_PLAYER:
			if (dam & 0x1) { /* Slow Poison */
				if (p_ptr->poisoned && !p_ptr->slow_poison) p_ptr->slow_poison = 1;
			}
			if (dam & 0x2) { /* Ungorge */
				if (p_ptr->food >= PY_FOOD_MAX) set_food(Ind, PY_FOOD_MAX - 1);
			}
			if (dam & 0x4) /* Neutralise Poison */
				(void)set_poisoned(Ind, 0, 0);
			if (dam & 0x8) /* Close cuts */
				(void)set_cut(Ind, 0, 0);
			if (dam & 0x10) { /* Remove conf/blind/stun */
				(void)set_confused(Ind, 0);
				(void)set_blind(Ind, 0);
			}
			if (dam & 0x20) /* Remove hallu */
				set_image(Ind, 0);
			if (dam & 0x40) { /* Restore stats */
				(void)do_res_stat(Ind, A_STR);
				(void)do_res_stat(Ind, A_CON);
				(void)do_res_stat(Ind, A_DEX);
				(void)do_res_stat(Ind, A_WIS);
				(void)do_res_stat(Ind, A_INT);
				(void)do_res_stat(Ind, A_CHR);
			}
			if (dam & 0x80) /* Restore exp */
				(void)restore_level(Ind);
			if (dam & 0x100) (void)set_stun(Ind, 0);
			break;
	case GF_RESURRECT_PLAYER:
			//if (p_ptr->ghost)
			resurrect_player(Ind, dam);
			break;
	case GF_SANITY_PLAYER:
			msg_format(Ind, "%s waves over your eyes, murmuring some words..", killer);
			set_afraid(Ind, 0);
			/* Stay bold for some turns */
			(void)set_res_fear(Ind, dam);
			(void)set_confused(Ind, 0);
			(void)set_image(Ind, 0);

			if (dam > 0) {
#if 1
				/* Testing new version that increases sanity - mikaelh */
				p_ptr->csane += dam;
				if (p_ptr->csane > p_ptr->msane) p_ptr->csane = p_ptr->msane;
				p_ptr->update |= PU_SANITY;
				p_ptr->redraw |= PR_SANITY;
				p_ptr->window |= (PW_PLAYER);
#else
				if (p_ptr->csane < p_ptr->msane * dam / 12) {
					p_ptr->csane = p_ptr->msane * dam / 12;
					/* the_sandman mika's addition, really */
					if (p_ptr->csane>p_ptr->msane) p_ptr->csane = p_ptr->msane;
							p_ptr->update |= PU_SANITY;
							p_ptr->redraw |= PR_SANITY;
							p_ptr->window |= (PW_PLAYER);
				}
#endif
				/* Give feedback to the healer so he knows when he may stop - C. Blue */
				if (p_ptr->csane == p_ptr->msane) msg_format_near(Ind, "%s appears to be in full command of %s mental faculties.", p_ptr->name, p_ptr->male ? "his" : "her");
			}
			break;
	case GF_SOULCURE_PLAYER:
			/* priest variant hurts undead, druid tea is fine */
			if (p_ptr->suscep_life) {
				msg_print(Ind, "You feel a calming warmth touching your soul.");
				take_hit(Ind, (p_ptr->chp / 3) * 2, killer, -who);
			}
			if (p_ptr->black_breath) {
				msg_print(Ind, "The hold of the Black Breath on you is broken!");
				p_ptr->black_breath = FALSE;
			}
			break;
	case GF_MINDBOOST_PLAYER:
			if (dam > 95) dam = 95;
			set_mindboost(Ind, dam, 15 + randint(6));
			break;

	case GF_TERROR:
		if (fuzzy || self) msg_print(Ind, "You hear terrifying noises!");
		else msg_format(Ind, "%^s creates a disturbing illusion!", killer);

		if (p_ptr->resist_conf ||
		    (p_ptr->mindboost && magik(p_ptr->mindboost_power)))
			msg_print(Ind, "You disbelieve the feeble spell.");
		else if (rand_int(100 + dam*6) < p_ptr->skill_sav)
			msg_print(Ind, "You disbelieve the feeble spell.");
		//else set_confused(Ind, p_ptr->confused + dam); too much for pvp
		else set_confused(Ind, p_ptr->confused + 2 + rand_int(3));

		if (p_ptr->resist_fear) msg_print(Ind, "You refuse to be frightened.");
		else if (rand_int(100 + dam*6) < p_ptr->skill_sav) msg_print(Ind, "You refuse to be frightened.");
		else {
			//(void)set_afraid(Ind, p_ptr->afraid + dam);  too much for pvp
			(void)set_afraid(Ind, p_ptr->afraid + 2 + rand_int(3));
		}

		dam = 0;
		break;

	case GF_OLD_CONF:
		if (fuzzy || self) msg_print(Ind, "You hear puzzling noises!");
		else msg_format(Ind, "%^s creates a mesmerising illusion!", killer);

		if (p_ptr->resist_conf ||
		    (p_ptr->mindboost && magik(p_ptr->mindboost_power)))
			msg_print(Ind, "You disbelieve the feeble spell.");
		else if (rand_int(100 + dam*6) < p_ptr->skill_sav && !(p_ptr->esp_link_flags & LINKF_OPEN)) /* An open mind invites psi attacks */
			msg_print(Ind, "You disbelieve the feeble spell.");
		//else set_confused(Ind, p_ptr->confused + dam); too much for pvp
		else set_confused(Ind, p_ptr->confused + 2 + rand_int(3));

		dam = 0;
		break;

	case GF_OLD_SLOW:
		if (fuzzy || self) msg_print(Ind, "Something drains power from your muscles!");
		else msg_format(Ind, "%^s drains power from your muscles!", killer);

		if (p_ptr->free_act) msg_print(Ind, "You are unaffected!");
		else if (rand_int(100 + dam*6) < p_ptr->skill_sav ||
		    (p_ptr->mindboost && magik(p_ptr->mindboost_power)))
			msg_print(Ind, "You resist the effects!");
		//else set_slow(Ind, p_ptr->slow + dam); too much for pvp..
		else set_slow(Ind, p_ptr->slow + 2 + rand_int(3));

		dam = 0;
		break;

	case GF_TURN_ALL:

		if (fuzzy || self) msg_print(Ind, "Something mumbles, and you hear scary noises!");
		else msg_format(Ind, "%^s casts a fearful illusion!", killer);

		if (p_ptr->resist_fear) msg_print(Ind, "You refuse to be frightened.");
		else if (rand_int(100 + dam*6) < p_ptr->skill_sav) msg_print(Ind, "You refuse to be frightened.");
		else {
			//(void)set_afraid(Ind, p_ptr->afraid + dam);  too much for pvp
			(void)set_afraid(Ind, p_ptr->afraid + 2 + rand_int(3));
		}

		dam = 0;
		break;

	case GF_OLD_POLY:
			s_printf("PLAYER_POLY: %s -> %s ", Players[0 - who]->name, p_ptr->name);
			if (p_ptr->afk) break;
			if (fuzzy || self) msg_print(Ind, "You feel bizzare!");
			else msg_format(Ind, "%^s polymorphs you!", killer);
			if (p_ptr->resist_nexus) {
				s_printf("resists\n");
				msg_print(Ind, "You resist the effects!");
			} else {
				s_printf("morphs\n");
				msg_print(Ind, "The magic continuum twists around you!");
				apply_morph(Ind, dam, killer, -who);
			}
			break;

	case GF_HAVOC:
		{
			bool ignore_heat = (p_ptr->resist_fire && p_ptr->oppose_fire) || p_ptr->immune_fire;

			if (p_ptr->resist_shard) dam = (dam * 5) / 6;
			if (p_ptr->resist_sound) dam = (dam * 5) / 6;
			if (p_ptr->immune_fire) dam = (dam * 4) / 5;
			else if (p_ptr->resist_fire || p_ptr->oppose_fire) dam = (dam * 5) / 6;
			if (p_ptr->resist_mana) dam = (dam * 5) / 6;
			if (p_ptr->resist_chaos) dam = (dam * 5) / 6;

			if (fuzzy) msg_format(Ind, "There is a shockwave around you of \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);

			if (!p_ptr->resist_shard && !p_ptr->no_cut)
				(void)set_cut(Ind, p_ptr->cut + ( dam / 2), -who );
			if (!p_ptr->resist_sound)
				(void)set_stun(Ind, p_ptr->stun + randint(20));

			if (!p_ptr->resist_shard || !p_ptr->resist_sound || !ignore_heat) {
				/* Don't kill inventory in bloodbond... */
				int breakable = 1;
				if (IS_PVP && check_blood_bond(Ind, -who)) breakable = 0;
				if (breakable) {
					if (p_ptr->resist_shard && p_ptr->resist_sound) inven_damage(Ind, set_fire_destroy, 3);
					else if (ignore_heat) inven_damage(Ind, set_impact_destroy, 3);
					else inven_damage(Ind, set_rocket_destroy, 4);
				}
			}

			take_hit(Ind, dam, killer, -who);
			break;
		}

	/* Rocket -- stun, cut, fire, raw impact */
	case GF_INFERNO:
	case GF_DETONATION:
	case GF_ROCKET:
		{
			bool ignore_heat = (p_ptr->resist_fire && p_ptr->oppose_fire) || p_ptr->immune_fire;

			if (p_ptr->resist_shard) dam = (dam * 5) / 6;
			if (p_ptr->resist_sound) dam = (dam * 5) / 6;
			if (p_ptr->immune_fire) dam = (dam * 3) / 4;
			else if (p_ptr->resist_fire || p_ptr->oppose_fire) dam = (dam * 4) / 5;

			if (fuzzy) msg_format(Ind, "There is an explosion around you of \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);

			if (!p_ptr->resist_shard && !p_ptr->no_cut)
				(void)set_cut(Ind, p_ptr->cut + ( dam / 2), -who );
			if (!p_ptr->resist_sound)
				(void)set_stun(Ind, p_ptr->stun + randint(20));

			if (!p_ptr->resist_shard || !p_ptr->resist_sound || !ignore_heat) {
				/* Don't kill inventory in bloodbond... */
				int breakable = 1;
				if (IS_PVP && check_blood_bond(Ind, -who)) breakable = 0;
				if (breakable) {
					if (p_ptr->resist_shard && p_ptr->resist_sound) inven_damage(Ind, set_fire_destroy, 3);
					else if (ignore_heat) inven_damage(Ind, set_impact_destroy, 3);
					else inven_damage(Ind, set_rocket_destroy, 4);
				}
			}

			take_hit(Ind, dam, killer, -who);
			break;
		}

		/* Standard damage -- also poisons / mutates player */
	case GF_NUKE:
			if (p_ptr->immune_poison) {
				dam = 0;
				if (who == PROJECTOR_TERRAIN) ; else /* RL-Doom? ;) */
				if (fuzzy) msg_format(Ind, "You are hit by radiation for \377%c%d \377wdamage!", damcol, dam);
				else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			} else {
				if (p_ptr->resist_pois) dam = (2 * dam + 2) / 5;
				if (p_ptr->oppose_pois) dam = (2 * dam + 2) / 5;
				if (p_ptr->suscep_pois && !(p_ptr->resist_pois || p_ptr->oppose_pois)) dam = (5 * dam + 2) / 3;
				if (fuzzy) msg_format(Ind, "You are hit by radiation for \377%c%d \377wdamage!", damcol, dam);
				else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
				take_hit(Ind, dam, killer, -who);
				if (!(p_ptr->resist_pois || p_ptr->oppose_pois)) {
					set_poisoned(Ind, p_ptr->poisoned + rand_int(dam) + 10, -who);

#if 0	// dang, later..
					if (randint(5) == 1) { /* 6 */
						msg_print("You undergo a freakish metamorphosis!");
						if (randint(4)==1) /* 4 */ do_poly_self();
						else corrupt_player();
					}
#endif	// 0

					if (randint(6) == 1) {
						/* Don't kill inventory in bloodbond... */
						int breakable = 1;
						if (IS_PVP && check_blood_bond(Ind, -who)) breakable = 0;
						if (breakable) inven_damage(Ind, set_acid_destroy, 2);
					}
				}
			}
			break;

		/* Standard damage */
	case GF_DISINTEGRATE:
			if (p_ptr->body_monster && (r_ptr->flags3 & RF3_HURT_ROCK)) dam = (dam * 3) / 2;
			if (fuzzy) msg_format(Ind, "You are hit by pure energy for \377%c%d \377wdamage!", damcol, dam);
			else msg_format(Ind, "%s \377%c%d \377wdamage!", attacker, damcol, dam);
			take_hit(Ind, dam, killer, -who);
			break;

		/* GF_BLIND - C. Blue */
	case GF_BLIND:
			if (p_ptr->resist_blind) msg_print(Ind, "You are unaffected!");
			else if (rand_int(100 + dam*6) < p_ptr->skill_sav) msg_print(Ind, "You resist the effects!");
			else if (!p_ptr->blind) (void)set_blind(Ind, dam);
			break;

		/* For shattering potions, but maybe work for wands too? */
	case GF_OLD_HEAL:
			if (fuzzy) msg_print(Ind, "You are hit by something invigorating!");
			(void)hp_player(Ind, dam);
			dam = 0;
			break;

	case GF_OLD_SPEED:
			if (fuzzy) msg_print(Ind, "You are hit by something!");
			(void)set_fast(Ind, randint(5), 10);
			dam = 0;
			break;

	case GF_TELE_TO:
		{
			//bool resists_tele = FALSE;
			//dun_level		*l_ptr = getfloor(wpos);
			int chance = (195 - p_ptr->skill_sav) / 2; 

			if (p_ptr->res_tele) chance = 50; 
			if (p_ptr->martyr) break;
			if (IS_PVP) {
				/* protect players in inns */
				cave_type **zcave;
				cave_type *c_ptr;
				if ((zcave = getcave(wpos))) {
					c_ptr = &zcave[p_ptr->py][p_ptr->px];
					if (f_info[c_ptr->feat].flags1 & FF1_PROTECTED) break;
				}

				/* protect AFK players */
				if (p_ptr->afk) break;

				/* Log PvP teleports - mikaelh */
				s_printf("%s teleported (GF_TELE_TO) %s at (%d,%d,%d).\n", Players[0 - who]->name, p_ptr->name,
					p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);

				/* Tell the one who caused it */
				msg_format(0 - who, "You command %s to return.", Players[0 - who]->play_vis[Ind] ? p_ptr->name : "It");
			}

			/* Teleport to nowhere..? */
			if (who >= 0 || who <= PROJECTOR_UNUSUAL) break;

			if (p_ptr->anti_tele) msg_print(Ind, "You are unaffected!");
			else if (magik(chance)) msg_print(Ind, "You resist the effect!");
			else {
				player_type *q_ptr = Players[0 - who];

				msg_format(Ind, "%^s commands you to return.", q_ptr->name);

				/* Prepare to teleport */
				teleport_player_to(Ind, q_ptr->py, q_ptr->px);
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		/* Hand of Doom */
	case GF_HAND_DOOM:
			/* Teleport to nowhere..? */
			if (who >= 0 || who <= PROJECTOR_UNUSUAL) break;

			msg_format(Ind, "%^s invokes the Hand of Doom!", Players[0 - who]->name);

			if (rand_int(100) < p_ptr->skill_sav) msg_print(Ind, "You resist the effects!");
			else {
				int dummy = (((s32b) ((65 + randint(25)) * (p_ptr->chp))) / 100);

				if (p_ptr->chp - dummy < 1) dummy = p_ptr->chp - 1;
				msg_print(Ind, "You feel your life fade away!");
				bypass_invuln = TRUE;
				take_hit(Ind, dummy, Players[0-who]->name, -who);
				bypass_invuln = FALSE;
				curse_equipment(Ind, 100, 20);
			}
			break;

	case GF_AWAY_UNDEAD:
			if (p_ptr->martyr) break;
			if (IS_PVP) {
				/* protect players in inns */
				cave_type **zcave;
				cave_type *c_ptr;
				if ((zcave = getcave(wpos))) {
					c_ptr = &zcave[p_ptr->py][p_ptr->px];
					if (f_info[c_ptr->feat].flags1 & FF1_PROTECTED) break;
				}

				/* protect AFK players */
				if (p_ptr->afk) break;

				/* Log PvP teleports - mikaelh */
				s_printf("%s teleported (GF_AWAY_UNDEAD) %s at (%d,%d,%d).\n", Players[0 - who]->name, p_ptr->name,
					p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			}

		if (p_ptr->ghost || p_ptr->suscep_life) {
			/* Only affect undead */
			if (p_ptr->res_tele) msg_print(Ind, "You are unaffected!");
			//else if (p_ptr->lev+50 < randint(150))
			else if (rand_int(100) < p_ptr->skill_sav) teleport_player(Ind, dam, TRUE);
			else msg_print(Ind, "You resist the effect!");
		}

			/* No "real" damage */
			dam = 0;
			break;

		/* Teleport evil (Use "dam" as "power") */
	case GF_AWAY_EVIL:
			if (p_ptr->martyr) break;
			if (IS_PVP) {
				/* protect players in inns */
				cave_type **zcave;
				cave_type *c_ptr;
				if ((zcave = getcave(wpos))) {
					c_ptr = &zcave[p_ptr->py][p_ptr->px];
					if (f_info[c_ptr->feat].flags1 & FF1_PROTECTED) break;
				}

				/* protect AFK players */
				if (p_ptr->afk) break;

				/* Log PvP teleports - mikaelh */
				s_printf("%s teleported (GF_AWAY_EVIL) %s at (%d,%d,%d).\n", Players[0 - who]->name, p_ptr->name,
					p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			}

			if (p_ptr->suscep_good) {
				/* Only affect evil */
				if (p_ptr->res_tele)
					msg_print(Ind, "You are unaffected!");
				//else if (p_ptr->lev+50 < randint(150))
				else if (rand_int(100) < p_ptr->skill_sav)
					teleport_player(Ind, dam, TRUE);
				else
					msg_print(Ind, "You resist the effect!");
			}
			/* No "real" damage */
			dam = 0;
			break;

#if 0	//already defined above for affecting ALL players!
		/* Teleport monster (Use "dam" as "power") */
	case GF_AWAY_ALL:
			if (p_ptr->martyr) break;
			//if (p_ptr->body_monster) { --commented out to be consistent with GF_DISP_ALL
			//if (p_ptr->body_monster) {
				bool resists_tele = FALSE;
				dun_level *l_ptr = getfloor(wpos);

				if (p_ptr->res_tele)
					msg_print(Ind, "You are unaffected!");
				//else if (p_ptr->lev+50 < randint(150))
				else if (rand_int(100) < p_ptr->skill_sav)
					teleport_player(Ind, dam, TRUE);
				else
					msg_print(Ind, "You resist the effect!");
			//}

			/* No "real" damage */
			dam = 0;
			break;
#endif

		/* Turn undead (Use "dam" as "power") */
	case GF_TURN_UNDEAD:
		/* Only affect undead */
		if (p_ptr->ghost || p_ptr->suscep_life) {
			if (p_ptr->resist_fear && !p_ptr->ghost) msg_print(Ind, "You are unaffected!");
			/* Attempt a saving throw */
			//else if (RES_OLD(p_ptr->lev, dam))
			else if (rand_int(100) < p_ptr->skill_sav) msg_print(Ind, "You resist the effect!");
			else (void)set_afraid(Ind, p_ptr->afraid + damroll(3, (dam / 2)) + 1);
		}

			/* No "real" damage */
			dam = 0;
			break;

		/* Turn evil (Use "dam" as "power") */
	case GF_TURN_EVIL:
			/* Only affect evil */
			if (p_ptr->suscep_good) {
				if (p_ptr->resist_fear) msg_print(Ind, "You are unaffected!");
				else if (rand_int(100) < p_ptr->skill_sav)
				//else if (RES_OLD(p_ptr->lev, dam))
					msg_print(Ind, "You resist the effect!");
				else (void)set_afraid(Ind, p_ptr->afraid + damroll(3, (dam / 2)) + 1);
			}
			/* No "real" damage */
			dam = 0;
			break;

#if 0	//already defined above for affecting ALL players!
		/* Turn monster (Use "dam" as "power") */
	case GF_TURN_ALL:
		//if (p_ptr->body_monster) { --commented out to be consistent with GF_DISP_ALL
			/* Attempt a saving throw */
			if ((p_ptr->resist_fear) || ...?...) msg_print(Ind, "You are unaffected!");
			else if (rand_int(100) < p_ptr->skill_sav)
			//else if (RES_OLD(p_ptr->lev, dam))
				msg_print(Ind, "You resist the effect!");
			else (void)set_afraid(Ind, p_ptr->afraid + damroll(3, (dam / 2)) + 1);
		//}
			/* No "real" damage */
			dam = 0;
			break;
#endif

			/* Dispel undead */
	case GF_DISP_UNDEAD:
			/* Only affect undead */
			if (p_ptr->ghost || p_ptr->suscep_life) {
				if (rand_int(100) < p_ptr->skill_sav) msg_print(Ind, "You shudder, but you resist the effect!");
				else {
					/* Message */
					msg_print(Ind, "You shudder!");
					dam /= 3; /* full dam is too harsh */
					take_hit(Ind, dam, killer, -who);
				}
			}
			/* Ignore other monsters */
			else {
				/* No damage */
				dam = 0;
			}
			break;

			/* Dispel evil */
	case GF_DISP_EVIL:
			/* Only affect evil */
			if (p_ptr->suscep_good) {
				if (rand_int(100) < p_ptr->skill_sav)
					msg_print(Ind, "You shudder, but you resist the effect!");
				else {
					msg_print(Ind, "You shudder!");
					dam /= 3; /* full dam is too harsh */
					take_hit(Ind, dam, killer, -who);
				}
			}

			/* Ignore other monsters */
			else {
				/* No damage */
				dam = 0;
			}
			break;

	case GF_DISP_DEMON:
			/* Only affect demons */
			if (p_ptr->body_monster && (r_ptr->flags3 & RF3_DEMON)) {
				if (rand_int(100) < p_ptr->skill_sav) msg_print(Ind, "You shudder, but you resist the effect!");
				else {
					/* Message */
					msg_print(Ind, "You shudder!");
					dam /= 3; /* full dam is too harsh */
					take_hit(Ind, dam, killer, -who);
				}
			}
			/* Ignore other monsters */
			else {
				/* No damage */
				dam = 0;
			}
			break;

		/* Dispel monster */
	case GF_DISP_ALL:
#if 0 /* well, since Maiar use it, let's make it work on all players (for PvP/BB) */
		if (p_ptr->body_monster) {
			if (rand_int(100) < p_ptr->skill_sav)
				msg_print(Ind, "You shudder, but you resist the effect!");
			else {
				/* Message */
				msg_print(Ind, "You shudder!");
				dam /= 4; /* full dam is too harsh */
				take_hit(Ind, dam, killer, -who);
			}
		}
#else
		if (rand_int(100) < p_ptr->skill_sav) msg_print(Ind, "You shudder, but you resist the effect!");
		else {
			/* Message */
			msg_print(Ind, "You shudder!");
			take_hit(Ind, dam, killer, -who);
		}
#endif
		break;

	/* mimic spell 'Cause Wounds' aka monsters' 'Curse' */
	case GF_CAUSE: {
		int chance = p_ptr->skill_sav;

		if (p_ptr->suscep_evil) chance = chance + (100 - chance) / 2;
		if (chance > 95) chance = 95;

		if (rand_int(100) < chance || p_ptr->no_cut) {
			msg_print(Ind, "You resist the effect!");
			break;
		}
		take_hit(Ind, dam, killer, -who);
		break; }

	/* Drain Life */
	case GF_OLD_DRAIN:
		/* lower damage in pvp */
		dam /= 2;

		/* Prevent internal overflow (int) - allow up to 35% leech */
		if (p_ptr->chp > 9362) dam = (p_ptr->chp / 100) * dam;
		else if (p_ptr->chp > 936) dam = ((p_ptr->chp / 10) * dam) / 10;
		else dam = (p_ptr->chp * dam) / 100;
		if (!dam) dam = 1;

		if (!fuzzy) {
			if (typ == GF_OLD_DRAIN) p_ptr->ret_dam = dam;
			else p_ptr->ret_dam = 0; /* paranoia */
		}

		if (p_ptr->prace == RACE_VAMPIRE) {
			dam = 0;
			p_ptr->ret_dam = 0;
		} else if (magik(p_ptr->skill_sav / 2)) {
			dam *= 3; dam /= (randint(6) + 6);
			if (!dam) dam = 1;
		}

		take_hit(Ind, dam, killer, -who);

		/* the_sandman: return 15% of the damage to player. This is special
		   since (we're not using the upto 35% rule because for the spell
		   (drain cloud) it will be too much) we aim for balance
		   HACK: the priest_spell variable is defined above.
		*/

		if (priest_spell) {
			if (!fuzzy && IS_PLAYER(-who)) {
				//msg_format(Ind, "\377gYou are healed for %d hit points", (dam * 15) / 100);
				if (dam) hp_player_quiet(-who, (dam * 15) / 100, TRUE);
			}
			p_ptr->ret_dam = 0;
		}

		break;

	case GF_ANNIHILATION:
		/* lower damage in pvp */
		dam /= 2;

		if (p_ptr->chp > 9362) dam = (p_ptr->chp / 100) * dam;
		else if (p_ptr->chp > 936) dam = ((p_ptr->chp / 10) * dam) / 10;
		else dam = (p_ptr->chp * dam) / 100;

		if (magik(p_ptr->skill_sav / 2)) {
			dam *= 3; dam /= (randint(6) + 6);
		}

		//no need to check for 1200 damage cap as it cannot be reached in pvp
		if (brief_rune_spell && dam > 600) dam = 600;

		if (!dam) dam = 1;
		take_hit(Ind, dam, killer, -who);
		break;

	/* Default */
	default:
		/* No damage */
		dam = 0;
		break;
	}

	/* Damage messages in pvp fights - mikaelh */
	if (who < 0 && who > PROJECTOR_UNUSUAL && dam > 0)
	//if (IS_PVP && dam > 0)
	{
		if (check_hostile(-who, Ind))
			msg_format(-who, "%s is hit for \377y%d \377wdamage.", p_ptr->name, dam);
	}

	/* Player was hit - mikaelh */
	p_ptr->got_hit = TRUE;

#if 0 //redundant?
	/* Skip non-connected players */
	if (p_ptr->conn != NOT_CONNECTED && (dam || who != PROJECTOR_TERRAIN)) {
		/* Disturb */
		disturb(Ind, 1, 0);
	}
#endif

	/* Return "Anything seen?" */
	return (obvious);
}









/*
 * Find the char to use to draw a moving bolt
 * It is moving (or has moved) from (x,y) to (nx,ny).
 * If the distance is not "one", we (may) return "*".
 */
static char bolt_char(int y, int x, int ny, int nx) {
	if ((ny == y) && (nx == x)) return '*';
	if (ny == y) return '-';
	if (nx == x) return '|';
	if ((ny-y) == (x-nx)) return '/';
	if ((ny-y) == (nx-x)) return '\\';
	return '*';
}



/*
 * Generic "beam"/"bolt"/"ball" projection routine.  -BEN-
 *
 * Input:
 *   who: Index of "source" monster (or "zero" if "player")
 *   rad: Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 *   y,x: Target location (or location to travel "towards")
 *   dam: Base damage roll to apply to affected monsters (or player)
 *   typ: Type of damage to apply to monsters (and objects)
 *   flg: Extra bit flags (see PROJECT_xxxx in "defines.h")
  *
 * Return:
 *   TRUE if any "effects" of the projection were observed, else FALSE
 *
 * Allows a monster (or player) to project a beam/bolt/ball of a given kind
 * towards a given location (optionally passing over the heads of interposing
 * monsters), and have it do a given amount of damage to the monsters (and
 * optionally objects) within the given radius of the final location.
 *
 * A "bolt" travels from source to target and affects only the target grid.
 * A "beam" travels from source to target, affecting all grids passed through.
 * A "ball" travels from source to the target, exploding at the target, and
 *   affecting everything within the given radius of the target location.
 *
 * Traditionally, a "bolt" does not affect anything on the ground, and does
 * not pass over the heads of interposing monsters, much like a traditional
 * missile, and will "stop" abruptly at the "target" even if no monster is
 * positioned there, while a "ball", on the other hand, passes over the heads
 * of monsters between the source and target, and affects everything except
 * the source monster which lies within the final radius, while a "beam"
 * affects every monster between the source and target, except for the casting
 * monster (or player), and rarely affects things on the ground.
 *
 * Two special flags allow us to use this function in special ways, the
 * "PROJECT_HIDE" flag allows us to perform "invisible" projections, while
 * the "PROJECT_JUMP" flag allows us to affect a specific grid, without
 * actually projecting from the source monster (or player).
 *
 * The player will only get "experience" for monsters killed by himself
 * Unique monsters can only be destroyed by attacks from the player
 *
 * Only 256 grids can be affected per projection, limiting the effective
 * "radius" of standard ball attacks to nine units (diameter nineteen).
 *
 * One can project in a given "direction" by combining PROJECT_THRU with small
 * offsets to the initial location (see "line_spell()"), or by calculating
 * "virtual targets" far away from the player.
 *
 * One can also use PROJECT_THRU to send a beam/bolt along an angled path,
 * continuing until it actually hits somethings (useful for "stone to mud").
 *
 * Bolts and Beams explode INSIDE walls, so that they can destroy doors.
 *
 * Balls must explode BEFORE hitting walls, or they would affect monsters
 * on both sides of a wall.  Some bug reports indicate that this is still
 * happening in 2.7.8 for Windows, though it appears to be impossible.
 *
 * We "pre-calculate" the blast area only in part for efficiency.
 * More importantly, this lets us do "explosions" from the "inside" out.
 * This results in a more logical distribution of "blast" treasure.
 * It also produces a better (in my opinion) animation of the explosion.
 * It could be (but is not) used to have the treasure dropped by monsters
 * in the middle of the explosion fall "outwards", and then be damaged by
 * the blast as it spreads outwards towards the treasure drop location.
 *
 * Walls and doors are included in the blast area, so that they can be
 * "burned" or "melted" in later versions.
 *
 * This algorithm is intended to maximize simplicity, not necessarily
 * efficiency, since this function is not a bottleneck in the code.
 *
 * We apply the blast effect from ground zero outwards, in several passes,
 * first affecting features, then objects, then monsters, then the player.
 * This allows walls to be removed before checking the object or monster
 * in the wall, and protects objects which are dropped by monsters killed
 * in the blast, and allows the player to see all affects before he is
 * killed or teleported away.  The semantics of this method are open to
 * various interpretations, but they seem to work well in practice.
 *
 * We process the blast area from ground-zero outwards to allow for better
 * distribution of treasure dropped by monsters, and because it provides a
 * pleasing visual effect at low cost.
 *
 * Note that the damage done by "ball" explosions decreases with distance.
 * This decrease is rapid, grids at radius "dist" take "1/dist" damage.
 *
 * Notice the "napalm" effect of "beam" weapons.  First they "project" to
 * the target, and then the damage "flows" along this beam of destruction.
 * The damage at every grid is the same as at the "center" of a "ball"
 * explosion, since the "beam" grids are treated as if they ARE at the
 * center of a "ball" explosion.
 *
 * Currently, specifying "beam" plus "ball" means that locations which are
 * covered by the initial "beam", and also covered by the final "ball", except
 * for the final grid (the epicenter of the ball), will be "hit twice", once
 * by the initial beam, and once by the exploding ball.  For the grid right
 * next to the epicenter, this results in 150% damage being done.  The center
 * does not have this problem, for the same reason the final grid in a "beam"
 * plus "bolt" does not -- it is explicitly removed.  Simply removing "beam"
 * grids which are covered by the "ball" will NOT work, as then they will
 * receive LESS damage than they should.  Do not combine "beam" with "ball".
 *
 * The array "gy[],gx[]" with current size "grids" is used to hold the
 * collected locations of all grids in the "blast area" plus "beam path".
 *
 * Note the rather complex usage of the "gm[]" array.  First, gm[0] is always
 * zero.  Second, for N>1, gm[N] is always the index (in gy[],gx[]) of the
 * first blast grid (see above) with radius "N" from the blast center.  Note
 * that only the first gm[1] grids in the blast area thus take full damage.
 * Also, note that gm[rad+1] is always equal to "grids", which is the total
 * number of blast grids.
 *
 * Note that once the projection is complete, (y2,x2) holds the final location
 * of bolts/beams, and the "epicenter" of balls.
 *
 * Note also that "rad" specifies the "inclusive" radius of projection blast,
 * so that a "rad" of "one" actually covers 5 or 9 grids, depending on the
 * implementation of the "distance" function.  Also, a bolt can be properly
 * viewed as a "ball" with a "rad" of "zero".
 *
 * Note that if no "target" is reached before the beam/bolt/ball travels the
 * maximum distance allowed (MAX_RANGE), no "blast" will be induced.  This
 * may be relevant even for bolts, since they have a "1x1" mini-blast.
 *
 * Some people have requested an "auto-explode ball attacks at max range"
 * option, which should probably be handled by this function.  XXX XXX XXX
 *
 * Note that for consistency, we "pretend" that the bolt actually takes "time"
 * to move from point A to point B, even if the player cannot see part of the
 * projection path.  Note that in general, the player will *always* see part
 * of the path, since it either starts at the player or ends on the player.
 *
 * Hack -- we assume that every "projection" is "self-illuminating".
 */
#if 0
bool lua_project(int who, int rad, struct worldpos *wpos, int y, int x, int dam, int typ, int flg, char attacker[80]) {
	return(project(who, rad, wpos, y, x, dam, typ, flg, attacker));
}
#endif
bool project(int who, int rad, struct worldpos *wpos_tmp, int y, int x, int dam, int typ, int flg, char *attacker) {
	int	i, j, t;
	int	 y1, x1, y2, x2;
	int	/*y0, x0,*/ y9, x9;
	int	dist, dist_hack = 0, true_dist = 0;
	int	who_can_see[26], num_can_see = 0;
	int	terrain_resistance = -1, terrain_damage = -1;
	bool	old_tacit = suppress_message, suppress_explosion = FALSE;

#ifdef OPTIMIZED_ANIMATIONS
	int path_y[MAX_RANGE];
	int path_x[MAX_RANGE];
	int path_num = 0;
#endif

	/* Number of grids in the "blast area" (including the "beam" path) */
	int grids = 0;
	int effect = 0;

	bool players_only = FALSE; /* spell affects players only */

	/* Coordinates of the affected grids */
	//byte gx[256], gy[256];
	//byte gx[tdi[PREPARE_RADIUS]], gy[tdi[PREPARE_RADIUS]];
	byte gx[512], gy[512];

	/* Encoded "radius" info (see above) */
	//byte gm[16];
	byte gm[PREPARE_RADIUS];

	/* for cave_proj(): allow it to travel onto certain terrain,
	   but not really much further - C. Blue */
	bool broke_on_terrain1 = FALSE, broke_on_terrain2 = FALSE;

	dun_level *l_ptr;
	cave_type **zcave;
	struct worldpos wpos_fix, *wpos = &wpos_fix;

	/* Affected location(s) */
	cave_type *c_ptr, *c_ptr2;

	/* Assume the player sees nothing */
	bool notice = FALSE;

	/* Assume the player has seen nothing */
	/*bool visual = FALSE;*/

	/* Assume the player has seen no blast grids */
	bool drawn = FALSE;

	/* Is the player blind? */
	/* Blindness is currently ignored for this function */
	/*bool blind;*/

	/* Non-unique monster names might start on lower-case, due to usage of snprintf() which doesn't know '%^s' formatting.. */
	char pattacker[MAX_CHARS];


	strcpy(pattacker, attacker);
	pattacker[0] = toupper(pattacker[0]);

	/* Hack: GF_CAUSE does travel by LoS but isn't a real bolt and actually unavoidable, except by saving throw! */
	if (typ == GF_CAUSE) flg |= PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;

	/* copy wpos in case it was a monster's wpos that gets erased from mon_take_hit() if monster dies */
	wpos_fix = *wpos_tmp;

	if (!(zcave = getcave(wpos))) return(FALSE);
	l_ptr = getfloor(wpos);

	/* Spells which never affect monsters, read:
	   Spells which we want to exclude from merely _waking monsters up_!
	   Note: Bolt spells will still be 'stopped' when hitting a monster. */
	if ((typ == GF_HEAL_PLAYER) || (typ == GF_WRAITH_PLAYER) ||
	    (typ == GF_SPEED_PLAYER) || (typ == GF_SHIELD_PLAYER) ||
	    (typ == GF_RECALL_PLAYER) || (typ == GF_BLESS_PLAYER) ||
	    (typ == GF_REMFEAR_PLAYER) || (typ == GF_SATHUNGER_PLAYER) ||
	    (typ == GF_REMCONF_PLAYER) || (typ == GF_REMIMAGE_PLAYER) ||
	    (typ == GF_RESFIRE_PLAYER) || (typ == GF_RESCOLD_PLAYER) ||
	    (typ == GF_CUREPOISON_PLAYER) || (typ == GF_SEEINVIS_PLAYER) ||
	    (typ == GF_SEEMAP_PLAYER) || (typ == GF_CURECUT_PLAYER) ||
	    (typ == GF_CURESTUN_PLAYER) || (typ == GF_DETECTCREATURE_PLAYER) ||
	    (typ == GF_DETECTDOOR_PLAYER) || (typ == GF_DETECTTRAP_PLAYER) ||
	    (typ == GF_TELEPORTLVL_PLAYER) || (typ == GF_RESPOIS_PLAYER) ||
	    (typ == GF_RESELEC_PLAYER) || (typ == GF_RESACID_PLAYER) ||
	    (typ == GF_HPINCREASE_PLAYER) || (typ == GF_HERO_PLAYER) ||
	    (typ == GF_SHERO_PLAYER) || (typ == GF_TELEPORT_PLAYER) ||
	    (typ == GF_ZEAL_PLAYER) || (typ == GF_MINDBOOST_PLAYER) ||
	    (typ == GF_RESTORE_PLAYER) || (typ == GF_REMCURSE_PLAYER) ||
	    (typ == GF_CURE_PLAYER) || (typ == GF_RESURRECT_PLAYER) ||
	    (typ == GF_SANITY_PLAYER) || (typ == GF_SOULCURE_PLAYER) ||
	    (typ == GF_IDENTIFY) || (typ == GF_SLOWPOISON_PLAYER))
		players_only = TRUE;


#ifdef PROJECTION_FLUSH_LIMIT
	count_project++;
#endif	// PROJECTION_FLUSH_LIMIT

	/* Location of player */
	/*y0 = py;
	x0 = px;*/


	/* Hack -- Jump to target */
	if (flg & PROJECT_JUMP) {
		x1 = x;
		y1 = y;

		/* Clear the flag (well, needed?) */
		//flg &= ~(PROJECT_JUMP);
	}

	/* Hack -- Start at a player */
	else if (who < 0 && who > PROJECTOR_UNUSUAL)
	//else if (IS_PVP)
	{
		x1 = Players[0 - who]->px;
		y1 = Players[0 - who]->py;
	}

	/* Start at a monster */
	else if (who > 0) {
		x1 = m_list[who].fx;
		y1 = m_list[who].fy;
	}
#if 1
	/* Oops */
	else {
		x1 = x;
		y1 = y;
	}
#endif	/* 0 */

	/* Default "destination" */
	y2 = y; x2 = x;

	/* Hack -- verify stuff */
	if ((flg & PROJECT_THRU) && (x1 == x2) && (y1 == y2))
		flg &= ~PROJECT_THRU;

	/* Hack -- Assume there will be no blast (max radius 16) */
	//for (dist = 0; dist < 16; dist++) gm[dist] = 0;
	for (dist = 0; dist < PREPARE_RADIUS; dist++) gm[dist] = 0;

	/* Hack -- Handle stuff */
	/*handle_stuff();*/


#ifdef DOUBLE_LOS_SAFETY
	/* Use projectable..() check to pre-determine if the line of fire is ok
	   and we may skip redundant checking that appears further below. */
	bool ok_DLS;
	if (IS_PVP) { /* ..but we're not a monster? */
 #ifndef PY_PROJ_WALL
		ok_DLS = projectable(wpos, y1, x1, y2, x2, MAX_RANGE);
 #else
		ok_DLS = projectable_wall(wpos, y1, x1, y2, x2, MAX_RANGE);
 #endif
	} else { /* Catch indirect attack spells! Those are RF4_ROCKET and RF4_BR_DISI. */
		/* Monsters always could target players in walls (even if the projection explodes _before_ the wall)
		   so we only need to use projectable_wall() here. */
		ok_DLS = projectable_wall(wpos, y1, x1, y2, x2, MAX_RANGE);
	}
	/* hack: catch non 'dir == 5' projections, aka manually directed */
	if (x1 == x2 || y1 == y2 || ABS(x2 - x1) == ABS(y2 - y1)) ok_DLS = FALSE;
#endif


	/* Start at the source */
	x = x9 = x1;
	y = y9 = y1;
	dist = 0;

	/* Project until done */
	while (TRUE) {
		/* Gather beam grids */
		if (flg & PROJECT_BEAM) {
			if (project_time_effect & EFF_WALL) {
				effect = new_effect(who, typ, dam, project_time, project_interval, wpos, y, x, 0, project_time_effect);
				if (effect != -1) zcave[y][x].effect = effect;
			}
			gy[grids] = y;
			gx[grids] = x;
			grids++;
#if DEBUG_LEVEL > 1
			if(grids > 500) s_printf("grids %d\n", grids);
#endif	/* DEBUG_LEVEL */
		}

		/* Check the grid */
		c_ptr = &zcave[y][x];

		/* XXX XXX Hack -- Display "beam" grids */
		if (!(flg & PROJECT_HIDE) &&
		    dist && (flg & PROJECT_BEAM))
#ifdef PROJECTION_FLUSH_LIMIT
			if (count_project < PROJECTION_FLUSH_LIMIT)
#endif	// PROJECTION_FLUSH_LIMIT
		{
			/* Hack -- Visual effect -- "explode" the grids */
			for (j = 1; j <= NumPlayers; j++) {
				player_type *p_ptr = Players[j];
				int dispx, dispy;
				byte attr;

				if (p_ptr->conn == NOT_CONNECTED)
					continue;

				if(!inarea(&p_ptr->wpos, wpos))
					continue;

				if (p_ptr->blind)
					continue;

				if (!panel_contains(y, x))
					continue;

				if (!player_has_los_bold(j, y, x))
					continue;

				dispx = x - p_ptr->panel_col_prt;
				dispy = y - p_ptr->panel_row_prt;

				attr = spell_color(typ);

				p_ptr->scr_info[dispy][dispx].c = '*';
				p_ptr->scr_info[dispy][dispx].a = attr;

				Send_char(j, dispx, dispy, attr, '*');
			}
		}


		/* Analyze whether the projection can overcome terrain in its way - C. Blue */
		/* Check: Fire vs Trees */
		if (allow_terraforming(wpos, FEAT_TREE)) {
			switch (c_ptr->feat) {
			case FEAT_TREE: terrain_resistance = 50; break;
			case FEAT_IVY: terrain_resistance = 0; break;
			case FEAT_BUSH: terrain_resistance = 20; break;
			case FEAT_DEAD_TREE: terrain_resistance = 30; break;
			default: terrain_resistance = -1; break;
			}
			switch (typ) {
			case GF_FIRE: terrain_damage = 100; break;
			case GF_METEOR: terrain_damage = 150; break;
			case GF_PLASMA: terrain_damage = 120; break;
			case GF_HELL_FIRE: terrain_damage = 130; break;
			default: terrain_damage = -1; break;
			}
		}

		/* Accordingly, stop the projection or have it go on unhindered */
		if (terrain_damage >= 0 && terrain_resistance >= 0 &&
		    magik(terrain_damage - terrain_resistance)) {
			/* go on unhindered by terrain, destroy terrain even */
		} else {
			/* Never pass through walls */
			if (flg & PROJECT_GRAV) { /* Running along the floor?.. */
				if (dist && !cave_floor_bold(zcave, y, x)) break;
#ifndef PROJ_MON_ON_WALL
			} else if (IS_PVP) { /* ..or rather levitating through the air? */
				if (dist && !cave_contact(zcave, y, x)
 #ifdef DOUBLE_LOS_SAFETY
				    && !ok_DLS
 #endif
				    ) break;
#endif
			} else { /* monsters can target certain non-los grid types directly */
				if (dist) {
#ifdef PROJ_ON_WALL
					if (broke_on_terrain1) break;
#else
 #ifdef DOUBLE_LOS_SAFETY
					if ((!cave_proj(zcave, y, x) && !ok_DLS)
 #else
					if (!cave_proj(zcave, y, x)
 #endif
					    || broke_on_terrain1) break;
#endif
					else if (!cave_contact(zcave, y, x)
#ifdef DOUBLE_LOS_SAFETY
					    && !ok_DLS
#endif
					    ) {
#ifndef PROJ_MON_ON_WALL
						/* if there isn't a player on the grid, we can't target it */
						if (zcave[y][x].m_idx >= 0) break;
#else
						/* if there isn't a player/monster on the grid, we can't target it */
						if (zcave[y][x].m_idx == 0) break;
#endif
						/* can't travel any further for sure now */
						broke_on_terrain1 = TRUE;
					}
				}
			}
		}


		/* Check for arrival at "final target" (if desired) */
		if (!(flg & PROJECT_THRU) && (x == x2) && (y == y2)) break;

		/* If allowed, and we have moved at all, stop when we hit anybody */
		/* -AD- Only stop if it isn't a party member */
		if ((c_ptr->m_idx != 0) && dist && (flg & PROJECT_STOP)) {
			if (who > 0) {
				/* hit first player (ignore monster) */
				if (c_ptr->m_idx < 0) break;
			}
			else if (who < 0) {
				/* always hit monsters */
				if (c_ptr->m_idx > 0) break;

				/* Hostile players hit each other */
				if (check_hostile(0 - who, 0 - c_ptr->m_idx)) break;

				/* If player hits himself, he hits others too */
				if (flg & PROJECT_PLAY) break;

#if 0 /* covered by PROJECT_PLAY above now */
				/* Always affect players (regardless of hostility/party state): */
				if (typ == GF_OLD_POLY) break;
#endif

#if 0				/* neutral people hit each other ..NOT! - C. Blue FF$$$ */
				if (!Players[0 - who]->party) break;

				/* people not in the same party hit each other ..NOT! - C. Blue */
				if (!player_in_party(Players[0 - who]->party, 0 - c_ptr->m_idx))
#if FRIEND_FIRE_CHANCE
					if (!magik(FRIEND_FIRE_CHANCE))
#endif
						break;
#endif
			}
			//else break;	// Huh? always break? XXX XXX
		}


		/* Calculate the new location */
		y9 = y;
		x9 = x;
		mmove2(&y9, &x9, y1, x1, y2, x2);
#ifdef DOUBLE_LOS_SAFETY
		/* After we reached our target we have no more need for double-los-safety */
		if (y9 == y2 && x9 == x2) ok_DLS = FALSE;
#endif


		/* Keep track of the distance traveled */
		dist++;

		/* Distance stuff: The 'dist > MAX_RANGE' part is basically obsolete
		   now that distance() is used to achieve 'true' distance. */

		/* Distance to target too great?
		   Use distance() to form a 'natural' circle shaped radius instead of a square shaped radius,
		   monsters do this too */
		if ((true_dist = distance(y1, x1, y9, x9)) > MAX_RANGE) break;

		/* Nothing can travel furthur than the maximal distance */
		if (dist > MAX_RANGE) break;

		/* Hack -- Balls explode BEFORE reaching walls or doors */
		if (flg & PROJECT_GRAV) { /* Running along the floor?.. */
			if (!cave_floor_bold(zcave, y9, x9) && (rad > 0)) break;
#ifndef PROJ_MON_ON_WALL
		} else if (IS_PVP) { /* ..or rather levitating through the air? */
			if (!cave_contact(zcave, y9, x9)
 #ifdef DOUBLE_LOS_SAFETY
			    && !ok_DLS
 #endif
			     && (rad > 0)) break;
#endif
		} else { /* monsters can target certain non-los grid types directly */
			if (rad > 0) {
#ifdef PROJ_ON_WALL
				if (broke_on_terrain2) break;
#else
 #ifdef DOUBLE_LOS_SAFETY
				if ((!cave_proj(zcave, y9, x9) && !ok_DLS)
 #else
				if (!cave_proj(zcave, y9, x9)
 #endif
				    || broke_on_terrain2) break;
#endif
				else if (!cave_contact(zcave, y9, x9)
#ifdef DOUBLE_LOS_SAFETY
				    && !ok_DLS
#endif
				    ) {
#ifndef PROJ_MON_ON_WALL
					/* if there isn't a player on the grid, we can't target it */
					if (zcave[y9][x9].m_idx >= 0) break;
#else
					/* if there isn't a player/monster on the grid, we can't target it */
					if (zcave[y9][x9].m_idx == 0) break;
#endif
					/* can't travel any further for sure now */
					broke_on_terrain2 = TRUE;
				}
			}
		}


		/* Only do visual effects (and delay) if requested */
		if (!(flg & PROJECT_HIDE))
#ifndef OPTIMIZED_ANIMATIONS
			if (count_project < PROJECTION_FLUSH_LIMIT)
#endif	// PROJECTION_FLUSH_LIMIT
		{
#ifdef PROJECTION_FLUSH_LIMIT
			for (j = 1; j <= NumPlayers; j++) {
				player_type *p_ptr = Players[j];
				int dispy, dispx;
				char ch;
				byte attr;

				if (p_ptr->conn == NOT_CONNECTED)
					continue;

				if (!inarea(&p_ptr->wpos, wpos))
					continue;

				if (p_ptr->blind)
					continue;

				if (!panel_contains(y9, x9))
					continue;

				if (!player_has_los_bold(j, y9, x9))
					continue;

				dispx = x9 - p_ptr->panel_col_prt;
				dispy = y9 - p_ptr->panel_row_prt;

				ch = bolt_char(y, x, y9, x9);
				attr = spell_color(typ);

				p_ptr->scr_info[dispy][dispx].c = ch;
				p_ptr->scr_info[dispy][dispx].a = attr;

				Send_char(j, dispx, dispy, attr, ch);

				/* Hack -- Show bolt char */
				if (dist % 2) Send_flush(j);
			}
#else /* OPTIMIZED_ANIMATIONS */
			/* Save the path */
			if (path_num < MAX_RANGE) {
				path_y[path_num] = y9;
				path_x[path_num] = x9;
				path_num++;
			}
#endif /* OPTIMIZED_ANIMATIONS */
		}

		/* Clean up */
		everyone_lite_spot(wpos, y9, x9);

		/* Save the new location */
		y = y9;
		x = x9;
	}
	
	if (project_time_effect & EFF_WALL) {
		flg &= ~(PROJECT_STAY);
		project_time = 0;
		project_interval = 0;
		project_time_effect = 0;
	}
	
	/* Hack: Usually, elemental bolt spells will not hurt floor/item if they already hurt a monster/player.
	         Some bolt spells (poly) don't need this flag, since they don't hurt items/floor at all. */
	if ((flg & PROJECT_EVSG) && zcave[y][x].m_idx != 0)
		flg &= ~(PROJECT_GRID | PROJECT_ITEM);

	/* hack: FF1_BLOCK_CONTACT grids prevent explosions,
	   since those would carry over on the other side if it's
	   just a wall of thickness 1 and possibly hit monsters there. */
	if (f_info[zcave[y9][x9].feat].flags1 & FF1_BLOCK_CONTACT) suppress_explosion = TRUE;

#ifdef OPTIMIZED_ANIMATIONS
	if (path_num) {
		/* Pick a random spot along the path */
		i = rand_int(path_num);
		y9 = path_y[i];
		x9 = path_x[i];

		for (j = 1; j <= NumPlayers; j++) {
			player_type *p_ptr = Players[j];
			int dispy, dispx;
			char ch;
			byte attr;

			if (p_ptr->conn == NOT_CONNECTED)
				continue;

			if (!inarea(&p_ptr->wpos, wpos))
				continue;

			if (p_ptr->blind)
				continue;

			if (!panel_contains(y9, x9))
				continue;

			if (!player_has_los_bold(j, y9, x9))
				continue;

			dispx = x9 - p_ptr->panel_col_prt;
			dispy = y9 - p_ptr->panel_row_prt;

			ch = bolt_char(y, x, y9, x9);
			attr = spell_color(typ);

			p_ptr->scr_info[dispy][dispx].c = ch;
			p_ptr->scr_info[dispy][dispx].a = attr;

			Send_char(j, dispx, dispy, attr, ch);

			/* Flush once */
			Send_flush(j);
		}

		/* Redraw later */
		everyone_lite_later_spot(wpos, y9, x9);
	}
#endif /* OPTIMIZED_ANIMATIONS */

	/* Save the "blast epicenter" */
	y2 = y;
	x2 = x;

	/* Start the "explosion" */
	gm[0] = 0;

	/* Hack -- make sure beams get to "explode" */
	gm[1] = grids;

	/* Ported hack for reflection */
	dist_hack = dist;

	/* If we found a "target", explode there */
	if (true_dist <= MAX_RANGE && !suppress_explosion) {
		/* Mega-Hack -- remove the final "beam" grid */
		//if ((flg & PROJECT_BEAM) && (grids > 0)) grids--;

		dist = 0;

		for (i = 0; i <= tdi[rad]; i++) {
			/* Encode some more "radius" info */
			if (i == tdi[dist]) {
				gm[++dist] = grids;
#if DEBUG_LEVEL > 2
				s_printf("dist:%d  i:%d\n", dist, i);
#endif
				if (dist > rad) break;
			}

			y = y2 + tdy[i];
			x = x2 + tdx[i];

			/* Ignore "illegal" locations */
			if (!in_bounds3(wpos, l_ptr, y, x)) continue;

#ifdef NO_EXPLOSION_OUT_OF_MAX_RANGE
			/* Don't create explosions that exceed MAX_RANGE from the caster */
			if (distance(y, x, y1, x1) > MAX_RANGE) continue;
#endif

#if 1
			if ((typ == GF_DISINTEGRATE) ||
			    /* Reduce disintegration effect of rockets to radius 1 - C. Blue */
			    ((typ == GF_ROCKET || typ == GF_DETONATION) && (ABS(y - y2) <= 1) && (ABS(x - x2) <= 1)) )
			{
				c_ptr2 = &zcave[y][x];
				if (cave_valid_bold(zcave,y,x))
//						&& (cave[y][x].feat < FEAT_PATTERN_START
//						 || cave[y][x].feat > FEAT_PATTERN_XTRA2))
				{
					if (
						//(c_ptr->feat != FEAT_WATER) &&
						(c_ptr2->feat != FEAT_SHAL_WATER) &&
						(c_ptr2->feat != FEAT_DEEP_WATER) &&
						(c_ptr2->feat != FEAT_TAINTED_WATER) &&
						(c_ptr2->feat != FEAT_DEEP_LAVA) &&
						(c_ptr2->feat != FEAT_SHAL_LAVA) &&
						//(c_ptr2->feat != FEAT_ASH) &&
						(c_ptr2->feat != FEAT_MUD) &&
						(c_ptr2->feat != FEAT_DIRT) &&
						(c_ptr2->feat != FEAT_HOME_OPEN) &&
						(c_ptr2->feat != FEAT_HOME))
					{
						if (allow_terraforming(wpos, FEAT_TREE)) {
							if (randint(2) == 1)
								cave_set_feat_live(wpos, y, x, FEAT_FLOOR);
							else
								cave_set_feat_live(wpos, y, x, FEAT_ASH);
							/* Cleanup Runemaster Glyphs - Kurzel */
							struct c_special *cs_ptr;
							cs_ptr = GetCS(c_ptr2, CS_RUNE);
							if (cs_ptr) cs_erase(c_ptr2, cs_ptr);
						}
					}

				/* Update some things -- similar to GF_KILL_WALL */
				//p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
				}
			}
#endif	/* 1 */
			/* else */ /* HERE!!!!*/
			/* Ball explosions are stopped by walls */
			if (!los(wpos, y2, x2, y, x)) continue;

			/* Save this grid */
			gy[grids] = y;
			gx[grids] = x;
			grids++;
			if(grids > 500) printf("grids %d\n", grids);
		}

#if 0
		/* Determine the blast area, work from the inside out */
		for (dist = 0; dist <= rad; dist++) {
			/* Scan the maximal blast area of radius "dist" */
			for (y = y2 - dist; y <= y2 + dist; y++) {
				for (x = x2 - dist; x <= x2 + dist; x++) {
					/* Ignore "illegal" locations */
					if (!in_bounds2(wpos, y, x)) continue;

					/* Enforce a "circular" explosion */
					if (distance(y2, x2, y, x) != dist) continue;

 #if 0
					if (typ == GF_DISINTEGRATE) {
						if (cave_valid_bold(y,x) &&
						    (cave[y][x].feat < FEAT_PATTERN_START
						    || cave[y][x].feat > FEAT_PATTERN_XTRA2))
							cave_set_feat_live(y, x, FEAT_FLOOR);

						/* Update some things -- similar to GF_KILL_WALL */
						p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
					}
 #endif	/* 0 */
					/* else */ /* HERE!!!!*/
					/* Ball explosions are stopped by walls */
					if (!los(wpos, y2, x2, y, x)) continue;

					/* Save this grid */
					gy[grids] = y;
					gx[grids] = x;
					grids++;
					if(grids>500) printf("grids %d\n", grids);
				}
			}

			/* Encode some more "radius" info */
			gm[dist+1] = grids;
		}
#endif	// 0
	}

	/* Speed -- ignore "non-explosions" */
	if (!grids) return (FALSE);

#ifndef OPTIMIZED_ANIMATIONS
	/* Display the "blast area" */
	if (!(flg & PROJECT_HIDE))
 #ifdef PROJECTION_FLUSH_LIMIT
		if (count_project < PROJECTION_FLUSH_LIMIT)
 #endif	// PROJECTION_FLUSH_LIMIT
	{
		/* Then do the "blast", from inside out */
		for (t = 0; t <= rad; t++) {
			/* Reset who can see */
			num_can_see = 0;

			/* Dump everything with this radius */
			for (i = gm[t]; i < gm[t+1]; i++) {
				/* Extract the location */
				y = gy[i];
				x = gx[i];

				for (j = 1; j <= NumPlayers; j++) {
					player_type *p_ptr = Players[j];
					int dispy, dispx;
					byte attr;
					int k;
					bool flag = TRUE;

					if (p_ptr->conn == NOT_CONNECTED)
						continue;

					if (!inarea(&p_ptr->wpos, wpos))
						continue;

					if (p_ptr->blind)
						continue;

					if (!panel_contains(y, x))
						continue;

					if (!player_has_los_bold(j, y, x))
						continue;

					attr = spell_color(typ);

					dispx = x - p_ptr->panel_col_prt;
					dispy = y - p_ptr->panel_row_prt;

					p_ptr->scr_info[dispy][dispx].c = '*';
					p_ptr->scr_info[dispy][dispx].a = attr;

					Send_char(j, dispx, dispy, attr, '*');

					drawn = TRUE;

					for (k = 0; k < num_can_see; k++) {
						if (who_can_see[k] == j)
							flag = FALSE;
					}

					if (flag) who_can_see[num_can_see++] = j;
				}
			}

			/* Flush each "radius" seperately */
			for (j = 0; j < num_can_see; j++) {
				/* Show this radius and delay */
				Send_flush(who_can_see[j]);
			}
		}

		/* Flush the erasing */
		if (drawn) {
			/* Erase the explosion drawn above */
			for (i = 0; i < grids; i++) {
				/* Extract the location */
				y = gy[i];
				x = gx[i];

				/* Erase if needed */
				everyone_lite_spot(wpos, y, x);
			}

			/* Flush the explosion */
			for (j = 0; j < num_can_see; j++) {
				/* Show this radius and delay */
				Send_flush(who_can_see[j]);
			}
		}
	}
#endif


	/* Check features */
	if (flg & PROJECT_GRID) {
		/* Start with "dist" of zero */
		dist = 0;

		/* Effect ? */
		if (flg & PROJECT_STAY) {
			/* I believe it's not right */
			//effect = new_effect(typ, dam, project_time, py, px, rad, project_time_effect);
			/* MEGAHACK -- quick hack to make fire_wall work
			 * this should be rewritten!	- Jir -
			 *
			 * It registers the 'wall' as if it was a ball:
			 *
			 *	   |--dist_hack--|
			 * (y,x) *------+------* (y2,x2)
			 *			  +pseudo 'centre'
			 */
			if (rad == 0) {
#if 1 /* Kurzel's patch commented this out - but this is required for fireworks! */
				effect = new_effect(who, typ, dam, project_time, project_interval, wpos,
				    (y + y2) / 2, (x + x2) / 2, dist_hack / 2 + 1,
				    project_time_effect);
#endif
#ifdef ARCADE_SERVER
#if 0
						if (project_time_effect & EFF_CROSSHAIR_A || project_time_effect & EFF_CROSSHAIR_B ||
						    project_time_effect & EFF_CROSSHAIR_C) {
							msg_broadcast(0, "making an effect");
							player_type *pfft_ptr = Players[project_interval];
							pfft_ptr->e = effect;
						}
#endif
#endif
			} else {
				effect = new_effect(who, typ, dam, project_time, project_interval, wpos,
						y2, x2, rad, project_time_effect);
			}
			project_interval = 0;
			project_time = 0;
			project_time_effect = 0;
		}

		/* Now hurt the cave grids (and objects) from the inside out */
		for (i = 0; i < grids; i++) {
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			if(!in_bounds(y,x)) continue;
			/* Affect the feature */
			if ((flg & PROJECT_STAY) || (flg & PROJECT_FULL)) dist = 0;
			if (project_f(0 - who, who, dist, wpos, y, x, dam, typ, flg)) notice = TRUE;

			/* Effect ? */
			if (flg & PROJECT_STAY) {
				if (effect != -1) /* check that we're not out of effects - mikaelh */
				{
					zcave[y][x].effect = effect;
					everyone_lite_spot(wpos, y, x);
				}
			}
		}
	}

	/* PROJECT_DUMY means we don't have to project on floor/items/monsters/players,
	   because the effect was just for visual entertainment.. - C. Blue */
	if (flg & PROJECT_DUMY) return(FALSE);

	/* Check objects */
	if (flg & PROJECT_ITEM) {
		/* Start with "dist" of zero */
		dist = 0;

		/* Now hurt the cave grids (and objects) from the inside out */
		for (i = 0; i < grids; i++) {
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			if (!in_bounds(y,x)) continue;
			/* Affect the object */
			if ((flg & PROJECT_STAY) || (flg & PROJECT_FULL)) dist = 0;
			if (project_i(0 - who, who, dist, wpos, y, x, dam, typ)) notice = TRUE;
		}
	}


	/* Check monsters */
	/* eww, hope traps won't kill the server here..	- Jir - */
	if ((flg & PROJECT_KILL) && !players_only) {
		/* Start with "dist" of zero */
		dist = 0;

		/* Mega-Hack */
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;

		if (who < 0 && who > PROJECTOR_UNUSUAL &&
		//if (IS_PVP &&
		    Players[0 - who]->taciturn_messages)
			suppress_message = TRUE;

		/* Now hurt the monsters, from inside out */
		for (i = 0; i < grids; i++) {
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* paranoia */
			if (!in_bounds2(wpos, y, x)) continue;

#ifndef PROJ_MON_ON_WALL
			/* Walls protect monsters */
			//if (!cave_floor_bold(zcave, y, x)) continue;
			/* Monsters can be hit on dark pits */
			if (!cave_contact(zcave, y, x)) continue;
#else
			/* Walls only protect monsters if it's not the very epicenter of the blast. */
			if (!cave_contact(zcave, y, x) && y != y2 && x != x2) continue;
#endif

			/* Affect the monster */
			//if (project_m(0-who, who, y2, x2, dist, wpos, y, x, dam, typ)) notice = TRUE;

			if (zcave[y][x].m_idx <= 0) continue;

			//if (grids <= 1 && (zcave[y][x].m_idx > 0))
/*			if (grids <= 1) {
				monster_type *m_ptr = &m_list[zcave[y][x].m_idx];
				monster_race *ref_ptr = race_inf(m_ptr);
				monster_race *ref_ptr = race_inf(&m_list[zcave[y][x].m_idx]);
			}
*/
			if ((flg & PROJECT_STAY) || (flg & PROJECT_FULL)) dist = 0;
			if (project_m(0 - who, who, y2, x2, dist, wpos, y, x, dam, typ, flg)) notice = TRUE;
		}

		/* Mega-Hack */
		if ((who < 0) && (project_m_n == 1) && (who > PROJECTOR_UNUSUAL))
		//if (IS_PVP && project_m_n == 1)
		{
			/* Location */
			x = project_m_x;
			y = project_m_y;

			/* Still here */
			if (who < 0) {
				player_type *p_ptr = Players[0 - who];
				int m_idx = zcave[y][x].m_idx;

				/* Hack - auto-track monster */
				if (m_idx > 0) {
					if (p_ptr->mon_vis[m_idx]) health_track(0 - who, m_idx);
				} else {
					if (p_ptr->play_vis[0 - m_idx]) health_track(0 - who, m_idx);
				}
			}
		}

		suppress_message = old_tacit;
	}

	/* Check player */
	if (flg & PROJECT_KILL) {
		/* Start with "dist" of zero */
		dist = 0;

		/* Clear the got_hit flags - mikaelh */
		for (i = 1; i <= NumPlayers; i++) {
			player_type *p_ptr = Players[i];

			if (p_ptr->conn == NOT_CONNECTED)
				continue;

			p_ptr->got_hit = FALSE;
		}

		/* Now see if the player gets hurt */
		for (i = 0; i < grids; i++) {
			/* Who is at the location */
			int player_idx;

			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* Set the player index */
			/* paranoia */
			if (!in_bounds2(wpos, y, x)) continue;

			player_idx = 0 - zcave[y][x].m_idx;

			/* Affect the player */
			if ((flg & PROJECT_STAY) || (flg & PROJECT_FULL)) dist = 0;
			if (project_p(player_idx, who, dist, wpos, y, x, dam, typ, rad, flg, pattacker)) notice = TRUE;
			/* reset stair-goi helper flag (used by project_p()) again */
			if (player_idx >= 1 && player_idx <= NumPlayers) Players[player_idx]->invuln_applied = FALSE;
		}
	}

#ifdef OPTIMIZED_ANIMATIONS
	/* Display the "blast area" */
	if (!(flg & PROJECT_HIDE))
 #ifdef PROJECTION_FLUSH_LIMIT
			if (count_project < PROJECTION_FLUSH_LIMIT)
 #endif	// PROJECTION_FLUSH_LIMIT
	{
		/* Then do the "blast", from inside out */
		for (t = 0; t <= rad; t++) {
			/* Reset who can see */
			num_can_see = 0;

			/* Dump everything with this radius */
			for (i = gm[t]; i < gm[t+1]; i++) {
				/* Extract the location */
				y = gy[i];
				x = gx[i];

				for (j = 1; j <= NumPlayers; j++) {
					player_type *p_ptr = Players[j];
					int dispy, dispx;
					byte attr;
					int k;
					bool flag = TRUE;

					if (p_ptr->conn == NOT_CONNECTED)
						continue;

					if (!inarea(&p_ptr->wpos, wpos))
						continue;

					if (p_ptr->blind)
						continue;

					if (!panel_contains(y, x))
						continue;

					if (!player_has_los_bold(j, y, x))
						continue;

					attr = spell_color(typ);

					dispx = x - p_ptr->panel_col_prt;
					dispy = y - p_ptr->panel_row_prt;

					p_ptr->scr_info[dispy][dispx].c = '*';
					p_ptr->scr_info[dispy][dispx].a = attr;

					Send_char(j, dispx, dispy, attr, '*');

					drawn = TRUE;

					for (k = 0; k < num_can_see; k++) {
						if (who_can_see[k] == j)
							flag = FALSE;
					}

					if (flag)
						who_can_see[num_can_see++] = j;
				}
			}
		}

		/* Flush the whole thing */
		for (j = 0; j < num_can_see; j++) {
			/* Show this radius and delay */
			Send_flush(who_can_see[j]);
		}

		/* Flush the erasing */
		if (drawn) {
			/* Erase the explosion drawn above */
			for (i = 0; i < grids; i++) {
				/* Extract the location */
				y = gy[i];
				x = gx[i];

				/* Erase a bit later */
				everyone_lite_later_spot(wpos, y, x);
			}
		}
	}
#endif

	/* Return "something was noticed" */
	return (notice);
}

/* Check whether player is actually in an area that offers unusual safety from various
   attacks and effects. This is used for special events like Arena Monster Challenge - C. Blue */
int safe_area(int Ind) {
	player_type *p_ptr = Players[Ind];
	//dungeon_type *d_ptr = getdungeon(&p_ptr->wpos);

	/* For 'Arena Monster Challenge' event: */
	if (ge_special_sector && in_arena(&p_ptr->wpos)) return 1;

	/* default: usual situation - not safe */
	return 0;
}


/* Helper function for monster_is_safe() to determine how damaging a
 * projection is and if we should therefore try to move out of it. - C. Blue
 * IMPORTANT: Keep in sync with project_m(). */
int approx_damage(int m_idx, int dam, int typ) {
	int j = 0, k, k_elec, k_sound, k_lite;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);
	cptr name = r_name_get(m_ptr);

	/* Priest drain-life/Brief runespell annihilation spell hack */
	switch (typ) {
	case GF_OLD_DRAIN:
		dam &= ~4096;
		break;
	case GF_ANNIHILATION:
		dam &= ~8192;
		break;
	}

#if 0
	int do_poly = 0;
	int do_dist = 0;
	int do_blind = 0;
	int do_conf = 0;
	int do_stun = 0;
	int do_sleep = 0;
	//int do_fear = 0;
#endif

	bool resist = FALSE;

	/* Hack: GF_LIFEHEAL might heal or kill a monster */
	if (typ == GF_LIFEHEAL) {
		if (r_ptr->flags3 & RF3_UNDEAD) {
			typ = GF_HOLY_FIRE;
		} else {
			typ = GF_OLD_HEAL;
		}
	}

	switch (typ) {
		case GF_SILENCE:
			if (!((r_ptr->flags4 & RF4_SPELLCASTER_MASK) |
			    (r_ptr->flags5 & RF5_SPELLCASTER_MASK) |
			    (r_ptr->flags6 & RF6_SPELLCASTER_MASK) |
			    (r_ptr->flags0 & RF0_SPELLCASTER_MASK)) ||
			    (r_ptr->level >= 98 && (r_ptr->flags1 & RF1_UNIQUE)))
				dam = 0;
			else if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags2 & RF2_POWERFUL))
				dam /= 2;
#if 0 /* 0 => use 'dam' as 'avoidance' factor although there is no real damage here */
			dam = 0;
#endif
			break;

		case GF_PSI:
			if ((r_ptr->flags9 & RF9_IM_PSI) || (r_ptr->flags2 & RF2_EMPTY_MIND) ||
			    (r_ptr->flags3 & RF3_NONLIVING))
			{
				dam = 0;
				break;
			} else if (r_ptr->flags9 & RF9_RES_PSI) {
				resist = TRUE;
			} else if (r_ptr->flags3 & RF3_UNDEAD) {
				resist = TRUE;
			} else if ((r_ptr->flags2 & RF2_STUPID) ||
			    ((r_ptr->flags3 & RF3_ANIMAL) && !(r_ptr->flags2 & RF2_CAN_SPEAK))) {
				resist = TRUE;
			} else if (r_ptr->flags2 & RF2_WEIRD_MIND)
				dam = (dam * 3 + 1) / 4;

			if ((r_ptr->flags2 & RF2_SMART) && !resist) dam += dam / 2;

			if (resist) dam /= 2;
			break;

		case GF_CHARMIGNORE:
			dam = 0;
			break;

		case GF_EARTHQUAKE:
			dam = 0;
			break;

		case GF_MISSILE:
			break;

		case GF_ACID:
			if (r_ptr->flags3 & RF3_IM_ACID)
				dam = 0;
			else if (r_ptr->flags9 & RF9_RES_ACID)
				dam /= 4;
			else if (r_ptr->flags9 & RF9_SUSCEP_ACID)
				dam *= 2;
			break;

		case GF_ELEC:
			if (r_ptr->flags3 & RF3_IM_ELEC)
				dam = 0;
			else if (r_ptr->flags9 & RF9_RES_ELEC)
				dam /= 4;
			else if (r_ptr->flags9 & RF9_SUSCEP_ELEC)
				dam *= 2;
			break;

		case GF_FIRE:
			if (r_ptr->flags3 & RF3_IM_FIRE)
				dam = 0;
			else if (r_ptr->flags9 & RF9_RES_FIRE)
				dam /= 4;
			else if (r_ptr->flags3 & RF3_SUSCEP_FIRE)
				dam *= 2;
			break;

		case GF_COLD:
			if (r_ptr->flags3 & RF3_IM_COLD)
				dam = 0;
			else if (r_ptr->flags9 & RF9_RES_COLD)
				dam /= 4;
			else if (r_ptr->flags3 & RF3_SUSCEP_COLD)
				dam *= 2;
			break;

		case GF_POIS:
			if ((r_ptr->flags3 & RF3_IM_POIS) ||
			    (r_ptr->flags3 & (RF3_NONLIVING)) || (r_ptr->flags3 & (RF3_UNDEAD)) ||
			    (r_ptr->d_char == 'A') || (r_ptr->d_char == 'E') || ((r_ptr->d_char == 'U') && (r_ptr->flags3 & RF3_DEMON)))
				dam = 0;
			else if (r_ptr->flags9 & RF9_RES_POIS)
				dam /= 4;
			else if (r_ptr->flags9 & RF9_SUSCEP_POIS)
				dam *= 2;
			break;

		case GF_UNBREATH:
			//if (magik(15)) do_pois = (10 + randint(11) + r) / (r + 1);
			if ((r_ptr->flags3 & (RF3_NONLIVING)) || (r_ptr->flags3 & (RF3_UNDEAD)) ||
				(r_ptr->d_char == 'A') || (r_ptr->d_char == 'E') || ((r_ptr->d_char == 'U') && (r_ptr->flags3 & RF3_DEMON)) ||
				(m_ptr->r_idx == RI_MORGOTH)) /* <- Morgoth */
				dam = 0;
				//do_pois = 0;
			else if (r_ptr->flags3 & RF3_IM_POIS)
				dam = (dam * 2) / 4;
			else if (r_ptr->flags9 & RF9_RES_POIS)
				dam = (dam * 3) / 4;
			else if (r_ptr->flags9 & RF9_SUSCEP_POIS)
				dam = (dam * 3) / 2;
			break;

		case GF_HELL_FIRE:
			if (r_ptr->flags3 & (RF3_GOOD)) {
				if (r_ptr->flags3 & RF3_IM_FIRE) {
					dam *= 2; dam /= 2;//(randint(4)+3);
				} else if (r_ptr->flags9 & RF9_RES_FIRE)
					dam = (dam * 3) / 2;
				else if (r_ptr->flags3 & RF3_SUSCEP_FIRE)
					dam *= 2;
				else
					dam *= 2;
			} else {
				if (r_ptr->flags3 & RF3_IM_FIRE) {
					dam *= 2; dam /= 4;//(randint(6)+10);
				} else if (r_ptr->flags9 & RF9_RES_FIRE)
					dam = (dam * 3) / 4;
				else if (r_ptr->flags3 & RF3_SUSCEP_FIRE)
					dam /= 2;
				else {
					//dam *= 5; dam /= (randint(3)+4);
				}
			}
			if (r_ptr->flags3 & (RF3_EVIL)) dam = (dam * 2) / 3;
			break;

		case GF_HOLY_ORB:
			if (r_ptr->flags3 & (RF3_GOOD))
				dam = 0;
			if (r_ptr->flags3 & RF3_EVIL)
				dam *= 2;
			break;

		case GF_HOLY_FIRE:
			if (r_ptr->flags3 & (RF3_GOOD))
				dam = 0;
			else if (r_ptr->flags3 & (RF3_EVIL)) {
				if (r_ptr->flags3 & RF3_IM_FIRE) {
					dam *= 2; dam = (dam * 2) / 3;//(randint(4)+3);
				} else if (r_ptr->flags9 & RF9_RES_FIRE)
					dam = (dam * 6) / 4;
				else if (r_ptr->flags3 & RF3_SUSCEP_FIRE)
					dam *= 2;
				else
					dam *= 2;
			} else {
				if (r_ptr->flags3 & RF3_IM_FIRE) {
					dam *= 2; dam /= 3;//(randint(6)+10);
				} else if (r_ptr->flags9 & RF9_RES_FIRE)
					dam = (dam * 3) / 4;
				else if (r_ptr->flags3 & RF3_SUSCEP_FIRE)
					dam /= 2;
				else {
					//dam *= 5; dam /= (randint(3)+4);
				}
			}
			break;

		case GF_ARROW:
			break;

		case GF_PLASMA: {
			/* 50% fire */
			k = dam / 2;
			if (r_ptr->flags3 & RF3_IM_FIRE) k = 0;
			else if (r_ptr->flags9 & RF9_RES_FIRE) k /= 4;
			else if (r_ptr->flags3 & RF3_SUSCEP_FIRE) k *= 2;
			/* 25% elec */
			k_elec = dam / 4;
			if (r_ptr->flags3 & RF3_IM_ELEC) k_elec = 0;
			else if (r_ptr->flags9 & RF9_RES_ELEC) k_elec /= 4;
			else if (r_ptr->flags9 & RF9_SUSCEP_ELEC) k_elec *= 2;
			/* 25% force */
			k_sound = dam / 4;
			if ((r_ptr->flags4 & RF4_BR_SOUN) || (r_ptr->flags9 & RF9_RES_SOUND)) k_sound /= 2;
			//else do_stun = randint(15) / div;

			dam = k + k_elec + k_sound;
			break;
		}

		case GF_NETHER_WEAK:
		case GF_NETHER:
			if (r_ptr->flags3 & RF3_UNDEAD)
				dam = 0;
			else if ((r_ptr->flags4 & RF4_BR_NETH) || (r_ptr->flags3 & RF3_RES_NETH))
				dam /= 3;
			else if (r_ptr->flags3 & RF3_DEMON)
				dam /= 2;
#if 0
			else if (r_ptr->flags3 & RF3_EVIL)
				dam /= 2;
#endif
			break;

		case GF_WATER:
		case GF_VAPOUR:
			if (r_ptr->flags9 & RF9_IM_WATER)
				dam = 0;
			else if (r_ptr->flags7 & RF7_AQUATIC)
				dam /= 9;
			else if (r_ptr->flags3 & RF3_RES_WATE)
				dam /= 4;
			break;

		case GF_WAVE:
			if (r_ptr->flags9 & RF9_IM_WATER)
				dam = 0;
			else if (r_ptr->flags7 & RF7_AQUATIC)
				dam /= 9;
			else if (r_ptr->flags3 & RF3_RES_WATE)
				dam /= 4;
			else
				//do_stun = 7;
			break;

		case GF_CHAOS:
			//if (r_ptr->level / 2 < 15) ;//do_poly = TRUE;
			//do_conf = 10;
			if ((r_ptr->flags4 & RF4_BR_CHAO) || (r_ptr->flags9 & RF9_RES_CHAOS)) {
				dam /= 3;
				//do_conf = 0;
				//do_poly = FALSE;
			}
			break;

		case GF_SHARDS:
			if ((r_ptr->flags4 & RF4_BR_SHAR) || (r_ptr->flags9 & RF9_RES_SHARDS))
				dam /= 3;
			else if (r_ptr->flags8 & RF8_NO_CUT)
				dam /= 2;
			break;

		case GF_HAVOC:
		{
			int res1 = 0, res2 = 0, res3 = 0, res4 = 0, res5 = 0; //shard,sound,fire,mana,chaos

			if ((r_ptr->flags4 & RF4_BR_SHAR) || (r_ptr->flags9 & RF9_RES_SHARDS))
				res1 = 2;
			//RF8_NO_CUT/RF3_NO_STUN doesn't help here
			if ((r_ptr->flags4 & RF4_BR_SOUN) || (r_ptr->flags9 & RF9_RES_SOUND))
				res2 = 2;
			if (r_ptr->flags3 & RF3_IM_FIRE)
				res3 = 4;
			else if (r_ptr->flags9 & RF9_RES_FIRE)
				res3 = 2;
			//No SUSCEP_FIRE check
			if ((r_ptr->flags4 & RF4_BR_MANA) || (r_ptr->flags9 & RF9_RES_MANA))
				res4 = 3;
			if ((r_ptr->flags4 & RF4_BR_CHAO) || (r_ptr->flags9 & RF9_RES_CHAOS))
				res5 = 3;

			switch (res1 + res2 + res3 + res4 + res5) {
			case 0: case 1: case 2: case 3: break;
			case 4: case 5: case 6: case 7:
				dam = (dam * 3 + 3) / 4;
				//do_cut = 0;
				break;
			case 8: case 9: case 10:
				dam /= 2;
				break;
			default: //11,12,13,14
				dam /= 3;
				break;
			}
		}
			break;

		case GF_INFERNO:
		case GF_DETONATION:
		case GF_ROCKET:
		{
			int res1 = 0, res2 = 0, res3 = 0; //shard,sound,fire

			if ((r_ptr->flags4 & RF4_BR_SHAR) || (r_ptr->flags9 & RF9_RES_SHARDS))
				res1 = 1;
			//RF8_NO_CUT/RF3_NO_STUN don't help here
			if ((r_ptr->flags4 & RF4_BR_SOUN) || (r_ptr->flags9 & RF9_RES_SOUND))
				res2 = 1;
			if (r_ptr->flags3 & RF3_IM_FIRE)
				res3 = 3;
			else if (r_ptr->flags9 & RF9_RES_FIRE)
				res3 = 1;
			//No SUSCEP_FIRE check

			switch (res1 + res2 + res3) {
			case 0: break;
			case 1: case 2:
				dam = (dam * 3 + 3) / 4;
				//do_cut = 0;
				break;
			default:
				dam /= 2;
				break;
			}
		}
			break;

		case GF_STUN:
			//do_stun = 18;
			if (r_ptr->flags9 & RF9_RES_SOUND) {}//do_stun /= 4;
			break;

		case GF_SOUND:
			//do_stun = 18;
			if ((r_ptr->flags4 & RF4_BR_SOUN) || (r_ptr->flags9 & RF9_RES_SOUND)) {
				dam /= 3;
				//do_stun = 0;
			}
			break;

		case GF_CONFUSION:
			//do_conf = 18;
			if ((r_ptr->flags4 & RF4_BR_CONF) ||
				(r_ptr->flags4 & RF4_BR_CHAO) || (r_ptr->flags9 & RF9_RES_CHAOS))
				dam /= 3;
			else if (r_ptr->flags3 & RF3_NO_CONF)
				dam /= 2;
			break;

		case GF_DISENCHANT:
			if ((r_ptr->flags4 & RF4_BR_DISE) ||
			    prefix(name, "Disen") ||
			    (r_ptr->flags3 & RF3_RES_DISE))
				dam /= 3;
			break;

		case GF_NEXUS:
			if ((r_ptr->flags4 & RF4_BR_NEXU) ||
			    prefix(name, "Nexus") ||
			    (r_ptr->flags3 & RF3_RES_NEXU))
				dam /= 3;
			break;

		case GF_FORCE:
			//do_stun = 8;
			if (r_ptr->flags4 & RF4_BR_WALL) dam /= 3;
			break;

		case GF_INERTIA:
			if (r_ptr->flags4 & RF4_BR_INER) dam /= 3;
			break;

		case GF_TIME: //Note: Also steals energy!
			if ((r_ptr->flags4 & RF4_BR_TIME) || (r_ptr->flags9 & RF9_RES_TIME)
			    || (r_ptr->flags3 & RF3_DEMON) || (r_ptr->flags3 & RF3_NONLIVING)
			    || (r_ptr->flags3 & RF3_UNDEAD))
				dam /= 3;
			break;

		case GF_GRAVITY:
		{
			bool resist_tele = FALSE;
			if ((r_ptr->flags3 & RF3_RES_TELE) || (r_ptr->flags9 & RF9_IM_TELE)) {
				if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flags9 & RF9_IM_TELE))
					resist_tele = TRUE;
			}

			if (!resist_tele) {}//do_dist = 10;
			else {}//do_dist = 0;

			if (r_ptr->flags4 & RF4_BR_GRAV) {
				dam /= 3;
				//do_dist = 0;
			}
			break;
		}

		case GF_MANA:
			if (r_ptr->flags9 & RF9_RES_MANA)
				dam /= 3;
			else if (r_ptr->flags4 & RF4_BR_MANA)
				dam /= 2;
			break;

		case GF_METEOR:
			break;

		case GF_ICE:
			//do_stun = 8;
			k = dam;

			dam = (k * 3) / 5;/* 60% COLD damage */
			if (r_ptr->flags3 & RF3_IM_COLD)
				dam = 0;
			else if (r_ptr->flags9 & RF9_RES_COLD)
				dam /= 4;
			else if (r_ptr->flags3 & RF3_SUSCEP_COLD)
				dam *= 2;

			k = (k * 2) / 5;/* 40% SHARDS damage */
			if ((r_ptr->flags4 & RF4_BR_SHAR) || (r_ptr->flags9 & RF9_RES_SHARDS))
				k /= 3;
			else if (r_ptr->flags8 & RF8_NO_CUT)
				k /= 2;
			dam = dam + k;
			break;

		case GF_THUNDER:
			k_elec = dam / 3; /* 33% ELEC damage */
			if (r_ptr->flags3 & RF3_IM_ELEC)
				k_elec = 0;
			else if (r_ptr->flags9 & RF9_RES_ELEC)
				k_elec /= 4;
			else if (r_ptr->flags9 & RF9_SUSCEP_ELEC)
				k_elec *= 2;

			k_sound = dam / 3; /* 33% SOUND damage */
			//do_stun = 8;
			if ((r_ptr->flags4 & RF4_BR_SOUN) || (r_ptr->flags9 & RF9_RES_SOUND)) {
				k_sound /= 3;
				//do_stun = 0;
			}

			k_lite = dam / 3; /* 33% LIGHT damage */
			//do_blind = damroll(3, (k_lite / 20)) + 1;
			if (r_ptr->d_char == 'A') {
				k_lite = 0;
				//do_blind = 0;
			} else if (r_ptr->flags3 & RF3_HURT_LITE) {
				k_lite *= 2;
				if (r_ptr->flags2 & RF2_REFLECTING) k_lite /= 2;
			} else if ((r_ptr->flags4 & RF4_BR_LITE) || (r_ptr->flags9 & RF9_RES_LITE) || (r_ptr->flags2 & RF2_REFLECTING)) {
				k_lite /= 4;
				//do_blind = 0;
			}

			dam = k_elec + k_sound + k_lite;
			break;

		case GF_OLD_DRAIN:
			if (m_ptr->hp > 9362)
				dam = (m_ptr->hp / 100) * dam;
			else if (m_ptr->hp > 936)
				dam = ((m_ptr->hp / 10) * dam) / 10;
			else
				dam = (m_ptr->hp * dam) / 100;

			if (dam > 900) dam = 900;

			if (r_ptr->flags1 & RF1_UNIQUE) dam /= 3;
			if (!dam) dam = 1;

			if ((r_ptr->flags3 & RF3_UNDEAD) ||
			    //(r_ptr->flags3 & RF3_DEMON) ||
			    (r_ptr->flags3 & RF3_NONLIVING) ||
			    (strchr("Egv", r_ptr->d_char)))
				dam = 0;
			break;

		case GF_ANNIHILATION:
			j = dam - 1;
			if (m_ptr->hp > 9362)
				dam = (m_ptr->hp / 100) * dam;
			else if (m_ptr->hp > 936)
				dam = ((m_ptr->hp / 10) * dam) / 10;
			else
				dam = (m_ptr->hp * dam) / 100;

			if (dam > 1200) dam = 1200;
			//ignoring the 600 cap for 'Brief' runespells

			if (r_ptr->flags1 & RF1_UNIQUE) dam /= 3;
			if (!dam) dam = 1;
			break;

		case GF_OLD_POLY:
			//do_poly = TRUE;
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->level > ((dam - 10) < 1 ? 1 : (dam - 10)) / 2 + 10))
				{}//do_poly = FALSE;

			dam = 0;
			break;

		case GF_OLD_CLONE:
		case GF_OLD_HEAL:
		case GF_HERO_MONSTER:
		case GF_REMFEAR:
		case GF_OLD_SPEED:
			dam = 0;
			break;

		case GF_OLD_SLOW:
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->level > ((dam - 10) < 1 ? 1 : (dam - 10)) + 10) ||
			    (r_ptr->level > ((dam - 10) < 1 ? 1 : (dam - 10)) / 2 + 10) || /* RES_OLD() */
			    !(m_ptr->mspeed >= 100 && m_ptr->mspeed > m_ptr->speed - 10))
				dam = 0;
			break;

		case GF_OLD_SLEEP:
			if (!((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags3 & RF3_NO_SLEEP) ||
			    (r_ptr->level > ((dam - 10) < 1 ? 1 : (dam - 10)) / 2 + 10))) /* RES_OLD() */
				{}//do_sleep = GF_OLD_SLEEP_DUR;
			dam = 0;
			break;

		case GF_OLD_CONF:
			//do_conf = damroll(3, (dam / 2)) + 1;
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags3 & RF3_NO_CONF) ||
			    (r_ptr->level > ((dam - 10) < 1 ? 1 : (dam - 10)) / 2 + 10)) /* RES_OLD() */
				{}//do_conf = 0;
			dam = 0;
			break;

		case GF_TERROR:
			//do_conf = damroll(3, (dam / 2)) + 1;
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    ((r_ptr->flags3 & RF3_NO_CONF) && (r_ptr->flags3 & RF3_NO_FEAR)) ||
			    (r_ptr->level > ((dam - 10) < 1 ? 1 : (dam - 10)) / 2 + 10)) /* RES_OLD() */
				{}//do_conf = do_fear = 0;
			dam = 0;
			break;

		case GF_BLIND:
			//do_blind = dam;
			dam = 0;
			break;

		case GF_CURSE:
			//do_conf = (damroll(3, (dam / 2)) + 1) / 3;
			//do_blind = dam / 3;
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->level > ((dam / 3 - 10) < 1 ? 1 : (dam / 3 - 10)) / 2 + 10)) /* RES_OLD */
				{}//do_blind = do_conf = 0;
			if (r_ptr->flags3 & RF3_NO_CONF) {}//do_conf = 0;

			dam /= 3; /* only applies damage in 1 of the 3 spell effects */

			if ((r_ptr->flags4 & RF4_BR_CONF) ||
				(r_ptr->flags4 & RF4_BR_CHAO) || (r_ptr->flags9 & RF9_RES_CHAOS))
				dam /= 3;
			else if (r_ptr->flags3 & RF3_NO_CONF)
				dam /= 2;
			break;

		case GF_HEALINGCLOUD:
			if (!(r_ptr->flags3 & RF3_UNDEAD))
				dam = 0;
			break;

		case GF_WATERPOISON:
			k = dam / 2;
			if ((r_ptr->flags3 & RF3_IM_POIS) ||
			  (r_ptr->flags3 & (RF3_NONLIVING)) || (r_ptr->flags3 & (RF3_UNDEAD)) ||
			  (r_ptr->d_char == 'A') || (r_ptr->d_char == 'E') || ((r_ptr->d_char == 'U') && (r_ptr->flags3 & RF3_DEMON)))
				k = 0;
			else if (r_ptr->flags9 & RF9_RES_POIS)
				k /= 4;
			else if (r_ptr->flags9 & RF9_SUSCEP_POIS)
				k *= 2;
			j = k;

			k = dam / 2;
			if (r_ptr->flags9 & RF9_IM_WATER)
				k = 0;
			else if (r_ptr->flags7 & RF7_AQUATIC)
				k /= 9;
			else if (r_ptr->flags3 & RF3_RES_WATE)
				k /= 4;
			dam = j + k;
			break;

		case GF_ICEPOISON:
			k = (dam * 3) / 5;
			if ((r_ptr->flags4 & RF4_BR_SHAR) || (r_ptr->flags9 & RF9_RES_SHARDS))
				k /= 3;
			else if (r_ptr->flags8 & RF8_NO_CUT)
				k /= 2;
			j = k;

			k = dam / 5;
			if (r_ptr->flags9 & RF9_IM_WATER)
				k = 0;
			else if (r_ptr->flags7 & RF7_AQUATIC)
				k /= 9;
			else if (r_ptr->flags3 & RF3_RES_WATE)
				k /= 4;
			j += k;

			k = dam / 5;
			if ((r_ptr->flags3 & RF3_IM_POIS) ||
			    (r_ptr->flags3 & (RF3_NONLIVING)) || (r_ptr->flags3 & (RF3_UNDEAD)) ||
			    (r_ptr->d_char == 'A') || (r_ptr->d_char == 'E') || ((r_ptr->d_char == 'U') && (r_ptr->flags3 & RF3_DEMON)))
				k = 0;
			else if (r_ptr->flags9 & RF9_RES_POIS)
				k /= 4;
			else if (r_ptr->flags9 & RF9_SUSCEP_POIS)
				k *= 2;
			dam = j + k;
			break;

		case GF_LITE_WEAK:
			if (!(r_ptr->flags3 & RF3_HURT_LITE))
				dam = 0;
			else if (r_ptr->flags2 & RF2_REFLECTING) dam /= 2;
			break;

		case GF_STARLITE:
		case GF_LITE:
			//do_blind = damroll(3, (dam / 20)) + 1;

			if (r_ptr->d_char == 'A') {
				//do_blind = 0;
				if (r_ptr->flags3 & RF3_EVIL) break;
				dam = 0;
			} else if ((r_ptr->flags4 & RF4_BR_LITE) || (r_ptr->flags9 & RF9_RES_LITE) || (r_ptr->flags2 & RF2_REFLECTING)) {
				dam /= 3;
				//do_blind = 0;
			} else if (r_ptr->flags3 & RF3_HURT_LITE) {
				dam *= 2;
				if (r_ptr->flags2 & RF2_REFLECTING) dam /= 2;
			}
			break;

		case GF_DARK:
			/* Exception: Bolt-type spells have no special effect */
			//if (flg & (PROJECT_NORF | PROJECT_JUMP))
				//do_blind = damroll(3, (dam / 20)) + 1;
			if ((r_ptr->flags4 & RF4_BR_DARK) || (r_ptr->flags9 & RF9_RES_DARK)
			    || (r_ptr->flags3 & RF3_UNDEAD)) {
				dam /= 3;
				//do_blind = 0;
			}
			break;

		case GF_KILL_WALL:
			if (!(r_ptr->flags3 & RF3_HURT_ROCK))
				dam = 0;
			break;

		case GF_AWAY_UNDEAD:
		case GF_AWAY_EVIL:
		case GF_AWAY_ALL:
		case GF_TURN_UNDEAD:
		case GF_TURN_EVIL:
		case GF_TURN_ALL:
			dam = 0;
			break;

		case GF_DISP_UNDEAD:
			if (!(r_ptr->flags3 & RF3_UNDEAD))
				dam = 0;
			break;
		case GF_DISP_EVIL:
			if (!(r_ptr->flags3 & RF3_EVIL))
				dam = 0;
			break;
		case GF_DISP_DEMON:
			if (!(r_ptr->flags3 & RF3_DEMON))
				dam = 0;
			break;
		case GF_DISP_ALL:
			break;
		case GF_CAUSE:
			if (r_ptr->d_char == 'A' || (r_ptr->flags8 & RF8_NO_CUT))
				dam = 0;
			else {
				k = 200; //assume level 50 player
				if (r_ptr->flags3 & RF3_EVIL) k *= 2;
				if (r_ptr->flags3 & RF3_GOOD) k /= 2;
				dam = (dam * (100 - (r_ptr->level * 100) / k)) / 100;
			}
			break;

		case GF_NUKE:
			if ((r_ptr->flags3 & RF3_IM_POIS) ||
			  (r_ptr->flags3 & (RF3_NONLIVING)) || (r_ptr->flags3 & (RF3_UNDEAD)) ||
			  (r_ptr->d_char == 'A') || ((r_ptr->d_char == 'U') && (r_ptr->flags3 & RF3_DEMON)))
				dam = 0;
			else if (r_ptr->flags9 & (RF9_RES_POIS))
				dam /= 3;
			break;

		case GF_DISINTEGRATE:
			if (r_ptr->flags3 & (RF3_HURT_ROCK))
				dam *= 2;
			if (r_ptr->flags1 & RF1_UNIQUE)
				dam >>= 1;
			break;

		case GF_HOLD:
		case GF_DOMINATE:
		case GF_TELE_TO:
		case GF_HAND_DOOM:
		case GF_STOP:
		case GF_STASIS:
		case GF_DEC_STR:
		case GF_DEC_DEX:
		case GF_DEC_CON:
		case GF_RES_STR:
		case GF_RES_DEX:
		case GF_RES_CON:
		case GF_INC_STR:
		case GF_INC_DEX:
		case GF_INC_CON:
		case GF_AUGMENTATION:
		case GF_RUINATION:
		default:
			/* No damage */
			dam = 0;
			break;
	}

#if 0
	/* maybe TODO: */
	int do_poly = 0;
	int do_dist = 0;
	int do_blind = 0;
	int do_conf = 0;
	int do_stun = 0;
	int do_sleep = 0;
//	int do_fear = 0;
#endif

	return (dam);
}
