/* $Id$ */
/* File: melee2.c */

/* Purpose: Monster spells and movement */

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
 * STUPID_MONSTERS flag is left for compatibility, but not recommended.
 * if you think the AI codes are slow (or hard), try the following:
 *
 * 1. reduce SAFETY_RADIUS.
 * 2. reduce INDIRECT_FREQ.
 * 3. define STUPID_MONSTER_SPELLS
 * 4. remove MONSTERS_HIDE_HEADS
 * 5. increase MULTI_HUED_UPDATE (though it's not AI code)
 *
 * Other codes don't affect the total speed so much.
 */
/* STUPID_MONSTER removes all the monster-AI codes, including:
 * - try to surround the player in (MONSTERS_HEMM_IN) <fast>
 * - try not to shoot other monst. with bolts (STUPID_MONSTER_SPELLS) <medium>
 * - pack of animals tries to swarm the player (SAFETY_RADIUS) <medium>
 * - try to hide from players when running (SAFETY_RADIUS) <medium>
 * - choose the best spell available (STUPID_MONSTER_SPELLS) <slow>
 * - try to cast spells indirectly (INDIRECT_FREQ) <slow>
 * - pick up as many items on floor as possible (MONSTERS_GREEDY) <fast>
 */
//#define STUPID_MONSTERS

#ifndef STUPID_MONSTERS
/* radius that every fleeing/hiding monster scans for safe place.
 * if bottle-neckie, reduce this value. [10]
 * set to 0 to disable it.
 */
#define		SAFETY_RADIUS	8
 
/* INDIRECT_FREQ does the following:
 *
 * ########....
 * ..x.......p.
 * ....###.....
 * ....@##.....
 * .....##Q....
 *
 * 'p' casts a spell on 'x', so that spell(ball and summoning) will affect '@'
 * indirectly.
 * also, they cast self-affecting spells(heal, blink etc).
 * 'Q' can never summons on '@', however (s)he may attempt to phase.
 *
 * Note that player can do the same thing (using /tar command), which was
 * an effective abuse in some variants.
 */

/* Chance of an out-of-sight monster to cast a spell, in percent.
 * reducing this also speeds the server up. [50]
 */
#define		INDIRECT_FREQ	50
 
/* pseudo 'radius' for summoning spells. default is 3.  */
#define		INDIRECT_SUMMONING_RADIUS	3

/* if defined, a monster will simply choose the spell by RNG and
 * never hesitate to shoot its friends.
 */
//#define	STUPID_MONSTER_SPELLS

/* though Quylthulgs are intelligent by nature, for the balance's sake
 * this flag bans Qs from casting spells indirectly (just like vanilla ones).
 */
#define		STUPID_Q

/*
 * Animal packs try to get the player out of corridors
 * (...unless they can move through walls -- TY)
 */
#define MONSTERS_HIDE_HEADS

/* horde of monsters will try to surround the player.
 * very fast and recommended.
 */
#define MONSTERS_HEMM_IN

/*
 * Chance of monsters that have TAKE_ITEM heading for treasures around them
 * instead of heading for players, in percent.	[30]
 *
 * If defined (even 0), monsters will also 'stand still' when a pile of items
 * is below their feet.
 */ 
#define		MONSTERS_GREEDY	30


#else	// STUPID_MONSTERS
/* disable everything */
#define	SAFETY_RADIUS	0
#define	INDIRECT_FREQ	0
#define	INDIRECT_SUMMONING_RADIUS	0
#define	STUPID_Q
#define	STUPID_MONSTER_SPELLS
//#define		MONSTERS_GREEDY	30
//#define MONSTERS_HIDE_HEADS
//#define MONSTERS_HEMM_IN


#endif // STUPID_MONSTERS


/* How frequent they change colours? [5] */
#define		MULTI_HUED_UPDATE	10

/* 
 * Chance of a breeder breeding, in percent. [0]
 * if you completely ban monsters from breeding, set this to -1(and not 0!).
 * 0 disables this check.
 */
#define		REPRO_RATE	50

/*
 * Extra check for summoners NOT to choose summoning spells, in percent. [50]
 */
#define		SUPPRESS_SUMMON_RATE	50

/* distance for AI_ANNOY (nothing to do with game speed.) */
#define		ANNOY_DISTANCE	5

/*
 * Adjust the chance of intelligent monster digging through the wall,
 * by percent. [100]
 * To disable, comment it out.
 */
#define		MONSTER_DIG_FACTOR	100

/*
 * If defined, monsters will pick up the gold like normal items.
 * Ever wondered why monster rogues never do that? :)
 */
#define		MONSTER_PICKUP_GOLD

/*
 * Chance of an item picked up by a monster to disappear, in %. [30]
 * Stolen items are not affected by this value.
 *
 * TODO: best if timed ... ie. the longer a monster holds it, the more
 * the chance of consuming.
 */
#define		MONSTER_ITEM_CONSUME	30


/*
 * DRS_SMART_OPTIONS is not available for now.
 */

#ifdef DRS_SMART_OPTIONS


/*
 * And now for Intelligent monster attacks (including spells).
 *
 * Original idea and code by "DRS" (David Reeves Sward).
 * Major modifications by "BEN" (Ben Harrison).
 *
 * Give monsters more intelligent attack/spell selection based on
 * observations of previous attacks on the player, and/or by allowing
 * the monster to "cheat" and know the player status.
 *
 * Maintain an idea of the player status, and use that information
 * to occasionally eliminate "ineffective" spell attacks.  We could
 * also eliminate ineffective normal attacks, but there is no reason
 * for the monster to do this, since he gains no benefit.
 * Note that MINDLESS monsters are not allowed to use this code.
 * And non-INTELLIGENT monsters only use it partially effectively.
 *
 * Actually learn what the player resists, and use that information
 * to remove attacks or spells before using them.  This will require
 * much less space, if I am not mistaken.  Thus, each monster gets a
 * set of 32 bit flags, "smart", build from the various "SM_*" flags.
 *
 * This has the added advantage that attacks and spells are related.
 * The "smart_learn" option means that the monster "learns" the flags
 * that should be set, and "smart_cheat" means that he "knows" them.
 * So "smart_cheat" means that the "smart" field is always up to date,
 * while "smart_learn" means that the "smart" field is slowly learned.
 * Both of them have the same effect on the "choose spell" routine.
 */




/*
 * Internal probablility routine
 */
static bool int_outof(monster_race *r_ptr, int prob)
{
	/* Non-Smart monsters are half as "smart" */
	if (!(r_ptr->flags2 & RF2_SMART)) prob = prob / 2;

	/* Roll the dice */
	return (rand_int(100) < prob);
}



/*
 * Remove the "bad" spells from a spell list
 */
static void remove_bad_spells(int m_idx, u32b *f4p, u32b *f5p, u32b *f6p)
{
	monster_type *m_ptr = &m_list[m_idx];
        monster_race *r_ptr = race_inf(m_ptr);

	u32b f4 = (*f4p);
	u32b f5 = (*f5p);
	u32b f6 = (*f6p);

	u32b smart = 0L;


	/* Too stupid to know anything */
	if (r_ptr->flags2 & RF2_STUPID) return;


	/* Must be cheating or learning */
	if (!smart_cheat && !smart_learn) return;


	/* Update acquired knowledge */
	if (smart_learn)
	{
		/* Hack -- Occasionally forget player status */
		if (m_ptr->smart && (rand_int(100) < 1)) m_ptr->smart = 0L;

		/* Use the memorized flags */
		smart = m_ptr->smart;
	}


	/* Cheat if requested */
	if (smart_cheat)
	{
		/* Know basic info */
		if (p_ptr->resist_acid) smart |= SM_RES_ACID;
		if (p_ptr->oppose_acid) smart |= SM_OPP_ACID;
		if (p_ptr->immune_acid) smart |= SM_IMM_ACID;
		if (p_ptr->resist_elec) smart |= SM_RES_ELEC;
		if (p_ptr->oppose_elec) smart |= SM_OPP_ELEC;
		if (p_ptr->immune_elec) smart |= SM_IMM_ELEC;
		if (p_ptr->resist_fire) smart |= SM_RES_FIRE;
		if (p_ptr->oppose_fire) smart |= SM_OPP_FIRE;
		if (p_ptr->immune_fire) smart |= SM_IMM_FIRE;
		if (p_ptr->resist_cold) smart |= SM_RES_COLD;
		if (p_ptr->oppose_cold) smart |= SM_OPP_COLD;
		if (p_ptr->immune_cold) smart |= SM_IMM_COLD;

		/* Know poison info */
		if (p_ptr->resist_pois) smart |= SM_RES_POIS;
		if (p_ptr->oppose_pois) smart |= SM_OPP_POIS;

		/* Know special resistances */
		if (p_ptr->resist_neth) smart |= SM_RES_NETH;
			if (p_ptr->immune_neth) smart |= SM_RES_NETH;
		if (p_ptr->resist_lite) smart |= SM_RES_LITE;
		if (p_ptr->resist_dark) smart |= SM_RES_DARK;
		if (p_ptr->resist_fear) smart |= SM_RES_FEAR;
		if (p_ptr->resist_conf) smart |= SM_RES_CONF;
		if (p_ptr->resist_chaos) smart |= SM_RES_CHAOS;
		if (p_ptr->resist_disen) smart |= SM_RES_DISEN;
		if (p_ptr->resist_blind) smart |= SM_RES_BLIND;
		if (p_ptr->resist_nexus) smart |= SM_RES_NEXUS;
		if (p_ptr->resist_sound) smart |= SM_RES_SOUND;
		if (p_ptr->resist_shard) smart |= SM_RES_SHARD;

		/* Know bizarre "resistances" */
		if (p_ptr->free_act) smart |= SM_IMM_FREE;
		if (!p_ptr->msp) smart |= SM_IMM_MANA;
	}


	/* Nothing known */
	if (!smart) return;


	if (smart & SM_IMM_ACID)
	{
		if (int_outof(r_ptr, 100)) f4 &= ~RF4_BR_ACID;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BA_ACID;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BO_ACID;
	}
	else if ((smart & SM_OPP_ACID) && (smart & SM_RES_ACID))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~RF4_BR_ACID;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BA_ACID;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BO_ACID;
	}
	else if ((smart & SM_OPP_ACID) || (smart & SM_RES_ACID))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~RF4_BR_ACID;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BA_ACID;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BO_ACID;
	}


	if (smart & SM_IMM_ELEC)
	{
		if (int_outof(r_ptr, 100)) f4 &= ~RF4_BR_ELEC;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BA_ELEC;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BO_ELEC;
	}
	else if ((smart & SM_OPP_ELEC) && (smart & SM_RES_ELEC))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~RF4_BR_ELEC;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BA_ELEC;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BO_ELEC;
	}
	else if ((smart & SM_OPP_ELEC) || (smart & SM_RES_ELEC))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~RF4_BR_ELEC;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BA_ELEC;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BO_ELEC;
	}


	if (smart & SM_IMM_FIRE)
	{
		if (int_outof(r_ptr, 100)) f4 &= ~RF4_BR_FIRE;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BA_FIRE;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BO_FIRE;
	}
	else if ((smart & SM_OPP_FIRE) && (smart & SM_RES_FIRE))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~RF4_BR_FIRE;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BA_FIRE;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BO_FIRE;
	}
	else if ((smart & SM_OPP_FIRE) || (smart & SM_RES_FIRE))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~RF4_BR_FIRE;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BA_FIRE;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BO_FIRE;
	}


	if (smart & SM_IMM_COLD)
	{
		if (int_outof(r_ptr, 100)) f4 &= ~RF4_BR_COLD;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BA_COLD;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BO_COLD;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BO_ICEE;
	}
	else if ((smart & SM_OPP_COLD) && (smart & SM_RES_COLD))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~RF4_BR_COLD;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BA_COLD;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BO_COLD;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BO_ICEE;
	}
	else if ((smart & SM_OPP_COLD) || (smart & SM_RES_COLD))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~RF4_BR_COLD;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BA_COLD;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BO_COLD;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BO_ICEE;
	}


	if ((smart & SM_OPP_POIS) && (smart & SM_RES_POIS))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~RF4_BR_POIS;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BA_POIS;
	}
	else if ((smart & SM_OPP_POIS) || (smart & SM_RES_POIS))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~RF4_BR_POIS;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BA_POIS;
	}


	if (smart & SM_RES_NETH)
	{
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_NETH;
		if (int_outof(r_ptr, 50)) f5 &= ~RF5_BA_NETH;
		if (int_outof(r_ptr, 50)) f5 &= ~RF5_BO_NETH;
	}

	if (smart & SM_RES_LITE)
	{
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_LITE;
	}

	if (smart & SM_RES_DARK)
	{
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_DARK;
		if (int_outof(r_ptr, 50)) f5 &= ~RF5_BA_DARK;
	}

	if (smart & SM_RES_FEAR)
	{
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_SCARE;
	}

	if (smart & SM_RES_CONF)
	{
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_CONF;
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_CONF;
	}

	if (smart & SM_RES_CHAOS)
	{
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_CONF;
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_CONF;
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_CHAO;
	}

	if (smart & SM_RES_DISEN)
	{
		if (int_outof(r_ptr, 100)) f4 &= ~RF4_BR_DISE;
	}

	if (smart & SM_RES_BLIND)
	{
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BLIND;
	}

	if (smart & SM_RES_NEXUS)
	{
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_NEXU;
		if (int_outof(r_ptr, 50)) f6 &= ~RF6_TELE_LEVEL;
	}

	if (smart & SM_RES_SOUND)
	{
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_SOUN;
	}

	if (smart & SM_RES_SHARD)
	{
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_SHAR;
	}


	if (smart & SM_IMM_FREE)
	{
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_HOLD;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_SLOW;
	}

	if (smart & SM_IMM_MANA)
	{
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_DRAIN_MANA;
	}


	/* XXX XXX XXX No spells left? */
	/* if (!f4 && !f5 && !f6) ... */


	(*f4p) = f4;
	(*f5p) = f5;
	(*f6p) = f6;
}


#endif


/*
 * Cast a bolt at the player
 * Stop if we hit a monster
 * Affect monsters and the player
 */
static void bolt(int Ind, int m_idx, int typ, int dam_hp)
{
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_STOP | PROJECT_KILL;

	/* Target the player with a bolt attack */
	(void)project(m_idx, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, dam_hp, typ, flg);
}


/*
 * Cast a breath (or ball) attack at the player
 * Pass over any monsters that may be in the way
 * Affect grids, objects, monsters, and the player
 */
static void breath(int Ind, int m_idx, int typ, int dam_hp, int y, int x, int rad)
{
	player_type *p_ptr = Players[Ind];

	int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	/* Target the player with a ball attack */
	(void)project(m_idx, rad, &p_ptr->wpos, y, x, dam_hp, typ, flg);
}
#if 0
static void breath(int Ind, int m_idx, int typ, int dam_hp, int rad)
{
	player_type *p_ptr = Players[Ind];

//	int rad;

	int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	monster_type *m_ptr = &m_list[m_idx];
        monster_race *r_ptr = race_inf(m_ptr);

	/* Determine the radius of the blast */
	if (rad < 1) rad = (r_ptr->flags2 & (RF2_POWERFUL)) ? 3 : 2;

	/* Target the player with a ball attack */
	(void)project(m_idx, rad, &p_ptr->wpos, p_ptr->py, p_ptr->px, dam_hp, typ, flg);
}
#endif	// 0

/*
 * Functions for Cleverer monster spells, borrowed from PernA.
 * Most of them are hard-coded, so change them manually :)
 * 			- Jir -
 */


/*
 * Determine if there is a space near the player in which
 * a summoned creature can appear
 */
static bool summon_possible(worldpos *wpos, int y1, int x1)
{
	int y, x, i;
	cave_type **zcave;

	if(!(zcave=getcave(wpos))) return(FALSE);
	
	/* Start with adjacent locations, spread further */
	for (i = 1; i <= tdi[2]; i++)
	{
		y = y1 + tdy[i];
		x = x1 + tdx[i];

		/* Ignore illegal locations */
		if (!in_bounds(y,x)) continue;

		/* Hack: no summon on glyph of warding */
		if (zcave[y][x].feat == FEAT_GLYPH) continue;
#if 0
		if (cave[y][x].feat == FEAT_MINOR_GLYPH) continue;

		/* Nor on the between */
		if (cave[y][x].feat == FEAT_BETWEEN) return (FALSE);

		/* ...nor on the Pattern */
		if ((cave[y][x].feat >= FEAT_PATTERN_START)
				&& (cave[y][x].feat <= FEAT_PATTERN_XTRA2)) continue;
#endif	// 0

		/* Require empty floor grid in line of sight */
		if (cave_empty_bold(zcave,y,x) && los(wpos, y1,x1,y,x)) return (TRUE);
	}

#if 0	
	/* Start at the player's location, and check 2 grids in each dir */
	for (y= y1-2; y<= y1+2; y++)
	{
		for (x = x1-2; x<=x1+2; x++)
		{
			/* Ignore illegal locations */
			if (!in_bounds(y,x)) continue;
			
			/* Only check a circular area */
			if (distance(y1,x1,y,x)>2) continue;
			
			/* Hack: no summon on glyph of warding */
			if (zcave[y][x].feat == FEAT_GLYPH) continue;
#if 0
			if (cave[y][x].feat == FEAT_MINOR_GLYPH) continue;

                        /* Nor on the between */
                        if (cave[y][x].feat == FEAT_BETWEEN) return (FALSE);

                        /* ...nor on the Pattern */
			if ((cave[y][x].feat >= FEAT_PATTERN_START)
				&& (cave[y][x].feat <= FEAT_PATTERN_XTRA2)) continue;
#endif	// 0
			
			/* Require empty floor grid in line of sight */
			if (cave_empty_bold(zcave,y,x) && los(wpos, y1,x1,y,x)) return (TRUE);
		}
	}
#endif	// 0
	return FALSE;
}

/*
 * Determine if a bolt spell will hit the player.
 *
 * This is exactly like "projectable", but it will return FALSE if a monster
 * is in the way.
 */
static bool clean_shot(worldpos *wpos, int y1, int x1, int y2, int x2)
{
	int dist, y, x;
	cave_type **zcave;

	if(!(zcave=getcave(wpos))) return(FALSE);
	
	/* Start at the initial location */
	y = y1, x = x1;
	
	/* See "project()" and "projectable()" */
	for (dist = 0; dist <= MAX_RANGE; dist++)
	{
		/* Never pass through walls */
		if (dist && !cave_floor_bold(zcave, y, x)) break;
		
		/* Never pass through monsters */
		if (dist && zcave[y][x].m_idx > 0)
		{
			break;
//			if (is_friend(&m_list[zcave[y][x].m_idx]) < 0) break;
		}
		
		/* Check for arrival at "final target" */
		if ((x == x2) && (y == y2)) return (TRUE);
		
		/* Calculate the new location */
		mmove2(&y, &x, y1, x1, y2, x2);
	}
	
	/* Assume obstruction */
	return (FALSE);
}


/*
 * Return TRUE if a spell is good for hurting the player (directly).
 */
static bool spell_attack(byte spell)
{
	/* All RF4 spells hurt (except for shriek) */
	if (spell < 128 && spell > 96) return (TRUE);
	
	/* Various "ball" spells */
	if (spell >= 128 && spell <= 128 + 8) return (TRUE);
	
	/* Unmagic is not */
	if (spell == 128 + 15) return (FALSE);

	/* "Cause wounds" and "bolt" spells */
	if (spell >= 128 + 12 && spell <= 128 + 27) return (TRUE);
	
	/* Hand of Doom */
	if (spell == 160 + 1) return (TRUE);
	
	/* Doesn't hurt */
	return (FALSE);
}


/*
 * Return TRUE if a spell is good for escaping.
 */
static bool spell_escape(byte spell)
{
	/* Blink or Teleport */
	if (spell == 160 + 4 || spell == 160 + 5) return (TRUE);
	
	/* Teleport the player away */
	if (spell == 160 + 9 || spell == 160 + 10) return (TRUE);
	
	/* Isn't good for escaping */
	return (FALSE);
}

/*
 * Return TRUE if a spell is good for annoying the player.
 */
static bool spell_annoy(byte spell)
{
	/* Shriek */
	if (spell == 96 + 0) return (TRUE);
	
	/* Brain smash, et al (added curses) */
//	if (spell >= 128 + 9 && spell <= 128 + 14) return (TRUE);
	/* and unmagic */
	if (spell >= 128 + 9 && spell <= 128 + 15) return (TRUE);
	
	/* Scare, confuse, blind, slow, paralyze */
	if (spell >= 128 + 27 && spell <= 128 + 31) return (TRUE);
	
	/* Teleport to */
	if (spell == 160 + 8) return (TRUE);
	
#if 0
	/* Hand of Doom */
	if (spell == 160 + 1) return (TRUE);
#endif
	
	/* Darkness, make traps, cause amnesia */
	if (spell >= 160 + 12 && spell <= 160 + 14) return (TRUE);
	
	/* Doesn't annoy */
	return (FALSE);
}

