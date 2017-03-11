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

#include <sys/time.h>

//#define IRON_TEAM_EXPERIENCE
/* todo before enabling: implement way to decrease party.experience again! */

/*
 * What % of exp points will be lost when resurrecting? [40]	- Jir -
 *
 * cf. GHOST_FADING in dungeon.c
 */
#ifdef ENABLE_INSTANT_RES
#define GHOST_XP_LOST	35
#else
#define GHOST_XP_LOST	40
#endif

/*
 * What % of exp points will be lost on instant resurrection?
 */
#define INSTANT_RES_XP_LOST	50

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

/* Reduce chance of inventory item destruction, to balance death for
   characters relying heavily on certain types of items? */
#define DEATH_PACK_ITEM_NICE

/* Loot item level is average of monster level and floor level - C. Blue
   Note: Imho the more floor level is taken into account, the more will
         melee chars who aim at high level weapons and armour be at a
         disadvantage, compared to light armour dropping everywhere.
         Also, floor level determines monster level of spawns anyway.
         However, contra side of disabling this is cheezy high unique
         farming on harmless floors via vaults for great loot -
         probably only affects melee chars though.
   Disabling this definition will cause loot to depend more on monster level. */
#define TRADITIONAL_LOOT_LEVEL

/* Should be defined to allow finding top-level items on non-NR floors
   in cases where you just cannot find monsters of high enough level so
   that averaging floor and monster level would pass the test.
   Details:
   Without this, if TRADITIONAL_LOOT_LEVEL is on, no loot above klevel 108
   could drop outside of NR anymore (Star Blade being highest normal monster
   at 90).
   Without TRADITIONAL_LOOT_LEVEL the situation becomes worse and usually no
   loot above klevel 102 could drop anymore (again Star Blades lv90 assumed).
   Note: Items of klevel > 115 usually cannot drop outside of NR, even with
   this on.
   EXCEPTIONS to all the above stuff:
   Note2: Objects still get an extra boost from being 'good' or 'great', of +10
   to object_level and from GREAT_OBJ which can even be +128. - C. Blue */
#define RANDOMIZED_LOOT_LEVEL

/* Level 50 limit for non-kings:    RECOMMENDED!  */
#define KINGCAP_LEV

/* exp limit for non-kings (level 50..69 depending on race/class) for non-kings: */
/*#define KINGCAP_EXP*/		/*  NOT RECOMMENDED to enable this!  */

/* Does NOT have effect if KINGCAP_EXP is defined.
   Total bonus skill points till level 50 for yeeks.
   All other class/race combinations will get less, down to 0 for
   max exp penalty of 472,5 %. Must be integer within 0..50. [25] */
#define WEAK_SKILLBONUS 0	/* NOT RECOMMENDED to set this > 0! */

/* Show real killer name even when hallucinating? */
#define SHOW_REALLY_DIED_FROM
/* In addition to above, show real killer instead of "insanity"? */
#ifdef SHOW_REALLY_DIED_FROM
 #define SHOW_REALLY_DIED_FROM_INSANITY
#endif

/* Killing a dungeon boss in IDDC will colour the asterisks l-dark instead of l-umber? */
//#define IDDC_BOSS_COL

/* Do player-kill messages of "Morgoth, Lord of Darkness" get some
   special flavour? - C. Blue */
#define MORGOTH_FUNKY_KILL_MSGS

/*
 * Modifier of semi-promised artifact drops, in percent.
 * It can happen that the quickest player will gather most of those
 * artifact; this can be used to defuse it somewhat. Won't affect '101%' chances.
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
	#define TRAD_SCROLL_MARGIN_ROW	(p_ptr->wide_scroll_margin ? 5 : 2)
	#define	SCROLL_MARGIN_ROW	(p_ptr->screen_hgt >= 26 ? \
					(p_ptr->wide_scroll_margin ? p_ptr->screen_hgt / 5 : p_ptr->screen_hgt / 13) : \
					TRAD_SCROLL_MARGIN_ROW)
	#define	SCROLL_MARGIN_COL	(p_ptr->wide_scroll_margin ? p_ptr->screen_wid / 5 : p_ptr->screen_wid / 13)
	#define TRAD_SCROLL_MARGIN_COL	SCROLL_MARGIN_COL
#else
	#define SCROLL_MARGIN_ROW 8
	#define SCROLL_MARGIN_COL 20
	#define TRAD_SCROLL_MARGIN_ROW	SCROLL_MARGIN_ROW
	#define TRAD_SCROLL_MARGIN_COL	SCROLL_MARGIN_COL
#endif

/* death_type definitions */
#define DEATH_PERMA	0
#define DEATH_INSANITY	1
#define DEATH_GHOST	2
#define DEATH_QUIT	3 /* suicide/retirement */


/* If during certain events, remember his/her account ID, for handing out a reward
   to a different character which he chooses on next login! - C. Blue
*/
static void buffer_account_for_event_deed(player_type *p_ptr, int death_type) {
	int i,j;

#if 0 /* why should this be enabled, hmm */
	for (i = 0; i < MAX_CONTENDER_BUFFERS; i++)
		if (ge_contender_buffer_ID[i] == p_ptr->account) return; /* player already has a buffer entry */
#endif
	for (i = 0; i < MAX_CONTENDER_BUFFERS; i++)
		if (ge_contender_buffer_ID[i] == 0) break;
	if (i == MAX_CONTENDER_BUFFERS) return; /* no free buffer entries anymore, sorry! */
	ge_contender_buffer_ID[i] = p_ptr->account;

	for (j = 0; j < MAX_GLOBAL_EVENTS; j++)
		switch (p_ptr->global_event_type[j]) {
		case GE_HIGHLANDER:
			if (p_ptr->global_event_progress[j][0] < 5) break; /* only rewarded if already in deathmatch phase! */
			if (death_type >= DEATH_QUIT) break; /* no reward for suiciding! */
			/* hand out the reward: */
			ge_contender_buffer_deed[i] = SV_DEED2_HIGHLANDER;
			return;
		case GE_DUNGEON_KEEPER:
			if (p_ptr->global_event_progress[j][0] < 1) break; /* only rewarded if actually already in the labyrinth! */
			if (death_type >= DEATH_QUIT) break; /* no reward for suiciding! */
			/* hand out the reward: */
			ge_contender_buffer_deed[i] = SV_DEED2_DUNGEONKEEPER;
			return;
		case GE_NONE:
		default:
			break;
		}

	ge_contender_buffer_ID[i] = 0; /* didn't find any event where player participated */
}

