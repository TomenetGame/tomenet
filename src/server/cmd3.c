/* $Id$ */
/* File: cmd3.c */

/* Purpose: Inventory commands */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"


/* Allow fruit bats to wear body armour? [experimental, disabled] */
#define BATS_ALLOW_BODY


bool bypass_inscrption = FALSE;
/*
 * Move an item from equipment list to pack
 * Note that only one item at a time can be wielded per slot.
 * Note that taking off an item when "full" will cause that item
 * to fall to the ground.
 */
void inven_takeoff(int Ind, int item, int amt, bool called_from_wield) {
	player_type	*p_ptr = Players[Ind];
	int		posn; //, j;
	object_type	*o_ptr;
	object_type	tmp_obj;
	cptr		act;
	char		o_name[ONAME_LEN];


	/* Get the item to take off */
	o_ptr = &(p_ptr->inventory[item]);

	/* Paranoia */
	if (amt <= 0) return;

	if ((!bypass_inscrption) && check_guard_inscription( o_ptr->note, 't' )) {
		msg_print(Ind, "The item's inscription prevents it.");
		return;
	}
	bypass_inscrption = FALSE;

#ifdef VAMPIRES_INV_CURSED
	if (p_ptr->prace == RACE_VAMPIRE) reverse_cursed(o_ptr);
 #ifdef ENABLE_HELLKNIGHT
	else if (p_ptr->pclass == CLASS_HELLKNIGHT) reverse_cursed(o_ptr); //them too!
 #endif
 #ifdef ENABLE_CPRIEST
	else if (p_ptr->pclass == CLASS_CPRIEST && p_ptr->body_monster == RI_BLOODTHIRSTER) reverse_cursed(o_ptr);
 #endif
#endif

	/* Sigil (reset it) */
	if (o_ptr->sigil) {
		msg_print(Ind, "The sigil fades away.");
		o_ptr->sigil = 0;
		o_ptr->sseed = 0;
	}

	if (p_ptr->bow_brand && item == INVEN_AMMO) set_bow_brand(Ind, 0, 0, 0);
	/* for now, if one of dual-wielded weapons is stashed away the brand fades for both..
	   could use o_ptr->xtraX to mark them specifically maybe, but also requires distinct messages, maybe too much. */
	else if (p_ptr->brand && (item == INVEN_WIELD || /* dual-wield */
	    (item == INVEN_ARM && o_ptr->tval != TV_SHIELD))) set_brand(Ind, 0, 0, 0);

	/* Verify */
	if (amt > o_ptr->number) amt = o_ptr->number;

	/* Make a copy to carry */
	tmp_obj = *o_ptr;
	tmp_obj.number = amt;

	/* What are we "doing" with the object */
	if (amt < o_ptr->number)
		act = "Took off";
	else if (item == INVEN_WIELD)
		act = "You were wielding";
	else if (item == INVEN_ARM)
		act = "You were wielding";
	else if (item == INVEN_BOW)
		act = "You were shooting with";
	else if (item == INVEN_LITE)
		act = "Light source was";
	/* Took off ammo */
	else if (item == INVEN_AMMO)
		act = "You were carrying in your quiver";
	/* Took off tool */
	else if (item == INVEN_TOOL)
		act = "You were using";
	else
		act = "You were wearing";

#ifdef USE_SOUND_2010
	sound_item(Ind, o_ptr->tval, o_ptr->sval, "takeoff_");
#endif

#if 0 //DSMs don't poly anymore due to cheeziness. They breathe instead.
	/* Polymorph back */
	/* XXX this can cause strange things for players with mimicry skill.. */
	if ((item == INVEN_BODY) && (o_ptr->tval == TV_DRAG_ARMOR)) {
		/* Well, so we gotta check if the player, in case he is a
		mimic, is using a form that can _only_ come from the armor */
		//if (p_ptr->pclass == CLASS_MIMIC) //Adventurers can also have mimic skill
		//{
			switch (o_ptr->sval) {
			case SV_DRAGON_BLACK:
			j = race_index("Ancient black dragon"); break;
			case SV_DRAGON_BLUE:
			j = race_index("Ancient blue dragon"); break;
			case SV_DRAGON_WHITE:
			j = race_index("Ancient white dragon"); break;
			case SV_DRAGON_RED:
			j = race_index("Ancient red dragon"); break;
			case SV_DRAGON_GREEN:
			j = race_index("Ancient green dragon"); break;
			case SV_DRAGON_MULTIHUED:
			j = race_index("Ancient multi-hued dragon"); break;
			case SV_DRAGON_PSEUDO:
			j = race_index("Ethereal drake"); break;
			//j = race_index("Pseudo dragon"); break;
			case SV_DRAGON_SHINING:
			j = race_index("Ethereal dragon"); break;
			case SV_DRAGON_LAW:
			j = race_index("Great Wyrm of Law"); break;
			case SV_DRAGON_BRONZE:
			j = race_index("Ancient bronze dragon"); break;
			case SV_DRAGON_GOLD:
			j = race_index("Ancient gold dragon"); break;
			case SV_DRAGON_CHAOS:
			j = race_index("Great Wyrm of Chaos"); break;
			case SV_DRAGON_BALANCE:
			j = race_index("Great Wyrm of Balance"); break;
			case SV_DRAGON_POWER:
			j = race_index("Great Wyrm of Power"); break;
			}
			if ((p_ptr->body_monster == j) &&
			    ((p_ptr->r_mimicry[j] < r_info[j].level) || 
			    (r_info[j].level > get_skill_scale(p_ptr, SKILL_MIMIC, 100))))
				do_mimic_change(Ind, 0, TRUE);
		/*}
		else do_mimic_change(Ind, 0, TRUE);
		*/
	}
#endif

#if POLY_RING_METHOD == 0
	/* Polymorph back */
	/* XXX this can cause strange things for players with mimicry skill.. */
	if ((item == INVEN_LEFT || item == INVEN_RIGHT) && (o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)) {
		if ((p_ptr->body_monster == o_ptr->pval) && 
		    ((p_ptr->r_mimicry[p_ptr->body_monster] < r_info[p_ptr->body_monster].level) ||
		    (get_skill_scale(p_ptr, SKILL_MIMIC, 100) < r_info[p_ptr->body_monster].level))) {
			/* If player hasn't got high enough kill count anymore now, poly back to player form! */
			msg_print(Ind, "You polymorph back to your normal form.");
			do_mimic_change(Ind, 0, TRUE);
		}
	}
#endif

	/* Check if item gave WRAITH form */
	if ((k_info[o_ptr->k_idx].flags3 & TR3_WRAITH) && p_ptr->tim_wraith)
		p_ptr->tim_wraith = 1;

	/* Artifacts */
	if (o_ptr->name1) {
		artifact_type *a_ptr;
		/* Obtain the artifact info */
		if (o_ptr->name1 == ART_RANDART)
			a_ptr = randart_make(o_ptr);
		else
			a_ptr = &a_info[o_ptr->name1];

		if ((a_ptr->flags3 & TR3_WRAITH) && p_ptr->tim_wraith) p_ptr->tim_wraith = 1;
	}

	/* Carry the object, saving the slot it went in */
	posn = inven_carry(Ind, &tmp_obj);

	/* Handles overflow */
	pack_overflow(Ind);


	/* Describe the result */
	if (amt < o_ptr->number)
		object_desc(Ind, o_name, &tmp_obj, TRUE, 3);
	else
		object_desc(Ind, o_name, o_ptr, TRUE, 3);
	msg_format(Ind, "%^s %s (%c).", act, o_name, index_to_label(posn));

	if (!called_from_wield && p_ptr->prace == RACE_HOBBIT && o_ptr->tval == TV_BOOTS)
		msg_print(Ind, "\377gYou feel more dextrous now, being barefeet.");

	/* Delete (part of) it */
	inven_item_increase(Ind, item, -amt);
	inven_item_optimize(Ind, item);

#ifdef ENABLE_STANCES
	/* take care of combat stances */
 #ifndef ALLOW_SHIELDLESS_DEFENSIVE_STANCE
	if ((item == INVEN_ARM && p_ptr->combat_stance == 1) ||
	    (item == INVEN_WIELD && p_ptr->combat_stance == 2)) {
 #else
	if (p_ptr->combat_stance &&
	    ((item == INVEN_ARM && !p_ptr->inventory[INVEN_WIELD].k_idx) ||
	    (item == INVEN_WIELD && (
	    !p_ptr->inventory[INVEN_ARM].k_idx || p_ptr->combat_stance == 2)))) {
 #endif
		msg_print(Ind, "\377sYou return to balanced combat stance.");
		p_ptr->combat_stance = 0;
		p_ptr->update |= (PU_BONUS);
		p_ptr->redraw |= (PR_PLUSSES | PR_STATE);
	}
#endif

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);
	/* Recalculate torch */
	p_ptr->update |= (PU_TORCH);
	/* Recalculate mana */
	p_ptr->update |= (PU_MANA | PU_HP | PU_SANITY);
	/* Redraw */
	p_ptr->redraw |= (PR_PLUSSES);
	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
}




/*
 * Drops (some of) an item from inventory to "near" the current location
 */
int inven_drop(int Ind, int item, int amt) {
	player_type *p_ptr = Players[Ind];

	object_type		*o_ptr;
	object_type		 tmp_obj;

	cptr		act;
	int		o_idx;
	char		o_name[ONAME_LEN];

	quest_info *q_ptr;


	/* Access the slot to be dropped */
	o_ptr = &(p_ptr->inventory[item]);

	/* Not too many */
	if (amt > o_ptr->number) amt = o_ptr->number;
	/* Error check/Nothing done? */
	if (amt <= 0) return -1;

	/* check for !d  or !* in inscriptions */
	if (!bypass_inscrption && check_guard_inscription(o_ptr->note, 'd')) {
		msg_print(Ind, "The item's inscription prevents it.");
		return -1;
	}
	bypass_inscrption = FALSE;

#ifdef USE_SOUND_2010
	sound_item(Ind, o_ptr->tval, o_ptr->sval, "drop_");
#endif

#ifdef VAMPIRES_INV_CURSED
	if (p_ptr->prace == RACE_VAMPIRE) reverse_cursed(o_ptr);
 #ifdef ENABLE_HELLKNIGHT
	else if (p_ptr->pclass == CLASS_HELLKNIGHT) reverse_cursed(o_ptr); //them too!
 #endif
 #ifdef ENABLE_CPRIEST
	else if (p_ptr->pclass == CLASS_CPRIEST && p_ptr->body_monster == RI_BLOODTHIRSTER) reverse_cursed(o_ptr);
 #endif
#endif

	/* Make a "fake" object */
	tmp_obj = *o_ptr;
	tmp_obj.number = amt;

	/*
	 * Hack -- If rods or wands are dropped, the total maximum timeout or 
	 * charges need to be allocated between the two stacks.  If all the items 
	 * are being dropped, it makes for a neater message to leave the original 
	 * stack's pval alone. -LM-
	 */
	if (is_magic_device(o_ptr->tval)) divide_charged_item(&tmp_obj, o_ptr, amt);

	/* What are we "doing" with the object */
	if (amt < o_ptr->number)
		act = "Dropped";
	else if (item == INVEN_WIELD)
		act = "Was wielding";
	else if (item == INVEN_ARM)
		act = "Was wielding";
	else if (item == INVEN_BOW)
		act = "Was shooting with";
	else if (item == INVEN_LITE)
		act = "Light source was";
	else if (item >= INVEN_WIELD)
		act = "Was wearing";
	else
		act = "Dropped";

	/* Message */
	object_desc(Ind, o_name, &tmp_obj, TRUE, 3);

#if 0 //DSMs don't poly anymore due to cheeziness. They breathe instead.
	/* Polymorph back */
	if ((item == INVEN_BODY) && (o_ptr->tval == TV_DRAG_ARMOR)) {
		/* Well, so we gotta check if the player, in case he is a
		mimic, is using a form that can _only_ come from the armor */
		//if (p_ptr->pclass == CLASS_MIMIC) //Adventurers can also have mimic skill
		//{
			switch (o_ptr->sval) {
			case SV_DRAGON_BLACK:
			j = race_index("Ancient black dragon"); break;
			case SV_DRAGON_BLUE:
			j = race_index("Ancient blue dragon"); break;
			case SV_DRAGON_WHITE:
			j = race_index("Ancient white dragon"); break;
			case SV_DRAGON_RED:
			j = race_index("Ancient red dragon"); break;
			case SV_DRAGON_GREEN:
			j = race_index("Ancient green dragon"); break;
			case SV_DRAGON_MULTIHUED:
			j = race_index("Ancient multi-hued dragon"); break;
			case SV_DRAGON_PSEUDO:
			j = race_index("Ethereal drake"); break;
			//j = race_index("Pseudo dragon"); break;
			case SV_DRAGON_SHINING:
			j = race_index("Ethereal dragon"); break;
			case SV_DRAGON_LAW:
			j = race_index("Great Wyrm of Law"); break;
			case SV_DRAGON_BRONZE:
			j = race_index("Ancient bronze dragon"); break;
			case SV_DRAGON_GOLD:
			j = race_index("Ancient gold dragon"); break;
			case SV_DRAGON_CHAOS:
			j = race_index("Great Wyrm of Chaos"); break;
			case SV_DRAGON_BALANCE:
			j = race_index("Great Wyrm of Balance"); break;
			case SV_DRAGON_POWER:
			j = race_index("Great Wyrm of Power"); break;
			}
			if ((p_ptr->body_monster == j) &&
			    ((p_ptr->r_mimicry[j] < r_info[j].level) ||
			    (r_info[j].level > get_skill_scale(p_ptr, SKILL_MIMIC, 100))))
				do_mimic_change(Ind, 0, TRUE);
		/*}
		else
		{
			do_mimic_change(Ind, 0, TRUE);
		}*/
	}
#endif

#if POLY_RING_METHOD == 0
	/* Polymorph back */
	/* XXX this can cause strange things for players with mimicry skill.. */
	if ((item == INVEN_LEFT || item == INVEN_RIGHT) &&
	    (o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)) {
		if ((p_ptr->body_monster == o_ptr->pval) && 
		    ((p_ptr->r_mimicry[p_ptr->body_monster] < r_info[p_ptr->body_monster].level) ||
		    (get_skill_scale(p_ptr, SKILL_MIMIC, 100) < r_info[p_ptr->body_monster].level)))
		{
			/* If player hasn't got high enough kill count anymore now, poly back to player form! */
 #if 1
			msg_print(Ind, "You polymorph back to your normal form.");
			do_mimic_change(Ind, 0, TRUE);
 #endif
			s_printf("DROP_EXPLOIT (poly): %s dropped %s\n", p_ptr->name, o_name);
		}
	}
#endif

	/* Check if item gave WRAITH form */
	if ((k_info[o_ptr->k_idx].flags3 & TR3_WRAITH) && p_ptr->tim_wraith && item >= INVEN_WIELD) {
		s_printf("DROP_EXPLOIT (wraith): %s dropped %s\n", p_ptr->name, o_name);
#if 1
		p_ptr->tim_wraith = 1;
#endif
	}

	/* Artifacts */
	if (o_ptr->name1) {
		artifact_type *a_ptr;
		/* Obtain the artifact info */
		if (o_ptr->name1 == ART_RANDART)
			a_ptr = randart_make(o_ptr);
		else
			a_ptr = &a_info[o_ptr->name1];

		if ((a_ptr->flags3 & TR3_WRAITH) && p_ptr->tim_wraith && item >= INVEN_WIELD) {
#if 1
			p_ptr->tim_wraith = 1;
#endif
			s_printf("DROP_EXPLOIT (wraith, art): %s dropped %s\n", p_ptr->name, o_name);
		}
	}

#ifdef ENABLE_STANCES
	/* take care of combat stances */
 #ifndef ALLOW_SHIELDLESS_DEFENSIVE_STANCE
	if ((item == INVEN_ARM && p_ptr->combat_stance == 1) ||
	    (item == INVEN_WIELD && p_ptr->combat_stance == 2)) {
 #else
	if (p_ptr->combat_stance &&
	    ((item == INVEN_ARM && !p_ptr->inventory[INVEN_WIELD].k_idx) ||
	    (item == INVEN_WIELD && (
	    !p_ptr->inventory[INVEN_ARM].k_idx || p_ptr->combat_stance == 2)))) {
 #endif
		msg_print(Ind, "\377sYou return to balanced combat stance.");
		p_ptr->combat_stance = 0;
		p_ptr->update |= (PU_BONUS);
		p_ptr->redraw |= (PR_PLUSSES | PR_STATE);
	}
#endif

	/* Message */
	msg_format(Ind, "%^s %s (%c).", act, o_name, index_to_label(item));

	/* Drop it (carefully) near the player */
	o_idx = drop_near_severe(Ind, &tmp_obj, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);

#ifdef PLAYER_STORES
	o_ptr = &o_list[o_idx];
	if (o_idx >= 0 && o_ptr->note && strstr(quark_str(o_ptr->note), "@S") && !o_ptr->questor
	    && inside_house(&p_ptr->wpos, o_ptr->ix, o_ptr->iy)) {
		object_desc(0, o_name, o_ptr, TRUE, 3);
		s_printf("PLAYER_STORE_OFFER: %s - %s (%d,%d,%d; %d,%d).\n",
		    p_ptr->name, o_name, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz,
		    o_ptr->ix, o_ptr->iy);
		/* appraise the item! */
		(void)price_item_player_store(Ind, o_ptr);
		o_ptr->housed = inside_which_house(&p_ptr->wpos, o_ptr->ix, o_ptr->iy);
	}
#endif

	/* Reattach questors to quest */
	if (o_ptr->questor) {
		q_ptr = &q_info[o_ptr->quest - 1];
		if (!q_ptr->defined || /* this quest no longer exists in q_info.txt? */
		    !q_ptr->active || /* or it's not supposed to be enabled atm? */
		    q_ptr->questors <= o_ptr->questor_idx) { /* ew */
			s_printf("QUESTOR DEPRECATED (on drop) o_idx %d, q_idx %d.\n", o_idx, o_ptr->quest - 1);
			o_ptr->questor = FALSE;
			/* delete him too, maybe? */
		} else q_ptr->questor[o_ptr->questor_idx].mo_idx = o_idx;
	}

	/* Decrease the item, optimize. */
	inven_item_increase(Ind, item, -amt); /* note that this calls the required boni-updating et al */
	inven_item_describe(Ind, item);
	inven_item_optimize(Ind, item);

	break_cloaking(Ind, 5);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);

	return o_idx;
}

/*
 * The "wearable" tester
 */