/*
 * Return TRUE if a spell summons help.
 */
static bool spell_summon(byte spell)
{
	/* All summon spells */
//	if (spell >= 160 + 13) return (TRUE);
	if (spell >= 160 + 15) return (TRUE);

	/* My interpretation of what it should be - evileye */
	if (spell == 160 + 3) return (TRUE);
	if (spell == 160 + 7) return (TRUE);
	if (spell == 160 + 11) return (TRUE);
	/* summon animal */
//	if (spell = 96 + 2) return (TRUE);
	
	/* Doesn't summon */
	return (FALSE);
}

/*
 * Return TRUE if a spell is good in a tactical situation.
 */
static bool spell_tactic(byte spell)
{
	/* Blink */
	if (spell == 160 + 4) return (TRUE);
	
	/* Not good */
	return (FALSE);
}


/*
 * Return TRUE if a spell hastes.
 */
static bool spell_haste(byte spell)
{
	/* Haste self */
	if (spell == 160 + 0) return (TRUE);
	
	/* Not a haste spell */
	return (FALSE);
}


/*
 * Return TRUE if a spell is good for healing.
 */
static bool spell_heal(byte spell)
{
	/* Heal */
	if (spell == 160 + 2) return (TRUE);
	
	/* No healing */
	return (FALSE);
}


/*
 * Have a monster choose a spell from a list of "useful" spells.
 *
 * Note that this list does NOT include spells that will just hit
 * other monsters, and the list is restricted when the monster is
 * "desperate".  Should that be the job of this function instead?
 *
 * Stupid monsters will just pick a spell randomly.  Smart monsters
 * will choose more "intelligently".
 *
 * Use the helper functions above to put spells into categories.
 *
 * This function may well be an efficiency bottleneck.
 */
static int choose_attack_spell(int Ind, int m_idx, byte spells[], byte num)
{
	player_type *p_ptr = Players[Ind];
	monster_type *m_ptr = &m_list[m_idx];
        monster_race *r_ptr = race_inf(m_ptr);
	
	byte escape[96], escape_num = 0;
	byte attack[96], attack_num = 0;
	byte summon[96], summon_num = 0;
	byte tactic[96], tactic_num = 0;
	byte annoy[96], annoy_num = 0;
	byte haste[96], haste_num = 0;
	byte heal[96], heal_num = 0;
	
	int i;
	
#if 0
	/* Stupid monsters choose randomly */
	if (r_ptr->flags2 & (RF2_STUPID))
	{
		/* Pick at random */
		return (spells[rand_int(num)]);
	}
#endif	// 0
	
	/* Categorize spells */
	for (i = 0; i < num; i++)
	{
		/* Escape spell? */
		if (spell_escape(spells[i])) escape[escape_num++] = spells[i];
		
		/* Attack spell? */
		if (spell_attack(spells[i])) attack[attack_num++] = spells[i];
		
		/* Summon spell? */
		if (spell_summon(spells[i])) summon[summon_num++] = spells[i];
		
		/* Tactical spell? */
		if (spell_tactic(spells[i])) tactic[tactic_num++] = spells[i];
		
		/* Annoyance spell? */
		if (spell_annoy(spells[i])) annoy[annoy_num++] = spells[i];
		
		/* Haste spell? */
		if (spell_haste(spells[i])) haste[haste_num++] = spells[i];
		
		/* Heal spell? */
		if (spell_heal(spells[i])) heal[heal_num++] = spells[i];
	}
	
	/*** Try to pick an appropriate spell type ***/
	
	/* Hurt badly or afraid, attempt to flee */
	if ((m_ptr->hp < m_ptr->maxhp / 3) || m_ptr->monfear)
	{
		/* Choose escape spell if possible */
		if (escape_num) return (escape[rand_int(escape_num)]);
	}
	
	/* Still hurt badly, couldn't flee, attempt to heal */
	if (m_ptr->hp < m_ptr->maxhp / 3 || m_ptr->stunned)
	{
		/* Choose heal spell if possible */
		if (heal_num) return (heal[rand_int(heal_num)]);
	}
	
	/* Player is close and we have attack spells, blink away */
	if ((distance(p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx) < 4) && attack_num && (rand_int(100) < 75))
	{
		/* Choose tactical spell */
		if (tactic_num) return (tactic[rand_int(tactic_num)]);
	}
	
	/* We're hurt (not badly), try to heal */
	if ((m_ptr->hp < m_ptr->maxhp * 3 / 4) && (rand_int(100) < 75))
	{
		/* Choose heal spell if possible */
		if (heal_num) return (heal[rand_int(heal_num)]);
	}
	
	/* Summon if possible (sometimes) */
	if (summon_num && (rand_int(100) < 50))
	{
		/* Choose summon spell */
		return (summon[rand_int(summon_num)]);
	}
	
	/* Attack spell (most of the time) */
	if (attack_num && (rand_int(100) < 85))
	{
		/* Choose attack spell */
		return (attack[rand_int(attack_num)]);
	}
	
	/* Try another tactical spell (sometimes) */
	if (tactic_num && (rand_int(100) < 50))
	{
		/* Choose tactic spell */
		return (tactic[rand_int(tactic_num)]);
	}
	
	/* Haste self if we aren't already somewhat hasted (rarely) */
        if (haste_num && (rand_int(100) < (20 + m_ptr->speed - m_ptr->mspeed)))
	{
		/* Choose haste spell */
		return (haste[rand_int(haste_num)]);
	}
	
	/* Annoy player (most of the time) */
	if (annoy_num && (rand_int(100) < 85))
	{
		/* Choose annoyance spell */
		return (annoy[rand_int(annoy_num)]);
	}
	
	/* Choose no spell */
	return (0);
}

//bool monst_check_grab(int Ind, int m_idx, cptr desc)
bool monst_check_grab(int m_idx, int mod, cptr desc)
{
//	player_type *p_ptr;
	monster_type	*m_ptr = &m_list[m_idx];
	monster_race    *r_ptr = race_inf(m_ptr);

	worldpos *wpos = &m_ptr->wpos;

	cave_type **zcave;
	int i, x2 = m_ptr->fx, y2 = m_ptr->fy;
	int grabchance;
	int rlev = r_ptr->level;

	if (!(zcave=getcave(wpos))) return(FALSE);

	for (i = 1; i <= NumPlayers; i++)
	{
		player_type *q_ptr = Players[i];

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;

		/* Skip players not on this depth */
		if (!inarea(&q_ptr->wpos, wpos)) continue;

		/* Compute distance */
		if (distance(y2, x2, q_ptr->py, q_ptr->px) > 1) continue;

		/* Cannot grab what you cannot see */
		if (!q_ptr->mon_vis[m_idx]) continue;

		if (q_ptr->confused || q_ptr->stun || q_ptr->afraid || q_ptr->paralyzed)
			continue;

		grabchance = get_skill_scale(q_ptr, SKILL_INTERCEPT, 150) - rlev;

		/* Apply Martial-arts bonus */
		if (get_skill(q_ptr, SKILL_MARTIAL_ARTS) && !monk_heavy_armor(q_ptr)
				&& !q_ptr->inventory[INVEN_WIELD].k_idx)
			grabchance += get_skill_scale(q_ptr, SKILL_MARTIAL_ARTS, 50);

		grabchance *= mod / 100;

		if (q_ptr->blind) grabchance -= 30;

		if (grabchance < 1) continue;

//		grabchance = 50 + q_ptr->lev/2 - (q_ptr->blind?30:0);
		if (grabchance > 95) grabchance = 95;

		/* Got disrupted ? */
		if (magik(grabchance))
		{
			char		m_name[80];
			/* Get the monster name (or "it") */
			monster_desc(i, m_name, m_idx, 0x00);

//			    msg_format(Ind, "\377o%^s fails to cast a spell.", m_name);
#if 0
			if (i == Ind) msg_format(Ind, "\377oYou grab %s back from teleportation!", m_name);
			else msg_format(Ind, "%s grabs %s back from teleportation!", q_ptr->name, m_name);
#else	// 0
			msg_format(i, "\377oYou intercept %s's attempt to %s!", m_name, desc);
			msg_format_near(i, "%s intercepts %s's attempt to %s!", q_ptr->name, m_name, desc);
#endif	// 0
			return TRUE;
		}
	}

	/* Assume no grabbing */
	return FALSE;
}


bool monst_check_antimagic(int Ind, int m_idx)
{
//	player_type *p_ptr;
	monster_type	*m_ptr = &m_list[m_idx];
//	monster_race    *r_ptr = race_inf(m_ptr);

	worldpos *wpos = &m_ptr->wpos;

	cave_type **zcave;
	int i, x, y, x2 = m_ptr->fx, y2 = m_ptr->fy;
	int dis, antichance;	// , antidis;

	if (!(zcave=getcave(wpos))) return(FALSE);

	for (i = 1; i <= NumPlayers; i++)
	{
		player_type *q_ptr = Players[i];

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;

		/* Skip players not on this depth */
		if (!inarea(&q_ptr->wpos, wpos)) continue;

		/* Compute distance */
//		dis = distance(y2, x2, q_ptr->py, q_ptr->px);
		if (distance(y2, x2, q_ptr->py, q_ptr->px) > q_ptr->antimagic_dis)
			continue;

//		antidis = q_ptr->antimagic_dis;

		antichance = q_ptr->antimagic;

//		antichance -= r_ptr->level;

		if (antichance > 95) antichance = 95;

		/* Reduction for party */
		/*
		if ((i != Ind) && player_in_party(p_ptr->party, i))
			antichance >>= 1;
		*/

		if (i != Ind) antichance >>= 1;

//		if (dis > antidis) antichance = 0;

		/* Got disrupted ? */
		if (magik(antichance))
		{
			char		m_name[80];
			/* Get the monster name (or "it") */
			monster_desc(Ind, m_name, m_idx, 0x00);

//			    msg_format(Ind, "\377o%^s fails to cast a spell.", m_name);
#if 0
			if (i == Ind) msg_format(Ind, "\377oYour anti-magic shield disrupts %s's attempts.", m_name);
			else msg_format(Ind, "%s's anti-magic shield disrupts %s's attempts.", q_ptr->name, m_name);
#else	// 0
			if (q_ptr->mon_vis[m_idx])
				msg_format(i, "\377oYour anti-magic shield disrupts %s's attempts.", m_name);
			msg_format_near(i, "%s's anti-magic shield disrupts %s's attempts.", q_ptr->name, m_name);
#endif	// 0

			return TRUE;
		}
	}

#if 0
	/* Use this code if monster's antimagic should hinder other monsters'
	 * casting.
	 */
	/* Scan the maximal area of radius "MONSTER_ANTIDIS" */
	for (y = y2 - MONSTER_ANTIDIS; y <= y2 + MONSTER_ANTIDIS; y++)
	{
		for (x = x2 - MONSTER_ANTIDIS; x <= x2 + MONSTER_ANTIDIS; x++)
		{
			/* Ignore "illegal" locations */
			if (!in_bounds2(wpos, y, x)) continue;

			if ((m_idx = zcave[y][x].m_idx)<=0) continue;

			/* Enforce a "circular" explosion */
			if ((dis = distance(y2, x2, y, x)) > MONSTER_ANTIDIS) continue;

			m_ptr = &m_list[m_idx];	// pfft, bad design

			/* dont use removed monsters */
			if(!m_ptr->r_idx) continue;

			r_ptr = race_inf(m_ptr);

			if (!(r_ptr->flags7 & RF7_DISBELIEVE)) continue;

			antichance = r_ptr->level / 2 + 20;
			antidis = r_ptr->level / 15 + 3;

			if (dis > antidis) continue;
			if (antichance > 95) antichance = 95;

			/* Got disrupted ? */
			if (magik(antichance))
			{
				if (p_ptr->mon_vis[m_idx])
				{
					char m_name[80];
					monster_desc(Ind, m_name, m_idx, 0);
					msg_format(Ind, "%^s's anti-magic shield disrupts your attempts.", m_name);
				}
				else
				{
					msg_print(Ind, "An anti-magic shield disrupts your attempts.");
				}
				return TRUE;
			}
		}
	}
#endif	// 0

	/* Assume no antimagic */
	return FALSE;
}

/*
 * Check if a monster can harm the player indirectly.		- Jir -
 * Return 99 if no suitable place is found.
 *
 * Great bottleneck :-/
 */
static int near_hit(int m_idx, int *yp, int *xp, int rad)
{
	monster_type *m_ptr = &m_list[m_idx];

	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

	int py = *yp;
	int px = *xp;

	int y, x, d = 1, i;	// , dis;
	/*
	int vy = magik(50) ? -1 : 1;
	int vx = magik(50) ? -1 : 1;
	bool giveup = TRUE;
	*/

//	int gy = 0, gx = 0, gdis = 0;

	cave_type **zcave;

	if (rad < 1) return 99;

	/* paranoia */
	if(!(zcave=getcave(&m_ptr->wpos))) return 99;

	for (i = 1; i <= tdi[rad]; i++)
	{
		if (i == tdi[d]) d++;

		y = py + tdy[i];
		x = px + tdx[i];

		/* Check nearby locations */

		/* Skip illegal locations */
		if (!in_bounds(y, x)) continue;

		/* Skip locations in a wall */
		if (!cave_floor_bold(zcave, y, x)) continue;

		/* Check if projectable */
		if (projectable(&m_ptr->wpos, fy, fx, y, x) &&
				projectable_wall(&m_ptr->wpos, y, x, py, px))
		{
			/* Good location */
			(*yp) = y;
			(*xp) = x;

			/* Found nice place */
			return (d);
		}
	}

#if 0
	/* Start with adjacent locations, spread further */
	for (d = 1; d < 4; d++)
	{
		giveup = TRUE;

		/* Check nearby locations */
		for (y = py - d; y <= py + d; y++)
//		for (y = py - d*vy; y <= py + d*vy; y+=vy)
		{
			for (x = px - d; x <= px + d; x++)
//			for (x = px - d*vx; x <= px + d*vx; x+=vx)
			{
				/* Skip illegal locations */
				if (!in_bounds(y, x)) continue;

				/* Skip locations in a wall */
				if (!cave_floor_bold(zcave, y, x)) continue;

				/* Check distance */
				if (distance(y, x, py, px) != d) continue;

				/* Check if projectable */
				if (projectable_wall(&m_ptr->wpos, fy, fx, y, x))
				{
					giveup = FALSE;
					if (projectable_wall(&m_ptr->wpos, y, x, py, px))
					{
						/* Good location */
						(*yp) = y;
						(*xp) = x;

						/* Found nice place */
						return (d);
					}
				}
			}
		}
		if (giveup) return (99);
	}
#endif	// 0

	/* No projectable place */
	return (99);
}


/*
 * Creatures can cast spells, shoot missiles, and breathe.
 *
 * Returns "TRUE" if a spell (or whatever) was (successfully) cast.
 *
 * XXX XXX XXX This function could use some work, but remember to
 * keep it as optimized as possible, while retaining generic code.
 *
 * Verify the various "blind-ness" checks in the code.
 *
 * XXX XXX XXX Note that several effects should really not be "seen"
 * if the player is blind.  See also "effects.c" for other "mistakes".
 *
 * Perhaps monsters should breathe at locations *near* the player,
 * since this would allow them to inflict "partial" damage.
 *
 * Perhaps smart monsters should decline to use "bolt" spells if
 * there is a monster in the way, unless they wish to kill it.
 *
 * Note that, to allow the use of the "track_target" option at some
 * later time, certain non-optimal things are done in the code below,
 * including explicit checks against the "direct" variable, which is
 * currently always true by the time it is checked, but which should
 * really be set according to an explicit "projectable()" test, and
 * the use of generic "x,y" locations instead of the player location,
 * with those values being initialized with the player location.
 *
 * It will not be possible to "correctly" handle the case in which a
 * monster attempts to attack a location which is thought to contain
 * the player, but which in fact is nowhere near the player, since this
 * might induce all sorts of messages about the attack itself, and about
 * the effects of the attack, which the player might or might not be in
 * a position to observe.  Thus, for simplicity, it is probably best to
 * only allow "faulty" attacks by a monster if one of the important grids
 * (probably the initial or final grid) is in fact in view of the player.
 * It may be necessary to actually prevent spell attacks except when the
 * monster actually has line of sight to the player.  Note that a monster
 * could be left in a bizarre situation after the player ducked behind a
 * pillar and then teleported away, for example.
 *
 * Note that certain spell attacks do not use the "project()" function
 * but "simulate" it via the "direct" variable, which is always at least
 * as restrictive as the "project()" function.  This is necessary to
 * prevent "blindness" attacks and such from bending around walls, etc,
 * and to allow the use of the "track_target" option in the future.
 *
 * Note that this function attempts to optimize the use of spells for the
 * cases in which the monster has no spells, or has spells but cannot use
 * them, or has spells but they will have no "useful" effect.  Note that
 * this function has been an efficiency bottleneck in the past.
 */