static void buffer_account_for_achievement_deed(player_type *p_ptr, int achievement) {
	int i;

#if 0 /* why should this be enabled, hmm */
	for (i = 0; i < MAX_ACHIEVEMENT_BUFFERS; i++)
		if (achievement_buffer_ID[i] == p_ptr->account) return; /* player already has a buffer entry */
#endif
	for (i = 0; i < MAX_ACHIEVEMENT_BUFFERS; i++)
		if (achievement_buffer_ID[i] == 0) break;
	if (i == MAX_ACHIEVEMENT_BUFFERS) return; /* no free buffer entries anymore, sorry! */
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
bool set_tim_thunder(int Ind, int v, int p1, int p2) {
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
bool set_tim_regen(int Ind, int v, int p) {
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
 * Set "p_ptr->tim_ffall"
 */
bool set_tim_ffall(int Ind, int v) {
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
 * Set "p_ptr->tim_lev"
 */
bool set_tim_lev(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_lev) {
			msg_print(Ind, "You feel light and your feet take off the ground.");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_lev) {
			msg_print(Ind, "You are suddenly a lot heavier.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_lev = v;

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
bool set_adrenaline(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

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
			if (!rand_int(5)) {
				msg_print(Ind, "Your adrenaline runs out, leaving you tired and weak.");
				do_dec_stat(Ind, A_CON, STAT_DEC_TEMPORARY);
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
bool set_biofeedback(int Ind, int v) {
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
					if (!rand_int(3)) set_stun_raw(Ind, p_ptr->stun + rand_int(30));
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
bool set_tim_esp(int Ind, int v) {
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
bool set_st_anchor(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->st_anchor) {
			//msg_print(Ind, "The Space/Time Continuum seems to solidify !");
			msg_print(Ind, "The air feels very still.");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->st_anchor) {
			//msg_print(Ind, "The Space/Time Continuum seems more flexible.");
			msg_print(Ind, "The air feels fresh again.");
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
bool set_prob_travel(int Ind, int v) {
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
 */
bool set_brand(int Ind, int v, int t, int p) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE, plural;
	char weapons[20];

	if (p_ptr->inventory[INVEN_WIELD].k_idx && /* dual-wield */
	    (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) {
		strcpy(weapons, "\377wYour weapons");
		plural = TRUE;
	} else if (p_ptr->inventory[INVEN_WIELD].k_idx ||
	    (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) {
		strcpy(weapons, "\377wYour weapon");
		plural = FALSE;
	} else {
		if (v) msg_print(Ind, "You are not wielding any melee weapons to brand."); /* failure */
		else { /* Shut */
			p_ptr->brand = 0;
			p_ptr->brand_t = 0;
			p_ptr->brand_d = 0;
		}
		return FALSE; /* don't notice anything */
	}

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->brand &&
		    (p_ptr->inventory[INVEN_WIELD].k_idx || /* dual-wield..*/
		    (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD))) {
			switch (t) {
				case TBRAND_BALL_ACID: //not used
				case TBRAND_ACID:
					if (plural) msg_format(Ind, "%s are branded with acid!", weapons);
					else msg_format(Ind, "%s is branded with acid!", weapons);
				break;
				case TBRAND_BALL_ELEC: //not used
				case TBRAND_ELEC:
					if (plural) msg_format(Ind, "%s are branded with lightning!", weapons);
					else msg_format(Ind, "%s is branded with lightning!", weapons);
				break;
				case TBRAND_BALL_FIRE: //not used
				case TBRAND_FIRE:
					if (plural) msg_format(Ind, "%s are branded with fire!", weapons);
					else msg_format(Ind, "%s is branded with fire!", weapons);
				break;
				case TBRAND_BALL_COLD: //not used
				case TBRAND_COLD:
					if (plural) msg_format(Ind, "%s are branded with frost!", weapons);
					else msg_format(Ind, "%s is branded with frost!", weapons);
				break;
				case TBRAND_POIS:
					if (plural) msg_format(Ind, "%s are branded with poison!", weapons);
					else msg_format(Ind, "%s is branded with poison!", weapons);
				break;
				case TBRAND_BASE:
					if (plural) msg_format(Ind, "%s glow in many colours!", weapons); //not used
					else msg_format(Ind, "%s glows in many colours!", weapons); //not used
				break;
				case TBRAND_CHAO:
					if (plural) msg_format(Ind, "%s seem to twist and warp!", weapons); //not used
					else msg_format(Ind, "%s seems to twist and warp!", weapons); //not used
				break;
				case TBRAND_VORP:
					if (plural) msg_format(Ind, "%s sharpen!", weapons); //not used
					else msg_format(Ind, "%s sharpens!", weapons); //not used
				break;
				case TBRAND_BALL_SOUN:
					if (plural) msg_format(Ind, "%s vibrate!", weapons); //not used
					else msg_format(Ind, "%s vibrates!", weapons); //not used
				break;
			}
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->brand) {
			switch (t) {
				case TBRAND_BALL_ACID: //not used
				case TBRAND_ACID:
					if (plural) msg_format(Ind, "\377WYour weapons are no longer branded with \377sacid.");
					else msg_format(Ind, "\377WYour weapon is no longer branded with \377sacid.");
				break;
				case TBRAND_BALL_ELEC: //not used
				case TBRAND_ELEC:
					if (plural) msg_format(Ind, "\377WYour weapons are no longer branded with \377blightning.");
					else msg_format(Ind, "\377WYour weapon is no longer branded with \377blightning.");
				break;
				case TBRAND_BALL_FIRE: //not used
				case TBRAND_FIRE:
					if (plural) msg_format(Ind, "\377WYour weapons are no longer branded with \377rfire.");
					else msg_format(Ind, "\377WYour weapon is no longer branded with \377rfire.");
				break;
				case TBRAND_BALL_COLD: //not used
				case TBRAND_COLD:
					if (plural) msg_format(Ind, "\377WYour weapons are no longer branded with \377wfrost.");
					else msg_format(Ind, "\377WYour weapon is no longer branded with \377wfrost.");
				break;
				case TBRAND_POIS:
					if (plural) msg_format(Ind, "\377WYour weapons are no longer branded with \377gpoison.");
					else msg_format(Ind, "\377WYour weapon is no longer branded with \377gpoison.");
				break;
				default:
					if (plural) msg_print(Ind, "\377WYour weapons are no longer branded.");
					else msg_print(Ind, "\377WYour weapon is no longer branded.");
				break;
			}
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
bool set_bow_brand(int Ind, int v, int t, int p) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	if (!p_ptr->inventory[INVEN_AMMO].k_idx) {
		if (v) msg_print(Ind, "Your quiver does not hold any ammunition to brand."); /* failure */
		else { /* Shut */
			p_ptr->bow_brand = 0;
			p_ptr->bow_brand_t = 0;
			p_ptr->bow_brand_d = 0;
		}
		return FALSE; /* don't notice anything */
	}

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->bow_brand) {
			switch (t) {
			case TBRAND_ELEC:
			case TBRAND_BALL_ELEC:
				msg_print(Ind, "\377oYour ammunition sparkles with lightnings!");
				break;
			case TBRAND_BALL_COLD:
			case TBRAND_COLD:
				msg_print(Ind, "\377oYour ammunition freezes!");
				break;
			case TBRAND_BALL_FIRE:
			case TBRAND_FIRE:
				msg_print(Ind, "\377oYour ammunition burns!");
				break;
			case TBRAND_BALL_ACID:
			case TBRAND_ACID:
				msg_print(Ind, "\377oYour ammunition drips acid!");
				break;
			case TBRAND_POIS:
				msg_print(Ind, "\377oYour ammunition is covered with venom!");
				break;
			case TBRAND_BASE:
				msg_print(Ind, "\377oYour ammunition glows in many colours!");
				break;
			case TBRAND_CHAO:
				msg_print(Ind, "\377oYour ammunition seems to twist and warp!");
				break;
			case TBRAND_VORP:
				msg_print(Ind, "\377oYour ammunition sharpens!");
				break;
			case TBRAND_BALL_SOUN:
				msg_print(Ind, "\377oYour ammunition vibrates!");
				break;
			}
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->bow_brand) {
			msg_print(Ind, "\377oThe branding magic on your ammunition ceases again.");
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
bool set_mimic(int Ind, int v, int p) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* force good values */
	if (v < 0) v = 0;

	/* Open */
	if (v) {
		if (!p_ptr->tim_mimic) {
			msg_print(Ind, "Your form changes!");
			notice = TRUE;
		} else if (p_ptr->tim_mimic > 100 && v <= 100) {
			msg_print(Ind, "\376\377LThe magical force stabilizing your form starts to fade...");
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_mimic && p_ptr->body_monster == p_ptr->tim_mimic_what) {
			//msg_print(Ind, "\376\377LYour form changes back to normal.");
			msg_print(Ind, "\376\377yYour form changes back to normal!");
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
bool set_tim_manashield(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_manashield) {
			msg_print(Ind, "\376\377wA purple shimmering shield forms around your body!");
			notice = TRUE;
		} else if (p_ptr->tim_manashield > 15 && v <= 15) {
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
bool set_tim_traps(int Ind, int v) {
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
bool set_invis(int Ind, int v, int p) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_invisibility && !p_ptr->invis) { //only give message if not already invis
			msg_format_near(Ind, "%s fades in the shadows!", p_ptr->name);
			msg_print(Ind, "You fade in the shadow!");
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->tim_invisibility && !p_ptr->tim_invis_power) { //only give message if no static invis anyway
			msg_format_near(Ind, "The shadows enveloping %s dissipate.", p_ptr->name);
			msg_print(Ind, "The shadows enveloping you dissipate.");
			notice = TRUE;
		}
		p = 0;
	}

	/* Use the value */
	p_ptr->tim_invisibility = v;
	p_ptr->tim_invis_power2 = p;

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
bool set_fury(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->fury) {
			if (p_ptr->shero) {
				msg_print(Ind, "You cannot grow additional fury while in a berserk rage!");
				return FALSE;
			}
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
bool set_tim_meditation(int Ind, int v) {
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
bool set_tim_wraith(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;
	cave_type **zcave;
	dun_level *l_ptr = getfloor(&p_ptr->wpos);
	if (!(zcave = getcave(&p_ptr->wpos))) return FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->tim_wraith) {
			if ((zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) ||
			    (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC))) {
				msg_print(Ind, "You feel different for a moment.");
				v = 0;
			} else {
				msg_format_near(Ind, "%s turns into a wraith!", p_ptr->name);
				msg_print(Ind, "You turn into a wraith!");
				notice = TRUE;

				p_ptr->wraith_in_wall = TRUE;
				p_ptr->redraw |= PR_BPR_WRAITH;
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
			zcave = getcave(&p_ptr->wpos);

			/* prevent running out of wraithform if we have a permanent source -> refresh it */
			if (zcave && in_bounds(p_ptr->py, p_ptr->px)) {
				if (p_ptr->body_monster && (r_info[p_ptr->body_monster].flags2 & RF2_PASS_WALL)) v = 10000;
				else {
					/* if a worn item grants wraith form, don't let it run out */
					u32b f1, f2, f3, f4, f5, f6, esp;
					object_type *o_ptr;
					int i;

					/* Scan the usable inventory */
					for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
						o_ptr = &p_ptr->inventory[i];

						/* Skip missing items */
						if (!o_ptr->k_idx) continue;

						/* Extract the item flags */
						object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
						if (f3 & (TR3_WRAITH)) {
							//p_ptr->wraith_form = TRUE;
							v = 10000;
							break;
						}
					}
				}
				if (v != 10000) {
					msg_format_near(Ind, "%s loses %s wraith powers.", p_ptr->name, p_ptr->male ? "his":"her");
					msg_print(Ind, "You lose your wraith powers.");
					p_ptr->redraw |= PR_BPR_WRAITH;
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
bool set_blind(int Ind, int v) {
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
			disturb(Ind, 1, 0); /* stop running and searching */
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
bool set_confused(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	if (p_ptr->martyr && v) return FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	if (get_skill(p_ptr, SKILL_MIND) >= 30) v /= 2;

	/* Open */
	if (v) {
		if (!p_ptr->confused) {
			disturb(Ind, 1, 0); /* stop running and searching */
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
bool set_poisoned(int Ind, int v, int attacker) {
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
bool set_afraid(int Ind, int v) {
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
bool set_paralyzed(int Ind, int v) {
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
bool set_image(int Ind, int v) {
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
bool set_fast(int Ind, int v, int p) {
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
bool set_slow(int Ind, int v) {
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
bool set_shield(int Ind, int v, int p, s16b o, s16b d1, s16b d2) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->shield) {
			switch (o) {
				case SHIELD_ICE:
					msg_print(Ind, "You are shielded by grinding ice!");
				break;
				case SHIELD_PLASMA:
					msg_print(Ind, "You are shielded by searing plasma!");
				break;
				default:
					msg_print(Ind, "A mystic shield forms around your body!");
				break;
			}
			notice = TRUE;
		}
	}

	/* Shut */
	else {
		if (p_ptr->shield) {
			switch (o) {
				case SHIELD_ICE:
					msg_print(Ind, "\377WYou are no longer shielded by \377Bice.");
				break;
				case SHIELD_PLASMA:
					msg_print(Ind, "\377WYou are no longer shielded by \377Rplasma.");
				break;
				default:
					msg_print(Ind, "Your mystic shield crumbles away.");
				break;
			}
			notice = TRUE;
		}
	}


	/* Use the value */
	p_ptr->shield = v;
	if (p_ptr->shield_power != p) notice = TRUE;
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

/*
 * Set "p_ptr->blessed", notice observable changes
 */
bool set_blessed(int Ind, int v) {
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

bool set_res_fear(int Ind, int v) {
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
bool set_hero(int Ind, int v) {
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
bool set_shero(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->shero) {
			if (p_ptr->fury) {
				msg_print(Ind, "The berserk rage replaces your fury!");
				set_fury(Ind, 0);
			}
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


bool set_melee_sprint(int Ind, int v) {
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
bool set_protevil(int Ind, int v) {
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
bool set_zeal(int Ind, int p, int v) {
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

bool set_martyr(int Ind, int v) {
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
			msg_format_near(Ind, "\377w%s burns in holy fire!", p_ptr->name);
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
			//p_ptr->chp = (p_ptr->mhp >= 30 * 15) ? 30 : p_ptr->mhp / 15;
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
	//Send_martyr(Ind); -- we're using halved output damage of all DISP damage types instead of this mana-doubling
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->invuln", notice observable changes
 */
bool set_invuln(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->invuln) {
			p_ptr->invuln_dur = v;
			msg_print(Ind, "\377A powerful iridescent shield forms around your body!");
			notice = TRUE;
		} else if (p_ptr->invuln > 5 && v <= 5) {
			msg_print(Ind, "\376\377vThe invulnerability shield starts to fade...");
		}
	}

	/* Shut */
	else {
		if (p_ptr->invuln &&
		    p_ptr->invuln_dur >= 5) { /* avoid spam on stair-GoI */
			msg_print(Ind, "\376\377vThe invulnerability shield fades away.");
		}
		notice = TRUE;
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
bool set_invuln_short(int Ind, int v) {
	player_type *p_ptr = Players[Ind];

	/* not cumulative */
	if (p_ptr->invuln > v) return (FALSE);

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Use the value */
	p_ptr->invuln = v;
	p_ptr->invuln_dur = v;

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
bool set_tim_invis(int Ind, int v) {
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
bool set_tim_infra(int Ind, int v) {
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
bool set_oppose_acid(int Ind, int v) {
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
bool set_oppose_elec(int Ind, int v) {
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
bool set_oppose_fire(int Ind, int v) {
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
bool set_oppose_cold(int Ind, int v) {
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
bool set_oppose_pois(int Ind, int v) {
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


/* like set_stun() but ignoring stance defense */
bool set_stun_raw(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	int old_aux, new_aux;
	bool notice = FALSE;

	if (p_ptr->martyr && v) return FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

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
			msg_format_near(Ind, "\377o%s is heavily stunned.", p_ptr->name);
			msg_print(Ind, "\377oYou have been heavily stunned.");
			break;

			/* Knocked out */
			case 3:
			msg_format_near(Ind, "%s has been knocked out.", p_ptr->name);
			msg_print(Ind, "\377rYou have been knocked out.");
			s_printf("%s EFFECT: Knockedout %s.\n", showtime(), p_ptr->name);
			p_ptr->energy = 0;/* paranoia, couldn't reproduce it so far - don't allow him a final action with his rest of energy */
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
 * Set "p_ptr->stun", notice observable changes
 *
 * Note the special code to only notice "range" changes.
 */
bool set_stun(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	int old_aux, new_aux;
	bool notice = FALSE;

	if (p_ptr->martyr && v) return FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* New/Experimental: Defensive stance helps! */
	if (p_ptr->combat_stance == 1)
		switch (p_ptr->combat_stance_power) {
		case 0: v = (v * 7 + 1) / 8; break;
		case 1: v = (v * 5 + 1) / 6; break;
		case 2: v = (v * 3 + 1) / 4; break;
		case 3: v = (v * 2 + 1) / 3; break;
		}

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
			msg_format_near(Ind, "\377o%s is heavily stunned.", p_ptr->name);
			msg_print(Ind, "\377oYou have been heavily stunned.");
			break;

			/* Knocked out */
			case 3:
			msg_format_near(Ind, "%s has been knocked out.", p_ptr->name);
			msg_print(Ind, "\377rYou have been knocked out.");
			s_printf("%s EFFECT: Knockedout %s.\n", showtime(), p_ptr->name);
			p_ptr->energy = 0;/* paranoia, couldn't reproduce it so far - don't allow him a final action with his rest of energy */
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
bool set_cut(int Ind, int v, int attacker) {
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

bool set_mindboost(int Ind, int p, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;


	/* Redraw the Blows/Round if any */
	if (v && p_ptr->mindboost_power != p) p_ptr->update |= PU_BONUS;

	/* set boni/EA */
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
			msg_print(Ind, "\376\377wYou create a kinetic barrier.");
			notice = TRUE;
		} else if (p_ptr->kinetic_shield > 10 && v <= 10) {
			msg_print(Ind, "\376\377vThe kinetic barrier starts to destabilize...");
		}
	}

	/* Shut */
	else {
		if (p_ptr->kinetic_shield) {
			msg_print(Ind, "\376\377vYour kinetic barrier destabilizes.");
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

#ifdef ENABLE_OCCULT
bool set_savingthrow(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = (FALSE);

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->temp_savingthrow) {
			msg_print(Ind, "You feel safe from peril.");
			notice = (TRUE);
		}
	}

	/* Shut */
	else { //v = 0;
		if (p_ptr->temp_savingthrow) {
			msg_print(Ind, "You no longer feel safe from peril.");
			notice = (TRUE);
		}
	}

	p_ptr->temp_savingthrow = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

bool set_spirit_shield(int Ind, int power, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->spirit_shield) {
			p_ptr->spirit_shield_pow = power;
			msg_print(Ind, "\376\377wYou feel the spirits watching over you.");
			notice = TRUE;
		} else if (p_ptr->spirit_shield > 10 && v <= 10) {
			msg_print(Ind, "\376\377vYou feel the spirits weakening");
		}
	}

	/* Shut */
	else {
		if (p_ptr->spirit_shield) {
			msg_print(Ind, "\376\377vYou feel the spirits watching over you are disappearing.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->spirit_shield = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}
#endif

#ifdef ENABLE_MAIA
/* timed hp bonus for RACE_MAIA */
bool do_divine_hp(int Ind, int p, int v) {
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

/* timed crit bonus for RACE_MAIA. */
bool do_divine_crit(int Ind, int p, int v) {
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

/* timed time and mana res bonus for RACE_MAIA. */
bool do_divine_xtra_res_time(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = (FALSE);

	/* Hack -- Force good values */
	v = (v > cfg.spell_stack_limit) ? cfg.spell_stack_limit : (v < 0) ? 0 : v;

	/* Open */
	if (v) {
		if (!p_ptr->divine_xtra_res_time) {
			msg_print(Ind, "You feel resistant to \377Btime\377w.");
			notice = (TRUE);
		}
	}

	/* Shut */
	else { //v = 0;
		if (p_ptr->divine_xtra_res_time) {
			msg_print(Ind, "You feel less resistant to \377Btime\377w.");
			notice = (TRUE);
		}
	}

	p_ptr->divine_xtra_res_time = v;

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
#else
bool do_divine_hp(int Ind, int p, int v) {
	return FALSE;
}
bool do_divine_crit(int Ind, int p, int v) {
	return FALSE;
}
bool do_divine_xtra_res_time(int Ind, int v) {
	return FALSE;
}
#endif

/*
 * Set "p_ptr->tim_deflect", notice observable changes
 */
bool set_tim_deflect(int Ind, int v) {
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

/*
 * Set "p_ptr->sh_fire/cold/elec", notice observable changes
 */
bool set_sh_fire_tim(int Ind, int v) {
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
bool set_sh_cold_tim(int Ind, int v) {
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
bool set_sh_elec_tim(int Ind, int v) {
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
bool set_food(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	int old_aux, new_aux;
	bool notice = FALSE;

	/* True Ghosts don't starve */
	if ((p_ptr->ghost) || (get_skill(p_ptr, SKILL_HSUPPORT) == 50) ||
	    (p_ptr->prace == RACE_MAIA && p_ptr->ptrait)) {
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

	/* Ents will never get gorged, but can still go hungry/thirsty */
	if (p_ptr->prace == RACE_ENT && v >= PY_FOOD_MAX ) {
		v = PY_FOOD_MAX - 1;
	}

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
			if (p_ptr->warning_hungry != 2) {
				p_ptr->warning_hungry = 2;
				if (p_ptr->prace == RACE_VAMPIRE) {
					msg_print(Ind, "\374\377RWARNING: You are 'weak' from hunger. Drink some blood by killing monsters");
					msg_print(Ind, "\374\377R         monsters in melee (close combat). Town monsters will work too.");
				} else if (p_ptr->prace == RACE_ENT) {
					msg_print(Ind, "\374\377RWARNING: You are 'weak' from hunger. Find something to drink or rest");
					msg_print(Ind, "\374\377R         (\377oSHIFT+r\377R) on earth/dirt/grass/water floor tiles for a while.");
				} else {
					msg_print(Ind, "\374\377RWARNING: You are 'weak' from hunger. Press \377oSHIFT+e\377R to eat something");
					msg_print(Ind, "\374\377R         or read a 'scroll of satisfy hunger' if you have one.");
				}
				s_printf("warning_hungry(weak): %s\n", p_ptr->name);
			}
			break;

			/* Hungry */
			case 2:
			msg_print(Ind, "You are getting hungry.");
			if (p_ptr->warning_hungry == 0) {
				p_ptr->warning_hungry = 1;
				if (p_ptr->prace == RACE_VAMPIRE) {
					msg_print(Ind, "\374\377oWARNING: Your character is 'hungry'. Drink some blood by killing some");
					msg_print(Ind, "\374\377o         monsters in melee (close combat). Town monsters will work too.");
				} else if (p_ptr->prace == RACE_ENT) {
					msg_print(Ind, "\374\377oWARNING: Your character is 'hungry'. Find something to drink or rest ");
					msg_print(Ind, "\374\377o         (\377RSHIFT+r\377o) on earth/dirt/grass/water floor tiles for a while.");
				} else {
					msg_print(Ind, "\374\377oWARNING: Your character is 'hungry'. Press \377RSHIFT+e\377o to eat something");
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
bool bless_temp_luck(int Ind, int pow, int dur) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;

	if (!p_ptr->bless_temp_luck) {
		msg_print(Ind, "\376\377gYou feel luck is on your side.");
		notice = TRUE;
	}
	if (!dur) {
		msg_print(Ind, "\376\377gYour lucky streak fades.");
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

#ifdef ENABLE_MAIA
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
#ifdef ENABLE_HELLKNIGHT /* and also for ENABLE_CPRIEST */
/* Don't multiply but set a skill's base value and modifier to a fixed value */
static void do_Maia_skill2(int Ind, int s, int v, int m) {
	/* Save old skill value */
	s32b val = Players[Ind]->s_info[s].value, tmp_val = Players[Ind]->s_info[s].mod;

	/* Release invested points */
	respec_skill(Ind, s, FALSE, FALSE);

	/* Modify skill, avoiding overflow (mod is u16b) */
	Players[Ind]->s_info[s].value = v;
	/* Cap to 2.0 */
	if (m > 2000) m = 2000;
	Players[Ind]->s_info[s].mod = m;

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
#endif
/* Change Maia skill chart after initiation */
void shape_Maia_skills(int Ind) {
	player_type *p_ptr = Players[Ind];

	switch (p_ptr->ptrait) {
	case TRAIT_CORRUPTED:
#ifdef ENABLE_HELLKNIGHT
		if (p_ptr->pclass == CLASS_PALADIN) {
			respec_skill(Ind, SKILL_HOFFENSE, FALSE, FALSE);
			Send_skill_info(Ind, SKILL_HOFFENSE, FALSE);
			respec_skill(Ind, SKILL_HCURING, FALSE, FALSE);
			Send_skill_info(Ind, SKILL_HCURING, FALSE);
			respec_skill(Ind, SKILL_HDEFENSE, FALSE, FALSE);
			p_ptr->s_info[SKILL_HDEFENSE].value = 0; //Paladin starts with +1000 in this skill
			Send_skill_info(Ind, SKILL_HDEFENSE, FALSE);
			respec_skill(Ind, SKILL_HSUPPORT, FALSE, FALSE);
			Send_skill_info(Ind, SKILL_HSUPPORT, FALSE);

			//hardcoded skill modifiers below...ugh (keep consistent with class template in tables.c)

			/* Fix some skills that have changed from the Paladin template:
			   hereticism; axe, polearm, blunt; all bloodmagic */
 #ifdef ENABLE_OHERETICISM
			p_ptr->s_info[SKILL_SCHOOL_OCCULT].dev = TRUE; //expand Occultism, to ensure the player notices it on the skill chart
			do_Maia_skill2(Ind, SKILL_OHERETICISM, 0, ((700 * 7) / 10 * 21) / 10);
 #endif
			respec_skill(Ind, SKILL_BLUNT, FALSE, FALSE);
			p_ptr->s_info[SKILL_BLUNT].mod = 0;
			Send_skill_info(Ind, SKILL_BLUNT, FALSE);
			do_Maia_skill2(Ind, SKILL_AXE, 0, (700 * 13) / 10);
			do_Maia_skill2(Ind, SKILL_POLEARM, 0, 700);

			p_ptr->s_info[SKILL_BLOOD_MAGIC].dev = TRUE; //expand Blood Magic, to ensure the player notices it on the skill chart
			do_Maia_skill2(Ind, SKILL_TRAUMATURGY, 0, (1500 * 7) / 10 * 3);
			do_Maia_skill2(Ind, SKILL_NECROMANCY, 0, (1300 * 7) / 10 * 3);
			do_Maia_skill2(Ind, SKILL_AURA_FEAR, 0, (1400 * 7) / 10 * 3);
			do_Maia_skill2(Ind, SKILL_AURA_SHIVER, 0, (1400 * 7) / 10 * 3);
			do_Maia_skill2(Ind, SKILL_AURA_DEATH, 0, (1300 * 7) / 10 * 3);
			Send_reliable(p_ptr->conn);
		}
#endif
#ifdef ENABLE_CPRIEST
		if (p_ptr->pclass == CLASS_PRIEST) {
			respec_skill(Ind, SKILL_HOFFENSE, FALSE, FALSE);
			Send_skill_info(Ind, SKILL_HOFFENSE, FALSE);
			respec_skill(Ind, SKILL_HCURING, FALSE, FALSE);
			p_ptr->s_info[SKILL_HCURING].value = 0; //Priest starts with +1000 in this skill
			Send_skill_info(Ind, SKILL_HCURING, FALSE);
			respec_skill(Ind, SKILL_HDEFENSE, FALSE, FALSE);
			Send_skill_info(Ind, SKILL_HDEFENSE, FALSE);
			respec_skill(Ind, SKILL_HSUPPORT, FALSE, FALSE);
			Send_skill_info(Ind, SKILL_HSUPPORT, FALSE);

			//hardcoded skill modifiers below...ugh (keep consistent with class template in tables.c)

			/* Fix some skills that have changed from the Paladin template:
			   hereticism; axe, polearm, blunt; all bloodmagic */
 #ifdef ENABLE_OHERETICISM /* (should actually always be defined if Corrupted Priests are enabled) */
			p_ptr->s_info[SKILL_SCHOOL_OCCULT].dev = TRUE; //expand Occultism, to ensure the player notices it on the skill chart
			do_Maia_skill2(Ind, SKILL_OHERETICISM, 0, ((1150 * 7) / 10 * 21) / 10);
 #endif
			respec_skill(Ind, SKILL_BLUNT, FALSE, FALSE);
			p_ptr->s_info[SKILL_BLUNT].mod = 0;
			Send_skill_info(Ind, SKILL_BLUNT, FALSE);
			/* Note: Corrupted trait doesn't give sword bonus but axe,
			   but since Enlightened trait gives priests melee bonus (blunt), we need Corrupted Priests to be on par,
			   as if their Blunt skill was just transferred to become Sword.. a bit inconsistent :/ */
			//do_Maia_skill2(Ind, SKILL_SWORD, 0, 600);
			do_Maia_skill2(Ind, SKILL_SWORD, 0, (600 * 13) / 10);

			p_ptr->s_info[SKILL_BLOOD_MAGIC].dev = TRUE; //expand Blood Magic, to ensure the player notices it on the skill chart
			do_Maia_skill2(Ind, SKILL_TRAUMATURGY, 0, (1400 * 7) / 10 * 3);
			do_Maia_skill2(Ind, SKILL_NECROMANCY, 0, (1400 * 7) / 10 * 3);
			do_Maia_skill2(Ind, SKILL_AURA_FEAR, 0, (1500 * 7) / 10 * 3);
			do_Maia_skill2(Ind, SKILL_AURA_SHIVER, 0, (1300 * 7) / 10 * 3);
			do_Maia_skill2(Ind, SKILL_AURA_DEATH, 0, (1300 * 7) / 10 * 3);
			Send_reliable(p_ptr->conn);
		}
#endif

		p_ptr->s_info[SKILL_HOFFENSE].mod = 0;
		p_ptr->s_info[SKILL_HCURING].mod = 0;
		p_ptr->s_info[SKILL_HDEFENSE].mod = 0;
		p_ptr->s_info[SKILL_HSUPPORT].mod = 0;

#ifdef ENABLE_OCCULT
		do_Maia_skill(Ind, SKILL_OSHADOW, 21);
		//respec_skill(Ind, SKILL_OSPIRIT, FALSE, FALSE);
		p_ptr->s_info[SKILL_OSPIRIT].mod = 0;
 #ifdef ENABLE_OHERETICISM
  #ifdef ENABLE_HELLKNIGHT
		if (p_ptr->pclass != CLASS_PALADIN
   #ifdef ENABLE_CPRIEST
		    && p_ptr->pclass != CLASS_PRIEST
   #endif
		    )
			do_Maia_skill(Ind, SKILL_OHERETICISM, 21); //for Paladins/Priests it's been handled above already
  #endif
 #endif
#endif

		/* Yay */
		do_Maia_skill(Ind, SKILL_FIRE, 17);
		do_Maia_skill(Ind, SKILL_AIR, 17);
		do_Maia_skill(Ind, SKILL_CONVEYANCE, 17);
		do_Maia_skill(Ind, SKILL_UDUN, 20);
#ifdef ENABLE_HELLKNIGHT /* blood magic skills were already converted above; this code here is just for 'normal' classes */
	    if (p_ptr->pclass != CLASS_PALADIN
 #ifdef ENABLE_CPRIEST
	     && p_ptr->pclass != CLASS_PRIEST
 #endif
	     ) {
#endif
		do_Maia_skill(Ind, SKILL_TRAUMATURGY, 30);
		do_Maia_skill(Ind, SKILL_NECROMANCY, 30);
		do_Maia_skill(Ind, SKILL_AURA_FEAR, 30);
		do_Maia_skill(Ind, SKILL_AURA_SHIVER, 30);
		do_Maia_skill(Ind, SKILL_AURA_DEATH, 30);
		do_Maia_skill(Ind, SKILL_AXE, 13);
#ifdef ENABLE_HELLKNIGHT
	    }
#endif
		do_Maia_skill(Ind, SKILL_MARTIAL_ARTS, 13);
		do_Maia_skill(Ind, SKILL_R_DARK, 17);
		do_Maia_skill(Ind, SKILL_R_NETH, 17);

#ifdef ENABLE_HELLKNIGHT
		if (p_ptr->pclass == CLASS_PALADIN) {
			p_ptr->pclass = CLASS_HELLKNIGHT;
			p_ptr->cp_ptr = &class_info[p_ptr->pclass];
			p_ptr->redraw |= PR_BASIC; //PR_TITLE;
			clockin(Ind, 10); //stamp class
			everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
		}
#endif
#ifdef ENABLE_CPRIEST
		if (p_ptr->pclass == CLASS_PRIEST) {
			p_ptr->pclass = CLASS_CPRIEST;
			p_ptr->cp_ptr = &class_info[p_ptr->pclass];
			p_ptr->redraw |= PR_BASIC; //PR_TITLE;
			clockin(Ind, 10); //stamp class
			everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
		}
#endif

		break;

	case TRAIT_ENLIGHTENED:
		/* Doh! */
#if 0
		respec_skill(Ind, SKILL_TRAUMATURGY, FALSE, FALSE);
		respec_skill(Ind, SKILL_NECROMANCY, FALSE, FALSE);
		respec_skill(Ind, SKILL_AURA_DEATH, FALSE, FALSE);
#endif
		p_ptr->s_info[SKILL_TRAUMATURGY].mod = 0;
		p_ptr->s_info[SKILL_NECROMANCY].mod = 0;
		p_ptr->s_info[SKILL_AURA_DEATH].mod = 0;

#ifdef ENABLE_OCCULT
		p_ptr->s_info[SKILL_OSHADOW].mod = 0;
		do_Maia_skill(Ind, SKILL_OSPIRIT, 21);
 #ifdef ENABLE_OHERETICISM
		p_ptr->s_info[SKILL_OHERETICISM].mod = 0;
 #endif
#endif

		/* Yay */
		do_Maia_skill(Ind, SKILL_AURA_FEAR, 30);
		do_Maia_skill(Ind, SKILL_AURA_SHIVER, 30);
		do_Maia_skill(Ind, SKILL_HOFFENSE, 21);
		do_Maia_skill(Ind, SKILL_HCURING, 21);
		do_Maia_skill(Ind, SKILL_HDEFENSE, 21);
		do_Maia_skill(Ind, SKILL_HSUPPORT, 21);
		do_Maia_skill(Ind, SKILL_DIVINATION, 17);
		do_Maia_skill(Ind, SKILL_SWORD, 13);
		do_Maia_skill(Ind, SKILL_BLUNT, 13);
		do_Maia_skill(Ind, SKILL_POLEARM, 13);
		do_Maia_skill(Ind, SKILL_R_LITE, 17);
		do_Maia_skill(Ind, SKILL_R_MANA, 17);
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
static void check_training(int Ind) {
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
void check_experience(int Ind) {
	player_type *p_ptr = Players[Ind];
	char str[160];

	bool newlv = FALSE, reglv = FALSE;
	int old_lev, i;
	//long int i;
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

		clockin(Ind, 1); /* Set player level */

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
		if (p_ptr->inval && p_ptr->lev >= 25) {
			msg_print(Ind, "\377rYou cannot gain level further, wait for an admin to validate your account.");
			break;
			//return;
		}

		process_hooks(HOOK_PLAYER_LEVEL, "d", Ind);

		/* Gain a level */
		p_ptr->lev++;

		clockin(Ind, 1); /* Set player level */

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
			if (is_older_than(&p_ptr->version, 4, 4, 8, 5, 0, 0)) p_ptr->redraw |= PR_STUDY;
			p_ptr->update |= PU_SKILL_MOD;

			newlv = TRUE;

			/* give some stats, using Training skill */
			check_training(Ind);
		} else {
			/* Player just regained a level he lost previously */
			reglv = TRUE;
		}

#ifdef USE_SOUND_2010
		/* see further below! */
#else
		sound(Ind, SOUND_LEVEL);
#endif
	}
	/* log regaining of levels */
	if (reglv) s_printf("%s has regained level %d.\n", p_ptr->name, p_ptr->lev);


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

	/* Try to own items we previously couldn't own */
	if (old_lev < p_ptr->lev) {
		object_type *o_ptr;

		for (i = 0; i < INVEN_PACK; i++) {
			o_ptr = &p_ptr->inventory[i];
			if (!o_ptr->k_idx) continue;
			if (o_ptr->owner == p_ptr->id) continue;
			can_use(Ind, o_ptr);
		}
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

	if (!newlv) {
		/* Handle stuff */
		handle_stuff(Ind);

		return;
	}


	/* Message */
	msg_format(Ind, "\374\377GWelcome to level %d. You have %d skill points.", p_ptr->lev, p_ptr->skill_points);
	if (!is_admin(p_ptr)) {
		if (p_ptr->lev == 99) l_printf("%s \\{U*** \\{g%s has attained level 99 \\{U***\n", showdate(), p_ptr->name);
		else if (p_ptr->lev >= 90) l_printf("%s \\{g%s has attained level %d\n", showdate(), p_ptr->name, p_ptr->lev);
		else if (old_lev < 80 && p_ptr->lev >= 80) l_printf("%s \\{g%s has attained level 80\n", showdate(), p_ptr->name);
		else if (old_lev < 70 && p_ptr->lev >= 70) l_printf("%s \\{g%s has attained level 70\n", showdate(), p_ptr->name);
		else if (old_lev < 60 && p_ptr->lev >= 60) l_printf("%s \\{g%s has attained level 60\n", showdate(), p_ptr->name);
	}
#ifdef USE_SOUND_2010
	sound(Ind, "levelup", NULL, SFX_TYPE_MISC, FALSE);
#endif

	snprintf(str, 160, "\374\377G%s has attained level %d.", p_ptr->name, p_ptr->lev);
	s_printf("%s has attained level %d.\n", p_ptr->name, p_ptr->lev);
	clockin(Ind, 1);	/* Set player level */
#ifdef TOMENET_WORLDS
	if (cfg.worldd_lvlup) world_msg(str);
#endif
	msg_broadcast(Ind, str);


	/* Disable certain warnings on reaching certain levels */
	disable_lowlevel_warnings(p_ptr);

	/* Give helpful msg about how to distribute skill points at first level-up */
	if (p_ptr->newbie_hints && (old_lev == 1 || (p_ptr->skill_points == (p_ptr->max_plv - 1) * SKILL_NB_BASE))) {
	    // && p_ptr->inval) /* (p_ptr->warning_skills) */
		msg_print(Ind, "\374\377GHINT: Press \377gSHIFT+g\377G to distribute your skill points!");
	}

	if (p_ptr->warning_cloak == 2 && p_ptr->lev >= 15)
		msg_print(Ind, "\374\377GHINT: You can press \377gSHIFT+v\377G to cloak your appearance.");

	/* Tell player to use numpad to move diagonally */
	if (old_lev < 2 && p_ptr->lev >= 2 && p_ptr->warning_numpadmove == 0) {
		msg_print(Ind, "\374\377yHINT: Use the number pad keys to move, that way you can move \377odiagonally\377y too!");
		msg_print(Ind, "\374\377y      (Or try using the \377orogue-like\377y key set, press \377o= 1 y\377y to enable it.)");
		s_printf("warning_numpadmove: %s\n", p_ptr->name);
		p_ptr->warning_numpadmove = 1;
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
	if (old_lev < 10 && p_ptr->lev >= 10 && !p_ptr->warning_macros) {
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
			msg_print(Ind, "\374\377oHINT: Create '\377Rmacros\377o' aka hotkeys to ensure survival in critical situations!");
			msg_print(Ind, "\374\377o      Press '\377R%\377o' and then '\377Rz\377o' to start the macro wizard.");
			s_printf("warning_macros (levup): %s\n", p_ptr->name);
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
			    //p_ptr->inventory[i].sval == SV_POTION_CURE_LIGHT ||
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
			msg_print(Ind, "\374\377yHINT: Press '\377om\377y' to access 'Fighting Techniques' and 'Shooting Techniques'!");
			msg_print(Ind, "\374\377y      You can also create macros for these. See the TomeNET guide for details.");
			s_printf("warning_techniques: %s\n", p_ptr->name);
		} else if (p_ptr->warning_technique_melee == 0) {
			msg_print(Ind, "\374\377yHINT: Press '\377om\377y' to access 'Fighting Techniques'!");
			msg_print(Ind, "\374\377y      You can also create macros for these. See the TomeNET guide for details.");
			s_printf("warning_technique_melee: %s\n", p_ptr->name);
		} else if (p_ptr->warning_technique_ranged == 0) {
			msg_print(Ind, "\374\377yHINT: Press '\377om\377y' to access 'Shooting Techniques'!");
			msg_print(Ind, "\374\377y      You can also create macros for these. See the TomeNET guide for details.");
			s_printf("warning_technique_ranged: %s\n", p_ptr->name);
		}
		p_ptr->warning_technique_melee = p_ptr->warning_technique_ranged = 1;
	}

	/* Give warning message to get phase/tele means, aimed at newbies */
	if (old_lev < 18 && p_ptr->lev >= 18 && p_ptr->warning_tele == 0) {
#if 1
		/* scan inventory for any scrolls/staves */
		bool found_items = FALSE;
		int i;
		for (i = 0; i < INVEN_PACK; i++) {
			if (!p_ptr->inventory[i].k_idx) continue;
			if (!object_known_p(Ind, &p_ptr->inventory[i])) continue;
			if (p_ptr->inventory[i].tval == TV_SCROLL && (
			    p_ptr->inventory[i].sval == SV_SCROLL_TELEPORT))
				found_items = TRUE;
			if (p_ptr->inventory[i].tval == TV_STAFF && (
			    p_ptr->inventory[i].sval == SV_STAFF_TELEPORTATION))
				found_items = TRUE;
		}
		if (!found_items)
#endif
		{
			msg_print(Ind, "\374\377oHINT: Buy scrolls of teleportation from the \377RBlack Market\377o or a staff");
			msg_print(Ind, "\374\377o      of teleportation from the \377RMagic Shop\377o to get out of trouble!");
			msg_print(Ind, "\374\377o      Ideally, create a \377Rmacro\377o for using them by a single key press.");
			s_printf("warning_tele: %s\n", p_ptr->name);
		}
		p_ptr->warning_tele = 1;
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
	case RACE_DRACONIAN:
		if (old_lev < 5 && p_ptr->lev >= 5) msg_print(Ind, "\374\377GYou learn to telepathically sense dragons!");
#ifndef ENABLE_DRACONIAN_TRAITS
		if (old_lev < 10 && p_ptr->lev >= 10) msg_print(Ind, "\374\377GYou become more resistant to fire!");
		if (old_lev < 15 && p_ptr->lev >= 15) msg_print(Ind, "\374\377GYou become more resistant to frost!");
		if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYou become more resistant to acid!");
		if (old_lev < 25 && p_ptr->lev >= 25) msg_print(Ind, "\374\377GYou become more resistant to lightning!");
#else
		if (old_lev < 8 && p_ptr->lev >= 8) msg_print(Ind, "\374\377GYou learn how to breathe an element!");
		switch (p_ptr->ptrait) {
		case TRAIT_BLUE: /* Draconic Blue */
			if (old_lev < 5 && p_ptr->lev >= 5) msg_print(Ind, "\374\377GYour attacks are branded by lightning!");
			if (old_lev < 15 && p_ptr->lev >= 15) msg_print(Ind, "\374\377GYou are enveloped in lightning!");
			if (old_lev < 25 && p_ptr->lev >= 25) msg_print(Ind, "\374\377GYou no longer fear electricity!");
			break;
		case TRAIT_WHITE: /* Draconic White */
			if (old_lev < 15 && p_ptr->lev >= 15) msg_print(Ind, "\374\377GYou are enveloped by freezing air!");
			if (old_lev < 25 && p_ptr->lev >= 25) msg_print(Ind, "\374\377GYou no longer fear frost!");
			break;
		case TRAIT_RED: /* Draconic Red */
			if (old_lev < 25 && p_ptr->lev >= 25) msg_print(Ind, "\374\377GYou no longer fear fire!");
			break;
		case TRAIT_BLACK: /* Draconic Black */
			if (old_lev < 25 && p_ptr->lev >= 25) msg_print(Ind, "\374\377GYou no longer fear acid!");
			break;
		case TRAIT_GREEN: /* Draconic Green */
			if (old_lev < 25 && p_ptr->lev >= 25) msg_print(Ind, "\374\377GYou no longer fear poison!");
			break;
		case TRAIT_MULTI: /* Draconic Multi-hued */
			if (old_lev < 5 && p_ptr->lev >= 5) msg_print(Ind, "\374\377GYou develop intrinsic resistance to electricity!");
			if (old_lev < 10 && p_ptr->lev >= 10) msg_print(Ind, "\374\377GYou develop intrinsic resistance to frost!");
			if (old_lev < 15 && p_ptr->lev >= 15) msg_print(Ind, "\374\377GYou develop intrinsic resistance to fire!");
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYou develop intrinsic resistance to acid!");
			if (old_lev < 25 && p_ptr->lev >= 25) msg_print(Ind, "\374\377GYou develop intrinsic resistance to poison!");
			break;
		case TRAIT_BRONZE: /* Draconic Bronze */
			if (old_lev < 5 && p_ptr->lev >= 5) msg_print(Ind, "\374\377GYou develop intrinsic resistance to confusion!");
			if (old_lev < 10 && p_ptr->lev >= 10) {
				msg_print(Ind, "\374\377GYou develop intrinsic resistance to electricity!");
				msg_print(Ind, "\374\377GYou develop intrinsic resistance to paralysis!");
			}
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYour scales have grown metallic enough to reflect attacks!");
			break;
		case TRAIT_SILVER: /* Draconic Silver */
			if (old_lev < 5 && p_ptr->lev >= 5) msg_print(Ind, "\374\377GYou develop intrinsic resistance to frost!");
			if (old_lev < 10 && p_ptr->lev >= 10) msg_print(Ind, "\374\377GYou develop intrinsic resistance to acid!");
			if (old_lev < 15 && p_ptr->lev >= 15) msg_print(Ind, "\374\377GYou develop intrinsic resistance to poison!");
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYour scales have grown metallic enough to reflect attacks!");
			break;
		case TRAIT_GOLD: /* Draconic Gold */
			if (old_lev < 5 && p_ptr->lev >= 5) msg_print(Ind, "\374\377GYou develop intrinsic resistance to fire!");
			if (old_lev < 10 && p_ptr->lev >= 10) msg_print(Ind, "\374\377GYou develop intrinsic resistance to acid!");
			if (old_lev < 15 && p_ptr->lev >= 15) msg_print(Ind, "\374\377GYou develop intrinsic resistance to sound!");
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYour scales have grown metallic enough to reflect attacks!");
			break;
		case TRAIT_LAW: /* Draconic Law */
			if (old_lev < 5 && p_ptr->lev >= 5) msg_print(Ind, "\374\377GYou develop intrinsic resistance to shards!");
			if (old_lev < 10 && p_ptr->lev >= 10) msg_print(Ind, "\374\377GYou develop intrinsic resistance to paralysis!");
			if (old_lev < 15 && p_ptr->lev >= 15) msg_print(Ind, "\374\377GYou develop intrinsic resistance to sound!");
			break;
		case TRAIT_CHAOS: /* Draconic Chaos */
			if (old_lev < 5 && p_ptr->lev >= 5) msg_print(Ind, "\374\377GYou develop intrinsic resistance to confusion!");
			if (old_lev < 15 && p_ptr->lev >= 15) msg_print(Ind, "\374\377GYou develop intrinsic resistance to chaos!");
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYou develop intrinsic resistance to disenchantment!");
			break;
		case TRAIT_BALANCE: /* Draconic Balance */
			if (old_lev < 10 && p_ptr->lev >= 10) msg_print(Ind, "\374\377GYou develop intrinsic resistance to disenchantment!");
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYou develop intrinsic resistance to sound!");
			break;
		case TRAIT_POWER: /* Draconic Power */
			if (old_lev < 5 && p_ptr->lev >= 5) msg_print(Ind, "\374\377GYou develop intrinsic resistance to blindness!");
			if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYour scales have grown metallic enough to reflect attacks!");
			break;
		}
#endif
		if (old_lev < 30 && p_ptr->lev >= 30) msg_print(Ind, "\374\377GYou learn how to levitate!");
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
		//if (old_lev < 30 && p_ptr->lev >= 30) msg_print(Ind, "\374\377GYou learn how to levitate!");
		if (old_lev < 20 && p_ptr->lev >= 20) {
			msg_print(Ind, "\374\377GYou are now able to turn into a vampire bat (#391)!");
			msg_print(Ind, "\374\377G(Press '\377gm\377G' key and choose '\377guse innate power\377G' to polymorph.)");
		}
#ifdef VAMPIRIC_MIST
		if (old_lev < 35 && p_ptr->lev >= 35) msg_print(Ind, "\374\377GYou are now able to turn into vampiric mist (#365)!");
#endif
		break;
#ifdef ENABLE_MAIA
	case RACE_MAIA:
		if (p_ptr->ptrait) break; /* In case we got *bad* exp drain for some unfathomable reason ;) */
		if (old_lev < 12 && p_ptr->lev >= 12) msg_print(Ind, "\374\377GWe all have to pick our own path some time...");
		//if (old_lev < 14 && p_ptr->lev >= 14) msg_print(Ind, "\374\377GYou are thirsty for blood: be it good or evil");
		if (old_lev < 14 && p_ptr->lev >= 14) msg_print(Ind, "\374\377GYour soul thirsts for shaping, either enlightenment or corruption!");
		if (old_lev <= 19 && p_ptr->lev >= 15 && old_lev < p_ptr->lev) {
			/* Killed none? nothing happens except if we reached the threshold level */
			if (p_ptr->r_killed[RI_CANDLEBEARER] == 0
			    && p_ptr->r_killed[RI_DARKLING] == 0) {
				/* Threshold level has been overstepped -> die */
				if (old_lev <= 19 && p_ptr->lev >= 20) {
					//msg_print(Ind, "\377RYou don't deserve to live.");
					msg_print(Ind, "\377RYour indecision proves you aren't ready yet to stay in this realm!");
					strcpy(p_ptr->died_from, "indecisiveness");
					p_ptr->deathblow = 0;
					p_ptr->death = TRUE;
				}
				/* We're done here.. */
				break;
			}

			/* Killed both? -> you die */
			if (p_ptr->r_killed[RI_CANDLEBEARER] != 0 &&
			    p_ptr->r_killed[RI_DARKLING] != 0) {
				msg_print(Ind, "\377RYour indecision proves you aren't ready yet to stay in this realm!");
				strcpy(p_ptr->died_from, "indecisiveness");
				p_ptr->deathblow = 0;
				p_ptr->death = TRUE;
				/* End of story, next.. */
				break;
			}

			/* Don't initiate earlier than at threshold level, it's a ceremony ^^ */
			if (p_ptr->lev <= 19) break;

			/* Modify skill tree */
			if (p_ptr->r_killed[RI_CANDLEBEARER] != 0) {
				//A demon appears!
				msg_print(Ind, "\374\377p*** \377GYour corruption grows well within you. \377p***");
				p_ptr->ptrait = TRAIT_CORRUPTED;
				msg_print(Ind, "\374\377GYou resist attacks based on fire or darkness!");
			} else {
				//An angel appears!
				msg_print(Ind, "\374\377p*** \377sYou have been ordained to be order in presence of chaos. \377p***");
				p_ptr->ptrait = TRAIT_ENLIGHTENED;
				msg_print(Ind, "\374\377GYou resist light-based attacks and you can now see the invisible!");
			}

			shape_Maia_skills(Ind);
			calc_techniques(Ind);

			p_ptr->redraw |= PR_SKILLS | PR_MISC;
			p_ptr->update |= PU_SKILL_INFO | PU_SKILL_MOD;
		}
		if (old_lev < 20 && p_ptr->lev >= 20)
			msg_print(Ind, "\374\377GYou don't require worldly food anymore to sustain your body.");
		if (old_lev < 50 && p_ptr->lev >= 50)
			//todo maybe?: aura msgs
			switch (p_ptr->ptrait) {
			case TRAIT_ENLIGHTENED:
				msg_print(Ind, "\374\377GYou become intrinsically resistant to poison, lightning and frost!");
				msg_print(Ind, "\374\377GYou gain intrinsic levitation powers.");
				break;
			case TRAIT_CORRUPTED:
				msg_print(Ind, "\374\377GYou become intrinsically resistant to poison and completely immune to fire!");
				break;
		}
		break;
#endif
	}


	/* those that depend on a class */
	switch (p_ptr->pclass) {
	case CLASS_ROGUE:
#ifdef ENABLE_CLOAKING
		if (old_lev < LEARN_CLOAKING_LEVEL && p_ptr->lev >= LEARN_CLOAKING_LEVEL) {
			msg_print(Ind, "\374\377GYou learn how to cloak yourself to pass unnoticed (press '\377gSHIFT+v\377G').");
			if (!p_ptr->warning_cloak) p_ptr->warning_cloak = 2;
		}
#endif
		break;
	case CLASS_RANGER:
		if (old_lev < 15 && p_ptr->lev >= 15) msg_print(Ind, "\374\377GYou learn how to move through dense forests easily.");
		if (old_lev < 25 && p_ptr->lev >= 25) msg_print(Ind, "\374\377GYou learn how to swim well, with heavy backpack even.");
		break;
	case CLASS_DRUID: /* Forms gained by Druids */
		/* compare mimic_druid in defines.h */
		if (old_lev < 5 && p_ptr->lev >= 5) {
			msg_print(Ind, "\374\377GYou learn how to change into a Cave Bear (#160) and Panther (#198)");
			msg_print(Ind, "\374\377G(Press '\377gm\377G' key and choose '\377guse innate power\377G' to polymorph.)");
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
		break;
	case CLASS_SHAMAN:
		if (old_lev < 20 && p_ptr->lev >= 20
		    && p_ptr->prace != RACE_ENT && p_ptr->prace != RACE_DARK_ELF
		    && (p_ptr->prace != RACE_MAIA || p_ptr->ptrait != TRAIT_ENLIGHTENED))
			msg_print(Ind, "\374\377GYou learn to see the invisible!");
		break;
	case CLASS_MINDCRAFTER:
		if (old_lev < 10 && p_ptr->lev >= 10
		    && p_ptr->prace != RACE_VAMPIRE)
			msg_print(Ind, "\374\377GYou learn to keep hold of your sanity!");
		if (old_lev < 20 && p_ptr->lev >= 20) msg_print(Ind, "\374\377GYou learn to keep strong hold of your sanity!");
		break;
	}

	/* learn fighting techniques */
	if (old_lev < mtech_lev[p_ptr->pclass][0] && p_ptr->lev >= mtech_lev[p_ptr->pclass][0])
		msg_print(Ind, "\374\377GYou learn the fighting technique 'Sprint'! (press '\377gm\377G')");
	if (old_lev < mtech_lev[p_ptr->pclass][1] && p_ptr->lev >= mtech_lev[p_ptr->pclass][1])
		msg_print(Ind, p_ptr->pclass == CLASS_MINDCRAFTER ?
		    "\374\377GYou learn the fighting technique 'Taunt'! (press '\377gm\377G')" :
		    "\374\377GYou learn the fighting technique 'Taunt'!");
	if (old_lev < mtech_lev[p_ptr->pclass][3] && p_ptr->lev >= mtech_lev[p_ptr->pclass][3])
		msg_print(Ind, "\374\377GYou learn the fighting technique 'Distract'!");
	if (old_lev < mtech_lev[p_ptr->pclass][4] && p_ptr->lev >= mtech_lev[p_ptr->pclass][4])
		msg_print(Ind, "\374\377GYou learn the fighting technique 'Apply Poison'!");
	if (old_lev < mtech_lev[p_ptr->pclass][6] && p_ptr->lev >= mtech_lev[p_ptr->pclass][6])
		msg_print(Ind, "\374\377GYou learn the fighting technique 'Track Animals'!");
	if (old_lev < mtech_lev[p_ptr->pclass][7] && p_ptr->lev >= mtech_lev[p_ptr->pclass][7])
		msg_print(Ind, "\374\377GYou learn the fighting technique 'Perceive Noise'!");
	if (old_lev < mtech_lev[p_ptr->pclass][8] && p_ptr->lev >= mtech_lev[p_ptr->pclass][8])
		msg_print(Ind, "\374\377GYou learn the fighting technique 'Flash Bomb'!");
	if (old_lev < mtech_lev[p_ptr->pclass][10] && p_ptr->lev >= mtech_lev[p_ptr->pclass][10])
		msg_print(Ind, "\374\377GYou learn the fighting technique 'Spin'!");
#ifdef ENABLE_ASSASSINATE
	if (old_lev < mtech_lev[p_ptr->pclass][11] && p_ptr->lev >= mtech_lev[p_ptr->pclass][11])
		msg_print(Ind, "\374\377GYou learn the fighting technique 'Assasinate'!");
#endif
	if (old_lev < mtech_lev[p_ptr->pclass][12] && p_ptr->lev >= mtech_lev[p_ptr->pclass][12])
		msg_print(Ind, "\374\377GYou learn the fighting technique 'Berserk'!");
	if (old_lev < mtech_lev[p_ptr->pclass][14] && p_ptr->lev >= mtech_lev[p_ptr->pclass][14]
	    && p_ptr->total_winner)
		msg_print(Ind, "\374\377GYou learn the royal fighting technique 'Shadow Run'!");

	/* update the client's 'm' menu for fighting techniques */
	calc_techniques(Ind);
	Send_skill_info(Ind, SKILL_TECHNIQUE, TRUE);


#ifdef ENABLE_STANCES
	/* increase SKILL_STANCE by +1 automatically (just for show :-p) if we actually have that skill */
	if (get_skill(p_ptr, SKILL_STANCE) && p_ptr->lev <= 50) {
		p_ptr->s_info[SKILL_STANCE].value = p_ptr->lev * 1000;
		/* Update the client */
		Send_skill_info(Ind, SKILL_STANCE, TRUE);
		/* give message if we learn a new stance (compare cmd6.c! keep it synchronized */
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
	//if (p_ptr->lev == 50 && !p_ptr->total_winner) msg_print(Ind, "\374\377GYou can't gain more levels until you defeat Morgoth, Lord of Darkness!");
	if (p_ptr->lev == 50 && !p_ptr->total_winner) msg_print(Ind, "\374\377G* To level up further, you must defeat Morgoth, Lord of Darkness! *");
#endif

	/* pvp mode cant go higher, but receives a reward maybe */
	if (p_ptr->mode & MODE_PVP) {
		if (get_skill(p_ptr, SKILL_MIMIC) &&
		    p_ptr->pclass != CLASS_DRUID &&
		    p_ptr->prace != RACE_VAMPIRE) {
			msg_print(Ind, "\375\377GYou gain one free mimicry transformation of your choice!");
			p_ptr->free_mimic = 1;
		}
		if (old_lev < MID_PVP_LEVEL && p_ptr->lev >= MID_PVP_LEVEL) {
			msg_broadcast_format(Ind, "\374\377G* %s has raised in ranks greatly! *", p_ptr->name);
			msg_print(Ind, "\375\377G* You have raised quite a bit in ranks of PvP characters!         *");
			msg_print(Ind, "\375\377G*   For that, you just received a reward, and if you die you will *");
			msg_print(Ind, "\375\377G*   also receive a deed on the next character you log in with.    *");
			give_reward(Ind, RESF_MID, "Gladiator's reward", 1, 0);
		}
		if (p_ptr->lev == MAX_PVP_LEVEL) {
			msg_broadcast_format(Ind, "\374\377G* %s has reached the highest level available to PvP characters! *", p_ptr->name);
			msg_print(Ind, "\375\377G* You have reached the highest level available to PvP characters! *");
			msg_print(Ind, "\375\377G*   For that, you just received a reward, and if you die you will *");
			msg_print(Ind, "\375\377G*   also receive a deed on the next character you log in with.    *");
			//buffer_account_for_achievement_deed(p_ptr, ACHV_PVP_MAX);
			give_reward(Ind, RESF_HIGH, "Gladiator's reward", 1, 0);
		}
	}

	/* Make a new copy of the skills - mikaelh */
	memcpy(p_ptr->s_info_old, p_ptr->s_info, MAX_SKILLS * sizeof(skill_player));
	p_ptr->skill_points_old = p_ptr->skill_points;

	/* Reskilling is now possible */
	p_ptr->reskill_possible = TRUE;

	/* Re-check house permissions, to display doors in correct colour (level-based door access!) */
	if (!p_ptr->wpos.wz) p_ptr->redraw |= PR_MAP;

	/* Handle stuff */
	handle_stuff(Ind);

	/* Update the skill points info on the client */
	Send_skill_info(Ind, 0, TRUE);
}


/*
 * Gain experience
 */
void gain_exp(int Ind, s64b amount) {
	player_type *p_ptr = Players[Ind];//, *p_ptr2=NULL;
	//int Ind2 = 0;

	if (is_admin(p_ptr) && p_ptr->lev >= 99) return;

	/* enforce dedicated Ironman Deep Dive Challenge character slot usage */
	if (amount && (p_ptr->mode & MODE_DED_IDDC) && !in_irondeepdive(&p_ptr->wpos)) {
#if 0 /* poof when gaining exp prematurely */
		msg_print(Ind, "\377RYou failed to enter the Ironman Deep Dive Challenge!");
		strcpy(p_ptr->died_from, "indetermination");
		p_ptr->deathblow = 0;
		p_ptr->death = TRUE;
		return;
#else /* just don't get exp */
		return;
#endif
	}

	if (p_ptr->IDDC_logscum) {
		//(spammy) msg_print(Ind, "\377oThis floor has become stale, take a staircase to move on!");
		return;
	}

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
 * (caused by GF_NETHER, GF_CHAOS, traps, exp-melee-hits, TY-curse)
 */
void lose_exp(int Ind, s32b amount) {
	player_type *p_ptr = Players[Ind];

	/* Amulet of Immortality */
	if (p_ptr->admin_invuln) return;

	if (safe_area(Ind)) return;

	/* Amulet of Immortality */
	if (p_ptr->admin_invuln) return;

	/* Paranoia */
	if (p_ptr->death) return;

	/* Hack -- player is secured inside a store/house except in dungeons */
	if (p_ptr->store_num != -1 && !p_ptr->wpos.wz && !bypass_invuln) return;

#if 0
	if (disturb) {
		break_cloaking(Ind, 0);
		stop_precision(Ind);
	}
#endif

	if (p_ptr->keep_life) {
		//msg_print(Ind, "You are impervious to life force drain!");
		return;
	}

#if 0 /* todo: get this right and all */
	/* Mega-Hack -- Apply "invulnerability" */
	if (p_ptr->invuln && (!bypass_invuln) && !p_ptr->invuln_applied) {
		/* Hack: Just reduce exp loss flat */
		amount = (amount + 1) / 2;
	}
#endif

	/* Never drop below zero experience */
	if (amount > p_ptr->exp) amount = p_ptr->exp - 1;
	if (!amount) return;

#if 1
	if (((p_ptr->alert_afk_dam && p_ptr->afk)
 #ifdef ALERT_OFFPANEL_DAM
	    || (p_ptr->alert_offpanel_dam && (p_ptr->panel_row_old != p_ptr->panel_row || p_ptr->panel_col_old != p_ptr->panel_col))
 #endif
	    )
 #ifdef USE_SOUND_2010
	    ) {
		Send_warning_beep(Ind);
		//sound(Ind, "warning", "page", SFX_TYPE_MISC, FALSE);
 #else
	    && p_ptr->paging == 0) {
		p_ptr->paging = 1;
 #endif
	}
#endif

#if 1
	/* warn if taking (continuous) damage while inside a store! */
	if (p_ptr->store_num != -1) {
 #ifdef USE_SOUND_2010
		Send_warning_beep(Ind);
		//sound(Ind, "warning", "page", SFX_TYPE_MISC, FALSE);
 #else
		if (p_ptr->paging == 0) p_ptr->paging = 1;
 #endif
		//all places using lose_exp() already give a message..
		//msg_print(Ind, "\377RWarning - your experience is getting drained!");
	}
#endif

	/* Lose some experience */
	p_ptr->exp -= amount;

	/* Check Experience */
	check_experience(Ind);
}


/* helper function to boost a character to a specific level (for Dungeon Keeper event) */
void gain_exp_to_level(int Ind, int level) {
	u32b k = 0;
	if (level <= 1) return;
	k = player_exp[level - 2];
	if (Players[Ind]->max_exp < k)
		/* make up for rounding error (+99) */
		gain_exp(Ind, ((k - Players[Ind]->max_exp) * Players[Ind]->expfact + 99) / 100);
}



/*
 * Hack -- Return the "automatic coin type" of a monster race
 * Used to allocate proper treasure when "Creeping coins" die
 *
 * XXX XXX XXX Note the use of actual "monster names"
 */
static int get_coin_type(monster_race *r_ptr) {
	cptr name = (r_name + r_ptr->name);

	/* Analyze "coin" monsters */
	if (r_ptr->d_char == '$') {
		/* Look for textual clues */
		if (strstr(name, " copper ")) return (1);
		if (strstr(name, " silver ")) return (2);
		if (strstr(name, " gold ")) return (10);
		if (strstr(name, " mithril ")) return (16);
		if (strstr(name, " adamantite ")) return (18);

		/* Look for textual clues */
		if (strstr(name, "Copper ")) return (1);
		if (strstr(name, "Silver ")) return (2);
		if (strstr(name, "Gold ")) return (10);
		if (strstr(name, "Mithril ")) return (16);
		if (strstr(name, "Adamantite ")) return (18);
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

/* Display Zu-Aon kills in special colours:
 *  UxU is too flashy, lcl is possible, xcx maybe best (Nether Realm floor look preserved in msg ;) */
#define ZU_AON_FLASHY_MSG
/* Asides from disallowing exp/kill credit/quest credit from henc-cheezed monsters, also disallow loot?
   Default is 'disabled' because a player can just as well visit a floor after the killing has been done, to loot it.
   Note: As a side effect, even if loot is allowed, DROP_CHOSEN is disabled (no fire stones from Dragonriders etc). */
//#define NO_HENC_CHEEZED_LOOT

bool monster_death(int Ind, int m_idx) {
	player_type *p_ptr = Players[Ind];
	player_type *q_ptr = Players[Ind];

	int	i, j, y, x, ny, nx;
	int	tmp_luck = p_ptr->luck;

	//int	dump_item = 0;
	//int	dump_gold = 0;

	int	number = 0;
	int	total = 0;

	char buf[160], m_name[MAX_CHARS], o_name[ONAME_LEN];
	cptr titlebuf;

	dungeon_type *d_ptr = getdungeon(&p_ptr->wpos);
	cave_type *c_ptr;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);
	bool is_Morgoth = (m_ptr->r_idx == RI_MORGOTH);
	bool is_Sauron = (m_ptr->r_idx == RI_SAURON);
	bool is_Pumpkin = (m_ptr->r_idx == RI_PUMPKIN1 || m_ptr->r_idx == RI_PUMPKIN2 || m_ptr->r_idx == RI_PUMPKIN3);
	int credit_idx = r_ptr->dup_idx ? r_ptr->dup_idx : m_ptr->r_idx;
	int r_idx = m_ptr->r_idx;
	//bool visible = (p_ptr->mon_vis[m_idx] || (r_ptr->flags1 & RF1_UNIQUE));

	bool good = (r_ptr->flags1 & RF1_DROP_GOOD) ? TRUE : FALSE;
	bool great = (r_ptr->flags1 & RF1_DROP_GREAT) ? TRUE : FALSE;

	bool do_gold = (!(r_ptr->flags1 & RF1_ONLY_ITEM));
	bool do_item = (!(r_ptr->flags1 & RF1_ONLY_GOLD));

#if FORCED_DROPS != 0
	bool drop_dual = FALSE;
	bool forced_drops_probable = FORCE_DROPS_PROBABLE / ((r_ptr->flags1 & (RF1_FRIEND | RF1_FRIENDS)) == 0x0 ? 1 : 3);
#endif

	int force_coin = get_coin_type(r_ptr);
	s16b local_quark = 0;
	object_type forge;
	object_type *qq_ptr;
	struct worldpos *wpos;
	cave_type **zcave;
	int dlev, rlev, tol_lev;

	int a_idx, chance, I_kind;
	artifact_type *a_ptr;

	bool henc_cheezed = FALSE, pvp = ((p_ptr->mode & MODE_PVP) != 0);
	u32b resf_drops = make_resf(p_ptr), resf_chosen = resf_drops;


	/* experimental: Zu-Aon drops only randarts */
	if (m_ptr->r_idx == RI_ZU_AON || m_ptr->r_idx == RI_BAD_LUCK_BAT)
		resf_drops |= (RESF_FORCERANDART | RESF_NOTRUEART);
	/* Morgoth never drops true artifacts */
	if (is_Morgoth) resf_drops |= RESF_NOTRUEART;

	/* terminate mindcrafter charm effect */
	if (m_ptr->charmedignore) {
		Players[m_ptr->charmedignore]->mcharming--;
		m_ptr->charmedignore = 0;
	}

	if (m_ptr->special) s_printf("MONSTER_DEATH: Golem of '%s' by '%s'.\n", lookup_player_name(m_ptr->owner), p_ptr->name);
#ifdef RPG_SERVER
	else if (m_ptr->pet) s_printf("MONSTER_DEATH: Pet of '%s' by '%s'\n", lookup_player_name(m_ptr->owner), p_ptr->name);
#endif

#ifdef RPG_SERVER
	/* Pet death. Update and inform the owner -the_sandman */
	if (m_ptr->pet) {
		for (i = NumPlayers; i > 0; i--) {
			if (m_ptr->owner == Players[i]->id) {
				msg_format(i, "\374\377R%s has killed your pet!", Players[Ind]->name);
				switch (Players[i]->name[strlen(Players[i]->name) - 1]) {
				case 's': case 'x': case 'z':
					msg_format(Ind, "\374\377RYou have killed %s' pet!", Players[i]->name);
					break;
				default:
					msg_format(Ind, "\374\377RYou have killed %s's pet!", Players[i]->name);
				}
				Players[i]->has_pet = 0;
				FREE(m_ptr->r_ptr, monster_race); //no drop, no exp.
				return FALSE;
			}
		}
	}
#endif

	if (cfg.henc_strictness && !p_ptr->total_winner &&
	    /* p_ptr->lev more logical but harsh: */
#if 1 /* player should always seek not too high-level party members compared to player's current real level? */
	    m_ptr->henc - p_ptr->max_lev > MAX_PARTY_LEVEL_DIFF + 1)
#else /* players may seek higher-level party members to team up with if he's died before? Weird combination so not recommended! */
	    m_ptr->henc - p_ptr->max_plv > MAX_PARTY_LEVEL_DIFF + 1)
#endif
		henc_cheezed = TRUE;

	/* Get the location */
	y = m_ptr->fy;
	x = m_ptr->fx;
	wpos = &m_ptr->wpos;
	if (!(zcave = getcave(wpos))) return FALSE;

	if (ge_special_sector && /* training tower event running? and we are there? */
	    in_arena(wpos)) {
		monster_desc(0, m_name, m_idx, 0x00);
		msg_broadcast_format(0, "\376\377S** %s has defeated %s! **", p_ptr->name, m_name);
		s_printf("EVENT_RESULT: %s (%d) has defeated %s.\n", p_ptr->name, p_ptr->lev, m_name);
	}

	/* get monster name for damage deal description */
	monster_desc(Ind, m_name, m_idx, 0x00);

	process_hooks(HOOK_MONSTER_DEATH, "d", Ind);

	if (season_halloween) {
		/* let everyone know, so they are prepared.. >:) */
		if ((m_ptr->r_idx == RI_PUMPKIN1 || m_ptr->r_idx == RI_PUMPKIN2 || m_ptr->r_idx == RI_PUMPKIN3)
		    && !m_ptr->clone) {
			msg_broadcast_format(0, "\374\377L**\377o%s has defeated a tasty halloween spirit!\377L**", p_ptr->name);
			s_printf("HALLOWEEN: %s (%d/%d) has defeated %s.\n", p_ptr->name, p_ptr->max_plv, p_ptr->max_lev, m_name);
			great_pumpkin_duration = 0;
			great_pumpkin_timer = 15 + rand_int(45);
			//great_pumpkin_killer = p_ptr->id;
			strcpy(great_pumpkin_killer, p_ptr->accountname);
		}
	} else if (season_xmas) {
		if ((m_ptr->r_idx == RI_SANTA1 || m_ptr->r_idx == RI_SANTA2)
		    && !m_ptr->clone) {
			msg_broadcast_format(0, "\374\377L**\377oSanta dropped the presents near %s!\377L**", p_ptr->name);
			s_printf("XMAS: %s (%d) has defeated %s.\n", p_ptr->name, p_ptr->max_plv, m_name);
			santa_claus_timer = 60 + rand_int(120);
		}
		/* Actually restore town music (or whichever) if Santa had his own music event */
		for (i = 1; i <= NumPlayers; i++) {
			if (Players[i]->music_monster == 67) {
				Players[i]->music_monster = -1;
				handle_music(i);
			}
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
				scatter(wpos, &wy, &wx, p_ptr->py, p_ptr->px, 20, 0);
			} while (!(in_bounds(wy, wx) && cave_floor_bold(zcave, wy, wx)) && --attempts);

			if (attempts > 0) {
#if 0
				if (is_friend(m_ptr) > 0) {
					if (summon_specific_friendly(wy, wx, 100, SUMMON_DAWN, FALSE)) {
						if (player_can_see_bold(wy, wx))
							msg_print ("A new warrior steps forth!");
 #ifdef USE_SOUND_2010
						//sound_near_site(wy, wx, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
 #endif
					}
				}
				else
#endif
				{
					if (summon_specific(wpos, wy, wx, 100, m_ptr->clone + 20, SUMMON_DAWN, 1, 0)) {
						if (player_can_see_bold(Ind, wy, wx))
							msg_print (Ind, "A new warrior steps forth!");
#ifdef USE_SOUND_2010
						//sound_near_site(wy, wx, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
#endif
					}
				}
			}
		}
	}

	/* One more ultra-hack: An Unmaker goes out with a big bang! */
	else if (strstr((r_name + r_ptr->name),"Unmaker")) {
		int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO;
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
				summon_override_checks = SO_IDDC;
				if (summon_specific(wpos, wy, wx, 100, 0, SUMMON_BLUE_HORROR, 1, 0)) { /* that's _not_ 2, lol */
					if (player_can_see_bold(Ind, wy, wx))
						msg_print (Ind, "A blue horror appears!");
#ifdef USE_SOUND_2010
					sound_near_site(wy, wx, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE);
#endif
				}
				summon_override_checks = SO_NONE;
			}
		}
	}

	/* Let monsters explode! */
	for (i = 0; i < 4; i++) {
		if (m_ptr->blow[i].method == RBM_EXPLODE) {
			int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO;
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
			case RBE_LITE:      typ = GF_LITE; break;
			//case RBE_HALLU:     typ = GF_CONFUSION; break;
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

#ifdef USE_SOUND_2010
			sound_near_monster(m_idx, "monster_explode", NULL, SFX_TYPE_MON_MISC);
#endif

			snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s explodes for", m_name);
			p_ptr->attacker[0] = toupper(p_ptr->attacker[0]);
			project(m_idx, 3, wpos, y, x, damage > base_damage ? base_damage : damage, typ, flg, p_ptr->attacker);
			break;
		}
	}

	/* Log-scumming in IDDC is like fighting clones */
	if (p_ptr->IDDC_logscum) return FALSE;
	/* enforce dedicated Ironman Deep Dive Challenge character slot usage */
	if ((p_ptr->mode & MODE_DED_IDDC) && !in_irondeepdive(&p_ptr->wpos)
	    && r_ptr->mexp) /* Allow kills in Bree */
		return FALSE;
	/* clones don't drop treasure or complete quests.. */
	if (m_ptr->clone) {
		/* Specialty - even for non-creditable Sauron:
		   If Sauron is killed in Mt Doom, allow the player to recall! */
		if (is_Sauron) {
			if (d_ptr->type == DI_MT_DOOM) {
				dun_level *l_ptr = getfloor(&p_ptr->wpos);
				l_ptr->flags1 |= LF1_IRON_RECALL;
				floor_msg_format(&p_ptr->wpos, "\374\377gYou don't sense a magic barrier here!");
			}
		}

		/* no credit/loot for clones */
		return FALSE;
	}
	/* ..neither do cheezed kills */
	if (henc_cheezed &&
	    !is_Morgoth && /* make exception for Morgoth, so hi-lvl fallen kings can re-king */
	    !is_Pumpkin /* allow a mixed hunting group */
#ifndef NO_HENC_CHEEZED_LOOT
	    /* ..however, never have henc'ed unique monsters drop loot, since kill credit won't be given for them, so could repeatedly loot them */
	    && (m_ptr->questor || (r_ptr->flags1 & RF1_UNIQUE))
#endif
	    )
		return FALSE;

	dlev = getlevel(wpos);
	rlev = r_ptr->level;

	/* Determine how much we can drop */
	if ((r_ptr->flags1 & RF1_DROP_60) && (rand_int(100) < 60)) number++;
	if ((r_ptr->flags1 & RF1_DROP_90) && (rand_int(100) < 90)) number++;
	if (r_ptr->flags0 & RF0_DROP_1) number++;
	if (r_ptr->flags1 & RF1_DROP_1D2) number += damroll(1, 2);
	if (r_ptr->flags0 & RF0_DROP_2) number += 2;
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

	/* Questors: Usually drop no items, except if specified */
	if (m_ptr->questor) {
		if (q_info[m_ptr->quest].defined && q_info[m_ptr->quest].questors > m_ptr->questor_idx) {
			if (!(q_info[m_ptr->quest].questor[m_ptr->questor_idx].drops & 0x1)) number = 0;
		} else {
			s_printf("QUESTOR_DEPRECATED (monster_dead)\n");
			number = 0;
		}
	}

#if FORCED_DROPS != 0
	/* Hack drops: Monster should drop specific item class? */
	/* re_info takes priority */
	switch (m_ptr->ego) {
	case 35: case 39: //unbeliever
		if (rand_int(3)) resf_drops |= RESF_COND_DARKSWORD;
		resf_drops |= RESF_COND2_HARMOUR;
		break;
	case 11: case 28: case 33: //rogues
		resf_drops |= RESF_COND_LSWORD;
		if (rand_int(2)) {
			drop_dual = TRUE;
			number++; // :o
		}
		resf_drops |= RESF_COND2_LARMOUR;
		break;
	case 8: case 26: //priests
		resf_drops |= RESF_COND_BLUNT;
		break;
	case 7: //shamans
		resf_drops |= RESF_CONDF_NOSWORD;
		resf_drops |= RESF_COND2_LARMOUR;
		break;
	case 9: case 30: //mages (mage/sorceror)
		if (rand_int(3)) resf_drops |= RESF_CONDF_MSTAFF;
		resf_drops |= RESF_COND2_LARMOUR;
		break;
	case 10: case 13: case 23: //archers
		resf_drops |= RESF_COND_RANGED;
		break;
	case 62: //runemasters
		if (rand_int(4)) {
			resf_drops |= RESF_CONDF_RUNE;
			number++; //mh, just don't look too forced in case we only drop one item :p
		}
		resf_drops |= RESF_COND2_LARMOUR;
		break;
	/* -- only do flexible vs tough armour choice for the remaining ego monsters here -- */
	case 5: case 27: //captain, warrior
		resf_drops |= RESF_COND2_HARMOUR;
		break;
	case 40: case 47: case 63: //monk, druid, ranger
		resf_drops |= RESF_COND2_LARMOUR;
		break;
	/* switch to r_info next: */
	default:
		switch (m_ptr->r_idx) {
		case 1048: //unbeliever with FRIENDS
			if (!rand_int(3)) resf_drops |= RESF_COND_DARKSWORD;
			resf_drops |= RESF_COND2_HARMOUR;
			break;
		case 1047: case 1049: case 1050: //unbeliever
			if (rand_int(3)) resf_drops |= RESF_COND_DARKSWORD;
			resf_drops |= RESF_COND2_HARMOUR;
			break;
		case 505: //Groo, the Wanderer!
		case 116:  //rogues with FRIENDS
		case 44: case 150: case 199: case 376: case 516: case 696: //rogues
			resf_drops |= RESF_COND_LSWORD;
			if (rand_int(2)) {
				drop_dual = TRUE;
				number++; // :o  (hope this isn't overkill with novice rogue packs)
			}
			resf_drops |= RESF_COND2_LARMOUR;
			break;
		case 564: ///nightblade has FRIENDS
		case 485: //ninja
			resf_drops |= RESF_COND_LSWORD;
			resf_drops |= RESF_COND2_LARMOUR;
			break;
		case 216: case 1058: case 1059: case 1075: //swordmen
			resf_drops |= RESF_COND_SWORD;
			resf_drops |= RESF_COND2_HARMOUR;
			break;
		case 109: //priests with FRIENDS
		case 45: case 225: //priests
		case 1017: case 689: case 1018: case 226: //evil priests
			resf_drops |= RESF_COND_BLUNT;
			break;
		case 888: case 906: case 217: //shamans
			resf_drops |= RESF_CONDF_NOSWORD;
			resf_drops |= RESF_COND2_LARMOUR;
			break;
		case 46: case 93: //novice mages without/with FRIENDS, both suck ;)
			if (!rand_int(5)) resf_drops |= RESF_CONDF_MSTAFF;
			resf_drops |= RESF_COND2_LARMOUR;
			break;
		case 240: case 449: case 638: case 738: case 178: case 657: //mages (mage/illusionist/sorcerer)
			if (rand_int(3)) resf_drops |= RESF_CONDF_MSTAFF;
			resf_drops |= RESF_COND2_LARMOUR;
			break;
		case 281: //gnome mages have FRIENDS
			if (!rand_int(5)) resf_drops |= RESF_CONDF_MSTAFF;
			resf_drops |= RESF_COND2_LARMOUR;
			break;
		case 375: //warlocks (dark-elven) have FRIENDS
			if (!rand_int(10)) resf_drops |= RESF_CONDF_MSTAFF;
			resf_drops |= RESF_COND2_LARMOUR;
			break;
		case 539: //slinger have FRIENDS
			resf_drops |= RESF_COND_SLING;
			break;
		/* -- only do flexible vs tough armour choice for the remaining monsters here -- */
		default:
			switch (m_ptr->r_idx) {
			/* monsters that don't fall into the usual colouring scheme */
			case 370: case 492: //monks
			case 532: //dagashi too
				resf_drops |= RESF_COND2_LARMOUR;
				break;
			case 693: case 182: case 887: case 905: //warriors
				resf_drops |= RESF_COND2_HARMOUR;
				break;
			case 239: //berserker
				//currently exempt, could use RESF_COND2_LARMOUR though?
				break;
			default:
				/* monsters that fall into the usual colouring scheme
				   and weren't caught above already */
				switch (r_ptr->d_attr) {
				case TERM_BLUE:
					if (r_ptr->d_char != 'p') break; //rogues
					resf_drops |= RESF_COND2_LARMOUR;
					break;
				case TERM_UMBER: case TERM_WHITE: //warriors & paladins
					if (r_ptr->d_char != 'p') break;//note: dark-elven warrior left out. maybe he prefers lighter armour :)
					resf_drops |= RESF_COND2_HARMOUR;
					break;
				case TERM_ORANGE:
					if (r_ptr->d_char != 'p') break; //mystics
					resf_drops |= RESF_COND2_LARMOUR;
					break;
				}
			}
		}
	}
#endif

	/* Drop some objects */
	for (j = 0; j < number; j++) {
		/* Try 20 times per item, increasing range */
		//for (i = 0; i < 20; ++i)
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
			object_level = (dlev + rlev) / 2;
 #ifdef RANDOMIZED_LOOT_LEVEL
			if (object_level < rlev) tol_lev = rlev - object_level;
			else tol_lev = dlev - object_level;
			if (tol_lev > 11) tol_lev = 13; /* need +12 levels of tolerance to allow depth-115 items to drop from level 80 monsters */
			object_level += rand_int(tol_lev);
 #endif
#else
			/* Monster level is more important than floor level */
			object_level = (dlev + rlev * 2) / 3;
 #ifdef RANDOMIZED_LOOT_LEVEL
			if (object_level < rlev) tol_lev = rlev - object_level;
			else tol_lev = dlev - object_level;
			if (tol_lev > 21) tol_lev = 21; /* need +20 levels of tolerance to allow depth-115 items to drop from level 80 monsters */
			object_level += rand_int(tol_lev);
 #endif
#endif

			/* No easy item hunting in towns.. */
			if (wpos->wz == 0) object_level = rlev / 2;

			/* Place Gold */
			if (do_gold && (!do_item || (rand_int(100) < 50))) {
				place_gold(wpos, y, x, 0);
				//if (player_can_see_bold(Ind, ny, nx)) dump_gold++;
			}

			/* Place Object */
			else {
				place_object_restrictor = RESF_NONE;

				/* hack to allow custom test l00t drop for admins: */
				if (is_admin(p_ptr)) {
					/* the hack works by using weapon's inscription! */
					char *k_tval, *k_sval;
					if (p_ptr->inventory[INVEN_WIELD].tval &&
					    p_ptr->inventory[INVEN_WIELD].note &&
					    (k_tval = strchr(quark_str(p_ptr->inventory[INVEN_WIELD].note), '%')) &&
					    (k_sval = strchr(k_tval, ':'))
					    ) {
						resf_drops |= RESF_DEBUG_ITEM;
						/* extract tval:sval */
						/* abuse luck parameter for this */
						tmp_luck = lookup_kind(atoi(k_tval + 1), atoi(k_sval + 1));
						/* catch invalid items */
						if (!tmp_luck) resf_drops &= ~RESF_DEBUG_ITEM;
					}
				}

#if FORCED_DROPS != 0
 #if 0
				/* only try forcing the item for the first item dropped */
				if (!j && magik(forced_drops_probable)) resf_drops |= RESF_COND_FORCE;
 #else
				/* retry forcing the item until it gets generated or all items have been dropped */
				if (magik(forced_drops_probable)) resf_drops |= RESF_COND_FORCE;
 #endif
#endif
				/* generate an object and place it */
				place_object(wpos, y, x, good, great, FALSE, resf_drops, r_ptr->drops, tmp_luck, ITEM_REMOVAL_NORMAL);
#if FORCED_DROPS != 0
				/* lift force, except if we're planning a dual-drop */
				if (!j) {
					if (!drop_dual) resf_drops &= ~RESF_COND_FORCE;
				} else resf_drops &= ~RESF_COND_FORCE;
				/* successfully created a lore-fitting drop that clears our conditions? */
				if (place_object_restrictor & RESF_COND_MASK) {
					if (drop_dual) drop_dual = FALSE; //actually get another one? (for monsters dual-wielding the same item class, aka rogues -> swords)
					else resf_drops &= ~RESF_COND_MASK; //lift the conditions
				}
#endif

				//if (player_can_see_bold(Ind, ny, nx)) dump_item++;
			}

			/* Reset the object level */
			object_level = dlev;

			/* Reset "coin" type */
			coin_type = 0;
		}
	}

	/* Forget it */
	unique_quark = 0;

	/* Take note of any dropped treasure */
#if 0
	/* XXX this doesn't work for now.. (not used anyway) */
	if (visible && (dump_item || dump_gold)) {
		/* Take notes on treasure */
		lore_treasure(m_idx, dump_item, dump_gold);
	}
#endif

#ifdef NO_HENC_CHEEZED_LOOT /* we allowed loot (but we still disallow exp/credit) */
	if (henc_cheezed &&
	    !is_Morgoth && /* make exception for Morgoth, so hi-lvl fallen kings can re-king */
	    !is_Pumpkin) /* allow a mixed hunting group */
		return FALSE;
#endif

	/* Check whether a quest requested this monster dead */
	if (p_ptr->quest_any_k_within_target) quest_check_goal_k(Ind, m_ptr);

	/* Maia traitage */
	if (p_ptr->prace == RACE_MAIA && !p_ptr->ptrait) {
		switch (m_ptr->r_idx) {
		case RI_CANDLEBEARER:
			if (p_ptr->r_killed[RI_DARKLING] != 0)
				msg_print(Ind, "\377rYour presence in the realm is forfeit.");
			else if (p_ptr->r_killed[RI_CANDLEBEARER] == 0)
				msg_print(Ind, "\377yYou have stepped on the path to corruption..");
			break;
		case RI_DARKLING:
			if (p_ptr->r_killed[RI_CANDLEBEARER] != 0)
				msg_print(Ind, "\377rYour presence in the realm is forfeit.");
			else if (p_ptr->r_killed[RI_DARKLING] == 0)
				msg_print(Ind, "\377yYou have stepped on the path to enlightenment..");
			break;
		}
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
			if (is_admin(Players[i])) continue;
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
	} else {

		/* Normal kill count */
		if (p_ptr->r_killed[r_idx] < 10000) p_ptr->r_killed[r_idx]++;

		/* Mimicry kill count */
		i = get_skill_scale(p_ptr, SKILL_MIMIC, 100);
		if (credit_idx
#if 1 /* New: Can only gain form knowledge while Mimicry skill is actually sufficient? */
		    && i >= r_info[credit_idx].level
#endif
		    && p_ptr->r_mimicry[credit_idx] < 10000 && !m_ptr->questor) {
			int before = p_ptr->r_mimicry[credit_idx];

			/* get +1 bonus credit in Ironman Deep Dive Challenge */
			if (in_irondeepdive(wpos))
#ifndef IDDC_MIMICRY_BOOST
				p_ptr->r_mimicry[credit_idx]++;
#else /* give a possibly greater boost than just +1 */
				p_ptr->r_mimicry[credit_idx] += IDDC_MIMICRY_BOOST;
#endif

#ifdef RPG_SERVER
			/* There is a 1 in (m_ptr->level - kill count)^2 chance of learning form straight away
			 * to make it easier (at least statistically) getting forms in the iron server. Plus,
			 * mimicked speed and hp are lowered already anyway.	- the_sandman */
			if ( ( r_info[m_ptr->r_idx].level - p_ptr->r_mimicry[credit_idx] > 0 ) &&
			     ( (randint((r_info[m_ptr->r_idx].level - p_ptr->r_mimicry[credit_idx]) *
			    (r_info[m_ptr->r_idx].level - p_ptr->r_mimicry[credit_idx])) == 1))) {
			    p_ptr->r_mimicry[credit_idx] = r_info[credit_idx].level;
			} else { /* Badluck */
				p_ptr->r_mimicry[credit_idx]++;

				/* Shamans have a chance to learn E forms very quickly */
				if (p_ptr->pclass == CLASS_SHAMAN && (mimic_shaman_E(credit_idx) || r_info[credit_idx].d_char == 'X'))
					p_ptr->r_mimicry[credit_idx] += 2;
			}
#else
			if (pvp) {
				/* PvP mode chars learn forms very quickly! */
				p_ptr->r_mimicry[credit_idx] += 3;
			} else {
				p_ptr->r_mimicry[credit_idx]++;

				/* Shamans have a chance to learn E forms very quickly */
				if (p_ptr->pclass == CLASS_SHAMAN && (mimic_shaman_E(credit_idx) || r_info[credit_idx].d_char == 'X'))
					p_ptr->r_mimicry[credit_idx] += 2;
			}
#endif

			if (p_ptr->r_mimicry[credit_idx] > 10000)
				p_ptr->r_mimicry[credit_idx] = 10000;

			if (i && i >= r_info[credit_idx].level &&
			    (before == 0 || /* <- for level 0 townspeople */
			     before < r_info[credit_idx].level) &&
			    (p_ptr->r_mimicry[credit_idx] >= r_info[credit_idx].level ||
			    /* for level 0 townspeople: */
			    r_info[credit_idx].level == 0)) {
				if (!((r_ptr->flags1 & RF1_UNIQUE) || (p_ptr->pclass == CLASS_DRUID) ||
				    ((p_ptr->pclass == CLASS_SHAMAN) && !mimic_shaman(credit_idx)) ||
				    (p_ptr->prace == RACE_VAMPIRE))) {
					msg_format(Ind, "\374\377UYou have learned the form of %s! (%d)",
					    r_info[credit_idx].name + r_name, credit_idx);
					/* smooth transition from poly ring form to known form */
					if (p_ptr->body_monster == credit_idx) p_ptr->tim_mimic = p_ptr->tim_mimic_what = 0;
					if (!p_ptr->warning_mimic) {
						p_ptr->warning_mimic = 1;
						msg_print(Ind, "\374\377U(Press '\377ym\377U' key and choose '\377yuse innate power\377U' to polymorph.)");
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
if (cfg.unikill_format) {
	/* let's try with titles before the name :) -C. Blue */
		titlebuf = get_ptitle(q_ptr, FALSE);

		if (is_Morgoth)
			snprintf(buf, sizeof(buf), "\374\377v**\377L%s was slain by %s %s.\377v**", r_name_get(m_ptr), titlebuf, p_ptr->name);
#ifdef ZU_AON_FLASHY_MSG
		else if (m_ptr->r_idx == RI_ZU_AON)
			snprintf(buf, sizeof(buf), "\374\377x**\377c%s was slain by %s %s.\377x**", r_name_get(m_ptr), titlebuf, p_ptr->name);
#endif
		else if ((r_ptr->flags0 & RF0_FINAL_GUARDIAN)) {
#ifdef IDDC_BOSS_COL
			if (in_irondeepdive(wpos)) snprintf(buf, sizeof(buf), "\374\377D**\377c%s was slain by %s %s.\377D**", r_name_get(m_ptr), titlebuf, p_ptr->name);
			else
#endif
			snprintf(buf, sizeof(buf), "\374\377U**\377c%s was slain by %s %s.\377U**", r_name_get(m_ptr), titlebuf, p_ptr->name);
		} else
			snprintf(buf, sizeof(buf), "\374\377b**\377c%s was slain by %s %s.\377b**", r_name_get(m_ptr), titlebuf, p_ptr->name);
} else {
	/* for now disabled (works though) since we don't have telepath class
	   at the moment, and party names would make the line grow too long if
	   combined with title before the actual name :/ -C. Blue */
		if (!Ind2) {
			if (is_Morgoth)
				snprintf(buf, sizeof(buf), "\374\377v**\377L%s was slain by %s.\377v**", r_name_get(m_ptr), p_ptr->name);
#ifdef ZU_AON_FLASHY_MSG
			else if (m_ptr->r_idx == RI_ZU_AON)
				snprintf(buf, sizeof(buf), "\374\377x**\377c%s was slain by %s.\377x**", r_name_get(m_ptr), p_ptr->name);
#endif
			else if ((r_ptr->flags0 & RF0_FINAL_GUARDIAN)) {
#ifdef IDDC_BOSS_COL
				if (in_irondeepdive(wpos)) snprintf(buf, sizeof(buf), "\374\377D**\377c%s was slain by %s.\377D**", r_name_get(m_ptr), p_ptr->name);
				else
#endif
				snprintf(buf, sizeof(buf), "\374\377U**\377c%s was slain by %s.\377U**", r_name_get(m_ptr), p_ptr->name);
			} else
				snprintf(buf, sizeof(buf), "\374\377b**\377c%s was slain by %s.\377b**", r_name_get(m_ptr), p_ptr->name);
		} else {
			if (is_Morgoth)
				snprintf(buf, sizeof(buf), "\374\377v**\377L%s was slain by fusion %s-%s.\377v**", r_name_get(m_ptr), p_ptr->name, p_ptr2->name);
#ifdef ZU_AON_FLASHY_MSG
			else if (m_ptr->r_idx == RI_ZU_AON)
				snprintf(buf, sizeof(buf), "\374\377x**\377c%s was slain by fusion %s-%s.\377x**", r_name_get(m_ptr), p_ptr->name, p_ptr2->name);
#endif
			else if ((r_ptr->flags0 & RF0_FINAL_GUARDIAN)) {
#ifdef IDDC_BOSS_COL
				if (in_irondeepdive(wpos)) snprintf(buf, sizeof(buf), "\374\377D**\377c%s was slain by fusion %s-%s.\377D**", r_name_get(m_ptr), p_ptr->name, p_ptr2->name);
				else
#endif
				snprintf(buf, sizeof(buf), "\374\377U**\377c%s was slain by fusion %s-%s.\377U**", r_name_get(m_ptr), p_ptr->name, p_ptr2->name);
			} else
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
#ifdef ZU_AON_FLASHY_MSG
					else if (m_ptr->r_idx == RI_ZU_AON)
						snprintf(buf, sizeof(buf), "\374\377x**\377c%s was slain by %s of %s.\377x**", r_name_get(m_ptr), p_ptr->name, parties[p_ptr->party].name);
#endif
					else if ((r_ptr->flags0 & RF0_FINAL_GUARDIAN)) {
#ifdef IDDC_BOSS_COL
						if (in_irondeepdive(wpos)) snprintf(buf, sizeof(buf), "\374\377D**\377c%s was slain by %s of %s.\377D**", r_name_get(m_ptr), p_ptr->name, parties[p_ptr->party].name);
						else
#endif
						snprintf(buf, sizeof(buf), "\374\377U**\377c%s was slain by %s of %s.\377U**", r_name_get(m_ptr), p_ptr->name, parties[p_ptr->party].name);
					} else
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
	   Note: If strict_etiquette is false, the player can actualy keep his arts!
	   Originally for RPG_SERVER, but atm enabled for all. - C. Blue */
	if (is_Morgoth
#if 1 /* hm */
	    && cfg.strict_etiquette
#endif
	    ) {
		dun_level *l_ptr = getfloor(&p_ptr->wpos);
		if ((((d_ptr->flags2 & DF2_IRON || d_ptr->flags1 & DF1_FORCE_DOWN)
		    && d_ptr->maxdepth > ABS(p_ptr->wpos.wz)) ||
		    (d_ptr->flags1 & DF1_NO_RECALL))
		    && !(l_ptr->flags1 & LF1_IRON_RECALL)
		    && !(d_ptr->flags2 & DF2_NO_EXIT_WOR)) {
			/* Allow exceptional recalling.. */
			l_ptr->flags1 |= LF1_IRON_RECALL;
			/* ..and notify everyone on the level about it */
			floor_msg_format(&p_ptr->wpos, "\374\377gYou don't sense a magic barrier here!");
		}
	}
	else if (is_Sauron) {
		/* If player killed Sauron, also mark the Shadow (formerly Necromancer) of Dol Guldur as killed!
		   This is required since we now need a dungeon boss for Dol Guldur again =)
		   So always kill the Shadow first, if you want his loot. - C. Blue */
		p_ptr->r_killed[RI_DOL_GULDUR] = 1;

		/* If Sauron is killed in Mt Doom, allow the player to recall! */
		if (d_ptr->type == DI_MT_DOOM) {
			dun_level *l_ptr = getfloor(&p_ptr->wpos);

			l_ptr->flags1 |= LF1_IRON_RECALL;
			floor_msg_format(&p_ptr->wpos, "\374\377gYou don't sense a magic barrier here!");
		}

		/* for The One Ring.. */
		if (in_irondeepdive(wpos)) sauron_weakened_iddc = FALSE;
		else sauron_weakened = FALSE;
	}

	/* Dungeon bosses often drop a dungeon-set true artifact (for now 1 in 3 chance) */
	if ((r_ptr->flags0 & RF0_FINAL_GUARDIAN)) {
		msg_format(Ind, "\374\377UYou have conquered %s!", d_name +
#ifdef IRONDEEPDIVE_MIXED_TYPES
		    d_info[in_irondeepdive(wpos) ? iddc[ABS(wpos->wz)].type : d_ptr->type].name
#else
		    d_info[d_ptr->type].name
#endif
		    );

		if ((
#ifdef IRONDEEPDIVE_MIXED_TYPES
		    in_irondeepdive(wpos) ? (a_idx = d_info[iddc[ABS(wpos->wz)].type].final_artifact) :
#endif
		    (a_idx = d_info[d_ptr->type].final_artifact))
		    /* hack: 0 rarity = always generate -- for Ring of Phasing! */

		    && (
#ifdef IRONDEEPDIVE_MIXED_TYPES
		    in_irondeepdive(wpos) || //Let's reward those brave IDDC participants?
#endif
		    (!a_info[a_idx].rarity || !rand_int(3)))

		    && !cfg.arts_disabled &&
#ifdef RING_OF_PHASING_NO_TIMEOUT
		    (
#endif
		    a_info[a_idx].cur_num == 0
#ifdef RING_OF_PHASING_NO_TIMEOUT
		    || a_idx == ART_PHASING)
#endif
		    ) {
#ifdef RING_OF_PHASING_NO_TIMEOUT
			/* remove current ring of phasing, so it can be dropped anew */
			if (a_info[a_idx].cur_num) erase_artifact(a_idx);
#endif
			s_printf("preparing FINAL_ARTIFACT %d", a_idx);
			a_ptr = &a_info[a_idx];
			qq_ptr = &forge;
			object_wipe(qq_ptr);
			I_kind = lookup_kind(a_ptr->tval, a_ptr->sval);
			/* Create the artifact */
			invcopy(qq_ptr, I_kind);
			qq_ptr->name1 = a_idx;

			if (!(resf_chosen & RESF_NOTRUEART) ||
			    ((resf_chosen & RESF_WINNER) && winner_artifact_p(qq_ptr))) {
				/* Extract the fields */
				qq_ptr->pval = a_ptr->pval;
				qq_ptr->ac = a_ptr->ac;
				qq_ptr->dd = a_ptr->dd;
				qq_ptr->ds = a_ptr->ds;
				qq_ptr->to_a = a_ptr->to_a;
				qq_ptr->to_h = a_ptr->to_h;
				qq_ptr->to_d = a_ptr->to_d;
				qq_ptr->weight = a_ptr->weight;

				object_desc(Ind, o_name, qq_ptr, TRUE, 3);
				s_printf(" '%s'", o_name);

				handle_art_inum(a_idx);

				/* Hack -- acquire "cursed" flag */
				if (a_ptr->flags3 & (TR3_CURSED)) qq_ptr->ident |= (ID_CURSED);

				/* Complete generation, especially level requirements check */
				apply_magic(wpos, qq_ptr, -2, FALSE, TRUE, FALSE, FALSE, resf_chosen);

				if (local_quark) {
					qq_ptr->note = local_quark;
					qq_ptr->note_utag = strlen(quark_str(local_quark));
				}

				/* Little sanity hack for level requirements
				   of the Ring of Phasing - would be 92 otherwise */
				if (a_idx == ART_PHASING) {
					qq_ptr->level = (60 + rand_int(6));
					qq_ptr->marked2 = ITEM_REMOVAL_NEVER;
				}

				/* Drop the artifact from heaven */
#ifdef PRE_OWN_DROP_CHOSEN
				else { /* ring of phasing is never 0'ed */
					qq_ptr->level = 0;
					qq_ptr->owner = p_ptr->id;
					qq_ptr->mode = p_ptr->mode;
					qq_ptr->iron_trade = p_ptr->iron_trade;
					determine_artifact_timeout(a_idx, wpos);
				}
#endif
				drop_near(0, qq_ptr, -1, wpos, y, x);
				s_printf("..dropped.\n");
			} else  s_printf("..failed.\n");
		} else if (
#ifdef IRONDEEPDIVE_MIXED_TYPES
		    in_irondeepdive(wpos) ? (I_kind = d_info[iddc[ABS(wpos->wz)].type].final_object) :
#endif
		    (I_kind = d_info[d_ptr->type].final_object)) {
			s_printf("preparing FINAL_OBJECT %d", I_kind);
			qq_ptr = &forge;
			object_wipe(qq_ptr);
			/* Create the object */
			invcopy(qq_ptr, I_kind + 1); /* weirdness, why is it actually 1 too low? */

			/* Complete generation, especially level requirements check */
			apply_magic(wpos, qq_ptr, -2, FALSE, TRUE, FALSE, FALSE, resf_chosen);

			object_desc(Ind, o_name, qq_ptr, TRUE, 3);
			s_printf(" '%s'", o_name);

			if (local_quark) {
				qq_ptr->note = local_quark;
				qq_ptr->note_utag = strlen(quark_str(local_quark));
			}

			/* Drop the object from heaven */
#ifdef PRE_OWN_DROP_CHOSEN
			qq_ptr->level = 0;
			qq_ptr->owner = p_ptr->id;
			qq_ptr->mode = p_ptr->mode;
			qq_ptr->iron_trade = p_ptr->iron_trade;
			if (true_artifact_p(qq_ptr)) determine_artifact_timeout(qq_ptr->name1, wpos);
#endif
			drop_near(0, qq_ptr, -1, wpos, y, x);
			s_printf("..dropped.\n");
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
					s_printf("%s *** total_winner : %s (lev %d (%d,%d))\n", showtime(), q_ptr->name, q_ptr->lev, q_ptr->max_lev, q_ptr->max_plv);

					s_printf("CHARACTER_WINNER: race=%s ; class=%s\n", race_info[q_ptr->prace].title, class_info[q_ptr->pclass].title);

					/* Lose ironman champion status if this total_winner title was actually
					   acquired after having already beaten the ironman deep dive challenge */
					q_ptr->iron_winner = FALSE;

					/* Redraw the "title" */
					q_ptr->redraw |= (PR_TITLE);
					clockin(i, 9);

#ifdef USE_SOUND_2010
					/* Play 'winner' music */
					handle_music(i);
#endif

					/* Congratulations */
					msg_print(i, "\377G*** CONGRATULATIONS ***");
					if (q_ptr->mode & (MODE_HARD | MODE_NO_GHOST)) {
						msg_format(i, "\374\377GYou have won the game and are henceforth titled '%s'!", (q_ptr->male) ? "Emperor" : "Empress");
						msg_broadcast_format(i, "\374\377v%s is henceforth known as %s %s", q_ptr->name, (q_ptr->male) ? "Emperor" : "Empress", q_ptr->name);
						if (!is_admin(q_ptr)) l_printf("%s \\{v%s (%d) has been crowned %s\n", showdate(), q_ptr->name, q_ptr->lev, q_ptr->male ? "emperor" : "empress");
#ifdef TOMENET_WORLDS
						if (cfg.worldd_pwin) world_msg(format("\374\377v%s is henceforth known as %s %s", q_ptr->name, (q_ptr->male) ? "Emperor" : "Empress", q_ptr->name));
#endif
					} else {
						msg_format(i, "\374\377GYou have won the game and are henceforth titled '%s!'", (q_ptr->male) ? "King" : "Queen");
						msg_broadcast_format(i, "\374\377v%s is henceforth known as %s %s", q_ptr->name, (q_ptr->male) ? "King" : "Queen", q_ptr->name);
						if (!is_admin(q_ptr)) l_printf("%s \\{v%s (%d) has been crowned %s\n", showdate(), q_ptr->name, q_ptr->lev, q_ptr->male ? "king" : "queen");
#ifdef TOMENET_WORLDS
						if (cfg.worldd_pwin) world_msg(format("\374\377v%s is henceforth known as %s %s", q_ptr->name, (q_ptr->male) ? "King" : "Queen", q_ptr->name));
#endif
					}
					msg_print(i, "\377G(You may retire (by committing suicide) when you are ready.)");

					num++;

					/* Set all his artifacts to double-speed timeout */
					for (j = 0; j < INVEN_TOTAL; j++)
						if (p_ptr->inventory[j].name1 &&
						    p_ptr->inventory[j].name1 != ART_RANDART
#ifdef L100_ARTS_LAST
						    && p_ptr->inventory[j].name1 != ART_POWER
						    && p_ptr->inventory[j].name1 != ART_BLADETURNER
#endif
						    )
							a_info[p_ptr->inventory[j].name1].winner = TRUE;

					/* Set his retire_timer if neccecary */
					if (cfg.retire_timer >= 0) {
						q_ptr->retire_timer = cfg.retire_timer;
						msg_format(i, "Otherwise you will retire after %d minutes of tenure.", cfg.retire_timer);
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
			apply_magic(wpos, &prize, -1, TRUE, TRUE, TRUE, FALSE, resf_chosen);

			prize.number = num;
			prize.level = 45;
			prize.note = local_quark;
			prize.note_utag = strlen(quark_str(local_quark));

			/* Drop it in the dungeon */
			if (wpos->wz) prize.marked2 = ITEM_REMOVAL_NEVER;
			else prize.marked2 = ITEM_REMOVAL_DEATH_WILD;
			drop_near(0, &prize, -1, wpos, y, x);

			/* Mega-Hack -- Prepare to make "Morgoth" */
			invcopy(&prize, lookup_kind(TV_CROWN, SV_MORGOTH));
			/* Mega-Hack -- Mark this item as "Morgoth" */
			prize.name1 = ART_MORGOTH;
			/* Mega-Hack -- Actually create "Morgoth" */
			apply_magic(wpos, &prize, -1, TRUE, TRUE, TRUE, FALSE, resf_chosen);

			prize.number = num;
			prize.level = 45;
			prize.note = local_quark;
			prize.note_utag = strlen(quark_str(local_quark));

			/* Drop it in the dungeon */
			if (wpos->wz) prize.marked2 = ITEM_REMOVAL_NEVER;
			else prize.marked2 = ITEM_REMOVAL_DEATH_WILD;
			drop_near(0, &prize, -1, wpos, y, x);


			/* Special reward: 1 *great* acquirement item per player. */
			i = object_level;
			object_level = 127;
			acquirement(wpos, y, x, num, TRUE, TRUE, RESF_WINNER | RESF_LIFE | RESF_NOTRUEART | RESF_EGOHI);
			object_level = i;

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
			return TRUE;

		} else if (strstr((r_name + r_ptr->name),"Smeagol")) {
			/* Get local object */
			qq_ptr = &forge;

			object_wipe(qq_ptr);

			/* Mega-Hack -- Prepare to make a ring of invisibility */
			/* Sorry, =inv is too nice.. */
			//invcopy(qq_ptr, lookup_kind(TV_RING, SV_RING_INVIS));
			invcopy(qq_ptr, lookup_kind(TV_RING, SV_RING_STEALTH));
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));

			apply_magic(wpos, qq_ptr, -1, TRUE, TRUE, FALSE, FALSE, RESF_NONE);

			qq_ptr->bpval = 5;
			/* Drop it in the dungeon */
			drop_near(0, qq_ptr, -1, wpos, y, x);

		/* finally made Robin Hood drop a Bow ;) */
		} else if (strstr((r_name + r_ptr->name),"Robin Hood, the Outlaw") && magik(50)) {
			qq_ptr = &forge;
			object_wipe(qq_ptr);
			invcopy(qq_ptr, lookup_kind(TV_BOW, SV_LONG_BOW));
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));
			apply_magic(wpos, qq_ptr, -1, TRUE, TRUE, TRUE, TRUE, resf_drops);
			drop_near(0, qq_ptr, -1, wpos, y, x);

		} else if (r_ptr->flags7 & RF7_NAZGUL) {
			/* Get local object */
			qq_ptr = &forge;

			object_wipe(qq_ptr);

			/* Mega-Hack -- Prepare to make a Ring of Power */
			invcopy(qq_ptr, lookup_kind(TV_RING, SV_RING_SPECIAL));
			qq_ptr->number = 1;

			qq_ptr->name1 = ART_RANDART;

			/* Piece together a 32-bit random seed */
			qq_ptr->name3 = (u32b)rand_int(0xFFFF) << 16;
			qq_ptr->name3 += rand_int(0xFFFF);

			/* Check the tval is allowed */
			//if (randart_make(qq_ptr) != NULL)

			apply_magic(wpos, qq_ptr, -1, FALSE, TRUE, FALSE, FALSE, RESF_NONE);

			/* Save the inscription */
			/* (pfft, not so smart..) */
			/*qq_ptr->note = quark_add(format("#of %s", r_name + r_ptr->name));*/
			qq_ptr->bpval = m_ptr->r_idx;

			/* Drop it in the dungeon */
#ifdef PRE_OWN_DROP_CHOSEN
			qq_ptr->level = 0;
			qq_ptr->owner = p_ptr->id;
			qq_ptr->mode = p_ptr->mode;
			qq_ptr->iron_trade = p_ptr->iron_trade; //not sure if Nazgul rings should really be tradeable in IDDC..
#endif
			drop_near(0, qq_ptr, -1, wpos, y, x);

		/* Hack - the Dragonriders give some firestone */
		} else if (r_ptr->flags3 & RF3_DRAGONRIDER) {
			/* Get local object */
			qq_ptr = &forge;

			/* Prepare to make some Firestone */
			if (magik(70)) invcopy(qq_ptr, lookup_kind(TV_FIRESTONE, SV_FIRESTONE));
			else invcopy(qq_ptr, lookup_kind(TV_FIRESTONE, SV_FIRE_SMALL));
			qq_ptr->number = (byte)rand_range(1,12);

			/* Drop it in the dungeon */
			drop_near(0, qq_ptr, -1, wpos, y, x);

		/* PernAngband additions */
		/* Mega^2-hack -- destroying the Stormbringer gives it us! */
		} else if (strstr((r_name + r_ptr->name),"Stormbringer")) {
			/* Get local object */
			qq_ptr = &forge;

			/* Prepare to make the Stormbringer */
			invcopy(qq_ptr, lookup_kind(TV_SWORD, SV_BLADE_OF_CHAOS));

			/* Megahack -- specify the ego */
			qq_ptr->name2 = EGO_STORMBRINGER;

			/* Piece together a 32-bit random seed */
			qq_ptr->name3 = (u32b)rand_int(0xFFFF) << 16;
			qq_ptr->name3 += rand_int(0xFFFF);

			apply_magic(wpos, qq_ptr, -1, FALSE, FALSE, FALSE, FALSE, RESF_NONE);

			qq_ptr->level = 0;
#if 0 /* Stormbringer not pre-owned? hm. */
#ifdef PRE_OWN_DROP_CHOSEN
			qq_ptr->level = 0;
			qq_ptr->owner = p_ptr->id;
			qq_ptr->mode = p_ptr->mode;
			qq_ptr->iron_trade = p_ptr->iron_trade;
#endif
#endif

			qq_ptr->ident |= ID_CURSED;

			/* hack for a good result */
			qq_ptr->to_h = 17 + rand_int(14);
			qq_ptr->to_d = 17 + rand_int(14);

			/* Drop it in the dungeon */
			drop_near(0, qq_ptr, -1, wpos, y, x);

#if 0 /* currently no such book */
		/* Raal's Tomes of Destruction drop a Raal's Tome of Destruction */
		//else if ((strstr((r_name + r_ptr->name),"Raal's Tome of Destruction")) && (rand_int(100) < 20))
		} else if ((strstr((r_name + r_ptr->name),"Raal's Tome of Destruction")) && (magik(1))) {
			/* Get local object */
			qq_ptr = &forge;

			/* Prepare to make a Raal's Tome of Destruction */
			//invcopy(qq_ptr, lookup_kind(TV_MAGIC_BOOK, 8));
			/* Make a Tome of the Hellflame (Udun) */
			invcopy(qq_ptr, lookup_kind(TV_BOOK, 11));

			/* Drop it in the dungeon */
			drop_near(0, qq_ptr, -1, wpos, y, x);
#endif

		} else if (m_ptr->r_idx == RI_LIVING_LIGHTNING) {
			int tries = 100;
			object_type forge_bak, forge_fallback;
			qq_ptr = &forge;
			/* possible loot:
			    skydsm,elec rod,elec ring,elec/mana rune,mage staff,tome of wind
			*/

			while (tries--) {
				object_wipe(qq_ptr);
				invcopy(qq_ptr, lookup_kind(TV_RING, SV_RING_ELEC));
				qq_ptr->number = 1;
				qq_ptr->note = local_quark;
				qq_ptr->note_utag = strlen(quark_str(local_quark));
				apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, TRUE, TRUE, RESF_FORCERANDART | RESF_NOTRUEART);
				/* hack - ensure non-cursed item: */
				if (qq_ptr->to_a > 0) break;
			}
#if 1
			tries = 3000;
			object_copy(&forge_bak, &forge);
			object_copy(&forge_fallback, &forge);
			while (qq_ptr->pval < 6 && tries--) {
				object_copy(&forge, &forge_bak);
				/* Piece together a 32-bit random seed */
				qq_ptr->name3 = (u32b)rand_int(0xFFFF) << 16;
				qq_ptr->name3 += rand_int(0xFFFF);
				randart_make(qq_ptr);
				apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, TRUE, TRUE, RESF_FORCERANDART | RESF_NOTRUEART);

				/* no AGGR/AM items */
				a_ptr = randart_make(qq_ptr);
				if ((a_ptr->flags3 & (TR3_AGGRAVATE | TR3_NO_MAGIC))
				    || artifact_power(a_ptr) < 90) {
					/* hack: don't even use these for fallback */
					qq_ptr->pval = 0;
					continue;
				}

				/* with the new TV_RING chance (higher imm chance) we may try */
				if (!(a_ptr->flags2 & TR2_IM_ELEC)) continue;

				/* keep best result for possible fallback */
				if (qq_ptr->pval > forge_fallback.pval) object_copy(&forge_fallback, qq_ptr);
			}
			if (qq_ptr->pval < 6) object_copy(qq_ptr, &forge_fallback);
#endif
			qq_ptr->timeout_magic = 0;
			drop_near(0, qq_ptr, -1, wpos, y, x);

			object_wipe(qq_ptr);
			invcopy(qq_ptr, lookup_kind(TV_BOOK, 2));
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));
			apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, TRUE, TRUE, RESF_NONE);
			drop_near(0, qq_ptr, -1, wpos, y, x);

			object_wipe(qq_ptr);
			invcopy(qq_ptr, lookup_kind(TV_RUNE, 5));
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));
			apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, TRUE, TRUE, RESF_NONE);
			drop_near(0, qq_ptr, -1, wpos, y, x);

			object_wipe(qq_ptr);
			invcopy(qq_ptr, lookup_kind(TV_RUNE, 8));
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));
			apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, TRUE, TRUE, RESF_NONE);
			drop_near(0, qq_ptr, -1, wpos, y, x);

			object_wipe(qq_ptr);
			invcopy(qq_ptr, lookup_kind(TV_ROD, SV_ROD_ELEC_BALL));
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));
			apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, TRUE, TRUE, RESF_NONE);
			qq_ptr->name2 = EGO_RISTARI;
			qq_ptr->name2b = 0;
			drop_near(0, qq_ptr, -1, wpos, y, x);

			object_wipe(qq_ptr);
			invcopy(qq_ptr, lookup_kind(TV_DRAG_ARMOR, SV_DRAGON_SKY));
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));
			qq_ptr->name1 = ART_RANDART;
			tries = 500;
			while (tries) {
				/* Piece together a 32-bit random seed */
				qq_ptr->name3 = (u32b)rand_int(0xFFFF) << 16;
				qq_ptr->name3 += rand_int(0xFFFF);
				apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, TRUE, TRUE, RESF_FORCERANDART | RESF_NOTRUEART | RESF_LIFE);

				a_ptr = randart_make(qq_ptr);
				if (artifact_power(a_ptr) >= 105 + 5 && /* at least +1 new mod gained; and +extra bonus boost */
				    qq_ptr->to_a > 0 && /* not cursed */
				    !(a_ptr->flags3 & (TR3_AGGRAVATE | TR3_NO_MAGIC)))
					break;
				tries--;
			}
			if (!tries) msg_format(Ind, "RI_LIVING_LIGHTNING: Re-rolling out of tries!");

			qq_ptr->timeout_magic = 0;
			drop_near(0, qq_ptr, -1, wpos, y, x);

		} else if (m_ptr->r_idx == RI_HELLRAISER) {
			/* Get local object */
			qq_ptr = &forge;

			object_wipe(qq_ptr);

			/* Drop Scroll Of Artifact Creation along with loot */
			invcopy(qq_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_ARTIFACT_CREATION));
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));

			apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, FALSE, FALSE, RESF_NONE);

			/* Drop it in the dungeon */
			drop_near(0, qq_ptr, -1, wpos, y, x);

			/* Prepare a second reward */
			object_wipe(qq_ptr);

			/* Drop Potions Of Learning along with loot */
#ifdef EXPAND_TV_POTION
			invcopy(qq_ptr, lookup_kind(TV_POTION, SV_POTION_LEARNING));
#else
			invcopy(qq_ptr, lookup_kind(TV_POTION2, SV_POTION2_LEARNING));
#endif
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));

			apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, FALSE, FALSE, RESF_NONE);

			/* Drop it in the dungeon */
//#ifdef PRE_OWN_DROP_CHOSEN -- learning potion must always be level 0
			qq_ptr->level = 0;
			qq_ptr->owner = p_ptr->id;
			qq_ptr->mode = p_ptr->mode;
			//learning potion must never be tradeable. for non-IDDC_IRON_COOP it actually would be, if any mob dropped it - which none does, fortunately:
			//qq_ptr->iron_trade = p_ptr->iron_trade;
//#endif
			drop_near(0, qq_ptr, -1, wpos, y, x);

		} else if (m_ptr->r_idx == RI_DOR) {
			/* Get local object */
			qq_ptr = &forge;

#if 0
			/* super-charged wands of rockets */
			object_wipe(qq_ptr);
			invcopy(qq_ptr, lookup_kind(TV_WAND, SV_WAND_ROCKETS));
			qq_ptr->number = 2 + rand_int(2);
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));
			apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, TRUE, TRUE, RESF_NONE);
			qq_ptr->pval = qq_ptr->number * 5 + 3 + rand_int(4);
			drop_near(0, qq_ptr, -1, wpos, y, x);
#endif

			/* a rod of havoc */
			object_wipe(qq_ptr);
			invcopy(qq_ptr, lookup_kind(TV_ROD, SV_ROD_HAVOC));
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));
			apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, TRUE, TRUE, RESF_NONE);
			/* hack ego power */
			qq_ptr->name2 = EGO_RISTARI;
			qq_ptr->name2b = 0;
			drop_near(0, qq_ptr, -1, wpos, y, x);

		/* dungeon boss, but drops multiple items */
		} else if (m_ptr->r_idx == RI_ZU_AON) {
			dun_level *l_ptr = getfloor(&p_ptr->wpos);

			l_ptr->flags2 |= LF2_COLLAPSING;
			nether_realm_collapsing = TRUE;
			nrc_x = m_ptr->fx;
			nrc_y = m_ptr->fy;

#if 1
			for (i = 1; i <= NumPlayers; i++) {
				if (inarea(&Players[i]->wpos, &p_ptr->wpos)
				    && !is_admin(Players[i]))
					l_printf("%s \\{U%s made it through the Nether Realm\n", showdate(), Players[i]->name);
			}
#endif

			/* Get local object */
			qq_ptr = &forge;
			object_wipe(qq_ptr);
			/* Drop Scroll Of Artifact Creation */
			invcopy(qq_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_ARTIFACT_CREATION));
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));
			apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, FALSE, FALSE, RESF_NONE);
			/* Drop it in the dungeon */
			drop_near(0, qq_ptr, -1, wpos, y, x);

			/* Prepare a second reward */
			object_wipe(qq_ptr);
			/* Drop Potions Of Learning along with loot */
#ifdef EXPAND_TV_POTION
			invcopy(qq_ptr, lookup_kind(TV_POTION, SV_POTION_LEARNING));
#else
			invcopy(qq_ptr, lookup_kind(TV_POTION2, SV_POTION2_LEARNING));
#endif
			qq_ptr->number = 1;
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));
			apply_magic(wpos, qq_ptr, 150, TRUE, TRUE, FALSE, FALSE, RESF_NONE);
			/* Drop it in the dungeon */
//#ifdef PRE_OWN_DROP_CHOSEN -- learning potion must always be level 0
			qq_ptr->level = 0;
			qq_ptr->owner = p_ptr->id;
			qq_ptr->mode = p_ptr->mode;
			//learning potion must never be tradeable. for non-IDDC_IRON_COOP it actually would be, if any mob dropped it - which none does, fortunately:
			//qq_ptr->iron_trade = p_ptr->iron_trade;
//#endif
			drop_near(0, qq_ptr, -1, wpos, y, x);

		} else if (!pvp) {
			a_idx = 0;
			chance = 0;
			I_kind = 0;

			if (strstr((r_name + r_ptr->name)," Mardra, rider of the Gold Loranth")) {
				a_idx = ART_MARDRA;
				chance = 55;
			} else if (strstr((r_name + r_ptr->name), "Saruman of Many Colours")) {
				a_idx = ART_ELENDIL;
				chance = 30;

				/* If Elendil drop fails, go for a top mage staff instead */
				if (magik(chance) && !cfg.arts_disabled && (a_info[a_idx].cur_num == 0)) chance = 100;
				else {
					a_idx = 0;

					/* Mage staff drop - added after treasure classes were fixed. */
					qq_ptr = &forge;
					object_wipe(qq_ptr);
					invcopy(qq_ptr, lookup_kind(TV_MSTAFF, SV_MSTAFF));
					qq_ptr->number = 1;
					qq_ptr->note = local_quark;
					qq_ptr->note_utag = strlen(quark_str(local_quark));
					apply_magic(wpos, qq_ptr, -1, TRUE, TRUE, TRUE, TRUE, resf_drops);
					/* hack ego power if not art already */
					if (!qq_ptr->name1) {
						qq_ptr->name2 = EGO_MWIZARDRY;
						qq_ptr->pval = 10;
						determine_level_req(object_level, qq_ptr);
					}
					drop_near(0, qq_ptr, -1, wpos, y, x);
				}
			} else if (strstr((r_name + r_ptr->name), "Gorlim, Betrayer of Barahir")) {
				a_idx = ART_GORLIM;
				chance = 30;
			} else if (strstr((r_name + r_ptr->name), "Hagen, son of Alberich")) { /* not in the game */
				a_idx = ART_NIMLOTH;
				chance = 66;
			} else if (strstr((r_name + r_ptr->name), "Gothmog, the High Captain of Balrogs")) {
				a_idx = ART_GOTHMOG;
				chance = 80;
			} else if (strstr((r_name + r_ptr->name), "Eol, the Dark Elf")) {
				if (magik(25)) a_idx = ART_ANGUIREL;
				else a_idx = ART_EOL;
				chance = 65;
			} else if (strstr((r_name + r_ptr->name), "Kronos, Lord of the Titans")) {
				a_idx = ART_KRONOS;
				chance = 80;
			/* Wyrms have a chance of dropping The Amulet of Grom, the Wyrm Hunter: -C. Blue */
			} else if ((r_ptr->flags3 & RF3_DRAGON)) {
				a_idx = ART_AMUGROM;
				chance = 101;

				/* only powerful wyrms may have a chance of dropping it */
				if ((m_ptr->maxhp < 6000) && rand_int(300)) a_idx = 0;/* strong wyrms at 6000+ */
				else if ((m_ptr->maxhp >= 6000) && (m_ptr->maxhp < 10000) && rand_int(150)) a_idx = 0;
				else if ((m_ptr->maxhp >= 10000) && rand_int(75)) a_idx = 0;/* gwop ^^ */
			}

#ifdef SEMI_PROMISED_ARTS_MODIFIER
			if (chance < 100 / SEMI_PROMISED_ARTS_MODIFIER) /* never turn into zero */
				chance = 1;
			else if (chance < 101) /* 101 = always drops */
				chance = (chance * SEMI_PROMISED_ARTS_MODIFIER) / 100;
#endif

			//if ((a_idx > 0) && ((randint(99)<chance) || (wizard)))
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

				if (!(resf_chosen & RESF_NOTRUEART) ||
				    ((resf_chosen & RESF_WINNER) && winner_artifact_p(qq_ptr))) {
					/* Extract the fields */
					qq_ptr->pval = a_ptr->pval;
					qq_ptr->ac = a_ptr->ac;
					qq_ptr->dd = a_ptr->dd;
					qq_ptr->ds = a_ptr->ds;
					qq_ptr->to_a = a_ptr->to_a;
					qq_ptr->to_h = a_ptr->to_h;
					qq_ptr->to_d = a_ptr->to_d;
					qq_ptr->weight = a_ptr->weight;

					object_desc(Ind, o_name, qq_ptr, TRUE, 3);
					s_printf("DROP_CHOSEN: '%s'\n", o_name);

					if (local_quark) {
						qq_ptr->note = local_quark;
						qq_ptr->note_utag = strlen(quark_str(local_quark));
					}

					//random_artifact_resistance(qq_ptr);
					handle_art_inum(a_idx);

					/* Hack -- acquire "cursed" flag */
					if (a_ptr->flags3 & (TR3_CURSED)) qq_ptr->ident |= (ID_CURSED);

					/* Complete generation, especially level requirements check */
					apply_magic(wpos, qq_ptr, -2, FALSE, TRUE, FALSE, FALSE, resf_chosen);

					/* Little sanity hack for level requirements
					   of the Ring of Phasing - would be 92 otherwise */
					if (a_idx == ART_PHASING) {
						qq_ptr->level = (60 + rand_int(6));
						qq_ptr->marked2 = ITEM_REMOVAL_NEVER;
					}

					/* Drop the artifact from heaven */
#ifdef PRE_OWN_DROP_CHOSEN
					qq_ptr->level = 0;
					qq_ptr->owner = p_ptr->id;
					qq_ptr->mode = p_ptr->mode;
					qq_ptr->iron_trade = p_ptr->iron_trade;
					determine_artifact_timeout(a_idx, wpos);
#endif
					drop_near(0, qq_ptr, -1, wpos, y, x);
				}
			}
		}
	}

#ifdef IDDC_EASY_SPEED_RINGS
 #if IDDC_EASY_SPEED_RINGS > 0
	/* IDDC hacks: Easy speed ring obtaining */
	if ((p_ptr->IDDC_flags & 0x3) &&
	    //basically wyrms+ and some especially feared uniques
	    (m_ptr->level >= 63 || (m_ptr->level >= 59 && (r_ptr->flags1 & RF1_UNIQUE))) &&
	    (r_ptr->flags1 & (RF1_DROP_GOOD | RF1_DROP_GREAT)) &&
	    //not TOO easy
  #if IDDC_EASY_SPEED_RINGS > 1
	    !rand_int(10)) {
  #else
	    !rand_int(15)) {
  #endif
		s_printf("Player '%s' : IDDC_flags %d -> ", p_ptr->name, p_ptr->IDDC_flags);
		/* decrement the last-2-bits-number by 1 */
		i = (p_ptr->IDDC_flags & 0x3) - 1;
		p_ptr->IDDC_flags = (p_ptr->IDDC_flags & ~0x3) | i;
		s_printf("%d\n", p_ptr->IDDC_flags);

		/* Get local object */
		qq_ptr = &forge;
		object_wipe(qq_ptr);

		invcopy(qq_ptr, lookup_kind(TV_RING, SV_RING_SPEED));
		qq_ptr->number = 1;
		if (local_quark) {
			qq_ptr->note = local_quark;
			qq_ptr->note_utag = strlen(quark_str(local_quark));
		}
		apply_magic(wpos, qq_ptr, -1, TRUE, TRUE, FALSE, FALSE, RESF_NONE);

		qq_ptr->bpval = 8 + rand_int(3); //make it decent
		qq_ptr->ident &= ~ID_CURSED; //paranoia
		determine_level_req(0, qq_ptr);

		drop_near(0, qq_ptr, -1, wpos, y, x);
	}
 #endif
#endif

	/* Hack - can find fireworks on new years eve (just exclude dungeons where the scroll would instantly be destroyed on drop in most cases) */
	if (((season_newyearseve && !(d_ptr && (d_ptr->type == DI_MT_DOOM || d_ptr->type == DI_SMALL_WATER_CAVE || d_ptr->type == DI_SUBMERGED_RUINS)))
#ifdef FIREWORK_DUNGEON
	    || (firework_dungeon_chance && d_ptr && d_ptr->type == firework_dungeon && !rand_int(firework_dungeon_chance))
#endif
	    ) &&
	    /* Monster is one of these common, non-aquatic humanoids? Then it's eligible. */
	    ((r_ptr->flags3 & (RF3_ORC | RF3_TROLL | RF3_GIANT)) || /*note: demons/undead don't do fireworks..*/
	    (strchr("hHkpty", r_ptr->d_char))) &&
	    !(r_ptr->flags7 & RF7_AQUATIC) &&
#if 0
	    /* Monster does usually drop at least one item? Then it's eligible. */
	    do_item &&
	    ((r_ptr->flags1 & (RF1_DROP_60 | RF1_DROP_90 | RF1_DROP_1D2 | RF1_DROP_2D2 | RF1_DROP_3D2 | RF1_DROP_4D2))
	    || (r_ptr->flags0 & (RF0_DROP_1 | RF0_DROP_2))) &&
#endif
	    (!rand_int(100)
	    || p_ptr->admin_dm || p_ptr->admin_wiz)) {
		/* Get local object */
		qq_ptr = &forge;

		/* Prepare to make some Firestone */
		invcopy(qq_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_FIREWORK));
		qq_ptr->number = 1;
		apply_magic(wpos, qq_ptr, -1, TRUE, TRUE, FALSE, FALSE, RESF_NONE);

		s_printf("NEWYEARSEVE: Dropped fireworks (%d,%d) for '%s'.\n", qq_ptr->xtra1, qq_ptr->xtra2, p_ptr->name);
		drop_near(0, qq_ptr, -1, wpos, y, x);
	}

	/* for when a quest giver turned non-invincible */
	if (m_ptr->questor) {
		if (q_info[m_ptr->quest].defined && q_info[m_ptr->quest].questors > m_ptr->questor_idx) {
			/* Drop a specific item? */
			if (q_info[m_ptr->quest].questor[m_ptr->questor_idx].drops & 0x2)
				questor_drop_specific(Ind, m_ptr->quest, m_ptr->questor_idx, wpos, x, y);
			/* Quest progression/fail effect? */
			questor_death(m_ptr->quest, m_ptr->questor_idx, wpos, 0);
		} else {
			s_printf("QUESTOR DEPRECATED (monster_dead2)\n");
		}
	}

	//if ((!force_coin) && (randint(100) < 50)) place_corpse(m_ptr);

	/* Only process "Quest Monsters" */
	if (!(r_ptr->flags1 & RF1_QUESTOR)) return TRUE;

	/* Hack -- Mark quests as complete */
	for (i = 0; i < MAX_XO_IDX; i++) {
		/* Hack -- note completed quests */
		if (xo_list[i].level == r_ptr->level) xo_list[i].level = 0;

		/* Count incomplete quests */
		if (xo_list[i].level) total++;
	}


	/* Need some stairs */
	if (total) {
		/* Stagger around */
		while (!cave_valid_bold(zcave, y, x)) {
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

	return TRUE;
}

/* FIXME: this function is known to be bypassable by nominally
 * 'party-owning'.
 */
void kill_house_contents(house_type *h_ptr) {
	struct worldpos *wpos = &h_ptr->wpos;
	object_type *o_ptr;
	int i;

	/* hack: for PLAYER_STORES actually allocate the cave temporarily,
	   just for delete_object()->delete_object_idx() to find any offered items and log their removal. */
	bool must_alloc = (getcave(wpos) == NULL);
	if (must_alloc) {
		alloc_dungeon_level(wpos);
		generate_cave(wpos, NULL);
	}

#ifdef USE_MANG_HOUSE
	if (h_ptr->flags & HF_RECT) {
		int sy, sx, ey, ex, x, y;
		sy = h_ptr->y + 1;
		sx = h_ptr->x + 1;
		ey = h_ptr->y + h_ptr->coords.rect.height - 1;
		ex = h_ptr->x + h_ptr->coords.rect.width - 1;
		for (y = sy; y < ey; y++) {
			for (x = sx; x < ex; x++)
				delete_object(wpos, y, x, TRUE);
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

		//todo: teleport any players inside out */
	}
#endif	// USE_MANG_HOUSE

#ifndef USE_MANG_HOUSE_ONLY
	for (i = 0; i < h_ptr->stock_num; i++) {
		o_ptr = &h_ptr->stock[i];
		if (o_ptr->k_idx && true_artifact_p(o_ptr))
			handle_art_d(o_ptr->name1);
		questitem_d(o_ptr, o_ptr->number);

#ifdef PLAYER_STORES
		/* Log removal of player store items */
		if (o_ptr->note && strstr(quark_str(o_ptr->note), "@S")) {
			char o_name[ONAME_LEN];//, p_name[NAME_LEN];
			object_desc(0, o_name, o_ptr, TRUE, 3);
			//s_printf("PLAYER_STORE_REMOVED (khouse): %s - %s (%d,%d,%d; %d,%d).\n",
			s_printf("PLAYER_STORE_REMOVED (khouse): %s (%d,%d,%d; %d,%d; %d).\n",
			    //p_name, o_name, wpos->wx, wpos->wy, wpos->wz,
			    o_name, wpos->wx, wpos->wy, wpos->wz,
			    o_ptr->ix, o_ptr->iy, i);
		}
#endif

		invwipe(o_ptr);
	}
	h_ptr->stock_num = 0;
#endif	// USE_MANG_HOUSE_ONLY

#ifdef HOUSE_PAINTING
	/* Remove house paint */
	h_ptr->colour = 0;
	fill_house(h_ptr, FILL_UNPAINT, NULL);
#endif

	/* hack: reset size/price of extendable (trad) houses */
	if ((h_ptr->flags & HF_TRAD)) {
		int area = (h_ptr->coords.rect.width - 2) * (h_ptr->coords.rect.height - 2);
		h_ptr->stock_size = (area >= STORE_INVEN_MAX) ? STORE_INVEN_MAX : area;

		/* note: trad houses currently can't have sizes > 30 (compare wild.c),
		   so it's sufficient to add term for medium houses + term for small houses,
		   ignoring the term for large houses. */
		h_ptr->dna->price = initial_house_price(h_ptr);
	}

	if (must_alloc) dealloc_dungeon_level(wpos);
}

void kill_houses(int id, byte type) {
	int i;
	for (i = 0; i < num_houses; i++) {
		struct dna_type *dna = houses[i].dna;
		if (dna->owner == id && dna->owner_type == type) {
			dna->owner = 0L;
			dna->creator = 0L;
			dna->a_flags = ACF_NONE;
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
	for (i = 0; i < o_max; i++) {
		o_ptr = &o_list[i];
		if (!o_ptr->k_idx) continue;
		if (o_ptr->owner == id) {
			o_ptr->owner = MAX_ID + 1;
			/* o_ptr->mode = 0; <- makes everlasting items usable! bad! */
		}
	}
}


#define QUIT_BAN_NONE	0
#define QUIT_BAN_ROLLER	1
#define QUIT_BAN_ALL	2

/* This function prevents DoS attack using suicide */
/* ;) DoS... its just annoying. hehe */
static void check_roller(int Ind) {
	player_type *p_ptr = Players[Ind];
	time_t now = time(&now);

	/* This was necessary ;) */
	if(is_admin(p_ptr)) return;

	if (!cfg.quit_ban_mode) return;

	if (cfg.quit_ban_mode == QUIT_BAN_ROLLER) {
		/* (s)he should have played somewhat */
		if (p_ptr->max_exp) return;

		/* staying for more than 60 seconds? */
		if (now - lookup_player_laston(p_ptr->id) > 60) return;

		/* died to a townie?
		if (p_ptr->ghost) return; */
	}

	/* ban her/him for 1 min */
	add_banlist(p_ptr->accountname, NULL, p_ptr->hostname, 1, "Suicide spam");
}


static void check_killing_reward(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* reward top gladiators */
	if (p_ptr->kills >= 10) {
		/* reset! */
		//p_ptr->kills -= 10;
		p_ptr->kills = 0;

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
	int i;

	//ACC_HOUSE_LIMIT
	i = acc_get_houses(p_ptr->accountname);
	i -= p_ptr->houses_owned;
	acc_set_houses(p_ptr->accountname, i);

	kill_houses(p_ptr->id, OT_PLAYER);
	rem_xorder(p_ptr->xorder_id);
	kill_objs(p_ptr->id);
	p_ptr->death = TRUE;

#ifdef AUCTION_SYSTEM
	auction_player_death(p_ptr->id);
#endif

	/* Remove him from his party/guild */
	if (p_ptr->party) {
		/* He leaves */
		party_leave(Ind, FALSE);
	}
	if (p_ptr->guild) {
		if ((guilds[p_ptr->guild].flags & GFLG_AUTO_READD)) {
			acc_set_guild(p_ptr->accountname, p_ptr->guild);
			acc_set_guild_dna(p_ptr->accountname, p_ptr->guild_dna);
		}
		if ((p_ptr->guild_flags & PGF_ADDER))
			acc_set_flags(p_ptr->accountname, ACC_GUILD_ADDER, TRUE);
		guild_leave(Ind, FALSE);
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

	/* temporarily reserve his character name in case he want's to remake that character */
	for (i = 0; i < MAX_RESERVED_NAMES; i++) {
		if (reserved_name_character[i][0]) continue;

		strcpy(reserved_name_character[i], p_ptr->name);
		strcpy(reserved_name_account[i], p_ptr->accountname);
		reserved_name_timeout[i] = 60 * 24; //minutes
		break;
	}
	if (i == MAX_RESERVED_NAMES)
		s_printf("Warning: Coudln't reserve character name '%s' for account '%s'!\n", p_ptr->name, p_ptr->accountname);

	/* Remove him from the player name database */
	delete_player_name(p_ptr->name);

	/* Put him on the high score list */
	if (!p_ptr->noscore && !(p_ptr->mode & (MODE_PVP | MODE_EVERLASTING)))
		add_high_score(Ind);

	/* Format string for death reason */
	if (death_type == DEATH_QUIT) strcpy(buf, "Committed suicide");
	else if (!strcmp(p_ptr->died_from, "It") || !strcmp(p_ptr->died_from, "insanity") || p_ptr->image)
		snprintf(buf, sizeof(buf), "Killed by %s (%d pts)", p_ptr->really_died_from, total_points(Ind));
	else snprintf(buf, sizeof(buf), "Killed by %s (%d pts)", p_ptr->died_from, total_points(Ind));

	/* Get rid of him */
	Destroy_connection(p_ptr->conn, buf);
}

static void inven_death_damage(int Ind, int verbose) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	char o_name[ONAME_LEN];
	int shuffle[INVEN_PACK];
	int inventory_loss = 0;
	int i, j, k;

	for (i = 0; i < INVEN_PACK; i++) shuffle[i] = i;
	intshuffle(shuffle, INVEN_PACK);

	/* Destroy some items randomly */
	for (i = 0; i < INVEN_PACK; i++) {
		j = shuffle[i];
		o_ptr = &p_ptr->inventory[j];
		if (!o_ptr->k_idx) continue;

		/* hack: don't discard his remaining gold - a penalty was already deducted from it */
		if (o_ptr->tval == TV_GOLD) continue;
		/* guild keys are supposedly indestructible, so whatever.. */
		if (o_ptr->tval == TV_KEY && o_ptr->sval == SV_GUILD_KEY) continue;
#ifdef DEATH_PACK_ITEM_NICE
		/* don't be too nasty to magic device users */
		if (is_magic_device(o_ptr->tval)) {
			k = object_value_real(0, o_ptr);
			if (k >= 2000 && rand_int(2)) continue;//basic bolt rods
			if (k >= 30000 && rand_int(2)) continue;//anni wand (rocket,havoc)
		}
		/* ..and to rare book carriers */
		if (o_ptr->tval == TV_BOOK &&
		    (o_ptr->sval == SV_CUSTOM_TOME_2 || o_ptr->sval == SV_CUSTOM_TOME_3)
		    && rand_int(2))
			continue;
#endif

		if (magik(DEATH_PACK_ITEM_LOST)) {
			object_desc(Ind, o_name, o_ptr, TRUE, 3);
			s_printf("item_lost: %s (slot %d)\n", o_name, j);

			if (verbose) {
				/* Message */
				msg_format(Ind, "\376\377oYour %s %s destroyed!", o_name,
				    ((o_ptr->number > 1) ? "were" : "was"));
			}

			if (true_artifact_p(o_ptr)) {
				/* set the artifact as unfound */
				handle_art_d(o_ptr->name1);
			}
			questitem_d(o_ptr, o_ptr->number);

			inven_item_increase(Ind, j, -(o_ptr->number));
			inven_item_optimize(Ind, j);
			inventory_loss++;

			if (inventory_loss >= 4) {
				break;
			}
		}
	}
}

static void equip_death_damage(int Ind, int verbose) {
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	char o_name[ONAME_LEN];
	int shuffle[INVEN_TOTAL];
	int equipment_loss = 0;
	int i, j;

	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) shuffle[i] = i;
	intshuffle(shuffle + INVEN_WIELD, INVEN_EQ);

	/* Destroy some items randomly */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		j = shuffle[i];
		o_ptr = &p_ptr->inventory[j];
		if (!o_ptr->k_idx) continue;

		if (magik(DEATH_EQ_ITEM_LOST)) {
			object_desc(Ind, o_name, o_ptr, TRUE, 3);
			s_printf("item_lost: %s (slot %d)\n", o_name, j);

			if (verbose) {
				/* Message */
				msg_format(Ind, "\376\377oYour %s %s destroyed!", o_name,
				    ((o_ptr->number > 1) ? "were" : "was"));
			}

			if (true_artifact_p(o_ptr)) {
				/* set the artifact as unfound */
				handle_art_d(o_ptr->name1);
			}
			questitem_d(o_ptr, o_ptr->number);

			inven_item_increase(Ind, j, -(o_ptr->number));
			inven_item_optimize(Ind, j);
			equipment_loss++;

			if (equipment_loss >= 1) {
				break;
			}
		}
	}
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

void player_death(int Ind) {
	player_type *p_ptr = Players[Ind], *p_ptr2 = NULL;
	int Ind2;
	object_type *o_ptr;
	monster_type *m_ptr;
	dungeon_type *d_ptr = getdungeon(&p_ptr->wpos);
	dun_level *l_ptr = getfloor(&p_ptr->wpos);
	char buf[1024], m_name_extra[MNAME_LEN], msg_layout = 'a';
	int i, k, j, tries = 0;
#if 0
	int inventory_loss = 0, equipment_loss = 0;
#endif
	//int inven_sort_map[INVEN_TOTAL];
	//wilderness_type *wild;
	bool hell = TRUE, secure = FALSE, ge_secure = FALSE, pvp = ((p_ptr->mode & MODE_PVP) != 0), erase = FALSE, insanity = streq(p_ptr->died_from, "insanity");
	cptr titlebuf;
	int death_type = -1; /* keep track of the way (s)he died, for buffer_account_for_event_deed() */
	bool world_broadcast = TRUE, just_fruitbat_transformation = (p_ptr->fruit_bat == -1);
	bool was_total_winner = p_ptr->total_winner, retire = FALSE;
	bool in_iddc = in_irondeepdive(&p_ptr->wpos);


	/* Cancel any pending Word of Recall, of course. */
	p_ptr->word_recall = 0;

	/* Get him out of any pending request input prompts :-p */
	if (p_ptr->request_id != RID_NONE) {
		Send_request_abort(Ind);
		p_ptr->request_id = RID_NONE;
	}

#ifdef SHOW_REALLY_DIED_FROM
	if ((streq(p_ptr->died_from, "It") || p_ptr->image
 #ifdef SHOW_REALLY_DIED_FROM_INSANITY
	    || insanity
 #endif
	    ) && !(
 #ifndef SHOW_REALLY_DIED_FROM_INSANITY
	    insanity ||
 #endif
	    streq(p_ptr->died_from, "indecisiveness") ||
	    streq(p_ptr->died_from, "indetermination"))
	    && p_ptr->really_died_from[0]) //paranoia?
		strcpy(p_ptr->died_from, p_ptr->really_died_from);
#endif

	/* character-intrinsic conditions violated -> unpreventable no-ghost death */
	if (streq(p_ptr->died_from, "indecisiveness") ||
	    streq(p_ptr->died_from, "indetermination"))
		erase = TRUE;

	/* Amulet of immortality prevents death */
	if (!erase && p_ptr->admin_invuln) {
		if (just_fruitbat_transformation) p_ptr->fruit_bat = 0;
		return;
	}

	/* prepare player's title */
	titlebuf = get_ptitle(p_ptr, FALSE);

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
			if (cfg.worldd_pdeath) world_msg(buf);
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
		handle_store_leave(Ind);
		Send_store_kick(Ind);
	}

	if (d_ptr && (d_ptr->flags2 & DF2_NO_DEATH)) secure = TRUE;

	if (in_iddc
	    && !is_admin(p_ptr)) {
		for (i = 0; i < IDDC_HIGHSCORE_SIZE; i++) {
			if (deep_dive_level[i] >= ABS(p_ptr->wpos.wz) || deep_dive_level[i] == -1) {
#ifdef IDDC_RESTRICT_ACC_CLASS /* only allow one entry of same account+class? : discard new entry */
				if (!strcmp(deep_dive_account[i], p_ptr->accountname) &&
				    deep_dive_class[i] == p_ptr->pclass) {
					i = IDDC_HIGHSCORE_DISPLAYED;
					break;
				}
#endif
				continue;
			}
#ifdef IDDC_RESTRICT_ACC_CLASS /* only allow one entry of same account+class? : discard previous entry */
			for (j = i; j < IDDC_HIGHSCORE_SIZE - 1; j++) {
				if (!strcmp(deep_dive_account[j], p_ptr->accountname) &&
				    deep_dive_class[i] == p_ptr->pclass) {
					int k;

					/* pull up all succeeding entries by 1  */
					for (k = j; k < IDDC_HIGHSCORE_SIZE - 1; k++) {
						deep_dive_level[k] = deep_dive_level[k + 1];
						strcpy(deep_dive_name[k], deep_dive_name[k + 1]);
						strcpy(deep_dive_char[k], deep_dive_char[k + 1]);
						strcpy(deep_dive_account[k], deep_dive_account[k + 1]);
						deep_dive_class[k] = deep_dive_class[k + 1];
					}
					break;
				}
			}
#endif
			/* push down all entries by 1, to make room for inserting new entry */
			for (j = IDDC_HIGHSCORE_SIZE - 1; j > i; j--) {
				deep_dive_level[j] = deep_dive_level[j - 1];
				strcpy(deep_dive_name[j], deep_dive_name[j - 1]);
				strcpy(deep_dive_char[j], deep_dive_char[j - 1]);
				strcpy(deep_dive_account[j], deep_dive_account[j - 1]);
				deep_dive_class[j] = deep_dive_class[j - 1];
			}
			deep_dive_level[i] = ABS(p_ptr->wpos.wz);
			//strcpy(deep_dive_name[i], p_ptr->name);
#ifdef IDDC_HISCORE_SHOWS_ICON
			sprintf(deep_dive_name[i], "%s, %s %s (\\{%c%c\\{s/\\{%c%d\\{s),",
			    p_ptr->name, get_prace(p_ptr), class_info[p_ptr->pclass].title, color_attr_to_char(p_ptr->cp_ptr->color), p_ptr->fruit_bat ? 'b' : '@',
			    p_ptr->ghost ? 'r' : 's', p_ptr->max_plv);
#else
			sprintf(deep_dive_name[i], "%s, %s %s (\\{%c%d\\{s),",
			    p_ptr->name, get_prace(p_ptr), class_info[p_ptr->pclass].title,
			    p_ptr->ghost ? 'r' : 's', p_ptr->max_plv);
#endif
			strcpy(deep_dive_char[i], p_ptr->name);
			strcpy(deep_dive_account[i], p_ptr->accountname);
			deep_dive_class[i] = p_ptr->pclass;
			break;
		}

		if (i < IDDC_HIGHSCORE_DISPLAYED) {
			sprintf(buf, "\374\377a%s reached floor %d in the Ironman Deep Dive challenge, placing %d%s!",
			    p_ptr->name, ABS(p_ptr->wpos.wz), i + 1, i == 0 ? "st" : (i == 1 ? "nd" : (i == 2 ? "rd" : "th")));
			msg_broadcast(0, buf);
#ifdef TOMENET_WORLDS
			if (cfg.worldd_events) world_msg(buf);
#endif
		} else {
			sprintf(buf, "\374\377a%s reached floor %d in the Ironman Deep Dive challenge!",
			    p_ptr->name, ABS(p_ptr->wpos.wz));
			msg_broadcast(0, buf);
#ifdef TOMENET_WORLDS
			if (cfg.worldd_events) world_msg(buf);
#endif
		}

		if (ABS(p_ptr->wpos.wz) >= 30)
			l_printf("%s \\{s%s (%d) reached floor %d in the Ironman Deep Dive challenge\n",
			    showdate(), p_ptr->name, p_ptr->max_plv, ABS(p_ptr->wpos.wz));
		else if (i < IDDC_HIGHSCORE_SIZE) { /* the score table is updated anyway, even if no l_printf() entry is created */
			char path[MAX_PATH_LENGTH];
			char path_rev[MAX_PATH_LENGTH];
			path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "legends.log");
			path_build(path_rev, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "legends-rev.log");
			reverse_lines(path, path_rev);
		}
	}

	if (ge_special_sector && in_arena(&p_ptr->wpos)) {
		secure = TRUE;
		ge_secure = TRUE; /* extra security for global events */
	}

	if (pvp) {
		secure = FALSE;
		msg_layout = 'L';
	}

	/* Hack -- amulet of life saving */
	if (p_ptr->alive && !erase && (secure ||
	    (p_ptr->inventory[INVEN_NECK].k_idx &&
	    p_ptr->inventory[INVEN_NECK].sval == SV_AMULET_LIFE_SAVING))) {
		s_printf("%s - %s (%d) was pseudo-killed by %s for %d damage at %d, %d, %d.\n", showtime(), p_ptr->name, p_ptr->lev, p_ptr->really_died_from, p_ptr->deathblow, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);

		if (!secure) {
			msg_print(Ind, "\377oYour amulet shatters into pieces!");
			inven_item_increase(Ind, INVEN_NECK, -99);
			//inven_item_describe(Ind, INVEN_NECK);
			inven_item_optimize(Ind, INVEN_NECK);
		}

		/* Cure him from various maladies */
		//p_ptr->black_breath = FALSE;
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
		//ptr->ghost = 0;

		/* Give him his hit points back */
		p_ptr->chp = p_ptr->mhp;
		p_ptr->chp_frac = 0;
		/* Sanity death? Ew. */
		if (p_ptr->csane < 0) p_ptr->csane = 0;

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
			else recall_player(Ind, "\377oYou almost died.. but your life was secured here!");

			p_ptr->safe_sane = TRUE;
			/* Apply small penalty for death */
			if (!ge_secure) { /* except if we were in a special event that protects */
#ifndef ARCADE_SERVER
				p_ptr->au = p_ptr->au * 4 / 5;
				p_ptr->max_exp = (p_ptr->max_exp * 4 + 1) / 5; /* never drop below 1! (Highlander Tournament exploit) */
				p_ptr->exp = p_ptr->max_exp;
#endif
#if 0
			} else {
				if (p_ptr->csane <= 10) p_ptr->csane = 10; /* just something, paranoia */
#endif
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

	if ((!(p_ptr->mode & MODE_NO_GHOST)) && !cfg.no_ghost && !pvp) {
		if(!p_ptr->wpos.wz || !(d_ptr->flags2 & (DF2_HELL | DF2_IRON)))
			hell = FALSE;
	}

	if ((Ind2 = get_esp_link(Ind, LINKF_PAIN, &p_ptr2))) {
		strcpy(p_ptr2->died_from, p_ptr->died_from);
		if (!p_ptr2->ghost) {
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

	if (erase) hell = TRUE;

	/* Morgoth's level might be NO_GHOST! */
	if (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_GHOST)) hell = TRUE;

	/* For global events (Highlander Tournament) */
	/* either instakill in sector 0,0... */
	if (p_ptr->global_event_temp & PEVF_NOGHOST_00) hell = TRUE;
	/* or instead teleport them to surface */
	/* added wpos checks due to weirdness. -Molt */
	if (p_ptr->wpos.wx != WPOS_SECTOR00_X && p_ptr->wpos.wy != WPOS_SECTOR00_Y && (p_ptr->global_event_temp & PEVF_SAFEDUN_00)) {
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
		//p_ptr->ghost = 0;
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

#ifdef ENABLE_INSTANT_RES
	if (p_ptr->insta_res && !erase && p_ptr->alive) {
		char instant_res_possible = TRUE;
		int dlvl = getlevel(&p_ptr->wpos);
		int instant_res_cost = dlvl * dlvl * 10 + 10;

		/* Only everlasters */
		if (!(p_ptr->mode & MODE_EVERLASTING))
			instant_res_possible = FALSE;

		/* If already a ghost, get destroyed */
		if (p_ptr->ghost)
			instant_res_possible = FALSE;

		/* Insanity is a no-ghost death */
		if (insanity)
			instant_res_possible = FALSE;

		/* Not on NO_GHOST levels */
		if (hell)
			instant_res_possible = FALSE;

		/* Not on suicides */
		if (!p_ptr->alive)
			instant_res_possible = FALSE;

 #ifdef INSTANT_RES_EXCEPTION
		/* Not in Nether Realm */
		if (in_netherrealm(&p_ptr->wpos))
			instant_res_possible = FALSE;
 #endif

		/* Divine wrath is meant to kill people */
		if (streq(p_ptr->died_from, "divine wrath"))
			instant_res_possible = FALSE;

		/* Check that the player has enough money */
		if (instant_res_cost > p_ptr->au + p_ptr->balance) {
			msg_print(Ind, "\376\377yYou do not have sufficient funds for instant-resurrection!");
			instant_res_possible = FALSE;
		}

		if (instant_res_possible) {
			int loss_factor, reduce;
			bool has_exp = p_ptr->max_exp != 0;
 #ifndef FALLEN_WINNERSONLY
			int i;
			u32b dummy, f5;
 #endif

			/* Log it */
			s_printf("%s - %s (%d) was defeated by %s for %d damage at %d, %d, %d. (INSTARES)\n", showtime(), p_ptr->name, p_ptr->lev, p_ptr->died_from, p_ptr->deathblow, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			if (!strcmp(p_ptr->died_from, "It") || insanity || p_ptr->image)
				s_printf("(%s was really defeated by %s.)\n", p_ptr->name, p_ptr->really_died_from);

 #ifdef USE_SOUND_2010
			/* Play the death sound */
			if (p_ptr->male) sound(Ind, "death_male", "death", SFX_TYPE_MISC, TRUE);
			else sound(Ind, "death_female", "death", SFX_TYPE_MISC, TRUE);
 #else
			sound(Ind, SOUND_DEATH);
 #endif

			/* Message to other players */
			if (cfg.unikill_format)
				snprintf(buf, sizeof(buf), "\374\377D%s %s (%d) was defeated by %s.", titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from);
			else
				snprintf(buf, sizeof(buf), "\374\377D%s (%d) was defeated by %s.", p_ptr->name, p_ptr->lev, p_ptr->died_from);

			msg_broadcast(Ind, buf);
 #ifdef TOMENET_WORLDS
			if (cfg.worldd_pdeath) world_msg(buf);
 #endif

			/* Add to legends log if he was a winner or very high level */
			if (!is_admin(p_ptr)) {
				if (p_ptr->total_winner)
					l_printf("%s \\{r%s royalty %s (%d) died and was instantly resurrected\n", showdate(), p_ptr->male ? "His" : "Her", p_ptr->name, p_ptr->lev);
				else if (p_ptr->lev >= 50)
					l_printf("%s \\{r%s (%d) died and was instantly resurrected\n", showdate(), p_ptr->name, p_ptr->lev);
			}

			/* Tell him what happened -- moved the messages up here so they get onto the chardump! */
			msg_format(Ind, "\374\377RYou were defeated by %s, but the priests have saved you.", p_ptr->died_from);

			/* new - death dump for insta-res too! */
			Send_chardump(Ind, "-death");

			/* Hm, this doesn't need to be on the char dump actually */
			msg_format(Ind, "\377oThey have requested a fee of %d gold pieces.", instant_res_cost);

			/* Cure him from various maladies */
			if (p_ptr->image) (void)set_image(Ind, 0);
			if (p_ptr->blind) (void)set_blind(Ind, 0);
			if (p_ptr->paralyzed) (void)set_paralyzed(Ind, 0);
			if (p_ptr->confused) (void)set_confused(Ind, 0);
			if (p_ptr->poisoned) (void)set_poisoned(Ind, 0, 0);
			if (p_ptr->stun) (void)set_stun(Ind, 0);
			if (p_ptr->cut) (void)set_cut(Ind, 0, 0);
			(void)set_food(Ind, PY_FOOD_FULL - 1);

			if (p_ptr->black_breath) {
				//msg_print(Ind, "The hold of the Black Breath on you is broken!");
				p_ptr->black_breath = FALSE;
			}

			/* Remove the death flag */
			p_ptr->death = FALSE;

			/* Give him his hit points back */
			p_ptr->chp = p_ptr->mhp;
			p_ptr->chp_frac = 0;

			/* Lose inventory and equipment items as per normal death */
 #ifdef DEATH_PACK_ITEM_LOST
			inven_death_damage(Ind, TRUE);
 #endif
 #ifdef DEATH_EQ_ITEM_LOST
			equip_death_damage(Ind, TRUE);
 #endif

			/* Extract the cost */
			p_ptr->au -= instant_res_cost;
			if (p_ptr->au < 0) {
				p_ptr->balance += p_ptr->au;
				p_ptr->au = 0;
			}

			/* Also lose some cash on death */
			s_printf("gold_lost: carried %d, remaining ", p_ptr->au);
 #if 0 /* lose 0 below 50k, up to 50% up to 500k, 50% after that */
			if (p_ptr->au <= 50000) ;
			else if (p_ptr->au <= 500000) p_ptr->au = (((p_ptr->au) * 100) / (100 + ((p_ptr->au - 50000) / 4500)));
			else p_ptr->au /= 2;
 #else /* lose 5..33% */
			/* overflow handling */
			if (p_ptr->au <= 20000000) p_ptr->au = (p_ptr->au * (rand_int(29) + 67)) / 100;
			else p_ptr->au = (p_ptr->au / 100) * (rand_int(29) + 67);
 #endif
			s_printf("%d.\n", p_ptr->au);

			p_ptr->safe_sane = TRUE;

			/* Lose some experience */
			loss_factor = INSTANT_RES_XP_LOST;
			if (get_skill(p_ptr, SKILL_HCURING) >= 50
 #ifdef ENABLE_OCCULT /* Occult */
			    || get_skill(p_ptr, SKILL_OSPIRIT) >= 50
 #endif
			    ) loss_factor -= 5;

			reduce = p_ptr->max_exp;
			reduce = reduce > 99999 ?
			reduce / 100 * loss_factor : reduce * loss_factor / 100;
			p_ptr->max_exp -= reduce;

			reduce = p_ptr->exp;
			reduce = reduce > 99999 ?
			reduce / 100 * loss_factor : reduce * loss_factor / 100;
			p_ptr->exp -= reduce;

			/* Prevent cheezing exp to 0 to become eligible for certain events */
			if (!p_ptr->max_exp && has_exp) p_ptr->exp = p_ptr->max_exp = 1;

			check_experience(Ind);

			/* Remove massive crown of Morgoth and Grond */
			if (p_ptr->inventory[INVEN_HEAD].k_idx && p_ptr->inventory[INVEN_HEAD].name1 == ART_MORGOTH) {
				char o_name[ONAME_LEN];

				o_ptr = &p_ptr->inventory[INVEN_HEAD];
				object_desc(Ind, o_name, o_ptr, FALSE, 3);
				msg_format(Ind, "\376\377oYour %s was destroyed!", o_name);
				handle_art_d(o_ptr->name1);

				inven_item_increase(Ind, INVEN_HEAD, -(o_ptr->number));
				inven_item_optimize(Ind, INVEN_HEAD);
			}
			if (p_ptr->inventory[INVEN_WIELD].k_idx && p_ptr->inventory[INVEN_WIELD].name1 == ART_GROND) {
				char o_name[ONAME_LEN];

				o_ptr = &p_ptr->inventory[INVEN_WIELD];
				object_desc(Ind, o_name, o_ptr, FALSE, 3);
				msg_format(Ind, "\376\377oYour %s was destroyed!", o_name);
				handle_art_d(o_ptr->name1);

				inven_item_increase(Ind, INVEN_WIELD, -(o_ptr->number));
				inven_item_optimize(Ind, INVEN_WIELD);
			}

			/* update stats */
			p_ptr->update |= PU_SANITY;
			update_stuff(Ind);
			p_ptr->safe_sane = FALSE;

			p_ptr->recall_pos.wx = p_ptr->town_x;
			p_ptr->recall_pos.wy = p_ptr->town_y;
			p_ptr->recall_pos.wz = 0;
			p_ptr->new_level_method = LEVEL_TO_TEMPLE;
			recall_player(Ind, "");

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

			/* No longer a winner */
			p_ptr->total_winner = FALSE;

			if (was_total_winner) {
				/* Set all his artifacts back to normal-speed timeout */
				if (!cfg.fallenkings_etiquette) {
					for (j = 0; j < INVEN_TOTAL; j++)
						if (p_ptr->inventory[j].name1 &&
						    p_ptr->inventory[j].name1 != ART_RANDART)
							a_info[p_ptr->inventory[j].name1].winner = FALSE;
				}

 #ifdef SOLO_REKING
				p_ptr->solo_reking = p_ptr->solo_reking_au = SOLO_REKING;
 #endif
			}

 #ifndef FALLEN_WINNERSONLY
			/* Take off winner artifacts and winner-only items */
			for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
				o_ptr = &p_ptr->inventory[i];
				object_flags(o_ptr, &dummy, &dummy, &dummy, &dummy, &f5, &dummy, &dummy);
				if ((f5 & TR5_WINNERS_ONLY)) {
					bypass_inscrption = TRUE;
					inven_takeoff(Ind, i, 255, FALSE);
				}
			}
 #endif

			/* Redraw */
			p_ptr->redraw |= (PR_BASIC);
			/* Update */
			p_ptr->update |= (PU_BONUS);

			p_ptr->deaths++;
			return;
		}
	}
#endif

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
		p_ptr->inventory[INVEN_PACK].iron_trade = p_ptr->iron_trade;

		/* Drop no more than 32000 gold */
		//if (p_ptr->au > 32000) p_ptr->au = 32000;
		/* (actually, this if-clause is not necessary) */
		s_printf("gold_lost: carried %d, remaining ", p_ptr->au);
#if 0 /* lose 0 below 50k, up to 50% up to 500k, 50% after that */
		if (p_ptr->au <= 50000) ;
		else if (p_ptr->au <= 500000) p_ptr->au = (((p_ptr->au) * 100) / (100 + ((p_ptr->au - 50000) / 4500)));
		else p_ptr->au /= 2;
#else /* lose 5..33% */
		p_ptr->au = (p_ptr->au * (rand_int(29) + 67)) / 100;
#endif

		if (p_ptr->max_plv >= cfg.newbies_cannot_drop) {
			/* Set the amount */
			p_ptr->inventory[INVEN_PACK].pval = p_ptr->au;
			/* set fitting gold 'colour' */
			p_ptr->inventory[INVEN_PACK].k_idx = gold_colour(p_ptr->au, FALSE, TRUE);
			p_ptr->inventory[INVEN_PACK].xtra1 = 1;//mark as 'compact' in case it falls on top of a normal money pile
			p_ptr->inventory[INVEN_PACK].sval = k_info[p_ptr->inventory[INVEN_PACK].k_idx].sval;
			s_printf("%d.\n", p_ptr->au);
		} else {
			invwipe(&p_ptr->inventory[INVEN_PACK]);
			s_printf("none.\n");
		}

		/* No more gold */
		p_ptr->au = 0;
	}

	/* Drop/lose items -------------------------------------------------- */

	/* Don't "lose" items on suicide (they all poof anyway, except for true arts possibly) */
#ifdef DEATH_PACK_ITEM_LOST
	if (p_ptr->alive) inven_death_damage(Ind, FALSE);
#endif
#ifdef DEATH_EQ_ITEM_LOST
	if (p_ptr->alive) equip_death_damage(Ind, FALSE);
#endif

	/* Setup the sorter */
	ang_sort_comp = ang_sort_comp_value;
	ang_sort_swap = ang_sort_swap_value;
	/* Remember original position before sorting */
	for (i = 0; i < INVEN_TOTAL; i++) p_ptr->inventory[i].inven_order = i;
	/* Sort the player's inventory according to value */
	ang_sort(Ind, p_ptr->inventory, NULL, INVEN_TOTAL);

	/* Starting with the most valuable, drop things one by one */
	for (i = 0; i < INVEN_TOTAL; i++) {
		bool away = FALSE;

		o_ptr = &p_ptr->inventory[i];
		/* Make sure we have an object */
		if (o_ptr->k_idx == 0) continue;

		if (o_ptr->questor) { /* questor items cannot be 'dropped', only destroyed! */
			questitem_d(o_ptr, o_ptr->number);
			continue;
		}

		/* Set all his artifacts back to normal-speed timeout */
		if (was_total_winner && !cfg.fallenkings_etiquette &&
		    p_ptr->inventory[i].name1 &&
		    p_ptr->inventory[i].name1 != ART_RANDART)
			a_info[p_ptr->inventory[i].name1].winner = FALSE;

		/* If we committed suicide, only drop artifacts */
		//if (!p_ptr->alive && !artifact_p(o_ptr)) continue;
		if (!p_ptr->alive) {
			if (!true_artifact_p(o_ptr)) continue;

			/* hack -- total winners do not drop artifacts when they suicide */
			//if (!p_ptr->alive && p_ptr->total_winner && artifact_p(&p_ptr->inventory[i]))

			/* Artifacts cannot be dropped after all */
			/* Don't litter Valinor -- Ring of Phasing must be destroyed anyways */
			if ((cfg.anti_arts_hoard) || in_valinor(&p_ptr->wpos)) {
				/* set the artifact as unfound */
				handle_art_d(o_ptr->name1);

				/* Don't drop the artifact */
				continue;
			}
		}

		/* Drop/scatter an item? */
		if (!is_admin(p_ptr) && !p_ptr->inval &&
		    /* actually do drop the untradable starter items, to reduce newbie frustration */
		    (p_ptr->max_plv >= cfg.newbies_cannot_drop || !o_ptr->level)
		    /* Don't drop Morgoth's crown or Grond */
		    && !(o_ptr->name1 == ART_MORGOTH) && !(o_ptr->name1 == ART_GROND)
#ifdef IDDC_NO_TRADE_CHEEZE
		    && !(in_iddc && o_ptr->NR_tradable)
#endif
		    ) {
#ifdef VAMPIRES_INV_CURSED
			if (i >= INVEN_WIELD) reverse_cursed(o_ptr);
#endif

#ifdef DEATH_ITEM_SCATTER
			/* Apply penalty of death */
			if (!artifact_p(o_ptr) && magik(DEATH_ITEM_SCATTER))
				away = TRUE;
			else
#endif	/* DEATH_ITEM_SCATTER */
			{
				if (p_ptr->wpos.wz) o_ptr->marked2 = ITEM_REMOVAL_NEVER;
				else if (istown(&p_ptr->wpos)) o_ptr->marked2 = ITEM_REMOVAL_DEATH_WILD;/* don't litter towns for long */
				else o_ptr->marked2 = ITEM_REMOVAL_LONG_WILD;/* don't litter wilderness eternally ^^ */

				/* Drop this one */
				away = (drop_near(0, o_ptr, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px) <= 0);
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
						o_idx = drop_near(0, o_ptr, 0, &p_ptr->wpos, y1, x1);
					}
			}
		} else {
			/* set the artifact as unfound */
			if (true_artifact_p(o_ptr)) handle_art_d(o_ptr->name1);
			questitem_d(o_ptr, o_ptr->number);
		}

		/* No more item */
		invwipe(o_ptr);
	}
	/* Invalidate 'item_newest' */
	Send_item_newest(Ind, -1);

	/* Get rid of him if he's a ghost or suffers a no-ghost death */
	if ((p_ptr->ghost || (hell && p_ptr->alive)) ||
	    insanity ||
	    streq(p_ptr->died_from, "indecisiveness") ||
	    streq(p_ptr->died_from, "indetermination") ||
	    ((p_ptr->lives == 1 + 1) && cfg.lifes && p_ptr->alive &&
	    !(p_ptr->mode & MODE_EVERLASTING))) {
		/* Tell players */
		if (insanity) {
			/* Tell him */
			msg_print(Ind, "\374\377RYou die.");
			//msg_print(Ind, NULL);
//todo: use 'died_from' (insanity-blinking-style):
			msg_format(Ind, "\374\377%c**\377rYou have been destroyed by \377oI\377Gn\377bs\377Ba\377sn\377Ri\377vt\377yy\377r.\377%c**", msg_layout, msg_layout);

s_printf("CHARACTER_TERMINATION: INSANITY race=%s ; class=%s ; trait=%s ; %d deaths\n", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title, trait_info[p_ptr->ptrait].title, p_ptr->deaths);

			if (cfg.unikill_format)
			snprintf(buf, sizeof(buf), "\374\377%c**\377r%s %s (%d) was destroyed by \377m%s\377r.\377%c**", msg_layout, titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
			else
			snprintf(buf, sizeof(buf), "\374\377%c**\377r%s (%d) was destroyed by \377m%s\377r.\377%c**", msg_layout, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
			s_printf("%s - %s (%d) was destroyed by %s for %d damage at %d, %d, %d.\n", showtime(), p_ptr->name, p_ptr->lev, p_ptr->died_from, p_ptr->deathblow, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			if (!strcmp(p_ptr->died_from, "It") || !strcmp(p_ptr->died_from, "insanity") || p_ptr->image)
				s_printf("(%s was really destroyed by %s.)\n", p_ptr->name, p_ptr->really_died_from);

#if CHATTERBOX_LEVEL > 2
			if (strstr(p_ptr->died_from, "Farmer Maggot's dog") && magik(50)) {
				msg_broadcast(Ind, "Suddenly a thought comes to your mind:");
				msg_broadcast(0, "Who let the dogs out?");
			} else if (p_ptr->last_words) {
				char death_message[80];

				(void)get_rnd_line("death.txt", 0, death_message, 80);
				msg_print(Ind, death_message);
			}
#endif	// CHATTERBOX_LEVEL

			death_type = DEATH_INSANITY;
			if (p_ptr->ghost) {
				death_type = DEATH_GHOST;
				Send_chardump(Ind, "-ghost");
			} else Send_chardump(Ind, "-death");
			Net_output1(Ind);
		} else if (p_ptr->ghost) {
			/* Tell him */
			msg_format(Ind, "\374\377a**\377rYour ghost was destroyed by %s.\377a**", p_ptr->died_from);

s_printf("CHARACTER_TERMINATION: GHOSTKILL race=%s ; class=%s ; trait=%s ; %d deaths\n", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title, trait_info[p_ptr->ptrait].title, p_ptr->deaths);

			if (cfg.unikill_format) {
				switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
				case 's': case 'x': case 'z':
					snprintf(buf, sizeof(buf), "\374\377a**\377r%s %s' (%d) ghost was destroyed by %s.\377a**", titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from);
					break;
				default:
					snprintf(buf, sizeof(buf), "\374\377a**\377r%s %s's (%d) ghost was destroyed by %s.\377a**", titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from);
				}
			} else {
				switch (p_ptr->name[strlen(p_ptr->name) - 1]) {
				case 's': case 'x': case 'z':
					snprintf(buf, sizeof(buf), "\374\377a**\377r%s' (%d) ghost was destroyed by %s.\377a**", p_ptr->name, p_ptr->lev, p_ptr->died_from);
					break;
				default:
					snprintf(buf, sizeof(buf), "\374\377a**\377r%s's (%d) ghost was destroyed by %s.\377a**", p_ptr->name, p_ptr->lev, p_ptr->died_from);
				}
			}
			s_printf("%s - %s's (%d) ghost was destroyed by %s for %d damage on %d, %d, %d.\n", showtime(), p_ptr->name, p_ptr->lev, p_ptr->died_from, p_ptr->deathblow, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			if (!strcmp(p_ptr->died_from, "It") || !strcmp(p_ptr->died_from, "insanity") || p_ptr->image)
				s_printf("(%s's ghost was really destroyed by %s.)\n", p_ptr->name, p_ptr->really_died_from);

#if CHATTERBOX_LEVEL > 2
			if (p_ptr->last_words) {
				char death_message[80];

				(void)get_rnd_line("death.txt", 0, death_message, 80);
				msg_print(Ind, death_message);
			}
#endif	// CHATTERBOX_LEVEL

			death_type = DEATH_GHOST;
		} else {
			/* Tell him */
			msg_print(Ind, "\374\377RYou die.");
			//msg_print(Ind, NULL);
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
#ifdef ENABLE_MAIA
				    || streq(p_ptr->died_from, "indecisiveness")
#endif
				    || streq(p_ptr->died_from, "indetermination")
				    || streq(p_ptr->died_from, "starvation")
				    || streq(p_ptr->died_from, "poisonous food")
				    || insanity) {
					msg_format(Ind, "\374\377%c**\377rYou have been killed by %s.\377%c**", msg_layout, p_ptr->died_from, msg_layout);
				} else if ((p_ptr->deathblow < 30) || ((p_ptr->deathblow < p_ptr->mhp / 2) && (p_ptr->deathblow < 450))) {
					msg_format(Ind, "\374\377%c**\377rYou have been annihilated by %s.\377%c**", msg_layout, p_ptr->died_from, msg_layout);
				} else {
					msg_format(Ind, "\374\377%c**\377rYou have been vaporized by %s.\377%c**", msg_layout, p_ptr->died_from, msg_layout);
				}

				if (cfg.unikill_format) {
					if ((p_ptr->deathblow < 10) || ((p_ptr->deathblow < p_ptr->mhp / 4) && (p_ptr->deathblow < 100))
#ifdef ENABLE_MAIA
					    || streq(p_ptr->died_from, "indecisiveness")
#endif
					    || streq(p_ptr->died_from, "indetermination")
					    || streq(p_ptr->died_from, "starvation")
					    || streq(p_ptr->died_from, "poisonous food")
					    || insanity)
						snprintf(buf, sizeof(buf), "\374\377%c**\377r%s %s (%d) was killed by %s.\377%c**", msg_layout, titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
					else if ((p_ptr->deathblow < 30) || ((p_ptr->deathblow < p_ptr->mhp / 2) && (p_ptr->deathblow < 450)))
						snprintf(buf, sizeof(buf), "\374\377%c**\377r%s %s (%d) was annihilated by %s.\377%c**", msg_layout, titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
					else
						snprintf(buf, sizeof(buf), "\374\377%c**\377r%s %s (%d) was vaporized by %s.\377%c**", msg_layout, titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
				} else {
					if ((p_ptr->deathblow < 10) || ((p_ptr->deathblow < p_ptr->mhp / 4) && (p_ptr->deathblow < 100))
#ifdef ENABLE_MAIA
					    || streq(p_ptr->died_from, "indecisiveness")
#endif
					    || streq(p_ptr->died_from, "indetermination")
					    || streq(p_ptr->died_from, "starvation")
					    || streq(p_ptr->died_from, "poisonous food")
					    || insanity)
						snprintf(buf, sizeof(buf), "\374\377%c**\377r%s (%d) was killed and destroyed by %s.\377%c**", msg_layout, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
					else if ((p_ptr->deathblow < 30) || ((p_ptr->deathblow < p_ptr->mhp / 2) && (p_ptr->deathblow < 450)))
						snprintf(buf, sizeof(buf), "\374\377%c**\377r%s (%d) was annihilated and destroyed by %s.\377%c**", msg_layout, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
					else
						snprintf(buf, sizeof(buf), "\374\377%c**\377r%s (%d) was vaporized and destroyed by %s.\377%c**", msg_layout, p_ptr->name, p_ptr->lev, p_ptr->died_from, msg_layout);
				}
#ifdef MORGOTH_FUNKY_KILL_MSGS
			}
#endif
			s_printf("%s - %s (%d) was killed and destroyed by %s for %d damage at %d, %d, %d.\n", showtime(), p_ptr->name, p_ptr->lev, p_ptr->died_from, p_ptr->deathblow, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
			if (!strcmp(p_ptr->died_from, "It") || !strcmp(p_ptr->died_from, "insanity") || p_ptr->image)
				s_printf("(%s was really killed and destroyed by %s.)\n", p_ptr->name, p_ptr->really_died_from);

s_printf("CHARACTER_TERMINATION: %s race=%s ; class=%s ; trait=%s ; %d deaths\n", pvp ? "PVP" : "NOGHOST", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title, trait_info[p_ptr->ptrait].title, p_ptr->deaths);

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

		if (is_admin(p_ptr)) snprintf(buf, sizeof(buf), "\376\377D%s bids farewell to this plane.", p_ptr->name);

		if ((!p_ptr->admin_dm) || (!cfg.secret_dungeon_master)){
#ifdef TOMENET_WORLDS
			if (cfg.worldd_pdeath) world_msg(buf);
#endif
			msg_broadcast(Ind, buf);
		}

		/* Reward the killer if it was a PvP-mode char */
		if (pvp) {
			int killer = name_lookup(Ind, p_ptr->really_died_from, FALSE, FALSE, TRUE);

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
//wrong code: used for rewards	Players[killer]->kills += p_ptr->kills;
#if 0 /* disabled since it doesn't make much sense */
				Players[killer]->kills_lower += p_ptr->kills_lower;
				Players[killer]->kills_equal += p_ptr->kills_equal;
				Players[killer]->kills_higher += p_ptr->kills_higher;
#endif
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
#if 0 /* wrong code (rewards) */
						Players[i]->kills += avg_kills;
#else
						if (avg_kills) Players[i]->kills++;
#endif
						check_killing_reward(i);
					}
				}
			}
		} else { /* wasn't a pvp-mode death */
			/* copy/paste from above pvp section, just for info */
			int killer = name_lookup(Ind, p_ptr->really_died_from, FALSE, FALSE, TRUE);
			if (killer) {
				if (Players[killer]->max_plv > p_ptr->max_plv) Players[killer]->kills_lower++;
				else if (Players[killer]->max_plv < p_ptr->max_plv) Players[killer]->kills_higher++;
				else Players[killer]->kills_equal++;
				//wrong code (rewards)	Players[killer]->kills += p_ptr->kills;
#if 0 /* not much sense */
				Players[killer]->kills_lower += p_ptr->kills_lower;
				Players[killer]->kills_equal += p_ptr->kills_equal;
				Players[killer]->kills_higher += p_ptr->kills_higher;
#endif
				Players[killer]->kills++;
				Players[killer]->kills_own++;

				/* no reward outside of PvP xD */
			}
		}

		/* Add to legends log if he was at least 50 or died vs Morgoth */
		if (!is_admin(p_ptr)) {
			if (p_ptr->total_winner)
				l_printf("%s \\{r%s royalty %s (%d) died\n", showdate(), p_ptr->male ? "His" : "Her", p_ptr->name, p_ptr->lev);
			else if (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_GHOST))
				l_printf("%s \\{r%s (%d) died facing Morgoth\n", showdate(), p_ptr->name, p_ptr->lev);
#ifndef RPG_SERVER
			/* for non-ghost deaths, display somewhat "lower" levels (below 50) too */
			else if (p_ptr->lev >= 40)
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
	if (p_ptr->total_winner && !is_admin(p_ptr) && p_ptr->alive) {
		l_printf("%s \\{r%s (%d) lost %s royal title by death\n", showdate(), p_ptr->name, p_ptr->lev, p_ptr->male ? "his" : "her");
#ifdef SOLO_REKING
		p_ptr->solo_reking = p_ptr->solo_reking_au = SOLO_REKING;
#endif
	}

	/* Tell everyone he died */
	if (p_ptr->alive) {
		if (cfg.unikill_format) {
			if ((p_ptr->deathblow < 10) || ((p_ptr->deathblow < p_ptr->mhp / 4) && (p_ptr->deathblow < 100))
#ifdef ENABLE_MAIA
			    || streq(p_ptr->died_from, "indecisiveness")
#endif
			    || streq(p_ptr->died_from, "indetermination")
			    || streq(p_ptr->died_from, "starvation")
			    || streq(p_ptr->died_from, "poisonous food")
			    || insanity) {
				/* snprintf(buf, sizeof(buf), "\374\377r%s was killed by %s.", p_ptr->name, p_ptr->died_from); */
				/* Add the player lvl to the death message. the_sandman */
				snprintf(buf, sizeof(buf), "\374\377r%s %s (%d) was killed by %s", titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from);
			}
			else if ((p_ptr->deathblow < 30) || ((p_ptr->deathblow < p_ptr->mhp / 2) && (p_ptr->deathblow < 450))) {
				/* snprintf(buf, sizeof(buf), "\377r%s was annihilated by %s.", p_ptr->name, p_ptr->died_from); */
				snprintf(buf, sizeof(buf), "\374\377r%s %s (%d) was annihilated by %s", titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from);
			}
			else {
				snprintf(buf, sizeof(buf), "\374\377r%s %s (%d) was vaporized by %s.", titlebuf, p_ptr->name, p_ptr->lev, p_ptr->died_from);
			}
		} else {
			if ((p_ptr->deathblow < 10) || ((p_ptr->deathblow < p_ptr->mhp / 4) && (p_ptr->deathblow < 100))
#ifdef ENABLE_MAIA
			    || streq(p_ptr->died_from, "indecisiveness")
#endif
			    || streq(p_ptr->died_from, "indetermination")
			    || streq(p_ptr->died_from, "starvation")
			    || streq(p_ptr->died_from, "poisonous food")
			    || insanity) {
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
		}
		s_printf("%s - %s (%d) was killed by %s for %d damage at %d, %d, %d.\n", showtime(), p_ptr->name, p_ptr->lev, p_ptr->died_from, p_ptr->deathblow, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
		if (!strcmp(p_ptr->died_from, "It") || !strcmp(p_ptr->died_from, "insanity") || p_ptr->image)
			s_printf("(%s was really killed by %s.)\n", p_ptr->name, p_ptr->really_died_from);

s_printf("CHARACTER_TERMINATION: NORMAL race=%s ; class=%s ; trait=%s ; %d deaths\n", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title, trait_info[p_ptr->ptrait].title, p_ptr->deaths);
	}
	else if (p_ptr->iron_winner) {
		if (p_ptr->total_winner) {
			snprintf(buf, sizeof(buf), "\374\377vThe iron emperor %s has retired to a warm, sunny climate.", p_ptr->name);
			if (!is_admin(p_ptr)) l_printf("%s \\{v%s (%d) retired from the iron throne\n", showdate(), p_ptr->name, p_ptr->lev);
		} else {
			snprintf(buf, sizeof(buf), "\374\377sThe iron champion %s has retired to a warm, sunny climate.", p_ptr->name);
			if (!is_admin(p_ptr)) l_printf("%s \\{s%s (%d) retired as an iron champion\n", showdate(), p_ptr->name, p_ptr->lev);
		}
		s_printf("%s - %s (%d) retired to a warm, sunny climate.\n", showtime(), p_ptr->name, p_ptr->lev);
		retire = TRUE;
		death_type = DEATH_QUIT;
s_printf("CHARACTER_TERMINATION: RETIREMENT race=%s ; class=%s ; trait=%s ; %d deaths\n", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title, trait_info[p_ptr->ptrait].title, p_ptr->deaths);
	}
	else if (!p_ptr->total_winner) {
		/* assume newb_suicide option for world broadcasts */
		if (p_ptr->max_plv == 1) world_broadcast = FALSE;

		snprintf(buf, sizeof(buf), "\374\377D%s committed suicide.", p_ptr->name);
		s_printf("%s - %s (%d) committed suicide.\n", showtime(), p_ptr->name, p_ptr->lev);
		death_type = DEATH_QUIT;
s_printf("CHARACTER_TERMINATION: SUICIDE race=%s ; class=%s ; trait=%s ; %d deaths\n", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title, trait_info[p_ptr->ptrait].title, p_ptr->deaths);
	} else {
		if (in_valinor(&p_ptr->wpos)) {
			snprintf(buf, sizeof(buf), "\374\377vThe unbeatable %s has retired to the shores of valinor.", p_ptr->name);
			if (!is_admin(p_ptr)) l_printf("%s \\{v%s (%d) retired to the shores of valinor\n", showdate(), p_ptr->name, p_ptr->lev);
		} else {
			snprintf(buf, sizeof(buf), "\374\377vThe unbeatable %s has retired to a warm, sunny climate.", p_ptr->name);
			if (!is_admin(p_ptr)) l_printf("%s \\{v%s (%d) retired to a warm, sunny climate\n", showdate(), p_ptr->name, p_ptr->lev);
		}
		s_printf("%s - %s (%d) retired to a warm, sunny climate.\n", showtime(), p_ptr->name, p_ptr->lev);
		retire = TRUE;
		death_type = DEATH_QUIT;
s_printf("CHARACTER_TERMINATION: RETIREMENT race=%s ; class=%s ; trait=%s ; %d deaths\n", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title, trait_info[p_ptr->ptrait].title, p_ptr->deaths);
	}

	if (is_admin(p_ptr)) {
		if (death_type == -1) snprintf(buf, sizeof(buf), "\376\377D%s enters a ghostly state.", p_ptr->name);
		else snprintf(buf, sizeof(buf), "\376\377D%s bids farewell to this plane.", p_ptr->name);
	}

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
					msg_format(i, "%c%s", '\376', buf + 1);
				else
					msg_print(i, buf);
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

	/* No longer a winner */
	p_ptr->total_winner = FALSE;

	/* Handle suicide */
	if (!p_ptr->alive) {
#ifdef TOMENET_WORLDS
		world_player(p_ptr->id, p_ptr->name, FALSE, TRUE);
#endif

		/* prevent suicide spam, if set in cfg */
		check_roller(Ind);

		if (!p_ptr->ghost) {
			if (retire) Send_chardump(Ind, "-retirement");
			else Send_chardump(Ind, "-death");
			Net_output1(Ind);
		}

		erase_player(Ind, death_type, FALSE);

		/* Done */
		return;
	}

	/* Tell him */
	msg_print(Ind, "\374\377RYou die.");
	//msg_print(Ind, NULL);
#if CHATTERBOX_LEVEL > 2
	if (p_ptr->last_words) {
		char death_message[80];

		(void)get_rnd_line("death.txt", 0, death_message, 80);
		msg_print(Ind, death_message);
	}
#endif	// CHATTERBOX_LEVEL

	/* Paranoia - ghosts getting destroyed are already caught above */
	if (p_ptr->ghost) Send_chardump(Ind, "-ghost"); else
	Send_chardump(Ind, "-death");

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

	if ((p_ptr->deathblow < 10) || ((p_ptr->deathblow < p_ptr->mhp / 4) && (p_ptr->deathblow < 100))
#ifdef ENABLE_MAIA
	    || streq(p_ptr->died_from, "indecisiveness")
#endif
	    || streq(p_ptr->died_from, "indetermination")
	    || streq(p_ptr->died_from, "starvation")
	    || streq(p_ptr->died_from, "poisonous food")
	    || insanity)
		msg_format(Ind, "\374\377RYou have been killed by %s.", p_ptr->died_from);
	else if ((p_ptr->deathblow < 30) || ((p_ptr->deathblow < p_ptr->mhp / 2) && (p_ptr->deathblow < 450)))
		msg_format(Ind, "\374\377RYou have been annihilated by %s.", p_ptr->died_from);
	else msg_format(Ind, "\374\377RYou have been vaporized by %s.", p_ptr->died_from);

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

	/* Turn him into a ghost */
	p_ptr->ghost = 1;
	/* Prevent accidental floating up/downwards depending on client option. - C. Blue */
	if (p_ptr->safe_float) p_ptr->safe_float_turns = 5;

	/* Hack -- drop bones :) */
#ifdef ENABLE_MAIA
	/* hackhack: Maiar don't have a real physical body ;) */
	if (p_ptr->prace != RACE_MAIA)
#endif
	for (i = 0; i < 4; i++) {
		object_type	forge;
		o_ptr = &forge;

		invcopy(o_ptr, lookup_kind(TV_SKELETON, i ? SV_BROKEN_BONE : SV_BROKEN_SKULL));
		object_known(o_ptr);
		object_aware(Ind, o_ptr);
		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
		o_ptr->iron_trade = p_ptr->iron_trade;
		o_ptr->level = 0;
		o_ptr->note = quark_add(format("# of %s", p_ptr->name));
		/* o_ptr->note = quark_add(format("#of %s", p_ptr->name));
		the_sandman: removed the auto-space-padding on {# inscs */

		if (p_ptr->wpos.wz) o_ptr->marked2 = ITEM_REMOVAL_NEVER;
		else if (istown(&p_ptr->wpos)) o_ptr->marked2 = ITEM_REMOVAL_DEATH_WILD;/* don't litter towns for long */
		else o_ptr->marked2 = ITEM_REMOVAL_LONG_WILD;/* don't litter wilderness eternally ^^ */
		(void)drop_near(0, o_ptr, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);
	}

	/* Give him his hit points back */
	p_ptr->chp = p_ptr->mhp;
	p_ptr->chp_frac = 0;

	/* Teleport him */
	/* XXX p_ptr->death allows teleportation even when NO_TELE etc. */
	teleport_player(Ind, 200, TRUE);

	/* Hack -- Give him/her the newbie death guide */
	//if (p_ptr->max_plv < 20)	/* Now it's for everyone */
	{
		object_type	forge;
		o_ptr = &forge;

		invcopy(o_ptr, lookup_kind(TV_PARCHMENT, SV_PARCHMENT_DEATH));
		object_known(o_ptr);
		object_aware(Ind, o_ptr);
		o_ptr->owner = p_ptr->id;
		o_ptr->mode = p_ptr->mode;
		o_ptr->iron_trade = p_ptr->iron_trade;
		o_ptr->level = 1;
		(void)inven_carry(Ind, o_ptr);
	}

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

#if 1 /* Enable, iff newbies-level leading to perma-death is disabled above. */
	if (p_ptr->max_plv < cfg.newbies_cannot_drop) {
		msg_format(Ind, "\374\377oYou died below level %d, which means that most items didn't drop.", cfg.newbies_cannot_drop);
		msg_print(Ind, "\374\377oTherefore, it's recommended to press '\377RQ\377o' to suicide and start over.");
		if (p_ptr->wpos.wz < 0) msg_print(Ind, "\374\377oIf you don't like to do that, use '\377R<\377o' to float back to town,");
		else if (p_ptr->wpos.wz > 0) msg_print(Ind, "\374\377oIf you don't like to do that, use '\377R>\377o' to float back to town,");
		else if (in_bree(&p_ptr->wpos)) msg_print(Ind, "\374\377oIf you don't like to do that, of course you may just continue");
		else msg_print(Ind, "\374\377oIf you don't like to do that, just continue by flying back to town");
		msg_print(Ind, "\374\377oand enter the temple (\377g4\377o) to be revived and handed some money.");
	} else
#endif
	if (!p_ptr->warning_death) {
		/* normal warning - in dungeon */
		if (p_ptr->wpos.wz) {
			p_ptr->warning_death = 1;
			msg_print(Ind, "\374\377yIf you leave this floor and nobody else stays on it, it will change and");
			msg_print(Ind, "\374\377ytherefore all your items would be lost! You now have two choices:");
			if (p_ptr->wpos.wz > 0)
				msg_print(Ind, "\374\377ya) Float back to town with '\377o>\377y' key and revive yourself in the temple ('\377g4\377y').");
			else
				msg_print(Ind, "\374\377ya) Float back to town with '\377o<\377y' key and revive yourself in the temple ('\377g4\377y').");
			msg_print(Ind, "\374\377yb) Stay here as a ghost and ask someone else to come and revive you.");
		} else {
		/* specialty - on worldmap */
			if (in_bree(&p_ptr->wpos))
				msg_print(Ind, "\374\377yRevive yourself by floating over to the temple ('\377g4\377y') and entering it.");
			else {
				msg_print(Ind, "\374\377yYou now have two choices:");
				msg_print(Ind, "\374\377ya) Float back to town and revive yourself by entering the temple ('\377g4\377y').");
				msg_print(Ind, "\374\377yb) Stay here as a ghost and ask someone else to come and revive you.");
			}
		}
	}

#if 0 /* currently disabled, because replaced by warning_death */
	/* Possibly tell him what to do now */
	if (p_ptr->warning_ghost == 0) {
		p_ptr->warning_ghost = 1;
		msg_print(Ind, "\375\377RHINT: You died! You can wait for someone to revive you or use \377o<\377R or \377o>");
		msg_print(Ind, "\375\377R      keys to float back to town and revive yourself in the temple (the \377g4\377R).");
		msg_print(Ind, "\375\377R      If you wish to start over, press \377oSHIFT+q\377R to erase this character.");
		s_printf("warning_ghost: %s\n", p_ptr->name);
	}
#endif
}

/*
 * Resurrect a player
 */

 /* To prevent people from ressurecting too many times, I am modifying this to give
    everyone 1 "freebie", and then to have a p_ptr->level % chance of failing to
    ressurect and have your ghost be destroyed.

    -APD-

    hmm, haven't gotten aroudn to doing this yet...

    loss_reduction tells by how much % the GHOST_XP_LOST is reduced (C. Blue).
 */
void resurrect_player(int Ind, int loss_factor) {
	player_type *p_ptr = Players[Ind];
	int reduce;
	bool has_exp = p_ptr->max_exp != 0;

	/* Hack -- the dungeon master can not resurrect */
	if (p_ptr->admin_dm) return;	// TRUE;

	/* Reset ghost flag */
	p_ptr->ghost = 0;

	disturb(Ind, 1, 0);

	/* limits - hack: '0' means use default value */
	if (!loss_factor) loss_factor = GHOST_XP_LOST;
	/* paranoia */
	else if (loss_factor < 0) loss_factor = GHOST_XP_LOST;
	else if (loss_factor > 100) loss_factor = GHOST_XP_LOST;

	/* Lose some experience */
	if (get_skill(p_ptr, SKILL_HCURING) >= 50
#ifdef ENABLE_OCCULT /* Occult */
	    || get_skill(p_ptr, SKILL_OSPIRIT) >= 50
#endif
	    ) loss_factor -= 5;
	if (loss_factor < 30) loss_factor = 30;//hardcoded mess

	reduce = p_ptr->max_exp;
	reduce = reduce > 99999 ?
	    reduce / 100 * loss_factor : reduce * loss_factor / 100;
	p_ptr->max_exp -= reduce;

	reduce = p_ptr->exp;
	reduce = reduce > 99999 ?
	    reduce / 100 * loss_factor : reduce * loss_factor / 100;
	p_ptr->exp -= reduce;

	/* Prevent cheezing exp to 0 to become eligible for certain events */
	if (!p_ptr->max_exp && has_exp) p_ptr->exp = p_ptr->max_exp = 1;

	p_ptr->safe_sane = TRUE;
	check_experience(Ind);
	p_ptr->update |= PU_SANITY;
	update_stuff(Ind);
	p_ptr->safe_sane = FALSE;

	/* Message */
	msg_print(Ind, "\376\377GYou feel life force return to your body!");
	msg_format_near(Ind, "%s is resurrected!", p_ptr->name);
	everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);

	/* (was in player_death: Take care of ghost suiciding before final resurrection (p_ptr->alive check, C. Blue)) */
	/*if (p_ptr->alive && ((p_ptr->lives > 0+1) && cfg.lifes)) p_ptr->lives--;*/
	/* Tell him his remaining lifes */
	if (!(p_ptr->mode & MODE_EVERLASTING)
	    && !(p_ptr->mode & MODE_PVP)) {
		if (p_ptr->lives > 1+1) p_ptr->lives--;
		if (cfg.lifes) {
			if (p_ptr->lives == 1+1)
				msg_print(Ind, "\376\377GYou have no more resurrections left!");
			else
				msg_format(Ind, "\376\377GYou have %d resurrections left.", p_ptr->lives-1-1);
		}
	}

	/* Bonus service: Also restore drained exp (for newbies, especially) */
	restore_level(Ind);

	/* Redraw */
	p_ptr->redraw |= (PR_BASIC);

	/* Update */
	p_ptr->update |= (PU_BONUS);

	/* Inform him of instant resurrection option */
	if (p_ptr->warning_instares == 0) {
		p_ptr->warning_instares = 1;
		msg_print(Ind, "\375\377yHINT: You can turn on \377oInstant Resurrection\377y in the temple by pressing '\377or\377y'.");
		msg_print(Ind, "\375\377y      Make sure to read up on it in the \377oguide\377y to understand pros and cons!");
		s_printf("warning_instares: %s\n", p_ptr->name);
	}

	/* Hack -- jail her/him */
	if (!p_ptr->wpos.wz && p_ptr->tim_susp
#ifdef JAIL_TOWN_AREA
	    && istownarea(&p_ptr->wpos, MAX_TOWNAREA)
#endif
	    )
		imprison(Ind, JAIL_OLD_CRIMES, "old crimes");
}

void check_xorders(){
	int i, j;
	struct player_type *q_ptr;
	for (i = 0; i < MAX_XORDERS; i++) {
		if (xorders[i].active && xorders[i].id) {
			if ((turn - xorders[i].turn) > MAX_XORDER_TURNS) {
				for (j = 1; j <= NumPlayers; j++) {
					q_ptr = Players[j];
					if (q_ptr && q_ptr->xorder_id == xorders[i].id) {
						msg_print(j, "\376\377oYou have failed your extermination order!");
						q_ptr->xorder_id = 0;
						q_ptr->xorder_num = 0;
					}
				}
				xorders[i].active = 0;
				xorders[i].id = 0;
				xorders[i].type = 0;
			}
		}
	}
}

void del_xorder(int id){
	int i;
	for (i = 0; i < MAX_XORDERS; i++) {
		if (xorders[i].id == id) {
			s_printf("Extermination order %d removed\n", id);
			xorders[i].active = 0;
			xorders[i].id = 0;
			xorders[i].type = 0;
		}
	}
}

/* One player leave a quest (death, deletion) */
void rem_xorder(u16b id){
	int i;

	s_printf("Player death. Extermination order id: %d\n", id);

	if (!id) return;

	for (i = 0; i < MAX_XORDERS; i++) {
		if (xorders[i].id == id) {
			break;
		}
	}
	if (i == MAX_XORDERS) return;
	s_printf("Extermination order found in slot %d\n",i);
	if (xorders[i].active) {
		xorders[i].active--;
		s_printf("Remaining active: %d\n", xorders[i].active);
		if (!xorders[i].active) {
			process_hooks(HOOK_QUEST_FAIL, "d", id);
			s_printf("delete call\n");
			del_xorder(id);
		}
	}
}

void kill_xorder(int Ind) {
	int i;
	u16b id, pos = 9999;
	player_type *p_ptr = Players[Ind], *q_ptr;
	char temp[160];
	bool great, verygreat = FALSE;
	u32b resf;

	id = p_ptr->xorder_id;
	for (i = 0; i < MAX_XORDERS; i++) {
		if (xorders[i].id == id) {
			pos = i;
			break;
		}
	}
	//if (pos == -1) return;	/* it's UNsigned :) */
	if (pos == 9999) return;

	process_hooks(HOOK_QUEST_FINISH, "d", Ind);

	if (xorders[i].flags & QUEST_RACE) {
		snprintf(temp, 160, "\374\377y%s has carried out a%s %s extermination order!",
		    p_ptr->name,
		    is_a_vowel(*(r_name + r_info[xorders[pos].type].name)) ? "n" : "",
		    r_name + r_info[xorders[pos].type].name);
		msg_broadcast(Ind, temp);
	}
	if (xorders[i].flags & QUEST_GUILD) {
		hash_entry *temphash;
		snprintf(temp, 160, "\374\377y%s has carried out the %s extermination order!", p_ptr->name, r_name + r_info[xorders[pos].type].name);
		if ((temphash = lookup_player(xorders[i].creator)) && temphash->guild) {
			guild_msg(temphash->guild ,temp);
			if (!p_ptr->guild) {
				guild_msg_format(temphash->guild, "\374\377%c%s is now a guild member!", COLOUR_CHAT_GUILD, p_ptr->name);
				guilds[temphash->guild].members++;
				msg_format(Ind, "\374\377yYou've been added to '\377%c%s\377w'.", COLOUR_CHAT_GUILD, guilds[temphash->guild].name);
				p_ptr->guild = temphash->guild;
				p_ptr->guild_dna = guilds[p_ptr->guild].dna;
				clockin(Ind, 3);	/* set in db */
			}
			else if (p_ptr->guild == temphash->guild) {
				guild_msg_format(temphash->guild, "\374\377%c%s has carried out the extermination order!", COLOUR_CHAT_GUILD, p_ptr->name);
			}
		}
	} else {
		object_type forge, *o_ptr = &forge;
		int avg, rlev = r_info[xorders[pos].type].level, plev = p_ptr->lev;

		msg_format(Ind, "\374\377yYou have carried out the %s extermination order!", r_name + r_info[xorders[pos].type].name);
		s_printf("r_info quest: %s won the %s extermination order (%d,%d,%d)\n", p_ptr->name, r_name + r_info[xorders[pos].type].name, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
		strcpy(temp, r_name + r_info[xorders[pos].type].name);
		strcat(temp, " extermination");
		unique_quark = quark_add(temp);

		/* grant verygreat rerolls for better value? */
		avg = ((rlev * 2) + (plev * 4)) / 2;
		avg = avg > 100 ? 100 : avg;
		//if (great && p_ptr->lev >= 25) verygreat = magik(r_info[xorders[pos].type].level - (5 - (p_ptr->lev / 5)));
		//if (great) verygreat = magik(((r_info[xorders[pos].type].level * 2) + (p_ptr->lev * 4)) / 5);
		avg /= 2; avg = 540 / (57 - avg) + 5; /* same as exp calculation ;) phew, Heureka.. (14..75) */

#if 0
		/* boost quest rewards for the iron price */
 #ifndef RPG_SERVER
		if (in_irondeepdive(&p_ptr->wpos)) {
 #endif
			great = TRUE;
			verygreat = magik(avg);
			resf = RESF_LOW2;
 #ifndef RPG_SERVER
		} else {
			great = magik(50 + (plev > rlev ? rlev : plev) * 2);
			if (great) verygreat = magik(avg);
			resf = RESF_LOW;
		}
 #endif
#else
		/* extermination orders have been buffed for IDDC by being based on floor level instead of player level,
		   buffing the quality on top of this would be too much. */
		great = magik(50 + (plev > rlev ? rlev : plev) * 2);
		if (great) verygreat = magik(avg);
		resf = RESF_LOW;
#endif

#if 0 /* needs more care, otherwise acquirement could even be BETTER than create_reward, depending on RESF.. */
		create_reward(Ind, o_ptr, getlevel(&p_ptr->wpos), getlevel(&p_ptr->wpos), great, verygreat, resf, 3000);
		if (!o_ptr->note) o_ptr->note = quark_add(temp);
		o_ptr->note_utag = strlen(temp);
		o_ptr->iron_trade = p_ptr->iron_trade;
		inven_carry(Ind, o_ptr);
#else
		acquirement_direct(o_ptr, &p_ptr->wpos, great, verygreat, resf);
		//s_printf("object rewarded %d,%d,%d\n", o_ptr->tval, o_ptr->sval, o_ptr->k_idx);
		o_ptr->iron_trade = p_ptr->iron_trade;
		inven_carry(Ind, o_ptr);
#endif
		unique_quark = 0;
	}

	for (i = 1; i <= NumPlayers; i++) {
		q_ptr = Players[i];
		if (q_ptr && q_ptr->xorder_id == id) {
			q_ptr->xorder_id = 0;
			q_ptr->xorder_num = 0;
		}
	}
	del_xorder(id);
}

s16b questid = 1;

bool add_xorder(int Ind, int target, u16b type, u16b num, u16b flags) {
	int i, j;
	bool added = FALSE;
	player_type *p_ptr = Players[target], *q_ptr;
	if (!p_ptr) return(FALSE);

	process_hooks(HOOK_GEN_QUEST, "d", Ind);

	for (i = 0; i < MAX_XORDERS; i++) {
		if (!xorders[i].active) {
			xorders[i].active = 0;
			xorders[i].id = questid;
			xorders[i].type = type;
			xorders[i].flags = flags;
			xorders[i].turn = turn;
			added = TRUE;
			break;
		}
	}
	if (!added) {
		msg_print(Ind, "Sorry, no more extermination orders are available at this time.");
		return(FALSE);
	}
	added = 0;

	/* give it only to the one original target player */
	j = target;
	q_ptr = Players[j];
#ifndef RPG_SERVER
	if (q_ptr->lev < 5 && !in_irondeepdive(&q_ptr->wpos)) return(FALSE); /* level 5 is minimum to do quests */
#else
	if (q_ptr->lev < 3) return(FALSE);
#endif
	q_ptr->xorder_id = questid;
	q_ptr->xorder_num = num;
	clockin(j, 4); /* register that player */
	if ((flags & QUEST_GUILD))
		msg_print(j, "\374\377oYou have been given an extermination order from your guild\377y!");
	else
		msg_print(j, "\376\377oYou have been given a extermination order\377y!");
	//msg_format(j, "\377oFind and kill \377y%d \377g%s%s\377y!", num, r_name+r_info[type].name, flags&QUEST_GUILD?"":" \377obefore any other player");
	msg_format(j, "\376\377oFind and kill \377y%d \377g%s\377o (level %d)!", num, r_name + r_info[type].name, r_info[type].level);
	msg_format(Ind, "\376\377oThe remaining time to carry it out is \377y%d\377o minutes.", MAX_XORDER_TURNS / (cfg.fps * 60));
	xorders[i].active++;

//???
	if (!xorders[i].active) {
		del_xorder(questid);
		return(FALSE);
	}

	s_printf("Added extermination order id %d (players %d), target %d (%s): %d x %s (%d,%d,%d)\n",
	    xorders[i].id, xorders[i].active, target, p_ptr != NULL ? p_ptr->name : "NULL",
	    num, r_name + r_info[type].name,
	    p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
	questid++;
	if (questid == 0) questid = 1;
	if (target != Ind) {
		if (flags & QUEST_GUILD) {
			guild_msg_format(Players[Ind]->guild, "\374\377%c%s has been given an extermination order!", COLOUR_CHAT_GUILD, p_ptr->name);
		}
		else msg_format(Ind, "Extermination order given to %s", p_ptr->name);
		xorders[i].creator = Players[Ind]->id;
	}

	if (in_irondeepdive(&p_ptr->wpos)) p_ptr->IDDC_flags |= 0xC;
	return(TRUE);
}

/* prepare some quest parameters for a standard kill quest */
bool prepare_xorder(int Ind, int j, u16b flags, int *level, u16b *type, u16b *num){
	int r = *type, i = *num, lev = *level, k = 0;
	player_type *p_ptr = Players[j];
	bool iddc = in_irondeepdive(&p_ptr->wpos), mandos = in_hallsofmandos(&p_ptr->wpos);

	if (p_ptr->xorder_id) {
		for (i = 0; i < MAX_XORDERS; i++) {
			if (xorders[i].id == p_ptr->xorder_id) {
				if (j == Ind) {
					msg_format(Ind, "\376\377oYour %sorder is to exterminate \377y%d \377g%s\377o (level %d).",
					    (xorders[i].flags & QUEST_GUILD) ? "guild's " : "", p_ptr->xorder_num,
					    r_name + r_info[xorders[i].type].name, r_info[xorders[i].type].level);
					msg_format(Ind, "\376\377oThe remaining time to carry it out is \377y%d\377o minutes.", (MAX_XORDER_TURNS - (turn - xorders[i].turn)) / (cfg.fps * 60));
				}
				return FALSE;
			}
		}
	}

	if (!iddc && !mandos) {
		if (p_ptr->mode & MODE_DED_IDDC) {
			msg_print(Ind, "\377yYou can only acquire an extermination order when you are inside the IDDC!");
			msg_print(Ind, "\377yUse the /xo (short for /xorder) command after you entered it.");
			return FALSE;
		}

		switch (p_ptr->store_num) {
		case STORE_MAYOR:
		case STORE_CASTLE:
		case STORE_SEATRULING:
		case STORE_KINGTOWER:
		case STORE_SEADOME:
			break;
		default:
			msg_print(Ind, "\377yPlease visit your local town hall or seat of ruling to receive an order!");
			return FALSE;
		}
	}

	if (p_ptr->IDDC_logscum) {
		msg_print(Ind, "\377yYou cannot acquire an extermination order on a stale floor.");
		msg_print(Ind, "\377yTake a staircase to move on to the next dungeon level.");
		return FALSE;
	}
	if (p_ptr->IDDC_flags & 0xC) {
		msg_print(Ind, "\377yYou cannot acquire an extermination order on this floor.");
		//msg_print(Ind, "\377yTake a staircase to move on to the next dungeon level.");
		return FALSE;
	}

	/* don't start too early -C. Blue */
#ifndef RPG_SERVER
	if (p_ptr->lev < 5 && !iddc) {
		msg_print(Ind, "\377oYou need to be level 5 or higher to receive an extermination order!");
#else /* for ironman there's no harm in allowing early quests */
	if (p_ptr->lev < 3) {
		msg_print(Ind, "\377oYou need to be level 3 or higher to receive an extermination order!");
#endif
		return FALSE;
	}

	if (iddc || mandos) {
		/* Specialty: In IDDC and Halls of Mandos, base the monster level on the floor level! */
		lev = getlevel(&p_ptr->wpos);
		lev += rand_int(lev / 20 + 3);
	} else {
		/* Normal: lev is the player's level. plev 1..50 -> mlev 1..100. */
		if (lev <= 50) lev += (lev * lev) / 83;
		else lev = 80 + rand_int(20);
	}

	get_mon_num_hook = xorder_aux;
	get_mon_num_prep(0, NULL);
	i = 2 + randint(5);

	do {
		r = get_mon_num(lev, lev - 10); //reduce OoD chance slightly

		k++;
		if (k > 100) lev--;
	} while (((lev - 5) > r_info[r].level && lev >= 5) ||
	    (r_info[r].flags1 & RF1_UNIQUE) ||
	    (r_info[r].flags7 & RF7_MULTIPLY) ||
	    !r_info[r].level); /* "no town quests" ;) */
	    //r_info[r].level <= 2); /* no Training Tower quests */

	/* easier in Ironman environments */
#ifndef RPG_SERVER
	if (iddc) {
#endif
		if (lev < 40) {
			if (r_info[r].flags1 & RF1_FRIENDS) i = i + 3 + randint(4);
			/* very easy for very low level non-friends quests */
			else if (lev < 20) i = (i + 1) / 2;
		} else {
			if (r_info[r].flags1 & RF1_FRIENDS) i = i + 9 + randint(5);
			if (i > 6) i--;
			if (i > 4) i--;
		}
#ifndef RPG_SERVER
	/* pack monsters require a high number, so 2-3 packs must be located */
	} else if (r_info[r].flags1 & RF1_FRIENDS) i = i + 11 + randint(7);
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
bool mon_take_hit(int Ind, int m_idx, int dam, bool *fear, cptr note) {
	player_type *p_ptr = Players[Ind];

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);

	s64b new_exp, new_exp_frac;
	s64b tmp_exp;
	int skill_trauma = (p_ptr->anti_magic || get_skill(p_ptr, SKILL_ANTIMAGIC)) ? 0 : get_skill_scale(p_ptr, SKILL_TRAUMATURGY, 100);
	bool old_tacit = suppress_message;

	//int dun_level2 = getlevel(&p_ptr->wpos);
	dungeon_type *dt_ptr2 = getdungeon(&p_ptr->wpos);
#if 0
	int dun_type2;
	dungeon_info_type *d_ptr2 = NULL;
	if (p_ptr->wpos.wz) {
		dun_type2 = dt_ptr2->type;
		d_ptr2 = &d_info[dun_type2];
	}
#endif

	p_ptr->test_count++;
	p_ptr->test_dam += dam;

	/* break charmignore */
	if (m_ptr->charmedignore) {
		Players[m_ptr->charmedignore]->mcharming--;
		m_ptr->charmedignore = 0;
	}

	/* Redraw (later) if needed */
	update_health(m_idx);

	/* Change monster's highest player encounter - mode 1+ : a player targetted this monster */
	if (!in_bree(&m_ptr->wpos)) { /* not in Bree, because of Halloween :) */
		if (m_ptr->henc < p_ptr->max_lev) m_ptr->henc = p_ptr->max_lev;
		if (m_ptr->henc_top < (p_ptr->max_lev + p_ptr->max_plv) / 2) m_ptr->henc_top = (p_ptr->max_lev + p_ptr->max_plv) / 2;
		if (m_ptr->henc < p_ptr->supp) m_ptr->henc = p_ptr->supp;
		if (m_ptr->henc_top < (p_ptr->max_lev + p_ptr->supp_top) / 2) m_ptr->henc_top = (p_ptr->max_lev + p_ptr->supp_top) / 2;
	}

	/* Traumaturgy skill - C. Blue */
	if (dam && skill_trauma &&
	    /*los(&p_ptr->wpos, p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx) && */
	    /*projectable(&p_ptr->wpos, p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx, MAX_RANGE) && */
	    target_able(Ind, m_idx) &&
	    (!(r_ptr->flags3 & RF3_UNDEAD)) &&
	    (!(r_ptr->flags3 & RF3_NONLIVING)) &&
	    (!(strchr("AEgv", r_ptr->d_char)))) {
		if (magik(p_ptr->antimagic)) {
#ifdef USE_SOUND_2010
			sound(Ind, "am_field", NULL, SFX_TYPE_MISC, FALSE);
#endif
			msg_format(Ind, "\377%cYour anti-magic field disrupts your traumaturgy.", COLOUR_AM_OWN);
			skill_trauma = FALSE;
		}
	} else skill_trauma = FALSE;
	if (skill_trauma) {
		/* difficult to balance, due to the different damage effects of spells- might need some changes */
		int eff_dam = (dam <= m_ptr->hp) ? dam : m_ptr->hp;
		long gain;

#if 0 /* linear gain */
		gain = (eff_dam * 100) / 50; //scale up by 100 for finer calc
#else /* log gain */
		if (eff_dam >= 50) {
			eff_dam = (eff_dam * 100) / 50; //scale up by 100 for finer calc
			gain = 4;
			while ((eff_dam = (eff_dam * 10) / 12) >= 100) gain++;
			//^ 1 (50), 3 (100), 12 (500), 16 (1000)
			gain *= 25; //100..500
		} else gain = eff_dam * 2;
#endif

#if 0 /* good balance but logical problem: more powerful monster gives less MP back than damaging some weak sucker */
		/* need a trauma skill matching rlev/2 or it'll become much less effective */
		if (skill_trauma < r_ptr->level) gain = (gain * skill_trauma * skill_trauma) / (r_ptr->level * r_ptr->level);
		if (gain < 100 && magik(gain)) gain = 1; /* fine-scale for values between +0..+1 MP */
		else gain /= 100; /* unscale back to actual MP points */
#else /* monster-level independant traumaturgy */
		/* actually for low damage give a certain minimum gain */
		if (gain < 100) gain = 100;
//s_printf("dgain %d, ", gain);
 #if 0 /* SKILL_TRAUMA has diminishing returns? */
		gain = (gain * (200 - (20000 / (skill_trauma + 100))) / 100); // (((...))) = 0..100, diminishing returns: 0->0%, 1->2%, 10->19%, 50->67%, 100->100%
 #else /* SKILL_TRAUMA has linear returns? */
		gain = (gain * skill_trauma) / 100; //linear 0..100%?
 #endif
//s_printf("cgain %d, ", gain);
		//if (!gain) gain = 1; //always a slight chance if we have the skill and inflicted >0 damage -- deprecated with minimum gain code further above
		/* translate it back from percent chances to actual MP points */
		if (gain < 100 && magik(gain)) gain = 1; /* fine-scale for values between +0..+1 MP */
		else gain /= 100; /* unscale back to actual MP points (rounding down, no fine-scaling here atm) */
//s_printf("gain %d\n", gain);
#endif

		if (gain && (p_ptr->csp < p_ptr->msp)
#ifdef MARTYR_NO_MANA
		    && !p_ptr->martyr
#endif
		    ) {
			msg_print(Ind, "You draw energy from the pain of your opponent.");
			p_ptr->csp += gain;
			if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;
			p_ptr->redraw |= (PR_MANA);
		}
	}

	/* Wake it up */
	m_ptr->csleep = 0;

	/* Some monsters are immune to death */
	if (r_ptr->flags7 & RF7_NO_DEATH) return FALSE;

	/* for when a quest giver turned non-invincible */
#if 0
	if (m_ptr->questor) {
		if (q_info[m_ptr->quest].defined && q_info[m_ptr->quest].questors > m_ptr->questor_idx) {
			if (q_info[m_ptr->quest].stage[q_info[m_ptr->quest].cur_stage].questor_hostility[m_ptr->questor_idx] &&
			    m_ptr->hp - dam <= q_info[m_ptr->quest].stage[q_info[m_ptr->quest].cur_stage].questor_hostility[m_ptr->questor_idx]->hostile_revert_hp)
				quest_questor_reverts(m_ptr->quest, m_ptr->questor_idx, &m_ptr->wpos);
		} else {
			s_printf("QUESTOR DEPRECATED (monster_dead3)\n");
		}
	}
#else
	if (m_ptr->questor && m_ptr->hp - dam <= m_ptr->limit_hp) {
		if (q_info[m_ptr->quest].defined && q_info[m_ptr->quest].questors > m_ptr->questor_idx)
			quest_questor_reverts(m_ptr->quest, m_ptr->questor_idx, &m_ptr->wpos);
		else
			s_printf("QUESTOR DEPRECATED (monster_dead3)\n");
	}
#endif

	/* Hurt it */
	m_ptr->hp -= dam;

	/* record the data for use in C_BLUE_AI */
	p_ptr->dam_turn[0] += (m_ptr->hp < dam) ? m_ptr->hp : dam;

	/* It is dead now */
	if (m_ptr->hp < 0) {
		char m_name[MNAME_LEN], m_name_real[MNAME_LEN];
		dun_level *l_ptr = getfloor(&p_ptr->wpos);
		bool necro = get_skill(p_ptr, SKILL_NECROMANCY) && !p_ptr->anti_magic && !get_skill(p_ptr, SKILL_ANTIMAGIC);

#ifdef ARCADE_SERVER
		cave_set_feat(&m_ptr->wpos, m_ptr->fy, m_ptr->fx, 172); /* drop "blood"? */
		if(m_ptr->hp < -1000) {

			object_type forge, *o_ptr;
			o_ptr = &forge;

			int i, head, arm, leg, part, ok;
			head = arm = leg = part = 0;
			for (i = 1; i < 5; i++) {
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
				(void)drop_near(0, o_ptr, 0, &m_ptr->wpos, m_ptr->fy, m_ptr->fx);
			}
		}
#endif

		/* prepare for experience calculation further down */
		if (m_ptr->level == 0) tmp_exp = r_ptr->mexp;
		else tmp_exp = r_ptr->mexp * m_ptr->level;

		/* for when a quest giver turned non-invincible */
		if (m_ptr->questor) {
			if (q_info[m_ptr->quest].defined && q_info[m_ptr->quest].questors > m_ptr->questor_idx) {
				if (q_info[m_ptr->quest].questor[m_ptr->questor_idx].exp != -1)
					tmp_exp = q_info[m_ptr->quest].questor[m_ptr->questor_idx].exp * m_ptr->level;
			} else {
				s_printf("QUESTOR DEPRECATED (monster_deatd2)\n");
			}
		}


		/* for obtaining statistical IDDC information: */
		if (l_ptr) l_ptr->monsters_killed++;

		/* Hack -- remove possible suppress flag */
		suppress_message = FALSE;

		/* Extract monster name */
		monster_desc(Ind, m_name, m_idx, 0);
		monster_desc(Ind, m_name_real, m_idx, 0x100);

#ifdef USE_SOUND_2010
#else
		sound(Ind, SOUND_KILL);
#endif

		/* Death by Missile/Spell attack */
		/* DEG modified spell damage messages. */
		if (note) {
			msg_format(Ind, "\377y%^s%s from \377g%d \377ydamage.", m_name, note, dam);
			msg_print_near_monvar(Ind, m_idx,
			    format("\377y%^s%s from \377g%d \377ydamage.", m_name_real, note, dam),
			    format("\377y%^s%s from \377g%d \377ydamage.", m_name, note, dam),
			    format("\377yIt%s from \377g%d \377ydamage.", note, dam));
		}

		/* Death by physical attack -- invisible monster */
		else if (!p_ptr->mon_vis[m_idx]) {
			msg_format(Ind, "\377yYou have killed %s.", m_name);
			/* other player(s) can maybe see it, so get at least 'killed' vs 'destroyed' right for them */
			if ((r_ptr->flags3 & RF3_DEMON) ||
			    (r_ptr->flags3 & RF3_UNDEAD) ||
			    (r_ptr->flags2 & RF2_STUPID) ||
			    (strchr("Evg", r_ptr->d_char)))
				msg_print_near_monvar(Ind, m_idx,
				    format("\377y%^s has been destroyed from \377g%d \377ydamage by %s.", m_name_real, dam, p_ptr->name),
				    format("\377y%^s has been destroyed from \377g%d \377ydamage by %s.", m_name, dam, p_ptr->name),
				    format("\377yIt has been killed from \377g%d \377ydamage by %s.", dam, p_ptr->name));
			else
				msg_print_near_monvar(Ind, m_idx,
				    format("\377y%^s has been killed from \377g%d \377ydamage by %s.", m_name_real, dam, p_ptr->name),
				    format("\377y%^s has been killed from \377g%d \377ydamage by %s.", m_name, dam, p_ptr->name),
				    format("\377yIt has been killed from \377g%d \377ydamage by %s.", dam, p_ptr->name));
		}

		/* Death by Physical attack -- non-living monster */
		else if ((r_ptr->flags3 & RF3_DEMON) ||
		    (r_ptr->flags3 & RF3_UNDEAD) ||
		    (r_ptr->flags2 & RF2_STUPID) ||
		    (strchr("Evg", r_ptr->d_char))) {
			msg_format(Ind, "\377yYou have destroyed %s.", m_name);
			msg_print_near_monvar(Ind, m_idx,
			    format("\377y%^s has been destroyed from \377g%d \377ydamage by %s.", m_name_real, dam, p_ptr->name),
			    format("\377y%^s has been destroyed from \377g%d \377ydamage by %s.", m_name, dam, p_ptr->name),
			    format("\377yIt has been killed from \377g%d \377ydamage by %s.", dam, p_ptr->name));
		}

		/* Death by Physical attack -- living monster */
		else {
			msg_format(Ind, "\377yYou have slain %s.", m_name);
			msg_print_near_monvar(Ind, m_idx,
			    format("\377y%^s has been slain from \377g%d \377ydamage by %s.", m_name_real, dam, p_ptr->name),
			    format("\377y%^s has been slain from \377g%d \377ydamage by %s.", m_name, dam, p_ptr->name),
			    format("\377yIt has been slain from \377g%d \377ydamage by %s.", dam, p_ptr->name));
		}

		/* Check if it's cloned unique, ie "someone else's spawn" */
		if ((r_ptr->flags1 & RF1_UNIQUE) && p_ptr->r_killed[m_ptr->r_idx] == 1)
			m_ptr->clone = 90; /* still allow some experience to be gained */

#ifdef SOLO_REKING
		/* Generate treasure and give kill credit */
		if (monster_death(Ind, m_idx) &&
		/* note: only our own killing blows count, not party exp! */
		    p_ptr->solo_reking) {
			int raw_exp = r_ptr->mexp * (100 - m_ptr->clone) / 100;

			/* appropriate depth still factors in! */
			raw_exp = det_exp_level(raw_exp, p_ptr->lev, getlevel(&p_ptr->wpos));

			p_ptr->solo_reking -= (raw_exp * 1) / 1; // 1 xp = 1 au (2.5?)
			if (p_ptr->solo_reking < 0) p_ptr->solo_reking = 0;
		}
#else
		/* Generate treasure and give kill credit */
		monster_death(Ind, m_idx);
#endif

		/* experience calculation: gain 2 decimal digits (for low-level exp'ing) */
		tmp_exp *= 100;

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
			if (dt_ptr2->flags1 & DF1_NO_UP)		factor += 5;
			if (dt_ptr2->flags2 & DF2_NO_RECALL_INTO)	factor += 5;
			if (dt_ptr2->flags1 & DF1_NO_RECALL)		factor += 10;
			if (dt_ptr2->flags1 & DF1_FORCE_DOWN)		factor += 10;
			if (dt_ptr2->flags2 & DF2_IRON)			factor += 15;
			if (dt_ptr2->flags2 & DF2_HELL)			factor += 10;
			if (dt_ptr2->flags2 & DF2_NO_DEATH)		factor -= 50;

			if (dt_ptr2->flags3 & DF3_EXP_5)		factor += 5;
			if (dt_ptr2->flags3 & DF3_EXP_10)		factor += 10;
			if (dt_ptr2->flags3 & DF3_EXP_20)		factor += 20;

#ifdef DUNGEON_VISIT_BONUS
			if (!(dt_ptr2->flags3 & DF3_NO_DUNGEON_BONUS))
				switch (dungeon_bonus[dt_ptr2->id]) {
				case 3: factor += 20; break;
				case 2: factor += 13; break;
				case 1: factor += 7; break;
				}
#endif

			tmp_exp = (tmp_exp * factor) / 100;
		}

		if (p_ptr->wpos.wz != 0) {
			/* Monsters in the Nether Realm give extra-high exp,
			   +2% per floor! (C. Blue) */
			if (dt_ptr2->type == DI_NETHER_REALM)
				tmp_exp = ((((-p_ptr->wpos.wz) * 2) + 100) * tmp_exp) / 100;
		}

		/* factor in clone state */
		tmp_exp = (tmp_exp * (100 - m_ptr->clone)) / 100;

		/* Split experience if in a party */
		if (p_ptr->party == 0 || p_ptr->ghost) {
			/* Don't allow cheap support from super-high level characters */
			if (cfg.henc_strictness && !p_ptr->total_winner &&
			    /* p_ptr->lev more logical but harsh: */
#if 1 /* player should always seek not too high-level party members compared to player's current real level? */
			    m_ptr->henc - p_ptr->max_lev > MAX_PARTY_LEVEL_DIFF + 1)
#else /* players may seek higher-level party members to team up with if he's died before? Weird combination so not recommended! */
			    m_ptr->henc - p_ptr->max_plv > MAX_PARTY_LEVEL_DIFF + 1)
#endif
				tmp_exp = 0; /* zonk */

			else {
				/* Higher characters who farm monsters on low levels compared to
				   their clvl will gain less exp. */
				if (!in_irondeepdive(&p_ptr->wpos) && !in_hallsofmandos(&p_ptr->wpos))
					tmp_exp = det_exp_level(tmp_exp, p_ptr->lev, getlevel(&p_ptr->wpos));

				/* Give some experience, undo 2 extra digits */
				new_exp = tmp_exp / p_ptr->lev / 100;

				/* Give fractional experience, undo 2 extra digits (*100L instead of *10000L) */
				new_exp_frac = ((tmp_exp - new_exp * p_ptr->lev * 100)
						* 100L) / p_ptr->lev + p_ptr->exp_frac;

				/* Never get too much exp off a monster
				   due to high level difference,
				   make exception for low exp boosts like "holy jackal" */
				if ((new_exp > r_ptr->mexp * 4) && (new_exp > 200)) {
					new_exp = r_ptr->mexp * 4;
					new_exp_frac = 0;
				}

				/* Keep track of experience */
				if (new_exp_frac >= 10000L) {
					new_exp++;
					p_ptr->exp_frac = new_exp_frac - 10000L;
				} else {
					p_ptr->exp_frac = new_exp_frac;
					p_ptr->redraw |= PR_EXP; //EXP_BAR_FINESCALE
				}

				/* Gain experience */
				if (new_exp) {
					if (!(p_ptr->mode & MODE_PVP)) gain_exp(Ind, new_exp);
				} else if (!p_ptr->warning_fracexp && tmp_exp) {
					msg_print(Ind, "\374\377ySome monsters give less than 1 experience point, but you still gain a bit!");
					s_printf("warning_fracexp: %s\n", p_ptr->name);
					p_ptr->warning_fracexp = 1;
				}
			}
		} else {
			/* Give experience to that party */
			/* Seemingly it's severe to cloning, but maybe it's ok :) */
			//if (!player_is_king(Ind) && !m_ptr->clone) party_gain_exp(Ind, p_ptr->party, tmp_exp);
			/* Since players won't share exp if leveldiff > MAX_PARTY_LEVEL_DIFF (7)
			   I see ne problem with kings sharing exp.
			   Otherwise Nether Realm parties are punished.. */
			//if (!player_is_king(Ind)) party_gain_exp(Ind, p_ptr->party, tmp_exp);
			//add 2 extra digits to r_ptr->mexp too by multiplying by 100, to match tmp_exp shift
			if (!(p_ptr->mode & MODE_PVP)) party_gain_exp(Ind, p_ptr->party, tmp_exp, r_ptr->mexp * 100, m_ptr->henc, m_ptr->henc_top);
		}


		/*
		 * Necromancy skill regenerates you
		 * Cannot drain an undead or nonliving monster
		 */
		if (necro &&
		    (!(r_ptr->flags3 & RF3_UNDEAD)) &&
		    (!(r_ptr->flags3 & RF3_NONLIVING)) &&
		    target_able(Ind, m_idx) && !p_ptr->ghost) { /* Target must be in LoS */
			if (magik(p_ptr->antimagic)) {
#ifdef USE_SOUND_2010
				sound(Ind, "am_field", NULL, SFX_TYPE_MISC, FALSE);
#endif
				msg_format(Ind, "\377%cYour anti-magic field disrupts your necromancy.", COLOUR_AM_OWN);
				necro = FALSE;
			}
		} else necro = FALSE;
		if (necro) {
			/*int gain = (r_ptr->level *
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

			if (((p_ptr->chp < p_ptr->mhp) || (p_ptr->csp < p_ptr->msp))
#ifdef MARTYR_NO_MANA
			    && !p_ptr->martyr
#endif
			    ) {
				msg_print(Ind, "You absorb the energy of the dying soul.");
#ifndef MARTYR_NO_MANA
				if (!p_ptr->martyr)
#endif
					hp_player_quiet(Ind, gain, TRUE);
				p_ptr->csp += gain_sp;
				if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;
				p_ptr->redraw |= (PR_MANA);
			}
		}

		/* RANGED ATTACKS ONLY vampire feeding! */
		/* Vampires feed off the life force! (if any) */
		// mimic forms for vampires/bats: 432, 520, 521, 623, 989
		if (p_ptr->vamp_fed_midx == m_idx) p_ptr->vamp_fed_midx = 0;
		else if (p_ptr->prace == RACE_VAMPIRE &&
		    !((r_ptr->flags3 & RF3_UNDEAD) ||
		    //(r_ptr->flags3 & RF3_DEMON) ||
		    (r_ptr->flags3 & RF3_NONLIVING) ||
		    (strchr("Egv", r_ptr->d_char)))
		    /* not too far away? */
		    && (ABS(m_ptr->fx - p_ptr->px) <= 1 && ABS(m_ptr->fy - p_ptr->py) <= 1)) {
			int feed = m_ptr->maxhp + 100;
			feed = (6 - (300 / feed)) * 100;//300..600
			if (r_ptr->flags3 & RF3_DEMON) feed /= 2;
			if (r_ptr->d_char == 'A') feed /= 3;
			/* Never get gorged */
			feed += p_ptr->food;
			if (feed >= PY_FOOD_MAX) feed = PY_FOOD_MAX - 1;
			set_food(Ind, feed);
		}

		/* Kill credit for quest */
		if (!m_ptr->clone) {
			int i, credit_idx = r_ptr->dup_idx ? r_ptr->dup_idx : m_ptr->r_idx;
			for (i = 0; i < MAX_XORDERS; i++) {
				if (p_ptr->xorder_id && xorders[i].id == p_ptr->xorder_id) {
					if (credit_idx == xorders[i].type) {
						p_ptr->xorder_num--;
						//there's a panic save bug after verygreat apply magic, q_n = -1 after
						if (p_ptr->xorder_num <= 0) kill_xorder(Ind);
						else msg_format(Ind, "%d more to go!", p_ptr->xorder_num);
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
		//if (r_ptr->flags1 & RF1_UNIQUE) r_ptr->max_num = 0;
		//p_ptr->r_killed[m_ptr->r_idx] = TRUE;

		/* Recall even invisible uniques or winners */
		if (p_ptr->mon_vis[m_idx] || (r_ptr->flags1 & RF1_UNIQUE)) {
			/* Count kills in all lives */
			if (!is_admin(p_ptr)) r_ptr->r_tkills++;

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

		/* prevent crash bug that happened in line 8237 (mon_take_hit),
		   apparently maxhp can extremely rarely be 0 - C. Blue */
		if (m_ptr->maxhp == 0) {
			s_printf("DBG_MAXHP_1 %d,%d\n", m_ptr->r_idx, m_ptr->ego);
			return FALSE;
		}

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
			m_ptr->monfear_gone = 0;
		}
	}

#endif

	/* Not dead yet */
	return (FALSE);
}

void monster_death_mon(int am_idx, int m_idx) {
	int i, j, y, x, ny, nx;
	int number = 0;
	cave_type *c_ptr;
	int dlev, rlev, tol_lev;

	monster_type *m_ptr = &m_list[m_idx];
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

	dlev = getlevel(wpos);
	rlev = r_ptr->level;

	/* Determine how much we can drop */
	if ((r_ptr->flags1 & RF1_DROP_60) && (rand_int(100) < 60)) number++;
	if ((r_ptr->flags1 & RF1_DROP_90) && (rand_int(100) < 90)) number++;
	if (r_ptr->flags0 & RF0_DROP_1) number++;
	if (r_ptr->flags1 & RF1_DROP_1D2) number += damroll(1, 2);
	if (r_ptr->flags0 & RF0_DROP_2) number += 2;
	if (r_ptr->flags1 & RF1_DROP_2D2) number += damroll(2, 2);
	if (r_ptr->flags1 & RF1_DROP_3D2) number += damroll(3, 2);
	if (r_ptr->flags1 & RF1_DROP_4D2) number += damroll(4, 2);

	/* Questors: Usually drop no items, except if specified */
	if (m_ptr->questor) {
		if (q_info[m_ptr->quest].defined && q_info[m_ptr->quest].questors > m_ptr->questor_idx) {
			if (!(q_info[m_ptr->quest].questor[m_ptr->questor_idx].drops & 0x1)) number = 0;
		} else {
			s_printf("QUESTOR_DEPRECATED (monster_dead)\n");
			number = 0;
		}
	}

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

#ifdef TRADITIONAL_LOOT_LEVEL
			/* Average dungeon and monster levels */
			object_level = (dlev + rlev) / 2;
 #ifdef RANDOMIZED_LOOT_LEVEL
			if (object_level < rlev) tol_lev = rlev - object_level;
			else tol_lev = dlev - object_level;
			if (tol_lev > 11) tol_lev = 13; /* need +12 levels of tolerance to allow depth-115 items to drop from level 80 monsters */
			object_level += rand_int(tol_lev);
 #endif
#else
			/* Monster level is more important than floor level */
			object_level = (dlev + rlev * 2) / 3;
 #ifdef RANDOMIZED_LOOT_LEVEL
			if (object_level < rlev) tol_lev = rlev - object_level;
			else tol_lev = dlev - object_level;
			if (tol_lev > 21) tol_lev = 21; /* need +20 levels of tolerance to allow depth-115 items to drop from level 80 monsters */
			object_level += rand_int(tol_lev);
 #endif
#endif

			/* No easy item hunting in towns.. */
			if (wpos->wz == 0) object_level = rlev / 2;

			/* Place Gold */
			if (do_gold && (!do_item || (rand_int(100) < 50)))
				place_gold(wpos, ny, nx, 0);
			/* Place Object */
			else {
				place_object_restrictor = RESF_NONE;
				place_object(wpos, ny, nx, good, great, FALSE, RESF_LOW, r_ptr->drops, 0, ITEM_REMOVAL_NORMAL);
			}

			/* Reset the object level */
			object_level = dlev;

			/* Reset "coin" type */
			coin_type = 0;

			/* Notice */
			note_spot_depth(wpos, ny, nx);

			/* Display */
			everyone_lite_spot(wpos, ny, nx);

			/* Under a player */
			if (c_ptr->m_idx < 0)
				msg_print(0 - c_ptr->m_idx, "You feel something roll beneath your feet.");

			break;
		}
	}

	if (m_ptr->special) s_printf("MONSTER_DEATH_MON: Golem of '%s'.\n", lookup_player_name(m_ptr->owner));
#ifdef RPG_SERVER
	else if (m_ptr->pet) s_printf("MONSTER_DEATH_MON: Pet of '%s'.\n", lookup_player_name(m_ptr->owner));
#endif

	/* for when a quest giver turned non-invincible */
	if (m_ptr->questor) {
		if (q_info[m_ptr->quest].defined && q_info[m_ptr->quest].questors > m_ptr->questor_idx) {
#if 0 /* For now, no player->no specific drop - oops. Todo: Review and improve maybe. */
			/* Drop a specific item? */
			if (q_info[m_ptr->quest].questor[m_ptr->questor_idx].drops & 0x2)
				questor_drop_specific(Ind, m_ptr->quest, m_ptr->questor_idx, wpos, x, y);
#endif
			/* Quest progression/fail effect? */
			questor_death(m_ptr->quest, m_ptr->questor_idx, wpos, 0);
		} else {
			s_printf("QUESTOR DEPRECATED (monster_dead2)\n");
		}
	}

	FREE(m_ptr->r_ptr, monster_race);
}

bool mon_take_hit_mon(int am_idx, int m_idx, int dam, bool *fear, cptr note) {
	monster_type *am_ptr = &m_list[am_idx];
	monster_type	*m_ptr = &m_list[m_idx];
	monster_race    *r_ptr = race_inf(m_ptr);
	s64b		new_exp;

	/* Redraw (later) if needed */
	update_health(m_idx);

	/* Wake it up */
	m_ptr->csleep = 0;

	/* Some monsters are immune to death */
	if (r_ptr->flags7 & RF7_NO_DEATH) return FALSE;

	/* Hurt it */
	m_ptr->hp -= dam;

	/* Cannot kill uniques */
	if ((r_ptr->flags1 & RF1_UNIQUE) && (m_ptr->hp < 1)) m_ptr->hp = 1;

	/* It is dead now */
	if (m_ptr->hp < 0) {
		/* Give some experience */
		//new_exp = ((long)r_ptr->mexp * r_ptr->level) / am_ptr->level;
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

		/* prevent crash bug that happened in line 8237 (mon_take_hit),
		   apparently maxhp can extremely rarely be 0 - C. Blue */
		if (m_ptr->maxhp == 0) {
			s_printf("DBG_MAXHP_2 %d,%d\n", m_ptr->r_idx, m_ptr->ego);
			return FALSE;
		}

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
			m_ptr->monfear_gone = 0;
		}
	}

#endif

	/* Not dead yet */
	return (FALSE);
}

/* Initialises the panel, when newly entering a level */
void panel_calculate(int Ind) {
	player_type *p_ptr = Players[Ind];

	p_ptr->panel_row = ((p_ptr->py - p_ptr->screen_hgt / 4) / (p_ptr->screen_hgt / 2));
	if (p_ptr->panel_row > p_ptr->max_panel_rows) p_ptr->panel_row = p_ptr->max_panel_rows;
	else if (p_ptr->panel_row < 0) p_ptr->panel_row = 0;

	p_ptr->panel_col = ((p_ptr->px - p_ptr->screen_wid / 4) / (p_ptr->screen_wid / 2));
	if (p_ptr->panel_col > p_ptr->max_panel_cols) p_ptr->panel_col = p_ptr->max_panel_cols;
	else if (p_ptr->panel_col < 0) p_ptr->panel_col = 0;

#if defined(ALERT_OFFPANEL_DAM) || defined(LOCATE_KEEPS_OVL)
	/* For alert-beeps on damage: Reset remembered panel */
	p_ptr->panel_row_old = p_ptr->panel_row;
	p_ptr->panel_col_old = p_ptr->panel_col;
#endif

	panel_bounds(Ind);

	tradpanel_calculate(Ind);
}
/* for functions of fixed size of effect that therefore require the
   'traditional' panel dimensions, such as Magic Mapping: */
void tradpanel_calculate(int Ind) {
	player_type *p_ptr = Players[Ind];

	p_ptr->tradpanel_row = ((p_ptr->py - SCREEN_HGT / 4) / (SCREEN_HGT / 2));
	if (p_ptr->tradpanel_row > p_ptr->max_tradpanel_rows) p_ptr->tradpanel_row = p_ptr->max_tradpanel_rows;
	else if (p_ptr->tradpanel_row < 0) p_ptr->tradpanel_row = 0;

	p_ptr->tradpanel_col = ((p_ptr->px - SCREEN_WID / 4) / (SCREEN_WID / 2));
	if (p_ptr->tradpanel_col > p_ptr->max_tradpanel_cols) p_ptr->tradpanel_col = p_ptr->max_tradpanel_cols;
	else if (p_ptr->tradpanel_col < 0) p_ptr->tradpanel_col = 0;

#if defined(ALERT_OFFPANEL_DAM) || defined(LOCATE_KEEPS_OVL)
	/* For alert-beeps on damage: Reset remembered panel */
	p_ptr->panel_row_old = p_ptr->panel_row;
	p_ptr->panel_col_old = p_ptr->panel_col;
#endif

	tradpanel_bounds(Ind);
}

/*
 * Calculates current boundaries
 * Called below and from "do_cmd_locate()".
 */
void panel_bounds(int Ind) {
	player_type *p_ptr = Players[Ind];

	p_ptr->panel_row_min = p_ptr->panel_row * (p_ptr->screen_hgt / 2);
	p_ptr->panel_row_max = p_ptr->panel_row_min + p_ptr->screen_hgt - 1;
	p_ptr->panel_row_prt = p_ptr->panel_row_min - SCREEN_PAD_TOP;
	p_ptr->panel_col_min = p_ptr->panel_col * (p_ptr->screen_wid / 2);
	p_ptr->panel_col_max = p_ptr->panel_col_min + p_ptr->screen_wid - 1;
	p_ptr->panel_col_prt = p_ptr->panel_col_min - SCREEN_PAD_LEFT;
}
void tradpanel_bounds(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* for stuff such as magic mapping that relies on traditional panel size */
	p_ptr->tradpanel_row_min = p_ptr->tradpanel_row * (SCREEN_HGT / 2);
	p_ptr->tradpanel_row_max = p_ptr->tradpanel_row_min + SCREEN_HGT - 1;
	p_ptr->tradpanel_col_min = p_ptr->tradpanel_col * (SCREEN_WID / 2);
	p_ptr->tradpanel_col_max = p_ptr->tradpanel_col_min + SCREEN_WID - 1;
}



/*
 * Given an row (y) and col (x), this routine detects when a move
 * off the screen has occurred and figures new borders. -RAK-
 *
 * "Update" forces a "full update" to take place.
 *
 * The map is reprinted if necessary, and "TRUE" is returned.
 */
void verify_panel(int Ind) {
	player_type *p_ptr = Players[Ind];

	int y = p_ptr->py;
	int x = p_ptr->px;

	int prow = p_ptr->panel_row;
	int pcol = p_ptr->panel_col;


	/* Also update (virtual) traditional panel */
	verify_tradpanel(Ind);


	/* Scroll screen when 2 grids from top/bottom edge */
	if ((y < p_ptr->panel_row_min + SCROLL_MARGIN_ROW) || (y > p_ptr->panel_row_max - SCROLL_MARGIN_ROW)) {
		prow = ((y - p_ptr->screen_hgt / 4) / (p_ptr->screen_hgt / 2));
		if (prow > p_ptr->max_panel_rows) prow = p_ptr->max_panel_rows;
		else if (prow < 0) prow = 0;
	}

	/* Scroll screen when 4 grids from left/right edge */
	if ((x < p_ptr->panel_col_min + SCROLL_MARGIN_COL) || (x > p_ptr->panel_col_max - SCROLL_MARGIN_COL)) {
		pcol = ((x - p_ptr->screen_wid / 4) / (p_ptr->screen_wid / 2));
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

#if defined(ALERT_OFFPANEL_DAM) || defined(LOCATE_KEEPS_OVL)
	/* For alert-beeps on damage: Reset remembered panel */
	p_ptr->panel_row_old = p_ptr->panel_row;
	p_ptr->panel_col_old = p_ptr->panel_col;
#endif

	/* client-side weather stuff */
	p_ptr->panel_changed = TRUE;

	/* Update stuff */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);
}
void verify_tradpanel(int Ind) {
	player_type *p_ptr = Players[Ind];

	int y = p_ptr->py;
	int x = p_ptr->px;

	int prow = p_ptr->tradpanel_row;
	int pcol = p_ptr->tradpanel_col;

	/* Scroll screen when 2 grids from top/bottom edge */
	if ((y < p_ptr->tradpanel_row_min + TRAD_SCROLL_MARGIN_ROW) || (y > p_ptr->tradpanel_row_max - TRAD_SCROLL_MARGIN_ROW)) {
		prow = ((y - SCREEN_HGT / 4) / (SCREEN_HGT / 2));
		if (prow > p_ptr->max_tradpanel_rows) prow = p_ptr->max_tradpanel_rows;
		else if (prow < 0) prow = 0;
	}

	/* Scroll screen when 4 grids from left/right edge */
	if ((x < p_ptr->tradpanel_col_min + TRAD_SCROLL_MARGIN_COL) || (x > p_ptr->tradpanel_col_max - TRAD_SCROLL_MARGIN_COL)) {
		pcol = ((x - SCREEN_WID / 4) / (SCREEN_WID / 2));
		if (pcol > p_ptr->max_tradpanel_cols) pcol = p_ptr->max_tradpanel_cols;
		else if (pcol < 0) pcol = 0;
	}

	/* Check for "no change" */
	if ((prow == p_ptr->tradpanel_row) && (pcol == p_ptr->tradpanel_col)) return;

#if 0 /* only in 'real' verify_panel() */
	/* Hack -- optional disturb on "panel change" */
	if (p_ptr->disturb_panel) disturb(Ind, 0, 0);
#endif

	/* Save the new panel info */
	p_ptr->tradpanel_row = prow;
	p_ptr->tradpanel_col = pcol;

	/* Recalculate the boundaries */
	tradpanel_bounds(Ind);

#if 0 /* only in 'real' verify_panel() */
	/* client-side weather stuff */
	p_ptr->panel_changed = TRUE;

	/* Update stuff */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);
#endif
}

/* Test whether the player is currently looking at his local surroundings (default)
   as opposed to some far off panel (done by cmd_locate()).
   Purpose: Prevent detection magic from working on remote areas. */
bool local_panel(int Ind) {
	player_type *p_ptr = Players[Ind];

	int y = p_ptr->py;
	int x = p_ptr->px;

	int prow = p_ptr->panel_row;
	int pcol = p_ptr->panel_col;


#if 0
	/* Also check (virtual) traditional panel */
	if (!local_tradpanel(Ind)) return FALSE;
#endif


	/* Scroll screen when 2 grids from top/bottom edge */
	if ((y < p_ptr->panel_row_min + SCROLL_MARGIN_ROW) || (y > p_ptr->panel_row_max - SCROLL_MARGIN_ROW)) {
		prow = ((y - p_ptr->screen_hgt / 4) / (p_ptr->screen_hgt / 2));
		if (prow > p_ptr->max_panel_rows) prow = p_ptr->max_panel_rows;
		else if (prow < 0) prow = 0;
	}

	/* Scroll screen when 4 grids from left/right edge */
	if ((x < p_ptr->panel_col_min + SCROLL_MARGIN_COL) || (x > p_ptr->panel_col_max - SCROLL_MARGIN_COL)) {
		pcol = ((x - p_ptr->screen_wid / 4) / (p_ptr->screen_wid / 2));
		if (pcol > p_ptr->max_panel_cols) pcol = p_ptr->max_panel_cols;
		else if (pcol < 0) pcol = 0;
	}

	/* Check for "no change" */
	if ((prow == p_ptr->panel_row) && (pcol == p_ptr->panel_col)) return TRUE;
	return FALSE;
}
#if 0
bool local_tradpanel(int Ind) {
	player_type *p_ptr = Players[Ind];

	int y = p_ptr->py;
	int x = p_ptr->px;

	int prow = p_ptr->tradpanel_row;
	int pcol = p_ptr->tradpanel_col;

	/* Scroll screen when 2 grids from top/bottom edge */
	if ((y < p_ptr->tradpanel_row_min + TRAD_SCROLL_MARGIN_ROW) || (y > p_ptr->tradpanel_row_max - TRAD_SCROLL_MARGIN_ROW)) {
		prow = ((y - SCREEN_HGT / 4) / (SCREEN_HGT / 2));
		if (prow > p_ptr->max_tradpanel_rows) prow = p_ptr->max_tradpanel_rows;
		else if (prow < 0) prow = 0;
	}

	/* Scroll screen when 4 grids from left/right edge */
	if ((x < p_ptr->tradpanel_col_min + TRAD_SCROLL_MARGIN_COL) || (x > p_ptr->tradpanel_col_max - TRAD_SCROLL_MARGIN_COL)) {
		pcol = ((x - SCREEN_WID / 4) / (SCREEN_WID / 2));
		if (pcol > p_ptr->max_tradpanel_cols) pcol = p_ptr->max_tradpanel_cols;
		else if (pcol < 0) pcol = 0;
	}

	/* Check for "no change" */
	if ((prow == p_ptr->tradpanel_row) && (pcol == p_ptr->tradpanel_col)) return TRUE;
	return FALSE;
}
#endif


/*
 * Monster health description
 */
cptr look_mon_desc(int m_idx) {
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);

	bool living = TRUE;
	int perc;


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


	/* prevent crash bug that happened in line 8237 (mon_take_hit),
	   apparently maxhp can extremely rarely be 0 - C. Blue */
	if (m_ptr->maxhp == 0) {
		s_printf("DBG_MAXHP_3 %d,%d\n", m_ptr->r_idx, m_ptr->ego);
		return "awake"; /* some excuse string ;) */
	}

	/* Calculate a health "percentage" */
	perc = 100L * m_ptr->hp / m_ptr->maxhp;

	if (perc >= 60) return (living ? "somewhat wounded" : "somewhat damaged");
	if (perc >= 25) return (living ? "wounded" : "damaged");
	if (perc >= 10) return (living ? "badly wounded" : "badly damaged");
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
void ang_sort_aux(int Ind, vptr u, vptr v, int p, int q) {
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

void ang_sort_extra_aux(int Ind, vptr u, vptr v, vptr w, int p, int q) {
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
		while (!(*ang_sort_extra_comp)(Ind, u, v, w, b, z)) b--;

		/* Slide i1 */
		while (!(*ang_sort_extra_comp)(Ind, u, v, w, z, a)) a++;

		/* Done partition */
		if (a >= b) break;

		/* Swap */
		(*ang_sort_extra_swap)(Ind, u, v, w, a, b);

		/* Advance */
		a++, b--;
	}

	/* Recurse left side */
	ang_sort_extra_aux(Ind, u, v, w, p, b);

	/* Recurse right side */
	ang_sort_extra_aux(Ind, u, v, w, b+1, q);
}


/*
 * Angband sorting algorithm -- quick sort in place
 *
 * Note that the details of the data we are sorting is hidden,
 * and we rely on the "ang_sort_comp()" and "ang_sort_swap()"
 * function hooks to interact with the data, which is given as
 * two pointers, and which may have any user-defined form.
 */
void ang_sort(int Ind, vptr u, vptr v, int n) {
	/* Sort the array */
	ang_sort_aux(Ind, u, v, 0, n-1);
}

/* Added this for further sorting the all monsters that are *closest* to
   the player by their sleep state: Target awake monsters first, before
   waking up sleeping ones. Suggested by Caine/Ifrit - C. Blue */
void ang_sort_extra(int Ind, vptr u, vptr v, vptr w, int n) {
	/* Sort the array */
	ang_sort_extra_aux(Ind, u, v, w, 0, n-1);
}

/* returns our max times 100 divided by our current...*/
static int player_wounded(s16b ind) {
	player_type *p_ptr = Players[ind];
	int wounded = ((p_ptr->mhp + 1) * 100) / (p_ptr->chp + 1); //prevent div/0

#if 0 /* Cure Wounds no longer cures status ailments */
	/* allow targetting healed up players that suffer from status ailments
	   curable by Cure Wounds spell - C. Blue */
	if (wounded == 100 &&
	    (p_ptr->cut || p_ptr->blind || p_ptr->confused || p_ptr->stun))
		wounded = 101;
#endif

	return wounded;
}

/* this should probably be somewhere more logical, but I should probably be
sleeping right now.....
Selects the most wounded target.

Hmm, I am sure there are faster sort algorithms out there... oh well, I don't
think it really matters... this one goes out to you Mr. Munroe.
-ADA-
*/

static void wounded_player_target_sort(int Ind, vptr sx, vptr sy, vptr id, int n) {
	int c, num;
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
bool ang_sort_comp_distance(int Ind, vptr u, vptr v, int a, int b) {
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
void ang_sort_swap_distance(int Ind, vptr u, vptr v, int a, int b) {
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


bool ang_sort_extra_comp_distance(int Ind, vptr u, vptr v, vptr w, int a, int b) {
	player_type *p_ptr = Players[Ind];

	byte *x = (byte*)(u);
	byte *y = (byte*)(v);
	byte *z = (byte*)(w);

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

	/* Compare the distances -- if equal, prefer the target that is not asleep, default to 'a'. */
	return ((da == db) ? (z[b] || !z[a]) : (da <= db));
}

void ang_sort_extra_swap_distance(int Ind, vptr u, vptr v, vptr w, int a, int b) {
	byte *x = (byte*)(u);
	byte *y = (byte*)(v);
	byte *z = (byte*)(w);

	byte temp;

	/* Swap "x" */
	temp = x[a];
	x[a] = x[b];
	x[b] = temp;

	/* Swap "y" */
	temp = y[a];
	y[a] = y[b];
	y[b] = temp;

	/* Swap "z" */
	temp = z[a];
	z[a] = z[b];
	z[b] = temp;
}



/*
 * Compare the values of two objects.
 *
 * Pointer "v" should not point to anything (it isn't used, anyway).
 */
bool ang_sort_comp_value(int Ind, vptr u, vptr v, int a, int b) {
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


void ang_sort_swap_value(int Ind, vptr u, vptr v, int a, int b) {
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
bool ang_sort_comp_mon_lev(int Ind, vptr u, vptr v, int a, int b) {
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
void ang_sort_swap_s16b(int Ind, vptr u, vptr v, int a, int b) {
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
bool ang_sort_comp_tval(int Ind, vptr u, vptr v, int a, int b) {
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
bool target_able(int Ind, int m_idx) {
	player_type *p_ptr = Players[Ind], *q_ptr;
	monster_type *m_ptr;

	if (!p_ptr) return FALSE;

	/* Hack -- no targeting hallucinations */
	if (p_ptr->image) return (FALSE);

	/* Safety check to prevent overflows below if any call of target_able() was executed with bad parameter here.
	   Note: target_who is only really used as parm in retaliate_...() (and in target_okay() itself), so this should never really happen.. */
	if (TARGET_STATIONARY(m_idx)) return FALSE; //paranoia

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

		if (m_ptr->owner == p_ptr->id) return(FALSE);

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

		if ((0 - m_idx) > NumPlayers) q_ptr = NULL;

		/* Paranoia check -- require a valid player */
		if (!q_ptr || q_ptr->conn == NOT_CONNECTED){
			p_ptr->target_who = 0;
			return (FALSE);
		}

		/* Players must be on same depth */
		if (!inarea(&p_ptr->wpos, &q_ptr->wpos)) return (FALSE);

		/* Player must be visible */
		if (!p_ptr->play_vis[-m_idx]) return (FALSE);

		/* Player must be in FoV */
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
bool target_okay(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Accept stationary targets */
	if (TARGET_STATIONARY(p_ptr->target_who)) return (TRUE);

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
s16b target_pick(int Ind, int y1, int x1, int dy, int dx) {
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
bool target_set(int Ind, int dir) {
	player_type *p_ptr = Players[Ind], *q_ptr;
	struct worldpos *wpos = &p_ptr->wpos;
	int		i, m, idx;
	int		y;
	int		x;
	//bool	flag = TRUE;
	bool	flag = autotarget;
	char	out_val[160];
	cave_type		*c_ptr;
	monster_type	*m_ptr;
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
			p_ptr->target_state[p_ptr->target_n] = m_ptr->csleep ? 1 : 0;
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
			p_ptr->target_state[p_ptr->target_n] = q_ptr->afk ? 1 : 0; /* AFK players count as 'asleep' ;) */
			p_ptr->target_n++;
		}

		/* Set the sort hooks */
		ang_sort_extra_comp = ang_sort_extra_comp_distance;
		ang_sort_extra_swap = ang_sort_extra_swap_distance;

		/* Sort the positions */
		ang_sort_extra(Ind, p_ptr->target_x, p_ptr->target_y, p_ptr->target_state, p_ptr->target_n);

		/* Collect indices */
		for (i = 0; i < p_ptr->target_n; i++) {
			c_ptr = &zcave[p_ptr->target_y[i]][p_ptr->target_x[i]];
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
			y = p_ptr->target_row + ddy[dir - 128];
			x = p_ptr->target_col + ddx[dir - 128];
			if (!in_bounds(y, x)) return FALSE; /* paranoia (won't harm, but seems weird) */
			p_ptr->target_row = y;
			p_ptr->target_col = x;
		}

		/* Info */
		if (dir != 128 + 5) /* code to confirm the targetted position, finishing the manual targetting process */
			strcpy(out_val, "[<dir>, t, q] "); /* still in the targetting process.. */

		/* Tell the client */
		Send_target_info(Ind, p_ptr->target_col - p_ptr->panel_col_prt, p_ptr->target_row - p_ptr->panel_row_prt, out_val);

		/* Check for completion */
		if (dir == 128 + 5) {
			p_ptr->target_who = MAX_M_IDX + 1; //TARGET_STATIONARY
			//p_ptr->target_who = 0;
//s_printf("target_info TRUE: %d,%d (%d,%d)\n", p_ptr->target_col - p_ptr->panel_col_prt, p_ptr->target_row - p_ptr->panel_row_prt, p_ptr->target_col, p_ptr->target_row);
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
		if (i > 0) {
			m = i;

#if 0 /* enable when this targetting method is fixed: */
    /* Currently you cannot select any available target, even though you can look at it,
	probably a problem in target_pick(). */
    /* Also make 'l'ooking at a monster target it, probably. (p_ptr->look_index vs p_ptr->target_who) */
			if (p_ptr->target_idx[i] > 0) {
				m_ptr = &m_list[p_ptr->target_idx[i]];
				snprintf(out_val, sizeof(out_val), "%s%s (Lv %d, %s%s)",
				    ((r_info[m_ptr->r_idx].flags1 & RF1_UNIQUE) && p_ptr->r_killed[m_ptr->r_idx] == 1) ? "\377D" : "",
				    r_name_get(&m_list[p_ptr->target_idx[m]]),
				    m_ptr->level, look_mon_desc(p_ptr->target_idx[m]),
				    m_ptr->clone ? ", clone" : "");
			} else if (p_ptr->target_idx[i] < 0) {
				q_ptr = Players[0 - p_ptr->target_idx[i]];
				if (q_ptr->body_monster) {
					snprintf(out_val, sizeof(out_val), "%s the %s (%s)", q_ptr->name, r_name + r_info[q_ptr->body_monster].name, get_ptitle(q_ptr, FALSE));
				} else {
					snprintf(out_val, sizeof(out_val), "%s the %s %s", q_ptr->name, get_prace(q_ptr), get_ptitle(q_ptr, FALSE));
				}
			}
			//strcpy(out_val, "[<dir>, t, q] ");
			Send_target_info(Ind, p_ptr->target_x[m] - p_ptr->panel_col_prt, p_ptr->target_y[m] - p_ptr->panel_row_prt, out_val);
#endif
		}
	}

	/* Target monsters */
	if (flag && p_ptr->target_n && p_ptr->target_idx[m] > 0) {
		y = p_ptr->target_y[m];
		x = p_ptr->target_x[m];
		idx = p_ptr->target_idx[m];

		m_ptr = &m_list[idx];

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
		if (TARGET_BEING(p_ptr->target_who)) health_track(Ind, p_ptr->target_who);
	}

	/* Failure */
	if (!p_ptr->target_who) return (FALSE);

	/* Clear target info */
	p_ptr->target_n = 0;

	/* Success */
	return (TRUE);
}


#if 0 /* not functional atm, replaced by target-closest strategy */

/* targets the most wounded teammate. should be useful for stuff like
 * heal other and teleport macros. -ADA-
 *
 * Now this function can take 3rd arg which specifies which player to
 * set the target.
 * This part was written by Asclep(DEG); thx for his courtesy!
 * */

bool target_set_friendly(int Ind, int dir, ...) {
	va_list ap;
	player_type *p_ptr = Players[Ind], *q_ptr;
	struct worldpos *wpos = &p_ptr->wpos;
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

	//if (!((castplayer > 0) && (castplayer < 20)))
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


 #if 0 /* currently no effect with wounded_player_target_sort() */
	/* Set the sort hooks */
	ang_sort_comp = ang_sort_comp_distance;
	ang_sort_swap = ang_sort_swap_distance;
 #endif
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

#else

/* targets most wounded player */
bool target_set_friendly(int Ind, int dir) {
	player_type *p_ptr = Players[Ind], *q_ptr;
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;
	int		i, m, idx;
	int		y;
	int		x;
	char	out_val[160];

	if (!(zcave = getcave(wpos))) return(FALSE);

	x = p_ptr->px;
	y = p_ptr->py;

	/* Go ahead and turn off target mode */
	p_ptr->target_who = 0;

	/* Turn off health tracking */
	health_track(Ind, 0);


	/* Reset "target" array */
	p_ptr->target_n = 0;

	for (i = 1; i <= NumPlayers; i++) {
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

 #if 1
	/* Sort the positions */
	wounded_player_target_sort(Ind, p_ptr->target_x, p_ptr->target_y, p_ptr->target_idx, p_ptr->target_n);
 #else //TODO
	/* Set the sort hooks */
	ang_sort_comp = ang_sort_comp_distance;
	ang_sort_swap = ang_sort_swap_distance;

	/* Sort the positions */
	ang_sort(Ind, p_ptr->target_x, p_ptr->target_y, p_ptr->target_n);
 #endif

	m = 0;

	/* handle player target.... */
	if (p_ptr->target_n) {
		y = p_ptr->target_y[m];
		x = p_ptr->target_x[m];
		idx = p_ptr->target_idx[m];

		/* Hack -- Track that player */
		health_track(Ind, 0 - idx);

		/* Hack -- handle stuff */
		handle_stuff(Ind);

		/* Describe */
		snprintf(out_val, sizeof(out_val), "%s targetted.", Players[idx]->name);

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

#endif



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
bool get_aim_dir(int Ind) {
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


bool get_item(int Ind, signed char tester_hook) { //paranoia @ 'signed' char =-p
	Send_item_request(Ind, tester_hook);

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
void set_recall_depth(player_type * p_ptr, object_type * o_ptr) {
	//int recall_depth = 0;
	//worldpos goal;

	unsigned char * inscription = (unsigned char *) quark_str(o_ptr->note);

	/* default to the players maximum depth */
	p_ptr->recall_pos.wx = p_ptr->wpos.wx;
	p_ptr->recall_pos.wy = p_ptr->wpos.wy;
#ifdef SEPARATE_RECALL_DEPTHS
	p_ptr->recall_pos.wz = (wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags &
			WILD_F_DOWN) ? 0 - get_recall_depth(&p_ptr->wpos, p_ptr) : get_recall_depth(&p_ptr->wpos, p_ptr);
#else
	p_ptr->recall_pos.wz = (wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags &
			WILD_F_DOWN) ? 0 - p_ptr->max_dlv : p_ptr->max_dlv;
#endif

#if 0
	p_ptr->recall_pos.wz = (wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags &
			WILD_F_DOWN) ? 0 - p_ptr->max_dlv :
			((wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags & WILD_F_UP) ?
			 p_ptr->max_dlv : 0);

	goal.wx = p_ptr->wpos.wx;
	goal.wy = p_ptr->wpos.wy;
	//goal.wz = 0 - p_ptr->max_dlv;	// hack -- default to 'dungeon'
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

	/* sanity check or crash */
	if (p_ptr->recall_pos.wx < 0) p_ptr->recall_pos.wx = 0;
	if (p_ptr->recall_pos.wy < 0) p_ptr->recall_pos.wy = 0;
}

bool set_recall_timer(int Ind, int v) {
	player_type *p_ptr = Players[Ind];
	bool notice = FALSE;
	struct dun_level *l_ptr = getfloor(&p_ptr->wpos);

	/* don't accidentally recall players in Ironman Deep Dive Challenge
	   by some effect (spell/Morgoth) */
	if (!is_admin(p_ptr) && (
#ifdef ANTI_TELE_CHEEZE
	    p_ptr->anti_tele ||
#endif
	    iddc_recall_fail(p_ptr, l_ptr))) {
		if (p_ptr->word_recall) v = 0;
		else {
			msg_print(Ind, "\377oThere is some static discharge in the air around you, but nothing happens.");
			return FALSE;
		}
	}

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

bool set_recall(int Ind, int v, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];
	struct dun_level *l_ptr = getfloor(&p_ptr->wpos);

	/* don't accidentally recall players in Ironman Deep Dive Challenge
	   by some effect (spell/Morgoth) */
	if (!is_admin(p_ptr) && (
#ifdef ANTI_TELE_CHEEZE
	    p_ptr->anti_tele ||
#endif
	    iddc_recall_fail(p_ptr, l_ptr))) {
		msg_print(Ind, "\377oThere is some static discharge in the air around you, but nothing happens.");
		return FALSE;
	}

	if (!p_ptr->word_recall) {
		set_recall_depth(p_ptr, o_ptr);
		return (set_recall_timer(Ind, v));
	} else {
		return (set_recall_timer(Ind, 0));
	}

}

void telekinesis_aux(int Ind, int item) {
	player_type *p_ptr = Players[Ind], *p2_ptr;
	object_type *q_ptr, *o_ptr = p_ptr->current_telekinesis;
	int Ind2;
	char *inscription, *scan;
#ifdef TELEKINESIS_GETITEM_SERVERSIDE
	int max_weight = p_ptr->current_telekinesis_mw;
#endif

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

#ifdef TELEKINESIS_GETITEM_SERVERSIDE
	if (q_ptr->weight * q_ptr->number > max_weight) {
		msg_format(Ind, "The item%s too heavy.", q_ptr->number > 1 ? "s are" : " is");
		return;
	}
#endif

	if (p2_ptr->ghost && !is_admin(p_ptr)) {
		msg_print(Ind, "You cannot send items to ghosts!");
		return;
	}

	if (compat_pmode(Ind, Ind2, FALSE) && !is_admin(p_ptr)) {
		msg_format(Ind, "You cannot contact %s beings!", compat_pmode(Ind, Ind2, FALSE));
		return;
	}

#ifdef IDDC_IRON_COOP
	if (in_irondeepdive(&p2_ptr->wpos) && (!p2_ptr->party || p2_ptr->party != p_ptr->party)) {
		msg_print(Ind, "\377yYou cannot contact outsiders.");
		if (!is_admin(p_ptr)) return;
	}
#endif
#ifdef IRON_IRON_TEAM
	if (p2_ptr->party && (parties[p2_ptr->party].mode & PA_IRONTEAM) && p2_ptr->party != p_ptr->party) {
		msg_print(Ind, "\377yYou cannot contact outsiders.");
		if (!is_admin(p_ptr)) return;
	}
#endif

	/* prevent winners picking up true arts accidentally */
	if (true_artifact_p(q_ptr) && !winner_artifact_p(q_ptr) &&
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
		//if (artifact_p(q_ptr) && cfg.anti_arts_pickup)
		{
			msg_print(Ind, "The target isn't powerful enough yet to receive that artifact!");
			if (!is_admin(p_ptr)) return;
		}
	}
#endif
	if ((k_info[q_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) &&
#ifdef FALLEN_WINNERSONLY
	    !p2_ptr->once_winner
#else
	    !p2_ptr->total_winner
#endif
	    ) {
		msg_print(Ind, "Only royalties are powerful enough to receive that item!");
		if (!is_admin(p_ptr)) return;
	}

	if (cfg.anti_arts_send && artifact_p(q_ptr) && !is_admin(p_ptr)) {
		msg_print(Ind, "The artifact resists telekinesis!");
		return;
	}

	/* questor items cannot be 'dropped', only destroyed! */
	if (q_ptr->questor) {
		msg_print(Ind, "This item cannot be sent by telekinesis!");
		return;
	}

	/* Add a check for full inventory of target player - mikaelh */
	if (!inven_carry_okay(Ind2, q_ptr, 0x0)) {
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
			d_ptr = getdungeon(&p_ptr->wpos);
			if (d_ptr && ((d_ptr->flags2 & (DF2_IRON | DF2_NO_RECALL_INTO)) || (d_ptr->flags1 & DF1_NO_RECALL))) {
				msg_print(Ind, "You are unable to contact that player");
				return;
			}
			d_ptr = getdungeon(&p2_ptr->wpos);
			if (d_ptr && ((d_ptr->flags2 & (DF2_IRON | DF2_NO_RECALL_INTO)) || (d_ptr->flags1 & DF1_NO_RECALL))) {
				msg_print(Ind, "You are unable to contact that player");
				return;
			}
		}

		if (!(p2_ptr->esp_link_flags & LINKF_TELEKIN)) {
			msg_print(Ind, "That player isn't concentrating on telekinesis at the moment.");
			if (!is_admin(p_ptr)) return;
		}

		/* Log it - mikaelh */
		object_desc_store(Ind, o_name, q_ptr, TRUE, 3);
		s_printf("(Tele) Item transaction from %s(%d) to %s(%d):\n  %s\n", p_ptr->name, p_ptr->lev, Players[Ind2]->name, Players[Ind2]->lev, o_name);

		if (true_artifact_p(q_ptr)) a_info[q_ptr->name1].carrier = p_ptr->id;

		/* Highlander Tournament: Don't allow transactions before it begins */
		if (!p2_ptr->max_exp) {
			msg_print(Ind2, "You gain a tiny bit of experience from receiving an item via telekinesis.");
			gain_exp(Ind2, 1);
		}

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

#ifdef USE_SOUND_2010
	sound_item(Ind, q_ptr->tval, q_ptr->sval, "pickup_");
#endif

	/* Wipe it */
	inven_item_increase(Ind, item, -99);
	inven_item_describe(Ind, item);
	inven_item_optimize(Ind, item);

	/* Combine the pack */
	p_ptr->notice |= (PN_COMBINE);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

}

int get_player(int Ind, object_type *o_ptr) {
	bool ok = FALSE;
	int Ind2 = 0;
	unsigned char * inscription = (unsigned char *) quark_str(o_ptr->note);
	char ins2[80];

	/* check for a valid inscription */
	if (inscription == NULL) {
		//msg_print(Ind, "Nobody to use the power with.");
		msg_print(Ind, "\377yNo target player specified.");
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

				//Ind2 = find_player_name(inscription);
				//Ind2 = name_lookup_loose(Ind, (cptr)inscription,< FALSE, FALSE, FALSE);
				Ind2 = name_lookup_loose(Ind, ins2, FALSE, FALSE, FALSE);
				if (Ind2) ok = TRUE;
			}
		}
		inscription++;
	}

	if (!ok) {
		msg_print(Ind, "\377yCouldn't find the target.");
		return 0;
	}

	if (Ind == Ind2) {
		msg_print(Ind, "\377yYou cannot do that on yourself.");
		return 0;
	}

	return Ind2;
}

/* Get base monster race to be conjured from an item inscription */
int get_monster(int Ind, object_type *o_ptr) {
	bool ok1 = TRUE, ok2 = TRUE;
	int r_idx = 0;

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

void blood_bond(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind], *p2_ptr;
	//bool ok = FALSE;
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
		msg_print(Ind, "\377oCouldn't blood bond because that player wasn't found.");
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

bool check_blood_bond(int Ind, int Ind2) {
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

void remove_blood_bond(int Ind, int Ind2) {
	player_type *p_ptr, *p2_ptr;
	player_list_type *pl_ptr, *ppl_ptr;

	p_ptr = Players[Ind];
	p2_ptr = Players[Ind2];
	if (!p2_ptr) return;

	ppl_ptr = NULL;
	pl_ptr = p_ptr->blood_bond;
	while (pl_ptr) {
		if (pl_ptr->id == p2_ptr->id) {
			if (ppl_ptr)
				ppl_ptr->next = pl_ptr->next;
			else
				/* First in the list */
				p_ptr->blood_bond = pl_ptr->next;
			KILL(pl_ptr, player_list_type);
			return;
		}
		ppl_ptr = pl_ptr;
		pl_ptr = pl_ptr->next;
	}
}
#ifdef TELEKINESIS_GETITEM_SERVERSIDE
bool telekinesis(int Ind, object_type *o_ptr, int max_weight) {
	player_type *p_ptr = Players[Ind];

	get_item(Ind, ITH_MAX_WEIGHT + max_weight);

	/* Clear any other pending actions */
	clear_current(Ind);

	p_ptr->current_telekinesis = o_ptr;
	p_ptr->current_telekinesis_mw = max_weight;

	return TRUE;
}
#endif

/* this has finally earned its own function, to make it easy for restoration to do this also */
bool do_scroll_life(int Ind) {
	int x,y;

	player_type * p_ptr = Players[Ind], *q_ptr;
	cave_type * c_ptr;
	cave_type **zcave;
	zcave = getcave(&p_ptr->wpos);
	if (!zcave) return(FALSE);

	for (y = -1; y <= 1; y++) {
		for (x = -1; x <= 1; x++) {
			c_ptr = &zcave[p_ptr->py+y][p_ptr->px+x];
			if (c_ptr->m_idx < 0) {
				q_ptr = Players[0 - c_ptr->m_idx];
				if (q_ptr->ghost) {
					if (cave_floor_bold(zcave, p_ptr->py + y, p_ptr->px + x) &&
					    !(c_ptr->info & CAVE_ICKY)) {
						resurrect_player(0 - c_ptr->m_idx, 0);
						/* if player is not in town and resurrected on *TRUE* death level
						   then this is a GOOD action. Reward the player */
						if (!istown(&p_ptr->wpos) && getlevel(&p_ptr->wpos) == q_ptr->died_from_depth) {
							u16b dal = 1 + ((2 * q_ptr->lev) / p_ptr->lev);

							if (p_ptr->align_good > dal) p_ptr->align_good -= dal;
							else p_ptr->align_good = 0;
						}
						return TRUE;
					} else msg_format(Ind, "The scroll fails for %s because there is no solid ground!", q_ptr->name);
				}
			}
		}
	}
	/* we did nore ressurect anyone */
	return FALSE;
}


/* modified above function to instead restore XP... used in priest spell rememberence */
bool do_restoreXP_other(int Ind) {
	int x,y;

	player_type * p_ptr = Players[Ind];
	cave_type * c_ptr;
	cave_type **zcave;
	if (!(zcave = getcave(&p_ptr->wpos))) return(FALSE);

	for (y = -1; y <= 1; y++) {
		for (x = -1; x <= 1; x++) {
			c_ptr = &zcave[p_ptr->py+y][p_ptr->px+x];

			if (c_ptr->m_idx < 0) {
				if (Players[0 - c_ptr->m_idx]->exp < Players[0 - c_ptr->m_idx]->max_exp) {
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
			teleport_player_level(i, TRUE);
		}
	}
	new_players_on_depth(wpos, 0, FALSE);
}

/* these Dungeon Master commands should probably be added somewhere else, but I am
 * hacking them together here to start.
 */

/* static or unstatic a level */
bool master_level(int Ind, char * parms) {
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
		/* additionally unstale it, simply by poofing it completely. - C. Blue */
		if (getcave(&twpos)) dealloc_dungeon_level(&twpos);
		msg_format(Ind, "The level %s has been unstaticed.", wpos_format(Ind, &twpos));
		break;
	}

	/* static the level */
	case 's':
		/* Increase the number of players on the dungeon
		 * masters level by one. */
		new_players_on_depth(&p_ptr->wpos, 1, TRUE);
		msg_format(Ind, "The level %s has been staticed.", wpos_format(Ind, &p_ptr->wpos));
		break;

	/* add dungeon stairs here */
	case 'D':
	{
		cave_type **zcave;
		u32b f1 = 0x0, f2 = 0x0, f3 = 0x0;
		if (!parms[1] || !parms[2] || p_ptr->wpos.wz) return FALSE;
		if (istown(&p_ptr->wpos)){
			msg_print(Ind,"Even you may not create dungeons in the town!");
			return FALSE;
		}
		/* extract flags (note that 0x01 are reservd hacks to prevent zero byte) */
		if (parms[4] & 0x02) f1 |= DF1_FORGET;
		if (parms[4] & 0x04) f3 |= (DF3_HIDDENLIB | DF3_DEEPSUPPLY);
		if (parms[4] & 0x08) f3 |= DF3_NO_SIMPLE_STORES;
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
		if (parms[3] == 't' && !(wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags & WILD_F_UP)){
			printf("tower: flags %x,%x\n", f1, f2);
			if ((zcave = getcave(&p_ptr->wpos))) {
				zcave[p_ptr->py][p_ptr->px].feat = FEAT_LESS;
				if (zcave[p_ptr->py][p_ptr->px].info & CAVE_JAIL) f3 |= DF3_JAIL_DUNGEON;
			}
			add_dungeon(&p_ptr->wpos, parms[1], parms[2], f1, f2, f3, TRUE, 0, parms[7], 0, 0);
			new_level_down_y(&p_ptr->wpos, p_ptr->py);
			new_level_down_x(&p_ptr->wpos, p_ptr->px);
		}
		if (parms[3] == 'd' && !(wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags & WILD_F_DOWN)){
			printf("dungeon: flags %x,%x\n", f1, f2);
			if ((zcave = getcave(&p_ptr->wpos))) {
				zcave[p_ptr->py][p_ptr->px].feat = FEAT_MORE;
				if (zcave[p_ptr->py][p_ptr->px].info & CAVE_JAIL) f3 |= DF3_JAIL_DUNGEON;
			}
			add_dungeon(&p_ptr->wpos, parms[1], parms[2], f1, f2, f3, FALSE, 0, parms[7], 0, 0);
			new_level_up_y(&p_ptr->wpos, p_ptr->py);
			new_level_up_x(&p_ptr->wpos, p_ptr->px);
		}
		break;
	}

	case 'R':
	{
		/* Remove dungeon (here) if it exists */
		cave_type **zcave;
		if (!(zcave = getcave(&p_ptr->wpos))) break;

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
			switch (zcave[p_ptr->py][p_ptr->px].feat) {
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
		if (!parms[1] || p_ptr->wpos.wz) return FALSE;
		if (istown(&p_ptr->wpos)) {
			msg_print(Ind, "There is already a town here!");
			return FALSE;
		}
		wpcopy(&twpos,&p_ptr->wpos);

		/* clean level first! */
		wipe_m_list(&p_ptr->wpos);
		wipe_o_list(&p_ptr->wpos);
		//wipe_t_list(&p_ptr->wpos);

		/* dont do this where there are houses! */
		for (i = 0; i < num_houses; i++) {
			if (inarea(&p_ptr->wpos, &houses[i].wpos))
				houses[i].flags |= HF_DELETED;
		}
		addtown(p_ptr->wpos.wy, p_ptr->wpos.wx, parms[1], 0, TOWN_VANILLA);
		unstatic_level(&twpos);
		if (getcave(&twpos))
			dealloc_dungeon_level(&twpos);

		break;
	}
	/* default -- do nothing. */
	default: break;
	}
	return TRUE;
}

/* static or unstatic a level (from chat-line command) */
bool master_level_specific(int Ind, struct worldpos *wpos, char * parms) {
	/* get the player pointer */
	player_type *p_ptr = Players[Ind];

	//if (strcmp(p_ptr->name, cfg_dungeon_master)) return FALSE;
	if (!is_admin(p_ptr)) return FALSE;

	switch (parms[0]) {
	/* unstatic the level */
	case 'u':
		unstatic_level(wpos);
		/* additionally unstale it, simply by poofing it completely. - C. Blue */
		dealloc_dungeon_level(wpos);
		//msg_format(Ind, "The level (%d,%d) %dft has been unstaticed.", wpos->wx, wpos->wy, wpos->wz * 50);
		msg_format(Ind, "The level %s has been unstaticed.", wpos_format(Ind, wpos));
		break;
	/* static the level */
	case 's':
		/* Increase the number of players on the dungeon
		 * masters level by one. */
		new_players_on_depth(&p_ptr->wpos, 1, TRUE);
		msg_print(Ind, "The level has been staticed.");
		break;
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
	player_type *p_ptr = Players[Ind];
	int i;

	for (i = 0; i < num_houses; i++) {
		if (inarea(&houses[i].wpos, &p_ptr->wpos)) {
			if (fill_house(&houses[i], FILL_PLAYER, p_ptr)) {
				if (access_door(Ind, houses[i].dna, FALSE) || admin_p(Ind)) {
					if (houses[i].dna->owner_type == OT_GUILD &&
					    p_ptr->guild == houses[i].dna->owner &&
					    guilds[p_ptr->guild].master == p_ptr->id) {
						if (p_ptr->au > 1000) {
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
bool master_build(int Ind, char * parms) {
	player_type * p_ptr = Players[Ind];
	cave_type * c_ptr;
	struct c_special *cs_ptr;
	static unsigned char new_feat = FEAT_WALL_EXTRA;
	cave_type **zcave;

	if (!(zcave = getcave(&p_ptr->wpos))) return(FALSE);

	if (!is_admin(p_ptr) && (!player_is_king(Ind)) && (!guild_build(Ind))) return FALSE;

	/* extract arguments, otherwise build a wall of type new_feat */
	if (parms) {
		/* Hack -- the first character specifies the type of wall */
		new_feat = parms[0];
		/* Hack -- toggle auto-build on/off */
		switch (parms[1]) {
		case 'T': p_ptr->master_move_hook = master_build; break;
		case 'F': p_ptr->master_move_hook = NULL; return FALSE;
		default : break;
		}
	}

	c_ptr = &zcave[p_ptr->py][p_ptr->px];

	/* Never destroy real house doors! Work on this later */
	if ((cs_ptr = GetCS(c_ptr, CS_DNADOOR))) return(FALSE);

	/* This part to be rewritten for stacked CS */
	c_ptr->feat = new_feat;
	if (c_ptr->feat == FEAT_HOME) {
		struct c_special *cs_ptr;
		/* new special door creation (with keys) */
		struct key_type *key;
		object_type newkey;
		int id;
		MAKE(key, struct key_type);
		sscanf(&parms[2],"%d",&id);
		key->id = id;
		invcopy(&newkey, lookup_kind(TV_KEY, SV_HOUSE_KEY ));
		newkey.pval = key->id;
		newkey.marked2 = ITEM_REMOVAL_NEVER;
		drop_near(0, &newkey, -1, &p_ptr->wpos, p_ptr->py, p_ptr->px);
		cs_ptr = ReplaceCS(c_ptr, CS_KEYDOOR);
		if (cs_ptr) cs_ptr->sc.ptr = key;
		else KILL(key, struct key_type);
		p_ptr->master_move_hook = NULL;	/*buggers up if not*/
	}
	if (c_ptr->feat == FEAT_SIGN) {
		struct c_special *cs_ptr;
		struct floor_insc *sign;
		MAKE(sign, struct floor_insc);
		strcpy(sign->text, &parms[2]);
		cs_ptr = ReplaceCS(c_ptr, CS_INSCRIP);
		if (cs_ptr) cs_ptr->sc.ptr = sign;
		else KILL(sign, struct floor_insc);
		p_ptr->master_move_hook = NULL;	/*buggers up if not*/
	}

	return TRUE;
}

static char master_specific_race_char = 'a';

static bool master_summon_specific_aux(int r_idx) {
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
static u16b master_summon_aux_monster_type(int Ind, char monster_type, char * monster_parms) {
	player_type *p_ptr = Players[Ind];
	int tmp, lev;

	/* handle each category of monster types */
	switch (monster_type) {
	/* specific monster specified */
	case 's':
		/* allows specification by monster No. */
		tmp = atoi(monster_parms);
		if (tmp > 0) return tmp;

		/* if the name was specified, summon this exact race */
		if (strlen(monster_parms) > 1) return race_index(monster_parms);
		/* otherwise, summon a monster that looks like us */
		else {
			master_specific_race_char = monster_parms[0];
			get_mon_num_hook = master_summon_specific_aux;
			get_mon_num_prep(0, NULL);
			//tmp = get_mon_num(rand_int(100) + 10);
			lev = (monster_parms[0] == 't') ? 0 : rand_int(100) + 10;
			tmp = get_mon_num(lev, lev);

			/* restore monster generator */
			get_mon_num_hook = dungeon_aux;

			/* return our monster */
			return tmp;
		}

	/* orc specified */
	case 'o':
		/* if not random, assume specific orc specified */
		if (strcmp(monster_parms, "random")) return race_index(monster_parms);
		/* random orc */
		else switch(rand_int(6)) {
			case 0: return race_index("Snaga");
			case 1: return race_index("Cave orc");
			case 2: return race_index("Hill orc");
			case 3: return race_index("Dark orc");
			case 4: return race_index("Half-orc");
			case 5: return race_index("Uruk");
		}
		break;

	/* low undead specified */
	case 'u':
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

	/* high undead specified */
	case 'U':
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

	/* specific depth specified */
	case 'd':
		get_mon_num_hook = NULL;//dungeon_aux; --'depth' doesn't have to be for dungeon, we could also be summoning in the wilderness..

		/* Wilderness-specific hook */
		if (!p_ptr->wpos.wz) set_mon_num_hook_wild(&p_ptr->wpos);

		get_mon_num_prep(0, NULL);
		return get_mon_num(monster_parms[0], monster_parms[0] - 20); //reduce OoD if we summon depth-specificly

	default : break;
	}

	/* failure */
	return 0;

}

/* Temporary debugging hack, to test the new excellents.
 */
bool master_acquire(int Ind, char * parms) {
	player_type * p_ptr = Players[Ind];

	if (!is_admin(p_ptr)) return FALSE;
	acquirement(&p_ptr->wpos, p_ptr->py, p_ptr->px, 1, TRUE, TRUE, make_resf(p_ptr));
	return TRUE;
}

/* Monster summoning options. More documentation on this later. */
bool master_summon(int Ind, char * parms) {
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
	if (parms) {
		/* the first character specifies the type of monster */
		summon_type = parms[0];
		summon_parms = parms[1];
		monster_type = parms[2];
		/* Hack -- since monster_parms is a string, throw it on the end */
		strcpy(monster_parms, &parms[3]);
	}

	switch (summon_type) {
	/* summon x here */
	case 'x':
		/* for each monster we are summoning */
		for (c = 0; c < summon_parms; c++) {
			/* figure out who to summon */
			r_idx = master_summon_aux_monster_type(Ind, monster_type, monster_parms);

			/* summon the monster, if we have a valid one */
			if (r_idx) summon_specific_race(&p_ptr->wpos, p_ptr->py, p_ptr->px, r_idx, 0, 1);
		}
		break;

	/* summon x at random locations */
	case 'X':
		for (c = 0; c < summon_parms; c++) {
			/* figure out who to summon */
			r_idx = master_summon_aux_monster_type(Ind, monster_type, monster_parms);

			/* summon the monster at a random location */
			if (r_idx) summon_specific_race_somewhere(&p_ptr->wpos,r_idx, 0, 1);
		}
		break;

	/* summon group of random size here */
	case 'g':
		/* figure out how many to summon */
		size = rand_int(rand_int(50)) + 2;

		/* figure out who to summon */
		r_idx = master_summon_aux_monster_type(Ind, monster_type, monster_parms);

		/* summon the group here */
		summon_specific_race(&p_ptr->wpos, p_ptr->py, p_ptr->px, r_idx, 0, size);
		break;

	/* summon group of random size at random location */
	case 'G':
		/* figure out how many to summon */
		size = rand_int(rand_int(50)) + 2;

		/* figure out who to summon */
		r_idx = master_summon_aux_monster_type(Ind, monster_type, monster_parms);

		/* someone the group at a random location */
		summon_specific_race_somewhere(&p_ptr->wpos, r_idx, 0, size);
		break;

	/* summon mode on (use with discretion... lets not be TOO mean ;-) )*/
	case 'T':
		summon_type = 'x';
		summon_parms = 1;

		p_ptr->master_move_hook = master_summon;
		break;

	/* summon mode off */
	case 'F':
		p_ptr->master_move_hook = NULL;
		break;
	}

	summon_override_checks = SO_NONE; /* clear all override flags */
	return TRUE;
}

#define FIND_CLOSEST_JAIL
bool imprison(int Ind, u16b time, char *reason) {
	int id, i, j;
#ifdef FIND_CLOSEST_JAIL
	int dist = 999, tmp, picked = -1;
#endif
	struct dna_type *dna;
	player_type *p_ptr = Players[Ind];
	char string[160];
	cave_type **zcave, **nzcave;
	struct worldpos old_wpos;

	/* Can't jail ghosts */
	if (p_ptr->ghost) {
		p_ptr->tim_susp += time;
		s_printf("TIM_SUSP_GHOST.\n");
		return FALSE;
	}

	if (!jails_enabled) {
		p_ptr->tim_susp = 0;
		s_printf("IMPRISON: '%s' DISABLED.\n", p_ptr->name);
		return FALSE;
	}

	if (!(zcave = getcave(&p_ptr->wpos))) return (FALSE);

	s_printf("IMPRISON: '%s' ", p_ptr->name);

	if (!p_ptr || !(id = lookup_player_id("Jailer"))) {
		s_printf("NO_JAILER.\n");
		return (FALSE);
	}

	if (p_ptr->wpos.wz) {
		p_ptr->tim_susp += time;
		s_printf("TIM_SUSP.\n");
		return (TRUE);
	}

#ifdef JAIL_TOWN_AREA /* only imprison when within town area? */
	if (!istownarea(&p_ptr->wpos, MAX_TOWNAREA)) {
		p_ptr->tim_susp += time;
		s_printf("NO_TOWN.\n");
		return (TRUE);
	}
#endif

	if (p_ptr->tim_jail) {
		p_ptr->tim_jail += time;
		s_printf("TIM_JAIL.\n");
		return (TRUE);
	}

	/* get appropriate prison house */
	for (i = 0; i < num_houses; i++) {
		if (!(houses[i].flags & HF_JAIL)) continue;
		if ((houses[i].flags & HF_DELETED)) continue;
		dna = houses[i].dna;
		if (dna->owner == id && dna->owner_type == OT_PLAYER) {
#ifndef FIND_CLOSEST_JAIL /* get first prison we find? */
			break;
#else /* get closest prison we find? */
			tmp = distance(houses[i].wpos.wy, houses[i].wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wx);
			if (tmp < dist) {
				dist = tmp;
				picked = i;
			}
#endif
		}
	}
#ifndef FIND_CLOSEST_JAIL
	if (i == num_houses) {
		p_ptr->tim_susp = 0;
		s_printf("NO_JAIL.\n");
		return (FALSE);
	}
#else
	if (picked == -1) {
		p_ptr->tim_susp = 0;
		s_printf("NO_JAIL.\n");
		return (FALSE);
	}
	i = picked;
#endif

	/* lazy, single prison system */
	/* hopefully no overcrowding! */
	if (!(nzcave = getcave(&houses[i].wpos))) {
		alloc_dungeon_level(&houses[i].wpos);
		generate_cave(&houses[i].wpos, p_ptr);

#if 1
		/* generate some vermin randomly, for flavour */
		nzcave = getcave(&houses[i].wpos);
		if (rand_int(2)) for (j = randint(2); j; j--) {
			int x, y;
			//abuse 'id'
			id = 10;
			while (--id) {
				if (rand_int(2)) x = houses[i].x - 15 + rand_int(13);
				else x = houses[i].x + 15 - rand_int(13);
				if (rand_int(2)) y = houses[i].y - 15 + rand_int(13);
				else y = houses[i].y + 15 - rand_int(13);

				if (!in_bounds(y, x)) continue;
				if (!(nzcave[y][x].info & CAVE_STCK)) continue;

				break;
			}
			if (id) place_monster_one(&houses[i].wpos,
			    y, x,
			    1, 0, 0, rand_int(2) ? TRUE : FALSE, 0, 0);
		}
#endif
	}

	store_exit(Ind);

	zcave[p_ptr->py][p_ptr->px].m_idx = 0;
	everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
	forget_lite(Ind);
	forget_view(Ind);
	wpcopy(&old_wpos, &p_ptr->wpos);
	wpcopy(&p_ptr->wpos, &houses[i].wpos);
	new_players_on_depth(&old_wpos, -1, TRUE);

	p_ptr->py = houses[i].y;
	p_ptr->px = houses[i].x;
	p_ptr->house_num = i + 1;

	/* that messes it up */
	/* nzcave[p_ptr->py][p_ptr->px].m_idx = (0-Ind); */
	new_players_on_depth(&p_ptr->wpos, 1, TRUE);

	p_ptr->new_level_flag = TRUE;
	p_ptr->new_level_method = LEVEL_HOUSE;

#ifdef JAILER_KILLS_WOR
	/* eat his WoR scrolls as suggested? */
	id = FALSE; //abuse 'id' and 'i'
	i = TRUE;
	for (j = 0; j < INVEN_WIELD; j++) {
		object_type *j_ptr;
		if (!p_ptr->inventory[j].k_idx) continue;
		j_ptr = &p_ptr->inventory[j];
		if ((j_ptr->tval == TV_SCROLL) && (j_ptr->sval == SV_SCROLL_WORD_OF_RECALL)) {
			if (id) i = FALSE;
			if (j_ptr->number > 1) i = FALSE;
			inven_item_increase(Ind, j, -j_ptr->number);
			inven_item_optimize(Ind, j);
			combine_pack(Ind);
			reorder_pack(Ind);
			id = TRUE;
		}
	}
	if (id) msg_format(Ind, "\376\377oThe jailer confiscates your word-of-recall scroll%s.", i ? "" : "s");
#endif

	everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
	snprintf(string, sizeof(string), "\374\377o%s was jailed for %s.", p_ptr->name, reason);
	msg_broadcast(Ind, string);
	msg_format(Ind, "\374\377oYou have been jailed for %s.", reason);
	p_ptr->tim_jail = time + p_ptr->tim_susp;
	p_ptr->tim_susp = 0;
#ifdef USE_SOUND_2010
	sound(Ind, "jailed", NULL, SFX_TYPE_MISC, TRUE);
#endif

	s_printf("DONE.\n");
	return (TRUE);
}

static void player_edit(char *name) {
//TODO?
}

bool master_player(int Ind, char *parms){
	player_type *p_ptr = Players[Ind];
	player_type *q_ptr;
	int Ind2 = 0;
	int i;
	struct account acc;
	int *id_list, n;
	char buf[MSG_LEN];

	if (!is_admin(p_ptr)) {
		msg_print(Ind,"You need to be the dungeon master to use this command.");
		return FALSE;
	}

	switch (parms[0]) {
	case 'E':	/* offline editor */
		for (i = 1; i <= NumPlayers; i++){
			if (!strcmp(Players[i]->name, &parms[1])){
				msg_format(Ind,"%s is currently playing",&parms[1]);
				return(FALSE);
			}
		}
		player_edit(&parms[1]);
		break;

	case 'A':	/* acquirement */
		Ind2 = name_lookup(Ind, &parms[1], FALSE, FALSE, TRUE);
		if (Ind2) {
			player_type *p_ptr2 = Players[Ind2];
			acquirement(&p_ptr2->wpos, p_ptr2->py, p_ptr2->px, 1, TRUE, TRUE, make_resf(p_ptr2));
			msg_format(Ind, "%s is granted an item.", p_ptr2->name);
			msg_print(Ind2, "You feel a divine favor!");
			return(FALSE);
		}
		//msg_print(Ind, "That player is not in the game.");
		break;

	case 'k':	/* admin wrath (preceed name with '!' for no-ghost kill */
		i = 1;
		if (parms[1] == '!') i = 2;
		Ind2 = name_lookup(Ind, &parms[i], FALSE, FALSE, TRUE);
		if (Ind2) {
			q_ptr = Players[Ind2];
			msg_print(Ind2, "\377rYou are hit by a bolt from the blue!");
			strcpy(q_ptr->died_from,"divine wrath");
			if (i == 2) q_ptr->global_event_temp |= PEVF_NOGHOST_00; //hack: no-ghost death
			q_ptr->deathblow = 0;
			player_death(Ind2);
			return(TRUE);
		}
		//msg_print(Ind, "That player is not in the game.");
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
		sprintf(buf, "\375\377r[\377%c%s\377r]\377%c %s", 'b', p_ptr->name, COLOUR_CHAT, &parms[1]); /* admin colour 'b' */
		censor_length = 0;
		msg_broadcast(0, buf);
#ifdef TOMENET_WORLDS
		if (cfg.worldd_broadcast) world_chat(0, buf);
#endif
		break;

	case 'r':	/* FULL ACCOUNT SCAN + RM */
		/* Delete a player from the database/savefile */
		if (GetAccount(&acc, &parms[1], NULL, FALSE)) {
			char name[80];
			n = player_id_list(&id_list, acc.id);
			for(i = 0; i < n; i++) {
				strcpy(name, lookup_player_name(id_list[i]));
				msg_format(Ind, "\377oDeleting %s", name);
				delete_player_id(id_list[i]);
				sf_delete(name);
			}
			if (n) C_KILL(id_list, n, int);
			acc.flags |= ACC_DELD;
			/* stamp in the deleted account */
			WriteAccount(&acc, FALSE);
			memset(acc.pass, 0, sizeof(acc.pass));
		} else msg_print(Ind, "\377rCould not find account");
		break;
	}
	return(FALSE);
}

static vault_type *get_vault(char *name) {
	int i;

	for (i = 0; i < MAX_V_IDX; i++) {
		if (strstr(v_name + v_info[i].name, name))
			return &v_info[i];
	}

	return NULL;
}

/* Generate something */
bool master_generate(int Ind, char * parms) {
	/* get the player pointer */
	player_type *p_ptr = Players[Ind];

	if (!is_admin(p_ptr)) return FALSE;

	switch (parms[0]) {
		/* generate a vault */
		case 'v':
		{
			vault_type *v_ptr = NULL;

			switch (parms[1]) {
			case '#':
				v_ptr = &v_info[parms[2] + 127];
				break;
			case 'n':
				v_ptr = get_vault(&parms[2]);
			}

			if(!v_ptr || !v_ptr->wid) return FALSE;

			//build_vault(&p_ptr->wpos, p_ptr->py, p_ptr->px, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text);
			build_vault(&p_ptr->wpos, p_ptr->py, p_ptr->px, v_ptr, p_ptr);

			break;
		}
	}
	return TRUE;
}

#if 0 /* some new esp link stuff - mikaelh */
bool establish_esp_link(int Ind, int Ind2, byte type, u16b flags, u16b end) {
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

		if (!(esp_ptr->flags & LINKF_HIDDEN)) {
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

void break_esp_link(int Ind, int Ind2) {
	player_type *p_ptr, *p2_ptr;
	esp_link_type *esp_ptr, *pest_link;

	p_ptr = Players[Ind];
	p2_ptr = Players[Ind2];
	if (!p2_ptr) return;

	pesp_ptr = NULL;
	esp_ptr = p_ptr->esp_link;
	while (esp_ptr) {
		if (esp_ptr->id == p2_ptr->id) {
			if (!(esp_ptr->flags & LINKF_HIDDEN)) {
				msg_format(Ind, "\377RYou break the mind link with %s.", p2_ptr->name);
				msg_format(Ind2, "\377R%s breaks the mind link with you.", p_ptr->name);
			}

			if (pesp_ptr) pest_ptr->next = esp_ptr->next;
			else p_ptr->esp_link = esp_ptr->next;
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

esp_link_type *check_esp_link(ind Ind, int Ind2) {
	player_type *p_ptr, *p2_ptr;
	esp_link_type *esp_ptr;

	p_ptr = Players[Ind];
	p2_ptr = Players[Ind2];
	if (!p2_ptr) return NULL;

	esp_ptr = p_ptr->esp_link;
	while (esp_ptr) {
		if (esp_ptr->id == p2_ptr->id) return esp_ptr;
		esp_ptr = esp_ptr->next;
	}
	return NULL;
}

bool check_esp_link_type(int Ind, int Ind2, u16b flags) {
	esp_link_type* esp_ptr;
	esp_ptr = check_esp_link(Ind, Ind2);

	if (esp_ptr && esp_ptr->flags & flags) return TRUE;
	else return FALSE;
}
#endif

void toggle_aura(int Ind, int aura) {
	char buf[80];
	player_type *p_ptr = Players[Ind];

	switch (aura) {
	case 0:
		if (!get_skill(p_ptr, SKILL_AURA_FEAR)) {
			msg_print(Ind, "You don't know how to unleash an aura of fear.");
			return;
		}
		break;
	case 1:
		if (!get_skill(p_ptr, SKILL_AURA_SHIVER)) {
			msg_print(Ind, "You don't know how to unleash a shivering aura.");
			return;
		}
		break;
	case 2:
		if (!get_skill(p_ptr, SKILL_AURA_DEATH)) {
			msg_print(Ind, "You don't know how to unleash an aura of death.");
			return;
		}
		break;
	}

	if (!p_ptr->aura[aura]) {
		if (p_ptr->anti_magic) {
			msg_format(Ind, "\377%cYour anti-magic shell disrupts your attempt.", COLOUR_AM_OWN);
			return;
		}
		if (get_skill(p_ptr, SKILL_ANTIMAGIC)) {
			msg_format(Ind, "\377%cYou don't believe in magic.", COLOUR_AM_OWN);
			return;
		}
	}

	p_ptr->aura[aura] = !p_ptr->aura[aura];

	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST
	    && aura == 1) {
		msg_print(Ind, "You cannot turn off your shivering aura granted by mist form.");
		return;
	}

	strcpy(buf, "\377sYour ");
	switch (aura) { /* up to MAX_AURAS */
	case 0: strcat(buf, "aura of fear"); break;
	case 1: strcat(buf, "shivering aura"); break;
	case 2: strcat(buf, "aura of death"); break;
	}
	strcat(buf, " is now ");
	if (p_ptr->aura[aura]) strcat(buf, "unleashed"); else strcat(buf, "suppressed");
	strcat(buf, "!");
	msg_print(Ind, buf);
}

void check_aura(int Ind, int aura) {
	char buf[80];
	player_type *p_ptr = Players[Ind];

	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST
	    && aura == 1) {
		msg_print(Ind, "\377sYour shivering aura is currently unleashed.");
		return;
	}

	strcpy(buf, "\377sYour ");
	switch (aura) { /* up to MAX_AURAS */
	case 0: strcat(buf, "aura of fear"); break;
	case 1: strcat(buf, "shivering aura"); break;
	case 2: strcat(buf, "aura of death"); break;
	}
	strcat(buf, " is currently ");
	if (p_ptr->aura[aura]) strcat(buf, "unleashed"); else strcat(buf, "suppressed");
	strcat(buf, ".");
	msg_print(Ind, buf);
}

/* determine minimum dungeon level required for a player of a particular
   character level to obtain optimum experience. */
int det_req_level(int plev) {
	if (plev < 20) return (0);
	else if (plev < 30) return (375 / (45 - plev));
	else if (plev < 50) return (650 / (56 - plev));
	else if (plev < 65) return (plev * 2);
	else if (plev < 75) return ((plev - 65) * 4 + 130);
	else return ((plev - 75) + 170);
}
/* calculate actual experience gain based on det_req_level */
s64b det_exp_level(s64b exp, int plev, int dlev) {
	int req_dlvl = det_req_level(plev);

	if (dlev < req_dlvl - 5) return 0; /* actually give zero exp for 'grey' levels? */
	if (dlev < req_dlvl) return ((exp * 2) / (2 + req_dlvl - dlev)); /* give reduced exp for 'yellow' levels */
	return (exp); /* normal exp */
}