#if 1 /* new way: 'fruit bat body' is checked first: it's restrictions will be inherited by all other forms */
bool item_tester_hook_wear(int Ind, int slot) {
	player_type *p_ptr = Players[Ind];
	monster_race *r_ptr = NULL;
	bool fishy = FALSE;
	if (p_ptr->body_monster) {
		r_ptr = &r_info[p_ptr->body_monster];
		if (r_ptr->d_char == '~') fishy = TRUE;
	}

	if (p_ptr->fruit_bat) {
		switch(slot) {
		case INVEN_RIGHT:
		case INVEN_LEFT:
		case INVEN_NECK:
		case INVEN_HEAD:
		case INVEN_LITE:
#ifdef BATS_ALLOW_BODY
		case INVEN_BODY:	//allow this mayyyybe?
#endif
		case INVEN_OUTER:
		case INVEN_ARM:
		case INVEN_TOOL:	//allow them to wear, say, picks and shovels - the_sandman
			break; //fine
		default:
			return FALSE; //not fine
		}
	}

	if (p_ptr->body_monster &&
	    (p_ptr->pclass != CLASS_DRUID) &&
	    ((p_ptr->pclass != CLASS_SHAMAN) || !mimic_shaman_fulleq(r_ptr->d_char)) &&
	    (p_ptr->prace != RACE_VAMPIRE)
	    ) {
		switch(slot) {
		case INVEN_WIELD:
		case INVEN_BOW:
			if (r_ptr->body_parts[BODY_WEAPON]) return (TRUE);
			break;
		case INVEN_LEFT:
			if (r_ptr->body_parts[BODY_FINGER] > 1) return (TRUE);
			break;
		case INVEN_RIGHT:
			if (r_ptr->body_parts[BODY_FINGER]) return (TRUE);
			break;
		case INVEN_NECK:
		case INVEN_HEAD:
			if (r_ptr->body_parts[BODY_HEAD]) return (TRUE);
			break;
		case INVEN_LITE:
			/* Always allow to carry light source? :/ */
			/* return (TRUE); break; */
			if (r_ptr->body_parts[BODY_WEAPON]) return (TRUE);
			if (r_ptr->body_parts[BODY_FINGER]) return (TRUE);
			if (r_ptr->body_parts[BODY_HEAD]) return (TRUE);
			if (r_ptr->body_parts[BODY_ARMS]) return (TRUE);
			break;
		case INVEN_BODY:
#ifdef BATS_ALLOW_BODY
			/* note: this check is redundant, because bats actually DO have a torso atm!
			   funnily, native fruit bat players do NOT have one without this option o_O. */
			switch (p_ptr->body_monster) {
			case 37: case 114: case 187: case 235: case 351:
			case 377: case RI_VAMPIRE_BAT: case 406: case 484: case 968:
				return TRUE;
			}
#endif
		case INVEN_OUTER:
		case INVEN_AMMO:
			if (r_ptr->body_parts[BODY_TORSO]) return (TRUE);
			break;
		case INVEN_ARM:
			if (r_ptr->body_parts[BODY_ARMS]) return (TRUE);
			break;
		case INVEN_TOOL:
			/* make a difference :) - C. Blue */
			if (r_ptr->body_parts[BODY_ARMS] ||
//			    r_ptr->body_parts[BODY_FINGER] ||
			    r_ptr->body_parts[BODY_WEAPON]) return (TRUE);
			break;
		case INVEN_HANDS:
			if (fishy) return FALSE;
//			if (r_ptr->body_parts[BODY_FINGER]) return (TRUE); too silyl (and powerful)
//			if (r_ptr->body_parts[BODY_ARMS]) return (TRUE); was standard, but now:
			if (r_ptr->body_parts[BODY_FINGER] && r_ptr->body_parts[BODY_ARMS]) return (TRUE);
			break;
		case INVEN_FEET:
			if (r_ptr->body_parts[BODY_LEGS]) return (TRUE);
			break;
		}
	}

	/* Check for a usable slot */
	else if (slot >= INVEN_WIELD) return (TRUE);

	/* Assume not wearable */
	return (FALSE);
}

#else // old way, deprecated

bool item_tester_hook_wear(int Ind, int slot) {
	player_type *p_ptr = Players[Ind];
	monster_race *r_ptr = NULL;
	bool fishy = FALSE;
	if (p_ptr->body_monster) {
		r_ptr = &r_info[p_ptr->body_monster];
		if (r_ptr->d_char == '~') fishy = TRUE;
	}

	/*
	 * Hack -- restrictions by forms
	 * I'm not quite sure if wielding 6 rings and 3 weps should be allowed..
	 * Shapechanging Druids do not get penalized...

	 * Another hack for shamans (very experimental):
	 * They can use their full equipment in E and G (spirit/elemental/ghost) form.
	 *
	 * Druid bats can't wear full eq.
	 */
#if 0
	if (p_ptr->body_monster &&
	    ((p_ptr->pclass != CLASS_DRUID) || p_ptr->fruit_bat) &&
	    ((p_ptr->pclass != CLASS_SHAMAN) || !mimic_shaman_fulleq(r_ptr->d_char)) &&
	    ((p_ptr->prace != RACE_VAMPIRE) || p_ptr->fruit_bat)
	    )
#else
	if (p_ptr->body_monster &&
	    (p_ptr->pclass != CLASS_DRUID) &&
	    ((p_ptr->pclass != CLASS_SHAMAN) || !mimic_shaman_fulleq(r_ptr->d_char)) &&
	    (p_ptr->prace != RACE_VAMPIRE)
	    )
#endif
	{
		switch(slot) {
		case INVEN_WIELD:
		case INVEN_BOW:
			if (r_ptr->body_parts[BODY_WEAPON]) return (TRUE);
			break;
		case INVEN_LEFT:
			if (r_ptr->body_parts[BODY_FINGER] > 1) return (TRUE);
			break;
		case INVEN_RIGHT:
			if (r_ptr->body_parts[BODY_FINGER]) return (TRUE);
			break;
		case INVEN_NECK:
		case INVEN_HEAD:
			if (r_ptr->body_parts[BODY_HEAD]) return (TRUE);
			break;
		case INVEN_LITE:
			/* Always allow to carry light source? :/ */
			/* return (TRUE); break; */
			if (r_ptr->body_parts[BODY_WEAPON]) return (TRUE);
			if (r_ptr->body_parts[BODY_FINGER]) return (TRUE);
			if (r_ptr->body_parts[BODY_HEAD]) return (TRUE);
			if (r_ptr->body_parts[BODY_ARMS]) return (TRUE);
			break;
		case INVEN_BODY:
#ifdef BATS_ALLOW_BODY
			/* note: this check is redundant, because bats actually DO have a torso atm!
			   funnily, native fruit bat players do NOT have one without this option o_O. */
			switch (p_ptr->body_monster) {
			case 37: case 114: case 187: case 235: case 351:
			case 377: case RI_VAMPIRE_BAT: case 406: case 484: case 968:
				return TRUE;
			}
#endif
		case INVEN_OUTER:
		case INVEN_AMMO:
			if (r_ptr->body_parts[BODY_TORSO]) return (TRUE);
			break;
		case INVEN_ARM:
			if (r_ptr->body_parts[BODY_ARMS]) return (TRUE);
			break;
		case INVEN_TOOL:
			/* make a difference :) - C. Blue */
			if (r_ptr->body_parts[BODY_ARMS] ||
//			    r_ptr->body_parts[BODY_FINGER] ||
			    r_ptr->body_parts[BODY_WEAPON]) return (TRUE);
			break;
		case INVEN_HANDS:
			if (fishy) return FALSE;
//			if (r_ptr->body_parts[BODY_FINGER]) return (TRUE); too silyl (and powerful)
//			if (r_ptr->body_parts[BODY_ARMS]) return (TRUE); was standard, but now:
			if (r_ptr->body_parts[BODY_FINGER] && r_ptr->body_parts[BODY_ARMS]) return (TRUE);
			break;
		case INVEN_FEET:
			if (r_ptr->body_parts[BODY_LEGS]) return (TRUE);
			break;
		}
	}
	/* Restrict fruit bats */
#if 0
	else if (p_ptr->fruit_bat && !p_ptr->body_monster)
#else
	else if (p_ptr->fruit_bat && (
	    !p_ptr->body_monster ||
	    p_ptr->pclass == CLASS_DRUID ||
	    (p_ptr->pclass == CLASS_SHAMAN && !mimic_shaman_fulleq(r_ptr->d_char)) ||
	    p_ptr->prace != RACE_VAMPIRE
	    ))
#endif
	{
		switch(slot) {
		case INVEN_RIGHT:
		case INVEN_LEFT:
		case INVEN_NECK:
		case INVEN_HEAD:
		case INVEN_LITE:
#ifdef BATS_ALLOW_BODY
		case INVEN_BODY:	//allow this mayyyybe?
#endif
		case INVEN_OUTER:
		case INVEN_ARM:
		case INVEN_TOOL:	//allow them to wear, say, picks and shovels - the_sandman
			return TRUE;
		}
	}
#if 0
	else if (r_info[p_ptr->body_monster].flags3 & RF3_DRAGON) {
		switch(slot) {
		case INVEN_WIELD:
		case INVEN_RIGHT:
		case INVEN_LEFT:
		case INVEN_HEAD:
		case INVEN_BODY:
		case INVEN_LITE:
		case INVEN_FEET:
			return TRUE;
		}
	}
#endif
	/* non-fruit bats */
	else {
		/* Check for a usable slot */
		if (slot >= INVEN_WIELD) {
#if 0
			/* use of shield is banned in do_cmd_wield if with 2H weapon.
			 * 3 slots are too severe.. thoughts?	- Jir -
			 */
			if (slot == INVEN_BOW && p_ptr->inventory[INVEN_WIELD].k_idx) {
				u32b f1, f2, f3, f4, f5, f6, esp;
				object_flags(&p_ptr->inventory[INVEN_WIELD], &f1, &f2, &f3, &f4, &f5, &f6, &esp);
				if (f4 & TR4_MUST2H) return(FALSE);
			}
#endif	// 0
			return (TRUE);
		}
	}

	/* Assume not wearable */
	return (FALSE);
}
#endif

/* Take off things that are no more wearable */
void do_takeoff_impossible(int Ind) {
	int k;
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	u32b f1, f2, f3, f4, f5, f6, esp;


	bypass_inscrption = TRUE;
	for (k = INVEN_WIELD; k < INVEN_TOTAL; k++) {
		o_ptr = &p_ptr->inventory[k];
		if ((o_ptr->k_idx) && /* following is a hack for dual-wield.. */
		    (!item_tester_hook_wear(Ind, (k == INVEN_ARM && o_ptr->tval != TV_SHIELD) ? INVEN_WIELD : k)))
		{
			/* Extract the flags */
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

			/* Morgie crown and The One Ring resists! */
			if (f3 & TR3_PERMA_CURSE) continue;

			/* Ahah TAKE IT OFF ! */
			inven_takeoff(Ind, k, 255, FALSE);
		}
		/* new: also redisplay empty slots as '(unavailable)' after a form change, if they are */
		if (!o_ptr->k_idx) Send_equip_availability(Ind, k);
		/* for druids/shamans: climbing set may not work with form even though it is equippable! */
		else if (k == INVEN_TOOL) Send_equip_availability(Ind, k);
	}
	bypass_inscrption = FALSE;
}

/*
 * Wield or wear a single item from the pack or floor
 * Added alt_slots to specify an alternative slot when an item fits in
 * two places - C. Blue
 * 0 = equip in first free slot that fits, if none is free, replace item in first slot (standard/traditional behaviour)
 * 1 = equip in first slot that fits, replacing the item if any
 * 2 = equip in second slot that fits, replacing the item if any
 * 4 = don't equip if slot is already occupied (ie don't replace by taking off an item)
 * Note: Rings make an exception in 4: First ring always goes in second ring slot.
 */
