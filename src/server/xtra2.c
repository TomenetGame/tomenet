/* $Id$ */
/* File: effects.c */

/* Purpose: effects of various "objects" */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"
#include "party.h"


//#define IRON_TEAM_EXPERIENCE
/* todo before enabling: implement way to decrease party.experience again! */


/*
 * What % of exp points will be lost when resurrecting? [40]	- Jir -
 *
 * cf. GHOST_FADING in dungeon.c
 */
#define GHOST_XP_LOST	40

/*
 * Chance of an item teleporting away when player dies, in percent. [10]
 * This is to balance death penalty after item stacking was implemented.
 * To disable, comment it out.
 */
#define DEATH_ITEM_SCATTER	0

/* Chance of an item from the player's inventory getting lost (aka deleted)
   when player dies, in percent [20]. - C. Blue (limited to 4) */
#define DEATH_PACK_ITEM_LOST	15

/* Chance of an item from the player's equipment getting lost (aka deleted)
   when player dies, in percent [10]. - C. Blue (limited to 1) */
#define DEATH_EQ_ITEM_LOST	10

/* Loot item level is average of monster level and floor level - C. Blue
   Note: Imho the more floor level is taken into account, the more will
         melee chars who aim at high level weapons and armour be at a
         disadvantage, compared to light armour dropping everywhere.
         Also, floor level determines monster level of spawns anyway.
         However, contra side is cheezy high unique farming on harmless
         floors via vaults - probably only affects melee chars though.
   Disabling this definition will cause loot to depend more on monster level. */
//#define TRADITIONAL_LOOT_LEVEL


/* Level 50 limit for non-kings:    RECOMMENDED!  */
#define KINGCAP_LEV

/* exp limit for non-kings (level 50..69 depending on race/class) for non-kings: */
/*#define KINGCAP_EXP*/		/*  NOT RECOMMENDED to enable this!  */

/* Does NOT have effect if KINGCAP_EXP is defined.
   Total bonus skill points till level 50 for yeeks.
   All other class/race combinations will get less, down to 0 for
   max exp penalty of 472,5 %. Must be integer within 0..50. [25] */
#define WEAK_SKILLBONUS 0	/* NOT RECOMMENDED to set this > 0! */


/* Do player-kill messages of "Morgoth, Lord of Darkness" get some
   special flavour? - C. Blue */
#define MORGOTH_FUNKY_KILL_MSGS

/*
 * Modifier of semi-promised artifact drops, in percent.
 * It can happen that the quickest player will gather most of those
 * artifact; this can be used to defuse it somewhat.
 * C. Blue: Better leave this commented out, otherwise _granted_
 * drops won't be granted anymore.
 */
// #define SEMI_PROMISED_ARTS_MODIFIER	50

/*
 * If defined, a player cannot gain more than 1 level at once.
 * It prevents so-called 'high-books cheeze'.
 */
/* Prolly no longer needed, since a player cannot gain exp from books now */
//#define LEVEL_GAINING_LIMIT


/*
 * Thresholds for scrolling.	[3,8] [2,4]
 * XXX They should be client-side numerical options.	- Jir -
 */
#ifndef ARCADE_SERVER
	#define	SCROLL_MARGIN_ROW	(p_ptr->wide_scroll_margin ? 5 : 2) /* 5:2 */
	#define	SCROLL_MARGIN_COL	(p_ptr->wide_scroll_margin ? 12 : 4) /* 16:4 */
#else
	#define SCROLL_MARGIN_ROW 8
	#define SCROLL_MARGIN_COL 20
#endif

/* Pre-set owner for DROP_CHOSEN items, so you can't cheeze them to someone else
   by ground-IDing them, then have someone else to pick them up! (especially for Nazgul rings) */
#define PRE_OWN_DROP_CHOSEN

/* death_type definitions */
#define DEATH_PERMA	0
#define DEATH_INSANITY	1
#define DEATH_GHOST	2
#define DEATH_QUIT	3 /* suicide/retirement */

/* If during certain events, remember his/her account ID, for handing out a reward
   to a different character which he chooses on next login! - C. Blue
*/
static void buffer_account_for_event_deed(player_type *p_ptr, int death_type)
{
	int i,j;
#if 0 /* why should this be enabled, hmm */
	for (i = 0; i < 128; i++)
		if (ge_contender_buffer_ID[i] == p_ptr->account) return; /* player already has a buffer entry */
#endif
	for (i = 0; i < 128; i++)
		if (ge_contender_buffer_ID[i] == 0) break;
	if (i == 128) return; /* no free buffer entries anymore, sorry! */
	ge_contender_buffer_ID[i] = p_ptr->account;
	for (j = 0; j < MAX_GLOBAL_EVENTS; j++)
		switch (p_ptr->global_event_type[j]) {
		case GE_HIGHLANDER:
			if (p_ptr->global_event_progress[j][0] < 5) break; /* only rewarded if already in deathmatch phase! */
			if (death_type >= DEATH_QUIT) break; /* no reward for suiciding! */
			/* hand out the reward: */
			ge_contender_buffer_deed[i] = SV_DEED2_HIGHLANDER;
			return;
		case GE_NONE:
		default:
			break;
		}
	ge_contender_buffer_ID[i] = 0; /* didn't find any event where player participated */
}

static void buffer_account_for_achievement_deed(player_type *p_ptr, int achievement)
{
	int i;
#if 0 /* why should this be enabled, hmm */
	for (i = 0; i < 128; i++)
		if (achievement_buffer_ID[i] == p_ptr->account) return; /* player already has a buffer entry */
#endif
	for (i = 0; i < 128; i++)
		if (achievement_buffer_ID[i] == 0) break;
	if (i == 128) return; /* no free buffer entries anymore, sorry! */
	achievement_buffer_ID[i] = p_ptr->account;
	switch (achievement) {
	case ACHV_PVP_MAX:
		achievement_buffer_deed[i] = SV_DEED_PVP_MAX;
		break;
	case ACHV_PVP_MID:
		achievement_buffer_deed[i] = SV_DEED_PVP_MID;
		break;
	case ACHV_PVP_MASS:
		achievement_buffer_deed[i] = SV_DEED_PVP_MASS;
		break;
	default:
		break;
	}
}

/*
 * Set "p_ptr->tim_thunder", notice observable changes
 */
bool set_tim_thunder(int Ind, int v, int p1, int p2)
{
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_thunder) {
			msg_print(Ind, "The air around you charges with lightning!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_thunder) {
			msg_print(Ind, "The air around you discharges.");
			notice = TRUE;
			p1 = p2 = 0;
		}
	}

	/* Use the value */
	p_ptr->tim_thunder = v;
	p_ptr->tim_thunder_p1 = p1;
	p_ptr->tim_thunder_p2 = p2;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->tim_regen", notice observable changes
 */
bool set_tim_regen(int Ind, int v, int p)
{
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_regen) {
			msg_print(Ind, "Your body regeneration abilities greatly increase!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_regen) {
			p = 0;
			msg_print(Ind, "Your body regeneration abilities becomes normal again.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_regen = v;
	p_ptr->tim_regen_pow = p;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->tim_trauma", notice observable changes
 */
bool set_tim_trauma(int Ind, int v, int p)
{
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;
	
	/* Open */
	if (v) {
		if (!p_ptr->tim_trauma) {
			msg_print(Ind, "You feel an unnatural desire for death and destruction.");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_trauma) {
			msg_print(Ind, "Your blood-lust fades.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_trauma = v;
	p_ptr->tim_trauma_pow = p;
	
	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->tim_ffall"
 */
bool set_tim_ffall(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_ffall) {
			msg_print(Ind, "You feel very light.");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_ffall) {
			msg_print(Ind, "You are suddenly heavier.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_ffall = v;

	/* Nothing to notice */
	if (!notice)
		return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state)
		disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->tim_fly"
 */
bool set_tim_fly(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_fly) {
			msg_print(Ind, "You feel able to reach the clouds.");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_fly) {
			msg_print(Ind, "You are suddenly a lot heavier.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_fly = v;

	/* Nothing to notice */
	if (!notice)
		return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state)
		disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->adrenaline", notice observable changes
 * Note the interaction with biofeedback
 */
bool set_adrenaline(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE, sudden = FALSE, crash = FALSE;

        int i;
	
	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->adrenaline) {
			msg_print(Ind, "Adrenaline surges through your veins!");
			if (p_ptr->biofeedback) {
				msg_print(Ind, "You lose control of your blood flow!");
				i = randint(randint(v));
				take_hit(Ind, damroll(2, i),"adrenaline poisoning", 0);
				v = v - i + 1;
				p_ptr->biofeedback = 0;
			}
			
			notice = TRUE;
		} else {
			/* Sudden crash */
			if (!rand_int(500) && (p_ptr->adrenaline >= v)) {
				msg_print(Ind, "Your adrenaline suddenly runs out!");
				v = 0;
				sudden = TRUE;
				if (!rand_int(2)) crash = TRUE;
			}
		}
		
		while (v > 30 + randint(p_ptr->lev * 5)) {
			msg_print(Ind, "Your body can't handle that much adrenaline!");
			i = randint(randint(v));
			take_hit(Ind, damroll(3, i * 2),"adrenaline poisoning", 0);
			v = v - i + 1;
		}		
	}

	/* Shut */
	else {
		if (p_ptr->adrenaline) {
			if (!rand_int(3)) {
				crash = TRUE;
				msg_print(Ind, "Your adrenaline runs out, leaving you tired and weak.");
			} else {
				msg_print(Ind, "Your heart slows down to normal.");
			}
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->adrenaline = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Notice */
	p_ptr->update |= (PU_BONUS | PU_HP);
	
	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
	
}
/*
 * Set "p_ptr->biofeedback", notice observable changes
 * Note the interaction with adrenaline
 */
bool set_biofeedback(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;
	
	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->biofeedback) {
			msg_print(Ind, "Your pulse slows and your body prepares to resist damage.");
			if (p_ptr->adrenaline) {
				msg_print(Ind, "The adrenaline drains out of your veins.");
				p_ptr->adrenaline = 0;
				if (!rand_int(8)) {
					msg_print(Ind, "You start to tremble as your blood sugar crashes.");
					set_slow(Ind, p_ptr->slow + rand_int(rand_int(16)));
					if (!rand_int(5)) set_paralyzed(Ind, p_ptr->paralyzed + 1);
					if (!rand_int(3)) set_stun(Ind, p_ptr->stun + rand_int(30));
				}
			}
			notice = TRUE;
		}		
	}

	/* Shut */
	else {
		if (p_ptr->biofeedback) {
			msg_print(Ind, "Your veins return to normal.");
			notice = TRUE;
		}
	}
				
	while (v > 35 + rand_int(rand_int(p_ptr->lev))) {
			msg_print(Ind, "You speed up your pulse to avoid fainting!");
			v -= 20;
	}

	/* Use the value */
	p_ptr->biofeedback = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Notice */
	p_ptr->update |= (PU_BONUS | PU_HP);
	
	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
	
}


/*
 * Set "p_ptr->tim_esp", notice observable changes
 */
bool set_tim_esp(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_esp) {
			msg_print(Ind, "Your mind expands !");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_esp) {
			msg_print(Ind, "Your mind retracts.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_esp = v;
	
	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS | PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->st_anchor", notice observable changes
 */
bool set_st_anchor(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->st_anchor) {
			msg_print(Ind, "The Space/Time Continuum seems to solidify !");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->st_anchor) {
			msg_print(Ind, "The Space/Time Continuum seems more flexible.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->st_anchor = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->prob_travel", notice observable changes
 */
bool set_prob_travel(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->prob_travel) {
			msg_print(Ind, "You feel instable !");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->prob_travel) {
			msg_print(Ind, "You feel more stable.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->prob_travel = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->brand", notice observable changes
 * Experimentally applied feature for runemastery. - Kurzel
 */
bool set_brand(int Ind, int v, int t, int p)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;
	
	char weapons[20], dual[2];
	
	//strcpy(weapons, "\377oYour weapon");
	strcpy(weapons, "\377wYour weapon");
	strcpy(dual, "s");
	if (p_ptr->inventory[INVEN_WIELD].k_idx &&
	    (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) {
		//strcpy(weapons, "\377oYour weapons");
		strcpy(weapons, "\377wYour weapons");
		strcpy(dual, "");
	}

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->brand &&
		    (p_ptr->inventory[INVEN_WIELD].k_idx || /* dual-wield..*/
		    (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)))
		{
		  switch (t) {
		    case BRAND_ELEC:
                    case BRAND_BALL_ELEC:
		      //msg_format(Ind, "%s sparkle%s with lightning!", weapons, dual);
		      msg_format(Ind, "%s glow%s with electricity!", weapons, dual);
		      break;
                    case BRAND_BALL_COLD:
		    case BRAND_COLD:
		      //msg_format(Ind, "%s freeze%s!", weapons, dual);
		      msg_format(Ind, "%s glow%s with cold!", weapons, dual);
		      break;
                    case BRAND_BALL_FIRE:
		    case BRAND_FIRE:
		      //msg_format(Ind, "%s burn%s!", weapons, dual);
		      msg_format(Ind, "%s glow%s with fire!", weapons, dual);
		      break;
                    case BRAND_BALL_ACID:
		    case BRAND_ACID:
		      //msg_format(Ind, "%s look%s acidic!", weapons, dual);
		      msg_format(Ind, "%s glow%s with acid!", weapons, dual);
		      break;
		    case BRAND_POIS:
		      //msg_format(Ind, "%s drip%s with venom!", weapons, dual);
		      msg_format(Ind, "%s glow%s with poison!", weapons, dual);
		      break;
		    case BRAND_MANA:
		      msg_format(Ind, "% glow%s with power!", weapons, dual);
		      break;
		    case BRAND_CONF:
		      msg_format(Ind, "%s glow%s many colors!", weapons, dual);
		      break;
		    case BRAND_SHARP:
		      //msg_format(Ind, "%s sharpen%s!", weapons, dual);
		      msg_format(Ind, "%s take%s on a vorpal glow!", weapons, dual);
		      break;
                    case BRAND_BALL_SOUND:
                      msg_format(Ind, "%s vibrate%s!", weapons, dual);
		      break;
		    }
		  notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->brand && p_ptr->inventory[INVEN_WIELD].k_idx) {
			//msg_print(Ind, "\377oYour weapon seems normal again.");
			msg_print(Ind, "\377sYour weapon seems normal again.");
			notice = TRUE;
			t = 0;
			p = 0;
		}
	}

	/* Use the value */
	p_ptr->brand = v;
	p_ptr->brand_t = t;
	p_ptr->brand_d = p;
	

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS | PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->bow_brand_xxx", notice observable changes
 */
bool set_bow_brand(int Ind, int v, int t, int p)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->bow_brand) {
		  switch (t) {
		    case BRAND_ELEC:
                    case BRAND_BALL_ELEC:
		      msg_print(Ind, "\377oYour ammo sparkles with lightnings !");
		      break;
                    case BRAND_BALL_COLD:
		    case BRAND_COLD:
		      msg_print(Ind, "\377oYour ammo freezes !");
		      break;
                    case BRAND_BALL_FIRE:
		    case BRAND_FIRE:
		      msg_print(Ind, "\377oYour ammo burns !");
		      break;
                    case BRAND_BALL_ACID:
		    case BRAND_ACID:
		      msg_print(Ind, "\377oYour ammo looks acidic !");
		      break;
		    case BRAND_POIS:
		      msg_print(Ind, "\377oYour ammo is covered with venom !");
		      break;
		    case BRAND_MANA:
		      msg_print(Ind, "\377oYour ammo glows with power !");
		      break;
		    case BRAND_CONF:
		      msg_print(Ind, "\377oYour ammo glows many colors !");
		      break;
		    case BRAND_SHARP:
		      msg_print(Ind, "\377oYour ammo sharpens !");
		      break;
                    case BRAND_BALL_SOUND:
                      msg_print(Ind, "\377oYour ammo vibrates !");
		      break;
		    }
		  notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->bow_brand) {
			msg_print(Ind, "\377oYour ammo seems normal again.");
			notice = TRUE;
			t = 0;
			p = 0;
		}
	}

	/* Use the value */
	p_ptr->bow_brand = v;
	p_ptr->bow_brand_t = t;
	p_ptr->bow_brand_d = p;
	

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS | PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_mimic", notice observable changes
 */
bool set_mimic(int Ind, int v, int p)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* force good values */
	if (v < 0) v = 0;

	/* Open */
	if (v) {
		if (!p_ptr->tim_mimic) {
			msg_print(Ind, "Your image changes!");
			notice = TRUE;
		} else if (p_ptr->tim_mimic > 100 && v <= 100) {
			msg_print(Ind, "\376\377LThe magical force stabilizing your form starts to fade...");
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_mimic) {
			msg_print(Ind, "\376\377LYour image changes back to normality.");
			do_mimic_change(Ind, 0, TRUE);
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_mimic = v;

#if 0 /* once you might have mimicked other classes, now we will use it for polymorph rings! */	
	/* Enforce good values */
	if (p < 0) p = 0;
	if (p >= MAX_CLASS) p = MAX_CLASS - 1;
	p_ptr->tim_mimic_what = p;
#endif

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS | PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->tim_manashield", notice observable changes
 */
bool set_tim_manashield(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_manashield) {
			msg_print(Ind, "\376\377vA purple shimmering shield forms around your body!");
			notice = TRUE;
		} else if (p_ptr->tim_manashield > 20 && v <= 20) {
			msg_print(Ind, "\376\377vThe disruption shield starts to flicker and fade...");
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_manashield) {
			msg_print(Ind, "\376\377vThe disruption shield fades away.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_manashield = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA);

	/* update so everyone sees the colour animation */
	everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->tim_traps", notice observable changes
 */
bool set_tim_traps(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_traps) {
			msg_print(Ind, "You can avoid all the traps !");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_traps) {
			msg_print(Ind, "You should worry about traps again.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_traps = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->tim_invis", notice observable changes
 */
bool set_invis(int Ind, int v, int p)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_invisibility) {
			msg_format_near(Ind, "%s fades in the shadows!", p_ptr->name);
			msg_print(Ind, "You fade in the shadow!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_invisibility) {
			msg_format_near(Ind, "The shadows enveloping %s disipate.", p_ptr->name);
			msg_print(Ind, "The shadows enveloping you disipate.");
			notice = TRUE;
		}
		p = 0;
	}

	/* Use the value */
	p_ptr->tim_invisibility = v;
	p_ptr->tim_invis_power = p;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS | PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->fury", notice observable changes
 */
bool set_fury(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->fury) {
			msg_print(Ind, "You grow a fury!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->fury) {
			msg_print(Ind, "The fury stops.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->fury = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni + hit points */
	p_ptr->update |= (PU_BONUS | PU_HP);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_meditation", notice observable changes
 */
bool set_tim_meditation(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_meditation) {
			msg_format_near(Ind, "%s starts a calm meditation!", p_ptr->name);
			msg_print(Ind, "You start a calm meditation!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_meditation) {
			msg_format_near(Ind, "%s stops meditating.", p_ptr->name);
			msg_print(Ind, "You stop your meditation.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_meditation = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_HP | PU_MANA);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->tim_wraith", notice observable changes
 */
bool set_tim_wraith(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;
	cave_type **zcave;
	dun_level *l_ptr = getfloor(&p_ptr->wpos);
	if(!(zcave=getcave(&p_ptr->wpos))) return FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_wraith) {
			if ((zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) ||
			    (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC)))
			{
				msg_print(Ind, "You feel different for a moment");
				v = 0;
			} else {
				msg_format_near(Ind, "%s turns into a wraith!", p_ptr->name);
				msg_print(Ind, "You turn into a wraith!");
				notice = TRUE;
			
				p_ptr->wraith_in_wall = TRUE;
			}
		}
#if 0	// I can't remember what was it for..
		// but for sure it's wrong
//it was probably for the old hack to prevent wraithing in/around town and breaking into houses that way - C. Blue
		else if(!p_ptr->wpos.wz && cave_floor_bold(zcave, p_ptr->py, p_ptr->px))
			return(FALSE);
#endif	// 0
	}

	/* Shut */
	else {
		if (p_ptr->tim_wraith) {
			/* In town it only runs out if you are not on a wall
			 * To prevent breaking into houses */
			/* important! check for illegal spaces */
			cave_type **zcave;
			zcave=getcave(&p_ptr->wpos);

			if (zcave && in_bounds(p_ptr->py, p_ptr->px)) {
				/* if a worn item grants wraith form, don't let it run out */
				u32b f1, f2, f3, f4, f5, esp;
				object_type *o_ptr;
				int i;
				/* Scan the usable inventory */
				for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
					o_ptr = &p_ptr->inventory[i];
					/* Skip missing items */
					if (!o_ptr->k_idx) continue;
					/* Extract the item flags */
					object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
					if (f3 & (TR3_WRAITH)) {
					        //p_ptr->wraith_form = TRUE;
					        v = 30000;
					}
				}
				if (v != 30000) {
					msg_format_near(Ind, "%s loses %s wraith powers.", p_ptr->name, p_ptr->male ? "his":"her");
					msg_print(Ind, "You lose your wraith powers.");
					notice = TRUE;

					/* That will hopefully prevent game hinging when loading */
					if (cave_floor_bold(zcave, p_ptr->py, p_ptr->px)) p_ptr->wraith_in_wall = FALSE;
				}
			}
			else v = 1;
		}
	}

	/* Use the value */
	p_ptr->tim_wraith = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->blind", notice observable changes
 *
 * Note the use of "PU_UN_LITE" and "PU_UN_VIEW", which is needed to
 * memorize any terrain features which suddenly become "visible".
 * Note that blindness is currently the only thing which can affect
 * "player_can_see_bold()".
 */
bool set_blind(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	if (p_ptr->martyr && v) return FALSE;

	/* the admin wizard can not be blinded */
	if (p_ptr->admin_wiz) return 1;

/* instead put into dungeon.c for faster recovery
	if (get_skill(p_ptr, SKILL_HCURING) >= 30) v /= 2;
*/
	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->blind) {
			msg_format_near(Ind, "%s gropes around blindly!", p_ptr->name);
			msg_print(Ind, "You are blind!");
			notice = TRUE;
		}
		
		break_shadow_running(Ind);
	}

	/* Shut */
	else {
		if (p_ptr->blind) {
			msg_format_near(Ind, "%s can see again.", p_ptr->name);
			msg_print(Ind, "You can see again.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->blind = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Forget stuff */
	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_BONUS);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Redraw the "blind" */
	p_ptr->redraw |= (PR_BLIND);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->confused", notice observable changes
 */
bool set_confused(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	if (p_ptr->martyr && v) return FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	if (get_skill(p_ptr, SKILL_MIND) >= 30) v /= 2;

	/* Open */
	if (v) {
		if (!p_ptr->confused) {
			msg_format_near(Ind, "%s appears confused!", p_ptr->name);
			msg_print(Ind, "You are confused!");
			notice = TRUE;
		}

		break_cloaking(Ind, 0);
		break_shadow_running(Ind);
		stop_precision(Ind);
		stop_shooting_till_kill(Ind);
	}

	/* Shut */
	else {
		if (p_ptr->confused) {
			msg_format_near(Ind, "%s appears less confused now.", p_ptr->name);
			msg_print(Ind, "You feel less confused now.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->confused = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Redraw the "confused" */
	p_ptr->redraw |= (PR_CONFUSED);

	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

#ifdef ARCADE_SERVER
void set_pushed(int Ind, int dir) {
	player_type *p_ptr = Players[Ind];
	p_ptr->pushed = 20;
	p_ptr->pushdir = dir;
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);
	handle_stuff(Ind);
	return;
}
#endif

/*
 * Set "p_ptr->poisoned", notice observable changes
 */
bool set_poisoned(int Ind, int v, int attacker)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	if (p_ptr->martyr && v) return FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

/* instead put into dungeon.c for faster recovery
	if (get_skill(p_ptr, SKILL_HCURING) >= 30) v /= 2;
*/
	/* Open */
	if (v) {
		if (!p_ptr->poisoned) {
			msg_print(Ind, "You are poisoned!");
			notice = TRUE;

			/* Remember who did it - mikaelh */
			p_ptr->poisoned_attacker = attacker;
		}
	}

	/* Shut */
	else {
		if (p_ptr->poisoned) {
			msg_print(Ind, "You are no longer poisoned.");
			notice = TRUE;

			/* Forget who did it - mikaelh */
			p_ptr->poisoned_attacker = 0;
		}
	}

	/* Use the value */
	p_ptr->poisoned = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Redraw the "poisoned" */
	p_ptr->redraw |= (PR_POISONED);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->afraid", notice observable changes
 */
bool set_afraid(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	if (p_ptr->martyr && v) return FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->afraid) {
			msg_format_near(Ind, "%s cowers in fear!", p_ptr->name);
			msg_print(Ind, "You are terrified!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->afraid) {
			msg_format_near(Ind, "%s appears bolder now.", p_ptr->name);
			msg_print(Ind, "You feel bolder now.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->afraid = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Redraw the "afraid" */
	p_ptr->redraw |= (PR_AFRAID);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->paralyzed", notice observable changes
 */
bool set_paralyzed(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	if (p_ptr->martyr && v) return FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->paralyzed) {
			msg_format_near(Ind, "%s becomes rigid!", p_ptr->name);
			msg_print(Ind, "You are paralyzed!");
			notice = TRUE;
			s_printf("%s EFFECT: Paralyzed %s.\n", showtime(), p_ptr->name);
		}
	}

	/* Shut */
	else {
		if (p_ptr->paralyzed) {
			msg_format_near(Ind, "%s can move again.", p_ptr->name);
			msg_print(Ind, "You can move again.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->paralyzed = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Redraw the state */
	p_ptr->redraw |= (PR_STATE);

	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->image", notice observable changes
 *
 * Note that we must redraw the map when hallucination changes.
 */
bool set_image(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

/* instead put into dungeon.c for faster recovery
	if (get_skill(p_ptr, SKILL_MIND) >= 30) v /= 2;
	if (get_skill(p_ptr, SKILL_HCURING) >= 50) v /= 2;
*/
	/* Open */
	if (v) {
		if (!p_ptr->image) {
			msg_format_near(Ind, "%s has been drugged.", p_ptr->name);
			msg_print(Ind, "You feel drugged!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->image) {
			msg_format_near(Ind, "%s has recovered from %s drug induced stupor.", p_ptr->name, p_ptr->male ? "his" : "her");
			msg_print(Ind, "You can see clearly again.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->image = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Update monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->fast", notice observable changes
 */
bool set_fast(int Ind, int v, int p)
{
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	if (p_ptr->mode & MODE_PVP) return(FALSE);

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->fast) {
			if (p > 0) {
				msg_format_near(Ind, "%s begins moving faster!", p_ptr->name);
				msg_print(Ind, "You feel yourself moving faster!");
			} else {
				msg_format_near(Ind, "%s moves slower!", p_ptr->name);
				msg_print(Ind, "You feel yourself moving slower!");
			}
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->fast) {
			if (p > 0) {
				msg_format_near(Ind, "%s slows down.", p_ptr->name);
				msg_print(Ind, "You feel yourself slow down.");
			} else {
				msg_format_near(Ind, "%s moves faster.", p_ptr->name);
				msg_print(Ind, "You feel yourself moving faster");
			}
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->fast = v;
	p_ptr->fast_mod = p;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->slow", notice observable changes
 */
bool set_slow(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	if (p_ptr->martyr && v) return FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->slow) {
			msg_format_near(Ind, "%s begins moving slower!", p_ptr->name);
			msg_print(Ind, "You feel yourself moving slower!");
			notice = TRUE;
			s_printf("%s EFFECT: Slowed %s.\n", showtime(), p_ptr->name);
		}
	}

	/* Shut */
	else {
		if (p_ptr->slow) {
			msg_format_near(Ind, "%s speeds up.", p_ptr->name);
			msg_print(Ind, "You feel yourself speed up.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->slow = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->shield", notice observable changes
 */
bool set_shield(int Ind, int v, int p, s16b o, s16b d1, s16b d2)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->shield) {
			msg_print(Ind, "A mystic shield forms around your body!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->shield) {
			msg_print(Ind, "Your mystic shield crumbles away.");
			notice = TRUE;
		}
	}


	/* Use the value */
	p_ptr->shield = v;
	if (p_ptr->shield_power != p) notice = TRUE; /* notice +AC changes on recasting (runecraft!) */
	p_ptr->shield_power = p;
	p_ptr->shield_opt = o;
	p_ptr->shield_power_opt = d1;
	p_ptr->shield_power_opt2 = d2;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

#ifdef ENABLE_RCRAFT
/*
 * Set "p_ptr->tim_deflect", notice observable changes
 */
bool set_tim_deflect(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_deflect) {
			msg_print(Ind, "A deflective shield forms around your body!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_deflect) {
			msg_print(Ind, "Your deflective shield crumbles away.");
			notice = TRUE;
		}
	}


	/* Use the value */
	p_ptr->tim_deflect = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}
#endif


/*
 * Set "p_ptr->blessed", notice observable changes
 */
bool set_blessed(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->blessed) {
			msg_format_near(Ind, "%s has become righteous.", p_ptr->name);
			msg_print(Ind, "You feel righteous!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->blessed) {
			msg_format_near(Ind, "%s has become less righteous.", p_ptr->name);
			msg_print(Ind, "The prayer has expired.");
			notice = TRUE;
			p_ptr->blessed_power = 0;
		}
	}

	/* Use the value */
	p_ptr->blessed = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

bool set_res_fear(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->res_fear_temp) {
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->res_fear_temp) {
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->res_fear_temp = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->hero", notice observable changes
 */
bool set_hero(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->hero) {
			msg_format_near(Ind, "%s has become a hero.", p_ptr->name);
			msg_print(Ind, "You feel like a hero!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->hero) {
			msg_format_near(Ind, "%s has become less of a hero.", p_ptr->name);
			msg_print(Ind, "The heroism wears off.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->hero = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni & hit points */
	p_ptr->update |= (PU_BONUS | PU_HP);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->shero", notice observable changes
 */
bool set_shero(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->shero) {
			msg_format_near(Ind, "%s has become a killing machine.", p_ptr->name);
			msg_print(Ind, "You feel like a killing machine!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->shero) {
			msg_format_near(Ind, "%s has returned to being a wimp.", p_ptr->name);
			msg_print(Ind, "You feel less Berserk.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->shero = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni + hit points */
	p_ptr->update |= (PU_BONUS | PU_HP);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


bool set_berserk(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->berserk) {
			msg_format_near(Ind, "%s has become a killing machine.", p_ptr->name);
			msg_print(Ind, "You feel like a killing machine!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->berserk) {
			msg_format_near(Ind, "%s has returned to being a wimp.", p_ptr->name);
			msg_print(Ind, "You feel less Berserk.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->berserk = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni + hit points */
	p_ptr->update |= (PU_BONUS | PU_HP);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


bool set_melee_sprint(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->melee_sprint) {
			msg_format_near(Ind, "%s starts sprinting.", p_ptr->name);
			msg_print(Ind, "You start sprinting!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->melee_sprint) {
			msg_format_near(Ind, "%s slows down.", p_ptr->name);
			msg_print(Ind, "You slow down.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->melee_sprint = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Recalculate hitpoints */
	p_ptr->update |= (PU_HP);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->protevil", notice observable changes
 */
bool set_protevil(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->protevil) {
			msg_print(Ind, "You feel safe from evil!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->protevil) {
			msg_print(Ind, "You no longer feel safe from evil.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->protevil = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->zeal", notice observable changes
 */
bool set_zeal(int Ind, int p, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* set EA */
	p_ptr->zeal_power = p;

	/* Open */
	if (v) {
		if (!p_ptr->zeal) {
			msg_print(Ind, "You heed a holy call!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->zeal) {
			msg_print(Ind, "The holy call fades.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->zeal = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Redraw the Blows/Round */	
	p_ptr->update |= PU_BONUS;

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

bool set_martyr(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack: Negative v means initiate martyr */
	if (v < 0) {
		v = -v;
		p_ptr->martyr_dur = v;
	}

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->martyr) {
			msg_print(Ind, "\377vYou feel the heavens grant your their powers.");
			hp_player_quiet(Ind, 5000, FALSE); /* fully heal */
			p_ptr->martyr_timeout = 1000;
			notice = TRUE;
			s_printf("MARTYRDOM: %s\n", p_ptr->name);
		} else {
			msg_print(Ind, "\377wYou burn in holy fire!");
			/* assumes that martyr starts at -15 turns! : */
			p_ptr->chp = (p_ptr->mhp * p_ptr->martyr) / p_ptr->martyr_dur; 
			/* Update health bars */
			update_health(0 - Ind);
			/* Redraw */
			p_ptr->redraw |= (PR_HP);

		    	/* update so everyone sees the colour animation */
			everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
		}
	}

	/* Shut */
	else {
		if (p_ptr->martyr) {
			/* Increased remaining HP (from 1), to buffer environmental influences
			   like nether/fire hit from the ground in the Nether Realm;
			   assumes that martyr starts at -15 turns! : */
//			p_ptr->chp = (p_ptr->mhp >= 30 * 15) ? 30 : p_ptr->mhp / 15;
			p_ptr->chp = (p_ptr->mhp <= 30 * p_ptr->martyr_dur) ? 30 : p_ptr->mhp / p_ptr->martyr_dur;
    		        /* Update health bars */
	                update_health(0 - Ind);
	                /* Redraw */
	                p_ptr->redraw |= (PR_HP);

			msg_print(Ind, "\377vYou collapse as your martyrium ends!");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->martyr = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

    	/* update so everyone sees the colour animation */
	everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->invuln", notice observable changes
 */
bool set_invuln(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->invuln) {
			msg_print(Ind, "\377vA powerful iridescent shield forms around your body!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->invuln) {
			/* Keeps the 2 turn GOI from getting annoying. DEG */
			if (get_skill(p_ptr, SKILL_MAGERY) > 39) {
				msg_print(Ind, "\377vThe invulnerability shield fades away.");
			}
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->invuln = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* update so everyone sees the colour animation */
	everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->invuln", but not notice observable changes
 * It should be used to protect players from recall-instadeath.  - Jir -
 * Note: known also as 'stair-GoI' (globe of invulnerability),
 * supposed to shortly protect players when they enter a new level. -C. Blue
 */
bool set_invuln_short(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	/* not cumulative */
	if (p_ptr->invuln > v) return (FALSE);

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Use the value */
	p_ptr->invuln = v;

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result (never noticeable) */
	return (FALSE);
}

/*
 * Set "p_ptr->tim_invis", notice observable changes
 */
bool set_tim_invis(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_invis) {
			msg_print(Ind, "Your eyes feel very sensitive!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_invis) {
			msg_print(Ind, "Your eyes feel less sensitive.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_invis = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_infra", notice observable changes
 */
bool set_tim_infra(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_infra) {
			msg_print(Ind, "Your eyes begin to tingle!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_infra) {
			msg_print(Ind, "Your eyes stop tingling.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_infra = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_acid", notice observable changes
 */
bool set_oppose_acid(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->oppose_acid) {
			msg_print(Ind, "You feel resistant to acid!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->oppose_acid) {
			msg_print(Ind, "\377WYou feel less resistant to \377sacid.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_acid = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_elec", notice observable changes
 */
bool set_oppose_elec(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->oppose_elec) {
			msg_print(Ind, "You feel resistant to electricity!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->oppose_elec) {
			msg_print(Ind, "\377WYou feel less resistant to \377belectricity.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_elec = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_fire", notice observable changes
 */
bool set_oppose_fire(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->oppose_fire) {
			msg_print(Ind, "You feel resistant to fire!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->oppose_fire) {
			msg_print(Ind, "\377WYou feel less resistant to \377rfire.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_fire = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_cold", notice observable changes
 */
bool set_oppose_cold(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->oppose_cold) {
			msg_print(Ind, "You feel resistant to cold!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->oppose_cold) {
			msg_print(Ind, "\377WYou feel less resistant to \377wcold.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_cold = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_pois", notice observable changes
 */
bool set_oppose_pois(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->oppose_pois) {
			msg_print(Ind, "You feel resistant to poison!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->oppose_pois) {
			msg_print(Ind, "\377WYou feel less resistant to \377gpoison.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_pois = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->stun", notice observable changes
 *
 * Note the special code to only notice "range" changes.
 */
bool set_stun(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	int old_aux, new_aux;

	bool notice = FALSE;

	if (p_ptr->martyr && v) return FALSE;

	/* hack -- the admin wizard can not be stunned */
//	if (p_ptr->admin_wiz) return TRUE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

#if 0 /* decided to make it just as combat skill working in dungeon.c to accelerate recovery! */
	/* apply HCURING in a special way: halve the duration of highest stun level that actually gets reached */
	if (get_skill(p_ptr, SKILL_HCURING) >= 40) v /= 2;
#endif

	/* Knocked out */
	if (p_ptr->stun > 100) old_aux = 3;
	/* Heavy stun */
	else if (p_ptr->stun > 50) old_aux = 2;
	/* Stun */
	else if (p_ptr->stun > 0) old_aux = 1;
	/* None */
	else old_aux = 0;

	/* Knocked out */
	if (v > 100) new_aux = 3;
	/* Heavy stun */
	else if (v > 50) new_aux = 2;
	/* Stun */
	else if (v > 0) new_aux = 1;
	/* None */
	else new_aux = 0;

	/* Increase stun */
	if (new_aux > old_aux) {
		/* Describe the state */
		switch (new_aux) {
			/* Stun */
			case 1:
			msg_format_near(Ind, "\377o%s appears stunned.", p_ptr->name);
			msg_print(Ind, "\377oYou have been stunned.");
			break;

			/* Heavy stun */
			case 2:
			msg_format_near(Ind, "\377o%s is very stunned.", p_ptr->name);
			msg_print(Ind, "\377oYou have been heavily stunned.");
			break;

			/* Knocked out */
			case 3:
			msg_format_near(Ind, "%s has been knocked out.", p_ptr->name);
			msg_print(Ind, "You have been knocked out.");
			s_printf("%s EFFECT: Knockedout %s.\n", showtime(), p_ptr->name);
			break;
		}
		
		break_shadow_running(Ind);

		/* Notice */
		notice = TRUE;
	}

	/* Decrease cut */
	else if (new_aux < old_aux) {
		/* Describe the state */
		switch (new_aux) {
			/* None */
			case 0:
			msg_format_near(Ind, "\377o%s is no longer stunned.", p_ptr->name);
			msg_print(Ind, "\377yYou are no longer stunned.");
			if (p_ptr->disturb_state) disturb(Ind, 0, 0);
			break;
		}

		/* Notice */
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->stun = v;

	/* No change */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Redraw the "stun" */
	p_ptr->redraw |= (PR_STUN);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->cut", notice observable changes
 *
 * Note the special code to only notice "range" changes.
 */
bool set_cut(int Ind, int v, int attacker)
{
	player_type *p_ptr = Players[Ind];

	int old_aux, new_aux;

	bool notice = FALSE;

	if (p_ptr->martyr && v) return FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

/* instead put into dungeon.c for faster recovery
	if (get_skill(p_ptr, SKILL_HCURING) >= 40) v /= 2;
*/

	/* p_ptr->no_cut? for mimic forms that cannot bleed */
	if (p_ptr->no_cut) v = 0;

	/* a ghost never bleeds */
	if (v && p_ptr->ghost) v = 0;

	/* Mortal wound */
	if (p_ptr->cut > 1000) old_aux = 7;
	/* Deep gash */
	else if (p_ptr->cut > 200) old_aux = 6;
	/* Severe cut */
	else if (p_ptr->cut > 100) old_aux = 5;
	/* Nasty cut */
	else if (p_ptr->cut > 50) old_aux = 4;
	/* Bad cut */
	else if (p_ptr->cut > 25) old_aux = 3;
	/* Light cut */
	else if (p_ptr->cut > 10) old_aux = 2;
	/* Graze */
	else if (p_ptr->cut > 0) old_aux = 1;
	/* None */
	else old_aux = 0;

	/* Mortal wound */
	if (v > 1000) new_aux = 7;
	/* Deep gash */
	else if (v > 200) new_aux = 6;
	/* Severe cut */
	else if (v > 100) new_aux = 5;
	/* Nasty cut */
	else if (v > 50) new_aux = 4;
	/* Bad cut */
	else if (v > 25) new_aux = 3;
	/* Light cut */
	else if (v > 10) new_aux = 2;
	/* Graze */
	else if (v > 0) new_aux = 1;
	/* None */
	else new_aux = 0;

	/* Increase cut */
	if (new_aux > old_aux) {
		/* Describe the state */
		switch (new_aux) {
			/* Graze */
			case 1:
			msg_print(Ind, "You have been given a graze.");
			break;

			/* Light cut */
			case 2:
			msg_print(Ind, "You have been given a light cut.");
			break;

			/* Bad cut */
			case 3:
			msg_print(Ind, "You have been given a bad cut.");
			break;

			/* Nasty cut */
			case 4:
			msg_print(Ind, "You have been given a nasty cut.");
			break;

			/* Severe cut */
			case 5:
			msg_print(Ind, "You have been given a severe cut.");
			break;

			/* Deep gash */
			case 6:
			msg_print(Ind, "You have been given a deep gash.");
			break;

			/* Mortal wound */
			case 7:
			msg_print(Ind, "You have been given a mortal wound.");
			break;
		}

		/* Notice */
		notice = TRUE;
	}

	/* Decrease cut */
	else if (new_aux < old_aux) {
		/* Describe the state */
		switch (new_aux) {
			/* None */
			case 0:
			msg_print(Ind, "You are no longer bleeding.");
			if (p_ptr->disturb_state) disturb(Ind, 0, 0);
			p_ptr->cut_attacker = 0;
			break;
		}

		/* Notice */
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->cut = v;

	/* Remember who did it - mikaelh */
	p_ptr->cut_attacker = attacker;

	/* No change */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Redraw the "cut" */
	p_ptr->redraw |= (PR_CUT);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

bool set_mindboost(int Ind, int p, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* set EA */
	p_ptr->mindboost_power = p;

	/* Open */
	if (v) {
		if (!p_ptr->mindboost) {
			msg_print(Ind, "Your mind overflows!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->mindboost) {
			msg_print(Ind, "The mind returns to normal.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->mindboost = v;

	if (set_afraid(Ind, 0)) notice = TRUE;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Redraw the Blows/Round if any */
	p_ptr->update |= PU_BONUS;

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

bool set_kinetic_shield(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->kinetic_shield) {
			msg_print(Ind, "You create a kinetic barrier.");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->kinetic_shield) {
			msg_print(Ind, "Your kinetic barrier destabilizes.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->kinetic_shield = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

#ifdef ENABLE_DIVINE
/* timed hp bonus for RACE_DIVINE.
   *fastest* path (SKILL_ASTRAL = lvl+2):
   +1 at lvl 39
   +2 at lvl 54
   +3 at lvl 60
*/
bool do_divine_hp(int Ind, int v, int p) {
        player_type *p_ptr = Players[Ind];
        bool notice = (FALSE);

        /* Hack -- Force good values */
        v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

        /* Open */
        if (v) {
                if (!p_ptr->divine_hp) {
                        msg_format_near(Ind, "%s prepares for aggression!", p_ptr->name);
                        msg_print(Ind, "You feel courageous.");
			p_ptr->divine_hp_mod = p;

                        notice = (TRUE);
                }
        }

        /* Shut */
        else { //v = 0;
                if (p_ptr->divine_hp) {
			p_ptr->divine_hp_mod = 0;
                        msg_format_near(Ind, "%s returns to %s normal self.", p_ptr->name, (p_ptr->male? "his" : "her"));
                        msg_print(Ind, "You no longer feel courageous.");
                        notice = (TRUE);
                }
        }

        p_ptr->divine_hp = v;

        /* Nothing to notice */
        if (!notice) return (FALSE);

        /* Disturb */
        if (p_ptr->disturb_state) disturb(Ind, 0, 0);

        /* Recalculate boni */
        p_ptr->update |= (PU_BONUS|PU_HP);
        /* Handle stuff */
        handle_stuff(Ind);

        /* Result */
        return (TRUE);
}

/* 
   timed crit bonus for RACE_DIVINE. 
   *fastest* path (SKILL_ASTRAL = lvl+2):
   +2 at lvl 44, +2 per 5 levels thereafter
*/
bool do_divine_crit(int Ind, int v, int p) {
        player_type *p_ptr = Players[Ind];
        bool notice = (FALSE);

        /* Hack -- Force good values */
        v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

        /* Open */
        if (v) {
                if (!p_ptr->divine_crit) {
                        msg_format_near(Ind, "%s seems focussed.", p_ptr->name);
                        msg_print(Ind, "You focus your intentions.");
			p_ptr->divine_crit_mod = p;

                        notice = (TRUE);
                }
        }

        /* Shut */
        else { //v = 0;
                if (p_ptr->divine_crit) {
			p_ptr->divine_crit_mod = 0;
                        msg_format_near(Ind, "%s seems less focussed", p_ptr->name);
                        msg_print(Ind, "Your focus dissipates.");
                        notice = (TRUE);
                }
        }

        p_ptr->divine_crit = v;

        /* Nothing to notice */
        if (!notice) return (FALSE);

        /* Disturb */
        if (p_ptr->disturb_state) disturb(Ind, 0, 0);

        /* Recalculate boni */
        p_ptr->update |= (PU_BONUS);
        /* Handle stuff */
        handle_stuff(Ind);

        /* Result */
        return (TRUE);
}

/* 
   timed time and mana res bonus for RACE_DIVINE. 
*/
bool do_divine_xtra_res_time_mana(int Ind, int v) {
        player_type *p_ptr = Players[Ind];
        bool notice = (FALSE);

        /* Hack -- Force good values */
        v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

        /* Open */
        if (v) {
                if (!p_ptr->divine_xtra_res_time_mana) {
			msg_print(Ind, "You feel resistant to \377Btime\377w.");
			msg_print(Ind, "You feel resistant to \377vmana\377w.");
                        notice = (TRUE);
                }
        }

        /* Shut */
        else { //v = 0;
                if (p_ptr->divine_xtra_res_time_mana) {
			msg_print(Ind, "You feel less resistant to \377Btime\377w.");
			msg_print(Ind, "You feel less resistant to \377vmana\377w.");
                        notice = (TRUE);
                }
        }

        p_ptr->divine_xtra_res_time_mana = v;

        /* Nothing to notice */
        if (!notice) return (FALSE);

        /* Disturb */
        if (p_ptr->disturb_state) disturb(Ind, 0, 0);

        /* Recalculate boni */
        p_ptr->update |= (PU_BONUS);

        /* Handle stuff */
        handle_stuff(Ind);

        /* Result */
        return (TRUE);
}
#endif

#ifdef ENABLE_RCRAFT
bool do_life_bonus(int Ind, int v, int p) {
        player_type *p_ptr = Players[Ind];
	return do_life_bonus_aux(Ind, v, p + p_ptr->temporary_to_l);
}
/* 
 *  temporary LIFE modifier. p can be negative.
 */
bool do_life_bonus_aux(int Ind, int v, int p) {
        player_type *p_ptr = Players[Ind];
        bool notice = (FALSE);

        /* Hack -- Force good values */
        v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

        /* Open */
        if (v) {
		p_ptr->temporary_to_l = p;
		notice = (TRUE);
        }

        /* Shut */
        else { //v = 0;
		p_ptr->temporary_to_l = 0;
		notice = (TRUE);
        }

        p_ptr->temporary_to_l_dur = v;

        /* Nothing to notice */
        if (!notice) return (FALSE);

        /* Disturb */
        if (p_ptr->disturb_state) disturb(Ind, 0, 0);

        /* Recalculate boni */
        p_ptr->update |= (PU_HP);

        /* Handle stuff */
        handle_stuff(Ind);

        /* Result */
        return (TRUE);
}

bool do_speed_bonus(int Ind, int v, int p) {
        player_type *p_ptr = Players[Ind];
	return do_speed_bonus_aux(Ind, v, p+p_ptr->temporary_speed);
}
/* 
 *  temporary SPEED modifier. p can be negative.
 */
bool do_speed_bonus_aux(int Ind, int v, int p) {
        player_type *p_ptr = Players[Ind];
        bool notice = (FALSE);

        /* Hack -- Force good values */
        v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

        /* Open */
        if (v) {
		p_ptr->temporary_speed = p;
		notice = (TRUE);
        }

        /* Shut */
        else { //v = 0;
		p_ptr->temporary_speed = 0;
		notice = (TRUE);
        }

        p_ptr->temporary_speed_dur = v;

        /* Nothing to notice */
        if (!notice) return (FALSE);

        /* Disturb */
        if (p_ptr->disturb_state) disturb(Ind, 0, 0);

        /* Recalculate boni */
        p_ptr->update |= (PU_BONUS);

        /* Handle stuff */
        handle_stuff(Ind);

        /* Result */
        return (TRUE);
}

/* 
 *  temporary AM shell!
 */
bool do_temporary_antimagic(int Ind, int v) {
        player_type *p_ptr = Players[Ind];
        bool notice = (FALSE);

        /* Hack -- Force good values */
        v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

        /* Open */
        if (v) {
                if (!p_ptr->temporary_am) {
			msg_print(Ind, "You are enveloped in an antimagic shell.");
                        notice = (TRUE);
                }
        }

        /* Shut */
        else { //v = 0;
                if (p_ptr->temporary_am) {
			msg_print(Ind, "Your antimagic shell disappears.");
                        notice = (TRUE);
                }
        }

        p_ptr->temporary_am = v;

        /* Nothing to notice */
        if (!notice) return (FALSE);

        /* Disturb */
        if (p_ptr->disturb_state) disturb(Ind, 0, 0);

        /* Recalculate boni */
        p_ptr->update |= (PU_BONUS);

        /* Handle stuff */
        handle_stuff(Ind);

        /* Result */
        return (TRUE);
}

#endif //ENABLE_RCRAFT
/*
 * Set "p_ptr->sh_fire/cold/elec", notice observable changes
 */
bool set_sh_fire_tim(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->sh_fire_tim) {
			if (!p_ptr->sh_fire_fix) {
				msg_print(Ind, "You are enveloped by scorching flames!");
				notice = TRUE;
			} else msg_print(Ind, "The scorching flames surrounding you are blazing high!");
		}
	}
	/* Shut */
	else {
		if (p_ptr->sh_fire_tim && !p_ptr->sh_fire_fix) {
			msg_print(Ind, "The scorching flames surrounding you cease.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->sh_fire_tim = v;
	/* Nothing to notice */
	if (!notice) return (FALSE);
	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);
	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);
	/* Handle stuff */
	handle_stuff(Ind);
	/* Result */
	return (TRUE);
}
bool set_sh_cold_tim(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->sh_cold_tim) {
			if (!p_ptr->sh_cold_fix) {
				msg_print(Ind, "You are enveloped by freezing cold!");
				notice = TRUE;
			} else msg_print(Ind, "The frost around you blows colder even!");
		}
	}
	/* Shut */
	else {
		if (p_ptr->sh_cold_tim && !p_ptr->sh_cold_fix) {
			msg_print(Ind, "The scorching flames surrounding you cease.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->sh_cold_tim = v;
	/* Nothing to notice */
	if (!notice) return (FALSE);
	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);
	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);
	/* Handle stuff */
	handle_stuff(Ind);
	/* Result */
	return (TRUE);
}
bool set_sh_elec_tim(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->sh_elec_tim) {
			if (!p_ptr->sh_elec_fix) {
				msg_print(Ind, "You are enveloped by sparkling static!");
				notice = TRUE;
			} else msg_print(Ind, "Static around you surges brightly!");
		}
	}
	/* Shut */
	else {
		if (p_ptr->sh_elec_tim && !p_ptr->sh_elec_fix) {
			msg_print(Ind, "The static sparkling around you ceases.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->sh_elec_tim = v;
	/* Nothing to notice */
	if (!notice) return (FALSE);
	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);
	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);
	/* Handle stuff */
	handle_stuff(Ind);
	/* Result */
	return (TRUE);
}



/*
 * Set "p_ptr->food", notice observable changes
 *
 * The "p_ptr->food" variable can get as large as 20000, allowing the
 * addition of the most "filling" item, Elvish Waybread, which adds
 * 7500 food units, without overflowing the 32767 maximum limit.
 *
 * Perhaps we should disturb the player with various messages,
 * especially messages about hunger status changes.  XXX XXX XXX
 *
 * Digestion of food is handled in "dungeon.c", in which, normally,
 * the player digests about 20 food units per 100 game turns, more
 * when "fast", more when "regenerating", less with "slow digestion",
 * but when the player is "gorged", he digests 100 food units per 10
 * game turns, or a full 1000 food units per 100 game turns.
 *
 * Note that the player's speed is reduced by 10 units while gorged,
 * so if the player eats a single food ration (5000 food units) when
 * full (15000 food units), he will be gorged for (5000/100)*10 = 500
 * game turns, or 500/(100/5) = 25 player turns (if nothing else is
 * affecting the player speed).
 */
bool set_food(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	int old_aux, new_aux;

	bool notice = FALSE;

	/* True Ghosts don't starve */
	if ((p_ptr->ghost) || (get_skill(p_ptr, SKILL_HSUPPORT) >= 40) ||
	    (p_ptr->prace == RACE_DIVINE && p_ptr->ptrait))
	{
	    p_ptr->food = PY_FOOD_FULL - 1;
	    return (FALSE);
	}
	/* Warrior does not need food badly */
#ifdef ARCADE_SERVER
	p_ptr->food = PY_FOOD_FULL - 1;
	return (FALSE);
#endif

	/* Hack -- Force good values */
	v = (v > 20000) ? 20000 : (v < 0) ? 0 : v;

	/* Fainting / Starving */
	if (p_ptr->food < PY_FOOD_FAINT) old_aux = 0;
	/* Weak */
	else if (p_ptr->food < PY_FOOD_WEAK) old_aux = 1;
	/* Hungry */
	else if (p_ptr->food < PY_FOOD_ALERT) old_aux = 2;
	/* Normal */
	else if (p_ptr->food < PY_FOOD_FULL) old_aux = 3;
	/* Full */
	else if (p_ptr->food < PY_FOOD_MAX) old_aux = 4;
	/* Gorged */
	else old_aux = 5;

	/* Fainting / Starving */
	if (v < PY_FOOD_FAINT) new_aux = 0;
	/* Weak */
	else if (v < PY_FOOD_WEAK) new_aux = 1;
	/* Hungry */
	else if (v < PY_FOOD_ALERT) new_aux = 2;
	/* Normal */
	else if (v < PY_FOOD_FULL) new_aux = 3;
	/* Full */
	else if (v < PY_FOOD_MAX) new_aux = 4;
	/* Gorged */
	else new_aux = 5;

	/* Food increase */
	if (new_aux > old_aux) {
		/* Describe the state */
		switch (new_aux) {
			/* Weak */
			case 1:
			msg_print(Ind, "You are still weak.");
			break;

			/* Hungry */
			case 2:
			msg_print(Ind, "You are still hungry.");
			break;

			/* Normal */
			case 3:
			msg_print(Ind, "You are no longer hungry.");
			break;

			/* Full */
			case 4:
			msg_print(Ind, "You are full!");
			break;

			/* Bloated */
			case 5:
			msg_print(Ind, "You have gorged yourself!");
			break;
		}

		/* Change */
		notice = TRUE;
	}

	/* Food decrease */
	else if (new_aux < old_aux) {
		/* Describe the state */
		switch (new_aux) {
			/* Fainting / Starving */
			case 0:
			msg_print(Ind, "\377RYou are getting faint from hunger!");
			break;

			/* Weak */
			case 1:
			msg_print(Ind, "You are getting weak from hunger!");
			if (p_ptr->warning_hungry <= 1 && p_ptr->max_plv <= 20) {
				p_ptr->warning_hungry = 2;
				if (p_ptr->prace == RACE_VAMPIRE) {
					msg_print(Ind, "\374\377RWARNING: You are 'weak' from hunger. Drink some blood by killing monsters");
					msg_print(Ind, "\374\377R         monsters in melee (close combat). Town monsters will work too.");
				} else if (p_ptr->prace == RACE_ENT) {
					msg_print(Ind, "\374\377RWARNING: You are 'weak' from hunger. Read a scroll of satisfy hunger or");
					msg_print(Ind, "\374\377R         rest (SHIFT+R) on earth/dirt/grass/water floor tiles for a while.");
				} else {
					msg_print(Ind, "\374\377RWARNING: You are 'weak' from hunger. Press SHIFT+E to eat something");
					msg_print(Ind, "\374\377R         or read a 'scroll of satisfy hunger' if you have one.");
				}
				s_printf("warning_hungry(weak): %s\n", p_ptr->name);
			}
			break;

			/* Hungry */
			case 2:
			msg_print(Ind, "You are getting hungry.");
			if (p_ptr->warning_hungry == 0 && p_ptr->max_plv <= 15) {
				p_ptr->warning_hungry = 1;
				if (p_ptr->prace == RACE_VAMPIRE) {
					msg_print(Ind, "\374\377oWARNING: Your character is 'hungry'. Drink some blood by killing some");
					msg_print(Ind, "\374\377o         monsters in melee (close combat). Town monsters will work too.");
				} else if (p_ptr->prace == RACE_ENT) {
					msg_print(Ind, "\374\377oWARNING: Your character is 'hungry'. Read a scroll of satisfy hunger or");
					msg_print(Ind, "\374\377o         rest (SHIFT+R) on earth/dirt/grass/water floor tiles for a while.");
				} else {
					msg_print(Ind, "\374\377oWARNING: Your character is 'hungry'. Press SHIFT+E to eat something");
					msg_print(Ind, "\374\377o         or read a 'scroll of satisfy hunger' if you have one.");
				}
				s_printf("warning_hungry(hungry): %s\n", p_ptr->name);
			}
			break;

			/* Normal */
			case 3:
			msg_print(Ind, "You are no longer full.");
			break;

			/* Full */
			case 4:
			msg_print(Ind, "You are no longer gorged.");
			break;
		}

		/* Change */
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->food = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Redraw hunger */
	p_ptr->redraw |= (PR_HUNGER);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->bless_temp_luck" - note: currently bless_temp_... aren't saved/loaded! (ie expire on logout)
 */
bool bless_temp_luck(int Ind, int pow, int dur)
{
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	if (!p_ptr->bless_temp_luck) {
		msg_print(Ind, "You feel luck is on your side.");
		notice = TRUE;
	}
	if (!dur) {
		msg_print(Ind, "Your lucky streak fades.");
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->bless_temp_luck = dur;
	p_ptr->bless_temp_luck_power = pow;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate boni */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

#ifdef ENABLE_DIVINE
/* helper function to modify Maia skills when they get a trait - C. Blue
   NOTE: IF THIS IS CALLED TOO MANY TIMES IN A ROW IT
         _MIGHT_ DISCONNET THE CLIENT WITH A 'write error'. */
static void do_Maia_skill(int Ind, int s, int m) {
	/* Save old skill value */
	s32b val = Players[Ind]->s_info[s].value, tmp_val = Players[Ind]->s_info[s].mod;

	/* Release invested points */
	respec_skill(Ind, s, FALSE, FALSE);

	/* Modify skill, avoiding overflow (mod is u16b) */
	tmp_val = (tmp_val * m) / 10;
	/* Cap to 2.0 */
	if (tmp_val > 2000) tmp_val = 2000;
	Players[Ind]->s_info[s].mod = tmp_val;

	/* Reinvest some of the points until old skill value is reached again */
	tmp_val = -1;
#if 0
	while (((Players[Ind]->s_info[s].value / 1000 < val / 1000 && /* smooth: avoid "overskilling" */
	    (Players[Ind]->s_info[s].value + Players[Ind]->s_info[s].mod) / 1000 <= val / 1000) ||
	    Players[Ind]->s_info[s].value + Players[Ind]->s_info[s].mod <= val) /* no "underskilling" */
#else
	while (Players[Ind]->s_info[s].value + Players[Ind]->s_info[s].mod <= val /* no "overskilling" */
#endif
	    /* avoid game maximum overflow - paranoia: */
	    && Players[Ind]->s_info[s].value < 50000
	    /* avoid cap overflow - extreme cases only (maxed auras): */
	    && Players[Ind]->s_info[s].value / 1000 < Players[Ind]->max_plv + 2
	    /* avoid getting stuck - paranoia (could waste 1 point actually if it really happened): */
	    && Players[Ind]->s_info[s].value != tmp_val) {
		/* make sure we don't loop forever in case we can't go higher */
		tmp_val = Players[Ind]->s_info[s].value;
		/* invest a point */
		increase_skill(Ind, s, TRUE);
	}

	/* Update it after the re-increasing has been finished */
	Send_skill_info(Ind, s, FALSE);
}
/* Change Maia skill chart after initiation */
void shape_Maia_skills(int Ind) {
	player_type *p_ptr = Players[Ind];

	switch (p_ptr->ptrait) {
	case TRAIT_CORRUPTED:
		/* Doh! */
		p_ptr->s_info[SKILL_HOFFENSE].mod = 0;
		p_ptr->s_info[SKILL_HCURING].mod = 0;
		p_ptr->s_info[SKILL_HDEFENSE].mod = 0;
		p_ptr->s_info[SKILL_HSUPPORT].mod = 0;

		/* Yay */
		do_Maia_skill(Ind, SKILL_AXE, 13);
		do_Maia_skill(Ind, SKILL_MARTIAL_ARTS, 13);
		do_Maia_skill(Ind, SKILL_FIRE, 17);
		do_Maia_skill(Ind, SKILL_AIR, 17);
		do_Maia_skill(Ind, SKILL_CONVEYANCE, 17);
		do_Maia_skill(Ind, SKILL_UDUN, 20);
		do_Maia_skill(Ind, SKILL_TRAUMATURGY, 30);
		do_Maia_skill(Ind, SKILL_NECROMANCY, 30);
		do_Maia_skill(Ind, SKILL_AURA_FEAR, 30);
		do_Maia_skill(Ind, SKILL_AURA_SHIVER, 30);
		do_Maia_skill(Ind, SKILL_AURA_DEATH, 30);
		break;

	case TRAIT_ENLIGHTENED:
		/* Doh! */
		p_ptr->s_info[SKILL_TRAUMATURGY].mod = 0;
		p_ptr->s_info[SKILL_NECROMANCY].mod = 0;
		p_ptr->s_info[SKILL_AURA_DEATH].mod = 0;

		/* Yay */
		do_Maia_skill(Ind, SKILL_AURA_FEAR, 30);
		do_Maia_skill(Ind, SKILL_AURA_SHIVER, 30);
		do_Maia_skill(Ind, SKILL_HOFFENSE, 24);
		do_Maia_skill(Ind, SKILL_HCURING, 24);
		do_Maia_skill(Ind, SKILL_HDEFENSE, 24);
		do_Maia_skill(Ind, SKILL_HSUPPORT, 24);
		do_Maia_skill(Ind, SKILL_DIVINATION, 17);
		do_Maia_skill(Ind, SKILL_SWORD, 13);
		do_Maia_skill(Ind, SKILL_BLUNT, 13);
		do_Maia_skill(Ind, SKILL_POLEARM, 13);
		break;
	default: ;
	}
}
#else /* shared player.pkg file */
void shape_Maia_skills(int Ind) { }
#endif

/*
 * Try to raise stats, esp. if low.		- Jir -
 */
static void check_training(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int train = get_skill_scale(p_ptr, SKILL_TRAINING, 50);
	int i, chance, value, value2;

	if (train < 1) return;

	for (i = 0; i < 6; i++) {
		value = p_ptr->stat_cur[i];
		value2 = p_ptr->stat_ind[i];
		chance = train;

		value += (p_ptr->rp_ptr->r_adj[i]);
		value += (p_ptr->cp_ptr->c_adj[i]);

		if (value > 12) chance /= 2;
		if (value > 17) chance /= 4;

		/* Hack -- High stats, low chance */
		if (magik(adj_str_hold[value2]) || !magik(chance)) continue;

		/* Hack -- if restored, not increase */
		if (!res_stat(Ind, i)) do_inc_stat(Ind, i);
	}

	/* Also, it can give an extra skill point */
	if (magik(train)) p_ptr->skill_points++;
}

/*
 * Advance experience levels and print experience
 */
void check_experience(int Ind)
{
	player_type *p_ptr = Players[Ind];

	bool newlv = FALSE;
	int old_lev;
//	long int i;
#ifdef LEVEL_GAINING_LIMIT
	int limit;
#endif	// LEVEL_GAINING_LIMIT

	/* paranoia -- fix the max level first */
	if (p_ptr->lev > p_ptr->max_plv)
		p_ptr->max_plv = p_ptr->lev;

	/* Note current level */
	old_lev = p_ptr->lev;

	/* Hack -- lower limit */
	if (p_ptr->exp < 0) p_ptr->exp = 0;

	/* Hack -- lower limit */
	if (p_ptr->max_exp < 0) p_ptr->max_exp = 0;

#ifdef LEVEL_GAINING_LIMIT
	/* upper limit */
 #ifndef ALT_EXPRATIO
	limit = (s64b)((s64b)player_exp[p_ptr->max_plv] *
			(s64b)p_ptr->expfact / 100L) - 1;
 #else
	limit = (s64b)(player_exp[p_ptr->max_plv] - 1);
 #endif
	/* Hack -- upper limit */
	if (p_ptr->exp > limit) p_ptr->exp = limit;
#endif	// LEVEL_GAINING_LIMIT

	/* Hack -- upper limit */
	if (p_ptr->exp > PY_MAX_EXP) p_ptr->exp = PY_MAX_EXP;

	/* Hack -- upper limit */
	if (p_ptr->max_exp > PY_MAX_EXP) p_ptr->max_exp = PY_MAX_EXP;


	/* Hack -- maintain "max" experience */
	if (p_ptr->exp > p_ptr->max_exp) p_ptr->max_exp = p_ptr->exp;

	/* Redraw experience */
	p_ptr->redraw |= (PR_EXP);


	/* Lose levels while possible */
#ifndef ALT_EXPRATIO
	while ((p_ptr->lev > 1) &&
               (p_ptr->exp < ((s64b)((s64b)player_exp[p_ptr->lev-2] * (s64b)p_ptr->expfact / 100L))))
#else
	while ((p_ptr->lev > 1) &&
               (p_ptr->exp < (s64b)player_exp[p_ptr->lev-2]))
#endif
	{
		/* Lose a level */
		p_ptr->lev--;

		clockin(Ind, 1);        /* Set player level */

	}


	/* Remember maximum level (the one displayed if life levels were restored right now) */
#ifndef ALT_EXPRATIO
	while ((p_ptr->max_lev > 1) &&
               (p_ptr->max_exp < ((s64b)((s64b)player_exp[p_ptr->max_lev-2] * (s64b)p_ptr->expfact / 100L))))
#else
	while ((p_ptr->max_lev > 1) &&
               (p_ptr->max_exp < (s64b)player_exp[p_ptr->max_lev-2]))
#endif
	{
		/* Lose a level */
		p_ptr->max_lev--;
	}


	/* Gain levels while possible */
#ifndef ALT_EXPRATIO
	while ((p_ptr->lev < (is_admin(p_ptr) ? PY_MAX_LEVEL : PY_MAX_PLAYER_LEVEL)) &&
			(p_ptr->exp >= ((s64b)(((s64b)player_exp[p_ptr->lev-1] * (s64b)p_ptr->expfact) / 100L))))
#else
	while ((p_ptr->lev < (is_admin(p_ptr) ? PY_MAX_LEVEL : PY_MAX_PLAYER_LEVEL)) &&
			(p_ptr->exp >= (s64b)player_exp[p_ptr->lev-1]))
#endif
	{
		if(p_ptr->inval && p_ptr->lev >= 25) {
			msg_print(Ind, "\377rYou cannot gain level further, wait for an admin to validate your account.");
			break;
//			return;
		}

		process_hooks(HOOK_PLAYER_LEVEL, "d", Ind);

		/* Gain a level */
		p_ptr->lev++;

		clockin(Ind, 1);        /* Set player level */

		/* Save the highest level */
		if (p_ptr->lev > p_ptr->max_plv) {
			p_ptr->max_plv = p_ptr->lev;

			/* gain skill points */
#ifdef KINGCAP_EXP
			/* min cap level is 50. check how far this
			character can come with the actual exp cap of
			21240000 (TL Ranger level 50) and adjust skill
			distribution so that characters gain 250..300
			skill points in total (TLRanger..YeekWarrior)*/
			for (i = 50; i < 69; i++) {
				if ((((s64b)player_exp[i-1] * (s64b)p_ptr->expfact) / 100L) > 21240000) break;
			}
			i--;/* i now contains the maximum reachable level for
			    this character, due to exp cap 21240000 */
			/* now calculate his skills/level ratio.
			It's (250+2.63*extraLevel) / personalCapLevel */
			i = i - 50;/* i now contains the extraLevel above 50 */
			i = ((250000 + (2630 * i)) / (50 + i));
			if (i != ((i / 1000) * 1000)) i += 10;/* +1 against rounding probs */
			i /= 10; /* i is now 500..4xx, containing the skills/levelup * 100 */
			/* Give player all of his skill points except the last one: */
			p_ptr->skill_points += SKILL_NB_BASE - 1;
			/* Eventually give him the last one, depending on his skills/levelup ratio: */
			if (rand_int(100) < (i - ((SKILL_NB_BASE - 1) * 100))) p_ptr->skill_points++;
#endif
#ifndef KINGCAP_EXP
			p_ptr->skill_points += SKILL_NB_BASE;
			if (WEAK_SKILLBONUS) {
				/* calculate total bonus skill points for this
				   player at level 50, save it in xsp: */
				long xsp_mul = WEAK_SKILLBONUS * 1000000 / ((573000 / (100 + 35)) - 1000);
				long xsp = ((573000 / (100 + p_ptr->expfact)) - 1) * xsp_mul;
				xsp /= 1000000;
				/* depending on his should-have-bonus give him an extra point randomly */
				if (rand_int(100) < (xsp * 100 / 50)) p_ptr->skill_points++;
			}
#endif
                        p_ptr->redraw |= PR_STUDY;
			p_ptr->update |= PU_SKILL_MOD;

			newlv = TRUE;

			/* give some stats, using Training skill */
			check_training(Ind);
		} else {
			/* Player just regained a level he lost previously */
			s_printf("%s has regained level %d.\n", p_ptr->name, p_ptr->lev);
		}

		/* Make a new copy of the skills - mikaelh */
		if (newlv) {
			memcpy(p_ptr->s_info_old, p_ptr->s_info, MAX_SKILLS * sizeof(skill_player));
			p_ptr->skill_points_old = p_ptr->skill_points;
		}

		/* Reskilling is now possible */
		p_ptr->reskill_possible = TRUE;

#ifdef USE_SOUND_2010
		/* see further below! */
#else
		sound(Ind, SOUND_LEVEL);
#endif
	}


	/* Remember maximum level (the one displayed if life levels were restored right now) */
#ifndef ALT_EXPRATIO
	while ((p_ptr->max_lev < (is_admin(p_ptr) ? PY_MAX_LEVEL : PY_MAX_PLAYER_LEVEL)) &&
			(p_ptr->max_exp >= ((s64b)(((s64b)player_exp[p_ptr->max_lev-1] * (s64b)p_ptr->expfact) / 100L))))
#else
	while ((p_ptr->max_lev < (is_admin(p_ptr) ? PY_MAX_LEVEL : PY_MAX_PLAYER_LEVEL)) &&
			(p_ptr->max_exp >= (s64b)player_exp[p_ptr->max_lev-1]))
#endif
	{
		/* Gain a level */
		p_ptr->max_lev++;
	}

	/* Redraw level-depending stuff.. */
	if (old_lev != p_ptr->lev) {
		/* Update some stuff */
		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SANITY);

		/* Redraw some stuff */
		p_ptr->redraw |= (PR_LEV | PR_TITLE | PR_DEPTH | PR_STATE);
			/* PR_STATE is only needed if player can unlearn 
			    techniques by dropping in levels */

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Window stuff - Items might be come (un)usable depending on level! */
	        p_ptr->window |= (PW_INVEN | PW_EQUIP);
	}

	/* Handle stuff */
	handle_stuff(Ind);

	if (newlv) {
		char str[160];
		/* Message */
		msg_format(Ind, "\374\377GWelcome to level %d. You have %d skill points.", p_ptr->lev, p_ptr->skill_points);
#ifdef USE_SOUND_2010
		sound(Ind, "levelup", NULL, SFX_TYPE_MISC, FALSE);
#endif

		/* Give helpful msg about how to distribute skill points at first level-up */
		if (p_ptr->newbie_hints && (old_lev == 1 || (p_ptr->skill_points == (p_ptr->max_plv - 1) * 5))) {
		    // && p_ptr->inval) /* (p_ptr->warning_skills) */
			msg_print(Ind, "\374\377GHINT: Press \377gSHIFT + g\377G to distribute your skill points!");
		}

		/* Remind how to send chat messages */
		if (old_lev < 3 && p_ptr->lev >= 3 && p_ptr->warning_chat == 0) {
			p_ptr->warning_chat = 1;
			msg_print(Ind, "\374\377oHINT: You can press '\377R:\377o' key to chat with other players, eg greet them!");
			s_printf("warning_chat: %s\n", p_ptr->name);
		}

		/* Give warning message to use word-of-recall, aimed at newbies */
		if (old_lev < 8 && p_ptr->lev >= 8 && p_ptr->warning_wor == 0) {
			/* scan inventory for any potions */
			bool found_items = FALSE;
			int i;
			for (i = 0; i < INVEN_PACK; i++) {
				if (!p_ptr->inventory[i].k_idx) continue;
				if (!object_known_p(Ind, &p_ptr->inventory[i])) continue;
				if (p_ptr->inventory[i].tval == TV_SCROLL &&
				    p_ptr->inventory[i].sval == SV_SCROLL_WORD_OF_RECALL)
					found_items = TRUE;
			}
			if (!found_items) {
				msg_print(Ind, "\374\377yHINT: You can use scrolls of \377Rword-of-recall\377y to teleport out of a dungeon");
				msg_print(Ind, "\374\377y      or back into it, making the tedious search for stairs obsolete!");
				s_printf("warning_wor: %s\n", p_ptr->name);
			}
			p_ptr->warning_wor = 1;
		}

		/* Give warning message to get involved with macros, aimed at newbies */
		if (old_lev < 10 && p_ptr->lev >= 10) { /* (p_ptr->warning_macros) */
			/* scan inventory for any macroish inscriptions */
			bool found_macroishness = FALSE;
			int i;
			for (i = 0; i < INVEN_PACK; i++)
				if (p_ptr->inventory[i].k_idx &&
				    p_ptr->inventory[i].note &&
				    strstr(quark_str(p_ptr->inventory[i].note), "@"))
					found_macroishness = TRUE;
			/* give a warning if it seems as if this character doesn't use any macros ;) */
			if (!found_macroishness) {
				msg_print(Ind, "\374\377oHINT: Start getting the hang of '\377Rmacros\377o' ('%' key) in order to ensure");
				msg_print(Ind, "\374\377o      survival in critical combat situations. \377RPress '%' and 'z'\377o to start the");
				msg_print(Ind, "\374\377o      macro wizard and/or read the guide (text file 'TomeNET-Guide.txt').");
				s_printf("warning_macros: %s\n", p_ptr->name);
			}
		}

		/* Give warning message to stock wound curing potions, aimed at newbies */
		if (old_lev < 12 && p_ptr->lev >= 12 && p_ptr->warning_potions == 0) {
#if 1
			/* scan inventory for any potions */
			bool found_items = FALSE;
			int i;
			for (i = 0; i < INVEN_PACK; i++) {
				if (!p_ptr->inventory[i].k_idx) continue;
				if (!object_known_p(Ind, &p_ptr->inventory[i])) continue;
				if (p_ptr->inventory[i].tval == TV_POTION && (
//				    p_ptr->inventory[i].sval == SV_POTION_CURE_LIGHT ||
				    p_ptr->inventory[i].sval == SV_POTION_CURE_SERIOUS ||
				    p_ptr->inventory[i].sval == SV_POTION_CURE_CRITICAL))
					found_items = TRUE;
			}
			if (!found_items)
#endif
			{
				msg_print(Ind, "\374\377oHINT: Buy potions of cure wounds from the \377Rtemple\377o in Bree. They will");
				msg_print(Ind, "\374\377o      restore your hit points, ensuring your survival in critical situations.");
				msg_print(Ind, "\374\377o      Ideally, create a \377Rmacro\377o for using them by a single key press!");
				s_printf("warning_potions: %s\n", p_ptr->name);
			}
			p_ptr->warning_potions = 1;
		}

		/* Give warning message to utilize techniques */
		if (old_lev < 15 && p_ptr->lev >= 15) {
			if (p_ptr->warning_technique_melee == 0 && p_ptr->warning_technique_ranged == 0) {
				msg_print(Ind, "\374\377yHINT: Press 'm' to access 'Fighting Techniques' and 'Shooting Techniques'!");
				msg_print(Ind, "\374\377y      You can also create macros for these. See the TomeNET guide for details.");
				s_printf("warning_techniques: %s\n", p_ptr->name);
			} else if (p_ptr->warning_technique_melee == 0) {
				msg_print(Ind, "\374\377yHINT: Press 'm' to access 'Fighting Techniques'!");
				msg_print(Ind, "\374\377y      You can also create macros for these. See the TomeNET guide for details.");
				s_printf("warning_technique_melee: %s\n", p_ptr->name);
			} else if (p_ptr->warning_technique_ranged == 0) {
				msg_print(Ind, "\374\377yHINT: Press 'm' to access 'Shooting Techniques'!");
				msg_print(Ind, "\374\377y      You can also create macros for these. See the TomeNET guide for details.");
				s_printf("warning_technique_ranged: %s\n", p_ptr->name);
			}
			p_ptr->warning_technique_melee = p_ptr->warning_technique_ranged = 1;
		}

		/* Introduce newly learned abilities (that depend on char level) */
		/* those that depend on a race */
		switch (p_ptr->prace) {
		case RACE_DWARF:
			if (old_lev < 30 && p_ptr->lev >= 30) msg_print(Ind, "\374\377GYou learn to climb mountains easily!");
			break;
		case RACE_ENT:
			if (old_lev < 4 && p_ptr->lev >= 4) msg_print(Ind, "\374\377GYou learn to see the invisible!");
			if (old_lev < 10 && p_ptr->lev >= 10) msg_print(Ind, "\374\377GYou learn to telepathically sense animals!");
			if (old_lev < 15 && p_ptr->lev >= 15) msg_print(Ind, "\374\377GYou learn to telepathically sense orcs!");
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYou learn to telepathically sense trolls!");
		if (old_lev < 25 && p_ptr->lev >= 25) msg_print(Ind, "\374\377GYou learn to telepathically sense giants!");
			if (old_lev < 30 && p_ptr->lev >= 30) msg_print(Ind, "\374\377GYou learn to telepathically sense dragons!");
			if (old_lev < 40 && p_ptr->lev >= 40) msg_print(Ind, "\374\377GYou learn to telepathically sense demons!");
			if (old_lev < 50 && p_ptr->lev >= 50) msg_print(Ind, "\374\377GYou learn to telepathically sense evil!");
		break;
		case RACE_DRIDER:
			if (old_lev < 5 && p_ptr->lev >= 5) msg_print(Ind, "\374\377GYou learn to telepathically sense dragons!");
			if (old_lev < 10 && p_ptr->lev >= 10) msg_print(Ind, "\374\377GYou become more resistant to fire!");
			if (old_lev < 15 && p_ptr->lev >= 15) msg_print(Ind, "\374\377GYou become more resistant to cold!");
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYou become more resistant to acid!");
			if (old_lev < 25 && p_ptr->lev >= 25) msg_print(Ind, "\374\377GYou become more resistant to lightning!");
			if (old_lev < 30 && p_ptr->lev >= 30) msg_print(Ind, "\374\377GYou learn how to fly!");
			break;
		case RACE_DARK_ELF:
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYou learn to see the invisible!");
			break;
		case RACE_VAMPIRE:
			if (old_lev < 10 && p_ptr->lev >= 10) msg_print(Ind, "\374\377GYour vision extends.");
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYour vision extends.");
			if (old_lev < 30 && p_ptr->lev >= 30) msg_print(Ind, "\374\377GYour vision extends.");
			if (old_lev < 40 && p_ptr->lev >= 40) msg_print(Ind, "\374\377GYour vision extends.");
			if (old_lev < 50 && p_ptr->lev >= 50) msg_print(Ind, "\374\377GYour vision extends.");
			if (old_lev < 60 && p_ptr->lev >= 60) msg_print(Ind, "\374\377GYour vision extends.");
			if (old_lev < 70 && p_ptr->lev >= 70) msg_print(Ind, "\374\377GYour vision extends.");
			if (old_lev < 80 && p_ptr->lev >= 80) msg_print(Ind, "\374\377GYour vision extends.");
			if (old_lev < 90 && p_ptr->lev >= 90) msg_print(Ind, "\374\377GYour vision extends.");
//			if (old_lev < 30 && p_ptr->lev >= 30) msg_print(Ind, "\374\377GYou learn how to fly!");
			if (old_lev < 20 && p_ptr->lev >= 20) {
				msg_print(Ind, "\374\377GYou are now able to turn into a vampire bat (#391)!");
				msg_print(Ind, "\374\377G(Press 'm' key, and choose 'use innate power', to do so.)");
			}
			break;
#ifdef ENABLE_DIVINE
		case RACE_DIVINE:
			if (p_ptr->ptrait) break; /* In case we got *bad* exp drain for some unfathomable reason ;) */
			if (old_lev < 12 && p_ptr->lev >= 12) msg_print(Ind, "\374\377GWe all have to pick our own path some time...");
//			if (old_lev < 14 && p_ptr->lev >= 14) msg_print(Ind, "\374\377GYou are thirsty for blood: be it good or evil");
			if (old_lev < 14 && p_ptr->lev >= 14) msg_print(Ind, "\374\377GYour soul thirsts for shaping, either enlightenment or corruption!");
			if (old_lev <= 19 && p_ptr->lev >= 15 && old_lev < p_ptr->lev) {
				/* Killed none? nothing happens except if we reached the threshold level */
				if (p_ptr->r_killed[MONSTER_RIDX_CANDLEBEARER] == 0
				    && p_ptr->r_killed[MONSTER_RIDX_DARKLING] == 0) {
					/* Threshold level has been overstepped -> die */
					if (old_lev == 19) {
//						msg_print(Ind, "\377RYou don't deserve to live.");
						msg_print(Ind, "\377RYour indecision proves you aren't ready yet to stay in this realm!");
						strcpy(p_ptr->died_from, "Indecisiveness");
						p_ptr->deathblow = 0;
						p_ptr->death = TRUE;
					}
					/* We're done here.. */
					break;
				}

				/* Killed both? -> you die */
				if (p_ptr->r_killed[MONSTER_RIDX_CANDLEBEARER] != 0 &&
				    p_ptr->r_killed[MONSTER_RIDX_DARKLING] != 0) {
					msg_print(Ind, "\377RYour indecision proves you aren't ready yet to stay in this realm!");
					strcpy(p_ptr->died_from, "Indecisiveness");
					p_ptr->deathblow = 0;
					p_ptr->death = TRUE;
					/* End of story, next.. */
					break;
				}

				/* Don't initiate earlier than at threshold level, it's a ceremony ^^ */
				if (p_ptr->lev <= 19) break;

				/* Modify skill tree */
				if (p_ptr->r_killed[MONSTER_RIDX_CANDLEBEARER] != 0) {
					//A demon appears!
					msg_print(Ind, "\374\377p*** \377GYour corruption grows well within you. \377p***");
					p_ptr->ptrait = TRAIT_CORRUPTED;
				} else {
					//An angel appears!
					msg_print(Ind, "\374\377p*** \377sYou have been ordained to be order in presence of chaos. \377p***");
					p_ptr->ptrait = TRAIT_ENLIGHTENED;
				}

				shape_Maia_skills(Ind);
				calc_techniques(Ind);

				p_ptr->redraw |= PR_SKILLS | PR_MISC;
				p_ptr->update |= PU_SKILL_INFO | PU_SKILL_MOD;
			}
			break;
#endif
		}

		/* those that depend on a class */
		switch (p_ptr->pclass) {
		case CLASS_ADVENTURER:
			if (old_lev < 6 && p_ptr->lev >= 6)
				msg_print(Ind, "\374\377GYou learn the fighting technique 'Sprint'! (press 'm')");
			if (old_lev < 15 && p_ptr->lev >= 15)
				msg_print(Ind, "\374\377GYou learn the fighting technique 'Taunt'!");
			/* Also update the client's 'm' menu for fighting techniques */
			calc_techniques(Ind);
			Send_skill_info(Ind, SKILL_TECHNIQUE, TRUE);
			break;
		case CLASS_ROGUE:
#ifdef ENABLE_CLOAKING
			if (old_lev < LEARN_CLOAKING_LEVEL && p_ptr->lev >= LEARN_CLOAKING_LEVEL)
				msg_print(Ind, "\374\377GYou learn how to cloak yourself to pass unnoticed (press 'V').");
#endif
			if (old_lev < 3 && p_ptr->lev >= 3)
				msg_print(Ind, "\374\377GYou learn the fighting technique 'Sprint'! (press 'm')");
			if (old_lev < 6 && p_ptr->lev >= 6)
				msg_print(Ind, "\374\377GYou learn the fighting technique 'Taunt'");
			if (old_lev < 9 && p_ptr->lev >= 9)
				msg_print(Ind, "\374\377GYou learn the fighting technique 'Distract'");
			if (old_lev < 12 && p_ptr->lev >= 12)
				msg_print(Ind, "\374\377GYou learn the fighting technique 'Flash bomb'");
#ifdef ENABLE_ASSASSINATE
			if (old_lev < 35 && p_ptr->lev >= 35)
				msg_print(Ind, "\374\377GYou learn the fighting technique 'Assasinate'");
#endif
			if (old_lev < 50 && p_ptr->lev >= 50 && p_ptr->total_winner)
				msg_print(Ind, "\374\377GYou learn the royal fighting technique 'Shadow run'");
			/* Also update the client's 'm' menu for fighting techniques */
			calc_techniques(Ind);
			Send_skill_info(Ind, SKILL_TECHNIQUE, TRUE);
			break;
		case CLASS_RANGER:
			if (old_lev < 15 && p_ptr->lev >= 15) msg_print(Ind, "\374\377GYou learn how to move through dense forests easily.");
			if (old_lev < 25 && p_ptr->lev >= 25) msg_print(Ind, "\374\377GYou learn how to swim well, with heavy backpack even.");
			break;
		case CLASS_DRUID: /* Forms gained by Druids */
			/* compare mimic_druid in defines.h */
			if (old_lev < 5 && p_ptr->lev >= 5) {
				msg_print(Ind, "\374\377GYou learn the fighting technique 'Sprint'! (press 'm')");
				msg_print(Ind, "\374\377GYou learn how to change into a Cave Bear (#160) and Panther (#198)");
				msg_print(Ind, "\374\377G(Press 'm' key, and choose 'use innate power', to do so.)");
			}
			if (old_lev < 10 && p_ptr->lev >= 10) {
				msg_print(Ind, "\374\377GYou learn how to change into a Grizzly Bear (#191) and Yeti (#154)");
				msg_print(Ind, "\374\377GYou learn how to walk among your brothers through deep forest.");
			}
			if (old_lev < 15 && p_ptr->lev >= 15) msg_print(Ind, "\374\377GYou learn how to change into a Griffon (#279) and Sasquatch (#343)");
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYou learn how to change into a Werebear (#414), Great Eagle (#335), Aranea (#963) and Great White Shark (#898)");
			if (old_lev < 25 && p_ptr->lev >= 25) msg_print(Ind, "\374\377GYou learn how to change into a Wyvern (#334) and Multi-hued Hound (#513)");
			if (old_lev < 30 && p_ptr->lev >= 30) msg_print(Ind, "\374\377GYou learn how to change into a 5-h-Hydra (#440), Minotaur (#641) and Giant Squid (#482)");
			if (old_lev < 35 && p_ptr->lev >= 35) msg_print(Ind, "\374\377GYou learn how to change into a 7-h-Hydra (#614), Elder Aranea (#964) and Plasma Hound (#726)");
			if (old_lev < 40 && p_ptr->lev >= 40) msg_print(Ind, "\374\377GYou learn how to change into an 11-h-Hydra (#688), Giant Roc (#640) and Lesser Kraken (#740)");
			if (old_lev < 45 && p_ptr->lev >= 45) msg_print(Ind, "\374\377GYou learn how to change into a Maulotaur (#723) and Winged Horror (#704)");// and Behemoth (#716)");
			if (old_lev < 50 && p_ptr->lev >= 50) msg_print(Ind, "\374\377GYou learn how to change into a Spectral tyrannosaur (#705), Jabberwock (#778) and Greater Kraken (775)");//Leviathan (#782)");
			if (old_lev < 55 && p_ptr->lev >= 55) msg_print(Ind, "\374\377GYou learn how to change into a Horned Serpent (#1131)");
			if (old_lev < 60 && p_ptr->lev >= 60) msg_print(Ind, "\374\377GYou learn how to change into a Firebird (#1127)");
			calc_techniques(Ind);
			Send_skill_info(Ind, SKILL_TECHNIQUE, TRUE);
			break;
		case CLASS_SHAMAN:
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYou learn to see the invisible!");
			break;
		case CLASS_RUNEMASTER:
			if (old_lev < 4 && p_ptr->lev >= 4)
				msg_print(Ind, "\374\377GYou learn the fighting technique 'Sprint'! (press 'm')");
			if (old_lev < 9 && p_ptr->lev >= 9)
				msg_print(Ind, "\374\377GYou learn the fighting technique 'Taunt'");
			/* Also update the client's 'm' menu for fighting techniques */
			calc_techniques(Ind);
			Send_skill_info(Ind, SKILL_TECHNIQUE, TRUE);
			break;
		case CLASS_MINDCRAFTER:
			if (old_lev < 10 && p_ptr->lev >= 10) msg_print(Ind, "\374\377GYou learn to keep hold of your sanity!");
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYou learn to keep strong hold of your sanity!");
			if (old_lev < 8 && p_ptr->lev >= 8)
				msg_print(Ind, "\374\377GYou learn the fighting technique 'Taunt' (press 'm')");
			if (old_lev < 12 && p_ptr->lev >= 12)
				msg_print(Ind, "\374\377GYou learn the fighting technique 'Distract'");
			/* Also update the client's 'm' menu for fighting techniques */
			calc_techniques(Ind);
			Send_skill_info(Ind, SKILL_TECHNIQUE, TRUE);
			break;
		}

#ifdef ENABLE_STANCES
		/* increase SKILL_STANCE by +1 automatically (just for show :-p) if we actually have that skill */
		if (get_skill(p_ptr, SKILL_STANCE) && p_ptr->lev <= 50) {
			p_ptr->s_info[SKILL_STANCE].value = p_ptr->lev * 1000;
			/* Update the client */
			Send_skill_info(Ind, SKILL_STANCE, TRUE);
			/* Also update the client's 'm' menu for fighting techniques */
			calc_techniques(Ind);
			Send_skill_info(Ind, SKILL_TECHNIQUE, TRUE);
			/* give message if we learn a new stance (compare cmd6.c! keep it synchronized */
		        /* take care of message about fighting techniques too: */
			msg_gained_abilities(Ind, (p_ptr-> lev - 1) * 10, SKILL_STANCE);
		}
#endif

#if 0		/* Make fruit bat gain speed on levelling up, instead of starting out with full +10 speed bonus? */
		if (p_ptr->fruit_bat == 1 && old_lev < p_ptr->lev) {
			if (p_ptr->lev % 5 == 0 && p_ptr->lev <= 35) msg_print(Ind, "\374\377GYour flying abilities have improved, you have gained some speed.");
		}
#endif

#ifdef KINGCAP_LEV
		/* Added a check that (s)he's not already a king - mikaelh */
//		if(p_ptr->lev == 50 && !p_ptr->total_winner) msg_print(Ind, "\374\377GYou can't gain more levels until you defeat Morgoth, Lord of Darkness!");
		if(p_ptr->lev == 50 && !p_ptr->total_winner) msg_print(Ind, "\374\377G* To level up further, you need to defeat Morgoth, Lord of Darkness! *");
#endif

		/* pvp mode cant go higher, but receives a reward maybe */
		if(p_ptr->mode & MODE_PVP) {
			if (get_skill(p_ptr, SKILL_MIMIC) &&
			    p_ptr->pclass != CLASS_DRUID &&
			    p_ptr->prace != RACE_VAMPIRE) {
				msg_print(Ind, "\375\377GYou gain one free mimicry transformation of your choice!");
				p_ptr->free_mimic = 1;
			}
			if(p_ptr->lev == MID_PVP_LEVEL) {
				msg_broadcast_format(Ind, "\374\377G* %s has raised in ranks greatly! *", p_ptr->name);
				msg_print(Ind, "\375\377G* You have raised quite a bit in ranks of PvP characters!         *");
				msg_print(Ind, "\375\377G*   For that, you just received a reward, and if you die you will *");
				msg_print(Ind, "\375\377G*   also receive a deed on the next character you log in with.    *");
		                give_reward(Ind, RESF_MID, "Gladiator's reward", 1, 0);
			}
			if(p_ptr->lev == MAX_PVP_LEVEL) {
				msg_broadcast_format(Ind, "\374\377G* %s has reached the highest level available to PvP characters! *", p_ptr->name);
				msg_print(Ind, "\375\377G* You have reached the highest level available to PvP characters! *");
				msg_print(Ind, "\375\377G*   For that, you just received a reward, and if you die you will *");
				msg_print(Ind, "\375\377G*   also receive a deed on the next character you log in with.    *");
//				buffer_account_for_achievement_deed(p_ptr, ACHV_PVP_MAX);
		                give_reward(Ind, RESF_HIGH, "Gladiator's reward", 1, 0);
			}
		}

		snprintf(str, 160, "\374\377G%s has attained level %d.", p_ptr->name, p_ptr->lev);
		s_printf("%s has attained level %d.\n", p_ptr->name, p_ptr->lev);
		clockin(Ind, 1);	/* Set player level */
#ifdef TOMENET_WORLDS
		if (cfg.worldd_lvlup) world_msg(str);
#endif
		msg_broadcast(Ind, str);

		/* Update the skill points info on the client */
		Send_skill_info(Ind, 0, TRUE);
	}
}


/*
 * Gain experience
 */
void gain_exp(int Ind, s64b amount) {
	player_type *p_ptr = Players[Ind];//, *p_ptr2=NULL;
//	int Ind2 = 0;

	if (is_admin(p_ptr) && p_ptr->lev >= 99) return;

#ifdef IRON_TEAM_EXPERIENCE
	int iron_team_members_here = 0, iron_team_limit = 0;
#endif

#ifdef ARCADE_SERVER
return;
#endif

	if (amount <= 0) return;
#ifdef ALT_EXPRATIO
	/* New way to gain exp: Exp ratio works no longer for determining required exp
	   to level up, but instead to determine how much exp you gain: */
	amount = (amount * 100L) / ((s64b)p_ptr->expfact);
	if (amount < 1) amount = 1;
#endif

	/* You cant gain xp on your land */
	if (player_is_king(Ind)) return;


#ifdef IRON_TEAM_EXPERIENCE
	/* moved here from party_gain_exp() for implementing the 'sync'-exception */
	/* Iron Teams only get exp if the whole team is on the same floor! - C. Blue */
	if (p_ptr->party && parties[p_ptr->party].mode == PA_IRONTEAM) {
		for (i = 1; i <= NumPlayers; i++) {
			if (p_ptr->conn == NOT_CONNECTED) continue;

			/* note: this line means that iron teams must not add
			admins, or the members won't gain exp anymore */
			if (is_admin(p_ptr)) continue;

			/* player on the same dungeon level? */
			if (!inarea(&p_ptr->wpos, wpos)) continue;

			/* count party members on the same dlvl */
			if (player_in_party(p_ptr->party, i)) iron_team_members_here++;
		}

		/* only gain exp if all members are here */
		if (iron_team_members_here != parties[p_ptr->party].members) {
			/* New: allow exception to somewhat 'sync' own exp w/ the exp of
			iron team member having the most exp, to avoid falling back too much.
			(most drastic example: death of everlasting char). - C. Blue */
			if (p_ptr->exp >= parties[p_ptr->party].experience) return;
			iron_team_limit = parties[p_ptr->party].experience;
		}
	}
#endif


	/* allow own kills to be gained */
	if (p_ptr->ghost) amount = (amount * 2) / 4;

#ifdef KINGCAP_LEV
	/* You must defeat morgoth before being allowed level > 50
	   otherwise stop 1 exp point before 51 */
 #ifndef ALT_EXPRATIO
	if ((!p_ptr->total_winner) && (p_ptr->exp + amount + 1 >=
	    ((s64b)((s64b)player_exp[50 - 1] * (s64b)p_ptr->expfact / 100L)))) {
		if (p_ptr->exp + 1 >=
		    ((s64b)((s64b)player_exp[50 - 1] * (s64b)p_ptr->expfact / 100L)))
			return;
		amount = ((s64b)((s64b)player_exp[50 - 1] * (s64b)p_ptr->expfact / 100L)) - p_ptr->exp;
		amount--;
	}
 #else
	if ((!p_ptr->total_winner) && (p_ptr->exp + amount + 1 >= ((s64b)player_exp[50 - 1]))) {
		if (p_ptr->exp + 1 >= ((s64b)player_exp[50 - 1]))
			return;
		amount = ((s64b)player_exp[50 - 1]) - p_ptr->exp;
		amount--;
	}
 #endif
#endif
#ifdef KINGCAP_EXP
	/* You must defeat morgoth before being allowed to gain more
	than 21,240,000 exp which is level 50 for Draconian Ranger <- might be OUTDATED */
	if ((!p_ptr->total_winner) && (p_ptr->exp + amount >= 21240000)) {
		if (p_ptr->exp >= 21240000) return;
		amount = 21240000 - p_ptr->exp;
	}
#endif

	/* PvP-mode players have a level limit */
	if (p_ptr->mode & MODE_PVP) {
#ifndef ALT_EXPRATIO
		if (p_ptr->exp + amount + 1 >= ((s64b)((s64b)player_exp[MAX_PVP_LEVEL - 1] *
					    (s64b)p_ptr->expfact / 100L))) {
			if (p_ptr->exp + 1 >= ((s64b)((s64b)player_exp[MAX_PVP_LEVEL - 1] *
					    (s64b)p_ptr->expfact / 100L)))
				return;
			amount = ((s64b)((s64b)player_exp[MAX_PVP_LEVEL - 1] * (s64b)p_ptr->expfact / 100L)) - p_ptr->exp;
			amount--;
		}
#else
		if (p_ptr->exp + amount + 1 >= ((s64b)player_exp[MAX_PVP_LEVEL - 1])) {
			if (p_ptr->exp + 1 >= ((s64b)player_exp[MAX_PVP_LEVEL - 1]))
				return;
			amount = ((s64b)player_exp[MAX_PVP_LEVEL - 1]) - p_ptr->exp;
			amount--;
		}
#endif
	}

#ifdef IRON_TEAM_EXPERIENCE
	/* new: allow players to 'sync' their exp to leading player in an iron team party */
	if (iron_team_limit && (p_ptr->exp + amount > iron_team_limit))
		amount = iron_team_limit - p_ptr->exp;
#endif

	/* Gain some experience */
	p_ptr->exp += amount;

	/* Slowly recover from experience drainage */
	if (p_ptr->exp < p_ptr->max_exp) {
#ifdef KINGCAP_LEV
		/* You must defeat morgoth before beong allowed level > 50 */
 #ifndef ALT_EXPRATIO
		if ((!p_ptr->total_winner) && (p_ptr->max_exp + (amount/10) + 1 >= ((s64b)((s64b)player_exp[50 - 1] *
		   (s64b)p_ptr->expfact / 100L)))) {
			if (p_ptr->max_exp >= ((s64b)((s64b)player_exp[50 - 1] *
			   (s64b)p_ptr->expfact / 100L)))
				return;
			amount = (((s64b)((s64b)player_exp[50 - 1] * (s64b)p_ptr->expfact / 100L)) - p_ptr->max_exp);
			amount--;
		}
 #else
		if ((!p_ptr->total_winner) && (p_ptr->max_exp + (amount/10) + 1 >= ((s64b)player_exp[50 - 1]))) {
			if (p_ptr->max_exp >= ((s64b)player_exp[50 - 1]))
				return;
			amount = (((s64b)player_exp[50 - 1]) - p_ptr->max_exp);
			amount--;
		}
 #endif
#endif
#ifdef KINGCAP_EXP
		if ((!p_ptr->total_winner) && (p_ptr->max_exp + (amount/10) >= 21240000)) {
			if (p_ptr->max_exp >= 21240000) return;
			amount = (21240000 - p_ptr->max_exp);
		}
#endif

#ifdef IRON_TEAM_EXPERIENCE
		/* new: allow players to 'sync' their exp to leading player in an iron team party */
		if (iron_team_limit && (p_ptr->max_exp + amount > iron_team_limit))
			amount = iron_team_limit - p_ptr->max_exp;
#endif

		/* Gain max experience (10%) */
		p_ptr->max_exp += amount / 10;
	}

	/* Check Experience */
	check_experience(Ind);

#ifdef IRON_TEAM_EXPERIENCE
	/* possibly set new maximum for iron team */
	if (p_ptr->party && parties[p_ptr->party].mode == PA_IRONTEAM &&
	    p_ptr->max_exp > parties[p_ptr->party].experience)
		parties[p_ptr->party].experience = p_ptr->max_exp;
#endif
}


/*
 * Lose experience
 */
void lose_exp(int Ind, s32b amount)
{
	player_type *p_ptr = Players[Ind];

	/* Amulet of Immortality */
	object_type *o_ptr = &p_ptr->inventory[INVEN_NECK];
	/* Skip empty items */
	if (o_ptr->k_idx) {
		if (o_ptr->tval == TV_AMULET &&
		    (o_ptr->sval == SV_AMULET_INVINCIBILITY || o_ptr->sval == SV_AMULET_INVULNERABILITY))
			return;
	}

	if (safe_area(Ind)) return;

	/* Never drop below zero experience */
	if (amount > p_ptr->exp) amount = p_ptr->exp - 1;

	/* Lose some experience */
	p_ptr->exp -= amount;

	/* Check Experience */
	check_experience(Ind);
}




/*
 * Hack -- Return the "automatic coin type" of a monster race
 * Used to allocate proper treasure when "Creeping coins" die
 *
 * XXX XXX XXX Note the use of actual "monster names"
 */
static int get_coin_type(monster_race *r_ptr)
{
	cptr name = (r_name + r_ptr->name);

	/* Analyze "coin" monsters */
	if (r_ptr->d_char == '$') {
		/* Look for textual clues */
		if (strstr(name, " copper ")) return (2);
		if (strstr(name, " silver ")) return (5);
		if (strstr(name, " gold ")) return (10);
		if (strstr(name, " mithril ")) return (16);
		if (strstr(name, " adamantite ")) return (17);

		/* Look for textual clues */
		if (strstr(name, "Copper ")) return (2);
		if (strstr(name, "Silver ")) return (5);
		if (strstr(name, "Gold ")) return (10);
		if (strstr(name, "Mithril ")) return (16);
		if (strstr(name, "Adamantite ")) return (17);
	}

	/* Assume nothing */
	return (0);
}


/*
 * Handle the "death" of a monster.
 *
 * Disperse treasures centered at the monster location based on the
 * various flags contained in the monster flags fields.
 *
 * Check for "Quest" completion when a quest monster is killed.
 *
 * Note that only the player can induce "monster_death()" on Uniques.
 * Thus (for now) all Quest monsters should be Uniques.
 *
 * Note that in a few, very rare, circumstances, killing Morgoth
 * may result in the Iron Crown of Morgoth crushing the Lead-Filled
 * Mace "Grond", since the Iron Crown is more important.
 */

void monster_death(int Ind, int m_idx)
{
	player_type *p_ptr = Players[Ind];
	player_type *q_ptr = Players[Ind];

	int	i, j, y, x, ny, nx;
	int	tmp_luck = p_ptr->luck;

	int	dump_item = 0;
	int	dump_gold = 0;

	int	number = 0;
	int	total = 0;

	char buf[160], m_name[MAX_CHARS];
	cptr titlebuf;

	cave_type *c_ptr;

	monster_type *m_ptr = &m_list[m_idx];
        monster_race *r_ptr = race_inf(m_ptr);
        bool is_Morgoth = (strcmp(r_name_get(m_ptr), "Morgoth, Lord of Darkness") == 0);
	int credit_idx = r_ptr->dup_idx ? r_ptr->dup_idx : m_ptr->r_idx;
	bool visible = (p_ptr->mon_vis[m_idx] || (r_ptr->flags1 & RF1_UNIQUE));

	bool good = (r_ptr->flags1 & RF1_DROP_GOOD) ? TRUE : FALSE;
	bool great = (r_ptr->flags1 & RF1_DROP_GREAT) ? TRUE : FALSE;

	bool do_gold = (!(r_ptr->flags1 & RF1_ONLY_ITEM));
	bool do_item = (!(r_ptr->flags1 & RF1_ONLY_GOLD));

	int force_coin = get_coin_type(r_ptr);
	s16b local_quark = 0;
	object_type forge;
	object_type *qq_ptr;
	struct worldpos *wpos;
	cave_type **zcave;

	int a_idx, chance, I_kind;
	artifact_type *a_ptr;

	bool henc_cheezed = FALSE, pvp = ((p_ptr->mode & MODE_PVP) != 0);
	u32b resf_tmp = RESF_NONE;

	/* terminate mindcrafter charm effect */
	if (m_ptr->charmedignore) {
		Players[m_ptr->charmedignore]->mcharming--;
		m_ptr->charmedignore = 0;
	}

#ifdef RPG_SERVER
	/* Pet death. Update and inform the owner -the_sandman */
	if (m_ptr->pet) {
		for (i = NumPlayers; i > 0; i--)
		{	
			if (m_ptr->owner == Players[i]->id) {
				msg_format(i, "\374\377R%s has killed your pet!", Players[Ind]->name);
				msg_format(Ind, "\374\377RYou have killed %s's pet!", Players[i]->name);
				Players[i]->has_pet=0;
				FREE(m_ptr->r_ptr, monster_race); //no drop, no exp.
				return;
			}
		}
	}
#endif

        if (cfg.henc_strictness && !p_ptr->total_winner) {
                if (m_ptr->highest_encounter - p_ptr->max_lev > MAX_PARTY_LEVEL_DIFF + 1) henc_cheezed = TRUE; /* p_ptr->lev more logical but harsh */
                if (p_ptr->supported_by - p_ptr->max_lev > MAX_PARTY_LEVEL_DIFF + 1) henc_cheezed = TRUE; /* p_ptr->lev more logical but harsh */
        }


	/* Get the location */
	y = m_ptr->fy;
	x = m_ptr->fx;
	wpos=&m_ptr->wpos;
	if(!(zcave=getcave(wpos))) return;

	if (ge_special_sector && /* training tower event running? and we are there? */
	    wpos->wx == WPOS_ARENA_X && wpos->wy == WPOS_ARENA_Y &&
	    wpos->wz == WPOS_ARENA_Z) {
		monster_desc(0, m_name, m_idx, 0x00);
		msg_broadcast_format(0, "\376\377S** %s has defeated %s! **", p_ptr->name, m_name);
		s_printf("EVENT_RESULT: %s (%d) has defeated %s.\n", p_ptr->name, p_ptr->lev, m_name);
	}
 
	/* get monster name for damage deal description */
	monster_desc(Ind, m_name, m_idx, 0x00);
	
	process_hooks(HOOK_MONSTER_DEATH, "d", Ind);

if (season_halloween) {
	/* let everyone know, so they are prepared.. >:) */
	if (m_ptr->r_idx == 1086 || m_ptr->r_idx == 1087 || m_ptr->r_idx == 1088) {
		msg_broadcast_format(0, "\374\377L**\377o%s has defeated a tasty halloween spirit!\377L**", p_ptr->name);
		s_printf("HALLOWEEN: %s has defeated %s.\n", p_ptr->name, m_name);
		great_pumpkin_timer = 15 + rand_int(45);
	}
}



	/*
	 * Mega^3-hack: killing a 'Warrior of the Dawn' is likely to
	 * spawn another in the fallen one's place!
	 */
	if (strstr((r_name + r_ptr->name),"the Dawn")) {
		if (!(randint(20) == 13)) {
			int wy = p_ptr->py, wx = p_ptr->px;
			int attempts = 100;

			do {
				scatter(wpos, &wy, &wx,p_ptr->py,p_ptr->px, 20, 0);
			} while (!(in_bounds(wy,wx) && cave_floor_bold(zcave, wy,wx)) && --attempts);

			if (attempts > 0) {
#if 0
                                if (is_friend(m_ptr) > 0) {
					if (summon_specific_friendly(wy, wx, 100, SUMMON_DAWN, FALSE)) {
						if (player_can_see_bold(wy, wx))
							msg_print ("A new warrior steps forth!");
					}
				}
				else
#endif
				{
					if (summon_specific(wpos, wy, wx, 100, m_ptr->clone + 20, SUMMON_DAWN, 1, 0)) {
						if (player_can_see_bold(Ind, wy, wx))
							msg_print (Ind, "A new warrior steps forth!");
					}
				}
			}
		}
	}

	/* One more ultra-hack: An Unmaker goes out with a big bang! */
	else if (strstr((r_name + r_ptr->name),"Unmaker")) {
		int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
		(void)project(m_idx, 6, wpos, y, x, 150, GF_CHAOS, flg, "The Unmaker explodes for");
	}

	/* Pink horrors are replaced with 2 Blue horrors */
	else if (strstr((r_name + r_ptr->name),"Pink horror")) {
		for (i = 0; i < 2; i++) {
			int wy = p_ptr->py, wx = p_ptr->px;
			int attempts = 100;

			do {
				scatter(wpos, &wy, &wx, p_ptr->py, p_ptr->px, 3, 0);
			} while (!(in_bounds(wy,wx) && cave_floor_bold(zcave, wy,wx)) && --attempts);

			if (attempts > 0) {
				if (summon_specific(wpos, wy, wx, 100, 0, SUMMON_BLUE_HORROR, 1, 0)) { /* that's _not_ 2, lol */
					if (player_can_see_bold(Ind, wy, wx))
						msg_print (Ind, "A blue horror appears!");
				}
			}
		}
	}

	/* Let monsters explode! */
	for (i = 0; i < 4; i++) {
		if (m_ptr->blow[i].method == RBM_EXPLODE) {
			int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
			int typ = GF_MISSILE;
			int d_dice = m_ptr->blow[i].d_dice;
			int d_side = m_ptr->blow[i].d_side;
			int damage = damroll(d_dice, d_side);
			int base_damage = r_ptr->blow[i].d_dice * r_ptr->blow[i].d_side; /* if monster didn't gain levels */

			switch (m_ptr->blow[i].effect) {
			case RBE_HURT:      typ = GF_MISSILE; break;
			case RBE_POISON:    typ = GF_POIS; break;
			case RBE_UN_BONUS:  typ = GF_DISENCHANT; break;
			case RBE_UN_POWER:  typ = GF_MISSILE; break; /* ToDo: Apply the correct effects */
			case RBE_EAT_GOLD:  typ = GF_MISSILE; break;
			case RBE_EAT_ITEM:  typ = GF_MISSILE; break;
			case RBE_EAT_FOOD:  typ = GF_MISSILE; break;
			case RBE_EAT_LITE:  typ = GF_MISSILE; break;
			case RBE_ACID:      typ = GF_ACID; break;
			case RBE_ELEC:      typ = GF_ELEC; break;
			case RBE_FIRE:      typ = GF_FIRE; break;
			case RBE_COLD:      typ = GF_COLD; break;
			case RBE_BLIND:     typ = GF_BLIND; break;
//			case RBE_HALLU:     typ = GF_CONFUSION; break;
			case RBE_HALLU:     typ = GF_CHAOS; break;	/* CAUTION! */
			case RBE_CONFUSE:   typ = GF_CONFUSION; break;
			case RBE_TERRIFY:   typ = GF_MISSILE; break;
			case RBE_PARALYZE:  typ = GF_MISSILE; break;
			case RBE_LOSE_STR:  typ = GF_MISSILE; break;
			case RBE_LOSE_DEX:  typ = GF_MISSILE; break;
			case RBE_LOSE_CON:  typ = GF_MISSILE; break;
			case RBE_LOSE_INT:  typ = GF_MISSILE; break;
			case RBE_LOSE_WIS:  typ = GF_MISSILE; break;
			case RBE_LOSE_CHR:  typ = GF_MISSILE; break;
			case RBE_LOSE_ALL:  typ = GF_MISSILE; break;
			case RBE_PARASITE:  typ = GF_MISSILE; break;
			case RBE_SHATTER:   typ = GF_DETONATION; break;
			case RBE_EXP_10:    typ = GF_MISSILE; break;
			case RBE_EXP_20:    typ = GF_MISSILE; break;
			case RBE_EXP_40:    typ = GF_MISSILE; break;
			case RBE_EXP_80:    typ = GF_MISSILE; break;
			case RBE_DISEASE:   typ = GF_POIS; break;
			case RBE_TIME:      typ = GF_TIME; break;
			case RBE_SANITY:    typ = GF_MISSILE; break;
			}

			snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s inflicts", m_name);
			project(m_idx, 3, wpos, y, x, damage > base_damage ? base_damage : damage, typ, flg, p_ptr->attacker);
			break;
		}
	}

	/* clones don't drop treasure or complete quests.. */
	if (m_ptr->clone) return;
	/* ..neither do cheezed kills */
	if (henc_cheezed &&
	    !is_Morgoth && /* make exception for Morgoth, so hi-lvl fallen kings can re-king */
	    !streq(r_name_get(m_ptr), "Great Pumpkin")) /* allow a mixed hunting group */
		return;


	/* Determine how much we can drop */
	if ((r_ptr->flags1 & RF1_DROP_60) && (rand_int(100) < 60)) number++;
	if ((r_ptr->flags1 & RF1_DROP_90) && (rand_int(100) < 90)) number++;
	if (r_ptr->flags1 & RF1_DROP_1D2) number += damroll(1, 2);
	if (r_ptr->flags1 & RF1_DROP_2D2) number += damroll(2, 2);
	if (r_ptr->flags1 & RF1_DROP_3D2) number += damroll(3, 2);
	if (r_ptr->flags1 & RF1_DROP_4D2) number += damroll(4, 2);

	/* Hack -- inscribe items that a unique drops */
	if (r_ptr->flags1 & RF1_UNIQUE) {
		local_quark = quark_add(r_name + r_ptr->name);
		unique_quark = local_quark;

		/* make uniques drop a bit better than normal monsters */
		tmp_luck += 20;
		/* luck caps at 40 */
		if (tmp_luck > 40) tmp_luck = 40;
	}

	/* Drop some objects */
	for (j = 0; j < number; j++) {
		/* Try 20 times per item, increasing range */
//		for (i = 0; i < 20; ++i)
		{
#if 0
			int d = (i + 14) / 15;

			/* Pick a "correct" location */
			scatter(wpos, &ny, &nx, y, x, d, 0);
			/* Must be "clean" floor grid */
			if (!cave_clean_bold(zcave, ny, nx)) continue;

			/* Access the grid */
			c_ptr = &zcave[ny][nx];
#endif	// 0

			/* Hack -- handle creeping coins */
			coin_type = force_coin;

#ifdef TRADITIONAL_LOOT_LEVEL
			/* Average dungeon and monster levels */
			object_level = (getlevel(wpos) + r_ptr->level) / 2;
#else
			/* Monster level is more important than floor level */
 #if 0 /* extreme */
			object_level = r_ptr->level;
 #endif
 #if 0 /* spiking average */
			if (rand_int(2)) object_level = (getlevel(wpos) + (r_ptr->level * 2)) / 3;
			else object_level = r_ptr->level;
 #endif
 #if 1 /* safe average */
			object_level = (getlevel(wpos) + r_ptr->level * 2) / 3;
 #endif
#endif

			/* No easy item hunting in towns.. */
			if (wpos->wz == 0) object_level = r_ptr->level / 2;

			/* Place Gold */
			if (do_gold && (!do_item || (rand_int(100) < 50))) {
				place_gold(wpos, y, x, 0);
//				if (player_can_see_bold(Ind, ny, nx)) dump_gold++;
			}

			/* Place Object */
			else {
				place_object_restrictor = RESF_NONE;
				/* prepare restriction flags for loot */
				resf_tmp = make_resf(p_ptr);

				/* Morgoth never drops true artifacts */
				if (is_Morgoth) resf_tmp |= RESF_NOTRUEART;

				/* hack to allow custom test l00t drop for admins: */
				if (is_admin(p_ptr)) {
					/* the hack works by using weapon's inscription! */
					char *k_tval, *k_sval;
					if (p_ptr->inventory[INVEN_WIELD].tval &&
					    p_ptr->inventory[INVEN_WIELD].note &&
					    (k_tval = strchr(quark_str(p_ptr->inventory[INVEN_WIELD].note), '%')) &&
					    (k_sval = strchr(k_tval, ':'))
					    ) {
						resf_tmp |= RESF_DEBUG_ITEM;
						/* extract tval:sval */
						/* abuse luck parameter for this */
						tmp_luck = lookup_kind(atoi(k_tval + 1), atoi(k_sval + 1));
						/* catch invalid items */
						if (!tmp_luck) resf_tmp &= ~RESF_DEBUG_ITEM;
					}
				}

				/* generate an object and place it */
				place_object(wpos, y, x, good, great, FALSE, resf_tmp, r_ptr->drops, tmp_luck, ITEM_REMOVAL_NORMAL);

//				if (player_can_see_bold(Ind, ny, nx)) dump_item++;
			}

			/* Reset the object level */
			object_level = getlevel(wpos);

			/* Reset "coin" type */
			coin_type = 0;
		}
	}

	/* Forget it */
	unique_quark = 0;

	/* Take note of any dropped treasure */
	/* XXX this doesn't work for now.. (not used anyway) */
	if (visible && (dump_item || dump_gold)) {
		/* Take notes on treasure */
		lore_treasure(m_idx, dump_item, dump_gold);
	}

	/* Get credit for unique monster kills */
	if (r_ptr->flags1 & RF1_UNIQUE) {
		/* Set unique monster to 'killed' for this player */
		p_ptr->r_killed[m_ptr->r_idx] = 1;
		Send_unique_monster(Ind, m_ptr->r_idx);
		/* Set unique monster to 'helped with' for all other nearby players
		   who haven't explicitely killed it yet - C. Blue */
		for (i = 1; i <= NumPlayers; i++) {
			if (i == Ind) continue;
//for testing		if (is_admin(Players[i])) continue;
			if (Players[i]->conn == NOT_CONNECTED) continue;
			/* it's sufficient to just be on the same dungeon floor to get credit */
			if (!inarea(&p_ptr->wpos, &Players[i]->wpos)) continue;
			/* must be in the same party though */
			if (!Players[i]->party || p_ptr->party != Players[i]->party) continue;

			/* Hack: '2' means 'helped' instead of 'killed' */
			if (!Players[i]->r_killed[m_ptr->r_idx]) {
				Players[i]->r_killed[m_ptr->r_idx] = 2;
				Send_unique_monster(i, m_ptr->r_idx);
			}
		}
	/* Get kill credit for non-uniques (important for mimics) */
	//HACK: added test for m_ptr->r_idx to suppress bad msgs about 0 forms learned (exploders?)
	} else if (credit_idx && p_ptr->r_killed[credit_idx] < 1000) {
		int before = p_ptr->r_killed[credit_idx];
		i = get_skill_scale(p_ptr, SKILL_MIMIC, 100);

#ifdef RPG_SERVER
		/* There is a 1 in (m_ptr->level - kill count)^2 chance of learning form straight away 
		 * to make it easier (at least statistically) getting forms in the iron server. Plus,
		 * mimicked speed and hp are lowered already anyway.	- the_sandman */
		if ( ( r_info[m_ptr->r_idx].level - p_ptr->r_killed[credit_idx] > 0 ) && 
		     ( (randint((r_info[m_ptr->r_idx].level - p_ptr->r_killed[credit_idx]) *
		     	     	(r_info[m_ptr->r_idx].level - p_ptr->r_killed[credit_idx])) == 1))) {
			p_ptr->r_killed[credit_idx] = r_info[credit_idx].level;
		} else { /* Badluck */
			p_ptr->r_killed[credit_idx]++;

			/* Shamans have a chance to learn E forms very quickly */
			if (p_ptr->pclass == CLASS_SHAMAN && mimic_shaman_E(credit_idx))
				p_ptr->r_killed[credit_idx] += 2;
		}
#else
		if (pvp) {
			/* PvP mode chars learn forms very quickly! */
			p_ptr->r_killed[credit_idx] += 3;
		} else {
			p_ptr->r_killed[credit_idx]++;

			/* Shamans have a chance to learn E forms very quickly */
			if (p_ptr->pclass == CLASS_SHAMAN && mimic_shaman_E(credit_idx))
				p_ptr->r_killed[credit_idx] += 2;
		}
#endif

		if (p_ptr->r_killed[credit_idx] > 1000)
			p_ptr->r_killed[credit_idx] = 1000;

		if (i && i >= r_info[credit_idx].level &&
		    (before == 0 || /* <- for level 0 townspeople */
		     before < r_info[credit_idx].level) &&
		    (p_ptr->r_killed[credit_idx] >= r_info[credit_idx].level ||
		    /* for level 0 townspeople: */
		    r_info[credit_idx].level == 0))
		{
			if (!((r_ptr->flags1 & RF1_UNIQUE) || (p_ptr->pclass == CLASS_DRUID) || 
			    ((p_ptr->pclass == CLASS_SHAMAN) && !mimic_shaman(credit_idx)) ||
			    (p_ptr->prace == RACE_VAMPIRE))) {
				msg_format(Ind, "\374\377UYou have learned the form of %s! (%d)",
				    r_info[credit_idx].name + r_name, credit_idx);
				if (!p_ptr->warning_mimic) {
					p_ptr->warning_mimic = 1;
					if (p_ptr->max_plv < 10) {
						msg_print(Ind, "\374\377U(Press '\377ym\377U' key, and choose 'use innate power', to do so.)");
						s_printf("warning_mimic: %s\n", p_ptr->name);
					}
				}
			}
		}
	}

	/* Take note of the killer */
	if ((r_ptr->flags1 & RF1_UNIQUE) && !pvp) {
		int Ind2 = 0;
		player_type *p_ptr2 = NULL;

#ifdef MUCHO_RUMOURS
		/*the_sandman prints a rumour */
		/* print the same message other players get before it - mikaelh */
		msg_print(Ind, "Suddenly a thought comes to your mind:");
		fortune(Ind, TRUE);
#endif

		/* give credit to the killer by default */
if(cfg.unikill_format){
	/* let's try with titles before the name :) -C. Blue */
		if (q_ptr->lev < 60)
		titlebuf = player_title[q_ptr->pclass][((q_ptr->lev)/5 < 10)?(q_ptr->lev)/5 : 10][1 - q_ptr->male];
		else
		titlebuf = player_title_special[q_ptr->pclass][(q_ptr->lev < PY_MAX_PLAYER_LEVEL)? (q_ptr->lev - 60)/10 : 4][1 - q_ptr->male];

		if (is_Morgoth) {
			snprintf(buf, sizeof(buf), "\374\377v**\377L%s was slain by %s %s.\377v**", r_name_get(m_ptr), titlebuf, p_ptr->name);
		} else {
			snprintf(buf, sizeof(buf), "\374\377b**\377c%s was slain by %s %s.\377b**", r_name_get(m_ptr), titlebuf, p_ptr->name);
		}
}else{
	/* for now disabled (works though) since we don't have telepath class
	   at the moment, and party names would make the line grow too long if
	   combined with title before the actual name :/ -C. Blue */
		if (!Ind2) {
			if (is_Morgoth)
				snprintf(buf, sizeof(buf), "\374\377v**\377L%s was slain by %s.\377v**", r_name_get(m_ptr), p_ptr->name);
			else
				snprintf(buf, sizeof(buf), "\374\377b**\377c%s was slain by %s.\377b**", r_name_get(m_ptr), p_ptr->name);
		} else {
			if (is_Morgoth)
				snprintf(buf, sizeof(buf), "\374\377v**\377L%s was slain by fusion %s-%s.\377v**", r_name_get(m_ptr), p_ptr->name, p_ptr2->name);
			else
				snprintf(buf, sizeof(buf), "\374\377b**\377c%s was slain by fusion %s-%s.\377b**", r_name_get(m_ptr), p_ptr->name, p_ptr2->name);
		}

		/* give credit to the party if there is a teammate on the 
		   level, and the level is not 0 (the town)  */
		if (p_ptr->party) {
			for (i = 1; i <= NumPlayers; i++) {
				if ( (Players[i]->party == p_ptr->party) && (inarea(&Players[i]->wpos, &p_ptr->wpos)) && (i != Ind) && (p_ptr->wpos.wz) )
				{
					if (is_Morgoth)
						snprintf(buf, sizeof(buf), "\374\377v**\377L%s was slain by %s of %s.\377v**", r_name_get(m_ptr), p_ptr->name, parties[p_ptr->party].name);
					else
						snprintf(buf, sizeof(buf), "\374\377b**\377c%s was slain by %s of %s.\377b**", r_name_get(m_ptr), p_ptr->name, parties[p_ptr->party].name);
					break;
				}

			}
		}
}

    if (!is_admin(p_ptr)) {
#ifdef TOMENET_WORLDS
		if (cfg.worldd_unideath)
			world_msg(buf);
		else if (cfg.worldd_pwin && is_Morgoth)
			world_msg(buf);
#endif
		/* Tell every player */
		msg_broadcast(-1, buf);
		/* Log event */
		s_printf("%s was slain by %s.\n", r_name_get(m_ptr), p_ptr->name);

	}
    }

	/* If the dungeon where Morgoth is killed is Ironman/Forcedown/No-Recall
	   allow recalling on this particular floor, since player will lose all true arts.
	   Originally for RPG_SERVER, but atm enabled for all. - C. Blue */
	if (is_Morgoth) {
		dungeon_type *d_ptr = getdungeon(&p_ptr->wpos);
		dun_level *l_ptr = getfloor(&p_ptr->wpos);
		if ((((d_ptr->flags2 & DF2_IRON || d_ptr->flags1 & DF1_FORCE_DOWN)
		    && d_ptr->maxdepth > ABS(p_ptr->wpos.wz)) ||
		    (d_ptr->flags1 & DF1_NO_RECALL))
		    && !(l_ptr->flags1 & LF1_IRON_RECALL)) {
			/* Allow exceptional recalling.. */
			l_ptr->flags1 |= LF1_IRON_RECALL;
			/* ..and notify everyone on the level about it */
			floor_msg_format(&p_ptr->wpos, "\377gYou don't sense a magic barrier here!");
		}
	}

	if (r_ptr->flags1 & (RF1_DROP_CHOSEN)) {
		/* Mega-Hack -- drop "winner" treasures */
		if (is_Morgoth && !pvp) {
			/* Hack -- an "object holder" */
			object_type prize;

			int num = 0;

			/* Nothing left, game over... */
			for (i = 1; i <= NumPlayers; i++) {
				q_ptr = Players[i];
				if (q_ptr->ghost) continue;
				/* Make everyone in the game in the same party on the
				 * same level greater than or equal to level 40 total
				 * winners.
				 */
				if ((((p_ptr->party) && (q_ptr->party == p_ptr->party)) ||
				    (q_ptr == p_ptr) ) && q_ptr->lev >= 40 && inarea(&p_ptr->wpos,&q_ptr->wpos))
				{

					/* Total winner */
					q_ptr->total_winner = TRUE;
					q_ptr->once_winner = TRUE;
					s_printf("%s *** total_winner : %s (lev %d (%d))\n", showtime(), q_ptr->name, q_ptr->lev, q_ptr->max_lev);

					s_printf("CHARACTER_WINNER: race=%s ; class=%s\n", race_info[q_ptr->prace].title, class_info[q_ptr->pclass].title);


					/* Redraw the "title" */
					q_ptr->redraw |= (PR_TITLE);

					/* Congratulations */
					msg_print(i, "\377G*** CONGRATULATIONS ***");
					if (q_ptr->mode & (MODE_HARD | MODE_NO_GHOST)) {
						msg_format(i, "\374\377GYou have won the game and are henceforth titled '%s'!", (q_ptr->male)?"Emperor":"Empress");
						msg_broadcast_format(i, "\374\377v%s is henceforth known as %s %s", q_ptr->name, (q_ptr->male)?"Emperor":"Empress", q_ptr->name);
						if (!is_admin(q_ptr)) l_printf("%s \\{v%s has been crowned %s\n", showdate(), q_ptr->name, q_ptr->male ? "emperor" : "empress");
#ifdef TOMENET_WORLDS
						if (cfg.worldd_pwin) world_msg(format("\374\377v%s is henceforth known as %s %s", q_ptr->name, (q_ptr->male)?"Emperor":"Empress", q_ptr->name));
#endif
					} else {
						msg_format(i, "\374\377GYou have won the game and are henceforth titled '%s!'", (q_ptr->male)?"King":"Queen");
						msg_broadcast_format(i, "\374\377v%s is henceforth known as %s %s", q_ptr->name, (q_ptr->male)?"King":"Queen", q_ptr->name);
						if (!is_admin(q_ptr)) l_printf("%s \\{v%s has been crowned %s\n", showdate(), q_ptr->name, q_ptr->male ? "king" : "queen");
#ifdef TOMENET_WORLDS
						if (cfg.worldd_pwin) world_msg(format("\374\377v%s is henceforth known as %s %s", q_ptr->name, (q_ptr->male)?"King":"Queen", q_ptr->name));
#endif
					}
					msg_print(i, "\377G(You may retire (by committing suicide) when you are ready.)");

					num++;

					/* Set his retire_timer if neccecary */
					if (cfg.retire_timer >= 0) {
						q_ptr->retire_timer = cfg.retire_timer;
						msg_format(i, "Otherwise you will retire after %s minutes of tenure.", cfg.retire_timer);
					}

					/* take char dump and screenshot from winning scene */
					if (is_newer_than(&q_ptr->version, 4, 4, 2, 0, 0, 0)) Send_chardump(i, "-victory");

#ifdef ENABLE_STANCES
					/* increase SKILL_STANCE by +1 automatically (just for show :-p) if we actually have that skill */
					if (get_skill(q_ptr, SKILL_STANCE) >= 45) {
						/* give message if we learn a new stance (compare cmd6.c! keep it synchronized */
						msg_print(i, "\374\377GYou learn how to enter Royal Rank combat stances.");
						/* automatically upgrade currently taken stance power */
						if (q_ptr->combat_stance) q_ptr->combat_stance_power = 3;
					}
#endif

					if (get_skill(q_ptr, SKILL_MARTIAL_ARTS) >= 48) {
	                            		msg_print(i, "\374\377GYou learn the Royal Titan's Fist technique.");
				                msg_print(i, "\374\377GYou learn the Royal Phoenix Claw technique.");
		                        }
					
					if (q_ptr->lev >= 50 && q_ptr->pclass == CLASS_ROGUE) {
						msg_print(i, "\374\377GYou learn the royal fighting technique 'Shadow Run'");
						calc_techniques(i);
						Send_skill_info(i, SKILL_TECHNIQUE, TRUE);
					}
				}
			}	

			/* Paranoia (if a ghost killed Morgoth) ;) - C. Blue */
		    if (num) {

			/* Mega-Hack -- Prepare to make "Grond" */
			invcopy(&prize, lookup_kind(TV_BLUNT, SV_GROND));
			/* Mega-Hack -- Mark this item as "Grond" */
			prize.name1 = ART_GROND;
			/* Mega-Hack -- Actually create "Grond" */
			apply_magic(wpos, &prize, -1, TRUE, TRUE, TRUE, FALSE, TRUE);

			prize.number = num;
			prize.level = 45;
			prize.note = local_quark;
			prize.note_utag = strlen(quark_str(local_quark));

			/* Drop it in the dungeon */
			if (wpos->wz) prize.marked2 = ITEM_REMOVAL_NEVER;
			else prize.marked2 = ITEM_REMOVAL_DEATH_WILD;
			drop_near(&prize, -1, wpos, y, x);

			/* Mega-Hack -- Prepare to make "Morgoth" */
			invcopy(&prize, lookup_kind(TV_CROWN, SV_MORGOTH));
			/* Mega-Hack -- Mark this item as "Morgoth" */
			prize.name1 = ART_MORGOTH;
			/* Mega-Hack -- Actually create "Morgoth" */
			apply_magic(wpos, &prize, -1, TRUE, TRUE, TRUE, FALSE, TRUE);

			prize.number = num;
			prize.level = 45;
			prize.note = local_quark;
			prize.note_utag = strlen(quark_str(local_quark));

			/* Drop it in the dungeon */
			if (wpos->wz) prize.marked2 = ITEM_REMOVAL_NEVER;
			else prize.marked2 = ITEM_REMOVAL_DEATH_WILD;
			drop_near(&prize, -1, wpos, y, x);

		    } /* Paranoia tag */

			/* Hack -- instantly retire any new winners if neccecary */
			if (cfg.retire_timer == 0) {
				for (i = 1; i <= NumPlayers; i++) {
					p_ptr = Players[i];
					if (p_ptr->total_winner)
						do_cmd_suicide(i);
				}
			}
			
			FREE(m_ptr->r_ptr, monster_race);
			return;
		} else if (strstr((r_name + r_ptr->name),"Smeagol")) {
			/* Get local object */
			qq_ptr = &forge;

			object_wipe(qq_ptr);

			/* Mega-Hack -- Prepare to make a ring of invisibility */
			/* Sorry, =inv is too nice.. */
			//                        invcopy(qq_ptr, lookup_kind(TV_RING, SV_RING_INVIS));
			invcopy(qq_ptr, lookup_kind(TV_RING, SV_RING_STEALTH));
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));

			apply_magic(wpos, qq_ptr, -1, TRUE, TRUE, FALSE, FALSE, FALSE);

			qq_ptr->bpval = 5;
			/* Drop it in the dungeon */
			drop_near(qq_ptr, -1, wpos, y, x);
		}
	        /* finally made Robin Hood drop a Bow ;) */
    	        else if (strstr((r_name + r_ptr->name),"Robin Hood, the Outlaw") && magik(50)) {
			qq_ptr = &forge;
			object_wipe(qq_ptr);
    		        invcopy(qq_ptr, lookup_kind(TV_BOW, SV_LONG_BOW));
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));
			apply_magic(wpos, qq_ptr, -1, TRUE, TRUE, TRUE, TRUE, make_resf(p_ptr));
			drop_near(qq_ptr, -1, wpos, y, x);
		}
#ifndef ENABLE_RCRAFT
		//Tikki-tikki-tembo will drop some rune :) id = 1032
		//else if (!strcmp((r_name + r_ptr->name),"Tik'srvzllat"))
		else if (m_ptr->r_idx == 1032 /* Tik */) {
#ifndef RUNECRAFT
			//Drops cloud rune
			/* Get local object */
			qq_ptr = &forge;
			object_wipe(qq_ptr);

			invcopy(qq_ptr, lookup_kind(TV_RUNE1, SV_RUNE1_CLOUD));
			apply_magic(wpos, qq_ptr, -1, TRUE, TRUE, FALSE, FALSE, FALSE);
			qq_ptr->number = 1;
			qq_ptr->level = 35;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));

			/* Drop it in the dungeon */
			drop_near(qq_ptr, -1, wpos, y, x);

			//Drops some earth runes, too
			/* Get local object */
			qq_ptr = &forge; object_wipe(qq_ptr);

			invcopy(qq_ptr, lookup_kind(TV_RUNE2, SV_RUNE2_STONE));
			apply_magic(wpos, qq_ptr, -1, TRUE, TRUE, FALSE, FALSE, FALSE);
			qq_ptr->number = 1;
			qq_ptr->level = 35;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));

			//And some armageddon
			/* Drop it in the dungeon */
			drop_near(qq_ptr, -1, wpos, y, x);

			/* Get local object */
			qq_ptr = &forge; object_wipe(qq_ptr);

			invcopy(qq_ptr, lookup_kind(TV_RUNE2, SV_RUNE2_ARMAGEDDON));
			apply_magic(wpos, qq_ptr, -1, TRUE, TRUE, FALSE, FALSE, FALSE);
			qq_ptr->number = 1;
			qq_ptr->level = 40;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));

			/* Drop it in the dungeon */
			drop_near(qq_ptr, -1, wpos, y, x);
#endif
		}
#endif
		else if (strstr((r_name + r_ptr->name),"The Hellraiser")) {
			/* Get local object */
			qq_ptr = &forge;

			object_wipe(qq_ptr);

			/* Drop Scroll Of Artifact Creation along with loot */
			invcopy(qq_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_ARTIFACT_CREATION));
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));

			apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, FALSE, FALSE, FALSE);

			/* Drop it in the dungeon */
			drop_near(qq_ptr, -1, wpos, y, x);

			/* Prepare a second reward */
			object_wipe(qq_ptr);

			/* Drop Potions Of Learning along with loot */
			invcopy(qq_ptr, lookup_kind(TV_POTION2, SV_POTION2_LEARNING));
			qq_ptr->number = randint(2);
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));

			apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, FALSE, FALSE, FALSE);

			/* Drop it in the dungeon */
#ifdef PRE_OWN_DROP_CHOSEN
			qq_ptr->level = 0;
			qq_ptr->owner = p_ptr->id;
			qq_ptr->mode = p_ptr->mode;
#endif
			drop_near(qq_ptr, -1, wpos, y, x);
		} else if (r_ptr->flags7 & RF7_NAZGUL) {
			/* Get local object */
			qq_ptr = &forge;

			object_wipe(qq_ptr);

			/* Mega-Hack -- Prepare to make a Ring of Power */
			invcopy(qq_ptr, lookup_kind(TV_RING, SV_RING_SPECIAL));
			qq_ptr->number = 1;

			qq_ptr->name1 = ART_RANDART;

			/* Piece together a 32-bit random seed */
			qq_ptr->name3 = rand_int(0xFFFF) << 16;
			qq_ptr->name3 += rand_int(0xFFFF);

			/* Check the tval is allowed */
//			if (randart_make(qq_ptr) != NULL)

			apply_magic(wpos, qq_ptr, -1, FALSE, TRUE, FALSE, FALSE, FALSE);

			/* Save the inscription */
			/* (pfft, not so smart..) */
			/*qq_ptr->note = quark_add(format("#of %s", r_name + r_ptr->name));*/
			qq_ptr->bpval = m_ptr->r_idx;

			/* Drop it in the dungeon */
#ifdef PRE_OWN_DROP_CHOSEN
			qq_ptr->level = 0;
			qq_ptr->owner = p_ptr->id;
			qq_ptr->mode = p_ptr->mode;
#endif

			drop_near(qq_ptr, -1, wpos, y, x);
		} else if (!pvp) {
			a_idx = 0;
			chance = 0;
			I_kind = 0;

			/* chances should be reduced, so that the quickest
			 * won't benefit too much?	- Jir - */
			if (strstr((r_name + r_ptr->name),"T'ron, the Rebel Dragonrider")) {
				a_idx = ART_TRON;
				chance = 75;
			} else if (strstr((r_name + r_ptr->name),"Mardra, rider of the Gold Loranth")) {
				a_idx = ART_MARDA;
				chance = 75;
			} else if (strstr((r_name + r_ptr->name),"Saruman of Many Colours")) {
				a_idx = ART_ELENDIL;
				chance = 30;
			} else if (strstr((r_name + r_ptr->name),"Hagen, son of Alberich")) { /* not in the game */
				a_idx = ART_NIMLOTH;
				chance = 66;
			} else if (strstr((r_name + r_ptr->name),"Muar, the Balrog")) { /* not in the game */
				a_idx = ART_CALRIS;
				chance = 60;
			} else if (strstr((r_name + r_ptr->name),"Gothmog, the High Captain of Balrogs")) {
				a_idx = ART_GOTHMOG;
				chance = 80;
			} else if (strstr((r_name + r_ptr->name),"Eol, the Dark Elf")) {
				if (magik(25)) a_idx = ART_ANGUIREL;
				else a_idx = ART_EOL;
				chance = 65;
			} else if (strstr((r_name + r_ptr->name),"Kronos, Lord of the Titans")) {
				a_idx = ART_KRONOS;
				chance = 80;
			} else if (strstr((r_name + r_ptr->name),"Zu-Aon, The Cosmic Border Guard")) {
#if 0
				for (i = 1; i <= NumPlayers; i++) {
					if (inarea(&Players[i]->wpos, &p_ptr->wpos)
					    && !is_admin(Players[i]))
						l_printf("%s \\{U%s made it through Nether Realm.\n", showdate(), Players[i]->name);
				}
#endif

				/* Get local object */
				qq_ptr = &forge;
				object_wipe(qq_ptr);
				/* Drop Scroll Of Artifact Creation if Ring Of Phasing already exists */
				invcopy(qq_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_ARTIFACT_CREATION));
				qq_ptr->number = 1; /*(a_info[a_idx].cur_num == 0)?1:2;*/
				qq_ptr->note = local_quark;
				qq_ptr->note_utag = strlen(quark_str(local_quark));
				apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, FALSE, FALSE, FALSE);
				/* Drop it in the dungeon */
				drop_near(qq_ptr, -1, wpos, y, x);

				/* Prepare a second reward */
				object_wipe(qq_ptr);
				/* Drop Potions Of Learning along with loot */
				invcopy(qq_ptr, lookup_kind(TV_POTION2, SV_POTION2_LEARNING));
				qq_ptr->number = (a_info[203].cur_num == 0)?1:2;
				qq_ptr->note = local_quark;
				qq_ptr->note_utag = strlen(quark_str(local_quark));
				apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, FALSE, FALSE, FALSE);
				/* Drop it in the dungeon */
#ifdef PRE_OWN_DROP_CHOSEN
				qq_ptr->level = 0;
				qq_ptr->owner = p_ptr->id;
				qq_ptr->mode = p_ptr->mode;
#endif
				drop_near(qq_ptr, -1, wpos, y, x);

				if (a_info[a_idx].cur_num == 0) {
					/* Generate Ring Of Phasing -w00t ;) */
					a_idx = 203;
					chance = 100;
				}
		}

#ifdef SEMI_PROMISED_ARTS_MODIFIER
			chance = chance * SEMI_PROMISED_ARTS_MODIFIER / 100;
#endif	// SEMI_PROMISED_ARTS_MODIFIER

//			if ((a_idx > 0) && ((randint(99)<chance) || (wizard)))
			if ((a_idx > 0) && magik(chance) && !cfg.arts_disabled &&
			    (a_info[a_idx].cur_num == 0)) {
				a_ptr = &a_info[a_idx];
				/* Get local object */
				qq_ptr = &forge;
				/* Wipe the object */
				object_wipe(qq_ptr);
				/* Acquire the "kind" index */
				I_kind = lookup_kind(a_ptr->tval, a_ptr->sval);
				/* Create the artifact */
				invcopy(qq_ptr, I_kind);
				/* Save the name */
				qq_ptr->name1 = a_idx;

				if (!(make_resf(p_ptr) & RESF_NOTRUEART) || winner_artifact_p(qq_ptr)) {
					/* Extract the fields */
					qq_ptr->pval = a_ptr->pval;
					qq_ptr->ac = a_ptr->ac;
					qq_ptr->dd = a_ptr->dd;
					qq_ptr->ds = a_ptr->ds;
					qq_ptr->to_a = a_ptr->to_a;
					qq_ptr->to_h = a_ptr->to_h;
					qq_ptr->to_d = a_ptr->to_d;
					qq_ptr->weight = a_ptr->weight;
					qq_ptr->note = local_quark;
					qq_ptr->note_utag = strlen(quark_str(local_quark));

//					random_artifact_resistance(qq_ptr);
					a_info[a_idx].cur_num++;

					/* Hack -- acquire "cursed" flag */
					if (a_ptr->flags3 & (TR3_CURSED)) qq_ptr->ident |= (ID_CURSED);

					/* Complete generation, especially level requirements check */
					apply_magic(wpos, qq_ptr, -2, FALSE, TRUE, FALSE, FALSE, TRUE);

					/* Little sanity hack for level requirements
					   of the Ring of Phasing - would be 92 otherwise */
					if (a_idx == 203) {
						qq_ptr->level = (60 + rand_int(6));
						qq_ptr->marked2 = ITEM_REMOVAL_NEVER;
					}

					/* Drop the artifact from heaven */
#ifdef PRE_OWN_DROP_CHOSEN
					qq_ptr->level = 0;
					qq_ptr->owner = p_ptr->id;
					qq_ptr->mode = p_ptr->mode;
#endif
					drop_near(qq_ptr, -1, wpos, y, x);
				}
			}
		}
	}

	/* Hack - the Dragonriders give some firestone */
	else if (r_ptr->flags3 & RF3_DRAGONRIDER)
	{
		/* Get local object */
		qq_ptr = &forge;

		/* Prepare to make some Firestone */
		if (magik(70)) invcopy(qq_ptr, lookup_kind(TV_FIRESTONE, SV_FIRESTONE));
		else invcopy(qq_ptr, lookup_kind(TV_FIRESTONE, SV_FIRE_SMALL));
		qq_ptr->number = (byte)rand_range(1,12);

		/* Drop it in the dungeon */
		drop_near(qq_ptr, -1, wpos, y, x);
	}

	/* PernAngband additions */
	/* Mega^2-hack -- destroying the Stormbringer gives it us! */
	else if (strstr((r_name + r_ptr->name),"Stormbringer"))
	{
		/* Get local object */
		qq_ptr = &forge;

		/* Prepare to make the Stormbringer */
		invcopy(qq_ptr, lookup_kind(TV_SWORD, SV_BLADE_OF_CHAOS));

		/* Megahack -- specify the ego */
		qq_ptr->name2 = EGO_STORMBRINGER;

		/* Piece together a 32-bit random seed */
		qq_ptr->name3 = rand_int(0xFFFF) << 16;
		qq_ptr->name3 += rand_int(0xFFFF);

		apply_magic(wpos, qq_ptr, -1, FALSE, FALSE, FALSE, FALSE, FALSE);
		qq_ptr->level = 0;

		qq_ptr->ident |= ID_CURSED;

		/* hack for a good result */
		qq_ptr->to_h = 17 + rand_int(14);
		qq_ptr->to_d = 17 + rand_int(14);

		/* Drop it in the dungeon */
		drop_near(qq_ptr, -1, wpos, y, x);
	}
        /* Raal's Tomes of Destruction drop a Raal's Tome of Destruction */
//        else if ((strstr((r_name + r_ptr->name),"Raal's Tome of Destruction")) && (rand_int(100) < 20))
        else if ((strstr((r_name + r_ptr->name),"Raal's Tome of Destruction")) && (magik(1)))
	{
		/* Get local object */
		qq_ptr = &forge;

                /* Prepare to make a Raal's Tome of Destruction */
//                invcopy(qq_ptr, lookup_kind(TV_MAGIC_BOOK, 8));
		/* Make a Tome of the Hellflame (Udun) */
                invcopy(qq_ptr, lookup_kind(TV_BOOK, 11));

		/* Drop it in the dungeon */
                drop_near(qq_ptr, -1, wpos, y, x);
	}

	/* Wyrms have a chance of dropping The Amulet of Grom, the Wyrm Hunter: -C. Blue */
	else if ((r_ptr->flags3 & RF3_DRAGON) & !pvp)
	{
		bool pfft = TRUE;
		a_idx = ART_AMUGROM;
		a_ptr = &a_info[a_idx];

		/* don't allow duplicates */
		if (a_ptr->cur_num) pfft = FALSE;
		/* only powerful wyrms may have a chance of dropping it */
		if (m_ptr->maxhp < 3500) pfft = FALSE;/* Dracolisk/Dracolich have 3500, Wyrms start at 4000 */
		else if ((m_ptr->maxhp < 6000) && rand_int(80)) pfft = FALSE;/* strong wyrms at 6000+ */
		else if ((m_ptr->maxhp >= 6000) && (m_ptr->maxhp < 10000) && rand_int(60)) pfft = FALSE;
		else if ((m_ptr->maxhp >= 10000) && rand_int(40)) pfft = FALSE;/* gwop ^^ */
#if 1
		if (pfft && !cfg.arts_disabled) {
			a_idx = ART_AMUGROM;
			a_ptr = &a_info[a_idx];
            		chance = 0;
                        I_kind = 0;

			/* Get local object */
			qq_ptr = &forge;
			/* Wipe the object */
			object_wipe(qq_ptr);

			/* Acquire the "kind" index */
			I_kind = lookup_kind(a_ptr->tval, a_ptr->sval);
			/* Create the artifact */
			invcopy(qq_ptr, I_kind);
			/* Save the name */
			qq_ptr->name1 = a_idx;

			/* Mega-Hack -- Actually create the amulet of Grom */
			apply_magic(wpos, qq_ptr, -2, TRUE, TRUE, TRUE, FALSE, TRUE);

			a_info[a_idx].cur_num++;
			/* Drop the artifact from heaven */
#ifdef PRE_OWN_DROP_CHOSEN
			qq_ptr->level = 0;
			qq_ptr->owner = p_ptr->id;
			qq_ptr->mode = p_ptr->mode;
#endif
			drop_near(qq_ptr, -1, wpos, y, x);
		}
#endif
	}


//        if((!force_coin)&&(randint(100)<50)) place_corpse(m_ptr);

	/* Only process "Quest Monsters" */
	if (!(r_ptr->flags1 & RF1_QUESTOR)) return;

	/* Hack -- Mark quests as complete */
	for (i = 0; i < MAX_Q_IDX; i++)
	{
		/* Hack -- note completed quests */
		if (q_list[i].level == r_ptr->level) q_list[i].level = 0;

		/* Count incomplete quests */
		if (q_list[i].level) total++;
	}


	/* Need some stairs */
	if (total)
	{
		/* Stagger around */
		while (!cave_valid_bold(zcave, y, x))
		{
			int d = 1;

			/* Pick a location */
			scatter(wpos, &ny, &nx, y, x, d, 0);

			/* Stagger */
			y = ny; x = nx;
		}

		/* Delete any old object XXX XXX XXX */
		delete_object(wpos, y, x, TRUE);

		/* Explain the stairway */
		msg_print(Ind, "A magical stairway appears...");

		/* Access the grid */
		c_ptr = &zcave[y][x];

		/* Create stairs down */
		c_ptr->feat = FEAT_MORE;

		/* Note the spot */
		note_spot_depth(wpos, y, x);

		/* Draw the spot */
		everyone_lite_spot(wpos, y, x);

		/* Remember to update everything */
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
	}

        FREE(m_ptr->r_ptr, monster_race);
}

/* FIXME: this function is known to be bypassable by nominally
 * 'party-owning'.
 */
void kill_house_contents(house_type *h_ptr){
	struct worldpos *wpos = &h_ptr->wpos;
	object_type *o_ptr;
	int i;

#ifdef USE_MANG_HOUSE
	if(h_ptr->flags&HF_RECT){
		int sy,sx,ey,ex,x,y;
		sy=h_ptr->y+1;
		sx=h_ptr->x+1;
		ey=h_ptr->y+h_ptr->coords.rect.height-1;
		ex=h_ptr->x+h_ptr->coords.rect.width-1;
		for(y=sy;y<ey;y++){
			for(x=sx;x<ex;x++){
				delete_object(wpos,y,x, TRUE);
			}
		}

		/* make sure no player gets stuck by being inside while it's sold - C. Blue */
		for (i = 1; i <= NumPlayers; i++) {
			if (inarea(&Players[i]->wpos, wpos) &&
			    Players[i]->py >= sy && Players[i]->py <= ey &&
			    Players[i]->px >= sx && Players[i]->px <= ex)
				teleport_player_force(i, 1);
		}
	} else {
		fill_house(h_ptr, FILL_CLEAR, NULL);
		/* Polygonal house */
	}
#endif	// USE_MANG_HOUSE

#ifndef USE_MANG_HOUSE_ONLY
	for (i = 0; i < h_ptr->stock_num; i++) {
		o_ptr = &h_ptr->stock[i];
		if (o_ptr->k_idx && true_artifact_p(o_ptr))
			handle_art_d(o_ptr->name1);
		invwipe(o_ptr);
	}
	h_ptr->stock_num = 0;
#endif	// USE_MANG_HOUSE_ONLY

#ifdef HOUSE_PAINTING
	/* Remove house paint */
	h_ptr->colour = 0;
	fill_house(h_ptr, FILL_UNPAINT, NULL);
#endif
}

void kill_houses(int id, byte type){
	int i;
	for(i=0;i<num_houses;i++){
		struct dna_type *dna=houses[i].dna;
		if(dna->owner==id && dna->owner_type==type){
			dna->owner=0L;
			dna->creator=0L;
			dna->a_flags=ACF_NONE;
			kill_house_contents(&houses[i]);
		}
	}
}

/* XXX maybe this function can delete the objects
 * to prevent 'house-owner char cheeze'	- Jir -
 */
void kill_objs(int id){
	int i;
	object_type *o_ptr;
	for(i=0;i<o_max;i++){
		o_ptr=&o_list[i];
		if(!o_ptr->k_idx) continue;
		if(o_ptr->owner==id){
			o_ptr->owner=MAX_ID+1;
			/* o_ptr->mode = 0; <- makes everlasting items usable! bad! */
		}
	}
}


#define QUIT_BAN_NONE	0
#define QUIT_BAN_ROLLER	1
#define QUIT_BAN_ALL	2

/* This function prevents DoS attack using suicide */
/* ;) DoS... its just annoying. hehe */
static void check_roller(int Ind)
{
	player_type *p_ptr = Players[Ind];
	time_t now = time(&now);

	/* This was necessary ;) */
	if(is_admin(p_ptr)) return;

	if (!cfg.quit_ban_mode) return;

	if (cfg.quit_ban_mode == QUIT_BAN_ROLLER)
	{
		/* (s)he should have played somewhat */
		if (p_ptr->max_exp) return;

		/* staying for more than 60 seconds? */
		if (now - lookup_player_laston(p_ptr->id) > 60) return;
		
		/* died to a townie? 
		if (p_ptr->ghost) return; */
	}

	/* ban her/him for 1 min */
	add_banlist(Ind, 1);
}


static void check_killing_reward(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* reward top gladiators */
	if (p_ptr->kills >= 10) {
		p_ptr->kills -= 10; /* reset! */
		msg_broadcast_format(Ind, "\374\377y** %s vanquished 10 opponents! **", p_ptr->name);
		msg_print(Ind, "\375\377G* Another 10 aggressors have fallen by your hands! *");
		msg_print(Ind, "\375\377G* You received a reward! *");
                give_reward(Ind, RESF_MID, "Gladiator's reward", 1, 0);
	}
}

/* deletes a ghost-dead player, cleans up his business, and disconnects him */
static void erase_player(int Ind, int death_type, bool static_floor) {
	player_type *p_ptr = Players[Ind];
	char buf[1024];

	kill_houses(p_ptr->id, OT_PLAYER);
	rem_quest(p_ptr->quest_id);
	kill_objs(p_ptr->id);
	p_ptr->death = TRUE;

#ifdef AUCTION_SYSTEM
	auction_player_death(p_ptr->id);
#endif

	/* Remove him from his party/guild */
	if (p_ptr->party) {
		/* He leaves */
		party_leave(Ind);
	}
	if (p_ptr->guild) {
		if ((guilds[p_ptr->guild].flags & GFLG_AUTO_READD))
			acc_set_guild(p_ptr->accountname, p_ptr->guild);
		guild_leave(Ind);
	}

	/* Ghosts dont static the lvl if under cfg_preserve_death_level ft. DEG */
	if (static_floor && getlevel(&p_ptr->wpos) < cfg.preserve_death_level) {
		struct worldpos old_wpos;
		/*
		 * HACK - Change the wpos temporarily so that new_players_on_depth
		 * won't think that the player is on the level - mikaelh
		 */
		wpcopy(&old_wpos, &p_ptr->wpos);
		p_ptr->wpos.wz++;
		new_players_on_depth(&old_wpos, -1, TRUE);
		p_ptr->wpos.wz--;
	}

	buffer_account_for_event_deed(p_ptr, death_type);

	/* Remove him from the player name database */
	delete_player_name(p_ptr->name);

	/* Put him on the high score list */
	if(!p_ptr->noscore && !(p_ptr->mode & (MODE_PVP | MODE_EVERLASTING)))
		add_high_score(Ind);

	/* Format string for death reason */
	if (death_type == DEATH_QUIT) strcpy(buf, "Committed suicide");
	else if (!strcmp(p_ptr->died_from, "It") || !strcmp(p_ptr->died_from, "Insanity") || p_ptr->image)
		snprintf(buf, sizeof(buf), "Killed by %s (%ld pts)", p_ptr->really_died_from, total_points(Ind));
	else snprintf(buf, sizeof(buf), "Killed by %s (%ld pts)", p_ptr->died_from, total_points(Ind));

	/* Get rid of him */
	Destroy_connection(p_ptr->conn, buf);
}

/*
 * Handle the death of a player and drop their stuff.
 */
 
 /* 
  HACKED to handle fruit bat 
  changed so players remain in the team when killed
  changed so when leader ghosts perish the team is disbanded
  -APD-
 */
 
void player_death(int Ind)
{
	player_type *p_ptr = Players[Ind], *p_ptr2 = NULL;
	int Ind2;
	object_type *o_ptr;
	monster_type *m_ptr;
	dungeon_type *d_ptr = getdungeon(&p_ptr->wpos);
	dun_level *l_ptr = getfloor(&p_ptr->wpos);
	char buf[1024], o_name[ONAME_LEN], m_name_extra[MNAME_LEN], msg_layout = 'a';
	int i, inventory_loss = 0, equipment_loss = 0, k, j, tries = 0;
//	int inven_sort_map[INVEN_TOTAL];
	//wilderness_type *wild;
	bool hell = TRUE, secure = FALSE, ge_secure = FALSE, pvp = ((p_ptr->mode & MODE_PVP) != 0);
	cptr titlebuf;
	int death_type = -1; /* keep track of the way (s)he died, for buffer_account_for_event_deed() */
	bool world_broadcast = TRUE, just_fruitbat_transformation = (p_ptr->fruit_bat == -1);

	/* Amulet of immortality prevents death */
	o_ptr = &p_ptr->inventory[INVEN_NECK];
	/* Skip empty items */
	if (o_ptr->k_idx && o_ptr->tval == TV_AMULET &&
	    (o_ptr->sval == SV_AMULET_INVINCIBILITY || o_ptr->sval == SV_AMULET_INVULNERABILITY)) {
		if (just_fruitbat_transformation) p_ptr->fruit_bat = 0;
		return;
	}

	/* prepare player's title */
	if (p_ptr->lev < 60)
		titlebuf = player_title[p_ptr->pclass][((p_ptr->lev)/5 < 10)?(p_ptr->lev)/5 : 10][1 - p_ptr->male];
	else
		titlebuf = player_title_special[p_ptr->pclass][(p_ptr->lev < PY_MAX_PLAYER_LEVEL)? (p_ptr->lev - 60)/10 : 4][1 - p_ptr->male];

	break_cloaking(Ind, 0);
	break_shadow_running(Ind);
	stop_precision(Ind);
	stop_shooting_till_kill(Ind);

	if (just_fruitbat_transformation) {
		p_ptr->death = FALSE;
		p_ptr->update |= (PU_BONUS);
		p_ptr->redraw |= (PR_HP | PR_GOLD | PR_BASIC | PR_DEPTH);
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);
		p_ptr->window |= (PW_INVEN | PW_EQUIP);
		p_ptr->fruit_bat = 2;
		calc_hitpoints(Ind);

		/* Tell the players */
		snprintf(buf, sizeof(buf), "\374\377o%s was turned into a fruit bat by %s!", p_ptr->name, p_ptr->died_from);
		/* handle the secret_dungeon_master option */
		if ((!p_ptr->admin_dm) || (!cfg.secret_dungeon_master)) {
#ifdef TOMENET_WORLDS
			if (cfg.worldd_pdeath && world_broadcast) world_msg(buf);
#endif
			msg_broadcast(Ind, buf);
		}
		return;
	}

#ifdef RPG_SERVER
	if (p_ptr->wpos.wz != 0) {
		for (i = m_top-1; i >= 0; i--) {
			monster_type *m_ptr = &m_list[i];
			if (m_ptr->owner == p_ptr->id && m_ptr->pet) {
				m_ptr->pet = 0; //default behaviour!
				m_ptr->owner = 0;
				i = -1;
			}
		}
	}
#endif

	/* very very rare case, but this can happen(eg. starvation) */
	if (p_ptr->store_num != -1) {
#ifdef PLAYER_STORES
		if (p_ptr->store_num <= -2) {
			/* unlock the fake store again which we had occupied */
			fake_store_visited[-2 - p_ptr->store_num] = 0;
		}
#endif
		handle_store_leave(Ind);
		Send_store_kick(Ind);
	}

	if (d_ptr && (d_ptr->flags2 & DF2_NO_DEATH)) secure = TRUE;

	if (p_ptr->wpos.wx == WPOS_IRONDEEPDIVE_X &&
	    p_ptr->wpos.wy == WPOS_IRONDEEPDIVE_Y &&
	    p_ptr->wpos.wz != 0
	    && !is_admin(p_ptr)) {
		for (i = 0; i < 20; i++) {
			if (deep_dive_level[i] >= ABS(p_ptr->wpos.wz) || deep_dive_level[i] == -1) continue;
			for (j = 20 - 1; j > i; j--) {
				deep_dive_level[j] = deep_dive_level[j - 1];
				strcpy(deep_dive_name[j], deep_dive_name[j - 1]);
			}
			deep_dive_level[i] = ABS(p_ptr->wpos.wz);
			strcpy(deep_dive_name[i], p_ptr->name);
			break;
		}

		if (i < 5)
			msg_broadcast_format(0, "\374\377a%s reached floor %d in the Ironman Deep Dive challenge, placing %d%s!",
			    p_ptr->name, ABS(p_ptr->wpos.wz), i + 1, i == 0 ? "st" : (i == 1 ? "nd" : (i == 2 ? "rd" : "th")));
		else
			msg_broadcast_format(0, "\374\377a%s reached floor %d in the Ironman Deep Dive challenge!",
			    p_ptr->name, ABS(p_ptr->wpos.wz));
		l_printf("%s \\{s%s (%d) reached floor %d in the Ironman Deep Dive challenge\n", showdate(), p_ptr->name, p_ptr->lev, ABS(p_ptr->wpos.wz));
	}

	if (ge_special_sector &&
	    (p_ptr->wpos.wx == WPOS_ARENA_X && p_ptr->wpos.wy == WPOS_ARENA_Y &&
	    p_ptr->wpos.wz == WPOS_ARENA_Z)) {
		secure = TRUE;
		ge_secure = TRUE; /* extra security for global events */
	}

	if (pvp) {
		secure = FALSE;
		msg_layout = 'L';
	}

	/* Hack -- amulet of life saving */
	if (p_ptr->alive && (secure ||
	    (p_ptr->inventory[INVEN_NECK].k_idx &&
	    p_ptr->inventory[INVEN_NECK].sval == SV_AMULET_LIFE_SAVING))) {
		s_printf("%s (%d) was pseudo-killed by %s for %d damage at %d, %d, %d.\n", p_ptr->name, p_ptr->lev, p_ptr->really_died_from, p_ptr->deathblow, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);

		if (!secure) {
			msg_print(Ind, "\377oYour amulet shatters into pieces!");
			inven_item_increase(Ind, INVEN_NECK, -99);
			//inven_item_describe(Ind, INVEN_NECK);
			inven_item_optimize(Ind, INVEN_NECK);
		}

		/* Cure him from various maladies */
//		p_ptr->black_breath = FALSE;
		if (p_ptr->image) (void)set_image(Ind, 0);
		if (p_ptr->blind) (void)set_blind(Ind, 0);
		if (p_ptr->paralyzed) (void)set_paralyzed(Ind, 0);
		if (p_ptr->confused) (void)set_confused(Ind, 0);
		if (p_ptr->poisoned) (void)set_poisoned(Ind, 0, 0);
		if (p_ptr->stun) (void)set_stun(Ind, 0);
		if (p_ptr->cut) (void)set_cut(Ind, 0, 0);
		/* if (p_ptr->food < PY_FOOD_ALERT) */
		(void)set_food(Ind, PY_FOOD_FULL - 1);

		/* Teleport him */
		teleport_player(Ind, 200, TRUE);

		/* Remove the death flag */
		p_ptr->death = FALSE;
//		ptr->ghost = 0;

		/* Give him his hit points back */
		p_ptr->chp = p_ptr->mhp;
		p_ptr->chp_frac = 0;

		if (secure) {
			p_ptr->new_level_method = (p_ptr->wpos.wz > 0 ? LEVEL_RECALL_DOWN : LEVEL_RECALL_UP);
			p_ptr->recall_pos.wx = p_ptr->wpos.wx;
			p_ptr->recall_pos.wy = p_ptr->wpos.wy;
			p_ptr->recall_pos.wz = 0;
			if (ge_secure) {
				k = 0;
				/* reset the monster :D */
			        for (i = 1; i < m_max; i++) {
			                m_ptr = &m_list[i];
					if (m_ptr->wpos.wx == p_ptr->wpos.wx && m_ptr->wpos.wy == p_ptr->wpos.wy && m_ptr->wpos.wz == p_ptr->wpos.wz) {
						k = m_ptr->r_idx;
						j = m_ptr->ego;
						monster_desc(0, m_name_extra, i, 0x00);
						m_name_extra[0] = toupper(m_name_extra[0]);
						wipe_m_list(&p_ptr->wpos);
						summon_override_checks = SO_ALL & ~SO_PROTECTED;
#if 1 /* GE_ARENA_ALLOW_EGO */
						while (!summon_detailed_one_somewhere(&p_ptr->wpos, k, j, FALSE, 101)
						    && (++tries < 1000));
#else
						while (!summon_specific_race_somewhere(&p_ptr->wpos, k, 100, 1)
						    && (++tries < 1000));
#endif
						summon_override_checks = SO_NONE;
						break;
					}
				}
				if (k) { /* usual */
					msg_broadcast_format(0, "\376\377A** %s has defeated %s! **", m_name_extra, p_ptr->name);
					s_printf("EVENT_RESULT: %s has defeated %s (%d) (%d damage).\n", m_name_extra, p_ptr->name, p_ptr->lev, p_ptr->deathblow);
				} else { /* can happen if monster dies first, then player dies to monster DoT */
					msg_broadcast_format(0, "\376\377A** %s didn't survive! **", p_ptr->name);
					s_printf("EVENT_RESULT: %s (%d) was defeated (%d damage).\n", p_ptr->name, p_ptr->lev, p_ptr->deathblow);
				}
				recall_player(Ind, "\377oYou die.. at least it felt like you did..!");
			}
			else recall_player(Ind, "\377oYou die.. but your life was secured here!");

			p_ptr->safe_sane = TRUE;
			/* Apply small penalty for death */
			if (!ge_secure) { /* except if we were in a special event that protects */
#ifndef ARCADE_SERVER
				p_ptr->au = p_ptr->au * 4 / 5;
				p_ptr->max_exp = (p_ptr->max_exp * 4 + 1) / 5; /* never drop below 1! (Highlander Tournament exploit) */
				p_ptr->exp = p_ptr->max_exp;
#endif
			} else {
				if (p_ptr->csane <= 10) p_ptr->csane = 10; /* just something, paranoia */
			}
			check_experience(Ind);

			/* update stats */
			p_ptr->update |= PU_SANITY;
			update_stuff(Ind);
			p_ptr->safe_sane = FALSE;
			/* Redraw */
			p_ptr->redraw |= (PR_BASIC);
			/* Update */
			p_ptr->update |= (PU_BONUS);
		} else {
			/* Went mad? */
			if (p_ptr->csane < 0) p_ptr->csane = 0;
		}

		/* Wow! You may return!! */
		if (!ge_secure) p_ptr->soft_deaths++;
		return;
	}

	if((!(p_ptr->mode & MODE_NO_GHOST)) && !cfg.no_ghost && !pvp){
		if(!p_ptr->wpos.wz || !(d_ptr->flags2 & (DF2_HELL | DF2_IRON)))
			hell = FALSE;
	}

	if ((Ind2 = get_esp_link(Ind, LINKF_PAIN, &p_ptr2))) {
		strcpy(p_ptr2->died_from, p_ptr->died_from);
		if (!p_ptr2->ghost)
		{
			strcpy(p_ptr2->died_from_list, p_ptr->died_from);
			p_ptr2->died_from_depth = getlevel(&p_ptr2->wpos);
			/* Hack to remember total winning */
                        if (p_ptr2->total_winner) strcat(p_ptr2->died_from_list, "\001");
		}
		/* new: 50-50 chance to die for linked mind */
		bypass_invuln = TRUE;
		if (p_ptr2->chp >= 30)
			take_hit(Ind2, p_ptr2->chp - 20 + rand_int(42), p_ptr->died_from, 0);
		else
			take_hit(Ind2, p_ptr2->chp - 2 + rand_int(6), p_ptr->died_from, 0);
		bypass_invuln = FALSE;
	}

	/* Morgoth's level might be NO_GHOST! */
	if (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_GHOST)) hell = TRUE;

	/* For global events (Highlander Tournament) */
	/* either instakill in sector 0,0... */
	if (p_ptr->global_event_temp & PEVF_NOGHOST_00) hell = TRUE;
	/* or instead teleport them to surface */
	/* added wpos checks due to weirdness. -Molt */
	if(p_ptr->wpos.wx != WPOS_SECTOR00_X && p_ptr->wpos.wy != WPOS_SECTOR00_Y && (p_ptr->global_event_temp & PEVF_SAFEDUN_00)) {
		s_printf("Somethin weird with %s. GET is %d\n", p_ptr->name, p_ptr->global_event_temp);
		msg_broadcast(0, "Uh oh, somethin's not right here.");
	}
	if ((p_ptr->global_event_temp & PEVF_SAFEDUN_00) && (p_ptr->csane >= 0) && p_ptr->wpos.wx == WPOS_SECTOR00_X && p_ptr->wpos.wy == WPOS_SECTOR00_Y && p_ptr->wpos.wz != 0) {
		s_printf("DEBUG_TOURNEY: player %s revived.\n", p_ptr->name);
		s_printf("%s (%d) was pseudo-killed by %s for %d damage at %d, %d, %d.\n", p_ptr->name, p_ptr->lev, p_ptr->really_died_from, p_ptr->deathblow, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);

		if (p_ptr->poisoned) (void)set_poisoned(Ind, 0, 0);
		if (p_ptr->cut) (void)set_cut(Ind, 0, 0);
		(void)set_food(Ind, PY_FOOD_FULL - 1);

		if (!sector00downstairs) p_ptr->global_event_temp &= ~PEVF_SAFEDUN_00; /* no longer safe from death */
		p_ptr->recall_pos.wx = 0;
		p_ptr->recall_pos.wy = 0;
		p_ptr->recall_pos.wz = 0;
		p_ptr->global_event_temp |= PEVF_PASS_00; /* pass through sector00separation */
		p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
		recall_player(Ind, "");

		/* Allow him to find the stairs quickly for re-entering highlander dungeon */
		wiz_lite(Ind);

		/* Teleport him */
		teleport_player(Ind, 200, TRUE);
		/* Remove the death flag */
		p_ptr->death = FALSE;
//		p_ptr->ghost = 0;
		/* Give him his hit points back */
		p_ptr->chp = p_ptr->mhp;
		p_ptr->chp_frac = 0;

		/* Redraw */
		p_ptr->redraw |= (PR_BASIC);
		/* Update */
		p_ptr->update |= (PU_BONUS);

		/* Inform him about his situation */
		if (!sector00downstairs) msg_print(Ind, "\377oYou were defeated too early and have to sit out the remaining time!");
		else msg_print(Ind, "\377oYou were defeated too early, find the staircase and re-enter the dungeon!");

		p_ptr->soft_deaths++;
		return;
	}

	/* Players of level cfg.nodrop [5] will die a no-ghost death.
	   This should clarify the situation for newbies and avoid them
	   getting confused and/or lacking their startup equipment.
	   On the other hand, it's confusing to ghost-die when you know
	   you chose everlasting +_+ */
#if 0 /* disable newbies-lvl explanation msg (much below) iff this is on */
	if (p_ptr->max_plv < cfg.newbies_cannot_drop) hell = TRUE;
#endif

#ifdef USE_SOUND_2010
	/* don't play a sfx for mere suicide */
	if (p_ptr->alive) {
		if (p_ptr->male) sound(Ind, "death_male", "death", SFX_TYPE_MISC, TRUE);
		else sound(Ind, "death_female", "death", SFX_TYPE_MISC, TRUE);
	}
#else
	/* don't play a sfx for mere suicide */
	if (p_ptr->alive) sound(Ind, SOUND_DEATH);
#endif

	/* Drop gold if player has any -------------------------------------- */
	if (p_ptr->alive && p_ptr->au) {
		/* Put the player's gold in the overflow slot */
		invcopy(&p_ptr->inventory[INVEN_PACK], lookup_kind(TV_GOLD, 9));
		/* Change the mode of the gold accordingly */
		p_ptr->inventory[INVEN_PACK].mode = p_ptr->mode;
		p_ptr->inventory[INVEN_PACK].owner = p_ptr->id; /* hack */

		/* Drop no more than 32000 gold */
//		if (p_ptr->au > 32000) p_ptr->au = 32000;
		/* (actually, this if-clause is not necessary) */
		s_printf("gold_lost: carried %d, remaining ", p_ptr->au);
		if (p_ptr->au <= 50000) ;
		else if (p_ptr->au <= 500000) p_ptr->au = (((p_ptr->au) * 100) / (100 + ((p_ptr->au - 50000) / 4500)));
		else p_ptr->au /= 2;

		if(p_ptr->max_plv >= cfg.newbies_cannot_drop){
			/* Set the amount */
			p_ptr->inventory[INVEN_PACK].pval = p_ptr->au;
			s_printf("%d.\n", p_ptr->au);
		} else {
			invwipe(&p_ptr->inventory[INVEN_PACK]);
			s_printf("none.\n");
		}

		/* No more gold */
		p_ptr->au = 0;
	}

	/* Drop/lose items -------------------------------------------------- */
	/* Setup the sorter */
	ang_sort_comp = ang_sort_comp_value;
	ang_sort_swap = ang_sort_swap_value;
	/* Remember original position before sorting */
	for (i = 0; i < INVEN_TOTAL; i++) p_ptr->inventory[i].inven_order = i;
	/* Sort the player's inventory according to value */
	ang_sort(Ind, p_ptr->inventory, NULL, INVEN_TOTAL);

	/* Starting with the most valuable, drop things one by one */
	for (i = 0; i < INVEN_TOTAL; i++) {
		bool away = FALSE, item_lost = FALSE;
		int real_pos = p_ptr->inventory[i].inven_order;

		o_ptr = &p_ptr->inventory[i];
		/* Make sure we have an object */
		if (o_ptr->k_idx == 0) continue;

		/* If we committed suicide, only drop artifacts */
//			if (!p_ptr->alive && !artifact_p(o_ptr)) continue;
		if (!p_ptr->alive) {
			if (!true_artifact_p(o_ptr)) continue;

			/* hack -- total winners do not drop artifacts when they suicide */
			//		if (!p_ptr->alive && p_ptr->total_winner && artifact_p(&p_ptr->inventory[i])) 

			/* Artifacts cannot be dropped after all */
			/* Don't litter Valinor -- Ring of Phasing must be destroyed anyways */
			if ((cfg.anti_arts_hoard) || (getlevel(&p_ptr->wpos) == 200)) {
				/* set the artifact as unfound */
				handle_art_d(o_ptr->name1);

				/* Don't drop the artifact */
				continue;
			}
		}

#ifdef DEATH_PACK_ITEM_LOST
		if ((real_pos < INVEN_PACK) && magik(DEATH_PACK_ITEM_LOST) && (inventory_loss < 4)) {
			inventory_loss++;
			item_lost = TRUE;
		}
#endif

#ifdef DEATH_EQ_ITEM_LOST
		if ((real_pos > INVEN_PACK) && magik(DEATH_EQ_ITEM_LOST) && (equipment_loss < 1)) {
			item_lost = TRUE;
			equipment_loss++;
		}
#endif

		if (!is_admin(p_ptr) && !p_ptr->inval && (p_ptr->max_plv >= cfg.newbies_cannot_drop) &&
		    /* Don't drop Morgoth's crown */
		    !(o_ptr->name1 == ART_MORGOTH) && !(o_ptr->name1 == ART_GROND)) {
#ifdef DEATH_ITEM_SCATTER
			/* Apply penalty of death */
			if (!artifact_p(o_ptr) && magik(DEATH_ITEM_SCATTER) && !item_lost)
				away = TRUE;
			else
#endif	/* DEATH_ITEM_SCATTER */
			{
				if (!item_lost) {
					if (p_ptr->wpos.wz) o_ptr->marked2 = ITEM_REMOVAL_NEVER;
					else if (istown(&p_ptr->wpos)) o_ptr->marked2 = ITEM_REMOVAL_DEATH_WILD;/* don't litter towns for long */
					else o_ptr->marked2 = ITEM_REMOVAL_LONG_WILD;/* don't litter wilderness eternally ^^ */

					/* Drop this one */
					away = drop_near(o_ptr, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px)
						<= 0 ? TRUE : FALSE;
				} else {
					object_desc(Ind, o_name, o_ptr, TRUE, 3);
//					if (object_value_real(0, o_ptr) >= 10000)
					s_printf("item_lost: %s (slot %d)\n", o_name, real_pos);
					if (true_artifact_p(o_ptr)) {
						/* set the artifact as unfound */
						handle_art_d(o_ptr->name1);
					}
				}
			}

			if (away) {
				int o_idx = 0, x1, y1, try = 500;
				cave_type **zcave;
				if ((zcave = getcave(&p_ptr->wpos))) /* this should never.. */
					while (o_idx <= 0 && try--) {
						x1 = rand_int(p_ptr->cur_wid);
						y1 = rand_int(p_ptr->cur_hgt);

						if (!cave_clean_bold(zcave, y1, x1)) continue;
						if (p_ptr->wpos.wz) o_ptr->marked2 = ITEM_REMOVAL_NEVER;
						else if (istown(&p_ptr->wpos)) o_ptr->marked2 = ITEM_REMOVAL_DEATH_WILD;/* don't litter towns for long */
						else o_ptr->marked2 = ITEM_REMOVAL_LONG_WILD;/* don't litter wilderness eternally ^^ */
						o_idx = drop_near(o_ptr, 0, &p_ptr->wpos, y1, x1);
					}
			}
		}
		else if (true_artifact_p(o_ptr)) {
			/* set the artifact as unfound */
			handle_art_d(o_ptr->name1);
		}

		/* No more item */
		invwipe(o_ptr);
	}

	/* Get rid of him if he's a ghost or suffers a no-ghost death */
	if ((p_ptr->ghost || (hell && p_ptr->alive)) ||
	    (streq(p_ptr->died_from, "Insanity")) ||
	    (streq(p_ptr->died_from, "Indecisiveness")) ||
	    ((p_ptr->lives == 1+1) && cfg.lifes && p_ptr->alive &&
	    !(p_ptr->mode & MODE_EVERLASTING))) {
		/* Tell players */
		if (streq(p_ptr->died_from, "Insanity")) {
			/* Tell him */
			msg_print(Ind, "\374\377RYou die.");
	//		msg_print(Ind, NULL);
			msg_format(Ind, "\374\377%c**\377rYou have been destroyed by \377oI\377Gn\377bs\377Ba\377sn\377Ri\377vt\377yy\377r.\377%c**", msg_layout, msg_layout);

s_printf("CHARACTER_TERMINATION: INSANITY race=%s ; class=%s\n", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title);

			if (cfg.unikill_format)
			snprintf(buf, sizeof(buf), "\374\377%c**\377r%s %s (%d) was destroyed by \377m%s\377r.\377%c**", msg_layout, titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
			else
			snprintf(buf, sizeof(buf), "\374\377%c**\377r%s (%d) was destroyed by \377m%s\377r.\377%c**", msg_layout, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
			s_printf("%s (%d) was destroyed by %s for %d damage at %d, %d, %d.\n", p_ptr->name, p_ptr->lev, p_ptr->died_from, p_ptr->deathblow, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			if (!strcmp(p_ptr->died_from, "It") || !strcmp(p_ptr->died_from, "Insanity") || p_ptr->image)
				s_printf("(%s was really destroyed by %s.)\n", p_ptr->name, p_ptr->really_died_from);

#if CHATTERBOX_LEVEL > 2
			if (strstr(p_ptr->died_from, "Farmer Maggot's dog") && magik(50)) {
				msg_broadcast(Ind, "Suddenly a thought comes to your mind:");
				msg_broadcast(0, "Who let the dogs out?");
			}
			else if (p_ptr->last_words)
			{
				char death_message[80];
    
        			(void)get_rnd_line("death.txt", 0, death_message, 80);
				msg_print(Ind, death_message);
			}
#endif	// CHATTERBOX_LEVEL

			death_type = DEATH_INSANITY;
			if (p_ptr->ghost) death_type = DEATH_GHOST;

			Send_chardump(Ind, "-death");
			Net_output1(Ind);
		} else if (p_ptr->ghost) {
			/* Tell him */
			msg_format(Ind, "\374\377a**\377rYour ghost was destroyed by %s.\377a**", p_ptr->died_from);

s_printf("CHARACTER_TERMINATION: GHOSTKILL race=%s ; class=%s\n", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title);

			if (cfg.unikill_format)
			snprintf(buf, sizeof(buf), "\374\377a**\377r%s %s's (%d) ghost was destroyed by %s.\377a**", titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from);
			else
			snprintf(buf, sizeof(buf), "\374\377a**\377r%s's (%d) ghost was destroyed by %s.\377a**", p_ptr->name, p_ptr->lev, p_ptr->died_from);
			s_printf("%s's (%d) ghost was destroyed by %s for %d damage on %d, %d, %d.\n", p_ptr->name, p_ptr->lev, p_ptr->died_from, p_ptr->deathblow, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			if (!strcmp(p_ptr->died_from, "It") || !strcmp(p_ptr->died_from, "Insanity") || p_ptr->image)
				s_printf("(%s's ghost was really destroyed by %s.)\n", p_ptr->name, p_ptr->really_died_from);

#if CHATTERBOX_LEVEL > 2
			if (p_ptr->last_words)
			{
				char death_message[80];
    
        			(void)get_rnd_line("death.txt", 0, death_message, 80);
				msg_print(Ind, death_message);
			}
#endif	// CHATTERBOX_LEVEL

			death_type = DEATH_GHOST;
		} else {
			/* Tell him */
			msg_print(Ind, "\374\377RYou die.");
	//		msg_print(Ind, NULL);
#ifdef MORGOTH_FUNKY_KILL_MSGS /* Might add some atmosphere? (lol) - C. Blue */
			if (!strcmp(p_ptr->died_from, "Morgoth, Lord of Darkness")) {
				char funky_msg[20];
				switch (randint(5)) {
				case 1:strcpy(funky_msg,"wasted");break;
				case 2:strcpy(funky_msg,"crushed");break;
				case 3:strcpy(funky_msg,"shredded");break;
				case 4:strcpy(funky_msg,"torn up");break;
				case 5:strcpy(funky_msg,"crushed");break; /* again :) */
				}
				msg_format(Ind, "\374\377%c**\377rYou have been %s by %s.\377%c**", msg_layout, funky_msg, p_ptr->died_from, msg_layout);
				if (cfg.unikill_format) {
					snprintf(buf, sizeof(buf), "\374\377%c**\377r%s %s (%d) was %s by %s.\377%c**", msg_layout, titlebuf, p_ptr->name, p_ptr->lev, funky_msg, p_ptr->died_from, msg_layout);
				} else {
					snprintf(buf, sizeof(buf), "\374\377%c**\377r%s (%d) was %s and destroyed by %s.\377%c**", msg_layout, p_ptr->name, p_ptr->lev, funky_msg, p_ptr->died_from, msg_layout);
				}
			} else {
#endif
				if ((p_ptr->deathblow < 10) || ((p_ptr->deathblow < p_ptr->mhp / 4) && (p_ptr->deathblow < 100))
#ifdef ENABLE_DIVINE
				    || (streq(p_ptr->died_from, "Indecisiveness"))
#endif
				    || (streq(p_ptr->died_from, "Insanity"))) {
					msg_format(Ind, "\374\377%c**\377rYou have been killed by %s.\377%c**", msg_layout, p_ptr->died_from, msg_layout);
				} else if ((p_ptr->deathblow < 30) || ((p_ptr->deathblow < p_ptr->mhp / 2) && (p_ptr->deathblow < 450))) {
					msg_format(Ind, "\374\377%c**\377rYou have been annihilated by %s.\377%c**", msg_layout, p_ptr->died_from, msg_layout);
				} else {
					msg_format(Ind, "\374\377%c**\377rYou have been vaporized by %s.\377%c**", msg_layout, p_ptr->died_from, msg_layout);
				}

				if (cfg.unikill_format) {
					if ((p_ptr->deathblow < 10) || ((p_ptr->deathblow < p_ptr->mhp / 4) && (p_ptr->deathblow < 100))
#ifdef ENABLE_DIVINE
					    || (streq(p_ptr->died_from, "Indecisiveness"))
#endif
					    || (streq(p_ptr->died_from, "Insanity")))
						snprintf(buf, sizeof(buf), "\374\377%c**\377r%s %s (%d) was killed by %s.\377%c**", msg_layout, titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
					else if ((p_ptr->deathblow < 30) || ((p_ptr->deathblow < p_ptr->mhp / 2) && (p_ptr->deathblow < 450)))
						snprintf(buf, sizeof(buf), "\374\377%c**\377r%s %s (%d) was annihilated by %s.\377%c**", msg_layout, titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
					else
						snprintf(buf, sizeof(buf), "\374\377%c**\377r%s %s (%d) was vaporized by %s.\377%c**", msg_layout, titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
				} else {
					if ((p_ptr->deathblow < 10) || ((p_ptr->deathblow < p_ptr->mhp / 4) && (p_ptr->deathblow < 100))
#ifdef ENABLE_DIVINE
					    || (streq(p_ptr->died_from, "Indecisiveness"))
#endif
					    || (streq(p_ptr->died_from, "Insanity")))
						snprintf(buf, sizeof(buf), "\374\377%c**\377r%s (%d) was killed and destroyed by %s.\377%c**", msg_layout, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
					else if ((p_ptr->deathblow < 30) || ((p_ptr->deathblow < p_ptr->mhp / 2) && (p_ptr->deathblow < 450)))
						snprintf(buf, sizeof(buf), "\374\377%c**\377r%s (%d) was annihilated and destroyed by %s.\377%c**", msg_layout, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
					else
						snprintf(buf, sizeof(buf), "\374\377%c**\377r%s (%d) was vaporized and destroyed by %s.\377%c**", msg_layout, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
				}
#ifdef MORGOTH_FUNKY_KILL_MSGS
			}
#endif
			s_printf("%s (%d) was killed and destroyed by %s for %d damage at %d, %d, %d.\n", p_ptr->name, p_ptr->lev, p_ptr->died_from, p_ptr->deathblow, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			if (!strcmp(p_ptr->died_from, "It") || !strcmp(p_ptr->died_from, "Insanity") || p_ptr->image)
				s_printf("(%s was really killed and destroyed by %s.)\n", p_ptr->name, p_ptr->really_died_from);

s_printf("CHARACTER_TERMINATION: %s race=%s ; class=%s\n", pvp ? "PVP" : "NOGHOST", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title);

#if CHATTERBOX_LEVEL > 2
			if (p_ptr->last_words) {
				char death_message[80];
				(void)get_rnd_line("death.txt", 0, death_message, 80);
				msg_print(Ind, death_message);
			}
#endif	// CHATTERBOX_LEVEL
			death_type = DEATH_PERMA;
			Send_chardump(Ind, "-death");
			Net_output1(Ind);
		}
#ifdef TOMENET_WORLDS
		world_player(p_ptr->id, p_ptr->name, FALSE, TRUE);
#endif

#if (MAX_PING_RECVS_LOGGED > 0)
			/* Print last ping reception times */
			struct timeval now;
			gettimeofday(&now, NULL);
			s_printf("PINGS_RECEIVED:");
			/* Starting from latest */
			for (i = 0; i < MAX_PING_RECVS_LOGGED; i++) {
				j = (p_ptr->pings_received_head - i + MAX_PING_RECVS_LOGGED) % MAX_PING_RECVS_LOGGED;
				if (p_ptr->pings_received[j].tv_sec) {
					s_printf(" %s", timediff(&p_ptr->pings_received[j], &now));
				}
			}
			s_printf("\n");
#endif

		if ((!p_ptr->admin_dm) || (!cfg.secret_dungeon_master)){
#ifdef TOMENET_WORLDS
			if (cfg.worldd_pdeath) world_msg(buf);
#endif
			msg_broadcast(Ind, buf);
		}

		/* Reward the killer if it was a PvP-mode char */
		if (pvp) {
			int killer = name_lookup_quiet(Ind, p_ptr->really_died_from, FALSE);

			/* reward him again, making restarting easier */
			if (p_ptr->max_plv == MID_PVP_LEVEL)
				buffer_account_for_achievement_deed(p_ptr, ACHV_PVP_MID);
			if (p_ptr->max_plv == MAX_PVP_LEVEL)
				buffer_account_for_achievement_deed(p_ptr, ACHV_PVP_MAX);

			if (killer) {
				if (Players[killer]->max_plv > p_ptr->max_plv) Players[killer]->kills_lower++;
				else if (Players[killer]->max_plv < p_ptr->max_plv) Players[killer]->kills_higher++;
				else Players[killer]->kills_equal++;

#if 0 /* only reward exp for killing same level or higher players */
				if (Players[killer]->max_plv <= p_ptr->max_plv) {
					/* note how expfact isn't multiplied, so a difference between the races remains :) */
					gain_exp(killer, (player_exp[Players[killer]->lev - 1] - player_exp[Players[killer]->lev - 2]) * (1 + (p_ptr->max_plv - 5) / (Players[killer]->lev - 5)));
				}
#else /* reward exp for all player-kills, but less for killing lower level chars */
				if (Players[killer]->max_plv <= p_ptr->max_plv) {
					/* note how expfact isn't multiplied, so a difference between the races/classes remains, as usual */
					gain_exp(killer, (player_exp[Players[killer]->lev - 1] - player_exp[Players[killer]->lev - 2]) * (1 + (p_ptr->max_plv - 5) / (Players[killer]->lev - 5)));
				} else {
					/* get less exp if player was lower than killer, dropping rapidly */
					k = 2 + Players[killer]->lev - p_ptr->max_plv;//2+k; k*k+0; *12/
					gain_exp(killer, ((player_exp[Players[killer]->lev - 1] - player_exp[Players[killer]->lev - 2]) * 10) / ((k * k) - 2));
				}
#endif

				/* get victim's kills credited as own ones >:) */
				Players[killer]->kills += p_ptr->kills;
				Players[killer]->kills_lower += p_ptr->kills_lower;
				Players[killer]->kills_equal += p_ptr->kills_equal;
				Players[killer]->kills_higher += p_ptr->kills_higher;

				Players[killer]->kills++;
				Players[killer]->kills_own++;

				check_killing_reward(killer);
			} else { /* killed by monster/trap? still reward nearby pvp players! */
				int players_in_area = 0, avg_kills;
				for (i = 1; i <= NumPlayers; i++) {
					if (i == Ind) continue;
					if (is_admin(Players[i])) continue;
					if (Players[i]->conn == NOT_CONNECTED) continue;
					if (!inarea(&p_ptr->wpos, &Players[i]->wpos)) continue;
					if (!(Players[i]->mode & MODE_PVP)) continue;
					players_in_area++;
				}
				if (players_in_area) {
				
					/* give everyone a 'share' of the kills */
#if 0 /* round downwards */
					avg_kills = p_ptr->kills / players_in_area;
					/* at least give 1 kill if player had one */
					if (!avg_kills && p_ptr->kills) avg_kills = 1;
#else /* round upwards! generous */
					avg_kills = (p_ptr->kills + players_in_area - 1) / players_in_area;
#endif
					/* get victim's kills credited as own ones >:) */
					for (i = 1; i <= NumPlayers; i++) {
						if (i == Ind) continue;
						if (is_admin(Players[i])) continue;
						if (Players[i]->conn == NOT_CONNECTED) continue;
						if (!inarea(&p_ptr->wpos, &Players[i]->wpos)) continue;
						if (!(Players[i]->mode & MODE_PVP)) continue;
						Players[i]->kills += avg_kills;
						check_killing_reward(i);
					}
				}
			}
		} else { /* wasn't a pvp-mode death */
			/* copy/paste from above pvp section, just for info */
			int killer = name_lookup_quiet(Ind, p_ptr->really_died_from, FALSE);
			if (killer) {
				if (Players[killer]->max_plv > p_ptr->max_plv) Players[killer]->kills_lower++;
				else if (Players[killer]->max_plv < p_ptr->max_plv) Players[killer]->kills_higher++;
				else Players[killer]->kills_equal++;
				Players[killer]->kills += p_ptr->kills;
				Players[killer]->kills_lower += p_ptr->kills_lower;
				Players[killer]->kills_equal += p_ptr->kills_equal;
				Players[killer]->kills_higher += p_ptr->kills_higher;
				Players[killer]->kills++;
				Players[killer]->kills_own++;
			}
		}

		/* Add to legends log if he was at least 50 or died vs Morgoth */
		if (!is_admin(p_ptr)) {
			if (p_ptr->total_winner)
				l_printf("%s \\{r%s royalty %s (%d) died\n", showdate(), p_ptr->male ? "His" : "Her", p_ptr->name, p_ptr->lev);
			else if (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_GHOST))
				l_printf("%s \\{r%s (%d) died facing Morgoth\n", showdate(), p_ptr->name, p_ptr->lev);
#ifndef RPG_SERVER
			else if (p_ptr->lev >= 50)
				l_printf("%s \\{r%s (%d) died\n", showdate(), p_ptr->name, p_ptr->lev);
#else /* for RPG_SERVER, also display more trivial deaths, so people know the player is up for startup-party again */
			else if (p_ptr->lev >= 20)
				l_printf("%s \\{r%s (%d) died\n", showdate(), p_ptr->name, p_ptr->lev);
#endif
		}

		erase_player(Ind, death_type, TRUE);

		/* Done */
		return;
	}

	/* --- non-noghost-death: everlasting or more lives left --- */
	/* Add to legends log if he was a winner */
	if (p_ptr->total_winner && !is_admin(p_ptr) && p_ptr->alive)
		l_printf("%s \\{r%s (%d) lost his royal title by death\n", showdate(), p_ptr->name, p_ptr->lev);

#if 1 /* Enable, iff newbies-level leading to perma-death is disabled above. */
	if (p_ptr->max_plv < cfg.newbies_cannot_drop) {
		msg_format(Ind, "\374\377oYou died below level %d, which means that your items didn't drop.", cfg.newbies_cannot_drop);
		msg_print(Ind, "\374\377oTherefore, it's recommended to press 'Q' to suicide and start over.");
		if (p_ptr->wpos.wz < 0) msg_print(Ind, "\374\377oIf you don't like to do that, use '<' to float back to town,");
		else if (p_ptr->wpos.wz > 0) msg_print(Ind, "\374\377oIf you don't like to do that, use '>' to float back to town,");
		else if (p_ptr->wpos.wx == cfg.town_x && p_ptr->wpos.wy == cfg.town_y) msg_print(Ind, "\374\377oIf you don't like to do that, of course you may just continue");
		else msg_print(Ind, "\374\377oIf you don't like to do that, just continue by flying back to town");
		msg_print(Ind, "\374\377oand enter the temple (\377g4\377o) to be revived and handed some money.");
	}
#endif

	/* Tell everyone he died */
	if (p_ptr->alive) {
		if ((p_ptr->deathblow < 10) || ((p_ptr->deathblow < p_ptr->mhp / 4) && (p_ptr->deathblow < 100))
#ifdef ENABLE_DIVINE
		    || (streq(p_ptr->died_from, "Indecisiveness"))
#endif
		    || (streq(p_ptr->died_from, "Insanity"))) {
			/* snprintf(buf, sizeof(buf), "\374\377r%s was killed by %s.", p_ptr->name, p_ptr->died_from); */
			/* Add the player lvl to the death message. the_sandman */
			snprintf(buf, sizeof(buf), "\374\377r%s (%d) was killed by %s", p_ptr->name, p_ptr->lev, p_ptr->died_from);
		}
		else if ((p_ptr->deathblow < 30) || ((p_ptr->deathblow < p_ptr->mhp / 2) && (p_ptr->deathblow < 450))) {
			/* snprintf(buf, sizeof(buf), "\377r%s was annihilated by %s.", p_ptr->name, p_ptr->died_from); */
			snprintf(buf, sizeof(buf), "\374\377r%s (%d) was annihilated by %s", p_ptr->name, p_ptr->lev, p_ptr->died_from);

		}
		else {
			snprintf(buf, sizeof(buf), "\374\377r%s (%d) was vaporized by %s.", p_ptr->name, p_ptr->lev, p_ptr->died_from);
		}
		s_printf("%s (%d) was killed by %s for %d damage at %d, %d, %d.\n", p_ptr->name, p_ptr->lev, p_ptr->died_from, p_ptr->deathblow, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
		if (!strcmp(p_ptr->died_from, "It") || !strcmp(p_ptr->died_from, "Insanity") || p_ptr->image)
			s_printf("(%s was really killed by %s.)\n", p_ptr->name, p_ptr->really_died_from);

s_printf("CHARACTER_TERMINATION: NORMAL race=%s ; class=%s\n", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title);
	}
	else if (!p_ptr->total_winner) {
		/* assume newb_suicide option for world broadcasts */
		if (p_ptr->max_plv == 1) world_broadcast = FALSE;

		snprintf(buf, sizeof(buf), "\374\377D%s committed suicide.", p_ptr->name);
		s_printf("%s (%d) committed suicide.\n", p_ptr->name, p_ptr->lev);
		death_type = DEATH_QUIT;
s_printf("CHARACTER_TERMINATION: SUICIDE race=%s ; class=%s\n", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title);
	} else {
		if (getlevel(&p_ptr->wpos) == 200) {
			snprintf(buf, sizeof(buf), "\374\377vThe unbeatable %s has retired to the shores of valinor.", p_ptr->name);
			if (!is_admin(p_ptr)) l_printf("%s \\{v%s (%d) retired to the shores of valinor\n", showdate(), p_ptr->name, p_ptr->lev);
		} else {
			snprintf(buf, sizeof(buf), "\374\377vThe unbeatable %s has retired to a warm, sunny climate.", p_ptr->name);
			if (!is_admin(p_ptr)) l_printf("%s \\{v%s (%d) retired to a warm, sunny climate\n", showdate(), p_ptr->name, p_ptr->lev);
		}
		s_printf("%s (%d) committed suicide. (Retirement)\n", p_ptr->name, p_ptr->lev);
		death_type = DEATH_QUIT;
s_printf("CHARACTER_TERMINATION: RETIREMENT race=%s ; class=%s\n", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title);
	}

	if (is_admin(p_ptr)) snprintf(buf, sizeof(buf), "\376\377D%s bids farewell to this plane.", p_ptr->name);

	/* Tell the players */
	/* handle the secret_dungeon_master option */
	if ((!p_ptr->admin_dm) || (!cfg.secret_dungeon_master)) {
		/* handle newbie suicide option by manually doing 'msg_broadcast': */
		if (death_type == DEATH_QUIT) {
			/* Tell every player */
			for (i = 1; i <= NumPlayers; i++) {
				if (Players[i]->conn == NOT_CONNECTED) continue;
				if (i == Ind) continue;
				if (!Players[i]->newb_suicide && p_ptr->max_plv == 1)
					msg_format(i, "\376\377D%s committed suicide.", p_ptr->name);
				else
					msg_format(i, "\374\377D%s committed suicide.", p_ptr->name);
			}
		} else msg_broadcast(Ind, buf);

#ifdef TOMENET_WORLDS
		if (cfg.worldd_pdeath && world_broadcast) world_msg(buf);
#endif
	}

#if 0
	/* Unown land */
	if (p_ptr->total_winner) {
#ifdef NEW_DUNGEON
/* FIXME */
/*
		msg_broadcast_format(Ind, "%d(%d) and %d(%d) are no more owned.", p_ptr->own1, p_ptr->own2, p_ptr->own1 * 50, p_ptr->own2 * 50);
		wild_info[p_ptr->own1].own = wild_info[p_ptr->own2].own = 0;
*/
#else
		msg_broadcast_format(Ind, "%d(%d) and %d(%d) are no more owned.", p_ptr->own1, p_ptr->own2, p_ptr->own1 * 50, p_ptr->own2 * 50);
		wild_info[p_ptr->own1].own = wild_info[p_ptr->own2].own = 0;
#endif
	}
#endif

	/* Hmm... Shouldn't this be after the death message so we can get a nice message for retiring winners? - mikaelh */
	/* No longer a winner */
	p_ptr->total_winner = FALSE;

	/* Handle suicide */
	if (!p_ptr->alive) {
#ifdef TOMENET_WORLDS
		world_player(p_ptr->id, p_ptr->name, FALSE, TRUE);
#endif

		/* prevent suicide spam, if set in cfg */
		check_roller(Ind);

		erase_player(Ind, death_type, FALSE);

		/* Done */
		return;
	}

	/* Polymorph back to player */
	if (p_ptr->body_monster) do_mimic_change(Ind, 0, TRUE);

	/* Cure him from various maladies */
	p_ptr->black_breath = FALSE;
	if (p_ptr->image) (void)set_image(Ind, 0);
	if (p_ptr->blind) (void)set_blind(Ind, 0);
	if (p_ptr->paralyzed) (void)set_paralyzed(Ind, 0);
	if (p_ptr->confused) (void)set_confused(Ind, 0);
	if (p_ptr->poisoned) (void)set_poisoned(Ind, 0, 0);
	if (p_ptr->stun) (void)set_stun(Ind, 0);
	if (p_ptr->cut) (void)set_cut(Ind, 0, 0);
	/* if (p_ptr->food < PY_FOOD_FULL) */
	(void)set_food(Ind, PY_FOOD_FULL - 1);

	/* Don't have 'vegetable' ghosts running around after equipment was dropped */
	p_ptr->safe_sane = TRUE;
	p_ptr->update |= PU_SANITY;
	update_stuff(Ind);
	p_ptr->safe_sane = FALSE;

	/* Tell him */
	msg_print(Ind, "\374\377RYou die.");
//	msg_print(Ind, NULL);
	if ((p_ptr->deathblow < 10) || ((p_ptr->deathblow < p_ptr->mhp / 4) && (p_ptr->deathblow < 100))
#ifdef ENABLE_DIVINE
	    || (streq(p_ptr->died_from, "Indecisiveness"))
#endif
	    || (streq(p_ptr->died_from, "Insanity"))) {
		msg_format(Ind, "\374\377RYou have been killed by %s.", p_ptr->died_from);
	}
	else if ((p_ptr->deathblow < 30) || ((p_ptr->deathblow < p_ptr->mhp / 2) && (p_ptr->deathblow < 450))) {
		msg_format(Ind, "\374\377RYou have been annihilated by %s.", p_ptr->died_from);
	}
	else {
		msg_format(Ind, "\374\377RYou have been vaporized by %s.", p_ptr->died_from);
	}

#if CHATTERBOX_LEVEL > 2
	if (p_ptr->last_words) {
		char death_message[80];

		(void)get_rnd_line("death.txt", 0, death_message, 80);
		msg_print(Ind, death_message);
	}
#endif	// CHATTERBOX_LEVEL

#if (MAX_PING_RECVS_LOGGED > 0)
	/* Print last ping reception times */
	struct timeval now;
	gettimeofday(&now, NULL);
	s_printf("PING_RECEIVED:");
	/* Starting from latest */
	for (i = 0; i < MAX_PING_RECVS_LOGGED; i++) {
		j = (p_ptr->pings_received_head - i + MAX_PING_RECVS_LOGGED) % MAX_PING_RECVS_LOGGED;
		if (p_ptr->pings_received[j].tv_sec) {
			s_printf(" %s", timediff(&p_ptr->pings_received[j], &now));
		}
	}
	s_printf("\n");
#endif

	Send_chardump(Ind, "-death");

	/* Turn him into a ghost */
	p_ptr->ghost = 1;
	/* Prevent accidental floating up/downwards depending on client option. - C. Blue */
	if (p_ptr->safe_float) p_ptr->safe_float_turns = 5;

	/* Hack -- drop bones :) */
#ifdef ENABLE_DIVINE
	/* hackhack: Maiar don't have a real physical body ;) */
	if (p_ptr->prace != RACE_DIVINE)
#endif
	for (i = 0; i < 4; i++) {
		object_type	forge;
		o_ptr = &forge;

		invcopy(o_ptr, lookup_kind(TV_SKELETON, i ? SV_BROKEN_BONE : SV_BROKEN_SKULL));
		object_known(o_ptr);
		object_aware(Ind, o_ptr);
		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
		o_ptr->level = 0;
		o_ptr->note = quark_add(format("# of %s", p_ptr->name));
		/* o_ptr->note = quark_add(format("#of %s", p_ptr->name));
		the_sandman: removed the auto-space-padding on {# inscs */

		if (p_ptr->wpos.wz) o_ptr->marked2 = ITEM_REMOVAL_NEVER;
		else if (istown(&p_ptr->wpos)) o_ptr->marked2 = ITEM_REMOVAL_DEATH_WILD;/* don't litter towns for long */
		else o_ptr->marked2 = ITEM_REMOVAL_LONG_WILD;/* don't litter wilderness eternally ^^ */
		(void)drop_near(o_ptr, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);
	}

	/* Give him his hit points back */
	p_ptr->chp = p_ptr->mhp;
	p_ptr->chp_frac = 0;

	/* Teleport him */
	/* XXX p_ptr->death allows teleportation even when NO_TELE etc. */
	teleport_player(Ind, 200, TRUE);

	/* Hack -- Give him/her the newbie death guide */
//	if (p_ptr->max_plv < 20)	/* Now it's for everyone */
	{
		object_type	forge;
		o_ptr = &forge;

		invcopy(o_ptr, lookup_kind(TV_PARCHMENT, SV_PARCHMENT_DEATH));
		object_known(o_ptr);
		object_aware(Ind, o_ptr);
		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
		o_ptr->level = 1;
		(void)inven_carry(Ind, o_ptr);
	}
	/* Cancel any WOR spells */
	p_ptr->word_recall = 0;

	/* He is carrying nothing */
	p_ptr->inven_cnt = 0;

	p_ptr->deaths++;

	/* Remove the death flag */
	p_ptr->death = FALSE;

	/* Update bonus */
	p_ptr->update |= (PU_BONUS);

	/* Redraw */
	p_ptr->redraw |= (PR_HP | PR_GOLD | PR_BASIC | PR_DEPTH);

	/* Notice */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Windows */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	/* Possibly tell him what to do now */
	if (p_ptr->lev <= 10 && p_ptr->warning_ghost == 0) {
		p_ptr->warning_ghost = 1;
		msg_print(Ind, "\375\377RHINT: You died! You can wait for someone to revive you or use \377o<\377R or \377o>");
		msg_print(Ind, "\375\377R      keys to float back to town and revive yourself in the temple (the \377g4\377R).");
		msg_print(Ind, "\375\377R      If you wish to start over, press \377oSHIFT+Q\377R to erase this character.");
		s_printf("warning_ghost: %s\n", p_ptr->name);
	}
}

/*
 * Resurrect a player
 */
 
 /* To prevent people from ressurecting too many times, I am modifying this to give
    everyone 1 "freebie", and then to have a p_ptr->level % chance of failing to 
    ressurect and have your ghost be destroyed.
    
    -APD-
    
    hmm, haven't gotten aroudn to doing this yet...
    
    exploss tells by how much % the GHOST_XP_LOSS is reduced (C. Blue).
 */
void resurrect_player(int Ind, int loss_reduction) {
	player_type *p_ptr = Players[Ind];
	int reduce, loss_factor;

	/* Hack -- the dungeon master can not resurrect */
	if (p_ptr->admin_dm) return;	// TRUE;

	/* Reset ghost flag */
	p_ptr->ghost = 0;
	
	disturb(Ind, 1, 0);

	/* paranoia limits */	
	if (loss_reduction < 0) loss_reduction = 0;
	if (loss_reduction > 100) loss_reduction = 100;

	/* capping exp loss for resurrection prayer:
	   exploss = 0 for life scroll,
	   exploss = 0..17 for resurection prayer at 0..17 spell level. */
	if (loss_reduction > 17) loss_reduction = 17;

	/* Lose some experience */
	loss_factor = GHOST_XP_LOST * (100 - loss_reduction) / 100;
	reduce = p_ptr->max_exp;
	reduce = reduce > 99999 ?
	    reduce / 100 * loss_factor : reduce * loss_factor / 100;
	p_ptr->max_exp -= reduce;

	reduce = p_ptr->exp;
	reduce = reduce > 99999 ?
	    reduce / 100 * loss_factor : reduce * loss_factor / 100;
	p_ptr->exp -= reduce;

	p_ptr->safe_sane = TRUE;
	check_experience(Ind);
	p_ptr->update |= PU_SANITY;
	update_stuff(Ind);
	p_ptr->safe_sane = FALSE;

	/* Message */
	msg_print(Ind, "You feel life return to your body.");
	everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

	/* (was in player_death: Take care of ghost suiciding before final resurrection (p_ptr->alive check, C. Blue)) */
	/*if (p_ptr->alive && ((p_ptr->lives > 0+1) && cfg.lifes)) p_ptr->lives--;*/
	/* Tell him his remaining lifes */
	if (!(p_ptr->mode & MODE_EVERLASTING)
	    && !(p_ptr->mode & MODE_PVP)) {
		if (p_ptr->lives > 1+1) p_ptr->lives--;
		if (cfg.lifes) {
			if (p_ptr->lives == 1+1)
				msg_print(Ind, "\377GYou have no more resurrections left!");
			else
				msg_format(Ind, "\377GYou have %d resurrections left.", p_ptr->lives-1-1);
		}
	}

	/* Bonus service: Also restore drained exp (for newbies, especially) */
	restore_level(Ind);

	/* Redraw */
	p_ptr->redraw |= (PR_BASIC);

	/* Update */
	p_ptr->update |= (PU_BONUS);
}

void check_quests(){
	int i, j;
	struct player_type *q_ptr;
	for (i = 0; i < 20; i++) {
		if (quests[i].active && quests[i].id) {
			if ((turn - quests[i].turn) / 10 > MAX_QUEST_TURNS) {
				for (j = 1; j <= NumPlayers; j++) {
					q_ptr = Players[j];
					if (q_ptr && q_ptr->quest_id == quests[i].id) {
						msg_print(j, "\377oYou have failed your quest");
						q_ptr->quest_id = 0;
						q_ptr->quest_num = 0;
					}
				}
				quests[i].active = 0;
				quests[i].id = 0;
				quests[i].type = 0;
			}
		}
	}
}

void del_quest(int id){
	int i;
	for (i = 0; i < 20; i++) {
		if (quests[i].id == id) {
			s_printf("quest %d removed\n", id);
			quests[i].active = 0;
			quests[i].id = 0;
			quests[i].type = 0;
		}
	}
}

/* One player leave a quest (death, deletion) */
void rem_quest(u16b id){
	int i;

	s_printf("Player death. Quest id: %d\n", id);

	if(!id) return;

	for(i=0;i<20;i++){
		if(quests[i].id==id){
			break;
		}
	}
	if(i==20) return;
	s_printf("Quest found in slot %d\n",i);
	if(quests[i].active){
		quests[i].active--;
		s_printf("Remaining active: %d\n", quests[i].active);
		if(!quests[i].active){
			process_hooks(HOOK_QUEST_FAIL, "d", id);
			s_printf("delete call\n");
			del_quest(id);
		}
	}
}

void kill_quest(int Ind) {
	int i;
	bool great = FALSE, verygreat = FALSE;
	u16b id, pos = 9999;
	player_type *p_ptr = Players[Ind], *q_ptr;
	char temp[160];

	id = p_ptr->quest_id;
	for (i = 0; i < 20; i++) {
		if (quests[i].id == id) {
			pos = i;
			break;
		}
	}
//	if(pos == -1) return;	/* it's UNsigned :) */
	if(pos == 9999) return;

	process_hooks(HOOK_QUEST_FINISH, "d", Ind);

	snprintf(temp, 160, "\374\377y%s has won the %s quest!", p_ptr->name, r_name + r_info[quests[pos].type].name);
	if (quests[i].flags & QUEST_RACE)
		msg_broadcast(Ind, temp);
	if (quests[i].flags & QUEST_GUILD){
		hash_entry *temphash;
		if ((temphash = lookup_player(quests[i].creator)) && temphash->guild) {
			guild_msg(temphash->guild ,temp);
			if (!p_ptr->guild) {
				guild_msg_format(temphash->guild, "\374\377%c%s is now a guild member!", COLOUR_CHAT_GUILD, p_ptr->name);
				guilds[temphash->guild].members++;
				msg_format(Ind, "\374You've been added to '\377%c%s\377w'.", COLOUR_CHAT_GUILD, guilds[temphash->guild].name);
				p_ptr->guild = temphash->guild;
				clockin(Ind, 3);	/* set in db */
			}
			else if (p_ptr->guild == temphash->guild) {
				guild_msg_format(temphash->guild, "\374\377%c%s has completed the quest!", COLOUR_CHAT_GUILD, p_ptr->name);
			}
		}
	} else{
//#ifdef RPG_SERVER
		object_type forge, *o_ptr = &forge;
//#endif
		int avg = ((r_info[quests[pos].type].level * 2) + (p_ptr->lev * 4)) / 2;
		avg = avg > 100 ? 100 : avg;
		msg_format(Ind, "\377yYou have won the %s quest!", r_name+r_info[quests[pos].type].name);
		s_printf("r_info quest: %s won the %s quest\n", p_ptr->name, r_name + r_info[quests[pos].type].name);
		/*
		   Temporary prize ... Too good perhaps...
		   it will do for now though
		   - I toned it down a bit via magik() -C. Blue
		*/
		strcpy(temp, r_name + r_info[quests[pos].type].name);
		strcat(temp, " quest");
		unique_quark = quark_add(temp);
		great = magik(50 + p_ptr->lev * 2);
//		if (great && p_ptr->lev >= 30) verygreat = magik((p_ptr->lev - 25) * 4);
//		if (great && p_ptr->lev >= 25) verygreat = magik(r_info[quests[pos].type].level - 5);
//		if (great && p_ptr->lev >= 25) verygreat = magik(r_info[quests[pos].type].level - (5 - (p_ptr->lev / 5)));
//		if (great) verygreat = magik(r_info[quests[pos].type].level + (p_ptr->lev / 2) - 15);
//		if (great) verygreat = magik(((r_info[quests[pos].type].level * 2) + (p_ptr->lev * 4)) / 5);
//		avg /= 2; avg = 540 / (58 - avg) + 20; /* same as exp calculation ;) */
//		avg /= 2; avg = 540 / (58 - avg) + 20; /* same as exp calculation ;) */
		avg /= 2; avg = 540 / (57 - avg) + 5; /* same as exp calculation ;) phew, Heureka.. */
		if (great) verygreat = magik(avg);
#if 0 /* needs more care, otherwise acquirement could even be BETTER than create_reward, depending on RESF.. */
//#ifdef RPG_SERVER
		create_reward(Ind, o_ptr, getlevel(&p_ptr->wpos), getlevel(&p_ptr->wpos), great, verygreat, RESF_LOW, 3000);
//		o_ptr->discount = 100;
		o_ptr->note = quark_add(temp);
		o_ptr->note_utag = strlen(temp);
		inven_carry(Ind, o_ptr);
//		drop_near(o_ptr, -1, &p_ptr->wpos, p_ptr->py, p_ptr->px);
#else
//		acquirement(&p_ptr->wpos, p_ptr->py, p_ptr->px, 1, great, verygreat, RESF_LOW);
		acquirement_direct(o_ptr, &p_ptr->wpos, great, verygreat, RESF_LOW);
//s_printf("object rewarded %d,%d,%d\n", o_ptr->tval, o_ptr->sval, o_ptr->k_idx);
		inven_carry(Ind, o_ptr);
#endif
		unique_quark = 0;
	}
	for (i = 1; i <= NumPlayers; i++) {
		q_ptr = Players[i];
		if (q_ptr && q_ptr->quest_id == id) {
			q_ptr->quest_id = 0;
			q_ptr->quest_num = 0;
		}
	}
	del_quest(id);
}

s16b questid = 1;

bool add_quest(int Ind, int target, u16b type, u16b num, u16b flags) {
	int i, j;
	bool added = FALSE;
	int midlevel;
	player_type *p_ptr = Players[target], *q_ptr;
	if (!p_ptr) return(FALSE);

	midlevel = p_ptr->lev;

	process_hooks(HOOK_GEN_QUEST, "d", Ind);

	for (i = 0; i < 20; i++) {
		if (!quests[i].active) {
			quests[i].active = 0;
			quests[i].id = questid;
			quests[i].type = type;
			quests[i].flags = flags;
			quests[i].turn = turn;
			added = TRUE;
			break;
		}
	}
	if (!added) return(FALSE);
	added = 0;

	/* give it only to the one original target player */
	j = target;
	q_ptr = Players[j];
#ifndef RPG_SERVER
	if (q_ptr->lev < 5) return(FALSE); /* level 5 is minimum to do quests */
#else
	if (q_ptr->lev < 3) return(FALSE);
#endif
	q_ptr->quest_id = questid;
	q_ptr->quest_num = num;
	clockin(j, 4); /* register that player */
	msg_format(j, "\377oYou have been given a %squest\377y!", flags & QUEST_GUILD ? "guild " : "");
//	msg_format(j, "\377oFind and kill \377y%d \377g%s%s\377y!", num, r_name+r_info[type].name, flags&QUEST_GUILD?"":" \377obefore any other player");
	msg_format(j, "\377oFind and kill \377y%d \377g%s\377y!", num, r_name + r_info[type].name);
	quests[i].active++;

	if (!quests[i].active) {
		del_quest(questid);
		return(FALSE);
	}
	s_printf("Added quest id %d (players %d)\n", quests[i].id, quests[i].active);
	questid++;
	if (questid == 0) questid = 1;
	if (target != Ind) {
		if (flags & QUEST_GUILD) {
			guild_msg_format(Players[Ind]->guild, "\374\377%c%s has been given a quest!", COLOUR_CHAT_GUILD, p_ptr->name);
		}
		else msg_format(Ind, "Quest given to %s", p_ptr->name);
		quests[i].creator = Players[Ind]->id;
	}
	return(TRUE);
}

/* prepare some quest parameters for a standard kill quest */
bool prepare_quest(int Ind, int j, u16b flags, int *level, u16b *type, u16b *num){
	int r = *type, i = *num, lev = *level, k = 0;

	if (Players[j]->quest_id) {
		for (i = 0; i < 20; i++) {
			if (quests[i].id == Players[j]->quest_id) {
				if (j == Ind)
					msg_format(Ind, "\377oYour quest to kill \377y%d \377g%s \377ois not complete.%s",
					    Players[Ind]->quest_num, r_name + r_info[quests[i].type].name,
					    quests[i].flags & QUEST_GUILD?" (guild)" : "");
				return FALSE;
			}
		}
	}

	/* don't start too early -C. Blue */
#ifndef RPG_SERVER
	if (Players[j]->lev < 5) {
		msg_print(Ind, "\377oYou need to be level 5 or higher to receive a quest!");
#else /* for ironman there's no harm in allowing early quests */
	if (Players[j]->lev < 3) {
		msg_print(Ind, "\377oYou need to be level 3 or higher to receive a quest!");
#endif
		return FALSE;
	}

	/* plev 1..50 -> mlev 1..100 (!) */
	if (lev <= 50) lev += (lev * lev) / 83;
	else lev = 80 + rand_int(20);

	get_mon_num_hook = quest_aux;
	get_mon_num_prep(0, NULL);
	i = 2 + randint(5);

	do {
		r = get_mon_num(lev, lev - 10); //reduce OoD chance slightly

		k++;
		if (k > 100) lev--;
	} while (((lev - 5) > r_info[r].level) ||
	    (r_info[r].flags1 & RF1_UNIQUE) ||
	    (r_info[r].flags7 & RF7_MULTIPLY) ||
	    r_info[r].level <= 2); /* no Training Tower quests */
#ifndef RPG_SERVER
	if (r_info[r].flags1 & RF1_FRIENDS) i = i + 11 + randint(7);
#else /* very hard in the beginning in ironman dungeons */
	if (lev < 40) {
		if (r_info[r].flags1 & RF1_FRIENDS) i = i * 2;
		else i = (i + 1) / 2;
	} else {
		if (r_info[r].flags1 & RF1_FRIENDS) i = i + 11 + randint(7);
	}
#endif

	/* Hack: If (non-)FRIENDS variant exists, use the first one (usually the non-FRIENDS one) */
	if (r_info[r].dup_idx) r = r_info[r].dup_idx;

	*level = lev; *type = r; *num = i;
	return TRUE;
}



/*
 * Decreases monsters hit points, handling monster death.
 *
 * We return TRUE if the monster has been killed (and deleted).
 *
 * We announce monster death (using an optional "death message"
 * if given, and a otherwise a generic killed/destroyed message).
 *
 * Only "physical attacks" can induce the "You have slain" message.
 * Missile and Spell attacks will induce the "dies" message, or
 * various "specialized" messages.  Note that "You have destroyed"
 * and "is destroyed" are synonyms for "You have slain" and "dies".
 *
 * Hack -- unseen monsters yield "You have killed it." message.
 *
 * Added fear (DGK) and check whether to print fear messages -CWS
 *
 * Genericized name, sex, and capitilization -BEN-
 *
 * As always, the "ghost" processing is a total hack.
 *
 * Hack -- we "delay" fear messages by passing around a "fear" flag.
 *
 * XXX XXX XXX Consider decreasing monster experience over time, say,
 * by using "(m_exp * m_lev * (m_lev)) / (p_lev * (m_lev + n_killed))"
 * instead of simply "(m_exp * m_lev) / (p_lev)", to make the first
 * monster worth more than subsequent monsters.  This would also need
 * to induce changes in the monster recall code.
 */
bool mon_take_hit(int Ind, int m_idx, int dam, bool *fear, cptr note)
{
	player_type *p_ptr = Players[Ind];

	monster_type	*m_ptr = &m_list[m_idx];
	monster_race    *r_ptr = race_inf(m_ptr);

	s64b		new_exp, new_exp_frac;
	s64b 		tmp_exp, req_lvl;
	s16b skill_trauma = get_skill(p_ptr, SKILL_TRAUMATURGY);
	long scale_trauma = 0;
	bool old_tacit = suppress_message;

//	int dun_level2 = getlevel(&p_ptr->wpos);
	dungeon_type *dt_ptr2 = getdungeon(&p_ptr->wpos);
	int dun_type2;
	dungeon_info_type *d_ptr2 = NULL;
	if (p_ptr->wpos.wz) {
	        dun_type2 = dt_ptr2->type;
	        d_ptr2 = &d_info[dun_type2];
	}

#ifdef TEST_SERVER
	p_ptr->test_count++;
	p_ptr->test_dam += dam;
#endif

	/* break charmignore */
	if (m_ptr->charmedignore) {
		Players[m_ptr->charmedignore]->mcharming--;
		m_ptr->charmedignore = 0;
	}

	/* Trauma boost spell */
	if (p_ptr->tim_trauma) {
		skill_trauma += (skill_trauma < (SKILL_MAX - (p_ptr->tim_trauma_pow * 1000)) ? (p_ptr->tim_trauma_pow * 1000) : SKILL_MAX - skill_trauma);
		scale_trauma = (((skill_trauma) * 20) / SKILL_MAX);
	}

	/* Redraw (later) if needed */
	update_health(m_idx);

	/* Change monster's highest player encounter - mode 1+ : a player targetted this monster */
	if (m_ptr->wpos.wx != cfg.town_x || m_ptr->wpos.wy != cfg.town_y || m_ptr->wpos.wz != 0) { /* not in Bree, because of Halloween :) */
		if (m_ptr->highest_encounter < p_ptr->max_lev) m_ptr->highest_encounter = p_ptr->max_lev;
	}

	/* Traumaturgy skill - C. Blue */
	if (dam && skill_trauma &&
/*	    los(&p_ptr->wpos, p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx) && */
/*	    projectable(&p_ptr->wpos, p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx, MAX_RANGE) && */
	    target_able(Ind, m_idx) &&
	    (!(r_ptr->flags3 & RF3_UNDEAD)) &&
	    (!(r_ptr->flags3 & RF3_NONLIVING)) &&
	    (!(strchr("AEgv", r_ptr->d_char))))
	{
		/* difficult to balance, due to the different damage effects of spells- might need some changes */
		long gain = scale_trauma;
		gain = (dam/20 > gain ? gain : dam/20);//50
		if (gain > m_ptr->hp) gain = m_ptr->hp;
		if (!gain && magik(dam * 5)) gain = 1; /* no perma-supply for level 1 mana bolts for now */

		if (gain && (p_ptr->csp < p_ptr->msp)) {
			msg_print(Ind, "You draw energy from the pain of your opponent.");
			p_ptr->csp += gain;
	                if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;
			p_ptr->redraw |= (PR_MANA);
		}
	}

	/* Some monsters are immune to death */
	if (r_ptr->flags7 & RF7_NO_DEATH) return FALSE;
	
	/* Wake it up */
	m_ptr->csleep = 0;

	/* Hurt it */
	m_ptr->hp -= dam;

	/* record the data for use in C_BLUE_AI */
	p_ptr->dam_turn[0] += (m_ptr->hp < dam) ? m_ptr->hp : dam;

	/* It is dead now */
	if (m_ptr->hp < 0) {
#ifdef ARCADE_SERVER
		cave_set_feat(&m_ptr->wpos, m_ptr->fy, m_ptr->fx, 172); /* drop "blood"? */
		if(m_ptr->hp < -1000) {

		object_type forge, *o_ptr;
		o_ptr = &forge;
int i, head, arm, leg, part, ok;
head = arm = leg = part = 0;
for(i=1; i < 5; i++) {
		ok = 0;
		while(ok == 0) {
			ok = 1;
			part = randint(4);
			if(part == 1 && head == 1)
			ok = 0;
			if(part == 2 && arm == 2)
			ok = 0;
			if(part == 4 && leg == 2)
			ok = 0;
		}
		if(part == 1)
		head++;
		if(part == 2)
		arm++;
		if(part == 4)
		leg++;
		invcopy(o_ptr, lookup_kind(TV_SKELETON, part));
		object_known(o_ptr);
		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
		o_ptr->level = 1;
		o_ptr->marked2 = ITEM_REMOVAL_NORMAL;
		(void)drop_near(o_ptr, 0, &m_ptr->wpos, m_ptr->fy, m_ptr->fx);
		}
		}
#endif
		char m_name[MNAME_LEN];
		dun_level *l_ptr = getfloor(&p_ptr->wpos);
		/* Had to change it for Halloween -C. Blue */
		if (m_ptr->level == 0) tmp_exp = r_ptr->mexp;
		else tmp_exp = r_ptr->mexp * m_ptr->level;

		/* Hack -- remove possible suppress flag */
		suppress_message = FALSE;

		/* Award players of disadvantageous situations */
		if (l_ptr) {
			int factor = 100;
			if (l_ptr->flags1 & LF1_NO_MAGIC)     factor += 10;
			if (l_ptr->flags1 & LF1_NO_MAP)       factor += 15;
			if (l_ptr->flags1 & LF1_NO_MAGIC_MAP) factor += 10;
			if (l_ptr->flags1 & LF1_NO_DESTROY)   factor += 5;
			if (l_ptr->flags1 & LF1_NO_GENO)      factor += 5;
			tmp_exp = (tmp_exp * factor) / 100;
		}

		if (p_ptr->wpos.wz) {
			int factor = 100;
			if (d_ptr2->flags1 & DF1_NO_UP)		factor += 5;
			if (d_ptr2->flags2 & DF2_NO_RECALL_INTO)factor += 5;
			if (d_ptr2->flags1 & DF1_NO_RECALL)	factor += 10;
			if (d_ptr2->flags1 & DF1_FORCE_DOWN)	factor += 10;
			if (d_ptr2->flags2 & DF2_IRON)		factor += 15;
			if (d_ptr2->flags2 & DF2_HELL)		factor += 10;
			if (d_ptr2->flags2 & DF2_NO_DEATH)	factor -= 50;
			tmp_exp = (tmp_exp * factor) / 100;
		}

		/* Extract monster name */
		monster_desc(Ind, m_name, m_idx, 0);

#ifdef USE_SOUND_2010
#else
		sound(Ind, SOUND_KILL);
#endif

		/* Death by Missile/Spell attack */
		/* DEG modified spell damage messages. */
		if (note) {
			msg_format_near(Ind, "\377y%^s%s from \377g%d \377ydamage.", m_name, note, dam);
			msg_format(Ind, "\377y%^s%s from \377g%d \377ydamage.", m_name, note, dam);
		}

		/* Death by physical attack -- invisible monster */
		else if (!p_ptr->mon_vis[m_idx]) {
			msg_format_near(Ind, "\377y%s has been killed from \377g%d \377ydamage by %s.", m_name, dam, p_ptr->name);
			msg_format(Ind, "\377yYou have killed %s.", m_name);
		}

		/* Death by Physical attack -- non-living monster */
		else if ((r_ptr->flags3 & RF3_DEMON) ||
		         (r_ptr->flags3 & RF3_UNDEAD) ||
		         (r_ptr->flags2 & RF2_STUPID) ||
		         (strchr("Evg", r_ptr->d_char))) {
			msg_format_near(Ind, "\377y%s has been destroyed from \377g%d \377ydamage by %s.", m_name, dam, p_ptr->name);
			msg_format(Ind, "\377yYou have destroyed %s.", m_name);
		}

		/* Death by Physical attack -- living monster */
		else {
			msg_format_near(Ind, "\377y%s has been slain from \377g%d \377ydamage by %s.", m_name, dam, p_ptr->name);
			msg_format(Ind, "\377yYou have slain %s.", m_name);
		}

		/* Check if it's cloned unique, ie "someone else's spawn" */
		if ((r_ptr->flags1 & RF1_UNIQUE) && p_ptr->r_killed[m_ptr->r_idx] == 1)
			m_ptr->clone = 90; /* still allow some experience to be gained */

		if (p_ptr->wpos.wz != 0) {
			/* Monsters in the Nether Realm give extra-high exp,
			   +2% per floor! (C. Blue) */
			if (dt_ptr2->type == 6) {
				tmp_exp = ((((-p_ptr->wpos.wz) * 2) + 100) * tmp_exp) / 100;
			}
		}

		/* Split experience if in a party */
		if (p_ptr->party == 0 || p_ptr->ghost) {
			/* Don't allow cheap support from super-high level characters */
			if (cfg.henc_strictness && !p_ptr->total_winner) {
				if (m_ptr->highest_encounter - p_ptr->max_lev > MAX_PARTY_LEVEL_DIFF + 1) tmp_exp = 0; /* zonk */ 
				if (p_ptr->supported_by - p_ptr->max_lev > MAX_PARTY_LEVEL_DIFF + 1) tmp_exp = 0; /* zonk */
			}

			/* Higher characters who farm monsters on low levels compared to
			   their clvl will gain less exp.
			   (note: this formula also occurs in party_gain_exp) */
			req_lvl = det_exp_level(p_ptr->lev);
			if (getlevel(&p_ptr->wpos) < req_lvl) tmp_exp = tmp_exp * 2 / (2 + req_lvl - getlevel(&p_ptr->wpos));

			/* Give some experience */
			new_exp = tmp_exp / p_ptr->lev;

			/* Never get too much exp off a monster
			   due to high level difference,
			   make exception for low exp boosts like "holy jackal" */
			if ((new_exp > r_ptr->mexp * 4) && (new_exp > 200)) new_exp = r_ptr->mexp * 4;

			new_exp_frac = ((tmp_exp % p_ptr->lev)
					* 0x10000L / p_ptr->lev) + p_ptr->exp_frac;

			/* Keep track of experience */
			if (new_exp_frac >= 0x10000L) {
				new_exp++;
				p_ptr->exp_frac = new_exp_frac - 0x10000L;
			} else {
				p_ptr->exp_frac = new_exp_frac;
			}

			/* Gain experience */
			if ((new_exp * (100 - m_ptr->clone)) / 100) {
				if (!(p_ptr->mode & MODE_PVP)) gain_exp(Ind, (new_exp * (100 - m_ptr->clone)) / 100);
			}
		} else {
			/* Give experience to that party */
			/* Seemingly it's severe to cloning, but maybe it's ok :) */
//			if (!player_is_king(Ind) && !m_ptr->clone) party_gain_exp(Ind, p_ptr->party, tmp_exp);
			/* Since players won't share exp if leveldiff > MAX_PARTY_LEVEL_DIFF (7)
			   I see ne problem with kings sharing exp.
			   Otherwise Nether Realm parties are punished.. */
//			if (!player_is_king(Ind)) party_gain_exp(Ind, p_ptr->party, (tmp_exp*(100-m_ptr->clone))/100);
			if (!(p_ptr->mode & MODE_PVP)) party_gain_exp(Ind, p_ptr->party, (tmp_exp * (100 - m_ptr->clone)) / 100, r_ptr->mexp, m_ptr->highest_encounter);
		}

		/*
		 * Necromancy skill regenerates you
		 * Cannot drain an undead or nonliving monster
		 */
		if (get_skill(p_ptr, SKILL_NECROMANCY) &&
			(!(r_ptr->flags3 & RF3_UNDEAD)) &&
			(!(r_ptr->flags3 & RF3_NONLIVING)) &&
			target_able(Ind, m_idx) && !p_ptr->ghost) /* Target must be in LoS */
		{
/*			int gain = (r_ptr->level *
				get_skill_scale(p_ptr, SKILL_NECROMANCY, 100)) / 100 +
				get_skill(p_ptr, SKILL_NECROMANCY); */
			long gain, gain_sp, skill; /* let's make it more complicated - gain HP and SP now - C. Blue */
			skill = get_skill_scale(p_ptr, SKILL_NECROMANCY, 50);
			gain = get_skill_scale(p_ptr, SKILL_NECROMANCY, 100);
			gain = (m_ptr->level > gain ? gain : m_ptr->level);
			gain_sp = gain;

			if (skill >= 15) gain = (2 + gain) * (2 + gain) * (2 + gain) / 327;
			else gain = ((3 + skill) * (3 +  skill) - 9) / 21;

			if (!gain) gain = 1; /* level 0 monsters (and super-low skill) give some energy too */

#if 0 /* strange values I guess */
			if (gain_sp >= 60) gain_sp = (gain_sp - 60) * 20 + 100;
			else if (gain_sp >= 40) gain_sp = (gain_sp - 40) * 4 + 20;
			else if (gain_sp >= 30) gain_sp = (gain_sp - 30) + 7;
			else if (gain_sp >= 20) gain_sp = (gain_sp - 20) / 2 + 2;
			else gain_sp /= 10;
			if (!gain_sp && magik(25)) gain_sp = 1; /* level 0 monsters have chance to give energy too */
#else
			gain_sp = ((gain_sp + 1) * (gain_sp + 1)) / 50;
			if (!gain_sp) gain_sp = 1; /* level 0 monsters give energy */
#endif

			if ((p_ptr->chp < p_ptr->mhp) || (p_ptr->csp < p_ptr->msp)) {
				msg_print(Ind, "You absorb the energy of the dying soul.");
				hp_player_quiet(Ind, gain, TRUE);
				p_ptr->csp += gain_sp;
				if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;
				p_ptr->redraw |= (PR_MANA);
			}
		}

		monster_death(Ind, m_idx);
		/* Generate treasure */
		if (!m_ptr->clone) {
			int i, credit_idx = r_ptr->dup_idx ? r_ptr->dup_idx : m_ptr->r_idx;
			for (i = 0; i < 20; i++) {
				if (p_ptr->quest_id && quests[i].id == p_ptr->quest_id) {
					if (credit_idx == quests[i].type) {
						p_ptr->quest_num--;
						if (p_ptr->quest_num <= 0) { //there's a panic save bug after verygreat apply magic, q_n = -1 after
							kill_quest(Ind);
						} else
							msg_format(Ind, "%d more to go!", p_ptr->quest_num);
					}
					break;
				}
			}
		}

#ifdef MONSTER_INVENTORY
		monster_drop_carried_objects(m_ptr);
#endif	// MONSTER_INVENTORY


		/* When the player kills a Unique, it stays dead */
		/* No more, this is handled byt p_ptr->r_killed -- DG */
//		if (r_ptr->flags1 & RF1_UNIQUE) r_ptr->max_num = 0;
//		p_ptr->r_killed[m_ptr->r_idx] = TRUE;

		/* Recall even invisible uniques or winners */
		if (p_ptr->mon_vis[m_idx] || (r_ptr->flags1 & RF1_UNIQUE)) {
			/* Count kills in all lives */
			r_ptr->r_tkills++;

			/* Hack -- Auto-recall */
			recent_track(m_ptr->r_idx);
		}

		/* Delete the monster */
		delete_monster_idx(m_idx, FALSE);

		/* Not afraid */
		(*fear) = FALSE;

		suppress_message = old_tacit;

		/* Monster is dead */
		return (TRUE);
	}


#ifdef ALLOW_FEAR

	/* Mega-Hack -- Pain cancels fear */
	if (m_ptr->monfear && (dam > 0)) {
		int tmp = randint(dam);

		/* Cure a little fear */
		if (tmp < m_ptr->monfear) {
			/* Reduce fear */
			m_ptr->monfear -= tmp;
		}

		/* Cure all the fear */
		else {
			/* Cure fear */
			m_ptr->monfear = 0;

			/* No more fear */
			(*fear) = FALSE;
		}
	}

	/* Sometimes a monster gets scared by damage */
	else if (!m_ptr->monfear && !(r_ptr->flags3 & RF3_NO_FEAR)) {
		int percentage;

		/* Percentage of fully healthy */
		percentage = (100L * m_ptr->hp) / m_ptr->maxhp;

		/*
		 * Run (sometimes) if at 10% or less of max hit points,
		 * or (usually) when hit for half its current hit points
		 */
		if (((percentage <= 10) && (rand_int(10) < percentage)) ||
		    ((dam >= m_ptr->hp) && (rand_int(100) < 80)))
		{
			/* Hack -- note fear */
			(*fear) = TRUE;

			/* XXX XXX XXX Hack -- Add some timed fear */
			m_ptr->monfear = (randint(10) +
			                  (((dam >= m_ptr->hp) && (percentage > 7)) ?
			                   20 : ((11 - percentage) * 5)));
		}
	}

#endif

	/* Not dead yet */
	return (FALSE);
}

void monster_death_mon(int am_idx, int m_idx)
{
	int			i, j, y, x, ny, nx;
	int			number = 0;
	cave_type		*c_ptr;

	monster_type	*m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);

	bool good = (r_ptr->flags1 & RF1_DROP_GOOD) ? TRUE : FALSE;
	bool great = (r_ptr->flags1 & RF1_DROP_GREAT) ? TRUE : FALSE;

	bool do_gold = (!(r_ptr->flags1 & RF1_ONLY_ITEM));
	bool do_item = (!(r_ptr->flags1 & RF1_ONLY_GOLD));

	int force_coin = get_coin_type(r_ptr);
	struct worldpos *wpos;
	cave_type **zcave;

	/* Get the location */
	y = m_ptr->fy;
	x = m_ptr->fx;
	wpos = &m_ptr->wpos;
	if (!(zcave = getcave(wpos))) return;

	/* Determine how much we can drop */
	if ((r_ptr->flags1 & RF1_DROP_60) && (rand_int(100) < 60)) number++;
	if ((r_ptr->flags1 & RF1_DROP_90) && (rand_int(100) < 90)) number++;
	if (r_ptr->flags1 & RF1_DROP_1D2) number += damroll(1, 2);
	if (r_ptr->flags1 & RF1_DROP_2D2) number += damroll(2, 2);
	if (r_ptr->flags1 & RF1_DROP_3D2) number += damroll(3, 2);
	if (r_ptr->flags1 & RF1_DROP_4D2) number += damroll(4, 2);

	/* Drop some objects */
	for (j = 0; j < number; j++) {
		/* Try 20 times per item, increasing range */
		for (i = 0; i < 20; ++i) {
			int d = (i + 14) / 15;

			/* Pick a "correct" location */
			scatter(wpos, &ny, &nx, y, x, d, 0);

			/* Must be "clean" floor grid */
			if (!cave_clean_bold(zcave, ny, nx)) continue;

			/* Access the grid */
			c_ptr = &zcave[ny][nx];

			/* Hack -- handle creeping coins */
			coin_type = force_coin;

			/* Average dungeon and monster levels */
			object_level = (getlevel(wpos) + r_ptr->level) / 2;

			/* No easy item hunting in towns.. */
			if (wpos->wz == 0) object_level = r_ptr->level / 2;

			/* Place Gold */
			if (do_gold && (!do_item || (rand_int(100) < 50))) {
				place_gold(wpos, ny, nx, 0);
			}
			/* Place Object */
			else {
				place_object_restrictor = RESF_NONE;
				place_object(wpos, ny, nx, good, great, FALSE, RESF_LOW, default_obj_theme, 0, ITEM_REMOVAL_NORMAL);
			}

			/* Reset the object level */
			object_level = getlevel(wpos);

			/* Reset "coin" type */
			coin_type = 0;

			/* Notice */
			note_spot_depth(wpos, ny, nx);

			/* Display */
			everyone_lite_spot(wpos, ny, nx);

			/* Under a player */
			if (c_ptr->m_idx < 0) {
				msg_print(0 - c_ptr->m_idx, "You feel something roll beneath your feet.");
			}

			break;
		}
	}

	FREE(m_ptr->r_ptr, monster_race);
}

bool mon_take_hit_mon(int am_idx, int m_idx, int dam, bool *fear, cptr note)
{
	monster_type *am_ptr = &m_list[am_idx];
	monster_type	*m_ptr = &m_list[m_idx];
	monster_race    *r_ptr = race_inf(m_ptr);
	s64b		new_exp;

	/* Redraw (later) if needed */
	update_health(m_idx);

	/* Wake it up */
	m_ptr->csleep = 0;

	/* Hurt it */
	m_ptr->hp -= dam;

	/* Cannot kill uniques */
	if ((r_ptr->flags1 & RF1_UNIQUE) && (m_ptr->hp < 1)) m_ptr->hp = 1;

	/* It is dead now */
	if (m_ptr->hp < 0) {
		/* Give some experience */
//		new_exp = ((long)r_ptr->mexp * r_ptr->level) / am_ptr->level;
		/* Division by zero occurs here when a pet attacks a townie (level 0) - mikaelh */
		/* Only gain exp when target monster level > 0 */
		if (am_ptr->level > 0) {
			new_exp = ((long)r_ptr->mexp * m_ptr->level) / am_ptr->level;

			/* Gain experience */
			if((new_exp*(100-m_ptr->clone))/100)
				/* disabled for golems for now, till attack-bug (9k damage) has been solved */
				if (!m_ptr->special && !m_ptr->owner)
					monster_gain_exp(am_idx, (new_exp*(100-m_ptr->clone))/100, TRUE);
		}
/*
switch (m_ptr->r_idx - 1) {
case SV_GOLEM_WOOD:
case SV_GOLEM_COPPER:
case SV_GOLEM_IRON:
case SV_GOLEM_ALUM:
case SV_GOLEM_SILVER:
case SV_GOLEM_GOLD:
case SV_GOLEM_MITHRIL:
case SV_GOLEM_ADAM:
} */

		/* Generate treasure */
                if (!m_ptr->clone) monster_death_mon(am_idx, m_idx);

		/* Delete the monster */
		delete_monster_idx(m_idx, TRUE);

		/* Not afraid */
		(*fear) = FALSE;

		/* Monster is dead */
		return (TRUE);
	}


#ifdef ALLOW_FEAR

	/* Mega-Hack -- Pain cancels fear */
	if (m_ptr->monfear && (dam > 0)) {
		int tmp = randint(dam);

		/* Cure a little fear */
		if (tmp < m_ptr->monfear) {
			/* Reduce fear */
			m_ptr->monfear -= tmp;
		}
		/* Cure all the fear */
		else {
			/* Cure fear */
			m_ptr->monfear = 0;

			/* No more fear */
			(*fear) = FALSE;
		}
	}

	/* Sometimes a monster gets scared by damage */
	else if (!m_ptr->monfear && !(r_ptr->flags3 & RF3_NO_FEAR)) {
		int percentage;

		/* Percentage of fully healthy */
		percentage = (100L * m_ptr->hp) / m_ptr->maxhp;

		/*
		 * Run (sometimes) if at 10% or less of max hit points,
		 * or (usually) when hit for half its current hit points
		 */
		if (((percentage <= 10) && (rand_int(10) < percentage)) ||
		    ((dam >= m_ptr->hp) && (rand_int(100) < 80))) {
			/* Hack -- note fear */
			(*fear) = TRUE;

			/* XXX XXX XXX Hack -- Add some timed fear */
			m_ptr->monfear = (randint(10) +
			    (((dam >= m_ptr->hp) && (percentage > 7)) ?
			    20 : ((11 - percentage) * 5)));
		}
	}

#endif

	/* Not dead yet */
	return (FALSE);
}



/*
 * Calculates current boundaries
 * Called below and from "do_cmd_locate()".
 */
void panel_bounds(int Ind)
{
	player_type *p_ptr = Players[Ind];

	p_ptr->panel_row_min = p_ptr->panel_row * (SCREEN_HGT / 2);
	p_ptr->panel_row_max = p_ptr->panel_row_min + SCREEN_HGT - 1;
	p_ptr->panel_row_prt = p_ptr->panel_row_min - 1;
	p_ptr->panel_col_min = p_ptr->panel_col * (SCREEN_WID / 2);
	p_ptr->panel_col_max = p_ptr->panel_col_min + SCREEN_WID - 1;
	p_ptr->panel_col_prt = p_ptr->panel_col_min - 13;
}



/*
 * Given an row (y) and col (x), this routine detects when a move
 * off the screen has occurred and figures new borders. -RAK-
 *
 * "Update" forces a "full update" to take place.
 *
 * The map is reprinted if necessary, and "TRUE" is returned.
 */
void verify_panel(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int y = p_ptr->py;
	int x = p_ptr->px;

	int prow = p_ptr->panel_row;
	int pcol = p_ptr->panel_col;

	/* Scroll screen when 2 grids from top/bottom edge */
	if ((y < p_ptr->panel_row_min + SCROLL_MARGIN_ROW) || (y > p_ptr->panel_row_max - SCROLL_MARGIN_ROW)) {
		prow = ((y - SCREEN_HGT / 4) / (SCREEN_HGT / 2));
		if (prow > p_ptr->max_panel_rows) prow = p_ptr->max_panel_rows;
		else if (prow < 0) prow = 0;
	}

	/* Scroll screen when 4 grids from left/right edge */
	if ((x < p_ptr->panel_col_min + SCROLL_MARGIN_COL) || (x > p_ptr->panel_col_max - SCROLL_MARGIN_COL)) {
		pcol = ((x - SCREEN_WID / 4) / (SCREEN_WID / 2));
		if (pcol > p_ptr->max_panel_cols) pcol = p_ptr->max_panel_cols;
		else if (pcol < 0) pcol = 0;
	}

	/* Check for "no change" */
	if ((prow == p_ptr->panel_row) && (pcol == p_ptr->panel_col)) return;

	/* Hack -- optional disturb on "panel change" */
	if (p_ptr->disturb_panel) disturb(Ind, 0, 0);

	/* Save the new panel info */
	p_ptr->panel_row = prow;
	p_ptr->panel_col = pcol;

	/* Recalculate the boundaries */
	panel_bounds(Ind);

	/* client-side weather stuff */
	p_ptr->panel_changed = TRUE;

	/* Update stuff */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);
}



/*
 * Monster health description
 */
cptr look_mon_desc(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);

	bool          living = TRUE;
	int           perc;


	/* Determine if the monster is "living" (vs "undead") */
	if (r_ptr->flags3 & RF3_UNDEAD) living = FALSE;
	if (r_ptr->flags3 & RF3_DEMON) living = FALSE;
	if (strchr("Egv", r_ptr->d_char)) living = FALSE;


	/* Healthy monsters */
	if (m_ptr->hp >= m_ptr->maxhp) {
		/* asleep even? */
		if (m_ptr->csleep) return("asleep");
		/* albeit stunned? */
		if (m_ptr->stunned > 100) return("knocked out");
		if (m_ptr->stunned > 50) return("heavily dazed");
		if (m_ptr->stunned) return("dazed");
		/* No damage (and no other effect either) */
		return (living ? "unhurt" : "undamaged");
	}


	/* Calculate a health "percentage" */
	perc = 100L * m_ptr->hp / m_ptr->maxhp;

	if (perc >= 60)
		return (living ? "somewhat wounded" : "somewhat damaged");

	if (perc >= 25)
		return (living ? "wounded" : "damaged");

	if (perc >= 10)
		return (living ? "badly wounded" : "badly damaged");

	return (living ? "almost dead" : "almost destroyed");
}



/*
 * Angband sorting algorithm -- quick sort in place
 *
 * Note that the details of the data we are sorting is hidden,
 * and we rely on the "ang_sort_comp()" and "ang_sort_swap()"
 * function hooks to interact with the data, which is given as
 * two pointers, and which may have any user-defined form.
 */
void ang_sort_aux(int Ind, vptr u, vptr v, int p, int q)
{
	int z, a, b;

	/* Done sort */
	if (p >= q) return;

	/* Pivot */
	z = p;

	/* Begin */
	a = p;
	b = q;

	/* Partition */
	while (TRUE) {
		/* Slide i2 */
		while (!(*ang_sort_comp)(Ind, u, v, b, z)) b--;

		/* Slide i1 */
		while (!(*ang_sort_comp)(Ind, u, v, z, a)) a++;

		/* Done partition */
		if (a >= b) break;

		/* Swap */
		(*ang_sort_swap)(Ind, u, v, a, b);

		/* Advance */
		a++, b--;
	}

	/* Recurse left side */
	ang_sort_aux(Ind, u, v, p, b);

	/* Recurse right side */
	ang_sort_aux(Ind, u, v, b+1, q);
}


/*
 * Angband sorting algorithm -- quick sort in place
 *
 * Note that the details of the data we are sorting is hidden,
 * and we rely on the "ang_sort_comp()" and "ang_sort_swap()"
 * function hooks to interact with the data, which is given as
 * two pointers, and which may have any user-defined form.
 */
void ang_sort(int Ind, vptr u, vptr v, int n)
{
	/* Sort the array */
	ang_sort_aux(Ind, u, v, 0, n-1);
}


/* returns our max times 100 divided by our current...*/
static int player_wounded(s16b ind)
{
	player_type *p_ptr = Players[ind];
	return (p_ptr->mhp * 100) / p_ptr->chp;
}

/* this should probably be somewhere more logical, but I should probably be
sleeping right now.....
Selects the most wounded target.

Hmm, I am sure there are faster sort algorithms out there... oh well, I don't 
think it really matters... this one goes out to you Mr. Munroe.
-ADA-
*/

static void wounded_player_target_sort(int Ind, vptr sx, vptr sy, vptr id, int n)
{
	int c,num;
	s16b swp;
	s16b * idx = (s16b *) id;
	byte * x = (byte *) sx;
	byte * y = (byte *) sy; 
	byte swpb;

	/* num equals our max index */
	num = n-1;

	while (num > 0) {
		for (c = 0; c < num; c++) {
			if (player_wounded(idx[c+1]) > player_wounded(idx[c])) {
				swp = idx[c];
				idx[c] = idx[c+1];
				idx[c+1] = swp;

				swpb = x[c];
				x[c] = x[c+1];
				x[c+1] = swpb;

				swpb = y[c];
				y[c] = y[c+1];
				y[c+1] = swpb;
			}
		}
	num--;
	}
}



/*
 * Sorting hook -- comp function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by double-distance to the player.
 */
bool ang_sort_comp_distance(int Ind, vptr u, vptr v, int a, int b)
{
	player_type *p_ptr = Players[Ind];

	byte *x = (byte*)(u);
	byte *y = (byte*)(v);

	int da, db, kx, ky;

	/* Absolute distance components */
	kx = x[a]; kx -= p_ptr->px; kx = ABS(kx);
	ky = y[a]; ky -= p_ptr->py; ky = ABS(ky);

	/* Approximate Double Distance to the first point */
	da = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

	/* Absolute distance components */
	kx = x[b]; kx -= p_ptr->px; kx = ABS(kx);
	ky = y[b]; ky -= p_ptr->py; ky = ABS(ky);

	/* Approximate Double Distance to the first point */
	db = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

	/* Compare the distances */
	return (da <= db);
}


/*
 * Sorting hook -- swap function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by distance to the player.
 */
void ang_sort_swap_distance(int Ind, vptr u, vptr v, int a, int b)
{
	byte *x = (byte*)(u);
	byte *y = (byte*)(v);

	byte temp;

	/* Swap "x" */
	temp = x[a];
	x[a] = x[b];
	x[b] = temp;

	/* Swap "y" */
	temp = y[a];
	y[a] = y[b];
	y[b] = temp;
}



/*
 * Compare the values of two objects.
 *
 * Pointer "v" should not point to anything (it isn't used, anyway).
 */
bool ang_sort_comp_value(int Ind, vptr u, vptr v, int a, int b)
{
	object_type *inven = (object_type *)u;
	s64b va, vb;

	if (inven[a].tval && inven[b].tval) {
		va = object_value(Ind, &inven[a]);
		vb = object_value(Ind, &inven[b]);

		return (va >= vb);
	}

	if (inven[a].tval) return FALSE;
	return TRUE;
}


void ang_sort_swap_value(int Ind, vptr u, vptr v, int a, int b)
{
	object_type *x = (object_type *)u;
	object_type temp;

	temp = x[a];
	x[a] = x[b];
	x[b] = temp;
}


/*
 * Sort a list of r_idx by level(depth).	- Jir -
 *
 * Pointer "v" should not point to anything (it isn't used, anyway).
 */
bool ang_sort_comp_mon_lev(int Ind, vptr u, vptr v, int a, int b)
{
	s16b *r_idx = (s16b*)u;
	s32b va, vb;
	monster_race *ra_ptr = &r_info[r_idx[a]];
	monster_race *rb_ptr = &r_info[r_idx[b]];

	if (ra_ptr->name && rb_ptr->name) {
		va = ra_ptr->level * 3000 + r_idx[a];
		vb = rb_ptr->level * 3000 + r_idx[b];

		return (va >= vb);
	}

	if (ra_ptr->name) return FALSE;
	return TRUE;
}


/* namely. */
void ang_sort_swap_s16b(int Ind, vptr u, vptr v, int a, int b)
{
	s16b *x = (s16b*)u;
	s16b temp;

	temp = x[a];
	x[a] = x[b];
	x[b] = temp;
}

/*
 * Sort a list of k_idx by tval and sval.	- Jir -
 *
 * Pointer "v" should not point to anything (it isn't used, anyway).
 */
bool ang_sort_comp_tval(int Ind, vptr u, vptr v, int a, int b)
{
	s16b *k_idx = (s16b*)u;
	s32b va, vb;
	object_kind *ka_ptr = &k_info[k_idx[a]];
	object_kind *kb_ptr = &k_info[k_idx[b]];

	if (ka_ptr->tval && kb_ptr->tval) {
		va = ka_ptr->tval * 256 + ka_ptr->sval;
		vb = kb_ptr->tval * 256 + kb_ptr->sval;

		return (va >= vb);
	}

	if (ka_ptr->tval) return FALSE;
	return TRUE;
}



/*** Targetting Code ***/


/*
 * Determine is a monster makes a reasonable target
 *
 * The concept of "targetting" was stolen from "Morgul" (?)
 *
 * The player can target any location, or any "target-able" monster.
 *
 * Currently, a monster is "target_able" if it is visible, and if
 * the player can hit it with a projection, and the player is not
 * hallucinating.  This allows use of "use closest target" macros.
 *
 * Future versions may restrict the ability to target "trappers"
 * and "mimics", but the semantics is a little bit weird.
 */
/* Allow auto-retaliation to not get 'disabled' by a Sparrow next to the player?
   Also see EXPENSIVE_NO_TARGET_TEST. */
//#define CHEAP_NO_TARGET_TEST
bool target_able(int Ind, int m_idx)
{
	player_type *p_ptr = Players[Ind], *q_ptr;
	monster_type *m_ptr;

	if (!p_ptr) return FALSE;

	/* Hack -- no targeting hallucinations */
	if (p_ptr->image) return (FALSE);

	/* Check for OK monster */
	if (m_idx > 0) {
		monster_race *r_ptr;

		/* Acquire pointer */
		m_ptr = &m_list[m_idx];
		r_ptr = race_inf(m_ptr);

		/* Monster must be visible */
		if (!p_ptr->mon_vis[m_idx]) return (FALSE);

		/* Monster must not be owned */
		if (p_ptr->id == m_ptr->owner) return (FALSE);

		/* Distance to target too great?
		   Use distance() to form a 'natural' circle shaped radius instead of a square shaped radius,
		   monsters do this too */
		if (distance(p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx) > MAX_RANGE) return (FALSE);

		/* Monster must be projectable */
#ifdef PY_PROJ_WALL
		if (!projectable_wall(&p_ptr->wpos, p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx, MAX_RANGE)) return (FALSE);
#else
		if (!projectable(&p_ptr->wpos, p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx, MAX_RANGE)) return (FALSE);
#endif

		if(m_ptr->owner == p_ptr->id) return(FALSE);

		/* XXX XXX XXX Hack -- Never target trappers */
		/* if (CLEAR_ATTR && CLEAR_CHAR) return (FALSE); */
		
		/* Cannot be targeted */
#ifdef CHEAP_NO_TARGET_TEST
		/* Allow targetting if it's next to player? */
		if (!(ABS(p_ptr->px - m_ptr->fx) <= 1 && ABS(p_ptr->py - m_ptr->fy) <= 1))
#endif
		if (r_ptr->flags7 & RF7_NO_TARGET) return (FALSE);

		/* Assume okay */
		return (TRUE);
	}

	/* Check for OK player */
	if (m_idx < 0) {
		/* Don't target oneself */
		if (Ind == 0 - m_idx) return(FALSE);

		/* Acquire pointer */
		q_ptr = Players[0 - m_idx];

		if ((0 - m_idx) > NumPlayers) q_ptr=NULL;

		/* Paranoia check -- require a valid player */
		if (!q_ptr || q_ptr->conn == NOT_CONNECTED){
			p_ptr->target_who = 0;
			return (FALSE);
		}

		/* Players must be on same depth */
		if (!inarea(&p_ptr->wpos, &q_ptr->wpos)) return (FALSE);

		/* Player must be visible */
		if (!player_can_see_bold(Ind, q_ptr->py, q_ptr->px)) return (FALSE);

		/* Distance to target too great?
		   Use distance() to form a 'natural' circle shaped radius instead of a square shaped radius,
		   monsters do this too */
		if (distance(p_ptr->py, p_ptr->px, q_ptr->py, q_ptr->px) > MAX_RANGE) return (FALSE);

		/* Player must be projectable */
#ifdef PY_PROJ_WALL
		if (!projectable_wall(&p_ptr->wpos, p_ptr->py, p_ptr->px, q_ptr->py, q_ptr->px, MAX_RANGE)) return (FALSE);
#else
		if (!projectable(&p_ptr->wpos, p_ptr->py, p_ptr->px, q_ptr->py, q_ptr->px, MAX_RANGE)) return (FALSE);
#endif

		/* Assume okay */
		return (TRUE);
	}

	/* Assume no target */
	return (FALSE);
}




/*
 * Update (if necessary) and verify (if possible) the target.
 *
 * We return TRUE if the target is "okay" and FALSE otherwise.
 */
bool target_okay(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Accept stationary targets */
//	if (p_ptr->target_who > MAX_M_IDX) return (TRUE);
	if (p_ptr->target_who < 0 - MAX_PLAYERS) return (TRUE);

	/* Check moving monsters */
	if (p_ptr->target_who > 0) {
		/* Accept reasonable targets */
		if (target_able(Ind, p_ptr->target_who)) {
			monster_type *m_ptr = &m_list[p_ptr->target_who];

			/* Acquire monster location */
			p_ptr->target_row = m_ptr->fy;
			p_ptr->target_col = m_ptr->fx;

			/* Good target */
			return (TRUE);
		}
	}

	/* Check moving players */
	if (p_ptr->target_who < 0) {
		/* Accept reasonable targets */
		if (target_able(Ind, p_ptr->target_who)) {
			player_type *q_ptr = Players[0 - p_ptr->target_who];

			/* Acquire player location */
			p_ptr->target_row = q_ptr->py;
			p_ptr->target_col = q_ptr->px;

			/* Good target */
			return (TRUE);
		}
	}

	/* Assume no target */
	return (FALSE);
}



/*
 * Hack -- help "select" a location (see below)
 */
s16b target_pick(int Ind, int y1, int x1, int dy, int dx)
{
	player_type *p_ptr = Players[Ind];
	int i, v;
	int x2, y2, x3, y3, x4, y4;
	int b_i = -1, b_v = 9999;

	/* Scan the locations */
	for (i = 0; i < p_ptr->target_n; i++) {
		/* Point 2 */
		x2 = p_ptr->target_x[i];
		y2 = p_ptr->target_y[i];

		/* Directed distance */
		x3 = (x2 - x1);
		y3 = (y2 - y1);

		/* Verify quadrant */
		if (dx && (x3 * dx <= 0)) continue;
		if (dy && (y3 * dy <= 0)) continue;

		/* Absolute distance */
		x4 = ABS(x3);
		y4 = ABS(y3);

		/* Verify quadrant */
		if (dy && !dx && (x4 > y4)) continue;
		if (dx && !dy && (y4 > x4)) continue;

		/* Approximate Double Distance */
		v = ((x4 > y4) ? (x4 + x4 + y4) : (y4 + y4 + x4));

		/* XXX XXX XXX Penalize location */

		/* Track best */
		if ((b_i >= 0) && (v >= b_v)) continue;

		/* Track best */
		b_i = i; b_v = v;
	}

	/* Result */
	return (b_i);
}


/*
 * Set a new target.  This code can be called from "get_aim_dir()"
 *
 * The target must be on the current panel.  Consider the use of
 * "panel_bounds()" to allow "off-panel" targets, perhaps by using
 * some form of "scrolling" the map around the cursor.  XXX XXX XXX
 *
 * That is, consider the possibility of "auto-scrolling" the screen
 * while the cursor moves around.  This may require changes in the
 * "update_mon()" code to allow "visibility" even if off panel.
 *
 * Hack -- targetting an "outer border grid" may be dangerous,
 * so this is not currently allowed.
 *
 * You can now use the direction keys to move among legal monsters,
 * just like the new "look" function allows the use of direction
 * keys to move amongst interesting locations.
 */
static bool autotarget = FALSE;
bool target_set(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind], *q_ptr;
	struct worldpos *wpos=&p_ptr->wpos;
	int		i, m, idx;
	int		y;
	int		x;
//	bool	flag = TRUE;
	bool	flag = autotarget;
	char	out_val[160];
	cave_type		*c_ptr;
	monster_type	*m_ptr;
	monster_race	*r_ptr;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return(FALSE);


	if (!dir) {
		x = p_ptr->px;
		y = p_ptr->py;

		/* Go ahead and turn off target mode */
		p_ptr->target_who = 0;

		/* Turn off health tracking */
		health_track(Ind, 0);


		/* Reset "target" array */
		p_ptr->target_n = 0;

		/* Collect "target-able" monsters */
		for (i = 1; i < m_max; i++) {
			monster_type *m_ptr = &m_list[i];

			/* Skip "dead" monsters */
			if (!m_ptr->r_idx) continue;

			/* Skip monsters not on this depth */
			if (!inarea(&m_ptr->wpos, wpos)) continue;

			/* Ignore "unreasonable" monsters */
			if (!target_able(Ind, i)) continue;

			/* Save this monster index */
			p_ptr->target_x[p_ptr->target_n] = m_ptr->fx;
			p_ptr->target_y[p_ptr->target_n] = m_ptr->fy;
			p_ptr->target_n++;
		}

		/* Collect "target-able" players */
		for (i = 1; i <= NumPlayers; i++) {
			/* Acquire pointer */
			q_ptr = Players[i];

			/* Don't target yourself */
			if (i == Ind) continue;

			/* Skip unconnected players */
			if (q_ptr->conn == NOT_CONNECTED) continue;

			/* Ignore players we aren't hostile to */
			if (!check_hostile(Ind, i)) {
					continue;
			}
			/* Ignore "unreasonable" players */
			if (!target_able(Ind, 0 - i)) continue;

			/* Save the player index */
			p_ptr->target_x[p_ptr->target_n] = q_ptr->px;
			p_ptr->target_y[p_ptr->target_n] = q_ptr->py;
			p_ptr->target_n++;
		}

		/* Set the sort hooks */
		ang_sort_comp = ang_sort_comp_distance;
		ang_sort_swap = ang_sort_swap_distance;

		/* Sort the positions */
		ang_sort(Ind, p_ptr->target_x, p_ptr->target_y, p_ptr->target_n);

		/* Collect indices */
		for (i = 0; i < p_ptr->target_n; i++) {
			cave_type *c_ptr = &zcave[p_ptr->target_y[i]][p_ptr->target_x[i]];

			p_ptr->target_idx[i] = c_ptr->m_idx;
		}

		/* Start near the player */
		m = 0;
	} else if (dir >= 128) {
		/* Initialize if needed */
		if (dir == 128) {
			p_ptr->target_col = p_ptr->px;
			p_ptr->target_row = p_ptr->py;
		} else {
			p_ptr->target_row += ddy[dir - 128];
			p_ptr->target_col += ddx[dir - 128];
		}

		/* Info */
		strcpy(out_val, "[<dir>, q] ");

		/* Tell the client */
		Send_target_info(Ind, p_ptr->target_col - p_ptr->panel_col_prt, p_ptr->target_row - p_ptr->panel_row_prt, out_val);

		/* Check for completion */
		if (dir == 128 + 5) {
			p_ptr->target_who = MAX_M_IDX + 1;
			return TRUE;
		}

		/* Done */
		return FALSE;
	} else {
		/* Start where we last left off */
		m = p_ptr->look_index;

		/* Reset the locations */
		for (i = 0; i < p_ptr->target_n; i++) {
			if (p_ptr->target_idx[i] > 0) {
				m_ptr = &m_list[p_ptr->target_idx[i]];

				p_ptr->target_y[i] = m_ptr->fy;
				p_ptr->target_x[i] = m_ptr->fx;
			} else if (p_ptr->target_idx[i] < 0) {
				q_ptr = Players[0 - p_ptr->target_idx[i]];

				p_ptr->target_y[i] = q_ptr->py;
				p_ptr->target_x[i] = q_ptr->px;
			}
		}

		/* Find a new monster */
		i = target_pick(Ind, p_ptr->target_y[m], p_ptr->target_x[m], ddy[dir], ddx[dir]);

		/* Use that monster */
		if (i > 0) m = i;
	}

	/* Target monsters */
	if (flag && p_ptr->target_n && p_ptr->target_idx[m] > 0) {
		y = p_ptr->target_y[m];
		x = p_ptr->target_x[m];
		idx = p_ptr->target_idx[m];

		c_ptr = &zcave[y][x];

		m_ptr = &m_list[idx];
		r_ptr = race_inf(m_ptr);

		/* Hack -- Track that monster race */
		recent_track(m_ptr->r_idx);

		/* Hack -- Track that monster */
		health_track(Ind, idx);

		/* Hack -- handle stuff */
		handle_stuff(Ind);

		/* Describe, prompt for recall */
		snprintf(out_val, sizeof(out_val),
		    "%s{%d} (%s) [<dir>, q, t] ",
		    r_name_get(m_ptr),
		    m_ptr->level,
		    look_mon_desc(idx));

		/* Tell the client about it */
		Send_target_info(Ind, x - p_ptr->panel_col_prt, y - p_ptr->panel_row_prt, out_val);
	} else if (flag && p_ptr->target_n && p_ptr->target_idx[m] < 0) {
		y = p_ptr->target_y[m];
		x = p_ptr->target_x[m];
		idx = p_ptr->target_idx[m];

		c_ptr = &zcave[y][x];

		q_ptr = Players[0 - idx];

		/* Hack -- Track that player */
		health_track(Ind, idx);

		/* Hack -- handle stuff */
		handle_stuff(Ind);

		/* Describe */
		snprintf(out_val, sizeof(out_val), "%s [<dir>, q, t] ", q_ptr->name);

		/* Tell the client about it */
		Send_target_info(Ind, x - p_ptr->panel_col_prt, y - p_ptr->panel_row_prt, out_val);
	}

	/* Remember current index */
	p_ptr->look_index = m;

	/* Set target */
	if (dir == 5 || autotarget) {
		p_ptr->target_who = p_ptr->target_idx[m];
		p_ptr->target_col = p_ptr->target_x[m];
		p_ptr->target_row = p_ptr->target_y[m];

		/* Track */
		if (p_ptr->target_who) health_track(Ind, p_ptr->target_who);
	}

	/* Failure */
	if (!p_ptr->target_who) return (FALSE);

	/* Clear target info */
	p_ptr->target_n = 0;

	/* Success */
	return (TRUE);
}

/* targets the most wounded teammate. should be useful for stuff like
 * heal other and teleport macros. -ADA-
 *
 * Now this function can take 3rd arg which specifies which player to
 * set the target.
 * This part was written by Asclep(DEG); thx for his courtesy!
 * */

bool target_set_friendly(int Ind, int dir, ...)
{
	va_list ap;
	player_type *p_ptr = Players[Ind], *q_ptr;
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
	int		i, m, castplayer, idx;
	int		y;
	int		x;
	char	out_val[160];
	cave_type	*c_ptr;

	if (!(zcave = getcave(wpos))) return(FALSE);

		va_start(ap,dir);
		castplayer = va_arg(ap,int);
		va_end(ap);

		x = p_ptr->px;
		y = p_ptr->py;

		/* Go ahead and turn off target mode */
		p_ptr->target_who = 0;

		/* Turn off health tracking */
		health_track(Ind, 0);


		/* Reset "target" array */
		p_ptr->target_n = 0;

//		if (!((castplayer > 0) && (castplayer < 20)))
		if (!((0 < castplayer) && (castplayer <= NumPlayers))) {
			/* Collect "target-able" players */
			for (i = 1; i <= NumPlayers; i++) {
				/* Acquire pointer */
				q_ptr = Players[i];

				/* Don't target yourself */
				if (i == Ind) continue;

				/* Skip unconnected players */
				if (q_ptr->conn == NOT_CONNECTED) continue;

				/* Ignore players we aren't friends with */
				if (check_hostile(Ind, i)) continue;

				/* if we are in party, only help members */
				if (p_ptr->party && (!player_in_party(p_ptr->party, i))) continue;

				/* Ignore "unreasonable" players */
				if (!target_able(Ind, 0 - i)) continue;

				/* Save the player index */
				p_ptr->target_x[p_ptr->target_n] = q_ptr->px;
				p_ptr->target_y[p_ptr->target_n] = q_ptr->py;
				p_ptr->target_idx[p_ptr->target_n] = i;
				p_ptr->target_n++;
			}
		} else {
			/* Acquire pointer */
			q_ptr = Players[castplayer];

			/* Skip unconnected players */
			if (q_ptr->conn == NOT_CONNECTED) return FALSE;

			/* Ignore "unreasonable" players */
			if (!target_able(Ind, 0 - castplayer)) return FALSE;

			/* Save the player index */
			p_ptr->target_x[p_ptr->target_n] = q_ptr->px;
			p_ptr->target_y[p_ptr->target_n] = q_ptr->py;
			p_ptr->target_idx[p_ptr->target_n] = castplayer;
			p_ptr->target_n++;
		}
		
			
		/* Set the sort hooks */ 
		ang_sort_comp = ang_sort_comp_distance;
		ang_sort_swap = ang_sort_swap_distance;

		/* Sort the positions */
		wounded_player_target_sort(Ind, p_ptr->target_x, p_ptr->target_y, p_ptr->target_idx, p_ptr->target_n);

		m = 0;

	/* too lazy to handle dirs right now */
	
	/* handle player target.... */	
	if (p_ptr->target_n) {
		y = p_ptr->target_y[m];
		x = p_ptr->target_x[m];
		idx = p_ptr->target_idx[m];

		c_ptr = &zcave[y][x];

		q_ptr = Players[idx];

		/* Hack -- Track that player */
		health_track(Ind, 0 - idx);

		/* Hack -- handle stuff */
		handle_stuff(Ind);

		/* Describe */
		snprintf(out_val, sizeof(out_val), "%s targetted.", q_ptr->name);

		/* Tell the client about it */
		Send_target_info(Ind, x - p_ptr->panel_col_prt, y - p_ptr->panel_row_prt, out_val);
	}

	/* Remember current index */
	p_ptr->look_index = m;

	p_ptr->target_who = 0 - p_ptr->target_idx[m];
	p_ptr->target_col = p_ptr->target_x[m];
	p_ptr->target_row = p_ptr->target_y[m];	

	/* Failure */
	if (!p_ptr->target_who) return (FALSE);

	/* Clear target info */
	p_ptr->target_n = 0;

	/* Success */
	return (TRUE);
}



/*
 * Get an "aiming direction" from the user.
 *
 * The "dir" is loaded with 1,2,3,4,6,7,8,9 for "actual direction", and
 * "0" for "current target", and "-1" for "entry aborted".
 *
 * Note that "Force Target", if set, will pre-empt user interaction,
 * if there is a usable target already set.
 *
 * Note that confusion over-rides any (explicit?) user choice.
 *
 * We just ask the client to send us a direction, unless we are confused --KLJ--
 */
bool get_aim_dir(int Ind)
{
	int dir;
	player_type *p_ptr = Players[Ind];

	if (p_ptr->auto_target) {
		autotarget = TRUE;
		target_set(Ind, 0);
		autotarget = FALSE;
	}

	/* Hack -- auto-target if requested */
	if (p_ptr->use_old_target && target_okay(Ind)) {
		dir = 5;

		/* XXX XXX Pretend we read this direction from the network */
		Handle_direction(Ind, dir);
		return (TRUE);
	}

	Send_direction(Ind);

	return (TRUE);
}


bool get_item(int Ind)
{
	Send_item_request(Ind);

	return (TRUE);
}

/*
 * Allows to travel both vertical/horizontal using Recall;
 * probably wilderness(horizontal) travel will be made by other means
 * in the future.
 *
 * Also, player_type doesn't contain the max.depth for each dungeon...
 * Currently, this function uses getlevel() to determine the max.depth
 * for each dungeon, but this should be replaced by actual depths
 * a player has ever been.	- Jir -
 */
void set_recall_depth(player_type * p_ptr, object_type * o_ptr)
{
//	int recall_depth = 0;
//	worldpos goal;

	unsigned char * inscription = (unsigned char *) quark_str(o_ptr->note);

	/* default to the players maximum depth */
	p_ptr->recall_pos.wx = p_ptr->wpos.wx;
	p_ptr->recall_pos.wy = p_ptr->wpos.wy;
	p_ptr->recall_pos.wz = (wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags &
			WILD_F_DOWN) ? 0 - p_ptr->max_dlv : p_ptr->max_dlv;
#if 0
	p_ptr->recall_pos.wz = (wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags &
			WILD_F_DOWN) ? 0 - p_ptr->max_dlv :
			((wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags & WILD_F_UP) ?
			 p_ptr->max_dlv : 0);

	goal.wx = p_ptr->wpos.wx;
	goal.wy = p_ptr->wpos.wy;
//	goal.wz = 0 - p_ptr->max_dlv;	// hack -- default to 'dungeon'
#endif	// 0

	/* check for a valid inscription */
	if (inscription == NULL) return;

	/* scan the inscription for @R */
	while (*inscription != '\0') {
		if (*inscription == '@') {
			inscription++;

			/* a valid @R has been located */
			if (*inscription == 'R') {
				inscription++;
				/* @RW for World(Wilderness) travel */
				/* It would be also amusing to limit the distance.. */
				if ((*inscription == 'W') || (*inscription == 'X')) {
					unsigned char * next;
					inscription++;
					p_ptr->recall_pos.wx = atoi((char *)inscription) % MAX_WILD_X;
					p_ptr->recall_pos.wz = 0;
					next = (unsigned char *)strchr((char *)inscription,',');
					if (next) {
						if (++next) p_ptr->recall_pos.wy = atoi((char*)next) % MAX_WILD_Y;
					}
				}
				else if (*inscription == 'Y') {
					inscription++;
					p_ptr->recall_pos.wy = atoi((char*)inscription) % MAX_WILD_Y;
					p_ptr->recall_pos.wz = 0;
				}
#if 1
				/* @RT for inter-Town travels (not fully implemented yet) */
				else if (*inscription == 'T') {
					inscription++;
					p_ptr->recall_pos.wx = p_ptr->town_x;
					p_ptr->recall_pos.wy = p_ptr->town_y;
					p_ptr->recall_pos.wz = 0;
				}
#endif
				else {
					int tmp = 0;
					if (*inscription == 'Z') inscription++;

					/* convert the inscription into a level index */
					if ((tmp = atoi((char*)inscription) /
							(p_ptr->depth_in_feet ? 50 : 1)))
						p_ptr->recall_pos.wz = tmp;

					/* catch user mistake: missing W in @RWx,y */
					while (*inscription != '\0') {
						if (*inscription == ',') {
							p_ptr->recall_pos.wz = 0;
							return;
						}
						inscription++;
					}
				}
			}
		}
		inscription++;
	}
}

bool set_recall_timer(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->word_recall) {
			msg_print(Ind, "\377oThe air about you becomes charged...");
			notice = TRUE;
		}
	}
	/* Shut */
	else {
		if (p_ptr->word_recall) {
			msg_print(Ind, "\377oA tension leaves the air around you...");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->word_recall = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Redraw the depth(colour) */
	p_ptr->redraw |= (PR_DEPTH);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

bool set_recall(int Ind, int v, object_type * o_ptr)
{
	player_type *p_ptr = Players[Ind];

	if (!p_ptr->word_recall) {
		set_recall_depth(p_ptr, o_ptr);
		return (set_recall_timer(Ind, v));
	} else {
		return (set_recall_timer(Ind, 0));
	}

}

void telekinesis_aux(int Ind, int item)
{
	player_type *p_ptr = Players[Ind], *p2_ptr;
	object_type *q_ptr, *o_ptr = p_ptr->current_telekinesis;
	int Ind2;
	char *inscription, *scan;

	p_ptr->current_telekinesis = NULL;

	/* Get the item (in the pack) */
	if (item >= 0) {
		q_ptr = &p_ptr->inventory[item];
	} else { /* Get the item (on the floor) */
		msg_print(Ind, "You must carry the object to teleport it.");
		return;
	}

	Ind2 = get_player(Ind, o_ptr);
	if (!Ind2) return;
	p2_ptr = Players[Ind2];

	if (p2_ptr->ghost && !is_admin(p_ptr)) {
		msg_print(Ind, "You cannot send items to ghosts!");
		return;
	}

	if (compat_pmode(Ind, Ind2)) {
		msg_format(Ind, "You cannot contact %s beings!", compat_pmode(Ind, Ind2));
		return;
	}

	/* prevent winners picking up true arts accidentally */
	if (true_artifact_p(o_ptr) && !winner_artifact_p(o_ptr) &&
	    p2_ptr->total_winner && cfg.kings_etiquette) {
		msg_print(Ind, "Royalties may not own true artifacts!");
		if (!is_admin(p2_ptr)) return;
	}

	/* the_sandman: item lvl restrictions are disabled in rpg */
#ifndef RPG_SERVER
	if ((q_ptr->owner) && (q_ptr->owner != p2_ptr->id) &&
	    (q_ptr->level > p2_ptr->lev || q_ptr->level == 0)) {
		if (cfg.anti_cheeze_telekinesis) {
			msg_print(Ind, "The target isn't powerful enough yet to receive that item!");
			if (!is_admin(p_ptr)) return;
		}
		if (true_artifact_p(q_ptr) && cfg.anti_arts_pickup)
//                      if (artifact_p(q_ptr) && cfg.anti_arts_pickup)
		{
			msg_print(Ind, "The target isn't powerful enough yet to receive that artifact!");
			if (!is_admin(p_ptr)) return;
		}
	}
#endif
	if ((k_info[q_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) && !p_ptr->once_winner
	    && !p_ptr->total_winner) { /* <- added this just for testing when admin char sets .total_winner=1 */
		msg_print(Ind, "Only royalties are powerful enough to receive that item!");
		if (!is_admin(p_ptr)) return;
	}

	if (cfg.anti_arts_send && artifact_p(q_ptr) && !is_admin(p_ptr)) {
		msg_print(Ind, "The artifact resists telekinesis!");
		return;
	}

	/* Add a check for full inventory of target player - mikaelh */
	if (!inven_carry_okay(Ind2, q_ptr)) {
		msg_print(Ind, "Item doesn't fit into the target player's inventory.");
		return;
	}

	/* Check that the target player isn't shopping - mikaelh */
	if (p2_ptr->store_num != -1) {
		msg_print(Ind, "Target player is currently shopping.");
		return;
	}

	/* You cannot send artifact */
	if ((cfg.anti_arts_hoard || p_ptr->total_winner) && true_artifact_p(q_ptr) && !is_admin(p_ptr)) {
		msg_print(Ind, "You have an acute feeling of loss!");
		handle_art_d(q_ptr->name1);
	} else {
		char o_name[ONAME_LEN];
		/* If they're not within the same dungeon level,
		   they cannot reach each other if
		   one is in an IRON or NO_RECALL dungeon/tower */
		if (!inarea(&p_ptr->wpos, &p2_ptr->wpos) && !is_admin(p_ptr)) {
			dungeon_type *d_ptr;
			d_ptr=getdungeon(&p_ptr->wpos);
			if(d_ptr && ((d_ptr->flags2 & (DF2_IRON | DF2_NO_RECALL_INTO)) || (d_ptr->flags1 & DF1_NO_RECALL))){
				msg_print(Ind, "You are unable to contact that player");
				return;
			}
			d_ptr = getdungeon(&p2_ptr->wpos);
			if(d_ptr && ((d_ptr->flags2 & (DF2_IRON | DF2_NO_RECALL_INTO)) || (d_ptr->flags1 & DF1_NO_RECALL))){
				msg_print(Ind, "You are unable to contact that player");
				return;
			}
		}

		if (!is_admin(p_ptr) && !(p2_ptr->esp_link_flags & LINKF_TELEKIN)) {
			msg_print(Ind, "That player isn't concentrating on telekinesis at the moment.");
			return;
		}


/* TEMPORARY ANTI-CHEEZE HACKS */  // todo: move to verify_level_req()
if (q_ptr->level != 0) {
	if (q_ptr->tval == TV_RING && q_ptr->sval == SV_RING_SPEED && q_ptr->level < 30 && (q_ptr->bpval > 0)) {
		s_printf("HACK-SPEEDREQ (Tele): %s(%d) ring (+%d): %d -> ", p_ptr->name, p_ptr->lev, q_ptr->bpval, q_ptr->level);
		determine_level_req(75, q_ptr);
		s_printf("%d.\n", q_ptr->level);
	}
	if (q_ptr->tval == TV_RING && q_ptr->sval >= SV_RING_MIGHT && q_ptr->sval <= SV_RING_CUNNINGNESS && q_ptr->level < q_ptr->bpval * 3 + 15) {   
		s_printf("HACK-STATSREQ (Tele): %s(%d) ring (+%d): %d -> ", p_ptr->name, p_ptr->lev, q_ptr->bpval, q_ptr->level);
		determine_level_req(25, q_ptr);
		s_printf("%d.\n", q_ptr->level);
	}
	if (q_ptr->tval == TV_POTION && q_ptr->sval >= SV_POTION_INC_STR && q_ptr->sval <= SV_POTION_INC_CHR && q_ptr->level < 28) {
		s_printf("HACK-STATPOT (Tele): %s(%d) potion: %d -> ", p_ptr->name, p_ptr->lev, q_ptr->level);
		determine_level_req(20, q_ptr);
		s_printf("%d.\n", q_ptr->level);
	}
}
if (is_weapon(q_ptr->tval) && !(k_info[q_ptr->k_idx].flags4 & (TR4_MUST2H | TR4_SHOULD2H))
    && (q_ptr->name2 == EGO_LIFE || q_ptr->name2b == EGO_LIFE) && (q_ptr->pval > 2)) {
	q_ptr->pval = 2;
}


		/* Log it - mikaelh */
		object_desc_store(Ind, o_name, q_ptr, TRUE, 3);
		s_printf("(Tele) Item transaction from %s(%d) to %s(%d):\n  %s\n", p_ptr->name, p_ptr->lev, Players[Ind2]->name, Players[Ind2]->lev, o_name);

		/* Highlander Tournament: Don't allow transactions before it begins */
		if (!p2_ptr->max_exp) gain_exp(Ind2, 1);

		/* Remove dangerous inscriptions - mikaelh */
		if (q_ptr->note && p2_ptr->clear_inscr) {
			scan = inscription = strdup(quark_str(q_ptr->note));

			while (*scan != '\0') {
				if (*scan == '@') {
					/* Replace @ with space */
					*scan = ' ';
				}
				scan++;
			}

			q_ptr->note = quark_add(inscription);
			free(inscription);
		}

		/* Actually teleport the object to the player inventory */
		can_use(Ind2, q_ptr); /* change owner */
		inven_carry(Ind2, q_ptr);

		/* Combine the pack */
		p2_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p2_ptr->window |= (PW_INVEN | PW_EQUIP);

		msg_format(Ind2, "You are hit by a powerful magic wave from %s.", p_ptr->name);
	}

	/* Wipe it */
	inven_item_increase(Ind, item, -99);
	inven_item_describe(Ind, item);
	inven_item_optimize(Ind, item);

	/* Combine the pack */
	p_ptr->notice |= (PN_COMBINE);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

}

int get_player(int Ind, object_type *o_ptr)
{
	bool ok = FALSE;
	int Ind2 = 0;
	unsigned char * inscription = (unsigned char *) quark_str(o_ptr->note);
	char ins2[80];

	/* check for a valid inscription */
	if (inscription == NULL) {
//		msg_print(Ind, "Nobody to use the power with.");
		msg_print(Ind, "\377rNo target player specified.");
		return 0;
	}

	/* scan the inscription for @P */
	while ((*inscription != '\0') && !ok) {
		if (*inscription == '@') {
			inscription++;

			/* a valid @P has been located */
			if (*inscription == 'P') {
				inscription++;

				/* stop at '#' symbol, to allow more inscription
				   variety without disturbing player name parsing */
				strcpy(ins2, (cptr)inscription);
				if (strchr(ins2, '#')) *(strchr(ins2, '#')) = '\0';

//				Ind2 = find_player_name(inscription);
//				Ind2 = name_lookup_loose(Ind, (cptr)inscription, FALSE);
				Ind2 = name_lookup_loose(Ind, ins2, FALSE);
				if (Ind2) ok = TRUE;
			}
		}
		inscription++;
	}

	if (!ok) {
		msg_print(Ind, "\377rCouldn't find the target.");
		return 0;
	}

	if (Ind == Ind2) {
		msg_print(Ind, "\377rYou cannot do that on yourself.");
		return 0;
	}

	return Ind2;
}

int get_monster(int Ind, object_type *o_ptr)
{
        bool ok1 = TRUE, ok2 = TRUE;
	int r_idx=0;

	unsigned char * inscription = (unsigned char *) quark_str(o_ptr->note);

       	/* check for a valid inscription */
	if (inscription == NULL) {
		msg_print(Ind, "No monster specified.");
		return 0;
	}
	
	/* scan the inscription for @M */
	while ((*inscription != '\0') && ok1 && ok2) {
		if (*inscription == '@') {
			inscription++;

			/* a valid @M has been located */
			if (*inscription == 'M') {
				inscription++;

				r_idx = atoi((cptr)inscription);
				if (r_idx < 1 || r_idx > MAX_R_IDX - 1) ok1 = FALSE;
				else if (!r_info[r_idx].name) ok1 = FALSE;
				else if (!Players[Ind]->r_killed[r_idx]) ok2 = FALSE;
			}
		}
		inscription++;
	}

        if (!ok1) {
		msg_print(Ind, "That monster does not exist.");
		return 0;
	}

	if (!ok2) {
		msg_print(Ind, "You haven't killed one of these monsters yet.");
		return 0;
	}

	return r_idx;
}

void blood_bond(int Ind, object_type *o_ptr)
{
        player_type *p_ptr = Players[Ind], *p2_ptr;
//      bool ok = FALSE;
        int Ind2;
	player_list_type *pl_ptr;
	cave_type **zcave;
	cave_type *c_ptr;

	if (p_ptr->pvpexception == 3) {
		msg_print(Ind, "Sorry, you're *not* allowed to attack other players.");
		return; /* otherwise, blood bond will result in insta-death */
	}

	Ind2 = get_player(Ind, o_ptr);
	if (!Ind2) {
		msg_print(Ind, "\377rCouldn't blood bond.");
		return;
	}
	p2_ptr = Players[Ind2];

	/* not during pvp-only or something (Highlander Tournament) */
	if (sector00separation &&
	    ((p_ptr->wpos.wx == WPOS_SECTOR00_X && p_ptr->wpos.wy == WPOS_SECTOR00_Y) ||
	    (p2_ptr->wpos.wx == WPOS_SECTOR00_X && p2_ptr->wpos.wy == WPOS_SECTOR00_Y))) {
		msg_print(Ind, "You cannot blood bond right now.");
		return;
	}

	if (check_blood_bond(Ind, Ind2)) {
		msg_format(Ind, "You are already blood bonded with %s.", p2_ptr->name);
		return;
	}

	/* To prevent an AFK player being teleported around: */
	if (p2_ptr->afk) {
		msg_print(Ind, "That player is currently AFK.");
		return;
	}

	/* protect players in inns */
	if ((zcave = getcave(&p2_ptr->wpos))) {
		c_ptr = &zcave[p2_ptr->py][p2_ptr->px];
		if (f_info[c_ptr->feat].flags1 & FF1_PROTECTED) {
			msg_print(Ind, "That player currently dwells on protected grounds.");
			return;
		}
	}

	MAKE(pl_ptr, player_list_type);
	pl_ptr->id = p2_ptr->id;
	if (p_ptr->blood_bond) {
		pl_ptr->next = p_ptr->blood_bond;
	} else {
		pl_ptr->next = NULL;
	}
	p_ptr->blood_bond = pl_ptr;

	MAKE(pl_ptr, player_list_type);
	pl_ptr->id = p_ptr->id;
	if (p_ptr->blood_bond) {
		pl_ptr->next = p2_ptr->blood_bond;
	} else {
		pl_ptr->next = NULL;
	}
	p2_ptr->blood_bond = pl_ptr;

	s_printf("BLOOD_BOND: %s blood bonds with %s\n", p_ptr->name, p2_ptr->name);
	msg_format(Ind, "\374\377cYou blood bond with %s.", p2_ptr->name);
	msg_format(Ind2, "\374%s blood bonds with you.", p_ptr->name);
	msg_broadcast_format(Ind, "\374\377c%s blood bonds with %s.", p_ptr->name, p2_ptr->name);

	/* new: auto-hostile, circumventing town peace zone functionality: */
	add_hostility(Ind, p2_ptr->name, TRUE);
	add_hostility(Ind2, p_ptr->name, FALSE);

	p_ptr->update |= PU_BONUS;
	p2_ptr->update |= PU_BONUS;
}

bool check_blood_bond(int Ind, int Ind2)
{
	player_type *p_ptr, *p2_ptr;
	player_list_type *pl_ptr;

	p_ptr = Players[Ind];
	p2_ptr = Players[Ind2];
	if (!p2_ptr) return FALSE;

	pl_ptr = p_ptr->blood_bond;
	while (pl_ptr) {
		if (pl_ptr->id == p2_ptr->id) return TRUE;
		pl_ptr = pl_ptr->next;
	}
	return FALSE;
}

void remove_blood_bond(int Ind, int Ind2)
{
	player_type *p_ptr, *p2_ptr;
	player_list_type *pl_ptr, *ppl_ptr;

	p_ptr = Players[Ind];
	p2_ptr = Players[Ind2];
	if (!p2_ptr) return;

	ppl_ptr = NULL;
	pl_ptr = p_ptr->blood_bond;
	while (pl_ptr)
	{
		if (pl_ptr->id == p2_ptr->id)
		{
			if (ppl_ptr)
			{
				ppl_ptr->next = pl_ptr->next;
			}
			else
			{
				/* First in the list */
				p_ptr->blood_bond = pl_ptr->next;
			}
			KILL(pl_ptr, player_list_type);
			return;
		}
		ppl_ptr = pl_ptr;
		pl_ptr = pl_ptr->next;
	}
}
	

bool telekinesis(int Ind, object_type *o_ptr)
{
  player_type *p_ptr = Players[Ind];

  p_ptr->current_telekinesis = o_ptr;
  get_item(Ind);

  return TRUE;
}

/* this has finally earned its own function, to make it easy for restoration to do this also */
bool do_scroll_life(int Ind)
{
	int x,y;
	
	player_type * p_ptr = Players[Ind], *q_ptr;
	cave_type * c_ptr;
	cave_type **zcave;
	zcave=getcave(&p_ptr->wpos);
	if(!zcave) return(FALSE);
	
	for (y = -1; y <= 1; y++)
	{
		for (x = -1; x <= 1; x++)
	 	{
	   		c_ptr = &zcave[p_ptr->py+y][p_ptr->px+x];
	  		if (c_ptr->m_idx < 0)
	   		{
				q_ptr=Players[0 - c_ptr->m_idx];
   				if (q_ptr->ghost)
   				{
					if (cave_floor_bold(zcave, p_ptr->py+y, p_ptr->px+x) &&
					    !(c_ptr->info & CAVE_ICKY))
					{
    						resurrect_player(0 - c_ptr->m_idx, 0);
	/* if player is not in town and resurrected on *TRUE* death level
	   then this is a GOOD action. Reward the player */
						if(!istown(&p_ptr->wpos) && getlevel(&p_ptr->wpos)==q_ptr->died_from_depth){
							u16b dal=1+((2*q_ptr->lev)/p_ptr->lev);
							if(p_ptr->align_good>dal)
								p_ptr->align_good-=dal;
							else p_ptr->align_good=0;
						}
	   			        	return TRUE;
					} else {
						msg_print(Ind, "The scroll fails here!");
					}
      				}
  			} 
  		}
  	}  	
  	/* we did nore ressurect anyone */
  	return FALSE; 
}


/* modified above function to instead restore XP... used in priest spell rememberence */
bool do_restoreXP_other(int Ind)
{
	int x,y;
	
	player_type * p_ptr = Players[Ind];
	cave_type * c_ptr;
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return(FALSE);
	
	for (y = -1; y <= 1; y++)
	{
		for (x = -1; x <= 1; x++)
	 	{
	   		c_ptr = &zcave[p_ptr->py+y][p_ptr->px+x];
	
	  		if (c_ptr->m_idx < 0)
	   		{
   				if (Players[0 - c_ptr->m_idx]->exp < Players[0 - c_ptr->m_idx]->max_exp)
   				{
    					restore_level(0 - c_ptr->m_idx);
   			        	return TRUE;
      				}
  			} 
  		}
  	}  	
  	/* we did nore ressurect anyone */
  	return FALSE; 
  }
  

/* Hack -- since the framerate has been boosted by five times since version
 * 0.6.0 to make game movement more smooth, we return the old level speed
 * times five to keep the same movement rate.
 */

void unstatic_level(struct worldpos *wpos){
	int i;

	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (Players[i]->st_anchor) {
			Players[i]->st_anchor = 0;
			msg_print(GetInd[Players[i]->id], "Your space/time anchor breaks");
		}
	}
	for (i = 1; i <= NumPlayers; i++){
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (inarea(&Players[i]->wpos, wpos)) {
			teleport_player_level(i);
		}
	}
	new_players_on_depth(wpos,0,FALSE);
}

/* these Dungeon Master commands should probably be added somewhere else, but I am
 * hacking them together here to start.
 */

/* static or unstatic a level */
bool master_level(int Ind, char * parms)
{
	int i;
	/* get the player pointer */
	player_type *p_ptr = Players[Ind];
	
	if (!is_admin(p_ptr)) return FALSE;

	switch (parms[0]) {
		/* unstatic the level */
		case 'u':
		{
			struct worldpos twpos;
			wpcopy(&twpos,&p_ptr->wpos);
			unstatic_level(&twpos);
       			msg_print(Ind, "The level has been unstaticed.");
			break;
		}

		/* static the level */
		case 's':
		{
			/* Increase the number of players on the dungeon 
			 * masters level by one. */
			new_players_on_depth(&p_ptr->wpos,1,TRUE);
			msg_print(Ind, "The level has been staticed.");
			break;
		}
		/* add dungeon stairs here */
		case 'D':
		{
			cave_type **zcave;
			u32b f1 = 0x0, f2 = 0x0;
			if(!parms[1] || !parms[2] || p_ptr->wpos.wz) return FALSE;
			if(istown(&p_ptr->wpos)){
				msg_print(Ind,"Even you may not create dungeons in the town!");
				return FALSE;
			}
			/* extract flags (note that 0x01 are reservd hacks to prevent zero byte) */
			if (parms[4] & 0x02) f1 |= DF1_FORGET;
			if (parms[5] & 0x02) f2 |= DF2_RANDOM;
			if (parms[5] & 0x04) f2 |= DF2_HELL;
			if (parms[5] & 0x08) f2 |= DF2_NO_MAGIC_MAP;
			if (parms[5] & 0x10) f2 |= DF2_IRON;
			if (parms[5] & 0x20) f2 |= DF2_TOWNS_IRONRECALL;
			if (parms[5] & 0x40) f2 |= DF2_TOWNS_RND;
			if (parms[5] & 0x80) f2 |= DF2_TOWNS_FIX;
			if (parms[6] & 0x04) f2 |= DF2_MISC_STORES;
			if (parms[6] & 0x08) {
				if (parms[6] & 0x10) f2 |= DF2_IRONRND1;
				if (parms[6] & 0x20) f2 |= DF2_IRONRND2;
				if (parms[6] & 0x40) f2 |= DF2_IRONRND3;
				if (parms[6] & 0x80) f2 |= DF2_IRONRND4;
			} else {
				if (parms[6] & 0x10) f2 |= DF2_IRONFIX1;
				if (parms[6] & 0x20) f2 |= DF2_IRONFIX2;
				if (parms[6] & 0x40) f2 |= DF2_IRONFIX3;
				if (parms[6] & 0x80) f2 |= DF2_IRONFIX4;
			}
			/* create tower or dungeon */
			if(parms[3] == 't' && !(wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags & WILD_F_UP)){
				printf("tower: flags %x,%x\n", f1, f2);
				add_dungeon(&p_ptr->wpos, parms[1], parms[2], f1, f2, TRUE, 0);
				new_level_down_y(&p_ptr->wpos, p_ptr->py);
				new_level_down_x(&p_ptr->wpos, p_ptr->px);
				if((zcave = getcave(&p_ptr->wpos))) {
					zcave[p_ptr->py][p_ptr->px].feat = FEAT_LESS;
				}
			}
			if(parms[3] == 'd' && !(wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags & WILD_F_DOWN)){
				printf("dungeon: flags %x,%x\n", f1, f2);
				add_dungeon(&p_ptr->wpos, parms[1], parms[2], f1, f2, FALSE, 0);
				new_level_up_y(&p_ptr->wpos, p_ptr->py);
				new_level_up_x(&p_ptr->wpos, p_ptr->px);
				if((zcave = getcave(&p_ptr->wpos))) {
					zcave[p_ptr->py][p_ptr->px].feat = FEAT_MORE;
				}
			}
			break;
		}
		case 'R':
		{
			/* Remove dungeon (here) if it exists */
			cave_type **zcave;
			if(!(zcave = getcave(&p_ptr->wpos))) break;

			/* either remove the dungeon we're currently in */
			if (p_ptr->wpos.wz) {
				if (p_ptr->wpos.wz < 0) {
					p_ptr->recall_pos.wx = p_ptr->wpos.wx;
					p_ptr->recall_pos.wy = p_ptr->wpos.wy;
					p_ptr->recall_pos.wz = 0;
					recall_player(Ind, "");
					rem_dungeon(&p_ptr->wpos, FALSE);
				} else {
					p_ptr->recall_pos.wx = p_ptr->wpos.wx;
					p_ptr->recall_pos.wy = p_ptr->wpos.wy;
					p_ptr->recall_pos.wz = 0;
					recall_player(Ind, "");
					rem_dungeon(&p_ptr->wpos, TRUE);
				}
			} else { /* or the one whose entrance staircase we're standing on */
				switch(zcave[p_ptr->py][p_ptr->px].feat){
					case FEAT_MORE:
						rem_dungeon(&p_ptr->wpos, FALSE);
						zcave[p_ptr->py][p_ptr->px].feat = FEAT_GRASS;
						break;
					case FEAT_LESS:
						rem_dungeon(&p_ptr->wpos, TRUE);
						zcave[p_ptr->py][p_ptr->px].feat = FEAT_GRASS;
						break;
					default:
						msg_print(Ind, "There is no dungeon here");
				}
			}
			break;
		}
		case 'T':
		{
			struct worldpos twpos;
			if(!parms[1] || p_ptr->wpos.wz) return FALSE;
			if(istown(&p_ptr->wpos)){
				msg_print(Ind, "There is already a town here!");
				return FALSE;
			}
			wpcopy(&twpos,&p_ptr->wpos);

			/* clean level first! */
			wipe_m_list(&p_ptr->wpos);
			wipe_o_list(&p_ptr->wpos);
//			wipe_t_list(&p_ptr->wpos);

			/* dont do this where there are houses! */
			for(i=0;i<num_houses;i++){
				if(inarea(&p_ptr->wpos, &houses[i].wpos)){
					houses[i].flags|=HF_DELETED;
				}
			}
			addtown(p_ptr->wpos.wy, p_ptr->wpos.wx, parms[1], 0, TOWN_VANILLA);
			unstatic_level(&twpos);
			if(getcave(&twpos))
				dealloc_dungeon_level(&twpos);

			break;
		}
		/* default -- do nothing. */
		default: break;
	}
	return TRUE;
}

/* static or unstatic a level (from chat-line command) */
bool master_level_specific(int Ind, struct worldpos *wpos, char * parms)
{
	/* get the player pointer */
	player_type *p_ptr = Players[Ind];
	
//	if (strcmp(p_ptr->name, cfg_dungeon_master)) return FALSE;
	if (!is_admin(p_ptr)) return FALSE;

	switch (parms[0])
	{
		/* unstatic the level */
		case 'u':
		{
			unstatic_level(wpos);
//       			msg_format(Ind, "The level (%d,%d) %dft has been unstaticed.", wpos->wx, wpos->wy, wpos->wz*50);
       			msg_format(Ind, "The level %s has been unstaticed.", wpos_format(Ind, wpos));
			break;
		}

		/* static the level */
		case 's':
		{
			/* Increase the number of players on the dungeon 
			 * masters level by one. */
			new_players_on_depth(&p_ptr->wpos,1,TRUE);
			msg_print(Ind, "The level has been staticed.");
			break;
		}
		/* default -- do nothing. */
		default: break;
	}
	return TRUE;
}


/*
 *
 * Guild build access 
 * Must be owner inside guild hall
 *
 */
//static bool guild_build(int Ind){
bool guild_build(int Ind){
	player_type *p_ptr=Players[Ind];
	int i;

	for(i=0;i<num_houses;i++){
		if(inarea(&houses[i].wpos, &p_ptr->wpos))
		{
			if(fill_house(&houses[i], FILL_PLAYER, p_ptr)){
				if(access_door(Ind, houses[i].dna, FALSE) || admin_p(Ind)){
					if(houses[i].dna->owner_type == OT_GUILD &&
					    p_ptr->guild == houses[i].dna->owner &&
					    guilds[p_ptr->guild].master == p_ptr->id){
						if(p_ptr->au > 1000){
							p_ptr->au -= 1000;
							p_ptr->redraw |= PR_GOLD;
							return(TRUE);
						}
					}
				}
				break;
			}
		}
	}
	return(FALSE);
}

/* Build walls and such.  This should probably be improved, I am just hacking
 * it together right now for Halloween. -APD
 */
bool master_build(int Ind, char * parms)
{
	player_type * p_ptr = Players[Ind];
	cave_type * c_ptr;
	struct c_special *cs_ptr;
	static unsigned char new_feat = FEAT_WALL_EXTRA;
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return(FALSE);

	if (!is_admin(p_ptr) && (!player_is_king(Ind)) && (!guild_build(Ind))) return FALSE;
	
	/* extract arguments, otherwise build a wall of type new_feat */
	if (parms)
	{
		/* Hack -- the first character specifies the type of wall */
		new_feat = parms[0];
		/* Hack -- toggle auto-build on/off */
		switch (parms[1])
		{
			case 'T': p_ptr->master_move_hook = master_build; break;
			case 'F': p_ptr->master_move_hook = NULL; break;
			default : break;
		}
	}

	c_ptr = &zcave[p_ptr->py][p_ptr->px];
	
	/* Never destroy real house doors! Work on this later */
	if((cs_ptr=GetCS(c_ptr, CS_DNADOOR))){
		return(FALSE);
	}

	/* This part to be rewritten for stacked CS */
	c_ptr->feat = new_feat;
	if(c_ptr->feat==FEAT_HOME){
		struct c_special *cs_ptr;
		/* new special door creation (with keys) */
		struct key_type *key;
		object_type newkey;
		int id;
		MAKE(key, struct key_type);
		sscanf(&parms[2],"%d",&id);
		key->id=id;
		invcopy(&newkey, lookup_kind(TV_KEY, 1));
		newkey.pval=key->id;
		newkey.marked2 = ITEM_REMOVAL_NEVER;
		drop_near(&newkey, -1, &p_ptr->wpos, p_ptr->py, p_ptr->px);
		cs_ptr=ReplaceCS(c_ptr, CS_KEYDOOR);
		if(cs_ptr){
			cs_ptr->sc.ptr=key;
		}
		else{
			KILL(key, struct key_type);
		}
		p_ptr->master_move_hook=NULL;	/*buggers up if not*/
	}
	if(c_ptr->feat==FEAT_SIGN){
		struct c_special *cs_ptr;
		struct floor_insc *sign;
		MAKE(sign, struct floor_insc);
		strcpy(sign->text, &parms[2]);
		cs_ptr=ReplaceCS(c_ptr, CS_INSCRIP);
		if(cs_ptr){
			cs_ptr->sc.ptr=sign;
		}
		else KILL(sign, struct floor_insc);
		p_ptr->master_move_hook=NULL;	/*buggers up if not*/
	}

	return TRUE;
}

static char master_specific_race_char = 'a';

static bool master_summon_specific_aux(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* no uniques */
	if (r_ptr->flags1 & RF1_UNIQUE) return FALSE;

	/* if we look like what we are looking for */
	if (r_ptr->d_char == master_specific_race_char) return TRUE;
	return FALSE;
}

/* Auxillary function to master_summon, determine the exact type of monster
 * to summon from a more general description.
 */
static u16b master_summon_aux_monster_type(int Ind, char monster_type, char * monster_parms)
{
	player_type *p_ptr = Players[Ind];
	int tmp, lev;

	/* handle each category of monster types */
	switch (monster_type)
	{
		/* specific monster specified */
		case 's':
		{
			/* allows specification by monster No. */
			tmp = atoi(monster_parms);
			if (tmp > 0) return tmp;

			/* if the name was specified, summon this exact race */
			if (strlen(monster_parms) > 1) return race_index(monster_parms);
			/* otherwise, summon a monster that looks like us */
			else
			{
				master_specific_race_char = monster_parms[0];
				get_mon_num_hook = master_summon_specific_aux;
				get_mon_num_prep(0, NULL);
//				tmp = get_mon_num(rand_int(100) + 10);
				lev = (monster_parms[0] == 't') ? 0 : rand_int(100) + 10;
				tmp = get_mon_num(lev, lev);

				/* restore monster generator */
				get_mon_num_hook = dungeon_aux;

				/* return our monster */
				return tmp;
			}
		}
		/* orc specified */
		case 'o':
		{
			/* if not random, assume specific orc specified */
			if (strcmp(monster_parms, "random")) return race_index(monster_parms);
			/* random orc */
			else switch(rand_int(6))
			{
				case 0: return race_index("Snaga");
				case 1: return race_index("Cave orc");
				case 2: return race_index("Hill orc");
				case 3: return race_index("Dark orc");
				case 4: return race_index("Half-orc");
				case 5: return race_index("Uruk");
			}
			break;
		}
		/* low undead specified */
		case 'u':
		{
			/* if not random, assume specific high undead specified */
			if (strcmp(monster_parms, "random")) return race_index(monster_parms);
			/* random low undead */
			else switch(rand_int(11))
			{
				case 0: return race_index("Poltergeist");
				case 1: return race_index("Green glutton ghost");
				case 2: return race_index("Lost soul");
				case 3: return race_index("Skeleton kobold");
				case 4: return race_index("Skeleton orc");
				case 5: return race_index("Skeleton human");
				case 6: return race_index("Zombified orc");
				case 7: return race_index("Zombified human");
				case 8: return race_index("Mummified orc");
				case 9: return race_index("Moaning spirit");
				case 10: return race_index("Vampire bat");
			}
			break;
		}
		
		/* high undead specified */
		case 'U':
		{
			/* if not random, assume specific high undead specified */
			if (strcmp(monster_parms, "random")) return race_index(monster_parms);
			/* random low undead */
			else switch(rand_int(13))
			{
				case 0: return race_index("Vampire");
				case 1: return race_index("Giant skeleton troll");
				case 2: return race_index("Lich");
				case 3: return race_index("Master vampire");
				case 4: return race_index("Dread");
				case 5: return race_index("Nether wraith");
				case 6: return race_index("Night mare");
				case 7: return race_index("Vampire lord");
				case 8: return race_index("Archpriest");
				case 9: return race_index("Undead beholder");
				case 10: return race_index("Dreadmaster");
				case 11: return race_index("Nightwing");
				case 12: return race_index("Nightcrawler");
			}
			break;
		}

		/* specific depth specified */
		case 'd':
		{
			get_mon_num_hook = dungeon_aux;

			/* Wilderness-specific hook */
			if (!p_ptr->wpos.wz) set_mon_num_hook_wild(&p_ptr->wpos);

			get_mon_num_prep(0, NULL);
			return get_mon_num(monster_parms[0], monster_parms[0] - 20); //reduce OoD if we summon depth-specificly
		}

		default : break;
	}

	/* failure */
	return 0;

}

/* Temporary debugging hack, to test the new excellents.
 */
bool master_acquire(int Ind, char * parms)
{
	player_type * p_ptr = Players[Ind];
	
	if (!is_admin(p_ptr)) return FALSE;
	acquirement(&p_ptr->wpos, p_ptr->py, p_ptr->px, 1, TRUE, TRUE, make_resf(p_ptr));
	return TRUE;
}

/* Monster summoning options. More documentation on this later. */
bool master_summon(int Ind, char * parms)
{
	int c;
	player_type * p_ptr = Players[Ind];

	static char monster_type = 0;  /* What type of monster we are -- specific, random orc, etc */
	static char monster_parms[80];
	static char summon_type = 0; /* what kind to summon -- x right here, group at random location, etc */
	static char summon_parms = 0; /* arguments to previous byte */
	static u16b r_idx = 0; /* which monser to actually summon, from previous variables */
	unsigned char size = 0;  /* how many monsters to actually summon */

	if (!is_admin(p_ptr) && (!player_is_king(Ind))) return FALSE;

	summon_override_checks = SO_ALL; /* set admin summoning flag for overriding all validity checks */

	/* extract arguments.  If none are found, summon previous type. */
	if (parms)
	{
		/* the first character specifies the type of monster */
		summon_type = parms[0];
		summon_parms = parms[1];
		monster_type = parms[2];
		/* Hack -- since monster_parms is a string, throw it on the end */
		strcpy(monster_parms, &parms[3]);
	}
	
	switch (summon_type)
	{
		/* summon x here */
		case 'x':
		{
			/* for each monster we are summoning */
			for (c = 0; c < summon_parms; c++)
			{
				/* figure out who to summon */
				r_idx = master_summon_aux_monster_type(Ind, monster_type, monster_parms);

				/* summon the monster, if we have a valid one */
				if (r_idx)
					summon_specific_race(&p_ptr->wpos, p_ptr->py, p_ptr->px, r_idx, 0, 1);
			}
			break;
		}

		/* summon x at random locations */
		case 'X':
		{
			for (c = 0; c < summon_parms; c++)
			{
				/* figure out who to summon */
				r_idx = master_summon_aux_monster_type(Ind, monster_type, monster_parms);

				/* summon the monster at a random location */
				if (r_idx)
					summon_specific_race_somewhere(&p_ptr->wpos,r_idx, 0, 1);
			}
			break;
		}

		/* summon group of random size here */
		case 'g':
		{
			/* figure out how many to summon */
			size = rand_int(rand_int(50)) + 2;

			/* figure out who to summon */
			r_idx = master_summon_aux_monster_type(Ind, monster_type, monster_parms);

			/* summon the group here */
			summon_specific_race(&p_ptr->wpos, p_ptr->py, p_ptr->px, r_idx, 0, size);
			break;
		}
		/* summon group of random size at random location */
		case 'G':
		{
			/* figure out how many to summon */
			size = rand_int(rand_int(50)) + 2;

			/* figure out who to summon */
			r_idx = master_summon_aux_monster_type(Ind, monster_type, monster_parms);

			/* someone the group at a random location */
			summon_specific_race_somewhere(&p_ptr->wpos, r_idx, 0, size);
			break;
		}
		/* summon mode on (use with discretion... lets not be TOO mean ;-) )*/
		case 'T':
		{	
			summon_type = 'x';
			summon_parms = 1;
			
			p_ptr->master_move_hook = master_summon;
			break;
		}

		/* summon mode off */
		case 'F':
		{
			p_ptr->master_move_hook = NULL;
			break;
		}
	}

	summon_override_checks = SO_NONE; /* clear all override flags */
	return TRUE;
}

bool imprison(int Ind, u16b time, char *reason){
	int id, i;
	struct dna_type *dna;
	player_type *p_ptr = Players[Ind];
	char string[160];
	cave_type **zcave, **nzcave;
	struct worldpos old_wpos;

	if (!p_ptr || !(id = lookup_player_id("Jailer"))) return (FALSE);

	if (!(zcave = getcave(&p_ptr->wpos))) return (FALSE);

	if (p_ptr->wpos.wz) {
		p_ptr->tim_susp += time;
		return (TRUE);
	}

	if (p_ptr->tim_jail) {
		p_ptr->tim_jail += time;
		return (TRUE);
	}

	for (i = 0; i < num_houses; i++) {
		if (!(houses[i].flags & HF_JAIL)) continue;
		dna = houses[i].dna;
		if (dna->owner == id && dna->owner_type == OT_PLAYER){
			/* lazy, single prison system */
			/* hopefully no overcrowding! */
			if (!(nzcave = getcave(&houses[i].wpos))){
				alloc_dungeon_level(&houses[i].wpos);
				generate_cave(&houses[i].wpos, p_ptr);
				/* nzcave=getcave(&houses[i].wpos); */
			}
			zcave[p_ptr->py][p_ptr->px].m_idx = 0;
			everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
			forget_lite(Ind);
			forget_view(Ind);
			wpcopy(&old_wpos, &p_ptr->wpos);
			wpcopy(&p_ptr->wpos, &houses[i].wpos);
			new_players_on_depth(&old_wpos, -1, TRUE);

			p_ptr->py = houses[i].y;
			p_ptr->px = houses[i].x;

			/* that messes it up */
			/* nzcave[p_ptr->py][p_ptr->px].m_idx=(0-Ind); */
			new_players_on_depth(&p_ptr->wpos, 1, TRUE);

			p_ptr->new_level_flag = TRUE;
			p_ptr->new_level_method = LEVEL_HOUSE;

			everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
			snprintf(string, sizeof(string), "\377v%s was jailed for %s", p_ptr->name, reason);
			msg_broadcast(Ind, string);
			msg_format(Ind, "\377vYou have been jailed for %s", reason);
			p_ptr->tim_jail = time + p_ptr->tim_susp;
			p_ptr->tim_susp = 0;
			
			return (TRUE);
		}
	}
	return (FALSE);
}

static void player_edit(char *name){
	
}

bool master_player(int Ind, char *parms){
	player_type *p_ptr=Players[Ind];
	player_type *q_ptr;
	int Ind2=0;
	int i;
	struct account *d_acc;
	int *id_list, n;

	if (!is_admin(p_ptr))
	{
		msg_print(Ind,"You need to be the dungeon master to use this command.");
		return FALSE;
	}
	switch(parms[0]){
		case 'E':	/* offline editor */
			for(i=1;i<=NumPlayers;i++){
				if(!strcmp(Players[i]->name,&parms[1])){
					msg_format(Ind,"%s is currently playing",&parms[1]);
					return(FALSE);
				}
			}
			player_edit(&parms[1]);

			break;
		case 'A':	/* acquirement */
			Ind2 = name_lookup(Ind, &parms[1], FALSE);
			if(Ind2)
			{
				player_type *p_ptr2 = Players[Ind2];
				acquirement(&p_ptr2->wpos, p_ptr2->py, p_ptr2->px, 1, TRUE, TRUE, !p_ptr2->total_winner);
				msg_format(Ind, "%s is granted an item.", p_ptr2->name);
				msg_print(Ind2, "You feel a divine favor!");
				return(FALSE);
			}
//			msg_print(Ind, "That player is not in the game.");
			break;
		case 'k':	/* admin wrath */
			Ind2 = name_lookup(Ind, &parms[1], FALSE);
			if(Ind2){
				q_ptr = Players[Ind2];
				msg_print(Ind2, "\377rYou are hit by a bolt from the blue!");
				strcpy(q_ptr->died_from,"divine wrath");
				//q_ptr->alive=FALSE;
				q_ptr->deathblow = 0;
				player_death(Ind2);
				return(TRUE);
			}
//			msg_print(Ind, "That player is not in the game.");

			break;
		case 'S':	/* Static a regular */
			stat_player(&parms[1], TRUE);
			break;
			
		case 'U':	/* Unstatic him */
			stat_player(&parms[1], FALSE);
			break;
		case 't':	/* DM telekinesis */
			/* I needed this before - it is useful */
			/* Unfortunately the current telekinesis */
			/* is not compatible with it, and I do not */
			/* want to combine it while there is a */
			/* potential bug. */
			break;
		case 'B':
			/* This could be fun - be wise dungeon master */
#ifdef TOMENET_WORLDS
			if (cfg.worldd_pubchat) world_msg(&parms[1]);
#endif
			msg_broadcast(0, &parms[1]);
			break;
		case 'r':	/* FULL ACCOUNT SCAN + RM */
			/* Delete a player from the database/savefile */
			d_acc = GetAccount(&parms[1], NULL, FALSE);
			if (d_acc != (struct account*)NULL) {
				char name[80];
				n = player_id_list(&id_list, d_acc->id);
				for(i = 0; i < n; i++) {
					strcpy(name, lookup_player_name(id_list[i]));
					msg_format(Ind, "\377oDeleting %s", name);
					delete_player_id(id_list[i]);
					sf_delete(name);
				}
				if (n) C_KILL(id_list, n, int);
				d_acc->flags|=ACC_DELD;
				/* stamp in the deleted account */
				WriteAccount(d_acc, FALSE);
				KILL(d_acc, struct account);
			}
			else
				msg_print(Ind, "\377rCould not find account");
			break;
	}
	return(FALSE);
}

static vault_type *get_vault(char *name)
{
	int i;
	
	for(i=0; i<MAX_V_IDX; i++)
	{
		if(strstr(v_name + v_info[i].name, name))
			return &v_info[i];
	}

	return NULL;
}

/* Generate something */
bool master_generate(int Ind, char * parms)
{
	/* get the player pointer */
	player_type *p_ptr = Players[Ind];
	
	if (!is_admin(p_ptr)) return FALSE;

	switch (parms[0])
	{
		/* generate a vault */
		case 'v':
		{
			vault_type *v_ptr = NULL;
			
			switch(parms[1])
			{
				case '#':
					v_ptr = &v_info[parms[2] + 127];
					break;
				case 'n':
					v_ptr = get_vault(&parms[2]);
			}
			
			if(!v_ptr || !v_ptr->wid) return FALSE;

//			build_vault(&p_ptr->wpos, p_ptr->py, p_ptr->px, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text);
			build_vault(&p_ptr->wpos, p_ptr->py, p_ptr->px, v_ptr, p_ptr);

			break;
		}
	}
	return TRUE;
}

#if 0 /* some new esp link stuff - mikaelh */
bool establish_esp_link(int Ind, int Ind2, byte type, u16b flags, u16b end)
{
	player_type *p_ptr, *p2_ptr;
	esp_link_type *esp_ptr;

	p_ptr = Players[Ind];
	p2_ptr = Players[Ind2];
	if (!p2_ptr) return;

	esp_ptr = check_esp_link(Ind, Ind2);
	if (esp_ptr) {
		if (esp_ptr->type == type) {
			/* compatible ESP link already exists, add flags */
			esp_ptr->flags |= flags;
			esp_ptr->end = end;
		}
		else return FALSE;
	}
	else {
		MAKE(esp_ptr, esp_link_type);

		esp_ptr->id = p2_ptr->id;
		esp_ptr->type = type;
		esp_ptr->flags = flags;
		esp_ptr->end = end;

		if (!(esp_ptr->flags & LINKF_HIDDEN))
		{
			msg_format(Ind, "\377oYou establish a mind link with %s.", p2_ptr->name);
			msg_format(Ind, "\377o%s has established a mind link with you.", p_ptr->name);
		}

		esp_ptr->next = p_ptr->esp_link;
		p_ptr->esp_link = esp_ptr;
	}

	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	p_ptr->update |= (PU_BONUS | PU_VIEW | PU_MANA | PU_HP);
	p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);
	p2_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	p2_ptr->update |= (PU_BONUS | PU_VIEW | PU_MANA | PU_HP);
	p2_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);

	return TRUE;
}

void break_esp_link(int Ind, int Ind2)
{
	player_type *p_ptr, *p2_ptr;
	esp_link_type *esp_ptr, *pest_link;

	p_ptr = Players[Ind];
	p2_ptr = Players[Ind2];
	if (!p2_ptr) return;

	pesp_ptr = NULL;
	esp_ptr = p_ptr->esp_link;
	while (esp_ptr)
	{
		if (esp_ptr->id == p2_ptr->id)
		{
			if (!(esp_ptr->flags & LINKF_HIDDEN)) {
				msg_format(Ind, "\377RYou break the mind link with %s.", p2_ptr->name);
				msg_format(Ind2, "\377R%s breaks the mind link with you.", p_ptr->name);
			}

			if (pesp_ptr)
			{
				pest_ptr->next = esp_ptr->next;
			}
			else
			{
				p_ptr->esp_link = esp_ptr->next;
			}
			KILL(esp_ptr, esp_link_type);

			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
			p_ptr->update |= (PU_BONUS | PU_VIEW | PU_MANA | PU_HP);
			p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);
			p2_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
			p2_ptr->update |= (PU_BONUS | PU_VIEW | PU_MANA | PU_HP);
			p2_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);
		}
		pesp_ptr = esp_ptr;
		esp_ptr = esp_ptr->next;
	}
}

esp_link_type *check_esp_link(ind Ind, int Ind2)
{
	player_type *p_ptr, *p2_ptr;
	esp_link_type *esp_ptr;

	p_ptr = Players[Ind];
	p2_ptr = Players[Ind2];
	if (!p2_ptr) return NULL;

	esp_ptr = p_ptr->esp_link;
	while (esp_ptr)
	{
		if (esp_ptr->id == p2_ptr->id) return esp_ptr;
		esp_ptr = esp_ptr->next;
	}
	return NULL;
}

bool check_esp_link_type(int Ind, int Ind2, u16b flags)
{
	esp_link_type* esp_ptr;
	esp_ptr = check_esp_link(Ind, Ind2);

	if (esp_ptr && esp_ptr->flags & flags) return TRUE;
	else return FALSE;
}
#endif

void toggle_aura(int Ind, int aura) {
	char buf[80];
	Players[Ind]->aura[aura] = !Players[Ind]->aura[aura];
	strcpy(buf, "\377sYour ");
	switch (aura) { /* up to MAX_AURAS */
	case 0: strcat(buf, "aura of fear"); break;
	case 1: strcat(buf, "shivering aura"); break;
	case 2: strcat(buf, "aura of death"); break;
	}
	strcat(buf, " is now ");
	if (Players[Ind]->aura[aura]) strcat(buf, "unleashed"); else strcat(buf, "suppressed");
	strcat(buf, "!");
	msg_print(Ind, buf);
}

void check_aura(int Ind, int aura) {
	char buf[80];
	strcpy(buf, "\377sYour ");
	switch (aura) { /* up to MAX_AURAS */
	case 0: strcat(buf, "aura of fear"); break;
	case 1: strcat(buf, "shivering aura"); break;
	case 2: strcat(buf, "aura of death"); break;
	}
	strcat(buf, " is currently ");
	if (Players[Ind]->aura[aura]) strcat(buf, "unleashed"); else strcat(buf, "suppressed");
	strcat(buf, ".");
	msg_print(Ind, buf);
}

int det_exp_level(int lev) {
	if (lev < 20) return(0);
        else if (lev < 30) return(375 / (45 - lev));
        else if (lev < 50) return(650 / (56 - lev));
        else return(lev * 2);
}
