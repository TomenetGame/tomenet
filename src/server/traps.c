/* $Id$ */
/* File: traps.c */

/* Purpose: handle traps */

/* the below copyright probably still applies, but it is heavily changed
 * copied, adapted & re-engineered by JK.
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

/*
 * Adapted from PernAngband to PernMangband by Jir.
 * All these changes also take over the Angband-licence
 * (as you see above).
 */

/* added this for consistency in some (unrelated) header-inclusion,
   it IS a server file, isn't it? */
#define SERVER
#include "angband.h"

/*
 * Modify the damage induced by missile traps, in percent. [100]
 * Adjust this value according to how wimpy your players are :)
 */
#define MISSILE_TRAP_FACTOR 75

static void destroy_chest(object_type *o_ptr);


#if 0
static bool do_trap_of_silliness(int Ind, int power) {
	//return (FALSE);

	player_type *p_ptr = Players[Ind];
	int i, j;
	bool aware = FALSE;

	for (i = 0; i < power; i++) {
		j = rand_int(max_k_idx);
		if (p_ptr->obj_aware[j]) aware = TRUE;
		p_ptr->obj_aware[j] = 0;

		j = rand_int(MAX_T_IDX);
		if (p_ptr->trap_ident[j]) aware = TRUE;
		p_ptr->trap_ident[j] = 0;
	}

	return (aware);

}
#endif // 0

bool do_player_drop_items(int Ind, int chance, bool trap) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	s16b i;
	bool message = FALSE, ident = FALSE;

	if (p_ptr->inval) return (FALSE);

	for (i = 0; i < INVEN_PACK; i++) {
		object_type tmp_obj = p_ptr->inventory[i];
		o_ptr = &p_ptr->inventory[i];
		if (!tmp_obj.k_idx) continue;
		if (randint(100) > chance) continue;
		if (tmp_obj.name1 == ART_POWER) continue;
		if (cfg.anti_arts_hoard && true_artifact_p(&tmp_obj) && (rand_int(100) > 9)) {
			char	o_name[ONAME_LEN];
			object_desc(Ind, o_name, &tmp_obj, TRUE, 0);

			msg_format(Ind, "%s resists the effect!", o_name);
			continue;
		}
		//tmp_obj = p_ptr->inventory[i];

		/* Hack -- If a wand, allocate total
		 * maximum timeouts or charges between those
		 * stolen and those missed. -LM-
		 */
		if (is_magic_device(o_ptr->tval)) divide_charged_item(&tmp_obj, o_ptr, 1);

		/* drop carefully */
		drop_near_severe(Ind, &tmp_obj, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);
		inven_item_increase(Ind, i,-999);
		inven_item_optimize(Ind, i);
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);
		if (trap && !message) {
			msg_print(Ind, "You are startled by a sudden sound.");
			message = TRUE;
		}
		ident = TRUE;
	}
	if (trap && !ident) msg_print(Ind, "You hear a sudden, strange sound.");

	return (ident);
}

bool do_player_scatter_items(int Ind, int chance, int rad) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	s16b i,j;
	bool message = FALSE;
	cave_type **zcave;
	zcave = getcave(&p_ptr->wpos);

	if (p_ptr->inval) return (FALSE);

	for (i = 0; i < INVEN_PACK; i++) {
		if (!p_ptr->inventory[i].k_idx) continue;
		if (rand_int(100) > chance) continue;
		for (j = 0; j < 10; j++) {
			object_type tmp_obj;
			s16b cx = p_ptr->px + rad - rand_int(rad * 2);
			s16b cy = p_ptr->py + rad - rand_int(rad * 2);
			if (!in_bounds(cy,cx)) continue;
			if (!cave_floor_bold(zcave, cy,cx)) continue;
			o_ptr = &p_ptr->inventory[i];
			tmp_obj = p_ptr->inventory[i];
			if (cfg.anti_arts_hoard && true_artifact_p(&tmp_obj) && (rand_int(100) > 9)) {
				char	o_name[ONAME_LEN];
				object_desc(Ind, o_name, &tmp_obj, TRUE, 0);

				msg_format(Ind, "%s resists the effect!", o_name);
				continue;
			}

			/* Hack -- If a wand, allocate total
			 * maximum timeouts or charges between those
			 * stolen and those missed. -LM-
			 */
			if (is_magic_device(o_ptr->tval)) divide_charged_item(&tmp_obj, o_ptr, 1);

			inven_item_increase(Ind, i,-999);
			inven_item_optimize(Ind, i);
			p_ptr->notice |= (PN_COMBINE | PN_REORDER);
			//(void)floor_carry(cy, cx, &tmp_obj);
			drop_near_severe(Ind, &tmp_obj, 0, &p_ptr->wpos, cy, cx);
			if (!message) {
				msg_print(Ind, "You feel light-footed.");
				message = TRUE;
			}
			if (player_has_los_bold(Ind, cy, cx)) {
				char i_name[ONAME_LEN];
				object_desc(Ind, i_name, &tmp_obj, TRUE, 3);
				note_spot(Ind, cy, cx);
				lite_spot(Ind, cy, cx);
				message = TRUE;
				msg_format(Ind, "Suddenly %s appear%s!", i_name, (tmp_obj.number > 1) ? "" : "s");
			}
			break;
		}
	}

	return (message);
}

static bool do_player_trap_garbage(int Ind, int times) {
	player_type *p_ptr = Players[Ind];
	int k, l, lv = getlevel(&p_ptr->wpos);
	bool ident = FALSE;
	object_type *o_ptr, forge;
	u32b f1, f2, f3, f4, f5, f6, esp;

	for (k = 0; k < times; k++) {
		l = rand_int(max_k_idx);

		/* hack -- !ruin, !death cannot be generated */
		if (!k_info[l].tval || k_info[l].cost || k_info[l].level > lv || k_info[l].level > 30) continue;

		o_ptr = &forge;
		invcopy(o_ptr, l);

		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
		if (f3 & TR3_INSTA_ART) continue;

		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
		o_ptr->iron_trade = p_ptr->iron_trade;

		ident = TRUE;
		if (inven_carry(Ind, o_ptr) < 0)
			drop_near(0, o_ptr, -1, &p_ptr->wpos, p_ptr->py, p_ptr->px);
	}
	return (ident);
}

/*
 * eg. if a player falls 250ft, set 'dis' to -5.
 */
static void do_player_trap_change_depth(int Ind, int dis) {
	player_type *p_ptr = Players[Ind];
	cave_type **zcave;
	struct worldpos old_wpos;

	zcave = getcave(&p_ptr->wpos);

	p_ptr->new_level_flag = TRUE;
	p_ptr->new_level_method = LEVEL_RAND;

	/* The player is gone */
	zcave[p_ptr->py][p_ptr->px].m_idx = 0;

	/* Show everyone that he's left */
	everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

	/* Erase his light */
	forget_lite(Ind);

	/* Save the old wpos */
	wpcopy(&old_wpos, &p_ptr->wpos);

	/* New wpos */
	p_ptr->wpos.wz += dis;

	new_players_on_depth(&old_wpos, -1, TRUE);
	new_players_on_depth(&p_ptr->wpos, 1, TRUE);
}

static bool do_player_trap_call_out(int Ind) {
	player_type *p_ptr = Players[Ind];
	s16b i,sn,cx,cy;
	s16b h_index = 0;
	s16b h_level = 0;
	monster_type *m_ptr;
	monster_race *r_ptr;
	char m_name[MNAME_LEN];
	bool ident = FALSE;
	cave_type **zcave;
	zcave = getcave(&p_ptr->wpos);
	int old_x, old_y;

	if (check_st_anchor(&p_ptr->wpos, p_ptr->py, p_ptr->px)) return(FALSE);

	for (i = 1; i < m_max; i++) {
		m_ptr = &m_list[i];
		r_ptr = race_inf(m_ptr);

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;
		if (!inarea(&p_ptr->wpos, &m_ptr->wpos)) continue;
		if ((r_ptr->flags1 & RF1_UNIQUE)) continue;

		if (m_ptr->level >= h_level) {
			h_level = m_ptr->level;
			h_index = i;
		}
	}
	/* if the level is empty of monsters, h_index will be 0 */
	if (!h_index) return(FALSE);

	m_ptr = &m_list[h_index];

	sn = 0;
	for (i = 1; i <= 9; i++) {
		cx = p_ptr->px + ddx[i];
		cy = p_ptr->py + ddy[i];
		if (!in_bounds(cy, cx)) continue;
		/* Skip non-empty grids */
		if (!cave_valid_bold(zcave, cy, cx)) continue; /* This wasn't really enough.. */
		if (!cave_empty_bold(zcave, cy, cx)) continue; /* better added this one;) -C. Blue */
		if (zcave[cy][cx].feat == FEAT_GLYPH) continue;
		if (zcave[cy][cx].feat == FEAT_RUNE) continue;
		if ((cx == p_ptr->px) && (cy == p_ptr->py)) continue;
		sn++;
		/* Randomize choice */
		if (rand_int(sn) > 0) continue;
		zcave[cy][cx].m_idx = h_index;
		zcave[m_ptr->fy][m_ptr->fx].m_idx = 0;
		old_x = m_ptr->fx;
		old_y = m_ptr->fy;
		m_ptr->fx = cx;
		m_ptr->fy = cy;
		/* we do not change the sublevel! */
		ident = TRUE;

		/* Gone from previous location - mikaelh */
		everyone_lite_spot(&p_ptr->wpos, old_y, old_x);

		update_mon(h_index, TRUE);

		/* Make sure everyone sees it now - mikaelh */
		everyone_lite_spot(&p_ptr->wpos, cy, cx);

		/* Actually wake it up... */
		m_ptr->csleep = 0;

		monster_desc(Ind, m_name, h_index, 0x08);
		msg_format(Ind, "You hear a rapid-shifting wail, and %s appears!",m_name);
		break;
	}
	return (ident);
}

/* done */
static bool do_trap_teleport_away(int Ind, object_type *i_ptr, s16b y, s16b x) {
	player_type *p_ptr = Players[Ind];
	bool ident = FALSE;
	char o_name[ONAME_LEN];

	s16b  o_idx = 0;
	object_type *o_ptr;
	//cave_type *c_ptr;

	s16b  x1;
	s16b  y1;

	/* Paranoia */
	cave_type **zcave;
	if (!in_bounds(y, x)) return(FALSE);
	if ((zcave = getcave(&p_ptr->wpos))) {
		/* naught */
//		c_ptr = &zcave[y][x];
	}
	else return(FALSE);
	if (check_st_anchor(&p_ptr->wpos, y, x)) return(FALSE);

	if (i_ptr == NULL) return(FALSE);

	/* God save the arts :) */
	if (i_ptr->name1 == ART_POWER) return (FALSE);
	if (cfg.anti_arts_hoard && true_artifact_p(i_ptr) && (rand_int(100) > 9)) return(FALSE);

	while (o_idx == 0) {
		x1 = rand_int(p_ptr->cur_wid);
		y1 = rand_int(p_ptr->cur_hgt);

		/* Obtain grid */
#if 0	// pfft, put off till f_info is revised
		c_ptr = &zcave[y][x];
		/* Require floor space (or shallow terrain) -KMW- */
		if (!(f_info[c_ptr->feat].flags1 & FF1_FLOOR)) continue;
#endif
		if (!cave_clean_bold(zcave, y1, x1)) continue;
		o_idx = drop_near_severe(Ind, i_ptr, 0, &p_ptr->wpos, y1, x1);
	}

	o_ptr = &o_list[o_idx];

	x1 = o_ptr->ix;
	y1 = o_ptr->iy;

	if (!p_ptr->blind) {
		note_spot(Ind, y,x);
		lite_spot(Ind, y,x);
		ident = TRUE;
		object_desc(Ind, o_name, i_ptr, FALSE, 0);
		if (player_has_los_bold(Ind, y1, x1)) {
			lite_spot(Ind, y1, x1);
			msg_format(Ind, "The %s suddenly stands elsewhere", o_name);

		} else msg_format(Ind, "You suddenly don't see the %s anymore!", o_name);
	} else msg_print(Ind, "You hear something move");
	return (ident);
}

/*
 * this function handles arrow & dagger traps, in various types.
 * num = number of missiles
 * tval, sval = kind of missiles
 * dd,ds = damage roll for missiles
 * poison_dam = additional poison damage
 * name = name given if you should die from it...
 *
 * return value = ident (always TRUE)
 */
static bool player_handle_missile_trap(int Ind, s16b num, s16b tval,
    s16b sval, s16b dd, s16b ds, s16b pdam, cptr name) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr, forge;
	s16b        i, dam, k_idx = lookup_kind(tval, sval);
	char        i_name[ONAME_LEN];
#ifndef NEW_DODGING
	int	dodge = p_ptr->dodge_level - (dd * ds) / 2;
#else
	int	dodge = apply_dodge_chance(Ind, 1 + dd * ds / 3 + 1000); /* dd,ds currently goes up to 12,16. 1000 is a hack. */
#endif
	o_ptr = &forge;
	invcopy(o_ptr, k_idx);
	o_ptr->number = num;
	apply_magic(&p_ptr->wpos, o_ptr, getlevel(&p_ptr->wpos), FALSE, FALSE, FALSE, FALSE, make_resf(p_ptr));

	/* No more perfection / EA / and other nice daggers from this one -C. Blue */
	/* If weapon has good boni, remove all ego abilities */
	if ((o_ptr->pval > 0) || (o_ptr->to_a > 0) || (o_ptr->to_h > 0) || (o_ptr->to_d > 0)) {
		/* if (o_ptr->name2 == EGO_SLAYING_WEAPON || o_ptr->name2b == EGO_SLAYING_WEAPON) */
		o_ptr->dd = k_info[o_ptr->k_idx].dd;
		o_ptr->ds = k_info[o_ptr->k_idx].ds;
		o_ptr->name1 = o_ptr->name2 = o_ptr->name2b = o_ptr->name3 = 0;
	}
        /* Reverse good bonuses */
	if (o_ptr->bpval > 0) o_ptr->bpval = 0;
	if (o_ptr->pval > 0) o_ptr->pval = 0;
	if (o_ptr->to_a > 0) o_ptr->to_a = 0;
	if (o_ptr->to_h > 0) o_ptr->to_h = -o_ptr->to_h;
	if (o_ptr->to_d > 0) o_ptr->to_d = -o_ptr->to_d;

	object_desc(Ind, i_name, o_ptr, TRUE, 0);
	dd = dd * MISSILE_TRAP_FACTOR / 100;

	if (num == 1)
		//      msg_format(Ind, "Suddenly %s hits you!", i_name);
		msg_format(Ind, "Suddenly %s is shot at you!", i_name);
	else
		//      msg_format(Ind, "Suddenly %s hit you!", i_name);
		msg_format(Ind, "Suddenly %s are shot at you!", i_name);

	for (i = 0; i < num; i++) {
		if ((p_ptr->kinetic_shield
#ifdef ENABLE_OCCULT
		    || p_ptr->spirit_shield
#endif
		    ) && p_ptr->csp >= (dd * ds) / 20) {
			/* drains mana on hit */
			p_ptr->csp -= (dd * ds) / 20;
			p_ptr->redraw |= PR_MANA;
			/* test for deflection */
#ifdef ENABLE_OCCULT
			if (p_ptr->kinetic_shield) {
				if (magik(50)) {
					msg_print(Ind, "Your kinetic shield deflects the attack!");
					continue;
				}
			} else if (p_ptr->spirit_shield) {
				if (magik(p_ptr->spirit_shield_pow)) {
					msg_print(Ind, "Your spirit shield deflects the attack!");
					continue;
				}
			}
#else
			if (magik(50)) {
				msg_print(Ind, "Your kinetic shield deflects the attack!");
				continue;
			}
#endif
		}
		if (p_ptr->reflect && magik(60)) {
			msg_print(Ind, "You deflect the attack!");
			continue;
		}
		if (magik(apply_block_chance(p_ptr, p_ptr->shield_deflect + 10))) {
			msg_print(Ind, "You deflect the attack!");
#ifdef USE_SOUND_2010
			if (p_ptr->sfx_defense) sound(Ind, "block_shield_projectile", NULL, SFX_TYPE_ATTACK, FALSE);
#endif

			continue;
		}
		if ((dodge > 0) && magik(dodge)) {
			msg_format(Ind, "\377%cYou dodge the attack!", COLOUR_DODGE_GOOD);
			continue;
		}

		dam = damroll(dd, ds);
		msg_format(Ind, "You are hit for \377o%d\377w damage!", dam);
		take_hit(Ind, dam, format("a %s", name), 0);
		redraw_stuff(Ind);
		if (pdam > 0) {
			if (!(p_ptr->resist_pois || p_ptr->oppose_pois || p_ptr->immune_poison))
				(void)set_poisoned(Ind, p_ptr->poisoned + pdam, 0);
		}
	}

	drop_near(0, o_ptr, -1, &p_ptr->wpos, p_ptr->py, p_ptr->px);

	return TRUE;
}

/*
 * this function handles a "breath" type trap - acid bolt, lightning balls etc.
 */
static bool player_handle_breath_trap(int Ind, s16b rad, s16b type, u16b trap) {
	player_type *p_ptr = Players[Ind];
	trap_kind *t_ptr = &t_info[trap];
	bool ident;
	s16b my_dd, my_ds, dam;

	my_dd = t_ptr->dd;
	my_ds = t_ptr->ds;

	/* these traps gets nastier as levels progress */
	if (getlevel(&p_ptr->wpos)> (2* t_ptr->minlevel)) {
		my_dd += (getlevel(&p_ptr->wpos) / 15);
		my_ds += (getlevel(&p_ptr->wpos) / 15);
	}
	dam = damroll(my_dd, my_ds);

	ident = project(PROJECTOR_TRAP, rad, &p_ptr->wpos, p_ptr->py, p_ptr->px, dam, type,
	    PROJECT_NORF | PROJECT_KILL | PROJECT_JUMP | PROJECT_GRID | PROJECT_ITEM | PROJECT_NODO, t_name + t_ptr->name);
	    // | PROJECT_KILL | PROJECT_JUMP
	return (ident);
}

/*
 * This function damages the player by a trap
 */
static void trap_hit(int Ind, s16b trap) {
	s16b dam;
	trap_kind *t_ptr = &t_info[trap];

	dam = damroll(t_ptr->dd, t_ptr->ds);
	take_hit(Ind, dam, format("%s %s", is_a_vowel(*(t_name + t_ptr->name)) ? "an" : "a", t_name + t_ptr->name), 0);
}

/*
 * this function activates one trap type, and returns
 * a bool indicating if this trap is now identified
 */