void do_cmd_wield(int Ind, int item, u16b alt_slots) {
	player_type *p_ptr = Players[Ind];

	int slot, num = 1;
	bool item_fits_dual = FALSE, equip_fits_dual = TRUE, all_cursed = FALSE;
	bool slot1 = (p_ptr->inventory[INVEN_WIELD].k_idx != 0);
	bool slot2 = (p_ptr->inventory[INVEN_ARM].k_idx != 0);
	bool alt = ((alt_slots & 0x2) != 0);

	bool ma_warning_weapon = FALSE, ma_warning_shield = FALSE, hobbit_warning = FALSE;

	object_type tmp_obj;
	object_type *o_ptr;
	object_type *x_ptr;

	cptr act;

	char o_name[ONAME_LEN];
	u32b f1 = 0 , f2 = 0 , f3 = 0, f4 = 0, f5, f6 = 0, esp = 0;
	bool highlander = FALSE, warn_takeoff = FALSE;


	/* Catch items that we have already equipped -
	   this can happen when entering items by name (@ key)
	   and it looks kinda bad to get repeated 'You are ...' messages for something already in place. */
	if (item > INVEN_PACK) {
		Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
		return;
	}

	/* Restrict the choices */
	/*item_tester_hook = item_tester_hook_wear;*/


	/* Get the item (in the pack) */
	if (item >= 0) {
		o_ptr = &(p_ptr->inventory[item]);
	} else { /* Get the item (on the floor) */
		if (-item >= o_max) {
			Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
			return; /* item doesn't exist */
		}

		o_ptr = &o_list[0 - item];
	}

	/* erase any interrupting inscriptions on Highlander amulets */
	if (o_ptr->tval == TV_AMULET &&
	    (o_ptr->sval == SV_AMULET_HIGHLANDS || o_ptr->sval == SV_AMULET_HIGHLANDS2)) {
		o_ptr->note = 0;
		highlander = TRUE;
	}

	if (check_guard_inscription(o_ptr->note, 'w')) {
		msg_print(Ind, "The item's inscription prevents it.");
		Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
		return;
	}

	/* Check the slot */
	slot = wield_slot(Ind, o_ptr);

	if (!item_tester_hook_wear(Ind, slot)) {
		msg_print(Ind, "You may not wield that item.");
		Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
		return;
	}

	if (!can_use_verbose(Ind, o_ptr)) {
		Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
		return;
	}

	/* Costumes allowed during halloween and xmas */
	if (!season_halloween && !season_xmas) {
		if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_COSTUME)) {
			msg_print(Ind, "It's not that time of the year anymore.");
			Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
			return;
		}
	}

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* check whether the item to wield is fit for dual-wielding */
	if ((o_ptr->weight <= DUAL_MAX_WEIGHT) &&
	    !(f4 & (TR4_MUST2H | TR4_SHOULD2H))) item_fits_dual = TRUE;
	/* check whether our current equipment allows a dual-wield setup with the new item */
	if (slot1 && (k_info[p_ptr->inventory[INVEN_WIELD].k_idx].flags4 & (TR4_MUST2H | TR4_SHOULD2H)))
		equip_fits_dual = FALSE;

	/* Do we have dual-wield and are trying to equip a weapon?.. */
	if (get_skill(p_ptr, SKILL_DUAL) && slot == INVEN_WIELD) {
#if 0
		/* Equip in arm slot if weapon slot alreay occupied but arm slot still empty */
		if ((p_ptr->inventory[INVEN_WIELD].k_idx || (alt_slots & 0x2))
		    && (!p_ptr->inventory[INVEN_ARM].k_idx || (alt_slots & 0x2))
		    && !(alt_slots & 0x1)
		    /* If to-wield weapon is 2h or 1.5h, choose normal INVEN_WIELD slot instead */
		    && item_fits_dual
		    /* If main-hand weapon is 2h or 1.5h, choose normal INVEN_WIELD slot instead */
		    && equip_fits_dual)
			slot = INVEN_ARM;
#else /* fix: allow 'W' to replace main hand if second hand is empty */
		/* Equip in arm slot if weapon slot alreay occupied but arm slot still empty */
		if (((!slot1 && alt) || (slot2 && alt) || (slot1 && !slot2 && !alt)) &&
		    !(alt_slots & 0x1) &&
		    /* If to-wield weapon is 2h or 1.5h, choose normal INVEN_WIELD slot instead */
		    item_fits_dual &&
		    /* If main-hand weapon is 2h or 1.5h, choose normal INVEN_WIELD slot instead */
		    equip_fits_dual)
			slot = INVEN_ARM;
#endif
	}

	/* to allow only right ring -> only right ring: */
	if (slot == INVEN_LEFT && (alt_slots & 0x2)) slot = INVEN_RIGHT;
	/* to allow no rings -> left ring: */
	else if (slot == INVEN_RIGHT && (alt_slots & 0x2)) slot = INVEN_LEFT;

	/* use the first fitting slot found? (unused) */
	if (slot == INVEN_RIGHT && (alt_slots & 0x1)) slot = INVEN_LEFT;

	if ((alt_slots & 0x4) && p_ptr->inventory[slot].k_idx) {
		object_desc(Ind, o_name, &(p_ptr->inventory[slot]), FALSE, 0);
		msg_format(Ind, "Take off your %s first.", o_name);
		Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
		return;
	}

	/* Prevent wielding into a cursed slot */
	/* Try alternative slots if one is cursed and item can go in multiple places */
	if (cursed_p(&p_ptr->inventory[slot]) && !(alt_slots & 0x3)) {
		switch (slot) {
		case INVEN_LEFT: slot = INVEN_RIGHT; all_cursed = TRUE; break;
		case INVEN_RIGHT: slot = INVEN_LEFT; all_cursed = TRUE; break;
		case INVEN_WIELD: if (get_skill(p_ptr, SKILL_DUAL) && item_fits_dual && equip_fits_dual) { slot = INVEN_ARM; all_cursed = TRUE; break; }
		}
	}
	if (cursed_p(&(p_ptr->inventory[slot]))) {
		/* Describe it */
		object_desc(Ind, o_name, &(p_ptr->inventory[slot]), FALSE, 0);

		/* Message */
		if (all_cursed)
			msg_format(Ind, "The items you are already %s both appear to be cursed.", describe_use(Ind, slot));
		else
			msg_format(Ind, "The %s you are %s appears to be cursed.", o_name, describe_use(Ind, slot));

		/* Cancel the command */
		Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
		return;
	}

	/* Two handed weapons can't be wielded with a shield */
	/* TODO: move to item_tester_hook_wear? */
	if ((f4 & TR4_MUST2H) &&
	    (p_ptr->inventory[INVEN_ARM].k_idx != 0)) {
#if 0 /* a) either give error msg, or.. */
		object_desc(Ind, o_name, o_ptr, FALSE, 0);
		if (get_skill(p_ptr, SKILL_DUAL))
			msg_format(Ind, "You cannot wield your %s with a shield or a secondary weapon.", o_name);
		else
			msg_format(Ind, "You cannot wield your %s with a shield.", o_name);
		Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
		return;
#else /* b) take off the left-hand item too */
		if (check_guard_inscription(p_ptr->inventory[INVEN_ARM].note, 't' )) {
			msg_print(Ind, "Your secondary item's inscription prevents taking it off.");
			Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
			return;
		};
		if (cursed_p(&p_ptr->inventory[INVEN_ARM]) && !is_admin(p_ptr)) {
			msg_print(Ind, "Hmmm, the secondary item you're wielding seems to be cursed.");
			Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
			return;
		}
		inven_takeoff(Ind, INVEN_ARM, 255, TRUE);
		if (item >= 0) {
			item = replay_inven_changes(Ind, item);
			o_ptr = &(p_ptr->inventory[item]);
			if (item == 0xFF) {
				Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
				return; //item is gone, shouldn't happen
			}
		}
#endif
	}

	/* Dual-wield not with 1.5h */
	if ((f4 & TR4_SHOULD2H) &&
	    (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) {
#if 0 /* a) either give error msg, or.. */
		object_desc(Ind, o_name, o_ptr, FALSE, 0);
		msg_format(Ind, "You cannot wield your %s with a secondary weapon.", o_name);
		Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
		return;
#else /* b) take off the secondary weapon */
		if (check_guard_inscription(p_ptr->inventory[INVEN_ARM].note, 't' )) {
			msg_print(Ind, "Your secondary weapon's inscription prevents taking it off.");
			Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
			return;
		};
		if (cursed_p(&p_ptr->inventory[INVEN_ARM]) && !is_admin(p_ptr)) {
			msg_print(Ind, "Hmmm, the secondary weapon you're wielding seems to be cursed.");
			Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
			return;
		}
		inven_takeoff(Ind, INVEN_ARM, 255, TRUE);
		if (item >= 0) {
			item = replay_inven_changes(Ind, item);
			o_ptr = &(p_ptr->inventory[item]);
			if (item == 0xFF) {
				Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
				return; //item is gone, shouldn't happen
			}
		}
#endif
	}

	/* Shields can't be wielded with a two-handed weapon */
	if (o_ptr->tval == TV_SHIELD && (x_ptr = &p_ptr->inventory[INVEN_WIELD])) {
		/* Extract the flags */
		object_flags(x_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		if ((f4 & TR4_MUST2H) && (x_ptr->k_idx)) {
#if 0 /* Prevent shield from being put on if wielding 2H */
			object_desc(Ind, o_name, o_ptr, FALSE, 0);
			msg_format(Ind, "You cannot wield your %s with a two-handed weapon.", o_name);
			Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
			return;
#else /* Take off 2h weapon when equipping a shield */
			if (check_guard_inscription(p_ptr->inventory[INVEN_WIELD].note, 't' )) {
				msg_print(Ind, "Your weapon's inscription prevents taking it off.");
				Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
				return;
			};
			if (cursed_p(&p_ptr->inventory[INVEN_WIELD]) && !is_admin(p_ptr)) {
				msg_print(Ind, "Hmmm, the weapon you're wielding seems to be cursed.");
				Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
				return;
			}
			inven_takeoff(Ind, INVEN_WIELD, 255, TRUE);
			if (item >= 0) {
				item = replay_inven_changes(Ind, item);
				o_ptr = &(p_ptr->inventory[item]);
				if (item == 0xFF) {
					Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
					return; //item is gone, shouldn't happen
				}
			}
#endif
		}
	}


	x_ptr = &(p_ptr->inventory[slot]);

	if (x_ptr->tval != TV_AMULET ||
	    (x_ptr->sval != SV_AMULET_HIGHLANDS && x_ptr->sval != SV_AMULET_HIGHLANDS2))
		highlander = FALSE;

	if (check_guard_inscription(x_ptr->note, 't') && !highlander) {
		msg_print(Ind, "The inscription of your equipped item prevents it.");
		Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
		return;
	};


#if 0
	/* Verify potential overflow */
	if ((p_ptr->inven_cnt >= INVEN_PACK) &&
	    ((item < 0) || (o_ptr->number > 1)))
	{
		/* Verify with the player */
		if (other_query_flag &&
		    !get_check(Ind, "Your pack may overflow.  Continue? ")) return;
	}
#endif

#ifdef USE_SOUND_2010
	sound_item(Ind, o_ptr->tval, o_ptr->sval, "wearwield_");
#endif

	/* Mega-hack -- prevent anyone but total winners from wielding the Massive Iron
	 * Crown of Morgoth or the Mighty Hammer 'Grond'.
	 */
	if (!(p_ptr->total_winner || is_admin(p_ptr))) {
		/* Attempting to wear the crown if you are not a winner is a very, very bad thing
		 * to do.
		 */
		if (o_ptr->name1 == ART_MORGOTH) {
			msg_print(Ind, "You are blasted by the Crown's power!");
			/* This should pierce invulnerability */
			bypass_invuln = TRUE;
			take_hit(Ind, 10000, "the Massive Iron Crown of Morgoth", 0);
			bypass_invuln = FALSE;
			Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
			return;
		}
		/* Attempting to wield Grond isn't so bad. */
		if (o_ptr->name1 == ART_GROND) {
			msg_print(Ind, "You are far too weak to wield the mighty Grond.");
			Send_confirm(Ind, (alt_slots & 0x2) ? PKT_WIELD2 : PKT_WIELD);
			return;
		}
	}

	/* display some warnings if the item will severely conflict with Martial Arts skill */
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS)) {
		if ((is_melee_weapon(o_ptr->tval) ||
		    o_ptr->tval == TV_MSTAFF ||
#ifndef ENABLE_MA_BOOMERANG
		    o_ptr->tval == TV_BOOMERANG ||
#endif
		    o_ptr->tval == TV_BOW)
#ifndef ENABLE_MA_BOOMERANG
		    && !p_ptr->inventory[INVEN_BOW].k_idx
#else
		    && p_ptr->inventory[INVEN_BOW].tval != TV_BOW
#endif
		    && !p_ptr->inventory[INVEN_WIELD].k_idx
		    && (!p_ptr->inventory[INVEN_ARM].k_idx || p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)) /* for dual-wielders */
			ma_warning_weapon = TRUE;
		else if (o_ptr->tval == TV_SHIELD &&
		    (!p_ptr->inventory[INVEN_ARM].k_idx || p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) /* for dual-wielders */
			ma_warning_shield = TRUE;
	}

	process_hooks(HOOK_WIELD, "d", Ind);

	/* Let's not end afk for this - C. Blue */
/*	un_afk_idle(Ind); */

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	/* for Hobbits wearing boots -> give message */
	if (p_ptr->prace == RACE_HOBBIT &&
	    o_ptr->tval == TV_BOOTS && !p_ptr->inventory[slot].k_idx) {
		hobbit_warning = TRUE;
	}

	/* Get a copy of the object to wield */
	tmp_obj = *o_ptr;

	if (slot == INVEN_AMMO) num = o_ptr->number;
	tmp_obj.number = num;

	/* Decrease the item (from the pack) */
	if (item >= 0) {
		inven_item_increase(Ind, item, -num);
		inven_item_optimize(Ind, item);
	}
	/* Decrease the item (from the floor) */
	else {
		floor_item_increase(0 - item, -num);
		floor_item_optimize(0 - item);
	}

	/* Access the wield slot */
	o_ptr = &(p_ptr->inventory[slot]);

	/*** Could make procedure "inven_wield()" ***/
	//no need to try to combine the non esp HL amulet?
	if (highlander) {
		o_ptr->to_h += tmp_obj.to_h;
		o_ptr->to_d += tmp_obj.to_d;
		o_ptr->to_a += tmp_obj.to_a;
		o_ptr->bpval += tmp_obj.bpval; /* this is normal, when it is generated via invcopy() */
		o_ptr->pval += tmp_obj.pval; /* unused, except if it was wished for */
		msg_print(Ind, "\377GThe amulets merge into one!");
	} else {

#if 0
		/* Take off the "entire" item if one is there */
		if (p_ptr->inventory[slot].k_idx) inven_takeoff(Ind, slot, 255, TRUE);
#else	// 0
		/* Take off existing item */
		if (slot != INVEN_AMMO) {
			if (o_ptr->k_idx) {
				/* Take off existing item */
				(void)inven_takeoff(Ind, slot, 255, TRUE);
			}
		} else {
			if (o_ptr->k_idx) {
				/* !M inscription tolerates different +hit / +dam enchantments,
				   which will be merged and averaged in object_absorb.
				   However, this doesn't work for cursed items or artefacts. - C. Blue */
				if (!object_similar(Ind, o_ptr, &tmp_obj, 0x1)) {
					/* Take off existing item */
					(void)inven_takeoff(Ind, slot, 255, TRUE);
				} else {
					// tmp_obj.number += o_ptr->number;
					object_absorb(Ind, &tmp_obj, o_ptr);
				}
			}
		}
#endif	// 0

		/* Wear the new stuff */
		*o_ptr = tmp_obj;

		/* Increase the weight */
		p_ptr->total_weight += o_ptr->weight * num;

		/* Increment the equip counter by hand */
		p_ptr->equip_cnt++;

		/* Where is the item now */
		switch (slot) {
		case INVEN_WIELD:
			act = "You are wielding";
			if (p_ptr->brand) set_brand(Ind, 0, 0, 0); /* actually only applies for dual-wield */
			break;
		case INVEN_ARM:
			act = "You are wielding";
			if (p_ptr->brand && o_ptr->tval != TV_SHIELD) set_brand(Ind, 0, 0, 0); /* dual-wield */
			break;
		case INVEN_BOW: act = "You are shooting with"; break;
		case INVEN_LITE: act = "Your light source is"; break;
		case INVEN_AMMO: act = "In your quiver you have"; break;
		case INVEN_TOOL: act = "You are using"; break;
		default: act = "You are wearing";
		}

		/* Describe the result */
		object_desc(Ind, o_name, o_ptr, TRUE, 3);

		/* Message */
		msg_format(Ind, "%^s %s (%c).", act, o_name, index_to_label(slot));

		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

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

#ifdef VAMPIRES_INV_CURSED
			if (p_ptr->prace == RACE_VAMPIRE) inverse_cursed(o_ptr);
 #ifdef ENABLE_HELLKNIGHT
			else if (p_ptr->pclass == CLASS_HELLKNIGHT) inverse_cursed(o_ptr); //them too!
 #endif
 #ifdef ENABLE_CPRIEST
			else if (p_ptr->pclass == CLASS_CPRIEST && p_ptr->body_monster == RI_BLOODTHIRSTER) inverse_cursed(o_ptr);
 #endif
#endif

			note_toggle_cursed(o_ptr, TRUE);

			/* Force dual-hand mode if wielding cursed weapon(s) */
			if ((slot == INVEN_WIELD || (slot == INVEN_ARM && is_melee_weapon(o_ptr->tval)))
			    && get_skill(p_ptr, SKILL_DUAL)) {
				p_ptr->dual_mode = TRUE;
				p_ptr->redraw |= PR_STATE;
			}
		}

		/* For priests, easy recognition of easily usable BLESSED weapons */
		if (p_ptr->pclass == CLASS_PRIEST && is_melee_weapon(o_ptr->tval) && (f3 & TR3_BLESSED) && !(o_ptr->ident & ID_KNOWN))
			msg_print(Ind, "This weapon appears to be blessed by the gods.");
	}

	/* already done elsewhere */
	//if (slot == INVEN_WIELD && o_ptr->k_idx && (k_info[o_ptr->k_idx].flags4 & TR4_MUST2H)) Send_equip_availability(Ind, INVEN_ARM);

#ifdef ENABLE_STANCES
	/* take care of combat stances */
	if (slot == INVEN_ARM && p_ptr->combat_stance == 2) {
		msg_print(Ind, "\377sYou return to balanced combat stance.");
		p_ptr->combat_stance = 0;
		p_ptr->update |= (PU_BONUS);
		p_ptr->redraw |= (PR_PLUSSES | PR_STATE);
	}
#endif

	/* Only warn about wrong ammo type at the very beginning (for archers, who carry one of each type) */
	if (!p_ptr->warning_ammotype && slot == INVEN_AMMO
	    && p_ptr->inventory[INVEN_BOW].tval == TV_BOW) {
		switch (o_ptr->tval) {
		case TV_SHOT:
			if (p_ptr->inventory[INVEN_BOW].sval != SV_SLING)
				msg_print(Ind, "\377yYou need a sling to fire pebbles or shots.");
			break;
		case TV_ARROW:
			if (p_ptr->inventory[INVEN_BOW].sval != SV_SHORT_BOW &&
			    p_ptr->inventory[INVEN_BOW].sval != SV_LONG_BOW)
				msg_print(Ind, "\377yYou need a bow to fire arrows.");
			break;
		case TV_BOLT:
			if (p_ptr->inventory[INVEN_BOW].sval != SV_LIGHT_XBOW &&
			    p_ptr->inventory[INVEN_BOW].sval != SV_HEAVY_XBOW)
				msg_print(Ind, "\377yYou need a crossbow to fire bolts.");
			break;
		}
	}

	/* Give additional warning messages if item prevents a certain ability */
	if (o_ptr->tval == TV_SHIELD) {
		if (get_skill(p_ptr, SKILL_DODGE))
			msg_print(Ind, "\377yYou cannot dodge attacks while wielding a shield.");
		if (get_skill(p_ptr, SKILL_MARTIAL_ARTS))
			msg_print(Ind, "\377yYou cannot use special martial art styles with a shield.");
		/* cannot use ranged techniques with a shield equipped */
		if (p_ptr->ranged_flare) {
			p_ptr->ranged_flare = 0;
			msg_print(Ind, "You dispose of the flare missile.");
		}
		if (p_ptr->ranged_precision) {
			p_ptr->ranged_precision = 0;
			msg_print(Ind, "You stop aiming overly precisely.");
		}
		if (p_ptr->ranged_double) {
			p_ptr->ranged_double = 0;
			msg_print(Ind, "You stop using double shots.");
		}
		if (p_ptr->ranged_barrage) {
			p_ptr->ranged_barrage = 0;
			msg_print(Ind, "You stop preparations for barrage.");
		}
	}

	/* display warnings, possibly */
	if (ma_warning_weapon && p_ptr->warning_ma_weapon == 0) {
#ifndef ENABLE_MA_BOOMERANG
		msg_print(Ind, "\374\377RWarning: Using any sort of weapon renders Martial Arts skill effectless.");
#else
		msg_print(Ind, "\374\377RWarning: Using any melee weapon or bow renders Martial Arts skill effectless.");
#endif
//		s_printf("warning_ma_weapon: %s\n", p_ptr->name);
		warn_takeoff = TRUE;

		/* might find esp-weapon at non-low levels, so stop spamming this warning then */
		if (p_ptr->lev >= 15) p_ptr->warning_ma_weapon = 1;
	}
	if (ma_warning_shield && p_ptr->warning_ma_shield == 0) {
		msg_print(Ind, "\374\377RWarning: Using a shield will prevent Martial Arts combat styles.");
//		s_printf("warning_ma_shield: %s\n", p_ptr->name);
		warn_takeoff = TRUE;

		/* might find esp-shield at non-low levels, so stop spamming this warning then */
		if (p_ptr->lev >= 15) p_ptr->warning_ma_shield = 1;
	}
	if (warn_takeoff) msg_print(Ind, "\374\377R         Press 't' key to take off your weapons or shield.");

	if (hobbit_warning) msg_print(Ind, "\377yYou feel somewhat less dextrous than when barefeet.");

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Recalculate torch */
	p_ptr->update |= (PU_TORCH);

	/* Recalculate mana */
	p_ptr->update |= (PU_MANA | PU_HP | PU_SANITY);

	/* Redraw */
	p_ptr->redraw |= (PR_PLUSSES | PR_ARMOR);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* warning messages, mostly for newbies */
	if (p_ptr->warning_bpr3 == 0 && slot == INVEN_WIELD)
		p_ptr->warning_bpr3 = 2;
}



/*
 * Take off an item
 */
void do_cmd_takeoff(int Ind, int item, int amt) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	u32b f3, dummy;


	/* Catch items that we have already unequipped -
	   this can happen when entering items by name (@ key)
	   and it looks kinda bad to get repeated 'You are ...' messages for something already in place. */
	if (item <= INVEN_PACK) {
		Send_confirm(Ind, PKT_TAKE_OFF); //+PKT_TAKE_OFF_AMT
		return;
	}

	if (amt <= 0) {
		Send_confirm(Ind, PKT_TAKE_OFF); //+PKT_TAKE_OFF_AMT
		return;
	}

#if 0
	/* Verify potential overflow */
	if (p_ptr->inven_cnt >= INVEN_PACK) {
		/* Verify with the player */
		if (other_query_flag &&
		    !get_check(Ind, "Your pack may overflow.  Continue? ")) return;
	}
#endif


	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &(p_ptr->inventory[item]);
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max) {
			Send_confirm(Ind, PKT_TAKE_OFF); //+PKT_TAKE_OFF_AMT
			return; /* item doesn't exist */
		}
		o_ptr = &o_list[0 - item];
	}

	if (check_guard_inscription( o_ptr->note, 'T' )) {
		msg_print(Ind, "The item's inscription prevents it.");
		Send_confirm(Ind, PKT_TAKE_OFF); //+PKT_TAKE_OFF_AMT
		return;
	}

	/* Item is cursed */
	if (cursed_p(o_ptr)) {
		object_flags(o_ptr, &dummy, &dummy, &f3, &dummy, &dummy, &dummy, &dummy);
		if (!(is_admin(p_ptr) ||
		    (p_ptr->ptrait == TRAIT_CORRUPTED && !(f3 & (TR3_HEAVY_CURSE | TR3_PERMA_CURSE)))
#if 1 /* allow Bloodthirster to take off/drop heavily cursed items? */
 #ifdef ENABLE_HELLKNIGHT
		    /* note: only for corrupted priests/paladins: */
		    || ((p_ptr->pclass == CLASS_HELLKNIGHT
  #ifdef ENABLE_CPRIEST
		    || p_ptr->pclass == CLASS_CPRIEST
  #endif
		    ) && p_ptr->body_monster == RI_BLOODTHIRSTER && !(f3 & TR3_PERMA_CURSE))
 #endif
#endif
		    )) {
			/* Oops */
			msg_print(Ind, "Hmmm, it seems to be cursed.");
			/* Nope */
			Send_confirm(Ind, PKT_TAKE_OFF); //+PKT_TAKE_OFF_AMT
			return;
		}
	}

	/* Let's not end afk for this - C. Blue */
/*	un_afk_idle(Ind); */

	/* Take a partial turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

	/* Take off the item */
	inven_takeoff(Ind, item, amt, FALSE);
}


/*
 * Drop an item
 */
void do_cmd_drop(int Ind, int item, int quantity) {
	player_type *p_ptr = Players[Ind];

	object_type *o_ptr;
	u32b f1, f2, f3, f4, f5, f6, esp;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &(p_ptr->inventory[item]);
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max) return; /* item doesn't exist */
		o_ptr = &o_list[0 - item];
	}

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

#ifdef IDDC_NO_TRADE_CHEEZE /* new anti-cheeze hack: abuse NR_tradable for this */
	if (in_irondeepdive(&p_ptr->wpos) && o_ptr->NR_tradable) {
		msg_format(Ind, "\377yYou may not drop items you brought from outside this dungeon until you reach at least floor %d.", IDDC_NO_TRADE_CHEEZE);
		return;
	}
#endif

	/* Handle the newbies_cannot_drop option */	
#if (STARTEQ_TREATMENT == 1)
	if (p_ptr->max_plv < cfg.newbies_cannot_drop && !is_admin(p_ptr) &&
	    o_ptr->tval != TV_GAME && o_ptr->tval != TV_KEY && o_ptr->tval != TV_SPECIAL) {
		if (p_ptr->rogue_like_commands)
			msg_print(Ind, "\377yYou are not experienced enough to drop items. (Destroy it with '\377oCTRL+d\377y' or sell it instead.)");
		else
			msg_print(Ind, "\377yYou are not experienced enough to drop items. (Destroy it with '\377ok\377y' or sell it instead.)");
		return;
	}
#endif

	if (check_guard_inscription(o_ptr->note, 'd')) {
		msg_print(Ind, "The item's inscription prevents it.");
		return;
	};

	/* Cannot remove cursed items */
	if (cursed_p(o_ptr)
	    && !(is_admin(p_ptr) /* Hack -- DM can */
#if 1 /* allow Bloodthirster to take off/drop heavily cursed items? */
 #ifdef ENABLE_HELLKNIGHT
	    /* note: only for corrupted priests/paladins: */
	    || ((p_ptr->pclass == CLASS_HELLKNIGHT
  #ifdef ENABLE_CPRIEST
	    || p_ptr->pclass == CLASS_CPRIEST
  #endif
	    ) && p_ptr->body_monster == RI_BLOODTHIRSTER && !(f3 & TR3_PERMA_CURSE))
 #endif
#endif
	    )) {
		if ((item >= INVEN_WIELD) ) {
			/* Oops */
			msg_print(Ind, "Hmmm, it seems to be cursed.");
			/* Nope */
			return;
		} else if (f4 & TR4_CURSE_NO_DROP) {
			/* Oops */
			msg_print(Ind, "Hmmm, you seem to be unable to drop it.");
			/* Nope */
			return;
		}
	}
	if (o_ptr->questor) {
		if (p_ptr->rogue_like_commands)
			msg_print(Ind, "\377yYou can't drop this item. Use '\377oCTRL+d\377y' to destroy it. (Might abandon the quest!)");
		else
			msg_print(Ind, "\377yYou cannot drop this item. Use '\377ok\377y' to destroy it. (Might abandon the quest!)");
		return;
	}

	if (p_ptr->inval) {
		if (p_ptr->rogue_like_commands)
			msg_print(Ind, "\377yYou may not drop items, wait for an admin to validate your account. (Destroy it with '\377oCTRL+d\377y' or sell it instead.)");
		else
			msg_print(Ind, "\377yYou may not drop items, wait for an admin to validate your account. (Destroy it with '\377ok\377y' or sell it instead.)");
		return;
	}

