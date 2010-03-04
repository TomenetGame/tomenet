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
void init_skill(player_type *p_ptr, u32b value, s16b mod, int i)
{
        p_ptr->s_info[i].value = value;
        p_ptr->s_info[i].mod = mod;
        if (s_info[i].flags1 & SKF1_HIDDEN)
		p_ptr->s_info[i].hidden = TRUE;
        else
		p_ptr->s_info[i].hidden = FALSE;
	p_ptr->s_info[i].touched = TRUE;
        if (s_info[i].flags1 & SKF1_DUMMY)
		p_ptr->s_info[i].dummy = TRUE;
        else
		p_ptr->s_info[i].dummy = FALSE;
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
	for (j = 0; j < MAX_SKILLS; j++)
	{
		if (p_ptr->cp_ptr->skills[j].skill == i)
		{
			value = p_ptr->cp_ptr->skills[j].value;
			mod = p_ptr->cp_ptr->skills[j].mod;

			*v = modify_aux(*v,
					value, p_ptr->cp_ptr->skills[j].vmod);
			*m = modify_aux(*m,
					mod, p_ptr->cp_ptr->skills[j].mmod);
		}
	}

	/* Race later (b/c of its modificative nature) */
	for (j = 0; j < MAX_SKILLS; j++)
	{
		if (p_ptr->rp_ptr->skills[j].skill == i)
		{
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
	int m;

//	int as = get_archery_skill(p_ptr);
//	int ws = get_weaponmastery_skill(p_ptr);

	/* Tell the player about new abilities that he gained from the skill increase */
	if (old_value == new_value) return;
	switch(i) {
	case SKILL_CLIMB:	if (new_value == 10) msg_print(Ind, "\374\377GYou learn how to climb mountains!");
				break;
	case SKILL_FLY: 	if (new_value == 10) msg_print(Ind, "\374\377GYou learn how to fly!");
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
		if (old_value == 0 && new_value > 0 &&
		    p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)
			msg_print(Ind, "\377oYou cannot use special martial art styles with a shield!");
		if (old_value < 10 && new_value >= 10) { /* the_sandman */
//			msg_print(Ind, "\374\377GYou feel as if you could take on the world!");
			msg_print(Ind, "\374\377GYou learn to use punching techniques.");
			msg_print(Ind, "\374\377GYour attack speed has become faster due to your training!");
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
			msg_print(Ind, "\374\377GYour attack speed has become faster due to your training!");
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
			msg_print(Ind, "\374\377GYour attack speed has become faster due to your training!");
		}
		if (old_value < 250 && new_value >= 250) {
			msg_print(Ind, "\374\377GYou learn to use jump kicks effectively.");
			msg_print(Ind, "\374\377GYou learn how to resist paralysis and move freely!");
		}
		if (old_value < 290 && new_value >= 290)
			msg_print(Ind, "\374\377GYou learn to use the Eagle's Claw technique.");
		if (old_value < 300 && new_value >= 300) {
			msg_print(Ind, "\374\377GYou learn how to swim easily!");
			msg_print(Ind, "\374\377GYour attack speed has become faster due to your training!");
		}
		if (old_value < 330 && new_value >= 330) {
			msg_print(Ind, "\374\377GYou get the hang of circle kicks.");
		}
/*		} if (old_value < 350 && new_value >= 350) {  <- this one is now at skill 1.000 already
			msg_print(Ind, "\374\377GYour attack speed has become faster due to your training!"); */
		if (old_value < 370 && new_value >= 370)
			msg_print(Ind, "\374\377GYou learn the Iron Fist technique.");
		if (old_value < 400 && new_value >= 400) {
			msg_print(Ind, "\374\377GYou learn how to climb mountains!");
			msg_print(Ind, "\374\377GYour attack speed has become faster due to your training!");
		}
		if (old_value < 410 && new_value >= 410)
			msg_print(Ind, "\374\377GYou get the hang of difficult flying kicks.");
		if (old_value < 450 && new_value >= 450) {
			msg_print(Ind, "\374\377GYou learn the Dragon Fist technique.");
			msg_print(Ind, "\374\377GYour attack speed has become faster due to your training!");
		}
		if (old_value < 480 && new_value >= 480) {
			msg_print(Ind, "\374\377GYou get the hang of effective Crushing Blows.");
			if (p_ptr->total_winner) {
				msg_print(Ind, "\374\377GYou learn the Royal Titan's Fist technique.");
				msg_print(Ind, "\374\377GYou learn the Royal Phoenix Claw technique.");
			}
		}
		if (old_value < 500 && new_value >= 500) {
			msg_print(Ind, "\374\377GYou learn the technique of flying!");
		/* The final +ea has been moved down from lvl 50 to lvl 1 to boost MA a little - the_sandman - moved it to 350 - C. Blue */
			msg_print(Ind, "\374\377GYour attack speed has become faster due to your training!");
		}
		break;
	case SKILL_STANCE:
		/* automatically upgrade currently taken stance power */
		switch (p_ptr->pclass) {
		case CLASS_WARRIOR:
			m = 0;
		        if (old_value < 50 && new_value >= 50) msg_print(Ind, "\374\377GYou learn how to enter a defensive stance (rank I).");
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
			m = 11;
			if (old_value < 100 && new_value >= 100) msg_print(Ind, "\374\377GYou learn how to enter a defensive stance (rank I).");
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
			m = 7;
			if (old_value < 50 && new_value >= 50) msg_print(Ind, "\374\377GYou learn how to enter a defensive stance (rank I).");
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
			m = 4;
			if (old_value < 50 && new_value >= 100) msg_print(Ind, "\374\377GYou learn how to enter a defensive stance (rank I).");
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
		default:
			m = 20; /* (for all other classes in theory, if they could learn this skill) */
		}
		if (old_value < 450 && new_value >= 450 && p_ptr->total_winner) {
			msg_print(Ind, "\374\377GYou learn how to enter Royal Rank combat stances."); 
		        if (p_ptr->combat_stance) p_ptr->combat_stance_power = 3;
		}
		if (old_value < 40 + m * 10 && new_value >= 40 + m * 10)
			msg_print(Ind, "\374\377GYou learn the fighting technique 'Sprint'!");
		if (old_value < 90 + m * 10 && new_value >= 90 + m * 10)
			msg_print(Ind, "\374\377GYou learn the fighting technique 'Taunt'");
		if (old_value < 160 + m * 10 && new_value >= 160 + m * 10)
			msg_print(Ind, "\374\377GYou learn the fighting technique 'Spin'!");
		if (old_value < 330 + m * 10 && new_value >= 330 + m * 10)
			msg_print(Ind, "\374\377GYou learn the fighting technique 'Berserk'!");
		break;
	case SKILL_ARCHERY:
		if (old_value < 40 && new_value >= 40)
			msg_print(Ind, "\374\377GYou learn the shooting technique 'Flare'!");
		if (old_value < 80 && new_value >= 80)
			msg_print(Ind, "\374\377GYou learn the shooting technique 'Precision shot'!");
		if (old_value < 100 && new_value >= 100)
			msg_print(Ind, "\374\377GYou learn how to create ammunition from bones and rubble!");
		if (old_value < 110 && new_value >= 110)
			msg_print(Ind, "\374\377GYou got better at recognizing the power of unknown ranged weapons!");
		if (old_value < 160 && new_value >= 160)
			msg_print(Ind, "\374\377GYou learn the shooting technique 'Double-shot'!");
		if (old_value < 200 && new_value >= 200)
			msg_print(Ind, "\374\377GYour ability to create ammunition improved remarkably!");
		if (old_value < 250 && new_value >= 250)
			msg_print(Ind, "\374\377GYou learn the shooting technique 'Barrage'!");
		if (old_value < 500 && new_value >= 500)
			msg_print(Ind, "\374\377GYour general shooting power gains extra might due to your training!");
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
			msg_print(Ind, "\374\377GYou somewhat recognize the usefulness of unknown ranged weapons and ammo.");
		} if (old_value < 410 && new_value >= 410) {
			msg_print(Ind, "\374\377GYou are able to feel curses on magical items.");
#endif
		}
		break;
	case SKILL_MAGIC:
		if (old_value < 110 && new_value >= 110) msg_print(Ind, "\374\377GYou got better at recognizing the power of unknown magical items.");
		break;
	case SKILL_EARTH:
		if (old_value < 300 && new_value >= 300) {
			msg_print(Ind, "\374\377GYou feel able to prevent shards of rock from striking you.");
		} if (old_value < 450 && new_value >= 450) {
			msg_print(Ind, "\374\377GYou feel able to prevent large masses of rock from striking you.");
		}
		break;
	case SKILL_AIR:
                if (old_value < 300 && new_value >= 300) {
                        msg_print(Ind, "\374\377GYou feel light as a feather.");
                } if (old_value < 400 && new_value >= 400) {
                        msg_print(Ind, "\374\377GYou feel able to breathe within poisoned air.");
                } if (old_value < 500 && new_value >= 500) {
                        msg_print(Ind, "\374\377GYou feel flying is easy.");
                }
                break;
	case SKILL_WATER:
                if (old_value < 300 && new_value >= 300) {
                        msg_print(Ind, "\374\377GYou feel able to prevent water streams from striking you.");
                } if (old_value < 400 && new_value >= 400) {
                        msg_print(Ind, "\374\377GYou feel able to move through water easily.");
                } if (old_value < 500 && new_value >= 500) {
                        msg_print(Ind, "\374\377GYou feel able to prevent tidal waves from striking you.");
                }
                break;
	case SKILL_FIRE:
                if (old_value < 300 && new_value >= 300) {
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
                if (old_value < 300 && new_value >= 300 &&
            	    get_skill(p_ptr, SKILL_UDUN) < 30) {
                        msg_print(Ind, "\374\377GYou are impervious to feeble teleportation attacks.");
                }
                break;
	case SKILL_DIVINATION:
                if (old_value < 500 && new_value >= 500) {
                        msg_print(Ind, "\374\377GYou find identifying items ridiculously easy.");
                }
                break;
	case SKILL_NATURE:
                if (old_value < 300 && new_value >= 300) {
                        msg_print(Ind, "\374\377GYour magic allows you to pass trees and forests easily.");
                } if (old_value < 400 && new_value >= 400) {
                        msg_print(Ind, "\374\377GYour magic allows you to pass water easily.");
                }
		/* + continuous effect */
                break;
	case SKILL_MIND:
                if (old_value < 300 && new_value >= 300) {
                        msg_print(Ind, "\374\377GYou feel strong against confusion and hallucinations.");
                } if (old_value < 400 && new_value >= 400) {
                        msg_print(Ind, "\374\377GYou learn to keep hold of your sanity.");
                } if (old_value < 500 && new_value >= 500) {
                        msg_print(Ind, "\374\377GYou learn to keep strong hold of your sanity.");
                }
                break;
	case SKILL_TEMPORAL:
                if (old_value < 500 && new_value >= 500) {
                        msg_print(Ind, "\374\377GYou don't fear time attacks as much anymore.");
                }
                break;
	case SKILL_UDUN:
                if (old_value < 300 && new_value >= 300 &&
            	    get_skill(p_ptr, SKILL_CONVEYANCE) < 30) {
                        msg_print(Ind, "\374\377GYou are impervious to feeble teleportation attacks.");
                } if (old_value < 400 && new_value >= 400) {
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
                }
		/* + continuous effect */
                break;
	case SKILL_HSUPPORT:
                if (old_value < 400 && new_value >= 400) {
                        msg_print(Ind, "\374\377GYou don't feel hunger for worldly food anymore.");
                } if (old_value < 500 && new_value >= 500) {
                        msg_print(Ind, "\374\377GYou feel superior to ancient curses.");
                }
                break;
	case SKILL_SWORD: case SKILL_AXE: case SKILL_BLUNT: case SKILL_POLEARM:
		if ((old_value < 250 && new_value >= 250) || (old_value < 500 && new_value >= 500)) {
			msg_print(Ind, "\374\377GYour attack speed has become faster due to your training!");
		}
		break;
	case SKILL_SLING:
		if ((old_value < 100 && new_value >= 100) || (old_value < 200 && new_value >= 200) ||
		    (old_value < 300 && new_value >= 300) || (old_value < 400 && new_value >= 400) || (old_value < 500 && new_value >= 500)) {
			msg_print(Ind, "\374\377GYour shooting speed has become faster due to your training!");
		}
		break;
	case SKILL_BOW:
		if ((old_value < 125 && new_value >= 125) || (old_value < 250 && new_value >= 250) ||
		    (old_value < 375 && new_value >= 375) || (old_value < 500 && new_value >= 500)) {
			msg_print(Ind, "\374\377GYour shooting speed has become faster due to your training!");
		}
		break;
	case SKILL_XBOW:
		if ((old_value < 250 && new_value >= 250) || (old_value < 500 && new_value >= 500)) {
			msg_print(Ind, "\374\377GYour shooting speed has become faster due to your training!");
			msg_print(Ind, "\374\377GYour general shooting power gains extra might due to your training!");
		}
		break;
/*	case SKILL_SLING:
	case SKILL_BOW:
	case SKILL_XBOW:
*/	case SKILL_BOOMERANG:
		if ((old_value < 166 && new_value >= 166) || (old_value < 333 && new_value >= 333) ||
		    (old_value < 500 && new_value >= 500)) {
			msg_print(Ind, "\374\377GYour shooting speed has become faster due to your training!");
		}
		break; 
#ifdef ENABLE_RUNEMASTER
	case SKILL_RUNEMASTERY:
		if (old_value < RSAFE_BOLT * 10 && new_value >= RSAFE_BOLT * 10) {
		    msg_print(Ind, "\374\377GYou are able to cast bolt rune spells without breaking the runes!");
		} else if (old_value < RSAFE_BEAM * 10 && new_value >= RSAFE_BEAM * 10) {
		    msg_print(Ind, "\374\377GYou are able to cast beam rune spells without breaking the runes!");
		} else if (old_value < RSAFE_BALL * 10 && new_value >= RSAFE_BALL * 10) {
		    msg_print(Ind, "\374\377GYou are able to cast ball rune spells without breaking the runes!");
		} else if (old_value < RSAFE_CLOUD * 10 && new_value >= RSAFE_CLOUD * 10) {
		    msg_print(Ind, "\374\377GYou are able to cast cloud rune spells without breaking the runes!");
		}
 #ifdef ALTERNATE_DMG
		if (old_value < RBARRIER * 10 && new_value >= RBARRIER * 10) {
		    msg_print(Ind, "\374\377GYou feel your potential unleashed.");
		}
 #endif /*ALTERNATE_DMG*/
#endif
	case SKILL_AURA_FEAR: if (old_value == 0 && new_value > 0) p_ptr->aura[0] = TRUE; break; /* MAX_AURAS */
	case SKILL_AURA_SHIVER: if (old_value == 0 && new_value > 0) p_ptr->aura[1] = TRUE; break;
	case SKILL_AURA_DEATH: if (old_value == 0 && new_value > 0) p_ptr->aura[2] = TRUE; break;
	}
}

/* Hrm this can be nasty for Sorcery/Antimagic */
// void recalc_skills_theory(s16b *invest, s32b *base_val, u16b *base_mod, s32b *bonus)
static void increase_related_skills(int Ind, int i)
{
	player_type *p_ptr = Players[Ind];
	int j;

	/* Modify related skills */
	for (j = 1; j < MAX_SKILLS; j++)
	{
		/* Ignore self */
		if (j == i) continue;

		/* Exclusive skills */
		if (s_info[i].action[j] == SKILL_EXCLUSIVE)
		{
			/* Turn it off */
			p_ptr->s_info[j].value = 0;
		}

		/* Non-exclusive skills */
		else
		{
			/* Save previous value */
			int old_value = get_skill_scale(p_ptr, j, 500);

			/* Increase / decrease with a % */
			s32b val = p_ptr->s_info[j].value +
				(p_ptr->s_info[j].mod * s_info[i].action[j] / 100);

			/* Skill value cannot be negative */
			if (val < 0) val = 0;

			/* It cannot exceed SKILL_MAX */
			if (val > SKILL_MAX) val = SKILL_MAX;

			/* Save the modified value */
			p_ptr->s_info[j].value = val;
			
			/* Update the client */
			calc_techniques(Ind);
			Send_skill_info(Ind, j);
			
			/* Take care of gained abilities */
			msg_gained_abilities(Ind, old_value, j);
		}
	}
}


/*
 * Advance the skill point of the skill specified by i and
 * modify related skills
 * note that we *MUST* send back a skill_info packet
 */
void increase_skill(int Ind, int i)
{
	player_type *p_ptr = Players[Ind];
	int old_value;
//	int as, ws, new_value;
//	int can_regain;

	/* No skill points to be allocated */
	if (p_ptr->skill_points <= 0)
	{
		Send_skill_info(Ind, i);
		return;
	}

	/* The skill cannot be increased */
	if (p_ptr->s_info[i].mod <= 0)
	{
		Send_skill_info(Ind, i);
		return;
	}

	/* The skill is already maxed */
	/* Some extra ability skills don't go over 1 ('1' meaning 'learnt') */
	if ((p_ptr->s_info[i].value >= SKILL_MAX) ||
	    ((p_ptr->s_info[i].value >= 1000) &&
	    ((i == SKILL_CLIMB) || (i == SKILL_FLY) ||
	    (i == SKILL_FREEACT) || (i == SKILL_RESCONF))))
	{
		Send_skill_info(Ind, i);
		return;
	}

	/* Cannot allocate more than player level + 2 levels */
	if ((p_ptr->s_info[i].value / SKILL_STEP) >= p_ptr->lev + 2)  /* <- this allows limit breaks at very high step values  -- handled in GET_SKILL now! */
//	if ((((p_ptr->s_info[i].value + p_ptr->s_info[i].mod) * 10) / SKILL_STEP) > (p_ptr->lev * 10) + 20)  /* <- this often doesn't allow proper increase to +2 at high step values */
	{
		Send_skill_info(Ind, i);
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
	if (p_ptr->s_info[i].value >= SKILL_MAX) p_ptr->s_info[i].value = SKILL_MAX;

	/* extra abilities cap at 1000 */
	if ((p_ptr->s_info[i].value >= 1000) &&
	    ((i == SKILL_CLIMB) || (i == SKILL_FLY) ||
	    (i == SKILL_FREEACT) || (i == SKILL_RESCONF)))
		p_ptr->s_info[i].value = 1000;

	/* Increase the skill */
	increase_related_skills(Ind, i);

	/* Update the client */
	calc_techniques(Ind);
	Send_skill_info(Ind, i);

	/* XXX updating is delayed till player leaves the skill screen */
	p_ptr->update |= (PU_SKILL_MOD);
	
	/* also update 'C' character screen live! */
	p_ptr->update |= (PU_BONUS);
	p_ptr->redraw |= (PR_SKILLS | PR_PLUSSES);

	/* Take care of gained abilities */
	msg_gained_abilities(Ind, old_value, i);
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
	for (i = 1; i < MAX_SKILLS; i++)
	{
		/* The name matches */
		if (streq(s_info[i].name + s_name, name)) return (i);
	}

	/* No match found */
	return (-1);
}


#if 0
/*
 *
 */
s16b get_skill(int skill)
{
	return (s_info[skill].value / SKILL_STEP);
}


/*
 *
 */
s16b get_skill_scale(int skill, u32b scale)
{
	/* XXX XXX XXX */
	return (((s_info[skill].value / 10) * (scale * (SKILL_STEP / 10)) /
	         (SKILL_MAX / 10)) /
	        (SKILL_STEP / 10));
}


/*
 *
 */
static int get_idx(int i)
{
	int j;

	for (j = 1; j < max_s_idx; j++)
	{
		if (s_info[j].order == i)
			return (j);
	}
	return (0);
}


/*
 *
 */
void init_table_aux(int table[MAX_SKILLS][2], int *idx, int father, int lev,
	bool full)
{
	int j, i;

	for (j = 1; j < max_s_idx; j++)
	{
		i = get_idx(j);

		if (s_info[i].father != father) continue;
		if (s_info[i].hidden) continue;

		table[*idx][0] = i;
		table[*idx][1] = lev;
		(*idx)++;
		if (s_info[i].dev || full) init_table_aux(table, idx, i, lev + 1, full);
	}
}


void init_table(int table[MAX_SKILLS][2], int *max, bool full)
{
	*max = 0;
	init_table_aux(table, max, -1, 0, full);
}


bool has_child(int sel)
{
	int i;

	for (i = 1; i < max_s_idx; i++)
	{
		if (s_info[i].father == sel)
			return (TRUE);
	}
	return (FALSE);
}


/*
 * Dump the skill tree
 */
void dump_skills(FILE *fff)
{
	int i, j, max = 0;
	int table[MAX_SKILLS][2];
	char buf[80];

	init_table(table, &max, TRUE);

	Term_clear();

	fprintf(fff, "\nSkills (points left: %d)", p_ptr->skill_points);

	for (j = 1; j < max; j++)
	{
		int z;

		i = table[j][0];

		if ((s_info[i].value == 0) && (i != SKILL_MISC))
		{
			if (s_info[i].mod == 0) continue;
		}

		sprintf(buf, "\n");

		for (z = 0; z < table[j][1]; z++) strcat(buf, "	 ");

		if (!has_child(i))
		{
			strcat(buf, format(" . %s", s_info[i].name + s_name));
		}
		else
		{
			strcat(buf, format(" - %s", s_info[i].name + s_name));
		}

		fprintf(fff, "%-50s%02ld.%03ld [%01d.%03d]",
		        buf, s_info[i].value / SKILL_STEP, s_info[i].value % SKILL_STEP,
		        s_info[i].mod / 1000, s_info[i].mod % 1000);
	}

	fprintf(fff, "\n");
}


/*
 * Draw the skill tree
 */
void print_skills(int table[MAX_SKILLS][2], int max, int sel, int start)
{
	int i, j;
	int wid, hgt;

	Term_clear();
	Term_get_size(&wid, &hgt);

	c_prt(TERM_WHITE, "ToME Skills Screen", 0, 28);
	c_prt((p_ptr->skill_points) ? TERM_L_BLUE : TERM_L_RED,
	      format("Skill points left: %d", p_ptr->skill_points), 1, 0);
	print_desc_aux(s_info[table[sel][0]].desc + s_text, 2, 0);

	for (j = start; j < start + (hgt - 4); j++)
	{
		byte color = TERM_WHITE;
		char deb = ' ', end = ' ';

		if (j >= max) break;

		i = table[j][0];

		if ((s_info[i].value == 0) && (i != SKILL_MISC))
		{
			if (s_info[i].mod == 0) color = TERM_L_DARK;
			else color = TERM_ORANGE;
		}
		else if ((s_info[i].value == SKILL_MAX) ||
			((s_info[i].value == 1000) &&
	    		((i == SKILL_CLIMB) || (i == SKILL_FLY) ||
			(i == SKILL_FREEACT) || (i == SKILL_RESCONF))))
			color = TERM_L_BLUE;

		if (s_info[i].hidden) color = TERM_L_RED;
		if (j == sel)
		{
			color = TERM_L_GREEN;
			deb = '[';
			end = ']';
		}
		if (!has_child(i))
		{
			c_prt(color, format("%c.%c%s", deb, end, s_info[i].name + s_name),
			      j + 4 - start, table[j][1] * 4);
		}
		else if (s_info[i].dev)
		{
			c_prt(color, format("%c-%c%s", deb, end, s_info[i].name + s_name),
			      j + 4 - start, table[j][1] * 4);
		}
		else
		{
			c_prt(color, format("%c+%c%s", deb, end, s_info[i].name + s_name),
			      j + 4 - start, table[j][1] * 4);
		}
		c_prt(color,
		      format("%02ld.%03ld [%01d.%03d]",
			         s_info[i].value / SKILL_STEP, s_info[i].value % SKILL_STEP,
			         s_info[i].mod / 1000, s_info[i].mod % 1000),
			  j + 4 - start, 60);
	}
}

/*
 * Checks various stuff to do when skills change, like new spells, ...
 */
void recalc_skills(bool init)
{
        static int thaum_level = 0;

        if (init)
        {
                thaum_level = get_skill_scale(SKILL_THAUMATURGY, 100);
        }
        else
        {
                int thaum_gain = 0;

                /* Gain thaum spells */
                while (thaum_level < get_skill_scale(SKILL_THAUMATURGY, 100))
                {
                        if (spell_num == MAX_SPELLS) break;
                        generate_spell(thaum_level);
                        thaum_level++;
                        thaum_gain++;
                }
                if (thaum_gain)
                {
                        if (thaum_gain == 1)
                                msg_print("You have gained one new thaumaturgy spell.");
                        else
                                msg_format("You have gained %d new thaumaturgy spells.", thaum_gain);
                }

		/* Update stuffs */
		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA |
			PU_POWERS | PU_SANITY | PU_BODY);

		/* Redraw various info */
		p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP);
        }
}

/*
 * Recalc the skill value
 */
void recalc_skills_theory(s16b *invest, s32b *base_val, u16b *base_mod, s32b *bonus)
{
        int i, j;

        for (i = 0; i < max_s_idx; i++)
        {
                /* Calc the base */
                s_info[i].value = base_val[i] + (base_mod[i] * invest[i]) + bonus[i];

                /* It cannot exceed SKILL_MAX */
                if (s_info[i].value > SKILL_MAX) s_info[i].value = SKILL_MAX;

                /* It cannot go below 0 */
                if (s_info[i].value < 0) s_info[i].value = 0;

                /* Modify related skills */
                for (j = 1; j < max_s_idx; j++)
                {
                        /* Ignore self */
                        if (j == i) continue;

                        /* Exclusive skills */
                        if ((s_info[i].action[j] == SKILL_EXCLUSIVE) && s_info[i].value && invest[i])
                        {
                                /* Turn it off */
                                s_info[j].value = 0;
                        }

                        /* Non-exclusive skills */
                        else
                        {
                                /* Increase / decrease with a % */
                                s32b val = s_info[j].value + (invest[i] * s_info[j].mod * s_info[i].action[j] / 100);

                                /* Skill value cannot be negative */
                                if (val < 0) val = 0;

                                /* It cannot exceed SKILL_MAX */
                                if (val > SKILL_MAX) val = SKILL_MAX;

                                /* Save the modified value */
                                s_info[j].value = val;
                        }
                }
        }
}

/*
 * Interreact with skills
 */
void do_cmd_skill()
{
	int sel = 0, start = 0, max;
	char c;
	int table[MAX_SKILLS][2];
	int i;
	int wid,hgt;
	s16b skill_points_save;
	s32b *skill_values_save;
	u16b *skill_mods_save;
	s16b *skill_rates_save;
	s16b *skill_invest;
	s32b *skill_bonus;

	recalc_skills(TRUE);

	/* Enter "icky" mode */
	character_icky = TRUE;

	/* Save the screen */
	Term_save();

	/* Allocate arrays to save skill values */
	C_MAKE(skill_values_save, MAX_SKILLS, s32b);
	C_MAKE(skill_mods_save, MAX_SKILLS, u16b);
	C_MAKE(skill_rates_save, MAX_SKILLS, s16b);
	C_MAKE(skill_invest, MAX_SKILLS, s16b);
	C_MAKE(skill_bonus, MAX_SKILLS, s32b);

	/* Save skill points */
	skill_points_save = p_ptr->skill_points;

	/* Save skill values */
	for (i = 0; i < max_s_idx; i++)
	{
		skill_type *s_ptr = &s_info[i];

		skill_values_save[i] = s_ptr->value;
		skill_mods_save[i] = s_ptr->mod;
		skill_rates_save[i] = s_ptr->rate;
		skill_invest[i] = 0;
	}

	/* Clear the screen */
	Term_clear();

	/* Initialise the skill list */
	init_table(table, &max, FALSE);

	while (TRUE)
	{
		Term_get_size(&wid, &hgt);

                /* Display list of skills */
                recalc_skills_theory(skill_invest, skill_values_save, skill_mods_save, skill_bonus);
                print_skills(table, max, sel, start);

		/* Wait for user input */
		c = inkey();

		/* Leave the skill screen */
		if (c == ESCAPE) break;

		/* Expand / collapse list of skills */
		else if (c == '\r')
		{
			if (s_info[table[sel][0]].dev) s_info[table[sel][0]].dev = FALSE;
			else s_info[table[sel][0]].dev = TRUE;
			init_table(table, &max, FALSE);
		}

		/* Next page */
		else if (c == 'n')
		{
			sel += (hgt - 4);
			if (sel >= max) sel = max - 1;
		}

		/* Previous page */
		else if (c == 'p')
		{
			sel -= (hgt - 4);
			if (sel < 0) sel = 0;
		}

		/* Select / increase a skill */
		else
		{
			int dir;

			/* Allow use of numpad / arrow keys / roguelike keys */
			dir = get_keymap_dir(c);

			/* Move cursor down */
			if (dir == 2) sel++;

			/* Move cursor up */
			if (dir == 8) sel--;

			/* Miscellaneous skills cannot be increased/decreased as a group */
			if (table[sel][0] == SKILL_MISC) continue;

			/* Increase the current skill */
			if (dir == 6) increase_skill(table[sel][0], skill_invest);

			/* Decrease the current skill */
			if (dir == 4) decrease_skill(table[sel][0], skill_invest);

			/* XXX XXX XXX Wizard mode commands outside of wizard2.c */

			/* Increase the skill */
			if (wizard && (c == '+')) skill_bonus[table[sel][0]] += SKILL_STEP;

			/* Decrease the skill */
			if (wizard && (c == '-')) skill_bonus[table[sel][0]] -= SKILL_STEP;

			/* Handle boundaries and scrolling */
			if (sel < 0) sel = max - 1;
			if (sel >= max) sel = 0;
			if (sel < start) start = sel;
			if (sel >= start + (hgt - 4)) start = sel - (hgt - 4) + 1;
		}
	}


	/* Some skill points are spent */
	if (p_ptr->skill_points != skill_points_save)
	{
		/* Flush input as we ask an important and irreversible question */
		flush();

		/* Ask we can commit the change */
		if (msg_box("Save and use these skill values? (y/n)", (int)(hgt / 2), (int)(wid / 2)) != 'y')
		{
			/* User declines -- restore the skill values before exiting */

			/* Restore skill points */
			p_ptr->skill_points = skill_points_save;

			/* Restore skill values */
			for (i = 0; i < max_s_idx; i++)
			{
				skill_type *s_ptr = &s_info[i];

				s_ptr->value = skill_values_save[i];
				s_ptr->mod = skill_mods_save[i];
				s_ptr->rate = skill_rates_save[i];
			}
		}
	}


	/* Free arrays to save skill values */
	C_FREE(skill_values_save, MAX_SKILLS, s32b);
	C_FREE(skill_mods_save, MAX_SKILLS, u16b);
	C_FREE(skill_rates_save, MAX_SKILLS, s16b);
	C_FREE(skill_invest, MAX_SKILLS, s16b);
	C_FREE(skill_bonus, MAX_SKILLS, s32b);

	/* Load the screen */
	Term_load();

	/* Leave "icky" mode */
	character_icky = FALSE;

        recalc_skills(FALSE);
}



/*
 * List of melee skills
 */
s16b melee_skills[MAX_MELEE] =
{
	SKILL_MASTERY,
	SKILL_HAND,
	SKILL_BEAR,
};
char *melee_names[MAX_MELEE] =
{
	"Weapon combat",
	"Barehanded combat",
	"Bearform combat",
};
static bool melee_bool[MAX_MELEE];
static int melee_num[MAX_MELEE];

s16b get_melee_skill()
{
	int i;

	for (i = 0; i < MAX_MELEE; i++)
	{
		if (p_ptr->melee_style == melee_skills[i])
			return (i);
	}
	return (0);
}

s16b get_melee_skills()
{
	int i, j = 0;

	for (i = 0; i < MAX_MELEE; i++)
	{
		if (s_info[melee_skills[i]].value && (!s_info[melee_skills[i]].hidden))
		{
			melee_bool[i] = TRUE;
			j++;
		}
		else
			melee_bool[i] = FALSE;
	}

	return (j);
}

static void choose_melee()
{
	int i, j, z = 0;

	character_icky = TRUE;
	Term_save();
	Term_clear();

	j = get_melee_skills();
	prt("Choose a melee style:", 0, 0);
	for (i = 0; i < MAX_MELEE; i++)
	{
		if (melee_bool[i])
		{
			prt(format("%c) %s", I2A(z), melee_names[i]), z + 1, 0);
			melee_num[z] = i;
			z++;
		}
	}

	while (TRUE)
	{
		char c = inkey();

		if (c == ESCAPE) break;
		if (A2I(c) < 0) continue;
		if (A2I(c) >= j) continue;

		for (i = 0, z = 0; z < A2I(c); i++)
			if (melee_bool[i]) z++;
		p_ptr->melee_style = melee_skills[melee_num[z]];
		for (i = INVEN_WIELD; p_ptr->body_parts[i - INVEN_WIELD] == INVEN_WIELD; i++)
			inven_takeoff(i, 255, FALSE);
		energy_use = 100;
		break;
	}

	Term_load();
	character_icky = FALSE;
}

void select_default_melee()
{
	int i;

        get_melee_skills();
        p_ptr->melee_style = SKILL_MASTERY;
	for (i = 0; i < MAX_MELEE; i++)
	{
		if (melee_bool[i])
		{
                        p_ptr->melee_style = melee_skills[i];
                        break;
		}
	}
}

/*
 * Print a batch of skills.
 */
static void print_skill_batch(int *p, int start, int max, bool mode)
{
	char buff[80];
	int i = start, j = 0;

	if (mode) prt(format("         %-31s", "Name"), 1, 20);

	for (i = start; i < (start + 20); i++)
	{
		if (i >= max) break;

                /* Hack -- only able to learn spells when learning is required */
                if ((p[i] == SKILL_LEARN) && (!must_learn_spells()))
                        continue;
                else if (p[i] > 0)
			snprintf(buff, 80, "  %c-%3d) %-30s", I2A(j), s_info[p[i]].action_mkey, s_text + s_info[p[i]].action_desc);
		else
			snprintf(buff, 80, "  %c-%3d) %-30s", I2A(j), 1, "Change melee style");

		if (mode) prt(buff, 2 + j, 20);
		j++;
	}
	if (mode) prt("", 2 + j, 20);
	prt(format("Select a skill (a-%c), @ to select by name, +/- to scroll:", I2A(j - 1)), 0, 0);
}

int do_cmd_activate_skill_aux()
{
	char which;
	int max = 0, i, start = 0;
	int ret;
	bool mode = FALSE;
	int *p;

	C_MAKE(p, max_s_idx, int);

	/* Count the max */

	/* More than 1 melee skill ? */
	if (get_melee_skills() > 1)
	{
		p[max++] = 0;
	}

	for (i = 1; i < max_s_idx; i++)
	{
		if (s_info[i].action_mkey && s_info[i].value)
		{
			int j;
			bool next = FALSE;

			/* Already got it ? */
                        for (j = 0; j < max; j++)
                        {
				if (s_info[i].action_mkey == s_info[p[j]].action_mkey)
				{
					next = TRUE;
					break;
				}
                        }
                        if (next) continue;

                        /* Hack -- only able to learn spells when learning is required */
                        if ((i == SKILL_LEARN) && (!must_learn_spells()))
                                continue;
                        p[max++] = i;
		}
	}

	if (!max)
	{
		msg_print("You dont have any activable skills.");
		return -1;
	}

	character_icky = TRUE;
	Term_save();

	while (TRUE)
	{
		print_skill_batch(p, start, max, mode);
		which = inkey();

		if (which == ESCAPE)
		{
			ret = -1;
			break;
		}
		else if (which == '*' || which == '?' || which == ' ')
		{
			mode = (mode)?FALSE:TRUE;
			Term_load();
			character_icky = FALSE;
		}
		else if (which == '+')
		{
			start += 20;
			if (start >= max) start -= 20;
			Term_load();
			character_icky = FALSE;
		}
		else if (which == '-')
		{
			start -= 20;
			if (start < 0) start += 20;
			Term_load();
			character_icky = FALSE;
		}
		else if (which == '@')
                {
                        char buf[80];

                        strcpy(buf, "Cast a spell");
                        if (!get_string("Skill action? ", buf, 79))
                                return FALSE;

                        /* Find the skill it is related to */
                        for (i = 1; i < max_s_idx; i++)
                        {
                                if (!strcmp(buf, s_info[i].action_desc + s_text) && get_skill(i))
                                        break;
                        }
                        if ((i < max_s_idx))
                        {
                                ret = i;
                                break;
                        }

		}
		else
		{
			which = tolower(which);
			if (start + A2I(which) >= max)
			{
				bell();
				continue;
			}
			if (start + A2I(which) < 0)
			{
				bell();
				continue;
			}

			ret = p[start + A2I(which)];
			break;
		}
	}
	Term_load();
	character_icky = FALSE;

	C_FREE(p, max_s_idx, int);

	return ret;
}

/* Ask & execute a skill -- currently unused (all if 0'ed). Check Receive_activate_skill instead */
void do_cmd_activate_skill()
{
	int x_idx;
	bool push = TRUE;

	/* Get the skill, if available */
	if (repeat_pull(&x_idx))
	{
		if ((x_idx < 0) || (x_idx >= max_s_idx)) return;
		push = FALSE;
	}
	else if (!command_arg) x_idx = do_cmd_activate_skill_aux();
	else
	{
		x_idx = command_arg - 1;
		if ((x_idx < 0) || (x_idx >= max_s_idx)) return;
                if ((!s_info[x_idx].value) || (!s_info[x_idx].action_mkey))
                {
                        msg_print("You cannot use this skill.");
                        return;
                }
	}

	if (x_idx == -1) return;

	if (push) repeat_push(x_idx);

	if (!x_idx)
	{
		choose_melee();
		return;
	}

#if 0
	/* Break goi/manashield */
	if (p_ptr->invuln)
	{
		set_invuln(0);
	}
	if (p_ptr->disrupt_shield)
	{
		set_disrupt_shield(0);
	}
#endif
	switch (s_info[x_idx].action_mkey)
	{
		case MKEY_ANTIMAGIC:
			do_cmd_unbeliever();
			break;
		case MKEY_MINDCRAFT:
			do_cmd_mindcraft();
			break;
		case MKEY_ALCHEMY:
			do_cmd_alchemist();
			break;
		case MKEY_MIMIC:
			do_cmd_mimic();
			break;
		case MKEY_POWER_MAGE:
			do_cmd_powermage();
			break;
		case MKEY_RUNE:
			do_cmd_runecrafter();
			break;
		case MKEY_FORGING:
			do_cmd_archer();
			break;
		case MKEY_INCARNATION:
			do_cmd_possessor();
			break;
		case MKEY_TELEKINESIS:
			do_cmd_portable_hole();
			break;
		case MKEY_REALM:
			do_cmd_cast();
			break;
		case MKEY_BLADE:
			do_cmd_blade();
			break;
		case MKEY_SUMMON:
			do_cmd_summoner();
			break;
		case MKEY_NECRO:
			do_cmd_necromancer();
			break;
		case MKEY_TRAP:
			do_cmd_set_trap();
			break;
		case MKEY_STEAL:
			do_cmd_steal();
			break;
		case MKEY_DODGE:
			use_ability_blade();
			break;
		case MKEY_PARRYBLOCK:
			check_parryblock();
			break;
		case MKEY_SCHOOL:
			cast_school_spell();
			break;
                case MKEY_COPY:
                        do_cmd_copy_spell();
                        break;
		default:
			process_hooks(HOOK_MKEY, "(d)", s_info[x_idx].action_mkey);
			break;
	}
}


/* Which magic forbids non FA gloves */
bool forbid_gloves()
{
	if (get_skill(SKILL_SORCERY)) return (TRUE);
	if (get_skill(SKILL_MANA)) return (TRUE);
	if (get_skill(SKILL_FIRE)) return (TRUE);
	if (get_skill(SKILL_AIR)) return (TRUE);
	if (get_skill(SKILL_WATER)) return (TRUE);
	if (get_skill(SKILL_EARTH)) return (TRUE);
	if (get_skill(SKILL_THAUMATURGY)) return (TRUE);
	return (FALSE);
}

/* Which gods forbid edged weapons */
bool forbid_non_blessed()
{
	GOD(GOD_ERU) return (TRUE);
	return (FALSE);
}

/*
 * Get a skill from a list
 */
static int available_skills[] =
{
        SKILL_COMBAT,
        SKILL_MASTERY,
        SKILL_ARCHERY,
        SKILL_HAND,
        SKILL_MAGIC,
        SKILL_DIVINATION,
        SKILL_CONVEYANCE,
        SKILL_SNEAK,
        SKILL_NECROMANCY,
        SKILL_SPIRITUALITY,
        SKILL_MINDCRAFT,
        SKILL_MIMICRY,
        SKILL_ANTIMAGIC,
        SKILL_RUNECRAFT,
        SKILL_TRAPPING,
        SKILL_STEALTH,
        SKILL_DISARMING,
        SKILL_THAUMATURGY,
        SKILL_SUMMON,
        SKILL_LORE,
        -1
};

void do_get_new_skill()
{
        char *items[4];
        int skl[4];
        u32b val[4], mod[4];
        bool used[MAX_SKILLS];
        int max = 0, max_a = 0, res;

        /* Check if some skills didnt influence other stuff */
        recalc_skills(TRUE);

        /* Init */
	for (max = 0; max < MAX_SKILLS; max++)
                used[max] = FALSE;

        /* Count the number of available skills */
        while (available_skills[max_a] != -1) max_a++;

        /* Get 4 skills */
        for (max = 0; max < 4; max++)
        {
                int i;
                skill_type *s_ptr;

                /* Get an non used skill */
                do
                {
                        i = rand_int(max_a);
                } while (used[available_skills[i]]);
                s_ptr = &s_info[available_skills[i]];
                used[available_skills[i]] = TRUE;

                if (s_ptr->mod)
                {
                        val[max] = s_ptr->mod * 3;
                        mod[max] = 0;
                }
                else
                {
                        mod[max] = 500;
                        val[max] = 1000;
                }
                if (s_ptr->value + val[max] > SKILL_MAX) val[max] = SKILL_MAX - s_ptr->value;
                skl[max] = available_skills[i];
                items[max] = (char *)string_make(format("%-40s: +%02ld.%03ld value, +%01d.%03d modifier", s_ptr->name + s_name, val[max] / SKILL_STEP, val[max] % SKILL_STEP, mod[max] / SKILL_STEP, mod[max] % SKILL_STEP));
        }
        res = ask_menu("Choose a skill to learn(a-d to choose, ESC to cancel)?", (char **)items, 4);

        /* Ok ? lets learn ! */
        if (res > -1)
        {
                skill_type *s_ptr = &s_info[skl[res]];
                s_ptr->value += val[res];
                s_ptr->mod += mod[res];
                if (mod[res])
                        msg_format("You can now learn the %s skill.", s_ptr->name + s_name);
                else
                        msg_format("Your knowledge of the %s skill increase.", s_ptr->name + s_name);
        }

        /* Free them ! */
	for (max = 0; max < 4; max++)
                string_free(items[max]);

        /* Check if some skills didnt influence other stuff */
        recalc_skills(FALSE);
}
#endif /*0*/

/* return value by which a skill was auto-modified by related skills
   instead of real point distribution by the player */
static s32b modified_by_related(player_type *p_ptr, int i) {
	int j, points;
	s32b val = 0, jv, jm;
	
	for (j = 1; j < MAX_SKILLS; j++)
	{
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
			if (jm)
				points = (p_ptr->s_info[j].value - jv - modified_by_related(p_ptr, j)) / jm;
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
   of skill points the user did. - C. Blue */
void respec_skill(int Ind, int i) {
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
	real_user_increase = p_ptr->s_info[i].value - v;
	/* Now get rid of amount of indirect increases we got
	   from related skills. */
	real_user_increase = real_user_increase - modified_by_related(p_ptr, i); /* recursive */
	/* catch overflow cap (example: skill starts at 1.000,
	   but is modified by a theoretical total of 50.000 from
	   various other skills -> would end at -1.000) */
	if (real_user_increase < 0) real_user_increase = 0;
	/* calculate skill points spent into this skill */
	spent_points = real_user_increase / m;

	/* modify related skills */
	for (j = 1; j < MAX_SKILLS; j++)
	{
		/* Ignore self */
		if (j == i) continue;

		/* Exclusive skills */
		if (s_info[i].action[j] == SKILL_EXCLUSIVE)
		{
			jv = 0; jm = 0;
			compute_skills(p_ptr, &jv, &jm, j);
			/* Turn it on again (!) */
			p_ptr->s_info[j].value = jv;
		} else { /* Non-exclusive skills */
			/* Decrease with a % */
			val = p_ptr->s_info[j].value -
			    ((p_ptr->s_info[j].mod * s_info[i].action[j] / 100) * spent_points);

			/* Skill value cannot be negative */
			if (val < 0) val = 0;
			/* It cannot exceed SKILL_MAX */
			if (val > SKILL_MAX) val = SKILL_MAX;

			/* Save the modified value */
			p_ptr->s_info[j].value = val;
			
			/* Update the client */
			Send_skill_info(Ind, j);
		}
	}

	/* Remove the points, ie set skill to its starting base value again
	   and just add synergy points it got from other related skills */
	p_ptr->s_info[i].value -= real_user_increase;
	/* Free the points, making them available */
	p_ptr->skill_points += spent_points;

	/* Update the client */
	calc_techniques(Ind);
	Send_skill_info(Ind, i);
	/* XXX updating is delayed till player leaves the skill screen */
	p_ptr->update |= (PU_SKILL_MOD);
	/* also update 'C' character screen live! */
	p_ptr->update |= (PU_BONUS);
	p_ptr->redraw |= (PR_SKILLS | PR_PLUSSES);
}

/* Complete skill-chart reset (full respec) - C. Blue */
void respec_skills(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i;
	s32b v, m; /* base starting skill value, skill modifier */

	/* Remove the points, ie set skills to its starting base values again */
	for (i = 1; i < MAX_SKILLS; i++) {
		v = 0; m = 0;
		compute_skills(p_ptr, &v, &m, i);
		p_ptr->s_info[i].value = v;
		p_ptr->s_info[i].mod = m;
		/* Update the client */
		Send_skill_info(Ind, i);
	}

	/* Calculate amount of skill points that should be
	    available to the player depending on his level */
	p_ptr->skill_points = (p_ptr->max_plv - 1) * 5;

	/* Update the client */
	calc_techniques(Ind);
	/* XXX updating is delayed till player leaves the skill screen */
	p_ptr->update |= (PU_SKILL_MOD);
	/* also update 'C' character screen live! */
	p_ptr->update |= (PU_BONUS);
	p_ptr->redraw |= (PR_SKILLS | PR_PLUSSES);
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
	real_user_increase = p_ptr->s_info[i].value - v;
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
