/* $Id$ */
/* File: skills.c */

/* Purpose: player skills */

/*
 * Copyright (c) 2001 DarkGod
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

/* added this for consistency in some (unrelated) header-inclusion,
   it IS a server file, isn't it? */
#define SERVER
#include "angband.h"

#if 0
/* Initialze the s_info array at server start */
bool init_s_info()
{
        int i;
        int order = 1;

        for (i = 0; i < MAX_SKILLS; i++)
        {
                /* Is there a skill to process here ? */
                if (skill_tree_init[i][1] > 0)
                {
                        /* Set it's father and order in the tree */
                        s_info[skill_tree_init[i][1]].father = skill_tree_init[i][0];
                        s_info[skill_tree_init[i][1]].order = order++;
                }
        }
        return FALSE;
}
#endif	// 0

/*
 * Initialize a skill with given values
 */
void init_skill(player_type *p_ptr, u32b value, s16b mod, int i) {
        p_ptr->s_info[i].base_value = value;
        p_ptr->s_info[i].value = value;
        p_ptr->s_info[i].mod = mod;
	p_ptr->s_info[i].touched = TRUE;
#if 0 //SMOOTHSKILLS
        if (s_info[i].flags1 & SKF1_HIDDEN)
		p_ptr->s_info[i].hidden = TRUE;
        else
		p_ptr->s_info[i].hidden = FALSE;
        if (s_info[i].flags1 & SKF1_DUMMY)
		p_ptr->s_info[i].dummy = TRUE;
        else
		p_ptr->s_info[i].dummy = FALSE;
#else
	p_ptr->s_info[i].flags1 = (char)(s_info[i].flags1 & 0xFF);

	/* hack: Rangers can train limited Archery skill */
	if (p_ptr->pclass == CLASS_RANGER && i == SKILL_ARCHERY)
		p_ptr->s_info[i].flags1 |= SKF1_MAX_10;
	/* hack: Druids/Vampires can't train Mimicry skill */
	if ((p_ptr->pclass == CLASS_DRUID || p_ptr->prace == RACE_VAMPIRE)
	    && i == SKILL_MIMIC)
		p_ptr->s_info[i].flags1 |= SKF1_MAX_1;
#endif
}

/*
 *
 */
s16b get_skill(player_type *p_ptr, int skill)
{
#if 0
	/* prevent breaking the +2 limit */
	int s;
	s =  (p_ptr->s_info[skill].value / SKILL_STEP);
	if (s > p_ptr->lev + 2) s = p_ptr->lev + 2;
	return s;
#else
	return (p_ptr->s_info[skill].value / SKILL_STEP);
#endif
}


/*
 *
 */
s16b get_skill_scale(player_type *p_ptr, int skill, u32b scale)
{
#if 0
	/* prevent breaking the +2 limit */
	int s;
/*	s =  ((p_ptr->s_info[skill].value * 1000) / SKILL_STEP);
	if (s > (p_ptr->lev * 1000) + 2000) s = (p_ptr->lev * 1000) + 2000;
	s = (s * SKILL_STEP) / 1000;
	return (((s / 10) * (scale * (SKILL_STEP / 10)) / (SKILL_MAX / 10)) / (SKILL_STEP / 10)); */

	/* Cleaning up this mess too... - mikaelh */
	s = p_ptr->s_info[skill].value;
	if (s > (p_ptr->lev + 2) * SKILL_STEP) s = (p_ptr->lev + 2) * SKILL_STEP;
	return ((s * scale) / SKILL_MAX);
	
#else
	/* XXX XXX XXX */
	/* return (((p_ptr->s_info[skill].value / 10) * (scale * (SKILL_STEP / 10)) /
	         (SKILL_MAX / 10)) /
	        (SKILL_STEP / 10)); */
	/* Simpler formula suggested by Potter - mikaelh */
	return ((p_ptr->s_info[skill].value * scale) / SKILL_MAX);

#endif
}

/* Allow very rough resolution of skills into very small scaled values
   (Added this for minus_... in dungeon.c - C. Blue) */
s16b get_skill_scale_fine(player_type *p_ptr, int skill, u32b scale)
{
	return (((p_ptr->s_info[skill].value * scale) / SKILL_MAX) +
		(magik(((p_ptr->s_info[skill].value * scale * 100) / SKILL_MAX) % 100) ? 1 : 0));
}

/* Will add, sub, .. */
static s32b modify_aux(s32b a, s32b b, char mod)
{
	switch (mod)
	{
		case '+':
			return (a + b);
			break;
		case '-':
			return (a - b);
			break;
		case '=':
			return (b);
			break;
		case '%':
			return ((a * b) / 100);
			break;
		default:
			return (0);
	}
}


/*
 * Gets the base value of a skill, given a race/class/...
 */
void compute_skills(player_type *p_ptr, s32b *v, s32b *m, int i)
{
	s32b value = 0, mod = 0, j;

	/***** class skills *****/

	/* find the skill mods for that class */
	for (j = 0; j < MAX_SKILLS; j++) {
		if (p_ptr->cp_ptr->skills[j].skill == i) {
			value = p_ptr->cp_ptr->skills[j].value;
			mod = p_ptr->cp_ptr->skills[j].mod;

			*v = modify_aux(*v,
					value, p_ptr->cp_ptr->skills[j].vmod);
			*m = modify_aux(*m,
					mod, p_ptr->cp_ptr->skills[j].mmod);
		}
	}

	/* Race later (b/c of its modificative nature) */
	for (j = 0; j < MAX_SKILLS; j++) {
		if (p_ptr->rp_ptr->skills[j].skill == i) {
			value = p_ptr->rp_ptr->skills[j].value;
			mod = p_ptr->rp_ptr->skills[j].mod;

			*v = modify_aux(*v,
					value, p_ptr->rp_ptr->skills[j].vmod);
			*m = modify_aux(*m,
					mod, p_ptr->rp_ptr->skills[j].mmod);
		}
	}
}

/* Display messages to the player, telling him about newly gained abilities
   from increasing a skill */
