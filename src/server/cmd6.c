/* File: cmd6.c */

/* Purpose: Object commands */

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
 * This file includes code for eating food, drinking potions,
 * reading scrolls, aiming wands, using staffs, zapping rods,
 * and activating artifacts.
 *
 * In all cases, if the player becomes "aware" of the item's use
 * by testing it, mark it as "aware" and reward some experience
 * based on the object's level, always rounding up.  If the player
 * remains "unaware", mark that object "kind" as "tried".
 *
 * This code now correctly handles the unstacking of wands, staffs,
 * and rods.  Note the overly paranoid warning about potential pack
 * overflow, which allows the player to use and drop a stacked item.
 *
 * In all "unstacking" scenarios, the "used" object is "carried" as if
 * the player had just picked it up.  In particular, this means that if
 * the use of an item induces pack overflow, that item will be dropped.
 *
 * For simplicity, these routines induce a full "pack reorganization"
 * which not only combines similar items, but also reorganizes various
 * items to obey the current "sorting" method.  This may require about
 * 400 item comparisons, but only occasionally.
 *
 * There may be a BIG problem with any "effect" that can cause "changes"
 * to the inventory.  For example, a "scroll of recharging" can cause
 * a wand/staff to "disappear", moving the inventory up.  Luckily, the
 * scrolls all appear BEFORE the staffs/wands, so this is not a problem.
 * But, for example, a "staff of recharging" could cause MAJOR problems.
 * In such a case, it will be best to either (1) "postpone" the effect
 * until the end of the function, or (2) "change" the effect, say, into
 * giving a staff "negative" charges, or "turning a staff into a stick".
 * It seems as though a "rod of recharging" might in fact cause problems.
 * The basic problem is that the act of recharging (and destroying) an
 * item causes the inducer of that action to "move", causing "o_ptr" to
 * no longer point at the correct item, with horrifying results.
 *
 * Note that food/potions/scrolls no longer use bit-flags for effects,
 * but instead use the "sval" (which is also used to sort the objects).
 */






/*
 * Eat some food (from the pack or floor)
 */
