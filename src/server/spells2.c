/* $Id$ */
/* File: spells2.c */

/* Purpose: Spell code (part 2) */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER
#include "angband.h"

/*
 * A warlock could easily beat the game by scumming & genociding vaults;
 * Here're some codes against this method.		- Jir -
 *
 * Note also that some vaults/quests will be genocide-resistant.
 */
/*
 * (Mass) Genocide spell being way too powerful, the caster's HP is halved
 * if this option is set.
 */
#define SEVERE_GENO
/*
 * Geno won't work if inside a vault(icky).
 * Strongly recommended.
 */
#define NO_GENO_ON_ICKY
/*
 * Each monster has (level) in RESIST_GENO chance of surviving this.
 */
#define RESIST_GENO 250

/*
 * 'Trap detection' is way too powerful traps and 'searching' ability
 * are almost meanless; if this flag is defined, the spell can only
 * detect some of the traps depending on the player level.
 */
//#define TRAP_DETECTION_FAILURE /* appearently unused */

/* for PVP mode diminishing healing calc - C. Blue */
//#define PVP_DIMINISHING_HEALING_CAP(p) (3 * ((p)->mhp + (p)->lev * 6)) /* unfair for non-mimics */
#define PVP_DIMINISHING_HEALING_CAP(p) (((p)->lev + 5) * ((p)->lev + 5)) /* 10: 225, 20: 625, 30: 1225 */

/* Give 'absence' messages for detection magic?
   This would need magic devices causing these to get identified, since we
   can read what they supposedly do. */
//#define DETECT_ABSENCE

/* Helper function to educe spell damage by 50% while in wraithform.
 *  Some spells must be excempt, because their 'dam'
 * actually stores their functionality. - C. Blue
 */
static void proj_dam_wraith(int typ, int *dam) {
	switch (typ) {
	case GF_RECALL_PLAYER: /* <- dam is timeout! */
	case GF_RESTORE_PLAYER:
	case GF_CURE_PLAYER:
	case GF_CURING:
	case GF_EXTRA_STATS:
		return;
	case GF_HEAL_PLAYER:
		*dam = (*dam & 0x3C00) + (*dam & 0x03FF) / 2;
		return;
	case GF_OLD_DRAIN:
		/* - sorry, 4096 is the priest spell hack :P */
		*dam = (*dam & 0x1000) + (*dam & 0x0FFF) / 2;
		break;
	case GF_ANNIHILATION:
		/* - sorry, 8192 is the 'Brief' rune spell hack :P */
		*dam = (*dam & 0x2000) + (*dam & 0x1FFF) / 2;
		break;
	case GF_DARK_WEAK:
		/* - sorry, 8192 is the shadow spell hack :P */
		*dam = (*dam & 0x2000) + (*dam & 0x1FFF) / 2;
		break;
	default:
		*dam /= 2;
	}
}

#ifdef ENABLE_MAIA
/*
 * For angelic beings, this spell will gather any party
 * members on the same level to him/herself. This also
 * tele_to's all monsters in LOS.
 * For demonic beings, do a dispel on LOS.
 */
void divine_vengeance(int Ind, int power) {
	player_type *p_ptr = Players[Ind];
	bool summon = !istownarea(&p_ptr->wpos, MAX_TOWNAREA);

	if (p_ptr->ptrait == TRAIT_ENLIGHTENED) {
		int i;
		/* players TELE_TO */
		if (p_ptr->party == 0) {
			msg_print(Ind, "You can only teleport to party members.");
		} else {
			fire_ball(Ind, GF_KILL_GLYPH, 0, 0, 2, "");
			for (i = 1; i <= NumPlayers; i++) {
				/* Skip self */
				if (i == Ind) continue;

				player_type *q_ptr = Players[i];

				/* Skip disconnected players */
				if (q_ptr->conn == NOT_CONNECTED) continue;

				/* Skip players not on this depth */
				if (!inarea(&q_ptr->wpos, &p_ptr->wpos)) continue;

				/* Skip DM if not DM himself */
				if (q_ptr->admin_dm && !p_ptr->admin_dm) continue;

				/* Skip players who haven't opened their mind */
				if (!(q_ptr->esp_link_flags & LINKF_OPEN)) continue;

				/* Skip players not in the same party */
				if (q_ptr->party == 0 || p_ptr->party == 0) continue;
				if (p_ptr->party != q_ptr->party) continue;

				/* Have a present from the nether world for each player you teleport! */
				if (summon)
#ifdef USE_SOUND_2010
					if (summon_specific(&p_ptr->wpos, p_ptr->py, p_ptr->px, getlevel(&p_ptr->wpos), 100, SUMMON_MONSTER, 0, cfg.clone_summoning))
						sound_near_site(p_ptr->py, p_ptr->px, &p_ptr->wpos, 0, "summon", NULL, SFX_TYPE_MISC, FALSE);
#else
					(void)summon_specific(&p_ptr->wpos, p_ptr->py, p_ptr->px, getlevel(&p_ptr->wpos), 100, SUMMON_MONSTER, 0, cfg.clone_summoning);
#endif

				teleport_player_to(i, p_ptr->py, p_ptr->px);
			}
		}
		/* monsters TELE_TO */
		project_los(Ind, GF_TELE_TO, 0, " commands return");
	} else if (p_ptr->ptrait == TRAIT_CORRUPTED) {
		dispel_monsters(Ind, power);
		//project_los(Ind, GF_DISP_ALL, power, " commands leave");
	}
}

int py_create_gateway(int Ind) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &(p_ptr->wpos);
	cave_type **zcave;
	struct dun_level *l_ptr = getfloor(wpos);

	if (!wpos->wz || !(zcave = getcave(wpos)) || !cave_clean_bold(zcave, p_ptr->py, p_ptr->px)) {
		msg_format(Ind, "\377wThis spell has no effect here.");
		return 0;
	}

	if ((l_ptr->flags1 & LF1_CUSTOM_GATEWAY)) {
		msg_print(Ind, "\377yThe presence of another active gateway spell is disturbing your attempt.");
		return 0;
	}

	if (p_ptr->voidx && p_ptr->voidy) {
		if (place_between_targetted(wpos, p_ptr->py, p_ptr->px, p_ptr->voidy, p_ptr->voidx)) {
			//TODO: jumpgate limit? Using level flag for this here:
			l_ptr->flags1 |= LF1_CUSTOM_GATEWAY;

			//When successfull, we clear state, so we can create more.
			p_ptr->voidx = 0;
			p_ptr->voidy = 0;

			p_ptr->redraw |= (PR_MAP);
			msg_format(Ind, "\377vYou successfully create a gateway between the two points!");
			msg_format_near(Ind, "\377v%s has successfully created a gateway!", p_ptr->name);
		}
		/* completed the spell successfully */
		return 2;
	}

	// Let's not have this in towns/houses; OK everywhere else
	if (!allow_terraforming(wpos, FEAT_TREE)) return 0;

	msg_format(Ind, "\377vYou set your mind to create a gateway here.");

	p_ptr->voidx = p_ptr->px;
	p_ptr->voidy = p_ptr->py;

	place_between_dummy(wpos, p_ptr->py, p_ptr->px);
	return 1;
}
/* A: instant wor for every party member on level. INCLUDING town recalls?
 *    /rec 32 32 -> will attempt to teleport party members to Bree.
 * further note: hm, this could fatally disturb idle party members. also dangerous if all party teleports into a D pit!!. make this only active if player (and therefore interested party members) are not in town.
 * D: void jumpgate creation
 */
void divine_gateway(int Ind) {
	player_type *p_ptr = Players[Ind];
	if (p_ptr->ptrait == TRAIT_ENLIGHTENED) {

		//XXX call set_recall ?
		if (istown(&p_ptr->wpos) || !p_ptr->wpos.wz) {
			msg_format(Ind, "\377wThis spell has no effect here.");
			return;
		}

		// Send some audio feedback
		Send_beep(Ind);

		set_recall_timer(Ind, 1);

		int i;
		for (i = 1; i <= NumPlayers; i++) {
			if (i == Ind) continue;
			if (Players[i]->conn == NOT_CONNECTED) continue;

			/* on the same dungeon floor */
			if (!inarea(&p_ptr->wpos, &Players[i]->wpos)) continue;

			/* must be in the same party */
			if (!Players[i]->party || p_ptr->party != Players[i]->party) continue;

			set_recall_timer(i, 1);
		}
	} else if (p_ptr->ptrait == TRAIT_CORRUPTED)
		(void)py_create_gateway(Ind);
}

#else /*lol, shared .pkg*/
void divine_vengeance(int Ind, int power) {
	return;
}
void divine_gateway(int Ind) {
	return;
}
#endif


/* for mindcrafters: teleport to friendly player with open mind - C. Blue */
void do_autokinesis_to(int Ind, int dis) {
	player_type *p_ptr = Players[Ind], *q_ptr;
	int i;

	if (p_ptr->party == 0) {
		msg_print(Ind, "You can only teleport to party members.");
		return;
	}

	for (i = 1; i <= NumPlayers; i++) {
		/* Skip self */
		if (i == Ind) continue;
		q_ptr = Players[i];
		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;
		/* Skip DM if not DM himself */
		if (q_ptr->admin_dm && !p_ptr->admin_dm) continue;
		/* Skip ghosts */
		if (q_ptr->ghost && !q_ptr->admin_dm) continue;
		/* Skip players not on this depth */
		if (!inarea(&q_ptr->wpos, &p_ptr->wpos)) continue;
		/* Skip players not in the same party */
		if (q_ptr->party == 0 || p_ptr->party != q_ptr->party) continue;
		/* Skip players who haven't opened their mind */
		if (!(q_ptr->esp_link_flags & LINKF_OPEN)) continue;
		/* Skip targets too far away */
		if (distance(p_ptr->py, p_ptr->px, q_ptr->py, q_ptr->px) > dis) continue;

		/* success */
		msg_print(Ind, "You reach an allied mind!");
		teleport_player_to(Ind, q_ptr->py, q_ptr->px);
		return;
	}

	/* fail */
	msg_print(Ind, "You couldn't make out any destination within your mental reach.");
}

/*
 * Grow trees
 */
void grow_trees(int Ind, int rad) {
	player_type *p_ptr = Players[Ind];
	int a, i, j;

	if (!allow_terraforming(&p_ptr->wpos, FEAT_TREE)) return;

#ifdef USE_SOUND_2010
        sound(Ind, "grow_trees", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

	for (a = 0; a < rad * rad + 11; a++) {
		cave_type **zcave = getcave(&p_ptr->wpos);

		i = (rand_int((rad * 2) + 1) - rad + rand_int((rad * 2) + 1) - rad) / 2;
		j = (rand_int((rad * 2) + 1) - rad + rand_int((rad * 2) + 1) - rad) / 2;

		if (!in_bounds2(&p_ptr->wpos, p_ptr->py + j, p_ptr->px + i)) continue;
		if (distance(p_ptr->py, p_ptr->px, p_ptr->py + j, p_ptr->px + i) > rad) continue;

		if (cave_naked_bold(zcave, p_ptr->py + j, p_ptr->px + i) &&
		    (zcave[p_ptr->py + j][p_ptr->px + i].feat != FEAT_HOME_OPEN)) /* HACK - not on open house door - mikaelh */
		{
			cave_set_feat_live(&p_ptr->wpos, p_ptr->py + j, p_ptr->px + i, magik(50)?FEAT_TREE:FEAT_BUSH);
#if 1
			/* Redraw - the trees might block view and cause wall shading etc! */
			for (i = 1; i <= NumPlayers; i++) {
				/* If he's not playing, skip him */
				if (Players[i]->conn == NOT_CONNECTED) continue;
				/* If he's not here, skip him */
				if (!inarea(&p_ptr->wpos, &Players[i]->wpos)) continue;

				Players[i]->update |= (PU_VIEW | PU_LITE | PU_FLOW); //PU_DISTANCE, PU_TORCH, PU_MONSTERS??; PU_FLOW needed? both VIEW and LITE needed?
			}
#endif
		}
	}
}

/*
 * Garden of the Gods handler :-)
 * Basically go through each wall tile in dungeon. For each one, there is a chance% of it
 * getting turned into a tree. - the_sandman
 */
bool create_garden(int Ind, int chance) {
	int y, x;
	player_type *p_ptr;	//Who(and where)?
	cave_type *c_ptr;
	p_ptr = Players[Ind];

	struct c_special *cs_ptr;       /* for special key doors */
	struct worldpos *wpos = &(p_ptr->wpos);

	cave_type **zcave;
	if(!(zcave = getcave(wpos))) return (FALSE);

	if (!allow_terraforming(wpos, FEAT_TREE)) return(FALSE);

	for (y = 0; y < MAX_HGT; y++) {
		for (x = 0; x < MAX_WID; x++) {
			if (!in_bounds(y, x)) continue;

			c_ptr = &zcave[y][x];

			/*
			 * Never transform vaults (altough it would create some potentially
			 * nasty effects >;) Imagine all those Z seeping out to get you...
			 */
			if (c_ptr->info & CAVE_ICKY) continue;

			if((cs_ptr = GetCS(c_ptr, CS_KEYDOOR))) continue;

			if (cave_valid_bold(zcave, y, x) && ( /* <- destroyable, no art on grid, not FF1_PERMANENT */
//				(c_ptr->feat == FEAT_QUARTZ) ||
//				(c_ptr->feat == FEAT_MAGMA) ||
//				(c_ptr->feat == FEAT_QUARTZ_H) ||
//				(c_ptr->feat == FEAT_QUARTZ_K) ||
//				(c_ptr->feat == FEAT_MAGMA_K) ||
//				(c_ptr->feat == FEAT_SAND_WALL) ||
//				(c_ptr->feat == FEAT_SAND_WALL_H) ||
//				(c_ptr->feat == FEAT_SAND_WALL_K) ||
//				(c_ptr->feat == FEAT_ICE_WALL) ||
//				(c_ptr->feat == FEAT_GLASS_WALL) ||
//				(c_ptr->feat == FEAT_ILLUS_WALL) ||
				(c_ptr->feat == FEAT_WALL_EXTRA) || /* granite walls: */
				(c_ptr->feat == FEAT_WALL_INNER) ||
				(c_ptr->feat == FEAT_WALL_OUTER) ||
				(c_ptr->feat == FEAT_WALL_SOLID))
//				&& !cave_floor_bold(zcave, y, x) /* don't convert empty floor! */
//				&& !(f_info[c_ptr->feat].flags1 & FF1_PERMANENT)
			    )
			{
				if (randint(100) < chance) {
					/* Delete the object (if any) */
					delete_object(wpos, y, x, TRUE);
					cave_set_feat_live(&p_ptr->wpos, y, x, magik(50)?FEAT_TREE:FEAT_BUSH);
					//c_ptr->feat = feat;
				}
			}
		} //width
	} //height

	/* Message */
	msg_print(Ind, "You see raw power escaping from the rocks!");
	msg_print_near(Ind, "You see raw power escaping from the rocks!");
	return (TRUE);
}//garden

/*
 * this will now add to both melee and ranged toHit instead at a rate of
 * spell_lvl/2
 * Yep. More druidry. - the_sandman
 */
bool do_focus(int Ind, int p, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->focus_time) {
			switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
			case 's': case 'x': case 'z':
				msg_format_near(Ind, "%s' arm movements turn blurry!", p_ptr->name);
				break;
			default:
				msg_format_near(Ind, "%s's arm movements turn blurry!", p_ptr->name);
			}
			msg_print(Ind, "Your arms feel... dexterous!");
			notice = TRUE;
		}
		p_ptr->focus_time = v;
		p_ptr->focus_val = p;
		calc_boni(Ind);
	}

	/* Shut */
	else {
		if (p_ptr->focus_time) {
			switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
			case 's': case 'x': case 'z':
				msg_format_near(Ind, "You can see %s' arms again.", p_ptr->name);
				break;
			default:
				msg_format_near(Ind, "You can see %s's arms again.", p_ptr->name);
			}
			msg_print(Ind, "Your feel your arms turn into lead.");
			notice = TRUE;
			p_ptr->focus_time = 0;
		}
		p_ptr->focus_val = 0;
	}

#if 0
//Buggy with another set_fast
	/* Decrease speed */
	if (p_ptr->focus_time != 0) set_fast(Ind, v, -(p / 5));
#endif

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Extra stats! By how much depends on the player's level.
 * At the moment it is +1 for every 7.
 * Druidry. - the_sandman
 */
bool do_xtra_stats(int Ind, int s, int p, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = (FALSE);

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->xtrastat_tim || p_ptr->xtrastat_pow < p) {
			msg_format_near(Ind, "%s seems to be more powerful!", p_ptr->name);
			msg_print(Ind, "You feel... powerful!");

			p_ptr->xtrastat_pow = p;
			p_ptr->xtrastat_which = s;

			notice = TRUE;
		}
	}
	/* Shut */
	else {
		if (p_ptr->xtrastat_tim) {
			msg_format_near(Ind, "%s returns to %s normal self.", p_ptr->name, (p_ptr->male? "his" : "her"));
			msg_print(Ind, "You somehow feel weak.");

			p_ptr->xtrastat_pow = 0;
			p_ptr->xtrastat_which = 0;

			notice = TRUE;
		}
	}

	p_ptr->xtrastat_tim = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}//xtra stats

/*
 * The last spell to be added for tonight!
 * "Banish" animals... Killing them, really. - the_sandman
 */
bool do_banish_animals(int Ind, int chance) {
	int i, j = 0, c;
	player_type *p_ptr;
	p_ptr = Players[Ind];
	cave_type **zcave;
	dun_level *l_ptr;
	struct worldpos *wpos = &p_ptr->wpos;

	if (Ind < 0 || chance <= 0) return (FALSE);
	if (!(zcave = getcave(wpos))) return(FALSE);
	if ((l_ptr = getfloor(wpos)) && l_ptr->flags1 & LF1_NO_GENO) return(FALSE);

	for (i = 1; i < m_max; i++) {
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = race_inf(m_ptr);
		if ((r_ptr->flags1 & RF1_UNIQUE)) continue;
		if (!(r_ptr->flags3 & RF3_ANIMAL)) continue;
		if ((r_ptr->flags9 & RF9_IM_TELE)) continue;
		//if ((r_ptr->flags3 & RF3_RES_TELE) && magik(50)) continue;
		if (!inarea(&m_ptr->wpos, wpos)) continue;

		/* Taken from genocide_aux(): Not valid inside a vault */
		if (zcave[m_ptr->fy][m_ptr->fx].info & CAVE_ICKY) continue;

		c = chance - r_ptr->level;
		if (c <= 0) continue;
		if (c > rand_int(100)) {
			//off with you!
			delete_monster_idx(i, TRUE);
			j++;
		}
	}
	if (j < 5) {
		msg_print(Ind, "\337gA few here long for their gardens.");
		msg_format_near(Ind, "\337g%s tries hard but alas can only convert a few animals.", p_ptr->name);
	} else if (j < 10) {
		msg_print(Ind, "\337gA handful here longs for their gardens.");
		msg_format_near(Ind, "\337g%s tries hard but alas can only convert a handful of animals.", p_ptr->name);
	} else if (j < 20) {
		msg_print(Ind, "\337gA pack here abandoned evil to retire in their gardens.");
		msg_format_near(Ind, "\337g%s tries hard and you notice animals going away.", p_ptr->name);
	} else if (j < 50) {
		msg_print(Ind, "\337gIt's a near-miracle!");
		msg_format_near(Ind, "\337g%s banishes animals with conviction.", p_ptr->name);
	} else {
		msg_print(Ind, "\337gThe Day of Returning is here!.");
		/* lol */
		switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
		case 's': case 'x': case 'z':
			msg_format_near(Ind, "\337gYou see %s' tears of joy at the number of returning animals.", p_ptr->name);
			break;
		default:
			msg_format_near(Ind, "\337gYou see %s's tears of joy at the number of returning animals.", p_ptr->name);
		}
	}
	return (TRUE);
}

/* For Dawn activation - C. Blue
   (note: highest non-unique undead is Black Reaver: level 74) */
bool do_banish_undead(int Ind, int chance) {
	int i, j = 0, c;
	player_type *p_ptr;
	p_ptr = Players[Ind];
	cave_type **zcave;
	dun_level *l_ptr;
	struct worldpos *wpos = &p_ptr->wpos;

	if (Ind < 0 || chance <= 0) return (FALSE);
	if (!(zcave = getcave(wpos))) return(FALSE);
	if ((l_ptr = getfloor(wpos)) && l_ptr->flags1 & LF1_NO_GENO) return(FALSE);

	for (i = 1; i < m_max; i++) {
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = race_inf(m_ptr);
		if ((r_ptr->flags1 & RF1_UNIQUE)) continue;
		if (!(r_ptr->flags3 & RF3_UNDEAD)) continue;
		if ((r_ptr->flags9 & RF9_IM_TELE)) continue;
		//if ((r_ptr->flags3 & RF3_RES_TELE) && magik(50)) continue;
		if (!inarea(&m_ptr->wpos, wpos)) continue;

		/* Taken from genocide_aux(): Not valid inside a vault */
		if (zcave[m_ptr->fy][m_ptr->fx].info & CAVE_ICKY) continue;

		c = chance - r_ptr->level;
		if (c <= 0) continue;
		if (c > rand_int(100)) {
			//off with you!
			delete_monster_idx(i, TRUE);
			j++;
		}
	}
	/* (no time for tears of joy dealing with undead) */
	return (TRUE);
}

/* For Mardra activation - C. Blue
   (note: highest non-unique D is GWoP: level 85) */
bool do_banish_dragons(int Ind, int chance) {
	int i, j = 0, c;
	player_type *p_ptr;
	p_ptr = Players[Ind];
	cave_type **zcave;
	dun_level *l_ptr;
	struct worldpos *wpos = &p_ptr->wpos;

	if (Ind < 0 || chance <= 0) return (FALSE);
	if (!(zcave = getcave(wpos))) return(FALSE);
	if ((l_ptr = getfloor(wpos)) && l_ptr->flags1 & LF1_NO_GENO) return(FALSE);

	for (i = 1; i < m_max; i++) {
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = race_inf(m_ptr);
		if ((r_ptr->flags1 & RF1_UNIQUE)) continue;
		if (!(r_ptr->flags3 & RF3_DRAGON)) continue;
		if ((r_ptr->flags9 & RF9_IM_TELE)) continue;
		//if ((r_ptr->flags3 & RF3_RES_TELE) && magik(50)) continue;
		if (!inarea(&m_ptr->wpos, wpos)) continue;

		/* Taken from genocide_aux(): Not valid inside a vault */
		if (zcave[m_ptr->fy][m_ptr->fx].info & CAVE_ICKY) continue;

		c = chance - r_ptr->level;
		if (c <= 0) continue;
		if (c > rand_int(100)) {
			//off with you!
			delete_monster_idx(i, TRUE);
			j++;
		}
	}
	/* (no time for tears of joy dealing with undead) */
	return (TRUE);
}

#ifdef ENABLE_OCCULT /* Occult */
/* Teleport-to a monster */
bool do_shadow_gate(int Ind, int range) {
	player_type *p_ptr = Players[Ind];
	int nx, ny, tx = -1, ty = -1, mdist = 999, dist, idx;
	cave_type **zcave;

	if (!p_ptr) return FALSE;
	zcave = getcave(&p_ptr->wpos);
	if (!zcave) return FALSE;

	for (nx = p_ptr->px - range; nx <= p_ptr->px + range; nx++)
	for (ny = p_ptr->py - range; ny <= p_ptr->py + range; ny++) {
		if (!in_bounds(ny, nx)) continue;
		/* out of range? */
		if ((dist = distance(ny, nx, p_ptr->py, p_ptr->px)) > range) continue;
		/* not a monster or hostile player? */
		if ((idx = zcave[ny][nx].m_idx) <= 0) {//not a monster?
			if (!idx || idx < -NumPlayers) continue;//not a player?
			else if (!check_hostile(-idx, Ind)) continue;
			/* LoS? (player) */
			//if (!p_ptr->play_vis[-idx]) continue;
		}
		/* LoS? (monster) */
		//else if (!p_ptr->mon_vis[idx]) continue;
		/* Real LoS (not ESP)? */
		if (!los(&p_ptr->wpos, p_ptr->py, p_ptr->px, ny, nx)) continue;
		/* don't remember monsters farther away than others */
		if (dist > mdist) continue;
		/* decide randomly between equally close ones */
		if (dist == mdist && rand_int(2)) continue;
		/* remember it! */
		mdist = dist;
		tx = nx;
		ty = ny;
	}
	if (tx == -1) {
		msg_print(Ind, "There is no adversary close enough to you.");
		return FALSE;
	}

	teleport_player_to(Ind, ty, tx);
	return TRUE;
}
#endif

/*
 * Increase players hit points, notice effects, and tell the player about it.
 */
bool hp_player(int Ind, int num) {
	player_type *p_ptr = Players[Ind];

	// The "number" that the character is displayed as before healing
	int old_num, new_num;
	long eff_num; /* actual amount of HP gain */
	long e = PVP_DIMINISHING_HEALING_CAP(p_ptr);

	if (p_ptr->no_heal) return FALSE;

#ifdef AUTO_RET_NEW /* for drain life etc */
	/* Don't allow phase/teleport for auto-retaliation methods */
	if (p_ptr->auto_retaliaty) {
		msg_print(Ind, "\377yYou cannot use means of healing for auto-retaliation.");
		return FALSE;
	}
#endif

	p_ptr->test_heal += num;

	// The "number" that the character is displayed as before healing
	old_num = (p_ptr->chp * 95) / (p_ptr->mhp*10);
	if (old_num >= 7) old_num = 10;

	/* player can't be healed while burning in the holy fire of martyrium */
	if (p_ptr->martyr && !bypass_invuln) return(FALSE);

	/* Hell mode is .. hard */
	if ((p_ptr->mode & MODE_HARD) && (num > 3)) num = num * 3 / 4;

	eff_num = (p_ptr->mhp - p_ptr->chp < num) ? (p_ptr->mhp - p_ptr->chp) : num;

	/* PVP mode uses diminishing healing - C. Blue */
	if (p_ptr->mode & MODE_PVP) {
		eff_num = eff_num * (p_ptr->heal_effect < e ? 100 :
				    ((e * 100) / ((p_ptr->heal_effect - (e * 2) / 3) * 3)));
		eff_num /= 100;
	}

	if (!eff_num) return(FALSE);

	if (p_ptr->chp < p_ptr->mhp) {
		/* data collection for PVP mode: weaken continous healing over time to prevent silliness (who stacks more pots) - C. Blue */
		p_ptr->heal_effect += eff_num;

		/* data collection for C_BLUE_AI: add up healing happening during current turn */
		p_ptr->heal_turn[0] += eff_num;

		/* refill HP, note that we can't use eff_num here due to chp_frac check */
		num = eff_num;
		p_ptr->chp += num;
		if (p_ptr->chp > p_ptr->mhp) {
			p_ptr->chp = p_ptr->mhp;
			p_ptr->chp_frac = 0;
		}

		/* Update health bars */
		update_health(0 - Ind);

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Figure out of if the player's "number" has changed */
		new_num = (p_ptr->chp * 95) / (p_ptr->mhp * 10);
		if (new_num >= 7) new_num = 10;

		/* If so then refresh everyone's view of this player */
		if (new_num != old_num)
			everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		num = num / 5;
		if (num < 3) {
			if (num == 0) {
				msg_print(Ind, "You feel a little better.");
			} else {
				msg_print(Ind, "You feel better.");
			}
		} else {
			if (num < 7) {
				msg_print(Ind, "You feel much better.");
			} else {
				msg_print(Ind, "You feel very good.");
			}
		}

		return (TRUE);
	}

	return (FALSE);
}

/*
 * Increase players hit points, notice effects, and don't tell the player it.
 * 'autoeffect' stands for non-'intended' healing, that applies automatically,
 * such as necromancy, vampiric items, standard body regeneration;
 * set 'autoeffect' to TRUE if you want to do forced healing without any
 * implications.
 */
bool hp_player_quiet(int Ind, int num, bool autoeffect) {
	player_type *p_ptr = Players[Ind];

	// The "number" that the character is displayed as before healing
	int old_num, new_num;
	long eff_num; /* actual amount of HP gain */
	long e = PVP_DIMINISHING_HEALING_CAP(p_ptr);

	if (p_ptr->no_heal && !autoeffect) return FALSE;

#ifdef AUTO_RET_NEW /* for drain life etc */
	/* Don't allow phase/teleport for auto-retaliation methods */
	if (p_ptr->auto_retaliaty && !autoeffect) { /* note: healing by drain-life-autoret is possible with this */
		msg_print(Ind, "\377yYou cannot use means of healing for auto-retaliation.");
		return FALSE;
	}
#endif

	p_ptr->test_heal += num;

	old_num = (p_ptr->chp * 95) / (p_ptr->mhp*10);
	if (old_num >= 7) old_num = 10;

	/* player can't be healed while burning in the holy fire of martyrium */
	if (p_ptr->martyr && !bypass_invuln) return(FALSE);

	/* Hell mode is .. hard */
	if ((p_ptr->mode & MODE_HARD) && (num > 3)) num = num * 3 / 4;

	eff_num = (p_ptr->mhp - p_ptr->chp < num) ? (p_ptr->mhp - p_ptr->chp) : num;

	/* PVP mode uses diminishing healing - C. Blue */
	if (!autoeffect && (p_ptr->mode & MODE_PVP)) {
		eff_num = eff_num * (p_ptr->heal_effect < e ? 100 :
				    ((e * 100) / ((p_ptr->heal_effect - (e * 2) / 3) * 3)));
		eff_num /= 100;
	}

	if (!eff_num) return(FALSE);

	if (p_ptr->chp < p_ptr->mhp) {
		if (!autoeffect) {
			/* data collection for PVP mode: weaken continous healing over time to prevent silliness (who stacks more pots) - C. Blue */
			p_ptr->heal_effect += eff_num;
		}
		/* data collection for C_BLUE_AI: add up healing happening during current turn */
		p_ptr->heal_turn[0] += eff_num;

		/* refill HP, note that we can't use eff_num here due to chp_frac check */
		num = eff_num;
		p_ptr->chp += num;
		if (p_ptr->chp > p_ptr->mhp) {
			p_ptr->chp = p_ptr->mhp;
			p_ptr->chp_frac = 0;
		}

		/* Update health bars */
		update_health(0 - Ind);

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Figure out of if the player's "number" has changed */
		new_num = (p_ptr->chp * 95) / (p_ptr->mhp*10);
		if (new_num >= 7) new_num = 10;

		/* If so then refresh everyone's view of this player */
		if (new_num != old_num)
			everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		return (TRUE);
	}

	return (FALSE);
}


void flash_bomb(int Ind) {
	player_type *p_ptr = Players[Ind];
	msg_print(Ind, "You throw down a blinding flash bomb!");
	msg_format_near(Ind, "%s throws a blinding flash bomb!", p_ptr->name);

#ifdef USE_SOUND_2010
	sound(Ind, "flash_bomb", NULL, SFX_TYPE_COMMAND, TRUE);
#endif

//	project_los(Ind, GF_BLIND, (p_ptr->lev / 10) + 4, "");
	project_los(Ind, GF_BLIND, (p_ptr->lev / 10) + 8, "");
}


/*
 * Leave a "glyph of warding" which prevents monster movement
 */
void warding_glyph(int Ind) {
	player_type *p_ptr = Players[Ind];

	cave_type **zcave;
	if (!(zcave = getcave(&p_ptr->wpos))) return;

	if (!allow_terraforming(&p_ptr->wpos, FEAT_GLYPH) && !is_admin(p_ptr)) return;

	cave_set_feat_live(&p_ptr->wpos, p_ptr->py, p_ptr->px, FEAT_GLYPH);
}


/*
 * Array of stat "descriptions"
 */
static cptr desc_stat_pos[] = {
	"strong",
	"smart",
	"wise",
	"dextrous",
	"healthy",
	"cute"
};

#if 0	// second set for future use
{
	"mighty",
	"intelligent",
	"sagacious",
	"agile",
	"sturdy",
	"charming"
}
#endif	// 0


/*
 * Array of stat "descriptions"
 */
static cptr desc_stat_neg[] = {
	"weak",
	"stupid",
	"naive",
	"clumsy",
	"sickly",
	"ugly"
};

static cptr desc_stat_neg2[] = {
	"strong",
	"bright",
	"wise",
	"agile",
	"hale",
	"beautiful"
};

#if 0
{
	"feeble",
	"dull",
	"foolish",
	"awkward",
	"fragile",
	"indecent"
},
#endif	// 0


/*
 * Lose a "point"
 */
bool do_dec_stat(int Ind, int stat, int mode) {
	player_type *p_ptr = Players[Ind];

	bool sust = FALSE;

	/* Access the "sustain" */
	switch (stat) {
		case A_STR: if (p_ptr->sustain_str) sust = TRUE; break;
		case A_INT: if (p_ptr->sustain_int) sust = TRUE; break;
		case A_WIS: if (p_ptr->sustain_wis) sust = TRUE; break;
		case A_DEX: if (p_ptr->sustain_dex) sust = TRUE; break;
		case A_CON: if (p_ptr->sustain_con) sust = TRUE; break;
		case A_CHR: if (p_ptr->sustain_chr) sust = TRUE; break;
	}

	/* Sustain */
	if (sust || safe_area(Ind)) {
		/* Message */
		msg_format(Ind, "You feel %s for a moment, but the feeling passes.",
		           desc_stat_neg[stat]);

		/* Notice effect */
		return (TRUE);
	}

	/* Attempt to reduce the stat */
	if (dec_stat(Ind, stat, 10, mode)) {
		/* Message */
		msg_format(Ind, "You feel very %s.", desc_stat_neg[stat]);

		/* Notice effect */
		return (TRUE);
	}

	/* Nothing obvious */
	return (FALSE);
}


/*
 * Lose a "point" by a TIME attack (or Black Breath)
 */
bool do_dec_stat_time(int Ind, int stat, int mode, int sust_chance, int reduction_mode, bool msg) {
	player_type *p_ptr = Players[Ind];

	bool sust = FALSE;

	/* Access the "sustain" */
	switch (stat) {
		case A_STR: if (p_ptr->sustain_str) sust = TRUE; break;
		case A_INT: if (p_ptr->sustain_int) sust = TRUE; break;
		case A_WIS: if (p_ptr->sustain_wis) sust = TRUE; break;
		case A_DEX: if (p_ptr->sustain_dex) sust = TRUE; break;
		case A_CON: if (p_ptr->sustain_con) sust = TRUE; break;
		case A_CHR: if (p_ptr->sustain_chr) sust = TRUE; break;
	}

	if (p_ptr->stat_cur[stat] <= 3) return(FALSE);

	/* Sustain */
	if ((sust && magik(sust_chance)) || safe_area(Ind)) {
		/* Message */
		msg_format(Ind, "You don't feel as %s as you used to be, but the feeling passes",
		           desc_stat_neg2[stat]);

		/* Notice effect */
		return (TRUE);
	}

	/* Message */
	if (msg) msg_format(Ind, "You're not as %s as you used to be.", desc_stat_neg2[stat]);

	/* Attempt to reduce the stat */
	switch (reduction_mode) {
	case 0:
		if (dec_stat(Ind, stat, 10, mode)) {
			/* Notice effect */
			return (TRUE);
		}
		break;
	case 1:
		p_ptr->stat_cur[stat] = (p_ptr->stat_cur[stat] * 3) / 4;
		break;
	case 2:
		p_ptr->stat_cur[stat] = (p_ptr->stat_cur[stat] * 6) / 7;
		break;
	case 3:
		p_ptr->stat_cur[stat] = (p_ptr->stat_cur[stat] * 7) / 8;
		break;
	}
	if (p_ptr->stat_cur[stat] < 3) p_ptr->stat_cur[stat] = 3;

	if (mode == STAT_DEC_PERMANENT) p_ptr->stat_max[stat] = p_ptr->stat_cur[stat];

	p_ptr->update |= (PU_BONUS | PU_MANA | PU_HP | PU_SANITY);

	/* Nothing obvious */
	return (TRUE);
}