void msg_gained_abilities(int Ind, int old_value, int i) {
	player_type *p_ptr = Players[Ind];
	int new_value = get_skill_scale(p_ptr, i, 500);

//	int as = get_archery_skill(p_ptr);
//	int ws = get_weaponmastery_skill(p_ptr);

	/* Tell the player about new abilities that he gained from the skill increase */
	if (old_value == new_value) return;
	switch (i) {
	case SKILL_CLIMB:	if (new_value == 10) msg_print(Ind, "\374\377GYou learn how to climb mountains!");
				break;
	case SKILL_LEVITATE: 	if (new_value == 10) msg_print(Ind, "\374\377GYou learn how to levitate!");
				break;
	case SKILL_FREEACT:	if (new_value == 10) msg_print(Ind, "\374\377GYou learn how to resist paralysis and move freely!");
				break;
	case SKILL_RESCONF:	if (new_value == 10) msg_print(Ind, "\374\377GYou learn how to keep a focussed mind and avoid confusion!");
				break;
	case SKILL_DODGE:
		if (old_value == 0 && new_value > 0 &&
		    p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)
			msg_print(Ind, "\377oYou cannot dodge attacks while wielding a shield!");
		break;
	case SKILL_MARTIAL_ARTS:
		if (old_value == 0 && new_value > 0) {
			bool warn_takeoff = FALSE;
			/* display some warnings if an item will severely conflict with Martial Arts skill */
			if (p_ptr->inventory[INVEN_WIELD].k_idx ||
			    is_melee_weapon(p_ptr->inventory[INVEN_ARM].tval) || /* for dual-wielders */
#ifndef ENABLE_MA_BOOMERANG
			    p_ptr->inventory[INVEN_BOW].k_idx) {
#else
			    p_ptr->inventory[INVEN_BOW].tval == TV_BOW) {
#endif
#ifndef ENABLE_MA_BOOMERANG
				msg_print(Ind, "\374\377RWarning: Using any sort of weapon renders Martial Arts skill effectless.");
#else
				msg_print(Ind, "\374\377RWarning: Using any melee weapon or bow renders Martial Arts skill effectless.");
#endif
				warn_takeoff = TRUE;
			}
			if (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD) {
				msg_print(Ind, "\377RWarning: You cannot use special martial art styles with a shield!");
				warn_takeoff = TRUE;
			}
			if (warn_takeoff) msg_print(Ind, "\374\377R         Press 't' key to take off your weapons or shield.");

			/* Martial artists shouldn't get a weapon-wield warning */
			p_ptr->warning_wield = 1;
			/* also don't send any weapon-bpr related warnings since their
			   suggested remedies don't work for us as MA user */
			p_ptr->warning_bpr = 1;
			p_ptr->warning_bpr2 = 1;
			p_ptr->warning_bpr3 = 1;
		}
		if (old_value < 10 && new_value >= 10) { /* the_sandman */
//			msg_print(Ind, "\374\377GYou feel as if you could take on the world!");
			msg_print(Ind, "\374\377GYou learn to use punching techniques.");
			msg_print(Ind, "\374\377GYour melee attack speed has become faster due to your training!");
		}
		if (old_value < 20 && new_value >= 20)
			msg_print(Ind, "\374\377GYou get the hang of using kicks.");
		if (old_value < 30 && new_value >= 30)
			msg_print(Ind, "\374\377GYou get the hang of using hand side strikes.");
		if (old_value < 50 && new_value >= 50)
			msg_print(Ind, "\374\377GYou get the hang of knee-based attacks.");
		if (old_value < 70 && new_value >= 70)
			msg_print(Ind, "\374\377GYou get the hang of elbow-based attacks.");
		if (old_value < 90 && new_value >= 90)
			msg_print(Ind, "\374\377GYou get the hang of butting techniques.");
		if (old_value < 100 && new_value >= 100) {
			msg_print(Ind, "\374\377GYou learn how to fall safely!");
			msg_print(Ind, "\374\377GYour melee attack speed has become faster due to your training!");
		}
		if (old_value < 110 && new_value >= 110)
			msg_print(Ind, "\374\377GYour kicks have improved.");
		if (old_value < 130 && new_value >= 130)
			msg_print(Ind, "\374\377GYou get the hang of well-timed uppercuts.");
		if (old_value < 150 && new_value >= 150)
			msg_print(Ind, "\374\377GYou learn how to tame your fear!");
		if (old_value < 160 && new_value >= 160)
			msg_print(Ind, "\374\377GYou get the hang of difficult double-kicks.");
		if (old_value < 200 && new_value >= 200) {
			msg_print(Ind, "\374\377GYou learn to use the Cat's Claw technique.");
			msg_print(Ind, "\374\377GYou learn how to keep your mind focussed and avoid confusion!");
			msg_print(Ind, "\374\377GYour melee attack speed has become faster due to your training!");
		}
		if (old_value < 250 && new_value >= 250) {
			msg_print(Ind, "\374\377GYou learn to use jump kicks effectively.");
			msg_print(Ind, "\374\377GYou learn how to resist paralysis and move freely!");
		}
		if (old_value < 290 && new_value >= 290)
			msg_print(Ind, "\374\377GYou learn to use the Eagle's Claw technique.");
		if (old_value < 300 && new_value >= 300) {
			msg_print(Ind, "\374\377GYou learn how to swim easily!");
			msg_print(Ind, "\374\377GYour melee attack speed has become faster due to your training!");
		}
		if (old_value < 330 && new_value >= 330) {
			msg_print(Ind, "\374\377GYou get the hang of circle kicks.");
		}
/*		} if (old_value < 350 && new_value >= 350) {  <- this one is now at skill 1.000 already
			msg_print(Ind, "\374\377GYour melee attack speed has become faster due to your training!"); */
		if (old_value < 370 && new_value >= 370)
			msg_print(Ind, "\374\377GYou learn the Iron Fist technique.");
		if (old_value < 400 && new_value >= 400) {
			msg_print(Ind, "\374\377GYou learn how to climb mountains!");
			msg_print(Ind, "\374\377GYour melee attack speed has become faster due to your training!");
		}
		if (old_value < 410 && new_value >= 410)
			msg_print(Ind, "\374\377GYou get the hang of difficult flying kicks.");
		if (old_value < 450 && new_value >= 450) {
			msg_print(Ind, "\374\377GYou learn the Dragon Fist technique.");
			msg_print(Ind, "\374\377GYour melee attack speed has become faster due to your training!");
		}
		if (old_value < 480 && new_value >= 480) {
			msg_print(Ind, "\374\377GYou get the hang of effective Crushing Blows.");
			if (p_ptr->total_winner) {
				msg_print(Ind, "\374\377GYou learn the Royal Titan's Fist technique.");
				msg_print(Ind, "\374\377GYou learn the Royal Phoenix Claw technique.");
			}
		}
		if (old_value < 500 && new_value >= 500) {
			msg_print(Ind, "\374\377GYou learn the technique of Levitation!");
		/* The final +ea has been moved down from lvl 50 to lvl 1 to boost MA a little - the_sandman - moved it to 350 - C. Blue */
			msg_print(Ind, "\374\377GYour melee attack speed has become faster due to your training!");
		}
		break;
	case SKILL_STANCE:
		/* automatically upgrade currently taken stance power */
		switch (p_ptr->pclass) {
		case CLASS_WARRIOR:
			if (old_value < 50 && new_value >= 50) msg_print(Ind, "\374\377GYou learn how to enter a defensive stance (rank I). ('\377gm\377G' key)");
			if (old_value < 150 && new_value >= 150) {
				msg_print(Ind, "\374\377GYou learn how to enter defensive stance rank II.");
				if (p_ptr->combat_stance == 1) p_ptr->combat_stance_power = 1;
			}
			if (old_value < 350 && new_value >= 350) {
				msg_print(Ind, "\374\377GYou learn how to enter defensive stance rank III.");
				if (p_ptr->combat_stance == 1) p_ptr->combat_stance_power = 2;
			}
			if (old_value < 100 && new_value >= 100) msg_print(Ind, "\374\377GYou learn how to enter an offensive stance (rank I).");
			if (old_value < 200 && new_value >= 200) {
				msg_print(Ind, "\374\377GYou learn how to enter offensive stance rank II.");
				if (p_ptr->combat_stance == 2) p_ptr->combat_stance_power = 1;
			}
			if (old_value < 400 && new_value >= 400) {
				msg_print(Ind, "\374\377GYou learn how to enter offensive stance rank III.");
				if (p_ptr->combat_stance == 2) p_ptr->combat_stance_power = 2;
			}
			break;
		case CLASS_MIMIC:
			if (old_value < 100 && new_value >= 100) msg_print(Ind, "\374\377GYou learn how to enter a defensive stance (rank I). ('\377gm\377G' key)");
			if (old_value < 200 && new_value >= 200) {
				msg_print(Ind, "\374\377GYou learn how to enter defensive stance rank II.");
				if (p_ptr->combat_stance == 1) p_ptr->combat_stance_power = 1;
			}
			if (old_value < 400 && new_value >= 400) {
				msg_print(Ind, "\374\377GYou learn how to enter defensive stance rank III.");
				if (p_ptr->combat_stance == 1) p_ptr->combat_stance_power = 2;
			}
			if (old_value < 100 && new_value >= 150) msg_print(Ind, "\374\377GYou learn how to enter an offensive stance (rank I).");
			if (old_value < 200 && new_value >= 250) {
				msg_print(Ind, "\374\377GYou learn how to enter offensive stance rank II.");
				if (p_ptr->combat_stance == 2) p_ptr->combat_stance_power = 1;
			}
			if (old_value < 400 && new_value >= 400) {
				msg_print(Ind, "\374\377GYou learn how to enter offensive stance rank III.");
				if (p_ptr->combat_stance == 2) p_ptr->combat_stance_power = 2;
			}
			break;
		case CLASS_PALADIN:
#ifdef ENABLE_DEATHKNIGHT
		case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
		case CLASS_HELLKNIGHT:
#endif
			if (old_value < 50 && new_value >= 50) msg_print(Ind, "\374\377GYou learn how to enter a defensive stance (rank I). ('\377gm\377G' key)");
			if (old_value < 200 && new_value >= 200) {
				msg_print(Ind, "\374\377GYou learn how to enter defensive stance rank II.");
				if (p_ptr->combat_stance == 1) p_ptr->combat_stance_power = 1;
			}
			if (old_value < 350 && new_value >= 350) {
				msg_print(Ind, "\374\377GYou learn how to enter defensive stance rank III.");
				if (p_ptr->combat_stance == 1) p_ptr->combat_stance_power = 2;
			}
			if (old_value < 100 && new_value >= 150) msg_print(Ind, "\374\377GYou learn how to enter an offensive stance (rank I).");
			if (old_value < 200 && new_value >= 250) {
				msg_print(Ind, "\374\377GYou learn how to enter offensive stance rank II.");
				if (p_ptr->combat_stance == 2) p_ptr->combat_stance_power = 1;
			}
			if (old_value < 400 && new_value >= 400) {
				msg_print(Ind, "\374\377GYou learn how to enter offensive stance rank III.");
				if (p_ptr->combat_stance == 2) p_ptr->combat_stance_power = 2;
			}
			break;
		case CLASS_RANGER:
			if (old_value < 50 && new_value >= 100) msg_print(Ind, "\374\377GYou learn how to enter a defensive stance (rank I). ('\377gm\377G' key)");
			if (old_value < 150 && new_value >= 200) {
				msg_print(Ind, "\374\377GYou learn how to enter defensive stance rank II.");
				if (p_ptr->combat_stance == 1) p_ptr->combat_stance_power = 1;
			}
			if (old_value < 350 && new_value >= 400) {
				msg_print(Ind, "\374\377GYou learn how to enter defensive stance rank III.");
				if (p_ptr->combat_stance == 1) p_ptr->combat_stance_power = 2;
			}
			if (old_value < 100 && new_value >= 150) msg_print(Ind, "\374\377GYou learn how to enter an offensive stance (rank I).");
			if (old_value < 200 && new_value >= 250) {
				msg_print(Ind, "\374\377GYou learn how to enter offensive stance rank II.");
				if (p_ptr->combat_stance == 2) p_ptr->combat_stance_power = 1;
			}
			if (old_value < 400 && new_value >= 400) {
				msg_print(Ind, "\374\377GYou learn how to enter offensive stance rank III.");
				if (p_ptr->combat_stance == 2) p_ptr->combat_stance_power = 2;
			}
			break;
		}

		/* learn royal stances at 45+ if winner */
		if (old_value < 450 && new_value >= 450 && p_ptr->total_winner) {
			msg_print(Ind, "\374\377GYou learn how to enter Royal Rank combat stances."); 
			if (p_ptr->combat_stance) p_ptr->combat_stance_power = 3;
		}
		break;
	case SKILL_ARCHERY:
		if (old_value < 40 && new_value >= 40)
			msg_print(Ind, "\374\377GYou learn the shooting technique 'Flare Missile'! (press '\377gm\377G')");
		if (old_value < 80 && new_value >= 80)
			msg_print(Ind, "\374\377GYou learn the shooting technique 'Precision Shot'!");
		if (old_value < 100 && new_value >= 100)
			msg_print(Ind, "\374\377GYou learn how to craft ammunition from bones and rubble!");
		if (old_value < 110 && new_value >= 110)
			msg_print(Ind, "\374\377GYou got better at recognizing the power of unknown ranged weapons and ammo!");
		if (old_value < 160 && new_value >= 160)
			msg_print(Ind, "\374\377GYou learn the shooting technique 'Double Shot'!");
		if (old_value < 200 && new_value >= 200)
			msg_print(Ind, "\374\377GYour ability to craft ammunition improved remarkably!");
		if (old_value < 250 && new_value >= 250)
			msg_print(Ind, "\374\377GYou learn the shooting technique 'Barrage'!");
//		if (old_value < 500 && new_value >= 500)
//			msg_print(Ind, "\374\377GYour general shooting power gains extra might due to your training!");
		break;
	case SKILL_COMBAT:
		if (old_value < 110 && new_value >= 110) {
			msg_print(Ind, "\374\377GYou got better at recognizing the power of unknown weapons.");
#if 0
		} if (old_value < 310 && new_value >= 310) {
			msg_print(Ind, "\374\377GYou got better at recognizing the power of unknown ranged weapons and ammo.");
		} if (old_value < 410 && new_value >= 410) {
			msg_print(Ind, "\374\377GYou got better at recognizing the power of unknown magical items.");
#else /* more true messages: */
		} if (old_value < 310 && new_value >= 310) {
			if (get_skill(p_ptr, SKILL_ARCHERY) < 11)
				msg_print(Ind, "\374\377GYou somewhat recognize the usefulness of unknown ranged weapons and ammo.");
		} if (old_value < 410 && new_value >= 410) {
			/* message somewhat redudant with classes/other skills which also give
			   ok_curse, but seems impractical to sort out really */
			msg_print(Ind, "\374\377GYou feel able to sense curses on all types of items.");
#endif
		}
		break;
	case SKILL_MAGIC:
		if (old_value < 110 && new_value >= 110
		    && get_skill(p_ptr, SKILL_DIVINATION) < 50) { //auto-id
			msg_print(Ind, "\374\377GYou got better at recognizing the power of unknown magical items.");
		}
		break;

	case SKILL_EARTH:
		if (old_value < 400 && new_value >= 400) {
			msg_print(Ind, "\374\377GYou feel able to prevent shards of rock from striking you.");
		} if (old_value < 500 && new_value >= 500) {
			msg_print(Ind, "\374\377GYou feel able to prevent large masses of rock from striking you.");
		}
		break;
	case SKILL_AIR:
		if (old_value < 300 && new_value >= 300
		    && p_ptr->prace != RACE_YEEK && p_ptr->prace != RACE_DRACONIAN && p_ptr->fruit_bat != 1) {
			msg_print(Ind, "\374\377GYou feel light as a feather.");
		} if (old_value < 400 && new_value >= 400
		    && p_ptr->prace != RACE_VAMPIRE && p_ptr->prace != RACE_KOBOLD && (p_ptr->prace != RACE_MAIA || p_ptr->lev < 50)
		    && p_ptr->ptrait != TRAIT_GREEN && p_ptr->ptrait != TRAIT_MULTI && p_ptr->ptrait != TRAIT_SILVER) {
			msg_print(Ind, "\374\377GYou feel able to breathe within poisoned air."); //res-poison
		} if (old_value < 500 && new_value >= 500
		    && (p_ptr->prace != RACE_DRACONIAN || p_ptr->lev < 30) && p_ptr->fruit_bat != 1) {
			msg_print(Ind, "\374\377GYou feel levitating is easy.");
		}
		break;
	case SKILL_WATER:
		if (old_value < 300 && new_value >= 300
		    && p_ptr->prace != RACE_ENT && p_ptr->pclass != CLASS_RANGER) {
			msg_print(Ind, "\374\377GYou feel able to move through water easily.");
		} if (old_value < 400 && new_value >= 400
		    && p_ptr->prace != RACE_ENT) {
			msg_print(Ind, "\374\377GYou feel able to prevent water streams from striking you.");
		} if (old_value < 500 && new_value >= 500) {
			msg_print(Ind, "\374\377GYou feel able to prevent tidal waves from striking you.");
		}
		break;
	case SKILL_FIRE:
		if (old_value < 300 && new_value >= 300
		    && p_ptr->ptrait != TRAIT_CORRUPTED
		    && p_ptr->ptrait != TRAIT_RED && p_ptr->ptrait != TRAIT_MULTI && p_ptr->ptrait != TRAIT_GOLD) {
			msg_print(Ind, "\374\377GYou feel able to resist fire easily.");
		} if (old_value < 500 && new_value >= 500) {
			msg_print(Ind, "\374\377GYou feel that fire cannot harm you anymore.");
		}
		break;
	case SKILL_MANA:
		if (old_value < 400 && new_value >= 400) {
			msg_print(Ind, "\374\377GYou feel able to defend from mana attacks easily.");
		}
		break;
	case SKILL_CONVEYANCE:
		if (old_value < 400 && new_value >= 400 &&
		    get_skill(p_ptr, SKILL_UDUN) < 30) {
			msg_print(Ind, "\374\377GYou are impervious to feeble teleportation attacks.");
		}
		break;
	case SKILL_DIVINATION:
		if (old_value < 500 && new_value >= 500) {
			msg_print(Ind, "\374\377GYou find identifying items ridiculously easy.");
			identify_pack(Ind);
		}
		break;
	case SKILL_NATURE:
		if (old_value < 300 && new_value >= 300
		    && p_ptr->prace != RACE_YEEK && p_ptr->prace != RACE_ENT
		    && p_ptr->pclass != CLASS_RANGER && p_ptr->pclass != CLASS_DRUID) {
			msg_print(Ind, "\374\377GYour magic allows you to pass trees and forests easily.");
		} if (old_value < 300 && new_value >= 300
		    && p_ptr->prace != RACE_ENT && p_ptr->pclass != CLASS_RANGER) {
			msg_print(Ind, "\374\377GYour magic allows you to pass water easily.");
		} if (old_value < 400 && new_value >= 400) {
			msg_print(Ind, "\374\377GYou feel able to breathe within poisoned air.");
		}
		/* + continuous effect */
		break;
	case SKILL_MIND:
		if (old_value < 300 && new_value >= 300) {
			msg_print(Ind, "\374\377GYou feel strong against confusion and hallucinations.");
		} if (old_value < 400 && new_value >= 400
		    && p_ptr->prace != RACE_VAMPIRE) {
			msg_print(Ind, "\374\377GYou learn to keep hold of your sanity.");
		} if (old_value < 500 && new_value >= 500) {
			msg_print(Ind, "\374\377GYou learn to keep strong hold of your sanity.");
		}
		break;
	case SKILL_TEMPORAL:
		if (old_value < 500 && new_value >= 500
		    && p_ptr->prace != RACE_HIGH_ELF && p_ptr->prace != RACE_VAMPIRE) {
			msg_print(Ind, "\374\377GYou don't fear time attacks as much anymore.");
		}
		break;
	case SKILL_UDUN:
		if (old_value < 400 && new_value >= 400
		    && p_ptr->prace != RACE_VAMPIRE) {
			msg_print(Ind, "\374\377GYou have strong control over your life force.");
		}
		break;
	case SKILL_META: /* + continuous effect */
		break;
	case SKILL_HOFFENSE:
		if (old_value < 300 && new_value >= 300) {
			msg_print(Ind, "\374\377GYou fight against undead with holy wrath.");
		} if (old_value < 400 && new_value >= 400) {
			msg_print(Ind, "\374\377GYou fight against demons with holy wrath.");
		} if (old_value < 500 && new_value >= 500) {
			msg_print(Ind, "\374\377GYou fight against evil with holy fury.");
		}
		break;
	case SKILL_HDEFENSE:
		if (old_value < 300 && new_value >= 300) {
			msg_print(Ind, "\374\377GYou stand fast against undead.");
		} if (old_value < 400 && new_value >= 400) {
			msg_print(Ind, "\374\377GYou stand fast against demons.");
		} if (old_value < 500 && new_value >= 500) {
			msg_print(Ind, "\374\377GYou stand fast against evil.");
		}
		break;
	case SKILL_HCURING:
		if (old_value < 300 && new_value >= 300) {
			msg_print(Ind, "\374\377GYou feel strong against blindness and poison.");
		} if (old_value < 400 && new_value >= 400) {
			msg_print(Ind, "\374\377GYou feel strong against stun and cuts.");
		} if (old_value < 500 && new_value >= 500) {
			msg_print(Ind, "\374\377GYou feel strong against hallucination and black breath.");
			msg_print(Ind, "\374\377GYour melee attacks inflict greater damage on undead.");
			msg_print(Ind, "\374\377GYour soul escapes less quickly on death.");
		}
		/* + continuous effect */
                break;
	case SKILL_HSUPPORT:
		if (old_value < 400 && new_value >= 400) {
			msg_print(Ind, "\374\377GYou feel superior to ancient curses.");
		}
		if (old_value < 500 && new_value >= 500 && p_ptr->prace != RACE_MAIA) {
			msg_print(Ind, "\374\377GYou don't feel hunger for worldly food anymore.");
		}
		break;
#ifdef ENABLE_OCCULT /* Occult */
	case SKILL_OSHADOW:
		if (old_value < 300 && new_value >= 300) {
			msg_print(Ind, "\374\377GYou feel strong against darkness.");
		}
		break;
	case SKILL_OSPIRIT:
		if (old_value < 300 && new_value >= 300) {
			msg_print(Ind, "\374\377GYou keep strong hold of your life force.");
		} if (old_value < 400 && new_value >= 400) {
			msg_print(Ind, "\374\377GYou fight against undead with holy wrath.");
		} if (old_value < 500 && new_value >= 500) {
			msg_print(Ind, "\374\377GYour soul escapes less quickly on death.");
		}
		break;
 #ifdef ENABLE_OHERETICISM
	case SKILL_OHERETICISM:
		if (old_value < 300 && new_value >= 300
		    && p_ptr->ptrait != TRAIT_CORRUPTED
		    && p_ptr->ptrait != TRAIT_RED && p_ptr->ptrait != TRAIT_MULTI && p_ptr->ptrait != TRAIT_GOLD) {
			msg_print(Ind, "\374\377GYou feel resistant against fire.");
		}
  #if 1
		if (old_value < 450 && new_value >= 450
		    && p_ptr->ptrait != TRAIT_CHAOS) {
			msg_print(Ind, "\374\377GYou feel resistant against chaos.");
		}
  #endif
 #endif
		break;
#endif

	case SKILL_SWORD: case SKILL_AXE: case SKILL_BLUNT: case SKILL_POLEARM:
		if ((old_value < 250 && new_value >= 250) || (old_value < 500 && new_value >= 500)) {
			msg_print(Ind, "\374\377GYour melee attack speed has become faster due to your training!");
		}
		break;
	case SKILL_SLING:
		if ((old_value < 100 && new_value >= 100) || (old_value < 200 && new_value >= 200) ||
		    (old_value < 300 && new_value >= 300) || (old_value < 400 && new_value >= 400) || (old_value < 500 && new_value >= 500)) {
			msg_print(Ind, "\374\377GYour shooting speed with slings has become faster due to your training!");
		}
		if (old_value < 500 && new_value >= 500)
			msg_print(Ind, "\374\377GYour shooting power with slings gains extra might due to your training!");
		break;
	case SKILL_BOW:
		if ((old_value < 125 && new_value >= 125) || (old_value < 250 && new_value >= 250) ||
		    (old_value < 375 && new_value >= 375) || (old_value < 500 && new_value >= 500)) {
			msg_print(Ind, "\374\377GYour shooting speed with bows has become faster due to your training!");
		}
		if (old_value < 500 && new_value >= 500)
			msg_print(Ind, "\374\377GYour shooting power with bows gains extra might due to your training!");
		break;
	case SKILL_XBOW:
		if ((old_value < 125 && new_value >= 125) || (old_value < 375 && new_value >= 375)) {
			msg_print(Ind, "\374\377GYour shooting power with crossbows gains extra might due to your training!");
		}
		if ((old_value < 250 && new_value >= 250) || (old_value < 500 && new_value >= 500)) {
			msg_print(Ind, "\374\377GYour shooting speed with crossbows has become faster due to your training!");
		}
		if (old_value < 500 && new_value >= 500)
			msg_print(Ind, "\374\377GYour shooting power with crossbows gains extra might due to your training!");
		break;
/*	case SKILL_SLING:
	case SKILL_BOW:
	case SKILL_XBOW:
*/	case SKILL_BOOMERANG:
		if ((old_value < 166 && new_value >= 166) || (old_value < 333 && new_value >= 333) ||
		    (old_value < 500 && new_value >= 500)) {
			msg_print(Ind, "\374\377GYour boomerang throwing speed has become faster due to your training!");
		}
		break;
	case SKILL_AURA_FEAR: if (old_value == 0 && new_value > 0 && !(p_ptr->anti_magic || get_skill(p_ptr, SKILL_ANTIMAGIC))) p_ptr->aura[0] = TRUE; break; /* MAX_AURAS */
	case SKILL_AURA_SHIVER: if (old_value == 0 && new_value > 0 && !(p_ptr->anti_magic || get_skill(p_ptr, SKILL_ANTIMAGIC))) p_ptr->aura[1] = TRUE; break;
	case SKILL_AURA_DEATH: if (old_value == 0 && new_value > 0 && !(p_ptr->anti_magic || get_skill(p_ptr, SKILL_ANTIMAGIC))) p_ptr->aura[2] = TRUE; break;
#if 0 /* obsolete */
	case SKILL_DIG:
		if (old_value < 300 && new_value >= 300) {
			msg_print(Ind, "\374\377GYou've become much better at prospecting.");
		}
		break;
#endif
	case SKILL_HEALTH:
		if (old_value < 100 && new_value >= 100) {
			p_ptr->sanity_bar = 1;
			msg_print(Ind, "\374\377GYour sanity indicator is more detailed now. Type '/snbar' to switch.");
		} else if (old_value < 200 && new_value >= 200) {
			p_ptr->sanity_bar = 2;
			msg_print(Ind, "\374\377GYour sanity indicator is more detailed now. Type '/snbar' to switch.");
		} else if (old_value < 400 && new_value >= 400) {
			p_ptr->sanity_bar = 3;
			msg_print(Ind, "\374\377GYour sanity indicator is more detailed now. Type '/snbar' to switch.");
		}
		break;
	case SKILL_ANTIMAGIC:
		if (!new_value) break; //paranoia..
		/* turn off all magic auras */
		if (p_ptr->aura[0]) toggle_aura(Ind, 0);
		if (p_ptr->aura[1]) toggle_aura(Ind, 1);
		if (p_ptr->aura[2]) toggle_aura(Ind, 2);
		break;
	}
}

