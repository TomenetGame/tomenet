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


/* Nice: minimum level of a player to be able to become infected by Black Breath by another player */
#define BB_INFECT_MINLEV 25


/* Anti-(nothing)-hack, following Tony Zeigler's (Ravyn) suggestion */
bool nothing_test(object_type *o_ptr, player_type *p_ptr, worldpos *wpos, int x, int y)
{
	char o_name[ONAME_LEN];

	if ((o_ptr->wpos.wx != wpos->wx) || (o_ptr->wpos.wy != wpos->wy) || (o_ptr->wpos.wz != wpos->wz) ||
	    (o_ptr->ix && (o_ptr->ix != x)) || (o_ptr->iy && (o_ptr->iy != y))) {
		/* Item is not at the same (or similar) location as the player? Then he can't pick it up.. */
		object_desc(0, o_name, o_ptr, TRUE, 3);
		if (p_ptr != NULL) {
/*		s_printf("NOTHINGHACK: item %s at %d,%d,%d (%d,%d) meets not target of %s at %d,%d,%d (%d,%d)\n",
		    o_name, o_ptr->wpos.wx, o_ptr->wpos.wy, o_ptr->wpos.wz, o_ptr->ix, o_ptr->iy,
		    p_ptr->name, wpos->wx, wpos->wy, wpos->wz, x, y);*/
		}
		return TRUE;
	}
	return FALSE;
}

/*
 * Determine if the player "hits" a monster (normal combat).
 * Note -- Always miss 5%, always hit 5%, otherwise random.
 */
bool test_hit_fire(int chance, int ac, int vis)
{
	int k;

	/* Percentile dice */
	k = rand_int(100);

	/* Hack -- Instant miss or hit */
	if (k < 10) return (k < 5);

	/* Never hit */
	if (chance <= 0) return (FALSE);

	/* Invisible monsters are harder to hit */
	if (!vis) chance = (chance + 1) / 2;

	/* Power competes against armor */
	if (rand_int(chance) < (ac * 3 / 4)) return (FALSE);

	/* Assume hit */
	return (TRUE);
}



/*
 * Determine if the player "hits" a monster (normal combat).
 *
 * Note -- Always miss 5%, always hit 5%, otherwise random.
 */
bool test_hit_melee(int chance, int ac, int vis)
{
	int k;

	/* Percentile dice */
	k = rand_int(100);

	/* Hack -- Instant miss or hit */
	if (k < 10) return (k < 5);

	/* Wimpy attack never hits */
	if (chance <= 0) return (FALSE);

	/* Penalize invisible targets */
	if (!vis) chance = (chance + 1) / 2;

	/* Power must defeat armor */
	if (rand_int(chance) < (ac * 3 / 4)) return (FALSE);

	/* Assume hit */
	return (TRUE);
}



/*
 * Critical hits (from objects thrown by player)
 * Factor in item weight, total plusses, and player level.
 */
s16b critical_shot(int Ind, int weight, int plus, int dam)
{
	player_type *p_ptr = NULL;
	int i, k;
	bool boomerang = FALSE;
//	int xtra_crit = p_ptr->xtra_crit + p_ptr->inventory;
//	if xtra_crit > 50 cap
//	xtra_crit = 65 - (975 / (xtra_crit + 15));

	/* Extract "shot" power */
	if (Ind > 0) {
		p_ptr = Players[Ind];
		if (p_ptr->inventory[INVEN_BOW].tval == TV_BOOMERANG) boomerang = TRUE;

		i = (weight + ((p_ptr->to_h + plus) * 5) +
		    (boomerang ? 0 : get_skill_scale(p_ptr, SKILL_ARCHERY, 150)));
		i += 50 * BOOST_CRIT(p_ptr->xtra_crit);
	}
	else i = weight;

	/* Critical hit */
	if (randint(3500) <= i) {
		k = weight + randint(700);

		if (Ind > 0) k += (boomerang ? 0 : get_skill_scale(p_ptr, SKILL_ARCHERY, 100)) + randint(600 - (12000 / (BOOST_CRIT(p_ptr->xtra_crit) + 20)));

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

	return (dam);
}



/*
 * Critical hits (by player)
 *
 * Factor in weapon weight, total plusses, player level.
 */
s16b critical_melee(int Ind, int weight, int plus, int dam, bool allow_skill_crit, int o_crit)
{
	player_type *p_ptr = Players[Ind];
	int i, k, w;

	/* Critical hits for rogues says 'with light swords'
	in the skill description, which is logical since you
	cannot maneuver a heavy weapon so that it pierces through
	the small weak spot of an opponent's armour, hitting exactly
	into his artery or sth. So #if 0.. */

	/* Extract critical maneuver potential (interesting term..) */
	/* The larger the weapon the more difficult to quickly strike
	the critical spot. Cap weight influence at 100+ lbs */
	w = weight;
	if (w > 100) w = 10;
	else w = 110 - w;
	if (w < 10) w = 10; /* shouldn't happen anyways */
	i = (w * 2) + ((p_ptr->to_h + plus) * 5) + get_skill_scale(p_ptr, SKILL_MASTERY, 150);

	i += 50 * BOOST_CRIT(p_ptr->xtra_crit + o_crit);
	if (allow_skill_crit) i += get_skill_scale(p_ptr, SKILL_CRITS, 40 * 50);

	/* Chance */
	if (randint(5000) <= i) {
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

	return (dam);
}



/*
 * Brands (including slay mods) the given damage depending on object type, hitting a given monster.
 *
 * Note that "flasks of oil" do NOT do fire damage, although they
 * certainly could be made to do so.  XXX XXX
 *
 * Note that most brands and slays are x3, except Slay Animal (x2),
 * Slay Evil (x2), and Kill dragon (x5).
 */
/* accepts Ind <=0 */
s16b tot_dam_aux(int Ind, object_type *o_ptr, int tdam, monster_type *m_ptr, char *brand_msg, bool thrown)
{
	int mult = FACTOR_MULT;
	monster_race *r_ptr = race_inf(m_ptr);
	u32b f1, f2, f3, f4, f5, esp;
	player_type *p_ptr = NULL;

	struct worldpos *wpos = &m_ptr->wpos;
	cave_type **zcave;
	cave_type *c_ptr;
	char m_name[MNAME_LEN];

	object_type *e_ptr;
	u32b ef1, ef2, ef3, ef4, ef5, eesp;
	int brands_total = 0, brand_msgs_added = 0;
	/* char brand_msg[80];*/

	monster_race *pr_ptr = NULL;
	bool apply_monster_brands = TRUE;
	int i, monster_brands = 0;
	u32b monster_brand[6], monster_brand_chosen;
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
	if (o_ptr->k_idx) {
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

		/* Hack -- extract temp branding */
		if (Ind > 0 && p_ptr->bow_brand) {
			switch (p_ptr->bow_brand_t) {
			case BRAND_ELEC:
				f1 |= TR1_BRAND_ELEC;
				break;
			case BRAND_COLD:
				f1 |= TR1_BRAND_COLD;
				break;
			case BRAND_FIRE:
				f1 |= TR1_BRAND_FIRE;
				break;
			case BRAND_ACID:
				f1 |= TR1_BRAND_ACID;
				break;
			case BRAND_POIS:
				f1 |= TR1_BRAND_POIS;
				break;
			}
		}
	} else {
		f1 = 0; f2 = 0; f3 = 0; f4 = 0; f5 = 0;
	}


	/* Apply brands from mimic monster forms */
        if (Ind > 0 && p_ptr->body_monster) {
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
			if (!o_ptr->k_idx) apply_monster_brands = TRUE;
			break;
		}
		/* change a.m.b.=TRUE to =FALSE at declaration above if u use this if0-part again */
#endif
#if 0
		/* If monster is using range weapons, the player gets the brand(s) even on range attacks */
		if ((!pr_ptr->flags4 & RF4_ARROW_1) && is_ammo(o_ptr->tval))
			apply_monster_brands = FALSE;
		/* If monster is fighting with a weapon, the player gets the brand(s) even with a weapon */
        	/* If monster is fighting without weapons, the player gets the brand(s) only if
		he fights with bare hands/martial arts */
		/* However, if the monster doesn't use weapons but nevertheless fires ammo, the player
		gets the brand(s) on ranged attacks */
		if ((!pr_ptr->body_parts[BODY_WEAPON]) &&
		    is_weapon(o_ptr->tval))
			if (o_ptr->k_idx) apply_monster_brands = FALSE;
#endif
		/* The player never gets brands on ranged attacks from a form */
		if (is_ammo(o_ptr->tval))
			apply_monster_brands = FALSE;
		/* The player doesn't get brands if he uses a weapon but the monster doesn't */
		if ((o_ptr->k_idx) && (!pr_ptr->body_parts[BODY_WEAPON]))
			apply_monster_brands = FALSE;

		/* Get monster brands. If monster has several, choose one randomly */
		for (i = 0; i < 4; i++) {
			if (pr_ptr->blow[i].d_dice * pr_ptr->blow[i].d_side) {
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

	/* Add brands/slaying from non-weapon items (gloves, frost-armour) */
	if (Ind > 0) for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		e_ptr = &p_ptr->inventory[i];
		/* k_ptr = &k_info[e_ptr->k_idx];
		pval = e_ptr->pval; not needed */
		/* Skip missing items */
		if (!e_ptr->k_idx) continue;
		/* Extract the item flags */
		object_flags(e_ptr, &ef1, &ef2, &ef3, &ef4, &ef5, &eesp);

		/* Weapon/Bow/Ammo/Tool brands don't have general effect on all attacks */
		/* All other items have general effect! */
		if ((i != INVEN_WIELD) && (i != INVEN_BOW) && (i != INVEN_AMMO) && (i != INVEN_TOOL) &&
		    (i != INVEN_ARM || p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)) /* dual-wielders */
			f1 |= ef1;

		/* Add bow branding on correct ammo types */
		if (i == INVEN_BOW && e_ptr->tval == TV_BOW) {
			if(( (e_ptr->sval == SV_SHORT_BOW || e_ptr->sval == SV_LONG_BOW) && o_ptr->tval == TV_ARROW) ||
			   ( (e_ptr->sval == SV_LIGHT_XBOW || e_ptr->sval == SV_HEAVY_XBOW) && o_ptr->tval == TV_BOLT) ||
			   (e_ptr->sval == SV_SLING && o_ptr->tval == TV_SHOT))
				f1 |= ef1;
		}
	}

	/* Extra melee branding */
	if (Ind > 0 && !is_ammo(o_ptr->tval)) {
		/* Apply brands from (powerful) auras! */
		if (get_skill(p_ptr, SKILL_AURA_SHIVER) >= 30) f1 |= TR1_BRAND_COLD;
		if (get_skill(p_ptr, SKILL_AURA_DEATH) >= 40) f1 |= (TR1_BRAND_COLD | TR1_BRAND_FIRE);
		/* Temporary weapon branding */
		if ((p_ptr->inventory[INVEN_WIELD].k_idx || 
		    (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) && /* dual-wield */
		    p_ptr->brand) {
			switch (p_ptr->brand_t) {
			case BRAND_ELEC:
				f1 |= TR1_BRAND_ELEC;
				break;
			case BRAND_COLD:
				f1 |= TR1_BRAND_COLD;
				break;
			case BRAND_FIRE:
				f1 |= TR1_BRAND_FIRE;
				break;
			case BRAND_ACID:
				f1 |= TR1_BRAND_ACID;
				break;
			case BRAND_POIS:
				f1 |= TR1_BRAND_POIS;
				break;
			}
		}
	}

#if 1 /* for debugging only, so far: */
	/* Display message for all applied brands each */
	if (f1 & TR1_BRAND_ACID) brands_total++;
	if (f1 & TR1_BRAND_ELEC) brands_total++;
	if (f1 & TR1_BRAND_FIRE) brands_total++;
	if (f1 & TR1_BRAND_COLD) brands_total++;
	if (f1 & TR1_BRAND_POIS) brands_total++;

	/* Avoid contradictionary brands,
	   let one of them randomly 'flicker up' on striking, suppressing the other */
	if ((f1 & TR1_BRAND_FIRE) && (f1 & TR1_BRAND_COLD)) {
		if (magik(50)) f1 &= ~TR1_BRAND_FIRE;
		else f1 &= ~TR1_BRAND_COLD;
		/* Hack - Make it still say 'blabla is hit by THE ELEMENTS' */
		if (brands_total != 5) brands_total--;
	}

	strcpy(brand_msg,m_name);
	strcat(brand_msg," is ");//"%^s is ");
	switch (brands_total) {
		/* full messages for only 1 brand */
		case 1:
		if (f1 & TR1_BRAND_ACID) strcat(brand_msg,"covered in acid");
		if (f1 & TR1_BRAND_ELEC) strcat(brand_msg,"struck by electricity");
		if (f1 & TR1_BRAND_FIRE) strcat(brand_msg,"enveloped in flames");
		if (f1 & TR1_BRAND_COLD) strcat(brand_msg,"covered with frost");
		if (f1 & TR1_BRAND_POIS) strcat(brand_msg,"contacted with poison");
		break;
		/* fully combined messages for 2 brands */
		case 2:
		if (f1 & TR1_BRAND_ACID) {
			strcat(brand_msg,"covered in acid");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_COLD) {
			/* cold is grammatically combined with acid since the verbum 'covered' is identical */
			if (brand_msgs_added > 0) {
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and frost");
			    else strcat(brand_msg,", frost");
			}
			else strcat(brand_msg,"covered with frost");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_ELEC) {
			if (brand_msgs_added > 0) {
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"struck by electricity");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_FIRE) {
			if (brand_msgs_added > 0) {
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"enveloped in flames");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_POIS) {
			if (brand_msgs_added > 0) {
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"contacted with poison");
			brand_msgs_added++;
		}
		break;
		/* shorter messages if more brands have to fit in the message-line */
		case 3:		case 4:
		strcat(brand_msg,"hit by ");
		if (f1 & TR1_BRAND_ACID) {
			strcat(brand_msg,"acid");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_COLD) {
			if (brand_msgs_added > 0) {
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"frost");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_ELEC) {
			if (brand_msgs_added > 0) {
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"electricity");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_FIRE) {
			if (brand_msgs_added > 0) {
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"flames");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_POIS) {
			if (brand_msgs_added > 0) {
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"poison");
			brand_msgs_added++;
		}
		break;
		/* short and simple for all brands */
		case 5:
		strcat(brand_msg,"hit by the elements");
		break;
	}
	strcat(brand_msg,"!");
	if (brands_total > 0) {
		//msg_format(Ind, brand_msg, m_name);
	}
	else strcpy(brand_msg,"");
#endif
	/* Some "weapons" and "ammo" do extra damage */
	switch (o_ptr->tval) {
/*		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BLUNT:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
		case TV_BOOMERANG:*/
		default:
		{
			/* Slay Animal */
			if ((f1 & TR1_SLAY_ANIMAL) &&
			    (r_ptr->flags3 & RF3_ANIMAL)) {
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_ANIMAL;*/

				if (mult < FACTOR_HURT) mult = FACTOR_HURT;
			}

			/* Slay Evil */
			if (((f1 & TR1_SLAY_EVIL) || (Ind > 0 && get_skill(p_ptr, SKILL_HOFFENSE) >= 50)
#ifdef ENABLE_MAIA
			    || (Ind > 0 && p_ptr->prace == RACE_MAIA && (p_ptr->ptrait == TRAIT_ENLIGHTENED) && p_ptr->lev >= 50)
#endif
			    ) && (r_ptr->flags3 & RF3_EVIL)) {
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_EVIL;*/
				if (mult < FACTOR_HURT) mult = FACTOR_HURT;
			}

			/* Slay Undead */
			if (((f1 & TR1_SLAY_UNDEAD) || (Ind > 0 && get_skill(p_ptr, SKILL_HOFFENSE) >= 30)) &&
			    (r_ptr->flags3 & RF3_UNDEAD)) {
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_UNDEAD;*/
				if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
			}

			/* Slay Demon */
			if (((f1 & TR1_SLAY_DEMON) || (Ind > 0 && get_skill(p_ptr, SKILL_HOFFENSE) >= 40)) &&
			    (r_ptr->flags3 & RF3_DEMON)) {
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DEMON;*/
				if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
			}

			/* Slay Orc */
			if ((f1 & TR1_SLAY_ORC) &&
			    (r_ptr->flags3 & RF3_ORC)) {
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_ORC;*/
				if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
			}

			/* Slay Troll */
			if ((f1 & TR1_SLAY_TROLL) &&
			    (r_ptr->flags3 & RF3_TROLL)) {
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_TROLL;*/
				if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
			}

			/* Slay Giant */
			if ((f1 & TR1_SLAY_GIANT) &&
			    (r_ptr->flags3 & RF3_GIANT)) {
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_GIANT;*/
				if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
			}

			/* Slay Dragon  */
			if ((f1 & TR1_SLAY_DRAGON) &&
			    (r_ptr->flags3 & RF3_DRAGON)) {
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DRAGON;*/
				if (mult < FACTOR_SLAY) mult = FACTOR_SLAY;
			}

			/* Execute Dragon */
			if ((f1 & TR1_KILL_DRAGON) &&
			    (r_ptr->flags3 & RF3_DRAGON)) {
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DRAGON;*/
				if (mult < FACTOR_KILL) mult = FACTOR_KILL;
			}

			/* Execute Undead */
			if ((f1 & TR1_KILL_UNDEAD) &&
			    (r_ptr->flags3 & RF3_UNDEAD)) {
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_UNDEAD;*/
				if (mult < FACTOR_KILL) mult = FACTOR_KILL;
			}

			/* Execute Undead */
			if ((f1 & TR1_KILL_DEMON) &&
			    (r_ptr->flags3 & RF3_DEMON)) {
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DEMON;*/
				if (mult < FACTOR_KILL) mult = FACTOR_KILL;
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
				} else if (r_ptr->flags9 & RF9_RES_ACID) {
					if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				}
				/* Otherwise, take the damage */
				else if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
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
				} else if (r_ptr->flags9 & RF9_RES_ELEC) {
				    if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				}
				/* Otherwise, take the damage */
				else if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
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
				} else if (r_ptr->flags9 & RF9_RES_FIRE) {
				    if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				}
				/* Otherwise, take the damage */
				else if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
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
				}
				else if (r_ptr->flags9 & RF9_RES_COLD) {
				    if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				}
				/* Otherwise, take the damage */
				else if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
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
//					if (magik(95)) *special |= SPEC_POIS;
				} else if (r_ptr->flags9 & RF9_RES_POIS) {
				    if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				}
				/* Otherwise, take the damage */
				else if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
			}

			break;
		}
	}

#ifdef TEST_SERVER
	msg_format(Ind, "tdam %d, mult %d, FACTOR_MULT %d, thr: %d, ammo: %d, MA: %d, weap: %d", tdam, mult, FACTOR_MULT,
	    (tdam * (((mult - FACTOR_MULT) * 10L) / 4 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT),
	    (tdam * (((mult - FACTOR_MULT) * 20L) / 5 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT),
	    (tdam * (((mult - FACTOR_MULT) * 10L) / 3 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT),
	    (tdam * mult) / FACTOR_MULT);
#endif

	/* If the object was thrown, reduce brand effect by 75%
	   to avoid insane damage. */
	if (thrown) return ((tdam * (((mult - FACTOR_MULT) * 10L) / 4 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT));

	/* Ranged weapons get less benefit from brands */
	if (is_ammo(o_ptr->tval))
		return ((tdam * (((mult - FACTOR_MULT) * 20L) / 5 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT));
//		return ((tdam * mult) / FACTOR_MULT);

	/* Martial Arts styles get less benefit from brands */
	if (!o_ptr->k_idx)
		return ((tdam * (((mult - FACTOR_MULT) * 10L) / 3 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT));

	/* Return the total damage */
	return ((tdam * mult) / FACTOR_MULT);
}

/*
 * Brands (including slay mods) the given damage depending on object type, hitting a given player.
 *
 * Note that "flasks of oil" do NOT do fire damage, although they
 * certainly could be made to do so.  XXX XXX
 */
s16b tot_dam_aux_player(int Ind, object_type *o_ptr, int tdam, player_type *q_ptr, char *brand_msg, bool thrown)
{
	int mult = FACTOR_MULT;
	u32b f1, f2, f3, f4, f5, esp;
	player_type *p_ptr = Players[Ind];
	object_type *e_ptr;
	u32b ef1, ef2, ef3, ef4, ef5, eesp;
	int brands_total, brand_msgs_added;
	/* char brand_msg[80]; */

	monster_race *pr_ptr = &r_info[p_ptr->body_monster];
	bool apply_monster_brands = TRUE;
	int i, monster_brands = 0;
	u32b monster_brand[6], monster_brand_chosen;
	monster_brand[1] = 0;
	monster_brand[2] = 0;
	monster_brand[3] = 0;
	monster_brand[4] = 0;
	monster_brand[5] = 0;

	/* Extract the flags */
	if (o_ptr->k_idx)
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
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
			if (!o_ptr->k_idx) apply_monster_brands = TRUE;
			break;
		}
		/* change a.m.b.=TRUE to =FALSE at declaration above if u use this if0-part again */
#endif
#if 0
		/* If monster is using range weapons, the player gets the brand(s) even on range attacks */
		if ((!pr_ptr->flags4 & RF4_ARROW_1) && is_ammo(o_ptr->tval))
			apply_monster_brands = FALSE;
		/* If monster is fighting with a weapon, the player gets the brand(s) even with a weapon */
        	/* If monster is fighting without weapons, the player gets the brand(s) only if
		he fights with bare hands/martial arts */
		/* However, if the monster doesn't use weapons but nevertheless fires ammo, the player
		gets the brand(s) on ranged attacks */
		if ((!pr_ptr->body_parts[BODY_WEAPON]) &&
		    is_weapon(o_ptr->tval))
			apply_monster_brands = FALSE;
#endif
		/* The player never gets brands on ranged attacks from a form */
		if (is_ammo(o_ptr->tval))
			apply_monster_brands = FALSE;
		/* The player doesn't get brands if he uses a weapon but the monster doesn't */
		if ((o_ptr->k_idx) && (!pr_ptr->body_parts[BODY_WEAPON]))
			apply_monster_brands = FALSE;

		/* Get monster brands. If monster has several, choose one randomly */
		for (i = 0; i < 4; i++) {
		        if (pr_ptr->blow[i].d_dice * pr_ptr->blow[i].d_side) {
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

	/* Add brands/slaying from non-weapon items (gloves, frost-armour) */
        for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
                e_ptr = &p_ptr->inventory[i];
                /* k_ptr = &k_info[e_ptr->k_idx];
                pval = e_ptr->pval; not needed */
	        /* Skip missing items */
        	if (!e_ptr->k_idx) continue;
	        /* Extract the item flags */
                object_flags(e_ptr, &ef1, &ef2, &ef3, &ef4, &ef5, &eesp);

		/* Weapon/Bow/Ammo/Tool brands don't have general effect on all attacks */
		/* All other items have general effect! */
		if ((i != INVEN_WIELD) && (i != INVEN_BOW) && (i != INVEN_AMMO) && (i != INVEN_TOOL) &&
		    (i != INVEN_ARM || p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)) /* dual-wielders */
			f1 |= ef1;
	}

#if 1 /* for debugging only, so far: */
	/* Display message for all applied brands each */
	brands_total = 0;
	brand_msgs_added = 0;
	if (f1 & TR1_BRAND_ACID) brands_total++;
	if (f1 & TR1_BRAND_ELEC) brands_total++;
	if (f1 & TR1_BRAND_FIRE) brands_total++;
	if (f1 & TR1_BRAND_COLD) brands_total++;
	if (f1 & TR1_BRAND_POIS) brands_total++;

        /* Avoid contradictionary brands,
           let one of them randomly 'flicker up' on striking, suppressing the other */
	if ((f1 & TR1_BRAND_FIRE) && (f1 & TR1_BRAND_COLD)) {
                if (magik(50)) f1 &= ~TR1_BRAND_FIRE;
                else f1 &= ~TR1_BRAND_COLD;
                /* Hack - Make it still say 'blabla is hit by THE ELEMENTS' */
                if (brands_total != 5) brands_total--;
	}

	strcpy(brand_msg,q_ptr->name);
	strcat(brand_msg," is ");//"%^s is ");
	switch (brands_total) {
		/* full messages for only 1 brand */
		case 1:
		if (f1 & TR1_BRAND_ACID) strcat(brand_msg,"covered in acid");
		if (f1 & TR1_BRAND_ELEC) strcat(brand_msg,"struck by electricity");
		if (f1 & TR1_BRAND_FIRE) strcat(brand_msg,"enveloped in flames");
		if (f1 & TR1_BRAND_COLD) strcat(brand_msg,"covered with frost");
		if (f1 & TR1_BRAND_POIS) strcat(brand_msg,"contacted with poison");
		break;
		/* fully combined messages for 2 brands */
		case 2:
		if (f1 & TR1_BRAND_ACID)
		{
			strcat(brand_msg,"covered in acid");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_COLD)
		{
			/* cold is grammatically combined with acid since the verbum 'covered' is identical */
			if (brand_msgs_added > 0)
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and frost");
			    else strcat(brand_msg,", frost");
			}
			else strcat(brand_msg,"covered with frost");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_ELEC)
		{
			if (brand_msgs_added > 0)
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"struck by electricity");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_FIRE)
		{
			if (brand_msgs_added > 0)
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"enveloped in flames");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_POIS)
		{
			if (brand_msgs_added > 0)
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"contacted with poison");
			brand_msgs_added++;
		}
		break;
		/* shorter messages if more brands have to fit in the message-line */
		case 3:		case 4:
		strcat(brand_msg,"hit by ");
		if (f1 & TR1_BRAND_ACID)
		{
			strcat(brand_msg,"acid");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_COLD)
		{
			if (brand_msg[0])
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"frost");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_ELEC)
		{
			if (brand_msg[0])
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"electricity");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_FIRE)
		{
			if (brand_msg[0])
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"flames");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_POIS)
		{
			if (brand_msg[0])
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"poison");
			brand_msgs_added++;
		}
		break;
		/* short and simple for all brands */
		case 5:
		if (f1 & TR1_BRAND_ACID) strcat(brand_msg,"hit by the elements");
		break;
	}
	strcat(brand_msg,"!");
	if (brands_total > 0)
	{
		//msg_format(Ind, brand_msg, q_ptr->name);
	}
	else strcpy(brand_msg,"");