bool player_activate_trap_type(int Ind, s16b y, s16b x, object_type *i_ptr, s16b item) {
	player_type *p_ptr = Players[Ind];
	worldpos *wpos = &p_ptr->wpos;
	bool ident = FALSE, never_id = FALSE;
	s16b trap = 0, vanish;
	//dungeon_info_type *d_ptr = &d_info[dungeon_type];

	s16b k, l, glev = getlevel(wpos);
	cave_type *c_ptr;
	trap_kind *t_ptr;

	object_kind *ok_ptr;

	struct dun_level *l_ptr = getfloor(wpos);
	bool no_summon = (l_ptr && (l_ptr->flags2 & LF2_NO_SUMMON));
	//bool no_teleport = (l_ptr && (l_ptr->flags2 & LF2_NO_TELE));

	/* Paranoia */
	cave_type **zcave;
	if (!in_bounds(y, x)) return(FALSE);

	if (!(zcave = getcave(wpos))) return(FALSE);
	c_ptr = &zcave[y][x];

#ifdef USE_SOUND_2010
	sound(Ind, "trap_setoff", NULL, SFX_TYPE_MISC, FALSE);
#endif

	if (item < 0) trap = GetCS(c_ptr, CS_TRAPS)->sc.trap.t_idx;

	if (i_ptr == NULL) {
		if (c_ptr->o_idx == 0) i_ptr = NULL;
		else i_ptr = &o_list[c_ptr->o_idx];
	} else trap = i_ptr->pval;

	if (trap == TRAP_OF_RANDOM_EFFECT) {
		never_id = TRUE;
		trap = TRAP_OF_ALE;
		for (l = 0; l < 99 ; l++) {
			k = rand_int(MAX_T_IDX);
			if (!t_info[k].name || t_info[k].minlevel > glev ||
			    k == TRAP_OF_ACQUIREMENT || k == TRAP_OF_RANDOM_EFFECT)
				continue;

			trap = k;
			break;
		}
	}

	t_ptr = &t_info[trap];
	vanish = t_ptr->vanish;
	never_id = never_id || (t_ptr->flags & FTRAP_NO_ID);

	s_printf("Trap %d (%s) triggered by %s.\n", trap, t_name + t_info[trap].name, p_ptr->name);

	switch (trap) {
#ifdef ARCADE_SERVER
		case TRAP_OF_SPREAD: {
			if (p_ptr->game == 1) {
				p_ptr->arc_a = 30;
				p_ptr->arc_b = 1;
				msg_print(Ind, "Spread shot, w00t.");
			} else if (p_ptr->game == 3) {
				p_ptr->arc_b += 3;
				msg_print(Ind, "+3 Length!");
			}
			destroy_traps_doors_touch(Ind, 0);
			break;
		}

		case TRAP_OF_LASER: {
			p_ptr->arc_a = 30;
			p_ptr->arc_b = 2;
			destroy_traps_doors_touch(Ind, 0);
			msg_print(Ind, "You got a frickin laser beam!");
			break;
		}

		case TRAP_OF_ROCKETS: {
			p_ptr->arc_a = 30;
			p_ptr->arc_b = 3;
			p_ptr->arc_c = 0;
			p_ptr->arc_d = 1;

			destroy_traps_doors_touch(Ind, 0);
			msg_print(Ind, "You're a rocket man.");
			break;
		}

		case TRAP_OF_HEALING: {
			destroy_traps_doors_touch(Ind, 0);
			hp_player(Ind, p_ptr->mhp-p_ptr->chp);
			break;
		}
#endif

		/* stat traps */
		case TRAP_OF_WEAKNESS_I:       ident = do_dec_stat(Ind, A_STR, STAT_DEC_TEMPORARY); break;
		case TRAP_OF_WEAKNESS_II:      ident = do_dec_stat(Ind, A_STR, STAT_DEC_NORMAL); break;
		case TRAP_OF_WEAKNESS_III:     ident = do_dec_stat(Ind, A_STR, STAT_DEC_PERMANENT); break;
		case TRAP_OF_INTELLIGENCE_I:   ident = do_dec_stat(Ind, A_INT, STAT_DEC_TEMPORARY); break;
		case TRAP_OF_INTELLIGENCE_II:  ident = do_dec_stat(Ind, A_INT, STAT_DEC_NORMAL); break;
		case TRAP_OF_INTELLIGENCE_III: ident = do_dec_stat(Ind, A_INT, STAT_DEC_PERMANENT); break;
		case TRAP_OF_WISDOM_I:         ident = do_dec_stat(Ind, A_WIS, STAT_DEC_TEMPORARY); break;
		case TRAP_OF_WISDOM_II:        ident = do_dec_stat(Ind, A_WIS, STAT_DEC_NORMAL); break;
		case TRAP_OF_WISDOM_III:       ident = do_dec_stat(Ind, A_WIS, STAT_DEC_PERMANENT); break;
		case TRAP_OF_FUMBLING_I:       ident = do_dec_stat(Ind, A_DEX, STAT_DEC_TEMPORARY); break;
		case TRAP_OF_FUMBLING_II:      ident = do_dec_stat(Ind, A_DEX, STAT_DEC_NORMAL); break;
		case TRAP_OF_FUMBLING_III:     ident = do_dec_stat(Ind, A_DEX, STAT_DEC_PERMANENT); break;
		case TRAP_OF_WASTING_I:        ident = do_dec_stat(Ind, A_CON, STAT_DEC_TEMPORARY); break;
		case TRAP_OF_WASTING_II:       ident = do_dec_stat(Ind, A_CON, STAT_DEC_NORMAL); break;
		case TRAP_OF_WASTING_III:      ident = do_dec_stat(Ind, A_CON, STAT_DEC_PERMANENT); break;
		case TRAP_OF_BEAUTY_I:         ident = do_dec_stat(Ind, A_CHR, STAT_DEC_TEMPORARY); break;
		case TRAP_OF_BEAUTY_II:        ident = do_dec_stat(Ind, A_CHR, STAT_DEC_NORMAL); break;
		case TRAP_OF_BEAUTY_III:       ident = do_dec_stat(Ind, A_CHR, STAT_DEC_PERMANENT); break;

		/* Trap of Curse Weapon */
		case TRAP_OF_CURSE_WEAPON:
			if (rand_int(120) < p_ptr->skill_sav) {
				msg_print(Ind, "You resist the effects!");
				break;
			}
			ident = curse_weapon(Ind);
			destroy_chest(i_ptr);
			break;

		/* Trap of Curse Armor */
		case TRAP_OF_CURSE_ARMOR:
			if (rand_int(120) < p_ptr->skill_sav) {
				msg_print(Ind, "You resist the effects!");
				break;
			}
			ident = curse_armor(Ind);
			destroy_chest(i_ptr);
			break;

		/* Earthquake Trap */
		case TRAP_OF_EARTHQUAKE:
			msg_print(Ind, "As you touch the trap, the ground starts to shake.");
			earthquake(wpos, y, x, 10);
			ident = TRUE;
			destroy_chest(i_ptr);
			break;

		/* Poison Needle Trap */
		case TRAP_OF_POISON_NEEDLE:
			if (!(p_ptr->resist_pois || p_ptr->oppose_pois || p_ptr->immune_poison)) {
				msg_print(Ind, "You prick yourself on a poisoned needle.");
				(void)set_poisoned(Ind, p_ptr->poisoned + rand_int(15) + 10, 0);
				ident = TRUE;
			} else {
				msg_print(Ind, "You prick yourself on a needle.");
			}
			break;

		/* Summon Monster Trap */
		case TRAP_OF_SUMMON_MONSTER:
			msg_print(Ind, "A spell hangs in the air.");
			if (no_summon) break;
			summon_override_checks = SO_IDDC;
			//for (k = 0; k < randint(3); k++) ident |= summon_specific(y, x, max_dlv[dungeon_type], 0, SUMMON_UNDEAD, 1);	// max?
			for (k = 0; k < randint(3); k++) ident |= summon_specific(wpos, y, x, glev, 100, SUMMON_ALL_U98, 1, 0);
			summon_override_checks = SO_NONE;
#ifdef USE_SOUND_2010
			if (ident) sound_near_site(y, x, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
#endif
			break;

		/* Summon Undead Trap */
		case TRAP_OF_SUMMON_UNDEAD:
			msg_print(Ind, "A mighty spell hangs in the air.");
			if (no_summon) break;
			summon_override_checks = SO_IDDC;
			for (k = 0; k < randint(3); k++) ident |= summon_specific(wpos, y, x, glev, 0, SUMMON_UNDEAD, 1, 0);
			summon_override_checks = SO_NONE;
#ifdef USE_SOUND_2010
			if (ident) sound_near_site(y, x, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
#endif
			break;

		/* Summon Greater Undead Trap */
		case TRAP_OF_SUMMON_GREATER_UNDEAD:
			msg_print(Ind, "An old and evil spell hangs in the air.");
			if (no_summon) break;
			summon_override_checks = SO_IDDC;
			for (k = 0; k < randint(3); k++) ident |= summon_specific(wpos, y, x, glev, 0, SUMMON_HI_UNDEAD, 1, 0);
			summon_override_checks = SO_NONE;
#ifdef USE_SOUND_2010
			if (ident) sound_near_site(y, x, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
#endif
			destroy_chest(i_ptr);
			break;

		/* Teleport Trap */
		case TRAP_OF_TELEPORT:
		{
			int chance = (195 - p_ptr->skill_sav) / 2;
			if (p_ptr->res_tele) chance = 50;

			if (p_ptr->anti_tele || check_st_anchor(wpos, y, x) || magik(chance)) break;
			msg_print(Ind, "The world whirls around you.");
			teleport_player(Ind, RATIO*67, TRUE);
			ident = TRUE;
			break;
		}

		/* Paralyzing Trap */
		case TRAP_OF_PARALYZING:
			if (!p_ptr->free_act) {
				msg_print(Ind, "You touch a poisoned part and can't move.");
				(void)set_paralyzed(Ind, p_ptr->paralyzed + rand_int(10) + 10);
				ident = TRUE;
			} else {
				msg_print(Ind, "You prick yourself on a needle.");
			}
			break;

		/* Explosive Device */
		case TRAP_OF_EXPLOSIVE_DEVICE:
			msg_print(Ind, "A hidden explosive device explodes in your face.");
#ifdef USE_SOUND_2010
			sound(Ind, "fireworks_norm", NULL, SFX_TYPE_MISC, TRUE); //"detonation"
#endif
			take_hit(Ind, damroll(5, 8), "an explosion", 0);
			ident = TRUE;
			destroy_chest(i_ptr);
			break;

		/* Teleport Away Trap */
		case TRAP_OF_TELEPORT_AWAY:
		{
			int i, ty, tx, item, amt, rad = glev / 20 - 1;
			object_type *o_ptr;
			cave_type *cc_ptr;

			if (rad < 0) rad = 0;

			for (i = 0; i < tdi[rad]; i++) {
				ty = y + tdy[i];
				tx = x + tdx[i];
				if (!in_bounds(ty, tx)) continue;
				cc_ptr = &zcave[ty][tx];

				/* teleport away all items */
				while (cc_ptr->o_idx != 0) {
					item = cc_ptr->o_idx;
					amt = o_list[item].number;
					o_ptr = &o_list[item];

					ident = do_trap_teleport_away(Ind, o_ptr, ty, tx);

					floor_item_increase(item, -amt);
					floor_item_optimize(item);
				}
			}
			destroy_chest(i_ptr); 
			break;
		}

		 /* Lose Memory Trap */
		case TRAP_OF_LOSE_MEMORY:
			if (!p_ptr->keep_life) lose_exp(Ind, p_ptr->exp / 10);
			ident |= dec_stat(Ind, A_WIS, rand_int(10) + 10, STAT_DEC_NORMAL);
			ident |= dec_stat(Ind, A_INT, rand_int(10) + 10, STAT_DEC_NORMAL);
			if (!p_ptr->resist_conf) {
				if (set_confused(Ind, p_ptr->confused + rand_int(20) + 20))
					ident = TRUE;
			}
			if (ident)
				msg_print(Ind, "You suddenly don't remember what you were doing.");
			else
				msg_print(Ind, "You feel an alien force probing your mind.");
			break;

		 /* Bitter Regret Trap */
		case TRAP_OF_BITTER_REGRET:
			msg_print(Ind, "An age-old and hideous sounding spell reverbs off the walls.");
			//		ident |= dec_stat(Ind, A_DEX, 25, TRUE);	// TRUE..!?
			ident |= dec_stat(Ind, A_DEX, 25, STAT_DEC_NORMAL);
			ident |= dec_stat(Ind, A_WIS, 25, STAT_DEC_NORMAL);
			ident |= dec_stat(Ind, A_CON, 25, STAT_DEC_NORMAL);
			ident |= dec_stat(Ind, A_STR, 25, STAT_DEC_NORMAL);
			ident |= dec_stat(Ind, A_CHR, 25, STAT_DEC_NORMAL);
			ident |= dec_stat(Ind, A_INT, 25, STAT_DEC_NORMAL);
			destroy_chest(i_ptr);
			break;

		/* Bowel Cramps Trap */
		case TRAP_OF_BOWEL_CRAMPS:
			if (p_ptr->suscep_life || p_ptr->prace == RACE_ENT) break;

			msg_print(Ind, "A wretched smelling gas cloud upsets your stomach.");
			take_hit(Ind, 1, "bowel cramps", 0);
			if (p_ptr->chp < p_ptr->mhp) /* *invincibility* fix */
				if (p_ptr->food >= PY_FOOD_FAINT)
					(void)set_food(Ind, PY_FOOD_FAINT - 1);
					//(void)set_food(Ind, PY_FOOD_STARVE - 1);
					//(void)set_food(Ind, PY_FOOD_STARVE + 20);
			(void)set_poisoned(Ind, 0, 0);
			if (!p_ptr->free_act && !p_ptr->slow_digest)
				(void)set_paralyzed(Ind, p_ptr->paralyzed + rand_int(3) + 3);
			ident = TRUE;
			break;

		/* Blindness/Confusion Trap */
		case TRAP_OF_BLINDNESS_CONFUSION:
			msg_print(Ind, "A powerful magic protected this.");
			if (!p_ptr->resist_blind) {
				(void)set_blind(Ind, p_ptr->blind + rand_int(100) + 100);
				ident = TRUE;
			}
			if (!p_ptr->resist_conf) {
				if (set_confused(Ind, p_ptr->confused + rand_int(20) + 15))
					ident = TRUE;
			}
			break;

		/* Aggravation Trap */
		case TRAP_OF_AGGRAVATION:
#ifdef USE_SOUND_2010
			sound_near(Ind, "shriek", NULL, SFX_TYPE_MON_SPELL);
			msg_print(Ind, "\377RYou hear a high-pitched humming noise echoing through the dungeons.");
			msg_print_near(Ind, "\377RYou hear a high-pitched humming noise echoing through the dungeons.");
#else
			/* wonder why this one was actually a hollow noise instead of the usual shriek.. */
			msg_print(Ind, "\377RYou hear a hollow noise echoing through the dungeons.");
			msg_print_near(Ind, "\377RYou hear a hollow noise echoing through the dungeons.");
#endif
			aggravate_monsters(Ind, -1);
			break;

		/* Multiplication Trap */
		case TRAP_OF_MULTIPLICATION:
			msg_print(Ind, "You hear a loud click.");
			for (k = -1; k <= 1; k++)
				for (l = -1; l <= 1; l++) {
					/* let's keep its spot empty if the multiplication trap runs out? */
					//if (k == 0 && l == 0) continue;

					if (in_bounds(y + l, x + k) && (!GetCS(&zcave[y + l][x + k], CS_TRAPS)))
						place_trap(wpos, y + l, x + k, -1);
				}
			ident = TRUE;
			break;

		/* Steal Item Trap */
		case TRAP_OF_STEAL_ITEM:
			/* please note that magical stealing is not so easily circumvented */
			if ((!p_ptr->paralyzed &&
			    (rand_int(160 + UNAWARENESS(p_ptr)) <
			    (adj_dex_safe[p_ptr->stat_ind[A_DEX]] + p_ptr->lev))) ||
			    (TOOL_EQUIPPED(p_ptr) == SV_TOOL_THEFT_PREVENTION &&
			     magik (75)) ) {
				/* Saving throw message */
				msg_print(Ind, "Your backpack seems to vibrate strangely!");
				break;
			}

			/* Find an item */
			for (k = 0; k < rand_int(10); k++) {
				char i_name[ONAME_LEN];

				/* Pick an item */
				s16b i = rand_int(INVEN_PACK);

				/* Obtain the item */
				object_type *j_ptr = &p_ptr->inventory[i], *q_ptr, forge;

				/* Accept real items */
				if (!j_ptr->k_idx) continue;

				/* Don't steal artifacts  -CFT */
				if (artifact_p(j_ptr)) continue;

				/* Get a description */
				object_desc(Ind, i_name, j_ptr, FALSE, 3);

				/* Message */
				msg_format(Ind, "\376\377o%sour %s (%c) was stolen!",
						((j_ptr->number > 1) ? "One of y" : "Y"),
						i_name, index_to_label(i));

				/* Create the item */
				q_ptr = &forge;
				object_copy(q_ptr, j_ptr);
				q_ptr->number = 1;
				if (is_magic_device(j_ptr->tval)) divide_charged_item(q_ptr, j_ptr, 1);

				/* Drop it somewhere */
				do_trap_teleport_away(Ind, q_ptr, y, x);

				inven_item_increase(Ind, i,-1);
				inven_item_optimize(Ind, i);
				ident = TRUE;
			}
			break;

		/* Summon Fast Quylthulgs Trap */
		case TRAP_OF_SUMMON_FAST_QUYLTHULGS:
			if (no_summon) break;
			summon_override_checks = SO_IDDC;
			for (k = 0; k < randint(3); k++)
				ident |= summon_specific(wpos, y, x, glev, 0, SUMMON_QUYLTHULG, 1, 0);
			summon_override_checks = SO_NONE;
			if (ident) {
				msg_print(Ind, "You suddenly have company.");
				if (!(p_ptr->mindboost && magik(p_ptr->mindboost_power)))
					(void)set_slow(Ind, p_ptr->slow + randint(25) + 15);
#ifdef USE_SOUND_2010
				sound_near_site(y, x, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
#endif
			}
			break;

		/* Trap of Sinking */
		case TRAP_OF_SINKING:
			/* MEGAHACK: Ignore Wilderness trap doors. */
			vanish = 100;
			if (!can_go_down(wpos, 0xF)) {
				//msg_print(Ind, "\377GYou feel quite certain something really awful just happened..");
				break;
			}
			ident = TRUE;
			vanish = 0;
			if (p_ptr->levitate) {
				msg_print(Ind, "You found a trap door!");
				break;
			}
			msg_print(Ind, "You fell through a trap door!");
			if (p_ptr->feather_fall || p_ptr->tim_wraith) {
				msg_print(Ind, "You float gently down to the next level.");
			} else {
				/* Inventory damage (Hack - use 'cold' type) */
				inven_damage(Ind, set_cold_destroy, damroll(2, 8));

				//int dam = damroll(2, 8);
				//take_hit(Ind, dam, name, 0);
				take_hit(Ind, damroll(2, 8), "a trap door", 0);
				//take_sanity_hit(Ind, damroll(1, 2), "a trap door");
			}
			do_player_trap_change_depth(Ind, -1);
			break;

		/* Trap of Mana Drain */
		case TRAP_OF_MANA_DRAIN:
			if (p_ptr->csp > 0) {
				p_ptr->csp = 0;
				p_ptr->csp_frac = 0;
				p_ptr->redraw |= (PR_MANA);
				msg_print(Ind, "You sense a great loss.");
				ident = TRUE;
			} else {
				if (p_ptr->msp == 0) /* no sense saying this unless you never have mana */
					msg_format(Ind, "Suddenly you feel glad you're only a %s.", p_ptr->cp_ptr->title);
				else
					msg_print(Ind, "Your head feels dizzy for a moment.");
			}
			break;

		/* Trap of Missing Money */
		case TRAP_OF_MISSING_MONEY:
		{
			u32b gold = (p_ptr->au / 10) + randint(25);

			if (gold < 2) gold = 2;
			if (gold > 5000) gold = (p_ptr->au / 20) + randint(3000);
			if (gold > (u32b) p_ptr->au) gold = p_ptr->au;
			p_ptr->au -= gold;
			if ((gold <= 0) ||
			    (TOOL_EQUIPPED(p_ptr) == SV_TOOL_MONEY_BELT && magik(100)) ) {
				msg_print(Ind, "You feel something touching you.");
			} else if (p_ptr->au) {
				msg_print(Ind, "Your purse feels lighter.");
				msg_format(Ind, "\376\377o%d coins were stolen!", (int)gold);
				ident = TRUE;
			} else {
				msg_print(Ind, "Your purse feels empty.");
				msg_print(Ind, "\376\377oAll of your coins were stolen!");
				ident = TRUE;
			}
			p_ptr->redraw |= (PR_GOLD);
			break;
		}
		/* Trap of No Return */
		case TRAP_OF_NO_RETURN: //basically ok, but we need fireproof WoR in BM before this gets enabled!
#if 1
		{
			object_type *j_ptr;
			s16b j;
			u32b f1, f2, f3, f4, f5, f6, esp;

			for (j = 0; j < INVEN_WIELD; j++) {
				if (!p_ptr->inventory[j].k_idx) continue;
				j_ptr = &p_ptr->inventory[j];
				if ((j_ptr->tval == TV_SCROLL)
				    && (j_ptr->sval == SV_SCROLL_WORD_OF_RECALL)) {
					/* handle 'fireproof' scrolls */
					object_flags(j_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
					if (f3 & TR3_IGNORE_FIRE) continue;

					inven_item_increase(Ind, j, -j_ptr->number);
					inven_item_optimize(Ind, j);
					combine_pack(Ind);
					reorder_pack(Ind);
					if (!ident)
						msg_print(Ind, "A small fire works its way through your backpack. Some scrolls are burnt.");
					else
						msg_print(Ind, "The fire hasn't finished.");
					ident = TRUE;
				}
				else if ((j_ptr->tval == TV_ROD) && (j_ptr->sval == SV_ROD_RECALL)) {
					discharge_rod(j_ptr, 999); /* a long time */
					if (!ident) msg_print(Ind, "You feel the air stabilize around you.");
					ident = TRUE;
				}
			}
			if ((!ident) && (p_ptr->word_recall)) {
				msg_print(Ind, "You feel like staying around.");
				p_ptr->word_recall = 0;
				p_ptr->redraw |= (PR_DEPTH);
				ident = TRUE;
			}
			break;
		}
#endif
		/* Trap of Silent Switching */
		case TRAP_OF_SILENT_SWITCHING:
		{
			s16b i,j,slot1,slot2;
			object_type *j_ptr, *k_ptr;
			for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
				j_ptr = &p_ptr->inventory[i];
				if (!j_ptr->k_idx) continue;
				slot1 = wield_slot(Ind, j_ptr);
				for (j = 0; j < INVEN_WIELD; j++) {
					k_ptr = &p_ptr->inventory[j];
					if (!k_ptr->k_idx) continue;
					/* this is a crude hack, but it prevent wielding 6 torches... */
					if (k_ptr->number > 1) continue;
					slot2 = wield_slot(Ind, k_ptr);
					/* a chance of 4 in 5 of switching something, then 2 in 5 to do it again */
					if (slot1 && (slot1 == slot2) && (rand_int(100) < (80 - ident * 40))) {
						object_type tmp_obj;
						tmp_obj = p_ptr->inventory[j];
						p_ptr->inventory[j] = p_ptr->inventory[i];
						p_ptr->inventory[i] = tmp_obj;
						ident = TRUE;
					}
				}
			}
			if (ident) {
				p_ptr->update |= (PU_BONUS);
				p_ptr->update |= (PU_TORCH);
				p_ptr->update |= (PU_MANA);
				msg_print(Ind, "You somehow feel an other person.");
			} else {
				msg_print(Ind, "You feel a lack of useful items.");
			}
			break;
		}
		/* Trap of Walls */
		case TRAP_OF_WALLS:
			//         ident = player_handle_trap_of_walls();
			/* let's do a quick job ;) */
			ident = player_handle_breath_trap(Ind, 1 + glev / 40, GF_STONE_WALL, TRAP_OF_WALLS);
			break;

		/* Trap of Calling Out */
		case TRAP_OF_CALLING_OUT:
			ident = do_player_trap_call_out(Ind);
			if (!ident) {
				/* Increase "afraid" */
				if (p_ptr->resist_fear)
					msg_print(Ind, "You feel as if you had a nightmare!");
				else if (rand_int(100) < p_ptr->skill_sav)
					msg_print(Ind, "You remember having a nightmare!");
				else {
					if (set_afraid(Ind, p_ptr->afraid + 3 + randint(40)))
						msg_print(Ind, "You have a vision of a powerful enemy.");
				}
			}
			break;

		/* Trap of Sliding */
		case TRAP_OF_SLIDING:
			/* ? */
			destroy_chest(i_ptr); /* ah well, let's at least do this then */
			break;

		/* Trap of Charges Drain */
		case TRAP_OF_CHARGES_DRAIN:
		{
			s16b         i;
			object_type *j_ptr;
			/* Find an item */
			for (k = 0; k < 10; k++) {
				i = rand_int(INVEN_PACK);
				j_ptr = &p_ptr->inventory[i];
				/* Drain charged wands/staffs */
				if (((j_ptr->tval == TV_STAFF) || (j_ptr->tval == TV_WAND)) &&
				    (j_ptr->pval)) {
					ident = TRUE;
					j_ptr->pval = j_ptr->pval / (randint(4)+1);
					/* Window stuff */
					p_ptr->window |= PW_INVEN;
					/* Combine / Reorder the pack */
					p_ptr->notice |= (PN_COMBINE | PN_REORDER);
					if (randint(10) > 3) break; /* 60% chance of only 1 */
				} else if (j_ptr->tval == TV_ROD) {
					ident = TRUE;
					discharge_rod(j_ptr, 20 + rand_int(20));
					/* Window stuff */
					p_ptr->window |= PW_INVEN;
					/* Combine / Reorder the pack */
					p_ptr->notice |= (PN_COMBINE | PN_REORDER);
				}
			}
			if (ident)
				msg_print(Ind, "Your backpack seems to be turned upside down.");
			else
				msg_print(Ind, "You hear a wail of great disappointment.");

			destroy_chest(i_ptr); 
			break;
		}
		/* Trap of Stair Movement */
		case TRAP_OF_STAIR_MOVEMENT:
#if 0	// after f_info!
		{
			s16b cx,cy,i,j;
			s16b cnt = 0;
			s16b cnt_seen = 0;
			s16b tmps, tmpx;
			u32b tmpf;
			bool seen = FALSE;
			s16b index_x[20],index_y[20]; /* 20 stairs per level is enough? */
			cave_type *cv_ptr;

			for (cx = 0; cx < p_ptr->cur_wid; cx++) {
				for (cy = 0; cy < p_ptr->cur_hgt; cy++) {
					cv_ptr = &zcave[cy][cx];
#if 0
					if ((cv_ptr->feat != FEAT_LESS) &&
					    (cv_ptr->feat != FEAT_MORE) &&
					    (cv_ptr->feat != FEAT_SHAFT_UP) &&
					    (cv_ptr->feat != FEAT_SHAFT_DOWN)) continue;
#endif
					if ((cv_ptr->feat != FEAT_LESS) &&
					    (cv_ptr->feat != FEAT_MORE)) continue;

					index_x[cnt] = cx;
					index_y[cnt] = cy;
					cnt++;
					if (cnt == 20) break;
				}
				if (cnt == 20) break;
			}
			if (cnt == 0) break;

			for (i = 0; i < cnt; i++) {
				for (j = 0; j < 10; j++) { /* try 10 times to relocate */
					cave_type *c_ptr2 = &zcave[index_y[i]][index_x[i]];

					cx = rand_int(p_ptr->cur_wid);
					cy = rand_int(p_ptr->cur_hgt);

					if ((cx == index_x[i]) || (cy == index_y[i])) continue;
					if (!cave_valid_bold(zcave, cy, cx) || zcave[cy][cx].o_idx != 0) continue;

					/* don't put anything in vaults/nests/pits */
					if (zcave[cy][cx].info & CAVE_ICKY) continue;
					if (zcave[cy][cx].info & CAVE_NEST_PIT) continue;

					tmpf = zcave[cy][cx].feat;
					tmps = zcave[cy][cx].info;
					tmpx = zcave[cy][cx].mimic;
					zcave[cy][cx].feat = c_ptr2->feat;
					zcave[cy][cx].info = c_ptr2->info;
					zcave[cy][cx].mimic = c_ptr2->mimic;
					c_ptr2->feat  = tmpf;
					c_ptr2->info  = tmps;
					c_ptr2->mimic = tmpx;

					/* if we are placing walls in rooms, make them rubble instead */
					if ((c_ptr2->info & CAVE_ROOM) &&
					    (c_ptr2->feat >= FEAT_WALL_EXTRA) &&
					    (c_ptr2->feat <= FEAT_PERM_SOLID)) {
						cave[index_y[i]][index_x[i]].feat = FEAT_RUBBLE;
					}
					if (player_has_los_bold(Ind, cy, cx)) {
						note_spot(Ind, cy,cx);
						lite_spot(Ind, cy,cx);
						seen = TRUE;
					} else {
						cave[cy][cx].info &= ~CAVE_MARK;
					}
					if (player_has_los_bold(index_y[i],index_x[i])) {
						note_spot(index_y[i],index_x[i]);
						lite_spot(index_y[i],index_x[i]);
						seen = TRUE;
					} else {
						cave[index_y[i]][index_x[i]].info &= ~CAVE_MARK;
					}
					break;
				}
				if (seen) cnt_seen++;
				seen = FALSE;
			} /* cnt loop */

			ident = (cnt_seen > 0);
			if ((ident) && (cnt_seen>1))
				msg_print(Ind, "You see some stairs move.");
					else if (ident)
						msg_print(Ind, "You see a stair move.");
					else
						msg_print(Ind, "You hear distant scraping noises.");
					p_ptr->redraw |= PR_MAP;
				} /* any stairs found */
			} else { /* are we on level 99 */
				msg_print(Ind, "You have a feeling that this trap could be dangerous.");
			}
		}
#endif	// 0
		break;

		/* Trap of New Trap */
		case TRAP_OF_NEW:
		// maybe could become more interesting if it spawns several more traps at random locations? :)
		{
			struct c_special *cs_ptr;
			cs_ptr = GetCS(c_ptr, CS_TRAPS);
			/* if we're on a floor or on a door, place a new trap */
			if ((item == -1) || (item == -2)) {
				cs_erase(c_ptr, cs_ptr);
				place_trap(wpos, y , x, 0);
				if (player_has_los_bold(Ind, y, x)) {
					note_spot(Ind, y, x);
					lite_spot(Ind, y, x);
				}
			} else {
				/* re-trap the chest */
				//				place_trap(wpos, y , x, 0);
				place_trap_object(i_ptr);
			}
			msg_print(Ind, "You hear a noise, and then its echo.");
			ident = FALSE;

			break;
		}

		/* Trap of Acquirement */
		case TRAP_OF_ACQUIREMENT:
		{
			struct c_special *cs_ptr;
			/* Get a nice thing */
			msg_print(Ind, "You notice something falling off the trap.");
			acquirement(wpos, y, x, 1, TRUE, TRUE, make_resf(p_ptr));
			//			 acquirement(wpos, y, x, 1, TRUE, FALSE);	// last is 'known' flag

			cs_ptr = GetCS(c_ptr, CS_TRAPS);
			cs_erase(c_ptr, cs_ptr);
			/* if we're on a floor or on a door, place a new trap */
			if ((item == -1) || (item == -2)) {
				place_trap(wpos, y , x, 0);
				if (player_has_los_bold(Ind, y, x)) {
					note_spot(Ind, y, x);
					lite_spot(Ind, y, x);
				}
			} else {
				/* re-trap the chest (??) */
				//				 place_trap(wpos, y , x, 0);
				place_trap_object(i_ptr);
			}
			msg_print(Ind, "You hear a noise, and then its echo.");
			/* Never known */
			ident = FALSE;

			break;
		}

		/* Trap of Scatter Items */
		case TRAP_OF_SCATTER_ITEMS:
#if 1
			ident = do_player_scatter_items(Ind, 70, 15);
			break;
#endif

		/* Trap of Decay */
		case TRAP_OF_DECAY:
			destroy_chest(i_ptr);
			break;

		/* Trap of Wasting Wands */
		case TRAP_OF_WASTING_WANDS:
			{
				s16b i;
				object_type *j_ptr;

				for (i = 0; i < INVEN_PACK; i++) {
					if (!p_ptr->inventory[i].k_idx) continue;
					j_ptr = &p_ptr->inventory[i];
					if (j_ptr->tval == TV_WAND) {
						if ((j_ptr->sval >= SV_WAND_NASTY_WAND) && (rand_int(5) == 1)) {
							if (object_known_p(Ind, j_ptr)) ident = TRUE;
							j_ptr->sval = rand_int(SV_WAND_NASTY_WAND);
							j_ptr->k_idx = lookup_kind(j_ptr->tval, j_ptr->sval);
							p_ptr->notice |= (PN_COMBINE | PN_REORDER);
							p_ptr->window |= PW_INVEN;
						}
						if ((j_ptr->sval >= SV_STAFF_NASTY_STAFF) && (rand_int(5) == 1)) {
							if (object_known_p(Ind, j_ptr)) ident = TRUE;
							j_ptr->sval = rand_int(SV_STAFF_NASTY_STAFF);
							j_ptr->k_idx = lookup_kind(j_ptr->tval, j_ptr->sval);
							p_ptr->notice |= (PN_COMBINE | PN_REORDER);
							p_ptr->window |= PW_INVEN;
						}
					}
				}
			}
			if (ident) msg_print(Ind, "You have lost trust in your backpack!");
			else msg_print(Ind, "You hear an echoing cry of rage.");
			break;

		/* Trap of Filling */
		case TRAP_OF_FILLING:
		{
			s16b nx, ny;
			for (nx = x - 8; nx <= x + 8; nx++) {
				for (ny = y - 8; ny <= y + 8; ny++) {
					if (!in_bounds (ny, nx)) continue;

					if (rand_int(distance(ny, nx, y, x)) > 3)
						place_trap(wpos, ny, nx, 0);
				}
			}
			msg_print(Ind, "The floor vibrates in a strange way.");
			ident = FALSE;
			break;
		}
		case TRAP_OF_DRAIN_SPEED: /* slightly insane, hence disabled */
#if 1
		{
			object_type *j_ptr;
			s16b j, chance = 75;
			u32b f1, f2, f3, f4, f5, f6, esp;

			//            for (j=0;j<INVEN_TOTAL;j++)
			/* From the foot ;D */
			for (j = INVEN_TOTAL - 1; j >= 0; j--) {
				/* don't bother the overflow slot */
				if (j == INVEN_PACK) continue;

				if (!p_ptr->inventory[j].k_idx) continue;
				j_ptr = &p_ptr->inventory[j];
				object_flags(j_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
				//               object_flags(j_ptr, &f1, &f2, &f3);

				/* is it a non-artifact speed item? */
				if ((!j_ptr->name1) && (f1 & TR1_SPEED) && (j_ptr->pval > 0)) {
					if (randint(100) < chance) {
						j_ptr->pval = j_ptr->pval / 2;
						if (j_ptr->pval == 0) j_ptr->pval--;
						chance /= 2;
						ident = TRUE;
					}
					inven_item_optimize(Ind, j);
				}
			}
			if (!ident)
				msg_print(Ind, "You feel some things in your pack vibrating.");
			else {
				combine_pack(Ind);
				reorder_pack(Ind);
				msg_print(Ind, "You suddenly feel you have time for self-reflection.");
				/* Recalculate bonuses */
				p_ptr->update |= (PU_BONUS);

				/* Recalculate mana */
				p_ptr->update |= (PU_MANA);

				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
			}
			break;
		}
#endif

		/*
		 * single missile traps
		*/

		case TRAP_OF_ARROW_I:
			ident = player_handle_missile_trap(Ind, 1, TV_ARROW, SV_AMMO_NORMAL, 4, 8, 0, "Arrow Trap"); break;
		case TRAP_OF_ARROW_II:
			ident = player_handle_missile_trap(Ind, 1, TV_BOLT, SV_AMMO_NORMAL, 10, 8, 0, "Bolt Trap"); break;
		case TRAP_OF_ARROW_III:
			ident = player_handle_missile_trap(Ind, 1, TV_ARROW, SV_AMMO_HEAVY, 12, 12, 0, "Seeker Arrow Trap"); break;
		case TRAP_OF_ARROW_IV:
			ident = player_handle_missile_trap(Ind, 1, TV_BOLT, SV_AMMO_HEAVY, 12, 16, 0, "Seeker Bolt Trap"); break;
		case TRAP_OF_POISON_ARROW_I:
			ident = player_handle_missile_trap(Ind, 1, TV_ARROW, SV_AMMO_NORMAL, 4, 8, 10 + randint(20), "Poison Arrow Trap"); break;
		case TRAP_OF_POISON_ARROW_II:
			ident = player_handle_missile_trap(Ind, 1, TV_BOLT, SV_AMMO_NORMAL, 10, 8, 15 + randint(30), "Poison Bolt Trap"); break;
		case TRAP_OF_POISON_ARROW_III:
			ident = player_handle_missile_trap(Ind, 1, TV_ARROW, SV_AMMO_HEAVY, 12, 12, 30 + randint(50), "Poison Seeker Arrow Trap"); break;
		case TRAP_OF_POISON_ARROW_IV:
			ident = player_handle_missile_trap(Ind, 1, TV_BOLT, SV_AMMO_HEAVY, 12, 16, 40 + randint(70), "Poison Seeker Bolt Trap"); break;
		case TRAP_OF_DAGGER_I:
			ident = player_handle_missile_trap(Ind, 1, TV_SWORD, SV_BROKEN_DAGGER, 4, 8, 0, "Dagger Trap"); break;
		case TRAP_OF_DAGGER_II:
			ident = player_handle_missile_trap(Ind, 1, TV_SWORD, SV_DAGGER, 10, 8, 0, "Dagger Trap"); break;
		case TRAP_OF_POISON_DAGGER_I:
			ident = player_handle_missile_trap(Ind, 1, TV_SWORD, SV_BROKEN_DAGGER, 4, 8, 15 + randint(20), "Poison Dagger Trap"); break;
		case TRAP_OF_POISON_DAGGER_II:
			ident = player_handle_missile_trap(Ind, 1, TV_SWORD, SV_DAGGER, 10, 8, 20 + randint(30), "Poison Dagger Trap"); break;

		/*
		 * multiple missile traps
		 * numbers range from 2 (level 0 to 14) to 10 (level 120 and up)
		 */

		case TRAP_OF_ARROWS_I:
			ident = player_handle_missile_trap(Ind, 2 + (glev / 30), TV_ARROW, SV_AMMO_NORMAL, 4, 8, 0, "Arrow Trap"); break;
		case TRAP_OF_ARROWS_II:
			ident = player_handle_missile_trap(Ind, 2 + (glev / 30), TV_BOLT, SV_AMMO_NORMAL, 5, 8, 0, "Bolt Trap"); break;
		case TRAP_OF_ARROWS_III:
			ident = player_handle_missile_trap(Ind, 2 + (glev / 30), TV_ARROW, SV_AMMO_HEAVY, 6, 9, 0, "Seeker Arrow Trap"); break;
		case TRAP_OF_ARROWS_IV:
			ident = player_handle_missile_trap(Ind, 2 + (glev / 30), TV_BOLT, SV_AMMO_HEAVY, 8, 10, 0, "Seeker Bolt Trap"); break;
		case TRAP_OF_POISON_ARROWS_I:
			ident = player_handle_missile_trap(Ind, 2 + (glev / 30), TV_ARROW, SV_AMMO_NORMAL, 4, 8, 10 + randint(20), "Poison Arrow Trap"); break;
		case TRAP_OF_POISON_ARROWS_II:
			ident = player_handle_missile_trap(Ind, 2 + (glev / 30), TV_BOLT, SV_AMMO_NORMAL, 5, 8, 15 + randint(30), "Poison Bolt Trap"); break;
		case TRAP_OF_POISON_ARROWS_III:
			ident = player_handle_missile_trap(Ind, 2 + (glev / 30), TV_ARROW, SV_AMMO_HEAVY, 6, 8, 30 + randint(50), "Poison Seeker Arrow Trap"); break;
		case TRAP_OF_POISON_ARROWS_IV:
			ident = player_handle_missile_trap(Ind, 2 + (glev / 30), TV_BOLT, SV_AMMO_HEAVY, 8, 10, 40 + randint(70), "Poison Seeker Bolt Trap"); break;
		case TRAP_OF_DAGGERS_I:
			ident = player_handle_missile_trap(Ind, 2 + (glev / 30), TV_SWORD, SV_BROKEN_DAGGER, 2, 8, 0, "Dagger Trap"); break;
		case TRAP_OF_DAGGERS_II:
			ident = player_handle_missile_trap(Ind, 2 + (glev / 30), TV_SWORD, SV_DAGGER, 3, 8, 0, "Dagger Trap"); break;
		case TRAP_OF_POISON_DAGGERS_I:
			ident = player_handle_missile_trap(Ind, 2 + (glev / 30), TV_SWORD, SV_BROKEN_DAGGER, 2, 8, 15 + randint(20), "Poison Dagger Trap"); break;
		case TRAP_OF_POISON_DAGGERS_II:
			ident = player_handle_missile_trap(Ind, 2 + (glev / 30), TV_SWORD, SV_DAGGER, 3, 8, 20 + randint(30), "Poison Dagger Trap"); break;

		/* it was '20,90,70'... */
		case TRAP_OF_DROP_ITEMS:
			ident = do_player_drop_items(Ind, 20, TRUE); destroy_chest(i_ptr); break;
		case TRAP_OF_DROP_ALL_ITEMS:
			ident = do_player_drop_items(Ind, 70, TRUE); destroy_chest(i_ptr); break;
		case TRAP_OF_DROP_EVERYTHING:
			ident = do_player_drop_items(Ind, 90, TRUE); destroy_chest(i_ptr); break;

		/* Bolt Trap */
		case TRAP_OF_ELEC_BOLT:       ident = player_handle_breath_trap(Ind, 1, GF_ELEC, TRAP_OF_ELEC_BOLT); break;
		case TRAP_OF_POIS_BOLT:       ident = player_handle_breath_trap(Ind, 1, GF_POIS, TRAP_OF_POIS_BOLT); break;
		case TRAP_OF_ACID_BOLT:       ident = player_handle_breath_trap(Ind, 1, GF_ACID, TRAP_OF_ACID_BOLT); destroy_chest(i_ptr); break;
		case TRAP_OF_COLD_BOLT:       ident = player_handle_breath_trap(Ind, 1, GF_COLD, TRAP_OF_COLD_BOLT); break;
		case TRAP_OF_FIRE_BOLT:       ident = player_handle_breath_trap(Ind, 1, GF_FIRE, TRAP_OF_FIRE_BOLT); destroy_chest(i_ptr); break;
		case TRAP_OF_PLASMA_BOLT:     ident = player_handle_breath_trap(Ind, 1, GF_PLASMA, TRAP_OF_PLASMA_BOLT); destroy_chest(i_ptr); break;
		case TRAP_OF_WATER_BOLT:      ident = player_handle_breath_trap(Ind, 1, GF_WATER, TRAP_OF_WATER_BOLT); break;
		case TRAP_OF_LITE_BOLT:       ident = player_handle_breath_trap(Ind, 1, GF_LITE, TRAP_OF_LITE_BOLT); break;
		case TRAP_OF_DARK_BOLT:       ident = player_handle_breath_trap(Ind, 1, GF_DARK, TRAP_OF_DARK_BOLT); break;
		case TRAP_OF_SHARDS_BOLT:     ident = player_handle_breath_trap(Ind, 1, GF_SHARDS, TRAP_OF_SHARDS_BOLT); destroy_chest(i_ptr); break;
		case TRAP_OF_SOUND_BOLT:      ident = player_handle_breath_trap(Ind, 1, GF_SOUND, TRAP_OF_SOUND_BOLT); break;
		case TRAP_OF_CONFUSION_BOLT:  ident = player_handle_breath_trap(Ind, 1, GF_CONFUSION, TRAP_OF_CONFUSION_BOLT); break;
		case TRAP_OF_FORCE_BOLT:      ident = player_handle_breath_trap(Ind, 1, GF_FORCE, TRAP_OF_FORCE_BOLT); destroy_chest(i_ptr); break;
		case TRAP_OF_INERTIA_BOLT:    ident = player_handle_breath_trap(Ind, 1, GF_INERTIA, TRAP_OF_INERTIA_BOLT); break;
		case TRAP_OF_MANA_BOLT:       ident = player_handle_breath_trap(Ind, 1, GF_MANA, TRAP_OF_MANA_BOLT); destroy_chest(i_ptr); break;
		case TRAP_OF_ICE_BOLT:        ident = player_handle_breath_trap(Ind, 1, GF_ICE, TRAP_OF_ICE_BOLT); break;
		case TRAP_OF_CHAOS_BOLT:      ident = player_handle_breath_trap(Ind, 1, GF_CHAOS, TRAP_OF_CHAOS_BOLT); destroy_chest(i_ptr); break;
		case TRAP_OF_NETHER_BOLT:     ident = player_handle_breath_trap(Ind, 1, GF_NETHER, TRAP_OF_NETHER_BOLT); break;
		case TRAP_OF_DISENCHANT_BOLT: ident = player_handle_breath_trap(Ind, 1, GF_DISENCHANT, TRAP_OF_DISENCHANT_BOLT); destroy_chest(i_ptr); break;
		case TRAP_OF_NEXUS_BOLT:      ident = player_handle_breath_trap(Ind, 1, GF_NEXUS, TRAP_OF_NEXUS_BOLT); break;
		case TRAP_OF_TIME_BOLT:       ident = player_handle_breath_trap(Ind, 1, GF_TIME, TRAP_OF_TIME_BOLT); break;
		case TRAP_OF_GRAVITY_BOLT:    ident = player_handle_breath_trap(Ind, 1, GF_GRAVITY, TRAP_OF_GRAVITY_BOLT); destroy_chest(i_ptr); break;

		/* Ball Trap */
		case TRAP_OF_ELEC_BALL:       ident = player_handle_breath_trap(Ind, 3, GF_ELEC, TRAP_OF_ELEC_BALL); break;
		case TRAP_OF_POIS_BALL:       ident = player_handle_breath_trap(Ind, 3, GF_POIS, TRAP_OF_POIS_BALL); break;
		case TRAP_OF_ACID_BALL:       ident = player_handle_breath_trap(Ind, 3, GF_ACID, TRAP_OF_ACID_BALL); destroy_chest(i_ptr); break;
		case TRAP_OF_COLD_BALL:       ident = player_handle_breath_trap(Ind, 3, GF_COLD, TRAP_OF_COLD_BALL); break;
		case TRAP_OF_FIRE_BALL:       ident = player_handle_breath_trap(Ind, 3, GF_FIRE, TRAP_OF_FIRE_BALL); destroy_chest(i_ptr); break;
		case TRAP_OF_PLASMA_BALL:     ident = player_handle_breath_trap(Ind, 3, GF_PLASMA, TRAP_OF_PLASMA_BALL); destroy_chest(i_ptr); break;
		case TRAP_OF_WATER_BALL:      ident = player_handle_breath_trap(Ind, 3, GF_WATER, TRAP_OF_WATER_BALL); break;
		case TRAP_OF_LITE_BALL:       ident = player_handle_breath_trap(Ind, 3, GF_LITE, TRAP_OF_LITE_BALL); break;
		case TRAP_OF_DARK_BALL:       ident = player_handle_breath_trap(Ind, 3, GF_DARK, TRAP_OF_DARK_BALL); break;
		case TRAP_OF_SHARDS_BALL:     ident = player_handle_breath_trap(Ind, 3, GF_SHARDS, TRAP_OF_SHARDS_BALL); destroy_chest(i_ptr); break;
		case TRAP_OF_SOUND_BALL:      ident = player_handle_breath_trap(Ind, 3, GF_SOUND, TRAP_OF_SOUND_BALL); break;
		case TRAP_OF_CONFUSION_BALL:  ident = player_handle_breath_trap(Ind, 3, GF_CONFUSION, TRAP_OF_CONFUSION_BALL); break;
		case TRAP_OF_FORCE_BALL:      ident = player_handle_breath_trap(Ind, 3, GF_FORCE, TRAP_OF_FORCE_BALL); destroy_chest(i_ptr); break;
		case TRAP_OF_INERTIA_BALL:    ident = player_handle_breath_trap(Ind, 3, GF_INERTIA, TRAP_OF_INERTIA_BALL); break;
		case TRAP_OF_MANA_BALL:       ident = player_handle_breath_trap(Ind, 3, GF_MANA, TRAP_OF_MANA_BALL); destroy_chest(i_ptr); break;
		case TRAP_OF_ICE_BALL:        ident = player_handle_breath_trap(Ind, 3, GF_ICE, TRAP_OF_ICE_BALL); destroy_chest(i_ptr); break;
		case TRAP_OF_CHAOS_BALL:      ident = player_handle_breath_trap(Ind, 3, GF_CHAOS, TRAP_OF_CHAOS_BALL); destroy_chest(i_ptr); break;
		case TRAP_OF_NETHER_BALL:     ident = player_handle_breath_trap(Ind, 3, GF_NETHER, TRAP_OF_NETHER_BALL); break;
		case TRAP_OF_DISENCHANT_BALL: ident = player_handle_breath_trap(Ind, 3, GF_DISENCHANT, TRAP_OF_DISENCHANT_BALL); destroy_chest(i_ptr); break;
		case TRAP_OF_NEXUS_BALL:      ident = player_handle_breath_trap(Ind, 3, GF_NEXUS, TRAP_OF_NEXUS_BALL); destroy_chest(i_ptr); break;
		case TRAP_OF_TIME_BALL:       ident = player_handle_breath_trap(Ind, 3, GF_TIME, TRAP_OF_TIME_BALL); destroy_chest(i_ptr); break;
		case TRAP_OF_GRAVITY_BALL:    ident = player_handle_breath_trap(Ind, 3, GF_GRAVITY, TRAP_OF_GRAVITY_BALL); destroy_chest(i_ptr); break;

		/* -SC- */
		case TRAP_OF_FEMINITY:
		break; //why not make a trap of change race/class too.
			msg_print(Ind, "Gas sprouts out... you feel you transmute.");
			trap_hit(Ind, trap);
			if (!p_ptr->male) break;
			//	 p_ptr->psex = SEX_FEMALE;
			//	 sp_ptr = &sex_info[p_ptr->psex];
			p_ptr->male = FALSE;
			ident = TRUE;
			p_ptr->redraw |= PR_MISC;
			break;
		case TRAP_OF_MASCULINITY:
		break; //why not make a trap of change race/class too.
#if 0
			msg_print(Ind, "Gas sprouts out... you feel you transmute.");
			trap_hit(Ind, trap);
			if (p_ptr->male) break;
			//	 p_ptr->psex = SEX_MALE;
			//	 sp_ptr = &sex_info[p_ptr->psex];
			p_ptr->male = TRUE;
			ident = TRUE;
			p_ptr->redraw |= PR_MISC;
			break;
#endif
		case TRAP_OF_NEUTRALITY:
		break; //rotfl.
#if 0
			msg_print(Ind, "Gas sprouts out... you feel you transmute.");
			//	 p_ptr->psex = SEX_NEUTER;
			//	 sp_ptr = &sex_info[p_ptr->psex];
			p_ptr->male = FALSE;
			ident = TRUE;
			trap_hit(Ind, trap);
			p_ptr->redraw |= PR_MISC;
#endif
			break;

		case TRAP_OF_AGING:
			if (p_ptr->prace == RACE_VAMPIRE || p_ptr->resist_time || p_ptr->prace == RACE_MAIA) {
				msg_print(Ind, "Colours are scintillating around you but you seem unaffected.");
				break;
			}
			msg_print(Ind, "Colors are scintillating around you, you see your past running before your eyes.");
			//         p_ptr->age += randint((rp_ptr->b_age + rmp_ptr->b_age) / 2);
			p_ptr->age += randint((p_ptr->rp_ptr->b_age));
			ident = TRUE;
			trap_hit(Ind, trap);
			p_ptr->redraw |= PR_VARIOUS;
			break;

		case TRAP_OF_GROWING:
		{
			s16b tmp;

			msg_print(Ind, "Heavy fumes sprout out... you feel you transmute.");
			//         if (p_ptr->psex == SEX_FEMALE) tmp = rp_ptr->f_b_ht + rmp_ptr->f_b_ht;
			//         else tmp = rp_ptr->m_b_ht + rmp_ptr->m_b_ht;

			if (!p_ptr->male) tmp = p_ptr->rp_ptr->f_b_ht;
			else tmp = p_ptr->rp_ptr->m_b_ht;

			p_ptr->ht += randint(tmp / 4);
			ident = TRUE;
			trap_hit(Ind, trap);
			p_ptr->redraw |= PR_VARIOUS;
			break;
		}

		case TRAP_OF_SHRINKING:
		{
			s16b tmp;

			msg_print(Ind, "Heavy fumes sprout out... you feel you transmute.");
			//         if (p_ptr->psex == SEX_FEMALE) tmp = rp_ptr->f_b_ht + rmp_ptr->f_b_ht;
			//         else tmp = rp_ptr->m_b_ht + rmp_ptr->m_b_ht;
			if (!p_ptr->male) tmp = p_ptr->rp_ptr->f_b_ht;
			else tmp = p_ptr->rp_ptr->m_b_ht;

			p_ptr->ht -= randint(tmp / 4);
			if (p_ptr->ht <= tmp / 4) p_ptr->ht = tmp / 4;
			ident = TRUE;
			trap_hit(Ind, trap);
			p_ptr->redraw |= PR_VARIOUS;
			break;
		}

		/* Trap of Tanker Drain */
		case TRAP_OF_TANKER_DRAIN:
#if 0	// ?_?
			if (p_ptr->ctp>0) {
				p_ptr->ctp = 0;
				p_ptr->redraw |= (PR_TANK);
				msg_print(Ind, "You sense a great loss.");
				ident = TRUE;
			} else {
				/* no sense saying this unless you never have tanker point */
				if (p_ptr->mtp == 0)
					msg_format(Ind, "Suddenly you feel glad you're only a %s", rp_ptr->title + rp_name);
				else
					msg_print(Ind, "Your head feels dizzy for a moment.");
			}
#endif
			break;

		/* Trap of Divine Anger */
		case TRAP_OF_DIVINE_ANGER: // No hell below us above us only sky o/~
#if 0 /* completely insane */
			if (p_ptr->pgod == 0)
				msg_format(Ind, "Suddenly you feel glad you're only a %s", cp_ptr->title);
			else {
				cptr name;

				name = deity_info[p_ptr->pgod-1].name;
				msg_format(Ind, "You feel you have angered %s.", name);
				set_grace(p_ptr->grace - 3000);
			}
#endif	// 0
			break;

		/* Trap of Divine Wrath */
		case TRAP_OF_DIVINE_WRATH:
#if 0	// No hell below us above us only sky o/~
			if (p_ptr->pgod == 0)
				msg_format(Ind, "Suddenly you feel glad you're only a %s", cp_ptr->title);
			else {
				cptr name;

				name = deity_info[p_ptr->pgod-1].name;

				msg_format(Ind, "%s quakes in rage: ``Thou art supremely insolent, mortal!!''", name);
				nasty_side_effect();
				set_grace(p_ptr->grace - 5000);
			}
#endif
			break;

		/* Trap of hallucination */
		case TRAP_OF_HALLUCINATION:
			msg_print(Ind, "Scintillating colours hypnotize you for a moment.");
			set_image(Ind, 80);
			break;

		/* Bolt Trap */
		case TRAP_OF_ROCKET: ident = player_handle_breath_trap(Ind, 1, GF_ROCKET, trap); destroy_chest(i_ptr); break;
		case TRAP_OF_NUKE_BOLT: ident =player_handle_breath_trap(Ind, 1, GF_NUKE, trap); break;
#if 1	// coming..when it comes :) //very pow erful btw. insta-kills weaker chars.
		case TRAP_OF_HOLY_FIRE: ident = player_handle_breath_trap(Ind, 1, GF_HOLY_FIRE, trap); break;
		case TRAP_OF_HELL_FIRE: ident = player_handle_breath_trap(Ind, 1, GF_HELL_FIRE, trap); destroy_chest(i_ptr); break;
#endif	// 0
		case TRAP_OF_PSI_BOLT: ident = player_handle_breath_trap(Ind, 1, GF_PSI, trap); break;
		//case TRAP_OF_PSI_DRAIN: ident = player_handle_breath_trap(1, GF_PSI_DRAIN, trap); break;

		/* Ball Trap */
		case TRAP_OF_NUKE_BALL: ident = player_handle_breath_trap(Ind, 3, GF_NUKE, TRAP_OF_NUKE_BALL); destroy_chest(i_ptr); break;
		//case TRAP_OF_PSI_BALL: ident = player_handle_breath_trap(3, GF_PSI, TRAP_OF_NUKE_BALL); break;

		/* PernMangband additions */
		/* Trap? of Ale vendor */
		case TRAP_OF_ALE:
		{
			u32b price = glev, amt;

			price *= price;
			if (price < 5) price = 5;
			if (price > 1000) price = 1000; /* better cap this thing somewhere - mikaelh; reduced it from 5k to 1k even - C. Blue */
			amt = (p_ptr->au / price);

			if (amt < 1) {
				msg_print(Ind, "You feel as if it's the day before the payday.");
			} else {
				object_type *o_ptr, forge;
				o_ptr = &forge;
				invcopy(o_ptr, lookup_kind(TV_FOOD, SV_FOOD_PINT_OF_ALE));

				if (amt > 8) amt = 8;//was 10
				amt = randint(amt);

				p_ptr->au -= amt * price;
				o_ptr->number = amt;
				drop_near(0, o_ptr, -1, wpos, p_ptr->py, p_ptr->px);

				msg_print(Ind, "You feel like having a shot.");
				ident = TRUE;

				p_ptr->redraw |= (PR_GOLD);
			}
			break;
		}
		/* Trap of Garbage */
		case TRAP_OF_GARBAGE:
			ident = do_player_trap_garbage(Ind, 300 + glev * 3);
			destroy_chest(i_ptr); 
			break;

      /* Trap of discordance */
		case TRAP_OF_HOSTILITY:
		break; //ill-minded - ppl got wrathed automatically because
#if 0
		//someone ELSE triggered this. think more carefully.
		//Further it's kinda meta-trap, changing the player's configuration.
			ident = player_handle_trap_of_hostility(Ind);
			ident = FALSE;		// never known
			break;
#endif
		/* Voluminous cuisine Trap */
		case TRAP_OF_CUISINE:
			msg_print(Ind, "You are treated to a marvellous elven cuisine!");
			if (!p_ptr->suscep_life && p_ptr->prace != RACE_ENT) {
				/* 1turn = 100 food value when satiated */
				(void)set_food(Ind, PY_FOOD_MAX + glev*50 + 1000 + rand_int(1000));
			}
			ident = TRUE;
			break;

		/* Trap of unmagic */
		case TRAP_OF_UNMAGIC:
			ident = unmagic(Ind);
			break;

		/* Vermin Trap */
		case TRAP_OF_VERMIN:
			l = randint(20 + glev) + 30;/*let's give the player a decent chance to clean the mess
						    over a reasonable amount of time.*/
			for (k = 0; k < l; k++) {
				s16b cx = x + 20 - rand_int(41);
				s16b cy = y + 20 - rand_int(41);

				if (!in_bounds(cy,cx)) {
					cx = x;
					cy = y;
				}

				/* paranoia */
				if (!cave_empty_bold(zcave, cy, cx)) {
					cx = x;
					cy = y;
				}

				summon_override_checks = SO_IDDC;
				ident |= summon_specific(wpos, cy, cx, glev, 0, SUMMON_VERMIN, 1, 0);
				summon_override_checks = SO_NONE;
			}
			if (ident) {
				msg_print(Ind, "You suddenly feel itchy.");
#ifdef USE_SOUND_2010
				//sound_near_site(y, x, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
#endif
			}
			break;

		/* Trap of amnesia (and not lose_memory) */
		case TRAP_OF_AMNESIA:
			if (rand_int(100) < p_ptr->skill_sav ||
			    (p_ptr->pclass == CLASS_MINDCRAFTER && magik(75)))
				msg_print(Ind, "You resist the effects!");
			else if (lose_all_info(Ind)) {
				msg_print(Ind, "Your memories fade away.");
				//ident = FALSE;	// haha you forget this also :)
			}
			break;

		case TRAP_OF_SILLINESS:
		break; //ill-minded, not silyl
#if 0
			if (rand_int(100) < p_ptr->skill_sav)
				msg_print(Ind, "You resist the effects!");
			else if (do_trap_of_silliness(Ind, 50 + glev)) {
				msg_print(Ind, "You feel somewhat silly.");
				//ident = FALSE;	// haha you forget this also :)
			}
			break;
#endif

		case TRAP_OF_GOODBYE_CHARLIE:
		{
			//s16b i,j;
			int chance = 10 + glev / 2;
			//bool message = FALSE;

			if (chance > 50) chance = 50;

			/* Send him/her back to home :) */
			p_ptr->recall_pos.wx = wpos->wx;
			p_ptr->recall_pos.wy = wpos->wy;
			p_ptr->recall_pos.wz = 0;

			if (!p_ptr->word_recall) {
				int delay = 120 - glev / 2;
				if (delay < 1) delay = 1;

				//				p_ptr->word_recall = rand_int(20) + 15;
				set_recall_timer(Ind, rand_int(delay) + 40);
				ident = TRUE;
			}
			//SO ill-minded. Ever hit it two times in a row after
			//cleaning vaults?
			//ident &= do_player_scatter_items(Ind, chance, 15);
			//without scattering it's acceptable.
			break;
		}

		case TRAP_OF_PRESENT_EXCHANGE:
			ident = do_player_drop_items(Ind, 50, TRUE);
			ident |= do_player_trap_garbage(Ind, 300 + glev * 3);
			break;

		case TRAP_OF_GARBAGE_FILLING:
		{
			s16b nx, ny;
			ident |= do_player_trap_garbage(Ind, 300 + glev * 3);
			for (nx = x - 8; nx <= x + 8; nx++) {
				for (ny = y - 8; ny <= y + 8; ny++) {
					if (!in_bounds (ny, nx)) continue;

					if (rand_int(distance(ny, nx, y, x)) > 3)
						place_trap(wpos, ny, nx, 0);
				}
			}
			//            msg_print(Ind, "The floor vibrates in a strange way.");
			ident = FALSE;
			break;
		}
		case TRAP_OF_CHASM:
		{
			int maxfall = 0;
			struct wilderness_type *wild;
			wild = &wild_info[wpos->wy][wpos->wx];

			/* MEGAHACK: Ignore Wilderness chasm. */
			vanish = 100;

			if (wpos->wz == 0 && wild->dungeon == NULL) break; /* Nothing below us as pointed out by miikkajo - mikaelh */

			if (wpos->wz > 0) maxfall = wpos->wz;
			else if(ABS(wpos->wz) < wild->dungeon->maxdepth)
				maxfall = wild->dungeon->maxdepth - ABS(wpos->wz);
			else {
				//msg_print(Ind, "\377GYou feel quite certain something really awful just happened..");
				break;
			}

			l = glev / 10;
			l = l > 3 ? 3 : (l < 1 ? 1 : l);
			k = maxfall > l ? randint(l) : randint(maxfall);

			ident = TRUE;
			vanish = 0;
			if (p_ptr->levitate) {
				msg_print(Ind, "You notice a deep chasm below you.");
				break;
			}
			msg_print(Ind, "You fell into a chasm!");
			if (p_ptr->feather_fall || p_ptr->tim_wraith)
				msg_print(Ind, "You float gently down the chasm.");
			else {
				msg_print(Ind, "You fall head over heels!!");
				l = damroll(2, 8) << k;	// max 512, avg 72
				//take_hit(Ind, dam, name, 0);

				/* Inventory damage (Hack - use 'cold' type) */
				inven_damage(Ind, set_cold_destroy, 15 * k);
				inven_damage(Ind, set_all_destroy, 3 * k);

				take_hit(Ind, l, "a chasm", 0);
				//take_sanity_hit(Ind, 1U << k, "a chasm");
			}

			do_player_trap_change_depth(Ind, -k);
			break;
		}

		case TRAP_OF_PIT:
			ident = TRUE;
			vanish = 0;
			if (p_ptr->levitate) {
				/* dont notice it */
				break;
			}
			msg_print(Ind, "You fell into a pit!");
			if (p_ptr->feather_fall || p_ptr->tim_wraith)
				msg_print(Ind, "You float gently to the bottom of the pit.");
			else {
				l = damroll(1, 8);

				/* Inventory damage */
				inven_damage(Ind, set_impact_destroy, l);

				//take_hit(Ind, dam, name, 0);
				take_hit(Ind, l, "a pit", 0);
				//take_sanity_hit(Ind, 1, "a pit");

				/* Maybe better make them other types of traps? */
				if (glev > 29) {
					msg_print(Ind, "You are pierced by the spikes!");
					(void)set_cut(Ind, p_ptr->cut + randint(glev * 2) + 30, 0);

					if (glev > 49 && !p_ptr->resist_pois && !p_ptr->oppose_pois && !p_ptr->immune_poison) {
						msg_print(Ind, "The spikes were poisoned!");
						(void)set_poisoned(Ind, p_ptr->poisoned + randint(glev * 2) + 50, 0);
					}
				}

			}
			break;

		/* Steal Item Trap */
		case TRAP_OF_SEASONED_TRAVELER:
		/* lose all your cash, invested in items worth 60k Au or so?
		all *healings* / fireproof WoR that took ages to collect just gone? great idea.
		PLAY the game before madly thinking out stuff.
		Suggestion from me: A seasoned traveller doesn't need the
		silyl standard stuff in order to survive. So I added a value
		check, see below. - C. Blue */
		{
			object_type *o_ptr;
			int i, j;
			bool iddc = in_irondeepdive(wpos);

			for (i = 0; i < INVEN_PACK; i++) {
				if (!p_ptr->inventory[i].k_idx) continue;
				if (rand_int(999) < p_ptr->skill_sav) continue;

				o_ptr = &p_ptr->inventory[i];

				if ((j = o_ptr->number) < 2) continue;
				if (o_ptr->name1 == ART_POWER) continue;

				ok_ptr = &k_info[o_ptr->k_idx];
				if (ok_ptr->cost > 150) continue;
				//cure crits cost 100, WoR 150, so it still poses a decent threat.
				if (is_ammo(o_ptr->tval)) continue; /* ammo is too important for archers, and hard to collect! */

				if (j > 8) j = iddc ? j >> 2 : j - (j >> 3);

				/* Hack -- If a wand, allocate total
				 * maximum timeouts or charges between those
				 * stolen and those missed. -LM-
				 */
				if (is_magic_device(o_ptr->tval)) divide_charged_item(NULL, o_ptr, j - 1); /* The charge goes to .. void ;) */

				inven_item_increase(Ind, i, 1 - j);
				inven_item_optimize(Ind, i);
				p_ptr->notice |= (PN_COMBINE | PN_REORDER);
				if (!ident && magik(j - 1)) {
					msg_print(Ind, "You suddenly feel nimble and light.");
					ident = TRUE;
				}
			}
			if (!ident) msg_print(Ind, "You feel displaced.");
			break;
		}
		/* Scribble Trap */
		case TRAP_OF_SCRIBBLE:
		break;
#if 0	/* insane - this COULD be cool if it was adjusted to be more sane */
		{
			int i, j, k, k_idx;
			object_type *o_ptr;
			object_type     forge;
			object_type     *q_ptr = &forge;

			for (i = 0; i < INVEN_PACK; i++) {
				o_ptr = &p_ptr->inventory[i];

				if (!o_ptr->k_idx) continue;
				if (o_ptr->tval != TV_SCROLL || o_ptr->sval != SV_SCROLL_NOTHING)
					continue;

				ident = TRUE;
				vanish = 100;

				j = o_ptr->number;
				j = randint(j < 5 ? j : 5);

				inven_item_increase(Ind, i, -j);
				inven_item_describe(Ind, i);
				inven_item_optimize(Ind, i);

				for (k = 0; k < j; k++) {
					for (k_idx = 0; !k_idx; k_idx = lookup_kind(TV_SCROLL, rand_int(SV_SCROLL_NOTHING)))
						/* nothing */;

					invcopy(q_ptr, k_idx);
					q_ptr->number = 1;
					q_ptr->discount = o_ptr->discount;
					q_ptr->owner = o_ptr->owner;
					q_ptr->mode = o_ptr->mode;
					q_ptr->level = o_ptr->level;
					q_ptr->note = o_ptr->note;
					(void)inven_carry(Ind, q_ptr);
				}

				break;
			}

			if (!ident)
				for (i = 0; i < INVEN_TOTAL; i++) {
					cptr str;
					o_ptr = &p_ptr->inventory[i];

					if (!o_ptr->k_idx) continue;
					if (o_ptr->name1 == ART_POWER) continue;
					if ((j = rand_int(100)) > 20 + glev) continue;

					ident = TRUE;

					if (!(j % 3)) str = "Vlad was here!!";
					else if (is_realm_book(o_ptr)) str = "!*!m!n!p";
					else switch (o_ptr->tval) {
						case TV_SCROLL:
							str = "!*!r";
							break;
						case TV_POTION:
						case TV_POTION2:
							str = "!*!q";
							break;
						case TV_FOOD:
							str = "!*!E";
							break;
						case TV_ROD:
						case TV_WAND:
						case TV_STAFF:
							str = "!*!a!u!z";
							break;
						default:
							str = "!*!f!t!w!A!F";
					}

					o_ptr->note = quark_add(str);
				}

			if (ident && !p_ptr->blind)
				msg_print(Ind, "A magic marker dances around you!");

			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

			break;
		}
#endif
		/* Slump Trap */
		case TRAP_OF_SLUMP:
		/*ill-minded. Usually you spent a longer time to max your stats.
		Having traps that permanently decrease one stat is fine, but
		-10 to all stats permanently is pathetic. */
		break;
#if 0
			k = glev / 5;
			k = k < 1 ? 1 : k;
			msg_print(Ind, "You feel as though you have hit a slump.");
			//		ident |= dec_stat(Ind, A_DEX, 25, TRUE);	// TRUE..!?
			ident |= dec_stat(Ind, A_DEX, k, STAT_DEC_PERMANENT);
			ident |= dec_stat(Ind, A_WIS, k, STAT_DEC_PERMANENT);
			ident |= dec_stat(Ind, A_CON, k, STAT_DEC_PERMANENT);
			ident |= dec_stat(Ind, A_STR, k, STAT_DEC_PERMANENT);
			ident |= dec_stat(Ind, A_CHR, k, STAT_DEC_PERMANENT);
			ident |= dec_stat(Ind, A_INT, k, STAT_DEC_PERMANENT);
			break;
#endif
		/* Ophiuchus Trap */
		case TRAP_OF_OPHIUCHUS:
		//lol, never met it, but looks interesting.
		{
			player_type *q_ptr;
			for (k = m_max - 1; k >= 1; k--) {
				monster_type *m_ptr = &m_list[k];

				if (inarea(&m_ptr->wpos,wpos))
					m_ptr->hp = m_ptr->maxhp;
			}
			for (k = 1; k <= NumPlayers; k++) {
				q_ptr = Players[k];
				if (q_ptr->conn == NOT_CONNECTED) continue;

				if (!inarea(wpos, &q_ptr->wpos)) continue;

				msg_print(k, "A fragrant mist fills the air...");
				hp_player(k, 9999);
				if (q_ptr->black_breath && magik(50)) {
					msg_print(k, "The hold of the Black Breath on you is broken!");
					q_ptr->black_breath = FALSE;
				}
			}
			break;
		}
		/* Lost Labor Trap */
		case TRAP_OF_LOST_LABOR:
			for (k = m_max - 1; k >= 1; k--) {
				monster_type *m_ptr = &m_list[k];

				if (inarea(&m_ptr->wpos,wpos)) {
					monster_race    *r_ptr = race_inf(m_ptr);
					if (!(r_ptr->flags1 & RF1_UNIQUE))
						m_ptr->clone = 100;
				}
			}
			msg_print(Ind, "You feel very despondent..");
			break;

		case TRAP_OF_DESPAIR: /* AHAH jir well done ;) */
		{
			object_type     forge;
			object_type     *o_ptr = &forge;

			msg_print(Ind, "You are driven to despair.");
			//		ident |= dec_stat(Ind, A_DEX, 25, TRUE);	// TRUE..!?
			ident |= dec_stat(Ind, A_DEX, 10, STAT_DEC_PERMANENT);
			ident |= dec_stat(Ind, A_WIS, 10, STAT_DEC_PERMANENT);
			ident |= dec_stat(Ind, A_CON, 10, STAT_DEC_PERMANENT);
			ident |= dec_stat(Ind, A_STR, 10, STAT_DEC_PERMANENT);
			ident |= dec_stat(Ind, A_CHR, 10, STAT_DEC_PERMANENT);
			ident |= dec_stat(Ind, A_INT, 10, STAT_DEC_PERMANENT);

			invcopy(o_ptr, lookup_kind(TV_POTION, SV_POTION_AUGMENTATION));
			o_ptr->number = 1;
			o_ptr->discount = 100;
			o_ptr->owner = p_ptr->id;
			o_ptr->mode = p_ptr->mode;
			o_ptr->level = 0;
			(void)inven_carry(Ind, o_ptr);

			invcopy(o_ptr, lookup_kind(TV_POTION, SV_POTION_DEATH));
			o_ptr->number = 1;
			o_ptr->discount = 100;
			o_ptr->owner = p_ptr->id;
			o_ptr->mode = p_ptr->mode;
			o_ptr->level = 0;
			(void)inven_carry(Ind, o_ptr);

			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

			break;
		}

		case TRAP_OF_RARE_BOOKS:
		{
			object_type     forge;
			object_type     *o_ptr = &forge;

			msg_print(Ind, "Suddenly you felt something really splendid just happened to you!");

                        /* 0 - 17 = Tomes */
			invcopy(o_ptr, lookup_kind(TV_BOOK, rand_range(0, 17)));
			o_ptr->number = 1;
			o_ptr->discount = 100;
			o_ptr->owner = p_ptr->id;
			o_ptr->mode = p_ptr->mode;
			o_ptr->level = 0;
			o_ptr->note = quark_add("!*");
			o_ptr->iron_trade = p_ptr->iron_trade;
			(void)inven_carry(Ind, o_ptr);

			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
			break;
		}
		/* Wrong Target Trap */
		case TRAP_OF_WRONG_TARGET:
		{
#if 0 /* pointless, instantly overwritten by auto-targetting */
			int tx, ty;

			/* Clear the target */
			p_ptr->target_who = 0;

			scatter(wpos, &ty, &tx, p_ptr->py, p_ptr->px, 15, 0);

			/* Set the target */
			p_ptr->target_row = ty;
			p_ptr->target_col = tx;

			/* Set 'stationary' target */
			p_ptr->target_who = 0 - MAX_PLAYERS - 2; //TARGET_STATIONARY

			msg_print(Ind, "You feel uneasy.");
#endif
			break;
		}

		/* Cleaning Trap */
		case TRAP_OF_CLEANING:
		{
			int ix, iy;
			char o_name[ONAME_LEN];

			/* Delete the existing objects */
			for (k = 1; k < o_max; k++) {
				object_type *o_ptr = &o_list[k];

				/* Skip dead objects */
				if (!o_ptr->k_idx) continue;

				/* Skip objects not on this depth */
				if (!inarea(&o_ptr->wpos, wpos)) continue;

				/* Skip 'owned' items, so that this won't be too harsh 
				 * in the rescue scene */
				if (o_ptr->owner) continue;

				ix = o_ptr->ix;
				iy = o_ptr->iy;

				/* Only within certain radius */
				if (distance(y, x, iy, ix) > glev / 5 + 5) continue;

				/* Mega-Hack -- preserve artifacts */
				if (true_artifact_p(o_ptr)) { /* && !object_known_p(o_ptr)*/
					if (a_info[o_ptr->name1].flags4 & TR4_SPECIAL_GENE)
						continue;
					msg_print(Ind, "You have an accute feeling of loss!");
				}

				if (!p_ptr->blind && player_has_los_bold(Ind, iy, ix)) {
					note_spot(Ind, iy,ix);
					lite_spot(Ind, iy,ix);
					ident = TRUE;
					object_desc(Ind, o_name, o_ptr, FALSE, 0);

					msg_format(Ind, "You suddenly don't see the %s anymore!", o_name);
				}

				//if (zcave) zcave[iy][ix].o_idx = 0;
				delete_object_idx(k, TRUE);

				/* Wipe the object */
				//WIPE(o_ptr, object_type);
			}
			/* Compact the object list */
			compact_objects(0, FALSE);
			destroy_chest(i_ptr); 
			break;
		}
		/* Preparation Trap */
		case TRAP_OF_PREPARE:
			/* Look for the existing objects */
			for (k = 1; k < o_max; k++) {
				object_type *o_ptr = &o_list[k];

				/* Skip dead objects */
				if (!o_ptr->k_idx) continue;

				/* Skip objects not on this depth */
				if (!inarea(&o_ptr->wpos, wpos)) continue;

				if (magik(glev)) place_trap(wpos, o_ptr->iy, o_ptr->ix, 0);
			}
			msg_print(Ind, "You feel uneasy.");
			break;

		case TRAP_OF_MOAT_I:
			if (p_ptr->levitate){
				/* dont notice it */
				vanish = 0;
				break;
			}
			if (!c_ptr) break;
			if (!p_ptr->blind) msg_print(Ind, "A pool of water appears under you!");
			//c_ptr->feat = FEAT_WATER;
			cave_set_feat_live(wpos, y, x, FEAT_DEEP_WATER);
			break;

		case TRAP_OF_MOAT_II:
			msg_print(Ind, "As you touch the trap, the ground starts to shake.");
			destroy_chest(i_ptr);
			destroy_area(wpos, y, x, 10, TRUE, FEAT_DEEP_WATER, 30);
			if (c_ptr) cave_set_feat_live(wpos, y, x, FEAT_DEEP_WATER);
			break;

		/* why not? :) */
		case TRAP_OF_DISINTEGRATION_I: ident = player_handle_breath_trap(Ind, 5, GF_DISINTEGRATE, trap); destroy_chest(i_ptr); break;
		case TRAP_OF_DISINTEGRATION_II: ident = player_handle_breath_trap(Ind, 3, GF_DISINTEGRATE, trap); destroy_chest(i_ptr); break;
		case TRAP_OF_BATTLE_FIELD:
			if (no_summon) break;
			ident = player_handle_breath_trap(Ind, 5, GF_DISINTEGRATE, trap);
			destroy_chest(i_ptr);
			summon_override_checks = SO_IDDC;
			for (k = 0; k < randint(3); k++) ident |= summon_specific(wpos, y, x, glev, 0, SUMMON_ALL_U98, 1, 0);
			summon_override_checks = SO_NONE;
			if (ident) {
				msg_print(Ind, "You hear drums of battle!");
#ifdef USE_SOUND_2010
				//sound_near_site(y, x, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
#endif
			}
			break;

		/* Death Molds Trap */
		case TRAP_OF_DEATH_MOLDS:
		case TRAP_OF_DEATH_SWORDS:
			l = rand_range(1, 2);
			for (k = tdi[l]; k < tdi[l + 1]; k++) {
				s16b cx = p_ptr->px + tdx[k];
				s16b cy = p_ptr->py + tdy[k];

				if (!in_bounds(cy,cx)) continue;

				/* paranoia */
				if (!cave_empty_bold(zcave, cy, cx)) continue;

				ident |= (place_monster_aux(wpos, cy, cx,
				    race_index(trap == TRAP_OF_DEATH_MOLDS ?
				    "Death mold" : "Death sword"), FALSE, FALSE, FALSE, 0) == 0 &&
				    player_has_los_bold(Ind, cy, cx));
			}
			if (ident) msg_print(Ind, "You suddenly see a siege of malice!");
			break;

		case TRAP_OF_UNLIGHT:
		{
			/* Access the lite */
			object_type *o_ptr = &p_ptr->inventory[INVEN_LITE];
			u32b f1, f2, f3, f4, f5, f6, esp;

			(void)unlite_area(Ind, 0, 3);
			ident = TRUE;

			if (!o_ptr->k_idx) break;

			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

			/* Permalight is immune */
			if (!(f4 & TR4_FUEL_LITE)) break;

			if (artifact_p(o_ptr)) {
				if (!p_ptr->blind) msg_print(Ind, "Your light resists the effects!");
				break;
			}

			/* Drain fuel */
			if (o_ptr->timeout > 0)	{ // hope this won't cause trouble..
				/* Reduce fuel */
				o_ptr->timeout = 1;

#if 0	// process_player_end
				/* Notice */
				if (!p_ptr->blind) {
					msg_print(Ind, "Your light dims.");
					ident = TRUE;
				}
#endif	// 0

				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP);
			}

			/* TODO: eat flasks and torches if deep enough */

			break;
		}
		/* Thirsty Trap */
		case TRAP_OF_THIRST:
		{
			int i, j, bottles = 0;
			object_type *o_ptr;
			object_type     forge;
			object_type     *q_ptr = &forge;

			for (i = 0; i < INVEN_PACK; i++) {
				o_ptr = &p_ptr->inventory[i];

				if (!o_ptr->k_idx) continue;
				if (o_ptr->tval != TV_POTION && o_ptr->tval != TV_POTION2)
					continue;

				ident = TRUE;
				//vanish = 90;

				j = o_ptr->number;
				/* j = randint(j < 20 ? j : 20); not cool for *healings* */
				j = randint(j < 30 ? j / 2 : 15);
				bottles += j;

				inven_item_increase(Ind, i, -j);
				inven_item_describe(Ind, i);
				inven_item_optimize(Ind, i);
			}

			if (bottles) {
				if (bottles > 99) bottles = 99;

				invcopy(q_ptr, lookup_kind(TV_BOTTLE, 1));
				q_ptr->number = bottles;
				q_ptr->level = glev / 10 + 1;
				q_ptr->note = quark_add("Thank you");
				q_ptr->owner = p_ptr->id;
				q_ptr->mode = p_ptr->mode;
				q_ptr->iron_trade = p_ptr->iron_trade;
				(void)inven_carry(Ind, q_ptr);

				/* This trap is polite */
				/* Don't mess with IDDC or Highlander */
				if (p_ptr->max_exp) gain_au(Ind, bottles * 10, TRUE, FALSE);
			}

			//if (ident)
			msg_print(Ind, "Your backpack makes a clattering noise.");
#ifdef USE_SOUND_2010
			sound(Ind, "quaff_potion", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

			break;
		}

		/* Cannibal chest trap */
		case TRAP_OF_FINGER_CATCHING:
			msg_print(Ind, "Ouch! You get your finger caught!");
			trap_hit(Ind, trap);
			(void)set_cut(Ind, p_ptr->cut + randint(glev) + 5, 0);
			if (magik(50)) {
				msg_print(Ind, "Your dominant hand gets hurt!");
				do_dec_stat(Ind, A_DEX, STAT_DEC_TEMPORARY);
			}
			ident = TRUE;
			break;

		/* Animate Coins Trap */
		case TRAP_OF_ANIMATE_COINS:
		{
			u32b price = glev, amt;

			price *= price;
			if (price < 200) price = 200;
			if (price > 1000) price = 1000;
			amt = (p_ptr->au / price);

			if (amt > 20) amt = 20;
			if (amt > (u32b) glev / 3) amt = glev / 3;

			summon_override_checks = SO_IDDC;
			for (k = 0; k < (int) amt; k++) {
				if (summon_specific(wpos, y, x, glev, 0, SUMMON_BIZARRE5, 1, 0)) {
					ident = TRUE;
					p_ptr->au -= price;
				}
			}
			summon_override_checks = SO_NONE;

			if (ident) {
				msg_print(Ind, "Your purse suddenly squirms!");
				p_ptr->redraw |= PR_GOLD;
				destroy_chest(i_ptr);
#ifdef USE_SOUND_2010
				//sound_near_site(y, x, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
#endif
			}
			else msg_print(Ind, "Your purse tickles.");

			break;
		}

		/*     SEND
		 * +)  MORE
		 * ---------
		 *    MONEY Trap */
		case TRAP_OF_REMITTANCE: {
			player_type *q_ptr;
			u32b amt, max_amt = p_ptr->au / NumPlayers;

			if (is_admin(p_ptr)) break; /* he usually carries several 10 millions of Au - C. Blue */
			if (max_amt < 100) break;

			for (k = 1; k <= NumPlayers; k++) {
				if (k == Ind) continue;
				q_ptr = Players[k];
				if (q_ptr->conn == NOT_CONNECTED) continue;
				if (is_admin(q_ptr)) continue; /* admins are invisible and have high levels */
				/* No transfer between everlasting and non-everlasting? */
				if (compat_pmode(Ind, k, FALSE)) continue;
				/* Don't mess with IDDC or Highlander */
				if (in_irondeepdive(&q_ptr->wpos) || !q_ptr->max_exp) continue;

//				if (!inarea(wpos, &q_ptr->wpos)) continue;

				amt = q_ptr->lev * 100;
				if (q_ptr->lev > 20) amt *= q_ptr->lev - 20;
				if (amt > max_amt) amt = max_amt;
				if (amt < 100) continue;

				p_ptr->au -= amt;
				/* hack: prevent s32b overflow */
				if (!(2000000000 - amt < q_ptr->au)) {
					q_ptr->au += amt;
					msg_print(k, "Your purse feels heavier.");
					q_ptr->redraw |= PR_GOLD;
				}
				ident = TRUE;
			}

			if (ident) {
				msg_print(Ind, "You feel very generous!");
				p_ptr->redraw |= PR_GOLD;
			}

			break;
		}

		/* Trap of Hide Traps */
		case TRAP_OF_HIDE_TRAPS:
		{
			s16b nx, ny;
			cave_type *cc_ptr;
			struct c_special *cs_ptr;
			int rad = 8 + glev / 3;
			for (nx = x - rad; nx <= x + rad; nx++) {
				for (ny = y - rad; ny <= y + rad; ny++) {
					if (!in_bounds (ny, nx)) continue;

					cc_ptr = &zcave[ny][nx];

					if (!(cs_ptr = GetCS(cc_ptr, CS_TRAPS))) continue;
					if (!cs_ptr->sc.trap.found) continue;

					cs_ptr->sc.trap.found = FALSE;
					ident = TRUE;

					everyone_lite_spot(wpos, ny, nx);
				}
			}
			if (ident) msg_print(Ind, "The floor vibrates in a strange way.");
			ident = FALSE;	// naturally.
			break;
		}

		/* Trap of Respawning */
		case TRAP_OF_RESPAWN:
		{
			dun_level *l_ptr = getfloor(&p_ptr->wpos);
			if (!l_ptr || !(l_ptr->flags1 & LF1_NO_NEW_MONSTER)) {
				/* Set the monster generation depth */
				monster_level = glev;
				for (k = 0; k < 5 + glev / 4; k++) {
					if (p_ptr->wpos.wz)
						ident |= alloc_monster(&p_ptr->wpos, MAX_SIGHT + 5, FALSE);
					else wild_add_monster(&p_ptr->wpos);
				}
			}
			msg_print(Ind, "You feel uneasy.");
			ident = FALSE;	// always out of LOS (cept ESP..)
		}

		/* Jack-in-the-box trap */
		case TRAP_OF_JACK:
			summon_override_checks = SO_IDDC;
			for (k = 0; k < randint(3); k++)
				ident |= summon_specific(wpos, y, x, glev, 0, SUMMON_BIZARRE6, 1, 0);
			summon_override_checks = SO_NONE;
#if 0
			if (ident) {
				msg_print(Ind, "Doh!!");
				set_stun_raw(Ind, p_ptr->stun + randint(50));
 #ifdef USE_SOUND_2010
				sound_near_site(y, x, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
 #endif
			}
#endif	// 0
			break;

		case TRAP_OF_SPOOKINESS:
			l = randint(3) + 3;/*let's give the player a decent chance to clean the mess
						    over a reasonable amount of time.*/
			for (k = 0; k < l; k++) {
				s16b cx, cy, tries = 10;

				while (--tries) {
					cy = rand_int(2) ? y + 3 + rand_int(8) : y - 3 - rand_int(8);
					cx = rand_int(2) ? x + 3 + rand_int(8) : x - 3 - rand_int(8);

					if (!in_bounds(cy, cx)) continue;

					/* paranoia */
					if (!cave_empty_bold(zcave, cy, cx)) continue;

					break;
				}
				if (!tries) {
					cx = x;
					cy = y;
					/* or maybe just
					break; ? */
				}

				summon_override_checks = SO_IDDC;
				ident |= summon_specific(wpos, cy, cx, glev, 0, SUMMON_SPOOK, 1, 0);
				summon_override_checks = SO_NONE;
			}
			if (ident) {
				msg_print(Ind, "There's something strange in the neighbourhood.");
#ifdef USE_SOUND_2010
				sound_near_site(y, x, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
#endif
			}
			break;

		default:
			/* XXX LUA hook here maybe? */
			s_printf("Executing unknown trap %d\n", trap);
	}

	/* some traps vanish */
	if (magik(vanish)) {
		struct c_special *cs_ptr = GetCS(c_ptr, CS_TRAPS);

		if (item < 0) {
			cs_erase(c_ptr, cs_ptr);

			/* Trap won't redraw while in 'S'earching mode though - fix: */
			everyone_clear_ovl_spot(wpos, y, x);

			/* since player is no longer moved onto the trap on triggering it,
			   we have to redraw the grid (noticed this on door traps) */
			everyone_lite_spot(wpos, y, x);
		} else {
			i_ptr->pval = 0;
			if (i_ptr->tval == TV_CHEST) destroy_chest(i_ptr);
		}

		ident = FALSE;
	} else if (!p_ptr->warning_trap) {
		msg_print(Ind, "\374\377yHINT: You triggered a trap! Traps can be nasty, but if you successfully disarm");
		msg_print(Ind, "\374\377y      one you gain experience from that. To try, step aside and press \377oSHIFT+d\377y.");
		s_printf("warning_trap: %s\n", p_ptr->name);
		p_ptr->warning_trap = 1;
	}

	p_ptr->redraw |= PR_VARIOUS | PR_MISC;

	/* Had the player seen it? */
	if (never_id || p_ptr->image || p_ptr->confused || p_ptr->blind ||
	    no_lite(Ind) || !inarea(&p_ptr->wpos, wpos) ||
	    !los(wpos, p_ptr->py, p_ptr->px, y, x))
	    //!player_has_los_bold(Ind, y, x))
		return (FALSE);
	else return ident;
}

void player_activate_door_trap(int Ind, s16b y, s16b x) {
	player_type *p_ptr = Players[Ind];
	cave_type *c_ptr;
	bool ident = FALSE;
	int t_idx = 0;
	struct c_special *cs_ptr;

	/* Paranoia */
	cave_type **zcave;
	if (!in_bounds(y, x)) return;
	if (!(zcave = getcave(&p_ptr->wpos))) return;

	c_ptr = &zcave[y][x];
	cs_ptr = GetCS(c_ptr, CS_TRAPS);
	t_idx = cs_ptr->sc.trap.t_idx;

	/* Return if trap or door not found */
	//if ((c_ptr->t_idx == 0) ||
	//    !(f_info[c_ptr->feat].flags1 & FF1_DOOR)) return;

	if (!t_idx) return;

	/* Disturb */
	disturb(Ind, 0, 0);

	/* Message */
	//msg_print(Ind, "You found a trap!");
	msg_print(Ind, "You triggered a trap!");

	/* Pick a trap */
	pick_trap(&p_ptr->wpos, y, x);

	/* Hit the trap */
	ident = player_activate_trap_type(Ind, y, x, NULL, -1);
	if (ident && !p_ptr->trap_ident[t_idx]) {
		p_ptr->trap_ident[t_idx] = TRUE;
		msg_format(Ind, "You identified that trap as %s.", t_name + t_info[t_idx].name);
	}
}

/*
 * Places a random trap at the given location.
 * The location must be a valid, empty, clean, floor grid.
 * mod: -1 means no multiplication trap allowed.
 */
// FEAT_DOOR stuffs should be revised after f_info reform	- Jir -
void place_trap(struct worldpos *wpos, int y, int x, int mod) {
	bool more = TRUE;
	s16b trap, lv;
	trap_kind *t_ptr;

	s16b cnt = 0;
	u32b flags;
	cave_type *c_ptr;
	//dungeon_info_type *d_ptr = &d_info[dungeon_type];
	struct c_special *cs_ptr;

	/* Paranoia -- verify location */
	cave_type **zcave;

#ifdef ARCADE_SERVER
	return;
#endif

	/* Not in Arena Monster Challenge, nor PvP Arena */
	if (in_arena(wpos) || in_pvparena(wpos)) return;

	if (!(zcave = getcave(wpos))) return;
	if (!in_bounds(y, x)) return;
	c_ptr = &zcave[y][x];

	/* No traps in Bree - C. Blue */
	if (in_bree(wpos)) return;
	/* Nor in Valinor */
	if (in_valinor(wpos)) return;

	/* No traps over traps/house doors etc */
	/* TODO: allow traps on jumpgates/fountains etc -- no, doesn't make much sense */
	if (c_ptr->special) return;	/* its a pointer now */

	/* Require empty, clean, floor grid */
	/* Hack - '+1' for secret doors */
	if (cave_floor_grid(c_ptr) || c_ptr->feat == FEAT_DEEP_WATER) flags = FTRAP_FLOOR;
	else if ((c_ptr->feat >= FEAT_DOOR_HEAD) &&
		    (c_ptr->feat <= FEAT_DOOR_TAIL + 1))
		flags = FTRAP_DOOR;
	else return;

#if 0 //allow?
	/* no traps on staircases */
	if (c_ptr->feat == FEAT_MORE || c_ptr->feat == FEAT_LESS ||
	    c_ptr->feat == FEAT_WAY_MORE || c_ptr->feat == FEAT_WAY_LESS)
		return;
#endif

	/* no traps on treasure veins */
	if (c_ptr->feat == FEAT_QUARTZ_H || c_ptr->feat == FEAT_QUARTZ_K ||
	    c_ptr->feat == FEAT_MAGMA_H || c_ptr->feat == FEAT_MAGMA_K ||
	    c_ptr->feat == FEAT_SANDWALL_H || c_ptr->feat == FEAT_SANDWALL_K)
		return;

	//if (!cave_naked_bold(zcave, y, x)) return;
	//if (!cave_floor_bold(zcave, y, x)) return;


	/* no traps in town or on first level */
	//if (dlev <= 1) return;

	/* traps only appears on empty floor */
	//if (!cave_floor_grid(c_ptr) && (!(f_info[c_ptr->feat].flags1 & FF1_DOOR))) return;

	/* set flags */
	/*
	if (((c_ptr->feat >= FEAT_DOOR_HEAD) && 
	   (c_ptr->feat <= FEAT_DOOR_TAIL)) ||
	*/
#if 0
	if (f_info[c_ptr->feat].flags1 & FF1_DOOR)
		flags = FTRAP_DOOR;
	else flags = FTRAP_FLOOR;
#endif	// 0

	lv = getlevel(wpos);

	/* try 100 times */
	while ((more) && (cnt++) < 100) {
		trap = rand_int(MAX_T_IDX);
		t_ptr = &t_info[trap];

		/* no 'multiplication 'trap? */
		if (mod == -1 && trap == TRAP_OF_MULTIPLICATION) continue;

		/* no traps below their minlevel */
		if (t_ptr->minlevel > lv) continue;

		/* is this a correct trap now?   */
		if (!(t_ptr->flags & flags)) continue;

		/* No special_gene traps */
		if (t_ptr->flags & FTRAP_SPECIAL_GENE) continue;

		/* Some traps can only spawn on level generation */
		if ((t_ptr->flags & FTRAP_LEVEL_GEN) && !level_generation_time) continue;

		/* hack, no trap door at the bottom of dungeon or in flat(non dungeon) places */
		//if (((d_ptr->maxdepth == dlev) || (d_ptr->flags1 & DF1_FLAT)) && (trap == TRAP_OF_SINKING)) continue;

		/* how probable is this trap   */
		if (rand_int(100) < t_ptr->probability) {
			//c_ptr->t_idx = trap;
			more = FALSE;
			if (!(cs_ptr = AddCS(c_ptr, CS_TRAPS))) return;
			//cs_ptr->type = CS_TRAPS;
			cs_ptr->sc.trap.t_idx = trap;
			cs_ptr->sc.trap.found = FALSE;
			//c_ptr = &zcave[y][x];
		}
	}

	return;
}

/* Unused appearently */
void place_trap_specific(struct worldpos *wpos, int y, int x, int mod, int found) {
	s16b trap;//, lv;
	//trap_kind *t_ptr;

	//s16b cnt = 0;
	//u32b flags;
	cave_type *c_ptr;
	//dungeon_info_type *d_ptr = &d_info[dungeon_type];
	struct c_special *cs_ptr;

	/* Paranoia -- verify location */
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return;
	if (!in_bounds(y, x)) return;
	c_ptr = &zcave[y][x];

	/* No traps in Bree - C. Blue */
	if (in_bree(wpos)) return;
	/* Nor in Valinor */
	if (in_valinor(wpos)) return;

	/* No traps over traps/house doors etc */
	/* TODO: allow traps on jumpgates/fountains etc */
	if (c_ptr->special) return;	/* its a pointer now */

	/* Require empty, clean, floor grid */
	/* Hack - '+1' for secret doors */
#if 0
	if (cave_floor_grid(c_ptr) || c_ptr->feat == FEAT_DEEP_WATER) flags = FTRAP_FLOOR;
	else if ((c_ptr->feat >= FEAT_DOOR_HEAD) && 
			(c_ptr->feat <= FEAT_DOOR_TAIL + 1))
		flags = FTRAP_DOOR;
	else return;
#else
	if (!(cave_floor_grid(c_ptr) || c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat >= FEAT_DOOR_HEAD))
		return;
#endif

	/* no traps on treasure veins */
	if (c_ptr->feat == FEAT_QUARTZ_H || c_ptr->feat == FEAT_QUARTZ_K ||
	    c_ptr->feat == FEAT_MAGMA_H || c_ptr->feat == FEAT_MAGMA_K ||
	    c_ptr->feat == FEAT_SANDWALL_H || c_ptr->feat == FEAT_SANDWALL_K)
		return;

	//if (!cave_naked_bold(zcave, y, x)) return;
	//if (!cave_floor_bold(zcave, y, x)) return;


	/* no traps in town or on first level */
	//if (dlev <= 1) return;

	/* traps only appears on empty floor */
	//if (!cave_floor_grid(c_ptr) && (!(f_info[c_ptr->feat].flags1 & FF1_DOOR))) return;

	/* set flags */
	/*
	if (((c_ptr->feat >= FEAT_DOOR_HEAD) && 
	    (c_ptr->feat <= FEAT_DOOR_TAIL)) ||
	*/
#if 0
	if (f_info[c_ptr->feat].flags1 & FF1_DOOR) flags = FTRAP_DOOR;
	else flags = FTRAP_FLOOR;
#endif	// 0

	//lv = getlevel(wpos);
	trap = mod;
	//t_ptr = &t_info[trap];
	if (!(cs_ptr = AddCS(c_ptr, CS_TRAPS))) return;
	cs_ptr->sc.trap.t_idx = trap;
	cs_ptr->sc.trap.found = FALSE;
	if(found) cs_ptr->sc.trap.found = TRUE;
	everyone_lite_spot(wpos, y, x);

	return;
}


/*
 * Places a random trap on the given chest.
 *
 * The object must be a valid chest.
 */
void place_trap_object(object_type *o_ptr) {
	bool more = TRUE;
	s16b trap;
	trap_kind *t_ptr;
	s16b cnt = 0;

	/* no traps in town or on first level */
	//if (dlev <= 1) return;

	/* try 100 times */
	while ((more) && (cnt++) < 100) {
		trap = randint(MAX_T_IDX - 1);
		t_ptr = &t_info[trap];

		/* no traps below their minlevel */
		/* o_ptr->wpos is not set yet */
		//if (t_ptr->minlevel>getlevel(&o_ptr->wpos)) continue;
		if (t_ptr->minlevel * 3 > o_ptr->level * 4) continue;

		/* is this a correct trap now?   */
		if (!(t_ptr->flags & FTRAP_CHEST)) continue;

		/* No special_gene traps */
		if (t_ptr->flags & FTRAP_SPECIAL_GENE) continue;

		/* how probable is this trap   */
		if (rand_int(100) < t_ptr->probability) {
			o_ptr->pval = trap;
			more = FALSE;
		}
	}

	return;
}

/* Dangerous trap placing function */
/* (Not so dangerous as was once	- Jir -) */
//void wiz_place_trap(int y, int x, int idx)
void wiz_place_trap(int Ind, int trap) {
	player_type *p_ptr = Players[Ind];
	int x = p_ptr->px, y = p_ptr->py;
	worldpos *wpos = &p_ptr->wpos;
	//s16b t_idx;
	trap_kind *t_ptr;
	struct c_special *cs_ptr;

	//s16b cnt = 0;
	u32b flags;
	cave_type *c_ptr;
	//dungeon_info_type *d_ptr = &d_info[dungeon_type];

	/* Paranoia -- verify location */
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return;
	if (!in_bounds(y, x)) return;
	c_ptr = &zcave[y][x];

	/* Require empty, clean, floor grid */
	if (!cave_floor_grid(c_ptr) &&
	    ((c_ptr->feat < FEAT_DOOR_HEAD) ||
	     (c_ptr->feat > FEAT_DOOR_TAIL)) &&
	    c_ptr->feat != FEAT_OPEN) {
		msg_print(Ind, "Inappropriate grid feature type!");
		return;
	}

	/* no traps on treasure veins */
	if (c_ptr->feat == FEAT_QUARTZ_H || c_ptr->feat == FEAT_QUARTZ_K ||
	    c_ptr->feat == FEAT_MAGMA_H || c_ptr->feat == FEAT_MAGMA_K ||
	    c_ptr->feat == FEAT_SANDWALL_H || c_ptr->feat == FEAT_SANDWALL_K) {
		msg_print(Ind, "Inappropriate grid feature type!");
		return;
	}

	/* No traps over traps/house doors etc */
	if (c_ptr->special) {
		msg_print(Ind, "Cave-Special already exists!");
		return;
	}


	//if (!cave_naked_bold(zcave, y, x)) return;
	//if (!cave_floor_bold(zcave, y, x)) return;


	/* no traps in town or on first level */
	//if (dlev <= 1) return;

	/* traps only appears on empty floor */
	//if (!cave_floor_grid(c_ptr) && (!(f_info[c_ptr->feat].flags1 & FF1_DOOR))) return;

	/* set flags */
	/*
	if (((c_ptr->feat >= FEAT_DOOR_HEAD) && 
	   (c_ptr->feat <= FEAT_DOOR_TAIL)) ||
	*/
#if 0
	if (f_info[c_ptr->feat].flags1 & FF1_DOOR)
		flags = FTRAP_DOOR;
	else flags = FTRAP_FLOOR;
#endif	// 0
	/* is this a correct trap now?   */
	if (trap < 0 || MAX_T_IDX < trap) {
		msg_print(Ind, "Trap index is out of range!");
		return;
	}

	t_ptr = &t_info[trap];

	/* is this a correct trap now?   */
	if (!t_ptr->name) {
		msg_print(Ind, "Specified no. of trap does not exist!");
		return;
	}

	/* no traps below their minlevel */
	if (t_ptr->minlevel > getlevel(wpos)) {
		msg_print(Ind, "Warning: The trap is out of depth!");
		//return;
	}

	if (((c_ptr->feat >= FEAT_DOOR_HEAD) && 
			(c_ptr->feat <= FEAT_DOOR_TAIL)) ||
			c_ptr->feat == FEAT_OPEN)
		flags = FTRAP_DOOR;
	else flags = FTRAP_FLOOR;

	/* is this a correct trap now?   */
	if (!(t_ptr->flags & flags)) {
		msg_print(Ind, "Feature type(door/floor) is not proper!");
		return;
	}

	/* hack, no trap door at the bottom of dungeon or in flat(non dungeon) places */
	//if (((d_ptr->maxdepth == dlev) || (d_ptr->flags1 & DF1_FLAT)) && (trap == TRAP_OF_SINKING)) continue;

	//c_ptr->t_idx = trap;

	if (!(cs_ptr = AddCS(c_ptr, CS_TRAPS))) return;
	//cs_ptr->type = CS_TRAPS;
	cs_ptr->sc.trap.t_idx = trap;
	cs_ptr->sc.trap.found = FALSE;
	//c_ptr = &zcave[y][x];

	return;

#if 0
	cave_type *c_ptr = &cave[y][x];

	/* Dangerous enough as it is... */
	if (!cave_floor_grid(c_ptr) && (!(f_info[c_ptr->feat].flags1 & FF1_DOOR))) return;

	c_ptr->t_idx = idx;
#endif	// 0
}


/*
 * Here begin monster traps code
 */

/* Hook to determine if an object is a device */
static bool item_tester_hook_device(object_type *o_ptr) {
	if (is_magic_device(o_ptr->tval)) return (TRUE);
	/* Assume not */
	return (FALSE);
}
/* Hook to determine if an object is a potion */
static bool item_tester_hook_potion(object_type *o_ptr) {
	if ((o_ptr->tval == TV_POTION) ||
	    (o_ptr->tval == TV_POTION2) ||
	    (o_ptr->tval == TV_FLASK)) return (TRUE);
	/* Assume not */
	return (FALSE);
}
/* Hook to determine if an object is a scroll or rune */
static bool item_tester_hook_scroll_rune(object_type *o_ptr) {
	if ((o_ptr->tval == TV_SCROLL) ||
	    (o_ptr->tval == TV_RUNE)) return (TRUE);
	/* Assume not */
	return (FALSE);
}

/*
 * quick hack for ToME floor_carry, used in do_cmd_set_trap		- Jir -
 *
 * c_special holds trapkit o_idx, and trapkit holds trapload o_idx
 */
//static s16b pop_montrap(worldpos *wpos, int y, int x, object_type *j_ptr)
static s16b pop_montrap(int Ind, object_type *j_ptr, u16b next_o_idx) {
	player_type *p_ptr = Players[Ind];
	s16b o_idx;
	worldpos *wpos = &p_ptr->wpos;
	int py = p_ptr->py, px = p_ptr->px;

	/* Make an object */
	o_idx = o_pop();

	/* Success */
	if (o_idx) {
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[o_idx];

		/* Structure Copy */
		object_copy(o_ptr, j_ptr);

		/* Location */
		o_ptr->iy = 255 - py;	/* Megahack - never inbounds XXX */
		o_ptr->ix = px;
		wpcopy(&o_ptr->wpos, wpos);

		/* Forget monster */
		o_ptr->held_m_idx = 0;

		/* Build a stack */
		o_ptr->next_o_idx = next_o_idx;

		/* don't remove it quickly in towns */
		o_ptr->marked2 = ITEM_REMOVAL_MONTRAP;
	}

	/* Result */
	return (o_idx);
}

/*
 * The trap setting code for rogues -MWK- 
 *
 * Also, it will fail or give weird results if the tvals are resorted!
 */
//void do_cmd_set_trap(int Ind, int item_kit, int item_load, int num)
void do_cmd_set_trap(int Ind, int item_kit, int item_load) {
	player_type *p_ptr = Players[Ind];
	//worldpos *wpos = &p_ptr->wpos;
	int py = p_ptr->py, px = p_ptr->px, i;
#if 0
	int item_kit, item_load, i;
#endif	// 0
	int num;

	object_type *o_ptr, *j_ptr, *i_ptr;
	//cptr q,s,c;
	object_type object_type_body;

	u32b f1, f2, f3, f4, f5, f6, esp;

	cave_type *c_ptr;
	cave_type **zcave;
	struct c_special *cs_ptr;

	zcave = getcave(&p_ptr->wpos);
	c_ptr = &zcave[py][px];

	if (!get_skill(p_ptr, SKILL_TRAPPING)) {
		msg_print(Ind, "You aren't proficient in trapping.");
		return;
	}

	/* Check some conditions */
	if (p_ptr->blind) {
		msg_print(Ind, "You can't see anything.");
		return;
	}
	if (no_lite(Ind)) {
		msg_print(Ind, "You don't dare to set a trap in the darkness.");
		return;
	}
	if (p_ptr->confused) {
		msg_print(Ind, "You are too confused!");
		return;
	}

	if ((f_info[c_ptr->feat].flags1 & FF1_PROTECTED) ||
	    (c_ptr->info & CAVE_PROT)) {
		msg_print(Ind, "You cannot set monster traps on this special floor.");
		return;
	}

	/* Only set traps on clean floor grids */
	/* TODO: allow to set traps on poisoned floor */
	if (!cave_clean_bold(zcave, py, px) ||
	    !cave_set_feat_live_ok(&p_ptr->wpos, py, px, FEAT_MON_TRAP) ||
	    c_ptr->special) {
		msg_print(Ind, "You cannot set a trap here.");
		return;
	}

	/* Get the objects */
	o_ptr = &p_ptr->inventory[item_kit];
	j_ptr = &p_ptr->inventory[item_load];

	if (!can_use_verbose(Ind, o_ptr)) return;
	if (!can_use_verbose(Ind, j_ptr)) return;

	/* Trap kits need a second object */
	switch (o_ptr->sval) {
		case SV_TRAPKIT_BOW:
			if (j_ptr->tval != TV_ARROW) return;
			break;
		case SV_TRAPKIT_XBOW:
			if (j_ptr->tval != TV_BOLT) return;
			break;
		case SV_TRAPKIT_SLING:
			if (j_ptr->tval != TV_SHOT) return;
			break;
		case SV_TRAPKIT_POTION:
			if (!item_tester_hook_potion(j_ptr)) return;
			break;
		case SV_TRAPKIT_SCROLL_RUNE:
			if (!item_tester_hook_scroll_rune(j_ptr)) return;
			break;
		case SV_TRAPKIT_DEVICE:
			if (!item_tester_hook_device(j_ptr)) return;
			break;
		default:
			msg_print(Ind, "Unknown trapping kit type!");
			break;
	}

	/* Hack -- yet another anti-cheeze(yaac) */
	if (p_ptr->max_plv < cfg.newbies_cannot_drop || p_ptr->inval) {
		o_ptr->level = 0;
		j_ptr->level = 0;
	}

	/* Assume a single object */
	num = 1;

	/* In some cases, take multiple objects to load */
	/* Hack -- don't ask XXX */
	if (o_ptr->sval != SV_TRAPKIT_DEVICE) {
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		if ((f3 & TR3_XTRA_SHOTS) && (o_ptr->pval > 0)) num += o_ptr->pval;
		if (f2 & (TRAP2_AUTOMATIC_5 | TRAP2_AUTOMATIC_99)) num = 99;
		if (num > j_ptr->number) num = j_ptr->number;
#if 0
		c = format("How many (1-%d)? ", num);
		/* Ask for number of items to use */
		num = get_quantity(c, num);
#endif
	}

	/* Canceled */
	if (!num) return;

	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	/* Take half a turn (otherwise it gets a bit tedious game-flow wise..) */
	p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

	/* Check interference */
	/* Basically it's not so good idea to set traps next to the enemy */
	if (interfere(Ind, 50 - get_skill_scale(p_ptr, SKILL_TRAPPING, 30))) /* setting-trap interference chance */
		return;

	/* Get local object */
	i_ptr = &object_type_body;

	/* Obtain local object for trap content */
	object_copy(i_ptr, j_ptr);

	/* Set number */
	i_ptr->number = num;

	/* Drop it here */
	//cave[py][px].special = floor_carry(py, px, i_ptr);
	if (!(cs_ptr = AddCS(c_ptr, CS_MON_TRAP))) return;

	/*
	 * Hack -- If rods or wands are dropped, the total maximum timeout or 
	 * charges need to be allocated between the two stacks.  If all the items 
	 * are being dropped, it makes for a neater message to leave the original 
	 * stack's pval alone. -LM-
	 */
	if (is_magic_device(j_ptr->tval)) divide_charged_item(i_ptr, j_ptr, 1);

	//cs_ptr->type = CS_MON_TRAP;
	//cs_ptr->sc.montrap.trap_load = pop_montrap(i_ptr);
	i = pop_montrap(Ind, i_ptr, 0);

	/* Obtain local object for trap trigger kit */
	object_copy(i_ptr, o_ptr);

	/* Set number */
	i_ptr->number = 1;

	/* Set difficulty */
	cs_ptr->sc.montrap.difficulty = get_skill(p_ptr, SKILL_TRAPPING);

	/* Drop it here */
	//cave[py][px].special2 = floor_carry(py, px, i_ptr);
	cs_ptr->sc.montrap.trap_kit = pop_montrap(Ind, i_ptr, i);

	/* Modify, Describe, Optimize */
	inven_item_increase(Ind, item_kit, -1);
	inven_item_describe(Ind, item_kit);
	inven_item_increase(Ind, item_load, -num);
	inven_item_describe(Ind, item_load);

	for (i = 0; i < INVEN_TOTAL; i++) {
		if (inven_item_optimize(Ind, i)) break;
	}
	for (i = 0; i < INVEN_TOTAL; i++) {
		inven_item_optimize(Ind, i);
	}

	/* Preserve former feat */
	cs_ptr->sc.montrap.feat = c_ptr->feat;

	/* Actually set the trap */
	cave_set_feat_live(&p_ptr->wpos, py, px, FEAT_MON_TRAP);
}

/*
 * Disamrs the monster traps(no failure)
 */
/* Hrm it's complicated.. 
 * We'd better not touch FEAT and use only CS	- Jir -
 */
void do_cmd_disarm_mon_trap_aux(worldpos *wpos, int y, int x) {
	int this_o_idx, next_o_idx;
	object_type forge;
	object_type *o_ptr;
	object_type *q_ptr;
	cave_type               *c_ptr;
	cave_type **zcave;
	struct c_special *cs_ptr;
	if (!(zcave = getcave(wpos))) return;

	c_ptr = &zcave[y][x];
	cs_ptr = GetCS(c_ptr, CS_MON_TRAP);
	cave_set_feat_live(wpos, y, x, cs_ptr->sc.montrap.feat);

	/* Drop objects being carried */
	for (this_o_idx = cs_ptr->sc.montrap.trap_kit; this_o_idx; this_o_idx = next_o_idx) {
		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Paranoia */
		o_ptr->held_m_idx = 0;

		/* Get local object */
		q_ptr = &forge;

		/* Copy the object */
		object_copy(q_ptr, o_ptr);

		/* Delete the object */
		delete_object_idx(this_o_idx, FALSE);

		/* Drop it */
		drop_near(0, q_ptr, -1, wpos, y, x);
	}

	//cave[py][px].special = cave[py][px].special2 = 0;
	cs_erase(c_ptr, cs_ptr);
}

void erase_mon_trap(worldpos *wpos, int y, int x) {
	int this_o_idx, next_o_idx;
	object_type *o_ptr;
	cave_type *c_ptr;
	cave_type **zcave;
	struct c_special *cs_ptr;

	if (!(zcave = getcave(wpos))) return;

	c_ptr = &zcave[y][x];
	cs_ptr = GetCS(c_ptr, CS_MON_TRAP);
	cave_set_feat_live(wpos, y, x, cs_ptr->sc.montrap.feat);

	/* Erase objects being carried */
	for (this_o_idx = cs_ptr->sc.montrap.trap_kit; this_o_idx; this_o_idx = next_o_idx) {
		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Paranoia */
		o_ptr->held_m_idx = 0;

		/* Delete the object */
		delete_object_idx(this_o_idx, TRUE);
	}

	//cave[py][px].special = cave[py][px].special2 = 0;
	cs_erase(c_ptr, cs_ptr);
}

/* hack: Identify the load? */
static void identify_mon_trap_load(int who, object_type *o_ptr) {
	bool flipped = FALSE;
	if (who <= 0) return;

	/* Combine / Reorder the pack (later) */
	//Players[who]->notice |= (PN_COMBINE | PN_REORDER);
	/* An identification was made */
	if (!object_aware_p(who, o_ptr)) {
		flipped = object_aware(who, o_ptr);
		//object_known(o_ptr);//only for object1.c artifact potion description... maybe obsolete
		if (!(Players[who]->mode & MODE_PVP)) gain_exp(who, (k_info[o_ptr->k_idx].level + (Players[who]->lev >> 1)) / Players[who]->lev);
	}
	/* The item has been tried */
	object_tried(who, o_ptr, flipped);
	/* Window stuff */
	//p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
}

/*
 * Monster hitting a rod trap -MWK-
 *
 * Return TRUE if the monster died
 */
static bool mon_hit_trap_aux_rod(int who, int m_idx, object_type *o_ptr) {
	int dam = 0, typ = 0, rad = 0;//unused huh, cloud = 0, cloudi = 0;
	monster_type *m_ptr = &m_list[m_idx];
	//monster_race    *r_ptr = race_inf(m_ptr);
	int y = m_ptr->fy;
	int x = m_ptr->fx;
	u32b f1, f2, f3, f4, f5, f6, esp, flg = PROJECT_NORF | PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID | PROJECT_JUMP | PROJECT_NODO | PROJECT_NODF;
	//object_kind *tip_ptr = &k_info[lookup_kind(TV_ROD, o_ptr->pval)];
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	bool perc = FALSE;

	cave_type **zcave;
	zcave = getcave(&m_ptr->wpos);

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* Depend on rod type */
        switch (o_ptr->sval) {
	case SV_ROD_DETECT_TRAP:
		//m_ptr->smart |= SM_NOTE_TRAP;
		break;
	case SV_ROD_DETECTION:
		//m_ptr->smart |= SM_NOTE_TRAP;
		break;
	case SV_ROD_ILLUMINATION:
		typ = GF_LITE;//GF_LITE_WEAK;
		dam = damroll(4, 6);
		rad = 2;
		//lite_room(y, x);
		flg &= ~(PROJECT_NODF);
		break;
	case SV_ROD_CURING:
		typ = GF_CURING; //GF_OLD_HEAL;
		dam = 0x4 + 0x8 + 0x10 + 0x20 + 0x100; //damroll(3, 4); /* and heal conf? */
		break;
	case SV_ROD_HEALING:
		typ = GF_OLD_HEAL;
		dam = 300;
		break;
	case SV_ROD_SPEED:
		typ = GF_OLD_SPEED;
		dam = 50;
		break;
	case SV_ROD_TELEPORT_AWAY:
		typ = GF_AWAY_ALL;
		dam = MAX_SIGHT * 5;
		break;
	case SV_ROD_DISARMING:
		break;
	case SV_ROD_LITE:
		typ = GF_LITE;//GF_LITE_WEAK;
		dam = damroll(6, 8);
		break;
	case SV_ROD_SLEEP_MONSTER:
		typ = GF_OLD_SLEEP;
		dam = 50;
		break;
	case SV_ROD_SLOW_MONSTER:
		typ = GF_OLD_SLOW;
		dam = 50;
		break;
	case SV_ROD_DRAIN_LIFE:
		typ = GF_OLD_DRAIN;
		dam = 10 + rand_int(5);
		perc = TRUE;
		break;
	case SV_ROD_POLYMORPH:
		typ = GF_OLD_POLY;
		dam = 50;
		break;
	case SV_ROD_ACID_BOLT:
		typ = GF_ACID;
		dam = damroll(8, 8);
		flg &= ~(PROJECT_NORF | PROJECT_NODO | PROJECT_NODF);
		flg |= PROJECT_EVSG;
		break;
	case SV_ROD_ELEC_BOLT:
		typ = GF_ELEC;
		dam = damroll(6, 8);
		flg &= ~(PROJECT_NORF | PROJECT_NODO | PROJECT_NODF);
		flg |= PROJECT_EVSG;
		break;
	case SV_ROD_FIRE_BOLT:
		typ = GF_FIRE;
		dam = damroll(10, 8);
		flg &= ~(PROJECT_NORF | PROJECT_NODO | PROJECT_NODF);
		flg |= PROJECT_EVSG;
		break;
	case SV_ROD_COLD_BOLT:
		typ = GF_COLD;
		dam = damroll(7, 8);
		flg &= ~(PROJECT_NORF | PROJECT_NODO | PROJECT_NODF);
		flg |= PROJECT_EVSG;
		break;
	case SV_ROD_ACID_BALL:
		typ = GF_ACID;
		dam = 100;
		rad = 2;
		flg &= ~(PROJECT_NODF);
		break;
	case SV_ROD_ELEC_BALL:
		typ = GF_ELEC;
		dam = 90;
		rad = 2;
		flg &= ~(PROJECT_NODF);
		break;
	case SV_ROD_FIRE_BALL:
		typ = GF_FIRE;
		dam = 105;
		rad = 2;
		flg &= ~(PROJECT_NODF);
		break;
	case SV_ROD_COLD_BALL:
		typ = GF_COLD;
		dam = 95;
		rad = 2;
		flg &= ~(PROJECT_NODF);
		break;
	case SV_ROD_HAVOC:
		typ = GF_HAVOC;
		dam = 200;
		rad = 5;
		flg &= ~(PROJECT_LODF);
		break;
	default:
		return (FALSE);
	}

	if (dam) identify_mon_trap_load(who, o_ptr);

	/* Trapping skill influences damage - C. Blue */
	if (perc) {
		/* it's a percentage, don't go crazy */
		dam += GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty / 10;
	} else if (typ != GF_CURING) {
		dam *= (50 + GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty * 3); dam /= 50;
		dam += GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty * 4;
	}

	/* Actually hit the monster */
	if (typ) (void) project(0 - who, rad, &m_ptr->wpos, y, x, dam, typ, flg, "");

	/* Set rod recharge time */
#ifndef NEW_MDEV_STACKING
	o_ptr->pval = (f4 & TR4_CHEAPNESS) ? k_ptr->level / 2 + 5 : k_ptr->level + 10);
#else
	//(TR4_CHEAPNESS is unused/deprecated)
	if (f4 & TR4_CHARGING) o_ptr->pval += (k_ptr->level + 10) / 2;
	else o_ptr->pval += k_ptr->level + 10;
	o_ptr->bpval++;
#endif

	return (zcave[y][x].m_idx == 0 ? TRUE : FALSE);
}

/*
 * Monster hitting a device trap -MWK-
 *
 * Return TRUE if the monster died
 */
static bool mon_hit_trap_aux_staff(int who, int m_idx, object_type *o_ptr) {
	monster_type *m_ptr = &m_list[m_idx];
	//monster_race    *r_ptr = race_inf(m_ptr);
	worldpos wpos = m_ptr->wpos;
	int dam = 0, typ = 0, rad = 0;//unused huh   , cloud = 0, cloudi = 0;
	int k;
	int y = m_ptr->fy;
	int x = m_ptr->fx;
	cave_type **zcave;
	zcave = getcave(&wpos);
	bool id = FALSE;

	/* Depend on staff type */
	switch (o_ptr->sval) {
		case SV_STAFF_IDENTIFY:
		case SV_STAFF_DETECT_DOOR:
		case SV_STAFF_DETECT_INVIS:
		case SV_STAFF_DETECT_EVIL:
		case SV_STAFF_DETECT_GOLD:
		case SV_STAFF_DETECT_ITEM:
		case SV_STAFF_MAPPING:
		case SV_STAFF_PROBING:
		case SV_STAFF_THE_MAGI:
			return (FALSE);

		case SV_STAFF_REMOVE_CURSE:
			typ = GF_DISP_UNDEAD;
			rad = 3;
			dam = 50;
			break;
		case SV_STAFF_DARKNESS:
			//unlite_room(y, x);
			typ = GF_DARK;//GF_DARK_WEAK;
			dam = 20;
			rad = 3;
			break;
		case SV_STAFF_SLOWNESS:
			typ = GF_OLD_SLOW;
			dam = damroll(5, 10);
			rad = 3;
			break;
		case SV_STAFF_HASTE_MONSTERS:
			typ = GF_OLD_SPEED;
			dam = damroll(5, 10);
			rad = 2;
			break;
		case SV_STAFF_SUMMONING:
			for (k = 0; k < randint(4) ; k++)
				id |= summon_specific(&wpos, y, x, getlevel(&wpos), 0, SUMMON_ALL_U98, 1, 0);
			if (id) {
				identify_mon_trap_load(who, o_ptr);
#ifdef USE_SOUND_2010
				sound_near_site(y, x, &wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
#endif
			}
			return (FALSE);
		case SV_STAFF_TELEPORTATION:
			typ = GF_AWAY_ALL;
			dam = 100;
			break;
		case SV_STAFF_STARLITE:
			/* Hack */
			typ = GF_STARLITE;//GF_LITE;//GF_LITE_WEAK;
			dam = damroll(6, 8);
			rad = 3;
			break;
		case SV_STAFF_LITE:
//			lite_room(y, x);
			typ = GF_LITE;//GF_LITE_WEAK;
			dam = damroll(3, 8);
			rad = 2;
			break;
		case SV_STAFF_DETECT_TRAP:
//                        m_ptr->smart |= SM_NOTE_TRAP;
			return (FALSE);
		case SV_STAFF_CURE_SERIOUS:
			typ = GF_OLD_HEAL;
			dam = damroll(6, 8);
			break;
		case SV_STAFF_CURING:
			typ = GF_CURING; //GF_OLD_HEAL;
			dam = 0x4 + 0x8 + 0x10 + 0x20 + 0x100; //randint(4); /* hack */
			break;
		case SV_STAFF_HEALING:
			typ = GF_OLD_HEAL;
			dam = 300;
			break;
		case SV_STAFF_SLEEP_MONSTERS:
			typ = GF_OLD_SLEEP;
			dam = damroll(5, 10);
			rad = 3;
			break;
		case SV_STAFF_SLOW_MONSTERS:
			typ = GF_OLD_SLOW;
			dam = damroll(5, 10);
			rad = 3;
			break;
		case SV_STAFF_SPEED:
			typ = GF_OLD_SPEED;
			dam = damroll(5, 10);
			break;
		case SV_STAFF_DISPEL_EVIL:
			typ = GF_DISP_EVIL;
			dam = 60;
			rad = 3;
			break;
		case SV_STAFF_POWER:
			typ = GF_DISP_ALL;
			dam = 80;
			rad = 3;
			break;
		case SV_STAFF_HOLINESS:
			typ = GF_DISP_EVIL;
			dam = 100;
			rad = 3;
			break;
#if 0
		case SV_STAFF_GENOCIDE:
		{
			monster_race *r_ptr = &r_info[m_ptr->r_idx];
                        genocide_aux(FALSE, r_ptr->d_char);
			/* although there's no point in a multiple genocide trap... */
                        return (cave[y][x].m_idx == 0 ? TRUE : FALSE);
		}
#endif	// 0
		case SV_STAFF_GENOCIDE:
		{
			monster_race    *r_ptr = race_inf(m_ptr);
			genocide_aux(0, &wpos, r_ptr->d_char);
			identify_mon_trap_load(who, o_ptr);
			/* although there's no point in a multiple genocide trap...
			   ..monsters could resist at 1st attempt maybe? */
			return (zcave[y][x].m_idx == 0 ? TRUE : FALSE);
		}
		case SV_STAFF_EARTHQUAKES:
			earthquake(&wpos, y, x, 10);
			identify_mon_trap_load(who, o_ptr);
			return (FALSE);
		case SV_STAFF_DESTRUCTION:
			destroy_area(&wpos, y, x, 15, TRUE, FEAT_FLOOR, 120);
			identify_mon_trap_load(who, o_ptr);
			return (FALSE);
		default:
			return (FALSE);
	}

	identify_mon_trap_load(who, o_ptr);

	/* Trapping skill influences damage - C. Blue */
	if (typ != GF_CURING) {
		dam *= (50 + GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty * 3); dam /= 50;
		dam += GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty * 3;
	}
	/* ..and new, also radius (if any, and if not kind of stone-prison like effect (rad 1) - which is paranoia since we don't have that atm) */
	if (rad >= 2) rad += GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty / 15;

	/* Actually hit the monster */
	(void) project(0 - who, rad, &wpos, y, x, dam, typ, PROJECT_NORF | PROJECT_KILL | PROJECT_ITEM | PROJECT_JUMP | PROJECT_NODO | PROJECT_LODF, "");
	return (zcave[y][x].m_idx == 0 ? TRUE : FALSE);
}

/* 
 * Monster hitting a scroll trap -MWK-
 *
 * Return TRUE if the monster died
 */
static bool mon_hit_trap_aux_scroll(int who, int m_idx, object_type *o_ptr) {
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);
	worldpos wpos = m_ptr->wpos;
	int dam = 0, typ = 0, rad = 0;//unused huh  , cloud = 0, cloudi = 0;
	int y = m_ptr->fy;
	int x = m_ptr->fx;
	int k;
	bool id = FALSE;
	cave_type **zcave;
	zcave = getcave(&wpos);
	dun_level *l_ptr = getfloor(&wpos);
	bool no_summon = (l_ptr && (l_ptr->flags2 & LF2_NO_SUMMON));
	//bool no_teleport = (l_ptr && (l_ptr->flags2 & LF2_NO_TELE));

	/* Depend on scroll type */
	switch (o_ptr->sval) {
		case SV_SCROLL_CURSE_ARMOR:
		case SV_SCROLL_CURSE_WEAPON:
		case SV_SCROLL_TRAP_CREATION: /* these don't work :-( */
		case SV_SCROLL_WORD_OF_RECALL: /* should these? */
		case SV_SCROLL_IDENTIFY:
		case SV_SCROLL_STAR_IDENTIFY:
		case SV_SCROLL_MAPPING:
		case SV_SCROLL_DETECT_GOLD:
		case SV_SCROLL_DETECT_ITEM:
		case SV_SCROLL_ENCHANT_ARMOR:
		case SV_SCROLL_ENCHANT_WEAPON_TO_HIT:
		case SV_SCROLL_ENCHANT_WEAPON_TO_DAM:
		case SV_SCROLL_STAR_ENCHANT_ARMOR:
		case SV_SCROLL_STAR_ENCHANT_WEAPON:
		case SV_SCROLL_RECHARGING:
		case SV_SCROLL_DETECT_DOOR:
		case SV_SCROLL_DETECT_INVIS:
		case SV_SCROLL_SATISFY_HUNGER:
		case SV_SCROLL_TRAP_DOOR_DESTRUCTION:
			return (FALSE);

		case SV_SCROLL_RUNE_OF_PROTECTION:
			typ = GF_STOP;
			dam = 100;
		case SV_SCROLL_PROTECTION_FROM_EVIL:
			typ = GF_DISP_EVIL;
			dam = 100;
			rad = 3;
		case SV_SCROLL_DARKNESS:
			//unlite_room(y, x);
			typ = GF_DARK;//GF_DARK_WEAK;
			dam = 15;
			rad = 3;
			break;
		case SV_SCROLL_AGGRAVATE_MONSTER:
			if (who > 0) {
				identify_mon_trap_load(who, o_ptr);

#ifdef USE_SOUND_2010
				sound_near(who, "shriek", NULL, SFX_TYPE_MON_SPELL);
#endif
				msg_print(who, "\377RYou hear a high-pitched humming noise echoing through the dungeons.");
				msg_print_near(who, "\377RYou hear a high-pitched humming noise echoing through the dungeons.");
				aggravate_monsters(who, m_idx);
			}
			return (FALSE);
		case SV_SCROLL_SUMMON_MONSTER:
			if (no_summon) return FALSE;
			summon_override_checks = SO_IDDC;
			for (k = 0; k < randint(3) ; k++) id |= summon_specific(&wpos, y, x, getlevel(&wpos), 0, SUMMON_ALL_U98, 1, 0);
			summon_override_checks = SO_NONE;
			if (id) {
				identify_mon_trap_load(who, o_ptr);
#ifdef USE_SOUND_2010
				sound_near_site(y, x, &wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
#endif
			}
			return (FALSE);
		case SV_SCROLL_SUMMON_UNDEAD:
			if (no_summon) return FALSE;
			summon_override_checks = SO_IDDC;
			for (k = 0; k < randint(3) ; k++) id |= summon_specific(&wpos, y, x, getlevel(&wpos), 0, SUMMON_UNDEAD, 1, 0);
			summon_override_checks = SO_NONE;
			if (id) {
				identify_mon_trap_load(who, o_ptr);
#ifdef USE_SOUND_2010
				sound_near_site(y, x, &wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
#endif
			}
			return (FALSE);
		case SV_SCROLL_PHASE_DOOR:
			typ = GF_AWAY_ALL;
			dam = 10;
			break;
		case SV_SCROLL_TELEPORT:
			typ = GF_AWAY_ALL;
			dam = 100;
			break;
		case SV_SCROLL_TELEPORT_LEVEL:
			delete_monster(&wpos, y, x, TRUE);
			identify_mon_trap_load(who, o_ptr);
			return (TRUE);
		case SV_SCROLL_LIGHT:
			//lite_room(y, x);
			typ = GF_LITE;//GF_LITE_WEAK;
			dam = damroll(3, 6);
			rad = 2;
			break;
		case SV_SCROLL_DETECT_TRAP:
			//m_ptr->smart |= SM_NOTE_TRAP;
			return (FALSE);
		/* Hrm aren't they too small..? */
		case SV_SCROLL_BLESSING:
			typ = GF_HOLY_FIRE;
			dam = damroll(3, 4);
			break;
		case SV_SCROLL_HOLY_CHANT:
			typ = GF_HOLY_FIRE;
			dam = damroll(10, 4);
			break;
		case SV_SCROLL_HOLY_PRAYER:
			typ = GF_HOLY_FIRE;
			dam = damroll(30, 4);
			break;
		case SV_SCROLL_MONSTER_CONFUSION:
			typ = GF_OLD_CONF;
			dam = damroll(5, 10);
			break;
		case SV_SCROLL_STAR_DESTRUCTION:
			destroy_area(&wpos, y, x, 15, TRUE, FEAT_FLOOR, 120);
			identify_mon_trap_load(who, o_ptr);
			return (FALSE);
		case SV_SCROLL_DISPEL_UNDEAD:
			typ = GF_DISP_UNDEAD;
			rad = 5;
			dam = 100;
			break;
		case SV_SCROLL_GENOCIDE:
			genocide_aux(0, &wpos, r_ptr->d_char);
			/* although there's no point in a multiple genocide trap... */
			//return (!(r_ptr->flags1 & RF1_UNIQUE));
			identify_mon_trap_load(who, o_ptr);
			return (zcave[y][x].m_idx == 0 ? TRUE : FALSE);
		case SV_SCROLL_OBLITERATION:
			obliteration(-m_idx);
			identify_mon_trap_load(who, o_ptr);
			return(TRUE);
		case SV_SCROLL_ACQUIREMENT:
			acquirement(&wpos, y, x, 1, TRUE, (wpos.wz != 0), make_resf(Players[who]));
			identify_mon_trap_load(who, o_ptr);
			return (FALSE);
		case SV_SCROLL_STAR_ACQUIREMENT:
			acquirement(&wpos, y, x, randint(2) + 1, TRUE, (wpos.wz != 0), make_resf(Players[who]));
			identify_mon_trap_load(who, o_ptr);
			return (FALSE);
		case SV_SCROLL_REMOVE_CURSE:
			typ = GF_DISP_UNDEAD;
			rad = 5;
			dam = 30;
			break;
		case SV_SCROLL_STAR_REMOVE_CURSE:
			typ = GF_DISP_UNDEAD;
			rad = 5;
			dam = 200;
			break;
		case SV_SCROLL_LIFE:
			if (r_ptr->d_char == 'G') {
				delete_monster(&wpos, y, x, TRUE);
				identify_mon_trap_load(who, o_ptr);
			}
			return (TRUE);
		case SV_SCROLL_FIRE:
			typ = GF_FIRE;
			rad = 3;
			dam = 100;
			break;
		case SV_SCROLL_ICE:
			typ = GF_ICE;
			rad = 3;
			//dam = 200;
			dam = 90;
			break;
		case SV_SCROLL_CHAOS:
			typ = GF_CHAOS;
			rad = 3;
			dam = 110;
			break;
		default:
			return (FALSE);
	}

	identify_mon_trap_load(who, o_ptr);

	/* Trapping skill influences damage - C. Blue */
	dam *= (50 + GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty * 3); dam /= 50;
	dam += GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty * 4;

	/* Actually hit the monster */
	(void) project(0 - who, rad, &wpos, y, x, dam, typ, PROJECT_NORF | PROJECT_KILL | PROJECT_ITEM | PROJECT_JUMP | PROJECT_NODO | PROJECT_LODF, "");
	return (zcave[y][x].m_idx == 0 ? TRUE : FALSE);
}

/*
 * Monster hitting a wand trap -MWK-
 *
 * Return TRUE if the monster died
 */
static bool mon_hit_trap_aux_wand(int who, int m_idx, object_type *o_ptr) {
	monster_type *m_ptr = &m_list[m_idx];
	int dam = 0, typ = 0, rad = 0;//, cloud = 0, cloudi = 0;
	int y = m_ptr->fy;
	int x = m_ptr->fx;
	cave_type **zcave;
	zcave = getcave(&m_ptr->wpos);
	u32b flg = PROJECT_NORF | PROJECT_KILL | PROJECT_ITEM | PROJECT_JUMP | PROJECT_NODF | PROJECT_NODO;
	bool perc = FALSE;

	/* Depend on wand type */
	switch ((o_ptr->sval == SV_WAND_WONDER) ? rand_int(SV_WAND_WONDER) : o_ptr->sval) {
		case SV_WAND_HEAL_MONSTER:
			typ = GF_OLD_HEAL;
			dam = damroll(4, 6);
			break;
		case SV_WAND_HASTE_MONSTER:
			typ = GF_OLD_SPEED;
			dam = damroll(5, 10);
			break;
		case SV_WAND_CLONE_MONSTER:
			typ = GF_OLD_CLONE;
			break;
		case SV_WAND_TELEPORT_AWAY:
			typ = GF_AWAY_ALL;
			dam = MAX_SIGHT * 5;
		        break;
		case SV_WAND_DISARMING:
			return (FALSE);
		case SV_WAND_TRAP_DOOR_DEST:
			return (FALSE);
		case SV_WAND_STONE_TO_MUD:
			typ = GF_KILL_WALL;
			dam = 50 + randint(30);
			break;
		case SV_WAND_LITE:
			typ = GF_LITE;//GF_LITE_WEAK;
			dam = damroll(6, 8);
			flg &= ~(PROJECT_NODF);
			break;
		case SV_WAND_SLEEP_MONSTER:
			typ = GF_OLD_SLEEP;
			dam = damroll(5, 10);
			break;
		case SV_WAND_SLOW_MONSTER:
			typ = GF_OLD_SLOW;
			dam = damroll(5, 10);
			break;
		case SV_WAND_CONFUSE_MONSTER:
			typ = GF_OLD_CONF;
			dam = damroll(5, 10);
			break;
		case SV_WAND_FEAR_MONSTER:
			typ = GF_TURN_ALL;
			dam = damroll(5, 10);
			break;
		case SV_WAND_DRAIN_LIFE:
			typ = GF_OLD_DRAIN;
			dam = 10 + rand_int(5);
			perc = TRUE;
			break;
		case SV_WAND_POLYMORPH:
			typ = GF_OLD_POLY;
			dam = damroll(5, 10);
			break;
		case SV_WAND_STINKING_CLOUD:
			typ = GF_POIS;
			dam = 20;//4 (if cloud)
			rad = 2;
			//cloud = 4;
			//cloudi = 9;
			break;
		case SV_WAND_MAGIC_MISSILE:
			typ = GF_MISSILE;
			dam = damroll(3, 6);
			flg &= ~(PROJECT_NORF | PROJECT_NODO | PROJECT_NODF);
			break;
		case SV_WAND_ACID_BOLT:
			typ = GF_ACID;
			dam = damroll(6, 8);
			flg &= ~(PROJECT_NORF | PROJECT_NODO | PROJECT_NODF);
			flg |= PROJECT_EVSG;
			break;
		case SV_WAND_FIRE_BOLT:
			typ = GF_FIRE;
			dam = damroll(6, 8);
			flg &= ~(PROJECT_NORF | PROJECT_NODO | PROJECT_NODF);
			flg |= PROJECT_EVSG;
			break;
		case SV_WAND_ELEC_BOLT:
			typ = GF_ELEC;
			dam = damroll(5, 8);
			flg &= ~(PROJECT_NORF | PROJECT_NODO | PROJECT_NODF);
			flg |= PROJECT_EVSG;
			break;
		case SV_WAND_COLD_BOLT:
			typ = GF_COLD;
			dam = damroll(5, 8);
			flg &= ~(PROJECT_NORF | PROJECT_NODO | PROJECT_NODF);
			flg |= PROJECT_EVSG;
			break;
		case SV_WAND_ACID_BALL:
			typ = GF_ACID;
			rad = 2;
			dam = 60;
			flg &= ~(PROJECT_NODF);
			break;
		case SV_WAND_ELEC_BALL:
			typ = GF_ELEC;
			rad = 2;
			dam = 52;
			flg &= ~(PROJECT_NODF);
			break;
		case SV_WAND_FIRE_BALL:
			typ = GF_FIRE;
			rad = 2;
			dam = 68;
			flg &= ~(PROJECT_NODF);
			break;
		case SV_WAND_COLD_BALL:
			typ = GF_COLD;
			dam = 60;
			rad = 2;
			flg &= ~(PROJECT_NODF);
			break;
		case SV_WAND_ANNIHILATION:
			typ = GF_ANNIHILATION;
			//dam = 15 + get_skill_scale(...who > 0.., SKILL_TRAPPING, 15); --hmm, maybe not, trap is the caster after all
			dam = 10 + randint(10); //raise the base a bit.. for now.
			perc = TRUE;
			break;
		case SV_WAND_DRAGON_FIRE:
			typ = GF_FIRE;
			dam = 200;
			rad = 3;
			flg &= ~(PROJECT_NODF);
			break;
		case SV_WAND_DRAGON_COLD:
			typ = GF_COLD;
			dam = 170;
			rad = 3;
			flg &= ~(PROJECT_NODF);
			break;
		case SV_WAND_DRAGON_BREATH:
			switch(randint(5)) {
				case 1: typ = GF_FIRE; break;
				case 2: typ = GF_ELEC; break;
				case 3: typ = GF_ACID; break;
				case 4: typ = GF_COLD; break;
				case 5: typ = GF_POIS; break;
			}
			dam = 250;
			rad = 3;
			break;
		case SV_WAND_TELEPORT_TO:
			typ = GF_TELE_TO;
			break;
		case SV_WAND_ROCKETS:
			typ = GF_ROCKET;
			dam = 300;
			rad = 3;
			flg &= ~(PROJECT_NODF);
			break;
		case SV_WAND_WALL_CREATION:
			identify_mon_trap_load(who, o_ptr);
			/* create a stone prison same as the istar spell - C. Blue */
			project(PROJECTOR_TRAP, 1, &m_ptr->wpos, y, x, 1, GF_STONE_WALL,
				PROJECT_NORF | PROJECT_KILL | PROJECT_JUMP | PROJECT_GRID | PROJECT_ITEM | PROJECT_NODO | PROJECT_NODF, "trap of walls"); /* shouldn't this strictly speaking be a projection from '0 - who' too?.. */
			return (FALSE);

		default:
			return (FALSE);
	}

	identify_mon_trap_load(who, o_ptr);

	/* Trapping skill influences damage - C. Blue */
	if (perc) {
		/* it's a percentage, don't go crazy */
		dam += GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty / 10;
	} else {
		/* normal damage increase */
		dam *= (50 + GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty * 2); dam /= 50;
		dam += GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty * 2;
	}

#ifdef USE_SOUND_2010
	if (rad && who > 0) { /* TODO: make it audible for ALL players in LOS maybe? probably too much */
		if (typ == GF_ROCKET) sound(who, "rocket", NULL, SFX_TYPE_MISC, FALSE);
		else if (typ == GF_DETONATION) sound(who, "detonation", NULL, SFX_TYPE_MISC, FALSE);
		else sound(who, "cast_ball", NULL, SFX_TYPE_MISC, FALSE);
	}
//	else sound(Ind, "", NULL, SFX_TYPE_MISC, FALSE);
#endif

	/* Actually hit the monster */
	(void) project(0 - who, rad, &m_ptr->wpos, y, x, dam, typ, flg, "");
	return (zcave[y][x].m_idx == 0 ? TRUE : FALSE);
}

/*
 * Monster hitting a potions trap -MWK-
 *
 * Return TRUE if the monster died
 */
static bool mon_hit_trap_aux_potion(int who, int m_idx, object_type *o_ptr) {
	monster_type *m_ptr = &m_list[m_idx];
	//monster_race *r_ptr = &r_info[m_ptr->r_idx];
	int dam = 0, typ = 0, rad = 1;//, cloud = 0, cloudi = 0;
	//int i;
	int y = m_ptr->fy;
	int x = m_ptr->fx;
	cave_type **zcave;
	zcave = getcave(&m_ptr->wpos);

	/* Depend on potion type */
	if (o_ptr->tval == TV_POTION) {
		switch (o_ptr->sval) {
		/* Nothing happens */
		case SV_POTION_WATER:
		case SV_POTION_APPLE_JUICE:
		case SV_POTION_SLIME_MOLD:
		case SV_POTION_INFRAVISION:
		case SV_POTION_DETECT_INVIS:
		case SV_POTION_SLOW_POISON:
		case SV_POTION_CURE_POISON:
		case SV_POTION_RESIST_HEAT:
		case SV_POTION_RESIST_COLD:
		case SV_POTION_RESTORE_MANA:
		case SV_POTION_STAR_RESTORE_MANA:
		case SV_POTION_RESTORE_EXP:
		case SV_POTION_ENLIGHTENMENT:
		case SV_POTION_STAR_ENLIGHTENMENT:
		case SV_POTION_SELF_KNOWLEDGE:
			return (FALSE);

		case SV_POTION_SALT_WATER:
			/*if (!r_ptr->flags3 & RF3_NO_STUN) {
				m_ptr->stunned += 5;
				msg_print_near_monster(m_idx, "appears stunned");
			}
			return (FALSE);*/
			typ = GF_BLIND;
			dam = damroll(3, 2);
			break;
		case SV_POTION_DEC_STR:
			typ = GF_DEC_STR;
			dam = 1; /*dummmy*/
			break;
		case SV_POTION_DEC_INT:	
			//m_ptr->aaf--;
		case SV_POTION_DEC_WIS:
			return (FALSE);
		case SV_POTION_DEC_DEX:
			typ = GF_DEC_DEX;
			dam = 1; /*dummmy*/
			break;
		case SV_POTION_DEC_CON:
			typ = GF_DEC_CON;
			dam = 1; /*dummmy*/
			break;
		case SV_POTION_DEC_CHR:
			return (FALSE);
		case SV_POTION_RES_STR:
			typ = GF_RES_STR;
			dam = 1; /*dummmy*/
			break;
		case SV_POTION_RES_INT:
		case SV_POTION_RES_WIS:
			return (FALSE);
		case SV_POTION_RES_DEX:
			typ = GF_RES_DEX;
			dam = 1; /*dummmy*/
			break;
		case SV_POTION_RES_CON:
			typ = GF_RES_CON;
			dam = 1; /*dummmy*/
			break;
		case SV_POTION_RES_CHR:
			return (FALSE);
		case SV_POTION_INC_STR:
			typ = GF_INC_STR;
			dam = 1; /*dummmy*/
			break;
		case SV_POTION_INC_INT:
		case SV_POTION_INC_WIS:
			return (FALSE);
		case SV_POTION_INC_DEX:
			typ = GF_INC_DEX;
			dam = 1; /*dummmy*/
			break;
		case SV_POTION_INC_CON:
			typ = GF_INC_CON;
			dam = 1; /*dummmy*/
			break;
		case SV_POTION_INC_CHR:
			return (FALSE);
		case SV_POTION_AUGMENTATION:
			typ = GF_AUGMENTATION;
			dam = 1; /*dummmy*/
			break;
		case SV_POTION_RUINATION:	/* ??? */
			typ = GF_RUINATION;
			dam = 1; /*dummmy*/
			break;
		case SV_POTION_EXPERIENCE:
			typ = GF_EXP;
			dam = 1; /* level */
			break;
		case SV_POTION_SLOWNESS:
			rad = 2;
			typ = GF_OLD_SLOW;
			dam = damroll(4, 6);
			break;
		case SV_POTION_POISON:
			typ = GF_POIS;
			dam = damroll(8, 6);
			rad = 3;
			//cloud = 3;
			//cloudi = 5;
			break;
		case SV_POTION_CONFUSION:
			rad = 3;
			typ = GF_CONFUSION;
			dam = damroll(5, 4);
			break;
		case SV_POTION_BLINDNESS:
			rad = 3;
			//typ = GF_DARK;
			typ = GF_BLIND;
			dam = 10;
			break;
		case SV_POTION_SLEEP:
			rad = 3;
			typ = GF_OLD_SLEEP;
			dam = damroll (4, 6);
			break;
		case SV_POTION_LOSE_MEMORIES:
			rad = 2;
			typ = GF_OLD_CONF;
			dam = damroll(10, 5);
			break;
		case SV_POTION_DETONATIONS:
			//typ = GF_DISINTEGRATE;
			typ = GF_DETONATION;
			dam = damroll(40, 20);
			rad = 3;
#ifdef USE_SOUND_2010
			/* sound only if we can see the trap detonate */
			if (who > 0 && los(&m_ptr->wpos, Players[who]->py, Players[who]->px, y, x)) {
				sound(who, "detonation", NULL, SFX_TYPE_MISC, FALSE);
			}
#endif
			break;
		case SV_POTION_DEATH:
			rad = 3;
			typ = GF_NETHER_WEAK;
			dam = damroll(60, 20);
			break;
		case SV_POTION_BOLDNESS:
			/*if (m_ptr->monfear) msg_print_near_monster(m_idx, "recovers the courage");
			m_ptr->monfear = 0;
			return (FALSE);*/
			rad = 4;
			typ = GF_REMFEAR;
			dam = 1; /*dummy*/
			break;
		case SV_POTION_SPEED:
			rad = 2;
			typ = GF_OLD_SPEED;
			dam = damroll(5, 10);
			break;
		case SV_POTION_HEROISM:
		case SV_POTION_BERSERK_STRENGTH:
			/*if (m_ptr->monfear) msg_print_near_monster(m_idx, "recovers the courage");
			m_ptr->monfear = 0;
			typ = GF_OLD_HEAL;*/
			typ = GF_HERO_MONSTER;
			dam = damroll(2, 10);
			rad = 2;
			break;
		case SV_POTION_CURE_LIGHT:
			typ = GF_OLD_HEAL;
			dam = damroll(3, 4);
			rad = 2;
			break;
		case SV_POTION_CURE_SERIOUS:
			typ = GF_OLD_HEAL;
			dam = damroll(4, 6);
			rad = 2;
			break;
		case SV_POTION_CURE_CRITICAL:
			typ = GF_OLD_HEAL;
			dam = damroll(6, 8);
			rad = 2;
			break;
		case SV_POTION_CURING:
			typ = GF_CURING; //GF_OLD_HEAL;
			dam = 0x4 + 0x8 + 0x10 + 0x20 + 0x100; //300;
			rad = 3;
			break;
		case SV_POTION_HEALING:
			typ = GF_OLD_HEAL;
			dam = 300;
			rad = 3;
			break;
		case SV_POTION_STAR_HEALING:
			typ = GF_OLD_HEAL;
			dam = 700;
			rad = 4;
			break;
		case SV_POTION_LIFE:
			typ = GF_LIFEHEAL;
			dam = damroll(30, 20);
			rad = 3;
			break;
		default:
			return (FALSE);
		}
	}
	else if (o_ptr->tval == TV_FLASK) {
		typ = GF_FIRE;
		dam = damroll(2, 4);
		rad = 2;
	}
	else {
		return (FALSE);
	}

	identify_mon_trap_load(who, o_ptr);

	/* Trapping skill influences damage - C. Blue */
	if (typ != GF_CURING) {
		dam *= (150 + GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty); dam /= 50;
		dam += GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty * 1;
	}

	/* Actually hit the monster */
	//(void) project_m(who, y, x, 0, y, x, dam, typ);
	(void) project(0 - who, rad, &m_ptr->wpos, y, x, dam, typ, (PROJECT_NORF | PROJECT_JUMP | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO | PROJECT_LODF), "");
	return (zcave[y][x].m_idx == 0 ? TRUE : FALSE);
}

static bool mon_hit_trap_aux_rune(int who, int m_idx, object_type *o_ptr) {
	monster_type *m_ptr = &m_list[m_idx];
	worldpos wpos = m_ptr->wpos;
	int dam = 0, typ = 0, rad = 0;
	int y = m_ptr->fy;
	int x = m_ptr->fx;
	cave_type **zcave;
	zcave = getcave(&wpos);

	switch (o_ptr->sval) {
	case SV_R_LITE:
		typ = GF_LITE;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_DARK:
		typ = GF_DARK;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_NEXU:
		typ = GF_NEXUS;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_NETH:
		typ = GF_NETHER;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_CHAO:
		typ = GF_CHAOS;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_MANA:
		typ = GF_MANA;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_CONF:
		typ = GF_CONFUSION;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_INER:
		typ = GF_INERTIA;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_ELEC:
		typ = GF_ELEC;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_FIRE:
		typ = GF_FIRE;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_WATE:
		typ = GF_WAVE;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_GRAV:
		typ = GF_GRAVITY;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_COLD:
		typ = GF_COLD;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_ACID:
		typ = GF_ACID;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_POIS:
		typ = GF_POIS;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_TIME:
		typ = GF_TIME;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_SOUN:
		typ = GF_SOUND;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_SHAR:
		typ = GF_SHARDS;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_DISE:
		typ = GF_DISENCHANT;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_ICEE:
		typ = GF_ICE;
		dam = damroll(10, 20);
		rad = 2;
	break;
	case SV_R_PLAS:
		typ = GF_PLASMA;
		dam = damroll(10, 20);
		rad = 2;
	break;
	default:
		return (FALSE);
	}

	/* Trapping skill influences damage - C. Blue */
	dam *= (50 + GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty * 3); dam /= 50;
	dam += GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty * 2;

	/* Actually hit the monster */
	(void) project(0 - who, rad, &wpos, y, x, dam, typ, PROJECT_NORF | PROJECT_KILL | PROJECT_ITEM | PROJECT_JUMP | PROJECT_NODO | PROJECT_LODF, "");
	return (zcave[y][x].m_idx == 0 ? TRUE : FALSE);
}

/*
 * Monster hitting a monster trap -MWK-
 * Returns True if the monster died, false otherwise
 */
/*
 * I'm not sure if this function should take 'Ind'	- Jir -
 *
 * XXX: should we allow 'Magic Arrow' monster traps? :)
 */
bool mon_hit_trap(int m_idx) {
	player_type *p_ptr = (player_type*)NULL;
	monster_type *m_ptr = &m_list[m_idx];
	//	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_race    *r_ptr = race_inf(m_ptr);
#if 0 /* DGDGDGDG */
	monster_lore *lore_ptr = &l_list[m_ptr->r_idx];
#endif

	object_type *kit_o_ptr, *load_o_ptr, *j_ptr;
	struct c_special *cs_ptr;

	u32b f1, f2, f3, f4, f5, f6, esp;

	object_type object_type_body;

	int mx = m_ptr->fx;
	int my = m_ptr->fy;

	int difficulty = 0;
	int smartness;

	char m_name[MNAME_LEN];
	char brand_msg[MAX_CHARS_WIDE] = { '\0' };

	bool notice = FALSE;
	bool disarm = FALSE;
	bool remove = FALSE;
	bool dead = FALSE;
	bool fear = FALSE;
	//s32b special = 0;

	int dam, chance, shots, trapping;
	int mul = 0;

	int i, who = PROJECTOR_MON_TRAP;
	cave_type *c_ptr;
	cave_type **zcave;
	//worldpos *wpos = &m_ptr->wpos;
	worldpos wpos = m_ptr->wpos;

	zcave = getcave(&wpos);
	if (!zcave) return(FALSE);

	c_ptr = &zcave[my][mx];

	cs_ptr = GetCS(c_ptr, CS_MON_TRAP);
	if (!cs_ptr) return(FALSE);

	/* Get the trap objects */
	kit_o_ptr = &o_list[cs_ptr->sc.montrap.trap_kit];
	load_o_ptr = &o_list[kit_o_ptr->next_o_idx];
	j_ptr = &object_type_body;

	/* Paranoia */
	if (!kit_o_ptr || !load_o_ptr) return (FALSE);

	/* Get trap properties */
	object_flags(kit_o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* Can set off check */
	/* Ghosts only set off Ghost traps */
	if ((r_ptr->flags2 & RF2_PASS_WALL) && !(f2 & TRAP2_KILL_GHOST)) return (FALSE);

	/* Some traps are specialized to some creatures */
	if (f2 & TRAP2_ONLY_MASK) {
		bool affect = FALSE;
		if ((f2 & TRAP2_ONLY_DRAGON) && (r_ptr->flags3 & RF3_DRAGON)) affect = TRUE;
		if ((f2 & TRAP2_ONLY_DEMON)  && (r_ptr->flags3 & RF3_DEMON))  affect = TRUE;
		if ((f2 & TRAP2_ONLY_UNDEAD) && (r_ptr->flags3 & RF3_UNDEAD)) affect = TRUE;
		if ((f2 & TRAP2_ONLY_EVIL)   && (r_ptr->flags3 & RF3_EVIL))   affect = TRUE;
		if ((f2 & TRAP2_ONLY_ANIMAL) && (r_ptr->flags3 & RF3_ANIMAL)) affect = TRUE;

		/* Don't set it off if forbidden */
		if (!affect) return (FALSE);
	}

	/* XXX Hack -- is the trapper online? */
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		/* Check if they are in here */
		if (kit_o_ptr->owner == p_ptr->id) {
			who = i;
			break;
		}
	}
	if (who > 0 && p_ptr->mon_vis[m_idx]) monster_desc(who, m_name, m_idx, 0);

	/* Get detection difficulty */
	/* High level players hide traps better */
	trapping = cs_ptr->sc.montrap.difficulty + 5;
	difficulty = (trapping * 3) / 2;	/* somewhat boosted than ToME */

	/* Darkness helps */
	if (!(c_ptr->info & (CAVE_LITE | CAVE_GLOW)) &&
	    !(r_ptr->flags9 & RF9_HAS_LITE) &&
	    !(r_ptr->flags4 & RF4_BR_DARK) &&
	    !(r_ptr->flags6 & RF6_DARKNESS))
		difficulty += 20;

	/* Some traps are well-hidden */
	if (f1 & TR1_STEALTH) difficulty += 10 * (kit_o_ptr->pval);

	/* Get monster smartness for trap detection */
	/* Higher level monsters are smarter */
	smartness = m_ptr->level;

	/* Smart monsters are better at detecting traps */
	if (r_ptr->flags2 & RF2_SMART) smartness += 10;

	/* Some monsters are great at detecting traps */
#if 0 /* DGDGDGDG */
	if (r_ptr->flags2 & RF2_NOTICE_TRAP) smartness += 20;
#endif
	/* Some monsters have already noticed one of out traps */
	//        if (m_ptr->smart & SM_NOTE_TRAP) smartness += 20;

	/* Stupid monsters are no good at detecting traps */
	if (r_ptr->flags2 & (RF2_STUPID | RF2_EMPTY_MIND)) smartness = -150;

	/* Check if the monster notices the trap */
	if (randint(300) > (difficulty - smartness + 150)) notice = TRUE;

	/* Disarm check */
	if (notice) {
		/* The next traps will be easier to spot! */
		//                m_ptr->smart |= SM_NOTE_TRAP;

		/* Tell the player about it */
#if 0 /* DGDGDGDG */
		if (m_ptr->ml) lore_ptr->r_flags2 |= (RF2_NOTICE_TRAP & r_ptr->flags2);
#endif
		/* Get trap disarming difficulty */
		difficulty = (kit_o_ptr->ac + kit_o_ptr->to_a);

		/* Get monster disarming ability */
		/* Higher level monsters are better */
		smartness = m_ptr->level / 5;

		/* Some monsters are great at disarming */
#if 0 /* DGDGDGDG */
		if (r_ptr->flags2 & RF2_DISARM_TRAP) smartness += 20;
#endif
		/* After disarming one trap, the next is easier */
#if 0 /* DGDGDGDG */
		if (m_ptr->status & STATUS_DISARM_TRAP) smartness += 20;
#endif
		/* Smart monsters are better at disarming */
		if (r_ptr->flags2 & RF2_SMART) smartness *= 2;

		/* Stupid monsters never disarm traps */
		if (r_ptr->flags2 & RF2_STUPID) smartness = -150;

		/* Nonsmart animals never disarm traps */
		if ((r_ptr->flags3 & RF3_ANIMAL) && !(r_ptr->flags2 & RF2_SMART)) smartness = -150;

		/* Check if the monster disarms the trap */
		if (randint(120) > (difficulty - smartness + 80)) disarm = TRUE;
	}

	/* If disarmed, remove the trap and print a message */
	if (disarm) {
		remove = TRUE;

		/* Next time disarming will be easier */
#if 0 /* DGDGDGDG */
		m_ptr->status |= STATUS_DISARM_TRAP;
#endif
#if 0
		if (who > 0 && p_ptr->mon_vis[m_idx]) {
			/* Get the name */
			monster_desc(who, m_name, m_idx, 0);

			/* Tell the player about it */
#if 0 /* DGDGDGDG */
			lore_ptr->r_flags2 |= (RF2_DISARM_TRAP & r_ptr->flags2);
#endif
			/* Print a message */
			msg_format(who, "%^s disarms a trap!", m_name);
		}
#endif
		msg_print_near_monster(m_idx, "disarms a trap!");
#ifdef USE_SOUND_2010
		sound_near_monster(m_idx, "disarm", NULL, SFX_TYPE_MISC);
#endif
	}

	/* Trap doesn't get removed but is also not ready to fire?
	   (Important only for rod-traps, so they don't get remove'd w/o actually having fired). */
	if (!disarm &&
	    kit_o_ptr->sval == SV_TRAPKIT_DEVICE &&
	    load_o_ptr->tval == TV_ROD) {
#ifndef NEW_MDEV_STACKING
		if (load_o_ptr->pval) return FALSE;
#else
		if (load_o_ptr->bpval == load_o_ptr->number) return FALSE;
#endif
	}

	/* Otherwise, activate the trap! */
	if (!disarm) {
#if 0
		/* Message for visible monster */
		if (who > 0 && p_ptr->mon_vis[m_idx]) {
			/* Get the name */
			monster_desc(who, m_name, m_idx, 0);

			/* Print a message */
			msg_format(who, "%^s sets off a trap!", m_name);
		} else {
			/* No message if monster isn't visible ? */
		}
#endif
		//msg_print_near_monster(m_idx, "sets off a trap!");
#ifdef USE_SOUND_2010
		sound_near_monster(m_idx, "trap_setoff", NULL, SFX_TYPE_MISC);
#endif

		/* Next time be more careful */
		//if (randint(100) < 80) m_ptr->smart |= SM_NOTE_TRAP;

		/* Actually activate the trap */
		switch (kit_o_ptr->sval) {
		case SV_TRAPKIT_BOW:
		case SV_TRAPKIT_XBOW:
		case SV_TRAPKIT_SLING:
			/* Get number of shots */
			shots = 1;
			if (f3 & TR3_XTRA_SHOTS) shots += kit_o_ptr->pval;
			if (shots <= 0) shots = 1;
			if (shots > load_o_ptr->number) shots = load_o_ptr->number;

			/* Paranoia */
			if (shots <= 0) remove = TRUE;

			while (shots-- && !dead) {
				/* Total base damage */
				dam = damroll(load_o_ptr->dd, load_o_ptr->ds) + (load_o_ptr->to_d + kit_o_ptr->to_d + 10) / 2;

				/* Total hit probability */
				chance = (kit_o_ptr->to_h + load_o_ptr->to_h + 20) * BTH_PLUS_ADJ;

				/* Damage multiplier */
				if (kit_o_ptr->sval == SV_TRAPKIT_SLING) mul = 30;
				if (kit_o_ptr->sval == SV_TRAPKIT_BOW) mul = 35;
				if (kit_o_ptr->sval == SV_TRAPKIT_XBOW) mul = 40;
				if (f3 & TR3_XTRA_MIGHT) mul += (kit_o_ptr->pval * 10);
				if (mul < 0) mul = 0;

				/* Multiply damage */
				dam = (dam * mul) / 10;

				/* Trapping skill influences damage - C. Blue */
				//dam *= (50 + GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty * 1); dam /= 50;
				dam += GetCS(&zcave[m_ptr->fy][m_ptr->fx], CS_MON_TRAP)->sc.montrap.difficulty * 2;

				/* Check if we hit the monster */
				if (test_hit_fire(chance, m_ptr->ac, TRUE)) {
					/* Assume a default death */
					cptr note_dies = " sets off a missile trap and dies";

					/* Some monsters get "destroyed" */
					if ((r_ptr->flags3 & (RF3_DEMON)) ||
					    (r_ptr->flags3 & (RF3_UNDEAD)) ||
					    (r_ptr->flags2 & (RF2_STUPID)) ||
					    (strchr("Evg", r_ptr->d_char))) {
						/* Special note at death */
						note_dies = " sets off a missile trap and is destroyed";
					}

					/* Message if visible */
#if 0
					if (who > 0 && p_ptr->mon_vis[m_idx]) {
						/* describe the monster (again, just in case :-) */
						monster_desc(who, m_name, m_idx, 0);

						/* Print a message */
						msg_format(who, "%^s is hit by a missile.", m_name);
					}
#endif	// 0

					/* Apply slays, brand, critical hits */
					// dam = tot_dam_aux(who, load_o_ptr, dam, m_ptr, &special, brand_msg);
					dam = tot_dam_aux(0, load_o_ptr, dam, m_ptr, brand_msg, FALSE);
					dam = critical_shot(0, load_o_ptr->weight + trapping * 10, load_o_ptr->to_h, dam, FALSE);

					/* No negative damage */
					if (dam < 0) dam = 0;

					/* If another monster did the damage, hurt the monster by hand */
					if (who <= 0) {
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
							/* Generate treasure, etc */
							//			if (!quiet) monster_death(Ind, c_ptr->m_idx);
							/* if treasure is generated, change TRUE to FALSE in below delete_monster_idx ! */
							/* Delete the monster */
							delete_monster_idx(c_ptr->m_idx,TRUE);

							dead = TRUE;

							/* Give detailed messages if destroyed */
							//if (!quiet && note) msg_format(Ind, "%^s%s", m_name, note);
						}

						/* Damaged monster */
						else {
#if 0
							/* Give detailed messages if visible or destroyed */
							if (!quiet && note && seen) msg_format(Ind, "%^s%s", m_name, note);

							/* Hack -- Pain message */
							else if (!quiet && dam > 0) message_pain(Ind, c_ptr->m_idx, dam);

							/* Hack -- handle sleep */
							if (do_sleep) m_ptr->csleep = do_sleep;
#endif
							msg_print_near_monster(m_idx, format("sets off a missile trap for %d damage.", dam));
						}
					}
					/* Hit the monster, check for death */
					else if (mon_take_hit(who, m_idx, dam, &fear, note_dies)) {
						/* Dead monster */
						dead = TRUE;
					}

					/* No death */
					else {
						if (who > 0 && p_ptr->mon_vis[m_idx]) {
#if 0 /* redundant with message_pain() below -> double message */
							if (r_ptr->flags1 & RF1_UNIQUE) {
								if (Players[who]->r_killed[m_ptr->r_idx] == 1) {
									msg_format(who, "\377D%^s sets off a missile trap for \377e%d \377Ddamage.", m_name, dam);
									if (Players[who]->warn_unique_credit) Send_beep(who);
								} else
									msg_format(who, "%^s sets off a missile trap for \377e%d \377wdamage.", m_name, dam);
							} else
								msg_format(who, "%^s sets off a missile trap for \377g%d \377wdamage.", m_name, dam);
#else
							msg_format(who, "%^s sets off a missile trap.", m_name);
#endif

							message_pain(who, m_idx, dam);

							//if (special) attack_special(m_ptr, special, dam);

							/* Take note */
							if (fear && !(m_ptr->csleep || m_ptr->stunned > 100)) {
//								msg_format(who, "%^s flees in terror!", m_name);
								if (m_ptr->r_idx != RI_MORGOTH)
									msg_print_near_monster(m_idx, "flees in terror!");
								else
									msg_print_near_monster(m_idx, "retreats!");
							}
						}
					}

				}

				/* KABOOM! */
				/* Hack -- minus * minus * minus = minus */
				if (load_o_ptr->pval) do_arrow_explode(who > 0 ? who : 0 - who, load_o_ptr, &wpos, my, mx, 2);

				if (!(load_o_ptr->tval == TV_ARROW &&
				    load_o_ptr->sval == SV_AMMO_MAGIC)
				    && !artifact_p(load_o_ptr)) {

					/* Copy and decrease ammo */
					object_copy(j_ptr, load_o_ptr);
					j_ptr->number = 1;

					/* Ethereal ammo decreases more slowly */
					if (load_o_ptr->name2 == EGO_ETHEREAL || load_o_ptr->name2b == EGO_ETHEREAL) {
						if (magik(breakage_chance(load_o_ptr))) load_o_ptr->number--;
					} else load_o_ptr->number--;

					if (load_o_ptr->number <= 0) {
						remove = TRUE;
						delete_object_idx(kit_o_ptr->next_o_idx, TRUE);
						kit_o_ptr->next_o_idx = 0;
					}

					/* Drop (or break) near that location */
					if (!load_o_ptr->pval) drop_near(0, j_ptr, breakage_chance(j_ptr), &wpos, my, mx);
				}

			}
			break;

		case SV_TRAPKIT_POTION:
			/* Get number of shots */
			shots = 1;
			if (f3 & TR3_XTRA_SHOTS) shots += kit_o_ptr->pval;
			if (shots <= 0) shots = 1;
			if (shots > load_o_ptr->number) shots = load_o_ptr->number;

			/* Paranoia */
			if (shots <= 0) remove = TRUE;

			while (shots-- && !dead) {

				/* Message if visible */
#if 0
				if (who > 0 && p_ptr->mon_vis[m_idx]) {
					/* describe the monster (again, just in case :-) */
					monster_desc(who, m_name, m_idx, 0);

					/* Print a message */
					msg_format(who, "%^s is hit by fumes.", m_name);
				}
#endif
				msg_print_near_monster(m_idx, "is hit by fumes.");

				/* Get the potion effect */
				dead = mon_hit_trap_aux_potion(who, m_idx, load_o_ptr);

				/* Copy and decrease ammo */
				object_copy(j_ptr, load_o_ptr);

				j_ptr->number = 1;

				load_o_ptr->number--;

				if (load_o_ptr->number <= 0) {
					remove = TRUE;
					delete_object_idx(kit_o_ptr->next_o_idx, TRUE);
					kit_o_ptr->next_o_idx = 0;
				}
			}
			break;

		case SV_TRAPKIT_SCROLL_RUNE:
			/* Get number of shots */
			shots = 1;
			if (f3 & TR3_XTRA_SHOTS) shots += kit_o_ptr->pval;
			if (shots <= 0) shots = 1;
			if (shots > load_o_ptr->number) shots = load_o_ptr->number;

			/* Paranoia */
			if (shots <= 0) remove = TRUE;

			while (shots-- && !dead) {

				/* Message if visible */
#if 0
				if (who > 0 && p_ptr->mon_vis[m_idx]) {
					/* describe the monster (again, just in case :-) */
					monster_desc(who, m_name, m_idx, 0);

					/* Print a message */
					msg_format(who, "%^s activates a spell!", m_name);
				}
#endif
				msg_print_near_monster(m_idx, "activates a spell!");

				/* Get the scroll or rune effect */
				if (load_o_ptr->tval == TV_SCROLL)
					dead = mon_hit_trap_aux_scroll(who, m_idx, load_o_ptr);
				else
					dead = mon_hit_trap_aux_rune(who, m_idx, load_o_ptr);

				/* Copy and decrease ammo */
				object_copy(j_ptr, load_o_ptr);
				j_ptr->number = 1;
				load_o_ptr->number--;

				if (load_o_ptr->number <= 0) {
					remove = TRUE;
					/* runes stay, scrolls poof */
					if (load_o_ptr->tval == TV_SCROLL) {
						delete_object_idx(kit_o_ptr->next_o_idx, TRUE);
						kit_o_ptr->next_o_idx = 0;
					} else {
						/* hack: runes don't get consumed, so counter the previous decrement */
						load_o_ptr->number++;
					}
				}
			}
			break;

		case SV_TRAPKIT_DEVICE:
			/* Get number of shots */
			shots = 1;
			if (load_o_ptr->tval == TV_ROD) {
#ifndef NEW_MDEV_STACKING
				if (load_o_ptr->pval) shots = 0;
#else
				if (load_o_ptr->bpval == load_o_ptr->number) shots = 0;
#endif
			} else {
				if (f3 & TR3_XTRA_SHOTS) shots += kit_o_ptr->pval;
				if (shots <= 0) shots = 1;
				if (shots > load_o_ptr->pval) shots = load_o_ptr->pval;
			}
			while (shots-- && !dead) {
#if 0
				/* Message if visible */
				// if (m_ptr->ml)
				if (who > 0 && p_ptr->mon_vis[m_idx]) {
					/* describe the monster (again, just in case :-) */
					monster_desc(m_name, m_idx, 0);

					/* Print a message */
					msg_format(Ind, "%^s is hit by some magic.", m_name);
				}
#endif
				/* Get the effect effect */
				switch(load_o_ptr->tval) {
				case TV_ROD:
					dead = mon_hit_trap_aux_rod(who, m_idx, load_o_ptr);
					break;
				case TV_WAND:
					dead = mon_hit_trap_aux_wand(who, m_idx, load_o_ptr);
					break;
				case TV_STAFF:
					dead = mon_hit_trap_aux_staff(who, m_idx, load_o_ptr);
					break;
				}
				/* Decrease charges */
				if (load_o_ptr->tval != TV_ROD) load_o_ptr->pval--;
			}
			break;

		default:
			s_printf("oops! nonexisting trap(sval: %d)!\n", kit_o_ptr->sval);
		}

		/* Non-automatic traps are removed */
		if (!(f2 & (TRAP2_AUTOMATIC_5 | TRAP2_AUTOMATIC_99))) remove = TRUE;
		else if (f2 & TRAP2_AUTOMATIC_5) remove = (randint(5) == 1);

	}

	/* Special trap effect -- teleport to */
	if ((f2 & TRAP2_TELEPORT_TO) && (!disarm) && (!dead) && who > 0)
		teleport_to_player(who, m_idx);

	/* Remove the trap if inactive now */
	//if (remove) cave_set_feat_live(wpos, my, mx, FEAT_FLOOR);
	if (remove) do_cmd_disarm_mon_trap_aux(&wpos, my, mx);

	/* did it die? */
	return (dead);
}

static void destroy_chest(object_type *o_ptr) {
	/* Hack to destroy chests */
	if (o_ptr && o_ptr->tval == TV_CHEST) { /* check for o_ptr - chest might already be destroyed at this point */
		//delete_object_idx(k, TRUE);
		o_ptr->sval = SV_CHEST_RUINED; /* Ruined chest now */
		o_ptr->pval = 0; /* untrapped */
		//o_ptr->bpval = 0;
		o_ptr->ident |= ID_KNOWN; /* easy to see it's a goner */
	}
}