/*
 * Restore lost "points" in a stat
 */
bool do_res_stat(int Ind, int stat) {
	/* Attempt to increase */
	if (res_stat(Ind, stat)) {
		/* Message */
		msg_format(Ind, "You feel less %s.", desc_stat_neg[stat]);

		/* Notice */
		return (TRUE);
	}

	/* Nothing obvious */
	return (FALSE);
}

/*
 * Restore temporary-lost "points" in a stat
 */
bool do_res_stat_temp(int Ind, int stat) {
	player_type *p_ptr = Players[Ind];

	/* Restore if needed */
	if (p_ptr->stat_cur[stat] != p_ptr->stat_max[stat]) {
		/* Restore */
		p_ptr->stat_cur[stat] += p_ptr->stat_los[stat];

		if (p_ptr->stat_cur[stat] > p_ptr->stat_max[stat])
			p_ptr->stat_cur[stat] = p_ptr->stat_max[stat];

		p_ptr->stat_los[stat] = 0;
		p_ptr->stat_cnt[stat] = 0;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Message */
		msg_format(Ind, "You feel less %s.", desc_stat_neg[stat]);
		/* Success */
		return (TRUE);
	}

	/* Nothing to restore */
	return (FALSE);
}

/*
 * Gain a "point" in a stat
 */
bool do_inc_stat(int Ind, int stat) {
	bool res;

	/* Restore strength */
	res = res_stat(Ind, stat);

	/* Attempt to increase */
	if (inc_stat(Ind, stat)) {
		/* Message */
		msg_format(Ind, "Wow!  You feel very %s!", desc_stat_pos[stat]);

		/* Notice */
		return (TRUE);
	}

	/* Restoration worked */
	if (res) {
		/* Message */
		msg_format(Ind, "You feel less %s.", desc_stat_neg[stat]);

		/* Notice */
		return (TRUE);
	}

	/* Nothing obvious */
	return (FALSE);
}



/* Two people reported that all their !X id scrolls got read at once,
   in rare random cases. (The picked up item gets IDed with the 1st
   scroll as it should be, the rest just get read anyway.)
   This function attempts to suppress that... */
void XID_paranoia(player_type *p_ptr) {
	//p_ptr->current_item = -1;
	p_ptr->command_rep = 0;
	p_ptr->command_rep_temp = 0;
	p_ptr->delayed_spell_temp = 0;
	p_ptr->delayed_spell = 0; //paranoia inside paranoia
}
/*
 * Identify everything being carried.
 * Done by a potion of "self knowledge".
 */
void identify_pack(int Ind) {
	player_type *p_ptr = Players[Ind];

	int                 i;
	object_type        *o_ptr;
	bool inven_unchanged[INVEN_TOTAL];

	XID_paranoia(p_ptr);

	/* Simply identify and know every item */
	for (i = 0; i < INVEN_TOTAL; i++) {
		inven_unchanged[i] = TRUE;
		o_ptr = &p_ptr->inventory[i];

		if (o_ptr->k_idx) {
			if (object_known_p(Ind, o_ptr) && object_aware_p(Ind, o_ptr)) continue;

			object_aware(Ind, o_ptr);
			object_known(o_ptr);
			inven_unchanged[i] = FALSE;
		}
	}

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Handle stuff */
	handle_stuff(Ind);

#if 0 /* moved to client-side, clean! */
	/* hack: trigger client-side auto-inscriptions for convenience,
	   if it isn't due anyway.  */
	if (!p_ptr->inventory_changes) Send_inventory_revision(Ind);
#endif
	/* well, instead: */
	for (i = 0; i < INVEN_TOTAL; i++) {
		if (inven_unchanged[i]) continue;
		Send_apply_auto_insc(Ind, i);
	}
}






/*
 * Used by the "enchant" function (chance of failure)
 */
static int enchant_table[16] = {
	0, 10,  50, 100, 200,
	300, 400, 500, 700, 875,
	960, 980, 990, 996, 999,
	1000
};

/*
 * Lock a door. Currently range of one only.. Maybe a beam
 * would be better?
 */
void wizard_lock(int Ind, int dir){
	player_type *p_ptr;
	cave_type **zcave, *c_ptr;
	int dy, dx;

	p_ptr = Players[Ind];
	if (!p_ptr) return;
	zcave = getcave(&p_ptr->wpos);
	if (!zcave) return;

	/* anti-cheeze: exp'ing with doors */
	if (!p_ptr->wpos.wz) return;

	dx = p_ptr->px + ddx[dir];
	dy = p_ptr->py + ddy[dir];

	c_ptr = &zcave[dy][dx];
	if (c_ptr->feat >= FEAT_DOOR_HEAD && c_ptr->feat < (FEAT_DOOR_HEAD + 7)) {
		if (c_ptr->feat == FEAT_DOOR_HEAD) {
			msg_print(Ind, "The door locks!");
			c_ptr->info |= CAVE_MAGELOCK;
		} else msg_print(Ind, "The door appears stronger!");
		c_ptr->feat++;
	} else if (c_ptr->feat != (FEAT_DOOR_HEAD + 7))
		msg_print(Ind, "You see no lockable door there");
	else
		msg_print(Ind, "The door is locked fast already!");
}

/*
 * Removes curses from items in inventory
 *
 * Note that Items which are "Perma-Cursed" (The One Ring,
 * The Crown of Morgoth) can NEVER be uncursed.
 *
 * Note that if "all" is FALSE, then Items which are
 * "Heavy-Cursed" (Mormegil, Calris, and Weapons of Morgul)
 * will not be uncursed.
 */
/*
 * 0x01 - all
 * 0x02 - limit to one item (added for projectable spell)
 */
int remove_curse_aux(int Ind, int all, int pInd) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
#ifdef NEW_REMOVE_CURSE
	char o_name[ONAME_LEN];
#endif

	int i, j, cnt = 0;
	u32b f1, f2, f3, f4, f5, f6, esp;

#ifdef NEW_REMOVE_CURSE
	if (pInd) msg_format(Ind, "%s recites some holy words..", Players[pInd]->name);
#endif

	for (j = 0; j < INVEN_TOTAL; j++) {
		i = (j + INVEN_WIELD) % INVEN_TOTAL;
		o_ptr = &p_ptr->inventory[i];

		/* Uncursed already */
		if (!cursed_p(o_ptr)) continue;

		/* Extract the flags */
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		/* Heavily Cursed Items need a special spell */
		if (!(all & 0x01) && (f3 & TR3_HEAVY_CURSE)) continue;

		/* Perma-Cursed Items can NEVER be uncursed */
		if ((f3 & TR3_PERMA_CURSE) && !is_admin(p_ptr)) continue;

#ifdef NEW_REMOVE_CURSE
		/* Allow rc to fail sometimes */
		if ((!(all & 0x01) || (f3 & TR3_HEAVY_CURSE)) && magik(o_ptr->level)) {
			object_desc(Ind, o_name, o_ptr, FALSE, 3);
			msg_format(Ind, "Your %s shimmer%s purple for a moment..", o_name, o_ptr->number == 1 ? "s" : "");
			continue;
		}

		object_desc(Ind, o_name, o_ptr, FALSE, 3);
		msg_format(Ind, "Your %s flash%s blue!", o_name, o_ptr->number == 1 ? "es" : "");
#endif

#ifdef VAMPIRES_INV_CURSED
		if (i >= INVEN_WIELD) reverse_cursed(o_ptr);
#endif

		/* Uncurse it */
		o_ptr->ident &= ~ID_CURSED;

		/* Hack -- Assume felt */
		o_ptr->ident |= ID_SENSE | ID_SENSED_ONCE;

		/* Take note */
		note_toggle_cursed(o_ptr, FALSE);

		/* Recalculate the bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Count the uncursings */
		cnt++;

		/* Not everything at once. */
#ifdef NEW_REMOVE_CURSE
		if ((all & 0x02) ||
#else
		if (
#endif
		    (!(all & 0x01) && magik(50)))
			break;
	}

	/* Return "something uncursed" */
	return (cnt);
}


/* well, erm... */
/*
 * Remove most curses
 */
bool remove_curse(int Ind) {
#ifndef NEW_REMOVE_CURSE
	return (remove_curse_aux(Ind, 0x0, 0));
#else
	/* assume we're called _only_ as the projectable holy school spell */
	int i, p, r = 0;
	cave_type **zcave;
	player_type *p_ptr = Players[Ind];

	/* remove our own curse(s) first - can't remove other player's curses while we're not cleansed ;) */
	i = remove_curse_aux(Ind, 0x0, 0);
	if (i) return i;

	/* remove someone else's curse */
	if ((zcave = getcave(&p_ptr->wpos))) {
		/* check grids around the player, first one we find possibly gets a curse fixed */
		for (i = 7; i >= 0; i--) {
			if ((p = zcave[p_ptr->py + ddy_ddd[i]][p_ptr->px + ddx_ddd[i]].m_idx >= 0)) continue;
			r = remove_curse_aux(-p, 0x0 + 0x2, Ind);
			break;
		}
	}

	/* remove our own curse(s) */
	return r;
#endif
}

/*
 * Remove all curses
 */
bool remove_all_curse(int Ind) {
#ifndef NEW_REMOVE_CURSE
	return (remove_curse_aux(Ind, 0x1, 0));
#else
	/* assume we're called _only_ as the projectable holy school spell */
	int i, p, r = 0;
	cave_type **zcave;
	player_type *p_ptr = Players[Ind];

	/* remove our own curse(s) first - can't remove other player's curses while we're not cleansed ;) */
	i = remove_curse_aux(Ind, 0x1, 0);
	if (i) return i;

	/* remove someone else's curse */
	if ((zcave = getcave(&p_ptr->wpos))) {
		/* check grids around the player, first one we find possibly gets a curse fixed */
		for (i = 7; i >= 0; i--) {
			if ((p = zcave[p_ptr->py + ddy_ddd[i]][p_ptr->px + ddx_ddd[i]].m_idx >= 0)) continue;
			r = remove_curse_aux(-p, 0x1 + 0x2, Ind);
			break;
		}
	}
	return r;
#endif
}

/*
 * Restores any drained experience
 */
bool restore_level(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Restore experience */
	if (p_ptr->exp < p_ptr->max_exp) {
		/* Message */
		msg_print(Ind, "You feel your life energies returning.");

		/* Restore the experience */
		p_ptr->exp = p_ptr->max_exp;

		/* Check the experience */
		check_experience(Ind);

		/* Did something */
		return (TRUE);
	}

	/* No effect */
	return (FALSE);
}


/*
 * self-knowledge... idea from nethack.  Useful for determining powers and
 * resistences of items.  It saves the screen, clears it, then starts listing
 * attributes, a screenful at a time.  (There are a LOT of attributes to
 * list.  It will probably take 2 or 3 screens for a powerful character whose
 * using several artifacts...) -CFT
 *
 * It is now a lot more efficient. -BEN-
 *
 * See also "identify_fully()".
 *
 * XXX XXX XXX Use the "show_file()" method, perhaps.
 */
void self_knowledge(int Ind) {
	player_type *p_ptr = Players[Ind];
	int		k;
	u32b	f1 = 0L, f2 = 0L, f3 = 0L, f4 = 0L, f5 = 0L, f6 = 0L;	//, esp = 0L;
	object_type	*o_ptr;
//	cptr	*info = p_ptr->info;
	bool	life = FALSE;
	FILE *fff;
#if 0
	char file_name[MAX_PATH_LENGTH];

	/* Temporary file */
	if (path_temp(file_name, MAX_PATH_LENGTH)) return;
	strcpy(p_ptr->infofile, file_name);
#endif	// 0
	/* Open a new file */
	fff = my_fopen(p_ptr->infofile, "wb");
	/* Current file viewing */
	strcpy(p_ptr->cur_file, p_ptr->infofile);

	/* Output color byte */
//	fprintf(fff, "%c", 'w');

	/* Let the player scroll through the info */
	p_ptr->special_file_type = TRUE;

	/* Acquire item flags from equipment */
	for (k = INVEN_WIELD; k < INVEN_TOTAL; k++) {
		u32b t1, t2, t3, t4, t5, t6, tesp;
		o_ptr = &p_ptr->inventory[k];
		/* Skip empty items */
		if (!o_ptr->k_idx) continue;
		/* Extract the flags */
		object_flags(o_ptr, &t1, &t2, &t3, &t4, &t5, &t6, &tesp);

		/* Extract flags */
		f1 |= t1;
		f2 |= t2;
		f3 |= t3;
		f4 |= t4;
		f5 |= t5;
		f6 |= t6;

		/* Mega Hack^3 -- check the amulet of life saving */
		if (o_ptr->tval == TV_AMULET &&
			o_ptr->sval == SV_AMULET_LIFE_SAVING)
			life = TRUE;
	}

	/* Mega Hack^3 -- describe the amulet of life saving */
	if (life) fprintf(fff, "Your life will be saved from perilous scene once.\n");

#if 0
	k = 100 - p_ptr->csane * 100 / p_ptr->msane;
	if (k < 100) {
//		sprintf(info[i++], "%d%% of your mind is under your control.", k);
		fprintf(fff, "%d%% of your mind is under your control.\n", k);
	}
#else
	/* Insanity warning (better message needed!) */
	if (p_ptr->csane < p_ptr->msane / 8) {
		fprintf(fff, "You are next to mad.\n");
	} else if (p_ptr->csane < p_ptr->msane / 4) {
		fprintf(fff, "Your mind is filled with insane thoughts.\n");
	} else if (p_ptr->csane < p_ptr->msane / 2) {
		fprintf(fff, "You have a seed of insanity in your mind.\n");
	}
#endif

	if (p_ptr->blind) fprintf(fff, "You cannot see.\n");
	if (p_ptr->confused) fprintf(fff, "You are confused.\n");
	if (p_ptr->afraid) fprintf(fff, "You are terrified.\n");
	if (p_ptr->cut) fprintf(fff, "\377rYou are bleeding.\n");
	if (p_ptr->stun) fprintf(fff, "\377oYou are stunned.\n");
	if (p_ptr->poisoned) fprintf(fff, "\377GYou are poisoned.\n");
	if (p_ptr->image) fprintf(fff, "You are hallucinating.\n");

	if (p_ptr->aggravate) fprintf(fff, "You aggravate monsters.\n");
	if (p_ptr->teleport) fprintf(fff, "Your position is very uncertain.\n");

	if (p_ptr->blessed) fprintf(fff, "You feel rightous.\n");
	if (p_ptr->hero) fprintf(fff, "You feel heroic.\n");
	if (p_ptr->shero) fprintf(fff, "You are in a berserk rage.\n");
	if (p_ptr->fury) fprintf(fff, "You are in a wild fury.\n");
	if (p_ptr->protevil) fprintf(fff, "You are protected from evil.\n");
#if 0
	if (p_ptr->protgood) fprintf(fff, "You are protected from good.\n");
#endif	// 0
	if (p_ptr->shield) fprintf(fff, "You are protected by a mystic shield.\n");
	if (p_ptr->invuln) fprintf(fff, "You are temporarily invulnerable.\n");
	if (p_ptr->confusing) fprintf(fff, "Your hands are glowing dull red.\n");
	if (p_ptr->stunning) fprintf(fff, "Your hands are very heavy.\n");
	if (p_ptr->searching) fprintf(fff, "You are looking around very carefully.\n");
#if 0
	if (p_ptr->new_spells) fprintf(fff, "You can learn some more spells.\n");
#endif
	if (p_ptr->word_recall) fprintf(fff, "You will soon be recalled.\n");
	if (p_ptr->see_infra) fprintf(fff, "Your eyes are sensitive to infrared light.\n");
	if (p_ptr->invis) fprintf(fff, "You are invisible.\n");
#if 0 /* really not required to tell the player this stuff, it'd be just spam */
	if (p_ptr->cloaked) fprintf(fff, "You are cloaked.\n");
	if (p_ptr->shadow_running) fprintf(fff, "You are shadow_running.\n");
#endif
	if (p_ptr->see_inv) fprintf(fff, "You can see invisible creatures.\n");
	if (p_ptr->feather_fall) fprintf(fff, "You land gently.\n");
#if 1
	if (p_ptr->climb) fprintf(fff, "You can climb high mountains.\n");
	if (p_ptr->levitate) fprintf(fff, "You can levitate.\n");
	if (p_ptr->can_swim) fprintf(fff, "You can swim easily.\n");
#endif	// 0
	if (p_ptr->free_act) fprintf(fff, "You have free action.\n");
	if (p_ptr->regenerate) fprintf(fff, "You regenerate quickly.\n");
	if (p_ptr->resist_time) fprintf(fff, "You are resistant to time.\n");
	if (p_ptr->resist_mana) fprintf(fff, "You are resistant to magical energy.\n");
	if (p_ptr->immune_water) fprintf(fff, "You are completely protected from unleashed water.\n");
	else if (p_ptr->resist_water) fprintf(fff, "You are resistant to unleashed water.\n");
	if (p_ptr->regen_mana) fprintf(fff, "You accumulate mana quickly.\n");

	if (p_ptr->prace == RACE_MAIA && p_ptr->ptrait) fprintf(fff, "You have no need for worldly food.\n");
	else if (get_skill(p_ptr, SKILL_HSUPPORT) >= 50) fprintf(fff, "You have no need for worldly food.\n");
	else if (p_ptr->ghost) fprintf(fff, "You have no need for worldly food.\n");
	else if (p_ptr->prace == RACE_VAMPIRE && p_ptr->total_winner) fprintf(fff, "Your appetite is small.\n");
	else if (p_ptr->slow_digest) fprintf(fff, "Your appetite is small.\n");

	if (p_ptr->telepathy) {
//		fprintf(fff, "You have ESP.\n");
		if (p_ptr->telepathy & ESP_ALL) fprintf(fff, "You have ESP.\n");
		else {
			if (p_ptr->telepathy & ESP_ORC) fprintf(fff, "You can sense the presence of orcs.\n");
			if (p_ptr->telepathy & ESP_TROLL) fprintf(fff, "You can sense the presence of trolls.\n");
			if (p_ptr->telepathy & ESP_DRAGON) fprintf(fff, "You can sense the presence of dragons.\n");
			if (p_ptr->telepathy & ESP_SPIDER) fprintf(fff, "You can sense the presence of spiders.\n");
			if (p_ptr->telepathy & ESP_GIANT) fprintf(fff, "You can sense the presence of giants.\n");
			if (p_ptr->telepathy & ESP_DEMON) fprintf(fff, "You can sense the presence of demons.\n");
			if (p_ptr->telepathy & ESP_UNDEAD) fprintf(fff, "You can sense presence of undead.\n");
			if (p_ptr->telepathy & ESP_EVIL) fprintf(fff, "You can sense the presence of evil beings.\n");
			if (p_ptr->telepathy & ESP_ANIMAL) fprintf(fff, "You can sense the presence of animals.\n");
			if (p_ptr->telepathy & ESP_DRAGONRIDER) fprintf(fff, "You can sense the presence of dragonriders.\n");
			if (p_ptr->telepathy & ESP_GOOD) fprintf(fff, "You can sense the presence of good beings.\n");
			if (p_ptr->telepathy & ESP_NONLIVING) fprintf(fff, "You can sense the presence of non-living things.\n");
			if (p_ptr->telepathy & ESP_UNIQUE) fprintf(fff, "You can sense the presence of unique beings.\n");
		}
	}
	if (p_ptr->antimagic)	// older (percent)
	{
//		fprintf(fff, "You are surrounded by an anti-magic field.\n");
		if (p_ptr->antimagic >= ANTIMAGIC_CAP) /* AM cap */
			fprintf(fff, "You are surrounded by a complete anti-magic field.\n");
		else if (p_ptr->antimagic >= ANTIMAGIC_CAP - 10)
			fprintf(fff, "You are surrounded by a mighty anti-magic field.\n");
		else if (p_ptr->antimagic >= ANTIMAGIC_CAP - 20)
			fprintf(fff, "You are surrounded by a strong anti-magic field.\n");
		else if (p_ptr->antimagic >= 50)
			fprintf(fff, "You are surrounded by an anti-magic field.\n");
		else if (p_ptr->antimagic >= 30)
			fprintf(fff, "You are surrounded by a weaker anti-magic field.\n");
		else fprintf(fff, "You are surrounded by a feeble anti-magic field.\n");

	}
#if 1
        if (p_ptr->anti_magic)	// newer (saving-throw boost)
                fprintf(fff, "You are surrounded by an anti-magic shell.\n");
#endif	// 0
	if (p_ptr->hold_life) fprintf(fff, "You have a firm hold on your life force.\n");
#if 1	// somewhat hacky
	if (p_ptr->lite) fprintf(fff, "You are carrying a permanent light.\n");
#endif	// 0
	if (p_ptr->auto_id) fprintf(fff, "You can sense magic.\n");
#if 1
	if (p_ptr->reflect) fprintf(fff, "You reflect arrows and bolts.\n");
	if (p_ptr->no_cut) fprintf(fff, "You cannot be cut.\n");

	if (p_ptr->reduce_insanity > 0) {
		fprintf(fff, "Your mind is somewhat resistant against insanity.\n");
	}
	if (p_ptr->suscep_fire) fprintf(fff, "You are susceptible to fire.\n");
	if (p_ptr->suscep_cold) fprintf(fff, "You are susceptible to cold.\n");
	if (p_ptr->suscep_acid) fprintf(fff, "You are susceptible to acid.\n");
	if (p_ptr->suscep_elec) fprintf(fff, "You are susceptible to electricity.\n");
	if (p_ptr->suscep_pois) fprintf(fff, "You are susceptible to poison.\n");
	if (p_ptr->suscep_lite) fprintf(fff, "You are susceptible to light.\n");
	if (p_ptr->suscep_good) fprintf(fff, "You are susceptible to evil-vanquishing effects.\n");
	if (p_ptr->suscep_evil) fprintf(fff, "You are susceptible to good-vanquishing effects.\n");
	if (p_ptr->suscep_life) fprintf(fff, "You are susceptible to undead-vanquishing effects.\n");

	if (p_ptr->sh_fire) fprintf(fff, "You are surrounded with a fiery aura.\n");
	if (p_ptr->sh_elec) fprintf(fff, "You are surrounded with electricity.\n");
	if (p_ptr->sh_cold) fprintf(fff, "You are surrounded with a freezing aura.\n");

	if (p_ptr->resist_continuum) fprintf(fff, "The space-time continuum cannot be disrupted near you.\n");
	if (p_ptr->anti_tele) fprintf(fff, "You are surrounded by an anti-teleportation field.\n");
	if (p_ptr->res_tele) fprintf(fff, "You resist incoming teleportation effects.\n");
#endif	// 0

	if (p_ptr->immune_acid) fprintf(fff, "You are completely immune to acid.\n");
	else if ((p_ptr->resist_acid) && (p_ptr->oppose_acid))
		fprintf(fff, "You resist acid exceptionally well.\n");
	else if ((p_ptr->resist_acid) || (p_ptr->oppose_acid))
		fprintf(fff, "You are resistant to acid.\n");

	if (p_ptr->immune_elec) fprintf(fff, "You are completely immune to lightning.\n");
	else if ((p_ptr->resist_elec) && (p_ptr->oppose_elec))
		fprintf(fff, "You resist lightning exceptionally well.\n");
	else if ((p_ptr->resist_elec) || (p_ptr->oppose_elec))
		fprintf(fff, "You are resistant to lightning.\n");

	if (p_ptr->immune_fire) fprintf(fff, "You are completely immune to fire.\n");
	else if ((p_ptr->resist_fire) && (p_ptr->oppose_fire))
		fprintf(fff, "You resist fire exceptionally well.\n");
	else if ((p_ptr->resist_fire) || (p_ptr->oppose_fire))
		fprintf(fff, "You are resistant to fire.\n");

	if (p_ptr->immune_cold) fprintf(fff, "You are completely immune to cold.\n");
	else if ((p_ptr->resist_cold) && (p_ptr->oppose_cold))
		fprintf(fff, "You resist cold exceptionally well.\n");
	else if ((p_ptr->resist_cold) || (p_ptr->oppose_cold))
		fprintf(fff, "You are resistant to cold.\n");

	if (p_ptr->immune_poison) fprintf(fff, "You are completely immune to poison.\n");
	else if ((p_ptr->resist_pois) && (p_ptr->oppose_pois))
		fprintf(fff, "You resist poison exceptionally well.\n");
	else if ((p_ptr->resist_pois) || (p_ptr->oppose_pois))
		fprintf(fff, "You are resistant to poison.\n");

	if (p_ptr->resist_lite) fprintf(fff, "You are resistant to bright light.\n");
	if (p_ptr->resist_dark) fprintf(fff, "You are resistant to darkness.\n");
	if (p_ptr->resist_conf) fprintf(fff, "You are resistant to confusion.\n");
	if (p_ptr->resist_sound) fprintf(fff, "You are resistant to sonic attacks.\n");
	if (p_ptr->resist_disen) fprintf(fff, "You are resistant to disenchantment.\n");
	if (p_ptr->resist_chaos) fprintf(fff, "You are resistant to chaos.\n");
	if (p_ptr->resist_shard) fprintf(fff, "You are resistant to blasts of shards.\n");
	if (p_ptr->resist_nexus) fprintf(fff, "You are resistant to nexus attacks.\n");
	if (p_ptr->immune_neth) fprintf(fff, "You are immune to nether forces.\n");
	else if (p_ptr->resist_neth) fprintf(fff, "You are resistant to nether forces.\n");
	if (p_ptr->resist_fear) fprintf(fff, "You are completely fearless.\n");
	if (p_ptr->resist_blind) fprintf(fff, "Your eyes are resistant to blindness.\n");

	if (p_ptr->sustain_str) fprintf(fff, "Your strength is sustained.\n");
	if (p_ptr->sustain_int) fprintf(fff, "Your intelligence is sustained.\n");
	if (p_ptr->sustain_wis) fprintf(fff, "Your wisdom is sustained.\n");
	if (p_ptr->sustain_dex) fprintf(fff, "Your dexterity is sustained.\n");
	if (p_ptr->sustain_con) fprintf(fff, "Your constitution is sustained.\n");
	if (p_ptr->sustain_chr) fprintf(fff, "Your charisma is sustained.\n");
	if (p_ptr->black_breath || p_ptr->black_breath_tmp)
		fprintf(fff, "You suffer from Black Breath.\n");

	if (f1 & TR1_STR) fprintf(fff, "Your strength is affected by your equipment.\n");
	if (f1 & TR1_INT) fprintf(fff, "Your intelligence is affected by your equipment.\n");
	if (f1 & TR1_WIS) fprintf(fff, "Your wisdom is affected by your equipment.\n");
	if (f1 & TR1_DEX) fprintf(fff, "Your dexterity is affected by your equipment.\n");
	if (f1 & TR1_CON) fprintf(fff, "Your constitution is affected by your equipment.\n");
	if (f1 & TR1_CHR) fprintf(fff, "Your charisma is affected by your equipment.\n");

	if (f1 & TR1_STEALTH) fprintf(fff, "Your stealth is affected by your equipment.\n");
	if (f1 & TR1_SEARCH) fprintf(fff, "Your searching ability is affected by your equipment.\n");
	if (f5 & TR5_DISARM) fprintf(fff, "Your disarming ability is affected by your equipment.\n");
	if (f1 & TR1_INFRA) fprintf(fff, "Your infravision is affected by your equipment.\n");
	if (f1 & TR1_TUNNEL) fprintf(fff, "Your digging ability is affected by your equipment.\n");
	if (f1 & TR1_SPEED) fprintf(fff, "Your speed is affected by your equipment.\n");
	if (f1 & TR1_BLOWS) fprintf(fff, "Your melee attack speed is affected by your equipment.\n");
	if (f5 & TR5_CRIT) fprintf(fff, "Your ability to score critical hits is affected by your equipment.\n");

	if (p_ptr->luck == 40)
		fprintf(fff, "You are ultimatively lucky!\n");
	else if (p_ptr->luck >= 30)
		fprintf(fff, "You are lucky very frequently!\n");
	else if (p_ptr->luck >= 20)
		fprintf(fff, "You are lucky often.\n");
	else if (p_ptr->luck >= 10)
		fprintf(fff, "You are lucky here and there.\n");
	else if (p_ptr->luck > 0)
		fprintf(fff, "You are lucky sometimes.\n");

	/* Analyze the weapon */
	if (p_ptr->inventory[INVEN_WIELD].k_idx ||
	    (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) /* dual-wield */
	{
		/* Indicate Blessing */
		if (f3 & TR3_BLESSED) fprintf(fff, "Your weapon has been blessed by the gods.\n");
		if (f5 & TR5_CHAOTIC) fprintf(fff, "Your weapon is branded with the Sign of Chaos.\n");
		/* Hack */
		if (f5 & TR5_IMPACT) fprintf(fff, "The impact of your weapon can cause earthquakes.\n");
		if (f5 & TR5_VORPAL) fprintf(fff, "Your weapon is very sharp.\n");
		if (f1 & TR1_VAMPIRIC) fprintf(fff, "Your weapon drains life from your foes.\n");

		/* Special "Attack Bonuses" */
		if (f1 & TR1_BRAND_ACID) fprintf(fff, "Your weapon melts your foes.\n");
		if (f1 & TR1_BRAND_ELEC) fprintf(fff, "Your weapon shocks your foes.\n");
		if (f1 & TR1_BRAND_FIRE) fprintf(fff, "Your weapon burns your foes.\n");
		if (f1 & TR1_BRAND_COLD) fprintf(fff, "Your weapon freezes your foes.\n");
		if (f1 & TR1_BRAND_POIS) fprintf(fff, "Your weapon poisons your foes.\n");

		/* Special "slay" flags */
		if (f1 & TR1_SLAY_ORC) fprintf(fff, "Your weapon is especially deadly against orcs.\n");
		if (f1 & TR1_SLAY_TROLL) fprintf(fff, "Your weapon is especially deadly against trolls.\n");
		if (f1 & TR1_SLAY_GIANT) fprintf(fff, "Your weapon is especially deadly against giants.\n");
		if (f1 & TR1_SLAY_ANIMAL) fprintf(fff, "Your weapon strikes at animals with extra force.\n");
		if (f1 & TR1_KILL_UNDEAD) fprintf(fff, "Your weapon is a great bane of undead.\n");
		else if (f1 & TR1_SLAY_UNDEAD) fprintf(fff, "Your weapon strikes at undead with holy wrath.\n");
		if (f1 & TR1_KILL_DEMON) fprintf(fff, "Your weapon is a great bane of demons.\n");
		else if (f1 & TR1_SLAY_DEMON) fprintf(fff, "Your weapon strikes at demons with holy wrath.\n");
		if (f1 & TR1_KILL_DRAGON) fprintf(fff, "Your weapon is a great bane of dragons.\n");
		else if (f1 & TR1_SLAY_DRAGON) fprintf(fff, "Your weapon is especially deadly against dragons.\n");
		if (f1 & TR1_SLAY_EVIL) fprintf(fff, "Your weapon strikes at evil with extra force.\n");
	}
//	info[i] = NULL;

	/* Close the file */
	my_fclose(fff);

	/* Let the client know to expect some info */
	strcpy(p_ptr->cur_file_title, "Knowledge of Yourself");
	Send_special_other(Ind);
}

/*
 * Forget everything
 */
bool lose_all_info(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i;
	char note2[ONAME_LEN], noteid[10];

	if (safe_area(Ind)) return TRUE;
	if (p_ptr->auto_id) return FALSE;

	/* Forget info about objects */
	for (i = 0; i < INVEN_TOTAL; i++) {
		object_type *o_ptr = &p_ptr->inventory[i];

		/* Skip non-items */
		if (!o_ptr->k_idx) continue;

		/* Allow "protection" by the MENTAL flag */
		if (o_ptr->ident & ID_MENTAL) continue;

		/* Remove "default inscriptions" */
		if (o_ptr->note && (o_ptr->ident & ID_SENSE)) {
#if 0
			/* Access the inscription */
			cptr q = quark_str(o_ptr->note);

			/* Hack -- Remove auto-inscriptions */
			if ((streq(q, "cursed")) ||
			    //"uncursed"?
			    (streq(q, "broken")) ||
			    (streq(q, "good")) ||
			    (streq(q, "average")) ||
			    (streq(q, "excellent")) ||
			    (streq(q, "worthless")) ||
			    (streq(q, "special")) ||
			    (streq(q, "terrible"))) {
				/* Forget the inscription */
				o_ptr->note = 0;
			}
#else
			note_crop_pseudoid(note2, noteid, quark_str(o_ptr->note));
			if (!note2[0]) o_ptr->note = o_ptr->note_utag = 0;//utag is paranoia
			else o_ptr->note = quark_add(note2);
#endif
		}

		/* Hack -- Clear the "empty" flag */
		o_ptr->ident &= ~ID_EMPTY;
		note_toggle_empty(o_ptr, FALSE);

		/* Hack -- Clear the "known" flag */
		o_ptr->ident &= ~ID_KNOWN;

		/* Hack -- Clear the "felt" flag */
		o_ptr->ident &= ~(ID_SENSE | ID_SENSE_HEAVY);
	}

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Mega-Hack -- Forget the map */
	wiz_dark(Ind);

	/* It worked */
	return (TRUE);
}

/*
 * Detect any treasure on the current panel		-RAK-
 *
 * We do not yet create any "hidden gold" features XXX XXX XXX
 */
/*
 * TODO:
 * 1. allow to display gold carried by monsters
 * 2. make this a ranged spell
 */
bool detect_treasure(int Ind, int rad) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	dun_level *l_ptr;
	int		py = p_ptr->py, px = p_ptr->px;

	int		y, x;
	bool	detect = FALSE;

	cave_type	*c_ptr;
	byte		*w_ptr;
	object_type	*o_ptr;
	cave_type **zcave;

	/* anti-exploit */
	if (!local_panel(Ind)) return FALSE;

	if (!(zcave = getcave(wpos))) return(FALSE);
	l_ptr = getfloor(wpos);

	if (l_ptr && (l_ptr->flags2 & LF2_NO_DETECT)) return FALSE;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_DETECT)) return FALSE;

	/* Scan the current panel */
//	for (y = p_ptr->panel_row_min; y <= p_ptr->panel_row_max; y++)
	for (y = py - rad; y <= py + rad; y++) {
//		for (x = p_ptr->panel_col_min; x <= p_ptr->panel_col_max; x++)
		for (x = px - rad; x <= px + rad; x++) {
			/* Reject locations outside of dungeon */
			if (!in_bounds4(l_ptr, y, x)) continue;

			/* Reject those out of radius */
			if (distance(py, px, y, x) > rad) continue;

			c_ptr = &zcave[y][x];
			w_ptr = &p_ptr->cave_flag[y][x];

			o_ptr = &o_list[c_ptr->o_idx];

			/* Magma/Quartz + Known Gold */
			if ((c_ptr->feat == FEAT_MAGMA_K) ||
			    (c_ptr->feat == FEAT_QUARTZ_K) ||
			    (c_ptr->feat == FEAT_SANDWALL_K))
			{
				/* Notice detected gold */
				if (!(*w_ptr & CAVE_MARK)) {
					/* Detect */
					detect = TRUE;

					/* Hack -- memorize the feature */
					*w_ptr |= CAVE_MARK;

					/* Redraw */
					lite_spot(Ind, y, x);
				}
			}

			/* Notice embedded gold */
			if ((c_ptr->feat == FEAT_MAGMA_H) ||
			    (c_ptr->feat == FEAT_QUARTZ_H) ||
			    (c_ptr->feat == FEAT_SANDWALL_H))
			{
				/* Expose the gold */
				c_ptr->feat += 0x02;

				/* Detect */
				detect = TRUE;

				/* Hack -- memorize the item */
				*w_ptr |= CAVE_MARK;

				/* Redraw */
				lite_spot(Ind, y, x);
			}

			/* Notice gold */
			if (o_ptr->tval == TV_GOLD) {
				/* Notice new items */
				if (!(p_ptr->obj_vis[c_ptr->o_idx])) {
					/* Detect */
					detect = TRUE;

					/* Hack -- memorize the item */
					p_ptr->obj_vis[c_ptr->o_idx] = TRUE;

					/* Redraw */
					lite_spot(Ind, y, x);
				}
			}
		}
	}

	return (detect);
}