void do_cmd_eat_food(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	int			ident, lev;

	object_type		*o_ptr;


	/* Restrict choices to food */
	item_tester_tval = TV_FOOD;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

        if( check_guard_inscription( o_ptr->note, 'E' )) {
                msg_print(Ind, "The item's inscription prevents it");
                return;
        };

        if (!can_use(Ind, o_ptr))
        {
                msg_print(Ind, "You are not high level enough.");
		return;
        }


	if (o_ptr->tval != TV_FOOD)
	{
		msg_print(Ind, "SERVER ERROR: Tried to eat non-food!");
		return;
	}


	/* Take a turn */
	p_ptr->energy -= level_speed(p_ptr->dun_depth);

	/* Identity not known yet */
	ident = FALSE;

	/* Object level */
	lev = k_info[o_ptr->k_idx].level;

	/* Analyze the food */
	switch (o_ptr->sval)
	{
		case SV_FOOD_POISON:
		{
			if (!(p_ptr->resist_pois || p_ptr->oppose_pois))
			{
				if (set_poisoned(Ind, p_ptr->poisoned + rand_int(10) + 10))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_FOOD_BLINDNESS:
		{
			if (!p_ptr->resist_blind)
			{
				if (set_blind(Ind, p_ptr->blind + rand_int(200) + 200))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_FOOD_PARANOIA:
		{
			if (!p_ptr->resist_fear)
			{
				if (set_afraid(Ind, p_ptr->afraid + rand_int(10) + 10))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_FOOD_CONFUSION:
		{
			if (!p_ptr->resist_conf)
			{
				if (set_confused(Ind, p_ptr->confused + rand_int(10) + 10))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_FOOD_HALLUCINATION:
		{
			if (!p_ptr->resist_chaos)
			{
				if (set_image(Ind, p_ptr->image + rand_int(250) + 250))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_FOOD_PARALYSIS:
		{
			if (!p_ptr->free_act)
			{
				if (set_paralyzed(Ind, p_ptr->paralyzed + rand_int(10) + 10))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_FOOD_WEAKNESS:
		{
			take_hit(Ind, damroll(6, 6), "poisonous food.");
			(void)do_dec_stat(Ind, A_STR);
			ident = TRUE;
			break;
		}

		case SV_FOOD_SICKNESS:
		{
			take_hit(Ind, damroll(6, 6), "poisonous food.");
			(void)do_dec_stat(Ind, A_CON);
			ident = TRUE;
			break;
		}

		case SV_FOOD_STUPIDITY:
		{
			take_hit(Ind, damroll(8, 8), "poisonous food.");
			(void)do_dec_stat(Ind, A_INT);
			ident = TRUE;
			break;
		}

		case SV_FOOD_NAIVETY:
		{
			take_hit(Ind, damroll(8, 8), "poisonous food.");
			(void)do_dec_stat(Ind, A_WIS);
			ident = TRUE;
			break;
		}

		case SV_FOOD_UNHEALTH:
		{
			take_hit(Ind, damroll(10, 10), "poisonous food.");
			(void)do_dec_stat(Ind, A_CON);
			ident = TRUE;
			break;
		}

		case SV_FOOD_DISEASE:
		{
			take_hit(Ind, damroll(10, 10), "poisonous food.");
			(void)do_dec_stat(Ind, A_STR);
			ident = TRUE;
			break;
		}

		case SV_FOOD_CURE_POISON:
		{
			if (set_poisoned(Ind, 0)) ident = TRUE;
			break;
		}

		case SV_FOOD_CURE_BLINDNESS:
		{
			if (set_blind(Ind, 0)) ident = TRUE;
			break;
		}

		case SV_FOOD_CURE_PARANOIA:
		{
			if (set_afraid(Ind, 0)) ident = TRUE;
			break;
		}

		case SV_FOOD_CURE_CONFUSION:
		{
			if (set_confused(Ind, 0)) ident = TRUE;
			break;
		}

		case SV_FOOD_CURE_SERIOUS:
		{
			if (hp_player(Ind, damroll(4, 8))) ident = TRUE;
			break;
		}

		case SV_FOOD_RESTORE_STR:
		{
			if (do_res_stat(Ind, A_STR)) ident = TRUE;
			break;
		}

		case SV_FOOD_RESTORE_CON:
		{
			if (do_res_stat(Ind, A_CON)) ident = TRUE;
			break;
		}

		case SV_FOOD_RESTORING:
		{
			if (do_res_stat(Ind, A_STR)) ident = TRUE;
			if (do_res_stat(Ind, A_INT)) ident = TRUE;
			if (do_res_stat(Ind, A_WIS)) ident = TRUE;
			if (do_res_stat(Ind, A_DEX)) ident = TRUE;
			if (do_res_stat(Ind, A_CON)) ident = TRUE;
			if (do_res_stat(Ind, A_CHR)) ident = TRUE;
			break;
		}


		case SV_FOOD_RATION:
		case SV_FOOD_BISCUIT:
		case SV_FOOD_JERKY:
		case SV_FOOD_SLIME_MOLD:
		{
			msg_print(Ind, "That tastes good.");
			ident = TRUE;
			break;
		}

		case SV_FOOD_WAYBREAD:
		{
			msg_print(Ind, "That tastes good.");
			(void)set_poisoned(Ind, 0);
			(void)hp_player(Ind, damroll(4, 8));
			ident = TRUE;
			break;
		}

		case SV_FOOD_PINT_OF_ALE:
		case SV_FOOD_PINT_OF_WINE:
		{
			msg_print(Ind, "That tastes good.");
			ident = TRUE;
			break;
		}

		case SV_FOOD_UNMAGIC:
		{
			/*
			set_adrenaline(Ind, 0);
			set_biofeedback(Ind, 0);
			set_tim_esp(Ind, 0);
			set_st_anchor(Ind, 0);
			set_prob_travel( Ind, 0);
			*/
			if (
				set_bow_brand(Ind, 0, 0, 0) |
				set_mimic(Ind, 0, 0) |
				set_tim_manashield(Ind, 0) |
				set_tim_traps(Ind, 0) |
				set_invis(Ind, 0, 0) |
				set_furry(Ind, 0) |
				set_tim_meditation(Ind, 0) |
//				set_tim_wraith(Ind, 0) |
				set_fast(Ind, 0) |
				set_shield(Ind, 0) |
				set_blessed(Ind, 0) |
				set_hero(Ind, 0) |
				set_shero(Ind, 0) |
				set_protevil(Ind, 0) |
				set_invuln(Ind, 0) |
				set_tim_invis(Ind, 0) |
				set_tim_infra(Ind, 0) |
				set_oppose_acid(Ind, 0) |
				set_oppose_elec(Ind, 0) |
				set_oppose_fire(Ind, 0) |
				set_oppose_cold(Ind, 0) |
				set_oppose_pois(Ind, 0)
				) ident = TRUE;

				/* In town it only runs out if you are not on a wall
				 * To prevent breaking into houses */
				if (players_on_depth[p_ptr->dun_depth] != 0) {
				/* important! check for illegal spaces */
					if (in_bounds(p_ptr->dun_depth, p_ptr->py, p_ptr->px)) {
						if ((p_ptr->dun_depth > 0) || (cave_floor_bold(p_ptr->dun_depth, p_ptr->py, p_ptr->px))) {
							if (set_tim_wraith(Ind, 0)) ident = TRUE;
						}
					}
				}

			break;
		}
	}


	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* We have tried it */
	object_tried(Ind, o_ptr);

	/* The player is now aware of the object */
	if (ident && !object_aware_p(Ind, o_ptr))
	{
		object_aware(Ind, o_ptr);
		gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Food can feed the player */
	(void)set_food(Ind, p_ptr->food + o_ptr->pval);


	/* Destroy a food in the pack */
	if (item >= 0)
	{
		inven_item_increase(Ind, item, -1);
		inven_item_describe(Ind, item);
		inven_item_optimize(Ind, item);
	}

	/* Destroy a food on the floor */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}
}




/*
 * Quaff a potion (from the pack or the floor)
 */
void do_cmd_quaff_potion(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	int		ident, lev;

	object_type	*o_ptr;


	/* Restrict choices to potions */
	item_tester_tval = TV_POTION;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

        if( check_guard_inscription( o_ptr->note, 'q' )) {
                msg_print(Ind, "The item's inscription prevents it");
                return;
        };

        if (!can_use(Ind, o_ptr))
        {
                msg_print(Ind, "You are not high level enough.");
		return;
        }


	if (o_ptr->tval != TV_POTION)
	{
		msg_print(Ind, "SERVER ERROR: Tried to quaff non-potion!");
		return;
	}

	/* Take a turn */
	p_ptr->energy -= level_speed(p_ptr->dun_depth);

	/* Not identified yet */
	ident = FALSE;

	/* Object level */
	lev = k_info[o_ptr->k_idx].level;

	/* Analyze the potion */
	switch (o_ptr->sval)
	{
		case SV_POTION_WATER:
		case SV_POTION_APPLE_JUICE:
		case SV_POTION_SLIME_MOLD:
		{
			msg_print(Ind, "\377GYou feel less thirsty.");
			ident = TRUE;
			break;
		}

		case SV_POTION_SLOWNESS:
		{
			if (set_slow(Ind, p_ptr->slow + randint(25) + 15)) ident = TRUE;
			break;
		}

		case SV_POTION_SALT_WATER:
		{
			msg_print(Ind, "The potion makes you vomit!");
			msg_format_near(Ind, "%s vomits!", p_ptr->name);
			/* made salt water less deadly -APD */
			(void)set_food(Ind, (p_ptr->food/2)-400);
			(void)set_poisoned(Ind, 0);
			(void)set_paralyzed(Ind, p_ptr->paralyzed + 4);
			ident = TRUE;
			break;
		}

		case SV_POTION_POISON:
		{
			if (!(p_ptr->resist_pois || p_ptr->oppose_pois))
			{
				if (set_poisoned(Ind, p_ptr->poisoned + rand_int(15) + 10))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_POTION_BLINDNESS:
		{
			if (!p_ptr->resist_blind)
			{
				if (set_blind(Ind, p_ptr->blind + rand_int(100) + 100))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_POTION_CONFUSION:
		{
			if (!p_ptr->resist_conf)
			{
				if (set_confused(Ind, p_ptr->confused + rand_int(20) + 15))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_POTION_SLEEP:
		{
			if (!p_ptr->free_act)
			{
				if (set_paralyzed(Ind, p_ptr->paralyzed + rand_int(4) + 4))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_POTION_LOSE_MEMORIES:
		{
			if (!p_ptr->hold_life && (p_ptr->exp > 0))
			{
				msg_print(Ind, "\377GYou feel your memories fade.");
				lose_exp(Ind, p_ptr->exp / 4);
				ident = TRUE;
			}
			break;
		}

		case SV_POTION_RUINATION:
		{
			msg_print(Ind, "Your nerves and muscles feel weak and lifeless!");
			take_hit(Ind, damroll(10, 10), "a potion of Ruination");
			(void)dec_stat(Ind, A_DEX, 25, TRUE);
			(void)dec_stat(Ind, A_WIS, 25, TRUE);
			(void)dec_stat(Ind, A_CON, 25, TRUE);
			(void)dec_stat(Ind, A_STR, 25, TRUE);
			(void)dec_stat(Ind, A_CHR, 25, TRUE);
			(void)dec_stat(Ind, A_INT, 25, TRUE);
			ident = TRUE;
			break;
		}

		case SV_POTION_DEC_STR:
		{
			if (do_dec_stat(Ind, A_STR)) ident = TRUE;
			break;
		}

		case SV_POTION_DEC_INT:
		{
			if (do_dec_stat(Ind, A_INT)) ident = TRUE;
			break;
		}

		case SV_POTION_DEC_WIS:
		{
			if (do_dec_stat(Ind, A_WIS)) ident = TRUE;
			break;
		}

		case SV_POTION_DEC_DEX:
		{
			if (do_dec_stat(Ind, A_DEX)) ident = TRUE;
			break;
		}

		case SV_POTION_DEC_CON:
		{
			if (do_dec_stat(Ind, A_CON)) ident = TRUE;
			break;
		}

		case SV_POTION_DEC_CHR:
		{
			if (do_dec_stat(Ind, A_CHR)) ident = TRUE;
			break;
		}

		case SV_POTION_DETONATIONS:
		{
			msg_print(Ind, "Massive explosions rupture your body!");
			msg_format_near(Ind, "%s blows up!", p_ptr->name);
			take_hit(Ind, damroll(50, 20), "a potion of Detonation");
			(void)set_stun(Ind, p_ptr->stun + 75);
			(void)set_cut(Ind, p_ptr->cut + 5000);
			ident = TRUE;
			break;
		}

		case SV_POTION_DEATH:
		{
			msg_print(Ind, "A feeling of Death flows through your body.");
			take_hit(Ind, 5000, "a potion of Death");
			ident = TRUE;
			break;
		}

		case SV_POTION_INFRAVISION:
		{
			if (set_tim_infra(Ind, p_ptr->tim_infra + 100 + randint(100)))
			{
				ident = TRUE;
			}
			break;
		}

		case SV_POTION_DETECT_INVIS:
		{
			if (set_tim_invis(Ind, p_ptr->tim_invis + 12 + randint(12)))
			{
				ident = TRUE;
			}
			break;
		}

		case SV_POTION_SLOW_POISON:
		{
			if (set_poisoned(Ind, p_ptr->poisoned / 2)) ident = TRUE;
			break;
		}

		case SV_POTION_CURE_POISON:
		{
			if (set_poisoned(Ind, 0)) ident = TRUE;
			break;
		}

		case SV_POTION_BOLDNESS:
		{
			if (set_afraid(Ind, 0)) ident = TRUE;
			break;
		}

		case SV_POTION_SPEED:
		{
			if (!p_ptr->fast)
			{
				if (set_fast(Ind, randint(25) + 15)) ident = TRUE;
			}
			else
			{
				(void)set_fast(Ind, p_ptr->fast + 5);
			}
			break;
		}

		case SV_POTION_RESIST_HEAT:
		{
			if (set_oppose_fire(Ind, p_ptr->oppose_fire + randint(10) + 10))
			{
				ident = TRUE;
			}
			break;
		}

		case SV_POTION_RESIST_COLD:
		{
			if (set_oppose_cold(Ind, p_ptr->oppose_cold + randint(10) + 10))
			{
				ident = TRUE;
			}
			break;
		}

		case SV_POTION_HEROISM:
		{
			if (hp_player(Ind, 10)) ident = TRUE;
			if (set_afraid(Ind, 0)) ident = TRUE;
			if (set_hero(Ind, p_ptr->hero + randint(25) + 25)) ident = TRUE;
			break;
		}

		case SV_POTION_BESERK_STRENGTH:
		{
			if (hp_player(Ind, 30)) ident = TRUE;
			if (set_afraid(Ind, 0)) ident = TRUE;
			if (set_shero(Ind, p_ptr->shero + randint(25) + 25)) ident = TRUE;
			break;
		}

		case SV_POTION_CURE_LIGHT:
		{
			if (hp_player(Ind, damroll(2, 8))) ident = TRUE;
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, p_ptr->cut - 10)) ident = TRUE;
			break;
		}

		case SV_POTION_CURE_SERIOUS:
		{
			if (hp_player(Ind, damroll(4, 8))) ident = TRUE;
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_confused(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, (p_ptr->cut / 2) - 50)) ident = TRUE;
			break;
		}

		case SV_POTION_CURE_CRITICAL:
		{
			if (hp_player(Ind, damroll(6, 8))) ident = TRUE;
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_confused(Ind, 0)) ident = TRUE;
			if (set_poisoned(Ind, 0)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0)) ident = TRUE;
			break;
		}

		case SV_POTION_HEALING:
		{
			if (hp_player(Ind, 300)) ident = TRUE;
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_confused(Ind, 0)) ident = TRUE;
			if (set_poisoned(Ind, 0)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0)) ident = TRUE;
			break;
		}

		case SV_POTION_STAR_HEALING:
		{
			if (hp_player(Ind, 1200)) ident = TRUE;
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_confused(Ind, 0)) ident = TRUE;
			if (set_poisoned(Ind, 0)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0)) ident = TRUE;
			break;
		}

		case SV_POTION_LIFE:
		{
			msg_print(Ind, "\377GYou feel life flow through your body!");
			restore_level(Ind);
			hp_player(Ind, 5000);
			(void)set_poisoned(Ind, 0);
			(void)set_blind(Ind, 0);
			(void)set_confused(Ind, 0);
			(void)set_image(Ind, 0);
			(void)set_stun(Ind, 0);
			(void)set_cut(Ind, 0);
			(void)do_res_stat(Ind, A_STR);
			(void)do_res_stat(Ind, A_CON);
			(void)do_res_stat(Ind, A_DEX);
			(void)do_res_stat(Ind, A_WIS);
			(void)do_res_stat(Ind, A_INT);
			(void)do_res_stat(Ind, A_CHR);
			ident = TRUE;
			break;
		}

		case SV_POTION_RESTORE_MANA:
		{
			if (p_ptr->csp < p_ptr->msp)
			{
//				p_ptr->csp += 300;
//				if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;
				p_ptr->csp = p_ptr->msp;
				p_ptr->csp_frac = 0;
				msg_print(Ind, "Your feel your head clear.");
				p_ptr->redraw |= (PR_MANA);
				p_ptr->window |= (PW_PLAYER);
				ident = TRUE;
			}
			break;
		}

		case SV_POTION_RESTORE_EXP:
		{
			if (restore_level(Ind)) ident = TRUE;
			break;
		}

		case SV_POTION_RES_STR:
		{
			if (do_res_stat(Ind, A_STR)) ident = TRUE;
			break;
		}

		case SV_POTION_RES_INT:
		{
			if (do_res_stat(Ind, A_INT)) ident = TRUE;
			break;
		}

		case SV_POTION_RES_WIS:
		{
			if (do_res_stat(Ind, A_WIS)) ident = TRUE;
			break;
		}

		case SV_POTION_RES_DEX:
		{
			if (do_res_stat(Ind, A_DEX)) ident = TRUE;
			break;
		}

		case SV_POTION_RES_CON:
		{
			if (do_res_stat(Ind, A_CON)) ident = TRUE;
			break;
		}

		case SV_POTION_RES_CHR:
		{
			if (do_res_stat(Ind, A_CHR)) ident = TRUE;
			break;
		}

		case SV_POTION_INC_STR:
		{
			if (do_inc_stat(Ind, A_STR)) ident = TRUE;
			break;
		}

		case SV_POTION_INC_INT:
		{
			if (do_inc_stat(Ind, A_INT)) ident = TRUE;
			break;
		}

		case SV_POTION_INC_WIS:
		{
			if (do_inc_stat(Ind, A_WIS)) ident = TRUE;
			break;
		}

		case SV_POTION_INC_DEX:
		{
			if (do_inc_stat(Ind, A_DEX)) ident = TRUE;
			break;
		}

		case SV_POTION_INC_CON:
		{
			if (do_inc_stat(Ind, A_CON)) ident = TRUE;
			break;
		}

		case SV_POTION_INC_CHR:
		{
			if (do_inc_stat(Ind, A_CHR)) ident = TRUE;
			break;
		}

		case SV_POTION_AUGMENTATION:
		{
			if (do_inc_stat(Ind, A_STR)) ident = TRUE;
			if (do_inc_stat(Ind, A_INT)) ident = TRUE;
			if (do_inc_stat(Ind, A_WIS)) ident = TRUE;
			if (do_inc_stat(Ind, A_DEX)) ident = TRUE;
			if (do_inc_stat(Ind, A_CON)) ident = TRUE;
			if (do_inc_stat(Ind, A_CHR)) ident = TRUE;
			break;
		}

		case SV_POTION_ENLIGHTENMENT:
		{
			msg_print(Ind, "An image of your surroundings forms in your mind...");
			wiz_lite(Ind);
			ident = TRUE;
			break;
		}

		case SV_POTION_STAR_ENLIGHTENMENT:
		{
			msg_print(Ind, "You begin to feel more enlightened...");
			msg_print(Ind, NULL);
			wiz_lite(Ind);
			(void)do_inc_stat(Ind, A_INT);
			(void)do_inc_stat(Ind, A_WIS);
			(void)detect_treasure(Ind);
			(void)detect_object(Ind);
			(void)detect_sdoor(Ind);
			(void)detect_trap(Ind);
			identify_pack(Ind);
			self_knowledge(Ind);
			ident = TRUE;
			break;
		}

		case SV_POTION_SELF_KNOWLEDGE:
		{
			msg_print(Ind, "You begin to know yourself a little better...");
			msg_print(Ind, NULL);
			self_knowledge(Ind);
			ident = TRUE;
			break;
		}

		case SV_POTION_EXPERIENCE:
		{
			if (p_ptr->exp < PY_MAX_EXP)
			{
				s32b ee = (p_ptr->exp / 2) + 10;
				if (ee > 100000L) ee = 100000L;
				msg_print(Ind, "\377GYou feel more experienced.");
				gain_exp(Ind, ee);
				ident = TRUE;
			}
			break;
		}

		case SV_POTION_INVIS:
		{
			if (set_invis(Ind, p_ptr->tim_invisibility + randint(20) + 20, p_ptr->tim_invis_power))
			{
				ident = TRUE;
			}
			break;
		}
	}


	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* The item has been tried */
	object_tried(Ind, o_ptr);

	/* An identification was made */
	if (ident && !object_aware_p(Ind, o_ptr))
	{
		object_aware(Ind, o_ptr);
		gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Potions can feed the player */
	(void)set_food(Ind, p_ptr->food + o_ptr->pval);


	/* Destroy a potion in the pack */
	if (item >= 0)
	{
		inven_item_increase(Ind, item, -1);
		inven_item_describe(Ind, item);
		inven_item_optimize(Ind, item);
	}

	/* Destroy a potion on the floor */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}
}


/*
 * Curse the players armor
 */
static bool curse_armor(int Ind)
{
	player_type *p_ptr = Players[Ind];

	object_type *o_ptr;

	char o_name[160];


	/* Curse the body armor */
	o_ptr = &p_ptr->inventory[INVEN_BODY];

	/* Nothing to curse */
	if (!o_ptr->k_idx) return (FALSE);


	/* Describe */
	object_desc(Ind, o_name, o_ptr, FALSE, 3);

	/* Attempt a saving throw for artifacts */
	if (artifact_p(o_ptr) && (rand_int(100) < 50))
	{
		/* Cool */
		msg_format(Ind, "A %s tries to %s, but your %s resists the effects!",
		           "terrible black aura", "surround your armor", o_name);
	}

	/* not artifact or failed save... */
	else
	{
		/* Oops */
		msg_format(Ind, "A terrible black aura blasts your %s!", o_name);

		/* Blast the armor */
		o_ptr->name1 = 0;
		o_ptr->name2 = EGO_BLASTED;
		o_ptr->name3 = 0;
		o_ptr->to_a = 0 - randint(5) - randint(5);
		o_ptr->to_h = 0;
		o_ptr->to_d = 0;
		o_ptr->ac = 0;
		o_ptr->dd = 0;
		o_ptr->ds = 0;

		/* Curse it */
		o_ptr->ident |= ID_CURSED;

		/* Break it */
		o_ptr->ident |= ID_BROKEN;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Recalculate mana */
		p_ptr->update |= (PU_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	}

	return (TRUE);
}


/*
 * Curse the players weapon
 */
static bool curse_weapon(int Ind)
{
	player_type *p_ptr = Players[Ind];

	object_type *o_ptr;

	char o_name[160];


	/* Curse the weapon */
	o_ptr = &p_ptr->inventory[INVEN_WIELD];

	/* Nothing to curse */
	if (!o_ptr->k_idx) return (FALSE);


	/* Describe */
	object_desc(Ind, o_name, o_ptr, FALSE, 3);

	/* Attempt a saving throw */
	if (artifact_p(o_ptr) && (rand_int(100) < 50))
	{
		/* Cool */
		msg_format(Ind, "A %s tries to %s, but your %s resists the effects!",
		           "terrible black aura", "surround your weapon", o_name);
	}

	/* not artifact or failed save... */
	else
	{
		/* Oops */
		msg_format(Ind, "A terrible black aura blasts your %s!", o_name);

		/* Shatter the weapon */
		o_ptr->name1 = 0;
		o_ptr->name2 = EGO_SHATTERED;
		o_ptr->name3 = 0;
		o_ptr->to_h = 0 - randint(5) - randint(5);
		o_ptr->to_d = 0 - randint(5) - randint(5);
		o_ptr->to_a = 0;
		o_ptr->ac = 0;
		o_ptr->dd = 0;
		o_ptr->ds = 0;

		/* Curse it */
		o_ptr->ident |= ID_CURSED;

		/* Break it */
		o_ptr->ident |= ID_BROKEN;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Recalculate mana */
		p_ptr->update |= (PU_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	}

	/* Notice */
	return (TRUE);
}


/*
 * Read a scroll (from the pack or floor).
 *
 * Certain scrolls can be "aborted" without losing the scroll.  These
 * include scrolls with no effects but recharge or identify, which are
 * cancelled before use.  XXX Reading them still takes a turn, though.
 */
 
 /*
 
 Added scroll of Life... uses vars x,y
-AD-
 */
 
void do_cmd_read_scroll(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];
	cave_type * c_ptr;

	int			k, used_up, ident, lev, x,y;

	object_type		*o_ptr;


	/* Check some conditions */
	if (p_ptr->blind)
	{
		msg_print(Ind, "You can't see anything.");
		return;
	}
	if (no_lite(Ind))
	{
		msg_print(Ind, "You have no light to read by.");
		return;
	}
	if (p_ptr->confused)
	{
		msg_print(Ind, "You are too confused!");
		return;
	}


	/* Restrict choices to scrolls */
	item_tester_tval = TV_SCROLL;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

        if( check_guard_inscription( o_ptr->note, 'r' )) {
                msg_print(Ind, "The item's inscription prevents it");
                return;
        };

	if (o_ptr->tval != TV_SCROLL)
	{
		msg_print(Ind, "SERVER ERROR: Tried to read non-scroll!");
		return;
	}

        if (!can_use(Ind, o_ptr))
        {
                msg_print(Ind, "You are not high level enough.");
		return;
        }

	/* Take a turn */
	p_ptr->energy -= level_speed(p_ptr->dun_depth);

	/* Not identified yet */
	ident = FALSE;

	/* Object level */
	lev = k_info[o_ptr->k_idx].level;

	/* Assume the scroll will get used up */
	used_up = TRUE;

	/* Analyze the scroll */
	switch (o_ptr->sval)
	{
		case SV_SCROLL_HOUSE:
		{
			unsigned char *ins=quark_str(o_ptr->note);
			bool floor=TRUE;
                        msg_print(Ind, "This is a house creation scroll.");
			ident = TRUE;
			if(ins){
				while((*ins!='\0')){
					if(*ins=='@'){
						ins++;
						if(*ins=='F'){
							floor=FALSE;
							break;
						}
					}
					ins++;
				}
			}
                        house_creation(Ind, floor);
			break;
		}
                case SV_SCROLL_GOLEM:
	        {	    
                        msg_print(Ind, "This is a golem creation scroll.");
			ident = TRUE;
                        golem_creation(Ind, 1);
			break;
            	}
	        case SV_SCROLL_BLOOD_BOND:
	        {
	    
	    		msg_print(Ind, "This is a blood bond scroll.");
			ident = TRUE;
			blood_bond(Ind, o_ptr);
			break;
            	}
	        case SV_SCROLL_ARTIFACT_CREATION:
	        {
	    
	    		msg_print(Ind, "This is an artifact creation scroll.");
			ident = TRUE;
			(void)create_artifact(Ind);
			used_up = FALSE;
			break;
            	}
		case SV_SCROLL_DARKNESS:
		{
			if (unlite_area(Ind, 10, 3)) ident = TRUE;
			if (!p_ptr->resist_blind)
			{
				(void)set_blind(Ind, p_ptr->blind + 3 + randint(5));
			}
			break;
		}

		case SV_SCROLL_AGGRAVATE_MONSTER:
		{
			msg_print(Ind, "There is a high pitched humming noise.");
			aggravate_monsters(Ind, 1);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_CURSE_ARMOR:
		{
			if (curse_armor(Ind)) ident = TRUE;
			break;
		}

		case SV_SCROLL_CURSE_WEAPON:
		{
			if (curse_weapon(Ind)) ident = TRUE;
			break;
		}

		case SV_SCROLL_SUMMON_MONSTER:
		{
			for (k = 0; k < randint(3); k++)
			{
				if (summon_specific(p_ptr->dun_depth, p_ptr->py, p_ptr->px, p_ptr->dun_depth, 0))
				{
					ident = TRUE;
				}
			}
			break;
		}
		
		/* not adding bounds checking now... because of perma walls
		   hope that I don't need to...... 
		   
		   OK, modified so you cant ressurect ghosts in walls......
		   to prevent bad things from happening in town.
		   */
		case SV_SCROLL_LIFE:
		{/*
			for (y = -1; y <= 1; y++)
			{
				for (x = -1; x <= 1; x++)
				{				
					c_ptr = &cave[p_ptr->dun_depth][p_ptr->py+y][p_ptr->px+x];
					
					if ((c_ptr->m_idx < 0) && (cave_floor_bold(p_ptr->dun_depth, p_ptr->py+y, p_ptr->px+x)))
					{
						if (Players[0 - c_ptr->m_idx]->ghost)
						{
							resurrect_player(0 - c_ptr->m_idx);
							break;
						}
					}
					
				}
			}
			*/
			restore_level(Ind);
			do_scroll_life(Ind);
			break;
		
		}
		
		case SV_SCROLL_SUMMON_UNDEAD:
		{
			for (k = 0; k < randint(3); k++)
			{
				if (summon_specific(p_ptr->dun_depth, p_ptr->py, p_ptr->px, p_ptr->dun_depth, SUMMON_UNDEAD))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_SCROLL_TRAP_CREATION:
		{
			if (trap_creation(Ind)) ident = TRUE;
			break;
		}

		case SV_SCROLL_PHASE_DOOR:
		{
			teleport_player(Ind, 10);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_TELEPORT:
		{
			teleport_player(Ind, 100);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_TELEPORT_LEVEL:
		{
			teleport_player_level(Ind);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_WORD_OF_RECALL:
		{
			if (p_ptr->word_recall == 0)
			{
				set_recall_depth(p_ptr, o_ptr);
				p_ptr->word_recall = randint(20) + 15;
				msg_print(Ind, "\377oThe air about you becomes charged...");
			}
			else
			{
				p_ptr->word_recall = 0;
				msg_print(Ind, "\377oA tension leaves the air around you...");
			}
			ident = TRUE;
			break;
		}

		case SV_SCROLL_IDENTIFY:
		{
			msg_print(Ind, "This is an identify scroll.");
			ident = TRUE;
			(void)ident_spell(Ind);
			used_up = FALSE;
			break;
		}

		case SV_SCROLL_STAR_IDENTIFY:
		{
			msg_print(Ind, "This is an *identify* scroll.");
			ident = TRUE;
			(void)identify_fully(Ind);
			used_up = FALSE;
			break;
		}

		case SV_SCROLL_REMOVE_CURSE:
		{
			if (remove_curse(Ind))
			{
				msg_print(Ind, "\377GYou feel as if someone is watching over you.");
				ident = TRUE;
			}
			break;
		}

		case SV_SCROLL_STAR_REMOVE_CURSE:
		{
			remove_all_curse(Ind);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_ENCHANT_ARMOR:
		{
			msg_print(Ind, "This is a scroll of enchant armor.");
			ident = TRUE;
			(void)enchant_spell(Ind, 0, 0, 1);
			used_up = FALSE;
			break;
		}

		case SV_SCROLL_ENCHANT_WEAPON_TO_HIT:
		{
			msg_print(Ind, "This is a scroll of enchant weapon to-hit.");
			(void)enchant_spell(Ind, 1, 0, 0);
			used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_ENCHANT_WEAPON_TO_DAM:
		{
			msg_print(Ind, "This is a scroll of enchant weapon to-dam.");
			(void)enchant_spell(Ind, 0, 1, 0);
			used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_ENCHANT_ARMOR:
		{
			msg_print(Ind, "This is a scroll of *enchant* armor.");
			(void)enchant_spell(Ind, 0, 0, randint(3) + 2);
			used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_ENCHANT_WEAPON:
		{
			msg_print(Ind, "This is a scroll of *enchant* weapon.");
			(void)enchant_spell(Ind, randint(3), randint(3), 0);
			used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_RECHARGING:
		{
			msg_print(Ind, "This is a scroll of recharging.");
			(void)recharge(Ind, 60);
			used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_LIGHT:
		{
			if (lite_area(Ind, damroll(2, 8), 2)) ident = TRUE;
			break;
		}

		case SV_SCROLL_MAPPING:
		{
			map_area(Ind);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_GOLD:
		{
			if (detect_treasure(Ind)) ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_ITEM:
		{
			if (detect_object(Ind)) ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_TRAP:
		{
			if (detect_trap(Ind)) ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_DOOR:
		{
			if (detect_sdoor(Ind)) ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_INVIS:
		{
			if (detect_invisible(Ind)) ident = TRUE;
			break;
		}

		case SV_SCROLL_SATISFY_HUNGER:
		{
			if (set_food(Ind, PY_FOOD_MAX - 1)) ident = TRUE;
			break;
		}

		case SV_SCROLL_BLESSING:
		{
			if (set_blessed(Ind, p_ptr->blessed + randint(12) + 6)) ident = TRUE;
			break;
		}

		case SV_SCROLL_HOLY_CHANT:
		{
			if (set_blessed(Ind, p_ptr->blessed + randint(24) + 12)) ident = TRUE;
			break;
		}

		case SV_SCROLL_HOLY_PRAYER:
		{
			if (set_blessed(Ind, p_ptr->blessed + randint(48) + 24)) ident = TRUE;
			break;
		}

		case SV_SCROLL_MONSTER_CONFUSION:
		{
			if (p_ptr->confusing == 0)
			{
				msg_print(Ind, "Your hands begin to glow.");
				p_ptr->confusing = TRUE;
				ident = TRUE;
			}
			break;
		}

		case SV_SCROLL_PROTECTION_FROM_EVIL:
		{
			k = 3 * p_ptr->lev;
			if (set_protevil(Ind, p_ptr->protevil + randint(25) + k)) ident = TRUE;
			break;
		}

		case SV_SCROLL_RUNE_OF_PROTECTION:
		{
			warding_glyph(Ind);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_TRAP_DOOR_DESTRUCTION:
		{
			if (destroy_doors_touch(Ind)) ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_DESTRUCTION:
		{
			destroy_area(p_ptr->dun_depth, p_ptr->py, p_ptr->px, 15, TRUE);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_DISPEL_UNDEAD:
		{
			if (dispel_undead(Ind, 60)) ident = TRUE;
			break;
		}

		case SV_SCROLL_GENOCIDE:
		{
			msg_print(Ind, "This is a genocide scroll.");
			(void)genocide(Ind);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_MASS_GENOCIDE:
		{
			msg_print(Ind, "This is a mass genocide scroll.");
			(void)mass_genocide(Ind);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_ACQUIREMENT:
		{
			acquirement(p_ptr->dun_depth, p_ptr->py, p_ptr->px, 1, TRUE);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_ACQUIREMENT:
		{
			acquirement(p_ptr->dun_depth, p_ptr->py, p_ptr->px, randint(2) + 1, TRUE);
			ident = TRUE;
			break;
		}
	}


	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* The item was tried */
	object_tried(Ind, o_ptr);

	/* An identification was made */
	if (ident && !object_aware_p(Ind, o_ptr))
	{
		object_aware(Ind, o_ptr);
		gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);



	/* Destroy a scroll in the pack */
	if (item >= 0)
	{
		inven_item_increase(Ind, item, -1);

		/* Hack -- allow certain scrolls to be "preserved" */
		if (!used_up) return;

		inven_item_describe(Ind, item);
		inven_item_optimize(Ind, item);
	}

	/* Destroy a scroll on the floor */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}
}







/*
 * Use a staff.			-RAK-
 *
 * One charge of one staff disappears.
 *
 * Hack -- staffs of identify can be "cancelled".
 */
void do_cmd_use_staff(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	int			ident, chance, k, lev;

	object_type		*o_ptr;

	/* Hack -- let staffs of identify get aborted */
	bool use_charge = TRUE;

	
	if (p_ptr->anti_magic)
	{
		msg_print(Ind, "An anti-magic shield disrupts your attempts.");	
		return;
	}
	if (p_ptr->pclass == CLASS_UNBELIEVER)
	{
		msg_print(Ind, "You don't believe in magic.");	
		return;
	}

	/* Restrict choices to wands */
	item_tester_tval = TV_STAFF;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	if( check_guard_inscription( o_ptr->note, 'u' )) {
                msg_print(Ind, "The item's inscription prevents it");
                return;
        }; 

	if (o_ptr->tval != TV_STAFF)
	{
		msg_print(Ind, "SERVER ERROR: Tried to use non-staff!");
		return;
	}

        if (!can_use(Ind, o_ptr))
        {
                msg_print(Ind, "You are not high level enough.");
		return;
        }

	/* Mega-Hack -- refuse to use a pile from the ground */
	if ((item < 0) && (o_ptr->number > 1))
	{
		msg_print(Ind, "You must first pick up the staffs.");
		return;
	}

	/* Verify potential overflow */
	/*if ((inven_cnt >= INVEN_PACK) &&
	    (o_ptr->number > 1))
	{*/
		/* Verify with the player */
		/*if (other_query_flag &&
		    !get_check("Your pack might overflow.  Continue? ")) return;
	}*/


	/* Take a turn */
	p_ptr->energy -= level_speed(p_ptr->dun_depth);

	/* Not identified yet */
	ident = FALSE;

	/* Extract the item level */
	lev = k_info[o_ptr->k_idx].level;

	/* Base chance of success */
	chance = p_ptr->skill_dev;

	/* Confusion hurts skill */
	if (p_ptr->confused) chance = chance / 2;

	/* Hight level objects are harder */
	chance = chance - ((lev > 50) ? 50 : lev);

	/* Give everyone a (slight) chance */
	if ((chance < USE_DEVICE) && (rand_int(USE_DEVICE - chance + 1) == 0))
	{
		chance = USE_DEVICE;
	}

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint(chance) < USE_DEVICE))
	{
		if (flush_failure) flush();
		msg_print(Ind, "You failed to use the staff properly.");
		return;
	}

	/* Notice empty staffs */
	if (o_ptr->pval <= 0)
	{
		if (flush_failure) flush();
		msg_print(Ind, "The staff has no charges left.");
		o_ptr->ident |= ID_EMPTY;
		
		/* Redraw */
		p_ptr->window |= (PW_INVEN);

		return;
	}


	/* Analyze the staff */
	switch (o_ptr->sval)
	{
		case SV_STAFF_DARKNESS:
		{
			if (unlite_area(Ind, 10, 3)) ident = TRUE;
			if (!p_ptr->resist_blind)
			{
				if (set_blind(Ind, p_ptr->blind + 3 + randint(5))) ident = TRUE;
			}
			break;
		}

		case SV_STAFF_SLOWNESS:
		{
			if (set_slow(Ind, p_ptr->slow + randint(30) + 15)) ident = TRUE;
			break;
		}

		case SV_STAFF_HASTE_MONSTERS:
		{
			if (speed_monsters(Ind)) ident = TRUE;
			break;
		}

		case SV_STAFF_SUMMONING:
		{
			for (k = 0; k < randint(4); k++)
			{
				if (summon_specific(p_ptr->dun_depth, p_ptr->py, p_ptr->px, p_ptr->dun_depth, 0))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_STAFF_TELEPORTATION:
		{
			msg_format_near(Ind, "%s teleports away!", p_ptr->name);
			teleport_player(Ind, 100);
			ident = TRUE;
			break;
		}

		case SV_STAFF_IDENTIFY:
		{
			if (!ident_spell(Ind)) use_charge = FALSE;
			ident = TRUE;
			break;
		}

		case SV_STAFF_REMOVE_CURSE:
		{
			if (remove_curse(Ind))
			{
				if (!p_ptr->blind)
				{
					msg_print(Ind, "The staff glows blue for a moment...");
				}
				ident = TRUE;
			}
			break;
		}

		case SV_STAFF_STARLITE:
		{
			if (!p_ptr->blind)
			{
				msg_print(Ind, "The end of the staff glows brightly...");
			}
			for (k = 0; k < 8; k++) lite_line(Ind, ddd[k]);
			ident = TRUE;
			break;
		}

		case SV_STAFF_LITE:
		{
			msg_format_near(Ind, "%s calls light.", p_ptr->name);
			if (lite_area(Ind, damroll(2, 8), 2)) ident = TRUE;
			break;
		}

		case SV_STAFF_MAPPING:
		{
			map_area(Ind);
			ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_GOLD:
		{
			if (detect_treasure(Ind)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_ITEM:
		{
			if (detect_object(Ind)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_TRAP:
		{
			if (detect_trap(Ind)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_DOOR:
		{
			if (detect_sdoor(Ind)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_INVIS:
		{
			if (detect_invisible(Ind)) ident = TRUE;
			break;
		}

		case SV_STAFF_DETECT_EVIL:
		{
			if (detect_evil(Ind)) ident = TRUE;
			break;
		}

		case SV_STAFF_CURE_LIGHT:
		{
			if (hp_player(Ind, randint(8))) ident = TRUE;
			break;
		}

		case SV_STAFF_CURING:
		{
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_poisoned(Ind, 0)) ident = TRUE;
			if (set_confused(Ind, 0)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0)) ident = TRUE;
			break;
		}

		case SV_STAFF_HEALING:
		{
			if (hp_player(Ind, 300)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0)) ident = TRUE;
			break;
		}

		case SV_STAFF_THE_MAGI:
		{
			if (do_res_stat(Ind, A_INT)) ident = TRUE;
			if (p_ptr->csp < p_ptr->msp)
			{
				p_ptr->csp = p_ptr->msp;
				p_ptr->csp_frac = 0;
				ident = TRUE;
				msg_print(Ind, "Your feel your head clear.");
				p_ptr->redraw |= (PR_MANA);
				p_ptr->window |= (PW_PLAYER);
			}
			break;
		}

		case SV_STAFF_SLEEP_MONSTERS:
		{
			if (sleep_monsters(Ind)) ident = TRUE;
			break;
		}

		case SV_STAFF_SLOW_MONSTERS:
		{
			if (slow_monsters(Ind)) ident = TRUE;
			break;
		}

		case SV_STAFF_SPEED:
		{
			if (!p_ptr->fast)
			{
				if (set_fast(Ind, randint(30) + 15)) ident = TRUE;
			}
			else
			{
				(void)set_fast(Ind, p_ptr->fast + 5);
			}
			break;
		}

		case SV_STAFF_PROBING:
		{
			probing(Ind);
			ident = TRUE;
			break;
		}

		case SV_STAFF_DISPEL_EVIL:
		{
			if (dispel_evil(Ind, 60)) ident = TRUE;
			break;
		}

		case SV_STAFF_POWER:
		{
			if (dispel_monsters(Ind, 120)) ident = TRUE;
			break;
		}

		case SV_STAFF_HOLINESS:
		{
			if (dispel_evil(Ind, 120)) ident = TRUE;
			k = 3 * p_ptr->lev;
			if (set_protevil(Ind, p_ptr->protevil + randint(25) + k)) ident = TRUE;
			if (set_poisoned(Ind, 0)) ident = TRUE;
			if (set_afraid(Ind, 0)) ident = TRUE;
			if (hp_player(Ind, 50)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0)) ident = TRUE;
			break;
		}

		case SV_STAFF_GENOCIDE:
		{
			(void)genocide(Ind);
			ident = TRUE;
			break;
		}

		case SV_STAFF_EARTHQUAKES:
		{
			msg_format_near(Ind, "%s causes the ground to shake!", p_ptr->name);
			earthquake(p_ptr->dun_depth, p_ptr->py, p_ptr->px, 10);
			ident = TRUE;
			break;
		}

		case SV_STAFF_DESTRUCTION:
		{
			msg_format_near(Ind, "%s unleashes great power!", p_ptr->name);
			destroy_area(p_ptr->dun_depth, p_ptr->py, p_ptr->px, 15, TRUE);
			ident = TRUE;
			break;
		}
	}


	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Tried the item */
	object_tried(Ind, o_ptr);

	/* An identification was made */
	if (ident && !object_aware_p(Ind, o_ptr))
	{
		object_aware(Ind, o_ptr);
		gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Hack -- some uses are "free" */
	if (!use_charge) return;


	/* Use a single charge */
	o_ptr->pval--;

	/* XXX Hack -- unstack if necessary */
	if ((item >= 0) && (o_ptr->number > 1))
	{
		/* Make a fake item */
		object_type tmp_obj;
		tmp_obj = *o_ptr;
		tmp_obj.number = 1;

		/* Restore the charges */
		o_ptr->pval++;

		/* Unstack the used item */
		o_ptr->number--;
		p_ptr->total_weight -= tmp_obj.weight;
		item = inven_carry(Ind, &tmp_obj);

		/* Message */
		msg_print(Ind, "You unstack your staff.");
	}

	/* Describe charges in the pack */
	if (item >= 0)
	{
		inven_item_charges(Ind, item);
	}

	/* Describe charges on the floor */
	else
	{
		floor_item_charges(0 - item);
	}
}


/*
 * Aim a wand (from the pack or floor).
 *
 * Use a single charge from a single item.
 * Handle "unstacking" in a logical manner.
 *
 * For simplicity, you cannot use a stack of items from the
 * ground.  This would require too much nasty code.
 *
 * There are no wands which can "destroy" themselves, in the inventory
 * or on the ground, so we can ignore this possibility.  Note that this
 * required giving "wand of wonder" the ability to ignore destruction
 * by electric balls.
 *
 * All wands can be "cancelled" at the "Direction?" prompt for free.
 *
 * Note that the basic "bolt" wands do slightly less damage than the
 * basic "bolt" rods, but the basic "ball" wands do the same damage
 * as the basic "ball" rods.
 */
void do_cmd_aim_wand(int Ind, int item, int dir)
{
	player_type *p_ptr = Players[Ind];

	int			lev, ident, chance, sval;

	object_type		*o_ptr;

	if (p_ptr->anti_magic)
	{
		msg_print(Ind, "An anti-magic shield disrupts your attempts.");	
		return;
	}
	if (p_ptr->pclass == CLASS_UNBELIEVER)
	{
		msg_print(Ind, "You don't believe in magic.");	
		return;
	}

	/* Restrict choices to wands */
	item_tester_tval = TV_WAND;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}
	if( check_guard_inscription( o_ptr->note, 'a' )) {
                msg_print(Ind, "The item's inscription prevents it");
                return;
        }; 

	if (o_ptr->tval != TV_WAND)
	{
		msg_print(Ind, "SERVER ERROR: Tried to use non-wand!");
		return;
	}

        if (!can_use(Ind, o_ptr))
        {
                msg_print(Ind, "You are not high level enough.");
		return;
        }

	/* Mega-Hack -- refuse to aim a pile from the ground */
	if ((item < 0) && (o_ptr->number > 1))
	{
		msg_print(Ind, "You must first pick up the wands.");
		return;
	}

	/* Hack -- verify potential overflow */
	/*if ((inven_cnt >= INVEN_PACK) &&
	    (o_ptr->number > 1))
	{*/
		/* Verify with the player */
	/*	if (other_query_flag &&
		    !get_check("Your pack might overflow.  Continue? ")) return;
	}*/


	/* Allow direction to be cancelled for free */
	/*if (!get_aim_dir(&dir)) return;*/


	/* Take a turn */
	p_ptr->energy -= level_speed(p_ptr->dun_depth);

	/* Not identified yet */
	ident = FALSE;

	/* Get the level */
	lev = k_info[o_ptr->k_idx].level;

	/* Base chance of success */
	chance = p_ptr->skill_dev;

	/* Confusion hurts skill */
	if (p_ptr->confused) chance = chance / 2;

	/* Hight level objects are harder */
	chance = chance - ((lev > 50) ? 50 : lev);

	/* Give everyone a (slight) chance */
	if ((chance < USE_DEVICE) && (rand_int(USE_DEVICE - chance + 1) == 0))
	{
		chance = USE_DEVICE;
	}

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint(chance) < USE_DEVICE))
	{
		if (flush_failure) flush();
		msg_print(Ind, "You failed to use the wand properly.");
		return;
	}

	/* The wand is already empty! */
	if (o_ptr->pval <= 0)
	{
		if (flush_failure) flush();
		msg_print(Ind, "The wand has no charges left.");
		o_ptr->ident |= ID_EMPTY;

		/* Redraw */
		p_ptr->window |= (PW_INVEN);

		return;
	}



	/* XXX Hack -- Extract the "sval" effect */
	sval = o_ptr->sval;

	/* XXX Hack -- Wand of wonder can do anything before it */
	if (sval == SV_WAND_WONDER) sval = rand_int(SV_WAND_WONDER);

	/* Analyze the wand */
	switch (sval)
	{
		case SV_WAND_HEAL_MONSTER:
		{
			if (heal_monster(Ind, dir)) ident = TRUE;
			break;
		}

		case SV_WAND_HASTE_MONSTER:
		{
			if (speed_monster(Ind, dir)) ident = TRUE;
			break;
		}

		case SV_WAND_CLONE_MONSTER:
		{
			if (clone_monster(Ind, dir)) ident = TRUE;
			break;
		}

		case SV_WAND_TELEPORT_AWAY:
		{
			if (teleport_monster(Ind, dir)) ident = TRUE;
			break;
		}

		case SV_WAND_DISARMING:
		{
			if (disarm_trap(Ind, dir)) ident = TRUE;
			break;
		}

		case SV_WAND_TRAP_DOOR_DEST:
		{
			if (destroy_door(Ind, dir)) ident = TRUE;
			break;
		}

		case SV_WAND_STONE_TO_MUD:
		{
			if (wall_to_mud(Ind, dir)) ident = TRUE;
			break;
		}

		case SV_WAND_LITE:
		{
			msg_print(Ind, "A line of blue shimmering light appears.");
			lite_line(Ind, dir);
			ident = TRUE;
			break;
		}

		case SV_WAND_SLEEP_MONSTER:
		{
			if (sleep_monster(Ind, dir)) ident = TRUE;
			break;
		}

		case SV_WAND_SLOW_MONSTER:
		{
			if (slow_monster(Ind, dir)) ident = TRUE;
			break;
		}

		case SV_WAND_CONFUSE_MONSTER:
		{
			if (confuse_monster(Ind, dir, 10)) ident = TRUE;
			break;
		}

		case SV_WAND_FEAR_MONSTER:
		{
			if (fear_monster(Ind, dir, 10)) ident = TRUE;
			break;
		}

		case SV_WAND_DRAIN_LIFE:
		{
			if (drain_life(Ind, dir, 75)) ident = TRUE;
			break;
		}

		case SV_WAND_POLYMORPH:
		{
			if (poly_monster(Ind, dir)) ident = TRUE;
			break;
		}

		case SV_WAND_STINKING_CLOUD:
		{
			msg_format_near(Ind, "%s fires a stinking cloud.", p_ptr->name);
			fire_ball(Ind, GF_POIS, dir, 12, 2);
			ident = TRUE;
			break;
		}

		case SV_WAND_MAGIC_MISSILE:
		{
			msg_format_near(Ind, "%s fires a magic missile.", p_ptr->name);
			fire_bolt_or_beam(Ind, 20, GF_MISSILE, dir, damroll(2, 6));
			ident = TRUE;
			break;
		}

		case SV_WAND_ACID_BOLT:
		{
			msg_format_near(Ind, "%s fires an acid bolt.", p_ptr->name);
			fire_bolt_or_beam(Ind, 20, GF_ACID, dir, damroll(5, 8));
			ident = TRUE;
			break;
		}

		case SV_WAND_ELEC_BOLT:
		{
			msg_format_near(Ind, "%s fires a lightning bolt.", p_ptr->name);
			fire_bolt_or_beam(Ind, 20, GF_ELEC, dir, damroll(3, 8));
			ident = TRUE;
			break;
		}

		case SV_WAND_FIRE_BOLT:
		{
			msg_format_near(Ind, "%s fires a fire bolt.", p_ptr->name);
			fire_bolt_or_beam(Ind, 20, GF_FIRE, dir, damroll(6, 8));
			ident = TRUE;
			break;
		}

		case SV_WAND_COLD_BOLT:
		{
			msg_format_near(Ind, "%s fires a frost bolt.", p_ptr->name);
			fire_bolt_or_beam(Ind, 20, GF_COLD, dir, damroll(3, 8));
			ident = TRUE;
			break;
		}

		case SV_WAND_ACID_BALL:
		{
			msg_format_near(Ind, "%s fires a ball of acid.", p_ptr->name);
			fire_ball(Ind, GF_ACID, dir, 60, 2);
			ident = TRUE;
			break;
		}

		case SV_WAND_ELEC_BALL:
		{
			msg_format_near(Ind, "%s fires a ball of electricity.", p_ptr->name);
			fire_ball(Ind, GF_ELEC, dir, 32, 2);
			ident = TRUE;
			break;
		}

		case SV_WAND_FIRE_BALL:
		{
			msg_format_near(Ind, "%s fires a fire ball.", p_ptr->name);
			fire_ball(Ind, GF_FIRE, dir, 72, 2);
			ident = TRUE;
			break;
		}

		case SV_WAND_COLD_BALL:
		{
			msg_format_near(Ind, "%s fires a frost ball.", p_ptr->name);
			fire_ball(Ind, GF_COLD, dir, 48, 2);
			ident = TRUE;
			break;
		}

		case SV_WAND_WONDER:
		{
			msg_print(Ind, "SERVER ERROR: Oops.  Wand of wonder activated.");
			break;
		}

		case SV_WAND_DRAGON_FIRE:
		{
			msg_format_near(Ind, "%s shoots dragon fire!", p_ptr->name);
			fire_ball(Ind, GF_FIRE, dir, 100, 3);
			ident = TRUE;
			break;
		}

		case SV_WAND_DRAGON_COLD:
		{
			msg_format_near(Ind, "%s shoots dragon frost!", p_ptr->name);
			fire_ball(Ind, GF_COLD, dir, 80, 3);
			ident = TRUE;
			break;
		}

		case SV_WAND_DRAGON_BREATH:
		{
			switch (randint(5))
			{
				case 1:
				{
					msg_format_near(Ind, "%s shoots dragon acid!", p_ptr->name);
					fire_ball(Ind, GF_ACID, dir, 100, 3);
					break;
				}

				case 2:
				{
					msg_format_near(Ind, "%s shoots dragon lightning!", p_ptr->name);
					fire_ball(Ind, GF_ELEC, dir, 80, 3);
					break;
				}

				case 3:
				{
					msg_format_near(Ind, "%s shoots dragon fire!", p_ptr->name);
					fire_ball(Ind, GF_FIRE, dir, 100, 3);
					break;
				}

				case 4:
				{
					msg_format_near(Ind, "%s shoots dragon frost!", p_ptr->name);
					fire_ball(Ind, GF_COLD, dir, 80, 3);
					break;
				}

				default:
				{
					msg_format_near(Ind, "%s shoots dragon poison!", p_ptr->name);
					fire_ball(Ind, GF_POIS, dir, 60, 3);
					break;
				}
			}

			ident = TRUE;
			break;
		}

		case SV_WAND_ANNIHILATION:
		{
			if (drain_life(Ind, dir, 125)) ident = TRUE;
			break;
		}
	}


	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Mark it as tried */
	object_tried(Ind, o_ptr);

	/* Apply identification */
	if (ident && !object_aware_p(Ind, o_ptr))
	{
		object_aware(Ind, o_ptr);
		gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Use a single charge */
	o_ptr->pval--;

	/* Hack -- unstack if necessary */
	if ((item >= 0) && (o_ptr->number > 1))
	{
		/* Make a fake item */
		object_type tmp_obj;
		tmp_obj = *o_ptr;
		tmp_obj.number = 1;

		/* Restore the charges */
		o_ptr->pval++;

		/* Unstack the used item */
		o_ptr->number--;
		p_ptr->total_weight -= tmp_obj.weight;
		item = inven_carry(Ind, &tmp_obj);

		/* Message */
		msg_print(Ind, "You unstack your wand.");
	}

	/* Describe the charges in the pack */
	if (item >= 0)
	{
		inven_item_charges(Ind, item);
	}

	/* Describe the charges on the floor */
	else
	{
		floor_item_charges(0 - item);
	}
}





/*
 * Activate (zap) a Rod
 *
 * Unstack fully charged rods as needed.
 *
 * Hack -- rods of perception/genocide can be "cancelled"
 * All rods can be cancelled at the "Direction?" prompt
 */
void do_cmd_zap_rod(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	int                 ident, chance, lev;

	object_type		*o_ptr;

	/* Hack -- let perception get aborted */
	bool use_charge = TRUE;

	if (p_ptr->anti_magic)
	{
		msg_print(Ind, "An anti-magic shield disrupts your attempts.");	
		return;
	}
	if (p_ptr->pclass == CLASS_UNBELIEVER)
	{
		msg_print(Ind, "You don't believe in magic.");	
		return;
	}

	/* Restrict choices to rods */
	item_tester_tval = TV_ROD;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}
	if( check_guard_inscription( o_ptr->note, 'z' )) {
                msg_print(Ind, "The item's inscription prevents it");
                return;
        }; 


	if (o_ptr->tval != TV_ROD)
	{
		msg_print(Ind, "SERVER ERROR: Tried to zap non-rod!");
		return;
	}

	/* Mega-Hack -- refuse to zap a pile from the ground */
	if ((item < 0) && (o_ptr->number > 1))
	{
		msg_print(Ind, "You must first pick up the rods.");
		return;
	}

        if (!can_use(Ind, o_ptr))
        {
                msg_print(Ind, "You are not high level enough.");
		return;
        }

	/* Get a direction (unless KNOWN not to need it) */
	if ((o_ptr->sval >= SV_ROD_MIN_DIRECTION) || !object_aware_p(Ind, o_ptr))
	{
		/* Get a direction, then return */
		p_ptr->current_rod = item;
		get_aim_dir(Ind);
		return;
	}


	/* Take a turn */
	p_ptr->energy -= level_speed(p_ptr->dun_depth);

	/* Not identified yet */
	ident = FALSE;

	/* Extract the item level */
	lev = k_info[o_ptr->k_idx].level;

	/* Base chance of success */
	chance = p_ptr->skill_dev;

	/* Confusion hurts skill */
	if (p_ptr->confused) chance = chance / 2;

	/* Hight level objects are harder */
	chance = chance - ((lev > 50) ? 50 : lev);

	/* Give everyone a (slight) chance */
	if ((chance < USE_DEVICE) && (rand_int(USE_DEVICE - chance + 1) == 0))
	{
		chance = USE_DEVICE;
	}

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint(chance) < USE_DEVICE))
	{
		if (flush_failure) flush();
		msg_print(Ind, "You failed to use the rod properly.");
		return;
	}

	/* Still charging */
	if (o_ptr->pval)
	{
		if (flush_failure) flush();
		msg_print(Ind, "The rod is still charging.");
		return;
	}


	/* Analyze the rod */
	switch (o_ptr->sval)
	{
		case SV_ROD_DETECT_TRAP:
		{
			if (detect_trap(Ind)) ident = TRUE;
			o_ptr->pval = 50;
			break;
		}

		case SV_ROD_DETECT_DOOR:
		{
			if (detect_sdoor(Ind)) ident = TRUE;
			o_ptr->pval = 70;
			break;
		}

		case SV_ROD_IDENTIFY:
		{
			ident = TRUE;
			if (!ident_spell(Ind)) use_charge = FALSE;
			o_ptr->pval = 10;
			break;
		}

		case SV_ROD_RECALL:
		{
			if (p_ptr->word_recall == 0)
			{
				set_recall_depth(p_ptr,o_ptr);
				msg_print(Ind, "\377oThe air about you becomes charged...");
				p_ptr->word_recall = 15 + randint(20);
			}
			else
			{
				msg_print(Ind, "\377oA tension leaves the air around you...");
				p_ptr->word_recall = 0;
			}
			ident = TRUE;
			o_ptr->pval = 60;
			break;
		}

		case SV_ROD_ILLUMINATION:
		{
			msg_format_near(Ind, "%s calls light.", p_ptr->name);
			if (lite_area(Ind, damroll(2, 8), 2)) ident = TRUE;
			o_ptr->pval = 30;
			break;
		}

		case SV_ROD_MAPPING:
		{
			map_area(Ind);
			ident = TRUE;
			o_ptr->pval = 99;
			break;
		}

		case SV_ROD_DETECTION:
		{
			detection(Ind);
			ident = TRUE;
			o_ptr->pval = 99;
			break;
		}

		case SV_ROD_PROBING:
		{
			probing(Ind);
			ident = TRUE;
			o_ptr->pval = 50;
			break;
		}

		case SV_ROD_CURING:
		{
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_poisoned(Ind, 0)) ident = TRUE;
			if (set_confused(Ind, 0)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0)) ident = TRUE;
			o_ptr->pval = 999;
			break;
		}

		case SV_ROD_HEALING:
		{
			if (hp_player(Ind, 500)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0)) ident = TRUE;
			o_ptr->pval = 999;
			break;
		}

		case SV_ROD_RESTORATION:
		{
			if (restore_level(Ind)) ident = TRUE;
			if (do_res_stat(Ind, A_STR)) ident = TRUE;
			if (do_res_stat(Ind, A_INT)) ident = TRUE;
			if (do_res_stat(Ind, A_WIS)) ident = TRUE;
			if (do_res_stat(Ind, A_DEX)) ident = TRUE;
			if (do_res_stat(Ind, A_CON)) ident = TRUE;
			if (do_res_stat(Ind, A_CHR)) ident = TRUE;
			o_ptr->pval = 999;
			break;
		}

		case SV_ROD_SPEED:
		{
			if (!p_ptr->fast)
			{
				if (set_fast(Ind, randint(30) + 15)) ident = TRUE;
			}
			else
			{
				(void)set_fast(Ind, p_ptr->fast + 5);
			}
			o_ptr->pval = 99;
			break;
		}

		default:
		{
			msg_print(Ind, "SERVER ERROR: Directional rod zapped in non-directional function!");
			return;
		}
	}


	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Tried the object */
	object_tried(Ind, o_ptr);

	/* Successfully determined the object function */
	if (ident && !object_aware_p(Ind, o_ptr))
	{
		object_aware(Ind, o_ptr);
		gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Hack -- deal with cancelled zap */
	if (!use_charge)
	{
		o_ptr->pval = 0;
		return;
	}


	/* XXX Hack -- unstack if necessary */
	if ((item >= 0) && (o_ptr->number > 1))
	{
		/* Make a fake item */
		object_type tmp_obj;
		tmp_obj = *o_ptr;
		tmp_obj.number = 1;

		/* Restore "charge" */
		o_ptr->pval = 0;

		/* Unstack the used item */
		o_ptr->number--;
		p_ptr->total_weight -= tmp_obj.weight;
		item = inven_carry(Ind, &tmp_obj);

		/* Message */
		msg_print(Ind, "You unstack your rod.");
	}
}



/*
 * Activate (zap) a Rod
 *
 * Unstack fully charged rods as needed.
 *
 * Hack -- rods of perception/genocide can be "cancelled"
 * All rods can be cancelled at the "Direction?" prompt
 */
void do_cmd_zap_rod_dir(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];

	int                 item, ident, chance, lev;

	object_type		*o_ptr;

	/* Hack -- let perception get aborted */
	bool use_charge = TRUE;


	item = p_ptr->current_rod;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}
	if( check_guard_inscription( o_ptr->note, 'z' )) {
                msg_print(Ind, "The item's inscription prevents it");
                return;
        }; 

	if (o_ptr->tval != TV_ROD)
	{
		msg_print(Ind, "SERVER ERROR: Tried to zap non-rod!");
		return;
	}

	/* Mega-Hack -- refuse to zap a pile from the ground */
	if ((item < 0) && (o_ptr->number > 1))
	{
		msg_print(Ind, "You must first pick up the rods.");
		return;
	}

        if (!can_use(Ind, o_ptr))
        {
                msg_print(Ind, "You are not high level enough.");
		return;
        }

	/* Hack -- verify potential overflow */
	/*if ((inven_cnt >= INVEN_PACK) &&
	    (o_ptr->number > 1))
	{*/
		/* Verify with the player */
		/*if (other_query_flag &&
		    !get_check("Your pack might overflow.  Continue? ")) return;
	}*/

	/* Take a turn */
	p_ptr->energy -= level_speed(p_ptr->dun_depth);

	/* Not identified yet */
	ident = FALSE;

	/* Extract the item level */
	lev = k_info[o_ptr->k_idx].level;

	/* Base chance of success */
	chance = p_ptr->skill_dev;

	/* Confusion hurts skill */
	if (p_ptr->confused) chance = chance / 2;

	/* Hight level objects are harder */
	chance = chance - ((lev > 50) ? 50 : lev);

	/* Give everyone a (slight) chance */
	if ((chance < USE_DEVICE) && (rand_int(USE_DEVICE - chance + 1) == 0))
	{
		chance = USE_DEVICE;
	}

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint(chance) < USE_DEVICE))
	{
		if (flush_failure) flush();
		msg_print(Ind, "You failed to use the rod properly.");
		return;
	}

	/* Still charging */
	if (o_ptr->pval)
	{
		if (flush_failure) flush();
		msg_print(Ind, "The rod is still charging.");
		return;
	}


	/* Analyze the rod */
	switch (o_ptr->sval)
	{
		case SV_ROD_TELEPORT_AWAY:
		{
			if (teleport_monster(Ind, dir)) ident = TRUE;
			o_ptr->pval = 25;
			break;
		}

		case SV_ROD_DISARMING:
		{
			if (disarm_trap(Ind, dir)) ident = TRUE;
			o_ptr->pval = 30;
			break;
		}

		case SV_ROD_LITE:
		{
			msg_print(Ind, "A line of blue shimmering light appears.");
			lite_line(Ind, dir);
			ident = TRUE;
			o_ptr->pval = 9;
			break;
		}

		case SV_ROD_SLEEP_MONSTER:
		{
			if (sleep_monster(Ind, dir)) ident = TRUE;
			o_ptr->pval = 18;
			break;
		}

		case SV_ROD_SLOW_MONSTER:
		{
			if (slow_monster(Ind, dir)) ident = TRUE;
			o_ptr->pval = 20;
			break;
		}

		case SV_ROD_DRAIN_LIFE:
		{
			if (drain_life(Ind, dir, 75)) ident = TRUE;
			o_ptr->pval = 23;
			break;
		}

		case SV_ROD_POLYMORPH:
		{
			if (poly_monster(Ind, dir)) ident = TRUE;
			o_ptr->pval = 25;
			break;
		}

		case SV_ROD_ACID_BOLT:
		{
			msg_format_near(Ind, "%s fires an acid bolt.", p_ptr->name);
			fire_bolt_or_beam(Ind, 10, GF_ACID, dir, damroll(6, 8));
			ident = TRUE;
			o_ptr->pval = 12;
			break;
		}

		case SV_ROD_ELEC_BOLT:
		{
			msg_format_near(Ind, "%s fires a lightning bolt.", p_ptr->name);
			fire_bolt_or_beam(Ind, 10, GF_ELEC, dir, damroll(3, 8));
			ident = TRUE;
			o_ptr->pval = 11;
			break;
		}

		case SV_ROD_FIRE_BOLT:
		{
			msg_format_near(Ind, "%s fires a fire bolt.", p_ptr->name);
			fire_bolt_or_beam(Ind, 10, GF_FIRE, dir, damroll(8, 8));
			ident = TRUE;
			o_ptr->pval = 15;
			break;
		}

		case SV_ROD_COLD_BOLT:
		{
			msg_format_near(Ind, "%s fires a frost bolt.", p_ptr->name);
			fire_bolt_or_beam(Ind, 10, GF_COLD, dir, damroll(5, 8));
			ident = TRUE;
			o_ptr->pval = 13;
			break;
		}

		case SV_ROD_ACID_BALL:
		{
			msg_format_near(Ind, "%s fires an acid ball.", p_ptr->name);
			fire_ball(Ind, GF_ACID, dir, 60, 2);
			ident = TRUE;
			o_ptr->pval = 27;
			break;
		}

		case SV_ROD_ELEC_BALL:
		{
			msg_format_near(Ind, "%s fires a lightning ball.", p_ptr->name);
			fire_ball(Ind, GF_ELEC, dir, 32, 2);
			ident = TRUE;
			o_ptr->pval = 23;
			break;
		}

		case SV_ROD_FIRE_BALL:
		{
			msg_format_near(Ind, "%s fires a fire ball.", p_ptr->name);
			fire_ball(Ind, GF_FIRE, dir, 72, 2);
			ident = TRUE;
			o_ptr->pval = 30;
			break;
		}

		case SV_ROD_COLD_BALL:
		{
			msg_format_near(Ind, "%s fires a frost ball.", p_ptr->name);
			fire_ball(Ind, GF_COLD, dir, 48, 2);
			ident = TRUE;
			o_ptr->pval = 25;
			break;
		}

		/* All of the following are needed if we tried zapping one of */
		/* these but we didn't know what it was. */
		case SV_ROD_DETECT_TRAP:
		{
			if (detect_trap(Ind)) ident = TRUE;
			o_ptr->pval = 50;
			break;
		}

		case SV_ROD_DETECT_DOOR:
		{
			if (detect_sdoor(Ind)) ident = TRUE;
			o_ptr->pval = 70;
			break;
		}

		case SV_ROD_IDENTIFY:
		{
			ident = TRUE;
			if (!ident_spell(Ind)) use_charge = FALSE;
			o_ptr->pval = 10;
			break;
		}

		case SV_ROD_RECALL:
		{
			if (p_ptr->word_recall == 0)
			{
				set_recall_depth(p_ptr,o_ptr);
				msg_print(Ind, "\377oThe air about you becomes charged...");
				p_ptr->word_recall = 15 + randint(20);
			}
			else
			{
				msg_print(Ind, "\377oA tension leaves the air around you...");
				p_ptr->word_recall = 0;
			}
			ident = TRUE;
			o_ptr->pval = 60;
			break;
		}

		case SV_ROD_ILLUMINATION:
		{
			msg_format_near(Ind, "%s calls light.", p_ptr->name);
			if (lite_area(Ind, damroll(2, 8), 2)) ident = TRUE;
			o_ptr->pval = 30;
			break;
		}

		case SV_ROD_MAPPING:
		{
			map_area(Ind);
			ident = TRUE;
			o_ptr->pval = 99;
			break;
		}

		case SV_ROD_DETECTION:
		{
			detection(Ind);
			ident = TRUE;
			o_ptr->pval = 99;
			break;
		}

		case SV_ROD_PROBING:
		{
			probing(Ind);
			ident = TRUE;
			o_ptr->pval = 50;
			break;
		}

		case SV_ROD_CURING:
		{
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_poisoned(Ind, 0)) ident = TRUE;
			if (set_confused(Ind, 0)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0)) ident = TRUE;
			o_ptr->pval = 999;
			break;
		}

		case SV_ROD_HEALING:
		{
			if (hp_player(Ind, 500)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0)) ident = TRUE;
			o_ptr->pval = 999;
			break;
		}

		case SV_ROD_RESTORATION:
		{
			if (restore_level(Ind)) ident = TRUE;
			if (do_res_stat(Ind, A_STR)) ident = TRUE;
			if (do_res_stat(Ind, A_INT)) ident = TRUE;
			if (do_res_stat(Ind, A_WIS)) ident = TRUE;
			if (do_res_stat(Ind, A_DEX)) ident = TRUE;
			if (do_res_stat(Ind, A_CON)) ident = TRUE;
			if (do_res_stat(Ind, A_CHR)) ident = TRUE;
			o_ptr->pval = 999;
			break;
		}

		case SV_ROD_SPEED:
		{
			if (!p_ptr->fast)
			{
				if (set_fast(Ind, randint(30) + 15)) ident = TRUE;
			}
			else
			{
				(void)set_fast(Ind, p_ptr->fast + 5);
			}
			o_ptr->pval = 99;
			break;
		}

		default:
		{
			msg_print(Ind, "SERVER ERROR: Tried to zap non-directional rod in directional function!");
			return;
		}
	}


	/* Clear the current rod */
	p_ptr->current_rod = -1;

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Tried the object */
	object_tried(Ind, o_ptr);

	/* Successfully determined the object function */
	if (ident && !object_aware_p(Ind, o_ptr))
	{
		object_aware(Ind, o_ptr);
		gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Hack -- deal with cancelled zap */
	if (!use_charge)
	{
		o_ptr->pval = 0;
		return;
	}


	/* XXX Hack -- unstack if necessary */
	if ((item >= 0) && (o_ptr->number > 1))
	{
		/* Make a fake item */
		object_type tmp_obj;
		tmp_obj = *o_ptr;
		tmp_obj.number = 1;

		/* Restore "charge" */
		o_ptr->pval = 0;

		/* Unstack the used item */
		o_ptr->number--;
		p_ptr->total_weight -= tmp_obj.weight;
		item = inven_carry(Ind, &tmp_obj);

		/* Message */
		msg_print(Ind, "You unstack your rod.");
	}
}



/*
 * Hook to determine if an object is activatable
 */
static bool item_tester_hook_activate(int Ind, object_type *o_ptr)
{
	u32b f1, f2, f3;

	/* Not known */
	if (!object_known_p(Ind, o_ptr)) return (FALSE);

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3);

	/* Check activation flag */
	if (f3 & TR3_ACTIVATE) return (TRUE);

	/* Assume not */
	return (FALSE);
}



/*
 * Hack -- activate the ring of power
 */
static void ring_of_power(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];

	/* Pick a random effect */
	switch (randint(10))
	{
		case 1:
		case 2:
		{
			/* Message */
			msg_print(Ind, "You are surrounded by a malignant aura.");

			/* Decrease all stats (permanently) */
			(void)dec_stat(Ind, A_STR, 50, TRUE);
			(void)dec_stat(Ind, A_INT, 50, TRUE);
			(void)dec_stat(Ind, A_WIS, 50, TRUE);
			(void)dec_stat(Ind, A_DEX, 50, TRUE);
			(void)dec_stat(Ind, A_CON, 50, TRUE);
			(void)dec_stat(Ind, A_CHR, 50, TRUE);

			/* Lose some experience (permanently) */
			p_ptr->exp -= (p_ptr->exp / 4);
			p_ptr->max_exp -= (p_ptr->exp / 4);
			check_experience(Ind);

			break;
		}

		case 3:
		{
			/* Message */
			msg_print(Ind, "You are surrounded by a powerful aura.");

			/* Dispel monsters */
			dispel_monsters(Ind, 1000);

			break;
		}

		case 4:
		case 5:
		case 6:
		{
			/* Mana Ball */
			fire_ball(Ind, GF_MANA, dir, 300, 3);

			break;
		}

		case 7:
		case 8:
		case 9:
		case 10:
		{
			/* Mana Bolt */
			fire_bolt(Ind, GF_MANA, dir, 250);

			break;
		}
	}
}




/*
 * Enchant some bolts
 */
static bool brand_bolts(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int i;

	/* Use the first (XXX) acceptable bolts */
	for (i = 0; i < INVEN_PACK; i++)
	{
		object_type *o_ptr = &p_ptr->inventory[i];

		/* Skip non-bolts */
		if (o_ptr->tval != TV_BOLT) continue;

		/* Skip artifacts and ego-items */
		if (artifact_p(o_ptr) || ego_item_p(o_ptr)) continue;

		/* Skip cursed/broken items */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) continue;

		/* Randomize */
		if (rand_int(100) < 75) continue;

		/* Message */
		msg_print(Ind, "Your bolts are covered in a fiery aura!");

		/* Ego-item */
		o_ptr->name2 = EGO_FLAME;

		/* Enchant */
		enchant(Ind, o_ptr, rand_int(3) + 4, ENCH_TOHIT | ENCH_TODAM);

		/* Notice */
		return (TRUE);
	}

	/* Flush */
	if (flush_failure) flush();

	/* Fail */
	msg_print(Ind, "The fiery enchantment failed.");

	/* Notice */
	return (TRUE);
}


/*
 * Activate a wielded object.  Wielded objects never stack.
 * And even if they did, activatable objects never stack.
 *
 * Currently, only (some) artifacts, and Dragon Scale Mail, can be activated.
 * But one could, for example, easily make an activatable "Ring of Plasma".
 *
 * Note that it always takes a turn to activate an artifact, even if
 * the user hits "escape" at the "direction" prompt.
 */
void do_cmd_activate(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	int         i, k, lev, chance;

	object_type *o_ptr;


	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	if( check_guard_inscription( o_ptr->note, 'A' )) {
                msg_print(Ind, "The item's inscription prevents it");
                return;
        }; 

        if (!can_use(Ind, o_ptr))
        {
                msg_print(Ind, "You are not high level enough.");
		return;
        }

	/* Test the item */
	if (!item_tester_hook_activate(Ind, o_ptr))
	{
		msg_print(Ind, "You cannot activate that item.");
		return;
	}

	/* Take a turn */
	p_ptr->energy -= level_speed(p_ptr->dun_depth);

	/* Extract the item level */
	lev = k_info[o_ptr->k_idx].level;

	/* Hack -- use artifact level instead */
	if (artifact_p(o_ptr)) lev = a_info[o_ptr->name1].level;

	/* Base chance of success */
	chance = p_ptr->skill_dev;

	/* Confusion hurts skill */
	if (p_ptr->confused) chance = chance / 2;

	/* Hight level objects are harder */
	chance = chance - ((lev > 50) ? 50 : lev);

	/* Give everyone a (slight) chance */
	if ((chance < USE_DEVICE) && (rand_int(USE_DEVICE - chance + 1) == 0))
	{
		chance = USE_DEVICE;
	}

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint(chance) < USE_DEVICE))
	{
		if (flush_failure) flush();
		msg_print(Ind, "You failed to activate it properly.");
		return;
	}

	/* Check the recharge */
	if (o_ptr->timeout)
	{
		msg_print(Ind, "It whines, glows and fades...");
		return;
	}


	/* Wonder Twin Powers... Activate! */
	msg_print(Ind, "You activate it...");

	/* Hack -- Dragon Scale Mail can be activated as well */
	if (o_ptr->tval == TV_DRAG_ARMOR)
	{
		  switch (o_ptr->sval)
		    {
		    case SV_DRAGON_BLACK:
		      {
			do_mimic_change(Ind, 429);
		      break;
		      }
		    case SV_DRAGON_BLUE:
		      {
		      do_mimic_change(Ind, 411);
		      break;
		      }
		    case SV_DRAGON_WHITE:
		      {
			do_mimic_change(Ind, 424);
			break;
		      }
		    case SV_DRAGON_RED:
		      {
			do_mimic_change(Ind, 444);
			break;
		      }
		    case SV_DRAGON_GREEN:
		      {
			do_mimic_change(Ind, 425);
			break;
		      }
		    case SV_DRAGON_MULTIHUED:
		      {
			do_mimic_change(Ind, 462);
			break;
		      }
		    case SV_DRAGON_SHINING:
		      {
			do_mimic_change(Ind, 463);
			break;
		      }
		    case SV_DRAGON_LAW:
		      {
			do_mimic_change(Ind, 520);
			break;
		      }
		    case SV_DRAGON_BRONZE:
		      {
			do_mimic_change(Ind, 412);
			break;
		    }
		    case SV_DRAGON_GOLD:
		      {
			do_mimic_change(Ind, 445);
			break;
		      }
		    case SV_DRAGON_CHAOS:
		      {
			do_mimic_change(Ind, 519);
			break;
		      }
		    case SV_DRAGON_BALANCE:
		      {
			do_mimic_change(Ind, 521);
			break;
		      }
		    case SV_DRAGON_POWER:
		      {
			do_mimic_change(Ind, 549);
			break;
		      }
		    }
		  o_ptr->timeout = 200 + rand_int(100);
		  return;
	}
        if (o_ptr->tval == TV_GOLEM)
        {
                int m_idx = 0, k;
                monster_type *m_ptr;

                /* Process the monsters */
                for (k = m_top - 1; k >= 0; k--)
                {
                        /* Access the index */
                        i = m_fast[k];

                        /* Access the monster */
                        m_ptr = &m_list[i];

                        /* Excise "dead" monsters */
                        if (!m_ptr->r_idx) continue;

                        if (m_ptr->owner != p_ptr->id) continue;

                        m_idx = i;
                        if (!m_idx) continue;
                        m_ptr = &m_list[m_idx];

                        if (!(m_ptr->r_ptr->extra & (1 << (o_ptr->sval - 200))))
                        {
                                msg_print(Ind, "I do not understand, master.");
                                continue;
                        }

                        switch (o_ptr->sval)
                        {
                                case SV_GOLEM_ATTACK:
                                        if (m_ptr->mind & (1 << (o_ptr->sval - 200)))
                                        {
                                                msg_print(Ind, "I wont attack your target anymore, master.");
                                                m_ptr->mind &= ~(1 << (o_ptr->sval - 200));
                                        }
                                        else
                                        {
                                                msg_print(Ind, "I will attack your target, master.");
                                                m_ptr->mind |= (1 << (o_ptr->sval - 200));
                                        }
                                        break;
                                case SV_GOLEM_FOLLOW:
                                        if (m_ptr->mind & (1 << (o_ptr->sval - 200)))
                                        {
                                                msg_print(Ind, "I wont follow you, master.");
                                                m_ptr->mind &= ~(1 << (o_ptr->sval - 200));
                                        }
                                        else
                                        {
                                                msg_print(Ind, "I will follow you, master.");
                                                m_ptr->mind |= (1 << (o_ptr->sval - 200));
                                        }
                                        break;
                                case SV_GOLEM_GUARD:
                                        if (m_ptr->mind & (1 << (o_ptr->sval - 200)))
                                        {
                                                msg_print(Ind, "I wont guard my position anymore, master.");
                                                m_ptr->mind &= ~(1 << (o_ptr->sval - 200));
                                        }
                                        else
                                        {
                                                msg_print(Ind, "I will guard my position, master.");
                                                m_ptr->mind |= (1 << (o_ptr->sval - 200));
                                        }
                                        break;
                        }
                }
                return;
        }

	/* Artifacts activate by name */
	if (o_ptr->name1)
	{
		/* This needs to be changed */
		switch (o_ptr->name1)
		{
			case ART_NARTHANC:
			{
				msg_print(Ind, "Your dagger is covered in fire...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_NIMTHANC:
			{
				msg_print(Ind, "Your dagger is covered in frost...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_DETHANC:
			{
				msg_print(Ind, "Your dagger is covered in sparks...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_RILIA:
			{
				msg_print(Ind, "Your dagger throbs deep green...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_BELANGIL:
			{
				msg_print(Ind, "Your dagger is covered in frost...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_DAL:
			{
				msg_print(Ind, "\377GYou feel energy flow through your feet...");
				(void)set_afraid(Ind, 0);
				(void)set_poisoned(Ind, 0);
				o_ptr->timeout = 5;
				break;
			}

			case ART_RINGIL:
			{
				msg_print(Ind, "Your sword glows an intense blue...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_ANDURIL:
			{
				msg_print(Ind, "Your sword glows an intense red...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_FIRESTAR:
			{
				msg_print(Ind, "Your morningstar rages in fire...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_FEANOR:
			{
				if (!p_ptr->fast)
				{
					(void)set_fast(Ind, randint(20) + 20);
				}
				else
				{
					(void)set_fast(Ind, p_ptr->fast + 5);
				}
				o_ptr->timeout = 200;
				break;
			}

			case ART_THEODEN:
			{
				msg_print(Ind, "The blade of your axe glows black...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_TURMIL:
			{
				msg_print(Ind, "The head of your hammer glows white...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_CASPANION:
			{
				msg_print(Ind, "Your armor glows bright red...");
				destroy_doors_touch(Ind);
				o_ptr->timeout = 10;
				break;
			}

			case ART_AVAVIR:
			{
				if (p_ptr->word_recall == 0)
				{
					set_recall_depth(p_ptr,o_ptr);
					p_ptr->word_recall = randint(20) + 15;
					msg_print(Ind, "\377oThe air about you becomes charged...");
				}
				else
				{
					p_ptr->word_recall = 0;
					msg_print(Ind, "\377oA tension leaves the air around you...");
				}
				o_ptr->timeout = 200;
				break;
			}

			case ART_TARATOL:
			{
				if (!p_ptr->fast)
				{
					(void)set_fast(Ind, randint(20) + 20);
				}
				else
				{
					(void)set_fast(Ind, p_ptr->fast + 5);
				}
				o_ptr->timeout = rand_int(100) + 100;
				break;
			}

			case ART_ERIRIL:
			{
				/* Identify and combine pack */
				(void)ident_spell(Ind);
				/* XXX Note that the artifact is always de-charged */
				o_ptr->timeout = 10;
				break;
			}

			case ART_OLORIN:
			{
				probing(Ind);
				o_ptr->timeout = 20;
				break;
			}

			case ART_EONWE:
			{
				msg_print(Ind, "Your axe lets out a long, shrill note...");
				(void)mass_genocide(Ind);
				o_ptr->timeout = 1000;
				break;
			}

			case ART_LOTHARANG:
			{
				msg_print(Ind, "Your battle axe radiates deep purple...");
				hp_player(Ind, damroll(4, 8));
				(void)set_cut(Ind, (p_ptr->cut / 2) - 50);
				o_ptr->timeout = rand_int(3) + 3;
				break;
			}

			case ART_CUBRAGOL:
			{
				(void)brand_bolts(Ind);
				o_ptr->timeout = 999;
				break;
			}

			case ART_ARUNRUTH:
			{
				msg_print(Ind, "Your sword glows a pale blue...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_AEGLOS:
			{
				msg_print(Ind, "Your spear glows a bright white...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_OROME:
			{
				msg_print(Ind, "Your spear pulsates...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_SOULKEEPER:
			{
				msg_print(Ind, "Your armor glows a bright white...");
				msg_print(Ind, "\377GYou feel much better...");
				(void)hp_player(Ind, 1000);
				(void)set_cut(Ind, 0);
				o_ptr->timeout = 888;
				break;
			}

			case ART_BELEGENNON:
			{
				teleport_player(Ind, 10);
				o_ptr->timeout = 2;
				break;
			}

			case ART_CELEBORN:
			{
				(void)genocide(Ind);
				o_ptr->timeout = 500;
				break;
			}

			case ART_LUTHIEN:
			{
				restore_level(Ind);
				o_ptr->timeout = 450;
				break;
			}

			case ART_ULMO:
			{
				msg_print(Ind, "Your trident glows deep red...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_COLLUIN:
			{
				msg_print(Ind, "Your cloak glows many colours...");
				(void)set_oppose_acid(Ind, p_ptr->oppose_acid + randint(20) + 20);
				(void)set_oppose_elec(Ind, p_ptr->oppose_elec + randint(20) + 20);
				(void)set_oppose_fire(Ind, p_ptr->oppose_fire + randint(20) + 20);
				(void)set_oppose_cold(Ind, p_ptr->oppose_cold + randint(20) + 20);
				(void)set_oppose_pois(Ind, p_ptr->oppose_pois + randint(20) + 20);
				o_ptr->timeout = 111;
				break;
			}

			case ART_HOLCOLLETH:
			{
				msg_print(Ind, "Your cloak glows deep blue...");
				sleep_monsters_touch(Ind);
				o_ptr->timeout = 55;
				break;
			}

			case ART_THINGOL:
			{
				msg_print(Ind, "You hear a low humming noise...");
				recharge(Ind, 60);
				o_ptr->timeout = 70;
				break;
			}

			case ART_COLANNON:
			{
				teleport_player(Ind, 100);
				o_ptr->timeout = 45;
				break;
			}

			case ART_TOTILA:
			{
				msg_print(Ind, "Your flail glows in scintillating colours...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_CAMMITHRIM:
			{
				msg_print(Ind, "Your gloves glow extremely brightly...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_PAURHACH:
			{
				msg_print(Ind, "Your gauntlets are covered in fire...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_PAURNIMMEN:
			{
				msg_print(Ind, "Your gauntlets are covered in frost...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_PAURAEGEN:
			{
				msg_print(Ind, "Your gauntlets are covered in sparks...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_PAURNEN:
			{
				msg_print(Ind, "Your gauntlets look very acidic...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_FINGOLFIN:
			{
				msg_print(Ind, "Magical spikes appear on your cesti...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_HOLHENNETH:
			{
				msg_print(Ind, "An image forms in your mind...");
				detection(Ind);
				o_ptr->timeout = rand_int(55) + 55;
				break;
			}

			case ART_GONDOR:
			{
				msg_print(Ind, "\377GYou feel a warm tingling inside...");
				(void)hp_player(Ind, 500);
				(void)set_cut(Ind, 0);
				o_ptr->timeout = 500;
				break;
			}

			case ART_RAZORBACK:
			{
				msg_print(Ind, "You are surrounded by lightning!");
				for (i = 0; i < 8; i++) fire_ball(Ind, GF_ELEC, ddd[i], 150, 3);
				o_ptr->timeout = 1000;
				break;
			}

			case ART_BLADETURNER:
			{
				msg_print(Ind, "Your armor glows many colours...");
				(void)hp_player(Ind, 30);
				(void)set_afraid(Ind, 0);
				(void)set_shero(Ind, p_ptr->shero + randint(50) + 50);
				(void)set_blessed(Ind, p_ptr->blessed + randint(50) + 50);
				(void)set_oppose_acid(Ind, p_ptr->oppose_acid + randint(50) + 50);
				(void)set_oppose_elec(Ind, p_ptr->oppose_elec + randint(50) + 50);
				(void)set_oppose_fire(Ind, p_ptr->oppose_fire + randint(50) + 50);
				(void)set_oppose_cold(Ind, p_ptr->oppose_cold + randint(50) + 50);
				(void)set_oppose_pois(Ind, p_ptr->oppose_pois + randint(50) + 50);
				o_ptr->timeout = 400;
				break;
			}


			case ART_GALADRIEL:
			{
				msg_print(Ind, "The phial wells with clear light...");
				lite_area(Ind, damroll(2, 15), 3);
				o_ptr->timeout = rand_int(10) + 10;
				break;
			}

			case ART_ELENDIL:
			{
				msg_print(Ind, "The star shines brightly...");
				map_area(Ind);
				o_ptr->timeout = rand_int(50) + 50;
				break;
			}

			case ART_THRAIN:
			{
				msg_print(Ind, "The stone glows a deep green...");
				wiz_lite(Ind);
				(void)detect_sdoor(Ind);
				(void)detect_trap(Ind);
				o_ptr->timeout = rand_int(100) + 100;
				break;
			}


			case ART_INGWE:
			{
				msg_print(Ind, "An aura of good floods the area...");
				dispel_evil(Ind, p_ptr->lev * 5);
				o_ptr->timeout = rand_int(300) + 300;
				break;
			}

			case ART_CARLAMMAS:
			{
				msg_print(Ind, "The amulet lets out a shrill wail...");
				k = 3 * p_ptr->lev;
				(void)set_protevil(Ind, p_ptr->protevil + randint(25) + k);
				o_ptr->timeout = rand_int(225) + 225;
				break;
			}


			case ART_TULKAS:
			{
				msg_print(Ind, "The ring glows brightly...");
				if (!p_ptr->fast)
				{
					(void)set_fast(Ind, randint(75) + 75);
				}
				else
				{
					(void)set_fast(Ind, p_ptr->fast + 5);
				}
				o_ptr->timeout = rand_int(150) + 150;
				break;
			}

			case ART_NARYA:
			{
				msg_print(Ind, "The ring glows deep red...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_NENYA:
			{
				msg_print(Ind, "The ring glows bright white...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_VILYA:
			{
				msg_print(Ind, "The ring glows deep blue...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}

			case ART_POWER:
			{
				msg_print(Ind, "The ring glows intensely black...");
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			}
		}

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Done */
		return;
	}

	/* Some ego items can be activated */
	else if (o_ptr->name2)
	{
		switch (o_ptr->name2)
		{
			case EGO_CLOAK_LORDLY_RES:
			{
				msg_print(Ind, "Your cloak flashes many colors...");

				(void)set_oppose_acid(Ind, p_ptr->oppose_acid + randint(40) + 40);
				(void)set_oppose_elec(Ind, p_ptr->oppose_elec + randint(40) + 40);
				(void)set_oppose_fire(Ind, p_ptr->oppose_fire + randint(40) + 40);
				(void)set_oppose_cold(Ind, p_ptr->oppose_cold + randint(40) + 40);
				(void)set_oppose_pois(Ind, p_ptr->oppose_pois + randint(40) + 40);

				o_ptr->timeout = rand_int(50) + 150;
				break;
			}
		}
		/* Done ego item activation */
		return;
	}

	/* Amulets of the moon can be activated for sleep monster */
	if ((o_ptr->tval == TV_AMULET) && (o_ptr->sval == SV_AMULET_THE_MOON))
	{
		msg_print(Ind, "Your amulet glows a deep blue...");
		sleep_monsters(Ind);

		o_ptr->timeout = rand_int(100) + 100;
		return;
	}


	if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH) && (p_ptr->pclass == CLASS_MIMIC))
	  {
	    /* If never used before, then set to the player form, otehrwise set the player form*/
	    if (!o_ptr->pval)
	      {
		msg_format(Ind, "The form of the ring seems to change to a small %s.", r_info[p_ptr->body_monster].name + r_name);
		o_ptr->pval = p_ptr->body_monster;
	      }
	    else
	      {
		monster_race *r_ptr = &r_info[o_ptr->pval];

		if ((r_ptr->level > p_ptr->lev * 2) || (p_ptr->r_killed[o_ptr->pval] < r_ptr->level))
		  {
		    msg_print(Ind, "You dont match the ring yet.");
		    return;
		  }

		msg_print(Ind, "You polymorph !");
		p_ptr->body_monster = o_ptr->pval;
		p_ptr->body_changed = TRUE;
       	
		p_ptr->update |= (PU_BONUS);

		/* Recalculate mana */
		p_ptr->update |= (PU_MANA | PU_HP);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	      }
	    return;
	  }

	/* Mistake */
	msg_print(Ind, "That object cannot be activated.");
}


void do_cmd_activate_dir(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;

	int item, chance;

	item = p_ptr->current_activation;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	if( check_guard_inscription( o_ptr->note, 'A' )) {
                msg_print(Ind, "The item's inscription prevents it");
                return;
        }; 

        if (!can_use(Ind, o_ptr))
        {
                msg_print(Ind, "You are not high level enough.");
		return;
        }

	/* Artifacts activate by name */
	if (o_ptr->name1)
	{
		/* This needs to be changed */
		switch (o_ptr->name1)
		{
			case ART_NARTHANC:
			{
				fire_bolt(Ind, GF_FIRE, dir, damroll(9, 8));
				o_ptr->timeout = rand_int(8) + 8;
				break;
			}

			case ART_NIMTHANC:
			{
				fire_bolt(Ind, GF_COLD, dir, damroll(6, 8));
				o_ptr->timeout = rand_int(7) + 7;
				break;
			}

			case ART_DETHANC:
			{
				fire_bolt(Ind, GF_ELEC, dir, damroll(4, 8));
				o_ptr->timeout = rand_int(6) + 6;
				break;
			}

			case ART_RILIA:
			{
				fire_ball(Ind, GF_POIS, dir, 12, 3);
				o_ptr->timeout = rand_int(4) + 4;
				break;
			}

			case ART_BELANGIL:
			{
				fire_ball(Ind, GF_COLD, dir, 48, 2);
				o_ptr->timeout = rand_int(5) + 5;
				break;
			}

			case ART_RINGIL:
			{
				fire_ball(Ind, GF_COLD, dir, 100, 2);
				o_ptr->timeout = 300;
				break;
			}

			case ART_ANDURIL:
			{
				fire_ball(Ind, GF_FIRE, dir, 72, 2);
				o_ptr->timeout = 400;
				break;
			}

			case ART_FIRESTAR:
			{
				fire_ball(Ind, GF_FIRE, dir, 72, 3);
				o_ptr->timeout = 100;
				break;
			}

			case ART_THEODEN:
			{
				drain_life(Ind, dir, 120);
				o_ptr->timeout = 400;
				break;
			}

			case ART_TURMIL:
			{
				drain_life(Ind, dir, 90);
				o_ptr->timeout = 70;
				break;
			}

			case ART_ARUNRUTH:
			{
				fire_bolt(Ind, GF_COLD, dir, damroll(12, 8));
				o_ptr->timeout = 500;
				break;
			}

			case ART_AEGLOS:
			{
				fire_ball(Ind, GF_COLD, dir, 100, 2);
				o_ptr->timeout = 500;
				break;
			}

			case ART_OROME:
			{
				wall_to_mud(Ind, dir);
				o_ptr->timeout = 5;
				break;
			}

			case ART_ULMO:
			{
				teleport_monster(Ind, dir);
				o_ptr->timeout = 150;
				break;
			}

			case ART_TOTILA:
			{
				confuse_monster(Ind, dir, 20);
				o_ptr->timeout = 15;
				break;
			}

			case ART_CAMMITHRIM:
			{
				fire_bolt(Ind, GF_MISSILE, dir, damroll(2, 6));
				o_ptr->timeout = 2;
				break;
			}

			case ART_PAURHACH:
			{
				fire_bolt(Ind, GF_FIRE, dir, damroll(9, 8));
				o_ptr->timeout = rand_int(8) + 8;
				break;
			}

			case ART_PAURNIMMEN:
			{
				fire_bolt(Ind, GF_COLD, dir, damroll(6, 8));
				o_ptr->timeout = rand_int(7) + 7;
				break;
			}

			case ART_PAURAEGEN:
			{
				fire_bolt(Ind, GF_ELEC, dir, damroll(4, 8));
				o_ptr->timeout = rand_int(6) + 6;
				break;
			}

			case ART_PAURNEN:
			{
				fire_bolt(Ind, GF_ACID, dir, damroll(5, 8));
				o_ptr->timeout = rand_int(5) + 5;
				break;
			}

			case ART_FINGOLFIN:
			{
				fire_bolt(Ind, GF_ARROW, dir, 150);
				o_ptr->timeout = rand_int(90) + 90;
				break;
			}

			case ART_NARYA:
			{
				fire_ball(Ind, GF_FIRE, dir, 120, 3);
				o_ptr->timeout = rand_int(225) + 225;
				break;
			}

			case ART_NENYA:
			{
				fire_ball(Ind, GF_COLD, dir, 200, 3);
				o_ptr->timeout = rand_int(325) + 325;
				break;
			}

			case ART_VILYA:
			{
				fire_ball(Ind, GF_ELEC, dir, 250, 3);
				o_ptr->timeout = rand_int(425) + 425;
				break;
			}

			case ART_POWER:
			{
				ring_of_power(Ind, dir);
				o_ptr->timeout = rand_int(450) + 450;
				break;
			}
		}

		/* Clear activation */
		p_ptr->current_activation = -1;

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Done */
		return;
	}

	/* Clear current activation */
	p_ptr->current_activation = -1;

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	/* Success */
	return;
}