/* Hrm this can be nasty for Sorcery/Antimagic */
// void recalc_skills_theory(s16b *invest, s32b *base_val, u16b *base_mod, s32b *bonus)
static void increase_related_skills(int Ind, int i, bool quiet) {
	player_type *p_ptr = Players[Ind];
	int j;

	/* Modify related skills */
	for (j = 0; j < MAX_SKILLS; j++) {
		/* Ignore self */
		if (j == i) continue;

		/* Exclusive skills */
		if (s_info[i].action[j] == SKILL_EXCLUSIVE) {
			/* Turn it off */
			p_ptr->s_info[j].value = 0;
		}

		/* Non-exclusive skills */
		else {
			/* Save previous value */
			int old_value = get_skill_scale(p_ptr, j, 500);

			/* Increase / decrease with a % */
			s32b val = p_ptr->s_info[j].value +
				(p_ptr->s_info[j].mod * s_info[i].action[j] / 100);

			/* Skill value cannot be negative */
			if (val < 0) val = 0;

			/* It cannot exceed SKILL_MAX */
			if ((p_ptr->s_info[j].flags1 & SKF1_MAX_1) && (val >= 1000)) val = 1000;
			else if ((p_ptr->s_info[j].flags1 & SKF1_MAX_10) && (val >= 10000)) val = 10000;
			else if ((p_ptr->s_info[j].flags1 & SKF1_MAX_20) && (val >= 20000)) val = 20000;
			else if ((p_ptr->s_info[j].flags1 & SKF1_MAX_25) && (val >= 25000)) val = 25000;
			else if (val > SKILL_MAX) val = SKILL_MAX;

			/* Save the modified value */
			p_ptr->s_info[j].value = val;

			/* Update the client */
			if (!quiet) {
				calc_techniques(Ind);
				Send_skill_info(Ind, j, TRUE);
			}

			/* Take care of gained abilities */
			if (!quiet) msg_gained_abilities(Ind, old_value, j);
		}
	}
}