#if 0
	/* Mega-Hack -- verify "dangerous" drops */
	if (cave[p_ptr->dun_depth][p_ptr->py][p_ptr->px].o_idx) {
		/* XXX XXX Verify with the player */
		if (other_query_flag &&
		    !get_check(Ind, "The item may disappear.  Continue? ")) return;
	}
#endif

	if (object_known_p(Ind, o_ptr)) {
#if 0 /* would prevent ppl from getting rid of unsellable artifacts */
		if (true_artifact_p(o_ptr) && !is_admin(p_ptr) &&
		    ((cfg.anti_arts_hoard && undepositable_artifact_p(o_ptr)) || (p_ptr->total_winner && !winner_artifact_p(o_ptr) && cfg.kings_etiquette))) {
			msg_print(Ind, "\377yThis item is a true artifact and cannot be dropped!");
			return;
		}
#endif
		if (p_ptr->wpos.wz == 0 && /* Assume houses are always on surface */
		    undepositable_artifact_p(o_ptr)) {
			if (inside_house(&p_ptr->wpos, p_ptr->px, p_ptr->py)) {
				if (cfg.anti_arts_house) {
					msg_print(Ind, "\377yThis item is a true artifact and cannot be dropped in a house!");
					return;
				}
			} else //if (!istown(&p_ptr->wpos))
				msg_print(Ind, "\377RWarning! If you leave this map sector, the artifact will likely disappear!");
		}
	}

	/* stop littering inns */
	if (o_ptr->level == 0 && o_ptr->owner == p_ptr->id && istown(&p_ptr->wpos) &&
	    !exceptionally_shareable_item(o_ptr) && o_ptr->tval != TV_GAME &&
	    !(o_ptr->tval == TV_PARCHMENT && (o_ptr->sval == SV_DEED_HIGHLANDER || o_ptr->sval == SV_DEED_DUNGEONKEEPER))) {
		msg_print(Ind, "\377yPlease don't litter the town with level 0 items which are unusable");
		if (p_ptr->rogue_like_commands)
			msg_print(Ind, "\377y by other players. Use '\377oCTRL+d\377y' to destroy an item instead.");
		else
			msg_print(Ind, "\377y by other players. Use '\377ok\377y' to destroy an item instead.");
		if (!is_admin(p_ptr)) return;
	}

	/* Let's not end afk for this - C. Blue */
/* 	un_afk_idle(Ind); */

#if (STARTEQ_TREATMENT > 1)
 #ifndef RPG_SERVER
	if (o_ptr->owner == p_ptr->id && p_ptr->max_plv < cfg.newbies_cannot_drop && !is_admin(p_ptr) &&
	    o_ptr->tval != TV_GAME && o_ptr->tval != TV_KEY && o_ptr->tval != TV_SPECIAL) {
		/* not for basic arrows, a bit too silyl compared to the annoyment/newbie confusion */
		if (!is_ammo(o_ptr->tval) || o_ptr->name1 || o_ptr->name2) o_ptr->level = 0;
		else o_ptr->xtra9 = 1; //mark as unsellable
	}
 #else
	if (o_ptr->owner == p_ptr->id && p_ptr->max_plv < 2 && !is_admin(p_ptr) &&
	    o_ptr->tval != TV_GAME && o_ptr->tval != TV_KEY && o_ptr->tval != TV_SPECIAL) {
		/* not for basic arrows, a bit too silyl compared to the annoyment/newbie confusion */
		if (!is_ammo(o_ptr->tval) || o_ptr->name1 || o_ptr->name2) o_ptr->level = 0;
		else o_ptr->xtra9 = 1; //mark as unsellable
	}
 #endif
#endif

	/* Take a partial turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

	/* Drop (some of) the item */
	inven_drop(Ind, item, quantity);
}


/*
 * Drop some gold
 */
void do_cmd_drop_gold(int Ind, s32b amt) {
	player_type *p_ptr = Players[Ind];

	object_type tmp_obj;

	/* Handle the newbies_cannot_drop option */
	if ((p_ptr->max_plv < cfg.newbies_cannot_drop) && !is_admin(p_ptr)) {
		msg_print(Ind, "You are not experienced enough to drop gold.");
		return;
	}

	if (p_ptr->inval) {
		msg_print(Ind, "You may not drop gold, wait for an admin to validate your account.");
		return;
	}

	/* Error checks */
	if (amt > p_ptr->au) {
		amt = p_ptr->au;
/*		msg_print(Ind, "You do not have that much gold.");
		return;
*/
	}
	if (amt <= 0) return;

	/* Setup the object */
	/* XXX Use "gold" object kind */
//	invcopy(&tmp_obj, 488);

	/* hack: player-dropped piles are bigger at same value, than normal money drops ;) */
	invcopy(&tmp_obj, gold_colour(amt, FALSE, TRUE));

	/* Setup the "worth" */
	tmp_obj.pval = amt;
	tmp_obj.xtra1 = 1; //mark as 'compact' gold pile

	/* Hack -- 'own' the gold */
	tmp_obj.owner = p_ptr->id;

	/* Non-everlasting can't take money from everlasting
	   and vice versa, depending on server cfg. */
	tmp_obj.mode = p_ptr->mode;

	/* Drop it */
	drop_near(0, &tmp_obj, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);

	/* Subtract from the player's gold */
	p_ptr->au -= amt;

	/* Let's not end afk for this - C. Blue */
/* 	un_afk_idle(Ind); */

	/* Message */
//	msg_format(Ind, "You drop %d pieces of gold.", amt);
	msg_format(Ind, "You drop %d pieces of %s.", amt, k_name + k_info[tmp_obj.k_idx].name);

#ifdef USE_SOUND_2010
	sound(Ind, "drop_gold", NULL, SFX_TYPE_COMMAND, FALSE);
#endif

/* #if DEBUG_LEVEL > 3 */
//	if (amt >= 10000) {
		p_ptr->last_gold_drop += amt;
		if (turn - p_ptr->last_gold_drop_timer >= cfg.fps * 2) {
			s_printf("Gold dropped (%d by %s at %d,%d,%d).\n", p_ptr->last_gold_drop, p_ptr->name, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			p_ptr->last_gold_drop = 0;
			p_ptr->last_gold_drop_timer = turn;
		}
//	}

	break_cloaking(Ind, 5);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);

	/* Redraw gold */
	p_ptr->redraw |= (PR_GOLD);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);
}


/*
 * Destroy an item
 */
bool do_cmd_destroy(int Ind, int item, int quantity) {
	player_type *p_ptr = Players[Ind];
	int			old_number;
	//bool		force = FALSE;
	object_type		*o_ptr;
	char		o_name[ONAME_LEN];
	u32b f1, f2, f3, f4, f5, f6, esp;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &(p_ptr->inventory[item]);
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max) return FALSE; /* item doesn't exist */
		o_ptr = &o_list[0 - item];
	}

	/* Describe the object */
	old_number = o_ptr->number;
	o_ptr->number = quantity;
	object_desc(Ind, o_name, o_ptr, TRUE, 3);
	o_ptr->number = old_number;

	if (check_guard_inscription( o_ptr->note, 'k')) {
		msg_print(Ind, "The item's inscription prevents it.");
		return FALSE;
	};
#if 0
	/* Verify if needed */
	if (!force || other_query_flag) {
		/* Make a verification */
		snprintf(out_val, sizeof(out_val), "Really destroy %s? ", o_name);
		if (!get_check(Ind, out_val)) return FALSE;
	}
#endif

	/* Let's not end afk for this - C. Blue */
/* 	un_afk_idle(Ind); */

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	if (o_ptr->questor && o_ptr->questor_invincible) {
		msg_print(Ind, "Hmmm, you seem to be unable to destroy it.");
		return FALSE;
	}
	if (((f4 & TR4_CURSE_NO_DROP) && cursed_p(o_ptr))
	    && !(is_admin(p_ptr)
#if 1 /* allow Bloodthirster to take off/drop heavily cursed items? */
 #ifdef ENABLE_HELLKNIGHT
	    /* note: only for corrupted priests/paladins: */
	    || ((p_ptr->pclass == CLASS_HELLKNIGHT
  #ifdef ENABLE_CPRIEST
	    || p_ptr->pclass == CLASS_CPRIEST
  #endif
	    ) && p_ptr->body_monster == RI_BLOODTHIRSTER && !(f3 & TR3_PERMA_CURSE))
 #endif
#endif
	    )) {
		/* Oops */
		msg_print(Ind, "Hmmm, you seem to be unable to destroy it.");

		/* Nope */
		return FALSE;
	}

#ifndef FUN_SERVER /* while server is being hacked, allow this (while /wish is also allowed) - C. Blue */
	/* Artifacts cannot be destroyed */
	if (like_artifact_p(o_ptr) && !is_admin(p_ptr)) {
		cptr feel = "special";

		/* Message */
		msg_format(Ind, "You cannot destroy %s.", o_name);

		/* Hack -- Handle icky artifacts */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) feel = "terrible";
		/* Hack -- inscribe the artifact */
		o_ptr->note = quark_add(feel);
		/* We have "felt" it (again) */
		o_ptr->ident |= (ID_SENSE | ID_SENSED_ONCE | ID_SENSE_HEAVY);

		/* Combine the pack */
		p_ptr->notice |= (PN_COMBINE);
		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);
		/* Done */
		return FALSE;
	}
#endif
	/* Keys cannot be destroyed */
	if (o_ptr->tval == TV_KEY && !is_admin(p_ptr)) {
		/* Message */
		msg_format(Ind, "You cannot destroy %s.", o_name);
		/* Done */
		return FALSE;
	}

	/* Cursed, equipped items cannot be destroyed */
	if (item >= INVEN_WIELD && cursed_p(o_ptr) && !is_admin(p_ptr)) {
		/* Message */
		msg_print(Ind, "Hmm, that seems to be cursed.");
		/* Done */
		return FALSE;
	}

#if POLY_RING_METHOD == 0
	/* Polymorph back */
	/* XXX this can cause strange things for players with mimicry skill.. */
	if ((item == INVEN_LEFT || item == INVEN_RIGHT) && (o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_POLYMORPH)) {
		if ((p_ptr->body_monster == o_ptr->pval) &&
		   ((p_ptr->r_mimicry[p_ptr->body_monster] < r_info[p_ptr->body_monster].level) ||
		   (get_skill_scale(p_ptr, SKILL_MIMIC, 100) < r_info[p_ptr->body_monster].level)))
		{
			/* If player hasn't got high enough kill count anymore now, poly back to player form! */
 #if 1
			msg_print(Ind, "You polymorph back to your normal form.");
			do_mimic_change(Ind, 0, TRUE);
 #endif
			s_printf("DESTROY_EXPLOIT (poly): %s destroyed %s\n", p_ptr->name, o_name);
		}
	}
#endif

	/* Check if item gave WRAITH form */
	if((k_info[o_ptr->k_idx].flags3 & TR3_WRAITH) && p_ptr->tim_wraith) {
		s_printf("DESTROY_EXPLOIT (wraith): %s destroyed %s\n", p_ptr->name, o_name);
#if 1
		p_ptr->tim_wraith = 1;
#endif
	}

	/* Message */
	msg_format(Ind, "You destroy %s.", o_name);

#ifdef USE_SOUND_2010
//	sound_item(Ind, o_ptr->tval, o_ptr->sval, "kill_");
#endif

	/* log big oopsies */
	if ((esp = object_value_real(0, o_ptr)) >= 100000)
		s_printf("DESTROYED_VALUABLE: %s: %s (%d)\n", p_ptr->name, o_name, esp);

	if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1);
	questitem_d(o_ptr, quantity);

	if (is_magic_device(o_ptr->tval)) divide_charged_item(NULL, o_ptr, quantity);

	/* Eliminate the item (from the pack) */
	if (item >= 0) {
		inven_item_increase(Ind, item, -quantity);
		inven_item_describe(Ind, item);
		inven_item_optimize(Ind, item);
	}
	/* Eliminate the item (from the floor) */
	else {
		floor_item_increase(0 - item, -quantity);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

	break_cloaking(Ind, 5);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);

	return TRUE;
}


/*
 * Observe an item which has been *identify*-ed
 */
void do_cmd_observe(int Ind, int item) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &(p_ptr->inventory[item]);
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max) return; /* item doesn't exist */
		o_ptr = &o_list[0 - item];
	}

	/* Require full knowledge */
	if (!(o_ptr->ident & ID_MENTAL) && !is_admin(p_ptr)) observe_aux(Ind, o_ptr, item);
	/* Describe it fully */
	else if (!identify_fully_aux(Ind, o_ptr, FALSE, item)) msg_print(Ind, "You see nothing special.");
}



/*
 * Remove the inscription from an object
 * XXX Mention item (when done)?
 */
void do_cmd_uninscribe(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;


	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &(p_ptr->inventory[item]);
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max)
			return; /* item doesn't exist */

		o_ptr = &o_list[0 - item];
	}

	/* Nothing to remove */
	if (!o_ptr->note) {
		msg_print(Ind, "That item had no inscription to remove.");
		return;
	}

	/* small hack, make shirt logos permanent */
	if (o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_SHIRT && !is_admin(p_ptr)) {
		msg_print(Ind, "Cannot uninscribe shirts.");
		return;
	}
	if ((o_ptr->tval == TV_SPECIAL ||
	    (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_CHEQUE)
	    ) && !is_admin(p_ptr)) {
		msg_print(Ind, "Cannot uninscribe this item.");
		return;
	}

	/* Message */
	msg_print(Ind, "Inscription removed.");

	/* Remove the incription */
	o_ptr->note = 0;
	o_ptr->note_utag = 0;

	/* Combine the pack */
	p_ptr->notice |= (PN_COMBINE);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);
}


/*
 * Inscribe an object with a comment
 */
void do_cmd_inscribe(int Ind, int item, cptr inscription) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	char o_name[ONAME_LEN], modins[MAX_CHARS];
	const char *qins;
	char *c;


	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &(p_ptr->inventory[item]);
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max)
			return; /* item doesn't exist */

		o_ptr = &o_list[0 - item];
	}

	/* small hack, make shirt logos permanent */
	if (o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_SHIRT && !is_admin(p_ptr)) {
		msg_print(Ind, "Cannot inscribe shirts.");
		return;
	}
	if ((o_ptr->tval == TV_SPECIAL ||
	    (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_CHEQUE)
	    ) && !is_admin(p_ptr)) {
		msg_print(Ind, "Cannot inscribe this item.");
		return;
	}

	/* Describe the activity */
	object_desc(Ind, o_name, o_ptr, TRUE, 3);

	/* Special hack: Inscribing '@@' applies an automatic item-powers inscription */
	if (!strcmp(inscription, "@@")) {
		char powins[1024]; //powins[MAX_CHARS_WIDE];  --let's play it safe..
		u32b f1, f2, f3, f4, f5, f6, esp, tmpf1, tmpf2, tmpf3, tmp;
		bool i_f = FALSE, i_c = FALSE, i_e = FALSE, i_a = FALSE, i_p = FALSE, i_w = FALSE, i_n = FALSE;

		if (!(o_ptr->ident & ID_MENTAL)) {
			msg_print(Ind, "\377yYou must have *identified* an item in order to power-inscribe it.");
			return;
		}

		msg_format(Ind, "Power-inscribing %s.", o_name);
		msg_print(Ind, NULL);

		powins[0] = 0;
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		/* specialty: recognize custom spell books and inscribe with their spell names! */
		if (o_ptr->tval == TV_BOOK && is_custom_tome(o_ptr->sval)) {
			if (o_ptr->xtra1) strcat(powins, format("%s,", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra1 - 1))));
			if (o_ptr->xtra2) strcat(powins, format("%s,", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra2 - 1))));
			if (o_ptr->xtra3) strcat(powins, format("%s,", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra3 - 1))));
			if (o_ptr->xtra4) strcat(powins, format("%s,", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra4 - 1))));
			if (o_ptr->xtra5) strcat(powins, format("%s,", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra5 - 1))));
			if (o_ptr->xtra6) strcat(powins, format("%s,", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra6 - 1))));
			if (o_ptr->xtra7) strcat(powins, format("%s,", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra7 - 1))));
			if (o_ptr->xtra8) strcat(powins, format("%s,", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra8 - 1))));
			if (o_ptr->xtra9) strcat(powins, format("%s,", string_exec_lua(Ind, format("return(__tmp_spells[%d].name)", o_ptr->xtra9 - 1))));
		}

		tmpf1 = f1 & (TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR);
		tmpf2 = f2 & (TR2_SUST_STR | TR2_SUST_INT | TR2_SUST_WIS | TR2_SUST_DEX | TR2_SUST_CON | TR2_SUST_CHR);
		tmpf3 = tmpf1 & tmpf2;
		if (tmpf3) {
			strcat(powins, "^");
			if (tmpf3 & TR1_STR) strcat(powins, "S");
			if (tmpf3 & TR1_INT) strcat(powins, "I");
			if (tmpf3 & TR1_WIS) strcat(powins, "W");
			if (tmpf3 & TR1_DEX) strcat(powins, "D");
			if (tmpf3 & TR1_CON) strcat(powins, "C");
			if (tmpf3 & TR1_CHR) strcat(powins, "H");
			tmpf1 &= ~tmpf3;
			tmpf2 &= ~tmpf3;
		}
		if (tmpf1) {
			strcat(powins, "+");
			if (tmpf1 & TR1_STR) strcat(powins, "S");
			if (tmpf1 & TR1_INT) strcat(powins, "I");
			if (tmpf1 & TR1_WIS) strcat(powins, "W");
			if (tmpf1 & TR1_DEX) strcat(powins, "D");
			if (tmpf1 & TR1_CON) strcat(powins, "C");
			if (tmpf1 & TR1_CHR) strcat(powins, "H");
		}
		if (tmpf2) {
			strcat(powins, "_");
			if (tmpf2 & TR2_SUST_STR) strcat(powins, "S");
			if (tmpf2 & TR2_SUST_INT) strcat(powins, "I");
			if (tmpf2 & TR2_SUST_WIS) strcat(powins, "W");
			if (tmpf2 & TR2_SUST_DEX) strcat(powins, "D");
			if (tmpf2 & TR2_SUST_CON) strcat(powins, "C");
			if (tmpf2 & TR2_SUST_CHR) strcat(powins, "H");
		}
		if (tmpf1 || tmpf2 || tmpf3) strcat(powins, ",");

		if (f3 & (TR3_XTRA_MIGHT)) strcat(powins, "Xm");
		if (f3 & (TR3_XTRA_SHOTS)) strcat(powins, "Xs");
		if (f1 & (TR1_LIFE)) strcat(powins, "HP");
		if (f1 & (TR1_MANA)) strcat(powins, "MP");
		//if (f1 & (TR1_SPELL)) strcat(powins, "Sp");
		if (f1 & (TR1_SPEED)) strcat(powins, "Spd");
		if (f1 & (TR1_BLOWS)) strcat(powins, "Att");
		if (f5 & (TR5_CRIT)) strcat(powins, "Crt");
		if (f1 & (TR1_STEALTH)) {
			if (o_ptr->tval != TV_TRAPKIT) strcat(powins, "Stl");
		}
