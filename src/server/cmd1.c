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

#define MAX_VAMPIRIC_DRAIN 100

/*
 * Allow wraith-formed player to pass through permawalls on the surface.
 */
/*
 * TODO: wraithes should only pass townwalls of her/his own house
 */
#define WRAITH_THROUGH_TOWNWALL




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
bool test_hit_norm(int chance, int ac, int vis)
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
//	player_type *p_ptr = Players[Ind];

	int i, k;

	/* Extract "shot" power */
	if (Ind > 0)
	{
		player_type *p_ptr = Players[Ind];

		i = (weight + ((p_ptr->to_h + plus) * 4) +
				get_skill_scale(p_ptr, SKILL_ARCHERY, 100));
		i += 50 * p_ptr->xtra_crit;
	}
	else i = weight;

	/* Critical hit */
	if (randint(5000) <= i)
	{
		k = weight + randint(500);

		if (k < 500)
		{
			msg_print(Ind, "It was a good hit!");
			dam = 2 * dam + 5;
		}
		else if (k < 1000)
		{
			msg_print(Ind, "It was a great hit!");
			dam = 2 * dam + 10;
		}
		else
		{
			msg_print(Ind, "It was a superb hit!");
			dam = 3 * dam + 15;
		}
	}

	return (dam);
}



/*
 * Critical hits (by player)
 *
 * Factor in weapon weight, total plusses, player level.
 */
s16b critical_norm(int Ind, int weight, int plus, int dam, bool allow_skill_crit)
{
	player_type *p_ptr = Players[Ind];

	int i, k, w;
	
	/* Critical hits for rogues says 'with light swords'
	in the skill description, which is logical since you
	cannot maneuver a heavy weapon so that it pierces through
	the small weak spot of an opponent's armour, hitting exactly
	into his artery or sth. So #if 0.. */

#if 0
	/* Extract "blow" power */
	i = (weight + ((p_ptr->to_h + plus) * 5) +
                 get_skill_scale(p_ptr, SKILL_MASTERY, 150));
        i += 50 * p_ptr->xtra_crit;
#else
	/* Extract critical maneuver potential (interesting term..) */
	/* The larger the weapon the more difficult to quickly strike
	the critical spot. Cap weight influence at 100+ lbs */
	w = weight;
	if (w > 100) w = 10;
	else w = 110 - w;
	if (w < 10) w = 10; /* shouldn't happen anyways */
	i = (w * 2) + ((p_ptr->to_h + plus) * 5) + get_skill_scale(p_ptr, SKILL_MASTERY, 150);
#endif

        if (allow_skill_crit)
        {
                i += get_skill_scale(p_ptr, SKILL_CRITS, 40 * 50);
        }

	/* Chance */
	if (randint(5000) <= i)
	{
		/* _If_ a critical hit is scored then it will deal
		more damage if the weapon is heavier */
		k = weight + randint(700);
                if (allow_skill_crit)
                {
                        k += get_skill_scale(p_ptr, SKILL_CRITS, 700);
                }

		if (k < 400)
		{
			msg_print(Ind, "It was a good hit!");
			dam = ((3 * dam) / 2) + 5;
		}
		else if (k < 700)
		{
			msg_print(Ind, "It was a great hit!");
			dam = 2 * dam + 10;
		}
		else if (k < 900)
		{
			msg_print(Ind, "It was a superb hit!");
			dam = ((5 * dam) / 2) + 15;
		}
		else if (k < 1300)
		{
			msg_print(Ind, "It was a *GREAT* hit!");
			dam = 3 * dam + 20;
		}
		else
		{
			msg_print(Ind, "It was a *SUPERB* hit!");
			dam = ((7 * dam) / 2) + 25;
		}
	}

	return (dam);
}



/*
 * Extract the "total damage" from a given object hitting a given monster.
 *
 * Note that "flasks of oil" do NOT do fire damage, although they
 * certainly could be made to do so.  XXX XXX
 *
 * Note that most brands and slays are x3, except Slay Animal (x2),
 * Slay Evil (x2), and Kill dragon (x5).
 */