/*
 * Advance the skill point of the skill specified by i and
 * modify related skills
 * note that we *MUST* send back a skill_info packet
 */
void increase_skill(int Ind, int i, bool quiet)
{
	player_type *p_ptr = Players[Ind];
	int old_value;
//	int as, ws, new_value;
//	int can_regain;

	/* Sanity check - mikaelh */
	if (i < 0 || i >= MAX_SKILLS)
	{
		return;
	}

	/* No skill points to be allocated */
	if (p_ptr->skill_points <= 0) {
		if (!quiet) Send_skill_info(Ind, i, TRUE);
		return;
	}

	/* The skill cannot be increased */
	if (p_ptr->s_info[i].mod <= 0) {
		if (!quiet) Send_skill_info(Ind, i, TRUE);
		return;
	}

	/* The skill is already maxed */
	if ((p_ptr->s_info[i].value >= SKILL_MAX) ||
	    /* Some extra ability skills don't go over 1 ('1' meaning 'learnt') */
	    ((p_ptr->s_info[i].flags1 & SKF1_MAX_1) && (p_ptr->s_info[i].value >= 1000)) ||
	    /* Hack: Archery for rangers doesn't go over 10 */
	    ((p_ptr->s_info[i].flags1 & SKF1_MAX_10) && (p_ptr->s_info[i].value >= 10000)) ||
	    /* unused: doesn't go over 20 */
	    ((p_ptr->s_info[i].flags1 & SKF1_MAX_20) && (p_ptr->s_info[i].value >= 20000)) ||
	    /* unused: doesn't go over 25 */
	    ((p_ptr->s_info[i].flags1 & SKF1_MAX_25) && (p_ptr->s_info[i].value >= 25000)))
	{
		if (!quiet) Send_skill_info(Ind, i, TRUE);
		return;
	}

	/* Cannot allocate more than player level + 2 levels */
	if ((p_ptr->s_info[i].value / SKILL_STEP) >= p_ptr->lev + 2)  /* <- this allows limit breaks at very high step values  -- handled in GET_SKILL now! */
//	if ((((p_ptr->s_info[i].value + p_ptr->s_info[i].mod) * 10) / SKILL_STEP) > (p_ptr->lev * 10) + 20)  /* <- this often doesn't allow proper increase to +2 at high step values */
	{
		if (!quiet) Send_skill_info(Ind, i, TRUE);
		return;
	}

	/* Spend an unallocated skill point */
	p_ptr->skill_points--;

	/* Save previous value for telling the player about newly gained
	   abilities later on. Round down extra-safely (just paranoia). */
//	old_value = (p_ptr->s_info[i].value - (p_ptr->s_info[i].value % SKILL_STEP)) / SKILL_STEP;
	/*multiply by 10, so we get +1 digit*/
	old_value = (p_ptr->s_info[i].value - (p_ptr->s_info[i].value % (SKILL_STEP / 10))) / (SKILL_STEP / 10);

	/* Increase the skill */
	p_ptr->s_info[i].value += p_ptr->s_info[i].mod;

	/* extra abilities cap at 1000 */
	if ((p_ptr->s_info[i].flags1 & SKF1_MAX_1) && (p_ptr->s_info[i].value >= 1000))
		p_ptr->s_info[i].value = 1000;
	/* hack: archery for rangers caps at 10000 */
	else if ((p_ptr->s_info[i].flags1 & SKF1_MAX_10) && (p_ptr->s_info[i].value >= 10000))
		p_ptr->s_info[i].value = 10000;
	/* unused: */
	else if ((p_ptr->s_info[i].flags1 & SKF1_MAX_20) && (p_ptr->s_info[i].value >= 20000))
		p_ptr->s_info[i].value = 20000;
	/* unused: */
	else if ((p_ptr->s_info[i].flags1 & SKF1_MAX_25) && (p_ptr->s_info[i].value >= 25000))
		p_ptr->s_info[i].value = 25000;
	/* cap at SKILL_MAX */
	else if (p_ptr->s_info[i].value >= SKILL_MAX) p_ptr->s_info[i].value = SKILL_MAX;

	/* Increase the skill */
	increase_related_skills(Ind, i, quiet);

	/* Update the client */
	if (!quiet) {
		calc_techniques(Ind);
		Send_skill_info(Ind, i, TRUE);
	}

	/* XXX updating is delayed till player leaves the skill screen */
	p_ptr->update |= (PU_SKILL_MOD);
	
	/* also update 'C' character screen live! */
	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA);
	p_ptr->redraw |= (PR_SKILLS | PR_PLUSSES | PR_SANITY);

	/* Take care of gained abilities */
	if (!quiet) msg_gained_abilities(Ind, old_value, i);
}
/*
 * Given the name of a skill, returns skill index or -1 if no
 * such skill is found
 */