#if 0
#ifdef ART_WITAN_STEALTH
		else if (o_ptr->name1 && o_ptr->tval == TV_BOOTS && o_ptr->sval == SV_PAIR_OF_WITAN_BOOTS)
			 strcat(powins, "Stl");
#endif
#endif
		if (f1 & (TR1_SEARCH)) strcat(powins, "Src");
		if (f1 & (TR1_INFRA)) strcat(powins, "IV");
		if (f1 & (TR1_TUNNEL)) strcat(powins, "Dig");
		if (f5 & (TR5_DISARM)) strcat(powins, "Dsr");
		if (f5 & (TR5_LUCK)) strcat(powins, "Lu");
		if (f5 & (TR5_CHAOTIC)) strcat(powins, "Cht");
		if (f1 & (TR1_VAMPIRIC)) strcat(powins, "Va");
		if (f5 & (TR5_VORPAL)) strcat(powins, "Vo");
		/*if (f5 & (TR5_WOUNDING))*/
		if (f5 & (TR5_IMPACT)) strcat(powins, "Eq");
		if (o_ptr->tval != TV_TRAPKIT) {
			if ((f2 & (TR2_IM_FIRE)) || (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_MULTIHUED && o_ptr->xtra2 & 0x01)) i_f = TRUE;
			if ((f2 & (TR2_IM_COLD)) || (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_MULTIHUED && o_ptr->xtra2 & 0x02)) i_c = TRUE;
			if ((f2 & (TR2_IM_ELEC)) || (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_MULTIHUED && o_ptr->xtra2 & 0x04)) i_e = TRUE;
			if ((f2 & (TR2_IM_ACID)) || (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_MULTIHUED && o_ptr->xtra2 & 0x08)) i_a = TRUE;
			if ((f5 & (TR5_IM_POISON)) || (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_MULTIHUED && o_ptr->xtra2 & 0x10)) i_p = TRUE;
			if (f5 & (TR5_IM_WATER)) i_w = TRUE;
			if (f4 & (TR4_IM_NETHER)) i_n = TRUE;
			if ((tmp = (i_f || i_c || i_e || i_a || i_p || i_w || i_n))) {
				if (powins[0]) strcat(powins, ",");
				strcat(powins, "*");
				if (i_f) strcat(powins, "F");
				if (i_c) strcat(powins, "C");
				if (i_e) strcat(powins, "E");
				if (i_a) strcat(powins, "A");
				if (i_p) strcat(powins, "P");
				if (i_w) strcat(powins, "W");
				if (i_n) strcat(powins, "N");
				strcat(powins, "*");
			}
		} else tmp = FALSE;
		//if (o_ptr->tval != TV_DRAG_ARMOR || o_ptr->sval != SV_DRAGON_MULTIHUED)
		{
			if (!(i_f && i_c && i_e && i_a)) {
				if ((f2 & TR2_RES_FIRE) && (f2 & TR2_RES_COLD) && (f2 & TR2_RES_ELEC) && (f2 & TR2_RES_ACID)) {
					if (!tmp && powins[0] && powins[strlen(powins) - 1] != ',') strcat(powins, ",");
					strcat(powins, "Base");
				} else {
					i_f = (f2 & TR2_RES_FIRE) && !(f2 & TR2_IM_FIRE);
					i_c = (f2 & TR2_RES_COLD) && !(f2 & TR2_IM_COLD);
					i_e = (f2 & TR2_RES_ELEC) && !(f2 & TR2_IM_ELEC);
					i_a = (f2 & TR2_RES_ACID) && !(f2 & TR2_IM_ACID);
					if (!tmp && powins[0] && (i_f | i_c | i_e | i_a)) {
						if (powins[strlen(powins) - 1] != ',') strcat(powins, ",");
						if (i_f) strcat(powins, "f");
						if (i_c) strcat(powins, "c");
						if (i_e) strcat(powins, "e");
						if (i_a) strcat(powins, "a");
					}
				}
			}
		}
		if (!i_p && (f2 & TR2_RES_POIS)) strcat(powins, "Po");
		if (!i_w && (f5 & TR5_RES_WATER)) strcat(powins, "Wa");
		if (!i_n && (f2 & TR2_RES_NETHER)) strcat(powins, "Nt");
		if (f2 & (TR2_RES_NEXUS)) strcat(powins, "Nx");
		if (f2 & (TR2_RES_CHAOS)) strcat(powins, "Ca");
		if (f2 & (TR2_RES_DISEN)) strcat(powins, "Di");
		if (f2 & (TR2_RES_SOUND)) strcat(powins, "So");
		if (f2 & (TR2_RES_SHARDS)) strcat(powins, "Sh");
		if (f2 & (TR2_RES_LITE)) strcat(powins, "Lt");
		if (f2 & (TR2_RES_DARK)) strcat(powins, "Dk");
		if (f5 & (TR5_RES_TIME)) strcat(powins, "Ti");
		if (f5 & (TR5_RES_MANA)) strcat(powins, "Ma");
		if (f2 & (TR2_RES_BLIND)) strcat(powins, "Bl");
		if (f2 & (TR2_RES_CONF)) strcat(powins, "Cf");

		if (f5 & TR5_RES_TELE) strcat(powins, "RT");
		if (f2 & (TR2_FREE_ACT)) strcat(powins, "FA");
		if (f2 & (TR2_HOLD_LIFE)) strcat(powins, "HL");
		if (f2 & (TR2_RES_FEAR)) strcat(powins, "Fe");
		if (f3 & (TR3_SEE_INVIS)) strcat(powins, "SI");
		if (f4 & (TR4_LEVITATE)) strcat(powins, "Lv");
		else if (f3 & (TR3_FEATHER)) strcat(powins, "FF");
		if (f3 & (TR3_SLOW_DIGEST)) strcat(powins, "SD");
		if (f3 & (TR3_REGEN)) strcat(powins, "Rg");
		if (f5 & (TR5_REGEN_MANA)) strcat(powins, "Rgm");

		if (f1 & (TR1_BRAND_ACID)) strcat(powins, "BA");
		if (f1 & (TR1_BRAND_ELEC)) strcat(powins, "BE");
		if (f1 & (TR1_BRAND_FIRE)) strcat(powins, "BF");
		if (f1 & (TR1_BRAND_COLD)) strcat(powins, "BC");
		if (f1 & (TR1_BRAND_POIS)) strcat(powins, "BP");
		if (f3 & (TR3_SH_FIRE)) strcat(powins, "SF");
		if (f5 & (TR5_SH_COLD)) strcat(powins, "SC");
		if (f3 & (TR3_SH_ELEC)) strcat(powins, "SE");

		if ((o_ptr->tval != TV_LITE) && ((f3 & (TR3_LITE1)) || (f4 & (TR4_LITE2)) || (f4 & (TR4_LITE3))))  strcat(powins, "+Lt");
		if (f5 & (TR5_INVIS)) strcat(powins, "Inv");
		if (f5 & (TR5_REFLECT)) strcat(powins, "Refl");
		if (f3 & (TR3_BLESSED)) strcat(powins, "Bless");
		if (f3 & (TR3_NO_MAGIC)) strcat(powins, "AM");
		if (f4 & (TR4_AUTO_ID)) strcat(powins, "ID");

		if (f5 & (TR5_PASS_WATER)) strcat(powins, "Swim");
		if (f4 & (TR4_CLIMB)) strcat(powins, "Climb");
		if (f3 & (TR3_WRAITH)) strcat(powins, "Wraith");

		if ((tmp = (f1 & (TR1_SLAY_ORC | TR1_SLAY_TROLL | TR1_SLAY_ORC | TR1_SLAY_TROLL | TR1_SLAY_GIANT | TR1_SLAY_ANIMAL | TR1_SLAY_UNDEAD | TR1_SLAY_DEMON | TR1_SLAY_DRAGON | TR1_SLAY_EVIL | TR1_KILL_UNDEAD | TR1_KILL_DEMON | TR1_KILL_DRAGON)))) {
			if (powins[0] && powins[strlen(powins) - 1] != ',') strcat(powins, ",");
			strcat(powins, "+");
			if (f1 & (TR1_SLAY_ORC)) strcat(powins, "o");
			if (f1 & (TR1_SLAY_TROLL)) strcat(powins, "T");
			if (f1 & (TR1_SLAY_GIANT)) strcat(powins, "P");
			if (f1 & (TR1_SLAY_ANIMAL)) strcat(powins, "a");
			if (!(f1 & (TR1_KILL_UNDEAD)) && (f1 & TR1_SLAY_UNDEAD)) strcat(powins, "W");
			if (!(f1 & (TR1_KILL_UNDEAD)) && (f1 & TR1_SLAY_UNDEAD)) strcat(powins, "U");
			if (!(f1 & (TR1_KILL_UNDEAD)) && (f1 & TR1_SLAY_UNDEAD)) strcat(powins, "D");
			if (f1 & (TR1_SLAY_EVIL)) strcat(powins, "Evil");
			if (f1 & (TR1_KILL_UNDEAD | TR1_KILL_DEMON | TR1_KILL_DRAGON)) {
				strcat(powins, "*");
				if (f1 & TR1_KILL_UNDEAD) strcat(powins, "W");
				if (f1 & TR1_KILL_DEMON) strcat(powins, "U");
				if (f1 & TR1_KILL_DRAGON) strcat(powins, "D");
			}
		}
		if (esp) {
			if (esp & ESP_ALL) {
				if (powins[0] && powins[strlen(powins) - 1] != ',') strcat(powins, ",");
				strcat(powins, "ESP");
			} else {
				if (powins[0] && powins[strlen(powins) - 1] != ',') strcat(powins, ",");
				strcat(powins, "~");
				if (esp & ESP_SPIDER) strcat(powins, "S");
				if (esp & ESP_ORC) strcat(powins, "o");
				if (esp & ESP_TROLL) strcat(powins, "T");
				if (esp & ESP_GIANT) strcat(powins, "P");
				if (esp & ESP_ANIMAL) strcat(powins, "a");
				if (esp & ESP_UNDEAD) strcat(powins, "W");
				if (esp & ESP_DEMON) strcat(powins, "U");
				if (esp & ESP_DRAGON) strcat(powins, "D");
				if (esp & ESP_DRAGONRIDER) strcat(powins, "DR");
				if (esp & ESP_GOOD) strcat(powins, "A");
				if (esp & ESP_NONLIVING) strcat(powins, "g");
				if (esp & ESP_UNIQUE) strcat(powins, "*");
				if (esp & ESP_EVIL) strcat(powins, "Evil");
			}
		}
		if (tmp || esp) strcat(powins, ",");

		if (f3 & (TR3_TELEPORT)) strcat(powins, "Tele");
		if (f3 & (TR3_NO_TELE)) strcat(powins, "NT");
		if (f5 & (TR5_DRAIN_HP)) strcat(powins, "Dr");
		if (f5 & (TR5_DRAIN_MANA)) strcat(powins, "Drm");
		if (f3 & (TR3_DRAIN_EXP)) strcat(powins, "Drx");
		if (f3 & (TR3_AGGRAVATE)) strcat(powins, "Aggr");

		/* covers both, tmp+esp and strange books.. */
		if (powins[strlen(powins) - 1] == ',') powins[strlen(powins) - 1] = 0;

		/* exploding ammo */
		if (is_ammo(o_ptr->tval) && (o_ptr->pval != 0)) {
			if (powins[0]) strcat(powins, " ");
			strcat(powins, "(");
			switch (o_ptr->pval) {
			case GF_ELEC: strcat(powins, "Lightning"); break;
			case GF_POIS: strcat(powins, "Poison"); break;
			case GF_ACID: strcat(powins, "Acid"); break;
			case GF_COLD: strcat(powins, "Frost"); break;
			case GF_FIRE: strcat(powins, "Fire"); break;
			case GF_PLASMA: strcat(powins, "Plasma"); break;
			case GF_LITE: strcat(powins, "Light"); break;
			case GF_DARK: strcat(powins, "Darkness"); break;
			case GF_SHARDS: strcat(powins, "Shards"); break;
			case GF_SOUND: strcat(powins, "Sound"); break;
			case GF_CONFUSION: strcat(powins, "Confusion"); break;
			case GF_FORCE: strcat(powins, "Force"); break;
			case GF_INERTIA: strcat(powins, "Inertia"); break;
			case GF_MANA: strcat(powins, "Mana"); break;
			case GF_METEOR: strcat(powins, "Mini-Meteors"); break;
			case GF_ICE: strcat(powins, "Ice"); break;
			case GF_CHAOS: strcat(powins, "Chaos"); break;
			case GF_NETHER: strcat(powins, "Nether"); break;
			case GF_NEXUS: strcat(powins, "Nexus"); break;
			case GF_TIME: strcat(powins, "Time"); break;
			case GF_GRAVITY: strcat(powins, "Gravity"); break;
			case GF_KILL_WALL: strcat(powins, "Stone-to-mud"); break;
			case GF_AWAY_ALL: strcat(powins, "Teleportation"); break;
			case GF_TURN_ALL: strcat(powins, "Fear"); break;
			case GF_NUKE: strcat(powins, "Toxic waste"); break;
			case GF_STUN: strcat(powins, "Stun"); break; //disabled
			case GF_DISINTEGRATE: strcat(powins, "Disintegration"); break;
			case GF_HELL_FIRE: strcat(powins, "Hellfire"); break;
			}
			strcat(powins, ")");
		}

		/* Watch total object name length */
		o_ptr->note = o_ptr->note_utag = 0;
		object_desc(Ind, o_name, o_ptr, TRUE, 3);
		if (ONAME_LEN - ((int)strlen(o_name)) - 1 >= 0) { /* paranoia */
			if (strlen(o_name) + strlen(powins) >= ONAME_LEN) powins[ONAME_LEN - strlen(o_name) - 1] = 0;

			/* Save the inscription */
			o_ptr->note = quark_add(powins);
			o_ptr->note_utag = 0;
		}

		/* Combine the pack */
		p_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);
		return;
	}

	/* Message */
	msg_format(Ind, "Inscribing %s.", o_name);
	msg_print(Ind, NULL);

	/* small hack to prevent using colour codes in inscriptions:
	   convert \{ to just {, compare nserver.c:Send_special_line()!
	   Note: Colour codes in inscriptions/item names aren't implemented anyway. */
	if (!is_admin(p_ptr)) while ((c = strstr(inscription, "\\{")))
		c[0] = '{';

	/* hack to fix auto-inscriptions: convert empty inscription to a #-type inscription */
	if (inscription[0] == '\0') inscription = "#";

	/* Comfort hack for reinscribing items:
	   Trigger with '\' as first character.
	   Then replace the part starting on first letter, until an usual delimiter char is found.
	   '\@@' or '\!!' will erase the corresponding tag from the inscription. */
	/* catch empty item inscription */
	if (!o_ptr->note && inscription[0] == '\\' && inscription[1] != '\\') {
		/* cannot delete anything in an empty inscription */
		if ((inscription[1] == '@' || inscription[1] == '!' || inscription [1] == '#')
		    && inscription[2] == inscription[1])
			return;
		/* clear '\' special feature trigger */
		while (inscription[0] == '\\') inscription++;
	}
	if (inscription[0] == '\\') {
		/* force append? */
		if (inscription[1] == '\\') {
			if (o_ptr->note) strcpy(modins, quark_str(o_ptr->note));
			else modins[0] = 0;
			if (strlen(modins) + strlen(inscription) - 1 > MAX_CHARS) {
				msg_print(Ind, "Inscription would become too long.");
				return;
			}
			strcat(modins, inscription + 2);
			inscription = modins;
		} else {
			bool append = TRUE;
			char modsrc[3];

			qins = quark_str(o_ptr->note);
			strcpy(modins, qins);

			modsrc[0] = inscription[1];
			/* search for specific @-tag to replace? */
			if (inscription[1] == '@') {
				/* duplicate tag, aka "delete!" ? */
				if (inscription[2] == '@') modsrc[1] = inscription[3];
				/* normal tag (replace or append) */
				else modsrc[1] = inscription[2];
				modsrc[2] = 0;
			}
			/* search for first !-tag to replace? */
			else modsrc[1] = 0;

			/* append or replace a @/!/# part? */
			if (inscription[1] == '@' || inscription[1] == '!' || inscription [1] == '#') {
				char *start = strstr(modins, modsrc);
				bool delete = FALSE;

				/* delete? */
				if (inscription[2] == inscription[1]) {
					delete = TRUE;
					/* definitely don't append: in case tag does not exist yet */
					append = FALSE;
				}

				/* replace? (or delete) */
				if (start) {
					const char *delimiter;
					const char *deltmp;

					append = FALSE;

					/* after '#' the line is always completely replaced;
					   same for @P because player names can contain spaces. */
					if (inscription[1] != '#' &&
					    !(inscription[1] == '@' && inscription[2] == 'P')) {
						deltmp = qins + (start - modins) + strlen(modsrc);
						while (*deltmp) {
							delimiter = strchr(" @!#", *deltmp);
							if (delimiter) break;
							deltmp++;
						}
						//if (!delimiter) delimiter = qins + strlen(qins); //point to zero terminator char
					} else deltmp = qins + strlen(qins);
					//delimiter = qins + strlen(qins); //point to zero terminator char

					if (delete) strcpy(start, deltmp);
					else { /* try to replace */
						//if ((start - modins) + strlen(inscription + 1) + strlen(delimiter) > MAX_CHARS) {
						if ((start - modins) + strlen(inscription + 1) + strlen(deltmp) > MAX_CHARS) {
							msg_print(Ind, "Inscription would become too long.");
							return;
						}

						/* replace */
						strcpy(start, inscription + 1);
						//strcat(start, delimiter);
						strcat(start, deltmp);
					}
				}

				/* new: trim trailing spaces, if anything was deleted */
				if (delete) while (modins[strlen(modins) - 1] == ' ') modins[strlen(modins) - 1] = 0;
			}
			/* append? */
			if (append) {
				if (strlen(modins) + strlen(inscription) > MAX_CHARS) {
					msg_print(Ind, "Inscription would become too long.");
					return;
				}
				strcat(modins, inscription + 1);
			}
			inscription = modins;
		}
	}

	/* Save the inscription */
	o_ptr->note = quark_add(inscription);
	o_ptr->note_utag = 0;

	/* Combine the pack */
	p_ptr->notice |= (PN_COMBINE);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);
}


/*
 * Steal an object from a monster
 */
/*
 * This is quite abusable.. you can rob Wormie, leave the floor, and
 * you'll meet him again and again... so I stop implementing this.
 * - Jir -
 */