/* detect treasures level-wide, for DIGGING skill */
bool floor_detect_treasure(int Ind) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	dun_level *l_ptr;
	int y, x;
	bool detect = FALSE;
	cave_type *c_ptr;
	byte *w_ptr;
	object_type *o_ptr;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return(FALSE);
	if (!(l_ptr = getfloor(wpos))) return(FALSE); /* doesn't work on surface levels (wpos.wz == 0) */

	if (l_ptr && (l_ptr->flags2 & LF2_NO_DETECT)) return FALSE;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_DETECT)) return FALSE;

	/* Scan the whole level */
	for (y = 0; y < l_ptr->hgt; y++) {
		for (x = 0; x < l_ptr->wid; x++) {
			/* Reject locations outside of dungeon */
			if (!in_bounds4(l_ptr, y, x)) continue;

			c_ptr = &zcave[y][x];
			w_ptr = &p_ptr->cave_flag[y][x];
			o_ptr = &o_list[c_ptr->o_idx];

			/* Magma/Quartz + Known Gold */
			if ((c_ptr->feat == FEAT_MAGMA_K) ||
			    (c_ptr->feat == FEAT_QUARTZ_K) ||
			    (c_ptr->feat == FEAT_SANDWALL_K)) {
				/* Notice detected gold */
				if (!(*w_ptr & CAVE_MARK)) {
					/* Detect */
					detect = TRUE;
					/* Hack -- memorize the feature */
					*w_ptr |= CAVE_MARK;
					/* Redraw */
					lite_spot(Ind, y, x);
				}
			}

			/* Notice embedded gold */
			if ((c_ptr->feat == FEAT_MAGMA_H) ||
			    (c_ptr->feat == FEAT_QUARTZ_H) ||
			    (c_ptr->feat == FEAT_SANDWALL_H)) {
				/* Expose the gold */
				c_ptr->feat += 0x02;
				/* Detect */
				detect = TRUE;
				/* Hack -- memorize the item */
				*w_ptr |= CAVE_MARK;
				/* Redraw */
				lite_spot(Ind, y, x);
			}

			/* Notice gold */
			if (o_ptr->tval == TV_GOLD) {
				/* Notice new items */
				if (!(p_ptr->obj_vis[c_ptr->o_idx])) {
					/* Detect */
					detect = TRUE;
					/* Hack -- memorize the item */
					p_ptr->obj_vis[c_ptr->o_idx] = TRUE;
					/* Redraw */
					lite_spot(Ind, y, x);
				}
			}
		}
	}
	return (detect);
}

/*
 * Detect magic items.
 *
 * This will light up all spaces with "magic" items, including artifacts,
 * ego-items, potions, scrolls, books, rods, wands, staves, amulets, rings,
 * and "enchanted" items of the "good" variety.
 *
 * It can probably be argued that this function is now too powerful.
 */
bool detect_magic(int Ind, int rad) {
	player_type *p_ptr = Players[Ind];

	struct worldpos *wpos = &p_ptr->wpos;
	dun_level *l_ptr;
//	int py = p_ptr->py, px = p_ptr->px;

	int	i, j, tv;
	bool	detect = FALSE;

	cave_type	*c_ptr;
	object_type	*o_ptr;

	cave_type **zcave;

	/* anti-exploit */
	if (!local_panel(Ind)) return FALSE;

	if (!(zcave = getcave(wpos))) return(FALSE);
	l_ptr = getfloor(wpos);

	if (l_ptr && (l_ptr->flags2 & LF2_NO_DETECT)) return FALSE;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_DETECT)) return FALSE;

	/* Scan the current panel */
	//for (i = p_ptr->panel_row_min; i <= p_ptr->panel_row_max; i++)
	for (i = p_ptr->py - rad; i <= p_ptr->py + rad; i++) {
		//for (j = p_ptr->panel_col_min; j <= p_ptr->panel_col_max; j++)
		for (j = p_ptr->px - rad; j <= p_ptr->px + rad; j++) {
			/* Reject locations outside of dungeon */
			if (!in_bounds4(l_ptr, i, j)) continue;

			/* Reject those out of radius */
			if (distance(p_ptr->py, p_ptr->px, i, j) > rad) continue;

			/* Access the grid and object */
			c_ptr = &zcave[i][j];
			o_ptr = &o_list[c_ptr->o_idx];

			/* Nothing there */
			if (!(c_ptr->o_idx)) continue;

			/* Examine the tval */
			tv = o_ptr->tval;

			/* Artifacts, misc magic items, or enchanted wearables */
			if (artifact_p(o_ptr) || ego_item_p(o_ptr) ||
			    (tv == TV_AMULET) || (tv == TV_RING) ||
			    is_magic_device(tv) ||
			    (tv == TV_SCROLL) || (tv == TV_POTION) ||
			    ((o_ptr->to_a > 0) || (o_ptr->to_h + o_ptr->to_d > 0)))
			{
				/* Note new items */
				if (!(p_ptr->obj_vis[c_ptr->o_idx]))
				{
					/* Detect */
					detect = TRUE;

					/* Memorize the item */
					p_ptr->obj_vis[c_ptr->o_idx] = TRUE;

					/* Redraw */
					lite_spot(Ind, i, j);
				}
			}
		}
	}

	/* Return result */
	return (detect);
}

/*
 * A "generic" detect monsters routine, tagged to flags3
 * NOTE: match_flag must be 0x0 or exactly one RF3_ flag or (hack) 0x3 for RF1_UNIQUE.
 */
//bool detect_creatures_xxx(u32b match_flag, int rad)
bool detect_creatures_xxx(int Ind, u32b match_flag) {
	player_type *p_ptr = Players[Ind];
	int  i, y, x;
	bool flag = FALSE;
	cptr desc_monsters = "weird monsters";

	dun_level *l_ptr = getfloor(&p_ptr->wpos);

	/* anti-exploit */
	if (!local_panel(Ind)) return FALSE;

	if (l_ptr && (l_ptr->flags2 & LF2_NO_DETECT)) return FALSE;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_DETECT)) return FALSE;

	/* Clear previously detected stuff */
	clear_ovl(Ind);

	/* Scan monsters */
	for (i = 1; i < m_max; i++) {
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = race_inf(m_ptr);

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip visible monsters */
		if (p_ptr->mon_vis[i]) continue;

		/* Skip monsters not on this depth */
		if (!inarea(&m_ptr->wpos, &p_ptr->wpos)) continue;

		/* Location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		// if (m_ptr->cdis > rad) continue;

		/* Detect evil monsters */
		if (!panel_contains(y, x)) continue;

		/* Detect evil monsters */
		if (!match_flag || /* hack: all */
		    (match_flag == 0x3 && (r_ptr->flags1 & RF1_UNIQUE)) || /* hack: uniques */
		    (match_flag != 0x3 && (r_ptr->flags3 & (match_flag)))) {
			byte a;
			char c;

			/* Hack - Temporarily visible */
			p_ptr->mon_vis[i] = TRUE;

			/* Get the look of the monster */
			map_info(Ind, y, x, &a, &c);

			/* No longer visible */
			p_ptr->mon_vis[i] = FALSE;

			/* Draw the monster on the screen */
			draw_spot_ovl(Ind, y, x, a, c);

			flag = TRUE;
		}
	}

	/* Scan players */
	for (i = 1; i <= NumPlayers; i++) {
		player_type *q_ptr = Players[i];

		int py = q_ptr->py;
		int px = q_ptr->px;

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;

		/* Skip ourself */
		if (i == Ind) continue;

		/* match flag */
		switch (match_flag) {
		case RF3_EVIL:
			if (!q_ptr->suscep_good) continue;
			break;
		case RF3_GOOD:
			if (!q_ptr->suscep_evil) continue;
			break;
		case RF3_DEMON:
			if (q_ptr->ptrait != TRAIT_CORRUPTED && !(q_ptr->body_monster && (r_info[q_ptr->body_monster].flags3 & RF3_DEMON))) continue;
			break;
		case RF3_UNDEAD:
			if (!q_ptr->suscep_life) continue;
			break;
		case RF3_ORC:
			if (q_ptr->prace != RACE_HALF_ORC && !(q_ptr->body_monster && (r_info[q_ptr->body_monster].flags3 & RF3_ORC))) continue;
			break;
		case RF3_DRAGON:
			if (q_ptr->prace != RACE_DRACONIAN && !(q_ptr->body_monster && (r_info[q_ptr->body_monster].flags3 & (RF3_DRAGON |  RF3_DRAGONRIDER)))) continue;
			break;
		case RF3_ANIMAL:
			if (q_ptr->prace != RACE_YEEK && !(q_ptr->body_monster && (r_info[q_ptr->body_monster].flags3 & RF3_ANIMAL))) continue;
			break;
		case 0x3:
			/* fun stuff (2): every player is unique (aw) */
			break;
		/* TODO: ...you know :) */
		default: //allow 'all'
			break;
		}

		/* Skip players not on this depth */
		if (!inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;

		/* Never detect the dungeon master! */
		if (q_ptr->admin_dm && !player_sees_dm(Ind)) continue;

		/* Skip visible players */
		if (p_ptr->play_vis[i]) continue;

		/* Detect all xxx players */
		if (panel_contains(py, px)) {
			byte a;
			char c;

			/* Hack - Temporarily visible */
			p_ptr->play_vis[i] = TRUE;

			/* Get the look of the player */
			map_info(Ind, py, px, &a, &c);

			/* No longer visible */
			p_ptr->play_vis[i] = FALSE;

			/* Draw the player on the screen */
			draw_spot_ovl(Ind, py, px, a, c);

			flag = TRUE;
		}
	}

	switch (match_flag) {
	case RF3_EVIL:
		desc_monsters = "evil";
		break;
	case RF3_GOOD:
		desc_monsters = "good";
		break;
	case RF3_DEMON:
		desc_monsters = "demons";
		break;
	case RF3_UNDEAD:
		desc_monsters = "the undead";
		break;
	case RF3_ORC:
		desc_monsters = "orcs";
		break;
	case RF3_DRAGON:
		desc_monsters = "dragonkind";
		break;
	case RF3_ANIMAL:
		desc_monsters = "animals";
		break;
	case 0x3: //hack
		desc_monsters = "unique creatures";
		break;
	/* TODO: ...you know :) */
	default: //allow 'all'
		desc_monsters = "creatures";
		break;
	}

	/* Describe */
	if (flag) {
		/* Describe result */
		msg_format(Ind, "You sense the presence of %s!", desc_monsters);
		//msg_print(NULL);

#if 0 /* this is #if 0'd to produce old behaviour w/o the pause - mikaelh */
		/* Wait */
		Send_pause(Ind);

		/* Mega-Hack -- Fix the monsters */
		update_monsters(FALSE);
#endif
	} else {
#ifdef DETECT_ABSENCE
		/* Describe result */
		msg_format(Ind, "You sense the absence of %s.", desc_monsters);
#endif
	}

	/* Result */
	return (flag);
}
bool detect_evil(int Ind) {
	return(detect_creatures_xxx(Ind, RF3_EVIL));
}

/*
 * Locates and displays all invisible creatures on current panel -RAK-
 */
bool detect_invisible(int Ind) {
	player_type *p_ptr = Players[Ind];

	int i;
	bool flag = FALSE;

	dun_level *l_ptr = getfloor(&p_ptr->wpos);

	/* anti-exploit */
	if (!local_panel(Ind)) return FALSE;

	if (l_ptr && (l_ptr->flags2 & LF2_NO_DETECT)) return FALSE;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_DETECT)) return FALSE;

	/* Clear previously detected stuff */
	clear_ovl(Ind);

	/* Detect all invisible monsters */
	for (i = 1; i < m_max; i++) {
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = race_inf(m_ptr);

		int fy = m_ptr->fy;
		int fx = m_ptr->fx;

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip visible monsters */
		if (p_ptr->mon_vis[i]) continue;

		/* Skip monsters not on this depth */
		if (!inarea(&m_ptr->wpos, &p_ptr->wpos)) continue;

		/* Detect all invisible monsters */
		if (panel_contains(fy, fx) && (r_ptr->flags2 & RF2_INVISIBLE)) {
			byte a;
			char c;

#ifdef OLD_MONSTER_LORE
			/* Take note that they are invisible */
			r_ptr->r_flags2 |= RF2_INVISIBLE;
#endif

			/* Hack - Temporarily visible */
			p_ptr->mon_vis[i] = TRUE;

			/* Get the look of the monster */
			map_info(Ind, fy, fx, &a, &c);

			/* No longer visible */
			p_ptr->mon_vis[i] = FALSE;

			/* Draw the monster on the screen */
			draw_spot_ovl(Ind, fy, fx, a, c);

			flag = TRUE;
		}
	}

	/* Detect all invisible players */
	for (i = 1; i <= NumPlayers; i++) {
		player_type *q_ptr = Players[i];

		int py = q_ptr->py;
		int px = q_ptr->px;

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;

		/* Skip oneself */
		if (i == Ind) continue;

		/* Skip visible players */
		if (p_ptr->play_vis[i]) continue;

		/* Skip players not on this depth */
		if (!inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;

		/* Skip the dungeon master */
		if (q_ptr->admin_dm && !player_sees_dm(Ind)) continue;

		/* Detect all invisible players */
		if (panel_contains(py, px) && q_ptr->invis)  {
			byte a;
			char c;

			/* Hack - Temporarily visible */
			p_ptr->play_vis[i] = TRUE;

			/* Get the look of the player */
			map_info(Ind, py, px, &a, &c);

			/* No longer visible */
			p_ptr->play_vis[i] = FALSE;

			/* Draw the player on the screen */
			draw_spot_ovl(Ind, py, px, a, c);

			flag = TRUE;
		}
	}

	/* Describe result, and clean up */
	if (flag) {
		/* Describe, and wait for acknowledgement */
		msg_print(Ind, "You sense the presence of invisible creatures!");
		msg_print(Ind, NULL);

#if 0 /* this is #if 0'd to produce old behaviour w/o the pause - mikaelh */
		/* Wait */
		Send_pause(Ind);

		/* Mega-Hack -- Fix the monsters and players */
		update_monsters(FALSE);
		update_players();
#endif
	} else {
#ifdef DETECT_ABSENCE
		msg_print(Ind, "You sense the absence of invisible creatures.");
		msg_print(Ind, NULL);
#endif
	}

	/* Result */
	return (flag);
}

/*
 * Display all non-invisible monsters/players on the current panel
 */
bool detect_creatures(int Ind) {
	player_type *p_ptr = Players[Ind];

	int	i;
	bool	flag = FALSE;

	dun_level *l_ptr = getfloor(&p_ptr->wpos);

	/* anti-exploit */
	if (!local_panel(Ind)) return FALSE;

	if (l_ptr && (l_ptr->flags2 & LF2_NO_DETECT)) return FALSE;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_DETECT)) return FALSE;

	/* Clear previously detected stuff */
	clear_ovl(Ind);

	/* Detect non-invisible monsters */
	for (i = 1; i < m_max; i++) {
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = race_inf(m_ptr);

		int fy = m_ptr->fy;
		int fx = m_ptr->fx;

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip visible monsters */
		if (p_ptr->mon_vis[i]) continue;

		/* Skip monsters not on this depth */
		if (!inarea(&m_ptr->wpos, &p_ptr->wpos)) continue;

		/* Detect all non-invisible monsters */
		if (panel_contains(fy, fx) && (!(r_ptr->flags2 & RF2_INVISIBLE))) {
			byte a;
			char c;

			/* Hack - Temporarily visible */
			p_ptr->mon_vis[i] = TRUE;

			/* Get the look of the monster */
			map_info(Ind, fy, fx, &a, &c);

			/* No longer visible */
			p_ptr->mon_vis[i] = FALSE;

			/* Draw the monster on the screen */
			draw_spot_ovl(Ind, fy, fx, a, c);

			flag = TRUE;
		}
	}

	/* Detect non-invisible players */
	for (i = 1; i <= NumPlayers; i++) {
		player_type *q_ptr = Players[i];

		int py = q_ptr->py;
		int px = q_ptr->px;

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;

		/* Skip ourself */
		if (i == Ind) continue;

		/* Skip players not on this depth */
		if (!inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;

		/* Never detect the dungeon master! */
		if (q_ptr->admin_dm && !player_sees_dm(Ind)) continue;

		/* Skip visible players */
		if (p_ptr->play_vis[i]) continue;

		/* Detect all non-invisible players */
		if (panel_contains(py, px) && !q_ptr->invis) {
			byte a;
			char c;

			/* Hack - Temporarily visible */
			p_ptr->play_vis[i] = TRUE;

			/* Get the look of the player */
			map_info(Ind, py, px, &a, &c);

			/* No longer visible */
			p_ptr->play_vis[i] = FALSE;

			/* Draw the player on the screen */
			draw_spot_ovl(Ind, py, px, a, c);

			flag = TRUE;
		}
	}

	/* Describe and clean up */
	if (flag) {
		/* Describe, and wait for acknowledgement */
		msg_print(Ind, "You sense the presence of creatures!");
		msg_print(Ind, NULL);

#if 0 /* this is #if 0'd to produce old behaviour w/o the pause - mikaelh */
		/* Wait */
		Send_pause(Ind);

		/* Mega-Hack -- Fix the monsters and players */
		update_monsters(FALSE);
		update_players();
#endif
	} else {
#ifdef DETECT_ABSENCE
		msg_print(Ind, "You sense the absence of creatures.");
		msg_print(Ind, NULL);
#endif
	}

	/* Result */
	return (flag);
}

/* New Rogue technique */
bool detect_noise(int Ind) {
	player_type *p_ptr = Players[Ind];
	int	i;
	bool	flag = FALSE;
	dun_level *l_ptr = getfloor(&p_ptr->wpos);

	/* anti-exploit */
	if (!local_panel(Ind)) return FALSE;

	if (l_ptr && (l_ptr->flags2 & LF2_NO_DETECT)) return FALSE;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_DETECT)) return FALSE;

	/* Clear previously detected stuff */
	clear_ovl(Ind);

	/* Detect non-invisible monsters */
	for (i = 1; i < m_max; i++) {
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = race_inf(m_ptr);
		int fy = m_ptr->fy;
		int fx = m_ptr->fx;

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;
		/* Skip visible monsters */
		if (p_ptr->mon_vis[i]) continue;
		/* Skip monsters not on this depth */
		if (!inarea(&m_ptr->wpos, &p_ptr->wpos)) continue;

		/* Specialties for noise-detection: don't detect monsters in noiseless state */
		if (m_ptr->csleep && r_ptr->d_char != 'E' && //elementals always give off some sort of noise ;)
		    ((r_ptr->flags3 & (RF3_NONLIVING | RF3_UNDEAD)) //RF3_DEMON debatable, maybe demon's don't really sleep at all?
		    || (r_ptr->flags7 & RF7_SPIDER)
		    || r_ptr->d_char == 'A')) continue;
		if (m_ptr->mspeed <= m_ptr->speed) { //aggravated (well, or hasted in this case) creatures are never stealthy
			if (r_ptr->d_char == 'f') continue; //felines
			if (r_ptr->d_char == 'p' && r_ptr->d_attr == TERM_BLUE && r_ptr->level >= 23) continue; //master rogues
			if (m_ptr->r_idx == 485 || m_ptr->r_idx == 564) continue; //ninja, nightblade
		}

		/* Detect all non-invisible monsters */
		if (panel_contains(fy, fx) && (!(r_ptr->flags2 & RF2_INVISIBLE))) {
			byte a;
			char c;

			/* Hack - Temporarily visible */
			p_ptr->mon_vis[i] = TRUE;
			/* Get the look of the monster */
			map_info(Ind, fy, fx, &a, &c);
			/* No longer visible */
			p_ptr->mon_vis[i] = FALSE;
			/* Draw the monster on the screen */
			draw_spot_ovl(Ind, fy, fx, a, c);

			flag = TRUE;
		}
	}

	/* Detect non-invisible players */
	for (i = 1; i <= NumPlayers; i++) {
		player_type *q_ptr = Players[i];
		int py = q_ptr->py;
		int px = q_ptr->px;

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;
		/* Skip ourself */
		if (i == Ind) continue;
		/* Skip players not on this depth */
		if (!inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;
		/* Never detect the dungeon master! */
		if (q_ptr->admin_dm && !player_sees_dm(Ind)) continue;
		/* Skip visible players */
		if (p_ptr->play_vis[i]) continue;

		/* Specialties for noise-detection */
		if (p_ptr->skill_stl >= 14 /*30*/) continue; //don't detect Heroic/Legendary stealth

		/* Detect all non-invisible players */
		if (panel_contains(py, px) && !q_ptr->invis) {
			byte a;
			char c;

			/* Hack - Temporarily visible */
			p_ptr->play_vis[i] = TRUE;
			/* Get the look of the player */
			map_info(Ind, py, px, &a, &c);
			/* No longer visible */
			p_ptr->play_vis[i] = FALSE;
			/* Draw the player on the screen */
			draw_spot_ovl(Ind, py, px, a, c);

			flag = TRUE;
		}
	}

	/* Describe and clean up */
	if (flag) {
		/* Describe, and wait for acknowledgement */
		msg_print(Ind, "You perceive some revealing sounds!");
		msg_print(Ind, NULL);

#if 0 /* this is #if 0'd to produce old behaviour w/o the pause - mikaelh */
		/* Wait */
		Send_pause(Ind);

		/* Mega-Hack -- Fix the monsters and players */
		update_monsters(FALSE);
		update_players();
#endif
	} else {
//#ifdef DETECT_ABSENCE
		msg_print(Ind, "You cannot hear anything revealing.");
		msg_print(Ind, NULL);
//#endif
	}

	/* Result */
	return (flag);
}

/*
 * Detect everything
 */
bool detection(int Ind, int rad) {
	bool	detect = FALSE;

	/* Detect the easy things */
	if (detect_treasure(Ind, rad)) detect = TRUE;
	if (detect_object(Ind, rad)) detect = TRUE;
	if (detect_trap(Ind, rad)) detect = TRUE;
	if (detect_sdoor(Ind, rad)) detect = TRUE;
	if (detect_creatures(Ind)) detect = TRUE;	/* not radius-ed for now */

	/* Result */
	return (detect);
}

#if 1
/*
 * Detect bounty, a rogue's skill
 */
bool detect_bounty(int Ind, int rad) {
	player_type *p_ptr = Players[Ind];

	// 10 ... 60 % of auto-detecting "stuff"
	int chance = (p_ptr->lev) + 10;

	struct worldpos *wpos = &p_ptr->wpos;
	dun_level *l_ptr;

	int i, j, t_idx = 0;

	bool	detect = FALSE;
	bool	detect_trap = FALSE;

	cave_type  *c_ptr;
	byte *w_ptr;
	cave_type **zcave;
	struct c_special *cs_ptr;

	object_type	*o_ptr;

	/* anti-exploit */
	if (!local_panel(Ind)) return FALSE;

	if (!(zcave = getcave(wpos))) return(FALSE);

	l_ptr = getfloor(wpos);
	if (l_ptr && (l_ptr->flags2 & LF2_NO_DETECT)) return FALSE;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_DETECT)) return FALSE;


	/* Scan the current panel */
	for (i = p_ptr->py - rad; i <= p_ptr->py + rad; i++) {
		for (j = p_ptr->px - rad; j <= p_ptr->px + rad; j++) {
			/* Reject locations outside of dungeon */
			if (!in_bounds4(l_ptr, i, j)) continue;

			/* Reject those out of radius */
			if (distance(p_ptr->py, p_ptr->px, i, j) > rad) continue;

			/* Access the grid */
			c_ptr = &zcave[i][j];
			w_ptr = &p_ptr->cave_flag[i][j];

			o_ptr = &o_list[c_ptr->o_idx];

			detect_trap = FALSE;

			/* Detect traps on chests */
			if ((c_ptr->o_idx) && (o_ptr->tval == TV_CHEST)
			    && p_ptr->obj_vis[c_ptr->o_idx] && (o_ptr->pval)
			    && !object_known_p(Ind, o_ptr) && magik(chance)) {
				/* Message =-p */
				msg_print(Ind, "You have discovered a trap on the chest!");
				/* Know the trap */
				object_known(o_ptr);
				/* Notice it */
//				disturb(Ind, 0, 0);
				detect = TRUE;
			}

			/* Detect invisible traps */
			if ((cs_ptr = GetCS(c_ptr, CS_TRAPS)) && magik(chance)) {
				t_idx = cs_ptr->sc.trap.t_idx;

				if (!cs_ptr->sc.trap.found) {
					/* Pick a trap */
					pick_trap(wpos, i, j);
				}

				/* Hack -- memorize it */
				*w_ptr |= CAVE_MARK;

				/* Obvious */
				detect = TRUE;
				detect_trap = TRUE;
			}

			/* Detect secret doors */
			if (c_ptr->feat == FEAT_SECRET && magik(chance)) {
				struct c_special *cs_ptr;

				/* Clear mimic feature */
				if ((cs_ptr = GetCS(c_ptr, CS_MIMIC)))
					cs_erase(c_ptr, cs_ptr);

				/* Find the door XXX XXX XXX */
				c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

				/* Memorize the door */
				*w_ptr |= CAVE_MARK;

				/* Obvious */
				detect = TRUE;
			}

			// You feel a gust of air from nearby ...
			if (((c_ptr->feat == FEAT_LESS) || (c_ptr->feat == FEAT_MORE) ||
			    (c_ptr->feat == FEAT_WAY_LESS) || (c_ptr->feat == FEAT_WAY_MORE))
				&& magik(chance)) {

				/* Memorize the stairs */
				*w_ptr |= CAVE_MARK;

				/* Obvious */
				detect = TRUE;
			}

			// You hear a jingle of coins nearby ...
			if (c_ptr->feat == FEAT_SHOP && magik(chance)) {
				/* Memorize the stairs */
				*w_ptr |= CAVE_MARK;

				/* Obvious */
				detect = TRUE;
			}
			if (detect) lite_spot(Ind, i, j);
			if (detect_trap) {
				if (c_ptr->o_idx && !c_ptr->m_idx) {
					byte a = get_trap_color(Ind, t_idx, c_ptr->feat);

					/* Hack - Always show traps under items when detecting - mikaelh */
					draw_spot_ovl(Ind, i, j, a, '^');
				} else {
					/* Normal redraw */
					lite_spot(Ind, i, j);
				}
			}
		}
	}
	return (detect);
}
#endif

/*
 * Detect all objects on the current panel		-RAK-
 */
bool detect_object(int Ind, int rad) {
	player_type *p_ptr = Players[Ind];

	struct worldpos *wpos = &p_ptr->wpos;
	dun_level *l_ptr;
	//int py = p_ptr->py, px = p_ptr->px;

	int	i, j;
	bool	detect = FALSE;

	cave_type	*c_ptr;
	object_type	*o_ptr;
	cave_type **zcave;


	/* anti-exploit */
	if (!local_panel(Ind)) return FALSE;

	if (!(zcave = getcave(wpos))) return(FALSE);
	l_ptr = getfloor(wpos);

	if (l_ptr && (l_ptr->flags2 & LF2_NO_DETECT)) return FALSE;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_DETECT)) return FALSE;

	/* Scan the current panel */
//	for (i = p_ptr->panel_row_min; i <= p_ptr->panel_row_max; i++)
	for (i = p_ptr->py - rad; i <= p_ptr->py + rad; i++) {
//		for (j = p_ptr->panel_col_min; j <= p_ptr->panel_col_max; j++)
		for (j = p_ptr->px - rad; j <= p_ptr->px + rad; j++) {
			/* Reject locations outside of dungeon */
			if (!in_bounds4(l_ptr, i, j)) continue;

			/* Reject those out of radius */
			if (distance(p_ptr->py, p_ptr->px, i, j) > rad) continue;

			c_ptr = &zcave[i][j];

			o_ptr = &o_list[c_ptr->o_idx];

			/* Nothing here */
			if (!(c_ptr->o_idx)) continue;

			/* Do not detect "gold" */
			if (o_ptr->tval == TV_GOLD) continue;

			/* Note new objects */
			if (!(p_ptr->obj_vis[c_ptr->o_idx])) {
				/* Detect */
				detect = TRUE;

				/* Hack -- memorize it */
				p_ptr->obj_vis[c_ptr->o_idx] = TRUE;

				/* Redraw */
				lite_spot(Ind, i, j);
			}
		}
	}

	return (detect);
}


/*
 * Locates and displays traps on current panel
 */
//bool detect_trap(int Ind)
bool detect_trap(int Ind, int rad) {
	player_type *p_ptr = Players[Ind];

	struct worldpos *wpos = &p_ptr->wpos;
	dun_level *l_ptr;
//	int	py = p_ptr->py, px = p_ptr->px;

	int	i, j, t_idx;
	bool	detect = FALSE;

	cave_type  *c_ptr;
	byte *w_ptr;
	cave_type **zcave;
	struct c_special *cs_ptr;

	object_type	*o_ptr;


	/* anti-exploit */
	if (!local_panel(Ind)) return FALSE;

	if (!(zcave = getcave(wpos))) return(FALSE);
	l_ptr = getfloor(wpos);

	if (l_ptr && (l_ptr->flags2 & LF2_NO_DETECT)) return FALSE;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_DETECT)) return FALSE;

	/* Clear previously detected stuff */
	clear_ovl(Ind);

	/* Scan the current panel */
//	for (i = p_ptr->panel_row_min; i <= p_ptr->panel_row_max; i++)
	for (i = p_ptr->py - rad; i <= p_ptr->py + rad; i++) {
//		for (j = p_ptr->panel_col_min; j <= p_ptr->panel_col_max; j++)
		for (j = p_ptr->px - rad; j <= p_ptr->px + rad; j++) {
			/* Reject locations outside of dungeon */
			if (!in_bounds4(l_ptr, i, j)) continue;

			/* Reject those out of radius */
			if (distance(p_ptr->py, p_ptr->px, i, j) > rad) continue;

			/* Access the grid */
			c_ptr = &zcave[i][j];
			w_ptr = &p_ptr->cave_flag[i][j];

			/* Hack - traps on undetected doors cannot be found */
/*			if (c_ptr->feat == FEAT_DOOR_TAIL + 1) continue;	--hmm why not */

			/* Detect traps on chests */
			o_ptr = &o_list[c_ptr->o_idx];
			if ((c_ptr->o_idx) && (o_ptr->tval == TV_CHEST) && (o_ptr->pval) && (!object_known_p(Ind, o_ptr))) {
				object_known(o_ptr);

				/* New trap detected */
				detect = TRUE;
			}

			/* Detect invisible traps */
			//if (c_ptr->feat == FEAT_INVIS)
			if ((cs_ptr = GetCS(c_ptr, CS_TRAPS))) {
				t_idx = cs_ptr->sc.trap.t_idx;

				if (!cs_ptr->sc.trap.found) {
					/* Pick a trap */
					pick_trap(wpos, i, j);

					/* New trap detected */
					detect = TRUE;
				}

				/* Hack -- memorize it */
				*w_ptr |= CAVE_MARK;

				/* Redraw */
				lite_spot(Ind, i, j);

				if (c_ptr->o_idx && !c_ptr->m_idx) {
					byte a = get_trap_color(Ind, t_idx, c_ptr->feat);

					/* Hack - Always show traps under items when detecting - mikaelh */
					draw_spot_ovl(Ind, i, j, a, '^');
				} else {
					/* Normal redraw */
					lite_spot(Ind, i, j);
				}

#if 0
				/* Already seen traps */
				else if (c_ptr->feat >= FEAT_TRAP_HEAD && c_ptr->feat <= FEAT_TRAP_TAIL) {
					/* Memorize it */
					*w_ptr |= CAVE_MARK;

					/* Redraw */
					lite_spot(Ind, i, j);

					/* Obvious */
					detect = TRUE;
				}
#endif
			}
#if 0
			else if ((cs_ptr = GetCS(c_ptr, CS_MON_TRAP))) {
				/* New trap detected */
				//hmm, how to do this? detect = TRUE;

				/* Hack -- memorize it */
				*w_ptr |= CAVE_MARK;

				/* Redraw */
				lite_spot(Ind, i, j);

				/* Normal redraw */
				lite_spot(Ind, i, j);
			}
#endif
		}
	}

	if (detect) {
		msg_print(Ind, "You have detected new traps.");
	} else {
#ifdef DETECT_ABSENCE
		msg_print(Ind, "You have detected no new traps.");
#endif
	}

	return (detect);
}



/*
 * Locates and displays all stairs and secret doors on current panel -RAK-
 */
bool detect_sdoor(int Ind, int rad) {
	player_type *p_ptr = Players[Ind];

	struct worldpos *wpos = &p_ptr->wpos;
	dun_level *l_ptr;
	//int py = p_ptr->py, px = p_ptr->px;

	int		i, j;
	bool	detect = FALSE;

	cave_type *c_ptr;
	byte *w_ptr;
	cave_type **zcave;

	/* anti-exploit */
	if (!local_panel(Ind)) return FALSE;

	if (!(zcave = getcave(wpos))) return(FALSE);
	l_ptr = getfloor(wpos);

	if (l_ptr && (l_ptr->flags2 & LF2_NO_DETECT)) return FALSE;
	if (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_DETECT)) return FALSE;

	/* Scan the panel */
//	for (i = p_ptr->panel_row_min; i <= p_ptr->panel_row_max; i++)
	for (i = p_ptr->py - rad; i <= p_ptr->py + rad; i++) {
//		for (j = p_ptr->panel_col_min; j <= p_ptr->panel_col_max; j++)
		for (j = p_ptr->px - rad; j <= p_ptr->px + rad; j++) {
			/* Reject locations outside of dungeon */
			if (!in_bounds4(l_ptr, i, j)) continue;

			/* Reject those out of radius */
			if (distance(p_ptr->py, p_ptr->px, i, j) > rad) continue;

			/* Access the grid and object */
			c_ptr = &zcave[i][j];
			w_ptr = &p_ptr->cave_flag[i][j];

			/* Hack -- detect secret doors */
			if (c_ptr->feat == FEAT_SECRET) {
				struct c_special *cs_ptr;

				/* Clear mimic feature */
				if ((cs_ptr = GetCS(c_ptr, CS_MIMIC)))
					cs_erase(c_ptr, cs_ptr);

				/* Find the door XXX XXX XXX */
				c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

				/* Memorize the door */
				*w_ptr |= CAVE_MARK;

				/* Redraw */
				lite_spot(Ind, i, j);

				/* Obvious */
				detect = TRUE;
			}

			/* Ignore known grids */
			if (*w_ptr & CAVE_MARK) continue;

			/* Hack -- detect stairs */
			if ((c_ptr->feat == FEAT_LESS) || (c_ptr->feat == FEAT_MORE) ||
			    (c_ptr->feat == FEAT_WAY_LESS) || (c_ptr->feat == FEAT_WAY_MORE))
			{
				/* Memorize the stairs */
				*w_ptr |= CAVE_MARK;

				/* Redraw */
				lite_spot(Ind, i, j);

				/* Obvious */
				detect = TRUE;
			}

			/* Hack -- detect dungeon shops */
			if (c_ptr->feat == FEAT_SHOP) {
				/* Memorize the stairs */
				*w_ptr |= CAVE_MARK;

				/* Redraw */
				lite_spot(Ind, i, j);

				/* Obvious */
				detect = TRUE;
			}
		}
	}

	return (detect);
}