s16b find_skill(cptr name)
{
	u16b i;

	/* Scan skill list */
//	for (i = 1; i < max_s_idx; i++)
	for (i = 0; i < MAX_SKILLS; i++)
	{
		/* The name matches */
		if (streq(s_info[i].name + s_name, name)) return (i);
	}

	/* No match found */
	return (-1);
}

/* return value by which a skill was auto-modified by related skills
   instead of real point distribution by the player */
static s32b modified_by_related(player_type *p_ptr, int i) {
	int j, points;
	s32b val = 0, jv, jm;
	
	for (j = 0; j < MAX_SKILLS; j++) {
		/* Ignore self */
		if (j == i) continue;

		/* Ignore skills that aren't modified by related skills */
		if ((s_info[j].action[i] != SKILL_EXCLUSIVE) &&
		    s_info[j].action[i] &&
		    /* hack against oscillation: only take care of increase atm '> 0': */
		    (s_info[j].action[i] > 0)) { 
			/* calc 'manual increase' of the increasing skill by excluding base value 'jv' */
			jv = 0; jm = 0;
			compute_skills(p_ptr, &jv, &jm, j);
			/* calc amount of points the user spent into the increasing skill */
//			if (jm)
			if (p_ptr->s_info[j].mod)
//				points = (p_ptr->s_info[j].value - p_ptr->s_info[j].base_value - modified_by_related(p_ptr, j)) / jm;
				points = (p_ptr->s_info[j].value - p_ptr->s_info[j].base_value - modified_by_related(p_ptr, j)) / p_ptr->s_info[j].mod;
			else
				points = 0;
			/* calc and stack up amount of the increase that skill 'i' experienced by that */
			val += ((p_ptr->s_info[i].mod * s_info[j].action[i] / 100) * points);

			/* Skill value cannot be negative */
			if (val < 0) val = 0;
			/* It cannot exceed SKILL_MAX */
			if (val > SKILL_MAX) val = SKILL_MAX;
		}
	}

	return (val);
}