void do_cmd_steal_from_monster(int Ind, int dir) {
#if 0
	player_type *p_ptr = Players[Ind], *q_ptr;
	cave_type **zcave;
	int x, y, dir = 0, item = -1, k = -1;
	cave_type *c_ptr;
	monster_type *m_ptr;
	object_type *o_ptr, forge;
	byte num = 0;
	bool done = FALSE;
	int monst_list[23];

	if(!(zcave = getcave(&p_ptr->wpos))) return;

	/* Ghosts cannot steal ; not in WRAITHFORM */
	if (p_ptr->ghost || p_ptr->tim_wraith || (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST)) {
		msg_print(Ind, "You cannot steal things in your immaterial form!");
		return;
	}

	/* Only works on adjacent monsters */
	if (!get_rep_dir(&dir)) return;
	y = py + ddy[dir];
	x = px + ddx[dir];
	c_ptr = &cave[y][x];

	if (!(c_ptr->m_idx)) {
		msg_print("There is no monster there!");
		return;
	}

	m_ptr = &m_list[c_ptr->m_idx];

	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);

	/* There were no non-gold items */
	if (!m_ptr->hold_o_idx) {
		msg_print("That monster has no objects!");
		return;
	}

 #if 0
	/* not in WRAITHFORM */
	if (p_ptr->tim_wraith) {
		msg_print("You can't grab anything!");
		return;
	}

	/* The monster is immune */
	if (r_info[m_ptr->r_idx].flags7 & (RF7_NO_THEFT)) {
		msg_print("The monster is guarding the treasures.");
		return;
	}

	screen_save();

	num = show_monster_inven(c_ptr->m_idx, monst_list);

	/* Repeat until done */
	while (!done) {
		char tmp_val[80];
		char which = ' ';

		/* Build the prompt */
		strnfmt(tmp_val, MAX_CHARS, "Choose an item to steal (a-%c) or ESC:",
				'a' - 1 + num);

		/* Show the prompt */
		prt(tmp_val, 0, 0);

		/* Get a key */
		which = inkey();

		/* Parse it */
		switch (which) {
		case ESCAPE:
			done = TRUE;
			break;

		default:
			int ver;

			/* Extract "query" setting */
			ver = isupper(which);
			which = tolower(which);

			k = islower(which) ? A2I(which) : -1;
			if (k < 0 || k >= num) {
				bell();
				break;
			}

			/* Verify the item */
			if (ver && !verify("Try", 0 - monst_list[k])) {
				done = TRUE;
				break;
			}

			/* Accept that choice */
			item = monst_list[k];
			done = TRUE;

			break;
		}
	}
 #endif	// 0

	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	if (item != -1) {
		int chance;

		chance = 40 - p_ptr->stat_ind[A_DEX];
		chance +=
			o_list[item].weight / (get_skill_scale(SKILL_STEALING, 19) + 1);
		chance += get_skill_scale(SKILL_STEALING, 29) + 1;
		chance -= (m_ptr->csleep) ? 10 : 0;
		chance += m_ptr->level;

		/* Failure check */
		if (rand_int(chance) > 1 + get_skill_scale(SKILL_STEALING, 25)) {
			/* Take a turn */
			energy_use = 100;

			/* Wake up */
			m_ptr->csleep = 0;

			/* Speed up because monsters are ANGRY when you try to thief them */
			if (m_ptr->mspeed < m_ptr->speed + 15)
				m_ptr->mspeed += 5; m_ptr->speed += 5;
			screen_load();
			break_cloaking(Ind, 0);
			msg_print("Oops ! The monster is now really *ANGRY*.");
			return;
		}

		/* Reconnect the objects list */
		if (num == 1) m_ptr->hold_o_idx = 0;
		else {
			if (k > 0) o_list[monst_list[k - 1]].next_o_idx = monst_list[k + 1];
			if (k + 1 >= num) o_list[monst_list[k - 1]].next_o_idx = 0;
			if (k == 0) m_ptr->hold_o_idx = monst_list[k + 1];
		}

		/* Rogues gain some xp */
		if (PRACE_FLAGS(PR1_EASE_STEAL)) {
			s32b max_point;

			/* Max XP gained from stealing */
			max_point = (o_list[item].weight / 2) + (m_ptr->level * 10);

			/* Randomise it a bit, with half a max guaranteed */
			if (!(p_ptr->mode & MODE_PVP)) gain_exp((max_point / 2) + (randint(max_point) / 2));

			/* Allow escape */
			if (get_check("Phase door?")) teleport_player(Ind, 10, TRUE);
		}

		/* Get the item */
		o_ptr = &forge;

		/* Special handling for gold */
		if (o_list[item].tval == TV_GOLD) {
			gain_au(Ind, o_ptr->pval, FALSE, FALSE);
			p_ptr->window |= (PW_PLAYER);
		} else {
			object_copy(o_ptr, &o_list[item]);
			inven_carry(o_ptr, FALSE);
		}

		/* Delete it */
		invwipe(&o_list[item]);
	}

	screen_load();

	/* Take a turn */
	energy_use = 100;
#endif	// 0
}


/*
 * Attempt to steal from another player
 */
/* TODO: Make it possible to steal from monsters.. */
void do_cmd_steal(int Ind, int dir) {
	player_type *p_ptr = Players[Ind], *q_ptr;
	cave_type *c_ptr;

	int success, notice;
	bool caught = FALSE;
	cave_type **zcave;
	u16b dal;
	bool etiquette;

	if (!(zcave = getcave(&p_ptr->wpos))) return;

	/* May not steal from yourself */
	if (!dir || dir == 5) return;

	/* Ghosts cannot steal */
	/* not in WRAITHFORM either */
	if (p_ptr->ghost || p_ptr->tim_wraith || (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST)) {
		msg_print(Ind, "You cannot steal things in your immaterial form!");
		return;
	}

	/* Make sure we have enough room */
	if (p_ptr->inven_cnt >= INVEN_PACK) {
		msg_print(Ind, "You have no room to steal anything.");
		return;
	}

	/* Examine target grid */
	c_ptr = &zcave[p_ptr->py + ddy[dir]][p_ptr->px + ddx[dir]];

	/* May only steal from players */
	if (c_ptr->m_idx >= 0) {
		msg_print(Ind, "You see nothing there to steal from.");
		return;
	}
	else if (c_ptr->m_idx > 0) {
		do_cmd_steal_from_monster(Ind, dir);
		return;
	}

	/* IDDC - don't get exp */
	if ((p_ptr->mode & MODE_DED_IDDC) && !in_irondeepdive(&p_ptr->wpos)) {
		msg_print(Ind, "You cannot steal from someone or your life would be forfeit.");
		return;
	}

	if (p_ptr->inval) {
		msg_print(Ind, "You cannot steal from other players without a valid account.");
		return;
	}

	if (p_ptr->max_plv < cfg.newbies_cannot_drop) {
		msg_format(Ind, "You cannot steal from other players until you are level %d.", cfg.newbies_cannot_drop);
		return;
	}

	/* Examine target */
	q_ptr = Players[0 - c_ptr->m_idx];
	etiquette =
	    ((cfg.fallenkings_etiquette && q_ptr->once_winner && !q_ptr->total_winner) ||
	    (cfg.kings_etiquette && q_ptr->total_winner)) ||
	    ((cfg.fallenkings_etiquette && p_ptr->once_winner && !p_ptr->total_winner) ||
	    (cfg.kings_etiquette && p_ptr->total_winner));

	/* No transactions from different mode */
	if (compat_pmode(Ind, 0 - c_ptr->m_idx, FALSE)) {
		msg_format(Ind, "You cannot steal from %s players.", compat_pmode(Ind, 0 - c_ptr->m_idx, FALSE));
		return;
	}

#ifdef IDDC_IRON_COOP
	if (in_irondeepdive(&p_ptr->wpos) && (!p_ptr->party || p_ptr->party != q_ptr->party)) {
		msg_print(Ind, "\377yYou cannot take items from outsiders.");
		if (!is_admin(p_ptr)) return;
	}
#endif
#ifdef IRON_IRON_TEAM
	if (p_ptr->party && (parties[p_ptr->party].mode & PA_IRONTEAM) && p_ptr->party != q_ptr->party) {
		msg_print(Ind, "\377yYou cannot take items from outsiders.");
		if (!is_admin(p_ptr)) return;
	}
#endif

	/* Small delay to prevent crazy steal-spam */
	if (p_ptr->pstealing) {
		msg_print(Ind, "You're still not calm enough to steal again..");
		return;
	}

#ifdef TOWN_NO_STEALING
	/* no stealing in town since town-pvp is diabled - except if in same party (for fun :-p) */
	if (istown(&p_ptr->wpos) && !player_in_party(q_ptr->party, Ind)) {
		msg_print(Ind, "\377oYou may not steal from strangers in town.");
		return;
	}
#endif
#ifdef PROTECTED_NO_STEALING
	if ((c_ptr->info & CAVE_PROT) || (f_info[c_ptr->feat].flags1 & FF1_PROTECTED)) {
		msg_print(Ind, "\377oThis location is protected and does not allow stealing.");
		return;
	}
#endif

	/* May not steal from AFK players, sportsmanship ;) - C. Blue */
	if (q_ptr->afk) {
		msg_print(Ind, "You may not steal from players who are AFK.");
		return;
	}

	if (is_admin(q_ptr) && !is_admin(p_ptr)) {
		msg_print(Ind, "Really? Darwin award protection prevents stealing from admins.");
		return;
	}

	/* S(he) is no longer afk */
	un_afk_idle(Ind);

	/* May not steal from hostile players */
	/* I doubt if it's reasonable..dunno	- Jir - */
#if 0 /* turned off now */
	if (check_hostile(0 - c_ptr->m_idx, Ind)) {
		/* Message */
		msg_format(Ind, "%^s is on guard against you.", q_ptr->name);
		return;
	}
#endif
	dal = (p_ptr->lev > q_ptr->lev ? p_ptr->lev - q_ptr->lev : 1);

	/* affect alignment on attempt (after hostile check) */
	/* evil thief! stealing from newbies */
	if (q_ptr->lev + 5 < p_ptr->lev){
		if((p_ptr->align_good) < (0xffff - dal))
			p_ptr->align_good += dal;
		else p_ptr->align_good = 0xffff;	/* very evil */
	}
	/* non lawful action in town :) */
	if (istown(&p_ptr->wpos) && (p_ptr->align_law) < (0xffff - dal))
		p_ptr->align_law += dal;
	else p_ptr->align_law = 0xffff;

	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);

#if 1 /* maybe rework this */
	/* Compute chance of success */
	success = 3 * (adj_dex_safe[p_ptr->stat_ind[A_DEX]] - adj_dex_safe[q_ptr->stat_ind[A_DEX]]);
	success += 2 * (UNAWARENESS(q_ptr) - UNAWARENESS(p_ptr));

	/* Compute base chance of being noticed */
	notice = (5 * (adj_mag_stat[q_ptr->stat_ind[A_INT]] - p_ptr->skill_stl)) / 3;

	/* Reversed this as suggested by Potter - mikaelh */
	notice -= 1 * (UNAWARENESS(q_ptr) - UNAWARENESS(p_ptr));

	//notice -= q_ptr->skill_fos; /* perception */

	/* Hack -- Rogues get bonuses to chances */
	if (get_skill(p_ptr, SKILL_STEALING)) {
		/* Increase chance by level */
		success += get_skill_scale(p_ptr, SKILL_STEALING, 150);
		notice -= get_skill_scale(p_ptr, SKILL_STEALING, 150);
	}
	/* Similar Hack -- Robber is hard to be robbed */
	if (get_skill(q_ptr, SKILL_STEALING)) {
		/* Increase chance by level */
		success -= get_skill_scale(p_ptr, SKILL_STEALING, 100);
		notice += get_skill_scale(p_ptr, SKILL_STEALING, 100);
	}

	/* Always small chance to fail */
	if (success > 95) success = 95;
	//if (notice < 5) notice = 5;

	/* Hack -- Always small chance to succeed */
	if (success < 2) success = 2;
#else
#endif

	/* Check for success */
	if (rand_int(100) < success) {
		/* Steal gold 25% of the time */
		if (rand_int(100) < 25) {
			int amt = q_ptr->au / 10;

			if (TOOL_EQUIPPED(q_ptr) == SV_TOOL_MONEY_BELT && magik (70)) {
				/* Saving throw message */
				msg_print(Ind, "You couldn't find any money!");
				amt = 0;
				s_printf("StealingPvP: %s fails to steal %d gold from %s (chance %d%%): money belt.\n", p_ptr->name, amt, q_ptr->name, success);
			}

			/* Transfer gold */
			if (amt) {
				/* Move from target to thief */
				q_ptr->au -= amt;
				gain_au(Ind, amt, FALSE, FALSE);
				/* Redraw */
				q_ptr->redraw |= (PR_GOLD);

				/* Tell thief */
				msg_format(Ind, "You steal %d gold.", amt);
				s_printf("StealingPvP: %s steals %d gold from %s (chance %d%%).\n", p_ptr->name, amt, q_ptr->name, success);
			}

			/* Always small chance to be noticed */
			if (notice < 5) notice = 5;

			/* Check for target noticing */
			if (rand_int(100) < notice) {
				/* Message */
				msg_format(0 - c_ptr->m_idx, "\377rYou notice %s stealing %d gold!", p_ptr->name, amt);
				caught = TRUE;
			}
		} else {
			int item;
			object_type *o_ptr, forge;
			char o_name[ONAME_LEN];

			/* Steal an item */
			item = rand_int(q_ptr->inven_cnt);

			/* Get object */
			o_ptr = &q_ptr->inventory[item];
			forge = *o_ptr;

			if (TOOL_EQUIPPED(q_ptr) == SV_TOOL_THEFT_PREVENTION && magik (80)) {
				/* Saving throw message */
				msg_print(Ind, "Your attempt to steal was interfered with by a strange device!");
				notice += 50;
				s_printf("StealingPvP: %s fails to steal from %s (chance %d%%): theft prevention.\n", p_ptr->name, q_ptr->name, success);
			}
			/* artifacts are HARD to steal. Cannot steal quest items or guild keys. */
			else if ((o_ptr->name1 && (rand_int(500) > success || etiquette)) ||
			    o_ptr->questor || o_ptr->quest || (o_ptr->tval == TV_KEY && o_ptr->sval == SV_GUILD_KEY)) {
				msg_print(Ind, "The object itself seems to evade your hand!");
				s_printf("StealingPvP: %s fails to steal from %s (chance %d%%): restricted item (1).\n", p_ptr->name, q_ptr->name, success);
			}
			else if (((k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) &&
#ifdef FALLEN_WINNERSONLY
			    !p_ptr->once_winner
#else
			    !p_ptr->total_winner
#endif
			    )
			    /* prevent winners picking up true arts accidentally */
			    || (true_artifact_p(o_ptr) && !winner_artifact_p(o_ptr) &&
			    p_ptr->total_winner && cfg.kings_etiquette)
#ifndef RPG_SERVER
			    || ((o_ptr->level > p_ptr->lev || o_ptr->level == 0) &&
			    !in_irondeepdive(&p_ptr->wpos) &&
			    (cfg.anti_cheeze_pickup || (true_artifact_p(o_ptr) && cfg.anti_arts_pickup)))
#endif
			    ) {
				msg_print(Ind, "The object itself seems to evade your hand!");
				s_printf("StealingPvP: %s fails to steal from %s (chance %d%%): restricted item (2).\n", p_ptr->name, q_ptr->name, success);
			} else {
				/* Turn level 0 food into level 1 food - mikaelh */
				if (o_ptr->level == 0 && shareable_starter_item(o_ptr)) {
					o_ptr->level = 1;
					o_ptr->discount = 100;
				}

				/* Give one item to thief */
				if (is_magic_device(o_ptr->tval)) divide_charged_item(&forge, o_ptr, 1);
				forge.number = 1;
				inven_carry(Ind, &forge);

				/* Take one from target */
				inven_item_increase(0 - c_ptr->m_idx, item, -1);
				inven_item_optimize(0 - c_ptr->m_idx, item);

				/* Tell thief what he got */
				object_desc(0, o_name, &forge, TRUE, 3);
				s_printf("StealingPvP: %s steals item %s from %s (chance %d%%).\n", p_ptr->name, o_name, q_ptr->name, success);
				object_desc(Ind, o_name, &forge, TRUE, 3);
				msg_format(Ind, "You stole %s.", o_name);

				if (true_artifact_p(o_ptr)) a_info[o_ptr->name1].carrier = p_ptr->id;

				/* Some events don't allow transactions before they begin */
				if (!p_ptr->max_exp) {
					msg_print(Ind, "You gain a tiny bit of experience from trading an item.");
					gain_exp(Ind, 1);
				}

				can_use(Ind, o_ptr);
				/* for Ironman Deep Dive Challenge cross-trading */
				o_ptr->mode = p_ptr->mode;

				/* Check whether this item was requested by an item-retrieval quest */
				if (p_ptr->quest_any_r_within_target) quest_check_goal_r(Ind, o_ptr);
			}

			/* Easier to notice heavier objects */
			notice += forge.weight;
			//	/ (get_skill_scale(SKILL_STEALING, 19) + 1);

			/* Always small chance to be noticed */
			if (notice < 5) notice = 5;

			/* Check for target noticing */
			if (rand_int(100) < notice) {
				/* Message */
				msg_format(0 - c_ptr->m_idx, "\377rYou notice %s stealing %s!", p_ptr->name, o_name);
				caught = TRUE;
			}
		}
	} else {
		msg_print(Ind, "You fail to steal anything.");
		s_printf("StealingPvP: %s fails to steal from %s.\n", p_ptr->name, q_ptr->name);

		/* Always small chance to be noticed */
		if (notice < 5) notice = 5;

		/* Easier to notice a failed attempt */
		if (rand_int(100) < notice + 50) {
			msg_format(0 - c_ptr->m_idx, "\377rYou notice %s trying to steal from you!", p_ptr->name);
			caught = TRUE;
		}
	}

	if (caught) break_cloaking(Ind, 0);

#if 1 /* Send him to jail! Only if not in party maybe? :) */
	if (caught
 #if 0
	    && !player_in_party(q_ptr->party, Ind)
 #endif
	    ) {
		bool je = jails_enabled;

		msg_print(Ind, "\377oYou have been seized by the guards!");
		msg_format_near(Ind, "%s is seized by the guards and thrown into jail!", p_ptr->name);
		jails_enabled = TRUE; //hack, in case it's disabled for swearing
		imprison(Ind, JAIL_STEALING, "stealing");
		jails_enabled = je;
	}
#endif
#if 0 /* Counter blow! (now turned off) */
	if (caught) {
		int i, j;
		object_type *o_ptr;

		/* Purge this traitor */
		if (player_in_party(q_ptr->party, Ind)) {
			int party = p_ptr->party;

			/* Temporary leave for the message */
			p_ptr->party = 0;

			/* Messages */
			msg_print(Ind, "\377oYou have been purged from your party.");
			party_msg_format(q_ptr->party, "\377R%s has betrayed your party!", p_ptr->name);

			p_ptr->party = party;
			party_leave(Ind, FALSE);

		}

		/* Make target hostile */
		if (q_ptr->exp > p_ptr->exp / 2 - 200) {
			if (Players[0 - c_ptr->m_idx]->pvpexception < 2)
			add_hostility(0 - c_ptr->m_idx, p_ptr->name, FALSE);
		}

		/* Message */
		msg_format(Ind, "\377r%s gave you an unexpected blow!",
		           q_ptr->name);

		set_stun_raw(Ind, p_ptr->stun + randint(50));
		set_confused(Ind, p_ptr->confused + rand_int(20) + 10);
		if (cfg.use_pk_rules == PK_RULES_DECLARE)
			p_ptr->pkill |= PKILL_KILLABLE;

		/* Thief drops some items from the shock of blow */
		if (cfg.newbies_cannot_drop <= p_ptr->lev && !p_ptr->inval) {
			for(i = rand_int(5); i < 5 ; i++ ) {
				j = rand_int(INVEN_TOTAL);
				o_ptr = &(p_ptr->inventory[j]);

				if (!o_ptr->tval) continue;

				/* He never drops body-armour this way */
				if (j == INVEN_BODY) continue;

				/* An artifact 'resists' */
				if (true_artifact_p(o_ptr) && rand_int(100) > 2)
					continue;
				inven_drop(Ind, j, randint(o_ptr->number * 2));
			}
		}

		/* The target gets angry */
		set_fury(0 - c_ptr->m_idx, q_ptr->fury + 15 + randint(15));

	}
