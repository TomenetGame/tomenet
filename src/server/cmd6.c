/* $Id$ */
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


/* How much drinking water feeds ents */
#define WATER_ENT_FOOD 1500


/* Quick hack to make use of firestones		- Jir -
 * This function should be obsoleted when ToME power.c is backported.
 */
/* Basically not cumulative */
static void do_tank(int Ind, int power)
{
	// player_type *p_ptr = Players[Ind];
	int i = randint(9);
	switch (i)
	{
		case 1:
			set_adrenaline(Ind, power + randint(power));
			break;
		case 2:
			set_biofeedback(Ind, power + randint(power));
			break;
		case 3:
		case 4:
			set_tim_esp(Ind, power + randint(power));
			break;
		case 5:
			set_prob_travel(Ind, power + randint(power));
			break;
		case 6:
			set_fury(Ind, power + randint(power));
			break;
		case 7:
			set_fast(Ind, power + randint(power), 10);
			break;
		case 8:
			set_shero(Ind, power + randint(power));
			break;
		case 9:
			set_oppose_acid(Ind, power + randint(power));
			set_oppose_elec(Ind, power + randint(power));
			set_oppose_fire(Ind, power + randint(power));
			set_oppose_cold(Ind, power + randint(power));
			set_oppose_pois(Ind, power + randint(power));
			break;
	}
}


bool eat_food(int Ind, int sval, object_type *o_ptr, bool *keep) {
	player_type *p_ptr = Players[Ind];
	bool ident = FALSE;

	/* Analyze the food */
	switch (sval) {
	case SV_FOOD_POISON:
		if (!(p_ptr->resist_pois || p_ptr->oppose_pois)) {
			if (set_poisoned(Ind, p_ptr->poisoned + rand_int(10) + 10, Ind))
				ident = TRUE;
		}
		break;

	case SV_FOOD_BLINDNESS:
		if (!p_ptr->resist_blind) {
			if (set_blind(Ind, p_ptr->blind + rand_int(200) + 200))
				ident = TRUE;
		}
		break;

	case SV_FOOD_PARANOIA:
		if (!p_ptr->resist_fear) {
			if (set_afraid(Ind, p_ptr->afraid + rand_int(10) + 10) ||
			    set_image(Ind, p_ptr->image + 20))
				ident = TRUE;
		}
		break;

	case SV_FOOD_CONFUSION:
		if (!p_ptr->resist_conf) {
			if (set_confused(Ind, p_ptr->confused + rand_int(10) + 10))
				ident = TRUE;
		}
		break;

	case SV_FOOD_HALLUCINATION:
		if (!p_ptr->resist_chaos) {
			take_sanity_hit(Ind, 2, "drugs");
			if (set_image(Ind, p_ptr->image + rand_int(250) + 250))
				ident = TRUE;
		}
		break;

	case SV_FOOD_PARALYSIS:
		if (!p_ptr->free_act) {
			if (set_paralyzed(Ind, p_ptr->paralyzed + rand_int(10) + 10))
				ident = TRUE;
		}
		break;

	case SV_FOOD_WEAKNESS:
		if (!p_ptr->suscep_life) take_hit(Ind, damroll(6, 6), "poisonous food", 0);
		(void)do_dec_stat(Ind, A_STR, STAT_DEC_NORMAL);
		ident = TRUE;
		break;

	case SV_FOOD_SICKNESS:
		if (!p_ptr->suscep_life) take_hit(Ind, damroll(6, 6), "poisonous food", 0);
		(void)do_dec_stat(Ind, A_CON, STAT_DEC_NORMAL);
		ident = TRUE;
		break;

	case SV_FOOD_STUPIDITY:
		if (!p_ptr->suscep_life) take_hit(Ind, damroll(8, 8), "poisonous food", 0);
		(void)do_dec_stat(Ind, A_INT, STAT_DEC_NORMAL);
		ident = TRUE;
		break;

	case SV_FOOD_NAIVETY:
		if (!p_ptr->suscep_life) take_hit(Ind, damroll(8, 8), "poisonous food", 0);
		(void)do_dec_stat(Ind, A_WIS, STAT_DEC_NORMAL);
		ident = TRUE;
		break;

	case SV_FOOD_UNHEALTH:
		if (!p_ptr->suscep_life) take_hit(Ind, damroll(10, 10), "poisonous food", 0);
		(void)do_dec_stat(Ind, A_CON, STAT_DEC_NORMAL);
		ident = TRUE;
		break;

	case SV_FOOD_DISEASE:
		if (!p_ptr->suscep_life) take_hit(Ind, damroll(10, 10), "poisonous food", 0);
		(void)do_dec_stat(Ind, A_STR, STAT_DEC_NORMAL);
		ident = TRUE;
		break;

	case SV_FOOD_CURE_POISON:
		if (set_poisoned(Ind, 0, 0)) ident = TRUE;
		break;

	case SV_FOOD_CURE_BLINDNESS:
		if (set_blind(Ind, 0)) ident = TRUE;
		break;

	case SV_FOOD_CURE_PARANOIA:
		if (set_afraid(Ind, 0)) ident = TRUE;
		//if (set_image(Ind, p_ptr->image / 2)) ident = TRUE;
		if (set_image(Ind, 0)) ident = TRUE;
		break;

	case SV_FOOD_CURE_CONFUSION:
		if (set_confused(Ind, 0)) ident = TRUE;
		break;

	case SV_FOOD_CURE_SERIOUS:
		if (set_blind(Ind, 0)) ident = TRUE;
		if (set_confused(Ind, 0)) ident = TRUE;
		if (set_cut(Ind, (p_ptr->cut / 2) - 50, p_ptr->cut_attacker)) ident = TRUE;
		//(void)set_poisoned(Ind, 0, 0);
		//(void)set_image(Ind, 0);	// ok?
		if (hp_player(Ind, damroll(6, 8))) ident = TRUE;
		break;

	case SV_FOOD_RESTORE_STR:
		if (do_res_stat(Ind, A_STR)) ident = TRUE;
		break;

	case SV_FOOD_RESTORE_CON:
		if (do_res_stat(Ind, A_CON)) ident = TRUE;
		break;

	case SV_FOOD_RESTORING:
		if (do_res_stat(Ind, A_STR)) ident = TRUE;
		if (do_res_stat(Ind, A_INT)) ident = TRUE;
		if (do_res_stat(Ind, A_WIS)) ident = TRUE;
		if (do_res_stat(Ind, A_DEX)) ident = TRUE;
		if (do_res_stat(Ind, A_CON)) ident = TRUE;
		if (do_res_stat(Ind, A_CHR)) ident = TRUE;
		if (restore_level(Ind)) ident = TRUE; /* <- new (for RPG_SERVER) */
		break;

	case SV_FOOD_FORTUNE_COOKIE:
		if (!p_ptr->suscep_life)
			msg_print(Ind, "That tastes good.");
		if (p_ptr->blind || no_lite(Ind)) {
			msg_print(Ind, "You feel some paper in it - what a pity you cannot see!");
		} else {
			msg_print(Ind, "There is message in the cookie. It says:");
			fortune(Ind, FALSE);
		}
		ident = TRUE;
		break;

	case SV_FOOD_ATHELAS:
		msg_print(Ind, "A fresh, clean essence rises, driving away wounds and poison.");
		ident = set_poisoned(Ind, 0, 0) |
			set_stun(Ind, 0) |
			set_cut(Ind, 0, 0);
		if (p_ptr->black_breath) {
			msg_print(Ind, "The hold of the Black Breath on you is broken!");
			p_ptr->black_breath = FALSE;
		}
		if (p_ptr->suscep_life) take_hit(Ind, 250, "a sprig of athelas", 0);
		ident = TRUE;
		break;

	case SV_FOOD_RATION:
		/* 'Rogue' tribute :) */
		if (magik(10)) {
			msg_print(Ind, "Yuk, that food tasted awful.");
			if (p_ptr->max_lev < 2 &&
			    !((p_ptr->mode & MODE_DED_IDDC) && !in_irondeepdive(&p_ptr->wpos)))
				gain_exp(Ind, 1);
			break;
		}
		/* Fall through */
	case SV_FOOD_BISCUIT:
	case SV_FOOD_JERKY:
	case SV_FOOD_SLIME_MOLD:
		if (!p_ptr->suscep_life || sval == SV_FOOD_SLIME_MOLD)
			msg_print(Ind, "That tastes good.");
		ident = TRUE;
		break;

	case SV_FOOD_WAYBREAD:
		if (!p_ptr->suscep_life && !p_ptr->suscep_good) {
			msg_print(Ind, "That tastes very good.");
			(void)set_poisoned(Ind, 0, 0);
			(void)set_image(Ind, 0);	// ok?
			(void)hp_player(Ind, damroll(5, 8));
			set_food(Ind, PY_FOOD_MAX - 1);
			ident = TRUE;
		} else {
			msg_print(Ind, "A surge of cleansing disrupts your body.");
			take_hit(Ind, damroll(5, 2), "lembas", 0);
		}
		break;

	case SV_FOOD_PINT_OF_ALE:
	case SV_FOOD_PINT_OF_WINE:
	    if (!p_ptr->suscep_life) {
		if (!o_ptr) {
			msg_format(Ind, "\377%c*HIC*", random_colour());
			msg_format_near(Ind, "\377%c%s hiccups!", random_colour(), p_ptr->name);

			if (magik(TRUE? 60 : 30))
				set_confused(Ind, p_ptr->confused + 20 + randint(20));
			if (magik(TRUE? 50 : 20))
				set_stun_raw(Ind, p_ptr->stun + 10 + randint(10));

			if (magik(TRUE? 50 : 10)){
				set_image(Ind, p_ptr->image + 10 + randint(10));
				take_sanity_hit(Ind, 1, "ale");
			}
			if (magik(TRUE? 10 : 20))
				set_paralyzed(Ind, p_ptr->paralyzed + 10 + randint(10));
			if (magik(TRUE? 50 : 10))
				set_hero(Ind, 10 + randint(10)); /* removed stacking */
			if (magik(TRUE? 20 : 5))
				set_shero(Ind, 5 + randint(10)); /* removed stacking */
			if (magik(TRUE? 5 : 10))
				set_afraid(Ind, p_ptr->afraid + 15 + randint(10));
			if (magik(TRUE? 5 : 10))
				set_slow(Ind, p_ptr->slow + 10 + randint(10));
			else if (magik(TRUE? 20 : 5))
				set_fast(Ind, 10 + randint(10), 10); /* removed stacking */
			/* Methyl! */
			if (magik(TRUE? 0 : 3))
				set_blind(Ind, p_ptr->blind + 10 + randint(10));
			if (rand_int(100) < p_ptr->food * magik(TRUE? 40 : 60) / PY_FOOD_MAX)
			{
				msg_print(Ind, "You become nauseous and vomit!");
				msg_format_near(Ind, "%s vomits!", p_ptr->name);
				/* made salt water less deadly -APD */
				(void)set_food(Ind, (p_ptr->food/2));
				(void)set_poisoned(Ind, 0, 0);
				(void)set_paralyzed(Ind, p_ptr->paralyzed + 4);
			}
			if (magik(TRUE? 2 : 3))
				(void)dec_stat(Ind, A_DEX, 1, STAT_DEC_TEMPORARY);
			if (magik(TRUE? 2 : 3))
				(void)dec_stat(Ind, A_WIS, 1, STAT_DEC_TEMPORARY);
			if (magik(TRUE? 0 : 1))
				(void)dec_stat(Ind, A_CON, 1, STAT_DEC_TEMPORARY);
			//			(void)dec_stat(Ind, A_STR, 1, STAT_DEC_TEMPORARY);
			if (magik(TRUE? 3 : 5))
				(void)dec_stat(Ind, A_CHR, 1, STAT_DEC_TEMPORARY);
			if (magik(TRUE? 2 : 3))
				(void)dec_stat(Ind, A_INT, 1, STAT_DEC_TEMPORARY);
		}
		/* Let's make this usable... - the_sandman */
		else if (o_ptr->name1 == ART_DWARVEN_ALE) {
			msg_print(Ind, "\377gYou drank the liquior of the gods");
			msg_format_near(Ind, "\377gYou look enviously as %s took a sip of The Ale", p_ptr->name);
			switch (randint(10)) {
			case 1:
			case 2:
			case 3:	// 3 in 10 for Hero effect
				set_hero(Ind, 20 + randint(10)); break;
			case 4:
			case 5:
			case 6:	// 3 in 10 for Speed
				set_fast(Ind, 20 + randint(10), 10); break;
			case 7:
			case 8:
			case 9: // 3 in 10 for Berserk
				set_shero(Ind, 20 + randint(10)); break;
			case 10:// 1 in 10 for confusion ;)
			default:
				if (!(p_ptr->resist_conf)) {
					set_confused(Ind, randint(10));
				} break;
			}
			p_ptr->food = PY_FOOD_FULL;	// A quaff will bring you to the norm sustenance level!
		} else if (magik(o_ptr->name2? 50 : 20)) {
			msg_format(Ind, "\377%c*HIC*", random_colour());
			msg_format_near(Ind, "\377%c%s hiccups!", random_colour(), p_ptr->name);

			if (magik(o_ptr->name2? 60 : 30))
				set_confused(Ind, p_ptr->confused + 20 + randint(20));
			if (magik(o_ptr->name2? 50 : 20))
				set_stun_raw(Ind, p_ptr->stun + 10 + randint(10));

			if (magik(o_ptr->name2? 50 : 10)){
				set_image(Ind, p_ptr->image + 10 + randint(10));
				take_sanity_hit(Ind, 1, "ale");
			}
			if (magik(o_ptr->name2? 10 : 20))
				set_paralyzed(Ind, p_ptr->paralyzed + 10 + randint(10));
			if (magik(o_ptr->name2? 50 : 10))
				set_hero(Ind, 10 + randint(10)); /* removed stacking */
			if (magik(o_ptr->name2? 20 : 5))
				set_shero(Ind, 5 + randint(10)); /* removed stacking */
			if (magik(o_ptr->name2? 5 : 10))
				set_afraid(Ind, p_ptr->afraid + 15 + randint(10));
			if (magik(o_ptr->name2? 5 : 10))
				set_slow(Ind, p_ptr->slow + 10 + randint(10));
			else if (magik(o_ptr->name2? 20 : 5))
				set_fast(Ind, 10 + randint(10), 10); /* removed stacking */
			/* Methyl! */
			if (magik(o_ptr->name2? 0 : 3))
				set_blind(Ind, p_ptr->blind + 10 + randint(10));
			if (rand_int(100) < p_ptr->food * magik(o_ptr->name2? 40 : 60) / PY_FOOD_MAX)
			{
				msg_print(Ind, "You become nauseous and vomit!");
				msg_format_near(Ind, "%s vomits!", p_ptr->name);
				/* made salt water less deadly -APD */
				(void)set_food(Ind, (p_ptr->food/2));
				(void)set_poisoned(Ind, 0, 0);
				(void)set_paralyzed(Ind, p_ptr->paralyzed + 4);
			}
			if (magik(o_ptr->name2? 2 : 3))
				(void)dec_stat(Ind, A_DEX, 1, STAT_DEC_TEMPORARY);
			if (magik(o_ptr->name2? 2 : 3))
				(void)dec_stat(Ind, A_WIS, 1, STAT_DEC_TEMPORARY);
			if (magik(o_ptr->name2? 0 : 1))
				(void)dec_stat(Ind, A_CON, 1, STAT_DEC_TEMPORARY);
			//			(void)dec_stat(Ind, A_STR, 1, STAT_DEC_TEMPORARY);
			if (magik(o_ptr->name2? 3 : 5))
				(void)dec_stat(Ind, A_CHR, 1, STAT_DEC_TEMPORARY);
			if (magik(o_ptr->name2? 2 : 3))
				(void)dec_stat(Ind, A_INT, 1, STAT_DEC_TEMPORARY);
		}
		else msg_print(Ind, "That tastes good.");
	    } else {
		msg_print(Ind, "That tastes fair but has no effect.");
	    }
		if (o_ptr && o_ptr->name1 == ART_DWARVEN_ALE) *keep = TRUE;

		ident = TRUE;
		break;

	case SV_FOOD_UNMAGIC:
		ident = unmagic(Ind);
		break;
	}

	return ident;
}

/*
 * Eat some food (from the pack or floor)
 */
void do_cmd_eat_food(int Ind, int item) {
	player_type *p_ptr = Players[Ind];

	int			ident, lev;

	object_type		*o_ptr;
	bool keep = FALSE, flipped = FALSE;


	/* Restrict choices to food */
	item_tester_tval = TV_FOOD;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &p_ptr->inventory[item];
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max) return; /* item doesn't exist */
		o_ptr = &o_list[0 - item];
	}

	if (check_guard_inscription( o_ptr->note, 'E' )) {
		msg_print(Ind, "The item's inscription prevents it.");
		return;
	}

	if (!can_use_verbose(Ind, o_ptr)) return;


	if (o_ptr->tval != TV_FOOD && o_ptr->tval != TV_FIRESTONE) {
//(may happen on death, from macro spam)		msg_print(Ind, "SERVER ERROR: Tried to eat non-food!");
		return;
	}

	if (p_ptr->prace == RACE_ENT && !p_ptr->body_monster) {
		/* Let them drink :) */
		if (o_ptr->tval != TV_FOOD ||
		    (o_ptr->sval != SV_FOOD_PINT_OF_ALE &&
		    o_ptr->sval != SV_FOOD_PINT_OF_WINE)) {
			msg_print(Ind, "You cannot eat food.");
			return;
		}
		if (o_ptr->tval == TV_FIRESTONE) {
			msg_print(Ind, "You cannot eat firestones.");
			return;
		}
	}

	/* Let the player stay afk while eating,
	   since we assume he's resting ;) - C. Blue */
/*	un_afk_idle(Ind); */

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

#ifdef USE_SOUND_2010
	if (o_ptr->sval == SV_FOOD_PINT_OF_ALE || o_ptr->sval == SV_FOOD_PINT_OF_WINE)
		sound(Ind, "quaff_potion", NULL, SFX_TYPE_COMMAND, FALSE);
	else
		sound(Ind, "eat", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

	/* Identity not known yet */
	ident = FALSE;

	/* Object level */
	lev = k_info[o_ptr->k_idx].level;

	/* (not quite) Normal foods */
	if (o_ptr->tval == TV_FOOD)
		ident = eat_food(Ind, o_ptr->sval, o_ptr, &keep);
	/* Firestones */
	else if (o_ptr->tval == TV_FIRESTONE) {
		bool dragon = FALSE;
		if (p_ptr->body_monster) {
			monster_race *r_ptr = &r_info[p_ptr->body_monster];
			if (strchr("dD", r_ptr->d_char)) dragon = TRUE;
		}
		if (p_ptr->prace == RACE_DRACONIAN) {
			switch (o_ptr->sval) {
			case SV_FIRE_SMALL:
				msg_print(Ind, "Grrrmfff ...");
				do_tank(Ind, 10);
				ident = TRUE;
				break;

			case SV_FIRESTONE:
				msg_print(Ind, "Grrrrmmmmmmfffffff ...");
				do_tank(Ind, 25);
				do_tank(Ind, 25);	// twice
				ident = TRUE;
				break;
			}
		}
		else if (!dragon) msg_print(Ind, "Yikes, you cannot eat this, you vomit!");
		else {
			msg_print(Ind, "That tastes weird..");
			switch (o_ptr->sval) {
			case SV_FIRE_SMALL:
				if (p_ptr->csp < p_ptr->msp
#ifdef MARTYR_NO_MANA
				    && !p_ptr->martyr
#endif
				    ) {
					p_ptr->csp += 30;
					if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;
					msg_print(Ind, "You feel your head clearing.");
					p_ptr->redraw |= (PR_MANA);
					p_ptr->window |= (PW_PLAYER);
					ident = TRUE;
				}
				break;

			case SV_FIRESTONE:
				if (p_ptr->csp < p_ptr->msp
#ifdef MARTYR_NO_MANA
				    && !p_ptr->martyr
#endif
				    ) {
					p_ptr->csp += 100;
					if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;
					msg_print(Ind, "You feel your head clearing.");
					p_ptr->redraw |= (PR_MANA);
					p_ptr->window |= (PW_PLAYER);
					ident = TRUE;
				}
				break;
			}
		}
	}


	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* The player is now aware of the object */
	if (ident && !object_aware_p(Ind, o_ptr)) {
		flipped = object_aware(Ind, o_ptr);
		if (!(p_ptr->mode & MODE_PVP)) gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* We have tried it */
	object_tried(Ind, o_ptr, flipped);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Food can feed the player */
	if (!p_ptr->suscep_life)
		(void)set_food(Ind, p_ptr->food + o_ptr->pval);
	else if (p_ptr->prace != RACE_VAMPIRE)
		(void)set_food(Ind, p_ptr->food + o_ptr->pval / 3);


	/* Hack -- really allow certain foods to be "preserved" */
	if (keep) return;


	/* Destroy a food in the pack */
	if (item >= 0) {
		inven_item_increase(Ind, item, -1);
		inven_item_describe(Ind, item);
		inven_item_optimize(Ind, item);
	}
	/* Destroy a food on the floor */
	else {
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}
}


/* Hack: pval < 0 means that we quaffed from a fountain, pval -257 means that it's a quest's status effect.
   Apart from feeding us, the only effect of these hacks are the messages we receive. */
bool quaff_potion(int Ind, int tval, int sval, int pval) {
	player_type *p_ptr = Players[Ind];
	int i, ident = FALSE, msg;

	if (pval == -257) {
		pval = 0;
		msg = 2; //quest
	} else if (pval < 0) {
		pval = -1 - pval;
		msg = 1; //fountain
	} else msg = 0; //potion (default)

	bypass_invuln = TRUE;

	/* Analyze the potion */
	if (tval == TV_POTION) {

		switch (sval) {
		case SV_POTION_WATER:
		case SV_POTION_APPLE_JUICE:
		case SV_POTION_SLIME_MOLD:
			if (!p_ptr->suscep_life) msg_print(Ind, "You feel less thirsty.");
			ident = TRUE; //easy ident? by taste or something?
			break;
		case SV_POTION_SLOWNESS:
			if (set_slow(Ind, p_ptr->slow + randint(25) + 15)) ident = TRUE;
			break;
		case SV_POTION_SALT_WATER:
			if (!p_ptr->suscep_life && p_ptr->prace != RACE_ENT) {
				if (!msg) msg_print(Ind, "The salty potion makes you vomit!");
				else if (msg == 1) msg_print(Ind, "The water is so salty that it makes you vomit!");
				else msg_print(Ind, "You feel sick and have to vomit!");
				msg_format_near(Ind, "%s vomits!", p_ptr->name);
				/* made salt water less deadly -APD */
				(void)set_food(Ind, (p_ptr->food / 2) - 400);
				(void)set_poisoned(Ind, 0, 0);
				(void)set_paralyzed(Ind, p_ptr->paralyzed + 4);
			} else {
				if (!msg) msg_print(Ind, "That potion tastes awfully salty.");
				else if (msg == 1) msg_print(Ind, "The water tastes awfully salty.");
			}
			ident = TRUE;
			break;
		case SV_POTION_POISON:
			if (!(p_ptr->resist_pois || p_ptr->oppose_pois))
				if (set_poisoned(Ind, p_ptr->poisoned + rand_int(15) + 10, 0)) ident = TRUE;
			break;
		case SV_POTION_BLINDNESS:
			if (!p_ptr->resist_blind)
				if (set_blind(Ind, p_ptr->blind + rand_int(100) + 100)) ident = TRUE;
			break;
		case SV_POTION_CONFUSION:
			if (!p_ptr->resist_conf)
				if (set_confused(Ind, p_ptr->confused + rand_int(20) + 15)) ident = TRUE;
			break;
#if 0
		case SV_POTION_MUTATION:
			ident = TRUE;
			break;
#endif
		case SV_POTION_SLEEP:
			if (!p_ptr->free_act)
				if (set_paralyzed(Ind, p_ptr->paralyzed + rand_int(4) + 4)) ident = TRUE;
			break;
		case SV_POTION_LOSE_MEMORIES:
			if (!p_ptr->hold_life && (p_ptr->exp > 0)) {
				if (p_ptr->keep_life) msg_print(Ind, "You are unaffected!");
				else {
					msg_print(Ind, "\377GYou feel your memories fade.");
					lose_exp(Ind, p_ptr->exp / 4);
					ident = TRUE;
				}
			}
			break;
		case SV_POTION_RUINATION:
			msg_print(Ind, "Your nerves and muscles feel weak and lifeless!");
			if (!msg) take_hit(Ind, damroll(10, 10), "a potion of ruination", 0);
			else if (msg == 1) take_hit(Ind, damroll(10, 10), "a fountain of ruination", 0);
			else take_hit(Ind, damroll(10, 10), "ruination", 0);
			(void)dec_stat(Ind, A_DEX, 25, STAT_DEC_NORMAL);
			(void)dec_stat(Ind, A_WIS, 25, STAT_DEC_NORMAL);
			(void)dec_stat(Ind, A_CON, 25, STAT_DEC_NORMAL);
			(void)dec_stat(Ind, A_STR, 25, STAT_DEC_NORMAL);
			(void)dec_stat(Ind, A_CHR, 25, STAT_DEC_NORMAL);
			(void)dec_stat(Ind, A_INT, 25, STAT_DEC_NORMAL);
			ident = TRUE;
			break;
		case SV_POTION_DEC_STR:
			if (do_dec_stat(Ind, A_STR, STAT_DEC_NORMAL)) ident = TRUE;
			break;
		case SV_POTION_DEC_INT:
			if (do_dec_stat(Ind, A_INT, STAT_DEC_NORMAL)) ident = TRUE;
			break;
		case SV_POTION_DEC_WIS:
			if (do_dec_stat(Ind, A_WIS, STAT_DEC_NORMAL)) ident = TRUE;
			break;
		case SV_POTION_DEC_DEX:
			if (do_dec_stat(Ind, A_DEX, STAT_DEC_NORMAL)) ident = TRUE;
			break;
		case SV_POTION_DEC_CON:
			if (do_dec_stat(Ind, A_CON, STAT_DEC_NORMAL)) ident = TRUE;
			break;
		case SV_POTION_DEC_CHR:
			if (do_dec_stat(Ind, A_CHR, STAT_DEC_NORMAL)) ident = TRUE;
			break;
		case SV_POTION_DETONATIONS:
#ifdef USE_SOUND_2010
			sound(Ind, "detonation", NULL, SFX_TYPE_MISC, TRUE);
#endif
			msg_print(Ind, "Massive explosions rupture your body!");
			msg_format_near(Ind, "%s blows up!", p_ptr->name);
			if (!msg) take_hit(Ind, damroll(50, 20), "a potion of detonation", 0);
			else if (msg == 1) take_hit(Ind, damroll(50, 20), "a fountain of detonation", 0); //disabled
			else take_hit(Ind, damroll(50, 20), "detonations", 0);
			(void)set_stun_raw(Ind, p_ptr->stun + 75);
			(void)set_cut(Ind, p_ptr->cut + 5000, Ind);
			ident = TRUE;
			break;
		case SV_POTION_DEATH:
			if (!p_ptr->suscep_life) {
				msg_print(Ind, "A feeling of death flows through your body.");
				if (!msg) take_hit(Ind, 5000, "a potion of death", 0);
				else if (msg == 1) take_hit(Ind, 5000, "a fountain of death", 0); //disabled
				else take_hit(Ind, 5000, "death", 0);
				ident = TRUE;
			} else {
				if (msg != 2) {
					msg_print(Ind, "You burp.");
					msg_format_near(Ind, "%s burps.", p_ptr->name);
				}
				if (p_ptr->cst < p_ptr->mst && !p_ptr->shadow_running) {
					msg_print(Ind, "You feel refreshed.");
					p_ptr->cst = p_ptr->mst;
					p_ptr->cst_frac = 0;
					p_ptr->redraw |= PR_STAMINA;
				}
			}
			break;
		case SV_POTION_INFRAVISION:
			if (set_tim_infra(Ind, 100 + randint(100))) ident = TRUE; /* removed stacking */
			break;
		case SV_POTION_DETECT_INVIS:
			if (set_tim_invis(Ind, 12 + randint(12))) ident = TRUE; /* removed stacking */
			break;
		case SV_POTION_INVIS:
			set_invis(Ind, 15 + randint(10), p_ptr->lev < 30 ? 24 : p_ptr->lev * 4 / 5);
			ident = TRUE;
		case SV_POTION_SLOW_POISON:
#if 0
			if (set_poisoned(Ind, p_ptr->poisoned / 2, p_ptr->poisoned_attacker)) ident = TRUE;
#else /* back to traditional way */
			if (p_ptr->poisoned && !p_ptr->slow_poison) {
				p_ptr->slow_poison = 1;
				ident = TRUE;
			}
#endif
			break;
		case SV_POTION_CURE_POISON:
			if (set_poisoned(Ind, 0, 0)) ident = TRUE;
			break;
		case SV_POTION_BOLDNESS:
			/* Stay bold for some turns */
			if (set_afraid(Ind, 0)) ident = TRUE;
			set_res_fear(Ind, 5);
			break;
		case SV_POTION_SPEED:
			if (set_fast(Ind, randint(25) + 15, 10)) ident = TRUE; /* removed stacking */
			break;
		case SV_POTION_RESIST_HEAT:
			if (set_oppose_fire(Ind, randint(10) + 10)) ident = TRUE; /* removed stacking */
			break;
		case SV_POTION_RESIST_COLD:
			if (set_oppose_cold(Ind, randint(10) + 10)) ident = TRUE; /* removed stacking */
			break;
		case SV_POTION_HEROISM:
			if (hp_player(Ind, 10)) ident = TRUE;
			if (set_afraid(Ind, 0)) ident = TRUE;
			if (set_hero(Ind, randint(25) + 25)) ident = TRUE; /* removed stacking */
			break;
		case SV_POTION_BERSERK_STRENGTH:
			if (hp_player(Ind, 30)) ident = TRUE;
			if (set_afraid(Ind, 0)) ident = TRUE;
			if (set_shero(Ind, randint(25) + 25)) ident = TRUE; /* removed stacking */
			break;
		case SV_POTION_CURE_LIGHT:
			if (hp_player(Ind, damroll(3, 8))) ident = TRUE;
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, p_ptr->cut - 10, p_ptr->cut_attacker)) ident = TRUE;
			break;
		case SV_POTION_CURE_SERIOUS:
			if (hp_player(Ind, damroll(6, 8))) ident = TRUE;
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_confused(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, (p_ptr->cut / 2) - 50, p_ptr->cut_attacker)) ident = TRUE;
			break;
		case SV_POTION_CURE_CRITICAL:
			if (hp_player(Ind, damroll(14, 8))) ident = TRUE;
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_confused(Ind, 0)) ident = TRUE;
			//if (set_poisoned(Ind, 0, 0)) ident = TRUE;	/* use specialized pots */
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0, 0)) ident = TRUE;
			break;
		case SV_POTION_HEALING:
			if (hp_player(Ind, 300)) ident = TRUE;
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_confused(Ind, 0)) ident = TRUE;
			if (set_poisoned(Ind, 0, 0)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0, 0)) ident = TRUE;
			break;
		case SV_POTION_STAR_HEALING:
			if (hp_player(Ind, 700)) ident = TRUE;
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_confused(Ind, 0)) ident = TRUE;
			if (set_poisoned(Ind, 0, 0)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0, 0)) ident = TRUE;
			break;
		case SV_POTION_LIFE:
			msg_print(Ind, "\377GYou feel life flow through your body!");
			restore_level(Ind);
			if (p_ptr->suscep_life) {
				if (!msg) take_hit(Ind, 500, "a potion of life", 0);
				else if (msg == 1) take_hit(Ind, 500, "a fountain of life", 0); //disabled
				else take_hit(Ind, 500, "life", 0);
			}
			else hp_player(Ind, 700);
			(void)set_poisoned(Ind, 0, 0);
			(void)set_blind(Ind, 0);
			(void)set_confused(Ind, 0);
			(void)set_image(Ind, 0);
			(void)set_stun(Ind, 0);
			(void)set_cut(Ind, 0, 0);
			(void)do_res_stat(Ind, A_STR);
			(void)do_res_stat(Ind, A_CON);
			(void)do_res_stat(Ind, A_DEX);
			(void)do_res_stat(Ind, A_WIS);
			(void)do_res_stat(Ind, A_INT);
			(void)do_res_stat(Ind, A_CHR);
			(void)restore_level(Ind);
			if (p_ptr->black_breath) {
				p_ptr->black_breath = FALSE;
				msg_print(Ind, "The hold of the Black Breath on you is broken!");
			}
			if (!p_ptr->suscep_life &&
			    p_ptr->cst < p_ptr->mst && !p_ptr->shadow_running) {
				msg_print(Ind, "You feel refreshed.");
				p_ptr->cst = p_ptr->mst;
				p_ptr->cst_frac = 0;
				p_ptr->redraw |= PR_STAMINA;
			}
			ident = TRUE;
			break;
		case SV_POTION_RESTORE_MANA:
			if (p_ptr->csp < p_ptr->msp
#ifdef MARTYR_NO_MANA
			    && !p_ptr->martyr
#endif
			    ) {
				p_ptr->csp += 500;
				if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;
				msg_print(Ind, "You feel your head clearing.");
				p_ptr->redraw |= (PR_MANA);
				p_ptr->window |= (PW_PLAYER);
				ident = TRUE;
			}
			break;
		case SV_POTION_STAR_RESTORE_MANA:
			if (p_ptr->csp < p_ptr->msp
#ifdef MARTYR_NO_MANA
			    && !p_ptr->martyr
#endif
			    ) {
				p_ptr->csp += 1000;
				if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;
				msg_print(Ind, "You feel your head clearing!");
				p_ptr->redraw |= (PR_MANA);
				p_ptr->window |= (PW_PLAYER);
				ident = TRUE;
			}
			break;
		case SV_POTION_RESTORE_EXP:
			if (restore_level(Ind)) ident = TRUE;
			break;
		case SV_POTION_RES_STR:
			if (do_res_stat(Ind, A_STR)) ident = TRUE;
			break;
		case SV_POTION_RES_INT:
			if (do_res_stat(Ind, A_INT)) ident = TRUE;
			break;
		case SV_POTION_RES_WIS:
			if (do_res_stat(Ind, A_WIS)) ident = TRUE;
			break;
		case SV_POTION_RES_DEX:
			if (do_res_stat(Ind, A_DEX)) ident = TRUE;
			break;
		case SV_POTION_RES_CON:
			if (do_res_stat(Ind, A_CON)) ident = TRUE;
			break;
		case SV_POTION_RES_CHR:
			if (do_res_stat(Ind, A_CHR)) ident = TRUE;
			break;
		case SV_POTION_INC_STR:
			if (do_inc_stat(Ind, A_STR)) ident = TRUE;
			break;
		case SV_POTION_INC_INT:
			if (do_inc_stat(Ind, A_INT)) ident = TRUE;
			break;
		case SV_POTION_INC_WIS:
			if (do_inc_stat(Ind, A_WIS)) ident = TRUE;
			break;
		case SV_POTION_INC_DEX:
			if (do_inc_stat(Ind, A_DEX)) ident = TRUE;
			break;
		case SV_POTION_INC_CON:
			if (do_inc_stat(Ind, A_CON)) ident = TRUE;
			break;
		case SV_POTION_INC_CHR:
			if (do_inc_stat(Ind, A_CHR)) ident = TRUE;
			break;
		case SV_POTION_AUGMENTATION:
			if (do_inc_stat(Ind, A_STR)) ident = TRUE;
			if (do_inc_stat(Ind, A_INT)) ident = TRUE;
			if (do_inc_stat(Ind, A_WIS)) ident = TRUE;
			if (do_inc_stat(Ind, A_DEX)) ident = TRUE;
			if (do_inc_stat(Ind, A_CON)) ident = TRUE;
			if (do_inc_stat(Ind, A_CHR)) ident = TRUE;
			break;
		case SV_POTION_ENLIGHTENMENT:
			identify_pack(Ind);
			msg_print(Ind, "An image of your surroundings forms in your mind...");
			wiz_lite(Ind);
			ident = TRUE;
			break;
		case SV_POTION_STAR_ENLIGHTENMENT:
#if 0 /* would need to increase price from 25k back to 80k ;) */
			for (i = 0; i < INVEN_TOTAL; i++)
				identify_fully_item_quiet(Ind, i);
#else
			identify_pack(Ind);
#endif
			msg_print(Ind, "You begin to feel more enlightened...");
			msg_print(Ind, NULL);
			wiz_lite(Ind);
			(void)do_inc_stat(Ind, A_INT);
			(void)do_inc_stat(Ind, A_WIS);
			(void)detect_treasure(Ind, DEFAULT_RADIUS * 2);
			(void)detect_object(Ind, DEFAULT_RADIUS * 2);
			(void)detect_sdoor(Ind, DEFAULT_RADIUS * 2);
			(void)detect_trap(Ind, DEFAULT_RADIUS * 2);
			identify_pack(Ind);
			self_knowledge(Ind);
			ident = TRUE;
			break;
		case SV_POTION_SELF_KNOWLEDGE:
			identify_pack(Ind);
			msg_print(Ind, "You begin to know yourself a little better...");
			msg_print(Ind, NULL);
			self_knowledge(Ind);
			ident = TRUE;
			break;
		case SV_POTION_EXPERIENCE:
			if (p_ptr->exp < PY_MAX_EXP) {
				s32b ee = (p_ptr->exp / 2) + 10;
				if (ee > 100000L) ee = 100000L;
#ifdef ALT_EXPRATIO
				ee = (ee * (s64b)p_ptr->expfact) / 100L; /* give same amount to anyone */
#endif
				if (!(p_ptr->mode & MODE_PVP)) {
					msg_print(Ind, "\377GYou feel more experienced.");
					gain_exp(Ind, ee);
				}
				ident = TRUE;
			}
			break;
			/* additions from PernA */
		case SV_POTION_CURING:
			if (hp_player(Ind, 50)) ident = TRUE;
			if (set_blind(Ind, 0)) ident = TRUE;
			if (set_poisoned(Ind, 0, 0)) ident = TRUE;
			if (set_confused(Ind, 0)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0, 0)) ident = TRUE;
			if (set_image(Ind, 0)) ident = TRUE;
			if (heal_insanity(Ind, 40)) ident = TRUE;
			if (p_ptr->food >= PY_FOOD_MAX) /* ungorge */
				if (set_food(Ind, PY_FOOD_MAX - 1 - pval)) ident = TRUE;
			if (do_res_stat(Ind, A_STR)) ident = TRUE;
			if (do_res_stat(Ind, A_CON)) ident = TRUE;
			if (do_res_stat(Ind, A_DEX)) ident = TRUE;
			if (do_res_stat(Ind, A_WIS)) ident = TRUE;
			if (do_res_stat(Ind, A_INT)) ident = TRUE;
			if (do_res_stat(Ind, A_CHR)) ident = TRUE;
			break;
		case SV_POTION_INVULNERABILITY:
			ident = set_invuln(Ind, randint(7) + 7); /* removed stacking */
			break;
		case SV_POTION_RESISTANCE:
			ident =
				set_oppose_acid(Ind, randint(20) + 20) |
				set_oppose_elec(Ind, randint(20) + 20) |
				set_oppose_fire(Ind, randint(20) + 20) |
				set_oppose_cold(Ind, randint(20) + 20) |
				set_oppose_pois(Ind, randint(20) + 20); /* removed stacking */
			break;
#ifdef EXPAND_TV_POTION
		//max sanity of a player can go up to around 900 later!
		case SV_POTION_CURE_LIGHT_SANITY:
			if (heal_insanity(Ind, damroll(4,8))) ident = TRUE;
			(void)set_image(Ind, 0);
			break;
		case SV_POTION_CURE_SERIOUS_SANITY:
			//if (heal_insanity(Ind, damroll(6,13))) ident = TRUE;
			if (heal_insanity(Ind, damroll(8,12))) ident = TRUE;
			(void)set_image(Ind, 0);
			break;
		case SV_POTION_CURE_CRITICAL_SANITY:
			//if (heal_insanity(Ind, damroll(9,20))) ident = TRUE;
			if (heal_insanity(Ind, damroll(16,18))) ident = TRUE;
			(void)set_image(Ind, 0);
			break;
		case SV_POTION_CURE_SANITY:
			//if (heal_insanity(Ind, damroll(14,32))) ident = TRUE;
			if (heal_insanity(Ind, damroll(32,27))) ident = TRUE;
			(void)set_image(Ind, 0);
			break;
		case SV_POTION_CHAUVE_SOURIS:
			//apply_morph(Ind, 100, "Potion of Chauve-Souris");
			if (!p_ptr->fruit_bat) {
				/* FRUIT BAT!!!!!! */
				if (p_ptr->body_monster) do_mimic_change(Ind, 0, TRUE);
				msg_print(Ind, "You have been turned into a fruit bat!");
				if (!msg) {
					strcpy(p_ptr->died_from,"a potion of Chauve-Souris");
					strcpy(p_ptr->really_died_from,"a potion of Chauve-Souris");
				} else if (msg == 1) {
					strcpy(p_ptr->died_from,"a fountain of Chauve-Souris");
					strcpy(p_ptr->really_died_from,"a fountain of Chauve-Souris");
				} else {
					strcpy(p_ptr->died_from,"Chauve-Souris");
					strcpy(p_ptr->really_died_from,"Chauve-Souris");
				}
				p_ptr->fruit_bat = -1;
				p_ptr->deathblow = 0;
				player_death(Ind);
				ident = TRUE;
			} else if(p_ptr->fruit_bat == 2) {
				msg_print(Ind, "You have been restored!");
				p_ptr->fruit_bat = 0;
				p_ptr->update |= (PU_BONUS | PU_HP);
				ident = TRUE;
			}
			//else msg_print(Ind, "You feel certain you are a fruit bat!"); <-wouldn't this msg require 'ident' too? disabled for now
			break;
		case SV_POTION_LEARNING:
			ident = TRUE;
			/* gain skill points */
			i = 1 + rand_int(3);
			p_ptr->skill_points += i;
			p_ptr->update |= PU_SKILL_MOD;
			if (is_older_than(&p_ptr->version, 4, 4, 8, 5, 0, 0)) p_ptr->redraw |= PR_STUDY;
			msg_format(Ind, "You gained %d more skill point%s.", i, (i == 1) ? "" : "s");
			s_printf("LEARNING: %s gained %d more skill point%s.\n", p_ptr->name, i, (i == 1) ? "" : "s");
			break;
#endif
		}
	} else { /* POTION2 */
		switch (sval) {
		//max sanity of a player can go up to around 900 later!
		case SV_POTION2_CURE_LIGHT_SANITY:
			if (heal_insanity(Ind, damroll(4,8))) ident = TRUE;
			(void)set_image(Ind, 0);
			break;
		case SV_POTION2_CURE_SERIOUS_SANITY:
			//if (heal_insanity(Ind, damroll(6,13))) ident = TRUE;
			if (heal_insanity(Ind, damroll(8,12))) ident = TRUE;
			(void)set_image(Ind, 0);
			break;
		case SV_POTION2_CURE_CRITICAL_SANITY:
			//if (heal_insanity(Ind, damroll(9,20))) ident = TRUE;
			if (heal_insanity(Ind, damroll(16,18))) ident = TRUE;
			(void)set_image(Ind, 0);
			break;
		case SV_POTION2_CURE_SANITY:
			//if (heal_insanity(Ind, damroll(14,32))) ident = TRUE;
			if (heal_insanity(Ind, damroll(32,27))) ident = TRUE;
			(void)set_image(Ind, 0);
			break;
		case SV_POTION2_CHAUVE_SOURIS:
			//apply_morph(Ind, 100, "Potion of Chauve-Souris");
			if (!p_ptr->fruit_bat) {
				/* FRUIT BAT!!!!!! */
				if (p_ptr->body_monster) do_mimic_change(Ind, 0, TRUE);
				msg_print(Ind, "You have been turned into a fruit bat!");
				if (!msg) {
					strcpy(p_ptr->died_from,"a potion of Chauve-Souris");
					strcpy(p_ptr->really_died_from,"a potion of Chauve-Souris");
				} else if (msg == 1) {
					strcpy(p_ptr->died_from,"a fountain of Chauve-Souris");
					strcpy(p_ptr->really_died_from,"a fountain of Chauve-Souris");
				} else {
					strcpy(p_ptr->died_from,"Chauve-Souris");
					strcpy(p_ptr->really_died_from,"Chauve-Souris");
				}
				p_ptr->fruit_bat = -1;
				p_ptr->deathblow = 0;
				player_death(Ind);
				ident = TRUE;
			} else if(p_ptr->fruit_bat == 2) {
				msg_print(Ind, "You have been restored!");
				p_ptr->fruit_bat = 0;
				p_ptr->update |= (PU_BONUS | PU_HP);
				ident = TRUE;
			}
			//else msg_print(Ind, "You feel certain you are a fruit bat!"); <-wouldn't this msg require 'ident' too? disabled for now
			break;
		case SV_POTION2_LEARNING:
			ident = TRUE;
			/* gain skill points */
			i = 1 + rand_int(3);
			p_ptr->skill_points += i;
			p_ptr->update |= PU_SKILL_MOD;
			if (is_older_than(&p_ptr->version, 4, 4, 8, 5, 0, 0)) p_ptr->redraw |= PR_STUDY;
			msg_format(Ind, "You gained %d more skill point%s.", i, (i == 1)?"":"s");
			s_printf("LEARNING: %s gained %d more skill point%s.\n", p_ptr->name, i, (i == 1) ? "" : "s");
			break;
		case SV_POTION2_AMBER:
			ident = TRUE;
			msg_print(Ind, "Your muscles bulge, and your skin turns to amber!");
			do_xtra_stats(Ind, 4, 8, 20 + rand_int(5));
			set_shero(Ind, 20); /* -AC cancelled by blessing below */
			p_ptr->blessed_power = 35;
			set_blessed(Ind, 20);
			break;
		}
	}

	bypass_invuln = FALSE;
	return (ident);
}

/*
 * Quaff a potion (from the pack or the floor)
 */
void do_cmd_quaff_potion(int Ind, int item) {
	player_type *p_ptr = Players[Ind];
	int ident, lev;
	object_type *o_ptr;
	bool flipped = FALSE;


	/* Restrict choices to potions (apparently meanless) */
	item_tester_tval = TV_POTION;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &p_ptr->inventory[item];
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max) return; /* item doesn't exist */
		o_ptr = &o_list[0 - item];
	}

	/* Hack -- allow to quaff ale/wine */
	if (o_ptr->tval == TV_FOOD) {
		do_cmd_eat_food(Ind, item);
		return;
	}

	if (check_guard_inscription(o_ptr->note, 'q')) {
		msg_print(Ind, "The item's inscription prevents it.");
		return;
	};

	if (!can_use_verbose(Ind, o_ptr)) return;

	if ((o_ptr->tval != TV_POTION) && (o_ptr->tval != TV_POTION2)) {
		//(may happen on death, from macro spam)	msg_print(Ind, "SERVER ERROR: Tried to quaff non-potion!");
		return;
	}

	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	/* For outdated learning potions */
	if (o_ptr->xtra1) {
		msg_print(Ind, "This potion seems to have crystallized and cannot be consumed anymore.");
		return;
	}

	/* Not identified yet */
	ident = FALSE;

	/* Object level */
	lev = k_info[o_ptr->k_idx].level;

	process_hooks(HOOK_QUAFF, "d", Ind);

	ident = quaff_potion(Ind, o_ptr->tval, o_ptr->sval, o_ptr->pval);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* An identification was made */
	if (ident && !object_aware_p(Ind, o_ptr)) {
		flipped = object_aware(Ind, o_ptr);
		//object_known(o_ptr);//only for object1.c artifact potion description... maybe obsolete
		if (!(p_ptr->mode & MODE_PVP)) gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* The item has been tried */
	object_tried(Ind, o_ptr, flipped);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Potions can feed the player */
	if (p_ptr->prace == RACE_VAMPIRE) {
		if (o_ptr->sval == SV_POTION_BLOOD) {
			int feed = o_ptr->pval + p_ptr->food;
			/* never get gorged */
			if (feed >= PY_FOOD_MAX) feed = PY_FOOD_MAX - 1;
			set_food(Ind, feed);
		}
	} else if (p_ptr->prace == RACE_ENT) {
		if (o_ptr->sval == SV_POTION_WATER) (void)set_food(Ind, p_ptr->food + WATER_ENT_FOOD);
		else (void)set_food(Ind, p_ptr->food + (o_ptr->pval * 2));
	} else if (p_ptr->suscep_life) {
		(void)set_food(Ind, p_ptr->food + (o_ptr->pval * 2) / 3);
	} else
		(void)set_food(Ind, p_ptr->food + o_ptr->pval);

	if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1);
	questitem_d(o_ptr, 1);

	/* Destroy a potion in the pack */
	if (item >= 0) {
		inven_item_increase(Ind, item, -1);
		inven_item_describe(Ind, item);
		inven_item_optimize(Ind, item);
	}

	/* Destroy a potion on the floor */
	else {
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

#ifdef USE_SOUND_2010
	sound(Ind, "quaff_potion", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
}

#ifdef FOUNTAIN_GUARDS
static void fountain_guard(int Ind, bool blood) {
	player_type *p_ptr = Players[Ind];
	int ridx = 0;

	if (!magik(FOUNTAIN_GUARDS)) return;

	if (blood) {
		if (getlevel(&p_ptr->wpos) >= 60) { switch (randint(3)) { case 1: ridx = 758; break; case 2: ridx = 994; break; case 3: ridx = 812; }
		} else if (getlevel(&p_ptr->wpos) >= 50) { switch (randint(3)) { case 1: ridx = 662; break; case 2: ridx = 569; case 3: ridx = 659; }
		} else { switch (randint(3)) { case 1: ridx = 566; break; break; case 2: ridx = 357; break; case 3: ridx = 568; }
		}
	} else {
		if (getlevel(&p_ptr->wpos) >= 40) { switch (randint(2)) { case 1: ridx = 924; break; case 2: ridx = 893; }
		} else if (getlevel(&p_ptr->wpos) >= 35) { switch (randint(3)) { case 1: ridx = 1038; break; case 2: ridx = 894; break; case 3: ridx = 902; }
		} else if (getlevel(&p_ptr->wpos) >= 30) { switch (randint(2)) { case 1: ridx = 512; break; case 2: ridx = 509; }
		} else if (getlevel(&p_ptr->wpos) >= 25) { ridx = 443;
		} else if (getlevel(&p_ptr->wpos) >= 20) { switch (randint(4)) { case 1: ridx = 919; break; case 2: ridx = 882; break; case 3: ridx = 927; break; case 4: ridx = 1057; }
		} else if (getlevel(&p_ptr->wpos) >= 15) { switch (randint(3)) { case 1: ridx = 303; break; case 2: ridx = 923; break; case 3: ridx = 926; }
		} else if (getlevel(&p_ptr->wpos) >= 10) { ridx = 925;
		} else if (getlevel(&p_ptr->wpos) >= 5) { ridx = 207;
		} else { ridx = 900;
		}
	}

	if (ridx) {
		s_printf("FOUNTAIN_GUARDS: %d ", ridx);

		msg_print(Ind, "A monster appears in the fountain!");
		summon_override_checks = SO_GRID_TERRAIN | SO_IDDC | SO_PLAYER_SUMMON;
		if (summon_specific_race(&p_ptr->wpos, p_ptr->py, p_ptr->px, ridx, 0, 1))
			s_printf("ok.\n");
		else s_printf("failed.\n");
		summon_override_checks = SO_NONE;
	}
}
#endif

/*
 * Drink from a fountain
 */
void do_cmd_drink_fountain(int Ind) {
	player_type *p_ptr = Players[Ind];
	cave_type **zcave;
	cave_type *c_ptr;
	c_special *cs_ptr;

	bool ident;
	int tval, sval, pval = 0, k_idx;

	if (!(zcave = getcave(&p_ptr->wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];


	/* HACK (sorta bad): if there's an item on the grid,
	   tell us which it is, instead of trying to drink from
	   a fountain! - C. Blue */
	if (c_ptr->o_idx) {
		whats_under_your_feet(Ind, TRUE);
		return;
	}


	/* decided to allow players in WRAITHFORM to drink ;) */
	if (p_ptr->ghost) {
		msg_print(Ind, "Ghosts cannot interact.");
		return;
	}

	if (c_ptr->feat == FEAT_EMPTY_FOUNTAIN) {
		msg_print(Ind, "The fountain is dried out.");
		return;
	} else if (c_ptr->feat == FEAT_DEEP_WATER ||
	    c_ptr->feat == FEAT_SHAL_WATER) {
#ifdef USE_SOUND_2010
		sound(Ind, "quaff_potion", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
		/* if it's the sea, it's salt water */
		if (!p_ptr->wpos.wz)
#if 0 /* problem: WILD_COAST is used for both oceans and lakes */
		    switch (wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type) {
			case WILD_SHORE1:
			case WILD_SHORE2:
			case WILD_OCEANBED1:
			case WILD_OCEANBED2:
			case WILD_OCEAN:
			case WILD_COAST:
#else /* abuse ambient-sfx logic - it works pretty well ^^ */
			if (p_ptr->sound_ambient == SFX_AMBIENT_SHORE) {
#endif /* actually instead, to do this non-hackily, it should just check for .type AND .bled to not be WILD_OCEAN (same result as this hack) */
				if (p_ptr->prace == RACE_ENT) msg_print(Ind, "The water is too salty to feed off.");
				else msg_print(Ind, "The water tastes very salty.");

				/* Take a turn */
				p_ptr->energy -= level_speed(&p_ptr->wpos);
				return;
		}
		/* lake/river: fresh water */
		if (!p_ptr->suscep_life) msg_print(Ind, "You feel less thirsty.");
		else msg_print(Ind, "You drink some.");
		if (p_ptr->prace == RACE_ENT) (void)set_food(Ind, p_ptr->food + WATER_ENT_FOOD);

#ifdef FOUNTAIN_GUARDS
		//(unlimited charges!) fountain_guard(Ind, FALSE);
#endif
		/* Take a turn */
		p_ptr->energy -= level_speed(&p_ptr->wpos);
		return;
	} else if (c_ptr->feat == FEAT_FOUNTAIN_BLOOD) {
#ifdef USE_SOUND_2010
		sound(Ind, "quaff_potion", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

		if (!(cs_ptr = GetCS(c_ptr, CS_FOUNTAIN))) {
			/* paranoia? */
			return;
		}

		if (p_ptr->prace == RACE_VAMPIRE) {
			int feed = k_info[lookup_kind(TV_POTION, SV_POTION_BLOOD)].pval + p_ptr->food;

			switch (rand_int(3)) {
			case 0: msg_print(Ind, "Delicious.");
			case 1: msg_print(Ind, "It's fresh.");
			case 2: msg_print(Ind, "You're less thirsty.");
			}

			/* never get gorged */
			if (feed >= PY_FOOD_MAX) feed = PY_FOOD_MAX - 1;
			set_food(Ind, feed);
		} else if (p_ptr->suscep_life) {
			msg_print(Ind, "You feel less thirsty.");
			(void)set_food(Ind, p_ptr->food + 100);
		} else {
			switch (rand_int(3)) {
			case 0: msg_print(Ind, "Ew."); break;
			case 1: msg_print(Ind, "Gross."); break;
			case 2: msg_print(Ind, "Nasty."); break;
			}
		}

#ifdef FOUNTAIN_GUARDS
		fountain_guard(Ind, TRUE);
#endif

		/* Take a turn */
		p_ptr->energy -= level_speed(&p_ptr->wpos);

		cs_ptr->sc.fountain.rest--;
		if (cs_ptr->sc.fountain.rest <= 0) {
			cave_set_feat(&p_ptr->wpos, p_ptr->py, p_ptr->px, FEAT_EMPTY_FOUNTAIN);
			cs_erase(c_ptr, cs_ptr);
			msg_print(Ind, "The fountain is dried out.");
		}
		return;
	}
	//else if (create_snowball(Ind, c_ptr)) return;

	if (c_ptr->feat != FEAT_FOUNTAIN) {
		//non-fountain-specific message, since we could also make snowballs instead of just filling bottles..
		msg_print(Ind, "There is nothing here.");
		return;
	}

	/* Oops! */
	if (!(cs_ptr = GetCS(c_ptr, CS_FOUNTAIN))) {
#ifdef USE_SOUND_2010
		sound(Ind, "quaff_potion", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
		if (!p_ptr->suscep_life) msg_print(Ind, "You feel less thirsty.");
		else msg_print(Ind, "You drink some.");
		if (p_ptr->prace == RACE_ENT) (void)set_food(Ind, p_ptr->food + WATER_ENT_FOOD);

#ifdef FOUNTAIN_GUARDS
		//(unlimited charges!) fountain_guard(Ind, FALSE);
#endif
		/* Take a turn */
		p_ptr->energy -= level_speed(&p_ptr->wpos);
		return;
	}

	if (cs_ptr->sc.fountain.rest <= 0) {
		msg_print(Ind, "The fountain is dried out.");
		return;
	}

	if (cs_ptr->sc.fountain.type <= SV_POTION_LAST) {
		tval = TV_POTION;
		sval = cs_ptr->sc.fountain.type;
	} else {
		tval = TV_POTION2;
		sval = cs_ptr->sc.fountain.type - SV_POTION_LAST;
	}

	k_idx = lookup_kind(tval, sval);

	/* Doh! */
	if (!k_idx) {
#ifdef USE_SOUND_2010
		sound(Ind, "quaff_potion", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
		if (!p_ptr->suscep_life) msg_print(Ind, "You feel less thirsty.");
		else msg_print(Ind, "You drink some.");
		if (p_ptr->prace == RACE_ENT) (void)set_food(Ind, p_ptr->food + WATER_ENT_FOOD);

#ifdef FOUNTAIN_GUARDS
		//(unlimited charges!) fountain_guard(Ind, FALSE);
#endif
		/* Take a turn */
		p_ptr->energy -= level_speed(&p_ptr->wpos);
		return;
	}

	pval = k_info[k_idx].pval;

	/* S(he) is no longer afk */
	un_afk_idle(Ind);
#ifdef USE_SOUND_2010
	sound(Ind, "quaff_potion", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

	s_printf("FOUNTAIN: %s: %s\n", p_ptr->name, k_name + k_info[k_idx].name);

	/* a little hack: avoid the extra less-thirsty-message for the 3 basic effectless potions.
	   Idea: these potions do not give vampires a 'less thirsty' message, but when a vampire
	   drinks from these fountains, he should still get a "you drink some" confirmation. */
	if (tval == TV_POTION)
		switch (sval) {
		case SV_POTION_WATER:
		case SV_POTION_APPLE_JUICE:
		case SV_POTION_SLIME_MOLD:
			ident = TRUE;
			if (p_ptr->prace != RACE_VAMPIRE) msg_print(Ind, "You feel less thirsty.");
			else msg_print(Ind, "You drink some.");
			break;
		default:
			ident = quaff_potion(Ind, tval, sval, -1 - pval);
		}
	else ident = quaff_potion(Ind, tval, sval, -1 - pval);
	if (ident) cs_ptr->sc.fountain.known = TRUE;
	else if (p_ptr->prace != RACE_VAMPIRE) msg_print(Ind, "You feel less thirsty.");
	else msg_print(Ind, "You drink some.");

	if (p_ptr->prace == RACE_VAMPIRE) ;
	else if (p_ptr->prace == RACE_ENT) {
		if (sval == SV_POTION_WATER) (void)set_food(Ind, p_ptr->food + WATER_ENT_FOOD);
		else (void)set_food(Ind, p_ptr->food + pval * 2);
	} else if (p_ptr->suscep_life)
		(void)set_food(Ind, p_ptr->food + (pval * 2) / 3);
	else
		(void)set_food(Ind, p_ptr->food + pval);

	cs_ptr->sc.fountain.rest--;
	if (cs_ptr->sc.fountain.rest <= 0) {
		cave_set_feat(&p_ptr->wpos, p_ptr->py, p_ptr->px, FEAT_EMPTY_FOUNTAIN);
		cs_erase(c_ptr, cs_ptr);
		msg_print(Ind, "The fountain is dried out.");
	}

#ifdef FOUNTAIN_GUARDS
	fountain_guard(Ind, FALSE);
#endif

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);
}



/*
 * Fill an empty bottle
 */
/* XXX this can be abusable; disable it in case */
/* TODO: allow to fill on FEAT_DEEP_WATER (potion of water :) */
/* Allow filling bottles with fountain blood? */
#define ALLOW_BLOOD_BOTTLING
void do_cmd_fill_bottle(int Ind) {
	player_type *p_ptr = Players[Ind];
	cave_type **zcave;
	cave_type *c_ptr;
	c_special *cs_ptr;
#ifdef FOUNTAIN_GUARDS
	bool guard = TRUE;
#endif

	//bool ident;
	int tval, sval, k_idx, item;

	object_type *q_ptr, forge;
	//cptr q, s;


	if (!(zcave = getcave(&p_ptr->wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	if (c_ptr->feat == FEAT_EMPTY_FOUNTAIN) {
		msg_print(Ind, "The fountain is dried out.");
		return;
	}

#ifndef ALLOW_BLOOD_BOTTLING
	if (c_ptr->feat == FEAT_FOUNTAIN_BLOOD) {
		msg_print(Ind, "Fresh blood perishes too quickly.");
		return;
	}

	/* don't fill from fountain but from liquid of the floor feature? */
	if (c_ptr->feat != FEAT_FOUNTAIN) {
#else
	if (c_ptr->feat != FEAT_FOUNTAIN && c_ptr->feat != FEAT_FOUNTAIN_BLOOD) {
#endif
		if (c_ptr->feat == FEAT_SHAL_WATER || c_ptr->feat == FEAT_DEEP_WATER) {
			if (p_ptr->tim_wraith) { /* not in WRAITHFORM */
				msg_print(Ind, "The water seems to pass through the bottle!");
				return;
			}

#if 0 /* problem: WILD_COAST is used for both oceans and lakes */
			switch (wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type) {
			case WILD_SHORE1:
			case WILD_SHORE2:
			case WILD_OCEANBED1:
			case WILD_OCEANBED2:
			case WILD_OCEAN:
			case WILD_COAST:
#else /* abuse ambient-sfx logic - it works pretty well ^^ */
			if (p_ptr->sound_ambient == SFX_AMBIENT_SHORE)
#endif /* actually instead, to do this non-hackily, it should just check for .type AND .bled to not be WILD_OCEAN (same result as this hack) */
				/* salt water */
				k_idx = lookup_kind(TV_POTION, SV_POTION_SALT_WATER);
			else
				/* normal water */
				k_idx = lookup_kind(TV_POTION, SV_POTION_WATER);

			if (!get_something_tval(Ind, TV_BOTTLE, &item)) {
				msg_print(Ind, "You have no bottles to fill.");
				return;
			}

			un_afk_idle(Ind);

			/* Destroy a bottle in the pack */
			if (item >= 0) {
				inven_item_increase(Ind, item, -1);
				inven_item_describe(Ind, item);
				inven_item_optimize(Ind, item);
			}
			/* Destroy a potion on the floor */
			else {
				floor_item_increase(0 - item, -1);
				floor_item_describe(0 - item);
				floor_item_optimize(0 - item);
			}

			/* Create the potion */
			q_ptr = &forge;
			object_prep(q_ptr, k_idx);
			q_ptr->number = 1;
			//q_ptr->owner = p_ptr->id;
			determine_level_req(getlevel(&p_ptr->wpos), q_ptr);

			object_aware(Ind, q_ptr);
			object_known(q_ptr);
			q_ptr->iron_trade = p_ptr->iron_trade;
			item = inven_carry(Ind, q_ptr);
			if (item >= 0) inven_item_describe(Ind, item);
			p_ptr->energy -= level_speed(&p_ptr->wpos);
			return;
		}

		msg_print(Ind, "You see nothing here to fill bottles with.");
		return;
	}

	/* Oops! */
	if (!(cs_ptr = GetCS(c_ptr, CS_FOUNTAIN))) return;

	if (cs_ptr->sc.fountain.rest <= 0) {
		msg_print(Ind, "The fountain is dried out.");
		return;
	}

	if (p_ptr->tim_wraith) { /* not in WRAITHFORM */
		msg_print(Ind, "The water seems to pass through the bottle!");
		return;
	}

	if (cs_ptr->sc.fountain.type <= SV_POTION_LAST) {
		tval = TV_POTION;
		sval = cs_ptr->sc.fountain.type;
	} else {
		tval = TV_POTION2;
		sval = cs_ptr->sc.fountain.type - SV_POTION_LAST;
	}

	k_idx = lookup_kind(tval, sval);

	/* Doh! */
	if (!k_idx) {
#ifdef FOUNTAIN_GUARDS
		guard = FALSE;
#endif
		k_idx = lookup_kind(TV_POTION, SV_POTION_WATER);
	}


	if (!get_something_tval(Ind, TV_BOTTLE, &item)) {
		msg_print(Ind, "You have no bottles to fill.");
		return;
	}

	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	/* Destroy a bottle in the pack */
	if (item >= 0) {
		inven_item_increase(Ind, item, -1);
		inven_item_describe(Ind, item);
		inven_item_optimize(Ind, item);
	}
	/* Destroy a potion on the floor */
	else {
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

	/* Create the potion */
	q_ptr = &forge;
	object_prep(q_ptr, k_idx);
	q_ptr->number = 1;
	//q_ptr->owner = p_ptr->id;
	determine_level_req(getlevel(&p_ptr->wpos), q_ptr);

	//if (c_ptr->info & CAVE_IDNT)
	if (cs_ptr->sc.fountain.known) {
		object_aware(Ind, q_ptr);
		object_known(q_ptr);
	}
	else if (object_aware_p(Ind, q_ptr)) cs_ptr->sc.fountain.known = TRUE;
#ifdef ALLOW_BLOOD_BOTTLING
	if (q_ptr->sval == SV_POTION_BLOOD) q_ptr->timeout = 1750 + rand_int(501);//goes bad after a while!
#endif
	q_ptr->iron_trade = p_ptr->iron_trade;
	item = inven_carry(Ind, q_ptr);
	if (item >= 0) inven_item_describe(Ind, item);

	cs_ptr->sc.fountain.rest--;

	if (cs_ptr->sc.fountain.rest <= 0) {
		cave_set_feat(&p_ptr->wpos, p_ptr->py, p_ptr->px, FEAT_EMPTY_FOUNTAIN);
		cs_erase(c_ptr, cs_ptr);
	}

#ifdef FOUNTAIN_GUARDS
	if (guard) fountain_guard(Ind, q_ptr->sval == SV_POTION_BLOOD);
#endif

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);
}


/*
 * Empty a potion in the backpack
 */
void do_cmd_empty_potion(int Ind, int slot) {
	player_type *p_ptr = Players[Ind];

	//bool ident;
	int tval;//, k_idx, item;

	object_type *o_ptr, *q_ptr, forge;
	//cptr q, s;

	o_ptr = &p_ptr->inventory[slot];
	if (!o_ptr->k_idx) return;

	tval = o_ptr->tval;

	/* specialty: empty brass lanterns, just to make them stack and sell later */
	if (tval == TV_LITE && o_ptr->sval == SV_LITE_LANTERN) {
		if (o_ptr->number == 1) msg_print(Ind, "You empty the lantern.");
		else msg_print(Ind, "You empty the stack of lanterns.");

		o_ptr->timeout = 0;
#if 0 /* currently the /empty command translates all slots to lower-case/inventory-only */
		if (slot <= INVEN_PACK) p_ptr->window |= PW_INVEN;
		else {
			disturb(Ind, 0, 0);
			p_ptr->window |= PW_EQUIP;
			p_ptr->update |= (PU_TORCH | PU_BONUS);
		}
#else
		p_ptr->window |= PW_INVEN;
#endif
		return;
	}

	if (tval == TV_FLASK) {
		msg_print(Ind, "\377oYou cannot use flasks for making potions.");
		return;
	}

	if (tval != TV_POTION && tval != TV_POTION2) {
		msg_print(Ind, "\377oYou cannot empty that.");
		return;
	}

	/* Create an empty bottle */
	q_ptr = &forge;
	object_wipe(q_ptr);
	q_ptr->number = 1;
	invcopy(q_ptr, lookup_kind(TV_BOTTLE, SV_EMPTY_BOTTLE));
	q_ptr->level = o_ptr->level;

	/* Destroy a potion in the pack */
	inven_item_increase(Ind, slot, -1);
	inven_item_describe(Ind, slot);
	inven_item_optimize(Ind, slot);

	/* let the player carry the bottle */
	q_ptr->iron_trade = p_ptr->iron_trade;
	slot = inven_carry(Ind, q_ptr);
	if (slot >= 0) inven_item_describe(Ind, slot);

	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);
}


/*
 * Curse the players armor
 */
bool curse_armor(int Ind) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	char o_name[ONAME_LEN];


	/* Curse the body armor */
	o_ptr = &p_ptr->inventory[INVEN_BODY];

	/* Nothing to curse */
	if (!o_ptr->k_idx) return (FALSE);

	/* might mess up quest stuff */
	if (o_ptr->questor ||
	    (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_QUEST))
		return FALSE;


	/* Describe */
	object_desc(Ind, o_name, o_ptr, FALSE, 3);

	/* Attempt a saving throw for artifacts */
	if (artifact_p(o_ptr) && (rand_int(100) < 30)) {
		/* Cool */
		msg_format(Ind, "A %s tries to %s, but your %s resists the effects!",
		    "terrible black aura", "surround your armour", o_name);
	}

	/* not artifact or failed save... */
	else {
		/* Oops */
		msg_format(Ind, "A terrible black aura blasts your %s!", o_name);

		if (true_artifact_p(o_ptr))
			handle_art_d(o_ptr->name1);

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
bool curse_weapon(int Ind) {
	player_type *p_ptr  = Players[Ind];
	object_type *o_ptr;
	char o_name[ONAME_LEN];

	/* Curse the weapon */
	o_ptr = &p_ptr->inventory[INVEN_WIELD];

	/* Nothing to curse */
	if (!o_ptr->k_idx &&
	    (!p_ptr->inventory[INVEN_ARM].k_idx || p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)) return (FALSE);
	if (!o_ptr->k_idx) o_ptr = &p_ptr->inventory[INVEN_ARM]; /* dual-wield..*/

	/* might mess up quest stuff */
	if (o_ptr->questor ||
	    (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_QUEST))
		return FALSE;


	/* Describe */
	object_desc(Ind, o_name, o_ptr, FALSE, 3);

	/* Attempt a saving throw */
	if (artifact_p(o_ptr) && (rand_int(100) < 30)) {
		/* Cool */
		msg_format(Ind, "A %s tries to %s, but your %s resists the effects!",
		    "terrible black aura", "surround your weapon", o_name);
	}

	/* not artifact or failed save... */
	else {
		/* Oops */
		msg_format(Ind, "A terrible black aura blasts your %s!", o_name);

		if (true_artifact_p(o_ptr))
			handle_art_d(o_ptr->name1);

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
 * Curse the players equipment in general	- Jir -
 */
#if 0	// let's use this for Hand-o-Doom :)
bool curse_an_item(int Ind, int slot) {
	player_type *p_ptr = Players[Ind];

	object_type *o_ptr;

	char o_name[ONAME_LEN];


	/* Curse the body armor */
	o_ptr = &p_ptr->inventory[slot];

	/* Nothing to curse */
	if (!o_ptr->k_idx) return (FALSE);

	/* might mess up quest stuff */
	if (o_ptr->questor ||
	    (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_QUEST))
		return FALSE;

	/* Describe */
	object_desc(Ind, o_name, o_ptr, FALSE, 3);

	/* Attempt a saving throw for artifacts */
	if (artifact_p(o_ptr) && (rand_int(100) < 50)) {
		/* Cool */
		msg_format(Ind, "A %s tries to %s, but your %s resists the effects!",
		    "terrible black aura", "surround you", o_name);
	}

	/* not artifact or failed save... */
	else {
		/* Oops */
		msg_format(Ind, "A terrible black aura blasts your %s!", o_name);

		if (true_artifact_p(o_ptr))
			handle_art_d(o_ptr->name1);

		/* Blast the armor */
		o_ptr->name1 = 0;
		o_ptr->name2 = EGO_BLASTED;	// ?
		o_ptr->name3 = 0;
		o_ptr->to_a = 0 - randint(5) - randint(5);
		o_ptr->to_h = 0 - randint(5) - randint(5);
		o_ptr->to_d = 0 - randint(5) - randint(5);
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
#endif	// 0

/*
 * Cancel magic in inventory.		- Jir -
 * Crappy, isn't it?  But it can..
 *
 * 0x01 - Affect the equipments too
 * 0x02 - Turn scrolls/potions/wands/rods/staves into 'Nothing' kind
 */
bool do_cancellation(int Ind, int flags) {
	player_type *p_ptr = Players[Ind];
	int i;
	bool ident = TRUE;

	s_printf("CANCELLATION: %s (%d)\n", p_ptr->name, flags);

	for (i = 0; i < ((flags & 0x01) ? INVEN_TOTAL : INVEN_WIELD); i++) {
		object_type *o_ptr = &p_ptr->inventory[i];
		if (!o_ptr->k_idx) continue;
		if (like_artifact_p(o_ptr)) continue;
		if (o_ptr->tval == TV_KEY) continue;
		if (o_ptr->tval == TV_FOOD) continue;
		if (o_ptr->tval == TV_FLASK) continue;
		if (o_ptr->tval == TV_CHEST) continue;
		if (o_ptr->name2 && o_ptr->name2 != EGO_SHATTERED && o_ptr->name2 != EGO_BLASTED) {
			ident = TRUE;
			o_ptr->name2 = 0;
		}
		if (o_ptr->name2b && o_ptr->name2b != EGO_SHATTERED && o_ptr->name2b != EGO_BLASTED) {
			ident = TRUE;
			o_ptr->name2b = 0;
		}
		if (o_ptr->name3) o_ptr->name3 = 0; //Kurzel - Fix stacking, see object2.c
		if (o_ptr->timeout_magic) { //Only affects magical timeouts at the moment, and not even auto-recharging state
			ident = TRUE;
			if (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_POLYMORPH)
				o_ptr->timeout_magic = 1;
			else
				o_ptr->timeout_magic = 0;
		}
		/* pval for potions is their nourishment value, so exempt them */
		if (o_ptr->tval != TV_POTION && o_ptr->pval > 0) {
			ident = TRUE;
			o_ptr->pval = 0;
		}
		if (o_ptr->bpval) {
			ident = TRUE;
			o_ptr->bpval = 0;
		}
		if (o_ptr->to_h > 0) {
			ident = TRUE;
			o_ptr->to_h = 0;
		}
		if (o_ptr->to_d > 0) {
			ident = TRUE;
			o_ptr->to_d = 0;
		}
		if (o_ptr->to_a > 0) {
			ident = TRUE;
			o_ptr->to_a = 0;
		}
#if 0	// Not so useful anyway
		if (flags & 0x02) {
			switch (o_ptr->tval)
			{
			}
		}
#endif	// 0
	}

	return (ident);
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

/* Pfft, it's silly long */
#define LOTTERY_MAX_PRIZE	7
static void do_lottery(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];
	int i = k_info[o_ptr->k_idx].cost, j, k = 0, i_drop = i, gold;

	i -= i * o_ptr->discount / 100;

	/* 30 * 4^7 = 30 * 16384 =   491,520 */
	/* 30 * 5^7 = 30 * 78125 = 2,343,750 */
	for (j = 0; j < LOTTERY_MAX_PRIZE; j++) {
		if (rand_int(4)) break;
		if (k) i *= 5;
		k++;
	}

	if (!j) {
		msg_print(Ind, "\377WYou draw a blank :-P");
		s_printf("Lottery results: %s drew a blank.\n", p_ptr->name);
	} else {
		cptr p = "th";

		k = LOTTERY_MAX_PRIZE + 1 - k;

		if ((k % 10) == 1) p = "st";
		else if ((k % 10) == 2) p = "nd";
		else if ((k % 10) == 3) p = "rd";

		s_printf("Lottery results: %s won the %d%s prize of %d Au.\n", p_ptr->name, k, p, i);

		if (k < 4) {
			if (k == 1) msg_broadcast_format(0, "\374\377y$$$ \377%c%s seems to hit the big time! \377y$$$", COLOUR_CHAT, p_ptr->name);
			else msg_broadcast_format(0, "\374\377%c%s seems to hit the big time!", COLOUR_CHAT, p_ptr->name);
			set_confused(Ind, p_ptr->confused + rand_int(10) + 10);
			set_image(Ind, p_ptr->image + rand_int(10) + 10);
		}
		msg_format(Ind, "\374\377%cYou won the %d%s prize!", COLOUR_CHAT, k, p);

		gold = i;

		/* Invert it again for following calcs (#else branch below) */
		k = LOTTERY_MAX_PRIZE + 1 - k;
		i_drop *= 2;
		for (j = 0; j < k; j++) {
			i_drop *= 3;
		}

		while (gold > 0) {
			object_type forge, *j_ptr = &forge;
			int drop;

#if 0 /*Too many piles*/
			drop = (i > 1000) ?
			    randint(i / 10 / (LOTTERY_MAX_PRIZE * LOTTERY_MAX_PRIZE
			    + 1 - k * k)) * 10 : i;
#else
			drop = i_drop;
#endif
			if (drop > gold) drop = gold;

			/* Wipe the object */
			object_wipe(j_ptr);

			/* Prepare a gold object */
			invcopy(j_ptr, gold_colour(drop, TRUE, FALSE));

			/* Determine how much the treasure is "worth" */
			//j_ptr->pval = (gold >= 15000) ? 15000 : gold;
			j_ptr->pval = drop;

			drop_near(0, j_ptr, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);

			gold -= drop;
		}

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);
	}
}

/*
 * Too much summon cheeze - check it before letting them summon - evileye
 * Let's not summon on worldmap surface at all but only in dungeons/towers,
 * since those have level requirements (C. Blue)
 *
 */
static int check_self_summon(player_type *p_ptr){
	cave_type **zcave, *c_ptr;
	struct dun_level *l_ptr = getfloor(&p_ptr->wpos);

	if (is_admin(p_ptr)) return(TRUE);

	if (l_ptr && (l_ptr->flags2 & LF2_NO_SUMMON)) return FALSE;

	if (((!cfg.surface_summoning) && (p_ptr->wpos.wz == 0))
	    || istownarea(&p_ptr->wpos, MAX_TOWNAREA)) /* poly ring anticheeze (those don't run out in town area) */
		return(FALSE);

	zcave = getcave(&p_ptr->wpos);
	if (zcave) {
		c_ptr = &zcave[p_ptr->py][p_ptr->px];

		/* not on wilderness edge */
		if (p_ptr->wpos.wz || in_bounds(p_ptr->py, p_ptr->px)) {
			/* and not sitting on the stairs */
			if(c_ptr->feat != FEAT_LESS && c_ptr->feat != FEAT_MORE &&
			    c_ptr->feat != FEAT_WAY_LESS && c_ptr->feat != FEAT_WAY_MORE &&
			    c_ptr->feat != FEAT_BETWEEN)
				return(TRUE);
		}
	}

	return(FALSE);
}

bool read_scroll(int Ind, int tval, int sval, object_type *o_ptr, int item, bool *used_up, bool *keep) {
	player_type *p_ptr = Players[Ind];
	int ident = FALSE;
	int k;
	char	m_name[MNAME_LEN];
	monster_type    *m_ptr;
	monster_race    *r_ptr;

	/* Analyze the scroll */
	if (tval == TV_SCROLL) {
		switch (sval) {
		case SV_SCROLL_HOUSE:
		{
			if (!o_ptr) break;

			/* With my fix, this scroll causes crash when rebooting server.
			 * w/o my fix, this scroll causes crash when finishing house
			 * creation.  Pfft
			 */
			//unsigned char *ins = quark_str(o_ptr->note);
			cptr ins = quark_str(o_ptr->note);
			bool floor = TRUE;
			bool jail = FALSE;
			msg_print(Ind, "This is a house creation scroll.");
			ident = TRUE;
			if(ins){
				while ((*ins != '\0')) {
					if (*ins == '@') {
						ins++;
						if (*ins == 'F') {
							floor = FALSE;
						}
						if (*ins == 'J') {
							jail = TRUE;
						}
					}
					ins++;
				}
			}
			house_creation(Ind, floor, jail);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_GOLEM:
			if (o_ptr) msg_print(Ind, "This is a golem creation scroll.");
			ident = TRUE;
			golem_creation(Ind, 1);
			break;

		case SV_SCROLL_BLOOD_BOND:
			if (!o_ptr) break;

			msg_print(Ind, "This is a blood bond scroll.");
			ident = TRUE;
			if (!(p_ptr->mode & MODE_PVP)) blood_bond(Ind, o_ptr);
			else msg_print(Ind, "True gladiators must fight to the death!");
			break;

		case SV_SCROLL_ARTIFACT_CREATION:
			if (!o_ptr) break;

			msg_print(Ind, "This is an artifact creation scroll.");
			ident = TRUE;
			/* check for 'no +LIFE' inscription */
			if (o_ptr->note && check_guard_inscription(o_ptr->note, 'L'))
				(void)create_artifact(Ind, TRUE);
			else
				(void)create_artifact(Ind, FALSE);
			*used_up = FALSE;
			p_ptr->using_up_item = item;
			break;

		case SV_SCROLL_DARKNESS:
			if (unlite_area(Ind, 10, 6)) ident = TRUE;
			if (!p_ptr->resist_dark)
				(void)set_blind(Ind, p_ptr->blind + 3 + randint(5));
			break;

		case SV_SCROLL_AGGRAVATE_MONSTER:
#ifdef USE_SOUND_2010
			sound_near(Ind, "shriek", NULL, SFX_TYPE_MON_SPELL);
#endif
 				msg_print(Ind, "\377RThere is a high-pitched humming noise.");
 				msg_print_near(Ind, "\377RThere is a high-pitched humming noise.");
			aggravate_monsters(Ind, -1);
			ident = TRUE;
			break;

		case SV_SCROLL_CURSE_ARMOR:
			if (curse_armor(Ind)) ident = TRUE;
			break;

		case SV_SCROLL_CURSE_WEAPON:
			if (curse_weapon(Ind)) ident = TRUE;
			break;

		case SV_SCROLL_SUMMON_MONSTER:
			if (!check_self_summon(p_ptr)) break;
			s_printf("SUMMON_MONSTER: %s\n", p_ptr->name);
			summon_override_checks = SO_IDDC | SO_PLAYER_SUMMON;
			for (k = 0; k < randint(3); k++) {
				if (summon_specific(&p_ptr->wpos, p_ptr->py, p_ptr->px, getlevel(&p_ptr->wpos), 0, SUMMON_ALL_U98, 1, 0))
					ident = TRUE;
			}
			summon_override_checks = SO_NONE;
#ifdef USE_SOUND_2010
			if (ident) sound_near_site(p_ptr->py, p_ptr->px, &p_ptr->wpos, 0, "summon", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
			break;

		case SV_SCROLL_CONJURE_MONSTER:
		{
			char m_name_real[MNAME_LEN];

			if (!o_ptr) break;

			if (!check_self_summon(p_ptr)) break;
			k = get_monster(Ind, o_ptr);
			if (!k) break;
			if (r_info[k].flags1 & RF1_UNIQUE) break;

			monster_race_desc(Ind, m_name, k, 0x88);
			monster_race_desc(Ind, m_name_real, k, 0x188);
			msg_format(Ind, "\377oYou conjure %s!", m_name);
			msg_print_near_monvar(Ind, -1,
			    format("\377o%s conjures %s", p_ptr->name, m_name_real),
			    format("\377o%s conjures %s", p_ptr->name, m_name),
			    format("\377o%s conjures some entity", p_ptr->name));//(unused)

			/* clone to avoid heavy mimic cheeze (and maybe exp cheeze) */
			if (!place_monster_one(&p_ptr->wpos, p_ptr->py, p_ptr->px, k, FALSE, FALSE, FALSE, 100, 100)) ident = TRUE;
#ifdef USE_SOUND_2010
			if (ident) sound_near_site(p_ptr->py, p_ptr->px, &p_ptr->wpos, 0, "summon", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
			break;
		}

		case SV_SCROLL_SLEEPING:
			msg_print(Ind, "A veil of sleep falls down over the wide surroundings..");
			for (k = m_top - 1; k >= 0; k--) {
				/* Access the monster */
				m_ptr = &m_list[m_fast[k]];
				r_ptr = race_inf(m_ptr);
				/* On this level? Test its highest encounter so far */
				if (inarea(&m_ptr->wpos, &p_ptr->wpos) &&
				    !((r_ptr->flags1 & RF1_UNIQUE) ||
				    (r_ptr->flags3 & RF3_NO_SLEEP)))
					m_ptr->csleep = 1000;
			}
			break;

		/* not adding bounds checking now... because of perma walls
		   hope that I don't need to......

		   OK, modified so you cant ressurect ghosts in walls......
		   to prevent bad things from happening in town.
		   */
		case SV_SCROLL_LIFE:
			/* only restore life levels if no resurrect */
			if(!do_scroll_life(Ind))
				restore_level(Ind);
			break;


		case SV_SCROLL_SUMMON_UNDEAD:
			if (!check_self_summon(p_ptr)) break;
			s_printf("SUMMON_UNDEAD: %s\n", p_ptr->name);
			summon_override_checks = SO_IDDC | SO_PLAYER_SUMMON;
			for (k = 0; k < randint(3); k++) {
				if (summon_specific(&p_ptr->wpos, p_ptr->py, p_ptr->px, getlevel(&p_ptr->wpos), 0, SUMMON_UNDEAD, 1, 0))
					ident = TRUE;
			}
			summon_override_checks = SO_NONE;
#ifdef USE_SOUND_2010
			if (ident) sound_near_site(p_ptr->py, p_ptr->px, &p_ptr->wpos, 0, "summon", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
			break;

		case SV_SCROLL_TRAP_CREATION:
			if (trap_creation(Ind, 5, 1)) ident = TRUE;
			break;

		case SV_SCROLL_PHASE_DOOR:
			teleport_player(Ind, 10, TRUE);
			ident = TRUE;
			break;

		case SV_SCROLL_TELEPORT:
			teleport_player(Ind, 100, FALSE);
			ident = TRUE;
			break;

		case SV_SCROLL_TELEPORT_LEVEL:
			teleport_player_level(Ind, FALSE);
			ident = TRUE;
			break;

		case SV_SCROLL_WORD_OF_RECALL:
			if (!o_ptr) break;

			set_recall(Ind, rand_int(20) + 15, o_ptr);
			ident = TRUE;
			break;

		case SV_SCROLL_IDENTIFY:
			if (!o_ptr) break;

			ident = TRUE;
			if (p_ptr->current_item != -1) (void)ident_spell(Ind); /* XID */
			else { /* Manual usage */
				msg_print(Ind, "This is an identify scroll.");
				(void)ident_spell(Ind);
				*used_up = FALSE;
			}
			p_ptr->using_up_item = item;
			break;

		case SV_SCROLL_STAR_IDENTIFY:
			if (!o_ptr) break;

			ident = TRUE;
			if (p_ptr->current_item != -1) (void)identify_fully(Ind); /* XID */
			else { /* Manual usage */
				msg_print(Ind, "This is an *identify* scroll.");
				(void)identify_fully(Ind);
				*used_up = FALSE;
			}
			p_ptr->using_up_item = item;
			break;

		case SV_SCROLL_REMOVE_CURSE:
#ifndef NEW_REMOVE_CURSE
			if (remove_curse(Ind)) {
				msg_print(Ind, "\377GYou feel as if someone is watching over you.");
#else
			if (remove_curse_aux(Ind, 0x0, 0)) {
#endif
				ident = TRUE;
			}
			break;

		case SV_SCROLL_STAR_REMOVE_CURSE:
#ifndef NEW_REMOVE_CURSE
			if (remove_all_curse(Ind)) {
				msg_print(Ind, "\377GYou feel as if someone is watching over you.");
#else
			if (remove_curse_aux(Ind, 0x1, 0)) {
#endif
				ident = TRUE;
			}
			break;

		case SV_SCROLL_ENCHANT_ARMOR:
			if (!o_ptr) break;

			msg_print(Ind, "This is a Scroll of Enchant Armour.");
			ident = TRUE;
			(void)enchant_spell(Ind, 0, 0, 1, o_ptr->discount == 100 ? ENCH_STOLEN : 0);
			*used_up = FALSE;
			p_ptr->using_up_item = item;
			break;

		case SV_SCROLL_ENCHANT_WEAPON_TO_HIT:
			if (!o_ptr) break;

			msg_print(Ind, "This is a Scroll of Enchant Weapon To-Hit.");
			(void)enchant_spell(Ind, 1, 0, 0, o_ptr->discount == 100 ? ENCH_STOLEN : 0);
			*used_up = FALSE;
			p_ptr->using_up_item = item;
			ident = TRUE;
			break;

		case SV_SCROLL_ENCHANT_WEAPON_TO_DAM:
			if (!o_ptr) break;

			msg_print(Ind, "This is a Scroll of Enchant Weapon To-Dam.");
			(void)enchant_spell(Ind, 0, 1, 0, o_ptr->discount == 100 ? ENCH_STOLEN : 0);
			*used_up = FALSE;
			p_ptr->using_up_item = item;
			ident = TRUE;
			break;

		case SV_SCROLL_STAR_ENCHANT_ARMOR:
			if (!o_ptr) break;

			msg_print(Ind, "This is a Scroll of *Enchant Armour*.");
			(void)enchant_spell(Ind, 0, 0, randint(3) + 3, o_ptr->discount == 100 ? ENCH_STOLEN : 0);
			*used_up = FALSE;
			p_ptr->using_up_item = item;
			ident = TRUE;
			break;

		case SV_SCROLL_STAR_ENCHANT_WEAPON:
			if (!o_ptr) break;

			msg_print(Ind, "This is a Scroll of *Enchant Weapon*.");
			(void)enchant_spell(Ind, 1 + randint(2), 1 + randint(2), 0, o_ptr->discount == 100 ? ENCH_STOLEN : 0);
			*used_up = FALSE;
			p_ptr->using_up_item = item;
			ident = TRUE;
			break;

		case SV_SCROLL_RECHARGING:
			if (!o_ptr) break;

			msg_print(Ind, "This is a Scroll of Recharging.");
			(void)recharge(Ind, 60);
			*used_up = FALSE;
			p_ptr->using_up_item = item;
			ident = TRUE;
			break;

		case SV_SCROLL_LIGHT:
			if (lite_area(Ind, damroll(2, 8), 2)) ident = TRUE;
			//if (p_ptr->suscep_lite && !p_ptr->resist_lite)
			if (p_ptr->prace == RACE_VAMPIRE && !p_ptr->resist_lite)
				take_hit(Ind, damroll(10, 3), o_ptr ? "a Scroll of Light" : "a flash of light", 0);
				//if (p_ptr->suscep_lite && !p_ptr->resist_lite && !p_ptr->resist_blind)
			if (p_ptr->prace == RACE_VAMPIRE && !p_ptr->resist_lite && !p_ptr->resist_blind)
				(void)set_blind(Ind, p_ptr->blind + 5 + randint(10));
			break;

		case SV_SCROLL_MAPPING:
			map_area(Ind);
			ident = TRUE;
			break;

		case SV_SCROLL_DETECT_GOLD:
			if (detect_treasure(Ind, DEFAULT_RADIUS * 2)) ident = TRUE;
			break;

		case SV_SCROLL_DETECT_ITEM:
			if (detect_object(Ind, DEFAULT_RADIUS * 3)) ident = TRUE;
			break;

		case SV_SCROLL_DETECT_TRAP:
			if (detect_trap(Ind, DEFAULT_RADIUS)) ident = TRUE;
			break;

		case SV_SCROLL_DETECT_DOOR:
			if (detect_sdoor(Ind, DEFAULT_RADIUS)) ident = TRUE;
			break;

		case SV_SCROLL_DETECT_INVIS:
			if (detect_invisible(Ind)) ident = TRUE;
			break;

		case SV_SCROLL_SATISFY_HUNGER:
			//if (!p_ptr->suscep_life)
			if (p_ptr->prace != RACE_VAMPIRE && p_ptr->prace != RACE_ENT) {
				if (set_food(Ind, PY_FOOD_MAX - 1)) ident = TRUE;
			} else
				if (o_ptr) msg_print(Ind, "The scroll has no effect on you.");
			break;

		case SV_SCROLL_BLESSING:
			if (p_ptr->suscep_good || p_ptr->suscep_life) {
			//if (p_ptr->prace == RACE_VAMPIRE) {
				take_hit(Ind, damroll(5, 3), o_ptr ? "a Scroll of Blessing" : "a blessing", 0);
			} else if (p_ptr->blessed_power <= 8) {
				p_ptr->blessed_power = 8;
				if (set_blessed(Ind, randint(12) + 6)) ident = TRUE; /* removed stacking */
			}
			break;

		case SV_SCROLL_HOLY_CHANT:
			if (p_ptr->suscep_good || p_ptr->suscep_life) {
			//if (p_ptr->prace == RACE_VAMPIRE) {
				take_hit(Ind, damroll(10, 3), o_ptr ? "a Scroll of Holy Chant" : "a chant", 0);
			} else if (p_ptr->blessed_power <= 14) {
				p_ptr->blessed_power = 14;
				if (set_blessed(Ind, randint(24) + 12)) ident = TRUE; /* removed stacking */
			}
			break;

		case SV_SCROLL_HOLY_PRAYER:
			if (p_ptr->suscep_good || p_ptr->suscep_life) {
			//if (p_ptr->prace == RACE_VAMPIRE) {
				take_hit(Ind, damroll(30, 3), o_ptr ? "a Scroll of Holy Prayer" : "a holy prayer", 0);
			} else if (p_ptr->blessed_power <= 20) {
				p_ptr->blessed_power = 20;
				if (set_blessed(Ind, randint(48) + 24)) ident = TRUE; /* removed stacking */
			}
			break;

		case SV_SCROLL_MONSTER_CONFUSION:
			if (p_ptr->confusing == 0) {
				msg_print(Ind, "Your hands begin to glow.");
				ident = TRUE;
			}
			p_ptr->confusing = randint(5);
			break;

		case SV_SCROLL_PROTECTION_FROM_EVIL:
			if (p_ptr->suscep_good || p_ptr->suscep_life) {
			//if (p_ptr->prace == RACE_VAMPIRE) {
				take_hit(Ind, damroll(10, 3), o_ptr ? "a Scroll of Protection from Evil" : "evil-repelling magic", 0);
			} else {
				if (set_protevil(Ind, randint(15) + 30)) ident = TRUE; /* removed stacking */
			}
			break;

		case SV_SCROLL_RUNE_OF_PROTECTION:
			warding_glyph(Ind);
			ident = TRUE;
			break;

		case SV_SCROLL_TRAP_DOOR_DESTRUCTION:
			if (destroy_traps_doors_touch(Ind, 1)) ident = TRUE;
			break;

		case SV_SCROLL_STAR_DESTRUCTION:
			s_printf("*DESTRUCTION* (scroll) by %s\n", p_ptr->name);
			destroy_area(&p_ptr->wpos, p_ptr->py, p_ptr->px, 15, TRUE, FEAT_FLOOR, 120);
			ident = TRUE;
			break;

		case SV_SCROLL_DISPEL_UNDEAD:
			if (dispel_undead(Ind, 100 + p_ptr->lev * 8)) ident = TRUE;
			//if (p_ptr->suscep_life)
			if (p_ptr->prace == RACE_VAMPIRE)
				take_hit(Ind, damroll(30, 3), o_ptr ? "a Scroll of Dispel Undead" : "undead-dispelling magic", 0);
			break;

		case SV_SCROLL_GENOCIDE:
			if (o_ptr) msg_print(Ind, "This is a genocide scroll.");
			(void)genocide(Ind);
			ident = TRUE;
			break;

		case SV_SCROLL_OBLITERATION:
			if (o_ptr) msg_print(Ind, "This is a obliteration scroll.");
			(void)obliteration(Ind);
			ident = TRUE;
			break;

		case SV_SCROLL_ACQUIREMENT:
		{
			int obj_tmp = object_level;
			//if (!o_ptr) break;

			object_level = getlevel(&p_ptr->wpos);
			if (o_ptr->discount == 100) object_discount = 100; /* stolen? */
			s_printf("%s: ACQ_SCROLL: by player %s\n", showtime(), p_ptr->name);
			acquirement(&p_ptr->wpos, p_ptr->py, p_ptr->px, 1, TRUE, (p_ptr->wpos.wz != 0), make_resf(p_ptr));
			object_discount = 0;
			object_level = obj_tmp; /*just paranoia, dunno if needed.*/
			ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_ACQUIREMENT:
		{
			int obj_tmp = object_level;
			//if (!o_ptr) break;

			object_level = getlevel(&p_ptr->wpos);
			if (o_ptr->discount == 100) object_discount = 100; /* stolen? */
			s_printf("%s: *ACQ_SCROLL*: by player %s\n", showtime(), p_ptr->name);
			acquirement(&p_ptr->wpos, p_ptr->py, p_ptr->px, randint(2) + 1, TRUE, (p_ptr->wpos.wz != 0), make_resf(p_ptr));
			object_discount = 0;
			object_level = obj_tmp; /*just paranoia, dunno if needed.*/
			ident = TRUE;
			break;
		}

		/* New Zangband scrolls */
		case SV_SCROLL_FIRE:
			sprintf(p_ptr->attacker, " is enveloped by fire for");
			fire_ball(Ind, GF_FIRE, 0, 200, 4, p_ptr->attacker);
			/* Note: "Double" damage since it is centered on the player ... */
			if (!(p_ptr->oppose_fire || p_ptr->resist_fire || p_ptr->immune_fire))
				//take_hit(Ind, 50+randint(50)+(p_ptr->suscep_fire)?20:0, "a Scroll of Fire", 0);
				take_hit(Ind, 100 + randint(100), o_ptr ? "a Scroll of Fire" : "fire", 0);
			ident = TRUE;
			break;

		case SV_SCROLL_ICE:
			sprintf(p_ptr->attacker, " enveloped by frost for");
			fire_ball(Ind, GF_ICE, 0, 200, 4, p_ptr->attacker);
			if (!(p_ptr->oppose_cold || p_ptr->resist_cold || p_ptr->immune_cold))
				take_hit(Ind, 100 + randint(100), o_ptr ? "a Scroll of Ice" : "ice", 0);
			ident = TRUE;
			break;

		case SV_SCROLL_CHAOS:
			sprintf(p_ptr->attacker, " is enveloped by raw chaos for");
			fire_ball(Ind, GF_CHAOS, 0, 500, 4, p_ptr->attacker);
			if (!p_ptr->resist_chaos)
				take_hit(Ind, 111+randint(111), o_ptr ? "a Scroll of Chaos" : "chaos", 0);
			ident = TRUE;
			break;

		case SV_SCROLL_RUMOR:
			if (o_ptr) msg_print(Ind, "You read the scroll:");
			fortune(Ind, magik(40) ? TRUE : FALSE);
			ident = TRUE;
			break;

		case SV_SCROLL_LOTTERY:
			if (!o_ptr) break;

			do_lottery(Ind, o_ptr);
			ident = TRUE;
			break;

#if 0	// implement them whenever you feel like :)
		case SV_SCROLL_BACCARAT:
		case SV_SCROLL_BLACK_JACK:
		case SV_SCROLL_ROULETTE:
		case SV_SCROLL_SLOT_MACHINE:
		case SV_SCROLL_BACK_GAMMON:
			break;
#endif	// 0

		case SV_SCROLL_ID_ALL:
			identify_pack(Ind);
			break;

		case SV_SCROLL_VERMIN_CONTROL:
			if (do_vermin_control(Ind)) ident = TRUE;
			break;

		case SV_SCROLL_NOTHING:
			if (o_ptr) msg_print(Ind, "This scroll seems to be blank.");
			ident = TRUE;
			*keep = TRUE;
			break;

		case SV_SCROLL_CANCELLATION:
			ident = do_cancellation(Ind, 0);
			if (ident) msg_print(Ind, "You feel your backpack less worthy.");
			break;

		case SV_SCROLL_WILDERNESS_MAP:
		{
#ifndef NEW_WILDERNESS_MAP_SCROLLS
			/* Reveal a random dungeon location (C. Blue): */
			int x, y, d_tries, d_no;
			dungeon_type *d_ptr;
			if (rand_int(100) < 50) {
				x = 0;
				/* first dungeon (d_tries = 0) is always 'wildernis'
				-> ignore it by skipping to d_tries = 1: */
				for (d_tries = 1; d_tries < MAX_D_IDX; d_tries++)
					if (d_info[d_tries].name) x++;
				d_no = randint(x);
				y = 0;
				for (d_tries = 1; d_tries < MAX_D_IDX; d_tries++) {
					if (d_info[d_tries].name) y++;
					if (y == d_no) {
						d_no = d_tries;
						d_tries = MAX_D_IDX;
					}
				}

				if (!strcmp(d_info[d_no].name + d_name, "The Shores of Valinor")) break;

				for (y = 0; y < MAX_WILD_Y; y++)
				for (x = 0; x < MAX_WILD_X; x++) {
					if ((d_ptr = wild_info[y][x].tower)) {
						if (d_ptr->type == d_no) {
							if (o_ptr) msg_format(Ind, "\377sThe scroll carries an inscription: %s at %d , %d", d_info[d_no].name + d_name, x, y);
							y = MAX_WILD_Y;
							break;
						}
					}
					if ((d_ptr = wild_info[y][x].dungeon)) {
						if (d_ptr->type == d_no) {
							if (o_ptr) msg_format(Ind, "\377sThe scroll carries an inscription: %s at %d , %d", d_info[d_no].name + d_name, x, y);
							y = MAX_WILD_Y;
							break;
						}
					}
				}
			}

			if (p_ptr->wpos.wz) {
				msg_print(Ind, "You have a strange feeling of loss.");
				break;
			}
			ident = reveal_wilderness_around_player(Ind, p_ptr->wpos.wy, p_ptr->wpos.wx, 0, 3);
			//if (ident) wild_display_map(Ind);
			if (ident) msg_print(Ind, "You seem to get a feel for the place.");

#else /* new scrolls */
			/* Reveal a random 3x3 patch of the world map, and give a message if it means we discovered a dungeon - C. Blue */
			int x = rand_int(MAX_WILD_X), y = rand_int(MAX_WILD_Y);
			int xs = (x >= 1) ? x - 1 : x, xe = (x < MAX_WILD_X - 1) ? x + 1 : x;
			int ys = (y >= 1) ? y - 1 : y, ye = (y < MAX_WILD_Y - 1) ? y + 1 : y;

			wilderness_type *wild;
			dungeon_type *d_ptr;

			msg_format(Ind, "\377sYou learn the world map layout around sector \377u(%d,%d)\377s.", (xs + xe) / 2, (ys + ye) / 2); //for patch area revelation

			for (x = xs; x <= xe; x++) for (y = ys; y <= ye; y++) {
				wild = &wild_info[y][x];

 #if 0 /* this only makes sense for single (x,y) revelation, not for patch area */
				if (p_ptr->wild_map[(x + y * MAX_WILD_X) / 8] &
				    (1U << (x + y * MAX_WILD_X) % 8)) {
					msg_format(Ind, "\377sThis world map piece shows the layout at \377u(%d,%d)\377s which you already know.", x, y);
					break;
				}
 #endif

				//msg_format(Ind, "\377sYou learn the world map layout at sector \377u(%d,%d)\377s.", x, y); //for single (x,y) revelation
				p_ptr->wild_map[(x + y * MAX_WILD_X) / 8] |= (1U << ((x + y * MAX_WILD_X) % 8));

				if ((d_ptr = wild->tower) && d_ptr->type != DI_VALINOR) {
					msg_print(Ind, "\377sYou learn that there is a tower at that location, called:");
					msg_format(Ind, "\377s  '\377u%s\377s'", get_dun_name(x, y, TRUE, d_ptr, 0, TRUE));
				}
				if ((d_ptr = wild->dungeon) && d_ptr->type != DI_VALINOR) {
					msg_print(Ind, "\377sYou learn that there is a dungeon at that location, called:");
					msg_format(Ind, "\377s  '\377u%s\377s'", get_dun_name(x, y, FALSE, d_ptr, 0, TRUE));
				}
			}
#endif
			break;
		}
		case SV_SCROLL_FIREWORK:
#if 0
			if (!season_newyearseve && o_ptr) {
				msg_print(Ind, "The scroll buzzes for a moment but nothing happens.");
				*keep = TRUE;
				break;
			}
#endif
			if (o_ptr) {
				msg_print(Ind, "You release a magical firework from the scroll!");
				msg_format_near(Ind, "%s releases a magical firework from a scroll!", p_ptr->name);
			}
			cast_fireworks(&p_ptr->wpos, p_ptr->px, p_ptr->py, o_ptr->xtra1 * 7 + o_ptr->xtra2); //size, colour
#ifdef USE_SOUND_2010
			sound_vol(Ind, "fireworks_launch", "", SFX_TYPE_AMBIENT, TRUE, 50);
#endif
			ident = TRUE;
			break;
		}
	} else if (tval == TV_PARCHMENT && o_ptr) {
#if 0
		/* Maps */
		if (sval >= 200) {
			int i, n;
			char buf[80], fil[20];

			strnfmt(fil, 20, "book-%d.txt",o_ptr->sval);

			n = atoi(get_line(fil, ANGBAND_DIR_FILE, buf, -1));

			/* Parse all the fields */
			for (i = 0; i < n; i += 4) {
				/* Grab the fields */
				int x = atoi(get_line(fil, ANGBAND_DIR_FILE, buf, i + 0));
				int y = atoi(get_line(fil, ANGBAND_DIR_FILE, buf, i + 1));
				int w = atoi(get_line(fil, ANGBAND_DIR_FILE, buf, i + 2));
				int h = atoi(get_line(fil, ANGBAND_DIR_FILE, buf, i + 3));

				reveal_wilderness_around_player(y, x, h, w);
			}
		}

		/* Normal parchments */
		else
#endif
		{
			/* Get the filename */
			char path[MAX_PATH_LENGTH];
			cptr q = format("book-%d.txt", sval);

			path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_TEXT, q);
			do_cmd_check_other_prepare(Ind, path, "");

			//*used_up = FALSE;
			*keep = TRUE;
		}
	}

	return ident;
}

/*
 * NOTE: seemingly, 'used_up' flag is used in a strange way to allow
 * item specification.  'keep' flag should be used for non-consuming
 * scrolls instead.		- Jir -
 */
#define FORMAT_VALUE_WITH_COMMATA
void do_cmd_read_scroll(int Ind, int item) {
	player_type *p_ptr = Players[Ind];
	//cave_type * c_ptr;

	int	ident, lev;
	bool	used_up, keep = FALSE, flipped = FALSE;

	object_type	*o_ptr;


	/* Check some conditions */
	if (p_ptr->blind) {
		msg_print(Ind, "You can't see anything.");
		s_printf("%s EFFECT: Blind prevented scroll for %s.\n", showtime(), p_ptr->name);
		return;
	}
	if (p_ptr->confused) {
		msg_print(Ind, "You are too confused!");
		s_printf("%s EFFECT: Confusion prevented scroll for %s.\n", showtime(), p_ptr->name);
		return;
	}

	/* Restrict choices to scrolls */
	item_tester_tval = TV_SCROLL;

	/* Get the item (in the pack) */
	if (item >= 0) {
		o_ptr = &p_ptr->inventory[item];
	}
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max)
			return; /* item doesn't exist */

		o_ptr = &o_list[0 - item];
	}

	if (no_lite(Ind) && !(p_ptr->ghost && (o_ptr->tval == TV_PARCHMENT) && (o_ptr->sval == SV_PARCHMENT_DEATH)))
	{
		msg_print(Ind, "You have no light to read by.");
		s_printf("%s EFFECT: No-light prevented scroll for %s.\n", showtime(), p_ptr->name);
		return;
	}

	if( check_guard_inscription( o_ptr->note, 'r' )) {
		msg_print(Ind, "The item's inscription prevents it.");
		s_printf("%s EFFECT: Inscription prevented scroll for %s.\n", showtime(), p_ptr->name);
		return;
	};

	if (o_ptr->tval != TV_SCROLL && o_ptr->tval != TV_PARCHMENT) {
//(may happen on death, from macro spam)		msg_print(Ind, "SERVER ERROR: Tried to read non-scroll!");
		return;
	}

	if (!can_use_verbose(Ind, o_ptr)) return;

#ifdef PLAYER_STORES
	/* Reading a cheque gives the money and costs no time. */
	if (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_CHEQUE) {
		u32b value = ps_get_cheque_value(o_ptr);
 #ifdef FORMAT_VALUE_WITH_COMMATA
		char val[MAX_CHARS], val2[MAX_CHARS], *d, *d2, *dm;
 #endif

		/* hack: prevent s32b overflow */
		if (!gain_au(Ind, value, FALSE, FALSE)) return;

		break_cloaking(Ind, 4);
		break_shadow_running(Ind);
		stop_precision(Ind);
		stop_shooting_till_kill(Ind);

s_printf("PLAYER_STORE_CASH: %s +%d (%s).\n", p_ptr->name, value, o_ptr->note ? quark_str(o_ptr->note) : "");
 #ifndef FORMAT_VALUE_WITH_COMMATA
		msg_format(Ind, "\375\377sYou acquire \377y%d\377s gold pieces.", value);
 #else
		sprintf(val, "%d", value);
		val2[0] = 0;
		d = val;
		dm = d + strlen(val);
		d2 = val2;
		while (*d) {
			if (!((dm - d) % 3) && d != val) {
				*d2 = ',';
				d2++;
			}
			*d2 = *d;
			d++;
			d2++;
		}
		*d2 = 0;
		msg_format(Ind, "\375\377sYou acquire \377y%s\377s gold pieces.", val2);
 #endif

		p_ptr->notice |= (PN_COMBINE | PN_REORDER);
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

		if (item >= 0) { /* Destroy scroll in the pack */
			inven_item_increase(Ind, item, -1);
			inven_item_describe(Ind, item);
			inven_item_optimize(Ind, item);
		} else { /* Destroy scroll on the floor */
			floor_item_increase(0 - item, -1);
			floor_item_describe(0 - item);
			floor_item_optimize(0 - item);
		}
		return;
	}
#endif

#if 0
	/* unbelievers need some more disadvantage, but this might be too much */
	antichance = p_ptr->antimagic / 4;
	if (antichance > ANTIMAGIC_CAP) antichance = ANTIMAGIC_CAP;/* AM cap */
	/* Got disrupted ? */
	if (magik(antichance)) {
#ifdef USE_SOUND_2010
		sound(Ind, "am_field", NULL, SFX_TYPE_MISC, FALSE);
#endif
		msg_print(Ind, "Your anti-magic field disrupts the scroll.");
		return;
	}
#endif

	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	/* Not identified yet */
	ident = FALSE;

	/* Object level */
	lev = k_info[o_ptr->k_idx].level;

	process_hooks(HOOK_READ, "d", Ind);

	/* Assume the scroll will get used up */
	used_up = TRUE;

#ifdef USE_SOUND_2010
	if (o_ptr->sval != SV_SCROLL_FIREWORK) /* these just combust and cause 'launch' sfx instead */
	sound(Ind, "read_scroll", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

	ident = read_scroll(Ind, o_ptr->tval, o_ptr->sval, o_ptr, item, &used_up, &keep);

	break_cloaking(Ind, 4);
	break_shadow_running(Ind);
	stop_precision(Ind);
#ifdef CONTINUE_FTK
	stop_shooting_till_kill(Ind);
#endif

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* An identification was made */
	if (ident && !object_aware_p(Ind, o_ptr)) {
		flipped = object_aware(Ind, o_ptr);
		if (!(p_ptr->mode & MODE_PVP)) gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* The item was tried */
	object_tried(Ind, o_ptr, flipped);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Hack -- really allow certain scrolls to be "preserved" */
	if (keep) return;

	if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1);
	questitem_d(o_ptr, 1);

	/* Destroy a scroll in the pack */
	if (item >= 0) {
		inven_item_increase(Ind, item, -1);

		/* Hack -- allow certain scrolls to be "preserved" */
		if (!used_up) return;

		inven_item_describe(Ind, item);
		inven_item_optimize(Ind, item);
	}
	/* Destroy a scroll on the floor */
	else {
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}
}



bool use_staff(int Ind, int sval, int rad, bool msg, bool *use_charge) {
	player_type *p_ptr = Players[Ind];
	bool ident = FALSE;
	int k;

	/* Analyze the staff */
	switch (sval) {
	case SV_STAFF_DARKNESS:
		if (unlite_area(Ind, 10, 3)) ident = TRUE;
		if (!p_ptr->resist_blind && !p_ptr->resist_dark) {
			if (set_blind(Ind, p_ptr->blind + 3 + randint(5) - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 3))) ident = TRUE;
		}
		break;

	case SV_STAFF_SLOWNESS:
		if (set_slow(Ind, p_ptr->slow + randint(30) + 15 - get_skill_scale(p_ptr, SKILL_DEVICE, 15))) ident = TRUE;
		break;

	case SV_STAFF_HASTE_MONSTERS:
		if (speed_monsters(Ind)) ident = TRUE;
		break;

	case SV_STAFF_SUMMONING:
		if (!check_self_summon(p_ptr)) break;
		//logfile spam- s_printf("SUMMON_SPECIFIC: %s\n", p_ptr->name);
		summon_override_checks = SO_IDDC | SO_PLAYER_SUMMON;
		for (k = 0; k < randint(4); k++) {
			if (summon_specific(&p_ptr->wpos, p_ptr->py, p_ptr->px, getlevel(&p_ptr->wpos), 0, SUMMON_ALL_U98, 1, 0))
				ident = TRUE;
		}
#ifdef USE_SOUND_2010
		if (ident) sound_near_site(p_ptr->py, p_ptr->px, &p_ptr->wpos, 0, "summon", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
		summon_override_checks = SO_NONE;
		break;

	case SV_STAFF_TELEPORTATION:
		if (teleport_player(Ind, 100 + get_skill_scale(p_ptr, SKILL_DEVICE, 100), FALSE))
			msg_format_near(Ind, msg ? "%s teleports away!" : "%s is teleported away!", p_ptr->name);
		ident = TRUE;
		break;

	case SV_STAFF_IDENTIFY:
		if (!ident_spell(Ind)) *use_charge = FALSE;
		ident = TRUE;
		break;

	case SV_STAFF_REMOVE_CURSE:
#ifndef NEW_REMOVE_CURSE
		if (remove_curse(Ind)) {
			if (!p_ptr->blind) {
				if (msg) msg_print(Ind, "The staff glows blue for a moment...");
				msg_print(Ind, "There is a blue glow for a moment...");
			}
#else
		if (remove_curse_aux(Ind, 0x0, 0)) {
#endif
			ident = TRUE;
		}
		break;

	case SV_STAFF_STARLITE:
		if (!p_ptr->blind) {
			if (msg) msg_print(Ind, "The end of the staff glows brightly...");
			else msg_print(Ind, "A bright light appears...");
		}
		lite_room(Ind, &p_ptr->wpos, p_ptr->py, p_ptr->px);
		for (k = 0; k < 8; k++) lite_line(Ind, ddd[k], damroll(5, 8) + get_skill_scale(p_ptr, SKILL_DEVICE, 100), TRUE);
		if (p_ptr->suscep_lite) take_hit(Ind, damroll((p_ptr->resist_lite ? 10: 30), 3), msg ? "a staff of starlight" : "starlight", 0);
		if (p_ptr->suscep_lite && !p_ptr->resist_lite && !p_ptr->resist_blind) (void)set_blind(Ind, p_ptr->blind + 5 + randint(10));
		ident = TRUE;
		break;

	case SV_STAFF_LITE:
		if (msg) msg_format_near(Ind, "%s calls light.", p_ptr->name);
		else msg_print_near(Ind, "A light appears.");
		if (lite_area(Ind, damroll(2 + get_skill_scale(p_ptr, SKILL_DEVICE, 10), 8), 2)) ident = TRUE;
		if (p_ptr->suscep_lite && !p_ptr->resist_lite) take_hit(Ind, damroll(20, 3), msg ? "a staff of Light" : "light", 0);
		if (p_ptr->suscep_lite && !p_ptr->resist_lite && !p_ptr->resist_blind) (void)set_blind(Ind, p_ptr->blind + 5 + randint(10));
		break;

	case SV_STAFF_MAPPING:
		map_area(Ind);
		ident = TRUE;
		break;

	case SV_STAFF_DETECT_GOLD:
		if (detect_treasure(Ind, rad * 2)) ident = TRUE;
		break;

	case SV_STAFF_DETECT_ITEM:
		if (detect_object(Ind, rad * 3)) ident = TRUE;
		break;

	case SV_STAFF_DETECT_TRAP:
		if (detect_trap(Ind, rad)) ident = TRUE;
		break;

	case SV_STAFF_DETECT_DOOR:
		if (detect_sdoor(Ind, rad)) ident = TRUE;
		break;

	case SV_STAFF_DETECT_INVIS:
		if (detect_invisible(Ind)) ident = TRUE;
		break;

	case SV_STAFF_DETECT_EVIL:
		if (detect_evil(Ind)) ident = TRUE;
		break;

	case SV_STAFF_CURE_SERIOUS:
//		if (hp_player(Ind, randint(8 + get_skill_scale(p_ptr, SKILL_DEVICE, 10)))) ident = TRUE;
		/* Turned it into 'Cure Serious Wounds' - C. Blue */
		if (set_blind(Ind, 0)) ident = TRUE;
		if (set_confused(Ind, 0)) ident = TRUE;
		if (set_cut(Ind, (p_ptr->cut / 2) - 50, p_ptr->cut_attacker)) ident = TRUE;
		if (hp_player(Ind, damroll(6 + get_skill_scale(p_ptr, SKILL_DEVICE, 9), 8))) ident = TRUE;
		break;

	case SV_STAFF_CURING:
		if (set_image(Ind, 0)) ident = TRUE;
		if (set_blind(Ind, 0)) ident = TRUE;
		if (set_poisoned(Ind, 0, 0)) ident = TRUE;
		if (set_confused(Ind, 0)) ident = TRUE;
		if (set_stun(Ind, 0)) ident = TRUE;
		if (set_cut(Ind, 0, 0)) ident = TRUE;
		if (p_ptr->food >= PY_FOOD_MAX) /* ungorge */
			if (set_food(Ind, PY_FOOD_MAX - 1)) ident = TRUE;
		break;

	case SV_STAFF_HEALING:
		if (hp_player(Ind, 250 + get_skill_scale(p_ptr, SKILL_DEVICE, 150))) ident = TRUE;
		if (set_stun(Ind, 0)) ident = TRUE;
		if (set_cut(Ind, 0, 0)) ident = TRUE;
		break;

	case SV_STAFF_THE_MAGI:
		if (do_res_stat(Ind, A_INT)) ident = TRUE;
		if (p_ptr->csp < p_ptr->msp
#ifdef MARTYR_NO_MANA
		    && !p_ptr->martyr
#endif
		    ) {
			p_ptr->csp += 500;
			if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;
			p_ptr->csp_frac = 0;
			ident = TRUE;
			msg_print(Ind, "You feel your head clearing.");
			p_ptr->redraw |= (PR_MANA);
			p_ptr->window |= (PW_PLAYER);
		}
		break;

	case SV_STAFF_SLEEP_MONSTERS:
		if (sleep_monsters(Ind, 10 + p_ptr->lev + get_skill_scale(p_ptr, SKILL_DEVICE, 50))) ident = TRUE;
		break;

	case SV_STAFF_SLOW_MONSTERS:
		if (slow_monsters(Ind, 10 + p_ptr->lev + get_skill_scale(p_ptr, SKILL_DEVICE, 50))) ident = TRUE;
		break;

	case SV_STAFF_SPEED:
		if (set_fast(Ind, randint(30) + 15 + get_skill_scale(p_ptr, SKILL_DEVICE, 10), 10)) ident = TRUE; /* removed stacking */
		break;

	case SV_STAFF_PROBING:
		probing(Ind);
		ident = TRUE;
		break;

	case SV_STAFF_DISPEL_EVIL:
		if (dispel_evil(Ind, 100 + get_skill_scale(p_ptr, SKILL_DEVICE, 300) + p_ptr->lev * 2)) ident = TRUE;
		if (p_ptr->suscep_good) {
			take_hit(Ind, damroll(30, 3), msg ? "a staff of dispel evil" : "evil-dispelling magic", 0);
			ident = TRUE;
		}
		break;

	case SV_STAFF_POWER:
		if (dispel_monsters(Ind, 200 + get_skill_scale(p_ptr, SKILL_DEVICE, 300))) ident = TRUE;
		break;

	case SV_STAFF_HOLINESS:
		if (dispel_evil(Ind, 200 + get_skill_scale(p_ptr, SKILL_DEVICE, 300))) ident = TRUE;
		if (p_ptr->suscep_good || p_ptr->suscep_life) {
			take_hit(Ind, damroll(50, 3), msg ? "a staff of holiness" : "holy aura", 0);
			ident = TRUE;
		} else {
			k = get_skill_scale(p_ptr, SKILL_DEVICE, 25);
			if (set_protevil(Ind, randint(15) + 30 + k)) ident = TRUE; /* removed stacking */
			if (set_poisoned(Ind, 0, 0)) ident = TRUE;
			if (set_afraid(Ind, 0)) ident = TRUE;
			if (hp_player(Ind, 50)) ident = TRUE;
			if (set_stun(Ind, 0)) ident = TRUE;
			if (set_cut(Ind, 0, 0)) ident = TRUE;
		}
		break;

	case SV_STAFF_GENOCIDE:
		(void)genocide(Ind);
		ident = TRUE;
		break;

	case SV_STAFF_EARTHQUAKES:
		if (msg) msg_format_near(Ind, "%s causes the ground to shake!", p_ptr->name);
		else msg_print_near(Ind, "The ground starts to shake!");
		earthquake(&p_ptr->wpos, p_ptr->py, p_ptr->px, 10);
		ident = TRUE;
		break;

	case SV_STAFF_DESTRUCTION:
		s_printf("*DESTRUCTION* (staff) by %s\n", p_ptr->name);
		if (msg) msg_format_near(Ind, "%s unleashes great power!", p_ptr->name);
		else msg_print_near(Ind, "Great power is unleashed!");
		destroy_area(&p_ptr->wpos, p_ptr->py, p_ptr->px, 15, TRUE, FEAT_FLOOR, 120);
		ident = TRUE;
		break;

	case SV_STAFF_STAR_IDENTIFY:
		if (!identify_fully(Ind)) *use_charge = FALSE;
		ident = TRUE;
		break;
	}

	return ident;
}

/*
 * Use a staff.			-RAK-
 *
 * One charge of one staff disappears.
 *
 * Hack -- staffs of identify can be "cancelled".
 */
void do_cmd_use_staff(int Ind, int item) {
	player_type *p_ptr = Players[Ind];
	int lev, ident, rad = DEFAULT_RADIUS_DEV(p_ptr);
	object_type *o_ptr;

	/* Hack -- let staffs of identify get aborted */
	bool use_charge = TRUE, flipped = FALSE;

#ifdef ENABLE_XID_MDEV
 #ifdef XID_REPEAT
	bool rep = p_ptr->command_rep_active
	    && p_ptr->current_item != -1; //extra sanity check, superfluous?
	p_ptr->command_rep = 0;
	p_ptr->command_rep_active = FALSE;
 #endif
#endif

	/* Break goi/manashield */
#if 0
	if (p_ptr->invuln)
		set_invuln(Ind, 0);
	if (p_ptr->tim_manashield)
		set_tim_manashield(Ind, 0);
	if (p_ptr->tim_wraith)
		set_tim_wraith(Ind, 0);
#endif	// 0


#if 1
	if (p_ptr->anti_magic) {
		msg_format(Ind, "\377%cYour anti-magic shell disrupts your attempt.", COLOUR_AM_OWN);
		return;
	}
#endif
	if (get_skill(p_ptr, SKILL_ANTIMAGIC)) {
		msg_format(Ind, "\377%cYou don't believe in magic.", COLOUR_AM_OWN);
		return;
	}
	if (magik((p_ptr->antimagic * 8) / 5)) {
#ifdef USE_SOUND_2010
		sound(Ind, "am_field", NULL, SFX_TYPE_MISC, FALSE);
#endif
		msg_format(Ind, "\377%cYour anti-magic field disrupts your attempt.", COLOUR_AM_OWN);
		return;
	}

	/* Restrict choices to wands */
	item_tester_tval = TV_STAFF;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &p_ptr->inventory[item];
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max) return; /* item doesn't exist */
		o_ptr = &o_list[0 - item];
	}

	if( check_guard_inscription( o_ptr->note, 'u' )) {
		msg_print(Ind, "The item's inscription prevents it.");
		return;
	};

	if (o_ptr->tval != TV_STAFF) {
//(may happen on death, from macro spam)		msg_print(Ind, "SERVER ERROR: Tried to use non-staff!");
		return;
	}

	if (!can_use_verbose(Ind, o_ptr)) return;

	/* Mega-Hack -- refuse to use a pile from the ground */
	if ((item < 0) && (o_ptr->number > 1)) {
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

	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	/* Not identified yet */
	ident = FALSE;

	if (!activate_magic_device(Ind, o_ptr)) {
		msg_format(Ind, "\377%cYou failed to use the staff properly." , COLOUR_MD_FAIL);
#ifdef ENABLE_XID_MDEV
 #ifdef XID_REPEAT
		/* hack: repeat ID-spell attempt until item is successfully identified */
		if (rep && !object_known_p(Ind, &p_ptr->inventory[p_ptr->current_item])) {
			sockbuf_t *conn_q = get_conn_q(Ind);

			p_ptr->command_rep = PKT_USE;
			p_ptr->command_rep_active = TRUE;
			Packet_printf(conn_q, "%c%hd", PKT_USE, item);
		} else p_ptr->current_item = -1;
 #endif
#endif
		return;
	}

	/* Notice empty staffs */
	if (o_ptr->pval <= 0) {
		msg_print(Ind, "The staff has no charges left.");
		o_ptr->ident |= ID_EMPTY;
		note_toggle_empty(o_ptr, TRUE);

#if 0
		if (!o_ptr->note) o_ptr->note = quark_add("empty");
		else if (strcmp(quark_str(o_ptr->note), "empty")) { // (*) check 1 of 2 (exact match)
			if (!strstr(quark_str(o_ptr->note), "empty-")) { // (*) check 2 of 2 (partial match)
				char insc[ONAME_LEN + 6];

				strcpy(insc, "empty-");
				strcat(insc, quark_str(o_ptr->note));
				insc[ONAME_LEN] = 0;
				o_ptr->note = quark_add(insc);
			}
		}
#endif

		/* Redraw */
		o_ptr->changed = !o_ptr->changed;
		p_ptr->window |= (PW_INVEN);

		return;
	}

	ident = use_staff(Ind, o_ptr->sval, rad, TRUE, &use_charge);
#ifdef ENABLE_XID_MDEV
 #ifdef XID_REPEAT
	if (rep) p_ptr->current_item = -1;
 #endif
#endif

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Extract the item level */
	lev = k_info[o_ptr->k_idx].level;

	/* An identification was made */
	if (ident && !object_aware_p(Ind, o_ptr)) {
		flipped = object_aware(Ind, o_ptr);
		if (!(p_ptr->mode & MODE_PVP)) gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Tried the item */
	object_tried(Ind, o_ptr, flipped);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	break_cloaking(Ind, 4);
	break_shadow_running(Ind);
	stop_precision(Ind);
#ifdef CONTINUE_FTK
	stop_shooting_till_kill(Ind);
#endif

	/* Hack -- some uses are "free" */
	if (!use_charge) return;


	/* Use a single charge */
	o_ptr->pval--;

#ifndef NEW_MDEV_STACKING
	/* XXX Hack -- unstack if necessary */
	if ((item >= 0) && (o_ptr->number > 1)) {
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
#endif

	/* Describe charges in the pack */
	if (item >= 0)
		inven_item_charges(Ind, item);
	/* Describe charges on the floor */
	else
		floor_item_charges(0 - item);
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
void do_cmd_aim_wand(int Ind, int item, int dir) {
	player_type *p_ptr = Players[Ind];
	int lev, ident, sval;
	object_type *o_ptr;
	bool flipped = FALSE;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &p_ptr->inventory[item];
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max) {
			p_ptr->shooting_till_kill = FALSE;
			return; /* item doesn't exist */
		}
		o_ptr = &o_list[0 - item];
	}
	if (o_ptr->tval != TV_WAND) {
//(may happen on death, from macro spam)		msg_print(Ind, "SERVER ERROR: Tried to use non-wand!");
		p_ptr->shooting_till_kill = FALSE;
		return;
	}

	/* Handle FTK targetting mode */
	if (p_ptr->shooting_till_kill) { /* we were shooting till kill last turn? */
		p_ptr->shooting_till_kill = FALSE; /* well, gotta re-test for another success now.. */
		p_ptr->shooty_till_kill = TRUE; /* so for now we are just ATTEMPTING to shoot till kill (assumed we have a monster for target) */
	}
	if (p_ptr->shooty_till_kill) {
		if (dir == 5 && check_guard_inscription(o_ptr->note, 'K')) {
			/* We lost our target? (monster dead?) */
			if (target_okay(Ind)) {
				/* To continue shooting_till_kill, check if spell requires clean LOS to target
				   with no other monsters in the way, so we won't wake up more monsters accidentally. */
 #ifndef PY_PROJ_WALL
				if (!projectable_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE)) return;
 #else
				if (!projectable_wall_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE)) return;
 #endif
			} else p_ptr->shooty_till_kill = FALSE;
		} else p_ptr->shooty_till_kill = FALSE;
	}

	/* Break goi/manashield */
#if 0
	if (p_ptr->invuln) set_invuln(Ind, 0);
	if (p_ptr->tim_manashield) set_tim_manashield(Ind, 0);
	if (p_ptr->tim_wraith) set_tim_wraith(Ind, 0);
#endif	// 0

#if 1	// anti_magic is not antimagic :)
	if (p_ptr->anti_magic) {
		msg_format(Ind, "\377%cYour anti-magic shell disrupts your attempt.", COLOUR_AM_OWN);
		return;
	}
#endif	// 0
	if (get_skill(p_ptr, SKILL_ANTIMAGIC)) {
		msg_format(Ind, "\377%cYou don't believe in magic.", COLOUR_AM_OWN);
		return;
	}
	/* Restrict choices to wands */
	item_tester_tval = TV_WAND;

	if( check_guard_inscription( o_ptr->note, 'a' )) {
		msg_print(Ind, "The item's inscription prevents it.");
		return;
	};

	if (!can_use_verbose(Ind, o_ptr)) return;

	/* Mega-Hack -- refuse to aim a pile from the ground */
	if ((item < 0) && (o_ptr->number > 1)) {
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


	/* New '+' feat in 4.4.6.2 */
	if (dir == 11) {
		get_aim_dir(Ind);
		p_ptr->current_wand = item;
		return;
	}


	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	break_cloaking(Ind, 3);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);


	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	/* Not identified yet */
	ident = FALSE;

	if (magik((p_ptr->antimagic * 8) / 5)) {
#ifdef USE_SOUND_2010
		sound(Ind, "am_field", NULL, SFX_TYPE_MISC, FALSE);
#endif
		msg_format(Ind, "\377%cYour anti-magic field disrupts your attempt.", COLOUR_AM_OWN);

		//don't cancel FTK since this failure was just a chance thing
		if (p_ptr->shooty_till_kill) {
			p_ptr->shooting_till_kill = TRUE;
			p_ptr->shoot_till_kill_wand = item;
			/* disable other ftk types */
			p_ptr->shoot_till_kill_mimic = 0;
			p_ptr->shoot_till_kill_spell = 0;
			p_ptr->shoot_till_kill_rcraft = FALSE;
			p_ptr->shoot_till_kill_rod = 0;
		}
		return;
	}

	if (!activate_magic_device(Ind, o_ptr)) {
		msg_format(Ind, "\377%cYou failed to use the wand properly." , COLOUR_MD_FAIL);

		//don't cancel FTK since this failure was just a chance thing
		if (p_ptr->shooty_till_kill) {
			p_ptr->shooting_till_kill = TRUE;
			p_ptr->shoot_till_kill_wand = item;
			/* disable other ftk types */
			p_ptr->shoot_till_kill_mimic = 0;
			p_ptr->shoot_till_kill_spell = 0;
			p_ptr->shoot_till_kill_rcraft = FALSE;
			p_ptr->shoot_till_kill_rod = 0;
		}
		return;
	}

	/* The wand is already empty! */
	if (o_ptr->pval <= 0) {
		msg_print(Ind, "The wand has no charges left.");
		o_ptr->ident |= ID_EMPTY;
		note_toggle_empty(o_ptr, TRUE);

#if 0
		if (!o_ptr->note) o_ptr->note = quark_add("empty");
		else if (strcmp(quark_str(o_ptr->note), "empty")) { // (*) check 1 of 2 (exact match)
			if (!strstr(quark_str(o_ptr->note), "empty-")) { // (*) check 2 of 2 (partial match)
				char insc[ONAME_LEN + 6];

				strcpy(insc, "empty-");
				strcat(insc, quark_str(o_ptr->note));
				insc[ONAME_LEN] = 0;
				o_ptr->note = quark_add(insc);
			}
		}
#endif

		/* Redraw */
		o_ptr->changed = !o_ptr->changed;
		p_ptr->window |= (PW_INVEN);

		return;
	}



	/* XXX Hack -- Extract the "sval" effect */
	sval = o_ptr->sval;

	/* XXX Hack -- Wand of wonder can do anything before it */
	if (sval == SV_WAND_WONDER) {
		sval = rand_int(SV_WAND_WONDER);
		/* limit wand of wonder damage in Highlander Tournament */
		if (!p_ptr->wpos.wx && !p_ptr->wpos.wy && !p_ptr->wpos.wz && sector00separation) {
			/* no high wand effects for cheap mass zapping in PvP */
			sval = rand_int(SV_WAND_WONDER - 8 - 2);
			if (sval >= SV_WAND_DRAIN_LIFE) sval += 2;
		}
	}

	/* Analyze the wand */
	switch (sval % 1000) {
	case SV_WAND_HEAL_MONSTER:
		if (heal_monster(Ind, dir)) ident = TRUE;
		break;

	case SV_WAND_HASTE_MONSTER:
		if (speed_monster(Ind, dir)) ident = TRUE;
		break;

	case SV_WAND_CLONE_MONSTER:
		if (clone_monster(Ind, dir)) ident = TRUE;
		break;

	case SV_WAND_TELEPORT_AWAY:
		if (teleport_monster(Ind, dir)) ident = TRUE;
		break;

	case SV_WAND_DISARMING:
		if (disarm_trap_door(Ind, dir)) ident = TRUE;
		break;

	case SV_WAND_TRAP_DOOR_DEST:
		if (destroy_trap_door(Ind, dir)) ident = TRUE;
		break;

	case SV_WAND_STONE_TO_MUD:
		if (wall_to_mud(Ind, dir)) ident = TRUE;
		break;

	case SV_WAND_LITE:
		msg_print(Ind, "A line of blue shimmering light appears.");
		lite_line(Ind, dir, damroll(6, 8) + get_skill_scale(p_ptr, SKILL_DEVICE, 50), FALSE);
		ident = TRUE;
		break;

	case SV_WAND_SLEEP_MONSTER:
		if (sleep_monster(Ind, dir, 10 + p_ptr->lev + get_skill_scale(p_ptr, SKILL_DEVICE, 50))) ident = TRUE;
		break;

	case SV_WAND_SLOW_MONSTER:
		if (slow_monster(Ind, dir, 10 + p_ptr->lev + get_skill_scale(p_ptr, SKILL_DEVICE, 50))) ident = TRUE;
		break;

	case SV_WAND_CONFUSE_MONSTER:
		if (confuse_monster(Ind, dir, 10 + p_ptr->lev + get_skill_scale(p_ptr, SKILL_DEVICE, 50))) ident = TRUE;
		break;

	case SV_WAND_FEAR_MONSTER:
		if (fear_monster(Ind, dir, 10 + p_ptr->lev + get_skill_scale(p_ptr, SKILL_DEVICE, 50))) ident = TRUE;
		break;

	case SV_WAND_DRAIN_LIFE:
		if (drain_life(Ind, dir, 10 + get_skill_scale(p_ptr, SKILL_DEVICE, 10))) {
			ident = TRUE;
			hp_player(Ind, p_ptr->ret_dam / 4);
			p_ptr->ret_dam = 0;
		}
		break;

	case SV_WAND_POLYMORPH:
		if (poly_monster(Ind, dir)) ident = TRUE;
		break;

	case SV_WAND_STINKING_CLOUD:
		msg_format_near(Ind, "%s fires a stinking cloud.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a stinking cloud for");
//		fire_ball(Ind, GF_POIS, dir, 12 + get_skill_scale(p_ptr, SKILL_DEVICE, 50), 2, p_ptr->attacker);
		fire_cloud(Ind, GF_POIS, dir, 4 + get_skill_scale(p_ptr, SKILL_DEVICE, 17), 2, 4, 9, p_ptr->attacker);
		ident = TRUE;
		break;

	case SV_WAND_MAGIC_MISSILE:
		msg_format_near(Ind, "%s fires a magic missile.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a magic missile for");
		fire_bolt_or_beam(Ind, 20, GF_MISSILE, dir, damroll(2 + get_skill_scale(p_ptr, SKILL_DEVICE, 23), 6), p_ptr->attacker);
		ident = TRUE;
		break;

	case SV_WAND_ACID_BOLT:
		msg_format_near(Ind, "%s fires an acid bolt.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires an acid bolt for");
		fire_bolt_or_beam(Ind, 20, GF_ACID, dir, damroll(6 + get_skill_scale(p_ptr, SKILL_DEVICE, 45), 8), p_ptr->attacker);
		ident = TRUE;
		break;

	case SV_WAND_ELEC_BOLT:
		msg_format_near(Ind, "%s fires a lightning bolt.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a lightning bolt for");
		fire_bolt_or_beam(Ind, 20, GF_ELEC, dir, damroll(4 + get_skill_scale(p_ptr, SKILL_DEVICE, 45), 8), p_ptr->attacker);
		ident = TRUE;
		break;

	case SV_WAND_FIRE_BOLT:
		msg_format_near(Ind, "%s fires a fire bolt.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a fire bolt for");
		fire_bolt_or_beam(Ind, 20, GF_FIRE, dir, damroll(7 + get_skill_scale(p_ptr, SKILL_DEVICE, 45), 8), p_ptr->attacker);
		ident = TRUE;
		break;

	case SV_WAND_COLD_BOLT:
		msg_format_near(Ind, "%s fires a frost bolt.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a frost bolt for");
		fire_bolt_or_beam(Ind, 20, GF_COLD, dir, damroll(5 + get_skill_scale(p_ptr, SKILL_DEVICE, 45), 8), p_ptr->attacker);
		ident = TRUE;
		break;

	case SV_WAND_ACID_BALL:
		msg_format_near(Ind, "%s fires a ball of acid.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a ball of acid for");
		fire_ball(Ind, GF_ACID, dir, 60 + get_skill_scale(p_ptr, SKILL_DEVICE, 330), 2, p_ptr->attacker);
		ident = TRUE;
		break;

	case SV_WAND_ELEC_BALL:
		msg_format_near(Ind, "%s fires a ball of electricity.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a ball of electricity for");
		fire_ball(Ind, GF_ELEC, dir, 40 + get_skill_scale(p_ptr, SKILL_DEVICE, 330), 2, p_ptr->attacker);
		ident = TRUE;
		break;

	case SV_WAND_FIRE_BALL:
		msg_format_near(Ind, "%s fires a fire ball.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a fire ball for");
		fire_ball(Ind, GF_FIRE, dir, 70 + get_skill_scale(p_ptr, SKILL_DEVICE, 330), 2, p_ptr->attacker);
		ident = TRUE;
		break;

	case SV_WAND_COLD_BALL:
		msg_format_near(Ind, "%s fires a frost ball.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a frost ball for");
		fire_ball(Ind, GF_COLD, dir, 50 + get_skill_scale(p_ptr, SKILL_DEVICE, 330), 2, p_ptr->attacker);
		ident = TRUE;
		break;

	case SV_WAND_WONDER:
		msg_print(Ind, "SERVER ERROR: Oops.  Wand of wonder activated.");
		break;

	case SV_WAND_DRAGON_FIRE:
		msg_format_near(Ind, "%s shoots dragonfire!", p_ptr->name);
		sprintf(p_ptr->attacker, " shoots dragonfire for");
		fire_ball(Ind, GF_FIRE, dir, 300 + get_skill_scale(p_ptr, SKILL_DEVICE, 300), 3, p_ptr->attacker);
		ident = TRUE;
		break;

	case SV_WAND_DRAGON_COLD:
		msg_format_near(Ind, "%s shoots dragonfrost!", p_ptr->name);
		sprintf(p_ptr->attacker, " shoots dragonfrost for");
		fire_ball(Ind, GF_COLD, dir, 260 + get_skill_scale(p_ptr, SKILL_DEVICE, 260), 3, p_ptr->attacker);
		ident = TRUE;
		break;

	case SV_WAND_DRAGON_BREATH:
		switch (randint(5)) {
		case 1:
			msg_format_near(Ind, "%s shoots dragon acid!", p_ptr->name);
			sprintf(p_ptr->attacker, " shoots dragon acid for");
			fire_ball(Ind, GF_ACID, dir, 400 + get_skill_scale(p_ptr, SKILL_DEVICE, 300), 3, p_ptr->attacker);
			break;
		case 2:
			msg_format_near(Ind, "%s shoots dragon lightning!", p_ptr->name);
			sprintf(p_ptr->attacker, " shoots dragon lightning for");
			fire_ball(Ind, GF_ELEC, dir, 320 + get_skill_scale(p_ptr, SKILL_DEVICE, 300), 3, p_ptr->attacker);
			break;
		case 3:
			msg_format_near(Ind, "%s shoots dragon fire!", p_ptr->name);
			sprintf(p_ptr->attacker, " shoots dragon fire for");
			fire_ball(Ind, GF_FIRE, dir, 400 + get_skill_scale(p_ptr, SKILL_DEVICE, 300), 3, p_ptr->attacker);
			break;
		case 4:
			msg_format_near(Ind, "%s shoots dragon frost!", p_ptr->name);
			sprintf(p_ptr->attacker, " shoots dragon frost for");
			fire_ball(Ind, GF_COLD, dir, 320 + get_skill_scale(p_ptr, SKILL_DEVICE, 300), 3, p_ptr->attacker);
			break;
		default:
			msg_format_near(Ind, "%s shoots dragon poison!", p_ptr->name);
			sprintf(p_ptr->attacker, " shoots dragon poison for");
			fire_ball(Ind, GF_POIS, dir, 240 + get_skill_scale(p_ptr, SKILL_DEVICE, 300), 3, p_ptr->attacker);
			break;
		}
		ident = TRUE;
		break;

	case SV_WAND_ANNIHILATION:
		if (annihilate(Ind, dir, 10 + get_skill_scale(p_ptr, SKILL_DEVICE, 10)))
			ident = TRUE;
		break;

	/* Additions from PernAngband	- Jir - */
	case SV_WAND_ROCKETS:
		msg_print(Ind, "You launch a rocket!");
		sprintf(p_ptr->attacker, " launches a rocket for");
		fire_ball(Ind, GF_ROCKET, dir, 600 + (randint(100) + get_skill_scale(p_ptr, SKILL_DEVICE, 600)), 2, p_ptr->attacker);
		ident = TRUE;
		break;
#if 0
	/* Hope we can port this someday.. */
	case SV_WAND_CHARM_MONSTER:
		if (charm_monster(dir, 45))
			ident = TRUE;
		break;
#endif	// 0

	case SV_WAND_WALL_CREATION:
		project_hook(Ind, GF_STONE_WALL, dir, 1, PROJECT_NORF | PROJECT_BEAM | PROJECT_KILL | PROJECT_GRID | PROJECT_NODO | PROJECT_NODF, "");
		ident = TRUE;
		break;

	/* TomeNET addition */
	case SV_WAND_TELEPORT_TO:
		ident = project_hook(Ind, GF_TELE_TO, dir, 1, PROJECT_STOP | PROJECT_KILL, "");
		break;
	}


	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Extract the item level */
	lev = k_info[o_ptr->k_idx].level;

	/* Apply identification */
	if (ident && !object_aware_p(Ind, o_ptr)) {
		flipped = object_aware(Ind, o_ptr);
		if (!(p_ptr->mode & MODE_PVP)) gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Mark it as tried */
	object_tried(Ind, o_ptr, flipped);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Use a single charge */
	o_ptr->pval--;

	/* Describe the charges in the pack */
	if (item >= 0)
		inven_item_charges(Ind, item);
	/* Describe the charges on the floor */
	else
		floor_item_charges(0 - item);

	if (p_ptr->shooty_till_kill) {
#if 0
		/* To continue shooting_till_kill, check if spell requires clean LOS to target
		   with no other monsters in the way, so we won't wake up more monsters accidentally. */
 #ifndef PY_PROJ_WALL
		if (!projectable_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE)) return;
 #else
		if (!projectable_wall_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE)) return;
 #endif

		/* We lost our target? (monster dead?) */
		if (dir != 5 || !target_okay(Ind)) return;
#endif
		/* we're now indeed ftk */
		p_ptr->shooting_till_kill = TRUE;
		p_ptr->shoot_till_kill_wand = item;
		/* disable other ftk types */
		p_ptr->shoot_till_kill_mimic = 0;
		p_ptr->shoot_till_kill_spell = 0;
		p_ptr->shoot_till_kill_rcraft = FALSE;
		p_ptr->shoot_till_kill_rod = 0;
	}
}


bool zap_rod(int Ind, int sval, int rad, object_type *o_ptr, bool *use_charge) {
	player_type *p_ptr = Players[Ind];
	bool ident = FALSE;

#ifdef NEW_MDEV_STACKING
	o_ptr->bpval++; /* count # of used rods of a stack of rods */
#endif

	/* Analyze the rod */
	switch (sval) {
	case SV_ROD_DETECT_TRAP:
		if (detect_trap(Ind, rad)) ident = TRUE;
		//if (o_ptr) o_ptr->pval += 50;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 25);
		break;

	case SV_ROD_DETECT_DOOR:
		if (detect_sdoor(Ind, rad)) ident = TRUE;
		//if (o_ptr) o_ptr->pval += 70;
		/* up to 50% faster with maxed MD - the_sandman */
		if (o_ptr) o_ptr->pval += 70 - get_skill_scale(p_ptr, SKILL_DEVICE, 35);
		break;

	case SV_ROD_IDENTIFY: //KEEP IN SYNC with cmd1.c '!X' zapping!
		ident = TRUE;
		if (!ident_spell(Ind)) *use_charge = FALSE;
		//if (o_ptr) o_ptr->pval += 10;
		/* up to 50% faster with maxed MD - the_sandman */
//at 0 skill, this is like auto-id	if (o_ptr) o_ptr->pval += 10 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 5);
		if (o_ptr) o_ptr->pval += 55 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 50);
		break;

	case SV_ROD_RECALL:
		if (!o_ptr) break;

		set_recall(Ind, rand_int(20) + 15, o_ptr);
		ident = TRUE;
		//o_ptr->pval += 60;
		/* up to a 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 60 - get_skill_scale(p_ptr, SKILL_DEVICE, 30);
		break;

	case SV_ROD_ILLUMINATION:
		if (o_ptr) msg_format_near(Ind, "%s calls light.", p_ptr->name);
		else msg_print_near(Ind, "A light appears.");
		if (lite_area(Ind, damroll(2, 8 + get_skill_scale(p_ptr, SKILL_DEVICE, 50)), 2)) ident = TRUE;
		if (p_ptr->suscep_lite && !p_ptr->resist_lite) take_hit(Ind, damroll(10, 3), "a rod of illumination", 0);
		if (p_ptr->suscep_lite && !p_ptr->resist_lite && !p_ptr->resist_blind) (void)set_blind(Ind, p_ptr->blind + 5 + randint(10));
		//if (o_ptr) o_ptr->pval += 30;
		/* up to a 50% faster with maxed MD - the_sandman */
		if (o_ptr) o_ptr->pval += 30 - get_skill_scale(p_ptr, SKILL_DEVICE, 15);
		break;

	case SV_ROD_MAPPING:
		map_area(Ind);
		ident = TRUE;
		//if (o_ptr) o_ptr->pval += 99;
		/* up to a 50% faster with maxed MD - the_sandman */
		if (o_ptr) o_ptr->pval += 99 - get_skill_scale(p_ptr, SKILL_DEVICE, 49);
		break;

	case SV_ROD_DETECTION:
		detection(Ind, rad);
		ident = TRUE;
		//if (o_ptr) o_ptr->pval += 99;
		/* up to a 50% faster with maxed MD - the_sandman */
		if (o_ptr) o_ptr->pval += 99 - get_skill_scale(p_ptr, SKILL_DEVICE, 49);
		break;

	case SV_ROD_PROBING:
		probing(Ind);
		ident = TRUE;
		//if (o_ptr) o_ptr->pval += 50;
		/* up to a 50% faster with maxed MD - the_sandman */
		if (o_ptr) o_ptr->pval += 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 25);
		break;

	case SV_ROD_CURING:
		if (set_image(Ind, 0)) ident = TRUE;
		if (set_blind(Ind, 0)) ident = TRUE;
		if (set_poisoned(Ind, 0, 0)) ident = TRUE;
		if (set_confused(Ind, 0)) ident = TRUE;
		if (set_stun(Ind, 0)) ident = TRUE;
		if (set_cut(Ind, 0, 0)) ident = TRUE;
		if (p_ptr->food >= PY_FOOD_MAX) /* ungorge */
			if (set_food(Ind, PY_FOOD_MAX - 1)) ident = TRUE;
		if (o_ptr) o_ptr->pval += 30 - get_skill_scale(p_ptr, SKILL_DEVICE, 20);
		break;

	case SV_ROD_HEALING:
//scale moar!		if (hp_player(Ind, 300 + get_skill_scale(p_ptr, SKILL_DEVICE, 50))) ident = TRUE;
		if (hp_player(Ind, 100 + get_skill_scale(p_ptr, SKILL_DEVICE, 300))) ident = TRUE;
		if (set_stun(Ind, 0)) ident = TRUE;
		if (set_cut(Ind, 0, 0)) ident = TRUE;
//a bit too much?	if (o_ptr) o_ptr->pval += 10 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 7);
		if (o_ptr) o_ptr->pval += 15 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 5);
		break;

	case SV_ROD_RESTORATION:
		if (restore_level(Ind)) ident = TRUE;
		if (do_res_stat(Ind, A_STR)) ident = TRUE;
		if (do_res_stat(Ind, A_INT)) ident = TRUE;
		if (do_res_stat(Ind, A_WIS)) ident = TRUE;
		if (do_res_stat(Ind, A_DEX)) ident = TRUE;
		if (do_res_stat(Ind, A_CON)) ident = TRUE;
		if (do_res_stat(Ind, A_CHR)) ident = TRUE;
		if (o_ptr) o_ptr->pval += 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 25);//adjusted it here too..
		break;

	case SV_ROD_SPEED:
		if (set_fast(Ind, randint(30) + 15, 10)) ident = TRUE; /* removed stacking */
		if (o_ptr) o_ptr->pval += 99 - get_skill_scale(p_ptr, SKILL_DEVICE, 49);
		break;

	case SV_ROD_NOTHING:
		break;

	default:
		msg_print(Ind, "SERVER ERROR: Directional rod zapped in non-directional function!");
		return FALSE;
	}

	return ident;
}

/*
 * Activate (zap) a Rod
 *
 * Unstack fully charged rods as needed.
 *
 * Hack -- rods of perception/genocide can be "cancelled"
 * All rods can be cancelled at the "Direction?" prompt
 */
void do_cmd_zap_rod(int Ind, int item, int dir) {
	player_type *p_ptr = Players[Ind];
	int lev, ident, rad = DEFAULT_RADIUS_DEV(p_ptr);
	object_type *o_ptr;
	u32b f4, dummy;
#ifdef NEW_MDEV_STACKING
	int pval_old;
#endif

	/* Hack -- let perception get aborted */
	bool use_charge = TRUE, flipped = FALSE;

#ifdef ENABLE_XID_MDEV
 #ifdef XID_REPEAT
	bool rep = p_ptr->command_rep_active
	    && p_ptr->current_item != -1; //extra sanity check, superfluous?
	p_ptr->command_rep = 0;
	p_ptr->command_rep_active = FALSE;
 #endif
#endif

	/* Break goi/manashield */
#if 0
	if (p_ptr->invuln)
		set_invuln(Ind, 0);
	if (p_ptr->tim_manashield)
		set_tim_manashield(Ind, 0);
	if (p_ptr->tim_wraith)
		set_tim_wraith(Ind, 0);
#endif	// 0

	if (get_skill(p_ptr, SKILL_ANTIMAGIC)) {
		msg_format(Ind, "\377%cYou don't believe in magic.", COLOUR_AM_OWN);
		return;
	}

	/* Restrict choices to rods */
	item_tester_tval = TV_ROD;

	/* Get the item (in the pack) */
	if (item >= 0)
		o_ptr = &p_ptr->inventory[item];
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max) return; /* item doesn't exist */
		o_ptr = &o_list[0 - item];
	}
	if (check_guard_inscription(o_ptr->note, 'z')) {
		msg_print(Ind, "The item's inscription prevents it.");
		return;
	};


	if (o_ptr->tval != TV_ROD) {
//(may happen on death, from macro spam)		msg_print(Ind, "SERVER ERROR: Tried to zap non-rod!");
		return;
	}

	/* Mega-Hack -- refuse to zap a pile from the ground */
	if ((item < 0) && (o_ptr->number > 1)) {
		msg_print(Ind, "You must first pick up the rods.");
		return;
	}

	if (!can_use_verbose(Ind, o_ptr)) return;

#if 1
	if (p_ptr->anti_magic) {
		msg_format(Ind, "\377%cYour anti-magic shell disrupts your attempt.", COLOUR_AM_OWN);
		return;
	}
#endif

	/* Get a direction (unless KNOWN not to need it) */
	/* Pfft, dirty, dirty, diiirrrrtie!! (FIXME) */
	if (rod_requires_direction(Ind, o_ptr)) {
		if (dir != 0 && dir != 11) {
		//redundant: lower versions cant have dir != 0. ---  && is_newer_than(&p_ptr->version, 4, 4, 5, 10, 0, 0)) {
			p_ptr->current_rod = item;
			do_cmd_zap_rod_dir(Ind, dir);
			return;
		}

		/* Get a direction, then return */
		p_ptr->current_rod = item;
		get_aim_dir(Ind);
		return;
	}

	/* Extract object flags */
	object_flags(o_ptr, &dummy, &dummy, &dummy, &f4, &dummy, &dummy, &dummy);

	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos) / ((f4 & TR4_FAST_CAST) ? 2 : 1);

	/* Not identified yet */
	ident = FALSE;

	if (magik((p_ptr->antimagic * 8) / 5)) {
#ifdef USE_SOUND_2010
		sound(Ind, "am_field", NULL, SFX_TYPE_MISC, FALSE);
#endif
		msg_format(Ind, "\377%cYour anti-magic field disrupts your attempt.", COLOUR_AM_OWN);
		return;
	}

	if (!activate_magic_device(Ind, o_ptr)) {
		msg_format(Ind, "\377%cYou failed to use the rod properly." , COLOUR_MD_FAIL);
#ifdef ENABLE_XID_MDEV
 #ifdef XID_REPEAT
		/* hack: repeat ID-spell attempt until item is successfully identified */
		if (rep && !object_known_p(Ind, &p_ptr->inventory[p_ptr->current_item])) {
			sockbuf_t *conn_q = get_conn_q(Ind);

			p_ptr->command_rep = PKT_ZAP;
			p_ptr->command_rep_active = TRUE;
			Packet_printf(conn_q, "%c%hd", PKT_ZAP, item);
		} else p_ptr->current_item = -1;
 #endif
#endif
		return;
	}

	/* Still charging */
#ifndef NEW_MDEV_STACKING
	if (o_ptr->pval) {
#else
	if (o_ptr->bpval == o_ptr->number) {
#endif
		if (o_ptr->number == 1) msg_print(Ind, "The rod is still charging.");
		else msg_print(Ind, "The rods are still charging.");
		return;
	}

	process_hooks(HOOK_ZAP, "d", Ind);

#ifdef NEW_MDEV_STACKING
	pval_old = o_ptr->pval;
#endif
	ident = zap_rod(Ind, o_ptr->sval, rad, o_ptr, &use_charge);
#ifdef NEW_MDEV_STACKING
	if (f4 & TR4_CHARGING) o_ptr->pval -= (o_ptr->pval - pval_old) / 2;
#else
	if (f4 & TR4_CHARGING) o_ptr->pval /= 2;
#endif
#ifdef ENABLE_XID_MDEV
 #ifdef XID_REPEAT
	if (rep) p_ptr->current_item = -1;
 #endif
#endif

	break_cloaking(Ind, 3);
	break_shadow_running(Ind);
	stop_precision(Ind);
#ifdef CONTINUE_FTK
	if (rod_requires_direction(Ind, o_ptr))
#endif
	stop_shooting_till_kill(Ind);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Extract the item level */
	lev = k_info[o_ptr->k_idx].level;

	/* Successfully determined the object function */
	if (ident && !object_aware_p(Ind, o_ptr)) {
		flipped = object_aware(Ind, o_ptr);
		if (!(p_ptr->mode & MODE_PVP)) gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Tried the object */
	object_tried(Ind, o_ptr, flipped);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Hack -- deal with cancelled zap */
	if (!use_charge) {
#ifndef NEW_MDEV_STACKING
		o_ptr->pval = 0;
#else
		o_ptr->pval = pval_old;
		o_ptr->bpval--;
#endif
		return;
	}

#ifndef NEW_MDEV_STACKING
	/* XXX Hack -- unstack if necessary */
	if ((item >= 0) && (o_ptr->number > 1)) {
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
#endif
}



/*
 * Activate (zap) a Rod that requires a direction (passed through to here from do_cmd_zap_rod())
 *
 * Unstack fully charged rods as needed.
 *
 * Hack -- rods of perception/genocide can be "cancelled"
 * All rods can be cancelled at the "Direction?" prompt
 */
void do_cmd_zap_rod_dir(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	int lev, item, ident, rad = DEFAULT_RADIUS_DEV(p_ptr);
	object_type *o_ptr;
	u32b f4, dummy;
	/* Hack -- let perception get aborted */
	bool use_charge = TRUE, flipped = FALSE;
#ifdef NEW_MDEV_STACKING
	int pval_old;
#endif

	item = p_ptr->current_rod;

	/* Get the item (in the pack) */
	if (item >= 0) {
		o_ptr = &p_ptr->inventory[item];
	}
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max) {
			p_ptr->shooting_till_kill = FALSE;
			return; /* item doesn't exist */
		}

		o_ptr = &o_list[0 - item];
	}
	if (o_ptr->tval != TV_ROD) {
//(may happen on death, from macro spam)		msg_print(Ind, "SERVER ERROR: Tried to zap non-rod!");
		p_ptr->shooting_till_kill = FALSE;
		return;
	}

	/* Handle FTK targetting mode */
	if (p_ptr->shooting_till_kill) { /* we were shooting till kill last turn? */
		p_ptr->shooting_till_kill = FALSE; /* well, gotta re-test for another success now.. */
		p_ptr->shooty_till_kill = TRUE; /* so for now we are just ATTEMPTING to shoot till kill (assumed we have a monster for target) */
	}
	if (p_ptr->shooty_till_kill) {
		if (dir == 5 && check_guard_inscription(o_ptr->note, 'K')) {
			/* We lost our target? (monster dead?) */
			if (target_okay(Ind)) {
				/* To continue shooting_till_kill, check if spell requires clean LOS to target
				   with no other monsters in the way, so we won't wake up more monsters accidentally. */
 #ifndef PY_PROJ_WALL
				if (!projectable_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE)) return;
 #else
				if (!projectable_wall_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE)) return;
 #endif
			} else p_ptr->shooty_till_kill = FALSE;
		} else p_ptr->shooty_till_kill = FALSE;
	}

	if (get_skill(p_ptr, SKILL_ANTIMAGIC)) {
		msg_format(Ind, "\377%cYou don't believe in magic.", COLOUR_AM_OWN);
		return;
	}

	if (check_guard_inscription( o_ptr->note, 'z')) {
		msg_print(Ind, "The item's inscription prevents it.");
		return;
	};

	/* Mega-Hack -- refuse to zap a pile from the ground */
	if ((item < 0) && (o_ptr->number > 1)) {
		msg_print(Ind, "You must first pick up the rods.");
		return;
	}

	if (!can_use_verbose(Ind, o_ptr)) return;

#if 1
	if (p_ptr->anti_magic) {
		msg_format(Ind, "\377%cYour anti-magic shell disrupts your attempt.", COLOUR_AM_OWN);
		return;
	}
#endif

	/* Hack -- verify potential overflow */
	/*if ((inven_cnt >= INVEN_PACK) &&
	    (o_ptr->number > 1))
	{*/
		/* Verify with the player */
		/*if (other_query_flag &&
		    !get_check("Your pack might overflow.  Continue? ")) return;
	}*/

	/* Extract object flags */
	object_flags(o_ptr, &dummy, &dummy, &dummy, &f4, &dummy, &dummy, &dummy);


	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos) / ((f4 & TR4_FAST_CAST) ? 2 : 1);

	/* Not identified yet */
	ident = FALSE;

	if (magik((p_ptr->antimagic * 8) / 5)) {
#ifdef USE_SOUND_2010
		sound(Ind, "am_field", NULL, SFX_TYPE_MISC, FALSE);
#endif
		msg_format(Ind, "\377%cYour anti-magic field disrupts your attempt.", COLOUR_AM_OWN);

		//don't cancel FTK since this failure was just a chance thing
		if (p_ptr->shooty_till_kill) {
			p_ptr->shooting_till_kill = TRUE;
			p_ptr->shoot_till_kill_rod = item;
			/* disable other ftk types */
			p_ptr->shoot_till_kill_mimic = 0;
			p_ptr->shoot_till_kill_spell = 0;
			p_ptr->shoot_till_kill_rcraft = FALSE;
			p_ptr->shoot_till_kill_wand = 0;
		}
		return;
	}

	/* Roll for usage */
	if (!activate_magic_device(Ind, o_ptr)) {
		msg_format(Ind, "\377%cYou failed to use the rod properly." , COLOUR_MD_FAIL);

		//don't cancel FTK since this failure was just a chance thing
		if (p_ptr->shooty_till_kill) {
			p_ptr->shooting_till_kill = TRUE;
			p_ptr->shoot_till_kill_rod = item;
			/* disable other ftk types */
			p_ptr->shoot_till_kill_mimic = 0;
			p_ptr->shoot_till_kill_spell = 0;
			p_ptr->shoot_till_kill_rcraft = FALSE;
			p_ptr->shoot_till_kill_wand = 0;
		}
		return;
	}

	/* Still charging */
#ifndef NEW_MDEV_STACKING
	if (o_ptr->pval) {
#else
	if (o_ptr->bpval == o_ptr->number) {
#endif
		if (o_ptr->number == 1) msg_print(Ind, "The rod is still charging.");
		else msg_print(Ind, "The rods are still charging.");
		return;
	}

	process_hooks(HOOK_ZAP, "d", Ind);

#ifdef NEW_MDEV_STACKING
	pval_old = o_ptr->pval;
	o_ptr->bpval++; /* count # of used rods of a stack of rods */
#endif

	/* Analyze the rod */
	switch (o_ptr->sval) {
	case SV_ROD_TELEPORT_AWAY:
		if (teleport_monster(Ind, dir)) ident = TRUE;
		//o_ptr->pval += 25;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 25 - get_skill_scale(p_ptr, SKILL_DEVICE, 12);
		break;

	case SV_ROD_DISARMING:
		if (disarm_trap_door(Ind, dir)) ident = TRUE;
		//o_ptr->pval += 30;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 30 - get_skill_scale(p_ptr, SKILL_DEVICE, 15);
		break;

	case SV_ROD_LITE:
		msg_print(Ind, "A line of blue shimmering light appears.");
		lite_line(Ind, dir, damroll(6, 8) + get_skill_scale(p_ptr, SKILL_DEVICE, 50), FALSE);
		ident = TRUE;
		//o_ptr->pval += 9;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 9 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 4);
		break;

	case SV_ROD_SLEEP_MONSTER:
		if (sleep_monster(Ind, dir, 10 + p_ptr->lev + get_skill_scale(p_ptr, SKILL_DEVICE, 50))) ident = TRUE;
		//o_ptr->pval += 18;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 18 - get_skill_scale(p_ptr, SKILL_DEVICE, 9);
		break;

	case SV_ROD_SLOW_MONSTER:
		if (slow_monster(Ind, dir, 10 + p_ptr->lev + get_skill_scale(p_ptr, SKILL_DEVICE, 50))) ident = TRUE;
		//o_ptr->pval += 20;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 20 - get_skill_scale(p_ptr, SKILL_DEVICE, 10);
		break;

	case SV_ROD_DRAIN_LIFE:
		if (drain_life(Ind, dir, 10 + get_skill_scale(p_ptr, SKILL_DEVICE, 10))) {
			ident = TRUE;
			hp_player(Ind, p_ptr->ret_dam / 4);
			p_ptr->ret_dam = 0;
		}
		//o_ptr->pval += 23;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 23 - get_skill_scale(p_ptr, SKILL_DEVICE, 12);
		break;

	case SV_ROD_POLYMORPH:
		//todo for IDDC maybe: change all rods to wands (disallow rod drops), if poly should be enabled
		if (poly_monster(Ind, dir)) ident = TRUE;
		//o_ptr->pval += 25;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 25 - get_skill_scale(p_ptr, SKILL_DEVICE, 12);
		break;

	case SV_ROD_ACID_BOLT:
		msg_format_near(Ind, "%s fires an acid bolt.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires an acid bolt for");
		fire_bolt_or_beam(Ind, 10, GF_ACID, dir, damroll(6 + get_skill_scale(p_ptr, SKILL_DEVICE, 45), 8), p_ptr->attacker);
		ident = TRUE;
		//o_ptr->pval += 12;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 12 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 6);
		break;

	case SV_ROD_ELEC_BOLT:
		msg_format_near(Ind, "%s fires a lightning bolt.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a lightning bolt for");
		fire_bolt_or_beam(Ind, 10, GF_ELEC, dir, damroll(4 + get_skill_scale(p_ptr, SKILL_DEVICE, 45), 8), p_ptr->attacker);
		ident = TRUE;
		//o_ptr->pval += 11;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 11 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 5);
		break;

	case SV_ROD_FIRE_BOLT:
		msg_format_near(Ind, "%s fires a fire bolt.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a fire bolt for");
		fire_bolt_or_beam(Ind, 10, GF_FIRE, dir, damroll(7 + get_skill_scale(p_ptr, SKILL_DEVICE, 45), 8), p_ptr->attacker);
		ident = TRUE;
		//o_ptr->pval += 15;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 15 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 7);
		break;

	case SV_ROD_COLD_BOLT:
		msg_format_near(Ind, "%s fires a frost bolt.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a frost bolt for");
		fire_bolt_or_beam(Ind, 10, GF_COLD, dir, damroll(5 + get_skill_scale(p_ptr, SKILL_DEVICE, 45), 8), p_ptr->attacker);
		ident = TRUE;
		//o_ptr->pval += 13;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 13 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 6);
		break;

	case SV_ROD_ACID_BALL:
		msg_format_near(Ind, "%s fires an acid ball.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires an acid ball for");
		fire_ball(Ind, GF_ACID, dir, 60 + get_skill_scale(p_ptr, SKILL_DEVICE, 270), 2, p_ptr->attacker);
		ident = TRUE;
		//o_ptr->pval += 27;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 27 - get_skill_scale(p_ptr, SKILL_DEVICE, 13);
		break;

	case SV_ROD_ELEC_BALL:
		msg_format_near(Ind, "%s fires a lightning ball.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a lightning ball for");
		fire_ball(Ind, GF_ELEC, dir, 32 + get_skill_scale(p_ptr, SKILL_DEVICE, 270), 2, p_ptr->attacker);
		ident = TRUE;
		//o_ptr->pval += 23;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 23 - get_skill_scale(p_ptr, SKILL_DEVICE, 11);
		break;

	case SV_ROD_FIRE_BALL:
		msg_format_near(Ind, "%s fires a fire ball.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a fire ball for");
		fire_ball(Ind, GF_FIRE, dir, 72 + get_skill_scale(p_ptr, SKILL_DEVICE, 270), 2, p_ptr->attacker);
		ident = TRUE;
		//o_ptr->pval += 30;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 30 - get_skill_scale(p_ptr, SKILL_DEVICE, 15);
		break;

	case SV_ROD_COLD_BALL:
		msg_format_near(Ind, "%s fires a frost ball.", p_ptr->name);
		sprintf(p_ptr->attacker, " fires a frost ball for");
		fire_ball(Ind, GF_COLD, dir, 48 + get_skill_scale(p_ptr, SKILL_DEVICE, 270), 2, p_ptr->attacker);
		ident = TRUE;
		//o_ptr->pval += 25;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 25 - get_skill_scale(p_ptr, SKILL_DEVICE, 12);
		break;

	/* All of the following are needed if we tried zapping one of */
	/* these but we didn't know what it was. */
	case SV_ROD_DETECT_TRAP:
		if (detect_trap(Ind, rad)) ident = TRUE;
		//o_ptr->pval += 50;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 25);
		break;

	case SV_ROD_DETECT_DOOR:
		if (detect_sdoor(Ind, rad)) ident = TRUE;
		//o_ptr->pval += 70;
		/* up to 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 70 - get_skill_scale(p_ptr, SKILL_DEVICE, 35);
		break;

	case SV_ROD_IDENTIFY:
		ident = TRUE;
		if (!ident_spell(Ind)) use_charge = FALSE;
		//o_ptr->pval += 10;
		/* up to 50% faster with maxed MD - the_sandman */
//at 0 skill, this is like auto-id	o_ptr->pval += 10 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 5);
		o_ptr->pval += 55 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 50);
		break;

	case SV_ROD_RECALL:
		set_recall(Ind, rand_int(20) + 15, o_ptr);
		ident = TRUE;
		//o_ptr->pval += 60;
		/* up to a 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 60 - get_skill_scale(p_ptr, SKILL_DEVICE, 30);
		break;

	case SV_ROD_ILLUMINATION:
		msg_format_near(Ind, "%s calls light.", p_ptr->name);
		if (lite_area(Ind, damroll(2, 8 + get_skill_scale(p_ptr, SKILL_DEVICE, 50)), 2)) ident = TRUE;
		if (p_ptr->suscep_lite && !p_ptr->resist_lite) take_hit(Ind, damroll(10, 3), "a rod of illumination", 0);
		if (p_ptr->suscep_lite && !p_ptr->resist_lite && !p_ptr->resist_blind) (void)set_blind(Ind, p_ptr->blind + 5 + randint(10));
		//o_ptr->pval += 30;
		/* up to a 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 30 - get_skill_scale(p_ptr, SKILL_DEVICE, 15);
		break;

	case SV_ROD_MAPPING:
		map_area(Ind);
		ident = TRUE;
		//o_ptr->pval += 99;
		/* up to a 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 99 - get_skill_scale(p_ptr, SKILL_DEVICE, 49);
		break;

	case SV_ROD_DETECTION:
		detection(Ind, rad);
		ident = TRUE;
		//o_ptr->pval += 99;
		/* up to a 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 99 - get_skill_scale(p_ptr, SKILL_DEVICE, 49);
		break;

	case SV_ROD_PROBING:
		probing(Ind);
		ident = TRUE;
		//o_ptr->pval += 50;
		/* up to a 50% faster with maxed MD - the_sandman */
		o_ptr->pval += 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 25);
		break;

	case SV_ROD_CURING:
		if (set_image(Ind, 0)) ident = TRUE;
		if (set_blind(Ind, 0)) ident = TRUE;
		if (set_poisoned(Ind, 0, 0)) ident = TRUE;
		if (set_confused(Ind, 0)) ident = TRUE;
		if (set_stun(Ind, 0)) ident = TRUE;
		if (set_cut(Ind, 0, 0)) ident = TRUE;
		if (p_ptr->food >= PY_FOOD_MAX) /* ungorge */
			if (set_food(Ind, PY_FOOD_MAX - 1)) ident = TRUE;
		o_ptr->pval += 30 - get_skill_scale(p_ptr, SKILL_DEVICE, 20);
		break;

	case SV_ROD_HEALING:
//scale moar!		if (hp_player(Ind, 300 + get_skill_scale(p_ptr, SKILL_DEVICE, 50))) ident = TRUE;
		if (hp_player(Ind, 100 + get_skill_scale(p_ptr, SKILL_DEVICE, 300))) ident = TRUE;
		if (set_stun(Ind, 0)) ident = TRUE;
		if (set_cut(Ind, 0, 0)) ident = TRUE;
//a bit too much?	o_ptr->pval += 10 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 7);
		o_ptr->pval += 15 - get_skill_scale_fine(p_ptr, SKILL_DEVICE, 5);
		break;

	case SV_ROD_RESTORATION:
		if (restore_level(Ind)) ident = TRUE;
		if (do_res_stat(Ind, A_STR)) ident = TRUE;
		if (do_res_stat(Ind, A_INT)) ident = TRUE;
		if (do_res_stat(Ind, A_WIS)) ident = TRUE;
		if (do_res_stat(Ind, A_DEX)) ident = TRUE;
		if (do_res_stat(Ind, A_CON)) ident = TRUE;
		if (do_res_stat(Ind, A_CHR)) ident = TRUE;
		o_ptr->pval += 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 25);//adjusted it here too..
		break;

	case SV_ROD_SPEED:
		if (set_fast(Ind, randint(30) + 15, 10)) ident = TRUE; /* removed stacking */
		o_ptr->pval += 99 - get_skill_scale(p_ptr, SKILL_DEVICE, 49);
		break;

	case SV_ROD_HAVOC:
		sprintf(p_ptr->attacker, " invokes havoc for");
		fire_ball(Ind, GF_HAVOC, dir, 500 + get_skill_scale(p_ptr, SKILL_DEVICE, 300), 4, p_ptr->attacker);
		fire_cloud(Ind, GF_HAVOC, dir, 50 + get_skill_scale(p_ptr, SKILL_DEVICE, 50), 4, get_skill_scale(p_ptr, SKILL_DEVICE, 3) + 12, 10, p_ptr->attacker); //max 1350 dam total
		ident = TRUE;
		o_ptr->pval += 105 - get_skill_scale(p_ptr, SKILL_DEVICE, 23);
		break;

	case SV_ROD_NOTHING:
		break;

	default:
		msg_print(Ind, "SERVER ERROR: Tried to zap non-directional rod in directional function!");
		return;
	}

	break_cloaking(Ind, 3);
	break_shadow_running(Ind);
	stop_precision(Ind);
#ifdef CONTINUE_FTK
	if (rod_requires_direction(Ind, o_ptr))
#endif
	stop_shooting_till_kill(Ind);

	/* Clear the current rod */
	p_ptr->current_rod = -1;

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Extract the item level */
	lev = k_info[o_ptr->k_idx].level;

	/* Successfully determined the object function */
	if (ident && !object_aware_p(Ind, o_ptr)) {
		flipped = object_aware(Ind, o_ptr);
		if (!(p_ptr->mode & MODE_PVP)) gain_exp(Ind, (lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	/* Tried the object */
	object_tried(Ind, o_ptr, flipped);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Hack -- deal with cancelled zap */
	if (!use_charge) {
#ifndef NEW_MDEV_STACKING
		o_ptr->pval = 0;
#else
		o_ptr->bpval--;
		o_ptr->pval = pval_old;
#endif
		return;
	}

#ifndef NEW_MDEV_STACKING
	/* XXX Hack -- unstack if necessary */
	if ((item >= 0) && (o_ptr->number > 1)) {
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
#endif

	if (p_ptr->shooty_till_kill) {
#if 0
		/* To continue shooting_till_kill, check if spell requires clean LOS to target
		   with no other monsters in the way, so we won't wake up more monsters accidentally. */
 #ifndef PY_PROJ_WALL
		if (!projectable_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE)) return;
 #else
		if (!projectable_wall_real(Ind, p_ptr->py, p_ptr->px, p_ptr->target_row, p_ptr->target_col, MAX_RANGE)) return;
 #endif

		/* We lost our target? (monster dead?) */
		if (dir != 5 || !target_okay(Ind)) return;
#endif
		/* we're now indeed ftk */
		p_ptr->shooting_till_kill = TRUE;
		p_ptr->shoot_till_kill_rod = item;
		/* disable other ftk types */
		p_ptr->shoot_till_kill_mimic = 0;
		p_ptr->shoot_till_kill_spell = 0;
		p_ptr->shoot_till_kill_rcraft = FALSE;
		p_ptr->shoot_till_kill_wand = 0;
	}
}



/*
 * Hook to determine if an object is activatable
 */
static bool item_tester_hook_activate(int Ind, object_type *o_ptr) {
	u32b f3, dummy;

	/* Not known */
	if (!object_known_p(Ind, o_ptr)) return (FALSE);

	/* Extract the flags */
	object_flags(o_ptr, &dummy, &dummy, &f3, &dummy, &dummy, &dummy, &dummy);

	/* Check activation flag */
	if (f3 & TR3_ACTIVATE) return (TRUE);

	/* Assume not */
	return (FALSE);
}



/*
 * Hack -- activate the ring of power
 */
static void ring_of_power(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];

	/* Pick a random effect */
	switch (randint(10)) {
	case 1:
	case 2:
		/* Message */
		msg_print(Ind, "You are surrounded by a malignant aura.");

		/* Decrease all stats (permanently) */
		(void)dec_stat(Ind, A_STR, 50, STAT_DEC_NORMAL);
		(void)dec_stat(Ind, A_INT, 50, STAT_DEC_NORMAL);
		(void)dec_stat(Ind, A_WIS, 50, STAT_DEC_NORMAL);
		(void)dec_stat(Ind, A_DEX, 50, STAT_DEC_NORMAL);
		(void)dec_stat(Ind, A_CON, 50, STAT_DEC_NORMAL);
		(void)dec_stat(Ind, A_CHR, 50, STAT_DEC_NORMAL);

		/* Lose some experience (permanently) */
		take_xp_hit(Ind, p_ptr->exp / 50, "Ring of Power", TRUE, FALSE, TRUE);
		break;

	case 3:
		/* Message */
		msg_print(Ind, "You are surrounded by a powerful aura.");

		/* Dispel monsters */
		dispel_monsters(Ind, 1000);

		break;

	case 4:
	case 5:
	case 6:
		/* Mana Ball */
		sprintf(p_ptr->attacker, " invokes a mana storm for");
		fire_ball(Ind, GF_MANA, dir, 500, 3, p_ptr->attacker);

		break;

	case 7:
	case 8:
	case 9:
	case 10:
		/* Mana Bolt */
		sprintf(p_ptr->attacker, " fires a mana bolt for");
		fire_bolt(Ind, GF_MANA, dir, 250, p_ptr->attacker);

		break;
	}
}




/*
 * Enchant some bolts
 */
static bool brand_bolts(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i;

	/* Use the first (XXX) acceptable bolts */
	for (i = 0; i < INVEN_PACK; i++) {
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

		/* Hack -- you don't sell the wep blessed by your god, do you? :) */
		o_ptr->discount = 100;

		/* Notice */
		return (TRUE);
	}

	/* Fail */
	msg_print(Ind, "The fiery enchantment failed.");

	/* Notice */
	return (TRUE);
}


/* hard-coded same as the 2 functions below, do_cmd_activate() and do_cmd_activate_dir().
   Returns TRUE if item is activatable, for sending the client proper targetting info. - C. Blue */
bool activation_requires_direction(object_type *o_ptr) {
	/* Art DSMs are handled below */
	if (o_ptr->tval == TV_DRAG_ARMOR && !o_ptr->name1)
		return TRUE;

	/* Artifacts activate by name */
	if (o_ptr->name1) {
		/* This needs to be changed */
		switch (o_ptr->name1) {
		case ART_NARTHANC:
		case ART_NIMTHANC:
		case ART_DETHANC:
		case ART_RILIA:
		case ART_BELANGIL:
		case ART_RINGIL:
		case ART_ANDURIL:
		case ART_FIRESTAR:
		case ART_THEODEN:
		case ART_TURMIL:
		case ART_ARUNRUTH:
		case ART_AEGLOS:
		case ART_OROME:
		case ART_ULMO:
		case ART_TOTILA:
		case ART_CAMMITHRIM:
		case ART_PAURHACH:
		case ART_PAURNIMMEN:
		case ART_PAURAEGEN:
		case ART_PAURNEN:
		case ART_FINGOLFIN:
		case ART_NARYA:
		case ART_NENYA:
		case ART_VILYA:
		case ART_POWER:
		case ART_MEDIATOR:
		case ART_AXE_GOTHMOG:
		case ART_MELKOR:
		case ART_NIGHT:
		case ART_NAIN:
		case ART_EOL:
		case ART_UMBAR:
		case ART_HELLFIRE:
		case ART_HAVOC:
			return TRUE;
		}
	}

	/* Hack -- Amulet of the Serpents can be activated as well */
	else if ((o_ptr->tval == TV_AMULET) && (o_ptr->sval == SV_AMULET_SERPENT)) {
		return TRUE;
	}

	else if (o_ptr->tval == TV_RING) {
		switch (o_ptr->sval) {
		case SV_RING_ELEC:
		case SV_RING_ACID:
		case SV_RING_ICE:
		case SV_RING_FLAMES:
			return TRUE;
		}
	}

	/* All other items aren't activatable */
	return FALSE;
}

bool rod_requires_direction(int Ind, object_type *o_ptr) {
	int sval = o_ptr->sval;

	if (Ind && !object_aware_p(Ind, o_ptr)) return TRUE;

	if ((sval >= SV_ROD_MIN_DIRECTION) &&
	    !(sval == SV_ROD_DETECT_TRAP) &&
	    !(sval == SV_ROD_HOME))
		return TRUE;

	return FALSE;
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
void do_cmd_activate(int Ind, int item, int dir) {
	player_type *p_ptr = Players[Ind];
	int i, k;
	bool done = FALSE;
	//int md = get_skill_scale(p_ptr, SKILL_DEVICE, 100);
	object_type *o_ptr;

#ifdef ENABLE_XID_MDEV
 #ifdef XID_REPEAT
	bool rep = p_ptr->command_rep_active
	    && p_ptr->current_item != -1; //extra sanity check, superfluous?
	p_ptr->command_rep = 0;
	p_ptr->command_rep_active = FALSE;
 #endif
#endif

	/* Break goi/manashield */
#if 0
	if (p_ptr->invuln)
		set_invuln(Ind, 0);
	if (p_ptr->tim_manashield)
		set_tim_manashield(Ind, 0);
	if (p_ptr->tim_wraith)
		set_tim_wraith(Ind, 0);
#endif	// 0


	/* Get the item (in the pack) */
	if (item >= 0) {
		o_ptr = &p_ptr->inventory[item];
	}
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max) return; /* item doesn't exist */
		o_ptr = &o_list[0 - item];
	}

	/* dual-wield hack: cannot activate items if armour is too heavy.
	   Spectral weapons will not drain life either ;). */
	if (item == INVEN_ARM && o_ptr->tval != TV_SHIELD && p_ptr->rogue_heavyarmor) {
		msg_format(Ind, "\377oYour armour is too heavy for dual-wielding, preventing activation of your secondary weapon.");
		return;
	}

	if (o_ptr->tval != TV_BOTTLE) { /* hack.. */
		if (p_ptr->anti_magic) {
			msg_format(Ind, "\377%cYour anti-magic shell disrupts your attempt.", COLOUR_AM_OWN);
			return;
		}
		if (get_skill(p_ptr, SKILL_ANTIMAGIC)) {
			msg_format(Ind, "\377%cYou don't believe in magic.", COLOUR_AM_OWN);
			return;
		}
		if (magik((p_ptr->antimagic * 8) / 5)) {
#ifdef USE_SOUND_2010
			sound(Ind, "am_field", NULL, SFX_TYPE_MISC, FALSE);
#endif
			msg_format(Ind, "\377%cYour anti-magic field disrupts your attempt.", COLOUR_AM_OWN);
			return;
		}
	}

	/* If the item can be equipped, it MUST be equipped to be activated */
	if ((item < INVEN_WIELD) && wearable_p(o_ptr)) {
		msg_print(Ind, "You must be using this item to activate it.");
		return;
	}

	if (check_guard_inscription( o_ptr->note, 'A')) {
		msg_print(Ind, "The item's inscription prevents it..");
		return;
	}

	if (!can_use_verbose(Ind, o_ptr)) return;

	/* Test the item */
	if (!item_tester_hook_activate(Ind, o_ptr)) {
		/* Hack -- activating bottles */
		if (o_ptr->tval == TV_BOTTLE) {
			do_cmd_fill_bottle(Ind);
			return;
		}

		msg_print(Ind, "You cannot activate that item.");
		return;
	}

	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	/* Roll for usage */
	if (!activate_magic_device(Ind, o_ptr) &&
	    o_ptr->tval != TV_BOOK) /* hack: blank books can always be 'activated' */
	{
		msg_format(Ind, "\377%cYou failed to activate it properly.", COLOUR_MD_FAIL);
#ifdef ENABLE_XID_MDEV
 #ifdef XID_REPEAT
		/* hack: repeat ID-spell attempt until item is successfully identified */
		if (rep && !object_known_p(Ind, &p_ptr->inventory[p_ptr->current_item])) {
			sockbuf_t *conn_q = get_conn_q(Ind);

			p_ptr->command_rep = PKT_ACTIVATE;
			p_ptr->command_rep_active = TRUE;
			Packet_printf(conn_q, "%c%hd", PKT_ACTIVATE, item);
		} else p_ptr->current_item = -1;
 #endif
#endif
		return;
	}

	/* Check the recharge */
	if (o_ptr->recharging) {
		msg_print(Ind, "It whines, glows and fades...");
		return;
	}

	process_hooks(HOOK_ACTIVATE, "d", Ind);

	if (o_ptr->tval != TV_RUNE) { //Disable rune 'you activate it' text.
		if (o_ptr->tval != TV_BOOK) {
			/* Wonder Twin Powers... Activate! */
			msg_print(Ind, "You activate it...");
		} else {
			msg_print(Ind, "You open the book to add another new spell..");
		}
	}

	break_cloaking(Ind, 0);
	stop_precision(Ind);
#ifndef CONTINUE_FTK
	stop_shooting_till_kill(Ind);
#endif

	if (activation_requires_direction(o_ptr)
//appearently not for A'able items >_>	    || !object_aware_p(Ind, o_ptr)
	    ) {
#ifdef CONTINUE_FTK
		stop_shooting_till_kill(Ind);
#endif

		if (dir != 0 && dir != 11) {
		//redundant: lower versions cant have dir != 0. ---  && is_newer_than(&p_ptr->version, 4, 4, 5, 10, 0, 0)) {
			p_ptr->current_activation = item;
			do_cmd_activate_dir(Ind, dir);
			return;
		}
	}

	// -------------------- special basic items that can't vary -------------------- //
	//(could be moved down to 'base items' for less efficiency but better sort order)

	/* Rune de/re-combination */
	if (o_ptr->tval == TV_RUNE) {
		if (o_ptr->sval < RCRAFT_MAX_ELEMENTS) { //basic rune, so combine; keep lowest sale value, highest level
			/* Remember the original rune */
			p_ptr->current_activation = item;

			/* Get the rune to combine with */
			rune_combine(Ind);

			return;
		} else { //top-tier rune, so decombine
			if (o_ptr->level || o_ptr->owner == p_ptr->id) { //Do we own it..?
				/* Lookup Component Rune SVALS (matched to tables.c index) */
				u16b e_flags = r_projections[o_ptr->sval].flags;
				byte element[RCRAFT_MAX_ELEMENTS];
				byte elements = 0; // = flags_to_elements(element, e_flags);
				for (i = 0; i < RCRAFT_MAX_ELEMENTS; i++)
					if ((e_flags & r_elements[i].flag) == r_elements[i].flag) {
						element[elements] = i;
						elements++;
					}

				/* Remember the original info */
				s32b owner = o_ptr->owner;
				byte mode = o_ptr->mode;
				s16b level = o_ptr->level;
				byte discount = o_ptr->discount;
				byte number = o_ptr->number;
				s16b note = o_ptr->note;
				s32b iron_trade = o_ptr->iron_trade;

				/* Rune Preservation? */
				if (check_guard_inscription(o_ptr->note, 'R')) {
					if (number > 1) number--;
					else {
						msg_format(Ind, "\377yYou choose to preserve your rune.");
						return;
					}
				}

				/* Destroy the rune stack in the pack (or reduce to just one) */
				msg_format(Ind, "There is a decoupling of magic.");
				inven_item_increase(Ind, item, -number);
				inven_item_optimize(Ind, item);

				/* Make new stacks */
				object_type *q_ptr, forge;
				for (i = 0; i < RSPELL_MAX_ELEMENTS; i++) {

					/* Default rune template */
					q_ptr = &forge;
					object_wipe(q_ptr);
					invcopy(q_ptr, lookup_kind(TV_RUNE, element[i]));

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
			}
			return;
		}
	}

	if (o_ptr->tval == TV_GOLEM) {
		int m_idx = 0, k;
		monster_type *m_ptr;

		/* Process the monsters */
		for (k = m_top - 1; k >= 0; k--) {
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

			if (!(m_ptr->r_ptr->extra & (1U << (o_ptr->sval - 200)))) {
				msg_print(Ind, "The golem appears indifferent to your command.");
				continue;
			}

			switch (o_ptr->sval) {
			case SV_GOLEM_ATTACK:
				if (m_ptr->mind & (1U << (o_ptr->sval - 200))) {
					msg_print(Ind, "The golem stops going for your target.");
					m_ptr->mind &= ~(1U << (o_ptr->sval - 200));
				} else {
					msg_print(Ind, "The golem approaches your target!");
					m_ptr->mind |= (1U << (o_ptr->sval - 200));
				}
				break;
			case SV_GOLEM_FOLLOW:
				if (m_ptr->mind & (1U << (o_ptr->sval - 200))) {
					msg_print(Ind, "The golem stops following you around.");
					m_ptr->mind &= ~(1U << (o_ptr->sval - 200));
				} else {
					msg_print(Ind, "The golem starts following you around!");
					m_ptr->mind |= (1U << (o_ptr->sval - 200));
				}
				break;
			case SV_GOLEM_GUARD:
				if (m_ptr->mind & (1U << (o_ptr->sval - 200))) {
					msg_print(Ind, "The golem stops being on guard.");
					m_ptr->mind &= ~(1U << (o_ptr->sval - 200));
				} else {
					msg_print(Ind, "The golem seems to be on guard now!");
					m_ptr->mind |= (1U << (o_ptr->sval - 200));
				}
				break;
			}
		}
		return;
	}

	/* add a single spell to the player's customizable tome */
	if (o_ptr->tval == TV_BOOK && is_custom_tome(o_ptr->sval)) {
		/* free space left? */
		i = 0;
		/* k_info-pval dependant */
		switch (o_ptr->bpval) {
		case 9: if (!o_ptr->xtra9) i = 1;
		case 8: if (!o_ptr->xtra8) i = 1;
		case 7: if (!o_ptr->xtra7) i = 1;
		case 6: if (!o_ptr->xtra6) i = 1;
		case 5: if (!o_ptr->xtra5) i = 1;
		case 4: if (!o_ptr->xtra4) i = 1;
		case 3: if (!o_ptr->xtra3) i = 1;
		case 2: if (!o_ptr->xtra2) i = 1;
		case 1: if (!o_ptr->xtra1) i = 1;
		default: break;
		}
		if (!i) {
			msg_print(Ind, "That book has no blank pages left!");
			return;
		}

		tome_creation(Ind);
		p_ptr->using_up_item = item; /* hack - gets swapped later */
		return;
	}

	// -------------------- artifacts -------------------- //

	if (o_ptr->name1 && o_ptr->name1 != ART_RANDART) {
		done = TRUE;
		/* Choose effect */
		switch (o_ptr->name1) {
		case ART_NARTHANC:
			msg_print(Ind, "Your dagger is covered in fire...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_NIMTHANC:
			msg_print(Ind, "Your dagger is covered in frost...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_DETHANC:
			msg_print(Ind, "Your dagger is covered in sparks...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_RILIA:
			msg_print(Ind, "Your dagger throbs deep green...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_BELANGIL:
			msg_print(Ind, "Your dagger is covered in frost...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_DAL:
			msg_print(Ind, "\377GYou feel energy flow through your feet...");
			(void)set_afraid(Ind, 0);
			(void)set_res_fear(Ind, 5);
			(void)set_poisoned(Ind, 0, 0);
			o_ptr->recharging = 5;
			break;
		case ART_RINGIL:
			msg_print(Ind, "Your sword glows an intense blue...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_ANDURIL:
			msg_print(Ind, "Your sword glows an intense red...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_FIRESTAR:
			msg_print(Ind, "Your morningstar rages in fire...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_FEANOR:
			(void)set_fast(Ind, randint(20) + 20, 15); /* removed stacking */
			o_ptr->recharging = 200 - get_skill_scale(p_ptr, SKILL_DEVICE, 150);
			break;
		case ART_THEODEN:
			msg_print(Ind, "The blade of your axe glows black...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_TURMIL:
			msg_print(Ind, "The head of your hammer glows white...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_CASPANION:
			msg_print(Ind, "Your armour glows bright red...");
			destroy_traps_doors_touch(Ind, 1);
			o_ptr->recharging = 10 - get_skill_scale(p_ptr, SKILL_DEVICE, 6);
			break;
		case ART_AVAVIR:
			msg_print(Ind, "Your scythe glows soft white...");
			set_recall(Ind, rand_int(20) + 15, o_ptr);
			o_ptr->recharging = 200 - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			break;
		case ART_TARATOL:
			(void)set_fast(Ind, randint(20) + 20, 15); /* removed stacking */
			o_ptr->recharging = rand_int(50) + 100 - get_skill_scale(p_ptr, SKILL_DEVICE, 70);
			break;
		case ART_ERIRIL:
			/* Identify and combine pack */
			(void)ident_spell(Ind);
			/* XXX Note that the artifact is always de-charged */
			o_ptr->recharging = 10 - get_skill_scale(p_ptr, SKILL_DEVICE, 8);
			break;
		case ART_RUYIWANG:
#if 0 /* ART_OLORIN */
			probing(Ind);
			detection(Ind, DEFAULT_RADIUS * 2);
			o_ptr->recharging = 20 - get_skill_scale(p_ptr, SKILL_DEVICE, 15);
#else /* ART_RUYIWANG */
			spin_attack(Ind); /* this one is nicer than do_spin */
			o_ptr->recharging = 20 + randint(5) - get_skill_scale(p_ptr, SKILL_DEVICE, 10);
#endif
			break;
		case ART_EONWE:
			msg_print(Ind, "Your axe lets out a long, shrill note...");
			(void)obliteration(Ind);
			o_ptr->recharging = 1000 - get_skill_scale(p_ptr, SKILL_DEVICE, 500);
			break;
		case ART_LOTHARANG:
			msg_print(Ind, "Your battle axe radiates deep purple...");
			hp_player(Ind, damroll(4, 8 + get_skill_scale(p_ptr, SKILL_DEVICE, 20)));
			(void)set_cut(Ind, (p_ptr->cut / 2) - 50, p_ptr->cut_attacker);
			o_ptr->recharging = rand_int(3) + 3 - get_skill_scale(p_ptr, SKILL_DEVICE, 2);//o_o
			break;
		case ART_CUBRAGOL:
			(void)brand_bolts(Ind);
			o_ptr->recharging = 999 - get_skill_scale(p_ptr, SKILL_DEVICE, 777);
			break;
		case ART_ARUNRUTH:
			msg_print(Ind, "Your sword glows a pale blue...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_AEGLOS:
			msg_print(Ind, "Your spear glows a bright white...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_OROME:
			msg_print(Ind, "Your spear pulsates...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_SOULKEEPER:
			msg_print(Ind, "Your armour glows a bright white...");
			msg_print(Ind, "\377GYou feel much better...");
			(void)hp_player(Ind, 1000);
			(void)set_cut(Ind, 0, 0);
			o_ptr->recharging = 888 - get_skill_scale(p_ptr, SKILL_DEVICE, 666);
			break;
		case ART_BELEGENNON:
			teleport_player(Ind, 10, TRUE);
			o_ptr->recharging = 2 - get_skill_scale(p_ptr, SKILL_DEVICE, 1);//>_>
			break;
		case ART_CELEBORN:
			(void)genocide(Ind);
			o_ptr->recharging = 500 - get_skill_scale(p_ptr, SKILL_DEVICE, 400);
			break;
		case ART_LUTHIEN:
			restore_level(Ind);
			o_ptr->recharging = 450 - get_skill_scale(p_ptr, SKILL_DEVICE, 350);
			break;
		case ART_ULMO:
			msg_print(Ind, "Your trident glows deep red...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_COLLUIN:
			msg_print(Ind, "Your cloak glows many colours...");
			(void)set_oppose_acid(Ind, randint(20) + 20); /* removed stacking */
			(void)set_oppose_elec(Ind, randint(20) + 20);
			(void)set_oppose_fire(Ind, randint(20) + 20);
			(void)set_oppose_cold(Ind, randint(20) + 20);
			(void)set_oppose_pois(Ind, randint(20) + 20);
			o_ptr->recharging = 111 - get_skill_scale(p_ptr, SKILL_DEVICE, 56);
			break;
		case ART_HOLCOLLETH:
			msg_print(Ind, "Your cloak glows deep blue...");
			sleep_monsters_touch(Ind);
			o_ptr->recharging = 55 - get_skill_scale(p_ptr, SKILL_DEVICE, 40);
			break;
		case ART_THINGOL:
			msg_print(Ind, "You hear a low humming noise...");
			recharge(Ind, 60 + get_skill_scale(p_ptr, SKILL_DEVICE, 40));
			o_ptr->recharging = 70 - get_skill_scale(p_ptr, SKILL_DEVICE, 50);
			break;
		case ART_COLANNON:
			teleport_player(Ind, 100, FALSE);
			o_ptr->recharging = 45 - get_skill_scale(p_ptr, SKILL_DEVICE, 35);
			break;
		case ART_TOTILA:
			msg_print(Ind, "Your flail glows in scintillating colours...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_CAMMITHRIM:
			msg_print(Ind, "Your gloves glow extremely brightly...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_PAURHACH:
			msg_print(Ind, "Your gauntlets are covered in fire...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_PAURNIMMEN:
			msg_print(Ind, "Your gauntlets are covered in frost...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_PAURAEGEN:
			msg_print(Ind, "Your gauntlets are covered in sparks...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_PAURNEN:
			msg_print(Ind, "Your gauntlets look very acidic...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_FINGOLFIN:
			msg_print(Ind, "Magical spikes appear on your cesti...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_HOLHENNETH:
			msg_print(Ind, "An image forms in your mind...");
			detection(Ind, DEFAULT_RADIUS * 2);
			o_ptr->recharging = rand_int(25) + 55 - get_skill_scale(p_ptr, SKILL_DEVICE, 40);
			break;
		case ART_GONDOR:
			msg_print(Ind, "\377GYou feel a warm tingling inside...");
			(void)hp_player(Ind, 500);
			(void)set_cut(Ind, 0, 0);
			o_ptr->recharging = 500 - get_skill_scale(p_ptr, SKILL_DEVICE, 400);
			break;
		case ART_RAZORBACK:
			msg_print(Ind, "You are surrounded by lightning!");
			sprintf(p_ptr->attacker, " casts a lightning ball for");
			for (i = 0; i < 8; i++) fire_ball(Ind, GF_ELEC, ddd[i], 150 + get_skill_scale(p_ptr, SKILL_DEVICE, 450), 3, p_ptr->attacker);
			o_ptr->recharging = 1000 - get_skill_scale(p_ptr, SKILL_DEVICE, 700);
			break;
		case ART_BLADETURNER:
			msg_print(Ind, "Your armour glows in many colours...");
			(void)hp_player(Ind, 30);
			(void)set_afraid(Ind, 0);
			(void)set_res_fear(Ind, 20);
			(void)set_shero(Ind, randint(50) + 50); /* removed stacking */
			p_ptr->blessed_power = 20;
			(void)set_blessed(Ind, randint(50) + 50); /* removed stacking */
			(void)set_oppose_acid(Ind, randint(50) + 50); /* removed stacking */
			(void)set_oppose_elec(Ind, randint(50) + 50);
			(void)set_oppose_fire(Ind, randint(50) + 50);
			(void)set_oppose_cold(Ind, randint(50) + 50);
			(void)set_oppose_pois(Ind, randint(50) + 50);
			o_ptr->recharging = 400 - get_skill_scale(p_ptr, SKILL_DEVICE, 200);
			break;
		case ART_GALADRIEL:
			msg_print(Ind, "The phial wells with clear light...");
			lite_area(Ind, damroll(2, 15 + get_skill_scale(p_ptr, SKILL_DEVICE, 50)), 3);
			if (p_ptr->suscep_lite) take_hit(Ind, damroll(50, 4), "the phial of galadriel", 0);
				if (p_ptr->suscep_lite && !p_ptr->resist_lite && !p_ptr->resist_blind) (void)set_blind(Ind, p_ptr->blind + 5 + randint(10));
			o_ptr->recharging = rand_int(10) + 10 - get_skill_scale(p_ptr, SKILL_DEVICE, 5);
			break;
		case ART_ELENDIL:
			msg_print(Ind, "The star shines brightly...");
			map_area(Ind);
			o_ptr->recharging = rand_int(25) + 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 40);
			break;
		case ART_THRAIN:
			msg_print(Ind, "The stone glows a deep green...");
			wiz_lite(Ind);
			(void)detect_sdoor(Ind, DEFAULT_RADIUS * 2);
			(void)detect_trap(Ind, DEFAULT_RADIUS * 2);
			o_ptr->recharging = rand_int(150) + 1000 - get_skill_scale(p_ptr, SKILL_DEVICE, 800);
			break;
		case ART_INGWE:
			msg_print(Ind, "An aura of good floods the area...");
			dispel_evil(Ind, p_ptr->lev * 10 + get_skill_scale(p_ptr, SKILL_DEVICE, 500));
			o_ptr->recharging = rand_int(150) + 300 - get_skill_scale(p_ptr, SKILL_DEVICE, 250);
			break;
		case ART_CARLAMMAS:
			msg_print(Ind, "The amulet lets out a shrill wail...");
			(void)set_protevil(Ind, randint(15) + 30); /* removed stacking */
			o_ptr->recharging = rand_int(125) + 225 - get_skill_scale(p_ptr, SKILL_DEVICE, 200);
			break;
		case ART_TULKAS:
			msg_print(Ind, "The ring glows brightly...");
			(void)set_fast(Ind, randint(50) + 75, 15); /* removed stacking */
			o_ptr->recharging = rand_int(100) + 150 - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			break;
		case ART_NARYA:
			msg_print(Ind, "The ring glows deep red...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_NENYA:
			msg_print(Ind, "The ring glows bright white...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_VILYA:
			msg_print(Ind, "The ring glows deep blue...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_POWER:
			msg_print(Ind, "The ring glows intensely black...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_GILGALAD:
			for (k = 1; k < 10; k++)
				if (k - 5) fire_beam(Ind, GF_LITE, k, 75 + get_skill_scale(p_ptr, SKILL_DEVICE, 150), " emits a beam of light for");
			o_ptr->recharging = rand_int(50) + 75 - get_skill_scale(p_ptr, SKILL_DEVICE, 60);
			break;
		case ART_CELEBRIMBOR:
			set_tim_esp(Ind, p_ptr->tim_esp + randint(20) + 20);
			 /* not removed stacking */
			o_ptr->recharging = rand_int(25) + 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 25);
			break;
		case ART_SKULLCLEAVER:
			destroy_area(&p_ptr->wpos, p_ptr->py, p_ptr->px, 15, TRUE, FEAT_FLOOR, 120);
			o_ptr->recharging = rand_int(200) + 200 - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			break;
		case ART_HARADRIM:
			set_afraid(Ind, 0);
			set_shero(Ind, randint(25) + 25); /* removed stacking */
			hp_player(Ind, 30);
			o_ptr->recharging = rand_int(40) + 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 35);
			break;
		case ART_FUNDIN:
			dispel_evil(Ind, p_ptr->lev * 8 + get_skill_scale(p_ptr, SKILL_DEVICE, 400));
			o_ptr->recharging = rand_int(50) + 100 - get_skill_scale(p_ptr, SKILL_DEVICE, 80);
			break;
		case ART_NAIN:
		case ART_EOL:
		case ART_UMBAR:
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_NUMENOR:
#if 0
			/* Give full knowledge */
			/* Hack -- Maximal info */
			monster_race *r_ptr;
			cave_type *c_ptr;
			int x, y, m;

			if (!tgt_pt(&x, &y)) break;

			c_ptr = &cave[y][x];
			if (!c_ptr->m_idx) break;

			r_ptr = race_inf(m_list[c_ptr->m_idx]);

 #ifdef OLD_MONSTER_LORE
			/* Observe "maximal" attacks */
			for (m = 0; m < 4; m++) {
				/* Examine "actual" blows */
				if (r_ptr->blow[m].effect || r_ptr->blow[m].method) {
					/* Hack -- maximal observations */
					r_ptr->r_blows[m] = MAX_UCHAR;
				}
			}

			/* Hack -- maximal drops // todo: correctly account for DROP_nn/DROP_n flags */
			r_ptr->r_drop_gold = r_ptr->r_drop_item =
				(((r_ptr->flags1 & (RF1_DROP_4D2)) ? 8 : 0) +
				 ((r_ptr->flags1 & (RF1_DROP_3D2)) ? 6 : 0) +
				 ((r_ptr->flags1 & (RF1_DROP_2D2)) ? 4 : 0) +
				 ((r_ptr->flags0 & (RF0_DROP_2))   ? 2 : 0) +
				 ((r_ptr->flags1 & (RF1_DROP_1D2)) ? 2 : 0) +
				 ((r_ptr->flags0 & (RF0_DROP_1))   ? 1 : 0) +
				 ((r_ptr->flags1 & (RF1_DROP_90))  ? 1 : 0) +
				 ((r_ptr->flags1 & (RF1_DROP_60))  ? 1 : 0));

			/* Hack -- but only "valid" drops */
			if (r_ptr->flags1 & (RF1_ONLY_GOLD)) r_ptr->r_drop_item = 0;
			if (r_ptr->flags1 & (RF1_ONLY_ITEM)) r_ptr->r_drop_gold = 0;

			/* Hack -- observe many spells */
			r_ptr->r_cast_innate = MAX_UCHAR;
			r_ptr->r_cast_spell = MAX_UCHAR;

			/* Hack -- know all the flags */
			r_ptr->r_flags1 = r_ptr->flags1;
			r_ptr->r_flags2 = r_ptr->flags2;
			r_ptr->r_flags3 = r_ptr->flags3;
			r_ptr->r_flags4 = r_ptr->flags4;
			r_ptr->r_flags5 = r_ptr->flags5;
			r_ptr->r_flags6 = r_ptr->flags6;
			r_ptr->r_flags7 = r_ptr->flags7;
			r_ptr->r_flags8 = r_ptr->flags8;
			r_ptr->r_flags9 = r_ptr->flags9;
 #endif

			o_ptr->recharging = rand_int(200) + 500 - get_skill_scale(p_ptr, SKILL_DEVICE, 350);
#else
			probing(Ind);
			o_ptr->recharging = rand_int(200) + 300 - get_skill_scale(p_ptr, SKILL_DEVICE, 250);
#endif
			break;
		case ART_KNOWLEDGE:
			identify_fully(Ind);
			msg_print(Ind, "\377RYou hear horrible, otherworldy sounds of the dead in your head..");
				take_sanity_hit(Ind, damroll(2, 7), "the sounds of the dead");
				//take_hit(Ind, damroll(10, 7), "the sounds of the dead", 0);
			o_ptr->recharging = rand_int(100) + 200 - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			break;
		case ART_UNDEATH:
			msg_print(Ind, "The phial wells with dark light...");
			unlite_area(Ind, damroll(2, 15 + get_skill_scale(p_ptr, SKILL_DEVICE, 50)), 3);
			take_hit(Ind, damroll(10, 10), "activating The Phial of Undeath", 0);
			if (!p_ptr->suscep_life) {
				(void)dec_stat(Ind, A_DEX, 25, STAT_DEC_PERMANENT);
				(void)dec_stat(Ind, A_WIS, 25, STAT_DEC_PERMANENT);
				(void)dec_stat(Ind, A_CON, 25, STAT_DEC_PERMANENT);
				(void)dec_stat(Ind, A_STR, 25, STAT_DEC_PERMANENT);
				(void)dec_stat(Ind, A_CHR, 25, STAT_DEC_PERMANENT);
				(void)dec_stat(Ind, A_INT, 25, STAT_DEC_PERMANENT);
			}
			o_ptr->recharging = rand_int(10) + 10 - get_skill_scale(p_ptr, SKILL_DEVICE, 5);
			break;
		case ART_HIMRING:
			(void)set_protevil(Ind, randint(15) + 30); /* removed stacking */
			o_ptr->recharging = rand_int(125) + 225 - get_skill_scale(p_ptr, SKILL_DEVICE, 150);
			break;
		case ART_FLAR:
#if 0
			/* Check for CAVE_STCK */

			msg_print(Ind, "You open a between gate. Choose a destination.");
			if (!tgt_pt(&ii,&ij)) return;
			p_ptr->energy -= 60 - plev;
			if (!cave_empty_bold(ij,ii) || (cave[ij][ii].info & CAVE_ICKY) ||
					(distance(ij,ii,py,px) > plev + 2) ||
					(!rand_int(plev * plev / 2)))
			{
				msg_print(Ind, "You fail to exit the between correctly!");
				p_ptr->energy -= 100;
				teleport_player(Ind, 10, TRUE);
			}
			else teleport_player_to(ij,ii);
			o_ptr->recharging = 100 - get_skill_scale(p_ptr, SKILL_DEVICE, 50);
#else
			/* Initiate or complete gateway creation. - C. Blue
			   If we initiated it, do not set a timeout, so we can activate
			   F'lar a second time at will to complete the spell, then timeout. */
			if (py_create_gateway(Ind) == 2) o_ptr->recharging = 1000 - get_skill_scale(p_ptr, SKILL_DEVICE, 500);
#endif
			break;
		case ART_BARAHIR:
			msg_print(Ind, "You exterminate small life.");
			(void)dispel_monsters(Ind, 100 + get_skill_scale(p_ptr, SKILL_DEVICE, 300));
			o_ptr->recharging = rand_int(55) + 55 - get_skill_scale(p_ptr, SKILL_DEVICE, 40);
			break;
		/* The Stone of Lore is perilous, for the sake of game balance. */
		case ART_STONE_LORE:
			msg_print(Ind, "The stone reveals hidden mysteries...");
			if (!ident_spell(Ind)) return;
			//if (!p_ptr->realm1)
			if (1) {
				/* Sufficient mana */
				if (20 <= p_ptr->csp) {
					/* Use some mana */
					p_ptr->csp -= 20;

					/* Confusing. */
					if (rand_int(5) == 0) (void)set_confused(Ind, p_ptr->confused + randint(5));
				}

				/* Over-exert the player */
				else {
					/* No mana left */
					p_ptr->csp = 0;
					p_ptr->csp_frac = 0;

					/* Message */
					msg_print(Ind, "You do not have enough mana to control the stone!");

					/* Confusing. */
					(void)set_confused(Ind, p_ptr->confused + 5 + randint(5));
				}

				/* Redraw mana */
				p_ptr->redraw |= (PR_MANA);
			}

			/* Exercise a little care... */
			//if (rand_int(20) == 0) take_hit(Ind, damroll(4, 10), "perilous secrets", 0); else
			take_hit(Ind, damroll(1, 12), "perilous secrets", 0);

			o_ptr->recharging = 10 - get_skill_scale(p_ptr, SKILL_DEVICE, 6);
			break;
		case ART_MEDIATOR:
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_DOR:
		case ART_GORLIM:
			turn_monsters(Ind, 40 + p_ptr->lev);
			o_ptr->recharging = 3 * (p_ptr->lev + 10);
			break;
		case ART_ANGUIREL:
			switch (randint(13)) {
				case 1: case 2: case 3: case 4: case 5:
					teleport_player(Ind, 10, TRUE);
					break;
				case 6: case 7: case 8: case 9: case 10:
					teleport_player(Ind, 222, FALSE);
					break;
				case 11: case 12:
				default:
					(void)stair_creation(Ind);
					break;
			}
			o_ptr->recharging = 35 - get_skill_scale(p_ptr, SKILL_DEVICE, 20);
			break;
		case ART_ERU:
			msg_print(Ind, "Your sword glows an intense white...");
			hp_player(Ind, 1000);
			heal_insanity(Ind, 50);
			set_blind(Ind, 0);
			set_poisoned(Ind, 0, 0);
			set_confused(Ind, 0);
			set_stun(Ind, 0);
			set_cut(Ind, 0, 0);
			set_image(Ind, 0);
			o_ptr->recharging = 500 - get_skill_scale(p_ptr, SKILL_DEVICE, 250);
			break;
		case ART_DAWN:
#if 0 /* needs pet code */
			msg_print(Ind, "You summon the Legion of the Dawn.");
			(void)summon_specific_friendly(py, px, dlev, SUMMON_DAWN, TRUE, 0);
 #ifdef USE_SOUND_2010
			sound_near_site(py, px, wpos, 0, "summon", NULL, SFX_TYPE_COMMAND, FALSE);
 #endif
#else
			do_banish_undead(Ind, 100 + get_skill_scale(p_ptr, SKILL_DEVICE, 30));
#endif
			o_ptr->recharging = 500 + randint(100) - get_skill_scale(p_ptr, SKILL_DEVICE, 400);
			break;
		case ART_EVENSTAR:
			restore_level(Ind);
			(void)do_res_stat(Ind, A_STR);
			(void)do_res_stat(Ind, A_DEX);
			(void)do_res_stat(Ind, A_CON);
			(void)do_res_stat(Ind, A_INT);
			(void)do_res_stat(Ind, A_WIS);
			(void)do_res_stat(Ind, A_CHR);
			o_ptr->recharging = 150 - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			break;
#if 1
		case ART_ELESSAR:
			if (p_ptr->black_breath)
				msg_print(Ind, "The hold of the Black Breath on you is broken!");
			p_ptr->black_breath = FALSE;
			hp_player(Ind, 100);
			o_ptr->recharging = 200 - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			break;
#endif
		case ART_GANDALF:
			msg_print(Ind, "Your mage staff glows deep blue...");
			if (p_ptr->csp < p_ptr->msp
#ifdef MARTYR_NO_MANA
			     && !p_ptr->martyr
#endif
			     ) {
				p_ptr->csp = p_ptr->msp;
				p_ptr->csp_frac = 0;
				msg_print(Ind, "You feel your head clearing.");
				p_ptr->redraw |= (PR_MANA);
				p_ptr->window |= (PW_PLAYER);
			}
			o_ptr->recharging = 666 - get_skill_scale(p_ptr, SKILL_DEVICE, 444);
			break;
		case ART_MARDRA:
#if 0	// needing pet code
			if (randint(3) == 1) {
				if (summon_specific(py, px, ((plev * 3) / 2), SUMMON_DRAGONRIDER, 0, 1)) {
					msg_print(Ind, "A DragonRider comes from the BETWEEN !");
					msg_print(Ind, "'I will burn you!'");
 #ifdef USE_SOUND_2010
					sound_near_site(py, px, wpos, 0, "summon", NULL, SFX_TYPE_COMMAND, FALSE);
 #endif
				}
			} else {
				if (summon_specific_friendly(py, px, ((plev * 3) / 2),
				    SUMMON_DRAGONRIDER, (bool)(plev == 50 ? TRUE : FALSE))) {
					msg_print(Ind, "A DragonRider comes from the BETWEEN !");
					msg_print(Ind, "'I will help you in your difficult task.'");
 #ifdef USE_SOUND_2010
					sound_near_site(py, px, wpos, 0, "summon", NULL, SFX_TYPE_COMMAND, FALSE);
 #endif
				}
			}
#else
			do_banish_dragons(Ind, 100 + get_skill_scale(p_ptr, SKILL_DEVICE, 30));
#endif
			o_ptr->recharging = 1000 - get_skill_scale(p_ptr, SKILL_DEVICE, 500);
			break;
		case ART_PALANTIR_ITHIL:
		case ART_PALANTIR:
			msg_print(Ind, "The stone glows a deep green...");
			wiz_lite_extra(Ind);
			(void)detect_trap(Ind, DEFAULT_RADIUS * 2);
			(void)detect_sdoor(Ind, DEFAULT_RADIUS * 2);
			//(void)detect_stair(Ind);
			o_ptr->recharging = rand_int(150) + 1000 - get_skill_scale(p_ptr, SKILL_DEVICE, 800);
			break;
#if 0	// Instruments
		case ART_ROBINTON:
			msg_format(Ind, "Your instrument starts %s",music_info[3].desc);
			p_ptr->music = 3; /* Full ID */
			o_ptr->recharging = music_info[p_ptr->music].init_recharge;
			break;
		case ART_PIEMUR:
			msg_format(Ind, "Your instrument starts %s",music_info[9].desc);
			p_ptr->music = 9;
			o_ptr->recharging = music_info[p_ptr->music].init_recharge;
			break;
		case ART_MENOLLY:
			msg_format(Ind, "Your instrument starts %s",music_info[10].desc);
			p_ptr->music = 10;
			o_ptr->recharging = music_info[p_ptr->music].init_recharge;
			break;
#endif
		case ART_EREBOR:
			msg_print(Ind, "Your pick twists in your hands.");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
#if 0
		case ART_DRUEDAIN:
			msg_print(Ind, "Your drum shows you the world.");
			detect_all();
			o_ptr->recharging = 99 - get_skill_scale(p_ptr, SKILL_DEVICE, 50);
			break;
		case ART_ROHAN:
			msg_print(Ind, "Your horn glows deep red.");
			set_afraid(0);
			set_shero(Ind, damroll(5,10) + 30); /* removed stacking */
			set_afraid(0);
			set_hero(Ind, damroll(5,10) + 30); /* removed stacking */
			set_fast(Ind, damroll(5,10) + 30, 20); /* removed stacking */
			hp_player(30);
			o_ptr->recharging = 250 + randint(50) - get_skill_scale(p_ptr, SKILL_DEVICE, 150);
			break;
		case ART_HELM:
			msg_print(Ind, "Your horn emits a loud sound.");
			if (!get_aim_dir(Ind)) return;
			sprintf(p_ptr->attacker, "'s horn emits a loud sound for");
			fire_ball(GF_SOUND, dir, 300 + get_skill_scale(p_ptr, SKILL_DEVICE, 300), 6, p_ptr->attacker);
			o_ptr->recharging = 300;
			break;
		case ART_BOROMIR:
			msg_print(Ind, "Your horn calls for help.");
			for(i = 0; i < 15; i++)
				summon_specific_friendly(py, px, ((plev * 3) / 2), SUMMON_HUMAN, TRUE);
 #ifdef USE_SOUND_2010
			sound_near_site(py, px, wpos, 0, "summon", NULL, SFX_TYPE_COMMAND, FALSE);
 #endif
			o_ptr->recharging = 1000 - get_skill_scale(p_ptr, SKILL_DEVICE, 500);
			break;
#endif	// 0
		case ART_HURIN:
			(void)set_fast(Ind, randint(50) + 50, 10); /* removed stacking */
			hp_player(Ind, 30);
			set_afraid(Ind, 0);
			set_shero(Ind, randint(50) + 50); /* removed stacking */
			o_ptr->recharging = rand_int(75) + 175 - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			break;
		case ART_AXE_GOTHMOG:
			msg_print(Ind, "Your lochaber axe erupts in fire...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		case ART_MELKOR:
			msg_print(Ind, "Your spear is covered of darkness...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
#if 0 /* not suited for multiplayer, and it's like an amulet of life-saving anyway */
		case ART_GROND:
			msg_print(Ind, "Your hammer hits the floor...");
			alter_reality();
			o_ptr->recharging = 20000;
			break;
#endif
		case ART_NATUREBANE:
			msg_print(Ind, "Your axe glows blood red...");
			//dispel_monsters(500 + get_skill_scale(p_ptr, SKILL_DEVICE, 500));
			do_banish_animals(Ind, 80);
			o_ptr->recharging = 200 + randint(200) - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			break;
		case ART_NIGHT:
			msg_print(Ind, "Your axe emits a black aura...");
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
#if 1
		case ART_ORCHAST:
			msg_print(Ind, "Your weapon glows brightly...");
			(void)detect_creatures_xxx(Ind, RF3_ORC);
			o_ptr->recharging = 10;
			break;
#endif	// 0
		/* ToNE-NET additions */
		case ART_BILBO:
			msg_print(Ind, "Your picklock flashes...");
			destroy_traps_touch(Ind, 1);
			o_ptr->recharging = 30 + randint(10) - get_skill_scale(p_ptr, SKILL_DEVICE, 25);
			break;
		case ART_SOULCURE:
			if (p_ptr->blessed_power <= 20) {
				msg_print(Ind, "Your gloves glow golden...");
				p_ptr->blessed_power = 20;
				set_blessed(Ind, randint(48) + 24); /* removed stacking */
				o_ptr->recharging = 150 + randint(100) - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			} else {
				msg_print(Ind, "Your gloves shimmer..");
			}
			break;
		case ART_AMUGROM:
			msg_print(Ind, "The amulet sparkles in scintillating colours...");
			o_ptr->recharging = 150 + randint(50) - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			(void)set_oppose_acid(Ind, randint(30) + 40); /* removed stacking */
			(void)set_oppose_elec(Ind, randint(30) + 40);
			(void)set_oppose_fire(Ind, randint(30) + 40);
			(void)set_oppose_cold(Ind, randint(30) + 40);
			(void)set_oppose_pois(Ind, randint(30) + 40);
			break;
		case ART_SPIRITSHARD:
			msg_print(Ind, "Shimmers and flashes travel over the surface of the amulet...");
			o_ptr->recharging = 100 + randint(50) - get_skill_scale(p_ptr, SKILL_DEVICE, 50);
			(void)set_tim_wraith(Ind, 50 + rand_int(11));
			break;
		case ART_PHASING:
			msg_print(Ind, "Your surroundings fade.. you are carried away through a tunnel of light!");
			//msg_print(Ind, "You hear a voice, saying 'Sorry, not yet implemented!'");
			o_ptr->recharging = 1000;
			p_ptr->auto_transport = AT_VALINOR;
			//p_ptr->paralyzed = 1; /* Paranoia? In case there is a timing glitch, allowing to drop the Ring of Phasing before arriving in Valinor ;) */
			break;
		case ART_LEBOHAUM:
			msg_print(Ind, "\377wYou hear a little song in your head in some unknown tongue:");
			msg_print(Ind, "\377u ~'Avec le casque Lebohaum y a jamais d'anicroches, je parcours les dongeons,~");
			msg_print(Ind, "\377u ~j'en prend plein la caboche. Avec le casque Lebohaum, tout ces monstres a la~");
			msg_print(Ind, "\377u ~con, je leur met bien profond: c'est moi le maitre du dongeon!~'");
			o_ptr->recharging = 30;
			break;
		case ART_SMASHER:
			if (destroy_doors_touch(Ind, 1)) {
#ifdef USE_SOUND_2010
				sound(Ind, "bash_door_break", NULL, SFX_TYPE_COMMAND, TRUE);
#endif
			}
			o_ptr->recharging = 15 + randint(3) - get_skill_scale(p_ptr, SKILL_DEVICE, 5);
			break;
		default: done = FALSE;
		}

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		if (o_ptr->recharging) return;
	}

	// -------------------- ego items -------------------- //

	if (!done) {
		if (is_ego_p(o_ptr, EGO_DRAGON)) {
			teleport_player(Ind, 100, FALSE);
			o_ptr->recharging = 50 + randint(35) - get_skill_scale(p_ptr, SKILL_DEVICE, 35);
			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
			/* Done */
			return;
		} else if (is_ego_p(o_ptr, EGO_JUMP)) {
			teleport_player(Ind, 10, TRUE);
			o_ptr->recharging = 15 + randint(10) - get_skill_scale(p_ptr, SKILL_DEVICE, 10);
			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
			/* Done */
			return;
		} else if (is_ego_p(o_ptr, EGO_SPINNING)) {
			//do_spin(Ind);
			spin_attack(Ind); /* this one is nicer than do_spin */
			o_ptr->recharging = 50 + randint(25) - get_skill_scale(p_ptr, SKILL_DEVICE, 35);
			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
			/* Done */
			return;
		} else if (is_ego_p(o_ptr, EGO_FURY)) {
			set_afraid(Ind, 0);
			set_fury(Ind, rand_int(5) + 15); /* removed stacking */
			hp_player(Ind, 40);
			o_ptr->recharging = 100 + randint(50) - get_skill_scale(p_ptr, SKILL_DEVICE, 50);
			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
			/* Done */
			return;
		} else if (is_ego_p(o_ptr, EGO_NOLDOR)) {
			detect_treasure(Ind, DEFAULT_RADIUS * 2);
			o_ptr->recharging = 20 + randint(10) - get_skill_scale(p_ptr, SKILL_DEVICE, 15);
			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
			/* Done */
			return;
		} else if (is_ego_p(o_ptr, EGO_SPECTRAL)) {
			//set_shadow(Ind, 20 + randint(20));
			set_tim_wraith(Ind, 15 + randint(10));
			o_ptr->recharging = 50 + randint(50) - get_skill_scale(p_ptr, SKILL_DEVICE, 25);
			/* Window stuff */
			p_ptr->window |= PW_INVEN | PW_EQUIP;
			/* Done */
			return;
		} else if (is_ego_p(o_ptr, EGO_CLOAK_LORDLY_RES)) {
			msg_print(Ind, "Your cloak flashes many colors...");
			(void)set_oppose_acid(Ind, randint(20) + 50); /* removed stacking */
			(void)set_oppose_elec(Ind, randint(20) + 50);
			(void)set_oppose_fire(Ind, randint(20) + 50);
			(void)set_oppose_cold(Ind, randint(20) + 50);
			(void)set_oppose_pois(Ind, randint(20) + 50);
			o_ptr->recharging = rand_int(40) + 150 - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
			return;
		} else if (is_ego_p(o_ptr, EGO_AURA_FIRE2)) {
		//else if (is_ego_p(o_ptr, EGO_AURA_FIRE) || is_ego_p(o_ptr, EGO_AURA_FIRE2)) {
			msg_print(Ind, "Your cloak flashes in flames...");
			(void)set_oppose_fire(Ind, randint(40) + 40);
			o_ptr->recharging = rand_int(40) + 150 - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
			return;
		} else if (is_ego_p(o_ptr, EGO_AURA_ELEC2)) {
		//else if (is_ego_p(o_ptr, EGO_AURA_ELEC) || is_ego_p(o_ptr, EGO_AURA_ELEC2)) {
			msg_print(Ind, "Your cloak sparkles with lightning...");
			(void)set_oppose_elec(Ind, randint(40) + 40);
			o_ptr->recharging = rand_int(40) + 150 - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
			return;
		} else if (is_ego_p(o_ptr, EGO_AURA_COLD2)) {
		//else if (is_ego_p(o_ptr, EGO_AURA_COLD) || is_ego_p(o_ptr, EGO_AURA_COLD2)) {
			msg_print(Ind, "Your cloak shines with frost...");
			(void)set_oppose_cold(Ind, randint(40) + 40);
			o_ptr->recharging = rand_int(40) + 150 - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
			return;
		}
	}

	// -------------------- base items -------------------- //

	/* Hack -- Dragon Scale Mail can be activated as well */
	/* Yikes, hard-coded r_idx.. */
	if (!done && o_ptr->tval == TV_DRAG_ARMOR && item == INVEN_BODY) {
		/* Breath activation */
		p_ptr->current_activation = item;
		get_aim_dir(Ind);
		return;
	}

	/* Hack -- Amulet of the Serpents can be activated as well */
	if (!done && o_ptr->tval == TV_AMULET) {
		switch (o_ptr->sval) {
		case SV_AMULET_SERPENT:
			/* Get a direction for breathing (or abort) */
			p_ptr->current_activation = item;
			get_aim_dir(Ind);
			return;
		/* Amulets of the moon can be activated for sleep monster */
		case SV_AMULET_THE_MOON:
			msg_print(Ind, "Your amulet glows a deep blue...");
			sleep_monsters(Ind, 20 + p_ptr->lev + get_skill_scale(p_ptr, SKILL_DEVICE, 80));
			o_ptr->recharging = rand_int(100) + 100;
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
			return;
		/* Amulets of rage can be activated for berserk strength */
		case SV_AMULET_RAGE:
			msg_print(Ind, "Your amulet sparkles bright red...");
			set_afraid(Ind, 0);
			set_fury(Ind, randint(10) + 15); /* removed stacking */
			hp_player(Ind, 40);
			o_ptr->recharging = rand_int(150) + 250 - get_skill_scale(p_ptr, SKILL_DEVICE, 150);
			p_ptr->window |= (PW_INVEN | PW_EQUIP);
			return;
		}
	}

	if (!done && o_ptr->tval == TV_RING) {
		switch (o_ptr->sval) {
			case SV_RING_ELEC:
			case SV_RING_ACID:
			case SV_RING_ICE:
			case SV_RING_FLAMES:
				/* Get a direction for breathing (or abort) */
				p_ptr->current_activation = item;
				get_aim_dir(Ind);
				return;
			/* Yes, this can be activated but at the cost of it's destruction */
			case SV_RING_TELEPORTATION:
				//if (!get_check("This will destroy the ring, do you want to continue ?")) break;
				msg_print(Ind, "The ring explode into a space distorsion.");
				teleport_player(Ind, 200, FALSE);

				/* It explodes, doesnt it ? */
				take_hit(Ind, damroll(2, 10), "an exploding ring", 0);

				inven_item_increase(Ind, item, -255);
				inven_item_optimize(Ind, item);
				break;
			case SV_RING_POLYMORPH:
				if (!(item == INVEN_LEFT || item == INVEN_RIGHT)){
					msg_print(Ind, "You must be wearing the ring!");
					return;
				}
				if (!get_skill(p_ptr, SKILL_MIMIC) ||
				    (p_ptr->pclass == CLASS_DRUID) ||
				    (p_ptr->prace == RACE_VAMPIRE) ||
				    (p_ptr->pclass == CLASS_SHAMAN && !mimic_shaman(o_ptr->pval))) {
					msg_print(Ind, "The ring starts to glow brightly, then fades again");
					return;
				}

				/* If never used before, then set to the player form,
				 * otherwise set the player form*/
				if (!o_ptr->pval) {
					if ((p_ptr->r_mimicry[p_ptr->body_monster] < r_info[p_ptr->body_monster].level) ||
					    (get_skill_scale(p_ptr, SKILL_MIMIC, 100) < r_info[p_ptr->body_monster].level))
						msg_print(Ind, "Nothing happens");
					else if (r_info[p_ptr->body_monster].level == 0)
						msg_print(Ind, "The ring starts to glow brightly, then fades again");
					else {
						msg_format(Ind, "The form of the ring seems to change to a small %s.", r_info[p_ptr->body_monster].name + r_name);
						o_ptr->pval = p_ptr->body_monster;

						/* Set appropriate level requirements */
						o_ptr->level = ring_of_polymorph_level(r_info[p_ptr->body_monster].level);

						/* Make the ring last only over a certain period of time >:) - C. Blue */
						o_ptr->timeout_magic = 3000 + get_skill_scale(p_ptr, SKILL_DEVICE, 2000) +
								rand_int(3001 - get_skill_scale(p_ptr, SKILL_DEVICE, 2000));

						msg_print(Ind, "Your knowledge is absorbed by the ring!");
						p_ptr->r_mimicry[p_ptr->body_monster] = 0;

						do_mimic_change(Ind, 0, TRUE);

						object_aware(Ind, o_ptr);
						object_known(o_ptr);

						/* log it */
						s_printf("POLYRING_CREATE: %s -> %s (%d/%d, %d).\n",
						    p_ptr->name, r_info[o_ptr->pval].name + r_name,
						    o_ptr->level, r_info[o_ptr->pval].level,
						    o_ptr->timeout_magic);
					}
				}
				/* activate the ring to change into its form! */
				else {
					/* Need skill; no need of killing count */
					if (r_info[o_ptr->pval].level > get_skill_scale(p_ptr, SKILL_MIMIC, 100)) {
						msg_print(Ind, "Your mimicry is not powerful enough yet.");
						return;
					}

#if POLY_RING_METHOD == 0
					/* Poly first, break then :) */
					/* reversed again since you need need to keep wearing the ring
					   or you will polymorph back */
					/* Take toll ('overhead energy') for activating */
					if (o_ptr->timeout_magic >= 1000) o_ptr->timeout_magic -= 500; /* 500 are approx. 5 minutes */
					else if (o_ptr->timeout_magic > 1) o_ptr->timeout_magic /= 2;

					do_mimic_change(Ind, o_ptr->pval, TRUE);

					/* log it */
					s_printf("POLYRING_ACTIVATE_0: %s -> %s (%d/%d, %d).\n",
					    p_ptr->name, r_info[o_ptr->pval].name + r_name,
					    o_ptr->level, r_info[o_ptr->pval].level,
					    o_ptr->timeout_magic);
#endif

#if POLY_RING_METHOD == 1
					msg_print(Ind, "\377yThe ring disintegrates, releasing a powerful magic wave!");
					do_mimic_change(Ind, o_ptr->pval, TRUE);
					p_ptr->tim_mimic = o_ptr->timeout_magic;
					p_ptr->tim_mimic_what = o_ptr->pval;

					/* log it */
					s_printf("POLYRING_ACTIVATE_1: %s -> %s (%d/%d, %d).\n",
					    p_ptr->name, r_info[o_ptr->pval].name + r_name,
					    o_ptr->level, r_info[o_ptr->pval].level,
					    o_ptr->timeout_magic);

					inven_item_increase(Ind, item, -1);
					inven_item_optimize(Ind, item);
#endif

				}
				break;
		}

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

#ifdef ENABLE_XID_MDEV
 #ifdef XID_REPEAT
		if (rep) p_ptr->current_item = -1;
 #endif
#endif

		/* Success */
		return;

	}

	/* Mistake */
	msg_print(Ind, "That object cannot be activated.");
}


void do_cmd_activate_dir(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	int item;
	bool done = FALSE;

	item = p_ptr->current_activation;

	if (p_ptr->anti_magic) {
		msg_format(Ind, "\377%cYour anti-magic shell disrupts your attempt.", COLOUR_AM_OWN);
		return;
	}
	if (get_skill(p_ptr, SKILL_ANTIMAGIC)) {
		msg_format(Ind, "\377%cYou don't believe in magic.", COLOUR_AM_OWN);
		return;
	}
	if (magik((p_ptr->antimagic * 8) / 5)) {
#ifdef USE_SOUND_2010
		sound(Ind, "am_field", NULL, SFX_TYPE_MISC, FALSE);
#endif
		msg_format(Ind, "\377%cYour anti-magic field disrupts your attempt.", COLOUR_AM_OWN);
		return;
	}

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &p_ptr->inventory[item];
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max) return; /* item doesn't exist */
		o_ptr = &o_list[0 - item];
	}

	/* If the item can be equipped, it MUST be equipped to be activated */
	if ((item < INVEN_WIELD) && wearable_p(o_ptr)) {
		msg_print(Ind, "You must be using this item to activate it.");
		return;
	}

	if (check_guard_inscription(o_ptr->note, 'A')) {
		msg_print(Ind, "The item's inscription prevents it.");
		return;
	};

	if (!can_use_verbose(Ind, o_ptr)) return;

	break_cloaking(Ind, 0);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);

	// -------------------- artifacts -------------------- //

	/* Artifacts activate by name */
	if (o_ptr->name1 && o_ptr->name1 != ART_RANDART) {
		done = TRUE;
		switch (o_ptr->name1) {
		case ART_NARTHANC:
			sprintf(p_ptr->attacker, " fires a fire bolt for");
			fire_bolt(Ind, GF_FIRE, dir, damroll(9 + get_skill_scale(p_ptr, SKILL_DEVICE, 20), 8), p_ptr->attacker);
			o_ptr->recharging = rand_int(4) + 8 - get_skill_scale(p_ptr, SKILL_DEVICE, 6);
			break;
		case ART_NIMTHANC:
			sprintf(p_ptr->attacker, " fires a frost bolt for");
			fire_bolt(Ind, GF_COLD, dir, damroll(6 + get_skill_scale(p_ptr, SKILL_DEVICE, 20), 8), p_ptr->attacker);
			o_ptr->recharging = rand_int(3) + 7 - get_skill_scale(p_ptr, SKILL_DEVICE, 5);
			break;
		case ART_DETHANC:
			sprintf(p_ptr->attacker, " fires a lightning bolt for");
			fire_bolt(Ind, GF_ELEC, dir, damroll(4 + get_skill_scale(p_ptr, SKILL_DEVICE, 20), 8), p_ptr->attacker);
			o_ptr->recharging = rand_int(3) + 6 - get_skill_scale(p_ptr, SKILL_DEVICE, 4);
			break;
		case ART_RILIA:
			sprintf(p_ptr->attacker, " casts a stinking cloud for");
			//fire_ball(Ind, GF_POIS, dir, 12 + get_skill_scale(p_ptr, SKILL_DEVICE, 20), 3, p_ptr->attacker);
			fire_cloud(Ind, GF_POIS, dir, 4 + get_skill_scale_fine(p_ptr, SKILL_DEVICE, 7), 3, 4, 9, p_ptr->attacker);
			o_ptr->recharging = rand_int(2) + 4 - get_skill_scale(p_ptr, SKILL_DEVICE, 2);
			break;
		case ART_BELANGIL:
			sprintf(p_ptr->attacker, " casts a cold ball for");
			fire_ball(Ind, GF_COLD, dir, 48 + get_skill_scale(p_ptr, SKILL_DEVICE, 60), 2, p_ptr->attacker);
			o_ptr->recharging = rand_int(2) + 5 - get_skill_scale(p_ptr, SKILL_DEVICE, 3);
			break;
		case ART_RINGIL:
			sprintf(p_ptr->attacker, " casts a cold ball for");
			fire_ball(Ind, GF_COLD, dir, 100 + get_skill_scale(p_ptr, SKILL_DEVICE, 300), 2, p_ptr->attacker);
			o_ptr->recharging = 300 - get_skill_scale(p_ptr, SKILL_DEVICE, 225);
			break;
		case ART_ANDURIL:
			sprintf(p_ptr->attacker, " casts a fire ball for");
			fire_ball(Ind, GF_FIRE, dir, 72 + get_skill_scale(p_ptr, SKILL_DEVICE, 150), 2, p_ptr->attacker);
			o_ptr->recharging = 400 - get_skill_scale(p_ptr, SKILL_DEVICE, 300);
			break;
		case ART_FIRESTAR:
			sprintf(p_ptr->attacker, " casts a fire ball for");
			fire_ball(Ind, GF_FIRE, dir, 72 + get_skill_scale(p_ptr, SKILL_DEVICE, 150), 3, p_ptr->attacker);
			o_ptr->recharging = 100 - get_skill_scale(p_ptr, SKILL_DEVICE, 75);
			break;
		case ART_THEODEN:
			if (drain_life(Ind, dir, 25))
				hp_player(Ind, p_ptr->ret_dam / 3);
			p_ptr->ret_dam = 0;
			o_ptr->recharging = 400 - get_skill_scale(p_ptr, SKILL_DEVICE, 300);
			break;
		case ART_TURMIL:
			if (drain_life(Ind, dir, 15))
				hp_player(Ind, p_ptr->ret_dam / 2);
			p_ptr->ret_dam = 0;
			o_ptr->recharging = 70 - get_skill_scale(p_ptr, SKILL_DEVICE, 50);
			break;
		case ART_ARUNRUTH:
			sprintf(p_ptr->attacker, " fires a frost bolt for");
			fire_bolt(Ind, GF_COLD, dir, damroll(12 + get_skill_scale(p_ptr, SKILL_DEVICE, 15), 8), p_ptr->attacker);
			o_ptr->recharging = 500 - get_skill_scale(p_ptr, SKILL_DEVICE, 400);
			break;
		case ART_AEGLOS:
			sprintf(p_ptr->attacker, " casts a cold ball for");
			fire_ball(Ind, GF_COLD, dir, 100 + get_skill_scale(p_ptr, SKILL_DEVICE, 150), 2, p_ptr->attacker);
			o_ptr->recharging = 500 - get_skill_scale(p_ptr, SKILL_DEVICE, 400);
			break;
		case ART_OROME:
			wall_to_mud(Ind, dir);
			o_ptr->recharging = 5 - get_skill_scale(p_ptr, SKILL_DEVICE, 3);
			break;
		case ART_ULMO:
			teleport_monster(Ind, dir);
			o_ptr->recharging = 150 - get_skill_scale(p_ptr, SKILL_DEVICE, 100);
			break;
		case ART_TOTILA:
			confuse_monster(Ind, dir, 10 + p_ptr->lev + get_skill_scale(p_ptr, SKILL_DEVICE, 50));
			o_ptr->recharging = 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 40);
			break;
		case ART_CAMMITHRIM:
			sprintf(p_ptr->attacker, " fires a missile for");
			fire_bolt(Ind, GF_MISSILE, dir, damroll(2 + get_skill_scale(p_ptr, SKILL_DEVICE, 8), 6), p_ptr->attacker);
			o_ptr->recharging = 2 - get_skill_scale(p_ptr, SKILL_DEVICE, 1);
			break;
		case ART_PAURHACH:
			sprintf(p_ptr->attacker, " fires a fire bolt for");
			fire_bolt(Ind, GF_FIRE, dir, damroll(9 + get_skill_scale(p_ptr, SKILL_DEVICE, 20), 8), p_ptr->attacker);
			o_ptr->recharging = rand_int(4) + 8 - get_skill_scale(p_ptr, SKILL_DEVICE, 6);
			break;
		case ART_PAURNIMMEN:
			sprintf(p_ptr->attacker, " fires a frost bolt for");
			fire_bolt(Ind, GF_COLD, dir, damroll(6 + get_skill_scale(p_ptr, SKILL_DEVICE, 20), 8), p_ptr->attacker);
			o_ptr->recharging = rand_int(3) + 7 - get_skill_scale(p_ptr, SKILL_DEVICE, 5);
			break;
		case ART_PAURAEGEN:
			sprintf(p_ptr->attacker, " fires a lightning bolt for");
			fire_bolt(Ind, GF_ELEC, dir, damroll(4 + get_skill_scale(p_ptr, SKILL_DEVICE, 20), 8), p_ptr->attacker);
			o_ptr->recharging = rand_int(3) + 6 - get_skill_scale(p_ptr, SKILL_DEVICE, 4);
			break;
		case ART_PAURNEN:
			sprintf(p_ptr->attacker, " fires an acid bolt for");
			fire_bolt(Ind, GF_ACID, dir, damroll(5 + get_skill_scale(p_ptr, SKILL_DEVICE, 20), 8), p_ptr->attacker);
			o_ptr->recharging = rand_int(2) + 5 - get_skill_scale(p_ptr, SKILL_DEVICE, 3);
			break;
		case ART_FINGOLFIN:
			sprintf(p_ptr->attacker, " fires an arrow for");
			fire_bolt(Ind, GF_ARROW, dir, 150, p_ptr->attacker);
			o_ptr->recharging = rand_int(30) + 90 - get_skill_scale(p_ptr, SKILL_DEVICE, 75);
			break;
		case ART_NARYA:
			sprintf(p_ptr->attacker, " casts a fire ball for");
			fire_ball(Ind, GF_FIRE, dir, 120 + get_skill_scale(p_ptr, SKILL_DEVICE, 250), 3, p_ptr->attacker);
			o_ptr->recharging = rand_int(75) + 225 - get_skill_scale(p_ptr, SKILL_DEVICE, 150);
			break;
		case ART_NENYA:
			sprintf(p_ptr->attacker, " casts a cold ball for");
			fire_ball(Ind, GF_COLD, dir, 200 + get_skill_scale(p_ptr, SKILL_DEVICE, 250), 3, p_ptr->attacker);
			o_ptr->recharging = rand_int(125) + 325 - get_skill_scale(p_ptr, SKILL_DEVICE, 225);
			break;
		case ART_VILYA:
			sprintf(p_ptr->attacker, " casts a lightning ball for");
			fire_ball(Ind, GF_ELEC, dir, 250 + get_skill_scale(p_ptr, SKILL_DEVICE, 250), 3, p_ptr->attacker);
			o_ptr->recharging = rand_int(175) + 425 - get_skill_scale(p_ptr, SKILL_DEVICE, 325);
			break;
		case ART_POWER:
			ring_of_power(Ind, dir);
			o_ptr->recharging = rand_int(450) + 450;
			break;
		case ART_MEDIATOR:
			msg_print(Ind, "You breathe the elements.");
			sprintf(p_ptr->attacker, " breathes the elements for");
			fire_ball(Ind, GF_MISSILE, dir, 300 + get_skill_scale(p_ptr, SKILL_DEVICE, 300), 4, p_ptr->attacker);
			msg_print(Ind, "Your armour glows in many colours...");
			(void)set_afraid(Ind, 0);
			(void)set_shero(Ind, randint(50) + 50); /* removed stacking */
			(void)hp_player(Ind, 30);
			p_ptr->blessed_power = 20;
			(void)set_blessed(Ind, randint(50) + 50); /* removed stacking */
			(void)set_oppose_acid(Ind, randint(50) + 50); /* removed stacking */
			(void)set_oppose_elec(Ind, randint(50) + 50);
			(void)set_oppose_fire(Ind, randint(50) + 50);
			(void)set_oppose_cold(Ind, randint(50) + 50);
			(void)set_oppose_pois(Ind, randint(50) + 50);
			o_ptr->recharging = 400 - get_skill_scale(p_ptr, SKILL_DEVICE, 300);
			break;
		case ART_EREBOR:
			if (do_prob_travel(Ind, dir)) {
				msg_print(Ind, "You found a passage!");
				o_ptr->recharging = 200 - get_skill_scale(p_ptr, SKILL_DEVICE, 150);
			} else msg_print(Ind, "Nothing happens!");
			break;
		case ART_AXE_GOTHMOG:
			sprintf(p_ptr->attacker, " casts a fireball for");
			fire_ball(Ind, GF_FIRE, dir, 300 + get_skill_scale(p_ptr, SKILL_DEVICE, 300), 4, p_ptr->attacker);
			o_ptr->recharging = 200 + rand_int(200) - get_skill_scale(p_ptr, SKILL_DEVICE, 150);
			break;
		case ART_MELKOR:
			sprintf(p_ptr->attacker, " casts a darkness storm for");
			fire_ball(Ind, GF_DARK, dir, 150 + get_skill_scale(p_ptr, SKILL_DEVICE, 150), 3, p_ptr->attacker);
			o_ptr->recharging = 100 - get_skill_scale(p_ptr, SKILL_DEVICE, 80);
			break;
		case ART_NIGHT:
		{
			int i;
			for (i = 0; i < 3; i++) {
				if (drain_life(Ind, dir, 7))
					hp_player(Ind, p_ptr->ret_dam / 2);
				p_ptr->ret_dam = 0;
			}
			o_ptr->recharging = 250 - get_skill_scale(p_ptr, SKILL_DEVICE, 200);
			break;
		}
		case ART_NAIN:
			wall_to_mud(Ind, dir);
			o_ptr->recharging = rand_int(5) + 7 - get_skill_scale(p_ptr, SKILL_DEVICE, 3);
			break;
		case ART_EOL:
			sprintf(p_ptr->attacker, " fires a mana bolt for");
			fire_bolt(Ind, GF_MANA, dir, damroll(9 + get_skill_scale(p_ptr, SKILL_DEVICE, 20), 8), p_ptr->attacker);
			o_ptr->recharging = rand_int(3) + 7 - get_skill_scale(p_ptr, SKILL_DEVICE, 5);
			break;
		case ART_UMBAR:
			sprintf(p_ptr->attacker, " fires a missile for");
			fire_bolt(Ind, GF_MISSILE, dir, damroll(10 + get_skill_scale(p_ptr, SKILL_DEVICE, 20), 10), p_ptr->attacker);
			o_ptr->recharging = rand_int(10) + 20 - get_skill_scale(p_ptr, SKILL_DEVICE, 15);
			break;
		case ART_HELLFIRE:
			sprintf(p_ptr->attacker, " conjures up hellfire for");
			fire_ball(Ind, GF_HELL_FIRE, dir, 400 + get_skill_scale(p_ptr, SKILL_DEVICE, 200), 3, p_ptr->attacker);
			o_ptr->recharging = randint(10) + 30;
			break;
		case ART_HAVOC:
			sprintf(p_ptr->attacker, " casts a force bolt for");
			fire_bolt(Ind, GF_FORCE, dir, damroll(8 + get_skill_scale(p_ptr, SKILL_DEVICE, 16), 8), p_ptr->attacker);
			o_ptr->recharging = rand_int(2) + 1;
			break;
		default: done = FALSE;
		}
	}

	// -------------------- ego items -------------------- //

	// -------------------- base items -------------------- //

	if (!done && o_ptr->tval == TV_DRAG_ARMOR && item == INVEN_BODY) {
		switch (o_ptr->sval) {
		case SV_DRAGON_BLACK:
			msg_print(Ind, "You breathe acid.");
			sprintf(p_ptr->attacker, " breathes acid for");
			fire_ball(Ind, GF_ACID, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
			break;
		case SV_DRAGON_BLUE:
			msg_print(Ind, "You breathe lightning.");
			sprintf(p_ptr->attacker, " breathes lightning for");
			fire_ball(Ind, GF_ELEC, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
			break;
		case SV_DRAGON_WHITE:
			msg_print(Ind, "You breathe frost.");
			sprintf(p_ptr->attacker, " breathes frost for");
			fire_ball(Ind, GF_COLD, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
			break;
		case SV_DRAGON_RED:
			msg_print(Ind, "You breathe fire.");
			sprintf(p_ptr->attacker, " breathes fire for");
			fire_ball(Ind, GF_FIRE, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
			break;
		case SV_DRAGON_GREEN:
			msg_print(Ind, "You breathe poison.");
			sprintf(p_ptr->attacker, " breathes poison for");
			fire_ball(Ind, GF_POIS, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
			break;
		case SV_DRAGON_MULTIHUED:
			switch(rand_int(5)){
			case 0:	msg_print(Ind, "You breathe acid.");
				sprintf(p_ptr->attacker, " breathes acid for");
				fire_ball(Ind, GF_ACID, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
				break;
			case 1:	msg_print(Ind, "You breathe lightning.");
				sprintf(p_ptr->attacker, " breathes lightning for");
				fire_ball(Ind, GF_ELEC, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
				break;
			case 2:	msg_print(Ind, "You breathe frost.");
				sprintf(p_ptr->attacker, " breathes frost for");
				fire_ball(Ind, GF_COLD, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
				break;
			case 3:	msg_print(Ind, "You breathe fire.");
				sprintf(p_ptr->attacker, " breathes fire for");
				fire_ball(Ind, GF_FIRE, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
				break;
			case 4:	msg_print(Ind, "You breathe poison.");
				sprintf(p_ptr->attacker, " breathes poison for");
				fire_ball(Ind, GF_POIS, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
				break;
			}
			break;
		case SV_DRAGON_PSEUDO:
			if (magik(50)) {
				msg_print(Ind, "You breathe light.");
				sprintf(p_ptr->attacker, " breathes light for");
				fire_ball(Ind, GF_LITE, dir, 200 + get_skill_scale(p_ptr, SKILL_DEVICE, 200), 4, p_ptr->attacker);
			} else {
				msg_print(Ind, "You breathe darkness.");
				sprintf(p_ptr->attacker, " breathes darkness for");
				fire_ball(Ind, GF_DARK, dir, 200 + get_skill_scale(p_ptr, SKILL_DEVICE, 200), 4, p_ptr->attacker);
			}
			break;
		case SV_DRAGON_SHINING:
			if (magik(50)) {
				msg_print(Ind, "You breathe light.");
				sprintf(p_ptr->attacker, " breathes light for");
				fire_ball(Ind, GF_LITE, dir, 400 + get_skill_scale(p_ptr, SKILL_DEVICE, 400), 4, p_ptr->attacker);
			} else {
				msg_print(Ind, "You breathe darkness.");
				sprintf(p_ptr->attacker, " breathes darkness for");
				fire_ball(Ind, GF_DARK, dir, 400 + get_skill_scale(p_ptr, SKILL_DEVICE, 400), 4, p_ptr->attacker);
			}
			break;
		case SV_DRAGON_LAW:
			switch(rand_int(2)){
			case 0:	msg_print(Ind, "You breathe shards.");
				sprintf(p_ptr->attacker, " breathes shards for");
				fire_ball(Ind, GF_SHARDS, dir, 400 + get_skill_scale(p_ptr, SKILL_DEVICE, 400), 4, p_ptr->attacker);
				break;
			case 1:	msg_print(Ind, "You breathe sound.");
				sprintf(p_ptr->attacker, " breathes sound for");
				fire_ball(Ind, GF_SOUND, dir, 400 + get_skill_scale(p_ptr, SKILL_DEVICE, 400), 4, p_ptr->attacker);
				break;
			}
			break;
		case SV_DRAGON_BRONZE:
			msg_print(Ind, "You breathe confusion.");
			sprintf(p_ptr->attacker, " breathes confusion for");
			fire_ball(Ind, GF_CONFUSION, dir, 300 + get_skill_scale(p_ptr, SKILL_DEVICE, 300), 4, p_ptr->attacker);
			break;
		case SV_DRAGON_GOLD:
			msg_print(Ind, "You breathe sound.");
			sprintf(p_ptr->attacker, " breathes sound for");
			fire_ball(Ind, GF_SOUND, dir, 400 + get_skill_scale(p_ptr, SKILL_DEVICE, 400), 4, p_ptr->attacker);
			break;
		case SV_DRAGON_CHAOS:
			msg_print(Ind, "You breathe chaos.");
			sprintf(p_ptr->attacker, " breathes chaos for");
			fire_ball(Ind, GF_CHAOS, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
			break;
		case SV_DRAGON_BALANCE:
			msg_print(Ind, "You breathe disenchantment.");
			sprintf(p_ptr->attacker, " breathes disenchantment for");
			fire_ball(Ind, GF_DISENCHANT, dir, 500 + get_skill_scale(p_ptr, SKILL_DEVICE, 500), 4, p_ptr->attacker);
			break;
		case SV_DRAGON_POWER:
			/* redux (blend out 'weaker' types that monsters may be immune to etc)
			   Even GF_NETHER isn't breathed for now, since it might be too bad vs undead.
				GF_SHARDS,	GF_INERTIA,	GF_SOUND,	GF_GRAVITY,
				GF_NEXUS,	GF_TIME,	GF_FORCE,	GF_MANA,
				GF_CHAOS,	GF_NUKE,	GF_PLASMA,	GF_DISINTEGRATE,
				GF_DISENCHANT,
			*/
			switch (rand_int(13)) {
			case 0:
				msg_print(Ind, "You breathe shards.");
				sprintf(p_ptr->attacker, " breathes shards for");
				fire_ball(Ind, GF_SHARDS, dir, 500 + get_skill_scale(p_ptr, SKILL_DEVICE, 500), 4, p_ptr->attacker);
				break;
			case 1:
				msg_print(Ind, "You breathe sound.");
				sprintf(p_ptr->attacker, " breathes sound for");
				fire_ball(Ind, GF_SOUND, dir, 500 + get_skill_scale(p_ptr, SKILL_DEVICE, 500), 4, p_ptr->attacker);
				break;
			case 2:
				msg_print(Ind, "You breathe force.");
				sprintf(p_ptr->attacker, " breathes force for");
				fire_ball(Ind, GF_FORCE, dir, 375 + get_skill_scale(p_ptr, SKILL_DEVICE, 375), 4, p_ptr->attacker);
				break;
			case 3:
				msg_print(Ind, "You breathe inertia.");
				sprintf(p_ptr->attacker, " breathes inertia for");
				fire_ball(Ind, GF_INERTIA, dir, 375 + get_skill_scale(p_ptr, SKILL_DEVICE, 375), 4, p_ptr->attacker);
				break;
			case 4:
				msg_print(Ind, "You breathe gravity.");
				sprintf(p_ptr->attacker, " breathes gravity for");
				fire_ball(Ind, GF_GRAVITY, dir, 350 + get_skill_scale(p_ptr, SKILL_DEVICE, 350), 4, p_ptr->attacker); //extra-boosted damage
				break;
			case 5:
				msg_print(Ind, "You breathe nexus.");
				sprintf(p_ptr->attacker, " breathes nexus for");
				fire_ball(Ind, GF_NEXUS, dir, 375 + get_skill_scale(p_ptr, SKILL_DEVICE, 375), 4, p_ptr->attacker);
				break;
			case 6:
				msg_print(Ind, "You breathe time.");
				sprintf(p_ptr->attacker, " breathes time for");
				fire_ball(Ind, GF_TIME, dir, 350 + get_skill_scale(p_ptr, SKILL_DEVICE, 350), 4, p_ptr->attacker); //extra-boosted damage
				break;
			case 7:
				msg_print(Ind, "You breathe mana.");
				sprintf(p_ptr->attacker, " breathes mana for");
				fire_ball(Ind, GF_MANA, dir, 450 + get_skill_scale(p_ptr, SKILL_DEVICE, 450), 4, p_ptr->attacker);
				break;
			case 8:
				msg_print(Ind, "You breathe plasma.");
				sprintf(p_ptr->attacker, " breathes plasma for");
				fire_ball(Ind, GF_PLASMA, dir, 500 + get_skill_scale(p_ptr, SKILL_DEVICE, 500), 4, p_ptr->attacker);
				break;
			case 9:
				msg_print(Ind, "You breathe toxic waste.");
				sprintf(p_ptr->attacker, " breathes toxic waste for");
				fire_ball(Ind, GF_NUKE, dir, 500 + get_skill_scale(p_ptr, SKILL_DEVICE, 500), 4, p_ptr->attacker);
				break;
			case 10:
				msg_print(Ind, "You breathe chaos.");
				sprintf(p_ptr->attacker, " breathes chaos for");
				fire_ball(Ind, GF_CHAOS, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
				break;
			case 11:
				msg_print(Ind, "You breathe disenchantment.");
				sprintf(p_ptr->attacker, " breathes disenchantment for");
				fire_ball(Ind, GF_DISENCHANT, dir, 500 + get_skill_scale(p_ptr, SKILL_DEVICE, 500), 4, p_ptr->attacker);
				break;
			case 12:
				msg_print(Ind, "You breathe disintegration.");
				sprintf(p_ptr->attacker, " breathes disintegration for");
				fire_ball(Ind, GF_DISINTEGRATE, dir, 400 + get_skill_scale(p_ptr, SKILL_DEVICE, 400), 4, p_ptr->attacker);
				break;
			}
			break;
		case SV_DRAGON_DEATH:
			msg_print(Ind, "You breathe nether.");
			sprintf(p_ptr->attacker, " breathes nether for");
			fire_ball(Ind, GF_NETHER, dir, 550 + get_skill_scale(p_ptr, SKILL_DEVICE, 500), 4, p_ptr->attacker);
			break;
		case SV_DRAGON_CRYSTAL:
			msg_print(Ind, "You breathe shards.");
			sprintf(p_ptr->attacker, " breathes shards for");
			fire_ball(Ind, GF_SHARDS, dir, 400 + get_skill_scale(p_ptr, SKILL_DEVICE, 400), 4, p_ptr->attacker);
			break;
		case SV_DRAGON_DRACOLICH:
			switch(rand_int(2)){
			case 0:	msg_print(Ind, "You breathe nether.");
				sprintf(p_ptr->attacker, " breathes nether for");
				fire_ball(Ind, GF_NETHER, dir, 550 + get_skill_scale(p_ptr, SKILL_DEVICE, 550), 4, p_ptr->attacker);
				break;
			case 1:	msg_print(Ind, "You breathe cold.");
				sprintf(p_ptr->attacker, " breathes cold for");
				fire_ball(Ind, GF_COLD, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
				break;
			}
			break;
		case SV_DRAGON_DRACOLISK:
			switch(rand_int(2)){
			case 0:	msg_print(Ind, "You breathe fire.");
				sprintf(p_ptr->attacker, " breathes fire for");
				fire_ball(Ind, GF_FIRE, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
				break;
			case 1:	msg_print(Ind, "You breathe nexus.");
				sprintf(p_ptr->attacker, " breathes nexus for");
				fire_ball(Ind, GF_NEXUS, dir, 250 + get_skill_scale(p_ptr, SKILL_DEVICE, 250), 4, p_ptr->attacker);
				break;
			}
			break;
		case SV_DRAGON_SKY:
			switch(rand_int(3)){
			case 0:	msg_print(Ind, "You breathe lightning.");
				sprintf(p_ptr->attacker, " breathes lightning for");
				fire_ball(Ind, GF_ELEC, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
				break;
			case 1:	msg_print(Ind, "You breathe light.");
				sprintf(p_ptr->attacker, " breathes light for");
				fire_ball(Ind, GF_LITE, dir, 400 + get_skill_scale(p_ptr, SKILL_DEVICE, 400), 4, p_ptr->attacker);
				break;
			case 2:	msg_print(Ind, "You breathe gravity.");
				sprintf(p_ptr->attacker, " breathes gravity for");
				fire_ball(Ind, GF_GRAVITY, dir, 300 + get_skill_scale(p_ptr, SKILL_DEVICE, 300), 4, p_ptr->attacker); //extra-boosted damage
				break;
			}
			break;
		case SV_DRAGON_SILVER:
			if (magik(50)) {
				msg_print(Ind, "You breathe inertia.");
				sprintf(p_ptr->attacker, " breathes inertia for");
				fire_ball(Ind, GF_INERTIA, dir, 250 + get_skill_scale(p_ptr, SKILL_DEVICE, 250), 4, p_ptr->attacker);
			} else {
				msg_print(Ind, "You breathe cold.");
				sprintf(p_ptr->attacker, " breathes cold for");
				fire_ball(Ind, GF_COLD, dir, 600 + get_skill_scale(p_ptr, SKILL_DEVICE, 600), 4, p_ptr->attacker);
			}
			break;
		}
		o_ptr->recharging = 250 + rand_int(20) - get_skill_scale(p_ptr, SKILL_DEVICE, 230);//pretty big effect^^
	}

	/* Hack -- Amulet of the Serpents can be activated as well */
	if (!done && o_ptr->tval == TV_AMULET && o_ptr->sval == SV_AMULET_SERPENT) {
		msg_print(Ind, "You breathe venom...");
		sprintf(p_ptr->attacker, " breathes venom for");
		fire_ball(Ind, GF_POIS, dir, 100 + get_skill_scale(p_ptr, SKILL_DEVICE, 200), 2, p_ptr->attacker);
		o_ptr->recharging = rand_int(60) + 40 - get_skill_scale(p_ptr, SKILL_DEVICE, 30);
	}
	else if (!done && o_ptr->tval == TV_RING) {
		switch (o_ptr->sval) {
		case SV_RING_ELEC:
			/* Get a direction for breathing (or abort) */
			sprintf(p_ptr->attacker, " casts a lightning ball for");
			fire_ball(Ind, GF_ELEC, dir, 50 + get_skill_scale(p_ptr, SKILL_DEVICE, 150), 2, p_ptr->attacker);
			(void)set_oppose_elec(Ind, randint(20) + 20); /* removed stacking */
			o_ptr->recharging = rand_int(25) + 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 25);
			break;
		case SV_RING_ACID:
			/* Get a direction for breathing (or abort) */
			sprintf(p_ptr->attacker, " casts an acid ball for");
			fire_ball(Ind, GF_ACID, dir, 50 + get_skill_scale(p_ptr, SKILL_DEVICE, 150), 2, p_ptr->attacker);
			(void)set_oppose_acid(Ind, randint(20) + 20); /* removed stacking */
			o_ptr->recharging = rand_int(25) + 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 25);
			break;
		case SV_RING_ICE:
			/* Get a direction for breathing (or abort) */
			sprintf(p_ptr->attacker, " casts a frost ball for");
			fire_ball(Ind, GF_COLD, dir, 50 + get_skill_scale(p_ptr, SKILL_DEVICE, 150), 2, p_ptr->attacker);
			(void)set_oppose_cold(Ind, randint(20) + 20); /* removed stacking */
			o_ptr->recharging = rand_int(25) + 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 25);
			break;
		case SV_RING_FLAMES:
			/* Get a direction for breathing (or abort) */
			sprintf(p_ptr->attacker, " casts a fire ball for");
			fire_ball(Ind, GF_FIRE, dir, 50 + get_skill_scale(p_ptr, SKILL_DEVICE, 150), 2, p_ptr->attacker);
			(void)set_oppose_fire(Ind, randint(20) + 20); /* removed stacking */
			o_ptr->recharging = rand_int(25) + 50 - get_skill_scale(p_ptr, SKILL_DEVICE, 25);
			break;
		}
	}

	/* Clear current activation */
	p_ptr->current_activation = -1;

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	/* Success */
	return;
}

bool unmagic(int Ind) {
	player_type *p_ptr = Players[Ind];
	bool ident = FALSE;

	/* Unmagic has no effect when the player is invulnerable. This prevents
	 * stair-GoI from being canceled prematurely by unmagic mushrooms etc.
	 */
	if (p_ptr->invuln || p_ptr->martyr) return FALSE;

	if (
		set_adrenaline(Ind, 0) |
		set_biofeedback(Ind, 0) |
		set_tim_esp(Ind, 0) |
		set_st_anchor(Ind, 0) |
		set_prob_travel(Ind, 0) |
		set_bow_brand(Ind, 0, 0, 0) |
#if 0
		set_mimic(Ind, 0, 0) |
#endif
#if 0
		(p_ptr->body_monster) |
	if (p_ptr->tim_mimic) do_mimic_change(Ind, 0, TRUE)
#endif
		set_tim_manashield(Ind, 0) |
		set_tim_traps(Ind, 0) |
		set_invis(Ind, 0, 0) |
		set_tim_meditation(Ind, 0) |
		set_tim_wraith(Ind, 0) |
		set_fast(Ind, 0, 0) |
		set_shield(Ind, 0, 0, SHIELD_NONE, 0, 0) |
		set_blessed(Ind, 0) |
		set_hero(Ind, 0) |
		set_shero(Ind, 0) |
		set_fury(Ind, 0) |
		set_protevil(Ind, 0) |
		set_invuln(Ind, 0) |
		set_tim_invis(Ind, 0) |
		set_tim_infra(Ind, 0) |
		set_oppose_acid(Ind, 0) |
		set_oppose_elec(Ind, 0) |
		set_oppose_fire(Ind, 0) |
		set_oppose_cold(Ind, 0) |
		set_oppose_pois(Ind, 0) |
		set_zeal(Ind, 0, 0) |
		set_mindboost(Ind, 0, 0) |
		//set_martyr(Ind, 0) |
		set_sh_fire_tim(Ind, 0) |
		set_sh_cold_tim(Ind, 0) |
		set_sh_elec_tim(Ind, 0) |
		set_kinetic_shield(Ind, 0) |
		do_mstopcharm(Ind) |
#ifdef ENABLE_OCCULT
		set_savingthrow(Ind, 0) |
		set_spirit_shield(Ind, 0, 0) |
#endif
		set_tim_deflect(Ind, 0)
	) ident = TRUE;

	if (p_ptr->word_recall) ident |= set_recall_timer(Ind, 0);

#if 0 /* taken out because it circumvents the HEALING spell anti-cheeze */
	p_ptr->supp = 0;
	p_ptr->support_timer = 0;
#endif
	return (ident);
}

/*
 * Displays random fortune/rumour.
 * Thanks Mihi!		- Jir -
 */
void fortune(int Ind, bool broadcast) {
	char Rumor[MAX_CHARS_WIDE], Broadcast[MAX_CHARS];

	strcpy(Broadcast, "Suddenly a thought comes to your mind:");
	msg_print(Ind, NULL);

	//switch(randint(20))
	switch(randint(80)) {
	case 1:
		get_rnd_line("chainswd.txt",0 , Rumor, MAX_CHARS_WIDE);
		break;
	case 2:
		get_rnd_line("error.txt",0 , Rumor, MAX_CHARS_WIDE);
		break;
	case 3:
	case 4:
	case 5:
		get_rnd_line("death.txt",0 , Rumor, MAX_CHARS_WIDE);
		break;
	default:
		if (magik(95)) get_rnd_line("rumors.txt",0 , Rumor, MAX_CHARS_WIDE);
		else {
			strcpy(Broadcast, "Suddenly an important thought comes to your mind:");
			get_rnd_line("hints.txt",0 , Rumor, MAX_CHARS_WIDE);
		}
	}

	bracer_ff(Rumor);
	//msg_format(Ind, "%s", Rumor);
	msg_print(Ind, Rumor);
	msg_print(Ind, NULL);

	if (broadcast) {
		msg_broadcast(Ind, Broadcast);
		msg_broadcast(Ind, Rumor);
	}

}

char random_colour() {
	//char tmp[] = "wWrRbBgGdDuUoyvs";
	char tmp[] = "dwsorgbuDWvyRGBU";

	return(tmp[randint(15)]);	// never 'd'
}


/*
 * These are out of place -- we should add cmd7.c in the near future.	- Jir -
 */

/*
 * Hook to determine if an object is convertible in an arrow/bolt or pebble
 */
static int fletchery_items(int Ind, int type) {
	player_type *p_ptr = Players[Ind];
	object_type     *o_ptr;
	int i;

	switch (type) {
	case SKILL_BOW:
	case SKILL_XBOW:
		for (i = 0; i < INVEN_PACK; i++) {
			o_ptr = &p_ptr->inventory[i];
			if (!o_ptr->k_idx) continue;
			if (!can_use_admin(Ind, o_ptr)) continue;
			/* Broken Stick */
			if (o_ptr->tval == TV_JUNK && o_ptr->sval == SV_WOODEN_STICK) return (i);
			if (o_ptr->tval == TV_SKELETON) return (i);
		}
		break;
	case SKILL_SLING:
		for (i = 0; i < INVEN_PACK; i++) {
			o_ptr = &p_ptr->inventory[i];
			if (!o_ptr->k_idx) continue;
			if (!can_use_admin(Ind, o_ptr)) continue;
			/* Shards of Pottery */
			if (o_ptr->tval == TV_JUNK && o_ptr->sval == SV_POTTERY) return (i);
		}
		break;
	}

	/* Failed */
	return (-1);
}

/* Dirty but useful macro */
#define do_fletchery_aux() \
	object_aware(Ind, q_ptr); \
	object_known(q_ptr); \
	if (tlev > 50) q_ptr->ident |= ID_MENTAL; \
	apply_magic(&p_ptr->wpos, q_ptr, tlev, FALSE, get_skill(p_ptr, SKILL_ARCHERY) >= 20, (magik(tlev / 10))?TRUE:FALSE, FALSE, RESF_NOART); \
	q_ptr->ident &= ~ID_CURSED; \
	q_ptr->note = quark_add("handmade"); \
	/* q_ptr->discount = 50 + 25 * rand_int(3); */ \
	msg_print(Ind, "You make some ammo.")
/*
	apply_magic(&p_ptr->wpos, q_ptr, tlev, TRUE, get_skill(p_ptr, SKILL_ARCHERY) >= 20, (magik(tlev / 10))?TRUE:FALSE, FALSE, make_resf(p_ptr)); \
	apply_magic(&p_ptr->wpos, q_ptr, tlev, TRUE, TRUE, (magik(tlev / 10))?TRUE:FALSE, FALSE, make_resf(p_ptr));
*/

/* finish creating sling ammo dug from rubble */
void create_sling_ammo_aux(int Ind) {
	player_type *p_ptr = Players[Ind];
	int tlev = get_skill_scale(p_ptr, SKILL_ARCHERY, 50) - 20
		+ get_skill_scale(p_ptr, SKILL_SLING, 35);
	object_type forge, *q_ptr;

	/* S(he) is no longer afk */
	//un_afk_idle(Ind);
	p_ptr->current_create_sling_ammo = FALSE;

	/* Get local object */
	q_ptr = &forge;

	/* Hack -- Give the player some bullets */
	invcopy(q_ptr, lookup_kind(TV_SHOT, m_bonus(2, tlev)));
	q_ptr->number = (byte)rand_range(15,30);
	q_ptr->iron_trade = p_ptr->iron_trade;
	do_fletchery_aux();

	if (q_ptr->name2 == EGO_ETHEREAL || q_ptr->name2b == EGO_ETHEREAL) q_ptr->number /= ETHEREAL_AMMO_REDUCTION;

	(void)inven_carry(Ind, q_ptr);

	//p_ptr->update |= (PU_VIEW | PU_FLOW | PU_MON_LITE);
	p_ptr->update |= (PU_VIEW | PU_FLOW);
	p_ptr->window |= (PW_OVERHEAD);

	p_ptr->energy -= level_speed(&p_ptr->wpos);
#if 0 /* not happening anyway */
	break_cloaking(Ind, 5);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
#endif
}

/*
 * do_cmd_cast calls this function if the player's class
 * is 'archer'.
 */
//void do_cmd_archer(void)
void do_cmd_fletchery(int Ind) {
	player_type *p_ptr = Players[Ind];
	int ext = 0, tlev = 0, raw_materials, raw_amount;
	//char ch;
	object_type forge, *q_ptr;
	int item;
	cave_type **zcave;

	if (!(zcave = getcave(&p_ptr->wpos))) return;


	if (p_ptr->confused) {
		msg_print(Ind, "You are too confused!");
		return;
	}
	if (p_ptr->blind) {
		msg_print(Ind, "You are blind!");
		return;
	}
	if (get_skill(p_ptr, SKILL_ARCHERY) < 10) {
		msg_print(Ind, "You don't know how to craft ammo well.");
		return;
	}

	ext = get_archery_skill(p_ptr);
	if (ext < 1) {
		msg_print(Ind, "Sorry, you have to wield a launcher first.");
		return;
	}
	if (ext == SKILL_BOOMERANG) {
		msg_print(Ind, "You don't need ammo for boomerangs, naturally.");
		return;
	}


	/* Prepare for object creation */

	tlev = get_skill_scale(p_ptr, SKILL_ARCHERY, 50) - 20
		+ get_skill_scale(p_ptr, ext, 35);

	item = fletchery_items(Ind, ext);

	/**********Create shots*********/
	if (ext == SKILL_SLING) {
		int x, y, dir;
		cave_type *c_ptr;

		if (item < 0) { //no item available? check for rubble then
#if 0
			/* Get the item (on the floor) */
			else if (-item >= o_max)
				return; /* item doesn't exist */
			    q_ptr = &o_list[0 - item];
#endif

			if (p_ptr->tim_wraith) { /* Not in WRAITHFORM ^^ */
				msg_print(Ind, "You can't pick up rubble in incorporeal form!");
				return;
			}
			if (p_ptr->IDDC_logscum) {
				msg_print(Ind, "\377RThis floor has become stale, take a staircase to move on!");
				return;
			}

			for (dir = 1; dir <= 9; dir++) {
				y = p_ptr->py + ddy[dir];
				x = p_ptr->px + ddx[dir];
				c_ptr = &zcave[y][x];
				if (c_ptr->feat == FEAT_RUBBLE) {
					/* new: actually tunnel through the rubble in order to craft ammo from it - C. Blue */

					/* need this here since we abuse it to reset current_create_sling_ammo */
					stop_precision(Ind);

					/* start digging the rubble.. */
					p_ptr->current_create_sling_ammo = TRUE;
					fake_Receive_tunnel(Ind, dir);
					return;
				}
			}
			msg_print(Ind, "There are no appropriate materials available.");
			return;
		}

		/* Get the item (in the pack) */
		q_ptr = &p_ptr->inventory[item];

		/* S(he) is no longer afk */
		un_afk_idle(Ind);

		/* Remember amount of raw materials used for this */
		raw_materials = q_ptr->number;
		/* Prevent large pack overflow which results in much lost ammo */
		if (raw_materials > 10) raw_materials = 10;

		/* Get local object */
		q_ptr = &forge;

		/* Hack -- Give the player some arrows */
		//q_ptr->number = (byte)rand_range(15,25);
		invcopy(q_ptr, lookup_kind(TV_SHOT, SV_AMMO_LIGHT));
		q_ptr->number = (p_ptr->inventory[item].weight * 2) / (q_ptr->weight + 1) + randint(5);
		if (q_ptr->number > 99) q_ptr->number = 99;
		raw_amount = q_ptr->number * raw_materials;
		do_fletchery_aux();

		if (item >= 0) {
			inven_item_increase(Ind, item, -raw_materials);//, -1)
			inven_item_describe(Ind, item);
			inven_item_optimize(Ind, item);
		} else {
			floor_item_increase(0 - item, -raw_materials);//, -1)
			floor_item_describe(0 - item);
			floor_item_optimize(0 - item);
		}

		if (q_ptr->name2 == EGO_ETHEREAL || q_ptr->name2b == EGO_ETHEREAL) raw_amount /= ETHEREAL_AMMO_REDUCTION;
		q_ptr->iron_trade = p_ptr->iron_trade;

		while (raw_amount > 99) {
			q_ptr->number = 99;
			raw_amount -= 99;
			(void)inven_carry(Ind, q_ptr);
		}
		if (raw_amount) {
			q_ptr->number = raw_amount;
			(void)inven_carry(Ind, q_ptr);
		}
	}

	/**********Create arrows*********/
	else if (ext == SKILL_BOW) {
		/* Get the item (in the pack) */
		if (item >= 0) q_ptr = &p_ptr->inventory[item];
		/* Get the item (on the floor) */
		else {
			msg_print(Ind, "You don't have appropriate materials.");
			return;
#if 0
			if (-item >= o_max)
				return; /* item doesn't exist */
			q_ptr = &o_list[0 - item];
#endif
		}

		/* S(he) is no longer afk */
		un_afk_idle(Ind);

		/* Remember amount of raw materials used for this */
		raw_materials = q_ptr->number;
		/* Prevent large pack overflow which results in much lost ammo */
		if (raw_materials > 10) raw_materials = 10;

		/* Get local object */
		q_ptr = &forge;

		/* Hack -- Give the player some arrows */
		//q_ptr->number = (byte)rand_range(15,25);
		invcopy(q_ptr, lookup_kind(TV_ARROW, m_bonus(1, tlev) + 1));
		q_ptr->number = p_ptr->inventory[item].weight / q_ptr->weight + randint(5);
		if (q_ptr->number > 99) q_ptr->number = 99;
		raw_amount = q_ptr->number * raw_materials;
		do_fletchery_aux();

		if (item >= 0) {
			inven_item_increase(Ind, item, -raw_materials);//, -1)
			inven_item_describe(Ind, item);
			inven_item_optimize(Ind, item);
		} else {
			floor_item_increase(0 - item, -raw_materials);//, -1)
			floor_item_describe(0 - item);
			floor_item_optimize(0 - item);
		}

		if (q_ptr->name2 == EGO_ETHEREAL || q_ptr->name2b == EGO_ETHEREAL) raw_amount /= ETHEREAL_AMMO_REDUCTION;
		q_ptr->iron_trade = p_ptr->iron_trade;

		while (raw_amount > 99) {
			q_ptr->number = 99;
			raw_amount -= 99;
			(void)inven_carry(Ind, q_ptr);
		}
		if (raw_amount) {
			q_ptr->number = raw_amount;
			(void)inven_carry(Ind, q_ptr);
		}
	}

	/**********Create bolts*********/
	else if (ext == SKILL_XBOW) {
		/* Get the item (in the pack) */
		if (item >= 0) q_ptr = &p_ptr->inventory[item];
		/* Get the item (on the floor) */
		else {
			msg_print(Ind, "You don't have appropriate materials.");
			return;
#if 0
			if (-item >= o_max)
				return; /* item doesn't exist */
			q_ptr = &o_list[0 - item];
#endif
		}

		/* S(he) is no longer afk */
		un_afk_idle(Ind);

		/* Remember amount of raw materials used for this */
		raw_materials = q_ptr->number;
		/* Prevent large pack overflow which results in much lost ammo */
		if (raw_materials > 10) raw_materials = 10;

		/* Get local object */
		q_ptr = &forge;

		/* Hack -- Give the player some bolts */
		invcopy(q_ptr, lookup_kind(TV_BOLT, m_bonus(1, tlev) + 1));
		//q_ptr->number = (byte)rand_range(15,25);
		q_ptr->number = p_ptr->inventory[item].weight / q_ptr->weight + randint(5);
		if (q_ptr->number > 99) q_ptr->number = 99;
		raw_amount = q_ptr->number * raw_materials;
		do_fletchery_aux();

		if (item >= 0) {
			inven_item_increase(Ind, item, -raw_materials);//, -1)
			inven_item_describe(Ind, item);
			inven_item_optimize(Ind, item);
		} else {
			floor_item_increase(0 - item, -raw_materials);//, -1)
			floor_item_describe(0 - item);
			floor_item_optimize(0 - item);
		}

		if (q_ptr->name2 == EGO_ETHEREAL || q_ptr->name2b == EGO_ETHEREAL) raw_amount /= ETHEREAL_AMMO_REDUCTION;
		q_ptr->iron_trade = p_ptr->iron_trade;

		while (raw_amount > 99) {
			q_ptr->number = 99;
			raw_amount -= 99;
			(void)inven_carry(Ind, q_ptr);
		}
		if (raw_amount) {
			q_ptr->number = raw_amount;
			(void)inven_carry(Ind, q_ptr);
		}
	}

	p_ptr->energy -= level_speed(&p_ptr->wpos);
	break_cloaking(Ind, 5);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
}

/*
 * Use a combat stance for armed melee combat - C. Blue
 * Note: SKILL_STANCE is increased automatically by +1.000 every 5 levels!
 * Players start out with 1.000 skill. The skill number is just for show though ;)
 */
void do_cmd_stance(int Ind, int stance) {
	int power = 0;
	player_type *p_ptr = Players[Ind];
#ifndef ENABLE_STANCES
	return;
#endif

	if (!get_skill(p_ptr, SKILL_STANCE)) return;

	switch(stance) {
	case 0: /* always known, no different power levels here */
		if (!p_ptr->combat_stance) {
			msg_print(Ind, "\377sYou already are in balanced stance!");
			return;
		}
		msg_print(Ind, "\377sYou enter a balanced stance!");
s_printf("SWITCH_STANCE: %s - balance\n", p_ptr->name);
	break;
	case 1:
		switch(p_ptr->pclass) {
		case CLASS_WARRIOR:
			if (p_ptr->max_lev < 5) {
				msg_print(Ind, "\377sYou haven't learned a defensive stance yet.");
				return;
			}
		break;
		case CLASS_MIMIC:
			if (p_ptr->max_lev < 10) {
				msg_print(Ind, "\377sYou haven't learned a defensive stance yet.");
				return;
			}
		break;
		case CLASS_PALADIN:
#ifdef ENABLE_DEATHKNIGHT
		case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
		case CLASS_HELLKNIGHT:
#endif
			if (p_ptr->max_lev < 5) {
				msg_print(Ind, "\377sYou haven't learned a defensive stance yet.");
				return;
			}
		break;
		case CLASS_RANGER:
			if (p_ptr->max_lev < 10) {
				msg_print(Ind, "\377sYou haven't learned a defensive stance yet.");
				return;
			}
		break;
		}

		if (p_ptr->combat_stance == 1) {
			msg_print(Ind, "\377sYou already are in defensive stance!");
			return;
		}
#ifndef ALLOW_SHIELDLESS_DEFENSIVE_STANCE
		if (!p_ptr->inventory[INVEN_ARM].k_idx ||
		    p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) { /* not dual-wielding? */
			msg_print(Ind, "\377yYou cannot enter defensive stance without wielding a shield.");
			return;
		}
#else
		if (!p_ptr->inventory[INVEN_WIELD].k_idx && !p_ptr->inventory[INVEN_ARM].k_idx) {
			msg_print(Ind, "\377yYou cannot enter defensive stance without wielding either weapon or shield.");
			return;
		}
#endif

		switch(p_ptr->pclass) {
		case CLASS_WARRIOR:
			if (p_ptr->max_lev < 15) {
				power = 0;
				msg_print(Ind, "\377sYou enter defensive stance rank I");
			} else if (p_ptr->max_lev < 35) {
				power = 1;
				msg_print(Ind, "\377sYou enter defensive stance rank II");
			} else if (!p_ptr->total_winner || p_ptr->max_lev < 45) {
				power = 2; /* up to level 50, and highest rank for non-totalwinners! */
				msg_print(Ind, "\377sYou enter defensive stance rank III");
			} else {
				power = 3; /* royal rank */
				msg_print(Ind, "\377sYou enter Royal Rank defensive stance");
			}
			break;
		case CLASS_MIMIC:
			if (p_ptr->max_lev < 20) {
				power = 0;
				msg_print(Ind, "\377sYou enter defensive stance rank I");
			} else if (p_ptr->max_lev < 40) {
				power = 1;
				msg_print(Ind, "\377sYou enter defensive stance rank II");
			} else if (!p_ptr->total_winner || p_ptr->max_lev < 45) {
				power = 2; /* up to level 50, and highest rank for non-totalwinners! */
				msg_print(Ind, "\377sYou enter defensive stance rank III");
			} else {
				power = 3; /* royal rank */
				msg_print(Ind, "\377sYou enter Royal Rank defensive stance");
			}
			break;
		case CLASS_PALADIN:
#ifdef ENABLE_DEATHKNIGHT
		case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
		case CLASS_HELLKNIGHT:
#endif
			if (p_ptr->max_lev < 20) {
				power = 0;
				msg_print(Ind, "\377sYou enter defensive stance rank I");
			} else if (p_ptr->max_lev < 35) {
				power = 1;
				msg_print(Ind, "\377sYou enter defensive stance rank II");
			} else if (!p_ptr->total_winner || p_ptr->max_lev < 45) {
				power = 2; /* up to level 50, and highest rank for non-totalwinners! */
				msg_print(Ind, "\377sYou enter defensive stance rank III");
			} else {
				power = 3; /* royal rank */
				msg_print(Ind, "\377sYou enter Royal Rank defensive stance");
			}
			break;
		case CLASS_RANGER:
			if (p_ptr->max_lev < 20) {
				power = 0;
				msg_print(Ind, "\377sYou enter defensive stance rank I");
			} else if (p_ptr->max_lev < 40) {
				power = 1;
				msg_print(Ind, "\377sYou enter defensive stance rank II");
			} else if (!p_ptr->total_winner || p_ptr->max_lev < 45) {
				power = 2; /* up to level 50, and highest rank for non-totalwinners! */
				msg_print(Ind, "\377sYou enter defensive stance rank III");
			} else {
				power = 3; /* royal rank */
				msg_print(Ind, "\377sYou enter Royal Rank defensive stance");
			}
			break;
		}
s_printf("SWITCH_STANCE: %s - defensive\n", p_ptr->name);
		break;

	case 2:
		switch(p_ptr->pclass) {
		case CLASS_WARRIOR:
			if (p_ptr->max_lev < 10) {
				msg_print(Ind, "\377sYou haven't learned an offensive stance yet.");
				return;
			}
			break;
		case CLASS_MIMIC:
			if (p_ptr->max_lev < 15) {
				msg_print(Ind, "\377sYou haven't learned an offensive stance yet.");
				return;
			}
			break;
		case CLASS_PALADIN:
#ifdef ENABLE_DEATHKNIGHT
		case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
		case CLASS_HELLKNIGHT:
#endif
			if (p_ptr->max_lev < 15) {
				msg_print(Ind, "\377sYou haven't learned an offensive stance yet.");
				return;
			}
			break;
		case CLASS_RANGER:
			if (p_ptr->max_lev < 15) {
				msg_print(Ind, "\377sYou haven't learned an offensive stance yet.");
				return;
			}
			break;
		}

		if (p_ptr->combat_stance == 2) {
			msg_print(Ind, "\377sYou already are in offensive stance!");
			return;
		}
		if (!p_ptr->inventory[INVEN_WIELD].k_idx || p_ptr->inventory[INVEN_ARM].k_idx) {
			msg_print(Ind, "\377yYou need to wield one weapon with both your hands for offensive stances.");
			return;
		}
		if (!(k_info[p_ptr->inventory[INVEN_WIELD].k_idx].flags4 & (TR4_MUST2H | TR4_SHOULD2H | TR4_COULD2H))) {
			msg_print(Ind, "\377yYour weapon is too small to use it in offensive stance effectively.");
			return;
		}

		switch(p_ptr->pclass) {
		case CLASS_WARRIOR:
			if (p_ptr->max_lev < 20) {
				power = 0;
				msg_print(Ind, "\377sYou enter offensive stance rank I");
			} else if (p_ptr->max_lev < 40) {
				power = 1;
				msg_print(Ind, "\377sYou enter offensive stance rank II");
			} else if (!p_ptr->total_winner || p_ptr->max_lev < 45) {
				power = 2; /* up to level 50, and highest rank for non-totalwinners! */
				msg_print(Ind, "\377sYou enter offensive stance rank III");
			} else {
				power = 3; /* royal rank */
				msg_print(Ind, "\377sYou enter Royal Rank offensive stance");
			}
			break;
		case CLASS_MIMIC:
			if (p_ptr->max_lev < 25) {
				power = 0;
				msg_print(Ind, "\377sYou enter offensive stance rank I");
			} else if (p_ptr->max_lev < 40) {
				power = 1;
				msg_print(Ind, "\377sYou enter offensive stance rank II");
			} else if (!p_ptr->total_winner || p_ptr->max_lev < 45) {
				power = 2; /* up to level 50, and highest rank for non-totalwinners! */
				msg_print(Ind, "\377sYou enter offensive stance rank III");
			} else {
				power = 3; /* royal rank */
				msg_print(Ind, "\377sYou enter Royal Rank offensive stance");
			}
			break;
#ifdef ENABLE_DEATHKNIGHT
		case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
		case CLASS_HELLKNIGHT:
#endif
		case CLASS_PALADIN:
			if (p_ptr->max_lev < 25) {
				power = 0;
				msg_print(Ind, "\377sYou enter offensive stance rank I");
			} else if (p_ptr->max_lev < 40) {
				power = 1;
				msg_print(Ind, "\377sYou enter offensive stance rank II");
			} else if (!p_ptr->total_winner || p_ptr->max_lev < 45) {
				power = 2; /* up to level 50, and highest rank for non-totalwinners! */
				msg_print(Ind, "\377sYou enter offensive stance rank III");
			} else {
				power = 3; /* royal rank */
				msg_print(Ind, "\377sYou enter Royal Rank offensive stance");
			}
			break;
		case CLASS_RANGER:
			if (p_ptr->max_lev < 25) {
				power = 0;
				msg_print(Ind, "\377sYou enter offensive stance rank I");
			} else if (p_ptr->max_lev < 40) {
				power = 1;
				msg_print(Ind, "\377sYou enter offensive stance rank II");
			} else if (!p_ptr->total_winner || p_ptr->max_lev < 45) {
				power = 2; /* up to level 50, and highest rank for non-totalwinners! */
				msg_print(Ind, "\377sYou enter offensive stance rank III");
			} else {
				power = 3; /* royal rank */
				msg_print(Ind, "\377sYou enter Royal Rank offensive stance");
			}
			break;
		}
s_printf("SWITCH_STANCE: %s - offensive\n", p_ptr->name);
		break;
	}

	p_ptr->energy -= level_speed(&p_ptr->wpos) / 2; /* takes half a turn to switch */
	p_ptr->combat_stance = stance;
	p_ptr->combat_stance_power = power;
	p_ptr->update |= (PU_BONUS);
	p_ptr->redraw |= (PR_PLUSSES | PR_STATE);
//	handle_stuff();
}

void do_cmd_melee_technique(int Ind, int technique) {
	player_type *p_ptr = Players[Ind];
	int i;

	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster) {
		msg_print(Ind, "You cannot use techniques while transformed.");
		return;
	}
	if (p_ptr->ghost) {
		msg_print(Ind, "You cannot use techniques as a ghost.");
		return;
	}
	if (p_ptr->confused) {
		msg_print(Ind, "You cannot use techniques while confused.");
		return;
	}
	if (p_ptr->blind) {
		switch (technique) {
		case 1: case 3: case 9:
		case 10: case 14:
			msg_print(Ind, "You cannot use this technique while blind.");
			return;
		}
	}
/*	it's superfluous, and rogues now get techniques too but don't have stances..
	if (!get_skill(p_ptr, SKILL_STANCE)) return;
*/

	if ((p_ptr->pclass == CLASS_ROGUE || p_ptr->pclass == CLASS_RUNEMASTER) &&
	    p_ptr->rogue_heavyarmor) {
		msg_print(Ind, "You cannot utilize techniques well while wearing too heavy armour.");
		return;
	}

	disturb(Ind, 1, 0); /* stop resting, searching and running */

	switch (technique) {
	case 0:	if (!(p_ptr->melee_techniques & MT_SPRINT)) return; /* Sprint */
		if (p_ptr->cst < 7) { msg_print(Ind, "Not enough stamina!"); return; }
		p_ptr->cst -= 7;
		un_afk_idle(Ind);
		break_cloaking(Ind, 0);
		break_shadow_running(Ind);
		stop_precision(Ind);
		stop_shooting_till_kill(Ind);

		set_melee_sprint(Ind, 9 + rand_int(3)); /* number of turns it lasts */
s_printf("TECHNIQUE_MELEE: %s - sprint\n", p_ptr->name);
		p_ptr->warning_technique_melee = 1;
		break;
	case 1:	if (!(p_ptr->melee_techniques & MT_TAUNT)) return; /* Taunt */
		if (p_ptr->cst < 2) { msg_print(Ind, "Not enough stamina!"); return; }
		//if (p_ptr->energy < level_speed(&p_ptr->wpos) / 4) return;
		if (p_ptr->energy <= 0) return;
		p_ptr->cst -= 2;
		p_ptr->energy -= level_speed(&p_ptr->wpos) / 4; /* doing it while fighting no prob */
		un_afk_idle(Ind);
		break_cloaking(Ind, 0);
		break_shadow_running(Ind);
		stop_precision(Ind);

		taunt_monsters(Ind);
s_printf("TECHNIQUE_MELEE: %s - taunt\n", p_ptr->name);
		p_ptr->warning_technique_melee = 1;
		break;
	case 3:	if (!(p_ptr->melee_techniques & MT_DISTRACT)) return; /* Distract */
		if (p_ptr->cst < 1) { msg_print(Ind, "Not enough stamina!"); return; }
		p_ptr->cst -= 1;
		p_ptr->energy -= level_speed(&p_ptr->wpos) / 2; /* just a quick grimace and mimicking ;) */
		un_afk_idle(Ind);
		break_cloaking(Ind, 0);
		break_shadow_running(Ind);
		stop_precision(Ind);

		distract_monsters(Ind);
s_printf("TECHNIQUE_MELEE: %s - distract\n", p_ptr->name);
		p_ptr->warning_technique_melee = 1;
		break;
	case 4:	if (!(p_ptr->melee_techniques & MT_POISON)) return; /* Apply Poison */
		//if (p_ptr->cst < 2) { msg_print(Ind, "Not enough stamina!"); return; }
		if (!p_ptr->inventory[INVEN_WIELD].k_idx && (!p_ptr->inventory[INVEN_ARM].k_idx || p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)) {
			msg_print(Ind, "You must wield a melee weapon to apply poison.");
			return;
		}
		for (i = 0; i < INVEN_WIELD; i++)
			if ((p_ptr->inventory[i].ident & ID_KNOWN) /* skip unidentified items */ &&
			    ((p_ptr->inventory[i].tval == TV_POTION && p_ptr->inventory[i].sval == SV_POTION_POISON) ||
			    (p_ptr->inventory[i].tval == TV_FOOD &&
			    (p_ptr->inventory[i].sval == SV_FOOD_POISON || p_ptr->inventory[i].sval == SV_FOOD_UNHEALTH)))) {
				//p_ptr->cst -= 2;
				inven_item_increase(Ind, i, -1);
				inven_item_describe(Ind, i);
				inven_item_optimize(Ind, i);
				set_brand(Ind, 50 + randint(20), TBRAND_POIS, 0);
				break;
			}
		if (i == INVEN_WIELD) {
			msg_print(Ind, "You are missing a poisonous ingredient.");
			return;
		}
		break_shadow_running(Ind);
		stop_precision(Ind);
		stop_shooting_till_kill(Ind);
		msg_print(Ind, "You apply the poisonous essence to your weapon..");
		p_ptr->energy -= level_speed(&p_ptr->wpos); /* prepare the shit.. */
s_printf("TECHNIQUE_MELEE: %s - apply poison\n", p_ptr->name);
		p_ptr->warning_technique_melee = 1;
		break;
	case 6:	if (!(p_ptr->melee_techniques & MT_TRACKANIM)) return; /* Track Animals */
		if (p_ptr->cst < 3) { msg_print(Ind, "Not enough stamina!"); return; }
		p_ptr->cst -= 3;
		p_ptr->energy -= level_speed(&p_ptr->wpos);
		(void)detect_creatures_xxx(Ind, RF3_ANIMAL);
s_printf("TECHNIQUE_MELEE: %s - track animals\n", p_ptr->name);
		p_ptr->warning_technique_melee = 1;
		break;
	case 7:	if (!(p_ptr->melee_techniques & MT_DETNOISE)) return; /* Perceive Noise */
		if (p_ptr->cst < 3) { msg_print(Ind, "Not enough stamina!"); return; }
		p_ptr->cst -= 3;
		p_ptr->energy -= level_speed(&p_ptr->wpos);
		detect_noise(Ind);
s_printf("TECHNIQUE_MELEE: %s - perceive noise\n", p_ptr->name);
		p_ptr->warning_technique_melee = 1;
		break;
	case 8:	if (!(p_ptr->melee_techniques & MT_FLASH)) return; /* Flash bomb */
		if (p_ptr->cst < 4) { msg_print(Ind, "Not enough stamina!"); return; }
		//if (p_ptr->energy < level_speed(&p_ptr->wpos)) return;
		if (p_ptr->energy <= 0) return;
		p_ptr->cst -= 4;
		p_ptr->energy -= level_speed(&p_ptr->wpos);
		un_afk_idle(Ind);
		break_cloaking(Ind, 0);
		break_shadow_running(Ind);
		stop_precision(Ind);
		stop_shooting_till_kill(Ind);

		flash_bomb(Ind);
s_printf("TECHNIQUE_MELEE: %s - flash bomb\n", p_ptr->name);
		p_ptr->warning_technique_melee = 1;
		break;
	case 10: if (!(p_ptr->melee_techniques & MT_SPIN)) return; /* Spin */
		if (p_ptr->cst < 5) { msg_print(Ind, "Not enough stamina!"); return; }
		if (p_ptr->afraid) {
			msg_print(Ind, "You are too afraid to attack!");
			return;
		}
		if (p_ptr->energy < level_speed(&p_ptr->wpos)) return; // ?
		p_ptr->cst -= 5;
		un_afk_idle(Ind);
		break_cloaking(Ind, 0);
		break_shadow_running(Ind);
		stop_precision(Ind);
		stop_shooting_till_kill(Ind);

		spin_attack(Ind);
		p_ptr->energy -= level_speed(&p_ptr->wpos);
s_printf("TECHNIQUE_MELEE: %s - spin\n", p_ptr->name);
		p_ptr->warning_technique_melee = 1;
		break;
#ifdef ENABLE_ASSASSINATE
	case 11: if (!(p_ptr->melee_techniques & MT_ASSA)) return; /* Assassinate */
		if (p_ptr->piercing_charged) {
			msg_print(Ind, "You drop your preparations for assassination.");
			p_ptr->piercing_charged = FALSE;
			p_ptr->piercing = 0;
			return;
		}
		if (p_ptr->cst < 9) { msg_print(Ind, "Not enough stamina!"); return; }
		if (!p_ptr->dual_wield || !p_ptr->dual_mode) {
			msg_print(Ind, "This attack requires two dual-wielded weapons!");
			return;
		}
		stop_precision(Ind);
		stop_shooting_till_kill(Ind);

		msg_print(Ind, "You prepare an armour-piercing attack combination..");
		p_ptr->piercing = 1000; /* [1000] : one round of piercing blows */
		p_ptr->piercing_charged = TRUE; /* Prepared and ready - ST won't regenerate until we use it! */
s_printf("TECHNIQUE_MELEE: %s - assassinate\n", p_ptr->name);
		p_ptr->warning_technique_melee = 1;
		break;
#endif
	case 12 :if (!(p_ptr->melee_techniques & MT_BERSERK)) return; /* Berserk */
		if (p_ptr->cst < 10) { msg_print(Ind, "Not enough stamina!"); return; }
		break_cloaking(Ind, 0);
		break_shadow_running(Ind);
		stop_precision(Ind);
		stop_shooting_till_kill(Ind);

		p_ptr->cst -= 10;
		un_afk_idle(Ind);
		hp_player(Ind, 20);
		set_afraid(Ind, 0);
		set_shero(Ind, randint(5) + 15);
s_printf("TECHNIQUE_MELEE: %s - berserk\n", p_ptr->name);
		p_ptr->warning_technique_melee = 1;
		break;
	case 14 :if (!(p_ptr->melee_techniques & MT_SRUN)) return; /* Shadow Run */
		shadow_run(Ind);
s_printf("TECHNIQUE_MELEE: %s - shadow run\n", p_ptr->name);
		p_ptr->warning_technique_melee = 1;
		break;
	default:
		msg_print(Ind, "Invalid technique.");
		s_printf("TECHNIQUE_MELEE: %s used invalid code %d.\n", p_ptr->name, technique);
		return;
	}

	p_ptr->redraw |= (PR_STAMINA);
	redraw_stuff(Ind);
}

void do_cmd_ranged_technique(int Ind, int technique) {
	player_type *p_ptr = Players[Ind];
	int i;

	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster) {
		msg_print(Ind, "You cannot use techniques while transformed.");
		return;
	}
	if (p_ptr->ghost) {
		msg_print(Ind, "You cannot use techniques as a ghost.");
		return;
	}
	if (p_ptr->confused) {
		msg_print(Ind, "You cannot use techniques while confused.");
		return;
	}
	if (p_ptr->blind) {
		msg_print(Ind, "You cannot use ranged techniques while blinded.");
		return;
	}

	if (!get_skill(p_ptr, SKILL_ARCHERY)) return; /* paranoia */

	if (technique != 3 || !p_ptr->ranged_double) { /* just toggling that one off? */
		if (technique != 2) {
			if (!p_ptr->inventory[INVEN_AMMO].tval) {
				msg_print(Ind, "You have no ammunition equipped.");
				return;
			}
			if (p_ptr->inventory[INVEN_AMMO].sval == SV_AMMO_CHARRED) {
				msg_print(Ind, "Charred ammunition is too brittle to use.");
				return;
			}
		}
		if (p_ptr->inventory[INVEN_BOW].tval == TV_BOOMERANG) {
			msg_print(Ind, "You cannot use techniques with a boomerang.");
			return;
		}
		if (p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD) {
			msg_print(Ind, "You cannot use techniques with a shield equipped.");
			return;
		}
	}

	disturb(Ind, 1, 0); /* stop things like running, resting.. */

	switch (technique) {
	case 0:	if (!(p_ptr->ranged_techniques & RT_FLARE)) return; /* Flare missile */
		if (p_ptr->ranged_flare) {
			msg_print(Ind, "You dispose of the flare missile.");
			p_ptr->ranged_flare = FALSE;
			return;
		}
		if (p_ptr->cst < 2) { msg_print(Ind, "Not enough stamina!"); return; }
		if (check_guard_inscription(p_ptr->inventory[INVEN_AMMO].note, 'k')) {
			msg_print(Ind, "Your ammo's inscription (!k) prevents using it as flare missile.");
			return;
		}
#if 0 /* using !k inscription in birth.c instead? */
		/* warn about and prevent using up the only magic ammo we got */
		if (p_ptr->inventory[INVEN_AMMO].name1 == 0 &&
		    p_ptr->inventory[INVEN_AMMO].sval == SV_AMMO_MAGIC &&
		    p_ptr->inventory[INVEN_AMMO].number == 1) {
			msg_print(Ind, "Flare would consume your only magic piece of ammo!");
			return;
		}
#endif
		for (i = 0; i < INVEN_WIELD; i++)
			if (p_ptr->inventory[i].tval == TV_FLASK) { /* oil */
				//p_ptr->cst -= 2;
				p_ptr->ranged_flare = TRUE;
				inven_item_increase(Ind, i, -1);
				inven_item_describe(Ind, i);
				inven_item_optimize(Ind, i);
				break;
			}
		if (!p_ptr->ranged_flare) {
			msg_print(Ind, "You are missing a flask of oil.");
			return;
		}
		break_shadow_running(Ind);
		stop_shooting_till_kill(Ind);
		p_ptr->ranged_precision = FALSE; p_ptr->ranged_double = FALSE; p_ptr->ranged_barrage = FALSE;
		p_ptr->energy -= level_speed(&p_ptr->wpos); /* prepare the shit.. */
		msg_print(Ind, "You prepare an oil-drenched shot..");
s_printf("TECHNIQUE_RANGED: %s - flare missile\n", p_ptr->name);
		p_ptr->warning_technique_ranged = 1;
		break;
	case 1:	if (!(p_ptr->ranged_techniques & RT_PRECS)) return; /* Precision shot */
		if (p_ptr->ranged_precision) {
			msg_print(Ind, "You stop aiming overly precisely.");
			p_ptr->ranged_precision = FALSE;
			return;
		}
		if (p_ptr->cst < 7) { msg_print(Ind, "Not enough stamina!"); return; }
		//p_ptr->cst -= 7;
		break_shadow_running(Ind);
		stop_shooting_till_kill(Ind);
		p_ptr->ranged_flare = FALSE; p_ptr->ranged_double = FALSE; p_ptr->ranged_barrage = FALSE;
		p_ptr->ranged_precision = TRUE;
		p_ptr->energy -= level_speed(&p_ptr->wpos); /* focus.. >:) (maybe even 2 turns oO) */
		msg_print(Ind, "You aim carefully for a precise shot..");
s_printf("TECHNIQUE_RANGED: %s - precision\n", p_ptr->name);
		p_ptr->warning_technique_ranged = 1;
		break;
	case 2:	if (!(p_ptr->ranged_techniques & RT_CRAFT)) return; /* Craft some ammunition */
s_printf("TECHNIQUE_RANGED: %s - ammo\n", p_ptr->name);
		p_ptr->warning_technique_ranged = 1;
		do_cmd_fletchery(Ind); /* was previously MKEY_FLETCHERY (9) */
		return;
	case 3:	if (!(p_ptr->ranged_techniques & RT_DOUBLE)) return; /* Double-shot */
		if (!p_ptr->ranged_double) {
			//if (p_ptr->cst < 1) { msg_print(Ind, "Not enough stamina!"); return; }
			if (p_ptr->inventory[INVEN_AMMO].tval && p_ptr->inventory[INVEN_AMMO].number < 2) {
				msg_print(Ind, "You need at least 2 projectiles for a dual-shot!");
				return;
			}
			p_ptr->ranged_double_used = 0;
			p_ptr->ranged_flare = FALSE; p_ptr->ranged_precision = FALSE; p_ptr->ranged_barrage = FALSE;
		}
		break_shadow_running(Ind);
		p_ptr->ranged_double = !p_ptr->ranged_double; /* toggle */
		if (p_ptr->ranged_double) msg_print(Ind, "You switch to shooting double shots.");
		else msg_print(Ind, "You stop using double shots.");
s_printf("TECHNIQUE_RANGED: %s - double\n", p_ptr->name);
		p_ptr->warning_technique_ranged = 1;
		break;
	case 4:	if (!(p_ptr->ranged_techniques & RT_BARRAGE)) return; /* Barrage */
		if (p_ptr->ranged_barrage) {
			msg_print(Ind, "You cancel preparations for barrage.");
			p_ptr->ranged_barrage = FALSE;
			return;
		}
		if (p_ptr->cst < 9) { msg_print(Ind, "Not enough stamina!"); return; }
		if (p_ptr->inventory[INVEN_AMMO].tval && p_ptr->inventory[INVEN_AMMO].number < 6) {
			msg_print(Ind, "You need at least 6 projectiles for a barrage!");
			return;
		}
		//p_ptr->cst -= 9;
		break_shadow_running(Ind);
		stop_shooting_till_kill(Ind);
		p_ptr->ranged_flare = FALSE; p_ptr->ranged_precision = FALSE; p_ptr->ranged_double = FALSE;
		p_ptr->ranged_barrage = TRUE;
		msg_print(Ind, "You prepare a powerful multi-shot barrage...");
s_printf("TECHNIQUE_RANGED: %s - barrage\n", p_ptr->name);
		p_ptr->warning_technique_ranged = 1;
//in cmd2.c!	p_ptr->energy -= level_speed(&p_ptr->wpos) / 2; /* You _prepare_ it.. */
		break;
	}
}

void do_cmd_breathe(int Ind) {
	player_type *p_ptr = Players[Ind];

#ifndef ENABLE_DRACONIAN_TRAITS
	return;
#endif

	p_ptr->current_breath = 1;
	get_aim_dir(Ind);
}
void do_cmd_breathe_aux(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	int trait;

#ifndef ENABLE_DRACONIAN_TRAITS
	return;
#endif
	/* feature not available? */
	if (!p_ptr->ptrait) return;

	if (!is_newer_than(&p_ptr->version, 4, 4, 5, 10, 0, 0)) {
		/* Only fire in direction 5 if we have a target */
		if ((dir == 5) && !target_okay(Ind)) {
			/* Reset current breath */
			p_ptr->current_breath = 0;
			return;
		}
	}

	if (p_ptr->prace != RACE_DRACONIAN || !p_ptr->ptrait) {
		msg_print(Ind, "You cannot breathe elements.");
		return;
	}
	if (p_ptr->ghost) {
		msg_print(Ind, "You cannot use your elemental breath as a ghost.");
		return;
	}
	if (p_ptr->confused) {
		msg_print(Ind, "You cannot use your elemental breath while confused.");
		return;
	}
	if (p_ptr->body_monster && !strchr("dDJRM", r_info[p_ptr->body_monster].d_char)) {
		msg_print(Ind, "You cannot use your elemental breath in your current form.");
		return;
	}
	if (p_ptr->lev < 8) {
		msg_print(Ind, "You need to be at least level 8 to breathe elements.");
		return;
	}
	if (p_ptr->cst < 3) { msg_print(Ind, "Not enough stamina!"); return; }

	/* New '+' feat in 4.4.6.2 */
	if (dir == 11) {
		get_aim_dir(Ind);
		p_ptr->current_breath = 1;
		return;
	}

	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);
	un_afk_idle(Ind);
	disturb(Ind, 1, 0); /* stop things like running, resting.. */

	p_ptr->cst -= 3;
	p_ptr->redraw |= PR_STAMINA;
	p_ptr->current_breath = 0;
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	trait = p_ptr->ptrait;
	if (trait == TRAIT_MULTI) /* Draconic Multi-hued */
		trait = rand_int(5) + TRAIT_BLUE;
	if (trait == TRAIT_POWER) {
		trait = rand_int(10) + TRAIT_BLUE;
		if (trait >= TRAIT_MULTI) trait++;
	}

	switch (trait) {
	case TRAIT_BLUE: /* Draconic Blue */
		sprintf(p_ptr->attacker, " breathes lightning for");
		msg_print(Ind, "You breathe lightning.");
		fire_ball(Ind, GF_ELEC, dir, ((p_ptr->chp / 3) > 500) ? 500 : (p_ptr->chp / 3), 2, p_ptr->attacker);
		break;
	case TRAIT_WHITE: /* Draconic White */
		sprintf(p_ptr->attacker, " breathes frost for");
		msg_print(Ind, "You breathe frost.");
		fire_ball(Ind, GF_COLD, dir, ((p_ptr->chp / 3) > 500) ? 500 : (p_ptr->chp / 3), 2, p_ptr->attacker);
		break;
	case TRAIT_RED: /* Draconic Red */
		sprintf(p_ptr->attacker, " breathes fire for");
		msg_print(Ind, "You breathe fire.");
		fire_ball(Ind, GF_FIRE, dir, ((p_ptr->chp / 3) > 500) ? 500 : (p_ptr->chp / 3), 2, p_ptr->attacker);
		break;
	case TRAIT_BLACK: /* Draconic Black */
		sprintf(p_ptr->attacker, " breathes acid for");
		msg_print(Ind, "You breathe acid.");
		fire_ball(Ind, GF_ACID, dir, ((p_ptr->chp / 3) > 500) ? 500 : (p_ptr->chp / 3), 2, p_ptr->attacker);
		break;
	case TRAIT_GREEN: /* Draconic Green */
		sprintf(p_ptr->attacker, " breathes poison for");
		msg_print(Ind, "You breathe poison.");
		fire_ball(Ind, GF_POIS, dir, ((p_ptr->chp / 3) > 450) ? 450 : (p_ptr->chp / 3), 2, p_ptr->attacker);
		break;
	case TRAIT_BRONZE: /* Draconic Bronze */
		sprintf(p_ptr->attacker, " breathes confusion for");
		msg_print(Ind, "You breathe confusion.");
		fire_ball(Ind, GF_CONFUSION, dir, ((p_ptr->chp / 3) > 350) ? 350 : (p_ptr->chp / 3), 2, p_ptr->attacker);
		break;
	case TRAIT_SILVER: /* Draconic Silver */
		sprintf(p_ptr->attacker, " breathes inertia for");
		msg_print(Ind, "You breathe inertia.");
		fire_ball(Ind, GF_INERTIA, dir, ((p_ptr->chp / 3) > 500) ? 500 : (p_ptr->chp / 3), 2, p_ptr->attacker);
		break;
	case TRAIT_GOLD: /* Draconic Gold */
		sprintf(p_ptr->attacker, " breathes sound for");
		msg_print(Ind, "You breathe sound.");
		fire_ball(Ind, GF_SOUND, dir, ((p_ptr->chp / 3) > 350) ? 350 : (p_ptr->chp / 3), 2, p_ptr->attacker);
		break;
	case TRAIT_LAW: /* Draconic Law */
		sprintf(p_ptr->attacker, " breathes shards for");
		msg_print(Ind, "You breathe shards.");
		fire_ball(Ind, GF_SHARDS, dir, ((p_ptr->chp / 3) > 350) ? 350 : (p_ptr->chp / 3), 2, p_ptr->attacker);
		break;
	case TRAIT_CHAOS: /* Draconic Chaos */
		sprintf(p_ptr->attacker, " breathes chaos for");
		msg_print(Ind, "You breathe chaos.");
		fire_ball(Ind, GF_CHAOS, dir, ((p_ptr->chp / 3) > 450) ? 450 : (p_ptr->chp / 3), 2, p_ptr->attacker);
		break;
	case TRAIT_BALANCE: /* Draconic Balance */
		sprintf(p_ptr->attacker, " breathes disenchantment for");
		msg_print(Ind, "You breathe disenchantment.");
		fire_ball(Ind, GF_DISENCHANT, dir, ((p_ptr->chp / 3) > 400) ? 400 : (p_ptr->chp / 3), 2, p_ptr->attacker);
		break;
	default: /* paranoia */
		msg_print(Ind, "\377yYou fail to breathe elements.");
		p_ptr->cst += 3; /* reimburse */
	}
}

bool create_snowball(int Ind, cave_type *c_ptr) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;

	if (cold_place(wpos) && /* during winter we can make snowballs~ */
	    /* not in dungeons (helcaraxe..?) -- IC we assume they're just ice, not really snow, snow is only found on the world surface :p */
	    !wpos->wz &&
	    /* not inside houses or on house walls (wraithform into own house) */
	    !(c_ptr->info & CAVE_ICKY) && !((f_info[c_ptr->feat].flags1 & FF1_WALL) && (f_info[c_ptr->feat].flags1 & FF1_PERMANENT)) &&
	    /* must be floor or tree/bush to grab snow from, just not solid walls basically: */
	    (cave_floor_grid(c_ptr) || c_ptr->feat == FEAT_BUSH || c_ptr->feat == FEAT_TREE || c_ptr->feat == FEAT_DEAD_TREE || c_ptr->feat == FEAT_MOUNTAIN) &&
	    /* there must be snow here actually, ie shaded white (hackz) */
	    manipulate_cave_colour_season(c_ptr, wpos, p_ptr->px, p_ptr->py, f_info[c_ptr->feat].f_attr) == TERM_WHITE) {
		object_type forge;

		invcopy(&forge, lookup_kind(TV_GAME, SV_SNOWBALL));
		forge.number = 1;
		object_aware(Ind, &forge);
		object_known(&forge);
		forge.discount = 0;
		forge.level = 1; //not 0 :)
		forge.ident |= ID_MENTAL;
		forge.iron_trade = p_ptr->iron_trade;

		/* Can melt: We don't use o_ptr->timeout because
		    a) it needs too many extra checks everywhere and
		    b) timeout is shown while pval is hidden, which is nice! */
		forge.pval = 150 + rand_int(51);
		inven_carry(Ind, &forge);
		msg_print(Ind, "You pick up some snow and form a snowball.");

		/* Take a turn */
		p_ptr->energy -= level_speed(&p_ptr->wpos);

		return TRUE;
	}
	return FALSE;
}