/*
 * Create stairs at the player location
 */
void stair_creation(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Access the grid */
	cave_type *c_ptr;

	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Access the player grid */
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* XXX XXX XXX */
	if (!cave_valid_bold(zcave, p_ptr->py, p_ptr->px)) {
		msg_print(Ind, "The object resists the spell.");
		return;
	}

	/* Hack -- Delete old contents */
	delete_object(wpos, p_ptr->py, p_ptr->px, TRUE);

	/* Create a staircase */
	if (!can_go_down(wpos, 0x1) && !can_go_up(wpos, 0x1)) {
		/* special..? */
	} else if (can_go_down(wpos, 0x1) && !can_go_up(wpos, 0x1)) {
		c_ptr->feat = FEAT_MORE;
	} else if(can_go_up(wpos, 0x1) && !can_go_down(wpos, 0x1)) {
		c_ptr->feat = FEAT_LESS;
	} else if (rand_int(100) < 50) {
		c_ptr->feat = FEAT_MORE;
	} else {
		c_ptr->feat = FEAT_LESS;
	}

	/* Notice */
	note_spot(Ind, p_ptr->py, p_ptr->px);

	/* Redraw */
	everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);
}




/*
 * Hook to specify "weapon"
 */
static bool item_tester_hook_weapon(object_type *o_ptr) {
	switch (o_ptr->tval) {
	case TV_TRAPKIT:/* <- and now new.. :) this allows cursing/enchanting shot/arrow/bolt trap kits! */
		if (!is_firearm_trapkit(o_ptr->sval)) return FALSE;
	case TV_SWORD:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_DIGGING:
	case TV_BOW:
	case TV_BOLT:
	case TV_ARROW:
	case TV_SHOT:
	case TV_MSTAFF:
	case TV_BOOMERANG:
	case TV_AXE:
		return (TRUE);
	}

	return (FALSE);
}


/*
 * Hook to specify "armour"
 */
static bool item_tester_hook_armour(object_type *o_ptr) {
	switch (o_ptr->tval) {
	case TV_DRAG_ARMOR:
	case TV_HARD_ARMOR:
	case TV_SOFT_ARMOR:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_CROWN:
	case TV_HELM:
	case TV_BOOTS:
	case TV_GLOVES:
	/* and now new.. :) */
	//nope, not enchantable -- case TV_TRAPKIT:
		return (TRUE);
	}

	return (FALSE);
}



/*
 * Enchants a plus onto an item.                        -RAK-
 *
 * Revamped!  Now takes item pointer, number of times to try enchanting,
 * and a flag of what to try enchanting.  Artifacts resist enchantment
 * some of the time, and successful enchantment to at least +0 might
 * break a curse on the item.  -CFT
 *
 * Note that an item can technically be enchanted all the way to +15 if
 * you wait a very, very, long time.  Going from +9 to +10 only works
 * about 5% of the time, and from +10 to +11 only about 1% of the time.
 *
 * Note that this function can now be used on "piles" of items, and
 * the larger the pile, the lower the chance of success.
 */
bool enchant(int Ind, object_type *o_ptr, int n, int eflag) {
	player_type *p_ptr = Players[Ind];
	int i, chance, prob;
	bool res = FALSE;
	bool a = like_artifact_p(o_ptr);
	u32b f1, f2, f3, f4, f5, f6, esp;
	bool did_tohit = FALSE, did_todam = FALSE, did_toac = FALSE;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* Unenchantable items always fail */
	if (!is_enchantable(o_ptr)) return FALSE;
	if (f5 & TR5_NO_ENCHANT) return (FALSE);
	/* Artifacts cannot be enchanted. */
	if (a) return (FALSE);

	/* Large piles resist enchantment */
	prob = o_ptr->number * 100;

	/* Missiles are easy to enchant */
	if (is_ammo(o_ptr->tval)) {
		prob = prob / 20;
	}

	/* Try "n" times */
	for (i = 0; i < n; i++) {
		/* Hack -- Roll for pile resistance */
		if (rand_int(prob) >= 100) continue;

		/* Enchant to hit, but not that easily multiple times over 9 */
		if ((eflag & ENCH_TOHIT) && (magik(30) || !(did_tohit && o_ptr->to_h > 9)))
		{
			if (o_ptr->to_h < 0) chance = 0;
			else if (o_ptr->to_h > 14) chance = 1000;
			else {
				chance = enchant_table[o_ptr->to_h];
				/* *Enchant Weapon* scrolls are better! */
				if (n > 1) chance = ((chance * 1) / 3);
			}

			if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
			{
				o_ptr->to_h++;
				res = TRUE;
				did_tohit = TRUE;

				/* only when you get it above -1 -CFT */
				if (cursed_p(o_ptr) &&
				    (!(f3 & TR3_PERMA_CURSE)) &&
//				    (o_ptr->to_h >= 0) && (rand_int(100) < 25))
				    (rand_int(100) < 10 + 10 * o_ptr->to_h))
				{
					msg_print(Ind, "The curse is broken!");
					o_ptr->ident &= ~ID_CURSED;
					o_ptr->ident |= ID_SENSE | ID_SENSED_ONCE;
					note_toggle_cursed(o_ptr, FALSE);
				}
			}
		}

		/* Enchant to damage, but not that easily multiple times over 9 */
		if ((eflag & ENCH_TODAM) && (magik(30) || !(did_todam && o_ptr->to_d > 9))) {
			if (o_ptr->to_d < 0) chance = 0;
			else if (o_ptr->to_d > 14) chance = 1000;
			else {
				chance = enchant_table[o_ptr->to_d];
				/* *Enchant Weapon* scrolls are better! */
				if (n > 1) chance = ((chance * 1) / 3);
			}

			if ((randint(1000) > chance) && (!a || (rand_int(100) < 50))) {
				o_ptr->to_d++;
				res = TRUE;
				did_todam = TRUE;

				/* only when you get it above -1 -CFT */
				if (cursed_p(o_ptr) &&
				    (!(f3 & TR3_PERMA_CURSE)) &&
//				    (o_ptr->to_d >= 0) && (rand_int(100) < 25))
				    (rand_int(100) < 10 + 10 * o_ptr->to_d))
				{
					msg_print(Ind, "The curse is broken!");
					o_ptr->ident &= ~ID_CURSED;
					o_ptr->ident |= ID_SENSE | ID_SENSED_ONCE;
					note_toggle_cursed(o_ptr, FALSE);
				}
			}
		}

		/* Enchant to armor class, but not that easily multiple times over 9 */
		if ((eflag & ENCH_TOAC) && (magik(30) || !(did_toac && o_ptr->to_a > 9))) {
			if (o_ptr->to_a < 0) chance = 0;
			else if (o_ptr->to_a > 14) chance = 1000;
			else {
				chance = enchant_table[o_ptr->to_a];
				/* *Enchant Armour* scrolls are better! */
				if (n > 1) chance = ((chance * 1) / 3);
			}

			if ((randint(1000) > chance) && (!a || (rand_int(100) < 50))) {
				o_ptr->to_a++;
				res = TRUE;
				did_toac = TRUE;

				/* only when you get it above -1 -CFT */
				if (cursed_p(o_ptr) &&
				    (!(f3 & TR3_PERMA_CURSE)) &&
				    (o_ptr->to_a >= 0) && (rand_int(100) < 25)) {
					msg_print(Ind, "The curse is broken!");
					o_ptr->ident &= ~ID_CURSED;
					o_ptr->ident |= ID_SENSE | ID_SENSED_ONCE;
					note_toggle_cursed(o_ptr, FALSE);
				}
			}
		}
	}

	/* Failure */
	if (!res) return (FALSE);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Resend plusses */
	p_ptr->redraw |= (PR_PLUSSES);

	/* Success */
	return (TRUE);
}

bool create_artifact(int Ind, bool nolife) {
	player_type *p_ptr = Players[Ind];

	/* just in case */
	s_printf("(%s) Player %s initiates Artifact Creation (nolife=%d).\n", showtime(), p_ptr->name, nolife);

	clear_current(Ind);

	p_ptr->current_artifact = TRUE;
	p_ptr->current_artifact_nolife = nolife;
	get_item(Ind, ITH_NONE);

	return TRUE;
}

bool create_artifact_aux(int Ind, int item) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	artifact_type *a_ptr;
	int tries = 0;
	char o_name[ONAME_LEN];
	s32b old_owner;/* anti-cheeze :) */
	u32b resf = make_resf(p_ptr);

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &p_ptr->inventory[item];
	/* Get the item (on the floor) */
	else o_ptr = &o_list[0 - item];

	old_owner = o_ptr->owner;

	/* Description */
	object_desc(Ind, o_name, o_ptr, FALSE, 0);
	s_printf("ART_CREATION by player %s: %s\n", p_ptr->name, o_name);

	if (((o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_SHIRT) ||
	    (o_ptr->tval == TV_SPECIAL)) /* <- must be checked here, not in randart_make() due to seals, see randart_make(). */
	     && !is_admin(p_ptr)) {
		msg_print(Ind, "\376\377yThe item appears unchanged!");
		return FALSE;
	}
	if (o_ptr->name1) {
		msg_print(Ind, "\376\377yThe creation fails due to the powerful magic of the target object!");
		return FALSE;
	}
	if (o_ptr->name2 || o_ptr->name2b) {
		msg_print(Ind, "\376\377yThe creation fails due to the strong magic of the target object!");
		return FALSE;
		o_ptr->name2 = 0;
		o_ptr->name2b = 0;
		msg_print(Ind, "The strong magic of that object dissolves!");
	}
	if (o_ptr->number > 1) {
/*		msg_print(Ind, "The creation fails because the magic is split to multiple targets!");
		return FALSE;*/
		o_ptr->number = 1;
		msg_print(Ind, "The stack of objects magically dissolves, leaving only a single item!");
	}

	/* Describe */
	msg_format(Ind, "%s %s glow%s *brightly*!",
	    ((item >= 0) ? "Your" : "The"), o_name,
	    ((o_ptr->number > 1) ? "" : "s"));

	o_ptr->name1 = ART_RANDART;

/* NOTE: MAKE SURE THE FOLLOWING CODE IS CONSISTENT WITH make_artifact() IN object2.c! */

	/* Randart loop. Try until an allowed randart was made */
	while (tries < 10) {
		tries++;

		/* Piece together a 32-bit random seed */
		o_ptr->name3 = (u32b)rand_int(0xFFFF) << 16;
		o_ptr->name3 += rand_int(0xFFFF);

		/* Check the tval is allowed */
		if (randart_make(o_ptr) == NULL) {
			/* If not, wipe seed. No randart today */
			o_ptr->name1 = 0;
			o_ptr->name3 = 0L;

			msg_print(Ind, "The item appears unchanged!");
			return FALSE;
		}

		/* If the resulting randart is allowed, leave the loop */
		a_ptr = randart_make(o_ptr);
		if (((resf & RESF_LIFE) && !p_ptr->current_artifact_nolife) || !(a_ptr->flags1 & TR1_LIFE)) break;
	}

	/* apply magic (which resets owner) and manually restore ownership again afterwards;
	   apply_magic() is used to set level requirements, and copy the a_ptr to o_ptr. */
	apply_magic(&p_ptr->wpos, o_ptr, 50, FALSE, FALSE, FALSE, FALSE, RESF_NONE);
	o_ptr->owner = old_owner;

	/* Hack - lose discount on item, looks bad/silly */
	o_ptr->discount = 0;
	o_ptr->note = 0;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Art creation finished */
	p_ptr->current_artifact = FALSE;
	p_ptr->current_artifact_nolife = FALSE;

	/* Log it (security) */
	/* Description */
	object_desc(Ind, o_name, o_ptr, FALSE, 3);
	s_printf("ART_CREATION succeeded: %s\n", o_name);

	/* Did we use up an item? (minus 1 art scroll) */
	if (p_ptr->using_up_item >= 0) {
		inven_item_describe(Ind, p_ptr->using_up_item);
		inven_item_optimize(Ind, p_ptr->using_up_item);
		p_ptr->using_up_item = -1;
	}

	return TRUE;
}

bool curse_spell(int Ind) {	// could be void
	player_type *p_ptr = Players[Ind];
	clear_current(Ind);
	get_item(Ind, ITH_NONE);
	p_ptr->current_curse = TRUE;	/* This is awful. I intend to change it */
	return TRUE;
}

bool curse_spell_aux(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr = &p_ptr->inventory[item];
	char o_name[ONAME_LEN];

	p_ptr->current_curse = FALSE;
	object_desc(Ind, o_name, o_ptr, FALSE, 0);


	if (artifact_p(o_ptr) && (randint(10) < 8)) {
		msg_print(Ind,"The artifact resists your attempts.");
		return(FALSE);
	}

	if (item_tester_hook_weapon(o_ptr)) {
		o_ptr->to_h = 0 - (randint(10) + 1);
		o_ptr->to_d = 0 - (randint(10) + 1);
	} else if (item_tester_hook_armour(o_ptr)) {
		o_ptr->to_a = 0 - (randint(10) + 1);
	} else {
		switch (o_ptr->tval) {
			case TV_RING:
			default:
				msg_print(Ind,"You cannot curse that item!");
				return(FALSE);
		}
	}

	msg_format(Ind,"\376\377yA terrible black aura surrounds your %s",
	    o_name, o_ptr->number > 1 ? "" : "s");
	/* except it doesnt actually get cursed properly yet. */
	o_ptr->name1 = 0;
	o_ptr->name3 = 0;
	o_ptr->ident |= ID_CURSED;
	o_ptr->ident &= ~(ID_KNOWN | ID_SENSE);	/* without this, the spell is pointless */

	if (o_ptr->name2) o_ptr->pval = 0 - (randint(10) + 1);	/* nasty */

	/* Did we use up an item? */
	if (p_ptr->using_up_item >= 0) {
		inven_item_describe(Ind, p_ptr->using_up_item);
		inven_item_optimize(Ind, p_ptr->using_up_item);
		p_ptr->using_up_item = -1;
	}

	/* Recalculate the bonuses - if stupid enough to curse worn item ;) */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	return(TRUE);
}

bool enchant_spell(int Ind, int num_hit, int num_dam, int num_ac, int flags) {
	player_type *p_ptr = Players[Ind];

#ifndef NEW_SHIELDS_NO_AC
	get_item(Ind, num_ac ? ITH_ENCH_AC : ITH_ENCH_WEAP);
#else
	get_item(Ind, num_ac ? ITH_ENCH_AC_NO_SHIELD : ITH_ENCH_WEAP);
#endif

	/* Clear any other pending actions - mikaelh */
	clear_current(Ind);

	p_ptr->current_enchant_h = num_hit;
	p_ptr->current_enchant_d = num_dam;
	p_ptr->current_enchant_a = num_ac;
	p_ptr->current_enchant_flag = flags;

	return (TRUE);
}

/*
 * Enchant an item (in the inventory or on the floor)
 * Note that "num_ac" requires armour, else weapon
 * Returns TRUE if attempted, FALSE if cancelled
 */
/*
 * //deprecated//For now, 'flags' is the chance of the item getting 'discounted'
 * in the process.
 */
bool enchant_spell_aux(int Ind, int item, int num_hit, int num_dam, int num_ac, int flags) {
	player_type *p_ptr = Players[Ind];
	bool okay = FALSE;
	object_type *o_ptr;
	char o_name[ONAME_LEN];

	/* Assume enchant weapon */
	item_tester_hook = item_tester_hook_weapon;

	/* Enchant armor if requested */
 	if (num_ac) item_tester_hook = item_tester_hook_armour;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &p_ptr->inventory[item];
	/* Get the item (on the floor) */
	else o_ptr = &o_list[0 - item];


	if (!item_tester_hook(o_ptr) || !is_enchantable(o_ptr)) {
		msg_print(Ind, "Sorry, you cannot enchant that item.");
#ifndef NEW_SHIELDS_NO_AC
		get_item(Ind, num_ac ? ITH_ENCH_AC : ITH_ENCH_WEAP);
#else
		get_item(Ind, num_ac ? ITH_ENCH_AC_NO_SHIELD : ITH_ENCH_WEAP);
#endif
		return (FALSE);
	}

	/* Description */
	object_desc(Ind, o_name, o_ptr, FALSE, 0);

	/* Describe */
	msg_format(Ind, "%s %s glow%s brightly!",
	    ((item >= 0) ? "Your" : "The"), o_name,
	    ((o_ptr->number > 1) ? "" : "s"));

	/* Enchant */
	if (enchant(Ind, o_ptr, num_hit, ENCH_TOHIT)) okay = TRUE;
	if (enchant(Ind, o_ptr, num_dam, ENCH_TODAM)) okay = TRUE;
	if (enchant(Ind, o_ptr, num_ac, ENCH_TOAC)) okay = TRUE;

	/* Artifacts cannot be enchanted. */
	//if (artifact_p(o_ptr)) msg_format(Ind,"Your %s %s unaffected.",o_name,((o_ptr->number != 1) ? "are" : "is"));

	/* Failure */
	if (!okay) {
		/* Message */
		msg_print(Ind, "The enchantment failed.");
	}

#if 0
	/* Anti-cheeze */
	if (okay && !artifact_p(o_ptr) && !ego_item_p(o_ptr) && magik(flags)) {
		int discount = (100 - o_ptr->discount) / 2;
		if (discount > 0) {
			o_ptr->discount += discount;

			/* Message */
			msg_format(Ind, "It spoiled the appearence of %s %s somewhat!",
			    ((item >= 0) ? "your" : "the"), o_name);
		}
	}
#else
	/* Anti-cheeze: Prevent stealers from making infinite money in IDDC towns.
	   It doesn't matter that much in other places, since there are usually
	   better/equal ways to make money.
	   For items probably >= 12k it doesn't matter anymore though. */
	if (okay && (flags & ENCH_STOLEN) &&
	    object_value_real(0, o_ptr) < 15000 && o_ptr->discount != 100 &&
	    isdungeontown(&p_ptr->wpos) && in_irondeepdive(&p_ptr->wpos)) {
		msg_format(Ind, "The stolen enchantment scroll spoiled the appearence of %s %s!",
		    ((item >= 0) ? "your" : "the"), o_name);
		o_ptr->discount = 100;
		if (!o_ptr->note) o_ptr->note = quark_add("devalued");
	}
#endif

	/* Did we use up an item? */
	if (p_ptr->using_up_item >= 0) {
		inven_item_describe(Ind, p_ptr->using_up_item);
		inven_item_optimize(Ind, p_ptr->using_up_item);
		p_ptr->using_up_item = -1;
	}

	p_ptr->current_enchant_h = -1;
	p_ptr->current_enchant_d = -1;
	p_ptr->current_enchant_a = -1;
	p_ptr->current_enchant_flag = -1;

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Something happened */
	return (TRUE);
}


bool ident_spell(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* originally special hack for !X on ID spells,
	   but now actually used for everything (scrolls etc) */
	if (p_ptr->current_item != -1) {
		clear_current(Ind);
		ident_spell_aux(Ind, p_ptr->current_item);
		return TRUE;
	}

	get_item(Ind, ITH_NONE);

	/* Clear any other pending actions - mikaelh */
	clear_current(Ind);

	p_ptr->current_identify = 1;

	return TRUE;
}

/*
 * Identify an object in the inventory (or on the floor)
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was identified, else FALSE.
 */
bool ident_spell_aux(int Ind, int item) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	char o_name[ONAME_LEN];

	/* clean up special hack, originally for !X on ID spells
	   but now actually used for everything (scrolls etc) */
	p_ptr->current_item = -1;

	XID_paranoia(p_ptr);

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &p_ptr->inventory[item];
	/* Get the item (on the floor) */
	else o_ptr = &o_list[0 - item];


	/* Identify it fully */
	object_aware(Ind, o_ptr);
	object_known(o_ptr);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* redraw to-hit/to-dam */
	p_ptr->redraw |= (PR_PLUSSES);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Description */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD) {
		msg_format(Ind, "%^s: %s (%c).",
		   describe_use(Ind, item), o_name, index_to_label(item));
	} else if (item >= 0) {
		msg_format(Ind, "In your pack: %s (%c).",
		    o_name, index_to_label(item));
	} else {
		msg_format(Ind, "On the ground: %s.",
		    o_name);
	}

#if 1
	if (!p_ptr->warning_inspect &&
	    (o_ptr->tval == TV_RING || o_ptr->tval == TV_AMULET)// || o_ptr->tval == TV_WAND || o_ptr->tval == TV_STAFF || o_ptr->tval == TV_ROD)
	    //&& *(k_text + k_info[o_ptr->k_idx].text) /* not this, it disables all 'basic' items such as sustain rings or example */
	    ) {
		msg_print(Ind, "\374\377yHINT: You can press '\377oShift+i\377y' to try and inspect an unknown item!");
		s_printf("warning_inspect: (id) %s\n", p_ptr->name);
		p_ptr->warning_inspect = 1;
	}
#endif

	/* Did we use up an item? */
	if (p_ptr->using_up_item >= 0) {
//		inven_item_describe(Ind, p_ptr->using_up_item); /* maybe not when IDing */
		inven_item_optimize(Ind, p_ptr->using_up_item);
		p_ptr->using_up_item = -1;
	}

	p_ptr->current_identify = 0;

	if (item >= 0) p_ptr->inventory[item].auto_insc = TRUE;

	/* Something happened */
	return (TRUE);
}


bool identify_fully(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* originally special hack for !X on ID spells,
	   but now used for *ID* scrolls too. */
	if (p_ptr->current_item != -1) {
		clear_current(Ind);
		identify_fully_item(Ind, p_ptr->current_item);
		return TRUE;
	}

	get_item(Ind, ITH_NONE);

	/* Clear any other pending actions - mikaelh */
	clear_current(Ind);

	p_ptr->current_star_identify = 1;

	return TRUE;
}


/*
 * Fully "identify" an object in the inventory	-BEN-
 * This routine returns TRUE if an item was identified.
 */
bool identify_fully_item(int Ind, int item) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	char o_name[ONAME_LEN];

	/* clean up special hack, originally for !X on ID spells
	   but now actually used for *ID* scrolls too. */
	p_ptr->current_item = -1;

	XID_paranoia(p_ptr);

	/* Get the item (in the pack) */
	if (item >= 0)
		o_ptr = &p_ptr->inventory[item];
	/* Get the item (on the floor) */
	else
		o_ptr = &o_list[0 - item];


	/* Identify it fully */
	object_aware(Ind, o_ptr);
	object_known(o_ptr);

	/* Mark the item as fully known */
	o_ptr->ident |= (ID_MENTAL);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Description */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
		msg_format(Ind, "%^s: %s (%c).",
		           describe_use(Ind, item), o_name, index_to_label(item));
	else if (item >= 0)
		msg_format(Ind, "In your pack: %s (%c).",
		           o_name, index_to_label(item));
	else
		msg_format(Ind, "On the ground: %s.",
		           o_name);

	/* Describe it fully */
	(void)identify_fully_aux(Ind, o_ptr, FALSE, item);

	/* Did we use up an item? */
	if (p_ptr->using_up_item >= 0) {
		//inven_item_describe(Ind, p_ptr->using_up_item); /* maybe not for *ID* */
		inven_item_optimize(Ind, p_ptr->using_up_item);
		p_ptr->using_up_item = -1;

		p_ptr->window |= PW_INVEN;
	}

	/* We no longer have a *identify* in progress */
	p_ptr->current_star_identify = 0;

	/* extra: in case the item wasn't normally identified yet but right away *id*ed, apply this too.. */
	if (item >= 0) p_ptr->inventory[item].auto_insc = TRUE;

	/* Success */
	return (TRUE);
}

/* silent version of identify_fully_item() that doesn't display anything - C. Blue */
//UNUSED
bool identify_fully_item_quiet(int Ind, int item) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &p_ptr->inventory[item];
	/* Get the item (on the floor) */
	else o_ptr = &o_list[0 - item];

	/* Identify it fully */
	object_aware(Ind, o_ptr);
	object_known(o_ptr);

	/* Mark the item as fully known */
	o_ptr->ident |= (ID_MENTAL);
	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);
	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);
	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Did we use up an item? */
	if (p_ptr->using_up_item >= 0) {
		//inven_item_describe(Ind, p_ptr->using_up_item); /* maybe not for *ID* */
		inven_item_optimize(Ind, p_ptr->using_up_item);
		p_ptr->using_up_item = -1;

		p_ptr->window |= PW_INVEN;
	}

	/* Handle stuff */
	handle_stuff(Ind);

	/* We no longer have a *identify* in progress */
	p_ptr->current_star_identify = 0;

	/* Success */
	return (TRUE);
}

/* variant of identify_fully_item_quiet that doesn't use player inventory,
   added for !X on Iron Helm of Knowledge, which probably doesn't make much
   sense though - C. Blue */
//UNUSED
bool identify_fully_object_quiet(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];

	/* Identify it fully */
	object_aware(Ind, o_ptr);
	object_known(o_ptr);

	/* Mark the item as fully known */
	o_ptr->ident |= (ID_MENTAL);

	/* Did we use up an item? */
	if (p_ptr->using_up_item >= 0) {
//		inven_item_describe(Ind, p_ptr->using_up_item); /* maybe not for *ID* */
		inven_item_optimize(Ind, p_ptr->using_up_item);
		p_ptr->using_up_item = -1;

		p_ptr->window |= PW_INVEN;
	}

	/* Handle stuff */
	handle_stuff(Ind);

	/* We no longer have a *identify* in progress */
	p_ptr->current_star_identify = 0;

	/* Success */
	return (TRUE);
}

/*
 * Hook for "get_item()".  Determine if something is rechargable.
 */
//static bool item_tester_hook_recharge(object_type *o_ptr)
bool item_tester_hook_recharge(object_type *o_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* Some objects cannot be recharged */
	if (f4 & TR4_NO_RECHARGE) return (FALSE);

	/* Recharge staffs/wands/rods */
	if (is_magic_device(o_ptr->tval)) return (TRUE);

	/* Nope */
	return (FALSE);
}


bool recharge(int Ind, int num) {
	player_type *p_ptr = Players[Ind];

	get_item(Ind, ITH_RECHARGE);

	/* Clear any other pending actions - mikaelh */
	clear_current(Ind);

	p_ptr->current_recharge = num;

	return TRUE;
}


/*
 * Recharge a wand/staff/rod from the pack or on the floor.
 *
 * Mage -- Recharge I --> recharge(5)
 * Mage -- Recharge II --> recharge(40)
 * Mage -- Recharge III --> recharge(100)
 *
 * Priest -- Recharge --> recharge(15)
 *
 * Scroll of recharging --> recharge(60)
 *
 * recharge(20) = 1/6 failure for empty 10th level wand
 * recharge(60) = 1/10 failure for empty 10th level wand
 *
 * It is harder to recharge high level, and highly charged wands.
 *
 * XXX XXX XXX Beware of "sliding index errors".
 *
 * Should probably not "destroy" over-charged items, unless we
 * "replace" them by, say, a broken stick or some such.  The only
 * reason this is okay is because "scrolls of recharging" appear
 * BEFORE all staffs/wands/rods in the inventory.  Note that the
 * new "auto_sort_pack" option would correctly handle replacing
 * the "broken" wand with any other item (i.e. a broken stick).
 *
 * XXX XXX XXX Perhaps we should auto-unstack recharging stacks.
 */
bool recharge_aux(int Ind, int item, int pow) {
	player_type *p_ptr = Players[Ind];
	int i, t, lev, dr;
	object_type *o_ptr;


	/* Only accept legal items */
	item_tester_hook = item_tester_hook_recharge;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &p_ptr->inventory[item];
	/* Get the item (on the floor) */
	else o_ptr = &o_list[0 - item];

	if (!item_tester_hook(o_ptr)) {
		msg_print(Ind, "You cannot recharge that item.");
		get_item(Ind, ITH_RECHARGE);
		return (FALSE);
	}

	/* Extract the object "level" */
	lev = k_info[o_ptr->k_idx].level;

	/* Recharge a rod */
	if (o_ptr->tval == TV_ROD) {
		if (pow <= 60) { /* not possible with scrolls of recharging */
			msg_print(Ind, "The flow of mana seems not powerful enough to recharge a rod.");
			return TRUE;
		}

		/* Extract a recharge power */
		i = (100 - lev + pow) / 5;

		/* Paranoia -- prevent crashes */
		if (i < 1) i = 1;

		/* Back-fire */
		if (rand_int(i) == 0) {
			/* Hack -- backfire */
			//msg_print(Ind, "The recharge backfires, draining the rod further!");
			msg_print(Ind, "There is a static discharge.");

			/* Hack -- decharge the rod */
			if (o_ptr->pval < 550) discharge_rod(o_ptr, 550 + rand_int(101));
		}

		/* Recharge */
		else {
			/* Rechange amount */
			t = (pow * damroll(2, 4));

			/* Recharge by that amount */
			if (o_ptr->pval > t) o_ptr->pval -= t;
			/* Fully recharged */
			else o_ptr->pval = 0;
		}
	}

	/* Recharge wand/staff */
	else {
		/* Recharge power */
		i = (pow + 100 - lev - (10 * o_ptr->pval)) / 15;

		/* Paranoia -- prevent crashes */
		if (i < 1) i = 1;

		/* Back-fire XXX XXX XXX */
		if (rand_int(i) == 0) {
			/* a chance to just discharge it instead of destroying it */
			if (!rand_int(2)) {
				msg_print(Ind, "There is a static discharge.");
				o_ptr->pval = 0;
				o_ptr->ident |= ID_EMPTY;
				note_toggle_empty(o_ptr, TRUE);
			} else {
				/* Dangerous Hack -- Destroy the item */
				msg_print(Ind, "There is a bright flash of light.");

				/* Reduce and describe inventory */
				if (item >= 0) {
					inven_item_increase(Ind, item, -(3 + rand_int(o_ptr->number - 2)));
					inven_item_describe(Ind, item);
					inven_item_optimize(Ind, item);
				}
				/* Reduce and describe floor item */
				else {
					floor_item_increase(0 - item, -(3 + rand_int(o_ptr->number - 2)));
					floor_item_describe(0 - item);
					floor_item_optimize(0 - item);
				}
			}
		}

		/* Hack: No controlled mass-summoning in ironman deep dive challenge */
		else if (in_irondeepdive(&p_ptr->wpos) &&
		    ((o_ptr->tval == TV_STAFF && o_ptr->sval == SV_STAFF_SUMMONING) ||
		    (o_ptr->tval == TV_WAND && o_ptr->sval == SV_WAND_POLYMORPH))) {
			msg_print(Ind, "There is a static discharge.");
			o_ptr->pval = 0;
			o_ptr->ident |= ID_EMPTY;
			note_toggle_empty(o_ptr, TRUE);
		}

		/* Recharge */
		else {
			/* Extract a "power" */
			t = (pow / (lev + 2)) + 1;

			/* Hack -- ego */
			if (o_ptr->name2 == EGO_PLENTY) t <<= 1;

#if 0 /* old way */
			/* Recharge based on the power */
			if (t > 0) o_ptr->pval += 2 + randint(t);
#else /* new way: correct wand stacking, added stack size dr */
			/* Wands stack, so recharging must multiply the power.
			   Add small 'laziness' diminishing returns malus. */
			if (o_ptr->tval == TV_WAND) {
				/* Recharge based on the power */
				//if (t > 0) o_ptr->pval += (2 * o_ptr->number) + rand_int(t * o_ptr->number);

				/* allow dr to factor in more: */
				//Diminishing returns start at number=10: ' * (1050 / (10 + 1000 / (o_ptr->number + 4)) - 4) '
				//Diminishing returns start at number=3~: ' * (450 / (10 + 400 / (o_ptr->number + 3)) - 3) '
				dr = 4500 / (10 + 400 / (o_ptr->number + 3)) - 30;
				if (t > 0) o_ptr->pval += (3 * dr) / 10 + rand_int(t * dr) / 10;
			} else {
				/* Recharge based on the power */
				//if (t > 0) o_ptr->pval += 2 + randint(t);

				/* allow dr to factor in more: */
				dr = 4500 / (10 + 400 / (o_ptr->number + 3)) - 30;
 #ifdef NEW_MDEV_STACKING
				if (t > 0) o_ptr->pval += 1 + (rand_int((t + 2) * dr)) / 10;
 #else
				if (t > 0) o_ptr->pval += 1 + (rand_int((t + 2) * dr) / o_ptr->number) / 10;
 #endif
			}
#endif

			/* Hack -- we no longer "know" the item */
			o_ptr->ident &= ~ID_KNOWN;

			/* Hack -- we no longer think the item is empty */
			o_ptr->ident &= ~ID_EMPTY;
			note_toggle_empty(o_ptr, FALSE);
		}
	}

	/* Did we use up an item? */
	if (p_ptr->using_up_item >= 0) {
		inven_item_describe(Ind, p_ptr->using_up_item);
		inven_item_optimize(Ind, p_ptr->using_up_item);
		p_ptr->using_up_item = -1;
	}

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	/* We no longer have a recharge in progress */
	p_ptr->current_recharge = 0;

	/* Something was done */
	return (TRUE);
}



/*
 * Apply a "project()" directly to all viewable monsters
 */
bool project_los(int Ind, int typ, int dam, char *attacker) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	int		i, x, y;
	int		flg = PROJECT_NORF | PROJECT_JUMP | PROJECT_KILL | PROJECT_HIDE;
	bool		obvious = FALSE;
	char		pattacker[80];

	if (Ind) snprintf(pattacker, 80, "%s%s", Players[Ind]->name, attacker);
	else snprintf(pattacker, 80, "Something%s", attacker);

	/* WRAITHFORM reduces damage/effect! */
	if (p_ptr->tim_wraith) proj_dam_wraith(typ, &dam);
#ifdef MARTYR_CUT_DISP
	/* hack for Martyrdom, to avoid easy deep pit sweeping */
	else if (p_ptr->martyr)
		switch (typ) {
		case GF_DISP_UNDEAD:
		case GF_DISP_DEMON:
		case GF_DISP_EVIL:
		case GF_DISP_ALL:
			dam /= 2;
			break;
		}
#endif

	/* Affect all (nearby) monsters */
	for (i = 1; i < m_max; i++) {
		monster_type *m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip monsters not on this depth */
		if(!inarea(wpos, &m_ptr->wpos)) continue;

		/* Location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Require line of sight */
		if (!player_has_los_bold(Ind, y, x)) continue;

		/* Don't exceed max range (which may be < sight range)! */
		if (distance(p_ptr->py, p_ptr->px, y, x) > MAX_RANGE) continue;
		/* Maybe also check for BLOCK_LOS/BLOCK_CONTACT grids? (glass walls..) */
		if (!projectable_wall(wpos, p_ptr->py, p_ptr->px, y, x, MAX_RANGE)) continue;

		/* Jump directly to the target monster */
		if (project(0 - Ind, 0, wpos, y, x, dam, typ, flg, pattacker)) obvious = TRUE;
	}