#endif

	/* set timeout before attempting to pvp-steal again */
	p_ptr->pstealing = 15; /* 15 turns aka ~10 seconds */

	/* Take a turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos);
}







/*
 * An "item_tester_hook" for refilling lanterns
 */
static bool item_tester_refill_lantern(object_type *o_ptr)
{
	/* (Rand)arts are not usable for refilling */
	if (o_ptr->name1) return (FALSE);

	/* Flasks of oil are okay */
	if (o_ptr->tval == TV_FLASK) return (TRUE);

	/* Other lanterns are okay */
	if ((o_ptr->tval == TV_LITE) &&
	    (o_ptr->sval == SV_LITE_LANTERN) &&
	    o_ptr->timeout)
		return (TRUE);

	/* Assume not okay */
	return (FALSE);
}


/*
 * Refill the players lamp (from the pack or floor)
 */
static void do_cmd_refill_lamp(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	object_type *o_ptr;
	object_type *j_ptr;

	long int used_fuel = 0, spilled_fuel = 0, available_fuel = 0;


	/* Restrict the choices */
	item_tester_hook = item_tester_refill_lantern;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &(p_ptr->inventory[item]);
	/* Get the item (on the floor) */
	else {
		if (-item >= o_max) return; /* item doesn't exist */
		o_ptr = &o_list[0 - item];
	}

	if (!item_tester_hook(o_ptr)) {
		msg_print(Ind, "You cannot refill with that!");
		return;
	}

	if (check_guard_inscription(o_ptr->note, 'F')) {
		msg_print(Ind, "The item's incription prevents it.");
		return;
	}

	/* Too kind? :) */
	if (artifact_p(o_ptr)) {
		msg_print(Ind, "Your light seems to resist!");
		return;
	}

	if (o_ptr->tval != TV_FLASK && o_ptr->timeout == 0) {
		msg_print(Ind, "That item has no fuel left!");
		return;
	}

	/* Let's not end afk for this. - C. Blue */
/*	un_afk_idle(Ind); */

	/* Take a partial turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

	/* Access the lantern */
	j_ptr = &(p_ptr->inventory[INVEN_LITE]);

	/* Refuel */
	used_fuel = j_ptr->timeout;
	if (o_ptr->tval == TV_FLASK) {
		j_ptr->timeout += o_ptr->pval;
	} else {
		spilled_fuel = (o_ptr->timeout * (randint(5) + (130 - adj_dex_th_mul[p_ptr->stat_ind[A_DEX]]) / 2)) / 100; /* spill some */
		available_fuel = o_ptr->timeout - spilled_fuel;
		j_ptr->timeout += available_fuel;
	}

	/* Message */
	msg_print(Ind, "You fuel your lamp.");

	/* Comment */
	if (j_ptr->timeout >= FUEL_LAMP) {
		j_ptr->timeout = FUEL_LAMP;
		msg_print(Ind, "Your lamp is full.");
	}
	used_fuel = j_ptr->timeout - used_fuel;

	if (o_ptr->tval == TV_FLASK || item <= 0) { /* just dispose of for now, if it was from the ground */
		/* Decrease the item (from the pack) */
		if (item >= 0) {
			inven_item_increase(Ind, item, -1);
			inven_item_describe(Ind, item);
			inven_item_optimize(Ind, item);
		}

		/* Decrease the item (from the floor) */
		else {
			floor_item_increase(0 - item, -1);
			floor_item_describe(0 - item);
			floor_item_optimize(0 - item);
		}
	} else { /* TV_LITE && SV_LITE_LANTERN */
		/* unstack lanterns if we used one of them for refilling */
		if ((item >= 0) && (o_ptr->number > 1)) {
			/* Make a fake item */
			object_type tmp_obj;
			tmp_obj = *o_ptr;
			tmp_obj.number = 1;

			/* drain its fuel */
			/* big numbers (*1000) to fix rounding errors: */
			tmp_obj.timeout -= ((used_fuel * 100000) / (100000 - ((100000 * spilled_fuel) / tmp_obj.timeout)));
			/* quick hack to hopefully fix rounding errors: */
			if (tmp_obj.timeout == 1) tmp_obj.timeout = 0;

			/* Unstack the used item */
			o_ptr->number--;
			p_ptr->total_weight -= tmp_obj.weight;
			tmp_obj.iron_trade = o_ptr->iron_trade;
			item = inven_carry(Ind, &tmp_obj);

			/* Message */
			msg_print(Ind, "You unstack your lanterns.");
		} else {
			/* big numbers (*1000) to fix rounding errors: */
			o_ptr->timeout -= ((used_fuel * 100000) / (100000 - ((100000 * spilled_fuel) / o_ptr->timeout)));
			/* quick hack to hopefully fix rounding errors: */
			if (o_ptr->timeout == 1) o_ptr->timeout = 0;
//			inven_item_describe(Ind, item);
		}
	}

	/* Recalculate torch */
	p_ptr->update |= (PU_TORCH | PU_LITE);
	p_ptr->window |= (PW_INVEN | PW_EQUIP);
	/* If multiple lanterns are now 0 turns, they can be combined */
	p_ptr->notice |= (PN_COMBINE);
}



/*
 * An "item_tester_hook" for refilling torches
 */
static bool item_tester_refill_torch(object_type *o_ptr)
{
	/* (Rand)arts are not usable for refilling */
	if (o_ptr->name1) return (FALSE);

	/* Torches are okay */
	if ((o_ptr->tval == TV_LITE) &&
	    (o_ptr->sval == SV_LITE_TORCH) &&
	    o_ptr->timeout)
		return (TRUE);

	/* Assume not okay */
	return (FALSE);
}


/*
 * Refuel the players torch (from the pack or floor)
 */
static void do_cmd_refill_torch(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	object_type *o_ptr;
	object_type *j_ptr;


	/* Restrict the choices */
	item_tester_hook = item_tester_refill_torch;

	if (item > INVEN_TOTAL) return;

	/* Get the item (in the pack) */
	if (item >= 0) {
		o_ptr = &(p_ptr->inventory[item]);
	}

	/* Get the item (on the floor) */
	else {
		if (-item >= o_max)
			return; /* item doesn't exist */

		o_ptr = &o_list[0 - item];
	}

	if (!item_tester_hook(o_ptr)) {
		msg_print(Ind, "You cannot refill with that!");
		return;
	}

	if (check_guard_inscription(o_ptr->note, 'F')) {
		msg_print(Ind, "The item's incription prevents it.");
		return;
	}

	/* Too kind? :) */
	if (artifact_p(o_ptr)) {
		msg_print(Ind, "Your light seems to resist!");
		return;
	}

	/* Let's not end afk for this. - C. Blue */
/*	un_afk_idle(Ind); */

	/* Take a partial turn */
	p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;

	/* Access the primary torch */
	j_ptr = &(p_ptr->inventory[INVEN_LITE]);

	/* Refuel */
	j_ptr->timeout += o_ptr->timeout;
	/* Lose very slightly sometimes */
	if (o_ptr->timeout >= 10) j_ptr->timeout -= rand_int(5);
 	else j_ptr->timeout -= o_ptr->timeout / (2 + rand_int(5));

	/* Message */
	msg_print(Ind, "You combine the torches.");

	/* Over-fuel message */
	if (j_ptr->timeout >= FUEL_TORCH) {
		j_ptr->timeout = FUEL_TORCH;
		msg_print(Ind, "Your torch is fully fueled.");
	}

	/* Refuel message */
	else {
		msg_print(Ind, "Your torch glows more brightly.");
	}

	/* Decrease the item (from the pack) */
	if (item >= 0) {
		inven_item_increase(Ind, item, -1);
		inven_item_describe(Ind, item);
		inven_item_optimize(Ind, item);
	}

	/* Decrease the item (from the floor) */
	else {
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

	/* Recalculate torch */
	p_ptr->update |= (PU_TORCH);
	p_ptr->window |= (PW_INVEN | PW_EQUIP);
}




/*
 * Refill the players lamp, or restock his torches
 */
void do_cmd_refill(int Ind, int item)
{
	player_type *p_ptr = Players[Ind];

	object_type *o_ptr;
	u32b f1, f2, f3, f4, f5, f6, esp;

	/* Get the light */
	o_ptr = &(p_ptr->inventory[INVEN_LITE]);

	/* It is nothing */
	if (o_ptr->tval != TV_LITE || !o_ptr->k_idx) {
		msg_print(Ind, "You are not wielding a light.");
		return;
	}

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	if (!(f4 & TR4_FUEL_LITE)) {
		msg_print(Ind, "Your light cannot be refilled.");
		return;
	}

	/* It's a lamp */
	else if (o_ptr->sval == SV_LITE_LANTERN) {
		do_cmd_refill_lamp(Ind, item);
	}

	/* It's a torch */
	else if (o_ptr->sval == SV_LITE_TORCH) {
		do_cmd_refill_torch(Ind, item);
	}

	/* No torch to refill */
	else {
		msg_print(Ind, "Your light cannot be refilled.");
	}
}

/*
 * Refill the players lamp, or restock his torches automatically.	- Jir -
 */
bool do_auto_refill(int Ind)
{
	player_type *p_ptr = Players[Ind];

	object_type *o_ptr;
	object_type *j_ptr = NULL;

	int i;
	u32b f1, f2, f3, f4, f5, f6, esp;

	/* Get the light */
	o_ptr = &(p_ptr->inventory[INVEN_LITE]);

	if( check_guard_inscription( o_ptr->note, 'F' )) {
		//msg_print(Ind, "The item's incription prevents it.");
		return (FALSE);
	}

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* It's a lamp */
	if (o_ptr->sval == SV_LITE_LANTERN) {
		/* Restrict the choices */
		item_tester_hook = item_tester_refill_lantern;

		for(i = 0; i < INVEN_PACK; i++) {
			j_ptr = &(p_ptr->inventory[i]);
			if (!item_tester_hook(j_ptr)) continue;
			if (artifact_p(j_ptr) || ego_item_p(j_ptr)) continue;
			if (check_guard_inscription(j_ptr->note, 'F')) continue;

			do_cmd_refill_lamp(Ind, i);
			return (TRUE);
		}
	}

	/* It's a torch */
	else if (o_ptr->sval == SV_LITE_TORCH) {
		/* Restrict the choices */
		item_tester_hook = item_tester_refill_torch;

		for(i = 0; i < INVEN_PACK; i++) {
			j_ptr = &(p_ptr->inventory[i]);
			if (!item_tester_hook(j_ptr)) continue;
			if (artifact_p(j_ptr) || ego_item_p(j_ptr)) continue;
			if (check_guard_inscription(j_ptr->note, 'F')) continue;

			do_cmd_refill_torch(Ind, i);
			return (TRUE);
		}
	}

	/* Assume false */
	return (FALSE);
}





/*
 * Target command
 */
void do_cmd_target(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];

	/* Set the target */
	if (target_set(Ind, dir) && !p_ptr->taciturn_messages) {
//		msg_print(Ind, "Target Selected.");
	} else {
		/*msg_print(Ind, "Target Aborted.");*/
	}
}

void do_cmd_target_friendly(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind];

	/* Set the target */
	if (target_set_friendly(Ind, dir) && !p_ptr->taciturn_messages) {
//		msg_print(Ind, "Target Selected.");
	} else {
		/*msg_print(Ind, "Target Aborted.");*/
	}
}


/*
 * Hack -- determine if a given location is "interesting" to a player
 */
static bool do_cmd_look_accept(int Ind, int y, int x) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;

	cave_type *c_ptr;
	byte *w_ptr;
	struct c_special *cs_ptr;

	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Examine the grid */
	c_ptr = &zcave[y][x];
	w_ptr = &p_ptr->cave_flag[y][x];

	/* Player grids */
	if (c_ptr->m_idx < 0 && p_ptr->play_vis[-c_ptr->m_idx]) {
#if 0
		player_type *q_ptr = Players[-c_ptr->m_idx];

		if ((!q_ptr->admin_dm || player_sees_dm(Ind)) &&
		    (player_has_los_bold(Ind, y, x) || p_ptr->telepathy))
#endif
			return (TRUE);
	}

	/* Visible monsters */
	if (c_ptr->m_idx > 0 && p_ptr->mon_vis[c_ptr->m_idx]) {
		/* Visible monsters */
		if (p_ptr->mon_vis[c_ptr->m_idx]) return (TRUE);
	}

	/* Objects */
	if (c_ptr->o_idx) {
		/* Memorized object */
		if (p_ptr->obj_vis[c_ptr->o_idx] || p_ptr->admin_dm) return (TRUE); /* finally, poor dm - C. Blue */
	}

	/* Traps */
	if ((cs_ptr = GetCS(c_ptr, CS_TRAPS))) {
		/* Revealed trap */
		if (cs_ptr->sc.trap.found) return (TRUE);
	}

	/* Monster Traps */
	if (GetCS(c_ptr, CS_MON_TRAP)) {
		return (TRUE);
	}

	/* Interesting memorized features */
	if (*w_ptr & CAVE_MARK) {
#if 0	// wow!
		/* Notice glyphs */
		if (c_ptr->feat == FEAT_GLYPH) return (TRUE);
		if (c_ptr->feat == FEAT_RUNE) return (TRUE);

		/* Notice doors */
		if (c_ptr->feat == FEAT_OPEN) return (TRUE);
		if (c_ptr->feat == FEAT_BROKEN) return (TRUE);

		/* Notice stairs */
		if (c_ptr->feat == FEAT_LESS) return (TRUE);
		if (c_ptr->feat == FEAT_MORE) return (TRUE);

		/* Notice shops */
		if (c_ptr->feat == FEAT_SHOP) return (TRUE);
#if 0
		if ((c_ptr->feat >= FEAT_SHOP_HEAD) &&
		    (c_ptr->feat <= FEAT_SHOP_TAIL)) return (TRUE);
#endif	// 0

		/* Notice doors */
		if ((c_ptr->feat >= FEAT_DOOR_HEAD) &&
		    (c_ptr->feat <= FEAT_DOOR_TAIL)) return (TRUE);

		/* Notice rubble */
		if (c_ptr->feat == FEAT_RUBBLE) return (TRUE);

		/* Notice veins with treasure */
		if (c_ptr->feat == FEAT_MAGMA_K) return (TRUE);
		if (c_ptr->feat == FEAT_QUARTZ_K) return (TRUE);
		if (c_ptr->feat == FEAT_SANDWALL_K) return (TRUE);
#endif	// 0

		/* Accept 'naturally' interesting features */
		if (f_info[c_ptr->feat].flags1 & FF1_NOTICE) return (TRUE);
	}

	/* Nope */
	return (FALSE);
}



/*
 * A new "look" command, similar to the "target" command.
 *
 * The "player grid" is always included in the "look" array, so
 * that this command will normally never "fail".
 *
 * XXX XXX XXX Allow "target" inside the "look" command (?)
 */