#endif
	/* Some "weapons" and "ammo" do extra damage */
	switch (o_ptr->tval)
	{
/*		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BLUNT:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
		case TV_BOOMERANG:*/
		default:
		{
			/* Brand (Acid) */
			if (f1 & TR1_BRAND_ACID)
			{
				/* Notice immunity */
				if (q_ptr->immune_acid)
				{
				}

				else if (q_ptr->resist_acid)
				{
					if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				}
				/* Otherwise, take the damage */
				else
				{
					if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
				}
			}

			/* Brand (Elec) */
			if (f1 & TR1_BRAND_ELEC)
			{
				/* Notice immunity */
				if (q_ptr->immune_elec)
				{
				}

				else if (q_ptr->resist_elec)
				{
					if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				}
				/* Otherwise, take the damage */
				else
				{
					if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
				}
			}

			/* Brand (Fire) */
			if (f1 & TR1_BRAND_FIRE)
			{
				/* Notice immunity */
				if (q_ptr->immune_fire)
				{
				}

				else if (q_ptr->resist_fire)
				{
					if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				}
				/* Otherwise, take the damage */
				else
				{
					if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
				}
			}

			/* Brand (Cold) */
			if (f1 & TR1_BRAND_COLD)
			{
				/* Notice immunity */
				if (q_ptr->immune_cold)
				{
				}

				else if (q_ptr->resist_cold)
				{
					if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				}
				/* Otherwise, take the damage */
				else
				{
					if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
				}
			}

			/* Brand (Poison) */
			if (f1 & TR1_BRAND_POIS)
			{
				/* Notice immunity */
				if (q_ptr->immune_poison)
				{
				}
				else if (q_ptr->resist_pois)
				{
					if (mult < FACTOR_BRAND_RES) mult = FACTOR_BRAND_RES;
				}
				/* Otherwise, take the damage */
				{
					if (mult < FACTOR_BRAND) mult = FACTOR_BRAND;
				}
			}
			break;
		}
	}

	/* If the object was thrown, reduce brand effect by 75%
	   to avoid insane damage. */
	if (thrown) return ((tdam * (((mult - FACTOR_MULT) * 10L) / 4 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT));

	/* Ranged weapons get less benefit from brands */
	if (is_ammo(o_ptr->tval))
		return ((tdam * (((mult - FACTOR_MULT) * 20L) / 5 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT));

	/* Martial Arts styles get less benefit from brands */
	if (!o_ptr->k_idx)
		return ((tdam * (((mult - FACTOR_MULT) * 10L) / 3 + 10 * FACTOR_MULT)) / (10 * FACTOR_MULT));

	/* Return the total damage */
	return ((tdam * mult) / FACTOR_MULT);
}

/*
 * Searches for hidden things.                  -RAK-
 */
 
void search(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int           y, x, chance;

	cave_type    *c_ptr;
	object_type  *o_ptr;
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	struct c_special *cs_ptr;
	if(!(zcave = getcave(wpos))) return;

	/* Admin doesn't */
	if (p_ptr->admin_dm) return;

	/* Start with base search ability */
	chance = p_ptr->skill_srh;

	/* Penalize various conditions */
	if (p_ptr->blind || no_lite(Ind)) chance = chance / 10;
	if (p_ptr->confused || p_ptr->image) chance = chance / 10;

	/* Search the nearby grids, which are always in bounds */
	
	for (y = (p_ptr->py - 1); y <= (p_ptr->py + 1); y++)
	{
		for (x = (p_ptr->px - 1); x <= (p_ptr->px + 1); x++)
		{
			/* Sometimes, notice things */
			if (rand_int(100) < chance)
			{
				/* Access the grid */
				c_ptr = &zcave[y][x];

				/* Access the object */
				o_ptr = &o_list[c_ptr->o_idx];

				/* Secret door */
				if (c_ptr->feat == FEAT_SECRET)
				{
					struct c_special *cs_ptr;

					/* Message */
					msg_print(Ind, "You have found a secret door.");

					/* Pick a door XXX XXX XXX */
					c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

					/* Clear mimic feature */
					if((cs_ptr=GetCS(c_ptr, CS_MIMIC)))
					{
						cs_erase(c_ptr, cs_ptr);
					}

					/* Notice */
					note_spot_depth(wpos, y, x);

					/* Redraw */
					everyone_lite_spot(wpos, y, x);
					/* Disturb */
					disturb(Ind, 0, 0);
				}

				/* Invisible trap */
//				if (c_ptr->feat == FEAT_INVIS)
				if((cs_ptr=GetCS(c_ptr, CS_TRAPS))){
					if (!cs_ptr->sc.trap.found)
					{
						/* Pick a trap */
						pick_trap(wpos, y, x);

						if (c_ptr->o_idx && !c_ptr->m_idx) {
							byte a = get_trap_color(Ind, cs_ptr->sc.trap.t_idx, c_ptr->feat);

							/* Hack - Always show traps under items when detecting - mikaelh */
							draw_spot_ovl(Ind, y, x, a, '^');
						} else {
							/* Normal redraw */
							lite_spot(Ind, y, x);
						}

						/* Message */
						msg_print(Ind, "You have found a trap.");

						/* Disturb */
						disturb(Ind, 0, 0);
					}
				}

				/* Search chests */
				else if (o_ptr->tval == TV_CHEST)
				{
					/* Examine chests for traps */
//					if (!object_known_p(Ind, o_ptr) && (t_info[o_ptr->pval]))
					if (!object_known_p(Ind, o_ptr) && (o_ptr->pval))
					{
						/* Message */
						msg_print(Ind, "You have discovered a trap on the chest!");

						/* Know the trap */
						object_known(o_ptr);

						/* Notice it */
						disturb(Ind, 0, 0);
					}
				}
			}
		}
	}
}


/* Hack -- tell player of the next object on the pile */
void whats_under_your_feet(int Ind) {
	object_type *o_ptr;

	char    o_name[ONAME_LEN];
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type *c_ptr;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
	c_ptr=&zcave[p_ptr->py][p_ptr->px];

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

	if (p_ptr->blind || no_lite(Ind))
		msg_format(Ind, "You feel %s%s here.", o_name, 
				o_ptr->next_o_idx ? " on a pile" : "");
	else msg_format(Ind, "You see %s%s.", o_name,
			o_ptr->next_o_idx ? " on a pile" : "");
}


/*
 * Player "wants" to pick up an object or gold.
 * Note that we ONLY handle things that can be picked up.
 * See "move_player()" for handling of other things.
 */
void carry(int Ind, int pickup, int confirm)
{
	object_type *o_ptr;

	char    o_name[ONAME_LEN], o_name_real[ONAME_LEN];
	u16b	old_note;
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type *c_ptr;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
	c_ptr=&zcave[p_ptr->py][p_ptr->px];

	/* Hack -- nothing here to pick up */
	if (!(c_ptr->o_idx)) return;

	/* Ghosts cannot pick things up */
	if ((p_ptr->ghost && !p_ptr->admin_dm)) return;

#if 0
    /* ok this is too harsh. also, it can be assumed that if you bow down to pick up items,
       they enter your wraith sphere well enough to become accessible */	
	/* not while in.. WRAITHFORM :D */
	if (p_ptr->tim_wraith && !p_ptr->admin_dm) return;
#endif

	/* Get the object */
	o_ptr = &o_list[c_ptr->o_idx];

	if (nothing_test(o_ptr, p_ptr, &p_ptr->wpos, p_ptr->px, p_ptr->py)) return;

	/* Auto id ? */
	if (p_ptr->auto_id) {
		object_aware(Ind, o_ptr);
		object_known(o_ptr);
	}

	/* Describe the object */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);
	object_desc(0, o_name_real, o_ptr, TRUE, 3);


/* log the encounters of players with special heavy armour, just for informative purpose */
//if (k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) s_printf("FOUND_WINNERS_ONLY: %s (%d) %s\n", p_ptr->name, p_ptr->wpos.wz, o_name_real);

	/* moved the object_felt_p() code downwards */


	/* Pick up gold */
	if (o_ptr->tval == TV_GOLD) {
		/* Disturb */
		disturb(Ind, 0, 0);

		/* hack for cloaking: since picking up anything breaks it,
		   we don't pickup gold except if the player really wants to */
		if ((p_ptr->cloaked == 1 || p_ptr->shadow_running) && !pickup) {
			if (p_ptr->blind || no_lite(Ind))
				msg_format(Ind, "You feel %s%s here.", o_name, 
						o_ptr->next_o_idx ? " on a pile" : "");
			else msg_format(Ind, "You see %s%s.", o_name,
						o_ptr->next_o_idx ? " on a pile" : "");
			Send_floor(Ind, o_ptr->tval);
			return;
		}

		if (p_ptr->inval && o_ptr->owner && p_ptr->id != o_ptr->owner) {
			msg_print(Ind, "\377oYou cannot take gold of other players without a valid account.");
			return;
		}

		if (compat_pomode(Ind, o_ptr)) {
			msg_format(Ind, "You cannot take money of %s players.", compat_pomode(Ind, o_ptr));
			return;
		}
		
		/* hack: prevent s32b overflow O_o (might happen to
		   'Illusionate' party at one point in the future? Dunno) */
		if (2000000000 - o_ptr->pval < p_ptr->au) {
			msg_format(Ind, "\377yYou cannot carry more than 2 billion worth of gold!");
			return;
		}

		/* Message */
		msg_format(Ind, "You have found %ld gold pieces worth of %s.",
			   (long int)o_ptr->pval, o_name);

#ifdef USE_SOUND_2010
		sound(Ind, "pickup_gold", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

/* #if DEBUG_LEVEL > 3 */
		if (o_ptr->pval >= 10000)
			s_printf("Gold found (%ld by %s at %d,%d,%d).\n", o_ptr->pval, p_ptr->name, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);

		/* Collect the gold */
		p_ptr->au += o_ptr->pval;

		/* Redraw gold */
		p_ptr->redraw |= (PR_GOLD);

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
					s_printf("%s -CHEEZY- Money transaction: %ldau from %s(%d) to %s(%d) at (%d,%d,%d)\n",
							showtime(), o_ptr->pval, name ? name : "(Dead player)", lev,
							p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
					c_printf("%s GOLD %s(%d,%s) %s(%d,%s) %ld\n",
							showtime(), name ? name : "(---)", lev, acc_name,
							p_ptr->name, p_ptr->lev, p_ptr->accountname, o_ptr->pval);
				} else {
					s_printf("%s Money transaction: %ldau from %s(%d) to %s(%d) at (%d,%d,%d)\n",
							showtime(), o_ptr->pval, name ? name : "(Dead player)", lev,
							p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
					c_printf("%s gold %s(%d,%s) %s(%d,%s) %ld\n",
							showtime(), name ? name : "(---)", lev, acc_name,
							p_ptr->name, p_ptr->lev, p_ptr->accountname, o_ptr->pval);
				}
 #else
				s_printf("%s Money transaction: %ldau from %s(%d) to %s(%d%s) at (%d,%d,%d)\n",
						showtime(), o_ptr->pval, name ? name : "(Dead player)", lev,
						p_ptr->name, p_ptr->lev,
						p_ptr->total_winner ? ",W" : (p_ptr->once_winner ? ",O" : ""),
						p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
				c_printf("%s GOLD %s(%d,%s) : %s(%d,%s%s) : %ld\n",
						showtime(), name ? name : "(---)", lev, acc_name,
						p_ptr->name, p_ptr->lev, p_ptr->accountname,
						p_ptr->total_winner ? ",W" : (p_ptr->once_winner ? ",O" : ""),
						o_ptr->pval);
 #endif
				/* Highlander Tournament: Don't allow transactions before it begins */
				if (!p_ptr->max_exp) gain_exp(Ind, 1);
			}
		}
#endif	// CHEEZELOG_LEVEL



		/* Delete gold */
//		delete_object(wpos, p_ptr->py, p_ptr->px);
		delete_object_idx(c_ptr->o_idx, FALSE);

		/* Hack -- tell the player of the next object on the pile */
		whats_under_your_feet(Ind);
	}

	/* Pick it up */
	else {
		bool force_pickup = check_guard_inscription(o_ptr->note, '=')
		    && p_ptr->id == o_ptr->owner;
		bool auto_load = check_guard_inscription(o_ptr->note, 'L')
		    && p_ptr->id == o_ptr->owner;

		/* Hack -- disturb */
		disturb(Ind, 0, 0);

		/* Describe the object */
		if (!pickup && !force_pickup) {
			char pseudoid[13];
			strcpy(pseudoid, "");
			/* felt an (non-changing!) object of same kind before via pseudo-id? then remember.
			   Note: currently all objects for which that is true are 'magic', hence we only
			   use object_value_auxX_MAGIC() below. */
			if (!object_aware_p(Ind, o_ptr) && !object_known_p(Ind, o_ptr) && object_felt_p(Ind, o_ptr)) {
				if (!object_felt_heavy_p(Ind, o_ptr)) {
					/* only show pseudoid if its current inscription doesn't already tell us! */
					if (!o_ptr->note || strcmp(quark_str(o_ptr->note), value_check_aux2_magic(o_ptr)))
						sprintf(pseudoid, " {%s}", value_check_aux2_magic(o_ptr));
				} else {
					/* only show pseudoid if its current inscription doesn't already tell us! */
					if (!o_ptr->note || strcmp(quark_str(o_ptr->note), value_check_aux1_magic(o_ptr)))
						sprintf(pseudoid, " {%s}", value_check_aux1_magic(o_ptr));
				}
			}

			if (p_ptr->blind || no_lite(Ind))
				msg_format(Ind, "You feel %s%s%s here.", o_name, pseudoid,
						o_ptr->next_o_idx ? " on a pile" : "");
			else msg_format(Ind, "You see %s%s%s.", o_name, pseudoid,
						o_ptr->next_o_idx ? " on a pile" : "");
			Send_floor(Ind, o_ptr->tval);
			return;
		}

		if (p_ptr->inval && o_ptr->owner && p_ptr->id != o_ptr->owner) {
			if ((o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_WORD_OF_RECALL) ||
			    (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_SATISFY_HUNGER) ||
			    (o_ptr->tval == TV_FOOD)) {
//				o_ptr->number = 1;
				o_ptr->discount = 100;
				if (o_ptr->level <= p_ptr->lev) {
//					o_ptr->owner = p_ptr->id;
					o_ptr->mode = p_ptr->mode;
				}
			// "Why not share ale? -Molt" <- good idea, here too!
			} else if (o_ptr->tval == TV_FOOD && o_ptr->sval == SV_FOOD_PINT_OF_ALE) {
				o_ptr->mode = p_ptr->mode;
				o_ptr->discount = 100;
  			} else if (o_ptr->tval == TV_FOOD && o_ptr->sval == SV_FOOD_PINT_OF_WINE) {
                                o_ptr->mode = p_ptr->mode;
				o_ptr->discount = 100;
                        } else {
				msg_print(Ind, "\377oYou cannot take items of other players without a valid account.");
				return;
			}
		}

		if (compat_pomode(Ind, o_ptr)) {
			/* Make an exception for WoR scrolls in case of rescue missions (become 100% off tho) */
			if ((o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_WORD_OF_RECALL) ||
			    (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_SATISFY_HUNGER) ||
			    (o_ptr->tval == TV_FOOD)) {
//				o_ptr->number = 1;
				o_ptr->discount = 100;
				if (o_ptr->level <= p_ptr->lev) {
//					o_ptr->owner = p_ptr->id;
					o_ptr->mode = p_ptr->mode;
				}
			/* Game pieces are free to be used */
			} else if ((o_ptr->tval == TV_SKELETON) && (o_ptr->sval >= 9)) {
				if (o_ptr->level <= p_ptr->lev) {
//					o_ptr->owner = p_ptr->id;
					o_ptr->mode = p_ptr->mode;
				}
		        /* exception for amulet of the highlands for tournaments */
    			} else if (o_ptr->tval == TV_AMULET && o_ptr->sval == SV_AMULET_HIGHLANDS) {
				o_ptr->mode = p_ptr->mode;
			// the one with esp
    			} else if (o_ptr->tval == TV_AMULET && o_ptr->sval == SV_AMULET_HIGHLANDS2) {
				o_ptr->mode = p_ptr->mode;
			// Why not share ale? -Molt
			} else if (o_ptr->tval == TV_FOOD && o_ptr->sval == SV_FOOD_PINT_OF_ALE) {
				o_ptr->mode = p_ptr->mode;
				o_ptr->discount = 100;
   			} else if (o_ptr->tval == TV_FOOD && o_ptr->sval == SV_FOOD_PINT_OF_WINE) {
                                o_ptr->mode = p_ptr->mode;
				o_ptr->discount = 100;
			} else {
				msg_format(Ind, "You cannot take items of %s players.", compat_pomode(Ind, o_ptr));
				return;
			}
		}

		/* prevent winners picking up true arts accidentally */
		if (true_artifact_p(o_ptr) && !winner_artifact_p(o_ptr) &&
		    p_ptr->total_winner && cfg.kings_etiquette) {
			msg_print(Ind, "Royalties may not own true artifacts!");
			if (!is_admin(p_ptr)) return;
		}

/*#ifdef RPG_SERVER -- let's do this also for normal server */
#if 1
		/* Turn level 0 food into level 1 food - mikaelh */
		if (o_ptr->owner && o_ptr->owner != p_ptr->id && o_ptr->level == 0 &&
		    ((o_ptr->tval == TV_FOOD && o_ptr->sval >= SV_FOOD_MIN_FOOD && o_ptr->sval <= SV_FOOD_MAX_FOOD) ||
		    (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_SATISFY_HUNGER)))
		{
			o_ptr->level = 1;
			o_ptr->discount = 100;
		}
#endif

/* the_sandman: item lvl restrictions are disabled in rpg */
#ifndef RPG_SERVER
		if ((o_ptr->owner) && (o_ptr->owner != p_ptr->id) &&
		    (o_ptr->level > p_ptr->lev || o_ptr->level == 0)) {
			if (cfg.anti_cheeze_pickup) {
				msg_print(Ind, "You aren't powerful enough yet to pick up that item!");
				return;
			/* new: this is to prevent newbies to pick up all nearby stuff with their
			   level 1 char aimlessly without being able to drop it again. */
			} else if (p_ptr->max_plv < cfg.newbies_cannot_drop) {
				msg_format(Ind, "You must at least be level %d to pick up items above your level.", cfg.newbies_cannot_drop);
				return;
			}
			if (true_artifact_p(o_ptr) && cfg.anti_arts_pickup)
//			if (artifact_p(o_ptr) && cfg.anti_arts_pickup)
			{
				msg_print(Ind, "You aren't powerful enough yet to pick up that artifact!");
				return;
			}

		}
#endif
		if ((k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) && !p_ptr->once_winner
		    && !p_ptr->total_winner) { /* <- added this just for testing when admin char sets .total_winner=1 */
			msg_print(Ind, "Only royalties are powerful enough to pick up that item!");
			if (!is_admin(p_ptr)) return;
		}

		/* Save old inscription in case pickup fails */
		old_note = o_ptr->note;

		/* Remove dangerous inscriptions when picking up items owned by other players - mikaelh */
		if (p_ptr->id != o_ptr->owner && o_ptr->note && p_ptr->clear_inscr) {
			char *inscription, *scan;
			scan = inscription = strdup(quark_str(o_ptr->note));

			while (*scan != '\0') {
				if (*scan == '@') {
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

#if 1
		/* Try to add to the quiver */
		if (object_similar(Ind, o_ptr, &p_ptr->inventory[INVEN_AMMO], 0x0)) {
			int slot = INVEN_AMMO, num = o_ptr->number;

			msg_print(Ind, "You add the ammo to your quiver.");

#if 0
			/* Own it */
			if (!o_ptr->owner) {
				o_ptr->owner = p_ptr->id;
				o_ptr->mode = p_ptr->mode;
			}
			can_use(Ind, o_ptr);
#endif	// 0

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
//			delete_object(wpos, p_ptr->py, p_ptr->px);
			delete_object_idx(c_ptr->o_idx, FALSE);

			/* Hack -- tell the player of the next object on the pile */
			whats_under_your_feet(Ind);

			/* Tell the client */
			//Send_floor(Ind, 0);

                        /* Note the spot */
                        note_spot_depth(wpos, p_ptr->py, p_ptr->px);

	                /* Draw the spot */
                        everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);

			/* Refresh */
			p_ptr->window |= PW_EQUIP;
		}
		/* Try to add to the empty quiver (XXX rewrite me - too long!) */
		else if (auto_load && is_ammo(o_ptr->tval) &&
		    !p_ptr->inventory[INVEN_AMMO].k_idx)
		{
			int slot = INVEN_AMMO;
			u32b f1 = 0 , f2 = 0 , f3 = 0, f4 = 0, f5 = 0, esp = 0;

			msg_print(Ind, "You add the ammo to your quiver.");

			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
			o_ptr->marked = 0;
			o_ptr->marked2 = ITEM_REMOVAL_NORMAL;

			/* Auto Curse */
			if (f3 & TR3_AUTO_CURSE) {
				/* The object recurse itself ! */
				o_ptr->ident |= ID_CURSED;
			}

			/* Cursed! */
			if (cursed_p(o_ptr)) {
				/* Warn the player */
				msg_print(Ind, "Oops! It feels deathly cold!");

				/* Note the curse */
				o_ptr->ident |= ID_SENSE | ID_SENSED_ONCE;
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
			//			delete_object(wpos, p_ptr->py, p_ptr->px);
			delete_object_idx(c_ptr->o_idx, FALSE);

			/* Hack -- tell the player of the next object on the pile */
			whats_under_your_feet(Ind);

			/* Tell the client */
			Send_floor(Ind, 0);

			/* Recalculate boni */
			p_ptr->update |= (PU_BONUS);

			/* Window stuff */
			p_ptr->window |= (PW_EQUIP);
		}
		/* Boomerangs: */
		else if (auto_load && o_ptr->tval == TV_BOOMERANG &&
		    !p_ptr->inventory[INVEN_BOW].k_idx && item_tester_hook_wear(Ind, INVEN_BOW))
		{
			int slot = INVEN_BOW;
			u32b f1 = 0 , f2 = 0 , f3 = 0, f4 = 0, f5 = 0, esp = 0;

			msg_print(Ind, "You ready the boomerang.");

			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
			o_ptr->marked = 0;
			o_ptr->marked2 = ITEM_REMOVAL_NORMAL;

			/* Auto Curse */
			if (f3 & TR3_AUTO_CURSE) {
				/* The object recurse itself ! */
				o_ptr->ident |= ID_CURSED;
			}

			/* Cursed! */
			if (cursed_p(o_ptr)) {
				/* Warn the player */
				msg_print(Ind, "Oops! It feels deathly cold!");

				/* Note the curse */
				o_ptr->ident |= ID_SENSE | ID_SENSED_ONCE;
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
			//			delete_object(wpos, p_ptr->py, p_ptr->px);
			delete_object_idx(c_ptr->o_idx, FALSE);

			/* Hack -- tell the player of the next object on the pile */
			whats_under_your_feet(Ind);

			/* Tell the client */
			Send_floor(Ind, 0);

			/* Recalculate boni */
			p_ptr->update |= (PU_BONUS);

			/* Window stuff */
			p_ptr->window |= (PW_EQUIP);
		}
#endif	// 0
		/* Note that the pack is too full */
		else if (!inven_carry_okay(Ind, o_ptr)) {
			msg_format(Ind, "You have no room for %s.", o_name);
			Send_floor(Ind, o_ptr->tval);

			/* Restore old inscription */
			o_ptr->note = old_note;

			return;
		}

		/* Pick up the item (if requested and allowed) */
		else {
			int okay = TRUE;

#if 0
			/* Hack -- query every item */
			if (p_ptr->carry_query_flag && !confirm) {
				char out_val[ONAME_LEN];
				snprintf(out_val, ONAME_LEN, "Pick up %s? ", o_name);
				Send_pickup_check(Ind, out_val);
				return;
			}
#endif	// 0

			/* Attempt to pick up an object. */
			if (okay) {
				int slot;


/* TEMPORARY ANTI-CHEEZE HACKS */  // todo: move to verify_level_req()
if (o_ptr->level != 0) {
	if (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_SPEED && (o_ptr->level < 30) && (o_ptr->bpval > 0)) {
		s_printf("HACK-SPEEDREQ: %s(%d) ring (+%d): %d -> ", p_ptr->name, p_ptr->lev, o_ptr->bpval, o_ptr->level);
		determine_level_req(75, o_ptr);
		s_printf("%d.\n", o_ptr->level);
	}
	if (o_ptr->tval == TV_RING && o_ptr->sval >= SV_RING_MIGHT && o_ptr->sval <= SV_RING_CUNNINGNESS && o_ptr->level < o_ptr->bpval * 3 + 15) {
		s_printf("HACK-STATSREQ: %s(%d) ring (+%d): %d -> ", p_ptr->name, p_ptr->lev, o_ptr->bpval, o_ptr->level);
		determine_level_req(25, o_ptr);
		s_printf("%d.\n", o_ptr->level);
	}
	if (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_POWER && (o_ptr->level < 45)) {
		s_printf("HACK-PDSM: %s(%d) : %d -> ", p_ptr->name, p_ptr->lev, o_ptr->level);
		determine_level_req(100, o_ptr);
		s_printf("%d.\n", o_ptr->level);
	}
	if (o_ptr->tval == TV_POTION && o_ptr->sval >= SV_POTION_INC_STR && o_ptr->sval <= SV_POTION_INC_CHR && o_ptr->level < 28) {
		s_printf("HACK-STATPOT: %s(%d) potion: %d -> ", p_ptr->name, p_ptr->lev, o_ptr->level);
		determine_level_req(20, o_ptr);
		s_printf("%d.\n", o_ptr->level);
	}
	if (o_ptr->tval == TV_SWORD && o_ptr->sval == SV_BLADE_OF_CHAOS && o_ptr->name2 == EGO_STORMBRINGER) {
		s_printf("HACK-STORMBRINGER.\n");
		o_ptr->level = 0;
	}
	if (o_ptr->tval == TV_HARD_ARMOR &&
	    (o_ptr->name2 == EGO_HEAVY_PRESERVANCE || o_ptr->name2b == EGO_HEAVY_PRESERVANCE) &&
	    o_ptr->pval > 1) {
		s_printf("HACK-PRESERVANCE: %s(%d) armour: +%d -> +1.\n", p_ptr->name, p_ptr->lev, o_ptr->pval);
		o_ptr->pval = 1;
	}
#ifndef ENABLE_RCRAFT
	/* Fix high quality runes levels */
	if (o_ptr->tval == TV_RUNE1) {
		s_printf("HACK-RUNE.\n");
		switch (o_ptr->sval) {
		case SV_RUNE1_CLOUD:
			o_ptr->level = 35;
			break;
		default:
			break;
		}
	}
	if (o_ptr->tval == TV_RUNE2) {
		s_printf("HACK-RUNE.\n");
		switch (o_ptr->sval) {
		case SV_RUNE2_STONE:
			o_ptr->level = 35;
			break;
		case SV_RUNE2_ARMAGEDDON:
			o_ptr->level = 40;
			break;
		default:
			break;
		}
	}
#endif
}
if (is_weapon(o_ptr->tval) && !(k_info[o_ptr->k_idx].flags4 & (TR4_MUST2H | TR4_SHOULD2H))
    && (o_ptr->name2 == EGO_LIFE || o_ptr->name2b == EGO_LIFE) && (o_ptr->pval > 2)) {
        o_ptr->pval = 2;
}


				/* Own it */
				if (!o_ptr->owner) {
					o_ptr->owner = p_ptr->id;
					o_ptr->mode = p_ptr->mode;

#if CHEEZELOG_LEVEL > 2
					if (true_artifact_p(o_ptr)) s_printf("%s Artifact %d found by %s(lv %d) at %d,%d,%d%s%s: %s\n",
									    showtime(), o_ptr->name1, p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, (c_ptr->info & CAVE_STCK) ? "N" : (c_ptr->info & CAVE_ICKY) ? "V" : "", (o_ptr->marked2 & ITEM_REMOVAL_NEVER) ? "G" : "", o_name_real);
					else if (o_ptr->name1 == ART_RANDART) s_printf("%s Randart found by %s(lv %d) at %d,%d,%d%s%s : %s\n",
									    showtime(), p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, (c_ptr->info & CAVE_STCK) ? "N" : (c_ptr->info & CAVE_ICKY) ? "V" : "", (o_ptr->marked2 & ITEM_REMOVAL_NEVER) ? "G" : "", o_name_real);

#endif	// CHEEZELOG_LEVEL
					/* log the encounters of players with special heavy armour, just for informative purpose */
					if (k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) s_printf("%s FOUND_WINNERS_ONLY: %s (%d) %s\n", showtime(), p_ptr->name, p_ptr->wpos.wz, o_name_real);
				}

#if CHEEZELOG_LEVEL > 2
				/* Take cheezelog
				 * TODO: ignore cheap items (like cure critical pot) */
				else if (p_ptr->id != o_ptr->owner &&
				        (o_ptr->tval != 1 && o_ptr->sval != 9) /* Heavy ball */ )
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
					c_printf("%s ITEM %s(%d,%s) %s(%d,%s) %lld(%d%%) %s\n",
							showtime(), name ? name : "(---)", lev, acc_name,
							p_ptr->name, p_ptr->lev, p_ptr->accountname,
							object_value_real(0, o_ptr), o_ptr->discount, o_name);
					} else {
					s_printf("%s Item transaction from %s(%d) to %s(%d) at (%d,%d,%d):\n  %s\n",
							showtime(), name ? name : "(Dead player)", lev,
							p_ptr->name, p_ptr->lev, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz,
							o_name);
					c_printf("%s item %s(%d,%s) %s(%d,%s) %lld(%d%%) %s\n",
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
					c_printf("%s ITEM %s(%d,%s) : %s(%d,%s%s) %lld(%d%%) : %s\n",
							showtime(), name ? name : "(---)", lev, acc_name,
							p_ptr->name, p_ptr->lev, p_ptr->accountname,
							p_ptr->total_winner ? ",W" : (p_ptr->once_winner ? ",O" : ""),
							object_value_real(0, o_ptr), o_ptr->discount, o_name);
 #endif

					/* Highlander Tournament: Don't allow transactions before it begins */
					if (!p_ptr->max_exp) gain_exp(Ind, 1);
				}
#endif	// CHEEZELOG_LEVEL

				can_use(Ind, o_ptr);

				/* the_sandman: attempt to id a newly picked up item if we have the means to do so.
 				 * Check that we don't know the item and can read a scroll - mikaelh */
				if (!object_aware_p(Ind, o_ptr) || !object_known_p(Ind, o_ptr)) { /* was just object_known_p */
					int index;
					for (index = 0; index < INVEN_PACK; index++) {
						/* Check if the player does want this feature (!X - for now :) ) */
						if (!check_guard_inscription(p_ptr->inventory[index].note, 'X')) continue;

						object_type *i_ptr = &(p_ptr->inventory[index]);

						/* ID rod && ID staff (no *perc*) */
						if (can_use(Ind, i_ptr) &&
						    ((i_ptr->tval == TV_ROD && i_ptr->sval == SV_ROD_IDENTIFY && !i_ptr->pval) || 
						    (i_ptr->tval == TV_STAFF && i_ptr->sval == SV_STAFF_IDENTIFY && i_ptr->pval > 0))) {
							if (p_ptr->antimagic || get_skill(p_ptr, SKILL_ANTIMAGIC)) {
								msg_format(Ind, "\377%cYour antimagic prevents you from using your magic device.", COLOUR_AM_OWN);
								continue; // find the scrolls =/
							}

							/* Extract the item level */
							int lev = k_info[o_ptr->k_idx].level;

							/* Base chance of success */
							int chance = p_ptr->skill_dev;

							/* Confusion hurts skill */
							if (p_ptr->confused) chance = chance / 2;

							/* perc rod only: is it easy to use? */
							if (i_ptr->tval == TV_ROD && (i_ptr->name2 == EGO_RSIMPLICITY /* of Simplicity */)) chance *= 2;

							/* High level objects are harder */
							chance = chance - ((lev > 50) ? 50 : lev) - (p_ptr->antimagic * 2);
							if (chance < 0) chance = 0;

							/* Give everyone a (slight) chance */
							if ((chance < USE_DEVICE) && (rand_int(USE_DEVICE - chance + 1) == 0)) chance = USE_DEVICE;

							/* Roll for usage */
							if ((chance < USE_DEVICE) || (randint(chance) < USE_DEVICE))
							{
								if (i_ptr->tval == TV_ROD)
									msg_print(Ind, "You failed to use the rod properly.");
								else
									msg_print(Ind, "You failed to use the staff properly.");
								break;
							}

							//We managed to pull it off :-)
							/* we id the newly picked up item */
							object_aware(Ind, o_ptr);
							object_known(o_ptr);

							p_ptr->notice |= (PN_COMBINE | PN_REORDER);
							object_tried(Ind, i_ptr);
							if (!object_aware_p(Ind, i_ptr)) object_aware(Ind, i_ptr);
							p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

							if (i_ptr->tval == TV_ROD) {
								//Formula is taken from cmd6.c
								i_ptr->pval = 10 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 5);
								if (i_ptr->name2 == EGO_RISTARI /* of istari */ || i_ptr->name2 == EGO_RCHARGING /* of charging */)
									i_ptr->pval /= 2;
							
								//Unstack
								if (i_ptr->number > 1) {
									/* Make a fake item */
									object_type tmp_obj;
									tmp_obj = *i_ptr;
									tmp_obj.number = 1;

									/* Restore "charge" */
									i_ptr->pval = 0;

									/* Unstack the used item */
									i_ptr->number--;
									p_ptr->total_weight -= tmp_obj.weight;
									inven_carry(Ind, &tmp_obj);

									/* Message */
									msg_print(Ind, "You unstack your rod.");
								}
							} else if (i_ptr->tval == TV_STAFF) {
								//Decrease the charge
								i_ptr->pval--;

								//Unstack
								if (i_ptr->number > 1) {
									/* Make a fake item */
									object_type tmp_obj;
									tmp_obj = *i_ptr;
									tmp_obj.number = 1;

									/* Restore the charges */
									i_ptr->pval++;

									/* Unstack the used item */
									i_ptr->number--;
									p_ptr->total_weight -= tmp_obj.weight;
									int item = inven_carry(Ind, &tmp_obj);

									/* Message */
									msg_print(Ind, "You unstack your staff."); 
									if (item >= 0) inven_item_charges(Ind, item);
									else floor_item_charges(0-item);
								}
							}

							/* consume a turn */
/* taken out for now since carry() in move_player() doesnt need energy. mass-'g'-presses result in frozen char for a while
							p_ptr->energy -= level_speed(&p_ptr->wpos);*/
							break;
						} else
						/* ID scroll */
						if (i_ptr->tval == TV_SCROLL && i_ptr->sval == SV_SCROLL_IDENTIFY && i_ptr->number > 0) { 
					 		if (p_ptr->blind || no_lite(Ind) || p_ptr->confused) {
								//Allow the possible existence of an id staff/rod in bag and contain !X?
								//continue;
								//Screw it. break!
								break;
							}

							/* we id the newly picked up item */
							object_aware(Ind, o_ptr);
							object_known(o_ptr);

							/* Destroy a scroll in the pack */
							if (index >= 0) {
								inven_item_increase(Ind, index, -1);
								inven_item_describe(Ind, index);
								inven_item_optimize(Ind, index);
							}

							/* consume a turn */
/* taken out for now since carry() in move_player() doesnt need energy. mass-'g'-presses result in frozen char for a while
							p_ptr->energy -= level_speed(&p_ptr->wpos);*/
							break;
						}
					} 
				}

				/* Carry the item */
				slot = inven_carry(Ind, o_ptr);

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

				/* Describe the object */
				object_desc(Ind, o_name, o_ptr, TRUE, 3);

				/* felt an (non-changing!) object of same kind before via pseudo-id? then remember.
				   Note: currently all objects for which that is true are 'magic', hence we only
				   use object_value_auxX_MAGIC() below. Note that if we add an item to an already
				   felt item in our inventory, the combined items will have ID_SENSE set of course,
				   so we won't get another "remember" message, just as it makes sense :) - C. Blue */
				if (!object_aware_p(Ind, o_ptr) && !object_known_p(Ind, o_ptr)
				    && !(o_ptr->ident & ID_SENSE) && object_felt_p(Ind, o_ptr)) {
					/* We have "felt" this kind of object already before */
				        o_ptr->ident |= (ID_SENSE);
					if (!object_felt_heavy_p(Ind, o_ptr)) {
						/* at least give a notice */
						msg_format(Ind, "You remember %s (%c) in your pack %s %s.",
						    o_name, index_to_label(slot), ((o_ptr->number != 1) ? "were" : "was"), value_check_aux2_magic(o_ptr));
						/* otherwise inscribe it textually */
						if (!o_ptr->note) o_ptr->note = quark_add(value_check_aux2_magic(o_ptr));
					} else {
						/* at least give a notice */
						msg_format(Ind, "You remember %s (%c) in your pack %s %s.",
						    o_name, index_to_label(slot), ((o_ptr->number != 1) ? "were" : "was"), value_check_aux1_magic(o_ptr));
						/* otherwise inscribe it textually */
						if (!o_ptr->note) o_ptr->note = quark_add(value_check_aux1_magic(o_ptr));
					}
#if 0					
					/* Combine / Reorder the pack (later) */
			                p_ptr->notice |= (PN_COMBINE | PN_REORDER);
			                /* Window stuff */
			                p_ptr->window |= (PW_INVEN | PW_EQUIP);
#endif
				} else {
					/* Just standard message */
					msg_format(Ind, "You have %s (%c).", o_name, index_to_label(slot));
				}

				/* guild key? */
				if(o_ptr->tval == TV_KEY && o_ptr->sval == 2){
					if(o_ptr->pval == p_ptr->guild){
						if(guilds[p_ptr->guild].master != p_ptr->id){
							guild_msg_format(p_ptr->guild, "\374\377%c%s is the new guildmaster!", COLOUR_CHAT_GUILD, p_ptr->name);
							guilds[p_ptr->guild].master = p_ptr->id;
							Send_guild(Ind, FALSE, FALSE);
						}
					}
				}

				/* Delete original */
//				delete_object(wpos, p_ptr->py, p_ptr->px);
				delete_object_idx(c_ptr->o_idx, FALSE);

				/* Hack -- tell the player of the next object on the pile */
				whats_under_your_feet(Ind);

				/* Tell the client */
				Send_floor(Ind, 0);
			}
		}
	}

#ifdef USE_SOUND_2010
	sound_item(Ind, o_ptr->tval, o_ptr->sval, "pickup_");
#endif

	/* splash! harm equipments */
	if (c_ptr->feat == FEAT_DEEP_WATER &&
	    TOOL_EQUIPPED(p_ptr) != SV_TOOL_TARPAULIN &&
//			magik(WATER_ITEM_DAMAGE_CHANCE))
	    magik(3) && !p_ptr->fly && !p_ptr->immune_water && !(p_ptr->resist_water && magik(50)))
	{
		if (!magik(get_skill_scale(p_ptr, SKILL_SWIM, 4900)))
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

	/* Get the item (must be in the pack) */
	if (item < 0) return;
	p_ptr->current_force_stack = item + 1;
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);
}

#if 0
/*
 * Determine if a trap affects the player.
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match trap power against player armor.
 */
static int check_hit(int Ind, int power)
{
	player_type *p_ptr = Players[Ind];

	int k, ac;

	/* Percentile dice */
	k = rand_int(100);

	/* Hack -- 5% hit, 5% miss */
	if (k < 10) return (k < 5);

	/* Paranoia -- No power */
	if (power <= 0) return (FALSE);

	/* Total armor */
	ac = p_ptr->ac + p_ptr->to_a;

	/* Power competes against Armor */
	if (randint(power) > ((ac * 3) / 4)) return (TRUE);

	/* Assume miss */
	return (FALSE);
}
#endif // if 0

/*
 * Handle player hitting a real trap
 */
void hit_trap(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int t_idx;
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	struct c_special *cs_ptr;
	cave_type               *c_ptr;
	bool ident=FALSE;

	/* Ghosts ignore traps */
	if ((p_ptr->ghost) || (p_ptr->tim_traps)) return;

	/* Disturb the player */
	disturb(Ind, 0, 0);

	/* Get the cave grid */
	if(!(zcave=getcave(wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	if(!(cs_ptr=GetCS(c_ptr, CS_TRAPS))) return;
	t_idx = cs_ptr->sc.trap.t_idx;

	if (t_idx) {
		ident = player_activate_trap_type(Ind, p_ptr->py, p_ptr->px, NULL, -1);
		if (ident && !p_ptr->trap_ident[t_idx]) {
			p_ptr->trap_ident[t_idx] = TRUE;
			msg_format(Ind, "You identified the trap as %s.",
				   t_name + t_info[t_idx].name);
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
static void py_attack_player(int Ind, int y, int x, bool old)
{
	player_type *p_ptr = Players[Ind];
	int num = 0, bonus, chance, slot;
	int k, k2, bs_multiplier;
	long int kl;
	player_type *q_ptr;
	object_type *o_ptr = NULL;
	char q_name[80], brand_msg[80] = { '\0' }, hit_desc[80];
	bool do_quake = FALSE;
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	cave_type *c_ptr;
	int fear_chance;
	bool pierced;

	bool		stab_skill = (get_skill(p_ptr, SKILL_BACKSTAB) != 0);
	bool		sleep_stab = TRUE, cloaked_stab = (p_ptr->cloaked == 1), shadow_stab = (p_ptr->shadow_running); /* can player backstab the monster? */
	bool		backstab = FALSE, stab_fleeing = FALSE; /* does player backstab the player? */
	bool		primary_wield = (p_ptr->inventory[INVEN_WIELD].k_idx != 0);
	bool		secondary_wield = (p_ptr->inventory[INVEN_ARM].k_idx != 0 && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD);
	bool		dual_wield = primary_wield && secondary_wield && p_ptr->dual_mode; /* Note: primary_wield && secondary_wield == p_ptr->dual_wield (from xtra1.c) actually. */
	int		dual_stab = (dual_wield ? 1 : 0); /* organizer variable for dual-wield backstab */
	bool		martial = FALSE;

	int		vorpal_cut = 0;
	int		chaos_effect = 0;
	int		vampiric_melee;
	bool		drain_msg = TRUE;
	int		drain_result = 0, drain_heal = 0, drain_frac;
	int		drain_left = MAX_VAMPIRIC_DRAIN;
	bool		drainable = TRUE;
//	bool		py_slept;
	bool		no_pk;

	monster_race *pr_ptr = &r_info[p_ptr->body_monster];
	bool apply_monster_effects = TRUE;
	int i, monster_effects, sfx = 0;
	u32b monster_effect[6], monster_effect_chosen;
	monster_effect[1] = 0;
	monster_effect[2] = 0;
	monster_effect[3] = 0;
	monster_effect[4] = 0;
	monster_effect[5] = 0;
	u32b f1 = 0, f2 = 0, f3 = 0, f4 = 0, f5 = 0, esp = 0;

	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];
	q_ptr = Players[0 - c_ptr->m_idx];
//	py_slept = (q_ptr->sleep != 0);
//	py_slept = q_ptr->afk; /* :D - unused though (also, AFK status can't be toggled for this anyway */
	no_pk = ((zcave[p_ptr->py][p_ptr->px].info & CAVE_NOPK) ||
	   (zcave[q_ptr->py][q_ptr->px].info & CAVE_NOPK));


#if 0
        /* May not assault AFK players, sportsmanship ;)
	   Also required for ranged attacks though, hence currently if0'ed - C. Blue */
	if (q_ptr->afk) {
		if (!p_ptr->afk) {
			/* Message, reduce spamming frequency */
//			if (!old)
			if (!(turn % (cfg.fps * 3))) msg_print(Ind, "For sportsmanship you may not assault players who are AFK.");
		}
		return;
	}
#endif

	/* Restrict attacking in WRAITHFORM */
	if (p_ptr->tim_wraith && !q_ptr->tim_wraith) return;

	/* Disturb both players */
	disturb(Ind, 0, 0);
	disturb(0 - c_ptr->m_idx, 0, 0);

	/* Extract name */
//	strcpy(q_name, q_ptr->name);
	player_desc(Ind, q_name, 0 - c_ptr->m_idx, 0);

	/* Track player health */
	if (p_ptr->play_vis[0 - c_ptr->m_idx]) health_track(Ind, c_ptr->m_idx);

	/* If target isn't already hostile toward attacker, make it so */
	if (!check_hostile(0 - c_ptr->m_idx, Ind)) {
		/* Make hostile */
		if (Players[0 - c_ptr->m_idx]->pvpexception < 2)
		add_hostility(0 - c_ptr->m_idx, p_ptr->name, FALSE);

		/* Log it if no blood bond - mikaelh */
		if (!player_list_find(p_ptr->blood_bond, Players[0 - c_ptr->m_idx]->id)) {
			s_printf("%s attacked %s.\n", p_ptr->name, Players[0 - c_ptr->m_idx]->name);
		}
	}
	/* Hack -- divided turn for auto-retaliator */
	if (!old) {
		p_ptr->energy -= level_speed(&p_ptr->wpos) / p_ptr->num_blow;
	}

	/* Handle attacker fear */
	if (p_ptr->afraid) {
		/* Message */
		msg_format(Ind, "You are too afraid to attack %s!", q_name);

		/* Done */
		return;
	}

	if (q_ptr->store_num == STORE_HOME) {
		/* Message */
//		msg_format(Ind, "You are too afraid to attack %s!", q_name);

		/* Done */
		return;
	}

	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
	/* Disturb the player */
//	q_ptr->sleep = 0;

	if (cfg.use_pk_rules == PK_RULES_DECLARE) {
		if (!(q_ptr->pkill & PKILL_KILLABLE)) {
			char string[30];
			snprintf(string, 30, "attacking %s", q_ptr->name);
			s_printf("%s attacked defenceless %s\n", p_ptr->name, q_ptr->name);
			if (!imprison(Ind, 500, string)) {
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

	k = drain_left / p_ptr->num_blow;
	/* ..and make up for rounding errors :) */
	drain_left = k + (magik(((drain_left - (k * p_ptr->num_blow)) * 100) / p_ptr->num_blow) ? 1 : 0);

	/* Handle player fear */
	if (p_ptr->afraid) {
		msg_format(Ind, "You are too afraid to attack %s!", q_name);
		suppress_message = FALSE;
		/* Done */
		return;
	}

	/* Cannot 'stab' with martial-arts */
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS)
	    && !p_ptr->inventory[INVEN_WIELD].k_idx
	    && !p_ptr->inventory[INVEN_ARM].k_idx
	    && !p_ptr->inventory[INVEN_BOW].k_idx)
		martial = TRUE;

#if 0
	/* check whether player can be backstabbed */
//	if (!q_ptr->sleep)
	if (!q_ptr->afk) /* :D */
		sleep_stab = FALSE;
	if (q_ptr->backstabbed) {
		cloaked_stab = FALSE; /* a monster can only be backstabbed once, except if it gets resleeped or stabbed while fleeing */
		shadow_stab = FALSE; /* assume monster doesn't fall twice for it */
	}
	if (!sleep_stab && !cloaked_stab && !shadow_stab) dual_stab = 0;
#else
	sleep_stab = FALSE;
#endif

	/* Re-check piercing */
	if (p_ptr->piercing_charged) {
		if (p_ptr->cst < 9) {
			msg_print(Ind, "Not enough stamina to execute an assassinating attack.");
			p_ptr->piercing_charged = FALSE;
			p_ptr->piercing = 0;
		} else {
			p_ptr->cst -= 9;
		}
	}

	/* Attack once for each legal blow */
	while (num++ < p_ptr->num_blow) {
#ifdef USE_SOUND_2010
		if (p_ptr->cut_sfx_attack) {
			sfx = extract_energy[p_ptr->pspeed] * p_ptr->num_blow;
			if (sfx) {
				p_ptr->count_cut_sfx_attack += 10000 / sfx;
				if (p_ptr->count_cut_sfx_attack >= 250) { /* 100 / 25 = 4 blows per turn */
					p_ptr->count_cut_sfx_attack -= 250;
					if (p_ptr->count_cut_sfx_attack >= 250) p_ptr->count_cut_sfx_attack = 0;
					sfx = 0;
				}
			}
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
			case 3: dual_stab = 4; break; /* becomes 0 at end of attack, disabling further dual-stabs */
			}
		}

		o_ptr = &p_ptr->inventory[slot];

		/* Manage backstabbing and 'flee-stabbing' */
		if (stab_skill && (o_ptr->tval == TV_SWORD)) { /* Need TV_SWORD type weapon to backstab */
			if (sleep_stab || cloaked_stab || shadow_stab) { /* Note: Cloaked backstab takes precedence over backstabbing a fleeing monster */
				backstab = TRUE;
//				q_ptr->backstabbed = 1;
			} else if (q_ptr->afraid) {
				stab_fleeing = TRUE;
			}
		}

		f1 = f2 = f3 = f4 = f5 = esp = 0x0;
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
		chaos_effect = 0; // we need this methinks..?

		if ((f4 & TR4_NEVER_BLOW)) {
			msg_print(Ind, "You can't attack with that weapon.");
			break;
		}

		/* check whether this weapon or we in general, are vampiric */
		if ((f1 & TR1_VAMPIRIC)) vampiric_melee = 100; /* weapon chance is always 100% */
		else vampiric_melee = p_ptr->vampiric_melee; /* non-weapon chance from other items is applied from xtra1.c */
#ifdef TEST_SERVER
		msg_format(Ind, "slot %d vamp %d", slot, vampiric_melee);
#endif

		/* Calculate the "attack quality" */
		bonus = p_ptr->to_h + o_ptr->to_h + p_ptr->to_h_melee;
		chance = (p_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));

		/* Test for hit */
		pierced = FALSE;
#ifndef PVP_AC_REDUCTION
		if (p_ptr->piercing || backstab || test_hit_melee(chance, q_ptr->ac + q_ptr->to_a, 1)) {
#else
//		if (p_ptr->piercing || backstab || test_hit_melee(chance, ((q_ptr->ac + q_ptr->to_a) * 2) / 3, 1)) {
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
			int dodge_chance = q_ptr->dodge_level - p_ptr->lev * 19 / 10;
			if (dodge_chance > DODGE_MAX_CHANCE) dodge_chance = DODGE_MAX_CHANCE;
			if (!backstab && (dodge_chance > 0) && magik(dodge_chance)) {
				msg_format(Ind, "\377c%s dodges your attack!", COLOUR_DODGE_PLY, q_name);
				msg_format(0 - c_ptr->m_idx, "\377%cYou dodge %s's attack!", COLOUR_DODGE_GOOD, p_ptr->name); 
  #ifdef USE_SOUND_2010
				if (sfx == 0) {
					if (o_ptr->k_idx && is_weapon(o_ptr->tval)
						sound(Ind, "miss_weapon", "miss", SFX_TYPE_ATTACK, FALSE);
					else
						sound(Ind, "miss", NULL, SFX_TYPE_ATTACK, FALSE);
				}
  #endif
				continue;
			}
 #endif
#else /* :-o */
			if (!backstab && magik(apply_dodge_chance(0 - c_ptr->m_idx, p_ptr->lev))) {
				msg_format(Ind, "\377%c%s dodges your attack!", COLOUR_DODGE_PLY, q_name);
				msg_format(0 - c_ptr->m_idx, "\377%cYou dodge %s's attack!", COLOUR_DODGE_GOOD, p_ptr->name);
 #ifdef USE_SOUND_2010
				if (sfx == 0) {
					if (o_ptr->k_idx && is_weapon(o_ptr->tval))
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
					msg_format(0 - c_ptr->m_idx, "\377%cYou block %s's attack!", COLOUR_BLOCK_GOOD, p_ptr->name);
 #ifdef USE_SOUND_2010
					if (sfx == 0) {
						sound(Ind, "block_shield", NULL, SFX_TYPE_ATTACK, FALSE);
					}
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
					msg_format(0 - c_ptr->m_idx, "\377%cYou parry %s's attack!", COLOUR_PARRY_GOOD, p_ptr->name);
 #ifdef USE_SOUND_2010
					if (sfx == 0) {
						sound(Ind, "parry_weapon", "parry", SFX_TYPE_ATTACK, FALSE);
					}
 #endif
					continue;
				}
			}
#endif

#ifdef USE_SOUND_2010
			if (o_ptr->k_idx && (is_weapon(o_ptr->tval) || o_ptr->tval == TV_MSTAFF))
				switch(o_ptr->tval) {
				case TV_SWORD: sound(Ind, "hit_sword", "hit", SFX_TYPE_ATTACK, FALSE); break;
				case TV_BLUNT: if (o_ptr->sval == SV_WHIP) sound(Ind, "hit_whip", "hit", SFX_TYPE_ATTACK, FALSE);
						else sound(Ind, "hit_blunt", "hit", SFX_TYPE_ATTACK, FALSE); break;
				case TV_AXE: sound(Ind, "hit_axe", "hit", SFX_TYPE_ATTACK, FALSE); break;
				case TV_POLEARM: sound(Ind, "hit_polearm", "hit", SFX_TYPE_ATTACK, FALSE); break;
				case TV_MSTAFF: sound(Ind, "hit_blunt", "hit", SFX_TYPE_ATTACK, FALSE); break;
				}
			else
				if (sfx == 0) {
					sound(Ind, "hit", NULL, SFX_TYPE_ATTACK, FALSE);
				}
#else
			sound(Ind, SOUND_HIT);
#endif
			sprintf(hit_desc, "You hit %s", q_name);

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
//						msg_format(Ind, ma_ptr->desc, q_name);
				}

				else {
					if (ma_ptr->effect) {
						stun_effect = (ma_ptr->effect/2) + randint(ma_ptr->effect/2);
					}

					sprintf(hit_desc, ma_ptr->desc, q_name);
//					msg_format(Ind, ma_ptr->desc, q_name);
				}

				k = tot_dam_aux_player(Ind, o_ptr, k, q_ptr, brand_msg, FALSE);
				k = critical_melee(Ind, marts * (randint(10)), ma_ptr->min_level, k, FALSE, 0);

				/* Apply the player damage boni */
				k += p_ptr->to_d + p_ptr->to_d_melee;

				if ((special_effect == MA_KNEE) && ((k + p_ptr->to_d + p_ptr->to_d_melee) < q_ptr->chp)) {
					msg_format(Ind, "%^s moans in agony!", q_name);
					stun_effect = 3 + randint(3);
					resist_stun /= 3;
				}

				if (stun_effect && ((k + p_ptr->to_d + p_ptr->to_d_melee) < q_ptr->chp)) {
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

				if (f5 & TR5_VORPAL && (randint(6) == 1)) {
//					k = o_ptr->dd * o_ptr->ds; /* full weapon efficiency */
//					if (magik(25)) k *= 3; else k *= 2; /* multi dice! */
					vorpal_cut = k; /* save dice */
				} else vorpal_cut = FALSE;

				/* weapons that inflict little damage, especially of only 1 damage dice,
				   mostly don't cause earthquakes at all */
				if ((p_ptr->impact || (f5 & TR5_IMPACT)) &&
				    500 / (10 + (k - o_ptr->dd) * o_ptr->dd * o_ptr->ds / (o_ptr->dd * (o_ptr->ds - 1) + 1)) < randint(35)) do_quake = TRUE;
				    /* I made the new formula above, to get a better chance curve over all the different weapon types. -C. Blue- */
				    /* Some old tries: */
//				    130 - ((k - o_ptr->dd) * o_ptr->dd * o_ptr->ds / (o_ptr->dd * (o_ptr->ds - 1) + 1)) < randint(130)) do_quake = TRUE;
//				    (150 / (10 + k - o_ptr->dd) < 11 - (2 / o_ptr->dd))) do_quake = TRUE;
//				    (150 / (1 + k - o_ptr->dd) < 23 - (2 / o_ptr->dd))) do_quake = TRUE;

				k = tot_dam_aux_player(Ind, o_ptr, k, q_ptr, brand_msg, FALSE);

#ifdef ENABLE_STANCES
				/* apply stun from offensive combat stance */
				if (p_ptr->combat_stance == 2) {
					int stun_effect, resist_stun;

					stun_effect = randint(get_skill_scale(p_ptr, SKILL_MASTERY, 10) + adj_con_fix[p_ptr->stat_cur[A_STR]] / 2) + 1;
					stun_effect /= 2;

					resist_stun = adj_con_fix[q_ptr->stat_cur[A_CON]]; /* 0..9 */
					if (q_ptr->free_act) resist_stun += 3;
					resist_stun += 6 - 300 / (50 + q_ptr->ac + q_ptr->to_a); /* 0..5 */
					resist_stun -= adj_con_fix[p_ptr->stat_cur[A_DEX]];
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

				k += o_ptr->to_d;

				/* Apply the player damage boni */
				k += p_ptr->to_d + p_ptr->to_d_melee;

				/* Critical strike moved here, since it works best
				with light weapons, which have low dice. So for gain
				we need the full damage including all to-dam boni */
				k2 = k; /* remember damage before crit */
#ifdef CRIT_VS_BACKSTAB
				if (!backstab && !stab_fleeing)
#endif
				k = critical_melee(Ind, o_ptr->weight, o_ptr->to_h + p_ptr->to_h_melee, k, ((o_ptr->tval == TV_SWORD) && (o_ptr->weight <= 100) && !p_ptr->rogue_heavyarmor), calc_crit_obj(Ind, o_ptr));

			/* handle bare fists/bat/ghost */
			} else {
				k = tot_dam_aux_player(Ind, o_ptr, k, q_ptr, brand_msg, FALSE);

				/* Apply the player damage boni */
				/* (should this also cancelled by nazgul?(for now not)) */
				k += p_ptr->to_d + p_ptr->to_d_melee;

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
				bs_multiplier = get_skill_scale(p_ptr, SKILL_BACKSTAB, 350 + rand_int(101));
				kl = k * (100 + bs_multiplier);
				kl /= (dual_stab ? 150 : 100);
				kl += q_ptr->chp / (dual_stab ? 30 : 20);
				k = kl;
#ifdef CRIT_VS_VORPAL
				kl = k2 * (100 + bs_multiplier);
				kl /= (dual_stab ? 150 : 100);
				kl += q_ptr->chp / (dual_stab ? 30 : 20);
				k2 = kl;
#endif
			}

			/* Vorpal bonus - multi-dice!
			   (currently +37.5% more branded dice damage and o_ptr->to_d on total average, just for the records) */
                        if (vorpal_cut) {
#ifdef CRIT_VS_VORPAL
				k2 += (magik(25) ? 2 : 1) * (o_ptr->to_d + tot_dam_aux_player(Ind, o_ptr, vorpal_cut, q_ptr, brand_msg, FALSE)); /* exempts critical strike */
				/* either critical hit or vorpal, not both */
				if (k2 > k) k = k2;
#else
				k += (magik(25) ? 2 : 1) * (o_ptr->to_d + tot_dam_aux_player(Ind, o_ptr, vorpal_cut, q_ptr, brand_msg, FALSE)); /* exempts critical strike */
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

			/* Cannot kill on this grid? */
			if (no_pk) {
				if (k > q_ptr->chp) k -= q_ptr->chp;
				if (k2 > q_ptr->chp) k2 -= q_ptr->chp;
			}

			/* Messages */
			if (backstab) {
				backstab = FALSE;
			   if (martial) {
				msg_format(Ind, "You twist the neck of %s for \377L%d \377wdamage.", q_name, k);
				msg_format(0 - c_ptr->m_idx, "%s twists your neck for \377R%d \377wdamage.", p_ptr->name, k);
			   } else {
				msg_format(Ind, "You stab helpless %s for \377L%d \377wdamage.", q_name, k);
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
//			else if (!martial) msg_format(Ind, "You hit %s for \377g%d \377wdamage.", m_name, k);
			else {
				msg_format(Ind, "%s for \377y%d \377wdamage.", hit_desc, k);
				msg_format(0 - c_ptr->m_idx, "%s hits you for \377R%d \377wdamage.", p_ptr->name, k);
			}
//less spam for now - C. Blue   if (strlen(brand_msg) > 0) msg_print(Ind, brand_msg);

			if (cfg.use_pk_rules == PK_RULES_NEVER && q_ptr->chp < 5){
				msg_format(Ind, "\374You have beaten %s", q_ptr->name);
				msg_format(0 - c_ptr->m_idx, "\374%s has beaten you up!", p_ptr->name);
				teleport_player(0 - c_ptr->m_idx, 400, TRUE);
			}

			take_hit(0 - c_ptr->m_idx, k, p_ptr->name, Ind);

			if (!c_ptr->m_idx) break;

			/* Check for death */
			if (q_ptr->death) break;

			py_touch_zap_player(Ind, -c_ptr->m_idx);

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

						hp_player_quiet(Ind, drain_heal, TRUE);
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
					if (pr_ptr->blow[i].d_dice * pr_ptr->blow[i].d_side) {
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
						/* Take "poison" effect */
						if (q_ptr->resist_pois || q_ptr->oppose_pois || q_ptr->immune_poison) {
							msg_format(Ind, "%^s is unaffected.", q_name);
						} else if (rand_int(100) < q_ptr->skill_sav) {
							msg_format(Ind, "%^s resists the disease.", q_name);
						} else {
							msg_format(Ind, "%^s suffers from disease.", q_name);
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
								set_stun(0 - c_ptr->m_idx, q_ptr->stun + 20 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 10)));
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
				p_ptr->confusing = FALSE;

				/* Message */
				msg_print(Ind, "Your hands stop glowing.");

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
			if (p_ptr->ghost) {
				fear_chance = 50 + (p_ptr->lev - q_ptr->lev) * 5;

				if (rand_int(100) < fear_chance) {
					msg_format(Ind, "%^s appears afraid.", q_name);
					set_afraid(0 - c_ptr->m_idx, q_ptr->afraid + 4 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 10)));
				} else {
					msg_format(Ind, "%^s resists the effect.", q_name);
				}
			}

			/* Fruit bats get life stealing.
			   Note: This is ok because fruit bats cannot wear weapons/gloves as a
			   source of vampirism.
			   Still to check: Vampire fruit bats! CURRENTLY STACKS! */
			if (p_ptr->fruit_bat == 1 && !p_ptr->body_monster) {
				int leech = q_ptr->chp;
				if (k < leech) leech = k;
				leech /= 10;
				hp_player_quiet(Ind, rand_int(leech), TRUE);
			}
		}

		/* Player misses */
		else {
			backstab = stab_fleeing = FALSE;

#ifdef USE_SOUND_2010
			if (sfx == 0) {
				if (o_ptr->k_idx && is_weapon(o_ptr->tval))
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
		if (!dual_stab) sleep_stab = cloaked_stab = shadow_stab = FALSE;
		if (dual_stab) {
			num--;
			continue;
		}

		/* Hack -- divided turn for auto-retaliator */
		if (!old) break;
	}

	/* Mega-Hack -- apply earthquake brand */
	if (do_quake) {
		if (o_ptr->k_idx
#ifdef ALLOW_NO_QUAKE_INSCRIPTION
		    && !check_guard_inscription(o_ptr->note, 'E')
#else
		    && (!check_guard_inscription(o_ptr->note, 'E') ||
		    o_ptr->name1 != ART_GROND)
#endif
		    ) {
			/* Giga-Hack -- equalize the chance (though not likely..) */
			if (old || randint(p_ptr->num_blow) < 3)
				earthquake(&p_ptr->wpos, p_ptr->py, p_ptr->px, 10);
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
//#define CRIT_VS_VORPAL
#define CRIT_VS_BACKSTAB
static void py_attack_mon(int Ind, int y, int x, bool old)
{
	player_type	*p_ptr = Players[Ind];
	int		num = 0, bonus, chance, slot, owner_Ind = 0, sfx = 0;
	int		k, k2, bs_multiplier;
	long int	kl;
	object_type	*o_ptr = NULL;
	bool		do_quake = FALSE;

	char		m_name[MNAME_LEN], brand_msg[80] = { '\0' }, hit_desc[80], mbname[MNAME_LEN];
	monster_type	*m_ptr;
	monster_race	*r_ptr;

	bool		fear = FALSE;
	int 		fear_chance;

	bool		stab_skill = (get_skill(p_ptr, SKILL_BACKSTAB) != 0);
	bool		sleep_stab = TRUE, cloaked_stab = (p_ptr->cloaked == 1), shadow_stab = (p_ptr->shadow_running); /* can player backstab the monster? */
	bool		backstab = FALSE, stab_fleeing = FALSE; /* does player backstab the monster? */
	bool		primary_wield = (p_ptr->inventory[INVEN_WIELD].k_idx != 0);
	bool		secondary_wield = (p_ptr->inventory[INVEN_ARM].k_idx != 0 && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD);
	bool		dual_wield = primary_wield && secondary_wield && p_ptr->dual_mode; /* Note: primary_wield && secondary_wield == p_ptr->dual_wield (from xtra1.c) actually. */
	int		dual_stab = (dual_wield ? 1 : 0); /* organizer variable for dual-wield backstab */
	bool		nolite, nolite2, martial = FALSE;
	int		block, parry;

	int		vorpal_cut = 0;
	int		chaos_effect = 0;
	int		vampiric_melee;
	bool		drain_msg = TRUE;
	int		drain_result = 0, drain_heal = 0;
	int		drain_left = MAX_VAMPIRIC_DRAIN;
	bool		drainable = TRUE;
	bool		mon_slept;


	struct worldpos	*wpos = &p_ptr->wpos;
	cave_type	**zcave;
	cave_type 	*c_ptr;

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

	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

	m_ptr = &m_list[c_ptr->m_idx];
	r_ptr = race_inf(m_ptr);
	mon_slept = (m_ptr->csleep != 0);

	if ((r_ptr->flags3 & RF3_UNDEAD) ||
//	    (r_ptr->flags3 & RF3_DEMON) ||
	    (r_ptr->flags3 & RF3_NONLIVING) ||
	    (strchr("Egv", r_ptr->d_char)))
		drainable = FALSE;

	nolite = !((c_ptr->info & (CAVE_LITE | CAVE_GLOW)) ||
		(r_ptr->flags4 & RF4_BR_DARK) ||
		(r_ptr->flags6 & RF6_DARKNESS));
	nolite2 = nolite && !(r_ptr->flags9 & RF9_HAS_LITE);

	/* Disturb the player */
	disturb(Ind, 0, 0);

	/* Extract monster name (or "it") */
	monster_desc(Ind, m_name, c_ptr->m_idx, 0);
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
//	    !owner_Ind || /* don't attack ownerless golems */
	    (owner_Ind && !check_hostile(Ind, owner_Ind))) /* only attack if owner is hostile */
#endif
	{
		int ox = m_ptr->fx, oy = m_ptr->fy, nx = p_ptr->px, ny = p_ptr->py;

		msg_format(Ind, "You swap positions with %s.", m_name);

		/* Update the new location */
		zcave[ny][nx].m_idx=c_ptr->m_idx;
		/* Update the old location */
		zcave[oy][ox].m_idx=-Ind;

		/* Move the monster */
		m_ptr->fy = ny;
		m_ptr->fx = nx;
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
		msg_format(Ind, "You are too afraid to attack %s!", m_name);
		suppress_message = FALSE;
		/* Done */
		return;
	}

	/* Cannot 'stab' with martial-arts */
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS)
	    && !p_ptr->inventory[INVEN_WIELD].k_idx
	    && !p_ptr->inventory[INVEN_ARM].k_idx
	    && !p_ptr->inventory[INVEN_BOW].k_idx)
		martial = TRUE;

	/* check whether monster can be backstabbed */
	if (!m_ptr->csleep /*&& m_ptr->ml*/) sleep_stab = FALSE;
	if (m_ptr->backstabbed) {
		cloaked_stab = FALSE; /* a monster can only be backstabbed once, except if it gets resleeped or stabbed while fleeing */
		shadow_stab = FALSE; /* assume monster doesn't fall twice for it */
	}
	if (!sleep_stab && !cloaked_stab && !shadow_stab) dual_stab = 0;

	/* cloaking mode stuff */
	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
	/* Disturb the monster */
	m_ptr->csleep = 0;

	/* Re-check piercing */
	if (p_ptr->piercing_charged) {
		if (p_ptr->cst < 9) {
			msg_print(Ind, "Not enough stamina to execute an assassinating attack.");
			p_ptr->piercing_charged = FALSE;
			p_ptr->piercing = 0;
		} else {
			p_ptr->cst -= 9;
			p_ptr->redraw |= PR_STAMINA;
		}
	}

	/* Attack once for each legal blow */
	while (num++ < p_ptr->num_blow) {
#ifdef USE_SOUND_2010
		if (p_ptr->cut_sfx_attack) {
			sfx = extract_energy[p_ptr->pspeed] * p_ptr->num_blow;
			if (sfx) {
				p_ptr->count_cut_sfx_attack += 10000 / sfx;
				if (p_ptr->count_cut_sfx_attack >= 250) { /* 100 / 25 = 4 blows per turn */
					p_ptr->count_cut_sfx_attack -= 250;
					if (p_ptr->count_cut_sfx_attack >= 250) p_ptr->count_cut_sfx_attack = 0;
					sfx = 0;
				}
			}
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
			case 3: dual_stab = 4; break; /* becomes 0 at end of attack, disabling further dual-stabs */
			}
		}

		o_ptr = &p_ptr->inventory[slot];

		/* Manage backstabbing and 'flee-stabbing' */
		if (stab_skill && (o_ptr->tval == TV_SWORD)) { /* Need TV_SWORD type weapon to backstab */
			if (sleep_stab || cloaked_stab || shadow_stab) { /* Note: Cloaked backstab takes precedence over backstabbing a fleeing monster */
				backstab = TRUE;
				m_ptr->backstabbed = 1;
			} else if (m_ptr->monfear /*&& m_ptr->ml)*/) {
				stab_fleeing = TRUE;
			}
		}

		u32b f1 = 0, f2 = 0, f3 = 0, f4 = 0, f5 = 0, esp = 0;
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
		chaos_effect = 0; // we need this methinks..?

		if ((f4 & TR4_NEVER_BLOW)) {
			msg_print(Ind, "You can't attack with that weapon.");
			break;
		}

		/* check whether this weapon or we in general, are vampiric */
		if (f1 & TR1_VAMPIRIC) vampiric_melee = 100; /* weapon chance is always 100% */
		else vampiric_melee = p_ptr->vampiric_melee; /* non-weapon chance from other items is applied from xtra1.c */

		/* Calculate the "attack quality" */
		bonus = p_ptr->to_h + o_ptr->to_h + p_ptr->to_h_melee;
		chance = (p_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));

		/* Plan ahead if a missed attack would be a blocked or parried one or just an
		   [hitchance-vs-AC-induced] miss. 'Piercing' requires this to be calculated ahead now. */
		block = parry = 0;
		if (strchr("AhHJkpPty", r_ptr->d_char) && /* leaving out Yeeks (else Serpent Man 'J') */
		    !(r_ptr->flags3 & RF3_ANIMAL)) {
#ifdef USE_BLOCKING
			block = 10;
#endif
#ifdef USE_PARRYING
			parry = 5 + m_ptr->ac / 10;
#endif
		}
		/* Evaluate: 0 = no, other values = yes */
		if (mon_slept || !magik(block)) block = 0;
		if (mon_slept || !magik(parry)) parry = 0;

		/* Test for hit */
		if (instakills(Ind) || backstab ||
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
			if (sfx == 0) {
				if (o_ptr->k_idx && (is_weapon(o_ptr->tval) || o_ptr->tval == TV_MSTAFF))
					switch(o_ptr->tval) {
					case TV_SWORD: sound(Ind, "hit_sword", "hit", SFX_TYPE_ATTACK, FALSE); break;
					case TV_BLUNT: if (o_ptr->sval == SV_WHIP) sound(Ind, "hit_whip", "hit", SFX_TYPE_ATTACK, FALSE);
							else sound(Ind, "hit_blunt", "hit", SFX_TYPE_ATTACK, FALSE); break;
					case TV_AXE: sound(Ind, "hit_axe", "hit", SFX_TYPE_ATTACK, FALSE); break;
					case TV_POLEARM: sound(Ind, "hit_polearm", "hit", SFX_TYPE_ATTACK, FALSE); break;
					case TV_MSTAFF: sound(Ind, "hit_blunt", "hit", SFX_TYPE_ATTACK, FALSE); break;
					}
				else
					sound(Ind, "hit", NULL, SFX_TYPE_ATTACK, FALSE);
			}
#else
			sound(Ind, SOUND_HIT);
#endif
			sprintf(hit_desc, "You hit %s", m_name);

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
					if (r_ptr->flags1 & RF1_MALE) {
						msg_format(Ind, "You hit %s in the groin with your knee!", m_name);
						special_effect = MA_KNEE;
					} else {
						sprintf(hit_desc, ma_ptr->desc, m_name);
//						msg_format(Ind, ma_ptr->desc, m_name);
					}
				}

				else if (ma_ptr->effect == MA_SLOW) {
					if (!((r_ptr->flags1 & RF1_NEVER_MOVE)
					    || strchr("ANUjmeEv$,DdsbBFIJQSXclnw!=?", r_ptr->d_char))) {
						msg_format(Ind, "You kick %s in the ankle.", m_name);
						special_effect = MA_SLOW;
					} else {
						sprintf(hit_desc, ma_ptr->desc, m_name);
//						msg_format(Ind, ma_ptr->desc, m_name);
					}
				} else if (ma_ptr->effect == MA_ROYAL_SLOW) {
					if (!((r_ptr->flags1 & RF1_NEVER_MOVE)
					    || strchr("ANjmeEv$,sbBFIJQSXlw!=?", r_ptr->d_char))) {
						msg_format(Ind, "You strike %s's pressure points.", m_name);
						special_effect = MA_SLOW;
					} else {
						sprintf(hit_desc, ma_ptr->desc, m_name);
//						msg_format(Ind, ma_ptr->desc, m_name);
					}
				} else {
					if (ma_ptr->effect)
						stun_effect = (ma_ptr->effect/2) + randint(ma_ptr->effect/2);
	
					sprintf(hit_desc, ma_ptr->desc, m_name);
//					msg_format(Ind, ma_ptr->desc, m_name);
				}

				k = tot_dam_aux(Ind, o_ptr, k, m_ptr, brand_msg, FALSE);
				k2 = k; /* remember damage before crit */
				k = critical_melee(Ind, marts * (randint(10)), ma_ptr->min_level, k, FALSE, 0);

				if ((special_effect == MA_KNEE) && ((k + p_ptr->to_d + p_ptr->to_d_melee) < m_ptr->hp)) {
					msg_format(Ind, "%^s moans in agony!", m_name);
					stun_effect = 7 + randint(13);
					resist_stun /= 3;
				}

				else if ((special_effect == MA_SLOW) && ((k + p_ptr->to_d + p_ptr->to_d_melee) < m_ptr->hp)) {
					if (!(r_ptr->flags1 & RF1_UNIQUE) &&
					    !((r_ptr->flags2 & RF2_POWERFUL) && (r_ptr->flags8 & RF8_NO_CUT)) &&
					    (randint(marts * 2) > r_ptr->level) &&
					    m_ptr->mspeed > m_ptr->speed - 20) {
						msg_format(Ind, "\377o%^s starts limping slower.", m_name);
						m_ptr->mspeed -= 3 + rand_int(3);
					}
				}

				if (stun_effect && ((k + p_ptr->to_d + p_ptr->to_d_melee) < m_ptr->hp)) {
					/* Stun the monster */
					if (r_ptr->flags3 & RF3_NO_STUN) {
						/* nothing:
						msg_format(Ind, "%^s is unaffected.", m_name);*/
					} else if (marts > randint(r_ptr->level + resist_stun + 10)) {
						m_ptr->stunned += (stun_effect + get_skill_scale(p_ptr, SKILL_COMBAT, 3));

						if (m_ptr->stunned > 100)
							msg_format(Ind, "\377y%^s is knocked out.", m_name);
						else if (m_ptr->stunned > 50)
							msg_format(Ind, "\377y%^s is heavily stunned.", m_name);
						else
							msg_format(Ind, "\377y%^s is stunned.", m_name);
					}
				}

				/* Apply the player damage boni */
				/* (should this also cancelled by nazgul?(for now not)) */
				k += p_ptr->to_d + p_ptr->to_d_melee;

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

				if (f5 & TR5_VORPAL && (randint(6) == 1)) {
//					k = o_ptr->dd * o_ptr->ds; /* full weapon efficiency */
//					if (magik(25)) k *= 3; else k *= 2; /* multi dice! */
					vorpal_cut = k; /* save dice */
				} else vorpal_cut = FALSE;

				/* weapons that inflict little damage, especially of only 1 damage dice,
				   mostly don't cause earthquakes at all */
				if ((p_ptr->impact || (f5 & TR5_IMPACT)) &&
				    500 / (10 + (k - o_ptr->dd) * o_ptr->dd * o_ptr->ds / (o_ptr->dd * (o_ptr->ds - 1) + 1)) < randint(35)) do_quake = TRUE;
				    /* I made the new formula above, to get a better chance curve over all the different weapon types. -C. Blue- */
				    /* Some old tries: */
//				    130 - ((k - o_ptr->dd) * o_ptr->dd * o_ptr->ds / (o_ptr->dd * (o_ptr->ds - 1) + 1)) < randint(130)) do_quake = TRUE;
//				    (150 / (10 + k - o_ptr->dd) < 11 - (2 / o_ptr->dd))) do_quake = TRUE;
//				    (150 / (1 + k - o_ptr->dd) < 23 - (2 / o_ptr->dd))) do_quake = TRUE;
				k = tot_dam_aux(Ind, o_ptr, k, m_ptr, brand_msg, FALSE);

#ifdef ENABLE_STANCES
				/* apply stun from offensive combat stance */
				if (p_ptr->combat_stance == 2) {

					stun_effect = randint(get_skill_scale(p_ptr, SKILL_MASTERY, 10) + adj_con_fix[p_ptr->stat_cur[A_STR]] / 2) + 1;
					stun_effect /= 2;

					resist_stun = 6 - 300 / (50 + m_ptr->ac); /* 0..5 */
					if (r_ptr->flags1 & RF1_UNIQUE) resist_stun += 10;
					if (r_ptr->flags3 & RF3_NO_CONF) resist_stun += 5;
					if (r_ptr->flags3 & RF3_NO_SLEEP) resist_stun += 5;
					if (r_ptr->flags3 & RF3_UNDEAD)	resist_stun += 10;

					switch(p_ptr->combat_stance_power) {
					case 0: resist_stun = (resist_stun * 5) / 4; break;
					case 1: resist_stun = (resist_stun * 4) / 4; break;
					case 2: resist_stun = (resist_stun * 3) / 4; break;
					case 3: resist_stun = (resist_stun * 2) / 3; break;
					}

					if (stun_effect && ((k + p_ptr->to_d + p_ptr->to_d_melee) < m_ptr->hp)) {
						/* Stun the monster */
						if (r_ptr->flags3 & RF3_NO_STUN) {
							/* nothing:
							msg_format(Ind, "%^s is unaffected.", m_name);*/
						}
//						else if (!magik((r_ptr->level * 2) / 3 + resist_stun)) {
//						else if (!magik(((r_ptr->level + 30) * 2) / 4 + resist_stun)) {
//						else if (!magik((r_ptr->level + 100) / 3 + resist_stun)) {
						else if (!magik(90 - (1000 / (r_ptr->level + 50)) + resist_stun)) {
							m_ptr->stunned += (stun_effect + get_skill_scale(p_ptr, SKILL_COMBAT, 3));
							if (m_ptr->stunned > 100)
								msg_format(Ind, "\377y%^s is knocked out.", m_name);
							else if (m_ptr->stunned > 50)
								msg_format(Ind, "\377y%^s is heavily stunned.", m_name);
							else
								msg_format(Ind, "\377y%^s is stunned.", m_name);
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

				if (vorpal_cut)	msg_format(Ind, "Your weapon cuts deep into %s!", m_name);

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
//				if (i && magik(20 + (i > 5 ? 5 : i) * 6)) {
				if (i && magik(i > 5 ? 5 : i)) {
					i = rand_int(i);
					if (i < mon_aqua) weapon_takes_damage(Ind, GF_WATER, slot);
					else if (i < mon_aqua + mon_acid) weapon_takes_damage(Ind, GF_ACID, slot);
					else weapon_takes_damage(Ind, GF_FIRE, slot);
				}

				/* May it clone the monster ? */
				if (((f4 & TR4_CLONE) && randint(1000) == 1)
				    || chaos_effect == 6) {
					msg_format(Ind, "Your weapon clones %^s!", m_name);
					multiply_monster(c_ptr->m_idx);
				}

				/* heheheheheh */
				if (!instakills(Ind)) do_nazgul(Ind, &k, &num, r_ptr, slot);

				/* Apply the player damage boni */
				/* (should this also cancelled by nazgul?(for now not)) */
				k += p_ptr->to_d + p_ptr->to_d_melee;

				/* Critical strike moved here, since it works best
				with light weapons, which have low dice. So for gain
				we need the full damage including all to-dam boni */
				k2 = k; /* remember damage before crit */
#ifdef CRIT_VS_BACKSTAB
				if (!backstab && !stab_fleeing)
#endif
				k = critical_melee(Ind, o_ptr->weight, o_ptr->to_h + p_ptr->to_h_melee, k, ((o_ptr->tval == TV_SWORD) && (o_ptr->weight <= 100) && !p_ptr->rogue_heavyarmor), calc_crit_obj(Ind, o_ptr));

			/* handle bare fists/bat/ghost */
			} else {
				k = tot_dam_aux(Ind, o_ptr, k, m_ptr, brand_msg, FALSE);

				/* Apply the player damage boni */
				/* (should this also cancelled by nazgul?(for now not)) */
				k += p_ptr->to_d + p_ptr->to_d_melee;

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
				bs_multiplier = get_skill_scale(p_ptr, SKILL_BACKSTAB, 350 + rand_int(101));
				kl = k * (100 + bs_multiplier);
				kl /= (dual_stab ? 150 : 100);
				kl += m_ptr->hp / (dual_stab ? 30 : 20);
				k = kl;
#ifdef CRIT_VS_VORPAL
				kl = k2 * (100 + bs_multiplier);
				kl /= (dual_stab ? 150 : 100);
				kl += m_ptr->hp / (dual_stab ? 30 : 20);
				k2 = kl;
#endif
			}

			/* Vorpal bonus - multi-dice!
			   (currently +37.5% more branded dice damage and o_ptr->to_d on total average, just for the records) */
                        if (vorpal_cut) {
#ifdef CRIT_VS_VORPAL
				k2 += (magik(25) ? 2 : 1) * (o_ptr->to_d + tot_dam_aux(Ind, o_ptr, vorpal_cut, m_ptr, brand_msg, FALSE)); /* exempts critical strike */
				/* either critical hit or vorpal, not both */
				if (k2 > k) k = k2;
#else
				k += (magik(25) ? 2 : 1) * (o_ptr->to_d + tot_dam_aux(Ind, o_ptr, vorpal_cut, m_ptr, brand_msg, FALSE)); /* exempts critical strike */
#endif
			}

			/* for admins: kill a target in one hit */
			if (instakills(Ind)) k = m_ptr->hp + 1;
			else if (p_ptr->admin_godly_strike) {
				p_ptr->admin_godly_strike--;
				if (!(r_ptr->flags1 & RF1_UNIQUE)) k = m_ptr->hp + 1;
			}

			/* DEG Updated hit message to include damage */
			if (backstab) {
				backstab = FALSE;
			   if (martial) {
				if (r_ptr->flags1 & RF1_UNIQUE)
/*				msg_format(Ind, "You %s twist the neck of the sleeping %s for \377e%d \377wdamage.", nolite ? "*CRUELLY*" : "cruelly", r_name_get(m_ptr), k);
				else msg_format(Ind, "You %s twist the neck of the sleeping %s for \377p%d \377wdamage.", nolite ? "*CRUELLY*" : "cruelly", r_name_get(m_ptr), k);
*/				msg_format(Ind, "You twist the neck of the sleeping %s for \377e%d \377wdamage.", r_name_get(m_ptr), k);
				else msg_format(Ind, "You twist the neck of the sleeping %s for \377p%d \377wdamage.", r_name_get(m_ptr), k);
			   } else {
				if (r_ptr->flags1 & RF1_UNIQUE)
/*				msg_format(Ind, "You %s stab the helpless, sleeping %s for \377e%d \377wdamage.", nolite ? "*CRUELLY*" : "cruelly", r_name_get(m_ptr), k);
				else msg_format(Ind, "You %s stab the helpless, sleeping %s for \377p%d \377wdamage.", nolite ? "*CRUELLY*" : "cruelly", r_name_get(m_ptr), k);
*/				msg_format(Ind, "You stab the helpless, sleeping %s for \377e%d \377wdamage.", r_name_get(m_ptr), k);
				else msg_format(Ind, "You stab the helpless, sleeping %s for \377p%d \377wdamage.", r_name_get(m_ptr), k);
			   }
			}
			else if (stab_fleeing) {
				stab_fleeing = FALSE;
			   if (martial) {
				if (r_ptr->flags1 & RF1_UNIQUE)
/*				msg_format(Ind, "You landed a %s hit on the fleeing %s's back for \377e%d \377wdamage.", nolite2 ? "terrible" : "bitter", r_name_get(m_ptr), k);
				else msg_format(Ind, "You landed a %s hit on the fleeing %s's back for \377g%d \377wdamage.", nolite2 ? "terrible" : "bitter", r_name_get(m_ptr), k);
*/				msg_format(Ind, "You strike the back of %s for \377e%d \377wdamage.", r_name_get(m_ptr), k);
				else msg_format(Ind, "You strike the back of %s for \377p%d \377wdamage.", r_name_get(m_ptr), k);
			   } else {
				if (r_ptr->flags1 & RF1_UNIQUE)
/*				msg_format(Ind, "You %s the fleeing %s for \377e%d \377wdamage.", nolite2 ? "*backstab*" : "backstab", r_name_get(m_ptr), k);
				else msg_format(Ind, "You %s the fleeing %s for \377g%d \377wdamage.", nolite2 ? "*backstab*" : "backstab", r_name_get(m_ptr), k);
*/				msg_format(Ind, "You backstab the fleeing %s for \377e%d \377wdamage.", r_name_get(m_ptr), k);
				else msg_format(Ind, "You backstab the fleeing %s for \377p%d \377wdamage.", r_name_get(m_ptr), k);
			   }
			}
//			else if ((r_ptr->flags1 & RF1_UNIQUE) && (!martial)) msg_format(Ind, "You hit %s for \377p%d \377wdamage.", m_name, k);
//			else if (!martial) msg_format(Ind, "You hit %s for \377g%d \377wdamage.", m_name, k);
			else {
				if (r_ptr->flags1 & RF1_UNIQUE)
				msg_format(Ind, "%s for \377e%d \377wdamage.", hit_desc, k);
				else msg_format(Ind, "%s for \377g%d \377wdamage.", hit_desc, k);
			}
//less spam for now - C. Blue   if (strlen(brand_msg) > 0) msg_print(Ind, brand_msg);

			/* target dummy */
			if (m_ptr->r_idx == 1101 || m_ptr->r_idx == 1126) {
				/* Hack: Reduce snow on it during winter season :) */
				m_ptr->extra -= 5;
				if (m_ptr->extra < 0) m_ptr->extra = 0;
					if ((m_ptr->r_idx == 1126) && (m_ptr->extra < 30)) {
					m_ptr->r_idx = 1101;
					everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
				}
			}

			/* Damage, check for fear and death */
			if (mon_take_hit(Ind, c_ptr->m_idx, k, &fear, NULL)) {
				/* Vampires feed off the life force! (if any) */
				// mimic forms for vampires/bats: 432, 520, 521, 623, 989
				if (p_ptr->prace == RACE_VAMPIRE && drainable) {
					int feed = m_ptr->maxhp + 100;
//					feed = (4 - (300 / feed)) * 1000;//1000..4000
					feed = (6 - (300 / feed)) * 100;//300..600
					if (r_ptr->flags3 & RF3_DEMON) feed /= 2;
					if (r_ptr->d_char == 'A') feed /= 3;
					/* Never get gorged */
					feed += p_ptr->food;
					if (feed >= PY_FOOD_MAX) feed = PY_FOOD_MAX - 1;
					set_food(Ind, feed);
				}
				break; /* monster is dead */
			}

			touch_zap_player(Ind, c_ptr->m_idx);

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
					if (pr_ptr->blow[i].d_dice * pr_ptr->blow[i].d_side) {
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
					case RBE_HALLU:
						if (r_ptr->flags3 & RF3_NO_CONF) break;
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

						hp_player_quiet(Ind, drain_heal, TRUE);
						/* We get to keep some of it! */
					}
				}
			}

			/* Confusion attack */
			if ((p_ptr->confusing) || (chaos_effect == 3)) {
				/* Cancel glowing hands */
				p_ptr->confusing = FALSE;

				/* Message */
				msg_print(Ind, "Your hands stop glowing.");

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

			else if (chaos_effect == 4) {
				if (teleport_away(c_ptr->m_idx, 50)) {
					msg_format(Ind, "%^s disappears!", m_name);
					num = p_ptr->num_blow + 1; /* Can't hit it anymore! */
				}
//				no_extra = TRUE;
			}

/*			else if ((chaos_effect == 5) && cave_floor_bold(zcave,y,x)
					&& (randint(90) > m_ptr->level))*/
			else if ((chaos_effect == 5) && (randint(150) > m_ptr->level)) {
				if (!((r_ptr->flags1 & RF1_UNIQUE) ||
				    (r_ptr->flags4 & RF4_BR_CHAO) ||
				    (r_ptr->flags9 & RF9_RES_CHAOS) ))
//						|| (m_ptr->mflag & MFLAG_QUEST)))
				{
					int tmp = poly_r_idx(m_ptr->r_idx);

					/* Pick a "new" monster race */

					/* Handle polymorph */
					if (tmp != m_ptr->r_idx) {
						msg_format(Ind, "%^s changes!", m_name);

						/* Create a new monster (no groups) */
						(void)place_monster_aux(wpos, y, x, tmp, FALSE, FALSE, m_ptr->clone, m_ptr->clone_summoning);

						/* "Kill" the "old" monster */
						delete_monster_idx(c_ptr->m_idx, TRUE);

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
				else
					msg_format(Ind, "%^s is unaffected.", m_name);
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
			if (p_ptr->ghost) {
				fear_chance = 50 + (p_ptr->lev - r_ptr->level) * 5;
				if (!(r_ptr->flags3 & RF3_NO_FEAR) && rand_int(100) < fear_chance) {
					msg_format(Ind, "%^s appears afraid.", m_name);
					m_ptr->monfear = m_ptr->monfear + 4 + rand_int(p_ptr->lev) / 5 + rand_int(get_skill_scale(p_ptr, SKILL_COMBAT, 5));
				}
			}

			/* Fruit bats get life stealing.
			   Note: This is ok because fruit bats cannot wear weapons/gloves as a
			   source of vampirism.
			   Still to check: Vampire fruit bats! CURRENTLY STACKS! */
			if (p_ptr->fruit_bat == 1 && !p_ptr->body_monster && drainable) {
				int leech = m_ptr->hp;
				if (k < leech) leech = k;
				leech /= 10;
				hp_player_quiet(Ind, rand_int(leech), TRUE);
			}
		}

		/* Player misses */
		else {
			backstab = stab_fleeing = FALSE;

			/* Message */
			if (block) {
				sprintf(hit_desc, "\377%c%s blocks.", COLOUR_BLOCK_MON, m_name);
				hit_desc[0] = toupper(hit_desc[0]);
				msg_format(Ind, hit_desc);
#ifdef USE_SOUND_2010
				if (sfx == 0) sound(Ind, "block_shield", NULL, SFX_TYPE_ATTACK, FALSE);
#endif
			} else if (parry) {
				sprintf(hit_desc, "\377%c%s parries.", COLOUR_PARRY_MON, m_name);
				hit_desc[0] = toupper(hit_desc[0]);
				msg_format(Ind, hit_desc);
#ifdef USE_SOUND_2010
				if (sfx == 0) sound(Ind, "parry_weapon", "parry", SFX_TYPE_ATTACK, FALSE);
#endif
			} else {
				msg_format(Ind, "You miss %s.", m_name);
#ifdef USE_SOUND_2010
				if (sfx == 0) {
					if (o_ptr->k_idx && is_weapon(o_ptr->tval))
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
		if (!dual_stab) sleep_stab = cloaked_stab = shadow_stab = FALSE;
		if (dual_stab) {
			num--;
			continue;
		}

		/* Hack -- divided turn for auto-retaliator */
		if (!old) break;
	}

	/* Hack -- delay fear messages */
	if (fear && p_ptr->mon_vis[c_ptr->m_idx]) {
#ifdef USE_SOUND_2010
#else
		sound(Ind, SOUND_FLEE);
#endif

		/* Message */
		if (!streq(m_name, "Morgoth, Lord of Darkness"))
			msg_format(Ind, "%^s flees in terror!", m_name);
		else
			msg_format(Ind, "%^s retreats!", m_name);
	}

	/* Mega-Hack -- apply earthquake brand */
	if (do_quake) {
		if (o_ptr->k_idx
#ifdef ALLOW_NO_QUAKE_INSCRIPTION
		    && !check_guard_inscription(o_ptr->note, 'E')
#else
		    && (!check_guard_inscription(o_ptr->note, 'E') ||
		    o_ptr->name1 != ART_GROND)
#endif
		    ) {
			/* Giga-Hack -- equalize the chance (though not likely..) */
			if (old || randint(p_ptr->num_blow) < 3)
				earthquake(&p_ptr->wpos, p_ptr->py, p_ptr->px, 10);
		}
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
void py_attack(int Ind, int y, int x, bool old)
{
	player_type *p_ptr = Players[Ind];
	cave_type **zcave;
	cave_type *c_ptr;
	struct worldpos *wpos = &p_ptr->wpos;
	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

	if ((p_ptr->body_monster &&
	    (r_info[p_ptr->body_monster].flags1 & RF1_NEVER_BLOW))
	    || !p_ptr->num_blow) return;

	/* Break goi/manashield */
#if 0
	if (p_ptr->invuln) set_invuln(Ind, 0);
	if (p_ptr->tim_manashield) set_tim_manashield(Ind, 0);
	if (p_ptr->tim_wraith) set_tim_wraith(Ind, 0);
#endif	// 0

	/* Check for monster */
	if (c_ptr->m_idx > 0)
		py_attack_mon(Ind, y, x, old);
	/* Check for player */
	else if (c_ptr->m_idx < 0 && cfg.use_pk_rules != PK_RULES_NEVER)
		py_attack_player(Ind, y, x, old);

	if (p_ptr->warning_ranged_autoret == 0) {
		p_ptr->warning_ranged_autoret = 1;
		if (p_ptr->max_plv <= 10) {
			msg_print(Ind, "\374\377yHINT: Inscribe your ranged weapon '\377R@O\377y' to use it for auto-retaliation!");
			s_printf("warning_ranged_autoret: %s\n", p_ptr->name);
		}
	}
}

void spin_attack(int Ind)
{
	player_type *p_ptr = Players[Ind];
	cave_type **zcave;
	cave_type *c_ptr;
	int i, j, x, y;
	struct worldpos *wpos = &p_ptr->wpos;
	if(!(zcave = getcave(wpos))) return;

	if (r_info[p_ptr->body_monster].flags1 & RF1_NEVER_BLOW
			|| !p_ptr->num_blow) return;

	if (!p_ptr->num_blow) return; /* paranoia */

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
// void touch_zap_player(int Ind, monster_type *m_ptr)
void touch_zap_player(int Ind, int m_idx)
{
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

			if (p_ptr->oppose_fire) aura_damage = (aura_damage+2) / 3;
			if (p_ptr->resist_fire) aura_damage = (aura_damage+2) / 3;
			if (p_ptr->suscep_fire) aura_damage = aura_damage * 2;

			msg_format(Ind, "You are enveloped in flames for \377w%d\377w damage!", aura_damage);
			take_hit(Ind, aura_damage, aura_dam, 0);
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

			if (p_ptr->oppose_elec) aura_damage = (aura_damage+2) / 3;
			if (p_ptr->resist_elec) aura_damage = (aura_damage+2) / 3;
			if (p_ptr->suscep_elec) aura_damage = aura_damage * 2;

			msg_format(Ind, "You get zapped for \377w%d\377w damage!", aura_damage);
			take_hit(Ind, aura_damage, aura_dam, 0);
#ifdef OLD_MONSTER_LORE
			r_ptr->r_flags2 |= RF2_AURA_ELEC;
#endif
			handle_stuff(Ind);
		}
	}


	if (r_ptr->flags2 & (RF3_AURA_COLD)) {
		if (!(p_ptr->immune_cold)) {
			char aura_dam[80];

			aura_damage = damroll(1 + (m_ptr->level / 26), 1 + (m_ptr->level / 17));

			/* Hack -- Get the "died from" name */
			monster_desc(Ind, aura_dam, m_idx, 0x88);

			if (p_ptr->oppose_cold) aura_damage = (aura_damage+2) / 3;
			if (p_ptr->resist_cold) aura_damage = (aura_damage+2) / 3;
			if (p_ptr->suscep_cold) aura_damage = aura_damage * 2;

			msg_format(Ind, "You are freezing for \377w%d\377w damage!", aura_damage);
			take_hit(Ind, aura_damage, aura_dam, 0);
#ifdef OLD_MONSTER_LORE
			r_ptr->r_flags3 |= RF3_AURA_COLD;
#endif
			handle_stuff(Ind);
		}
	}
}
void py_touch_zap_player(int Ind, int Ind2)
{
	player_type *q_ptr = Players[Ind2], *p_ptr = Players[Ind];
	int aura_damage = 0;

	if (q_ptr->sh_fire) {
		if (!(p_ptr->immune_fire)) {
			aura_damage = damroll(2, 6);
			if (p_ptr->oppose_fire) aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->resist_fire) aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->suscep_fire) aura_damage = aura_damage * 2;

			msg_format(Ind, "You are enveloped in flames for \377w%d\377w damage!", aura_damage);
			take_hit(Ind, aura_damage, "a fire aura", Ind2);
			handle_stuff(Ind);
		}
	}
	if (q_ptr->sh_cold) {
		if (!(p_ptr->immune_cold)) {
			aura_damage = damroll(2, 6);
			if (p_ptr->oppose_cold) aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->resist_cold) aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->suscep_cold) aura_damage = aura_damage * 2;

			msg_format(Ind, "You are chilled by frost for \377w%d\377w damage!", aura_damage);
			take_hit(Ind, aura_damage, "a frost aura", Ind2);
			handle_stuff(Ind);
		}
	}
	if (q_ptr->sh_elec) {
		if (!(p_ptr->immune_elec)) {
			aura_damage = damroll(2, 6);
			if (p_ptr->oppose_elec) aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->resist_elec) aura_damage = (aura_damage + 2) / 3;
			if (p_ptr->suscep_elec) aura_damage = aura_damage * 2;

			msg_format(Ind, "You are struck by lightning for \377w%d\377w damage!", aura_damage);
			take_hit(Ind, aura_damage, "a lightning aura", Ind2);
			handle_stuff(Ind);
		}
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
//void do_nazgul(int *k, int *num, int num_blow, int weap, monster_race *r_ptr, object_type *o_ptr)
void do_nazgul(int Ind, int *k, int *num, monster_race *r_ptr, int slot)
{
	object_type *o_ptr = &Players[Ind]->inventory[slot];
	char o_name[ONAME_LEN];

	if (r_ptr->flags7 & RF7_NAZGUL) {
//		int weap = 0;	// Hack!  <- ???
		u32b f1, f2, f3, f4, f5, esp;

		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

		if ((!o_ptr->name2) && (!artifact_p(o_ptr))) {
			if (!(f1 & TR1_SLAY_EVIL) && !(f1 & TR1_SLAY_UNDEAD) && !(f1 & TR1_KILL_UNDEAD)) {
				msg_print(Ind, "The Ringwraith is IMPERVIOUS to the mundane weapon.");
				*k = 0;
			}

			/* Dark Swords resist somewhat */
			if ((o_ptr->tval == TV_SWORD && o_ptr->sval == SV_DARK_SWORD) ? magik(15) : magik(100)) {
				object_desc(0, o_name, o_ptr, TRUE, 3);
				s_printf("NAZGUL_DISI_NORM: %s : %s.\n", Players[Ind]->name, o_name);

				msg_print(Ind, "\376\377rYour weapon *DISINTEGRATES*!");
//				inven_item_increase(Ind, INVEN_WIELD + weap, -1);
//				inven_item_optimize(Ind, INVEN_WIELD + weap);
				inven_item_increase(Ind, slot, -1); /* this way using slot we can handle dual-wield */
				inven_item_optimize(Ind, slot);
				/* To stop attacking */
	//			*num = num_blow;
			}
		} else if (like_artifact_p(o_ptr)) {
			if (!(f1 & TR1_SLAY_EVIL) && !(f1 & TR1_SLAY_UNDEAD) && !(f1 & TR1_KILL_UNDEAD)) {
				msg_print(Ind, "The Ringwraith is IMPERVIOUS to the mundane weapon.");
				*k = 0;
			}

//			apply_disenchant(Ind, INVEN_WIELD + weap);
//			apply_disenchant(Ind, weap);

			/* 1/1000 chance of getting destroyed.
			   Exploit-fix here for permacursed items. (Grond only) */
			if (!rand_int(1000) && !(f3 & TR3_PERMA_CURSE)) {
				object_desc(0, o_name, o_ptr, TRUE, 3);
				s_printf("NAZGUL_DISI_ARTLIKE: %s : %s.\n", Players[Ind]->name, o_name);

				if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1);
				msg_print(Ind, "\376\377rYour weapon is destroyed!");
//				inven_item_increase(Ind, INVEN_WIELD + weap, -1);
//				inven_item_optimize(Ind, INVEN_WIELD + weap);
				inven_item_increase(Ind, slot, -1);
				inven_item_optimize(Ind, slot);

				/* To stop attacking */
//				*num = num_blow;
			}
		} else if (o_ptr->name2) {
			if (!(f1 & TR1_SLAY_EVIL) && !(f1 & TR1_SLAY_UNDEAD) && !(f1 & TR1_KILL_UNDEAD)) {
				msg_print(Ind, "The Ringwraith is IMPERVIOUS to the mundane weapon.");
				*k = 0;
			}

			/* Dark Swords and *Slay Undead* weapons resist the Nazgul,
			   other (ego) weapons have a high chance of getting destroyed */
			if (((o_ptr->tval == TV_SWORD && o_ptr->sval == SV_DARK_SWORD)
			    || o_ptr->name2 == EGO_KILL_UNDEAD || o_ptr->name2b == EGO_KILL_UNDEAD
			    ) ? magik(3) : magik(20))
			{
				object_desc(0, o_name, o_ptr, TRUE, 3);
				s_printf("NAZGUL_DISI_EGO: %s : %s.\n", Players[Ind]->name, o_name);

				msg_print(Ind, "\376\377rYour weapon is destroyed!");
//				inven_item_increase(Ind, INVEN_WIELD + weap, -1);
//				inven_item_optimize(Ind, INVEN_WIELD + weap);
				inven_item_increase(Ind, slot, -1);
				inven_item_optimize(Ind, slot);

				/* To stop attacking */
//				*num = num_blow;
			}
		}

		/* If any damage is done, then 25% chance of getting the Black Breath */
//		if ((*k) && magik(25) && !Players[Ind]->black_breath) {
		if (magik(15) && !Players[Ind]->black_breath) {
			s_printf("EFFECT: BLACK-BREATH - %s was infected by a Nazgul\n", Players[Ind]->name);
			set_black_breath(Ind);
		}
	}
}

void set_black_breath(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* deja */
	if (p_ptr->black_breath) return;
	if (p_ptr->ghost) return;

	msg_print(Ind, "\376\377DYour foe calls upon your soul!");
	msg_print(Ind, "\376\377DYou feel the Black Breath slowly draining you of life...");
	p_ptr->black_breath = TRUE;
}


/* Do a probability travel in a wall */
void do_prob_travel(int Ind, int dir)
{
  player_type *p_ptr = Players[Ind];
  int x = p_ptr->px, y = p_ptr->py, tries = 0;
  bool do_move = TRUE;
  struct worldpos *wpos=&p_ptr->wpos;
  cave_type **zcave;
  dun_level *l_ptr = getfloor(&p_ptr->wpos);
  if(!(zcave=getcave(wpos))) return;

  /* Paranoia */
  if (dir == 5) return;
  if ((dir < 1) || (dir > 9)) return;

  /* No probability travel in sticky vaults */
  if(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) return;

  /* Neither on NO_MAGIC levels */
  if (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC)) return;

  x += ddx[dir];
  y += ddy[dir];

  while (++tries < 1000)
    {
      /* Do not get out of the level */
      if (!in_bounds(y, x))
	{
	  do_move = FALSE;
	  break;
	}

      /* Still in rock ? continue */
      if ((!cave_empty_bold(zcave, y, x)) || (zcave[y][x].info & CAVE_ICKY))
	{
	  y += ddy[dir];
	  x += ddx[dir];
	  continue;
	}

      /* Everything is ok */
      do_move = TRUE;
      break;
    }
  if (tries == 1000) return; /* fail */

  if (do_move)
    {
      int oy, ox;

      /* Save old location */
      oy = p_ptr->py;
      ox = p_ptr->px;

      /* Move the player */
      p_ptr->py = y;
      p_ptr->px = x;

      /* Update the player indices */
      zcave[oy][ox].m_idx = 0;
      zcave[y][x].m_idx = 0 - Ind;
      cave_midx_debug(wpos, y, x, -Ind);

      /* Redraw new spot */
      everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);

      /* Redraw old spot */
      everyone_lite_spot(wpos, oy, ox);

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
}


/* Experimental! lets hope not bugged */
/* Wraith walk in own house */
bool wraith_access(int Ind) {
	player_type *p_ptr  =Players[Ind];
	int i;
	bool house = FALSE;

	for (i = 0; i < num_houses; i++) {
		if (inarea(&houses[i].wpos, &p_ptr->wpos)) {
			if (fill_house(&houses[i], FILL_PLAYER, p_ptr)) {
				house = TRUE;
				if (access_door(Ind, houses[i].dna, TRUE)
				    || admin_p(Ind))
					return(TRUE);
				break;
			}
		}
	}
	return (house ? FALSE : TRUE);
}


/*
 * Hack function to determine if the player has access to the GIVEN location,
 * using the function above.	- Jir -
 */
static bool wraith_access_virtual(int Ind, int y, int x)
{
	player_type *p_ptr=Players[Ind];
	int oy = p_ptr->py, ox = p_ptr->px;
	bool result;

	p_ptr->py = y;
	p_ptr->px = x;

	result = wraith_access(Ind);

	p_ptr->py = oy;
	p_ptr->px = ox;

	return (result);
}



/* borrowed from ToME	- Jir - */
/* NOTE: in ToME fly gives free FF, but in TomeNET not. */
bool player_can_enter(int Ind, byte feature)
{
	player_type *p_ptr = Players[Ind];
	bool pass_wall;

	bool only_wall = FALSE;

	/* Dungeon Master pass through everything (cept array boundary :) */
	if (p_ptr->admin_dm &&
	    feature != FEAT_PERM_SOLID &&
	    feature != FEAT_HIGH_MOUNT_SOLID &&
	    feature != FEAT_GLIT_WATER)
//	if (p_ptr->admin_dm && !(f_info[feature].flags1 & FF1_BOUNDARY))
		return (TRUE);

	/* Player can not walk through "walls" unless in Shadow Form */
//        if (p_ptr->wraith_form || (PRACE_FLAG(PR1_SEMI_WRAITH)))
	if (/*p_ptr->wraith_form ||*/ p_ptr->ghost || p_ptr->tim_wraith)
		pass_wall = TRUE;
	else
		pass_wall = FALSE;

#if 0	// it's interesting.. hope we can have similar feature :)
	/* Wall mimicry force the player to stay in walls */
	if (p_ptr->mimic_extra & CLASS_WALL) only_wall = TRUE;
#endif

	switch (feature) {
#if 0
		/* NOTE: we're not to backport wild_mode (it's cheezy);
		 * however it's good idea to restrict crossing severer is nice idea
		 * so p_ptr->wild_mode code can be recycled.	- Jir -
		 */
		case FEAT_DEEP_WATER:
			if (p_ptr->wild_mode) {
				int wt = (adj_str_wgt[p_ptr->stat_ind[A_STR]] * 100) / 2;

				if ((calc_total_weight() < wt) || (p_ptr->ffall))
					return (TRUE);
				else
					return (FALSE);
			} else
				return (TRUE);

		case FEAT_SHAL_LAVA:
			if (p_ptr->wild_mode) {
				if ((p_ptr->resist_fire) ||
					(p_ptr->immune_fire) ||
					(p_ptr->oppose_fire) || (p_ptr->ffall))
					return (TRUE);
				else
					return (FALSE);
			} else
				return (TRUE);

		case FEAT_DEEP_LAVA:
			if (p_ptr->wild_mode) {
				if ((p_ptr->resist_fire) ||
					(p_ptr->immune_fire) ||
					(p_ptr->oppose_fire) || (p_ptr->ffall))
					return (TRUE);
				else
					return (FALSE);
			} else
				return (TRUE);
#endif
		case FEAT_DEEP_WATER:
		case FEAT_SHAL_LAVA:
		case FEAT_DEEP_LAVA:
			return (TRUE);	/* you can pass, but you may suffer dmg */

		case FEAT_DEAD_TREE:
			if ((p_ptr->fly) || pass_wall || p_ptr->town_pass_trees)
			    return (TRUE);
		case FEAT_BUSH:
		case FEAT_TREE:
		{
			/* 708 = Ent (passes trees), 83/142 novice ranger, 345 ranger, 637 ranger chieftain, 945 high-elven ranger */
			if ((p_ptr->fly) || (p_ptr->pass_trees) || pass_wall || p_ptr->town_pass_trees)
#if 0
			    (PRACE_FLAG(PR1_PASS_TREE)) ||
			    (get_skill(SKILL_DRUID) > 15) ||
			    (p_ptr->mimic_form == MIMIC_ENT))
#endif
				return (TRUE);
			else
				return (FALSE);
		}

#if 0
		case FEAT_WALL_HOUSE:
		{
			if (!pass_wall || !wraith_access_virtual(Ind)) return (FALSE);
			else return (TRUE);
		}
#endif	// 0

		default:
		{
			if ((p_ptr->climb) && (f_info[feature].flags1 & FF1_CAN_CLIMB))
				return (TRUE);
			if ((p_ptr->fly) &&
				((f_info[feature].flags1 & FF1_CAN_FLY) ||
				 (f_info[feature].flags1 & FF1_CAN_LEVITATE)))
				return (TRUE);
			else if (only_wall && (f_info[feature].flags1 & FF1_FLOOR))
				return (FALSE);
			else if ((p_ptr->feather_fall) &&
					 (f_info[feature].flags1 & FF1_CAN_LEVITATE))
				return (TRUE);
			else if ((pass_wall || only_wall) &&
					 (f_info[feature].flags1 & FF1_CAN_PASS))
				return (TRUE);
			else if (f_info[feature].flags1 & FF1_NO_WALK)
				return (FALSE);
			else if ((f_info[feature].flags1 & FF1_WEB) &&
					 (!(r_info[p_ptr->body_monster].flags7 & RF7_SPIDER)))
				return (FALSE);
		}
	}

	return (TRUE);
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

void move_player(int Ind, int dir, int do_pickup)
{
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos, nwpos, old_wpos;

	int y, x, oldx, oldy;
	int i;
	//bool do_move = FALSE;

	cave_type               *c_ptr;
	struct c_special	*cs_ptr;
	object_type             *o_ptr;
	monster_type            *m_ptr;
	byte                    *w_ptr;
	monster_race *r_ptr = &r_info[p_ptr->body_monster];
	cave_type **zcave;
	int csmove = TRUE;
	
	bool old_grid_sunlit = FALSE, new_grid_sunlit = FALSE; /* for vampires */

	if (!(zcave = getcave(wpos))) return;


	/* (S)He's no longer AFK, lol */
	un_afk_idle(Ind);

	/* Can we move ? */
	if (r_ptr->flags1 & RF1_NEVER_MOVE) {
		msg_print(Ind, "You cannot move by nature.");
		return;
	}
	
	stop_precision(Ind); /* aimed precision shot gets interrupted by moving around */
	stop_shooting_till_kill(Ind);

	/* Find the result of moving */

	/* -C. Blue- I toned down monster RAND_50 and RAND_25 for a mimicking player,
	assuming that the mimic does not use the monster mind but its own to control
	the body, on the other hand the body still carries reflexes from the monster ;)
	- technical reason was to make more forms useful, especially RAND_50 forms */
/*	if (((p_ptr->pclass != CLASS_SHAMAN) || ((r_ptr->d_char != 'E') && (r_ptr->d_char != 'G'))) && */
	/* And now shamans gain advantage by linking to the being's mind instead of copying it..or something..err ^^ */
	if ((p_ptr->pclass != CLASS_SHAMAN) &&
	    (((r_ptr->flags5 & RF5_RAND_100) && magik(40)) ||
	    ((r_ptr->flags1 & RF1_RAND_50) && (r_ptr->flags1 & RF1_RAND_25) && magik(30)) ||
	    ((r_ptr->flags1 & RF1_RAND_50) && (!(r_ptr->flags1 & RF1_RAND_25)) && magik(20)) ||
	    ((!(r_ptr->flags1 & RF1_RAND_50)) && (r_ptr->flags1 & RF1_RAND_25) && magik(10))))
	{
		do {
			i = randint(9);
			y = p_ptr->py + ddy[i];
			x = p_ptr->px + ddx[i];
		} while (i == 5);
	} else {
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];
	}

	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* Check sunlight, for vampires */
	if (!p_ptr->wpos.wz && !night_surface && !(c_ptr->info & CAVE_PROT) &&
	    !(f_info[c_ptr->feat].flags1 & FF1_PROTECTED))
		old_grid_sunlit = TRUE;

	/* Slip on icy floor */
	if ((c_ptr->feat == FEAT_ICE) && (!p_ptr->feather_fall && !p_ptr->fly)) {
		if (magik(70 - p_ptr->lev)) {
			do {
				i = randint(9);
				y = p_ptr->py + ddy[i];
				x = p_ptr->px + ddx[i];
			} while (i == 5);
			msg_print(Ind, "You slip on the icy floor.");
		}
#if 0
		else
			tmp = dir;
#endif
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
			/* Hack: Nobody leaves (0, 0) while sector00separation is on - mikaelh */
			if (sector00separation && wpos->wx == WPOS_SECTOR00_X &&
			    wpos->wy == WPOS_SECTOR00_Y && !is_admin(p_ptr)) {
				return;
			}

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

			/* Hack: Nobody enters (0, 0) while sector00separation is on - mikaelh */
			if (sector00separation && nwpos.wx == WPOS_SECTOR00_X &&
			    nwpos.wy == WPOS_SECTOR00_Y && !is_admin(p_ptr)) {
				p_ptr->px = oldx;
				p_ptr->py = oldy;
				return;
			}

			/* Remove the player from the old location */
			zcave[oldy][oldx].m_idx = 0;

			/* Show everyone that's he left */
			everyone_lite_spot(&p_ptr->wpos, oldy, oldx);

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
			if(!p_ptr->ghost)
				p_ptr->wild_map[(p_ptr->wpos.wx + p_ptr->wpos.wy * MAX_WILD_X) / 8] |=
				    (1 << ((p_ptr->wpos.wx + p_ptr->wpos.wy * MAX_WILD_X) % 8));

			new_players_on_depth(wpos, 1, TRUE);
			p_ptr->new_level_flag = TRUE;
			p_ptr->new_level_method = LEVEL_OUTSIDE;

			return;
		}
	}

	/* Examine the destination */
	c_ptr = &zcave[y][x];

	if (!p_ptr->wpos.wz && !night_surface && !(c_ptr->info & CAVE_PROT) &&
	    !(f_info[c_ptr->feat].flags1 & FF1_PROTECTED))
		new_grid_sunlit = TRUE;

	w_ptr = &p_ptr->cave_flag[y][x];

	/* Get the object */
	o_ptr = &o_list[c_ptr->o_idx];

	/* Get the monster */
	m_ptr = &m_list[c_ptr->m_idx];

	/* Save "last direction moved" */
	p_ptr->last_dir = dir;

	/* Bump into other players */
	if (c_ptr->m_idx < 0)
	    /* mountains for example are FF1_PERMANENT too! */
//	    && (f_info[c_ptr->feat].flags1 & FF1_SWITCH_MASK)) /* never swich places into perma wall */
	{
		player_type *q_ptr = Players[0 - c_ptr->m_idx];
		int Ind2 = 0 - c_ptr->m_idx;
		bool blocks_important_feat = FALSE; /* does the player block an important feature, like staircases in towns? - C. Blue
						       always make them 'switch places' instead of bumping. */

		switch (c_ptr->feat) {
		case FEAT_SHOP: if (GetCS(c_ptr, CS_SHOP)->sc.omni != 7) break; /* Inn entrance */
		case FEAT_WAY_MORE:
		case FEAT_WAY_LESS:
		case FEAT_MORE:
		case FEAT_LESS:
//			if (q_ptr->afk || !q_ptr->wpos.wz) blocks_important_feat = TRUE; 
			if (!q_ptr->wpos.wz) blocks_important_feat = TRUE; 
			break;
		default: break;
		}

		/* Check for an attack */
		if (cfg.use_pk_rules != PK_RULES_NEVER &&
		    check_hostile(Ind, Ind2)) {
			py_attack(Ind, y, x, TRUE);
			return;
		}

		/* If both want to switch, do it */
#if 0
		/* TODO: always swap when in party
		 * this can allow one to pass through walls... :(
		 */
		else if ( (!p_ptr->ghost && !q_ptr->ghost &&
				((ddy[q_ptr->last_dir] == -(ddy[dir]) &&
				ddx[q_ptr->last_dir] == (-ddx[dir]))) ||
				(player_in_party(p_ptr->party, Ind2) &&
				 !q_ptr->store_num) )||
				(q_ptr->admin_dm) )
#else
		else if (((!p_ptr->ghost && !q_ptr->ghost &&
			 (ddy[q_ptr->last_dir] == -(ddy[dir])) &&
			 (ddx[q_ptr->last_dir] == (-ddx[dir])) &&
			 !p_ptr->afk && !q_ptr->afk) ||
			q_ptr->admin_dm || blocks_important_feat)
			/* don't switch someone 'out' of a shop, except for the Inn */
			&& (!(cs_ptr = GetCS(c_ptr, CS_SHOP)) || cs_ptr->sc.omni == 7)
//moved above		&& !(f_info[c_ptr->feat].flags1 & FF1_PERMANENT)) /* never swich places into perma wall (only case possible: if target player is admin) */
			&& (f_info[c_ptr->feat].flags1 & FF1_SWITCH_MASK)) /* never swich places into perma wall */
#endif	// 0

		{
/*		    	if (!((!wpos->wz) && (p_ptr->tim_wraith || q_ptr->tim_wraith)))*/
			/* switch places only if BOTH have WRAITHFORM or NONE has it, well or if target is a DM */
			if ((!(p_ptr->afk || q_ptr->afk) && /* dont move AFK players into trees to kill them */
			    ((p_ptr->tim_wraith && q_ptr->tim_wraith) || (!p_ptr->tim_wraith && !q_ptr->tim_wraith)))
			    || blocks_important_feat || q_ptr->admin_dm)
			{

				c_ptr->m_idx = 0 - Ind;
				zcave[p_ptr->py][p_ptr->px].m_idx = 0 - Ind2;

				q_ptr->py = p_ptr->py;
				q_ptr->px = p_ptr->px;
				calc_boni(Ind2); /* in case he's a vampire and got swapped from/onto sunlit grid */

				p_ptr->py = y;
				p_ptr->px = x;
				if (old_grid_sunlit != new_grid_sunlit) calc_boni(Ind);

				cave_midx_debug(wpos, p_ptr->py, p_ptr->px, -Ind);
				cave_midx_debug(wpos, q_ptr->py, q_ptr->px, -Ind2);

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
					stop_shooting_till_kill(Ind);

					/* Disturb both of them */
					disturb(Ind, 1, 0);
					disturb(Ind2, 1, 0);
				}

				/* Re-show both grids */
				everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);
				everyone_lite_spot(wpos, q_ptr->py, q_ptr->px);

				p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
				q_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
#if 0
				/* Check for new panel (redraw map) */
				verify_panel(Ind);
				/* Update the monsters */
				p_ptr->update |= (PU_DISTANCE);
				/* Window stuff */
				p_ptr->window |= (PW_OVERHEAD);
				/* Hack -- quickly update the view, to reduce perceived lag */
				redraw_stuff(Ind);
				window_stuff(Ind);
#endif
#if 0 /* replace suppressed switching by bumping */
			} else if ((p_ptr->afk || q_ptr->afk) && 
			    ((p_ptr->tim_wraith && q_ptr->tim_wraith) || (!p_ptr->tim_wraith && !q_ptr->tim_wraith)))
			{
#endif
			} else {
				black_breath_infection(Ind, Ind2); /* =p */
				disturb(Ind, 1, 0); /* turn off running, so player won't be un-AFK'ed automatically */
//was here->			return;
			}
			return;//<- moved it here, so switching gets terminated too
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
					int tackle;
					tackle = randint(20);
					if (tackle > 10) {
						tmp_obj = *o_ptr;
						if (tackle < 18) {
							msg_format_near(Ind2, "\377v%s is tackled by %s", q_ptr->name, p_ptr->name);
							msg_format(Ind2, "\377r%s tackles you", p_ptr->name);
							tmp_obj.marked2 = ITEM_REMOVAL_NEVER;
							drop_near(&tmp_obj, -1, wpos, y, x);
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
				}else {
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
		    (!instakills(Ind) ||
		    /* except except if he's not disabled auto-retaliation */
		    (p_ptr->running && p_ptr->inventory[INVEN_WIELD].note &&
		    strstr(quark_str(p_ptr->inventory[INVEN_WIELD].note), "@Rx")))) {
			/* save old player location */
			oldx = p_ptr->px;
			oldy = p_ptr->py;
			/* update player location */
			p_ptr->px = m_list[c_ptr->m_idx].fx;
			p_ptr->py = m_list[c_ptr->m_idx].fy;
			if (old_grid_sunlit != new_grid_sunlit) calc_boni(Ind);
			/* update monster location */
			m_list[c_ptr->m_idx].fx = oldx;
			m_list[c_ptr->m_idx].fy = oldy;
			/* update cave monster indexes */
			zcave[oldy][oldx].m_idx = c_ptr->m_idx;
			c_ptr->m_idx = -Ind;
			/* Re-show both grids */
			everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);
			everyone_lite_spot(wpos, oldy, oldx);
		}
		/* Attack */
		else {
			/* hack: admins who are running with their scythe won't perform a run-attack - C. Blue */
			if (instakills(Ind) && p_ptr->running) {
				disturb(Ind, 0, 0); /* stop running first */
				return;
			}
			py_attack(Ind, y, x, TRUE);
		}

		/* done in any case */
		return;
	}

	/* Prob travel */
	if (p_ptr->prob_travel && (!cave_floor_bold(zcave, y, x))) {
		do_prob_travel(Ind, dir);
		return;
	}

	/* now this is temp while i redesign!!! - do not change  <- ok and who wrote this and when? =p */
	cs_ptr = c_ptr->special;
	while (cs_ptr) {
		int tcv;
		tcv = csfunc[cs_ptr->type].activate(cs_ptr, y, x, Ind);
		cs_ptr = cs_ptr->next;
		if (!tcv) {
			csmove = FALSE;
			printf("csmove is false\n");
		}
	}

	/* Player can not walk through "walls", but ghosts can */
	if (!player_can_enter(Ind, c_ptr->feat) || !csmove) {
		/* walk-through entry for house owners ... sry it's DIRTY -Jir- */
		bool myhome = FALSE;
		bool passing = (p_ptr->tim_wraith || p_ptr->ghost);

		/* XXX quick fix */
		if (passing) {
#if 0
			if (c_ptr->feat == FEAT_HOME) {
				struct c_special *cs_ptr;
				if ((!(cs_ptr = GetCS(c_ptr, CS_DNADOOR)) ||
				    !access_door(Ind, cs_ptr->sc.ptr, TRUE)) && !admin_p(Ind))
				{
					msg_print(Ind, "The door blocks your movement.");
					disturb(Ind, 0, 0);
					return;
				}

				myhome = TRUE;
				msg_print(Ind, "\377GYou pass through the door.");
			} else
#endif	// 0

			/* XXX maybe needless anymore */
			if (c_ptr->feat == FEAT_WALL_HOUSE) {
				if (!wraith_access_virtual(Ind, y, x)) {
					msg_print(Ind, "The wall blocks your movement.");
					disturb(Ind, 0, 0);
					return;
				}

				myhome = TRUE;
				msg_print(Ind, "\377GYou pass through the house wall.");
			}
		}


#if 0
		/* Hack -- Exception for trees (in a bad way :-/) */
		if (!myhome && c_ptr->feat == FEAT_TREE &&
		    (p_ptr->fly || p_ptr->pass_trees))
			myhome = TRUE;
#endif	// 0

	    if (!myhome) {

		/* Disturb the player */
		disturb(Ind, 0, 0);

		/* Notice things in the dark */
		if (!(*w_ptr & CAVE_MARK) &&
		    (p_ptr->blind || !(*w_ptr & CAVE_LITE)))
		{
			if (c_ptr->feat == FEAT_SIGN){
				/*msg_print(Ind, "\377GYou feel a signpost blocking your way.");*/
				*w_ptr |= CAVE_MARK;
				everyone_lite_spot(wpos, y, x);
			}

			/* Rubble */
			if (c_ptr->feat == FEAT_RUBBLE) {
				msg_print(Ind, "\377GYou feel some rubble blocking your way.");
				*w_ptr |= CAVE_MARK;
				everyone_lite_spot(wpos, y, x);
			}

			/* Closed door */
			else if ((c_ptr->feat < FEAT_SECRET && c_ptr->feat >= FEAT_DOOR_HEAD) ||
				 (c_ptr->feat == FEAT_HOME))
			{
				msg_print(Ind, "\377GYou feel a closed door blocking your way.");
				*w_ptr |= CAVE_MARK;
				everyone_lite_spot(wpos, y, x);
			}

			/* Tree */
			else if (c_ptr->feat == FEAT_TREE || c_ptr->feat == FEAT_DEAD_TREE ||
			    c_ptr->feat == FEAT_BUSH) {
				msg_print(Ind, "\377GYou feel a tree blocking your way.");
				*w_ptr |= CAVE_MARK;
				everyone_lite_spot(wpos, y, x);
			}

			/* Dark Pit */
			else if (c_ptr->feat == FEAT_DARK_PIT) {
				msg_print(Ind, "\377GYou don't feel any ground ahead of you.");
				*w_ptr |= CAVE_MARK;
				everyone_lite_spot(wpos, y, x);
			}

			/* Wall (or secret door) */
			else {
				msg_print(Ind, "\377GYou feel a wall blocking your way.");
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
				}
				/* Tree */
				else if (c_ptr->feat == FEAT_TREE || c_ptr->feat == FEAT_DEAD_TREE ||
				    c_ptr->feat == FEAT_BUSH) {
					msg_print(Ind, "There is a tree blocking your way.");
				} else if (c_ptr->feat == FEAT_DARK_PIT) {
					msg_print(Ind, "There is a dark pit in your way.");
				}
				/* Wall (or secret door) */
				else if (c_ptr->feat != FEAT_SIGN) {
					msg_print(Ind, "There is a wall blocking your way.");
				}
			}
		}
		return;
	    } /* 'if (!myhome)' ends here */
	}


	/* XXX fly? */
	else if ((c_ptr->feat == FEAT_DARK_PIT) && !p_ptr->feather_fall &&
	    !p_ptr->fly && !p_ptr->admin_dm) {
		msg_print(Ind, "You can't cross the chasm.");

		disturb(Ind, 0, 0);
		return;
	}

	/* Normal movement */
//	else
	{
		int oy, ox;
		struct c_special *cs_ptr;

		/* Save old location */
		oy = p_ptr->py;
		ox = p_ptr->px;

		/* Move the player */
		p_ptr->py = y;
		p_ptr->px = x;
		if (old_grid_sunlit != new_grid_sunlit) calc_boni(Ind);

		if (zcave[y][x].info & CAVE_STCK && !(zcave[oy][ox].info & CAVE_STCK)) {
			msg_print(Ind, "\377DThe air in here feels very still.");
			p_ptr->redraw |= PR_DEPTH; /* hack: depth colour indicates no-tele */
#ifdef USE_SOUND_2010
			/* New: Have bgm indicate no-tele too! */
			handle_music(Ind);
#endif

			msg_format_near(Ind, "%s loses %s wraith powers.", p_ptr->name, p_ptr->male ? "his":"her");
			msg_print(Ind, "You lose your wraith powers.");
			/* Automatically disable permanent wraith form (set_tim_wraith) */
			p_ptr->update |= PU_BONUS;
		}
		if (zcave[oy][ox].info & CAVE_STCK && !(zcave[y][x].info & CAVE_STCK)) {
			msg_print(Ind, "\377sFresh air greets you as you leave the vault.");
			p_ptr->redraw |= PR_DEPTH; /* hack: depth colour indicates no-tele */
#ifdef USE_SOUND_2010
			/* New: Have bgm indicate no-tele too! */
			handle_music(Ind);
#endif

			/* Automatically re-enable permanent wraith form (set_tim_wraith) */
			p_ptr->update |= PU_BONUS;
		}
		/* Update the player indices */
		zcave[oy][ox].m_idx = 0;
		zcave[y][x].m_idx = 0 - Ind;
		cave_midx_debug(wpos, y, x, -Ind);

		/* Redraw new spot */
		everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);

		/* Redraw old spot */
		everyone_lite_spot(wpos, oy, ox);

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

		/* Spontaneous Searching */
		if ((p_ptr->skill_fos >= 75) ||
		    (0 == rand_int(76 - p_ptr->skill_fos))) {
			search(Ind);
		}

		/* Continuous Searching */
		if (p_ptr->searching) {
			if (p_ptr->pclass == CLASS_ROGUE && !p_ptr->rogue_heavyarmor) {
				//Radius of 5 ... 15 squares
				detect_bounty(Ind, (p_ptr->lev / 5) + 5);
			} else {
				search(Ind);
			}
		}

		/* Handle "objects" */
		if (c_ptr->o_idx && !p_ptr->admin_dm) carry(Ind, do_pickup, 0);
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
			(cs_ptr = GetCS(c_ptr, CS_SHOP)) && cs_ptr->sc.omni == 3)

		{
			if(p_ptr->wild_map[(p_ptr->wpos.wx + p_ptr->wpos.wy*MAX_WILD_X)/8] & (1<<((p_ptr->wpos.wx + p_ptr->wpos.wy*MAX_WILD_X)%8))){
				/* Resurrect him */
				resurrect_player(Ind, 0);

				/* Give him some gold to restart */
//				if (p_ptr->lev > 1 && !p_ptr->admin_dm) {
				if (!p_ptr->admin_dm) {
					/* int i = (p_ptr->lev > 4)?(p_ptr->lev - 3) * 100:100; */
//					int i = (p_ptr->lev > 4)?(p_ptr->lev - 3) * 100 + (p_ptr->lev / 10) * (p_ptr->lev / 10) * 800:100;
//					int i = (p_ptr->lev > 4) ? 100 + (p_ptr->lev * p_ptr->lev * p_ptr->lev) / 5 : 100;
					int i = 300 + (p_ptr->lev * p_ptr->lev * p_ptr->lev) / 2; /* buffed it greatly, yet still sensible */
					msg_format(Ind, "The temple priest gives you %ld gold pieces for your revival!", i);
					p_ptr->au += i;
				}
			}
			else msg_print(Ind, "\377rThe temple priest turns you away!");
		}
#ifndef USE_MANG_HOUSE_ONLY
		else if ((c_ptr->feat == FEAT_HOME || c_ptr->feat == FEAT_HOME_OPEN)
		    && (!p_ptr->ghost || is_admin(p_ptr)))
		{
			disturb(Ind, 1, 0);
			do_cmd_trad_house(Ind);
//			return;	/* allow builders to build */
		}
#endif	// USE_MANG_HOUSE_ONLY


		/* Discover invisible traps */
//		else if (c_ptr->feat == FEAT_INVIS)
		if ((cs_ptr = GetCS(c_ptr, CS_TRAPS)) && !p_ptr->ghost) {
			bool hit = TRUE;

			/* Disturb */
			disturb(Ind, 0, 0);

			if (!cs_ptr->sc.trap.found) {
				/* Message */
//				msg_print(Ind, "You found a trap!");
				msg_print(Ind, "You triggered a trap!");

				/* Pick a trap */
				pick_trap(&p_ptr->wpos, p_ptr->py, p_ptr->px);
			}
//			else if (magik(get_skill_scale(p_ptr, SKILL_DISARM, 90)
			else if (magik(get_skill_scale(p_ptr, SKILL_TRAPPING, 90) - UNAWARENESS(p_ptr))) {
#ifndef ARCADE_SERVER
				msg_print(Ind, "You carefully avoid touching the trap.");
				hit = FALSE;
#endif
			}

			/* Hit the trap */
			if (hit) hit_trap(Ind);
		}

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
	}
}

void black_breath_infection(int Ind, int Ind2)
{
	player_type *p_ptr = Players[Ind];
	player_type *q_ptr = Players[Ind2];

	/* Prevent players who are AFK from getting infected in towns - mikaelh */
	if (p_ptr->black_breath && !q_ptr->black_breath && magik(25) && !(q_ptr->afk && istown(&q_ptr->wpos)) && q_ptr->lev > cfg.newbies_cannot_drop && q_ptr->lev >= BB_INFECT_MINLEV)
	{
		s_printf("EFFECT: BLACK-BREATH - %s was infected by %s\n", q_ptr->name, p_ptr->name);
		set_black_breath(Ind2);
	}
	if (q_ptr->black_breath && !p_ptr->black_breath && magik(25) && !(p_ptr->afk && istown(&p_ptr->wpos)) && p_ptr->lev > cfg.newbies_cannot_drop && p_ptr->lev >= BB_INFECT_MINLEV)
	{
		s_printf("EFFECT: BLACK-BREATH - %s was infected by %s\n", p_ptr->name, q_ptr->name);
		set_black_breath(Ind);
	}
}

/*
 * Hack -- Check for a "motion blocker" (see below)
 */
int see_wall(int Ind, int dir, int y, int x)
{
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);

	/* Get the new location */
	y += ddy[dir];
	x += ddx[dir];

	/* Illegal grids are blank */
	/* XXX this should be blocked by permawalls, hopefully. */
	/* Had a crash occuring in cave_floor_bold check, y = 1, x = -1, 32,32,-500
	   So I'm trying an ugly hack - C. Blue */
//	if (!in_bounds2(wpos, y, x)) { /* RUNNING_FIX_DEBUG */
	if (!in_bounds_array(y, x)) {
		/* Hack be sure the player is inbounds */
		if (p_ptr->px < 0) p_ptr->px = 0;
		if (p_ptr->py < 0) p_ptr->py = 0;
		if (p_ptr->px >= MAX_WID) p_ptr->px = MAX_WID - 1;
		if (p_ptr->py >= MAX_HGT) p_ptr->py = MAX_HGT - 1;
		/* Update the location's player index */
		zcave[p_ptr->py][p_ptr->px].m_idx = 0 - Ind;
		cave_midx_debug(wpos, p_ptr->py, p_ptr->px, -Ind);
		return (FALSE);
	}

	/* Ghosts run right through everything */
	if ((p_ptr->ghost || p_ptr->tim_wraith)) return (FALSE);

#if 0
	/* Do wilderness hack, keep running from one outside level to another */
	if ((!in_bounds(y, x)) && (wpos->wz==0)) return FALSE;
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
	if (zcave[y][x].feat == FEAT_PERM_CLEAR) return (FALSE); /* here goes part 1.. */
	/* appearently run_init() works ok, ie treats them as open area thanks to calling see_wall().
	   checking run_test() now.. done. Added FEAT_PERM_CLEAR checks there too. Seems working fine now!
	   (Appearently those grids aren't CAVE_MARK'ed.) */
#endif

#if 1 //def NEW_RUNNING_FEAT
	/* don't accept trees as open area? */
	if (p_ptr->running_on_floor && (zcave[y][x].feat == FEAT_DEAD_TREE || zcave[y][x].feat == FEAT_TREE || zcave[y][x].feat == FEAT_BUSH)) return(TRUE);
#endif

	/* Must actually block motion */
	if (cave_floor_bold(zcave, y, x)) return (FALSE);

	if (f_info[zcave[y][x].feat].flags1 & FF1_CAN_RUN) return (FALSE);

#if 1 /* NEW_RUNNING_FEAT */
	/* hack - allow 'running' when flying over something */
	if ((f_info[zcave[y][x].feat].flags1 & (FF1_CAN_FLY | FF1_CAN_RUN)) && p_ptr->fly) return (FALSE);
	/* hack - allow 'running' if player may pass trees  */
	if ((zcave[y][x].feat == FEAT_DEAD_TREE || zcave[y][x].feat == FEAT_TREE || zcave[y][x].feat == FEAT_BUSH)
	     && p_ptr->pass_trees) return (FALSE);
	/* hack - allow 'running' if player can swim - HARDCODED :( */
	if ((zcave[y][x].feat == 84 || zcave[y][x].feat == 103 || zcave[y][x].feat == 174 || zcave[y][x].feat == 187)
	     && p_ptr->can_swim) return (FALSE);
#endif
	/* Must be known to the player */
	if (!(p_ptr->cave_flag[y][x] & CAVE_MARK)) return (FALSE);

	/* Default */
	return (TRUE);
}


/*
 * Hack -- Check for an "unknown corner" (see below)
 */
static int see_nothing(int dir, int Ind, int y, int x)
{
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);

	/* Get the new location */
	y += ddy[dir];
	x += ddx[dir];

	/* Illegal grids are unknown */
	if (!in_bounds2(wpos, y, x)) return (TRUE);

	/* Memorized grids are known */
	if (p_ptr->cave_flag[y][x] & CAVE_MARK) return (FALSE);

	/* Non-floor grids are unknown */
	if (!cave_floor_bold(zcave, y, x)) return (TRUE);

	/* Viewable grids are known */
	if (player_can_see_bold(Ind, y, x)) return (FALSE);

	/* Default */
	return (TRUE);
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
static void run_init(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];

	int             row, col, deepleft, deepright;
	int             i, shortleft, shortright;

	cave_type **zcave;
	if(!(zcave = getcave(&p_ptr->wpos))) return;

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
	if (see_wall(Ind, cycle[i+1], p_ptr->py, p_ptr->px))
	{
		/* if in the dungeon */
//		if (p_ptr->wpos.wz)
		{
			p_ptr->find_breakleft = TRUE;
			shortleft = TRUE;
		}
	}
	else if (see_wall(Ind, cycle[i+1], row, col))
	{
		/* if in the dungeon */
//		if (p_ptr->wpos.wz)
		{
			p_ptr->find_breakleft = TRUE;
			deepleft = TRUE;
		}
	}

	/* Check for walls */   
	if (see_wall(Ind, cycle[i-1], p_ptr->py, p_ptr->px))
	{
		/* if in the dungeon */
//		if (p_ptr->wpos.wz)
		{
			p_ptr->find_breakright = TRUE;
			shortright = TRUE;
		}
	}
	else if (see_wall(Ind, cycle[i-1], row, col))
	{
		/* if in the dungeon */
//		if (p_ptr->wpos.wz)
		{
			p_ptr->find_breakright = TRUE;
			deepright = TRUE;
		}
	}

	if (p_ptr->find_breakleft && p_ptr->find_breakright)
	{
		/* Not looking for open area */
		/* In the town/wilderness, always in an open area */
//		if (p_ptr->wpos.wz)
			p_ptr->find_openarea = FALSE;   

		/* Hack -- allow angled corridor entry */
		if (dir & 0x01)
		{
			if (deepleft && !deepright)
			{
				p_ptr->find_prevdir = cycle[i - 1];
			}
			else if (deepright && !deepleft)
			{
				p_ptr->find_prevdir = cycle[i + 1];
			}
		}

		/* Hack -- allow blunt corridor entry */
		else if (see_wall(Ind, cycle[i], row, col))
		{
			if (shortleft && !shortright)
			{
				p_ptr->find_prevdir = cycle[i - 2];
			}
			else if (shortright && !shortleft)
			{
				p_ptr->find_prevdir = cycle[i + 2];
			}
		}
	}
}


/*
 * Update the current "run" path
 *
 * Return TRUE if the running should be stopped
 */
/* TODO: aquatics should stop when next to non-water */
static bool run_test(int Ind)
{
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;

	int                     prev_dir, new_dir, check_dir = 0;

	int                     row, col;
	int                     i, max, inv;
	int                     option, option2;
	bool aqua = p_ptr->can_swim || p_ptr->fly || (get_skill_scale(p_ptr, SKILL_SWIM, 500) >= 7) ||
	    ((p_ptr->body_monster) && (
	    (r_info[p_ptr->body_monster].flags7 & RF7_AQUATIC) ||
	    (r_info[p_ptr->body_monster].flags3 & RF3_UNDEAD) ));

	cave_type               *c_ptr;
	byte                    *w_ptr;
	cave_type **zcave;
	struct c_special *cs_ptr;
	if(!(zcave = getcave(wpos))) return(FALSE);

	/* XXX -- Ghosts never stop running */
	if (p_ptr->ghost || p_ptr->tim_wraith) return (FALSE);

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
//		if ((!in_bounds(row, col)) && (wpos->wz==0)) continue; /* FIX_RUNNING_DEBUG_WILDTRANSITION */

		c_ptr = &zcave[row][col];
		w_ptr = &p_ptr->cave_flag[row][col];

		/* unlit grids abort running */
		if (!(c_ptr->info & (CAVE_LITE | CAVE_GLOW))) {
			if (!p_ptr->warning_run_lite && p_ptr->lev <= 10) {
				msg_print(Ind, "\374\377yHINT: You cannot run in the dark. Press 'w' to equip a light source!");
				p_ptr->warning_run_lite = TRUE;
				s_printf("warning_run_lite: %s\n", p_ptr->name);
			}
			return TRUE;
		}

		/* Visible monsters abort running */
		if (c_ptr->m_idx > 0 &&
		    /* Pets don't interrupt running */
		    (!(m_list[c_ptr->m_idx].pet))
		    /* Even in Bree (despite of Santa Claus: Rogues in cloaking mode might want to not 'run him over' but wait for allies) - C. Blue */
		    ) {
			/* Visible monster */
			if (p_ptr->mon_vis[c_ptr->m_idx] &&
			   (!(m_list[c_ptr->m_idx].special) && 
			   r_info[m_list[c_ptr->m_idx].r_idx].level != 0))
					return (TRUE);

		}

#ifdef HOSTILITY_ABORTS_RUNNING /* pvp mode chars cannot run with this on */
		/* Hostile characters will stop each other from running.
		 * This should lessen the melee storming effect in PVP fights */
		if (c_ptr->m_idx < 0 && check_hostile(Ind, 0 - c_ptr->m_idx)) return (TRUE);
#endif

		/* Visible objects abort running */
		if (c_ptr->o_idx)
		{
			/* Visible object */
			if (p_ptr->obj_vis[c_ptr->o_idx]) return (TRUE);
		}

		/* Visible traps abort running */
		if((cs_ptr=GetCS(c_ptr, CS_TRAPS)) && cs_ptr->sc.trap.found) return TRUE;

		/* Hack -- basically stop in water */
		if (c_ptr->feat == FEAT_DEEP_WATER && !aqua)
			return TRUE;

		/* Assume unknown */
		inv = TRUE;

		/* Check memorized grids */
		if (*w_ptr & CAVE_MARK)
		{
			bool notice = TRUE;

			/* Examine the terrain */
			switch (c_ptr->feat)
			{
#if 0
				/* Floors */
				case FEAT_FLOOR:

				/* Invis traps */
//				case FEAT_INVIS:

				/* Secret doors */
				case FEAT_SECRET:

				/* Normal veins */
				case FEAT_MAGMA:
				case FEAT_QUARTZ:

				/* Hidden treasure */
				case FEAT_MAGMA_H:
				case FEAT_QUARTZ_H:

				/* Grass, trees, and dirt */
				case FEAT_GRASS:
				case FEAT_TREE:
				case FEAT_DIRT:

				/* Walls */
				case FEAT_WALL_EXTRA:
				case FEAT_WALL_INNER:
				case FEAT_WALL_OUTER:
				case FEAT_WALL_SOLID:
				case FEAT_PERM_EXTRA:
				case FEAT_PERM_INNER:
				case FEAT_PERM_OUTER:
				case FEAT_PERM_SOLID:
				case FEAT_PERM_CLEAR:
				{
					/* Ignore */
					notice = FALSE;

					/* Done */
					break;
				}
#endif	// 0

				/* FIXME: this can be funny with running speed boost */
				case FEAT_DEEP_LAVA:
				case FEAT_SHAL_LAVA:
				{
					/* Ignore */
					if (p_ptr->invuln || p_ptr->immune_fire) notice = FALSE;

					/* Done */
					break;
				}

#if 0
				case FEAT_DEEP_WATER:
				{
					/* Ignore */
					if (aqua) notice = FALSE;

					/* Done */
					break;
				}
#endif	// 0
				case FEAT_ICE:
				{
					/* Ignore */
					if (p_ptr->feather_fall || p_ptr->fly) notice = FALSE;

					/* Done */
					break;
				}

				/* Open doors */
				case FEAT_OPEN:
				case FEAT_BROKEN:
				{
					/* Option -- ignore */
					if (p_ptr->find_ignore_doors) notice = FALSE;

					/* Done */
					break;
				}

				/* Stairs */
				case FEAT_LESS:
				case FEAT_MORE:
				case FEAT_WAY_LESS:
				case FEAT_WAY_MORE:
				case FEAT_SHAFT_UP:
				case FEAT_SHAFT_DOWN:
				/* XXX */
				case FEAT_BETWEEN:
				case FEAT_BETWEEN2:
				{
					/* Option -- ignore */
					if (p_ptr->find_ignore_stairs) notice = FALSE;

					/* Done */
					break;
				}

				/* Water */
				case FEAT_DEEP_WATER:
				{
					if (aqua) notice = FALSE;

					/* Done */
					break;
				}
				
				/* allow to run through wilderness transitions - C. Blue (compare see_wall comments) */
				case FEAT_PERM_CLEAR:
					notice = FALSE;
					break;
			}

			/* Check the "don't notice running" flag */
			if (f_info[c_ptr->feat].flags1 & FF1_DONT_NOTICE_RUNNING)
			{
				notice = FALSE;
			}

			/* Interesting feature */
			if (notice) return (TRUE);

			/* The grid is "visible" */
			inv = FALSE;
		}

		/* Analyze unknown grids and floors */
		/* wilderness hack to run from one level to the next */
		if (inv || ((!in_bounds(row, col)) && (wpos->wz==0))  || 
		    (cave_running_bold(p_ptr, zcave, row, col)
		    /* If player is running on floor grids right now, don't treat tree grids as "passable" even if he could pass them: */
		    && !(cave_running_bold_notrees(p_ptr, zcave, p_ptr->py, p_ptr->px)
			&& cave_running_bold_trees(p_ptr, zcave, row, col)) )
		    
		    )
		{
			/* Looking for open area */
			if (p_ptr->find_openarea)
			{
				/* Nothing */
			}

			/* The first new direction. */
			else if (!option)
			{
				option = new_dir;
			}

			/* Three new directions. Stop running. */
			else if (option2)
			{
				return (TRUE);
			}

			/* Two non-adjacent new directions.  Stop running. */
			else if (option != cycle[chome[prev_dir] + i - 1])
			{
				return (TRUE);
			}

			/* Two new (adjacent) directions (case 1) */
			else if (new_dir & 0x01)
			{
				check_dir = cycle[chome[prev_dir] + i - 2];
				option2 = new_dir;
			}

			/* Two new (adjacent) directions (case 2) */
			else
			{
				check_dir = cycle[chome[prev_dir] + i + 1];
				option2 = option;
				option = new_dir;
			}
		}

		/* Obstacle, while looking for open area */
		/* When in the town/wilderness, don't break left/right. */
		else
		{
			if (p_ptr->find_openarea)
			{
				if (i < 0)
				{
					/* Break to the right */
//					if (p_ptr->wpos.wz)
						p_ptr->find_breakright = (TRUE);
				}

				else if (i > 0)
				{
					/* Break to the left */
//					if (p_ptr->wpos.wz)
						p_ptr->find_breakleft = (TRUE);
				}
			}
		}
	}


	/* Looking for open area */
	if (p_ptr->find_openarea)
	{
		/* Hack -- look again */
		for (i = -max; i < 0; i++)
		{
			new_dir = cycle[chome[prev_dir] + i];

			row = p_ptr->py + ddy[new_dir];
			col = p_ptr->px + ddx[new_dir];

//			if ((!in_bounds(row, col)) && (wpos->wz==0)) continue; /* FIX_RUNNING_DEBUG_WILDTRANSITION */

			/* Unknown grid or floor */
			if  (!(p_ptr->cave_flag[row][col] & CAVE_MARK) ||
			    (zcave[row][col].feat == FEAT_PERM_CLEAR) || /* allow running next to level border - C. Blue */
			    (cave_running_bold(p_ptr, zcave, row, col)
			    /* If player is running on floor grids right now, don't treat tree grids as "passable" even if he could pass them: */
			    && !(cave_running_bold_notrees(p_ptr, zcave, p_ptr->py, p_ptr->px)
			       && cave_running_bold_trees(p_ptr, zcave, row, col)) )
			    )
			{
				/* Looking to break right */
				if (p_ptr->find_breakright)
				{                                                                       
					return (TRUE);
				}
			}

			/* Obstacle */
			else
			{
				/* Looking to break left */
				if (p_ptr->find_breakleft)
				{                                       
					return (TRUE);
				}
			}
		}

		/* Hack -- look again */
		for (i = max; i > 0; i--)
		{
			new_dir = cycle[chome[prev_dir] + i];

			row = p_ptr->py + ddy[new_dir];
			col = p_ptr->px + ddx[new_dir];

//			if ((!in_bounds(row, col)) && (wpos->wz==0)) continue; /* FIX_RUNNING_DEBUG_WILDTRANSITION */

			/* Unknown grid or floor */
			if (!(p_ptr->cave_flag[row][col] & CAVE_MARK) ||
			    (zcave[row][col].feat == FEAT_PERM_CLEAR) || /* allow running next to level border - C. Blue */
			    (cave_running_bold(p_ptr, zcave, row, col)
			    /* If player is running on floor grids right now, don't treat tree grids as "passable" even if he could pass them: */
			    && !(cave_running_bold_notrees(p_ptr, zcave, p_ptr->py, p_ptr->px)
			       && cave_running_bold_trees(p_ptr, zcave, row, col)) )
			    )
			{
				/* Looking to break left */
				if (p_ptr->find_breakleft)
				{                               
					return (TRUE);
				}
			}

			/* Obstacle */
			else
			{
				/* Looking to break right */
				if (p_ptr->find_breakright)
				{
					return (TRUE);
				}
			}
		}
	}


	/* Not looking for open area */
	else
	{
		/* No options */
		if (!option)
		{
			return (TRUE);
		}

		/* One option */
		else if (!option2)
		{
			/* Primary option */
			p_ptr->find_current = option;

			/* No other options */
			p_ptr->find_prevdir = option;
		}

		/* Two options, examining corners */
		else if (p_ptr->find_examine && !p_ptr->find_cut)
		{
			/* Primary option */
			p_ptr->find_current = option;

			/* Hack -- allow curving */
			p_ptr->find_prevdir = option2;
		}

		/* Two options, pick one */
		else
		{
			/* Get next location */
			row = p_ptr->py + ddy[option];
			col = p_ptr->px + ddx[option];

			/* Don't see that it is closed off. */
			/* This could be a potential corner or an intersection. */
			if (!see_wall(Ind, option, row, col) ||
			    !see_wall(Ind, check_dir, row, col))
			{
				/* Can not see anything ahead and in the direction we */
				/* are turning, assume that it is a potential corner. */
				if (p_ptr->find_examine &&
				    see_nothing(option, Ind, row, col) &&
				    see_nothing(option2, Ind, row, col))
				{
					p_ptr->find_current = option;
					p_ptr->find_prevdir = option2;
				}

				/* STOP: we are next to an intersection or a room */
				else
				{
					return (TRUE);
				}
			}

			/* This corner is seen to be enclosed; we cut the corner. */
			else if (p_ptr->find_cut)
			{
				p_ptr->find_current = option2;
				p_ptr->find_prevdir = option2;
			}

			/* This corner is seen to be enclosed, and we */
			/* deliberately go the long way. */
			else
			{
				p_ptr->find_current = option;
				p_ptr->find_prevdir = option2;
			}
		}
	}


	/* About to hit a known wall, stop */
	if (see_wall(Ind, p_ptr->find_current, p_ptr->py, p_ptr->px))
	{
		return (TRUE);
	}


	/* Failure */
	return (FALSE);
}



/*
 * Take one step along the current "run" path
 */
void run_step(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];
	int prev_dir;

	/* slower 'running' movement over certain terrain */
	int real_speed = cfg.running_speed;
	cave_type *c_ptr, **zcave;
	if(!(zcave = getcave(&p_ptr->wpos))) return;
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
		if (p_ptr->energy < level_speed(&p_ptr->wpos) / real_speed)
			return;
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

		p_ptr->warning_run = 1;

		/* C. Blue fun stuff =p */
		if (prev_dir != p_ptr->find_current) p_ptr->corner_turn++;
	}

	/* Decrease the run counter */
	if (--(p_ptr->running) <= 0) return;

	/* Move the player, using the "pickup" flag */
	move_player(Ind, p_ptr->find_current, p_ptr->always_pickup);
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

	if (Players[Ind]->paralyzed || Players[Ind]->stun >= 100) return(0);

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
//	chance = (dodge * plev) / (attack_level * 3); /* 50vs50 -> 33%, 75vs75 -> 31%, 99vs100 -> 30%, 50vs100 -> 25%, 63 vs 127 -> 23%, 80 vs 50 -> 43% */
	chance = (dodge * plev * 2) / (attack_level * 5); /* 50vs50 -> 40%, 75vs75 -> 37%, 99vs100 -> 36%, 50vs100 -> 30%, 63 vs 127 -> 29%, 80 vs 50 -> 52% */
	/* with new lower-limit: 40vs20 -> 59%, 50vs50 -> 40%, 75vs75 -> 37%, 99vs100 -> 36%, 50vs100 -> 30%, 63 vs 127 -> 29%, 80 vs 50 -> 43% */

#if 1 /* instead of capping... */
	if (chance > DODGE_MAX_CHANCE) chance = DODGE_MAX_CHANCE;
#else /* ...let it scale? >:) */
        chance = (chance * DODGE_MAX_CHANCE) / 100;
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
	if (!p_ptr->shield_deflect || n <= 0 || p_ptr->paralyzed || p_ptr->stun >= 100) return(0);
	if (p_ptr->confused) n = (n * 5) / 10;
	if (p_ptr->blind) n = (n * 3) / 10;
	if (p_ptr->image) n = (n * 5) / 10;
	if (p_ptr->stun) n = (n * 7) / 10;
	if (p_ptr->stun > 50) n = (n * 7) / 10;
	if (!n) n = 1;
	return (n);
}

int apply_parry_chance(player_type *p_ptr, int n) { /* n can already be modified chance */
	if (!p_ptr->weapon_parry || n <= 0 || p_ptr->paralyzed || p_ptr->stun >= 100) return(0);
	if (p_ptr->confused) n = (n * 6) / 10;
	if (p_ptr->blind) n = (n * 1) / 10;
	if (p_ptr->image) n = (n * 7) / 10;
	if (p_ptr->stun) n = (n * 7) / 10;
	if (p_ptr->stun > 50) n = (n * 7) / 10;
	if (!n) n = 1;
	return (n);
}