#if 1	//this would require to differ between project_los calls that are meant
	//to work on players and those that are meant to work on monsters only
	//like GF_OLD_SPEED, GF_OLD_HEAL, etc. If this is done, an attacker string
	//should be added as well, however.

	/* Affect all (nearby) non-partied players */
	for (i = 1; i <= NumPlayers; i++) {
		/* If he's not playing, skip him */
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* If he's not here, skip him */
		if (!inarea(wpos, &Players[i]->wpos))
			continue;

		/* Ignore players we aren't hostile to */
		//if (!check_hostile(Ind, i)) continue;

		/* if we are in the same party, don't affect target player */
		if (p_ptr->party && (player_in_party(p_ptr->party, i)))
			continue;

		/* Location */
		y = Players[i]->py;
		x = Players[i]->px;

		/* Require line of sight */
		if (!player_has_los_bold(Ind, y, x)) continue;

		/* Jump directly to the target player */
		if (project(0 - Ind, 0, wpos, y, x, dam, typ, flg, pattacker)) obvious = TRUE;
	}
#endif

	/* Result */
	return (obvious);
}


/* Speed monsters */
bool speed_monsters(int Ind) {
	player_type *p_ptr = Players[Ind];
	return (project_los(Ind, GF_OLD_SPEED, p_ptr->lev, ""));
}
/* Slow monsters */
bool slow_monsters(int Ind, int pow) {
	return (project_los(Ind, GF_OLD_SLOW, pow, ""));
}
/* Sleep monsters */
bool sleep_monsters(int Ind, int pow) {
	return (project_los(Ind, GF_OLD_SLEEP, pow, ""));
}
/* Fear monsters */
bool fear_monsters(int Ind, int pow) {
	return (project_los(Ind, GF_TURN_ALL, pow, ""));
}
/* Stun monsters */
bool stun_monsters(int Ind, int pow) {
	return (project_los(Ind, GF_STUN, pow, ""));
}


/* Banish evil monsters */
bool away_evil(int Ind, int dist) {
	return (project_los(Ind, GF_AWAY_EVIL, dist, " banishes all evil"));
}


/* Turn undead */
bool turn_undead(int Ind) {
	player_type *p_ptr = Players[Ind];
	return (project_los(Ind, GF_TURN_UNDEAD, p_ptr->lev, " calls against all undead"));
}
/* Turn everyone */
bool turn_monsters(int Ind, int dam) {
	return (project_los(Ind, GF_TURN_ALL, dam, " calls against all monsters"));
}


/* Dispel undead monsters */
bool dispel_undead(int Ind, int dam) {
	return (project_los(Ind, GF_DISP_UNDEAD, dam, " banishes all undead"));
}
/* Dispel evil monsters */
bool dispel_evil(int Ind, int dam) {
	return (project_los(Ind, GF_DISP_EVIL, dam, " banishes all evil"));
}
/* Dispel demons */
bool dispel_demons(int Ind, int dam) {
	return (project_los(Ind, GF_DISP_DEMON, dam, " banishes all demons"));
}
/* Dispel all monsters */
bool dispel_monsters(int Ind, int dam) {
	return (project_los(Ind, GF_DISP_ALL, dam, " banishes all monsters"));
}


/*
 * Wake up all monsters, and speed up "los" monsters.
 */
void aggravate_monsters(int Ind, int who) {
	player_type *p_ptr = Players[Ind];
	monster_type * m_ptr;
	monster_race *r_ptr;

	int i;

	bool sleep = FALSE;
	bool speed = FALSE;

	/* Aggravate everyone nearby */
	for (i = 1; i < m_max; i++) {
		m_ptr = &m_list[i];
		r_ptr = race_inf(m_ptr);

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip monsters not on this depth */
		if (!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;

		/* Skip aggravating monster (or player) */
		if (i == who) continue;

		/* Wake up nearby sleeping monsters */
		if (distance(p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx) < MAX_SIGHT * 2)
#if 0
		if (m_ptr->cdis < MAX_SIGHT * 2)
#endif
		{
			/* Wake up */
			if (m_ptr->csleep) {
				/* Wake up */
				m_ptr->csleep = 0;
				sleep = TRUE;
			}
		}

		/* Don't speed up undead/nonliving monsters */
		if ((r_ptr->flags3 & RF3_NONLIVING) || (r_ptr->flags2 & RF2_EMPTY_MIND)) continue;

		/* Speed up monsters in line of sight */
		if (player_has_los_bold(Ind, m_ptr->fy, m_ptr->fx)) {
			/* Speed up (instantly) to racial base + 10 */
                        if (m_ptr->mspeed < m_ptr->speed + 10) {
				/* Speed up */
                                m_ptr->mspeed = m_ptr->speed + 10;
				speed = TRUE;
			}
		}
	}

	/* Messages */
	/* the_sandman: added _near so other players can hear too */
	if (speed) {
		msg_print(Ind, "You feel a sudden stirring nearby!");
		msg_print_near(Ind, "You feel a sudden stirring nearby!");
	} else if (sleep) {
		msg_print(Ind, "You hear a sudden stirring in the distance!");
		msg_print_near(Ind, "You hear a sudden stirring in the distance!");
	}
}

/*
 * Wake up all monsters
 */
void wakeup_monsters(int Ind, int who) {
	player_type *p_ptr = Players[Ind];
	int i;
	bool sleep = FALSE;

	/* Aggravate everyone nearby */
	for (i = 1; i < m_max; i++) {
		monster_type	*m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip monsters not on this depth */
		if (!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;

		/* Skip aggravating monster (or player) */
		if (i == who) continue;

		/* Wake up nearby sleeping monsters */
		if (distance(p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx) < MAX_SIGHT * 2)
#if 0
		if (m_ptr->cdis < MAX_SIGHT * 2)
#endif
		{
			/* Wake up */
			if (m_ptr->csleep) {
				/* Wake up */
				m_ptr->csleep = 0;
				sleep = TRUE;
			}
		}
	}

	/* Messages */
	/* the_sandman: added _near so other players can hear too */
	if (sleep) {
		msg_print(Ind, "You hear a sudden stirring in the distance!");
		msg_print_near(Ind, "You hear a sudden stirring in the distance!");
	}
}

void wakeup_monsters_somewhat(int Ind, int who) {
	player_type *p_ptr = Players[Ind];
	int i;
	bool sleep = FALSE;

	/* Aggravate everyone nearby */
	for (i = 1; i < m_max; i++) {
		monster_type *m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip monsters not on this depth */
		if (!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;

		/* Skip aggravating monster (or player) */
		if (i == who) continue;

		/* Wake up nearby sleeping monsters */
		if (distance(p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx) < MAX_SIGHT * 2)
#if 0
		if (m_ptr->cdis < MAX_SIGHT * 2)
#endif
		{
			/* Disturb sleep somewhat */
			if (m_ptr->csleep) {
				m_ptr->csleep = (m_ptr->csleep * 3) / 5;
				m_ptr->csleep -= 10 + rand_int(6); //10
				if (m_ptr->csleep <= 0) {
					m_ptr->csleep = 0;
					sleep = TRUE;
				}
			}
		}
	}

#if 1 /* better style, not to display a msg maybe? */
	/* Messages */
	/* the_sandman: added _near so other players can hear too */
	if (sleep) {
		msg_print(Ind, "You hear a sudden stirring in the distance!");
		msg_print_near(Ind, "You hear a sudden stirring in the distance!");
	}
#endif
}

void distract_monsters(int Ind) {
	player_type *p_ptr = Players[Ind];
	monster_type	*m_ptr;
	monster_race *r_ptr;

	int i;
	bool tauntable;

	msg_print(Ind, "You make yourself look less threatening than your team mates.");
	msg_format_near(Ind, "%s pretends you're more threatening than him.", p_ptr->name);
	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);

	for (i = 1; i < m_max; i++) {
		m_ptr = &m_list[i];
		r_ptr = race_inf(m_ptr);

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;
		if (!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;

		if ((r_ptr->flags3 & (RF3_ORC | RF3_TROLL | RF3_GIANT | RF3_DEMON)) ||
		    (strchr("hHkptny", r_ptr->d_char))) tauntable = TRUE;
		else tauntable = FALSE;

		if (r_ptr->level >= 98) tauntable = FALSE; /* end-game specialties are exempt */
		if (r_ptr->flags1 & RF1_UNIQUE) {
#if 0 /* still distractable, hence if 0'ed */
			if (r_ptr->flags2 & RF2_POWERFUL) {
				if (magik(75)) tauntable = FALSE; /* powerful unique monsters prefer to stay alive */
			} else
#endif
				{
				if (magik(50)) tauntable = FALSE; /* unique monsters resist more often */
			}
		}
		else if (magik(25)) tauntable = FALSE; /* all monsters sometimes resist taunt */
//		else if (magik(r_ptr->level / 3)) tauntable = FALSE; /* all monsters sometimes resist taunt */

#if 0 /*actually, being POWERFUL doesn't really protect.. */
		if (r_ptr->flags2 & RF2_POWERFUL) tauntable = FALSE;
#endif
#if 0 /* shamans -_- and not only that, way too many monsters are SMART, so commented out for now */
		if (r_ptr->flags2 & RF2_SMART) tauntable = FALSE; /* smart monsters don't fall for taunts */
#endif
		if (r_ptr->flags3 & RF3_NONLIVING) tauntable = FALSE; /* nonliving monsters can't perceive taunts */
		if (r_ptr->flags2 & RF2_EMPTY_MIND) tauntable = FALSE; /* empty-minded monsters can't perceive taunts */
		if ((r_ptr->flags2 & RF2_WEIRD_MIND) && magik(50)) tauntable = FALSE; /* weird-minded monsters are hard to predict */
		/* RF2_CAN_SPEAK ?? */

		if (!tauntable) continue;

		/* monster stands right next to this player? */
		if (ABS(m_ptr->fy - p_ptr->py) <= 1 && ABS(m_ptr->fx - p_ptr->px) <= 1 &&
		    m_ptr->last_target_melee == Ind) {
			/* set another (adjacent) player as target */
			m_ptr->switch_target = Ind;
		}
	}
}

/* attempts to make certain types of AI_ANNOY monsters approach the player for a while */
void taunt_monsters(int Ind)
{
	player_type *p_ptr = Players[Ind];
	monster_type	*m_ptr;
	monster_race *r_ptr;

	int i;
	bool sleep = FALSE, tauntable;

	msg_print(Ind, "You call out a taunt!");
	msg_format_near(Ind, "%s calls out a taunt!", p_ptr->name);
	break_cloaking(Ind, 0);
	stop_precision(Ind);

	for (i = 1; i < m_max; i++) {
		m_ptr = &m_list[i];
		r_ptr = race_inf(m_ptr);

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;
		if(!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;

		if ((r_ptr->flags3 & (RF3_ORC | RF3_TROLL | RF3_GIANT | RF3_DEMON)) ||
		    (strchr("hHkptny", r_ptr->d_char))) tauntable = TRUE;
		else tauntable = FALSE;

		if (r_ptr->level >= 98) tauntable = FALSE; /* end-game specialties are exempt */
		if (r_ptr->flags1 & RF1_UNIQUE) {
			if (r_ptr->flags2 & RF2_POWERFUL) {
				if (magik(75)) tauntable = FALSE; /* powerful unique monsters prefer to stay alive */
			} else {
				if (magik(50)) tauntable = FALSE; /* unique monsters resist more often */
			}
		}
		else if (magik(20)) tauntable = FALSE; /* all monsters sometimes resist taunt */
//		else if (magik(r_ptr->level / 3)) tauntable = FALSE; /* all monsters sometimes resist taunt */

#if 0 /*actually, being POWERFUL doesn't really protect.. */
		if (r_ptr->flags2 & RF2_POWERFUL) tauntable = FALSE;
#endif
#if 0 /* shamans -_- and not only that, way too many monsters are SMART, so commented out for now */
		if (r_ptr->flags2 & RF2_SMART) tauntable = FALSE; /* smart monsters don't fall for taunts */
#endif
		if (r_ptr->flags3 & RF3_NONLIVING) tauntable = FALSE; /* nonliving monsters can't perceive taunts */
		if (r_ptr->flags2 & RF2_EMPTY_MIND) tauntable = FALSE; /* empty-minded monsters can't perceive taunts */
		if ((r_ptr->flags2 & RF2_WEIRD_MIND) && magik(50)) tauntable = FALSE; /* weird-minded monsters are hard to predict */
		/* RF2_CAN_SPEAK ?? */

		/* some egos aren't tauntable possibly */
		switch(m_ptr->ego) {
		case 10: case 13: case 23: /* archers */
		case 9: case 30: /* mages */
			tauntable = FALSE;
			break;
		default: ;
		}

		/* some monsters (hard-coded ;() aren't tauntable */
		switch(m_ptr->r_idx) {
		case 46: case 93: case 240: case 449: case 638: case 738: /* p mages */
		case 178: case 281: case 375: case 657: /* h mages */
		case 539: /* h archers */
			tauntable = FALSE;
			break;
		default: ;
		}

		if (!tauntable) continue;

		if (player_has_los_bold(Ind, m_ptr->fy, m_ptr->fx)) {
#if 0
			/* wake up */
			if (m_ptr->csleep) {
				m_ptr->csleep = 0;
				sleep = TRUE;
			}
#endif

			/* taunt */
			m_ptr->monfear = 0;
			m_ptr->taunted = 7; /* number of monster moves staying taunted */
			//was 5
		}

		/* monster stands right next to this player? */
		if (ABS(m_ptr->fy - p_ptr->py) <= 1 && ABS(m_ptr->fx - p_ptr->px) <= 1)
			m_ptr->last_target_melee = Ind;
	}

	if (sleep) {
		msg_print(Ind, "You hear a sudden stirring in the distance!");
		msg_print_near(Ind, "You hear a sudden stirring in the distance!");
	}
}

/* Need it for detonation pots in potion_smash_effect - C. Blue */
void aggravate_monsters_floorpos(worldpos *wpos, int x, int y) {
	int i;

	/* Aggravate everyone nearby */
	for (i = 1; i < m_max; i++) {
		monster_type	*m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip monsters not on this depth */
		if (!inarea(wpos, &m_ptr->wpos)) continue;

		/* Wake up nearby sleeping monsters */
		if (distance(y, x, m_ptr->fy, m_ptr->fx) < MAX_SIGHT * 2)
#if 0
		if (m_ptr->cdis < MAX_SIGHT * 2)
#endif
		{
			/* Wake up */
			if (m_ptr->csleep) {
				/* Wake up */
				m_ptr->csleep = 0;
			}
		}
	}
}


/*
 * Wake up minions (escorts/friends). May be used by a pack monster or unique.
 * As soon as a monster is hit by a player or has a LOS to him. - C. Blue
 */
void wake_minions(int Ind, int who) {
	player_type *p_ptr = Players[Ind];

	monster_type	*mw_ptr = &m_list[who], *m_ptr = NULL;
	monster_race    *rw_ptr = race_inf(m_ptr), *r_ptr = NULL;
	char		mw_name[MNAME_LEN];

	int i;

	bool sleep = FALSE;
//	bool speed = FALSE;


	monster_desc(Ind, mw_name, who, 0x00);

	/* Aggravate everyone nearby */
	for (i = 1; i < m_max; i++) {
		m_ptr = &m_list[i];
		r_ptr = race_inf(m_ptr);

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip monsters not on this depth */
		if (!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;

		/* Skip aggravating monster (or player) */
		if (i == who) continue;

		/* wake_minions used by a FRIENDS monster? */
		if (rw_ptr->flags1 & (RF1_FRIEND | RF1_FRIENDS)) {
			/* Skip monsters who aren't its friends */
			if (m_ptr->r_idx != mw_ptr->r_idx) continue;
		}

		/* wake_minions used by a UNIQUE monster? */
		if (rw_ptr->flags1 & RF1_UNIQUE) {
			/* Skip monsters who don't belong to its escort */

			/* Paranoia -- Skip identical monsters */
			if (m_ptr->r_idx == mw_ptr->r_idx) continue;
			/* Require similar "race" */
			if (r_ptr->d_char != rw_ptr->d_char) continue;
			/* Skip more advanced monsters */
			if (r_ptr->level > rw_ptr->level) continue;
			/* Skip unique monsters */
			if (r_ptr->flags1 & RF1_UNIQUE) continue;
			/* Skip non-escorting monsters */
//			if (m_ptr->escorting != who) continue;
		}

		/* wake_minions used by an escorting monster? */
#if 0	/* m_ptr->escorting = m_idx not yet implemented */
		if (mw_ptr->escorting) {
			/* Wake up his leader, which always is unique */
			if (m_ptr->r_idx != mw_ptr->escorting) continue;
			/* Leader wakes the pack */
			wake_minions(Ind, i);
		}
#endif

		/* Wake up nearby sleeping monsters */
		if (distance(mw_ptr->fy, mw_ptr->fx, m_ptr->fy, m_ptr->fx) > rw_ptr->aaf) continue;
#if 0
		if (m_ptr->cdis < MAX_SIGHT * 2)
#endif
		{
			/* Wake up */
			if (m_ptr->csleep) {
				/* Wake up */
				m_ptr->csleep = 0;
				sleep = TRUE;
			}
		}
#if 0
		/* Speed up monsters in line of sight */
		if (player_has_los_bold(Ind, m_ptr->fy, m_ptr->fx)) {
			/* Speed up (instantly) to racial base + 10 */
                        if (m_ptr->mspeed < m_ptr->speed + 10) {
				/* Speed up */
                                m_ptr->mspeed = m_ptr->speed + 10;
				speed = TRUE;
			}
		}
#endif
	}

	/* Messages */
	if (!sleep) return;
//	if (speed) msg_print(Ind, "You feel a sudden stirring nearby!");
//	else if (sleep) msg_print(Ind, "You hear a sudden stirring in the distance!");
	if ((r_ptr->flags2 & RF2_EMPTY_MIND) || (r_ptr->flags3 & RF3_NONLIVING)) ;
	else if (r_ptr->flags3 & RF3_DRAGON) msg_format(Ind, "%s roars loudly!", mw_name);
	else if (r_ptr->flags3 & RF3_UNDEAD) msg_format(Ind, "%s moans loudly!", mw_name);
	else if (r_ptr->flags3 & RF3_ANIMAL) msg_format(Ind, "%s makes an alerting sound!", mw_name);
	else if ((r_ptr->flags3 & (RF3_ORC | RF3_TROLL | RF3_GIANT | RF3_DEMON | RF3_DRAGONRIDER)) ||
	    (strchr("AhHJkpPtyn", r_ptr->d_char))) {
		msg_format(Ind, "%s shouts a command!", mw_name);
		msg_format_near(Ind, "%s shouts a command!", mw_name);
	}
}


/*
 * If Ind <=0, no one takes the damage.	- Jir -OA
 */
bool genocide_aux(int Ind, worldpos *wpos, char typ) {
	player_type *p_ptr = Players[Ind];
	int i;
	bool result = FALSE;
	int tmp;	// , d = 999;

	dun_level *l_ptr = getfloor(wpos);
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return(FALSE);
	if (l_ptr && l_ptr->flags1 & LF1_NO_GENO) return(FALSE);

	bypass_invuln = TRUE;

	/* Delete the monsters of that "type" */
	for (i = 1; i < m_max; i++) {
		monster_type	*m_ptr = &m_list[i];
		monster_race    *r_ptr = race_inf(m_ptr);

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Hack -- Skip Unique Monsters */
		if (r_ptr->flags1 & RF1_UNIQUE) continue;

		/* Skip "wrong" monsters */
		if (r_ptr->d_char != typ) continue;

		/* Skip monsters not on this depth */
		if(!inarea(wpos, &m_ptr->wpos)) continue;

		/* Skip those immune */
		if (r_ptr->flags9 & RF9_IM_TELE) continue;

		/* Roll for resistance */
		tmp = r_ptr->level;
#ifdef RESIST_GENO
		if (randint(RESIST_GENO) < tmp) continue;
#endif	// RESIST_GENO

#ifdef NO_GENO_ON_ICKY
		/* Not valid inside a vault */
		if (zcave[m_ptr->fy][m_ptr->fx].info & CAVE_ICKY) continue;
#endif	// NO_GENO_ON_ICKY

		/* Delete the monster */
		delete_monster_idx(i, TRUE);

		/* Take damage */
		if (Ind > 0)
		{
			if (!p_ptr->admin_dm)
				take_hit(Ind, randint(4 + (tmp >> 3)), "the strain of casting Genocide", 0);

			/* Redraw */
			p_ptr->redraw |= (PR_HP);

			/* Window stuff */
			/* p_ptr->window |= (PW_PLAYER); */

			/* Handle */
			handle_stuff(Ind);

			/* Delay */
//			Send_flush(Ind); /* I don't think a delay is really necessary - mikaelh */
		}

		/* Take note */
		result = TRUE;
	}

	if (Ind > 0) {
#ifdef SEVERE_GENO
		if (!p_ptr->death && result && !p_ptr->admin_dm)
			take_hit(Ind, p_ptr->chp >> 1, "the strain of casting Genocide", 0);

		/* Redraw */
		p_ptr->redraw |= (PR_HP);
#endif	// SEVERE_GENO

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Handle */
		handle_stuff(Ind);
	}

	bypass_invuln = FALSE;

	return (result);
}

/*
 * Delete all non-unique monsters of a given "type" from the level
 *
 * This is different from normal Angband now -- the closest non-unique
 * monster is chosen as the designed character to genocide.
 */
bool genocide(int Ind) {
	player_type *p_ptr = Players[Ind];
	int	i;
	char	typ = -1;
	int	d = 999, tmp;

	worldpos *wpos = &p_ptr->wpos;
	dun_level *l_ptr = getfloor(wpos);
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return(FALSE);
	if(l_ptr && l_ptr->flags1 & LF1_NO_GENO) return(FALSE);	// double check..

	/* Search all monsters and find the closest */
	for (i = 1; i < m_max; i++) {
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = race_inf(m_ptr);

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Hack -- Skip Unique Monsters */
		if (r_ptr->flags1 & RF1_UNIQUE) continue;

		/* Skip monsters not on this depth */
		if(!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;

		/* Check distance */
		if ((tmp = distance(p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx)) < d) {
			/* Set closest distance */
			d = tmp;

			/* Set char */
			typ = r_ptr->d_char;
		}
	}

	/* Check to make sure we found a monster */
	if (d == 999) return FALSE;

	return (genocide_aux(Ind, wpos, typ));
}


/*
 * Delete all nearby (non-unique) monsters
 * Either Ind must be a player's index for use by a player,
 * or Ind must be -m_idx for use by a monster trap.
 */
bool obliteration(int who) {
	player_type *p_ptr;
	int i, tmp, rad, x, y;
	bool result = FALSE;

	worldpos *wpos;
	dun_level *l_ptr;
	cave_type **zcave;

	/* monster hit a monster trap? */
	if (who < 0) {
		rad = 1;
		p_ptr = NULL;
		wpos = &m_list[-who].wpos;
		x = m_list[-who].fx;
		y = m_list[-who].fy;
	}
	/* player cast it */
	else {
		rad = MAX_SIGHT;
		p_ptr = Players[who];
		wpos = &p_ptr->wpos;
		x = p_ptr->px;
		y = p_ptr->py;
	}

	if (!(zcave = getcave(wpos))) return(FALSE);
	l_ptr = getfloor(wpos);
	if (l_ptr && l_ptr->flags1 & LF1_NO_GENO) return(FALSE);

	bypass_invuln = TRUE;

	/* Delete the (nearby) monsters */
	for (i = 1; i < m_max; i++) {
		monster_type *m_ptr = &m_list[i];
                monster_race *r_ptr = race_inf(m_ptr);

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip monsters not on this depth */
		if (!inarea(wpos, &m_ptr->wpos)) continue;

		/* Hack -- Skip unique monsters */
		if (r_ptr->flags1 & RF1_UNIQUE) continue;

		/* Skip distant monsters */
		if (distance(y, x, m_ptr->fy, m_ptr->fx) > rad)
#if 0
		if (m_ptr->cdis > MAX_SIGHT)
#endif
			continue;

		/* Skip those immune */
		if (r_ptr->flags9 & RF9_IM_TELE) continue;

		/* Roll for resistance */
		tmp = r_ptr->level;
#ifdef RESIST_GENO
		if (randint(RESIST_GENO) < tmp) continue;
#endif	// RESIST_GENO

#ifdef NO_GENO_ON_ICKY
		/* Not valid inside a vault */
		if (zcave[m_ptr->fy][m_ptr->fx].info & CAVE_ICKY && p_ptr && !p_ptr->admin_dm)
			continue;
#endif	// NO_GENO_ON_ICKY

		/* Delete the monster */
		delete_monster_idx(i, TRUE);

		/* Hack -- visual feedback */
		/* does not effect the dungeon master, because it disturbs his movement
		 */
		if (p_ptr && !p_ptr->admin_dm) {
			take_hit(who, randint(3 + (tmp >> 3)), "the strain of casting Genocide", 0);

			/* Redraw */
			p_ptr->redraw |= (PR_HP);

			/* Window stuff */
			/* p_ptr->window |= (PW_PLAYER); */

			/* Handle */
			handle_stuff(who);

			/* Delay */
			//Send_flush(who); /* I don't think a delay is really necessary - mikaelh */
		}

		/* Note effect */
		result = TRUE;
	}

	if (p_ptr) {
#ifdef SEVERE_GENO
		if (!p_ptr->death && result && !p_ptr->admin_dm) {
			take_hit(who, p_ptr->chp >> 1, "the strain of casting Genocide", 0);

			/* Redraw */
			p_ptr->redraw |= (PR_HP);
		}
#endif

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Handle */
		handle_stuff(who);
	}

	bypass_invuln = FALSE;

	return (result);
}



/*
 * Probe nearby monsters
 */
bool probing(int Ind) {
	monster_type *m_ptr;
	monster_race *r_ptr;
	int            i;
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	bool	probe = FALSE;

	/* Probe all (nearby) monsters */
	for (i = 1; i < m_max; i++) {
		m_ptr = &m_list[i];
		r_ptr = race_inf(m_ptr);

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip monsters not on this depth */
		if (!inarea(&m_ptr->wpos, wpos)) continue;

		/* Require line of sight */
		if (!player_has_los_bold(Ind, m_ptr->fy, m_ptr->fx)) continue;

		/* Probe visible monsters */
		if (p_ptr->mon_vis[i]) {
			char m_name[MNAME_LEN];
			char buf[80];
			int j;

			/* Start the message */
			if (!probe) msg_print(Ind, "Probing...");

			/* Get "the monster" or "something" */
			monster_desc(Ind, m_name, i, 0x04);
			sprintf(buf, "blows");

			for (j = 0; j < 4; j++)
				if (m_ptr->blow[j].d_dice) strcat(buf, format(" %dd%d", m_ptr->blow[j].d_dice, m_ptr->blow[j].d_side));

			/* Describe the monster */
			if (r_ptr->flags7 & RF7_NO_DEATH)
				msg_format(Ind, "%^s (%d) has unknown hp, %d ac, %d speed.", m_name, m_ptr->level, m_ptr->ac, m_ptr->mspeed - 110);
			else
				msg_format(Ind, "%^s (%d) has %d hp, %d ac, %d speed.", m_name, m_ptr->level, m_ptr->hp, m_ptr->ac, m_ptr->mspeed - 110);
			/* include m_idx for admins */
			if (is_admin(p_ptr)) msg_format(Ind, "%^s (Lv%d,%d) %s.", m_name, m_ptr->level, i, buf);
			else msg_format(Ind, "%^s (Lv%d) %s.", m_name, m_ptr->level, buf);

			/* Learn all of the non-spell, non-treasure flags */
			lore_do_probe(i);

			/* Probe worked */
			probe = TRUE;
		}
	}

	/* Done */
	if (probe) msg_print(Ind, "That's all.");

	/* Result */
	return (probe);
}



/*
 * The spell of destruction
 *
 * This spell "deletes" monsters (instead of "killing" them).
 *
 * Later we may use one function for both "destruction" and
 * "earthquake" by using the "full" to select "destruction".
 */
void destroy_area(struct worldpos *wpos, int y1, int x1, int r, bool full, byte feat, int stun) {
	int y, x, k, t, Ind;
	player_type *p_ptr;
	cave_type *c_ptr;
	/*bool flag = FALSE;*/

	dun_level *l_ptr = getfloor(wpos);
	struct c_special *cs_ptr;       /* for special key doors */
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return;
	if(l_ptr && l_ptr->flags1 & LF1_NO_DESTROY) return;

	/* among others, make sure town areas aren't affected.. */
	if (!allow_terraforming(wpos, FEAT_WALL_EXTRA)) return;

	/* XXX XXX */
	full = full ? full : 0;

	/* Set to default */
	feat = feat ? feat : FEAT_FLOOR;

	/* Can't trigger within Vault? - note: this prevents exploit as well as death-_trap_ at once :) */
	if (zcave[y1][x1].info & CAVE_ICKY) return;

	/* Big area of affect */
	for (y = (y1 - r); y <= (y1 + r); y++) {
		for (x = (x1 - r); x <= (x1 + r); x++) {
			/* Skip illegal grids */
			if (!in_bounds(y, x)) continue;

			/* Extract the distance */
			k = distance(y1, x1, y, x);

			/* Stay in the circle of death */
			if (k > r) continue;

			/* Access the grid */
			c_ptr = &zcave[y][x];

			/* Hack -- Notice player affect */
			if (c_ptr->m_idx < 0) {
				Ind = 0 - c_ptr->m_idx;
				p_ptr = Players[Ind];

				/* Message */
				msg_print(Ind, "\377oThere is a searing blast of light and a terrible shockwave!");
#ifdef USE_SOUND_2010
				sound(Ind, "destruction", NULL, SFX_TYPE_MISC, FALSE);
#endif

				/* Blind the player */
				if (!p_ptr->resist_blind && !p_ptr->resist_lite) {
					/* Become blind */
					(void)set_blind(Ind, p_ptr->blind + 20 + randint(10));
				}
				if (!is_admin(p_ptr)) {
					/* ~20s in NR, depends tho */
#if 0 /* will prevent k.o. */
					if (!k) (void)set_stun_raw(Ind, stun);
					else (void)set_stun(Ind, stun);
#else /* keep k.o.'ing */
					(void)set_stun_raw(Ind, stun);
#endif
				}

				/* Mega-Hack -- Forget the view and lite */
				p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

				/* Update stuff */
				p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

				/* Update the monsters */
				p_ptr->update |= (PU_MONSTERS);

				/* Redraw map */
				p_ptr->redraw |= (PR_MAP);

				/* Window stuff */
				p_ptr->window |= (PW_OVERHEAD);

				/* Do not hurt this grid */
				continue;
			}

			/* Vault is protected */
			if (c_ptr->info & CAVE_ICKY) continue;

			/* Lose room and nest */
			/* Hack -- don't do this to houses/rooms outside the dungeon,
			 * this will protect hosues outside town.
			 */
			if (wpos->wz) {
				/* Lose room and nest */
				c_ptr->info &= ~(CAVE_ROOM | CAVE_NEST_PIT);
			}

			/* Delete the monster (if any) */
			if (c_ptr->m_idx > 0) {
				monster_race *r_ptr = race_inf(&m_list[c_ptr->m_idx]);
				if (!(r_ptr->flags9 & RF9_IM_TELE)) delete_monster(wpos, y, x, TRUE);
				else continue;
			}

			/* Special key doors are protected -C. Blue */
			if ((cs_ptr = GetCS(c_ptr, CS_KEYDOOR))) continue;

			/* Lose light and knowledge */
			c_ptr->info &= ~(CAVE_GLOW);
			everyone_forget_spot(wpos, y, x);

			/* Hack -- Skip the epicenter */
			if ((y == y1) && (x == x1)) continue;

			/* Destroy "valid" grids */
			//if ((cave_valid_bold(zcave, y, x)) && !(c_ptr->info&CAVE_ICKY))
			if (cave_valid_bold(zcave, y, x)) {
				struct c_special *cs_ptr;

				/* Delete the object (if any) */
				delete_object(wpos, y, x, TRUE);

				/* Wall (or floor) type */
				t = rand_int(200);

				if ((cs_ptr = GetCS(c_ptr, CS_TRAPS))) {
					/* Destroy the trap */
					if (t < 100) cs_erase(c_ptr, cs_ptr);
					else cs_ptr->sc.trap.found = FALSE;

					/* Redraw */
					//everyone_lite_spot(wpos, y, x);
				}

				/* Granite */
				if (t < 20) {
					/* Create granite wall */
					cave_set_feat_live(wpos, y, x, FEAT_WALL_EXTRA);
				}

				/* Quartz */
				else if (t < 70) {
					/* Create quartz vein */
					cave_set_feat_live(wpos, y, x, FEAT_QUARTZ);
				}

				/* Magma */
				else if (t < 100) {
					/* Create magma vein */
					cave_set_feat_live(wpos, y, x, FEAT_MAGMA);
				}

				/* Floor */
				else {
					/* Create floor or whatever specified */
					cave_set_feat_live(wpos, y, x, feat);
				}
			}
		}
	}

	/* Stun everyone around who hasn't been affected by the initial stun */
	for (k = 1; k <= NumPlayers; k++) {
		p_ptr = Players[k];
		t = distance(y1, x1, p_ptr->py, p_ptr->px);
		if (!is_admin(p_ptr) && inarea(wpos, &p_ptr->wpos) && t > r) {
			msg_print(k, "\377oYou are hit by a terrible shockwave!");
#if 0 /* will prevent k.o. */
			(void)set_stun(k, stun - (t - r) / 3);
#else /* keep k.o.'ing */
			(void)set_stun_raw(k, stun - (t - r) / 3);
#endif
		}
	}
}


/*
 * Induce an "earthquake" of the given radius at the given location.
 *
 * This will turn some walls into floors and some floors into walls.
 *
 * The player will take damage and "jump" into a safe grid if possible,
 * otherwise, he will "tunnel" through the rubble instantaneously.
 *
 * Monsters will take damage, and "jump" into a safe grid if possible,
 * otherwise they will be "buried" in the rubble, disappearing from
 * the level in the same way that they do when genocided.
 *
 * Note that thus the player and monsters (except eaters of walls and
 * passers through walls) will never occupy the same grid as a wall.
 * Note that as of now (2.7.8) no monster may occupy a "wall" grid, even
 * for a single turn, unless that monster can pass_walls or kill_walls.
 * This has allowed massive simplification of the "monster" code.
 */
void earthquake(struct worldpos *wpos, int cy, int cx, int r) {
	int		i, t, y, x, yy, xx, dy, dx, oy, ox;
	int		damage = 0;
	int		sn = 0, sy = 0, sx = 0;
	int Ind;
	player_type *p_ptr;
	/*bool	hurt = FALSE;*/
	cave_type	*c_ptr;
	bool	map[32][32];
	dun_level *l_ptr = getfloor(wpos);
	struct c_special *cs_ptr;	/* for special key doors */
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return;
	if(l_ptr && (l_ptr->flags1 & LF1_NO_DESTROY) && !override_LF1_NO_DESTROY) return;
	override_LF1_NO_DESTROY = FALSE;

	/* among others, make sure town areas aren't affected.. */
	if (!allow_terraforming(wpos, FEAT_WALL_EXTRA)) return;

	/* Paranoia -- Dnforce maximum range */
	if (r > 12) r = 12;

	/* Clear the "maximal blast" area */
	for (y = 0; y < 32; y++) {
		for (x = 0; x < 32; x++) {
			map[y][x] = FALSE;
		}
	}

	/* No one has taken any damage from this earthquake yet - mikaelh */
	for (Ind = 1; Ind <= NumPlayers; Ind++)
		Players[Ind]->total_damage = 0;

	/* Check around the epicenter */
	for (dy = -r; dy <= r; dy++) {
		for (dx = -r; dx <= r; dx++) {
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip illegal grids */
			if (!in_bounds(yy, xx)) continue;

			/* Skip distant grids */
			t = distance(cy, cx, yy, xx);
			if (t > r) continue;

			/* Access the grid */
			c_ptr = &zcave[yy][xx];

			/* Hack -- ICKY spaces are protected outside of the dungeon */
			if ((!wpos->wz) && (c_ptr->info & CAVE_ICKY)) continue;

			/* Special key doors are protected -C. Blue */
			if ((cs_ptr = GetCS(c_ptr, CS_KEYDOOR))) continue;

#if 0 /* noticed this at 10.jan 2009, if'0ed it, and added below else branch,
	since it allows teleportation within vaults!  - C. Blue */
/* note: this miiiight be ok for non-no_tele vaults, or if no_tele is also removed, and if
   additionally the vault wasn't build from perma-walls, so the walls actually got quaked away too;
   for that, also check 'cave_valid_bold' check used to actually destroy grids (further below). */

			/* Lose room and vault and nest */
			c_ptr->info &= ~(CAVE_ROOM | CAVE_NEST_PIT | CAVE_ICKY | CAVE_STCK);

#else /* keep preventing teleport from targetting a space inside the (quaked) vault */
			/* Lose room and nest */
			c_ptr->info &= ~(CAVE_ROOM | CAVE_NEST_PIT);

 #if 0 /* addition mentioned above, but maybe bad idea? */
			if (!(c_ptr->info & (CAVE_ICKY_PERMA | CAVE_STCK))) c_ptr->info &= ~CAVE_ICKY;
 #endif
#endif

			/* Lose light */
			c_ptr->info &= ~(CAVE_GLOW);

			/* This can be really annoying and frustrating - mikaelh */
			//everyone_forget_spot(wpos, y, x);
			everyone_lite_spot(wpos, y, x);

#ifdef USE_SOUND_2010
			if (c_ptr->m_idx < 0) sound(-c_ptr->m_idx, "earthquake", NULL, SFX_TYPE_NO_OVERLAP, FALSE);
#endif

			/* Skip the epicenter */
			if ((!dx && !dy) && r) continue;

			/* Skip most grids */
			if (rand_int(100) < 85) continue;

			/* Damage this grid */
			map[16 + yy - cy][16 + xx - cx] = TRUE;

			/* Hack -- Take note of player damage */
			if (c_ptr->m_idx < 0) {
				Ind = 0 - c_ptr->m_idx;
				p_ptr = Players[Ind];
				store_exit(Ind);

				sn = 0;

				/* Check around the player */
				for (i = 0; i < 8; i++) {
					/* Access the location */
					y = p_ptr->py + ddy[i];
					x = p_ptr->px + ddx[i];

					/* Skip non-empty grids */
					if (!cave_empty_bold(zcave, y, x)) continue;

					/* Important -- Skip "quake" grids */
					if (map[16+y-cy][16+x-cx]) continue;

					/* Count "safe" grids */
					sn++;

					/* Randomize choice */
					if (rand_int(sn) > 0) continue;

					/* Save the safe location */
					sy = y; sx = x;
				}

				/* Random message */
				switch (randint(3)) {
				case 1:
					msg_print(Ind, "The cave ceiling collapses!");
					break;
				case 2:
					msg_print(Ind, "The cave floor twists in an unnatural way!");
					break;
				default:
					msg_print(Ind, "The cave quakes! You are pummeled with debris!");
					break;
				}

				/* Hurt the player a lot */
				if (!sn) {
					/* Message and damage */
					damage = 300;
					if (get_skill(p_ptr, SKILL_EARTH) == 50) damage /= 2;
					/* Cap the damage - mikaelh */
					if (damage + p_ptr->total_damage > 300) {
						damage = 300 - p_ptr->total_damage;
					}
					if (damage) msg_format(Ind, "You are severely crushed for \377o%d\377w damage!", damage);

					/* Stun only once - mikaelh */
					if (p_ptr->total_damage == 0) {
						if (!t) (void)set_stun_raw(Ind, p_ptr->stun + randint(40));
						else (void)set_stun(Ind, p_ptr->stun + randint(40));
					}
				}

				/* Destroy the grid, and push the player to safety */
				else {
					/* Calculate results */
					switch (randint(3) - (magik((p_ptr->dodge_level * 3) / 4) ? 1 : 0) - (p_ptr->shadow_running ? 1 : 0)) {
						case -1:
						case 0:
						case 1:
							damage = 0;
							msg_format(Ind, "\377%cYou nimbly dodge the blast and take no damage!", COLOUR_DODGE_GOOD);
							break;
						case 2:
							damage = damroll(10, 4);
							if (get_skill(p_ptr, SKILL_EARTH) == 50) damage /= 2;
							/* Cap the damage - mikaelh */
							if (damage + p_ptr->total_damage > 300)
								damage = 300 - p_ptr->total_damage;
							if (damage)
								msg_format(Ind, "You are bashed by rubble for \377o%d\377w damage!", damage);

							/* Stun only once - mikaelh */
							if (p_ptr->total_damage == 0) {
								if (!t) (void)set_stun_raw(Ind, p_ptr->stun + randint(33));
								else (void)set_stun(Ind, p_ptr->stun + randint(33));
							}
							break;
						case 3:
							damage = damroll(30, 4);
							if (get_skill(p_ptr, SKILL_EARTH) == 50) damage /= 2;
							/* Cap the damage - mikaelh */
							if (damage + p_ptr->total_damage > 300)
								damage = 300 - p_ptr->total_damage;
							if (damage)
								msg_format(Ind, "You are crushed between the floor and ceiling for \377o%d\377w damage!", damage);

							/* Stun only once - mikaelh */
							if (p_ptr->total_damage == 0) {
								if (!t) (void)set_stun_raw(Ind, p_ptr->stun + randint(33));
								else (void)set_stun(Ind, p_ptr->stun + randint(33));
							}
							break;
					}

					/* Save the old location */
					oy = p_ptr->py;
					ox = p_ptr->px;

					/* Move the player to the safe location */
					p_ptr->py = sy;
					p_ptr->px = sx;

					/* Update the cave player indices */
					zcave[oy][ox].m_idx = 0;
					zcave[sy][sx].m_idx = 0 - Ind;
					cave_midx_debug(wpos, sy, sx, -Ind);

					/* Redraw the old spot */
					everyone_lite_spot(wpos, oy, ox);

					/* Redraw the new spot */
					everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);

					/* Check for new panel */
					verify_panel(Ind);
				}

				/* Important -- no wall on player */
				map[16 + p_ptr->py - cy][16 + p_ptr->px - cx] = FALSE;

				/* Take some damage */
				if (damage) {
					take_hit(Ind, damage, "an earthquake", 0);

					/* Add it tot the total damage taken - mikaelh */
					p_ptr->total_damage += damage;
				}

				/* Mega-Hack -- Forget the view and lite */
				p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

				/* Update stuff */
				p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

				/* Update the monsters */
				p_ptr->update |= (PU_DISTANCE);

				/* Update the health bar */
				p_ptr->redraw |= (PR_HEALTH);

				/* Redraw map */
				p_ptr->redraw |= (PR_MAP);

				/* Window stuff */
				p_ptr->window |= (PW_OVERHEAD);
			}
		}
	}


	/* Examine the quaked region */
	for (dy = -r; dy <= r; dy++) {
		for (dx = -r; dx <= r; dx++) {
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip unaffected grids */
			if (!map[16 + yy - cy][16 + xx - cx]) continue;

			/* Access the grid */
			c_ptr = &zcave[yy][xx];

			/* Process monsters */
			if (c_ptr->m_idx > 0) {
				monster_type *m_ptr = &m_list[c_ptr->m_idx];
                                monster_race *r_ptr = race_inf(m_ptr);

				/* Most monsters cannot co-exist with rock */
				if (!(r_ptr->flags2 & RF2_KILL_WALL) &&
				    !(r_ptr->flags2 & RF2_PASS_WALL)) {
					/* Assume not safe */
					sn = 0;

					/* Monster can move to escape the wall */
					if (!(r_ptr->flags1 & RF1_NEVER_MOVE)) {
						/* Look for safety */
						for (i = 0; i < 8; i++) {
							/* Access the grid */
							y = yy + ddy[i];
							x = xx + ddx[i];

							/* Skip non-empty grids */
							if (!cave_empty_bold(zcave, y, x)) continue;

							/* Hack -- no safety on glyph of warding */
							if (zcave[y][x].feat == FEAT_GLYPH) continue;
							if (zcave[y][x].feat == FEAT_RUNE) continue;

							/* Important -- Skip "quake" grids */
							if (map[16 + y - cy][16 + x - cx]) continue;

							/* Count "safe" grids */
							sn++;

							/* Randomize choice */
							if (rand_int(sn) > 0) continue;

							/* Save the safe grid */
							sy = y; sx = x;
						}
					}

					/* Describe the monster */
					/*monster_desc(Ind, m_name, c_ptr->m_idx, 0);*/

					/* Scream in pain */
					/*msg_format("%^s wails out in pain!", m_name);*/

					/* Take damage from the quake */
					damage = (sn ? damroll(4, 8) : 200);

					/* Monster is certainly awake */
					m_ptr->csleep = 0;

					/* Apply damage directly */
					m_ptr->hp -= damage;

					/* Delete (not kill) "dead" monsters */
					if (m_ptr->hp < 0) {
						/* Message */
						/*msg_format("%^s is embedded in the rock!", m_name);*/

						/* Delete the monster */
						delete_monster(wpos, yy, xx, TRUE);

						/* No longer safe */
						sn = 0;
					}

					/* Hack -- Escape from the rock */
					if (sn) {
						int m_idx = zcave[yy][xx].m_idx;

						/* Update the new location */
						zcave[sy][sx].m_idx = m_idx;

						/* Update the old location */
						zcave[yy][xx].m_idx = 0;

						/* Move the monster */
						m_ptr->fy = sy;
						m_ptr->fx = sx;

						/* Update the monster (new location) */
						update_mon(m_idx, TRUE);

						/* Redraw the new grid */
						everyone_lite_spot(wpos, yy, xx);

						/* Redraw the new grid */
						everyone_lite_spot(wpos, sy, sx);
					}
				}
			}
		}
	}


	/* Examine the quaked region */
	for (dy = -r; dy <= r; dy++) {
		for (dx = -r; dx <= r; dx++) {
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip unaffected grids */
			if (!map[16+yy-cy][16+xx-cx]) continue;

			/* Access the cave grid */
			c_ptr = &zcave[yy][xx];

			/* Paranoia -- never affect player */
			if (c_ptr->m_idx < 0) continue;

			/* Destroy location (if valid) */
			if (cave_valid_bold(zcave, yy, xx)) {
				bool floor = cave_floor_bold(zcave, yy, xx);

				/* Delete any object that is still there */
				delete_object(wpos, yy, xx, TRUE);

				/* Wall (or floor) type */
				t = (floor ? rand_int(100) : 200);

				/* Granite */
				if (t < 20)
					/* Create granite wall */
					cave_set_feat_live(wpos, yy, xx, FEAT_WALL_EXTRA);

				/* Quartz */
				else if (t < 70)
					/* Create quartz vein */
					cave_set_feat_live(wpos, yy, xx, FEAT_QUARTZ);

				/* Magma */
				else if (t < 100)
					/* Create magma vein */
					cave_set_feat_live(wpos, yy, xx, FEAT_MAGMA);

				/* Floor */
				else
					/* Create floor */
//uses static array set in generate.c, fix!	place_floor_live(wpos, yy, xx);
					cave_set_feat_live(wpos, yy, xx, FEAT_FLOOR);
			}
		}
	}
}

/* Wipe everything */
void wipe_spell(struct worldpos *wpos, int cy, int cx, int r) {
	int		yy, xx, dy, dx;
	cave_type	*c_ptr;

	cave_type **zcave;
	if(!(zcave = getcave(wpos))) return;
	/* Don't hurt town or surrounding areas */
	if(istownarea(wpos, MAX_TOWNAREA)) return;

	/* Paranoia -- Dnforce maximum range */
	if (r > 12) r = 12;

	/* Check around the epicenter */
	for (dy = -r; dy <= r; dy++) {
		for (dx = -r; dx <= r; dx++) {
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip illegal grids */
			if (!in_bounds(yy, xx)) continue;

			/* Skip distant grids */
			if (distance(cy, cx, yy, xx) > r) continue;

			/* Access the grid */
			c_ptr = &zcave[yy][xx];

			/* Hack -- ICKY spaces are protected outside of the dungeon */
			if (c_ptr->info & CAVE_ICKY) continue;

			/* Lose room and vault */
			c_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY | CAVE_NEST_PIT | CAVE_STCK | CAVE_ICKY_PERMA);

			/* Turn into basic floor */
			cave_set_feat(wpos, yy, xx, FEAT_FLOOR);

			/* Delete monsters */
			if (c_ptr->m_idx > 0) {
				monster_race *r_ptr = race_inf(&m_list[c_ptr->m_idx]);
				if (!(r_ptr->flags9 & RF9_IM_TELE)) delete_monster(wpos, yy, xx, TRUE);
				else continue;
			}

			/* Delete objects */
			delete_object(wpos, yy, xx, TRUE);

			everyone_lite_spot(wpos, yy, xx);
		}
	}
}


/*
 * This routine clears the entire "temp" set.
 *
 * This routine will Perma-Lite all "temp" grids.
 *
 * This routine is used (only) by "lite_room()"
 *
 * Dark grids are illuminated.
 *
 * Also, process all affected monsters.
 *
 * SMART monsters always wake up when illuminated
 * NORMAL monsters wake up 1/4 the time when illuminated
 * STUPID monsters wake up 1/10 the time when illuminated
 */
static void cave_temp_room_lite(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i;
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Clear them all */
	for (i = 0; i < p_ptr->temp_n; i++) {
		int y = p_ptr->temp_y[i];
		int x = p_ptr->temp_x[i];

		cave_type *c_ptr = &zcave[y][x];

		/* No longer in the array */
		c_ptr->info &= ~CAVE_TEMP;

		/* Update only non-CAVE_GLOW grids */
		/* if (c_ptr->info & CAVE_GLOW) continue; */

		/* Perma-Lite */
		c_ptr->info |= CAVE_GLOW;

		/* Process affected monsters */
		if (c_ptr->m_idx > 0) {
			int chance = 25;
			monster_type	*m_ptr = &m_list[c_ptr->m_idx];
			monster_race    *r_ptr = race_inf(m_ptr);

			/* Update the monster */
			update_mon(c_ptr->m_idx, FALSE);

			/* Stupid monsters rarely wake up */
			if (r_ptr->flags2 & RF2_STUPID) chance = 10;

			/* Smart monsters always wake up */
			if (r_ptr->flags2 & RF2_SMART) chance = 100;

			/* Sometimes monsters wake up */
			if (m_ptr->csleep && (rand_int(100) < chance)) {
				/* Wake up! */
				m_ptr->csleep = 0;

				/* Notice the "waking up" */
				if (p_ptr->mon_vis[c_ptr->m_idx]) {
					char m_name[MNAME_LEN];

					/* Acquire the monster name */
					monster_desc(Ind, m_name, c_ptr->m_idx, 0);

					/* Dump a message */
					msg_format(Ind, "%^s wakes up.", m_name);
				}
			}
		}

		/* Note */
		note_spot_depth(wpos, y, x);

		/* Redraw */
		everyone_lite_spot(wpos, y, x);
	}

	/* None left */
	p_ptr->temp_n = 0;
}



/*
 * This routine clears the entire "temp" set.
 *
 * This routine will "darken" all "temp" grids.
 *
 * In addition, some of these grids will be "unmarked".
 *
 * This routine is used (only) by "unlite_room()"
 *
 * Also, process all affected monsters
 */
static void cave_temp_room_unlite(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i;
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Clear them all */
	for (i = 0; i < p_ptr->temp_n; i++) {
		int y = p_ptr->temp_y[i];
		int x = p_ptr->temp_x[i];
		cave_type *c_ptr = &zcave[y][x];

		/* No longer in the array */
		c_ptr->info &= ~CAVE_TEMP;

		/* Darken the grid */
		c_ptr->info &= ~CAVE_GLOW;

		/* Hack -- Forget "boring" grids */
//		if (c_ptr->feat <= FEAT_INVIS)
		if (cave_plain_floor_grid(c_ptr)) {
			/* Forget the grid */
			p_ptr->cave_flag[y][x] &= ~CAVE_MARK;

			/* Notice */
			note_spot_depth(wpos, y, x);
		}

		/* Process affected monsters */
		if (c_ptr->m_idx > 0) {
			/* Update the monster */
			update_mon(c_ptr->m_idx, FALSE);
		}

		/* Redraw */
		everyone_lite_spot(wpos, y, x);
	}

	/* None left */
	p_ptr->temp_n = 0;
}




/*
 * Aux function -- see below
 */
static void cave_temp_room_aux(int Ind, struct worldpos *wpos, int y, int x) {
	player_type *p_ptr = Players[Ind];

	cave_type *c_ptr;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

	/* Avoid infinite recursion */
	if (c_ptr->info & CAVE_TEMP) return;

	/* Do not "leave" the current room */
	if (!(c_ptr->info & CAVE_ROOM)) return;

	/* Paranoia -- verify space */
	if (p_ptr->temp_n == TEMP_MAX) return;

	/* Mark the grid as "seen" */
	c_ptr->info |= CAVE_TEMP;

	/* Add it to the "seen" set */
	p_ptr->temp_y[p_ptr->temp_n] = y;
	p_ptr->temp_x[p_ptr->temp_n] = x;
	p_ptr->temp_n++;
}




/*
 * Illuminate any room containing the given location.
 */
void lite_room(int Ind, struct worldpos *wpos, int y1, int x1) {
	player_type *p_ptr = Players[Ind];
	int i, x, y;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Add the initial grid */
	cave_temp_room_aux(Ind, wpos, y1, x1);

	/* While grids are in the queue, add their neighbors */
	for (i = 0; i < p_ptr->temp_n; i++) {
		x = p_ptr->temp_x[i], y = p_ptr->temp_y[i];

		/* Walls get lit, but stop light */
		if (!cave_floor_bold(zcave, y, x)) continue;

		/* Spread adjacent */
		cave_temp_room_aux(Ind, wpos, y + 1, x);
		cave_temp_room_aux(Ind, wpos, y - 1, x);
		cave_temp_room_aux(Ind, wpos, y, x + 1);
		cave_temp_room_aux(Ind, wpos, y, x - 1);

		/* Spread diagonal */
		cave_temp_room_aux(Ind, wpos, y + 1, x + 1);
		cave_temp_room_aux(Ind, wpos, y - 1, x - 1);
		cave_temp_room_aux(Ind, wpos, y - 1, x + 1);
		cave_temp_room_aux(Ind, wpos, y + 1, x - 1);
	}

	/* Now, lite them all up at once */
	cave_temp_room_lite(Ind);
}


/*
 * Darken all rooms containing the given location
 */
void unlite_room(int Ind, struct worldpos *wpos, int y1, int x1) {
	player_type *p_ptr = Players[Ind];
	int i, x, y;

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	/* Add the initial grid */
	cave_temp_room_aux(Ind, wpos, y1, x1);

	/* Spread, breadth first */
	for (i = 0; i < p_ptr->temp_n; i++) {
		x = p_ptr->temp_x[i], y = p_ptr->temp_y[i];

		/* Walls get dark, but stop darkness */
		if (!cave_floor_bold(zcave, y, x)) continue;

		/* Spread adjacent */
		cave_temp_room_aux(Ind, wpos, y + 1, x);
		cave_temp_room_aux(Ind, wpos, y - 1, x);
		cave_temp_room_aux(Ind, wpos, y, x + 1);
		cave_temp_room_aux(Ind, wpos, y, x - 1);

		/* Spread diagonal */
		cave_temp_room_aux(Ind, wpos, y + 1, x + 1);
		cave_temp_room_aux(Ind, wpos, y - 1, x - 1);
		cave_temp_room_aux(Ind, wpos, y - 1, x + 1);
		cave_temp_room_aux(Ind, wpos, y + 1, x - 1);
	}

	/* Now, darken them all at once */
	cave_temp_room_unlite(Ind);
}



/*
 * Hack -- call light around the player
 * Affect all monsters in the projection radius
 */
bool lite_area(int Ind, int dam, int rad) {
	player_type *p_ptr = Players[Ind];
	int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_KILL | PROJECT_NODO | PROJECT_NODF;

	/* WRAITHFORM reduces damage/effect! */
	if (p_ptr->tim_wraith) proj_dam_wraith(GF_LITE_WEAK, &dam);

	/* Hack -- Message */
	if (!p_ptr->blind)
		msg_print(Ind, "You are surrounded by a white light.");

	/* Hook into the "project()" function */
	(void)project(0 - Ind, rad, &p_ptr->wpos, p_ptr->py, p_ptr->px, dam, GF_LITE_WEAK, flg, "");

	/* Lite up the room */
	lite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);

	/* Assume seen */
	return (TRUE);
}


/*
 * Hack -- call darkness around the player
 * Affect all monsters in the projection radius
 */
bool unlite_area(int Ind, int dam, int rad) {
	player_type *p_ptr = Players[Ind];
	int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_KILL | PROJECT_NODO | PROJECT_NODF;

	/* WRAITHFORM reduces damage/effect! */
	if (p_ptr->tim_wraith) proj_dam_wraith(GF_DARK_WEAK, &dam);

	/* Hack -- Message */
	if (!p_ptr->blind) msg_print(Ind, "Darkness surrounds you.");

	/* Hook into the "project()" function */
	(void)project(0 - Ind, rad, &p_ptr->wpos, p_ptr->py, p_ptr->px, dam, GF_DARK_WEAK, flg, "");

	/* Lite up the room */
	unlite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);

	/* Assume seen */
	return (TRUE);
}



/*
 * Cast a ball spell
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 */
bool fire_ball(int Ind, int typ, int dir, int dam, int rad, char *attacker) {
	player_type *p_ptr = Players[Ind];
	char pattacker[80];
	int tx, ty;

	int flg = PROJECT_NORF | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO;

	/* WRAITHFORM reduces damage/effect */
	if (p_ptr->tim_wraith) proj_dam_wraith(typ, &dam);

	/* Use the given direction */
	tx = p_ptr->px + 99 * ddx[dir];
	ty = p_ptr->py + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay(Ind)) {
		flg &= ~PROJECT_STOP;
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}
#if 1
#ifdef USE_SOUND_2010
	if (typ == GF_ROCKET) sound(Ind, "rocket", NULL, SFX_TYPE_COMMAND, FALSE);
	else if (typ == GF_DETONATION) sound(Ind, "detonation", NULL, SFX_TYPE_COMMAND, FALSE);
	else if (typ == GF_STONE_WALL) sound(Ind, "stone_wall", NULL, SFX_TYPE_COMMAND, FALSE);
	else {
		/* The 'cast_ball' sound is only for attack spells */
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
		    (typ != GF_HERO_PLAYER) && (typ != GF_SHERO_PLAYER) &&
		    (typ != GF_TELEPORT_PLAYER) && (typ != GF_ZEAL_PLAYER) &&
		    (typ != GF_RESTORE_PLAYER) && (typ != GF_REMCURSE_PLAYER) &&
		    (typ != GF_CURE_PLAYER) && (typ != GF_RESURRECT_PLAYER) &&
		    (typ != GF_SANITY_PLAYER) && (typ != GF_SOULCURE_PLAYER) &&
		    (typ != GF_OLD_HEAL) && (typ != GF_OLD_SPEED) && (typ != GF_PUSH) &&
		    (typ != GF_HEALINGCLOUD) && /* Also not a hostile spell */
		    (typ != GF_EXTRA_STATS) &&
		    (typ != GF_MINDBOOST_PLAYER) && (typ != GF_IDENTIFY) &&
		    (typ != GF_SLOWPOISON_PLAYER) && (typ != GF_CURING) &&
		    (typ != GF_OLD_POLY)) /* Non-hostile players may polymorph each other */
			if (p_ptr->sfx_magicattack) sound(Ind, "cast_ball", NULL, SFX_TYPE_COMMAND, TRUE);
	}
#endif
#endif
	/* Analyze the "dir" and the "target".  Hurt items on floor. */
	snprintf(pattacker, 80, "%s%s", p_ptr->name, attacker);

	/* affect self + players + monsters AND give credit on kill */
	flg = mod_ball_spell_flags(typ, flg);
	return (project(0 - Ind, rad, &p_ptr->wpos, ty, tx, dam, typ, flg, pattacker));
}

