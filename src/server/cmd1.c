/* $Id$ */
/* File: cmd1.c */

/* Purpose: Movement commands (part 1) */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"


/* Disallow ghost-travelling to other towns? */
#define PROHIBIT_GHOST_TRAVEL

/* Nice: minimum level of a player to be able to become infected by Black Breath by another player */
#define BB_INFECT_MINLEV 25


static void run_init(int Ind, int dir);
static void moved_player(int Ind, player_type *p_ptr, cave_type **zcave, int ox, int oy);


/* Anti-(nothing)-hack, following Tony Zeigler's (Ravyn) suggestion */
#ifdef BACKTRACE_NOTHINGS
 #include <execinfo.h>
#endif
bool nothing_test(object_type *o_ptr, player_type *p_ptr, worldpos *wpos, int x, int y, int loc) {
	char o_name[ONAME_LEN];
	cave_type **zcave = getcave(wpos);
	int idx = zcave ? zcave[y][x].o_idx : -1;

	if ((o_ptr->wpos.wx != wpos->wx) || (o_ptr->wpos.wy != wpos->wy) || (o_ptr->wpos.wz != wpos->wz) ||
	    (!o_ptr->held_m_idx && ((o_ptr->ix && (o_ptr->ix != x)) || (o_ptr->iy && (o_ptr->iy != y))))) {
		/* Item is not at the same (or similar) location as the player? Then he can't pick it up.. */
		object_desc(0, o_name, o_ptr, TRUE, 3);
		if (p_ptr != NULL) {
#if 1
			s_printf("NOTHINGHACK (%d): item %s at %d,%d,%d (%d,%d) meets not target of %s at %d,%d,%d (%d,%d)(c-oi %d)\n",
			    loc, o_name, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz, o_ptr->ix, o_ptr->iy,
			    p_ptr->name, wpos->wx, wpos->wy, wpos->wz, x, y, idx);
#endif
		} else {
#if 1
			s_printf("NOTHINGHACK (%d): item %s at %d,%d,%d (%d,%d) meets not target at %d,%d,%d (%d,%d) (c-oi %d)\n",
			    loc, o_name, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz, o_ptr->ix, o_ptr->iy,
			    wpos->wx, wpos->wy, wpos->wz, x, y, idx);
#endif
		}
#ifdef BACKTRACE_NOTHINGS
		{
			int size, i;
			void *buf[1000];
			char **fnames;

			size = backtrace(buf, 1000);
			s_printf("size = %d\n", size);

			fnames = backtrace_symbols(buf, size);
			for (i = 0; i < size; i++)
				s_printf("%s\n", fnames[i]);
		}
#endif
#ifdef FIX_NOTHINGS
		if (zcave) {
			zcave[y][x].o_idx = 0;
			everyone_lite_spot(wpos, y, x);
			s_printf("NOTHINGHACK: cleared cave reference\n");
		}
#endif
		return(TRUE);
	}
	return(FALSE);
}
bool nothing_test2(cave_type *c_ptr, int x, int y, struct worldpos *wpos, int marker) {
	object_type *o_ptr;

	if (!c_ptr->o_idx) {
#if 0 /* happens as it should in object2.c, marker 0, it seems. */
		s_printf("NOTHING_TEST2: o_idx = 0 (%d)\n", marker);
#endif
		return(FALSE);
	}

	o_ptr = &o_list[c_ptr->o_idx];
	if (inarea(&o_ptr->wpos, wpos) &&
	    (o_ptr->held_m_idx || (o_ptr->ix == x && o_ptr->iy == y))) return(TRUE);

	s_printf("NOTHING_TEST2: o_idx %d, iwpos %d,%d,%d, ix,iy %d,%d. wpos %d,%d,%d x,y %d,%d (%d)\n",
	    c_ptr->o_idx, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz, o_ptr->ix, o_ptr->iy,
	    wpos->wx, wpos->wy, wpos->wz, x, y,
	    marker);
	return(FALSE);
}

/*
 * Determine if the player "hits" a monster (normal combat).
 * Also used for PvP - for now not doing any adjustments in case TO_AC_CAP_30 is enabled, unlike check_hit().
 * Note -- Always miss 5%, always hit 5%, otherwise random.
 */
bool test_hit_fire(int chance, int ac, int vis) {
	int k;

	/* Percentile dice */
	k = rand_int(100);
	/* Hack -- Instant miss or hit */
	if (k < 10) return(k < 5);

	/* Never hit */
	if (chance <= 0) return(FALSE);
	/* Invisible monsters are harder to hit */
	if (!vis) chance = (chance + 1) / 2;
	/* Power competes against armor */
	if (rand_int(chance) < (ac * 3 / 4)) return(FALSE);

	/* Assume hit */
	return(TRUE);
}



/*
 * Determine if the player "hits" a monster (normal combat).
 * Also used for PvP - for now not doing any adjustments in case TO_AC_CAP_30 is enabled, unlike check_hit().
 * Note -- Always miss 5%, always hit 5%, otherwise random.
 */
bool test_hit_melee(int chance, int ac, int vis) {
	int k;

	/* Percentile dice */
	k = rand_int(100);
	/* Hack -- Instant miss or hit */
	if (k < 10) return(k < 5);

	/* Wimpy attack never hits */
	if (chance <= 0) return(FALSE);
	/* Penalize invisible targets */
	if (!vis) chance = (chance + 1) / 2;
	/* Power must defeat armor */
	if (rand_int(chance) < (ac * 3 / 4)) return(FALSE);

	/* Assume hit */
	return(TRUE);
}

/*
 * Critical hits (from objects thrown by player)
 * Factor in item weight, total plusses, and player level.
 * shot: TRUE for actual shooting, FALSE for throwing (boomerangs and items from inventory): It applies 'Archery' skill.
 */
s16b critical_shot(int Ind, int weight, int plus, int dam, int o_crit, bool precision, bool shot) {
	player_type *p_ptr = NULL;
	int i, k;
	//int xtra_crit = p_ptr->xtra_crit + p_ptr->inventory;
	//if xtra_crit > 50 cap
	//xtra_crit = 65 - (975 / (xtra_crit + 15));

	/* Extract "shot" power */
	if (Ind > 0) {
		p_ptr = Players[Ind];
		i = weight + (p_ptr->to_h + p_ptr->to_h_ranged + plus) * 5 + (shot ? get_skill_scale(p_ptr, SKILL_ARCHERY, 150) : 0);
		i += 50 * BOOST_CRIT(p_ptr->xtra_crit + o_crit); //0..2350; 10->1010, 20->1650, 35->2100, 50->2350
	}
	else i = weight + plus * 5;

	/* Critical hit */
	if (precision || randint(3500) <= i) {
		k = weight + randint(700);
		if (Ind > 0) k += (shot ? get_skill_scale(p_ptr, SKILL_ARCHERY, 100) : 0) + randint(600 - (12000 / (BOOST_CRIT(p_ptr->xtra_crit + o_crit) + 20)));

		if (precision) {
			if (k < 650) k = 650;
			k += rand_int(500);
		}

		if (k < 350) {
			if (Ind > 0) msg_print(Ind, "It was a good hit!");
			dam = (4 * dam) / 3 + 5;
		} else if (k < 650) {
			if (Ind > 0) msg_print(Ind, "It was a great hit!");
			dam = (5 * dam) / 3 + 10;
		} else if (k < 900) {
			if (Ind > 0) msg_print(Ind, "It was a superb hit!");
			dam = (6 * dam) / 3 + 10;
		} else if (k < 1100) {
			if (Ind > 0) msg_print(Ind, "It was a *GREAT* hit!");
			dam = (7 * dam) / 3 + 10;
		} else {
			if (Ind > 0) msg_print(Ind, "It was a *SUPERB* hit!");
			dam = (8 * dam) / 3 + 15;
		}
	}

	return(dam);
}



/*
 * Critical hits (by player)
 *
 * Factor in weapon weight, total plusses, player level.
 */
s16b critical_throw(int Ind, int weight, int plus, int dam, int o_crit) {
	player_type *p_ptr;
	int i, k, w;

	if (Ind > 0) {
		p_ptr = Players[Ind];

#if 1 /* Like for melee weapons, heavier weapons make it harder to land a crit? */
		/* Extract critical maneuver potential (interesting term..) */
		/* The larger the weapon the more difficult to quickly strike
		the critical spot. Cap weight influence at 100+ lb */
		w = weight;
		if (w > 100) w = 10;
		else w = 110 - w;
		if (w < 10) w = 10; /* shouldn't happen anyways */
		i = w * 2 + (p_ptr->to_h + p_ptr->to_h_thrown + plus) * 5 + get_skill_scale(p_ptr, SKILL_MASTERY, 150); /* We use weapon-mastery skill, which is usually melee, for throwing */
#else /* Like for firing ranged waepons, heavier shots make it easier to land a crit? */
		i = weight + (p_ptr->to_h + p_ptr->to_h_thrown + plus) * 5 + get_skill_scale(p_ptr, SKILL_MASTERY, 150); /* We use weapon-mastery skill, which is usually melee, for throwing */
#endif
		i += 50 * BOOST_CRIT(p_ptr->xtra_crit + o_crit); //0..2350; 10->1010, 20->1650, 35->2100, 50->2350
	}
	else i = weight;

	/* Critical hit */
	if (randint(3500) <= i) {
		/* _If_ a critical hit is scored then it will deal more damage if the weapon is heavier */
		k = weight + randint(700) + 500 - (10000 / (BOOST_CRIT(p_ptr->xtra_crit + o_crit) + 20));
		if (Ind > 0) k += get_skill_scale(p_ptr, SKILL_MASTERY, 100) + randint(600 - (12000 / (BOOST_CRIT(p_ptr->xtra_crit + o_crit) + 20)));

		if (k < 350) {
			if (Ind > 0) msg_print(Ind, "It was a good hit!");
			dam = (4 * dam) / 3 + 5;
		} else if (k < 650) {
			if (Ind > 0) msg_print(Ind, "It was a great hit!");
			dam = (5 * dam) / 3 + 10;
		} else if (k < 900) {
			if (Ind > 0) msg_print(Ind, "It was a superb hit!");
			dam = (6 * dam) / 3 + 10;
		} else if (k < 1100) {
			if (Ind > 0) msg_print(Ind, "It was a *GREAT* hit!");
			dam = (7 * dam) / 3 + 10;
		} else {
			if (Ind > 0) msg_print(Ind, "It was a *SUPERB* hit!");
			dam = (8 * dam) / 3 + 15;
		}
	}

	return(dam);
}


/*
 * Critical hits (by player)
 *
 * Factor in weapon weight, total plusses, player level.
 * Also called for martial arts, then MA skill will simulate a 'weight'.
 */
s16b critical_melee(int Ind, int weight, int plus, int dam, int o_crit, bool allow_skill_crit, bool weapon) {
	player_type *p_ptr = Players[Ind];
	int i, k, w;

	/* Critical hits for rogues says 'with light swords'
	in the skill description, which is logical since you
	cannot maneuver a heavy weapon so that it pierces through
	the small weak spot of an opponent's armour, hitting exactly
	into his artery or sth. */

	/* Extract critical maneuver potential (interesting term..) */
	/* The larger the weapon the more difficult to quickly strike
	the critical spot. Cap weight influence at 10.0 lb */
	w = weight;
	if (w > 100) w = 10;
	else w = 110 - w;
	if (w < 10) w = 10; /* shouldn't happen anyways */
	i = w * 2 + (p_ptr->to_h + p_ptr->to_h_melee + plus) * 5 + (weapon ? get_skill_scale(p_ptr, SKILL_MASTERY, 150) : 0);

	i += 50 * BOOST_CRIT(p_ptr->xtra_crit + o_crit); //0..2350; 10->1010, 20->1650, 35->2100, 50->2350
	if (allow_skill_crit) i += get_skill_scale(p_ptr, SKILL_CRITS, 40 * 50);

	/* Chance */
	if (p_ptr->melee_timeout_crit_dual || randint(5000) <= i) {
		/* _If_ a critical hit is scored then it will deal
		more damage if the weapon is heavier */
		k = weight + randint(700) + 500 - (10000 / (BOOST_CRIT(p_ptr->xtra_crit + o_crit) + 20));
		if (allow_skill_crit) k += randint(get_skill_scale(p_ptr, SKILL_CRITS, 900));

		if (k < 400) {
			msg_print(Ind, "It was a good hit!");
			dam = ((4 * dam) / 3) + 5;
		} else if (k < 700) {
			msg_print(Ind, "It was a great hit!");
			dam = ((5 * dam) / 3) + 10;
		} else if (k < 900) {
			msg_print(Ind, "It was a superb hit!");
			dam = ((6 * dam) / 3) + 15;
		} else if (k < 1300) {
			msg_print(Ind, "It was a *GREAT* hit!");
			dam = ((7 * dam) / 3) + 20;
		} else {
			msg_print(Ind, "It was a *SUPERB* hit!");
			dam = ((8 * dam) / 3) + 25;
		}
	}

	return(dam);
}



/*
 * Brands (including slay mods) the given damage depending on object type, hitting a given monster.
 *
 * Note that most brands and slays are x3, except Slay Animal (x2),
 * Slay Evil (x2), and Kill dragon (x5).
 */
/* accepts Ind <=0.
   slot is only important for melee weapons, for temporary brands. */
s16b brand_dam_aux(int Ind, object_type *o_ptr, int slot, int tdam, monster_type *m_ptr, bool thrown) {
	int mult = FACTOR_MULT, bonus = 0;
	monster_race *r_ptr = race_inf(m_ptr);
	u32b f1, f2, f3, f4, f5, f6, esp;
	u16b fx = 0x0;
	player_type *p_ptr = NULL;

	struct worldpos *wpos = &m_ptr->wpos;
	cave_type **zcave;
	cave_type *c_ptr;
	char m_name[MNAME_LEN];

	object_type *e_ptr;
	u32b ef1, ef2, ef3, ef4, ef5, ef6, eesp;

	monster_race *pr_ptr = NULL;
	bool apply_monster_brands = TRUE;
	int i, monster_brands = 0;
	u32b monster_brand[6], monster_brand_chosen;

	bool melee = !o_ptr || (!is_ammo(o_ptr->tval) && o_ptr->tval != TV_BOOMERANG);


	monster_brand[1] = 0;
	monster_brand[2] = 0;
	monster_brand[3] = 0;
	monster_brand[4] = 0;
	monster_brand[5] = 0;

	if (Ind > 0) {
		p_ptr = Players[Ind];
		pr_ptr = &r_info[p_ptr->body_monster];
	}

	if (!(zcave = getcave(wpos))) return(tdam);
	c_ptr = &zcave[m_ptr->fy][m_ptr->fx];

	/* Extract monster name (or "it") */
	monster_desc(Ind, m_name, c_ptr->m_idx, 0);

	/* Extract the flags */
	if (o_ptr && o_ptr->k_idx) {
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		/* Hack -- extract temp branding */
		if (p_ptr && p_ptr->ammo_brand) {
			switch (p_ptr->ammo_brand_t) {
			case TBRAND_ELEC:
				f1 |= TR1_BRAND_ELEC;
				break;
			case TBRAND_COLD:
				f1 |= TR1_BRAND_COLD;
				break;
			case TBRAND_FIRE:
				f1 |= TR1_BRAND_FIRE;
				break;
			case TBRAND_ACID:
				f1 |= TR1_BRAND_ACID;
				break;
			case TBRAND_POIS:
				f1 |= TR1_BRAND_POIS;
				break;
			case TBRAND_BASE:
				f1 |= (TR1_BRAND_FIRE | TR1_BRAND_COLD | TR1_BRAND_ELEC | TR1_BRAND_ACID);
				break;
			/* flags that exist in temporary form only */
			case TBRAND_HELLFIRE:
				fx = TBRAND_HELLFIRE;
				break;
			case TBRAND_VAMPIRIC:
				fx = TBRAND_VAMPIRIC;
				break;
			}
		}

	} else {
		f1 = 0; f2 = 0; f3 = 0; f4 = 0; f5 = 0;
	}


	/* Apply brands from mimic monster forms */
	if (p_ptr && p_ptr->body_monster) {
#if 0
		switch (pr_ptr->r_ptr->d_char) {
			/* If monster is fighting with a weapon, the player gets the brand(s) even with a weapon */
			case 'p':	case 'h':	case 't':
			case 'o':	case 'y':	case 'k':
			apply_monster_brands = TRUE;
			break;
			/* If monster is fighting without weapons, the player gets the brand(s) only if
			he fights with bare hands/martial arts */
			default:
			if (!o_ptr || !o_ptr->k_idx) apply_monster_brands = TRUE;
			break;
		}
		/* change a.m.b.=TRUE to =FALSE at declaration above if u use this if0-part again */
#endif
#if 0
		/* If monster is using range weapons, the player gets the brand(s) even on range attacks */
		if ((!pr_ptr->flags4 & RF4_ARROW_1) && o_ptr && is_ammo(o_ptr->tval))
			apply_monster_brands = FALSE;
		/* If monster is fighting with a weapon, the player gets the brand(s) even with a weapon */
		/* If monster is fighting without weapons, the player gets the brand(s) only if
		he fights with bare hands/martial arts */
		/* However, if the monster doesn't use weapons but nevertheless fires ammo, the player
		gets the brand(s) on ranged attacks */
		if ((!pr_ptr->body_parts[BODY_WEAPON]) &&
		    o_ptr && o_ptr->k_idx && is_melee_weapon(o_ptr->tval))
			apply_monster_brands = FALSE;
#endif
		/* The player never gets brands on ranged attacks from a form */
		if (!melee)
			apply_monster_brands = FALSE;
		/* The player doesn't get brands if he uses a weapon but the monster doesn't */
		if (o_ptr && o_ptr->k_idx && (!pr_ptr->body_parts[BODY_WEAPON]))
			apply_monster_brands = FALSE;

		/* Get monster brands. If monster has several, choose one randomly */
		for (i = 0; i < 4; i++) {
			if (pr_ptr->blow[i].d_dice && pr_ptr->blow[i].d_side) {
				switch (pr_ptr->blow[i].effect) {
				case RBE_ACID:
					monster_brands++;
					monster_brand[monster_brands] = TR1_BRAND_ACID;
					break;
				case RBE_ELEC:
					monster_brands++;
					monster_brand[monster_brands] = TR1_BRAND_ELEC;
					break;
				case RBE_FIRE:
					monster_brands++;
					monster_brand[monster_brands] = TR1_BRAND_FIRE;
					break;
				case RBE_COLD:
					monster_brands++;
					monster_brand[monster_brands] = TR1_BRAND_COLD;
					break;
				case RBE_POISON:	case RBE_DISEASE:
					monster_brands++;
					monster_brand[monster_brands] = TR1_BRAND_POIS;
					break;
				default:
					monster_brands++;
					monster_brand[monster_brands] = 0;
					break;
				}
			}
		}
		/* Choose random brand from the ones available */
		monster_brand_chosen = monster_brand[1 + rand_int(monster_brands)];

		/* Modify damage */
		if (apply_monster_brands) f1 |= monster_brand_chosen;
	}

	/* Add bow branding on correct ammo types */
	if (p_ptr && o_ptr) {
		e_ptr = &p_ptr->inventory[INVEN_BOW];
		if (e_ptr->k_idx && e_ptr->tval == TV_BOW) {
			/* Extract the item flags */
			object_flags(e_ptr, &ef1, &ef2, &ef3, &ef4, &ef5, &ef6, &eesp);

			if (((e_ptr->sval == SV_SHORT_BOW || e_ptr->sval == SV_LONG_BOW) && o_ptr->tval == TV_ARROW) ||
			   ((e_ptr->sval == SV_LIGHT_XBOW || e_ptr->sval == SV_HEAVY_XBOW) && o_ptr->tval == TV_BOLT) ||
			   (e_ptr->sval == SV_SLING && o_ptr->tval == TV_SHOT))
				f1 |= ef1;
		}
	}

	/* Draconian traits, skills, enlighted slay.. */
	if (p_ptr) f1 |= p_ptr->slay | p_ptr->slay_equip;

	/* Extra melee branding */
	if (p_ptr && melee) {
		/* holy curing only so far? */
		f1 |= p_ptr->slay_melee;

		/* Temporary weapon branding */
		if ((!p_ptr->melee_brand_ma && o_ptr && o_ptr->k_idx &&
		    ((p_ptr->melee_brand && slot == INVEN_WIELD) || (p_ptr->melee_brand2 && slot == INVEN_ARM))) || /* we have a weapon brand? */
		    (p_ptr->melee_brand_ma && p_ptr->melee_brand && (!o_ptr || !o_ptr->k_idx))) { /* we have a barehanded brand? */
			switch (slot == INVEN_ARM ? p_ptr->melee_brand2_t : p_ptr->melee_brand_t) { /* Default for MA branding is also melee_brand_t! */
			case TBRAND_ELEC:
				f1 |= TR1_BRAND_ELEC;
				break;
			case TBRAND_COLD:
				f1 |= TR1_BRAND_COLD;
				break;
			case TBRAND_FIRE:
				f1 |= TR1_BRAND_FIRE;
				break;
			case TBRAND_ACID:
				f1 |= TR1_BRAND_ACID;
				break;
			case TBRAND_POIS:
				f1 |= TR1_BRAND_POIS;
				break;
			case TBRAND_BASE:
				f1 |= (TR1_BRAND_FIRE | TR1_BRAND_COLD | TR1_BRAND_ELEC | TR1_BRAND_ACID);
				break;
			case TBRAND_CHAO:
				f5 |= TR5_CHAOTIC;
				break;
			/* flags that exist in temporary form only */
			case TBRAND_HELLFIRE:
				fx |= TBRAND_HELLFIRE;
				break;
			case TBRAND_VAMPIRIC:
				fx |= TBRAND_VAMPIRIC;
				break;
			}
		}
	}

	/* Check static aura melee brands */
	if (melee) {
		/* not "temporary", and not coming "from" or being "on" the actual weapon. Available and "directly transmitted" via aura of death */
		if (p_ptr->aura[AURA_SHIVER] && get_skill(p_ptr, SKILL_AURA_SHIVER) >= 30) f1 |= TR1_BRAND_COLD;
		if (p_ptr->aura[AURA_DEATH] && get_skill(p_ptr, SKILL_AURA_DEATH) >= 40) fx |= (TBRAND_PLASMA | TBRAND_ICE);
	}


	/* Avoid contradictionary brands,
	   let one of them randomly 'flicker up' on striking, suppressing the other */
	if ((f1 & TR1_BRAND_COLD) && (fx & TBRAND_HELLFIRE))
		f1 &= ~TR1_BRAND_COLD; //hellfire gets priority over basic cold

	i = 0;
	if (f1 & TR1_BRAND_FIRE) i++;
	if (f1 & TR1_BRAND_COLD) i--;
	if (fx & TBRAND_HELLFIRE) i++;
	if (fx & TBRAND_PLASMA) i++;
	if (fx & TBRAND_ICE) i--;

	/* Note: Having fire+hellfire+plasma will actually cancel out a lone cold or ice brand completely. */
	if (((f1 & TR1_BRAND_FIRE) || (fx & (TBRAND_PLASMA | TBRAND_HELLFIRE)))
	    && ((fx & TBRAND_ICE) || (f1 & TR1_BRAND_COLD))) {
		if (magik(50 - i * 25)) {
			f1 &= ~TR1_BRAND_FIRE;
			fx &= ~TBRAND_HELLFIRE;
			fx &= ~TBRAND_PLASMA;
		} else {
			f1 &= ~TR1_BRAND_COLD;
			fx &= ~TBRAND_ICE;
		}
	}


	/* Apply all slays and brands */
	/* Slay Animal */
	if ((f1 & TR1_SLAY_ANIMAL) &&
	    (r_ptr->flags3 & RF3_ANIMAL)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_ANIMAL;*/

		if (mult < FACTOR_HURT) mult = FACTOR_HURT;
		if (bonus < FLAT_HURT_BONUS) bonus = FLAT_HURT_BONUS;
	}

	/* Slay Evil */
	if ((f1 & TR1_SLAY_EVIL) && (r_ptr->flags3 & RF3_EVIL)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_EVIL;*/
		if (mult < FACTOR_HURT) mult = FACTOR_HURT;
		if (bonus < FLAT_HURT_BONUS) bonus = FLAT_HURT_BONUS;
	}

	/* Slay Undead */
	if ((f1 & TR1_SLAY_UNDEAD) && (r_ptr->flags3 & RF3_UNDEAD)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_UNDEAD;*/
		if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
		if (bonus < FLAT_SLAY_BONUS) bonus = FLAT_SLAY_BONUS;
	}

	/* Slay Demon */
	if ((f1 & TR1_SLAY_DEMON) && (r_ptr->flags3 & RF3_DEMON)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DEMON;*/
		if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
		if (bonus < FLAT_SLAY_BONUS) bonus = FLAT_SLAY_BONUS;
	}

	/* Slay Orc */
	if ((f1 & TR1_SLAY_ORC) &&
	    (r_ptr->flags3 & RF3_ORC)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_ORC;*/
		if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
		if (bonus < FLAT_SLAY_BONUS) bonus = FLAT_SLAY_BONUS;
	}

	/* Slay Troll */
	if ((f1 & TR1_SLAY_TROLL) &&
	    (r_ptr->flags3 & RF3_TROLL)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_TROLL;*/
		if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
		if (bonus < FLAT_SLAY_BONUS) bonus = FLAT_SLAY_BONUS;
	}

	/* Slay Giant */
	if ((f1 & TR1_SLAY_GIANT) &&
	    (r_ptr->flags3 & RF3_GIANT)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_GIANT;*/
		if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
		if (bonus < FLAT_SLAY_BONUS) bonus = FLAT_SLAY_BONUS;
	}

	/* Slay Dragon  */
	if ((f1 & TR1_SLAY_DRAGON) &&
	    (r_ptr->flags3 & (RF3_DRAGON | RF3_DRAGONRIDER))) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DRAGON;*/
		if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
		if (bonus < FLAT_SLAY_BONUS) bonus = FLAT_SLAY_BONUS;
	}

	/* Execute Dragon */
	if ((f1 & TR1_KILL_DRAGON) &&
	    (r_ptr->flags3 & (RF3_DRAGON | RF3_DRAGONRIDER))) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DRAGON;*/
		if (mult < FACTOR_KILL) mult = FACTOR_KILL;
		if (bonus < FLAT_KILL_BONUS) bonus = FLAT_KILL_BONUS;
	}

	/* Execute Undead */
	if ((f1 & TR1_KILL_UNDEAD) &&
	    (r_ptr->flags3 & RF3_UNDEAD)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_UNDEAD;*/
		if (mult < FACTOR_KILL) mult = FACTOR_KILL;
		if (bonus < FLAT_KILL_BONUS) bonus = FLAT_KILL_BONUS;
	}

	/* Execute Undead */
	if ((f1 & TR1_KILL_DEMON) &&
	    (r_ptr->flags3 & RF3_DEMON)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DEMON;*/
		if (mult < FACTOR_KILL) mult = FACTOR_KILL;
		if (bonus < FLAT_KILL_BONUS) bonus = FLAT_KILL_BONUS;
	}


	/* Brand (Acid) */
	if (f1 & TR1_BRAND_ACID) {
		/* Notice immunity */
		if (r_ptr->flags3 & RF3_IM_ACID) {
			/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_ACID;*/
		}
		/* Notice susceptibility */
		else if (r_ptr->flags9 & (RF9_SUSCEP_ACID)) {
#if 0
			if (m_ptr->ml) r_ptr->r_flags9 |= (RF9_SUSCEP_ACID);
#endif
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else if (r_ptr->flags9 & RF9_RES_ACID) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
		}
		/* Otherwise, take the damage */
		else {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		}
	}

	/* Brand (Elec) */
	if (f1 & TR1_BRAND_ELEC) {
		/* Notice immunity */
		if (r_ptr->flags3 & RF3_IM_ELEC) {
			/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_ELEC;*/
		}
		/* Notice susceptibility */
		else if (r_ptr->flags9 & (RF9_SUSCEP_ELEC)) {
#if 0
			if (m_ptr->ml) r_ptr->r_flags9 |= (RF9_SUSCEP_ELEC);
#endif
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else if (r_ptr->flags9 & RF9_RES_ELEC) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
		}
		/* Otherwise, take the damage */
		else {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		}
	}

	/* Brand (Fire) */
	if (f1 & TR1_BRAND_FIRE) {
		/* Notice immunity */
		if (r_ptr->flags3 & RF3_IM_FIRE) {
			/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_FIRE;*/
		}
		/* Notice susceptibility */
		else if (r_ptr->flags3 & (RF3_SUSCEP_FIRE)) {
#if 0
			if (m_ptr->ml) r_ptr->r_flags3 |= (RF3_SUSCEP_FIRE);
#endif
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else if (r_ptr->flags9 & RF9_RES_FIRE) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
		}
		/* Otherwise, take the damage */
		else {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		}
	}

	/* Brand (Cold) */
	if (f1 & TR1_BRAND_COLD) {
		/* Notice immunity */
		if (r_ptr->flags3 & RF3_IM_COLD) {
			/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_COLD;*/
		}
		/* Notice susceptibility */
		else if (r_ptr->flags3 & (RF3_SUSCEP_COLD)) {
#if 0
			if (m_ptr->ml) r_ptr->r_flags3 |= (RF3_SUSCEP_COLD);
#endif
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		}
		else if (r_ptr->flags9 & RF9_RES_COLD) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
		}
		/* Otherwise, take the damage */
		else {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		}
	}


	/* Brand (Pois) */
	if (f1 & TR1_BRAND_POIS) {
		/* Notice immunity */
		if (r_ptr->flags3 & RF3_IM_POIS) {
			/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_POIS;*/
		}
		/* Notice susceptibility */
		else if (r_ptr->flags9 & (RF9_SUSCEP_POIS)) {
#if 0
			if (m_ptr->ml) r_ptr->r_flags9 |= (RF9_SUSCEP_POIS);
#endif
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			//if (magik(95)) *special |= SPEC_POIS;
			if (bonus < FLAT_BRAND_BONUS_POISON) bonus = FLAT_BRAND_BONUS_POISON;
		} else if (r_ptr->flags9 & RF9_RES_POIS) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
		}
		/* Otherwise, take the damage */
		else {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS_POISON) bonus = FLAT_BRAND_BONUS_POISON;
		}
	}

	/* Brand (Hellfire) */
	if (fx & TBRAND_HELLFIRE) {
		if (r_ptr->flags3 & RF3_GOOD) {
			if (r_ptr->flags3 & RF3_IM_FIRE) {
				if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
				if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
			} else if (r_ptr->flags9 & RF9_RES_FIRE) {
				if (mult < FACTOR_BRAND_STRONG) mult = FACTOR_BRAND_STRONG;
				if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
			} else {
				if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
				if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
			}
		} else {
			if (r_ptr->flags3 & RF3_IM_FIRE) {
				if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				if (bonus < FLAT_MIN_BONUS) bonus = FLAT_MIN_BONUS;
			} else if (r_ptr->flags3 & RF3_SUSCEP_FIRE) {
				if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
				if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
			} else if ((r_ptr->flags9 & RF9_RES_FIRE) && (r_ptr->flags3 & RF3_DEMON)) {
				if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				if (bonus < FLAT_MIN_BONUS) bonus = FLAT_MIN_BONUS;
			} else if ((r_ptr->flags9 & RF9_RES_FIRE) || (r_ptr->flags3 & RF3_DEMON)) {
				if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
				if (bonus < FLAT_HALF_BONUS) bonus = FLAT_HALF_BONUS;
			} else {
				if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
				if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
			}
		}
	}
	if (fx & TBRAND_PLASMA) { /* 50% fire + 25% elec + 25% force */
		i = 12;
		if (r_ptr->flags3 & RF3_IM_FIRE) {
			i -= 6;
			/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_FIRE;*/
		}
		/* Notice susceptibility */
		else if (r_ptr->flags3 & (RF3_SUSCEP_FIRE)) {
#if 0
			if (m_ptr->ml) r_ptr->r_flags3 |= (RF3_SUSCEP_FIRE);
#endif
		} else if (r_ptr->flags9 & RF9_RES_FIRE) i -= 4;
		else i -= 2;

		/* Notice immunity */
		if (r_ptr->flags3 & RF3_IM_ELEC) {
			i -= 3;
			/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_ELEC;*/
		}
		/* Notice susceptibility */
		else if (r_ptr->flags9 & (RF9_SUSCEP_ELEC)) {
#if 0
			if (m_ptr->ml) r_ptr->r_flags9 |= (RF9_SUSCEP_ELEC);
#endif
		} else if (r_ptr->flags9 & RF9_RES_ELEC) i -= 2;
		else i--;

		if ((r_ptr->flags4 & RF4_BR_SOUN) || (r_ptr->flags9 & RF9_RES_SOUND)) i -= 2;
		//else if (!(r_ptr->flags3 & RF3_NO_STUN)) do_stun = randint(15) / div;
		else if (r_ptr->flags3 & RF3_NO_STUN) i--;

		if (i >= 10) {
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else if (i >= 8) {
			if (mult < FACTOR_BRAND_STRONG) mult = FACTOR_BRAND_STRONG;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else if (i >= 6) {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else if (i >= 4) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
			if (bonus < FLAT_HALF_BONUS) bonus = FLAT_HALF_BONUS;
		} else if (i) {
			if (bonus < FLAT_MIN_BONUS) bonus = FLAT_MIN_BONUS;
		}
		//else: impossible to reach as there is no sound-immunity
	}
	if (fx & TBRAND_ICE) { //40% cold + 60% shards, but should just do 1:1 for simplification
		i = 15;

		/* Notice immunity */
		if (r_ptr->flags3 & RF3_IM_COLD) {
			i -= 6;
			/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_COLD;*/
		}
		/* Notice susceptibility */
		else if (r_ptr->flags3 & (RF3_SUSCEP_COLD)) {
#if 0
			if (m_ptr->ml) r_ptr->r_flags3 |= (RF3_SUSCEP_COLD);
#endif
		}
		else if (r_ptr->flags9 & RF9_RES_COLD) i -= 4;
		else i -= 2;

		if ((r_ptr->flags4 & RF4_BR_SHAR) || (r_ptr->flags9 & RF9_RES_SHARDS)) i -= 6;
		else if (r_ptr->flags8 & RF8_NO_CUT) i -= 4;

		if (i >= 11) {
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else if (i >= 9) {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else if (i >= 5) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
			if (bonus < FLAT_HALF_BONUS) bonus = FLAT_HALF_BONUS;
		} else if (i >= 3) {
			if (bonus < FLAT_MIN_BONUS) bonus = FLAT_MIN_BONUS;
		}
		//else: impossible to reach as there is no shard-immunity
	}

#if 0
#ifdef TEST_SERVER
	msg_format(Ind, "bonus %d, tdam %d, mult %d, FAC-MUL %d, thr: %d, ammo: %d, MA: %d, weap: %d", bonus, tdam, mult, FACTOR_MULT,
	    (tdam * (((mult - FACTOR_MULT) * 10L) / 4 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT),
	    (tdam * (((mult - FACTOR_MULT) * 20L) / 5 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT),
	    (tdam * (((mult - FACTOR_MULT) * 10L) / 3 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT),
	    (tdam * mult) / FACTOR_MULT);
	if (thrown) msg_print(Ind, "thrown");
	else if (o_ptr && is_ammo(o_ptr->tval)) msg_print(Ind, "shot");
	else if (!o_ptr || !o_ptr->k_idx) msg_print(Ind, "ma/bare");
	else msg_print(Ind, "weapon");
#endif
#endif

	/* If the object was thrown, reduce brand effect by 75%
	   to avoid insane damage. */
	if (thrown)
		//return((tdam * (((mult - FACTOR_MULT) * 10L) / 3 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT));// no 'bonus'
		return(bonus / 2 + (tdam * (((mult - FACTOR_MULT) * 10L) / 3 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT));// hm, give bonus, see ammo comment below

	/* Ranged weapons (except for boomerangs) get less benefit from brands */
	if (o_ptr && is_ammo(o_ptr->tval))
		//return((tdam * (((mult - FACTOR_MULT) * 20L) / 5 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT));// no 'bonus'
		return(bonus / 2 + (tdam * (((mult - FACTOR_MULT) * 20L) / 5 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT));// hm, rather give bonus, or some slays with 1d4 ammo will just not show ANY increase :/
		//return((tdam * mult) / FACTOR_MULT);

	/* Martial Arts styles (and bare-handed) get less benefit from brands */
	if (!o_ptr || !o_ptr->k_idx)
		return((bonus * 2) / 3 + ((tdam * (((mult - FACTOR_MULT) * 10L) / 2 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT)));

	/* Return the total damage */
	return(bonus + ((tdam * mult) / FACTOR_MULT));
}

/*
 * Brands (including slay mods) the given damage depending on object type, hitting a given player.
 * slot is only important for melee weapons, for temporary brands.
 */
s16b brand_dam_aux_player(int Ind, object_type *o_ptr, int slot, int tdam, player_type *q_ptr, bool thrown) {
	int mult = FACTOR_MULT, bonus = 0;
	u32b f1, f2, f3, f4, f5, f6, esp;
	u16b fx = 0x0;
	player_type *p_ptr = NULL;
	object_type *e_ptr;
	u32b ef1, ef2, ef3, ef4, ef5, ef6, eesp;

	monster_race *pr_ptr = NULL;
	bool apply_monster_brands = TRUE;
	int i, monster_brands = 0;
	u32b monster_brand[6], monster_brand_chosen;

	bool melee = !o_ptr || (!is_ammo(o_ptr->tval) && o_ptr->tval != TV_BOOMERANG);
	u32b q_flags3 = 0x0;


	monster_brand[1] = 0;
	monster_brand[2] = 0;
	monster_brand[3] = 0;
	monster_brand[4] = 0;
	monster_brand[5] = 0;

	if (Ind > 0) {
		p_ptr = Players[Ind];
		pr_ptr = &r_info[p_ptr->body_monster];
	}

	/* Extract the flags */
	if (o_ptr && o_ptr->k_idx)
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
	else {
		f1 = 0; f2 = 0; f3 = 0; f4 = 0; f5 = 0;
	}

	/* Apply brands from mimic monster forms */
	if (p_ptr->body_monster) {
#if 0
		switch (pr_ptr->r_ptr->d_char) {
			/* If monster is fighting with a weapon, the player gets the brand(s) even with a weapon */
			case 'p':	case 'h':	case 't':
			case 'o':	case 'y':	case 'k':
			apply_monster_brands = TRUE;
			break;
			/* If monster is fighting without weapons, the player gets the brand(s) only if
			he fights with bare hands/martial arts */
			default:
			if (!o_ptr || !o_ptr->k_idx) apply_monster_brands = TRUE;
			break;
		}
		/* change a.m.b.=TRUE to =FALSE at declaration above if u use this if0-part again */
#endif
#if 0
		/* If monster is using range weapons, the player gets the brand(s) even on range attacks */
		if ((!pr_ptr->flags4 & RF4_ARROW_1) && o_ptr && is_ammo(o_ptr->tval))
			apply_monster_brands = FALSE;
		/* If monster is fighting with a weapon, the player gets the brand(s) even with a weapon */
		/* If monster is fighting without weapons, the player gets the brand(s) only if
		he fights with bare hands/martial arts */
		/* However, if the monster doesn't use weapons but nevertheless fires ammo, the player
		gets the brand(s) on ranged attacks */
		if ((!pr_ptr->body_parts[BODY_WEAPON]) &&
		    o_ptr && is_melee_weapon(o_ptr->tval))
			apply_monster_brands = FALSE;
#endif
		/* The player never gets brands on ranged attacks from a form */
		if (!melee)
			apply_monster_brands = FALSE;
		/* The player doesn't get brands if he uses a weapon but the monster doesn't */
		if (o_ptr && o_ptr->k_idx && (!pr_ptr->body_parts[BODY_WEAPON]))
			apply_monster_brands = FALSE;

		/* Get monster brands. If monster has several, choose one randomly */
		for (i = 0; i < 4; i++) {
			if (pr_ptr->blow[i].d_dice && pr_ptr->blow[i].d_side) {
				switch (pr_ptr->blow[i].effect) {
				case RBE_ACID:
					monster_brands++;
					monster_brand[monster_brands] = TR1_BRAND_ACID;
					break;
				case RBE_ELEC:
					monster_brands++;
					monster_brand[monster_brands] = TR1_BRAND_ELEC;
					break;
				case RBE_FIRE:
					monster_brands++;
					monster_brand[monster_brands] = TR1_BRAND_FIRE;
					break;
				case RBE_COLD:
					monster_brands++;
					monster_brand[monster_brands] = TR1_BRAND_COLD;
					break;
				case RBE_POISON:	// case RBE_DISEASE: vs player DISEASE will poison instead.
					monster_brands++;
					monster_brand[monster_brands] = TR1_BRAND_POIS;
					break;
				default:
					monster_brands++;
					monster_brand[monster_brands] = 0;
					break;
				}
			}
		}
		/* Choose random brand from the ones available */
		monster_brand_chosen = monster_brand[1 + rand_int(monster_brands)];

		/* Modify damage */
		if (apply_monster_brands) f1 |= monster_brand_chosen;
	}

	/* Add bow branding on correct ammo types */
	if (p_ptr && o_ptr) {
		e_ptr = &p_ptr->inventory[INVEN_BOW];
		if (e_ptr->k_idx && e_ptr->tval == TV_BOW) {
			/* Extract the item flags */
			object_flags(e_ptr, &ef1, &ef2, &ef3, &ef4, &ef5, &ef6, &eesp);

			if (((e_ptr->sval == SV_SHORT_BOW || e_ptr->sval == SV_LONG_BOW) && o_ptr->tval == TV_ARROW) ||
			   ((e_ptr->sval == SV_LIGHT_XBOW || e_ptr->sval == SV_HEAVY_XBOW) && o_ptr->tval == TV_BOLT) ||
			   (e_ptr->sval == SV_SLING && o_ptr->tval == TV_SHOT))
				f1 |= ef1;
		}
	}

	/* Draconian traits, skills, enlighted slay.. */
	if (p_ptr) f1 |= p_ptr->slay | p_ptr->slay_equip;

	/* Extra melee branding */
	if (p_ptr && melee) {
		/* holy curing only so far? */
		f1 |= p_ptr->slay_melee;

		/* Temporary weapon branding */
		if ((!p_ptr->melee_brand_ma && o_ptr && o_ptr->k_idx &&
		    ((p_ptr->melee_brand && slot == INVEN_WIELD) || (p_ptr->melee_brand2 && slot == INVEN_ARM))) || /* we have a weapon brand? */
		    (p_ptr->melee_brand_ma && p_ptr->melee_brand && (!o_ptr || !o_ptr->k_idx))) { /* we have a barehanded brand? */
			switch (slot == INVEN_ARM ? p_ptr->melee_brand2_t : p_ptr->melee_brand_t) { /* Default for MA branding is also melee_brand_t! */
			case TBRAND_ELEC:
				f1 |= TR1_BRAND_ELEC;
				break;
			case TBRAND_COLD:
				f1 |= TR1_BRAND_COLD;
				break;
			case TBRAND_FIRE:
				f1 |= TR1_BRAND_FIRE;
				break;
			case TBRAND_ACID:
				f1 |= TR1_BRAND_ACID;
				break;
			case TBRAND_POIS:
				f1 |= TR1_BRAND_POIS;
				break;
			case TBRAND_BASE:
				f1 |= (TR1_BRAND_FIRE | TR1_BRAND_COLD | TR1_BRAND_ELEC | TR1_BRAND_ACID);
				break;
			case TBRAND_CHAO:
				f5 |= TR5_CHAOTIC;
				break;
			/* flags that exist in temporary form only */
			case TBRAND_HELLFIRE:
				fx |= TBRAND_HELLFIRE;
				break;
			case TBRAND_VAMPIRIC:
				fx |= TBRAND_VAMPIRIC;
				break;
			}
		}
	}

	/* Check static aura melee brands */
	if (melee) {
		/* not "temporary", and not coming "from" or being "on" the actual weapon. Available and "directly transmitted" via aura of death */
		if (p_ptr->aura[AURA_SHIVER] && get_skill(p_ptr, SKILL_AURA_SHIVER) >= 30) f1 |= TR1_BRAND_COLD;
		if (p_ptr->aura[AURA_DEATH] && get_skill(p_ptr, SKILL_AURA_DEATH) >= 40) fx |= (TBRAND_PLASMA | TBRAND_ICE);
	}


	/* Avoid contradictionary brands,
	   let one of them randomly 'flicker up' on striking, suppressing the other */
	if ((f1 & TR1_BRAND_COLD) && (fx & TBRAND_HELLFIRE))
		f1 &= ~TR1_BRAND_COLD; //hellfire gets priority over basic cold

	i = 0;
	if (f1 & TR1_BRAND_FIRE) i++;
	if (f1 & TR1_BRAND_COLD) i--;
	if (fx & TBRAND_HELLFIRE) i++;
	if (fx & TBRAND_PLASMA) i++;
	if (fx & TBRAND_ICE) i--;

	/* Note: Having fire+hellfire+plasma will actually cancel out a lone cold or ice brand completely. */
	if (((f1 & TR1_BRAND_FIRE) || (fx & (TBRAND_PLASMA | TBRAND_HELLFIRE)))
	    && ((fx & TBRAND_ICE) || (f1 & TR1_BRAND_COLD))) {
		if (magik(50 - i * 25)) {
			f1 &= ~TR1_BRAND_FIRE;
			fx &= ~TBRAND_HELLFIRE;
			fx &= ~TBRAND_PLASMA;
		} else {
			f1 &= ~TR1_BRAND_COLD;
			fx &= ~TBRAND_ICE;
		}
	}


	/* emulate slay-susceptibilities */
	if (q_ptr->body_monster) q_flags3 |= r_info[q_ptr->body_monster].flags3;
	if (q_ptr->suscep_good) q_flags3 |= RF3_EVIL;
	if (q_ptr->suscep_evil) q_flags3 |= RF3_GOOD; //unused (enlightened maia)
	if (q_ptr->suscep_life) q_flags3 |= RF3_UNDEAD; //covers RACE_VAMPIRE
	if (q_ptr->prace == RACE_DRACONIAN) q_flags3 |= RF3_DRAGON;
	if (q_ptr->ptrait == TRAIT_CORRUPTED) q_flags3 |= RF3_DEMON;
	if (q_ptr->prace == RACE_YEEK) q_flags3 |= RF3_ANIMAL; // D:
	if (q_ptr->prace == RACE_HALF_ORC) q_flags3 |= RF3_ORC;
	if (q_ptr->prace == RACE_HALF_TROLL) q_flags3 |= RF3_TROLL;


	/* Apply all slays/brands */
	/* Slay Animal */
	if ((f1 & TR1_SLAY_ANIMAL) && (q_flags3 & RF3_ANIMAL)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_ANIMAL;*/

		if (mult < FACTOR_HURT) mult = FACTOR_HURT;
		if (bonus < FLAT_HURT_BONUS) bonus = FLAT_HURT_BONUS;
	}

	/* Slay Evil */
	if ((f1 & TR1_SLAY_EVIL) && (q_flags3 & RF3_EVIL)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_EVIL;*/
		if (mult < FACTOR_HURT) mult = FACTOR_HURT;
		if (bonus < FLAT_HURT_BONUS) bonus = FLAT_HURT_BONUS;
	}

	/* Slay Undead */
	if ((f1 & TR1_SLAY_UNDEAD) && (q_flags3 & RF3_UNDEAD)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_UNDEAD;*/
		if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
		if (bonus < FLAT_SLAY_BONUS) bonus = FLAT_SLAY_BONUS;
	}

	/* Slay Demon */
	if ((f1 & TR1_SLAY_DEMON) && (q_flags3 & RF3_DEMON)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DEMON;*/
		if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
		if (bonus < FLAT_SLAY_BONUS) bonus = FLAT_SLAY_BONUS;
	}

	/* Slay Orc */
	if ((f1 & TR1_SLAY_ORC) && (q_flags3 & RF3_ORC)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_ORC;*/
		if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
		if (bonus < FLAT_SLAY_BONUS) bonus = FLAT_SLAY_BONUS;
	}

	/* Slay Troll */
	if ((f1 & TR1_SLAY_TROLL) && (q_flags3 & RF3_TROLL)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_TROLL;*/
		if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
		if (bonus < FLAT_SLAY_BONUS) bonus = FLAT_SLAY_BONUS;
	}

	/* Slay Giant */
	if ((f1 & TR1_SLAY_GIANT) && (q_flags3 & RF3_GIANT)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_GIANT;*/
		if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
		if (bonus < FLAT_SLAY_BONUS) bonus = FLAT_SLAY_BONUS;
	}

	/* Slay Dragon  */
	if ((f1 & TR1_SLAY_DRAGON) && (q_flags3 & RF3_DRAGON)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DRAGON;*/
		if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
		if (bonus < FLAT_SLAY_BONUS) bonus = FLAT_SLAY_BONUS;
	}

	/* Execute Dragon */
	if ((f1 & TR1_KILL_DRAGON) && (q_flags3 & RF3_DRAGON)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DRAGON;*/
		if (mult < FACTOR_KILL) mult = FACTOR_KILL;
		if (bonus < FLAT_KILL_BONUS) bonus = FLAT_KILL_BONUS;
	}

	/* Execute Undead */
	if ((f1 & TR1_KILL_UNDEAD) && (q_flags3 & RF3_UNDEAD)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_UNDEAD;*/
		if (mult < FACTOR_KILL) mult = FACTOR_KILL;
		if (bonus < FLAT_KILL_BONUS) bonus = FLAT_KILL_BONUS;
	}

	/* Execute Undead */
	if ((f1 & TR1_KILL_DEMON) && (q_flags3 & RF3_DEMON)) {
		/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DEMON;*/
		if (mult < FACTOR_KILL) mult = FACTOR_KILL;
		if (bonus < FLAT_KILL_BONUS) bonus = FLAT_KILL_BONUS;
	}


	/* Brand (Acid) */
	if (f1 & TR1_BRAND_ACID) {
		/* Notice immunity */
		if (q_ptr->immune_acid) ;
		else if (q_ptr->resist_acid) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
		}
		/* Otherwise, take the damage */
		else if (q_ptr->suscep_acid) {
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		}
	}

	/* Brand (Elec) */
	if (f1 & TR1_BRAND_ELEC) {
		/* Notice immunity */
		if (q_ptr->immune_elec) ;
		else if (q_ptr->resist_elec) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
		}
		/* Otherwise, take the damage */
		else if (q_ptr->suscep_elec) {
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		}
	}

	/* Brand (Fire) */
	if (f1 & TR1_BRAND_FIRE) {
		/* Notice immunity */
		if (q_ptr->immune_fire) ;
		else if (q_ptr->resist_fire) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
		}
		/* Otherwise, take the damage */
		else if (q_ptr->suscep_fire) {
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		}
	}

	/* Brand (Cold) */
	if (f1 & TR1_BRAND_COLD) {
		/* Notice immunity */
		if (q_ptr->immune_cold) ;
		else if (q_ptr->resist_cold) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
		}
		/* Otherwise, take the damage */
		else if (q_ptr->suscep_cold) {
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		}
	}

	/* Brand (Poison) */
	if (f1 & TR1_BRAND_POIS) {
		/* Notice immunity */
		if (q_ptr->immune_poison) ;
		else if (q_ptr->resist_pois) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
		}
		/* Otherwise, take the damage */
		else if (q_ptr->suscep_pois) {
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			if (bonus < FLAT_BRAND_BONUS_POISON) bonus = FLAT_BRAND_BONUS_POISON;
		} else {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS_POISON) bonus = FLAT_BRAND_BONUS_POISON;
		}
	}

	/* Brand (Hellfire) */
	if (fx & TBRAND_HELLFIRE) {
		if (q_flags3 & RF3_GOOD) {
			if (q_ptr->immune_fire) {
				if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
				if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
			} else if (q_ptr->resist_fire) {
				if (mult < FACTOR_BRAND_STRONG) mult = FACTOR_BRAND_STRONG;
				if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
			} else {
				if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
				if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
			}
		} else {
			if (q_ptr->immune_fire) {
				if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				if (bonus < FLAT_MIN_BONUS) bonus = FLAT_MIN_BONUS;
			} else if (q_ptr->suscep_fire) {
				if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
				if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
			} else if (q_ptr->resist_fire && (q_flags3 & RF3_DEMON)) {
				if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				if (bonus < FLAT_MIN_BONUS) bonus = FLAT_MIN_BONUS;
			} else if (q_ptr->resist_fire || (q_flags3 & RF3_DEMON)) {
				if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
				if (bonus < FLAT_HALF_BONUS) bonus = FLAT_HALF_BONUS;
			} else {
				if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
				if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
			}
		}
	}

	if (fx & TBRAND_PLASMA) { /* 50% fire + 25% elec + 25% force */
		i = 12;
		if (q_ptr->immune_fire) i -= 6;
		else if (q_ptr->suscep_fire) ;
		else if (q_ptr->resist_fire) i -= 4;
		else i -= 2;

		if (q_ptr->immune_elec) i -= 3;
		else if (q_ptr->suscep_elec) ;
		else if (q_ptr->resist_elec) i -= 2;
		else i--;

		if (q_ptr->resist_sound) i -= 2;
		//else do_stun = randint(15) / div;

		if (i >= 10) {
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else if (i >= 8) {
			if (mult < FACTOR_BRAND_STRONG) mult = FACTOR_BRAND_STRONG;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else if (i >= 6) {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else if (i >= 4) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
			if (bonus < FLAT_HURT_BONUS) bonus = FLAT_HALF_BONUS;
		} else if (i) {
			if (bonus < FLAT_HALF_BONUS) bonus = FLAT_MIN_BONUS;
		}
		//else: impossible to reach as there is no sound-immunity
	}
	if (fx & TBRAND_ICE) { //40% cold + 60% shards, but should just do 1:1 for simplification
		i = 15;

		if (q_ptr->immune_cold) i -= 6;
		else if (q_ptr->suscep_cold) ;
		else if (q_ptr->resist_cold) i -= 4;
		else i -= 2;

		if (q_ptr->resist_shard || q_ptr->biofeedback) i -= 6;
		else if (q_ptr->no_cut) i -= 4;

		if (i >= 11) {
			if (mult < FACTOR_BRAND_SUSC) mult = FACTOR_BRAND_SUSC;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else if (i >= 9) {
			if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_BRAND_BONUS;
		} else if (i >= 5) {
			if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
			if (bonus < FLAT_BRAND_BONUS) bonus = FLAT_HALF_BONUS;
		} else if (i >= 3) {
			if (bonus < FLAT_HALF_BONUS) bonus = FLAT_MIN_BONUS;
		}
		//else: impossible to reach as there is no shard-immunity
	}

	/* If the object was thrown, reduce brand effect by 75%
	   to avoid insane damage. */
	if (thrown) return((tdam * (((mult - FACTOR_MULT) * 10L) / 4 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT));// no 'bonus'

	/* Ranged weapons (except for boomerangs) get less benefit from brands */
	if (o_ptr && is_ammo(o_ptr->tval))
		return((tdam * (((mult - FACTOR_MULT) * 20L) / 5 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT));// no 'bonus'

	/* Martial Arts styles get less benefit from brands */
	if (!o_ptr || !o_ptr->k_idx)
		return((bonus * 2) / 3 + ((tdam * (((mult - FACTOR_MULT) * 10L) / 3 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT)));

	/* Return the total damage */
	return(bonus + ((tdam * mult) / FACTOR_MULT));
}

int search_chance(player_type *p_ptr) {
	/* Start with base search ability */
	int chance = p_ptr->skill_srh;

	/* Penalize various conditions */
	if (p_ptr->blind || no_lite(p_ptr->Ind)) chance = chance / 10;
	if (p_ptr->confused || p_ptr->image) chance = chance / 10;

	return(chance);
}
/*
 * Searches for hidden things.                  -RAK-
 */
/* Disallow discovering a secret AND a trap in one search attempt? Problem if enabled: Inconsistencies with detect_bounty(),
   as there are 4 more things (7 total) it searches for, compared to just 3 here [DISABLED]: */
//#define NO_COMBO_FINDINGS /* keep consistent with detect_bounty() ! Keep disabled, because it's not implemented in detect_bounty() atm! */
void search(int Ind) {
	player_type *p_ptr = Players[Ind];
	int y, x, chance;
#ifdef NO_COMBO_FINDINGS
	int findings, finding[3];
#endif
	cave_type *c_ptr;
	object_type *o_ptr;
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	struct c_special *cs_ptr;

	if (!(zcave = getcave(wpos))) return;

	/* Admin doesn't */
	if (p_ptr->admin_dm) return;

	chance = search_chance(p_ptr);

	/* Search the nearby grids, which are always in bounds */

	for (y = (p_ptr->py - 1); y <= (p_ptr->py + 1); y++) {
		for (x = (p_ptr->px - 1); x <= (p_ptr->px + 1); x++) {
			/* Access the grid */
			c_ptr = &zcave[y][x];

			if (c_ptr->custom_lua_search < 0 && exec_lua(0, format("custom_search(%d,%d)", Ind, c_ptr->custom_lua_search))) return;
			if (c_ptr->custom_lua_search_diff_minus) chance -= c_ptr->custom_lua_search_diff_minus;
			if (c_ptr->custom_lua_search_diff_chance) chance = (chance * c_ptr->custom_lua_search_diff_chance) / 100;

			/* Sometimes, notice things */
			if (!magik(chance)) continue;

			/* Access the object */
			o_ptr = &o_list[c_ptr->o_idx];

#ifdef NO_COMBO_FINDINGS
			findings = 0;
			/* Secret door */
			if (c_ptr->feat == FEAT_SECRET) {
				finding[findings] = 0;
				findings++;
			}
			/* Invisible trap */
			//if (c_ptr->feat == FEAT_INVIS) &&
			if ((cs_ptr = GetCS(c_ptr, CS_TRAPS)) && !cs_ptr->sc.trap.found) {
				finding[findings] = 1;
				findings++;
			}
			/* Search chests */
			if (o_ptr->tval == TV_CHEST /* Examine chests for traps */
			    //&& !object_known_p(Ind, o_ptr) && t_info[o_ptr->pval])
			    && !object_known_p(Ind, o_ptr) && o_ptr->pval) {
				finding[findings] = 2;
				findings++;
			}
			if (c_ptr->custom_lua_search > 0 && exec_lua(0, format("custom_search(%d,%d)", Ind, c_ptr->custom_lua_search))) return;
			if (!findings) continue;

			switch (finding[rand_int(findings)]) {
			case 0:
#else
			/* Secret door */
			if (c_ptr->feat == FEAT_SECRET) {
#endif
				/* Message */
				msg_print(Ind, "You have found a secret door.");
				/* Pick a door XXX XXX XXX */
				c_ptr->feat = FEAT_DOOR_HEAD + 0x00; // add random 'lockage'? ' + (!rand_int(4) ? rand_int(8) : 0) ' -- not just here but whereever secret doors are un-secreted
				/* Clear mimic feature */
				if ((cs_ptr = GetCS(c_ptr, CS_MIMIC))) cs_erase(c_ptr, cs_ptr);
				/* Notice */
				note_spot_depth(wpos, y, x);
				/* Redraw */
				everyone_lite_spot(wpos, y, x);
				/* Disturb */
				disturb(Ind, 0, 0);
#ifdef NO_COMBO_FINDINGS
				break;
			case 1:
#else
			}
			/* Invisible trap */
			//if (c_ptr->feat == FEAT_INVIS) &&
			if ((cs_ptr = GetCS(c_ptr, CS_TRAPS)) && !cs_ptr->sc.trap.found) {
#endif
				/* Mark trap as found */
				trap_found(wpos, y, x);
				/* Display it */
				if (c_ptr->o_idx && !c_ptr->m_idx) {
					byte a = get_trap_color(Ind, cs_ptr->sc.trap.t_idx, c_ptr->feat);

					/* Hack - Always show traps under items when detecting - mikaelh */
#ifdef GRAPHICS_BG_MASK
					draw_spot_ovl(Ind, y, x, a, p_ptr->f_char[FEAT_TRAP], 0, 0);
#else
					draw_spot_ovl(Ind, y, x, a, p_ptr->f_char[FEAT_TRAP]);
#endif
				} else {
					/* Normal redraw */
					lite_spot(Ind, y, x);
				}
				/* Message */
				msg_print(Ind, "You have found a trap.");
				/* Disturb */
				disturb(Ind, 0, 0);
#ifdef NO_COMBO_FINDINGS
				break;
			case 2:
#else
			}
			/* Search chests */
			else if (o_ptr->tval == TV_CHEST /* Examine chests for traps */
			    //&& !object_known_p(Ind, o_ptr) && t_info[o_ptr->pval])
			    && !object_known_p(Ind, o_ptr) && o_ptr->pval) {
#endif
				/* Message */
				msg_print(Ind, "You have discovered a trap on the chest!");
				/* Know the trap */
				object_known(o_ptr);
				/* Notice it */
				disturb(Ind, 0, 0);
			}

			if (c_ptr->custom_lua_search > 0 && exec_lua(0, format("custom_search(%d,%d)", Ind, c_ptr->custom_lua_search))) return;
		}
	}
}


/* Hack -- tell player of the next object on the pile */
void whats_under_your_feet(int Ind, bool force) {
	object_type *o_ptr;

	char    o_name[ONAME_LEN];
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type *c_ptr;
	cave_type **zcave;

	if (p_ptr->ghost && !force) return;

	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	if (!c_ptr->o_idx) return;

	/* Get the object */
	o_ptr = &o_list[c_ptr->o_idx];

	if (!o_ptr->k_idx) return;

	/* Auto id ? */
	if (p_ptr->auto_id) {
		object_aware(Ind, o_ptr);
		object_known(o_ptr);
	}

	/* Describe the object */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	if (is_atleast(&p_ptr->version, 4, 7, 3, 3, 0, 0)) {
		Send_whats_under_you_feet(Ind, o_name, !exceptionally_shareable_item(o_ptr) && compat_pomode(Ind, o_ptr), p_ptr->blind || no_lite(Ind), o_ptr->next_o_idx > 0);
	} else {
		if (!exceptionally_shareable_item(o_ptr) && compat_pomode(Ind, o_ptr)) {
			if (p_ptr->blind || no_lite(Ind))
				msg_format(Ind, "\377DYou feel %s%s here.", o_name, o_ptr->next_o_idx ? " on a pile" : "");
			else
				msg_format(Ind, "\377DYou see %s%s.", o_name, o_ptr->next_o_idx ? " on a pile" : "");
		} else {
			if (p_ptr->blind || no_lite(Ind))
				msg_format(Ind, "You feel %s%s here.", o_name, o_ptr->next_o_idx ? " on a pile" : "");
			else
				msg_format(Ind, "You see %s%s.", o_name, o_ptr->next_o_idx ? " on a pile" : "");
		}
	}
}

#ifdef ENABLE_SUBINVEN
/* For shop-purchase/steal and home-'purchase': Check if we can directly stow an item into any bag we might carry, for when the normal inventory is full.
   Note: While auto_stow() just takes a specific subinven as parameter, auto_stow_okay() actually scans all exsiting subinvens in the player's inventory. */
bool auto_stow_okay(int Ind, object_type *o_ptr, bool store_bought) {
	int i;
	object_type *s_ptr;
	player_type *p_ptr = Players[Ind];

	/* Inscription to specifically prevent auto-stowing of an object. Useful for *perception* staff deposited in a house for example. */
	if (check_guard_inscription(o_ptr->note, 'S')) return(FALSE);

	/* Don't auto-stow true artifacts;
	   exception for true-art trapkits is possible, but would need to add subinvens to art-location and -erasure code first. */
	if (true_artifact_p(o_ptr)) return(FALSE);

	/* Don't auto-stow unidentified items */
	if (!object_known_p(Ind, o_ptr) || !object_aware_p(Ind, o_ptr)) return(FALSE);

	/* Don't auto-stow if player cannot access stowed items due to outdated client */
	if (is_older_than(&p_ptr->version, 4, 8, 0, 0, 0, 0)) return(FALSE);

	for (i = 0; i < INVEN_PACK; i++) {
		s_ptr = &p_ptr->inventory[i];
		/* Subinvens are at the top of the inventory */
		if (s_ptr->tval != TV_SUBINVEN) break;

		/* Must fit the object type */
		if (!item_matches_subinven(Ind, get_subinven_group(s_ptr->sval), o_ptr)) continue;

		/* Player disabled auto-stow via bag inscription? */
		if (!subinven_can_stack(Ind, o_ptr, i, store_bought)) {
 #ifdef SUBINVEN_LIMIT_GROUP
			break;
 #else
			continue;
 #endif
		}

		return(TRUE);
	}

	return(FALSE);
}

/* Try to put a newly acquired item into a specialized bag automatically.
   If the item was from the floor, o_idx must be specified, otherwise it must be -1.
   Returns:
    If item was completely stowed, returns subinvenslot of the item stowed.
    If item was partially stowed, return -subinvenslot if any of the six @<A|O|S><0|1> inscriptions is on the bag, otherwise FALSE.
    If item was not stowed at all, returns FALSE. (Eg we have to set try_pickup = FALSE in carry() in that case).
    If o_stowed_ptr isn't NULL its reference will be set to the resulting stowed object. */
s16b auto_stow(int Ind, int sub_sval, object_type *o_ptr, int o_idx, bool pick_one, bool store_bought, bool quiet, u32b cave_info) {
	int i, num, slot, globalslot;
	object_type *s_ptr, forge_one, *o_ptr_tmp = o_ptr;
	player_type *p_ptr = Players[Ind];
	bool delete_it, fully_stowed = FALSE, stowed_some = FALSE, pick_all = FALSE;

	/* '!g' inscription forces pick_one (if it wasn't set). */
	if (!pick_one && check_guard_inscription(o_ptr->note, 'g')) pick_one = TRUE;
	/* Hack: !g and !G turn 'EXPLICIT pick_one' into 'pick_all' */
	else if (pick_one && (check_guard_inscription(o_ptr->note, 'G') || check_guard_inscription(o_ptr->note, 'g'))) {
		pick_one = FALSE;
		pick_all = TRUE;
	}

	/* Inscription to specifically prevent auto-stowing of an object. Useful for *perception* staff deposited in a house for example. */
	if (check_guard_inscription(o_ptr->note, 'S')) return(FALSE);

	/* Don't auto-stow true artifacts;
	   exception for true-art trapkits is possible, but would need to add subinvens to art-location and -erasure code first. */
	if (true_artifact_p(o_ptr)) return(FALSE);

	/* Don't auto-stow unidentified items */
	if (!object_known_p(Ind, o_ptr) || !object_aware_p(Ind, o_ptr)) return(FALSE);

	/* Don't auto-stow if player cannot access stowed items due to outdated client */
	if (is_older_than(&p_ptr->version, 4, 8, 0, 0, 0, 0)) return(FALSE);

	/* Hack number */
	forge_one.tval = 0;
	if (pick_one) {
		num = o_ptr->number;

		/* for pick_one: need to divide wand/staff charges */
		if (is_magic_device(o_ptr->tval) && num > 1) {
			forge_one = *o_ptr;
			forge_one.number = 1;
			divide_charged_item(&forge_one, o_ptr, 1);
			o_ptr->number--;
			o_ptr = &forge_one;
		} else o_ptr->number = 1;
	}

	for (i = 0; i < INVEN_PACK; i++) {
		s_ptr = &p_ptr->inventory[i];
		/* Subinvens are at the top of the inventory */
		if (s_ptr->tval != TV_SUBINVEN) break;

		/* Must fit the object type */
		if (s_ptr->sval != sub_sval) continue;

		/* Don't auto-stow if player cannot access stowed items due to outdated client */
		if (s_ptr->sval == SV_SI_POTION_BELT && !is_newer_than(&p_ptr->version, 4, 9, 1, 0, 0, 0)) continue;

 #ifdef SI_WRAPPING_SKILL
		if (s_ptr->sval == SV_SI_MDEVP_WRAPPING && get_skill(p_ptr, SKILL_DEVICE) < SI_WRAPPING_SKILL && get_skill(p_ptr, SKILL_TRAPPING) < SI_WRAPPING_SKILL) continue;
 #endif

		/* Player disabled auto-stow via bag inscription? */
		if (!subinven_can_stack(Ind, o_ptr, i, store_bought)) {
 #ifdef SUBINVEN_LIMIT_GROUP
			break;
 #else
			continue;
 #endif
		}

		handle_pickup_item(Ind, o_ptr, cave_info);

		/* Eligible subinventory found, try to move as much as possible */
		slot = subinven_stow_aux(Ind, o_ptr, i, quiet, pick_all);
		if (slot) { /* Paranoia. Will always be TRUE as we already checked subinven_can_stack() just above. */
			stowed_some = TRUE;
			globalslot = (i + 1) * SUBINVEN_INVEN_MUL + ABS(slot) - 1;
			Send_item_newest(Ind, globalslot);
			Send_apply_auto_insc(Ind, globalslot);
			if ((fully_stowed = (slot > 0))) break; /* If complete stack was moved, we're done */
		}
 #ifdef SUBINVEN_LIMIT_GROUP
		break;
 #endif
	}

	if (forge_one.tval) o_ptr = o_ptr_tmp;
	else
	/* Unhack number */
	if (pick_one) o_ptr->number += num - 1;

	if (!o_ptr->number) delete_it = TRUE;
	else delete_it = FALSE;

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);
	/* Recalculate mana */
	p_ptr->update |= (PU_MANA | PU_HP | PU_SANITY);
	/* Redraw */
	p_ptr->redraw |= (PR_PLUSSES | PR_ARMOR);
	/* Window stuff */
	//p_ptr->window |= (PW_EQUIP | PW_PLAYER);

	/* If it was not an item from the floor, we'll discard/delete it manually */
	if (o_idx == -1) return(delete_it ? globalslot : FALSE);

	/* We picked up everything there was! Delete original */
	if (delete_it) {
		delete_object_idx(o_idx, FALSE, FALSE);

		/* Hack -- tell the player of the next object on the pile */
		whats_under_your_feet(Ind, FALSE);

		/* Tell the client */
		Send_floor(Ind, 0);

		return(globalslot);
	} else if (!pick_one) {
		if (check_guard_inscription(s_ptr->note, 'A') >= 1 ||
		    check_guard_inscription(s_ptr->note, 'O') >= 1 ||
		    check_guard_inscription(s_ptr->note, 'S') >= 1) {
			/* As long as we did stow some, stop for now. If the player wants to pick up the
			   rest of the stack, he'll have to reissue the pickup command.
			   Reasoning: !An, !On, !Sn are usually used for managing restocking of the bag.
			              Therefore it is unlikely that the player intends to pick up the
			              remaining stack into his normal inventory. */
			return(stowed_some ? -globalslot : FALSE);
		}
		/* There are still items left in the stack, and we didn't try to pick up just one,
		   so additionally try now to pick up the rest of this pile normally */
		//o_ptr = &o_list[o_idx];
		return(FALSE);
	}
	/* We tried to pick_one and managed to stow it! So we're done. */
	else if (fully_stowed) return(globalslot);
	/* We tried to pick_one, but still failed to stow it! So try to pick it up normally now. */
	else return(FALSE);
}
#endif

void handle_pickup_item(int Ind, object_type *o_ptr, u32b cave_info) {
	player_type *p_ptr = Players[Ind];
	char o_name[ONAME_LEN], o_name_real[ONAME_LEN];
	struct worldpos *wpos = &p_ptr->wpos;

	object_desc(Ind, o_name, o_ptr, TRUE, 3);
	object_desc(0, o_name_real, o_ptr, TRUE, 3);

	/* Check whether this item was requested by an item-retrieval quest */
	if (p_ptr->quest_any_r_within_target) quest_check_goal_r(Ind, o_ptr);

	/* Own it */
	if (!o_ptr->owner) {
		int dlev = getlevel(&p_ptr->wpos); /* Just for logging 'expensive' items in relation to dlev */
		s64b value = object_value_real(0, o_ptr); //add * o_ptr->number?
		int min_value = (dlev <= 10 ? 10000 : (dlev < 30 ? dlev * 1000 : (dlev < 50 ? (dlev - 10) * 1500 : (dlev - 20) * 2000)));

		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;

		/* One-time imprint "*identifyability*" for client's ITH_STARID/item_tester_hook_starid: */
		if (!maybe_hidden_powers(Ind, o_ptr, FALSE, NULL)) o_ptr->ident |= ID_NO_HIDDEN;

		/* Actually only imprint iron_trade on newly owned items. This allows us to have stolen items be unexchangeable in IDDC if we want that: */
		o_ptr->iron_trade = p_ptr->iron_trade;
		if (o_ptr->iron_turn != -1) //paranoia
			o_ptr->iron_turn = turn;

#if CHEEZELOG_LEVEL > 2
		/* Log when an item is found that is especially valuable (for its dungeon level, relative for dlevs < 30). */
		if (value >= min_value)
			s_printf("EXPENSIVE_ITEM: %s (%ld Au) found by %s(lv %d) at %d,%d,%d%s%s (dlv %d)\n",
			    o_name_real, value, p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz,
			    (cave_info & CAVE_STCK) ? "N" : (cave_info & CAVE_ICKY) ? "V" : "", (o_ptr->marked2 == ITEM_REMOVAL_NEVER) ? "G" : "", dlev);
#endif

		if (true_artifact_p(o_ptr)) {
			a_info[o_ptr->name1].carrier = p_ptr->id;
			determine_artifact_timeout(o_ptr->name1, wpos);
#if CHEEZELOG_LEVEL > 2
			s_printf("%s Artifact %d found by %s(lv %d) at %d,%d,%d%s%s: %s\n",
			    showtime(), o_ptr->name1, p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, (cave_info & CAVE_STCK) ? "N" : (cave_info & CAVE_ICKY) ? "V" : "", (o_ptr->marked2 == ITEM_REMOVAL_NEVER) ? "G" : "", o_name_real);
#endif
			/* Log top arts (except Grond/Crown of course) - atm this excludes Razorback and Mediator */
			if ((a_info[o_ptr->name1].level >= 100 || o_ptr->name1 == ART_DWARVEN_ALE)
			    && !multiple_artifact_p(o_ptr) && !is_admin(p_ptr)) {
				char o_name_short[ONAME_LEN];

				object_desc(0, o_name_short, o_ptr, TRUE, 256);
				l_printf("%s \\{U%s found %s\n", showdate(), p_ptr->name, o_name_short);
			}
		}
#if CHEEZELOG_LEVEL > 2
		else if (o_ptr->name1 == ART_RANDART) s_printf("%s Randart found by %s(lv %d) at %d,%d,%d%s%s : %s\n",
		    showtime(), p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, (cave_info & CAVE_STCK) ? "N" : (cave_info & CAVE_ICKY) ? "V" : "", (o_ptr->marked2 == ITEM_REMOVAL_NEVER) ? "G" : "", o_name_real);

#endif
		/* log the encounters of players with special heavy armour, just for informative purpose */
		if (k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) s_printf("%s FOUND_WINNERS_ONLY: %s (%d) %s\n", showtime(), p_ptr->name, p_ptr->wpos.wz, o_name_real);

		/* Some events don't allow transactions before they begin. Extend this to no_soloist-event-drops (Santa Claus). */
		if (o_ptr->no_soloist && !p_ptr->max_exp && !in_irondeepdive(wpos) && !in_hallsofmandos(wpos)) {
			msg_print(Ind, "Due to the item's origin you gain a tiny bit of experience from picking it up.");
			gain_exp(Ind, 1);
		}
		/* Prevent IDDC cheeze */
		else if (wpos->wz && !p_ptr->max_exp && !in_irondeepdive(wpos) && !in_hallsofmandos(wpos)) {
			msg_print(Ind, "You gain a tiny bit of experience from picking up your first item in a dungeon.");
			gain_exp(Ind, 1);
		}
	}

#if CHEEZELOG_LEVEL > 2
	/* Take cheezelog
	 * TODO: ignore cheap items (like cure critical pot) */
 #if 0
	else if (p_ptr->id != o_ptr->owner &&
	    !(o_ptr->tval == TV_GAME && o_ptr->sval == SV_GAME_BALL) /* Heavy ball */ )
 #else
	else if (p_ptr->id != o_ptr->owner)
 #endif
	{
		cptr name = lookup_player_name(o_ptr->owner);
		int lev = lookup_player_level(o_ptr->owner);
		cptr acc_name = lookup_accountname(o_ptr->owner);

		object_desc_store(Ind, o_name, o_ptr, TRUE, 3);
 #if 0
		/* If level diff. is too large, target player is too low,
		   and items aren't loot of a dead player, this might be cheeze! */
		if ((lev > p_ptr->lev + 7) && (p_ptr->lev < 40) && (name)) {
			s_printf("%s -CHEEZY- Item transaction from %s(%d) to %s(%d) at (%d,%d,%d):\n  %s\n",
				showtime(), name ? name : "(Dead player)", lev,
				p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz,
				o_name);
			c_printf("%s ITEM %s(%d,%s) %s(%d,%s) %" PRId64 "(%d%%) %s\n",
				showtime(), name ? name : "(---)", lev, acc_name,
				p_ptr->name, p_ptr->lev, p_ptr->accountname,
				object_value_real(0, o_ptr), o_ptr->discount, o_name);
		} else {
			s_printf("%s Item transaction from %s(%d) to %s(%d) at (%d,%d,%d):\n  %s\n",
				showtime(), name ? name : "(Dead player)", lev,
				p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz,
				o_name);
			c_printf("%s item %s(%d,%s) %s(%d,%s) %" PRId64 "(%d%%) %s\n",
				showtime(), name ? name : "(---)", lev, acc_name,
				p_ptr->name, p_ptr->lev, p_ptr->accountname,
				object_value_real(0, o_ptr), o_ptr->discount, o_name);
		}
 #else
		s_printf("%s Item transaction from %s(%d) to %s(%d%s) at (%d,%d,%d):\n  %s\n",
				showtime(), name ? name : "(Dead player)", lev,
				p_ptr->name, p_ptr->lev, p_ptr->total_winner ? ",W" : (p_ptr->once_winner ? ",O" : ""),
				p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz,
				o_name);
		c_printf("%s ITEM %s(%d,%s) : %s(%d,%s%s) %" PRId64 "(%d%%) : %s\n",
				showtime(), name ? name : "(---)", lev, acc_name,
				p_ptr->name, p_ptr->lev, p_ptr->accountname,
				p_ptr->total_winner ? ",W" : (p_ptr->once_winner ? ",O" : ""),
				object_value_real(0, o_ptr), o_ptr->discount, o_name);
 #endif

/* Since there apparently is sometimes a bug happening that triggers the 'redistributed' hack in load.c, log this to track the bug */
s_printf("bugtracking: name1=%d, owner=%d(%s), carrier=%d, p-id=%d(%s)\n", o_ptr->name1, o_ptr->owner, name, a_info[o_ptr->name1].carrier, p_ptr->id, p_ptr->name);

		if (true_artifact_p(o_ptr)) a_info[o_ptr->name1].carrier = p_ptr->id;

		/* Some events don't allow transactions before they begins */
		if (!p_ptr->max_exp && !in_irondeepdive(wpos) && !in_hallsofmandos(wpos)) {
			msg_print(Ind, "You gain a tiny bit of experience from trading an item.");
			gain_exp(Ind, 1);
		}
	}
#endif	// CHEEZELOG_LEVEL

#if CHEEZELOG_LEVEL > 2
	/* for PRE_OWN_DROP_CHOSEN: */
	else {
		if (true_artifact_p(o_ptr))
			s_printf("%s Artifact %d picked up by %s(lv %d) at %d,%d,%d%s%s: %s\n",
			    showtime(), o_ptr->name1, p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, (cave_info & CAVE_STCK) ? "N" : (cave_info & CAVE_ICKY) ? "V" : "", (o_ptr->marked2 == ITEM_REMOVAL_NEVER) ? "G" : "", o_name_real);
 #if 0 /* pointless spam */
		else if (o_ptr->name1 == ART_RANDART)
			s_printf("%s Randart found by %s(lv %d) at %d,%d,%d%s%s : %s\n",
			    showtime(), p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, (cave_info & CAVE_STCK) ? "N" : (cave_info & CAVE_ICKY) ? "V" : "", (o_ptr->marked2 == ITEM_REMOVAL_NEVER) ? "G" : "", o_name_real);
 #endif
	}
#endif

	/* log special objects, except for seals */
	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval) {
		s_printf("%s Special object '%s' sv=%d,x1=%d,x2=%d,q=%d,qs=%d picked up by %s(lv %d) at %d,%d,%d%s%s : %s\n",
		    showtime(), o_name_real, o_ptr->sval, o_ptr->xtra1, o_ptr->xtra2, o_ptr->quest, o_ptr->quest_stage, p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, (cave_info & CAVE_STCK) ? "N" : (cave_info & CAVE_ICKY) ? "V" : "", (o_ptr->marked2 == ITEM_REMOVAL_NEVER) ? "G" : "", o_name_real);
	}

	can_use(Ind, o_ptr);
	/* for Ironman Deep Dive Challenge cross-trading */
	o_ptr->mode = p_ptr->mode;

#ifdef ENABLE_SUBINVEN
	/* ('empty-chest-hack') - Auto-remove 'empty' marker if we pick it up */
	if (o_ptr->tval == TV_SUBINVEN && o_ptr->xtra8) o_ptr->xtra8 = 2;
#endif
}

/*
 * Player "wants" to pick up an object or gold.
 * Note that we ONLY handle things that can be picked up.
 * See "move_player()" for handling of other things.
 *
 * 'pickup':   0 = don't pick up anything
 *             1 = pick up ('g') or autopickup-on-move (client option)
 *             2 = explicit pickup ('g' key)
 * 'pick_one': Only pick up one piece from a stack of same item type
 *             (currently not implemented for ammo)
 * 'confirm':  Not implemented. Would ask the player if he really wants to.
 */
/* Prevent characters in Bree from taking gold/items while they cannot drop
   them again due to being lower level than cfg.newbies_cannot_drop? */
#define NEWBIES_CANT_GRAB_IN_BREE
void carry(int Ind, int pickup, int confirm, bool pick_one) {
	object_type *o_ptr;

	char    o_name[ONAME_LEN], o_name_real[ONAME_LEN];
	u16b	old_note;
	char	old_note_utag;
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type *c_ptr;
	cave_type **zcave;

	bool forbidden = FALSE; /* for leaderless guild halls */

#ifdef USE_SOUND_2010
	bool inven_carried = FALSE; /* avoid duplicate sfx */
#endif

	/* stuff for 'pick_one' hack: */
	bool pick_one_org = pick_one; //remember our original intention for auto_stow(), as pick_one will get modified here
	int num_org;
	bool try_pickup = TRUE;
	bool delete_it = TRUE; /* usually, the item is fully picked up and therefore deleted from the floor */
	bool pick_all = FALSE;


	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* Hack -- nothing here to pick up */
	if (!(c_ptr->o_idx)) {
		if (pickup == 2) (void)create_snowball(Ind, c_ptr);
		return;
	}

	/* Ghosts cannot pick things up */
	if (p_ptr->ghost && !is_admin(p_ptr)) {
		if (pickup) {
			//anti-spam? p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;
			msg_print(Ind, "\377yGhosts cannot pick up things. You need to get resurrected first.");
		}
		/* Don't even do the "You see/feel ... here" messages */
		return;
	}

#if 0
    /* ok this is too harsh. also, it can be assumed that if you bow down to pick up items,
       they enter your wraith sphere well enough to become accessible */
	/* not while in.. WRAITHFORM :D */
	if (p_ptr->tim_wraith && !p_ptr->admin_dm) return;
#endif

	/* Get the object */
	o_ptr = &o_list[c_ptr->o_idx];
	/* Hack: !g and !G turn 'EXPLICIT pick_one' into 'pick_all' */
	if (pickup && pick_one && (check_guard_inscription(o_ptr->note, 'G') || check_guard_inscription(o_ptr->note, 'g'))) {
		pick_one = FALSE;
		pick_all = TRUE;
	}

	if (nothing_test(o_ptr, p_ptr, &p_ptr->wpos, p_ptr->px, p_ptr->py, 9)) return; //was 1

	/* Hack: !g inscription induces pick_one! (Only when picking up, not when just feeling/looking at the item) */
	if (pickup && !pick_all && check_guard_inscription(o_ptr->note, 'g')) pick_one = TRUE;

	/* Cannot pick up stuff in leaderless guild halls */
	if ((zcave[p_ptr->py][p_ptr->px].info2 & CAVE2_GUILD_SUS) &&
	    /* exception: Guild Keys can always be picked up, since they make you the new guild master
	       and therefore end the 'leaderless' status of a guild. */
	    !(o_ptr->tval == TV_KEY && o_ptr->sval == SV_GUILD_KEY))
		 forbidden = TRUE;

	/* Auto id ? */
	if (p_ptr->auto_id) {
		object_aware(Ind, o_ptr);
		object_known(o_ptr);
	}

#ifdef SUBINVEN_LIMIT_GROUP
	/* Hack for bags, since they are allowed to stack on the floor (eg inside houses)
	   but not in the player inventory: */
	if (o_ptr->tval == TV_SUBINVEN) pick_one = TRUE;
#endif

	/* Describe the object */
	if (pick_one && !is_ammo(o_ptr->tval)) { //hack the name, but 'pick_one' is currently not implemeted for ammo
		int num = o_ptr->number;

		o_ptr->number = 1;
		object_desc(Ind, o_name, o_ptr, TRUE, 3);
		object_desc(0, o_name_real, o_ptr, TRUE, 3);
		o_ptr->number = num;
	} else {
		object_desc(Ind, o_name, o_ptr, TRUE, 3);
		object_desc(0, o_name_real, o_ptr, TRUE, 3);
	}


	/* Pick up gold */
	if (o_ptr->tval == TV_GOLD) {
		s32b amount = o_ptr->pval;

		/* Disturb */
		disturb(Ind, 0, 0);

		/* hack for cloaking: since picking up anything breaks it,
		   we don't pickup gold except if the player really wants to */
		if (((p_ptr->cloaked == 1 || p_ptr->shadow_running) && !pickup) || forbidden || (p_ptr->ghost && !p_ptr->admin_dm)) {
			if (is_atleast(&p_ptr->version, 4, 7, 3, 3, 0, 0)) {
				Send_whats_under_you_feet(Ind, o_name, compat_pomode(Ind, o_ptr), p_ptr->blind || no_lite(Ind), o_ptr->next_o_idx);
			} else {
				if (compat_pomode(Ind, o_ptr)) {
					if (p_ptr->blind || no_lite(Ind))
						msg_format(Ind, "\377DYou feel %s%s here.", o_name, o_ptr->next_o_idx ? " on a pile" : "");
					else
						msg_format(Ind, "\377DYou see %s%s.", o_name, o_ptr->next_o_idx ? " on a pile" : "");
				} else {
					if (p_ptr->blind || no_lite(Ind))
						msg_format(Ind, "You feel %s%s here.", o_name, o_ptr->next_o_idx ? " on a pile" : "");
					else
						msg_format(Ind, "You see %s%s.", o_name, o_ptr->next_o_idx ? " on a pile" : "");
				}
			}
			Send_floor(Ind, o_ptr->tval);
			return;
		}

		if (p_ptr->IDDC_logscum && !o_ptr->owner) {//o_ptr->owner != p_ptr->id) {
			msg_print(Ind, "\377oThis floor has become stale, take a staircase to move on!");
			return;
		}

		if (p_ptr->mode & MODE_SOLO) {
			if (o_ptr->owner && o_ptr->owner != p_ptr->id) {
				msg_print(Ind, "\377yYou cannot exchange goods or money with other players.");
				if (!is_admin(p_ptr)) return;
			}
			if (!o_ptr->owner && o_ptr->killer && o_ptr->killer != p_ptr->id) {
				msg_print(Ind, "\377yYou cannot take loot that does not belong to you.");
				if (!is_admin(p_ptr)) return;
			}
			if (o_ptr->no_soloist) {
				msg_print(Ind, "\377yDue to its origin this item is not eligible for Soloists.");
				if (!is_admin(p_ptr)) return;
			}
		}
#ifdef IDDC_IRON_COOP
		if (in_irondeepdive(wpos) && o_ptr->owner && o_ptr->owner != p_ptr->id
		    //&& (!p_ptr->party || lookup_player_party(o_ptr->owner) != p_ptr->party)) {
		    && o_ptr->iron_trade != p_ptr->iron_trade) {
			msg_print(Ind, "\377yYou cannot pick up money from outsiders.");
			if (!is_admin(p_ptr)) return;
		}
#endif
#ifdef IRON_IRON_TEAM
		if (p_ptr->party && (parties[p_ptr->party].mode & PA_IRONTEAM) && o_ptr->owner && o_ptr->owner != p_ptr->id
		    //&& lookup_player_party(o_ptr->owner) != p_ptr->party) {
		    && o_ptr->iron_trade != p_ptr->iron_trade) {
			msg_print(Ind, "\377yYou cannot pick up money from outsiders.");
			if (!is_admin(p_ptr)) return;
		}
#endif
#ifdef IDDC_RESTRICTED_TRADING
		if (o_ptr->owner && o_ptr->owner != p_ptr->id && in_irondeepdive(&p_ptr->wpos) && !o_ptr->xtra3) { //xtra3: money from chests is exempt
			msg_print(Ind, "\377yYou cannot transfer money in the Ironman Deep Dive Challenge.");
			if (!is_admin(p_ptr)) return;
		}
#endif

#ifdef NEWBIES_CANT_GRAB_IN_BREE
		/* Avoid people picking up things that they cannot drop again? */
		if (in_bree(wpos) &&
		    p_ptr->max_plv < cfg.newbies_cannot_drop && o_ptr->owner && p_ptr->id != o_ptr->owner) {
			msg_format(Ind, "You cannot take gold from other people in Bree until you are level %d.", cfg.newbies_cannot_drop);
			if (!is_admin(p_ptr)) return;
		}
#endif

		if (p_ptr->inval && o_ptr->owner && p_ptr->id != o_ptr->owner) {
			msg_print(Ind, "\377oYou cannot take gold of other players without a valid account.");
			return;
		}

		if (compat_pomode(Ind, o_ptr)) {
			msg_format(Ind, "You cannot take money of %s players.", compat_pomode(Ind, o_ptr));
			if (!is_admin(p_ptr)) return;
		}

		if ((p_ptr->mode & MODE_DED_IDDC) && !in_irondeepdive(&p_ptr->wpos)
#ifdef DED_IDDC_MANDOS
		    && !in_hallsofmandos(&p_ptr->wpos)
#endif
		    && o_ptr->owner && o_ptr->owner != p_ptr->id) {
			msg_print(Ind, "\377yYou cannot pick up someone else's money or your life would be forfeit.");
			return;
		}
#ifdef EVENT_TOWNIE_GOLD_LIMIT
		/* If we are still below the limit but this gold pile would exceed it
		   then only pick up as much of it as is allowed! - C. Blue */
		if (!p_ptr->max_exp && EVENT_TOWNIE_GOLD_LIMIT != -1 &&
		    !o_ptr->owner &&
		    p_ptr->gold_picked_up < EVENT_TOWNIE_GOLD_LIMIT && amount > EVENT_TOWNIE_GOLD_LIMIT - p_ptr->gold_picked_up)
			amount = EVENT_TOWNIE_GOLD_LIMIT - p_ptr->gold_picked_up;
#endif
#ifdef IDDC_AUTOSPLIT_GOLD
		/* Pondos suggested that an IDDC party member with Digging doesn't allow
		   the other two to profit from its ability, here's an attempt to remedy this: */
		if (!o_ptr->owner && in_irondeepdive(wpos) && p_ptr->party) {
			int i, p = 0, PInd[NumPlayers], amount_dist;
			player_type *q_ptr;

			/* Paranoia-check on ourself, rough 'worst case' estimate.
			   If we don't pass, we don't touch the gold at all and hence won't distribute it either */
			if (PY_MAX_GOLD - amount / 2 < p_ptr->au) {
				msg_format(Ind, "\377yYou cannot carry more than %d gold!", PY_MAX_GOLD);
				return;
			}

			/* Count party members on the same floor with us */
			for (i = 1; i <= NumPlayers; i++) {
				if (i == Ind) continue; /* skip ourself */
				q_ptr = Players[i];
				if (q_ptr->conn == NOT_CONNECTED) continue;
				if (is_admin(p_ptr) != is_admin(q_ptr)) continue;
				if (!player_in_party(q_ptr->party, i)) continue;
				if (!inarea(&q_ptr->wpos, wpos)) continue;
				if (q_ptr->IDDC_logscum) continue;

				/* Create map for efficiency */
				PInd[p] = i;
				p++;
			}
			/* Found any other party members here? Otherwise we're out of here and continue normally. */
			if (p) {
				/* Paranoia-check on everyone else, rough 'worst case' estimate again. */
				for (i = 0; i < p; i++)
					if (PY_MAX_GOLD - amount / 2 < Players[PInd[i]]->au) {
						/* Remove this player from the list to share with */
						PInd[i] = PInd[p - 1];
						PInd[p - 1] = 0;
						p--;
					}

				/* If other eligible players are there and amount (rounded down) isn't too tiny to split, split the gold evenly */
				if (p && amount / (p + 1) != 0) {
					/* Try to split gold as evenly as possible */
					amount_dist = amount / (p + 1);
					/* Correct rounding mistakes by leaving the original player potentially with extra Au matching the correct total sum
					   (eg 8 Au and 3 players -> +4 (us) +2 +2): */
					amount -= amount_dist * p;
					/* Distribute */
					for (i = 0; i < p; i++) {
						(void)gain_au(PInd[i], amount_dist, FALSE, TRUE);
 #if 1 /* hm, spammy? */
						msg_format(PInd[i], "You have gained a share of %d gold pieces worth of %s.", amount_dist, o_name);
 #endif
 #ifdef USE_SOUND_2010
						sound(PInd[i], "pickup_gold", NULL, SFX_TYPE_COMMAND, FALSE);
 #endif
						Players[PInd[i]]->window |= (PW_PLAYER);
						/* Reduce the gold object accordingly! */
						o_ptr->pval -= amount_dist;
					}
				}
			}
		}
#endif
		/* Collect the gold */
		if (!gain_au(Ind, amount, FALSE, p_ptr->id == o_ptr->owner)) return;

		/* Message */
		msg_format(Ind, "You have found %d gold pieces worth of %s.", amount, o_name);
#ifdef USE_SOUND_2010
		sound(Ind, "pickup_gold", NULL, SFX_TYPE_COMMAND, FALSE);
		inven_carried = TRUE;
#endif
		if ((c_ptr->info2 & CAVE2_MINED) && !p_ptr->warning_tunnel_hidden) {
			msg_print(Ind, "\374\377yHINT: Mining hidden veins yields more than the right away spottable ones!");
			c_ptr->info2 &= ~CAVE2_MINED;
			p_ptr->warning_tunnel_hidden = 1;
			s_printf("warning_tunnel_hidden: %s\n", p_ptr->name);
		}
/* #if DEBUG_LEVEL > 3 */
		if (amount >= 10000) s_printf("Gold found threshold (%d by %s at %d,%d,%d).\n", amount, p_ptr->name, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Take log for possible cheeze */
#if CHEEZELOG_LEVEL > 1
		if (o_ptr->owner) {
			cptr name = lookup_player_name(o_ptr->owner);
			int lev = lookup_player_level(o_ptr->owner);
			cptr acc_name = lookup_accountname(o_ptr->owner);

			if (p_ptr->id != o_ptr->owner) {
 #if 0
				if ((lev > p_ptr->lev + 7) && (p_ptr->lev < 40) && (name)) {
					s_printf("%s -CHEEZY- Money transaction: %dau from %s(%d) to %s(%d) at (%d,%d,%d)\n",
							showtime(), amount, name ? name : "(Dead player)", lev,
							p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
					c_printf("%s GOLD %s(%d,%s) %s(%d,%s) %d\n",
							showtime(), name ? name : "(---)", lev, acc_name,
							p_ptr->name, p_ptr->lev, p_ptr->accountname, amount);
				} else {
					s_printf("%s Money transaction: %dau from %s(%d) to %s(%d) at (%d,%d,%d)\n",
							showtime(), amount, name ? name : "(Dead player)", lev,
							p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
					c_printf("%s gold %s(%d,%s) %s(%d,%s) %d\n",
							showtime(), name ? name : "(---)", lev, acc_name,
							p_ptr->name, p_ptr->lev, p_ptr->accountname, amount);
				}
 #else
				s_printf("%s Money transaction: %dau from %s(%d) to %s(%d%s) at (%d,%d,%d)\n",
						showtime(), amount, name ? name : "(Dead player)", lev,
						p_ptr->name, p_ptr->lev,
						p_ptr->total_winner ? ",W" : (p_ptr->once_winner ? ",O" : ""),
						p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
				c_printf("%s GOLD %s(%d,%s) : %s(%d,%s%s) : %d\n",
						showtime(), name ? name : "(---)", lev, acc_name,
						p_ptr->name, p_ptr->lev, p_ptr->accountname,
						p_ptr->total_winner ? ",W" : (p_ptr->once_winner ? ",O" : ""),
						amount);
 #endif
				/* Some events don't allow transactions before they begin */
				if (!p_ptr->max_exp && !in_irondeepdive(wpos) && !in_hallsofmandos(wpos)) {
					msg_print(Ind, "You gain a tiny bit of experience from exchanging money.");
					gain_exp(Ind, 1);
				}
			}
		}
#endif	// CHEEZELOG_LEVEL


		/* Delete gold */
		if (amount == o_ptr->pval) {
			//delete_object(wpos, p_ptr->py, p_ptr->px, FALSE);
			delete_object_idx(c_ptr->o_idx, FALSE, FALSE);//TRUE, but it's only gold, so can't be trueart anyway
		}
		/* Reduce gold */
		else o_ptr->pval -= amount;

		/* Hack -- tell the player of the next object on the pile */
		whats_under_your_feet(Ind, FALSE);
	}

	/* Pick it up */
	else {
		bool force_pickup = check_guard_inscription(o_ptr->note, '=')
		    && p_ptr->id == o_ptr->owner && !p_ptr->ghost;
		bool auto_load = check_guard_inscription(o_ptr->note, 'L')
		    && p_ptr->id == o_ptr->owner && !p_ptr->ghost;
		int pick_some = FALSE, limitG = -1;
#ifdef ENABLE_SUBINVEN
		bool stowed = FALSE;
#endif

		/* Hack -- disturb */
		disturb(Ind, 0, 0);

		/* Hack for auto-picking up chemicals */
		if (o_ptr->tval == TV_CHEMICAL && o_ptr->sval != SV_MIXTURE && (o_ptr->mode & MODE_NEWLOOT_ITEM) && p_ptr->autopickup_chemicals) force_pickup = TRUE;

		/* Describe the object */
		if ((!pickup && !force_pickup) || forbidden) {
			char pseudoid[13];
			cptr feel;

			pseudoid[0] = 0;
			/* felt an (non-changing!) object of same kind before via pseudo-id? then remember.
			   Note: currently all objects for which that is true are 'magic', hence we only
			   use object_value_auxX_MAGIC() below. */
			if (!object_aware_p(Ind, o_ptr) && !object_known_p(Ind, o_ptr) && object_felt_p(Ind, o_ptr)
			    /* Also, rings and amulets aren't covered by auxX_magic, so we have to exempt them (null string!): */
			    && o_ptr->tval != TV_RING && o_ptr->tval != TV_AMULET) {
				if (!object_felt_heavy_p(Ind, o_ptr)) {
					/* only show pseudoid if its current inscription doesn't already tell us! */
					feel = value_check_aux2_magic(o_ptr);
					if (feel && (!o_ptr->note || strcmp(quark_str(o_ptr->note), feel)))
						sprintf(pseudoid, " {%s}", value_check_aux2_magic(o_ptr));
				} else {
					feel = value_check_aux1_magic(o_ptr);
					/* only show pseudoid if its current inscription doesn't already tell us! */
					if (feel && (!o_ptr->note || strcmp(quark_str(o_ptr->note), feel)))
						sprintf(pseudoid, " {%s}", value_check_aux1_magic(o_ptr));
				}
			}

			if (is_atleast(&p_ptr->version, 4, 7, 3, 3, 0, 0)) {
				strcat(o_name, pseudoid);
				Send_whats_under_you_feet(Ind, o_name, compat_pomode(Ind, o_ptr), p_ptr->blind || no_lite(Ind), o_ptr->next_o_idx);
			} else {
				if (compat_pomode(Ind, o_ptr)) {
					if (p_ptr->blind || no_lite(Ind))
						msg_format(Ind, "\377DYou feel %s%s%s here.", o_name, pseudoid, o_ptr->next_o_idx ? " on a pile" : "");
					else
						msg_format(Ind, "\377DYou see %s%s%s.", o_name, pseudoid, o_ptr->next_o_idx ? " on a pile" : "");
				} else {
					if (p_ptr->blind || no_lite(Ind))
						msg_format(Ind, "You feel %s%s%s here.", o_name, pseudoid, o_ptr->next_o_idx ? " on a pile" : "");
					else
						msg_format(Ind, "You see %s%s%s.", o_name, pseudoid, o_ptr->next_o_idx ? " on a pile" : "");
				}
			}
			Send_floor(Ind, o_ptr->tval);
			return;
		}

//#ifdef SUBINVEN_LIMIT_GROUP  -- rely on inven_carry_okay() for now
//#endif

		if (p_ptr->IDDC_logscum && !o_ptr->owner) {//o_ptr->owner != p_ptr->id) {
			msg_print(Ind, "\377oThis floor has become stale, take a staircase to move on!");
			if (!is_admin(p_ptr)) return;
		}

		if (p_ptr->mode & MODE_SOLO) {
			if (o_ptr->owner && o_ptr->owner != p_ptr->id) {
				msg_print(Ind, "\377yYou cannot exchange goods or money with other players.");
				if (!is_admin(p_ptr)) return;
			}
			if (!o_ptr->owner && o_ptr->killer && o_ptr->killer != p_ptr->id) {
				msg_print(Ind, "\377yYou cannot take loot that does not belong to you.");
				if (!is_admin(p_ptr)) return;
			}
			if (o_ptr->no_soloist) {
				msg_print(Ind, "\377yDue to its origin this item is not eligible for Soloists.");
				if (!is_admin(p_ptr)) return;
			}
		}
#ifdef IDDC_IRON_COOP
		if (in_irondeepdive(wpos) && o_ptr->owner && o_ptr->owner != p_ptr->id
		    //&& (!p_ptr->party || lookup_player_party(o_ptr->owner) != p_ptr->party)) {
		    && o_ptr->iron_trade != p_ptr->iron_trade) {
			msg_print(Ind, "\377yYou cannot pick up starter items or items from outsiders.");
			if (!is_admin(p_ptr)) return;
		}
#endif
#ifdef IRON_IRON_TEAM
		if (p_ptr->party && (parties[p_ptr->party].mode & PA_IRONTEAM) && o_ptr->owner && o_ptr->owner != p_ptr->id
		    //&& lookup_player_party(o_ptr->owner) != p_ptr->party) {
		    && o_ptr->iron_trade != p_ptr->iron_trade) {
			msg_print(Ind, "\377yYou cannot pick up starter items or items from outsiders.");
			if (!is_admin(p_ptr)) return;
		}
#endif
#ifdef IDDC_RESTRICTED_TRADING
		if (in_irondeepdive(&p_ptr->wpos) && o_ptr->owner && o_ptr->owner != p_ptr->id) {
 #if 1
			if (exceptionally_shareable_item(o_ptr)) o_ptr->discount = 100;
			else
 #endif
			if (p_ptr->IDDC_logscum) {//todo: DED_IDDC_MANDOS
				msg_print(Ind, "\377yYou cannot take items on stale floors.");
				if (!is_admin(p_ptr)) return;
			}
			else if (!o_ptr->iron_trade || !o_ptr->iron_turn) { /* includes the extra items every character who enters the IDDC gains as a starter bonus */
				msg_print(Ind, "\377yYou cannot trade items that haven't been found in the IDDC.");
				if (!is_admin(p_ptr)) return;
			}
			else if (!p_ptr->party) {
				msg_print(Ind, "\377yYou cannot take items from anyone not in your party.");
				if (!is_admin(p_ptr)) return;
			}
			else if (p_ptr->iron_trade != o_ptr->iron_trade) {
				if (lookup_player_party(o_ptr->owner) != p_ptr->party)
					msg_print(Ind, "\377yYou cannot take items from outsiders.");
				else
 #ifdef TEST_SERVER /* debug */
					msg_format(Ind, "\377yOwner found this before joining party (%d, %d, %d, %d).", p_ptr->iron_trade, o_ptr->iron_trade, p_ptr->iron_turn, o_ptr->iron_turn);
 #else
					msg_print(Ind, "\377yYou cannot take items whose owner found them before joining the party.");
 #endif
				if (!is_admin(p_ptr)) return;
			}
			else if (o_ptr->iron_turn == -1) {
				msg_print(Ind, "\377yYou cannot take this item as it was bought or stolen from a shop.");
				if (!is_admin(p_ptr)) return;
			}
			else if (p_ptr->iron_turn > o_ptr->iron_turn) {
 #ifdef TEST_SERVER /* debug */
					msg_format(Ind, "\377yPredates you joining party (%d, %d, %d, %d).", p_ptr->iron_trade, o_ptr->iron_trade, p_ptr->iron_turn, o_ptr->iron_turn);
 #else
				msg_print(Ind, "\377yYou cannot take this item as it predates you joining the party.");
 #endif
				if (!is_admin(p_ptr)) return;
			}
		}
#endif

		/* cannot carry the same questor twice */
		if (o_ptr->questor) {
			int i;
			object_type *qo_ptr;

			for (i = 0; i < INVEN_PACK; i++) {
				qo_ptr = &p_ptr->inventory[i];
				if (!qo_ptr->k_idx || !qo_ptr->questor) continue;
				if (qo_ptr->quest != o_ptr->quest || qo_ptr->questor_idx != o_ptr->questor_idx) continue;
				msg_print(Ind, "\377yYou cannot carry more than one object of this type!");
				return;
			}
		}

#ifdef NEWBIES_CANT_GRAB_IN_BREE
		/* Avoid people picking up things that they cannot drop again? */
		if (in_bree(wpos) &&
		    p_ptr->max_plv < cfg.newbies_cannot_drop && o_ptr->owner && p_ptr->id != o_ptr->owner
		    && p_ptr->max_plv < o_ptr->level /* added this to allow giving revived level 1 newbies basic items which they lost, to avoid suicide */
		    ) {
			msg_format(Ind, "You cannot take items from other people in Bree until you are level %d.", cfg.newbies_cannot_drop);
			if (!is_admin(p_ptr)) return;
		}
#endif

		if (p_ptr->inval && o_ptr->owner && p_ptr->id != o_ptr->owner) {
			if (exceptionally_shareable_item(o_ptr)) {
				//o_ptr->number = 1;
				o_ptr->discount = 100;
				if (o_ptr->level <= p_ptr->lev) {
					if (!o_ptr->level) o_ptr->owner = p_ptr->id;
					o_ptr->mode = p_ptr->mode;
				}
			} else {
				msg_print(Ind, "\377oYou cannot take items of other players without a valid account.");
				return;
			}
		}

		if (compat_pomode(Ind, o_ptr)) {
			/* Make an exception for WoR scrolls in case of rescue missions (become 100% off tho) */
			if (exceptionally_shareable_item(o_ptr)) {
				//o_ptr->number = 1;
				o_ptr->discount = 100;
				if (o_ptr->level <= p_ptr->lev) {
					if (!o_ptr->level) o_ptr->owner = p_ptr->id;
					o_ptr->mode = p_ptr->mode;
				}
			/* Game pieces are free to be used */
			} else if (o_ptr->tval == TV_GAME) {
				if (o_ptr->level <= p_ptr->lev) {
					if (!o_ptr->level) o_ptr->owner = p_ptr->id;
					o_ptr->mode = p_ptr->mode;
				}
			/* exception for amulet of the highlands for tournaments */
			} else if (o_ptr->tval == TV_AMULET &&
			    (o_ptr->sval == SV_AMULET_HIGHLANDS || o_ptr->sval == SV_AMULET_HIGHLANDS2)) {
				o_ptr->mode = p_ptr->mode;
			} else {
				msg_format(Ind, "You cannot take items of %s players.", compat_pomode(Ind, o_ptr));
				if (!is_admin(p_ptr)) return;
			}
		}

		/* prevent winners picking up true arts accidentally */
		if (true_artifact_p(o_ptr) && !winner_artifact_p(o_ptr) &&
		    p_ptr->total_winner && cfg.kings_etiquette) {
			msg_print(Ind, "Royalties may not own true artifacts!");
			if (!is_admin(p_ptr)) return;
		}

		if ((p_ptr->mode & MODE_DED_IDDC) && !in_irondeepdive(&p_ptr->wpos)
#ifdef DED_IDDC_MANDOS
		    && !in_hallsofmandos(&p_ptr->wpos)
#endif
		    ) {
			if (o_ptr->owner && o_ptr->owner != p_ptr->id) {
				msg_print(Ind, "\377yYou cannot pick up someone else's goods or your life would be forfeit.");
				return;
			}
			if (!o_ptr->owner && o_ptr->killer && o_ptr->killer != p_ptr->id) {
				msg_print(Ind, "\377yYou cannot take loot that does not belong to you.");
				if (!is_admin(p_ptr)) return;
			}
			if (o_ptr->no_soloist) {
				msg_print(Ind, "\377yDue to its origin you cannot pick up this item.");
				return;
			}
		}

		if ((k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) &&
#ifdef FALLEN_WINNERSONLY
		    !p_ptr->once_winner
#else
		    !p_ptr->total_winner
#endif
		    ) {
			msg_print(Ind, "Only royalties are powerful enough to pick up that item!");
			if (!is_admin(p_ptr)) return;
		}

		/* Wrapped gifts: Totally enforce level restrictions */
		if (o_ptr->tval == TV_SPECIAL && o_ptr->sval >= SV_GIFT_WRAPPING_START && o_ptr->sval <= SV_GIFT_WRAPPING_END
		    && o_ptr->owner && o_ptr->owner != p_ptr->id && p_ptr->lev < o_ptr->level) {
			msg_print(Ind, "Your level must at least be the same as the gift in order to pick it up.");
			if (!is_admin(p_ptr)) return;
		}

#ifdef ENABLE_DEMOLITIONIST
		if (o_ptr->tval == TV_CHARGE && o_ptr->timeout) {
			msg_print(Ind, "\377yYou must disarm the charge before picking it up!");
			if (!is_admin(p_ptr)) return;
		}
#endif

/*#ifdef RPG_SERVER -- let's do this also for normal server */
#if 1
		/* Turn level 0 food into level 1 food - mikaelh */
		if (o_ptr->owner && o_ptr->owner != p_ptr->id && o_ptr->level == 0 &&
		    shareable_starter_item(o_ptr)) {
			o_ptr->level = 1;
			o_ptr->discount = 100;
		}
#endif

/* the_sandman: item lvl restrictions are disabled in rpg */
#ifndef RPG_SERVER
		if (o_ptr->owner && o_ptr->owner != p_ptr->id &&
		    (o_ptr->level > p_ptr->lev || o_ptr->level == 0) &&
		    !in_irondeepdive(&p_ptr->wpos)) {
			if (cfg.anti_cheeze_pickup) {
				if (o_ptr->level) {
					msg_format(Ind, "You must be level %d or higher to pick up that item!", o_ptr->level);
					if (!is_admin(p_ptr)) return;
				}
 #if 1 /* doesn't matter probably? Food exchange was already done above. */
				else {
					msg_print(Ind, "You cannot pick up a zero-level item that doesn't belong to you.");
					if (!is_admin(p_ptr)) return;
				}
 #endif
			/* new: this is to prevent newbies to pick up all nearby stuff with their
			   level 1 char aimlessly without being able to drop it again. */
			} else if (p_ptr->max_plv < cfg.newbies_cannot_drop) {
				msg_format(Ind, "You must at least be level %d to pick up items above your level.", cfg.newbies_cannot_drop);
				if (!is_admin(p_ptr)) return;
			}
 #if 1
			/* this is for a similar purpose: in the inn, don't allow picking up items that we can't immediately use */
			else if ((f_info[c_ptr->feat].flags1 & FF1_PROTECTED) && p_ptr->lev < o_ptr->level
			    && !in_irondeepdive(&p_ptr->wpos)) {
				msg_print(Ind, "Inside an inn you cannot pick up items that are higher level than you.");
				if (!is_admin(p_ptr)) return;
			}
 #endif
			else if (true_artifact_p(o_ptr) && cfg.anti_arts_pickup)
			//else if (artifact_p(o_ptr) && cfg.anti_arts_pickup)
			{
				if (o_ptr->level == 0) msg_print(Ind, "You cannot pick up a zero-level artifact that you don't own.");
				else msg_format(Ind, "You must be level %d or higher to pick up that artifact!", o_ptr->level);
				if (!is_admin(p_ptr)) return;
			}
			else if (o_ptr->tval == TV_JUNK && o_ptr->sval == SV_GLASS_SHARD) {
				msg_print(Ind, "You cannot seem to grasp the shard.");
				if (!is_admin(p_ptr)) return;
			}
		}
#endif

		/* Even if it's not guaranteed yet that we can pick up this item, just clear the newloot flag already anyway, it did enough... */
		o_ptr->mode &= ~MODE_NEWLOOT_ITEM;

		/* Save old inscription in case pickup fails */
		old_note = o_ptr->note;
		old_note_utag = o_ptr->note_utag;

		/* Remove dangerous inscriptions when picking up items owned by other players - mikaelh */
		if (p_ptr->id != o_ptr->owner && o_ptr->note && p_ptr->clear_inscr) {
			char *inscription, *scan;
			scan = inscription = strdup(quark_str(o_ptr->note));

			while (*scan != '\0') {
				if (*scan == '@'
				    /* exempt power-inscriptions: */
				    && *(scan + 1) != '&' && *(scan + 1) != '^') {
					/* Replace @ with space */
					*scan = ' ';
				}
				scan++;
			}

			o_ptr->note = quark_add(inscription);
			free(inscription);
		}

		if (p_ptr->auto_untag && o_ptr->note_utag && o_ptr->note) {
			/* copy-pasted from slash.c '/untag' command basically */
			char note2[80];
			int j = strlen(quark_str(o_ptr->note)) - o_ptr->note_utag;

			if (j >= 0) { /* bugfix hack */
				//s_printf("j: %d, strlen: %d, note_utag: %d, i: %d.\n", j, strlen(quark_str(o_ptr->note)), o_ptr->note_utag, i);
				strncpy(note2, quark_str(o_ptr->note), j);
				if (j > 0 && note2[j - 1] == '-') j--; /* absorb '-' orphaned spacers */
				note2[j] = 0; /* terminate string */
				o_ptr->note_utag = 0;
				if (note2[0]) o_ptr->note = quark_add(note2);
				else o_ptr->note = 0;
			} else o_ptr->note_utag = 0; //paranoia?
		}

		/* Try to add to the quiver */
		if (object_similar(Ind, o_ptr, &p_ptr->inventory[INVEN_AMMO], 0x0)) {
			//note: 'pick_one' is not implemented here!
			int slot = INVEN_AMMO, num = o_ptr->number;

			msg_print(Ind, "You add the ammo to your quiver.");

			/* Check whether this item was requested by an item-retrieval quest */
			if (p_ptr->quest_any_r_within_target) quest_check_goal_r(Ind, o_ptr);

			/* Get the item again */
			o_ptr = &(p_ptr->inventory[slot]);

			o_ptr->number += num;
			p_ptr->total_weight += num * o_ptr->weight;

			/* Describe the object */
			object_desc(Ind, o_name, o_ptr, TRUE, 3);
			o_ptr->marked = 0;
			o_ptr->marked2 = ITEM_REMOVAL_NORMAL;

			/* Message */
			msg_format(Ind, "You have %s (%c).", o_name, index_to_label(slot));

			/* Delete original */
			//delete_object(wpos, p_ptr->py, p_ptr->px, FALSE);
			delete_object_idx(c_ptr->o_idx, FALSE, FALSE);

			/* Hack -- tell the player of the next object on the pile */
			whats_under_your_feet(Ind, FALSE);

			/* Tell the client */
			//Send_floor(Ind, 0);

			/* Note the spot */
			note_spot_depth(wpos, p_ptr->py, p_ptr->px);

			/* Draw the spot */
			everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);

			/* Refresh */
			p_ptr->window |= PW_EQUIP;

			try_pickup = FALSE;
		}
		/* Try to add to the empty quiver (XXX rewrite me - too long!) */
		else if (auto_load && is_ammo(o_ptr->tval) &&
		    !p_ptr->inventory[INVEN_AMMO].k_idx) {
			u32b f1, f2, f3, f4, f5, f6, esp;
			//note: 'pick_one' is not implemented here!
			int slot = INVEN_AMMO;

			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

			msg_print(Ind, "You put the ammo into your quiver.");

			o_ptr->marked = 0;
			o_ptr->marked2 = ITEM_REMOVAL_NORMAL;

			/* Auto Curse */
			if (f3 & TR3_AUTO_CURSE) {
				/* The object recurse itself ! */
				if (!(o_ptr->ident & ID_CURSED)) {
					o_ptr->ident |= ID_CURSED;
					s_printf("AUTO_RECURSE(1): %s : %d,%d,%d,%d (%d)\n", p_ptr->name, o_ptr->tval, o_ptr->sval, o_ptr->bpval, o_ptr->pval, slot);
				}
			}

			/* Cursed! */
			if (cursed_p(o_ptr)) {
#ifdef VAMPIRES_INV_CURSED
				if (p_ptr->prace == RACE_VAMPIRE) inverse_cursed(o_ptr);
 #ifdef ENABLE_HELLKNIGHT
				else if (p_ptr->pclass == CLASS_HELLKNIGHT) inverse_cursed(o_ptr); //them too!
 #endif
 #ifdef ENABLE_CPRIEST
				else if (p_ptr->pclass == CLASS_CPRIEST && p_ptr->body_monster == RI_BLOODTHIRSTER) inverse_cursed(o_ptr);
 #endif
#endif
				if (cursed_p(o_ptr)) { //in case INVERSE_CURSED_RANDARTS triggered
					/* Warn the player */
					msg_print(Ind, "Oops! It feels deathly cold!");
					/* Note the curse */
					o_ptr->ident |= ID_SENSE | ID_SENSED_ONCE;

					note_toggle_cursed(o_ptr, TRUE);
				}
			}

			/* Structure copy to insert the new item */
			p_ptr->inventory[slot] = (*o_ptr);

			/* Forget the old location */
			p_ptr->inventory[slot].iy = p_ptr->inventory[slot].ix = 0;
			p_ptr->inventory[slot].wpos.wx = 0;
			p_ptr->inventory[slot].wpos.wy = 0;
			p_ptr->inventory[slot].wpos.wz = 0;
			/* Clean out unused fields */
			p_ptr->inventory[slot].next_o_idx = 0;
			p_ptr->inventory[slot].held_m_idx = 0;


			/* Increase the weight, prepare to redraw */
			p_ptr->total_weight += (o_ptr->number * o_ptr->weight);

			/* Get the item again */
			o_ptr = &(p_ptr->inventory[slot]);

			/* Describe the object */
			object_desc(Ind, o_name, o_ptr, TRUE, 3);
			o_ptr->marked = 0;
			o_ptr->marked2 = ITEM_REMOVAL_NORMAL;

			/* Message */
			msg_format(Ind, "You have %s (%c).", o_name, index_to_label(slot));

			/* Delete original */
			//delete_object(wpos, p_ptr->py, p_ptr->px, FALSE);
			delete_object_idx(c_ptr->o_idx, FALSE, FALSE);

			/* Hack -- tell the player of the next object on the pile */
			whats_under_your_feet(Ind, FALSE);

			/* Tell the client */
			Send_floor(Ind, 0);

			/* Recalculate boni */
			p_ptr->update |= (PU_BONUS);

			/* Window stuff */
			p_ptr->window |= (PW_EQUIP);

			try_pickup = FALSE;
		}
		/* Boomerangs: */
		else if (auto_load && o_ptr->tval == TV_BOOMERANG &&
		    !p_ptr->inventory[INVEN_BOW].k_idx && item_tester_hook_wear(Ind, INVEN_BOW)) {
			u32b f1, f2, f3, f4, f5, f6, esp;
			int slot = INVEN_BOW;

			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
			msg_print(Ind, "You ready the boomerang.");

			o_ptr->marked = 0;
			o_ptr->marked2 = ITEM_REMOVAL_NORMAL;

			/* Auto Curse */
			if (f3 & TR3_AUTO_CURSE) {
				/* The object recurse itself ! */
				if (!(o_ptr->ident & ID_CURSED)) {
					o_ptr->ident |= ID_CURSED;
					s_printf("AUTO_RECURSE(2): %s : %d,%d,%d,%d (%d)\n", p_ptr->name, o_ptr->tval, o_ptr->sval, o_ptr->bpval, o_ptr->pval, slot);
				}
			}

			/* Cursed! */
			if (cursed_p(o_ptr)) {
#ifdef VAMPIRES_INV_CURSED
				if (p_ptr->prace == RACE_VAMPIRE) inverse_cursed(o_ptr);
 #ifdef ENABLE_HELLKNIGHT
				else if (p_ptr->pclass == CLASS_HELLKNIGHT) inverse_cursed(o_ptr); //them too!
 #endif
 #ifdef ENABLE_CPRIEST
				else if (p_ptr->pclass == CLASS_CPRIEST && p_ptr->body_monster == RI_BLOODTHIRSTER) inverse_cursed(o_ptr);
 #endif
#endif
				if (cursed_p(o_ptr)) { //in case INVERSE_CURSED_RANDARTS triggered
					/* Warn the player */
					msg_print(Ind, "Oops! It feels deathly cold!");
					/* Note the curse */
					o_ptr->ident |= ID_SENSE | ID_SENSED_ONCE;

					note_toggle_cursed(o_ptr, TRUE);
				}
			}

			/* Structure copy to insert the new item */
			p_ptr->inventory[slot] = (*o_ptr);

			/* cannot equip more than one at once */
			p_ptr->inventory[slot].number = 1;
			o_ptr->number--;
			if (!o_ptr->number) delete_it = TRUE;
			else delete_it = FALSE;

			/* Forget the old location */
			p_ptr->inventory[slot].iy = p_ptr->inventory[slot].ix = 0;
			p_ptr->inventory[slot].wpos.wx = 0;
			p_ptr->inventory[slot].wpos.wy = 0;
			p_ptr->inventory[slot].wpos.wz = 0;
			/* Clean out unused fields */
			p_ptr->inventory[slot].next_o_idx = 0;
			p_ptr->inventory[slot].held_m_idx = 0;


			/* Increase the weight, prepare to redraw */
			p_ptr->total_weight += o_ptr->weight;

			/* Get the item again */
			o_ptr = &(p_ptr->inventory[slot]);

			/* Describe the object */
			object_desc(Ind, o_name, o_ptr, TRUE, 3);
			o_ptr->marked = 0;
			o_ptr->marked2 = ITEM_REMOVAL_NORMAL;

			/* Message */
			msg_format(Ind, "You have %s (%c).", o_name, index_to_label(slot));

			/* Delete original */
			//delete_object(wpos, p_ptr->py, p_ptr->px, FALSE);
			if (delete_it) {
				delete_object_idx(c_ptr->o_idx, FALSE, FALSE);

				/* Hack -- tell the player of the next object on the pile */
				whats_under_your_feet(Ind, FALSE);

				/* Tell the client */
				Send_floor(Ind, 0);

				try_pickup = FALSE;
			} else if (!pick_one) {
				/* ^ if we didn't delete it, additionally try to pick up the rest of the pile */
				o_ptr = &o_list[c_ptr->o_idx];
			} else try_pickup = FALSE; //we only wanted to pick up one anyway, which we put into our bow slot now

			/* Recalculate boni */
			p_ptr->update |= (PU_BONUS);

			/* Recalculate mana */
			p_ptr->update |= (PU_MANA | PU_HP | PU_SANITY);

			/* Redraw */
			p_ptr->redraw |= (PR_PLUSSES | PR_ARMOR);

			/* Window stuff */
			p_ptr->window |= (PW_EQUIP | PW_PLAYER);
		}

#ifdef ENABLE_SUBINVEN
		num_org = o_ptr->number; /* For !g/!G inscription on the target item, to keep track whether we actually auto-stowed any of it. */

		/* Try to put into a specialized bag automatically -- note that this currently means that apply_XID() isn't called (which cannot handle subinventory items atm anyway) */
		switch (o_ptr->tval) {
		case TV_CHEMICAL: /* DEMOLITIONIST stuff */
			stowed = auto_stow(Ind, SV_SI_SATCHEL, o_ptr, c_ptr->o_idx, pick_one_org, FALSE, FALSE, c_ptr->info);
			break;
		case TV_TRAPKIT:
			stowed = auto_stow(Ind, SV_SI_TRAPKIT_BAG, o_ptr, c_ptr->o_idx, pick_one_org, FALSE, FALSE, c_ptr->info);
			break;
		case TV_ROD:
			/* Note that this returns FALSE too if rod is of a flavour yet unknown to the player, covering that case on the fly! :) */
			if (rod_requires_direction(Ind, o_ptr)) break;
			/* Fall through */
		case TV_STAFF:
			stowed = auto_stow(Ind, SV_SI_MDEVP_WRAPPING, o_ptr, c_ptr->o_idx, pick_one_org, FALSE, FALSE, c_ptr->info);
			break;
		case TV_POTION: case TV_POTION2: case TV_BOTTLE:
			stowed = auto_stow(Ind, SV_SI_POTION_BELT, o_ptr, c_ptr->o_idx, pick_one_org, FALSE, FALSE, c_ptr->info);
			break;
		case TV_FOOD:
		case TV_FIRESTONE:
			stowed = auto_stow(Ind, SV_SI_FOOD_BAG, o_ptr, c_ptr->o_idx, pick_one_org, FALSE, FALSE, c_ptr->info);
			break;
		}
		if (stowed) try_pickup = pick_one = FALSE; //ensure to not trigger the number = 1 hack for pick_one (!)

		/* If the item contains a !G or !g inscription and was already auto-stowed at least partially, and we're not using pick_all, do not pick up any more of it! */
		if (o_ptr->number != num_org && (check_guard_inscription(o_ptr->note, 'g') || check_guard_inscription(o_ptr->note, 'G'))) try_pickup = FALSE;
#endif

		/* After the number might have been reduced by auto_stow() above: */
		num_org = o_ptr->number; /* Hack in case 'pick_one' or 'pick_some' are invoked */

		/* hack for 'pick_one' (needed for inven_carry_okay() too, or rather for the object_similar() check inside of it */
		if (pick_one) o_ptr->number = 1;

		/* If we haven't picked up the item yet (ie ammo slot or subinventory), try normal inventory now */
		if (try_pickup) {
			object_type forge, *o_floor_ptr; //for creating a structure copy to 'forge' for 'pick_one'/'pick_some' hacks
			int slot;

			/* Note that the pack is too full - here we check for !Gn inscription for specific amount, via 0x20 tolerance marker */
			if (!(pick_some = inven_carry_okay(Ind, o_ptr, (pickup == 2) ? 0x20 : 0x0))) {
				msg_format(Ind, "You have no room for %s.", o_name);
				Send_floor(Ind, o_ptr->tval);

				/* Restore old inscription */
				o_ptr->note = old_note;
				o_ptr->note_utag = old_note_utag;

				/* unhack 'pick_one' */
				o_ptr->number = num_org;

				return;
			}

			/* New: Also check the item to pick up for !G inscription, for when we don't have any of it in our inventory yet,
			   to preemptively apply that value to the amount we probably want to pick up (maybe make it a client option).
			   Note: We only check the inven_carry_okay() return value for '-1' for 'no more space'.
			    If there was an existing !G inscription in our inventory, we'll accept that and not process the possible object's own !G inscription here. */
			if (!pick_all && pick_some == -1 && (limitG = check_guard_inscription(o_ptr->note, 'G') - 1) > 0)
				if (limitG < o_ptr->number) pick_some = limitG;

			/* Actually ensure that there is at least one slot left in case we filled the whole inventory with CURSE_NO_DROP items */
			if (pick_some == -1 && !inven_carry_cursed_okay(Ind, o_ptr, 0x0)) {
				/* Give a somewhat misleading message, to not spoil him that he actually was protected */
				msg_print(Ind, "A divine force stops you from picking up that item!");
				s_printf("NO_PICKUP_CURSE_NO_DROP: Player '%s', item '%s'.\n", p_ptr->name, o_name);
				Send_floor(Ind, o_ptr->tval);

				/* Restore old inscription */
				o_ptr->note = old_note;
				o_ptr->note_utag = old_note_utag;

				/* unhack 'pick_one' */
				o_ptr->number = num_org;

				return;
			}

			/* Pick up the item (if requested and allowed) */
			o_floor_ptr = o_ptr;

			/* Use i as check just for 'normal picking up' (0) vs 'pickup specific amount' (>0) from now on,
			   so we discard the third value here (-1) as we have already checked for 'no room' above (was '0')
			   and therefore no longer need the distinction of -1 and 0. */
			if (pick_some == -1) pick_some = 0;
			else if (pick_some) o_ptr->number = pick_some;

			/* hack 'pick_one' */
			if (pick_one) {
				forge = (*o_floor_ptr);
				delete_it = (num_org == 1);
				/* use the new temporary forge object for reference */
				o_ptr = &forge;
			/* We re-use/clone the 'pick_one' hack for the new pick_some aka !Gn inscription with n specified (and > 0) */
			} else if (pick_some) {
				forge = (*o_floor_ptr);
				delete_it = (num_org == pick_some);
				/* use the new temporary forge object for reference */
				o_ptr = &forge;
			} else delete_it = TRUE; //delete the object from the floor, sinc we fully picked it up (in case of stack of items)

			/* Attempt to pick up an object. */

			/* For pick_one/pick_some (otherwise delete_it would be TRUE here):
			   Need to divide wand/staff charges - thanks Dj_Wolf */
			if (!delete_it && is_magic_device(o_ptr->tval)) {
				o_floor_ptr->number = num_org; //temporarily unhack pick_one/pick_some
				if (pick_one) {
					divide_charged_item(o_ptr, o_floor_ptr, 1);
					o_floor_ptr->number = 1; //rehack pick_one
				} else if (pick_some) {
					divide_charged_item(o_ptr, o_floor_ptr, pick_some);
					o_floor_ptr->number = pick_some; //rehack pick_some
				}
			}

			handle_pickup_item(Ind, o_ptr, c_ptr->info);

			/* Carry the item */
			o_ptr->quest_credited = TRUE; //hack: avoid double-crediting
			/* 'Auto-load' throwing weapons? To make it fair, works on all weapons. Ez disarm-recovery possible! */
			if (!(auto_load && is_weapon(o_ptr->tval) && object_known_p(Ind, o_ptr) && (slot = do_cmd_wield(Ind, -c_ptr->o_idx, 4)) != -1))
			/* Normal pick-up */
			slot = inven_carry(Ind, o_ptr);
#ifdef USE_SOUND_2010
			/* We already carried the item, don't play another (->redundant) sfx later! */
			inven_carried = TRUE;
#endif
			o_ptr->quest_credited = FALSE; //unhack.

			/* Get the item again */
			o_ptr = &(p_ptr->inventory[slot]);
			o_ptr->marked = 0;
			o_ptr->marked2 = ITEM_REMOVAL_NORMAL;

#if 0
			if (!o_ptr->level) {
				if (p_ptr->dun_depth > 0) o_ptr->level = p_ptr->dun_depth;
				else o_ptr->level = -p_ptr->dun_depth;
				if (o_ptr->level > 100) o_ptr->level = 100;
			}
#endif

			/* the_sandman: attempt to id a newly picked up item if we have the means to do so.
			 * Check that we don't know the item and can read a scroll - mikaelh */
			if (!object_aware_p(Ind, o_ptr) || !object_known_p(Ind, o_ptr)) /* was just object_known_p */
				apply_XID(Ind, o_ptr, slot);

			/* Describe the object */
			object_desc(Ind, o_name, o_ptr, TRUE, 3);

			if (!remember_sense(Ind, slot, o_ptr)) {
				/* Just standard message */
				msg_format(Ind, "You have %s (%c).", o_name, index_to_label(slot));
			}

			if (!p_ptr->warning_lamp_oil && o_ptr->tval == TV_LITE && o_ptr->sval == SV_LITE_LANTERN
			    && p_ptr->inventory[INVEN_LITE].tval == TV_LITE && p_ptr->inventory[INVEN_LITE].sval == SV_LITE_LANTERN) {
				msg_print(Ind, "\374\377yHINT: You can also refill your lantern with another lantern with \377oSHIFT+F\377y,");
				msg_print(Ind, "\374\377y      and the chat command '\377o/empty\377y' can empty lanterns to make them stack.");
				s_printf("warning_lamp_oil: %s\n", p_ptr->name);
				p_ptr->warning_lamp_oil = 1;
			}

			if (!p_ptr->warning_inspect &&
			    (o_ptr->tval == TV_RING || o_ptr->tval == TV_AMULET)// || o_ptr->tval == TV_WAND || o_ptr->tval == TV_STAFF || o_ptr->tval == TV_ROD)
			    && object_known_p(Ind, o_ptr)
			    //&& *(k_text + k_info[o_ptr->k_idx].text) /* not this, it disables all 'basic' items such as sustain rings or example */
			    ) {
				msg_print(Ind, "\374\377yHINT: You can press '\377oShift+i\377y' to try and inspect an unknown item!");
				s_printf("warning_inspect: %s\n", p_ptr->name);
				p_ptr->warning_inspect = 1;
			}

			/* guild key? */
			if (o_ptr->tval == TV_KEY && o_ptr->sval == SV_GUILD_KEY) {
				if (o_ptr->pval == p_ptr->guild && !lookup_player_name(guilds[p_ptr->guild].master)) {
					if (p_ptr->lev < 30) msg_print(Ind, "\377yYou need to be at least level 30 to become a guild master.");
					else {
						/* anti-cheeze: People could get an extra house on each character.
						   So we allow only one guild master per player account to at least
						   reduce the nonsense to 1 extra house per Account.. */
						struct account acc;

						bool success = GetAccount(&acc, p_ptr->accountname, NULL, FALSE, NULL, NULL);

						/* paranoia */
						if (success) {
							int *id_list, ids, i, j;
							bool ok = TRUE;

							ids = player_id_list(&id_list, acc.id);
							for (i = 0; i < ids; i++) {
								if ((j = lookup_player_guild(id_list[i])) && /* one of his characters is in a guild.. */
								    guilds[j].master == id_list[i]) { /* ..and he is actually the master of that guild? */
									msg_print(Ind, "\377yOnly one character per account is allowed to be a guild master.");
									ok = FALSE;
									break;
								}
							}
							if (ids) C_KILL(id_list, ids, int);

							if (ok) {
								/* set guild hall to 'no longer suspended' */
								if ((i = guilds[p_ptr->guild].h_idx)) {
									houses[i - 1].flags &= ~HF_GUILD_SUS;
									fill_house(&houses[i - 1], FILL_GUILD_SUS_UNDO, NULL);
								}
								guilds[p_ptr->guild].timeout = 0; /* phew */

								guild_msg_format(p_ptr->guild, "\374\377%c%s is the new guild master!", COLOUR_CHAT_GUILD, p_ptr->name);
								guilds[p_ptr->guild].master = p_ptr->id;
								/* hack: change guild hall creator id to him */
								if (guilds[p_ptr->guild].h_idx) houses[guilds[p_ptr->guild].h_idx - 1].dna->creator = p_ptr->dna;
								Send_guild(Ind, FALSE, FALSE);
								Send_guild_config(p_ptr->guild);
							}
						}
					}
				}
			}

			if (o_ptr->tval == TV_SUBINVEN) {
				if (is_older_than(&p_ptr->version, 4, 8, 0, 0, 0, 0))
					msg_print(Ind, "\377oYou need to use at least client version \377R4.8.0\377o to use this bag! Your current client won't work!");
				if (o_ptr->sval == SV_SI_POTION_BELT && !is_newer_than(&p_ptr->version, 4, 9, 1, 0, 0, 0))
					msg_print(Ind, "\377oYou need to use at least the \377RTEST client 4.9.1\377o or a higher client version to use this bag! Your current client won't work!");
#if 0 /* Maybe spammy/unnecessary */
#ifdef SI_WRAPPING_SKILL
				if (o_ptr->sval == SV_SI_MDEVP_WRAPPING && p_ptr->newbie_hints && get_skill(p_ptr, SKILL_DEVICE) < SI_WRAPPING_SKILL && get_skill(p_ptr, SKILL_TRAPPING) < SI_WRAPPING_SKILL)
					msg_format(Ind, "\377yYou need %d in 'Magic Device' or 'Trapping' skill to use antistatic wrappings!", SI_WRAPPING_SKILL);
#endif
#endif
			}

			/* Delete original */
			//delete_object(wpos, p_ptr->py, p_ptr->px, FALSE);
			if (delete_it) {
				/* extra logging for artifacts */
				if (o_ptr->name1) {
					char o_name[ONAME_LEN];

					object_desc_store(0, o_name, o_ptr, TRUE, 3);
					s_printf("%s artifact picked up at (%d,%d,%d) by '%s':\n  %s\n",
					    showtime(), wpos->wx, wpos->wy, wpos->wz,
					    p_ptr->name, o_name);
				}

				delete_object_idx(c_ptr->o_idx, FALSE, FALSE);

				/* Hack -- tell the player of the next object on the pile */
				whats_under_your_feet(Ind, FALSE);

				/* Tell the client */
				Send_floor(Ind, 0);
			} else if (pick_one) /* unhack 'pick_one' - we picked up one item off the pile */
				o_floor_ptr->number = num_org - 1;
			else if (pick_some)
				o_floor_ptr->number = num_org - pick_some;
		} /* try_pickup */
	}

#ifdef USE_SOUND_2010
	/* hack: inven_carry() also calls sound_item()! */
	if (!inven_carried) sound_item(Ind, o_ptr->tval, o_ptr->sval, "pickup_");
#endif

	/* splash! harm items */
	if (feat_is_deep_water(c_ptr->feat) && magik(WATER_ITEM_DAMAGE_CHANCE)
	    && !p_ptr->levitate && !(p_ptr->body_monster && (r_info[p_ptr->body_monster].flags7 & RF7_AQUATIC)) // && !p_ptr->tim_wraith
	    && !p_ptr->immune_water && !(p_ptr->resist_water && magik(50))) {
		if (TOOL_EQUIPPED(p_ptr) != SV_TOOL_TARPAULIN
		    && !magik(get_skill_scale(p_ptr, SKILL_SWIM, 4900)))
			inven_damage(Ind, set_water_destroy, 1);
		equip_damage(Ind, GF_WATER);
	}

	break_cloaking(Ind, 5);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
}

/* forcibly stack a level 0 item with normal-level items in your inventory */
void do_cmd_force_stack(int Ind, int item) {
	player_type *p_ptr = Players[Ind];

	p_ptr->current_force_stack = item + 1;
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);
}

#if 0
/*
 * Determine if a trap affects the player.
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match trap power against player armor.
 */
static int check_hit(int Ind, int power) {
	player_type *p_ptr = Players[Ind];
	int k, ac;

	/* Percentile dice */
	k = rand_int(100);

	/* Hack -- 5% hit, 5% miss */
	if (k < 10) return(k < 5);

	/* Paranoia -- No power */
	if (power <= 0) return(FALSE);

	/* Total armor */
	ac = p_ptr->ac + p_ptr->to_a;

	/* Power competes against Armor */
	if (randint(power) > ((ac * 3) / 4)) return(TRUE);

	/* Assume miss */
	return(FALSE);
}
#endif // if 0

/*
 * Handle player hitting a real trap
 */
void hit_trap(int Ind) {
	player_type *p_ptr = Players[Ind];
	int t_idx;
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	struct c_special *cs_ptr;
	cave_type *c_ptr;
	bool ident = FALSE;

	/* Ghosts ignore traps */
	if ((p_ptr->ghost) || (p_ptr->tim_traps) || (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST)) return;

	/* Disturb the player */
	disturb(Ind, 0, 0);

	/* Get the cave grid */
	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* PvP: Monster traps and runes of hostile players can affect us too */
	if ((cs_ptr = GetCS(c_ptr, CS_MON_TRAP))) {
		py_hit_trap(Ind);
		return;
	}
	if ((cs_ptr = GetCS(c_ptr, CS_RUNE))) {
		py_warding_rune_break(Ind);
		return;
	}

	if (!(cs_ptr = GetCS(c_ptr, CS_TRAPS))) return;
	t_idx = cs_ptr->sc.trap.t_idx;

	if (t_idx) {
		ident = player_activate_trap_type(Ind, p_ptr->py, p_ptr->px, NULL, -1);
		if (ident && !p_ptr->trap_ident[t_idx]) {
			p_ptr->trap_ident[t_idx] = TRUE;
			msg_format(Ind, "You identified the trap as %s.", t_name + t_info[t_idx].name);
		}
	}
}



/*
 * Player attacks another player!
 *
 * If no "weapon" is available, then "punch" the player one time.
 */
/*
 * NOTE: New attacking features from PernAngband are not
 * implemented yet for pvp!! (FIXME)		- Jir -
 *
 * Note: old == TRUE if not auto-retaliating actually
 *       (important for dual-backstab treatment) - C. Blue
 */
/* TODO: q_ptr/p_ptr->name should be replaced by strings made by player_desc */
//note: we assume that p_ptr->num_blow isn't 0 (div/0)
static void py_attack_player(int Ind, int y, int x, byte old) {
	player_type *p_ptr = Players[Ind];
	int num = 0, bonus, chance, slot;
	int k, k2, k3;
	long int kl;
	player_type *q_ptr;
	object_type *o_ptr = NULL;
#if defined(WIELD_BOOKS) || defined(WIELD_DEVICES)
	object_type forge_zero = { 0 }; /* Simulate an empty inventory slot, specifically 'wield' slot. */
#endif
	char q_name[NAME_LEN], hit_desc[MAX_CHARS_WIDE];
	bool do_quake = FALSE;
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	cave_type *c_ptr;
	int fear_chance;
	bool pierced;

	int		bs_skill = get_skill_scale(p_ptr, SKILL_BACKSTAB, 50); //for pvp it's 1/2 of pve (100 there)
	bool		stab_skill = (bs_skill != 0 && !p_ptr->rogue_heavyarmor);
	bool		sleep_stab = TRUE, cloaked_stab = (p_ptr->cloaked == 1), shadow_stab = (p_ptr->shadow_running); /* can player backstab the monster? */
	bool		backstab = FALSE, stab_fleeing = FALSE; /* does player backstab the player? */
	bool		primary_wield, secondary_wield, dual_wield;
	int		dual_stab;
	bool		martial = FALSE;

	int		vorpal_cut = 0;
	int		chaos_effect = 0;
	int		vampiric_melee;
	bool		drain_msg = TRUE;
	int		drain_result = 0, drain_heal = 0, drain_frac;
	int		drain_left = MAX_VAMPIRIC_DRAIN;
	bool		drainable = TRUE, backstab_feed = FALSE;
	//bool		py_slept;
	bool		no_pk;
#ifndef NEW_DODGING
	int		dodge_chance;
#endif
#ifdef ENABLE_BLOOD_FRENZY
	bool		blood_frenzy_ok, bleeding = TRUE;
	int		blood_frenzy_step;
#endif

	monster_race *pr_ptr = &r_info[p_ptr->body_monster], *qr_ptr;
	bool apply_monster_effects = TRUE;
	int i, monster_effects;
#ifdef USE_SOUND_2010
	int sfx = 0;
#endif
	u32b monster_effect[6], monster_effect_chosen;
	u32b f1 = 0, f2 = 0, f3 = 0, f4 = 0, f5 = 0, f6 = 0, esp = 0;


	monster_effect[1] = 0;
	monster_effect[2] = 0;
	monster_effect[3] = 0;
	monster_effect[4] = 0;
	monster_effect[5] = 0;

	o_ptr = &p_ptr->inventory[INVEN_WIELD];
	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT && o_ptr->xtra3 & 0x0200)
		primary_wield = is_melee_item(o_ptr->tval2);
	else
		primary_wield = is_melee_item(o_ptr->tval);

	o_ptr = &p_ptr->inventory[INVEN_ARM];
	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT && o_ptr->xtra3 & 0x0200)
		secondary_wield = (o_ptr->tval2 != 0 && o_ptr->tval2 != TV_SHIELD);
	else
		secondary_wield = (o_ptr->tval != 0 && o_ptr->tval != TV_SHIELD);

	dual_wield = primary_wield && secondary_wield && p_ptr->dual_mode; /* Note: primary_wield && secondary_wield == p_ptr->dual_wield (from xtra1.c) actually. */
	dual_stab = (dual_wield ? 1 : 0); /* organizer variable for dual-wield backstab */
#ifdef ENABLE_BLOOD_FRENZY
	blood_frenzy_ok = dual_wield && p_ptr->inventory[INVEN_WIELD].tval == TV_AXE && p_ptr->inventory[INVEN_ARM].tval == TV_AXE && get_skill(p_ptr, SKILL_TRAUMATURGY) >= 17;
	if (blood_frenzy_ok) blood_frenzy_step = ((2 + get_skill_scale(p_ptr, SKILL_TRAUMATURGY, 400) + p_ptr->num_blow - 1) * (15 + p_ptr->num_blow)) / (15 * p_ptr->num_blow);
#endif

	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];
	q_ptr = Players[0 - c_ptr->m_idx];
	qr_ptr = &r_info[q_ptr->body_monster];
	//py_slept = (q_ptr->sleep != 0);
	//py_slept = q_ptr->afk; /* :D - unused though (also, AFK status can't be toggled for this anyway */
	no_pk = ((zcave[p_ptr->py][p_ptr->px].info & CAVE_NOPK) ||
	   (zcave[q_ptr->py][q_ptr->px].info & CAVE_NOPK));

	if (q_ptr->body_monster) {
		if ((qr_ptr->flags3 & RF3_UNDEAD) ||
		    //(qr_ptr->flags3 & RF3_DEMON) ||
		    (qr_ptr->flags3 & RF3_NONLIVING)) {
			drainable = FALSE;
#ifdef ENABLE_BLOOD_FRENZY
			bleeding = (qr_ptr->d_char == 'V');
#endif
		} else if (strchr("EgvwlIFijmxszQX", qr_ptr->d_char)) {
			drainable = FALSE;
#ifdef ENABLE_BLOOD_FRENZY
			bleeding = (qr_ptr->d_char == 'Q');
#endif
		}
	}
#ifdef ENABLE_BLOOD_FRENZY
	bleeding &= !q_ptr->no_cut; //includes 'A' form handling
#endif

	if (p_ptr->melee_timeout_crit_dual && p_ptr->melee_crit_dual != -q_ptr->Ind)
		p_ptr->melee_timeout_crit_dual = 0;

#if 0
	/* May not assault AFK players, sportsmanship ;)
	   Also required for ranged attacks though, hence currently if0'ed - C. Blue */
	if (q_ptr->afk) {
		if (!p_ptr->afk) {
			/* Message, reduce spamming frequency */
			//if (!old)
			if (!(turn % (cfg.fps * 3))) msg_print(Ind, "For sportsmanship you may not assault players who are AFK.");
		}
		return;
	}
#endif

#ifdef ENABLE_OUNLIFE
	/* Attacking on purpose terminates Wraithstep */
	if (p_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1) && !q_ptr->tim_wraith)
		set_tim_wraith(Ind, 0);
#endif

	/* Restrict attacking in WRAITHFORM */
	if (p_ptr->tim_wraith && !q_ptr->tim_wraith) return;

	/* Disturb both players */
	disturb(Ind, 0, 0);
	disturb(0 - c_ptr->m_idx, 0, 0);

	/* Extract name */
	//strcpy(q_name, q_ptr->name);
	player_desc(Ind, q_name, 0 - c_ptr->m_idx, 0);

	/* Track player health */
	if (p_ptr->play_vis[0 - c_ptr->m_idx]) health_track(Ind, c_ptr->m_idx);

	/* If target isn't already hostile toward attacker, make it so */
	if (!check_hostile(0 - c_ptr->m_idx, Ind)) {
		bool result = FALSE;

		/* Make hostile */
		if (Players[0 - c_ptr->m_idx]->pvpexception < 2)
			result = add_hostility(0 - c_ptr->m_idx, p_ptr->name, FALSE, FALSE);

		/* Log it if no blood bond - mikaelh */
		if (!player_list_find(p_ptr->blood_bond, Players[0 - c_ptr->m_idx]->id)
		    && !istown(wpos)) /* avoid logfile spam from Stormbringer in towns */
			s_printf("%s attacked %s (melee; result %d).\n", p_ptr->name, Players[0 - c_ptr->m_idx]->name, result);
	}
	/* Hack -- divided turn for auto-retaliator */
	if (!old) {
		p_ptr->energy -= level_speed(&p_ptr->wpos) / p_ptr->num_blow;
	}

	/* Handle attacker fear */
	if (p_ptr->afraid) {
		/* Message */
		msg_format(Ind, "\377yYou are too afraid to attack %s!", q_name);

		/* Done */
		return;
	}

	if (q_ptr->store_num == STORE_HOME || q_ptr->store_num == STORE_HOME_DUN) {
		/* Message */
		//msg_format(Ind, "\377yYou are too afraid to attack %s!", q_name);

		/* Done */
		return;
	}

	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
	bandage_fails(Ind);
	/* Disturb the player */
	//q_ptr->sleep = 0;

#ifndef KURZEL_PK
	if (cfg.use_pk_rules == PK_RULES_DECLARE) {
		if (!(q_ptr->pkill & PKILL_KILLABLE)) {
			char string[30];

			snprintf(string, 30, "attacking %s", q_ptr->name);
			s_printf("%s attacked defenceless %s\n", p_ptr->name, q_ptr->name);
			if (!imprison(Ind, JAIL_MURDER_KPK, string)) {
				/* This wrath can be too much */
				//take_hit(Ind, randint(p_ptr->lev*30), "wrath of the Gods", 0);
				/* It's prison here :) */
				msg_print(Ind, "{yYou feel yourself bound hand and foot!");
				set_paralyzed(Ind, p_ptr->paralyzed + rand_int(15) + 15);
				return;
			}
			else return;
		}
	}
#endif
	k = drain_left / p_ptr->num_blow;
	/* ..and make up for rounding errors :) */
	drain_left = k + (magik(((drain_left - (k * p_ptr->num_blow)) * 100) / p_ptr->num_blow) ? 1 : 0);

	/* Handle player fear */
	if (p_ptr->afraid) {
		msg_format(Ind, "\377yYou are too afraid to attack %s!", q_name);
		suppress_message = FALSE;
		/* Done */
		return;
	}

	/* Cannot 'stab' with martial-arts */
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS)
	    && !p_ptr->inventory[INVEN_WIELD].k_idx
	    && !p_ptr->inventory[INVEN_ARM].k_idx
#ifndef ENABLE_MA_BOOMERANG
	    && !p_ptr->inventory[INVEN_BOW].k_idx)
#else
	    && p_ptr->inventory[INVEN_BOW].tval != TV_BOW)
#endif
		martial = TRUE;

#if 0
	/* check whether player can be backstabbed */
	//if (!q_ptr->sleep)
	if (!q_ptr->afk) /* :D */
		sleep_stab = FALSE;
	if (q_ptr->backstabbed) {
		cloaked_stab = FALSE; /* a monster can only be backstabbed once, except if it gets resleeped or stabbed while fleeing */
		shadow_stab = FALSE; /* assume monster doesn't fall twice for it */
	}
	if (!sleep_stab && !cloaked_stab && !shadow_stab && !p_ptr->melee_timeout_crit_dual) dual_stab = 0;
#else
	sleep_stab = FALSE;
#endif

	/* Re-check piercing */
	if (p_ptr->piercing_charged) {
		if (p_ptr->cst < 9) {
			msg_print(Ind, "\377oNot enough stamina to execute an assassinating attack.");
			p_ptr->piercing_charged = FALSE;
			p_ptr->piercing = 0;
		} else {
			use_stamina(p_ptr, 9);
		}
	}

#ifdef TARGET_SWITCHING_COST
	/* Hack for 'old' attack (bumping into enemy): Skip one attack. */
	if (old == 2) num++;
#endif

	/* Attack once for each legal blow */
	while (num++ < p_ptr->num_blow) {
#ifdef USE_SOUND_2010
		if (p_ptr->cut_sfx_attack) {
			sfx = (extract_energy[p_ptr->pspeed] / 10) * p_ptr->num_blow;
			if (sfx) {
				p_ptr->count_cut_sfx_attack += 10000 / sfx;
				if (p_ptr->count_cut_sfx_attack >= 250) { /* 100 / 25 = 4 blows per turn */
					p_ptr->count_cut_sfx_attack -= 250;
					if (p_ptr->count_cut_sfx_attack >= 250) p_ptr->count_cut_sfx_attack = 0;
					sfx = 0;
				}
			}
		}
		if (p_ptr->half_sfx_attack && sfx == 0) {
			if (p_ptr->half_sfx_attack_state) sfx = -1;
			p_ptr->half_sfx_attack_state = !p_ptr->half_sfx_attack_state;
		}
#endif

		/* Access the weapon. Added dual-mode check:
		   Only use secondary weapon if we're not in main-hand mode!
		   This is to prevent bad Nazgul accidents where both weapons go poof
		   although character wasn't in dual-mode. - C. Blue */
		if (!primary_wield && secondary_wield && p_ptr->dual_mode) slot = INVEN_ARM;
		else slot = INVEN_WIELD;

		if (dual_wield) {
			switch (dual_stab) {
			case 0: if (magik(50)) slot = INVEN_ARM; break; /* not in a situation to dual-stab */
			case 1:	if (magik(50)) { /* we may dual-stab, randomly pick 1st or 2nd weapon.. */
					slot = INVEN_ARM;
					dual_stab = 3;
				} else {
					dual_stab = 2;
				}
				break;
			case 2: slot = INVEN_ARM; /* and switch to opposite weapon in the second attack.. */
			/* Fall through */
			case 3: dual_stab = 4; break; /* becomes 0 at end of attack, disabling further dual-stabs */
			}
		}

		o_ptr = &p_ptr->inventory[slot];
#if defined(WIELD_BOOKS) || defined(WIELD_DEVICES)
		/* We _are_ wielding an item, but it does _not_ count in any way for melee attacking?
		   (Unlike for exaple a Mage Staff, which does count; it has dice and can be enchanted even.)
		   Then hack it to point to an empty item (same as an empty wield slot): */
		if (!primary_wield && slot == INVEN_WIELD && o_ptr->tval) o_ptr = &forge_zero;
#endif

		/* Manage backstabbing and 'flee-stabbing' */
		//todo: vortices, oozes, elementals, constructs, undead, plants, swarms, some undead maybe, incorporeal undead if you aren't incorporeal
		//..also no-cut
		if (stab_skill && /* Need appropriate melee weapon or martial arts to backstab */
		    p_ptr->mon_vis[c_ptr->m_idx] &&
		    (rogue_armed_melee(o_ptr, p_ptr) ||
		    (martial && (!q_ptr->body_monster || (qr_ptr->body_parts[BODY_HEAD] && qr_ptr->body_parts[BODY_TORSO]))))) {
			if (sleep_stab || cloaked_stab || shadow_stab) { /* Note: Cloaked backstab takes precedence over backstabbing a fleeing monster */
				backstab = TRUE;
				//q_ptr->backstabbed = 1;
			} else if (q_ptr->afraid) {
				stab_fleeing = TRUE;
			}
		}

		f1 = f2 = f3 = f4 = f5 = esp = 0x0;
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
		chaos_effect = 0; // we need this methinks..?

		if ((f4 & TR4_NEVER_BLOW)) {
			msg_print(Ind, "\377yYou can't attack with that weapon.");
			break;
		}

		/* check whether this weapon or we in general, are vampiric */
		if ((slot == INVEN_WIELD && p_ptr->melee_brand && p_ptr->melee_brand_t == TBRAND_VAMPIRIC) ||
		    (slot == INVEN_ARM && p_ptr->melee_brand2 && p_ptr->melee_brand2_t == TBRAND_VAMPIRIC))
			vampiric_melee = 100;
		else if ((f1 & TR1_VAMPIRIC)) vampiric_melee = 100; /* weapon chance is always 100% */
		else vampiric_melee = p_ptr->vampiric_melee; /* non-weapon chance from other items is applied from xtra1.c */
#ifdef TEST_SERVER
		msg_format(Ind, "slot %d vamp %d", slot, vampiric_melee);
#endif

		/* Calculate the "attack quality" */
		bonus = p_ptr->to_h + p_ptr->to_h_melee + o_ptr->to_h;
		chance = (p_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));
		if (p_ptr->blind) chance >>= 1;

		p_ptr->test_attacks++;

		/* Shadow Dispersion: Take precedence even before AC check (aka test_hit_melee()) now. */
		if (q_ptr->dispersion && q_ptr->cst) {
			msg_format(Ind, "\377%c%s disperses around your attack!", COLOUR_BLOCK_PLY, q_name);
			msg_format(0 - c_ptr->m_idx, "\377%cYou disperse around %s's attack!", COLOUR_DODGE_GOOD, p_ptr->name);
			if (magik(q_ptr->dispersion)) use_stamina(q_ptr, 1);
			continue;
		}

		/* TODO: Add 'outer' shields: Kinetic, Spirit, PfE, Invuln; right here (mostly COLOUR_DODGE_GOOD for dodger/COLOUR_DODGE_PLY for observer). */

		/* Test for hit */
		pierced = FALSE;
#ifndef PVP_AC_REDUCTION
		if (p_ptr->piercing || backstab || test_hit_melee(chance, q_ptr->ac + q_ptr->to_a, 1)) {
#else
		//if (p_ptr->piercing || backstab || test_hit_melee(chance, ((q_ptr->ac + q_ptr->to_a) * 2) / 3, 1)) {
		if (p_ptr->piercing || backstab ||
		    test_hit_melee(chance, (q_ptr->ac + q_ptr->to_a) > AC_CAP ?
		    AC_CAP : q_ptr->ac + q_ptr->to_a, 1)) {
#endif
			/* handle 'piercing' countdown */
			if (p_ptr->piercing) {
				pierced = TRUE;
				p_ptr->piercing_charged = FALSE;
				p_ptr->piercing -= 1000 / p_ptr->num_blow;
				/* since above division is rounded down, discard remainder, just to 'clean up' */
				if (p_ptr->piercing < 1000 / p_ptr->num_blow) p_ptr->piercing = 0;
			}

#ifndef NEW_DODGING
 #if 1 /*SKILL_DODGE works in pvp? ^_^" */
			/* 20 dodge vs lvl 20 => 22% max chance
			 * 30 dodge vs lvl 30 => 33% max chance
			 * 45 dodge vs lvl 50 => 40% max chance
			 * 50 dodge vs lvl 50 => 55% max chance
			 * ---- Start to curve down if opponent is way past 50 ----
			 * 50 dodge vs lvl 55 => 45% max chance
			 * 50 dodge vs lvl 60 => 36% max chance
			 * 50 dodge vs lvl 70 => 17% max chance
			 * ---- Level 79+ melee hits are not dodgable ----
			 */
			dodge_chance = q_ptr->dodge_level - p_ptr->lev * 19 / 10;
			if (dodge_chance > DODGE_CAP) dodge_chance = DODGE_CAP;

			if (!backstab && (dodge_chance > 0) && magik(dodge_chance)) {
				msg_format(Ind, "\377c%s dodges your attack!", COLOUR_DODGE_PLY, q_name);
				switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
				case 's': case 'x': case 'z':
					msg_format(0 - c_ptr->m_idx, "\377%cYou dodge %s' attack!", COLOUR_DODGE_GOOD, p_ptr->name);
					break;
				default:
					msg_format(0 - c_ptr->m_idx, "\377%cYou dodge %s's attack!", COLOUR_DODGE_GOOD, p_ptr->name);
				}
  #ifdef USE_SOUND_2010
				if (sfx == 0 && p_ptr->sfx_combat) {
					if (o_ptr->k_idx && is_melee_weapon(o_ptr->tval)
						sound(Ind, "miss_weapon", "miss", SFX_TYPE_ATTACK, FALSE);
					else
						sound(Ind, "miss", NULL, SFX_TYPE_ATTACK, FALSE);
				}
  #endif
				continue;
			}
 #endif
#else /* :-o */
			if (!backstab && magik(apply_dodge_chance(0 - c_ptr->m_idx, p_ptr->lev * 2))) {
				msg_format(Ind, "\377%c%s dodges your attack!", COLOUR_DODGE_PLY, q_name);
				switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
				case 's': case 'x': case 'z':
					msg_format(0 - c_ptr->m_idx, "\377%cYou dodge %s' attack!", COLOUR_DODGE_GOOD, p_ptr->name);
					break;
				default:
					msg_format(0 - c_ptr->m_idx, "\377%cYou dodge %s's attack!", COLOUR_DODGE_GOOD, p_ptr->name);
				}
 #ifdef USE_SOUND_2010
				if (sfx == 0 && p_ptr->sfx_combat) {
					if (o_ptr->k_idx && is_melee_weapon(o_ptr->tval))
						sound(Ind, "miss_weapon", "miss", SFX_TYPE_ATTACK, FALSE);
					else
						sound(Ind, "miss", NULL, SFX_TYPE_ATTACK, FALSE);
				}
 #endif
				continue;
			}
#endif

#ifdef USE_BLOCKING
			/* Parry/Block - belongs to new-NR-viability changes */
			/* choose whether to attempt to block or to parry (can't do both at once),
			   50% chance each, except for if weapon is missing (anti-retaliate-inscription
			   has been left out, since if you want max block, you'll have to take off your weapon!) */
			if (!backstab && q_ptr->shield_deflect && (!q_ptr->weapon_parry || magik(q_ptr->combat_stance == 1 ? 75 : 50))) {
				if (magik(apply_block_chance(q_ptr, q_ptr->shield_deflect + 10))) { /* boost for PvP! */
					msg_format(Ind, "\377%c%s blocks your attack!", COLOUR_BLOCK_PLY, q_name);
					switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
					case 's': case 'x': case 'z':
						msg_format(0 - c_ptr->m_idx, "\377%cYou block %s' attack!", COLOUR_BLOCK_GOOD, p_ptr->name);
						break;
					default:
						msg_format(0 - c_ptr->m_idx, "\377%cYou block %s's attack!", COLOUR_BLOCK_GOOD, p_ptr->name);
					}
 #ifdef USE_SOUND_2010
					if (sfx == 0 && p_ptr->sfx_defense)
						sound(Ind, "block_shield", NULL, SFX_TYPE_ATTACK, FALSE);
 #endif
					continue;
				}
			}
#endif
#ifdef USE_PARRYING
			if (!backstab && q_ptr->weapon_parry) {
				if (magik(apply_parry_chance(q_ptr, q_ptr->weapon_parry
				     /* boost for PvP!:  Note: No need to check second hand, because 2h cannot be dual-wielded.
				        this implies that 2h-weapons always go into INVEN_WIELD though. */
				    + ((k_info[q_ptr->inventory[INVEN_WIELD].k_idx].flags4 & TR4_MUST2H) ? 10 : 5)
				    + (q_ptr->dual_wield && q_ptr->dual_mode ? 10 : 0)
				    ))) {
					msg_format(Ind, "\377%c%s parries your attack!", COLOUR_PARRY_PLY, q_name);
					switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
					case 's': case 'x': case 'z':
						msg_format(0 - c_ptr->m_idx, "\377%cYou parry %s' attack!", COLOUR_PARRY_GOOD, p_ptr->name);
						break;
					default:
						msg_format(0 - c_ptr->m_idx, "\377%cYou parry %s's attack!", COLOUR_PARRY_GOOD, p_ptr->name);
					}
 #ifdef USE_SOUND_2010
					if (sfx == 0 && p_ptr->sfx_defense)
						sound(Ind, "parry_weapon", "parry", SFX_TYPE_ATTACK, FALSE);
 #endif
					continue;
				}
			}
#endif

#ifdef USE_SOUND_2010
			if (sfx == 0 && p_ptr->sfx_combat) {
				if (o_ptr->k_idx && (is_melee_weapon(o_ptr->tval) ||
 #ifdef EQUIPPABLE_DIGGERS
				    o_ptr->tval == TV_DIGGING ||
 #endif
				    o_ptr->tval == TV_MSTAFF))
					switch (o_ptr->tval) {
					case TV_SWORD: sound(Ind, "hit_sword", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
					case TV_BLUNT:	if (o_ptr->sval == SV_WHIP) sound(Ind, "hit_whip", "hit_weapon", SFX_TYPE_ATTACK, FALSE);
							else sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_ATTACK, FALSE);
							break;
					case TV_AXE: sound(Ind, "hit_axe", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
					case TV_POLEARM: sound(Ind, "hit_polearm", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
 #ifdef EQUIPPABLE_DIGGERS
					case TV_DIGGING:
 #endif
					case TV_MSTAFF: sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
					}
				else
					sound(Ind, "hit", NULL, SFX_TYPE_ATTACK, FALSE);
			}
#else
			sound(Ind, SOUND_HIT);
#endif
			sprintf(hit_desc, "You hit %s", q_name);

#ifdef ENABLE_BLOOD_FRENZY
			/* Blood frenzy? */
			if (blood_frenzy_ok) {
				if (bleeding) {
					p_ptr->blood_frenzy_rage += blood_frenzy_step;
					/* Enter frenzy after spilling enough blood */
					if (p_ptr->blood_frenzy_rage >= 1000) {
						p_ptr->blood_frenzy_rage = 1000;
						if (!p_ptr->blood_frenzy_active) {
							p_ptr->blood_frenzy_active = TRUE;
							p_ptr->update |= PU_BONUS;
							msg_print(Ind, "\377BYou enter a blood frenzy!");
							if (!rand_int(3)) switch (rand_int(p_ptr->ptrait == TRAIT_CORRUPTED ? 4 : 2)) {
							//msg_format(Ind, "~\377s%s\377w~", Rumor);
							case 0: msg_print(Ind, "'Break their backs!'"); break;
							case 1: msg_print(Ind, "'KILL! MAIM! BURN!'"); break;
							case 2: msg_print(Ind, "'Blood for the Blood god!'"); break;
							case 3: msg_print(Ind, "'Skulls for the Skull Throne!'"); break;
							}
						}
					}
				}
			} else if (p_ptr->blood_frenzy_active) {
				p_ptr->blood_frenzy_rage = 0; // reset completely (!)
				p_ptr->blood_frenzy_active = FALSE;
				p_ptr->update |= PU_BONUS;
				msg_print(Ind, "\377WYour blood frenzy ceases.");
			}
#endif

			/* Hack -- bare hands do one damage */
			k = 1;

			/* Ghosts do damages relative to level */
			/*
			if (p_ptr->ghost)
				k = p_ptr->lev;
			if (p_ptr->fruit_bat)
				k = p_ptr->lev;
			*/
				//k = p_ptr->lev * (p_ptr->lev + 50) / 50;
			if (martial) {
				int special_effect = 0, stun_effect = 0, times = 0;
				martial_arts *ma_ptr = &ma_blows[0], *old_ptr = &ma_blows[0];
				int resist_stun = 0;
				int marts = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 50);

				if (q_ptr->resist_conf) resist_stun += 44;
				if (q_ptr->free_act) resist_stun += 44;

				for (times = 0; times < (marts < 7 ? 1 : marts / 7); times++) {
					/* Attempt 'times' */
					do {
						ma_ptr = &ma_blows[rand_int(p_ptr->total_winner ? MAX_MA : MAX_NONWINNER_MA)];
					}
					while ((ma_ptr->min_level > marts)
					    || (randint(marts)<ma_ptr->rchance));

					/* keep the highest level attack available we found */
					if ((ma_ptr->min_level >= old_ptr->min_level) &&
					    !(p_ptr->stun || p_ptr->confused)) {
						old_ptr = ma_ptr;
					} else {
						ma_ptr = old_ptr;
					}
				}

				k = damroll(ma_ptr->dd, ma_ptr->ds);

				if (ma_ptr->effect == MA_KNEE) {
					if (q_ptr->male) {
						msg_format(Ind, "You hit %s in the groin with your knee!", q_name);
						special_effect = MA_KNEE;
					} else
						sprintf(hit_desc, ma_ptr->desc, q_name);
						//msg_format(Ind, ma_ptr->desc, q_name);
				}

				else {
					if (ma_ptr->effect) {
						stun_effect = (ma_ptr->effect/2) + randint(ma_ptr->effect/2);
					}

					sprintf(hit_desc, ma_ptr->desc, q_name);
					//msg_format(Ind, ma_ptr->desc, q_name);
				}

#ifdef CRIT_UNBRANDED
				k2 = k;
				k = brand_dam_aux_player(Ind, NULL, -1, k, q_ptr, FALSE);
				k2 = k - k2; /* remember difference between branded and unbranded dice */

				/* Apply the player damage boni */
				k += p_ptr->to_d + p_ptr->to_d_melee;

				k3 = critical_melee(Ind, marts * (randint(10)), ma_ptr->min_level, k - k2, 0, FALSE, FALSE);
				k3 += k2;
#else
				k = brand_dam_aux_player(Ind, NULL, -1, k, q_ptr, FALSE);

				/* Apply the player damage boni */
				k += p_ptr->to_d + p_ptr->to_d_melee;

				k3 = critical_melee(Ind, marts * (randint(10)), ma_ptr->min_level, k, 0, FALSE, FALSE);
#endif

#ifdef CRIT_VS_BACKSTAB
				if (!backstab && !stab_fleeing)
#endif
				k = k3;

				if ((special_effect == MA_KNEE) && (k < q_ptr->chp)) {
					msg_format(Ind, "%^s moans in agony!", q_name);
					stun_effect = 3 + randint(3);
					resist_stun /= 3;
				}

				if (stun_effect && (k < q_ptr->chp)) {
					if (marts > randint((q_ptr->lev * 2) + resist_stun + 10)) {
						msg_format(Ind, "\377y%^s is stunned.", q_name);

						set_stun(0 - c_ptr->m_idx, q_ptr->stun + stun_effect + get_skill_scale(p_ptr, SKILL_COMBAT, 3));
					}
				}

				/* Vampiric drain */
				if ((magik(vampiric_melee)) && drainable)
					drain_result = q_ptr->chp;
				else
					drain_result = 0;
			/* Handle normal weapon */
			} else if (o_ptr->k_idx) {

				k = damroll(o_ptr->dd, o_ptr->ds);

				/* weapons that inflict little damage, especially of only 1 damage dice,
				   mostly don't cause earthquakes at all */
				if ((p_ptr->impact || (f5 & TR5_IMPACT)) &&
				    500 / (10 + (k - o_ptr->dd) * o_ptr->dd * o_ptr->ds / (o_ptr->dd * (o_ptr->ds - 1) + 1)) < randint(35)) do_quake = TRUE;
				    /* I made the new formula above, to get a better chance curve over all the different weapon types. -C. Blue- */
				    /* Some old tries: */
				//    130 - ((k - o_ptr->dd) * o_ptr->dd * o_ptr->ds / (o_ptr->dd * (o_ptr->ds - 1) + 1)) < randint(130)) do_quake = TRUE;
				//    (150 / (10 + k - o_ptr->dd) < 11 - (2 / o_ptr->dd))) do_quake = TRUE;
				//    (150 / (1 + k - o_ptr->dd) < 23 - (2 / o_ptr->dd))) do_quake = TRUE;

#if defined(VORPAL_UNBRANDED) || defined(VORPAL_LOWBRANDED)
				if ((f5 & TR5_VORPAL) && !q_ptr->no_cut && !rand_int(VORPAL_CHANCE)) vorpal_cut = k; /* save unbranded dice */
				else vorpal_cut = FALSE;
#endif
#ifdef CRIT_UNBRANDED
				k2 = k;
				k = brand_dam_aux_player(Ind, o_ptr, slot, k, q_ptr, FALSE);
				k2 = k - k2; /* remember difference between branded and unbranded dice */
#else
				k = brand_dam_aux_player(Ind, o_ptr, slot, k, q_ptr, FALSE);
#endif
#ifdef VORPAL_LOWBRANDED
				if (vorpal_cut) vorpal_cut = (vorpal_cut + k) / 2;
#else
 #ifndef VORPAL_UNBRANDED
				if ((f5 & TR5_VORPAL) && !q_ptr->no_cut && !rand_int(VORPAL_CHANCE)) vorpal_cut = k; /* save branded dice */
				else vorpal_cut = FALSE;
 #endif
#endif

#ifdef ENABLE_STANCES
				/* apply stun from offensive combat stance */
				if (p_ptr->combat_stance == 2) {
					int stun_effect, resist_stun;

					stun_effect = randint(get_skill_scale(p_ptr, SKILL_MASTERY, 10) + adj_con_fix[p_ptr->stat_ind[A_STR]] / 2) + 1;
					stun_effect /= 2;

					resist_stun = adj_con_fix[q_ptr->stat_ind[A_CON]]; /* 0..9 */
					if (q_ptr->free_act) resist_stun += 3;
					resist_stun += 6 - 300 / (50 + q_ptr->ac + q_ptr->to_a); /* 0..5 */
					resist_stun -= adj_con_fix[p_ptr->stat_ind[A_DEX]];
					if (resist_stun < 0) resist_stun = 0; /* 0..17 (usually 8 vs fighters) */

					switch (p_ptr->combat_stance_power) {
					case 0: if (!magik(20 - resist_stun * 2)) stun_effect = 0; break;
					case 1: if (!magik(25 - resist_stun * 2)) stun_effect = 0; break;
					case 2: if (!magik(30 - resist_stun * 2)) stun_effect = 0; break;
					case 3: if (!magik(35 - resist_stun * 2)) stun_effect = 0; break;
					}
					msg_format(Ind, "\377y%^s is stunned.", q_name);
					set_stun(0 - c_ptr->m_idx, q_ptr->stun + stun_effect + get_skill_scale(p_ptr, SKILL_COMBAT, 3));
				}
#endif

				/* Select a chaotic effect (10% chance) */
				if ((f5 & TR5_CHAOTIC) && !rand_int(10)) {
					if (!rand_int(2)) {
						/* Vampiric (50%) (50%) */
						chaos_effect = 1;
					} else if (!rand_int(1000)) {
						/* Quake (0.050%) (49.975%) */
						chaos_effect = 2;
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

				/* Vampiric drain */
				if (((chaos_effect == 1) ||
				    magik(vampiric_melee)) && drainable)
					drain_result = q_ptr->chp;
				else
					drain_result = 0;

				if (chaos_effect == 2) do_quake = TRUE;
				if (vorpal_cut) msg_format(Ind, "Your weapon cuts deep into %s!", q_name);

				/* NOTE: chaos_effect == 3 aka confusion is not applied atm! Maybe too OP at the given frequency. */

				k += o_ptr->to_d;

				/* Apply the player damage boni */
				k += p_ptr->to_d + p_ptr->to_d_melee;

#ifdef DEFENSIVE_STANCE_TOTAL_MELEE_REDUCTION
				if (p_ptr->combat_stance == 1) {
					if (p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)
						switch (p_ptr->combat_stance_power) {
						case 0: k = (k * 7 + 9) / 10; break;
						case 1: k = (k * 7 + 9) / 10; break;
						case 2: k = (k * 7 + 9) / 10; break;
						case 3: k = (k * 7 + 9) / 10; break;
						}
					else
						switch (p_ptr->combat_stance_power) {
						case 0: k = (k * 6 + 9) / 10; break;
						case 1: k = (k * 7 + 9) / 10; break;
						case 2: k = (k * 7 + 9) / 10; break;
						case 3: k = (k * 7 + 9) / 10; break;
						}
				}
#endif

				/* Critical strike moved here, since it works best
				with light weapons, which have low dice. So for gain
				we need the full damage including all to-dam boni */
#ifdef CRIT_UNBRANDED
				k3 = critical_melee(Ind, o_ptr->weight, o_ptr->to_h, k - k2, calc_crit_obj(o_ptr), rogue_armed_melee(o_ptr, p_ptr), TRUE);
				k3 += k2;
#else
				k3 = critical_melee(Ind, o_ptr->weight, o_ptr->to_h, k, calc_crit_obj(o_ptr), rogue_armed_melee(o_ptr, p_ptr), TRUE);
#endif
				k2 = k; /* remember damage before crit */
#ifdef CRIT_VS_BACKSTAB
				if (!backstab && !stab_fleeing)
#endif
				k = k3;

				/* penalty for weapons in bat form */
				if (p_ptr->body_monster == RI_VAMPIRE_BAT) k /= 2;
			/* handle bare fists/bat/ghost */
			} else {
				k = brand_dam_aux_player(Ind, NULL, -1, k, q_ptr, FALSE);

				/* Apply the player damage boni */
				/* (should this also cancelled by nazgul?(for now not)) */
				k += p_ptr->to_d + p_ptr->to_d_melee;

				k3 = k;

				/* Vampiric drain */
				if ((magik(vampiric_melee)) && drainable)
					drain_result = q_ptr->chp;
				else
					drain_result = 0;
			}

			/* No negative damage */
			if (k < 0) k = 0;

			/* New backstab formula: it works like criticals now and takes a bit of monster hp into account */
			/* Note that the multiplier is after all the damage calc is done! So may need tweaking! */
			if (backstab || stab_fleeing) {
				int bs_flat = q_ptr->chp / (dual_stab ? 30 : 20);
				int bs_multiplier = 100 + get_skill_scale(p_ptr, SKILL_BACKSTAB, 350 + rand_int(101));

				/* similar to dodging, the effect is weaker if the skill level doesn't match the monster level */
				bs_flat = (bs_flat * bs_skill) / (q_ptr->lev >= bs_skill ? q_ptr->lev : bs_skill);

				kl = k * bs_multiplier;
				kl /= (dual_stab ? 150 : 100);
				kl += bs_flat;
#ifdef CRIT_VS_BACKSTAB
				if (k3 > kl) {
					backstab = stab_fleeing = FALSE;
					k = k3;
				} else
#endif
				k = kl;
#ifdef CRIT_VS_VORPAL
				kl = k2 * bs_multiplier;
				kl /= (dual_stab ? 150 : 100);
				kl += bs_flat;
				k2 = kl;
#endif
			}

			/* Vorpal bonus - multi-dice!
			   (currently +31.25% more branded dice damage on total average, just for the records) */
			if (vorpal_cut) {
#ifdef CRIT_VS_VORPAL
				k2 += (magik(25) ? 2 : 1) * (vorpal_cut + 5); /* exempts critical strike */
				/* either critical hit or vorpal, not both */
				if (k2 > k) k = k2;
#else
				k += (magik(25) ? 2 : 1) * (vorpal_cut + 5); /* exempts critical strike */
#endif
			}

			/* factor in AC */
			if (!pierced
#ifdef PVP_BACKSTAB_PIERCES
			    && !backstab
#endif
			    )
				k -= (k * (((q_ptr->ac + q_ptr->to_a) < AC_CAP) ? (q_ptr->ac + q_ptr->to_a) : AC_CAP) / AC_CAP_DIV);

			/* Special hack: In pvp, make (royal defensive) stance somewhat less great */
			if (p_ptr->combat_stance == 1) k = (k * 2 + 1) / 3;

			/* Remember original damage for vampirism (less rounding trouble..) */
			k2 = k;
			/* Reduce damage in PvP */
			k = (k + PVP_MELEE_DAM_REDUCTION - 1) / PVP_MELEE_DAM_REDUCTION;

			/* Messages */
			if (backstab) {
				backstab_feed = TRUE;
				backstab = FALSE;
				if (martial) {
					msg_format(Ind, "You twist the neck of %s%s for \377L%d \377wdamage.", sleep_stab ? "sleeping " : "", q_name, k);
					msg_format(0 - c_ptr->m_idx, "%s twists your neck for \377R%d \377wdamage.", p_ptr->name, k);
				} else {
					msg_format(Ind, "You stab helpless %s%s for \377L%d \377wdamage.", sleep_stab ? "sleeping " : "", q_name, k);
					msg_format(0 - c_ptr->m_idx, "%s backstabs you for \377R%d \377wdamage.", p_ptr->name, k);
				}
			}
			else if (stab_fleeing) {
				stab_fleeing = FALSE;
				if (martial) {
					msg_format(Ind, "You strike the back of %s for \377L%d \377wdamage.", q_name, k);
					msg_format(0 - c_ptr->m_idx, "%s strikes your back for \377R%d \377wdamage.", p_ptr->name, k);
				} else {
					msg_format(Ind, "You backstab fleeing %s for \377L%d \377wdamage.", q_name, k);
					msg_format(0 - c_ptr->m_idx, "%s backstabs you for \377R%d \377wdamage.", p_ptr->name, k);
				}
			}
			//else if (!martial) msg_format(Ind, "You hit %s for \377g%d \377wdamage.", m_name, k);
			else {
				msg_format(Ind, "%s for \377y%d \377wdamage.", hit_desc, k);
				msg_format(0 - c_ptr->m_idx, "%s hits you for \377R%d \377wdamage.", p_ptr->name, k);
			}

			/* Cannot kill on this grid? */
			if (no_pk) {
				if (k > q_ptr->chp) k = q_ptr->chp;
				if (k2 > q_ptr->chp) k2 = q_ptr->chp;
			}

			/* Exploding attack - Kurzel */
			if (p_ptr->nimbus) do_nimbus(Ind, y, x);

			/* no_pk handled */
			if (cfg.use_pk_rules == PK_RULES_NEVER && q_ptr->chp - k <= 0) {
				msg_format(Ind, "\374You have beaten %s", q_ptr->name);
				msg_format(0 - c_ptr->m_idx, "\374%s has beaten you up!", p_ptr->name);
				teleport_player(0 - c_ptr->m_idx, 400, TRUE);

				/* End of the fight */
				break;
			}

			p_ptr->vamp_fed_midx = -c_ptr->m_idx;
			p_ptr->tmp_x = 0; //blood bond marker
			take_hit(0 - c_ptr->m_idx, k, p_ptr->name, Ind);
			/* Blood bond won? End of the fight */
			if (p_ptr->tmp_x) break;

			/* Check for death */
			if (q_ptr->death) {
				/* Vampires feed off the life force! (if any) */
				// mimic forms for vampires/bats: 432, 520, 521, 623, 989
				if (p_ptr->prace == RACE_VAMPIRE && drainable) {
					int feed = q_ptr->mhp + 100;

					//feed = (4 - (300 / feed)) * 1000;//1000..4000
					feed = (6 - (300 / feed)) * 100;//300..600
					if (backstab_feed) feed *= 2;
					if (q_ptr->prace == RACE_VAMPIRE) feed /= 3;
					set_food(Ind, feed + p_ptr->food);
				}

#ifdef USE_SOUND_2010
				/* hack: always play 'hit' sfx for final killing hit,
				   so if we didn't play it already (we did so if sfx==0) then play it now instead. */
				if (sfx && p_ptr->sfx_combat) {
					if (o_ptr->k_idx && (is_melee_weapon(o_ptr->tval) ||
 #ifdef EQUIPPABLE_DIGGERS
					    o_ptr->tval == TV_DIGGING ||
 #endif
					    o_ptr->tval == TV_MSTAFF))
						switch (o_ptr->tval) {
						case TV_SWORD: sound(Ind, "hit_sword", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
						case TV_BLUNT:	if (o_ptr->sval == SV_WHIP) sound(Ind, "hit_whip", "hit_weapon", SFX_TYPE_ATTACK, FALSE);
								else sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_ATTACK, FALSE);
								break;
						case TV_AXE: sound(Ind, "hit_axe", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
						case TV_POLEARM: sound(Ind, "hit_polearm", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
 #ifdef EQUIPPABLE_DIGGERS
						case TV_DIGGING:
 #endif
						case TV_MSTAFF: sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
						}
					else
						sound(Ind, "hit", NULL, SFX_TYPE_ATTACK, FALSE);
				}
//#else
//				sound(Ind, SOUND_HIT);
#endif
				/* End of the fight */
				break;
			}

			/* Redundant check actually, we already caught all 'End of the fight' cases above. */
			if (!c_ptr->m_idx) break;

			/* -- Target didn't die, so the fight is still going on -- */

			/* Apply aura damage to attacker */
			py_touch_zap_player(Ind, -c_ptr->m_idx);
			/* todo maybe: handle player death from getting zapped? */

			/* VAMPIRIC: Are we draining it?  A little note: If the monster is
			   dead, the drain does not work... */
			if (drain_result) {
				drain_result -= q_ptr->chp;  /* Calculate the difference */
				/* Compensate for PvP damage reduction */
				drain_result *= PVP_MELEE_DAM_REDUCTION;
				if (drain_result > k2) drain_result = k2;

				if (drain_result > 0) { /* Did we really hurt it? */
					drain_heal = randint(2)
					    + damroll(2, /* was 4,../6 -- was 8 for 50 max_drain */
					    drain_result / 16);

					if (drain_left) {
						if (drain_heal < drain_left) {
#if 0
							drain_left -= drain_heal;
#endif
						} else {
							drain_heal = drain_left;
#if 0
							drain_left = 0;
#endif
						}

						/* factor in the melee damage reduction for PvP,
						   compensate fractions from dividing low integers here */
						drain_frac = (drain_heal * 10 + PVP_MELEE_DAM_REDUCTION - 1) / PVP_MELEE_DAM_REDUCTION;
						drain_heal = (drain_heal + PVP_MELEE_DAM_REDUCTION - 1) / PVP_MELEE_DAM_REDUCTION;
						drain_frac -= (drain_heal * 10);
						if (drain_frac > rand_int(10)) drain_heal++;

						if (drain_msg) {
							if (martial || !o_ptr->k_idx) {
#ifndef TEST_SERVER
								if (is_admin(p_ptr)) /* for debugging purpose */
#endif
								msg_format(Ind, "Your hits drain \377o%d\377w life from %s!", drain_heal, q_name);
#ifndef TEST_SERVER
								else
								msg_format(Ind, "Your hits drain life from %s!", q_name);
#endif
							} else {
#ifndef TEST_SERVER
								if (is_admin(p_ptr))
#endif
								msg_format(Ind, "Your weapon drains \377o%d\377w life from %s!", drain_heal, q_name);
#ifndef TEST_SERVER
								else
								msg_format(Ind, "Your weapon drains life from %s!", q_name);
#endif
							}
#if 0
							drain_msg = FALSE;
#endif
						}

						i = p_ptr->chp;
						hp_player(Ind, drain_heal, TRUE, TRUE);
						p_ptr->test_drain += (p_ptr->chp - i);
						/* We get to keep some of it! */
					}
				}
			}

			/* Apply effects from mimic monster forms */
			if (p_ptr->body_monster) {
#if 0
				switch (pr_ptr->r_ptr->d_char) {
					/* If monster is fighting with a weapon, the player gets the effect(s) even with a weapon */
					case 'p':	case 'h':	case 't':
					case 'o':	case 'y':	case 'k':
					apply_monster_effects = TRUE;
					break;
					/* If monster is fighting without weapons, the player gets the effect(s) only if
					he fights with bare hands/martial arts */
					default:
					if (!o_ptr->k_idx) apply_monster_effects = TRUE;
					break;
				}
				/* change a.m.b.=TRUE to =FALSE at declaration above if u use this if0-part again */
#endif
				/* If monster is fighting with a weapon, the player gets the effect(s) even with a weapon */
				/* If monster is fighting without weapons, the player gets the effect(s) only if
				he fights with bare hands/martial arts */
				if (!pr_ptr->body_parts[BODY_WEAPON])
					if (o_ptr->k_idx) apply_monster_effects = FALSE;

				/* Get monster effects. If monster has several, choose one randomly */
				monster_effects = 0;
				for (i = 0; i < 4; i++) {
					if (pr_ptr->blow[i].d_dice && pr_ptr->blow[i].d_side) {
						monster_effects++;
						monster_effect[monster_effects] = pr_ptr->blow[i].effect;
					}
				}
				/* Choose random brand from the ones available */
				monster_effect_chosen = monster_effect[1 + rand_int(monster_effects)];

				/* Modify damage effect */
				if (apply_monster_effects) {
					switch (monster_effect_chosen) {
					case RBE_DISEASE:
						/* Take "poison" effect (for now mimic form brand just disease->poison) */
						if (q_ptr->resist_pois || q_ptr->oppose_pois || q_ptr->immune_poison) {
							msg_format(Ind, "%^s is unaffected.", q_name);
						} else if (rand_int(100) < q_ptr->skill_sav) {
							msg_format(Ind, "%^s resists the disease.", q_name);
						} else {
							msg_format(Ind, "%^s suffers from disease.", q_name);
							/* Note: We use set_poisoned() instead of set_diseased() because we say in general,
							   that RBE_DISEASE yields _poison_ brand for mimicry users,
							   as currently monster-diseasing doesn't exist... */
							set_poisoned(0 - c_ptr->m_idx, q_ptr->poisoned + randint(p_ptr->lev) + 5, Ind);
						}
						break;
					case RBE_BLIND:
						/* Increase "blind" */
						if (q_ptr->resist_blind)
						/*  for (i = 1; i <= NumPlayers; i++)
						if (Players[i]->id == q_ptr->id) { */
						{
							msg_format(Ind, "%^s is unaffected.", q_name);
						} else if (rand_int(100) < q_ptr->skill_sav) {
							msg_format(Ind, "%^s resists the effect.", q_name);
						} else {
							set_blind(0 - c_ptr->m_idx, q_ptr->blind + 10 + randint(p_ptr->lev));
						}
						break;
					case RBE_HALLU:
						/* Increase "image" */
						if (q_ptr->resist_chaos) {
							msg_format(Ind, "%^s is unaffected.", q_name);
						} else if (rand_int(100) < q_ptr->lev) {
							msg_format(Ind, "%^s resists the effect.", q_name);
						} else {
							set_image(0 - c_ptr->m_idx, q_ptr->image + 3 + randint(p_ptr->lev / 2));
						}
						break;
					case RBE_CONFUSE:
						if (!p_ptr->confusing) {
							/* Confuse the monster */
							if (q_ptr->resist_conf) {
								msg_format(Ind, "%^s is unaffected.", q_name);
							} else if (rand_int(100) < q_ptr->lev) {
								msg_format(Ind, "%^s resists the effect.", q_name);
							} else {
								msg_format(Ind, "%^s appears confused.", q_name);
								set_confused(0 - c_ptr->m_idx, q_ptr->confused + 10 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 10)));
							}
						}
						break;
					case RBE_TERRIFY:
						fear_chance = 50 + (p_ptr->lev - q_ptr->lev) * 5;
						if (q_ptr->resist_fear) {
							msg_format(Ind, "%^s is unaffected.", q_name);
						} else if (rand_int(100) < fear_chance) {
							msg_format(Ind, "%^s appears afraid.", q_name);
							set_afraid(0 - c_ptr->m_idx, q_ptr->afraid + 4 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 10)));
						} else {
							msg_format(Ind, "%^s resists the effect.", q_name);
						}
						break;
					case RBE_PARALYZE:
						/* Increase "paralyzed" */
						if (q_ptr->free_act) {
							msg_format(Ind, "%^s is unaffected.", q_name);
						} else if (rand_int(100) < q_ptr->skill_sav) {
							msg_format(Ind, "%^s resists the effect.", q_name);
						} else {
							set_paralyzed(0 - c_ptr->m_idx, q_ptr->paralyzed + 3 + randint(p_ptr->lev));
						}
#if 0
						if (!p_ptr->stunning) {
							/* Stun the monster */
							if (rand_int(100) < q_ptr->lev) {
								msg_format(Ind, "%^s resists the effect.", q_name);
							} else {
								msg_format(Ind, "\377o%^s appears stunned.", q_name);
								set_stun_raw(0 - c_ptr->m_idx, q_ptr->stun + 20 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 10)));
							}
						}
#endif
						break;
					}
				}
			}

			/* Confusion attack */
			if (p_ptr->confusing) {
				/* Cancel glowing hands */
				p_ptr->confusing--;

				/* Message */
				if (!p_ptr->confusing) msg_print(Ind, "Your hands stop glowing.");

				/* Confuse the monster */
				if (q_ptr->resist_conf) {
					msg_format(Ind, "%^s is unaffected.", q_name);
				} else if (rand_int(100) < q_ptr->lev) {
					msg_format(Ind, "%^s resists the effect.", q_name);
				} else {
					msg_format(Ind, "%^s appears confused.", q_name);
					set_confused(0 - c_ptr->m_idx, q_ptr->confused + 3 + rand_int(2 + get_skill_scale(p_ptr, SKILL_COMBAT, 10)));
				}
			}

			/* Stunning attack */
			if (p_ptr->stunning) {
				/* Cancel heavy hands */
				p_ptr->stunning = FALSE;

				/* Message */
				msg_print(Ind, "Your hands feel less heavy.");

				/* Stun the monster */
				if (rand_int(100) < q_ptr->lev) {
					msg_format(Ind, "%^s resists the effect.", q_name);
				} else {
					msg_format(Ind, "\377o%^s appears stunned.", q_name);
					set_stun(0 - c_ptr->m_idx, q_ptr->stun + 20 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 10)));
				}
			}

			/* Ghosts get fear attacks */
			i = get_skill(p_ptr, SKILL_AURA_FEAR); // O_+ "Fear Brand" - Kurzel
			if (p_ptr->ghost || ((i >= 20) && magik(i))) {
				fear_chance = 50 + (p_ptr->lev - q_ptr->lev) * 5;
				if (q_ptr->resist_fear) {
					msg_format(Ind, "%^s is unaffected.", q_name);
				} else if (rand_int(100) < fear_chance) {
					msg_format(Ind, "%^s appears afraid.", q_name);
					set_afraid(0 - c_ptr->m_idx, q_ptr->afraid + 4 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 10)));
				} else {
					msg_format(Ind, "%^s resists the effect.", q_name);
				}
			}

			/* Apply Poison technique used with non-standard poison potion for special fx */
			if (p_ptr->melee_brand_flags & TBRAND_F_POTION_MUSHROOM) {
#if 0 /* Tone down confusion brand? */
				if (p_ptr->melee_brand_t != SV_POTION_CONFUSION || !rand_int(10))
#else /* Town down ANY potion brands? */
				if (!rand_int(10))
#endif
					potion_mushroom_branding(Ind, q_ptr->px, q_ptr->py, p_ptr->melee_brand_t, FALSE);
			}

#if 0 /* moved to xtra1.c */
			/* Fruit bats get life stealing.
			   Note: This is ok because fruit bats cannot wear weapons/gloves as a
			   source of vampirism.
			   Still to check: Vampire fruit bats! CURRENTLY STACKS! */
			if (p_ptr->fruit_bat == 1 && !p_ptr->body_monster) {
				int leech = q_ptr->chp;

				if (k < leech) leech = k;
				leech /= 10;
				hp_player(Ind, rand_int(leech), TRUE, TRUE);
			}
#endif

#ifdef ENABLE_OHERETICISM
			/* Extend ongoing 'Boundless Hate' spell thanks to Traumaturgy feedback? */
			if (p_ptr->hate_prolong == 1) p_ptr->hate_prolong = 2;
#endif
		}

		/* Player misses */
		else {
			backstab = stab_fleeing = FALSE;

#ifdef USE_SOUND_2010
			if (sfx == 0 && p_ptr->sfx_combat) {
				if (o_ptr->k_idx && is_melee_weapon(o_ptr->tval))
					sound(Ind, "miss_weapon", "miss", SFX_TYPE_ATTACK, FALSE);
				else
					sound(Ind, "miss", NULL, SFX_TYPE_ATTACK, FALSE);
			}
#else
			sound(Ind, SOUND_MISS);
#endif
			/* Messages */
			msg_format(Ind, "You miss %s.", q_name);
			msg_format(0 - c_ptr->m_idx, "%s misses you.", p_ptr->name);
		}

		/* hack for dual-backstabbing: get a free b.p.r.
		   (needed as workaround for sleep-dual-stabbing executed
		   by auto-retaliator, where old-check below would otherwise break) - C. Blue */
		if (dual_stab == 4) dual_stab = 0;
		if (!dual_stab) p_ptr->melee_timeout_crit_dual = sleep_stab = cloaked_stab = shadow_stab = FALSE;
		if (dual_stab) {
			num--;
			continue;
		}

		/* Hack -- divided turn for auto-retaliator */
		if (!old) break;
	}

	/* Mega-Hack -- apply earthquake brand */
	if (do_quake && !p_ptr->quaked && magik(QUAKE_CHANCE)) {
		if (o_ptr->k_idx
#ifdef ALLOW_NO_QUAKE_INSCRIPTION
		    && !check_guard_inscription(o_ptr->note, 'Q')
#else
		    && (!check_guard_inscription(o_ptr->note, 'Q') ||
		    o_ptr->name1 != ART_GROND)
#endif
		    ) {
			/* Giga-Hack -- equalize the chance (though not likely..) */
			if (old || randint(p_ptr->num_blow) < 3) {
				earthquake(&p_ptr->wpos, p_ptr->py, p_ptr->px, 5);
				p_ptr->quaked = TRUE;
			}
		}
	}
}



/*
 * Player attacks a (poor, defenseless) creature        -RAK-
 *
 * If no "weapon" is available, then "punch" the monster one time.
 *
 * Note: old == TRUE if not auto-retaliating actually
 *       (important for dual-backstab treatment) - C. Blue
 */
//note: we assume that p_ptr->num_blow isn't 0 (div/0)
static void py_attack_mon(int Ind, int y, int x, byte old) {
	player_type	*p_ptr = Players[Ind];
	int		num = 0, bonus, chance, slot, owner_Ind = 0;
#ifdef USE_SOUND_2010
	int		sfx = 0;
#endif
	int		k, k3;
#if defined(CRIT_VS_VORPAL) || defined(CRIT_UNBRANDED)
	int		k2;
#endif
	long int	kl;
	object_type	*o_ptr = NULL;
#if defined(WIELD_BOOKS) || defined(WIELD_DEVICES)
	object_type forge_zero = { 0 }; /* Simulate an empty inventory slot, specifically 'wield' slot. */
#endif
	bool		do_quake = FALSE;

	char		m_name[MNAME_LEN], m_name_raw[MNAME_LEN], hit_desc[MAX_CHARS_WIDE], mbname[MNAME_LEN];
	monster_type	*m_ptr;
	monster_race	*r_ptr;

	bool		fear = FALSE;
	int		fear_chance;

	int		bs_skill = get_skill_scale(p_ptr, SKILL_BACKSTAB, 100);
	bool		stab_skill = (bs_skill != 0 && !p_ptr->rogue_heavyarmor);
	bool		sleep_stab = TRUE, cloaked_stab = (p_ptr->cloaked == 1), shadow_stab = (p_ptr->shadow_running); /* can player backstab the monster? */
	bool		backstab = FALSE, stab_fleeing = FALSE; /* does player backstab the monster? */
	bool		primary_wield, secondary_wield, dual_wield;
	int		dual_stab;
	bool		martial = FALSE, did_stun, did_knee, did_slow;
	bool		weapon; //atm PvE only: Backstabbing can cause stun effect
	int		block, parry;
#ifdef ENABLE_BLOOD_FRENZY
	bool		blood_frenzy_ok, bleeding = TRUE;
	int		blood_frenzy_step;
#endif

	int		vorpal_cut = 0;
	int		chaos_effect = 0;
	int		vampiric_melee;
	bool		drain_msg = TRUE;
	int		drain_result = 0, drain_heal = 0;
	int		drain_left = MAX_VAMPIRIC_DRAIN;
	bool		drainable = TRUE, backstab_feed = FALSE;
	int		feed;
	bool		helpless, uniq_bell = FALSE;
	char		uniq = 'w';


	struct worldpos	*wpos = &p_ptr->wpos;
	cave_type	**zcave;
	cave_type	*c_ptr;

	monster_race *pr_ptr = &r_info[p_ptr->body_monster];
	int mon_aqua = 0, mon_acid = 0, mon_fire = 0;
	bool apply_monster_effects = TRUE;
	int i, monster_effects;
	u32b monster_effect[6], monster_effect_chosen;


	monster_effect[1] = 0;
	monster_effect[2] = 0;
	monster_effect[3] = 0;
	monster_effect[4] = 0;
	monster_effect[5] = 0;

	o_ptr = &p_ptr->inventory[INVEN_WIELD];
	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT && o_ptr->xtra3 & 0x0200)
		primary_wield = is_melee_item(o_ptr->tval2);
	else
		primary_wield = is_melee_item(o_ptr->tval);

	o_ptr = &p_ptr->inventory[INVEN_ARM];
	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT && o_ptr->xtra3 & 0x0200)
		secondary_wield = (o_ptr->tval2 != 0 && o_ptr->tval2 != TV_SHIELD);
	else
		secondary_wield = (o_ptr->tval != 0 && o_ptr->tval != TV_SHIELD);

	dual_wield = primary_wield && secondary_wield && p_ptr->dual_mode; /* Note: primary_wield && secondary_wield == p_ptr->dual_wield (from xtra1.c) actually. */
	dual_stab = (dual_wield ? 1 : 0); /* organizer variable for dual-wield backstab */
	weapon = primary_wield || secondary_wield;
#ifdef ENABLE_BLOOD_FRENZY
	blood_frenzy_ok = dual_wield && p_ptr->inventory[INVEN_WIELD].tval == TV_AXE && p_ptr->inventory[INVEN_ARM].tval == TV_AXE && get_skill(p_ptr, SKILL_TRAUMATURGY) >= 17;
	if (blood_frenzy_ok) blood_frenzy_step = ((2 + get_skill_scale(p_ptr, SKILL_TRAUMATURGY, 400) + p_ptr->num_blow - 1) * (15 + p_ptr->num_blow)) / (15 * p_ptr->num_blow);
#endif

	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

	m_ptr = &m_list[c_ptr->m_idx];
	r_ptr = race_inf(m_ptr);
	helpless = (m_ptr->csleep || m_ptr->stunned > 100 || m_ptr->confused);

	if (p_ptr->melee_timeout_crit_dual && p_ptr->melee_crit_dual != c_ptr->m_idx)
		p_ptr->melee_timeout_crit_dual = 0;

	if (m_ptr->status & M_STATUS_FRIENDLY) return;

	if ((r_ptr->flags3 & RF3_UNDEAD) ||
	    //(r_ptr->flags3 & RF3_DEMON) ||
	    (r_ptr->flags3 & RF3_NONLIVING)) {
		drainable = FALSE;
#ifdef ENABLE_BLOOD_FRENZY
		bleeding = (r_ptr->d_char == 'V');
#endif
	} else if (strchr("EgvwlIFijmxszQX", r_ptr->d_char)) {
		drainable = FALSE;
#ifdef ENABLE_BLOOD_FRENZY
		bleeding = (r_ptr->d_char == 'Q');
#endif
	}
#ifdef ENABLE_BLOOD_FRENZY
	else bleeding = !(r_ptr->d_char == 'A' || (r_ptr->flags8 & RF8_NO_CUT));
#endif

	/* is it a unique we already got kill credit for? */
	if ((r_ptr->flags1 & RF1_UNIQUE) &&
	    p_ptr->r_killed[m_ptr->r_idx] == 1) {
		uniq = 'D';
		if (p_ptr->warn_unique_credit) uniq_bell = TRUE;
	}

	/* Disturb the player */
	disturb(Ind, 0, 0);

	/* Extract monster name (or "it") */
	monster_desc(Ind, m_name, c_ptr->m_idx, 0);
	monster_desc(Ind, m_name_raw, c_ptr->m_idx, 0x01);
	/* Prepare lower-case'd name for elementality tests */
	strcpy(mbname, m_name);
	mbname[0] = tolower(mbname[0]);

	/* try to find its owner online */
	if (m_ptr->owner)
		for (i = NumPlayers; i > 0; i--)
			if (m_ptr->owner == Players[i]->id) {
				owner_Ind = i;
				break;
			}

#if 0 /* golems get attacked by other players */
	if ((m_ptr->owner == p_ptr->id && !p_ptr->confused &&
		p_ptr->mon_vis[c_ptr->m_idx]) ||
		(m_ptr->owner != p_ptr->id && m_ptr->pet)) //dont kill pets either, meanie!
#else /* prevent golems being attacked by other players */
	if ((m_ptr->owner == p_ptr->id && !p_ptr->confused && p_ptr->mon_vis[c_ptr->m_idx]) ||
	    (m_ptr->owner == p_ptr->id && m_ptr->pet) || //dont kill pets either, meanie!
	    //!owner_Ind || /* don't attack ownerless golems */
	    (owner_Ind && !check_hostile(Ind, owner_Ind))) /* only attack if owner is hostile */
#endif
	{
		int ox = m_ptr->fx, oy = m_ptr->fy, nx = p_ptr->px, ny = p_ptr->py;

		msg_format(Ind, "You swap positions with %s.", m_name);

		/* Update the new location */
		zcave[ny][nx].m_idx = c_ptr->m_idx;
		/* Update the old location */
		zcave[oy][ox].m_idx = -Ind;

		/* Move the monster */
		m_ptr->fy = ny;
		m_ptr->fx = nx;
		store_exit(Ind);
		p_ptr->py = oy;
		p_ptr->px = ox;

		cave_midx_debug(wpos, oy, ox, -Ind);

		/* Update the monster (new location) */
		update_mon(zcave[ny][nx].m_idx, TRUE);
		/* Redraw the old grid */
		everyone_lite_spot(wpos, oy, ox);
		/* Redraw the new grid */
		everyone_lite_spot(wpos, ny, nx);
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

		return;
	}

	/* Hack -- suppress messages */
	if (p_ptr->taciturn_messages) suppress_message = TRUE;
	/* Auto-Recall if possible and visible */
	if (p_ptr->mon_vis[c_ptr->m_idx]) recent_track(m_ptr->r_idx);
	/* Track a new monster */
	if (p_ptr->mon_vis[c_ptr->m_idx]) health_track(Ind, c_ptr->m_idx);

#ifdef ENABLE_OUNLIFE
	/* Attacking on purpose terminates Wraithstep */
	if (p_ptr->tim_wraith && (p_ptr->tim_wraithstep & 0x1) &&
	    ((r_ptr->flags2 & RF2_KILL_WALL) || !(r_ptr->flags2 & RF2_PASS_WALL))) {
		set_tim_wraith(Ind, 0);
	}
#endif

	/* can't attack while in WRAITHFORM */
	/* wraithed players can attack wraithed monsters - mikaelh */
	if (p_ptr->tim_wraith && !is_admin(p_ptr) &&
	    ((r_ptr->flags2 & RF2_KILL_WALL) || !(r_ptr->flags2 & RF2_PASS_WALL))) /* lil fix (Morgoth) - C. Blue */
		return;

	/* Hack -- divided turn for auto-retaliator */
	if (!old) {
		p_ptr->energy -= level_speed(&p_ptr->wpos) / p_ptr->num_blow;
		/* -C. Blue- We're only executing ONE blow and will break out then,
		   so adjust the maximum drain accordingly: */
#if 0
		k = drain_left / p_ptr->num_blow;
		/* ..and make up for rounding errors :) */
		drain_left = k + (magik(((drain_left - (k * p_ptr->num_blow)) * 100) / p_ptr->num_blow) ? 1 : 0);
#endif
	}

#if 1
	k = drain_left / p_ptr->num_blow;
	/* ..and make up for rounding errors :) */
	drain_left = k + (magik(((drain_left - (k * p_ptr->num_blow)) * 100) / p_ptr->num_blow) ? 1 : 0);
#endif

	/* Handle player fear */
	if (p_ptr->afraid) {
		msg_format(Ind, "\377yYou are too afraid to attack %s!", m_name);
		suppress_message = FALSE;
		/* Done */
		return;
	}

	/* Cannot 'stab' with martial-arts */
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS)
	    && !p_ptr->inventory[INVEN_WIELD].k_idx
	    && !p_ptr->inventory[INVEN_ARM].k_idx
#ifndef ENABLE_MA_BOOMERANG
	    && !p_ptr->inventory[INVEN_BOW].k_idx)
#else
	    && p_ptr->inventory[INVEN_BOW].tval != TV_BOW)
#endif
		martial = TRUE;

	/* check whether monster can be backstabbed */
	if (!m_ptr->csleep /*&& m_ptr->ml*/) sleep_stab = FALSE;
	if (m_ptr->backstabbed) {
		cloaked_stab = FALSE; /* a monster can only be backstabbed once, except if it gets resleeped or stabbed while fleeing */
		shadow_stab = FALSE; /* assume monster doesn't fall twice for it */
	}
	if (!sleep_stab && !cloaked_stab && !shadow_stab && !p_ptr->melee_timeout_crit_dual) dual_stab = 0;

	/* cloaking mode stuff */
	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
	bandage_fails(Ind);

	/* Disturb the monster */
	if (m_ptr->csleep) {
		m_ptr->csleep = 0;
		if (m_ptr->custom_lua_awoke) exec_lua(0, format("custom_monster_awoke(%d,%d,%d)", Ind, c_ptr->m_idx, m_ptr->custom_lua_awoke));
	}

	/* Re-check piercing */
	if (p_ptr->piercing_charged) {
		if (p_ptr->cst < 9) {
			msg_print(Ind, "Not enough stamina to execute an assassinating attack.");
			p_ptr->piercing_charged = FALSE;
			p_ptr->piercing = 0;
		} else {
			use_stamina(p_ptr, 9);
			p_ptr->redraw |= PR_STAMINA;
		}
	}

#ifdef TARGET_SWITCHING_COST
	/* Hack for 'old' attack (bumping into enemy): Skip one attack. */
	if (old == 2) num++;
#endif

	/* Attack once for each legal blow */
	while (num++ < p_ptr->num_blow) {
		u32b f1 = 0, f2 = 0, f3 = 0, f4 = 0, f5 = 0, f6 = 0, esp = 0;

		did_stun = FALSE;
		did_knee = FALSE;
		did_slow = FALSE;

#ifdef USE_SOUND_2010
		if (p_ptr->cut_sfx_attack) {
			sfx = (extract_energy[p_ptr->pspeed] / 10) * p_ptr->num_blow;
			if (sfx) {
				p_ptr->count_cut_sfx_attack += 10000 / sfx;
				if (p_ptr->count_cut_sfx_attack >= 250) { /* 100 / 25 = 4 blows per turn */
					p_ptr->count_cut_sfx_attack -= 250;
					if (p_ptr->count_cut_sfx_attack >= 250) p_ptr->count_cut_sfx_attack = 0;
					sfx = 0;
				}
			}
		}
		if (p_ptr->half_sfx_attack && sfx == 0) {
			if (p_ptr->half_sfx_attack_state) sfx = -1;
			p_ptr->half_sfx_attack_state = !p_ptr->half_sfx_attack_state;
		}
#endif

		/* Access the weapon. Added dual-mode check:
		   Only use secondary weapon if we're not in main-hand mode!
		   This is to prevent bad Nazgul accidents where both weapons go poof
		   although character wasn't in dual-mode. - C. Blue */
		if (!primary_wield && secondary_wield && p_ptr->dual_mode) slot = INVEN_ARM;
		else slot = INVEN_WIELD;

		if (dual_wield) {
			switch (dual_stab) {
			case 0: if (magik(50)) slot = INVEN_ARM; break; /* not in a situation to dual-stab */
			case 1:	if (magik(50)) { /* we may dual-stab, randomly pick 1st or 2nd weapon.. */
					slot = INVEN_ARM;
					dual_stab = 3;
				} else {
					dual_stab = 2;
				}
				break;
			case 2: slot = INVEN_ARM; /* and switch to opposite weapon in the second attack.. */
			/* Fall through */
			case 3: dual_stab = 4; break; /* becomes 0 at end of attack, disabling further dual-stabs */
			}
		}

		o_ptr = &p_ptr->inventory[slot];
#if defined(WIELD_BOOKS) || defined(WIELD_DEVICES)
		/* We _are_ wielding an item, but it does _not_ count in any way for melee attacking?
		   (Unlike for exaple a Mage Staff, which does count; it has dice and can be enchanted even.)
		   Then hack it to point to an empty item (same as an empty wield slot): */
		if (!primary_wield && slot == INVEN_WIELD && o_ptr->tval) o_ptr = &forge_zero;
#endif

		/* Manage backstabbing and 'flee-stabbing' */
		if (stab_skill && /* Need appropriate melee weapon or martial arts to backstab */
		    p_ptr->mon_vis[c_ptr->m_idx] &&
		    (rogue_armed_melee(o_ptr, p_ptr) ||
		    (martial && r_ptr->body_parts[BODY_HEAD] && r_ptr->body_parts[BODY_TORSO]))) {
			if (sleep_stab || cloaked_stab || shadow_stab) { /* Note: Cloaked backstab takes precedence over backstabbing a fleeing monster */
				backstab = TRUE;
				m_ptr->backstabbed = 1;
			} else if (m_ptr->monfear /*&& m_ptr->ml)*/) {
				stab_fleeing = TRUE;
			}
		}

		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
		chaos_effect = 0; // we need this methinks..?

		if ((f4 & TR4_NEVER_BLOW)) {
			msg_print(Ind, "You can't attack with that weapon.");
			break;
		}

		/* check whether this weapon or we in general, are vampiric */
		if ((slot == INVEN_WIELD && p_ptr->melee_brand && p_ptr->melee_brand_t == TBRAND_VAMPIRIC) ||
		    (slot == INVEN_ARM && p_ptr->melee_brand2 && p_ptr->melee_brand2_t == TBRAND_VAMPIRIC))
			vampiric_melee = 100;
		else if (f1 & TR1_VAMPIRIC) vampiric_melee = 100; /* weapon chance is always 100% */
		else vampiric_melee = p_ptr->vampiric_melee; /* non-weapon chance from other items is applied from xtra1.c */

		/* Calculate the "attack quality" */
		bonus = p_ptr->to_h + p_ptr->to_h_melee + o_ptr->to_h;
		chance = (p_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));
		if (p_ptr->blind) chance >>= 1;
//s_printf("M chance %d, skill_thn %d, bonus %d\n", chance, p_ptr->skill_thn, bonus);//DEBUG hit chance

		/* Plan ahead if a missed attack would be a blocked or parried one or just an
		   [hitchance-vs-AC-induced] miss. 'Piercing' requires this to be calculated ahead now. */
		block = parry = 0;
		if (strchr("hHJkpPtyn", r_ptr->d_char) && /* leaving out Yeeks (else Serpent Man 'J') */
		    !(r_ptr->flags3 & RF3_ANIMAL)) {
#ifdef USE_PARRYING
			parry = 5 + m_ptr->ac / 10;
#endif
#ifdef USE_BLOCKING
			if (!(r_ptr->flags8 & RF8_NO_BLOCK)) block = 10;
 #ifdef USE_PARRYING
			else parry += 5; /* assuming 2-handed weapon or otherwise greater parrying abilities */
 #endif
#endif
		}
		/* Evaluate: 0 = no, other values = yes */
#ifdef USE_BLOCKING
		if (helpless || !magik(block)) block = 0;
#endif
#ifdef USE_PARRYING
		if (helpless || !magik(parry)) parry = 0;
#endif

		p_ptr->test_attacks++;
		/* Test for hit */
		if (p_ptr->instakills || backstab ||
		    test_hit_melee(chance, m_ptr->ac, p_ptr->mon_vis[c_ptr->m_idx]) ||
		    (p_ptr->piercing && !block && !parry)) {
			/* handle 'piercing' countdown */
			if (p_ptr->piercing) {
				p_ptr->piercing_charged = FALSE;
				p_ptr->piercing -= 1000 / p_ptr->num_blow;
				/* since above division is rounded down, discard remainder, just to 'clean up' */
				if (p_ptr->piercing < 1000 / p_ptr->num_blow) p_ptr->piercing = 0;
			}

#ifdef USE_SOUND_2010
			if (sfx == 0 && p_ptr->sfx_combat) {
				if (o_ptr->k_idx && (is_melee_weapon(o_ptr->tval) ||
 #ifdef EQUIPPABLE_DIGGERS
				    o_ptr->tval == TV_DIGGING ||
 #endif
				    o_ptr->tval == TV_MSTAFF))
					switch (o_ptr->tval) {
					case TV_SWORD: sound(Ind, "hit_sword", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
					case TV_BLUNT:	if (o_ptr->sval == SV_WHIP) sound(Ind, "hit_whip", "hit_weapon", SFX_TYPE_ATTACK, FALSE);
							else sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_ATTACK, FALSE);
							break;
					case TV_AXE: sound(Ind, "hit_axe", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
					case TV_POLEARM: sound(Ind, "hit_polearm", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
 #ifdef EQUIPPABLE_DIGGERS
					case TV_DIGGING:
 #endif
					case TV_MSTAFF: sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
					}
				else
					sound(Ind, "hit", NULL, SFX_TYPE_ATTACK, FALSE);
			}
#else
			sound(Ind, SOUND_HIT);
#endif
			sprintf(hit_desc, "You hit %s", m_name);

#ifdef ENABLE_BLOOD_FRENZY
			/* Blood frenzy? */
			if (blood_frenzy_ok) {
				if (bleeding) {
					p_ptr->blood_frenzy_rage += blood_frenzy_step;
					/* Enter frenzy after spilling enough blood */
					if (p_ptr->blood_frenzy_rage >= 1000) {
						p_ptr->blood_frenzy_rage = 1000;
						if (!p_ptr->blood_frenzy_active) {
							p_ptr->blood_frenzy_active = TRUE;
							p_ptr->update |= PU_BONUS;
							msg_print(Ind, "\377BYou enter a blood frenzy!");
						}
					}
				}
			} else if (p_ptr->blood_frenzy_active) {
				p_ptr->blood_frenzy_rage = 0; // reset completely (!)
				p_ptr->blood_frenzy_active = FALSE;
				p_ptr->update |= PU_BONUS;
				msg_print(Ind, "\377WYour blood frenzy ceases.");
			}
#endif

			/* Hack -- bare hands do one damage */
			k = 1;

			/* Ghosts get damage relative to level */
			/*
			if (p_ptr->ghost)
				k = p_ptr->lev;
			if (p_ptr->fruit_bat)
				k = p_ptr->lev;
			*/
				//k = p_ptr->lev * ((p_ptr->lev / 10) + 1);

			if (martial) {
				int special_effect = 0, stun_effect = 0, times = 0;
				martial_arts * ma_ptr = &ma_blows[0], * old_ptr = &ma_blows[0];
				int resist_stun = 0;
				int marts = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 50);

				if (r_ptr->flags1 & RF1_UNIQUE) resist_stun += 88;
				if (r_ptr->flags3 & RF3_NO_CONF) resist_stun += 44;
				if (r_ptr->flags3 & RF3_NO_SLEEP) resist_stun += 44;
				if (r_ptr->flags3 & RF3_UNDEAD)
					resist_stun += 88;

				for (times = 0; times < (marts < 7 ? 1 : marts / 7); times++) {
					/* Attempt 'times' */
					do {
						ma_ptr = &ma_blows[rand_int(p_ptr->total_winner ? MAX_MA : MAX_NONWINNER_MA)];
					}
					while ((ma_ptr->min_level > marts)
					    || (randint(marts)<ma_ptr->rchance));

					/* keep the highest level attack available we found */
					if ((ma_ptr->min_level >= old_ptr->min_level) &&
					    !(p_ptr->stun || p_ptr->confused)) {
						old_ptr = ma_ptr;
					} else {
						ma_ptr = old_ptr;
					}
				}

				k = damroll(ma_ptr->dd, ma_ptr->ds);

				if (ma_ptr->effect == MA_KNEE) {
#if 0 /* less message order problems */
					if (r_ptr->flags1 & RF1_MALE) {
						msg_format(Ind, "You hit %s in the groin with your knee!", m_name);
						special_effect = MA_KNEE;
					} else {
						sprintf(hit_desc, ma_ptr->desc, m_name);
						//msg_format(Ind, ma_ptr->desc, m_name);
					}
#else
					if (r_ptr->flags1 & RF1_MALE) special_effect = MA_KNEE;
					sprintf(hit_desc, ma_ptr->desc, m_name);
#endif
				}

				else if (ma_ptr->effect == MA_SLOW) {
#if 0 /* less message order problems */
					if (!((r_ptr->flags2 & RF2_NEVER_MOVE)
					    || strchr("ANUjmeEv$,DdsbBFIJQSXclnw!=?+", r_ptr->d_char))) {
						msg_format(Ind, "You kick %s in the ankle.", m_name);
						special_effect = MA_SLOW;
					} else {
						sprintf(hit_desc, ma_ptr->desc, m_name);
						//msg_format(Ind, ma_ptr->desc, m_name);
					}
#else
					if (!((r_ptr->flags2 & RF2_NEVER_MOVE)
					    || strchr("ANUjmeEv$,DdsbBFIJQSXclnw!=?+", r_ptr->d_char)))
						special_effect = MA_SLOW;
					sprintf(hit_desc, ma_ptr->desc, m_name);
#endif
				} else if (ma_ptr->effect == MA_ROYAL_SLOW) { /* works against U,D,d,J,c,n */
#if 0 /* less message order problems */
					if (!((r_ptr->flags2 & RF2_NEVER_MOVE)
					    || strchr("ANjmeEv$,sbBFIQSXlw!=?+", r_ptr->d_char))) {
						switch (m_name[strlen(m_name) - 1]) {
						case 's': case 'x': case 'z':
							msg_format(Ind, "You strike %s' pressure points.", m_name);
							break;
						default:
							msg_format(Ind, "You strike %s's pressure points.", m_name);
						}
						special_effect = MA_SLOW;
					} else {
						sprintf(hit_desc, ma_ptr->desc, m_name);
						//msg_format(Ind, ma_ptr->desc, m_name);
					}
#else
					if (!((r_ptr->flags2 & RF2_NEVER_MOVE)
					    || strchr("ANjmeEv$,sbBFIQSXlw!=?+", r_ptr->d_char)))
						special_effect = MA_SLOW;
					sprintf(hit_desc, ma_ptr->desc, m_name);
#endif
				} else {
					if (ma_ptr->effect)
						stun_effect = (ma_ptr->effect/2) + randint(ma_ptr->effect/2);

					sprintf(hit_desc, ma_ptr->desc, m_name);
					//msg_format(Ind, ma_ptr->desc, m_name);
				}

#ifdef CRIT_UNBRANDED
				k2 = k;
				k = brand_dam_aux(Ind, NULL, k, -1, m_ptr, FALSE);
				k2 = k - k2; /* remember difference between branded and unbranded dice */

				if (!p_ptr->instakills) {
					do_nazgul(Ind, &k, r_ptr, -1);
					if (k == 0) k2 = 0;
				}

				/* Apply the player damage boni */
				k += p_ptr->to_d + p_ptr->to_d_melee;

				k3 = critical_melee(Ind, marts * (randint(10)), ma_ptr->min_level, k - k2, 0, FALSE, FALSE);
				k3 += k2;
#else
				k = brand_dam_aux(Ind, NULL, -1, k, m_ptr, FALSE);

				if (!p_ptr->instakills) do_nazgul(Ind, &k, r_ptr, -1);

				/* Apply the player damage boni */
				k += p_ptr->to_d + p_ptr->to_d_melee;

				k3 = critical_melee(Ind, marts * (randint(10)), ma_ptr->min_level, k, 0, FALSE, FALSE);
#endif
#ifdef CRIT_VS_VORPAL
				k2 = k; /* remember damage before crit */
#endif
#ifdef CRIT_VS_BACKSTAB
				if (!backstab && !stab_fleeing)
#endif
				k = k3;

				if ((special_effect == MA_KNEE) && (k < m_ptr->hp)) {
					did_knee = TRUE;
					stun_effect = 7 + randint(13);
					resist_stun /= 3;
				}

				else if ((special_effect == MA_SLOW) && (k < m_ptr->hp)) {
					if (!(r_ptr->flags1 & RF1_UNIQUE) &&
					    !((r_ptr->flags2 & RF2_POWERFUL) && (r_ptr->flags8 & RF8_NO_CUT)) &&
					    (randint(marts * 2) > r_ptr->level) &&
					    m_ptr->mspeed > m_ptr->speed - 10) {
						did_slow = TRUE;
						m_ptr->mspeed -= 3 + rand_int(3);
					}
				}

				if (stun_effect && (k < m_ptr->hp)) {
					/* Stun the monster */
					if (r_ptr->flags3 & RF3_NO_STUN) {
						/* nothing:
						msg_format(Ind, "%^s is unaffected.", m_name);*/
					} else if (marts > randint(r_ptr->level + resist_stun + 10)) {
						m_ptr->stunned += (stun_effect + get_skill_scale(p_ptr, SKILL_COMBAT, 3));
						did_stun = TRUE;
					}
				}

				/* Vampiric drain */
				if ((magik(vampiric_melee)) && drainable)
					drain_result = m_ptr->hp;
				else
					drain_result = 0;
			/* Handle normal weapon */
			} else if (o_ptr->k_idx) {
#ifdef ENABLE_STANCES
				int stun_effect, resist_stun;
#endif
				k = damroll(o_ptr->dd, o_ptr->ds);

				/* weapons that inflict little damage, especially of only 1 damage dice,
				   mostly don't cause earthquakes at all */
				if ((p_ptr->impact || (f5 & TR5_IMPACT)) &&
				    500 / (10 + (k - o_ptr->dd) * o_ptr->dd * o_ptr->ds / (o_ptr->dd * (o_ptr->ds - 1) + 1)) < randint(35)) do_quake = TRUE;
				    //5..10..45<randint (9d9..9d5..9d1 hit -> ~70% overall avg chance with Grond)
				    //18 for 6d6 weapon at 6d3.5 roll -> 49% on average
				    //25 for 4d5 weapon at 4d3 roll -> ~29% on average
				    /* I made the new formula above, to get a better chance curve over all the different weapon types. -C. Blue- */
				    /* Some old tries: */
				//    130 - ((k - o_ptr->dd) * o_ptr->dd * o_ptr->ds / (o_ptr->dd * (o_ptr->ds - 1) + 1)) < randint(130)) do_quake = TRUE;
				//    (150 / (10 + k - o_ptr->dd) < 11 - (2 / o_ptr->dd))) do_quake = TRUE;
				//    (150 / (1 + k - o_ptr->dd) < 23 - (2 / o_ptr->dd))) do_quake = TRUE;

#if defined(VORPAL_UNBRANDED) || defined(VORPAL_LOWBRANDED)
				if ((f5 & TR5_VORPAL) && !(r_ptr->flags8 & RF8_NO_CUT) && !rand_int(VORPAL_CHANCE)) vorpal_cut = k; /* save unbranded dice */
				else vorpal_cut = FALSE;
#endif
#ifdef CRIT_UNBRANDED
				k2 = k;
				k = brand_dam_aux(Ind, o_ptr, slot, k, m_ptr, FALSE);
				k2 = k - k2; /* remember difference between branded and unbranded dice */
#else
				k = brand_dam_aux(Ind, o_ptr, slot, k, m_ptr, FALSE);
#endif
#ifdef VORPAL_LOWBRANDED
				if (vorpal_cut) vorpal_cut = (vorpal_cut + k) / 2;
#else
 #ifndef VORPAL_UNBRANDED
				if ((f5 & TR5_VORPAL) && !(r_ptr->flags8 & RF8_NO_CUT) && !rand_int(VORPAL_CHANCE)) vorpal_cut = k; /* save branded dice */
				else vorpal_cut = FALSE;
 #endif
#endif

#ifdef ENABLE_STANCES
				/* apply stun from offensive combat stance */
				if (p_ptr->combat_stance == 2) {
					stun_effect = randint(get_skill_scale(p_ptr, SKILL_MASTERY, 10) + adj_con_fix[p_ptr->stat_ind[A_STR]] / 2) + 1;
					stun_effect /= 2;

					resist_stun = 6 - 300 / (50 + m_ptr->ac); /* 0..5 */
					if (r_ptr->flags1 & RF1_UNIQUE) resist_stun += 10;
					if (r_ptr->flags3 & RF3_NO_CONF) resist_stun += 5;
					if (r_ptr->flags3 & RF3_NO_SLEEP) resist_stun += 5;
					if (r_ptr->flags3 & RF3_UNDEAD)	resist_stun += 10;

					switch (p_ptr->combat_stance_power) {
					case 0: resist_stun = (resist_stun * 5) / 4; break;
					case 1: resist_stun = (resist_stun * 4) / 4; break;
					case 2: resist_stun = (resist_stun * 3) / 4; break;
					case 3: resist_stun = (resist_stun * 2) / 3; break;
					}

					if (stun_effect && !(r_ptr->flags3 & RF3_NO_STUN)) {
						/* Stun the monster */
						if (!magik(90 - (1000 / (r_ptr->level + 50)) + resist_stun)) {
							m_ptr->stunned += (stun_effect + get_skill_scale(p_ptr, SKILL_COMBAT, 3));
							did_stun = TRUE;
						}
					}
				}
#endif

				/* Select a chaotic effect (10% chance) */
				if ((f5 & TR5_CHAOTIC) && !rand_int(10)) {
					if (!rand_int(2)) {
						/* Vampiric (50%) (50%) */
						chaos_effect = 1;
					} else if (!rand_int(1000)) {
						/* Quake (0.050%) (49.975%) */
						chaos_effect = 2;
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

				/* Vampiric drain */
				if (((chaos_effect == 1) ||
				    magik(vampiric_melee)) && drainable)
					drain_result = m_ptr->hp;
				else
					drain_result = 0;

				if (chaos_effect == 2) do_quake = TRUE;

				if (vorpal_cut) msg_format(Ind, "Your weapon cuts deep into %s!", m_name);

				k += o_ptr->to_d;

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

				/* May it clone the monster ? */
				if (((f4 & TR4_CLONE) && randint(1000) == 1)
				    || chaos_effect == 6) {
					msg_format(Ind, "Your weapon clones %s!", m_name);
					multiply_monster(c_ptr->m_idx);
				}

				/* heheheheheh */
				if (!p_ptr->instakills) do_nazgul(Ind, &k, r_ptr, slot);

				/* Apply the player damage boni */
				/* (should this also cancelled by nazgul?(for now not)) */
				k += p_ptr->to_d + p_ptr->to_d_melee;

#ifdef DEFENSIVE_STANCE_TOTAL_MELEE_REDUCTION
				if (p_ptr->combat_stance == 1) {
					if (p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)
						switch (p_ptr->combat_stance_power) {
						case 0: k = (k * 7 + 9) / 10; break;
						case 1: k = (k * 7 + 9) / 10; break;
						case 2: k = (k * 7 + 9) / 10; break;
						case 3: k = (k * 7 + 9) / 10; break;
						}
					else
						switch (p_ptr->combat_stance_power) {
						case 0: k = (k * 6 + 9) / 10; break;
						case 1: k = (k * 7 + 9) / 10; break;
						case 2: k = (k * 7 + 9) / 10; break;
						case 3: k = (k * 7 + 9) / 10; break;
						}
				}
#endif

				/* Critical strike moved here, since it works best
				with light weapons, which have low dice. So for gain
				we need the full damage including all to-dam boni */
#ifdef CRIT_UNBRANDED
				k3 = critical_melee(Ind, o_ptr->weight, o_ptr->to_h, k - k2, calc_crit_obj(o_ptr), rogue_armed_melee(o_ptr, p_ptr), TRUE);
				k3 += k2;
#else
				k3 = critical_melee(Ind, o_ptr->weight, o_ptr->to_h, k, calc_crit_obj(o_ptr), rogue_armed_melee(o_ptr, p_ptr), TRUE);
#endif
#ifdef CRIT_VS_VORPAL
				k2 = k; /* remember damage before crit */
#endif
#ifdef CRIT_VS_BACKSTAB
				if (!backstab && !stab_fleeing)
#endif
				k = k3;

				/* penalty for weapons in bat form */
				if (p_ptr->body_monster == RI_VAMPIRE_BAT) k /= 2;
			/* handle bare fists/bat/ghost */
			} else {
				k = brand_dam_aux(Ind, NULL, -1, k, m_ptr, FALSE);

				if (!p_ptr->instakills) do_nazgul(Ind, &k, r_ptr, -1);

				/* Apply the player damage boni */
				/* (should this also cancelled by nazgul? not for now) */
				k += p_ptr->to_d + p_ptr->to_d_melee;

				k3 = k;

				/* Vampiric drain */
				if ((magik(vampiric_melee)) && drainable)
					drain_result = m_ptr->hp;
				else
					drain_result = 0;
			}

			/* No negative damage */
			if (k < 0) k = 0;

			/* New backstab formula: it works like criticals now and takes a bit of monster hp into account */
			/* Note that the multiplier is after all the damage calc is done! So may need tweaking! */
			if (backstab || stab_fleeing) {
				int bs_flat = m_ptr->hp / (dual_stab ? 30 : 20);
				int bs_multiplier = 100 + get_skill_scale(p_ptr, SKILL_BACKSTAB, 350 + rand_int(101));

				/* similar to dodging, the effect is weaker if the skill level doesn't match the monster level */
				bs_flat = (bs_flat * bs_skill) / (r_ptr->level >= bs_skill ? r_ptr->level : bs_skill);

				kl = k * bs_multiplier;
				kl /= (dual_stab ? 150 : 100);
				kl += bs_flat;
#ifdef CRIT_VS_BACKSTAB
				if (k3 > kl) {
					backstab = stab_fleeing = FALSE;
					k = k3;
				} else
#endif
				k = kl;

#ifdef CRIT_VS_VORPAL
				kl = k2 * bs_multiplier;
				kl /= (dual_stab ? 150 : 100);
				kl += bs_flat;
				k2 = kl;
#endif

#if 1
				/* Experimental: Backstabbing skill can apply stun effect (currently PvE only). */
				if (backstab && !did_stun && !(r_ptr->flags3 & RF3_NO_STUN) && get_skill(p_ptr, SKILL_BACKSTAB)) {
					/* For rogues: Nerve-point based stun ;) uses DEX instead of STR, and Backstabbing skill, only slightly Weaponmastery. */
					int stun_effect, resist_stun;

					resist_stun = 6 - 300 / (50 + m_ptr->ac); /* 0..5 */
					if (r_ptr->flags1 & RF1_UNIQUE) resist_stun += 10;
					/* NO_CONF and NO_SLEEP have no effect vs Backstabbing */
					if (r_ptr->flags3 & RF3_UNDEAD)	resist_stun += 10;

					/* Combat stances have no effect, Backstabbing is sort of its own 'stance'.. */

					/* Stun the monster? */
					stun_effect = 110 - (2270 / (r_ptr->level + 100)) + resist_stun;//88..126
					stun_effect -= get_skill_scale(p_ptr, SKILL_BACKSTAB, 100);
					if (rand_int(100) > stun_effect) { //-12..26
						/* Omae wa mou shindeiru */
						stun_effect = (((weapon ? get_skill_scale(p_ptr, SKILL_MASTERY, 5) : 0) + adj_con_fix[p_ptr->stat_ind[A_DEX]] + 1) * (50 + rand_int(51) - (r_ptr->level / 3))) / 100;
						if (stun_effect) {
							m_ptr->stunned += stun_effect;
							did_stun = TRUE; /* Nani? */
						}
					}
				}
#endif
			}

			/* Vorpal bonus - multi-dice!
			   (currently +31.25% more branded dice damage on total average, just for the records) */
			if (vorpal_cut) {
#ifdef CRIT_VS_VORPAL
				k2 += (magik(25) ? 2 : 1) * (vorpal_cut + 5); /* exempts critical strike */
				/* either critical hit or vorpal, not both */
				if (k2 > k) k = k2;
#else
				k += (magik(25) ? 2 : 1) * (vorpal_cut + 5); /* exempts critical strike */
#endif
			}

			if (m_ptr->r_idx == RI_MIRROR) k = (k * MIRROR_REDUCE_DAM_TAKEN_MELEE + 99) / 100;

			/* for admins: kill a target in one hit */
			if (p_ptr->instakills) k = m_ptr->hp + 1;
			else if (p_ptr->admin_godly_strike) {
				p_ptr->admin_godly_strike--;
				if (!(r_ptr->flags1 & RF1_UNIQUE)) k = m_ptr->hp + 1;
			}

			/* Don't apply effects if monster just dies from this attack anyway */
			if (k > m_ptr->hp) did_stun = did_knee = did_slow = FALSE;

			/* DEG Updated hit message to include damage */
			if (backstab) {
				backstab_feed = TRUE;
				backstab = FALSE;
				if (martial) {
					if (r_ptr->flags1 & RF1_UNIQUE) {
						msg_format(Ind, "\377%cYou twist the neck of %s%s for \377e%d \377%cdamage.", uniq, sleep_stab ? "the sleeping " : "", m_name_raw, k, uniq);
						if (uniq_bell) Send_beep(Ind);
					}
					else msg_format(Ind, "You twist the neck of the %s%s for \377p%d \377wdamage.", sleep_stab ? "the sleeping " : "", m_name_raw, k);
				} else {
					if (r_ptr->flags1 & RF1_UNIQUE) {
						msg_format(Ind, "\377%cYou stab %s%s for \377e%d \377%cdamage.", uniq, sleep_stab ? "the sleeping " : "", m_name_raw, k, uniq);
						if (uniq_bell) Send_beep(Ind);
					}
					else msg_format(Ind, "You stab %s%s for \377p%d \377wdamage.", sleep_stab ? "the sleeping " : "", m_name_raw, k);
				}
			}
			else if (stab_fleeing) {
				stab_fleeing = FALSE;
				if (martial) {
					if (r_ptr->flags1 & RF1_UNIQUE) {
						msg_format(Ind, "\377%cYou strike the back of %s for \377e%d \377%cdamage.", uniq, m_name, k, uniq);
						if (uniq_bell) Send_beep(Ind);
					}
					else msg_format(Ind, "You strike the back of %s for \377p%d \377wdamage.", m_name, k);
				} else {
					if (r_ptr->flags1 & RF1_UNIQUE) {
						msg_format(Ind, "You backstab the fleeing %s for \377e%d \377wdamage.", m_name_raw, k);
						if (uniq_bell) Send_beep(Ind);
					}
					else msg_format(Ind, "\377%cYou backstab the fleeing %s for \377p%d \377%cdamage.", uniq, m_name_raw, k, uniq);
				}
			}
			//else if ((r_ptr->flags1 & RF1_UNIQUE) && (!martial)) msg_format(Ind, "You hit %s for \377p%d \377wdamage.", m_name, k);
			//else if (!martial) msg_format(Ind, "You hit %s for \377g%d \377wdamage.", m_name, k);
			else {
				if (r_ptr->flags1 & RF1_UNIQUE) {
					msg_format(Ind, "\377%c%s for \377e%d \377%cdamage.", uniq, hit_desc, k, uniq);
					if (uniq_bell) Send_beep(Ind);
				}
				else msg_format(Ind, "%s for \377g%d \377wdamage.", hit_desc, k);
			}

			if (did_stun) {
				if (m_ptr->stunned > 100)
					msg_format(Ind, "\377y%^s is knocked out.", m_name);
				else if (m_ptr->stunned > 50)
					msg_format(Ind, "\377y%^s is heavily stunned.", m_name);
				else
					msg_format(Ind, "\377y%^s is stunned.", m_name);
			}
			if (did_knee) msg_format(Ind, "%^s moans in agony!", m_name);
			if (did_slow) msg_format(Ind, "\377o%^s starts limping slower.", m_name);


			/* target dummy */
			if (m_ptr->r_idx == RI_TARGET_DUMMY1 || m_ptr->r_idx == RI_TARGET_DUMMY2) {
				/* Hack: Reduce snow on it during winter season :) */
				m_ptr->extra -= 5;
				if (m_ptr->extra < 0) m_ptr->extra = 0;
					if ((m_ptr->r_idx == RI_TARGET_DUMMY2) && (m_ptr->extra < 30)) {
					m_ptr->r_idx = RI_TARGET_DUMMY1;
					everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
				}
			}
			if (m_ptr->r_idx == RI_TARGET_DUMMYA1 || m_ptr->r_idx == RI_TARGET_DUMMYA2) {
				/* Hack: Reduce snow on it during winter season :) */
				m_ptr->extra -= 5;
				if (m_ptr->extra < 0) m_ptr->extra = 0;
					if ((m_ptr->r_idx == RI_TARGET_DUMMYA2) && (m_ptr->extra < 30)) {
					m_ptr->r_idx = RI_TARGET_DUMMYA1;
					everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
				}
			}

			/* Damage, check for fear and death */
			feed = m_ptr->maxhp + 100;
			p_ptr->vamp_fed_midx = c_ptr->m_idx;
			if (mon_take_hit(Ind, c_ptr->m_idx, k, &fear, NULL)) {

				/* Vampires feed off the life force! (if any) */
				// mimic forms for vampires/bats: 432, 520, 521, 623, 989
				if (p_ptr->prace == RACE_VAMPIRE && drainable) {
					//feed = (4 - (300 / feed)) * 1000;//1000..4000
					feed = (6 - (300 / feed)) * 100;//300..600
					if (r_ptr->flags3 & RF3_DEMON) feed /= 2;
					if (r_ptr->d_char == 'A') feed /= 3;
					if (backstab_feed) feed *= 2;
					set_food(Ind, feed + p_ptr->food);
				}

#ifdef USE_SOUND_2010
				/* hack: always play 'hit' sfx for final killing hit,
				   so if we didn't play it already (we did so if sfx==0) then play it now instead. */
				if (sfx && p_ptr->sfx_combat) {
					if (o_ptr->k_idx && (is_melee_weapon(o_ptr->tval) ||
 #ifdef EQUIPPABLE_DIGGERS
					    o_ptr->tval == TV_DIGGING ||
 #endif
					    o_ptr->tval == TV_MSTAFF))
						switch (o_ptr->tval) {
						case TV_SWORD: sound(Ind, "hit_sword", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
						case TV_BLUNT:	if (o_ptr->sval == SV_WHIP) sound(Ind, "hit_whip", "hit_weapon", SFX_TYPE_ATTACK, FALSE);
								else sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_ATTACK, FALSE);
								break;
						case TV_AXE: sound(Ind, "hit_axe", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
						case TV_POLEARM: sound(Ind, "hit_polearm", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
 #ifdef EQUIPPABLE_DIGGERS
						case TV_DIGGING:
 #endif
						case TV_MSTAFF: sound(Ind, "hit_blunt", "hit_weapon", SFX_TYPE_ATTACK, FALSE); break;
						}
					else
						sound(Ind, "hit", NULL, SFX_TYPE_ATTACK, FALSE);
				}
//#else
//				sound(Ind, SOUND_HIT);
#endif

				/* Exploding Attack - Kurzel */
				if (p_ptr->nimbus) do_nimbus(Ind, y, x);

				fear = FALSE; /* paranoia */
				break; /* monster is dead */
			}

#ifdef ENABLE_OHERETICISM
			/* Extend ongoing 'Boundless Hate' spell thanks to Traumaturgy feedback? */
			if (p_ptr->hate_prolong == 1) p_ptr->hate_prolong = 2;
#endif

			touch_zap_player(Ind, c_ptr->m_idx);
			/* todo maybe: handle player death from getting zapped? */

			/* Apply effects from mimic monster forms */
			if (p_ptr->body_monster) {
				/* If monster is fighting with a weapon, the player gets the effect(s) even with a weapon */
				/* If monster is fighting without weapons, the player gets the effect(s) only if
				he fights with bare hands/martial arts */
				if (!pr_ptr->body_parts[BODY_WEAPON])
					if (o_ptr->k_idx) apply_monster_effects = FALSE;

				/* Get monster effects. If monster has several, choose one randomly */
				monster_effects = 0;
				for (i = 0; i < 4; i++) {
					if (pr_ptr->blow[i].d_dice && pr_ptr->blow[i].d_side) {
						monster_effects++;
						monster_effect[monster_effects] = pr_ptr->blow[i].effect;
					}
				}
				/* Choose random brand from the ones available */
				monster_effect_chosen = monster_effect[1 + rand_int(monster_effects)];

				/* Modify damage effect */
				if (apply_monster_effects) {
					switch (monster_effect_chosen) {
					case RBE_BLIND:
						if (r_ptr->flags3 & RF3_NO_SLEEP) break;
						/* Fall through */
					case RBE_HALLU:
						if (r_ptr->flags3 & RF3_NO_CONF) break;
						/* Fall through */
					case RBE_CONFUSE:
						if (!p_ptr->confusing) {
							/* Confuse the monster */
							if (r_ptr->flags3 & RF3_NO_CONF) {
#ifdef OLD_MONSTER_LORE
								if (p_ptr->mon_vis[c_ptr->m_idx]) r_ptr->r_flags3 |= RF3_NO_CONF;
#endif
								msg_format(Ind, "%^s is unaffected.", m_name);
							} else if (rand_int(100) < r_ptr->level) {
								msg_format(Ind, "%^s resists the effect.", m_name);
							} else {
								msg_format(Ind, "%^s appears confused.", m_name);
								m_ptr->confused += 10 + rand_int(p_ptr->lev) / 5 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 5));
							}
						}
						break;
					case RBE_TERRIFY:
						fear_chance = 50 + (p_ptr->lev - r_ptr->level) * 5;
						if (!(r_ptr->flags3 & RF3_NO_FEAR) && rand_int(100) < fear_chance) {
							msg_format(Ind, "%^s appears afraid.", m_name);
							m_ptr->monfear = m_ptr->monfear + 4 + rand_int(p_ptr->lev) / 5 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 5));
							m_ptr->monfear_gone = 0;
						}
						break;
					case RBE_PARALYZE:
						if (!p_ptr->stunning) {
							/* Stun the monster */
							if (r_ptr->flags3 & RF3_NO_STUN) {
#ifdef OLD_MONSTER_LORE
								if (p_ptr->mon_vis[c_ptr->m_idx]) r_ptr->r_flags3 |= RF3_NO_STUN;
#endif
								msg_format(Ind, "%^s is unaffected.", m_name);
							} else if (rand_int(115) < r_ptr->level) {
								msg_format(Ind, "%^s resists the effect.", m_name);
							} else {
								if (!m_ptr->stunned)
									msg_format(Ind, "\377o%^s appears stunned.", m_name);
								else
									msg_format(Ind, "\377o%^s appears more stunned.", m_name);
								m_ptr->stunned += 20 + rand_int(p_ptr->lev) / 5 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 5));
							}
						}
						break;
					}
				}
			}

			/* VAMPIRIC: Are we draining it?  A little note: If the monster is
			   dead, the drain does not work... */
			if (drain_result) {
				drain_result -= m_ptr->hp;  /* Calculate the difference */

				if (drain_result > 0) { /* Did we really hurt it? */
					drain_heal = randint(2) + damroll(2,(drain_result / 16));/* was 4,../6 -- was 8 for 50 max_drain */

					if (drain_left) {
						if (drain_heal < drain_left) {
#if 0
							drain_left -= drain_heal;
#endif
						} else {
							drain_heal = drain_left;
#if 0
							drain_left = 0;
#endif
						}

						if (drain_msg) {
							if (martial || !o_ptr->k_idx) {
#ifndef TEST_SERVER
								if (is_admin(p_ptr)) /* for debugging purpose */
#endif
								msg_format(Ind, "Your hits drain \377o%d\377w life from %s!", drain_heal, m_name);
#ifndef TEST_SERVER
								else
								msg_format(Ind, "Your hits drain life from %s!", m_name);
#endif
							} else {
#ifndef TEST_SERVER
								if (is_admin(p_ptr))
#endif
								msg_format(Ind, "Your weapon drains \377o%d\377w life from %s!", drain_heal, m_name);
#ifndef TEST_SERVER
								else
								msg_format(Ind, "Your weapon drains life from %s!", m_name);
#endif
							}
#if 0
							drain_msg = FALSE;
#endif
						}

						if (m_ptr->r_idx == RI_BLUE) drain_heal >>= 1;
						i = p_ptr->chp;
						hp_player(Ind, drain_heal, TRUE, TRUE);
						p_ptr->test_drain += (p_ptr->chp - i);
						/* We get to keep some of it! */
					}
				}
			}

			/* Confusion attack */
			if (p_ptr->confusing || chaos_effect == 3) {
				/* Cancel glowing hands */
				if (p_ptr->confusing) p_ptr->confusing--;

				/* Message */
				if (!p_ptr->confusing) msg_print(Ind, "Your hands stop glowing.");

				/* Confuse the monster */
				if (r_ptr->flags3 & RF3_NO_CONF) {
#ifdef OLD_MONSTER_LORE
					if (p_ptr->mon_vis[c_ptr->m_idx]) r_ptr->r_flags3 |= RF3_NO_CONF;
#endif
					msg_format(Ind, "%^s is unaffected.", m_name);
				} else if (rand_int(100) < r_ptr->level) {
					msg_format(Ind, "%^s resists the effect.", m_name);
				} else {
					msg_format(Ind, "%^s appears confused.", m_name);
					m_ptr->confused += 10 + rand_int(p_ptr->lev) / 5 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 5));
				}
			}
			if (chaos_effect == 4) {
				if (!(r_ptr->flags9 & RF9_IM_TELE) &&
				    !(r_ptr->flags3 & RF3_RES_TELE) &&
				    !(r_ptr->flags1 & RF1_UNIQUE)) {
					if (m_ptr->level > randint(100)) {
						if (teleport_away(c_ptr->m_idx, 50)) {
							msg_format(Ind, "%^s disappears!", m_name);
							num = p_ptr->num_blow + 1; /* Can't hit it anymore! */
						}
					} else msg_format(Ind, "%^s resists the effect.", m_name);
					//no_extra = TRUE;
				} else msg_format(Ind, "%^s is unaffected.", m_name);
			}
/*			else if (chaos_effect == 5 && cave_floor_bold(zcave,y,x)
			    && (randint(90) > m_ptr->level))*/
			else if (chaos_effect == 5) {
				if (!((r_ptr->flags1 & RF1_UNIQUE) ||
				    (r_ptr->flags4 & RF4_BR_CHAO) ||
				    (r_ptr->flags9 & RF9_IM_TELE) ||
				    (r_ptr->flags9 & RF9_RES_CHAOS) ))
				    //|| (m_ptr->mflag & MFLAG_QUEST)))
				    {
					if (randint(150) > m_ptr->level) {
						int tmp = poly_r_idx(m_ptr->r_idx);

						/* Pick a "new" monster race */

						/* Handle polymorph */
						if (tmp != m_ptr->r_idx) {
							int cl = m_ptr->clone, cls = m_ptr->clone_summoning;

							msg_format(Ind, "%^s changes!", m_name);

							/* "Kill" the "old" monster */
							delete_monster_idx(c_ptr->m_idx, TRUE);

							/* Create a new monster (no groups) */
							(void)place_monster_aux(wpos, y, x, tmp, FALSE, FALSE, cl, cls);

							/* XXX XXX XXX Hack -- Assume success */

							/* Hack -- Get new monster */
							m_ptr = &m_list[c_ptr->m_idx];

							/* Oops, we need a different name... */
							monster_desc(Ind, m_name, c_ptr->m_idx, 0);

							/* Hack -- Get new race */
							r_ptr = race_inf(m_ptr);

							fear = FALSE;
						}
					}
					else msg_format(Ind, "%^s resists the effect.", m_name);
				} else msg_format(Ind, "%^s is unaffected.", m_name);
			}

			/* Stunning attack */
			if (p_ptr->stunning) {
				/* Cancel heavy hands */
				p_ptr->stunning = FALSE;

				/* Message */
				msg_print(Ind, "Your hands feel less heavy.");

				/* Stun the monster */
				if (r_ptr->flags3 & RF3_NO_STUN) {
#ifdef OLD_MONSTER_LORE
					if (p_ptr->mon_vis[c_ptr->m_idx]) r_ptr->r_flags3 |= RF3_NO_STUN;
#endif
					msg_format(Ind, "%^s is unaffected.", m_name);
				} else if (rand_int(115) < r_ptr->level) {
					msg_format(Ind, "%^s resists the effect.", m_name);
				} else {
					if (!m_ptr->stunned)
						msg_format(Ind, "\377o%^s appears stunned.", m_name);
					else
						msg_format(Ind, "\377o%^s appears more stunned.", m_name);
					m_ptr->stunned += 20 + rand_int(p_ptr->lev) / 5 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 5));
				}
			}

			/* Ghosts get fear attacks */
			i = get_skill(p_ptr, SKILL_AURA_FEAR); // O_+ "Fear Brand" - Kurzel
			if (p_ptr->ghost || (p_ptr->aura[AURA_FEAR] && i >= 20 && magik(i))) {
				fear_chance = 50 + (p_ptr->lev - r_ptr->level) * 5;
				if (!(r_ptr->flags3 & RF3_NO_FEAR) && rand_int(100) < fear_chance) {
					msg_format(Ind, "%^s appears afraid.", m_name);
					m_ptr->monfear = m_ptr->monfear + 4 + rand_int(p_ptr->lev) / 5 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 5));
					m_ptr->monfear_gone = 0;
				}
			}

			/* Apply Poison technique used with non-standard poison potion for special fx */
			if (p_ptr->melee_brand_flags & TBRAND_F_POTION_MUSHROOM) potion_mushroom_branding(Ind, m_ptr->fx, m_ptr->fy, p_ptr->melee_brand_t, FALSE);

#if 0 /* moved to xtra1.c */
			/* Fruit bats get life stealing.
			   Note: This is ok because fruit bats cannot wear weapons/gloves as a
			   source of vampirism.
			   Still to check: Vampire fruit bats! CURRENTLY STACKS! */
			if (p_ptr->fruit_bat == 1 && !p_ptr->body_monster && drainable) {
				int leech = m_ptr->hp;

				if (k < leech) leech = k;
				leech /= 10;
				hp_player(Ind, rand_int(leech), TRUE, TRUE);
			}
#endif

			/* Exploding Attack - Kurzel */
			if (p_ptr->nimbus) do_nimbus(Ind, y, x);
			/* Have to check if monster died from the nimbus projection... */
			if (!m_ptr->r_idx) {
				fear = FALSE; /* paranoia */
				break; /* monster is dead */
			}
		}
		/* Player misses */
		else {
			backstab = stab_fleeing = FALSE;

			/* Message */
//TODO: dodges your attack? ninjas, cats!
			if (block) {
				sprintf(hit_desc, "\377%c%s blocks.", COLOUR_BLOCK_MON, m_name);
				hit_desc[2] = toupper(hit_desc[2]);
				msg_print(Ind, hit_desc);
#ifdef USE_SOUND_2010
				if (sfx == 0 && p_ptr->sfx_defense) sound(Ind, "block_shield", NULL, SFX_TYPE_ATTACK, FALSE);
#endif
			} else if (parry) {
				sprintf(hit_desc, "\377%c%s parries.", COLOUR_PARRY_MON, m_name);
				hit_desc[2] = toupper(hit_desc[2]);
				msg_print(Ind, hit_desc);
#ifdef USE_SOUND_2010
				if (sfx == 0 && p_ptr->sfx_defense) sound(Ind, "parry_weapon", "parry", SFX_TYPE_ATTACK, FALSE);
#endif
			} else {
				msg_format(Ind, "You miss %s.", m_name);
#ifdef USE_SOUND_2010
				if (sfx == 0 && p_ptr->sfx_combat) {
					if (o_ptr->k_idx && is_melee_weapon(o_ptr->tval))
						sound(Ind, "miss_weapon", "miss", SFX_TYPE_ATTACK, FALSE);
					else
						sound(Ind, "miss", NULL, SFX_TYPE_ATTACK, FALSE);
				}
#else
				sound(Ind, SOUND_MISS);
#endif
			}
		}

		/* hack for dual-backstabbing: get a free b.p.r.
		   (needed as workaround for sleep-dual-stabbing executed
		   by auto-retaliator, where old-check below would otherwise break) - C. Blue */
		if (dual_stab == 4) dual_stab = 0;
		if (!dual_stab) p_ptr->melee_timeout_crit_dual = sleep_stab = cloaked_stab = shadow_stab = FALSE;
		if (dual_stab) {
			num--;
			continue;
		}

		/* Hack -- divided turn for auto-retaliator */
		if (!old) break;
	}

	/* Hack -- delay fear messages */
	if (fear && p_ptr->mon_vis[c_ptr->m_idx] && !(m_ptr->csleep || m_ptr->stunned > 100)) {
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

	/* Mega-Hack -- apply earthquake brand */
	if (do_quake && !p_ptr->quaked && magik(QUAKE_CHANCE)) {
		if (o_ptr->k_idx
#ifdef ALLOW_NO_QUAKE_INSCRIPTION
		    && !check_guard_inscription(o_ptr->note, 'Q')
#else
		    && (!check_guard_inscription(o_ptr->note, 'Q') ||
		    o_ptr->name1 != ART_GROND)
#endif
		    ) {
			/* Giga-Hack -- equalize the chance (though not likely..) */
			if (old || randint(p_ptr->num_blow) < 3) {
				earthquake(&p_ptr->wpos, p_ptr->py, p_ptr->px, 5);
				p_ptr->quaked = TRUE;
			}
		}
	}

	suppress_message = FALSE;
}

void py_bash(int Ind, int y, int x) {
	cave_type	**zcave;
	cave_type	*c_ptr;

	if (!(zcave = getcave(&Players[Ind]->wpos))) return;
	c_ptr = &zcave[y][x];

	/* Check for monster */
	if (c_ptr->m_idx > 0)
		py_bash_mon(Ind, y, x);
	/* Check for player */
	else if (c_ptr->m_idx < 0 && cfg.use_pk_rules != PK_RULES_NEVER)
		py_bash_py(Ind, y, x);
}

/* Bash a monster for stun effect and a little damage, costs stamina to avoid stun-spam into k.o.. - C. Blue
   Simple handling: No brands, cannot crit, no vampirism, no quake, just STR, shield weight, combat skill.
   Also, independant of combat stance actually - we perceive the shield bash more as a fighting technique than an attack. */
void py_bash_mon(int Ind, int y, int x) {
	player_type	*p_ptr = Players[Ind];
	int		stun_effect, chance, owner_Ind = 0, resist_stun = 0;
	int		k;

	char		m_name[MNAME_LEN], hit_desc[MAX_CHARS_WIDE];
	monster_type	*m_ptr;
	monster_race	*r_ptr;

	bool		fear = FALSE, did_stun = FALSE;

	int		block;

	bool		helpless, uniq_bell = FALSE;
	char		uniq = 'w';

	struct worldpos	*wpos = &p_ptr->wpos;
	cave_type	**zcave;
	cave_type	*c_ptr;

#ifndef NEW_SHIELDS_NO_AC
	int mon_acid = 0, mon_fire = 0;
#endif
	int i;


	/* Hack -- suppress messages */
	if (p_ptr->taciturn_messages) suppress_message = TRUE;

	if (!(p_ptr->melee_techniques & MT_BASH)) {
		msg_print(Ind, "\377yYou are not proficient in shield-bashing opponents.");
		return;
	}

	if (!p_ptr->num_blow) {
		msg_print(Ind, "\377yYou cannot attack.");
		return;
	}

	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

	m_ptr = &m_list[c_ptr->m_idx];
	r_ptr = race_inf(m_ptr);
	helpless = (m_ptr->csleep || m_ptr->stunned > 100 || m_ptr->confused);

	/* Disturb the player */
	disturb(Ind, 0, 0);

	/* Extract monster name (or "it") */
	monster_desc(Ind, m_name, c_ptr->m_idx, 0);

	/* Auto-Recall if possible and visible */
	if (p_ptr->mon_vis[c_ptr->m_idx]) recent_track(m_ptr->r_idx);
	/* Track a new monster */
	if (p_ptr->mon_vis[c_ptr->m_idx]) health_track(Ind, c_ptr->m_idx);

	if (m_ptr->status & M_STATUS_FRIENDLY) return;

	/* is it a unique we already got kill credit for? */
	if ((r_ptr->flags1 & RF1_UNIQUE) &&
	    p_ptr->r_killed[m_ptr->r_idx] == 1) {
		uniq = 'D';
		if (p_ptr->warn_unique_credit) uniq_bell = TRUE;
	}

	/* can't attack while in WRAITHFORM */
	/* wraithed players can attack wraithed monsters - mikaelh */
	if (p_ptr->tim_wraith && !is_admin(p_ptr) &&
	    ((r_ptr->flags2 & RF2_KILL_WALL) || !(r_ptr->flags2 & RF2_PASS_WALL))) /* lil fix (Morgoth) - C. Blue */
		return;

	p_ptr->energy -= level_speed(&p_ptr->wpos);

	/* try to find its owner online */
	if (m_ptr->owner)
		for (i = NumPlayers; i > 0; i--)
			if (m_ptr->owner == Players[i]->id) {
				owner_Ind = i;
				break;
			}

#if 0 /* golems get attacked by other players */
	if ((m_ptr->owner == p_ptr->id && !p_ptr->confused &&
		p_ptr->mon_vis[c_ptr->m_idx]) ||
		(m_ptr->owner != p_ptr->id && m_ptr->pet)) //dont kill pets either, meanie!
#else /* prevent golems being attacked by other players */
	if ((m_ptr->owner == p_ptr->id && !p_ptr->confused && p_ptr->mon_vis[c_ptr->m_idx]) ||
	    (m_ptr->owner == p_ptr->id && m_ptr->pet) || //dont kill pets either, meanie!
	    //!owner_Ind || /* don't attack ownerless golems */
	    (owner_Ind && !check_hostile(Ind, owner_Ind))) /* only attack if owner is hostile */
#endif
	{
		int ox = m_ptr->fx, oy = m_ptr->fy, nx = p_ptr->px, ny = p_ptr->py;

		msg_format(Ind, "You swap positions with %s.", m_name);

		/* Update the new location */
		zcave[ny][nx].m_idx = c_ptr->m_idx;
		/* Update the old location */
		zcave[oy][ox].m_idx = -Ind;

		/* Move the monster */
		m_ptr->fy = ny;
		m_ptr->fx = nx;
		store_exit(Ind);
		p_ptr->py = oy;
		p_ptr->px = ox;

		cave_midx_debug(wpos, oy, ox, -Ind);

		/* Update the monster (new location) */
		update_mon(zcave[ny][nx].m_idx, TRUE);
		/* Redraw the old grid */
		everyone_lite_spot(wpos, oy, ox);
		/* Redraw the new grid */
		everyone_lite_spot(wpos, ny, nx);
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

		return;
	}

	/* Handle player fear */
	if (p_ptr->afraid) {
		msg_format(Ind, "\377yYou are too afraid to attack %s!", m_name);
		suppress_message = FALSE;
		/* Done */
		return;
	}

	/* Cannot 'bash' monsters without shield */
	if (p_ptr->inventory[INVEN_WIELD].tval != TV_SHIELD && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) {
		msg_format(Ind, "\377yYou need a shield to bash others effectively.");
		return;
	}

	/* check whether monster can be bashed */
	if (m_ptr->monfear) {
		msg_print(Ind, "\377yYou cannot bash monsters that are running away from you.");
		return;
	}

	/* cannot wield the shield? */
	if (p_ptr->heavy_shield) {
		msg_print(Ind, "\377oYour shield is too heavy for you to bash with it effectively.");
		return;
	}

	if (p_ptr->cst < 6) {
		msg_print(Ind, "\377oNot enough stamina!");
		return;
	}
#ifndef TEST_SERVER
	use_stamina(p_ptr, 6);
	p_ptr->redraw |= PR_STAMINA;
	redraw_stuff(Ind);
#endif

s_printf("TECHNIQUE_MELEE: %s - bash\n", p_ptr->name);

	/* cloaking mode stuff */
	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
	bandage_fails(Ind);

	/* Disturb the monster */
	if (m_ptr->csleep) {
		m_ptr->csleep = 0;
		if (m_ptr->custom_lua_awoke) exec_lua(0, format("custom_monster_awoke(%d,%d,%d)", Ind, c_ptr->m_idx, m_ptr->custom_lua_awoke));
	}

	/* Calculate damage from shield weight (50..120,160 for AA) and strength */
	k = 0;
	/* Edge case: use the nastier of our two shields - if allowed even */
	if (p_ptr->inventory[INVEN_WIELD].tval == TV_SHIELD) k = p_ptr->inventory[INVEN_WIELD].weight;
	if (p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD && p_ptr->inventory[INVEN_ARM].weight > k) k = p_ptr->inventory[INVEN_ARM].weight;
	/* Add our strength */
	k /= 10;
	k += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128); //-2..+15
	k += rand_int(3);
	/* Apply the player damage boni */
	//k += p_ptr->to_d + p_ptr->to_d_melee;

	/* Calculate stun effect */
	stun_effect = adj_str_td[p_ptr->stat_ind[A_STR]] - 128; //-2..+15
	stun_effect += get_skill_scale(p_ptr, SKILL_COMBAT, 10);
	stun_effect += 3 + rand_int(3); //ensure it's at least >= 1

	/* Calculate chance to hit (includes -here drastically reduced- enemy AC to determine 'miss') */
	chance = 60 + 2 * (7 + adj_dex_th[p_ptr->stat_ind[A_STR]] - 128); //-7..+20
	if (p_ptr->blind) chance >>= 1;

	/* Plan ahead if a missed attack would be a blocked or parried one or just an
	   [hitchance-vs-AC-induced] miss. 'Piercing' requires this to be calculated ahead now. */
	block = 0;
	if (strchr("hHJkpPtyn", r_ptr->d_char) && /* leaving out Yeeks (else Serpent Man 'J') */
	    !(r_ptr->flags3 & RF3_ANIMAL) &&
	    !(r_ptr->flags8 & RF8_NO_BLOCK)) {
#ifdef USE_BLOCKING
		block = 10;
#endif
	}
	/* Evaluate: 0 = no, other values = yes */
	if (helpless || !magik(block)) block = 0;

	p_ptr->test_attacks++;
	/* Test for hit */
	if (p_ptr->instakills || !block ||
	    test_hit_melee(chance, m_ptr->ac / 3, p_ptr->mon_vis[c_ptr->m_idx])) {
#ifdef USE_SOUND_2010
		if (p_ptr->sfx_combat) sound(Ind, "bash", "hit_blunt", SFX_TYPE_ATTACK, FALSE);
#else
		//sound(Ind, SOUND_HIT);
#endif
		sprintf(hit_desc, "You bash %s", m_name);

		i = k; /* Nazgul won't reduce our damage */
		if (!p_ptr->instakills) do_nazgul(Ind, &i, r_ptr, -1);

		if (stun_effect && (k < m_ptr->hp)) {
			/* Stun the monster */
			if (r_ptr->flags3 & RF3_NO_STUN) {
				msg_format(Ind, "%^s is unaffected.", m_name);
			} else {
				if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags3 & RF3_UNDEAD)) resist_stun += 500;
#if 0 /* normal roll for stun chance */
				if (rand_int(1000) > (50 + r_ptr->level + resist_stun)) {
#else /* more guaranteed stun, in the light of already having had to roll for test_hit_melee() to get here even */
				if (rand_int(5000) > (50 + r_ptr->level + resist_stun * 5)) {
#endif
					m_ptr->stunned += stun_effect;
					did_stun = TRUE;
				} else msg_format(Ind, "%^s resists the effect!", m_name);
			}
		}

#ifndef NEW_SHIELDS_NO_AC
		/* Does the weapon take damage from hitting acidic/fiery/aquatic monsters? */
		for (i = 0; i < 4; i++) {
			if (r_ptr->blow[i].effect == RBE_ACID) mon_acid++;
			if (r_ptr->blow[i].effect == RBE_FIRE) mon_fire++;
		}
		if (mon_acid + mon_fire) {
			if (randint(mon_acid + mon_fire) > mon_acid) {
				if (magik(5)) shield_takes_damage(Ind, GF_FIRE);
			} else {
				if (magik(10)) shield_takes_damage(Ind, GF_ACID);
			}
		}
#endif

		/* No negative damage */
		if (k < 0) k = 0;

		if (m_ptr->r_idx == RI_MIRROR) k = (k * MIRROR_REDUCE_DAM_TAKEN_MELEE + 99) / 100;

		/* for admins: kill a target in one hit */
		if (p_ptr->instakills) k = m_ptr->hp + 1;
		else if (p_ptr->admin_godly_strike) {
			p_ptr->admin_godly_strike--;
			if (!(r_ptr->flags1 & RF1_UNIQUE)) k = m_ptr->hp + 1;
		}

		/* Don't apply effects if monster just dies from this attack anyway */
		if (k > m_ptr->hp) did_stun = FALSE;

		if (r_ptr->flags1 & RF1_UNIQUE) {
			msg_format(Ind, "\377%c%s for \377e%d \377%cdamage.", uniq, hit_desc, k, uniq);
			if (uniq_bell) Send_beep(Ind);
		}
		else msg_format(Ind, "%s for \377g%d \377wdamage.", hit_desc, k);

		if (did_stun) {
			if (m_ptr->stunned > 100)
				msg_format(Ind, "\377y%^s is knocked out.", m_name);
			else if (m_ptr->stunned > 50)
				msg_format(Ind, "\377y%^s is heavily stunned.", m_name);
			else
				msg_format(Ind, "\377y%^s is stunned.", m_name);
		}

		/* target dummy */
		if (m_ptr->r_idx == RI_TARGET_DUMMY1 || m_ptr->r_idx == RI_TARGET_DUMMY2) {
			/* Hack: Reduce snow on it during winter season :) */
			m_ptr->extra -= 30;
			if (m_ptr->extra < 0) m_ptr->extra = 0;
				if ((m_ptr->r_idx == RI_TARGET_DUMMY2) && (m_ptr->extra < 30)) {
				m_ptr->r_idx = RI_TARGET_DUMMY1;
				everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
			}
		}
		if (m_ptr->r_idx == RI_TARGET_DUMMYA1 || m_ptr->r_idx == RI_TARGET_DUMMYA2) {
			/* Hack: Reduce snow on it during winter season :) */
			m_ptr->extra -= 30;
			if (m_ptr->extra < 0) m_ptr->extra = 0;
				if ((m_ptr->r_idx == RI_TARGET_DUMMYA2) && (m_ptr->extra < 30)) {
				m_ptr->r_idx = RI_TARGET_DUMMYA1;
				everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
			}
		}

		/* Damage, check for fear and death */
		if (mon_take_hit(Ind, c_ptr->m_idx, k, &fear, NULL)) {
			/* Exploding Attack - Kurzel */
			//if (p_ptr->nimbus) do_nimbus(Ind, y, x);  -- unsure

			fear = FALSE; /* paranoia */
			/* monster is dead */
		}

		/* Monster is still alive */
		else {
#ifdef ENABLE_OHERETICISM
			/* Extend ongoing 'Boundless Hate' spell thanks to Traumaturgy feedback? */
			if (p_ptr->hate_prolong == 1) p_ptr->hate_prolong = 2;
#endif

			touch_zap_player(Ind, c_ptr->m_idx);
			/* todo maybe: handle player death from getting zapped? */

			/* Exploding Attack - Kurzel */
			//if (p_ptr->nimbus) do_nimbus(Ind, y, x); -- unsure

		}
	}

	/* Player misses */
	else {
		/* Message */
		if (block) {
			sprintf(hit_desc, "\377%c%s blocks.", COLOUR_BLOCK_MON, m_name);
			hit_desc[2] = toupper(hit_desc[2]);
			msg_print(Ind, hit_desc);
#ifdef USE_SOUND_2010
			if (p_ptr->sfx_defense) sound(Ind, "block_shield", NULL, SFX_TYPE_ATTACK, FALSE);
#endif
		} else {
			msg_format(Ind, "You miss %s.", m_name);
#ifdef USE_SOUND_2010
			if (p_ptr->sfx_combat) sound(Ind, "miss", NULL, SFX_TYPE_ATTACK, FALSE);
#else
			sound(Ind, SOUND_MISS);
#endif
		}
	}

	/* Hack -- delay fear messages */
	if (fear && p_ptr->mon_vis[c_ptr->m_idx] && !(m_ptr->csleep || m_ptr->stunned > 100)) {
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

	suppress_message = FALSE;
}

/* Bash a player for stun effect and a little damage, costs stamina to avoid stun-spam into k.o.. - C. Blue
   Simple handling: No brands, cannot crit, no vampirism, no quake, just STR, shield weight, combat skill.
   Also, independant of combat stance actually - we perceive the shield bash more as a fighting technique than an attack. */
void py_bash_py(int Ind, int y, int x) {
	int		Ind2;
	player_type	*p_ptr = Players[Ind], *q_ptr;
	int		stun_effect, chance;
	int		k;

	char		q_name[NAME_LEN], hit_desc[MAX_CHARS_WIDE];

	bool		did_stun = FALSE;
	bool		no_pk;

	struct worldpos	*wpos = &p_ptr->wpos;
	cave_type	**zcave;
	cave_type	*c_ptr;

#ifndef NEW_SHIELDS_NO_AC
	int py_acid = 0, py_fire = 0;
#endif


	/* Skip bashing our own grid */
	if (p_ptr->px == x && p_ptr->py == y) return;

	/* Hack -- suppress messages */
	if (p_ptr->taciturn_messages) suppress_message = TRUE;

	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

	Ind2 = 0 - c_ptr->m_idx;
	q_ptr = Players[Ind2];

	no_pk = ((zcave[p_ptr->py][p_ptr->px].info & CAVE_NOPK) ||
	    (zcave[q_ptr->py][q_ptr->px].info & CAVE_NOPK));

	disturb(Ind, 0, 0);

	/* Extract name */
	player_desc(Ind, q_name, Ind2, 0);

	/* Track player health */
	if (p_ptr->play_vis[Ind2]) health_track(Ind, c_ptr->m_idx);

	/* can't attack while in WRAITHFORM */
	/* wraithed players can attack wraithed monsters - mikaelh */
	if (p_ptr->tim_wraith && !is_admin(p_ptr) && !q_ptr->tim_wraith) return;

	/* Don't allow moving AFK players around, unless they block stairs, gates, etc.
	   (Note: This might also include not so important feats, but the PERMA+REMEMBER heuristic is quite good in general.) */
	if (q_ptr->afk && (f_info[zcave[q_ptr->py][q_ptr->px].feat].flags1 & (FF1_PERMANENT | FF1_REMEMBER)) != (FF1_PERMANENT | FF1_REMEMBER)) {
		msg_print(Ind, "You cannot push past AFK players who don't stand on special features.");
		return;
	}
	if (q_ptr->store_num != -1) {
		msg_print(Ind, "You cannot push past players who are currently inside a shop.");
		return;
	}

	/* Only attack if player is hostile. Otherwise we just do a friendly place-switching here: */
	if (cfg.use_pk_rules == PK_RULES_NEVER || !check_hostile(Ind, Ind2)) {
		int ox = q_ptr->px, oy = q_ptr->py, nx = p_ptr->px, ny = p_ptr->py;

		p_ptr->energy -= level_speed(&p_ptr->wpos);
		disturb(Ind2, 0, 0);

		/* This intended act of pushing past isn't 'hostile' but also not 'nice' as you try to bash someone out of the way sort of,
		   displacing him in the process which could even be dangerous (same is possible by just bumping into him in the opposite of
		   his last executed movement direction though, so this is not always the only way to move someone else who might be afk),
		   so we still do some weight check for success. - C. Blue */
		if (adj_str_wgt[p_ptr->stat_ind[A_STR]] * 25 < q_ptr->wt + p_ptr->total_weight / 10) {
			/* Note about wt and total_weight: Body weight (wt) is measured in lb while total item weight (inven+equip) is measured in lb * 10, as used in k_info.txt */
			msg_format(Ind, "You are not strong enough to push past %s.", q_name);
			black_breath_infection(Ind, Ind2);
			return;
		}

		msg_format(Ind, "You push past %s.", q_name);
		msg_format(Ind2, "%^s pushes past you.", p_ptr->name);

		/* Update the new location */
		zcave[ny][nx].m_idx = -Ind2;
		/* Update the old location */
		zcave[oy][ox].m_idx = -Ind;

		/* Move the playerr */
		q_ptr->py = ny;
		q_ptr->px = nx;
		store_exit(Ind);
		store_exit(Ind2);
		p_ptr->py = oy;
		p_ptr->px = ox;

		cave_midx_debug(wpos, oy, ox, -Ind);

		/* Disturbances */
		black_breath_infection(Ind, Ind2);
		stop_precision(Ind2);
		stop_shooting_till_kill(Ind2);
		disturb(Ind, 1, 0);
		disturb(Ind2, 1, 0);
		/* Grid effects/tips */
		moved_player(Ind2, q_ptr, zcave, p_ptr->px, p_ptr->py);
		moved_player(Ind, p_ptr, zcave, q_ptr->px, q_ptr->py);

		/* Redraw the old grid */
		everyone_lite_spot(wpos, oy, ox);
		/* Redraw the new grid */
		everyone_lite_spot(wpos, ny, nx);
		/* Check for new panel (redraw map) */
		verify_panel(Ind);
		verify_panel(Ind2);
		/* Update stuff */
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
		q_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
		/* Update the monsters */
		p_ptr->update |= (PU_DISTANCE);
		q_ptr->update |= (PU_DISTANCE);
		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD);
		q_ptr->window |= (PW_OVERHEAD);
		/* Handle stuff XXX XXX XXX */
		handle_stuff(Ind);
		handle_stuff(Ind2);
		return;
	}

	/* --- Hostile shield-bash fighting technique --- */

	if (!(p_ptr->melee_techniques & MT_BASH)) {
		msg_print(Ind, "You are not proficient in shield-bashing opponents.");
		return;
	}

#if 0 /* Bashing is not a weapon-based attack! */
	if (!p_ptr->num_blow) {
		msg_print(Ind, "\377yYou cannot attack.");
		return;
	}
#endif

	/* Handle player fear */
	if (p_ptr->afraid) {
		msg_format(Ind, "\377yYou are too afraid to attack %s!", q_name);
		suppress_message = FALSE;
		/* Done */
		return;
	}

	/* Cannot 'bash' monsters without shield */
	if (p_ptr->inventory[INVEN_WIELD].tval != TV_SHIELD && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) {
		msg_format(Ind, "\377yYou need a shield to bash others effectively.");
		return;
	}

#if 0
	/* check whether player can be bashed */
	if (q_ptr->afraid) {
		msg_print(Ind, "\377yYou cannot bash someone who is running away from you.");
		return;
	}
#endif

	/* cannot wield the shield? */
	if (p_ptr->heavy_shield) {
		msg_print(Ind, "\377oYour shield is too heavy for you to bash with it effectively.");
		return;
	}

	if (p_ptr->cst < 6) {
		msg_print(Ind, "\377oNot enough stamina!");
		return;
	}
	use_stamina(p_ptr, 6);
	p_ptr->redraw |= PR_STAMINA;
	redraw_stuff(Ind);

	p_ptr->energy -= level_speed(&p_ptr->wpos);
	disturb(Ind2, 0, 0);

s_printf("TECHNIQUE_MELEE: %s - bash\n", p_ptr->name);

	/* cloaking mode stuff */
	break_cloaking(Ind, 0);
	break_cloaking(Ind2, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_precision(Ind2);
	stop_shooting_till_kill(Ind);
	stop_shooting_till_kill(Ind2);
	bandage_fails(Ind);
	bandage_fails(Ind2);

	/* Calculate damage from shield weight (50..120,160 for AA) and strength */
	k = 0;
	/* Edge case: use the nastier of our two shields - if allowed even */
	if (p_ptr->inventory[INVEN_WIELD].tval == TV_SHIELD) k = p_ptr->inventory[INVEN_WIELD].weight;
	if (p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD && p_ptr->inventory[INVEN_ARM].weight > k) k = p_ptr->inventory[INVEN_ARM].weight;
	/* Add our strength */
	k /= 10;
	k += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128); //-2..+15
	k += rand_int(3);
	/* Apply the player damage boni */
	//k += p_ptr->to_d + p_ptr->to_d_melee;

	k /= PVP_MELEE_DAM_REDUCTION;
	if (!k) k = 1;

	/* Calculate stun effect */
	stun_effect = adj_str_td[p_ptr->stat_ind[A_STR]] - 128; //-2..+15
	stun_effect += get_skill_scale(p_ptr, SKILL_COMBAT, 10);
	stun_effect += 3 + rand_int(3); //ensure it's at least >= 1
	/* Apply enemy combat experience */
	stun_effect -= get_skill_scale(q_ptr, SKILL_COMBAT, 10);
	if (stun_effect < 1) stun_effect = 1;

	/* Calculate chance to hit (includes -here drastically reduced- enemy AC to determine 'miss') */
	chance = 60 + 2 * (7 + adj_dex_th[p_ptr->stat_ind[A_STR]] - 128); //-7..+20
	if (p_ptr->blind) chance >>= 1;

#ifdef USE_BLOCKING
	if (q_ptr->shield_deflect && (!q_ptr->weapon_parry || magik(q_ptr->combat_stance == 1 ? 75 : 50))) {
		if (magik(apply_block_chance(q_ptr, q_ptr->shield_deflect + 0))) {
			msg_format(Ind, "\377%c%s blocks your attack!", COLOUR_BLOCK_PLY, q_name);
			switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
			case 's': case 'x': case 'z':
				msg_format(0 - c_ptr->m_idx, "\377%cYou block %s' attack!", COLOUR_BLOCK_GOOD, p_ptr->name);
				break;
			default:
				msg_format(0 - c_ptr->m_idx, "\377%cYou block %s's attack!", COLOUR_BLOCK_GOOD, p_ptr->name);
			}
 #ifdef USE_SOUND_2010
			if (p_ptr->sfx_defense) sound(Ind, "block_shield", NULL, SFX_TYPE_ATTACK, FALSE);
 #endif

		suppress_message = FALSE;
		return;
		}
	}
#endif

	p_ptr->test_attacks++;
	/* Test for hit */
	if (p_ptr->instakills || test_hit_melee(chance, (q_ptr->ac + q_ptr->to_a) > AC_CAP ? AC_CAP : q_ptr->ac + q_ptr->to_a, 1)) {
#ifdef USE_SOUND_2010
		if (p_ptr->sfx_combat) sound(Ind, "bash", "hit_blunt", SFX_TYPE_ATTACK, FALSE);
#else
		//sound(Ind, SOUND_HIT);
#endif
		sprintf(hit_desc, "You bash %s", q_name);

		if (stun_effect && (k < q_ptr->chp)) {
			/* Stun the player */
#if 0 /* normal roll for stun chance */
			if (rand_int(1000) > (50 + q_ptr->lev)) {
#else /* more guaranteed stun, in the light of already having had to roll for test_hit_melee() to get here even */
			if (rand_int(5000) > (50 + q_ptr->lev)) {
#endif
				msg_format(Ind, "\377y%^s is stunned.", q_name);

				set_stun(Ind2, q_ptr->stun + stun_effect);
				did_stun = TRUE;
			} else msg_format(Ind, "%^s resists the effect!", q_name);
		}

#ifndef NEW_SHIELDS_NO_AC
		/* Does the weapon take damage from hitting acidic/fiery/aquatic monsters? */
		if (q_ptr->brand_acid) py_acid++;
		if (q_ptr->brand_fire) py_fire++;
		if (py_acid + py_fire) {
			if (randint(py_acid + py_fire) > py_acid) {
				if (magik(5)) shield_takes_damage(Ind, GF_FIRE);
			} else {
				if (magik(10)) shield_takes_damage(Ind, GF_ACID);
			}
		}
#endif

		/* No negative damage */
		if (k < 0) k = 0;

		/* Don't apply effects if monster just dies from this attack anyway */
		if (k > q_ptr->chp) did_stun = FALSE;

		if (did_stun) {
			if (q_ptr->stun > 100)
				msg_format(Ind, "\377y%^s is knocked out.", q_name);
			else if (q_ptr->stun > 50)
				msg_format(Ind, "\377y%^s is heavily stunned.", q_name);
			else
				msg_format(Ind, "\377y%^s is stunned.", q_name);
		}

		/* Cannot kill on this grid? */
		if (no_pk) {
			if (k > q_ptr->chp) k = q_ptr->chp;
		}

		/* no_pk handled */
		if (cfg.use_pk_rules == PK_RULES_NEVER && q_ptr->chp - k <= 0) {
			msg_format(Ind, "\374You have beaten %s", q_ptr->name);
			msg_format(0 - c_ptr->m_idx, "\374%s has beaten you up!", p_ptr->name);
			teleport_player(0 - c_ptr->m_idx, 400, TRUE);

			/* End of the fight */
			return;
		}

		/* Damage, check for fear and death */
		take_hit(0 - c_ptr->m_idx, k, p_ptr->name, Ind);
		if (q_ptr->death) return;

		/* Player is still alive */
#ifdef ENABLE_OHERETICISM
		/* Extend ongoing 'Boundless Hate' spell thanks to Traumaturgy feedback? */
		if (p_ptr->hate_prolong == 1) p_ptr->hate_prolong = 2;
#endif

		py_touch_zap_player(Ind, Ind2);
		/* todo maybe: handle player death from getting zapped? */

		/* Exploding Attack - Kurzel */
		//if (p_ptr->nimbus) do_nimbus(Ind, y, x); -- unsure
	}

	/* Player misses */
	else {
		msg_format(Ind, "You miss %s.", q_name);
#ifdef USE_SOUND_2010
		if (p_ptr->sfx_combat) sound(Ind, "miss", NULL, SFX_TYPE_ATTACK, FALSE);
#else
		sound(Ind, SOUND_MISS);
#endif
	}

	suppress_message = FALSE;
}


/*
 * Attacking something, figure out what and spawn appropriately.
 *
 * If 'old' is TRUE, it's just same as ever; player will attack
 * (num_blow) times, and energy consumption is not calculated here.
 * If FALSE, player attacks only once, no matter what num_blow is,
 * and (1/num_blow) turn is consumed.   - Jir -
 */
void py_attack(int Ind, int y, int x, byte old) {
	player_type *p_ptr = Players[Ind];
	cave_type **zcave;
	cave_type *c_ptr;
	struct worldpos *wpos = &p_ptr->wpos;

	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST) return;
	if (p_ptr->body_monster && (r_info[p_ptr->body_monster].flags2 & RF2_NEVER_BLOW)) return;
	if (!p_ptr->num_blow) return; //prevent div/0 in py_attack_..() routines

	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];


	/* Break goi/manashield */
#if 0
	if (p_ptr->invuln) set_invuln(Ind, 0);
	if (p_ptr->tim_manashield) set_tim_manashield(Ind, 0);
	if (p_ptr->tim_wraith) set_tim_wraith(Ind, 0);
#endif	// 0

#ifdef TARGET_SWITCHING_COST
	/* Time cost for automatically switching target during auto-retaliation.
	   Note: bumping into a target will consume a full turn of energy even if it dies before we hit it with our full number of bpr,
	         so we don't need to check in that case really and can just focus on auto-retaliation instead. */
	if (c_ptr->m_idx /* paranoia? */
	    /* we did attack something right now without any pause afterwards,
	    and it was something different than our current target? */
	    && p_ptr->tsc_lasttarget != c_ptr->m_idx
	    /* leeway - don't apply it to already pretty slow attackers */
	    && p_ptr->num_blow > 2) {
		/* we switched to a new target? */
		if (p_ptr->tsc_lasttarget) {
			p_ptr->tsc_lasttarget = c_ptr->m_idx;
			/* skip a blow, for turning towards our new target */
			if (!old) {
				p_ptr->energy -= level_speed(&p_ptr->wpos) / p_ptr->num_blow;
				return;
			} else old = 2; //marker for skipping a blow
		}
		/* we actually just began melee combat, attacking our very first target - we're already prepared. */
		p_ptr->tsc_lasttarget = c_ptr->m_idx;
	}
#endif

	/* Check for monster */
	if (c_ptr->m_idx > 0)
		py_attack_mon(Ind, y, x, old);
	/* Check for player */
	else if (c_ptr->m_idx < 0 && cfg.use_pk_rules != PK_RULES_NEVER)
		py_attack_player(Ind, y, x, old);

	if (p_ptr->warning_ranged_autoret == 0) {
		p_ptr->warning_ranged_autoret = 1;
		msg_print(Ind, "\374\377yHINT: Inscribe your ranged weapon '\377R@O\377y' to use it for auto-retaliation!");
		s_printf("warning_ranged_autoret: %s\n", p_ptr->name);
	}
}

/* Hit monsters around us 'num_blow + 8' times in total, adding a stun effect to each blow. */
void spin_attack(int Ind) {
	player_type *p_ptr = Players[Ind];
	cave_type **zcave;
	cave_type *c_ptr;
	int i, j, x, y;
	struct worldpos *wpos = &p_ptr->wpos;

	if (!(zcave = getcave(wpos))) return;
	if (p_ptr->body_monster && (r_info[p_ptr->body_monster].flags2 & RF2_NEVER_BLOW)) return;
	if (!p_ptr->num_blow) return; /* paranoia */

	/* In case we got here by weapon activation: */
	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST) {
		msg_print(Ind, "\377yYou cannot spin while in mist form.");
		return;
	}

	msg_print(Ind, "You spin around in a mighty sweep!");
	msg_format_near(Ind, "%s spins around in a mighty sweep!", p_ptr->name);

	/* Break goi/manashield */
#if 0
	if (p_ptr->invuln) set_invuln(Ind, 0);
	if (p_ptr->tim_manashield) set_tim_manashield(Ind, 0);
	if (p_ptr->tim_wraith) set_tim_wraith(Ind, 0);
#endif	// 0

	j = rand_int(8);
	for (i = 1; i <= p_ptr->num_blow + 8; i++) {
		if ((j + i) % 9 + 1 == 5) continue;
		x = p_ptr->px + ddx[(j + i) % 9 + 1];
		y = p_ptr->py + ddy[(j + i) % 9 + 1];
		if (!in_bounds(y, x)) continue;
		c_ptr = &zcave[y][x];

		/* Check for monster */
		if (c_ptr->m_idx > 0) {
			p_ptr->stunning = TRUE;
			p_ptr->energy += level_speed(&p_ptr->wpos) / p_ptr->num_blow;
			py_attack_mon(Ind, y, x, FALSE); /* only 1 attack */
		}
		/* Check for player */
//		else if (c_ptr->m_idx < 0 && cfg.use_pk_rules != PK_RULES_NEVER) py_attack_player(Ind, y, x, old);
	}
}

/* PernAngband addition */
void touch_zap_player(int Ind, int m_idx) {
	monster_type	*m_ptr = &m_list[m_idx];

	player_type *p_ptr = Players[Ind];
	int aura_damage = 0;
	monster_race *r_ptr = race_inf(m_ptr);

	if (r_ptr->flags2 & (RF2_AURA_FIRE)) {
		if (!(p_ptr->immune_fire)) {
			char aura_dam[80];

			aura_damage = damroll(1 + (m_ptr->level / 26), 1 + (m_ptr->level / 17));

			/* Hack -- Get the "died from" name */
			monster_desc(Ind, aura_dam, m_idx, 0x88);

			if (p_ptr->oppose_fire || (p_ptr->nimbus && p_ptr->nimbus_t == GF_FIRE))
				aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->resist_fire || (p_ptr->nimbus && p_ptr->nimbus_t == GF_FIRE))
				aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->suscep_fire && !(p_ptr->nimbus && p_ptr->nimbus_t == GF_FIRE))
				aura_damage = aura_damage * 2;

			if (m_ptr->r_idx == RI_MIRROR) aura_damage = (aura_damage * MIRROR_REDUCE_DAM_DEALT_AURA + 99) / 100;

			msg_format(Ind, "You are enveloped in flames for \377w%d\377w damage!", aura_damage);
			take_hit(Ind, aura_damage, aura_dam, -m_idx);
#ifdef OLD_MONSTER_LORE
			r_ptr->r_flags2 |= RF2_AURA_FIRE;
#endif
			handle_stuff(Ind);
		}
	}


	if (r_ptr->flags2 & (RF2_AURA_ELEC)) {
		if (!(p_ptr->immune_elec)) {
			char aura_dam[80];

			aura_damage = damroll(1 + (m_ptr->level / 26), 1 + (m_ptr->level / 17));

			/* Hack -- Get the "died from" name */
			monster_desc(Ind, aura_dam, m_idx, 0x88);

			if (p_ptr->oppose_elec || (p_ptr->nimbus && p_ptr->nimbus_t == GF_ELEC))
				aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->resist_elec || (p_ptr->nimbus && p_ptr->nimbus_t == GF_ELEC))
				aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->suscep_elec && !(p_ptr->nimbus && p_ptr->nimbus_t == GF_ELEC))
				aura_damage = aura_damage * 2;

			if (m_ptr->r_idx == RI_MIRROR) aura_damage = (aura_damage * MIRROR_REDUCE_DAM_DEALT_AURA + 99) / 100;

			msg_format(Ind, "You get zapped for \377w%d\377w damage!", aura_damage);
			take_hit(Ind, aura_damage, aura_dam, -m_idx);
#ifdef OLD_MONSTER_LORE
			r_ptr->r_flags2 |= RF2_AURA_ELEC;
#endif
			handle_stuff(Ind);
		}
	}


	if (r_ptr->flags3 & (RF3_AURA_COLD)) {
		if (!(p_ptr->immune_cold)) {
			char aura_dam[80];

			aura_damage = damroll(1 + (m_ptr->level / 26), 1 + (m_ptr->level / 17));

			/* Hack -- Get the "died from" name */
			monster_desc(Ind, aura_dam, m_idx, 0x88);

			if (p_ptr->oppose_cold || (p_ptr->nimbus && p_ptr->nimbus_t == GF_COLD))
				aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->resist_cold || (p_ptr->nimbus && p_ptr->nimbus_t == GF_COLD))
				aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->suscep_cold && !(p_ptr->nimbus && p_ptr->nimbus_t == GF_COLD))
				aura_damage = aura_damage * 2;

			if (m_ptr->r_idx == RI_MIRROR) aura_damage = (aura_damage * MIRROR_REDUCE_DAM_DEALT_AURA + 99) / 100;

			msg_format(Ind, "You are freezing for \377w%d\377w damage!", aura_damage);
			take_hit(Ind, aura_damage, aura_dam, -m_idx);
#ifdef OLD_MONSTER_LORE
			r_ptr->r_flags3 |= RF3_AURA_COLD;
#endif
			handle_stuff(Ind);
		}
	}
}
void py_touch_zap_player(int Ind, int Ind2) {
	player_type *q_ptr = Players[Ind2], *p_ptr = Players[Ind];
	int aura_damage = 0;
	/* Check if our 'intrinsic' (Blood Magic, not from item/external spell) auras were suppressed by our own antimagic field. */
	bool aura_ok = !magik((p_ptr->antimagic * 8) / 5);
	int auras_failed = 0;

	/* Alternate contradicting auras, same as with brands */
	if (!p_ptr->death && q_ptr->sh_fire && q_ptr->sh_cold) {
		if (rand_int(2)) {
			if (!(p_ptr->immune_fire)) {
				aura_damage = damroll(2, 6);
				if (p_ptr->oppose_fire || (p_ptr->nimbus && p_ptr->nimbus_t == GF_FIRE))
					aura_damage = (aura_damage + 2) / 3;
				if (p_ptr->resist_fire || (p_ptr->nimbus && p_ptr->nimbus_t == GF_FIRE))
					aura_damage = (aura_damage + 2) / 3;
				if (p_ptr->suscep_fire && !(p_ptr->nimbus && p_ptr->nimbus_t == GF_FIRE))
					aura_damage = aura_damage * 2;

				aura_damage = (aura_damage + PVP_AURA_DAM_REDUCTION - 1) / PVP_AURA_DAM_REDUCTION;

				msg_format(Ind, "You are enveloped in flames for \377w%d\377w damage!", aura_damage);
				msg_format(Ind2, "%s is enveloped in flames for \377w%d\377w damage!", p_ptr->name, aura_damage);
				take_hit(Ind, aura_damage, "a fire aura", Ind2);
				handle_stuff(Ind);
			}
		} else {
			if (!(p_ptr->immune_cold)) {
				aura_damage = damroll(2, 6);
				if (p_ptr->oppose_cold || (p_ptr->nimbus && p_ptr->nimbus_t == GF_COLD))
					aura_damage = (aura_damage + 2) / 3;
				if (p_ptr->resist_cold || (p_ptr->nimbus && p_ptr->nimbus_t == GF_COLD))
					aura_damage = (aura_damage + 2) / 3;
				if (p_ptr->suscep_cold && !(p_ptr->nimbus && p_ptr->nimbus_t == GF_COLD))
					aura_damage = aura_damage * 2;

				aura_damage = (aura_damage + PVP_AURA_DAM_REDUCTION - 1) / PVP_AURA_DAM_REDUCTION;

				msg_format(Ind, "You are freezing for \377w%d\377w damage!", aura_damage);
				msg_format(Ind2, "%s is freezing for \377w%d\377w damage!", p_ptr->name, aura_damage);
				take_hit(Ind, aura_damage, "a frost aura", Ind2);
				handle_stuff(Ind);
			}
		}
	} else {
		if (!p_ptr->death && q_ptr->sh_fire) {
			if (!(p_ptr->immune_fire)) {
				if (p_ptr->oppose_fire || (p_ptr->nimbus && p_ptr->nimbus_t == GF_FIRE))
					aura_damage = (aura_damage + 2) / 3;
				if (p_ptr->resist_fire || (p_ptr->nimbus && p_ptr->nimbus_t == GF_FIRE))
					aura_damage = (aura_damage + 2) / 3;
				if (p_ptr->suscep_fire && !(p_ptr->nimbus && p_ptr->nimbus_t == GF_FIRE))
					aura_damage = aura_damage * 2;

				aura_damage = (aura_damage + PVP_AURA_DAM_REDUCTION - 1) / PVP_AURA_DAM_REDUCTION;

				msg_format(Ind, "You are enveloped in flames for \377w%d\377w damage!", aura_damage);
				msg_format(Ind2, "%s is enveloped in flames for \377w%d\377w damage!", p_ptr->name, aura_damage);
				take_hit(Ind, aura_damage, "a fire aura", Ind2);
				handle_stuff(Ind);
			}
		}
		if (!p_ptr->death && q_ptr->sh_cold) {
			if (!(p_ptr->immune_cold)) {
				aura_damage = damroll(2, 6);
				if (p_ptr->oppose_cold || (p_ptr->nimbus && p_ptr->nimbus_t == GF_COLD))
					aura_damage = (aura_damage + 2) / 3;
				if (p_ptr->resist_cold || (p_ptr->nimbus && p_ptr->nimbus_t == GF_COLD))
					aura_damage = (aura_damage + 2) / 3;
				if (p_ptr->suscep_cold && !(p_ptr->nimbus && p_ptr->nimbus_t == GF_COLD))
					aura_damage = aura_damage * 2;

				aura_damage = (aura_damage + PVP_AURA_DAM_REDUCTION - 1) / PVP_AURA_DAM_REDUCTION;

				msg_format(Ind, "You are freezing for \377w%d\377w damage!", aura_damage);
				msg_format(Ind2, "%s is freezing for \377w%d\377w damage!", p_ptr->name, aura_damage);
				take_hit(Ind, aura_damage, "a frost aura", Ind2);
				handle_stuff(Ind);
			}
		}
	}
	if (!p_ptr->death && q_ptr->sh_elec) {
		if (!(p_ptr->immune_elec)) {
			aura_damage = damroll(2, 6);
			if (p_ptr->oppose_elec || (p_ptr->nimbus && p_ptr->nimbus_t == GF_ELEC))
				aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->resist_elec || (p_ptr->nimbus && p_ptr->nimbus_t == GF_ELEC))
				aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->suscep_elec && !(p_ptr->nimbus && p_ptr->nimbus_t == GF_ELEC))
				aura_damage = aura_damage * 2;

			aura_damage = (aura_damage + PVP_AURA_DAM_REDUCTION - 1) / PVP_AURA_DAM_REDUCTION;

			msg_format(Ind, "You get zapped for \377w%d\377w damage!", aura_damage);
			msg_format(Ind2, "%s gets zapped for \377w%d\377w damage!", p_ptr->name, aura_damage);
			take_hit(Ind, aura_damage, "a lightning aura", Ind2);
			handle_stuff(Ind);
		}
	}

	/* Nimbus - Kurzel */
	if (q_ptr->nimbus && !p_ptr->death) {
		msg_format(Ind, "You breach an aura of power!");
		msg_format(Ind2, "%^s breaches your aura of power!", p_ptr->name);
		do_nimbus(Ind2, p_ptr->py, p_ptr->px);
	}

	/*
	 *Apply the 'shield auras'
	 */
	if (q_ptr->shield) {
		/* force shield */
		if (!p_ptr->death && (q_ptr->shield_opt & SHIELD_COUNTER)) {
			aura_damage = damroll(q_ptr->shield_power_opt, q_ptr->shield_power_opt2);
			msg_format(Ind, "You get bashed by a mystic shield for \377w%d\377w!", aura_damage);
			aura_damage = (aura_damage + PVP_AURA_DAM_REDUCTION - 1) / PVP_AURA_DAM_REDUCTION;
			take_hit(Ind, aura_damage, "a mystic shield", Ind2);
			handle_stuff(Ind);
		}
		/* fire shield */
		if (!p_ptr->death && (q_ptr->shield_opt & SHIELD_FIRE)) {
			if (!p_ptr->immune_fire) {
				aura_damage = damroll(q_ptr->shield_power_opt, q_ptr->shield_power_opt2);
				if (p_ptr->oppose_fire || (p_ptr->nimbus && p_ptr->nimbus_t == GF_FIRE))
					aura_damage = (aura_damage + 2) / 3;
				if (p_ptr->resist_fire || (p_ptr->nimbus && p_ptr->nimbus_t == GF_FIRE))
					aura_damage = (aura_damage + 2) / 3;
				if (p_ptr->suscep_fire && !(p_ptr->nimbus && p_ptr->nimbus_t == GF_FIRE))
					aura_damage = aura_damage * 2;

				aura_damage = (aura_damage + PVP_AURA_DAM_REDUCTION - 1) / PVP_AURA_DAM_REDUCTION;

				msg_format(Ind, "You are enveloped in flames for \377w%d\377w damage!", aura_damage);
				msg_format(Ind2, "%s is enveloped in flames for \377w%d\377w damage!", p_ptr->name, aura_damage);
				take_hit(Ind, aura_damage, "a fire aura", Ind2);
				handle_stuff(Ind);
			}
		}
		/* ice shield, functionally similar to aura of death - Kurzel */
		if (!p_ptr->death && (q_ptr->shield_opt & SHIELD_ICE)) {
			if (magik(25)) {
				sprintf(p_ptr->attacker, " is enveloped in ice for");
				fire_ball(Ind2, GF_ICE, 0, damroll(p_ptr->shield_power_opt, p_ptr->shield_power_opt2), 1, p_ptr->attacker);
			}
		}
		/* plasma shield, functionally similar to aura of death - Kurzel */
		if (!p_ptr->death && (q_ptr->shield_opt & SHIELD_PLASMA)) {
			if (magik(25)) {
				sprintf(p_ptr->attacker, " is enveloped in plasma for");
				fire_ball(Ind2, GF_PLASMA, 0, damroll(p_ptr->shield_power_opt, p_ptr->shield_power_opt2), 1, p_ptr->attacker);
			}
		}
	}

	/*
	 * Apply the blood magic auras
	 */
	/* Aura of fear is now affected by the monster level too */
	if (!p_ptr->death && get_skill(q_ptr, SKILL_AURA_FEAR) && q_ptr->aura[AURA_FEAR]
	    && !p_ptr->resist_fear) {
		if (magik(get_skill_scale(q_ptr, SKILL_AURA_FEAR, 30) + 5) &&
		    p_ptr->lev < get_skill_scale(q_ptr, SKILL_AURA_FEAR, 100)) {
			if (aura_ok) {
				msg_format(Ind2, "%^s breaches your aura of fear.", p_ptr->name);
				(void)set_afraid(Ind, p_ptr->afraid + 2 + get_skill_scale(p_ptr, SKILL_AURA_FEAR, 2));
			}
			else auras_failed++;
		}
	}
	/* Shivering Aura is affected by the target level */
	if (!p_ptr->death &&
	    ((get_skill(q_ptr, SKILL_AURA_SHIVER) && q_ptr->aura[AURA_SHIVER]) ||
	    (q_ptr->prace == RACE_VAMPIRE && q_ptr->body_monster == RI_VAMPIRIC_MIST))
	    && !(p_ptr->resist_sound
	    || (p_ptr->nimbus && (p_ptr->nimbus_t == GF_SOUND || p_ptr->nimbus_t == GF_FORCE)))
	    && !p_ptr->immune_cold) {
		int chance_trigger = get_skill_scale(q_ptr, SKILL_AURA_SHIVER, 25);
		int threshold_effect = get_skill_scale(q_ptr, SKILL_AURA_SHIVER, 100);

		if (q_ptr->prace == RACE_VAMPIRE && q_ptr->body_monster == RI_VAMPIRIC_MIST) {
			chance_trigger = 25; //max
			threshold_effect = (q_ptr->lev < 50) ? q_ptr->lev * 2 : 100; //80..100 (max)
		}

		chance_trigger += 25; //generic boost

		if (magik(chance_trigger) && (p_ptr->lev < threshold_effect)) {
			if (aura_ok) {
				msg_format(Ind2, "%^s breaches your shivering aura.", p_ptr->name);
				(void)set_stun_raw(Ind, p_ptr->stun + 5);
			}
			else auras_failed++;
		}
	}
	/* Aura of death is NOT affected by target level*/
	if (!p_ptr->death && get_skill(q_ptr, SKILL_AURA_DEATH) && q_ptr->aura[AURA_DEATH]) {
		int chance = get_skill_scale(q_ptr, SKILL_AURA_DEATH, 50);

		if (magik(chance)) {
			if (aura_ok) {
				int dam = 5 + chance * 3;

				if (magik(50)) {
					msg_format(Ind2, "%s breaches your death aura, causing a plasma outburst!", p_ptr->name);
					sprintf(q_ptr->attacker, " eradiates a wave of plasma for");
					fire_ball(Ind2, GF_PLASMA, 0, dam, 1, q_ptr->attacker);
				} else {
					msg_format(Ind2, "%s breaches your death aura, causing an ice explosion!", p_ptr->name);
					sprintf(q_ptr->attacker, " eradiates a wave of ice for");
					fire_ball(Ind2, GF_ICE, 0, dam, 1, q_ptr->attacker);
				}
			} else auras_failed++;
		}
	}
	/* Notify if our 'intrinsic' (Blood Magic, not from item/external spell) auras failed.. */
	if (auras_failed) {
#ifdef USE_SOUND_2010
		sound(Ind, "am_field", NULL, SFX_TYPE_MISC, FALSE);
#endif
		msg_format(Ind, "\377%cYour anti-magic field disrupts your aura%s.", COLOUR_AM_OWN, auras_failed == 1 ? "" : "s");
	}
}


/* Hiho! Finally *Nazguls* had come!		- Jir -
 *
 * However, following features are not implemented yet:
 * - multi-weapons attack
 * - attack interrupting
 *
 */

/* Apply nazgul effects */
/* Mega Hack -- Hitting Nazgul is REALY dangerous
 * (ideas from Akhronath) */
void do_nazgul(int Ind, int *k, monster_race *r_ptr, int slot) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr = (slot == -1 ? NULL : &p_ptr->inventory[slot]);
	char o_name[ONAME_LEN];
	u32b f1, f2, f3, f4, f5, f6, esp;

	if (!(r_ptr->flags7 & RF7_NAZGUL)) return;

	if (o_ptr) object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp); /* Weapons do not gain slay mods from other items! Only from skills. */
	else f1 = p_ptr->slay_equip; /* Bare-handed combat does gain slay mods from other items! */
	f1 |= p_ptr->slay | p_ptr->slay_melee; /* Apply all skill/trait slays. */

	if (!o_ptr) {
		/* Martial arts or bare-handed attacks */
		if (!(f1 & (TR1_SLAY_EVIL | TR1_SLAY_UNDEAD | TR1_KILL_UNDEAD))) {
			msg_print(Ind, "The Ringwraith is IMPERVIOUS to your mundane strikes.");
			*k = 0;
		}
	} else if ((!o_ptr->name2) && (!artifact_p(o_ptr))) {
		if (!(f1 & (TR1_SLAY_EVIL | TR1_SLAY_UNDEAD | TR1_KILL_UNDEAD))) {
			msg_print(Ind, "The Ringwraith is IMPERVIOUS to the mundane weapon.");
			*k = 0;
		}

		/* Dark Swords resist somewhat */
		if ((o_ptr->tval == TV_SWORD && o_ptr->sval == SV_DARK_SWORD) ? magik(15) : magik(100)) {
			object_desc(0, o_name, o_ptr, TRUE, 3);
			s_printf("NAZGUL_DISI_NORM: %s : %s.\n", p_ptr->name, o_name);

#ifdef USE_SOUND_2010
			sound_item(Ind, o_ptr->tval, o_ptr->sval, "kill_");
#endif
			msg_print(Ind, "\376\377rYour weapon *DISINTEGRATES*!");
			//inven_item_increase(Ind, INVEN_WIELD + weap, -1);
			//inven_item_optimize(Ind, INVEN_WIELD + weap);
			inven_item_increase(Ind, slot, -1); /* this way using slot we can handle dual-wield */
			inven_item_describe_important(Ind, slot);
			inven_item_optimize(Ind, slot);
		}
	} else if (like_artifact_p(o_ptr)) {
		if (!(f1 & (TR1_SLAY_EVIL | TR1_SLAY_UNDEAD | TR1_KILL_UNDEAD))
		    && !(f4 & TR4_BLACK_BREATH)) { /* there are no (pseudo)artifact Morgul weapons atm, but anyway.. */
			msg_print(Ind, "The Ringwraith is IMPERVIOUS to the mundane weapon.");
			*k = 0;
		}

		//apply_disenchant(Ind, INVEN_WIELD + weap);
		//apply_disenchant(Ind, weap);

		/* 1/1000 chance of getting destroyed.
		   Exploit-fix here for permacursed items. (Grond only) */
		if (!rand_int(1000) && !(f3 & TR3_PERMA_CURSE) && !indestructible_artifact_p(o_ptr)) {
			object_desc(0, o_name, o_ptr, TRUE, 3);
			s_printf("NAZGUL_DISI_ARTLIKE: %s : %s.\n", p_ptr->name, o_name);

#ifdef USE_SOUND_2010
			sound_item(Ind, o_ptr->tval, o_ptr->sval, "kill_");
#endif

			if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1);
			msg_print(Ind, "\376\377rYour weapon is destroyed!");
			//inven_item_increase(Ind, INVEN_WIELD + weap, -1);
			//inven_item_optimize(Ind, INVEN_WIELD + weap);
			inven_item_increase(Ind, slot, -1);
			inven_item_describe_important(Ind, slot);
			inven_item_optimize(Ind, slot);
		}
	} else if (o_ptr->name2) {
		if (!(f1 & (TR1_SLAY_EVIL | TR1_SLAY_UNDEAD | TR1_KILL_UNDEAD))
		    && !(f4 & TR4_BLACK_BREATH)) { /* :-O for Morgul weapons */
			msg_print(Ind, "The Ringwraith is IMPERVIOUS to the mundane weapon.");
			*k = 0;
		}

		/* Dark Swords and *Slay Undead* weapons resist the Nazgul,
		   other (ego) weapons have a high chance of getting destroyed */
		if ((f4 & TR4_BLACK_BREATH) ? (!rand_int(1000)) : ( /* see explanation above */
		    ((o_ptr->tval == TV_SWORD && o_ptr->sval == SV_DARK_SWORD)
		    || o_ptr->name2 == EGO_KILL_UNDEAD || o_ptr->name2b == EGO_KILL_UNDEAD
		    ) ? magik(3) : magik(20))) {
			object_desc(0, o_name, o_ptr, TRUE, 3);
			s_printf("NAZGUL_DISI_EGO: %s : %s.\n", p_ptr->name, o_name);
#ifdef USE_SOUND_2010
			sound_item(Ind, o_ptr->tval, o_ptr->sval, "kill_");
#endif

			msg_print(Ind, "\376\377rYour weapon is destroyed!");
			//inven_item_increase(Ind, INVEN_WIELD + weap, -1);
			//inven_item_optimize(Ind, INVEN_WIELD + weap);
			inven_item_increase(Ind, slot, -1);
			inven_item_describe_important(Ind, slot);
			inven_item_optimize(Ind, slot);
		}
	}

	/* Chance of getting the Black Breath */
	if (!p_ptr->black_breath &&
	    /* Hobbits and undead forms resist slightly, non-weapon attacks are especially susceptible */
	    magik((p_ptr->prace == RACE_HOBBIT || p_ptr->body_monster == RI_HALFLING_SLINGER || p_ptr->body_monster == RI_SLHOBBIT || p_ptr->suscep_life ? 5 : 10) + (o_ptr ? 0 : 10)))
		set_black_breath(Ind, MAX_PLAYERS);
}

void set_black_breath(int Ind, int cause) {
	player_type *p_ptr = Players[Ind];

	if (p_ptr->ghost) return;
#ifdef VAMPIRES_BB_IMMUNE
	if (p_ptr->prace == RACE_VAMPIRE) return;
#endif
	if (p_ptr->martyr) return;

	switch (cause) {
	case MAX_PLAYERS: s_printf("EFFECT: BLACK-BREATH - %s was infected by a Nazgul\n", p_ptr->name); break;
	case 0:
		s_printf("BLACK-BREATH (Morgul): %s\n", p_ptr->name);
		p_ptr->black_breath = 2; //'granted' via equipped item -> cannot be broken unless item is unequipped first
		break;
	default:
		if (cause > 0) s_printf("EFFECT: BLACK-BREATH - %s was infected by %s\n", p_ptr->name, Players[cause]->name);
		else {
			char m_name[MNAME_LEN];

			monster_desc(Ind, m_name, -cause, 0);
			s_printf("EFFECT: BLACK-BREATH - %s was infected by %s\n", p_ptr->name, m_name);
		}
	}

	if (!p_ptr->black_breath) {
		msg_print(Ind, "\376\377DYour foe calls upon your soul!");
		msg_print(Ind, "\376\377DYou feel the Black Breath slowly draining you of life...");
		p_ptr->black_breath = 1;
	}
}


/* Do a probability travel in a wall */
bool do_prob_travel(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	int x = p_ptr->px, y = p_ptr->py, tries = 0;
	bool do_move = TRUE;
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	dun_level *l_ptr = getfloor(&p_ptr->wpos);

	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Paranoia */
	if (dir == 5) return(FALSE);
	if ((dir < 1) || (dir > 9)) return(FALSE);

	/* No probability travel in sticky vaults */
	if ((zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK)) return(FALSE);

	/* Neither on NO_MAGIC levels */
	if (l_ptr && (l_ptr->flags1 & LF1_NO_MAGIC)) return(FALSE);

	x += ddx[dir];
	y += ddy[dir];

	while (++tries < 1000) {
		/* Do not get out of the level */
		if (!in_bounds(y, x)) {
			do_move = FALSE;
			break;
		}

		/* Still in rock ? continue */
		if ((!cave_empty_bold(zcave, y, x)) || (zcave[y][x].info & (CAVE_ICKY | CAVE_STCK | CAVE_NO_PROB)) /* <- sometimes vault space gets overwritten by normal rooms, retaining no-tele property */
		    /* don't prob into sickbay area (and not into shops either) */
		     || (f_info[zcave[y][x].feat].flags2 & FF2_NO_PROB)) {
			y += ddy[dir];
			x += ddx[dir];
			continue;
		}

		/* Everything is ok */
		do_move = TRUE;
		break;
	}
	if (tries == 1000) return(FALSE); /* fail */

	if (do_move) {
		int oy, ox;

		/* Save old location */
		oy = p_ptr->py;
		ox = p_ptr->px;

		/* Move the player */
		store_exit(Ind);
		p_ptr->py = y;
		p_ptr->px = x;

		/* Update the player indices */
		zcave[oy][ox].m_idx = 0;
		zcave[y][x].m_idx = 0 - Ind;
		cave_midx_debug(wpos, y, x, -Ind);
		grid_affects_player(Ind, ox, oy);

		/* Redraw new spot */
		everyone_lite_spot_move(Ind, wpos, p_ptr->py, p_ptr->px);

		/* Redraw old spot */
		everyone_lite_spot_move(Ind, wpos, oy, ox);

		/* Check for new panel (redraw map) */
		verify_panel(Ind);

		/* Update stuff */
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

		/* Update the monsters */
		p_ptr->update |= (PU_DISTANCE);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD);

		/* Hack -- quickly update the view, to reduce perceived lag */
		redraw_stuff(Ind);
		window_stuff(Ind);
	}
	return(TRUE);
}


/* Experimental! lets hope not bugged */
/* Wraith walk in own house */
bool wraith_access(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i;
	bool house = FALSE;
	cave_type **zcave = getcave(&p_ptr->wpos);

	for (i = 0; i < num_houses; i++) {
		if (inarea(&houses[i].wpos, &p_ptr->wpos)) {
			if (fill_house(&houses[i], FILL_PLAYER, p_ptr)) {
				house = TRUE;
				if (access_door(Ind, houses[i].dna, TRUE)
				    || admin_p(Ind))
#ifdef USE_MANG_HOUSE_ONLY
					/* Allow players access on any house grid, ie even on perma-walls of non-mangstyle houses? */
					return(TRUE);
#else
					/* Allow players access on house door, and additionally on permawalls of mangstyle houses (paranoia: !zcave) */
					return((houses[i].flags & HF_TRAD) ?
					    ((!zcave || zcave[p_ptr->py][p_ptr->px].feat == FEAT_WALL_HOUSE || feat_is_window(zcave[p_ptr->py][p_ptr->px].feat)) ? FALSE : TRUE)
					    : TRUE);
#endif
				break;
			}
		}
	}
	return(house ? FALSE : TRUE);
}


/*
 * Hack function to determine if the player has access to the GIVEN location,
 * using the function above.	- Jir -
 */
static bool wraith_access_virtual(int Ind, int y, int x) {
	player_type *p_ptr = Players[Ind];
	int oy = p_ptr->py, ox = p_ptr->px;
	bool result;

	p_ptr->py = y;
	p_ptr->px = x;

	result = wraith_access(Ind);

	p_ptr->py = oy;
	p_ptr->px = ox;

	return(result);
}



/* borrowed from ToME	- Jir - */
/* 'comfortably': also check for things like lava if player isn't fire immune. - C. Blue
 * This function is a mess and needs cleaning up, especially with wraith step now.
 * Why do we have all three flags WALL, NO_WALK, CAN_PASS and then still proceed PERMANENT on top of it even...? */
bool player_can_enter(int Ind, u16b feature, bool comfortably) {
	player_type *p_ptr = Players[Ind];
	bool pass_wall;
	bool only_wall = FALSE;

	/* Dungeon Master pass through everything (cept array boundary :) */
	if (p_ptr->admin_dm && !(f_info[feature].flags2 & FF2_BOUNDARY)) return(TRUE);

	/* Special one-way doors for quests: Allow traversing if we're on a CAVE_ICKY grid. */
	if (feature == FEAT_ESCAPE_DOOR || feature == FEAT_SICKBAY_DOOR) {
		cave_type cave = getcave(&p_ptr->wpos)[p_ptr->py][p_ptr->px];

		if ((cave.info & CAVE_ICKY) || (f_info[cave.feat].flags1 & FF1_PROTECTED)) return(TRUE);
		return(FALSE);
	}

	/* Player can not walk through "walls" unless in Shadow Form */
	//if (p_ptr->wraith_form || (PRACE_FLAG(PR1_SEMI_WRAITH)))
	if (/*p_ptr->wraith_form ||*/ p_ptr->ghost || p_ptr->tim_wraith) pass_wall = TRUE;
	else pass_wall = FALSE;

	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST) {
		if (feature == FEAT_HOME || (feature >= FEAT_DOOR_HEAD && feature <= FEAT_DOOR_TAIL)
		    || feature == FEAT_BUSH || feature == FEAT_TREE || feature == FEAT_DEAD_TREE)
			return(TRUE);
		//use natural drown/damage code for this stuff instead:
		//if (feat_is_deep_water(feature) || feat_is_deep_lava(feature)) return(FALSE);
	}

#if 0	// it's interesting.. hope we can have similar feature :)
	/* Wall mimicry force the player to stay in walls */
	if (p_ptr->mimic_extra & CLASS_WALL) only_wall = TRUE;
#endif

	switch (feature) {
	case FEAT_DEEP_WATER:
	case FEAT_GLIT_WATER:
	case FEAT_ANIM_DEEP_WATER_EAST:
	case FEAT_ANIM_DEEP_WATER_WEST:
	case FEAT_ANIM_DEEP_WATER_NORTH:
	case FEAT_ANIM_DEEP_WATER_SOUTH:
		if (comfortably &&
		    //!(p_ptr->immune_water || p_ptr->res_water ||.
		    !(p_ptr->can_swim || p_ptr->levitate || p_ptr->ghost || p_ptr->tim_wraith))
			break;
		return(TRUE);	/* you can pass, but you may suffer dmg */

	case FEAT_SHAL_LAVA:
	case FEAT_DEEP_LAVA:
	case FEAT_ANIM_SHAL_LAVA_EAST:
	case FEAT_ANIM_SHAL_LAVA_WEST:
	case FEAT_ANIM_SHAL_LAVA_NORTH:
	case FEAT_ANIM_SHAL_LAVA_SOUTH:
	case FEAT_ANIM_DEEP_LAVA_EAST:
	case FEAT_ANIM_DEEP_LAVA_WEST:
	case FEAT_ANIM_DEEP_LAVA_NORTH:
	case FEAT_ANIM_DEEP_LAVA_SOUTH:
	case FEAT_FIRE:
	case FEAT_GREAT_FIRE:
		if (comfortably && !p_ptr->immune_fire &&
		    !(p_ptr->resist_fire && p_ptr->oppose_fire))
			break;
		return(TRUE);	/* you can pass, but you may suffer dmg */

	case FEAT_DEAD_TREE:
		if ((p_ptr->levitate) || pass_wall || p_ptr->town_pass_trees) return(TRUE);
		break;
	case FEAT_BUSH:
	case FEAT_TREE:
		/* 708 = Ent (passes trees), 83/142 novice ranger, 345 ranger, 637 ranger chieftain, 945 high-elven ranger */
		if ((p_ptr->levitate) || (p_ptr->pass_trees) || pass_wall || p_ptr->town_pass_trees)
			return(TRUE);
		else break;
#if 0
	case FEAT_WALL_HOUSE:
		if (!pass_wall || !wraith_access_virtual(Ind), xxx, yyy) return(FALSE);
		return(TRUE);
#endif

	default:
		if ((p_ptr->climb) && (f_info[feature].flags1 & FF1_CAN_CLIMB)) return(TRUE);
		if ((p_ptr->levitate) && ((f_info[feature].flags1 & FF1_CAN_LEVITATE) || (f_info[feature].flags1 & FF1_CAN_FEATHER))) return(TRUE);

		if (only_wall && (f_info[feature].flags1 & FF1_FLOOR)) break;
		else if ((p_ptr->feather_fall || p_ptr->tim_wraith) && (f_info[feature].flags1 & FF1_CAN_FEATHER)) return(TRUE);

		if ((pass_wall || only_wall) && (f_info[feature].flags1 & FF1_CAN_PASS)) return(TRUE);
		/* Permanent walls that aren't specific 'terrain' (eg mountains): Cannot enter even in wraithform */
		else if (f_info[feature].flags1 & FF1_WALL) {
			if ((f_info[feature].flags1 & FF1_PERMANENT) && !(f_info[feature].flags1 & FF1_CAN_PASS)) return(FALSE);
			if (!pass_wall || (f_info[feature].flags1 & FF1_PERMANENT)) break;
		}
		/* doors, etc */
		if (f_info[feature].flags1 & FF1_NO_WALK) break;
		else if ((f_info[feature].flags1 & FF1_WEB) && (!(r_info[p_ptr->body_monster].flags7 & RF7_SPIDER))) break;

		return(TRUE);
	}

	/* Enter Wraithstep */
	if (!comfortably && (p_ptr->tim_wraithstep & 0x1) && (p_ptr->tim_wraithstep & 0xF0)
#if 0
	     && !pass_wall &&
	    !(f_info[feature].flags1 & FF1_FLOOR) && !((f_info[feature].flags1 & FF1_LOS) && !(f_info[feature].flags1 & FF1_WALL))
#endif
	    ) {
		p_ptr->tim_wraithstep &= ~0xF0;
		p_ptr->tim_wraith = 1;
		p_ptr->redraw |= PR_BPR_WRAITH_PROB;
		msg_format_near(Ind, "%s turns into a wraith!", p_ptr->name);
		msg_print(Ind, "You turn into a wraith!");
		return(TRUE);
	}

	return(FALSE);
}

/* Helper function for move_player():
   Handle various things that change for the player depending on the grid he left
   vs the new grid he just entered.
   Note: Further 'interesting' effects (searching, noticing objects, entering stores)
         are skipped here, as these are not applied when players are switching places
         but only on normal movement. */
static void moved_player(int Ind, player_type *p_ptr, cave_type **zcave, int ox, int oy) {
	int x = p_ptr->px, y = p_ptr->py;
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type *c_ptr = &zcave[y][x];
	struct c_special *cs_ptr;

	/* un-snow */
	p_ptr->temp_misc_1 &= ~0x08;

	grid_affects_player(Ind, ox, oy);

	cave_midx_debug(wpos, y, x, -Ind);

	/* Redraw new spot */
	everyone_lite_spot_move(Ind, wpos, y, x);
	/* Redraw old spot */
	everyone_lite_spot_move(Ind, wpos, oy, ox);

	/* Check for new panel (redraw map) */
	verify_panel(Ind);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
	/* Update the monsters */
	p_ptr->update |= (PU_DISTANCE);
	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Hack -- quickly update the view, to reduce perceived lag */
	redraw_stuff(Ind);
	window_stuff(Ind);

	/* Stairs, gates, fountains */
	if (!wpos->wz &&
	    (c_ptr->feat == FEAT_MORE || c_ptr->feat == FEAT_WAY_MORE ||
	    c_ptr->feat == FEAT_LESS || c_ptr->feat == FEAT_WAY_LESS)) {
		dungeon_type *d_ptr = NULL;

		if (c_ptr->feat == FEAT_MORE || c_ptr->feat == FEAT_WAY_MORE) d_ptr = wild_info[wpos->wy][wpos->wx].dungeon;
		if (c_ptr->feat == FEAT_LESS || c_ptr->feat == FEAT_WAY_LESS) d_ptr = wild_info[wpos->wy][wpos->wx].tower;

		/* paranoia - maybe it's a broken staircase ^^ */
		if (d_ptr) {
			if (!p_ptr->warning_staircase) {
				msg_print(Ind, "\374\377yHINT: You found a staircase. Press the according key '\377o<\377y' or '\377o>\377y' to enter!");
				s_printf("warning_staircase: %s\n", p_ptr->name);
				//p_ptr->warning_staircase = 1;
			}

			if (!p_ptr->warning_staircase_iddc && on_irondeepdive(&p_ptr->wpos) &&
			    ((WPOS_IRONDEEPDIVE_Z > 0 && (c_ptr->feat == FEAT_LESS || c_ptr->feat == FEAT_WAY_LESS)) ||
			    (WPOS_IRONDEEPDIVE_Z < 0 && (c_ptr->feat == FEAT_MORE || c_ptr->feat == FEAT_WAY_MORE)))) {
				msg_print(Ind, "\374\377oWARNING: \377yThe dark grey staircase indicates an 'Ironman' dungeon,");
				msg_print(Ind, "\374\377y         this particular one being the 'Ironman Deep Dive Challenge'!");
				if (p_ptr->mode & MODE_DED_IDDC)
					msg_print(Ind, "\374\377y         If you enter, you cannot escape until you make it to the bottom!");
				else
					msg_print(Ind, "\374\377y         If you enter, you cannot escape until you reach Menegroth!");
				s_printf("warning_staircase_iddc: %s\n", p_ptr->name);
				p_ptr->warning_staircase_iddc = 1;
			}
			/* Give a special tip for the Halls of Mandos, so DED_IDDC people aren't nervous they got the wrong dungeon aka "did I turn off the stove?" */
			else if (!p_ptr->warning_staircase_mandos && hallsofmandos_wpos_x == p_ptr->wpos.wx && hallsofmandos_wpos_y == p_ptr->wpos.wy
			    && ((hallsofmandos_wpos_z > 0 && (c_ptr->feat == FEAT_LESS || c_ptr->feat == FEAT_WAY_LESS)) ||
			    (hallsofmandos_wpos_z < 0 && (c_ptr->feat == FEAT_MORE || c_ptr->feat == FEAT_WAY_MORE)))) {
				msg_print(Ind, "\374\377oWARNING: \377yThe dark grey staircase indicates an 'Ironman' dungeon,");
				msg_print(Ind, "\374\377y         this particular one being the otherworldly Halls of Mandos!");
				msg_print(Ind, "\374\377y         If you enter, you cannot escape until you make it to the bottom!");
				s_printf("warning_staircase_mandos: %s\n", p_ptr->name);
				p_ptr->warning_staircase_mandos = 1;
			}
			else if (!p_ptr->warning_staircase_oneway) {
				if (d_ptr->flags2 & DF2_IRON) {
					msg_print(Ind, "\374\377oWARNING: \377yThe dark grey staircase indicates an 'Ironman' dungeon!");
					msg_print(Ind, "\374\377y         That means that you cannot escape until you reach the bottom and");
					msg_print(Ind, "\374\377y         read a scroll of word-of-recall there! Also, death is \377opermanent\377y!");
					s_printf("warning_staircase_oneway(iron): %s\n", p_ptr->name);
					p_ptr->warning_staircase_oneway = 1;
				} else if (d_ptr->flags1 & DF1_NO_UP) {
					msg_print(Ind, "\374\377oWARNING: \377yThe orange staircase indicates a 'No-up' dungeon!");
					msg_print(Ind, "\374\377y         That means that you cannot take a staircase back up. You can");
					msg_print(Ind, "\374\377y         only escape by reading a scroll of word-of-recall!");
					s_printf("warning_staircase_oneway(no_up): %s\n", p_ptr->name);
					p_ptr->warning_staircase_oneway = 1;
				} else if (d_ptr->flags1 & DF1_FORCE_DOWN) {
					msg_print(Ind, "\374\377oWARNING: \377yThe light red staircase indicates a 'Force-down' dungeon!");
					msg_print(Ind, "\374\377y         That means that you cannot escape until you reach the bottom and");
					msg_print(Ind, "\374\377y         read a scroll of word-of-recall there!");
					s_printf("warning_staircase_oneway(force_down): %s\n", p_ptr->name);
					p_ptr->warning_staircase_oneway = 1;
				}
			}
		}
	}

	if (!p_ptr->warning_voidjumpgate && c_ptr->feat == FEAT_BETWEEN) {
		msg_print(Ind, "\374\377yHINT: You found a void jump gate. You may press '\377o>\377y' to teleport!");
		s_printf("warning_voidjumpgate: %s\n", p_ptr->name);
		//p_ptr->warning_voidjumpgate = 1;
	}

	if (!p_ptr->warning_fountain && c_ptr->feat == FEAT_FOUNTAIN) {
		msg_print(Ind, "\374\377yHINT: You found a fountain. Press '\377o_\377y' if you want to drink from it!");
		s_printf("warning_fountain: %s\n", p_ptr->name);
		p_ptr->warning_fountain = 1;
	}


	/* Trigger traps */
	if (!p_ptr->ghost && !(p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST)) {
		if ((cs_ptr = GetCS(c_ptr, CS_TRAPS))) {
#ifndef ARCADE_SERVER
			bool hit = TRUE;
#endif

			/* Disturb */
			disturb(Ind, 0, 0);

			if (!cs_ptr->sc.trap.found) {
				/* Message */
				msg_print(Ind, "You triggered a trap!");

				/* Mark trap as found */
				trap_found(&p_ptr->wpos, y, x);
			}
#ifndef ARCADE_SERVER
			else if (magik(get_skill_scale(p_ptr, SKILL_TRAPPING, 90) - UNAWARENESS(p_ptr))) {
				msg_print(Ind, "You carefully avoid touching the trap.");
				hit = FALSE;
			}
#endif

			/* Hit the trap */
#ifndef ARCADE_SERVER
			if (hit)
#endif
				hit_trap(Ind);
		}
		/* For PvP: */
		if ((cs_ptr = GetCS(c_ptr, CS_MON_TRAP))) {
			py_hit_trap(Ind);
		}
		if ((cs_ptr = GetCS(c_ptr, CS_RUNE))) {
			py_warding_rune_break(Ind);
		}
	}
}

/*
 * Move player in the given direction, with the given "pickup" flag.
 *
 * This routine should (probably) always induce energy expenditure.
 *
 * Note that moving will *always* take a turn, and will *always* hit
 * any monster which might be in the destination grid.  Previously,
 * moving into walls was "free" and did NOT hit invisible monsters.
 */

 /* Bounds checking is used in dungeon levels <= 0, which is used
    to move between wilderness levels.

    The wilderness levels are stored in rings radiating from the town,
    see calculate_world_index for more information.

    Diagonals aren't handled properly, but I don't feel that is important.

    -APD-
 */
void move_player(int Ind, int dir, int do_pickup, char *consume_full_energy) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos, nwpos, old_wpos;

	int y, x, oldx, oldy;
	int i;

	cave_type *c_ptr;
	struct c_special *cs_ptr;
	byte *w_ptr;
	monster_race *r_ptr = &r_info[p_ptr->body_monster];
	cave_type **zcave;

	//bool do_move = FALSE;
	bool rnd = FALSE;
	int rnd_move =
	    ((r_ptr->flags1 & RF1_RAND_5) ? 5 : 0) + ((r_ptr->flags1 & RF1_RAND_10) ? 10 : 0) +
	    ((r_ptr->flags1 & RF1_RAND_25) ? 25 : 0) + ((r_ptr->flags1 & RF1_RAND_50) ? 50 : 0) +
	    ((r_ptr->flags1 & RF1_RAND_100) ? 100 : 0);
	int csmove = TRUE;
	int iterations = 15;

	if (!(zcave = getcave(wpos))) return;

	/* We haven't done anything that would consume full turn energy by default */
	if (consume_full_energy) *consume_full_energy = FALSE;

	/* (S)He's no longer AFK, lol */
	un_afk_idle(Ind);

	/* Can we move ? */
	if (r_ptr->flags2 & RF2_NEVER_MOVE) {
		msg_print(Ind, "You cannot move by nature.");
		disturb(Ind, 0, 0); /* Stop us in case we were running */
		return;
	}

	stop_precision(Ind); /* aimed precision shot gets interrupted by moving around */
	stop_shooting_till_kill(Ind);
	if (p_ptr->steamblast) {
		msg_print(Ind, "You cancel your preparations for a steam blast charge.");
		p_ptr->steamblast = 0;
	}

	/* Find the result of moving */

	/* Vampires turning into bats only get very slightly erratic movement */
	if (p_ptr->prace == RACE_VAMPIRE) {
		if (p_ptr->body_monster == RI_VAMPIRE_BAT && magik(5)) {
			do {
				i = randint(9);
				y = p_ptr->py + ddy[i];
				x = p_ptr->px + ddx[i];

				/* convenience hack: don't run into walls, because that's just too silly */
				if (!player_can_enter(Ind, zcave[y][x].feat, FALSE)) i = 5;
				/* ..and also don't switch sectors accidentally */
				if (zcave[y][x].feat == FEAT_PERM_CLEAR) i = 5;
				/* ..aand conveniently also don't move onto grids that don't allow running if we're currently running */
				if (p_ptr->running && !(f_info[zcave[y][x].feat].flags1 & FF1_CAN_RUN)) i = 5;
				/* convenience hack: don't stop running if we just left proximity of a wall */
				if (p_ptr->running) rnd = TRUE;
			} while (i == 5 && --iterations > 0);
			/* be nice and fall back safely.. */
			if (!iterations) {
				y = p_ptr->py + ddy[dir];
				x = p_ptr->px + ddx[dir];
			}
		} else {
			y = p_ptr->py + ddy[dir];
			x = p_ptr->px + ddx[dir];
		}
	}
	/* -C. Blue- I toned down monster RAND_xxx for a mimicking player,
	assuming that the mimic does not use the monster mind but its own to control
	the body, on the other hand the body still carries reflexes from the monster ;)
	- technical reason was to make more forms useful, especially RAND_50 forms */
	/* if (((p_ptr->pclass != CLASS_SHAMAN) || ((r_ptr->d_char != 'E') && (r_ptr->d_char != 'G'))) && */
	/* And now shamans gain advantage by linking to the being's mind instead of copying it..or something..err ^^ */
	else if ((p_ptr->pclass != CLASS_SHAMAN) && rnd_move && magik((rnd_move * 4) / 10)) {
		do {
			i = randint(9);
			y = p_ptr->py + ddy[i];
			x = p_ptr->px + ddx[i];

			/* convenience hack: don't run into walls, because that's just too silly */
			if (!player_can_enter(Ind, zcave[y][x].feat, FALSE)) i = 5;
			/* ..and also don't switch sectors accidentally */
			if (zcave[y][x].feat == FEAT_PERM_CLEAR) i = 5;
			/* ..aand conveniently also don't move onto grids that don't allow running if we're currently running */
			if (p_ptr->running && !(f_info[zcave[y][x].feat].flags1 & FF1_CAN_RUN)) i = 5;
			/* convenience hack: don't stop running if we just left proximity of a wall */
			if (p_ptr->running) rnd = TRUE;
		} while (i == 5 && --iterations > 0);
		/* be nice and fall back safely.. */
		if (!iterations) {
			y = p_ptr->py + ddy[dir];
			x = p_ptr->px + ddx[dir];
		}
	} else {
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];
	}

	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* Slip on icy floor */
	if ((c_ptr->feat == FEAT_ICE || c_ptr->slippery >= 1000) && (!p_ptr->feather_fall && !p_ptr->levitate && !p_ptr->tim_wraith &&
	    /* Except for /animals/monsters/ that are used to cold, especially Yeti and co */
	    !(p_ptr->body_monster && //(r_info[p_ptr->body_monster].flags3 & (RF3_ANIMAL | RF3_IM_COLD)) == (RF3_ANIMAL | RF3_IM_COLD)))) {
	    (r_info[p_ptr->body_monster].flags3 & RF3_IM_COLD)))) {
		//if (magik(70 - p_ptr->lev)) {
		//if (magik(55 - (p_ptr->lev > 50 ? 50 : p_ptr->lev) / 2 - (7 - 128 + adj_dex_th[p_ptr->stat_ind[A_DEX]]))) { //DEX adjustment: 0..27
		if (magik(2 * (35 - (p_ptr->lev > 50 ? 50 : p_ptr->lev) / 3 - (4 - 128 + adj_dex_ta[p_ptr->stat_ind[A_DEX]])))) { //DEX adjustment: 0..18
			iterations = 10;//not strictly needed here, but anyway
			do {
				i = randint(9);
				y = p_ptr->py + ddy[i];
				x = p_ptr->px + ddx[i];
			} while (i == 5 && --iterations > 0);
			if (c_ptr->slippery >= 1000) {
				c_ptr->slippery -= 1000;
				msg_print(Ind, "You slip on the oily floor."); //no confusion effect, unlike for monsters
			} else msg_print(Ind, "You slip on the icy floor.");
		} /* for now, don't decrease slipperyness if we didn't slip */
#if 0
		else tmp = dir;
#endif

		/* Fire/elec aura: burn up all the oil immediately; for now no fire effects or anything */
		if (p_ptr->sh_fire || p_ptr->sh_elec) c_ptr->slippery = 0;
	}

	/* Update wilderness positions */
	if (wpos->wz == 0) {
		/* Make sure he hasn't just changed depth */
		if (p_ptr->new_level_flag) return;

		/* save his old location */
		wpcopy(&old_wpos, wpos);
		wpcopy(&nwpos, wpos);
		oldx = p_ptr->px; oldy = p_ptr->py;

		/* we have gone off the map */
		if (!in_bounds(y, x)) {
			/* Hack: Nobody leaves (0, 0) while sector000separation is on - mikaelh */
			if (sector000separation && wpos->wx == WPOS_SECTOR000_X &&
			    wpos->wy == WPOS_SECTOR000_Y && !is_admin(p_ptr)) {
				return;
			}

			store_exit(Ind);

			/* find his new location */
			if (y <= 0) {
				/* new player location */
				nwpos.wy++;
				p_ptr->py = MAX_HGT - 2;
			}
			else if (y >= MAX_HGT - 1) {
				/* new player location */
				nwpos.wy--;
				p_ptr->py = 1;
			}
			else if (x <= 0) {
				/* new player location */
				nwpos.wx--;
				p_ptr->px = MAX_WID - 2;
			}
			else if (x >= MAX_WID - 1) {
				/* new player location */
				nwpos.wx++;
				p_ptr->px = 1;
			}

			/* check to make sure he hasnt hit the edge of the world */
			if (nwpos.wx < 0 || nwpos.wx >= MAX_WILD_X ||
			    nwpos.wy < 0 || nwpos.wy >= MAX_WILD_Y) {
				p_ptr->px = oldx;
				p_ptr->py = oldy;
				return;
			}

			/* Hack: Nobody enters (0, 0) while sector000separation is on - mikaelh */
			if (sector000separation && nwpos.wx == WPOS_SECTOR000_X &&
			    nwpos.wy == WPOS_SECTOR000_Y && !is_admin(p_ptr)) {
				p_ptr->px = oldx;
				p_ptr->py = oldy;
				return;
			}

			/* Remove the player from the old location */
			zcave[oldy][oldx].m_idx = 0;

			/* Show everyone that's he left */
			everyone_lite_spot_move(Ind, &p_ptr->wpos, oldy, oldx);

			/* forget his light and viewing area */
			forget_lite(Ind);
			forget_view(Ind);

			/* Hack -- take a turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);

			/* Change wpos */
			wpcopy(wpos, &nwpos);

			/* A player has left this depth */
			new_players_on_depth(&old_wpos, -1, TRUE);

			/* update the wilderness map */
			if (!p_ptr->ghost)
				p_ptr->wild_map[(p_ptr->wpos.wx + p_ptr->wpos.wy * MAX_WILD_X) / 8] |=
				    (1U << ((p_ptr->wpos.wx + p_ptr->wpos.wy * MAX_WILD_X) % 8));

			p_ptr->new_level_flag = TRUE;
			p_ptr->new_level_method = LEVEL_OUTSIDE;
			new_players_on_depth(wpos, 1, TRUE);
			return;
		}
	}

	/* Examine the destination */
	c_ptr = &zcave[y][x];

	w_ptr = &p_ptr->cave_flag[y][x];

	/* Save "last direction moved" */
	p_ptr->last_dir = dir;

	/* Bump into other players */
	if (c_ptr->m_idx < 0)
	    /* mountains for example are FF1_PERMANENT too! */
	    //&& (f_info[c_ptr->feat].flags1 & FF1_SWITCH_MASK)) /* never swich places into perma wall */
	{
		player_type *q_ptr = Players[0 - c_ptr->m_idx];
		int Ind2 = 0 - c_ptr->m_idx;
		bool blocks_important_feat = FALSE; /* does the player block an important feature, like staircases in towns? - C. Blue
						       always make them 'switch places' instead of bumping. */
		bool remain_in_store = FALSE, replace_in_store = FALSE;

		switch (c_ptr->feat) {
		case FEAT_SHOP:
			/* Only inn entrance is important */
			if (GetCS(c_ptr, CS_SHOP)->sc.omni != 7
			    && q_ptr->store_num != -1) { /* except if the player isn't actually shopping but just blocking the store entrance! */
				if (!q_ptr->admin_dm) remain_in_store = TRUE; /* Player is genuinely shopping, don't swap him off the store grid! */
				else replace_in_store = TRUE;
				break;
			}
			/* Fall through */
		case FEAT_WAY_MORE:
		case FEAT_WAY_LESS:
		case FEAT_MORE:
		case FEAT_LESS:
			//if (q_ptr->afk || !q_ptr->wpos.wz) blocks_important_feat = TRUE;
			/* Only on world surface? */
			if (!q_ptr->wpos.wz) blocks_important_feat = TRUE;
			break;
		default: break;
		}

		/* Check for an attack */
		if (cfg.use_pk_rules != PK_RULES_NEVER &&
		    check_hostile(Ind, Ind2)) {
			/* Full energy must always be consumed when attacking */
			if (consume_full_energy) *consume_full_energy = TRUE;

			py_attack(Ind, y, x, TRUE);
			return;
		}

		/* If both want to switch, do it */
#if 0
		/* TODO: always swap when in party
		 * this can allow one to pass through walls... :(
		 */
		else if ((!p_ptr->ghost && !q_ptr->ghost &&
		    ((ddy[q_ptr->last_dir] == -(ddy[dir]) &&
		    ddx[q_ptr->last_dir] == (-ddx[dir]))) ||
		    (player_in_party(p_ptr->party, Ind2) &&
		    q_ptr->store_num == -1)) ||
		    (q_ptr->admin_dm) )
#else
		else if (((!p_ptr->ghost && !q_ptr->ghost &&
		    (ddy[q_ptr->last_dir] == -(ddy[dir])) &&
		    (ddx[q_ptr->last_dir] == (-ddx[dir])) &&
		    !p_ptr->afk && !q_ptr->afk) ||
		    q_ptr->admin_dm || blocks_important_feat || (c_ptr->info2 & CAVE2_SWITCH))
		    /* don't switch someone 'out' of a shop, except for the Inn
		       or if the player isn't actually 'visiting' the shop but just glitched onto the grid: */
		    && !remain_in_store && !replace_in_store
//moved above	    && !(f_info[c_ptr->feat].flags1 & FF1_PERMANENT)) /* never swich places into perma wall (only case possible: if target player is admin) */
		    && (f_info[c_ptr->feat].flags1 & FF1_SWITCH_MASK) /* never swich places into perma wall */
		    && !(p_ptr->admin_dm && !(q_ptr->admin_dm || q_ptr->admin_wiz))) /* dm shouldn't switch with non-dms ever */
#endif
		{
			/* if (!((!wpos->wz) && (p_ptr->tim_wraith || q_ptr->tim_wraith)))*/
			/* switch places only if BOTH have WRAITHFORM or NONE has it, well or if target is a DM */
			if ((!(p_ptr->afk || q_ptr->afk) && /* dont move AFK players into trees to kill them */
			    ((p_ptr->tim_wraith && q_ptr->tim_wraith) || (!p_ptr->tim_wraith && !q_ptr->tim_wraith)))
			    || q_ptr->admin_dm || blocks_important_feat || (c_ptr->info2 & CAVE2_SWITCH)) {
				store_exit(Ind);
				store_exit(Ind2);

				c_ptr->m_idx = 0 - Ind;
				zcave[p_ptr->py][p_ptr->px].m_idx = 0 - Ind2;

				q_ptr->py = p_ptr->py;
				q_ptr->px = p_ptr->px;

				p_ptr->py = y;
				p_ptr->px = x;

				/* Tell both of them */
				/* Don't tell people they bumped into the Dungeon Master */
				if (!q_ptr->admin_dm) {
					/* Hack if invisible */
					if (p_ptr->play_vis[Ind2])
						msg_format(Ind, "You switch places with %s.", q_ptr->name);
					else
						msg_print(Ind, "You switch places with it.");

					/* Hack if invisible */
					if (q_ptr->play_vis[Ind])
						msg_format(Ind2, "You switch places with %s.", p_ptr->name);
					else
						msg_print(Ind2, "You switch places with it.");

					black_breath_infection(Ind, Ind2);
					stop_precision(Ind2);
					stop_shooting_till_kill(Ind2);

					/* Disturb both of them */
					disturb(Ind, 1, 0);
					disturb(Ind2, 1, 0);
				}

				moved_player(Ind2, q_ptr, zcave, p_ptr->px, p_ptr->py);
				moved_player(Ind, p_ptr, zcave, q_ptr->px, q_ptr->py);
			} else {
				black_breath_infection(Ind, Ind2); /* =p */
				disturb(Ind, 1, 0); /* turn off running, so player won't be un-AFK'ed automatically */
			}
			return;
		}

		/* Hack -- the Dungeon Master cannot bump people */
		else if (!p_ptr->admin_dm) {
			/* Don't tell people they bumped into the Dungeon Master */
			if (!q_ptr->admin_dm) {
				/* Tell both about it */
				/* Hack if invisible */
				int ball = has_ball(q_ptr);

				if (p_ptr->team && ball != -1 && q_ptr->team != p_ptr->team) {
					object_type *o_ptr = &q_ptr->inventory[ball];
					object_type tmp_obj;
					int tackle = randint(20);

					if (tackle > 10) {
						tmp_obj = *o_ptr;
						if (tackle < 18) {
							msg_format_near(Ind2, "\377v%s is tackled by %s", q_ptr->name, p_ptr->name);
							msg_format(Ind2, "\377r%s tackles you", p_ptr->name);
							tmp_obj.marked2 = ITEM_REMOVAL_NEVER;
							drop_near(TRUE, 0, &tmp_obj, -1, wpos, y, x);
						} else {
							msg_format_near(Ind2, "\377v%s gets the ball from %s", p_ptr->name, q_ptr->name);
							msg_format(Ind2, "\377v%s gets the ball from you", p_ptr->name);
							inven_carry(Ind, o_ptr);
						} /*the_sandman: added violet colour for successful tackles
							and red for attempts*/
						inven_item_increase(Ind2, ball, -1);
						inven_item_describe(Ind2, ball);
						inven_item_optimize(Ind2, ball);
						q_ptr->energy = 0;
					} else {
						msg_format(Ind2, "\377r%s tries to tackle you", p_ptr->name);
						msg_format(Ind, "\377rYou fail to tackle %s", q_ptr->name);
					}
				} else {
					if (p_ptr->play_vis[Ind2])
						msg_format(Ind, "You bump into %s.", q_ptr->name);
					else
						msg_print(Ind, "You bump into it.");

					/* Hack if invisible */
					if (q_ptr->play_vis[Ind])
						msg_format(Ind2, "%s bumps into you.", p_ptr->name);
					else
						msg_print(Ind2, "It bumps into you.");
				}

				black_breath_infection(Ind, Ind2);

				/* Disturb both parties */
				disturb(Ind, 1, 0);
				disturb(Ind2, 1, 0);

				return;
			}
		} else { /* is admin: */
			/* admin just does nothing instead of bumping into someone */
			return;
		}
	}

	/* Hack -- attack monsters */
	if (c_ptr->m_idx > 0) {
		/* Hack -- the dungeon master switches places with his monsters */
		if (p_ptr->admin_dm &&
		    /* except if he wields his scythe (uhoh!) */
		    (!p_ptr->instakills ||
		    /* except if he's not disabled auto-retaliation */
		    (p_ptr->running && p_ptr->inventory[INVEN_WIELD].note &&
		    strstr(quark_str(p_ptr->inventory[INVEN_WIELD].note), "@Ox")))) {
			/* save old player location */
			oldx = p_ptr->px;
			oldy = p_ptr->py;
			/* update player location */
			store_exit(Ind);
			p_ptr->px = m_list[c_ptr->m_idx].fx;
			p_ptr->py = m_list[c_ptr->m_idx].fy;

			/* update monster location */
			m_list[c_ptr->m_idx].fx = oldx;
			m_list[c_ptr->m_idx].fy = oldy;
			/* update cave monster indices */
			zcave[oldy][oldx].m_idx = c_ptr->m_idx;
			c_ptr->m_idx = -Ind;

			moved_player(Ind, p_ptr, zcave, oldx, oldy);
			return;
		}
		/* Questor? Bump -> talk :D */
		else if (m_list[c_ptr->m_idx].questor && !m_list[c_ptr->m_idx].questor_hostile) {
			disturb(Ind, 1, 0);
			/* hack: if we're already acquiring it, don't try to re-acquire it meaninglessly.
			   This happens when someone is keeping the directional key pressed down, sending
			   consecutive bump-into orders here, while the delayed input prompt has not yet
			   shown up for him. */
			if (!p_ptr->delay_str && !p_ptr->delay_cfr
			    /* ..also account for the small latency moment between clearing delay_str
			       and the prompt actually popping up on player's client-side: */
			    && !p_ptr->request_id)
				quest_interact(Ind, m_list[c_ptr->m_idx].quest, m_list[c_ptr->m_idx].questor_idx, NULL);
		}
		/* Attack */
		else {
			/* hack: admins who are running with their scythe won't perform a run-attack - C. Blue
			   and hack: cloaked players who are running _while wraithed_ will stop running first, too. */
			if ((p_ptr->instakills || p_ptr->cloaked) && p_ptr->running) {
				disturb(Ind, 0, 0); /* stop running first */
				return;
			}

			/* Full energy must always be consumed when attacking */
			if (consume_full_energy) *consume_full_energy = TRUE;

#if 0 /* not for now (note: this code doesn't work yet!) */
			/* Allow performing a different action that melee attacking */
			for (i = 0; i < INVEN_TOTAL; i++) {
				if (!p_ptr->inventory[i].note) continue;
				if (strstr(quark_str(p_ptr->inventory[i].note), "@M")) {
					if (p_ptr->stormbringer || (
 #ifdef AUTO_RET_CMD
					    !retaliate_cmd(Ind, fallback) &&
 #endif
					    !retaliate_item(Ind, item, at_O_inscription, fallback)))
						py_attack(Ind, y, x, TRUE);
				}
			}
			if (i == INVEN_TOTAL)
#endif
			/* Normal attack */
			py_attack(Ind, y, x, TRUE);
		}

		/* done in any case */
		return;
	}

	/* Prob travel */
	if (p_ptr->prob_travel && (!cave_floor_bold(zcave, y, x))) {
		(void)do_prob_travel(Ind, dir);
		return;
	}

	/* now this is temp while i redesign!!! - do not change  <- ok and who wrote this and when? =p */
	if (!(cs_ptr = GetCS(c_ptr, CS_RUNE))) { //fix for walking-over-rune panic save, maybe cleanup this code? - Kurzel
		cs_ptr = c_ptr->special;
		while (cs_ptr) {
			int tcv = csfunc[cs_ptr->type].activate(cs_ptr, y, x, Ind);

			cs_ptr = cs_ptr->next;
			if (!tcv) {
				csmove = FALSE;
				s_printf("csmove is false\n");
			}
		}
	}

	/* Player can not walk through "walls", but ghosts can */
	if (!player_can_enter(Ind, c_ptr->feat, FALSE) || !csmove) {
		bool my_home = FALSE;

		if (p_ptr->tim_wraith || p_ptr->ghost ||
		    (p_ptr->prace == RACE_VAMPIRE && p_ptr->lev >= 35) /* vampire mist QoL hack */
		    ) {
			if (c_ptr->feat == FEAT_WALL_HOUSE || feat_is_window(c_ptr->feat)) {
				if (!wraith_access_virtual(Ind, y, x)) {
					msg_print(Ind, "The wall blocks your movement.");
					disturb(Ind, 0, 0);

					/* Hack -- refund some energy */
					p_ptr->energy += level_speed(&p_ptr->wpos) / 2;

					return;
				}
				if (p_ptr->tim_wraith || p_ptr->ghost) msg_print(Ind, "You pass through the house wall.");
				else msg_print(Ind, "You swiftly change into vampiric mist to pass through the house wall."); /* Vampire race QoL */
				my_home = TRUE;
			}
		}

		if (!my_home) {
			/* Disturb the player */
			disturb(Ind, 0, 0);

			/* Notice things in the dark */
			if (!(*w_ptr & CAVE_MARK) &&
			    (p_ptr->blind || !(*w_ptr & CAVE_LITE))) {
				if (c_ptr->feat == FEAT_SIGN) {
					msg_print(Ind, "You feel some structure blocking your way.");
					*w_ptr |= CAVE_MARK;
					everyone_lite_spot(wpos, y, x);
				/* Rubble */
				} else if (c_ptr->feat == FEAT_RUBBLE) {
					msg_print(Ind, "You feel some rubble blocking your way.");
					*w_ptr |= CAVE_MARK;
					everyone_lite_spot(wpos, y, x);

					if (!p_ptr->warning_tunnel) {
						if (p_ptr->rogue_like_commands)
							msg_print(Ind, "\374\377yHINT: You can try to tunnel through obstacles with '\377o+\377y' key.");
						else
							msg_print(Ind, "\374\377yHINT: You can try to tunnel through obstacles with \377oSHIFT+t\377y.");
						msg_print(Ind, "\374\377y      Using a shovel or, even better, a pick increases chance of success.");
						s_printf("warning_tunnel: %s\n", p_ptr->name);
						p_ptr->warning_tunnel = 1;
					}
				/* Treasure - just for the 'warning' tip */
				} else if (c_ptr->feat == FEAT_MAGMA_K || c_ptr->feat == FEAT_QUARTZ_K || c_ptr->feat == FEAT_SANDWALL_K) {
					msg_print(Ind, "You feel a wall blocking your way.");
					*w_ptr |= CAVE_MARK;
					everyone_lite_spot(wpos, y, x);

					if (!p_ptr->warning_tunnel2) {
						if (p_ptr->rogue_like_commands)
							msg_print(Ind, "\374\377yHINT: You can try to dig out treasure with '\377o+\377y' key.");
						else
							msg_print(Ind, "\374\377yHINT: You can try to dig out treasure with \377oSHIFT+t\377y.");
						msg_print(Ind, "\374\377y      Using a shovel or, even better, a pick increases chance of success.");
						s_printf("warning_tunnel2: %s\n", p_ptr->name);
						p_ptr->warning_tunnel2 = 1;
					}
				/* Closed door */
				} else if ((c_ptr->feat < FEAT_SECRET && c_ptr->feat >= FEAT_DOOR_HEAD) ||
					 (c_ptr->feat == FEAT_HOME)) {
					msg_print(Ind, "You feel a closed door blocking your way.");
					*w_ptr |= CAVE_MARK;
					everyone_lite_spot(wpos, y, x);
				/* Tree */
				} else if (c_ptr->feat == FEAT_TREE || c_ptr->feat == FEAT_DEAD_TREE ||
				    c_ptr->feat == FEAT_BUSH) {
					msg_print(Ind, "You feel a tree blocking your way.");
					*w_ptr |= CAVE_MARK;
					everyone_lite_spot(wpos, y, x);
				/* Dark Pit */
				} else if (c_ptr->feat == FEAT_DARK_PIT) {
					msg_print(Ind, "You don't feel any ground ahead of you.");
					*w_ptr |= CAVE_MARK;
					everyone_lite_spot(wpos, y, x);
				/* Mountains */
				} else if (c_ptr->feat == FEAT_PERM_MOUNTAIN || c_ptr->feat == FEAT_HIGH_MOUNT_SOLID) {
					msg_print(Ind, "There is a steep mountain blocking your way.");
					*w_ptr |= CAVE_MARK;
					everyone_lite_spot(wpos, y, x);
				} else if (c_ptr->feat == FEAT_MOUNTAIN || c_ptr->feat == FEAT_HIGH_MOUNTAIN) {
					msg_print(Ind, "There is a mountain blocking your way.");
					*w_ptr |= CAVE_MARK;
					everyone_lite_spot(wpos, y, x);
				} else if (c_ptr->feat == FEAT_ABYSS || c_ptr->feat == FEAT_ABYSS_BOUNDARY) {
					msg_print(Ind, "There is an endless abyss blocking your way.");
					*w_ptr |= CAVE_MARK;
					everyone_lite_spot(wpos, y, x);
				} else if (c_ptr->feat == FEAT_CLOUDYSKY) {
					msg_print(Ind, "There is an endless depth below the clouds, blocking your way.");
					*w_ptr |= CAVE_MARK;
					everyone_lite_spot(wpos, y, x);
				} else if (c_ptr->feat == FEAT_SICKBAY_DOOR) {
					msg_print(Ind, "You feel a door blocking your way.");
					*w_ptr |= CAVE_MARK;
					everyone_lite_spot(wpos, y, x);
				} else if (c_ptr->feat == FEAT_WEB) {
					*w_ptr |= CAVE_MARK;
					everyone_lite_spot(wpos, y, x);
					msg_print(Ind, "You feel something sticky blocking your way.");
				/* Wall (or secret door) */
				} else {
					msg_print(Ind, "You feel a wall blocking your way.");
					//msg_format(Ind, "You feel %s.", f_text + f_info[c_ptr->feat].block);

					*w_ptr |= CAVE_MARK;
					everyone_lite_spot(wpos, y, x);
				}
			}

			/* Notice things */
			else {
				//struct c_special *cs_ptr;
				/* Closed doors */
				if ((c_ptr->feat < FEAT_SECRET && c_ptr->feat >= FEAT_DOOR_HEAD) ||
				    (c_ptr->feat == FEAT_HOME)) {
					if (p_ptr->easy_open) do_cmd_open(Ind, dir);
					else msg_print(Ind, "There is a closed door blocking your way.");
				} else if (p_ptr->auto_tunnel) {
					do_cmd_tunnel(Ind, dir, TRUE);
				} else if (p_ptr->easy_tunnel) {
					do_cmd_tunnel(Ind, dir, FALSE);
				} else {
					/* Rubble */
					if (c_ptr->feat == FEAT_RUBBLE) {
						msg_print(Ind, "There is rubble blocking your way.");

					if (!p_ptr->warning_tunnel) {
							if (p_ptr->rogue_like_commands)
								msg_print(Ind, "\374\377yHINT: You can try to tunnel through obstacles with '\377o+\377y' key.");
							else
								msg_print(Ind, "\374\377yHINT: You can try to tunnel through obstacles with \377oSHIFT+t\377y.");
							msg_print(Ind, "\374\377y      Using a shovel or, even better, a pick increases chance of success.");
							s_printf("warning_tunnel: %s\n", p_ptr->name);
							p_ptr->warning_tunnel = 1;
						}
					}
					/* Treasure - just for the 'warning' tip */
					else if (c_ptr->feat == FEAT_MAGMA_K || c_ptr->feat == FEAT_QUARTZ_K || c_ptr->feat == FEAT_SANDWALL_K) {
						msg_print(Ind, "There is a wall blocking your way.");
						//msg_print(Ind, "There is a wall with valuable minerals blocking your way.");

					if (!p_ptr->warning_tunnel2) {
							if (p_ptr->rogue_like_commands)
								msg_print(Ind, "\374\377yHINT: You can try to dig out treasure with '\377o+\377y' key.");
							else
								msg_print(Ind, "\374\377yHINT: You can try to dig out treasure with \377oSHIFT+t\377y.");
							msg_print(Ind, "\374\377y      Using a shovel or, even better, a pick increases chance of success.");
							s_printf("warning_tunnel2: %s\n", p_ptr->name);
							p_ptr->warning_tunnel2 = 1;
						}
					}
					/* Tree */
					else if (c_ptr->feat == FEAT_TREE || c_ptr->feat == FEAT_DEAD_TREE ||
					    c_ptr->feat == FEAT_BUSH) {
						msg_print(Ind, "There is a tree blocking your way.");
					} else if (c_ptr->feat == FEAT_DARK_PIT) {
						msg_print(Ind, "There is a dark pit in your way.");
					} else if (c_ptr->feat == FEAT_PERM_MOUNTAIN || c_ptr->feat == FEAT_HIGH_MOUNT_SOLID) {
						msg_print(Ind, "There is a steep mountain blocking your way.");
					} else if (c_ptr->feat == FEAT_MOUNTAIN || c_ptr->feat == FEAT_HIGH_MOUNTAIN) {
						msg_print(Ind, "There is a mountain blocking your way.");
					} else if (c_ptr->feat == FEAT_ABYSS || c_ptr->feat == FEAT_ABYSS_BOUNDARY) {
						msg_print(Ind, "There is an endless abyss blocking your way.");
					} else if (c_ptr->feat == FEAT_GLIT_WATER) { // BOUNDARY flavor - Kurzel
						msg_print(Ind, "There is endless glittering water blocking your way.");
					} else if (c_ptr->feat == FEAT_CLOUDYSKY) {
						msg_print(Ind, "There is an endless depth below the clouds, blocking your way.");
					} else if (c_ptr->feat == FEAT_SICKBAY_DOOR) {
						msg_print(Ind, "You are not allowed to enter the sickbay.");
					} else if (c_ptr->feat == FEAT_WEB) {
						msg_print(Ind, "There are thick spiderwebs blocking your way.");
					} else if (c_ptr->feat == FEAT_GRAND_MIRROR) {
						msg_print(Ind, "There is a grand mirror in your way.");
					/* Wall (or secret door) */
					} else if (c_ptr->feat != FEAT_SIGN) {
						msg_print(Ind, "There is a wall blocking your way.");
						//msg_format(Ind, "There is %s.", f_text + f_info[c_ptr->feat].block);
					}
				}
			}

			/* Hack -- refund some energy */
			p_ptr->energy += level_speed(&p_ptr->wpos) / 2;

			return;
		}
	}
	/* is this actually still needed or dead code? */
	else if ((c_ptr->feat == FEAT_DARK_PIT) && !p_ptr->feather_fall &&
	    !p_ptr->levitate && !p_ptr->tim_wraith && !p_ptr->admin_dm) {
		msg_print(Ind, "You can't cross the chasm.");

		disturb(Ind, 0, 0);

		/* Hack -- refund some energy */
		p_ptr->energy += level_speed(&p_ptr->wpos) / 2;

		return;
	}

	/* Normal movement */
	{
		int oy, ox;
		struct c_special *cs_ptr;

		/* Save old location */
		oy = p_ptr->py;
		ox = p_ptr->px;

		/* Move the player */
		store_exit(Ind);
		p_ptr->py = y;
		p_ptr->px = x;

		/* Update the player indices */
		zcave[oy][ox].m_idx = 0;
		zcave[y][x].m_idx = 0 - Ind;

		/* Spontaneous Searching */
		if ((p_ptr->skill_fos >= 75) || (0 == rand_int(76 - p_ptr->skill_fos))) search(Ind);

		/* Continuous Searching */
		if (p_ptr->searching) {
			if (p_ptr->pclass == CLASS_ROGUE && !p_ptr->rogue_heavyarmor) detect_bounty(Ind);
			else search(Ind);
		}

		/* Handle "objects" */
		if (c_ptr->o_idx && !p_ptr->admin_dm) carry(Ind, do_pickup, 0, FALSE);
		else Send_floor(Ind, 0);

		/* Handle "store doors" */
		if (((!p_ptr->ghost) || p_ptr->admin_dm) &&
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
		}

		/* Handle resurrection */
		else if (p_ptr->ghost && c_ptr->feat == FEAT_SHOP &&
		    (cs_ptr = GetCS(c_ptr, CS_SHOP)) && cs_ptr->sc.omni == 3) {
#ifdef PROHIBIT_GHOST_TRAVEL /* disallow ghost-travelling to other towns? */
			if (!(p_ptr->wild_map[(p_ptr->wpos.wx + p_ptr->wpos.wy * MAX_WILD_X) / 8] & (1U << ((p_ptr->wpos.wx + p_ptr->wpos.wy * MAX_WILD_X) % 8))))
				msg_print(Ind, "\377rThe temple priest turns you away!");
			else
#endif
			{
				/* Resurrect him */
				resurrect_player(Ind, 0);

				/* Give him some gold to restart */
				//if (p_ptr->lev > 1 && !p_ptr->admin_dm) {
				if (!p_ptr->admin_dm) {
					/* int i = (p_ptr->lev > 4)?(p_ptr->lev - 3) * 100:100; */
					//int i = (p_ptr->lev > 4)?(p_ptr->lev - 3) * 100 + (p_ptr->lev / 10) * (p_ptr->lev / 10) * 800 : 100;
//					int i = (p_ptr->lev > 4) ? 100 + (p_ptr->lev * p_ptr->lev * p_ptr->lev) / 5 : 100;
#if 0 /* got exploited by chain-dying on purpose */
					int i = 300 + (p_ptr->lev * p_ptr->lev * p_ptr->lev) / 2; /* buffed it greatly, yet still sensible */
#else
					int i = 300 + (p_ptr->lev * p_ptr->lev * p_ptr->lev) / 4 + p_ptr->lev * 15;
#endif
					msg_format(Ind, "The temple priest gives you %d gold pieces for your revival!", i);
					gain_au(Ind, i, FALSE, FALSE);
				}
			}
		}
#ifndef USE_MANG_HOUSE_ONLY
		else if ((c_ptr->feat == FEAT_HOME || c_ptr->feat == FEAT_HOME_OPEN)
		    && (!p_ptr->ghost || is_admin(p_ptr))) {
			disturb(Ind, 1, 0);
			do_cmd_trad_house(Ind);
			//return;	/* allow builders to build */
		}
#endif	// USE_MANG_HOUSE_ONLY


		/* Mega-hack -- if we are the dungeon master, and our movement hook
		 * is set, call it.  This is used to make things like building walls
		 * and summoning monster armies easier.
		 */

#if 0
		if ((!strcmp(p_ptr->name,cfg_dungeon_master) || player_is_king(Ind))
		    && p_ptr->master_move_hook)
#endif
		/* Check BEFORE setting ;) */
		if (p_ptr->master_move_hook)
			p_ptr->master_move_hook(Ind, NULL);

		/* Moved this down so it's after 'do_cmd_trad_house()' which sets store_num (used for ambient sfx in moved_player()->grid_affects_player()) */
		moved_player(Ind, p_ptr, zcave, ox, oy);

		if (rnd) run_init(Ind, dir);
	}
}

void black_breath_infection(int Ind, int Ind2) {
	player_type *p_ptr = Players[Ind];
	player_type *q_ptr = Players[Ind2];

	/* Bree is a safe zone */
	if (in_bree(&p_ptr->wpos) || in_trainingtower(&p_ptr->wpos) || in_valinor(&p_ptr->wpos)) return;

	/* Prevent players who are AFK from getting infected in towns - mikaelh */
	if (p_ptr->black_breath && !q_ptr->black_breath &&
	    magik(q_ptr->prace == RACE_HOBBIT || q_ptr->body_monster == RI_HALFLING_SLINGER || q_ptr->body_monster == RI_SLHOBBIT || q_ptr->suscep_life ? 10 : 25) &&
	    !(q_ptr->afk && istown(&q_ptr->wpos)) && q_ptr->lev > cfg.newbies_cannot_drop && q_ptr->lev >= BB_INFECT_MINLEV)
		set_black_breath(Ind2, Ind);
	if (q_ptr->black_breath && !p_ptr->black_breath &&
	    magik(p_ptr->prace == RACE_HOBBIT || p_ptr->body_monster == RI_HALFLING_SLINGER || p_ptr->body_monster == RI_SLHOBBIT || p_ptr->suscep_life ? 10 : 25) &&
	    !(p_ptr->afk && istown(&p_ptr->wpos)) && p_ptr->lev > cfg.newbies_cannot_drop && p_ptr->lev >= BB_INFECT_MINLEV)
		set_black_breath(Ind, Ind2);
}

/*
 * Hack -- Check for a "motion blocker" (see below)
 */
int see_wall(int Ind, int dir, int y, int x) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Get the new location */
	y += ddy[dir];
	x += ddx[dir];

	/* Illegal grids are blank */
	/* XXX this should be blocked by permawalls, hopefully. */
	/* Had a crash occuring in cave_floor_bold check, y = 1, x = -1, 32,32,-500
	   So I'm trying an ugly hack - C. Blue */
	//if (!in_bounds(y, x)) { /* RUNNING_FIX_DEBUG */
	if (!in_bounds_array(y, x)) {
		/* Hack be sure the player is inbounds */
		if (p_ptr->px < 0) p_ptr->px = 0;
		if (p_ptr->py < 0) p_ptr->py = 0;
		if (p_ptr->px >= MAX_WID) p_ptr->px = MAX_WID - 1;
		if (p_ptr->py >= MAX_HGT) p_ptr->py = MAX_HGT - 1;
		/* Update the location's player index */
		zcave[p_ptr->py][p_ptr->px].m_idx = 0 - Ind;
		cave_midx_debug(wpos, p_ptr->py, p_ptr->px, -Ind);
		return(FALSE);
	}

	/* Ghosts run right through everything */
	if ((p_ptr->ghost || p_ptr->tim_wraith)) return(FALSE);

	/* Wraithstep initialization phase? */
	if ((p_ptr->tim_wraithstep & 0x1) && (p_ptr->tim_wraithstep & 0xF0)) return(FALSE);

#if 0
	/* Do wilderness hack, keep running from one outside level to another */
	if ((!in_bounds(y, x)) && (wpos->wz == 0)) return(FALSE);
#else
	/* replacing the above hack by simply using DONT_NOTICE_RUNNING | FLOOR | CAN_RUN
	   flags in f_info for feat FEAT_PERM_CLEAR (0x16, the invisible level border).
	   However, when changing direction while running on px=1 sometimes panics,
	   debugging atm.. - C. Blue */
	/* it seems to crash in update_view, because 'cave_los_grid' usually stops at the
	   level borders, preventing the game from trying to update cave grids outside of
	   the valid array. Seems adding those flags mentioned above breaks the cave_los_grid
	   check. Checking.. */
	/* too bad, removing FLOOR flag prevents running along gondo walls. appearently the
	   grids are treated in 2 different ways (when starting to run as walls due to missing
	   FLOOR flag, while running as floor-like open area). correcting that would probably
	   the cleanest way, if that assumption was correct. */
	/* Gonna add an in_bounds_array check to update_view instead, easy as that? */
	/* Pft, it works, but update_lite also relies on the missing FLOOR, lol.
	   Ok, really gonna try and fix the root instead, ie the different treatment by
	   run-initialization in comparison to continue-running-testing.. */
	if (zcave[y][x].feat == FEAT_PERM_CLEAR) return(FALSE); /* here goes part 1.. */
	/* appearently run_init() works ok, ie treats them as open area thanks to calling see_wall().
	   checking run_test() now.. done. Added FEAT_PERM_CLEAR checks there too. Seems working fine now!
	   (Appearently those grids aren't CAVE_MARK'ed.) */
#endif

#if 1 //def NEW_RUNNING_FEAT
	/* don't accept trees as open area? */
	if (p_ptr->running_on_floor && (zcave[y][x].feat == FEAT_DEAD_TREE || zcave[y][x].feat == FEAT_TREE || zcave[y][x].feat == FEAT_BUSH)) return(TRUE);
#endif

	/* Must actually block motion */
	if (cave_floor_bold(zcave, y, x)) return(FALSE);

	if (f_info[zcave[y][x].feat].flags1 & FF1_CAN_RUN) return(FALSE);

#if 1 /* NEW_RUNNING_FEAT */
	/* hack - allow 'running' when levitating over something */
	if ((f_info[zcave[y][x].feat].flags1 & (FF1_CAN_LEVITATE | FF1_CAN_RUN)) && p_ptr->levitate) return(FALSE);
	/* hack - allow 'running' if player may pass trees  */
	if ((zcave[y][x].feat == FEAT_DEAD_TREE || zcave[y][x].feat == FEAT_TREE || zcave[y][x].feat == FEAT_BUSH)
	     && p_ptr->pass_trees) return(FALSE);
	/* hack - allow 'running' if player can swim - HARDCODED :( */
	if (feat_is_water(zcave[y][x].feat) && p_ptr->can_swim) return(FALSE);
#endif
	/* Must be known to the player */
	if (!(p_ptr->cave_flag[y][x] & CAVE_MARK)) return(FALSE);

	/* Default */
	return(TRUE);
}


/*
 * Hack -- Check for an "unknown corner" (see below)
 */
static int see_nothing(int dir, int Ind, int y, int x) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Get the new location */
	y += ddy[dir];
	x += ddx[dir];

	/* Illegal grids are unknown */
	if (!in_bounds(y, x)) return(TRUE);

	/* Memorized grids are known */
	if (p_ptr->cave_flag[y][x] & CAVE_MARK) return(FALSE);

	/* Non-floor grids are unknown */
	if (!cave_floor_bold(zcave, y, x)) return(TRUE);

	/* Viewable grids are known */
	if (player_can_see_bold(Ind, y, x)) return(FALSE);

	/* Default */
	return(TRUE);
}





/*
 * The running algorithm:                       -CJS-
 *
 * In the diagrams below, the player has just arrived in the
 * grid marked as '@', and he has just come from a grid marked
 * as 'o', and he is about to enter the grid marked as 'x'.
 *
 * Of course, if the "requested" move was impossible, then you
 * will of course be blocked, and will stop.
 *
 * Overview: You keep moving until something interesting happens.
 * If you are in an enclosed space, you follow corners. This is
 * the usual corridor scheme. If you are in an open space, you go
 * straight, but stop before entering enclosed space. This is
 * analogous to reaching doorways. If you have enclosed space on
 * one side only (that is, running along side a wall) stop if
 * your wall opens out, or your open space closes in. Either case
 * corresponds to a doorway.
 *
 * What happens depends on what you can really SEE. (i.e. if you
 * have no light, then running along a dark corridor is JUST like
 * running in a dark room.) The algorithm works equally well in
 * corridors, rooms, mine tailings, earthquake rubble, etc, etc.
 *
 * These conditions are kept in static memory:
 * find_openarea         You are in the open on at least one
 * side.
 * find_breakleft        You have a wall on the left, and will
 * stop if it opens
 * find_breakright       You have a wall on the right, and will
 * stop if it opens
 *
 * To initialize these conditions, we examine the grids adjacent
 * to the grid marked 'x', two on each side (marked 'L' and 'R').
 * If either one of the two grids on a given side is seen to be
 * closed, then that side is considered to be closed. If both
 * sides are closed, then it is an enclosed (corridor) run.
 *
 * LL           L
 * @x          LxR
 * RR          @R
 *
 * Looking at more than just the immediate squares is
 * significant. Consider the following case. A run along the
 * corridor will stop just before entering the center point,
 * because a choice is clearly established. Running in any of
 * three available directions will be defined as a corridor run.
 * Note that a minor hack is inserted to make the angled corridor
 * entry (with one side blocked near and the other side blocked
 * further away from the runner) work correctly. The runner moves
 * diagonally, but then saves the previous direction as being
 * straight into the gap. Otherwise, the tail end of the other
 * entry would be perceived as an alternative on the next move.
 *
 * #.#
 * ##.##
 * .@x..
 * ##.##
 * #.#
 *
 * Likewise, a run along a wall, and then into a doorway (two
 * runs) will work correctly. A single run rightwards from @ will
 * stop at 1. Another run right and down will enter the corridor
 * and make the corner, stopping at the 2.
 *
 * #@x    1
 * ########### ######
 * 2        #
 * #############
 * #
 *
 * After any move, the function area_affect is called to
 * determine the new surroundings, and the direction of
 * subsequent moves. It examines the current player location
 * (at which the runner has just arrived) and the previous
 * direction (from which the runner is considered to have come).
 *
 * Moving one square in some direction places you adjacent to
 * three or five new squares (for straight and diagonal moves
 * respectively) to which you were not previously adjacent,
 * marked as '!' in the diagrams below.
 *
 * ...!   ...
 * .o@!   .o.!
 * ...!   ..@!
 * !!!
 *
 * You STOP if any of the new squares are interesting in any way:
 * for example, if they contain visible monsters or treasure.
 *
 * You STOP if any of the newly adjacent squares seem to be open,
 * and you are also looking for a break on that side. (that is,
 * find_openarea AND find_break).
 *
 * You STOP if any of the newly adjacent squares do NOT seem to be
 * open and you are in an open area, and that side was previously
 * entirely open.
 *
 * Corners: If you are not in the open (i.e. you are in a corridor)
 * and there is only one way to go in the new squares, then turn in
 * that direction. If there are more than two new ways to go, STOP.
 * If there are two ways to go, and those ways are separated by a
 * square which does not seem to be open, then STOP.
 *
 * Otherwise, we have a potential corner. There are two new open
 * squares, which are also adjacent. One of the new squares is
 * diagonally located, the other is straight on (as in the diagram).
 * We consider two more squares further out (marked below as ?).
 *
 * We assign "option" to the straight-on grid, and "option2" to the
 * diagonal grid, and "check_dir" to the grid marked 's'.
 *
 * .s
 * @x?
 * #?
 *
 * If they are both seen to be closed, then it is seen that no
 * benefit is gained from moving straight. It is a known corner.
 * To cut the corner, go diagonally, otherwise go straight, but
 * pretend you stepped diagonally into that next location for a
 * full view next time. Conversely, if one of the ? squares is
 * not seen to be closed, then there is a potential choice. We check
 * to see whether it is a potential corner or an intersection/room entrance.
 * If the square two spaces straight ahead, and the space marked with 's'
 * are both blank, then it is a potential corner and enter if find_examine
 * is set, otherwise must stop because it is not a corner.
 */




/*
 * Hack -- allow quick "cycling" through the legal directions
 */
byte cycle[] =
{ 1, 2, 3, 6, 9, 8, 7, 4, 1, 2, 3, 6, 9, 8, 7, 4, 1 };

/*
 * Hack -- map each direction into the "middle" of the "cycle[]" array
 */
byte chome[] =
{ 0, 8, 9, 10, 7, 0, 11, 6, 5, 4 };

/*
 * The direction we are running
 */
/*static byte find_current;*/

/*
 * The direction we came from
 */
/*static byte find_prevdir;*/

/*
 * We are looking for open area
 */
/*static bool find_openarea;*/

/*
 * We are looking for a break
 */
/*static bool find_breakright;
static bool find_breakleft;*/



/*
 * Initialize the running algorithm for a new direction.
 *
 * Diagonal Corridor -- allow diaginal entry into corridors.
 *
 * Blunt Corridor -- If there is a wall two spaces ahead and
 * we seem to be in a corridor, then force a turn into the side
 * corridor, must be moving straight into a corridor here. ???
 *
 * Diagonal Corridor    Blunt Corridor (?)
 *       # #                  #
 *       #x#                 @x#
 *       @p.                  p
 */
static void run_init(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	int row, col, deepleft, deepright;
	int i, shortleft, shortright;
	cave_type **zcave;

	if (!(zcave = getcave(&p_ptr->wpos))) return;

	/* Manual direction changes reset the corner counter
	   (for safety reasons only, might be serious running
	   in the dungeon in a dangerous situation) */
	p_ptr->corner_turn = 0;

	/* Save the direction */
	p_ptr->find_current = dir;

	/* Assume running straight */
	p_ptr->find_prevdir = dir;

	/* Assume looking for open area */
	p_ptr->find_openarea = TRUE;

	/* Assume not looking for breaks */
	p_ptr->find_breakright = p_ptr->find_breakleft = FALSE;

	/* Assume no nearby walls */
	deepleft = deepright = FALSE;
	shortright = shortleft = FALSE;

	/* Find the destination grid */
	row = p_ptr->py + ddy[dir];
	col = p_ptr->px + ddx[dir];

	/* Extract cycle index */
	i = chome[dir];

#if 1 /* NEW_RUNNING_FEAT */
	if (cave_running_bold_notrees(p_ptr, zcave, p_ptr->py, p_ptr->px) &&
	    cave_running_bold_notrees(p_ptr, zcave, row, col))
		p_ptr->running_on_floor = TRUE;
#endif

	/* Check for walls */
	/* When in the town/wilderness, don't break left/right. -APD- */
	if (see_wall(Ind, cycle[i + 1], p_ptr->py, p_ptr->px)) {
		/* if in the dungeon */
		//if (p_ptr->wpos.wz)
		{
			p_ptr->find_breakleft = TRUE;
			shortleft = TRUE;
		}
	} else if (see_wall(Ind, cycle[i + 1], row, col)) {
		/* if in the dungeon */
		//if (p_ptr->wpos.wz)
		{
			p_ptr->find_breakleft = TRUE;
			deepleft = TRUE;
		}
	}

	/* Check for walls */
	if (see_wall(Ind, cycle[i - 1], p_ptr->py, p_ptr->px)) {
		/* if in the dungeon */
		//if (p_ptr->wpos.wz)
		{
			p_ptr->find_breakright = TRUE;
			shortright = TRUE;
		}
	} else if (see_wall(Ind, cycle[i - 1], row, col)) {
		/* if in the dungeon */
		//if (p_ptr->wpos.wz)
		{
			p_ptr->find_breakright = TRUE;
			deepright = TRUE;
		}
	}

	if (p_ptr->find_breakleft && p_ptr->find_breakright) {
		/* Not looking for open area */
		/* In the town/wilderness, always in an open area */
		//if (p_ptr->wpos.wz)
			p_ptr->find_openarea = FALSE;

		/* Hack -- allow angled corridor entry */
		if (dir & 0x01) {
			if (deepleft && !deepright)
				p_ptr->find_prevdir = cycle[i - 1];
			else if (deepright && !deepleft)
				p_ptr->find_prevdir = cycle[i + 1];
		}

		/* Hack -- allow blunt corridor entry */
		else if (see_wall(Ind, cycle[i], row, col)) {
			if (shortleft && !shortright)
				p_ptr->find_prevdir = cycle[i - 2];
			else if (shortright && !shortleft)
				p_ptr->find_prevdir = cycle[i + 2];
		}
	}
}


/*
 * Update the current "run" path
 *
 * Return TRUE if the running should be stopped
 */
/* TODO: aquatics should stop when next to non-water */
static bool run_test(int Ind) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;

	int prev_dir, new_dir, check_dir = 0;

	int row, col;
	int i, max, inv;
	int option, option2;
	bool aqua = p_ptr->can_swim || p_ptr->levitate || (get_skill_scale(p_ptr, SKILL_SWIM, 500) >= 7) ||
	    ((p_ptr->body_monster) && (
	    (r_info[p_ptr->body_monster].flags7 & RF7_AQUATIC) ||
	    (r_info[p_ptr->body_monster].flags3 & RF3_UNDEAD) ));

	cave_type *c_ptr, **zcave;
	byte *w_ptr;
	struct c_special *cs_ptr;
	bool unprotected;

	if (!(zcave = getcave(wpos))) return(FALSE);

	/* XXX -- Ghosts never stop running */
	if (p_ptr->ghost || p_ptr->tim_wraith) return(FALSE);

	unprotected = !(f_info[zcave[p_ptr->py][p_ptr->px].feat].flags1 & FF1_PROTECTED);

	/* No options yet */
	option = 0;
	option2 = 0;

	/* Where we came from */
	prev_dir = p_ptr->find_prevdir;

	/* Range of newly adjacent grids */
	max = (prev_dir & 0x01) + 1;

	/* Look at every newly adjacent square. */
	for (i = -max; i <= max; i++) {
		new_dir = cycle[chome[prev_dir] + i];

		row = p_ptr->py + ddy[new_dir];
		col = p_ptr->px + ddx[new_dir];

//		if (!in_bounds(row, col)) continue; /* RUNNING_FIX_DEBUG */
/* next line is actually doing the trick (allow running transitions of wilderness levels)
   but it's a bad hack, so just for debugging: */
//		if ((!in_bounds(row, col)) && (wpos->wz == 0)) continue; /* FIX_RUNNING_DEBUG_WILDTRANSITION */

		c_ptr = &zcave[row][col];
		w_ptr = &p_ptr->cave_flag[row][col];

		/* unlit grids abort running */
		if (!(c_ptr->info & (CAVE_LITE | CAVE_GLOW))
		    /* Except inside inns, which are assumed to be perma-lit! Otherwise the dark house walls during night would
		       prevent the player from running along the inner walls of the inn if he didn't have a light source! - C. Blue */
		    && unprotected) {
			if (p_ptr->warning_run_lite != 10) {
				p_ptr->warning_run_lite++;
				if (p_ptr->warning_run_lite == 9) {
					p_ptr->warning_run_lite = 0;
					msg_print(Ind, "\374\377yHINT: You cannot run in the dark. Press '\377ow\377y' and equip a light source!");
					s_printf("warning_run_lite: %s\n", p_ptr->name);
				}
			}
			return(TRUE);
		}

		/* Visible monsters abort running */
		if (c_ptr->m_idx > 0 &&
		    /* Pets don't interrupt running */
		    (!(m_list[c_ptr->m_idx].pet))
		    /* Even in Bree (despite of Santa Claus: Rogues in cloaking mode might want to not 'run him over' but wait for allies) - C. Blue */
		    ) {
			//dun_level *l_ptr = getfloor(&p_ptr->wpos);

			/* Visible monster */
			if (p_ptr->mon_vis[c_ptr->m_idx] &&
			   (!(m_list[c_ptr->m_idx].special) &&
			   //(!l_ptr || !(l_ptr->flags1 & LF1_CAN_ALWAYS_RUN)) &&  ---apparently not needed
			   r_info[m_list[c_ptr->m_idx].r_idx].level != 0))
				return(TRUE);
		}

#ifdef HOSTILITY_ABORTS_RUNNING /* pvp mode chars cannot run with this on */
		/* Hostile characters will stop each other from running.
		 * This should lessen the melee storming effect in PVP fights */
		if (c_ptr->m_idx < 0 && check_hostile(Ind, 0 - c_ptr->m_idx)) return(TRUE);
#endif

		/* Visible objects abort running */
		if (c_ptr->o_idx) {
			/* Visible object */
			if (p_ptr->obj_vis[c_ptr->o_idx]) return(TRUE);
		}

		/* Visible traps abort running */
		if ((cs_ptr = GetCS(c_ptr, CS_TRAPS)) && cs_ptr->sc.trap.found) return(TRUE);

		/* Hack -- basically stop in water */
		if (feat_is_deep_water(c_ptr->feat) && !aqua) return(TRUE);

		/* Assume unknown */
		inv = TRUE;

		/* Check memorized grids */
		if (*w_ptr & CAVE_MARK) {
			bool notice = TRUE;

			/* Examine the terrain */
			switch (c_ptr->feat) {
			/* FIXME: this can be funny with running speed boost */
			case FEAT_SHAL_LAVA:
			case FEAT_DEEP_LAVA:
			case FEAT_ANIM_SHAL_LAVA_EAST:
			case FEAT_ANIM_SHAL_LAVA_WEST:
			case FEAT_ANIM_SHAL_LAVA_NORTH:
			case FEAT_ANIM_SHAL_LAVA_SOUTH:
			case FEAT_ANIM_DEEP_LAVA_EAST:
			case FEAT_ANIM_DEEP_LAVA_WEST:
			case FEAT_ANIM_DEEP_LAVA_NORTH:
			case FEAT_ANIM_DEEP_LAVA_SOUTH:
			//FEAT_FIRE/FEAT_GREAT_FIRE?
				/* Ignore */
				if (p_ptr->invuln || p_ptr->immune_fire) notice = FALSE;
				/* Done */
				break;
			case FEAT_ICE:
				/* Ignore */
				if (p_ptr->feather_fall || p_ptr->levitate || p_ptr->tim_wraith) notice = FALSE;
				/* Done */
				break;
			/* Open doors */
			case FEAT_OPEN:
			case FEAT_BROKEN:
				/* Option -- ignore */
				if (p_ptr->find_ignore_doors) notice = FALSE;
				/* Done */
				break;
			/* Stairs */
			case FEAT_LESS:
			case FEAT_MORE:
			case FEAT_WAY_LESS:
			case FEAT_WAY_MORE:
			case FEAT_SHAFT_UP:
			case FEAT_SHAFT_DOWN:
			case FEAT_BETWEEN:
			case FEAT_CYCLIC_LESS:
			case FEAT_CYCLIC_MORE:
			case FEAT_BEACON:
				/* Option -- ignore */
				if (p_ptr->find_ignore_stairs && !in_deathfate2(wpos)) notice = FALSE;
				/* Done */
				break;
			/* Water */
			case FEAT_DEEP_WATER:
			case FEAT_GLIT_WATER:
			case FEAT_ANIM_DEEP_WATER_EAST:
			case FEAT_ANIM_DEEP_WATER_WEST:
			case FEAT_ANIM_DEEP_WATER_NORTH:
			case FEAT_ANIM_DEEP_WATER_SOUTH:
				if (aqua) notice = FALSE;
				/* Done */
				break;
			/* allow to run through wilderness transitions - C. Blue (compare see_wall comments) */
			case FEAT_PERM_CLEAR:
				notice = FALSE;
				break;
			}

			/* Check the "don't notice running" flag */
			if (f_info[c_ptr->feat].flags1 & FF1_DONT_NOTICE_RUNNING) notice = FALSE;

			/* Interesting feature */
			if (notice) return(TRUE);

			/* The grid is "visible" */
			inv = FALSE;
		}

		/* Option -- ignore */
		if (c_ptr->feat == FEAT_MON_TRAP && !p_ptr->find_ignore_montraps) return(TRUE);

		/* Analyze unknown grids and floors */
		/* wilderness hack to run from one level to the next */
		if (inv || ((!in_bounds(row, col)) && (wpos->wz == 0))  ||
		    (cave_running_bold(p_ptr, zcave, row, col)
		    /* If player is running on floor grids right now, don't treat tree grids as "passable" even if he could pass them: */
		    && !(cave_running_bold_notrees(p_ptr, zcave, p_ptr->py, p_ptr->px)
			&& cave_running_bold_trees(p_ptr, zcave, row, col)) )
		    ) {
			/* Looking for open area */
			if (p_ptr->find_openarea) ; /* Nothing */

			/* The first new direction. */
			else if (!option) option = new_dir;

			/* Three new directions. Stop running. */
			else if (option2) return(TRUE);

			/* Two non-adjacent new directions.  Stop running. */
			else if (option != cycle[chome[prev_dir] + i - 1]) return(TRUE);

			/* Two new (adjacent) directions (case 1) */
			else if (new_dir & 0x01) {
				check_dir = cycle[chome[prev_dir] + i - 2];
				option2 = new_dir;
			}

			/* Two new (adjacent) directions (case 2) */
			else {
				check_dir = cycle[chome[prev_dir] + i + 1];
				option2 = option;
				option = new_dir;
			}
		}

		/* Obstacle, while looking for open area */
		/* When in the town/wilderness, don't break left/right. */
		else {
			if (p_ptr->find_openarea) {
				if (i < 0) {
					/* Break to the right */
					//if (p_ptr->wpos.wz)
					p_ptr->find_breakright = (TRUE);
				} else if (i > 0) {
					/* Break to the left */
					//if (p_ptr->wpos.wz)
					p_ptr->find_breakleft = (TRUE);
				}
			}
		}
	}

	/* Looking for open area */
	if (p_ptr->find_openarea) {
		/* Hack -- look again */
		for (i = -max; i < 0; i++) {
			new_dir = cycle[chome[prev_dir] + i];

			row = p_ptr->py + ddy[new_dir];
			col = p_ptr->px + ddx[new_dir];

			//if ((!in_bounds(row, col)) && (wpos->wz == 0)) continue; /* FIX_RUNNING_DEBUG_WILDTRANSITION */

			/* Unknown grid or floor */
			if  (!(p_ptr->cave_flag[row][col] & CAVE_MARK) ||
			    (zcave[row][col].feat == FEAT_PERM_CLEAR) || /* allow running next to level border - C. Blue */
			    (cave_running_bold(p_ptr, zcave, row, col)
			    /* If player is running on floor grids right now, don't treat tree grids as "passable" even if he could pass them: */
			    && !(cave_running_bold_notrees(p_ptr, zcave, p_ptr->py, p_ptr->px)
			       && cave_running_bold_trees(p_ptr, zcave, row, col)) )
			    ) {
				/* Looking to break right */
				if (p_ptr->find_breakright) return(TRUE);
			}
			/* Obstacle */
			else {
				/* Looking to break left */
				if (p_ptr->find_breakleft) return(TRUE);
			}
		}

		/* Hack -- look again */
		for (i = max; i > 0; i--) {
			new_dir = cycle[chome[prev_dir] + i];

			row = p_ptr->py + ddy[new_dir];
			col = p_ptr->px + ddx[new_dir];

			//if ((!in_bounds(row, col)) && (wpos->wz == 0)) continue; /* FIX_RUNNING_DEBUG_WILDTRANSITION */

			/* Unknown grid or floor */
			if (!(p_ptr->cave_flag[row][col] & CAVE_MARK) ||
			    (zcave[row][col].feat == FEAT_PERM_CLEAR) || /* allow running next to level border - C. Blue */
			    (cave_running_bold(p_ptr, zcave, row, col)
			    /* If player is running on floor grids right now, don't treat tree grids as "passable" even if he could pass them: */
			    && !(cave_running_bold_notrees(p_ptr, zcave, p_ptr->py, p_ptr->px)
			       && cave_running_bold_trees(p_ptr, zcave, row, col)) )
			    ) {
				/* Looking to break left */
				if (p_ptr->find_breakleft) return(TRUE);
			}
			/* Obstacle */
			else {
				/* Looking to break right */
				if (p_ptr->find_breakright) return(TRUE);
			}
		}
	}

	/* Not looking for open area */
	else {
		/* No options */
		if (!option) return(TRUE);

		/* One option */
		else if (!option2) {
			/* Primary option */
			p_ptr->find_current = option;

			/* No other options */
			p_ptr->find_prevdir = option;
		}

		/* Two options, examining corners */
		else if (p_ptr->find_examine && !p_ptr->find_cut) {
			/* Primary option */
			p_ptr->find_current = option;

			/* Hack -- allow curving */
			p_ptr->find_prevdir = option2;
		}

		/* Two options, pick one */
		else {
			/* Get next location */
			row = p_ptr->py + ddy[option];
			col = p_ptr->px + ddx[option];

			/* Don't see that it is closed off. */
			/* This could be a potential corner or an intersection. */
			if (!see_wall(Ind, option, row, col) ||
			    !see_wall(Ind, check_dir, row, col)) {
				/* Can not see anything ahead and in the direction we */
				/* are turning, assume that it is a potential corner. */
				if (p_ptr->find_examine &&
				    see_nothing(option, Ind, row, col) &&
				    see_nothing(option2, Ind, row, col)) {
					p_ptr->find_current = option;
					p_ptr->find_prevdir = option2;
				}

				/* STOP: we are next to an intersection or a room */
				else return(TRUE);
			}

			/* This corner is seen to be enclosed; we cut the corner. */
			else if (p_ptr->find_cut) {
				p_ptr->find_current = option2;
				p_ptr->find_prevdir = option2;
			}

			/* This corner is seen to be enclosed, and we */
			/* deliberately go the long way. */
			else {
				p_ptr->find_current = option;
				p_ptr->find_prevdir = option2;
			}
		}
	}

	/* About to hit a known wall, stop */
	if (see_wall(Ind, p_ptr->find_current, p_ptr->py, p_ptr->px))return(TRUE);

	/* Failure */
	return(FALSE);
}



/*
 * Take one step along the current "run" path
 */
void run_step(int Ind, int dir, char *consume_full_energy) {
	player_type *p_ptr = Players[Ind];
	int prev_dir;

	/* slower 'running' movement over certain terrain */
	int real_speed = cfg.running_speed;
	cave_type *c_ptr, **zcave;

	/* We haven't done anything that would consume full turn energy by default */
	*consume_full_energy = FALSE;

	if (!(zcave = getcave(&p_ptr->wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	eff_running_speed(&real_speed, p_ptr, c_ptr);

	/* Check for just changed level */
	if (p_ptr->new_level_flag) return;

	/* Start running */
	if (dir) {
		/* Calculate torch radius */
		p_ptr->update |= (PU_TORCH);
		/* Initialize */
		run_init(Ind, dir);
		/* check if we have enough energy to move */
		if (p_ptr->energy < level_speed(&p_ptr->wpos) / real_speed) return;
	}

	/* Keep running */
	else {
		prev_dir = p_ptr->find_prevdir;

		/* Update run */
		if (run_test(Ind)) {
			/* Disturb */
			disturb(Ind, 0, 0);

			/* A break in running calms down */
			p_ptr->corner_turn = 0;

			/* Done */
			return;
		}

		if (p_ptr->warning_run < 3) p_ptr->warning_run++;

		/* C. Blue fun stuff =p */
		if (prev_dir != p_ptr->find_current) p_ptr->corner_turn++;
	}

	/* Decrease the run counter */
	if (--(p_ptr->running) <= 0) return;

	/* Move the player, using the "pickup" flag */
	move_player(Ind, p_ptr->find_current, p_ptr->always_pickup, consume_full_energy);
}

/*
 * Get a real chance of dodging, based on the player's dodge_level (1..100)
 * and the difficulty ie relation between attack level and player level - C. Blue
 * (This will only be used if NEW_DODGING is defined.)
 */
int apply_dodge_chance(int Ind, int attack_level) {
	int plev = Players[Ind]->lev;
	int skill = get_skill(Players[Ind], SKILL_DODGE);

	int dodge = Players[Ind]->dodge_level;
	int chance;

	if (Players[Ind]->paralyzed || Players[Ind]->stun > 100) return(0);

	/* Dodging doesn't work with a shield */
	if (Players[Ind]->inventory[INVEN_ARM].k_idx && Players[Ind]->inventory[INVEN_ARM].tval == TV_SHIELD) return(1);

	/* hack: adding 1000 to attack_level means it's a ranged attack and we are
	   supposed to halve the chance to dodge it */
	if (attack_level > 1000) {
		attack_level -= 1000;
		dodge /= 2;
	}

/* --- Limits lower attacker level to slightly above half dodger level */
	/* although this is a bit unfair, it keeps it somewhat sane (no perma-dodge vs lowbies). */
	if (attack_level < plev / 3) attack_level = plev / 3;
	/* and keep super-high-level players somewhat in line with their dodging chances, since
	   dodging is skill-dependant and shouldn't scale upwards too wildly with their level */
	if (attack_level < plev) attack_level = plev - (((plev - attack_level) * 2) / 3);
/* --- */

	/* lower limit (townies & co), preventing calc bugs */
	if (attack_level < 1) attack_level = 1;

	/* smooth out player level a little bit if it's above level 50, the cap for skill values,
	   to provide a smoother base for the following reduction-calculation a line below. */
	if (plev > 50) plev = 50 + (plev - 50) / 2;
	/* reduce the dodge-chance-reduction the dodger experiences vs high level monsters. */
	if (attack_level > plev) attack_level = plev + (attack_level - plev) / 3; /* was /2 */

	/* give especially low dodge chance during the first 2 levels,
	   because most chars start out with 1.000 dodging, and everyone
	   would dodge way too well right at the start then. */
	if (plev == 1) dodge /= 3;
	if (plev == 2) dodge = (dodge * 2) / 3;

	/* reduce player's effective dodge level if (s)he neglected to train dodging skill alongside character level. */
	/* note: training dodge skill +2 ahead (like other skills usually) won't help for dodging, sorry. */
	dodge = (dodge * (skill > plev ? plev : skill)) / (plev >= 50 ? 50 : plev);

	/* calculate real dodge chance from our dodge level, and relation of our level vs enemy level. */
#if 0
//	chance = (dodge * plev) / (attack_level * 3); /* 50vs50 -> 33%, 75vs75 -> 31%, 99vs100 -> 30%, 50vs100 -> 25%, 63 vs 127 -> 23%, 80 vs 50 -> 43% */
	chance = (dodge * plev * 2) / (attack_level * 5); /* 50vs50 -> 40%, 75vs75 -> 37%, 99vs100 -> 36%, 50vs100 -> 30%, 63 vs 127 -> 29%, 80 vs 50 -> 52% */
	/* with new lower-limit: 40vs20 -> 59%, 50vs50 -> 40%, 75vs75 -> 37%, 99vs100 -> 36%, 50vs100 -> 30%, 63 vs 127 -> 29%, 80 vs 50 -> 43% */
#else /* experimental: improve dodge chance in general, from ~1/3 vs high level to ~1/2 */
	chance = (dodge * plev * 2) / (attack_level * 3); /* 50vs100 -> 50%, 63 vs 127 -> 47% */
#endif

#if 1 /* instead of capping... */
	if (chance > DODGE_CAP) chance = DODGE_CAP;
#else /* ...let it scale? >:) */
	chance = (chance * DODGE_CAP) / 100;
#endif

	/* New- some malicious effects */
	if (Players[Ind]->confused) chance = (chance * 7) / 10;
	if (Players[Ind]->blind) chance = (chance * 2) / 10;
	if (Players[Ind]->image) chance = (chance * 8) / 10;
	if (Players[Ind]->stun) chance = (chance * 7) / 10;
	if (Players[Ind]->stun > 50) chance = (chance * 5) / 10;

	/* always slight chance to actually evade an enemy attack, no matter whether skilled or not :) */
	if (chance < 1) chance = 1;

	return(chance);
}

int apply_block_chance(player_type *p_ptr, int n) { /* n can already be modified chance */
	if (!p_ptr->shield_deflect || n <= 0 || p_ptr->paralyzed || p_ptr->stun > 100) return(0);
	if (p_ptr->confused) n = (n * 5) / 10;
	if (p_ptr->blind) n = (n * 3) / 10;
	if (p_ptr->image) n = (n * 5) / 10;
	if (p_ptr->stun) n = (n * 7) / 10;
	if (p_ptr->stun > 50) n = (n * 7) / 10;
	if (!n) n = 1;
	return(n);
}

int apply_parry_chance(player_type *p_ptr, int n) { /* n can already be modified chance */
	if (!p_ptr->weapon_parry || n <= 0 || p_ptr->paralyzed || p_ptr->stun > 100) return(0);
	if (p_ptr->confused) n = (n * 6) / 10;
	if (p_ptr->blind) n = (n * 1) / 10;
	if (p_ptr->image) n = (n * 7) / 10;
	if (p_ptr->stun) n = (n * 7) / 10;
	if (p_ptr->stun > 50) n = (n * 7) / 10;
	if (!n) n = 1;
	return(n);
}

/* Remember our previous pseudo-id feeling on flavoured items */
bool remember_sense(int Ind, int slot, object_type *o_ptr) {
	/* felt an (non-changing!) object of same kind before via pseudo-id? then remember.
	   Note: currently all objects for which that is true are 'magic', hence we only
	   use object_value_auxX_MAGIC() below. Note that if we add an item to an already
	   felt item in our inventory, the combined items will have ID_SENSE set of course,
	   so we won't get another "remember" message, just as it makes sense :) - C. Blue */
	if (!object_aware_p(Ind, o_ptr) && !object_known_p(Ind, o_ptr)
	    && !(o_ptr->ident & ID_SENSE) && object_felt_p(Ind, o_ptr)) {
		/* Also, rings and amulets aren't covered by auxX_magic, so we have to exempt them (null string!): */
		if (o_ptr->tval != TV_RING && o_ptr->tval != TV_AMULET) {
			char o_name[ONAME_LEN];
			cptr feel;

			object_desc(Ind, o_name, o_ptr, TRUE, 3);

			if (!object_felt_heavy_p(Ind, o_ptr)) {
				feel = value_check_aux2_magic(o_ptr);
				/* at least give a notice */
				if (feel) {
					msg_format(Ind, "You remember %s (%c) in your pack %s %s.",
					    o_name, index_to_label(slot), ((o_ptr->number != 1) ? "were" : "was"), feel);
					/* otherwise inscribe it textually */
					if (!o_ptr->note) {
						o_ptr->note = quark_add(feel);
						o_ptr->auto_insc = TRUE;
					}
					Send_item_newest_2nd(Ind, slot);
				}
			} else {
				feel = value_check_aux1_magic(o_ptr);
				/* at least give a notice */
				if (feel) {
					msg_format(Ind, "You remember %s (%c) in your pack %s %s.",
					    o_name, index_to_label(slot), ((o_ptr->number != 1) ? "were" : "was"), feel);
					/* otherwise inscribe it textually */
					if (!o_ptr->note) {
						o_ptr->note = quark_add(feel);
						o_ptr->auto_insc = TRUE;
					}
					Send_item_newest_2nd(Ind, slot);
				}
			}
			return(TRUE);
		}
		/* We have "felt" this kind of object already before */
		o_ptr->ident |= (ID_SENSE);
	}
	return(FALSE);
}