/* accepts Ind <=0 */
s16b tot_dam_aux(int Ind, object_type *o_ptr, int tdam, monster_type *m_ptr, char *brand_msg)
{
//	player_type *p_ptr = Players[Ind];

	int mult = 1;

	monster_race *r_ptr = race_inf(m_ptr);

	u32b f1, f2, f3, f4, f5, esp;
	player_type *p_ptr = Players[Ind];

	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	cave_type *c_ptr;
	char m_name[80];

	object_type *e_ptr;
	u32b ef1, ef2, ef3, ef4, ef5, eesp;
	int brands_total, brand_msgs_added;
	/* char brand_msg[80];*/

	monster_race *pr_ptr = &r_info[p_ptr->body_monster];
	bool apply_monster_brands = TRUE;
	int i, monster_brands = 0;
	u32b monster_brand[6], monster_brand_chosen;
	monster_brand[1] = 0;
	monster_brand[2] = 0;
	monster_brand[3] = 0;
	monster_brand[4] = 0;
	monster_brand[5] = 0;

	if(!(zcave=getcave(wpos))) return(tdam);
	c_ptr=&zcave[m_ptr->fy][m_ptr->fx];

	/* Extract monster name (or "it") */
	monster_desc(Ind, m_name, c_ptr->m_idx, 0);

	/* Extract the flags */
	if (o_ptr->k_idx)
	{
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

		/* Hack -- extract temp branding */
		if (Ind > 0)
		{
			if (p_ptr->bow_brand)
			{
	
				switch (p_ptr->bow_brand_t)
				{
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
	}
	else
	{
		f1 = 0; f2 = 0; f3 = 0; f4 = 0; f5 = 0;
	}


	/* Apply brands from mimic monster forms */
        if (p_ptr->body_monster)
	{
#if 0
		switch (pr_ptr->r_ptr->d_char)
		{
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
		if ((!pr_ptr->flags4 & RF4_ARROW_1) &&
		    ((o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_BOLT)))
			apply_monster_brands = FALSE;
		/* If monster is fighting with a weapon, the player gets the brand(s) even with a weapon */
        	/* If monster is fighting without weapons, the player gets the brand(s) only if
		he fights with bare hands/martial arts */
		/* However, if the monster doesn't use weapons but nevertheless fires ammo, the player
		gets the brand(s) on ranged attacks */
		if ((!pr_ptr->body_parts[BODY_WEAPON]) &&
		    (!((o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_BOLT))))
			if (o_ptr->k_idx) apply_monster_brands = FALSE;
#endif
		/* The player never gets brands on ranged attacks from a form */
		if ((o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_BOLT))
			apply_monster_brands = FALSE;
		/* The player doesn't get brands if he uses a weapon but the monster doesn't */
		if ((o_ptr->k_idx) && (!pr_ptr->body_parts[BODY_WEAPON]))
			apply_monster_brands = FALSE;

		/* Get monster brands. If monster has several, choose one randomly */
		for (i = 0; i < 4; i++)
		{
		        if (pr_ptr->blow[i].d_dice * pr_ptr->blow[i].d_side)
			{
				switch (pr_ptr->blow[i].effect)
				{
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
        for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
        {
                e_ptr = &p_ptr->inventory[i];
                /* k_ptr = &k_info[e_ptr->k_idx];
                pval = e_ptr->pval; not needed */
	        /* Skip missing items */
        	if (!e_ptr->k_idx) continue;
	        /* Extract the item flags */
                object_flags(e_ptr, &ef1, &ef2, &ef3, &ef4, &ef5, &eesp);

		/* Weapon/Bow/Ammo/Tool brands don't have general effect on all attacks */
		/* All other items have general effect! */
		if ((i != INVEN_WIELD) && (i != INVEN_AMMO) && (i != INVEN_TOOL)) f1 |= ef1;

		/* Add bow branding on correct ammo types */
		if(i==INVEN_BOW && e_ptr->tval==TV_BOW){
			if(( (e_ptr->sval==SV_SHORT_BOW || e_ptr->sval==SV_LONG_BOW) && o_ptr->tval==TV_ARROW) ||
			   ( (e_ptr->sval==SV_LIGHT_XBOW || e_ptr->sval==SV_HEAVY_XBOW) && o_ptr->tval==TV_BOLT) ||
			   (e_ptr->sval==SV_SLING && o_ptr->tval==TV_SHOT))
			   f1|=ef1;
		}
	}

	/* Temporary weapon branding */
	if(p_ptr->brand && p_ptr->inventory[INVEN_WIELD].k_idx){
		if (!((o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_BOLT))){
			if(p_ptr->brand){
				switch (p_ptr->brand_t)
				{
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
	strcpy(brand_msg,m_name);
	strcat(brand_msg," is ");//"%^s is ");
	switch (brands_total)
	{
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
			if (!(brand_msg == ""))
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"frost");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_ELEC)
		{
			if (!(brand_msg == ""))
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"electricity");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_FIRE)
		{
			if (!(brand_msg == ""))
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"flames");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_POIS)
		{
			if (!(brand_msg == ""))
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
		//msg_format(Ind, brand_msg, m_name);
	}
	else strcpy(brand_msg,"");
#endif
	/* Some "weapons" and "ammo" do extra damage */
	switch (o_ptr->tval)
	{
/*		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
		case TV_BOOMERANG:*/
		default:
		{
			/* Slay Animal */
			if ((f1 & TR1_SLAY_ANIMAL) &&
			    (r_ptr->flags3 & RF3_ANIMAL))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_ANIMAL;*/

				if (mult < 2) mult = 2;
			}

			/* Slay Evil */
			if ((f1 & TR1_SLAY_EVIL) &&
			    (r_ptr->flags3 & RF3_EVIL))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_EVIL;*/

				if (mult < 2) mult = 2;
			}

			/* Slay Undead */
			if ((f1 & TR1_SLAY_UNDEAD) &&
			    (r_ptr->flags3 & RF3_UNDEAD))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_UNDEAD;*/

				if (mult < 3) mult = 3;
			}

			/* Slay Demon */
			if ((f1 & TR1_SLAY_DEMON) &&
			    (r_ptr->flags3 & RF3_DEMON))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DEMON;*/

				if (mult < 3) mult = 3;
			}

			/* Slay Orc */
			if ((f1 & TR1_SLAY_ORC) &&
			    (r_ptr->flags3 & RF3_ORC))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_ORC;*/

				if (mult < 3) mult = 3;
			}

			/* Slay Troll */
			if ((f1 & TR1_SLAY_TROLL) &&
			    (r_ptr->flags3 & RF3_TROLL))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_TROLL;*/

				if (mult < 3) mult = 3;
			}

			/* Slay Giant */
			if ((f1 & TR1_SLAY_GIANT) &&
			    (r_ptr->flags3 & RF3_GIANT))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_GIANT;*/

				if (mult < 3) mult = 3;
			}

			/* Slay Dragon  */
			if ((f1 & TR1_SLAY_DRAGON) &&
			    (r_ptr->flags3 & RF3_DRAGON))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DRAGON;*/

				if (mult < 3) mult = 3;
			}

			/* Execute Dragon */
			if ((f1 & TR1_KILL_DRAGON) &&
			    (r_ptr->flags3 & RF3_DRAGON))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DRAGON;*/

				if (mult < 5) mult = 5;
			}

			/* Execute Undead */
			if ((f5 & TR5_KILL_UNDEAD) &&
			    (r_ptr->flags3 & RF3_UNDEAD))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_UNDEAD;*/

				if (mult < 5) mult = 5;
			}

			/* Execute Undead */
			if ((f5 & TR5_KILL_DEMON) &&
			    (r_ptr->flags3 & RF3_DEMON))
			{
				/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_DEMON;*/

				if (mult < 5) mult = 5;
			}


			/* Brand (Acid) */
			if (f1 & TR1_BRAND_ACID)
			{
				/* Notice immunity */
				if (r_ptr->flags3 & RF3_IM_ACID)
				{
					/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_ACID;*/
				}
				/* Notice susceptibility */
				else if (r_ptr->flags9 & (RF9_SUSCEP_ACID))
				{
#if 0
					if (m_ptr->ml)
					{
						r_ptr->r_flags9 |= (RF9_SUSCEP_ACID);
					}
#endif	// 0
					if (mult < 6) mult = 6;
				}
				else if (r_ptr->flags9 & RF9_RES_ACID)
				{
				    if (mult < 2) mult = 2;
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}

			/* Brand (Elec) */
			if (f1 & TR1_BRAND_ELEC)
			{
				/* Notice immunity */
				if (r_ptr->flags3 & RF3_IM_ELEC)
				{
					/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_ELEC;*/
				}

				/* Notice susceptibility */
				else if (r_ptr->flags9 & (RF9_SUSCEP_ELEC))
				{
#if 0
					if (m_ptr->ml)
					{
						r_ptr->r_flags9 |= (RF9_SUSCEP_ELEC);
					}
#endif	// 0
					if (mult < 6) mult = 6;
				}
				else if (r_ptr->flags9 & RF9_RES_ELEC)
				{
				    if (mult < 2) mult = 2;
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}

			/* Brand (Fire) */
			if (f1 & TR1_BRAND_FIRE)
			{
				/* Notice immunity */
				if (r_ptr->flags3 & RF3_IM_FIRE)
				{
					/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_FIRE;*/
				}
				/* Notice susceptibility */
				else if (r_ptr->flags3 & (RF3_SUSCEP_FIRE))
				{
#if 0
					if (m_ptr->ml)
					{
						r_ptr->r_flags3 |= (RF3_SUSCEP_FIRE);
					}
#endif	// 0
					if (mult < 6) mult = 6;
				}
				else if (r_ptr->flags9 & RF9_RES_FIRE)
				{
				    if (mult < 2) mult = 2;
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}

			/* Brand (Cold) */
			if (f1 & TR1_BRAND_COLD)
			{
				/* Notice immunity */
				if (r_ptr->flags3 & RF3_IM_COLD)
				{
					/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_COLD;*/
				}
				/* Notice susceptibility */
				else if (r_ptr->flags3 & (RF3_SUSCEP_COLD))
				{
#if 0
					if (m_ptr->ml)
					{
						r_ptr->r_flags3 |= (RF3_SUSCEP_COLD);
					}
#endif	// 0
					if (mult < 6) mult = 6;
				}
				else if (r_ptr->flags9 & RF9_RES_COLD)
				{
				    if (mult < 2) mult = 2;
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}


			/* Brand (Pois) */
			if (f1 & TR1_BRAND_POIS)
			{
				/* Notice immunity */
				if (r_ptr->flags3 & RF3_IM_POIS)
				{
					/*if (m_ptr->ml) r_ptr->r_flags3 |= RF3_IM_POIS;*/
				}

				/* Notice susceptibility */
				else if (r_ptr->flags9 & (RF9_SUSCEP_POIS))
				{
#if 0
					if (m_ptr->ml)
					{
						r_ptr->r_flags9 |= (RF9_SUSCEP_POIS);
					}
#endif	// 0
					if (mult < 6) mult = 6;
//					if (magik(95)) *special |= SPEC_POIS;
				}
				else if (r_ptr->flags9 & RF9_RES_POIS)
				{
				    if (mult < 2) mult = 2;
				}

				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
				}
			}

			break;
		}
	}


	/* Return the total damage */
	return (tdam * mult);
}

/*
 * Extract the "total damage" from a given object hitting a given player.
 *
 * Note that "flasks of oil" do NOT do fire damage, although they
 * certainly could be made to do so.  XXX XXX
 */
s16b tot_dam_aux_player(int Ind, object_type *o_ptr, int tdam, player_type *q_ptr, char *brand_msg)
{
	int mult = 1;

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
	else
	{
	    f1 = 0; f2 = 0; f3 = 0; f4 = 0; f5 = 0;
	}

	/* Apply brands from mimic monster forms */
        if (p_ptr->body_monster)
	{
#if 0
		switch (pr_ptr->r_ptr->d_char)
		{
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
		if ((!pr_ptr->flags4 & RF4_ARROW_1) &&
		    ((o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_BOLT)))
			apply_monster_brands = FALSE;
		/* If monster is fighting with a weapon, the player gets the brand(s) even with a weapon */
        	/* If monster is fighting without weapons, the player gets the brand(s) only if
		he fights with bare hands/martial arts */
		/* However, if the monster doesn't use weapons but nevertheless fires ammo, the player
		gets the brand(s) on ranged attacks */
		if ((!pr_ptr->body_parts[BODY_WEAPON]) &&
		    (!((o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_BOLT))))
			if (o_ptr->k_idx) apply_monster_brands = FALSE;
#endif
		/* The player never gets brands on ranged attacks from a form */
		if ((o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_BOLT))
			apply_monster_brands = FALSE;
		/* The player doesn't get brands if he uses a weapon but the monster doesn't */
		if ((o_ptr->k_idx) && (!pr_ptr->body_parts[BODY_WEAPON]))
			apply_monster_brands = FALSE;

		/* Get monster brands. If monster has several, choose one randomly */
		for (i = 0; i < 4; i++)
		{
		        if (pr_ptr->blow[i].d_dice * pr_ptr->blow[i].d_side)
			{
				switch (pr_ptr->blow[i].effect)
				{
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
        for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
        {
                e_ptr = &p_ptr->inventory[i];
                /* k_ptr = &k_info[e_ptr->k_idx];
                pval = e_ptr->pval; not needed */
	        /* Skip missing items */
        	if (!e_ptr->k_idx) continue;
	        /* Extract the item flags */
                object_flags(e_ptr, &ef1, &ef2, &ef3, &ef4, &ef5, &eesp);

		/* Weapon/Bow/Ammo/Tool brands don't have general effect on all attacks */
		/* All other items have general effect! */
		if ((i != INVEN_WIELD) && (i != INVEN_BOW) && (i != INVEN_AMMO) && (i != INVEN_TOOL)) f1 |= ef1;
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
	strcpy(brand_msg,q_ptr->name);
	strcat(brand_msg," is ");//"%^s is ");
	switch (brands_total)
	{
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
			if (!(brand_msg == ""))
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"frost");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_ELEC)
		{
			if (!(brand_msg == ""))
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"electricity");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_FIRE)
		{
			if (!(brand_msg == ""))
			{
			    if (brand_msgs_added == (brands_total - 1)) strcat(brand_msg," and ");
			    else strcat(brand_msg,", ");
			}
			strcat(brand_msg,"flames");
			brand_msgs_added++;
		}
		if (f1 & TR1_BRAND_POIS)
		{
			if (!(brand_msg == ""))
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
		case TV_HAFTED:
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
					if (mult < 2) mult = 2;
				}
				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
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
					if (mult < 2) mult = 2;
				}
				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
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
					if (mult < 2) mult = 2;
				}
				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
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
					if (mult < 2) mult = 2;
				}
				/* Otherwise, take the damage */
				else
				{
					if (mult < 3) mult = 3;
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
					if (mult < 2) mult = 2;
				}
				/* Otherwise, take the damage */
				{
					if (mult < 3) mult = 3;
				}
			}
			break;
		}
	}


	/* Return the total damage */
	return (tdam * mult);
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
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	struct c_special *cs_ptr;
	if(!(zcave=getcave(wpos))) return;

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
static void whats_under_your_feet(int Ind)
{
	object_type *o_ptr;

	char    o_name[160];
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
	if (p_ptr->auto_id)
	{
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

	char    o_name[160];
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

	/* Get the object */
	o_ptr = &o_list[c_ptr->o_idx];

	/* Auto id ? */
	if (p_ptr->auto_id)
	{
		object_aware(Ind, o_ptr);
		object_known(o_ptr);
	}

	/* Describe the object */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	/* Pick up gold */
	if (o_ptr->tval == TV_GOLD)
	{
		/* Disturb */
		disturb(Ind, 0, 0);

		/* Message */
		msg_format(Ind, "You have found %ld gold pieces worth of %s.",
			   (long)o_ptr->pval, o_name);

		/* Collect the gold */
		p_ptr->au += o_ptr->pval;

		/* Redraw gold */
		p_ptr->redraw |= (PR_GOLD);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Take log for possible cheeze */
#if CHEEZELOG_LEVEL > 1
		if (o_ptr->owner)
		{
			cptr name = lookup_player_name(o_ptr->owner);
			if (p_ptr->id != o_ptr->owner)
				s_printf("%s Money transaction: %dau from %s to %s(lv %d)\n",
						showtime(), o_ptr->pval, name ? name : "(Dead player)",
						p_ptr->name, p_ptr->lev);
		}
#endif	// CHEEZELOG_LEVEL



		/* Delete gold */
//		delete_object(wpos, p_ptr->py, p_ptr->px);
		delete_object_idx(c_ptr->o_idx);

		/* Hack -- tell the player of the next object on the pile */
		whats_under_your_feet(Ind);
	}

	/* Pick it up */
	else
	{
		bool force_pickup = check_guard_inscription(o_ptr->note, '=')
			&& p_ptr->id == o_ptr->owner;

		/* Hack -- disturb */
		disturb(Ind, 0, 0);

		/* Describe the object */
		if (!pickup && !force_pickup)
		{
			if (p_ptr->blind || no_lite(Ind))
				msg_format(Ind, "You feel %s%s here.", o_name, 
						o_ptr->next_o_idx ? " on a pile" : "");
			else msg_format(Ind, "You see %s%s.", o_name,
						o_ptr->next_o_idx ? " on a pile" : "");
			Send_floor(Ind, o_ptr->tval);
			return;
		}
#if 1
		/* Try to add to the quiver */
		else if (object_similar(Ind, o_ptr, &p_ptr->inventory[INVEN_AMMO]))
		{
			int slot = INVEN_AMMO, num = o_ptr->number;

			msg_print(Ind, "You add the ammo to your quiver.");

#if 0
			/* Own it */
			if (!o_ptr->owner) o_ptr->owner = p_ptr->id;
			can_use(Ind, o_ptr);
#endif	// 0

			/* Get the item again */
			o_ptr = &(p_ptr->inventory[slot]);

			o_ptr->number += num;
			p_ptr->total_weight += num * o_ptr->weight;

			/* Describe the object */
			object_desc(Ind, o_name, o_ptr, TRUE, 3);
			o_ptr->marked=0;

			/* Message */
			msg_format(Ind, "You have %s (%c).", o_name, index_to_label(slot));

			/* Delete original */
//			delete_object(wpos, p_ptr->py, p_ptr->px);
			delete_object_idx(c_ptr->o_idx);

			/* Hack -- tell the player of the next object on the pile */
			whats_under_your_feet(Ind);

			/* Tell the client */
			Send_floor(Ind, 0);

			/* Refresh */
			p_ptr->window |= PW_EQUIP;
		}
		/* Try to add to the empty quiver (XXX rewrite me - too long!) */
		else if (force_pickup && !p_ptr->inventory[INVEN_AMMO].k_idx &&
				wield_slot(Ind, o_ptr) == INVEN_AMMO)
		{
			int slot = INVEN_AMMO;
			u32b f1 = 0 , f2 = 0 , f3 = 0, f4 = 0, f5 = 0, esp = 0;

			msg_print(Ind, "You add the ammo to your quiver.");

			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

			/* Auto Curse */
			if (f3 & TR3_AUTO_CURSE)
			{
				/* The object recurse itself ! */
				o_ptr->ident |= ID_CURSED;
			}

			/* Cursed! */
			if (cursed_p(o_ptr))
			{
				/* Warn the player */
				msg_print(Ind, "Oops! It feels deathly cold!");

				/* Note the curse */
				o_ptr->ident |= ID_SENSE;
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
			o_ptr->marked=0;

			/* Message */
			msg_format(Ind, "You have %s (%c).", o_name, index_to_label(slot));

			/* Delete original */
			//			delete_object(wpos, p_ptr->py, p_ptr->px);
			delete_object_idx(c_ptr->o_idx);

			/* Hack -- tell the player of the next object on the pile */
			whats_under_your_feet(Ind);

			/* Tell the client */
			Send_floor(Ind, 0);

			/* Recalculate bonuses */
			p_ptr->update |= (PU_BONUS);

			/* Window stuff */
			p_ptr->window |= (PW_EQUIP);
		}
#endif	// 0
		/* Note that the pack is too full */
		else if (!inven_carry_okay(Ind, o_ptr))
		{
			msg_format(Ind, "You have no room for %s.", o_name);
			Send_floor(Ind, o_ptr->tval);
		}

		/* Pick up the item (if requested and allowed) */
		else
		{
			int okay = TRUE;

#if 0
			/* Hack -- query every item */
			if (p_ptr->carry_query_flag && !confirm)
			{
				char out_val[160];
				sprintf(out_val, "Pick up %s? ", o_name);
				Send_pickup_check(Ind, out_val);
				return;
			}
#endif	// 0

			/* Attempt to pick up an object. */
			if (okay)
			{
				int slot;

				/* Own it */
				if (!o_ptr->owner) o_ptr->owner = p_ptr->id;

#if CHEEZELOG_LEVEL > 2
				/* Take cheezelog
				 * TODO: ignore cheap items (like cure critical pot) */
				else if (p_ptr->id != o_ptr->owner)
				{
					cptr name = lookup_player_name(o_ptr->owner);
					object_desc_store(Ind, o_name, o_ptr, TRUE, 3);
					s_printf("%s Item transaction from %s to %s(lv %d):\n  %s\n  ",
							showtime(), name ? name : "(Dead player)",
							p_ptr->name, p_ptr->lev, o_name);
				}
#endif	// CHEEZELOG_LEVEL

				can_use(Ind, o_ptr);

				/* Carry the item */
				slot = inven_carry(Ind, o_ptr);

				/* Get the item again */
				o_ptr = &(p_ptr->inventory[slot]);

#if 0
				if (!o_ptr->level)
				{
					if (p_ptr->dun_depth > 0) o_ptr->level = p_ptr->dun_depth;
					else o_ptr->level = -p_ptr->dun_depth;
					if (o_ptr->level > 100) o_ptr->level = 100;
				}
#endif

				/* Describe the object */
				object_desc(Ind, o_name, o_ptr, TRUE, 3);
				o_ptr->marked=0;

				/* Message */
				msg_format(Ind, "You have %s (%c).", o_name, index_to_label(slot));

				/* guild key? */
				if(o_ptr->tval==TV_KEY && o_ptr->sval==2){
					if(o_ptr->pval==p_ptr->guild){
						if(guilds[p_ptr->guild].master!=p_ptr->id){
							guild_msg_format(p_ptr->guild, "%s is the new guildmaster!", p_ptr->name);
							guilds[p_ptr->guild].master=p_ptr->id;
						}
					}
				}

				/* Delete original */
//				delete_object(wpos, p_ptr->py, p_ptr->px);
				delete_object_idx(c_ptr->o_idx);

				/* Hack -- tell the player of the next object on the pile */
				whats_under_your_feet(Ind);

				/* Tell the client */
				Send_floor(Ind, 0);
			}
		}
	}

	/* splash! harm equipments */
	if (c_ptr->feat == FEAT_DEEP_WATER &&
			TOOL_EQUIPPED(p_ptr) != SV_TOOL_TARPAULIN &&
//			magik(WATER_ITEM_DAMAGE_CHANCE))
			magik(3))
	{
		inven_damage(Ind, set_water_destroy, 1);
		if (magik(20)) minus_ac(Ind, 1);
	}

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
static void hit_trap(int Ind)
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

	if (t_idx)
	{
		ident = player_activate_trap_type(Ind, p_ptr->py, p_ptr->px, NULL, -1);
		if (ident && !p_ptr->trap_ident[t_idx])
		{
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
 */
/* TODO: p_ptr->name should be replaced by strings made by player_desc */
static void py_attack_player(int Ind, int y, int x, bool old)
{
	player_type *p_ptr = Players[Ind];
	int num = 0, k, bonus, chance;
	player_type *q_ptr;

	object_type *o_ptr;

	char p_name[80], brand_msg[80];

	bool do_quake = FALSE;

	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	cave_type *c_ptr;

	int fear_chance;

	monster_race *pr_ptr = &r_info[p_ptr->body_monster];
	bool apply_monster_effects = TRUE;
	int i, monster_effects;
	u32b monster_effect[6], monster_effect_chosen;
	monster_effect[1] = 0;
	monster_effect[2] = 0;
	monster_effect[3] = 0;
	monster_effect[4] = 0;
	monster_effect[5] = 0;

	if(!(zcave=getcave(wpos))) return;
	c_ptr=&zcave[y][x];
	q_ptr = Players[0 - c_ptr->m_idx];

	/* Disturb both players */
	disturb(Ind, 0, 0);
	disturb(0 - c_ptr->m_idx, 0, 0);

	/* Extract name */
//	strcpy(p_name, q_ptr->name);
	player_desc(Ind, p_name, 0 - c_ptr->m_idx, 0);

	/* Track player health */
	if (p_ptr->play_vis[0 - c_ptr->m_idx]) health_track(Ind, c_ptr->m_idx);

	/* If target isn't already hostile toward attacker, make it so */
	if (!check_hostile(0 - c_ptr->m_idx, Ind))
	{
		/* Make hostile */
		add_hostility(0 - c_ptr->m_idx, p_ptr->name);
	}
	if (cfg.use_pk_rules == PK_RULES_DECLARE)
	{
		if(!(q_ptr->pkill & PKILL_KILLABLE)){
			char string[30];
			sprintf(string, "attacking %s", q_ptr->name);
			s_printf("%s attacked defenceless %s\n", p_ptr->name, q_ptr->name);
			if(!imprison(Ind, 500, string)){
				/* This wrath can be too much */
				//			take_hit(Ind, randint(p_ptr->lev*30), "wrath of the Gods");
				/* It's prison here :) */
				msg_print(Ind, "{yYou feel yourself bound hand and foot!");
				set_paralyzed(Ind, p_ptr->paralyzed + rand_int(15) + 15);
				return;
			}
			else return;
		}
	}

	/* Hack -- divided turn for auto-retaliator */
	if (!old)
	{
		p_ptr->energy -= level_speed(&p_ptr->wpos) / p_ptr->num_blow;
	}

	/* Handle attacker fear */
	if (p_ptr->afraid)
	{
		/* Message */
		msg_format(Ind, "You are too afraid to attack %s!", p_name);

		/* Done */
		return;
	}

	if (q_ptr->store_num == 7)
	{
		/* Message */
//		msg_format(Ind, "You are too afraid to attack %s!", p_name);

		/* Done */
		return;
	}

	/* Access the weapon */
	o_ptr = &(p_ptr->inventory[INVEN_WIELD]);

	/* Calculate the "attack quality" */
	bonus = p_ptr->to_h + o_ptr->to_h + p_ptr->to_h_melee;
	chance = (p_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));


	/* Attack once for each legal blow */
	while (num++ < p_ptr->num_blow)
	{
		/* Test for hit */
		if (test_hit_norm(chance, q_ptr->ac + q_ptr->to_a, 1))
		{
			/* Sound */
			sound(Ind, SOUND_HIT);

			/* Hack -- bare hands do one damage */
			k = 1;

			/* Ghosts do damages relative to level */
			if (p_ptr->ghost)
				k = p_ptr->lev;
			if (p_ptr->fruit_bat)
				k = p_ptr->lev * ((p_ptr->lev / 10) + 1);
#if 1 // DGDGDG -- monks are no more
//			if (p_ptr->pclass == CLASS_MONK)
			if (get_skill(p_ptr, SKILL_MARTIAL_ARTS) && !o_ptr->k_idx
					&& !p_ptr->inventory[INVEN_ARM].k_idx
					&& !p_ptr->inventory[INVEN_BOW].k_idx)
			{
				int special_effect = 0, stun_effect = 0, times = 0;
				martial_arts *ma_ptr = &ma_blows[0], *old_ptr = &ma_blows[0];
				int resist_stun = 0;
				int marts = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 50);
				if (q_ptr->resist_conf) resist_stun += 44;
				if (q_ptr->free_act) resist_stun += 44;

				for (times = 0; times < (marts<7?1:marts/7); times++)
				/* Attempt 'times' */
				{
					do
					{
						ma_ptr = &ma_blows[rand_int(MAX_MA)];
					}
					while ((ma_ptr->min_level > marts)
					    || (randint(marts)<ma_ptr->chance));

					/* keep the highest level attack available we found */
					if ((ma_ptr->min_level > old_ptr->min_level) &&
					    !(p_ptr->stun || p_ptr->confused))
					{
						old_ptr = ma_ptr;
					}
					else
					{
						ma_ptr = old_ptr;
					}
				}

				k = damroll(ma_ptr->dd, ma_ptr->ds);

				if (ma_ptr->effect == MA_KNEE)
				{
					if (q_ptr->male)
					{
						msg_format(Ind, "You hit %s in the groin with your knee!", q_ptr->name);
						special_effect = MA_KNEE;
					}
					else
						msg_format(Ind, ma_ptr->desc, q_ptr->name);
				}

				else
				{
					if (ma_ptr->effect)
					{
						stun_effect = (ma_ptr->effect/2) + randint(ma_ptr->effect/2);
					}

					msg_format(Ind, ma_ptr->desc, q_ptr->name);
				}

				k = tot_dam_aux_player(Ind, o_ptr, k, q_ptr, brand_msg);
				k = critical_norm(Ind, marts * (randint(10)), ma_ptr->min_level, k, FALSE);

				if ((special_effect == MA_KNEE) && ((k + p_ptr->to_d) < q_ptr->chp))
				{
					msg_format(Ind, "%^s moans in agony!", q_ptr->name);
					stun_effect = 7 + randint(13);
					resist_stun /= 3;
				}

				if (stun_effect && ((k + p_ptr->to_d) < q_ptr->chp))
				{
					if (marts > randint((q_ptr->lev * 2) + resist_stun + 10))
					{
						msg_format(Ind, "\377o%^s is stunned.", q_ptr->name);

						set_stun(0 - c_ptr->m_idx, q_ptr->stun + stun_effect);
					}
				}
			} else
#endif	// 1 (Martial arts)
			/* Handle normal weapon */
			if (o_ptr->k_idx)
			{
				k = damroll(o_ptr->dd, o_ptr->ds);
				k = tot_dam_aux_player(Ind, o_ptr, k, q_ptr, brand_msg);
				if (p_ptr->impact && (k > 50)) do_quake = TRUE;
				k = critical_norm(Ind, o_ptr->weight, o_ptr->to_h + p_ptr->to_h_melee, k, ((o_ptr->tval == TV_SWORD) && (o_ptr->weight < 50)));
				k += o_ptr->to_d;
			}
			/* handle bare fists/bat/ghost */
			else
			{
				k = tot_dam_aux_player(Ind, o_ptr, k, q_ptr, brand_msg);
			}
			
			/* Apply the player damage bonuses */
			k += p_ptr->to_d + p_ptr->to_d_melee;

			/* No negative damage */
			if (k < 0) k = 0;

			/* XXX Reduce damage by 1/3 */
			k = (k + 2) / 3;

			/* Damage */
			if(zcave[p_ptr->py][p_ptr->px].info&CAVE_NOPK ||
			   zcave[q_ptr->py][q_ptr->px].info&CAVE_NOPK){
				if(k>q_ptr->chp) k-=q_ptr->chp;
				take_hit(0 - c_ptr->m_idx, k, p_ptr->name);

				/* Messages */
				msg_format(Ind, "You hit %s for \377o%d \377wdamage.", p_name, k);
				msg_format(0 - c_ptr->m_idx, "%s hits you for \377R%d \377wdamage.", p_ptr->name, k);
				if (strlen(brand_msg) > 0) msg_print(Ind, brand_msg);

				if(q_ptr->chp<5){
					msg_format(Ind, "You have beaten %s", q_ptr->name);
					msg_format(0-c_ptr->m_idx, "%s has beaten you up!", p_ptr->name);
					teleport_player(0 - c_ptr->m_idx, 400);
				}
			}
			else
			{
				take_hit(0 - c_ptr->m_idx, k, p_ptr->name);

				/* Messages */
				msg_format(Ind, "You hit %s for \377o%d \377wdamage.", p_name, k);
				msg_format(0 - c_ptr->m_idx, "%s hits you for \377R%d \377wdamage.", p_ptr->name, k);
				if (strlen(brand_msg) > 0) msg_print(Ind, brand_msg);

				if(cfg.use_pk_rules==PK_RULES_NEVER && q_ptr->chp<5){
					msg_format(Ind, "You have beaten %s", q_ptr->name);
					msg_format(0-c_ptr->m_idx, "%s has beaten you up!", p_ptr->name);
					teleport_player(0 - c_ptr->m_idx, 400);
				}
			}

			if(!c_ptr->m_idx) break;

			/* Check for death */
			if (q_ptr->death) break;

			/* Apply effects from mimic monster forms */
		        if (p_ptr->body_monster)
			{
#if 0
				switch (pr_ptr->r_ptr->d_char)
				{
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
				for (i = 0; i < 4; i++)
				{
				        if (pr_ptr->blow[i].d_dice * pr_ptr->blow[i].d_side)
					{
						monster_effects++;
						monster_effect[monster_effects] = pr_ptr->blow[i].effect;
					}
				}
				/* Choose random brand from the ones available */
				monster_effect_chosen = monster_effect[1 + rand_int(monster_effects)];
				
				
				/* Modify damage effect */
				if (apply_monster_effects)
				{
					switch (monster_effect_chosen)
					{
					case RBE_DISEASE:
                                        /* Take "poison" effect */
					        if (q_ptr->resist_pois || q_ptr->oppose_pois || q_ptr->immune_poison)
		                                {
		                                        msg_format(Ind, "%^s is unaffected.", p_name);
	                                        }
	                                        else if (rand_int(100) < q_ptr->skill_sav)
	                                        {
							msg_format(Ind, "%^s resists the disease.", p_name);
	                                        }
						else
		                                {
							msg_format(Ind, "%^s suffers from disease.", p_name);
				                        set_poisoned(0 - c_ptr->m_idx, q_ptr->poisoned + randint(p_ptr->lev) + 5);
						}
						break;
					case RBE_BLIND:
				                /* Increase "blind" */
	        	                        if (q_ptr->resist_blind)
						/*  for (i = 1; i <= NumPlayers; i++)
				                if (Players[i]->id == q_ptr->id) { */
		                                {
		                                        msg_format(Ind, "%^s is unaffected.", p_name);
	                                        }
	                                        else if (rand_int(100) < q_ptr->skill_sav)
	                                        {
							msg_format(Ind, "%^s resists the effect.", p_name);
	                                        }
	                                        else
						{
							set_blind(0 - c_ptr->m_idx, q_ptr->blind + 10 + randint(p_ptr->lev));
						}
						break;
					case RBE_HALLU:
	                                        /* Increase "image" */
	                                        if (q_ptr->resist_chaos)
		                                {
		                                        msg_format(Ind, "%^s is unaffected.", p_name);
	                                        }
	                                        else if (rand_int(100) < q_ptr->lev)
	                                        {
							msg_format(Ind, "%^s resists the effect.", p_name);
	                                        }
	                                        else
						{
                                    			set_image(0 - c_ptr->m_idx, q_ptr->image + 3 + randint(p_ptr->lev / 2));
						}
						break;
					case RBE_CONFUSE:
						if (!p_ptr->confusing)
						{
							/* Confuse the monster */
							if (q_ptr->resist_conf)
							{
								msg_format(Ind, "%^s is unaffected.", p_name);
							}
							else if (rand_int(100) < q_ptr->lev)
			    				{
								msg_format(Ind, "%^s resists the effect.", p_name);
							}
							else
							{
								msg_format(Ind, "%^s appears confused.", p_name);
								set_confused(0 - c_ptr->m_idx, q_ptr->confused + 10 + rand_int(get_skill(p_ptr, SKILL_COMBAT)) / 5);
							}
							
						}
						break;
					case RBE_TERRIFY:
						fear_chance = 50 + (p_ptr->lev - q_ptr->lev) * 5;
						if (rand_int(100) < fear_chance)
						{
							msg_format(Ind, "%^s appears afraid.", p_name);
							set_afraid(0 - c_ptr->m_idx, q_ptr->afraid + 4 + rand_int(get_skill(p_ptr, SKILL_COMBAT)) / 5);
						}
						else
		    				{
							msg_format(Ind, "%^s resists the effect.", p_name);
						}
						break;
					case RBE_PARALYZE:
	                                        /* Increase "paralyzed" */
		                                if (q_ptr->free_act)
		                                {
		                                        msg_format(Ind, "%^s is unaffected.", p_name);
	                                        }
	                                        else if (rand_int(100) < q_ptr->skill_sav)
	                                        {
							msg_format(Ind, "%^s resists the effect.", p_name);
	                                        }
	                                        else
						{
                            				set_paralyzed(0 - c_ptr->m_idx, q_ptr->paralyzed + 3 + randint(p_ptr->lev));
				                }
#if 0
						if (!p_ptr->stunning)
						{
							/* Stun the monster */
							if (rand_int(100) < q_ptr->lev)
							{
								msg_format(Ind, "%^s resists the effect.", p_name);
							}
							else
							{
								msg_format(Ind, "\377o%^s appears stunned.", p_name);
								set_stun(0 - c_ptr->m_idx, q_ptr->stun + 20 + rand_int(get_skill(p_ptr, SKILL_COMBAT)) / 5);
							}
						}
#endif
						break;
					}
				}
			}

			/* Confusion attack */
			if (p_ptr->confusing)
			{
				/* Cancel glowing hands */
				p_ptr->confusing = FALSE;

				/* Message */
				msg_print(Ind, "Your hands stop glowing.");

				/* Confuse the monster */
				if (q_ptr->resist_conf)
				{
					msg_format(Ind, "%^s is unaffected.", p_name);
				}
				else if (rand_int(100) < q_ptr->lev)
				{
					msg_format(Ind, "%^s resists the effect.", p_name);
				}
				else
				{
					msg_format(Ind, "%^s appears confused.", p_name);
					set_confused(0 - c_ptr->m_idx, q_ptr->confused + 10 + rand_int(get_skill(p_ptr, SKILL_COMBAT)) / 5);
				}
			}

			/* Stunning attack */
			if (p_ptr->stunning)
			{
				/* Cancel heavy hands */
				p_ptr->stunning = FALSE;

				/* Message */
				msg_print(Ind, "Your hands feel less heavy.");

				/* Stun the monster */
				if (rand_int(100) < q_ptr->lev)
				{
					msg_format(Ind, "%^s resists the effect.", p_name);
				}
				else
				{
					msg_format(Ind, "\377o%^s appears stunned.", p_name);
					set_stun(0 - c_ptr->m_idx, q_ptr->stun + 20 + rand_int(get_skill(p_ptr, SKILL_COMBAT)) / 5);
				}
			}

			/* Ghosts get fear attacks */
			if (p_ptr->ghost)
			{
				fear_chance = 50 + (p_ptr->lev - q_ptr->lev) * 5;

				if (rand_int(100) < fear_chance)
				{
					msg_format(Ind, "%^s appears afraid.", p_name);
					set_afraid(0 - c_ptr->m_idx, q_ptr->afraid + 4 + rand_int(get_skill(p_ptr, SKILL_COMBAT)) / 5);
				}
				else
				{
					msg_format(Ind, "%^s resists the effect.", p_name);
				}
			}

			/* Fruit bats get life stealing */
			if (p_ptr->fruit_bat)
			{
				int leech;
				
				leech = q_ptr->chp;
				if (k < leech) leech = k;
				leech /= 10;
			
				hp_player(Ind, rand_int(leech));
			}
		}

		/* Player misses */
		else
		{
			/* Sound */
			sound(Ind, SOUND_MISS);

			/* Messages */
			msg_format(Ind, "You miss %s.", p_name);
			msg_format(0 - c_ptr->m_idx, "%s misses you.", p_ptr->name);
		}

		/* Hack -- divided turn for auto-retaliator */
		if (!old)
		{
			break;
		}
	}

	/* Mega-Hack -- apply earthquake brand */
	if (do_quake)
	{
		if (o_ptr->k_idx && check_guard_inscription(o_ptr->note, 'E' ))
		{
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
 */
static void py_attack_mon(int Ind, int y, int x, bool old)
{
	player_type *p_ptr = Players[Ind];
	int                     num = 0, k, bonus, chance;

        object_type             *o_ptr;

	char            m_name[80], brand_msg[80];

	monster_type	*m_ptr;
	monster_race    *r_ptr;

	bool		fear = FALSE;
	bool            do_quake = FALSE;

	bool            backstab = FALSE, stab_fleeing = FALSE;
	bool nolite, nolite2, martial = FALSE;


	bool            vorpal_cut = FALSE;
	int             chaos_effect = 0;
	bool            drain_msg = TRUE;
	int             drain_result = 0, drain_heal = 0;
	int             drain_left = MAX_VAMPIRIC_DRAIN;

	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	cave_type *c_ptr;

	int fear_chance;

	monster_race *pr_ptr = &r_info[p_ptr->body_monster];
	bool apply_monster_effects = TRUE;
	int i, monster_effects;
	u32b monster_effect[6], monster_effect_chosen;
	monster_effect[1] = 0;
	monster_effect[2] = 0;
	monster_effect[3] = 0;
	monster_effect[4] = 0;
	monster_effect[5] = 0;

	if(!(zcave=getcave(wpos))) return;
	c_ptr=&zcave[y][x];

	m_ptr = &m_list[c_ptr->m_idx];
	r_ptr = race_inf(m_ptr);

	nolite = !((c_ptr->info & (CAVE_LITE | CAVE_GLOW)) ||
		(r_ptr->flags4 & RF4_BR_DARK) ||
		(r_ptr->flags6 & RF6_DARKNESS));
	nolite2 = nolite && !(r_ptr->flags9 & RF9_HAS_LITE);

	/* Disturb the player */
	disturb(Ind, 0, 0);

	/* Extract monster name (or "it") */
	monster_desc(Ind, m_name, c_ptr->m_idx, 0);

	if (m_ptr->owner == p_ptr->id && !p_ptr->confused &&
			p_ptr->mon_vis[c_ptr->m_idx])
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


	/* Hack -- divided turn for auto-retaliator */
	if (!old)
	{
		p_ptr->energy -= level_speed(&p_ptr->wpos) / p_ptr->num_blow;
	}

	/* Handle player fear */
	if (p_ptr->afraid)
	{
		/* Message */
		msg_format(Ind, "You are too afraid to attack %s!", m_name);

		suppress_message = FALSE;

		/* Done */
		return;
	}

	/* Access the weapon */
	o_ptr = &(p_ptr->inventory[INVEN_WIELD]);


	/* Cannot 'stab' with martial-arts */
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS) && !o_ptr->k_idx
			&& !p_ptr->inventory[INVEN_ARM].k_idx
			&& !p_ptr->inventory[INVEN_BOW].k_idx)
	{
		martial = TRUE;
	}
	/* Need TV_SWORD type weapon to backstab */
	else if (get_skill(p_ptr, SKILL_BACKSTAB) && (o_ptr->tval == TV_SWORD))
	{
		if(m_ptr->csleep /*&& m_ptr->ml*/)
			backstab = TRUE;
		else if (m_ptr->monfear /*&& m_ptr->ml)*/)
			stab_fleeing = TRUE;
	}


	/* Disturb the monster */
	m_ptr->csleep = 0;


	/* Attack once for each legal blow */
	while (num++ < p_ptr->num_blow)
	{
		u32b f1=0, f2=0, f3=0, f4=0, f5=0, esp=0;
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
		chaos_effect = 0;	// we need this methinks..?

		if((f4 & TR4_NEVER_BLOW))
		{
			msg_print(Ind, "You can't attack with that weapon.");
			break;
		}
		
		/* Calculate the "attack quality" */
		bonus = p_ptr->to_h + o_ptr->to_h + p_ptr->to_h_melee;
		chance = (p_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));

		/* Test for hit */
		if (test_hit_norm(chance, m_ptr->ac, p_ptr->mon_vis[c_ptr->m_idx]))
		{
			/* Sound */
			sound(Ind, SOUND_HIT);

			/* Message */
			/* DEG Updated hit message to include damage 
			if(backstab)
				msg_format(Ind, "You %s stab the helpless, sleeping %s!",
						nolite ? "*CRUELLY*" : "cruelly", r_name_get(m_ptr));
			else if (stab_fleeing)
				msg_format(Ind, "You %s the fleeing %s!",
						nolite2 ? "*backstab*" : "backstab", r_name_get(m_ptr));
			else if (!martial) msg_format(Ind, "You hit %s for \377g%d \377wdamage.", m_name, k); */
			
			/* Hack -- bare hands do one damage */
			k = 1;

			/* Ghosts get damage relative to level */
			if (p_ptr->ghost)
				k = p_ptr->lev;
				
			if (p_ptr->fruit_bat)
				k = p_ptr->lev * ((p_ptr->lev / 10) + 1);
#if 1 // DGHDGDGDG -- monks are no more
//			if (p_ptr->pclass == CLASS_MONK)
#if 0
			if (get_skill(p_ptr, SKILL_MARTIAL_ARTS) && !o_ptr->k_idx
					&& !p_ptr->inventory[INVEN_ARM].k_idx
					&& !p_ptr->inventory[INVEN_BOW].k_idx)
#endif	// 0
			if (martial)
			{
				int special_effect = 0, stun_effect = 0, times = 0;
				martial_arts * ma_ptr = &ma_blows[0], * old_ptr = &ma_blows[0];
				int resist_stun = 0;
				int marts = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 50);
				if (r_ptr->flags1 & RF1_UNIQUE) resist_stun += 88;
				if (r_ptr->flags3 & RF3_NO_CONF) resist_stun += 44;
				if (r_ptr->flags3 & RF3_NO_SLEEP) resist_stun += 44;
				if (r_ptr->flags3 & RF3_UNDEAD)
					resist_stun += 88;

				for (times = 0; times < (marts<7?1:marts/7); times++)
				/* Attempt 'times' */
				{
					do
					{
						ma_ptr = &ma_blows[(randint(MAX_MA))-1];
					}
					while ((ma_ptr->min_level > marts)
					    || (randint(marts)<ma_ptr->chance));

					/* keep the highest level attack available we found */
					if ((ma_ptr->min_level > old_ptr->min_level) &&
					    !(p_ptr->stun || p_ptr->confused))
					{
						old_ptr = ma_ptr;
					}
					else
					{
						ma_ptr = old_ptr;
					}
				}

				k = damroll(ma_ptr->dd, ma_ptr->ds);

				if (ma_ptr->effect == MA_KNEE)
				{
					if (r_ptr->flags1 & RF1_MALE)
					{
						msg_format(Ind, "You hit %s in the groin with your knee!", m_name);
						special_effect = MA_KNEE;
					}
					else
						msg_format(Ind, ma_ptr->desc, m_name);
				}

				else if (ma_ptr->effect == MA_SLOW)
				{
					if (!((r_ptr->flags1 & RF1_NEVER_MOVE)
					    || strchr("UjmeEv$,DdsbBFIJQSXclnw!=?", r_ptr->d_char)))
					{
						msg_format(Ind, "You kick %s in the ankle.", m_name);
						special_effect = MA_SLOW;
					}
					else msg_format(Ind, ma_ptr->desc, m_name);
				}
				else
				{
					if (ma_ptr->effect)
					{
						stun_effect = (ma_ptr->effect/2) + randint(ma_ptr->effect/2);
					}

					msg_format(Ind, ma_ptr->desc, m_name);
				}

				k = tot_dam_aux(Ind, o_ptr, k, m_ptr, brand_msg);
				k = critical_norm(Ind, marts * (randint(10)), ma_ptr->min_level, k, FALSE);

				if ((special_effect == MA_KNEE) && ((k + p_ptr->to_d) < m_ptr->hp))
				{
					msg_format(Ind, "%^s moans in agony!", m_name);
					stun_effect = 7 + randint(13);
					resist_stun /= 3;
				}

				else if ((special_effect == MA_SLOW) && ((k + p_ptr->to_d) < m_ptr->hp))
				{
					if (!(r_ptr->flags1 & RF1_UNIQUE) &&
					    (randint(marts * 2) > r_ptr->level) &&
					    m_ptr->mspeed > 60)
					{
						msg_format(Ind, "\377o%^s starts limping slower.", m_name);
						m_ptr->mspeed -= 10;
					}
				}

				if (stun_effect && ((k + p_ptr->to_d) < m_ptr->hp))
				{
					if (marts > randint(r_ptr->level + resist_stun + 10))
					{
						m_ptr->stunned += (stun_effect);

						if (m_ptr->stunned > 100)
							msg_format(Ind, "\377o%^s is knocked out.", m_name);
						else if (m_ptr->stunned > 50)
							msg_format(Ind, "\377o%^s is heavily stunned.", m_name);
						else
							msg_format(Ind, "\377o%^s is stunned.", m_name);
					}
				}
			} else
#endif	// 1 (martial arts)
			/* Handle normal weapon */
			if (o_ptr->k_idx)
			{
				k = damroll(o_ptr->dd, o_ptr->ds);
				k = tot_dam_aux(Ind, o_ptr, k, m_ptr, brand_msg);

				if (backstab)
				{
					k += (k * (nolite ? 3 : 1) *
						get_skill_scale(p_ptr, SKILL_BACKSTAB, 300)) / 100;
//					backstab = FALSE;
				}
				else if (stab_fleeing)
				{
					k += (k * (nolite2 ? 2 : 1) *
						get_skill_scale(p_ptr, SKILL_BACKSTAB, 210)) / 100;
//					stab_fleeing = FALSE;
				}

				/* Select a chaotic effect (50% chance) */
				if ((f5 & TR5_CHAOTIC) && (randint(2)==1))
				{
					if (randint(5) < 3)
					{
						/* Vampiric (20%) */
						chaos_effect = 1;
					}
					else if (randint(250) == 1)
					{
						/* Quake (0.12%) */
						chaos_effect = 2;
					}
					else if (randint(10) != 1)
					{
						/* Confusion (26.892%) */
						chaos_effect = 3;
					}
					else if (randint(2) == 1)
					{
						/* Teleport away (1.494%) */
						chaos_effect = 4;
					}
					else
					{
						/* Polymorph (1.494%) */
						chaos_effect = 5;
					}
				}

				/* Vampiric drain */
				if ((f1 & TR1_VAMPIRIC) || (chaos_effect == 1))
				{
					if (!((r_ptr->flags3 & RF3_UNDEAD) || (r_ptr->flags3 & RF3_NONLIVING)))
						drain_result = m_ptr->hp;
					else
						drain_result = 0;
				}

                                if (f1 & TR1_VORPAL && (randint(6) == 1))
                                        vorpal_cut = TRUE;
				else vorpal_cut = FALSE;

				if ((p_ptr->impact && (k > 50)) || chaos_effect == 2) do_quake = TRUE;

				k = critical_norm(Ind, o_ptr->weight, o_ptr->to_h + p_ptr->to_h_melee, k, ((o_ptr->tval == TV_SWORD) && (o_ptr->weight < 50)));
				if (vorpal_cut)
				{
					int step_k = k;

					msg_format(Ind, "Your weapon cuts deep into %s!", m_name);
					do
					{
						k += step_k;
					}
					while (randint(4) == 1);
				}

				k += o_ptr->to_d;


				/* May it clone the monster ? */
				if ((f4 & TR4_CLONE) && magik(30))
				{
					msg_format(Ind, "Oh no ! Your weapon clones %^s!", m_name);
					multiply_monster(c_ptr->m_idx);
				}

				/* heheheheheh */
				do_nazgul(Ind, &k, &num, r_ptr, o_ptr);

			}
			/* handle bare fists/bat/ghost */
			else
			{
				k = tot_dam_aux(Ind, o_ptr, k, m_ptr, brand_msg);
			}

			/* Apply the player damage bonuses */
			/* (should this also cancelled by nazgul?(for now not)) */
			k += p_ptr->to_d + p_ptr->to_d_melee;

			/* Penalty for could-2H when having a shield */
			if ((f4 & TR4_COULD2H) &&
					p_ptr->inventory[INVEN_ARM].k_idx) k /= 2;

			/* No negative damage */
			if (k < 0) k = 0;

			/* DEG Updated hit message to include damage */
			if(backstab)
			{
				backstab = FALSE;
				if (r_ptr->flags1 & RF1_UNIQUE)
				msg_format(Ind, "You %s stab the helpless, sleeping %s for \377e%d \377wdamage.", nolite ? "*CRUELLY*" : "cruelly", r_name_get(m_ptr), k);
				else msg_format(Ind, "You %s stab the helpless, sleeping %s for \377p%d \377wdamage.", nolite ? "*CRUELLY*" : "cruelly", r_name_get(m_ptr), k);
			}
			else if (stab_fleeing)
			{
				stab_fleeing = FALSE;
				if (r_ptr->flags1 & RF1_UNIQUE)
				msg_format(Ind, "You %s the fleeing %s for \377e%d \377wdamage.", nolite2 ? "*backstab*" : "backstab", r_name_get(m_ptr), k);
				else msg_format(Ind, "You %s the fleeing %s for \377g%d \377wdamage.", nolite2 ? "*backstab*" : "backstab", r_name_get(m_ptr), k);
			}
//			else if ((r_ptr->flags1 & RF1_UNIQUE) && (!martial)) msg_format(Ind, "You hit %s for \377p%d \377wdamage.", m_name, k);
//			else if (!martial) msg_format(Ind, "You hit %s for \377g%d \377wdamage.", m_name, k);
			else
			{
				if (r_ptr->flags1 & RF1_UNIQUE)
				msg_format(Ind, "You hit %s for \377e%d \377wdamage.", m_name, k);
				else msg_format(Ind, "You hit %s for \377g%d \377wdamage.", m_name, k);
			}
			if (strlen(brand_msg) > 0) msg_print(Ind, brand_msg);

			/* Damage, check for fear and death */
			if (mon_take_hit(Ind, c_ptr->m_idx, k, &fear, NULL)) break;

			touch_zap_player(Ind, c_ptr->m_idx);

			/* Apply effects from mimic monster forms */
		        if (p_ptr->body_monster)
			{
#if 0
				switch (pr_ptr->r_ptr->d_char)
				{
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
				for (i = 0; i < 4; i++)
				{
				        if (pr_ptr->blow[i].d_dice * pr_ptr->blow[i].d_side)
					{
						monster_effects++;
	    					monster_effect[monster_effects] = pr_ptr->blow[i].effect;
					}
				}
				/* Choose random brand from the ones available */
				monster_effect_chosen = monster_effect[1 + rand_int(monster_effects)];
				
				
				/* Modify damage effect */
				if (apply_monster_effects)
				{
					switch (monster_effect_chosen)
					{
					case RBE_BLIND:
						if (r_ptr->flags3 & RF3_NO_SLEEP) break;
					case RBE_HALLU:
						if (r_ptr->flags3 & RF3_NO_CONF) break;
					case RBE_CONFUSE:
						if (!p_ptr->confusing)
						{
							/* Confuse the monster */
							if (r_ptr->flags3 & RF3_NO_CONF)
							{
								if (p_ptr->mon_vis[c_ptr->m_idx]) r_ptr->r_flags3 |= RF3_NO_CONF;
								msg_format(Ind, "%^s is unaffected.", m_name);
							}
							else if (rand_int(100) < r_ptr->level)
							{
			    					msg_format(Ind, "%^s resists the effect.", m_name);
    		    					}
							else
							{
								msg_format(Ind, "%^s appears confused.", m_name);
								m_ptr->confused += 10 + rand_int(p_ptr->lev) / 5;
							}
							
						}
						break;
					case RBE_TERRIFY:
						fear_chance = 50 + (p_ptr->lev - r_ptr->level) * 5;
						if (!(r_ptr->flags3 & RF3_NO_FEAR) && rand_int(100) < fear_chance)
						{
							msg_format(Ind, "%^s appears afraid.", m_name);
							m_ptr->monfear = m_ptr->monfear + 4 + rand_int(p_ptr->lev) / 5;
						}
						break;
					case RBE_PARALYZE:
						if (!p_ptr->stunning)
						{
							/* Stun the monster */
							if (r_ptr->flags3 & RF3_NO_STUN)
							{
								if (p_ptr->mon_vis[c_ptr->m_idx]) r_ptr->r_flags3 |= RF3_NO_STUN;
								msg_format(Ind, "%^s is unaffected.", m_name);
							}
							else if (rand_int(115) < r_ptr->level)
							{
								msg_format(Ind, "%^s resists the effect.", m_name);
							}
							else
							{
								if (!m_ptr->stunned)
									msg_format(Ind, "\377o%^s appears stuned.", m_name);
								else
									msg_format(Ind, "\377o%^s appears more stunned.", m_name);
								m_ptr->stunned += 20 + rand_int(p_ptr->lev) / 5;
							}
						}
						break;
					}
				}
			}

			/* Are we draining it?  A little note: If the monster is
			   dead, the drain does not work... */

			if (drain_result)
			{
				drain_result -= m_ptr->hp;  /* Calculate the difference */

				if (drain_result > 0) /* Did we really hurt it? */
				{
					drain_heal = damroll(4,(drain_result / 6));

					if (drain_left)
					{
						if (drain_heal < drain_left)
						{
							drain_left -= drain_heal;
						}
						else
						{
							drain_heal = drain_left;
							drain_left = 0;
						}

						if (drain_msg)
						{
							msg_format(Ind, "Your weapon drains life from %s!", m_name);
							drain_msg = FALSE;
						}

						hp_player(Ind, drain_heal);
						/* We get to keep some of it! */
					}
				}
			}

			/* Confusion attack */
			if ((p_ptr->confusing) || (chaos_effect == 3))
			{
				/* Cancel glowing hands */
				p_ptr->confusing = FALSE;

				/* Message */
				msg_print(Ind, "Your hands stop glowing.");

				/* Confuse the monster */
				if (r_ptr->flags3 & RF3_NO_CONF)
				{
					if (p_ptr->mon_vis[c_ptr->m_idx]) r_ptr->r_flags3 |= RF3_NO_CONF;
					msg_format(Ind, "%^s is unaffected.", m_name);
				}
				else if (rand_int(100) < r_ptr->level)
				{
					msg_format(Ind, "%^s resists the effect.", m_name);
				}
				else
				{
					msg_format(Ind, "%^s appears confused.", m_name);
					m_ptr->confused += 10 + rand_int(p_ptr->lev) / 5;
				}
			}

			else if (chaos_effect == 4)
			{
				if (teleport_away(c_ptr->m_idx, 50))
				{
					msg_format(Ind, "%^s disappears!", m_name);
					num = p_ptr->num_blow + 1; /* Can't hit it anymore! */
				}
//				no_extra = TRUE;
			}

			else if ((chaos_effect == 5) && cave_floor_bold(zcave,y,x)
					&& (randint(90) > m_ptr->level))
			{
				if (!((r_ptr->flags1 & RF1_UNIQUE) ||
							(r_ptr->flags4 & RF4_BR_CHAO) ))
//						|| (m_ptr->mflag & MFLAG_QUEST)))
				{
					int tmp = poly_r_idx(m_ptr->r_idx);

					/* Pick a "new" monster race */

					/* Handle polymorph */
					if (tmp != m_ptr->r_idx)
					{
						msg_format(Ind, "%^s changes!", m_name);

						/* Create a new monster (no groups) */
						(void)place_monster_aux(wpos, y, x, tmp, FALSE, FALSE, m_ptr->clone);

						/* "Kill" the "old" monster */
						delete_monster_idx(c_ptr->m_idx);

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
			if (p_ptr->stunning)
			{
				/* Cancel heavy hands */
				p_ptr->stunning = FALSE;

				/* Message */
				msg_print(Ind, "Your hands feel less heavy.");

				/* Stun the monster */
				if (r_ptr->flags3 & RF3_NO_STUN)
				{
					if (p_ptr->mon_vis[c_ptr->m_idx]) r_ptr->r_flags3 |= RF3_NO_STUN;
					msg_format(Ind, "%^s is unaffected.", m_name);
				}
				else if (rand_int(115) < r_ptr->level)
				{
					msg_format(Ind, "%^s resists the effect.", m_name);
				}
				else
				{
					if (!m_ptr->stunned)
						msg_format(Ind, "\377o%^s appears stuned.", m_name);
					else
						msg_format(Ind, "\377o%^s appears more stunned.", m_name);
					m_ptr->stunned += 20 + rand_int(p_ptr->lev) / 5;
				}
			}

			/* Ghosts get fear attacks */
			if (p_ptr->ghost)
			{
				fear_chance = 50 + (p_ptr->lev - r_ptr->level) * 5;

				if (!(r_ptr->flags3 & RF3_NO_FEAR) && rand_int(100) < fear_chance)
				{
					msg_format(Ind, "%^s appears afraid.", m_name);
					m_ptr->monfear = m_ptr->monfear + 4 + rand_int(p_ptr->lev) / 5;
				}
			}


			/* Fruit bats get life stealing */
			if (p_ptr->fruit_bat)
			{
				int leech;
				
				leech = m_ptr->hp;
				if (k < leech) leech = k;
				leech /= 10;
			
				hp_player(Ind, rand_int(leech));
			}
		}

		/* Player misses */
		else
		{
			/* Sound */
			sound(Ind, SOUND_MISS);

			backstab = FALSE;

			/* Message */
			msg_format(Ind, "You miss %s.", m_name);
		}

		/* Hack -- divided turn for auto-retaliator */
		if (!old)
		{
			break;
		}
	}

	/* Hack -- delay fear messages */
	if (fear && p_ptr->mon_vis[c_ptr->m_idx])
	{
		/* Sound */
		sound(Ind, SOUND_FLEE);

		/* Message */
		msg_format(Ind, "%^s flees in terror!", m_name);
	}

	/* Mega-Hack -- apply earthquake brand */
	if (do_quake)
	{
		if (o_ptr->k_idx && check_guard_inscription(o_ptr->note, 'E' ))
		{
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
	struct worldpos *wpos=&p_ptr->wpos;
	if(!(zcave=getcave(wpos))) return;
	c_ptr=&zcave[y][x];

	if (r_info[p_ptr->body_monster].flags1 & RF1_NEVER_BLOW
			|| !p_ptr->num_blow) return;

	/* Break goi/manashield */
	if (p_ptr->invuln)
	{
		set_invuln(Ind, 0);
	}
	if (p_ptr->tim_manashield)
	{
		set_tim_manashield(Ind, 0);
	}
#if 0
	if (p_ptr->tim_wraith)
	{
		set_tim_wraith(Ind, 0);
	}
#endif	// 0

	/* Check for monster */
	if (c_ptr->m_idx > 0)
		py_attack_mon(Ind, y, x, old);

	/* Check for player */
	else if (c_ptr->m_idx < 0 && cfg.use_pk_rules != PK_RULES_NEVER)
		py_attack_player(Ind, y, x, old);
}

/* PernAngband addition */
// void touch_zap_player(int Ind, monster_type *m_ptr)
void touch_zap_player(int Ind, int m_idx)
{
	monster_type	*m_ptr = &m_list[m_idx];

	player_type *p_ptr = Players[Ind];
	int aura_damage = 0;
	monster_race *r_ptr = race_inf(m_ptr);

	if (r_ptr->flags2 & (RF2_AURA_FIRE))
	{
		if (!(p_ptr->immune_fire))
		{
			char aura_dam[80];

			aura_damage = damroll(1 + (m_ptr->level / 26), 1 + (m_ptr->level / 17));

			/* Hack -- Get the "died from" name */
			monster_desc(Ind, aura_dam, m_idx, 0x88);

			msg_print(Ind, "You are suddenly very hot!");

			if (p_ptr->oppose_fire) aura_damage = (aura_damage+2) / 3;
			if (p_ptr->resist_fire) aura_damage = (aura_damage+2) / 3;
			if (p_ptr->sensible_fire) aura_damage = (aura_damage+2) * 2;

			take_hit(Ind, aura_damage, aura_dam);
			r_ptr->r_flags2 |= RF2_AURA_FIRE;
			handle_stuff(Ind);
		}
	}


	if (r_ptr->flags2 & (RF2_AURA_ELEC))
	{
		if (!(p_ptr->immune_elec))
		{
			char aura_dam[80];

			aura_damage = damroll(1 + (m_ptr->level / 26), 1 + (m_ptr->level / 17));

			/* Hack -- Get the "died from" name */
			monster_desc(Ind, aura_dam, m_idx, 0x88);

			if (p_ptr->oppose_elec) aura_damage = (aura_damage+2) / 3;
			if (p_ptr->resist_elec) aura_damage = (aura_damage+2) / 3;
			if (p_ptr->sensible_elec) aura_damage = (aura_damage+2) * 2;

			msg_print(Ind, "You get zapped!");
			take_hit(Ind, aura_damage, aura_dam);
			r_ptr->r_flags2 |= RF2_AURA_ELEC;
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
void do_nazgul(int Ind, int *k, int *num, monster_race *r_ptr, object_type *o_ptr)
{
	if (r_ptr->flags7 & RF7_NAZGUL)
	{
		int weap = 0;	// Hack!
		u32b f1, f2, f3, f4, f5, esp;

		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

		if ((!o_ptr->name2) && (!artifact_p(o_ptr)))
		{
			msg_print(Ind, "\377rYour weapon *DISINTEGRATES*!");
			*k = 0;
			inven_item_increase(Ind, INVEN_WIELD + weap, -1);
			inven_item_optimize(Ind, INVEN_WIELD + weap);

			/* To stop attacking */
//			*num = num_blow;
		}
		else if (artifact_p(o_ptr))
		{
			if (!(f1 & TR1_SLAY_EVIL) && !(f1 & TR1_SLAY_UNDEAD) && !(f5 & TR5_KILL_UNDEAD))
			{
				msg_print(Ind, "The Ringwraith is IMPERVIOUS to the mundane weapon.");
				*k = 0;
			}

//			apply_disenchant(Ind, INVEN_WIELD + weap);
			apply_disenchant(Ind, weap);

			/* 1/1000 chance of getting destroyed */
			if (!rand_int(1000))
			{
				if (true_artifact_p(o_ptr))
				{
					a_info[o_ptr->name1].cur_num = 0;
					a_info[o_ptr->name1].known = FALSE;
				}

				msg_print(Ind, "\377rYour weapon is destroyed !");
				inven_item_increase(Ind, INVEN_WIELD + weap, -1);
				inven_item_optimize(Ind, INVEN_WIELD + weap);

				/* To stop attacking */
//				*num = num_blow;
			}
		}
		else if (o_ptr->name2)
		{
			if (!(f1 & TR1_SLAY_EVIL) && !(f1 & TR1_SLAY_UNDEAD) && !(f5 & TR5_KILL_UNDEAD))
			{
				msg_print(Ind, "The Ringwraith is IMPERVIOUS to the mundane weapon.");
				*k = 0;
			}

			/* 25% chance of getting destroyed */
			if (magik(25))
			{
				msg_print(Ind, "\377rYour weapon is destroyed !");
				inven_item_increase(Ind, INVEN_WIELD + weap, -1);
				inven_item_optimize(Ind, INVEN_WIELD + weap);

				/* To stop attacking */
//				*num = num_blow;
			}
		}

		/* If any damage is done, then 25% chance of getting the Black Breath */
		if (*k)
		{
			if (magik(25))
			{
				set_black_breath(Ind);
			}
		}
	}
}

void set_black_breath(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* deja */
	if (p_ptr->black_breath) return;
	if (p_ptr->ghost) return;

	msg_print(Ind, "Your foe calls upon your soul!");
	msg_print(Ind, "You feel the Black Breath slowly draining you of life...");
	p_ptr->black_breath = TRUE;
}


/* Do a probability travel in a wall */
void do_prob_travel(int Ind, int dir)
{
  player_type *p_ptr = Players[Ind];
  int x = p_ptr->px, y = p_ptr->py;
  bool do_move = TRUE;
  struct worldpos *wpos=&p_ptr->wpos;
  cave_type **zcave;
  if(!(zcave=getcave(wpos))) return;

  /* Paranoia */
  if (dir == 5) return;
  if ((dir < 1) || (dir > 9)) return;

  x += ddx[dir];
  y += ddy[dir];

  while (TRUE)
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


static bool wraith_access(int Ind){
	/* Experimental! lets hope not bugged */
	/* Wraith walk in own house */
	player_type *p_ptr=Players[Ind];
	int i;
	bool house=FALSE;

	for(i=0;i<num_houses;i++){
		if(inarea(&houses[i].wpos, &p_ptr->wpos))
		{
			if(fill_house(&houses[i], FILL_PLAYER, p_ptr)){
				house=TRUE;
				if(access_door(Ind, houses[i].dna)){
#if 0	// Jir test
					if(houses[i].flags & HF_APART){
						break;
					}
					else
#endif	// 0
						return(TRUE);
				}
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
	if (p_ptr->admin_dm && feature != FEAT_PERM_SOLID)
		return (TRUE);

	/* Player can not walk through "walls" unless in Shadow Form */
//        if (p_ptr->wraith_form || (PRACE_FLAG(PR1_SEMI_WRAITH)))
	if (/*p_ptr->wraith_form ||*/ p_ptr->ghost || p_ptr->tim_wraith)
		pass_wall = TRUE;
	else
		pass_wall = FALSE;

#if 0	// it's interesting.. hope we can have similar feature :)
	/* Wall mimicry force the player to stay in walls */
	if (p_ptr->mimic_extra & CLASS_WALL)
	{
		only_wall = TRUE;
	}
#endif	// 0

	switch (feature)
	{
#if 0
		/* NOTE: we're not to backport wild_mode (it's cheezy);
		 * however it's good idea to restrict crossing severer is nice idea
		 * so p_ptr->wild_mode code can be recycled.	- Jir -
		 */
		case FEAT_DEEP_WATER:
			if (p_ptr->wild_mode)
			{
				int wt = (adj_str_wgt[p_ptr->stat_ind[A_STR]] * 100) / 2;

				if ((calc_total_weight() < wt) || (p_ptr->ffall))
					return (TRUE);
				else
					return (FALSE);
			}
			else
				return (TRUE);

		case FEAT_SHAL_LAVA:
			if (p_ptr->wild_mode)
			{
				if ((p_ptr->resist_fire) ||
					(p_ptr->immune_fire) ||
					(p_ptr->oppose_fire) || (p_ptr->ffall))
					return (TRUE);
				else
					return (FALSE);
			}
			else
				return (TRUE);

		case FEAT_DEEP_LAVA:
			if (p_ptr->wild_mode)
			{
				if ((p_ptr->resist_fire) ||
					(p_ptr->immune_fire) ||
					(p_ptr->oppose_fire) || (p_ptr->ffall))
					return (TRUE);
				else
					return (FALSE);
			}
			else
				return (TRUE);
#endif	// 0
		case FEAT_DEEP_WATER:
		case FEAT_SHAL_LAVA:
		case FEAT_DEEP_LAVA:
			return (TRUE);	/* you can pass, but you may suffer dmg */

		case FEAT_DEAD_TREE:
			if ((p_ptr->fly) || pass_wall)
			    return (TRUE);
		/* 	handled in default:
		    case FEAT_SMALL_TREES: 	*/
		case FEAT_TREES:
		{
			/* 708 = Ent (passes trees), 83/142 novice ranger, 345 ranger, 637 ranger chieftain, 945 high-elven ranger */
			if ((p_ptr->fly) || (p_ptr->pass_trees) || pass_wall)
#if 0
					(PRACE_FLAG(PR1_PASS_TREE)) ||
				(get_skill(SKILL_DRUID) > 15) ||
				(p_ptr->mimic_form == MIMIC_ENT))
#endif	// 0
				return (TRUE);
			else
				return (FALSE);
		}

#if 0
		case FEAT_WALL_HOUSE:
		{
			if (!pass_wall || !wraith_access(Ind)) return (FALSE);
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
	struct worldpos *wpos=&p_ptr->wpos, nwpos;

	int                     y, x, oldx, oldy;
	int i;
	//bool do_move = FALSE;

	cave_type               *c_ptr;
	struct c_special	*cs_ptr;
	object_type             *o_ptr;
	monster_type    *m_ptr;
	byte                    *w_ptr;
	monster_race *r_ptr = &r_info[p_ptr->body_monster];
	cave_type **zcave;
	int csmove=TRUE;

	if(!(zcave=getcave(wpos))) return;


	/* (S)He's no longer AFK, lol */
	if (p_ptr->afk) toggle_afk(Ind);

	/* Can we move ? */
	if (r_ptr->flags1 & RF1_NEVER_MOVE)
	{
		msg_print(Ind, "You cannot move by nature.");
		return;
	}

	/* Find the result of moving */
	/* -C. Blue- I toned dow monster RAND_50 and RAND_25 for a mimicrying player,
	assuming that the mimic does not use the monster mind but its own to control
	the body, on the other hand the body still carries reflexes from the monster ;)
	- technical reason was to make more forms useful, especially RAND_50 forms */
	if (((r_ptr->flags1 & RF1_RAND_50) && (r_ptr->flags1 & RF1_RAND_25) && magik(30)) ||
	   ((r_ptr->flags1 & RF1_RAND_50) && (!(r_ptr->flags1 & RF1_RAND_25)) && magik(20)) ||
	   ((!(r_ptr->flags1 & RF1_RAND_50)) && (r_ptr->flags1 & RF1_RAND_25) && magik(10)))
	{
		do
		{
			i = randint(9);
			y = p_ptr->py + ddy[i];
			x = p_ptr->px + ddx[i];
		} while (i == 5);
	}
	else
	{
		y = p_ptr->py + ddy[dir];
		x = p_ptr->px + ddx[dir];
	}

	c_ptr = &zcave[p_ptr->py][p_ptr->px];
	if ((c_ptr->feat == FEAT_ICE) && (!p_ptr->feather_fall && !p_ptr->fly))
	{
		if (magik(70 - p_ptr->lev))
		{
			do
			{
				i = randint(9);
				y = p_ptr->py + ddy[i];
				x = p_ptr->px + ddx[i];
			} while (i == 5);
			msg_print(Ind, "You slip on the icy floor.");
		}
#if 0
		else
			tmp = dir;
#endif	// 0
	}

	/* Update wilderness positions */
	if(wpos->wz==0)
	{
		/* Make sure he hasn't just changed depth */
		if (p_ptr->new_level_flag) return;
		
		/* save his old location */
		wpcopy(&nwpos, wpos);
		oldx = p_ptr->px; oldy = p_ptr->py;
		
		/* we have gone off the map */
		if (!in_bounds(y, x))
		{
			/* find his new location */
			if (y <= 0)
			{       
				/* new player location */
				nwpos.wy++;
				p_ptr->py = MAX_HGT-2;
			}
			if (y >= 65)
			{                       
				/* new player location */  
				nwpos.wy--;
				p_ptr->py = 1;
			}
			if (x <= 0)
			{
				/* new player location */
				nwpos.wx--;
				p_ptr->px = MAX_WID-2;
			}
			if (x >= 197)
			{
				/* new player location */
				nwpos.wx++;                       
				p_ptr->px = 1;
			}
		
			/* check to make sure he hasnt hit the edge of the world */
			if(nwpos.wx<0 || nwpos.wx>=MAX_WILD_X || nwpos.wy<0 || nwpos.wy>=MAX_WILD_Y)
			{
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
			
			/* A player has left this depth */
			new_players_on_depth(wpos,-1,TRUE);

			p_ptr->wpos.wx = nwpos.wx;
			p_ptr->wpos.wy = nwpos.wy;
		
			/* update the wilderness map */
			if(!p_ptr->ghost)
				p_ptr->wild_map[(p_ptr->wpos.wx + p_ptr->wpos.wy*MAX_WILD_X)/8] |= (1<<((p_ptr->wpos.wx + p_ptr->wpos.wy*MAX_WILD_X)%8));
			
			new_players_on_depth(wpos,1,TRUE);
			p_ptr->new_level_flag = TRUE;
			p_ptr->new_level_method = LEVEL_OUTSIDE;
#if 0	// it's done in process_player_change_wpos
			if(istown(&p_ptr->wpos)){
				p_ptr->town_x=p_ptr->wpos.wx;
				p_ptr->town_y=p_ptr->wpos.wy;
			}
#endif	// 0

			return;
		}
	}

	/* Examine the destination */
	c_ptr = &zcave[y][x];
	w_ptr = &p_ptr->cave_flag[y][x];

	/* Get the object */
	o_ptr = &o_list[c_ptr->o_idx];

	/* Get the monster */
	m_ptr = &m_list[c_ptr->m_idx];

	/* Save "last direction moved" */
	p_ptr->last_dir = dir;

	/* Bump into other players */
	if (c_ptr->m_idx < 0)
	{
		player_type *q_ptr = Players[0 - c_ptr->m_idx];
		int Ind2 = 0 - c_ptr->m_idx;

		/* Check for an attack */
		if (cfg.use_pk_rules != PK_RULES_NEVER &&
			check_hostile(Ind, Ind2))
			py_attack(Ind, y, x, TRUE);

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
		else if ((!p_ptr->ghost && !q_ptr->ghost &&
			 (ddy[q_ptr->last_dir] == -(ddy[dir])) &&
			 (ddx[q_ptr->last_dir] == (-ddx[dir]))) ||
			(q_ptr->admin_dm))
#endif	// 0
		{
		  if (!((!wpos->wz) && (p_ptr->tim_wraith || q_ptr->tim_wraith)))
		    {

			c_ptr->m_idx = 0 - Ind;
			zcave[p_ptr->py][p_ptr->px].m_idx = 0 - Ind2;

			q_ptr->py = p_ptr->py;
			q_ptr->px = p_ptr->px;

			p_ptr->py = y;
			p_ptr->px = x;

			/* Tell both of them */
			/* Don't tell people they bumped into the Dungeon Master */
			if (!q_ptr->admin_dm)
			{
				/* Hack if invisible */
				if (p_ptr->play_vis[Ind2])                              
					msg_format(Ind, "You switch places with %s.", q_ptr->name);
				else
					msg_format(Ind, "You switch places with it.");
				
				/* Hack if invisible */
				if (q_ptr->play_vis[Ind])
					msg_format(Ind2, "You switch places with %s.", p_ptr->name);
				else
					msg_format(Ind2, "You switch places with it.");

				black_breath_infection(Ind, Ind2);
			}

			/* Disturb both of them */
			disturb(Ind, 1, 0);
			disturb(Ind2, 1, 0);

			/* Re-show both grids */
			everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);
			everyone_lite_spot(wpos, q_ptr->py, q_ptr->px);

			p_ptr->update |= PU_LITE;
			q_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
		    }           
		}

		/* Hack -- the Dungeon Master cannot bump people */
		else if (!p_ptr->admin_dm)
		{
			/* Tell both about it */
			/* Hack if invisible */
			if (p_ptr->play_vis[Ind2])
				msg_format(Ind, "You bump into %s.", q_ptr->name);
			else
				msg_format(Ind, "You bump into it.");
			
			/* Hack if invisible */
			if (q_ptr->play_vis[Ind])
				msg_format(Ind2, "%s bumps into you.", p_ptr->name);
			else
				msg_format(Ind2, "It bumps into you.");

			black_breath_infection(Ind, Ind2);

			/* Disturb both parties */
			disturb(Ind, 1, 0);
			disturb(Ind2, 1, 0);
		}
		return;
	}

	/* Hack -- attack monsters */
	if (c_ptr->m_idx > 0)
	{
		/* Hack -- the dungeon master switches places with his monsters */
		if (p_ptr->admin_dm)
		{
			/* save old player location */
			oldx = p_ptr->px;
			oldy = p_ptr->py;
			/* update player location */
			p_ptr->px = m_list[c_ptr->m_idx].fx;
			p_ptr->py = m_list[c_ptr->m_idx].fy;
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
		else py_attack(Ind, y, x, TRUE);
		return;
	}

	/* Prob travel */
	if (p_ptr->prob_travel && (!cave_floor_bold(zcave, y, x)))
	{
		do_prob_travel(Ind, dir);
		return;
	}

	/* now this is temp while i redesign!!! - do not change */
	cs_ptr=c_ptr->special;
	while(cs_ptr){
		int tcv;
		tcv=csfunc[cs_ptr->type].activate(cs_ptr, y, x, Ind);
		cs_ptr=cs_ptr->next;
		if(!tcv){
			csmove=FALSE;
			printf("csmove is false\n");
		}
	}

	/* Player can not walk through "walls", but ghosts can */
	if (!player_can_enter(Ind, c_ptr->feat) || !csmove)
	{
		/* walk-through entry for house owners ... sry it's DIRTY -Jir- */
		bool myhome = FALSE;
		bool passing = (p_ptr->tim_wraith || p_ptr->ghost);

		/* XXX quick fix */
		if (passing)
		{
#if 0
			if (c_ptr->feat == FEAT_HOME)
			{
				struct c_special *cs_ptr;
				if(!(cs_ptr=GetCS(c_ptr, CS_DNADOOR)) ||
						!access_door(Ind, cs_ptr->sc.ptr))
				{
					msg_print(Ind, "The door blocks your movement.");
					disturb(Ind, 0, 0);
					return;
				}

				myhome = TRUE;
				msg_print(Ind, "\377GYou pass through the door.");
			}
			else
#endif	// 0

			/* XXX maybe needless anymore */
			if(c_ptr->feat == FEAT_WALL_HOUSE)
			{
				if (!wraith_access_virtual(Ind, y, x))
				{
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
		if (!myhome && c_ptr->feat == FEAT_TREES &&
				(p_ptr->fly || p_ptr->pass_trees))
			myhome = TRUE;
#endif	// 0

		if (!myhome)
		{

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
			if (c_ptr->feat == FEAT_RUBBLE)
			{
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
			else if (c_ptr->feat == FEAT_TREES || c_ptr->feat==FEAT_DEAD_TREE)
			{
				msg_print(Ind, "\377GYou feel a tree blocking your way.");
				*w_ptr |= CAVE_MARK;
				everyone_lite_spot(wpos, y, x);
			}

			/* Wall (or secret door) */
			else
			{
				msg_print(Ind, "\377GYou feel a wall blocking your way.");
				*w_ptr |= CAVE_MARK;
				everyone_lite_spot(wpos, y, x);
			}
		}

		/* Notice things */
		else
		{
			//struct c_special *cs_ptr;
			/* Closed doors */
			if ((c_ptr->feat < FEAT_SECRET && c_ptr->feat >= FEAT_DOOR_HEAD) || 
				 (c_ptr->feat == FEAT_HOME))
			{
				msg_print(Ind, "There is a closed door blocking your way.");
			}

			else if (p_ptr->easy_tunnel || (p_ptr->skill_dig > 4000))
			{
				do_cmd_tunnel(Ind, dir);
			}
			else
			{
				/* Rubble */
				if (c_ptr->feat == FEAT_RUBBLE)
				{
					msg_print(Ind, "There is rubble blocking your way.");
				}
				/* Tree */
				else if (c_ptr->feat == FEAT_TREES || c_ptr->feat==FEAT_DEAD_TREE)
				{
					msg_print(Ind, "There is a tree blocking your way.");
				}
				/* Wall (or secret door) */
				else
				{
					msg_print(Ind, "There is a wall blocking your way.");
				}
			}
		}
		return;
		} /* 'if (!myhome)' ends here */
	}


	/* XXX fly? */
	else if ((c_ptr->feat == FEAT_DARK_PIT) && !p_ptr->feather_fall &&
			!p_ptr->fly && !p_ptr->admin_dm)
	{
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

		if(zcave[y][x].info & CAVE_STCK && !(zcave[oy][ox].info & CAVE_STCK))
			msg_print(Ind, "\377DThe air in here feels very still.");
		if(zcave[oy][ox].info & CAVE_STCK && !(zcave[y][x].info & CAVE_STCK))
			msg_print(Ind, "\377sFresh air greets you as you leave the vault.");
		/* Update the player indices */
		zcave[oy][ox].m_idx = 0;
		zcave[y][x].m_idx = 0 - Ind;

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
		if ((p_ptr->skill_fos >= 50) ||
		    (0 == rand_int(50 - p_ptr->skill_fos)))
		{
			search(Ind);
		}

		/* Continuous Searching */
		if (p_ptr->searching)
		{
			search(Ind);
		}

		/* Handle "objects" */
		if (c_ptr->o_idx && !p_ptr->admin_dm) carry(Ind, do_pickup, 0);
		else Send_floor(Ind, 0);

		/* Handle "store doors" */
		if ((!p_ptr->ghost) &&
		    (c_ptr->feat == FEAT_SHOP))
#if 0
		    (c_ptr->feat >= FEAT_SHOP_HEAD) &&
		    (c_ptr->feat <= FEAT_SHOP_TAIL))
#endif	// 0
		{
			/* Disturb */
			disturb(Ind, 0, 0);

			/* Hack -- Enter store */
			command_new = '_';
			do_cmd_store(Ind);
		}

		/* Handle resurrection */
		else if (p_ptr->ghost && c_ptr->feat == FEAT_SHOP &&
			(cs_ptr=GetCS(c_ptr, CS_SHOP)) && cs_ptr->sc.omni == 3)

		{
			if(p_ptr->wild_map[(p_ptr->wpos.wx + p_ptr->wpos.wy*MAX_WILD_X)/8] & (1<<((p_ptr->wpos.wx + p_ptr->wpos.wy*MAX_WILD_X)%8))){
				/* Resurrect him */
				resurrect_player(Ind);

				/* Give him some gold to restart */
				if (p_ptr->lev > 1 && !p_ptr->admin_dm)
				{
					/* int i = (p_ptr->lev > 4)?(p_ptr->lev - 3) * 100:100; */
					int i = (p_ptr->lev > 4)?(p_ptr->lev - 3) * 100 + (p_ptr->lev / 10) * (p_ptr->lev / 10) * 800:100;
					msg_format(Ind, "The temple priest gives you %ld gold pieces for your revival!", i);
					p_ptr->au += i;
				}
			}
			else msg_print(Ind, "\377rThe temple priest turns you away!");
		}
#ifndef USE_MANG_HOUSE_ONLY
		else if ((c_ptr->feat == FEAT_HOME || c_ptr->feat == FEAT_HOME_OPEN)
				&& !p_ptr->ghost)
		{
			do_cmd_trad_house(Ind);
//			return;	/* allow builders to build */
		}
#endif	// USE_MANG_HOUSE_ONLY


		/* Discover invisible traps */
//		else if (c_ptr->feat == FEAT_INVIS)
		if((cs_ptr=GetCS(c_ptr, CS_TRAPS)) && !p_ptr->ghost)
		{
			bool hit = TRUE;

			/* Disturb */
			disturb(Ind, 0, 0);

			if (!cs_ptr->sc.trap.found)
			{
				/* Message */
//				msg_print(Ind, "You found a trap!");
				msg_print(Ind, "You triggered a trap!");

				/* Pick a trap */
				pick_trap(&p_ptr->wpos, p_ptr->py, p_ptr->px);
			}
			else if (magik(get_skill_scale(p_ptr, SKILL_DISARM, 90)
						- UNAWARENESS(p_ptr)))
			{
				msg_print(Ind, "You carefully avoid touching the trap.");
				hit = FALSE;
			}

			/* Hit the trap */
			if (hit) hit_trap(Ind);
		}

		/* Mega-hack -- if we are the dungeon master, and our movement hook
		 * is set, call it.  This is used to make things like building walls
		 * and summoning monster armies easier.
		 */

#if 0
		if ((!strcmp(p_ptr->name,cfg_dungeon_master) || player_is_king(Ind)) && p_ptr->master_move_hook)
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

	if (p_ptr->black_breath && magik(20)) set_black_breath(Ind2);
	if (q_ptr->black_breath && magik(20)) set_black_breath(Ind);
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

	/* Ghosts run right through everything */
	if ((p_ptr->ghost || p_ptr->tim_wraith)) return (FALSE);

	/* Do wilderness hack, keep running from one outside level to another */
	if ( (!in_bounds(y, x)) && (wpos->wz==0) ) return FALSE;

	/* Illegal grids are blank */
	/* XXX this should be blocked by permawalls, hopefully. */
//	if (!in_bounds2(wpos, y, x)) return (FALSE);

	/* Must actually block motion */
	if (cave_floor_bold(zcave, y, x)) return (FALSE);

	if (f_info[zcave[y][x].feat].flags1 & FF1_CAN_RUN) return (FALSE);

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
static byte cycle[] =
{ 1, 2, 3, 6, 9, 8, 7, 4, 1, 2, 3, 6, 9, 8, 7, 4, 1 };

/*
 * Hack -- map each direction into the "middle" of the "cycle[]" array
 */
static byte chome[] =
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
	struct worldpos *wpos=&p_ptr->wpos;

	int                     prev_dir, new_dir, check_dir = 0;

	int                     row, col;
	int                     i, max, inv;
	int                     option, option2;
	bool	aqua = p_ptr->can_swim || p_ptr->fly || (get_skill(p_ptr, SKILL_SWIM) > 29) ||
					((p_ptr->body_monster) && (
					(r_info[p_ptr->body_monster].flags7 & RF7_AQUATIC) ||
					(r_info[p_ptr->body_monster].flags3 & RF3_UNDEAD) ));

	cave_type               *c_ptr;
	byte                    *w_ptr;
	cave_type **zcave;
	struct c_special *cs_ptr;
	if(!(zcave=getcave(wpos))) return(FALSE);

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
	for (i = -max; i <= max; i++)
	{
		new_dir = cycle[chome[prev_dir] + i];

		row = p_ptr->py + ddy[new_dir];
		col = p_ptr->px + ddx[new_dir];

		c_ptr = &zcave[row][col];
		w_ptr = &p_ptr->cave_flag[row][col];


		/* Visible monsters abort running */
		if (c_ptr->m_idx > 0)
		{
			/* Visible monster */
			if (p_ptr->mon_vis[c_ptr->m_idx] &&
					!(m_list[c_ptr->m_idx].special) &&
					r_info[m_list[c_ptr->m_idx].r_idx].level != 0)
					return (TRUE);
		}

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
				case FEAT_TREES:
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
		if (inv || cave_floor_bold(zcave, row, col) || ((!in_bounds(row, col)) && (wpos->wz==0)) )
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

			/* Unknown grid or floor */
			if (!(p_ptr->cave_flag[row][col] & CAVE_MARK) || cave_floor_bold(zcave, row, col))
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

			/* Unknown grid or floor */
			if (!(p_ptr->cave_flag[row][col] & CAVE_MARK) || cave_floor_bold(zcave, row, col))
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

	/* Check for just changed level */
	if (p_ptr->new_level_flag) return;

	/* Start running */
	if (dir)
	{
		/* Calculate torch radius */
		p_ptr->update |= (PU_TORCH);

		/* Initialize */
		run_init(Ind, dir);
		/* check if we have enough energy to move */
		if (p_ptr->energy < level_speed(&p_ptr->wpos)/cfg.running_speed)
			return;
	}

	/* Keep running */
	else
	{
		/* Update run */
		if (run_test(Ind))
		{
			/* Disturb */
			disturb(Ind, 0, 0);

			/* Done */
			return;
		}
	}

	/* Decrease the run counter */
	if (--(p_ptr->running) <= 0) return;

	/* Move the player, using the "pickup" flag */
	move_player(Ind, p_ptr->find_current, p_ptr->always_pickup);
}