/*
 * Cast a full ball spell
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 */
bool fire_full_ball(int Ind, int typ, int dir, int dam, int rad, char *attacker) {
	player_type *p_ptr = Players[Ind];
	char pattacker[80];
	int tx, ty;

	int flg = PROJECT_NORF | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_FULL | PROJECT_NODO;


	/* WRAITHFORM reduces damage/effect! */
	if (p_ptr->tim_wraith) proj_dam_wraith(typ, &dam);

	/* Use the given direction */
	tx = p_ptr->px + 99 * ddx[dir];
	ty = p_ptr->py + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay(Ind)) {
		flg &= ~(PROJECT_STOP);
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}
#if 1
#ifdef USE_SOUND_2010
	if (typ == GF_ROCKET) sound(Ind, "rocket", NULL, SFX_TYPE_COMMAND, FALSE);
	else if (typ == GF_DETONATION) sound(Ind, "detonation", NULL, SFX_TYPE_COMMAND, FALSE);
	else if (typ == GF_STONE_WALL) sound(Ind, "stone_wall", NULL, SFX_TYPE_COMMAND, FALSE);
	else {
		/* The 'cast_ball' sound is only for attack spells */
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
		    (typ != GF_HERO_PLAYER) && (typ != GF_SHERO_PLAYER) &&
		    (typ != GF_TELEPORT_PLAYER) && (typ != GF_ZEAL_PLAYER) &&
		    (typ != GF_RESTORE_PLAYER) && (typ != GF_REMCURSE_PLAYER) &&
		    (typ != GF_CURE_PLAYER) && (typ != GF_RESURRECT_PLAYER) &&
		    (typ != GF_SANITY_PLAYER) && (typ != GF_SOULCURE_PLAYER) &&
		    (typ != GF_OLD_HEAL) && (typ != GF_OLD_SPEED) && (typ != GF_PUSH) &&
		    (typ != GF_HEALINGCLOUD) && /* Also not a hostile spell */
		    (typ != GF_EXTRA_STATS) &&
		    (typ != GF_MINDBOOST_PLAYER) && (typ != GF_IDENTIFY) &&
		    (typ != GF_SLOWPOISON_PLAYER) && (typ != GF_CURING) &&
		    (typ != GF_OLD_POLY)) /* Non-hostile players may polymorph each other */
			if (p_ptr->sfx_magicattack) sound(Ind, "cast_ball", NULL, SFX_TYPE_COMMAND, TRUE);
	}
#endif
#endif
	/* Analyze the "dir" and the "target".  Hurt items on floor. */
	snprintf(pattacker, 80, "%s%s", p_ptr->name, attacker);

	/* affect self + players + monsters AND give credit on kill */
	flg = mod_ball_spell_flags(typ, flg);
	return (project(0 - Ind, rad, &p_ptr->wpos, ty, tx, dam, typ, flg, pattacker));
}

/*
 * Cast N radius 1 ball spells, don't pass over target
 * In "target" mode, ball centers have a radius 1 spread
 * In "direction" mode, ball centers deal half damage
 * This preserves the intended damage range of the spell type - Kurzel
 */
bool fire_swarm(int Ind, int typ, int dir, int dam, int num, char *attacker) {
	player_type *p_ptr = Players[Ind];
	char pattacker[80];
	int tx, ty;

	int flg = PROJECT_NORF | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO;
	//flg &= ~PROJECT_STOP; //Never pass over monsters, actually increases the chance of a direct hit to about 3 in 9. - Kurzel

	/* Prepare to loop */
	byte i;
	int spread[3] = {-1, 0, 1};


	/* WRAITHFORM reduces damage/effect */
	if (p_ptr->tim_wraith) proj_dam_wraith(typ, &dam);

	/* Use the given direction */
	tx = p_ptr->px + 99 * ddx[dir];
	ty = p_ptr->py + 99 * ddy[dir];

	/* affect self + players + monsters AND give credit on kill */
	flg = mod_ball_spell_flags(typ, flg);

	if (!((dir == 5) && target_okay(Ind))) { //Enforce minimum main target damage if the swarm doesn't spread. - Kurzel
		dam /= 2;
		flg |= PROJECT_FULL;
	}

	for (i = 0; i < num; i++) {
		/* Hack -- Use an actual "target", but modify tx/ty such that each ball likely falls 'off-target' - Kurzel */
		// For monsters surrounded by walls or if the player is adjacent to walls, some balls may miss completely:
		// ###x... or ###x#.. or even ###x###
		// xD...@.    xD.....         .D...@.
		// ###x...    #.x..@.         x.x....
		if ((dir == 5) && target_okay(Ind)) {
			tx = p_ptr->target_col + spread[rand_int(3)]; //rand_int() gives 0-2
			ty = p_ptr->target_row + spread[rand_int(3)]; //randint() gives 1-3 (sheesh, rename these? - Kurzel)
		}
	#if 1
	#ifdef USE_SOUND_2010
		if (typ == GF_ROCKET) sound(Ind, "rocket", NULL, SFX_TYPE_COMMAND, FALSE);
		else if (typ == GF_DETONATION) sound(Ind, "detonation", NULL, SFX_TYPE_COMMAND, FALSE);
		else if (typ == GF_STONE_WALL) sound(Ind, "stone_wall", NULL, SFX_TYPE_COMMAND, FALSE);
		else {
			/* The 'cast_ball' sound is only for attack spells */
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
			    (typ != GF_HERO_PLAYER) && (typ != GF_SHERO_PLAYER) &&
			    (typ != GF_TELEPORT_PLAYER) && (typ != GF_ZEAL_PLAYER) &&
			    (typ != GF_RESTORE_PLAYER) && (typ != GF_REMCURSE_PLAYER) &&
			    (typ != GF_CURE_PLAYER) && (typ != GF_RESURRECT_PLAYER) &&
			    (typ != GF_SANITY_PLAYER) && (typ != GF_SOULCURE_PLAYER) &&
			    (typ != GF_OLD_HEAL) && (typ != GF_OLD_SPEED) && (typ != GF_PUSH) &&
			    (typ != GF_HEALINGCLOUD) && /* Also not a hostile spell */
			    (typ != GF_EXTRA_STATS) &&
			    (typ != GF_MINDBOOST_PLAYER) && (typ != GF_IDENTIFY) &&
			    (typ != GF_SLOWPOISON_PLAYER) && (typ != GF_CURING) &&
			    (typ != GF_OLD_POLY)) /* Non-hostile players may polymorph each other */
				if (p_ptr->sfx_magicattack) sound(Ind, "cast_ball", NULL, SFX_TYPE_COMMAND, TRUE);
		}
	#endif
	#endif
		/* Analyze the "dir" and the "target".  Hurt items on floor. */
		snprintf(pattacker, 80, "%s%s", p_ptr->name, attacker);

		project(0 - Ind, 1, &p_ptr->wpos, ty, tx, dam, typ, flg, pattacker); //Always radius 1, probably don't want to spam project() with larger. - Kurzel
	}

	return (TRUE); //mh
}

/*
 * Cast a cloud spell
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 */
bool fire_cloud(int Ind, int typ, int dir, int dam, int rad, int time, int interval, char *attacker) {
	player_type *p_ptr = Players[Ind];
	int tx, ty;

	int flg = PROJECT_NORF | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_STAY | PROJECT_NODF | PROJECT_NODO;

	char pattacker[80];

	/* WRAITHFORM reduces damage/effect! */
	if (p_ptr->tim_wraith) proj_dam_wraith(typ, &dam);

	/* Use the given direction */
	tx = p_ptr->px + 99 * ddx[dir];
	ty = p_ptr->py + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay(Ind)) {
		flg &= ~(PROJECT_STOP);
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}
	project_interval = interval;
	project_time = time;

	if (snprintf(pattacker, 80, "%s%s", Players[Ind]->name, attacker) < 0) return (FALSE);

	/* Analyze the "dir" and the "target".  Hurt items on floor. */

#ifdef USE_SOUND_2010
	/* paranoia, aka this won't exist as "clouds".. */
	if (typ == GF_ROCKET) sound(Ind, "rocket", NULL, SFX_TYPE_COMMAND, FALSE);
	else if (typ == GF_DETONATION) sound(Ind, "detonation", NULL, SFX_TYPE_COMMAND, FALSE);
	else if (typ == GF_STONE_WALL) sound(Ind, "stone_wall", NULL, SFX_TYPE_COMMAND, FALSE);
	/* only this one needed really */
	else if (p_ptr->sfx_magicattack) sound(Ind, "cast_cloud", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

	flg = mod_ball_spell_flags(typ, flg);
	return (project(0 - Ind, (rad > 16) ? 16 : rad, &p_ptr->wpos, ty, tx, dam, typ, flg, pattacker));
}

/*
 * Cast a wave spell
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 */
bool fire_wave(int Ind, int typ, int dir, int dam, int rad, int time, int interval, s32b eff, char *attacker) {
	char pattacker[80];

	project_time_effect = eff;
	snprintf(pattacker, 80, "%s%s", Players[Ind]->name, attacker);
	return (fire_cloud(Ind, typ, dir, dam, rad, time, interval, pattacker));
}

/*
 * Singing in the rain :-o
 */
bool cast_raindrop(worldpos *wpos, int x) {
	char pattacker[80];
	strcpy(pattacker, "");
	int pseudo_y_start;

	int flg = PROJECT_DUMY | PROJECT_GRID | PROJECT_STAY;

	project_time_effect = EFF_RAINING;
	project_interval = 3;
	/* let more drops appear at top line, simulating that they were
	   created 'above' the screen, so we don't get quite empty top lines */
	pseudo_y_start = rand_int(66 - 1 + 20) - 20;
	project_time = pseudo_y_start < 0 ? 20 + pseudo_y_start : 20;
	if (pseudo_y_start < 0) pseudo_y_start = 0;
	return (project(PROJECTOR_EFFECT, 0, wpos, pseudo_y_start, x, 0, GF_RAINDROP, flg, pattacker));

	//project_time = 20;//1 + randint(66 - 3);
	//return (project(PROJECTOR_EFFECT, 0, wpos, 1 + randint(66 - 3), x, 0, GF_RAINDROP, flg, pattacker));
}

/*
 * Let it snow, let it snow, let it snow... (WINTER_SEASON)
 * 'interval' is movement speed
 */
bool cast_snowflake(worldpos *wpos, int x, int interval) {
	char pattacker[80];
	strcpy(pattacker, "");

	int flg = PROJECT_DUMY | PROJECT_GRID | PROJECT_STAY;

	project_time_effect = EFF_SNOWING;
	project_interval = interval;
	project_time = 100; /* just any value long enough to traverse the screen */

	return (project(PROJECTOR_EFFECT, 0, wpos, 0, x, 0, GF_SNOWFLAKE, flg, pattacker));
}

/*
 * Fireworks! (NEW_YEARS_EVE)
 */