void do_cmd_look(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	player_type *q_ptr;
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	int		y, x, i;

	cave_type *c_ptr;
	monster_type *m_ptr;
	object_type *o_ptr;

	cptr p1 = "A ", p2 = "", info = "";
	struct c_special *cs_ptr;

	char o_name[ONAME_LEN];
	char out_val[ONAME_LEN], tmp_val[ONAME_LEN];

	if(!(zcave = getcave(wpos))) return;

	/* Blind */
	if (p_ptr->blind) {
		msg_print(Ind, "You can't see a damn thing!");
		return;
	}

	/* Hallucinating */
	if (p_ptr->image) {
		msg_print(Ind, "You can't believe what you are seeing!");
		return;
	}


	/* Reset "temp" array */
	/* Only if this is the first time, or if we've been asked to reset */
	if (!dir) {
		p_ptr->target_n = 0;

		/* Scan the current panel */
		for (y = p_ptr->panel_row_min; y <= p_ptr->panel_row_max; y++) {
			for (x = p_ptr->panel_col_min; x <= p_ptr->panel_col_max; x++) {
				/* Require line of sight, unless "look" is "expanded" */
				if (!player_has_los_bold(Ind, y, x)) continue;

				/* Require interesting contents */
				if (!do_cmd_look_accept(Ind, y, x)) continue;

				/* Save the location */
				p_ptr->target_x[p_ptr->target_n] = x;
				p_ptr->target_y[p_ptr->target_n] = y;
				p_ptr->target_n++;
			}
		}

		/* Start near the player */
		p_ptr->look_index = 0;

		/* Paranoia */
		if (!p_ptr->target_n) {
			msg_print(Ind, "You see nothing special. [<dir>, p, q]");
			return;
		}


		/* Set the sort hooks */
		ang_sort_comp = ang_sort_comp_distance;
		ang_sort_swap = ang_sort_swap_distance;

		/* Sort the positions */
		ang_sort(Ind, p_ptr->target_x, p_ptr->target_y, p_ptr->target_n);

		/* Collect monster and player indices */
		for (i = 0; i < p_ptr->target_n; i++) {
			c_ptr = &zcave[p_ptr->target_y[i]][p_ptr->target_x[i]];

			if (c_ptr->m_idx != 0)
				p_ptr->target_idx[i] = c_ptr->m_idx;
			else p_ptr->target_idx[i] = 0;
		}

        } else if (dir >= 128) {
		/* Initialize if needed */
		if (dir == 128) {
			x = p_ptr->target_col = p_ptr->px;
			y = p_ptr->target_row = p_ptr->py;
		} else {
			x = p_ptr->target_col + ddx[dir - 128];
			if (!player_has_los_bold(Ind, p_ptr->target_row, x) && !is_admin(p_ptr)) x = p_ptr->target_col;
			y = p_ptr->target_row + ddy[dir - 128];
			if (!player_has_los_bold(Ind, y, p_ptr->target_col) && !is_admin(p_ptr)) y = p_ptr->target_row;

			if (!in_bounds(y, x)) return;

			p_ptr->target_col = x;
			p_ptr->target_row = y;
		}

		/* Check for confirmation to actually look at this grid */
		if (dir == 128 + 5) {
			p_ptr->look_index = 0;
			p_ptr->target_x[0] = p_ptr->target_col;
			p_ptr->target_y[0] = p_ptr->target_row;
		}
		/* Just manually ground-targetting - do not spam look-info for each single grid */
		else {
			/* Info */
			if (p_ptr->rogue_like_commands) strcpy(out_val, "[<dir>, x, p, q] ");
			else strcpy(out_val, "[<dir>, l, p, q] ");

			/* Tell the client */
			Send_target_info(Ind, x - p_ptr->panel_col_prt, y - p_ptr->panel_row_prt, out_val);

			return;
		}
	}

	/* Motion */
	else {
		/* Reset the locations */
		for (i = 0; i < p_ptr->target_n; i++) {
			if (p_ptr->target_idx[i] > 0) {
				m_ptr = &m_list[p_ptr->target_idx[i]];

				p_ptr->target_y[i] = m_ptr->fy;
				p_ptr->target_x[i] = m_ptr->fx;
			} else if (p_ptr->target_idx[i] < 0) {
				q_ptr = Players[0 - p_ptr->target_idx[i]];

				/* Check for player leaving */
				if (((0 - p_ptr->target_idx[i]) > NumPlayers) ||
				     (!inarea(&q_ptr->wpos, &p_ptr->wpos))) {
					p_ptr->target_y[i] = 0;
					p_ptr->target_x[i] = 0;
				} else {
					p_ptr->target_y[i] = q_ptr->py;
					p_ptr->target_x[i] = q_ptr->px;
				}
			}
		}

		/* Find a new grid if possible */
		i = target_pick(Ind, p_ptr->target_y[p_ptr->look_index], p_ptr->target_x[p_ptr->look_index], ddy[dir], ddx[dir]);

		/* Use that grid */
		if (i >= 0) p_ptr->look_index = i;
	}

	/* Describe */
	y = p_ptr->target_y[p_ptr->look_index];
	x = p_ptr->target_x[p_ptr->look_index];

	/* Paranoia */
	/* thats extreme paranoia */
	if (!(zcave = getcave(wpos))) return;
	c_ptr = &zcave[y][x];

	if (c_ptr->m_idx < 0 && p_ptr->play_vis[0-c_ptr->m_idx] &&
	    (!Players[0-c_ptr->m_idx]->admin_dm || player_sees_dm(Ind))) {
		q_ptr = Players[0 - c_ptr->m_idx];

		/* Track health */
		health_track(Ind, c_ptr->m_idx);

		/* Format string */
		if ((q_ptr->inventory[INVEN_BODY].tval == TV_SOFT_ARMOR) && (q_ptr->inventory[INVEN_BODY].sval == SV_COSTUME)) {
			snprintf(out_val, sizeof(out_val), "%s the %s (%s)", q_ptr->name, r_name + r_info[q_ptr->inventory[INVEN_BODY].bpval].name, get_ptitle(q_ptr, FALSE));
		} else if (q_ptr->body_monster) {
			snprintf(out_val, sizeof(out_val), "%s the %s (%s)", q_ptr->name, r_name + r_info[q_ptr->body_monster].name, get_ptitle(q_ptr, FALSE));
		} else {
#if 0 /* use normal race_info.title */
			snprintf(out_val, sizeof(out_val), "%s the %s %s", q_ptr->name, race_info[q_ptr->prace].title, get_ptitle(q_ptr, FALSE));
			//, class_info[q_ptr->pclass].title
#else /* use special_prace_lookup */
			snprintf(out_val, sizeof(out_val), "%s the %s %s", q_ptr->name, get_prace(q_ptr), get_ptitle(q_ptr, FALSE));
#endif
		}
	} else if (c_ptr->m_idx > 0 && p_ptr->mon_vis[c_ptr->m_idx]) {	/* TODO: handle monster mimics */
		bool done_unique;
		m_ptr = &m_list[c_ptr->m_idx];

		/* a unique which the looker already killed? */
		if ((r_info[m_ptr->r_idx].flags1 & RF1_UNIQUE) &&
		    p_ptr->r_killed[m_ptr->r_idx] == 1)
			done_unique = TRUE;
		else done_unique = FALSE;

		/* Track health */
		health_track(Ind, c_ptr->m_idx);

		/* Format string */
		if (m_ptr->questor)
			snprintf(out_val, sizeof(out_val), "\377%c%s (Lv %d, %s)",
			    m_ptr->questor_invincible ? 'G' : ((m_ptr->questor_hostile & 0x1) ? 'R' : 'G'),
			    r_name_get(&m_list[c_ptr->m_idx]),
			    m_ptr->level, look_mon_desc(c_ptr->m_idx));
		else
#if 0 /* attach 'slain' for uniques we already killed */
//                snprintf(out_val, sizeof(out_val), "%s (%s)", r_name_get(&m_list[c_ptr->m_idx]), look_mon_desc(c_ptr->m_idx));
		snprintf(out_val, sizeof(out_val), "%s (Lv %d, %s%s)", r_name_get(&m_list[c_ptr->m_idx]),
		    m_ptr->level, look_mon_desc(c_ptr->m_idx),
		    m_ptr->clone ? ", clone" : (done_unique ? ", slain" : ""));
#else /* use different colour for uniques we already killed */
		snprintf(out_val, sizeof(out_val), "%s%s (Lv %d, %s%s)",
		    done_unique ? "\377D" : "",
		    r_name_get(&m_list[c_ptr->m_idx]),
		    m_ptr->level, look_mon_desc(c_ptr->m_idx),
		    m_ptr->clone ? ", clone" : "");
#endif
	} else if (c_ptr->o_idx) {
		o_ptr = &o_list[c_ptr->o_idx];

		/* Format string */
		if (o_ptr->questor) {
#if 0
			snprintf(out_val, sizeof(out_val), "\377%c%sYou see %s%s",
			    o_ptr->questor_invincible ? 'G' : 'G', compat_pomode(Ind, o_ptr) ? "\377D" : "",
			    q_info[o_ptr->quest - 1].questor[o_ptr->questor_idx].name, o_ptr->next_o_idx ? " on a pile" : "");
#else
			object_desc(Ind, o_name, o_ptr, TRUE, 3);
			snprintf(out_val, sizeof(out_val), "\377%c%sYou see %s%s",
			    o_ptr->questor_invincible ? 'G' : 'G', compat_pomode(Ind, o_ptr) ? "\377D" : "",
			    o_name, o_ptr->next_o_idx ? " on a pile" : "");
#endif
		} else {
			/* Obtain an object description */
			object_desc(Ind, o_name, o_ptr, TRUE, 3);

			snprintf(out_val, sizeof(out_val), "%sYou see %s%s",
			    (compat_pomode(Ind, o_ptr) && !exceptionally_shareable_item(o_ptr)) ? "\377D" : "", o_name, o_ptr->next_o_idx ? " on a pile" : "");
		}

		/* Check if the object is on a detected trap */
		if ((cs_ptr = GetCS(c_ptr, CS_TRAPS))) {
			int t_idx = cs_ptr->sc.trap.t_idx;
			if (cs_ptr->sc.trap.found) {
				if (p_ptr->trap_ident[t_idx])
					p1 = t_name + t_info[t_idx].name;
				else
					p1 = "trapped";

				/* Add trap description at the end */
				strcat(out_val, " {");
				strcat(out_val, p1);
				strcat(out_val, "}");
				/* Also add a ^ at the beginning of the line
				in case it's too long to read its end */
				strcpy(tmp_val, "^ ");
				strcat(tmp_val, out_val);
				strcpy(out_val, tmp_val);
			}
		}

	} else {
		int feat = f_info[c_ptr->feat].mimic;
		cptr name = f_name + f_info[feat].name;

		if (f_info[c_ptr->feat].flags2 & FF2_NO_ARTICLE) p1 = "";
		else if (is_a_vowel(name[0])) p1 = "An ";

		/* Hack -- add trap description */
		if ((cs_ptr = GetCS(c_ptr, CS_TRAPS))) {
			int t_idx = cs_ptr->sc.trap.t_idx;
			if (cs_ptr->sc.trap.found) {
				if (p_ptr->trap_ident[t_idx])
					p1 = t_name + t_info[t_idx].name;
				else
					p1 = "A trap";

				p2 = " on ";
			}
		}

		/* Hack -- special description for store doors */
		//if ((feat >= FEAT_SHOP_HEAD) && (feat <= FEAT_SHOP_TAIL))
		if (feat == FEAT_SHOP) {
			p1 = "The entrance to ";

			/* TODO: store name! */
			if ((cs_ptr = GetCS(c_ptr, CS_SHOP))) {
				name = st_name + st_info[cs_ptr->sc.omni].name;
			}

		}

		if ((feat == FEAT_FOUNTAIN) &&
		    (cs_ptr = GetCS(c_ptr, CS_FOUNTAIN)) &&
		    cs_ptr->sc.fountain.known) {
			object_kind *k_ptr;
			int tval, sval;

			if (cs_ptr->sc.fountain.type <= SV_POTION_LAST) {
				tval = TV_POTION;
				sval = cs_ptr->sc.fountain.type;
			} else {
				tval = TV_POTION2;
				sval = cs_ptr->sc.fountain.type - SV_POTION_LAST;
			}

			k_ptr = &k_info[lookup_kind(tval, sval)];
			info = k_name + k_ptr->name;
		}

		if (feat == FEAT_SIGN) /* give instructions how to actually read it ;) */
			name = "signpost \377D(bump to read)\377w";

		/* Message */
		if (strlen(info)) snprintf(out_val, sizeof(out_val), "%s%s%s (%s)", p1, p2, name, info);
		else snprintf(out_val, sizeof(out_val), "%s%s%s", p1, p2, name);

		if (f_info[c_ptr->feat].flags2 & FF2_NO_ARTICLE) out_val[0] = toupper(out_val[0]);
	}

	if (is_admin(p_ptr)) strcat(out_val, format(" (%d)", c_ptr->feat));

	/* Append a little info */
	strcat(out_val, " [<dir>, p, q]");

	/* Tell the client */
	Send_target_info(Ind, x - p_ptr->panel_col_prt, y - p_ptr->panel_row_prt, out_val);
}




/*
 * Allow the player to examine other sectors on the map
 */
void do_cmd_locate(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];

	int	y1, x1, y2, x2;
	int	prow = p_ptr->panel_row;
	int	pcol = p_ptr->panel_col;
	char	tmp_val[MAX_CHARS];
	char	out_val[MAX_CHARS_WIDE];
	char trad_val[23];


	/* No direction, recenter */
	if (!dir) {
#ifdef LOCATE_KEEPS_OVL
		/* Problem: verify_panel() does recenter, but not necessarily to the
		   same panel_row_old/panel_col_old, because sometimes a 1-panel-change
		   doesn't require snapping back!
		   However, even if not snapping back, if we changed the panel we do
		   need to shift our overlay panel accordingly, below.
		   So, remember the real old panel coords to know how far we need to shift. */
		int prow_old = p_ptr->panel_row_old;
		int pcol_old = p_ptr->panel_col_old;
#endif

		/* Recenter map around the player */
		verify_panel(Ind);

#ifdef LOCATE_KEEPS_OVL
		/* For LOCATE_KEEPS_OVL: adjust-shift existing overlay to new panel,
		   as it should still be partially lay within it. */
		if (prow_old != p_ptr->panel_row || pcol_old != p_ptr->panel_col) {
			int ovl_offset_x = (p_ptr->panel_col - pcol_old) * (p_ptr->screen_wid / 2);
			int ovl_offset_y = (p_ptr->panel_row - prow_old) * (p_ptr->screen_hgt / 2);
			int ox, oy;
			int sx, sy, ex, ey, ix, iy;

			if (ovl_offset_x > 0) {
				sx = 0;
				ex = MAX_WINDOW_WID;
				ix = 1;
			} else {
				sx = MAX_WINDOW_WID - 1;
				ex = -1;
				ix = -1;
			}
			if (ovl_offset_y > 0) {
				sy = 0;
				ey = MAX_WINDOW_HGT;
				iy = 1;
			} else {
				sy = MAX_WINDOW_HGT - 1;
				ey = -1;
				iy = -1;
			}

			for (x1 = sx; x1 != ex; x1 += ix)
				for (y1 = sy; y1 != ey; y1 += iy) {
					ox = x1 + ovl_offset_x;
					oy = y1 + ovl_offset_y;
					/* Verify that we're not exceeding our overlay buffer */
					if (ox >= 0 && oy >= 0 && ox < MAX_WINDOW_WID && oy < MAX_WINDOW_HGT) {
						p_ptr->ovl_info[y1][x1] = p_ptr->ovl_info[oy][ox];
						if (p_ptr->ovl_info[y1][x1].c)
							p_ptr->cave_flag[y1 + p_ptr->panel_row_prt][x1 + p_ptr->panel_col_prt] |= CAVE_AOVL;
					} else {
						/* Clear all the cropped parts */
						p_ptr->ovl_info[y1][x1].c = 0;
						p_ptr->ovl_info[y1][x1].a = 0;
						p_ptr->cave_flag[y1 + p_ptr->panel_row_prt][x1 + p_ptr->panel_col_prt] &= ~CAVE_AOVL;
					}
				}
		}
#endif

#ifdef LOCATE_KEEPS_OVL
		/* Undo: Was possibly set by verify_panel() if ESC'ing out of do_cmd_locate() before switching back to your home panel */
		p_ptr->redraw &= ~PR_MAP;
#endif

		/* Any change? otherwise no need to redraw */
		if ((prow != p_ptr->panel_row) || (pcol != p_ptr->panel_col)) {
			/* Update stuff */
			p_ptr->update |= (PU_MONSTERS);

			/* Redraw map */
#ifdef LOCATE_KEEPS_OVL
			p_ptr->redraw2 |= (PR2_MAP_SCR);
#else
			p_ptr->redraw |= (PR_MAP);
#endif

			/* Window stuff */
			p_ptr->window |= (PW_OVERHEAD);

			/* Handle stuff */
			handle_stuff(Ind);
		}

		//redundant, already done in verify_panel()?
#if defined(ALERT_OFFPANEL_DAM) || defined(LOCATE_KEEPS_OVL)
		/* For alert-beeps on damage: Reset remembered panel */
		p_ptr->panel_row_old = p_ptr->panel_row;
		p_ptr->panel_col_old = p_ptr->panel_col;
#endif

		return;
	}

	/* Initialize */
	if (dir == 5) {
		/* Remember current panel */
		p_ptr->panel_row_old = p_ptr->panel_row;
		p_ptr->panel_col_old = p_ptr->panel_col;
	}

	/* Start at current panel */
	y2 = p_ptr->panel_row;
	x2 = p_ptr->panel_col;

	/* Initial panel */
	y1 = p_ptr->panel_row_old;
	x1 = p_ptr->panel_col_old;

	/* Apply the motion */
	y2 += ddy[dir];
	x2 += ddx[dir];

	/* Verify the row */
	if (y2 > p_ptr->max_panel_rows) y2 = p_ptr->max_panel_rows;
	else if (y2 < 0) y2 = 0;

	/* Verify the col */
	if (x2 > p_ptr->max_panel_cols) x2 = p_ptr->max_panel_cols;
	else if (x2 < 0) x2 = 0;

	/* Describe the location */
	if ((y2 == y1) && (x2 == x1)) {
		tmp_val[0] = '\0';

		/* For BIG_MAP users, also display the traditional sector they are located in,
		   to make communication about dungeon entrances etc easier. */
		if (p_ptr->screen_wid == SCREEN_WID && p_ptr->screen_hgt == SCREEN_HGT)
			trad_val[0] = 0;
		else
			sprintf(trad_val, ", traditionally [%d,%d]", p_ptr->tradpanel_row, p_ptr->tradpanel_col);
	} else {
		sprintf(tmp_val, "%s%s of",
		        ((y2 < y1) ? " North" : (y2 > y1) ? " South" : ""),
		        ((x2 < x1) ? " West" : (x2 > x1) ? " East" : ""));
		trad_val[0] = 0;
	}

	/* Prepare to ask which way to look */
	sprintf(out_val,
	    "Map sector [%d,%d], which is%s your sector%s. Direction (or ESC)?",
	    y2, x2, tmp_val, trad_val);

	msg_print(Ind, out_val);

	/* Set the panel location */
	p_ptr->panel_row = y2;
	p_ptr->panel_col = x2;

	/* Recalculate the boundaries */
	panel_bounds(Ind);

	/* any change? otherwise no need to redraw everything */
	if ((prow != p_ptr->panel_row) || (pcol != p_ptr->panel_col)) {
		/* client-side weather stuff */
		p_ptr->panel_changed = TRUE;

		/* Redraw map */
#ifdef LOCATE_KEEPS_OVL
		p_ptr->redraw2 |= (PR2_MAP_SCR);
#else
		p_ptr->redraw |= (PR_MAP);
#endif

		/* Update stuff */
		p_ptr->update |= (PU_MONSTERS);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD);
	}

	/* Handle stuff */
	handle_stuff(Ind);
}



/* Not fully implemented - mikaelh */
#if 0

/*
 * The table of "symbol info" -- each entry is a string of the form
 * "X:desc" where "X" is the trigger, and "desc" is the "info".
 */
static cptr ident_info[] =
{
	" :A dark grid",
	"!:A potion (or oil)",
	"\":An amulet (or necklace)",
	"#:A wall (or secret door)",
	"$:Treasure (gold or gems)",
	"%:A vein (magma or quartz)",
	/* "&:unused", */
	"':An open door",
	"(:Soft armour",
	"):A shield",
	"*:A vein with treasure",
	"+:A closed door",
	",:Food (or mushroom patch)",
	"-:A wand (or rod)",
	".:Floor",
	"/:A polearm (Axe/Pike/etc)",
	/* "0:unused", */
	"1:Entrance to General Store",
	"2:Entrance to Armory",
	"3:Entrance to Weaponsmith",
	"4:Entrance to Temple",
	"5:Entrance to Alchemy shop",
	"6:Entrance to Magic store",
	"7:Entrance to Black Market",
	"8:Entrance to Tavern",
	/* "9:unused", */
	"::Rubble",
	";:A glyph of warding",
	"<:An up staircase",
	"=:A ring",
	">:A down staircase",
	"?:A scroll",
	"@:You",
	"A:Angel",
	"B:Bird",
	"C:Canine",
	"D:Ancient Dragon/Wyrm",
	"E:Elemental",
	"F:Dragon Fly",
	"G:Ghost",
	"H:Hybrid",
	"I:Insect",
	"J:Snake",
	"K:Killer Beetle",
	"L:Lich",
	"M:Multi-Headed Reptile",
	/* "N:unused", */
	"O:Ogre",
	"P:Giant Humanoid",
	"Q:Quylthulg (Pulsing Flesh Mound)",
	"R:Reptile/Amphibian",
	"S:Spider/Scorpion/Tick",
	"T:Troll",
	"U:Major Demon",
	"V:Vampire",
	"W:Wight/Wraith/etc",
	"X:Xorn/Xaren/etc",
	"Y:Yeti",
	"Z:Zephyr Hound",
	"[:Hard armour",
	"\\:A blunt weapon (mace/whip/etc)",
	"]:Misc. armour",
	"^:A trap",
	"_:A staff",
	/* "`:unused", */
	"a:Ant",
	"b:Bat",
	"c:Centipede",
	"d:Dragon",
	"e:Floating Eye",
	"f:Feline",
	"g:Golem",
	"h:Hobbit/Elf/Dwarf",
	"i:Icky Thing",
	"j:Jelly",
	"k:Kobold",
	"l:Louse",
	"m:Mold",
	"n:Naga",
	"o:Orc",
	"p:Person/Human",
	"q:Quadruped",
	"r:Rodent",
	"s:Skeleton",
	"t:Townsperson",
	"u:Minor Demon",
	"v:Vortex",
	"w:Worm/Worm-Mass",
	/* "x:unused", */
	"y:Yeek",
	"z:Zombie/Mummy",
	"{:A missile (arrow/bolt/shot)",
	"|:An edged weapon (sword/dagger/etc)",
	"}:A launcher (bow/crossbow/sling)",
	"~:A tool (or miscellaneous item)",
	NULL
};


/*
 * Identify a character
 *
 * Note that the player ghosts are ignored. XXX XXX XXX
 */
void do_cmd_query_symbol(int Ind, char sym)
{
	int		i;
	char	buf[128];


	/* If no symbol, abort --KLJ-- */
	if (!sym)
		return;

	/* Find that character info, and describe it */
	for (i = 0; ident_info[i]; ++i)
	{
		if (sym == ident_info[i][0]) break;
	}

	/* Describe */
	if (ident_info[i])
	{
		sprintf(buf, "%c - %s.", sym, ident_info[i] + 2);
	}
	else
	{
		sprintf(buf, "%c - %s.", sym, "Unknown Symbol");
	}

	/* Display the result */
	msg_print(Ind, buf);
}

#endif // 0