bool make_attack_spell(int Ind, int m_idx)
{
	player_type *p_ptr = Players[Ind];

	struct worldpos *wpos=&p_ptr->wpos;

	dun_level		*l_ptr = getfloor(wpos);

	int			k, chance, thrown_spell, rlev;	// , failrate;

	byte		spell[96], num = 0;

	u32b		f4, f5, f6;

	monster_type	*m_ptr = &m_list[m_idx];
        monster_race    *r_ptr = race_inf(m_ptr);

	object_type *o_ptr = &p_ptr->inventory[INVEN_WIELD];

	char		m_name[80];
	char		m_poss[80];

	char		ddesc[80];

	/* Target location */
	int x = p_ptr->px;
	int y = p_ptr->py;

	/* Summon count */
	int count = 0;

	/* Extract the blind-ness */
	bool blind = (p_ptr->blind ? TRUE : FALSE);

	/* Extract the "within-the-vision-ness" */
	bool visible = p_ptr->mon_vis[m_idx];

	/* Extract the "see-able-ness" */
	bool seen = !blind && visible;

	/* Assume "normal" target */
	bool normal = TRUE;

	/* Assume "projectable" */
	bool direct = TRUE;

	bool stupid, summon;
	int rad = 0, srad;


//	int antichance = 0, antidis = 0;

	/* Cannot cast spells when confused */
	if (m_ptr->confused) return (FALSE);

	/* Hack -- Extract the spell probability */
	chance = (r_ptr->freq_inate + r_ptr->freq_spell) / 2;

	/* Not allowed to cast spells */
//	if (!chance) return (FALSE);

	/* Only do spells occasionally */
	if (rand_int(100) >= chance) return (FALSE);

	/* Compute the probability of the unbeliever to disrupt any magic attempts */
#if 0
	if ((p_ptr->pclass == CLASS_UNBELIEVER))
	  {
	    antichance += p_ptr->lev;
	    antidis += 1 + (p_ptr->lev / 11);
	  }


	if (o_ptr->k_idx)
	  {
	    u32b f1, f2, f3, f4, f5, esp;

			  /* Extract the flags */
			  object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

//	    if (f2 & TR2_ANTI_MAGIC)
	    if (f2 & TR3_NO_MAGIC)
	      {
		int minus = o_ptr->to_h + o_ptr->to_d + o_ptr->pval;
		/* if ((minus < 0) && (p_ptr->pclass != CLASS_UNBELIEVER)) minus = 0; */
		if (p_ptr->pclass != CLASS_UNBELIEVER)
		{
			if (minus < -p_ptr->lev / 2) minus = -p_ptr->lev / 2;
			if (minus < -40) minus = -40;
		}

		/* Enchanting DarkSwords is not a wise thing */
		antichance += 50 - minus;
		antidis += 4 - (minus / 10);
	      }
	  }
#endif	// 0
#if 0	
	antichance = p_ptr->antimagic;
	antidis = p_ptr->antimagic_dis;

	if (m_ptr->cdis > antidis) antichance = 0;
#endif	// 0


	/* XXX XXX XXX Handle "track_target" option (?) */


	/* Extract the racial spell flags */
	f4 = r_ptr->flags4;
	f5 = r_ptr->flags5;
	f6 = r_ptr->flags6;

	/* radius of ball spells and breathes.
	 * XXX this doesn't reflect some exceptions(eg. radius=4 spells). */
	srad = (r_ptr->flags2 & (RF2_POWERFUL)) ? 3 : 2;

	/* Hack -- require projectable player */
	if (normal)
	{
		/* Check range */
		if (m_ptr->cdis > MAX_RANGE) return (FALSE);

		/* Check path */
#if INDIRECT_FREQ < 1
		if (!projectable_wall(&p_ptr->wpos, m_ptr->fy, m_ptr->fx, p_ptr->py, p_ptr->px)) return (FALSE);
#else
		summon = f4 & (RF4_SUMMON_MASK) || f5 & (RF5_SUMMON_MASK) || f6 & (RF6_SUMMON_MASK);

		if (!projectable_wall(&p_ptr->wpos, m_ptr->fy, m_ptr->fx, p_ptr->py, p_ptr->px))
		{
#ifdef STUPID_Q
			if (r_ptr->d_char == 'Q') return (FALSE);
#endif	// STUPID_Q

			if (!magik(INDIRECT_FREQ)) return (FALSE);

			direct = FALSE;

			/* effort to avoid bottlenecks.. */
			if (summon ||
				f4 & RF4_RADIUS_SPELLS ||
				f5 & RF5_RADIUS_SPELLS)
				rad = near_hit(m_idx, &y, &x,
						srad > INDIRECT_SUMMONING_RADIUS ?
						srad : INDIRECT_SUMMONING_RADIUS);
			else rad = 99;

//			if (rad > 3 || (rad == 3 && !(r_ptr->flags2 & (RF2_POWERFUL)))) 
			if (rad > srad) 
			{
				/* Remove inappropriate spells -
				 * 'direct' spells should be removed this way also */
				f4 &= ~(RF4_RADIUS_SPELLS);
				f5 &= ~(RF5_RADIUS_SPELLS);
//				f6 &= RF6_INT_MASK;

				/* No spells left */
				if (!f4 && !f5 && !f6) return (FALSE);
			}
		}
#endif	// INDIRECT_FREQ
	}


	/* Hack -- allow "desperate" spells */
	if ((r_ptr->flags2 & RF2_SMART) &&
	    (m_ptr->hp < m_ptr->maxhp / 10) &&
	    (rand_int(100) < 50))
	{
		/* Require intelligent spells */
		f4 &= RF4_INT_MASK;
		f5 &= RF5_INT_MASK;
		f6 &= RF6_INT_MASK;

		/* No spells left */
		if (!f4 && !f5 && !f6) return (FALSE);
	}

	/* Stupid monster flag */
	stupid = r_ptr->flags2 & (RF2_STUPID);
	

#ifdef DRS_SMART_OPTIONS

	/* Remove the "ineffective" spells */
	remove_bad_spells(m_idx, &f4, &f5, &f6);

	/* No spells left */
	if (!f4 && !f5 && !f6) return (FALSE);

#endif

#ifndef	STUPID_MONSTER_SPELLS
	/* Check for a clean bolt shot */
	if (!direct ||
		((f4&(RF4_BOLT_MASK) || f5 & (RF5_BOLT_MASK) || f6&(RF6_BOLT_MASK)) &&
		 !stupid && !clean_shot(wpos, m_ptr->fy, m_ptr->fx, y, x)))
	{
		/* Remove spells that will only hurt friends */
		f4 &= ~(RF4_BOLT_MASK);
		f5 &= ~(RF5_BOLT_MASK);
		f6 &= ~(RF6_BOLT_MASK);
	}

	/* Check for a possible summon */
	/*
	if (rad > 3 || ((f4 & (RF4_SUMMON_MASK) || f5 & (RF5_SUMMON_MASK) ||
				f6 & (RF6_SUMMON_MASK)) &&
	*/
//	if (rad > 3 ||
	if (rad > INDIRECT_SUMMONING_RADIUS || magik(SUPPRESS_SUMMON_RATE) ||
		(summon && !stupid &&
		!(summon_possible(wpos, y, x))))	// <= we can omit this now?
	{
		/* Remove summoning spells */
		f4 &= ~(RF4_SUMMON_MASK);
		f5 &= ~(RF5_SUMMON_MASK);
		f6 &= ~(RF6_SUMMON_MASK);
	}

	/* No spells left */
	if (!f4 && !f5 && !f6) return (FALSE);
#endif	// STUPID_MONSTER_SPELLS

	/* Extract the "inate" spells */
	for (k = 0; k < 32; k++)
	{
		if (f4 & (1L << k)) spell[num++] = k + 32 * 3;
		if (f5 & (1L << k)) spell[num++] = k + 32 * 4;
		if (f6 & (1L << k)) spell[num++] = k + 32 * 5;
	}

#if 0
	/* Extract the "normal" spells */
	for (k = 0; k < 32; k++)
	{
		if (f5 & (1L << k)) spell[num++] = k + 32 * 4;
	}

	/* Extract the "bizarre" spells */
	for (k = 0; k < 32; k++)
	{
		if (f6 & (1L << k)) spell[num++] = k + 32 * 5;
	}
#endif	// 0

	/* No spells left */
	if (!num) return (FALSE);


	/* Stop if player is dead or gone */
	if (!p_ptr->alive || p_ptr->death || p_ptr->new_level_flag) return (FALSE);


	/* Get the monster name (or "it") */
	monster_desc(Ind, m_name, m_idx, 0x00);

#ifdef STUPID_MONSTER_SPELLS
	/* Choose a spell to cast */
	thrown_spell = spell[rand_int(num)];
#else
	thrown_spell = stupid ? spell[rand_int(num)] :
		choose_attack_spell(Ind, m_idx, spell, num);

	/* Abort if no spell was chosen */
	if (!thrown_spell) return (FALSE);
#endif	// STUPID_MONSTER_SPELLS

	if(thrown_spell < 128 && l_ptr && l_ptr->flags1 & LF1_NO_MAGIC)
		return(FALSE);

	/* Extract the monster level */
	rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

#ifndef STUPID_MONSTER_SPELLS
	if (!stupid && thrown_spell >= 128)
	{
		int factor = 0;

		/* Extract the 'stun' factor */
		if (m_ptr->stunned > 50) factor += 25;
		if (m_ptr->stunned) factor += 15;

		if (magik(25 - (rlev + 3) / 4) || magik(factor))
		{
			/* Message */
			if (direct)
				msg_format(Ind, "%^s tries to cast a spell, but fails.", m_name);

			return (TRUE);
		}

		if (monst_check_grab(m_idx, 50, "cast")) return (TRUE);
	}
#endif	// STUPID_MONSTER_SPELLS

	/* Get the monster possessive ("his"/"her"/"its") */
	monster_desc(Ind, m_poss, m_idx, 0x22);

	/* Hack -- Get the "died from" name */
	monster_desc(Ind, ddesc, m_idx, 0x88);


	/* Cast the spell. */
	switch (thrown_spell)
	{
		/* RF4_SHRIEK */
		case 96+0:
		{
			if (!direct) break;
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			msg_format(Ind, "%^s makes a high pitched shriek.", m_name);
			aggravate_monsters(Ind, m_idx);
			break;
		}

		/* RF4_XXX2X4 */
		/* (used to be RF4_MULTIPLY .. not a spell) */
		case 96+1:
		{
			break;
		}
#if 0
		/* RF4_XXX3X4 */
		case 96+2:
		{
			break;
		}
#endif

		/* RF4_S_ANIMAL */
		case 96+2:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons an animal!", m_name);
			for (k = 0; k < 1; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_ANIMAL);
			}
			if (blind && count) msg_print(Ind, "You hear something appear nearby.");
			break;
		}
			
#if 0
		/* RF4_XXX4X4 */
		case 96+3:
		{
			break;
		}
#endif
			/* RF4_XXX4X4 */
			/* (RF4_ROCKET) */
		case 96+3:
			{
				disturb(Ind, 1, 0);
			 if (blind) msg_format(Ind, "%^s shoots something.", m_name);
			 else msg_format(Ind, "%^s fires a rocket.", m_name);
			 breath(Ind, m_idx, GF_ROCKET,
				 ((m_ptr->hp / 4) > 800 ? 800 : (m_ptr->hp / 4)), y, x, 2);
			 update_smart_learn(m_idx, DRS_SHARD);
			 break;
			}
			

		/* RF4_ARROW_1 */
		case 96+4:
		{
//			int power = rlev / 2 + randint(rlev / 2),
			int	dice = 1 + rlev / 8,
				fois = 1 + rlev / 20;
#if 0
			if (power > 8) dice += 2;
			if (power > 20) dice += 2;
			if (power > 30) dice += 2;
#endif
			disturb(Ind, 1, 0);
			if (monst_check_grab(m_idx, 100, "fire")) break;
			for (k = 0; k < fois; k++)
			{
				if (blind) msg_format(Ind, "%^s makes a strange noise.", m_name);
				else msg_format(Ind, "%^s fires an arrow.", m_name);
				bolt(Ind, m_idx, GF_ARROW, damroll(dice, 6));
				if (p_ptr->death) break;
			}
			break;
		}

		/* RF4_ARROW_2 */
		case 96+5:
		{
//			int power = rlev / 2 + randint(rlev / 2),
//				fois = 1 + rlev / 20;
			int	dice = 3 + rlev / 15;
#if 0
			if (power > 8) dice += 2;
			if (power > 20) dice += 2;
			if (power > 30) dice += 2;
#endif
			
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s makes a strange noise.", m_name);
			else msg_format(Ind, "%^s fires a missile.", m_name);
			bolt(Ind, m_idx, GF_ARROW, damroll(dice, 6));
		}

#if 0
		/* RF4_ARROW_1 */
		case 96+4:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s makes a strange noise.", m_name);
			else msg_format(Ind, "%^s fires an arrow.", m_name);
			bolt(Ind, m_idx, GF_ARROW, damroll(1, 6));
			break;
		}

		/* RF4_ARROW_2 */
		case 96+5:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s makes a strange noise.", m_name);
			else msg_format(Ind, "%^s fires an arrow!", m_name);
			bolt(Ind, m_idx, GF_ARROW, damroll(3, 6));
			break;
		}

		/* RF4_ARROW_3 */
		case 96+6:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s makes a strange noise.", m_name);
			else msg_format(Ind, "%^s fires a missile.", m_name);
			bolt(Ind, m_idx, GF_ARROW, damroll(5, 6));
			break;
		}

		/* RF4_ARROW_4 */
		case 96+7:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s makes a strange noise.", m_name);
			else msg_format(Ind, "%^s fires a missile!", m_name);
			bolt(Ind, m_idx, GF_ARROW, damroll(7, 6));
			break;
		}

#endif	// 0

		/* former RF4_ARROW_3 (prolly RF4_BRAND_ARROW?) */
		case 96+6:
		{
			break;
		}

		/* former RF4_ARROW_4 */
		case 96+7:
		{
			break;
		}

		/* RF4_BR_ACID */
		case 96+8:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes acid.", m_name);
			breath(Ind, m_idx, GF_ACID,
			       ((m_ptr->hp / 3) > 1600 ? 1600 : (m_ptr->hp / 3)), y, x, srad);
			update_smart_learn(m_idx, DRS_ACID);
			break;
		}

		/* RF4_BR_ELEC */
		case 96+9:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes lightning.", m_name);
			breath(Ind, m_idx, GF_ELEC,
			       ((m_ptr->hp / 3) > 1600 ? 1600 : (m_ptr->hp / 3)), y, x, srad);
			update_smart_learn(m_idx, DRS_ELEC);
			break;
		}

		/* RF4_BR_FIRE */
		case 96+10:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes fire.", m_name);
			breath(Ind, m_idx, GF_FIRE,
			       ((m_ptr->hp / 3) > 1600 ? 1600 : (m_ptr->hp / 3)), y, x, srad);
			update_smart_learn(m_idx, DRS_FIRE);
			break;
		}

		/* RF4_BR_COLD */
		case 96+11:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes frost.", m_name);
			breath(Ind, m_idx, GF_COLD,
			       ((m_ptr->hp / 3) > 1600 ? 1600 : (m_ptr->hp / 3)), y, x, srad);
			update_smart_learn(m_idx, DRS_COLD);
			break;
		}

		/* RF4_BR_POIS */
		case 96+12:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes gas.", m_name);
			breath(Ind, m_idx, GF_POIS,
			       ((m_ptr->hp / 3) > 800 ? 800 : (m_ptr->hp / 3)), y, x, srad);
			update_smart_learn(m_idx, DRS_POIS);
			break;
		}

		/* RF4_BR_NETH */
		case 96+13:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes nether.", m_name);
			breath(Ind, m_idx, GF_NETHER,
			       ((m_ptr->hp / 6) > 550 ? 550 : (m_ptr->hp / 6)), y, x, srad);
			update_smart_learn(m_idx, DRS_NETH);
			break;
		}

		/* RF4_BR_LITE */
		case 96+14:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes light.", m_name);
			breath(Ind, m_idx, GF_LITE,
			       ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6)), y, x, srad);
			update_smart_learn(m_idx, DRS_LITE);
			break;
		}

		/* RF4_BR_DARK */
		case 96+15:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes darkness.", m_name);
			breath(Ind, m_idx, GF_DARK,
			       ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6)), y, x, srad);
			update_smart_learn(m_idx, DRS_DARK);
			break;
		}

		/* RF4_BR_CONF */
		case 96+16:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes confusion.", m_name);
			breath(Ind, m_idx, GF_CONFUSION,
			       ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6)), y, x, srad);
			update_smart_learn(m_idx, DRS_CONF);
			break;
		}

		/* RF4_BR_SOUN */
		case 96+17:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes sound.", m_name);
			breath(Ind, m_idx, GF_SOUND,
			       ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6)), y, x, srad);
			update_smart_learn(m_idx, DRS_SOUND);
			break;
		}

		/* RF4_BR_CHAO */
		case 96+18:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes chaos.", m_name);
			breath(Ind, m_idx, GF_CHAOS,
			       ((m_ptr->hp / 6) > 600 ? 600 : (m_ptr->hp / 6)), y, x, srad);
			update_smart_learn(m_idx, DRS_CHAOS);
			break;
		}

		/* RF4_BR_DISE */
		case 96+19:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes disenchantment.", m_name);
			breath(Ind, m_idx, GF_DISENCHANT,
			       ((m_ptr->hp / 6) > 500 ? 500 : (m_ptr->hp / 6)), y, x, srad);
			update_smart_learn(m_idx, DRS_DISEN);
			break;
		}

		/* RF4_BR_NEXU */
		case 96+20:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes nexus.", m_name);
			breath(Ind, m_idx, GF_NEXUS,
			       ((m_ptr->hp / 3) > 250 ? 250 : (m_ptr->hp / 3)), y, x, srad);
			update_smart_learn(m_idx, DRS_NEXUS);
			break;
		}

		/* RF4_BR_TIME */
		case 96+21:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes time.", m_name);
			breath(Ind, m_idx, GF_TIME,
			       ((m_ptr->hp / 3) > 150 ? 150 : (m_ptr->hp / 3)), y, x, srad);
			break;
		}

		/* RF4_BR_INER */
		case 96+22:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes inertia.", m_name);
			breath(Ind, m_idx, GF_INERTIA,
			       ((m_ptr->hp / 6) > 200 ? 200 : (m_ptr->hp / 6)), y, x, srad);
			break;
		}

		/* RF4_BR_GRAV */
		case 96+23:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes gravity.", m_name);
			breath(Ind, m_idx, GF_GRAVITY,
			       ((m_ptr->hp / 3) > 200 ? 200 : (m_ptr->hp / 3)), y, x, srad);
			break;
		}

		/* RF4_BR_SHAR */
		case 96+24:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes shards.", m_name);
			breath(Ind, m_idx, GF_SHARDS,
			       ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6)), y, x, srad);
			update_smart_learn(m_idx, DRS_SHARD);
			break;
		}

		/* RF4_BR_PLAS */
		case 96+25:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes plasma.", m_name);
			breath(Ind, m_idx, GF_PLASMA,
			       ((m_ptr->hp / 6) > 150 ? 150 : (m_ptr->hp / 6)), y, x, srad);
			break;
		}

		/* RF4_BR_WALL */
		case 96+26:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s breathes.", m_name);
			else msg_format(Ind, "%^s breathes force.", m_name);
			breath(Ind, m_idx, GF_FORCE,
			       ((m_ptr->hp / 6) > 200 ? 200 : (m_ptr->hp / 6)), y, x, srad);
			break;
		}

		/* RF4_BR_MANA */
		case 96+27:
		{
			 disturb(Ind, 1, 0);
			 if (blind) msg_format(Ind, "%^s breathes.", m_name);
			 else msg_format(Ind, "%^s breathes magical energy.", m_name);
			 breath(Ind, m_idx, GF_MANA,
				 ((m_ptr->hp / 3) > 250 ? 250 : (m_ptr->hp / 3)), y, x, srad);
			break;
		}

		 /* RF4_XXX5X4 */
		 /* RF4_BA_NUKE */
	 case 96+28:
		 {
			 disturb(Ind, 1, 0);
			 if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			 else msg_format(Ind, "%^s casts a ball of radiation.", m_name);
			 breath(Ind, m_idx, GF_NUKE, (rlev + damroll(10, 6)), y, x, 2);
			 update_smart_learn(m_idx, DRS_POIS);
			 break;
		 }
		 
		 /* RF4_XXX6X4 */
		 /* RF4_BR_NUKE */
	 case 96+29:
		 {
			 disturb(Ind, 1, 0);
			 if (blind) msg_format(Ind, "%^s breathes.", m_name);
			 else msg_format(Ind, "%^s breathes toxic waste.", m_name);
			 breath(Ind, m_idx, GF_NUKE,
				 ((m_ptr->hp / 3) > 800 ? 800 : (m_ptr->hp / 3)), y, x, srad);
			 update_smart_learn(m_idx, DRS_POIS);
			 break;
		 }
		 
		 /* RF4_XXX7X4 */
		 /* RF4_BA_CHAO */
	 case 96+30:
		 {
			 disturb(Ind, 1, 0);
			 if (blind) msg_format(Ind, "%^s mumbles frighteningly.", m_name);
                         else msg_format(Ind, "%^s invokes a raw chaos.", m_name);
			 breath(Ind, m_idx, GF_CHAOS, (rlev * 2) + damroll(10, 10), y, x, 4);
			 update_smart_learn(m_idx, DRS_CHAOS);
			 break;
		 }
		 
		 /* RF4_XXX8X4 -> Disintegration breath! */
		 /* RF4_BA_DISI */
	 case 96+31:
		 {
			 disturb(Ind, 1, 0);
			 if (blind) msg_format(Ind, "%^s breathes.", m_name);
			 else msg_format(Ind, "%^s breathes disintegration.", m_name);
			 breath(Ind, m_idx, GF_DISINTEGRATE,
				 ((m_ptr->hp / 3) > 300 ? 300 : (m_ptr->hp / 3)), y, x, srad);
			 break;
		 }
		 



		/* RF5_BA_ACID */
		case 128+0:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts an acid ball.", m_name);
			breath(Ind, m_idx, GF_ACID,
			       randint(rlev * 3) + 15, y, x, srad);
			update_smart_learn(m_idx, DRS_ACID);
			break;
		}

		/* RF5_BA_ELEC */
		case 128+1:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a lightning ball.", m_name);
			breath(Ind, m_idx, GF_ELEC,
			       randint(rlev * 3 / 2) + 8, y, x, srad);
			update_smart_learn(m_idx, DRS_ELEC);
			break;
		}

		/* RF5_BA_FIRE */
		case 128+2:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a fire ball.", m_name);
			breath(Ind, m_idx, GF_FIRE,
			       randint(rlev * 7 / 2) + 10, y, x, srad);
			update_smart_learn(m_idx, DRS_FIRE);
			break;
		}

		/* RF5_BA_COLD */
		case 128+3:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a frost ball.", m_name);
			breath(Ind, m_idx, GF_COLD,
			       randint(rlev * 3 / 2) + 10, y, x, srad);
			update_smart_learn(m_idx, DRS_COLD);
			break;
		}

		/* RF5_BA_POIS */
		case 128+4:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a stinking cloud.", m_name);
			breath(Ind, m_idx, GF_POIS,
			       damroll(12, 2), y, x, srad);
			update_smart_learn(m_idx, DRS_POIS);
			break;
		}

		/* RF5_BA_NETH */
		case 128+5:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a nether ball.", m_name);
			breath(Ind, m_idx, GF_NETHER, (50 + damroll(10, 10) + rlev), y, x, srad);
			update_smart_learn(m_idx, DRS_NETH);
			break;
		}

		/* RF5_BA_WATE */
		case 128+6:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s gestures fluidly.", m_name);
			msg_print(Ind, "You are engulfed in a whirlpool.");
			breath(Ind, m_idx, GF_WATER,
			       randint(rlev * 5 / 2) + 50, y, x, srad);
			break;
		}

		/* RF5_BA_MANA */
		case 128+7:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles powerfully.", m_name);
			else msg_format(Ind, "%^s invokes a mana storm.", m_name);
			breath(Ind, m_idx, GF_MANA,
			       (rlev * 5) + damroll(10, 10), y, x, srad);
			break;
		}

		/* RF5_BA_DARK */
		case 128+8:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles powerfully.", m_name);
			else msg_format(Ind, "%^s invokes a darkness storm.", m_name);
			breath(Ind, m_idx, GF_DARK,
			       (rlev * 5) + damroll(10, 10), y, x, srad);
			update_smart_learn(m_idx, DRS_DARK);
			break;
		}

		/* RF5_DRAIN_MANA */
		case 128+9:
		{
			if (!direct) break;
			if (monst_check_antimagic(Ind, m_idx)) break;
			if (p_ptr->csp)
			{
				int r1;

				/* Disturb if legal */
				disturb(Ind, 1, 0);

				/* Basic message */
				msg_format(Ind, "%^s draws psychic energy from you!", m_name);

				/* Attack power */
				r1 = (randint(rlev) / 2) + 1;

				/* Full drain */
				if (r1 >= p_ptr->csp)
				{
					r1 = p_ptr->csp;
					p_ptr->csp = 0;
					p_ptr->csp_frac = 0;
				}

				/* Partial drain */
				else
				{
					p_ptr->csp -= r1;
				}

				/* Redraw mana */
				p_ptr->redraw |= (PR_MANA);

				/* Window stuff */
				p_ptr->window |= (PW_PLAYER);

				/* Heal the monster */
				if (m_ptr->hp < m_ptr->maxhp)
				{
					/* Heal */
					m_ptr->hp += (6 * r1);
					if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

					/* Redraw (later) if needed */
					update_health(m_idx);

					/* Special message */
					if (seen)
					{
						msg_format(Ind, "%^s appears healthier.", m_name);
					}
				}
			}
			update_smart_learn(m_idx, DRS_MANA);
			break;
		}

		/* RF5_MIND_BLAST */
		case 128+10:
		{
			if (!direct) break;
			disturb(Ind, 1, 0);
			if (!seen)
			{
				msg_print(Ind, "You feel something focusing on your mind.");
			}
			else
			{
				msg_format(Ind, "%^s gazes deep into your eyes.", m_name);
			}

			if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
			}
			else
			{
				msg_print(Ind, "Your mind is blasted by psionic energy.");
				if (!p_ptr->resist_conf)
				{
					(void)set_confused(Ind, p_ptr->confused + rand_int(4) + 4);
				}

				if ((!p_ptr->resist_chaos) && (randint(3)==1))
				{
					(void) set_image(Ind, p_ptr->image + rand_int(250) + 150);
				}
				 

				take_sanity_hit(Ind, damroll(8, 8), ddesc);
//				take_hit(Ind, damroll(8, 8), ddesc);
			}
			break;
		}

		/* RF5_BRAIN_SMASH */
		case 128+11:
		{
			if (!direct) break;
			disturb(Ind, 1, 0);
			if (!seen)
			{
				msg_print(Ind, "You feel something focusing on your mind.");
			}
			else
			{
				msg_format(Ind, "%^s looks deep into your eyes.", m_name);
			}
			if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
			}
			else
			{
				msg_print(Ind, "Your mind is blasted by psionic energy.");
//				take_hit(Ind, damroll(12, 15), ddesc);
				take_sanity_hit(Ind, damroll(12,15), ddesc);
				if (!p_ptr->resist_blind)
				{
					(void)set_blind(Ind, p_ptr->blind + 8 + rand_int(8));
				}
				if (!p_ptr->resist_conf)
				{
					(void)set_confused(Ind, p_ptr->confused + rand_int(4) + 4);
				}
				if (!p_ptr->free_act)
				{
					(void)set_paralyzed(Ind, p_ptr->paralyzed + rand_int(4) + 4);
				}
				(void)set_slow(Ind, p_ptr->slow + rand_int(4) + 4);
			}
			break;
		}

		/* RF5_CURSE (former CAUSE1~4) */
		case 128+12:
		{
			/* rebalance might be needed? */
			int power = rlev / 2 + randint(rlev);
			if (!direct) break;
			disturb(Ind, 1, 0);
			if (power < 15)
			{
				if (blind) msg_format(Ind, "%^s mumbles.", m_name);
				else msg_format(Ind, "%^s points at you and curses.", m_name);
				if (rand_int(100) < p_ptr->skill_sav)
				{
					msg_print(Ind, "You resist the effects!");
				}
				else
				{
					take_hit(Ind, damroll(3, 8), ddesc);
				}
				break;
			}

			/* RF5_CAUSE_2 */
			else if (power < 35)
			{
				if (blind) msg_format(Ind, "%^s mumbles.", m_name);
				else msg_format(Ind, "%^s points at you and curses horribly.", m_name);
				if (rand_int(100) < p_ptr->skill_sav)
				{
					msg_print(Ind, "You resist the effects!");
				}
				else
				{
					take_hit(Ind, damroll(8, 8), ddesc);
				}
				break;
			}

			/* RF5_CAUSE_3 */
			else if (power < 50)
			{
				if (blind) msg_format(Ind, "%^s mumbles loudly.", m_name);
				else msg_format(Ind, "%^s points at you, incanting terribly!", m_name);
				if (rand_int(100) < p_ptr->skill_sav)
				{
					msg_print(Ind, "You resist the effects!");
				}
				else
				{
					take_hit(Ind, damroll(10, 15), ddesc);
				}
				break;
			}

			/* RF5_CAUSE_4 */
			else
			{
				if (blind) msg_format(Ind, "%^s screams the word 'DIE!'", m_name);
				else msg_format(Ind, "%^s points at you, screaming the word DIE!", m_name);
				if (rand_int(100) < p_ptr->skill_sav)
				{
					msg_print(Ind, "You resist the effects!");
				}
				else
				{
					take_hit(Ind, damroll(15, 15), ddesc);
					(void)set_cut(Ind, p_ptr->cut + damroll(10, 10));
				}
				break;
			}
		}