bool cast_fireworks(worldpos *wpos, int x, int y, int typ) {
	char pattacker[80];
	strcpy(pattacker, "");

	int flg = PROJECT_DUMY | PROJECT_GRID | PROJECT_STAY;

	if (typ == -1) typ = rand_int(7 * 3); /* colour & style */

	if (typ < 7) project_time_effect = EFF_FIREWORKS1;
	else if (typ < 7 * 2) {
		project_time_effect = EFF_FIREWORKS2;
		typ -= 7;
	} else {
		project_time_effect = EFF_FIREWORKS3;
		typ -= 7 * 2;
	}

	switch (typ) {
	case 0: typ = GF_FW_FIRE; break;
	case 1: typ = GF_FW_ELEC; break;
	case 2: typ = GF_FW_POIS; break;
	case 3: typ = GF_FW_LITE; break;
	case 4: typ = GF_FW_SHDI; break;
	case 5: typ = GF_FW_SHDM; break;
	case 6: typ = GF_FW_MULT; break;
	}

#if 0
	project_interval = 4;
	project_time = 5 + 5; /* X units to rise into the air, X units to explode */
#else
	/* Adjustments - mikaelh */
	project_interval = 5;
	project_time = 8 + 8; /* X units to rise into the air, X units to explode */
	//if (project_time_effect == EFF_FIREWORKS3) project_time += 2 + 2;
#endif

	return (project(PROJECTOR_EFFECT, 0, wpos, y, x, 0, typ, flg, pattacker)); /* typ -> colour */
}

/* For Nether Realm collapse ;) - C. Blue */
bool cast_lightning(worldpos *wpos, int x, int y) {
	char pattacker[80];
	int flg = PROJECT_DUMY | PROJECT_GRID | PROJECT_STAY;
	int typ = rand_int(3 * 2); /* style / mirrored direction? */

	strcpy(pattacker, "");

	if (typ < 2) {
		project_time_effect = EFF_LIGHTNING1;
		project_time = 14;
	} else if (typ < 2 * 2) {
		project_time_effect = EFF_LIGHTNING2;
		project_time = 8;
		typ -= 2;
	} else {
		project_time_effect = EFF_LIGHTNING3;
		project_time = 10;
		typ -= 2 * 2;
	}

	project_interval = 1;
	project_time += 10; /* afterglow */

#ifdef USE_SOUND_2010
	sound_floor_vol(wpos, "thunder", NULL, SFX_TYPE_AMBIENT, 100);
#endif

	return (project(PROJECTOR_EFFECT, 0, wpos, y, x, typ, GF_SHOW_LIGHTNING, flg, pattacker));
}

/* For reworked Thunderstorm spell */
bool thunderstorm_visual(worldpos *wpos, int x, int y) {
	char pattacker[80];
	int flg = PROJECT_DUMY | PROJECT_GRID | PROJECT_STAY;

	strcpy(pattacker, "");

	project_time_effect = EFF_THUNDER_VISUAL;
	// 25,1; 5,3
	project_time = 5;
	project_interval = 3;

	return (project(PROJECTOR_EFFECT, 0, wpos, y, x, 0, GF_THUNDER_VISUAL, flg, pattacker));
}


/*
 * Player swaps position with whatever in (lty, ltx)
 * usually used for 'Void Jumpgate'		- Jir -
 */
/*
 * Fixed monster swapping bug which caused many
 * unfair deaths and weirdness.
 * (evileye)
 */
bool swap_position(int Ind, int lty, int ltx){
	player_type *p_ptr;
	worldpos *wpos;
	int tx, ty;
	cave_type *c_ptr;
	cave_type **zcave;
#ifdef ENABLE_SELF_FLASHING
	bool panel;
#endif

	p_ptr = Players[Ind];
	if (!p_ptr) return FALSE;

	wpos = &p_ptr->wpos;
	if (!(zcave = getcave(wpos))) return FALSE;

	store_exit(Ind);

	c_ptr = &zcave[lty][ltx];

	/* Keep track of the old location */
	tx = p_ptr->px;
	ty = p_ptr->py;

	/* Move the player */
	p_ptr->px = ltx;
	p_ptr->py = lty;

	if (!c_ptr->m_idx) {
		/* Free space */
		/* Update the old location */
		zcave[ty][tx].m_idx = 0;
		c_ptr->m_idx = 0 - Ind;
		cave_midx_debug(wpos, lty, ltx, -Ind);

		/* Redraw/remember old location */
		note_spot_depth(wpos, ty, tx);
		everyone_lite_spot(wpos, ty, tx);

		/* Redraw new grid */
		everyone_lite_spot(wpos, lty, ltx);
	} else if (c_ptr->m_idx > 0) {
		/* Monster */
		monster_type *m_ptr = &m_list[c_ptr->m_idx];
		zcave[ty][tx].m_idx = c_ptr->m_idx;

		/* Move the monster */
		m_ptr->fy = ty;
		m_ptr->fx = tx;

		/* Update the new location */
		c_ptr->m_idx = 0 - Ind;
		cave_midx_debug(wpos, lty, ltx, -Ind);

		/* Update monster (new location) */
		update_mon(zcave[ty][tx].m_idx, TRUE);

		/* Redraw/remember old location */
		note_spot_depth(wpos, ty, tx);
		everyone_lite_spot(wpos, ty, tx);

		/* Redraw new grid */
		everyone_lite_spot(wpos, lty, ltx);
	} else {//if (c_ptr->m_idx < 0) {
		/* Other player */
		int Ind2 = 0 - c_ptr->m_idx;
		player_type *q_ptr = NULL;
		if (Ind2) q_ptr = Players[Ind2];

		/* Shift them if they are real */
		if (q_ptr) {
			store_exit(Ind2);

			q_ptr->py = ty;
			q_ptr->px = tx;
			#ifdef ARCADE_SERVER
			if (p_ptr->game == 3) {
			p_ptr->arc_c = q_ptr->arc_c = 1;
			p_ptr->arc_d = Ind2; }
			#endif
		}

		p_ptr->px = ltx;
		p_ptr->py = lty;

		/* Update old player location */
		c_ptr->m_idx = 0 - Ind;
		cave_midx_debug(wpos, lty, ltx, -Ind);
		zcave[ty][tx].m_idx = 0 - Ind2;
		cave_midx_debug(wpos, ty, tx, -Ind2);

		/* Redraw/remember old location */
		note_spot_depth(wpos, ty, tx);
		everyone_lite_spot(wpos, ty, tx);

		/* Redraw new grid */
		everyone_lite_spot(wpos, lty, ltx);

		if (Ind2) verify_panel(Ind2);
		if (q_ptr) {
			/* Update stuff */
			q_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_DISTANCE);

			/* Update Window */
			q_ptr->window |= (PW_OVERHEAD);

			/* Handle stuff */
			if (!q_ptr->death) handle_stuff(Ind);
		}
	}

	/* Check for new panel (redraw map) */
#ifdef ENABLE_SELF_FLASHING
	panel = !local_panel(Ind);
#endif
	verify_panel(Ind);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_DISTANCE);

	/* Update Window */
	p_ptr->window |= (PW_OVERHEAD);

	/* Handle stuff */
	if (!p_ptr->death) handle_stuff(Ind);

	return panel;
}


/*
 * Hack -- apply a "projection()" in a direction (or at the target)
 */
bool project_hook(int Ind, int typ, int dir, int dam, int flg, char *attacker) {
	player_type *p_ptr = Players[Ind];
	int tx, ty;

	/* WRAITHFORM reduces damage/effect! */
	if (p_ptr->tim_wraith) proj_dam_wraith(typ, &dam);

	/* Pass through the target if needed */
	flg |= (PROJECT_THRU);

	/* Use the given direction */
	tx = p_ptr->px + ddx[dir];
	ty = p_ptr->py + ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay(Ind)) {
		//cave_type **zcave = getcave(&p_ptr->wpos);

		tx = p_ptr->target_col;
		ty = p_ptr->target_row;

#if 0 /* oops, nonsense >_< this only concerns PROJECT_GRID/PROJECT_ITEM flags of course.. */
		/* Allow targetting the item/floor of a specific grid with a bolt spell
		   ..if no monster on it! Assume that the floor won't get hit if the monster
		     instead gets hit, basically. Makes sense for bolt spells and prevents
		     too much loot destruction. */
		if (!(flg & PROJECT_BEAM) &&
		    zcave && /* paranoia */
		    zcave[ty][tx].m_idx == 0)
			flg &= ~PROJECT_THRU;
#else
		if (!(flg & PROJECT_BEAM)) flg &= ~PROJECT_THRU;
#endif
	}

	/* Analyze the "dir" and the "target", do NOT explode */
	return (project(0 - Ind, 0, &p_ptr->wpos, ty, tx, dam, typ, flg, attacker));
}


/*
 * Cast a bolt spell
 * Stop if we hit a monster, as a "bolt"
 * Affect monsters (not grids or objects)
 */
bool fire_bolt(int Ind, int typ, int dir, int dam, char *attacker) {
	char pattacker[80];
	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID | PROJECT_EVSG;
	snprintf(pattacker, 80, "%s%s", Players[Ind]->name, attacker);

#ifdef USE_SOUND_2010
	if (Players[Ind]->sfx_magicattack) sound(Ind, "cast_bolt", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

	return (project_hook(Ind, typ, dir, dam, flg, pattacker));
}

/*
 * Cast a beam spell
 * Pass through monsters, as a "beam"
 * Affect monsters (not grids or objects)
 */
bool fire_beam(int Ind, int typ, int dir, int dam, char *attacker) {
	char pattacker[80];
		//int flg = PROJECT_BEAM | PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID;
		//Actually, since beams affect the whole tile, don't deflect or reflect them (we don't have proper code for changing beam path anyway) - Kurzel
	int flg = PROJECT_NORF | PROJECT_BEAM | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM | PROJECT_NODF | PROJECT_NODO;
		snprintf(pattacker, 80, "%s%s", Players[Ind]->name, attacker);

#ifdef USE_SOUND_2010
	if (Players[Ind]->sfx_magicattack) sound(Ind, "cast_beam", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

	return (project_hook(Ind, typ, dir, dam, flg, pattacker));
}

/*
 * Cast a cloud spell
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 */
bool fire_wall(int Ind, int typ, int dir, int dam, int time, int interval, char *attacker) {
	player_type *p_ptr = Players[Ind];
	int tx, ty;

	int flg = PROJECT_NORF | PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_STAY | PROJECT_THRU | PROJECT_NODF | PROJECT_NODO;

	/* WRAITHFORM reduces damage/effect! */
	if (p_ptr->tim_wraith) proj_dam_wraith(typ, &dam);

	/* Use the given direction */
	tx = p_ptr->px + ddx[dir];
	ty = p_ptr->py + ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay(Ind)) {
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}
	project_time_effect = EFF_WALL;
	project_interval = interval;
	project_time = time;

#ifdef USE_SOUND_2010
	sound(Ind, "cast_wall", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

	/* Analyze the "dir" and the "target", do NOT explode */
	return (project(0 - Ind, 0, &p_ptr->wpos, ty, tx, dam, typ, flg, attacker));
}

/*
 * Cast a bolt spell, or rarely, a beam spell
 */
bool fire_bolt_or_beam(int Ind, int prob, int typ, int dir, int dam, char *attacker) {
#if 0 /* too dangerous in case it wakes up more monsters? */
	if (rand_int(100) < prob)
		return (fire_beam(Ind, typ, dir, dam, attacker));
	else
		return (fire_bolt(Ind, typ, dir, dam, attacker));
#else
	return (fire_bolt(Ind, typ, dir, dam, attacker));
#endif
}

/* Target bolt-like, but able to pass 'over' untargetted enemies to hit target grid,
   ie it manifests directly at the target location (like a ball of radius 0).
   Added for new mindcrafter spells; sort of a fake PROJECT_JUMP. - C. Blue */
bool fire_grid_bolt(int Ind, int typ, int dir, int dam, char *attacker) {
	player_type *p_ptr = Players[Ind];
	char pattacker[80];
	int tx, ty;

	int flg = PROJECT_NORF | PROJECT_HIDE | PROJECT_STOP | PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID | PROJECT_EVSG | PROJECT_NODF | PROJECT_NODO;

	/* WRAITHFORM reduces damage/effect! */
	if (p_ptr->tim_wraith) proj_dam_wraith(typ, &dam);

	/* Use the given direction */
	tx = p_ptr->px + 99 * ddx[dir];
	ty = p_ptr->py + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay(Ind)) {
		flg &= ~PROJECT_STOP;
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}

#ifdef USE_SOUND_2010
	if (p_ptr->sfx_magicattack) sound(Ind, "cast_bolt", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

	/* Analyze the "dir" and the "target".  Hurt items on floor. */
	snprintf(pattacker, 80, "%s%s", p_ptr->name, attacker);

	if (typ == GF_HEAL_PLAYER) flg |= PROJECT_PLAY; //mod_ball_spell_flags(typ, flg);
	return (project(0 - Ind, 0, &p_ptr->wpos, ty, tx, dam, typ, flg, attacker));
}

/* Added for new mindcrafter spells.
   The '_grid_' part here means that the beam also hits the floor grids,
   ie feat/terrain/item! Added for mindcrafter disarming. - C. Blue */
bool fire_grid_beam(int Ind, int typ, int dir, int dam, char *attacker) {
	player_type *p_ptr = Players[Ind];
	char pattacker[80];

	// (project_grid is required for disarm beam! same for project_item, for chests!)
	int flg = PROJECT_NORF | PROJECT_BEAM | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM | PROJECT_NODF | PROJECT_NODO;
	//flg |= PROJECT_STOP | PROJECT_HIDE;

	/* Analyze the "dir" and the "target".  Hurt items on floor. */
	snprintf(pattacker, 80, "%s%s", p_ptr->name, attacker);

#ifdef USE_SOUND_2010
	if (p_ptr->sfx_magicattack) sound(Ind, "cast_beam", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

#if 0 /* why needed for beam? */
	/* Use the given direction */
	int tx = p_ptr->px + 99 * ddx[dir], ty = p_ptr->py + 99 * ddy[dir];

	/* WRAITHFORM reduces damage/effect! */
	if (p_ptr->tim_wraith) proj_dam_wraith(typ, &dam);

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay(Ind)) {
		flg &= ~PROJECT_STOP;
		tx = p_ptr->target_col;
		ty = p_ptr->target_row;
	}

	return (project(0 - Ind, 0, &p_ptr->wpos, ty, tx, dam, typ, flg, attacker));
#else
	return (project_hook(Ind, typ, dir, dam, flg, pattacker));
#endif

}


/*
 * Some of the old functions
 */
/* TODO: the result should be affected by skills (and not plev) */

bool lite_line(int Ind, int dir, int dam, bool starlight) {
	int flg = PROJECT_NORF | PROJECT_BEAM | PROJECT_GRID | PROJECT_KILL | PROJECT_NODF | PROJECT_NODO;
	return (project_hook(Ind, starlight? GF_STARLITE : GF_LITE_WEAK, dir, dam, flg, ""));
}

bool drain_life(int Ind, int dir, int dam) {
	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;
	return(project_hook(Ind, GF_OLD_DRAIN, dir, dam, flg, ""));
}

bool annihilate(int Ind, int dir, int dam) {
	//int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO; //old
	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO | PROJECT_ITEM | PROJECT_GRID | PROJECT_EVSG;
	return(project_hook(Ind, GF_ANNIHILATION, dir, dam, flg, ""));
}

bool wall_to_mud(int Ind, int dir) {
	int flg = PROJECT_NORF | PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODF | PROJECT_NODO;
	return (project_hook(Ind, GF_KILL_WALL, dir, 20 + randint(30), flg, ""));
}

bool destroy_trap_door(int Ind, int dir) {
	int flg = PROJECT_NORF | PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_NODF | PROJECT_NODO;
	return (project_hook(Ind, GF_KILL_TRAP_DOOR, dir, 0, flg, ""));
}

bool disarm_trap_door(int Ind, int dir) {
	int flg = PROJECT_NORF | PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_NODF | PROJECT_NODO;
	return (project_hook(Ind, GF_KILL_TRAP, dir, 0, flg, ""));
}

bool heal_monster(int Ind, int dir) {
	int flg = PROJECT_STOP | PROJECT_KILL;
	snprintf(Players[Ind]->attacker, sizeof(Players[Ind]->attacker), "%s heals you for", Players[Ind]->name);
	return (project_hook(Ind, GF_OLD_HEAL, dir, damroll(4, 6), flg, Players[Ind]->attacker));
}

bool speed_monster(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;
	return (project_hook(Ind, GF_OLD_SPEED, dir, p_ptr->lev, flg, ""));
}

bool slow_monster(int Ind, int dir, int pow) {
	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;
	return (project_hook(Ind, GF_OLD_SLOW, dir, pow, flg, ""));
}

bool sleep_monster(int Ind, int dir, int pow) {
	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;
	return (project_hook(Ind, GF_OLD_SLEEP, dir, pow, flg, ""));
}

bool confuse_monster(int Ind, int dir, int pow) {
	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;
	return (project_hook(Ind, GF_OLD_CONF, dir, pow, flg, ""));
}

bool poly_monster(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_SELF | PROJECT_PLAY | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;
	return (project_hook(Ind, GF_OLD_POLY, dir, p_ptr->lev, flg, ""));
}

bool clone_monster(int Ind, int dir) {
	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;
	return (project_hook(Ind, GF_OLD_CLONE, dir, 0, flg, ""));
}

bool fear_monster(int Ind, int dir, int pow) {
	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;
	return (project_hook(Ind, GF_TURN_ALL, dir, pow, flg, ""));
}

bool teleport_monster(int Ind, int dir) {
	int flg = PROJECT_BEAM | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;
	return (project_hook(Ind, GF_AWAY_ALL, dir, MAX_SIGHT * 5, flg, ""));
}

bool cure_light_wounds_proj(int Ind, int dir) {
	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;
	snprintf(Players[Ind]->attacker, sizeof(Players[Ind]->attacker), "%s heals you for", Players[Ind]->name);
	return (project_hook(Ind, GF_HEAL_PLAYER, dir, damroll(2, 10), flg, Players[Ind]->attacker));
}

bool cure_serious_wounds_proj(int Ind, int dir) {
	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;
	snprintf(Players[Ind]->attacker, sizeof(Players[Ind]->attacker),"%s heals you for", Players[Ind]->name);
	return (project_hook(Ind, GF_HEAL_PLAYER, dir, damroll(4, 10), flg, Players[Ind]->attacker));
}

bool cure_critical_wounds_proj(int Ind, int dir) {
	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;
	snprintf(Players[Ind]->attacker, sizeof(Players[Ind]->attacker),"%s heals you for", Players[Ind]->name);
	return (project_hook(Ind, GF_HEAL_PLAYER, dir, damroll(6, 10), flg, Players[Ind]->attacker));
}

bool heal_other_proj(int Ind, int dir) {
	int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_NORF | PROJECT_NODF | PROJECT_NODO;
	snprintf(Players[Ind]->attacker, sizeof(Players[Ind]->attacker),"%s heals you for", Players[Ind]->name);
	return (project_hook(Ind, GF_HEAL_PLAYER, dir, 100, flg, Players[Ind]->attacker));
}



/*
 * Hooks -- affect adjacent grids (radius 1 ball attack)
 */

bool door_creation(int Ind) {
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE | PROJECT_NODF | PROJECT_NODO;
	return (project(0 - Ind, 1, &p_ptr->wpos, p_ptr->py, p_ptr->px, 0, GF_MAKE_DOOR, flg, ""));
}

bool trap_creation(int Ind, int mod, int rad) {
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE | PROJECT_NODF | PROJECT_NODO;
	return (project(0 - Ind, rad, &p_ptr->wpos, p_ptr->py, p_ptr->px, mod, GF_MAKE_TRAP, flg, ""));
}

bool destroy_doors_touch(int Ind, int rad) {
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE | PROJECT_NODF | PROJECT_NODO;
	return (project(0 - Ind, rad, &p_ptr->wpos, p_ptr->py, p_ptr->px, 0, GF_KILL_DOOR, flg, ""));
}
bool destroy_traps_touch(int Ind, int rad) {
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE | PROJECT_NODF | PROJECT_NODO;
	return (project(0 - Ind, rad, &p_ptr->wpos, p_ptr->py, p_ptr->px, 0, GF_KILL_TRAP, flg, ""));
}
bool destroy_traps_doors_touch(int Ind, int rad) {
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE | PROJECT_NODF | PROJECT_NODO;
	return (project(0 - Ind, rad, &p_ptr->wpos, p_ptr->py, p_ptr->px, 0, GF_KILL_TRAP_DOOR, flg, ""));
}

bool sleep_monsters_touch(int Ind) {
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_NORF | PROJECT_KILL | PROJECT_HIDE | PROJECT_NODF | PROJECT_NODO;
	return (project(0 - Ind, 1, &p_ptr->wpos, p_ptr->py, p_ptr->px, p_ptr->lev, GF_OLD_SLEEP, flg, ""));
}

/* Scan magical powers for the golem */
static void scan_golem_flags(object_type *o_ptr, monster_race *r_ptr) {
	u32b f1, f2, f3, f4, f5, f6, esp;

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	if (f1 & TR1_LIFE) {
		r_ptr->hdice += o_ptr->pval;
		if (r_ptr->hdice > 13) r_ptr->hdice = 13;
	}
	//if (f1 & TR1_SPEED) r_ptr->speed += o_ptr->pval * 2 / 3;
	if (f1 & TR1_TUNNEL) r_ptr->flags2 |= RF2_KILL_WALL;
	if (f2 & TR2_RES_FIRE) r_ptr->flags3 |= RF3_IM_FIRE;
	if (f2 & TR2_RES_ACID) r_ptr->flags3 |= RF3_IM_ACID;
	if (f2 & TR2_RES_NETHER) r_ptr->flags3 |= RF3_RES_NETH;
	if (f2 & TR2_RES_NEXUS) r_ptr->flags3 |= RF3_RES_NEXU;
	if (f2 & TR2_RES_DISEN) r_ptr->flags3 |= RF3_RES_DISE;

	/* Allow use of runes - some are redundant (for completeness) */
	if (o_ptr->tval == TV_RUNE) switch(o_ptr->sval) {
#if 0
	case SV_R_LITE: r_ptr->flags9 |= RF9_RES_LITE; break;
	case SV_R_DARK: r_ptr->flags9 |= RF9_RES_DARK; break;
#endif
	case SV_R_NEXU: r_ptr->flags3 |= RF3_RES_NEXU; break;
	case SV_R_NETH: r_ptr->flags3 |= RF3_RES_NETH; break;
#if 0
	case SV_R_CHAO: r_ptr->flags9 |= RF9_RES_CHAOS; break;
	case SV_R_MANA: r_ptr->flags9 |= RF9_RES_MANA; break;

	case SV_R_CONF: r_ptr->flags3 |= RF3_NO_CONF; break;
//	case SV_R_INER: r_ptr->flags4 |= RF4_BR_INER; break; //Inertia has no resist; alt: RF3_NO_TELE!
	case SV_R_ELEC: r_ptr->flags3 |= RF3_IM_ELEC; break;
#endif
	case SV_R_FIRE: r_ptr->flags3 |= RF3_IM_FIRE; break;
#if 0
	case SV_R_WATE: r_ptr->flags3 |= RF3_RES_WATE; break;
//	case SV_R_GRAV: r_ptr->flags4 |= RF4_BR_GRAV; break; //Gravity has no resist (for mobs)!
	case SV_R_COLD: r_ptr->flags3 |= RF3_IM_COLD; break;
#endif
	case SV_R_ACID: r_ptr->flags3 |= RF3_IM_ACID; break;
#if 0
	case SV_R_POIS: r_ptr->flags3 |= RF3_IM_POIS; break;
	case SV_R_TIME: r_ptr->flags9 |= RF9_RES_TIME; break;
	case SV_R_SOUN: r_ptr->flags9 |= RF9_RES_SOUND; break;
	case SV_R_SHAR: r_ptr->flags9 |= RF9_RES_SHARDS; break;
#endif
	case SV_R_DISE: r_ptr->flags3 |= RF3_RES_DISE; break;
#if 0
	//Hack, gestalt elements add mixture.. - Kurzel
	//case SV_R_ICEE: r_ptr->flags3 |= RF3_IM_COLD; r_ptr->flags9 |= RF9_RES_SHARDS; r_ptr->flags9 |= RF9_RES_SOUND; break; //See common/tables.c - Kurzel
	case SV_R_ICEE: r_ptr->flags3 |= RF3_IM_COLD; r_ptr->flags9 |= RF9_RES_SHARDS; break; //See common/tables.c - Kurzel
	//case SV_R_PLAS: r_ptr->flags9 |= RF9_RES_ELEC; r_ptr->flags9 |= RF9_RES_FIRE; r_ptr->flags9 |= RF9_RES_SOUND; break;
#endif
	case SV_R_PLAS:
#if 0
		r_ptr->flags3 |= RF3_IM_FIRE;
		r_ptr->flags9 |= RF9_RES_ELEC;
		r_ptr->flags9 |= RF9_RES_SOUND;
#else
		r_ptr->flags3 |= RF3_IM_FIRE; break;
#endif
		break;

	default: break;
	}
}

/* multi builder stuff - move when complete */
struct builder {
	int player;
	int lx, ly, dx, dy, minx, miny, maxx, maxy;
	int sx, sy;
	int odir;
	int moves;
	int cvert;
	struct worldpos wpos;
	bool nofloor;
	bool jail;
	struct dna_type *dna;
	char *vert;
	struct c_special *cs;
	struct builder *next;
};

#define MAX_BUILDERS 4	/* Just so the builders can go on strike */
/* end of move stuff */

static bool poly_build(int Ind, char *args) {
	static struct builder *builders = NULL;
	static int num_build = 0;

	player_type *p_ptr = Players[Ind];
	struct builder *curr = builders;
	int x, y;
	int dir = 0;
	cave_type **zcave;
	if (!(zcave = getcave(&p_ptr->wpos))) return(FALSE);
	while (curr) {
		struct builder *prev = NULL;
		bool kill = FALSE;
		if (curr->player == p_ptr->id) break;
		if (!lookup_player_name(curr->player)) {	/* disconnect or free builders */
			if (prev)
				prev->next = curr->next;
			else
				builders = curr->next;
			kill = TRUE;
		}
		prev = curr;
		curr = curr->next;
		if (kill) {
			KILL(prev, struct builder);
		}
	}

	if (!curr) {			/* new builder */
#ifdef MAX_BUILDERS
		if (num_build == MAX_BUILDERS){
			msg_print(Ind,"The builders are on strike!");
			return FALSE;
		}
#endif
		MAKE(curr, struct builder);
		curr->next = builders;	/* insert is fastest */
		curr->player = p_ptr->id;	/* set him up */
		C_MAKE(curr->vert, MAXCOORD, char);
		MAKE(curr->dna, struct dna_type);
		curr->dna->creator = p_ptr->dna;
		curr->dna->owner = p_ptr->id;
		curr->dna->owner_type = OT_PLAYER;
		curr->dna->a_flags = ACF_NONE;
		curr->dna->min_level = ACF_NONE;
		curr->dna->price = 5;	/* so work out */
		curr->odir = 0;
		curr->cvert = 0;
		curr->nofloor = (args[0] == 'N');
		curr->jail = (args[1] == 'Y');
		curr->sx = p_ptr->px;
		curr->sy = p_ptr->py;
		curr->minx = curr->maxx = curr->sx;
		curr->miny = curr->maxy = curr->sy;
		curr->dx = curr->lx = curr->sx;
		curr->dy = curr->ly = curr->sy;
		curr->moves = 25;	/* always new */
		wpcopy(&curr->wpos, &p_ptr->wpos);
//		if (zcave[curr->sy][curr->sx].feat == FEAT_PERM_EXTRA){
		if (zcave[curr->sy][curr->sx].feat == FEAT_WALL_HOUSE){
#if 0	/* not necessary? - evileye */
			zcave[curr->sy][curr->sx].special.sc.ptr = NULL;
#endif
			msg_print(Ind, "Your foundations were laid insecurely");
			KILL(curr->dna, struct dna_type);
			C_KILL(curr->vert, MAXCOORD, char);
			p_ptr->master_move_hook = NULL;
			KILL(curr, struct builder);	/* Sack the builders! */
			return FALSE;
		}
		zcave[curr->sy][curr->sx].feat = FEAT_HOME_OPEN;
		/* CS_DNADOOR seems to be added twice (wild_add_uhouse)..
		 * please correct it, Evileye?	- Jir -
		 */
#if 0
		if ((curr->cs = AddCS(&zcave[curr->sy][curr->sx], CS_DNADOOR))) {
			curr->cs->sc.ptr = curr->dna;
		}
#endif
		builders = curr;
		return TRUE;
	}

	if (args) {
		curr->moves += 25;
		return TRUE;
	}
	x = p_ptr->px;
	y = p_ptr->py;
	curr->minx = MIN(x, curr->minx);
	curr->maxx = MAX(x, curr->maxx);
	curr->miny = MIN(y, curr->miny);
	curr->maxy = MAX(y, curr->maxy);
	if (x != curr->dx) {
		if (x > curr->dx) dir = 1;
		else dir = 2;
	}
	if (y != curr->dy) {
		if (dir) {
			curr->moves = 0;
			/* diagonal! house building failed */
		}
		if(y > curr->dy) dir |= 4;
		else dir |= 8;
	}
	if (curr->odir != dir) {
		if (curr->odir) {		/* first move not new vertex */
			curr->vert[curr->cvert++] = curr->dx-curr->lx;
			curr->vert[curr->cvert++] = curr->dy-curr->ly;
		}
		curr->lx = curr->dx;
		curr->ly = curr->dy;
		curr->odir = dir;		/* change direction, add vertex */
	}
	curr->dx = x;
	curr->dy = y;

	if (p_ptr->px == curr->sx && p_ptr->py == curr->sy && curr->moves) {	/* check for close */
		curr->vert[curr->cvert++] = curr->dx-curr->lx;			/* last vertex */
		curr->vert[curr->cvert++] = curr->dy-curr->ly;
		if (curr->cvert == 10 || curr->cvert == 8) {
			/* rectangle! */
			houses[num_houses].flags = HF_RECT;
			houses[num_houses].x = curr->minx;
			houses[num_houses].y = curr->miny;
			houses[num_houses].coords.rect.width = curr->maxx + 1 - curr->minx;
			houses[num_houses].coords.rect.height = curr->maxy + 1 - curr->miny;
#if 0 /* hm, isn't this buggy? should just be same as for HF_NONE? */
			houses[num_houses].dx = curr->sx-curr->minx;
			houses[num_houses].dy = curr->sy-curr->miny;
#else
			houses[num_houses].dx = curr->sx;
			houses[num_houses].dy = curr->sy;
#endif
			C_KILL(curr->vert, MAXCOORD, char);
		} else {
			houses[num_houses].flags = HF_NONE;	/* polygonal */
			houses[num_houses].x = curr->sx;
			houses[num_houses].y = curr->sy;
			houses[num_houses].coords.poly = curr->vert;
			/* was missing: actually remember the door position! (for pick_house() to work) */
			houses[num_houses].dx = curr->sx;
			houses[num_houses].dy = curr->sy;
		}
		if (curr->nofloor) houses[num_houses].flags |= HF_NOFLOOR;
		if (curr->jail) houses[num_houses].flags |= HF_JAIL;
/* Moat testing */
#if 0
		houses[num_houses].flags |= HF_MOAT;
#endif
/* Do not commit! */
		wpcopy(&houses[num_houses].wpos, &p_ptr->wpos);
		houses[num_houses].dna = curr->dna;
		if (curr->cvert >= 8 && fill_house(&houses[num_houses], FILL_MAKEHOUSE, NULL)){
			int area = (curr->maxx-curr->minx) * (curr->maxy-curr->miny);
			houses[num_houses].flags |= HF_SELFBUILT;
			curr->dna->price = area * area * 400; //initial_house_price(&houses[num_houses])
			wild_add_uhouse(&houses[num_houses]);
			msg_print(Ind,"You have completed your house");
			num_houses++;
		} else {
			msg_print(Ind,"Your house was built unsoundly");
			if(curr->vert) C_KILL(curr->vert, MAXCOORD, char);
			KILL(curr->dna, struct dna_type);
		}
		curr->player = 0;		/* send the builders home */
		p_ptr->master_move_hook = NULL;
		p_ptr->update |= PU_VIEW;
		return TRUE;
	}
	/* no going off depth, and no spoiling moats */
	if (inarea(&curr->wpos, &p_ptr->wpos) && !(zcave[curr->dy][curr->dx].info&CAVE_ICKY && zcave[curr->dy][curr->dx].feat == FEAT_DEEP_WATER)) {
		zcave[curr->dy][curr->dx].feat = FEAT_WALL_EXTRA;
//		zcave[curr->dy][curr->dx].feat = FEAT_WALL_HOUSE;
		if (curr->cvert < MAXCOORD && (--curr->moves) > 0) return TRUE;
		p_ptr->update |= PU_VIEW;
	}
	msg_print(Ind,"Your house building attempt has failed");
	cs_erase(&zcave[curr->sy][curr->sx], curr->cs);
	KILL(curr->dna, struct dna_type);
	C_KILL(curr->vert, MAXCOORD, char);
	curr->player = 0;		/* send the builders home */
	p_ptr->master_move_hook = NULL;
	return FALSE;
}

void house_creation(int Ind, bool floor, bool jail) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	char buildargs[3];

	/* set master_move_hook : a bit like a setuid really ;) */
	printf("floor: %d jail: %d\n",floor,jail);

	/* No building in town */
	if (wpos->wz) {
		msg_print(Ind, "\376\377yYou must build on the world surface.");
		return;
	}
	if (istownarea(wpos, MAX_TOWNAREA)) {
		msg_print(Ind, "\376\377yYou cannot build within a town area.");
		if (!is_admin(p_ptr)) return;
	}
	if (house_alloc - num_houses < 32) {
		GROW(houses, house_alloc, house_alloc + 512, house_type);
		GROW(houses_bak, house_alloc, house_alloc + 512, house_type);
		house_alloc += 512;
	}
	p_ptr->master_move_hook = poly_build;

	buildargs[0] = (floor ? 'Y' : 'N');
	buildargs[1] = (jail ? 'Y' : 'N');
	buildargs[2] = '\0';

	poly_build(Ind, (char*)&buildargs);	/* Its a (char*) ;( */
}

/* (Note: Apparently currently only used by Moltor's second_handler().) */
extern bool place_foe(int owner_id, struct worldpos *wpos, int y, int x, int r_idx) {
	int                     i, Ind, j;
	cave_type               *c_ptr;

	monster_type    *m_ptr;
	monster_race    *r_ptr = &r_info[r_idx];

	char buf[80];

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return (0);
	/* Verify location */
	if (!in_bounds(y, x)) return (0);
	/* Require empty space */
	if (!cave_empty_bold(zcave, y, x)) return (0);

	/* Paranoia */
	if (!r_idx) return (0);

	/* Paranoia */
	if (!r_ptr->name) return (0);

	/* Update r_ptr due to possible r_idx changes */
	r_ptr = &r_info[r_idx];

	if ((r_ptr->flags1 & RF1_UNIQUE)) return (0);

	c_ptr = &zcave[y][x];

	/* Make a new monster */
	c_ptr->m_idx = m_pop();

	/* Mega-Hack -- catch "failure" */
	if (!c_ptr->m_idx) return (0);

	/* Get a new monster record */
	m_ptr = &m_list[c_ptr->m_idx];

	/* Save the race */
	m_ptr->r_idx = r_idx;

	/* Place the monster at the location */
	m_ptr->fy = y;
	m_ptr->fx = x;
	wpcopy(&m_ptr->wpos, wpos);

	m_ptr->special = m_ptr->questor = 0;

	/* Hack -- Count the monsters on the level */
	r_ptr->cur_num++;


	/* Hack -- count the number of "reproducers" */
	if (r_ptr->flags7 & RF7_MULTIPLY) num_repro++;


	/* Assign maximal hitpoints */
	if (r_ptr->flags1 & RF1_FORCE_MAXHP)
		m_ptr->maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
	else
		m_ptr->maxhp = damroll(r_ptr->hdice, r_ptr->hside);

	/* And start out fully healthy */
	m_ptr->hp = m_ptr->maxhp;

	/* Extract the monster base speed */
	m_ptr->speed = r_ptr->speed;
	/* set cur speed to base speed */
	m_ptr->mspeed = m_ptr->speed;

	/* Extract base ac and  other things */
	m_ptr->ac = r_ptr->ac;

	for (j = 0; j < 4; j++) {
		m_ptr->blow[j].effect = r_ptr->blow[j].effect;
		m_ptr->blow[j].method = r_ptr->blow[j].method;
		m_ptr->blow[j].d_dice = r_ptr->blow[j].d_dice;
		m_ptr->blow[j].d_side = r_ptr->blow[j].d_side;
	}
	m_ptr->level = r_ptr->level;
	m_ptr->exp = MONSTER_EXP(m_ptr->level);
	m_ptr->owner = 0;

	/* Hack -- small racial variety */
	if (!(r_ptr->flags1 & RF1_UNIQUE)) {
		/* Allow some small variation per monster */
		i = extract_energy[m_ptr->speed] / 100;
		if (i) {
			j = rand_spread(0, i);
			m_ptr->mspeed += j;

			if (m_ptr->mspeed < j) m_ptr->mspeed = 255;
		}
	}


	/* Give a random starting energy */
	m_ptr->energy = rand_int(1000);

	/* Hack -- Reduce risk of "instant death by breath weapons" */
	if (r_ptr->flags1 & RF1_FORCE_SLEEP) {
		/* Start out with minimal energy */
		m_ptr->energy = rand_int(100);
	}

	/* Starts 'flickered out'? */
	if ((r_ptr->flags2 & RF2_WEIRD_MIND) && rand_int(10)) m_ptr->no_esp_phase = TRUE;

	/* No "damage" yet */
	m_ptr->stunned = 0;
	m_ptr->confused = 0;
	m_ptr->monfear = 0;

	/* No knowledge */
	m_ptr->cdis = 0;

	/* clone value */

	m_ptr->owner = 0;
	m_ptr->pet = 0;

	for (Ind = 1; Ind <= NumPlayers; Ind++) {
		if (Players[Ind]->conn == NOT_CONNECTED)
			continue;

		Players[Ind]->mon_los[c_ptr->m_idx] = 0;
		Players[Ind]->mon_vis[c_ptr->m_idx] = 0;
	}

	if (getlevel(wpos) > (m_ptr->level + 7)) {
		m_ptr->exp = MONSTER_EXP(m_ptr->level + ((getlevel(wpos) - (m_ptr->level + 7)) / 3));
		monster_check_experience(c_ptr->m_idx, TRUE);
	}

	strcpy(buf, (r_name + r_ptr->name));

	/* Update the monster */
	update_mon(c_ptr->m_idx, TRUE);


	/* Assume no sleeping */
	m_ptr->csleep = 0;

	/* STR */
	for (j = 0; j < 4; j++) {
		m_ptr->blow[j].org_d_dice = r_ptr->blow[j].d_dice;
		m_ptr->blow[j].org_d_side = r_ptr->blow[j].d_side;
	}
	/* DEX */
	m_ptr->org_ac = m_ptr->ac;
	/* CON */
	m_ptr->org_maxhp = m_ptr->maxhp;

#ifdef MONSTER_ASTAR
	if (r_ptr->flags0 & RF0_ASTAR) {
		/* search for an available A* table to use */
		for (j = 0; j < ASTAR_MAX_INSTANCES; j++) {
			/* found an available instance? */
			if (astar_info_open[j].m_idx == -1) {
				astar_info_open[j].m_idx = c_ptr->m_idx;
				m_ptr->astar_idx = j;
 #ifdef ASTAR_DISTRIBUTE
				astar_info_closed[j].nodes = 0;
 #endif
				break;
			}
		}
		/* no instance available? Mark us (-1) to use normal movement instead */
		if (j == ASTAR_MAX_INSTANCES) m_ptr->astar_idx = -1;
	} else m_ptr->astar_idx = -1;
#endif

	return (TRUE);
}
#ifdef RPG_SERVER
bool place_pet(int owner_id, struct worldpos *wpos, int y, int x, int r_idx) {
	int                     Ind, j;
	cave_type               *c_ptr;

	monster_type    *m_ptr;
	monster_race    *r_ptr = &r_info[r_idx];

	char buf[80];

	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return (0);


	/* Verify location */
	if (!in_bounds(y, x)) return (0);
	/* Require empty space */
	if (!cave_empty_bold(zcave, y, x)) return (0);
	/* Hack -- no creation on glyph of warding */
	if (zcave[y][x].feat == FEAT_GLYPH) return (0);
	if (zcave[y][x].feat == FEAT_RUNE) return (0);

	/* Paranoia */
	if (!r_idx) return (0);

	/* Paranoia */
	if (!r_ptr->name) return (0);

	/* Update r_ptr due to possible r_idx changes */
	r_ptr = &r_info[r_idx];

	/* No uniques, obviously */
	if (r_ptr->flags1 & RF1_UNIQUE) return (0);

	/* No breeders */
	if (r_ptr->flags7 & RF7_MULTIPLY) return (0);

	c_ptr = &zcave[y][x];

	/* Make a new monster */
	c_ptr->m_idx = m_pop();

	/* Mega-Hack -- catch "failure" */
	if (!c_ptr->m_idx) return (0);

	/* Get a new monster record */
	m_ptr = &m_list[c_ptr->m_idx];

	/* Save the race */
	m_ptr->r_idx = r_idx;

	/* Place the monster at the location */
	m_ptr->fy = y;
	m_ptr->fx = x;
	wpcopy(&m_ptr->wpos, wpos);

	m_ptr->special = m_ptr->questor = 0;

	/* Hack -- Count the monsters on the level */
	r_ptr->cur_num++;

	/* Assign maximal hitpoints */
	if (r_ptr->flags1 & RF1_FORCE_MAXHP)
		m_ptr->maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
	else
		m_ptr->maxhp = damroll(r_ptr->hdice, r_ptr->hside);

	/* And start out fully healthy */
	m_ptr->hp = m_ptr->maxhp;

	/* Extract the monster base speed */
	m_ptr->speed = r_ptr->speed;
	/* set cur speed to base speed */
	m_ptr->mspeed = m_ptr->speed;

	/* Extract base ac and  other things */
	m_ptr->ac = r_ptr->ac;

	for (j = 0; j < 4; j++) {
		m_ptr->blow[j].effect = r_ptr->blow[j].effect;
		m_ptr->blow[j].method = r_ptr->blow[j].method;
		m_ptr->blow[j].d_dice = r_ptr->blow[j].d_dice;
		m_ptr->blow[j].d_side = r_ptr->blow[j].d_side;
	}
	m_ptr->level = r_ptr->level;
	m_ptr->exp = MONSTER_EXP(m_ptr->level);
	m_ptr->owner = 0;

	/* Give a random starting energy */
	m_ptr->energy = rand_int(100);

	/* No "damage" yet */
	m_ptr->stunned = 0;
	m_ptr->confused = 0;
	m_ptr->monfear = 0;

	/* No knowledge */
	m_ptr->cdis = 0;

	/* special pet value */
	m_ptr->owner = Players[owner_id]->id;
	m_ptr->pet = 1;

	for (Ind = 1; Ind <= NumPlayers; Ind++) {
		if (Players[Ind]->conn == NOT_CONNECTED)
			continue;

		Players[Ind]->mon_los[c_ptr->m_idx] = 0;
		Players[Ind]->mon_vis[c_ptr->m_idx] = 0;
	}

	if (getlevel(wpos) >= (m_ptr->level + 8)) {
		m_ptr->exp = MONSTER_EXP(m_ptr->level + ((getlevel(wpos) - m_ptr->level - 5) / 3));
		monster_check_experience(c_ptr->m_idx, TRUE);
	}

	strcpy(buf, (r_name + r_ptr->name));

	/* Update the monster */
	update_mon(c_ptr->m_idx, TRUE);

	/* Assume no sleeping */
	m_ptr->csleep = 0;

	/* STR */
	for (j = 0; j < 4; j++) {
		m_ptr->blow[j].org_d_dice = r_ptr->blow[j].d_dice;
		m_ptr->blow[j].org_d_side = r_ptr->blow[j].d_side;
	}
	/* DEX */
	m_ptr->org_ac = m_ptr->ac;
	/* CON */
	m_ptr->org_maxhp = m_ptr->maxhp;

	return (TRUE);
}

/* Create a servant ! */
char pet_creation(int Ind)
{ //put the sanity tests here and call place_pet
	int id = 955; //green dr for now
	/* bleh, green dr is too powerful, lets do spiders. i'm fond of spiders. */
	int lev = Players[Ind]->lev;
	if (lev < 5) id = (randint(2)>1 ? 60 /*cave S*/ : 62 /*wild cat*/);
	else if (lev < 10) id = 127; //wood S
	else if (lev < 15) id = 277; //mirkwood S
	else if (lev < 20) id = 963; //aranea
	else id = 964; //elder aranea

	if (!Players[Ind]->has_pet) {
		place_pet(Ind,
		   &(Players[Ind]->wpos), Players[Ind]->py, Players[Ind]->px+1,  /* E of player */
		   id);
		Players[Ind]->has_pet = 1;
		return 1;
	}
	return 0;
}
#endif

/* Create a mindless servant ! */
void golem_creation(int Ind, int max) {
	player_type *p_ptr = Players[Ind];
	monster_race *r_ptr;
	monster_type *m_ptr;
	int i, tmp_dam = 0;
	int golem_type = -1;
	int golem_arms[4], golem_m_arms = 0;
	int golem_legs[2], golem_m_legs = 0;
	s16b golem_flags = 0;
	cave_type *c_ptr;
	int x, y, k, g_cnt = 0;
	bool okay = FALSE;
	object_type *o_ptr;
	char o_name[ONAME_LEN];
	unsigned char *inscription;
	cave_type **zcave;
	if (!(zcave = getcave(&p_ptr->wpos))) return;

	/* Process the monsters */
	for (k = m_top - 1; k >= 0; k--) {
		/* Access the index */
		i = m_fast[k];

		/* Access the monster */
		m_ptr = &m_list[i];

		/* Excise "dead" monsters */
		if (!m_ptr->r_idx) continue;

		if (m_ptr->owner != p_ptr->id) continue;

		if (!i) continue;

		g_cnt++;
	}

	if (g_cnt >= max) {
		msg_print(Ind, "You cannot create more golems.");
		return;
	}

	for (x = p_ptr->px - 1; x <= p_ptr->px; x++)
		for (y = p_ptr->py - 1; y <= p_ptr->py; y++) {
			/* Verify location */
			if (!in_bounds(y, x)) continue;
			/* Require empty space */
			if (!cave_empty_bold(zcave, y, x)) continue;

			/* Hack -- no creation on glyph of warding */
			if (zcave[y][x].feat == FEAT_GLYPH) continue;
			if (zcave[y][x].feat == FEAT_RUNE) continue;

			if ((p_ptr->px == x) && (p_ptr->py == y)) continue;

			okay = TRUE;
			break;
		}

	if (!okay) {
		msg_print(Ind, "You don't have sufficient space to create a golem.");
		return;
	}


	s_printf("GOLEM_CREATION: '%s' initiated.\n", p_ptr->name);

	/* Access the location */
	c_ptr = &zcave[y][x];

	/* Make a new monster */
	c_ptr->m_idx = m_pop();

	/* Mega-Hack -- catch "failure" */
	if (!c_ptr->m_idx) return;

	/* Grab and allocate */
	m_ptr = &m_list[c_ptr->m_idx];
	MAKE(m_ptr->r_ptr, monster_race);
	m_ptr->special = TRUE;
	m_ptr->questor = FALSE;
	m_ptr->fx = x;
	m_ptr->fy = y;

	r_ptr = m_ptr->r_ptr;

	r_ptr->flags1 = 0;
	r_ptr->flags2 = 0;
	r_ptr->flags3 = 0;
	r_ptr->flags4 = 0;
	r_ptr->flags5 = 0;
	r_ptr->flags6 = 0;
	r_ptr->flags7 = 0;
	r_ptr->flags8 = 0;
	r_ptr->flags9 = 0;
	r_ptr->flags0 = 0;

	msg_print(Ind, "Some of your items begins to consume in roaring flames.");

	/* Find items used for "golemification" */
	for (i = 0; i < INVEN_WIELD; i++) {
		o_ptr = &p_ptr->inventory[i];

		if (o_ptr->tval != TV_GOLEM)  continue;

		if (o_ptr->sval <= SV_GOLEM_ADAM) {
			if (golem_type != -1) continue;

			object_desc(0, o_name, o_ptr, FALSE, 0);
			s_printf("GOLEM_CREATION: consumed %s.\n", o_name);

			golem_type = o_ptr->sval;
			inven_item_increase(Ind, i, -1);
			inven_item_optimize(Ind, i);
			i--;
			continue;
		}
		else if (o_ptr->sval == SV_GOLEM_ARM && golem_m_arms < 4) {
			while (o_ptr->number) {
				if (golem_m_arms == 4) break;

				object_desc(0, o_name, o_ptr, FALSE, 0);
				s_printf("GOLEM_CREATION: consumed %s.\n", o_name);

				golem_arms[golem_m_arms++] = o_ptr->pval;
				inven_item_increase(Ind, i, -1);
			}
			inven_item_optimize(Ind, i);
			i--;
			continue;
		}
		else if (o_ptr->sval == SV_GOLEM_LEG && golem_m_legs < 2) {
			while (o_ptr->number) {
				if (golem_m_legs == 2) break;//30 is too ridiculous for SPEED..

				object_desc(0, o_name, o_ptr, FALSE, 0);
				s_printf("GOLEM_CREATION: consumed %s.\n", o_name);

				golem_legs[golem_m_legs++] = o_ptr->pval;
				inven_item_increase(Ind, i, -1);
			}
			inven_item_optimize(Ind, i);
			i--;
			continue;
		}
		/* golem command scrolls */
		else golem_flags |= 1U << (o_ptr->sval - 200);
	}

	/* Ahah FAIL !!! */
	if (golem_type == -1 || golem_m_legs < 2) {
		s_printf("GOLEM_CREATION: failed! type %d, legs %d.\n", golem_type, golem_m_legs);
		msg_print(Ind, "The spell fails! You lose all your material.");
		delete_monster_idx(c_ptr->m_idx, TRUE);
		return;
	}

	r_ptr->text = 0;
	r_ptr->name = 0;
	r_ptr->sleep = 0;
	r_ptr->aaf = 20;
	r_ptr->speed = 110;
	for (i = 0; i < golem_m_legs; i++)
		r_ptr->speed += golem_legs[i];
	r_ptr->mexp = 1;

	/* default colour, new: will be reset depending on base material */
	r_ptr->d_attr = r_ptr->x_attr = TERM_YELLOW;
	r_ptr->d_char = r_ptr->x_char = 'g';

	r_ptr->freq_innate = 0;
	r_ptr->freq_spell = 0;
	r_ptr->flags1 |= RF1_FORCE_MAXHP;
	r_ptr->flags2 |= RF2_STUPID | RF2_EMPTY_MIND | RF2_REGENERATE | RF2_POWERFUL | RF2_BASH_DOOR | RF2_MOVE_BODY;
	r_ptr->flags3 |= RF3_HURT_ROCK | RF3_IM_COLD | RF3_IM_ELEC | RF3_IM_POIS | RF3_NO_FEAR | RF3_NO_CONF | RF3_NO_SLEEP;
	r_ptr->flags9 |= RF9_IM_TELE;

	switch (golem_type) {
		case SV_GOLEM_WOOD:
			r_ptr->hdice = 10;
			r_ptr->hside = 10;
			r_ptr->ac = 20;
			r_ptr->d_attr = r_ptr->x_attr = TERM_UMBER;
			break;
		case SV_GOLEM_COPPER:
			r_ptr->hdice = 10;
			r_ptr->hside = 20;
			r_ptr->ac = 40;
			r_ptr->d_attr = r_ptr->x_attr = TERM_ORANGE;
			break;
		case SV_GOLEM_IRON:
			r_ptr->hdice = 10;
			r_ptr->hside = 40;
			r_ptr->ac = 70;
			r_ptr->d_attr = r_ptr->x_attr = TERM_L_DARK;
			break;
		case SV_GOLEM_ALUM:
			r_ptr->hdice = 10;
			r_ptr->hside = 60;
			r_ptr->ac = 90;
			r_ptr->d_attr = r_ptr->x_attr = TERM_SLATE;
			break;
		case SV_GOLEM_SILVER:
			r_ptr->hdice = 10;
			r_ptr->hside = 70;
			r_ptr->ac = 100;
			r_ptr->d_attr = r_ptr->x_attr = TERM_L_WHITE;
			break;
		case SV_GOLEM_GOLD:
			r_ptr->hdice = 10;
			r_ptr->hside = 80;
			r_ptr->ac = 130;
			r_ptr->d_attr = r_ptr->x_attr = TERM_YELLOW;
			break;
		case SV_GOLEM_MITHRIL:
			r_ptr->hdice = 10;
			r_ptr->hside = 100;
			r_ptr->ac = 160;
			r_ptr->d_attr = r_ptr->x_attr = TERM_L_BLUE;
			break;
		case SV_GOLEM_ADAM:
			r_ptr->hdice = 10;
			r_ptr->hside = 150;
			r_ptr->ac = 210;
			r_ptr->d_attr = r_ptr->x_attr = TERM_VIOLET;
			break;
		//default:
	}

	r_ptr->extra = golem_flags;
#if 1
	/* Find items used for "golemification" */
	for (i = 0; i < INVEN_WIELD; i++) {
		o_ptr = &p_ptr->inventory[i];
		inscription = (unsigned char *) quark_str(o_ptr->note);

		/* Additionnal items ? */
		if (inscription != NULL) {
			/* scan the inscription for @G */
			while ((*inscription != '\0')) {
				if (*inscription == '@') {
					inscription++;

					/* a valid @G has been located */
					if (*inscription == 'G') {
						inscription++;

						object_desc(0, o_name, o_ptr, FALSE, 0);
						s_printf("GOLEM_CREATION: extra consumed %s.\n", o_name);

						scan_golem_flags(o_ptr, r_ptr);
						/* scan_golem_flags uses only one item */
						inven_item_increase(Ind,i,-1);
						inven_item_optimize(Ind,i);
						i--;
						continue;
					}
				}
				inscription++;
			}
		}
	}
#endif

	/* extract base speed */
	m_ptr->speed = r_ptr->speed;
	/* set cur speed to base speed */
	m_ptr->mspeed = m_ptr->speed;
	m_ptr->ac = r_ptr->ac;
	m_ptr->maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
	m_ptr->hp = maxroll(r_ptr->hdice, r_ptr->hside);
	m_ptr->clone = 100;

	for (i = 0; i < golem_m_arms; i++) {
		m_ptr->blow[i].method = r_ptr->blow[i].method = RBM_HIT;
		m_ptr->blow[i].effect = r_ptr->blow[i].effect = RBE_HURT;
		m_ptr->blow[i].d_dice = r_ptr->blow[i].d_dice = (golem_type + 1) * 2; //(golem types start at 0, hence +1)
		m_ptr->blow[i].d_side = r_ptr->blow[i].d_side = golem_arms[i];
		tmp_dam += (m_ptr->blow[i].d_dice + 1) * m_ptr->blow[i].d_side;
	}

	m_ptr->owner = p_ptr->id;
	m_ptr->r_idx = 1 + golem_type;

	m_ptr->level = p_ptr->lev;
	m_ptr->exp = MONSTER_EXP(m_ptr->level);

	/* Assume no sleeping */
	m_ptr->csleep = 0;
	wpcopy(&m_ptr->wpos, &p_ptr->wpos);

	/* No "damage" yet */
	m_ptr->stunned = 0;
	m_ptr->confused = 0;
	m_ptr->monfear = 0;

	/* No knowledge */
	m_ptr->cdis = 0;
	m_ptr->mind = GOLEM_NONE;

	/* prevent other players from killing it on accident */
	r_ptr->flags8 |= RF8_NO_AUTORET | RF8_GENO_PERSIST | RF8_GENO_NO_THIN;
	r_ptr->flags7 |= RF7_NO_TARGET;

	/* Update the monster */
	update_mon(c_ptr->m_idx, TRUE);

	s_printf("GOLEM_CREATION: succeeded! type %d, speed %d, damage %d, commands %d\n", golem_type, m_ptr->speed, tmp_dam / 2, r_ptr->extra);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);
	/* Window stuff */
	p_ptr->window |= (PW_INVEN);
}