/* Free all points spent into a certain skill and make them available again.
   Note that it is actually impossible under current skill functionality
   to reconstruct the exact points the user spent on all skills in all cases.
   Reasons:
    - If the skill in question for example is a skill that gets
    increased as a result of increasing another skill, we won't
    know how many points the player actually invested into it if
    the skill is maxed out.
    - Similarly, we wont know how many points to subtract from
    related skills when we erase a specific skill, if that related
    skill is maxed, because the player might have spent more points
    into it than would be required to max it.
    - Even if we start analyzing all skills, we won't know which one
    got more 'overspent' points as soon as there are two ore more
    maximized skills in question.
   For a complete respec function see respec_skills() below.
   This function is still possible though, if we agree that it may
   'optimize' the skill point spending for the users. It can't return
   more points than the user actually could spend in an 'ideal skill chart'.
   So it wouldn't hurt really, if we allow this to correct any waste
   of skill points the user did. - C. Blue
   update_skills: Change base values and base mods to up-to-date values. */
void respec_skill(int Ind, int i, bool update_skill, bool polymorph) {
	player_type *p_ptr = Players[Ind];
	int j;
	s32b v = 0, m = 0; /* base starting skill value, skill modifier */
	s32b jv, jm;
	s32b real_user_increase = 0, val;
	int spent_points;

	compute_skills(p_ptr, &v, &m, i);

	/* Calculate amount of 'manual increase' to this skill.
	   Manual meaning either direct increases or indirect
	   increases from related skills. */
	real_user_increase = p_ptr->s_info[i].value - p_ptr->s_info[i].base_value;
	/* Now get rid of amount of indirect increases we got
	   from related skills. */
	real_user_increase = real_user_increase - modified_by_related(p_ptr, i); /* recursive */
	/* catch overflow cap (example: skill starts at 1.000,
	   but is modified by a theoretical total of 50.000 from
	   various other skills -> would end at -1.000) */
	if (real_user_increase < 0) real_user_increase = 0;
	/* calculate skill points spent into this skill */
//	spent_points = real_user_increase / m;
	if (p_ptr->s_info[i].mod)
		spent_points = real_user_increase / p_ptr->s_info[i].mod;
	else
		spent_points = 0;

	/* modify related skills */
	for (j = 0; j < MAX_SKILLS; j++) {
		/* Ignore self */
		if (j == i) continue;

		/* Exclusive skills */
		if (s_info[i].action[j] == SKILL_EXCLUSIVE) {
			jv = 0; jm = 0;
			compute_skills(p_ptr, &jv, &jm, j);
			/* Turn it on again (!) */
//			p_ptr->s_info[j].value = jv;
			p_ptr->s_info[j].value = p_ptr->s_info[j].base_value;
		} else { /* Non-exclusive skills */
			/* Decrease with a % */
			val = p_ptr->s_info[j].value -
			    ((p_ptr->s_info[j].mod * s_info[i].action[j] / 100) * spent_points);

			/* Skill value cannot be negative */
			if (val < 0) val = 0;
			/* It cannot exceed SKILL_MAX */
			if ((p_ptr->s_info[j].flags1 & SKF1_MAX_1) && (val > 1000)) val = 1000;
			else if ((p_ptr->s_info[j].flags1 & SKF1_MAX_10) && (val > 10000)) val = 10000;
			else if ((p_ptr->s_info[j].flags1 & SKF1_MAX_20) && (val > 10000)) val = 20000;
			else if ((p_ptr->s_info[j].flags1 & SKF1_MAX_25) && (val > 10000)) val = 25000;
			else if (val > SKILL_MAX) val = SKILL_MAX;

			/* Save the modified value */
			p_ptr->s_info[j].value = val;

			/* Update the client */
			Send_skill_info(Ind, j, FALSE);
		}
	}

	/* Remove the points, ie set skill to its starting base value again
	   and just add synergy points it got from other related skills */
	p_ptr->s_info[i].value -= real_user_increase;
	/* Free the points, making them available */
	p_ptr->skill_points += spent_points;

	if (update_skill) { /* hack: fix skill.mod */
		p_ptr->s_info[i].mod = m;
		p_ptr->s_info[i].value = v;
		p_ptr->s_info[i].base_value = v;
		//new (for occult): also fix flags (DUMMY flag for SKILL_SCHOOL_OCCULT)
		p_ptr->s_info[i].flags1 = (char)(s_info[i].flags1 & 0xFF);
	}
	if (p_ptr->fruit_bat == 1) fruit_bat_skills(p_ptr);

	/* in case we changed mimicry skill */
	if (polymorph) do_mimic_change(Ind, 0, TRUE);

	/* hack - fix SKILL_STANCE skill */
	if (i == SKILL_STANCE && get_skill(p_ptr, SKILL_STANCE)) {
		if (p_ptr->max_plv < 50) p_ptr->s_info[SKILL_STANCE].value = p_ptr->max_plv * 1000;
		else p_ptr->s_info[SKILL_STANCE].value = 50000;
		/* Update the client */
		Send_skill_info(Ind, SKILL_STANCE, TRUE);
		Send_skill_info(Ind, SKILL_TECHNIQUE, TRUE);
	}

	/* Update the client */
	calc_techniques(Ind);
	Send_skill_info(Ind, i, FALSE);
	/* XXX updating is delayed till player leaves the skill screen */
	p_ptr->update |= (PU_SKILL_MOD);
	/* also update 'C' character screen live! */
	p_ptr->update |= (PU_BONUS);
	p_ptr->redraw |= (PR_SKILLS | PR_PLUSSES);

	/* Discard old "save point" for /undoskills command */
	memcpy(p_ptr->s_info_old, p_ptr->s_info, MAX_SKILLS * sizeof(skill_player));
	p_ptr->skill_points_old = p_ptr->skill_points;
	p_ptr->reskill_possible = TRUE;
}