#if 0
		/* RF5_CAUSE_1 */
		case 128+12:
		{
			if (!direct) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s points at you and curses.", m_name);
			if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
			}
			else
			{
				take_hit(Ind, damroll(3, 8), ddesc);
			}
			break;
		}

		/* RF5_CAUSE_2 */
		case 128+13:
		{
			if (!direct) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s points at you and curses horribly.", m_name);
			if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
			}
			else
			{
				take_hit(Ind, damroll(8, 8), ddesc);
			}
			break;
		}

		/* RF5_CAUSE_3 */
		case 128+14:
		{
			if (!direct) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles loudly.", m_name);
			else msg_format(Ind, "%^s points at you, incanting terribly!", m_name);
			if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
			}
			else
			{
				take_hit(Ind, damroll(10, 15), ddesc);
			}
			break;
		}

		/* RF5_CAUSE_4 */
		case 128+15:
		{
			if (!direct) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s screams the word 'DIE!'", m_name);
			else msg_format(Ind, "%^s points at you, screaming the word DIE!", m_name);
			if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
			}
			else
			{
				take_hit(Ind, damroll(15, 15), ddesc);
				(void)set_cut(Ind, p_ptr->cut + damroll(10, 10));
			}
			break;
		}
#endif	// 0

		/* RF5_XXX4X4? */
		case 128+13:
		{
			break;
		}
		/* RF5_XXX4X4? */
		case 128+14:
		{
			break;
		}
		/* RF5_XXX4X4? */
		/* RF5_UNMAGIC (former CAUSE4) */
		case 128+15:
		{
			if (!direct) break;
			disturb(Ind, 1, 0);
#if 0	// oops, this cannot be 'magic' ;)
			if (monst_check_antimagic(Ind, m_idx)) break;
#endif	// 0
//			if (blind) msg_format(Ind, "%^s mumbles coldly.", m_name); else
			msg_format(Ind, "%^s mumbles coldly.", m_name);
			if (rand_int(120) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
			}
			else if (!unmagic(Ind))
			{
				msg_print(Ind, "You are unaffected!");
			}
			break;
		}

		/* RF5_BO_ACID */
		case 128+16:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a acid bolt.", m_name);
			bolt(Ind, m_idx, GF_ACID,
			     damroll(7, 8) + (rlev / 3));
			update_smart_learn(m_idx, DRS_ACID);
			break;
		}

		/* RF5_BO_ELEC */
		case 128+17:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a lightning bolt.", m_name);
			bolt(Ind, m_idx, GF_ELEC,
			     damroll(4, 8) + (rlev / 3));
			update_smart_learn(m_idx, DRS_ELEC);
			break;
		}

		/* RF5_BO_FIRE */
		case 128+18:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a fire bolt.", m_name);
			bolt(Ind, m_idx, GF_FIRE,
			     damroll(9, 8) + (rlev / 3));
			update_smart_learn(m_idx, DRS_FIRE);
			break;
		}

		/* RF5_BO_COLD */
		case 128+19:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a frost bolt.", m_name);
			bolt(Ind, m_idx, GF_COLD,
			     damroll(6, 8) + (rlev / 3));
			update_smart_learn(m_idx, DRS_COLD);
			break;
		}

		/* RF5_BO_POIS */
		case 128+20:
		{
			/* XXX XXX XXX */
			break;
		}

		/* RF5_BO_NETH */
		case 128+21:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a nether bolt.", m_name);
			bolt(Ind, m_idx, GF_NETHER,
			     30 + damroll(5, 5) + (rlev * 3) / 2);
			update_smart_learn(m_idx, DRS_NETH);
			break;
		}

		/* RF5_BO_WATE */
		case 128+22:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a water bolt.", m_name);
			bolt(Ind, m_idx, GF_WATER,
			     damroll(10, 10) + (rlev));
			break;
		}

		/* RF5_BO_MANA */
		case 128+23:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a mana bolt.", m_name);
			bolt(Ind, m_idx, GF_MANA,
			     randint(rlev * 7 / 2) + 50);
			break;
		}

		/* RF5_BO_PLAS */
		case 128+24:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a plasma bolt.", m_name);
			bolt(Ind, m_idx, GF_PLASMA,
			     10 + damroll(8, 7) + (rlev));
			break;
		}

		/* RF5_BO_ICEE */
		case 128+25:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts an ice bolt.", m_name);
			bolt(Ind, m_idx, GF_ICE,
			     damroll(6, 6) + (rlev));
			update_smart_learn(m_idx, DRS_COLD);
			break;
		}

		/* RF5_MISSILE */
		case 128+26:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a magic missile.", m_name);
			bolt(Ind, m_idx, GF_MISSILE,
			     damroll(2, 6) + (rlev / 3));
			break;
		}

		/* RF5_SCARE */
		case 128+27:
		{
			if (!direct) break;
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles, and you hear scary noises.", m_name);
			else msg_format(Ind, "%^s casts a fearful illusion.", m_name);
			if (p_ptr->resist_fear)
			{
				msg_print(Ind, "You refuse to be frightened.");
			}
			else if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You refuse to be frightened.");
			}
			else
			{
				(void)set_afraid(Ind, p_ptr->afraid + rand_int(4) + 4);
			}
			update_smart_learn(m_idx, DRS_FEAR);
			break;
		}

		/* RF5_BLIND */
		case 128+28:
		{
			if (!direct) break;
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s casts a spell, burning your eyes!", m_name);
			if (p_ptr->resist_blind)
			{
				msg_print(Ind, "You are unaffected!");
			}
			else if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
			}
			else
			{
				(void)set_blind(Ind, 12 + rand_int(4));
			}
			update_smart_learn(m_idx, DRS_BLIND);
			break;
		}

		/* RF5_CONF */
		case 128+29:
		{
			if (!direct) break;
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles, and you hear puzzling noises.", m_name);
			else msg_format(Ind, "%^s creates a mesmerising illusion.", m_name);
			if (p_ptr->resist_conf)
			{
				msg_print(Ind, "You disbelieve the feeble spell.");
			}
			else if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You disbelieve the feeble spell.");
			}
			else
			{
				(void)set_confused(Ind, p_ptr->confused + rand_int(4) + 4);
			}
			update_smart_learn(m_idx, DRS_CONF);
			break;
		}

		/* RF5_SLOW */
		case 128+30:
		{
			if (!direct) break;
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			msg_format(Ind, "%^s drains power from your muscles!", m_name);
			if (p_ptr->free_act)
			{
				msg_print(Ind, "You are unaffected!");
			}
			else if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
			}
			else
			{
				(void)set_slow(Ind, p_ptr->slow + rand_int(4) + 4);
			}
			update_smart_learn(m_idx, DRS_FREE);
			break;
		}

		/* RF5_HOLD */
		case 128+31:
		{
			if (!direct) break;
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s stares deep into your eyes!", m_name);
			if (p_ptr->free_act)
			{
				msg_print(Ind, "You are unaffected!");
			}
			else if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_format(Ind, "You resist the effects!");
			}
			else
			{
				(void)set_paralyzed(Ind, p_ptr->paralyzed + rand_int(4) + 4);
			}
			update_smart_learn(m_idx, DRS_FREE);
			break;
		}



		/* RF6_HASTE */
		case 160+0:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind)
			{
				msg_format(Ind, "%^s mumbles.", m_name);
			}
			else
			{
				msg_format(Ind, "%^s concentrates on %s body.", m_name, m_poss);
			}

			/* Allow quick speed increases to base+10 */
			if (m_ptr->mspeed < m_ptr->speed + 10)
			{
				if (visible) msg_format(Ind, "%^s starts moving faster.", m_name);
				m_ptr->mspeed += 10;
			}

			/* Allow small speed increases to base+20 */
			else if (m_ptr->mspeed < m_ptr->speed + 20)
			{
				if (visible) msg_format(Ind, "%^s starts moving faster.", m_name);
				m_ptr->mspeed += 2;
			}

			break;
		}

		/* RF6_XXX1X6 */
		/* RF6_HAND_DOOM */
		/* this should be amplified by some means! */
		case 160+1:
		{
			// if (!direct) break;	/* allow it over wall, or not..? */
			disturb(Ind, 1, 0);
			msg_format(Ind, "%^s invokes the Hand of Doom!", m_name);
			if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_format(Ind, "You resist the effects!");
			}
			else
			{
				int dummy = (((s32b) ((65 + randint(25)) * (p_ptr->chp))) / 100);
				msg_print(Ind, "Your feel your life fade away!");
				bypass_invuln = TRUE;
				take_hit(Ind, dummy, m_name);
				bypass_invuln = FALSE;
				curse_equipment(Ind, 100, 20);

				if (p_ptr->chp < 1) p_ptr->chp = 1;
			}
			break;
		}

		/* RF6_HEAL */
		case 160+2:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);

			/* Message */
			if (visible)
				if (blind)
				{
					msg_format(Ind, "%^s mumbles.", m_name);
				}
				else
				{
					msg_format(Ind, "%^s concentrates on %s wounds.",
							m_name, m_poss);
				}

			/* Heal some */
			m_ptr->hp += (rlev * 6);
			if (m_ptr->stunned)
			{
				m_ptr->stunned -= rlev * 2;
				if (m_ptr->stunned <= 0)
				{
					m_ptr->stunned = 0;
					if (seen)
					{
						msg_format(Ind, "%^s no longer looks stunned!", m_name);
					}
					else
					{
						msg_format(Ind, "%^s no longer sounds stunned!", m_name);
					}
				}
			}

			/* Fully healed */
			if (m_ptr->hp >= m_ptr->maxhp)
			{
				/* Fully healed */
				m_ptr->hp = m_ptr->maxhp;

				/* Message */
				if (seen)
				{
					msg_format(Ind, "%^s looks REALLY healthy!", m_name);
				}
				else
				{
					msg_format(Ind, "%^s sounds REALLY healthy!", m_name);
				}
			}

			/* Partially healed */
			else
			{
				/* Message */
				if (seen)
				{
					msg_format(Ind, "%^s looks healthier.", m_name);
				}
				else
				{
					msg_format(Ind, "%^s sounds healthier.", m_name);
				}
			}

			/* Redraw (later) if needed */
			update_health(m_idx);

			/* Cancel fear */
			if (m_ptr->monfear)
			{
				/* Cancel fear */
				m_ptr->monfear = 0;

				/* Message */
				msg_format(Ind, "%^s recovers %s courage.", m_name, m_poss);
			}

			break;
		}

		/* RF6_XXX2X6 */
		/* RF6_S_ANIMALS */
		case 160+3:
		{
			disturb(Ind, 1, 0);
			if (monst_check_antimagic(Ind, m_idx)) break;
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons some animals!", m_name);
			for (k = 0; k < 4; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_ANIMAL);
			}
			if (blind && count) msg_print(Ind, "You hear something appear nearby.");
			break;
		}
		 

		/* RF6_BLINK */
		case 160+4:
		{
			disturb(Ind, 1, 0);
			if (monst_check_antimagic(Ind, m_idx)) break;
//			if (monst_check_grab(Ind, m_idx)) break;
			/* it's low b/c check for spellcast is already done */
			if (monst_check_grab(m_idx, 40, "teleport")) break;
			if (teleport_away(m_idx, 10) && visible)
				msg_format(Ind, "%^s blinks away.", m_name);
			break;
		}

		/* RF6_TPORT */
		case 160+5:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