bool summon_cyber(int Ind, int s_clone, int clone_summoning) {
	player_type *p_ptr = Players[Ind];
	int i;
	int max_cyber = (getlevel(&p_ptr->wpos)/ 50) + randint(6);
	bool ok = FALSE;

	for (i = 0; i < max_cyber; i++)
		ok = ok || summon_specific(&p_ptr->wpos, p_ptr->py, p_ptr->px, 100, s_clone, SUMMON_HI_DEMON, 1, clone_summoning);
	return ok;
}

/* Heal insanity. */
bool heal_insanity(int Ind, int val)
{
	player_type *p_ptr = Players[Ind];

	if (p_ptr->csane < p_ptr->msane) {
		p_ptr->csane += val;

		if (p_ptr->csane >= p_ptr->msane) {
			p_ptr->csane = p_ptr->msane;
			p_ptr->csane_frac = 0;
		}

		p_ptr->update |= PU_SANITY;
		p_ptr->redraw |= PR_SANITY;
		p_ptr->window |= (PW_PLAYER);

		if (val < (p_ptr->msane/8)) {
			msg_print(Ind, "You feel a little saner.");
		} else if (val < (p_ptr->msane/4)) {
			msg_print(Ind, "You feel saner.");
		} else if (val < (p_ptr->msane/2)) {
			msg_print(Ind, "You feel much saner.");
		} else {
			msg_print(Ind, "You feel very sane.");
		}

		return TRUE;
	}

	return FALSE;
}

bool do_vermin_control(int Ind) {
	dun_level *l_ptr = getfloor(&Players[Ind]->wpos);
	if (l_ptr && !(l_ptr->flags1 & LF1_NO_MULTIPLY)) {
		l_ptr->flags1 |= LF1_NO_MULTIPLY;
		msg_print(Ind, "You feel less itchy.");
		return TRUE;
	}
	return FALSE;
}

void rune_combine(int Ind) {
	player_type *p_ptr = Players[Ind];

	clear_current(Ind);

	p_ptr->current_rune = TRUE;
	get_item(Ind, ITH_RUNE);

	return;
}

void rune_combine_aux(int Ind, int item) {
	player_type *p_ptr = Players[Ind];

	if (item == p_ptr->current_activation) return; //don't combine the same rune

	/* Recall the first rune */
	object_type *o_ptr = &p_ptr->inventory[p_ptr->current_activation];

	/* Store the combining flag */
	s16b e_flags = r_elements[o_ptr->sval].flag;

	/* Remember the original info */
	byte sval = o_ptr->sval;
	s32b owner = o_ptr->owner;
	byte mode = o_ptr->mode;
	s16b level = o_ptr->level;
	byte discount = o_ptr->discount;
	byte number = o_ptr->number;
	s16b note = o_ptr->note; //keep the incription of the initial rune
	s32b iron_trade = o_ptr->iron_trade;

	/* Rune Preservation? */
	if (check_guard_inscription(o_ptr->note, 'R')) {
		if (number > 1) number--;
		else {
			msg_format(Ind, "\377yYou choose to preserve your rune.");
			return;
		}
	}

	/* Recall the second rune */
	o_ptr = &p_ptr->inventory[item];

	/* Sanity */
	if ((o_ptr->sval == sval) //same rune type
	    || (o_ptr->tval != TV_RUNE) //not a rune (obselete)
	    || (o_ptr->sval >= RCRAFT_MAX_ELEMENTS) //not a basic rune (obselete)
	    || (!(o_ptr->level) && !(o_ptr->owner == p_ptr->id))) { //not owned
		msg_format(Ind, "You cannot combine these runes!");
		return;
	}

	/* Store the combining flag */
	e_flags |= r_elements[o_ptr->sval].flag;

	/* Lookup resulting rune -- (flags_to_projection) */
	byte i;
	for (i = 0; i < RCRAFT_MAX_PROJECTIONS; i++) {
		if (e_flags == r_projections[i].flags)
			break;
	}

	/* Store combined item values -- use worst case! */
	level = (o_ptr->level > level) ? o_ptr->level : level;
	discount = (o_ptr->discount > discount) ? o_ptr->discount : discount;
	number = (o_ptr->number < number) ? o_ptr->number : number;

	/* Rune Preservation? (second rune) */
	if (check_guard_inscription(o_ptr->note, 'R')) {
		if (number > 1) number--;
		else {
			msg_format(Ind, "\377yYou choose to preserve your rune.");
			return;
		}
	}

	/* Destroy the rune stacks in the pack */
	msg_format(Ind, "There is a coupling of magic.");
	inven_item_increase(Ind, p_ptr->current_activation, -number);
	inven_item_increase(Ind, item, -number);
	if (p_ptr->current_activation > item) { //higher value (lower in inventory) first; to preserve indexes
		inven_item_optimize(Ind, p_ptr->current_activation);
		inven_item_optimize(Ind, item);
	} else {
		inven_item_optimize(Ind, item);
		inven_item_optimize(Ind, p_ptr->current_activation);
	}

	/* Make new stack */
	object_type *q_ptr, forge;

	/* Default rune template */
	q_ptr = &forge;
	object_wipe(q_ptr);
	invcopy(q_ptr, lookup_kind(TV_RUNE, i));

	/* Recall original parameters */
	q_ptr->owner = owner;
	q_ptr->mode = mode;
	q_ptr->level = level;
	q_ptr->discount = discount;
	q_ptr->number = number;
	q_ptr->note = note;
	q_ptr->iron_trade = iron_trade;

	/* Create the rune stack */
	inven_carry(Ind, q_ptr);

}

/* see create_custom_tome_aux() below */
void tome_creation(int Ind) {
	player_type *p_ptr = Players[Ind];

	clear_current(Ind);

	p_ptr->current_tome_creation = TRUE;
	get_item(Ind, ITH_CUSTOM_TOME);

	return;
}

/* add a spell scroll to the player's custom-made tome - C. Blue
   Note: pval must be incremented by 1, because first spell
   (MANATHRUST) starts at 0, but we use 0 for <not used>
   because xtra1..9 are of type 'byte' so we can't use -1. */
void tome_creation_aux(int Ind, int item) {
	player_type	*p_ptr = Players[Ind];
	bool		okay = FALSE;
	object_type	*o_ptr, *o2_ptr;
	char		o_name[ONAME_LEN];
	s16b		*xtra;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &p_ptr->inventory[item];
	/* Get the item (on the floor) */
	else o_ptr = &o_list[0 - item];
	/* Get the item (in the pack) */
	if (p_ptr->using_up_item >= 0) o2_ptr = &p_ptr->inventory[p_ptr->using_up_item];
	/* Get the item (on the floor) */
	else o2_ptr = &o_list[0 - p_ptr->using_up_item];

	/* severe error: custom book no longer there */
	if (o_ptr->tval != TV_BOOK || !is_custom_tome(o_ptr->sval)) {
		/* completely start from scratch (have to re-'activate') */
		msg_print(Ind, "A book's inventory location was changed, please retry!");
		clear_current(Ind); /* <- not required actually */
		return;
	}

#if 1 /* done in do_cmd_activate already. double-check */
	/* free space left? */
	/* k_info-pval dependant */
	switch (o_ptr->bpval) {
	case 9: if (!o_ptr->xtra9) okay = TRUE;
	case 8: if (!o_ptr->xtra8) okay = TRUE;
	case 7: if (!o_ptr->xtra7) okay = TRUE;
	case 6: if (!o_ptr->xtra6) okay = TRUE;
	case 5: if (!o_ptr->xtra5) okay = TRUE;
	case 4: if (!o_ptr->xtra4) okay = TRUE;
	case 3: if (!o_ptr->xtra3) okay = TRUE;
	case 2: if (!o_ptr->xtra2) okay = TRUE;
	case 1: if (!o_ptr->xtra1) okay = TRUE;
	default: break;
	}
	if (!okay) {
		/* completely start from scratch (have to re-'activate') */
		msg_print(Ind, "That book has no blank pages left!");
		clear_current(Ind); /* <- not required actually */
		return;
	}
#endif

	if (o2_ptr->tval != TV_BOOK || o2_ptr->sval != SV_SPELLBOOK) {
		msg_print(Ind, "You can only transcribe a spell scroll!");
		/* restore silyl hack.. */
		p_ptr->using_up_item = item;
		/* try again */
		get_item(Ind, ITH_CUSTOM_TOME);
		return;
	}

/* for DISCRETE_SPELL_SYSTEM: DSS_EXPANDED_SCROLLS */
//TODO: call Send_spell_request(Ind, item); here if client is not outdated (DSS_EXPANDED_SCROLLS)

	/* need to actually be able to cast the spell in order to transcribe it! */
	if (exec_lua(Ind, format("return get_level(%d, %d, 50, -50)", Ind, o2_ptr->pval)) < 1) {
		msg_print(Ind, "Your knowledge of that spell is insufficient!");
		if (!is_admin(p_ptr)) {
			/* restore silyl hack.. */
			p_ptr->using_up_item = item;
			/* try again */
			get_item(Ind, ITH_CUSTOM_TOME);
			return;
		}
	}

	/* check for duplicate entry -> prevent it */
	if (o_ptr->xtra1 == o2_ptr->pval + 1 ||
	    o_ptr->xtra2 == o2_ptr->pval + 1 ||
	    o_ptr->xtra3 == o2_ptr->pval + 1 ||
	    o_ptr->xtra4 == o2_ptr->pval + 1 ||
	    o_ptr->xtra5 == o2_ptr->pval + 1 ||
	    o_ptr->xtra6 == o2_ptr->pval + 1 ||
	    o_ptr->xtra7 == o2_ptr->pval + 1 ||
	    o_ptr->xtra8 == o2_ptr->pval + 1 ||
	    o_ptr->xtra9 == o2_ptr->pval + 1) {
		msg_print(Ind, "The book already contains that spell!");
		/* restore silyl hack.. */
		p_ptr->using_up_item = item;
		/* try again */
		get_item(Ind, ITH_CUSTOM_TOME);
		return;
	}

	/* - Success finally - */

	/* Find the next free slot */
	if (!o_ptr->xtra1) xtra = &o_ptr->xtra1;
	else if (!o_ptr->xtra2) xtra = &o_ptr->xtra2;
	else if (!o_ptr->xtra3) xtra = &o_ptr->xtra3;
	else if (!o_ptr->xtra4) xtra = &o_ptr->xtra4;
	else if (!o_ptr->xtra5) xtra = &o_ptr->xtra5;
	else if (!o_ptr->xtra6) xtra = &o_ptr->xtra6;
	else if (!o_ptr->xtra7) xtra = &o_ptr->xtra7;
	else if (!o_ptr->xtra8) xtra = &o_ptr->xtra8;
	else xtra = &o_ptr->xtra9;

	/* transcribe (add it)! */
	*xtra = o2_ptr->pval + 1;

	/* Description */
	object_desc(Ind, o_name, o_ptr, FALSE, 0);
	/* Describe */
	msg_format(Ind, "%s %s glow%s brightly!",
	    ((item >= 0) ? "Your" : "The"), o_name,
	    ((o_ptr->number > 1) ? "" : "s"));

	/* Did we use up an item? */
	if (p_ptr->using_up_item >= 0) {
		inven_item_increase(Ind, p_ptr->using_up_item, -1);
		inven_item_describe(Ind, p_ptr->using_up_item);
		inven_item_optimize(Ind, p_ptr->using_up_item);
		p_ptr->using_up_item = -1;
	}

	/* unstack if our custom book was originally in a pile */
	if ((item >= 0) && (o_ptr->number > 1)) {
		/* Make a fake item */
		object_type tmp_obj;
		tmp_obj = *o_ptr;
		tmp_obj.number = 1;

		/* Restore remaining 'untouched' stack of books */
		*xtra = 0;

		/* Message */
		msg_print(Ind, "You unstack your book.");

		/* Unstack the used item */
		o_ptr->number--;
		p_ptr->total_weight -= tmp_obj.weight;
		tmp_obj.iron_trade = o_ptr->iron_trade;
		item = inven_carry(Ind, &tmp_obj);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);
	p_ptr->notice |= (PN_REORDER);

	/* Something happened */
	return;
}

bool do_mstopcharm(int Ind) {
	player_type *p_ptr = Players[Ind];
	monster_type *m_ptr;
	int m;
	bool unknown = TRUE;

	if (p_ptr->mcharming == 0) return FALSE; /* optimization */

	p_ptr->mcharming = 0;

	for (m = m_top - 1; m >= 0; m--) {
		m_ptr = &m_list[m_fast[m]];
		//r_ptr = race_inf(m_ptr);
		if (m_ptr->charmedignore && m_ptr->charmedignore == Ind)
			unknown = m_ptr->charmedignore = 0;
	}
	return unknown;
}

bool test_charmedignore(int Ind, int Ind_charmer, monster_race *r_ptr) {
	player_type *q_ptr = Players[Ind_charmer];
	int chance = 1, cost = 1;

	/* non team-mates are not affected (and would cost extra mana) */
	if (Ind != Ind_charmer &&
	    (!player_in_party(q_ptr->party, Ind)))
		return FALSE;

	/* some monsters show resistance spikes */
	if ((r_ptr->flags2 & RF2_SMART) ||
	    (r_ptr->flags3 & RF3_NO_CONF) ||
	    (r_ptr->flags1 & RF1_UNIQUE) ||
	    (r_ptr->flags2 & RF2_POWERFUL)) {
		chance++;
		cost++;
	}

	/* especially costly? */
	if (r_ptr->level > (q_ptr->lev * 7) / 5)
		cost++;

	/* spell continuously burns mana! */
	if (turn % (cfg.fps / 10) == 0) {
		if (q_ptr->csp < 1) {
			do_mstopcharm(Ind_charmer);
			return FALSE;
		}
		q_ptr->csp -= 1;
		q_ptr->redraw |= PR_MANA;
	}

	/* the charm-caster himself is pretty safe */
	if (Ind == Ind_charmer && magik(100 - chance))
		return TRUE;
	/* in same party?= slightly reduced effect >:) */
	if (magik(99 - 2 * chance))
		return TRUE;

	return FALSE;
}

/* Add certain flags to a ball/breath/cloud projection depending on the damage type. */
u32b mod_ball_spell_flags(int typ, u32b flags) {
	switch (typ) {
	/* stuff that also acts on the casting player himself */
	case GF_HEAL_PLAYER:
	case GF_HEALINGCLOUD:
		return (flags | PROJECT_PLAY);
	/* 'environmentally instant' stuff */
	case GF_SOUND:
	case GF_CONFUSION:
	case GF_INERTIA:
	case GF_TIME:
	case GF_GRAVITY:
	case GF_STUN:
	case GF_PSI:
	case GF_UNBREATH:
	case GF_THUNDER:
	case GF_ANNIHILATION:
		return (flags | PROJECT_LODF);
	/* very powerful 'force' stuff */
	case GF_FORCE:
	case GF_METEOR:
	case GF_HAVOC:
	case GF_INFERNO:
	case GF_DETONATION:
	case GF_ROCKET:
	case GF_WAVE:
		return (flags | PROJECT_LODF);
	}
	return flags;
}
