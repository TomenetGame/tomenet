/* $Id$ */
/* File: cmd2.c */

/* Purpose: Movement commands (part 2) */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"


/* Amount of experience a player gets for disarming a trap. - C. Blue
   (It's personal, not distributed over the party)
   diff,lvl go from 2,2 to 12,38 atm (some traps disabled).
   dlev was added to the formula, because some traps (arrow/bolt traps)
   depend on dungeon depth for determining the effect, too.
   Also reduce XP gain if player level is high compared to floor depth, same as for XP gain from monsters.*/
#define TRAP_EXP_RAW(t_idx, dlev) \
    ((((((t_info[t_idx].difficulty * t_info[t_idx].minlevel) + ((dlev) > 5 ? (dlev) - 5 : 0)) * \
    (200 - t_info[t_idx].probability) / 100) /* so far here: 2...398 XP from rough testing of a bunch of traps */ \
    + 2) * 3) / 2) /* ...increase XP gain somewhat further, especially for the low-end traps */
#define TRAP_EXP(p_ptr, t_idx, dlev) \
    ((!in_irondeepdive(&p_ptr->wpos) && (!in_hallsofmandos(&p_ptr->wpos) || p_ptr->lev >= 50)) ? \
    det_exp_level(TRAP_EXP_RAW(t_idx, dlev), p_ptr->lev, getlevel(&p_ptr->wpos)) : \
    TRAP_EXP_RAW(t_idx, dlev))

/* Maximum level a character may have reached so far to still be eligible
   to enter the Ironman Deep Dive Challenge. Two main possiblities:
   a) [12] for far-away from town
   b)  [1] for adjacent to town
   c)  [0] same as [1], it's a hack that means 'level 1 AND 0 max_exp'. <- RECOMMENDED */
#define IRONDEEPDIVE_MAXLEV 0

/* chance of walking in a random direction when confused and trying to climb,
 * in percent. [50]
 */
#define STAIR_FAIL_IF_CONFUSED 50

/* duration of GoI when climbing stairs.	[2], must be 0<=n<=4. */
#define STAIR_GOI_LENGTH 3

/* Boomerangs cannot be destroyed by using them (they just fall to the floor). */
#define INDESTRUCTIBLE_BOOMERANGS

/* 4.7.3a: Boomerangs may get VORPAL flag, need all the melee formulae here too now
   ---- TODO: Maybe implement some of this stuff, but maybe we just don't need it! ... */

/* Inverse chance to get a vorpal cut (1 in n) [4] */
#define R_VORPAL_CHANCE 4

/* Better hits of one override worse hits of the other,
   instead of completely stacking for silly amounts. Recommended: V [off].
   Note: Crit already makes Vorpal not so useful, so probably just keep R_CRIT_VS_VORPAL off anyway. */
//#define R_CRIT_VS_VORPAL

/* Crit multiplier should affect unbranded dice+todam instead of branded dice+todam? [off]
   Advantage: Reduce huge gap between not so top 2h dice and top 2h dice weapons.
   Big disadvantage: A +10 crit weapon wouldn't get more than ~4% damage increase even from a KILL mod.
   NOTE: Currently only applies to melee. */
//#define CRIT_UNBRANDED

/* VORPAL being affected by brands? (+15 to-d & 2xbranded:
   +5% for crit weapons, +9% for non-crit weapons,
   crit +21% MoD over ZH, non-crit +13% MoD over ZH;)
   Recommended state is inverse of R_CRIT_VS_VORPAL (reduces vorpal efficiency in brand/kill flag scenario)
    or off (keeps vorpal efficiency in brand/kill scenario)
    or use R_VORPAL_LOWBRANDED to compromise (recommended). */
#ifndef R_CRIT_VS_VORPAL
 //#define R_VORPAL_UNBRANDED
 //#define R_VORPAL_LOWBRANDED  -- actually too weak for boomerangs, let's try full branding!
#endif

/* Allow bashing other players without being hostile, just for place-switching effect */
#define FRIENDLY_BASH



#ifdef USE_SOUND_2010
static void staircase_sfx(int Ind) {
	int b = Players[Ind]->body_monster;

	if (b) {
		/* Hardcoded forms */
		switch (b) {
		/* Riders */
		case 955: case 956: case 957: case 958: case 959:
			sound(Ind, "staircase_rider", "staircase", SFX_TYPE_COMMAND, FALSE);
			return;
		/* 'H' forms, oh dear.. at least cover the druid forms: */
		case 279:
			sound(Ind, "staircase_pad", "staircase", SFX_TYPE_COMMAND, FALSE);
			return;
		case 641: case 723:
			sound(Ind, "staircase_rider", "staircase", SFX_TYPE_COMMAND, FALSE);
			return;
		case 778:
			sound(Ind, "staircase_pad", "staircase", SFX_TYPE_COMMAND, FALSE);
			return;
		}

		switch (r_info[b].d_char) {
		case 'b': case 'B': case 'F':
			sound(Ind, "staircase_fly", "staircase", SFX_TYPE_COMMAND, FALSE);
			return;
		case 'C': case 'f': case 'Z':
		case 'R': case 'd': case 'D': case 'q': case 'r': case 'M':
			sound(Ind, "staircase_pad", "staircase", SFX_TYPE_COMMAND, FALSE);
			return;
		//case 'X': ~?~
		case '~': case 'J': case 'i': case 'n': case 'v': case 'w': case 'E': case 'G':
			sound(Ind, "staircase_slither", "staircase", SFX_TYPE_COMMAND, FALSE);
			return;
		case 'I': case 'S': case 'c': case 'a': case 'l': case 'K':
			sound(Ind, "staircase_scuttle", "staircase", SFX_TYPE_COMMAND, FALSE);
			return;
		default:
			sound(Ind, "staircase", NULL, SFX_TYPE_COMMAND, FALSE);
			return;
		}
	} else if (Players[Ind]->fruit_bat) sound(Ind, "staircase_fly", "staircase", SFX_TYPE_COMMAND, FALSE);
	else sound(Ind, "staircase", NULL, SFX_TYPE_COMMAND, FALSE);
}
#endif

/*
 * Go up one level                                      -RAK-
 */
void do_cmd_go_up(int Ind) {
	player_type *p_ptr = Players[Ind];
	monster_race *r_ptr = &r_info[p_ptr->body_monster];
	cave_type *c_ptr;
	struct worldpos *wpos = &p_ptr->wpos, old_wpos = p_ptr->wpos;
	bool tower = FALSE, dungeon = FALSE, surface = FALSE;
	cave_type **zcave;
#ifndef RPG_SERVER
	bool one_way = FALSE;
#endif
	int i;
#ifdef PROBTRAVEL_AVOIDS_OTHERS
	int z = 0; /* Just to suppress compiler warning */
#endif
#ifdef NOMAGIC_INHIBITS_LEVEL_PROBTRAVEL
	struct dun_level *l_ptr;
#endif
#ifdef DED_IDDC_AWARE
	bool obtained = FALSE;
#endif
	bool iddc = (p_ptr->wpos.wx == WPOS_IRONDEEPDIVE_X && p_ptr->wpos.wy == WPOS_IRONDEEPDIVE_Y && 1 == WPOS_IRONDEEPDIVE_Z);
	bool mandos = (p_ptr->wpos.wx == hallsofmandos_wpos_x && p_ptr->wpos.wy == hallsofmandos_wpos_y && 1 == hallsofmandos_wpos_z);

	if (!(zcave = getcave(wpos))) return;
#ifdef NOMAGIC_INHIBITS_LEVEL_PROBTRAVEL
	l_ptr = getfloor(wpos);
#endif

	if (wpos->wz > 0) tower = TRUE;
	if (wpos->wz < 0) dungeon = TRUE;
	if (wpos->wz == 0) surface = TRUE;

	if ((p_ptr->mode & MODE_DED_IDDC) && surface) {
		if (!iddc
#ifdef DED_IDDC_MANDOS
		    && !mandos
#endif
		    ) {
#ifdef DED_IDDC_MANDOS
			msg_print(Ind, "\377yYou may only enter the Ironman Deep Dive Challenge or the Halls of Mandos!");
#else
			msg_print(Ind, "\377yYou may not enter any other dungeon besides the Ironman Deep Dive Challenge!");
#endif
			return;
		}
#ifdef DED_IDDC_AWARE
		if (iddc) {
			for (i = 0; i < MAX_K_IDX; i++)
				if (magik(DED_IDDC_AWARE)) p_ptr->obj_aware[i] = TRUE;
			obtained = TRUE;
		}
#endif
	}

#ifndef RPG_SERVER
	/* Is this a one-way tower? */
	if (tower) {
		if ((wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_IRON) ||
		    (wild_info[wpos->wy][wpos->wx].tower->flags1 & (DF1_FORCE_DOWN | DF1_NO_UP)))
			one_way = TRUE;
	}
#endif

	/* Make sure he hasn't just changed depth */
	if (p_ptr->new_level_flag)
		return;

	/* Can we move ? */
	if (r_ptr->flags2 & RF2_NEVER_MOVE) {
		msg_print(Ind, "You cannot move by nature.");
		return;
	}

	if (cfg.runlevel < 5 && surface) {
		msg_print(Ind, "The tower is closed");
		return;
	}

#if STAIR_FAIL_IF_CONFUSED
	/* Hack -- handle confusion */
	if (p_ptr->confused && magik(STAIR_FAIL_IF_CONFUSED)) {
		int dir = 5, tries = 10;

		/* Prevent walking nowhere */
		while (dir == 5 && --tries)
			dir = rand_int(9) + 1;

		do_cmd_walk(Ind, dir, FALSE);
		return;
	}
#endif // STAIR_FAIL_IF_CONFUSED

	/* not for some global events (Highlander Tournament) */
	if ((p_ptr->global_event_temp & PEVF_SEPDUN_00) && (p_ptr->wpos.wz == -1)) {
		msg_print(Ind, "The staircase is blocked by huge chunks of rocks!");
		if (!is_admin(p_ptr)) return;
	}

	/* Player grid */
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* Quest stairs to preserve xy position! - Kurzel */
	if (c_ptr->feat == FEAT_QUEST_UP) {
		p_ptr->energy -= level_speed(&p_ptr->wpos);
		if (interfere(Ind, 20)) return;
		un_afk_idle(Ind);
		if (c_ptr->custom_lua_way) exec_lua(0, format("custom_way(%d,%d)", Ind, c_ptr->custom_lua_way));
		everyone_lite_spot_move(Ind, wpos, p_ptr->py, p_ptr->px);
		forget_lite(Ind);
		forget_view(Ind);
		p_ptr->new_level_method = LEVEL_PROB_TRAVEL;
		wpcopy(&old_wpos, wpos);
		wpos->wz += 1;
		msg_print(Ind, "You go up stairs.");
		new_players_on_depth(&old_wpos, -1, TRUE);
		p_ptr->new_level_flag = TRUE;
		new_players_on_depth(wpos, 1, TRUE);
		set_invuln_short(Ind, STAIR_GOI_LENGTH);
		c_ptr->m_idx = 0;
		return;
	}

	if (c_ptr->feat == FEAT_CYCLIC_LESS) {
		/* Hack -- take a turn */
		p_ptr->energy -= level_speed(wpos);

		/* Hack: Leave the dungeon, as we're taking a 'physical staircase out'! */
		un_afk_idle(Ind);

		everyone_lite_spot_move(Ind, wpos, p_ptr->py, p_ptr->px);
		forget_lite(Ind);
		forget_view(Ind);
		p_ptr->energy -= level_speed(wpos);
		p_ptr->new_level_method = LEVEL_GHOST;
		if (c_ptr->custom_lua_way) exec_lua(0, format("custom_way(%d,%d)", Ind, c_ptr->custom_lua_way));
		wpcopy(&old_wpos, wpos);
		wpos->wz = 0;
		new_players_on_depth(&old_wpos, -1, TRUE);
		p_ptr->new_level_flag = TRUE;
		new_players_on_depth(wpos, 1, TRUE);

		msg_format(Ind, "\377%cYou leave %s..", COLOUR_DUNGEON, get_dun_name(old_wpos.wx, old_wpos.wy, TRUE, wild_info[old_wpos.wy][old_wpos.wx].tower, 0, FALSE));
#ifdef RPG_SERVER /* stair scumming in non-IRON dungeons might create mad spam otherwise */
		if (p_ptr->party)
		for (i = 1; i <= NumPlayers; i++) {
		        if (Players[i]->conn == NOT_CONNECTED) continue;
			if (i == Ind) continue;
			if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
			if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
			if (Players[i]->party == p_ptr->party)
				msg_format(i, "\374\377G[\377%c%s has left %s..\377G]", COLOUR_DUNGEON, p_ptr->name, get_dun_name(old_wpos.wx, old_wpos.wy, TRUE, wild_info[old_wpos.wy][old_wpos.wx].tower, 0, FALSE));
		}
#endif
		set_invuln_short(Ind, STAIR_GOI_LENGTH);
		return;
	}

	/* Verify stairs if not a ghost, or admin wizard */
	if (!is_admin(p_ptr) &&
	    c_ptr->feat != FEAT_LESS && c_ptr->feat != FEAT_WAY_LESS &&
	    (!p_ptr->prob_travel || (c_ptr->info & CAVE_STCK)))
	{
		struct worldpos twpos;

		wpcopy(&twpos, wpos);
		if (twpos.wz < 0) twpos.wz++;
		if (!p_ptr->ghost) {
			msg_print(Ind, "I see no up staircase here.");
			return;
		}
#if 1	// no need for this anymore - C. Blue
	// ..argh there is! people exploit the hell out of every possibility they see. - C. Blue
#ifdef SEPARATE_RECALL_DEPTHS
		else if ((get_recall_depth(&twpos, p_ptr) + 5 <= ABS(twpos.wz)) && (twpos.wz != 0)) /* player may return to town though! */
#else
		else if ((p_ptr->max_dlv + 5 <= getlevel(&twpos)) && (twpos.wz != 0)) /* player may return to town though! */
#endif
		{
			/* anti Ghost-dive */
			msg_print(Ind, "A mysterious force prevents you from going up.");
			return;
		}
#endif
	}
	if (surface) {
		if (!(wild_info[wpos->wy][wpos->wx].flags & WILD_F_UP)) {
			if (c_ptr->feat == FEAT_LESS || c_ptr->feat == FEAT_WAY_LESS)
				msg_print(Ind, "\377sOnly ivy-clad ruins of a former tower remain at this place..");
			else msg_print(Ind, "There is nothing above you.");
			return;
		} else if ((c_ptr->feat != FEAT_LESS && c_ptr->feat != FEAT_WAY_LESS) &&
			    ((!p_ptr->prob_travel || (wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_NO_ENTRY_PROB)) &&
			    (!p_ptr->ghost || (wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_NO_ENTRY_FLOAT)))) {
			msg_print(Ind, "There is nothing above you.");
			if (!is_admin(p_ptr)) return;
		}
	}
	if (tower && wild_info[wpos->wy][wpos->wx].tower->maxdepth == wpos->wz) {
		msg_print(Ind, "You are at the top of the tower!");
		return;
	}

#ifdef RPG_SERVER /* Exclude NO_DEATH dungeons from the gameplay */
	if ((surface) && (wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_NO_DEATH)) {
		/* needed for 'Arena Monster Challenge' again, NO_DEATH are now simply empty (no items/monsters) */
		if (!(in_bree(wpos)) && !is_admin(p_ptr)) {
			msg_print(Ind, "\377sOnly ivy-clad ruins of a former tower remain at this place..");
			return;
		}
	}
#endif

#if 0 /* 0'ed because in such a dungeon there shouldn't be any staircases generated in the first place. If there are, then it's probably for some special thingy and intended. Or it's a bug. */
	/*if (dungeon && !p_ptr->ghost && (wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_IRON)) {*/
	if (dungeon && ((wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_IRON) ||
	     (wild_info[wpos->wy][wpos->wx].dungeon->flags1 & (DF1_FORCE_DOWN | DF1_NO_UP))) &&
	     (c_ptr->feat == FEAT_LESS || c_ptr->feat == FEAT_WAY_LESS)) {
		msg_print(Ind, "\377rThe stairways leading upwards are magically sealed in this dungeon.");
		if (!is_admin(p_ptr)) return;
	}
#endif

#if 1
	/* probability travel/ghost floating restrictions */
	if (dungeon) {
		if (c_ptr->feat != FEAT_LESS && c_ptr->feat != FEAT_WAY_LESS) {
#ifdef NOMAGIC_INHIBITS_LEVEL_PROBTRAVEL
			if (l_ptr && p_ptr->prob_travel && !p_ptr->ghost && (l_ptr->flags1 & LF1_NO_MAGIC)) {
				if (!is_admin(p_ptr)) return;
			}
#endif
#if 0 /* done via NO_MAGIC */
#ifdef NETHERREALM_BOTTOM_RESTRICT
			if (!p_ptr->ghost && netherrealm_bottom(wpos)) {
				msg_print(Ind, "\377rA magical force prevents you from travelling upwards.");
				if (!is_admin(p_ptr)) return;
			}
#endif
#endif
			if (!p_ptr->ghost && (wild_info[wpos->wy][wpos->wx].dungeon->flags1 & (DF1_NO_RECALL | DF1_NO_UP | DF1_FORCE_DOWN))) {
				msg_print(Ind, "\377rA magical force prevents you from travelling upwards.");
				if (!is_admin(p_ptr)) return;
			}
			if (wpos->wz == -1 && p_ptr->ghost && wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_NO_EXIT_FLOAT) {
				msg_print(Ind, "\377rA magical force prevents you from floating upwards.");
				if (!is_admin(p_ptr)) return;
			}
			if (wpos->wz == -1 && p_ptr->prob_travel && wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_NO_EXIT_PROB) {
				msg_print(Ind, "\377rA magical force prevents you from travelling upwards.");
				if (!is_admin(p_ptr)) return;
			}
		}
	} else {
		if (c_ptr->feat != FEAT_LESS && c_ptr->feat != FEAT_WAY_LESS) {
#ifdef NOMAGIC_INHIBITS_LEVEL_PROBTRAVEL
			if (l_ptr && p_ptr->prob_travel && !p_ptr->ghost && (l_ptr->flags1 & LF1_NO_MAGIC)) {
				if (!is_admin(p_ptr)) return;
			}
#endif
			/* for Nether Realm: No ghost diving! */
			if ((wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_NO_RECALL_INTO)) {
				msg_print(Ind, "\377rA magical force prevents you from floating upwards.");
				if (!is_admin(p_ptr)) return;
			}
			if (!p_ptr->ghost &&
			    ((wild_info[wpos->wy][wpos->wx].tower->flags1 & DF1_NO_RECALL) ||
			    //(wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_NO_RECALL_INTO) ||  //redundant, see above
			    (wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_IRON))) {
				msg_print(Ind, "\377rA magical force prevents you from floating upwards.");
				if (!is_admin(p_ptr)) return;
			}
		}
	}
#endif
#ifndef ARCADE_SERVER
	if (surface) {
		dungeon_type *d_ptr = wild_info[wpos->wy][wpos->wx].tower;

 #ifdef OBEY_DUNGEON_LEVEL_REQUIREMENTS
		//if (d_ptr->baselevel-p_ptr->max_dlv>2) {
		//if ((!d_ptr->type && d_ptr->baselevel - p_ptr->max_dlv > 10) ||
		if ((!d_ptr->type && d_ptr->baselevel <= (p_ptr->lev * 3) / 2 + 7) ||
		    (d_ptr->type && d_info[d_ptr->type].min_plev > p_ptr->lev))
		{
			msg_print(Ind, "\377rAs you attempt to ascend, you are gripped by an uncontrollable fear.");
			if (!is_admin(p_ptr)) {
				set_afraid(Ind, 10);//+(d_ptr->baselevel-p_ptr->max_dlv));
				return;
			}
		}
 #endif
		/* Nether Realm only for Kings/Queens*/
		if ((d_ptr->type == DI_NETHER_REALM || d_ptr->type == DI_CLOUD_PLANES) && !p_ptr->total_winner) {
			msg_print(Ind, "\377rAs you attempt to ascend, you are gripped by an uncontrollable fear.");
			if (!is_admin(p_ptr)) {
				set_afraid(Ind, 10);//+(d_ptr->baselevel-p_ptr->max_dlv));
				return;
			}
		}
	}
#endif
	if (p_ptr->inval && p_ptr->wpos.wz >= 10) {
		msg_print(Ind, "\377You may go no higher without a valid account.");
		return;
	}

	if (surface && iddc) {
		if (p_ptr->mode & MODE_PVP) {
			msg_format(Ind, "\377DPvP-mode characters are not eligible to enter the Ironman Deep Dive Challenge!");
			if (!is_admin(p_ptr)) return;
		}
		if (p_ptr->max_plv > IRONDEEPDIVE_MAXLEV &&
		    (IRONDEEPDIVE_MAXLEV || p_ptr->max_exp)) {
			if (IRONDEEPDIVE_MAXLEV) msg_format(Ind, "\377DYou may not enter once you exceeded character level %d!", IRONDEEPDIVE_MAXLEV);
			else msg_print(Ind, "\377DYou may not enter once you gained any experience!");
			if (!is_admin(p_ptr)) return;
		}
#ifdef IDDC_IRON_TEAM_ONLY
		if (p_ptr->party && !(parties[p_ptr->party].mode & PA_IRONTEAM)) {
			msg_print(Ind, "\377yYour party must be an 'Iron Team' to enter the Ironman Deep Dive Challenge.");
			if (!is_admin(p_ptr)) return;
		}
#endif
#ifdef IDDC_RESTRICTED_PARTYING
		/* Force him to re-party once he entered the IDDC.
		   This is required for enforcing the single-char-per-party IDDC rule. */
		if (p_ptr->party) {
			msg_print(Ind, "\377yYou cannot enter the IDDC while already in a party.");
			if (!is_admin(p_ptr)) return;
		}
#endif

		/* S(he) is no longer afk */
		un_afk_idle(Ind);

		/* disable WoR hint */
		p_ptr->warning_wor = 1;
		p_ptr->warning_wor2 = 1;
		p_ptr->warning_depth = 2;
#if 1
		/* Give him some free ID scrolls when entering IDDC? */
		{ object_type forge;

		invcopy(&forge, lookup_kind(TV_SCROLL, SV_SCROLL_IDENTIFY));
		forge.number = 20;
		forge.level = 0;
		forge.discount = 0;
		forge.ident |= ID_MENTAL;
		forge.owner = p_ptr->id;
		forge.mode = p_ptr->mode;
		object_aware(Ind, &forge);
		object_known(&forge);
		i = inven_carry(Ind, &forge);
		if (i != -1 ) {
			char o_name[ONAME_LEN];

			object_desc(Ind, o_name, &forge, TRUE, 3);
			msg_format(Ind, "You have %s (%c).", o_name, index_to_label(i));
		}}
#endif
	} else un_afk_idle(Ind); /* S(he) is no longer afk */

	/* Success */
	if (c_ptr->feat == FEAT_LESS || c_ptr->feat == FEAT_WAY_LESS) {
		p_ptr->warning_staircase = 1;
		process_hooks(HOOK_STAIR, "d", Ind);
		if (surface) {
			dungeon_type *d_ptr = wild_info[wpos->wy][wpos->wx].tower;

			msg_format(Ind, "\377%cYou enter %s..", COLOUR_DUNGEON, get_dun_name(wpos->wx, wpos->wy, TRUE, d_ptr, 0, FALSE));
#ifdef DED_IDDC_AWARE
			if (obtained) msg_print(Ind, "\377gYou obtain some item knowledge.");
#endif
#ifdef RPG_SERVER /* stair scumming in non-IRON dungeons might create mad spam otherwise */
			if (p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->party == p_ptr->party)
					msg_format(i, "\374\377G[\377%c%s has entered %s..\377G]", COLOUR_DUNGEON, p_ptr->name, get_dun_name(wpos->wx, wpos->wy, TRUE, d_ptr, 0, FALSE));
			}
#endif
#ifdef ENABLE_INSTANT_RES
 #ifdef INSTANT_RES_EXCEPTION
			if (d_ptr->type == DI_NETHER_REALM && p_ptr->insta_res)
				msg_print(Ind, "\374\377R*** Warning: Instant Resurrection is not possible in the Nether Realm! ***");
 #endif
#endif
#ifdef GLOBAL_DUNGEON_KNOWLEDGE
			/* we now 'learned' the base level of this dungeon */
			if (!is_admin(p_ptr)) d_ptr->known |= 0x2;
#endif
			/* for jail dungeons, not actually needed, just to stay clean */
			p_ptr->house_num = 0;
		}
#if 0 /* Disable use of dungeon names */
 #ifdef IRONDEEPDIVE_MIXED_TYPES //Kurzel -- Hardcode (final transition floor is 2 currently, transition immediately after static towns, paranoia for last floor)
		else if (in_irondeepdive(wpos) && (iddc[ABS(wpos->wz)].step == 2 || ABS(wpos->wz) == IDDC_TOWN1_FIXED || ABS(wpos->wz) == IDDC_TOWN2_FIXED) && ABS(wpos->wz) != 127) {
			msg_format(Ind, "\377%cYou enter %s..", COLOUR_DUNGEON, d_name + d_info[iddc[ABS(wpos->wz) + 1].type].name);
 #ifdef DED_IDDC_AWARE
			if (obtained) msg_print(Ind, "\377gYou obtain some item knowledge.");
 #endif
			if (p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->party == p_ptr->party)
					msg_format(i, "\374\377G[\377%c%s has entered %s..\377G]", COLOUR_DUNGEON, p_ptr->name, d_name + d_info[iddc[ABS(wpos->wz) + 1].type].name);
			}
		}
		else if (in_irondeepdive(wpos) && ABS(wpos->wz) != 127) {
			if ((!iddc[ABS(wpos->wz)].step && iddc[ABS(wpos->wz) + 1].step) || ABS(wpos->wz) == 39 || ABS(wpos->wz) == 79 || ABS(wpos->wz) == 119)  {
				msg_format(Ind, "\377%cYou leave %s..", COLOUR_DUNGEON, d_name + d_info[iddc[ABS(wpos->wz) + 1].type].name);
				if (p_ptr->party)
				for (i = 1; i <= NumPlayers; i++) {
					if (Players[i]->conn == NOT_CONNECTED) continue;
					if (i == Ind) continue;
					if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
					if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
					if (Players[i]->party == p_ptr->party)
						msg_format(i, "\374\377G[\377%c%s has left %s..\377G]", COLOUR_DUNGEON, p_ptr->name, d_name + d_info[iddc[ABS(wpos->wz) + 1].type].name);
				}
			}
		}
 #endif
#endif
		else if (wpos->wz == -1) {
			msg_format(Ind, "\377%cYou leave %s..", COLOUR_DUNGEON, get_dun_name(wpos->wx, wpos->wy, FALSE, wild_info[wpos->wy][wpos->wx].dungeon, 0, FALSE));
			if (in_deathfate(wpos)) msg_print(Ind, "There is no trace of the mysterious figure, seems you took a different turn.");
#ifdef RPG_SERVER /* stair scumming in non-IRON dungeons might create mad spam otherwise */
			if (p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->party == p_ptr->party)
					msg_format(i, "\374\377G[\377%c%s has left %s..\377G]", COLOUR_DUNGEON, p_ptr->name, get_dun_name(wpos->wx, wpos->wy, FALSE, wild_info[wpos->wy][wpos->wx].dungeon, 0, FALSE));
			}
#endif
		}
		else if (c_ptr->feat == FEAT_WAY_LESS) {
			msg_print(Ind, "You enter the previous area.");
#ifdef RPG_SERVER /* stair scumming in non-IRON dungeons might create mad spam otherwise */
			if (p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->wpos.wz <= 0) continue; /* must be in same dungeon/tower */
				if (Players[i]->party == p_ptr->party) {
					if (Players[i]->depth_in_feet)
						msg_format(i, "\374\377G[\377%c%s took a staircase upwards to %dft..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz + 1) * 50);
					else
						msg_format(i, "\374\377G[\377%c%s took a staircase upwards to lv %d..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz + 1) * 50);
				}
			}
#else
			if (one_way && p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->wpos.wz <= 0) continue; /* must be in same dungeon/tower */
				if (Players[i]->party == p_ptr->party) {
					if (Players[i]->depth_in_feet)
						msg_format(i, "\374\377G[\377%c%s took a staircase upwards to %dft..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz + 1) * 50);
					else
						msg_format(i, "\374\377G[\377%c%s took a staircase upwards to lv %d..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz + 1) * 50);
				}
			}
#endif
		} else {
			msg_print(Ind, "You enter a maze of up staircases.");
#ifdef RPG_SERVER /* stair scumming in non-IRON dungeons might create mad spam otherwise */
			if (p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->wpos.wz <= 0) continue; /* must be in same dungeon/tower */
				if (Players[i]->party == p_ptr->party) {
					if (Players[i]->depth_in_feet)
						msg_format(i, "\374\377G[\377%c%s took a staircase upwards to %dft..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz + 1) * 50);
					else
						msg_format(i, "\374\377G[\377%c%s took a staircase upwards to lv %d..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz + 1) * 50);
				}
			}
#else
			if (one_way && p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->wpos.wz <= 0) continue; /* must be in same dungeon/tower */
				if (Players[i]->party == p_ptr->party) {
					if (Players[i]->depth_in_feet)
						msg_format(i, "\374\377G[\377%c%s took a staircase upwards to %dft..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz + 1) * 50);
					else
						msg_format(i, "\374\377G[\377%c%s took a staircase upwards to lv %d..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz + 1) * 50);
				}
			}
#endif
		}
		p_ptr->new_level_method = LEVEL_UP;
#ifdef USE_SOUND_2010
		staircase_sfx(Ind);
#endif
	} else {
		if (p_ptr->safe_float_turns) {
			msg_print(Ind, "Floating attempt blocked by client option 'safe_float' (in '=1').");
			return;
		}

#ifdef RPG_SERVER /* This is an iron-server... Prob trav should not work - the_sandman */
		msg_print(Ind, "This harsh world knows not what you're trying to do.");
		forget_view(Ind); //the_sandman
		if (!is_admin(p_ptr)) return;
#endif

		if (surface || tower) {
			if (wild_info[wpos->wy][wpos->wx].tower->flags2 & (DF2_IRON
			    | (p_ptr->ghost ? DF2_NO_DEATH : 0))) { //hack: don't ghost into the training tower (might confuse newbies who return from barrow-downs)
				msg_print(Ind, "There is a magic barrier blocking your attempt.");
				if (!is_admin(p_ptr)) return;
			}
		} else {
			if (wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_IRON) {
				msg_print(Ind, "There is a magic barrier blocking your attempt.");
				if (!is_admin(p_ptr)) return;
			}
		}

		if (surface) {
			msg_format(Ind, "\377%cYou float into %s..", COLOUR_DUNGEON, get_dun_name(wpos->wx, wpos->wy, TRUE, wild_info[wpos->wy][wpos->wx].tower, 0, FALSE));
#ifdef GLOBAL_DUNGEON_KNOWLEDGE
			/* we now 'learned' the base level of this dungeon */
			if (!is_admin(p_ptr)) wild_info[wpos->wy][wpos->wx].tower->known |= 0x2;
#endif
		}
		else if (wpos->wz == -1) msg_format(Ind, "\377%cYou float out of %s..", COLOUR_DUNGEON, get_dun_name(wpos->wx, wpos->wy, FALSE, wild_info[wpos->wy][wpos->wx].dungeon, 0, FALSE));
#ifndef PROBTRAVEL_AVOIDS_OTHERS
		else msg_print(Ind, "You float upwards.");
#else
		else if (p_ptr->ghost) msg_print(Ind, "You float upwards.");
#endif
		if (p_ptr->ghost) p_ptr->new_level_method = LEVEL_GHOST;
#ifndef PROBTRAVEL_AVOIDS_OTHERS
		else p_ptr->new_level_method = LEVEL_PROB_TRAVEL;
#else
		else {
			/* Note 1: possible mini-exploit here: if the skipped floor has NO_MAGIC flag
			   and we're running with PROBTRAVEL_AVOIDS_OTHERS (disabled by default though)
			   we'd not get affected even though we're "crossing" that floor, in theory.

			   Note 2: if a whole dungeon is completely blocked by people and we try to
			   probtravel into it, we'll get the "You float into.." message followed by the
			   failure message, which is a bit ugly. :-p */

			int i, z_max;
			player_type *q_ptr;
			bool skip, arrived = FALSE;

			if (surface || tower) z_max = wild_info[wpos->wy][wpos->wx].tower->maxdepth; //into a tower or further up the tower
			else z_max = 0; //from dungeon upwards until surface (redundant, actually)

			do {
				wpos->wz++;

				/* always allow to float up to the surface, leaving a dungeon */
				if (!wpos->wz) {
					arrived = TRUE;
					break;
				}

				skip = FALSE;
				for (i = 1; i <= NumPlayers; i++) {
					if (i == Ind) continue;
					q_ptr = Players[i];
					if (!inarea(&q_ptr->wpos, wpos)) continue;
					if (q_ptr->party && p_ptr->party && player_in_party(q_ptr->party, Ind)) {
						skip = FALSE;
						break;
					}
					if (q_ptr->ghost) continue; /* Allow rescuing someone via probtravel */
					skip = TRUE;
				}
				if (!skip) {
					arrived = TRUE;
					break;
				}
			} while (wpos->wz < z_max);

			z = wpos->wz; /* Remember new depth in case of successful probtravel */
			wpos->wz = old_wpos.wz; /* restore our depth for now, it will be set to z again when handling successful probtravel. */

			/* Unsuccessful probtravel */
			if (!arrived) {
				msg_print(Ind, "There is a magical discharge in the air as probability travel fails!");
				return;
			}

			p_ptr->new_level_method = LEVEL_PROB_TRAVEL;
			msg_print(Ind, "You float upwards.");
		}
#endif
	}

	if (c_ptr->custom_lua_way) exec_lua(0, format("custom_way(%d,%d)", Ind, c_ptr->custom_lua_way));

	/* Remove the player from the old location */
	c_ptr->m_idx = 0;

	/* Show everyone that's he left */
	everyone_lite_spot_move(Ind, wpos, p_ptr->py, p_ptr->px);

	/* Forget his lite and viewing area */
	forget_lite(Ind);
	forget_view(Ind);

	/* Hack -- take a turn */
	p_ptr->energy -= level_speed(wpos);

	/* A player has left this depth */
#ifdef PROBTRAVEL_AVOIDS_OTHERS
	if (p_ptr->new_level_method != LEVEL_PROB_TRAVEL) {
#endif
		wpos->wz++;
#ifdef PROBTRAVEL_AVOIDS_OTHERS
	} else wpos->wz = z; /* Set depth to probtravel result, possibly having skipped some floors. */
#endif
	new_players_on_depth(&old_wpos, -1, TRUE);
	p_ptr->new_level_flag = TRUE;
	new_players_on_depth(wpos, 1, TRUE);

	forget_view(Ind); //the_sandman

	/* He'll be safe for 2 turns */
	set_invuln_short(Ind, STAIR_GOI_LENGTH);

	/* Create a way back */
	create_down_stair = TRUE; /* appearently unused */

	/* C. Blue -- Megahack to fix the player having enough energy left to perform another action after taking upstairs,
	   but insufficient energy to perform another action after taking downstairs, due to different new floor speeds!
	   Especially for going upwards this is required to fix the "you cannot see" bug, when the player has enough
	   energy to cast a spell BEFORE process_player_change_wpos() gets called, while when going downstairs,
	   in receive_activate_skill() the spellcast command will rebound once due to insufficient energy, resulting in
	   the spell to actually work (even though this could be considered exactly the wrong way around :D)..
	   So this hack in both cmd_go_up and cmd_go_down fixes two things:
	   1) them being treated inequally, since after up the player can still act while after down he can't
	   2) the "you cannot see" bug that results from '1)' when trying to cast right after going up. */
	if (p_ptr->energy >= level_speed(wpos)) p_ptr->energy = level_speed(wpos) - 1;
}

/*
 * Returns TRUE if we are in the Between...
 */
/* I want to be famous, a star of the screen,
 * but you can do something in between...
 */
static bool between_effect(int Ind, cave_type *c_ptr) {
	player_type *p_ptr = Players[Ind];
	byte bx, by;
	struct c_special *cs_ptr;
	struct dun_level *l_ptr = getfloor(&p_ptr->wpos);
	int wid, hgt;

	if (!(cs_ptr = GetCS(c_ptr, CS_BETWEEN))) return(FALSE);

	/* allow void gates on surface levels (for 0,0 events) */
	if (l_ptr) {
		wid = l_ptr->wid - 1;
		hgt = l_ptr->hgt - 1;
	} else {
		wid = MAX_WID - 1;
		hgt = MAX_HGT - 1;
	}

	by = cs_ptr->sc.between.fy;
	bx = cs_ptr->sc.between.fx;

	/* (HACK) sanity check to cover cut-off vaults with missing void gate end-points! - C. Blue */
	if (bx < 1 || by < 1 || bx >= wid || by >= hgt) {
		msg_print(Ind, "The gate seems broken.");
		return(TRUE);
	}

	msg_print(Ind, "You fall into the void. Brrrr! It's deadly cold.");

	//if (PRACE_FLAG(PR1_TP))
	if (p_ptr->prace == RACE_DRACONIAN) {
		int dam = (p_ptr->mhp * 50) / (100 + p_ptr->ac + p_ptr->to_a + p_ptr->lev * 2);

		dam = (dam * (100 + distance(by, bx, p_ptr->py, p_ptr->px))) / 200 + 5;
		bypass_invuln = TRUE;
		take_hit(Ind, dam, "going between", 0);
		bypass_invuln = FALSE;
	}

#ifdef USE_SOUND_2010
	//sound(Ind, "teleport", NULL, SFX_TYPE_COMMAND, TRUE);
	sound(Ind, "phase_door", NULL, SFX_TYPE_COMMAND, TRUE);
#endif

#ifdef ENABLE_SELF_FLASHING
	if (
#endif
	    swap_position(Ind, by, bx)
#ifdef ENABLE_SELF_FLASHING
	    && p_ptr->flash_self) {
		/* flicker player for a moment, to allow for easy location */
		/* -- only do this when our view panel has changed */
		p_ptr->flashing_self = cfg.fps / FLASH_SELF_DIV;
	}
#else
	    ;
#endif

	/* To avoid being teleported back */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	return(TRUE);
}

/* for special gates (event: Dungeon Keeper) - C. Blue
   Taking a transport beacon usually teleports the player to Bree
   and [successfully] terminates all global events for him. */
static bool beacon_effect(int Ind, cave_type *c_ptr) {
	player_type *p_ptr = Players[Ind];
	signed char ev_idx;
	s16b k, parm;
	char buf[1024];
	global_event_type *ge;
	object_type forge, *o_ptr = &forge;

#ifdef USE_SOUND_2010
	sound(Ind, "recall", NULL, SFX_TYPE_COMMAND, TRUE); //"teleport"
#endif

	/* Beacons in sector00 dungeon/tower/surface lead to Bree transportation; other beacons aren't used atm. */
	if (!in_sector00(&p_ptr->wpos)) return(FALSE);

	for (ev_idx = 0; ev_idx < MAX_GLOBAL_EVENTS; ev_idx++) {
		ge = &global_event[ev_idx];
		if (ge->getype == GE_NONE) continue; /* This event is not active */
		for (k = 0; k < 128; k++) {
			if (ge->beacon_wpos[k].wz == 32767) continue; /* Marker for 'unused' beacon_wpos slot. */
			if (!inarea(&ge->beacon_wpos[k], &p_ptr->wpos)) continue; /* This event has no beacon located here */
			parm = ge->beacon_parm[k];
			break;
		}
		if (k != 128) break;
	}
	if (ev_idx == MAX_GLOBAL_EVENTS) return(FALSE); /* orphaned beacon */
	(void)parm; /* slay compiler warning 'unused' */

	/* Unsign, they have left the event, in case of ongoing events - Kurzel */
	for (k = 0; k < MAX_GE_PARTICIPANTS; k++)
		if (ge->participant[k] == p_ptr->id) {
			s_printf("%s EVENT_UNPARTICIPATE (beacon): '%s' (%d) -> #%d '%s'(%d) [%d]\n", showtime(), p_ptr->name, p_ptr->Ind, ev_idx, ge->title, ge->getype, k);
			ge->participant[k] = 0;
			break;
		}
	/* Player is not participant of the event belonging to this beacon? oO */
	if (k == MAX_GE_PARTICIPANTS) {
		s_printf("beacon_effect: player '%s' is not participant of event #%d!\n", p_ptr->name, ev_idx);
#if 0 /* allow this for now ;) if only for admin-controlled testing of events */
		return(FALSE);
#endif
	}

	/* Paranoia: Player might have signed up for an event that is now no longer available/cancelled. */
	if (!ge->getype) {
		p_ptr->global_event_type[ev_idx] = GE_NONE; /* no longer participant */
		return(FALSE);
	} else p_ptr->global_event_type[ev_idx] = ge->getype; // Paranoia - reset unexpected 0 here - Kurzel


	switch (p_ptr->global_event_type[ev_idx]) {
#ifdef DM_MODULES
	case GE_ADVENTURE:
		/* tell everyone + himself that he won */
		sprintf(buf, "\374\377a>>%s completed %s!<<", p_ptr->name, ge->title);
		msg_broadcast(0, buf);
 #ifdef TOMENET_WORLDS
		if (cfg.worldd_events) world_msg(buf);
 #endif
 #ifdef USE_SOUND_2010
		sound(Ind, "success", NULL, SFX_TYPE_MISC, FALSE);
 #endif
		s_printf("%s EVENT_WON: %s wins %d '%s'(%d)\n", showtime(), p_ptr->name, ev_idx + 1, ge->title, ge->getype);
		p_ptr->event_won_flags |= 1 << (GE_ADVENTURE - 1 + ge->extra[0]); // HACK - Paranoia - Kurzel
		break; // No additional rewards for now. - Kurzel
#endif
	case GE_DUNGEON_KEEPER:
		/* tell everyone + himself that he won */
		sprintf(buf, "\374\377a>>%s wins %s!<<", p_ptr->name, ge->title);
		msg_broadcast(0, buf);
#ifdef TOMENET_WORLDS
		if (cfg.worldd_events) world_msg(buf);
#endif
#ifdef USE_SOUND_2010
		sound(Ind, "success", NULL, SFX_TYPE_MISC, FALSE);
#endif
		s_printf("%s EVENT_WON: %s wins %d '%s'(%d)\n", showtime(), p_ptr->name, ev_idx + 1, ge->title, ge->getype);
		//l_printf("%s \\{s%s has won %s\n", showdate(), p_ptr->name, ge->title);
		p_ptr->event_won_flags |= 1 << (GE_DUNGEON_KEEPER - 1);

		/* boost him to level 3, so he can distribute enough skills for the reward creation to work */
		if (p_ptr->max_lev < 3) gain_exp_to_level(Ind, 3);
		/* extra optional niceness :-p boost already-level-3 chars to level 4 just for the heck of it.. */
		else gain_exp_to_level(Ind, 4);

		/* create reward parchment */
		k = lookup_kind(TV_PARCHMENT, SV_DEED_DUNGEONKEEPER);
		invcopy(o_ptr, k);
		o_ptr->number = 1;
		object_aware(Ind, o_ptr);
		object_known(o_ptr);
		o_ptr->discount = 0;
		o_ptr->level = 0;
		o_ptr->ident |= ID_MENTAL;
		//o_ptr->note = quark_add("Dungeon Keeper reward");
		inven_carry(Ind, o_ptr);
		break;
	case GE_NONE:
	default:
		break;
	}

	p_ptr->global_event_type[ev_idx] = GE_NONE; /* no longer participant */

	msg_print(Ind, "\377GYou are transported out of here and far away!");
	p_ptr->recall_pos = BREE_WPOS;
	p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
	p_ptr->global_event_temp = 0x0; /* clear all flags */
	recall_player(Ind, "");

	return(TRUE);
}

/*
 * Go down one level
 */
void do_cmd_go_down(int Ind) {
	player_type *p_ptr = Players[Ind];
	monster_race *r_ptr = &r_info[p_ptr->body_monster];
	cave_type *c_ptr;
	struct worldpos *wpos = &p_ptr->wpos, old_wpos = p_ptr->wpos;
	bool tower = FALSE, dungeon = FALSE, surface = FALSE;
	cave_type **zcave;
#ifndef RPG_SERVER
	bool one_way = FALSE; //ironman, no_up, force_down
#endif
	int i;
#ifdef PROBTRAVEL_AVOIDS_OTHERS
	int z = 0; /* Just to suppress compiler warning */
#endif
#ifdef NOMAGIC_INHIBITS_LEVEL_PROBTRAVEL
	struct dun_level *l_ptr;
#endif
#ifdef DED_IDDC_AWARE
	bool obtained = FALSE;
#endif
	dungeon_type *d_ptr = NULL;
	bool iddc = (p_ptr->wpos.wx == WPOS_IRONDEEPDIVE_X && p_ptr->wpos.wy == WPOS_IRONDEEPDIVE_Y && 0 -1 == WPOS_IRONDEEPDIVE_Z);
	bool mandos = (p_ptr->wpos.wx == hallsofmandos_wpos_x && p_ptr->wpos.wy == hallsofmandos_wpos_y && -1 == hallsofmandos_wpos_z);

	if (!(zcave = getcave(wpos))) return;
#ifdef NOMAGIC_INHIBITS_LEVEL_PROBTRAVEL
	l_ptr = getfloor(wpos);
#endif

	if (wpos->wz > 0) {
		tower = TRUE;
		d_ptr = wild_info[wpos->wy][wpos->wx].tower;
	}
	if (wpos->wz < 0) {
		dungeon = TRUE;
		d_ptr = wild_info[wpos->wy][wpos->wx].dungeon;
	}
	if (wpos->wz == 0) surface = TRUE;

#ifndef RPG_SERVER
	/* Is this a one-way dungeon? */
	if (dungeon) {
		if ((wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_IRON) ||
		    (wild_info[wpos->wy][wpos->wx].dungeon->flags1 & (DF1_FORCE_DOWN | DF1_NO_UP)))
			one_way = TRUE;
	}
#endif

	/* Make sure he hasn't just changed depth */
	if (p_ptr->new_level_flag)
		return;

	/* Can we move ? */
	if (r_ptr->flags2 & RF2_NEVER_MOVE) {
		msg_print(Ind, "You cannot move by nature.");
		return;
	}

#if STAIR_FAIL_IF_CONFUSED
	/* Hack -- handle confusion */
	if (p_ptr->confused && magik(STAIR_FAIL_IF_CONFUSED)) {
		int dir = 5, tries = 10;

		/* Prevent walking nowhere */
		while (dir == 5 && --tries)
			dir = rand_int(9) + 1;

		do_cmd_walk(Ind, dir, FALSE);
		return;
	}
#endif // STAIR_FAIL_IF_CONFUSED


	/* Player grid */
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* Hack -- Enter a store (and between gates, etc) */
	if ((!p_ptr->ghost || is_admin(p_ptr)) &&
	    (c_ptr->feat == FEAT_SHOP))
#if 0
	    (c_ptr->feat >= FEAT_SHOP_HEAD) &&
	    (c_ptr->feat <= FEAT_SHOP_TAIL))
#endif	// 0
	{
		/* Disturb */
		disturb(Ind, 1, 0);

		/* Hack -- Enter store */
		command_new = '_';
		do_cmd_store(Ind);
		return;
	}

	if ((p_ptr->mode & MODE_DED_IDDC) && surface) {
		if (!iddc
#ifdef DED_IDDC_MANDOS
		    && !mandos
#endif
		    ) {
#ifdef DED_IDDC_MANDOS
			msg_print(Ind, "\377yYou may only enter the Ironman Deep Dive Challenge or the Halls of Mandos!");
#else
			msg_print(Ind, "\377yYou may not enter any other dungeon besides the Ironman Deep Dive Challenge!");
#endif
			return;
		}
#ifdef DED_IDDC_AWARE
		if (iddc) {
			for (i = 0; i < MAX_K_IDX; i++)
				if (magik(DED_IDDC_AWARE)) p_ptr->obj_aware[i] = TRUE;
			obtained = TRUE;
		}
#endif
	}

	if (cfg.runlevel < 5 && surface) {
		msg_print(Ind, "The dungeon is closed");
		return;
	}

	if (c_ptr->feat == FEAT_BETWEEN) {
		if (d_ptr && !d_ptr->type && d_ptr->theme == DI_DEATH_FATE) {
			p_ptr->player_sees_dm = (p_ptr->inventory[INVEN_HEAD].tval && p_ptr->inventory[INVEN_HEAD].name1 == ART_GOGGLES_DM);

			un_afk_idle(Ind);

			c_ptr->m_idx = 0;
			everyone_lite_spot_move(Ind, wpos, p_ptr->py, p_ptr->px);
			forget_lite(Ind);
			forget_view(Ind);
			p_ptr->energy -= level_speed(&p_ptr->wpos);
			p_ptr->new_level_method = LEVEL_GHOST;
			wpcopy(&old_wpos, wpos);
			if (wpos->wz > 0) wpos->wz--; else wpos->wz++;
			new_players_on_depth(&old_wpos, -1, TRUE);
			p_ptr->new_level_flag = TRUE;
			new_players_on_depth(wpos, 1, TRUE);
			//forget_view(Ind); //the_sandman

			msg_print(Ind, "You leave the party, passing through a gate obscured by magical fog...");
			set_invuln_short(Ind, STAIR_GOI_LENGTH);
			if (!players_on_depth(&old_wpos)) {
				if (old_wpos.wz > 0) {
					if (wild_info[old_wpos.wy][old_wpos.wx].tower) (void)rem_dungeon(&old_wpos, TRUE);
				} else {
					if (wild_info[old_wpos.wy][old_wpos.wx].dungeon) (void)rem_dungeon(&old_wpos, FALSE);
				}
			}
			return;
		}

		/* Check interference */
		if (interfere(Ind, 20)) { /* between gate interference chance */
			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			return;
		}

		if (c_ptr->custom_lua_way) exec_lua(0, format("custom_way(%d,%d)", Ind, c_ptr->custom_lua_way));
		p_ptr->warning_voidjumpgate = 1;
		if (between_effect(Ind, c_ptr)) return;
		/* not jumped? strange.. */
	}

	if (c_ptr->feat == FEAT_IRID_GATE) {
		struct worldpos twpos = { WPOS_DF_X, WPOS_DF_Y, WPOS_DF_Z };
		cave_type **zcave;

		for (i = 0; i < INVEN_PACK; i++) if (p_ptr->inventory[i].tval == TV_JUNK && p_ptr->inventory[i].sval == SV_GLASS_SHARD) {
			/* Only travel to the 2nd instance if the first instance isn't already spawned instead. */
			if ((zcave = getcave(&twpos))) {
				struct dun_level *l_ptr = getfloor(&twpos);

				if (!(l_ptr->flags2 & LF2_BROKEN)) {
					//msg_format(Ind, "The gate seems broken, but %s flashes brightly for a moment but then subsides.", p_ptr->inventory[i].number != 1 ? "one of your glass shards" : "your glass shard");
					msg_format(Ind, "%s flashes brightly for a moment but subsides again.", p_ptr->inventory[i].number != 1 ? "One of your glass shards" : "Your glass shard");
					return;
				}
			}

			/* S(he) is no longer afk */
			un_afk_idle(Ind);

			msg_format(Ind, "%s disintegrates in a flurry of colours...", p_ptr->inventory[i].number != 1 ? "One of your glass shards" : "Your glass shard");
			inven_item_increase(Ind, i, -1);
			inven_item_describe(Ind, i);
			inven_item_optimize(Ind, i);
			p_ptr->recall_pos.wx = twpos.wx;
			p_ptr->recall_pos.wy = twpos.wy;
			p_ptr->recall_pos.wz = twpos.wz;
			p_ptr->new_level_method = LEVEL_RAND;
			p_ptr->temp_misc_1 |= 0x80;
			recall_player(Ind, "");
			p_ptr->auto_transport = AT_MUMBLE;
			p_ptr->auto_transport_turn = turn;
			return;
		}
		msg_print(Ind, "The gate twinkles silvery for a moment but it seems broken.");
		return;
	}

	if (c_ptr->feat == FEAT_BEACON) {
		/* Hack -- take a turn */
		p_ptr->energy -= level_speed(&p_ptr->wpos);

		/* Check interference */
		if (interfere(Ind, 20)) return; /* beacon interference chance */

		if (c_ptr->custom_lua_way) exec_lua(0, format("custom_way(%d,%d)", Ind, c_ptr->custom_lua_way));
		if (beacon_effect(Ind, c_ptr)) return;
		/* not transported? strange.. */
	}

	/* Quest stairs to preserve xy position! - Kurzel */
	if (c_ptr->feat == FEAT_QUEST_DOWN) {
		p_ptr->energy -= level_speed(&p_ptr->wpos);
		if (interfere(Ind, 20)) return;
		un_afk_idle(Ind);
		if (c_ptr->custom_lua_way) exec_lua(0, format("custom_way(%d,%d)", Ind, c_ptr->custom_lua_way));
		everyone_lite_spot_move(Ind, wpos, p_ptr->py, p_ptr->px);
		forget_lite(Ind);
		forget_view(Ind);
		p_ptr->new_level_method = LEVEL_PROB_TRAVEL;
		wpcopy(&old_wpos, wpos);
		wpos->wz -= 1;
		msg_print(Ind, "You go down stairs.");
		new_players_on_depth(&old_wpos, -1, TRUE);
		p_ptr->new_level_flag = TRUE;
		new_players_on_depth(wpos, 1, TRUE);
		set_invuln_short(Ind, STAIR_GOI_LENGTH);
		c_ptr->m_idx = 0;
		return;
	}

	if (c_ptr->feat == FEAT_CYCLIC_MORE) {
		/* Hack -- take a turn */
		p_ptr->energy -= level_speed(wpos);

		/* Hack: Leave the dungeon, as we're taking a 'physical staircase out'! */
		un_afk_idle(Ind);

		everyone_lite_spot_move(Ind, wpos, p_ptr->py, p_ptr->px);
		forget_lite(Ind);
		forget_view(Ind);
		p_ptr->energy -= level_speed(wpos);
		p_ptr->new_level_method = LEVEL_GHOST;
		if (c_ptr->custom_lua_way) exec_lua(0, format("custom_way(%d,%d)", Ind, c_ptr->custom_lua_way));
		wpcopy(&old_wpos, wpos);
		wpos->wz = 0;
		new_players_on_depth(&old_wpos, -1, TRUE);
		p_ptr->new_level_flag = TRUE;
		new_players_on_depth(wpos, 1, TRUE);

		msg_format(Ind, "\377%cYou leave %s..", COLOUR_DUNGEON, get_dun_name(old_wpos.wx, old_wpos.wy, TRUE, wild_info[old_wpos.wy][old_wpos.wx].tower, 0, FALSE));
#ifdef RPG_SERVER /* stair scumming in non-IRON dungeons might create mad spam otherwise */
		if (p_ptr->party)
		for (i = 1; i <= NumPlayers; i++) {
		        if (Players[i]->conn == NOT_CONNECTED) continue;
			if (i == Ind) continue;
			if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
			if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
			if (Players[i]->party == p_ptr->party)
				msg_format(i, "\374\377G[\377%c%s has left %s..\377G]", COLOUR_DUNGEON, p_ptr->name, get_dun_name(old_wpos.wx, old_wpos.wy, TRUE, wild_info[old_wpos.wy][old_wpos.wx].tower, 0, FALSE));
		}
#endif
		set_invuln_short(Ind, STAIR_GOI_LENGTH);
		return;
	}

	/* Verify stairs */
	//if (!p_ptr->ghost && (strcmp(p_ptr->name,cfg_admin_wizard)) && c_ptr->feat != FEAT_MORE && !p_ptr->prob_travel)
	if (!is_admin(p_ptr) &&
	    c_ptr->feat != FEAT_MORE && c_ptr->feat != FEAT_WAY_MORE &&
	    (!p_ptr->prob_travel || (c_ptr->info & CAVE_STCK)))
	{
		struct worldpos twpos;

		wpcopy(&twpos, wpos);
		if (twpos.wz > 0) twpos.wz--;
		if (!p_ptr->ghost) {
			msg_print(Ind, "I see no down staircase here.");
			return;
		}
#if 1	// no need for this anymore - C. Blue
	// ..argh there is! people exploit the hell out of every possibility they see. - C. Blue
#ifdef SEPARATE_RECALL_DEPTHS
		else if ((get_recall_depth(&twpos, p_ptr) + 5 <= ABS(twpos.wz)) && (twpos.wz != 0)) /* player may return to town though! */
#else
		else if ((p_ptr->max_dlv + 5 <= getlevel(&twpos)) && (twpos.wz != 0)) /* player may return to town though! */
#endif
		{
			/* anti Ghost-dive */
			msg_print(Ind, "A mysterious force prevents you from going down.");
			return;
		}
#endif
	}
	if (surface) {
		if (!(wild_info[wpos->wy][wpos->wx].flags & WILD_F_DOWN)) {
			if (c_ptr->feat == FEAT_MORE || c_ptr->feat == FEAT_WAY_MORE)
				msg_print(Ind, "\377sOnly mud-filled ruins of former catacombs remain at this place..");
			else msg_print(Ind, "There is nothing below you.");
			return;
		} else if ((c_ptr->feat != FEAT_MORE && c_ptr->feat != FEAT_WAY_MORE) &&
			    ((!p_ptr->prob_travel || (wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_NO_ENTRY_PROB)) &&
			    (!p_ptr->ghost || (wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_NO_ENTRY_FLOAT)))) {
			msg_print(Ind, "There is nothing below you.");
			if (!is_admin(p_ptr)) return;
		}
	}

	/* Verify maximum depth */
	if (dungeon && wild_info[wpos->wy][wpos->wx].dungeon->maxdepth == -wpos->wz) {
		msg_print(Ind, "You are at the bottom of the dungeon.");
		return;
	}

#ifdef RPG_SERVER /* Exclude NO_DEATH dungeons from the gameplay */
	if ((surface) && (wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_NO_DEATH)) {
		msg_print(Ind, "\377sOnly mud-filled ruins of former catacombs remain at this place..");
		if (!is_admin(p_ptr)) return;
	}
#endif

	if ((p_ptr->global_event_temp & PEVF_SEPDUN_00) && (p_ptr->wpos.wz == 1)) {
		msg_print(Ind, "\377sThe way is blocked by huge chunks of rocks!");
		if (!is_admin(p_ptr)) return;
	}

#if 0 /* 0'ed because in such a dungeon there shouldn't be any staircases generated in the first place. If there are, then it's probably for some special thingy and intended. Or it's a bug. */
	/*if (tower && !p_ptr->ghost && (wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_IRON)) {*/
	if (tower && ((wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_IRON) ||
	     (wild_info[wpos->wy][wpos->wx].tower->flags1 & (DF1_FORCE_DOWN | DF1_NO_UP))) &&
	    (c_ptr->feat == FEAT_MORE || c_ptr->feat == FEAT_WAY_MORE)) {
		msg_print(Ind, "\377rThe stairways leading downwards are magically sealed in this tower.");
		if (!is_admin(p_ptr)) return;
	}
#endif

#if 1
	/* probability travel/ghost floating restrictions */
	if (tower) {
		if ((c_ptr->feat != FEAT_MORE) && (c_ptr->feat != FEAT_WAY_MORE)) {
#ifdef NOMAGIC_INHIBITS_LEVEL_PROBTRAVEL
			if (l_ptr && p_ptr->prob_travel && !p_ptr->ghost && (l_ptr->flags1 & LF1_NO_MAGIC)) {
				if (!is_admin(p_ptr)) return;
			}
#endif
#if 0 /* done via NO_MAGIC */
#ifdef NETHERREALM_BOTTOM_RESTRICT
			if (!p_ptr->ghost && netherrealm_bottom(wpos)) {
				msg_print(Ind, "\377rA magical force prevents you from travelling downwards.");
				if (!is_admin(p_ptr)) return;
			}
#endif
#endif
			if ((!p_ptr->ghost) && (wild_info[wpos->wy][wpos->wx].tower->flags1 & (DF1_NO_RECALL | DF1_NO_UP | DF1_FORCE_DOWN))) {
				msg_print(Ind, "\377rA magical force prevents you from travelling downwards.");
				if (!is_admin(p_ptr)) return;
			}
			if (wpos->wz == 1 && p_ptr->ghost && wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_NO_EXIT_FLOAT) {
				msg_print(Ind, "\377rA magical force prevents you from floating downwards.");
				if (!is_admin(p_ptr)) return;
			}
			if (wpos->wz == 1 && p_ptr->prob_travel && wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_NO_EXIT_PROB) {
				msg_print(Ind, "\377rA magical force prevents you from travelling downwards.");
				if (!is_admin(p_ptr)) return;
			}
		}
	} else {
		if ((c_ptr->feat != FEAT_MORE) && (c_ptr->feat != FEAT_WAY_MORE)) {
#ifdef NOMAGIC_INHIBITS_LEVEL_PROBTRAVEL
			if (l_ptr && p_ptr->prob_travel && !p_ptr->ghost && (l_ptr->flags1 & LF1_NO_MAGIC)) {
				if (!is_admin(p_ptr)) return;
			}
#endif
			/* for Nether Realm: No ghost diving! */
			if ((wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_NO_RECALL_INTO)) {
				msg_print(Ind, "\377rA magical force prevents you from floating downwards.");
				if (!is_admin(p_ptr)) return;
			}
			if (!p_ptr->ghost &&
			    ((wild_info[wpos->wy][wpos->wx].dungeon->flags1 & DF1_NO_RECALL) ||
			    //(wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_NO_RECALL_INTO) ||  //redundant, see above
			    (wild_info[wpos->wy][wpos->wx].dungeon->flags2 & DF2_IRON))) {
				msg_print(Ind, "\377rA magical force prevents you from floating downwards.");
				if (!is_admin(p_ptr)) return;
			}
		}
	}
#endif

#ifndef ARCADE_SERVER
	if (surface) {
		dungeon_type *d_ptr = wild_info[wpos->wy][wpos->wx].dungeon;

 #ifdef OBEY_DUNGEON_LEVEL_REQUIREMENTS
		//if (d_ptr->baselevel-p_ptr->max_dlv>2) {
		//if (d_ptr->baselevel-p_ptr->max_dlv>2 ||
		//    d_info[d_ptr->type].min_plev > p_ptr->lev)
		//if ((!d_ptr->type && d_ptr->baselevel-p_ptr->max_dlv > 10) ||
		if ((!d_ptr->type && d_ptr->baselevel <= (p_ptr->lev * 3) / 2 + 7) ||
		    (d_ptr->type && d_info[d_ptr->type].min_plev > p_ptr->lev)) {
			msg_print(Ind, "\377rAs you attempt to descend, you are gripped by an uncontrollable fear.");
			if (!is_admin(p_ptr)) {
				set_afraid(Ind, 10);//+(d_ptr->baselevel-p_ptr->max_dlv));
				return;
			}
		}
 #endif
		/* Nether Realm only for Kings/Queens*/
		if ((d_ptr->type == DI_NETHER_REALM || d_ptr->type == DI_CLOUD_PLANES) && !p_ptr->total_winner) {
			msg_print(Ind, "\377rAs you attempt to descend, you are gripped by an uncontrollable fear.");
			if (!is_admin(p_ptr)) {
				set_afraid(Ind, 10);//+(d_ptr->baselevel-p_ptr->max_dlv));
				return;
			}
		}
	}
#endif

	if (p_ptr->inval && p_ptr->wpos.wz <= -10) {
		msg_print(Ind, "\377You may go no lower without a valid account.");
		return;
	}

	if (surface && iddc) {
		if (p_ptr->mode & MODE_PVP) {
			msg_format(Ind, "\377DPvP-mode characters are not eligible to enter the Ironman Deep Dive Challenge!");
			if (!is_admin(p_ptr)) return;
		}
		if (p_ptr->max_plv > IRONDEEPDIVE_MAXLEV &&
		    (IRONDEEPDIVE_MAXLEV || p_ptr->max_exp)) {
			if (IRONDEEPDIVE_MAXLEV) msg_format(Ind, "\377DYou may not enter once you exceeded character level %d!", IRONDEEPDIVE_MAXLEV);
			else msg_print(Ind, "\377DYou may not enter once you gained any experience!");
			if (!is_admin(p_ptr)) return;
		}
#ifdef IDDC_IRON_TEAM_ONLY
		if (p_ptr->party && !(parties[p_ptr->party].mode & PA_IRONTEAM)) {
			msg_print(Ind, "\377yYour party must be an 'Iron Team' to enter the Ironman Deep Dive Challenge.");
			if (!is_admin(p_ptr)) return;
		}
#endif
#ifdef IDDC_RESTRICTED_PARTYING
		/* Force him to re-party once he entered the IDDC.
		   This is required for enforcing the single-char-per-party IDDC rule. */
		if (p_ptr->party) {
			msg_print(Ind, "\377yYou cannot enter the IDDC while already in a party.");
			if (!is_admin(p_ptr)) return;
		}
#endif

		/* S(he) is no longer afk */
		un_afk_idle(Ind);

		/* disable WoR hint */
		p_ptr->warning_wor = 1;
		p_ptr->warning_wor2 = 1;
		p_ptr->warning_depth = 2;
#if 1
		/* Give him some free ID scrolls when entering IDDC? */
		{ object_type forge;

		invcopy(&forge, lookup_kind(TV_SCROLL, SV_SCROLL_IDENTIFY));
		forge.number = 20;
		forge.level = 0;
		forge.discount = 0;
		forge.ident |= ID_MENTAL;
		forge.owner = p_ptr->id;
		forge.mode = p_ptr->mode;
		object_aware(Ind, &forge);
		object_known(&forge);
		i = inven_carry(Ind, &forge);
		if (i != -1 ) {
			char o_name[ONAME_LEN];

			object_desc(Ind, o_name, &forge, TRUE, 3);
			msg_format(Ind, "You have %s (%c).", o_name, index_to_label(i));
		}}
#endif
	} else un_afk_idle(Ind); /* S(he) is no longer afk */

	/* Success */
	if (c_ptr->feat == FEAT_MORE || c_ptr->feat == FEAT_WAY_MORE) {
		p_ptr->warning_staircase = 1;
		process_hooks(HOOK_STAIR, "d", Ind);
		if (surface) {
			dungeon_type *d_ptr = wild_info[wpos->wy][wpos->wx].dungeon;

			msg_format(Ind, "\377%cYou enter %s..", COLOUR_DUNGEON, get_dun_name(wpos->wx, wpos->wy, FALSE, d_ptr, 0, FALSE));
#ifdef DED_IDDC_AWARE
			if (obtained) msg_print(Ind, "\377gYou obtain some item knowledge.");
#endif
#ifdef RPG_SERVER /* stair scumming in non-IRON dungeons might create mad spam otherwise */
			if (p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->party == p_ptr->party)
					msg_format(i, "\374\377G[\377%c%s has entered %s..\377G]", COLOUR_DUNGEON, p_ptr->name, get_dun_name(wpos->wx, wpos->wy, FALSE, d_ptr, 0, FALSE));
			}
#endif
#ifdef ENABLE_INSTANT_RES
 #ifdef INSTANT_RES_EXCEPTION
			if (d_ptr->type == DI_NETHER_REALM && p_ptr->insta_res)
				msg_print(Ind, "\374\377R*** Warning: Instant Resurrection is not possible in the Nether Realm! ***");
 #endif
#endif
#ifdef GLOBAL_DUNGEON_KNOWLEDGE
			/* we now 'learned' the base level of this dungeon */
			if (!is_admin(p_ptr)) d_ptr->known |= 0x2;
#endif
			/* for jail dungeons, not actually needed, just to stay clean */
			p_ptr->house_num = 0;
		}
#if 0 /* Disable use of dungeon names */
 #ifdef IRONDEEPDIVE_MIXED_TYPES //Kurzel -- Hardcode (final transition floor is 2 currently, transition immediately after static towns, paranoia for last floor)
		else if (in_irondeepdive(wpos) && (iddc[ABS(wpos->wz)].step == 2 || ABS(wpos->wz) == IDDC_TOWN1_FIXED || ABS(wpos->wz) == IDDC_TOWN2_FIXED) && ABS(wpos->wz) != 127) {
			msg_format(Ind, "\377%cYou enter %s..", COLOUR_DUNGEON, d_name + d_info[iddc[ABS(wpos->wz) + 1].type].name);
 #ifdef DED_IDDC_AWARE
			if (obtained) msg_print(Ind, "\377gYou obtain some item knowledge.");
 #endif
			if (p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->party == p_ptr->party)
					msg_format(i, "\374\377G[\377%c%s has entered %s..\377G]", COLOUR_DUNGEON, p_ptr->name, d_name + d_info[iddc[ABS(wpos->wz) + 1].type].name);
			}
		}
		else if (in_irondeepdive(wpos) && ABS(wpos->wz) != 127) {
			if ((!iddc[ABS(wpos->wz)].step && iddc[ABS(wpos->wz) + 1].step) || ABS(wpos->wz) == 39 || ABS(wpos->wz) == 79 || ABS(wpos->wz) == 119)  {
				msg_format(Ind, "\377%cYou leave %s..", COLOUR_DUNGEON, d_name + d_info[iddc[ABS(wpos->wz) + 1].type].name);
				if (p_ptr->party)
				for (i = 1; i <= NumPlayers; i++) {
					if (Players[i]->conn == NOT_CONNECTED) continue;
					if (i == Ind) continue;
					if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
					if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
					if (Players[i]->party == p_ptr->party)
						msg_format(i, "\374\377G[\377%c%s has left %s..\377G]", COLOUR_DUNGEON, p_ptr->name, d_name + d_info[iddc[ABS(wpos->wz) + 1].type].name);
				}
			}
		}
 #endif
#endif
		else if (wpos->wz == 1) {
			msg_format(Ind, "\377%cYou leave %s..", COLOUR_DUNGEON, get_dun_name(wpos->wx, wpos->wy, TRUE, wild_info[wpos->wy][wpos->wx].tower, 0, FALSE));
#ifdef RPG_SERVER /* stair scumming in non-IRON dungeons might create mad spam otherwise */
			if (p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->party == p_ptr->party)
					msg_format(i, "\374\377G[\377%c%s has left %s..\377G]", COLOUR_DUNGEON, p_ptr->name, get_dun_name(wpos->wx, wpos->wy, TRUE, wild_info[wpos->wy][wpos->wx].tower, 0, FALSE));
			}
#endif
		}
		else if (c_ptr->feat == FEAT_WAY_MORE) {
			msg_print(Ind, "You enter the next area.");
#ifdef RPG_SERVER /* stair scumming in non-IRON dungeons might create mad spam otherwise */
			if (p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->wpos.wz >= 0) continue; /* must be in same dungeon/tower */
				if (Players[i]->party == p_ptr->party) {
					if (Players[i]->depth_in_feet)
						msg_format(i, "\374\377G[\377%c%s took a staircase downwards to %dft..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz - 1) * 50);
					else
						msg_format(i, "\374\377G[\377%c%s took a staircase downwards to lv %d..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz - 1) * 50);
				}
			}
#else
			if (one_way && p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->wpos.wz >= 0) continue; /* must be in same dungeon/tower */
				if (Players[i]->party == p_ptr->party) {
					if (Players[i]->depth_in_feet)
						msg_format(i, "\374\377G[\377%c%s took a staircase downwards to %dft..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz - 1) * 50);
					else
						msg_format(i, "\374\377G[\377%c%s took a staircase downwards to lv %d..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz - 1) * 50);
				}
			}
#endif
		} else {
			msg_print(Ind, "You enter a maze of down staircases.");
#ifdef RPG_SERVER /* stair scumming in non-IRON dungeons might create mad spam otherwise */
			if (p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->wpos.wz >= 0) continue; /* must be in same dungeon/tower */
				if (Players[i]->party == p_ptr->party) {
					if (Players[i]->depth_in_feet)
						msg_format(i, "\374\377G[\377%c%s took a staircase downwards to %dft..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz - 1) * 50);
					else
						msg_format(i, "\374\377G[\377%c%s took a staircase downwards to lv %d..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz - 1) * 50);
				}
			}
#else
			if (one_way && p_ptr->party)
			for (i = 1; i <= NumPlayers; i++) {
			        if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (p_ptr->admin_dm && cfg.secret_dungeon_master && !is_admin(Players[i])) continue;
				if (Players[i]->wpos.wx != wpos->wx || Players[i]->wpos.wy != wpos->wy) continue;
				if (Players[i]->wpos.wz >= 0) continue; /* must be in same dungeon/tower */
				if (Players[i]->party == p_ptr->party) {
					if (Players[i]->depth_in_feet)
						msg_format(i, "\374\377G[\377%c%s took a staircase downwards to %dft..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz - 1) * 50);
					else
						msg_format(i, "\374\377G[\377%c%s took a staircase downwards to lv %d..\377G]", COLOUR_DUNGEON, p_ptr->name, (wpos->wz - 1) * 50);
				}
			}
#endif
		}
		p_ptr->new_level_method = LEVEL_DOWN;
#ifdef USE_SOUND_2010
		staircase_sfx(Ind);
#endif
	} else {
		if (p_ptr->safe_float_turns) {
			msg_print(Ind, "Floating attempt blocked by client option 'safe_float' (in '=1').");
			return;
		}

#ifdef RPG_SERVER /* This is an iron-server... Prob trav should not work - the_sandman */
		msg_print(Ind, "This harsh world knows not what you're trying to do.");
		forget_view(Ind); //the_sandman
		if (!is_admin(p_ptr)) return;
#endif

		if (surface || dungeon) {
			if (wild_info[wpos->wy][wpos->wx].dungeon->flags2 & (DF2_IRON
			    | (p_ptr->ghost ? DF2_NO_DEATH : 0))) { //hack: don't ghost into the training tower (might confuse newbies who return from barrow-downs)
				msg_print(Ind, "There is a magic barrier blocking your attempt.");
				if (!is_admin(p_ptr)) return;
			}
		} else {
			if (wild_info[wpos->wy][wpos->wx].tower->flags2 & DF2_IRON) {
				msg_print(Ind, "There is a magic barrier blocking your attempt.");
				if (!is_admin(p_ptr)) return;
			}
		}

		if (surface) {
			msg_format(Ind, "\377%cYou float into %s..", COLOUR_DUNGEON, get_dun_name(wpos->wx, wpos->wy, FALSE, wild_info[wpos->wy][wpos->wx].dungeon, 0, FALSE));
#ifdef GLOBAL_DUNGEON_KNOWLEDGE
			/* we now 'learned' the base level of this dungeon */
			if (!is_admin(p_ptr)) wild_info[wpos->wy][wpos->wx].dungeon->known |= 0x2;
#endif
		}
		else if (wpos->wz == 1) msg_format(Ind, "\377%cYou float out of %s..", COLOUR_DUNGEON, get_dun_name(wpos->wx, wpos->wy, TRUE, wild_info[wpos->wy][wpos->wx].tower, 0, FALSE));
#ifndef PROBTRAVEL_AVOIDS_OTHERS
		else msg_print(Ind, "You float downwards.");
#else
		else if (p_ptr->ghost) msg_print(Ind, "You float downwards.");
#endif
		if (p_ptr->ghost) p_ptr->new_level_method = LEVEL_GHOST;
#ifndef PROBTRAVEL_AVOIDS_OTHERS
		else p_ptr->new_level_method = LEVEL_PROB_TRAVEL;
#else
		else {
			/* Note 1: possible mini-exploit here: if the skipped floor has NO_MAGIC flag
			   and we're running with PROBTRAVEL_AVOIDS_OTHERS (disabled by default though)
			   we'd not get affected even though we're "crossing" that floor, in theory.

			   Note 2: if a whole dungeon is completely blocked by people and we try to
			   probtravel into it, we'll get the "You float into.." message followed by the
			   failure message, which is a bit ugly. :-p */

			int i, z_min;
			player_type *q_ptr;
			bool skip, arrived = FALSE;

			if (surface || dungeon) z_min = -wild_info[wpos->wy][wpos->wx].dungeon->maxdepth; //into a dungeon or further down the dungeon
			else z_min = 0; //from tower downwards until surface (redundant, actually)

			do {
				wpos->wz--;

				if (!wpos->wz) { /* always allow to float down to the surface, leaving a tower */
					arrived = TRUE;
					break;
				}

				skip = FALSE;
				for (i = 1; i <= NumPlayers; i++) {
					if (i == Ind) continue;
					q_ptr = Players[i];
					if (!inarea(&q_ptr->wpos, wpos)) continue;
					if (q_ptr->party && p_ptr->party && player_in_party(q_ptr->party, Ind)) {
						skip = FALSE;
						break;
					}
					if (q_ptr->ghost) continue; /* Allow rescuing someone via probtravel */
					skip = TRUE;
				}
				if (!skip) {
					arrived = TRUE;
					break;
				}
			} while (wpos->wz > z_min);

			z = wpos->wz; /* Remember new depth in case of successful probtravel */
			wpos->wz = old_wpos.wz; /* restore our depth for now, it will be set to z again when handling successful probtravel. */

			/* Unsuccessful probtravel */
			if (!arrived) {
				msg_print(Ind, "There is a magical discharge in the air as probability travel fails!");
				return;
			}

			p_ptr->new_level_method = LEVEL_PROB_TRAVEL;
			msg_print(Ind, "You float downwards.");
		}
#endif
	}

	if (c_ptr->custom_lua_way) exec_lua(0, format("custom_way(%d,%d)", Ind, c_ptr->custom_lua_way));

	/* Remove the player from the old location */
	c_ptr->m_idx = 0;

	/* Show everyone that's he left */
	everyone_lite_spot_move(Ind, wpos, p_ptr->py, p_ptr->px);

	/* Forget his lite and viewing area */
	forget_lite(Ind);
	forget_view(Ind);

	/* Hack -- take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	/* A player has left this depth */
#ifdef PROBTRAVEL_AVOIDS_OTHERS
	if (p_ptr->new_level_method != LEVEL_PROB_TRAVEL) {
#endif
		wpos->wz--;
#ifdef PROBTRAVEL_AVOIDS_OTHERS
	} else wpos->wz = z; /* Set depth to probtravel result, possibly having skipped some floors. */
#endif
	new_players_on_depth(&old_wpos, -1, TRUE);
	p_ptr->new_level_flag = TRUE;
	new_players_on_depth(wpos, 1, TRUE);

	forget_view(Ind); //the_sandman

	/* He'll be safe for 2 turns */
	set_invuln_short(Ind, STAIR_GOI_LENGTH);

	/* Create a way back */
	create_up_stair = TRUE; /* appearently unused */

	/* C. Blue -- Megahack to fix the player having enough energy left to perform another action after taking upstairs,
	   but insufficient energy to perform another action after taking downstairs, due to different new floor speeds!
	   Especially for going upwards this is required to fix the "you cannot see" bug, when the player has enough
	   energy to cast a spell BEFORE process_player_change_wpos() gets called, while when going downstairs,
	   in receive_activate_skill() the spellcast command will rebound once due to insufficient energy, resulting in
	   the spell to actually work (even though this could be considered exactly the wrong way around :D)..
	   So this hack in both cmd_go_up and cmd_go_down fixes two things:
	   1) them being treated inequally, since after up the player can still act while after down he can't
	   2) the "you cannot see" bug that results from '1)' when trying to cast right after going up. */
	if (p_ptr->energy >= level_speed(wpos)) p_ptr->energy = level_speed(wpos) - 1;
}



/*
 * Simple command to "search" for one turn
 */
void do_cmd_search(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

#if CHATTERBOX_LEVEL > 2	/* This can be noisy */
	if (!p_ptr->taciturn_messages)
		msg_print(Ind, "You carefully search things around you..");
#endif	// CHATTERBOX_LEVEL

	/* Repeat if requested */
	if (p_ptr->always_repeat) p_ptr->command_rep = PKT_SEARCH;

	/* Search */
	if (p_ptr->pclass == CLASS_ROGUE && !p_ptr->rogue_heavyarmor) detect_bounty(Ind);
	else search(Ind);
}


/*
 * Hack -- toggle search mode
 */
void do_cmd_toggle_search(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Stop searching */
	if (p_ptr->searching) {
		/* Clear the searching flag */
		p_ptr->searching = FALSE;

		/* Recalculate boni */
		p_ptr->update |= (PU_BONUS);

		/* Redraw stuff */
		p_ptr->redraw |= (PR_STATE | PR_MAP);
	}

	/* Start searching */
	else {
		/* S(he) is no longer afk */
		un_afk_idle(Ind);

		/* Set the searching flag */
		p_ptr->searching = TRUE;
		break_shadow_running(Ind);
		stop_precision(Ind);

		/* Update stuff */
		p_ptr->update |= (PU_BONUS);

		/* Redraw stuff */
		p_ptr->redraw |= (PR_STATE | PR_SPEED | PR_MAP);
	}
}



/*
 * Allocates objects upon opening a chest    -BEN-
 *
 * Disperse treasures from the chest "o_ptr", centered at (x,y).
 *
 * Small chests often contain "gold", while Large chests always contain
 * items.  Wooden chests contain 2 items, Iron chests contain 4 items,
 * and Steel chests contain 6 items.  The "value" of the items in a
 * chest is based on the "power" of the chest, which is in turn based
 * on the level on which the chest is generated.
 */
static void chest_death(int Ind, int y, int x, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;

	bool small;
	int number;
	long cash;

	if (!(zcave = getcave(wpos))) return;

	/* Must be a chest */
	if (o_ptr->tval != TV_CHEST) return;

	/* Small chests often hold "gold" */
	small = (o_ptr->sval < SV_CHEST_MIN_LARGE);

	/* Determine how much to drop (see above) */
	number = (o_ptr->sval % SV_CHEST_MIN_LARGE) * 2;

	/* Custom LUA hacks */
	if (o_ptr->xtra2) exec_lua(0, format("custom_chest_open(%d,%d,%d,%d)", Ind, o_ptr->xtra2, small, number));
	if (o_ptr->xtra4) number = o_ptr->xtra2 - 1;

	/* Generate some treasure */
	if (o_ptr->pval && (number > 0)) {
		/* let's not be scrooges =) - C. Blue */
		cash = (((o_ptr->level + 10) * (o_ptr->level + 10)) * 3) / number;
		if (o_ptr->xtra5) cash = o_ptr->xtra5; //(note btw: 'xtraN' are s16b)

		/* Opening a chest -- this hack makes sure we don't find a chest in a chest, even though yo like chests */
		if (!o_ptr->iron_turn) opening_chest = turn; //by now all existing chests should long have iron_turn set to something, so this check might not be needed anymore
		else opening_chest = o_ptr->iron_turn;

		/* Determine the "value" of the items */
		//object_level = ABS(o_ptr->pval) + 10;
		object_level = ABS(o_ptr->level) + 10;
		place_object_restrictor = RESF_NONE;

		/* Drop some objects (non-chests) */
		for (; number > 0; --number) {
			/* Small chests often drop gold */
			if (small && magik(75))
				place_gold(Ind, wpos, y, x, 10, (cash * (80 + rand_int(41))) / 100);
			else if (!small && magik(20))
				place_gold(Ind, wpos, y, x, 10, (cash * (80 + rand_int(41))) / 100);
			/* Otherwise drop an item */
			else
				/* mostly DROP_GOOD */
				place_object(Ind, wpos, y, x, magik(75) ? TRUE : FALSE, FALSE, FALSE, make_resf(p_ptr), default_obj_theme, 0, ITEM_REMOVAL_NORMAL, TRUE);
		}

		/* Reset the object level */
		object_level = getlevel(wpos);

		/* No longer opening a chest */
		opening_chest = FALSE;
	}

	/* Empty */
	o_ptr->pval = 0;
	o_ptr->ident |= ID_KNOWN | ID_NO_HIDDEN; /* obsolete-- easy to see it's empty and that's it */

	/* Known */
	object_known(o_ptr);

#ifdef SUBINVEN_CHESTS
	/* Convert opened and therefore now empty chests to usable containers */
	o_ptr->tval = TV_SUBINVEN;
	o_ptr->sval += SV_SI_CHEST_CONVERSION;
	/* o_ptr->sval is mapped identically! */
	o_ptr->k_idx = lookup_kind(o_ptr->tval, o_ptr->sval);
	/* Fill in bpval */
	o_ptr->bpval = k_info[o_ptr->k_idx].pval;

	/* For object_desc() to auto-add " (empty)" to an opened chest that has been converted to a bag-chest. (See 'empty-chest-hack'.) */
	o_ptr->xtra8 = 1; /* to allow quick check of 'empty'ness.. */
#endif
}


/*
 * Chests have traps too.
 *
 * Exploding chest destroys contents (and traps).
 * Note that the chest itself is never destroyed.
 */
static bool chest_trap(int Ind, int y, int x, int o_idx) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr = &o_list[o_idx];

	int trap;
	bool ident = FALSE;

	/* Only analyze chests */
	if (o_ptr->tval != TV_CHEST) return(FALSE);

	/* Ignore disarmed chests */
	if (o_ptr->pval <= 0) return(FALSE);

	/* Obtain the trap */
	trap = o_ptr->pval;

	/* Message */
	//msg_print(Ind, "You found a trap!");
	msg_print(Ind, "You triggered a trap!");

#ifdef USE_SOUND_2010
	sound(Ind, "trap_setoff", NULL, SFX_TYPE_MISC, FALSE);
#endif

	/* Custom LUA hacks */
	if (o_ptr->xtra1) exec_lua(0, format("custom_chest_trap(%d,%d)", Ind, o_ptr->xtra1));
	/* Skip normal trap routines? */
	if (o_ptr->xtra4 & 0x8) return(TRUE);

	/* Set off trap */
	ident = player_activate_trap_type(Ind, y, x, o_ptr, o_idx);

	if (ident && !p_ptr->trap_ident[trap]) {
		p_ptr->trap_ident[trap] = TRUE;
		msg_format(Ind, "You identified the trap as %s.", t_name + t_info[trap].name);
	}

	return(TRUE);
}


/*
 * Return the index of a house given an coordinate pair
 */
int pick_house(struct worldpos *wpos, int y, int x) {
	int i;

	/* Check each house */
	for (i = 0; i < num_houses; i++) {
		/* Check this one */
		if (houses[i].dx == x && houses[i].dy == y && inarea(&houses[i].wpos, wpos))
			/* Return */
			return(i);
	}

	/* Failure */
	return(-1);
}
/* Reverse function of pick_house() */
int pick_player(house_type *h_ptr) {
	int i;

	/* Check each house */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this one */
		if (Players[i]->px == h_ptr->dx && Players[i]->py == h_ptr->dy && inarea(&Players[i]->wpos, &h_ptr->wpos))
			/* Return */
			return(i);
	}

	/* Failure */
	return(0);
}

/* Test if a coordinate (player pos usually) is inside a building
   (or on its edge), in a simple (risky?) way - C. Blue */
bool inside_house(struct worldpos *wpos, int x, int y) {
	static cave_type *c_ptr, **zcave; //for efficiency

	if (wpos->wz) return(FALSE);

#if 0
	/* In case CAVE_ICKY overworld-structures will exist in the future.
	   Note that this will probably exclude all jails too. */
	if (!istownarea(wpos, MAX_TOWNAREA)) return(FALSE);
#endif

	zcave = getcave(wpos);

	/* This check was added so inside_house() can be used for player stores in delete_object_idx(),
	   otherwise segfault when objects are erased from non-allocated areas (cleanup routines).
	   So kill_house_contents() will no longer trigger PLAYER_STORE_REMOVED log entries. :|
	   (It seems too inefficient to allocate+generate+deallocate the floor for each item, need a better solution..)
	   --- correction, kill_house_contents() WILL allocate the map and call delete_object->delete_object_idx which will trigger PLAYER_STORE_REMOVED. */
	if (!zcave) return(FALSE);

	c_ptr = &zcave[y][x];
	/* assume all houses are on the world surface (and vaults aren't) */
	if ((c_ptr->info & CAVE_ICKY) &&
	    c_ptr->feat != FEAT_DEEP_WATER && c_ptr->feat != FEAT_DRAWBRIDGE && c_ptr->feat != FEAT_DRAWBRIDGE_HORIZ) /* moat and drawbridge aren't "inside" the house! */
		return(TRUE);

	return(FALSE);
}
/* Another version, for efficiency */
static bool inside_house_simple(cave_type *c_ptr) {
	/* assume all houses are on the world surface (and vaults aren't) */
	if ((c_ptr->info & CAVE_ICKY) &&
	    c_ptr->feat != FEAT_DEEP_WATER && c_ptr->feat != FEAT_DRAWBRIDGE && c_ptr->feat != FEAT_DRAWBRIDGE_HORIZ) /* moat and drawbridge aren't "inside" the house! */
		return(TRUE);

	return(FALSE);
}
/* like inside_house() but more costly since it returns the actual house index + 1,
   or 0 for 'not inside any house'.
   Note: Does not test unowned houses, unlike inside_house().
   NOTE: currently only checks HF_RECT houses, not polygons (jails). */
int inside_which_house(struct worldpos *wpos, int x, int y) {
	int i;
	house_type *h_ptr;

	for (i = 0; i < num_houses; i++) {
		h_ptr = &houses[i];

		/* skip unowned houses */
		if (!h_ptr->dna->owner) continue;

		if (!inarea(&h_ptr->wpos, wpos)) continue;

		if (h_ptr->flags & HF_RECT) {
			if (h_ptr->x <= x && h_ptr->x + h_ptr->coords.rect.width - 1 >= x &&
			    h_ptr->y <= y && h_ptr->y + h_ptr->coords.rect.height - 1 >= y)
				return(i + 1);
#if 0 /* not needed atm */
		} else {
			/* anyone want to implement non-rectangular houses :-p? */
#endif
		}
	}

	return(-1);
}

bool inside_inn(player_type *p_ptr, cave_type *c_ptr) {
	int shop = -1;

	if ((c_ptr)->feat == FEAT_SHOP) shop = GetCS(c_ptr, CS_SHOP)->sc.omni;
	if (((f_info[(c_ptr)->feat].flags1 & FF1_PROTECTED) &&
	    (istown(&(p_ptr)->wpos) || isdungeontown(&(p_ptr)->wpos))) ||
	    shop == STORE_HOME || shop == STORE_HOME_DUN || (p_ptr)->store_num == STORE_INN || (p_ptr)->store_num == STORE_DUNGEON_INN)
		return(TRUE);
	return(FALSE);
}

/* Door change permissions
 * args example: ynn1 -> Party, Class, Race, MinLevel
 */
static bool chmod_door(int Ind, struct dna_type *dna, char *args) {
	player_type *p_ptr = Players[Ind];

	if (!is_admin(p_ptr)) {
		if (dna->creator != p_ptr->dna) {
			/* note: the house 'master' (aka creator = first buyer/builder) is not necessarily the house owner! -- wrong!:
			   It is always the current owner, except for guild halls, where the owner is the guild and this is the current-guild-master's dna. */
			msg_print(Ind, "Only the house's master may change permissions.");
			return(FALSE);
		}
		/* more security needed... */
	}

	if (dna->a_flags & ACF_PARTY) {
		if (!p_ptr->party) {
			msg_print(Ind, "You are not in a party.");
			return(FALSE);
		}
		if (dna->owner_type != OT_PLAYER) {
			msg_print(Ind, "Only houses owned by the party owner can be given party access.");
			return(FALSE);
		}
		if (strcmp(parties[p_ptr->party].owner, lookup_player_name(dna->owner))) {
			//msg_print(Ind, "You must be owner of your party to allow party access.");
			msg_print(Ind, "Only houses owned by the party owner can be given party access.");
			return(FALSE);
		}
	}

	if ((dna->a_flags = args[1])) dna->min_level = atoi(&args[2]);

	return(TRUE);
}

/* Door change ownership */
static bool chown_door(int Ind, struct dna_type *dna, char *args, int x, int y) {
	player_type *p_ptr = Players[Ind], *q_ptr = p_ptr; /* q_ptr set to p_ptr just to slay compiler warning */
	int newowner = -1;
	int i, h_idx;
	int acc_houses = acc_get_houses(p_ptr->accountname), acc_houses2;
	bool loaded = FALSE, admin = admin_p(Ind);

	/* to prevent house amount cheeze (houses_owned)
	   let's just say house ownership can't be transferred.
	   Changing the door access should be sufficient. */
	if (!is_admin(p_ptr)) {
		if (dna->creator != p_ptr->dna) return(FALSE);
		/* maybe make guild master only chown guild -> "guild hall" */
	}
	switch (args[1]) {
	case '1':
		h_idx = pick_house(&p_ptr->wpos, y, x);
		/* paranoia */
		if (h_idx == -1) {
			msg_print(Ind, "There is no transferrable house there.");
			return(FALSE);
		}

		/* Check house limit of target player! */
		i = name_lookup(Ind, &args[2], FALSE, FALSE, TRUE);
		if (!i) {
			/* Trying to add another of our own characters maybe? */
			int j, *id_list, ids, slot;
			struct account acc;
			hash_entry *ptr;
			object_type *o_ptr;
			s32b id;

			if (!GetAccount(&acc, p_ptr->accountname, NULL, FALSE, NULL, NULL)) { /* paranoia */
				/* uhh.. */
				msg_print(Ind, "Your account is corrupted, please contact an admin.");
				return(FALSE);
			}

			if (admin) {
				ids = 0; //not using id_list checks, instead direct access to all existing characters
				if (!(id = lookup_player_id(&args[2]))) {
					msg_print(Ind, "(Admin access) - Character does not exist.");
					return(FALSE);
				}
				msg_format(Ind, "(Admin access) - Transferring house to %s (%s)", lookup_player_name(id), lookup_accountname(id));
			} else {
				ids = player_id_list(&id_list, acc.id);
				for (j = 0; j < ids; j++)
					if (!strcmp(lookup_player_name(id_list[j]), &args[2])) break;
				if (j == ids) {
					msg_print(Ind, "Character not online, nor found in your list of characters.");
					if (ids) C_KILL(id_list, ids, int);
					return(FALSE);
				}
				id = id_list[j];
			}

			/* hack - actually load that character (same account as us) */
			loaded = TRUE;
			NumPlayers++;
			MAKE(Players[NumPlayers], player_type);
			q_ptr = Players[NumPlayers];
			q_ptr->inventory = C_NEW(INVEN_TOTAL, object_type);
			for (slot = 0; slot < NUM_HASH_ENTRIES; slot++) {
				ptr = hash_table[slot];
				while (ptr) {
					if (ptr->id != id) {
						/* advance to next character */
						ptr = ptr->next;
						continue;
					}

					/* clear his data */
					o_ptr = q_ptr->inventory;
					WIPE(q_ptr, player_type);
					q_ptr->inventory = o_ptr;
					q_ptr->Ind = NumPlayers;
					C_WIPE(q_ptr->inventory, INVEN_TOTAL, object_type);
					/* set his supposed name */
					strcpy(q_ptr->name, ptr->name);
					/* generate savefile name */
					process_player_name(NumPlayers, TRUE);
					/* try to load him! */
					if (!load_player(NumPlayers)) {
						/* bad fail */
						s_printf("HOUSE_CHOWN_SELF: %d, '%s' load_player('%s') failed\n", h_idx, p_ptr->name, &args[2]);
						/* unhack */
						C_FREE(q_ptr->inventory, INVEN_TOTAL, object_type);
						KILL(q_ptr, player_type);
						NumPlayers--;
						msg_print(Ind, "House transfer failed.");
						if (ids) C_KILL(id_list, ids, int);
						return(FALSE);
					}

					/* break */
					slot = NUM_HASH_ENTRIES;
					i = NumPlayers;
					break;
				}
			}
			if (ids) C_KILL(id_list, ids, int);
			if (!i) { /* Paranoia */
				C_FREE(q_ptr->inventory, INVEN_TOTAL, object_type);
				KILL(q_ptr, player_type);
				NumPlayers--;
				msg_print(Ind, "House transfer failed.");
				s_printf("HOUSE_CHOWN_SELF: FAILED. source %s, house %d, dest %s.\n", p_ptr->name, pick_house(&p_ptr->wpos, y, x), &args[2]);
				return(FALSE);
			}
			/* Log */
			s_printf("HOUSE_CHOWN_SELF: source %s, house %d, dest %s.\n", p_ptr->name, pick_house(&p_ptr->wpos, y, x), &args[2]);
		}

		if (compat_pmode(Ind, i, TRUE)) {
			msg_format(Ind, "You cannot transfer houses to %s players!", compat_pmode(Ind, i, TRUE));
			/* unhack */
			if (loaded) {
				C_FREE(q_ptr->inventory, INVEN_TOTAL, object_type);
				KILL(q_ptr, player_type);
				NumPlayers--;
			}
			return(FALSE);
		}
		if (Players[i]->inval) {
			msg_print(Ind, "You cannot transfer houses to players that aren't validated!");
			/* unhack */
			if (loaded) {
				C_FREE(q_ptr->inventory, INVEN_TOTAL, object_type);
				KILL(q_ptr, player_type);
				NumPlayers--;
			}
			return(FALSE);
		}
		if ((Players[i]->mode & MODE_PVP) && Players[i]->houses_owned >= 1) {
			msg_print(Ind, "PvP characters may not own more than one house!");
			/* unhack */
			if (loaded) {
				C_FREE(q_ptr->inventory, INVEN_TOTAL, object_type);
				KILL(q_ptr, player_type);
				NumPlayers--;
			}
			return(FALSE);
		}
		if (Players[i]->mode & MODE_DED_IDDC) {
			msg_print(Ind, "IDDC-exclusive characters may not own houses!");
			/* unhack */
			if (loaded) {
				C_FREE(q_ptr->inventory, INVEN_TOTAL, object_type);
				KILL(q_ptr, player_type);
				NumPlayers--;
			}
			return(FALSE);
		}
		if (cfg.houses_per_player && (Players[i]->houses_owned >= ((Players[i]->lev > 50 ? 50 : Players[i]->lev) / cfg.houses_per_player)) && !is_admin(Players[i])) {
			if ((int)(Players[i]->lev / cfg.houses_per_player) == 0)
				msg_format(Ind, "That player needs to be at least level %d to own a house!", cfg.houses_per_player);
			else
				msg_print(Ind, "At his current level, that player cannot own more houses!");
			/* unhack */
			if (loaded) {
				C_FREE(q_ptr->inventory, INVEN_TOTAL, object_type);
				KILL(q_ptr, player_type);
				NumPlayers--;
			}
			return(FALSE);
		}

		//ACC_HOUSE_LIMIT
		if (!loaded && /* actually not if we transfer to ourselves.. */
		    acc_get_houses(Players[i]->accountname) >= cfg.acc_house_limit) {
			msg_print(Ind, "That player cannot own more houses on his account!");
			return(FALSE);
		}

		if (houses[h_idx].flags & HF_MOAT) {
			if (cfg.castles_for_kings && !Players[i]->total_winner) {
				msg_print(Ind, "That player is neither king nor queen, neither emperor nor empress!");
				/* unhack */
				if (loaded) {
					C_FREE(q_ptr->inventory, INVEN_TOTAL, object_type);
					KILL(q_ptr, player_type);
					NumPlayers--;
				}
				return(FALSE);
			}
			if (cfg.castles_per_player && (Players[i]->castles_owned >= cfg.castles_per_player))  {
				if (cfg.castles_per_player == 1)
					msg_format(Ind, "That player already owns a castle!");
				else
					msg_format(Ind, "That player already owns %d castles!", cfg.castles_per_player);
				/* unhack */
				if (loaded) {
					C_FREE(q_ptr->inventory, INVEN_TOTAL, object_type);
					KILL(q_ptr, player_type);
					NumPlayers--;
				}
				return(FALSE);
			}
		}

		/* Finally change the owner */
		newowner = lookup_player_id_messy(&args[2]);
		if (!newowner) newowner = -1;
		else {
			/* Remove old owner */
			if (dna->owner_type == OT_PLAYER) {
				p_ptr->houses_owned--;
				if (houses[h_idx].flags & HF_MOAT) p_ptr->castles_owned--;

				//ACC_HOUSE_LIMIT:
				if (!loaded) { /* actually not if we transfer to ourselves.. */
					acc_houses--;
					acc_set_houses(p_ptr->accountname, acc_houses);
				}

				clockin(Ind, 8);
			} else if (dna->owner_type == OT_GUILD) {
				guilds[dna->owner].h_idx = 0;
				Send_guild_config(dna->owner);
			}
		}
		break;
#ifdef ENABLE_GUILD_HALL
	case '2':
		h_idx = pick_house(&p_ptr->wpos, y, x);
		/* paranoia */
		if (h_idx == -1) {
			msg_print(Ind, "There is no transferrable house there.");
			return(FALSE);
		}
		/* only the guild master can assign an OT_GUILD house */
		if (!p_ptr->guild || p_ptr->id != guilds[p_ptr->guild].master) {
			msg_print(Ind, "You must be the guild master to give house ownership to a guild.");
			return(FALSE);
		}
		/* there can only be one guild hall */
		if (guilds[p_ptr->guild].h_idx) {
			msg_print(Ind, "The guild already has a guild hall.");
			return(FALSE);
		}

		/* guild halls must be mang-style houses (technically too, for CAVE2_GUILD_SUS flag to work!) */
		if ((houses[h_idx].flags & HF_TRAD)) {
			msg_print(Ind, "\377yGuild halls must not be list-type (store-like) houses.");
			return(FALSE);
		}

		/* guild master loses one own house (side note: stays imprinted as 'creator' though), gets transferred to the guild as 'guild hall' */
		if (dna->owner_type == OT_PLAYER) {
			p_ptr->houses_owned--;
			if (houses[h_idx].flags & HF_MOAT) p_ptr->castles_owned--;

			//ACC_HOUSE_LIMIT
			acc_houses--;
			acc_set_houses(p_ptr->accountname, acc_houses);

			clockin(Ind, 8);
		}
		newowner = p_ptr->guild;
		msg_format(Ind, "This house is now owned by %s.", guilds[p_ptr->guild].name);
		break;
#endif
/*	case '3':
		newowner = party_lookup(&args[2]);
		break;
	case '4':
		for (i = 0; i < MAX_CLASS; i++) {
			if (!strcmp(&args[2], class_info[i].title))
				newowner = i;
		}
		break;
	case '5':
		for (i = 0; i < MAX_RACE; i++) {
			if (!strcmp(&args[2], race_info[i].title))
				newowner = i;
		}
		break; */
	default:
		msg_print(Ind, "\377ySorry, this feature is not available");
		if (!is_admin(p_ptr)) return(FALSE);
	}

	if (newowner != -1) {
		/* 1: OT_PLAYER */
		if (args[1] == '1') {
			player_type *p2_ptr;
#ifdef USE_MANG_HOUSE
			house_type *h_ptr = &houses[h_idx];
			int sy = 0, sx = 0, ey = 0, ex = 0; //frigging compiler warnings ('uninitialized')

			if (h_ptr->flags & HF_RECT) {
				sy = h_ptr->y + 1;
				sx = h_ptr->x + 1;
				ey = h_ptr->y + h_ptr->coords.rect.height - 1;
				ex = h_ptr->x + h_ptr->coords.rect.width - 1;
			}
#endif
			for (i = 1; i <= NumPlayers; i++) { /* in game? maybe long winded */
				p2_ptr = Players[i];
				if (p2_ptr->id == newowner) {
					//if (dna->owner_type == OT_PLAYER) p_ptr->houses_owned--;
					p2_ptr->houses_owned++;
					if (houses[h_idx].flags & HF_MOAT) {
						//if (dna->owner_type == OT_PLAYER) p_ptr->castles_owned--;
						p2_ptr->castles_owned++;
					}

					//ACC_HOUSE_LIMIT:
					if (!loaded) { /* actually not if we tried to transfer to ourselves.. */
						acc_houses2 = acc_get_houses(p2_ptr->accountname);
						acc_houses2++;
						acc_set_houses(p2_ptr->accountname, acc_houses2);
					}

					clockin(i, 8);

					dna->creator = p2_ptr->dna;
					dna->owner = newowner;
					dna->owner_type = args[1] - '0';
					dna->a_flags = ACF_NONE;

					/* unhack */
					if (loaded) {
						/* finally write him back after transfer succeeded */
						save_player(i);

						C_FREE(q_ptr->inventory, INVEN_TOTAL, object_type);
						KILL(q_ptr, player_type);
						NumPlayers--;
					}

					return(TRUE);
				}
#ifdef USE_MANG_HOUSE
				else {
					/* Teleport the former owner out of the house if he's still inside,
					   so he doesn't get stuck. */
					if (h_ptr->flags & HF_RECT) {
						if (inarea(&p2_ptr->wpos, &h_ptr->wpos) &&
						    p2_ptr->py >= sy && p2_ptr->py <= ey &&
						    p2_ptr->px >= sx && p2_ptr->px <= ex)
							teleport_player_force(i, 1);
					} else {
						//fill_house(h_ptr, FILL_CLEAR, NULL);
						/* Polygonal house */

						//todo: teleport any players inside out */
					}
				}
#endif
			}
			//hard paranoia: undo the house transfer!
			p_ptr->houses_owned++;
			if (houses[h_idx].flags & HF_MOAT) p_ptr->castles_owned++;

			//ACC_HOUSE_LIMIT:
			if (!loaded) { /* actually not if we tried to transfer to ourselves.. */
				acc_houses++;
				acc_set_houses(p_ptr->accountname, acc_houses);
			}

			clockin(Ind, 8);

			/* unhack */
			if (loaded) {
				C_FREE(q_ptr->inventory, INVEN_TOTAL, object_type);
				KILL(q_ptr, player_type);
				NumPlayers--;
			}
			return(FALSE);
		}

		/* 2: OT_GUILD */
		if (args[1] == '2') {
			dna->owner = newowner;
			dna->owner_type = args[1] - '0';
			if (dna->owner_type == OT_GUILD) {
				guilds[p_ptr->guild].h_idx = h_idx + 1;
				Send_guild_config(p_ptr->guild);
			}
			return(TRUE);
		}
	}

	/* unhack if needed */
	if (loaded) {
		C_FREE(q_ptr->inventory, INVEN_TOTAL, object_type);
		KILL(q_ptr, player_type);
		NumPlayers--;
	}
	return(FALSE);
}

/* Returns > 0 if access is granted,
   and <= 0 if access is denied.
   Note == FALSE -> no message will be printed. - C. Blue */
bool access_door(int Ind, struct dna_type *dna, bool note) {
	player_type *p_ptr = Players[Ind];

	if (!dna->owner) return(FALSE); /* house doesn't belong to anybody */

	/*if (is_admin(p_ptr))
		return(TRUE); - moved to allow more overview for admins when looking at
				house door colours on the world surface - C. Blue */

	/* house belongs to someone of incompatible mode!
	   in that case, no further checking is required,
	   since won't ever be able to access it.
	   (Added for PLAYER_STORES especially.)
	   This check was previously only in access_door_colour() but was necessary here too,
	   because of new way to access houses of own characters account-wide. */
	if (p_ptr->mode & MODE_PVP) {
		if (!(dna->mode & MODE_PVP)) return(FALSE);
	} else if (p_ptr->mode & MODE_EVERLASTING) {
		if (!(dna->mode & MODE_EVERLASTING)) return(FALSE);
	} else if (dna->mode & MODE_PVP) return(FALSE);
	else if (dna->mode & MODE_EVERLASTING) return(FALSE);

	/* Soloist can ONLY access his own houses */
	if (p_ptr->mode & MODE_SOLO) {
		if (dna->owner_type == OT_PLAYER && p_ptr->id == dna->owner && p_ptr->dna == dna->creator) return(TRUE);
		return(FALSE);
	}

	/* Test for cumulative restrictions */
	if (p_ptr->dna != dna->creator) {
		if ((dna->a_flags & ACF_LEVEL) && p_ptr->max_plv < dna->min_level) {
			if (note) msg_print(Ind, "Your level doesn't match the house restriction yet.");
			return(FALSE); /* defies logic a bit, but for speed */
		}
		if ((dna->a_flags & ACF_CLASS) && (p_ptr->pclass != (dna->creator & 0xff))) {
			if (note) msg_print(Ind, "Your class doesn't match the house restriction.");
			return(FALSE);
		}
		if ((dna->a_flags & ACF_RACE) && (p_ptr->prace != ((dna->creator >> 8) & 0xff))) {
			if (note) msg_print(Ind, "Your race doesn't match the house restriction.");
			return(FALSE);
		}

		/* Allow ACF_WINNER and ACF_FALLEN_WINNER to actually stack positively */
		if ((dna->a_flags & ACF_WINNER) && (dna->a_flags & ACF_FALLENWINNER) && !p_ptr->once_winner) {
			if (note) msg_print(Ind, "You must be (fallen) royalty to match the house restriction.");
			return(FALSE);
		}
		else if ((dna->a_flags & ACF_WINNER) && !p_ptr->total_winner) {
			if (note) msg_print(Ind, "You must be royalty to match the house restriction.");
			return(FALSE);
		}
		else if ((dna->a_flags & ACF_FALLENWINNER) && (p_ptr->total_winner || !p_ptr->once_winner)) {
			if (note) msg_print(Ind, "You must be fallen royalty to match the house restriction.");
			return(FALSE);
		}

		if ((dna->a_flags & ACF_NOGHOST) && !(p_ptr->mode & MODE_NO_GHOST)) {
			/* extra message in case ACF_WINNER is also specified, just for fanciness */
			if (dna->a_flags & ACF_WINNER) {
				if (note) msg_print(Ind, "You must be emperor or empress to match the house restriction.");
			} else {
				if (note) msg_print(Ind, "You must be unworldly to match the house restriction.");
			}
			return(FALSE);
		}
	}

	switch (dna->owner_type) {
	case OT_PLAYER:
		/* new doors in new server different */
		if (p_ptr->id == dna->owner && p_ptr->dna == dna->creator) return(TRUE);
		/* allow access to houses owned by other characters of your own account as if they were yours (as long as mode is compatible) */
		if (p_ptr->account == lookup_player_account(dna->owner)) return(TRUE);
		/* party access? */
		if (dna->a_flags & ACF_PARTY) {
			if (!p_ptr->party) return(FALSE);
			if (!strcmp(parties[p_ptr->party].owner, lookup_player_name(dna->owner)))
				return(TRUE);
		}
		return(FALSE);
	case OT_PARTY:
		if (!p_ptr->party) return(FALSE);
		if (player_in_party(dna->owner, Ind)) return(TRUE);
		return(FALSE);
	case OT_CLASS:
		if (p_ptr->pclass == dna->owner) return(TRUE);
		return(FALSE);
	case OT_RACE:
		if (p_ptr->prace == dna->owner) return(TRUE);
		return(FALSE);
	case OT_GUILD:
		if (!p_ptr->guild) return(FALSE);
		if (p_ptr->guild == dna->owner) return(TRUE);
		return(FALSE);
	}
	return(FALSE);
}

/* Note: these colours will be affected by nightly darkening, so shades
   shouldn't be used but completely different colours only..
   but since we don't have enough colours, it doesn't matter - C. Blue */
#if 0 /* colour is already used for party-access -> uw/hellish only */
 #define DOOR_COLOUR_INCOMPATIBLE TERM_L_DARK
#else
 #define DOOR_COLOUR_INCOMPATIBLE TERM_SLATE
#endif
int access_door_colour(int Ind, struct dna_type *dna) {
	player_type *p_ptr = Players[Ind];

	/* paranoia: for when the poly-house door bug occurred */
	if (!dna) return(TERM_ACID);

	/* house doesn't belong to anybody? */
	if (!dna->owner) return(TERM_UMBER);

	/* house belongs to someone of incompatible mode!
	   in that case, no further checking is required,
	   since won't ever be able to access it.
	   (Added for PLAYER_STORES especially.) */
	if (p_ptr->mode & MODE_PVP) {
		if (!(dna->mode & MODE_PVP)) return(DOOR_COLOUR_INCOMPATIBLE);
	} else if (p_ptr->mode & MODE_EVERLASTING) {
		if (!(dna->mode & MODE_EVERLASTING)) return(DOOR_COLOUR_INCOMPATIBLE);
	} else if (dna->mode & MODE_PVP) return(DOOR_COLOUR_INCOMPATIBLE);
	else if (dna->mode & MODE_EVERLASTING) return(DOOR_COLOUR_INCOMPATIBLE);

	/* Soloist can ONLY access his own houses */
	if (p_ptr->mode & MODE_SOLO) {
		if (dna->owner_type == OT_PLAYER && p_ptr->id == dna->owner && p_ptr->dna == dna->creator) return(TERM_L_GREEN);
		return(DOOR_COLOUR_INCOMPATIBLE);
	}

	/* test house access permissions */
	switch (dna->owner_type) {
	case OT_PLAYER:
		/* new doors in new server different */
		if (p_ptr->id == dna->owner && p_ptr->dna == dna->creator) return(TERM_L_GREEN);
		/* allow access to houses owned by other characters of your own account as if they were yours (as long as mode is compatible) */
		if (p_ptr->account == lookup_player_account(dna->owner)) return(TERM_GREEN);
		/* party access with possible restrictions */
		if (dna->a_flags & ACF_PARTY) {
			if (!p_ptr->party) return(TERM_SLATE);
			/* exploit fix: create party houses, then promote someone else to leader */
			if (!strcmp(parties[p_ptr->party].owner, lookup_player_name(dna->owner))) {
				if (dna->a_flags & ACF_CLASS) {
					if (p_ptr->pclass == (dna->creator & 0xff)) {
						if (dna->a_flags & ACF_LEVEL && p_ptr->max_plv < dna->min_level && p_ptr->dna != dna->creator) return(TERM_YELLOW);
						return(TERM_WHITE);
					} else return(TERM_ORANGE);
				}
				if (dna->a_flags & ACF_RACE) {
					if (p_ptr->prace == ((dna->creator >> 8) & 0xff)) {
						if ((dna->a_flags & ACF_LEVEL) && p_ptr->max_plv < dna->min_level && p_ptr->dna != dna->creator) return(TERM_YELLOW);
						return(TERM_WHITE);
					} else return(TERM_ORANGE);
				}

				/* ACF_WINNER and ACF_FALLENWINNER can be used together */
				if ((dna->a_flags & ACF_WINNER) && (dna->a_flags & ACF_FALLENWINNER)) {
					if (p_ptr->once_winner) {
						if ((dna->a_flags & ACF_LEVEL) && p_ptr->max_plv < dna->min_level && p_ptr->dna != dna->creator) return(TERM_YELLOW);
						return(TERM_L_RED);
					} else return(TERM_RED);
				}
				else if (dna->a_flags & ACF_WINNER) {
					if (p_ptr->total_winner) {
						if ((dna->a_flags & ACF_LEVEL) && p_ptr->max_plv < dna->min_level && p_ptr->dna != dna->creator) return(TERM_YELLOW);
						return(TERM_L_RED);
					} else return(TERM_RED);
				}
				else if (dna->a_flags & ACF_FALLENWINNER) {
					if (!p_ptr->total_winner && p_ptr->once_winner) {
						if ((dna->a_flags & ACF_LEVEL) && p_ptr->max_plv < dna->min_level && p_ptr->dna != dna->creator) return(TERM_YELLOW);
						return(TERM_L_RED);
					} else return(TERM_RED);
				}

				if (dna->a_flags & ACF_NOGHOST) {
					if (p_ptr->mode & MODE_NO_GHOST) {
						if ((dna->a_flags & ACF_LEVEL) && p_ptr->max_plv < dna->min_level && p_ptr->dna != dna->creator) return(TERM_YELLOW);
					}
					return(TERM_L_DARK);
				}
				if ((dna->a_flags & ACF_LEVEL) && p_ptr->max_plv < dna->min_level && p_ptr->dna != dna->creator) return(TERM_YELLOW);
				return(TERM_L_BLUE);
			}
		}
		break;
	case OT_PARTY:
		if (!p_ptr->party) return(TERM_SLATE);
		if (player_in_party(dna->owner, Ind)) {
			if ((dna->a_flags & ACF_LEVEL) && p_ptr->max_plv < dna->min_level && p_ptr->dna != dna->creator) return(TERM_YELLOW);
			return(TERM_L_BLUE);
		}
		break;
	case OT_CLASS:
		if (p_ptr->pclass == dna->owner) {
			if ((dna->a_flags & ACF_LEVEL) && p_ptr->max_plv < dna->min_level && p_ptr->dna != dna->creator) return(TERM_YELLOW);
			return(TERM_WHITE);
		}
		break;
	case OT_RACE:
		if (p_ptr->pclass == dna->owner) {
			if ((dna->a_flags & ACF_LEVEL) && p_ptr->max_plv < dna->min_level && p_ptr->dna != dna->creator) return(TERM_YELLOW);
			return(TERM_WHITE);
		}
		break;
	case OT_GUILD:
		if (!p_ptr->guild) return(TERM_SLATE);
		if (p_ptr->guild == dna->owner) {
			if ((dna->a_flags & ACF_LEVEL) && p_ptr->max_plv < dna->min_level && p_ptr->dna != dna->creator) return(TERM_YELLOW);
			return(TERM_VIOLET);
		}
		break;
	}

	/* we have no permission to access */
	return(TERM_SLATE);
	//if (dna->a_flags & ACF_STORE) return(TERM_MULTI); /* older idea. Instead, see PLAYER_STORE - C. Blue */
}

cptr get_house_owner(struct c_special *cs_ptr) {
	static char string[80];
	struct dna_type *dna = cs_ptr->sc.ptr;

	strcpy(string, "nobody.");
	if (dna->owner) {
		//char *name;
		cptr name;

		switch (dna->owner_type) {
		case OT_PLAYER:
			if ((name = lookup_player_name(dna->owner)))
			strcpy(string, name);
			break;
		case OT_PARTY:
			if (strlen(parties[dna->owner].name))
			strcpy(string, parties[dna->owner].name);
			break;
		case OT_CLASS:
			strcpy(string, class_info[dna->owner].title);
			strcat(string, "s");
			break;
		case OT_RACE:
			strcpy(string, race_info[dna->owner].title);
			strcat(string, "s");
			break;
		case OT_GUILD:
			strcpy(string, "the guild '");
			strcat(string, guilds[dna->owner].name);
			strcat(string, "'");
			break;
		}
	}
	return(string);
}

/*
 * Open a closed door or closed chest.
 *
 * Note unlocking a locked door/chest is worth one experience point.
 */
void do_cmd_open(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave, *c_ptr;

	int y, x, i, j;
	int flag;

	object_type *o_ptr;

	bool more = FALSE;
	bool cannot_spectral = CANNOT_OPERATE_SPECTRAL;
	bool cannot_form = CANNOT_OPERATE_FORM;

	if (!(zcave = getcave(wpos))) return;

#ifdef ENABLE_OUNLIFE
	/* Wraithstep gets auto-cancelled on forced interaction with solid environment */
	if (p_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1)) set_tim_wraith(Ind, 0);
#endif

	/* Ghosts cannot open doors/chests; not in WRAITHFORM either, not in no OPEN_DOOR monster forms either for doors */
	if ((cannot_spectral && !is_admin(p_ptr)) || cannot_form) {
	    //&& !strchr("thpkng", r_info[p_ptr->body_monster].d_char))) {
		struct c_special *cs_ptr;

		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];
		c_ptr = &zcave[y][x];

		/* Allow entering player houses (own houses, pstores, other houses we have access to) no matter in which no-OPEN_DOOR-form/wraith/ghoststate we are */
		if (c_ptr->feat != FEAT_HOME || !(cs_ptr = GetCS(c_ptr, CS_DNADOOR)) || !access_door(Ind, cs_ptr->sc.ptr, TRUE)) {
			if (cannot_spectral) msg_print(Ind, "Without a material body you cannot open things!");
			else { //ie cannot_form
				msg_print(Ind, "In your current form you cannot open things!");
				if (!p_ptr->warning_bash
				    /* Can never bash open inaccessible player house doors */
				    && (c_ptr->feat != FEAT_HOME || !cs_ptr)) {
					if (p_ptr->rogue_like_commands)
						msg_print(Ind, "\374\377yHINT: You can press '\377of\377y' to try to force (bash) a door open.");
					else
						msg_print(Ind, "\374\377yHINT: You can press '\377oB\377y' to try to bash a door open.");
					s_printf("warning_bash: %s\n", p_ptr->name);
					p_ptr->warning_bash = 1;
				}
			}
			return;
		}
	}

	/* Get a "repeated" direction */
	if (dir) {
		/* Get requested location */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];
		/* Get requested grid */
		c_ptr = &zcave[y][x];
		/* Get the object (if any) */
		o_ptr = &o_list[c_ptr->o_idx];

		/* hack for fluff */
		if (c_ptr->feat == FEAT_SEALED_DOOR)
			msg_print(Ind, "That door cannot be opened.");

		else if (c_ptr->feat == FEAT_WINDOW) {
			if (!inside_house(wpos, p_ptr->px, p_ptr->py) && !inside_inn(p_ptr, &zcave[p_ptr->py][p_ptr->px])) {
				msg_print(Ind, "You cannot open that window from outside.");
				return;
			}
			cave_set_feat_live(wpos, y, x, FEAT_OPEN_WINDOW);

			un_afk_idle(Ind);
			break_cloaking(Ind, 3);
			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);

			/* Take half a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

#ifdef USE_SOUND_2010
			//sound(Ind, "open_window", "open_chest", SFX_TYPE_COMMAND, TRUE);
			sound_near_site(y, x, wpos, 0, "open_window", "open_chest", SFX_TYPE_COMMAND, FALSE);
#endif
		}
		else if (c_ptr->feat == FEAT_WINDOW_SMALL) {
			if (!inside_house(wpos, p_ptr->px, p_ptr->py) && !inside_inn(p_ptr, &zcave[p_ptr->py][p_ptr->px])) {
				msg_print(Ind, "You cannot open that window from outside.");
				return;
			}
			cave_set_feat_live(wpos, y, x, FEAT_OPEN_WINDOW_SMALL);

			un_afk_idle(Ind);
			break_cloaking(Ind, 3);
			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);

			/* Take half a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

#ifdef USE_SOUND_2010
			//sound(Ind, "open_window", "open_chest", SFX_TYPE_COMMAND, TRUE);
			sound_near_site(y, x, wpos, 0, "open_window", "open_chest", SFX_TYPE_COMMAND, FALSE);
#endif
		}
		else if (c_ptr->feat == FEAT_BARRED_WINDOW) {
			if (!inside_house(wpos, p_ptr->px, p_ptr->py) && !inside_inn(p_ptr, &zcave[p_ptr->py][p_ptr->px])) {
				msg_print(Ind, "You cannot open that window from outside.");
				return;
			}
			cave_set_feat_live(wpos, y, x, FEAT_WINDOW); /* Open shutters ^^ */

			un_afk_idle(Ind);
			break_cloaking(Ind, 3);
			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);

			/* Take half a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

#ifdef USE_SOUND_2010
			//sound(Ind, "open_window", "open_chest", SFX_TYPE_COMMAND, TRUE);
			sound_near_site(y, x, wpos, 0, "open_window", "open_chest", SFX_TYPE_COMMAND, FALSE);
#endif
		}
		else if (c_ptr->feat == FEAT_BARRED_WINDOW_SMALL) {
			if (!inside_house(wpos, p_ptr->px, p_ptr->py) && !inside_inn(p_ptr, &zcave[p_ptr->py][p_ptr->px])) {
				msg_print(Ind, "You cannot open that window from outside.");
				return;
			}
			cave_set_feat_live(wpos, y, x, FEAT_WINDOW_SMALL); /* Open shutters ^^ */

			un_afk_idle(Ind);
			break_cloaking(Ind, 3);
			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);

			/* Take half a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

#ifdef USE_SOUND_2010
			//sound(Ind, "open_window", "open_chest", SFX_TYPE_COMMAND, TRUE);
			sound_near_site(y, x, wpos, 0, "open_window", "open_chest", SFX_TYPE_COMMAND, FALSE);
#endif
		}

		/* Nothing useful */
		else if (!((c_ptr->feat >= FEAT_DOOR_HEAD) &&
		      (c_ptr->feat <= FEAT_DOOR_TAIL)) &&
		    !((c_ptr->feat == FEAT_HOME)) &&
		    (o_ptr->tval != TV_CHEST))
		{
#if 1 /* just for fun */
			/* fun exception: open door mimic players */
			if ((i = -c_ptr->m_idx) > 0) {
				player_type *q_ptr = Players[i];

				if (q_ptr->body_monster == RI_DOOR_MIMIC && !(q_ptr->temp_misc_1 & 0x01)) {
					q_ptr->temp_misc_1 |= 0x01; /* 'open' :-p */
					msg_print(i, "That tickles!");
					note_spot(i, q_ptr->py, q_ptr->px);
					everyone_lite_spot(&q_ptr->wpos, q_ptr->py, q_ptr->px);

					/* S(he) is no longer afk */
					un_afk_idle(Ind);

					break_cloaking(Ind, 3);
					break_shadow_running(Ind);
					stop_precision(Ind);
					stop_shooting_till_kill(Ind);

					/* Take half a turn */
					p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

#ifdef USE_SOUND_2010
					sound(Ind, "open_door", NULL, SFX_TYPE_COMMAND, TRUE);
#endif

					return;
				}
			}
#endif
			/* Message */
			msg_print(Ind, "You see nothing there to open.");
		}

		/* Monster in the way */
		else if (c_ptr->m_idx > 0) {
			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);
			/* Message */
			msg_print(Ind, "There is a monster in the way!");
			/* Attack */
			py_attack(Ind, y, x, TRUE);
		}

		/* Open a closed chest. */
		else if (o_ptr->tval == TV_CHEST) {
			/* S(he) is no longer afk */
			un_afk_idle(Ind);
			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);
			/* Assume opened successfully */
			flag = TRUE;
			/* Attempt to unlock it */
			if (o_ptr->pval > 0) {
				trap_kind *t_ptr = &t_info[o_ptr->pval];

				/* Assume locked, and thus not open */
				flag = FALSE;
				/* Get the "disarm" factor */
				i = p_ptr->skill_dis;
				/* Penalize some conditions */
				if (p_ptr->blind || no_lite(Ind)) i = i / 10;
				if (p_ptr->confused || p_ptr->image) i = i / 10;
				/* Extract the difficulty */
				j = i - t_ptr->difficulty;
				// j = i - o_ptr->pval;
				/* Always have a small chance of success */
				if (j < 2) j = 2;
				/* Success -- May still have traps */
				if (rand_int(100) < j) {
					msg_print(Ind, "You have picked the lock.");
					flag = TRUE;
#ifdef USE_SOUND_2010
					sound(Ind, "open_pick", NULL, SFX_TYPE_COMMAND, TRUE);
#endif
					/* opening it uses only 1x trdifficulty instead of 3x. (Very maybe TODO: SHOW_XP_GAIN) */
					if (!(p_ptr->mode & MODE_PVP)) gain_exp(Ind, TRAP_EXP(p_ptr, o_ptr->pval, getlevel(&p_ptr->wpos)) / 3);
				}

				/* Failure -- Keep trying */
				else {
					/* We may continue repeating */
					more = TRUE;
					msg_print(Ind, "You failed to pick the lock.");
				}
			}
			/* can't open a non-closed chest */
			else if (o_ptr->pval == 0) flag = FALSE;

			/* Allowed to open */
			if (flag) {
				bool trp;

#ifdef USE_SOUND_2010
				sound(Ind, "open_chest", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
				if (o_ptr->custom_lua_usage) exec_lua(0, format("custom_object_usage(%d,%d,%d,%d,%d)", Ind, c_ptr->o_idx, -1, 10, o_ptr->custom_lua_usage));

				/* Apply chest traps, if any */
				trp = chest_trap(Ind, y, x, c_ptr->o_idx);

				break_cloaking(Ind, 3);
				break_shadow_running(Ind);
				stop_precision(Ind);
				stop_shooting_till_kill(Ind);

				/* Some traps might destroy the chest on setting off */
				if (o_ptr->sval != SV_CHEST_RUINED) {
					/* Let the Chest drop items */
					chest_death(Ind, y, x, o_ptr);
					if (o_ptr->xtra3 & 0x4) { /* Special option (custom lua): Erase chest on successful opening? */
						delete_object_idx(c_ptr->o_idx, FALSE, FALSE);
						trp = FALSE; /* Don't try to delete an already deleted object! */
					}
				}
				if (trp) {
					if ((o_ptr->xtra3 & 0x1) || /* Special option (custom lua): Erase chest whenever the trap was set off? */
					    (o_ptr->sval == SV_CHEST_RUINED && (o_ptr->xtra3 & 0x2))) /* Special option (custom lua): Erase the chest if it got ruined by the trap? */
						delete_object_idx(c_ptr->o_idx, FALSE, FALSE);
				}
				/* Redraw chest for custom mapping users as it changed from closed to opened chest: */
				everyone_lite_spot(wpos, y, x);
			}
		}

		/* Jammed door */
		else if (c_ptr->feat >= FEAT_DOOR_HEAD + 0x08) {
			/* S(he) is no longer afk */
			un_afk_idle(Ind);
			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);
			/* Stuck */
			msg_print(Ind, "The door appears to be stuck.");

			if (!p_ptr->warning_bash) {
				if (p_ptr->rogue_like_commands)
					msg_print(Ind, "\374\377yHINT: You can press '\377of\377y' to try to force (bash) a door open.");
				else
					msg_print(Ind, "\374\377yHINT: You can press '\377oB\377y' to try to bash a door open.");
				s_printf("warning_bash: %s\n", p_ptr->name);
				p_ptr->warning_bash = 1;
			}

#ifdef USE_SOUND_2010
			sound(Ind, "open_door_stuck", NULL, SFX_TYPE_COMMAND, TRUE);
#endif
		}

		/* Locked door */
		else if (c_ptr->feat >= FEAT_DOOR_HEAD + 0x01) {
			/* S(he) is no longer afk */
			un_afk_idle(Ind);
			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);
			/* Disarm factor */
			i = p_ptr->skill_dis;
			/* Penalize some conditions */
			if (p_ptr->blind || no_lite(Ind)) i = i / 10;
			if (p_ptr->confused || p_ptr->image) i = i / 10;
			/* Extract the lock power */
			j = c_ptr->feat - FEAT_DOOR_HEAD;
			/* Extract the difficulty XXX XXX XXX */
			j = i - (j * 4);
			/* Always have a small chance of success */
			if (j < 2) j = 2;
			/* Success */
			if (rand_int(100) < j) {
				/* Message */
				msg_print(Ind, "You have picked the lock.");
#ifdef USE_SOUND_2010
				sound(Ind, "open_pick", NULL, SFX_TYPE_COMMAND, TRUE);
#endif
				break_cloaking(Ind, 3);
				break_shadow_running(Ind);
				stop_precision(Ind);
				stop_shooting_till_kill(Ind);

				/* Set off trap */
				if (GetCS(c_ptr, CS_TRAPS)) player_activate_door_trap(Ind, y, x);

				/* Automatic bot detection - mikaelh */
				if (!(c_ptr->info2 & CAVE2_MAGELOCK)) {
					p_ptr->silly_door_exp++;
					if (p_ptr->silly_door_exp >= 100) {
						msg_print(Ind, "Botting never pays off...");
						take_xp_hit(Ind, 1, "botting", TRUE, FALSE, TRUE, 0);
					} else {
						/* Experience */
						if (!(p_ptr->mode & MODE_PVP)) gain_exp(Ind, 1);
					}
				}

				/* Open the door */
				c_ptr->feat = FEAT_OPEN;
				/* Notice */
				note_spot_depth(wpos, y, x);
				/* Redraw */
				everyone_lite_spot(wpos, y, x);
				/* Update some things */
				p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
			}

			/* Failure */
			else {
				/* Message */
				msg_print(Ind, "You failed to pick the lock.");

				/* We may keep trying */
				more = TRUE;
			}
		}

		/* Home */
		else if (c_ptr->feat == FEAT_HOME) {
			struct c_special *cs_ptr;

			if ((cs_ptr = GetCS(c_ptr, CS_DNADOOR))) { /* orig house failure */
				cave_type *c2_ptr = &zcave[p_ptr->py][p_ptr->px];

				/* We can access this house or are an admin */
				if (access_door(Ind, cs_ptr->sc.ptr, TRUE) || is_admin(p_ptr)) {
#if USE_MANG_HOUSE_ONLY || TRUE /* let'em open it, so that thevery can take place :) */
					/* Open the door */
					c_ptr->feat = FEAT_HOME_OPEN;
 #ifdef USE_SOUND_2010
					sound(Ind, "open_door", NULL, SFX_TYPE_COMMAND, TRUE);
 #endif
#else	/* USE_MANG_HOUSE */
					msg_print(Ind, "Just walk in.");
#endif	/* USE_MANG_HOUSE */
					/* S(he) is no longer afk */
					un_afk_idle(Ind);
					break_cloaking(Ind, 3);
					break_shadow_running(Ind);
					stop_precision(Ind);
					stop_shooting_till_kill(Ind);

					/* Take half a turn */
					p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;
					/* Notice */
					note_spot_depth(wpos, y, x);
					/* Redraw */
					everyone_lite_spot(wpos, y, x);
					/* Update some things */
					p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);

				/* We cannot access this house. Special hack: Never get stuck inside a house that we don't have access to!
				   Make sure we ignore drawbridge and moat, since these are outside of the house yet cave-icky.
				   Exception: The jail. */
				} else if ((c2_ptr->info & CAVE_ICKY) && c2_ptr->feat != FEAT_DRAWBRIDGE && c2_ptr->feat != FEAT_DRAWBRIDGE_HORIZ && c2_ptr->feat != FEAT_DEEP_WATER) {
					if ((c_ptr->info & CAVE_JAIL) && p_ptr->tim_jail) msg_format(Ind, "Abide your jail sentence, lasting %d more seconds..", p_ptr->tim_jail);
					else teleport_player_force(Ind, 1);

				/* We cannot access this house */
				} else {
					struct dna_type *dna = cs_ptr->sc.ptr;

					if (!strcmp(get_house_owner(cs_ptr), "nobody.")) {
						msg_format(Ind, "\377oThat house costs %d gold.", house_price_player(dna->price, p_ptr->stat_ind[A_CHR]));
#ifdef USE_SOUND_2010
						//sound(Ind, "open_door_stuck", NULL, SFX_TYPE_COMMAND, TRUE);
#endif
					} else {
#ifdef PLAYER_STORES
						/* We don't have house access, but if it's set up as
						   a player-run store, we may enter it to buy things - C. Blue */
						disturb(Ind, 1, 0);
						if (!do_cmd_player_store(Ind, x, y)) {
#endif
							msg_format(Ind, "\377sThat house is owned by %s.",get_house_owner(cs_ptr));
#ifdef USE_SOUND_2010
							sound(Ind, "open_door_stuck", NULL, SFX_TYPE_COMMAND, TRUE);
#endif
						}
					}
				}
				return;
			}

			if ((cs_ptr = GetCS(c_ptr, CS_KEYDOOR))) { /* currently not used in the game */
				struct key_type *key = cs_ptr->sc.ptr;

				for (j = 0; j < INVEN_PACK; j++) {
					object_type *o_ptr = &p_ptr->inventory[j];
					if (o_ptr->tval == TV_KEY && o_ptr->sval == SV_HOUSE_KEY && o_ptr->pval == key->id) {
						c_ptr->feat = FEAT_HOME_OPEN;
						/* S(he) is no longer afk */
						un_afk_idle(Ind);
						break_cloaking(Ind, 3);
						break_shadow_running(Ind);
						stop_precision(Ind);
						stop_shooting_till_kill(Ind);
						p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;
						note_spot_depth(wpos, y, x);
						everyone_lite_spot(wpos, y, x);
						p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
						msg_format(Ind, "\377gThe key fits in the lock. %d:%d",key->id, o_ptr->pval);
#ifdef USE_SOUND_2010
						sound(Ind, "open_pick", NULL, SFX_TYPE_COMMAND, TRUE);
#endif
						return;
					} else if (is_admin(p_ptr)) {
						c_ptr->feat = FEAT_HOME_OPEN;
						/* S(he) is no longer afk */
						un_afk_idle(Ind);
						break_cloaking(Ind, 3);
						break_shadow_running(Ind);
						stop_precision(Ind);
						stop_shooting_till_kill(Ind);
						p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;
						note_spot_depth(wpos, y, x);
						everyone_lite_spot(wpos, y, x);
						p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
						msg_format(Ind, "\377gThe door crashes open. %d",key->id);
#ifdef USE_SOUND_2010
						sound(Ind, "bash_door_break", NULL, SFX_TYPE_COMMAND, TRUE);
#endif
						return;
					}
				}
				msg_print(Ind, "\377rYou need a key to open this door.");
#ifdef USE_SOUND_2010
				sound(Ind, "open_door_stuck", NULL, SFX_TYPE_COMMAND, TRUE);
#endif
			}
		}

		/* Closed door */
		else {
			/* S(he) is no longer afk */
			un_afk_idle(Ind);

			break_cloaking(Ind, 3);
			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);

			/* Take half a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

			if (!(p_ptr->confused || p_ptr->blind)
			    || magik(33)) {
				/* Set off trap */
				if (GetCS(c_ptr, CS_TRAPS)) player_activate_door_trap(Ind, y, x);

				/* Open the door */
				c_ptr->feat = FEAT_OPEN;
#ifdef USE_SOUND_2010
				sound(Ind, "open_door", NULL, SFX_TYPE_COMMAND, TRUE);
#endif
				/* Notice */
				note_spot_depth(wpos, y, x);
				/* Redraw */
				everyone_lite_spot(wpos, y, x);
				/* Update some things */
				p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
			} else {
				/* Message */
				msg_print(Ind, "You fumble..");

				/* We may keep trying */
				more = TRUE;
			}
		}
	}

	/* Cancel repeat unless we may continue */
	if (!more) disturb(Ind, 0, 0);
	else if (p_ptr->always_repeat) p_ptr->command_rep = PKT_OPEN;
}


/*
 * Close an open door.
 */
void do_cmd_close(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;

	int                     y, x;
	cave_type               *c_ptr;

	bool more = FALSE;
	bool cannot_spectral = CANNOT_OPERATE_SPECTRAL;
	bool cannot_form = CANNOT_OPERATE_FORM;

	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return;

#ifdef ENABLE_OUNLIFE
	/* Wraithstep gets auto-cancelled on forced interaction with solid environment */
	if (p_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1)) set_tim_wraith(Ind, 0);
#endif

	/* Ghosts cannot close ; not in WRAITHFORM */
	if ((cannot_spectral && !is_admin(p_ptr)) || cannot_form) {
		msg_print(Ind, "You cannot close things!");
		return;
	}

	/* Get a "repeated" direction */
	if (dir) {
		/* Get requested location */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];

		/* Get grid and contents */
		c_ptr = &zcave[y][x];

		/* Broken door */
		if (c_ptr->feat == FEAT_BROKEN) {
			/* Message */
			msg_print(Ind, "The door appears to be broken.");
		}

		/* hack for fluff */
		else if (c_ptr->feat == FEAT_UNSEALED_DOOR)
			msg_print(Ind, "That door cannot be closed.");

		else if (c_ptr->feat == FEAT_OPEN_WINDOW) {
#if 1 /* xD */
			if (!inside_house(wpos, p_ptr->px, p_ptr->py) && !inside_inn(p_ptr, &zcave[p_ptr->py][p_ptr->px])) {
				msg_print(Ind, "You cannot close that window from outside.");
				return;
			}
#endif
			cave_set_feat_live(wpos, y, x, FEAT_WINDOW);

			un_afk_idle(Ind);
			break_cloaking(Ind, 3);
			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);

			/* Take half a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

#ifdef USE_SOUND_2010
			//sound(Ind, "close_window", "close_door", SFX_TYPE_COMMAND, TRUE);
			sound_near_site(y, x, wpos, 0, "close_window", "close_door", SFX_TYPE_COMMAND, FALSE);
#endif
		}
		else if (c_ptr->feat == FEAT_OPEN_WINDOW_SMALL) {
#if 1 /* xD */
			if (!inside_house(wpos, p_ptr->px, p_ptr->py) && !inside_inn(p_ptr, &zcave[p_ptr->py][p_ptr->px])) {
				msg_print(Ind, "You cannot close that window from outside.");
				return;
			}
#endif
			cave_set_feat_live(wpos, y, x, FEAT_WINDOW_SMALL);

			un_afk_idle(Ind);
			break_cloaking(Ind, 3);
			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);

			/* Take half a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

#ifdef USE_SOUND_2010
			//sound(Ind, "close_window", "close_door", SFX_TYPE_COMMAND, TRUE);
			sound_near_site(y, x, wpos, 0, "close_window", "close_door", SFX_TYPE_COMMAND, FALSE);
#endif
		}

		else if (c_ptr->feat == FEAT_WINDOW) {
#if 1 /* xD */
			if (!inside_house(wpos, p_ptr->px, p_ptr->py) && !inside_inn(p_ptr, &zcave[p_ptr->py][p_ptr->px])) {
				msg_print(Ind, "You cannot close that window from outside.");
				return;
			}
#endif
			cave_set_feat_live(wpos, y, x, FEAT_BARRED_WINDOW); /* closed the shutters ^^ */

			un_afk_idle(Ind);
			break_cloaking(Ind, 3);
			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);

			/* Take half a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

#ifdef USE_SOUND_2010
			//sound(Ind, "close_window", "close_door", SFX_TYPE_COMMAND, TRUE);
			sound_near_site(y, x, wpos, 0, "close_window", "close_door", SFX_TYPE_COMMAND, FALSE);
#endif
		}
		else if (c_ptr->feat == FEAT_WINDOW_SMALL) {
#if 1 /* xD */
			if (!inside_house(wpos, p_ptr->px, p_ptr->py) && !inside_inn(p_ptr, &zcave[p_ptr->py][p_ptr->px])) {
				msg_print(Ind, "You cannot close that window from outside.");
				return;
			}
#endif
			cave_set_feat_live(wpos, y, x, FEAT_BARRED_WINDOW_SMALL); /* closed the shutters ^^ */

			un_afk_idle(Ind);
			break_cloaking(Ind, 3);
			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);

			/* Take half a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

#ifdef USE_SOUND_2010
			//sound(Ind, "close_window", "close_door", SFX_TYPE_COMMAND, TRUE);
			sound_near_site(y, x, wpos, 0, "close_window", "close_door", SFX_TYPE_COMMAND, FALSE);
#endif
		}

		/* Require open door */
		else if (c_ptr->feat != FEAT_OPEN && c_ptr->feat != FEAT_HOME_OPEN) {
#if 1 /* just for fun */
			/* fun exception: open door mimic players */
			int i;

			if ((i = -c_ptr->m_idx) > 0) {
				player_type *q_ptr = Players[i];
				if (q_ptr->body_monster == RI_DOOR_MIMIC && (q_ptr->temp_misc_1 & 0x01)) {
					q_ptr->temp_misc_1 &= ~0x01; /* 'close' :-p */
					msg_print(i, "That tickles!");
					note_spot(i, q_ptr->py, q_ptr->px);
					everyone_lite_spot(&q_ptr->wpos, q_ptr->py, q_ptr->px);

					/* S(he) is no longer afk */
					un_afk_idle(Ind);

					break_cloaking(Ind, 2);
					break_shadow_running(Ind);
					stop_precision(Ind);
					stop_shooting_till_kill(Ind);
					/* Take a turn */
					p_ptr->energy -= level_speed(&p_ptr->wpos);
#ifdef USE_SOUND_2010
					sound(Ind, "close_door", NULL, SFX_TYPE_COMMAND, TRUE);
#endif

					return;
				}
			}
#endif

			/* Message */
			msg_print(Ind, "You see nothing there to close.");
		}

		/* Monster in the way */
		else if (c_ptr->m_idx > 0) {
			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			/* Message */
			msg_print(Ind, "There is a monster in the way!");

			/* Attack */
			py_attack(Ind, y, x, TRUE);
		}

		/* Object in the way */
		else if (c_ptr->o_idx > 0) {
			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			/* Message */
			msg_print(Ind, "There is an object blocking the doorway!");
		}

		/* House door, close it */
		else if (c_ptr->feat == FEAT_HOME_OPEN) {
			/* S(he) is no longer afk */
			un_afk_idle(Ind);

			break_cloaking(Ind, 2);
			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);

			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			/* Close the door */
			c_ptr->feat = FEAT_HOME;
#ifdef USE_SOUND_2010
			sound(Ind, "close_door", NULL, SFX_TYPE_COMMAND, TRUE);
#endif

			/* Notice */
			note_spot_depth(wpos, y, x);

			/* Redraw */
			everyone_lite_spot(wpos, y, x);

			/* Update some things */
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
		}

		/* Close the door */
		else {
			/* S(he) is no longer afk */
			un_afk_idle(Ind);
			break_cloaking(Ind, 2);
			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);

#ifdef USE_SOUND_2010
			sound(Ind, "close_door", NULL, SFX_TYPE_COMMAND, TRUE);
#endif

			/* Set off trap */
			if (GetCS(c_ptr, CS_TRAPS)) player_activate_door_trap(Ind, y, x);

			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			/* Close the door */
			c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

			/* Notice */
			note_spot_depth(wpos, y, x);

			/* Redraw */
			everyone_lite_spot(wpos, y, x);

			/* Update some things */
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
		}
	}

	/* Cancel repeat unless we may continue */
	if (!more) disturb(Ind, 0, 0);
}


/*
 * Check the terrain around the location to see whether erosion from surrounding lava or water would take place if the feature was removed.
 * TODO: expand this for more generic terrain types		- Jir -
 */
u16b twall_erosion(worldpos *wpos, int y, int x, u16b feat) {
	int tx, ty, d;
	cave_type **zcave;
	cave_type *c_ptr;

	if (!(zcave = getcave(wpos))) return(FALSE);

	for (d = 1; d <= 9; d++) {
		if (d == 5) continue;

		tx = x + ddx[d];
		ty = y + ddy[d];

		if (!in_bounds(ty, tx)) continue;

		c_ptr = &zcave[ty][tx];
		if (feat_is_deep_water(c_ptr->feat)) {
			//feat = FEAT_DEEP_WATER; /* <- this is only if FEAT_DEEP_WATER is also terraformable in turn (see cave_set_feat_live) */
			feat = FEAT_SHAL_WATER; /* <- this should be used otherwise */
			break;
		} else if (feat_is_deep_lava(c_ptr->feat)) {
			//feat = FEAT_DEEP_LAVA; /* <- this is only if FEAT_DEEP_LAVA is also terraformable in turn (see cave_set_feat_live) */
			feat = FEAT_SHAL_LAVA; /* <- this should be used otherwise */
			break;
		}
	}

	/* Result */
	return(feat);
}

/*
 * Tunnel through wall.  Assumes valid location.
 *
 * Note that it is impossible to "extend" rooms past their
 * outer walls (which are actually part of the room).
 *
 * This will, however, produce grids which are NOT illuminated
 * (or darkened) along with the rest of the room.
 */
bool twall(int Ind, struct worldpos *wpos, int y, int x, u16b feat) {
	byte *w_ptr = Ind ? &Players[Ind]->cave_flag[y][x] : NULL;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Paranoia -- Require a wall or door or some such */
	if (cave_floor_bold(zcave, y, x)) return(FALSE);

	/* Remove the feature */
	cave_set_feat_live(wpos, y, x, twall_erosion(wpos, y, x, feat));

	/* Forget the "field mark" */
	if (Ind) *w_ptr &= ~CAVE_MARK;

	/* Result */
	return(TRUE);
}


/* --- Tunneling routines --- */

/* Reveal a secret door if tunnelling triggered a trap on it? */
#define TRAP_REVEALS_DOOR
/* Chance to find a rune when having proficiency in runes */
#define RUNE_CHANCE 1000
/* Actually give special message to indicate when we have zero chance to tunnel through a specific material */
#define INDICATE_IMPOSSIBLE "You cannot seem to make a dent in the"
/* Two methods of capping Digging skill vs floor depth.
   The 'complicated' method works with the same threshold levels as for determining the minimum depth for a character to gain full XP from monsters,
   thereby distributing the Digging skill much smoother over the range of dungeon levels with a 'natural' power curve.
   The simple method just caps Digging skill to floor depth, which means at dungeon level 50 and deeper the full skill is always applied.
   The 'complicated' method is therefore a depth-buff for Digging skill < 40 and a depth-nerf for Digging skill > 40. (The overall gains on optimum depths are very similar.) */
#define DIG_SKILL_VS_DEPTH_COMPLICATED

/* Just terraform a feat and potentially produce treasure.
 * Ind: Can be 0.
 * quiet_full: No messages/sound effects, for detonation-based excavation!
 * */
void do_cmd_tunnel_aux(int Ind, struct worldpos *wpos, int x, int y, int power, int wood_power, int fibre_power, bool quiet_borer, bool quiet_full, bool *no_quake, bool *more, bool *door) {
	player_type *p_ptr = Ind ? Players[Ind] : NULL;
	object_type *o_ptr = &p_ptr->inventory[INVEN_TOOL];
#ifdef ENABLE_DEMOLITIONIST
 #ifdef DEMOLITIONIST_IDDC_ONLY
	bool in_iddc = in_irondeepdive(wpos);
 #endif
#endif
	bool no_specials = in_sector00(wpos);

	int cfeat;
	u32b cinfo, cinfo2;
	int skill_dig = (!Ind || quiet_borer) ? 0 : get_skill(p_ptr, SKILL_DIG), mining = skill_dig;
	int dual_power = wood_power > fibre_power ? wood_power : fibre_power;
	int dug_feat = FEAT_NONE, tval = 0, sval = 0, special_k_idx = 0; //chest / golem base material / rune
	struct dun_level *l_ptr = getfloor(wpos);

	int old_object_level = object_level;
	int find_level = getlevel(wpos), find_level_base = find_level;

	int rune_proficiency = 0;

	feature_type *f_ptr;
	cave_type **zcave, *c_ptr;

	object_type forge;
	char o_name[ONAME_LEN];


	/* Init */
	*no_quake = *more = *door = FALSE;

	/* Extract grid */
	zcave = getcave(wpos);
	c_ptr = &zcave[y][x];
	cfeat = c_ptr->feat;
	cinfo = c_ptr->info;
	cinfo2 = c_ptr->info2;
	f_ptr = &f_info[cfeat];


	/* find highest rune skill to determine our rune-proficiency */
	if (Ind && !quiet_borer) {
		if (p_ptr->s_info[SKILL_R_LITE].value > rune_proficiency) rune_proficiency = p_ptr->s_info[SKILL_R_LITE].value;
		if (p_ptr->s_info[SKILL_R_DARK].value > rune_proficiency) rune_proficiency = p_ptr->s_info[SKILL_R_DARK].value;
		if (p_ptr->s_info[SKILL_R_NEXU].value > rune_proficiency) rune_proficiency = p_ptr->s_info[SKILL_R_NEXU].value;
		if (p_ptr->s_info[SKILL_R_NETH].value > rune_proficiency) rune_proficiency = p_ptr->s_info[SKILL_R_NETH].value;
		if (p_ptr->s_info[SKILL_R_CHAO].value > rune_proficiency) rune_proficiency = p_ptr->s_info[SKILL_R_CHAO].value;
		if (p_ptr->s_info[SKILL_R_MANA].value > rune_proficiency) rune_proficiency = p_ptr->s_info[SKILL_R_MANA].value;
		rune_proficiency /= 1000;
		if (rune_proficiency >= 5) rune_proficiency = rune_proficiency + 10;
		else if (rune_proficiency) rune_proficiency = (rune_proficiency + 2) * (rune_proficiency + 2) / 3;
	}

	/* find_level = floor level (0..n), mining = digging skill (0..50) or 0 for non-manual labour */
#ifdef DIG_SKILL_VS_DEPTH_COMPLICATED
	if (find_level < det_req_level(mining) || find_level < 20) mining = find_level <= 20 ? find_level : det_req_level_inverse(find_level);
#else
	if (find_level < mining) mining = find_level;
#endif
	find_level += mining / 3;
	/* Above was +find_level*2 and +mining/3, but on BD-1750 you find up to 6k-piles -
	   these lesser-boosted values seem actually to result in ~ the same profits as long as you don't chill out on low floors compared to your skills a lot. */

	/* Discover special features or objects when mining.
	   Note: Not in monster KILL_WALL form or via magic; not on world surface: wpos->wz == 0 ! */
	if (l_ptr && !quiet_borer && !no_specials) {
		/* prepare to discover a special feature */
		if ((rand_int(5000) <= mining + 5) && can_go_up(wpos, 0x1)) dug_feat = FEAT_WAY_LESS;
		else if ((rand_int(5000) <= mining + 5) && can_go_down(wpos, 0x1)) dug_feat = FEAT_WAY_MORE;
		else if ((rand_int(3000) <= mining + 10) && !(l_ptr->flags1 & LF1_NO_WATER)) dug_feat = FEAT_FOUNTAIN;
		else if (rand_int(500) < ((l_ptr->flags1 & LF1_NO_LAVA) ? 0 : ((l_ptr->flags1 & LF1_LAVA) ? 50 : 3))) dug_feat = FEAT_SHAL_LAVA;
		else if (rand_int(500) < ((l_ptr->flags1 & LF1_NO_WATER) ? 0 : ((l_ptr->flags1 & LF1_WATER) ? 50 : 8))) dug_feat = FEAT_SHAL_WATER;

		/* prepare to find a special object */
		else if (rand_int(2000) <= mining) {
			tval = TV_CHEST;
#if 1 /* maybe todo: use get_obj_num_prep_tval() instead of this hardcoding chances.. */
			sval = rand_int(mining) * rand_int(50);
			/* 1,2,3, 5,6,7 - according to k_info.txt */
			if (sval > 1600) sval = SV_CHEST_LARGE_STEEL;
			else if (sval > 1100) sval = SV_CHEST_LARGE_IRON;
			else if (sval > 700) sval = SV_CHEST_LARGE_WOODEN;
			else if (sval > 300) sval = SV_CHEST_SMALL_STEEL;
			else if (sval > 100) sval = SV_CHEST_SMALL_IRON;
			else sval = SV_CHEST_SMALL_WOODEN;
#endif
			special_k_idx = lookup_kind(tval, sval);

		} else if (rand_int(1000) < (mining + 2) / 2) {
		    if (!rand_int(5)) {
			tval = TV_GOLEM;
#if 1 /* maybe todo: use get_obj_num_prep_tval() instead of this hardcoding chances.. */
			sval = rand_int(mining) * rand_int(40);
			/* 0..7 - according to k_info.txt */
			if (sval >= 1900) sval = SV_GOLEM_ADAM;
			else if (sval >= 1800) sval = SV_GOLEM_MITHRIL;
			else if (sval >= 1600) sval = SV_GOLEM_GOLD;
			else if (sval >= 1300) sval = SV_GOLEM_SILVER;
			else if (sval >= 900) sval = SV_GOLEM_ALUM;
			else if (sval >= 500) sval = SV_GOLEM_IRON;
			else sval = SV_GOLEM_COPPER;
			//else sval = SV_GOLEM_WOOD; -- no wood from mining stone, reserve for tree-hacking
#endif
			special_k_idx = lookup_kind(tval, sval);
		    }
		    else {
			tval = TV_RUNE;
			get_obj_num_hook = NULL;
			get_obj_num_prep_tval(tval, RESF_MASK_MID);
			special_k_idx = get_obj_num(10 + getlevel(wpos), RESF_MASK_MID);
			if (!special_k_idx) tval = 0;
		    }
		}

		/* hack 1/2: rune proficiency adds to chance for 'special feature' uncovering giving a rune, if nothing else is uncovered */
		if (dug_feat == FEAT_NONE && !tval && rand_int(RUNE_CHANCE) < rune_proficiency && Ind && !p_ptr->IDDC_logscum) {
			tval = TV_RUNE;
			get_obj_num_hook = NULL;
			get_obj_num_prep_tval(tval, RESF_MASK_MID);
			special_k_idx = get_obj_num(10 + getlevel(wpos), RESF_MASK_MID);
			if (!special_k_idx) tval = 0;
		}
	}
	/* if in monster KILL_WALL form or via magic */
	else if (l_ptr && quiet_borer && !no_specials) {
		/* prepare to discover a special feature */
		if (rand_int(500) < ((l_ptr->flags1 & LF1_NO_LAVA) ? 0 : ((l_ptr->flags1 & LF1_LAVA) ? 50 : 3))) dug_feat = FEAT_SHAL_LAVA;
		else if (rand_int(500) < ((l_ptr->flags1 & LF1_NO_WATER) ? 0 : ((l_ptr->flags1 & LF1_WATER) ? 50 : 8))) dug_feat = FEAT_SHAL_WATER;
	}
	/* Never discover special bonus features or objects on stale floors, just allow basic lava/water etc tile uncovering */
	if (Ind && p_ptr->IDDC_logscum) {
		if (dug_feat == FEAT_FOUNTAIN) dug_feat = FEAT_NONE;
		special_k_idx = tval = 0;
	}

	/* Ok, we may finally tunnel.. */
	if (Ind && p_ptr->taciturn_messages) suppress_message = TRUE;

	/* Rubble */
	if (cfeat == FEAT_RUBBLE) {
		*no_quake = TRUE;

		/* Remove the rubble */
		if (power > rand_int(200) && twall(Ind, wpos, y, x, FEAT_FLOOR)) {
			/* Message */
			if (Ind && !quiet_full) {
				msg_print(Ind, "You have removed the rubble.");
#ifdef USE_SOUND_2010
				if (!quiet_borer) sound(Ind, "tunnel_rubble", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
			}

			/* Hack -- place an object - Not in town (Khazad becomes l00t source), not on stale IDDC floors */
			if (!istown(wpos) && Ind && !p_ptr->IDDC_logscum && !(cinfo2 & CAVE2_NOYIELD)) {
				/* discovered a special feature? */
				if (dug_feat == FEAT_FOUNTAIN) {
					place_fountain(wpos, y, x);
					note_spot_depth(wpos, y, x);
					everyone_lite_spot(wpos, y, x);
					if (!quiet_full) msg_print(Ind, "You have laid open a fountain!");
					s_printf("DIGGING: %s found a fountain.\n", Ind ? p_ptr->name : "<noone>");
				} else if (dug_feat != FEAT_NONE) {
					cave_set_feat_live(wpos, y, x, dug_feat);
					if (dug_feat == FEAT_WAY_MORE || dug_feat == FEAT_WAY_LESS) {
						if (!quiet_full) msg_print(Ind, "You have uncovered a stairway!");
						s_printf("DIGGING: %s found a staircase.\n", Ind ? p_ptr->name : "<noone>");
					} else {
						//s_printf("DIGGING: %s found water/lava.\n", p_ptr->name);
					}
				} else if (special_k_idx) {
					/* no golem body pieces from rubble, instead allow limbs! */
					if (tval == TV_GOLEM) {
						if (rand_int(2)) sval = SV_GOLEM_ARM;
						else sval = SV_GOLEM_LEG;
						special_k_idx = lookup_kind(tval, sval);
					}
					invcopy(&forge, special_k_idx);
					apply_magic(wpos, &forge, -2, TRUE, TRUE, TRUE, FALSE, make_resf(p_ptr));
					forge.number = 1;
					//forge.level = ;
					forge.marked2 = ITEM_REMOVAL_NORMAL;
					if (!quiet_full) msg_print(Ind, "You have found something!");
					drop_near(TRUE, 0, &forge, -1, wpos, y, x);
					if (tval == TV_CHEST)
						s_printf("DIGGING: %s found a chest.\n", Ind ? p_ptr->name : "<noone>");
					else if (tval == TV_RUNE)
						s_printf("DIGGING: %s found a rune.\n", Ind ? p_ptr->name : "<noone>");
					else
						s_printf("DIGGING: %s found a specific non-golem item.\n", Ind ? p_ptr->name : "<noone>");
				} else if (rand_int(120) < 10 + mining && !p_ptr->IDDC_logscum) { //Basic random item finding. This is allowed atm even if 'no_specials'.
					place_object_restrictor = RESF_NONE;
#if 1
					object_level = find_level_base;
					generate_object(Ind, &forge, wpos, magik(mining), magik(mining / 10), FALSE, make_resf(p_ptr) | RESF_MASK_MID,
						default_obj_theme, p_ptr->luck);
					object_level = old_object_level;
					object_desc(0, o_name, &forge, TRUE, 3);
					if (forge.tval == TV_RUNE && quiet_borer) { /* quiet_borer stone2mud/wall-annihilation also kills any rock-based items ie runes! */
						s_printf("DIGGING: %s eradicated item: %s.\n", Ind ? p_ptr->name : "<noone>", o_name);
					} else {
						s_printf("DIGGING: %s found item: %s.\n", Ind ? p_ptr->name : "<noone>", o_name);
						drop_near(TRUE, 0, &forge, -1, wpos, y, x);
					}
#else //TODO if reenabled: TV_RUNE must be prevented in case of quiet_borer, via new RESF_ flag
					object_level = find_level_base;
					place_object(Ind, wpos, y, x, magik(mining), magik(mining / 10), FALSE, make_resf(p_ptr) | RESF_MASK_MID,
						default_obj_theme, p_ptr->luck, ITEM_REMOVAL_NORMAL, FALSE);
					s_printf("DIGGING: %s found a random item.\n", Ind ? p_ptr->name : "<noone>");
					object_level = old_object_level;
#endif
					if (player_can_see_bold(Ind, y, x) && !quiet_full) msg_print(Ind, "You have found something!");
				}
			}

			/* Notice */
			note_spot_depth(wpos, y, x);

			/* Display */
			everyone_lite_spot(wpos, y, x);

			if (Ind && p_ptr->current_create_sling_ammo) create_sling_ammo_aux(Ind);
		} else {
			/* Message, keep digging */
			if (Ind && !quiet_full) {
				msg_print(Ind, "You dig in the rubble.");
#ifdef USE_SOUND_2010
				if (!quiet_borer) sound(Ind, "tunnel_rubble", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				*more = TRUE;
			}
		}
	}
	else if (cfeat == FEAT_TREE) {
		*no_quake = TRUE;

		/* mow down the vegetation */
		if (((power > wood_power ? power : wood_power) > rand_int(400)) && twall(Ind, wpos, y, x, FEAT_GRASS)) { /* 400 */
			/* Message */
			if (Ind && !quiet_full && !quiet_borer) {
				msg_print(Ind, "You hack your way through the vegetation.");
				if (p_ptr->prace == RACE_ENT) msg_print(Ind, "You have a bad feeling about it.");
#ifdef USE_SOUND_2010
				sound(Ind, "tunnel_tree", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
			}

			if (special_k_idx && tval == TV_GOLEM && Ind && !p_ptr->IDDC_logscum) {
				/* trees can only give wood for golem material: */
				sval = SV_GOLEM_WOOD;
				special_k_idx = lookup_kind(tval, sval);

				invcopy(&forge, special_k_idx);
				apply_magic(wpos, &forge, -2, TRUE, TRUE, TRUE, FALSE, make_resf(p_ptr));
				forge.number = 1;
				//forge.level = ;
				forge.marked2 = ITEM_REMOVAL_NORMAL;
				if (!quiet_full) msg_print(Ind, "You have found something!");
				drop_near(TRUE, 0, &forge, -1, wpos, y, x);
				s_printf("DIGGING: %s found a massive wood piece.\n", Ind ? p_ptr->name : "<noone>");
			} else if (!rand_int(65 - skill_dig / 2) && Ind && !p_ptr->IDDC_logscum && !quiet_borer) {
				/* for player store signs: (non-massive) wood pieces */
				invcopy(&forge, lookup_kind(TV_JUNK, SV_WOOD_PIECE));
				apply_magic(wpos, &forge, -2, TRUE, TRUE, TRUE, FALSE, make_resf(p_ptr));
				forge.number = 1;
				//forge.level = ;
				forge.marked2 = ITEM_REMOVAL_NORMAL;
				if (!quiet_full) msg_print(Ind, "You have found something!");
				drop_near(TRUE, 0, &forge, -1, wpos, y, x);
				s_printf("DIGGING: %s found a wood piece.\n", Ind ? p_ptr->name : "<noone>");
			}
#ifdef ENABLE_DEMOLITIONIST
			else if (
 #ifdef DEMOLITIONIST_IDDC_ONLY
			    in_iddc &&
 #endif
			    /* Note: Increased rarity from 5 to 20 because pieces of wood and other items containing wood can now be ground to chips simply */
			    (Ind && !p_ptr->suppress_ingredients && skill_dig >= ENABLE_DEMOLITIONIST) && !rand_int(3) && !p_ptr->IDDC_logscum) {
				object_type forge;

				invcopy(&forge, lookup_kind(TV_CHEMICAL, SV_WOOD_CHIPS));
				//s_printf("CHEMICAL: %s found wood chips.\n", p_ptr->name); --too common, spammy msg
				forge.owner = p_ptr->id;
				forge.ident |= ID_NO_HIDDEN;
				forge.mode = p_ptr->mode | MODE_NEWLOOT_ITEM;
				forge.iron_trade = p_ptr->iron_trade;
				forge.iron_turn = turn;
				forge.level = 0;
				forge.number = 1;
				forge.weight = k_info[forge.k_idx].weight;
				forge.marked2 = ITEM_REMOVAL_NORMAL;
				drop_near(TRUE, 0, &forge, -1, wpos, y, x);
				if (!p_ptr->warning_ingredients) {
					msg_print(Ind, "\374\377yHINT: You sometimes find ingredients in addition to normal loot because of your");
					msg_print(Ind, "\374\377y      Demolitionist perk. You can toggle these drops via the '\377o/ing\377y' command.");
					msg_print(Ind, "\374\377y      To save bag space you can buy an alchemy satchel at the alchemist in town.");
					s_printf("warning_ingredients: %s\n", p_ptr->name);
					p_ptr->warning_ingredients = 1;
				}
			}
#endif

			/* Notice */
			note_spot_depth(wpos, y, x);
			/* Display */
			everyone_lite_spot(wpos, y, x);
		} else {
			/* Message, keep digging */
			if (Ind && !quiet_full) {
				msg_print(Ind, "You attempt to clear a path.");
#ifdef USE_SOUND_2010
				sound(Ind, "tunnel_tree", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				*more = TRUE;
			}
		}
	} else if (cfeat == FEAT_BUSH) {
		*no_quake = TRUE;

		/* mow down the vegetation */
		if (((power > dual_power ? power : dual_power) > rand_int(300)) && twall(Ind, wpos, y, x, FEAT_GRASS)) { /* 400 */
			/* Message */
			if (Ind && !quiet_full && !quiet_borer) {
				msg_print(Ind, "You hack your way through the vegetation.");
				if (p_ptr->prace == RACE_ENT) msg_print(Ind, "You have a bad feeling about it.");
#ifdef USE_SOUND_2010
				sound(Ind, "tunnel_tree", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
			}

#ifdef ENABLE_DEMOLITIONIST
			if (
 #ifdef DEMOLITIONIST_IDDC_ONLY
			    in_iddc &&
 #endif
			    /* Note: Increased rarity from 5 to 20 because pieces of wood and other items containing wood can now be ground to chips simply */
			    (Ind && !p_ptr->suppress_ingredients && skill_dig >= ENABLE_DEMOLITIONIST) && !rand_int(7) && !p_ptr->IDDC_logscum) {
				object_type forge;

				invcopy(&forge, lookup_kind(TV_CHEMICAL, SV_WOOD_CHIPS));
				//s_printf("CHEMICAL: %s found wood chips.\n", Ind ? p_ptr->name : "<noone>");
				forge.owner = p_ptr->id;
				forge.ident |= ID_NO_HIDDEN;
				forge.mode = p_ptr->mode | MODE_NEWLOOT_ITEM;
				forge.iron_trade = p_ptr->iron_trade;
				forge.iron_turn = turn;
				forge.level = 0;
				forge.number = 1;
				forge.weight = k_info[forge.k_idx].weight;
				forge.marked2 = ITEM_REMOVAL_NORMAL;
				drop_near(TRUE, 0, &forge, -1, wpos, y, x);
				if (!p_ptr->warning_ingredients) {
					msg_print(Ind, "\374\377yHINT: You sometimes find ingredients in addition to normal loot because of your");
					msg_print(Ind, "\374\377y      Demolitionist perk. You can toggle these drops via the '\377o/ing\377y' command.");
					msg_print(Ind, "\374\377y      To save bag space you can buy an alchemy satchel at the alchemist in town.");
					s_printf("warning_ingredients: %s\n", p_ptr->name);
					p_ptr->warning_ingredients = 1;
				}
			}
#endif

			/* Notice */
			note_spot_depth(wpos, y, x);

			/* Display */
			everyone_lite_spot(wpos, y, x);
		} else {
			/* Message, keep digging */
			if (Ind && !quiet_full) {
				msg_print(Ind, "You attempt to clear a path.");
#ifdef USE_SOUND_2010
				sound(Ind, "tunnel_tree", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				*more = TRUE;
			}
		}
	} else if (cfeat == FEAT_IVY) {
		*no_quake = TRUE;

		/* mow down the vegetation */
		if (((power > fibre_power ? power : fibre_power) > rand_int(200)) && twall(Ind, wpos, y, x, FEAT_GRASS)) { /* 400 */
			/* Message */
			if (Ind && !quiet_full && !quiet_borer) {
				msg_print(Ind, "You hack your way through the vegetation.");
#ifdef USE_SOUND_2010
				sound(Ind, "tunnel_tree", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
			}

			/* Notice */
			note_spot_depth(wpos, y, x);

			/* Display */
			everyone_lite_spot(wpos, y, x);
		} else {
			/* Message, keep digging */
			if (Ind && !quiet_full) {
				msg_print(Ind, "You attempt to clear a path.");
#ifdef USE_SOUND_2010
				sound(Ind, "tunnel_tree", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				*more = TRUE;
			}
		}
	} else if (cfeat == FEAT_DEAD_TREE) {
		*no_quake = TRUE;

		/* mow down the vegetation */
		if (((power > wood_power ? power : wood_power) > rand_int(300)) && twall(Ind, wpos, y, x, FEAT_GRASS)) { /* 600 */
			/* Message */
			if (Ind && !quiet_full && !quiet_borer) {
				msg_print(Ind, "You hack your way through the vegetation.");
#ifdef USE_SOUND_2010
				sound(Ind, "tunnel_tree", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
			}

#ifdef ENABLE_DEMOLITIONIST
			if (
 #ifdef DEMOLITIONIST_IDDC_ONLY
			    in_iddc &&
 #endif
			    /* Note: Increased rarity from 5 to 20 because pieces of wood and other items containing wood can now be ground to chips simply */
			    (Ind && !p_ptr->suppress_ingredients && skill_dig >= ENABLE_DEMOLITIONIST) && !rand_int(7) && !p_ptr->IDDC_logscum) {
				object_type forge;

				invcopy(&forge, lookup_kind(TV_CHEMICAL, SV_WOOD_CHIPS));
				//s_printf("CHEMICAL: %s found wood chips.\n", Ind ? p_ptr->name : "<noone>");
				forge.owner = p_ptr->id;
				forge.ident |= ID_NO_HIDDEN;
				forge.mode = p_ptr->mode | MODE_NEWLOOT_ITEM;
				forge.iron_trade = p_ptr->iron_trade;
				forge.iron_turn = turn;
				forge.level = 0;
				forge.number = 1;
				forge.weight = k_info[forge.k_idx].weight;
				forge.marked2 = ITEM_REMOVAL_NORMAL;
				drop_near(TRUE, 0, &forge, -1, wpos, y, x);
				if (!p_ptr->warning_ingredients) {
					msg_print(Ind, "\374\377yHINT: You sometimes find ingredients in addition to normal loot because of your");
					msg_print(Ind, "\374\377y      Demolitionist perk. You can toggle these drops via the '\377o/ing\377y' command.");
					msg_print(Ind, "\374\377y      To save bag space you can buy an alchemy satchel at the alchemist in town.");
					s_printf("warning_ingredients: %s\n", p_ptr->name);
					p_ptr->warning_ingredients = 1;
				}
			}
#endif

			/* Notice */
			note_spot_depth(wpos, y, x);

			/* Display */
			everyone_lite_spot(wpos, y, x);
		} else {
			/* Message, keep digging */
			if (Ind && !quiet_full) {
				msg_print(Ind, "You attempt to clear a path.");
#ifdef USE_SOUND_2010
				sound(Ind, "tunnel_tree", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				*more = TRUE;
			}
		}
	}
	/* Quartz / Magma / Sandwall, with or without (hidden or obvious) treasure */
	else if (((cfeat >= FEAT_MAGMA) && (cfeat <= FEAT_QUARTZ_K)) ||
	    ((cfeat >= FEAT_SANDWALL) && (cfeat <= FEAT_SANDWALL_K))) {
		bool okay = FALSE, gold = FALSE, hard = FALSE, soft = FALSE;
		bool nonobvious = (cinfo & CAVE_ENCASED) != 0; //treasure veins that aren't generated out in the open, but enclosed in streamers

		/* Found gold? Non-obvious vein even for increased value? */
		switch (cfeat) {
		case FEAT_MAGMA_H:
		case FEAT_QUARTZ_H:
		case FEAT_SANDWALL_H:
			//these hidden treasure veins are currently not generated though (todo)
			nonobvious = TRUE;
			//fall through
		case FEAT_MAGMA_K:
		case FEAT_QUARTZ_K:
		case FEAT_SANDWALL_K:
			gold = TRUE;
			break;
		}
		/* Hardness of the material? */
		switch (cfeat) {
		case FEAT_QUARTZ:
		case FEAT_QUARTZ_H:
		case FEAT_QUARTZ_K:
			hard = TRUE;
			break;
		case FEAT_SANDWALL:
		case FEAT_SANDWALL_H:
		case FEAT_SANDWALL_K:
			soft = TRUE;
			break;
		}

#ifdef INDICATE_IMPOSSIBLE
		/* Let us observe (and halt tunneling) if it's impossible for us to make a dent into this material */
		if (hard) {
			/* quartz */
			if (power <= 20) {
 #ifdef USE_SOUND_2010
				if (Ind && !quiet_full) {
					/* Play 'attempt' digging sound once at least */
					if (!quiet_borer) sound(Ind, "tunnel_rock", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
 #endif
					msg_format(Ind, "%s quartz vein.", INDICATE_IMPOSSIBLE);
				}
				return;
			}
		} else if (!soft) {
			/* magma */
			if (power <= 10) {
				if (Ind && !quiet_full) {
 #ifdef USE_SOUND_2010
					/* Play 'attempt' digging sound once at least */
					if (!quiet_borer) sound(Ind, "tunnel_rock", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
 #endif
					msg_format(Ind, "%s magma intrusion.", INDICATE_IMPOSSIBLE);
				}
				return;
			}
		}
#endif

		/* Quartz */
		if (hard) okay = (power > 20 + rand_int(800)); /* 800 */
		/* Sandwall */
		else if (soft) okay = (power > rand_int(300));
		/* Magma */
		else okay = (power > 10 + rand_int(400)); /* 400 */

		if (istown(wpos) || !Ind || p_ptr->IDDC_logscum) gold = FALSE;

		/* hack 2/2: rune proficiency gives a cance to find a rune instead of cash, for nonobvious (enclosed) treasure veins */
		if (gold &&
		    l_ptr && !quiet_borer &&
		    !(special_k_idx && tval == TV_GOLEM) && /* golem actually overrides even proficiency-based rune chance.. */
		    nonobvious) {
			/* Old 'rune' chance doesn't apply to 'gold', so reset it first */
			if (tval == TV_RUNE) special_k_idx = tval = 0;
			/* Apply new rune-proficiency based chance to find a rune from 'gold'. */
			if (rand_int(RUNE_CHANCE / 2) < rune_proficiency) {
				int fallback = special_k_idx, fallback_tval = tval;

				tval = TV_RUNE;
				get_obj_num_hook = NULL;
				get_obj_num_prep_tval(tval, RESF_MASK_MID);
				special_k_idx = get_obj_num(10 + getlevel(wpos), RESF_MASK_MID);
				if (!special_k_idx) {
					special_k_idx = fallback;
					tval = fallback_tval;
				}
			}
		}

		/* Success */
		if (okay && twall(Ind, wpos, y, x, soft ? FEAT_SAND : FEAT_FLOOR)) {
			if (Ind && !quiet_full && !quiet_borer) {
				if (!p_ptr->warning_tunnel4 && o_ptr->k_idx && o_ptr->tval == TV_DIGGING) {
					/* Don't display hint if digging tool is already so highly enchanted that it was probably done manually -> player seems to know */
					if (o_ptr->to_h < 8 || o_ptr->to_d < 8) msg_print(Ind, "\374\377yHINT: Further strengthen your digging tool by enchanting it to-hit and to-dam!");
					s_printf("warning_tunnel4: %s\n", p_ptr->name);
					p_ptr->warning_tunnel4 = 1;
				}
#ifdef USE_SOUND_2010
				sound(Ind, "tunnel_rock", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				msg_format(Ind, "You have finished the tunnel in the %s.", soft ? "sandwall" : (hard ? "quartz vein" : "magma intrusion"));
			}

			/* Found treasure */
			if (gold) {
				if (special_k_idx && (tval == TV_GOLEM || tval == TV_RUNE)) {
					invcopy(&forge, special_k_idx);
					apply_magic(wpos, &forge, -2, TRUE, TRUE, TRUE, FALSE, make_resf(p_ptr));
					forge.number = 1;
					//forge.level = ;
					forge.marked2 = ITEM_REMOVAL_NORMAL;
					drop_near(TRUE, 0, &forge, -1, wpos, y, x);
					s_printf("DIGGING: %s found a %s.\n", p_ptr->name, tval == TV_GOLEM ? "metal piece" : "rune");
				} else {
					int mult = 30;
					/* problem: level-advancement is superlinear which should probably go for rewards too,
					   but first few skills shouldn't explosively inflate the difference between (eg take 1.000 Digging to gain +25% cash),
					   however towards 50 it shouldn't increase exponentially into oblivion so we need a limiter term for that region probably - C. Blue */
					int bonus = nonobvious ?
#ifdef DIG_SKILL_VS_DEPTH_COMPLICATED
					    (1000 * ((4 + mining) * (4 + mining)) / (3000 / (101 - mining))) / 25 + 80 + rand_int(find_level_base) + skill_dig : //lulz
#else
					/* For this strict dlev capping "if (mining > find_level * 1) mining = find_level;" the /25 divisor should be /20, which still gives good results. */
					    (1000 * ((4 + mining) * (4 + mining)) / (3000 / (101 - mining))) / 20 + 80 + rand_int(find_level_base) + skill_dig :
#endif
					    mining * 3 + skill_dig * 2;

					object_level = find_level;
					place_gold(Ind, wpos, y, x, mult, bonus);
					if (nonobvious) s_printf("DIGGING: %s (F%d,S%d,O%d) digs nonobvious (x%d+%d=%dAu).\n",
					    p_ptr->name, find_level_base, skill_dig, object_level,
					    mult, bonus, !c_ptr->o_idx ? 0 : (o_list[c_ptr->o_idx].tval != TV_GOLD ? 0 : o_list[c_ptr->o_idx].pval));
					else s_printf("DIGGING: %s (F%d,S%d,O%d) digs obvious (x%d+%d=%dAu).\n",
					    p_ptr->name, find_level_base, skill_dig, object_level,
					    mult, bonus, !c_ptr->o_idx ? 0 : (o_list[c_ptr->o_idx].tval != TV_GOLD ? 0 : o_list[c_ptr->o_idx].pval));
					object_level = old_object_level;
					c_ptr->info2 |= CAVE2_MINED; //mark for warning_tunnel_hidden
				}
				note_spot_depth(wpos, y, x);
				everyone_lite_spot(wpos, y, x);
				if (Ind && !quiet_full) msg_print(Ind, "You have found something!");
			} else if (istown(wpos)) {
				/* nothing */
			/* found special feature */
			} else if (dug_feat == FEAT_FOUNTAIN) {
				if (magik(35)) {
					place_fountain(wpos, y, x);
					note_spot_depth(wpos, y, x);
					everyone_lite_spot(wpos, y, x);
					if (Ind && !quiet_full) msg_print(Ind, "You have laid open a fountain!");
					s_printf("DIGGING: %s found a fountain.\n", Ind ? p_ptr->name : "<noone>");
				}
			} else if (dug_feat != FEAT_NONE &&
			    dug_feat != FEAT_WAY_MORE &&
			    dug_feat != FEAT_WAY_LESS) {
				//if (magik(100)) {
					cave_set_feat_live(wpos, y, x, dug_feat);
					//s_printf("DIGGING: %s found water/lava.\n", Ind ? p_ptr->name : "<noone>");
				//}
			} else if (!rand_int(10) && special_k_idx && tval == TV_RUNE && !p_ptr->IDDC_logscum) {
					invcopy(&forge, special_k_idx);
					apply_magic(wpos, &forge, -2, TRUE, TRUE, TRUE, FALSE, make_resf(p_ptr));
					forge.number = 1;
					//forge.level = ;
					forge.marked2 = ITEM_REMOVAL_NORMAL;
					if (Ind && !quiet_full)  msg_print(Ind, "You have found something!");
					drop_near(TRUE, 0, &forge, -1, wpos, y, x);
					s_printf("DIGGING: %s found a rune.\n", Ind ? p_ptr->name : "<noone>");
			}
#ifdef ENABLE_DEMOLITIONIST
			/* Sandwall - Possibly find ingredients: Saltpetre */
			else if (
 #ifdef DEMOLITIONIST_IDDC_ONLY
			    in_iddc &&
 #endif
			    soft && Ind && !p_ptr->suppress_ingredients && skill_dig >= ENABLE_DEMOLITIONIST && !rand_int(7) && !p_ptr->IDDC_logscum) {
				object_type forge;

				invcopy(&forge, lookup_kind(TV_CHEMICAL, SV_SALTPETRE));
				//s_printf("CHEMICAL: %s found saltpetre.\n", p_ptr->name);
				forge.owner = p_ptr->id;
				forge.ident |= ID_NO_HIDDEN;
				forge.mode = p_ptr->mode | MODE_NEWLOOT_ITEM;
				forge.iron_trade = p_ptr->iron_trade;
				forge.iron_turn = turn;
				forge.level = 0;
				forge.number = 1;
				forge.weight = k_info[forge.k_idx].weight;
				forge.marked2 = ITEM_REMOVAL_NORMAL;
				drop_near(TRUE, 0, &forge, -1, wpos, y, x);
				if (!p_ptr->warning_ingredients) {
					msg_print(Ind, "\374\377yHINT: You sometimes find ingredients in addition to normal loot because of your");
					msg_print(Ind, "\374\377y      Demolitionist perk. You can toggle these drops via the '\377o/ing\377y' command.");
					msg_print(Ind, "\374\377y      To save bag space you can buy an alchemy satchel at the alchemist in town.");
					s_printf("warning_ingredients: %s\n", p_ptr->name);
					p_ptr->warning_ingredients = 1;
				}
			}
			/* Magma - Possibly find ingredients: Sulfur (volcanic/undersea), Vitriol */
			else if (
 #ifdef DEMOLITIONIST_IDDC_ONLY
			    in_iddc &&
 #endif
			    !hard && !soft && Ind && !p_ptr->suppress_ingredients && skill_dig >= ENABLE_DEMOLITIONIST && !rand_int(7) && !p_ptr->IDDC_logscum) {
				object_type forge;

				if (skill_dig >= ENABLE_DEMOLITIONIST + 10) invcopy(&forge, lookup_kind(TV_CHEMICAL, rand_int(4) ? SV_SULFUR : SV_VITRIOL));
				else invcopy(&forge, lookup_kind(TV_CHEMICAL, SV_SULFUR));
				//s_printf("CHEMICAL: %s found %s.\n", p_ptr->name, forge.sval == SV_SULFUR ? "sulfur" : "vitriol");
				forge.owner = p_ptr->id;
				forge.ident |= ID_NO_HIDDEN;
				forge.mode = p_ptr->mode | MODE_NEWLOOT_ITEM;
				forge.iron_trade = p_ptr->iron_trade;
				forge.iron_turn = turn;
				forge.level = 0;
				forge.number = 1;
				forge.weight = k_info[forge.k_idx].weight;
				forge.marked2 = ITEM_REMOVAL_NORMAL;
				drop_near(TRUE, 0, &forge, -1, wpos, y, x);
				if (!p_ptr->warning_ingredients) {
					msg_print(Ind, "\374\377yHINT: You sometimes find ingredients in addition to normal loot because of your");
					msg_print(Ind, "\374\377y      Demolitionist perk. You can toggle these drops via the '\377o/ing\377y' command.");
					msg_print(Ind, "\374\377y      To save bag space you can buy an alchemy satchel at the alchemist in town.");
					s_printf("warning_ingredients: %s\n", p_ptr->name);
					p_ptr->warning_ingredients = 1;
				}
			}
			/* Quartz (whatever =p) - Possibly find ingredients: Metal powder */
			else if (
 #ifdef DEMOLITIONIST_IDDC_ONLY
			    in_iddc &&
 #endif
			    hard && Ind && !p_ptr->suppress_ingredients && skill_dig >= ENABLE_DEMOLITIONIST + 5 && !rand_int(7) && !p_ptr->IDDC_logscum) {
				object_type forge;

				invcopy(&forge, lookup_kind(TV_CHEMICAL, SV_METAL_POWDER));
				//s_printf("CHEMICAL: %s found Reactive Metal Powder.\n", p_ptr->name);
				forge.owner = p_ptr->id;
				forge.ident |= ID_NO_HIDDEN;
				forge.mode = p_ptr->mode | MODE_NEWLOOT_ITEM;
				forge.iron_trade = p_ptr->iron_trade;
				forge.iron_turn = turn;
				forge.level = 0;
				forge.number = 1;
				forge.weight = k_info[forge.k_idx].weight;
				forge.marked2 = ITEM_REMOVAL_NORMAL;
				drop_near(TRUE, 0, &forge, -1, wpos, y, x);
				if (!p_ptr->warning_ingredients) {
					msg_print(Ind, "\374\377yHINT: You sometimes find ingredients in addition to normal loot because of your");
					msg_print(Ind, "\374\377y      Demolitionist perk. You can toggle these drops via the '\377o/ing\377y' command.");
					msg_print(Ind, "\374\377y      To save bag space you can buy an alchemy satchel at the alchemist in town.");
					s_printf("warning_ingredients: %s\n", p_ptr->name);
					p_ptr->warning_ingredients = 1;
				}
			}
#endif
		}

		/* Failure */
		else {
			/* Message, continue digging */
			if (Ind && !quiet_full) {
				msg_print(Ind, f_text + f_ptr->tunnel);
#ifdef USE_SOUND_2010
				if (!quiet_borer) sound(Ind, "tunnel_rock", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				*more = TRUE;
			}
		}
#if 0
		/* Failure (quartz) */
		else if (hard) {
			/* Message, continue digging */
			if (Ind && !quiet_full) {
				msg_print(Ind, "You tunnel into the quartz vein.");
#ifdef USE_SOUND_2010
				if (!quiet_borer) sound(Ind, "tunnel_rock", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				*more = TRUE;
			}
		}

		/* Failure (magma) */
		else {
			/* Message, continue digging */
			if (Ind && !quiet_full) {
				msg_print(Ind, "You tunnel into the magma intrusion.");
#ifdef USE_SOUND_2010
				if (!quiet_borer) sound(Ind, "tunnel_rock", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				*more = TRUE;
			}
		}
#endif	/* 0 */
	}
	/* Default to secret doors */
	else if (cfeat == FEAT_SECRET) {
		struct c_special *cs_ptr;
		int featm = cfeat, diff, diff_plus = 0; /* cfeat: just to kill compiler warning */

		if ((cs_ptr = GetCS(c_ptr, CS_MIMIC)))
			featm = cs_ptr->sc.omni;
		else /* Apply "mimic" field */
			featm = f_info[featm].mimic;

		/* Is the mimicked feat un-tunnelable? */
		if (!(f_info[featm].flags1 & FF1_TUNNELABLE) ||
		    (f_info[featm].flags1 & FF1_PERMANENT)) {
			if (Ind && !quiet_full) {
				/* Message */
				msg_print(Ind, f_text + f_info[featm].tunnel); 
				p_ptr->energy -= level_speed(wpos) / 2;
			}
			return;
		}

#if 0
		/* This might give it away, maybe comment out.. */
		*no_quake = TRUE;
#endif

		/* To allow for INDICATE_IMPOSSIBLE, we need to distinguish between mimicked features that would affect it. */
		switch (featm) {
		case FEAT_WEB:
			if (power < fibre_power) power = fibre_power;
			diff = 100;
			break;
		case FEAT_RUBBLE:
			diff = 200;
			break;
		case FEAT_IVY:
			if (power < fibre_power) power = fibre_power;
			diff = 200;
			break;
		case FEAT_BUSH:
			if (power < dual_power) power = dual_power;
			diff = 300;
			break;
		case FEAT_DEAD_TREE:
			if (power < wood_power) power = wood_power;
			diff = 300;
			break;
		case FEAT_TREE:
			if (power < wood_power) power = wood_power;
			diff = 400;
			break;
		case FEAT_MAGMA:
		case FEAT_MAGMA_H:
		case FEAT_MAGMA_K:
			diff = 400;
			diff_plus = 10;
			break;
		case FEAT_QUARTZ:
		case FEAT_QUARTZ_H:
		case FEAT_QUARTZ_K:
			diff = 800;
			diff_plus = 20;
			break;
		default:
			/* granite wall, ice wall, misc walls.. */
			if (featm >= FEAT_WALL_EXTRA) {
				diff = 1600;
				diff_plus = 40;
			}
			/* nothing left actually! - arbitrary paranoia values (see below, keep consistent anyway) */
			else {
				diff = 1200;
				diff_plus = 30;
			}
			break;
		}

#ifdef INDICATE_IMPOSSIBLE
		/* Let us observe (and halt tunneling) if it's impossible for us to make a dent into this material */
		if (power <= diff_plus) {
			if (Ind && !quiet_full) {
 #ifdef USE_SOUND_2010
				/* Play 'attempt' digging sound once at least */
				if (!quiet_borer) sound(Ind, "tunnel_rock", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
 #endif
				msg_format(Ind, "%s %s.", INDICATE_IMPOSSIBLE, f_name + f_info[featm].name);
			}
			return;
		}
#endif

#ifdef INDICATE_IMPOSSIBLE
		/* Let us observe (and halt tunneling) if it's impossible for us to make a dent into this material */
#endif

		/* hack: 'successful' tunnelling reveals the secret door. */
		if (power > rand_int(diff) + diff_plus) {
			struct c_special *cs_ptr;

			if (Ind && !quiet_full) msg_print(Ind, "You have found a secret door!");
			/* un-hide the true feat (a door!) */
			c_ptr->feat = FEAT_DOOR_HEAD + 0x00;
			/* Clear mimic feature */
			if ((cs_ptr = GetCS(c_ptr, CS_MIMIC))) cs_erase(c_ptr, cs_ptr);

			note_spot_depth(wpos, y, x);
			everyone_lite_spot(wpos, y, x);
			*door = TRUE;
			if (c_ptr->custom_lua_search > 0 && exec_lua(0, format("custom_search(%d,%d)", Ind, c_ptr->custom_lua_search))) return;
		} else {
			if (Ind && !quiet_full) {
				msg_print(Ind, f_text + f_info[featm].tunnel);
				*more = TRUE;
			}
		}

#ifdef USE_SOUND_2010
		/* sound: for now assume that only such features get mimicked that
		   would cause 'tunnel_rock' sfx, eg no trees or rubble. - C. Blue */
		if (Ind && !quiet_borer) sound(Ind, "tunnel_rock", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif

		/* Set off trap */
		if (GetCS(c_ptr, CS_TRAPS)) {
#ifdef TRAP_REVEALS_DOOR
			struct c_special *cs_ptr;
#endif
			if (Ind && !quiet_full) player_activate_door_trap(Ind, y, x);
			/* got disturbed! */
			*more = FALSE;
			*door = TRUE;
#ifdef TRAP_REVEALS_DOOR
			/* Message */
			if (Ind && !quiet_full) msg_print(Ind, "You have found a secret door.");
			/* Pick a door XXX XXX XXX */
			c_ptr->feat = FEAT_DOOR_HEAD + 0x00;
			/* Clear mimic feature */
			if ((cs_ptr = GetCS(c_ptr, CS_MIMIC))) cs_erase(c_ptr, cs_ptr);

			/* Notice */
			note_spot_depth(wpos, y, x);
			/* Redraw */
			everyone_lite_spot(wpos, y, x);
#endif
			if (c_ptr->custom_lua_search > 0 && exec_lua(0, format("custom_search(%d,%d)", Ind, c_ptr->custom_lua_search))) return;
		}
#if 0 /* keep tunneling, as the player cannot know he's actually searching now: The feat still appears like solid wall. */
		/* Hack -- Search */
		if (Ind && !quiet_full && *more) search(Ind);
#endif
	}
	/* Granite + misc (Ice..) */
	else if (cfeat >= FEAT_WALL_EXTRA) {
#ifdef INDICATE_IMPOSSIBLE
		/* Let us observe (and halt tunneling) if it's impossible for us to make a dent into this material */
		if (power <= 40) {
			if (Ind && !quiet_full) {
 #ifdef USE_SOUND_2010
				/* Play 'attempt' digging sound once at least */
				if (!quiet_borer) sound(Ind, "tunnel_rock", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
 #endif
				msg_format(Ind, "%s %s.", INDICATE_IMPOSSIBLE, f_name + f_info[cfeat].name);
			}
			return;
		}
#endif

		/* Tunnel */
		if ((power > 40 + rand_int(1600)) && twall(Ind, wpos, y, x, cfeat == FEAT_ICE_WALL ? FEAT_ICE : FEAT_FLOOR)) { /* 1600 */
			if (Ind && !quiet_full && !quiet_borer) {
				msg_format(Ind, "You have finished the tunnel in the %s.", f_name + f_info[cfeat].name);
#ifdef USE_SOUND_2010
				sound(Ind, "tunnel_rock", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
			}
			if (!istown(wpos)) {
				/* found special feature */
				if (dug_feat == FEAT_FOUNTAIN) {
					if (magik(50)) {
						place_fountain(wpos, y, x);
						note_spot_depth(wpos, y, x);
						everyone_lite_spot(wpos, y, x);
						if (Ind && !quiet_full) msg_print(Ind, "You have laid open a fountain!");
						s_printf("DIGGING: %s found a fountain.\n", Ind ? p_ptr->name : "<noone>");
					}
				} else if (dug_feat != FEAT_NONE &&
				    dug_feat != FEAT_WAY_MORE &&
				    dug_feat != FEAT_WAY_LESS) {
					if (magik(100)) {
						cave_set_feat_live(wpos, y, x, dug_feat);
						//s_printf("DIGGING: %s found water/lava.\n", Ind ? p_ptr->name : "<noone>");
					}
				} else if (!rand_int(20) && special_k_idx && tval == TV_RUNE) {
					invcopy(&forge, special_k_idx);
					apply_magic(wpos, &forge, -2, TRUE, TRUE, TRUE, FALSE, make_resf(p_ptr));
					forge.number = 1;
					//forge.level = ;
					forge.marked2 = ITEM_REMOVAL_NORMAL;
					if (Ind && !quiet_full) msg_print(Ind, "You have found something!");
					drop_near(TRUE, 0, &forge, -1, wpos, y, x);
					s_printf("DIGGING: %s found a rune.\n", Ind ? p_ptr->name : "<noone>");
				}
			}
		}

		/* Keep trying */
		else {
			/* We may continue tunelling */
			if (Ind && !quiet_full) {
				//msg_print(Ind, "You tunnel into the granite wall.");
				msg_print(Ind, f_text + f_ptr->tunnel);
#ifdef USE_SOUND_2010
				if (!quiet_borer) sound(Ind, "tunnel_rock", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				*more = TRUE;
			}
		}
	}
	/* Spider Webs */
	else if (cfeat == FEAT_WEB) {
		*no_quake = TRUE;

		/* Tunnel - hack: swords/axes help similarly as for trees/bushes/ivy */
		if ((((power > fibre_power) ? power : fibre_power) > rand_int(100)) && twall(Ind, wpos, y, x, FEAT_DIRT)) {
			if (Ind && !quiet_full && !quiet_borer) {
				msg_print(Ind, "You have cleared the spider web.");
#ifdef USE_SOUND_2010
				sound(Ind, "miss", NULL, SFX_TYPE_NO_OVERLAP, TRUE); //'swiping' sfx^^
#endif
			}
		}
		/* Keep trying */
		else {
			/* We may continue tunelling */
			if (Ind && !quiet_full) {
				msg_print(Ind, "You try to clear the spider web.");
#ifdef USE_SOUND_2010
				sound(Ind, "miss", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				*more = TRUE;
			}
		}
	}
	/* Other stuff - Note: There is none left, actually.
	   Ice walls are treated as 'Granite Wall' above already! */
	else {
#ifdef INDICATE_IMPOSSIBLE
		/* Let us observe (and halt tunneling) if it's impossible for us to make a dent into this material */
		if (power <= 30) {
			if (Ind && !quiet_full) {
 #ifdef USE_SOUND_2010
				/* Play 'attempt' digging sound once at least */
				if (!quiet_borer) sound(Ind, "tunnel_rock", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
 #endif
				msg_format(Ind, "%s %s.", INDICATE_IMPOSSIBLE, f_name + f_info[cfeat].name);
			}
			return;
		}
#endif

		/* Tunnel */
		if ((power > 30 + rand_int(1200)) && twall(Ind, wpos, y, x, FEAT_FLOOR)) { /* some random, arbitrary hardness between granite and next-softer materials.. unused anyway */
			if (Ind && !quiet_full && !quiet_borer) {
				msg_format(Ind, "You have finished the tunnel in the %s.", f_name + f_info[cfeat].name);
#ifdef USE_SOUND_2010
				sound(Ind, "tunnel_rubble", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
			}
		}
		/* Keep trying */
		else {
			/* We may continue tunelling */
			if (Ind && !quiet_full) {
				msg_print(Ind, f_text + f_ptr->tunnel);
#ifdef USE_SOUND_2010
				sound(Ind, "tunnel_rubble", NULL, SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				*more = TRUE;
			}
		}
	}
}

/*
 * Tunnels through "walls" (including rubble and closed doors)
 *
 * Note that tunneling almost always takes time, since otherwise
 * you can use tunnelling to find monsters.  Also note that you
 * must tunnel in order to hit invisible monsters in walls (etc).
 *
 * Digging is very difficult without a "digger" weapon, but can be
 * accomplished by strong players using heavy weapons.
 *
 * quiet_borer: KILL_WALL form that instantly removes the feat.
 *
 */
/* XXX possibly wrong */
/* New Digging features: Uncover features or find objects - (C. Blue)
   -Only possible when digging by hand/tool, not via any magic.
   -Rubble: Staircase, Fountain, Lava/Water,
            Chest, Random item, Golem limb, Rune
   -Tree: Golem body (wood), Piece of Wood
   -Quartz/Magma obvious treasure vein: Fountain, Lava/Water,
            Golem body (metal), Rune
   -Quartz/Magma hidden treasure vein: Fountain, Lava/Water,
            Golem body (metal), Rune (high chance if proficient)
   -Granite Wall: Fountain, Lava/Water
            Rune
    The highest proficiency in any rune-skill decides how much the chance to
    find a rune in above cases of 'nonobvious' treasure will be, independantly
    of digging skill.
    For obvious treasure or just hard walls, the chance to find a rune depends
    (as finding any other special thing) only on digging skill.

    NOTE: Ivy is listed but currently counts as FLOOR, so it has no effect/isn't tunneable.
*/
/* TODO: Make shovels and picks actually rather inefficient vs plants: Increase plants' digging difficulties and in turn raise wood_power and fibre_power for proper weapons. */
void do_cmd_tunnel(int Ind, int dir, bool quiet_borer) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
#ifdef ENABLE_DEMOLITIONIST
 #ifdef DEMOLITIONIST_IDDC_ONLY
	bool in_iddc = in_irondeepdive(wpos);
 #endif
#endif

	cave_type **zcave, *c_ptr;
	feature_type *f_ptr;
	bool old_floor = FALSE, no_quake, more, door;

	object_type *o_ptr = &p_ptr->inventory[INVEN_TOOL];
	object_type *o2_ptr = &p_ptr->inventory[INVEN_WIELD];
	object_type *o3_ptr = &p_ptr->inventory[INVEN_ARM];
	object_type *o23_ptr = o2_ptr;
	char o_name[ONAME_LEN];

	int cfeat, y, x;
#ifndef EQUIPPABLE_DIGGERS
	int power = p_ptr->skill_dig + (quiet_borer ? 20000 : 0);
#else
	int power = (p_ptr->skill_dig > p_ptr->skill_dig2 ? p_ptr->skill_dig : p_ptr->skill_dig2) + (quiet_borer ? 20000 : 0);
	bool swapped = FALSE;
	object_type object_storage;
#endif
	int wood_power = 0, fibre_power = 0;

	bool impact = FALSE;
	int impact_power = 0, impact_power_tool = 0, impact_power_weapon = 0, impact_power_weapon2 = 0;


	if (!(zcave = getcave(wpos))) return;


	/* --- Prepare player digging tools, digging power, digging direction --- */


	/* Get a direction to tunnel, or Abort */
	if (!dir) return; /* dir == 0 is currently possible since bad_dir() is used in nserver.c, but client doesn't send such dir usually, and it has no use for digging atm. */

#ifdef ENABLE_OUNLIFE
	/* Wraithstep gets auto-cancelled on forced interaction with solid environment */
	if (p_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1)) set_tim_wraith(Ind, 0);
#endif

	/* Ghosts have no need to tunnel ; not in WRAITHFORM */
	if (CANNOT_OPERATE_SPECTRAL && !is_admin(p_ptr)) {
		/* Message */
		msg_print(Ind, "You cannot tunnel without a material body.");
		p_ptr->energy -= level_speed(&p_ptr->wpos) / 3;
		return;
	}

#ifdef EQUIPPABLE_DIGGERS
//s_printf("pow %d, sd %d, sd2 %d.  ", power, p_ptr->skill_dig, p_ptr->skill_dig2);
	/* Do we wield a digging tool in the "wield" slot? */
	if (o2_ptr->k_idx && o2_ptr->tval == TV_DIGGING) {
		/* If we don't wield a digging tool in the tool slot, use the one in the wield slot instead */
		if (!o_ptr->k_idx || o_ptr->tval != TV_DIGGING) {
			/* Hack: Temporarily switch the two slots! */
			object_storage = *o_ptr;
			*o_ptr = *o2_ptr;
			*o2_ptr = object_storage;
			swapped = TRUE;
		}
		/* We wield two digging tools. Pick the stronger of the two automatically */
		else if (p_ptr->skill_dig2 > p_ptr->skill_dig) {
			/* Hack: Temporarily switch the two slots! */
			object_storage = *o_ptr;
			*o_ptr = *o2_ptr;
			*o2_ptr = object_storage;
			swapped = TRUE;
		}
	}
#endif

	/* check if our weapons can help hacking down wood etc */
	if (o2_ptr->k_idx && !p_ptr->heavy_wield) {
		switch (o2_ptr->tval) {
		case TV_AXE:
			fibre_power = 20 + o2_ptr->weight / 20 + (o2_ptr->to_h + o2_ptr->to_d) / 3;
			wood_power = 40 + o2_ptr->weight / 10 + (o2_ptr->to_h + o2_ptr->to_d) / 2; break;
		case TV_SWORD:
			fibre_power = 40 + o2_ptr->weight / 40 + (o2_ptr->to_h + o2_ptr->to_d) / 2;
			wood_power = 20 + o2_ptr->weight / 20 + (o2_ptr->to_h + o2_ptr->to_d) / 3; break;
		case TV_POLEARM:
			if (o2_ptr->sval == SV_SCYTHE ||
			    o2_ptr->sval == SV_SCYTHE_OF_SLICING ||
			    o2_ptr->sval == SV_SICKLE)
				fibre_power = 40 + o2_ptr->weight / 10 + (o2_ptr->to_h + o2_ptr->to_d) / 2;
			break;
		}

		wood_power += adj_con_mhp[p_ptr->stat_ind[A_STR]] - 128 + 5; //+0..+30
		fibre_power += adj_con_mhp[p_ptr->stat_ind[A_STR]] - 128 + 5; //+0..+30

		if (p_ptr->awkward_wield) {
			wood_power >>= 1;
			fibre_power >>= 1;
		}
	}
	if (o3_ptr->k_idx && !p_ptr->heavy_wield) {
		int wp = 0, fp = 0;

		switch (o3_ptr->tval) {
		case TV_AXE:
			fp = 30 + o3_ptr->weight / 20 + (o3_ptr->to_h + o3_ptr->to_d) / 3;
			wp = 60 + o3_ptr->weight / 10 + (o3_ptr->to_h + o3_ptr->to_d) / 2; break;
		case TV_SWORD:
			fp = 60 + o3_ptr->weight / 10 + (o3_ptr->to_h + o3_ptr->to_d) / 2;
			wp = 30 + o3_ptr->weight / 20 + (o3_ptr->to_h + o3_ptr->to_d) / 3; break;
		case TV_POLEARM:
			if (o3_ptr->sval == SV_SCYTHE ||
			    o3_ptr->sval == SV_SCYTHE_OF_SLICING ||
			    o3_ptr->sval == SV_SICKLE)
				fp = 60 + o3_ptr->weight / 10 + (o3_ptr->to_h + o3_ptr->to_d) / 2;
			break;
		}

		wp += adj_con_mhp[p_ptr->stat_ind[A_STR]] - 128 + 5; //+0..+30
		fp += adj_con_mhp[p_ptr->stat_ind[A_STR]] - 128 + 5; //+0..+30

		if (p_ptr->awkward_wield) {
			wp >>= 1;
			fp >>= 1;
		}

		if (wp > wood_power) wood_power = wp;
		if (fp > fibre_power) fibre_power = fp;
	}

	/* Must be have something to dig with, or power gets halved */
	if (!o_ptr->k_idx || o_ptr->tval != TV_DIGGING) {
		power >>= 1;
		if (!p_ptr->warning_tunnel3) {
			msg_print(Ind, "\374\377yHINT: You can tunnel more effectively if you equip a digging tool.");
			msg_print(Ind, "\374\377y      The general store in town sells some basic shovels and picks.");
			s_printf("warning_tunnel3: %s\n", p_ptr->name);
			p_ptr->warning_tunnel3 = 1;
		}
	}

	/* have at least 1 digging power so you can dig through rubble/organic stuff */
	if (!power) power = 1;

	/* Check whether we cause an earthquake */
	/* Check player himself */
	if (p_ptr->impact) impact_power = adj_str_dig[p_ptr->stat_ind[A_STR]]; //0..100; actually there is no such item giving this flag atm
	/* Check digging tool */
	if (o_ptr->k_idx && o_ptr->tval == TV_DIGGING
#ifdef ALLOW_NO_QUAKE_INSCRIPTION
	    && (!check_guard_inscription(o_ptr->note, 'Q') || dir == 5)
#endif
	    ) {
		u32b fx, f5;

		object_flags(o_ptr, &fx, &fx, &fx, &fx, &f5, &fx, &fx);
		if (f5 & TR5_IMPACT) {
			impact_power_tool = power; //1..530 (digging power is used)
			if (impact_power_tool >= 201) impact_power_tool = 201; //cap for guaranteed success, aka 100%
			if (impact_power_tool) impact_power_tool = (impact_power_tool - 1) / 2; /* calculate actual percentage for easier comparison with the other two candidates */
		}
	}
	/* Check weapons */
	if (o2_ptr->k_idx
#ifdef ALLOW_NO_QUAKE_INSCRIPTION
	    && (!check_guard_inscription(o2_ptr->note, 'Q') || dir == 5)
#else
	    && (!check_guard_inscription(o2_ptr->note, 'Q') || o2_ptr->name1 != ART_GROND || dir == 5)
#endif
	    ) {
		u32b fx, f5;

		object_flags(o2_ptr, &fx, &fx, &fx, &fx, &f5, &fx, &fx);
		//~18 avg (6d6 weapon) -> 49% chance on attack; 5(best)..45(worst) total range
		if (f5 & TR5_IMPACT) {
			impact_power_weapon = 500 / (10 + (((o2_ptr->dd * (o2_ptr->ds + 1)) / 2) - o2_ptr->dd) * o2_ptr->dd * o2_ptr->ds / (o2_ptr->dd * (o2_ptr->ds - 1) + 1));
			impact_power_weapon = 100 - impact_power_weapon * 100 / 35; /* calculate actual percentage for easier comparison with the other two candidates */
			if (impact_power_weapon <= 0) impact_power_weapon = 1; //if the weapon doesn't totally suck we might be able to get a lucky roll instead of the average roll - give this case least priority though of all 3.
		}
	}
	if (o3_ptr->k_idx && (is_weapon(o3_ptr->tval) || o3_ptr->tval == TV_MSTAFF)
#ifdef ALLOW_NO_QUAKE_INSCRIPTION
	    && (!check_guard_inscription(o3_ptr->note, 'Q') || dir == 5)
#endif
	    ) {
		u32b fx, f5;

		object_flags(o3_ptr, &fx, &fx, &fx, &fx, &f5, &fx, &fx);
		//~18 avg (6d6 weapon) -> 49% chance on attack; 5(best)..45(worst) total range
		if (f5 & TR5_IMPACT) {
			impact_power_weapon2 = 500 / (10 + (((o3_ptr->dd * (o3_ptr->ds + 1)) / 2) - o3_ptr->dd) * o3_ptr->dd * o3_ptr->ds / (o3_ptr->dd * (o3_ptr->ds - 1) + 1));
			impact_power_weapon2 = 100 - impact_power_weapon2 * 100 / 35; /* calculate actual percentage for easier comparison with the other two candidates */
			if (impact_power_weapon2 <= 0) impact_power_weapon2 = 1; //if the weapon doesn't totally suck we might be able to get a lucky roll instead of the average roll - give this case least priority though of all 3.
		}
	}

	/* Get location and grid */
	y = p_ptr->py + ddy[dir];
	x = p_ptr->px + ddx[dir];
	c_ptr = &zcave[y][x];

	/* Extract grid */
	cfeat = c_ptr->feat;
	f_ptr = &f_info[cfeat];

	if (c_ptr->custom_lua_tunnel < 0 && exec_lua(0, format("custom_tunnel(%d,%d)", Ind, c_ptr->custom_lua_tunnel))) return;
	if (c_ptr->custom_lua_tunnel_hand < 0 && !quiet_borer && exec_lua(0, format("custom_tunnel_hand(%d,%d)", Ind, c_ptr->custom_lua_tunnel_hand))) return;


	/* --- Specialty: Strike ground beneath us to cause an earthquake --- */


	/* No tunnelling through empty air, but allow 'tunneling' the floor we're standing on to cause quakes */
	if ((cave_floor_bold(zcave, y, x)) || (cfeat == FEAT_PERM_CLEAR)) {
		/* Hack: Allow causing earthquakes with appropriate weapons (!) or diggers or p_ptr->impact (unavailable atm) by hitting the empty floor */
		if (dir == 5 && cfeat != FEAT_PERM_CLEAR) {
			if (istownarea(&p_ptr->wpos, MAX_TOWNAREA)) {
				msg_print(Ind, "The floor around the town area seems very solid.");
				p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;
#ifdef EQUIPPABLE_DIGGERS
				if (swapped) {
					/* Unhack */
					object_storage = *o_ptr;
					*o_ptr = *o2_ptr;
					*o2_ptr = object_storage;
				}
#endif
				return;
			}

			/* we need to deduct one turn of energy appropriately */
			p_ptr->energy -= level_speed(&p_ptr->wpos);
			un_afk_idle(Ind);
			break_cloaking(Ind, 0);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);
			disturb(Ind, 0, 0);

			/* Apply earthquakes - maybe todo somehow: exempt sand floor/nether mist? =p */

			/* Pick our most promising source of earthquaking (barehanded vs digging tool vs weapon) */
			if (impact_power_weapon2 > impact_power_weapon) {
				 //Secondary weapon is quakier than primary, so use it instead
				o23_ptr = o3_ptr;
				impact_power_weapon = impact_power_weapon2;
			}
			if (impact_power > impact_power_tool) {
				if (impact_power >= impact_power_weapon) impact_power_weapon = 0;
				else impact_power = 0;
				impact_power_tool = 0;
			} else {
				impact_power = 0;
				if (impact_power_tool >= impact_power_weapon) impact_power_weapon = 0;
				else impact_power_tool = 0;
			}
			/* Take our best pick and calculate if we succeed.. */
			if (impact_power) {
				msg_print(Ind, "You strike the ground.");
#ifdef USE_SOUND_2010
				sound(Ind, "hit", "", SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				if (magik(impact_power)) impact = TRUE;
			} else if (impact_power_tool) {
				object_desc(0, o_name, o_ptr, TRUE, 256);
				msg_format(Ind, "You strike the ground with your %s.", o_name);
#ifdef USE_SOUND_2010
				sound(Ind, "hit_floor", "hit_blunt", SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				if (magik(impact_power_tool)) impact = TRUE;
			} else if (impact_power_weapon) {
				object_desc(0, o_name, o23_ptr, TRUE, 256);
				msg_format(Ind, "You strike the ground with your %s.", o_name);
#ifdef USE_SOUND_2010
				switch (o23_ptr->tval) {
				case TV_SWORD: sound(Ind, "hit_sword", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE); break;
				case TV_BLUNT:	if (o_ptr->sval == SV_WHIP) sound(Ind, "hit_whip", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE);
						else sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE);
						break;
				case TV_AXE: sound(Ind, "hit_axe", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE); break;
				case TV_POLEARM: sound(Ind, "hit_polearm", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE); break;
				case TV_MSTAFF: sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE); break;
				}
#endif
				if (magik(impact_power_weapon)) impact = TRUE;
			} else {
				/* Futile attempt as we don't have any earthquake powers at all -
				   anyway, just prioritize the digging tool then in this case just for picking the appropriate sfx =_= */
				if (o_ptr->k_idx && o_ptr->tval == TV_DIGGING) {
					object_desc(0, o_name, o_ptr, TRUE, 256);
					msg_format(Ind, "You strike the ground with your %s.", o_name);
#ifdef USE_SOUND_2010
					sound(Ind, "hit_floor", "hit_blunt", SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				} else if (o2_ptr->k_idx) {
					object_desc(0, o_name, o2_ptr, TRUE, 256);
					msg_format(Ind, "You strike the ground with your %s.", o_name);
#ifdef USE_SOUND_2010
					switch (o2_ptr->tval) {
					case TV_SWORD: sound(Ind, "hit_sword", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE); break;
					case TV_BLUNT:	if (o2_ptr->sval == SV_WHIP) sound(Ind, "hit_whip", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE);
							else sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE);
							break;
					case TV_AXE: sound(Ind, "hit_axe", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE); break;
					case TV_POLEARM: sound(Ind, "hit_polearm", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE); break;
					case TV_MSTAFF: sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE); break;
					}
#endif
				} else if (o3_ptr->k_idx && (is_weapon(o3_ptr->tval) || o3_ptr->tval == TV_MSTAFF)) {
					object_desc(0, o_name, o3_ptr, TRUE, 256);
					msg_format(Ind, "You strike the ground with your %s.", o_name);
#ifdef USE_SOUND_2010
					switch (o3_ptr->tval) {
					case TV_SWORD: sound(Ind, "hit_sword", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE); break;
					case TV_BLUNT:	if (o3_ptr->sval == SV_WHIP) sound(Ind, "hit_whip", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE);
							else sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE);
							break;
					case TV_AXE: sound(Ind, "hit_axe", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE); break;
					case TV_POLEARM: sound(Ind, "hit_polearm", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE); break;
					case TV_MSTAFF: sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_NO_OVERLAP, TRUE); break;
					}
#endif
				} else {
					msg_print(Ind, "You strike the ground.");
#ifdef USE_SOUND_2010
					sound(Ind, "hit", "", SFX_TYPE_NO_OVERLAP, TRUE);
#endif
				}
			}

			/* Finally, quake maybe! */
			if (impact && magik(QUAKE_CHANCE)) {
				//sound(Ind, NULL, NULL, SFX_TYPE_STOP, TRUE); /* Stop "hit_floor"/"tunnel_rock" sfx */
				earthquake(&p_ptr->wpos, p_ptr->py, p_ptr->px, 7);
			}
#ifdef EQUIPPABLE_DIGGERS
			if (swapped) {
				/* Unhack */
				object_storage = *o_ptr;
				*o_ptr = *o2_ptr;
				*o2_ptr = object_storage;
			}
#endif
			return;
		}

		msg_print(Ind, "You see nothing there to tunnel through.");
		disturb(Ind, 0, 0);
		p_ptr->energy -= level_speed(&p_ptr->wpos) / 3;
#ifdef EQUIPPABLE_DIGGERS
		if (swapped) {
			/* Unhack */
			object_storage = *o_ptr;
			*o_ptr = *o2_ptr;
			*o2_ptr = object_storage;
		}
#endif
		return;
	}


	/* Hack: Digging cancels Searching Mode */
	if (p_ptr->searching) {
		p_ptr->searching = FALSE;
		p_ptr->update |= PU_BONUS;
		p_ptr->redraw |= PR_STATE | PR_MAP;
	}

	/* --- Handle secret, permanent, monster, NO_TFORM - finally if we pass, deduce energy and stop our other actions --- */


	/* No tunnelling through doors */
	if ((f_ptr->flags1 & FF1_DOOR) && cfeat != FEAT_SECRET) {
		/* Try opening it instead */
		do_cmd_open(Ind, dir);
#ifdef EQUIPPABLE_DIGGERS
		if (swapped) {
			/* Unhack */
			object_storage = *o_ptr;
			*o_ptr = *o2_ptr;
			*o2_ptr = object_storage;
		}
#endif
		return;
	}

	/* Check that the material is actually tunnelable at all */
	//do_cmd_tunnel_test(int y, int x, FALSE)
	if (!(f_ptr->flags1 & FF1_TUNNELABLE) ||
	    (f_ptr->flags1 & FF1_PERMANENT)) {
		/* Message */
		msg_print(Ind, f_text + f_ptr->tunnel);
		/* Nope */
		p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;
#ifdef EQUIPPABLE_DIGGERS
		if (swapped) {
			/* Unhack */
			object_storage = *o_ptr;
			*o_ptr = *o2_ptr;
			*o2_ptr = object_storage;
		}
#endif
		return;
	}

	/* No destroying of town layouts */
	else if (!allow_terraforming(wpos, FEAT_TREE)) {
		msg_print(Ind, "You may not tunnel in this area.");
		p_ptr->energy -= level_speed(&p_ptr->wpos) / 3;
#ifdef EQUIPPABLE_DIGGERS
		if (swapped) {
			/* Unhack */
			object_storage = *o_ptr;
			*o_ptr = *o2_ptr;
			*o2_ptr = object_storage;
		}
#endif
		return;
	}

	if ((f_info[c_ptr->feat].flags2 & FF2_NO_TFORM) || (c_ptr->info & CAVE_NO_TFORM)) {
		msg_print(Ind, "This feature seems to be too solid.");
		p_ptr->energy -= level_speed(&p_ptr->wpos) / 3;
#ifdef EQUIPPABLE_DIGGERS
		if (swapped) {
			/* Unhack */
			object_storage = *o_ptr;
			*o_ptr = *o2_ptr;
			*o2_ptr = object_storage;
		}
#endif
		return;
	}

	/* A monster is in the way */
	if (c_ptr->m_idx > 0) {
		/* Take a turn */
		p_ptr->energy -= level_speed(&p_ptr->wpos);
		msg_print(Ind, "There is a monster in the way!");
		/* Attack */
		py_attack(Ind, y, x, TRUE);
#ifdef EQUIPPABLE_DIGGERS
		if (swapped) {
			/* Unhack */
			object_storage = *o_ptr;
			*o_ptr = *o2_ptr;
			*o2_ptr = object_storage;
		}
#endif
		return;
	}

	/* Okay, prepare to try digging */
	un_afk_idle(Ind);
	break_cloaking(Ind, 0);
	break_shadow_running(Ind);

	/* we abuse stop_precision() to reset current_create_sling_ammo, so need this hack here: */
	if (!p_ptr->current_create_sling_ammo) stop_precision(Ind);
	stop_shooting_till_kill(Ind);

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);


	/* --- Handle actual tunneling process --- */


	do_cmd_tunnel_aux(Ind, wpos, x, y, power, wood_power, fibre_power, quiet_borer, FALSE, &no_quake, &more, &door);


	/* --- Aftermath --- */


#ifdef USE_SOUND_2010
	/* If we successfully tunneled this grid, stop sfx */
	if (p_ptr->command_rep && p_ptr->command_rep != PKT_BASH && !more)
		sound(Ind, NULL, NULL, SFX_TYPE_STOP, TRUE);
#endif
	/* Notice "blockage" changes */
	if (old_floor != cave_floor_bold(zcave, y, x)) {
		/* Update some things */
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
	}

	/* Apply Earthquakes */
	if (!no_quake) { /* don't quake from digging rather soft stuff */
		if (!quiet_borer) {
			/* As we are digging and not just hitting the ground, pick our digging tool over our weapon.
			   As for weapons used as digging tools - these do not concern feats that are hard enough to cause quakes (but only fibre/wood/webs etc). */
			if (o_ptr->k_idx && o_ptr->tval == TV_DIGGING) {
				if (magik(impact_power_tool)) impact = TRUE;
			}
#if 0 /* no, weapons only really help with fiber and wood, which are 'soft' aka no_quake. This gets too crazy. */
			/* We are digging with a weapon? */
			else if (o2_ptr->k_idx || (o3_ptr->k_idx && (is_weapon(o3_ptr->tval) || o3_ptr->tval == TV_MSTAFF))) {
				if (impact_power_weapon2 > impact_power_weapon) impact_power_weapon = impact_power_weapon2;
				if (magik(impact_power_weapon)) impact = TRUE;
			}
#endif
			/* Bare-handed o_o */
			else if (magik(impact_power)) impact = TRUE;
		}
		/* Bare-handed o_o - will even trigger as KILL_WALL form as this impact is
		   intrinsic/automatically utilized as it is not from weapon nor digging tool. */
		else if (magik(impact_power)) impact = TRUE;

		/* Finally, quake maybe! */
		if (impact && magik(QUAKE_CHANCE)) {
			earthquake(&p_ptr->wpos, p_ptr->py, p_ptr->px, 7);
			more = FALSE;
		}
	}

	/* Cancel repetition unless we can continue */
	if (!more) {
		disturb(Ind, 0, 0);
		if (!door) {
			if (c_ptr->custom_lua_tunnel > 0) exec_lua(0, format("custom_tunnel(%d,%d)", Ind, c_ptr->custom_lua_tunnel));
			if (c_ptr->custom_lua_tunnel_hand > 0 && !quiet_borer) exec_lua(0, format("custom_tunnel_hand(%d,%d)", Ind, c_ptr->custom_lua_tunnel_hand));
		}
	} else if (p_ptr->always_repeat) p_ptr->command_rep = PKT_TUNNEL;

#ifdef EQUIPPABLE_DIGGERS
	if (swapped) {
		/* Unhack */
		object_storage = *o_ptr;
		*o_ptr = *o2_ptr;
		*o2_ptr = object_storage;
	}
#endif
}


/*
 * Try to identify a trap when disarming it.	- Jir -
 */
static void do_id_trap(int Ind, int t_idx) {
	player_type *p_ptr = Players[Ind];
	trap_kind *tr_ptr = &t_info[t_idx];
	int power;

	/* Already known? */
	if (p_ptr->trap_ident[t_idx]) return;

	/* Need proper skill */
	//if (get_skill(p_ptr, SKILL_DISARM) < 20) return;
	//if (!get_skill(p_ptr, SKILL_DISARM)) return;
	if (!get_skill(p_ptr, SKILL_TRAPPING)) return;

	/* Impossible? */
	if (tr_ptr->flags & FTRAP_NO_ID) return;
	if (UNAWARENESS(p_ptr) || no_lite(Ind)) return;

	/* Check for failure (hard) */
	power = (tr_ptr->difficulty + 2) * (tr_ptr->minlevel + 10) * 5;
	if (tr_ptr->flags & FTRAP_EASY_ID) power /= 10;

	//if (randint(power) > get_skill(p_ptr, SKILL_DISARM)) return;
	if (randint(power) > get_skill(p_ptr, SKILL_TRAPPING)) return;

	/* Good job */
	p_ptr->trap_ident[t_idx] = TRUE;
	/* Some traps seem unnamed atm - looks ugly */
	if (strlen(t_name + tr_ptr->name))
		msg_format(Ind, "You identified that trap as %s.", t_name + tr_ptr->name);

	/* Fair reward; so it's double exp when disarming AND
	   for the first time also IDing - C. Blue */
	if (!(p_ptr->mode & MODE_PVP)) gain_exp(Ind, TRAP_EXP(p_ptr, t_idx, getlevel(&p_ptr->wpos)));
}

/*
 * Disarms a trap, or chest     -RAK-
 */

/* A nice idea:
 * Disarming a trap will have a disarm_skill% chance of dropping a
 * random trap kit? =) - the_sandman
 */
void do_cmd_disarm(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;

	int                 y, x, i, j, power;

	cave_type               *c_ptr;
	object_type             *o_ptr;
	trap_kind *t_ptr;
	int t_idx = 0;

	bool more = FALSE, done = FALSE;
	cave_type **zcave;


	if (!(zcave = getcave(wpos))) return;

#ifdef ENABLE_OUNLIFE
	/* Wraithstep gets auto-cancelled on forced interaction with solid environment */
	if (p_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1)) set_tim_wraith(Ind, 0);
#endif

	/* Ghosts cannot disarm ; not in WRAITHFORM */
	if (CANNOT_OPERATE_SPECTRAL) {
		msg_print(Ind, "Without a material body you cannot disarm things!");
		if (!is_admin(p_ptr)) return;
	} //todo maybe: Add CANNOT_OPERATE_FORM check too?

	break_shadow_running(Ind);

	/* Get a direction (or abort) */
	if (dir) {
		struct c_special *cs_ptr;

		/* Get location */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];

		/* Get grid and contents */
		c_ptr = &zcave[y][x];

		/* Access the item */
		o_ptr = &o_list[c_ptr->o_idx];

		/* Access the trap */
		if ((cs_ptr = GetCS(c_ptr, CS_TRAPS)))
			t_idx = cs_ptr->sc.trap.t_idx;

		/* Nothing useful */
		if ((!t_idx || !cs_ptr->sc.trap.found) &&
		    o_ptr->tval != TV_CHEST &&
		    !(cs_ptr = GetCS(c_ptr, CS_MON_TRAP))
#ifdef ENABLE_DEMOLITIONIST
		    && !(o_ptr->tval == TV_CHARGE && o_ptr->timeout)
#endif
		    ) {
			/* Message */
			msg_print(Ind, "You see nothing there to disarm.");
			done = TRUE;
		}

		/* Monster in the way */
		else if (c_ptr->m_idx > 0) {
			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			/* Message */
			msg_print(Ind, "There is a monster in the way!");

			/* Attack */
			py_attack(Ind, y, x, TRUE);

			done = TRUE;
		}

#ifdef ENABLE_DEMOLITIONIST
		else if (o_ptr->tval == TV_CHARGE && o_ptr->timeout) {
			if (!magik(100 + k_info[o_ptr->k_idx].level - 80)) {
				msg_print(Ind, "You extinguish the fuse!");
				o_ptr->timeout = 0;
				everyone_lite_spot(wpos, y, x);
 #ifdef USE_SOUND_2010
				sound_near_site(p_ptr->py, p_ptr->px, wpos, 0, "item_rune", NULL, SFX_TYPE_MISC, FALSE);
 #endif
			} else msg_print(Ind, "\377yYou fail to extinguish the fuse!"); //TODO: Add 'more = TRUE' functionality
			disturb(Ind, 0, 0);

			if (o_ptr->custom_lua_usage) exec_lua(0, format("custom_object_usage(%d,%d,%d,%d,%d)", Ind, c_ptr->o_idx, 0, 8, o_ptr->custom_lua_usage));
			return;
		}
#endif

		/* Normal disarm */
		else if (o_ptr->tval == TV_CHEST) {
			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			/* Must find the trap first. */
			if (!object_known_p(Ind, o_ptr)) msg_print(Ind, "I don't see any traps.");
			/* Already disarmed/unlocked */
			else if (o_ptr->pval <= 0) msg_print(Ind, "The chest is not trapped.");
			/* Success (get a lot of experience) */
			else {
				t_ptr = &t_info[o_ptr->pval];

				/* Disarm the chest */
				//more = do_cmd_disarm_chest(y, x, c_ptr->o_idx);

				/* Get the "disarm" factor */
				i = p_ptr->skill_dis;

				/* Penalize some conditions */
				if (p_ptr->blind || no_lite(Ind)) i = i / 10;
				if (p_ptr->confused || p_ptr->image) i = i / 10;

				/* Extract the difficulty */
				j = i - t_ptr->difficulty * 3;

				/* Always have a small chance of success */
				if (j < 2) j = 2;

				/* S(he) is no longer afk */
				un_afk_idle(Ind);
				p_ptr->warning_trap = 1;

				if (rand_int(100) < j) {
#ifdef USE_SOUND_2010
					sound(Ind, "disarm", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
					if (!(p_ptr->mode & MODE_PVP)) {
#ifdef SHOW_XP_GAIN
						gain_exp_onhold(Ind, TRAP_EXP(p_ptr, o_ptr->pval, getlevel(&p_ptr->wpos)));
						msg_format(Ind, "You have disarmed the chest. (%d XP)", p_ptr->gain_exp);
						apply_exp(Ind);
#else
						msg_print(Ind, "You have disarmed the chest.");
						gain_exp(Ind, TRAP_EXP(p_ptr, o_ptr->pval, getlevel(&p_ptr->wpos)));
#endif
					} else msg_print(Ind, "You have disarmed the chest.");

					do_id_trap(Ind, o_ptr->pval);

					/* Actually disarm it */
					o_ptr->pval = (0 - o_ptr->pval);

					done = TRUE;

					if (o_ptr->custom_lua_usage) exec_lua(0, format("custom_object_usage(%d,%d,%d,%d,%d)", Ind, c_ptr->o_idx, 0, 9, o_ptr->custom_lua_usage));
				}
				/* Failure -- Keep trying */
				else if ((i > 5) && (randint(i) > 5)) {
					/* We may keep trying */
					more = TRUE;
					done = TRUE;
					msg_print(Ind, "You failed to disarm the chest.");

					break_shadow_running(Ind);
					stop_precision(Ind);
					stop_shooting_till_kill(Ind);

					if (o_ptr->custom_lua_usage) exec_lua(0, format("custom_object_usage(%d,%d,%d,%d,%d)", Ind, c_ptr->o_idx, 0, 8, o_ptr->custom_lua_usage));
				}
				/* Failure -- Set off the trap */
				else {
					/* Paranoia to have a separate 'if' for this - at this point, the chest must be validly trapped */
					if (chest_trap(Ind, y, x, c_ptr->o_idx)) {
						msg_print(Ind, "You set off a trap!");
						if ((o_ptr->xtra3 & 0x1) || /* Erase chest whenever the trap was set off */
						    (o_ptr->sval == SV_CHEST_RUINED && (o_ptr->xtra3 & 0x2))) /* Erase the chest if it got ruined by the trap */
							delete_object_idx(c_ptr->o_idx, FALSE, FALSE);

						break_cloaking(Ind, 0);
						break_shadow_running(Ind);
						stop_precision(Ind);
						stop_shooting_till_kill(Ind);

						if (o_ptr->custom_lua_usage) exec_lua(0, format("custom_object_usage(%d,%d,%d,%d,%d)", Ind, c_ptr->o_idx, 0, 8, o_ptr->custom_lua_usage));
					}
					done = TRUE;
				}
			}

			/* XXX hrm it's ugly */
			/* We didn't spot any trap on the chest/it wasn't trapped, and there is also no other trap around to disarm? Then we're done. */
			if ((!t_idx || !cs_ptr->sc.trap.found) &&
			    !(cs_ptr = GetCS(c_ptr, CS_MON_TRAP)))
				done = TRUE;
		}

		/* Disarm the trap */
		/*else if (c_ptr->feat == FEAT_MON_TRAP) */
		if (!done && cs_ptr->type == CS_MON_TRAP) { /* same thing.. */
			/* S(he) is no longer afk */
			un_afk_idle(Ind);

			/* Take half a turn, otherwise it gets a bit tedious game-flow wise.. */
			if (get_skill(p_ptr, SKILL_TRAPPING) >= cs_ptr->sc.montrap.difficulty)
				p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;
			else
				p_ptr->energy -= level_speed(&p_ptr->wpos);

			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);

#ifdef ENABLE_DEMOLITIONIST
			/* We abuse montraps for this atm, need to interject here *cough* */
			if (cs_ptr->sc.montrap.difficulty >= 100) { //100+ = hack marker for blast charge
				if (!magik(cs_ptr->sc.montrap.difficulty - 80)) {
					msg_print(Ind, "You extinguish the fuse!");
 #ifdef USE_SOUND_2010
					sound_near_site(p_ptr->py, p_ptr->px, wpos, 0, "item_rune", NULL, SFX_TYPE_MISC, FALSE);
 #endif
					do_cmd_disarm_mon_trap_aux(Ind, wpos, y, x);
					//if (o_ptr->custom_lua_usage) exec_lua(0, format("custom_object_usage(%d,%d,%d,%d,%d)", Ind, c_ptr->o_idx, 0, 9, o_ptr->custom_lua_usage));
				} else {
					msg_print(Ind, "\377yYou fail to extinguish the fuse!");
					//TODO: Add 'more = TRUE' functionality
					//if (o_ptr->custom_lua_usage) exec_lua(0, format("custom_object_usage(%d,%d,%d,%d,%d)", Ind, c_ptr->o_idx, 0, 8, o_ptr->custom_lua_usage));
				}
				disturb(Ind, 0, 0);
				return;
			}
#endif

			msg_print(Ind, "You disarm the monster trap.");
#ifdef USE_SOUND_2010
			sound(Ind, "disarm", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
			do_cmd_disarm_mon_trap_aux(Ind, wpos, y, x);
			more = FALSE;
			done = TRUE;

			if (!p_ptr->warning_edmt) {
				if (!is_newer_than(&p_ptr->version, 4, 7, 3, 0, 0, 0))
					msg_print(Ind, "\374\377yHINT: Look into command \377o/edmt\377y for easier mass-disarming of monster traps.");
				else
					msg_print(Ind, "\374\377yHINT: Look into command \377o/edmt\377y and option \377oeasy_disarm_montraps\377y (in '=2') for easier mass-disarming of monster traps.");
				s_printf("warning_edmt: %s\n", p_ptr->name);
				p_ptr->warning_edmt = 1;
			}

			//if (o_ptr->custom_lua_usage) exec_lua(0, format("custom_object_usage(%d,%d,%d,%d,%d)", Ind, c_ptr->o_idx, 0, 9, o_ptr->custom_lua_usage));
		}

		/* Disarm a trap */
		if (!done && (t_idx && cs_ptr->sc.trap.found)) {
			cptr name;

			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);

			/* Access trap name */
			if (p_ptr->trap_ident[t_idx])
				name = (t_name + t_info[t_idx].name);
			else
				name = "unknown trap";

			//cptr name = (f_name + f_info[c_ptr->feat].name);

			/* S(he) is no longer afk */
			un_afk_idle(Ind);

			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			/* Get the "disarm" factor */
			i = p_ptr->skill_dis;

			/* Penalize some conditions */
			if (p_ptr->blind || no_lite(Ind)) i = i / 10;
			if (p_ptr->confused || p_ptr->image) i = i / 10;

			/* XXX XXX XXX Variable power? */

			/* Extract trap "power" */
			power = t_info[t_idx].difficulty * 3;
			//power = 5;

			/* Extract the difficulty */
			j = i - power;

			/* Always have a small chance of success */
			if (j < 2) j = 2;

			/* Success */
			if (rand_int(100) < j) {
				/* A chance to drop a trapkit; equal to the trapping skill */
				int sdis = (int)(p_ptr->s_info[SKILL_TRAPPING].value / 1000);

#ifdef USE_SOUND_2010
				sound(Ind, "disarm", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
				/* Traps of missing money can drop some of their stolen cash ;) */
				if (t_idx == TRAP_OF_MISSING_MONEY && rand_int(4))
					place_gold(Ind, &p_ptr->wpos, y, x, 10, 0);//rand_int(getlevel(&p_ptr->wpos) * getlevel(&p_ptr->wpos) / 2));
					//NOTE: In theory this can be abused to transfer gold cross-mode/to soloists even, but the amount is negligible.

				/* Reward */
				if (!(p_ptr->mode & MODE_PVP)) {
#ifdef SHOW_XP_GAIN
					gain_exp_onhold(Ind, (TRAP_EXP(p_ptr, t_idx, getlevel(&p_ptr->wpos)) * (MAX_CLONE_TRAPPING - cs_ptr->sc.trap.clone)) / MAX_CLONE_TRAPPING);
					msg_format(Ind, "You have disarmed the %s. (%d XP)", name, p_ptr->gain_exp);
					apply_exp(Ind);
#else
					msg_format(Ind, "You have disarmed the %s.", name);
					gain_exp(Ind, (TRAP_EXP(p_ptr, t_idx, getlevel(&p_ptr->wpos)) * (MAX_CLONE_TRAPPING - cs_ptr->sc.trap.clone)) / MAX_CLONE_TRAPPING);
#endif
				} else msg_format(Ind, "You have disarmed the %s.", name);

				/* Try to identify it */
				do_id_trap(Ind, t_idx);

				if (magik(sdis)) {
					object_type forge;
					object_type* yay = &forge;

					invcopy(yay, lookup_kind(TV_TRAPKIT, randint(6)));

					/* Let's make it so that there is always a chance for an awesome trap--
					   more so the better you are at trapping (from 1% ... 6%).
					   Since the trap appears ~50% of the time at max trapping,
					   the total chance for this is about 3% */
					if (magik((sdis / 10) + 1)) {
						apply_magic(&p_ptr->wpos, yay, -2, TRUE, TRUE, TRUE, FALSE, make_resf(p_ptr));
						drop_near(TRUE, 0, yay, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);
						msg_print(Ind, "You have created a wonderful trapkit using pieces of the disarmed trap.");
					} else {
						apply_magic(&p_ptr->wpos, yay, -2, TRUE, FALSE, FALSE, FALSE, make_resf(p_ptr));
						drop_near(TRUE, 0, yay, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);
						msg_print(Ind, "You have fashioned a trapkit of a sort from the disarmed trap.");
					}
				}

				/* Remove the trap */
				cs_erase(c_ptr, cs_ptr);
				//c_ptr->feat = FEAT_FLOOR;

				/* Forget the "field mark" */
				everyone_forget_spot(wpos, y, x);
				/* Notice */
				note_spot_depth(wpos, y, x);
				/* Redisplay the grid */
				//everyone_clear_ovl_spot(wpos, y, x);
				everyone_lite_spot(wpos, y, x);

				/* move the player onto the trap grid
				   except if it was a trap on a closed door or mimicked feature,
				   since we might not want to open it yet,
				   especially while being cloaked */
				/* NOTE that the player might normally NOT be able to move onto the trap grid! (bats/forms that can't open doors) -- seems safe for now */
				if (dir != 5
				    && !(c_ptr->feat >= FEAT_DOOR_HEAD && c_ptr->feat <= FEAT_DOOR_TAIL)
				    && c_ptr->feat != FEAT_SECRET /* Don't move into secret doors that haven't been found out yet... TODO: Check FEAT_SECRET vs CS_MIMIC usage for granite/mountains. */
				    && !GetCS(c_ptr, CS_MIMIC)) /* This is for secret doors in mountains, as FEAT_SECRET is specifically secret door inside granite wall? */
					move_player(Ind, dir, FALSE, NULL);
				else
					everyone_lite_spot(wpos, y, x);
			}

			/* Failure -- Keep trying */
			else if ((i > 5) && (randint(i) > 5)) {
				break_shadow_running(Ind);
				stop_precision(Ind);
				stop_shooting_till_kill(Ind);

				/* Message */
				msg_format(Ind, "You failed to disarm the %s.", name);

				/* We may keep trying */
				more = TRUE;
			}

			/* Failure -- Set off the trap */
			else {
				/* Message */
				msg_format(Ind, "You set off the %s!", name);

#if 0 /* the problem is that the player might NOT be able to move onto the trap grid! (bats/forms that can't open doors) */
				/* Move the player onto the trap */
				if (dir != 5) move_player(Ind, dir, FALSE, NULL); /* moving doesn't 100% imply setting it off */
				else hit_trap(Ind); /* but we can allow this weakness, assuming that you are less likely to get hit if you stand besides the trap instead of right on it */
#else /* so just hit him without trying to move him. Probably makes more sense anyway. */
				//hit_trap(Ind); -- does not work because we're not standing on the grid
				player_activate_door_trap(Ind, y, x);
#endif
				break_cloaking(Ind, 0);
				break_shadow_running(Ind);
				stop_precision(Ind);
				stop_shooting_till_kill(Ind);
			}

			p_ptr->warning_trap = 1;
			/* only aesthetic */
			done = TRUE;
		}
	}

	/* Cancel repeat unless told not to */
	if (!more) disturb(Ind, 0, 0);
	else if (p_ptr->always_repeat) p_ptr->command_rep = PKT_DISARM;
}


/*
 * Bash open a door, success based on character strength
 *
 * For a closed door, pval is positive if locked; negative if stuck.
 *
 * For an open door, pval is positive for a broken door.
 *
 * A closed door can be opened - harder if locked. Any door might be
 * bashed open (and thereby broken). Bashing a door is (potentially)
 * faster! You move into the door way. To open a stuck door, it must
 * be bashed. A closed door can be jammed (see do_cmd_spike()).
 *
 * Breatures can also open or bash doors, see elsewhere.
 *
 * We need to use character body weight for something, or else we need
 * to no longer give female characters extra starting gold.
 */
void do_cmd_bash(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	monster_race *r_ptr = &r_info[p_ptr->body_monster];

	int y, x;
	int bash, temp, num;
	cave_type *c_ptr;
	bool more = FALSE, water = FALSE;
	char bash_type = 1;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return;

#ifdef ENABLE_OUNLIFE
	/* Wraithstep gets auto-cancelled on forced interaction with solid environment */
	if (p_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1)) set_tim_wraith(Ind, 0);
#endif

	/* Ghosts cannot bash ; not in WRAITHFORM */
	if (CANNOT_OPERATE_SPECTRAL) {
		/* Message */
		msg_print(Ind, "You cannot bash things without a material body!");
		if (!is_admin(p_ptr)) return;
	}

	/* New '+' feat in 4.4.6.2 */
	if (dir == 11) {
		get_aim_dir(Ind);
		p_ptr->current_bash = 1;
		return;
	}

	if ((p_ptr->fruit_bat && !p_ptr->body_monster) ||
	    (p_ptr->body_monster && !(r_ptr->flags2 & RF2_BASH_DOOR)))
		bash_type = 2;

	/* Get a "repeated" direction */
	if (dir) {
		/* Check for "target request" and valid _melee range_ target */
		if (dir == 5 && target_okay(Ind) && distance(p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col)) {
			x = p_ptr->target_col;
			y = p_ptr->target_row;
		}
		/* hack: bashing onto our grid while not having a valid target causes random direction of distance 1 (this is a QoL hack for looting piles) */
		else if (dir == 5) {
			dir = randint(8);
			if (dir == 5) dir = 9;

			bash_type = 3;

			/* Bash location */
			y = p_ptr->py;
			x = p_ptr->px;
		}
		/* Bash into a user-specified direction */
		else {
			/* Bash location */
			y = p_ptr->py + ddy[dir];
			x = p_ptr->px + ddx[dir];
		}

		/* Get grid */
		c_ptr = &zcave[y][x];

		/* for leaderless guild houses */
		if ((zcave[y][x].info2 & CAVE2_GUILD_SUS)) return;

		if (feat_is_water(c_ptr->feat) || feat_is_lava(c_ptr->feat)) {
			if (bash_type != 3) bash_type = 2;
			water = TRUE;
		}

		/* Monster in the way */
		if (c_ptr->m_idx > 0) {
#if 0 /* normal way */
			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);
			/* Attack with all BpR */
			py_attack(Ind, y, x, TRUE);
#else /* new 2022 */
			py_bash_mon(Ind, y, x);
#endif
		}
		/* Player in the way */
		else if (c_ptr->m_idx < 0
#ifndef FRIENDLY_BASH /* We can only bash someone when we're hostile? (In which case it will turn into the 'Bash' fighting technique.) */
		    && cfg.use_pk_rules != PK_RULES_NEVER && check_hostile(Ind, -c_ptr->m_idx)
#endif
		    )
			py_bash_py(Ind, y, x);
		else if (c_ptr->feat == FEAT_GRAND_MIRROR) {
			int x2, y2, i;

			p_ptr->energy -= level_speed(&p_ptr->wpos);
			cave_set_feat_live(&p_ptr->wpos, y, x, FEAT_SHATTERED_MIRROR);
			msg_print(Ind, "You bash the mirror and it shatters into a thousand pieces.");
			msg_print(Ind, " But the image of yourself that you saw in it.. is still there!");

			p_ptr->suspended = turn + cfg.fps * 3; //short break to grasp the situation ^^
			p_ptr->redraw |= PR_STATE;
#ifdef USE_SOUND_2010
			//sound(Ind, "shatter_potion", NULL, SFX_TYPE_MISC, TRUE);
			sound_floor_vol(wpos, "shatter_potion", NULL, SFX_TYPE_MISC, 100); //^^'
			if (is_atleast(&p_ptr->version, 4, 7, 4, 4, 0, 0)) {
				sound_floor_vol(wpos, "destruction", NULL, SFX_TYPE_MISC, 100);
				Send_screenflash(Ind);
			} else
				sound_floor_vol(wpos, "thunder", NULL, SFX_TYPE_AMBIENT, 100); //ambient, for implied lightning visuals
#endif
			summon_override_checks = SO_ALL;
			temp = 50;
			do scatter(wpos, &y2, &x2, y, x, 1, TRUE);
			while (!(in_bounds(y2, x2) && cave_empty_bold(zcave, y2, x2) && distance(y2, x2, p_ptr->py, p_ptr->px) > 1) && --temp);
			if (!place_monster_one(&p_ptr->wpos, y2, x2, RI_MIRROR, 0, 0, 0, 0, 0)) {
				/* Success */
				monster_type *m_ptr;

				/* Modify starting stats initially: */
				if ((temp = zcave[y2][x2].m_idx) > 0) {
					m_ptr = &m_list[temp];
					if (m_ptr->r_idx == RI_MIRROR) { /* Extra paranoia.. */
						/* Init monster template with fixed properties: */
						py2mon_init();
						m_ptr->related = p_ptr->id; /* imprint player on the mirror image ;) */
						m_ptr->related_type = 0;
						/* Main problem: Player could init the mirror with a low profile char and then switch places with a high profile char,
						   so even the 'immutable' stats might need adjusting, as the opponent character changes completely.
						   Maybe 'last_target' can be utilized to detect opponents switching.
						   So we do init base stats here, but we will need to keep updating them on the fly all the time: */
						py2mon_init_base(m_ptr, p_ptr);
					}
				} else s_printf("MIRROR misplaced for '%s' (%d)!\n", p_ptr->name, Ind); //paranoia?
			} else s_printf("MIRROR placement failed for '%s' (%d)!\n", p_ptr->name, Ind); //paranoia?
			summon_override_checks = SO_NONE;

			for (i = 1; i <= NumPlayers; i++) {
				player_type *q_ptr = Players[i];

				/* Skip disconnected players */
				if (q_ptr->conn == NOT_CONNECTED) continue;
				/* Skip players not on this depth */
				if (!inarea(&q_ptr->wpos, wpos)) continue;

				Send_music(Ind, -4, -4, -4);
			}
		}
		/* Nothing useful */
		else if (!((c_ptr->feat >= FEAT_DOOR_HEAD) &&
		    (c_ptr->feat <= FEAT_DOOR_TAIL)) &&
		    c_ptr->feat != FEAT_HOME) {
			int item;

			/* Hack -- 'kick' an item ala NetHack */
			if ((item = c_ptr->o_idx)) {
				object_type *o_ptr = &o_list[c_ptr->o_idx];

				if (nothing_test(o_ptr, p_ptr, &p_ptr->wpos, x, y, 11)) return; //was 2

				if (o_ptr->questor) {
					msg_print(Ind, "\377yThe item doesn't move an inch!");
					return;
				}

#if 0 /* allow water-bashing of 1 grid range, same as for fruit bats now - C. Blue */
				if (water) {
					/* S(he) is no longer afk */
					un_afk_idle(Ind);

					/* Take a turn */
					p_ptr->energy -= level_speed(&p_ptr->wpos);

					msg_print(Ind, "Splash!");
					break_cloaking(Ind, 0);
					break_shadow_running(Ind);
					stop_precision(Ind);
					stop_shooting_till_kill(Ind);
					bandage_fails(Ind);
					return;
				}
#endif

				/* Potions smash open */
				if (o_ptr->tval == TV_POTION ||
				    o_ptr->tval == TV_POTION2 ||
				    o_ptr->tval == TV_FLASK ||
				    o_ptr->tval == TV_BOTTLE ||
				    (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT && (o_ptr->xtra3 & 0x0002))) {
					char o_name[ONAME_LEN];

					object_desc(Ind, o_name, o_ptr, FALSE, 3);

					/* S(he) is no longer afk */
					un_afk_idle(Ind);

					/* Take a turn */
					p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

					/* Message */
					/* TODO: handle blindness */
					msg_format_near_site(y, x, wpos, 0, TRUE, "The %s shatters!", o_name);
#ifdef USE_SOUND_2010
					sound_near_site(y, x, wpos, 0, "shatter_potion", NULL, SFX_TYPE_MISC, FALSE);
#endif

					//if (potion_smash_effect(0, wpos, y, x, o_ptr->sval))
					if (k_info[o_ptr->k_idx].tval == TV_POTION)
						/* This should harm the player too, but for now no way :/ */
						potion_smash_effect(0 - Ind, wpos, y, x, o_ptr->sval);
					else if (k_info[o_ptr->k_idx].tval == TV_FLASK)
						potion_smash_effect(0 - Ind, wpos, y, x, o_ptr->sval + 200);

					num = 1;//o_ptr->number;
					floor_item_increase(item, -num);
					floor_item_optimize(item);

					break_cloaking(Ind, 0);
					break_shadow_running(Ind);
					stop_precision(Ind);
					stop_shooting_till_kill(Ind);
					bandage_fails(Ind);
					return;
				}

				if (water) msg_print(Ind, "Splash!");
				do_cmd_throw(Ind, dir, 0 - c_ptr->o_idx, bash_type);

				/* Set off trap (Hack -- handle like door trap) */
				if (GetCS(c_ptr, CS_TRAPS)) player_activate_door_trap(Ind, y, x);

				return;
			}
			else
			/* Message */
			msg_print(Ind, "You see nothing there to bash.");
		}

		/* Bash a closed door */
		else {
			if ((p_ptr->fruit_bat && !p_ptr->body_monster) ||
			    (p_ptr->body_monster && !(r_ptr->flags2 & RF2_BASH_DOOR))) {
				msg_print(Ind, "You cannot bash doors!");
				if (!is_admin(p_ptr)) return;
			}

			/* S(he) is no longer afk */
			un_afk_idle(Ind);

			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			/* Message */
			msg_print(Ind, "You smash into the door!");

			/* Hack -- Bash power based on strength */
			/* (Ranges from 3 to 20 to 100 to 200, +10 on avg. from +1 STR) */
			bash = adj_str_blow[p_ptr->stat_ind[A_STR]];

			/* Extract door power */
			temp = ((c_ptr->feat - FEAT_DOOR_HEAD) & 0x07);

			/* Compare bash power to door power XXX XXX XXX */
			temp = (bash - (temp * 10));

			/* Hack -- always have a chance */
			if (temp < 1) temp = 1;

			/* Hack -- attempt to bash down the door */
			if (rand_int(100) < temp && c_ptr->feat != FEAT_HOME) {
				/* Message */
				msg_print(Ind, "The door crashes open!");

#ifdef USE_SOUND_2010
				sound(Ind, "bash_door_break", NULL, SFX_TYPE_COMMAND, TRUE);
#endif

				/* reduce sleep of nearby monsters */
				wakeup_monsters_somewhat(Ind, -1);

				/* Set off trap */
				if (GetCS(c_ptr, CS_TRAPS)) player_activate_door_trap(Ind, y, x);

				/* Break down the door */
				if (magik(DOOR_BASH_BREAKAGE)) c_ptr->feat = FEAT_BROKEN;
				/* Open the door */
				else c_ptr->feat = FEAT_OPEN;

				/* Notice */
				note_spot_depth(wpos, y, x);

				/* Redraw */
				everyone_lite_spot(wpos, y, x);

				/* Hack -- Fall through the door */
				move_player(Ind, dir, FALSE, NULL);

				/* Update some things */
				p_ptr->update |= (PU_VIEW | PU_LITE);
				p_ptr->update |= (PU_DISTANCE);
			}

			/* Saving throw against stun */
			else if (rand_int(100) < adj_dex_safe[p_ptr->stat_ind[A_DEX]] + p_ptr->lev) {
				/* Message */
				msg_print(Ind, "The door holds firm.");

#ifdef USE_SOUND_2010
				sound(Ind, "bash_door_hold", NULL, SFX_TYPE_COMMAND, TRUE);
#endif

				/* Allow repeated bashing */
				more = TRUE;
			}

			/* High dexterity yields coolness */
			else {
				/* Message */
				msg_print(Ind, "You are off-balance.");

#ifdef USE_SOUND_2010
				sound(Ind, "bash_door_hold", NULL, SFX_TYPE_COMMAND, TRUE);
#endif

				/* Hack -- Lose balance ala paralysis */
				(void)set_paralyzed(Ind, p_ptr->paralyzed + 2 + rand_int(2));
			}

			break_cloaking(Ind, 0);
			break_shadow_running(Ind);
			stop_precision(Ind);
			stop_shooting_till_kill(Ind);
			bandage_fails(Ind);
		}
	}

	/* Unless valid action taken, cancel bash */
	if (!more) disturb(Ind, 0, 0);
	else if (p_ptr->always_repeat) p_ptr->command_rep = PKT_BASH;
}



/*
 * Find the index of some "spikes", if possible.
 *
 * XXX XXX XXX Let user choose a pile of spikes, perhaps?
 */
/* Now this can be used for any tvals.	- Jir - */
//static bool get_spike(int Ind, int *ip)
bool get_something_tval(int Ind, int tval, int *ip) {
	player_type *p_ptr = Players[Ind];
	int i;
	object_type *o_ptr;
#ifdef ENABLE_SUBINVEN
	int k;
	object_type *s_ptr;
#endif

	/* Check every item in the pack */
	for (i = 0; i < INVEN_PACK; i++) {
		o_ptr = &(p_ptr->inventory[i]);

		/* Check the "tval" code */
		if (o_ptr->tval == tval) {
			if (!can_use_admin(Ind, o_ptr)) continue;

			/* Save the spike index */
			(*ip) = i;

			/* Success */
			return(TRUE);
		}

#ifdef ENABLE_SUBINVEN
		if (o_ptr->tval == TV_SUBINVEN) {
			for (k = 0; k < o_ptr->bpval; k++) { /* bpval is subinven size */
				s_ptr = &p_ptr->subinventory[i][k];
				if (!s_ptr->tval) break;

				/* Check the "tval" code */
				if (s_ptr->tval == tval) {
					if (!can_use_admin(Ind, s_ptr)) continue;

					/* Save the spike index */
					(*ip) = (i + 1) * SUBINVEN_INVEN_MUL + k;

					/* Success */
					return(TRUE);
				}
			}
		}
#endif
	}

	/* Oops */
	return(FALSE);
}



/*
 * Jam a closed door with a spike
 *
 * This command may NOT be repeated
 */
void do_cmd_spike(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;

	int y, x, item;

	cave_type *c_ptr;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return;

#ifdef ENABLE_OUNLIFE
	/* Wraithstep gets auto-cancelled on forced interaction with solid environment */
	if (p_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1)) set_tim_wraith(Ind, 0);
#endif

	/* Ghosts cannot spike ; not in WRAITHFORM */
	if (CANNOT_OPERATE_SPECTRAL && !is_admin(p_ptr)) {
		/* Message */
		msg_print(Ind, "You cannot spike doors without a material body!");
		return;
	}

	/* Get a "repeated" direction */
	if (dir) {
		/* Get location */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];

		/* Get grid and contents */
		c_ptr = &zcave[y][x];

		/* Require closed door */
		if (!((c_ptr->feat >= FEAT_DOOR_HEAD) &&
		      (c_ptr->feat <= FEAT_DOOR_TAIL))) {
			/* Message */
			msg_print(Ind, "You see nothing there to spike.");
		}

		/* Get a spike */
//		else if (!get_spike(Ind, &item))
		else if (!get_something_tval(Ind, TV_SPIKE, &item)) {
			/* Message */
			msg_print(Ind, "You have no spikes!");
		}

		/* Is a monster in the way? */
		else if (c_ptr->m_idx > 0) {
			/* S(he) is no longer afk */
			un_afk_idle(Ind);

			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			/* Message */
			msg_print(Ind, "There is a monster in the way!");

			/* Attack */
			py_attack(Ind, y, x, TRUE);
		}

		/* Go for it */
		else {
			/* Take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			/* Successful jamming */
			msg_print(Ind, "You jam the door with a spike.");

			/* Set off trap */
			if (GetCS(c_ptr, CS_TRAPS)) player_activate_door_trap(Ind, y, x);

			/* Convert "locked" to "stuck" XXX XXX XXX */
			if (c_ptr->feat < FEAT_DOOR_HEAD + 0x08) c_ptr->feat += 0x08;

			/* Add one spike to the door */
			if (c_ptr->feat < FEAT_DOOR_TAIL) c_ptr->feat++;

			/* Use up, and describe, a single spike, from the bottom */
			inven_item_increase(Ind, item, -1);
			inven_item_describe(Ind, item);
			inven_item_optimize(Ind, item);
		}
	}
}



/*
 * Support code for the "Walk" and "Jump" commands
 */
void do_cmd_walk(int Ind, int dir, int pickup) {
	player_type *p_ptr = Players[Ind];
	cave_type *c_ptr;

	bool more = FALSE;
	cave_type **zcave;

	if (!(zcave = getcave(&p_ptr->wpos))) return;

	if (!p_ptr->warning_numpadmove &&
	    (dir == 1 || dir == 3 || dir == 7 || dir == 9))
		p_ptr->warning_numpadmove = 1;

	/* Make sure he hasn't just switched levels */
	if (p_ptr->new_level_flag) return;

	/* Get a "repeated" direction */
	if (dir) {
		char consume_full_energy;

		/* Hack -- handle confusion */
		if (p_ptr->confused) {
			int tries = 10;
			dir = 5;

			/* Prevent walking nowhere */
			while (dir == 5 && --tries)
				dir = rand_int(9) + 1;
		}

		if (p_ptr->steamblast && dir != 5) {
			c_ptr = &zcave[p_ptr->py + ddy[dir]][p_ptr->px + ddx[dir]];
			if (c_ptr->feat >= FEAT_DOOR_HEAD && c_ptr->feat <= FEAT_DOOR_TAIL) {
#ifdef ENABLE_OUNLIFE
				/* Wraithstep gets auto-cancelled on forced interaction with solid environment */
				if (p_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1)) set_tim_wraith(Ind, 0);
#endif
				if (!CANNOT_OPERATE_SPECTRAL && !CANNOT_OPERATE_FORM) {
					do_steamblast(Ind, p_ptr->px + ddx[dir], p_ptr->py + ddy[dir], TRUE);
					return;
				}
			}
		}

		/* Handle confinement */
		if (p_ptr->stopped && dir != 5) {
			/* Try to break the rune */
			if (rand_int(200) < p_ptr->lev) {
				msg_print(Ind, "You break the rune!");
				set_stopped(Ind, 0);
			} else {
				p_ptr->energy -= level_speed(&p_ptr->wpos);
				msg_print(Ind, "You fail to break the confinement rune!");
				return;
			}
		}

		/* Handle the cfg.door_bump_open option */
		if (cfg.door_bump_open) {
			struct c_special *cs_ptr;

			/* Get requested grid */
			c_ptr = &zcave[p_ptr->py + ddy[dir]][p_ptr->px + ddx[dir]];

			/* This should be cfg.trap_bump_disarm? */
			if ((cfg.door_bump_open & BUMP_OPEN_TRAP) &&
			    p_ptr->easy_disarm &&
			    (cs_ptr = GetCS(c_ptr, CS_TRAPS)) &&
			    cs_ptr->sc.trap.found &&
			    !c_ptr->o_idx &&
			    !c_ptr->m_idx &&
			    !UNAWARENESS(p_ptr) &&
			    !no_lite(Ind) )
			{
				do_cmd_disarm(Ind, dir);
				return;
			}

			if ((cfg.door_bump_open & BUMP_OPEN_DOOR) &&
			    p_ptr->easy_open &&
			    (c_ptr->feat >= FEAT_DOOR_HEAD) &&
			    (c_ptr->feat <= FEAT_DOOR_TAIL)) {
#ifdef ENABLE_OUNLIFE
				/* Wraithstep gets auto-cancelled on forced interaction with solid environment */
				if (p_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1)) set_tim_wraith(Ind, 0);
#endif
				if (!CANNOT_OPERATE_SPECTRAL && !CANNOT_OPERATE_FORM) { /* players in WRAITHFORM can't open doors - mikaelh */
					do_cmd_open(Ind, dir);
					return;
				}
			}
			else if ((cfg.door_bump_open & BUMP_OPEN_DOOR) &&
			    p_ptr->easy_open &&
			    (c_ptr->feat == FEAT_HOME_HEAD)) {
				if ((cs_ptr = GetCS(c_ptr, CS_DNADOOR))) { /* orig house failure */
					if ((!(cfg.door_bump_open & BUMP_OPEN_HOUSE) ||
					    !access_door(Ind, cs_ptr->sc.ptr, FALSE)) &&
					    !admin_p(Ind)) {
#ifdef ENABLE_OUNLIFE
						/* Wraithstep gets auto-cancelled on forced interaction with solid environment */
						if (p_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1)) set_tim_wraith(Ind, 0);
#endif
						if (!CANNOT_OPERATE_SPECTRAL && !CANNOT_OPERATE_FORM) { /* players in WRAITHFORM can't open doors - mikaelh */
							do_cmd_open(Ind, dir);
							return;
						}
					}
				}
			}
		}

		if (p_ptr->easy_disarm_montraps) {
			/* Get requested grid */
			c_ptr = &zcave[p_ptr->py + ddy[dir]][p_ptr->px + ddx[dir]];

			if (!c_ptr->m_idx) {
			    //&& !c_ptr->o_idx
				/* This should be cfg.trap_bump_disarm? */
				if (GetCS(c_ptr, CS_MON_TRAP)) {
					do_cmd_disarm(Ind, dir);
					return;
				}
			}
		}

		/* Actually move the character */
		move_player(Ind, dir, pickup, &consume_full_energy);
		p_ptr->warning_autoret_ok = 0;

		/* Take a turn (or less) */
		if (consume_full_energy) p_ptr->energy -= level_speed(&p_ptr->wpos);//force-attacking always costs a whole turn
		else {
			int fast_move = 100;

			if (p_ptr->melee_sprint || p_ptr->shadow_running) fast_move /= 2;
			if (p_ptr->mode & MODE_PVP) fast_move /= 2;
			/* 'Shadow walk' effect - move faster in the shadows */
			if (get_skill(p_ptr, SKILL_OSHADOW) >= 10 && no_real_lite(Ind))
				fast_move = (fast_move * (15 - get_skill_scale(p_ptr, SKILL_OSHADOW, 5))) / 15;

			p_ptr->energy -= (level_speed(&p_ptr->wpos) * fast_move) / 100;
		}

		/* Allow more walking */
		more = TRUE;
	}

	/* Cancel repeat unless we may continue */
	if (!more) disturb(Ind, 0, 0);
}



/*
 * Start running.
 */
/* Hack -- since this command has different cases of energy requirements and
 * if we don't have enough energy sometimes we want to queue and sometimes we
 * don't, we do all of the energy checking within this function.  If after all
 * is said and done we want to queue the command, we return a 0.  If we don't,
 * we return a 2.
 */
int do_cmd_run(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	cave_type *c_ptr, **zcave;

	/* slower 'running' movement over certain terrain */
	int real_speed = cfg.running_speed;
	if (!(zcave = getcave(&p_ptr->wpos))) return(FALSE);
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	if (!p_ptr->warning_numpadmove &&
	    (dir == 1 || dir == 3 || dir == 7 || dir == 9))
		p_ptr->warning_numpadmove = 1;

	eff_running_speed(&real_speed, p_ptr, c_ptr);
#if 1 /* NEW_RUNNING_FEAT */
	/* running over floor grids, or special grids that we couldn't run over without according ability? Used by see_wall !*/
	p_ptr->running_on_floor = FALSE;
#endif

	/* Get a "repeated" direction */
	if (dir) {
		/* Handle confinement */
		if (p_ptr->stopped && dir != 5) {
			/* Try to break the rune */
			if (rand_int(200) < p_ptr->lev) {
				msg_print(Ind, "You break the rune!");
				set_stopped(Ind, 0);
			} else {
				p_ptr->energy -= level_speed(&p_ptr->wpos);
				msg_print(Ind, "You fail to break the confinement rune!");
				disturb(Ind, 0, 0);
				return(2);
			}
		}

		/* Make sure we have an empty space to run into */
		if (see_wall(Ind, dir, p_ptr->py, p_ptr->px)) {
			/* Prob travel */
			if (p_ptr->prob_travel && (!cave_floor_bold(zcave, p_ptr->py, p_ptr->px))) {
				(void)do_prob_travel(Ind, dir);
				return(2);
			}

			/* Handle the cfg_door_bump option */
			if (cfg.door_bump_open && p_ptr->easy_open) {
				/* Get requested grid */
				c_ptr = &zcave[p_ptr->py + ddy[dir]][p_ptr->px + ddx[dir]];

				if (((c_ptr->feat >= FEAT_DOOR_HEAD) &&
				      (c_ptr->feat <= FEAT_DOOR_TAIL)) ||
				    ((c_ptr->feat == FEAT_HOME))) {
					bool ret = TRUE;

					/* Check if we have enough energy to open the door */
					if (p_ptr->energy >= level_speed(&p_ptr->wpos)) {
#ifdef ENABLE_OUNLIFE
						/* Wraithstep gets auto-cancelled on forced interaction with solid environment */
						if (p_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1)) set_tim_wraith(Ind, 0);
#endif
						if (!CANNOT_OPERATE_SPECTRAL && !CANNOT_OPERATE_FORM) { /* players in WRAITHFORM can't open doors - mikaelh */
							/* If so, open it. */
							do_cmd_open(Ind, dir);
						} else ret = FALSE;
					}
					if (ret) return(2);
				}
			}

			/* Message */
			msg_print(Ind, "You cannot run in that direction.");

			/* Disturb */
			disturb(Ind, 0, 0);

			return(2);
		}

		/* Make sure we have enough energy to start running */
#ifdef NEW_AUTORET_RESERVE_ENERGY
		p_ptr->energy += (p_ptr->instant_retaliator
 #ifdef NEW_AUTORET_RESERVE_ENERGY_WORKAROUND
		    || p_ptr->running
 #endif
		    ? 0 : p_ptr->reserve_energy);
#endif
		if (p_ptr->energy >= (level_speed(&p_ptr->wpos) * (real_speed + 1)) / real_speed)
		//if (p_ptr->energy >= level_speed(&p_ptr->wpos)) /* otherwise auto-retaliation will never allow running */
		{
			char consume_full_energy;

			/* Hack -- Set the run counter */
			p_ptr->running = 20000; //enough to cross the world horizontally (was 1000)

			/* First step */
			run_step(Ind, dir, &consume_full_energy);

			if (consume_full_energy)
				/* Consume the normal full amount of energy in case we have e.g. attacked a monster */
				p_ptr->energy -= level_speed(&p_ptr->wpos);
			else
				/* Reset the player's energy so he can't sprint several spaces
				 * in the first round of running.  */
				p_ptr->energy = level_speed(&p_ptr->wpos);

#ifdef NEW_AUTORET_RESERVE_ENERGY
			if (!p_ptr->instant_retaliator &&
 #ifdef NEW_AUTORET_RESERVE_ENERGY_WORKAROUND
			    !p_ptr->running &&
 #endif
			    p_ptr->energy >= level_speed(&p_ptr->wpos) - 1) {
				p_ptr->reserve_energy = level_speed(&p_ptr->wpos) - 1;
				p_ptr->energy -= p_ptr->reserve_energy;
			} else p_ptr->reserve_energy = 0;
#endif

			return(2);
		}

#ifdef NEW_AUTORET_RESERVE_ENERGY
		if (!p_ptr->instant_retaliator &&
 #ifdef NEW_AUTORET_RESERVE_ENERGY_WORKAROUND
		    !p_ptr->running &&
 #endif
		    p_ptr->energy >= level_speed(&p_ptr->wpos) - 1) {
			p_ptr->reserve_energy = level_speed(&p_ptr->wpos) - 1;
			p_ptr->energy -= p_ptr->reserve_energy;
		} else p_ptr->reserve_energy = 0;
#endif

		/* If we don't have enough energy to run and monsters aren't around,
		 * try to queue the run command.
		 */
		return(0);
	}

	return(2);
}



/*
 * Stay still.  Search.  Enter stores.
 * Pick up treasure if "pickup" is true.
 * Specialty: pickup == 2 -> explicit pickup (not autopickup, per client option)
 */
void do_cmd_stay(int Ind, int pickup, bool one) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type *c_ptr;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return;
	if (p_ptr->new_level_flag) return;

	c_ptr = &zcave[p_ptr->py][p_ptr->px];


#if 0 /* We don't want any of this */
	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	/* Spontaneous Searching */
	if ((p_ptr->skill_fos >= 75) || (0 == rand_int(76 - p_ptr->skill_fos))) search(Ind);

	/* Continuous Searching */
	if (p_ptr->searching) search(Ind);
#endif


	/* Hack -- enter a store if we are on one */
	if (c_ptr->feat == FEAT_SHOP)
#if 0
	if ((c_ptr->feat >= FEAT_SHOP_HEAD) &&
	    (c_ptr->feat <= FEAT_SHOP_TAIL))
#endif	// 0
	{
		/* Disturb */
		disturb(Ind, 0, 0);

		/* Hack -- enter store */
		command_new = '_';
	}


	/* Try to Pick up anything under us */
	carry(Ind, pickup, 1, one);
}

/*
 * Determines the odds of an object breaking when thrown at a monster
 *
 * Note that artifacts never break, see the "drop_near()" function.
 */
int breakage_chance(object_type *o_ptr) {
	/* artifacts never break */
	if (artifact_p(o_ptr)) return(0);
	if ((o_ptr->questor && o_ptr->questor_invincible)) return(0);

	/* Special: Light armour and shields seldom break */
	if (is_textile_armour(o_ptr->tval, o_ptr->sval)) return(2);
	if (o_ptr->tval == TV_SHIELD) return(2);

	/* Examine the item type */
	switch (o_ptr->tval) {
	/* Always break */
	case TV_FLASK:
	case TV_POTION:
	case TV_POTION2:
	case TV_BOTTLE:
		return(100);

	/* Often break */
	case TV_FOOD:
	case TV_JUNK:
	case TV_LITE:
	case TV_SKELETON:
	case TV_TRAPKIT:
		return(50);

	/* Sometimes break */
	//case TV_SPIKE:
	case TV_SCROLL:
	case TV_WAND:
		return(25);

	case TV_SHOT:
	case TV_BOLT:
	case TV_ARROW:
		if (o_ptr->sval == SV_AMMO_MAGIC && !cursed_p(o_ptr)) return(0);
		else if (o_ptr->name2 == EGO_ETHEREAL || o_ptr->name2b == EGO_ETHEREAL) return(10);
		else if (o_ptr->tval == TV_SHOT) return(10);
		else if (o_ptr->tval == TV_BOLT) return(15);
		return(20);

	/* seldom break */
	case TV_BOOMERANG:
	case TV_FIRESTONE:
	case TV_RING:
	case TV_AMULET:
	//case TV_RUNE:
	case TV_GEM:
		return(2);

	/* never break */
	case TV_GAME:
	case TV_GOLD:
	case TV_KEY:
		return(0);

	/* special */
	case TV_GOLEM:
		if (o_ptr->sval <= SV_GOLEM_ADAM) return(0); /* massive piece */
		if (o_ptr->sval >= SV_GOLEM_ATTACK) return(25); /* scroll */
		return(10); /* arm/leg */

#ifdef ENABLE_DEMOLITIONIST
	case TV_CHARGE:
		return(5);
#endif

	/* for throwing weapons: Weapons in general are meant to be used for hitting, so should be ok */
	case TV_SWORD:
	case TV_AXE:
	case TV_BLUNT:
	case TV_POLEARM:
		if (is_throwing_weapon(o_ptr)) return(2);
		return(3);
	}

	/* Default: Rarely break */
	/* (eg rod, staff, mstaff, weapon, other armour, tool) */
	return(10);
}

/* Magic brand, shield - Kurzel */
void do_nimbus(int Ind, int y, int x) {
	player_type *p_ptr = Players[Ind];

	if (p_ptr->cmp < 2) {
		p_ptr->cmp = 0;
		(void)set_nimbus(Ind, 0, p_ptr->nimbus_t, p_ptr->nimbus_d);
		return;
	}
	p_ptr->cmp = p_ptr->cmp - 2;
	p_ptr->redraw |= (PR_MANA);
#ifdef USE_SOUND_2010
	sound(Ind, "cast_ball", NULL, SFX_TYPE_COMMAND, TRUE);
#endif
	project(0 - Ind, 1, &p_ptr->wpos, y, x, p_ptr->nimbus_d, p_ptr->nimbus_t,
	PROJECT_NORF | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO | PROJECT_LODF, "");
	return;
}

/* Add a nice ball if needed */
static void do_arrow_brand_effect(int Ind, int y, int x) {
	player_type *p_ptr = Players[Ind];

	switch (p_ptr->ammo_brand_t) {
	case TBRAND_BALL_FIRE:
		project(0 - Ind, 2, &p_ptr->wpos, y, x, 30, GF_FIRE, PROJECT_NORF | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO | PROJECT_LODF, "");
		break;
	case TBRAND_BALL_COLD:
		project(0 - Ind, 2, &p_ptr->wpos, y, x, 35, GF_COLD, PROJECT_NORF | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO | PROJECT_LODF, "");
		break;
	case TBRAND_BALL_ELEC:
		project(0 - Ind, 2, &p_ptr->wpos, y, x, 40, GF_ELEC, PROJECT_NORF | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO | PROJECT_LODF, "");
		break;
	case TBRAND_BALL_ACID:
		project(0 - Ind, 2, &p_ptr->wpos, y, x, 45, GF_ACID, PROJECT_NORF | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO | PROJECT_LODF, "");
		break;
	case TBRAND_BALL_SOUN:
		project(0 - Ind, 2, &p_ptr->wpos, y, x, 30, GF_SOUND, PROJECT_NORF | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO | PROJECT_LODF, "");
		break;
	case TBRAND_CHAO:
		/* XXX This allows the target player to 'dodge' the effect... */
		project(0 - Ind, 0, &p_ptr->wpos, y, x, 1, GF_CONFUSION, PROJECT_NORF | PROJECT_JUMP | PROJECT_KILL | PROJECT_NODO | PROJECT_NODF, "");
		break;
	}
}

/* Exploding arrow */
//static void do_arrow_explode(int Ind, object_type *o_ptr, int y, int x)
void do_arrow_explode(int Ind, object_type *o_ptr, worldpos *wpos, int y, int x, int might) {
	//player_type *p_ptr = Players[Ind];
	int rad = 0;
	//int dam = (damroll(o_ptr->dd, o_ptr->ds) + o_ptr->to_d) * 2 * ((might / 3) + 1);
	//int dam = (damroll(o_ptr->dd, o_ptr->ds) + o_ptr->to_d) * 4;
	//int dam = (damroll(o_ptr->dd, o_ptr->ds) + 5) * 3 + o_ptr->to_d;
	int dam = (damroll(o_ptr->dd, o_ptr->ds) + 10) * 2 + o_ptr->to_d;
	int flag = PROJECT_NORF | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NODO;

	switch (o_ptr->sval) {
	case SV_AMMO_LIGHT: rad = 1; break;
	case SV_AMMO_NORMAL: rad = 2; break;
	case SV_AMMO_HEAVY: rad = 3; break;
	//case SV_AMMO_MAGIC <- magic arrows only, don't explode
	case SV_AMMO_SILVER: rad = 2; break;
	}

	if ((rad > 2) && (
	    (o_ptr->pval == GF_KILL_WALL) ||
	    (o_ptr->pval == GF_DISINTEGRATE) ||
	    (o_ptr->pval == GF_AWAY_ALL) ||
	    (o_ptr->pval == GF_TURN_ALL) ||
	    (o_ptr->pval == GF_NEXUS) ||
	    (o_ptr->pval == GF_GRAVITY) ||
	    (o_ptr->pval == GF_TIME) ||
	    (o_ptr->pval == GF_STUN)))
		rad = 2;

	project(0 - Ind, rad, wpos, y, x, dam, o_ptr->pval, flag, "");
}

/*
 * Return multiplier of an object
 * NOTE: Launchers are pretty hacky! The (sval % 10) actually encodes the multiplier!
 *       So slings must have sval 2, bows 12 and 13, xbows 23 and 24.
 */
int get_shooter_mult(object_type *o_ptr) {
	/* Assume a base multiplier */
	int tmul = 1, sval;

	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT && o_ptr->xtra3 & 0x0200) sval = o_ptr->sval2;
	else sval = o_ptr->sval;

	/* Analyze the launcher */
	switch (sval) {
	case SV_SLING:
		/* Sling and ammo */
		tmul = 2;
		break;
	case SV_SHORT_BOW:
		/* Short Bow and Arrow */
		tmul = 2;
		break;
	case SV_LONG_BOW:
		/* Long Bow and Arrow */
		tmul = 3;
		break;
	case SV_LIGHT_XBOW:
		/* Light Crossbow and Bolt */
		tmul = 3;
		break;
	case SV_HEAVY_XBOW:
		/* Heavy Crossbow and Bolt */
		tmul = 4;
		break;
	}
	return(tmul);
}


/* Turn off afk mode? Or is this just auto-retaliation. */
bool retaliating_cmd = FALSE;

/*
 * Fire an object from the pack or floor.
 *
 * You may only fire items that "match" your missile launcher.
 *
 * You must use slings + pebbles/shots, bows + arrows, xbows + bolts.
 *
 * See "calc_boni()" for more calculations and such.
 *
 * Note that "firing" a missile is MUCH better than "throwing" it.
 *
 * Note: "unseen" monsters are very hard to hit.
 *
 * Objects are more likely to break if they "attempt" to hit a monster.
 *
 * Rangers (with Bows) and Anyone (with "Extra Shots") get extra shots.
 *
 * The "extra shot" code works by decreasing the amount of energy
 * required to make each shot, spreading the shots out over time.
 *
 * Note that when firing missiles, the launcher multiplier is applied
 * after all the boni are added in, making multipliers very useful.
 *
 * Note that Bows of "Extra Might" get extra range and an extra bonus
 * for the damage multiplier.
 *
 * Note that Bows of "Extra Shots" give an extra shot.
 */
/* Added a lot of hacks to handle boomerangs.	- Jir - */
/* Added another lot of hacks to handle quiver-slot.	- Jir - */
/* XXX it's... way too dirty. consider using 'project()' */
void do_cmd_fire(int Ind, int dir) {
	player_type *p_ptr = Players[Ind], *q_ptr;
	struct worldpos *wpos = &p_ptr->wpos;
	bool warn = FALSE;

	int i, j, y, x, ny, nx, ty, tx, bx, by;
#ifdef PY_FIRE_ON_WALL
	int prev_x, prev_y; /* for flare missile being able to light up a room under projectable_wall() conditions */
#endif
	int tdam, tdis, thits, tmul, vorpal_cut = 0;
	int bonus, chance;
	int cur_dis, visible, real_dis;
	int breakage = 0, num_ricochet = 0, ricochet_chance = 0;
	int aimed_ricochet = FALSE;
	bool random_ricochetting = FALSE; /* When TRUE it means get_outward_target() hasn't done all required projectable-los-checks, so we have to do that. */
	int item = INVEN_AMMO;
	int archery = get_archery_skill(p_ptr);

	object_type throw_obj;
	object_type *o_ptr;
	object_type *j_ptr;
	bool hit_body = FALSE;

	int missile_attr;
	int missile_char;

#ifdef OPTIMIZED_ANIMATIONS
	/* Projectile path */
	int path_y[MAX_RANGE];
	int path_x[MAX_RANGE];
	int path_num = 0;
#endif

	bool drain_msg = TRUE;
	int drain_result = 0, drain_heal = 0;
	int drain_left = MAX_VAMPIRIC_DRAIN_RANGED;
	bool drainable = TRUE;
	bool ranged_double_real = FALSE, ranged_flare_body = FALSE;

	int		break_chance;
#ifdef USE_SOUND_2010
	int		sfx = 0;
#endif
	u32b f1, f1a, f5, fx, esp;

	char o_name[ONAME_LEN];
	bool returning = FALSE, magic = FALSE, boomerang = FALSE, ethereal = FALSE;
	bool target_ok = target_okay(Ind);
#if 0 //DEBUG
#ifdef USE_SOUND_2010
	int otval;
#endif
#endif
	cave_type **zcave;


	if (!(zcave = getcave(wpos))) return;

	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRE_BAT) {
		msg_print(Ind, "\377yYou cannot use ranged weapons in bat form.");
		return;
	}
	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST) {
		msg_print(Ind, "\377yYou cannot use ranged weapons in mist form.");
		return;
	}

	/* New '+' feat in 4.4.6.2 */
	if (dir == 11) {
		get_aim_dir(Ind);
		p_ptr->current_fire = 1;
		return;
	}

	if (p_ptr->shooting_till_kill) { /* we were shooting till kill last turn? */
		p_ptr->shooting_till_kill = FALSE; /* well, gotta re-test for another success now.. */
		if (dir == 5) p_ptr->shooty_till_kill = TRUE; /* so for now we are just ATTEMPTING to shoot till kill (assumed we have a monster for target) */
	}

	/* To continue shooting_till_kill, require clean LOS to target
	   with no other monsters in the way, so we won't wake up more monsters accidentally. */
	if (p_ptr->shooty_till_kill) {
		if (target_ok) {
#ifndef PY_FIRE_ON_WALL
			if (!projectable_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE)) {
#else
			if (!projectable_wall_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE)) {
#endif
				return;
			}
		} else {
			/* don't fire-till-kill at ourself! */
			p_ptr->shooty_till_kill = FALSE;
		}
	}

	/* Break goi/manashield */
#if 0
	if (p_ptr->invuln) set_invuln(Ind, 0);
	if (p_ptr->tim_manashield) set_tim_manashield(Ind, 0);
	if (p_ptr->tim_wraith) set_tim_wraith(Ind, 0);
#endif	// 0

	/* Get the "bow" (if any) */
	j_ptr = &(p_ptr->inventory[INVEN_BOW]);

	/* if no launcher is equipped check for boomerangs
	   inscribed '!L' in inventory - C. Blue */
	if ((!j_ptr->tval || !j_ptr->number) && item_tester_hook_wear(Ind, INVEN_BOW)) {
		for (i = 0; i < INVEN_PACK; i++) {
			if (p_ptr->inventory[i].tval == TV_BOOMERANG &&
			    check_guard_inscription(p_ptr->inventory[i].note, 'L')) {
#if 0
				/* Hack: Reduce energy cost in do_cmd_wield() by half a turn */
				p_ptr->energy += level_speed(&p_ptr->wpos) / 2;
#endif
				(void)do_cmd_wield(Ind, i, 0x0);
				break;
			}
		}
	}

	/* Require a launcher */
	if (!j_ptr->tval) {
		msg_print(Ind, "\377yYou have nothing to fire with.");
		return;
	}

	/* Extract the item flags */
	object_flags(j_ptr, &f1, &fx, &fx, &fx, &f5, &fx, &esp);

	if (j_ptr->tval == TV_BOOMERANG) {
		boomerang = TRUE;
		item = INVEN_BOW;
	}

	o_ptr = &(p_ptr->inventory[item]);

	/* if quiver is empty, try auto-reloading with items
	   inscribed '!L' in inventory - C. Blue */
	if (!o_ptr->tval || !o_ptr->number) {
		for (i = 0; i < INVEN_PACK; i++) {
			if (is_ammo(p_ptr->inventory[i].tval) &&
			    check_guard_inscription(p_ptr->inventory[i].note, 'L')) {
#if 0
				/* Hack: Reduce energy cost in do_cmd_wield() by half a turn */
				p_ptr->energy += level_speed(&p_ptr->wpos) / 2;
#endif
				(void)do_cmd_wield(Ind, i, 0x0);
				break;
			}
		}
	}

	/* ethereal ammo? */
	if (o_ptr->name2 == EGO_ETHEREAL || o_ptr->name2b == EGO_ETHEREAL) {
		returning = TRUE; /* even if cursed, if that's even possible */
		ethereal = TRUE;
	}

	if (check_guard_inscription(o_ptr->note, 'f')) {
		msg_print(Ind, "\377yThe item's inscription prevents it.");
		return;
	}

	if (!can_use_verbose(Ind, o_ptr)) return;

	if (!o_ptr->tval || !o_ptr->number) {
		msg_print(Ind, "\377yYour quiver is empty!");
		return;
	}

	if (o_ptr->tval != p_ptr->tval_ammo && !boomerang) {
		switch (p_ptr->tval_ammo) {
		case TV_SHOT: msg_print(Ind, "Your ranged weapon can only fire shots or pebbles!"); return;
		case TV_ARROW: msg_print(Ind, "Your ranged weapon can only fire arrows!"); return;
		case TV_BOLT: msg_print(Ind, "Your ranged weapon can only fire bolts!"); return;
		}
		msg_print(Ind, "Your ranged weapon is too heavy for you to use!"); return;
		return;
	}

	/* Extract the item flags */
	object_flags(o_ptr, &f1a, &fx, &fx, &fx, &fx, &fx, &esp);

	/* Use the proper number of shots */
	if (!(thits = p_ptr->num_fire)) return; //div/0 check

	if (!boomerang) {
		/* Actually "fire" the object */
		bonus = (p_ptr->to_h + p_ptr->to_h_ranged * (p_ptr->ranged_precision ? 2 : 1)
			+ o_ptr->to_h + j_ptr->to_h);
		chance = (p_ptr->skill_thb + (bonus * BTH_PLUS_ADJ));

		//tmul = j_ptr->sval % 10;
		tmul = get_shooter_mult(j_ptr);
	} else {
		/* Actually "fire" the object */
		bonus = (p_ptr->to_h + p_ptr->to_h_ranged + o_ptr->to_h);
		chance = (p_ptr->skill_tht + (bonus * BTH_PLUS_ADJ));

		/* Assume a base multiplier */
		tmul = 1;

		/* Hack -- sorta magic */
		returning = TRUE;
	}
	if (p_ptr->blind) chance >>= 2;

//s_printf("R chance %d, skill_thb %d, bonus %d\n", chance, p_ptr->skill_thb, bonus); //DEBUG hit chance

	/* Is this magic Arrow or magic shots or magic bolts? */
	if (is_ammo(o_ptr->tval) && o_ptr->sval == SV_AMMO_MAGIC) {
		magic = TRUE;
		if (!cursed_p(o_ptr)) returning = TRUE;
	}
	/* Artifact ammo doesn't drop to floor */
	if (artifact_p(o_ptr)) returning = TRUE; /* even if cursed */

	/* Get extra "power" from "extra might" */
	if (!boomerang) tmul += p_ptr->xtra_might;
	/* Base range */
	tdis = 9 + 3 * tmul;
	if (boomerang) tdis = 15;
	/* Boomerangs use hacked xmight to apply a global damage scaling by skill - but not affecting throwing range */
	if (boomerang) tmul = 10 + p_ptr->xtra_might;

	/* Play fairly */
#ifdef ARROW_DIST_LIMIT
	if (tdis > ARROW_DIST_LIMIT) tdis = ARROW_DIST_LIMIT; /* == MAX_RANGE */
#else
	if (tdis > MAX_RANGE) tdis = MAX_RANGE;
#endif

	/* Distance to target too great?
	   Use distance() to form a 'natural' circle shaped radius instead of a square shaped radius,
	   monsters do this too.
	   If target is not ok, it means we fire at our own grid, so don't return() in that case. */
	if (dir == 5 && target_ok && distance(p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col) > tdis) return;

	/* Only fire in direction 5 if we have a target */
	/* Also only fire at all at a desired target, if we're actually within shooting range - C. Blue */
	if (dir == 5) {
		if (target_ok) {
			/* some minor launchers cannot achieve required range for the specified target - C. Blue */
#ifndef PY_FIRE_ON_WALL
			if (!projectable(&p_ptr->wpos, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, tdis)) return;
#else
			if (!projectable_wall(&p_ptr->wpos, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, tdis)) return;
#endif
		} else {
			/* for 4.4.6+, allow shooting (exploding) ammo 'at oneself' */
			if (!is_newer_than(&p_ptr->version, 4, 4, 5, 10, 0, 0)) return;
		}
	}

#ifdef TARGET_SWITCHING_COST_RANGED
	/* Time cost for switching target during ongoing combat. */
	/* we did attack something right now without any pause afterwards,
	   and it was something different than our current target?
	   (Paranoid todo: account for stationary targetting, could potentially be exploited if insane) */
	if (p_ptr->tsc_lasttarget != p_ptr->target_who
	    /* leeway - don't apply it to already pretty slow attackers */
	    && p_ptr->num_fire > 2) {
		/* we switched to a new target? */
		if (p_ptr->tsc_lasttarget) { //todo: maybe allow 'double shot' technique to sometimes bypass switching cost?
			p_ptr->tsc_lasttarget = p_ptr->target_who;
			/* skip a shot worth of energy, for setting aim to our new target */
			p_ptr->energy -= level_speed(&p_ptr->wpos) / thits;
		} else
			/* we actually just began ranged combat, attacking our very first target - we're already prepared. */
			p_ptr->tsc_lasttarget = p_ptr->target_who;
	}
#endif

	/* Check if monsters around him/her hinder this */
	//if (interfere(Ind, cfg.spell_interfere * 3)) return;
	/* boomerang is harder to intercept since it can just be swung as weapon :> - C. Blue */
	if (boomerang) {
		if (interfere(Ind, 25)) {
			p_ptr->energy -= level_speed(&p_ptr->wpos) / thits;
#ifdef INTERFERE_KEEPS_FTK
			if (p_ptr->shooty_till_kill) {
				p_ptr->shooting_till_kill = TRUE;
				/* disable other ftk types */
				p_ptr->shoot_till_kill_spell = 0;
				p_ptr->shoot_till_kill_mimic = 0;
				p_ptr->shoot_till_kill_rcraft = FALSE;
				p_ptr->shoot_till_kill_wand = 0;
				p_ptr->shoot_till_kill_rod = 0;
			}
#endif
			return; /* boomerang interference chance */
		}
	} else {
		if (interfere(Ind, p_ptr->ranged_precision ? 80 : 50)) {
			if (p_ptr->ranged_barrage &&
			    o_ptr->number >= 6 &&
			    p_ptr->cst >= 9)
				p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;
			else
				/* Take a (partial) turn */
				p_ptr->energy -= (level_speed(&p_ptr->wpos) /
				    (thits * (p_ptr->ranged_double && o_ptr->number >= 2 && p_ptr->cst >= 1 ? 2 : 1)));

#ifdef INTERFERE_KEEPS_FTK
			if (p_ptr->shooty_till_kill) {
				p_ptr->shooting_till_kill = TRUE;
				/* disable other ftk types */
				p_ptr->shoot_till_kill_spell = 0;
				p_ptr->shoot_till_kill_mimic = 0;
				p_ptr->shoot_till_kill_rcraft = FALSE;
				p_ptr->shoot_till_kill_wand = 0;
				p_ptr->shoot_till_kill_rod = 0;
			}
#endif
			return; /* shooting interference chance */
		}
	}

	if (p_ptr->ranged_flare) {
		if (boomerang) {
			msg_print(Ind, "You cannot use shooting techniques with a boomerang!");
			p_ptr->ranged_flare = FALSE;
		} else if (p_ptr->cst < 2) {
			msg_print(Ind, "Not enough stamina for a flare missile.");
			p_ptr->ranged_flare = FALSE;
		} else if (check_guard_inscription(p_ptr->inventory[INVEN_AMMO].note, 'k')) {
			msg_print(Ind, "Your ammo's inscription (!k) prevents using it as flare missile.");
			p_ptr->ranged_flare = FALSE;
		} else {
			use_stamina(p_ptr, 2);
			msg_format_near(Ind, "%s fires a flare missile.", p_ptr->name);
		}
	}
	if (p_ptr->ranged_precision) {
		if (boomerang) {
			msg_print(Ind, "You cannot use shooting techniques with a boomerang!");
			p_ptr->ranged_precision = FALSE;
		} else if (p_ptr->cst < 7) {
			msg_print(Ind, "Not enough stamina for a precision shot.");
			p_ptr->ranged_precision = FALSE;
		} else use_stamina(p_ptr, 7);
	}
	if (p_ptr->ranged_double) {
		if (boomerang) {
			msg_print(Ind, "You cannot use shooting techniques with a boomerang!");
			p_ptr->ranged_double = FALSE;
		} else if (o_ptr->number < 2) {
			msg_print(Ind, "You need at least 2 projectiles for a double shot!");
			p_ptr->ranged_double = FALSE;
		}
		else if (p_ptr->cst >= 1) { /* don't toggle off even if we reach 0 stamina */
			ranged_double_real = TRUE;
		}
	}
	if (p_ptr->ranged_barrage) {
		if (boomerang) {
			msg_print(Ind, "You cannot use shooting techniques with a boomerang!");
			p_ptr->ranged_barrage = FALSE;
		} else if (o_ptr->number < 6) { /* && !returning) {  :-p */
			msg_print(Ind, "You need at least 6 projectiles for a barrage!");
			p_ptr->ranged_barrage = FALSE;
		} else if (p_ptr->cst < 9) {
			msg_print(Ind, "Not enough stamina for barrage.");
			p_ptr->ranged_barrage = FALSE;
		} else {
			use_stamina(p_ptr, 9);
			msg_format_near(Ind, "%s fires a multi-shot barrage!", p_ptr->name);
		}
	}
	p_ptr->redraw |= PR_STAMINA;

	/* silent fail */
	if (returning) {
		/* hack - allow use of magic ammo for flare now,
		   but in that case make it non-returning since it burns on the floor,
		   serving as light source, as normal ammo would;
		   make sure artifact magic ammo doesn't work for 'flare'
		   because we don't want arts to get destroyed; ethereal doesn't work either. */
		if (!magic || artifact_p(o_ptr)) p_ptr->ranged_flare = FALSE;
	}

	/* Double # of shots? */
	if (ranged_double_real) {
		thits *= 2;

		/* prevent possible exploit (fire 4 rounds with a 5bpr shooter, then switch to 2bpr shooter)
		   although hardly someone will use that =-p */
		if (p_ptr->ranged_double_used >= thits) p_ptr->ranged_double_used = 0;

		if (!p_ptr->ranged_double_used) {
			if (!rand_int(5)) use_stamina(p_ptr, 1); /* artificially: slow drain */
			p_ptr->ranged_double_used = thits;
		}
		p_ptr->ranged_double_used--;
	}


	if (p_ptr->ranged_barrage)
		p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;
	else
		/* Take a (partial) turn */
		p_ptr->energy -= (level_speed(&p_ptr->wpos) / thits);

	/* if we intended to shoot till kill, then we did succeed now,
	   passed all tests that might return() instead, and ARE shooting
	   to kill. Note: Cursed arrow failure is exempt a.t.m. - C. Blue */
	if (p_ptr->shooty_till_kill) {
		p_ptr->shooting_till_kill = TRUE;
		/* disable other ftk types */
		p_ptr->shoot_till_kill_spell = 0;
		p_ptr->shoot_till_kill_mimic = 0;
		p_ptr->shoot_till_kill_rcraft = FALSE;
		p_ptr->shoot_till_kill_wand = 0;
		p_ptr->shoot_till_kill_rod = 0;
	}

	if (!boomerang && cursed_p(o_ptr) && magik(50)) {
		msg_print(Ind, "You somehow failed to fire!");
		return;
	}

#if (STARTEQ_TREATMENT > 1)
	/* Items belonging to and then being dropped by a character whose level is < cfg.newbies_cannot_drop become unsalable. */
	if (o_ptr->owner == p_ptr->id && p_ptr->max_plv < cfg.newbies_cannot_drop && !is_admin(p_ptr) &&
	    //o_ptr->tval != TV_GAME && o_ptr->tval != TV_KEY && --cannot fire keys or chests oO
	    o_ptr->tval != TV_SPECIAL) {
		/* Not for basic arrows, a bit too silyl compared to the annoyment/newbie confusion.
		   Basically, we want to turn everything level 0 that he could buy in town shops and then drop for someone else to pick up and utilize/monetize. */
		if (!is_ammo(o_ptr->tval) || o_ptr->name1 || o_ptr->name2) o_ptr->level = 0;
		o_ptr->mode |= MODE_STARTER_ITEM; //hack: mark as unsellable
	}
#endif

	/* S(he) is no longer afk */
	if (p_ptr->afk && !retaliating_cmd) un_afk_idle(Ind);
	retaliating_cmd = FALSE;

	/* Adjust VAMPIRIC draining to shooting speed, since
	   do_cmd_fire only handles ONE shot :/ */
	drain_left /= thits;

	/* Hack -- suppress messages */
	if (p_ptr->taciturn_messages) suppress_message = TRUE;

	/* Ricochets ? */
	/* Some players do not want ricocheting shots as to not wake up other
	 * monsters. How about we remove ricocheting shots for slingers, but
	 * instead, adds a chance to do a double damage (than normal and/or crit)
	 * shots (i.e., seperate than crit and stackable bonus)?  - the_sandman */
	if ((boomerang || !check_guard_inscription(o_ptr->note, 'R')) &&
	    !check_guard_inscription(j_ptr->note, 'R')) {
		/* Sling mastery yields bullet ricochets */
		if (archery == SKILL_SLING && !magic && !ethereal && !p_ptr->ranged_barrage) {
			num_ricochet = randint(get_skill_scale_fine(p_ptr, SKILL_SLING, 3));
			num_ricochet = (num_ricochet < 0) ? 0 : num_ricochet;
			ricochet_chance = 33 + get_skill_scale(p_ptr, SKILL_SLING, 42);
			if (!check_guard_inscription(o_ptr->note, 'O') &&
			    !check_guard_inscription(j_ptr->note, 'O') &&
			    !p_ptr->image && !p_ptr->confused &&
			    (i = get_skill(p_ptr, SKILL_SLING)) >= 15)
				aimed_ricochet = i - 14;
		}
		/* Boomerangs can leave a trail of decimation among weaker critters */
		else if (boomerang) {
			num_ricochet = randint(get_skill_scale_fine(p_ptr, SKILL_BOOMERANG, 5));
			num_ricochet = (num_ricochet < 0) ? 0 : num_ricochet;
			ricochet_chance = 33 + get_skill_scale(p_ptr, SKILL_BOOMERANG, 42);
			if (!check_guard_inscription(j_ptr->note, 'O') &&
			    !p_ptr->image && !p_ptr->confused &&
			    (i = get_skill(p_ptr, SKILL_BOOMERANG)) >= 20)
				aimed_ricochet = i - 19;
		}
	}

	/* Create a "local missile object" */
	throw_obj = *o_ptr;
	throw_obj.number = p_ptr->ranged_barrage ? 6 : 1; /* doesn't work anyway for boomerangs since it tested for 6 items */

	/* Use the missile object */
	o_ptr = &throw_obj;

#if 0 //DEBUG
#ifdef USE_SOUND_2010
	/* save for sfx later (after the missile object was maybe already destroyed) */
	otval = o_ptr->tval;
#endif
#endif

	/* Describe the object */
	object_desc(Ind, o_name, o_ptr, FALSE, 3);

	/* Find the color and symbol for the object for throwing */
	missile_attr = object_attr(o_ptr);
	missile_char = object_char(o_ptr);


	/* Start at the player */
	y = p_ptr->py;
	x = p_ptr->px;
	by = y;
	bx = x;

	/* Predict the "target" location */
	tx = p_ptr->px + 99 * ddx[dir]; // just use MAX_RANGE instead of 99...?
	ty = p_ptr->py + 99 * ddy[dir];

	/* Check for "target request" */
	if (dir == 5 && target_ok) {
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}

	break_chance = breakage_chance(o_ptr);

	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	bandage_fails(Ind);

	/* Reduce and describe inventory */
	if (!boomerang) {
		/* C. Blue - Artifact ammo never runs out (similar to magic arrows:) */
		if (artifact_p(o_ptr)) {
			/* doesn't run out/drop */
		} else if (ethereal) { /* being nice in regards to ranged_barrage break_chance */
			if (cursed_p(o_ptr) ? TRUE : magik(break_chance * (p_ptr->ranged_barrage ? 3 : 1))) {
				if (item >= 0) {
					inven_item_increase(Ind, item, -1);
					//inven_item_describe(Ind, item);
					msg_format(Ind, "\377wThe %s fades away.", o_name);
					inven_item_optimize(Ind, item);
				} else {
					floor_item_increase(0 - item, -1);
					floor_item_optimize(0 - item);
				}
			}
		} else if (!magic || cursed_p(o_ptr)) {
			if (item >= 0) {
				inven_item_increase(Ind, item, p_ptr->ranged_barrage ? -6 : -1);
				inven_item_describe(Ind, item);
				inven_item_optimize(Ind, item);
			}
			/* Reduce and describe floor item */
			else {
				floor_item_increase(0 - item, p_ptr->ranged_barrage ? -6 : -1);
				floor_item_optimize(0 - item);
			}
		} else {
#if 0 /* magic ammo (non-cursed) isn't decreased from shooting! */
			if (item >= 0) {
				inven_item_describe(Ind, item);
				inven_item_optimize(Ind, item);
			}
			/* Reduce and describe floor item */
			else {
				floor_item_optimize(0 - item);
			}
#endif
		}
	}

	/* Hack -- Handle stuff */
	handle_stuff(Ind);

#ifdef USE_SOUND_2010
	if (p_ptr->cut_sfx_attack) {
		sfx = (extract_energy[p_ptr->pspeed] / 10) * p_ptr->num_fire;
		if (sfx) {
			p_ptr->count_cut_sfx_attack += 10000 / sfx;
			if (p_ptr->count_cut_sfx_attack >= 200) { /* 5 attacks per turn */
				p_ptr->count_cut_sfx_attack -= 200;
				if (p_ptr->count_cut_sfx_attack >= 200) p_ptr->count_cut_sfx_attack = 0;
				sfx = 0;
			}
		}
	}
	if (p_ptr->half_sfx_attack && sfx == 0) {
		if (p_ptr->half_sfx_attack_state) sfx = -1;
		p_ptr->half_sfx_attack_state = !p_ptr->half_sfx_attack_state;
	}
	if (sfx == 0 && p_ptr->sfx_combat)
 #if 0 //DEBUG
		switch (otval) {
 #else
		switch (o_ptr->tval) {
 #endif
		case TV_SHOT: sound(Ind, "fire_shot", NULL, SFX_TYPE_ATTACK, FALSE); break;
		case TV_ARROW: sound(Ind, "fire_arrow", NULL, SFX_TYPE_ATTACK, FALSE); break;
		case TV_BOLT: sound(Ind, "fire_bolt", NULL, SFX_TYPE_ATTACK, FALSE); break;
		case TV_BOOMERANG: sound(Ind, "fire_boomerang", NULL, SFX_TYPE_ATTACK, FALSE); break;
	}
#endif

	if (!returning && !ethereal
	    && p_ptr->warning_autopickup < p_ptr->lev) { /* wow - repeat this hint every level =P */
		if (strchr(o_name, '!') && o_ptr->sval != SV_AMMO_MAGIC) p_ptr->warning_autopickup = PY_MAX_LEVEL;
		else {
			p_ptr->warning_autopickup = p_ptr->lev;
			msg_print(Ind, "\374\377yHINT: Press '\377o{\377y' key and inscribe your ammunition '\377o!=\377y' to pick it up automatically!");
			s_printf("warning_autopickup: %s\n", p_ptr->name);
		}
	}

#ifdef PY_FIRE_ON_WALL
	/* For flare missile under projectable_wall() settings */
	prev_x = bx;
	prev_y = by;
#endif

	if (!p_ptr->warning_macros && dir != 5 && dir < 10) warn = TRUE;

	while (TRUE) {
		/* Travel until stopped */
#if 0 /* I think it travelled once too far, since it's mmove2'ed in the very beginning of every for-pass, */
/* and it goes from 0 to tdis, that's tdis+1 steps in total. Correct me if wrong~~  C. Blue */
		//for (cur_dis = 0; cur_dis <= tdis; )
		for (cur_dis = real_dis = 0; real_dis <= tdis; )
#else
		//for (cur_dis = 0; cur_dis < tdis; )
		for (cur_dis = real_dis = 0; real_dis < tdis; )
#endif
		{
			ny = y;
			nx = x;

			/* Hack -- Stop at the target */
			if (y == ty && x == tx) break;

			/* Calculate the new location (see "project()") */
			//mmove2(&ny, &nx, p_ptr->py, p_ptr->px, ty, tx);
			mmove2(&ny, &nx, by, bx, ty, tx);

#ifdef DOUBLE_LOS_SAFETY
			/* skip checks if we already used projectable..() routines to
			   determine that it's fine, so it'd be redundant and also require
			   additional code to comply with DOUBLE_LOS_SAFETY. */
			if (dir != 5 || random_ricochetting) {
#endif
				/* Sanity check because server crashed here on 2012-01-10 - mikaelh */
				if (!in_bounds_array(ny, nx)) break;

#ifndef PY_FIRE_ON_WALL
				/* Stopped by walls/doors */
				if (!cave_contact(zcave, ny, nx)) break;
#else
				/* Stopped by protected grids (Inn doors, also stopping monsters' ranged attacks!) */
				if (f_info[zcave[ny][nx].feat].flags1 & (FF1_BLOCK_LOS | FF1_BLOCK_CONTACT)) break;
#endif
#ifdef DOUBLE_LOS_SAFETY
			}
			/* End of skipping redundant checks for targetted shots. */
#endif

			/* Advance the distance */
			cur_dis++;

			/* Distance to target too great?
			   Use distance() to form a 'natural' circle shaped radius instead of a square shaped radius,
			   monsters do this too */
			real_dis = distance(by, bx, ny, nx);

#ifdef PY_FIRE_ON_WALL
			/* For flare missile under projectable_wall() settings */
			prev_x = x;
			prev_y = y;
#endif

			/* Save the new location */
			x = nx;
			y = ny;

#ifndef OPTIMIZED_ANIMATIONS
			/* Save the old "player pointer" */
			q_ptr = p_ptr;

			/* Display it for each player */
			for (i = 1; i <= NumPlayers; i++) {
				int dispx, dispy;

				/* Use this player */
				p_ptr = Players[i];

				/* If he's not playing, skip him */
				if (p_ptr->conn == NOT_CONNECTED) continue;

				/* If he's not here, skip him */
				if (!inarea(&p_ptr->wpos, wpos)) continue;

				/* The player can see the (on screen) missile */
				if (panel_contains(y, x) && player_can_see_bold(i, y, x)) {
					/* Draw, Hilite, Fresh, Pause, Erase */
					dispy = y - p_ptr->panel_row_prt;
					dispx = x - p_ptr->panel_col_prt;

					/* Remember the projectile */
					p_ptr->scr_info[dispy][dispx].c = missile_char;
					p_ptr->scr_info[dispy][dispx].a = missile_attr;

					/* Tell the client */
#ifdef GRAPHICS_BG_MASK
					Send_char(i, dispx, dispy, missile_attr, missile_char, 0, 0);
#else
					Send_char(i, dispx, dispy, missile_attr, missile_char);
#endif

					/* Flush and wait */
					if (cur_dis % (boomerang ? 2 : tmul)) Send_flush(i);

					/* Restore */
					lite_spot(i, y, x);
				}
			}

			/* Restore the player pointer */
			p_ptr = q_ptr;

#else /* OPTIMIZED_ANIMATIONS */

			/* Save the projectile path */
			if (path_num < MAX_RANGE) {
				path_y[path_num] = y;
				path_x[path_num] = x;
				path_num++;
			}
#endif /* OPTIMIZED_ANIMATIONS */

			/* Player here, hit him */
			if (zcave[y][x].m_idx < 0) {
				if (cfg.use_pk_rules == PK_RULES_NEVER) {
					/* Stop looking */
					if (!p_ptr->ammo_brand || (p_ptr->ammo_brand_t != TBRAND_VORP)) break;
				} else {
					cave_type *c_ptr = &zcave[y][x];
					char p_name[80];

					q_ptr = Players[0 - c_ptr->m_idx];
					/* Get the name */
					strcpy(p_name, q_ptr->name);

					p_ptr->test_attacks++;
					/* AD hack -- "pass over" players in same party */
#ifdef KURZEL_PK
					if ((
#else
					if (((p_ptr->pkill & PKILL_KILLER) ||
#endif
					    check_hostile(Ind, 0 - c_ptr->m_idx) ||
					    magik(NEUTRAL_FIRE_CHANCE)) &&
					    (check_blood_bond(Ind, 0 - c_ptr->m_idx) ||
					     p_ptr->party == 0 ||
					     !player_in_party(p_ptr->party, 0 - c_ptr->m_idx) ||
					     magik(FRIEND_FIRE_CHANCE))
					    && !p_ptr->admin_dm) {

						/* Check the visibility */
						visible = p_ptr->play_vis[0 - c_ptr->m_idx];

						/* Note the -potential- collision (still have to check if we passed AC) */
						hit_body = TRUE;

#ifndef KURZEL_PK
						if (cfg.use_pk_rules == PK_RULES_DECLARE) {
							if ((zcave[p_ptr->py][p_ptr->px].info & CAVE_NOPK) || (zcave[q_ptr->py][q_ptr->px].info & CAVE_NOPK)) {
								if (visible && (!player_in_party(Players[0 - c_ptr->m_idx]->party, Ind))) {
									p_ptr->target_who = 0;
									do_player_drop_items(Ind, 40, FALSE);
									imprison(Ind, JAIL_MURDER, "attempted murder");
								}
							}
						}
#endif

						if (q_ptr->dispersion && q_ptr->cst) {
							msg_format(0 - c_ptr->m_idx, "\377%cYou disperse around the projectile!", COLOUR_DODGE_GOOD);
							if (visible) msg_format(Ind, "\377%c%s disperses around %s.", COLOUR_DODGE_NEAR, p_name, o_name);
							if (magik(q_ptr->dispersion)) use_stamina(q_ptr, 1);
							continue;
						}

						/* Did we hit it (penalize range) */
#ifndef PVP_AC_REDUCTION
						if ((test_hit_fire(chance - cur_dis, q_ptr->ac + q_ptr->to_a, visible)
						    || (p_ptr->ranged_precision && visible))
						    && (!q_ptr->shadow_running || !rand_int(3))) {
#else
						//if (test_hit_fire(chance - cur_dis, ((q_ptr->ac + q_ptr->to_a) * 2) / 3, visible)) {
						if ((test_hit_fire(chance - cur_dis,
						    (q_ptr->ac + q_ptr->to_a > AC_CAP) ? AC_CAP : q_ptr->ac + q_ptr->to_a,
						    visible) || (p_ptr->ranged_precision && visible))
						    && (!q_ptr->shadow_running || !rand_int(3))) {
#endif
							bool dodged = FALSE;

#ifndef NEW_DODGING
							if (get_skill(q_ptr, SKILL_DODGE)) {
								//int chance = (q_ptr->dodge_level - p_ptr->lev - archery) / 3;
								int chance = (q_ptr->dodge_level - p_ptr->lev - get_skill(p_ptr, archery)) / 3;

								if ((chance > 0) && magik(chance)) {
									//msg_print(Ind, "You dodge a magical attack!");
									msg_format(0 - c_ptr->m_idx, "\377%cYou dodge the projectile!", COLOUR_DODGE_GOOD);
									if (visible) msg_format(Ind, "\377%c%s dodges %s.", COLOUR_DODGE_NEAR, p_name, o_name);
									dodged = TRUE;
/* gotta remove the dodged=TRUE stuff etc, we simply continue!*/	continue;
								}
							}
#else
							//if (magik(apply_dodge_chance(0 - c_ptr->m_idx, p_ptr->lev + get_skill(p_ptr, archery) / 3 + 1000))) { /* hack - more difficult to dodge ranged attacks */
							if (magik(apply_dodge_chance(0 - c_ptr->m_idx, p_ptr->lev + get_skill(p_ptr, archery) / 3)) && (!p_ptr->ranged_barrage || magik(50))) {
								// msg_print(Ind, "You dodge a magical attack!");
								msg_format(0 - c_ptr->m_idx, "\377%cYou dodge the projectile!", COLOUR_DODGE_GOOD);
								if (visible) msg_format(Ind, "\377%c%s dodges %s.", COLOUR_DODGE_NEAR, p_name, o_name);
								continue;
							}
#endif

#ifdef USE_BLOCKING /* Parry/Block - belongs to new-NR-viability changes */
							/* choose whether to attempt to block or to parry (can't do both at once),
							   50% chance each, except for if weapon is missing (anti-retaliate-inscription
							   has been left out, since if you want max block, you'll have to take off your weapon!) */
							if (q_ptr->shield_deflect &&
							    (!q_ptr->inventory[INVEN_WIELD].k_idx || magik(q_ptr->combat_stance == 1 ? 75 : 50))) {
								if (magik(apply_block_chance(q_ptr, q_ptr->shield_deflect + 15))) { /* boost for PvP! */
									if (visible) msg_format(Ind, "\377%c%s blocks %s!", COLOUR_BLOCK_PLY, p_name, o_name);
									switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
									case 's': case 'x': case 'z':
										msg_format(0 - c_ptr->m_idx, "\377%cYou block %s' attack!", COLOUR_BLOCK_GOOD, p_ptr->name);
										break;
									default:
										msg_format(0 - c_ptr->m_idx, "\377%cYou block %s's attack!", COLOUR_BLOCK_GOOD, p_ptr->name);
									}
 #ifdef USE_SOUND_2010
									if (sfx == 0 && p_ptr->sfx_defense) sound(Ind, "block_shield_projectile", NULL, SFX_TYPE_ATTACK, FALSE);
 #endif
									continue;
								}
							}
#endif
#if 0 /* disable for now? */
 #ifdef USE_PARRYING
							if (q_ptr->weapon_parry) {
								// && !p_ptr->ranged_barrage etc, any prepared bow stance?
								if (magik(apply_parry_chance(q_ptr, q_ptr->weapon_parry + 5))) { /* boost for PvP! */
									if (visible) msg_format(Ind, "\377%c%s parries %s!", COLOUR_PARRY_PLY, p_name, o_name);
									switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
									case 's': case 'x': case 'z':
										msg_format(0 - c_ptr->m_idx, "\377%cYou parry %s' attack!", COLOUR_PARRY_GOOD, p_ptr->name);
										break;
									default:
										msg_format(0 - c_ptr->m_idx, "\377%cYou parry %s's attack!", COLOUR_PARRY_GOOD, p_ptr->name);
									}
  #ifdef USE_SOUND_2010
									if (sfx == 0 && p_ptr->sfx_defense) sound(Ind, "parry_weapon", "parry", SFX_TYPE_ATTACK, FALSE);
  #endif
									continue;
								}
							}
 #endif
#endif

							if (!dodged) {	// 'goto' would be cleaner
								if (!boomerang) {
									/* Base damage from thrown object plus launcher bonus */
									tdam = damroll(o_ptr->dd, o_ptr->ds);
									tdam = brand_dam_aux_player(Ind, o_ptr, -1, tdam, q_ptr, FALSE);
									if (p_ptr->ranged_flare) tdam += damroll(2, 6); /* compare to dice in k_info for oil flask */
									if (p_ptr->ammo_brand) tdam += p_ptr->ammo_brand_d;
									tdam += o_ptr->to_d;
									tdam += j_ptr->to_d + p_ptr->to_d_ranged;

									/* Boost the damage */
									tdam *= tmul;

#ifdef DEFENSIVE_STANCE_FIXED_RANGED_REDUCTION
									if (p_ptr->combat_stance == 1) tdam /= 2;
#endif
								} else {
									 /* Base damage from thrown object */
									tdam = damroll(o_ptr->dd, o_ptr->ds);
#if defined(R_VORPAL_UNBRANDED) || defined(R_VORPAL_LOWBRANDED)
									if ((f5 & TR5_VORPAL) && !(q_ptr->no_cut) && !rand_int(R_VORPAL_CHANCE)) vorpal_cut = tdam; /* save unbranded dice */
									else vorpal_cut = FALSE;
#endif
									tdam = brand_dam_aux_player(Ind, o_ptr, -1, tdam, q_ptr, FALSE);
									tdam += o_ptr->to_d; //moved this here, or vorpal dam-addon will be too low
#ifdef R_VORPAL_LOWBRANDED
									if (vorpal_cut) vorpal_cut = (vorpal_cut + tdam) / 2;
#else
 #ifndef R_VORPAL_UNBRANDED
									if ((f5 & TR5_VORPAL) && !q_ptr->no_cut && !rand_int(R_VORPAL_CHANCE)) vorpal_cut = tdam; /* save branded dice */
									else vorpal_cut = FALSE;
 #endif
#endif
									tdam += p_ptr->to_d_ranged;

									/* Boost the damage */
									tdam = (tdam * tmul) / 10;

#ifdef DEFENSIVE_STANCE_FIXED_RANGED_REDUCTION
									if (p_ptr->combat_stance == 1) tdam /= 2;
#endif
								}
								ranged_flare_body = TRUE;

								if (!p_ptr->ranged_precision)
									tdam = critical_shot(Ind, o_ptr->weight, o_ptr->to_h, tdam,
									    calc_crit_obj(o_ptr) + (boomerang ? 0 : calc_crit_obj(j_ptr)), FALSE, !boomerang);
								else /* Precision shot skips most AC reduction */
									tdam = critical_shot(Ind, o_ptr->weight, o_ptr->to_h + 150, tdam,
									    calc_crit_obj(o_ptr) + (boomerang ? 0 : calc_crit_obj(j_ptr)), TRUE, !boomerang); //boomerangs can't do 'precision shot' anyway, pft

								/* Vorpal bonus - multi-dice!
								   (currently +31.25% more branded dice damage on total average, just for the records) */
								if (vorpal_cut) {
									msg_format(Ind, "Your weapon pierces deep into %s!", q_ptr->name);
#ifdef R_CRIT_VS_VORPAL
    xxx disabled xxx
									k2 += (magik(25) ? 2 : 1) * (vorpal_cut + 5); /* exempts critical strike */
									/* either critical hit or vorpal, not both */
									if (k2 > k) k = k2;
#else
									tdam += (magik(25) ? 2 : 1) * (vorpal_cut + 5); /* exempts critical strike */
#endif
								}

								/* Factor in AC, reducing the damage. 'Precision shot' skips AC reduction! */
								if (!p_ptr->ranged_precision)
									tdam -= (tdam * (((q_ptr->ac + q_ptr->to_a) < AC_CAP) ? (q_ptr->ac + q_ptr->to_a) : AC_CAP) / AC_CAP_DIV);
								else
									p_ptr->ranged_precision = FALSE;

								/* XXX Reduce damage by 1/3 */
								tdam = (tdam + PVP_SHOOT_DAM_REDUCTION - 1) / PVP_SHOOT_DAM_REDUCTION;

								if (ranged_double_real) tdam = (tdam * 35) / 100;

								if (p_ptr->ranged_barrage) tdam *= 2; // maybe 3 even

								/* No negative damage */
								if (tdam < 0) tdam = 0;

#ifdef ENABLE_OUNLIFE
								/* Wraithstep gets auto-cancelled on forced interaction with solid environment */
								if (p_ptr->tim_wraith && !q_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1)) set_tim_wraith(Ind, 0);
#endif

								/* can't attack while in WRAITHFORM (explosion still works) */
								if (p_ptr->tim_wraith && !q_ptr->tim_wraith) tdam = 0;

								/* Handle unseen player */
								if (!visible) {
									/* Invisible player */
									msg_format(Ind, "The %s finds a mark.", o_name);
									msg_format(0 - c_ptr->m_idx, "You are hit by a %s!", o_name);
								}
								/* Handle visible player */
								else {
									/* Messages */
									msg_format(Ind, "The %s hits %s for \377y%d \377wdamage.", o_name, p_name, tdam);
									if ((o_name[0] == 'A') || (o_name[0] == 'E') || (o_name[0] == 'I') || (o_name[0] == 'O') || (o_name[0] == 'U') ||
									    (o_name[0] == 'a') || (o_name[0] == 'e') || (o_name[0] == 'i') || (o_name[0] == 'o') || (o_name[0] == 'u'))
										msg_format(0 - c_ptr->m_idx, "%^s hits you with an %s for \377R%d \377wdamage.", p_ptr->name, o_name, tdam);
									else	msg_format(0 - c_ptr->m_idx, "%^s hits you with a %s for \377R%d \377wdamage.", p_ptr->name, o_name, tdam);

									/* Track this player's health */
									health_track(Ind, c_ptr->m_idx);
								}

								/* If this was intentional, make target hostile */
								if (check_hostile(Ind, 0 - c_ptr->m_idx)) {
									/* Make target hostile if not already */
									if (!check_hostile(0 - c_ptr->m_idx, Ind)) {
										bool result = FALSE;

										if (Players[0 - c_ptr->m_idx]->pvpexception < 2)
											result = add_hostility(0 - c_ptr->m_idx, p_ptr->name, FALSE, FALSE);

										/* Log it if no blood bond - mikaelh */
										if (!player_list_find(p_ptr->blood_bond, Players[0 - c_ptr->m_idx]->id)) {
											s_printf("%s attacked %s (shot; result %d).\n", p_ptr->name, Players[0 - c_ptr->m_idx]->name, result);
										}
									}
								}

								if ((p_ptr->ammo_brand && (p_ptr->ammo_brand_t == TBRAND_CHAO)) && !q_ptr->resist_conf && !boomerang)
									(void)set_confused(0 - c_ptr->m_idx, q_ptr->confused + 5);

								if (p_ptr->ranged_barrage)
									set_stun(0 - c_ptr->m_idx, q_ptr->stun + 35 + get_skill_scale(p_ptr, SKILL_COMBAT, 5));

								/* Take damage */
								take_hit(0 - c_ptr->m_idx, tdam, p_ptr->name, Ind);
								/* XXX confusion arrow is not handled right
								 * in do_arrow_brand_effect */
								if (!boomerang && p_ptr->ammo_brand_t
								    && p_ptr->ammo_brand_t != TBRAND_CHAO)
									do_arrow_brand_effect(Ind, y, x);

								/* Ammo pval takes precedence - Kurzel */
								if (!boomerang && !magic && o_ptr->pval)
									do_arrow_explode(Ind, o_ptr, wpos, y, x, tmul);
								else if (p_ptr->nimbus) do_nimbus(Ind, y, x);

								/* Stop looking */
#if 0
								/* We hit an entity (even if we didn't pass AC test), so we MUST break,
								   for ricochetting and all. Not sure what the next line is about, but it needs adjusting:
								   We must always break here, or ricochetting must be disabled, or hit_body evaluation must be expanded. */
								if (!p_ptr->ammo_brand || (p_ptr->ammo_brand_t != TBRAND_VORP) || boomerang) break;
#else
								break;
#endif
							}
						}

					} /* end hack */
				}
			}

			/* Monster here, Try to hit it */
			if (zcave[y][x].m_idx > 0) {
				cave_type *c_ptr = &zcave[y][x];

				monster_type *m_ptr = &m_list[c_ptr->m_idx];
				monster_race *r_ptr = race_inf(m_ptr);
				drainable = !((r_ptr->flags3 & RF3_UNDEAD) ||
					    /* (r_ptr->flags3 & RF3_DEMON) ||*/
					    (r_ptr->flags3 & RF3_NONLIVING) ||
					    (strchr("EgvwlIFijmxszQX", r_ptr->d_char)));

				/* Do not hit pets - the_sandman */
				if (m_ptr->pet) break;

				p_ptr->test_attacks++;
				/* Check the visibility */
				visible = p_ptr->mon_vis[c_ptr->m_idx];

				/* Note the -potential- collision (still have to check if we passed AC) */
				if (!(o_ptr->tval == TV_GAME && o_ptr->sval == SV_GAME_BALL)) hit_body = TRUE;

				/* Did we hit it (penalize range) */
				if (test_hit_fire(chance - cur_dis, m_ptr->ac, visible) || (p_ptr->ranged_precision && visible)) {
					bool fear = FALSE;
					char m_name[MNAME_LEN];

					/* Assume a default death */
					cptr note_dies = " dies";

					/* Get "the monster" or "it" */
					monster_desc(Ind, m_name, c_ptr->m_idx, 0);

					/* Some monsters get "destroyed" */
					if ((r_ptr->flags3 & RF3_DEMON) ||
					    (r_ptr->flags3 & RF3_UNDEAD) ||
					    (r_ptr->flags2 & RF2_STUPID) ||
					    (strchr("Evg", r_ptr->d_char))) {
						/* Special note at death */
						note_dies = " is destroyed";
					}


					/* Handle reflection - it's back, though weaker */
					/* Experimental: Boomerangs can't be deflected - C. Blue */
					if (!boomerang && (r_ptr->flags2 & RF2_REFLECTING) && magik(50)) {
						if (visible) msg_format(Ind, "The %s was deflected.", o_name);
						if (!num_ricochet) { /* Gain ricochet (for bows and xbows, who don't have it natively) */
							num_ricochet = 1;
							hit_body = 1;
						}
						aimed_ricochet = FALSE; /* In any case, ricocheting off a reflector is random and not aimable anymore even with skillz */
						if (!boomerang && !magic && o_ptr->pval) do_arrow_explode(Ind, o_ptr, wpos, y, x, tmul);
						break;
					}

//TODO: pseudo-"dodging", parrying
#ifdef USE_BLOCKING
					/* handle blocking (deflection) */
					if (strchr("hHJkpPtyn", r_ptr->d_char) && /* leaving out Yeeks (else Serpent Man 'J') */
					    !(r_ptr->flags3 & RF3_ANIMAL) && !(r_ptr->flags8 & RF8_NO_BLOCK) && !m_ptr->csleep && m_ptr->stunned <= 100 && !m_ptr->confused
					    && !rand_int(24 - r_ptr->level / 10)) { /* small chance to block arrows */
						if (visible) {
							char hit_desc[MAX_CHARS + 12];

							sprintf(hit_desc, "\377%c%s blocks.", COLOUR_BLOCK_MON, m_name);
							hit_desc[2] = toupper(hit_desc[2]);
							msg_print(Ind, hit_desc);
						}
						hit_body = 1;
						if (!boomerang && !magic && o_ptr->pval) do_arrow_explode(Ind, o_ptr, wpos, y, x, tmul);
						break;
					}
#endif

					/* Handle unseen monster */
					if (!visible) {
						/* Invisible monster */
						msg_format(Ind, "The %s finds a mark.", o_name);
					}
					/* Handle visible monster */
					else {
						/* Message */
						msg_format(Ind, "The %s hits %s.", o_name, m_name);

						/* Hack -- Track this monster race */
						if (visible) recent_track(m_ptr->r_idx);

						/* Hack -- Track this monster */
						if (visible) health_track(Ind, c_ptr->m_idx);
					}

					if (!boomerang) {
						/* Base damage from thrown object plus launcher bonus */
						tdam = damroll(o_ptr->dd, o_ptr->ds);
						tdam = brand_dam_aux(Ind, o_ptr, -1, tdam, m_ptr, FALSE);
						if (p_ptr->ranged_flare) tdam += damroll(2, 6); /* compare to dice in k_info for oil flask */
						if (p_ptr->ammo_brand) tdam += p_ptr->ammo_brand_d;
						tdam += o_ptr->to_d;
						tdam += j_ptr->to_d + p_ptr->to_d_ranged;

						/* Boost the damage */
						tdam *= tmul;

#ifdef DEFENSIVE_STANCE_FIXED_RANGED_REDUCTION
						if (p_ptr->combat_stance == 1) tdam /= 2;
#endif
					} else {
						 /* Base damage from thrown object */
						tdam = damroll(o_ptr->dd, o_ptr->ds);
#if defined(R_VORPAL_UNBRANDED) || defined(R_VORPAL_LOWBRANDED)
						if ((f5 & TR5_VORPAL) && !(r_ptr->flags8 & RF8_NO_CUT) && !rand_int(R_VORPAL_CHANCE)) vorpal_cut = tdam; /* save unbranded dice */
						else vorpal_cut = FALSE;
#endif
						tdam = brand_dam_aux(Ind, o_ptr, -1, tdam, m_ptr, FALSE);
						tdam += o_ptr->to_d; //moved this here, or vorpal dam-addon will be too low
#ifdef R_VORPAL_LOWBRANDED
						if (vorpal_cut) vorpal_cut = (vorpal_cut + tdam) / 2;
#else
 #ifndef R_VORPAL_UNBRANDED
						if ((f5 & TR5_VORPAL) && !(r_ptr->flags8 & RF8_NO_CUT) && !rand_int(R_VORPAL_CHANCE)) vorpal_cut = tdam; /* save branded dice */
						else vorpal_cut = FALSE;
 #endif
#endif
						tdam += p_ptr->to_d_ranged;

						/* Boost the damage */
						tdam = (tdam * tmul) / 10;

#ifdef DEFENSIVE_STANCE_FIXED_RANGED_REDUCTION
						if (p_ptr->combat_stance == 1) tdam /= 2;
#endif
					}
					ranged_flare_body = TRUE;

					/* Apply special damage XXX XXX XXX */
					if (!p_ptr->ranged_precision)
						tdam = critical_shot(Ind, o_ptr->weight, o_ptr->to_h, tdam,
						    calc_crit_obj(o_ptr) + (boomerang ? 0 : calc_crit_obj(j_ptr)), FALSE, !boomerang);
					else /* Precision shot skips most AC reduction */
						tdam = critical_shot(Ind, o_ptr->weight, o_ptr->to_h + 500, tdam,
						    calc_crit_obj(o_ptr) + (boomerang ? 0 : calc_crit_obj(j_ptr)), TRUE, !boomerang);

					/* Vorpal bonus - multi-dice!
					   (currently +31.25% more branded dice damage on total average, just for the records) */
					if (vorpal_cut) {
						msg_format(Ind, "Your boomerang cuts deep into %s!", m_name);
#ifdef R_CRIT_VS_VORPAL
    xxx disabled xxx
						k2 += (magik(25) ? 2 : 1) * (vorpal_cut + 5); /* exempts critical strike */
						/* either critical hit or vorpal, not both */
						if (k2 > k) k = k2;
#else
						tdam += (magik(25) ? 2 : 1) * (vorpal_cut + 5); /* exempts critical strike */
#endif
					}

					if (p_ptr->ranged_precision)
						; /* < --- TODO? Maybe in the future: apply monster AC for damage reduction here --- > */
					else
						p_ptr->ranged_precision = FALSE;

					if (ranged_double_real) tdam = (tdam * 35) / 100;

					if (p_ptr->ranged_barrage) tdam *= 2; // maybe 3 even

					if (m_ptr->r_idx == RI_MIRROR) tdam = (tdam * MIRROR_REDUCE_DAM_TAKEN_RANGED + 99) / 100;

#ifdef ENABLE_OUNLIFE
					/* Wraithstep gets auto-cancelled on forced interaction with solid environment */
					if (p_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1) &&
						((r_ptr->flags2 & RF2_KILL_WALL) || !(r_ptr->flags2 & RF2_PASS_WALL)))
						    set_tim_wraith(Ind, 0);
#endif
					/* can't attack while in WRAITHFORM (explosion still works) */
					/* wraithed players can attack wraithed monsters - mikaelh */
					if (p_ptr->tim_wraith &&
					    ((r_ptr->flags2 & RF2_KILL_WALL) || !(r_ptr->flags2 & RF2_PASS_WALL)))
						tdam = 0;

					/* No negative damage */
					if (tdam < 0) tdam = 0;

#if 0
					/* XXX consider using project() with GF_CONF */
					if ((p_ptr->ammo_brand && (p_ptr->ammo_brand_t == TBRAND_CHAO)) &&
					    !(r_ptr->flags3 & RF3_NO_CONF) &&
					    !(r_ptr->flags4 & RF4_BR_CONF) &&
					    !(r_ptr->flags4 & RF4_BR_CHAO) && !boomerang) {
						int i;

						/* Already partially confused */
						if (m_ptr->confused) i = m_ptr->confused + p_ptr->lev;
						/* Was not confused */
						else i = p_ptr->lev;
						/* Apply confusion */
						m_ptr->confused = (i < 200) ? i : 200;

						if (visible) {
							char m_name[MNAME_LEN];

							/* Get "the monster" or "it" */
							monster_desc(Ind, m_name, c_ptr->m_idx, 0);

							/* Message */
							msg_format(Ind, "%^s appears confused.", m_name);
						}
					}
#endif	// 0

					/* Vampiric drain  - Launcher or ammo can be vampiric!
					   - for now vampiric from items helps a bit,
					   - vampiric from vampire form doesn't (relies on melee-biting!) */
					if ((magik(p_ptr->vampiric_ranged)) && drainable)
						drain_result = m_ptr->hp;
					else
						drain_result = 0;

					if (p_ptr->admin_godly_strike) {
						p_ptr->admin_godly_strike--;
						if (!(r_ptr->flags1 & RF1_UNIQUE)) tdam = m_ptr->hp + 1;
					}

#define RI_MIRROR_REDUCED_RANGED 4
#ifdef RI_MIRROR_REDUCED_RANGED
					/* Experimental: Extra damage reduction vs shooting */
					if (m_ptr->r_idx == RI_MIRROR) tdam = (tdam + RI_MIRROR_REDUCED_RANGED - 1) / RI_MIRROR_REDUCED_RANGED;
#endif

					/* Hit the monster, check for death */
					if (mon_take_hit(Ind, c_ptr->m_idx, tdam, &fear, note_dies)) {
						/* note: if the monster we hit wasn't the one targetted, then continue shooting.
							 It can only mean that this monster was invisible to us, hence the
							 character couldn't have control over avoiding to target it.
							 Edit: Or it was a ricochet! */
						if (dir == 5 && y == ty && x == tx && !random_ricochetting)
							/* Dead targetted monster */
							p_ptr->shooting_till_kill = FALSE;

#ifdef USE_SOUND_2010
						/* hack: always play 'hit' sfx for final killing shot,
						   so if we didn't play it already (we did so if sfx==0) then play it now instead. */
						if (sfx && p_ptr->sfx_combat)
							switch (o_ptr->tval) {
							case TV_SHOT: sound(Ind, "fire_shot", NULL, SFX_TYPE_ATTACK, FALSE); break;
							case TV_ARROW: sound(Ind, "fire_arrow", NULL, SFX_TYPE_ATTACK, FALSE); break;
							case TV_BOLT: sound(Ind, "fire_bolt", NULL, SFX_TYPE_ATTACK, FALSE); break;
							case TV_BOOMERANG: sound(Ind, "fire_boomerang", NULL, SFX_TYPE_ATTACK, FALSE); break;
							}
#endif
					}
					/* No death */
					else {
						/* Message */
						message_pain(Ind, c_ptr->m_idx, tdam);

						/* VAMPIRIC: Are we draining it?  A little note: If the monster is
						dead, the drain does not work... */
						if (drain_result) {
							drain_result -= m_ptr->hp;  /* Calculate the difference */

							if (drain_result > 0) { /* Did we really hurt it? */
								drain_heal = randint(2) + damroll(2,(drain_result / 16));/* was 4,../6 -- was /8 for 50 max_drain */

								if (drain_left) {
									if (drain_heal < drain_left) {
										drain_left -= drain_heal;
									} else {
										drain_heal = drain_left;
										drain_left = 0;
									}

									if (drain_msg) {
										msg_format(Ind, "Your shot drains life from %s!", m_name);
										drain_msg = FALSE;
									}

									if (m_ptr->r_idx == RI_BLUE) drain_heal >>= 1;
									i = p_ptr->chp;
									hp_player(Ind, drain_heal, TRUE, TRUE);
									p_ptr->test_drain += (p_ptr->chp - i);
									/* We get to keep some of it! */
								}
							}
						}

						if (p_ptr->ranged_barrage) {
							if (!(r_ptr->flags3 & RF3_NO_STUN)) {
								if (!m_ptr->stunned) msg_format(Ind, "\377y%^s is stunned.", m_name);
								else msg_format(Ind, "\377y%^s appears more stunned.", m_name);
								m_ptr->stunned = m_ptr->stunned + 20 + get_skill_scale(p_ptr, SKILL_COMBAT, 5);
							} else {
								msg_format(Ind, "\377o%^s resists the effect.", m_name);
							}
						}

						/* Take note */
						if (fear && visible && !(m_ptr->csleep || m_ptr->stunned > 100)) {
							char m_name[MNAME_LEN];
#ifdef USE_SOUND_2010
#else
							sound(Ind, SOUND_FLEE);
#endif
							/* Get the monster name (or "it") */
							monster_desc(Ind, m_name, c_ptr->m_idx, 0);

							/* Message */
							if (m_ptr->r_idx != RI_MORGOTH)
								msg_format(Ind, "%^s flees in terror!", m_name);
							else
								msg_format(Ind, "%^s retreats!", m_name);
						}
					}

					if (!boomerang && p_ptr->ammo_brand_t)
						do_arrow_brand_effect(Ind, y, x);

					/* Ammo pval takes precedence - Kurzel */
					if (!boomerang && !magic && o_ptr->pval)
						do_arrow_explode(Ind, o_ptr, wpos, y, x, tmul);
					else if (p_ptr->nimbus) do_nimbus(Ind, y, x);

					/* Stop looking */
#if 0
					/* We hit an entity (even if we didn't pass AC test), so we MUST break,
					   for ricochetting and all. Not sure what the next line is about, but it needs adjusting:
					   We must always break here, or ricochetting must be disabled, or hit_body evaluation must be expanded. */
					if (!p_ptr->ammo_brand || (p_ptr->ammo_brand_t != TBRAND_VORP) || boomerang) break;
#else
					break;
#endif
				}
			}

			/* server crashed further below when ny was 66, dir was 10 but then became 3, odd stuff.. */
			if (in_bounds_array(ny, nx))
#ifdef DOUBLE_LOS_SAFETY
			/* skip checks if we already used projectable..() routines to test. */
			if (dir != 5 || random_ricochetting) {
#endif
#ifdef PY_FIRE_ON_WALL
				/* Stopped by walls/doors */
				if (!cave_contact(zcave, ny, nx)) break;
#endif
#ifdef DOUBLE_LOS_SAFETY
			}
#endif
		}

		/* Extra (exploding) hack: */
#ifdef DOUBLE_LOS_SAFETY
		/* skip checks if we already used projectable..() routines to test. */
		if (dir != 5 || random_ricochetting) {
#endif
			/* server crashed here when ny was 66, dir was 10 but then became 3, odd stuff.. */
			if (in_bounds_array(ny, nx) &&
			    !boomerang && !magic && o_ptr->pval) {
				if (!cave_contact(zcave, ny, nx)) /* Stopped by walls/doors ?*/
#ifndef PY_FIRE_ON_WALL
					do_arrow_explode(Ind, o_ptr, wpos, y, x, tmul);
#else
					/* important: explode BEFORE wall, or we could target a wall grid and hit stuff beyond it */
					do_arrow_explode(Ind, o_ptr, wpos, prev_y, prev_x, tmul);
#endif
				else if (dir == 5 && !target_ok) /* fired 'at oneself'? */
					do_arrow_explode(Ind, o_ptr, wpos, y, x, tmul);
			}
#ifdef DOUBLE_LOS_SAFETY
		} else if (!target_ok /* fired 'at oneself'? */
		    && !boomerang && !magic && o_ptr->pval) {
			do_arrow_explode(Ind, o_ptr, wpos, y, x, tmul);
		}
#endif

		/*todo: even if not hit_body, boomerangs should have chance to drop to the ground.. */

		/* Chance of breakage (during attacks). (Note: break_chance for boomerangs at this point in code is breakage_chance(), which was 2.) */
		if (archery == SKILL_BOOMERANG) {
			/* finer resolution to match reduced break rate of boomerangs - C. Blue */
			j = (hit_body ? break_chance : 0) * 100;
			//j = (j * (1000 - get_skill_scale(p_ptr, archery, 950))) / 1000;
			j = (j * (2500 - get_skill_scale(p_ptr, archery, 2450))) / 5000;
		} else {
			j = (hit_body ? break_chance : break_chance / 3) * 100;
			//j = (j * (100 - get_skill_scale(p_ptr, archery, 80))) / 100; <- for when base ammo break chance was 50% in breakage_chance
			j = (j * (100 - get_skill_scale(p_ptr, archery, 90))) / 100;
		}

		/* Break ammo? (In case of boomerangs this turns into a drop. Chance at the time of writing: 1 in 5000.) */
		if ((((o_ptr->pval != 0) && !boomerang) || (rand_int(10000) < j))
		    && !magic && !ethereal && !artifact_p(o_ptr)
		    && !(boomerang && (o_ptr->name2 || o_ptr->name2b))) { /* <- 2024 for rune sigils - ego-boomerangs don't drop to the floor either, like arts */
			/* Remember as broken */
			breakage = 100;

			if (boomerang) { /* change a large part of the break chance to actually
					  result in not returning instead of real breaking - C. Blue */
#ifdef INDESTRUCTIBLE_BOOMERANGS
				if (TRUE) {
#else
				if (o_ptr->name2 || o_ptr->name2b || magik(90)) { /* Let egos not be destroyed but just drop */
#endif
					msg_format(Ind, "\377oYour %s drops to the ground.",o_name);

					if (!p_ptr->warning_boomerang) {
						msg_print(Ind, "\374\377yHINT: Press '\377o{\377y' key and inscribe your boomerang '\377o!=L\377y'");
						msg_print(Ind, "\374\377y      to automatically pick it up and equip it just by moving over it.");
						s_printf("warning_boomerang: %s\n", p_ptr->name);
						p_ptr->warning_boomerang = 1;
					}
#if 1 /* hope this works */
					inven_item_increase(Ind, item, -1);
					inven_item_optimize(Ind, item);
					drop_near(TRUE, Ind, &throw_obj, 0, wpos, y, x);
#else /* silly - like failing to catch it instead of it not returning ;-p */
					inven_drop(TRUE, Ind, INVEN_BOW, 1, TRUE);
#endif
				} else {
					msg_format(Ind, "\377oYour %s breaks.", o_name);
					inven_item_increase(Ind, item, -1);
					inven_item_optimize(Ind, item);
				}

				/* Recalculate boni */
				p_ptr->update |= (PU_BONUS);
				/* Recalculate torch */
				p_ptr->update |= (PU_TORCH);
				/* Recalculate mana */
				p_ptr->update |= (PU_MANA | PU_HP | PU_SANITY);
				/* Redraw */
				p_ptr->redraw |= (PR_PLUSSES | PR_ARMOR);
				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
			}

			/* Ammo or boomerang is destroyed. */
			break;
		}

		/* If no break, the ammo can ricochet */
		if (num_ricochet && hit_body && magik(ricochet_chance) && !p_ptr->ranged_barrage) {
			int avoid_dir = determine_dir(bx, by, x, y);

			/* New base location */
			tx = by = y;
			tx = bx = x;

			if (aimed_ricochet) {
				/* skillfulyl play pool or billard */
				random_ricochetting = !get_outward_target(Ind, &tx, &ty, tdis, avoid_dir, FALSE, aimed_ricochet > 10, TRUE);
				/* if there are no eligible target grids (happens if even random grids all fail because we'd hit a sleeping/charmed monster), don't ricochet! */
				if (random_ricochetting && tx == x && ty == y) break;
			} else
				/* New, completely random target location */
				random_ricochetting = !get_outward_target(Ind, &tx, &ty, tdis, avoid_dir, TRUE, FALSE, FALSE);

			msg_format(Ind, "The %s ricochets!", o_name);
			hit_body = FALSE;
			num_ricochet--;
			continue;
		}

		/* Shot has finally arrived at the target destination */
		break;
	}

#ifndef OPTIMIZED_ANIMATIONS
	/* Back in the U.S.S.R */
	if (boomerang && !breakage) {
		/* Save the old "player pointer" */
		q_ptr = p_ptr;

		/* For boomerang-ricocheting: tx/ty are far-distance targets (99 * ddx/ddy), reset them to the actual position! */
		tx = x;
		ty = y;

		for (cur_dis = 0; cur_dis <= tdis; ) {
			/* Hack -- Stop at the target */
			if ((y == q_ptr->py) && (x == q_ptr->px)) break;

			/* Calculate the new location (see "project()") */
			ny = y;
			nx = x;
			//mmove2(&ny, &nx, p_ptr->py, p_ptr->px, ty, tx);
			mmove2(&ny, &nx, ty, tx, q_ptr->py, q_ptr->px);

 #ifdef DOUBLE_LOS_SAFETY
			/* skip checks if we already used projectable..() routines to test. */
			if (dir != 5 || random_ricochetting) {
 #endif
				/* Stopped by walls/doors */
				if (!cave_contact(zcave, ny, nx)) break;
 #ifdef DOUBLE_LOS_SAFETY
			}
 #endif

			/* Advance the distance */
			cur_dis++;

			/* Save the new location */
			x = nx;
			y = ny;

			/* Display it for each player */
			for (i = 1; i <= NumPlayers; i++) {
				int dispx, dispy;

				/* Use this player */
				p_ptr = Players[i];

				/* If he's not playing, skip him */
				if (p_ptr->conn == NOT_CONNECTED) continue;

				/* If he's not here, skip him */
				if (!inarea(&p_ptr->wpos, wpos)) continue;

				/* The player can see the (on screen) missile */
				if (panel_contains(y, x) && player_can_see_bold(i, y, x)) {
					/* Draw, Hilite, Fresh, Pause, Erase */
					dispy = y - p_ptr->panel_row_prt;
					dispx = x - p_ptr->panel_col_prt;

					/* Remember the projectile */
					p_ptr->scr_info[dispy][dispx].c = missile_char;
					p_ptr->scr_info[dispy][dispx].a = missile_attr;

					/* Tell the client */
#ifdef GRAPHICS_BG_MASK
					Send_char(i, dispx, dispy, missile_attr, missile_char, 0, 0);
#else
					Send_char(i, dispx, dispy, missile_attr, missile_char);
#endif

					/* Flush and wait */
					if (cur_dis % (boomerang ? 2 : tmul)) Send_flush(i);

					/* Restore */
					lite_spot(i, y, x);
				}
			}
		}
		/* Restore the player pointer */
		p_ptr = q_ptr;
	}
#endif /* OPTMIZED_ANIMATIONS */

#ifdef OPTIMIZED_ANIMATIONS
	if (path_num) {
		/* Pick a random spot along the path */
		j = rand_int(path_num);
		ny = path_y[j];
		nx = path_x[j];

		/* Save the old "player pointer" */
		q_ptr = p_ptr;

		/* Draw a projectile here for everyone */
		for (i = 1; i <= NumPlayers; i++) {
			int dispx, dispy;

			/* Use this player */
			p_ptr = Players[i];

			/* If he's not playing, skip him */
			if (p_ptr->conn == NOT_CONNECTED) continue;

			/* If he's not here, skip him */
			if (!inarea(&p_ptr->wpos, wpos)) continue;

			/* The player can see the (on screen) missile */
			if (panel_contains(ny, nx) && player_can_see_bold(i, ny, nx)) {
				/* Draw */
				dispy = ny - p_ptr->panel_row_prt;
				dispx = nx - p_ptr->panel_col_prt;

				/* Remember the projectile */
				p_ptr->scr_info[dispy][dispx].c = missile_char;
				p_ptr->scr_info[dispy][dispx].a = missile_attr;

				/* Tell the client */
#ifdef GRAPHICS_BG_MASK
				Send_char(i, dispx, dispy, missile_attr, missile_char, 0, 0);
#else
				Send_char(i, dispx, dispy, missile_attr, missile_char);
#endif

				/* Flush once */
				Send_flush(i);
			}

			/* Restore later */
			everyone_lite_later_spot(wpos, ny, nx);
		}

		/* Restore the player pointer */
		p_ptr = q_ptr;
	}
#endif /* OPTIMIZED_ANIMATIONS */

	/* Hack -- "Never litter the floor" inscription {!g} */
	if (check_guard_inscription(o_ptr->note, 'b'))
	    //|| p_ptr->max_plv < cfg.newbies_cannot_drop)
		breakage = 101;

	if (p_ptr->ranged_flare && !boomerang) {
		if (!hit_body && !ranged_flare_body) {
			object_type forge;

			(void)project(0 - Ind, 2, wpos, y, x, damroll(2, 6), GF_FLARE, PROJECT_NORF | PROJECT_GRID | PROJECT_KILL | PROJECT_NODO, "");

#ifndef PY_FIRE_ON_WALL
			lite_room(Ind, wpos, y, x); //lite_area()?
#else
			lite_room(Ind, wpos, prev_y, prev_x); //lite_area()?
#endif
			breakage = 0;
			invcopy(&forge, lookup_kind(o_ptr->tval, SV_AMMO_CHARRED));
			forge.to_h = -rand_int(10);
			forge.to_d = -rand_int(10);
			*o_ptr = forge;
			o_ptr->ident |= ID_KNOWN | ID_SENSED_ONCE;

			if (magic) { /* hack: the one exception where magic ammo doesn't return */
				inven_item_increase(Ind, item, -1);
				inven_item_describe(Ind, item);
				inven_item_optimize(Ind, item);
				/* Window stuff */
				//p_ptr->window |= (PW_EQUIP);
			}
		} else {
			breakage = 101; /* projectile gets destroyed by burning brightly + hitting someone */
		}
	}

	p_ptr->ranged_flare = FALSE;
	p_ptr->ranged_barrage = FALSE; /* whether we hit a monster or fired into empty air (ie missed).. */

	/* Drop (or break) near that location */
	/* by C. Blue - Now art ammo never drops, since it doesn't run out */
	if (!returning) {
#ifdef PY_FIRE_ON_WALL
		if (!cave_los(zcave, y, x)) /* target coordinates were in a wall? */
			/* drop it right there _before_ the wall, not 'in' the wall and then calling scatter().. */
			drop_near(TRUE, Ind, o_ptr, breakage, wpos, prev_y, prev_x);
		else
#endif
		drop_near(TRUE, Ind, o_ptr, breakage, wpos, y, x);
	}

	suppress_message = FALSE;

#if 1 //what was the point of this hack again?..
	/* hack for auto-retaliator hack in dungeon.c fire_till_kill-related call of do_cmd_fire() */
	p_ptr->auto_retaliating = FALSE;
#endif

	if (warn) {
		msg_print(Ind, "\374\377oHINT: Create a '\377Rmacro\377o' aka hotkey to fire your ranged weapon with a single");
		msg_print(Ind, "\374\377o      keypress: Press '\377R%\377o' and then '\377Rz\377o' to invoke the macro wizard!");
		p_ptr->warning_macros = 1;
		s_printf("warning_macros (fire): %s\n", p_ptr->name);
	}
}

/*
 * Check if neighboring monster(s) interferes with player's action.  - Jir -
 */
bool interfere(int Ind, int chance) {
	player_type *p_ptr = Players[Ind], *q_ptr;
	monster_race *r_ptr;
	monster_type *m_ptr;
	int d, i, tx, ty, x = p_ptr->px, y = p_ptr->py;
	int calmness = get_skill_scale(p_ptr, SKILL_CALMNESS, 80);
	cave_type **zcave;

	if (!(zcave = getcave(&p_ptr->wpos))) return(FALSE);

	/* monster doesn't know the player is actually next to it?
	   (ignore cloak_neutralized for the time being) */
	if (p_ptr->cloaked) return(FALSE);

	/* too fast to get grabbed! */
	if (p_ptr->shadow_running) return(FALSE);

	/* cannot interfere with ghosts */
	if (p_ptr->ghost) return(FALSE);

	chance = chance * (100 - calmness) / 100;

	/* Check if monsters around him/her hinder the action */
	for (d = 1; d <= 9; d++) {
		if (d == 5) continue;

		tx = x + ddx[d];
		ty = y + ddy[d];

		if (!in_bounds(ty, tx)) continue;

		if (!(i = zcave[ty][tx].m_idx)) continue;
		if (i > 0) {
			m_ptr = &m_list[i];
			r_ptr = race_inf(m_ptr);
			//if (r_info[m_list[i].r_idx].flags2 & RF2_NEVER_MOVE)
			/* monster doesn't act? */
			if (r_ptr->flags2 & RF2_NEVER_MOVE) continue;
			if (r_ptr->flags2 & RF2_NEVER_ACT) continue;
			if (m_ptr->status & M_STATUS_FRIENDLY) continue;
			/* Sleeping etc.. monsters don't interfere o_O - C. Blue */
			if (m_ptr->csleep || m_ptr->monfear || m_ptr->stunned || m_ptr->confused)
				continue;
			/* Pet never interfere  - the_sandman */
			if (m_ptr->pet) continue;
		} else {
			q_ptr = Players[-i];
			/* hostile player? */
			if (!check_hostile(Ind, -i) ||
			    q_ptr->paralyzed || q_ptr->stun > 100 || q_ptr->confused || q_ptr->afraid ||
			    (r_info[q_ptr->body_monster].flags2 & RF2_NEVER_MOVE) ||
			    (r_info[q_ptr->body_monster].flags2 & RF2_NEVER_ACT))
				continue;
#ifdef ENABLE_STANCES
			if (q_ptr->combat_stance == 1) switch (q_ptr->combat_stance_power) {
				case 0: chance += get_skill_scale(q_ptr, SKILL_INTERCEPT, 23); break;
				case 1: chance += get_skill_scale(q_ptr, SKILL_INTERCEPT, 26); break;
				case 2: chance += get_skill_scale(q_ptr, SKILL_INTERCEPT, 29); break;
				case 3: chance += get_skill_scale(q_ptr, SKILL_INTERCEPT, 35); break;
			} else if (q_ptr->combat_stance == 2) switch (q_ptr->combat_stance_power) {
				case 0: chance += get_skill_scale(q_ptr, SKILL_INTERCEPT, 53); break;
				case 1: chance += get_skill_scale(q_ptr, SKILL_INTERCEPT, 56); break;
				case 2: chance += get_skill_scale(q_ptr, SKILL_INTERCEPT, 59); break;
				case 3: chance += get_skill_scale(q_ptr, SKILL_INTERCEPT, 65); break;
			} else
#endif
			chance += get_skill_scale(q_ptr, SKILL_INTERCEPT, 50);
		}

		/* Cannot grab what you cannot see */
		//todo: implement for monsters (if (!p_ptr->mon_vis[m_idx]) continue;)
		//basically, we could try to use 'inv' flag that is set to player_invis() and save it in m_ptr, to access it here?

		if (chance > 95) chance = 95;
		if (rand_int(100) < chance) {
			char m_name[MNAME_LEN];

			if (i > 0) {
				monster_desc(Ind, m_name, i, 0);
			} else {
				/* FIXME: even not visible... :( */
				//strcpy(m_name, q_ptr->name);
				/* fixed :) */
				player_desc(Ind, m_name, -i, 0);
			}
			msg_format(Ind, "\377%c%^s interferes with your attempt!", COLOUR_IC_MON, m_name);
			return(TRUE);
		}
	}

	return(FALSE);
}


/*
 * Throw an object from the pack or floor.
 *
 * Note: "unseen" monsters are very hard to hit.
 *
 * Should throwing a weapon do full damage?  Should it allow the magic
 * to hit bonus of the weapon to have an effect?  Should it ever cause
 * the item to be destroyed?  Should it do any damage at all?
 */
void do_cmd_throw(int Ind, int dir, int item, char bashing) {
	player_type *p_ptr = Players[Ind], *q_ptr;
	struct worldpos *wpos = &p_ptr->wpos;

	int i, j, y, x, ny, nx, ty, tx, wall_x, wall_y;
	int chance, tdam, tdis, k2, k3, vorpal_cut = 0, chaos_effect = 0, instakills;
	int mul, div;
	int cur_dis, visible, real_dis;
	int moved_number = 1, original_number;
	int start_ix = 0, start_iy = 0;

	object_type throw_obj;
	object_type *o_ptr;

	bool hit_body = FALSE, target_ok = target_okay(Ind);
	bool hit_wall = FALSE;
	bool returning, blessed_weapon, old_instakills;

	int missile_attr;
	int missile_char;

#ifdef OPTIMIZED_ANIMATIONS
	/* Projectile path */
	int path_y[MAX_RANGE];
	int path_x[MAX_RANGE];
	int path_num = 0;
#endif

	char o_name[ONAME_LEN];
	u32b f1, f2, f3, f4, f5, f6, esp;
	bool throwing_weapon;
	cave_type **zcave, *c_ptr;


	if (!(zcave = getcave(wpos))) return;

	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST) {
		msg_print(Ind, "You cannot throw or bash things in mist form.");
		return;
	}

	/* New in 4.7.3a+: Throw equipment.
	   Limit it though, cannot throw all kind of pieces - armour/jewelry needs to be taken off first. */
	if (item > INVEN_BOW && item != INVEN_LITE && item < INVEN_AMMO) {
		msg_print(Ind, "You must take off this item first to throw it.");
		return;
	}

	/* New '+' feat in 4.4.6.2 */
	if (dir == 11) {
		get_aim_dir(Ind);
		p_ptr->current_throw = item;
		return;
	}

	if (dir == 5) {
		if (target_ok) {
#ifndef PY_FIRE_ON_WALL
			if (!projectable_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE))
#else
			if (!projectable_wall_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE))
#endif
				return;
		} else return;
	}

	/* throwing works even while in WRAITHFORM (due to slow speed of
	   object, allowing it to lose the wraithform ;) */

	/* Break goi/manashield */
#if 0
	if (p_ptr->invuln) set_invuln(Ind, 0);
	if (p_ptr->tim_manashield) set_tim_manashield(Ind, 0);
	if (p_ptr->tim_wraith) set_tim_wraith(Ind, 0);
#endif	// 0

	/* Access the item (if in the pack) */
	if (item >= 0) o_ptr = &(p_ptr->inventory[item]);
	else {
		if (-item >= o_max) return; /* item doesn't exist */
		o_ptr = &o_list[0 - item];
		start_ix = o_ptr->ix;
		start_iy = o_ptr->iy;
	}

	if (o_ptr->tval == 0) {
		if (!bashing) msg_print(Ind, "\377rYou cannot throw that.");
		else msg_print(Ind, "\377rYou cannot bash that.");
		return;
	}

	if (!bashing) {
		/* Handle the newbies_cannot_drop option */
#if (STARTEQ_TREATMENT == 1)
		if ((p_ptr->max_plv < cfg.newbies_cannot_drop || p_ptr->inval) && !is_admin(p_ptr) &&
		    !exceptionally_shareable_item(o_ptr) && o_ptr->tval != TV_GAME &&
		    !(o_ptr->tval == TV_PARCHMENT && (o_ptr->sval == SV_DEED_HIGHLANDER || o_ptr->sval == SV_DEED_DUNGEONKEEPER))) {
/*			msg_format(Ind, "Please don't litter the %s.",
			    istown(wpos) ? "town":(wpos->wz ? "dungeon":"Nature"));*/
			msg_format(Ind, "You need to be at least level %d to throw items.", cfg.newbies_cannot_drop);
			return;
		}
#endif
	}

	if (!bashing && check_guard_inscription(o_ptr->note, 'v')) {
		msg_print(Ind, "\377yThe item's inscription prevents it.");
		return;
	};

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
	/* Equipped items only! */
	returning = (f6 & TR6_RETURNING) && (item >= INVEN_WIELD);
	blessed_weapon = (f3 & TR3_BLESSED);

	/* Hack - Cannot throw away 'no drop' cursed items */
	if (cursed_p(o_ptr) && (f4 & TR4_CURSE_NO_DROP) && item >= 0 && !bashing) {
		/* Oops */
		msg_print(Ind, "\377yHmmm, you seem to be unable to throw it.");

		/* Nope */
		return;
	}

	if (o_ptr->questor) {
		if (p_ptr->rogue_like_commands)
			msg_print(Ind, "\377yYou can't drop this item. Use '\377oCTRL+d\377y' to destroy it. (Might abandon the quest!)");
		else
			msg_print(Ind, "\377yYou cannot drop this item. Use '\377ok\377y' to destroy it. (Might abandon the quest!)");
		return;
	}

	if (!bashing) {
#if (STARTEQ_TREATMENT > 1)
		/* Items belonging to and then being dropped by a character whose level is < cfg.newbies_cannot_drop become unsalable. */
		if (o_ptr->owner == p_ptr->id && p_ptr->max_plv < cfg.newbies_cannot_drop && !is_admin(p_ptr) &&
		    o_ptr->tval != TV_GAME && o_ptr->tval != TV_KEY && o_ptr->tval != TV_SPECIAL) {
			/* not for basic arrows, a bit too silyl compared to the annoyment/newbie confusion.
			   Basically, we want to turn everything level 0 that he could buy in town shops and then drop for someone else to pick up and utilize/monetize. */
			if ((!is_ammo(o_ptr->tval) || o_ptr->name1 || o_ptr->name2) && o_ptr->tval != TV_CHEST) o_ptr->level = 0;
			o_ptr->mode |= MODE_STARTER_ITEM; //hack: mark as unsellable
		}
#endif
	}

	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
	bandage_fails(Ind);


	/* Create a "local missile object" */
	throw_obj = *o_ptr;
	original_number = o_ptr->number;
	/* hack: if we kick (bash) certain types of items around, move the
	   whole stack instead of just one.
	   note: currently number has no effect on throwing range (weight!) */
	if (bashing) {
		if ((o_ptr->tval == TV_LITE && o_ptr->sval == SV_LITE_TORCH)
		    || is_cheap_misc(o_ptr->tval))
		    // || is_ammo(o_ptr->tval))
			moved_number = o_ptr->number;
	}
	throw_obj.number = moved_number;


	/*
	 * Hack -- If rods or wands are dropped, the total maximum timeout or
	 * charges need to be allocated between the two stacks.  If all the items
	 * are being dropped, it makes for a neater message to leave the original
	 * stack's pval alone. -LM-
	 */
	if (is_magic_device(o_ptr->tval)) divide_charged_item(&throw_obj, o_ptr, 1);

	/* Reduce and describe inventory */
	if (item >= 0) {
		if (!returning) {
#ifdef ENABLE_SUBINVEN
			/* If we drop a subinventory, remove all items and place them into the player's inventory */
			if (o_ptr->tval == TV_SUBINVEN && moved_number >= o_ptr->number) empty_subinven(Ind, item, FALSE, FALSE);
#endif

			inven_item_increase(Ind, item, -moved_number);
			inven_item_describe(Ind, item);
			inven_item_optimize(Ind, item);
		}
	}
	/* Reduce and describe floor item */
	else {
		floor_item_increase(0 - item, -moved_number);
		floor_item_optimize(0 - item);
	}

	/* Use the local object */
	o_ptr = &throw_obj;

	throwing_weapon = is_throwing_weapon(o_ptr);
	if (throwing_weapon) {
		/* Check is same as for heavy_wield */
		if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10) {
			msg_print(Ind, "\377oYou have trouble throwing such a heavy weapon effectively.");
			throwing_weapon = FALSE;
		}

		/* Check is same as for icky_wield, but just for message here. Actual penalties are applied in calc_boni_weapon().  */
		/* Priest weapon penalty for non-blessed edged weapons (assumes priests cannot dual-wield) */
		if ((p_ptr->pclass == CLASS_PRIEST && !blessed_weapon &&
		    (o_ptr->tval == TV_SWORD || o_ptr->tval == TV_POLEARM || o_ptr->tval == TV_AXE))
		    || (p_ptr->prace == RACE_VAMPIRE && blessed_weapon)
		    || (p_ptr->ptrait == TRAIT_CORRUPTED && blessed_weapon)
#ifdef ENABLE_CPRIEST
		    || (p_ptr->pclass == CLASS_CPRIEST && blessed_weapon)
#endif
#ifdef ENABLE_HELLKNIGHT
		    || (p_ptr->pclass == CLASS_HELLKNIGHT && blessed_weapon)
#endif
		    )
			msg_print(Ind, "\377oYou do not feel comfortable with that throwing weapon.");
	}

	/* Description */
	object_desc(Ind, o_name, o_ptr, FALSE, 3);


	/* Handle rugby ball */
	if (o_ptr->tval == TV_GAME && o_ptr->sval == SV_GAME_BALL && !bashing) {
		msg_print(Ind, "\377yYou pass the ball.");
		msg_format_near(Ind, "\377y%s passes the ball.", p_ptr->name);
	}

	/* Handle snowball */
	if (o_ptr->tval == TV_GAME && o_ptr->sval == SV_SNOWBALL) {
		/* Bashing will destroy it */
		if (bashing) {
			msg_print(Ind, "The snowball is pulverized.");
			return;
		}
	}


	/* Find the color and symbol for the object for throwing */
	missile_attr = object_attr(o_ptr);
	missile_char = object_char(o_ptr);


	/* Extract a "distance multiplier" */
	mul = 10;

	/* Enforce a minimum "weight" of one pound */
	div = ((o_ptr->weight > 10) ? o_ptr->weight : 10);

	/* Hack -- Distance -- Reward strength, penalize weight */
	tdis = (adj_str_blow[p_ptr->stat_ind[A_STR]] + 20) * mul / div;
	if (p_ptr->shero) tdis += 100 * mul / div;

	/* hack for rugby - all throws capped */
	if (o_ptr->tval == TV_GAME && o_ptr->sval == SV_GAME_BALL) {
		if (tdis < 5) tdis = 5;
	}

#ifdef ENABLE_DEMOLITIONIST
	/* Hack - Blast charges cannot be thrown thaaaat far (let's assume high airflow resistance =p) */
	if (o_ptr->tval == TV_CHARGE) tdis = (tdis * 7) / 10;
#endif

	/* Max distance of 10 for not effectively throwable weapons */
	if (throwing_weapon) {
		if (tdis > 15) tdis = 15;
	} else {
		if (tdis > 10) tdis = 10;
	}

	/* Fruit bat/water bashing? Actually limit throwing too! */
	if (bashing == 3) tdis = 0; /* non-fruit bats too: for bashing an item that is on our own grid */
	else if (bashing == 2) tdis = 1; /* at least allow minimal item movement, especially for bashing piles */

	/* Chance of hitting */
	chance = p_ptr->skill_tht + ((p_ptr->to_h + p_ptr->to_h_thrown) * BTH_PLUS_ADJ);
	if (p_ptr->blind) chance >>= 2;


	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);


	/* Start at the player */
	y = p_ptr->py;
	x = p_ptr->px;

	/* Predict the "target" location */
	tx = p_ptr->px + 99 * ddx[dir];
	ty = p_ptr->py + 99 * ddy[dir];

	/* Check for "target request" */
	if ((dir == 5) && target_ok) {
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}


	/* Hack -- Handle stuff */
	handle_stuff(Ind);

	if (bashing && o_ptr->custom_lua_usage) exec_lua(0, format("custom_object_usage(%d,%d,%d,%d,%d)", Ind, item < 0 ? -item : 0, item >= 0 ? item : -1, 12, o_ptr->custom_lua_usage));

	/* Hack this for throwing items: "for admins: kill a target in one hit" */
	instakills = (o_ptr->name1 == ART_SCYTHE_DM) ? ((o_ptr->note && strstr(quark_str(o_ptr->note), "IDDQD")) ? 2 : 1) : 0; //at doom's gate...

	/* Travel until stopped */
	for (cur_dis = real_dis = 0; real_dis <= tdis; ) {
		/* Hack -- Stop at the target */
		if ((y == ty) && (x == tx)) break;

		/* Calculate the new location (see "project()") */
		ny = y;
		nx = x;
		mmove2(&ny, &nx, p_ptr->py, p_ptr->px, ty, tx);

#ifdef DOUBLE_LOS_SAFETY
		/* skip checks if we already used projectable..() routines to
		   determine that it's fine, so it'd be redundant and also require
		   additional code to comply with DOUBLE_LOS_SAFETY. */
		if (dir != 5) {
#endif
			/* Stopped by protected grids (Inn doors, also stopping monsters' ranged attacks!) */
			if (f_info[zcave[ny][nx].feat].flags1 & (FF1_BLOCK_LOS | FF1_BLOCK_CONTACT)) break;

			/* Stopped by walls/doors */
			if (!cave_los(zcave, ny, nx)) {
				hit_wall = TRUE;
#if 1
				/* Hacky exception: If it's a snowball, allow him to actually hit a player on this wall grid! */
				if (o_ptr->tval != TV_GAME || o_ptr->sval != SV_SNOWBALL || zcave[ny][nx].m_idx >= 0)

#endif
				break;
			}

			/* stopped by open house doors! -
			   added this to prevent items landing ON an open door of a list house,
			   making it impossible to pick up the item again because the character
			   would enter the house when trying to step onto the grid with the item. - C. Blue */
			if (zcave[ny][nx].feat == FEAT_HOME_OPEN) break;

#ifdef DOUBLE_LOS_SAFETY
		}
#endif

		/* Advance the distance */
		cur_dis++;

		real_dis = distance(p_ptr->py, p_ptr->px, ny, nx);

		/* Save the new location */
		x = nx;
		y = ny;

#ifndef OPTIMIZED_ANIMATIONS
		/* Save the old "player pointer" */
		q_ptr = p_ptr;

		/* Display it for each player */
		for (i = 1; i <= NumPlayers; i++) {
			int dispx, dispy;

			/* Use this player */
			p_ptr = Players[i];

			/* If he's not playing, skip him */
			if (p_ptr->conn == NOT_CONNECTED) continue;

			/* If he's not here, skip him */
			if (!inarea(wpos, &p_ptr->wpos)) continue;

			/* The player can see the (on screen) missile */
			if (panel_contains(y, x) && player_can_see_bold(i, y, x)) {
				/* Draw, Hilite, Fresh, Pause, Erase */
				dispy = y - p_ptr->panel_row_prt;
				dispx = x - p_ptr->panel_col_prt;

				/* Remember the projectile */
				p_ptr->scr_info[dispy][dispx].c = missile_char;
				p_ptr->scr_info[dispy][dispx].a = missile_attr;

				/* Tell the client */
#ifdef GRAPHICS_BG_MASK
				Send_char(i, dispx, dispy, missile_attr, missile_char, 0, 0);
#else
				Send_char(i, dispx, dispy, missile_attr, missile_char);
#endif

				/* Flush and wait */
				if (cur_dis % 2) Send_flush(i);

				/* Restore */
				lite_spot(i, y, x);
			}
		}

		/* Restore the player pointer */
		p_ptr = q_ptr;

#else /* OPTIMIZED_ANIMATIONS */

		/* Save the projectile path */
		if (path_num < MAX_RANGE) {
			path_y[path_num] = y;
			path_x[path_num] = x;
			path_num++;
		}
#endif /* OPTIMIZED_ANIMATIONS */

		/* Player here, try to hit him */
		if (zcave[y][x].m_idx < 0) {
			/* rugby */
			if (o_ptr->tval == TV_GAME && o_ptr->sval == SV_GAME_BALL) {
				c_ptr = &zcave[y][x];

				q_ptr = Players[0 - c_ptr->m_idx];
				if (rand_int(150) > 105 + 256 - adj_dex_th[p_ptr->stat_ind[A_DEX]] - adj_dex_th[q_ptr->stat_ind[A_DEX]]) {
					msg_format_near(0 - c_ptr->m_idx, "\377y%s catches the ball", q_ptr->name);
					msg_print(0 - c_ptr->m_idx, "\377yYou catch the ball");
					inven_carry(0 - c_ptr->m_idx, o_ptr);
				} else {
					msg_format_near(0 - c_ptr->m_idx, "\377r%s misses the ball", q_ptr->name);
					msg_print(0 - c_ptr->m_idx, "\377rYou miss the ball");
					o_ptr->marked2 = ITEM_REMOVAL_NEVER;
					drop_near(TRUE, 0, o_ptr, -1, wpos, y, x);
				}
				/* and stop */
				return;
			}
			/* snowball fight */
			else if (o_ptr->tval == TV_GAME && o_ptr->sval == SV_SNOWBALL) {
				c_ptr = &zcave[y][x];

				q_ptr = Players[0 - c_ptr->m_idx];
				msg_format_near(0 - c_ptr->m_idx, "%s hits %s with a snowball!", p_ptr->name, q_ptr->name);
				msg_format(0 - c_ptr->m_idx, "%s hits you with a snowball!", p_ptr->name);
#ifdef USE_SOUND_2010
				sound_near_site(q_ptr->py, q_ptr->px, &q_ptr->wpos, 0, "snowball", "", SFX_TYPE_COMMAND, TRUE);
#endif
				q_ptr->temp_misc_1 |= 0x08; //snowed
				q_ptr->temp_misc_2 = 110 + rand_int(21); //snowed duration
				note_spot(0 - c_ptr->m_idx, q_ptr->py, q_ptr->px);
				update_player(0 - c_ptr->m_idx); //becomes visible!
				everyone_lite_spot(&q_ptr->wpos, q_ptr->py, q_ptr->px);
				/* snowball doesn't survive it though (and never deals any damage) */
				return;
			}

			if (cfg.use_pk_rules == PK_RULES_NEVER) {
				/* Stop looking */
				break;
			} else {
				c_ptr = &zcave[y][x];

				p_ptr->test_attacks++;
				q_ptr = Players[0 - c_ptr->m_idx];

				/* Check the visibility */
				visible = p_ptr->play_vis[0 - c_ptr->m_idx];

				/* Note the collision */
				hit_body = TRUE;

				/* Did we hit him (penalize range) */
#ifndef PVP_AC_REDUCTION
				if (instakills || test_hit_fire(chance - cur_dis, q_ptr->ac + q_ptr->to_a, visible)) {
#else
				//if (test_hit_fire(chance - cur_dis, ((q_ptr->ac + q_ptr->to_a) * 2) / 3, visible)) {
				if (instakills || test_hit_fire(chance - cur_dis,
				    /* Special perks: Heavy throwing weapons are efficient vs target's AC. Another halving happens if the target is fleeing from us. */
				    (((q_ptr->ac + q_ptr->to_a > AC_CAP) ? AC_CAP : q_ptr->ac + q_ptr->to_a) * 10) / (throwing_weapon ? 10 + o_ptr->weight / 30 : 10) / (q_ptr->afraid ? 2 : 1),
				    visible)) {
#endif
					char p_name[80];

					/* Get his name */
					strcpy(p_name, q_ptr->name);

// TODO: Handle USE_PARRYING (maybe)/USE_BLOCKING/dodging
					/* Handle reflection - thrown items cannot really be deflected if they're bulky.
					   This works well with throwing_weapon, as spears/hatchets start at 5.0, and for dagger weights it fits very nicely too! */
					if (q_ptr->reflect && magik(50 - o_ptr->weight)) {
						if (visible) msg_format(Ind, "The %s was deflected.", o_name);
						break;
					}

					/* Hack -- Base damage from thrown object */
					tdam = damroll(o_ptr->dd, o_ptr->ds);
#if defined(VORPAL_UNBRANDED) || defined(VORPAL_LOWBRANDED)
					if ((f5 & TR5_VORPAL) && !q_ptr->no_cut && !rand_int(VORPAL_CHANCE)) vorpal_cut = tdam; /* save unbranded dice */
					else vorpal_cut = FALSE;
#endif
#ifdef CRIT_UNBRANDED
					k2 = tdam;
					tdam = brand_dam_aux_player(Ind, o_ptr, item, tdam, q_ptr, FALSE);
					k2 = tdam - k2; /* remember difference between branded and unbranded dice */
#else
					tdam = brand_dam_aux_player(Ind, o_ptr, item, tdam, q_ptr, FALSE);
#endif
#ifdef VORPAL_LOWBRANDED
					if (vorpal_cut) vorpal_cut = (vorpal_cut + tdam) / 2;
#else
 #ifndef VORPAL_UNBRANDED
					if ((f5 & TR5_VORPAL) && !q_ptr->no_cut && !rand_int(VORPAL_CHANCE)) vorpal_cut = tdam; /* save branded dice */
					else vorpal_cut = FALSE;
 #endif
#endif
					if (vorpal_cut) msg_format(Ind, "Your weapon cuts deep into %s!", q_name);

					/* Select a chaotic effect (10% chance) */
					if ((f5 & TR5_CHAOTIC) && !rand_int(10)) {
						if (!rand_int(2)) {
							/* Vampiric (50%) (50%) */
							chaos_effect = 1;
#if 0 /* not for throwing stuff */
						} else if (!rand_int(1000)) {
							/* Quake (0.050%) (49.975%) */
							chaos_effect = 2;
#endif
						} else if (!rand_int(2)) {
							/* Confusion (25%) (24.9875%) */
							chaos_effect = 3;
						} else if (!rand_int(30)) {
							/* Teleport away (0.83%) (24.1545833%) */
							chaos_effect = 4;
						} else if (!rand_int(50)) {
							/* Polymorph (0.48%) (23.6714917%) */
							chaos_effect = 5;
						} else if (!rand_int(300)) {
							/* Clone (0.079%) */
							chaos_effect = 6;
						}
					}

					tdam += o_ptr->to_d;

					/* Specialty: Only daggers (includes main gauche), axes and spears/tridents can be thrown effectively) */
					if (throwing_weapon) {
						int to_hit, to_dam, org_to_h, org_to_h_thrown;

						tdam += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
#if 0
						/* About adding weight-damage, we have to be a bit careful, as heavier weapons already receive greater melee damage dice anyway: */
						tdam += (50 / (380 / (o_ptr->weight + 5) + 4)) - 1;
#else
						/* Weight has a larger effect on the damage output,
						   as you can basically hold 15 daggers at the same weight cost as 1 heavy weapon,
						   so it needs some more advantage */
						tdam += o_ptr->weight / 10;
						/* And even add the dice AGAIN! */
						tdam += damroll(o_ptr->dd, o_ptr->ds);
#endif

						/* Hack: For thowing weapons actually apply the player damage boni.
						   The reason is that otherwise throwing weapons will quickly be outdamaged by melee weapons
						   and not make a dent anymore in comparison, except at very low character levels. */
						org_to_h = p_ptr->to_h;
						org_to_h_thrown = p_ptr->to_h_thrown;
						calc_boni_weapon(Ind, o_ptr, &to_hit, &to_dam);
						/* Hack throwing boni */
						p_ptr->to_h = 0;
						p_ptr->to_h_thrown = to_hit;
						/* Apply hacked to-hit and to-dam values for the throwing weapon */
						tdam += p_ptr->to_d + p_ptr->to_d_melee;
#ifdef CRIT_UNBRANDED
						k3 = critical_throw(Ind, o_ptr->weight, o_ptr->to_h, tdam - k2, calc_crit_obj(o_ptr));
						k3 += k2;
#else
						k3 = critical_throw(Ind, o_ptr->weight, o_ptr->to_h, tdam, calc_crit_obj(o_ptr));
#endif
						/* Unhack throwing boni */
						p_ptr->to_h = org_to_h;
						p_ptr->to_h_thrown = org_to_h_thrown;

						k2 = tdam; /* remember damage before crit */

						tdam = k3;

						/* penalty for weapons in bat form */
						if (p_ptr->body_monster == RI_VAMPIRE_BAT) tdam /= 2;

						/* No negative damage */
						if (tdam < 0) tdam = 0;

						/* Vorpal bonus - multi-dice!
						   (currently +31.25% more branded dice damage on total average, just for the records) */
						if (vorpal_cut) {
#ifdef CRIT_VS_VORPAL
							k2 += (magik(25) ? 2 : 1) * (vorpal_cut + 5); /* exempts critical strike */
							/* either critical hit or vorpal, not both */
							if (k2 > tdam) tdam = k2;
#else
							tdam += (magik(25) ? 2 : 1) * (vorpal_cut + 5); /* exempts critical strike */
#endif
						}

						/* factor in AC */
						tdam -= (tdam * (((q_ptr->ac + q_ptr->to_a) < AC_CAP) ? (q_ptr->ac + q_ptr->to_a) : AC_CAP) / AC_CAP_DIV);

						/* Remember original damage for vampirism (less rounding trouble..) */
						k2 = tdam;

						/* Note: No rune nimbus application */

						//TODO: actually apply chaos_effect and k2->vampirism

					} else if (is_melee_item(o_ptr->tval)) {
						tdam = (tdam * 2) / 3; /* assumption: Melee weapon dice/damage are meant for 'proper use', while other items get dice defined in k_info exactly for the purpose of throwing! */
						tdam += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128) / 2;
						/* For throwing a non-throwingweapon, we ignore CRIT flag of item instead of applying calc_crit_obj(o_ptr): */
						tdam = critical_throw(Ind, o_ptr->weight, o_ptr->to_h, tdam, 0);
					} else {
						/* For throwing a non-throwingweapon, we ignore CRIT flag of item instead of applying calc_crit_obj(o_ptr): */
						tdam = critical_throw(Ind, o_ptr->weight, o_ptr->to_h, tdam, 0);
					}

					/* Note: Combat stance isn't applied except for this: */
#ifdef DEFENSIVE_STANCE_FIXED_RANGED_REDUCTION
					if (p_ptr->combat_stance == 1) tdam /= 2;
#endif

					/* No negative damage */
					if (tdam < 0) tdam = 0;

					/* XXX Reduce damage by 1/3 */
					tdam = (tdam + PVP_THROW_DAM_REDUCTION - 1) / PVP_THROW_DAM_REDUCTION;

					/* Handle unseen player */
					if (!visible) {
						/* Messages */
						msg_format(Ind, "The %s finds a mark!", o_name);
						msg_format(0 - c_ptr->m_idx, "You are hit by a %s!", o_name);
					}

					/* Handle visible player */
					else {
						/* Messages */
						msg_format(Ind, "The %s hits %s for \377y%d \377wdamage.", o_name, p_name, tdam);
						if ((o_name[0] == 'A') || (o_name[0] == 'E') || (o_name[0] == 'I') || (o_name[0] == 'O') || (o_name[0] == 'U') ||
						    (o_name[0] == 'a') || (o_name[0] == 'e') || (o_name[0] == 'i') || (o_name[0] == 'o') || (o_name[0] == 'u'))
							msg_format(0 - c_ptr->m_idx, "%^s hits you with an %s for \377R%d \377wdamage.", p_ptr->name, o_name, tdam);
						else	msg_format(0 - c_ptr->m_idx, "%^s hits you with a %s for \377R%d \377wdamage.", p_ptr->name, o_name, tdam);

						/* Track player's health */
						health_track(Ind, c_ptr->m_idx);
					}

					/* Take damage */
					take_hit(0 - c_ptr->m_idx, tdam, p_ptr->name, Ind);

					/* Stop looking */
					break;
				}
			}
		}

		/* Monster here, Try to hit it */
		if (zcave[y][x].m_idx > 0) {
			c_ptr = &zcave[y][x];

			monster_type *m_ptr = &m_list[c_ptr->m_idx];
			monster_race *r_ptr = race_inf(m_ptr);

			p_ptr->test_attacks++;
			/* Check the visibility */
			visible = p_ptr->mon_vis[c_ptr->m_idx];

			/* Note the collision */
			hit_body = TRUE;

			/* Did we hit it (penalize range) */
			if (instakills || test_hit_fire(chance - cur_dis,
			    /* Special perks: Heavy throwing weapons are efficient vs target's AC. Another halving happens if the target is fleeing from us. */
			    (m_ptr->ac * 10) / (throwing_weapon ? 10 + o_ptr->weight / 30 : 10) / (m_ptr->monfear ? 2 : 1),
			    visible)) {
				bool fear = FALSE;
				char m_name[MNAME_LEN];

				/* Get "the monster" or "it" */
				monster_desc(Ind, m_name, c_ptr->m_idx, 0);


				/* Assume a default death */
				cptr note_dies = " dies";

				/* Some monsters get "destroyed" */
				if ((r_ptr->flags3 & RF3_DEMON) ||
				    (r_ptr->flags3 & RF3_UNDEAD) ||
				    (r_ptr->flags2 & RF2_STUPID) ||
				    (strchr("Evg", r_ptr->d_char)))
				{
					/* Special note at death */
					note_dies = " is destroyed";
				}

				/* Handle reflection - thrown items cannot really be deflected if they're bulky.
				   This works well with throwing_weapon, as spears/hatchets start at 5.0, and for dagger weights it fits very nicely too! */
				if ((r_ptr->flags2 & RF2_REFLECTING) && magik(50 - o_ptr->weight)) {
					if (visible) msg_format(Ind, "The %s was deflected.", o_name);
					break;
				}

//TODO: USE_PARRYING (maybe); monster dodging
#ifdef USE_BLOCKING
				/* handle blocking (deflection) */
				if (strchr("hHJkpPtyn", r_ptr->d_char) && /* leaving out Yeeks (else Serpent Man 'J') */
				    !(r_ptr->flags3 & RF3_ANIMAL) && !(r_ptr->flags8 & RF8_NO_BLOCK) && !m_ptr->csleep && m_ptr->stunned <= 100 && !m_ptr->confused
				    && !rand_int(5 - r_ptr->level / 50)) { /* decent chance to block throws */
					if (visible) {
						char hit_desc[MAX_CHARS + 12];

						sprintf(hit_desc, "\377%c%s blocks.", COLOUR_BLOCK_MON, m_name);
						hit_desc[2] = toupper(hit_desc[2]);
						msg_print(Ind, hit_desc);
					}
					break;
				}
#endif


				/* Handle unseen monster */
				if (!visible) {
					/* Invisible monster */
					msg_format(Ind, "The %s finds a mark.", o_name);
				}

				/* Handle visible monster */
				else {
					/* Message */
					msg_format(Ind, "The %s hits %s.", o_name, m_name);

					/* Hack -- Track this monster race */
					if (visible) recent_track(m_ptr->r_idx);

					/* Hack -- Track this monster */
					if (visible) health_track(Ind, c_ptr->m_idx);
				}

				/* Hack -- Base damage from thrown object */
				tdam = damroll(o_ptr->dd, o_ptr->ds);
				tdam = brand_dam_aux(Ind, o_ptr, item, tdam, m_ptr, TRUE);

#if defined(VORPAL_UNBRANDED) || defined(VORPAL_LOWBRANDED)
				if ((f5 & TR5_VORPAL) && !(r_ptr->flags8 & RF8_NO_CUT) && !rand_int(VORPAL_CHANCE)) vorpal_cut = tdam; /* save unbranded dice */
				else vorpal_cut = FALSE;
#endif
#ifdef CRIT_UNBRANDED
				k2 = tdam;
				tdam = brand_dam_aux(Ind, o_ptr, item, tdam, m_ptr, FALSE);
				k2 = tdam - k2; /* remember difference between branded and unbranded dice */
#else
				tdam = brand_dam_aux(Ind, o_ptr, item, tdam, m_ptr, FALSE);
#endif
#ifdef VORPAL_LOWBRANDED
				if (vorpal_cut) vorpal_cut = (vorpal_cut + tdam) / 2;
#else
 #ifndef VORPAL_UNBRANDED
				if ((f5 & TR5_VORPAL) && !(r_ptr->flags8 & RF8_NO_CUT) && !rand_int(VORPAL_CHANCE)) vorpal_cut = tdam; /* save branded dice */
				else vorpal_cut = FALSE;
 #endif
#endif
				if (vorpal_cut) msg_format(Ind, "Your weapon cuts deep into %s!", m_name);

				/* Select a chaotic effect (10% chance) */
				if ((f5 & TR5_CHAOTIC) && !rand_int(10)) {
					if (!rand_int(2)) {
						/* Vampiric (50%) (50%) */
						chaos_effect = 1;
#if 0 /* not for throwing stuff */
					} else if (!rand_int(1000)) {
						/* Quake (0.050%) (49.975%) */
						chaos_effect = 2;
#endif
					} else if (!rand_int(2)) {
						/* Confusion (25%) (24.9875%) */
						chaos_effect = 3;
					} else if (!rand_int(30)) {
						/* Teleport away (0.83%) (24.1545833%) */
						chaos_effect = 4;
					} else if (!rand_int(50)) {
						/* Polymorph (0.48%) (23.6714917%) */
						chaos_effect = 5;
					} else if (!rand_int(300)) {
						/* Clone (0.079%) */
						chaos_effect = 6;
					}
				}

				tdam += o_ptr->to_d;

				/* Specialty: Only daggers (includes main gauche), axes and spears/tridents can be thrown effectively) */
				if (throwing_weapon) {
					int to_hit = 0, to_dam = 0, org_to_h, org_to_h_thrown;

					tdam += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
#if 0
					/* About adding weight-damage, we have to be a bit careful, as heavier weapons already receive greater melee damage dice anyway: */
					tdam += (50 / (380 / (o_ptr->weight + 5) + 4)) - 1;//^^ ->
					//(10~) dagger +0, 30 (main gauche) +2, 60 (spears/tomahawk) +4 (!), 90 (trident/broad spear) +5, 120 (trifurcate spear, light war axe) +6, 170 (battle axe) +7, 240 (heavy war axe) +8, 380 (thunder axe) +9
					//(note: no axe near 90 lbs for the +5 dmg; tomahawk is 80 lbs.)
#else
					/* Weight has a larger effect on the damage output,
					   as you can basically hold 15 daggers at the same weight cost as 1 heavy weapon,
					   so it needs some more advantage */
					tdam += o_ptr->weight / 10;
					/* And even add the dice AGAIN! */
					tdam += damroll(o_ptr->dd, o_ptr->ds);
#endif

					/* Hack: For throwing weapons actually apply the player damage boni.
					   The reason is that otherwise throwing weapons will quickly be outdamaged by melee weapons
					   and not make a dent anymore in comparison, except at very low character levels. */
					org_to_h = p_ptr->to_h;
					org_to_h_thrown = p_ptr->to_h_thrown;
					calc_boni_weapon(Ind, o_ptr, &to_hit, &to_dam);
					/* Hack throwing boni */
					p_ptr->to_h = 0;
					p_ptr->to_h_thrown = to_hit;
					/* Apply hacked to-hit and to-dam values for the throwing weapon */
					tdam += to_dam;
#ifdef CRIT_UNBRANDED
					k3 = critical_throw(Ind, o_ptr->weight, o_ptr->to_h, tdam - k2, calc_crit_obj(o_ptr));
					k3 += k2;
#else
					k3 = critical_throw(Ind, o_ptr->weight, o_ptr->to_h, tdam, calc_crit_obj(o_ptr));
#endif
					/* Unhack throwing boni */
					p_ptr->to_h = org_to_h;
					p_ptr->to_h_thrown = org_to_h_thrown;

#ifdef CRIT_VS_VORPAL
					k2 = tdam; /* remember damage before crit */
#endif
					tdam = k3;

					/* penalty for weapons in bat form */
					if (p_ptr->body_monster == RI_VAMPIRE_BAT) tdam /= 2;

					/* No negative damage */
					if (tdam < 0) tdam = 0;

					/* Vorpal bonus - multi-dice!
					   (currently +31.25% more branded dice damage on total average, just for the records) */
					if (vorpal_cut) {
#ifdef CRIT_VS_VORPAL
						k2 += (magik(25) ? 2 : 1) * (vorpal_cut + 5); /* exempts critical strike */
						/* either critical hit or vorpal, not both */
						if (k2 > tdam) tdam = k2;
#else
						tdam += (magik(25) ? 2 : 1) * (vorpal_cut + 5); /* exempts critical strike */
#endif
					}

					/* factor in AC */
					//tdam -= (tdam * (((q_ptr->ac + q_ptr->to_a) < AC_CAP) ? (q_ptr->ac + q_ptr->to_a) : AC_CAP) / AC_CAP_DIV);

					/* Remember original damage for vampirism (less rounding trouble..) */
					k2 = tdam;

					//TODO: apply chaos_effect and k2-vampirism later

					/* Note: No rune nimbus application */

				} else if (is_melee_item(o_ptr->tval)) {
					tdam = (tdam * 2) / 3; /* assumption: Melee weapon dice/damage are meant for 'proper use', while other items get dice defined in k_info exactly for the purpose of throwing! */
					tdam += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128) / 2;
					/* For throwing a non-throwingweapon, we ignore CRIT flag of item instead of applying calc_crit_obj(o_ptr): */
					tdam = critical_throw(Ind, o_ptr->weight, o_ptr->to_h, tdam, 0);
				} else {
					/* For throwing a non-throwingweapon, we ignore CRIT flag of item instead of applying calc_crit_obj(o_ptr): */
					tdam = critical_throw(Ind, o_ptr->weight, o_ptr->to_h, tdam, 0);
				}

				/* Note: Combat stance isn't applied except for this: */
#ifdef DEFENSIVE_STANCE_FIXED_RANGED_REDUCTION
				if (p_ptr->combat_stance == 1) tdam /= 2;
#endif

				/* No negative damage */
				if (tdam < 0) tdam = 0;

				/* Snowballs deal no damage */
				if (o_ptr->tval == TV_GAME && o_ptr->sval == SV_SNOWBALL) {
					tdam = 0;

					/* Target dummy "snowiness" hack */
					if ((m_ptr->r_idx == RI_TARGET_DUMMY1 || m_ptr->r_idx == RI_TARGET_DUMMY2) &&
					    m_ptr->extra < 60) {
						m_ptr->extra += 6 + 7; //getting hit will subtract 7 right away again
						if (m_ptr->r_idx == RI_TARGET_DUMMY1 && m_ptr->extra >= 30) {
							m_ptr->r_idx = RI_TARGET_DUMMY2;
							everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
						}
					}
					if ((m_ptr->r_idx == RI_TARGET_DUMMYA1 || m_ptr->r_idx == RI_TARGET_DUMMYA2) &&
					    m_ptr->extra < 60) {
						m_ptr->extra += 6 + 7; //getting hit will subtract 7 right away again
						if (m_ptr->r_idx == RI_TARGET_DUMMYA1 && m_ptr->extra >= 30) {
							m_ptr->r_idx = RI_TARGET_DUMMYA2;
							everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
						}
					}
				}

				if (m_ptr->r_idx == RI_MIRROR) tdam = (tdam * MIRROR_REDUCE_DAM_TAKEN_THROW + 99) / 100;

				if (instakills) tdam = m_ptr->hp + 1;
				else if (p_ptr->admin_godly_strike) {
					p_ptr->admin_godly_strike--;
					if (!(r_ptr->flags1 & RF1_UNIQUE)) tdam = m_ptr->hp + 1;
				}

#if 0 //todo
				//TODO: apply chaos_effect and k2-vampirism here

				/* May it clone the monster ? */
				if (((f4 & TR4_CLONE) && randint(1000) == 1)
				    || chaos_effect == 6) {
					msg_format(Ind, "Your weapon clones %s!", m_name);
					multiply_monster(c_ptr->m_idx);
				}

				/* Does the weapon take damage from hitting acidic/fiery/aquatic monsters? */
				for (i = 0; i < 4; i++) {
					if (r_ptr->blow[i].effect == RBE_ACID) mon_acid++;
					if (r_ptr->blow[i].effect == RBE_FIRE) mon_fire++;
				}
				if (r_ptr->flags4 & RF4_BR_ACID) mon_acid += 2;
				if (r_ptr->flags4 & RF4_BR_FIRE) mon_fire += 2;
				if (strstr(mbname, "water")) mon_aqua = 4;
				if (strstr(mbname, "acid")) mon_acid = 4;
				if (strstr(mbname, "fire") || strstr(mbname, "fiery")) mon_fire = 4;
				if (p_ptr->resist_water) mon_aqua /= 2;
				if (p_ptr->immune_water) mon_aqua = 0;
				if (p_ptr->resist_acid || p_ptr->oppose_acid) mon_acid /= 2;
				if (p_ptr->immune_acid) mon_acid = 0;
				if (p_ptr->resist_fire || p_ptr->oppose_fire) mon_fire /= 2;
				if (p_ptr->immune_fire) mon_fire = 0;
				i = mon_aqua + mon_acid + mon_fire;
				//if (i && magik(20 + (i > 5 ? 5 : i) * 6)) {
				if (i && magik(i > 5 ? 5 : i)) {
					i = rand_int(i);

					if (i < mon_aqua) weapon_takes_damage(Ind, GF_WATER, slot);
					else if (i < mon_aqua + mon_acid) weapon_takes_damage(Ind, GF_ACID, slot);
					else weapon_takes_damage(Ind, GF_FIRE, slot);
				}

				if (!instakills) do_nazgul(Ind, &tdam, r_ptr, slot);
#endif

				/* Hit the monster, check for death */
				old_instakills = p_ptr->instakills;
				p_ptr->instakills = instakills;
				if (mon_take_hit(Ind, c_ptr->m_idx, tdam, &fear, note_dies)) {
					/* Dead monster */
				}
				/* No death */
				else {
					/* Message */
					message_pain(Ind, c_ptr->m_idx, tdam);

					/* Take note */
					if (fear && visible) {
						char m_name[MNAME_LEN];

#ifdef USE_SOUND_2010
#else
						sound(Ind, SOUND_FLEE);
#endif

						/* Get the monster name (or "it") */
						monster_desc(Ind, m_name, c_ptr->m_idx, 0);

						/* Message */
						if (m_ptr->r_idx != RI_MORGOTH)
							msg_format(Ind, "%^s flees in terror!", m_name);
						else
							msg_format(Ind, "%^s retreats!", m_name);
					}
				}
				/* Unhack */
				p_ptr->instakills = old_instakills;

				/* Exploding Attack - Kurzel */
				if (p_ptr->nimbus) do_nimbus(Ind, y, x);

				/* Stop looking */
				break;
			}
		}
	}

	wall_x = nx;
	wall_y = ny;

#ifdef OPTIMIZED_ANIMATIONS
	if (path_num) {
		/* Pick a random spot along the path */
		j = rand_int(path_num);
		ny = path_y[j];
		nx = path_x[j];

		/* Save the old "player pointer" */
		q_ptr = p_ptr;

		/* Draw a projectile here for everyone */
		for (i = 1; i <= NumPlayers; i++) {
			/* Use this player */
			p_ptr = Players[i];

			/* If he's not playing, skip him */
			if (p_ptr->conn == NOT_CONNECTED)
				continue;

			/* If he's not here, skip him */
			if (!inarea(&p_ptr->wpos, wpos))
				continue;

			/* The player can see the (on screen) missile */
			if (panel_contains(ny, nx) && player_can_see_bold(i, ny, nx)) {
				int dispx, dispy;

				/* Draw */
				dispy = ny - p_ptr->panel_row_prt;
				dispx = nx - p_ptr->panel_col_prt;

				/* Remember the projectile */
				p_ptr->scr_info[dispy][dispx].c = missile_char;
				p_ptr->scr_info[dispy][dispx].a = missile_attr;

				/* Tell the client */
#ifdef GRAPHICS_BG_MASK
				Send_char(i, dispx, dispy, missile_attr, missile_char, 0, 0);
#else
				Send_char(i, dispx, dispy, missile_attr, missile_char);
#endif

				/* Flush once */
				Send_flush(i);
			}

			/* Restore later */
			everyone_lite_later_spot(wpos, ny, nx);
		}

		/* Restore the player pointer */
		p_ptr = q_ptr;
	}
#endif /* OPTIMIZED_ANIMATIONS */

	/* Aside from potion-smash-effect which is handled further below, some extra items might set oil ablaze, such as torches or any fire-based light */
	if (o_ptr->tval == TV_LITE && o_ptr->tval != SV_LITE_FEANORIAN
	    && zcave[y][x].slippery >= 1000)
		light_oil(zcave, wpos, x, y, 0);

	/* Snowballs never survive a throw */
	if (o_ptr->tval == TV_GAME && o_ptr->sval == SV_SNOWBALL) return;

	/* Artifacts never break but always auto-return, same as for ammo fired */
	if (returning) return;

	/* Non-returning only: If we threw an equipment item away, recalculate our boni */
	if (item >= INVEN_WIELD) equip_thrown(Ind, item, o_ptr, original_number);

	/* Chance of breakage (during attacks) */
	j = (hit_body ? breakage_chance(o_ptr) : 0);

	/* Note about TR5_IMPACT: No need to check, as we just assume it'd have to be thrown effectively like a throwing_weapon to actually trigger quake,
	   but at this time 'of Earthquakes' ego is only available on heavy blunt weapons, which do not count as is_throwing_weapon() anyway. */

	/* Potions smash open */
	/* BTW, why not o_ptr->tval ? :) */
	if (k_info[o_ptr->k_idx].tval == TV_POTION ||
	    k_info[o_ptr->k_idx].tval == TV_POTION2 ||
	    k_info[o_ptr->k_idx].tval == TV_FLASK ||
	    k_info[o_ptr->k_idx].tval == TV_BOTTLE ||
	    (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT && (o_ptr->xtra3 & 0x0002))) {
		if ((hit_body) || (hit_wall) || (randint(100) < j)) {
			/* hack: shatter _on_ the wall grid, not _before_ it */
			if (hit_wall) {
				x = wall_x;
				y = wall_y;
			}

			/* Message */
			/* TODO: handle blindness */
			msg_format_near_site(y, x, wpos, 0, TRUE, "The %s shatters!", o_name);
#ifdef USE_SOUND_2010
			sound_near_site(y, x, wpos, 0, "shatter_potion", NULL, SFX_TYPE_MISC, FALSE);
#endif

			//if (potion_smash_effect(0, wpos, y, x, o_ptr->sval))
			if (k_info[o_ptr->k_idx].tval == TV_POTION) {
				if (potion_smash_effect(0 - Ind, wpos, y, x, o_ptr->sval))
				//if (potion_smash_effect(PROJECTOR_POTION, wpos, y, x, o_ptr->sval))
				{
#if 0
					if (cave[y][x].m_idx) {
						char m_name[MNAME_LEN];

						monster_desc(m_name, &m_list[cave[y][x].m_idx], 0);
						switch (is_friend(&m_list[cave[y][x].m_idx])) {
						case 1:
							msg_format("%^s gets angry!", m_name);
							change_side(&m_list[cave[y][x].m_idx]);
							break;
						case 0:
							msg_format("%^s gets angry!", m_name);
							m_list[cave[y][x].m_idx].status = MSTATUS_NEUTRAL_M;
							break;
						}
					}
#endif	// 0
				}
			}
			else if (k_info[o_ptr->k_idx].tval == TV_FLASK) (void)potion_smash_effect(0 - Ind, wpos, y, x, o_ptr->sval + 200);

			return;
		} else j = 0;
	}

#ifdef ENABLE_DEMOLITIONIST
	/* Hack: If we're throwing a demolition charge, auto-arm it if allowed! */
	if (!bashing && o_ptr->tval == TV_CHARGE) {
		if (arm_charge_conditions(Ind, o_ptr, TRUE)) {
			/* If in cold environment, sometimes extinguish */
			o_ptr->xtra8 = cold_place(wpos);

			/* We use throwing direction as trap-effect direction, makes sense sort of.
			   However, if our dir is targetted, aka '5', we have to approxmiate.. */
			if (dir == 5) {
				int cdir;

				if (p_ptr->px < p_ptr->target_col) {
					if (p_ptr->py < p_ptr->target_row) cdir = 3;
					else if (p_ptr->py > p_ptr->target_row) cdir = 9;
					else cdir = 6;
				} else if (p_ptr->px > p_ptr->target_col) {
					if (p_ptr->py < p_ptr->target_row) cdir = 1;
					else if (p_ptr->py > p_ptr->target_row) cdir = 7;
					else cdir = 4;
				} else {
					if (p_ptr->py < p_ptr->target_row) cdir = 2;
					else if (p_ptr->py > p_ptr->target_row) cdir = 8;
					else {
						//doesn't make much sense to throw at self, but just use random direction in that case
						cdir = randint(8);
						if (cdir >= 5) cdir++;
					}
				}
				arm_charge_dir_and_fuse(o_ptr, cdir);
			} else arm_charge_dir_and_fuse(o_ptr, dir);
 #if 0 /* xD no? */
			/* throwing can affect the fuse burning sometimes */
			if (o_ptr->timeout) {
				o_ptr->timeout = o_ptr->timeout - rand_int(4);
				if (!o_ptr->timeout) o_ptr->timeout = 1;
			}
 #endif
			/* If thrown into fire, insta-trigger */
			if (feat_is_lava(zcave[y][x].feat) || feat_is_acute_fire(zcave[y][x].feat)) o_ptr->timeout = -1;
		}
	}
#endif

	/* Drop (or break) near that location */

	/* Special exception: If item failed to get dropped, ie vanished, we will restore it at its
	   original position IF it was bashed from inside a house to a target location inside a house.
	   This is done to protect items inside houses from weird bashes that would erase them.
	   (Funny hypothetical side effect: We could bash an item inside a house against another player
	   who is inside the house too, he'd notice it, yet it lands in front of our own feet again.) */
	if (drop_near(FALSE, Ind, o_ptr, -j - 1, wpos, y, x) == -2) { /* bashing went wrong aka no space in house at target location? */
		/* Second chance? */
		if (!wpos->wz /* world surface is condition for 'house' */
		    && start_ix /* only if it was bashed, not if it was thrown */
		    && inside_house_simple(&zcave[start_iy][start_ix]) /* starting location must be inside house */
		    && inside_house_simple(&zcave[y][x])) /* target location must be inside house */
			drop_near(TRUE, Ind, o_ptr, j, wpos, start_iy, start_ix);
		else {
			/* Manually clean up */
			if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1);
			questitem_d(o_ptr, o_ptr->number);
		}
	}
}

static void destroy_house(int Ind, struct dna_type *dna) {
	player_type *p_ptr = Players[Ind];
	int i;
	house_type *h_ptr;
	char owner[MAX_CHARS], owner_type[20];

	if (!is_admin(p_ptr)) {
		msg_print(Ind, "\377rYour attempts to destroy the house fail.");
		return;
	}
	for (i = 0; i < num_houses; i++) {
		h_ptr = &houses[i];
		if (h_ptr->dna == dna) {
			if (h_ptr->flags & HF_STOCK) {
				msg_print(Ind, "\377oThat house may not be destroyed. (HF_STOCK)");
				//return;
			}
			switch (h_ptr->dna->owner_type) {
			case OT_PLAYER:
				strcpy(owner, lookup_player_name(h_ptr->dna->owner));
				strcpy(owner_type, "P");
				break;
			case OT_GUILD:
				strcpy(owner, guilds[h_ptr->dna->owner].name);
				strcpy(owner_type, "G");
				break;
			default:
				strcpy(owner, "UNDEFINED");
			}
			msg_format(Ind, "\377DThe house %d (dna: c=%d o=%s ot=%s; dx,dy=%d,%d) crumbles away.", i, h_ptr->dna->creator, owner, owner_type, h_ptr->dx, h_ptr->dy);
			/* quicker than copying back an array. */
			fill_house(h_ptr, FILL_MAKEHOUSE, NULL);
			h_ptr->flags |= HF_DELETED;

#if 0 /* let's do this in fill_house(), so it also takes care of poly-houses - C. Blue */
			/* redraw map so change becomes visible */
			if (h_ptr->flags & HF_RECT) {
				for (x = 0; x < h_ptr->coords.rect.width; x++)
				for (y = 0; y < h_ptr->coords.rect.height; y++)
					everyone_lite_spot(&h_ptr->wpos, h_ptr->y + y, h_ptr->x + x);
			}
#endif
			break;
		}
	}
}

void house_admin(int Ind, int dir, char *args) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	int x, y;
	int success = 0;
	cave_type *c_ptr;
	struct dna_type *dna;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return;	//(FALSE);

	if (dir && args) {
		struct c_special *cs_ptr;

		/* Get requested direction */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];
		/* Get requested grid */
		c_ptr = &zcave[y][x];
		switch (c_ptr->feat) {
		case FEAT_HOME:
		case FEAT_HOME_OPEN:
			if ((cs_ptr = GetCS(c_ptr, CS_DNADOOR))) {
				/* Test for things that don't require access: */
				switch (args[0]) {
				case 'S': /* enter a player store */
					disturb(Ind, 1, 0);
					if (!do_cmd_player_store(Ind, x, y))
						msg_print(Ind, "There is no player store there.");
					return;
				case 'H':
					knock_house(Ind, x, y);
					return;
				}

				/* Test for things that do require access: */
				dna = cs_ptr->sc.ptr;
				if (access_door(Ind, dna, FALSE) || admin_p(Ind)) {
					switch (args[0]) {
					case 'O':
						success = chown_door(Ind, dna, args, x, y);
						break;
					case 'M':
						success = chmod_door(Ind, dna, args);
						break;
					case 'K':
						destroy_house(Ind, dna);
						return;
					case 'P':
						paint_house(Ind, x, y, atoi(&args[1]));
						return;
					case 'T':
						args[20] = 0;//ensure max of 20 - 1 characters, starting at #1
						tag_house(Ind, x, y, &args[1]);
						return;
					}

					if (success) {
						msg_print(Ind, "\377gChange successful.");
						/* take note of door colour change */
						everyone_lite_spot(wpos, y, x);
					} else msg_print(Ind, "\377oChange failed.");
				} else msg_print(Ind, "\377oAccess not permitted.");
			} else msg_print(Ind, "\377yNot a door of a private house.");
			break;
		case FEAT_WINDOW:
		case FEAT_WINDOW_SMALL:
		case FEAT_OPEN_WINDOW:
		case FEAT_OPEN_WINDOW_SMALL:
		case FEAT_BARRED_WINDOW:
		case FEAT_BARRED_WINDOW_SMALL:
			/* Windows only allow knocking */
			if (args[0] == 'H') {
				knock_window(Ind, x, y);
				return;
			}
			//fall through
		default:
			msg_print(Ind, "\377sThere is no house door there.");
		}
	}
	return;
}

/*
 * Buy a house.  It is assumed that the player already knows the
 * price.

 Hacked to sell houses for half price. -APD-
 Doesn't use a turn / disable AFK at this time. - C. Blue
 */
void do_cmd_purchase_house(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;

	int y, x, h_idx;
	//int64_t price; /* I'm hoping this will be 64 bits.  I dont know if it will be portable. */
	//s64b price;
	s32b price;

	cave_type *c_ptr = NULL;
	struct dna_type *dna;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return;

	/* Ghosts cannot buy houses */
	if ((p_ptr->ghost) && !is_admin(p_ptr)) {
		/* Message */
		msg_print(Ind, "You cannot buy a house.");

		return;
	}

	if (p_ptr->inval) {
		msg_print(Ind, "\377yYou may not buy/sell a house, wait for an admin to validate your account.");
		return;
	}


	if (dir > 9) dir = 0;	/* temp hack */

	/* Be sure we have a direction */
	if (dir > 0) {
		struct c_special *cs_ptr;
		int acc_houses = acc_get_houses(p_ptr->accountname);

		/* Get requested direction */
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];

		/* Get requested grid */
		if (in_bounds(y, x)) c_ptr = &zcave[y][x];

		/* Check for a house */
		if (!(c_ptr && (c_ptr->feat == FEAT_HOME || c_ptr->feat == FEAT_HOME_OPEN)
		    && (cs_ptr = GetCS(c_ptr, CS_DNADOOR)))) {
			/* No house, message */
			msg_print(Ind, "You see nothing to buy/sell there.");
			return;
		}

		dna = cs_ptr->sc.ptr;

		h_idx = pick_house(&p_ptr->wpos, y, x);
		/* paranoia */
		if (h_idx == -1) {
			msg_print(Ind, "That's not a house to buy/sell!");
			return;
		}

		/* Check for already-owned house */
		if (dna->owner) {
			if (access_door(Ind, dna, FALSE) || admin_p(Ind)) {
				if (p_ptr->dna == dna->creator) {
					price = house_price_player(dna->price, p_ptr->stat_ind[A_CHR]);
					if (!gain_au(Ind, price / 2, FALSE, FALSE)) return;
#ifdef USE_SOUND_2010
					sound(Ind, "pickup_gold", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

					/* sell house */
					msg_format(Ind, "You sell your house for %d gold.", price / 2);
					s_printf("SELL_HOUSE: %s sells %d (%d,%d) [%d,%d] for %d\n", p_ptr->name, h_idx, wpos->wx, wpos->wy, houses[h_idx].dx, houses[h_idx].dy, price / 2);
					if (dna->owner_type != OT_GUILD) {
						p_ptr->houses_owned--;
						if (houses[h_idx].flags & HF_MOAT) p_ptr->castles_owned--;

						//ACC_HOUSE_LIMIT
						acc_houses--;
						acc_set_houses(p_ptr->accountname, acc_houses);
						clockin(Ind, 8);
					}

					/* make sure we don't get stuck by selling it while inside - C. Blue */
					if (zcave[p_ptr->py][p_ptr->px].info & CAVE_ICKY)
						teleport_player_force(Ind, 1);
				} else {
					if (!is_admin(p_ptr)) {
						msg_print(Ind, "Only the rightful owner can sell a house");
						return;
					} else msg_print(Ind, "The house is reset");
				}
				if (dna->owner_type == OT_GUILD) {
					guilds[dna->owner].h_idx = 0;
					Send_guild_config(dna->owner);

					/* and in case it was suspended due to leaderlessness,
					   so the next guild buying this house won't get a surprise.. */
					houses[h_idx].flags &= ~HF_GUILD_SUS;
					fill_house(&houses[h_idx], FILL_GUILD_SUS_UNDO, NULL);
				}
				dna->creator = 0L;
				dna->owner = 0L;
				dna->a_flags = ACF_NONE;

				/* Erase house contents */
				kill_house_contents(h_idx);

				/* take note of door colour change */
				c_ptr->feat = FEAT_HOME; /* make sure door is closed, in case it was open when we sold it */
				everyone_lite_spot(wpos, y, x);
				return;
			}
			msg_print(Ind, "That house does not belong to you!");
			return;
		}

		/* we are buying */
		price = house_price_player(dna->price, p_ptr->stat_ind[A_CHR]);
		if (price > p_ptr->au) {
			msg_print(Ind, "You do not have enough gold!");
			return;
		}

		if (cfg.acc_house_limit) { //ACC_HOUSE_LIMIT
			if (acc_houses >= cfg.acc_house_limit) {
				//msg_format(Ind, "The number of houses is limited to %d across all characters of an account.", cfg.acc_house_limit);
				msg_format(Ind, "You already own the sum of %d houses across all of your characters,", cfg.acc_house_limit);
				msg_print(Ind, " which is the total house limit per account.");
				return;
			}
		}

		if ((p_ptr->mode & MODE_PVP) && p_ptr->houses_owned >= 1) {
			msg_print(Ind, "PvP characters may not own more than one house!");
			return;
		}
		if (p_ptr->mode & MODE_DED_IDDC) {
			msg_print(Ind, "IDDC-exclusive characters may not own houses!");
			return;
		}

		if (cfg.houses_per_player && (p_ptr->houses_owned >= ((p_ptr->lev > 50 ? 50 : p_ptr->lev) / cfg.houses_per_player)) && !is_admin(p_ptr)) {
			if ((int)(p_ptr->lev / cfg.houses_per_player) == 0)
				msg_format(Ind, "You need to be at least level %d to buy a house!", cfg.houses_per_player);
			else if ((int)(p_ptr->lev / cfg.houses_per_player) == 1)
				msg_print(Ind, "At your level, you cannot own more than 1 house!");
			else
				msg_format(Ind, "At your level, you cannot own more than %d houses!", (int)((p_ptr->lev > 50 ? 50 : p_ptr->lev) / cfg.houses_per_player));
			return;
		}
		if (houses[h_idx].flags & HF_MOAT) {
			if (cfg.castles_for_kings && !p_ptr->total_winner) {
				msg_print(Ind, "You must be king or queen, emperor or empress, to buy a castle!");
				return;
			}
			if (cfg.castles_per_player && (p_ptr->castles_owned >= cfg.castles_per_player))  {
				if (cfg.castles_per_player == 1)
					msg_format(Ind, "You cannot own more than 1 castle!");
				else
					msg_format(Ind, "You cannot own more than %d castles!", cfg.castles_per_player);
				return;
			}
		}
		msg_format(Ind, "You buy the house for %d gold.", price);
		p_ptr->au -= price;
#ifdef USE_SOUND_2010
		sound(Ind, "drop_gold", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
		p_ptr->houses_owned++;
		if (houses[h_idx].flags & HF_MOAT) p_ptr->castles_owned++;
		s_printf("BUY_HOUSE: %s buys %d (%d,%d) [%d,%d] for %d\n", p_ptr->name, h_idx, wpos->wx, wpos->wy, houses[h_idx].dx, houses[h_idx].dy, price);

		//ACC_HOUSE_LIMIT
		acc_houses++;
		acc_set_houses(p_ptr->accountname, acc_houses);
		clockin(Ind, 8);

		dna->creator = p_ptr->dna;
		dna->mode = p_ptr->mode;
		dna->owner = p_ptr->id;
		dna->owner_type = OT_PLAYER;
		dna->a_flags = ACF_NONE;
		dna->min_level = 1;

		/* Erase house contents */
		kill_house_contents(h_idx);

		/* Redraw */
		p_ptr->redraw |= (PR_GOLD);

		/* take note of door colour change */
		everyone_lite_spot(wpos, y, x);
	}
}

/* King own a territory */
void do_cmd_own(int Ind) {
	player_type *p_ptr = Players[Ind];
	char buf[100];

	if (!p_ptr->total_winner) {
		msg_format(Ind, "You must be a %s to own a land!", (p_ptr->male)?"king":"queen");
		return;
	}

	if (!p_ptr->own1.wx && !p_ptr->own2.wx && !p_ptr->own1.wy && !p_ptr->own2.wy && !p_ptr->own1.wz && !p_ptr->own2.wz) {
		msg_print(Ind, "You can't own more than 2 terrains.");
		return;
	}

	if (wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].own) {
		msg_print(Ind, "Sorry this land is owned by someone else.");
		return;
	}
	if (p_ptr->wpos.wz) {
		msg_print(Ind, "Sorry you can't own the dungeon");
		return;
	}

	if (istownarea(&p_ptr->wpos, MAX_TOWNAREA)) {
		msg_print(Ind, "Sorry this land is owned by the town.");
		return;
	}

	/* Ok all check did lets appropriate */
	if (p_ptr->own1.wx || p_ptr->own1.wy || p_ptr->own1.wz)
		wpcopy(&p_ptr->own2, &p_ptr->wpos);
	else
		wpcopy(&p_ptr->own1, &p_ptr->wpos);
	wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].own = p_ptr->id;

	if (p_ptr->mode & (MODE_HARD | MODE_NO_GHOST)) {
		snprintf(buf, sizeof(buf), "%s %s now owns (%d,%d).", (p_ptr->male) ? "Emperor" : "Empress", p_ptr->name, p_ptr->wpos.wx, p_ptr->wpos.wy);
	} else {
		snprintf(buf, sizeof(buf), "%s %s now owns (%d,%d).", (p_ptr->male) ? "King" : "Queen", p_ptr->name, p_ptr->wpos.wx, p_ptr->wpos.wy);
	}
	msg_broadcast(Ind, buf);
	msg_print(Ind, buf);
}



/* mental fusion - mindcrafter special ability */
void do_cmd_fusion(int Ind) {
#ifdef TEST_SERVER
	player_type *p_ptr = Players[Ind], *q_ptr;
	cave_type **zcave;//, *c_ptr;
	int x, y, dir, i;

	if (p_ptr->esp_link) {
		msg_print(Ind, "\377yYou are already in a mind fusion.");
		return;
	}

	if (!(zcave = getcave(&p_ptr->wpos))) return;

	/* search grids adjacent to player for someone with open mind to fuse */
	for (dir = 0; dir < 8; dir++) {
		y = p_ptr->py + ddy_ddd[dir];
		x = p_ptr->px + ddx_ddd[dir];
		if (!in_bounds(y, x)) continue;

		/* Get requested grid, check for friendly open minded player */
		i = -zcave[y][x].m_idx;
		if (i <= 0) continue;
		q_ptr = Players[i];

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;
		/* Skip DM if not DM himself */
		if (q_ptr->admin_dm && !p_ptr->admin_dm) continue;
		/* Skip Ghosts */
		if (q_ptr->ghost && !q_ptr->admin_dm) continue;
		/* Skip players not in the same party */
		if (q_ptr->party == 0 || p_ptr->party != q_ptr->party)
		/* Skip players who haven't opened their mind */
		if (!(q_ptr->esp_link_flags & LINKF_OPEN)) continue;
		/* Skip players who are already in a fusion */
		if (q_ptr->esp_link) continue;

		/* found one - establish! */
		q_ptr->esp_link = p_ptr->id;
		p_ptr->esp_link = q_ptr->id;
		q_ptr->esp_link_type = LINK_DOMINANT; /* transmit to me */
		p_ptr->esp_link_type = LINK_DOMINATED; /* receive from him */
		p_ptr->esp_link_end = q_ptr->esp_link_end = 0;
		q_ptr->esp_link_flags = LINKF_VIEW | LINKF_PAIN | LINKF_MISC | LINKF_OBJ;
		p_ptr->esp_link_flags = LINKF_VIEW_DEDICATED; /* don't show own map info */
		p_ptr->update |= PU_MUSIC; /* don't hear own music */
		/* redraw map of target player, which will redraw our view too */
		q_ptr->redraw |= PR_MAP;

		/* notify and done */
		msg_format(Ind, "\377BMind fusion with %s was successful!", q_ptr->name);
		msg_format(i, "\377BMind fusion with %s was successful!", p_ptr->name);
		return;
	}

	/* failure */
	msg_print(Ind, "\377yNo eligible target found nearby.");
#endif
}

/*
 * Rogue special ability - enter cloaking (stealth+search+special invisibility) mode - C. Blue
 */
void do_cmd_cloak(int Ind) {
	player_type *p_ptr = Players[Ind];

#ifdef ENABLE_MCRAFT
 #if 0 /* instead via spell schools atm */
	/* hack: also use the key for fusion,
	   a new mindcrafter special ability - C. Blue */
	if (p_ptr->pclass == CLASS_MINDCRAFTER) {
		do_cmd_fusion(Ind);
		return;
	}
 #endif
#endif

#ifdef ARCADE_SERVER
return;
#endif

#ifndef ENABLE_CLOAKING
	return;
#endif
	/* Don't allow cloaking while inside a store */
	if (p_ptr->store_num != -1) return;

	if (p_ptr->pclass != CLASS_ROGUE) {
		msg_format(Ind, "\377yOnly rogues can use cloaking.");
		return;
	}

    if (!p_ptr->cloaked) {
	if (p_ptr->lev < LEARN_CLOAKING_LEVEL) {
		msg_format(Ind, "\377yYou need to be level %d to learn how to cloak yourself effectively.", LEARN_CLOAKING_LEVEL);
		return;
	}
	if (p_ptr->body_monster) { /* in case of vampire bat, for vampire rogue! */
		msg_print(Ind, "\377yYou cannot cloak yourself while you are transformed.");
		return;
	}
	if (p_ptr->rogue_heavyarmor) {
		msg_print(Ind, "\377yYour armour is too heavy to cloak yourself effectively.");
		return;
	}
	if (p_ptr->inventory[INVEN_WIELD].k_idx && (k_info[p_ptr->inventory[INVEN_WIELD].k_idx].flags4 & (TR4_MUST2H | TR4_SHOULD2H))
	    && !p_ptr->instakills) {
		msg_print(Ind, "\377yYour weapon is too large to cloak yourself effectively.");
		return;
	}
	if (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD) {
		msg_print(Ind, "\377yYou cannot cloak yourself effectively while carrying a shield.");
		return;
	}
	if (p_ptr->stormbringer) {
		msg_print(Ind, "\377yYou cannot cloak yourself while wielding the Stormbringer!");
		return;
	}
	if (p_ptr->aggravate) {
		msg_print(Ind, "\377yYou cannot cloak yourself while using aggravating items!");
		return;
	}
	if (p_ptr->ghost) {
		msg_print(Ind, "\377yYou cannot cloak yourself while you are dead!");
		if (!is_admin(p_ptr)) return;
	}
	if (p_ptr->blind) {
		msg_print(Ind, "\377yYou cannot cloak yourself while blind.");
		return;
	}
	if (p_ptr->confused) {
		msg_print(Ind, "\377yYou cannot cloak yourself while confused.");
		return;
	}
	if (p_ptr->stun > 50) {
		msg_print(Ind, "\377yYou cannot cloak yourself while heavily stunned.");
		return;
	}
	if (p_ptr->paralyzed) { /* just paranoia, no energy anyway */
		msg_print(Ind, "\377yYou cannot cloak yourself while paralyzed.");
		if (!is_admin(p_ptr)) return;
	}
    }

	/* S(he) is no longer afk */
#if 0
	un_afk_idle(Ind);
#else
	p_ptr->idle_char = 0;
	if (p_ptr->muted_when_idle) Send_idle(Ind, FALSE);
#endif

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	if (p_ptr->cloaked > 1) {
		stop_cloaking(Ind);
	} else if (p_ptr->cloaked == 1) {
		msg_print(Ind, "\377WYou uncloak yourself.");
		p_ptr->cloaked = 0;
	} else {
		break_shadow_running(Ind);
		stop_precision(Ind);
		stop_shooting_till_kill(Ind);
		/* stop other actions like running.. */
		disturb(Ind, 1, 0);
		/* prepare to cloak.. */
		msg_print(Ind, "\377sYou begin to cloak your appearance...");
		msg_format_near(Ind, "\377w%s begins to cloak %s appearance...", p_ptr->name, p_ptr->male ? "his" : "her");
		p_ptr->cloaked = 10; /* take this number of seconds-1 to complete */
	}
	p_ptr->update |= (PU_BONUS | PU_LITE | PU_VIEW | PU_MONSTERS);
	p_ptr->redraw |= (PR_STATE | PR_SPEED);
}

/* break cloaking -
   'discovered=0' means that the character just cannot keep himself cloaked,
   while 'discovered > 0' stands for a suspicious action that might have been
   noted by an observing monster and hence neutralizes his disguise for n turns. - C. Blue */
void break_cloaking(int Ind, int discovered) {
	if (!discovered) {
		if (Players[Ind]->cloaked) {
			msg_print(Ind, "\377oYour camouflage drops!");
			if (Players[Ind]->cloaked == 1) msg_format_near(Ind, "%s appears before your eyes.", Players[Ind]->name);
			Players[Ind]->cloaked = 0;
			Players[Ind]->update |= (PU_BONUS | PU_LITE | PU_VIEW);
			Players[Ind]->redraw |= (PR_STATE | PR_SPEED);
		}
	}
	/* neutralize cloak effect for a number of <discovered> turns,
	   due to a suspicious action we performed that might have been
	   noticed by a monster */
	if (discovered > Players[Ind]->cloak_neutralized) Players[Ind]->cloak_neutralized = discovered;
}

/* stop cloaking preparations */
void stop_cloaking(int Ind) {
	if (Players[Ind]->cloaked > 1) {
		msg_print(Ind, "\377yYou stop cloaking preparations.");
		Players[Ind]->cloaked = 0;
		Players[Ind]->update |= (PU_BONUS | PU_LITE | PU_VIEW | PU_MONSTERS);
		Players[Ind]->redraw |= (PR_STATE | PR_SPEED);

	}
}

/* stop ranged technique 'barrage' preparation */
void stop_precision(int Ind) {
	if (Players[Ind]->ranged_precision) {
		Players[Ind]->ranged_precision = FALSE;
		msg_print(Ind, "\377yYou stop aiming.");
	}

	/* abuse for tunnelling for sling ammo creation,
	   since it's the same situation: 'player must not perform any other action till complete'
	   - except for firing, but firing isn't possible while tunnelling, so it's fine. */
	Players[Ind]->current_create_sling_ammo = FALSE;
}

/* stop shooting-till-kill */
void stop_shooting_till_kill(int Ind) {
	player_type *p_ptr = Players[Ind];

	p_ptr->shooting_till_kill = FALSE;

	p_ptr->shoot_till_kill_book = 0;
	p_ptr->shoot_till_kill_spell = 0;
	p_ptr->shoot_till_kill_mimic = 0;
	p_ptr->shoot_till_kill_wand = 0;
	p_ptr->shoot_till_kill_rod = 0;

	if (p_ptr->shoot_till_kill_rcraft) {
		p_ptr->FTK_e_flags = 0;
		p_ptr->FTK_m_flags = 0;
		p_ptr->FTK_energy = 0;

		p_ptr->shoot_till_kill_rcraft = FALSE;
	}
}

/* stop ranged technique 'barrage' preparation */
void bandage_fails(int Ind) {
	player_type *p_ptr = Players[Ind];
	int tmp;

	if (!p_ptr->cut_bandaged) return;

	msg_print(Ind, "\376Your bandage comes off and your wound reopens!");
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* clear cut_intrinsic_nocut properly */
	tmp  = p_ptr->cut_bandaged;
	p_ptr->cut_bandaged = 0;
	(void)set_cut(Ind, p_ptr->cut + tmp, p_ptr->cut_attacker, TRUE);
}

/*
 * Rogue special ability - shadow running mode - C. Blue
 */
void shadow_run(int Ind) {
	player_type *p_ptr = Players[Ind];

	if (p_ptr->shadow_running) {
		p_ptr->shadow_running = FALSE;
		msg_print(Ind, "Your silhouette stabilizes and your movements return to normal.");
		msg_format_near(Ind, "%s silhouette stabilizes and %s movements return to normal.", p_ptr->name, p_ptr->male ? "his" : "her");
		p_ptr->update |= (PU_BONUS | PU_VIEW);
		p_ptr->redraw |= (PR_STATE | PR_SPEED);
		/* update so everyone sees the colour animation */
		everyone_lite_spot_move(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
		return;
	}

	if (p_ptr->body_monster) { /* in case of vampire bat, for vampire rogue! */
		msg_print(Ind, "\377yYou cannot shadow run while you are transformed.");
		return;
	}
	if (p_ptr->rogue_heavyarmor) {
		msg_print(Ind, "\377yYour armour is too heavy for effective shadow running.");
		return;
	}
	if (p_ptr->inventory[INVEN_WIELD].k_idx && (k_info[p_ptr->inventory[INVEN_WIELD].k_idx].flags4 & (TR4_MUST2H | TR4_SHOULD2H))
	    && !p_ptr->instakills) {
		msg_print(Ind, "\377yYour weapon is too large for effective shadow running.");
		return;
	}
	if (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD) {
		msg_print(Ind, "\377yYou cannot shadow run effectively while carrying a shield.");
		return;
	}
	if (p_ptr->stormbringer) {
		msg_print(Ind, "\377yYou cannot shadow run while wielding the Stormbringer!");
		return;
	}
	if (p_ptr->ghost) {
		msg_print(Ind, "\377yYou cannot shadow run while you are dead!");
		if (!is_admin(p_ptr)) return;
	}
	if (p_ptr->blind) {
		msg_print(Ind, "\377yYou cannot shadow run while blind.");
		return;
	}
	if (p_ptr->confused) {
		msg_print(Ind, "\377yYou cannot shadow run while confused.");
		return;
	}
	if (p_ptr->stun) {
		msg_print(Ind, "\377yYou cannot start shadow running while stunned.");
		return;
	}
	if (p_ptr->paralyzed) { /* just paranoia, no energy anyway */
		msg_print(Ind, "\377yYou cannot start shadow running while paralyzed.");
		if (!is_admin(p_ptr)) return;
	}

	if (p_ptr->cst < 10) { msg_print(Ind, "Not enough stamina!"); return; }

	use_stamina(p_ptr, 10);
	un_afk_idle(Ind);
	disturb(Ind, 1, 0); /* stop resting, searching and running */

	break_cloaking(Ind, 0);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
	p_ptr->shadow_running = TRUE;
	msg_print(Ind, "Your silhouette turns shadowy and your movements become lightning-fast!");
	switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
	case 's': case 'x': case 'z':
		msg_format_near(Ind, "%s' silhouette turns shadowy and %s movements become lightning-fast!", p_ptr->name, p_ptr->male ? "his" : "her");
		break;
	default:
		msg_format_near(Ind, "%s's silhouette turns shadowy and %s movements become lightning-fast!", p_ptr->name, p_ptr->male ? "his" : "her");
	}
	p_ptr->update |= (PU_BONUS | PU_VIEW);
	p_ptr->redraw |= (PR_STATE | PR_SPEED);
	/* update so everyone sees the colour animation */
	everyone_lite_spot_move(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
}

/* break shadow running */
void break_shadow_running(int Ind) {
	if (Players[Ind]->shadow_running) {
		msg_print(Ind, "Your silhouette stabilizes and your movements return to normal.");
		msg_format_near(Ind, "%s silhouette stabilizes and %s movements return to normal.", Players[Ind]->name, Players[Ind]->male ? "his" : "her");
		Players[Ind]->shadow_running = FALSE;
		Players[Ind]->update |= (PU_BONUS | PU_VIEW);
		Players[Ind]->redraw |= (PR_STATE | PR_SPEED);
		/* update so everyone sees the colour animation */
		everyone_lite_spot_move(Ind, &Players[Ind]->wpos, Players[Ind]->py, Players[Ind]->px);
	}
}