//			if (monst_check_grab(Ind, m_idx)) break;
			if (monst_check_grab(m_idx, 40, "teleport")) break;
			if (teleport_away(m_idx, MAX_SIGHT * 2 + 5) && visible)
				msg_format(Ind, "%^s teleports away.", m_name);
			break;
		}

		/* RF6_XXX3X6 */
                /* RF6_ANIM_DEAD */
		case 160+6:
		{
			break;
		}

		/* RF6_XXX4X6 */
		/* RF6_S_BUG */
		case 160+7:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically codes some software bugs.", m_name);
			for (k = 0; k < 6; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_BUG);
			}
			if (blind && count) msg_print(Ind, "You hear many things appear nearby.");
			break;
		}
			

		/* RF6_TELE_TO */
		case 160+8:
		{
			if (!direct) break;
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			/* Hack -- duplicated check to avoid silly message */
			if (p_ptr->anti_tele || check_st_anchor(wpos, p_ptr->py, p_ptr->px))
			{
				msg_format(Ind, "%^s commands you to return, but you don't care.", m_name);
				break;
			}
			msg_format(Ind, "%^s commands you to return.", m_name);
			teleport_player_to(Ind, m_ptr->fy, m_ptr->fx);
			break;
		}

		/* RF6_TELE_AWAY */
		case 160+9:
		{
			if (!direct) break;
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			/* Hack -- duplicated check to avoid silly message */
			if (p_ptr->anti_tele || check_st_anchor(wpos, p_ptr->py, p_ptr->px))
			{
				msg_format(Ind, "%^s tries to teleport you away in vain.", m_name);
				break;
			}
			msg_format(Ind, "%^s teleports you away.", m_name);
			teleport_player(Ind, 100);
			break;
		}

		/* RF6_TELE_LEVEL */
		case 160+10:
		{
			if (!direct) break;
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles strangely.", m_name);
			else msg_format(Ind, "%^s gestures at your feet.", m_name);
			if (p_ptr->resist_nexus)
			{
				msg_print(Ind, "You are unaffected!");
			}
			else if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
			}
			else
			{
				teleport_player_level(Ind);
			}
			update_smart_learn(m_idx, DRS_NEXUS);
			break;
		}

		/* RF6_XXX5 */
		/* RF6_S_RNG */
		case 160+11:
		{
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically codes some RNGs.", m_name);
			for (k = 0; k < 6; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_RNG);
			}
			if (blind && count) msg_print(Ind, "You hear many things appear nearby.");
			break;
		}


		/* RF6_DARKNESS */
		case 160+12:
		{
			if (!direct) break;
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s gestures in shadow.", m_name);
			(void)unlite_area(Ind, 0, 3);
			break;
		}

		/* RF6_TRAPS */
		case 160+13:
		{
			if (!direct) break;
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles, and then cackles evilly.", m_name);
			else msg_format(Ind, "%^s casts a spell and cackles evilly.", m_name);
			(void)trap_creation(Ind, 3, magik(rlev) ? (magik(30) ? 3 : 2) : 1);
			break;
		}

		/* RF6_FORGET */
		case 160+14:
		{
			if (!direct) break;
			disturb(Ind, 1, 0);
			msg_format(Ind, "%^s tries to blank your mind.", m_name);

			if (rand_int(100) < p_ptr->skill_sav)
			{
				msg_print(Ind, "You resist the effects!");
			}
			else if (lose_all_info(Ind))
			{
				msg_print(Ind, "Your memories fade away.");
			}
			break;
		}

		/* RF6_XXX6X6 */
		/* RF6_S_DRAGONRIDER */
		case 160+15:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons a DragonRider!", m_name);
			for (k = 0; k < 1; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_DRAGONRIDER);
			}
			if (blind && count) msg_print(Ind, "You hear something appear nearby.");
			break;
		}


		/* RF6_XXX7X6 */
		/* RF6_SUMMON_KIN */
		case 160+16:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons %s %s.",
					m_name, m_poss,
					((r_ptr->flags1) & RF1_UNIQUE ?
					 "minions" : "kin"));
			summon_kin_type = r_ptr->d_char; /* Big hack */

			for (k = 0; k < 6; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_KIN);
			}
			if (blind && count) msg_print(Ind, "You hear many things appear nearby.");

			break;
		}

		/* RF6_XXX8X6 */
		/* RF6_S_HI_DEMON */
		case 160+17:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons greater demons!", m_name);
			if (blind && count) msg_print(Ind, "You hear heavy steps nearby.");
			summon_cyber(Ind);
			break;
		}

		/* RF6_S_MONSTER */
		case 160+18:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons help!", m_name);
			for (k = 0; k < 1; k++)
			{
				count += summon_specific(wpos, y, x, rlev, 0);
			}
			if (blind && count) msg_print(Ind, "You hear something appear nearby.");
			break;
		}

		/* RF6_S_MONSTERS */
		case 160+19:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons monsters!", m_name);
			for (k = 0; k < 8; k++)
			{
				count += summon_specific(wpos, y, x, rlev, 0);
			}
			if (blind && count) msg_print(Ind, "You hear many things appear nearby.");
			break;
		}

		/* RF6_S_ANT */
		case 160+20:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons ants.", m_name);
			for (k = 0; k < 6; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_ANT);
			}
			if (blind && count) msg_print(Ind, "You hear many things appear nearby.");
			break;
		}

		/* RF6_S_SPIDER */
		case 160+21:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons spiders.", m_name);
			for (k = 0; k < 6; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_SPIDER);
			}
			if (blind && count) msg_print(Ind, "You hear many things appear nearby.");
			break;
		}

		/* RF6_S_HOUND */
		case 160+22:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons hounds.", m_name);
			for (k = 0; k < 6; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_HOUND);
			}
			if (blind && count) msg_print(Ind, "You hear many things appear nearby.");
			break;
		}

		/* RF6_S_HYDRA */
		case 160+23:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons hydras.", m_name);
			for (k = 0; k < 6; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_HYDRA);
			}
			if (blind && count) msg_print(Ind, "You hear many things appear nearby.");
			break;
		}

		/* RF6_S_ANGEL */
		case 160+24:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons an angel!", m_name);
			for (k = 0; k < 1; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_ANGEL);
			}
			if (blind && count) msg_print(Ind, "You hear something appear nearby.");
			break;
		}

		/* RF6_S_DEMON */
		case 160+25:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons a hellish adversary!", m_name);
			for (k = 0; k < 1; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_DEMON);
			}
			if (blind && count) msg_print(Ind, "You hear something appear nearby.");
			break;
		}

		/* RF6_S_UNDEAD */
		case 160+26:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons an undead adversary!", m_name);
			for (k = 0; k < 1; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_UNDEAD);
			}
			if (blind && count) msg_print(Ind, "You hear something appear nearby.");
			break;
		}

		/* RF6_S_DRAGON */
		case 160+27:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons a dragon!", m_name);
			for (k = 0; k < 1; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_DRAGON);
			}
			if (blind && count) msg_print(Ind, "You hear something appear nearby.");
			break;
		}

		/* RF6_S_HI_UNDEAD */
		case 160+28:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons greater undead!", m_name);
			for (k = 0; k < 8; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_HI_UNDEAD);
			}
			if (blind && count)
			{
				msg_print(Ind, "You hear many creepy things appear nearby.");
			}
			break;
		}

		/* RF6_S_HI_DRAGON */
		case 160+29:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons ancient dragons!", m_name);
			for (k = 0; k < 8; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_HI_DRAGON);
			}
			if (blind && count)
			{
				msg_print(Ind, "You hear many powerful things appear nearby.");
			}
			break;
		}

		/* RF6_S_WRAITH */
		case 160+30:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons mighty undead opponents!", m_name);
			for (k = 0; k < 8; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_WRAITH);
			}
			for (k = 0; k < 8; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_HI_UNDEAD);
			}
			if (blind && count)
			{
				msg_print(Ind, "You hear many creepy things appear nearby.");
			}
			break;
		}

		/* RF6_S_UNIQUE */
		case 160+31:
		{
			if (monst_check_antimagic(Ind, m_idx)) break;
			disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s magically summons special opponents!", m_name);
			for (k = 0; k < 8; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_UNIQUE);
			}
			for (k = 0; k < 8; k++)
			{
				count += summon_specific(wpos, y, x, rlev, SUMMON_HI_UNDEAD);
			}
			if (blind && count)
			{
				msg_print(Ind, "You hear many powerful things appear nearby.");
			}
			break;
		}
	}


	/* Remember what the monster did to us */
	if (seen)
	{
		/* Inate spell */
		if (thrown_spell < 32*4)
		{
			r_ptr->r_flags4 |= (1L << (thrown_spell - 32*3));
			if (r_ptr->r_cast_inate < MAX_UCHAR) r_ptr->r_cast_inate++;
		}

		/* Bolt or Ball */
		else if (thrown_spell < 32*5)
		{
			r_ptr->r_flags5 |= (1L << (thrown_spell - 32*4));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}

		/* Special spell */
		else if (thrown_spell < 32*6)
		{
			r_ptr->r_flags6 |= (1L << (thrown_spell - 32*5));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}
	}


	/* Always take note of monsters that kill you */
	if (p_ptr->death && (r_ptr->r_deaths < MAX_SHORT)) r_ptr->r_deaths++;


	/* A spell was cast */
	return (TRUE);
}


/*
 * Returns whether a given monster will try to run from the player.
 *
 * Monsters will attempt to avoid very powerful players.  See below.
 *
 * Because this function is called so often, little details are important
 * for efficiency.  Like not using "mod" or "div" when possible.  And
 * attempting to check the conditions in an optimal order.  Note that
 * "(x << 2) == (x * 4)" if "x" has enough bits to hold the result.
 *
 * Note that this function is responsible for about one to five percent
 * of the processor use in normal conditions...
 */
int mon_will_run(int Ind, int m_idx)
{
	player_type *p_ptr = Players[Ind];

	monster_type *m_ptr = &m_list[m_idx];

	cave_type **zcave;
#ifdef ALLOW_TERROR

        monster_race *r_ptr = race_inf(m_ptr);

	u16b p_lev, m_lev;
	u16b p_chp, p_mhp;
	u16b m_chp, m_mhp;
	u32b p_val, m_val;

	/* Hack -- aquatic life outa water */
	if(!(zcave=getcave(&m_ptr->wpos))) return(FALSE); // I'll run instead!

	if (zcave[m_ptr->fy][m_ptr->fx].feat != FEAT_WATER)
	{
		if (r_ptr->flags7 & RF7_AQUATIC) return (TRUE);
	}
	else
	{
		if (!(r_ptr->flags3 & RF3_UNDEAD) &&
				!(r_ptr->flags7 & (RF7_AQUATIC | RF7_CAN_SWIM | RF7_CAN_FLY) ))
			return(TRUE);
	}

#endif

	/* Keep monsters from running too far away */
	if (m_ptr->cdis > MAX_SIGHT + 5) return (FALSE);

	/* All "afraid" monsters will run away */
	if (m_ptr->monfear) return (TRUE);

#ifdef ALLOW_TERROR

	/* Nearby monsters will not become terrified */
	if (m_ptr->cdis <= 5) return (FALSE);

	/* Examine player power (level) */
	p_lev = p_ptr->lev;

	/* Examine monster power (level plus morale) */
//	m_lev = r_ptr->level + (m_idx & 0x08) + 25;
	/* Hack.. baby don't run.. */
	m_lev = r_ptr->level * 3 / 2 + (m_idx & 0x08) + 25;

	/* Optimize extreme cases below */
	if (m_lev > p_lev + 4) return (FALSE);
	if (m_lev + 4 <= p_lev) return (TRUE);

	/* Examine player health */
	p_chp = p_ptr->chp;
	p_mhp = p_ptr->mhp;

	/* Examine monster health */
	m_chp = m_ptr->hp;
	m_mhp = m_ptr->maxhp;

	/* Prepare to optimize the calculation */
	p_val = (p_lev * p_mhp) + (p_chp << 2);	/* div p_mhp */
	m_val = (m_lev * m_mhp) + (m_chp << 2);	/* div m_mhp */

	/* Strong players scare strong monsters */
	if (p_val * m_mhp > m_val * p_mhp) return (TRUE);

#endif

	/* Assume no terror */
	return (FALSE);
}




#ifdef MONSTER_FLOW

/*
 * Choose the "best" direction for "flowing"
 *
 * Note that ghosts and rock-eaters are never allowed to "flow",
 * since they should move directly towards the player.
 *
 * Prefer "non-diagonal" directions, but twiddle them a little
 * to angle slightly towards the player's actual location.
 *
 * Allow very perceptive monsters to track old "spoor" left by
 * previous locations occupied by the player.  This will tend
 * to have monsters end up either near the player or on a grid
 * recently occupied by the player (and left via "teleport").
 *
 * Note that if "smell" is turned on, all monsters get vicious.
 *
 * Also note that teleporting away from a location will cause
 * the monsters who were chasing you to converge on that location
 * as long as you are still near enough to "annoy" them without
 * being close enough to chase directly.  I have no idea what will
 * happen if you combine "smell" with low "aaf" values.
 */
static bool get_moves_aux(int m_idx, int *yp, int *xp)
{
	int i, y, x, y1, x1, when = 0, cost = 999;

	cave_type *c_ptr;

	monster_type *m_ptr = &m_list[m_idx];
        monster_race *r_ptr = race_inf(m_ptr);

	/* Monster flowing disabled */
	if (!flow_by_sound) return (FALSE);

	/* Monster can go through rocks */
	if (r_ptr->flags2 & RF2_PASS_WALL) return (FALSE);
	if (r_ptr->flags2 & RF2_KILL_WALL) return (FALSE);

	/* Monster location */
	y1 = m_ptr->fy;
	x1 = m_ptr->fx;

	/* Monster grid */
	c_ptr = &cave[y1][x1];

	/* The player is not currently near the monster grid */
	if (c_ptr->when < cave[py][px].when)
	{
		/* The player has never been near the monster grid */
		if (!c_ptr->when) return (FALSE);

		/* The monster is not allowed to track the player */
		if (!flow_by_smell) return (FALSE);
	}

	/* Monster is too far away to notice the player */
	if (c_ptr->cost > MONSTER_FLOW_DEPTH) return (FALSE);
	if (c_ptr->cost > r_ptr->aaf) return (FALSE);

	/* Hack -- Player can see us, run towards him */
	if (player_has_los_bold(y1, x1)) return (FALSE);

	/* Check nearby grids, diagonals first */
	for (i = 7; i >= 0; i--)
	{
		/* Get the location */
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];

		/* Ignore illegal locations */
		if (!cave[y][x].when) continue;

		/* Ignore ancient locations */
		if (cave[y][x].when < when) continue;

		/* Ignore distant locations */
		if (cave[y][x].cost > cost) continue;

		/* Save the cost and time */
		when = cave[y][x].when;
		cost = cave[y][x].cost;

		/* Hack -- Save the "twiddled" location */
		(*yp) = py + 16 * ddy_ddd[i];
		(*xp) = px + 16 * ddx_ddd[i];
	}

	/* No legal move (?) */
	if (!when) return (FALSE);

	/* Success */
	return (TRUE);
}

#endif


#if SAFETY_RADIUS > 0
/*
* Choose a "safe" location near a monster for it to run toward.
*
* A location is "safe" if it can be reached quickly and the player
* is not able to fire into it (it isn't a "clean shot").  So, this will
* cause monsters to "duck" behind walls.  Hopefully, monsters will also
* try to run towards corridor openings if they are in a room.
*
* This function may take lots of CPU time if lots of monsters are
* fleeing.
*
* Return TRUE if a safe location is available.
*/
static bool find_safety(int Ind, int m_idx, int *yp, int *xp)
{
	player_type *p_ptr = Players[Ind];
	monster_type *m_ptr = &m_list[m_idx];

	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x, d = 1, dis, i;
	int gy = 0, gx = 0, gdis = 0;

	cave_type **zcave;
	/* paranoia */
	if(!(zcave=getcave(&m_ptr->wpos))) return(FALSE);

	/* Start with adjacent locations, spread further */
	for (i = 1; i <= tdi[SAFETY_RADIUS]; i++)
	{
		if (i == tdi[d])
		{
			d++;
			if (gdis) break;
		}

		y = fy + tdy[i];
		x = fx + tdx[i];

		/* Skip illegal locations */
		if (!in_bounds(y, x)) continue;

		/* Skip locations in a wall */
		if (!cave_floor_bold(zcave, y, x)) continue;

#ifdef MONSTER_FLOW
		/* Check for "availability" (if monsters can flow) */
		if (flow_by_sound)
		{
			/* Ignore grids very far from the player */
			if (cave[y][x].when < cave[py][px].when) continue;

			/* Ignore too-distant grids */
			if (cave[y][x].cost > cave[fy][fx].cost + 2 * d) continue;
		}
#endif
		/* Check for absence of shot */
		if (!projectable(&m_ptr->wpos, y, x, py, px))
		{
			/* Calculate distance from player */
			dis = distance(y, x, py, px);

			/* Remember if further than previous */
			if (dis > gdis)
			{
				gy = y;
				gx = x;
				gdis = dis;
			}
		}
	}

	/* Check for success */
	if (gdis > 0)
	{
		/* Good location */
		(*yp) = fy - gy;
		(*xp) = fx - gx;

		/* Found safe place */
		return (TRUE);
	}

#if 0
	/* Start with adjacent locations, spread further */
	for (d = 1; d < SAFETY_RADIUS; d++)
	{
		/* Check nearby locations */
		for (y = fy - d; y <= fy + d; y++)
		{
			for (x = fx - d; x <= fx + d; x++)
			{
				/* Skip illegal locations */
				if (!in_bounds(y, x)) continue;

				/* Skip locations in a wall */
				if (!cave_floor_bold(zcave, y, x)) continue;

				/* Check distance */
				if (distance(y, x, fy, fx) != d) continue;
#ifdef MONSTER_FLOW
				/* Check for "availability" (if monsters can flow) */
				if (flow_by_sound)
				{
					/* Ignore grids very far from the player */
					if (cave[y][x].when < cave[py][px].when) continue;

					/* Ignore too-distant grids */
					if (cave[y][x].cost > cave[fy][fx].cost + 2 * d) continue;
				}
#endif
				/* Check for absence of shot */
				if (!projectable(&m_ptr->wpos, y, x, py, px))
				{
					/* Calculate distance from player */
					dis = distance(y, x, py, px);

					/* Remember if further than previous */
					if (dis > gdis)
					{
						gy = y;
						gx = x;
						gdis = dis;
					}
				}
			}
		}

		/* Check for success */
		if (gdis > 0)
		{
			/* Good location */
			(*yp) = fy - gy;
			(*xp) = fx - gx;

			/* Found safe place */
			return (TRUE);
		}
	}
#endif	// 0

	/* No safe place */
	return (FALSE);
}
#endif // SAFETY_RADIUS


#ifdef MONSTERS_HIDE_HEADS
/*
 * Choose a good hiding place near a monster for it to run toward.
 *
 * Pack monsters will use this to "ambush" the player and lure him out
 * of corridors into open space so they can swarm him.
 *
 * Return TRUE if a good location is available.
 */
/*
 * I did ported this, however, seemingly AI_ANNOY works 50 times
 * better than this function :(			- Jir -
 */
static bool find_hiding(int Ind, int m_idx, int *yp, int *xp)
{
	player_type *p_ptr = Players[Ind];
	monster_type *m_ptr = &m_list[m_idx];

	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x, d=1, dis, i;
	int gy = 0, gx = 0, gdis = 999, min;

	cave_type **zcave;
	/* paranoia */
	if(!(zcave=getcave(&m_ptr->wpos))) return(FALSE);

	/* Closest distance to get */
	min = distance(py, px, fy, fx) * 3 / 4 + 2;

	/* Start with adjacent locations, spread further */
	for (i = 1; i <= tdi[SAFETY_RADIUS]; i++)
	{
		if (i == tdi[d])
		{
			d++;
			if (gdis < 999) break;
		}

		y = fy + tdy[i];
		x = fx + tdx[i];

		/* Skip illegal locations */
		if (!in_bounds(y, x)) continue;

		/* Skip locations in a wall */
		if (!cave_floor_bold(zcave, y, x)) continue;

		/* Check for hidden, available grid */
		if (!player_can_see_bold(Ind, y, x) && clean_shot(&m_ptr->wpos, fy, fx, y, x))
		{
			/* Calculate distance from player */
			dis = distance(y, x, py, px);

			/* Remember if closer than previous */
			if (dis < gdis && dis >= min)
			{
				gy = y;
				gx = x;
				gdis = dis;
			}
		}
	}

	/* Check for success */
	if (gdis < 999)
	{
		/* Good location */
		(*yp) = fy - gy;
		(*xp) = fx - gx;

		/* Found good place */
		return (TRUE);
	}

#if 0
	/* Start with adjacent locations, spread further */
	for (d = 1; d < SAFETY_RADIUS; d++)
	{
		/* Check nearby locations */
		for (y = fy - d; y <= fy + d; y++)
		{
			for (x = fx - d; x <= fx + d; x++)
			{
				/* Skip illegal locations */
				if (!in_bounds(y, x)) continue;

				/* Skip locations in a wall */
				if (!cave_floor_bold(zcave, y, x)) continue;

				/* Check distance */
				if (distance(y, x, fy, fx) != d) continue;

				/* Check for hidden, available grid */
				if (!player_can_see_bold(Ind, y, x) && clean_shot(&m_ptr->wpos, fy, fx, y, x))
				{
					/* Calculate distance from player */
					dis = distance(y, x, py, px);

					/* Remember if closer than previous */
					if (dis < gdis && dis >= min)
					{
						gy = y;
						gx = x;
						gdis = dis;
					}
				}
			}
		}

		/* Check for success */
		if (gdis < 999)
		{
			/* Good location */
			(*yp) = fy - gy;
			(*xp) = fx - gx;

			/* Found good place */
			return (TRUE);
		}
	}
#endif	// 0

	/* No good place */
	return (FALSE);
}
#endif	// MONSTERS_HIDE_HEADS


static bool monster_can_pickup(monster_race *r_ptr, object_type *o_ptr)
{
	u32b f1, f2, f3, f4, f5, esp;
	u32b flg3 = 0L;

	if (artifact_p(o_ptr)) return (FALSE);

	/* Extract some flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* React to objects that hurt the monster */
	if (f1 & TR1_KILL_DRAGON) flg3 |= RF3_DRAGON;
	if (f1 & TR1_SLAY_DRAGON) flg3 |= RF3_DRAGON;
	if (f1 & TR1_SLAY_TROLL) flg3 |= RF3_TROLL;
	if (f1 & TR1_SLAY_GIANT) flg3 |= RF3_GIANT;
	if (f1 & TR1_SLAY_ORC) flg3 |= RF3_ORC;
	if (f1 & TR1_SLAY_DEMON) flg3 |= RF3_DEMON;
	if (f1 & TR1_SLAY_UNDEAD) flg3 |= RF3_UNDEAD;
	if (f1 & TR1_SLAY_ANIMAL) flg3 |= RF3_ANIMAL;
	if (f1 & TR1_SLAY_EVIL) flg3 |= RF3_EVIL;

	/* The object cannot be picked up by the monster */
	if (r_ptr->flags3 & flg3) return (FALSE);

	/* Ok */
	return (TRUE);
}

static int digging_difficulty(byte feat)
{
	if (feat >= FEAT_WALL_EXTRA) return (200);
	else if (feat >= FEAT_MAGMA)
	{
		if ((feat - FEAT_MAGMA) & 0x01) return (100);
		else return (50);
	}
	else if (feat == FEAT_RUBBLE) return (30);
	else if (feat == FEAT_TREE) return (50);
	else if (feat == FEAT_EVIL_TREE) return (30);	/* hehe it's evil */

	/* huh? ...it's not our role */
	return (3000);
}

/*
 * Choose "logical" directions for monster movement
 */
/*
 * TODO: Aquatic out of water should rush for one
 */