/* Complete skill-chart reset (full respec) - C. Blue
   update_skill: Change base value and base mod to up-to-date values. */
void respec_skills(int Ind, bool update_skills) {
	player_type *p_ptr = Players[Ind];
	int i;
	s32b v, m; /* base starting skill value, skill modifier */

	/* Remove the points, ie set skills to its starting base values again */
	for (i = 0; i < MAX_SKILLS; i++) {
		v = 0; m = 0;
		if (update_skills) {
			compute_skills(p_ptr, &v, &m, i);
			p_ptr->s_info[i].base_value = v;
			p_ptr->s_info[i].value = v;
			p_ptr->s_info[i].mod = m;
			//new (for occult): also fix flags (DUMMY flag for SKILL_SCHOOL_OCCULT)
			p_ptr->s_info[i].flags1 = (char)(s_info[i].flags1 & 0xFF);
		} else {
			p_ptr->s_info[i].value = p_ptr->s_info[i].base_value;
		}
	}
	if (p_ptr->fruit_bat == 1) fruit_bat_skills(p_ptr);
	/* Update the client */
	for (i = 0; i < MAX_SKILLS; i++) Send_skill_info(Ind, i, FALSE);

	/* Calculate amount of skill points that should be
	    available to the player depending on his level */
	p_ptr->skill_points = (p_ptr->max_plv - 1) * SKILL_NB_BASE;

	/* in case we changed mimicry skill */
	do_mimic_change(Ind, 0, TRUE);

	/* hack - fix SKILL_STANCE skill */
	if (get_skill(p_ptr, SKILL_STANCE)) {
		if (p_ptr->max_plv < 50) p_ptr->s_info[SKILL_STANCE].value = p_ptr->max_plv * 1000;
		else p_ptr->s_info[SKILL_STANCE].value = 50000;
		/* Update the client */
		Send_skill_info(Ind, SKILL_STANCE, TRUE);
		Send_skill_info(Ind, SKILL_TECHNIQUE, TRUE);
	}

	/* Update the client */
	calc_techniques(Ind);
	/* XXX updating is delayed till player leaves the skill screen */
	p_ptr->update |= (PU_SKILL_MOD);
	/* also update 'C' character screen live! */
	p_ptr->update |= (PU_BONUS);
	p_ptr->redraw |= (PR_SKILLS | PR_PLUSSES);

	/* Discard old "save point" for /undoskills command */
	memcpy(p_ptr->s_info_old, p_ptr->s_info, MAX_SKILLS * sizeof(skill_player));
	p_ptr->skill_points_old = p_ptr->skill_points;
	p_ptr->reskill_possible = TRUE;
}