static void get_moves(int Ind, int m_idx, int *mm)
{
	player_type *p_ptr = Players[Ind];

	monster_type *m_ptr = &m_list[m_idx];
        monster_race *r_ptr = race_inf(m_ptr);

	int y, ay, x, ax;

	int move_val = 0;

	int y2 = p_ptr->py;
	int x2 = p_ptr->px;
	bool done = FALSE;	// not used fully (FIXME)


#ifdef MONSTER_FLOW
	/* Flow towards the player */
	if (flow_by_sound)
	{
		/* Flow towards the player */
		(void)get_moves_aux(m_idx, &y2, &x2);
	}
#endif

	/* Extract the "pseudo-direction" */
	y = m_ptr->fy - y2;
	x = m_ptr->fx - x2;

	if (r_ptr->flags1 & RF1_NEVER_MOVE) done = TRUE;

	/* Apply fear if possible and necessary */
	else if (mon_will_run(Ind, m_idx))
	{
#if SAFETY_RADIUS == 0
		/* XXX XXX Not very "smart" */
		y = (-y), x = (-x);
#else
		/* Try to find safe place */
		if (!find_safety(Ind, m_idx, &y, &x))
		{
			/* This is not a very "smart" method XXX XXX */
			y = (-y);
			x = (-x);
		}
#endif	// SAFETY_RADIUS
		done = TRUE;
	}


	/* Tease the player */
	else if (r_ptr->flags7 & RF7_AI_ANNOY)
	{
		if (distance(m_ptr->fy, m_ptr->fx, y2, x2) < ANNOY_DISTANCE)
		{
			y = -y;
			x = -x;
			/* so that they never get reversed again */
			done = TRUE;
		}
	}
#if 0
	/* Death orbs .. */
	if (r_ptr->flags2 & RF2_DEATH_ORB)
	{
		if (!los(m_ptr->fy, m_ptr->fx, y2, x2))
		{
			return (FALSE);
		}
	}
#endif	// 0

//        if (!stupid_monsters && (is_friend(m_ptr) < 0))
#ifndef STUPID_MONSTERS
	if (!done)
	{
		int tx = x2, ty = y2;
		cave_type **zcave;
		/* paranoia */
		if(!(zcave=getcave(&m_ptr->wpos))) return;
#ifdef	MONSTERS_HIDE_HEADS
		/*
		 * Animal packs try to get the player out of corridors
		 * (...unless they can move through walls -- TY)
		 */
		if ((r_ptr->flags1 & RF1_FRIENDS) &&
				(r_ptr->flags3 & RF3_ANIMAL) &&
				!((r_ptr->flags2 & (RF2_PASS_WALL)) ||
					(r_ptr->flags2 & (RF2_KILL_WALL))))
		{
			int i, room = 0;

			/* Count room grids next to player */
			for (i = 0; i < 8; i++)
			{
				/* Check grid */
				if (zcave[ty + ddy_ddd[i]][tx + ddx_ddd[i]].info & (CAVE_ROOM))
				{
					/* One more room grid */
					room++;
				}
			}

			/* Not in a room and strong player */
			if ((room < 8) && (p_ptr->chp > ((p_ptr->mhp * 3) / 4)))
			{
				/* Find hiding place */
				if (find_hiding(Ind, m_idx, &y, &x)) done = TRUE;
			}
		}
#endif	// MONSTERS_HIDE_HEADS
#ifdef	MONSTERS_GREEDY
		/* Monsters try to pick things up */
		if (!done && (r_ptr->flags2 & (RF2_TAKE_ITEM | RF2_KILL_ITEM)) &&
			(zcave[m_ptr->fy][m_ptr->fx].o_idx) && magik(80))
		{
			object_type *o_ptr = &o_list[zcave[m_ptr->fy][m_ptr->fx].o_idx];
			if (o_ptr->k_idx &&
#ifndef MONSTER_PICKUP_GOLD
				(o_ptr->tval != TV_GOLD) &&
#endif	// MONSTER_PICKUP_GOLD
				(o_ptr->tval != TV_KEY) &&
				(monster_can_pickup(r_ptr, o_ptr)))
			{
				/* Just Stay */
				return;
			}
		}
		if (!done && (r_ptr->flags2 & RF2_TAKE_ITEM) && magik(MONSTERS_GREEDY))
		{
			int i;

			/* Find an empty square near the target to fill */
			for (i = 0; i < 8; i++)
			{
				/* Pick squares near itself (semi-randomly) */
				y2 = m_ptr->fy + ddy_ddd[(m_idx + i) & 7];
				x2 = m_ptr->fx + ddx_ddd[(m_idx + i) & 7];

				/* Ignore filled grids */
				if (!cave_empty_bold(zcave, y2, x2)) continue;

				/* look for booty */
				if (zcave[y2][x2].o_idx)
				{
					object_type *o_ptr = &o_list[zcave[y2][x2].o_idx];
					if (o_ptr->k_idx &&
#ifndef MONSTER_PICKUP_GOLD
							(o_ptr->tval != TV_GOLD) &&
#endif	// MONSTER_PICKUP_GOLD
							(o_ptr->tval != TV_KEY) &&
							(monster_can_pickup(r_ptr, o_ptr)))
					{
						/* Extract the new "pseudo-direction" */
						y = m_ptr->fy - y2;
						x = m_ptr->fx - x2;

						/* Done */
						done = TRUE;

						break;
					}
				}
			}
		}
#endif	// MONSTERS_GREEDY
#ifdef	MONSTERS_HEMM_IN
		/* Monster groups try to surround the player */
		if (!done && (r_ptr->flags1 & RF1_FRIENDS))
		{
			int i;

			/* Find an empty square near the target to fill */
			for (i = 0; i < 8; i++)
			{
				/* Pick squares near target (semi-randomly) */
				y2 = ty + ddy_ddd[(m_idx + i) & 7];
				x2 = tx + ddx_ddd[(m_idx + i) & 7];

				/* Already there? */
				if ((m_ptr->fy == y2) && (m_ptr->fx == x2))
				{
					/* Attack the target */
					y2 = ty;
					x2 = tx;

					break;
				}

				/* Ignore filled grids */
				if (!cave_empty_bold(zcave, y2, x2)) continue;

				/* Try to fill this hole */
				break;
			}

			/* Extract the new "pseudo-direction" */
			y = m_ptr->fy - y2;
			x = m_ptr->fx - x2;

			/* Done */
			done = TRUE;
		}
	}
#endif	// MONSTERS_HEMM_IN
#endif	// STUPID_MONSTERS


	/* Extract the "absolute distances" */
	ax = ABS(x);
	ay = ABS(y);

	/* Do something weird */
	if (y < 0) move_val += 8;
	if (x > 0) move_val += 4;

	/* Prevent the diamond maneuvre */
	if (ay > (ax << 1))
	{
		move_val++;
		move_val++;
	}
	else if (ax > (ay << 1))
	{
		move_val++;
	}

	/* Extract some directions */
	switch (move_val)
	{
		case 0:
		mm[0] = 9;
		if (ay > ax)
		{
			mm[1] = 8;
			mm[2] = 6;
			mm[3] = 7;
			mm[4] = 3;
		}
		else
		{
			mm[1] = 6;
			mm[2] = 8;
			mm[3] = 3;
			mm[4] = 7;
		}
		break;
		case 1:
		case 9:
		mm[0] = 6;
		if (y < 0)
		{
			mm[1] = 3;
			mm[2] = 9;
			mm[3] = 2;
			mm[4] = 8;
		}
		else
		{
			mm[1] = 9;
			mm[2] = 3;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
		case 2:
		case 6:
		mm[0] = 8;
		if (x < 0)
		{
			mm[1] = 9;
			mm[2] = 7;
			mm[3] = 6;
			mm[4] = 4;
		}
		else
		{
			mm[1] = 7;
			mm[2] = 9;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
		case 4:
		mm[0] = 7;
		if (ay > ax)
		{
			mm[1] = 8;
			mm[2] = 4;
			mm[3] = 9;
			mm[4] = 1;
		}
		else
		{
			mm[1] = 4;
			mm[2] = 8;
			mm[3] = 1;
			mm[4] = 9;
		}
		break;
		case 5:
		case 13:
		mm[0] = 4;
		if (y < 0)
		{
			mm[1] = 1;
			mm[2] = 7;
			mm[3] = 2;
			mm[4] = 8;
		}
		else
		{
			mm[1] = 7;
			mm[2] = 1;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
		case 8:
		mm[0] = 3;
		if (ay > ax)
		{
			mm[1] = 2;
			mm[2] = 6;
			mm[3] = 1;
			mm[4] = 9;
		}
		else
		{
			mm[1] = 6;
			mm[2] = 2;
			mm[3] = 9;
			mm[4] = 1;
		}
		break;
		case 10:
		case 14:
		mm[0] = 2;
		if (x < 0)
		{
			mm[1] = 3;
			mm[2] = 1;
			mm[3] = 6;
			mm[4] = 4;
		}
		else
		{
			mm[1] = 1;
			mm[2] = 3;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
		case 12:
		mm[0] = 1;
		if (ay > ax)
		{
			mm[1] = 2;
			mm[2] = 4;
			mm[3] = 3;
			mm[4] = 7;
		}
		else
		{
			mm[1] = 4;
			mm[2] = 2;
			mm[3] = 7;
			mm[4] = 3;
		}
		break;
	}
}

/*
 * Choose "logical" directions for monster golem movement
 * Returns TRUE to move, FALSE to stand still
 */
static bool get_moves_golem(int Ind, int m_idx, int *mm)
{
        player_type *p_ptr;

	monster_type *m_ptr = &m_list[m_idx];

	int y, ay, x, ax;

	int move_val = 0;

        int tm_idx = 0;

        int y2, x2;

        if (Ind > 0) p_ptr = Players[Ind];
        else p_ptr = NULL;

        /* Lets find a target */

        if ((p_ptr != NULL) && (m_ptr->mind & GOLEM_ATTACK) && p_ptr->target_who && (p_ptr->target_who > 0 || check_hostile(Ind, -p_ptr->target_who)))
        {
                tm_idx = p_ptr->target_who;
        }
        else if (m_ptr->mind & GOLEM_GUARD)
        {
                int sx, sy;
                s32b max_hp = 0;

                /* Scan grids around */
                for (sx = m_ptr->fx - 1; sx <= m_ptr->fx + 1; sx++)
                for (sy = m_ptr->fy - 1; sy <= m_ptr->fy + 1; sy++)
                {
                        cave_type *c_ptr;
			cave_type **zcave;
                        if (!in_bounds(sy, sx)) continue;

			/* ignore ourself */
			if(sx==m_ptr->fx && sy==m_ptr->fy) continue;

			/* no point if there are no players on depth */
			/* and it would crash anyway ;) */

			if(!(zcave=getcave(&m_ptr->wpos))) return FALSE;
                        c_ptr = &zcave[sy][sx];

			if(!c_ptr->m_idx) continue;

                        if (c_ptr->m_idx > 0)
                        {
                                if (max_hp < m_list[c_ptr->m_idx].maxhp)
                                {
                                        max_hp = m_list[c_ptr->m_idx].maxhp;
                                        tm_idx = c_ptr->m_idx;
                                }
                        }
                        else
                        {
                                if ((max_hp < Players[-c_ptr->m_idx]->mhp) && (m_ptr->owner != Players[-c_ptr->m_idx]->id))
                                {
                                        max_hp = Players[-c_ptr->m_idx]->mhp;
                                        tm_idx = c_ptr->m_idx;
                                }
                        }
                }
        }
        /* Nothing else to do ? */
        if ((p_ptr != NULL) && !tm_idx && (m_ptr->mind & GOLEM_FOLLOW))
        {
                tm_idx = -Ind;
        }

        if (!tm_idx) return FALSE;

	if(!(inarea(&m_ptr->wpos, (tm_idx>0)? &m_list[tm_idx].wpos:&Players[-tm_idx]->wpos))) return FALSE;

        y2 = (tm_idx > 0)?m_list[tm_idx].fy:Players[-tm_idx]->py;
        x2 = (tm_idx > 0)?m_list[tm_idx].fx:Players[-tm_idx]->px;

	/* Extract the "pseudo-direction" */
	y = m_ptr->fy - y2;
	x = m_ptr->fx - x2;

	/* Extract the "absolute distances" */
	ax = ABS(x);
	ay = ABS(y);

	/* Do something weird */
	if (y < 0) move_val += 8;
	if (x > 0) move_val += 4;

	/* Prevent the diamond maneuvre */
	if (ay > (ax << 1))
	{
		move_val++;
		move_val++;
	}
	else if (ax > (ay << 1))
	{
		move_val++;
	}

	/* Extract some directions */
	switch (move_val)
	{
		case 0:
		mm[0] = 9;
		if (ay > ax)
		{
			mm[1] = 8;
			mm[2] = 6;
			mm[3] = 7;
			mm[4] = 3;
		}
		else
		{
			mm[1] = 6;
			mm[2] = 8;
			mm[3] = 3;
			mm[4] = 7;
		}
		break;
		case 1:
		case 9:
		mm[0] = 6;
		if (y < 0)
		{
			mm[1] = 3;
			mm[2] = 9;
			mm[3] = 2;
			mm[4] = 8;
		}
		else
		{
			mm[1] = 9;
			mm[2] = 3;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
		case 2:
		case 6:
		mm[0] = 8;
		if (x < 0)
		{
			mm[1] = 9;
			mm[2] = 7;
			mm[3] = 6;
			mm[4] = 4;
		}
		else
		{
			mm[1] = 7;
			mm[2] = 9;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
		case 4:
		mm[0] = 7;
		if (ay > ax)
		{
			mm[1] = 8;
			mm[2] = 4;
			mm[3] = 9;
			mm[4] = 1;
		}
		else
		{
			mm[1] = 4;
			mm[2] = 8;
			mm[3] = 1;
			mm[4] = 9;
		}
		break;
		case 5:
		case 13:
		mm[0] = 4;
		if (y < 0)
		{
			mm[1] = 1;
			mm[2] = 7;
			mm[3] = 2;
			mm[4] = 8;
		}
		else
		{
			mm[1] = 7;
			mm[2] = 1;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
		case 8:
		mm[0] = 3;
		if (ay > ax)
		{
			mm[1] = 2;
			mm[2] = 6;
			mm[3] = 1;
			mm[4] = 9;
		}
		else
		{
			mm[1] = 6;
			mm[2] = 2;
			mm[3] = 9;
			mm[4] = 1;
		}
		break;
		case 10:
		case 14:
		mm[0] = 2;
		if (x < 0)
		{
			mm[1] = 3;
			mm[2] = 1;
			mm[3] = 6;
			mm[4] = 4;
		}
		else
		{
			mm[1] = 1;
			mm[2] = 3;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
		case 12:
		mm[0] = 1;
		if (ay > ax)
		{
			mm[1] = 2;
			mm[2] = 4;
			mm[3] = 3;
			mm[4] = 7;
		}
		else
		{
			mm[1] = 4;
			mm[2] = 2;
			mm[3] = 7;
			mm[4] = 3;
		}
		break;
	}

        return TRUE;
}



/*
 * Hack -- local "player stealth" value (see below)
 */
static u32b noise = 0L;


/*
 * Process a monster
 *
 * The monster is known to be within 100 grids of the player
 *
 * In several cases, we directly update the monster lore
 *
 * Note that a monster is only allowed to "reproduce" if there
 * are a limited number of "reproducing" monsters on the current
 * level.  This should prevent the level from being "swamped" by
 * reproducing monsters.  It also allows a large mass of mice to
 * prevent a louse from multiplying, but this is a small price to
 * pay for a simple multiplication method.
 *
 * XXX Monster fear is slightly odd, in particular, monsters will
 * fixate on opening a door even if they cannot open it.  Actually,
 * the same thing happens to normal monsters when they hit a door
 *
 * XXX XXX XXX In addition, monsters which *cannot* open or bash
 * down a door will still stand there trying to open it...
 *
 * XXX Technically, need to check for monster in the way
 * combined with that monster being in a wall (or door?)
 *
 * A "direction" of "5" means "pick a random direction".
 *
 * Note that the "Ind" specifies the player that the monster will go after.
 */
static void process_monster(int Ind, int m_idx)
{
	player_type *p_ptr = Players[Ind];

	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;

	monster_type	*m_ptr = &m_list[m_idx];
        monster_race    *r_ptr = race_inf(m_ptr);

	int			i, d, oy, ox, ny, nx;

	int			mm[8];

	cave_type    	*c_ptr;
	object_type 	*o_ptr;
	monster_type	*y_ptr;

	bool		do_turn;
	bool		do_move;
	bool		do_view;

	bool		did_open_door;
	bool		did_bash_door;
	bool		did_take_item;
	bool		did_kill_item;
	bool		did_move_body;
	bool		did_kill_body;
	bool		did_pass_wall;
	bool		did_kill_wall;


/* Hack -- don't process monsters on wilderness levels that have not
	   been regenerated yet.
	*/
	if(!(zcave=getcave(wpos))) return;

	/* Handle "sleep" */
	if (m_ptr->csleep)
	{
		u32b notice = 0;

		/* Hack -- handle non-aggravation */
		if (!p_ptr->aggravate) notice = rand_int(1024);

		/* Hack -- See if monster "notices" player */
		if ((notice * notice * notice) <= noise)
		{
			/* Hack -- amount of "waking" */
			int d = 1;

			/* Hack -- make sure the distance isn't zero */
			if (m_ptr->cdis == 0) m_ptr->cdis = 1;

			/* Wake up faster near the player */
			if (m_ptr->cdis < 50) d = (100 / m_ptr->cdis);

			/* Hack -- handle aggravation */
			if (p_ptr->aggravate) d = m_ptr->csleep;

			/* Still asleep */
			if (m_ptr->csleep > d)
			{
				/* Monster wakes up "a little bit" */
				m_ptr->csleep -= d;

				/* Notice the "not waking up" */
				if (p_ptr->mon_vis[m_idx])
				{
					/* Hack -- Count the ignores */
					if (r_ptr->r_ignore < MAX_UCHAR) r_ptr->r_ignore++;
				}
			}

			/* Just woke up */
			else
			{
				/* Reset sleep counter */
				m_ptr->csleep = 0;

				/* Notice the "waking up" */
				msg_print_near_monster(m_idx, "wakes up.");

#if 0
				if (p_ptr->mon_vis[m_idx])
				{
					char m_name[80];

					/* Acquire the monster name */
					monster_desc(Ind, m_name, m_idx, 0);

					/* Dump a message */
					msg_format(Ind, "%^s wakes up.", m_name);

					/* Hack -- Count the wakings */
					/* not used at all, seemingly */
					if (r_ptr->r_wake < MAX_UCHAR) r_ptr->r_wake++;
				}
#endif	// 0
			}
		}

		/* Still sleeping */
		if (m_ptr->csleep) 
		{
			m_ptr->energy -= level_speed(&m_ptr->wpos);
			return;
		}
	}


	/* Handle "stun" */
	if (m_ptr->stunned)
	{
		int d = 1;

		/* Make a "saving throw" against stun */
		if (rand_int(5000) <= r_ptr->level * r_ptr->level)
		{
			/* Recover fully */
			d = m_ptr->stunned;
		}

		/* Hack -- Recover from stun */
		if (m_ptr->stunned > d)
		{
			/* Recover somewhat */
			m_ptr->stunned -= d;
		}

		/* Fully recover */
		else
		{
			/* Recover fully */
			m_ptr->stunned = 0;

			/* Message if visible */
			msg_print_near_monster(m_idx, "is no longer stunned.");
#if 0
			if (p_ptr->mon_vis[m_idx])
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(Ind, m_name, m_idx, 0);

				/* Dump a message */
				msg_format(Ind, "\377o%^s is no longer stunned.", m_name);
			}
#endif	// 0
		}

		/* Still stunned */
		if (m_ptr->stunned > 100)
		{
			m_ptr->energy -= level_speed(&m_ptr->wpos);
			return;
		}
	}


	/* Handle confusion */
	if (m_ptr->confused)
	{
		/* Amount of "boldness" */
		int d = randint(r_ptr->level / 10 + 1);

		/* Still confused */
		if (m_ptr->confused > d)
		{
			/* Reduce the confusion */
			m_ptr->confused -= d;
		}

		/* Recovered */
		else
		{
			/* No longer confused */
			m_ptr->confused = 0;

			/* Message if visible */
			msg_print_near_monster(m_idx, "is no longer confused.");
#if 0
			if (p_ptr->mon_vis[m_idx])
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(Ind, m_name, m_idx, 0);

				/* Dump a message */
				msg_format(Ind, "%^s is no longer confused.", m_name);
			}
#endif	// 0
		}
	}


	/* Handle "fear" */
	if (m_ptr->monfear)
	{
		/* Amount of "boldness" */
		int d = randint(r_ptr->level / 10 + 1);

		/* Still afraid */
		if (m_ptr->monfear > d)
		{
			/* Reduce the fear */
			m_ptr->monfear -= d;
		}

		/* Recover from fear, take note if seen */
		else
		{
			/* No longer afraid */
			m_ptr->monfear = 0;

			/* Visual note */
			msg_print_near_monster(m_idx, "recovers the courage.");
#if 0
			if (p_ptr->mon_vis[m_idx])
			{
				char m_name[80];
				char m_poss[80];

				/* Acquire the monster name/poss */
				monster_desc(Ind, m_name, m_idx, 0);
				monster_desc(Ind, m_poss, m_idx, 0x22);

				/* Dump a message */
				msg_format(Ind, "%^s recovers %s courage.", m_name, m_poss);
			}
#endif	// 0
		}
	}

	/* Get the origin */
	oy = m_ptr->fy;
	ox = m_ptr->fx;
	
#if 0	// too bad hack!
	/* Hack -- aquatic life outa water */
	if (zcave[oy][ox].feat != FEAT_WATER)
	{
		if (r_ptr->flags7 & RF7_AQUATIC) m_ptr->monfear = 50;
	}
	else
	{
		if (!(r_ptr->flags3 & RF3_UNDEAD) &&
				!(r_ptr->flags7 & (RF7_AQUATIC | RF7_CAN_SWIM | RF7_CAN_FLY) ))
			m_ptr->monfear = 50;
	}
#endif	// 0


	/* attempt to "mutiply" if able and allowed */

	if((r_ptr->flags7 & RF7_MULTIPLY) &&
		(!istown(wpos) && (m_ptr->wpos.wz != 0 ||
		 wild_info[m_ptr->wpos.wy][m_ptr->wpos.wx].radius > 2) ) &&
		(num_repro < MAX_REPRO))
#if REPRO_RATE
		if (magik(REPRO_RATE))
#endif	// REPRO_RATE
		{
			int k, y, x;
			dun_level *l_ptr = getfloor(wpos);

			if (!(l_ptr && l_ptr->flags1 & LF1_NO_MULTIPLY))
			{
				/* Count the adjacent monsters */
				for (k = 0, y = oy - 1; y <= oy + 1; y++)
				{
					for (x = ox - 1; x <= ox + 1; x++)
					{
						if (zcave[y][x].m_idx > 0) k++;
					}
				}

				/* Hack -- multiply slower in crowded areas */
				if ((k < 4) && (!k || !rand_int(k * MON_MULT_ADJ)))
				{
					/* Try to multiply */
					if (multiply_monster(m_idx))
					{
						/* Take note if visible */
						if (p_ptr->mon_vis[m_idx]) r_ptr->flags7 |= RF7_MULTIPLY;

						/* Multiplying takes energy */
						m_ptr->energy -= level_speed(&m_ptr->wpos);

						return;
					}
				}
			}
		}
						
	/* Attempt to cast a spell */
	if (make_attack_spell(Ind, m_idx)) 
	{
		m_ptr->energy -= level_speed(&m_ptr->wpos);
		return;
	}


	/* Hack -- Assume no movement */
	mm[0] = mm[1] = mm[2] = mm[3] = 0;
	mm[4] = mm[5] = mm[6] = mm[7] = 0;


	/* Confused -- 100% random */
	if (m_ptr->confused)
	{
		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}

	/* 75% random movement */
	else if ((r_ptr->flags1 & RF1_RAND_50) &&
	         (r_ptr->flags1 & RF1_RAND_25) &&
	         (rand_int(100) < 75))
	{
		/* Memorize flags */
		if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags1 |= RF1_RAND_50;
		if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags1 |= RF1_RAND_25;

		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}

	/* 50% random movement */
	else if ((r_ptr->flags1 & RF1_RAND_50) &&
	         (rand_int(100) < 50))
	{
		/* Memorize flags */
		if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags1 |= RF1_RAND_50;

		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}

	/* 25% random movement */
	else if ((r_ptr->flags1 & RF1_RAND_25) &&
	         (rand_int(100) < 25))
	{
		/* Memorize flags */
		if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags1 |= RF1_RAND_25;

		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}

	/* Normal movement */
	else
	{
		/* Logical moves */
		get_moves(Ind, m_idx, mm);
	}


	/* Assume nothing */
	do_turn = FALSE;
	do_move = FALSE;
	do_view = FALSE;

	/* Assume nothing */
	did_open_door = FALSE;
	did_bash_door = FALSE;
	did_take_item = FALSE;
	did_kill_item = FALSE;
	did_move_body = FALSE;
	did_kill_body = FALSE;
	did_pass_wall = FALSE;
	did_kill_wall = FALSE;


	/* Take a zero-terminated array of "directions" */
	for (i = 0; mm[i]; i++)
	{
		/* Get the direction */
		d = mm[i];

		/* Hack -- allow "randomized" motion */
		if (d == 5) d = ddd[rand_int(8)];

		/* Get the destination */
		ny = oy + ddy[d];
		nx = ox + ddx[d];			
		
		/* Access that cave grid */
		c_ptr = &zcave[ny][nx];

		/* Access that cave grid's contents */
		o_ptr = &o_list[c_ptr->o_idx];

		/* Access that cave grid's contents */
		y_ptr = &m_list[c_ptr->m_idx];


		/* Tavern entrance? */
		if (c_ptr->feat == FEAT_SHOP_TAIL)
		{
			/* Nothing */
		}

		/* Floor is open? */
		else if (cave_floor_bold(zcave, ny, nx))
		{
			/* Go ahead and move */
			do_move = TRUE;
		}

		/* Player ghost in wall XXX */
		else if (c_ptr->m_idx < 0)
		{
			/* Move into player */
			do_move = TRUE;
		}

		/* Permanent wall */
		/* Hack: Morgie DIGS!! */
		else if ( (c_ptr->feat >= FEAT_PERM_EXTRA &&
				!((r_ptr->flags2 & RF2_KILL_WALL) &&
					(r_ptr->flags2 & RF2_PASS_WALL) &&
					(c_ptr->feat != FEAT_PERM_SOLID) && !rand_int(9000)))
				|| (c_ptr->feat == FEAT_PERM_CLEAR) )
		{
			/* Nothing */
		}

		/* Monster destroys walls (and doors) */
#ifdef MONSTER_DIG_FACTOR
		else if (r_ptr->flags2 & RF2_KILL_WALL ||
				(!(r_ptr->flags1 & RF1_NEVER_MOVE ||
				   r_ptr->flags2 & RF2_EMPTY_MIND ||
				   r_ptr->flags2 & RF2_STUPID)) &&
				(!rand_int(digging_difficulty(c_ptr->feat) * MONSTER_DIG_FACTOR)
				 && magik(5 + r_ptr->level))) 
#else
		else if (r_ptr->flags2 & RF2_KILL_WALL)
#endif
		{
			/* Eat through walls/doors/rubble */
			do_move = TRUE;

			/* Monster destroyed a wall */
			did_kill_wall = TRUE;

			/* Create floor */
//			c_ptr->feat = FEAT_FLOOR;
			c_ptr->feat = twall_erosion(wpos, ny, nx);

			/* Forget the "field mark", if any */
			everyone_forget_spot(wpos, ny, nx);

			/* Notice */
			note_spot_depth(wpos, ny, nx);

			/* Redraw */
			everyone_lite_spot(wpos, ny, nx);

			/* Note changes to viewable region */
			if (player_has_los_bold(Ind, ny, nx)) do_view = TRUE;
		}

		/* Monster moves through walls (and doors) */
		else if (r_ptr->flags2 & RF2_PASS_WALL)
		{
			/* Pass through walls/doors/rubble */
			do_move = TRUE;

			/* Monster went through a wall */
			did_pass_wall = TRUE;
		}

		/* Handle doors and secret doors */
		else if (((c_ptr->feat >= FEAT_DOOR_HEAD) &&
		          (c_ptr->feat <= FEAT_DOOR_TAIL)) ||
		         (c_ptr->feat == FEAT_SECRET))
		{
			bool may_bash = TRUE;

			/* Take a turn */
			do_turn = TRUE;

			/* Creature can open doors. */
			if (r_ptr->flags2 & RF2_OPEN_DOOR)
			{
				/* Closed doors and secret doors */
				if ((c_ptr->feat == FEAT_DOOR_HEAD) ||
				    (c_ptr->feat == FEAT_SECRET))
				{
					/* The door is open */
					did_open_door = TRUE;

					/* Do not bash the door */
					may_bash = FALSE;
				}

				/* Locked doors (not jammed) */
				else if (c_ptr->feat < FEAT_DOOR_HEAD + 0x08)
				{
					int k;

					/* Door power */
					k = ((c_ptr->feat - FEAT_DOOR_HEAD) & 0x07);

#if 0
					/* XXX XXX XXX XXX Old test (pval 10 to 20) */
					if (randint((m_ptr->hp + 1) * (50 + o_ptr->pval)) <
					    40 * (m_ptr->hp - 10 - o_ptr->pval));
#endif

					/* Try to unlock it XXX XXX XXX */
					if (rand_int(m_ptr->hp / 10) > k)
					{
						/* Unlock the door */
						c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

						/* Do not bash the door */
						may_bash = FALSE;
					}
				}
			}

			/* Stuck doors -- attempt to bash them down if allowed */
			if (may_bash && (r_ptr->flags2 & RF2_BASH_DOOR))
			{
				int k;

				/* Door power */
				k = ((c_ptr->feat - FEAT_DOOR_HEAD) & 0x07);

#if 0
				/* XXX XXX XXX XXX Old test (pval 10 to 20) */
				if (randint((m_ptr->hp + 1) * (50 + o_ptr->pval)) <
				    40 * (m_ptr->hp - 10 - o_ptr->pval));
#endif

				/* Attempt to Bash XXX XXX XXX */
				if (rand_int(m_ptr->hp / 10) > k)
				{
					/* Message */
//					msg_print(Ind, "You hear a door burst open!");
					msg_print_near_site(ny, nx, wpos, "You hear a door burst open!");

					/* Disturb (sometimes) */
					if (p_ptr->disturb_minor) disturb(Ind, 0, 0);

					/* The door was bashed open */
					did_bash_door = TRUE;

					/* Hack -- fall into doorway */
					do_move = TRUE;
				}
			}


			/* Deal with doors in the way */
			if (did_open_door || did_bash_door)
			{
				/* Break down the door */
				if (did_bash_door && (rand_int(100) < 50))
				{
					c_ptr->feat = FEAT_BROKEN;
				}

				/* Open the door */
				else
				{
					c_ptr->feat = FEAT_OPEN;
				}

				/* Notice */
				note_spot_depth(wpos, ny, nx);

				/* Redraw */
				everyone_lite_spot(wpos, ny, nx);

				/* Handle viewable doors */
				if (player_has_los_bold(Ind, ny, nx)) do_view = TRUE;
			}
		}


		/* Hack -- check for Glyph of Warding */
		if (do_move && (c_ptr->feat == FEAT_GLYPH))
		{
			/* Assume no move allowed */
			do_move = FALSE;

			/* Break the ward */
			if (randint(BREAK_GLYPH) < r_ptr->level)
			{
				/* Describe observable breakage */
				/* Prolly FIXME */
				msg_print_near_site(ny, nx, wpos, "The rune of protection is broken!");
#if 0
				if (Players[Ind]->cave_flag[ny][nx] & CAVE_MARK)
				{
					msg_print(Ind, "The rune of protection is broken!");
				}
#endif	// 0

				/* Break the rune */
				c_ptr->feat = FEAT_FLOOR;

				/* Allow movement */
				do_move = TRUE;
			}
		}

		/* Some monsters never attack */
		if (do_move && (ny == p_ptr->py) && (nx == p_ptr->px) &&
		    (r_ptr->flags1 & RF1_NEVER_BLOW))
		{
			/* Hack -- memorize lack of attacks */
			/* if (m_ptr->ml) r_ptr->r_flags1 |= RF1_NEVER_BLOW; */

			/* Do not move */
			do_move = FALSE;
		}


		/* The player is in the way.  Attack him. */
		if (do_move && (c_ptr->m_idx < 0))
		{
			/* Do the attack */
			(void)make_attack_normal(0 - c_ptr->m_idx, m_idx);

			/* Do not move */
			do_move = FALSE;

			/* Took a turn */
			do_turn = TRUE;
		}


		/* Some monsters never move */
		if (do_move && (r_ptr->flags1 & RF1_NEVER_MOVE))
		{
			/* Hack -- memorize lack of attacks */
			/* if (m_ptr->ml) r_ptr->r_flags1 |= RF1_NEVER_MOVE; */

			/* Do not move */
			do_move = FALSE;

			/* Hack -- use up the turn */
			do_turn = TRUE;
		}

		/* restrict aquatic life to the pond */
		if(do_move && (r_ptr->flags7 & RF7_AQUATIC)){
			if((c_ptr->feat != FEAT_WATER) &&
				(zcave[oy][ox].feat == FEAT_WATER)) do_move=FALSE;
		}

		/* Hack -- those that hate water */
		if (do_move && c_ptr->feat == FEAT_WATER)
		{
			if (!(r_ptr->flags3 & RF3_UNDEAD) &&
				!(r_ptr->flags7 & (RF7_AQUATIC | RF7_CAN_SWIM | RF7_CAN_FLY) ) &&
				(zcave[oy][ox].feat != FEAT_WATER))
				do_move = FALSE;
		}

		/* A monster is in the way */
		if (do_move && c_ptr->m_idx > 0)
		{
                        monster_race *z_ptr = race_inf(y_ptr);

			/* Assume no movement */
			do_move = FALSE;

			/* Kill weaker monsters */
			if ((r_ptr->flags2 & RF2_KILL_BODY) &&
			    (r_ptr->mexp > z_ptr->mexp) && !y_ptr->owner)
			{
				/* Allow movement */
				do_move = TRUE;

				/* Monster ate another monster */
				did_kill_body = TRUE;

				/* XXX XXX XXX Message */

				/* Kill the monster */
				delete_monster(wpos, ny, nx);

				/* Hack -- get the empty monster */
				y_ptr = &m_list[c_ptr->m_idx];
			}

			/* Push past weaker monsters (unless leaving a wall) */
			if ((r_ptr->flags2 & RF2_MOVE_BODY) &&
			    (r_ptr->mexp > z_ptr->mexp) &&
			    (cave_floor_bold(zcave, m_ptr->fy, m_ptr->fx)))
			{
				/* Allow movement */
				do_move = TRUE;

				/* Monster pushed past another monster */
				did_move_body = TRUE;

				/* XXX XXX XXX Message */
			}
		}

		/* Hack -- player hinders its movement */
		if (do_move && monst_check_grab(m_idx, 70, "run"))
		{
			/* Take a turn */
			do_turn = TRUE;

			do_move = FALSE;
		}


		/* Creature has been allowed to move */
		if (do_move)
		{
			/* Take a turn */
			do_turn = TRUE;

			/* Hack -- Update the old location */
			zcave[oy][ox].m_idx = c_ptr->m_idx;

			/* Mega-Hack -- move the old monster, if any */
			if (c_ptr->m_idx > 0)
			{
				/* Move the old monster */
				y_ptr->fy = oy;
				y_ptr->fx = ox;

				/* Update the old monster */
				update_mon(c_ptr->m_idx, TRUE);
			}

			/* Hack -- Update the new location */
			c_ptr->m_idx = m_idx;

			/* Move the monster */
			m_ptr->fy = ny;
			m_ptr->fx = nx;

			/* Update the monster */
			update_mon(m_idx, TRUE);

			/* Redraw the old grid */
			everyone_lite_spot(wpos, oy, ox);

			/* Redraw the new grid */
			everyone_lite_spot(wpos, ny, nx);

			/* Possible disturb */
			if (p_ptr->mon_vis[m_idx] &&
			    (p_ptr->disturb_move ||
			     (p_ptr->mon_los[m_idx] &&
			      p_ptr->disturb_near)) &&
				r_ptr->level != 0)
			{
				/* Disturb */
                                if (p_ptr->id != m_ptr->owner) disturb(Ind, 0, 0);
			}


			/* XXX XXX XXX Change for Angband 2.8.0 */

			/* Take or Kill objects (not "gold") on the floor */
			if (o_ptr->k_idx &&
#ifndef MONSTER_PICKUP_GOLD
				(o_ptr->tval != TV_GOLD) &&
#endif	// MONSTER_PICKUP_GOLD
			    (o_ptr->tval != TV_KEY) &&
			    ((r_ptr->flags2 & RF2_TAKE_ITEM) ||
			     (r_ptr->flags2 & RF2_KILL_ITEM)))
			{
				char m_name[80];
				char o_name[160];

				/* Check the grid */
				o_ptr = &o_list[c_ptr->o_idx];

				/* Acquire the object name */
				object_desc(Ind, o_name, o_ptr, TRUE, 3);

				/* Acquire the monster name */
				monster_desc(Ind, m_name, m_idx, 0x04);

				/* The object cannot be picked up by the monster */
				if (!monster_can_pickup(r_ptr, o_ptr))
				{
					/* Only give a message for "take_item" */
					if (r_ptr->flags2 & RF2_TAKE_ITEM)
					{
						/* Take note */
						did_take_item = TRUE;

						/* Describe observable situations */
						if (p_ptr->mon_vis[m_idx] && player_has_los_bold(Ind, ny, nx))
						{
							/* Dump a message */
							msg_format(Ind, "%^s tries to pick up %s, but fails.",
							           m_name, o_name);
						}
					}
				}

				/* Pick up the item */
				else if (r_ptr->flags2 & RF2_TAKE_ITEM)
				{
					s16b this_o_idx = 0;

					/* Take note */
					did_take_item = TRUE;

					/* Describe observable situations */
					if (player_has_los_bold(Ind, ny, nx))
					{
						/* Dump a message */
						msg_format(Ind, "%^s picks up %s.", m_name, o_name);
					}

#ifdef MONSTER_INVENTORY
					this_o_idx = c_ptr->o_idx;

#ifdef MONSTER_ITEM_CONSUME
					if (magik(MONSTER_ITEM_CONSUME))
					{
						/* Delete the object */
						delete_object_idx(this_o_idx);
					}
					else
#endif	// MONSTER_ITEM_CONSUME
					{
						/* Excise the object */
						excise_object_idx(this_o_idx);

						/* Forget mark */
						//					o_ptr->marked = FALSE;

						/* Forget location */
						o_ptr->iy = o_ptr->ix = 0;

						/* Memorize monster */
						o_ptr->held_m_idx = m_idx;

						/* Build a stack */
						o_ptr->next_o_idx = m_ptr->hold_o_idx;

						/* Carry object */
						m_ptr->hold_o_idx = this_o_idx;
					}
#else
					/* Delete the object */
					delete_object(wpos, ny, nx);
#endif	// MONSTER_INVENTORY
				}

				/* Destroy the item */
				else
				{
					/* Take note */
					did_kill_item = TRUE;

					/* Describe observable situations */
					if (player_has_los_bold(Ind, ny, nx))
					{
						/* Dump a message */
						msg_format(Ind, "%^s crushes %s.", m_name, o_name);
					}

					/* Delete the object */
					delete_object(wpos, ny, nx);
				}
			}
		}

		/* Stop when done */
		if (do_turn) 
		{
			m_ptr->energy -= level_speed(&m_ptr->wpos);
			break;
		}
	}

	/* Notice changes in view */
	if (do_view)
	{
		/* Update some things */
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
	}


	/* Learn things from observable monster */
	if (p_ptr->mon_vis[m_idx])
	{
		/* Monster opened a door */
		if (did_open_door) r_ptr->r_flags2 |= RF2_OPEN_DOOR;

		/* Monster bashed a door */
		if (did_bash_door) r_ptr->r_flags2 |= RF2_BASH_DOOR;

		/* Monster tried to pick something up */
		if (did_take_item) r_ptr->r_flags2 |= RF2_TAKE_ITEM;

		/* Monster tried to crush something */
		if (did_kill_item) r_ptr->r_flags2 |= RF2_KILL_ITEM;

		/* Monster pushed past another monster */
		if (did_move_body) r_ptr->r_flags2 |= RF2_MOVE_BODY;

		/* Monster ate another monster */
		if (did_kill_body) r_ptr->r_flags2 |= RF2_KILL_BODY;

		/* Monster passed through a wall */
		if (did_pass_wall) r_ptr->r_flags2 |= RF2_PASS_WALL;

		/* Monster destroyed a wall */
		if (did_kill_wall) r_ptr->r_flags2 |= RF2_KILL_WALL;
	}


	/* Hack -- get "bold" if out of options */
	if (!do_turn && !do_move && m_ptr->monfear)
	{
		/* No longer afraid */
		m_ptr->monfear = 0;

		/* Message if seen */
		if (p_ptr->mon_vis[m_idx])
		{
			char m_name[80];

			/* Acquire the monster name */
			monster_desc(Ind, m_name, m_idx, 0);

			/* Dump a message */
//			msg_format(Ind, "%^s turns to fight!", m_name);
			msg_print_near_monster(m_idx, "turns to fight!");
		}

		/* XXX XXX XXX Actually do something now (?) */
	}
}

static void process_monster_golem(int Ind, int m_idx)
{
        player_type *p_ptr;

	monster_type	*m_ptr = &m_list[m_idx];
        monster_race    *r_ptr = race_inf(m_ptr);
	struct worldpos *wpos=&m_ptr->wpos;

	int			i, d, oy, ox, ny, nx;

	int			mm[8];

	cave_type    	*c_ptr;
	object_type 	*o_ptr;
	monster_type	*y_ptr;

	bool		do_turn;
	bool		do_move;
	bool		do_view;

	bool		did_open_door;
	bool		did_bash_door;
	bool		did_take_item;
	bool		did_kill_item;
	bool		did_move_body;
	bool		did_kill_body;
	bool		did_pass_wall;
	bool		did_kill_wall;


        /* Hack -- don't process monsters on wilderness levels that have not
	   been regenerated yet.
	*/
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;

        if (Ind > 0) p_ptr = Players[Ind];
        else p_ptr = NULL;

	/* Handle "stun" */
	if (m_ptr->stunned)
	{
		int d = 1;

		/* Make a "saving throw" against stun */
		if (rand_int(5000) <= r_ptr->level * r_ptr->level)
		{
			/* Recover fully */
			d = m_ptr->stunned;
		}

		/* Hack -- Recover from stun */
		if (m_ptr->stunned > d)
		{
			/* Recover somewhat */
			m_ptr->stunned -= d;
		}

		/* Fully recover */
		else
		{
			/* Recover fully */
			m_ptr->stunned = 0;
		}

		/* Still stunned */
		if (m_ptr->stunned) 
		{
			m_ptr->energy -= level_speed(&m_ptr->wpos);
			return;
		}
	}


	/* Handle confusion */
	if (m_ptr->confused)
	{
		/* Amount of "boldness" */
		int d = randint(r_ptr->level / 10 + 1);

		/* Still confused */
		if (m_ptr->confused > d)
		{
			/* Reduce the confusion */
			m_ptr->confused -= d;
		}

		/* Recovered */
		else
		{
			/* No longer confused */
			m_ptr->confused = 0;
		}
	}

	/* Get the origin */
	oy = m_ptr->fy;
	ox = m_ptr->fx;
	
#if 0 /* No golem spells -- yet */
	/* Attempt to cast a spell */
	if (make_attack_spell(Ind, m_idx)) 
	{
		m_ptr->energy -= level_speed(&m_ptr->wpos);
		return;
	}
#endif

	/* Hack -- Assume no movement */
	mm[0] = mm[1] = mm[2] = mm[3] = 0;
	mm[4] = mm[5] = mm[6] = mm[7] = 0;


	/* Confused -- 100% random */
	if (m_ptr->confused)
	{
		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}
	/* Normal movement */
	else
	{
		/* Logical moves */
                if (!get_moves_golem(Ind, m_idx, mm)) return;
	}


	/* Assume nothing */
	do_turn = FALSE;
	do_move = FALSE;
	do_view = FALSE;

	/* Assume nothing */
	did_open_door = FALSE;
	did_bash_door = FALSE;
	did_take_item = FALSE;
	did_kill_item = FALSE;
	did_move_body = FALSE;
	did_kill_body = FALSE;
	did_pass_wall = FALSE;
	did_kill_wall = FALSE;


	/* Take a zero-terminated array of "directions" */
	for (i = 0; mm[i]; i++)
	{
		/* Get the direction */
		d = mm[i];

		/* Hack -- allow "randomized" motion */
		if (d == 5) d = ddd[rand_int(8)];

		/* Get the destination */
		ny = oy + ddy[d];
		nx = ox + ddx[d];			
		
		/* Access that cave grid */
		c_ptr = &zcave[ny][nx];

		/* Access that cave grid's contents */
		o_ptr = &o_list[c_ptr->o_idx];

		/* Access that cave grid's contents */
		y_ptr = &m_list[c_ptr->m_idx];


		/* Tavern entrance? */
		if (c_ptr->feat == FEAT_SHOP_TAIL)
		{
			/* Nothing */
		}

		/* Floor is open? */
		else if (cave_floor_bold(zcave, ny, nx))
		{
			/* Go ahead and move */
			do_move = TRUE;
		}

		/* Player ghost in wall XXX */
		else if (c_ptr->m_idx < 0)
		{
			/* Move into player */
			do_move = TRUE;
		}

		/* Permanent wall */
		else if ( (c_ptr->feat >= FEAT_PERM_EXTRA) || (c_ptr->feat == FEAT_PERM_CLEAR) )
		{
			/* Nothing */
		}

		/* Monster moves through walls (and doors) */
		else if (r_ptr->flags2 & RF2_PASS_WALL)
		{
			/* Pass through walls/doors/rubble */
			do_move = TRUE;

			/* Monster went through a wall */
			did_pass_wall = TRUE;
		}

		/* Monster destroys walls (and doors) */
		else if (r_ptr->flags2 & RF2_KILL_WALL)
		{
			/* Eat through walls/doors/rubble */
			do_move = TRUE;

			/* Monster destroyed a wall */
			did_kill_wall = TRUE;

			/* Create floor */
//			c_ptr->feat = FEAT_FLOOR;
			c_ptr->feat = twall_erosion(wpos, ny, nx);

			/* Forget the "field mark", if any */
			everyone_forget_spot(wpos, ny, nx);

			/* Notice */
			note_spot_depth(wpos, ny, nx);

			/* Redraw */
			everyone_lite_spot(wpos, ny, nx);

			/* Note changes to viewable region */
			if (player_has_los_bold(Ind, ny, nx)) do_view = TRUE;
		}

		/* Handle doors and secret doors */
		else if (((c_ptr->feat >= FEAT_DOOR_HEAD) &&
		          (c_ptr->feat <= FEAT_DOOR_TAIL)) ||
		         (c_ptr->feat == FEAT_SECRET))
		{
			bool may_bash = TRUE;

			/* Take a turn */
			do_turn = TRUE;

			/* Creature can open doors. */
			if (r_ptr->flags2 & RF2_OPEN_DOOR)
			{
				/* Closed doors and secret doors */
				if ((c_ptr->feat == FEAT_DOOR_HEAD) ||
				    (c_ptr->feat == FEAT_SECRET))
				{
					/* The door is open */
					did_open_door = TRUE;

					/* Do not bash the door */
					may_bash = FALSE;
				}

				/* Locked doors (not jammed) */
				else if (c_ptr->feat < FEAT_DOOR_HEAD + 0x08)
				{
					int k;

					/* Door power */
					k = ((c_ptr->feat - FEAT_DOOR_HEAD) & 0x07);

#if 0
					/* XXX XXX XXX XXX Old test (pval 10 to 20) */
					if (randint((m_ptr->hp + 1) * (50 + o_ptr->pval)) <
					    40 * (m_ptr->hp - 10 - o_ptr->pval));
#endif

					/* Try to unlock it XXX XXX XXX */
					if (rand_int(m_ptr->hp / 10) > k)
					{
						/* Unlock the door */
						c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

						/* Do not bash the door */
						may_bash = FALSE;
					}
				}
			}

			/* Stuck doors -- attempt to bash them down if allowed */
			if (may_bash && (r_ptr->flags2 & RF2_BASH_DOOR))
			{
				int k;

				/* Door power */
				k = ((c_ptr->feat - FEAT_DOOR_HEAD) & 0x07);

#if 0
				/* XXX XXX XXX XXX Old test (pval 10 to 20) */
				if (randint((m_ptr->hp + 1) * (50 + o_ptr->pval)) <
				    40 * (m_ptr->hp - 10 - o_ptr->pval));
#endif

				/* Attempt to Bash XXX XXX XXX */
				if (rand_int(m_ptr->hp / 10) > k)
				{
					/* The door was bashed open */
					did_bash_door = TRUE;

					/* Hack -- fall into doorway */
					do_move = TRUE;
				}
			}


			/* Deal with doors in the way */
			if (did_open_door || did_bash_door)
			{
				/* Break down the door */
				if (did_bash_door && (rand_int(100) < 50))
				{
					c_ptr->feat = FEAT_BROKEN;
				}

				/* Open the door */
				else
				{
					c_ptr->feat = FEAT_OPEN;
				}

				/* Notice */
				note_spot_depth(wpos, ny, nx);

				/* Redraw */
				everyone_lite_spot(wpos, ny, nx);
			}
		}

		/* The player is in the way.  Attack him. */
                if (do_move && (c_ptr->m_idx < 0))
		{
			/* Do the attack */
                        if (Players[0-c_ptr->m_idx]->id != m_ptr->owner) (void)make_attack_normal(0 - c_ptr->m_idx, m_idx);

			/* Do not move */
			do_move = FALSE;

			/* Took a turn */
			do_turn = TRUE;
		}

		/* A monster is in the way */
		if (do_move && c_ptr->m_idx > 0)
		{
                        /* Attack it ! */
                        if (m_ptr->owner != y_ptr->owner) monster_attack_normal(c_ptr->m_idx, m_idx);

			/* Assume no movement */
			do_move = FALSE;

			/* Take a turn */
			do_turn = TRUE;
		}


		/* Creature has been allowed move */
		if (do_move)
		{
			/* Take a turn */
			do_turn = TRUE;

			/* Hack -- Update the old location */
			zcave[oy][ox].m_idx = c_ptr->m_idx;

			/* Mega-Hack -- move the old monster, if any */
			if (c_ptr->m_idx > 0)
			{
				/* Move the old monster */
				y_ptr->fy = oy;
				y_ptr->fx = ox;

				/* Update the old monster */
				update_mon(c_ptr->m_idx, TRUE);
			}

			/* Hack -- Update the new location */
			c_ptr->m_idx = m_idx;

			/* Move the monster */
			m_ptr->fy = ny;
			m_ptr->fx = nx;

			/* Update the monster */
			update_mon(m_idx, TRUE);

			/* Redraw the old grid */
			everyone_lite_spot(wpos, oy, ox);

			/* Redraw the new grid */
			everyone_lite_spot(wpos, ny, nx);
		}

		/* Stop when done */
		if (do_turn) 
		{
			m_ptr->energy -= level_speed(&m_ptr->wpos);
			break;
		}
	}

#if 0
	/* Notice changes in view */
	if (do_view)
	{
		/* Update some things */
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
	}
#endif

	/* Hack -- get "bold" if out of options */
	if (!do_turn && !do_move && m_ptr->monfear)
	{
		/* No longer afraid */
		m_ptr->monfear = 0;
	}
}


/* Determine whether the player is invisible to a monster */
static bool player_invis(int Ind, monster_type *m_ptr)
{
	player_type *p_ptr = Players[Ind];
	s16b inv, mlv;
	monster_race *r_ptr;

	inv = p_ptr->invis;
	if(p_ptr->ghost) inv+=1;
	if (inv < 1) return(FALSE);

	r_ptr = race_inf(m_ptr);

	if (r_ptr->flags2 & RF2_INVISIBLE || r_ptr->flags1 & RF1_QUESTOR)
		return(FALSE);

	mlv = (s16b) r_ptr->level;

	if (r_ptr->flags3 & RF3_NO_SLEEP)
	{
		mlv += 5;
	}
	if (r_ptr->flags3 & RF3_DRAGON)
	{
		mlv += 10;
	}
	if (r_ptr->flags3 & RF3_UNDEAD)
	{
		mlv += 12;
	}
	if (r_ptr->flags3 & RF3_DEMON)
	{
		mlv += 10;
	}
	if (r_ptr->flags3 & RF3_ANIMAL)
	{
		mlv += 3;
	}
	if (r_ptr->flags3 & RF3_ORC)
	{
		mlv -= 15;
	}
	if (r_ptr->flags3 & RF3_TROLL)
	{
		mlv -= 10;
	}
	if (r_ptr->flags2 & RF2_STUPID)
	{
		mlv /= 2;
	}
	if (r_ptr->flags2 & RF2_SMART)
	{
		mlv = (mlv * 5) / 4;
	}
	if (mlv < 1)
	{
		mlv = 1;
	}

	return (inv >= randint((mlv * 10) / 7));
}



/*
 * Process all the "live" monsters, once per game turn.
 *
 * During each game turn, we scan through the list of all the "live" monsters,
 * (backwards, so we can excise any "freshly dead" monsters), energizing each
 * monster, and allowing fully energized monsters to move, attack, pass, etc.
 *
 * Note that monsters can never move in the monster array (except when the
 * "compact_monsters()" function is called by "dungeon()" or "save_player()").
 *
 * This function is responsible for at least half of the processor time
 * on a normal system with a "normal" amount of monsters and a player doing
 * normal things.
 *
 * When the player is resting, virtually 90% of the processor time is spent
 * in this function, and its children, "process_monster()" and "make_move()".
 *
 * Most of the rest of the time is spent in "update_view()" and "lite_spot()",
 * especially when the player is running.
 *
 * Note the use of the new special "m_fast" array, which allows us to only
 * process monsters which are alive (or at least recently alive), which may
 * provide some optimization, especially when resting.  Note that monsters
 * which are only recently alive are excised, using a simple "excision"
 * method relying on the fact that the array is processed backwards.
 *
 * Note that "new" monsters are always added by "m_pop()" and they are
 * always added at the end of the "m_fast" array.
 */
 
/* FIXME */
 
void process_monsters(void)
{
	int			k, i, e, pl, tmp, j;
	int			fx, fy;

	bool		test;

	int closest, dis_to_closest, lowhp;
	bool blos, new_los; 

	monster_type	*m_ptr;
	monster_race	*r_ptr;
	player_type *p_ptr;


#ifdef PROJECTION_FLUSH_LIMIT
	/* Reset projection counts */
	count_project = 0;
#endif	// PROJECTION_FLUSH_LIMIT


	/* Process the monsters */
	for (k = m_top - 1; k >= 0; k--)
	{
/*		int closest = -1, dis_to_closest = 9999, lowhp = 9999;
               	bool blos=FALSE, new_los;	*/

		/* Access the index */
		i = m_fast[k];

		/* Access the monster */
                m_ptr = &m_list[i];

		/* Excise "dead" monsters */
		if (!m_ptr->r_idx)
		{
			/* Excise the monster */
			m_fast[k] = m_fast[--m_top];

			/* Skip */
			continue;
		}

		/* Efficiency */
//		if (!(getcave(m_ptr->wpos))) return(FALSE);
		if (!players_on_depth(&m_ptr->wpos)) continue;

		/* Obtain the energy boost */
		e = extract_energy[m_ptr->mspeed];

		/* Give this monster some energy */
		m_ptr->energy += e;

		tmp = level_speed(&m_ptr->wpos);

		/* Not enough energy to move */
		if (m_ptr->energy < tmp) continue;

		/* Make sure we don't store up too much energy */
		//if (m_ptr->energy > tmp) m_ptr->energy = tmp;
		m_ptr->energy = tmp;

		closest = -1;
		dis_to_closest = 9999;
		lowhp = 9999;
		blos=FALSE;

		/* Find the closest player */
		for (pl = 1; pl < NumPlayers + 1; pl++) 
		{ 
			p_ptr = Players[pl]; 

			/* Only check him if he is playing */ 
			if (p_ptr->conn == NOT_CONNECTED) 
				continue; 

			/* Make sure he's on the same dungeon level */ 
			if (!inarea(&p_ptr->wpos, &m_ptr->wpos))
				continue; 

			/* Hack -- Skip him if he's shopping */ 
			if (p_ptr->store_num != -1) 
				continue; 

			/* Hack -- make the dungeon master invisible to monsters */ 
			if (p_ptr->admin_dm) continue; 

			/* Monsters serve a king on his land they dont attack him */ 
			// if (player_is_king(pl)) continue; 

			/* Skip if the monster can't see the player */ 
			if (player_invis(pl, m_ptr)) continue; 

			/* Compute distance */ 
			j = distance(p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx); 

			/* Glaur. Check if monster has LOS to the player */ 
			new_los=los(&p_ptr->wpos, p_ptr->py, p_ptr->px, m_ptr->fy,m_ptr->fx);

			/*	if (p_ptr->ghost)
				if (!new_los)
				j += 100; */

			/* Glaur. Check that the closest VISIBLE target gets selected, 
			   if no visible one available just take the closest*/ 
			if (((blos >= new_los) && (j > dis_to_closest)) || (blos > new_los)) 
				continue; 

			/* Glaur. Skip if same distance and stronger and same visibility*/ 
			if ((j == dis_to_closest) && (p_ptr->chp > lowhp) && (blos == new_los)) 
				continue; 

			/* Remember this player */ 
			blos = new_los; 
			dis_to_closest = j; 
			closest = pl; 
			lowhp = p_ptr->chp; 
		} 


		/* Paranoia -- Make sure we found a closest player */
		if (closest == -1)
			continue;

		m_ptr->cdis = dis_to_closest;
		m_ptr->closest_player = closest;

		/* Hack -- Require proximity */
		if (m_ptr->cdis >= 100) continue;

		p_ptr = Players[closest];

		/* Hack -- calculate the "player noise" */
		noise = (1L << (30 - p_ptr->skill_stl));


		/* Access the race */
                r_ptr = race_inf(m_ptr);

		/* Access the location */
		fx = m_ptr->fx;
		fy = m_ptr->fy;


		/* Assume no move */
		test = FALSE;

		/* Handle "sensing radius" */
		if (m_ptr->cdis <= r_ptr->aaf)
		{
			/* We can "sense" the player */
			test = TRUE;
		}

		/* Handle "sight" and "aggravation" */
		else if ((m_ptr->cdis <= MAX_SIGHT) &&
		         (player_has_los_bold(closest, fy, fx) ||
		          p_ptr->aggravate))
		{
			/* We can "see" or "feel" the player */
			test = TRUE;
		}

#ifdef MONSTER_FLOW
		/* Hack -- Monsters can "smell" the player from far away */
		/* Note that most monsters have "aaf" of "20" or so */
		else if (flow_by_sound &&
		         (cave[py][px].when == cave[fy][fx].when) &&
		         (cave[fy][fx].cost < MONSTER_FLOW_DEPTH) &&
		         (cave[fy][fx].cost < r_ptr->aaf))
		{
			/* We can "smell" the player */
			test = TRUE;
		}
#endif

		/* Do nothing */
		if (!test) continue;


		/* Process the monster */
		if (!m_ptr->special) process_monster(closest, i);
		else
		{
			int p = find_player(m_ptr->owner);
			process_monster_golem(p, i);
		}

		/* Hack -- notice death or departure */
		if (!p_ptr->alive || p_ptr->death || p_ptr->new_level_flag) break;
	}

	/* Only when needed, every five game turns */
	/* TODO: move this to the client side!!! */
//	if (scan_monsters && (!(turn%5)))
	if (scan_monsters && (!(turn % MULTI_HUED_UPDATE)))
	{
		/* Shimmer multi-hued monsters */
		for (i = 1; i < m_max; i++)
		{
			m_ptr = &m_list[i];

			/* Skip dead monsters */
			if (!m_ptr->r_idx) continue;

			/* Access the monster race */
                        r_ptr = race_inf(m_ptr);

			/* Skip non-multi-hued monsters */
//			if (!(r_ptr->flags1 & RF1_ATTR_MULTI)) continue;
			if (!((r_ptr->flags1 & RF1_ATTR_MULTI) ||
					(r_ptr->flags1 & RF1_UNIQUE) ||
					(m_ptr->ego) )) continue;

			/* Shimmer Multi-Hued Monsters */
			everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
		}
	}
}



/* Due to incapability of adding new item flags,
 * this curse seems too soft.. pfft		- Jir -
 */
void curse_equipment(int Ind, int chance, int heavy_chance)
{
	player_type *p_ptr = Players[Ind];
	bool changed = FALSE;
        u32b    o1, o2, o3, o4, esp, o5;
	object_type * o_ptr = &p_ptr->inventory[INVEN_WIELD - 1 + randint(12)];

	if (randint(100) > chance) return;

	if (!(o_ptr->k_idx)) return;

        object_flags(o_ptr, &o1, &o2, &o3, &o4, &o5, &esp);


	/* Extra, biased saving throw for blessed items */
	if ((o3 & (TR3_BLESSED)) && (randint(888) > chance))
	{   
		char o_name[256];
		object_desc(Ind, o_name, o_ptr, FALSE, 0);
		msg_format(Ind, "Your %s resist%s cursing!", o_name,
			((o_ptr->number > 1) ? "" : "s"));
		/* Hmmm -- can we wear multiple items? If not, this is unnecessary */
		return;
	}

#if 0	// l8r..
	if ((randint(100) <= heavy_chance) &&
		(o_ptr->name1 || o_ptr->name2 || o_ptr->art_name))
	{
		if (!(o3 & TR3_HEAVY_CURSE))
			changed = TRUE;
		o_ptr->art_flags3 |= TR3_HEAVY_CURSE;
		o_ptr->art_flags3 |= TR3_CURSED;
		o_ptr->ident |= IDENT_CURSED;
	}
	else
#endif	// 0
	{
		if (!(o_ptr->ident & (ID_CURSED)))
			changed = TRUE;
//		o_ptr->art_flags3 |= TR3_CURSED;
		o_ptr->ident |= ID_CURSED;
	}

	if (changed)
	{
		msg_print(Ind, "There is a malignant black aura surrounding you...");
		if (o_ptr->note)
		{
			if (streq(quark_str(o_ptr->note), "uncursed"))
			{
				o_ptr->note = 0;
			}
		}
	}
}