/* return amount of points that were invested into a skill */
int invested_skill_points(int Ind, int i) {
	player_type *p_ptr = Players[Ind];
	s32b v = 0, m = 0; /* base starting skill value, skill modifier */
	s32b real_user_increase = 0;

	compute_skills(p_ptr, &v, &m, i);

	/* Calculate amount of 'manual increase' to this skill.
	   Manual meaning either direct increases or indirect
	   increases from related skills. */
	real_user_increase = p_ptr->s_info[i].value - p_ptr->s_info[i].base_value;
	/* Now get rid of amount of indirect increases we got
	   from related skills. */
	real_user_increase = real_user_increase - modified_by_related(p_ptr, i); /* recursive */
	/* catch overflow cap (example: skill starts at 1.000,
	   but is modified by a theoretical total of 50.000 from
	   various other skills -> would end at -1.000) */
	if (real_user_increase < 0) real_user_increase = 0;
	/* calculate skill points spent into this skill */
	return(real_user_increase / m);
}

/* Disable skills that fruit bats could not put to use anyway.
   This could be moved to tables.c to form a new type, aka body mod skills
   like racial/class skills currently do (applied by compute_skills()). */
void fruit_bat_skills(player_type *p_ptr) {
	p_ptr->s_info[SKILL_MASTERY].value = p_ptr->s_info[SKILL_MASTERY].mod = 0;
	p_ptr->s_info[SKILL_SWORD].value = p_ptr->s_info[SKILL_SWORD].mod = 0;
	p_ptr->s_info[SKILL_CRITS].value = p_ptr->s_info[SKILL_CRITS].mod = 0;
	p_ptr->s_info[SKILL_BLUNT].value = p_ptr->s_info[SKILL_BLUNT].mod = 0;
	p_ptr->s_info[SKILL_AXE].value = p_ptr->s_info[SKILL_AXE].mod = 0;
	p_ptr->s_info[SKILL_POLEARM].value = p_ptr->s_info[SKILL_POLEARM].mod = 0;
	p_ptr->s_info[SKILL_DUAL].value = p_ptr->s_info[SKILL_DUAL].mod = 0;

	p_ptr->s_info[SKILL_ARCHERY].value = p_ptr->s_info[SKILL_ARCHERY].mod = 0;
	p_ptr->s_info[SKILL_BOW].value = p_ptr->s_info[SKILL_BOW].mod = 0;
	p_ptr->s_info[SKILL_XBOW].value = p_ptr->s_info[SKILL_XBOW].mod = 0;
	p_ptr->s_info[SKILL_SLING].value = p_ptr->s_info[SKILL_SLING].mod = 0;
	p_ptr->s_info[SKILL_BOOMERANG].value = p_ptr->s_info[SKILL_BOOMERANG].mod = 0;
}
