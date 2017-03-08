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

/* added this for consistency in some (unrelated) header-inclusion,
   it IS a server file, isn't it? */
#define SERVER

#include "angband.h"


#ifdef TELEPORT_SURPRISES
 #define TELEPORT_SURPRISED(p_ptr,r_ptr) \
    (p_ptr->teleported && rand_int(2) && \
    !strchr("eAN", (r_ptr)->d_char) && \
    !(((r_ptr)->flags1 & RF1_UNIQUE) && ((r_ptr)->flags2 & RF2_SMART) && ((r_ptr)->flags2 & RF2_POWERFUL)) && \
    !((r_ptr)->flags7 & RF7_NAZGUL))
#endif

#define C_BLUE_AI		/* pick player to approach / move towards */
#define C_BLUE_AI_MELEE		/* choose wisely between multiple targets (only initially though) */
#define C_BLUE_AI_MELEE_NASTY	/* reevaluate after initial target pick, if SMART */
/* ANTI_SEVEN_EXPLOIT: There are 2 hacks to prevent slightly silyl ping-pong movement when next to epicenter,
   although that behaviour is probably harmless. None, one, or both may be activated. VAR1 will additionally
   prevent any moves that increase the distance to the epicenter, which shouldnt happen in situations in which
   this is actually exploitable at all anyway, though. So, I recommend turning on VAR2 only^^ - C. Blue */
//#define ANTI_SEVEN_EXPLOIT_VAR1	/* prevents increasing distance to epicenter (probably can't happen in actual gameplay anyway -_-); */
					/* ..stops algorithm if we don't get closer; makes heavy use of distance() */
#define ANTI_SEVEN_EXPLOIT_VAR2		/* stops algorithm if we get LOS to from where the projecting player actually cast his projection */

/*
 * STUPID_MONSTERS flag is left for compatibility, but not recommended.
 * if you think the AI codes are slow (or hard), try the following:
 *
 * (0. if not yet, try AUTO_PURGE in tomenet.cfg first!)
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
 * - choose the best spell available (STUPID_MONSTER_SPELLS) <medium>
 * - try to cast spells indirectly (INDIRECT_FREQ) <slow>
 * - pick up as many items on floor as possible (MONSTERS_GREEDY) <fast>
 * currently not including try to avoid being kited (C_BLUE_AI)
 * C_BLUE_AI_MELEE ignores STUPID_MONSTER flag partially, as makes sense.
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
 #define		INDIRECT_SUMMONING_RADIUS	2

/* if defined, a monster will simply choose the spell by RNG and
 * never hesitate to shoot its friends.
 * (Thanks to Vanilla Angband code, it's not so slow now)
 */
//#define	STUPID_MONSTER_SPELLS

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
 #define SAFETY_RADIUS	0
 #define INDIRECT_FREQ	0
 #define INDIRECT_SUMMONING_RADIUS	0
// #define STUPID_Q
 #define STUPID_MONSTER_SPELLS
// #define MONSTERS_GREEDY	30
// #define MONSTERS_HIDE_HEADS
// #define MONSTERS_HEMM_IN

#endif // STUPID_MONSTERS


/* How frequent they change colours? [2]
 * Note that this value is multiplied by MONSTER_TURNS.
 */
#define		MULTI_HUED_UPDATE	2

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
 * 0 = always, 1 = max normal chances, >1 = reduced chances.
 * To disable, comment it out.
 */
//#define		MONSTER_DIG_FACTOR	100

/* Chance of a monster crossing 'impossible' grid (so that an aquatic
 * can go back to the water), in percent. [20] */
#define		MONSTER_CROSS_IMPOSSIBLE_CHANCE		20

/*
 * If defined, monsters will pick up the gold like normal items.
 * Ever wondered why monster rogues never do that? :)
 * (Exception: Monsters may still _steal_ gold even if this is disabled!)
 */
#define		MONSTER_PICKUP_GOLD

/* Monsters won't take/kill/steal items of TV_SPECIAL */
#define MON_IGNORE_SPECIAL

/* Monsters won't take/kill/steal items of TV_KEY */
//#define MON_IGNORE_KEYS

/*
 * Chance of an item picked up by a monster to disappear, in %. [30]
 * Stolen items are not affected by this value.
 *
 * TODO: best if timed ... ie. the longer a monster holds it, the more
 * the chance of consuming.
 */
#define		MONSTER_ITEM_CONSUME	30

/* Notes on Qs:
 * Quylthulgs do have spell-frequency (1_IN_n), but since they never use
 * their energy to moving(NEVER_MOVE) they all act as if they have (1_IN_1).
 * We can 'fix' it by making them spend energy by *not* casting a spell, but
 * that made them way too weak.
 *
 * if you want it 'fixed', undef this option.
 */
#define Q_ENERGY_EXCEPTION
/* This flag ban Quylthulgs from summoning out of sight. */
#define Q_LOS_EXCEPTION
/* though Quylthulgs are intelligent by nature, for the balance's sake
 * this flag bans Qs from casting spells indirectly (just like vanilla ones).
 */
//#define		STUPID_Q

#ifdef USE_SOUND_2010
 /* For bolt() sfx */
 #define SFX_BOLT_MAGIC 0
 #define SFX_BOLT_SHOT 1
 #define SFX_BOLT_ARROW 2
 #define SFX_BOLT_BOLT 3
 #define SFX_BOLT_MISSILE 4
 #define SFX_BOLT_BOULDER 5

 /* The way monster attack sfx are played:
    0: Play it for the targetted player only, at max volume
    1: Play it for everyone nearby, decreasing volume with distance to monster */
 #define MONSTER_SFX_WAY 1
#endif

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
static bool int_outof(monster_race *r_ptr, int prob) {
	/* Non-Smart monsters are half as "smart" */
	if (!(r_ptr->flags2 & RF2_SMART)) prob = prob / 2;

	/* Roll the dice */
	return (rand_int(100) < prob);
}



/*
 * Remove the "bad" spells from a spell list
 */
static void remove_bad_spells(int m_idx, u32b *f4p, u32b *f5p, u32b *f6p, u32b *f0p) {
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);

	u32b f4 = (*f4p);
	u32b f5 = (*f5p);
	u32b f6 = (*f6p);
	u32b f0 = (*f0p);

	u32b smart = 0L;


	/* Too stupid to know anything */
	if (r_ptr->flags2 & RF2_STUPID) return;

	/* Must be cheating or learning */
	if (!smart_cheat && !smart_learn) return;

	/* Update acquired knowledge */
	if (smart_learn) {
		/* Hack -- Occasionally forget player status */
		if (m_ptr->smart && (rand_int(100) < 1)) m_ptr->smart = 0L;

		/* Use the memorized flags */
		smart = m_ptr->smart;
	}

	/* Cheat if requested */
	if (smart_cheat) {
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


	if (smart & SM_IMM_ACID) {
		if (int_outof(r_ptr, 100)) f4 &= ~RF4_BR_ACID;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BA_ACID;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BO_ACID;
	} else if ((smart & SM_OPP_ACID) && (smart & SM_RES_ACID)) {
		if (int_outof(r_ptr, 80)) f4 &= ~RF4_BR_ACID;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BA_ACID;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BO_ACID;
	} else if ((smart & SM_OPP_ACID) || (smart & SM_RES_ACID)) {
		if (int_outof(r_ptr, 30)) f4 &= ~RF4_BR_ACID;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BA_ACID;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BO_ACID;
	}

	if (smart & SM_IMM_ELEC) {
		if (int_outof(r_ptr, 100)) f4 &= ~RF4_BR_ELEC;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BA_ELEC;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BO_ELEC;
	} else if ((smart & SM_OPP_ELEC) && (smart & SM_RES_ELEC)) {
		if (int_outof(r_ptr, 80)) f4 &= ~RF4_BR_ELEC;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BA_ELEC;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BO_ELEC;
	} else if ((smart & SM_OPP_ELEC) || (smart & SM_RES_ELEC)) {
		if (int_outof(r_ptr, 30)) f4 &= ~RF4_BR_ELEC;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BA_ELEC;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BO_ELEC;
	}

	if (smart & SM_IMM_FIRE) {
		if (int_outof(r_ptr, 100)) f4 &= ~RF4_BR_FIRE;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BA_FIRE;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BO_FIRE;
	} else if ((smart & SM_OPP_FIRE) && (smart & SM_RES_FIRE)) {
		if (int_outof(r_ptr, 80)) f4 &= ~RF4_BR_FIRE;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BA_FIRE;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BO_FIRE;
	} else if ((smart & SM_OPP_FIRE) || (smart & SM_RES_FIRE)) {
		if (int_outof(r_ptr, 30)) f4 &= ~RF4_BR_FIRE;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BA_FIRE;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BO_FIRE;
	}

	if (smart & SM_IMM_COLD) {
		if (int_outof(r_ptr, 100)) f4 &= ~RF4_BR_COLD;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BA_COLD;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BO_COLD;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BO_ICEE;
	} else if ((smart & SM_OPP_COLD) && (smart & SM_RES_COLD)) {
		if (int_outof(r_ptr, 80)) f4 &= ~RF4_BR_COLD;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BA_COLD;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BO_COLD;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BO_ICEE;
	} else if ((smart & SM_OPP_COLD) || (smart & SM_RES_COLD)) {
		if (int_outof(r_ptr, 30)) f4 &= ~RF4_BR_COLD;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BA_COLD;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BO_COLD;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BO_ICEE;
	}

	if ((smart & SM_OPP_POIS) && (smart & SM_RES_POIS)) {
		if (int_outof(r_ptr, 80)) f4 &= ~RF4_BR_POIS;
		if (int_outof(r_ptr, 80)) f5 &= ~RF5_BA_POIS;
	} else if ((smart & SM_OPP_POIS) || (smart & SM_RES_POIS)) {
		if (int_outof(r_ptr, 30)) f4 &= ~RF4_BR_POIS;
		if (int_outof(r_ptr, 30)) f5 &= ~RF5_BA_POIS;
	}

	if (smart & SM_RES_NETH) {
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_NETH;
		if (int_outof(r_ptr, 50)) f5 &= ~RF5_BA_NETH;
		if (int_outof(r_ptr, 50)) f5 &= ~RF5_BO_NETH;
	}
	if (smart & SM_RES_LITE) {
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_LITE;
	}
	if (smart & SM_RES_DARK) {
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_DARK;
		if (int_outof(r_ptr, 50)) f5 &= ~RF5_BA_DARK;
	}
	if (smart & SM_RES_FEAR) {
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_SCARE;
	}
	if (smart & SM_RES_CONF) {
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_CONF;
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_CONF;
	}
	if (smart & SM_RES_CHAOS) {
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_CONF;
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_CONF;
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_CHAO;
	}
	if (smart & SM_RES_DISEN) {
		if (int_outof(r_ptr, 100)) f4 &= ~RF4_BR_DISE;
		if (int_outof(r_ptr, 100)) f0 &= ~RF0_BO_DISE;
		if (int_outof(r_ptr, 100)) f0 &= ~RF0_BA_DISE;
	}
	if (smart & SM_RES_BLIND) {
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_BLIND;
	}
	if (smart & SM_RES_NEXUS) {
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_NEXU;
		if (int_outof(r_ptr, 50)) f6 &= ~RF6_TELE_LEVEL;
	}
	if (smart & SM_RES_SOUND) {
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_SOUN;
	}
	if (smart & SM_RES_SHARD) {
		if (int_outof(r_ptr, 50)) f4 &= ~RF4_BR_SHAR;
	}
	if (smart & SM_IMM_FREE) {
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_HOLD;
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_SLOW;
	}
	if (smart & SM_IMM_MANA) {
		if (int_outof(r_ptr, 100)) f5 &= ~RF5_DRAIN_MANA;
	}

	/* XXX XXX XXX No spells left? */
	/* if (!f4 && !f5 && !f6) ... */

	(*f4p) = f4;
	(*f5p) = f5;
	(*f6p) = f6;
	(*f0p) = f0;
}


#endif


/*
 * Cast a bolt at the player
 * Stop if we hit a monster
 * Affect monsters and the player
 */
static void bolt(int Ind, int m_idx, int typ, int dam_hp, int sfx_typ) {
	player_type *p_ptr = Players[Ind];
	int flg = PROJECT_STOP | PROJECT_KILL;

#ifdef USE_SOUND_2010
 #if !defined(MONSTER_SFX_WAY) || (MONSTER_SFX_WAY < 1)
	if (p_ptr->sfx_monsterattack)
		switch (sfx_typ) {
		case SFX_BOLT_MAGIC:
			sound(Ind, "cast_bolt", NULL, SFX_TYPE_MON_SPELL, TRUE);
			break;
		case SFX_BOLT_SHOT:
			sound(Ind, "fire_shot", NULL, SFX_TYPE_MON_SPELL, TRUE);
			break;
		case SFX_BOLT_ARROW:
			sound(Ind, "fire_arrow", NULL, SFX_TYPE_MON_SPELL, TRUE);
			break;
		case SFX_BOLT_BOLT:
			sound(Ind, "fire_bolt", NULL, SFX_TYPE_MON_SPELL, TRUE);
			break;
		case SFX_BOLT_MISSILE:
			sound(Ind, "fire_missile", NULL, SFX_TYPE_MON_SPELL, TRUE);
			break;
		case SFX_BOLT_BOULDER:
			sound(Ind, "throw_boulder", NULL, SFX_TYPE_MON_SPELL, TRUE);
			break;
		}
 #else
	switch (sfx_typ) {
	case SFX_BOLT_MAGIC:
		sound_near_monster_atk(m_idx, 0, "cast_bolt", NULL, SFX_TYPE_MON_SPELL);
		break;
	case SFX_BOLT_SHOT:
		sound_near_monster_atk(m_idx, 0, "fire_shot", NULL, SFX_TYPE_MON_SPELL);
		break;
	case SFX_BOLT_ARROW:
		sound_near_monster_atk(m_idx, 0, "fire_arrow", NULL, SFX_TYPE_MON_SPELL);
		break;
	case SFX_BOLT_BOLT:
		sound_near_monster_atk(m_idx, 0, "fire_bolt", NULL, SFX_TYPE_MON_SPELL);
		break;
	case SFX_BOLT_MISSILE:
		sound_near_monster_atk(m_idx, 0, "fire_missile", NULL, SFX_TYPE_MON_SPELL);
		break;
	case SFX_BOLT_BOULDER:
		sound_near_monster_atk(m_idx, 0, "throw_boulder", NULL, SFX_TYPE_MON_SPELL);
		break;
	}
 #endif
#endif

	/* Target the player with a bolt attack */
	(void)project(m_idx, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, dam_hp, typ, flg, p_ptr->attacker);
}


/*
 * Cast a breath (or ball) attack at the player
 * Pass over any monsters that may be in the way
 * Affect grids, objects, monsters, and the player
 */
static void breath(int Ind, int m_idx, int typ, int dam_hp, int y, int x, int rad) {
	player_type *p_ptr = Players[Ind];

#ifdef USE_SOUND_2010
 #if !defined(MONSTER_SFX_WAY) || (MONSTER_SFX_WAY < 1)
	if (p_ptr->sfx_monsterattack) sound(Ind, "breath", NULL, SFX_TYPE_MON_SPELL, TRUE);
 #else
	/* hack: we play it at lower volume for non-targetted players but at full volume
	   for the one targetted, since he's in the breath's "air stream" :) - C. Blue */
	if (p_ptr->sfx_monsterattack) sound(Ind, "breath", NULL, SFX_TYPE_MON_SPELL, TRUE);
	sound_near_monster_atk(m_idx, Ind, "breath", NULL, SFX_TYPE_MON_SPELL);
 #endif
#endif

	int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO;

	/* Target the player with a ball attack */
	(void)project(m_idx, rad, &p_ptr->wpos, y, x, dam_hp, typ, flg, p_ptr->attacker);
}
#if 0
static void breath(int Ind, int m_idx, int typ, int dam_hp, int rad) {
	player_type *p_ptr = Players[Ind];
//	int rad;
	int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);

	/* Determine the radius of the blast */
	if (rad < 1) rad = (r_ptr->flags2 & (RF2_POWERFUL)) ? 3 : 2;

#ifdef USE_SOUND_2010
 #if !defined(MONSTER_SFX_WAY) || (MONSTER_SFX_WAY < 1)
	if (p_ptr->sfx_monsterattack) sound(Ind, "breath", NULL, SFX_TYPE_MON_SPELL, TRUE);
 #else
	if (p_ptr->sfx_monsterattack) sound(Ind, "breath", NULL, SFX_TYPE_MON_SPELL, TRUE);
	sound_near_monster_atk(m_idx, Ind, "breath", NULL, SFX_TYPE_MON_SPELL);
 #endif
#endif

	/* Target the player with a ball attack */
	(void)project(m_idx, rad, &p_ptr->wpos, p_ptr->py, p_ptr->px, dam_hp, typ, flg, p_ptr->attacker);
}
#endif	/*0*/
static void ball(int Ind, int m_idx, int typ, int dam_hp, int y, int x, int rad) {
	player_type *p_ptr = Players[Ind];

#ifdef USE_SOUND_2010
 #if !defined(MONSTER_SFX_WAY) || (MONSTER_SFX_WAY < 1)
	if (typ == GF_ROCKET) {
		sound(Ind, "rocket", NULL, SFX_TYPE_MON_SPELL, TRUE);
		/* everyone nearby the monster can hear it too, even if no LOS */
		sound_near_monster_atk(m_idx, Ind, "rocket", NULL, SFX_TYPE_MON_SPELL);
		//sound_near_site(m_list[m_idx].fy, m_list[m_idx].fx, &m_list[m_idx].wpos, Ind, "rocket", NULL, SFX_TYPE_MON_SPELL, FALSE);
	}
	else if (typ == GF_DETONATION) {
		sound(Ind, "detonation", NULL, SFX_TYPE_MON_SPELL, TRUE);
		/* everyone nearby the monster can hear it too, even if no LOS */
		sound_near_monster_atk(m_idx, Ind, "detonation", NULL, SFX_TYPE_MON_SPELL);
		//sound_near_site(m_list[m_idx].fy, m_list[m_idx].fx, &m_list[m_idx].wpos, Ind, "detonation", NULL, SFX_TYPE_MON_SPELL, FALSE);
	}
	else if (typ == GF_STONE_WALL) sound(Ind, "stone_wall", NULL, SFX_TYPE_MON_SPELL, TRUE);
	else if (p_ptr->sfx_monsterattack) sound(Ind, "cast_ball", NULL, SFX_TYPE_MON_SPELL, TRUE);
 #else
	if (typ == GF_ROCKET) {
		sound(Ind, "rocket", NULL, SFX_TYPE_MON_SPELL, TRUE);
		/* everyone nearby the monster can hear it too, even if no LOS */
		sound_near_monster_atk(m_idx, Ind, "rocket", NULL, SFX_TYPE_MON_SPELL);
		//sound_near_site(m_list[m_idx].fy, m_list[m_idx].fx, &m_list[m_idx].wpos, Ind, "rocket", NULL, SFX_TYPE_MON_SPELL, FALSE);
	}
	else if (typ == GF_DETONATION) {
		sound(Ind, "detonation", NULL, SFX_TYPE_MON_SPELL, TRUE);
		/* everyone nearby the monster can hear it too, even if no LOS */
		sound_near_monster_atk(m_idx, Ind, "detonation", NULL, SFX_TYPE_MON_SPELL);
		//sound_near_site(m_list[m_idx].fy, m_list[m_idx].fx, &m_list[m_idx].wpos, Ind, "detonation", NULL, SFX_TYPE_MON_SPELL, FALSE);
	}
	else if (typ == GF_STONE_WALL) {
		sound(Ind, "stone_wall", NULL, SFX_TYPE_MON_SPELL, TRUE);
		sound_near_monster_atk(m_idx, Ind, "stone_wall", NULL, SFX_TYPE_MON_SPELL);
	}
	else if (p_ptr->sfx_monsterattack) {
		sound(Ind, "cast_ball", NULL, SFX_TYPE_MON_SPELL, TRUE);
		sound_near_monster_atk(m_idx, Ind, "cast_ball", NULL, SFX_TYPE_MON_SPELL);
	}
 #endif
#endif

	int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_NODO;

	/* Target the player with a ball attack */
	(void)project(m_idx, rad, &p_ptr->wpos, y, x, dam_hp, typ, flg, p_ptr->attacker);
}

#if 0
/*
 * Cast a beam at the player
 * Affect monsters, items?, and the player
 */
static void beam(int Ind, int m_idx, int typ, int dam_hp) {
	player_type *p_ptr = Players[Ind];
	int flg = PROJECT_STOP | PROJECT_KILL;

#ifdef USE_SOUND_2010
 #if !defined(MONSTER_SFX_WAY) || (MONSTER_SFX_WAY < 1)
	if (p_ptr->sfx_monsterattack) sound(Ind, "cast_beam", NULL, SFX_TYPE_MON_SPELL, TRUE);
 #else
	if (p_ptr->sfx_monsterattack) sound(Ind, "cast_beam", NULL, SFX_TYPE_MON_SPELL, TRUE);
	sound_near_monster_atk(m_idx, Ind, "cast_beam", NULL, SFX_TYPE_MON_SPELL);
 #endif
#endif
	/* Target the player with a bolt attack */
	(void)project(m_idx, 0, &p_ptr->wpos, p_ptr->py, p_ptr->px, dam_hp, typ, flg, p_ptr->attacker);
}

/*
 * Cast a cloud attack at the player
 * Pass over any monsters that may be in the way
 * Affect grids, objects, monsters, and the player
 */
static void cloud(int Ind, int m_idx, int typ, int dam_hp, int y, int x, int rad, int duration, int interval) {
	player_type *p_ptr = Players[Ind];
	int flg = PROJECT_NORF | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_STAY | PROJECT_NODO | PROJECT_NODF;

	project_time = duration;
	project_interval = interval;

#ifdef USE_SOUND_2010
 #if !defined(MONSTER_SFX_WAY) || (MONSTER_SFX_WAY < 1)
	if (p_ptr->sfx_monsterattack) sound(Ind, "cast_cloud", NULL, SFX_TYPE_MON_SPELL, TRUE);
 #else
	if (p_ptr->sfx_monsterattack) sound(Ind, "cast_cloud", NULL, SFX_TYPE_MON_SPELL, TRUE);
	sound_near_monster_atk(m_idx, Ind, "cast_cloud", NULL, SFX_TYPE_MON_SPELL);
 #endif
#endif
	/* Target the player with a ball attack */
	(void)project(m_idx, rad, &p_ptr->wpos, y, x, dam_hp, typ, flg, p_ptr->attacker);
}
#endif


/*
 * Functions for Cleverer monster spells, borrowed from PernA.
 * Most of them are hard-coded, so change them manually :)
 * 			- Jir -
 */


/*
 * Determine if there is a space near the player in which
 * a summoned creature can appear
 */
static bool summon_possible(worldpos *wpos, int y1, int x1) {
	int y, x, i;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Start with adjacent locations, spread further */
	for (i = 1; i <= tdi[2]; i++) {
		y = y1 + tdy[i];
		x = x1 + tdx[i];

		/* Ignore illegal locations */
		if (!in_bounds(y,x)) continue;

		/* Hack: no summon on glyph of warding */
		if (zcave[y][x].feat == FEAT_GLYPH) continue;
		if (zcave[y][x].feat == FEAT_RUNE) continue;
#if 0
		if (cave[y][x].feat == FEAT_MINOR_GLYPH) continue;

		/* Nor on the between */
		if (cave[y][x].feat == FEAT_BETWEEN) return (FALSE);

		/* ...nor on the Pattern */
		if ((cave[y][x].feat >= FEAT_PATTERN_START)
				&& (cave[y][x].feat <= FEAT_PATTERN_XTRA2)) continue;
#endif	// 0

		/* Require empty floor grid in line of sight */
		/* Changed to allow summoning on mountains */
		if (cave_empty_bold(zcave,y,x) && los(wpos, y1,x1,y,x)) return (TRUE);
#if 0
		if ((cave_empty_bold(zcave,y,x) || cave_empty_mountain(zcave,y,x)) &&
		     los(wpos, y1,x1,y,x)) return (TRUE);
#endif
	}

#if 0
	/* Start at the player's location, and check 2 grids in each dir */
	for (y = y1 - 2; y<= y1 + 2; y++) {
		for (x = x1 - 2; x <= x1 + 2; x++) {
			/* Ignore illegal locations */
			if (!in_bounds(y,x)) continue;

			/* Only check a circular area */
			if (distance(y1,x1,y,x)>2) continue;
			
			/* Hack: no summon on glyph of warding */
			if (zcave[y][x].feat == FEAT_GLYPH) continue;
			if (zcave[y][x].feat == FEAT_RUNE) continue;
#if 0
			if (cave[y][x].feat == FEAT_MINOR_GLYPH) continue;

			/* Nor on the between */
			if (cave[y][x].feat == FEAT_BETWEEN) return (FALSE);

			/* ...nor on the Pattern */
			if ((cave[y][x].feat >= FEAT_PATTERN_START)
				&& (cave[y][x].feat <= FEAT_PATTERN_XTRA2)) continue;
#endif	// 0
			
			/* Require empty floor grid in line of sight */
			/* Changed for summoning on mountains */
			if (cave_empty_bold(zcave,y,x) && los(wpos, y1,x1,y,x)) return (TRUE);
#if 0
			if ((cave_empty_bold(zcave,y,x) || cave_empty_mountain(zcave,y,x)) &&
			    los(wpos, y1,x1,y,x)) return (TRUE);
#endif
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
/* Potential high-profile BUG: bool los() 'Travel horiz/vert..' probably differs from
   bool clean_shot() 'mmove()' so that monsters can fire at you without you seeing them.
   Fixing it via bad hack: just adding an extra los() check here for now. */
#define CRUDE_TARGETTING_LOS_FIX

#ifdef DOUBLE_LOS_SAFETY
static bool clean_shot_DLS(worldpos *wpos, int y1, int x1, int y2, int x2, int range, int m_idx);
static bool clean_shot(worldpos *wpos, int y1, int x1, int y2, int x2, int range, int m_idx) {
	return (clean_shot_DLS(wpos, y1, x1, y2, x2, range, m_idx) || clean_shot_DLS(wpos, y2, x2, y1, x1, range, m_idx));
}
static bool clean_shot_DLS(worldpos *wpos, int y1, int x1, int y2, int x2, int range, int m_idx) {
#else
static bool clean_shot(worldpos *wpos, int y1, int x1, int y2, int x2, int range) {
#endif
	int dist, y, x;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Start at the initial location */
	y = y1, x = x1;

	/* See "project()" and "projectable()" */
	for (dist = 0; dist <= range; dist++) {
		/* Never pass through walls */
		if (dist && !cave_contact(zcave, y, x)) break;

		/* Never pass through monsters */
		if (dist && zcave[y][x].m_idx > 0
#ifdef DOUBLE_LOS_SAFETY
		    && zcave[y][x].m_idx != m_idx
#endif
		    ) {
			break;
//			if (is_friend(&m_list[zcave[y][x].m_idx]) < 0) break;
		}

		/* Check for arrival at "final target" */
		if ((x == x2) && (y == y2)) {
#ifdef CRUDE_TARGETTING_LOS_FIX
			/* (bugfix via bad hack) ensure monsters cant snipe us from out of LoS */
			if (!los(wpos, y1, x1, y2, x2)) return FALSE;
#endif
			return (TRUE);
		}

		/* Calculate the new location */
		mmove2(&y, &x, y1, x1, y2, x2);
	}

	/* Assume obstruction */
	return (FALSE);
}
/* Relates to clean_shot like projectable_wall to projectable */
#ifdef DOUBLE_LOS_SAFETY
static bool clean_shot_wall_DLS(worldpos *wpos, int y1, int x1, int y2, int x2, int range, int m_idx);
static bool clean_shot_wall(worldpos *wpos, int y1, int x1, int y2, int x2, int range, int m_idx) {
	return (clean_shot_wall_DLS(wpos, y1, x1, y2, x2, range, m_idx) || clean_shot_wall_DLS(wpos, y2, x2, y1, x1, range, m_idx));
}
static bool clean_shot_wall_DLS(worldpos *wpos, int y1, int x1, int y2, int x2, int range, int m_idx) {
#else
static bool clean_shot_wall(worldpos *wpos, int y1, int x1, int y2, int x2, int range) {
#endif
	int dist, y, x;
	cave_type **zcave;

	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Start at the initial location */
	y = y1, x = x1;

	/* See "project()" and "projectable()" */
	for (dist = 0; dist <= range; dist++) {
		/* Never pass through monsters */
		if (dist && zcave[y][x].m_idx > 0
#ifdef DOUBLE_LOS_SAFETY
		    && zcave[y][x].m_idx != m_idx
#endif
		    ) {
			break;
//			if (is_friend(&m_list[zcave[y][x].m_idx]) < 0) break;
		}

		/* Check for arrival at "final target" */
		if ((x == x2) && (y == y2)) {
#ifdef CRUDE_TARGETTING_LOS_FIX
			/* (bugfix via bad hack) ensure monsters cant snipe us from out of LoS */
			if (!los(wpos, y1, x1, y2, x2)) return FALSE;
#endif
			return (TRUE);
		}

		/* Never pass through walls */
		if (dist && !cave_contact(zcave, y, x)) break;

		/* Calculate the new location */
		mmove2(&y, &x, y1, x1, y2, x2);
	}

	/* Assume obstruction */
	return (FALSE);
}


#if 0	// obsolete
/*
 * Return TRUE if a spell is good for hurting the player (directly).
 */
static bool spell_attack(byte spell) {
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
static bool spell_escape(byte spell) {
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
static bool spell_annoy(byte spell) {
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
static bool spell_summon(byte spell) {
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
static bool spell_tactic(byte spell) {
	/* Blink */
	if (spell == 160 + 4) return (TRUE);

	/* Not good */
	return (FALSE);
}


/*
 * Return TRUE if a spell hastes.
 */
static bool spell_haste(byte spell) {
	/* Haste self */
	if (spell == 160 + 0) return (TRUE);

	/* Not a haste spell */
	return (FALSE);
}


/*
 * Return TRUE if a spell is good for healing.
 */
static bool spell_heal(byte spell) {
	/* Heal */
	if (spell == 160 + 2) return (TRUE);

	/* No healing */
	return (FALSE);
}
#endif	// 0


/*
 * Offsets for the spell indices
 */
#define RF4_OFFSET (32 * 3)
#define RF5_OFFSET (32 * 4)
#define RF6_OFFSET (32 * 5)
#define RF0_OFFSET (32 * 9)


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
#if 0
static int choose_attack_spell(int Ind, int m_idx, byte spells[], byte num) {
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
	if (r_ptr->flags2 & (RF2_STUPID)) {
		/* Pick at random */
		return (spells[rand_int(num)]);
	}
 #endif	// 0

	/* Categorize spells */
	for (i = 0; i < num; i++) {
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
	if ((m_ptr->hp < m_ptr->maxhp / 3) || m_ptr->monfear) {
		/* Choose escape spell if possible */
		if (escape_num) return (escape[rand_int(escape_num)]);
	}

	/* Still hurt badly, couldn't flee, attempt to heal */
	if (m_ptr->hp < m_ptr->maxhp / 3 || m_ptr->stunned) {
		/* Choose heal spell if possible */
		if (heal_num) return (heal[rand_int(heal_num)]);
	}

	/* Player is close and we have attack spells, blink away */
	if ((distance(p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx) < 4) && attack_num && (rand_int(100) < 75)) {
		/* Choose tactical spell */
		if (tactic_num) return (tactic[rand_int(tactic_num)]);
	}

	/* We're hurt (not badly), try to heal */
	if ((m_ptr->hp < m_ptr->maxhp * 3 / 4) && (rand_int(100) < 75)) {
		/* Choose heal spell if possible */
		if (heal_num) return (heal[rand_int(heal_num)]);
	}

	/* Summon if possible (sometimes) */
	if (summon_num && (rand_int(100) < 50)) {
		/* Choose summon spell */
		return (summon[rand_int(summon_num)]);
	}

	/* Attack spell (most of the time) */
	if (attack_num && (rand_int(100) < 85)) {
		/* Choose attack spell */
		return (attack[rand_int(attack_num)]);
	}

	/* Try another tactical spell (sometimes) */
	if (tactic_num && (rand_int(100) < 50)) {
		/* Choose tactic spell */
		return (tactic[rand_int(tactic_num)]);
	}

	/* Haste self if we aren't already somewhat hasted (rarely) */
	if (haste_num && (rand_int(100) < (20 + m_ptr->speed - m_ptr->mspeed))) {
		/* Choose haste spell */
		return (haste[rand_int(haste_num)]);
	}

	/* Annoy player (most of the time) */
	if (annoy_num && (rand_int(100) < 85)) {
		/* Choose annoyance spell */
		return (annoy[rand_int(annoy_num)]);
	}

	/* Choose no spell */
	return (0);
}
#else	// 0
/*
 * Faster and smarter code, borrowed from (Vanilla) Angband 3.0.0.
 */
/* Hack -- borrowing 'direct' flag for los check */
static int choose_attack_spell(int Ind, int m_idx, u32b f4, u32b f5, u32b f6, u32b f0, bool direct) {
	//player_type *p_ptr = Players[Ind];

	int i, num = 0;
	byte spells[96];

#ifndef STUPID_MONSTER_SPELLS
	u32b f4_mask = 0L;
	u32b f5_mask = 0L;
	u32b f6_mask = 0L;
	u32b f0_mask = 0L;

	//int py = p_ptr->py, px = p_ptr->px;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);

	bool has_escape, has_attack, has_summon, has_tactic;
	bool has_annoy, has_haste, has_heal;


	/* Smart monsters restrict their spell choices. */
	if (!(r_ptr->flags2 & (RF2_STUPID))) {
		/* What have we got? */
		has_escape = ((f4 & (RF4_ESCAPE_MASK)) ||
			      (f5 & (RF5_ESCAPE_MASK)) ||
			      (f6 & (RF6_ESCAPE_MASK)) ||
			      (f0 & (RF0_ESCAPE_MASK)));
		has_attack = ((f4 & (RF4_ATTACK_MASK)) ||
			      (f5 & (RF5_ATTACK_MASK)) ||
			      (f6 & (RF6_ATTACK_MASK)) ||
			      (f0 & (RF0_ATTACK_MASK)));
		has_summon = ((f4 & (RF4_SUMMON_MASK)) ||
			      (f5 & (RF5_SUMMON_MASK)) ||
			      (f6 & (RF6_SUMMON_MASK)) ||
			      (f0 & (RF0_SUMMON_MASK)));
		has_tactic = ((f4 & (RF4_TACTIC_MASK)) ||
			      (f5 & (RF5_TACTIC_MASK)) ||
			      (f6 & (RF6_TACTIC_MASK)) ||
			      (f0 & (RF0_TACTIC_MASK)));
		has_annoy = ((f4 & (RF4_ANNOY_MASK)) ||
			     (f5 & (RF5_ANNOY_MASK)) ||
			     (f6 & (RF6_ANNOY_MASK)) ||
			     (f0 & (RF0_ANNOY_MASK)));
		has_haste = ((f4 & (RF4_HASTE_MASK)) ||
			     (f5 & (RF5_HASTE_MASK)) ||
			     (f6 & (RF6_HASTE_MASK)) ||
			     (f0 & (RF0_HASTE_MASK)));
		has_heal = ((f4 & (RF4_HEAL_MASK)) ||
			    (f5 & (RF5_HEAL_MASK)) ||
			    (f6 & (RF6_HEAL_MASK)) ||
			    (f0 & (RF0_HEAL_MASK)));

		/*** Try to pick an appropriate spell type ***/

		/* Hurt badly or afraid, attempt to flee */
		/* If too far, attempt to change position */
		if (has_escape && (
		    ((m_ptr->hp < m_ptr->maxhp / 4 || m_ptr->monfear) && direct) ||
		    m_ptr->cdis > MAX_RANGE ||
		    m_ptr->ai_state & AI_STATE_EFFECT)) {
			/* Choose escape spell */
			f4_mask = (RF4_ESCAPE_MASK);
			f5_mask = (RF5_ESCAPE_MASK);
			f6_mask = (RF6_ESCAPE_MASK);
			f0_mask = (RF0_ESCAPE_MASK);
		}

		/* Still hurt badly, couldn't flee, attempt to heal */
		else if (has_heal && (m_ptr->hp < m_ptr->maxhp / 4 || m_ptr->stunned)) {
			/* Choose heal spell */
			f4_mask = (RF4_HEAL_MASK);
			f5_mask = (RF5_HEAL_MASK);
			f6_mask = (RF6_HEAL_MASK);
			f0_mask = (RF0_HEAL_MASK);
		}

		/* Player is close and we have attack spells, blink away */
//		else if (has_tactic && (distance(py, px, m_ptr->fy, m_ptr->fx) < 4) &&
		else if (has_tactic && (m_ptr->cdis < 4) &&
		    (has_attack || has_summon) && (rand_int(100) < 75)) {
			/* Choose tactical spell */
			f4_mask = (RF4_TACTIC_MASK);
			f5_mask = (RF5_TACTIC_MASK);
			f6_mask = (RF6_TACTIC_MASK);
			f0_mask = (RF0_TACTIC_MASK);
		}

		/* We're hurt (not badly), try to heal */
		else if (has_heal && (m_ptr->hp < m_ptr->maxhp * 3 / 4) &&
		    (rand_int(100) < 60)) {
			/* Choose heal spell */
			f4_mask = (RF4_HEAL_MASK);
			f5_mask = (RF5_HEAL_MASK);
			f6_mask = (RF6_HEAL_MASK);
			f0_mask = (RF0_HEAL_MASK);
		}

		/* Summon if possible (sometimes) */
		else if (has_summon && (rand_int(100) < 50)) {
			/* Choose summon spell */
			f4_mask = (RF4_SUMMON_MASK);
			f5_mask = (RF5_SUMMON_MASK);
			f6_mask = (RF6_SUMMON_MASK);
			f0_mask = (RF0_SUMMON_MASK);
		}

		/* Attack spell (most of the time) */
		else if (has_attack && (rand_int(100) < 85)) {
			/* Choose attack spell */
			f4_mask = (RF4_ATTACK_MASK);
			f5_mask = (RF5_ATTACK_MASK);
			f6_mask = (RF6_ATTACK_MASK);
			f0_mask = (RF0_ATTACK_MASK);
		}

		/* Try another tactical spell (sometimes) */
		else if (has_tactic && (rand_int(100) < 50)) {
			/* Choose tactic spell */
			f4_mask = (RF4_TACTIC_MASK);
			f5_mask = (RF5_TACTIC_MASK);
			f6_mask = (RF6_TACTIC_MASK);
			f0_mask = (RF0_TACTIC_MASK);
		}

		/* Haste self if we aren't already somewhat hasted (rarely) */
		/* XXX check it */
		else if (has_haste && (rand_int(100) < (20 + r_ptr->speed - m_ptr->mspeed))) {
			/* Choose haste spell */
			f4_mask = (RF4_HASTE_MASK);
			f5_mask = (RF5_HASTE_MASK);
			f6_mask = (RF6_HASTE_MASK);
			f0_mask = (RF0_HASTE_MASK);
		}

		/* Annoy player (most of the time) */
		else if (has_annoy && (rand_int(100) < 85)) {
			/* Choose annoyance spell */
			f4_mask = (RF4_ANNOY_MASK);
			f5_mask = (RF5_ANNOY_MASK);
			f6_mask = (RF6_ANNOY_MASK);
			f0_mask = (RF0_ANNOY_MASK);
		}

		/* Else choose no spell (The masks default to this.) */

		/* Keep only the interesting spells */
		f4 &= f4_mask;
		f5 &= f5_mask;
		f6 &= f6_mask;
		f0 &= f0_mask;

		/* Anything left? */
		if (!(f4 || f5 || f6 || f0)) return (0);
	}

#endif /* STUPID_MONSTER_SPELLS */

	/* Extract the "innate" spells */
	for (i = 0; i < 32; i++) {
		if (f4 & (1U << i)) spells[num++] = i + RF4_OFFSET;
	}

	/* Extract the "normal" spells */
	for (i = 0; i < 32; i++) {
		if (f5 & (1U << i)) spells[num++] = i + RF5_OFFSET;
	}

	/* Extract the "bizarre" spells */
	for (i = 0; i < 32; i++) {
		if (f6 & (1U << i)) spells[num++] = i + RF6_OFFSET;
	}

	/* Extract the "extra" spells */
	for (i = 0; i < 32; i++) {
		if (f0 & (1U << i)) spells[num++] = i + RF0_OFFSET;
	}

if (season_halloween) {
	/* Halloween event hack: The Great Pumpkin -C. Blue */
	//if (!strcmp(r_ptr->name,"The Great Pumpkin"))
	if ((m_ptr->r_idx == RI_PUMPKIN1) || (m_ptr->r_idx == RI_PUMPKIN2) || (m_ptr->r_idx == RI_PUMPKIN3)) {
		/* more than 1/3 HP: Moan much, tele rarely */
		if (m_ptr->hp > (m_ptr->maxhp / 3))
		switch(rand_int(17)) {
		case 0:	case 1:
			if(f5 & (1U << 30)) return(RF5_OFFSET + 30);	//RF5_SLOW
			break;
		case 2:	case 3: case 4: case 5:
			if(f5 & (1U << 27)) return(RF5_OFFSET + 27);	//RF5_SCARE
			break;
		case 6:	case 7:	case 8:
			if(f5 & (1U << 31)) return(RF5_OFFSET + 31);	//RF5_HOLD
			break;
		case 9: case 10:
			if(f6 & (1U << 5)) return(RF6_OFFSET + 5);	//RF6_TPORT
			break;
		default:
			if(f4 & (1U << 30)) return(RF4_OFFSET + 30);	//RF4_MOAN
			break;
		}
		/* Just more than 1/6 HP: Moan less, tele more often */
		else if (m_ptr->hp > (m_ptr->maxhp / 6))
		switch(rand_int(17)) {
		case 0:	case 1:
			if(f5 & (1U << 30)) return(RF5_OFFSET + 30);	//RF5_SLOW
			break;
		case 2:	case 3: case 4: case 5:
			if(f5 & (1U << 27)) return(RF5_OFFSET + 27);	//RF5_SCARE
			break;
		case 6:	case 7:	case 8:
			if(f5 & (1U << 31)) return(RF5_OFFSET + 31);	//RF5_HOLD
			break;
		case 9: case 10: case 11:// case 12:
			if(f6 & (1U << 5)) return(RF6_OFFSET + 5);	//RF6_TPORT
			break;
		default:
			if(f4 & (1U << 30)) return(RF4_OFFSET + 30);	//RF4_MOAN
			break;
		}
		/* Nearly dead: Moan rarely, tele often */
		else
		switch(rand_int(17)) {
		case 0:	case 1:
			if(f5 & (1U << 30)) return(RF5_OFFSET + 30);	//RF5_SLOW
			break;
		case 2:	case 3: case 4: case 5:
			if(f5 & (1U << 27)) return(RF5_OFFSET + 27);	//RF5_SCARE
			break;
		case 6:	case 7:	case 8:
			if(f5 & (1U << 31)) return(RF5_OFFSET + 31);	//RF5_HOLD
			break;
		case 9: case 10: case 11: case 12:// case 13: case 14:
			if(f6 & (1U << 5)) return(RF6_OFFSET + 5);	//RF6_TPORT
			break;
		default:
			if(f4 & (1U << 30)) return(RF4_OFFSET + 30);	//RF4_MOAN
			break;
		}
	}
}

	/* Paranoia */
	if (num == 0) return 0;

	/* Pick at random */
	return (spells[rand_int(num)]);
}
#endif	// 0

/* Check if player intercept's a monster's attempt to do something */
//bool monst_check_grab(int Ind, int m_idx, cptr desc)
/* Don't allow interception of multiple players to stack? */
#define NO_INTERCEPTION_STACKING
bool monst_check_grab(int m_idx, int mod, cptr desc) {
	monster_type	*m_ptr = &m_list[m_idx];
	monster_race    *r_ptr = race_inf(m_ptr);

	worldpos *wpos = &m_ptr->wpos;
	player_type *q_ptr;

	cave_type **zcave;
	int i, x2 = m_ptr->fx, y2 = m_ptr->fy;
	int grabchance = 0;
#ifdef NO_INTERCEPTION_STACKING
	int grabchance_top = 0, i_top = 0;
#endif
#ifdef GENERIC_INTERCEPTION
	int eff_lev;
#endif
	int rlev = r_ptr->level;
#ifdef ENABLE_STANCES
	int fac = 100;
#endif


	/* hack: if we dont auto-retaliate vs a monster than we dont intercept either */
	if (r_ptr->flags8 & RF8_NO_AUTORET) return FALSE;

	/* cannot intercept elementals and vortices (not '#' at this time) */
	if (strchr("Ev*", r_ptr->d_char)) return FALSE;

	/* paranoia? */
	if (!(zcave = getcave(wpos))) return(FALSE);

	for (i = 1; i <= NumPlayers; i++) {
		q_ptr = Players[i];

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;

		/* Skip players not on this depth */
		if (!inarea(&q_ptr->wpos, wpos)) continue;

		/* Skip dungeon masters */
		if (q_ptr->admin_dm) continue;

		/* Ghosts cannot intercept */
		if (q_ptr->ghost) continue;

		/* Vampire in mist form cannot intercept */
		if (q_ptr->prace == RACE_VAMPIRE && q_ptr->body_monster == RI_VAMPIRIC_MIST) continue;

		/* can't intercept while in wraithform if monster isn't */
		if (q_ptr->tim_wraith &&
		    ((r_ptr->flags2 & RF2_KILL_WALL) || !(r_ptr->flags2 & RF2_PASS_WALL)))
			return FALSE;

		if (q_ptr->cloaked || q_ptr->shadow_running) continue;

		if (q_ptr->confused || q_ptr->stun || q_ptr->afraid || q_ptr->paralyzed || q_ptr->resting)
			continue;

		/* Cannot grab what you cannot see */
		if (!q_ptr->mon_vis[m_idx]) continue;

		/* Compute distance */
		if (distance(y2, x2, q_ptr->py, q_ptr->px) > 1) continue;

		grabchance = get_skill_scale(q_ptr, SKILL_INTERCEPT, 100);

#ifdef ENABLE_STANCES
		if (q_ptr->combat_stance == 1) switch(q_ptr->combat_stance_power) {
			case 0: fac = 50; break;
			case 1: fac = 55; break;
			case 2: fac = 60; break;
			case 3: fac = 65; break;
		} else if (q_ptr->combat_stance == 2) switch(q_ptr->combat_stance_power) {
			case 0: fac = 104; break;
			case 1: fac = 107; break;
			case 2: fac = 110; break;
			case 3: fac = 115; break;
		}
 #ifndef GENERIC_INTERCEPTION /* old way: actually modify grabchance before subtracing rlev factor */
		grabchance = (grabchance * fac) / 100;
 #endif
#endif

		/* Apply Martial-arts bonus */
		if (get_skill(q_ptr, SKILL_MARTIAL_ARTS) && !monk_heavy_armor(q_ptr) &&
		    !q_ptr->inventory[INVEN_WIELD].k_idx &&
#ifndef ENABLE_MA_BOOMERANG
		    !q_ptr->inventory[INVEN_BOW].k_idx &&
#else
		    q_ptr->inventory[INVEN_BOW].tval != TV_BOW &&
#endif
		    !q_ptr->inventory[INVEN_ARM].k_idx)
			grabchance += get_skill_scale(q_ptr, SKILL_MARTIAL_ARTS, 25);

		grabchance -= (rlev / 3);

#ifdef GENERIC_INTERCEPTION
		eff_lev = 0;
		/* the skill is available to this character? (threshold is paranoia for ignoring
		   racial-bonus flukes) - then give him a base chance even when untrained */
		if (q_ptr->s_info[SKILL_INTERCEPT].mod >= 300) eff_lev = q_ptr->lev < 50 ? q_ptr->lev : 50;
		/* alternatively, still make MA count, since it gives interception chance too */
		else if (get_skill(q_ptr, SKILL_MARTIAL_ARTS)) {
			eff_lev = get_skill(q_ptr, SKILL_MARTIAL_ARTS);
			if (eff_lev > q_ptr->lev) eff_lev = q_ptr->lev;
		}

		//w/o G_I: 50.000: 67..115 -> 33..140 (MA); 0.000: -33..0
		grabchance = (grabchance * 2) / 5;
		if (eff_lev) grabchance += 5 + (eff_lev * 2) / 5;
 #ifdef ENABLE_STANCES
		grabchance = (grabchance * fac) / 100; /* new way: modify final grabchance after rlev subtraction has been applied */
 #endif
#endif

		/* apply action-specific modifier */
		grabchance = (grabchance * mod) / 100;

		if (q_ptr->blind) grabchance /= 3;

		if (grabchance <= 0) continue;

#ifndef NO_INTERCEPTION_STACKING
		if (grabchance >= INTERCEPT_CAP) grabchance = INTERCEPT_CAP;

		/* Got disrupted ? */
		if (magik(grabchance)) {
			char m_name[MNAME_LEN], m_name_real[MNAME_LEN], bgen[2], bgen_real[2];

			/* Get the monster name (or "it") */
			monster_desc(i, m_name, m_idx, 0x00);
			monster_desc(i, m_name_real, m_idx, 0x100);
			switch (m_name[strlen(m_name) - 1]) {
			case 's': case 'x': case 'z':
				bgen[0] = 0;
				break;
			default:
				bgen[0] = 's';
				bgen[1] = 0;
			}
			switch (m_name_real[strlen(m_name_real) - 1]) {
			case 's': case 'x': case 'z':
				bgen_real[0] = 0;
				break;
			default:
				bgen_real[0] = 's';
				bgen_real[1] = 0;
			}

			msg_format(i, "\377%cYou intercept %s'%s attempt to %s!", COLOUR_IC_GOOD, m_name, bgen, desc);
			msg_print_near_monvar(i, m_idx,
			    format("\377%c%s intercepts %s'%s attempt to %s!", COLOUR_IC_NEAR, q_ptr->name, m_name_real, bgen_real, desc),
			    format("\377%c%s intercepts %s'%s attempt to %s!", COLOUR_IC_NEAR, q_ptr->name, m_name, bgen, desc),
			    format("\377%c%s intercepts it!", COLOUR_IC_NEAR, q_ptr->name));
			return TRUE;
		}
#else
		if (grabchance >= INTERCEPT_CAP) {
			grabchance_top = INTERCEPT_CAP;
			i_top = i;
			/* can't go higher, so we may stop here */
			break;
		}
		if (grabchance > grabchance_top) {
			grabchance_top = grabchance;
			i_top = i;
		}
#endif
	}

#ifdef NO_INTERCEPTION_STACKING
	/* Got disrupted ? */
	if (magik(grabchance_top)) {
		char m_name[MNAME_LEN], m_name_real[MNAME_LEN], bgen[2], bgen_real[2];
		/* Get the monster name (or "it") */
		monster_desc(i_top, m_name, m_idx, 0x00);
		monster_desc(i_top, m_name_real, m_idx, 0x100);
		switch (m_name[strlen(m_name) - 1]) {
		case 's': case 'x': case 'z':
			bgen[0] = 0;
			break;
		default:
			bgen[0] = 's';
			bgen[1] = 0;
		}
		switch (m_name_real[strlen(m_name_real) - 1]) {
		case 's': case 'x': case 'z':
			bgen_real[0] = 0;
			break;
		default:
			bgen_real[0] = 's';
			bgen_real[1] = 0;
		}

		msg_format(i_top, "\377%cYou intercept %s'%s attempt to %s!", COLOUR_IC_GOOD, m_name, bgen, desc);
		msg_print_near_monvar(i_top, m_idx,
		    format("\377%c%s intercepts %s'%s attempt to %s!", COLOUR_IC_NEAR, Players[i_top]->name, m_name_real, bgen_real, desc),
		    format("\377%c%s intercepts %s'%s attempt to %s!", COLOUR_IC_NEAR, Players[i_top]->name, m_name, bgen, desc),
		    format("\377%c%s intercepts it!", COLOUR_IC_NEAR, Players[i_top]->name));
		return TRUE;
	}
 #ifdef COMBO_AM_IC_CAP
	m_ptr->intercepted = grabchance_top; //we tried
 #endif
#else
 #ifdef COMBO_AM_IC_CAP
	m_ptr->intercepted = grabchance; //we tried
 #endif
#endif

	/* Assume no grabbing */
	return FALSE;
}


/* (Note that AM fields of players other than <Ind> will actually have only halved effect.) */
static bool monst_check_antimagic(int Ind, int m_idx) {
	//player_type *p_ptr;
	monster_type *m_ptr = &m_list[m_idx];
	//monster_race *r_ptr = race_inf(m_ptr);

	worldpos *wpos = &m_ptr->wpos;

	cave_type **zcave;
	int i, x2 = m_ptr->fx, y2 = m_ptr->fy;
	int antichance = 0, highest_antichance = 0, anti_Ind = 0;// , dis, antidis;

#ifdef COMBO_AM_IC_CAP
	int temp_cap, slope, combo_chance;
#endif


	if (!(zcave = getcave(wpos))) return(FALSE);

	/* this one just cannot be suppressed */
	if (m_ptr->r_idx == RI_LIVING_LIGHTNING) return FALSE;

	/* bad hack: Also abuse this function to check for silence-effect - C. Blue */
	if (m_ptr->silenced > 0 && magik(ANTIMAGIC_CAP)) { //could also use INTERCEPT_CAP instead
		/* no message, just 'no mana to cast' ;) */
		return(TRUE);
	}

	/* Multiple AM fields don't stack; only the strongest has effect - C. Blue */
	for (i = 1; i <= NumPlayers; i++) {
		player_type *q_ptr = Players[i];

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;
		/* Skip players not on this depth */
		if (!inarea(&q_ptr->wpos, wpos)) continue;

		/* Compute distance */
		if (distance(y2, x2, q_ptr->py, q_ptr->px) > q_ptr->antimagic_dis) continue;
		//antidis = q_ptr->antimagic_dis;
		//if (dis > antidis) antichance = 0;
		antichance = q_ptr->antimagic;
		//antichance -= r_ptr->level;

		if (antichance > ANTIMAGIC_CAP) antichance = ANTIMAGIC_CAP;

		/* Reduction for party */
		//if ((i != Ind) && player_in_party(p_ptr->party, i)) antichance >>= 1;

		/* Reduce chance by 50% if monster isn't casting on yourself: */
		if (i != Ind) antichance >>= 1;

		if (antichance > highest_antichance) {
			highest_antichance = antichance;
			anti_Ind = i;
		}
	}

#ifdef COMBO_AM_IC_CAP
	/* from the combined cap, calculate the effective (reduced) AM cap */
	slope = 10000 - (slope_fak * m_ptr->intercepted); /* calculate the new, reduced slope depending on initial IC */
	combo_chance = m_ptr->intercepted * 100 + (highest_antichance * slope) / 100; /* calculate the actual effective combined (total) suppression chance of the monster's spell */
	temp_cap = 100 - (10000 - combo_chance) / (100 - m_ptr->intercepted); /* calculate the effective AM chance (cap) from the total chance and IM chance. */
	/* apply it */
	if (highest_antichance > temp_cap) highest_antichance = temp_cap;
#endif

	/* Got disrupted ? */
	if (magik(highest_antichance)) {
		if ((Players[anti_Ind]->cave_flag[m_ptr->fy][m_ptr->fx] & CAVE_VIEW)) {//got LOS?
			char m_name[MNAME_LEN], m_name_real[MNAME_LEN], bgen[2], bgen_real[2];
			/* Get the monster name (or "it") */
			monster_desc(Ind, m_name, m_idx, 0x00);
			monster_desc(Ind, m_name_real, m_idx, 0x100);
			switch (m_name[strlen(m_name) - 1]) {
			case 's': case 'x': case 'z':
				bgen[0] = 0;
				break;
			default:
				bgen[0] = 's';
				bgen[1] = 0;
			}
			switch (m_name_real[strlen(m_name_real) - 1]) {
			case 's': case 'x': case 'z':
				bgen_real[0] = 0;
				break;
			default:
				bgen_real[0] = 's';
				bgen_real[1] = 0;
			}

			//msg_format(Ind, "\377o%^s fails to cast a spell.", m_name);
#if 0
			if (i == anti_Ind) msg_format(anti_Ind, "\377%cYour anti-magic field disrupts %s'%s attempts.", COLOUR_AM_GOOD, m_name, bgen);
			else msg_format(anti_Ind, "\377%c%s's anti-magic field disrupts %s'%s attempts.", COLOUR_AM_NEAR, Players[anti_Ind]->name, m_name, bgen);
#else
			if (Players[anti_Ind]->mon_vis[m_idx]) {
#ifdef USE_SOUND_2010
				sound(anti_Ind, "am_field", NULL, SFX_TYPE_MISC, FALSE);
#endif
				msg_format(anti_Ind, "\377%cYour anti-magic field disrupts %s'%s attempts.", COLOUR_AM_GOOD, m_name, bgen);
			}
			msg_print_near_monvar(anti_Ind, m_idx,
			    format("\377%c%s's anti-magic field disrupts %s'%s attempts.", COLOUR_AM_NEAR, Players[anti_Ind]->name, m_name_real, bgen_real),
			    format("\377%c%s's anti-magic field disrupts %s'%s attempts.", COLOUR_AM_NEAR, Players[anti_Ind]->name, m_name, bgen),
			    format("\377%c%s's anti-magic field disrupts its attempts.", COLOUR_AM_NEAR, Players[anti_Ind]->name));
#endif
		}
		return TRUE;
	}

#if 0
	/* Use this code if monster's antimagic should hinder other monsters'
	 * casting.
	 */
	/* Scan the maximal area of radius "MONSTER_ANTIDIS" */
	for (y = y2 - MONSTER_ANTIDIS; y <= y2 + MONSTER_ANTIDIS; y++) {
		for (x = x2 - MONSTER_ANTIDIS; x <= x2 + MONSTER_ANTIDIS; x++) {
			/* Ignore "illegal" locations */
			if (!in_bounds2(wpos, y, x)) continue;
			if ((m_idx = zcave[y][x].m_idx) <= 0) continue;

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
			if (antichance > ANTIMAGIC_CAP) antichance = ANTIMAGIC_CAP; /* AM cap */

			/* Got disrupted ? */
			if (!magik(antichance)) continue;

			if (p_ptr->mon_vis[m_idx]) {
				char m_name[MNAME_LEN], bgen[2];

				monster_desc(Ind, m_name, m_idx, 0);
				switch (m_name[strlen(m_name) - 1]) {
				case 's': case 'x': case 'z':
					bgen[0] = 0;
					break;
				default:
					bgen[0] = 's';
					bgen[1] = 0;
				}
				msg_format(Ind, "\377%c%^s'%s anti-magic field disrupts your attempts.", COLOUR_AM_MON, m_name, bgen);
			} else
				msg_format(Ind, "\377%cAn anti-magic field disrupts your attempts.", COLOUR_AM_MON);
#ifdef USE_SOUND_2010
			sound_near_monster(m_idx, "am_field", NULL, SFX_TYPE_MONSTER_MISC, FALSE);
#endif

			return TRUE;
		}
	}
#endif	// 0

	/* Assume no antimagic */
	return FALSE;
}

/* (Note that AM fields of players other than <Ind> will actually have only halved effect.) */
int world_check_antimagic(int Ind) {
	player_type *p_ptr = Players[Ind];
	worldpos *wpos = &p_ptr->wpos;

	cave_type **zcave;
	int i, x2 = p_ptr->px, y2 = p_ptr->py;
	int antichance = 0, highest_antichance = 0, anti_Ind = 0;

	if (!(zcave = getcave(wpos))) return(FALSE);

	/* Multiple AM fields don't stack; only the strongest has effect - C. Blue */
	for (i = 1; i <= NumPlayers; i++) {
		player_type *q_ptr = Players[i];

		/* Skip disconnected players */
		if (q_ptr->conn == NOT_CONNECTED) continue;

		/* Skip players not on this depth */
		if (!inarea(&q_ptr->wpos, wpos)) continue;

		/* Compute distance */
		if (distance(y2, x2, q_ptr->py, q_ptr->px) > q_ptr->antimagic_dis) continue;
		antichance = q_ptr->antimagic;

		if (antichance > ANTIMAGIC_CAP) antichance = ANTIMAGIC_CAP; /* AM cap */

		/* Reduce chance by 50% if monster isn't casting on yourself: */
		if (i != Ind) antichance >>= 1;

		if (antichance > highest_antichance) {
			highest_antichance = antichance;
			anti_Ind = i;
		}
	}

	/* Got disrupted ? */
	if (magik(highest_antichance)) return anti_Ind;

	/* Assume no antimagic effect */
	return 0;
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
	if (!(zcave = getcave(&m_ptr->wpos))) return 99;

	for (i = 1; i <= tdi[rad]; i++)
	{
		if (i == tdi[d]) d++;

		y = py + tdy[i];
		x = px + tdx[i];

		/* Check nearby locations */

		/* Skip illegal locations */
		if (!in_bounds(y, x)) continue;

		/* Skip locations in a wall */
		if (!cave_contact(zcave, y, x)) continue;

		/* Check if projectable */
		if (projectable(&m_ptr->wpos, fy, fx, y, x, MAX_RANGE) &&
		    projectable_wall(&m_ptr->wpos, y, x, py, px, MAX_RANGE))
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
//		for (y = py - d * vy; y <= py + d * vy; y += vy)
		{
			for (x = px - d; x <= px + d; x++)
//			for (x = px - d * vx; x <= px + d * vx; x += vx)
			{
				/* Skip illegal locations */
				if (!in_bounds(y, x)) continue;

				/* Skip locations in a wall */
//				if (!cave_floor_bold(zcave, y, x)) continue;
				if (!cave_contact(zcave, y, x)) continue;

				/* Check distance */
				if (distance(y, x, py, px) != d) continue;

				/* Check if projectable */
				if (projectable_wall(&m_ptr->wpos, fy, fx, y, x, MAX_RANGE))
				{
					giveup = FALSE;
					if (projectable_wall(&m_ptr->wpos, y, x, py, px, MAX_RANGE))
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
#ifdef USE_SOUND_2010
 #define HANDLE_SUMMON(HEAR, SEE) \
		if (blind) msg_format(Ind, "%^s mumbles.", m_name); \
		if (count) { \
			m_ptr->clone_summoning = clone_summoning; \
			if (blind) msg_format(Ind, "You hear %s appear nearby.", HEAR); \
			else msg_format(Ind, "%^s magically summons %s!", m_name, SEE); \
			sound_near_site(ys, xs, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE); \
		} else msg_format(Ind, "%^s tries to cast a spell, but fails.", m_name);

 #define HANDLE_SUMMON2(HEAR, SEE) \
		if (blind) msg_format(Ind, "%^s mumbles.", m_name); \
		if (count) { \
			m_ptr->clone_summoning = clone_summoning; \
			if (blind) msg_format(Ind, "%s", HEAR); \
			else msg_format(Ind, "%^s magically summons %s!", m_name, SEE); \
			sound_near_site(ys, xs, wpos, 0, "summon", NULL, SFX_TYPE_MON_SPELL, FALSE); \
		} else msg_format(Ind, "%^s tries to cast a spell, but fails.", m_name);
#else
 #define HANDLE_SUMMON(HEAR, SEE) \
		if (blind) msg_format(Ind, "%^s mumbles.", m_name); \
		if (count) { \
			m_ptr->clone_summoning = clone_summoning; \
			if (blind) msg_format(Ind, "You hear %s appear nearby.", HEAR); \
			else msg_format(Ind, "%^s magically summons %s!", m_name, SEE); \
		} else msg_format(Ind, "%^s tries to cast a spell, but fails.", m_name);

 #define HANDLE_SUMMON2(HEAR, SEE) \
		if (blind) msg_format(Ind, "%^s mumbles.", m_name); \
		if (count) { \
			m_ptr->clone_summoning = clone_summoning; \
			if (blind) msg_print(Ind, "%s", HEAR); \
			else msg_format(Ind, "%^s magically summons %s!", m_name, SEE); \
		} else msg_format(Ind, "%^s tries to cast a spell, but fails.", m_name);
#endif

bool make_attack_spell(int Ind, int m_idx) {
	player_type *p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	dun_level	*l_ptr = getfloor(wpos);
	int		k, chance, thrown_spell, rlev; // , failrate;
	//byte		spell[96], num = 0;
	u32b		f4, f5, f6, f7, f0;
	monster_type	*m_ptr = &m_list[m_idx];
	monster_race	*r_ptr = race_inf(m_ptr);
	//object_type	*o_ptr = &p_ptr->inventory[INVEN_WIELD];
	char		m_name[MNAME_LEN], m_name_real[MNAME_LEN];
	char		m_poss[MNAME_LEN];
	char		ddesc[MNAME_LEN];

	/* Target location */
	int x = p_ptr->px;
	int y = p_ptr->py;
	/* for shadow running */
	int xs = x;
	int ys = y;
	/* Summon count */
	int count = 0;

	/* scatter summoning target location if player is shadow running, ie hard to pin down */
	if (p_ptr->shadow_running) scatter(wpos, &ys, &xs, y, x, 5, 0);

	bool blind = (p_ptr->blind ? TRUE : FALSE);
	/* Extract the "within-the-vision-ness" --
	   Note: This now requires LoS because it is only used
	         for non-direct spells. Idea here:
	         We can easily guess who cast that fireball 'around the corner'.. */
	bool visible = p_ptr->mon_vis[m_idx]
			&& player_has_los_bold(Ind, m_ptr->fy, m_ptr->fx);//new: require LoS
	/* Extract the "see-able-ness" --
	   Note: This allows non-LoS (aka ESP-only) visibility,
	         because it is only used for direct spells. */
	bool seen = p_ptr->mon_vis[m_idx] && !blind;
	/* Assume "normal" target */
	bool normal = TRUE;
	/* Assume "projectable" */
	bool direct = TRUE, local = FALSE;
	bool stupid = r_ptr->flags2 & (RF2_STUPID), summon = FALSE;

	int rad = 0, srad;
	//u32b f7 = race_inf(&m_list[m_idx])->flags7;
	int s_clone = 0, clone_summoning = m_ptr->clone_summoning;
	//int eff_m_hp;
	/* To avoid TELE_TO from CAVE_ICKY pos on player outside */
	cave_type **zcave;
	/* Save the old location */
	int oy = m_ptr->fy;
	int ox = m_ptr->fx;
	/* Space/Time Anchor */
	//bool st_anchor = check_st_anchor(&m_ptr->wpos, oy, ox);

#ifdef SAURON_ANTI_GLYPH
	bool summon_test = FALSE;
	monster_race *base_r_ptr = &r_info[m_ptr->r_idx];
#endif
	//int antichance = 0, antidis = 0;


	wpos = &m_ptr->wpos;
	if (!(zcave = getcave(wpos))) return FALSE;

	/* Don't attack your master */
	if (p_ptr->id == m_ptr->owner) return (FALSE);

	/* Cannot cast spells when confused */
	if (m_ptr->confused) return (FALSE);

	/* Hack -- Extract the spell probability */
	chance = (r_ptr->freq_innate + r_ptr->freq_spell) / 2;

	/* Not allowed to cast spells */
	//if (!chance) return (FALSE);

	/* Only do spells occasionally */
	if (rand_int(100) >= chance) return (FALSE);


	/* XXX XXX XXX Handle "track_target" option (?) */


	/* Extract the racial spell flags */
	f4 = r_ptr->flags4;
	f5 = r_ptr->flags5;
	f6 = r_ptr->flags6;
	f7 = r_ptr->flags7;
	f0 = r_ptr->flags0;

	/* unable to summon on this floor? */
	if ((l_ptr && (l_ptr->flags2 & LF2_NO_SUMMON))
	    || (in_sector00(wpos) && (sector00flags2 & LF2_NO_SUMMON))) {
		/* Remove summoning spells */
		f4 &= ~(RF4_SUMMON_MASK);
		f5 &= ~(RF5_SUMMON_MASK);
		f6 &= ~(RF6_SUMMON_MASK);
		f0 &= ~(RF0_SUMMON_MASK);
	}

	/* unable to teleport on this floor? */
	if ((l_ptr && (l_ptr->flags2 & LF2_NO_TELE))
	    || (in_sector00(wpos) && (sector00flags2 & LF2_NO_TELE))
	    /* don't start futile attempts to tele on non-tele grids? */
	    || (!stupid && (!(r_ptr->flags2 & RF2_EMPTY_MIND) || (r_ptr->flags2 & RF2_SMART)) && (zcave[oy][ox].info & CAVE_STCK))) {
		/* Remove teleport spells */
		f6 &= ~(RF6_BLINK | RF6_TPORT | RF6_TELE_TO | RF6_TELE_AWAY | RF6_TELE_LEVEL);
	}

	/* reduce exp from summons and from summons' summons.. */
	if (cfg.clone_summoning != 999) clone_summoning++;
	if (f7 & RF7_S_LOWEXP) s_clone = 75;
	if (f7 & RF7_S_NOEXP) s_clone = 100;

	/* Only innate spells */
//(restricted it a bit, see guide)	if(l_ptr && l_ptr->flags1 & LF1_NO_MAGIC) f5 = f6 = 0;

	/* radius of ball spells and breathes.
	 * XXX this doesn't reflect some exceptions(eg. radius=4 spells). */
	srad = (r_ptr->flags2 & (RF2_POWERFUL)) ? 3 : 2; /* was 2 : 1 */

	/* NOTE: it is abusable that MAX_RANGE is 18 and player arrow range
	 * is 25-50; one can massacre uniques without any dangers.
	 * This attempt to prevent it somewhat by allowing monsters to cast
	 * some spells (like teleport) under such situations.
	 * -- arrow range has been limited by now. leaving this as it is though. --
	 */
#ifndef	STUPID_MONSTER_SPELLS /* see MAX_SIGHT in process_monsters */
	if (m_ptr->cdis > MAX_RANGE) {
		if (!los(wpos, y, x, m_ptr->fy, m_ptr->fx)) return (FALSE);

		f4 &= (RF4_INDIRECT_MASK | (m_ptr->cdis <= MAX_RANGE + 12 ? RF4_SUMMON_MASK : 0));
		f5 &= (RF5_INDIRECT_MASK | (m_ptr->cdis <= MAX_RANGE + 12 ? RF5_SUMMON_MASK : 0));
		f6 &= (RF6_INDIRECT_MASK | (m_ptr->cdis <= MAX_RANGE + 12 ? RF6_SUMMON_MASK : 0));
		f0 &= (RF0_INDIRECT_MASK | (m_ptr->cdis <= MAX_RANGE + 12 ? RF0_SUMMON_MASK : 0));

		/* No spells left */
		if (!f4 && !f5 && !f6 && !f0) return (FALSE);

		normal = FALSE;
		direct = FALSE;
		local = TRUE;

		/* Hack -- summon around itself */
		y = ys = m_ptr->fy;
		x = xs = m_ptr->fx;
		summon = (f4 & (RF4_SUMMON_MASK)) || (f5 & (RF5_SUMMON_MASK)) ||
			(f6 & (RF6_SUMMON_MASK)) || (f0 & (RF0_SUMMON_MASK));
	}
#else	/* STUPID_MONSTER_SPELLS */
	if (m_ptr->cdis > MAX_RANGE) return (FALSE);
#endif	/* STUPID_MONSTER_SPELLS */

	/* Hack -- require projectable player */
	if (normal) {
		/* Check range */
		//if (m_ptr->cdis > MAX_RANGE) return (FALSE);

		/* Check path */
#if INDIRECT_FREQ < 1
		if (!projectable_wall(&p_ptr->wpos, m_ptr->fy, m_ptr->fx, p_ptr->py, p_ptr->px, MAX_RANGE)) return (FALSE);
#else
		summon = (f4 & (RF4_SUMMON_MASK)) || (f5 & (RF5_SUMMON_MASK)) || (f6 & (RF6_SUMMON_MASK)) || (f0 & (RF0_SUMMON_MASK));

		if (!projectable_wall(&p_ptr->wpos, m_ptr->fy, m_ptr->fx, p_ptr->py, p_ptr->px, MAX_RANGE)) {
#ifdef STUPID_Q
			if (r_ptr->d_char == 'Q') return (FALSE);
#endif	// STUPID_Q
#ifdef Q_LOS_EXCEPTION
			if (r_ptr->d_char == 'Q') summon = FALSE;
#endif	// Q_LOS_EXCEPTION

			if (!magik(INDIRECT_FREQ)) return (FALSE);

			direct = FALSE;

			/* effort to avoid bottlenecks.. */
			if (summon ||
					(f4 & RF4_RADIUS_SPELLS) ||
					(f5 & RF5_RADIUS_SPELLS) ||
					(f6 & RF6_RADIUS_SPELLS) || /* (HACK) added this one, not sure if cool! */
					(f0 & RF0_RADIUS_SPELLS))
				rad = near_hit(m_idx, &y, &x,
						srad > INDIRECT_SUMMONING_RADIUS ?
						srad : INDIRECT_SUMMONING_RADIUS);
			else rad = 99;

			//if (rad > 3 || (rad == 3 && !(r_ptr->flags2 & (RF2_POWERFUL))))
			if (rad > srad) {
				local = TRUE;

				/* Remove inappropriate spells */
				f4 &= ~(RF4_RADIUS_SPELLS);
				f5 &= ~(RF5_RADIUS_SPELLS);
				f6 &= ~(RF6_RADIUS_SPELLS); /* (HACK) added - unsure if cool */
				f0 &= ~(RF0_RADIUS_SPELLS); /* (HACK) added - unsure if cool */
				//f6 &= RF6_INT_MASK;
			}

			/* remove 'direct' spells */
			f4 &= ~(RF4_DIRECT_MASK);
			f5 &= ~(RF5_DIRECT_MASK);
			f6 &= ~(RF6_DIRECT_MASK);
			f0 &= ~(RF0_DIRECT_MASK); /* (HACK) added - unsure if cool */

			/* No spells left */
			if (!f4 && !f5 && !f6 && !f0) return (FALSE);
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
		f0 &= RF0_INT_MASK;

		/* No spells left */
		if (!f4 && !f5 && !f6 && !f0) return (FALSE);
	}


#ifdef DRS_SMART_OPTIONS

	/* Remove the "ineffective" spells */
	remove_bad_spells(m_idx, &f4, &f5, &f6, &f6, &f0);

	/* No spells left */
	if (!f4 && !f5 && !f6 && !f0) return (FALSE);

#endif

#ifndef	STUPID_MONSTER_SPELLS
	/* Check for a clean bolt shot */
	if (!direct ||
	    ((f4 & (RF4_BOLT_MASK) || f5 & (RF5_BOLT_MASK) || f6 & (RF6_BOLT_MASK) || f0 & (RF0_BOLT_MASK)) &&
	     !stupid &&
 #ifndef MON_BOLT_ON_WALL
  #ifdef DOUBLE_LOS_SAFETY
	     !clean_shot(wpos, m_ptr->fy, m_ptr->fx, y, x, MAX_RANGE, m_idx)))
  #else
	     !clean_shot(wpos, m_ptr->fy, m_ptr->fx, y, x, MAX_RANGE)))
  #endif
 #else
  #ifdef DOUBLE_LOS_SAFETY
	     !clean_shot_wall(wpos, m_ptr->fy, m_ptr->fx, y, x, MAX_RANGE, m_idx)))
  #else
	     !clean_shot_wall(wpos, m_ptr->fy, m_ptr->fx, y, x, MAX_RANGE)))
  #endif
 #endif
	{
		/* Remove spells that will only hurt friends */
		f4 &= ~(RF4_BOLT_MASK);
		f5 &= ~(RF5_BOLT_MASK);
		f6 &= ~(RF6_BOLT_MASK);
		f0 &= ~(RF0_BOLT_MASK);
	}

	/* Check for a possible summon */
	/*
	if (rad > 3 || ((f4 & (RF4_SUMMON_MASK) || f5 & (RF5_SUMMON_MASK) ||
	    f6 & (RF6_SUMMON_MASK) || f0 & (RF0_SUMMON_MASK)) &&
	*/
	//if (rad > 3 ||
	summon_test = summon_possible(wpos, ys, xs);
#ifdef SAURON_ANTI_GLYPH
	if (m_ptr->r_idx == RI_SAURON && summon && !summon_test && m_ptr->hp < m_ptr->maxhp) {
		base_r_ptr->freq_spell = base_r_ptr->freq_innate = SAURON_SPELL_BOOST;
		if (!m_ptr->extra) s_printf("SAURON: boost (glyph/nospace summon).\n");
		m_ptr->extra = 5; /* stay boosted for 5 turns at least */
	}
#endif
	if (rad > INDIRECT_SUMMONING_RADIUS || magik(SUPPRESS_SUMMON_RATE) ||
	    (summon && !stupid &&
	    !summon_test)) {
		/* Remove summoning spells */
		f4 &= ~(RF4_SUMMON_MASK);
		f5 &= ~(RF5_SUMMON_MASK);
		f6 &= ~(RF6_SUMMON_MASK);
		f0 &= ~(RF0_SUMMON_MASK);
	}

	/* No spells left */
	if (!f4 && !f5 && !f6 && !f0) return (FALSE);
#endif	// STUPID_MONSTER_SPELLS

#if 0
	/* Extract the "innate" spells */
	for (k = 0; k < 32; k++) {
		if (f4 & (1U << k)) spell[num++] = k + RF4_OFFSET;
		if (f5 & (1U << k)) spell[num++] = k + RF5_OFFSET;
		if (f6 & (1U << k)) spell[num++] = k + RF6_OFFSET;
		if (f0 & (1U << k)) spell[num++] = k + RF0_OFFSET;
	}

	/* Extract the "normal" spells */
	for (k = 0; k < 32; k++)
		if (f5 & (1U << k)) spell[num++] = k + RF5_OFFSET;
	/* Extract the "bizarre" spells */
	for (k = 0; k < 32; k++)
		if (f6 & (1U << k)) spell[num++] = k + RF6_OFFSET;
	/* Extract the "extra" spells */
	for (k = 0; k < 32; k++)
		if (f0 & (1U << k)) spell[num++] = k + RF0_OFFSET;
	/* No spells left */
	if (!num) return (FALSE);
#endif	// 0

	/* Stop if player is dead or gone */
	if (!p_ptr->alive || p_ptr->death || p_ptr->new_level_flag) return (FALSE);


	/* Get the monster name (or "it") */
	monster_desc(Ind, m_name, m_idx, 0x00);
	monster_desc(Ind, m_name_real, m_idx, 0x100);

	/* Choose a spell to cast */
	//thrown_spell = choose_attack_spell(Ind, m_idx, spell, num);
	thrown_spell = choose_attack_spell(Ind, m_idx, f4, f5, f6, f0, direct);

	/* Abort if no spell was chosen */
	if (!thrown_spell) return (FALSE);

#if 0
	if (thrown_spell > 127 && l_ptr && l_ptr->flags1 & LF1_NO_MAGIC)
		return(FALSE);
#endif	// 0

	/* Extract the monster level */
	rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

#ifndef STUPID_MONSTER_SPELLS
	if (!stupid && (thrown_spell >= 128 || thrown_spell == 98)) { //98 = S_ANIMAL
		int factor = 0;

		/* Extract the 'stun' factor */
		if (m_ptr->stunned > 50) factor += 25;
		if (m_ptr->stunned) factor += 15;

		if (magik(25 - (rlev + 3) / 4) || magik(factor)) {
			if (direct) msg_format(Ind, "%^s tries to cast a spell, but fails.", m_name);
			return (TRUE);
		}
 #ifdef GENERIC_INTERCEPTION
		if (monst_check_grab(m_idx, 85, "cast")) return (TRUE);
 #else
		if (monst_check_grab(m_idx, 75, "cast")) return (TRUE);
 #endif
	}
#endif	// STUPID_MONSTER_SPELLS

#if 0 /* instead we added an energy reduction for summoned monsters! - C. Blue */
	/* Hack: Prevent overkill from monsters who gained lots of HP from levelling up
	   compared to their r_info version (hounds in Nether Realm) - C. Blue */
	if (r_ptr->d_char == 'Z' && m_ptr->hp > r_ptr->hdice * r_ptr->hside) eff_m_hp = r_ptr->hdice * r_ptr->hside;
	else eff_m_hp = m_ptr->hp;
#endif

	/* Get the monster possessive ("his"/"her"/"its") */
	monster_desc(Ind, m_poss, m_idx, 0x22);

	/* Hack -- Get the "died from" name */
	monster_desc(Ind, ddesc, m_idx, 0x0188);


	/* Cast the spell. */
	switch (thrown_spell) {

	/* RF4_SHRIEK */
	case RF4_OFFSET+0:
		//if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		/* the_sandman: changed it so that other ppl nearby will know too */
		msg_format(Ind, "\377R%^s makes a high-pitched shriek.", m_name);
		msg_print_near_monvar(Ind, m_idx,
		    format("\377R%^s makes a high-pitched shriek.", m_name_real),
		    format("\377R%^s makes a high-pitched shriek.", m_name),
		    format("\377RIt makes a high-pitched shriek."));
#ifdef USE_SOUND_2010
		sound_near(Ind, "shriek", NULL, SFX_TYPE_MON_SPELL);
#endif
		//can be spammy!	s_printf("SHRIEK: %s -> %s.\n", m_name, p_ptr->name);
		aggravate_monsters(Ind, m_idx);
		break;

	/* RF4_UNMAGIC */
	case RF4_OFFSET+1:
		disturb(Ind, 1, 0);
#if 0	// oops, this cannot be 'magic' ;)
		if (monst_check_antimagic(Ind, m_idx)) break;
#endif
		//if (blind) msg_format(Ind, "%^s mumbles coldly.", m_name); else
		msg_format(Ind, "%^s mumbles coldly.", m_name);
		if (rand_int(120) < p_ptr->skill_sav)
			msg_print(Ind, "You resist the effects!");
		else if (!unmagic(Ind))
			msg_print(Ind, "You are unaffected!");
		break;

#if 0
	/* RF4_XXX3X4 */
	case RF4_OFFSET+2:
		break;
#endif

	/* RF4_S_ANIMAL */
	case RF4_OFFSET+2:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 1; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_ANIMAL, 1, clone_summoning);
		HANDLE_SUMMON("something", "an animal")
		break;

	/* RF4_ROCKET */
	case RF4_OFFSET+3:
		if (!local) disturb(Ind, 1, 0);
		//if (blind) msg_format(Ind, "%^s shoots something.", m_name);
		if (blind) msg_print(Ind, "You hear a dull, heavy sound.");
		//else msg_format(Ind, "%^s fires a rocket.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s fires a rocket for", m_name);
		ball(Ind, m_idx, GF_ROCKET,
		    ((m_ptr->maxhp / 4) > 800 ? 800 : (m_ptr->maxhp / 4)), y, x, 2);
		update_smart_learn(m_idx, DRS_SHARD);
		break;

#if 0
	/* RF4_ARROW_1 */
	case RF4_OFFSET+4:
		disturb(Ind, 1, 0);
		if (blind) msg_print(Ind, "You hear a strange noise.");
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%^s fires an arrow for", m_name);
		bolt(Ind, m_idx, GF_ARROW, damroll(1, 6), SFX_BOLT_ARROW);
		break;

	/* RF4_ARROW_2 */
	case RF4_OFFSET+5:
		disturb(Ind, 1, 0);
		if (blind) msg_print(Ind, "You hear a strange noise.");
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%^s fires an arrow for", m_name);
		bolt(Ind, m_idx, GF_ARROW, damroll(3, 6), SFX_BOLT_ARROW);
		break;

	/* RF4_ARROW_3 */
	case RF4_OFFSET+6:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s makes a strange noise.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s fires a missile for", m_name);
		bolt(Ind, m_idx, GF_ARROW, damroll(5, 6), SFX_BOLT_MISSILE);
		break;

	/* RF4_ARROW_4 */
	case RF4_OFFSET+7:
		disturb(Ind, 1, 0);
		if (blind) msg_print(Ind, "You hear a strange noise.");
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s fires a missile for", m_name);
		bolt(Ind, m_idx, GF_ARROW, damroll(7, 6), SFX_BOLT_MISSILE);
		break;

#endif	// 0


	/* RF4_ARROW_1 (arrow, light) */
	case RF4_OFFSET+4: {
		//int power = rlev / 2 + randint(rlev / 2),
		int	dice = 1 + rlev / 8,
		fois = 1 + rlev / 20;
#if 0
		if (power > 8) dice += 2;
		if (power > 20) dice += 2;
		if (power > 30) dice += 2;
#endif
		disturb(Ind, 1, 0);
		if (monst_check_grab(m_idx, 100, "fire")) break;
		for (k = 0; k < fois; k++) {
			if (blind) msg_print(Ind, "You hear a whizzing noise.");
			snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s fires an arrow for", m_name);
			bolt(Ind, m_idx, GF_ARROW, damroll(dice, 6), SFX_BOLT_ARROW);
			if (p_ptr->death) break;
		}
		break;
		}

	/* RF4_ARROW_2 (shot, heavy) */
	case RF4_OFFSET+5: {
		//int power = rlev / 2 + randint(rlev / 2),
		//fois = 1 + rlev / 20;
		int	dice = 3 + rlev / 5;
#if 0
		if (power > 8) dice += 2;
		if (power > 20) dice += 2;
		if (power > 30) dice += 2;
#endif

		disturb(Ind, 1, 0);
		if (monst_check_grab(m_idx, 100, "fire")) break;
		if (blind) msg_print(Ind, "You hear a strange noise.");
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s fires a shot for", m_name);
		bolt(Ind, m_idx, GF_ARROW, damroll(dice, 6), SFX_BOLT_SHOT);
		break;
		}

	/* former RF4_ARROW_3 (bolt, heavy) */
	case RF4_OFFSET+6: {
		int	dice = 3 + rlev / 5;
#if 0
		if (power > 8) dice += 2;
		if (power > 20) dice += 2;
		if (power > 30) dice += 2;
#endif

		disturb(Ind, 1, 0);
		if (monst_check_grab(m_idx, 100, "fire")) break;
		if (blind) msg_print(Ind, "You hear a strange noise.");
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s fires a bolt for", m_name);
		bolt(Ind, m_idx, GF_ARROW, damroll(dice, 6), SFX_BOLT_BOLT);
		break;
		}

	/* former RF4_ARROW_4 (generic missile, heavy) */
	case RF4_OFFSET+7: {
		int	dice = 3 + rlev / 5;
#if 0
		if (power > 8) dice += 2;
		if (power > 20) dice += 2;
		if (power > 30) dice += 2;
#endif

		disturb(Ind, 1, 0);
		if (blind) msg_print(Ind, "You hear a strange noise.");
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s fires a missile for", m_name);
		bolt(Ind, m_idx, GF_MISSILE, damroll(dice, 6), SFX_BOLT_MISSILE);
		break;
		}

	/* RF4_BR_ACID */
	case RF4_OFFSET+8:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes acid for", m_name);
		breath(Ind, m_idx, GF_ACID, ((m_ptr->hp / 3) > 1200 ? 1200 : (m_ptr->hp / 3)), y, x, srad);
		update_smart_learn(m_idx, DRS_ACID);
		break;

	/* RF4_BR_ELEC */
	case RF4_OFFSET+9:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes lightning for", m_name);
		breath(Ind, m_idx, GF_ELEC, ((m_ptr->hp / 3) > 1200 ? 1200 : (m_ptr->hp / 3)), y, x, srad);
		update_smart_learn(m_idx, DRS_ELEC);
		break;

	/* RF4_BR_FIRE */
	case RF4_OFFSET+10:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes fire for", m_name);
		breath(Ind, m_idx, GF_FIRE, ((m_ptr->hp / 3) > 1200 ? 1200 : (m_ptr->hp / 3)), y, x, srad);
		update_smart_learn(m_idx, DRS_FIRE);
		break;

	/* RF4_BR_COLD */
	case RF4_OFFSET+11:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes frost for", m_name);
		breath(Ind, m_idx, GF_COLD, ((m_ptr->hp / 3) > 1200 ? 1200 : (m_ptr->hp / 3)), y, x, srad);
		update_smart_learn(m_idx, DRS_COLD);
		break;

	/* RF4_BR_POIS */
	case RF4_OFFSET+12:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes gas for", m_name);
		breath(Ind, m_idx, GF_POIS, ((m_ptr->hp / 3) > 800 ? 800 : (m_ptr->hp / 3)), y, x, srad);
		update_smart_learn(m_idx, DRS_POIS);
		break;

	/* RF4_BR_NETH */
	case RF4_OFFSET+13:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes nether for", m_name);
		breath(Ind, m_idx, GF_NETHER, ((m_ptr->hp / 6) > 550 ? 550 : (m_ptr->hp / 6)), y, x, srad);
		update_smart_learn(m_idx, DRS_NETH);
		break;

	/* RF4_BR_LITE */
	case RF4_OFFSET+14:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes light for", m_name);
		breath(Ind, m_idx, GF_LITE, ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6)), y, x, srad);
		update_smart_learn(m_idx, DRS_LITE);
		break;

	/* RF4_BR_DARK */
	case RF4_OFFSET+15:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes darkness for", m_name);
		breath(Ind, m_idx, GF_DARK, ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6)), y, x, srad);
		update_smart_learn(m_idx, DRS_DARK);
		break;

	/* RF4_BR_CONF */
	case RF4_OFFSET+16:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes confusion for", m_name);
		breath(Ind, m_idx, GF_CONFUSION, ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6)), y, x, srad);
		update_smart_learn(m_idx, DRS_CONF);
		break;

	/* RF4_BR_SOUN */
	case RF4_OFFSET+17:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes sound for", m_name);
		breath(Ind, m_idx, GF_SOUND, ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6)), y, x, srad);
		update_smart_learn(m_idx, DRS_SOUND);
		break;

	/* RF4_BR_CHAO */
	case RF4_OFFSET+18:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes chaos for", m_name);
		breath(Ind, m_idx, GF_CHAOS, ((m_ptr->hp / 6) > 600 ? 600 : (m_ptr->hp / 6)), y, x, srad);
		update_smart_learn(m_idx, DRS_CHAOS);
		break;

	/* RF4_BR_DISE */
	case RF4_OFFSET+19:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes disenchantment for", m_name);
		breath(Ind, m_idx, GF_DISENCHANT, ((m_ptr->hp / 6) > 500 ? 500 : (m_ptr->hp / 6)), y, x, srad);
		update_smart_learn(m_idx, DRS_DISEN);
		break;

	/* RF4_BR_NEXU */
	case RF4_OFFSET+20:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes nexus for", m_name);
		breath(Ind, m_idx, GF_NEXUS, ((m_ptr->hp / 3) > 250 ? 250 : (m_ptr->hp / 3)), y, x, srad);
		update_smart_learn(m_idx, DRS_NEXUS);
		break;

	/* RF4_BR_TIME */
	case RF4_OFFSET+21:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes time for", m_name);
		breath(Ind, m_idx, GF_TIME, ((m_ptr->hp / 3) > 150 ? 150 : (m_ptr->hp / 3)), y, x, srad);
		break;

	/* RF4_BR_INER */
	case RF4_OFFSET+22:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes inertia for", m_name);
		breath(Ind, m_idx, GF_INERTIA, ((m_ptr->hp / 6) > 200 ? 200 : (m_ptr->hp / 6)), y, x, srad);
		break;

	/* RF4_BR_GRAV */
	case RF4_OFFSET+23:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes gravity for", m_name);
		breath(Ind, m_idx, GF_GRAVITY, ((m_ptr->hp / 3) > 150 ? 150 : (m_ptr->hp / 3)), y, x, srad);
		break;

	/* RF4_BR_SHAR */
	case RF4_OFFSET+24:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes shards for", m_name);
		breath(Ind, m_idx, GF_SHARDS, ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6)), y, x, srad);
		update_smart_learn(m_idx, DRS_SHARD);
		break;

	/* RF4_BR_PLAS */
	case RF4_OFFSET+25:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes plasma for", m_name);
		breath(Ind, m_idx, GF_PLASMA, ((m_ptr->hp / 6) > 150 ? 150 : (m_ptr->hp / 6)), y, x, srad);
		break;

	/* RF4_BR_WALL */
	case RF4_OFFSET+26:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes force for", m_name);
		breath(Ind, m_idx, GF_FORCE, ((m_ptr->hp / 6) > 200 ? 200 : (m_ptr->hp / 6)), y, x, srad);
		break;

	/* RF4_BR_MANA */
	case RF4_OFFSET+27:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes magical energy", m_name);
		breath(Ind, m_idx, GF_MANA, ((m_ptr->hp / 3) > 250 ? 250 : (m_ptr->hp / 3)), y, x, srad);
		break;

	/* RF4_XXX5X4 */
	/* RF4_BR_DISI */
	case RF4_OFFSET+28:
		if (!local) disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes disintegration for", m_name);
		breath(Ind, m_idx, GF_DISINTEGRATE, ((m_ptr->hp / 3) > 300 ? 300 : (m_ptr->hp / 3)), y, x, srad);
		break;

	/* RF4_XXX6X4 */
	/* RF4_BR_NUKE */
	case RF4_OFFSET+29:
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s breathes.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s breathes toxic waste for", m_name);
		breath(Ind, m_idx, GF_NUKE, ((m_ptr->hp / 3) > 800 ? 800 : (m_ptr->hp / 3)), y, x, srad);
		update_smart_learn(m_idx, DRS_POIS);
		break;

	/* RF4_MOAN */
	case RF4_OFFSET+30:
		if (season_halloween) {
			/* Halloween event code for ranged MOAN -C. Blue */

			disturb(Ind, 1, 0);
			switch(rand_int(4)) {
			/* Colour change for Halloween */
			case 0:
				msg_format(Ind, "\377o%^s screams: Trick or treat!", m_name);
				break;
			case 1:
				msg_format(Ind, "\377o%^s says: Happy halloween!", m_name);
				break;
			case 2:
				msg_format(Ind, "\377o%^s moans loudly.", m_name);
				break;
			case 3:
				msg_format(Ind, "\377o%^s says: Have you seen The Great Pumpkin?", m_name);
				break;
			}
		}
		break;

	/* RF4_BOULDER */
	case RF4_OFFSET+31:
		//note: not intercepted atm
		disturb(Ind, 1, 0);
		if (blind) msg_print(Ind, "You hear something grunt with exertion.");
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s hurls a boulder at you for", m_name);
		bolt(Ind, m_idx, GF_ARROW, damroll(1 + r_ptr->level / 7, 12), SFX_BOLT_BOULDER);
		break;

	/* RF5_BA_ACID */
	case RF5_OFFSET+0:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts an acid ball of", m_name);
		ball(Ind, m_idx, GF_ACID, randint(rlev * 3) + 15, y, x, srad);
		update_smart_learn(m_idx, DRS_ACID);
		break;

	/* RF5_BA_ELEC */
	case RF5_OFFSET+1:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a lightning ball of", m_name);
		ball(Ind, m_idx, GF_ELEC, randint(rlev * 3 / 2) + 8, y, x, srad);
		update_smart_learn(m_idx, DRS_ELEC);
		break;

	/* RF5_BA_FIRE */
	case RF5_OFFSET+2:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a fire ball of", m_name);
		ball(Ind, m_idx, GF_FIRE, randint(rlev * 7 / 2) + 10, y, x, srad);
		update_smart_learn(m_idx, DRS_FIRE);
		break;

	/* RF5_BA_COLD */
	case RF5_OFFSET+3:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a frost ball of", m_name);
		ball(Ind, m_idx, GF_COLD, randint(rlev * 2) + 10, y, x, srad);
		update_smart_learn(m_idx, DRS_COLD);
		break;

	/* RF5_BA_POIS */
	case RF5_OFFSET+4:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a stinking cloud for", m_name);
		ball(Ind, m_idx, GF_POIS, damroll(12, 2), y, x, srad);
		update_smart_learn(m_idx, DRS_POIS);
		break;

	/* RF5_BA_NETH */
	case RF5_OFFSET+5:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts an nether ball of", m_name);
		ball(Ind, m_idx, GF_NETHER, (50 + damroll(10, 10) + rlev * 4), y, x, srad);
		update_smart_learn(m_idx, DRS_NETH);
		break;

	/* RF5_BA_WATE */
	case RF5_OFFSET+6:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		msg_format(Ind, "%^s gestures fluidly.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "You are engulfed in a whirlpool for");
		ball(Ind, m_idx, GF_WATER, randint(rlev * 5 / 2) + 50, y, x, srad);
		break;

	/* RF5_BA_MANA */
	case RF5_OFFSET+7:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles powerfully.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s invokes a mana storm for", m_name);
		ball(Ind, m_idx, GF_MANA, (rlev * 5) + damroll(10, 10), y, x, srad);
		break;

	/* RF5_BA_DARK */
	case RF5_OFFSET+8:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles powerfully.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s invokes a darkness storm for", m_name);
		ball(Ind, m_idx, GF_DARK, (rlev * 5) + damroll(10, 10), y, x, srad);
		update_smart_learn(m_idx, DRS_DARK);
		break;

	/* RF5_DRAIN_MANA */
	case RF5_OFFSET+9:
		if (monst_check_antimagic(Ind, m_idx)) break;
		if (p_ptr->csp) {
			int r1;

			/* Disturb if legal */
			disturb(Ind, 1, 0);

			/* Basic message */
			msg_format(Ind, "%^s draws psychic energy from you!", m_name);

			/* Attack power */
			r1 = (randint(rlev * 2) + randint(3) + rlev + 10) / 4;

			/* An open mind invites mana drain attacks */
			if ((p_ptr->esp_link_flags & LINKF_OPEN)) r1 *= 2;

			/* Full drain */
			if (r1 >= p_ptr->csp) {
				r1 = p_ptr->csp;
				p_ptr->csp = 0;
				p_ptr->csp_frac = 0;
			}
			/* Partial drain */
			else p_ptr->csp -= r1;

			/* Redraw mana */
			p_ptr->redraw |= (PR_MANA);

			/* Window stuff */
			p_ptr->window |= (PW_PLAYER);

			/* Heal the monster */
			if (m_ptr->hp < m_ptr->maxhp) {
				/* Heal */
				m_ptr->hp += r1 * 2;
				if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

				/* Redraw (later) if needed */
				update_health(m_idx);

				/* Special message */
				if (seen) msg_format(Ind, "%^s appears healthier.", m_name);
			}
		}
		update_smart_learn(m_idx, DRS_MANA);
		break;

	/* RF5_MIND_BLAST */
	case RF5_OFFSET+10:
		disturb(Ind, 1, 0);
		if (!seen)
			msg_print(Ind, "You feel something focussing on your mind.");
		else
			msg_format(Ind, "%^s gazes deep into your eyes.", m_name);

		if (rand_int(100) < p_ptr->skill_sav && !(p_ptr->esp_link_flags & LINKF_OPEN)) { /* An open mind invites psi attacks */
			msg_print(Ind, "You resist the effects!");
		} else {
			msg_print(Ind, "\377RYour mind is blasted by psionic energy.");
			//take_hit(Ind, damroll(8, 8), ddesc, 0);
			take_sanity_hit(Ind, damroll(6, 6), ddesc);/* 8,8 was too powerful */
			if (!p_ptr->resist_conf)
				(void)set_confused(Ind, p_ptr->confused + rand_int(4) + 4);

			if ((!p_ptr->resist_chaos) && (randint(3) == 1))
				(void) set_image(Ind, p_ptr->image + rand_int(250) + 150);
		}
		break;

	/* RF5_BRAIN_SMASH */
	case RF5_OFFSET+11:
		disturb(Ind, 1, 0);
		if (!seen)
			msg_print(Ind, "You feel something focussing on your mind.");
		else
			msg_format(Ind, "%^s looks deep into your eyes.", m_name);

		if (rand_int(100) < p_ptr->skill_sav && !(p_ptr->esp_link_flags & LINKF_OPEN)) { /* An open mind invites psi attacks */
			msg_print(Ind, "You resist the effects!");
		} else {
			msg_print(Ind, "\377RYour mind is blasted by psionic energy.");
			//take_hit(Ind, damroll(12, 15), ddesc, 0);
			take_sanity_hit(Ind, damroll(9,9), ddesc);/* 12,15 was too powerful */
			if (!p_ptr->resist_blind)
				(void)set_blind(Ind, p_ptr->blind + 8 + rand_int(8));
			if (!p_ptr->resist_conf)
				(void)set_confused(Ind, p_ptr->confused + rand_int(4) + 4);
			if (!p_ptr->free_act)
				(void)set_paralyzed(Ind, p_ptr->paralyzed + rand_int(4) + 4);
			(void)set_slow(Ind, p_ptr->slow + rand_int(4) + 4);
		}
		break;

	/* RF5_CURSE (former CAUSE1~4) */
	case RF5_OFFSET+12: {
		/* No antimagic check -- is 'curse' magic? */
		/* rebalance might be needed? */
		int power = rlev / 2 + randint(rlev);

#if 0 /* maybe in the future */
		char damcol = 'o';
		if (race_inf(m_ptr)->flags1 & RF1_UNIQUE) damcol = 'L';
		msg_format(Ind, "%^s points at you and curses for \377%c%d \377wdamage.", m_name, damcol, dam);
#endif

		if (monst_check_antimagic(Ind, m_idx) && !(rand_int(4))) break;
		disturb(Ind, 1, 0);
		if (power < 15) {
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s points at you and curses.", m_name);
#ifdef USE_SOUND_2010
 #if !defined(MONSTER_SFX_WAY) || (MONSTER_SFX_WAY < 1)
			if (p_ptr->sfx_monsterattack) sound(Ind, "curse", NULL, SFX_TYPE_MON_SPELL, FALSE);
 #else
			if (p_ptr->sfx_monsterattack) sound(Ind, "curse", NULL, SFX_TYPE_MON_SPELL, FALSE);
			sound_near_monster_atk(m_idx, Ind, "curse", NULL, SFX_TYPE_MON_SPELL);
 #endif
#endif
			if (rand_int(100) < p_ptr->skill_sav || p_ptr->no_cut)
				msg_print(Ind, "You resist the effects!");
			else
				take_hit(Ind, damroll(3, 8), ddesc, 0);
			break;
		}
		/* RF5_CAUSE_2 */
		else if (power < 35) {
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s points at you and curses horribly.", m_name);
#ifdef USE_SOUND_2010
 #if !defined(MONSTER_SFX_WAY) || (MONSTER_SFX_WAY < 1)
			if (p_ptr->sfx_monsterattack) sound(Ind, "curse", NULL, SFX_TYPE_MON_SPELL, FALSE);
 #else
			if (p_ptr->sfx_monsterattack) sound(Ind, "curse", NULL, SFX_TYPE_MON_SPELL, FALSE);
			sound_near_monster_atk(m_idx, Ind, "curse", NULL, SFX_TYPE_MON_SPELL);
 #endif
#endif
			if (rand_int(100) < p_ptr->skill_sav || p_ptr->no_cut)
				msg_print(Ind, "You resist the effects!");
			else {
				take_hit(Ind, damroll(8, 8), ddesc, 0);
				(void)set_cut(Ind, p_ptr->cut + damroll(2, 3), 0);
			}
			break;
		}
		/* RF5_CAUSE_3 */
		else if (power < 50) {
			if (blind) msg_format(Ind, "%^s mumbles loudly.", m_name);
			else msg_format(Ind, "%^s points at you, incanting terribly!", m_name);
#ifdef USE_SOUND_2010
 #if !defined(MONSTER_SFX_WAY) || (MONSTER_SFX_WAY < 1)
			if (p_ptr->sfx_monsterattack) sound(Ind, "curse", NULL, SFX_TYPE_MON_SPELL, FALSE);
 #else
			if (p_ptr->sfx_monsterattack) sound(Ind, "curse", NULL, SFX_TYPE_MON_SPELL, FALSE);
			sound_near_monster_atk(m_idx, Ind, "curse", NULL, SFX_TYPE_MON_SPELL);
 #endif
#endif
			if (rand_int(100) < p_ptr->skill_sav || p_ptr->no_cut)
				msg_print(Ind, "You resist the effects!");
			else {
				take_hit(Ind, damroll(10, 15), ddesc, 0);
				(void)set_cut(Ind, p_ptr->cut + damroll(5, 5), 0);
			}
			break;
		}
		/* RF5_CAUSE_4 */
		else {
			if (blind) msg_format(Ind, "%^s screams the word 'DIE!'", m_name);
			else msg_format(Ind, "%^s points at you, screaming the word 'DIE'!", m_name);
#ifdef USE_SOUND_2010
 #if !defined(MONSTER_SFX_WAY) || (MONSTER_SFX_WAY < 1)
			if (p_ptr->sfx_monsterattack) sound(Ind, "curse", NULL, SFX_TYPE_MON_SPELL, FALSE);
 #else
			if (p_ptr->sfx_monsterattack) sound(Ind, "curse", NULL, SFX_TYPE_MON_SPELL, FALSE);
			sound_near_monster_atk(m_idx, Ind, "curse", NULL, SFX_TYPE_MON_SPELL);
 #endif
#endif
			if (rand_int(100) < p_ptr->skill_sav || p_ptr->no_cut)
				msg_print(Ind, "You resist the effects!");
			else {
				//take_hit(Ind, damroll(15, 15), ddesc, 0);
				take_hit(Ind, damroll(power / 4, 15), ddesc, 0);
				(void)set_cut(Ind, p_ptr->cut + damroll(10, 10), 0);
			}
			break;
		}
		}

	/* RF5_XXX4X4? */
	case RF5_OFFSET+13:
		break;

	/* RF5_BA_NUKE */
	case RF5_OFFSET+14:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a ball of radiation of", m_name);
		ball(Ind, m_idx, GF_NUKE, (rlev * 3 + damroll(10, 6)), y, x, 2);
		update_smart_learn(m_idx, DRS_POIS);
		break;

	/* RF5_BA_CHAO */
	case RF5_OFFSET+15:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles frighteningly.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s invokes raw chaos for", m_name);
		ball(Ind, m_idx, GF_CHAOS, (rlev * 4) + damroll(10, 10), y, x, 4);
		update_smart_learn(m_idx, DRS_CHAOS);
		break;

	/* RF5_BO_ACID */
	case RF5_OFFSET+16:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts an acid bolt of", m_name);
		bolt(Ind, m_idx, GF_ACID, damroll(7, 8) + (rlev / 3), SFX_BOLT_MAGIC);
		update_smart_learn(m_idx, DRS_ACID);
		break;

	/* RF5_BO_ELEC */
	case RF5_OFFSET+17:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a lightning bolt of", m_name);
		bolt(Ind, m_idx, GF_ELEC, damroll(4, 8) + (rlev / 3), SFX_BOLT_MAGIC);
		update_smart_learn(m_idx, DRS_ELEC);
		break;

	/* RF5_BO_FIRE */
	case RF5_OFFSET+18:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a fire bolt of", m_name);
		bolt(Ind, m_idx, GF_FIRE, damroll(9, 8) + (rlev / 3), SFX_BOLT_MAGIC);
		update_smart_learn(m_idx, DRS_FIRE);
		break;

	/* RF5_BO_COLD */
	case RF5_OFFSET+19:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a frost bolt of", m_name);
		bolt(Ind, m_idx, GF_COLD, damroll(6, 8) + (rlev / 3), SFX_BOLT_MAGIC);
		update_smart_learn(m_idx, DRS_COLD);
		break;

	/* RF5_BO_POIS */
	case RF5_OFFSET+20:
		/* XXX XXX XXX */
		break;

	/* RF5_BO_NETH */
	case RF5_OFFSET+21:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a nether bolt of", m_name);
		bolt(Ind, m_idx, GF_NETHER, 30 + damroll(5, 5) + (rlev * 3) / 2, SFX_BOLT_MAGIC);
		update_smart_learn(m_idx, DRS_NETH);
		break;

	/* RF5_BO_WATE */
	case RF5_OFFSET+22:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a water bolt of", m_name);
		bolt(Ind, m_idx, GF_WATER, damroll(10, 10) + (rlev), SFX_BOLT_MAGIC);
		break;

	/* RF5_BO_MANA */
	case RF5_OFFSET+23:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a mana bolt of", m_name);
		bolt(Ind, m_idx, GF_MANA, randint(rlev * 7 / 2) + 50, SFX_BOLT_MAGIC);
		break;

	/* RF5_BO_PLAS */
	case RF5_OFFSET+24:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a plasma bolt of", m_name);
		bolt(Ind, m_idx, GF_PLASMA, 10 + damroll(8, 7) + (rlev), SFX_BOLT_MAGIC);
		break;

	/* RF5_BO_ICEE */
	case RF5_OFFSET+25:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts an ice bolt of", m_name);
		bolt(Ind, m_idx, GF_ICE, damroll(6, 6) + (rlev), SFX_BOLT_MAGIC);
		update_smart_learn(m_idx, DRS_COLD);
		break;

	/* RF5_MISSILE */
	case RF5_OFFSET+26:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a magic missile of", m_name);
		bolt(Ind, m_idx, GF_MISSILE, damroll(2, 6) + (rlev / 3), SFX_BOLT_MAGIC);
		break;

	/* RF5_SCARE */
	case RF5_OFFSET+27:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_print(Ind, "You hear scary noises.");
		else msg_format(Ind, "%^s casts a fearful illusion.", m_name);
		if (p_ptr->resist_fear)
			msg_print(Ind, "You refuse to be frightened.");
		else if (rand_int(100) < p_ptr->skill_sav)
			msg_print(Ind, "You refuse to be frightened.");
		else
			(void)set_afraid(Ind, p_ptr->afraid + rand_int(4) + 4);
		update_smart_learn(m_idx, DRS_FEAR);
		break;

	/* RF5_BLIND */
	case RF5_OFFSET+28:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		else msg_format(Ind, "%^s casts a spell, burning your eyes!", m_name);
		if (p_ptr->resist_blind)
			msg_print(Ind, "You are unaffected!");
		else if (rand_int(100) < p_ptr->skill_sav)
			msg_print(Ind, "You resist the effects!");
		else
			(void)set_blind(Ind, 12 + rand_int(4));
		update_smart_learn(m_idx, DRS_BLIND);
		break;

	/* RF5_CONF */
	case RF5_OFFSET+29:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles, and you hear puzzling noises.", m_name);
		else msg_format(Ind, "%^s creates a mesmerising illusion.", m_name);
		if (p_ptr->resist_conf)
			msg_print(Ind, "You disbelieve the feeble spell.");
		else if ((rand_int(100) < p_ptr->skill_sav && !(p_ptr->esp_link_flags & LINKF_OPEN)) || /* An open mind invites psi attacks */
		    (p_ptr->mindboost && magik(p_ptr->mindboost_power)))
			msg_print(Ind, "You disbelieve the feeble spell.");
		else
			(void)set_confused(Ind, p_ptr->confused + rand_int(4) + 4);
		update_smart_learn(m_idx, DRS_CONF);
		break;

	/* RF5_SLOW */
	case RF5_OFFSET+30:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		msg_format(Ind, "%^s drains power from your muscles!", m_name);
		if (p_ptr->free_act)
			msg_print(Ind, "You are unaffected!");
		else if (rand_int(100) < p_ptr->skill_sav ||
		    (p_ptr->mindboost && magik(p_ptr->mindboost_power)))
			msg_print(Ind, "You resist the effects!");
		else
			(void)set_slow(Ind, p_ptr->slow + rand_int(4) + 4);
		update_smart_learn(m_idx, DRS_FREE);
		break;

	/* RF5_HOLD */
	case RF5_OFFSET+31:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		else msg_format(Ind, "%^s stares deep into your eyes!", m_name);
		if (p_ptr->free_act)
			msg_print(Ind, "You are unaffected!");
		else if (rand_int(100) < p_ptr->skill_sav ||
		    (p_ptr->mindboost && magik(p_ptr->mindboost_power)))
			msg_print(Ind, "You resist the effects!");
		else
			(void)set_paralyzed(Ind, p_ptr->paralyzed + rand_int(4) + 4);
		update_smart_learn(m_idx, DRS_FREE);
		break;

	/* RF6_HASTE */
	case RF6_OFFSET+0:
		if (monst_check_antimagic(Ind, m_idx)) break;
		if (visible) {
			//disturb(Ind, 1, 0);
			if (blind)
				msg_format(Ind, "%^s mumbles.", m_name);
			else
				msg_format(Ind, "%^s concentrates on %s body.", m_name, m_poss);
		}

		/* Allow quick speed increases to base+10 */
		if (m_ptr->mspeed < m_ptr->speed + 10) {
			if (visible) msg_format(Ind, "%^s starts moving faster.", m_name);
			m_ptr->mspeed += 10;
		}

		/* Allow small speed increases to base+20 */
		else if (m_ptr->mspeed < m_ptr->speed + 20) {
			if (visible) msg_format(Ind, "%^s starts moving faster.", m_name);
			m_ptr->mspeed += 2;
		}

		break;

	/* RF6_XXX1X6 */
	/* RF6_HAND_DOOM */
	/* this should be amplified by some means! */
	case RF6_OFFSET+1:
		// if (!direct) break;	/* allow it over wall, or not..? */
		disturb(Ind, 1, 0);
		msg_format(Ind, "%^s invokes the Hand of Doom!", m_name);
		if (rand_int(100) < p_ptr->skill_sav)
			msg_print(Ind, "You resist the effects!");
		else {
			int dummy = (((s32b) ((65 + randint(25)) * (p_ptr->chp))) / 100);
			if (p_ptr->chp - dummy < 1) dummy = p_ptr->chp - 1;
			msg_print(Ind, "You feel your life fade away!");
			bypass_invuln = TRUE;
			take_hit(Ind, dummy, m_name, 0);
			bypass_invuln = FALSE;
			curse_equipment(Ind, 100, 20);
//			if (p_ptr->chp < 1) p_ptr->chp = 1;
		}
		break;

	/* RF6_HEAL */
	case RF6_OFFSET+2:
		if (monst_check_antimagic(Ind, m_idx)) break;
		if (visible) {
			//disturb(Ind, 1, 0);
			if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			else msg_format(Ind, "%^s concentrates on %s wounds.", m_name, m_poss);
		}

		/* Some heal data for 'rlev * 6' ('1' means 100%, assuming max hp dice):
		   Novice priest solo/group ~1/2, ~1
		   Wormtongue ~1/5, Robin Hood ~1/3, Orfax ~1/2
		   Moon Beast ~3/4, Priest ~3/4
		   Boldor ~1/4, Khim/Ibun ~1/6, It ~1/5
		   Archangel ~1/3
		   Shelob ~1/10
		   Cherub ~1/4, Greater Mummy ~2/3
		   Castamir ~1/4,
		   Lesser Titan ~1/8,
		   Jack of Shadows ~1/9,
		   Utgard-Loke ~1/15,
		   Demilich, Keeper of Secrets ~1/10,
		   Saruman ~1/19,
		   Ungoliant ~1/30,
		   Nodens ~1/15,
		   Star-Spawn ~1/15,
		   Nether Guard (assumed 45kHP, lv121) ~1/60,
		   Zu-Aon (assumed 117kHP, lv147) ~1/130.

		   For most monsters in low/mid levels, 1/4 was a decent effective
		   average for HEAL. Low-HP monsters would naturally profit especially
		   much, such as priests, working out nicely.
		   The problem starts with high-HP monsters, especially since HP and
		   damage in TomeNET are higher on average than in vanilla.
		   As Mikael pointed out, monsters that are SMART will prefer heal
		   spells when wounded, and possibly also teleport. And monsters that
		   teleport in general, also can heal while the player has to reapproach.
		   This is especially nasty if the monster casts 1_IN_1 or similar.
		   So to the normal HEAL that is still feasible for those cases. - C. Blue
		*/

		/* Note, no need to check for RF2_STUPID, since there is no
		   stupid monster that can heal so far. */
		if ((r_ptr->flags4 & RF4_ESCAPE_MASK) ||
		    (r_ptr->flags5 & RF5_ESCAPE_MASK) ||
		    (r_ptr->flags6 & RF6_ESCAPE_MASK) ||
		    (r_ptr->flags0 & RF0_ESCAPE_MASK)) {
			/* Heal some */
			m_ptr->hp += (rlev * 6);
		} else {
			/* New: Make it useful for high-level monsters. Abuse k and count. */
			k = rlev * 6;
			count = m_ptr->maxhp / 5; /* Good values would probably be 1/6..1/4 */
			m_ptr->hp += (k < count) ? count : k;
		}

		if (m_ptr->stunned) {
			m_ptr->stunned -= rlev * 2;
			if (m_ptr->stunned <= 0) {
				m_ptr->stunned = 0;
				if (visible && seen) msg_format(Ind, "%^s no longer looks stunned!", m_name);
				//else msg_format(Ind, "%^s no longer sounds stunned!", m_name);
			}
		}

		/* Fully healed? */
		if (m_ptr->hp >= m_ptr->maxhp) {
			m_ptr->hp = m_ptr->maxhp;
			if (visible && seen) msg_format(Ind, "%^s looks REALLY healthy!", m_name);
			//else msg_format(Ind, "%^s sounds REALLY healthy!", m_name);
		}
		/* Partially healed */
		else {
			if (visible && seen) msg_format(Ind, "%^s looks healthier.", m_name);
			//else msg_format(Ind, "%^s sounds healthier.", m_name);
		}

		/* Redraw (later) if needed */
		update_health(m_idx);

		/* Cancel fear */
		if (m_ptr->monfear) {
			/* Cancel fear */
			m_ptr->monfear = 0;
			if (visible && seen) msg_format(Ind, "%^s recovers %s courage.", m_name, m_poss);
		}

		break;

	/* RF6_XXX2X6 */
	/* RF6_S_ANIMALS */
	case RF6_OFFSET+3:
		disturb(Ind, 1, 0);
		if (monst_check_antimagic(Ind, m_idx)) break;
		for (k = 0; k < 4; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_ANIMAL, 1, clone_summoning);
		HANDLE_SUMMON("something", "some animals")
		break;

	/* RF6_BLINK */
	case RF6_OFFSET+4:
		if (monst_check_antimagic(Ind, m_idx)) break;

		/* No teleporting within no-tele vaults and such */
		if (zcave[oy][ox].info & CAVE_STCK) {
			//msg_format(Ind, "%^s fails to blink.", m_name);
			break;
		}

/*		if (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC)) {
			msg_format(Ind, "%^s fails to blink.", m_name);
			break;
		}
*/
		//if (monst_check_grab(Ind, m_idx)) break;
		/* it's low b/c check for spellcast is already done */
		if (monst_check_grab(m_idx, 50, "teleport")) break;
		if (teleport_away(m_idx, 10) && visible) {
			//disturb(Ind, 1, 0);
			if (blind) msg_print(Ind, "You hear something blink away.");
			else msg_format(Ind, "%^s blinks away.", m_name);
#ifdef USE_SOUND_2010
			/* redudant: already done in teleport_away()
			sound_near_monster(m_idx, "blink", NULL, SFX_TYPE_MON_SPELL);
			*/
#endif
		}
		break;

	/* RF6_TPORT */
	case RF6_OFFSET+5:
		if (monst_check_antimagic(Ind, m_idx)) break;

		/* No teleporting within no-tele vaults and such */
		if (zcave[oy][ox].info & CAVE_STCK) {
			//msg_format(Ind, "%^s fails to teleport.", m_name);
			break;
		}

/*		if (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC)) {
			msg_format(Ind, "%^s fails to teleport.", m_name);
			break;
		}
*/
		//if (monst_check_grab(Ind, m_idx)) break;
		if (monst_check_grab(m_idx, 50, "teleport")) break;
		if (teleport_away(m_idx, MAX_SIGHT * 2 + 5) && visible) {
			//disturb(Ind, 1, 0);
			if (blind) msg_print(Ind, "You hear something teleport away.");
			else msg_format(Ind, "%^s teleports away.", m_name);
#ifdef USE_SOUND_2010
			sound_near_monster(m_idx, "teleport", NULL, SFX_TYPE_MON_SPELL);
#endif
		}
		break;

	/* RF6_XXX3X6 */
	/* RF6_RAISE_DEAD */
	case RF6_OFFSET+6:
		break;

	/* RF6_XXX4X6 */
	/* RF6_S_BUG */
	case RF6_OFFSET+7:
		disturb(Ind, 1, 0);
		for (k = 0; k < 6; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_BUG, 1, clone_summoning);
		//note: it was actually 'codes some..', not 'summons some..'
		HANDLE_SUMMON("many things", "some software bugs")
		break;

	/* RF6_TELE_TO */
	case RF6_OFFSET+8:
		if (monst_check_antimagic(Ind, m_idx)) break;
		if (p_ptr->martyr) break;

		/* No teleporting within no-tele vaults and such */
		if ((zcave[oy][ox].info & CAVE_STCK) || (zcave[y][x].info & CAVE_STCK)) {
			msg_format(Ind, "%^s fails to command you to return.", m_name);
			break;
		}

/*		if (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC)) {
			msg_format(Ind, "%^s fails to command you to return.", m_name);
			break;
		}
*/
		disturb(Ind, 1, 0);
		if (m_ptr->r_idx != RI_ZU_AON) { /* can always TELE_TO */
			int chance = (195 - p_ptr->skill_sav) / 2;
			if (p_ptr->res_tele) chance = 50;
			/* Hack -- duplicated check to avoid silly message */
			if (p_ptr->anti_tele || check_st_anchor(wpos, p_ptr->py, p_ptr->px) ||
			    magik(chance)) {
				msg_format(Ind, "%^s commands you to return, but you don't care.", m_name);
				break;
			}
		}
		stop_shooting_till_kill(Ind);
		msg_format(Ind, "%^s commands you to return.", m_name);
		teleport_player_to(Ind, m_ptr->fy, m_ptr->fx);
		break;

	/* RF6_TELE_AWAY */
	case RF6_OFFSET+9: {
		int chance = (195 - p_ptr->skill_sav) / 2;
		if (p_ptr->res_tele) chance = 50;

		if (monst_check_antimagic(Ind, m_idx)) break;
		if (p_ptr->martyr) break;

		/* No teleporting within no-tele vaults and such */
		if ((zcave[oy][ox].info & CAVE_STCK) || (zcave[y][x].info & CAVE_STCK)) {
			msg_format(Ind, "%^s fails to teleport you away.", m_name);
			break;
		}

/*		if (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC)) {
			msg_format(Ind, "%^s fails to teleport you away.", m_name);
			break;
		}
*/
		disturb(Ind, 1, 0);
		/* Hack -- duplicated check to avoid silly message */
		if (p_ptr->anti_tele || check_st_anchor(wpos, p_ptr->py, p_ptr->px) || magik(chance)) {
			msg_format(Ind, "%^s tries to teleport you away in vain.", m_name);
			break;
		}
		msg_format(Ind, "%^s teleports you away.", m_name);
		msg_print_near_monvar(Ind, m_idx,
		    format("%^s teleports %s away.", m_name_real, p_ptr->name),
		    format("%^s teleports %s away.", m_name, p_ptr->name),
		    format("It teleports %s away.", p_ptr->name));
		teleport_player(Ind, 100, TRUE);
		break;
		}

	/* RF6_TELE_LEVEL */
	case RF6_OFFSET+10:
		if (monst_check_antimagic(Ind, m_idx)) break;
		if (p_ptr->martyr) break;

		/* No teleporting within no-tele vaults and such */
		if ((zcave[oy][ox].info & CAVE_STCK) || (zcave[y][x].info & CAVE_STCK)) {
			msg_format(Ind, "%^s fails to teleport you away.", m_name);
			break;
		}

/*		if (p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC)) {
			msg_format(Ind, "%^s fails to teleport you away.", m_name);
			break;
		}
*/
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles strangely.", m_name);
		else msg_format(Ind, "%^s gestures at your feet.", m_name);
		if (p_ptr->resist_nexus)
			msg_print(Ind, "You are unaffected!");
		else if (rand_int(100) < p_ptr->skill_sav)
			msg_print(Ind, "You resist the effects!");
		else {
			msg_print_near_monvar(Ind, m_idx,
			    format("%^s teleports %s away.", m_name_real, p_ptr->name),
			    format("%^s teleports %s away.", m_name, p_ptr->name),
			    format("It teleports %s away.", p_ptr->name));
			teleport_player_level(Ind, FALSE);
		}
		update_smart_learn(m_idx, DRS_NEXUS);
		break;

	/* RF6_XXX5 */
	/* RF6_S_RNG */
	case RF6_OFFSET+11:
		disturb(Ind, 1, 0);
		for (k = 0; k < 6; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_RNG, 1, clone_summoning);
		//note: it was actually 'codes some..', not 'summons some..'
		HANDLE_SUMMON("many things", "some RNGs")
		break;

	/* RF6_DARKNESS */
	case RF6_OFFSET+12:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		else msg_format(Ind, "%^s gestures in shadow.", m_name);
		(void)unlite_area(Ind, 0, 3);
		break;

	/* RF6_TRAPS */
	case RF6_OFFSET+13:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles and cackles evilly.", m_name);
		else msg_format(Ind, "%^s casts a spell and cackles evilly.", m_name);
		(void)trap_creation(Ind, 3, magik(rlev) ? (magik(30) ? 3 : 2) : 1);
		break;

	/* RF6_FORGET */
	case RF6_OFFSET+14:
		disturb(Ind, 1, 0);
		msg_format(Ind, "%^s tries to blank your mind.", m_name);
#ifdef USE_SOUND_2010
		/* should be ok to just abuse the insanity sfx for this? */
		sound(Ind, "insanity", NULL, SFX_TYPE_MON_SPELL, TRUE);
#endif

		if ((rand_int(100) < p_ptr->skill_sav && !(p_ptr->esp_link_flags & LINKF_OPEN)) /* An open mind invites psi attacks */
		    || (p_ptr->pclass == CLASS_MINDCRAFTER && magik(75)))
			msg_print(Ind, "You resist the effects!");
		else if (lose_all_info(Ind))
			msg_print(Ind, "Your memories fade away.");
		break;

	/* RF6_XXX6X6 */
	/* RF6_S_DRAGONRIDER */
	case RF6_OFFSET+15:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 1; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_DRAGONRIDER, 1, clone_summoning);
		HANDLE_SUMMON("something", "a dragonrider")
		break;

	/* RF6_XXX7X6 */
	/* RF6_SUMMON_KIN */
	case RF6_OFFSET+16: {
		char tmp[MAX_CHARS];

		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		sprintf(tmp, "%s %s", m_poss, ((r_ptr->flags1) & RF1_UNIQUE ? "minions" : "kin"));

		summon_kin_type = r_ptr->d_char; /* Big hack */
		for (k = 0; k < 6; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_KIN, 1, clone_summoning);
		HANDLE_SUMMON("many things", tmp)
		break;
		}

	/* RF6_XXX8X6 */
	/* RF6_S_HI_DEMON */
	case RF6_OFFSET+17:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);

#if 1 /* probably intended only for Oremorj? (which is currently JOKEANGBAND) - C. Blue */
		if (m_ptr->r_idx == RI_OREMORJ) {
			if (summon_cyber(Ind, s_clone, clone_summoning)) {
				if (blind) msg_print(Ind, "You hear heavy steps nearby.");
				else msg_format(Ind, "%^s magically summons greater demons!", m_name);
				m_ptr->clone_summoning = clone_summoning;
			} else if (blind) msg_format(Ind, "%^s mumbles.", m_name);
			break;
		}
#endif

		for (k = 0; k < 8; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_HI_DEMON, 1, clone_summoning);
		HANDLE_SUMMON2("You feel hellish auras appear nearby.", "greater demons")
		break;

	/* RF6_S_MONSTER */
	case RF6_OFFSET+18:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 1; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_MONSTER, 1, clone_summoning);
		HANDLE_SUMMON("something", "help")
		break;

	/* RF6_S_MONSTERS */
	case RF6_OFFSET+19:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 8; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_MONSTER, 1, clone_summoning);
		HANDLE_SUMMON("many things", "monsters")
		break;

	/* RF6_S_ANT */
	case RF6_OFFSET+20:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 6; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_ANT, 1, clone_summoning);
		HANDLE_SUMMON("many things", "ants")
		break;

	/* RF6_S_SPIDER */
	case RF6_OFFSET+21:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 6; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_SPIDER, 1, clone_summoning);
		HANDLE_SUMMON("many things", "spiders")
		break;

	/* RF6_S_HOUND */
	case RF6_OFFSET+22:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 6; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_HOUND, 1, clone_summoning);
		HANDLE_SUMMON("many things", "hounds")
		break;

	/* RF6_S_HYDRA */
	case RF6_OFFSET+23:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 6; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_HYDRA, 1, clone_summoning);
		HANDLE_SUMMON("many things", "hydras")
		break;

	/* RF6_S_ANGEL */
	case RF6_OFFSET+24:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 1; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_ANGEL, 1, clone_summoning);
		HANDLE_SUMMON("something", "an angel")
		break;

	/* RF6_S_DEMON */
	case RF6_OFFSET+25:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 1; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_DEMON, 1, clone_summoning);
		HANDLE_SUMMON("something", "a hellish adversary")
		break;

	/* RF6_S_UNDEAD */
	case RF6_OFFSET+26:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 1; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_UNDEAD, 1, clone_summoning);
		HANDLE_SUMMON("something", "an undead adversary")
		break;

	/* RF6_S_DRAGON */
	case RF6_OFFSET+27:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 1; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_DRAGON, 1, clone_summoning);
		HANDLE_SUMMON("something", "a dragon")
		break;

	/* RF6_S_HI_UNDEAD */
	case RF6_OFFSET+28:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 8; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_HI_UNDEAD, 1, clone_summoning);
		HANDLE_SUMMON("many creepy things", "greater undead")
		break;

	/* RF6_S_HI_DRAGON */
	case RF6_OFFSET+29:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 8; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_HI_DRAGON, 1, clone_summoning);
		HANDLE_SUMMON("many powerful things", "ancient dragons")
		break;

	/* RF6_S_NAZGUL */
	case RF6_OFFSET+30:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 8; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_NAZGUL, 1, clone_summoning);
		for (k = 0; k < 8; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_HI_UNDEAD, 1, clone_summoning);
		HANDLE_SUMMON("many creepy things", "mighty undead opponents")
		break;

	/* RF6_S_UNIQUE */
	case RF6_OFFSET+31:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 8; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_UNIQUE, 1, clone_summoning);
		for (k = 0; k < 8; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_HI_MONSTER, 1, clone_summoning);
		HANDLE_SUMMON("many powerful things", "special opponents")
		break;

	/* RF0_S_HI_MONSTER */
	case RF0_OFFSET+0:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 1; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_HI_MONSTER, 1, clone_summoning);
		HANDLE_SUMMON("something", "help")
		break;

	/* RF0_S_HI_MONSTERS */
	case RF0_OFFSET+1:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 8; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_HI_MONSTER, 1, clone_summoning);
		HANDLE_SUMMON("many things", "monsters")
		break;

	/* RF0_S_HI_UNIQUE */
	case RF0_OFFSET+2:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		for (k = 0; k < 8; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_HI_UNIQUE, 1, clone_summoning);
		for (k = 0; k < 8; k++)
			count += summon_specific(wpos, ys, xs, rlev, s_clone, SUMMON_HI_MONSTER, 1, clone_summoning);
		HANDLE_SUMMON("many powerful things", "special opponents")
		break;

	/* RF0_BO_DISE */
	case RF0_OFFSET+3:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a disenchantment bolt of", m_name);
		bolt(Ind, m_idx, GF_DISENCHANT, 25 + damroll(4, 5) + (rlev * 3) / 2, SFX_BOLT_MAGIC);
		update_smart_learn(m_idx, DRS_DISEN);
		break;

	/* RF0_BA_DISE */
	case RF0_OFFSET+4:
		if (monst_check_antimagic(Ind, m_idx)) break;
		disturb(Ind, 1, 0);
		if (blind) msg_format(Ind, "%^s mumbles.", m_name);
		snprintf(p_ptr->attacker, sizeof(p_ptr->attacker), "%s casts a disenchantment ball of", m_name);
		ball(Ind, m_idx, GF_DISENCHANT, (40 + damroll(6, 10) + rlev * 4), y, x, srad);
		update_smart_learn(m_idx, DRS_DISEN);
		break;
	}


#ifdef OLD_MONSTER_LORE
	/* Remember what the monster did to us */
	if (seen) {
		/* Innate spell */
		if (thrown_spell < 32*4) {
			r_ptr->r_flags4 |= (1U << (thrown_spell - 32*3));
			if (r_ptr->r_cast_innate < MAX_UCHAR) r_ptr->r_cast_inate++;
		}
		/* Bolt or Ball */
		else if (thrown_spell < 32*5) {
			r_ptr->r_flags5 |= (1U << (thrown_spell - 32*4));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}
		/* Special spell */
		else if (thrown_spell < 32*6) {
			r_ptr->r_flags6 |= (1U << (thrown_spell - 32*5));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}
	}
#endif


	/* Always take note of monsters that kill you */
	if (p_ptr->death) r_ptr->r_deaths++;

#ifdef COMBO_AM_IC_CAP
	/* Reset combo-cap-checking */
	m_ptr->intercepted = 0;
#endif

	/* A spell was cast (or antimagic'ed) */
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
int mon_will_run(int Ind, int m_idx) {
	player_type *p_ptr = Players[Ind];
	monster_type *m_ptr = &m_list[m_idx];
	cave_type **zcave;

#ifdef ALLOW_TERROR
	monster_race *r_ptr = race_inf(m_ptr);

	u16b p_lev, m_lev;
	u16b p_chp, p_mhp;
	long m_chp, m_mhp;
	u32b p_val, m_val;

	if (!(zcave = getcave(&m_ptr->wpos))) return(FALSE);

 #if 0 // I'll run instead!
	/* Hack -- aquatic life outa water */
	if (zcave[m_ptr->fy][m_ptr->fx].feat != FEAT_DEEP_WATER) {
		if (r_ptr->flags7 & RF7_AQUATIC) return (TRUE);
	} else {
		if (!(r_ptr->flags3 & RF3_UNDEAD) &&
		    !(r_ptr->flags7 & (RF7_AQUATIC | RF7_CAN_SWIM | RF7_CAN_FLY) ))
			return(TRUE);
	}
 #else	// 0
	if (!monster_can_cross_terrain(zcave[m_ptr->fy][m_ptr->fx].feat, r_ptr, FALSE, zcave[m_ptr->fy][m_ptr->fx].info))
		return 999;
 #endif	// 0

#endif

	/* Keep monsters from running too far away */
	if (m_ptr->cdis > MAX_SIGHT + 5) return (FALSE);

	/* All "afraid" monsters will run away */
	if (m_ptr->monfear) return (TRUE);

#ifdef ALLOW_TERROR /* player level >> monster level -> 'terror' */
	/* only if monster has a mind */
	if ((r_ptr->flags3 & RF3_NONLIVING) || (r_ptr->flags2 & RF2_EMPTY_MIND)
	    || (r_ptr->flags3 & RF3_NO_FEAR) || (r_ptr->flags7 & RF7_FRIENDLY))
		return(FALSE);

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
static bool get_moves_flow(int Ind, int m_idx, int *yp, int *xp) {
	int i, y, x, y1, x1, when = 0, cost = 999;
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);
	player_type *p_ptr = Players[Ind];
	cave_type **zcave, *c_ptr;

	if (!(zcave = getcave(&m_ptr->wpos))) return FALSE;

	/* Monster flowing disabled */
	if (!flow_by_sound && !flow_by_smell) return (FALSE);

	/* Monster can go through rocks */
	if (r_ptr->flags2 & RF2_PASS_WALL) return (FALSE);
	if (r_ptr->flags2 & RF2_KILL_WALL) return (FALSE);

	/* Monster location */
	y1 = m_ptr->fy;
	x1 = m_ptr->fx;

	/* Monster grid */
	c_ptr = &zcave[y1][x1];

	/* The player is not currently near the monster grid */
	if (c_ptr->when < zcave[py][px]->when) {
		/* The player has never been near the monster grid */
		if (!c_ptr->when) return (FALSE);

		/* The monster is not allowed to track the player */
		if (!flow_by_smell) return (FALSE);
	}

	/* Monster is too far away to notice the player */
	if (c_ptr->cost > MONSTER_FLOW_DEPTH) return (FALSE);
	if (c_ptr->cost > r_ptr->aaf) return (FALSE);

	/* Hack -- Player can see us, run towards him */
	if (player_has_los_bold(Ind, y1, x1)) return (FALSE);

	/* Check nearby grids, diagonals first */
	for (i = 7; i >= 0; i--) {
		/* Get the location */
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];

		/* Ignore illegal locations */
		if (!zcave[y][x].when) continue;

		/* Ignore ancient locations */
		if (zcave[y][x].when < when) continue;

		/* Ignore distant locations */
		if (zcave[y][x].cost > cost) continue;

		/* Save the cost and time */
		when = zcave[y][x].when;
		cost = zcave[y][x].cost;

		/* Hack -- Save the "twiddled" location */
		(*yp) = p_ptr->py + 16 * ddy_ddd[i];
		(*xp) = p_ptr->px + 16 * ddx_ddd[i];
	}

	/* No legal move (?) */
	if (!when) return (FALSE);

	/* Success */
	return (TRUE);
}

#endif


#ifdef MONSTER_ASTAR
/* Get monster moves for A* pathfinding - C. Blue
 * Return values:
 * -3  ASTAR_DISTRIBUTE only:
 *     No result yet, we just interrupted the algorithm temporarily for
 *     this server frame, to resume it in the next.
 * -2  didn't invoke. Either there was no spare memory slot for us,
 *                    or we have direct LoS to player already,
 *                    or zcave bugged out :p.
 * -1  success.
 *  0  we can't move! Maybe enclosed in a stone prison or similar.
 *  1  we can get closer to the player but didn't find a complete path to
 *     actually reaching him completely.
 *  2  we can move, but all moves we were able to find would actually
 *     increase distance to the player.
 */
static int get_moves_astar(int Ind, int m_idx, int *yp, int *xp) {
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);
	player_type *p_ptr = Players[Ind];
	int i, ireal, j;
	int x, y, mx, my, px, py;
 #ifdef ASTAR_DISTRIBUTE
	int acc_last;
 #endif

	astar_list_open *ao;
	astar_list_closed *ac;
	int aoc, acc; //node counters for both lists
	astar_node *aonode, *acnode, min_node, tmp_node;
	int minF, minIdx = 0, destIdx = 0;
	cave_type **zcave, *c_ptr;
	bool skip, end = FALSE, found = FALSE;

	/* Did we get a spare A* table? */
	if (m_ptr->astar_idx == -1) return -2;

 #if 0 /* such a monster shouldn't have gotten ASTAR flag in the first place */
	/* Monster can go through permanent rocks even? Morgoth only, and he's never on
	   levels with mountains or other perma-wall obstacles that he cannot destroy. */
	if ((r_ptr->flags2 & RF2_PASS_WALL) && (r_ptr->flags2 & RF2_KILL_WALL)) return -2;
 #endif

	if (!(zcave = getcave(&m_ptr->wpos))) return -2; //paranoia

	/* Monster location */
	mx = m_ptr->fx;
	my = m_ptr->fy;

	/* Hack -- Player can see us, run towards him */
	if (player_has_los_bold(Ind, my, mx)) return -2;

	ao = &astar_info_open[m_ptr->astar_idx];
	ac = &astar_info_closed[m_ptr->astar_idx];
	aoc = ao->nodes;
	acc = ac->nodes;
	aonode = ao->node;
	acnode = ac->node;

	/* Player location */
	px = p_ptr->px;
	py = p_ptr->py;

 #ifdef ASTAR_DISTRIBUTE
	/* Initialise? */
	if (acc == 0) {
 #endif
		/* Initialise open and closed lists.
		   Note: This especially initialises all nodes' 'in_use' with zero. */
		memset(aonode, 0, sizeof(astar_node) * ASTAR_MAX_NODES);
		memset(acnode, 0, sizeof(astar_node) * ASTAR_MAX_NODES);

		/* Create our starting node and put it on the open list */
		aonode[0].x = mx;
		aonode[0].y = my;
		/* Note: F, G, H and parent_idx of this node have been initialised with zero, as they should be. */
		aonode[0].in_use = TRUE;
		aoc = 1;

 #ifndef ASTAR_DISTRIBUTE
		/* Closed list starts with zero nodes */
		acc = 0;
 #else
		/* If we leave the function in the middle of calculating,
		   return the special result 'temporarily on hold' */
		ao->result = -3;
	} else if (ao->result != -3) { /* We got a result! */
		/* Are we allowed to set it now? */
		//hack: we abused xp/yp to indicate that it's not yet our turn
		if (*xp == mx && *yp == my) {
			return -3; //not yet
		}

		switch (ao->result) {
		/* Result was 'success'? */
		case -1:
			/* Then use our valid result values to determine our movement */
			*xp = acnode[0].x;
			*yp = acnode[0].y;
			break;
		/* Result was 'no moves'? */
		case 0:
			/* We can't move at all, aka entombed?
			   get_moves() will try to blink or teleport in this case. */
			break;
		/* Result was 'indirect'? (aka out of reach/memory) */
		case 1:
			/* Currently we just try to get any closer at all in this case,
			   even if we might find out afterwards that this wasn't a good route.
			   (Still not worse than if we fell back to normal movement, so it's fine.) */
			//todo: use some optimized, higher-level pathing routine maybe, using rooms and hallways as nodes etc..

			/* Use our 'hope' result to determine our movement */
			*xp = acnode[0].x;
			*yp = acnode[0].y;
			break;
		/* Result was 'no good moves'? */
		case 2:
			/* We don't have moves that bring us closer.
			   get_moves() will sometimes try to blink in this case,
			   to possibly get past a hindering wall and open up a new path. */
			break;
		}

		/* Reset our list, for our next algorithm run */
		ac->nodes = 0;
		return ao->result;
	}

	/* Rememeber size of closed list, just so we won't immediately jump out again */
	acc_last = acc;
 #endif

	/* tmp_node is always 'in_use', hehe */
	tmp_node.in_use = TRUE;

	/* A-Star ends when open list is empty */
	while (aoc) {
 #ifdef ASTAR_DISTRIBUTE
		/* Stop here for now and resume in the next server frame? */
		if (acc > acc_last && acc % ASTAR_DISTRIBUTE == 0) {

			/* Write back */
			ao->nodes = aoc;
			ac->nodes = acc;

			return -3;
		}
 #endif
		minF = 8192; //something higher than any expected movement cost, even in a maze level =P
		/* Find node on open list with minimum F */
		for (i = 0, ireal = 0; ireal < aoc; i++) {
			/* skip notes marked as 'deleted' */
			if (!aonode[i].in_use) continue;
			ireal++;
			if (aonode[i].F < minF) {
				minF = aonode[i].F;
				minIdx = i;
			}
		}

		/* Pop that node off the open list and hold it in 'min_node' */
		min_node = aonode[minIdx];
		aonode[minIdx].in_use = FALSE; //it's now 'deleted'
		aoc--;

		/* Push our current node on the closed list */
		for (i = 0; i < ASTAR_MAX_NODES; i++) {
			if (acnode[i].in_use) continue;

			acnode[i] = min_node;
			acc++;
			break;
		}
		if (i == ASTAR_MAX_NODES) break; //oops, out of memory!

		/* Efficiency: Already set that partial info of all successor nodes, which is invariant for all of them.
		   There is really no need to do this over and over inside the loop below. ^^ */
		tmp_node.parent_idx = i; //parent node 'min_node' will in any case land on top of the closed list
		tmp_node.G = min_node.G + 1; //all grid distances in TomeNET are always 1

		/* Generate its surrounding successor nodes */
		for (j = 0; j < 8; j++) {
			x = min_node.x + ddx_ddd[j];
			y = min_node.y + ddy_ddd[j];

			/* Skip non-existant grids */
			if (!in_bounds(y, x)) continue;
			c_ptr = &zcave[y][x];

			/* Is it the player grid? We got you!
			   No matter if we can actually enter this grid or not, since we can still attack him anyway.
			   Note: We don't need to set xp/yp because they've already been initialised
			   with the player's location in our calling function get_moves(). */
			if (x == px && y == py) {
				end = TRUE;

				/* Add it to the closed list, still.. */
				for (i = 0; i < ASTAR_MAX_NODES; i++) {
					if (acnode[i].in_use) continue;

					acnode[i] = tmp_node;
					acc++;
					break;
				}
				if (i == ASTAR_MAX_NODES) break; //oops, out of memory!

				/* Note: x,y,H,F have not yet been set, but it doesn't matter :p -
				   we only need the parent_idx really. */

				/* Success */
				found = TRUE;
				destIdx = i;
				break;
			}

			/* Skip forbidden grids */
			if (!creature_can_enter3(r_ptr, c_ptr)) continue;

			/* Create and hold the successor node in 'tmp_node' to process it further */
			tmp_node.x = x;
			tmp_node.y = y;
			tmp_node.H = ASTAR_HEURISTICS(x, y, px, py);
			tmp_node.F = tmp_node.G + tmp_node.H;
			skip = FALSE;

			/* Compare this successor node with all nodes still in the open list */
			for (i = 0, ireal = 0; ireal < aoc; i++) {
				/* skip notes marked as 'deleted' */
				if (!aonode[i].in_use) continue;
				ireal++;

				/* Does the open list node have same position as the current successor node? */
				if (aonode[i].x == tmp_node.x && aonode[i].y == tmp_node.y) {
					/* ..it also hasn't better G than we do? */
					if (aonode[i].G <= tmp_node.G) {
						/* Then just discard this successor. */
						skip = TRUE;
						break;
					}
					/* Otherwise, remove this worse one from the open list */
					aonode[i].in_use = FALSE;
					aoc--;
				}
			}
			if (skip) continue;

			/* Compare this successor node with all nodes in the closed list */
			for (i = 0, ireal = 0; ireal < acc; i++) {
				/* skip notes marked as 'deleted' */
				if (!acnode[i].in_use) continue;
				ireal++;

				/* Does the closed list node have same position as the current successor node? */
				if (acnode[i].x == tmp_node.x && acnode[i].y == tmp_node.y) {
					/* ..it also hasn't better G than we do? */
					if (acnode[i].G <= tmp_node.G) {
						/* Then just discard this successor. */
						skip = TRUE;
						break;
					}
					/* Otherwise, remove this worse one from the closed list */
					acnode[i].in_use = FALSE;
					acc--;
				}
			}
			if (skip) continue;

			/* Ok, add this successor to the open list */
			for (i = 0; i < ASTAR_MAX_NODES; i++) {
				if (aonode[i].in_use) continue;

				aonode[i] = tmp_node;
				aoc++;
				break;
			}
			/* If (i == ASTAR_MAX_NODES) then oops - we're out of memory and have to discard this node.. */
		}

		/* We found a way or ran out of memory? */
		if (end) break;
	}

 #ifdef ASTAR_DISTRIBUTE
	/* Write back */
	ao->nodes = aoc;
	ac->nodes = acc;

	/* For ALL results, we can already reset acc now for our next algorithm run */
	if (*xp != mx || *yp != my) //hack: we abused xp/yp to indicate that it's not yet our turn
		/* It WAS our turn, so reset! */
		ac->nodes = 0;
 #endif

	/* We found a way? */
	if (found) {
#ifdef TEST_SERVER
s_printf("ASTAR: -1 (found, %d,%d,%d)\n", aoc, acc, turn);
#endif
		/* Backtrace the path from destination (player) to start (monster)
		   through the closed list, from player position to monster position. */
		i = destIdx;

		while (TRUE) {
			/* Stop one grid before the very first node (parent_idx = 0) aka
			   the monster's grid, as the first node after that one is our goal. */
			if (!acnode[i].parent_idx) break;
			/* Go back another step */
			i = acnode[i].parent_idx;
		}

 #ifdef ASTAR_DISTRIBUTE
		/* Remember result for when it's actually our turn? */
		if (*xp == mx && *yp == my) { //hack: we abused xp/yp to indicate that it's not yet our turn
			ao->result = -1;
			//another hack: abuse first closed node to store our x,y result temporarily
			acnode[0].x = acnode[i].x;
			acnode[0].y = acnode[i].y;
		} else {
			/* Return our movement coordinates */
			*xp = acnode[i].x;
			*yp = acnode[i].y;
		}
 #else
		/* Return our movement coordinates */
		*xp = acnode[i].x;
		*yp = acnode[i].y;
 #endif

		return -1;
	}

	/* No legal move at all, aka the only node we checked
	   (and put on the closed list accordingly) was our starter node? */
	else if (acc == 1) {
#ifdef TEST_SERVER
s_printf("ASTAR: 0 (no moves, %d,%d,%d)\n", aoc, acc, turn);
#endif
 #ifdef ASTAR_DISTRIBUTE
		/* Remember result for when it's actually our turn? */
		if (*xp == mx && *yp == my) //hack: we abused xp/yp to indicate that it's not yet our turn
			ao->result = 0;
 #endif
		/* Don't do anything herre if we can't move..
		   get_moves() will try to cast blink or teleport in this case. */
		return 0;
	}

	/* We didn't find a clear way, ending up out of memory.
	   Two possible reasons:
	   - target is too far or unreachable, but we can at least still move in a way that brings us closer.
	   - we just can't get closer (ways are blocked). */

	/* We found a way to get closer to the target at least? */
#if 0
#ifdef TEST_SERVER
s_printf("ASTAR: 1 (indirect, %d,%d,%d)\n", aoc, acc, turn);
#endif
 #ifdef ASTAR_DISTRIBUTE
	/* Remember result for when it's actually our turn? */
	if (*xp == mx && *yp == my) //hack: we abused xp/yp to indicate that it's not yet our turn
		ao->result = 1;
 #endif
	return 1;
#else
	/* Scan all grids on the closed list to find the one closest to the target.
	   Condition: Must be an improvement over our current position! (Otherwise
	   we'll have to return result '2' aka 'no good moves' instead.) */
	minF = distance(mx, my, px, py); //abuse for distance
	destIdx = -1;
	for (i = 0, ireal = 0; ireal < acc; i++) {
		/* skip notes marked as 'deleted' */
		if (!acnode[i].in_use) continue;
		ireal++;

		if ((j = distance(acnode[i].x, acnode[i].y, px, py)) < minF) {
			minF = j;
			destIdx = i;
		}
	}
	if (destIdx != -1) {
		/* Backtrace the path from destination (player) to start (monster)
		   through the closed list, from player position to monster position. */
		i = destIdx;
#ifdef TEST_SERVER
s_printf("ASTAR: 1 (indirect, %d,%d,%d) -> [%d]:%d,%d d:%d\n", aoc, acc, turn, i, acnode[i].x, acnode[i].y, minF);
#endif

		while (TRUE) {
			/* Stop one grid before the very first node (parent_idx = 0) aka
			   the monster's grid, as the first node after that one is our goal. */
			if (!acnode[i].parent_idx) break;
			/* Go back another step */
			i = acnode[i].parent_idx;
		}

 #ifdef ASTAR_DISTRIBUTE
		/* Remember result for when it's actually our turn? */
		if (*xp == mx && *yp == my) { //hack: we abused xp/yp to indicate that it's not yet our turn
			ao->result = 1;
			//another hack: abuse first closed node to store our x,y result temporarily
			acnode[0].x = acnode[i].x;
			acnode[0].y = acnode[i].y;
		} else {
			/* Return our movement coordinates */
			*xp = acnode[i].x;
			*yp = acnode[i].y;
		}
 #else
		/* Return our movement coordinates */
		*xp = acnode[i].x;
		*yp = acnode[i].y;
 #endif

		return 1;
	}
#endif

	/* We can move, but didn't find any move that could get us closer to the target */
#ifdef TEST_SERVER
s_printf("ASTAR: 2 (no good moves, %d,%d,%d)\n", aoc, acc, turn);
#endif
 #ifdef ASTAR_DISTRIBUTE
	/* Remember result for when it's actually our turn? */
	if (*xp == mx && *yp == my) //hack: we abused xp/yp to indicate that it's not yet our turn
		ao->result = 2;
 #endif
	return 2;
}
#endif


/* Is the monster willing to avoid that grid?	- Jir - */
static bool monster_is_safe(int m_idx, monster_type *m_ptr, monster_race *r_ptr, cave_type *c_ptr) {
	effect_type *e_ptr;
	//cptr name;
	int dam;

#if 1	/* moved for efficiency */
	if (!c_ptr->effect) return (TRUE);
	if (r_ptr->flags2 & RF2_STUPID) return (TRUE);
#endif

	e_ptr = &effects[c_ptr->effect];

	/* It's mine :) */
	if (e_ptr->who == m_idx) return (TRUE);

	dam = e_ptr->dam;
	//name = r_name_get(m_ptr);

#if 0
	/* XXX Make sure to add whatever might be needed!
	   Maybe use approx_damage() for this - C. Blue */
	switch (e_ptr->type) {
		case GF_ACID:
			if (r_ptr->flags3 & RF3_IM_ACID) dam = 0;
			else if (r_ptr->flags9 & RF9_RES_ACID) dam /= 4;
			break;
		case GF_ELEC:
			if (r_ptr->flags3 & RF3_IM_ELEC) dam = 0;
			else if (r_ptr->flags9 & RF9_RES_ELEC) dam /= 4;
			break;
		case GF_FIRE:
			if (r_ptr->flags3 & RF3_IM_FIRE) dam = 0;
			else if (r_ptr->flags9 & RF9_RES_FIRE) dam /= 4;
			break;
		case GF_COLD:
			if (r_ptr->flags3 & RF3_IM_COLD) dam = 0;
			else if (r_ptr->flags3 & RF3_UNDEAD) dam = 0;
			else if (r_ptr->flags9 & RF9_RES_COLD) dam /= 4;
			break;
		case GF_POIS:
			if (r_ptr->flags3 & RF3_IM_POIS) dam = 0;
			else if (r_ptr->flags3 & RF3_UNDEAD) dam = 0;
			else if (r_ptr->flags3 & RF3_NONLIVING) dam = 0;
			else if (r_ptr->d_char == 'A') dam = 0;
			else if (r_ptr->flags9 & RF9_RES_POIS) dam /= 4;
			break;
		case GF_WATER:
		case GF_VAPOUR:
		case GF_WAVE:
			if (r_ptr->flags9 & RF9_IM_WATER) dam = 0;
			else if (r_ptr->flags7 & RF7_AQUATIC) dam /= 9;
			else if (r_ptr->flags3 & RF3_RES_WATE) dam /= 4;
			break;

		/* all effects that are bad for monsters: */
		case GF_PLASMA:
		case GF_HOLY_ORB:
		case GF_LITE:
		case GF_DARK:
		case GF_LITE_WEAK:
		case GF_STARLITE:
		case GF_DARK_WEAK:
		case GF_SHARDS:
		case GF_SOUND:
		case GF_CONFUSION:
		case GF_FORCE:
		case GF_INERTIA:
		case GF_MANA:
		case GF_METEOR:
		case GF_ICE:
		case GF_CHAOS:
		case GF_NETHER:
		case GF_DISENCHANT:
		case GF_NEXUS:
		case GF_TIME:
		case GF_GRAVITY:
		case GF_KILL_WALL:
		case GF_OLD_POLY:
		case GF_OLD_SLOW:
		case GF_OLD_CONF:
		case GF_OLD_SLEEP:
		case GF_OLD_DRAIN:
		case GF_AWAY_UNDEAD:
		case GF_AWAY_EVIL:
		case GF_AWAY_ALL:
		case GF_TURN_UNDEAD:
		case GF_TURN_EVIL:
		case GF_TURN_ALL:
		case GF_DISP_UNDEAD:
		case GF_DISP_EVIL:
		case GF_DISP_ALL:
		case GF_EARTHQUAKE:
		case GF_STUN:
		case GF_PSI:
		case GF_HOLY_FIRE:
		case GF_DISINTEGRATE:
		case GF_HELL_FIRE:
		case GF_MAKE_GLYPH:
		case GF_CURSE:
		case GF_WATERPOISON:
		case GF_ICEPOISON:
		case GF_HAVOC:
		case GF_INFERNO:
		case GF_DETONATION:
		case GF_ROCKET:
		case GF_DEC_STR:
		case GF_DEC_DEX:
		case GF_DEC_CON:
		case GF_RUINATION:
		case GF_NUKE:
		case GF_BLIND:
		case GF_HOLD:
//		case GF_DOMINATE:
		case GF_UNBREATH:
		case GF_WAVE:
		case GF_DISP_DEMON:
//		case GF_HAND_DOOM:
		case GF_STOP:
		case GF_STASIS:
		/* To remove some hacks? */
		case GF_THUNDER:
		case GF_ANNIHILATION:
			break;

		default: /* no need to avoid healing cloud or similar effects - C. Blue */
			dam = 0;
	}
#else
	/* Use new function 'approx_damage()' for this - C. Blue */
	dam = approx_damage(m_idx, dam, e_ptr->type);
#endif

#if 0 /* original */
	return (m_ptr->hp >= dam * 30);
#else /* less exploitable, ie avoid pushing monsters around without them able to retaliate */
	return (m_ptr->hp >= dam * 20);
#endif
}

#if 0	/* Replaced by monster_can_cross_terrain */
/* This should be more generic, of course.	- Jir - */
static bool monster_is_comfortable(monster_race *r_ptr, cave_type *c_ptr)
{
	/* No worry */
	if ((r_ptr->flags3 & RF3_UNDEAD) ||
			(r_ptr->flags7 & (RF7_CAN_SWIM | RF7_CAN_FLY) ))
		return (TRUE);

	/* I'd like to be under the sea ./~ */
	if (r_ptr->flags7 & RF7_AQUATIC) return (c_ptr->feat == FEAT_DEEP_WATER);
	else return (c_ptr->feat != FEAT_DEEP_WATER);
}
#endif	// 0

#if SAFETY_RADIUS > 0
/*
 * Those functions can be bandled into one generic find_* function
 * using hooks maybe.
 */

/*
 * Choose a location w/o bad effect near a monster for it to run toward.
 * - Jir -
 */
static bool find_noeffect(int m_idx, int *yp, int *xp)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);

	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

	int y, x, d = 1, i;
	int gy = 0, gx = 0, gdis = 0;

	/* Hack -- please don't run to northwest always */
	char dy = magik(50) ? 1 : -1;
	char dx = magik(50) ? 1 : -1;

	cave_type **zcave;
	/* paranoia */
	if(!(zcave = getcave(&m_ptr->wpos))) return(FALSE);

	/* Start with adjacent locations, spread further */
	for (i = 1; i <= tdi[SAFETY_RADIUS]; i++) {
		if (i == tdi[d]) {
			d++;
			if (gdis) break;
		}

		y = fy + tdy[i] * dy;
		x = fx + tdx[i] * dx;

		/* Skip illegal locations */
		if (!in_bounds(y, x)) continue;

		/* Skip locations in a wall */
#if 0
		if (!cave_floor_bold(zcave, y, x) &&
				!((r_ptr->flags2 & (RF2_PASS_WALL)) ||
					(r_ptr->flags2 & (RF2_KILL_WALL))))
			continue;
#else
		if (!creature_can_enter2(r_ptr, &zcave[y][x])) continue;
#endif

		/* Check for absence of bad effect */
		if (!monster_is_safe(m_idx, m_ptr, r_ptr, &zcave[y][x])) continue;

#ifdef MONSTER_FLOW
		/* Check for "availability" (if monsters can flow) */
		if (flow_by_sound) {
			/* Ignore grids very far from the player */
			if (zcave[y][x].when < zcave[py][px].when) continue;

			/* Ignore too-distant grids */
			if (zcave[y][x].cost > zcave[fy][fx].cost + 2 * d) continue;
		}
#endif
		/* Remember if further than previous */
		if (d > gdis) {
			gy = y;
			gx = x;
			gdis = d;
		}
	}

	/* Check for success */
	if (gdis > 0) {
		/* Good location */
		(*yp) = fy - gy;
		(*xp) = fx - gx;

		/* Found safe place */
		return (TRUE);
	}


	/* No safe place */
	return (FALSE);
}

/*
 * Choose a location suitable for that monster.
 * For now, it's for aquatics to get back to the water.
 * - Jir -
 */
static bool find_terrain(int m_idx, int *yp, int *xp)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);

	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

#if 0
	byte feat = FEAT_DEEP_WATER;	// maybe feat[10] or sth
	bool negate = FALSE;
#endif	// 0

	int y, x, d = 1, i;
	int gy = 0, gx = 0, gdis = 0;

	cave_type **zcave;
	/* paranoia */
	if (!(zcave = getcave(&m_ptr->wpos))) return(FALSE);

#if 0
	/* What do you want? */
	if (r_ptr->flags7 & RF7_AQUATIC) feat = FEAT_DEEP_WATER;
	else
	{
		feat = FEAT_DEEP_WATER;
		negate = TRUE;
	}
//	else return (TRUE);
#endif	// 0

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

		/* Skip bad locations */
#if 0
		if (!negate && zcave[y][x].feat != feat) continue;
		if (negate && zcave[y][x].feat == feat) continue;
#endif	// 0
		if (!monster_can_cross_terrain(zcave[y][x].feat, r_ptr, FALSE, zcave[y][x].info)) continue;

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
		/* Remember if further than previous */
		if (d > gdis)
		{
			gy = y;
			gx = x;
			gdis = d;
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


	/* No safe place */
	return (FALSE);
}

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
	if (!(zcave = getcave(&m_ptr->wpos))) return(FALSE);

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
#if 0
		if (!cave_floor_bold(zcave, y, x)) continue;
#else
		if (!creature_can_enter2(race_inf(m_ptr), &zcave[y][x])) continue;
#endif

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
		if (!projectable(&m_ptr->wpos, y, x, py, px, MAX_RANGE))
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
#if 0
				if (!cave_floor_bold(zcave, y, x)) continue;
#else
				if (!creature_can_enter2(race_inf(m_ptr), &zcave[y][x])) continue;
#endif

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
				if (!projectable(&m_ptr->wpos, y, x, py, px, MAX_RANGE))
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

	int y, x, d = 1, dis, i;
	int gy = 0, gx = 0, gdis = 999, min;

	cave_type **zcave;
	/* paranoia */
	if (!(zcave = getcave(&m_ptr->wpos))) return(FALSE);

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
#if 0
		if (!cave_floor_bold(zcave, y, x)) continue;
#else
		/* The monster assumes that grids it cannot enter work for making a good hiding place.
		   It might be surprised though if the player can indeed cross some of those grids ;) */
		if (!creature_can_enter2(race_inf(m_ptr), &zcave[y][x])) continue;
#endif

		/* Check for hidden, available grid */
		if (!player_can_see_bold(Ind, y, x) &&
#ifdef DOUBLE_LOS_SAFETY
		    clean_shot(&m_ptr->wpos, fy, fx, y, x, MAX_RANGE, m_idx))
#else
		    clean_shot(&m_ptr->wpos, fy, fx, y, x, MAX_RANGE))
#endif
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
//				if (!cave_floor_bold(zcave, y, x)) continue;
				if (!creature_can_enter2(race_inf(m_ptr), &zcave[y][x])) continue;

				/* Check distance */
				if (distance(y, x, fy, fx) != d) continue;

				/* Check for hidden, available grid */
				if (!player_can_see_bold(Ind, y, x) && clean_shot(&m_ptr->wpos, fy, fx, y, x, MAX_RANGE))
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
	u32b f1, f2, f3, f4, f5, f6, esp;
	u32b flg3 = 0L;

	if (artifact_p(o_ptr) && (rand_int(150) > r_ptr->level)) return (FALSE);

	/* Extract some flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* React to objects that hurt the monster */
	if (f1 & TR1_KILL_DRAGON) flg3 |= RF3_DRAGON;
	if (f1 & TR1_SLAY_DRAGON) flg3 |= RF3_DRAGON;
	if (f1 & TR1_SLAY_TROLL) flg3 |= RF3_TROLL;
	if (f1 & TR1_SLAY_GIANT) flg3 |= RF3_GIANT;
	if (f1 & TR1_SLAY_ORC) flg3 |= RF3_ORC;
	if (f1 & TR1_SLAY_DEMON) flg3 |= RF3_DEMON;
	if (f1 & TR1_KILL_DEMON) flg3 |= RF3_DEMON;
	if (f1 & TR1_SLAY_UNDEAD) flg3 |= RF3_UNDEAD;
	if (f1 & TR1_KILL_UNDEAD) flg3 |= RF3_UNDEAD;
	if (f1 & TR1_SLAY_ANIMAL) flg3 |= RF3_ANIMAL;
	if (f1 & TR1_SLAY_EVIL) flg3 |= RF3_EVIL;

	/* The object cannot be picked up by the monster */
	if ((r_ptr->flags3 & flg3) && (rand_int(150) > r_ptr->level)) return (FALSE);

	/* Ok */
	return (TRUE);
}

#ifdef MONSTER_DIG_FACTOR
static int digging_difficulty(byte feat)
{
#if 0
	if (!(f_info[feat].flags1 & FF1_TUNNELABLE) ||
			(f_info[feat].flags1 & FF1_PERMANENT)) return (3000);
#endif	// 0

	if ((feat == FEAT_SANDWALL_H) || (feat == FEAT_SANDWALL_K)) return (25);
	if (feat == FEAT_RUBBLE) return (30);
	if (feat == FEAT_TREE) return (50);
	if (feat == FEAT_BUSH) return (35);
	if (feat == FEAT_IVY) return (20);
	if (feat == FEAT_DEAD_TREE) return (30);	/* hehe it's evil */
	if (feat >= FEAT_WALL_EXTRA) return (200);
	if (feat >= FEAT_MAGMA)
	{
		if ((feat - FEAT_MAGMA) & 0x01) return (100);
		else return (50);
	}

	/* huh? ...it's not our role */
	return (3000);
}
#endif

bool mon_allowed_pickup(int tval) {
	if (tval != TV_GAME
#ifndef MONSTER_PICKUP_GOLD
	    && tval != TV_GOLD
#endif
#ifdef MON_IGNORE_KEYS
	    && tval != TV_KEY
#endif
#ifdef MON_IGNORE_SPECIAL
	    && tval != TV_SPECIAL
#endif
	)
	    return TRUE;
	return FALSE;
}

/*
 * Choose "logical" directions for monster movement
 */
/*
 * TODO: Aquatic out of water should rush for one
 * Changed to bool for get_moves_astar():
 *  get_moves_astar() may actually use movement spells!
 *  So we need to know if that was done and end our turn prematurely accordingly,
 *  it will return FALSE usually, and TRUE if A* movement spells were used. - C. Blue
 */
static bool get_moves(int Ind, int m_idx, int *mm) {
	player_type *p_ptr = Players[Ind];

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);

	int y, ay, x, ax, mwr;
	int move_val = 0;

	int y2 = p_ptr->py;
	int x2 = p_ptr->px;
	bool done = FALSE, c_blue_ai_done = FALSE;	// not used fully (FIXME)
	bool close_combat = FALSE;
	bool robin = (m_ptr->r_idx == RI_ROBIN && !m_ptr->extra);


	/* If player is next to us, we might eventually decide to attack instead of avoiding terrain problems etc */
	if (ABS(m_ptr->fx - x2) <= 1 && ABS(m_ptr->fy - y2) <= 1 &&
	    (!(r_ptr->flags7 & RF7_AI_ANNOY) || m_ptr->taunted))
		close_combat = TRUE;

#ifdef MONSTER_ASTAR
	/* Monster uses A* pathfinding algorithm? - C. Blue */
	if ((r_ptr->flags0 & RF0_ASTAR) && (m_ptr->astar_idx != -1))
		switch (get_moves_astar(Ind, m_idx, &y2, &x2)) {
		case 0: /* No moves (entombed) - blink or teleport */
			//no worky: m_ptr->ai_state |= AI_STATE_EFFECT;

			if (!(r_ptr->flags6 & (RF6_BLINK | RF6_TPORT))) break; //proceed normally.

			{
			int spellmove = 0; //two choices, blink or teleport
			char m_name[MNAME_LEN];
			int chance, rlev;
			cave_type **zcave = getcave(&m_ptr->wpos);

			chance = (r_ptr->freq_innate + r_ptr->freq_spell) / 2;
			/* Only do spells occasionally */
			if (rand_int(100) >= chance) break; //proceed normally.

			if (r_ptr->flags6 & RF6_BLINK) spellmove += 2;
			if (r_ptr->flags6 & RF6_TPORT) spellmove += 4;

			switch (spellmove + rand_int(2)) {
			case 2: case 3: case 6:
				/* We blink */
				spellmove = 1;
				break;
			case 4: case 5: case 7:
				/* We teleport */
				spellmove = 2;
				break;
			}

			/* --- COPY/PASTED from make_attack_spell() --- keep in sync! --- */

			/* Get the monster name (or "it") */
			monster_desc(Ind, m_name, m_idx, 0x00);

			/* Extract the monster level */
			rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

#ifndef STUPID_MONSTER_SPELLS
			if (!(r_ptr->flags2 & (RF2_STUPID))) {
				int factor = 0;

				/* Extract the 'stun' factor */
				if (m_ptr->stunned > 50) factor += 25;
				if (m_ptr->stunned) factor += 15;

				if (magik(25 - (rlev + 3) / 4) || magik(factor)) return TRUE;
 #ifdef GENERIC_INTERCEPTION
				if (monst_check_grab(m_idx, 85, "cast")) return TRUE;
 #else
				if (monst_check_grab(m_idx, 75, "cast")) return TRUE;
 #endif
			}
#endif

			if (monst_check_antimagic(Ind, m_idx)) return TRUE;

			/* No teleporting within no-tele vaults and such */
			if (zcave && //paranoia
			    zcave[m_ptr->fy][m_ptr->fx].info & CAVE_STCK) {
				//msg_format(Ind, "%^s fails to blink.", m_name);
				return TRUE;
			}

			if (monst_check_grab(m_idx, 50, "teleport")) return TRUE;

			switch (spellmove) {
			case 1:
				if (teleport_away(m_idx, 10) && p_ptr->mon_vis[m_idx]) {
					if (p_ptr->blind) msg_print(Ind, "You hear something blink away.");
					else msg_format(Ind, "%^s blinks away.", m_name);
				}
				return TRUE;
			case 2:
				if (teleport_away(m_idx, MAX_SIGHT * 2 + 5) && p_ptr->mon_vis[m_idx]) {
					if (p_ptr->blind) msg_print(Ind, "You hear something teleport away.");
					else msg_format(Ind, "%^s teleports away.", m_name);
#ifdef USE_SOUND_2010
					sound_near_monster(m_idx, "teleport", NULL, SFX_TYPE_MON_SPELL);
#endif
				}
				return TRUE;
			}
			break; //we didn't do anything - can't happen anymore at this point though - paranoia
			}
		case 2: /* No good moves (can't get closer) - blink or wait */
			if (!(r_ptr->flags6 & RF6_BLINK)) break; //we can't blink. Proceed normally.
			if (rand_int(3)) break; //we decided to not to blink, but just wait..

			/* Blink to try and get past a hindering wall possibly */
			{
			char m_name[MNAME_LEN];
			int chance, rlev;
			cave_type **zcave = getcave(&m_ptr->wpos);

			chance = (r_ptr->freq_innate + r_ptr->freq_spell) / 2;
			/* Only do spells occasionally */
			if (rand_int(100) >= chance) break; //proceed normally.

			/* --- COPY/PASTED from make_attack_spell() --- keep in sync! --- */

			/* Get the monster name (or "it") */
			monster_desc(Ind, m_name, m_idx, 0x00);

			/* Extract the monster level */
			rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

#ifndef STUPID_MONSTER_SPELLS
			if (!(r_ptr->flags2 & (RF2_STUPID))) {
				int factor = 0;

				/* Extract the 'stun' factor */
				if (m_ptr->stunned > 50) factor += 25;
				if (m_ptr->stunned) factor += 15;

				if (magik(25 - (rlev + 3) / 4) || magik(factor)) return TRUE;
 #ifdef GENERIC_INTERCEPTION
				if (monst_check_grab(m_idx, 85, "cast")) return TRUE;
 #else
				if (monst_check_grab(m_idx, 75, "cast")) return TRUE;
 #endif
			}
#endif

			if (monst_check_antimagic(Ind, m_idx)) return TRUE;

			/* No teleporting within no-tele vaults and such */
			if (zcave && //paranoia
			    zcave[m_ptr->fy][m_ptr->fx].info & CAVE_STCK) {
				//msg_format(Ind, "%^s fails to blink.", m_name);
				return TRUE;
			}

			if (monst_check_grab(m_idx, 50, "teleport")) return TRUE;

			if (teleport_away(m_idx, 10) && p_ptr->mon_vis[m_idx]) {
				if (p_ptr->blind) msg_print(Ind, "You hear something blink away.");
				else msg_format(Ind, "%^s blinks away.", m_name);
			}
			return TRUE;
			}
		/* Either we're fine or we don't want to do anything special:
		   -1 = we're performing A* fine.
		    1 = we can't find a path to the target, but we can at least get closer. */
		case -1:
		case 1:
			break;
		case -2:
			/* We couldn't acquire an A* slot,
			   or we actually have LoS to the player.
			   -> Fall back to normal movement. */
#ifdef TEST_SERVER
s_printf("ASTAR_FALLBACK\n");
#endif
			break;
		case -3:
#ifdef TEST_SERVER
s_printf("ASTAR_INCOMPLETE\n");
#endif
			/* Shouldn't happen here. -3 should only be returned in process_monsters_astar().
			   It can happen though if ASTAR_DISTRIBUTE is too small compared to ASTAR_MAX_NODES
			   and therefore cannot finish up the path-finding before the monster can move again.
			   Or in other words, if the monster moves too fast for the path-finding to complete.
			   So in case this actually does happen, we could..
			   -fall back to normal movement routine in between while A* has not yet finished.
			    This is bad and can cause wrong movement, but still better than nothing.
			   -actually stop the monster for this turn, to allow path-finding hopefully finish
			    next turn (or even worse, later..). */
#if 0
			/* Fall back to normal movement */
			x2 = p_ptr->px; //repair coordinates for normal movement,
			y2 = p_ptr->py; //since they were marker-hacked by get_moves_astar().
			break;
#else
			/* Skip turn to wait for A* path-finding to complete.
			   Hm, results seem not really noticable? :/ */

			/* Note: We need assume that we need to consume energy nevertheless, so we don't stack it like crazy.
			   However, monster code that refills energy actually doesn't allow having more than level_speed
			   (1 turn) worth of energy, so it doesn't matter.
			   The advantage of not consuming energy is that we can move right away when ASTAR_DISTRIBUTE
			   finishes. So, compensate here to keep our energy up: */
			m_ptr->energy += level_speed(&m_ptr->wpos); //cancel out energy reduction applied after returning

			return TRUE;
#endif
		}
 #ifdef MONSTER_FLOW
	else
 #endif
#endif
#ifdef MONSTER_FLOW
	/* Flow towards the player */
	if (flow_by_sound) {
		/* Flow towards the player */
		(void)get_moves_flow(Ind, m_idx, &y2, &x2);
	}
#endif

	/* Extract the "pseudo-direction" */
	y = m_ptr->fy - y2;
	x = m_ptr->fx - x2;

	if ((r_ptr->flags1 & RF1_NEVER_MOVE) || m_ptr->no_move) {
		done = TRUE;
		m_ptr->last_target = 0;
	}
#if SAFETY_RADIUS > 0
	else if ((m_ptr->ai_state & AI_STATE_EFFECT)
	    && !close_combat) {
		/* Try to find safe place */
		if (find_noeffect(m_idx, &y, &x)) {
			done = TRUE;
			m_ptr->last_target = 0;
		}
	}
#endif
	if (!done && (m_ptr->ai_state & AI_STATE_TERRAIN)
	    && !close_combat) {
		/* Try to find safe place */
		if (find_terrain(m_idx, &y, &x)) {
			done = TRUE;
			m_ptr->last_target = 0;
		}
	}
	/* Apply fear if possible and necessary */
	else if ((mwr = mon_will_run(Ind, m_idx)) != FALSE
	    && !(mwr == 999 && close_combat)) {
#if SAFETY_RADIUS == 0
		/* XXX XXX Not very "smart" */
		y = (-y), x = (-x);

		/* Hack -- run in zigzags */
		if (!x) x = magik(50) ? y : -y;
		if (!y) y = magik(50) ? x : -x;

		done = TRUE;
		m_ptr->last_target = 0;
#else
		/* Try to find safe place */
		if (!find_safety(Ind, m_idx, &y, &x)) {
 #if 0 /* nulled, so the monster keeps pursuing the player */
			/* This is not a very "smart" method XXX XXX */
			y = (-y);
			x = (-x);
 #else
			/* just make it slightly irregular movement? */
			if (mwr != 999 || !rand_int(8)) {
				y = (-y);
				x = (-x);

				/* Hack -- run in zigzags */
				if (!x) x = magik(50) ? y : -y;
				if (!y) y = magik(50) ? x : -x;

				done = TRUE;
				m_ptr->last_target = 0;
			} else {
				/* but otherwise still pursue player */
				y = m_ptr->fy - y2;
				x = m_ptr->fx - x2;
			}
 #endif
		} else {
			/* Hack -- run in zigzags */
			if (!x) x = magik(50) ? y : -y;
			if (!y) y = magik(50) ? x : -x;

			done = TRUE;
			m_ptr->last_target = 0;
		}
#endif	/* SAFETY_RADIUS */
	}
	/* Tease the player */
	else if (((r_ptr->flags7 & RF7_AI_ANNOY) || robin) && !m_ptr->taunted) {
		/* Cheeze note: Use of distance() makes diagonal approaching preferable, with the monster
		   keeping only 2-3 grids distance instead of 3-4 for straight vertical/horizontal distances */
		if (distance(m_ptr->fy, m_ptr->fx, p_ptr->py, p_ptr->px) < ANNOY_DISTANCE) {
			y = -y;
			x = -x;
			/* so that they never get reversed again */
			done = TRUE;
			m_ptr->last_target = 0;

			/* do we have sprint or taunt? */
			if (p_ptr->warning_ai_annoy == 0 && p_ptr->mon_vis[m_idx]) {
				if ((p_ptr->melee_techniques & 0x0003)) {
					p_ptr->warning_ai_annoy = 1;
					msg_print(Ind, "\377yHINT: Use fighting techniques '\377osprint\377y' or '\377otaunt\377y' to catch monsters that try to");
					msg_print(Ind, "\377y      keep their distance to you. (Press keys \377o% z i\377y to set up a macro.)");
					msg_print(Ind, "\377y      Some monsters might need multiple taunts, some never fall for it.");
					s_printf("warning_ai_annoy (technique): %s\n", p_ptr->name);
#if 0 /* keep warning active for when the player learned techniques (if ever) */
				} else {
					p_ptr->warning_ai_annoy = 1;
					msg_print(Ind, "\377yHINT: Use ranged attacks to kill monsters that try to keep their distance.");
					msg_print(Ind, "\377y      If it's weak, throwing an item ('\377ov\377y') might work too, eg flasks of oil.");
					s_printf("warning_ai_annoy (ranged): %s\n", p_ptr->name);
#endif
				}
			}
		}
	}
	
#ifdef C_BLUE_AI
	/* Anti-cheeze vs Hit&Run-tactics if player has slightly superiour speed:
	   Monster tries to make player approach so it gets attack turns! -C. Blue */
	else if ((
 #if 1 /* Different behaviour, depending on monster type and level? */
		/* Demons are reckless, undead/nonliving are rarely intelligent, animals neither */
		((r_ptr->level >= 30) &&
		!(r_ptr->flags3 & (RF3_NONLIVING | RF3_UNDEAD | RF3_ANIMAL | RF3_DEMON)) &&
		!(r_ptr->flags2 & RF2_EMPTY_MIND)) ||
		/* Elder animals have great instinct or intelligence */
		((r_ptr->level >= 50) &&
		!(r_ptr->flags3 & (RF3_NONLIVING | RF3_UNDEAD | RF3_DEMON)) &&
		!(r_ptr->flags2 & RF2_EMPTY_MIND)) ||
 #endif
		(r_ptr->flags2 & RF2_SMART)) && !(r_ptr->flags2 & RF2_STUPID) &&
		/* Distance must == 2; distance() won't work for diagonals here */
		((ABS(m_ptr->fy - y2) == 2 && ABS(m_ptr->fx - x2) <= 2) ||
		(ABS(m_ptr->fy - y2) <= 2 && ABS(m_ptr->fx - x2) == 2)) &&
		/* Player must be faster than the monster to cheeze */
		(p_ptr->pspeed > m_ptr->mspeed) && rand_int(600) &&
 #if 1 /* Watch our/player's HP? */
		/* As long as we have good HP there's no need to hold back,
		   [if player is low on HP we should try to attack him anyways,
		   this is not basing on consequent logic though, since we probably still can't hurt him] */
		(((m_ptr->hp <= (m_ptr->maxhp * 3) / 4) && (p_ptr->chp > (p_ptr->mhp * 3) / 4)) ||
		/* If we're very low on HP, only try to attack the player if he's hurt *badly* */
		((m_ptr->hp < (m_ptr->maxhp * 1) / 2) && ((p_ptr->chp >= (p_ptr->mhp * 1) / 5) || (p_ptr->chp >= 200)))) &&
 #endif
		/* No need to keep a distance if the player doesn't pose
		   a potential threat in close combat: */
		(p_ptr->s_info[SKILL_MASTERY].value >= 3000 || p_ptr->s_info[SKILL_MARTIAL_ARTS].value >= 10000) &&
		/* If there's los we can just cast a spell -
		   this assumes the monster can cast damaging spells, might need fixing */
//EXPERIMENTALLY COMMENTED OUT		!los(&p_ptr->wpos, y2, x2, m_ptr->fy, m_ptr->fx) &&
		/* EMPTY_MINDed skeleton, zombie, spectral egos don't care anymore */
		(m_ptr->ego != 1 && m_ptr->ego != 2 && m_ptr->ego != 4) &&
		/* Only stay back if the player moved away from us -
		   this assumes the monster didn't perform a RAND_ turn, might need fixing */
		(m_ptr->last_target == p_ptr->id)) {

		int xt,yt, more_monsters_nearby = 0;
		cave_type **zcave = getcave(&p_ptr->wpos);
		if (zcave) {
			monster_type *mx_ptr;
			for (yt = m_ptr->fy - 5; yt <= m_ptr->fy + 5; yt ++)
			for (xt = m_ptr->fx - 5; xt <= m_ptr->fx + 5; xt ++) {
				if (in_bounds(yt,xt) && (zcave[yt][xt].m_idx > 0) &&
				   !(yt == m_ptr->fy && xt == m_ptr->fx)) {
					mx_ptr = &m_list[zcave[yt][xt].m_idx];
					if (!mx_ptr->csleep && !mx_ptr->confused && !mx_ptr->monfear && (mx_ptr->level * 3 > m_ptr->level))
						more_monsters_nearby++;
				}
			}
			/* Don't approach the player without enough support! */
			if (more_monsters_nearby < 2) {
				bool clockwise = TRUE; /* circle the player clockwise */
				bool tested_so_far = FALSE;
				/* Often stay still and don't move at all to save your
				   turn for attacking in case the player appraoches. */
				if (rand_int(100)) return FALSE;
				/* Move randomly without getting closer -
				   this will cancel the actual point of C_BLUE_AI!
				   Moving randomly is merely eye-candy, that's why
				   rand_int() was tested, to restrict it greatly. */
				if (rand_int(2)) clockwise = FALSE; /* circle the player anti-clockwise */
				for (yt = m_ptr->fy - 1; yt <= m_ptr->fy + 1; yt ++)
				for (xt = m_ptr->fx - 1; xt <= m_ptr->fx + 1; xt ++) {
					if (in_bounds(yt, xt) && !(yt == m_ptr->fy && xt == m_ptr->fx) &&
					    /* Random target position mustn't change distance to player */
					    /* Better not enter a position perfectly diagonal to player */
					    ((ABS(yt - y2) == 2 && ABS(xt - x2) <= 1) ||
					    (ABS(yt - y2) <= 1 && ABS(xt - x2) == 2))) {
						/* Only 2 fields should ever pass the previous if-clause!
						   So we can separate them using boolean values: */
						if (clockwise != tested_so_far) {
							x = m_ptr->fx - xt;
							y = m_ptr->fy - yt;
							done = TRUE;
							c_blue_ai_done = TRUE;
							/* keep last_target on this player to remember to
							   still stay back from him in the next round */
						}
						tested_so_far = TRUE;
					}
				}
				/* If monster didn't want to move randomly,
				   just stay still (shouldn't happen though) */
				if (!c_blue_ai_done) return FALSE;
			}
		}
	}
#endif /* C_BLUE_AI */

#if 0
	/* Death orbs .. */
	if (r_ptr->flags2 & RF2_DEATH_ORB) {
		if (!los(m_ptr->fy, m_ptr->fx, y2, x2)) return FALSE;
	}
#endif

	//if (!stupid_monsters && (is_friend(m_ptr) < 0))
#ifndef STUPID_MONSTERS
	if (!done) {
		int tx = x2, ty = y2;
		cave_type **zcave;
		/* paranoia */
		if (!(zcave = getcave(&m_ptr->wpos))) return FALSE;
 #ifdef	MONSTERS_HIDE_HEADS
		/*
		 * Animal packs try to get the player out of corridors
		 * (...unless they can move through walls -- TY)
		 */
		if ((r_ptr->flags1 & RF1_FRIENDS) &&
			(r_ptr->flags3 & RF3_ANIMAL) &&
			!((r_ptr->flags2 & (RF2_PASS_WALL)) ||
			(r_ptr->flags2 & (RF2_KILL_WALL)) ||
			safe_area(Ind)))
		{
			int i, room = 0;

			/* Count room grids next to player */
			for (i = 0; i < 8; i++) {
				/* Check grid */
				if (zcave[ty + ddy_ddd[i]][tx + ddx_ddd[i]].info & (CAVE_ROOM)) {
					/* One more room grid */
					room++;
				}
			}

			/* Not in a room and strong player */
			if ((room < 8) && (p_ptr->chp > ((p_ptr->mhp * 3) / 4))) {
				/* Find hiding place */
				if (find_hiding(Ind, m_idx, &y, &x)) done = TRUE;
			}
		}
 #endif	// MONSTERS_HIDE_HEADS
 #ifdef	MONSTERS_GREEDY
		/* Monsters try to pick things up */
		if (!done && (r_ptr->flags2 & (RF2_TAKE_ITEM | RF2_KILL_ITEM)) &&
		    (zcave[m_ptr->fy][m_ptr->fx].o_idx) && magik(80)) {
			object_type *o_ptr = &o_list[zcave[m_ptr->fy][m_ptr->fx].o_idx];

			if (o_ptr->k_idx && mon_allowed_pickup(o_ptr->tval) &&
			    monster_can_pickup(r_ptr, o_ptr)) {
				/* Just Stay */
				return FALSE;
			}
		}
		if (!done && (r_ptr->flags2 & RF2_TAKE_ITEM) && magik(MONSTERS_GREEDY)) {
			int i;

			/* Find an empty square near the target to fill */
			for (i = 0; i < 8; i++) {
				/* Pick squares near itself (semi-randomly) */
				y2 = m_ptr->fy + ddy_ddd[(m_idx + i) & 7];
				x2 = m_ptr->fx + ddx_ddd[(m_idx + i) & 7];

				/* Ignore filled grids */
				if (!cave_empty_bold(zcave, y2, x2)) continue;

				/* look for booty */
				if (zcave[y2][x2].o_idx) {
					object_type *o_ptr = &o_list[zcave[y2][x2].o_idx];

					if (o_ptr->k_idx && mon_allowed_pickup(o_ptr->tval) &&
					    monster_can_pickup(r_ptr, o_ptr)) {
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
		if (!done && (r_ptr->flags1 & RF1_FRIENDS)) {
			int i;

			/* Find an empty square near the target to fill */
			for (i = 0; i < 8; i++) {
				/* Pick squares near target (semi-randomly) */
				y2 = ty + ddy_ddd[(m_idx + i) & 7];
				x2 = tx + ddx_ddd[(m_idx + i) & 7];

				/* Already there? */
				if ((m_ptr->fy == y2) && (m_ptr->fx == x2)) {
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

#ifdef C_BLUE_AI
	/* Don't waste turns if the player is hiding in non-passable
	   (to us) area to charge in a pattern that won't allow us to
	   get a turn if we keep moving back and forth pointlessly. */
	if ((
 #if 1 /* Different behaviour, depending on monster type and level? */
		/* Demons are reckless, undead/nonliving are rarely intelligent, animals neither */
		((r_ptr->level >= 30) &&
		!(r_ptr->flags3 & (RF3_NONLIVING | RF3_UNDEAD | RF3_DEMON)) &&
		!(r_ptr->flags2 & RF2_EMPTY_MIND)) || /* RF3_ANIMAL | */
		/* Elder animals have great instinct or intelligence */
		((r_ptr->level >= 50) && !(r_ptr->flags2 & RF2_EMPTY_MIND)) || /* no smart death orbs */
 #endif
		(r_ptr->flags2 & RF2_SMART)) && !(r_ptr->flags2 & RF2_STUPID) &&
		/* Distance must == 2; distance() won't work for diagonals here */
		((ABS(m_ptr->fy - y2) == 2 && ABS(m_ptr->fx - x2) <= 2) ||
		(ABS(m_ptr->fy - y2) <= 2 && ABS(m_ptr->fx - x2) == 2)) &&
		/* Player must be faster than the monster to cheeze */
		(p_ptr->pspeed > m_ptr->mspeed) && rand_int(600) &&
 #if 1 /* Watch our/player's HP? */
		/* As long as we have good HP there's no need to hold back,
		   [if player is low on HP we should try to attack him anyways,
		   this is not basing on consequent logic though, since we probably still can't hurt him] */
		(((m_ptr->hp <= (m_ptr->maxhp * 3) / 5) && (p_ptr->chp > (p_ptr->mhp * 3) / 4)) ||
		/* If we're very low on HP, only try to attack the player if he's hurt *badly* */
		((m_ptr->hp < (m_ptr->maxhp * 1) / 4) && (p_ptr->chp >= (p_ptr->mhp * 1) / 5))) &&
 #endif
		/* No need to keep a distance if the player doesn't pose
		   a potential threat in close combat: */
		(p_ptr->s_info[SKILL_MASTERY].value >= 3000 || p_ptr->s_info[SKILL_MARTIAL_ARTS].value >= 10000) &&
		/* If there's los we can just cast a spell -
		   this assumes the monster can cast damaging spells, might need fixing */
//EXPERIMENTALLY COMMENTED OUT		!los(&p_ptr->wpos, y2, x2, m_ptr->fy, m_ptr->fx) &&
		/* EMPTY_MINDed skeleton, zombie, spectral egos don't care anymore */
		(m_ptr->ego != 1 && m_ptr->ego != 2 && m_ptr->ego != 4) &&
		/* Only stay back if the player moved away from us -
		   this assumes the monster didn't perform a RAND_ turn, might need fixing */
		(m_ptr->last_target == p_ptr->id))
	{
		/* Stay still if we have a perfect position towards the player -
		   no need to waste our turn then */
		if ((ABS(m_ptr->fy - y2) == 0) || (ABS(m_ptr->fx - x2) == 0)) return FALSE;
	}
#endif /* C_BLUE_AI */

	/* Extract the "absolute distances" */
	ax = ABS(x);
	ay = ABS(y);

	if (!done) {
		/* Hack -- chase player avoiding arrows
		 * In the real-time, it does work :)
		 */
		if (ax < 2 && ay > 5 &&
		    projectable(&m_ptr->wpos, m_ptr->fy, m_ptr->fx, y2, x2, MAX_RANGE))
		{
			x = (x > 0 || (!x && magik(50))) ? -ay / 2: ay / 2;
			ax = ay / 2;
		}
		if (ay < 2 && ax > 5 &&
		    projectable(&m_ptr->wpos, m_ptr->fy, m_ptr->fx, y2, x2, MAX_RANGE))
		{
			y = (y > 0 || (!y && magik(50))) ? -ax / 2: ax / 2;
			ay = ax / 2;
		}
	}

	/* Do something weird */
	if (y < 0) move_val += 8;
	if (x > 0) move_val += 4;

	/* Prevent the diamond maneuvre */
	if (ay > (ax << 1)) {
		move_val++;
		move_val++;
	} else if (ax > (ay << 1)) {
		move_val++;
	}

	/* Extract some directions */
	switch (move_val) {
	case 0:
		mm[0] = 9;
		if (ay > ax) {
			mm[1] = 8;
			mm[2] = 6;
			mm[3] = 7;
			mm[4] = 3;
		} else {
			mm[1] = 6;
			mm[2] = 8;
			mm[3] = 3;
			mm[4] = 7;
		}
		break;
	case 1:
	case 9:
		mm[0] = 6;
		if (y < 0) {
			mm[1] = 3;
			mm[2] = 9;
			mm[3] = 2;
			mm[4] = 8;
		} else {
			mm[1] = 9;
			mm[2] = 3;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
	case 2:
	case 6:
		mm[0] = 8;
		if (x < 0) {
			mm[1] = 9;
			mm[2] = 7;
			mm[3] = 6;
			mm[4] = 4;
		} else {
			mm[1] = 7;
			mm[2] = 9;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
	case 4:
		mm[0] = 7;
		if (ay > ax) {
			mm[1] = 8;
			mm[2] = 4;
			mm[3] = 9;
			mm[4] = 1;
		} else {
			mm[1] = 4;
			mm[2] = 8;
			mm[3] = 1;
			mm[4] = 9;
		}
		break;
	case 5:
	case 13:
		mm[0] = 4;
		if (y < 0) {
			mm[1] = 1;
			mm[2] = 7;
			mm[3] = 2;
			mm[4] = 8;
		} else {
			mm[1] = 7;
			mm[2] = 1;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
	case 8:
		mm[0] = 3;
		if (ay > ax) {
			mm[1] = 2;
			mm[2] = 6;
			mm[3] = 1;
			mm[4] = 9;
		} else {
			mm[1] = 6;
			mm[2] = 2;
			mm[3] = 9;
			mm[4] = 1;
		}
		break;
	case 10:
	case 14:
		mm[0] = 2;
		if (x < 0) {
			mm[1] = 3;
			mm[2] = 1;
			mm[3] = 6;
			mm[4] = 4;
		} else {
			mm[1] = 1;
			mm[2] = 3;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
	case 12:
		mm[0] = 1;
		if (ay > ax) {
			mm[1] = 2;
			mm[2] = 4;
			mm[3] = 3;
			mm[4] = 7;
		} else {
			mm[1] = 4;
			mm[2] = 2;
			mm[3] = 7;
			mm[4] = 3;
		}
		break;
	}

	return FALSE;
}

#ifdef ARCADE_SERVER
static void get_moves_arc(int targy, int targx, int m_idx, int *mm) {

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);

	int y, ay, x, ax;
	int move_val = 0;

	int y2 = targy;
	int x2 = targx;
	//bool done = FALSE;//, c_blue_ai_done = FALSE;	// not used fully (FIXME)


	/* Extract the "pseudo-direction" */
	y = m_ptr->fy - y2;
	x = m_ptr->fx - x2;

	if ((r_ptr->flags1 & RF1_NEVER_MOVE) || m_ptr->no_move) {
		//done = TRUE;
		m_ptr->last_target = 0;
	}


	/* Extract the "absolute distances" */
	ax = ABS(x);
	ay = ABS(y);


	/* Do something weird */
	if (y < 0) move_val += 8;
	if (x > 0) move_val += 4;

	/* Prevent the diamond maneuvre */
	if (ay > (ax << 1)) {
		move_val++;
		move_val++;
	} else if (ax > (ay << 1)) {
		move_val++;
	}

	/* Extract some directions */
	switch (move_val) {
		case 0:
		mm[0] = 9;
		if (ay > ax) {
			mm[1] = 8;
			mm[2] = 6;
			mm[3] = 7;
			mm[4] = 3;
		} else {
			mm[1] = 6;
			mm[2] = 8;
			mm[3] = 3;
			mm[4] = 7;
		}
		break;
		case 1:
		case 9:
		mm[0] = 6;
		if (y < 0) {
			mm[1] = 3;
			mm[2] = 9;
			mm[3] = 2;
			mm[4] = 8;
		} else {
			mm[1] = 9;
			mm[2] = 3;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
		case 2:
		case 6:
		mm[0] = 8;
		if (x < 0) {
			mm[1] = 9;
			mm[2] = 7;
			mm[3] = 6;
			mm[4] = 4;
		} else {
			mm[1] = 7;
			mm[2] = 9;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
		case 4:
		mm[0] = 7;
		if (ay > ax) {
			mm[1] = 8;
			mm[2] = 4;
			mm[3] = 9;
			mm[4] = 1;
		} else {
			mm[1] = 4;
			mm[2] = 8;
			mm[3] = 1;
			mm[4] = 9;
		}
		break;
		case 5:
		case 13:
		mm[0] = 4;
		if (y < 0) {
			mm[1] = 1;
			mm[2] = 7;
			mm[3] = 2;
			mm[4] = 8;
		} else {
			mm[1] = 7;
			mm[2] = 1;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
		case 8:
		mm[0] = 3;
		if (ay > ax) {
			mm[1] = 2;
			mm[2] = 6;
			mm[3] = 1;
			mm[4] = 9;
		} else {
			mm[1] = 6;
			mm[2] = 2;
			mm[3] = 9;
			mm[4] = 1;
		}
		break;
		case 10:
		case 14:
		mm[0] = 2;
		if (x < 0) {
			mm[1] = 3;
			mm[2] = 1;
			mm[3] = 6;
			mm[4] = 4;
		} else {
			mm[1] = 1;
			mm[2] = 3;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
		case 12:
		mm[0] = 1;
		if (ay > ax) {
			mm[1] = 2;
			mm[2] = 4;
			mm[3] = 3;
			mm[4] = 7;
		} else {
			mm[1] = 4;
			mm[2] = 2;
			mm[3] = 7;
			mm[4] = 3;
		}
		break;
	}
}
#endif

#ifdef RPG_SERVER
/*
 * Choose "logical" directions for pet movement
 * Returns TRUE to move, FALSE to stand still
 */
static bool get_moves_pet(int Ind, int m_idx, int *mm) {
	player_type *p_ptr;
	monster_type *m_ptr = &m_list[m_idx];

	int y, ay, x, ax;
	int move_val = 0;

	int tm_idx = 0;
	int y2, x2;

	if (Ind > 0) p_ptr = Players[Ind];
	else p_ptr = NULL;

	/* Lets find a target */

	if ((p_ptr != NULL) && (m_ptr->mind & GOLEM_ATTACK) && TARGET_BEING(p_ptr->target_who) && (p_ptr->target_who > 0 || check_hostile(Ind, -p_ptr->target_who)))
		tm_idx = p_ptr->target_who;
	else// if (m_ptr->mind & GOLEM_GUARD)
	{
		int sx, sy;
		s32b max_hp = 0;

		/* Scan grids around */
		for (sx = m_ptr->fx - 1; sx <= m_ptr->fx + 1; sx++)
		for (sy = m_ptr->fy - 1; sy <= m_ptr->fy + 1; sy++) {
			cave_type *c_ptr;
			cave_type **zcave;
			if (!in_bounds(sy, sx)) continue;

			/* ignore ourself */
			if (sx == m_ptr->fx && sy == m_ptr->fy) continue;

			/* no point if there are no players on depth */
			/* and it would crash anyway ;) */

			if (!(zcave = getcave(&m_ptr->wpos))) return FALSE;
			c_ptr = &zcave[sy][sx];

			if (!c_ptr->m_idx) continue;

			if (c_ptr->m_idx > 0) {
				if (max_hp < m_list[c_ptr->m_idx].maxhp) {
					max_hp = m_list[c_ptr->m_idx].maxhp;
					tm_idx = c_ptr->m_idx;
				}
			} else {
				if ((max_hp < Players[-c_ptr->m_idx]->mhp) && (m_ptr->owner != Players[-c_ptr->m_idx]->id)) {
					max_hp = Players[-c_ptr->m_idx]->mhp;
					tm_idx = c_ptr->m_idx;
				}
			}
		}
	}
	/* Nothing else to do ? */
	if ((p_ptr != NULL) && !tm_idx && (m_ptr->mind & GOLEM_FOLLOW))
		tm_idx = -Ind;

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
	if (ay > (ax << 1)) {
		move_val++;
		move_val++;
	} else if (ax > (ay << 1))
		move_val++;

	/* Extract some directions */
	switch (move_val) {
	case 0:
		mm[0] = 9;
		if (ay > ax) {
			mm[1] = 8;
			mm[2] = 6;
			mm[3] = 7;
			mm[4] = 3;
		} else {
			mm[1] = 6;
			mm[2] = 8;
			mm[3] = 3;
			mm[4] = 7;
		}
		break;
	case 1:
	case 9:
		mm[0] = 6;
		if (y < 0) {
			mm[1] = 3;
			mm[2] = 9;
			mm[3] = 2;
			mm[4] = 8;
		} else {
			mm[1] = 9;
			mm[2] = 3;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
	case 2:
	case 6:
		mm[0] = 8;
		if (x < 0) {
			mm[1] = 9;
			mm[2] = 7;
			mm[3] = 6;
			mm[4] = 4;
		} else {
			mm[1] = 7;
			mm[2] = 9;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
	case 4:
		mm[0] = 7;
		if (ay > ax) {
			mm[1] = 8;
			mm[2] = 4;
			mm[3] = 9;
			mm[4] = 1;
		} else {
			mm[1] = 4;
			mm[2] = 8;
			mm[3] = 1;
			mm[4] = 9;
		}
		break;
	case 5:
	case 13:
		mm[0] = 4;
		if (y < 0) {
			mm[1] = 1;
			mm[2] = 7;
			mm[3] = 2;
			mm[4] = 8;
		} else {
			mm[1] = 7;
			mm[2] = 1;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
	case 8:
		mm[0] = 3;
		if (ay > ax) {
			mm[1] = 2;
			mm[2] = 6;
			mm[3] = 1;
			mm[4] = 9;
		} else {
			mm[1] = 6;
			mm[2] = 2;
			mm[3] = 9;
			mm[4] = 1;
		}
		break;
	case 10:
	case 14:
		mm[0] = 2;
		if (x < 0) {
			mm[1] = 3;
			mm[2] = 1;
			mm[3] = 6;
			mm[4] = 4;
		} else {
			mm[1] = 1;
			mm[2] = 3;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
	case 12:
		mm[0] = 1;
		if (ay > ax) {
			mm[1] = 2;
			mm[2] = 4;
			mm[3] = 3;
			mm[4] = 7;
		} else {
			mm[1] = 4;
			mm[2] = 2;
			mm[3] = 7;
			mm[4] = 3;
		}
		break;
	}

	return TRUE;
}
#endif

/*
 * Choose "logical" directions for monster golem movement
 * Returns TRUE to move, FALSE to stand still
 */
static bool get_moves_golem(int Ind, int m_idx, int *mm) {
	player_type *p_ptr;
	monster_type *m_ptr = &m_list[m_idx];

	int y, ay, x, ax;
	int move_val = 0;
	int tm_idx = 0;

	int y2, x2;

	if (Ind > 0) p_ptr = Players[Ind];
	else p_ptr = NULL;

	/* Lets find a target */

	if ((p_ptr != NULL) && (m_ptr->mind & GOLEM_ATTACK) && TARGET_BEING(p_ptr->target_who) && (p_ptr->target_who > 0 || check_hostile(Ind, -p_ptr->target_who)))
		tm_idx = p_ptr->target_who;
	else if (m_ptr->mind & GOLEM_GUARD) {
		int sx, sy;
		s32b max_hp = 0;

		/* Scan grids around */
		for (sx = m_ptr->fx - 1; sx <= m_ptr->fx + 1; sx++)
		for (sy = m_ptr->fy - 1; sy <= m_ptr->fy + 1; sy++) {
			cave_type *c_ptr;
			cave_type **zcave;
			if (!in_bounds(sy, sx)) continue;

			/* ignore ourself */
			if (sx == m_ptr->fx && sy == m_ptr->fy) continue;

			/* no point if there are no players on depth */
			/* and it would crash anyway ;) */

			if (!(zcave = getcave(&m_ptr->wpos))) return FALSE;
			c_ptr = &zcave[sy][sx];

			if(!c_ptr->m_idx) continue;

			if (c_ptr->m_idx > 0) {
				if (max_hp < m_list[c_ptr->m_idx].maxhp) {
					max_hp = m_list[c_ptr->m_idx].maxhp;
					tm_idx = c_ptr->m_idx;
				}
			} else {
				if ((max_hp < Players[-c_ptr->m_idx]->mhp) && (m_ptr->owner != Players[-c_ptr->m_idx]->id)) {
					max_hp = Players[-c_ptr->m_idx]->mhp;
					tm_idx = c_ptr->m_idx;
				}
			}
		}
	}
	/* Nothing else to do ? */
	if ((p_ptr != NULL) && !tm_idx && (m_ptr->mind & GOLEM_FOLLOW))
		tm_idx = -Ind;

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
	if (ay > (ax << 1)) {
		move_val++;
		move_val++;
	} else if (ax > (ay << 1))
		move_val++;

	/* Extract some directions */
	switch (move_val) {
	case 0:
		mm[0] = 9;
		if (ay > ax) {
			mm[1] = 8;
			mm[2] = 6;
			mm[3] = 7;
			mm[4] = 3;
		} else {
			mm[1] = 6;
			mm[2] = 8;
			mm[3] = 3;
			mm[4] = 7;
		}
		break;
	case 1:
	case 9:
		mm[0] = 6;
		if (y < 0) {
			mm[1] = 3;
			mm[2] = 9;
			mm[3] = 2;
			mm[4] = 8;
		} else {
			mm[1] = 9;
			mm[2] = 3;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
	case 2:
	case 6:
		mm[0] = 8;
		if (x < 0) {
			mm[1] = 9;
			mm[2] = 7;
			mm[3] = 6;
			mm[4] = 4;
		} else {
			mm[1] = 7;
			mm[2] = 9;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
	case 4:
		mm[0] = 7;
		if (ay > ax) {
			mm[1] = 8;
			mm[2] = 4;
			mm[3] = 9;
			mm[4] = 1;
		} else {
			mm[1] = 4;
			mm[2] = 8;
			mm[3] = 1;
			mm[4] = 9;
		}
		break;
	case 5:
	case 13:
		mm[0] = 4;
		if (y < 0) {
			mm[1] = 1;
			mm[2] = 7;
			mm[3] = 2;
			mm[4] = 8;
		} else {
			mm[1] = 7;
			mm[2] = 1;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
	case 8:
		mm[0] = 3;
		if (ay > ax) {
			mm[1] = 2;
			mm[2] = 6;
			mm[3] = 1;
			mm[4] = 9;
		} else {
			mm[1] = 6;
			mm[2] = 2;
			mm[3] = 9;
			mm[4] = 1;
		}
		break;
	case 10:
	case 14:
		mm[0] = 2;
		if (x < 0) {
			mm[1] = 3;
			mm[2] = 1;
			mm[3] = 6;
			mm[4] = 4;
		} else {
			mm[1] = 1;
			mm[2] = 3;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
	case 12:
		mm[0] = 1;
		if (ay > ax) {
			mm[1] = 2;
			mm[2] = 4;
			mm[3] = 3;
			mm[4] = 7;
		} else {
			mm[1] = 4;
			mm[2] = 2;
			mm[3] = 7;
			mm[4] = 3;
		}
		break;
	}

	return TRUE;
}

static bool get_moves_questor(int Ind, int m_idx, int *mm) {
	monster_type *m_ptr = &m_list[m_idx];
	int y, ay, x, ax;
	int move_val = 0;
	int tm_idx = 0;
	int y2, x2;

	/* Lets find a target adjacent to us */
	{
		int sx, sy;
		s32b max_hp = 0;

		/* Scan grids around */
		for (sx = m_ptr->fx - 1; sx <= m_ptr->fx + 1; sx++)
		for (sy = m_ptr->fy - 1; sy <= m_ptr->fy + 1; sy++) {
			cave_type *c_ptr;
			cave_type **zcave;
			if (!in_bounds(sy, sx)) continue;

			/* ignore ourself */
			if (sx == m_ptr->fx && sy == m_ptr->fy) continue;

			/* no point if there are no players on depth */
			/* and it would crash anyway ;) */

			if (!(zcave = getcave(&m_ptr->wpos))) return FALSE;
			c_ptr = &zcave[sy][sx];

			if (!c_ptr->m_idx) continue;
			if (c_ptr->m_idx > 0) {
				if (max_hp < m_list[c_ptr->m_idx].maxhp) {
					max_hp = m_list[c_ptr->m_idx].maxhp;
					tm_idx = c_ptr->m_idx;
				}
			} else {
				if ((max_hp < Players[-c_ptr->m_idx]->mhp) && (m_ptr->owner != Players[-c_ptr->m_idx]->id)) {
					max_hp = Players[-c_ptr->m_idx]->mhp;
					tm_idx = c_ptr->m_idx;
				}
			}
		}
	}

	/* adjacent hostile monster to attack? then attack instead of moving */
	if (tm_idx) {
		y2 = (tm_idx > 0) ? m_list[tm_idx].fy : Players[-tm_idx]->py;
		x2 = (tm_idx > 0) ? m_list[tm_idx].fx : Players[-tm_idx]->px;
		//isn't this BAD paranoia?
		if (!(inarea(&m_ptr->wpos, (tm_idx > 0)? &m_list[tm_idx].wpos : &Players[-tm_idx]->wpos))) return FALSE;
	}
	/* continue moving */
	else {
		y2 = m_ptr->desty;
		x2 = m_ptr->destx;
	}

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
	if (ay > (ax << 1)) {
		move_val++;
		move_val++;
	} else if (ax > (ay << 1)) {
		move_val++;
	}

	/* Extract some directions */
	switch (move_val) {
	case 0:
		mm[0] = 9;
		if (ay > ax) {
			mm[1] = 8; mm[2] = 6; mm[3] = 7; mm[4] = 3;
		} else {
			mm[1] = 6; mm[2] = 8; mm[3] = 3; mm[4] = 7;
		}
		break;
	case 1:
	case 9:
		mm[0] = 6;
		if (y < 0) {
			mm[1] = 3; mm[2] = 9; mm[3] = 2; mm[4] = 8;
		} else {
			mm[1] = 9; mm[2] = 3; mm[3] = 8; mm[4] = 2;
		}
		break;
	case 2:
	case 6:
		mm[0] = 8;
		if (x < 0) {
			mm[1] = 9; mm[2] = 7; mm[3] = 6; mm[4] = 4;
		} else {
			mm[1] = 7; mm[2] = 9; mm[3] = 4; mm[4] = 6;
		}
		break;
	case 4:
		mm[0] = 7;
		if (ay > ax) {
			mm[1] = 8; mm[2] = 4; mm[3] = 9; mm[4] = 1;
		} else {
			mm[1] = 4; mm[2] = 8; mm[3] = 1; mm[4] = 9;
		}
		break;
	case 5:
	case 13:
		mm[0] = 4;
		if (y < 0) {
			mm[1] = 1; mm[2] = 7; mm[3] = 2; mm[4] = 8;
		} else {
			mm[1] = 7; mm[2] = 1; mm[3] = 8; mm[4] = 2;
		}
		break;
	case 8:
		mm[0] = 3;
		if (ay > ax) {
			mm[1] = 2; mm[2] = 6; mm[3] = 1; mm[4] = 9;
		} else {
			mm[1] = 6; mm[2] = 2; mm[3] = 9; mm[4] = 1;
		}
		break;
	case 10:
	case 14:
		mm[0] = 2;
		if (x < 0) {
			mm[1] = 3; mm[2] = 1; mm[3] = 6; mm[4] = 4;
		} else {
			mm[1] = 1; mm[2] = 3; mm[3] = 4; mm[4] = 6;
		}
		break;
	case 12:
		mm[0] = 1;
		if (ay > ax) {
			mm[1] = 2; mm[2] = 4; mm[3] = 3; mm[4] = 7;
		} else {
			mm[1] = 4; mm[2] = 2; mm[3] = 7; mm[4] = 3;
		}
		break;
	}
	return TRUE;
}


/* Determine whether the player is invisible to a monster */
/* Maybe we'd better add SEE_INVIS flags to the monsters.. */
static bool player_invis(int Ind, monster_type *m_ptr, int dist) {
	player_type *p_ptr = Players[Ind];
	monster_race *r_ptr = race_inf(m_ptr);
	s16b inv, mlv;

	if (r_ptr->flags2 & RF2_INVISIBLE ||
	    r_ptr->flags1 & RF1_QUESTOR ||
	    r_ptr->flags3 & RF3_DRAGONRIDER)	/* have ESP */
		return(FALSE);
	/* since RF1_QUESTOR is currently not used/completely implemented,
	   I hard-code Morgoth and Sauron and Zu-Aon here - C. Blue */
	if ((m_ptr->r_idx == RI_SAURON) || (m_ptr->r_idx == RI_MORGOTH) || (m_ptr->r_idx == RI_ZU_AON)) return(FALSE);

	/* Probably they detect things by non-optical means */
	if (r_ptr->flags3 & RF3_NONLIVING && r_ptr->flags2 & RF2_EMPTY_MIND)
		return(FALSE);

	/* Added all monsters that can see through 'Cloaking' ability too - C. Blue */
	if (strchr("eAN", r_ptr->d_char) ||
	    ((r_ptr->flags1 & RF1_UNIQUE) && (r_ptr->flags2 & RF2_SMART) && (r_ptr->flags2 & RF2_POWERFUL)) ||
#ifdef MONSTER_ASTAR
	    (r_ptr->flags0 & RF0_ASTAR) || /* hmm.. */
#endif
	    (r_ptr->flags7 & RF7_NAZGUL))
		return(FALSE);

	inv = p_ptr->invis;

#ifdef VAMPIRIC_MIST
	/* mistform gives invisibility vs monsters */
	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST) {
		/* using same formula as for invis granted by items */
		int j = (p_ptr->lev > 50 ? 50 : p_ptr->lev) * 4 / 5;

		if (j > inv) inv = j;
	}
#endif

	if (p_ptr->ghost) inv += 10;

	/* Bad conditions */
	if (p_ptr->cur_lite) inv /= 2;
	if (p_ptr->aggravate) inv /= 3;

	if (inv < 1) return(FALSE);

	mlv = (s16b) r_ptr->level + r_ptr->aaf - dist * 2;

#if 0	// PernMangband one
	if (r_ptr->flags3 & RF3_NO_SLEEP)
		mlv += 5;
	if (r_ptr->flags3 & RF3_DRAGON)
		mlv += 10;
	if (r_ptr->flags3 & RF3_UNDEAD)
		mlv += 12;
	if (r_ptr->flags3 & RF3_DEMON)
		mlv += 10;
	if (r_ptr->flags3 & RF3_ANIMAL)
		mlv += 3;
	if (r_ptr->flags3 & RF3_ORC)
		mlv -= 15;
	if (r_ptr->flags3 & RF3_TROLL)
		mlv -= 10;
	if (r_ptr->flags2 & RF2_STUPID)
		mlv /= 2;
	if (r_ptr->flags2 & RF2_SMART)
		mlv = (mlv * 5) / 4;
	if (mlv < 1)
		mlv = 1;
#else	// ToME one
	if (r_ptr->flags3 & RF3_NO_SLEEP)
		mlv += 10;
	if (r_ptr->flags3 & RF3_DRAGON)
		mlv += 20;
	if (r_ptr->flags3 & RF3_UNDEAD)
		mlv += 15;
	if (r_ptr->flags3 & RF3_DEMON)
		mlv += 15;
	if (r_ptr->flags3 & RF3_ANIMAL)
		mlv += 15;
	if (r_ptr->flags3 & RF3_ORC)
		mlv -= 15;
	if (r_ptr->flags3 & RF3_TROLL)
		mlv -= 10;
	if (r_ptr->flags2 & RF2_STUPID)
		mlv /= 2;
	if (r_ptr->flags2 & RF2_SMART)
		mlv = (mlv * 5) / 4;
	if (mlv < 1)
		mlv = 1;

#endif	// 0

	return (inv >= randint((mlv * 10) / 7));
}

/*
 * Special NPC processing
 *
 * Call LUA stuff where needed.
 * Experimental
 * Evileye
 */
void process_npcs() {
#if 0
	struct cave_type **zcave;
	zcave = getcave(&Npcs[0].wpos);
	if (!Npcs[0].active) return;
	if (zcave != (cave_type**)NULL) {
		process_hooks(HOOK_NPCTEST, "d", &Npcs[0]);
	}
#endif
}


static player_type *get_melee_target(monster_race *r_ptr, monster_type *m_ptr, cave_type *c_ptr, bool pfriend) {
	int p_idx_target = c_ptr ? 0 - c_ptr->m_idx : 0;
	player_type *pd_ptr = p_idx_target ? Players[p_idx_target] : NULL;
	cave_type **zcave = getcave(&m_ptr->wpos);

	if ((!p_idx_target && c_ptr) || !zcave) return NULL; //paranoia x 2

#ifdef C_BLUE_AI_MELEE /* if multiple targets adjacent, choose between them */
	if (!m_ptr->confused) {
		bool keeping_previous_target = FALSE;
		int i, p_idx_chosen = p_idx_target, reason;
		int p_idx[9], targets = 0;
		int p_idx_weak[9], p_idx_medium[9];//, p_idx_strong[9];
		int weak_targets = 0, medium_targets = 0, strong_targets = 0;
		int p_idx_low[9], low_targets = 0, lowest_target_level = 100, highest_target_level = 0;
		cave_type *cd_ptr;
		int p_idx_non_distracting[9], non_distracting_targets = 0;
		int target_toughness = 0;
		int ox = m_ptr->fx, oy = m_ptr->fy;

		/* remember our previous target */
		m_ptr->last_target_melee = m_ptr->last_target_melee_temp;

		/* get all adjacent players */
		for (i = 0; i < 8; i++) {
			if (!in_bounds(oy + ddy_ddd[i], ox + ddx_ddd[i])) continue;

			cd_ptr = &zcave[oy + ddy_ddd[i]][ox + ddx_ddd[i]];

			/* found not a player? */
			if (cd_ptr->m_idx >= 0) continue;

			pd_ptr = Players[-(cd_ptr->m_idx)];

			/* get him if allowed */
			if ((m_ptr->owner == pd_ptr->id) || /* Don't attack your master! */
			    pd_ptr->admin_invinc /* Invincible players can't be atacked! */
			    /* non-hostile questors dont attack people */
			    || pfriend
 #ifdef TELEPORT_SURPRISES
			    || TELEPORT_SURPRISED(pd_ptr, r_ptr)
 #endif
			    ) /* surprise effect */
				continue;

			/* for when we don't have any target yet (AI_ANNOY specialty) */
			if (!p_idx_target) p_idx_target = pd_ptr->Ind;

			/* did we choose this player for target in our previous turn?
			   then lets stick with it and not change targets again */
			if (-cd_ptr->m_idx == m_ptr->last_target_melee &&
			    -cd_ptr->m_idx != m_ptr->switch_target) {
				keeping_previous_target = TRUE;
 #ifdef C_BLUE_AI_MELEE_NASTY
				break;
 #endif
			}

			targets++;
			p_idx[targets] = -cd_ptr->m_idx;

			/* did that player NOT use a 'distract' (ie detaunting) ability on this monster? */
			if (-cd_ptr->m_idx != m_ptr->switch_target) {
				non_distracting_targets++;
				p_idx_non_distracting[non_distracting_targets] = -cd_ptr->m_idx;

				/* remember whether player's class is vulnerable or tough in melee situation.
				   a pretty rough estimate, since skills are not taken into account at all. */
				switch (pd_ptr->pclass) {
				case CLASS_ROGUE:
					/* todo maybe: might be medium target if not crit-hitter/intercepter?
					   but only SMART monsters could possibly recognise the player's skills,
					   so just use own AC and HP for now, to determine our confidence: */
					if (m_ptr->ac >= 120 + rand_int(20) || m_ptr->hp >= 4000 + rand_int(2000)) {
						target_toughness = 1;
						break;
					}
					target_toughness = 2;//wowie
					break;

				case CLASS_WARRIOR:
#ifdef ENABLE_DEATHKNIGHT
				case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
				case CLASS_HELLKNIGHT:
#endif
				case CLASS_PALADIN:
				case CLASS_MIMIC:
					target_toughness = 2;
					break;

				case CLASS_DRUID: /* rather tough? */
				case CLASS_MINDCRAFTER: /* rather tough? */
					target_toughness = 2;
					break;

				case CLASS_ADVENTURER: /* todo maybe: depends on mimic form */
					if ((!pd_ptr->body_monster || pd_ptr->form_hp_ratio < 125)
					    && pd_ptr->ac + pd_ptr->to_a < p_tough_ac[pd_ptr->lev > 50 ? 50 : pd_ptr->lev - 1]) {
						target_toughness = 0;
						break;
					}
					target_toughness = 1;
					break;

				case CLASS_RANGER:
				case CLASS_ARCHER:
					target_toughness = 1;
					break;

				case CLASS_SHAMAN: /* todo maybe: depends on mimic form */
					if ((pd_ptr->body_monster && pd_ptr->form_hp_ratio >= 125)
					    || pd_ptr->ac + pd_ptr->to_a >= p_tough_ac[pd_ptr->lev > 50 ? 50 : pd_ptr->lev - 1]) {
						target_toughness = 1;
						break;
					}
					target_toughness = 0;
					break;

				case CLASS_RUNEMASTER: /* medium or light? */
					target_toughness = 0;
					break;

				case CLASS_MAGE:
					/* the odds are turning? >_> */
					if (pd_ptr->tim_manashield) {
						target_toughness = 1;//might even be 2?
						break;
					}
					target_toughness = 0;
					break;

				case CLASS_PRIEST:
					/* todo maybe: actually, priests can be skilled to make medium targets?
					   --recognise anti-evil aura: */
					if (get_skill(pd_ptr, SKILL_HDEFENSE) >= 30 && (r_ptr->flags3 & RF3_UNDEAD) && pd_ptr->lev * 2 >= r_ptr->level)
						target_toughness = 1;
					else if (get_skill(pd_ptr, SKILL_HDEFENSE) >= 40 && (r_ptr->flags3 & RF3_DEMON) && pd_ptr->lev * 3 >= r_ptr->level * 2)
						target_toughness = 1;
					else if (get_skill(pd_ptr, SKILL_HDEFENSE) >= 50 && (r_ptr->flags3 & RF3_EVIL) && pd_ptr->lev * 3 >= r_ptr->level * 2)
						target_toughness = 1;
					else
						target_toughness = 0;
					break;
#ifdef ENABLE_CPRIEST
				case CLASS_CPRIEST:
					target_toughness = 0; //we're definitely not a healer, but still zero probably
					break;
#endif
				}

				/* Note: Leaving out 'protection from evil' scroll/prayer for now, since that one is primarily effective
				   at lower levels, and there it's well-deserved fun to see baddies getting repelled from you ^^ */

				/* if a player is low on hit points, consider him weaker */
				if (target_toughness && (pd_ptr->chp * 10) / pd_ptr->mhp <= 4) target_toughness--;

				switch (target_toughness) {
				case 2:
					strong_targets++;
					//p_idx_strong[strong_targets] = -cd_ptr->m_idx;
					break;
				case 1:
					medium_targets++;
					p_idx_medium[medium_targets] = -cd_ptr->m_idx;
					break;
				default:
					weak_targets++;
					p_idx_weak[weak_targets] = -cd_ptr->m_idx;
					break;
				}
			}

			/* remember lowest and highest victim level */
			if (pd_ptr->lev < lowest_target_level) lowest_target_level = pd_ptr->lev;
			if (pd_ptr->lev > highest_target_level) highest_target_level = pd_ptr->lev;
		}

 #ifdef C_BLUE_AI_MELEE_NASTY
		/* if we have multiple targets and our currently picked one isn't ideal, reevaluate sometimes */
		if ((r_ptr->flags2 & RF2_SMART) && !rand_int(5))
			keeping_previous_target = FALSE;
 #endif

//s_printf("targets=%d\n", targets);
		if (targets) { /* Note: If an admin is in the path of a monster, this may happen to be 0 */

			/* *** Rules to choose between allowed targets: ***
			   Animals attack the one that "seems" most wounded: (hp_max / hp_cur) ?
			    maybe not good. except for wolves or such :).
			    attack randomly for now.
			   Low undead who have empty mind/Nonliving/weirdmind/emptymind attack randomly.
			   Really "weakest" target would be based on
			    HP/AC/parry/deflect/dodge/manashield/level/class, too complicated.
			   SMART monsters attack based on class and level (can have manashield at level x?) :).
			    At high levels this matters less, then just slightly prefer lowest level player.
			   STUPID monsters attack randomly.
			   Angels attack randomly (they don't swing like "hey you're weakest, I'm gonna kill you first!").
			   POWERFUL or deemed powerful (ancient dragons, dragonriders) or "proud" monsters attack randomly.
			   All other monsters -including minded undead- attack based on class during low levels, and random at high levels.
			*/
			reason = 1;

			if (r_ptr->flags7 & RF7_NO_DEATH) reason = 0;
			if (r_ptr->flags7 & RF7_MULTIPLY) reason = 0;
			if (r_ptr->flags3 & RF3_NONLIVING) reason = 0;
			if (r_ptr->flags3 & RF3_GOOD) reason = 0;
			if (r_ptr->flags3 & RF3_GIANT) reason = 0;
			if (r_ptr->flags3 & RF3_ANIMAL) reason = 0;
			if (r_ptr->flags2 & RF2_STUPID) reason = 0;
			if (r_ptr->flags2 & RF2_WEIRD_MIND) reason = 0;

			if (r_ptr->flags2 & RF2_POWERFUL) reason = 0; /* doesn't care ;) */
			if (strchr("AD", r_ptr->d_char)) reason = 0;
			if (r_ptr->flags2 & RF2_SMART) reason = 2;
			if (r_ptr->flags2 & RF2_EMPTY_MIND) reason = -1;

 #ifndef STUPID_MONSTER
			/* generate behaviour based on selected rule */
			switch (reason) {
			case -1: /* random & ignore distraction attempts */
				p_idx_chosen = p_idx[randint(targets)];
				break;
			case 0: /* random */
				if (non_distracting_targets && magik(90)) p_idx_chosen = p_idx_non_distracting[randint(non_distracting_targets)];
				else p_idx_chosen = p_idx[randint(targets)];
				break;
			case 1: /* at low level choose weaker classes first */
				if ((lowest_target_level < 30)
				    && magik(90)) {
					if (weak_targets && magik(90)) p_idx_chosen = p_idx_weak[randint(weak_targets)];
					else if (medium_targets && magik(90)) p_idx_chosen = p_idx_medium[randint(medium_targets)];
					else if (non_distracting_targets && magik(90)) p_idx_chosen = p_idx_non_distracting[randint(non_distracting_targets)];
					else p_idx_chosen = p_idx[randint(targets)];
				} else if (non_distracting_targets &&
				    magik(90))
					p_idx_chosen = p_idx_non_distracting[randint(non_distracting_targets)];
				else p_idx_chosen = p_idx[randint(targets)];
				break;
			case 2: /* like (1), but more sophisticated */
				if ((highest_target_level < 30 && lowest_target_level + 5 > highest_target_level) &&
				    magik(90)) {
					if (weak_targets && magik(90)) p_idx_chosen = p_idx_weak[randint(weak_targets)];
					else if (medium_targets && magik(90)) p_idx_chosen = p_idx_medium[randint(medium_targets)];
					else if (non_distracting_targets && magik(90)) p_idx_chosen = p_idx_non_distracting[randint(non_distracting_targets)];
					else p_idx_chosen = p_idx[randint(targets)];
				} else if (highest_target_level > lowest_target_level + lowest_target_level / 10) {
					if (magik(90)) {
						/* check which targets have a significantly lower level than others */
						for (i = 1; i <= targets; i++) {
							if (Players[p_idx[i]]->lev + Players[p_idx[i]]->lev / 10 < highest_target_level) {
								low_targets++;
								p_idx_low[low_targets] = p_idx[i];
							}
						}
						/* choose one of the lowbies of the player pack */
						p_idx_chosen = p_idx_low[randint(low_targets)];
					} else p_idx_chosen = p_idx[randint(targets)];
				} else {
					p_idx_chosen = p_idx[randint(targets)];
				}
				break;
			}
 #else
			/* not sure whether STUPID_MONSTERS should really keep distracting possible */
			if (non_distracting_targets && magik(90)) p_idx_chosen = p_idx_non_distracting[randint(non_distracting_targets)];
			else p_idx_chosen = p_idx[randint(targets)];
 #endif
			/* Remember this target, so we won't switch to a different one
			   in case this player has a levelup or something during our fighting,
			   depending on our monster's way to choose a target (see above).
			   This could be made dependent on further monster behaviour! :) */
			if (keeping_previous_target) p_idx_chosen = m_ptr->last_target_melee;
			/* note: storing id would be cleaner than idx, but it doesn't really make a practical difference */
			else m_ptr->last_target_melee = p_idx_chosen;
			
			/* connect to outside world */
			p_idx_target = p_idx_chosen;
		}
	} else
#endif /* C_BLUE_AI_MELEE */

	/* Don't attack your master! Invincible players can't be atacked! */
	if (!p_idx_target || m_ptr->owner == pd_ptr->id || pd_ptr->admin_invinc
	    /* non-hostile questors dont attack people */
	    || pfriend
 #ifdef TELEPORT_SURPRISES
	    || TELEPORT_SURPRISED(pd_ptr, r_ptr)
 #endif
	    )
		return NULL;

	return (p_idx_target ? Players[p_idx_target] : NULL);
}

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
/* TODO: revise FEAT_*, or strange intrusions can happen */
static void process_monster(int Ind, int m_idx, bool force_random_movement) {
	player_type	*p_ptr = Players[Ind];
	struct worldpos *wpos = &p_ptr->wpos;
	cave_type	**zcave;

	monster_type	*m_ptr = &m_list[m_idx];
	monster_race	*r_ptr = race_inf(m_ptr);
	monster_race	*base_r_ptr = &r_info[m_ptr->r_idx];

	int		i, d, oy, ox, ny, nx;
#ifdef ARCADE_SERVER
	int n;
#endif

	int		mm[8];
#ifdef ANTI_SEVEN_EXPLOIT
	int		mm2[8];
#endif
	bool		random_move = FALSE;

	cave_type    	*c_ptr;
	object_type 	*o_ptr;
	monster_type	*y_ptr;

	bool		do_turn;
	bool		do_move;
	bool		do_view;
	bool		do_melee;

	bool		did_open_door;
	bool		did_bash_door;
	bool		take_item_override = FALSE;
#ifdef OLD_MONSTER_LORE
	bool		did_take_item;
	bool		did_kill_item;
	bool		did_move_body;
	bool		did_kill_body;
	bool		did_pass_wall;
	bool		did_kill_wall;
#endif
	bool		inv;

	/* for questors: pfriend doesn't attack players/pets/golems, mfriend doesn't attack monsters */
	bool		pfriend = (m_ptr->questor && (m_ptr->questor_hostile & 0x1) == 0);
	bool		mfriend = !m_ptr->questor || (m_ptr->questor_hostile & 0x2) == 0;


/* Hack -- don't process monsters on wilderness levels that have not
	   been regenerated yet.
	*/
	if (!(zcave = getcave(wpos))) return;

	/* If the monster can't see the player */
	inv = player_invis(Ind, m_ptr, m_ptr->cdis);

#ifdef SAURON_ANTI_GLYPH
	if (m_ptr->r_idx == RI_SAURON && m_ptr->extra) m_ptr->extra--;
#endif

	/* Handle "sleep" */
	if (m_ptr->csleep) {
		u32b notice = 0;
		bool aggravated = FALSE;

		/* Hack -- handle non-aggravation */
#if 0
		if (!p_ptr->aggravate) notice = rand_int(1024);
#else
		/* check everyone on the floor */
		notice = rand_int(1024);
		for (i = 1; i <= NumPlayers; i++) {
			player_type *q_ptr = Players[i];

			/* Skip disconnected players */
			if (q_ptr->conn == NOT_CONNECTED) continue;

			if (!q_ptr->aggravate) continue;

			/* Skip players not on this depth */
			if (!inarea(&q_ptr->wpos, wpos)) continue;

			/* Compute distance */
			/* XXX value is same with that in process_monsters */
#ifndef REDUCED_AGGRAVATION
			if (distance(m_ptr->fy, m_ptr->fx, q_ptr->py, q_ptr->px) >= 100)
#else
			/* Aggravation is not 'infecting' other players on the map */
			if (Ind != i) continue;
			if (distance(m_ptr->fy, m_ptr->fx, q_ptr->py, q_ptr->px) >= 50)
#endif
				continue;

			notice = 0;
			aggravated = TRUE;
			break;
		}
#endif	// 0

		/* Use the closest player (calculated in process_monsters()) */
		p_ptr = Players[m_ptr->closest_player];

		/* Calculate the "player noise" */
		u32b noise = (1U << (30 - p_ptr->skill_stl));

		/* Hack -- See if monster "notices" player */
		if ((notice * notice * notice) <= noise || aggravated) {
			/* Hack -- amount of "waking" */
			int d = 1;

			/* Hack -- make sure the distance isn't zero */
			if (m_ptr->cdis == 0) m_ptr->cdis = 1;

			/* Wake up faster near the player */
			if (m_ptr->cdis < 50) d = (100 / m_ptr->cdis);

			/* Hack -- handle aggravation */
//			if (p_ptr->aggravate) d = m_ptr->csleep;
			if (aggravated) d = m_ptr->csleep;

			/* Still asleep */
			if (m_ptr->csleep > d) {
				/* Monster wakes up "a little bit" */
				m_ptr->csleep -= d;

#ifdef OLD_MONSTER_LORE
				/* Notice the "not waking up" */
				if (p_ptr->mon_vis[m_idx]) {
					/* Hack -- Count the ignores */
					r_ptr->r_ignore++;
				}
#endif
			}
			/* Just woke up */
			else {
				/* Reset sleep counter */
				m_ptr->csleep = 0;

				/* Notice the "waking up" */
				msg_print_near_monster(m_idx, "wakes up.");

#if 0
				if (p_ptr->mon_vis[m_idx]) {
					char m_name[MNAME_LEN];
					monster_desc(Ind, m_name, m_idx, 0);
					msg_format(Ind, "%^s wakes up.", m_name);

					/* Hack -- Count the wakings */
					/* not used at all, seemingly */
					r_ptr->r_wake++;
				}
#endif	// 0
			}
		}

		/* Still sleeping */
		if (m_ptr->csleep) {
			m_ptr->energy -= level_speed(&m_ptr->wpos);
			return;
		}
	}
	/* Hack: Handle hard-coded fluff specialties (get serious if
	   monster is actually in combat, in most cases) - C. Blue */
	else if (m_ptr->hp >= (m_ptr->maxhp * 7) / 10) {
		/* Ufthak of Cirith Ungol is mortally afraid of spiders */
		if (m_ptr->r_idx == RI_UFTHAK &&
		    (r_info[p_ptr->body_monster].flags7 & RF7_SPIDER) &&
		    m_ptr->monfear < 20 && !m_ptr->monfear_gone)
			m_ptr->monfear = 20; /* he recovers at 2 per turn */
	}


	/* Handle "stun" */
	if (m_ptr->stunned) {
		int d = 1;

		/* Make a "saving throw" against stun */
		/* if (rand_int(5000) <= r_ptr->level * r_ptr->level) */
		/* the_sandman: Blegh; lvl 71+ monsters will recover immediately.*/
		if (rand_int(200) <= r_ptr->level) { /* needs testing! */
			/* Recover fully */
			d = m_ptr->stunned;
		}

		/* Hack -- Recover from stun */
		if (m_ptr->stunned > d) {
			/* Recover somewhat */
			m_ptr->stunned -= d;
		}
		/* Fully recover */
		else {
			/* Recover fully */
			m_ptr->stunned = 0;

			/* Message if visible */
			msg_print_near_monster(m_idx, "is no longer stunned.");
#if 0
			if (p_ptr->mon_vis[m_idx]) {
				char m_name[MNAME_LEN];
				monster_desc(Ind, m_name, m_idx, 0);
				msg_format(Ind, "\377o%^s is no longer stunned.", m_name);
			}
#endif	// 0
		}

		/* Still stunned */
		if (m_ptr->stunned > 100) {
			m_ptr->energy -= level_speed(&m_ptr->wpos);
			return;
		}
	}


	/* Handle confusion */
	if (m_ptr->confused) {
		/* Amount of "boldness" */
		int d = randint(r_ptr->level / 10 + 1);

		/* Still confused */
		if (m_ptr->confused > d) {
			/* Reduce the confusion */
			m_ptr->confused -= d;
		}
		/* Recovered */
		else {
			/* No longer confused */
			m_ptr->confused = 0;

			/* Message if visible */
			msg_print_near_monster(m_idx, "is no longer confused.");
#if 0
			if (p_ptr->mon_vis[m_idx]) {
				char m_name[MNAME_LEN];
				monster_desc(Ind, m_name, m_idx, 0);
				msg_format(Ind, "%^s is no longer confused.", m_name);
			}
#endif	// 0
		}
	}


	/* Handle "fear" */
	if (m_ptr->monfear
#if 1 /* experimental: don't recover from fear until healed up somewhat? except if we're usually attacking from afar anyway */
	    && ((m_ptr->hp * 100) / m_ptr->maxhp >= 20 + rand_int(11) || (r_ptr->flags7 & RF7_AI_ANNOY))
#endif
	    ) {
		/* Amount of "boldness" */
		int d = randint(r_ptr->level / 10 + 1);

		/* Still afraid */
		if (m_ptr->monfear > d) {
			/* Reduce the fear */
			m_ptr->monfear -= d;
		}
		/* Recover from fear, take note if seen */
		else {
			/* No longer afraid */
			m_ptr->monfear = 0;

			/* Visual note */
			msg_print_near_monster(m_idx, "becomes courageous again.");
#if 0
			if (p_ptr->mon_vis[m_idx]) {
				char m_name[MNAME_LEN];
				char m_poss[MNAME_LEN];

				/* Acquire the monster name/poss */
				monster_desc(Ind, m_name, m_idx, 0);
				monster_desc(Ind, m_poss, m_idx, 0x22);

				/* Dump a message */
				msg_format(Ind, "%^s recovers %s courage.", m_name, m_poss);
			}
#endif	// 0
		}
	}

	/* Handle "taunted" */
	if (m_ptr->taunted) m_ptr->taunted--;

	/* Handle "silenced" */
	if (m_ptr->silenced > 0) m_ptr->silenced--;
	else if (m_ptr->silenced < 0) m_ptr->silenced++;

	/* Handle "stopped" */
	if (m_ptr->no_move) {
		m_ptr->no_move--;
		//m_ptr->monfear = 0; }
		if (!m_ptr->no_move) msg_print_near_monster(m_idx, "is no longer frozen to the ground.");
	}

	/* Get the origin */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

#if 0	// too bad hack!
	/* Hack -- aquatic life outa water */
	if (zcave[oy][ox].feat != FEAT_WATER) {
		if (r_ptr->flags7 & RF7_AQUATIC) {
			m_ptr->monfear = 50;
			m_ptr->monfear_gone = 0;
		}
	} else {
		if (!(r_ptr->flags3 & RF3_UNDEAD) &&
		    !(r_ptr->flags7 & (RF7_AQUATIC | RF7_CAN_SWIM | RF7_CAN_FLY) )) {
			m_ptr->monfear = 50;
			m_ptr->monfear_gone = 0;
		}
	}
#endif	// 0


	/* attempt to "mutiply" if able and allowed */

	//if((r_ptr->flags7 & RF7_MULTIPLY) &&
	//Let's not allow mobs that can fire missiles to multiply..
	if((r_ptr->flags7 & RF7_MULTIPLY) && (!(r_ptr->flags5 & RF5_MISSILE)) &&
	    (!istown(wpos) && (m_ptr->wpos.wz != 0 ||
	     wild_info[m_ptr->wpos.wy][m_ptr->wpos.wx].radius >= MAX_TOWNAREA) ) &&
	    (num_repro < MAX_REPRO))
#if REPRO_RATE
		if (magik(REPRO_RATE))
#endif	// REPRO_RATE
		{
			int k, y, x;
			dun_level *l_ptr = getfloor(wpos);

			if (!(l_ptr && l_ptr->flags1 & LF1_NO_MULTIPLY) ||
			    (r_ptr->flags3 & RF3_NONLIVING)) /* vermin control has no effect on non-living creatures */
			{
				/* Count the adjacent monsters */
				for (k = 0, y = oy - 1; y <= oy + 1; y++) {
					for (x = ox - 1; x <= ox + 1; x++) {
						if (zcave[y][x].m_idx > 0) k++;
					}
				}

				/* Hack -- multiply slower in crowded areas */
				if ((k < 4) && (!k || !rand_int(k * MON_MULT_ADJ))) {
					/* Try to multiply */
					if (multiply_monster(m_idx)) {
						/* Take note if visible */
						if (p_ptr->mon_vis[m_idx]) r_ptr->flags7 |= RF7_MULTIPLY;

						/* Multiplying takes energy */
						m_ptr->energy -= level_speed(&m_ptr->wpos);

						return;
					}
				}
			}
		}


	/* Set AI state */
	m_ptr->ai_state = 0;
	c_ptr = &zcave[oy][ox];

	/* Non-stupid monsters only */
#if 0
	if (!(r_ptr->flags2 & RF2_STUPID)) {
		/* Evil player tainted the grid? */
		if (c_ptr->effect) {
			if (!monster_is_safe(m_idx, m_ptr, r_ptr, c_ptr))
				m_ptr->ai_state |= AI_STATE_EFFECT;
		}
	}
#else
	if (!monster_is_safe(m_idx, m_ptr, r_ptr, c_ptr))
		m_ptr->ai_state |= AI_STATE_EFFECT;
#endif

	/* All the monsters */
	/* You cannot breathe? */
	//if (!monster_is_comfortable(r_ptr, c_ptr))
	if (!monster_can_cross_terrain(c_ptr->feat, r_ptr, FALSE, c_ptr->info))
		m_ptr->ai_state |= AI_STATE_TERRAIN;


	/* Attempt to cast a spell */
	if (!inv && !force_random_movement && make_attack_spell(Ind, m_idx)) {
		m_ptr->energy -= level_speed(&m_ptr->wpos);
		return;
	}


	/* Hack -- Assume no movement */
	mm[0] = mm[1] = mm[2] = mm[3] = 0;
	mm[4] = mm[5] = mm[6] = mm[7] = 0;


	/* Confused -- 100% random */
	if (m_ptr->confused || force_random_movement || (r_ptr->flags5 & RF5_RAND_100)) {
		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
		random_move = TRUE;
	}

	/* 75% random movement */
	else if (((r_ptr->flags1 & RF1_RAND_50) &&
	    (r_ptr->flags1 & RF1_RAND_25) &&
	    (rand_int(100) < 75)) || inv) {
#ifdef OLD_MONSTER_LORE
		/* Memorize flags */
		if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags1 |= RF1_RAND_50;
		if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags1 |= RF1_RAND_25;
#endif

		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
		random_move = TRUE;
	}

	/* 50% random movement */
	else if ((r_ptr->flags1 & RF1_RAND_50) &&
	    (rand_int(100) < 50)) {
#ifdef OLD_MONSTER_LORE
		/* Memorize flags */
		if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags1 |= RF1_RAND_50;
#endif

		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
		random_move = TRUE;
	}

	/* 25% random movement */
	else if ((r_ptr->flags1 & RF1_RAND_25) &&
	    (rand_int(100) < 25)) {
#ifdef OLD_MONSTER_LORE
		/* Memorize flags */
		if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags1 |= RF1_RAND_25;
#endif

		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
		random_move = TRUE;
	}

	/* 5% random movement */
	else if ((r_ptr->flags0 & RF0_RAND_5) &&
	    (rand_int(100) < 5)) {
#ifdef OLD_MONSTER_LORE
		/* Memorize flags */
		if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags0 |= RF0_RAND_5;
#endif

		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
		random_move = TRUE;
	}

	/* Normal movement */
	else {
		/* Logical moves */
#ifdef ARCADE_SERVER
		if ((m_ptr->r_idx >= RI_ARCADE_START) && (m_ptr->r_idx <= RI_ARCADE_END)) {
			for(n = 1; n <= NumPlayers; n++) {
				player_type *p_ptr = Players[n];
				if(p_ptr->game == 4 && p_ptr->team == 5) {
					get_moves_arc(p_ptr->arc_b, p_ptr->arc_a, m_idx, mm);
					n = NumPlayers + 1;
				}
			}
		} else
#endif
		if (m_ptr->questor) get_moves_questor(Ind, m_idx, mm); /* note: we're not using a 'process_monster_questor()' for now */
		else if (get_moves(Ind, m_idx, mm)) {
			/* TRUE result can only come from get_moves_astar() and means we decided to use a movement spell.
			   In that case we're done now. */
			m_ptr->energy -= level_speed(&m_ptr->wpos);
			return;
		}
	}

#ifdef ANTI_SEVEN_EXPLOIT /* code part: 'monster has planned an actual movement' */
	if (!random_move) {
		mm2[0] = 0; /* paranoia: pre-terminate array */
		if (m_ptr->previous_direction) {
			/* if monster has clean LoS to player, cancel anti-exploit and resume normal behaviour */
			if (projectable_wall(&p_ptr->wpos, m_ptr->fy, m_ptr->fx, p_ptr->py, p_ptr->px, MAX_RANGE))
				m_ptr->previous_direction = 0;
 #ifdef ANTI_SEVEN_EXPLOIT_VAR2
			/* hack to prevent slightly silyl ping-pong movement when next to epicenter,
			although probably harmless:
			If monster has LOS to where the player once fired from, terminate algorithm. */
			if (projectable_wall(&p_ptr->wpos, m_ptr->fy, m_ptr->fx, m_ptr->p_ty, m_ptr->p_tx, MAX_RANGE))
				m_ptr->previous_direction = 0;
 #endif
		}
// #if 1 /* enable? */
		if (m_ptr->previous_direction == -1) {
			/* would none regularly planned move _not decrease_ distance to target _player_ ? */
			/* ok this gets not checked either this time, not that important probably =p
			if (foo..distance decrease stuff..) */
			{
				/* allow only directions that _decrease_ (not: not increase) distance to _epicentrum_ */
				d = 0;
				for (i = 1; i <= 9; i++) {
					/* direction must decrease distance (might need tweaking!) */
					if (((m_ptr->damage_ty - oy) * ddy[i] <= 0) &&
					    ((m_ptr->damage_tx - ox) * ddx[i] <= 0))
						continue;
 #ifdef ANTI_SEVEN_EXPLOIT_VAR1
					/* tweaked: prevent (probably harmless) ping-pong'ing when adjacent to destination */
					if (distance(oy + ddy[i], ox + ddx[i], m_ptr->damage_ty, m_ptr->damage_tx) > m_ptr->damage_dis)
						continue;
 #endif
					mm2[d] = i;
					d++;
				}
				/* zero-terminate movement array */
				mm2[d] = 0;
			}
		} else if (m_ptr->previous_direction > 0) {
			/* are we already standing on epicentrum x,y ? */
			if (ox == m_ptr->damage_tx && oy == m_ptr->damage_ty) {
				/* little hack (optional I guess): avoid going back into exactly the direction we just came from, just because we can :-s */
				/* BEGIN optional hack.. */
				d = 0;
				for (i = 1; i <= 9; i++) {
					/* avoid retreating into the exact direction we came from */
					if (i == 10 - m_ptr->previous_direction) continue;
					/* direction must decrease distance */
					if (((p_ptr->py - oy) * ddy[i] <= 0) &&
					    ((p_ptr->px - ox) * ddx[i] <= 0))
						continue;
					mm2[d] = i;
					d++;
				}
				/* zero-terminate movement array */
				mm2[d] = 0;
				/* ..END optional hack. */
				/* reset direction to signal end of our special behaviour. */
				m_ptr->previous_direction = 0;
			} else { /* continue overriding movement so we get closer to epicentrum */
				/* allow only directions that _decrease_ (not: not increase) distance to _epicentrum_ */
				d = 0;
				for (i = 1; i <= 9; i++) {
					/* direction must decrease distance (might need tweaking!) */
					if (((m_ptr->damage_ty - oy) * ddy[i] <= 0) &&
					    ((m_ptr->damage_tx - ox) * ddx[i] <= 0))
						continue;
					/* latest fix, for 2-space-passages!
					   avoid retreating into the exact direction we came from */
					if (i == 10 - m_ptr->previous_direction) continue;
 #ifdef ANTI_SEVEN_EXPLOIT_VAR1
					/* tweaked: prevent (probably harmless) ping-pong'ing when adjacent to destination */
					if (distance(oy + ddy[i], ox + ddx[i], m_ptr->damage_ty, m_ptr->damage_tx) > m_ptr->damage_dis)
						continue;
 #endif
					mm2[d] = i;
					d++;
				}
				/* zero-terminate movement array */
				mm2[d] = 0;
			}
		}
// #endif /* enabled? */
		/* paranoia probably, but might fix odd behaviour (possibly observed
		   'temporarily getting stuck' in monster arena) in special levels
		   where FF1_PROTECTED grids occur. well, doesn't cost much anyway:
		   instead of overwriting mm[] directly, we used mm2[] and now check if we
		   have at least 1 direction, otherwise cancel the anti-exploit! */
		if (mm2[0]) for(i = 0; i < 8; i++) mm[i] = mm2[i];
		else m_ptr->previous_direction = 0;
	}
#endif

	/* Assume nothing */
	do_turn = FALSE;
	do_move = FALSE;
	do_view = FALSE;
	/* for AI_ANNOY only: fall back to attacking if out of moves */
	do_melee = ((r_ptr->flags7 & RF7_AI_ANNOY) && !(r_ptr->flags1 & RF1_NEVER_BLOW));

	/* Assume nothing */
	did_open_door = FALSE;
	did_bash_door = FALSE;
#ifdef OLD_MONSTER_LORE
	did_take_item = FALSE;
	did_kill_item = FALSE;
	did_move_body = FALSE;
	did_kill_body = FALSE;
	did_pass_wall = FALSE;
	did_kill_wall = FALSE;
#endif

	/* Take a zero-terminated array of "directions" */
	for (i = 0; mm[i]; i++) {
		/* Get the direction */
		d = mm[i];

		/* Hack -- allow "randomized" motion */
		if (d == 5) d = ddd[rand_int(8)];

		/* Get the destination */
		ny = oy + ddy[d];
		nx = ox + ddx[d];

		/* panic saves during halloween, adding this for now */
//let's not suppress symptoms for now, so we find the root		if (!in_bounds(ny, nx)) continue;

		/* Access that cave grid */
		c_ptr = &zcave[ny][nx];

		/* Access that cave grid's contents */
		o_ptr = &o_list[c_ptr->o_idx];

		/* Access that cave grid's contents */
		y_ptr = &m_list[c_ptr->m_idx];

		/* Tavern entrance? */
//		if (c_ptr->feat == FEAT_SHOP_TAIL - 1)
		/* Shops only protect on world surface, dungeon shops are dangerous!
		   (optional alternative: make player unable to attack while in shop in turn) */
		if (c_ptr->feat == FEAT_SHOP) {
			/* attack player in dungeon store */
			if (wpos->wz && c_ptr->m_idx < 0) do_move = TRUE;
			/* else nothing */
		}

		/* "protected" grid without player on it => cannot enter!  - C. Blue */
		else if ((f_info[c_ptr->feat].flags1 & FF1_PROTECTED) &&
		    (c_ptr->m_idx >= 0)) {
			/* nothing */
		}

		/* walk around invincible admins instead of trying to attack them */
		else if (c_ptr->m_idx < 0 && Players[-c_ptr->m_idx]->admin_invinc) ;

/* Isn't this whole 'Tainted grid' stuff OBSOLETE?
   Because: If monster_is_safe is false, it will always have set AI_STATE_EFFECT. */
		/* Tainted grid? */
#if 0 /* testing new hack below */
		else if (!(m_ptr->ai_state & AI_STATE_EFFECT) &&
		    !monster_is_safe(m_idx, m_ptr, r_ptr, c_ptr)) {
			/* Nothing */
		}
#else /* attack anyway if player is next to us! */
		else if (!(m_ptr->ai_state & AI_STATE_EFFECT) &&
		    !monster_is_safe(m_idx, m_ptr, r_ptr, c_ptr)
		    && !(c_ptr->m_idx < 0)) {
			/* Nothing */
		}
#endif

		else if (creature_can_enter(r_ptr, c_ptr)) {
			do_move = TRUE;

			/* if we can pass without wall-killing, yet are wall-killers, then we should kill obstacles if we can! */
			if (((r_ptr->flags2 & RF2_KILL_WALL) ||
			    ((base_r_ptr->flags2 & RF2_POWERFUL) &&
			    (c_ptr->feat == FEAT_DEAD_TREE ||
			    c_ptr->feat == FEAT_TREE ||
			    c_ptr->feat == FEAT_BUSH)))
			    /* only if the feat is legal to remove (ie wallish) */
			    && cave_dig_wall_grid(c_ptr)) {
#if 0 /* if ever used, gotta fix: only if RF2_KILL_WALL !*/
				did_kill_wall = TRUE;
#endif

				/* Forget the "field mark", if any */
				everyone_forget_spot(wpos, ny, nx);

				cave_set_feat_live(wpos, ny, nx, twall_erosion(wpos, ny, nx));

				/* Note changes to viewable region */
				if (player_has_los_bold(Ind, ny, nx)) do_view = TRUE;
			}
		}

		/* Player ghost in wall or on FF1_PROTECTED grid: Monster may attack in melee anyway */
		else if (c_ptr->m_idx < 0) {
			/* Move into player */
			do_move = TRUE;
		}

		/* Let monsters pass permanent but passable walls if they have PASS_WALL! */
		else if ((f_info[c_ptr->feat].flags1 & FF1_PERMANENT) &&
		    (f_info[c_ptr->feat].flags1 & FF1_CAN_PASS) &&
		    (r_ptr->flags2 & RF2_PASS_WALL)) {
			/* Pass through walls/doors/rubble */
			do_move = TRUE;

#ifdef OLD_MONSTER_LORE
			/* Monster went through a wall */
			did_pass_wall = TRUE;
#endif
		}

		/* Permanent wall */
		/* Hack: Morgy DIGS!! */
//		else if ( (c_ptr->feat >= FEAT_PERM_EXTRA &&
		else if (((f_info[c_ptr->feat].flags1 & FF1_PERMANENT) &&
		    !((r_ptr->flags2 & RF2_KILL_WALL) &&
		     (r_ptr->flags2 & RF2_PASS_WALL) &&
		     !rand_int(100)))
		    || (f_info[c_ptr->feat].flags2 & FF2_BOUNDARY)
		    || (c_ptr->feat == FEAT_PERM_CLEAR)
		    || (c_ptr->feat == FEAT_HOME)
		    || (c_ptr->feat == FEAT_WALL_HOUSE))
		{
			/* Nothing */
			/* Paranoia: Keep do_melee enabled, so Morgoth would in theory (ie if he had
			   AI_ANNOY, which he of course doesn't have) be allowed to make TWO actions
			   per move: 1) try digging and 2) actually attack, if digging failed. */
		}

		/* Monster destroys walls (and doors) */
		/* Note: Since do_melee isn't false'd if rand(digging) fails, monsters could still attack
		   despite having had a go at digging (slightly cheating since that'd be 2 actions a turn). */
#ifdef MONSTER_DIG_FACTOR
		/* EVILEYE - correct me if i interpreted this wrongly. */
		/* You're right, mine was wrong - Jir - */
		else if (r_ptr->flags2 & RF2_KILL_WALL ||
		    (!(r_ptr->flags1 & RF1_NEVER_MOVE ||
		    r_ptr->flags2 & RF2_EMPTY_MIND ||
		   r_ptr->flags2 & RF2_STUPID) &&
		    (!rand_int(digging_difficulty(c_ptr->feat) * MONSTER_DIG_FACTOR)
		     && magik(5 + r_ptr->level))))
#else
		else if ((r_ptr->flags2 & RF2_KILL_WALL) ||
		    /* POWERFUL monsters can hack down trees */
		    ((base_r_ptr->flags2 & RF2_POWERFUL) &&
		    //c_ptr->feat == FEAT_TREE || c_ptr->feat == FEAT_EVIL_TREE ||
		    (c_ptr->feat == FEAT_DEAD_TREE || c_ptr->feat == FEAT_TREE ||
		    c_ptr->feat == FEAT_BUSH || c_ptr->feat == FEAT_IVY)))
#endif
		{
			/* Eat through walls/doors/rubble */
			do_move = TRUE;

#ifdef OLD_MONSTER_LORE
			/* Monster destroyed a wall */
#if 0 /* if ever used, gotta fix: only if RF2_KILL_WALL !*/
			did_kill_wall = TRUE;
#endif
#endif

			/* Forget the "field mark", if any */
			everyone_forget_spot(wpos, ny, nx);

			/* Create floor */
//			c_ptr->feat = FEAT_FLOOR;
			//c_ptr->feat = twall_erosion(wpos, ny, nx);
			cave_set_feat_live(wpos, ny, nx, twall_erosion(wpos, ny, nx));

			/* Note changes to viewable region */
			if (player_has_los_bold(Ind, ny, nx)) do_view = TRUE;
		}

		/* Monster moves through walls (and doors) */
//		else if (r_ptr->flags2 & RF2_PASS_WALL)
		else if ((f_info[c_ptr->feat].flags1 & FF1_CAN_PASS) && (r_ptr->flags2 & (RF2_PASS_WALL))) {
			/* Pass through walls/doors/rubble */
			do_move = TRUE;

#ifdef OLD_MONSTER_LORE
			/* Monster went through a wall */
			did_pass_wall = TRUE;
#endif
		}

		/* Handle doors and secret doors */
		else if (((c_ptr->feat >= FEAT_DOOR_HEAD) &&
			  (c_ptr->feat <= FEAT_DOOR_TAIL)) ||
			 (c_ptr->feat == FEAT_SECRET))
		{
			bool may_bash = TRUE;

			/* Creature can open doors. */
			if (r_ptr->flags2 & RF2_OPEN_DOOR) {
				/* Take a turn */
				do_turn = TRUE;

				/* Closed doors and secret doors */
				if ((c_ptr->feat == FEAT_DOOR_HEAD) ||
				    (c_ptr->feat == FEAT_SECRET))
				{
					/* The door is open */
					did_open_door = TRUE;
#ifdef USE_SOUND_2010
					sound_near_site(ny, nx, wpos, 0, "open_door", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
					/* Do not bash the door */
					may_bash = FALSE;
				}

				/* Locked doors (not jammed) */
				else if (c_ptr->feat < FEAT_DOOR_HEAD + 0x08) {
					int k;

					/* Door power */
					k = ((c_ptr->feat - FEAT_DOOR_HEAD) & 0x07);

#if 0
					/* XXX XXX XXX XXX Old test (pval 10 to 20) */
					if (randint((m_ptr->hp + 1) * (50 + o_ptr->pval)) <
					    40 * (m_ptr->hp - 10 - o_ptr->pval));
#endif

					/* Try to unlock it XXX XXX XXX */
					if (rand_int(m_ptr->hp / 10) > k) {
						/* Unlock the door */
						c_ptr->feat = FEAT_DOOR_HEAD + 0x00;
#ifdef USE_SOUND_2010
						sound_near_site(ny, nx, wpos, 0, "open_pick", NULL, SFX_TYPE_COMMAND, FALSE);
#endif
						/* Do not bash the door */
						may_bash = FALSE;
					}
				}
			}

			/* Stuck doors -- attempt to bash them down if allowed */
			if (may_bash && (r_ptr->flags2 & RF2_BASH_DOOR)) {
				int k;

				/* Take a turn */
				do_turn = TRUE;

				/* Door power */
				k = ((c_ptr->feat - FEAT_DOOR_HEAD) & 0x07);

#if 0
				/* XXX XXX XXX XXX Old test (pval 10 to 20) */
				if (randint((m_ptr->hp + 1) * (50 + o_ptr->pval)) <
				    40 * (m_ptr->hp - 10 - o_ptr->pval));
#endif

				/* Attempt to Bash XXX XXX XXX */
				if (rand_int(m_ptr->hp / 10) > k) {
					/* Message */
//					msg_print(Ind, "You hear a door burst open!");
					msg_print_near_site(ny, nx, wpos, 0, FALSE, "You hear a door burst open!");
#ifdef USE_SOUND_2010
					if (rand_int(3)) /* some variety, although not entirely correct :) */
						sound_near_site(ny, nx, wpos, 0, "bash_door_break", NULL, SFX_TYPE_COMMAND, FALSE);
					else
						sound_near_site(ny, nx, wpos, 0, "bash_door_hold", NULL, SFX_TYPE_COMMAND, FALSE);
#endif


					/* Disturb (sometimes) */
					if (p_ptr->disturb_minor) disturb(Ind, 0, 0);

					/* The door was bashed open */
					did_bash_door = TRUE;

					/* Hack -- fall into doorway */
					do_move = TRUE;
				}
			}


			/* Deal with doors in the way */
			if (did_open_door || did_bash_door) {
				/* Break down the door */
				if (did_bash_door && (rand_int(100) < 50))
					c_ptr->feat = FEAT_BROKEN;
				/* Open the door */
				else
					c_ptr->feat = FEAT_OPEN;

				/* Notice */
				note_spot_depth(wpos, ny, nx);

				/* Redraw */
				everyone_lite_spot(wpos, ny, nx);

				/* Handle viewable doors */
				if (player_has_los_bold(Ind, ny, nx)) do_view = TRUE;
			}

			/* Note: Since do_melee isn't false'd if rand(opening/bashing) fails, monsters could still attack
			   despite having had a go at opening/bashing (slightly cheating since that'd be 2 actions a turn). */
		}
		/* Floor is trapped? */
		else if (c_ptr->feat == FEAT_MON_TRAP) {
			/* Go ahead and move */
			do_move = TRUE;
		}



		/* Hack -- check for Glyph of Warding / Rune of Protection */
		if (do_move && ((c_ptr->feat == FEAT_GLYPH) || (c_ptr->feat == FEAT_RUNE)) &&
		    !((r_ptr->flags1 & RF1_NEVER_MOVE) && (r_ptr->flags1 & RF1_NEVER_BLOW))) {
			/* Assume no move allowed */
			do_move = FALSE;

			/* Break the ward - Michael (because he embodies the Glyph power sort of),
			   Morgoth and certain Nether Realm monsters may insta-break them. */
			if (randint(BREAK_GLYPH) < r_ptr->level || r_ptr->level >= 98) { // || r_ptr->level == 98 || r_ptr->level >= 100) {
				if (c_ptr->feat == FEAT_GLYPH) {
					/* Describe observable breakage */
					/* Prolly FIXME */
					msg_print_near_site(ny, nx, wpos, 0, TRUE, "The rune of protection is broken!");
					
					/* Break the rune */
					cave_set_feat_live(wpos, ny, nx, FEAT_FLOOR);
					
					/* Allow movement */
					do_move = TRUE;
				} else { //(c_ptr->feat == FEAT_RUNE)
					/* Describe observable breakage */
					/* Prolly FIXME */
					msg_print_near_site(ny, nx, wpos, 0, TRUE, "The glyph of warding is broken!");

					/* Do this after the monster moves */
					//if (warding_rune_break(m_idx)) return;
					
					/* Allow movement */
					do_move = TRUE;
				}
			}

			/* Note: Since do_melee isn't false'd if rand(digging) fails, monsters could still attack
			   despite having had a go at digging (slightly cheating since that'd be 2 actions a turn). */

#ifdef SAURON_ANTI_GLYPH
			/* Special power boost for Sauron if he gets hindered by glyphs */
			else if (m_ptr->r_idx == RI_SAURON && base_r_ptr->freq_innate != SAURON_SPELL_BOOST) {
				base_r_ptr->freq_spell = base_r_ptr->freq_innate = SAURON_SPELL_BOOST;
				s_printf("SAURON: boost (glyph move).\n");
			}
		} else if (do_move && m_ptr->r_idx == RI_SAURON &&
		    !m_ptr->extra && /* avoid oscillating too quickly from glyphs-prevent-summoning boost */
		    base_r_ptr->freq_innate != 50) {
			base_r_ptr->freq_spell = base_r_ptr->freq_innate = 50; /* hardcoded :| */
			s_printf("SAURON: normal.\n");
		}
#else
		}
#endif

		/* Some monsters never attack */
		if (do_move && (ny == p_ptr->py) && (nx == p_ptr->px) &&
		    (r_ptr->flags1 & RF1_NEVER_BLOW))
		{
#ifdef OLD_MONSTER_LORE
			/* Hack -- memorize lack of attacks */
			/* if (m_ptr->ml) r_ptr->r_flags1 |= RF1_NEVER_BLOW; */
#endif

			/* Do not move */
			do_move = FALSE;
		}


		/* The player is in the way.  Attack him. */
		if (do_move && (c_ptr->m_idx < 0)) {
			player_type *q_ptr = get_melee_target(r_ptr, m_ptr, c_ptr, pfriend);

			if (q_ptr && !q_ptr->admin_invinc) {
				/* Push past weaker players (unless leaving a wall) */
				if ((r_ptr->flags2 & RF2_MOVE_BODY) &&
//				    (cave_floor_bold(zcave, m_ptr->fy, m_ptr->fx)) &&
				    (cave_floor_bold(zcave, oy, ox)) &&
				    magik(10) && !q_ptr->martyr &&
				    (r_ptr->level > randint(q_ptr->lev * 20 + q_ptr->wt * 5)))
				{
					char m_name[MNAME_LEN];
					monster_desc(0, m_name, m_idx, 0);

					/* Allow movement */
					do_move = TRUE;

#ifdef OLD_MONSTER_LORE
					/* Monster pushed past the player */
					did_move_body = TRUE;
#endif

					/* Log this to be safe about MOVE_BODY vs TELE_TO related kills */
					s_printf("MOVE_BODY: '%s' got switched by '%s'.\n", q_ptr->name, m_name);
				} else {
					/* Do the attack */
					(void)make_attack_melee(q_ptr->Ind, m_idx);

					/* Took a turn */
					do_turn = TRUE;

					/* Do not move */
					do_move = FALSE;
				}
			} else {
				/* Do not move */
				do_move = FALSE;
			}
		}


		/* Some monsters never move */
		if (do_move && ((r_ptr->flags1 & RF1_NEVER_MOVE) || m_ptr->no_move)) {
#ifdef OLD_MONSTER_LORE
			/* Hack -- memorize lack of attacks */
			/* if (m_ptr->ml) r_ptr->r_flags1 |= RF1_NEVER_MOVE; */
#endif

			/* Do not move */
			do_move = FALSE;

			/* Hack -- use up the turn */
#ifdef Q_ENERGY_EXCEPTION
			if (r_ptr->d_char != 'Q')
#endif	// Q_ENERGY_EXCEPTION
				do_turn = TRUE;
		}

		/*
		 * Check if monster can cross terrain
		 * This is checked after the normal attacks
		 * to allow monsters to attack an enemy,
		 * even if it can't enter the terrain.
		 */
		if (do_move && !monster_can_cross_terrain(c_ptr->feat, r_ptr, FALSE, c_ptr->info)
//			&& monster_can_cross_terrain(zcave[oy][ox].feat, r_ptr, FALSE, zcave[oy][ox].info)
		    ) {
			if (monster_can_cross_terrain(zcave[oy][ox].feat, r_ptr, FALSE, zcave[oy][ox].info) ||
			    !magik(MONSTER_CROSS_IMPOSSIBLE_CHANCE)) {
				/* Assume no move allowed */
				do_move = FALSE;
			}
		}

		/* A monster is in the way */
		if (do_move && c_ptr->m_idx > 0) {
			monster_race *z_ptr = race_inf(y_ptr);

			/* Assume no movement */
			do_move = FALSE;

			/* Kill weaker monsters */
			if ((r_ptr->flags2 & RF2_KILL_BODY) &&
			    (r_ptr->mexp > z_ptr->mexp) &&
			    !y_ptr->owner) { /* exception: pets/golems are never insta-killed via KILL_BODY */
				/* Allow movement */
				do_move = TRUE;

#ifdef OLD_MONSTER_LORE
				/* Monster ate another monster */
				did_kill_body = TRUE;
#endif

				/* XXX XXX XXX Message */

				/* Kill the monster */
				delete_monster(wpos, ny, nx, TRUE);

				/* Hack -- get the empty monster */
				y_ptr = &m_list[c_ptr->m_idx];
			}
			/* attack player's pet */
			else if (m_list[c_ptr->m_idx].pet && !pfriend) {
				/* Do the attack */
				(void)monster_attack_normal(c_ptr->m_idx, m_idx);

				/* Took a turn */
				do_turn = TRUE;

				/* Do not move */
				do_move = FALSE;
			}
			/* questor attacks monster? */
			else if ((m_ptr->questor && !mfriend) ||
			    /* monster attacks questor? */
			    (y_ptr->questor && (y_ptr->questor_hostile & 0x2))) {
				/* Attack it ! */
				monster_attack_normal(c_ptr->m_idx, m_idx);

				/* Assume no movement */
				do_move = FALSE;

				/* Take a turn */
				do_turn = TRUE;
			}
			/* Push past weaker monsters (unless leaving a wall) */
			else if ((r_ptr->flags2 & RF2_MOVE_BODY) &&
			    (r_ptr->mexp > z_ptr->mexp) &&
			    (cave_floor_bold(zcave, m_ptr->fy, m_ptr->fx)))
			{
				/* Allow movement */
				do_move = TRUE;
#ifdef OLD_MONSTER_LORE
				/* Monster pushed past another monster */
				did_move_body = TRUE;
#endif
				/* XXX XXX XXX Message */
			}
		}

		/* Hack -- player hinders its movement */
#ifdef GENERIC_INTERCEPTION
		if (do_move && !pfriend && monst_check_grab(m_idx, 90, "run")) {
#else
		if (do_move && !pfriend && monst_check_grab(m_idx, 85, "run")) {
#endif
			/* Take a turn */
			do_turn = TRUE;

			do_move = FALSE;
		}


		/* Creature has been allowed to move */
		if (do_move) {
#ifdef ANTI_SEVEN_EXPLOIT /* code part: 'pick up here, after having just overridden a movement step: remember the step direction' */
			if (!random_move && m_ptr->previous_direction != 0) {
				m_ptr->previous_direction = d;
			}
#endif
			/* Take a turn */
			do_turn = TRUE;

			/* Hack -- Update the old location */
			zcave[oy][ox].m_idx = c_ptr->m_idx;

			/* Mega-Hack -- move the old monster, if any */
			if (c_ptr->m_idx > 0) {
				/* Move the old monster */
				y_ptr->fy = oy;
				y_ptr->fx = ox;

				/* Update the old monster */
				update_mon(c_ptr->m_idx, TRUE);
			} else if (c_ptr->m_idx < 0) { /* SHOULD HAPPEN ONLY FOR PETS --happened by MOVE_BODY to a player */
				player_type *q_ptr = Players[0 - c_ptr->m_idx];
				char m_name[MNAME_LEN];

				/* Acquire the monster name */
				monster_desc(Ind, m_name, m_idx, 0x04);

				store_exit(0 - c_ptr->m_idx);

				q_ptr->py = oy;
				q_ptr->px = ox;

				/* Update the old monster */
				update_player(0 - c_ptr->m_idx);
				msg_format(0 - c_ptr->m_idx, "\377y%^s switches place with you!", m_name);

				stop_precision(0 - c_ptr->m_idx);
				stop_shooting_till_kill(0 - c_ptr->m_idx);
			}
			cave_midx_debug(wpos, oy, ox, c_ptr->m_idx);

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
				r_ptr->level != 0 &&
			    /* Not in Bree (for Santa Claus) - C. Blue */
			    !in_bree(&p_ptr->wpos))
			{
				/* Disturb */
				if (p_ptr->id != m_ptr->owner) disturb(Ind, 0, 0);
			}

			/* for leaderless guild halls */
			if ((r_ptr->flags2 & RF2_TAKE_ITEM) && (zcave[ny][nx].info & CAVE_GUILD_SUS))
				take_item_override = TRUE;

			/* Take or Kill objects (not "gold") on the floor */
			if (o_ptr->k_idx &&
			    ((r_ptr->flags2 & RF2_TAKE_ITEM) ||
			     (r_ptr->flags2 & RF2_KILL_ITEM)
#if 1
			    || (m_ptr->r_idx == RI_PANDA)
#endif
			    ) && mon_allowed_pickup(o_ptr->tval)) {
				char m_name[MNAME_LEN];
				char m_name_real[MNAME_LEN];
				char o_name[ONAME_LEN];

				/* Check the grid */
				o_ptr = &o_list[c_ptr->o_idx];

				/* Acquire the object name */
				object_desc(Ind, o_name, o_ptr, TRUE, 3);

				/* Acquire the monster name */
				monster_desc(Ind, m_name, m_idx, 0x04);

				/* Real name for logging */
				monster_desc(Ind, m_name_real, m_idx, 0x100 | 0x80);

				/* Prevent monsters from 'exploiting' (nothing)s */
				if (nothing_test(o_ptr, p_ptr, wpos, nx, ny, 4)) {
//					s_printf("NOTHINGHACK: monster %s doesn't meet item %s at wpos %d,%d,%d.\n", m_name_real, o_name, wpos->wx, wpos->wy, wpos->wz);
				}

				/* The object cannot be picked up by the monster */
				else if (!monster_can_pickup(r_ptr, o_ptr)) {
					/* Only give a message for "take_item" */
					if (r_ptr->flags2 & RF2_TAKE_ITEM) {
#ifdef OLD_MONSTER_LORE
						/* Take note */
						did_take_item = TRUE;
#endif

						/* Describe observable situations */
						if (p_ptr->mon_vis[m_idx] && player_has_los_bold(Ind, ny, nx)) {
							/* Dump a message */
							msg_format(Ind, "%^s tries to pick up %s, but fails.",
							    m_name, o_name);
						}
					}
				}

				/* Pick up the item */
				else if ((r_ptr->flags2 & RF2_TAKE_ITEM)
				    /* idea: don't carry valuable loot ouf ot no-tele vaults for the player's delight */
				    && !(zcave[ny][nx].info & CAVE_STCK)
				    && !take_item_override)
				{
					s16b this_o_idx = 0;

#ifdef OLD_MONSTER_LORE
					/* Take note */
					did_take_item = TRUE;
#endif

					/* Describe observable situations */
					if (player_has_los_bold(Ind, ny, nx)) {
						/* Dump a message */
						msg_format(Ind, "%^s picks up %s.", m_name, o_name);
					}

#ifdef MONSTER_INVENTORY
					this_o_idx = c_ptr->o_idx;

 #ifdef MONSTER_ITEM_CONSUME
					if (magik(MONSTER_ITEM_CONSUME)) {
						/* the_sandman - logs monsters pick_up? :S (to track items going poof
						 * without any explanation(s)... To reduce the amount of spammage, lets
						 * just log the owned items... All the housed items are owned anyway */
						if (o_ptr->owner) s_printf("ITEM_TAKEN_DELETE: %s by %s\n", o_name, m_name_real);

						/* Delete the object */
						delete_object_idx(this_o_idx, TRUE);
					} else
 #endif	// MONSTER_ITEM_CONSUME
					{
						/* the_sandman - logs monsters pick_up? :S (to track items going poof
						 * without any explanation(s)... To reduce the amount of spammage, lets
						 * just log the owned items... All the housed items are owned anyway */
						if (o_ptr->owner) s_printf("ITEM_TAKEN: %s by %s\n", o_name, m_name_real);

						/* paranoia */
						o_ptr->held_m_idx = 0;

						/* Excise the object */
						excise_object_idx(this_o_idx);

						/* Forget mark */
//						o_ptr->marked = FALSE;

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
					/* the_sandman - logs monsters pick_up? :S (to track items going poof
					 * without any explanation(s)... To reduce the amount of spammage, lets
					 * just log the owned items... All the housed items are owned anyway */
					if (o_ptr->owner) s_printf("ITEM_TAKEN_DELETE: %s by %s\n", o_name, m_name_real);

					/* Delete the object */
					delete_object(wpos, ny, nx, TRUE);
#endif	// MONSTER_INVENTORY
				}

				/* Destroy the item */
				else if ((r_ptr->flags2 & RF2_KILL_ITEM) || take_item_override) {
#ifdef OLD_MONSTER_LORE
					/* Take note */
					did_kill_item = TRUE;
#endif

					/* C. Blue - added logging for KILL_ITEM monsters (see above TAKE_ITEM) */
					if (o_ptr->owner) s_printf("ITEM_KILLED: %s by %s\n", o_name, m_name_real);

					/* Describe observable situations */
					if (player_has_los_bold(Ind, ny, nx)) {
						/* Dump a message */
						msg_format(Ind, "%^s crushes %s.", m_name, o_name);
					}

					/* Delete the object */
//					delete_object(wpos, ny, nx);	/* arts.. */
					delete_object_idx(c_ptr->o_idx, TRUE);

#if 0	// XXX
					/* Scan all objects in the grid */
					for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
						/* Acquire object */
						o_ptr = &o_list[this_o_idx];

						/* Acquire next object */
						next_o_idx = o_ptr->next_o_idx;

						if (artifact_p(o_ptr)) {
							c_ptr->o_idx = this_o_idx;
							nothing_test2(c_ptr, nx, ny, wpos, 5);
							break;
						}

						/* Wipe the object */
						delete_object_idx(this_o_idx, TRUE);
					}
#endif
				}

#if 1 /* special item consumers, hard-coded */
				/* Pick up the item */
				else if (m_ptr->r_idx == RI_PANDA &&
				    o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT &&
				    (strstr(o_name, "bamboo") || strstr(o_name, "Bamboo"))) {
					s_printf("ITEM_CONSUMED: %s by %s\n", o_name, m_name_real);

					/* Describe observable situations */
 #if 0
					if (player_has_los_bold(Ind, ny, nx))
						msg_format(Ind, "%^s eats %s.", m_name, o_name); /* ^^ */
 #else
					msg_print_near_monster(m_idx, format("eats %s.", o_name));
 #endif

					m_ptr->energy -= level_speed(&m_ptr->wpos) * 8;//seconds, approx. 8 is max due to s16b overflow!

					/* Delete the object */
					delete_object(wpos, ny, nx, TRUE);
				}
			}
#endif

			/* Check for monster trap */
			if (c_ptr->feat == FEAT_MON_TRAP && mon_hit_trap(m_idx)) return;
			
			/* Explosive glyph? */
			if (c_ptr->feat == FEAT_RUNE && warding_rune_break(m_idx)) return;

			/* Questor arrived at walk destination? */
			if (m_ptr->questor && nx == m_ptr->destx && ny == m_ptr->desty)
				quest_questor_arrived(m_ptr->quest, m_ptr->questor_idx, wpos);
		}

		/* Stop when done */
		if (do_turn)  {
			m_ptr->energy -= level_speed(&m_ptr->wpos);
			break;
		}
	}

	/* AI_ANNOY special treatment: Actually attack when out of moves! */
	if (!do_move && !do_turn && do_melee) {
		player_type *q_ptr = get_melee_target(r_ptr, m_ptr, NULL, pfriend);

		if (q_ptr) {
			m_ptr->energy -= level_speed(&m_ptr->wpos);

			/* Note: we ignore RF2_MOVE_BODY here, instead we always do a normal attack.
			   This doesn't matter much though, since this is only for AI_ANNOY anyway. */

			/* Do the attack */
			(void)make_attack_melee(q_ptr->Ind, m_idx);

			/* Took a turn */
			do_turn = TRUE;
			m_ptr->energy -= level_speed(&m_ptr->wpos);

			/* Do not move */
			do_move = FALSE;
		}
	}

	/* Notice changes in view */
	if (do_view) {
		/* Update some things */
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
	}


#ifdef OLD_MONSTER_LORE
	/* Learn things from observable monster */
	if (p_ptr->mon_vis[m_idx]) {
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
#endif


	/* Hack -- get "bold" if out of options */
	if (!do_turn && !do_move && m_ptr->monfear) {
		/* No longer afraid */
		m_ptr->monfear = 0;
		//m_ptr->monfear_gone = 1;

		/* Message if seen */
		if (p_ptr->mon_vis[m_idx]) {
			char m_name[MNAME_LEN];

			/* Acquire the monster name */
			monster_desc(Ind, m_name, m_idx, 0);

			/* Dump a message */
//			msg_format(Ind, "%^s turns to fight!", m_name);
			msg_print_near_monster(m_idx, "turns to fight!");
		}

		/* XXX XXX XXX Actually do something now (?) */
	}

#ifdef ANTI_SEVEN_EXPLOIT /* code part: 'save new closest player distance after making a RANDOM MOVE' */
	if (m_ptr->previous_direction) {
 #ifdef ANTI_SEVEN_EXPLOIT_VAR1
		i = distance(m_ptr->fy, m_ptr->fx, m_ptr->damage_ty, m_ptr->damage_tx);
 #endif
		if (random_move) {
			/* note: currently doesn't use the real new cdis,
			   but instead the old cdis (so it's not really in
			   effect, but probably it doesn't matter much atm */
			m_ptr->cdis_on_damage = m_ptr->cdis;
 #ifdef ANTI_SEVEN_EXPLOIT_VAR1
		} else {
			/* update distance to epicenter, if we didn't get closer despite of moving, STOP algorithm.
			This is actually for preventing going back and forth when next to the epicenter.
			Although it'd possibly be harmless if we did so. */
			if (do_move && (i == m_ptr->damage_dis)) m_ptr->previous_direction = 0;
 #endif
		}
 #ifdef ANTI_SEVEN_EXPLOIT_VAR1
		/* update distance to epicenter */
		m_ptr->damage_dis = i;
 #endif
	}
#endif
}
#ifdef RPG_SERVER
/* the pet handler. note that at the moment it _may_ be almost
 * identical to the golem's handler, except for some little
 * stuff. but let's NOT merge the two and add pet check hacks to
 * the golem handler. (i have plans for the pet system)
 * - the_sandman
 */
static void process_monster_pet(int Ind, int m_idx) {
	//player_type *p_ptr;
	monster_type	*m_ptr = &m_list[m_idx];
	monster_race    *r_ptr = race_inf(m_ptr);
	struct worldpos *wpos = &m_ptr->wpos;

	int			i, d, oy, ox, ny, nx;

	int			mm[8];

	cave_type    	*c_ptr;
	//object_type 	*o_ptr;
	monster_type	*y_ptr;

	bool		do_turn;
	bool		do_move;
	//bool		do_view;

	bool		did_open_door;
	bool		did_bash_door;
#ifdef OLD_MONSTER_LORE
	bool		did_take_item;
	bool		did_kill_item;
	bool		did_move_body;
	bool		did_kill_body;
	bool		did_pass_wall;
	bool		did_kill_wall;
#endif


   /* hack -- don't process monsters on wilderness levels that have not
    * been regenerated yet.
	*/
   cave_type **zcave;
   if (!(zcave = getcave(wpos))) return;

#if 0
   if (Ind > 0) p_ptr = Players[Ind];
   else p_ptr = NULL;
#endif
   m_ptr->mind |= (GOLEM_ATTACK|GOLEM_GUARD|GOLEM_FOLLOW);

	/* handle "stun" */
	if (m_ptr->stunned) {
		int d = 1;

		/* make a "saving throw" against stun */
		if (rand_int(5000) <= r_ptr->level * r_ptr->level) {
			/* recover fully */
			d = m_ptr->stunned;
		}

		/* hack -- recover from stun */
		if (m_ptr->stunned > d) {
			/* recover somewhat */
			m_ptr->stunned -= d;
		}
		/* fully recover */
		else {
			/* recover fully */
			m_ptr->stunned = 0;
		}

		/* still stunned */
		if (m_ptr->stunned)  {
			m_ptr->energy -= level_speed(&m_ptr->wpos);
			return;
		}
	}

	/* handle confusion */
	if (m_ptr->confused && !(r_ptr->flags3&RF3_NO_CONF)) {
		/* amount of "boldness" */
		int d = randint(r_ptr->level / 10 + 1);

		/* still confused */
		if (m_ptr->confused > d) {
			/* reduce the confusion */
			m_ptr->confused -= d;
		}
		/* recovered */
		else {
			/* no longer confused */
			m_ptr->confused = 0;
		}
	}

	/* get the origin */
	oy = m_ptr->fy;
	ox = m_ptr->fx;
	
	/* hack -- assume no movement */
	mm[0] = mm[1] = mm[2] = mm[3] = 0;
	mm[4] = mm[5] = mm[6] = mm[7] = 0;


	/* confused -- 100% random */
	if (m_ptr->confused) {
		/* try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}
	/* normal movement */
	else {
		/* logical moves */
		if (!get_moves_pet(Ind, m_idx, mm)) return;
	}


	/* assume nothing */
	do_turn = FALSE;
	do_move = FALSE;
	//do_view = FALSE;

	/* assume nothing */
	did_open_door = FALSE;
	did_bash_door = FALSE;
#ifdef OLD_MONSTER_LORE
	did_take_item = FALSE;
	did_kill_item = FALSE;
	did_move_body = FALSE;
	did_kill_body = FALSE;
	did_pass_wall = FALSE;
	did_kill_wall = FALSE;
#endif

	/* take a zero-terminated array of "directions" */
	for (i = 0; mm[i]; i++) {
		/* get the direction */
		d = mm[i];

		/* hack -- allow "randomized" motion */
		if (d == 5) d = ddd[rand_int(8)];

		/* get the destination */
		ny = oy + ddy[d];
		nx = ox + ddx[d];			
		
		/* access that cave grid */
		c_ptr = &zcave[ny][nx];

		/* access that cave grid's contents */
		//o_ptr = &o_list[c_ptr->o_idx];

		/* access that cave grid's contents */
		y_ptr = &m_list[c_ptr->m_idx];

		/* Tavern entrance? */
		if (c_ptr->feat == FEAT_SHOP)
			do_move = TRUE;
#if 0
		/* Floor is open? */
		else if (cave_floor_bold(zcave, ny, nx)) {
			/* Go ahead and move */
			do_move = TRUE;
		}
#else
		/* Floor is open? */
		else if (creature_can_enter(r_ptr, c_ptr)) {
			/* Go ahead and move */
			do_move = TRUE;
		}
#endif
		/* Player ghost in wall XXX */
		else if (c_ptr->m_idx < 0) {
			/* Move into player */
			do_move = TRUE;
		}
		/* Permanent wall / permanently wanted structure */
		else if (((f_info[c_ptr->feat].flags1 & FF1_PERMANENT) ||
			(f_info[c_ptr->feat].flags2 & FF2_BOUNDARY) ||
			(c_ptr->feat == FEAT_WALL_HOUSE) ||
			(c_ptr->feat == FEAT_HOME_HEAD) ||
			(c_ptr->feat == FEAT_HOME_TAIL) ||
			(c_ptr->feat == FEAT_HOME_OPEN) ||
			(c_ptr->feat == FEAT_HOME) ||
			(c_ptr->feat == FEAT_ALTAR_HEAD) ||
			(c_ptr->feat == FEAT_ALTAR_TAIL))
			&& !(
			(c_ptr->feat == FEAT_TREE) ||
			(c_ptr->feat == FEAT_BUSH) ||
			(c_ptr->feat == FEAT_DEAD_TREE)))
		{
			/* Nothing */
		}
		else if ( (c_ptr->feat == FEAT_TREE) ||
			(c_ptr->feat == FEAT_BUSH) ||
			(c_ptr->feat == FEAT_DEAD_TREE)) {
			if (r_ptr->flags7 & RF7_CAN_FLY)
				do_move = TRUE;
		}
		/* Monster moves through walls (and doors) */
		else if (r_ptr->flags2 & RF2_PASS_WALL) {
			/* Pass through walls/doors/rubble */
			do_move = TRUE;

#ifdef OLD_MONSTER_LORE
			/* Monster went through a wall */
			did_pass_wall = TRUE;
#endif
		}
		/* Monster destroys walls (and doors) */
		else if (r_ptr->flags2 & RF2_KILL_WALL) {
			/* Eat through walls/doors/rubble */
			do_move = TRUE;

#ifdef OLD_MONSTER_LORE
			/* Monster destroyed a wall */
			did_kill_wall = TRUE;
#endif

			/* Create floor */
//			c_ptr->feat = FEAT_FLOOR;
			cave_set_feat_live(wpos, ny, nx, twall_erosion(wpos, ny, nx));

			/* Forget the "field mark", if any */
			everyone_forget_spot(wpos, ny, nx);

			/* Notice */
			note_spot_depth(wpos, ny, nx);

			/* Redraw */
			everyone_lite_spot(wpos, ny, nx);

			/* Note changes to viewable region */
			//if (player_has_los_bold(Ind, ny, nx)) do_view = TRUE;
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
			if (r_ptr->flags2 & RF2_OPEN_DOOR) {
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
				else if (c_ptr->feat < FEAT_DOOR_HEAD + 0x08) {
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
			if (may_bash && (r_ptr->flags2 & RF2_BASH_DOOR)) {
				int k;

				/* Door power */
				k = ((c_ptr->feat - FEAT_DOOR_HEAD) & 0x07);

#if 0
				/* XXX XXX XXX XXX Old test (pval 10 to 20) */
				if (randint((m_ptr->hp + 1) * (50 + o_ptr->pval)) <
				    40 * (m_ptr->hp - 10 - o_ptr->pval));
#endif

				/* Attempt to Bash XXX XXX XXX */
				if (rand_int(m_ptr->hp / 10) > k) {
					/* The door was bashed open */
					did_bash_door = TRUE;

					/* Hack -- fall into doorway */
					do_move = TRUE;
				}
			}

			/* Deal with doors in the way */
			if (did_open_door || did_bash_door) {
				/* Break down the door */
				if (did_bash_door && (rand_int(100) < 50))
					c_ptr->feat = FEAT_BROKEN;
				/* Open the door */
				else
					c_ptr->feat = FEAT_OPEN;

				/* notice */
				note_spot_depth(wpos, ny, nx);

				/* redraw */
				everyone_lite_spot(wpos, ny, nx);
			}
		}

		if (!find_player(m_ptr->owner)) return; //hack: the owner must be online please. taking this out -> panic()
		/* the player is in the way.  attack him. */
		if (do_move && (c_ptr->m_idx < 0) && (c_ptr->m_idx >= -NumPlayers)) {
			player_type *p_ptr = Players[0-c_ptr->m_idx];

			/* sanity check */
			if (p_ptr->id != m_ptr->owner &&
			   (find_player(m_ptr->owner) == 0 ||
			    find_player(-c_ptr->m_idx) == 0)) {
				do_move = FALSE;
				do_turn = FALSE;
			}
			/* do the attack only if hostile... */
			if (p_ptr->id != m_ptr->owner &&
			    check_hostile(0-c_ptr->m_idx, find_player(m_ptr->owner))) {
				(void)make_attack_melee(0 - c_ptr->m_idx, m_idx);
				do_move = FALSE;
				do_turn = TRUE;
			} else {
				if (m_ptr->owner != p_ptr->id) {
					if (magik(10))
						msg_format(find_player(m_ptr->owner),
						 "\377gYour pet is staring at %s.",
						 p_ptr->name);
					if (magik(10)) {
						player_type *q_ptr = Players[find_player(m_ptr->owner)];

						switch (q_ptr->name[strlen(q_ptr->name) - 1]) {
						case 's': case 'x': case 'z':
							msg_format(-c_ptr->m_idx, "\377g%s' pet is staring at you.", q_ptr->name);
							break;
						default:
							msg_format(-c_ptr->m_idx, "\377g%s's pet is staring at you.", q_ptr->name);
						}
					}
				}
				do_move = FALSE;
				do_turn = TRUE;
			}
		}
		/* a monster is in the way */
		else if (do_move && c_ptr->m_idx > 0) {
			/* attack it ! */
			if (m_ptr->owner != y_ptr->owner || (y_ptr->owner && check_hostile(find_player(m_ptr->owner), find_player(y_ptr->owner))))
			//if (m_ptr->owner != y_ptr->owner || !(y_ptr->pet && y_ptr->owner && !check_hostile(find_player(m_ptr->owner), find_player(y_ptr->owner))))
				monster_attack_normal(c_ptr->m_idx, m_idx);

			/* assume no movement */
			do_move = FALSE;
			/* take a turn */
			do_turn = TRUE;
		}


		/* creature has been allowed move */
		if (do_move) {
			/* take a turn */
			do_turn = TRUE;

			/* hack -- update the old location */
			zcave[oy][ox].m_idx = c_ptr->m_idx;

			/* mega-hack -- move the old monster, if any */
			if (c_ptr->m_idx > 0) {
				/* move the old monster */
				y_ptr->fy = oy;
				y_ptr->fx = ox;

				/* update the old monster */
				update_mon(c_ptr->m_idx, TRUE);
			}

			/* hack -- update the new location */
			c_ptr->m_idx = m_idx;

			/* move the monster */
			m_ptr->fy = ny;
			m_ptr->fx = nx;

			/* update the monster */
			update_mon(m_idx, TRUE);
cave_midx_debug(wpos, oy, ox, c_ptr->m_idx);
			/* redraw the old grid */
			everyone_lite_spot(wpos, oy, ox);

			/* redraw the new grid */
			everyone_lite_spot(wpos, ny, nx);
		}

		/* stop when done */
		if (do_turn) {
			m_ptr->energy -= level_speed(&m_ptr->wpos);
			break;
		}
	}

#if 0
	/* Notice changes in view */
	if (do_view) {
		/* Update some things */
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
	}
#endif

	/* hack -- get "bold" if out of options */
	if (!do_turn && !do_move && m_ptr->monfear) {
		/* no longer afraid */
		m_ptr->monfear = 0;
		//m_ptr->monfear_gone = 1;
	}
}
#endif
static void process_monster_golem(int Ind, int m_idx) {
	//player_type *p_ptr;

	monster_type	*m_ptr = &m_list[m_idx];
	monster_race    *r_ptr = race_inf(m_ptr);
	struct worldpos *wpos = &m_ptr->wpos;

	int			i, d, oy, ox, ny, nx, Ind_owner;

	int			mm[8];

	cave_type    	*c_ptr;
	//object_type 	*o_ptr;
	monster_type	*y_ptr;

	bool		do_turn;
	bool		do_move;
	//bool		do_view;

	bool		did_open_door;
	bool		did_bash_door;
#ifdef OLD_MONSTER_LORE
	bool		did_take_item;
	bool		did_kill_item;
	bool		did_move_body;
	bool		did_kill_body;
	bool		did_pass_wall;
	bool		did_kill_wall;
#endif


	/* Hack -- don't process monsters on wilderness levels that have not
	   been regenerated yet.
	*/
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;

	Ind_owner = find_player(m_ptr->owner);
#if 0
	/* Owner not logged in -> don't attack?
	   Maybe not. It's cool if the golem can kill monsters around your house for example :) */
	if (!Ind_owner) return;
#endif

	//if (Ind > 0) p_ptr = Players[Ind];
	//else p_ptr = NULL;

	/* Handle "stun" */
	if (m_ptr->stunned) {
		int d = 1;

		/* Make a "saving throw" against stun */
		if (rand_int(5000) <= r_ptr->level * r_ptr->level) {
			/* Recover fully */
			d = m_ptr->stunned;
		}

		/* Hack -- Recover from stun */
		if (m_ptr->stunned > d) {
			/* Recover somewhat */
			m_ptr->stunned -= d;
		}
		/* Fully recover */
		else {
			/* Recover fully */
			m_ptr->stunned = 0;
		}

		/* Still stunned */
		if (m_ptr->stunned) {
			m_ptr->energy -= level_speed(&m_ptr->wpos);
			return;
		}
	}


	/* Handle confusion */
	if (m_ptr->confused) {
		/* Amount of "boldness" */
		int d = randint(r_ptr->level / 10 + 1);

		/* Still confused */
		if (m_ptr->confused > d) {
			/* Reduce the confusion */
			m_ptr->confused -= d;
		}
		/* Recovered */
		else {
			/* No longer confused */
			m_ptr->confused = 0;
		}
	}

	/* Get the origin */
	oy = m_ptr->fy;
	ox = m_ptr->fx;
	
#if 0 /* No golem spells -- yet */
	/* Attempt to cast a spell */
	if (make_attack_spell(Ind, m_idx)) {
		m_ptr->energy -= level_speed(&m_ptr->wpos);
		return;
	}
#endif

	/* Hack -- Assume no movement */
	mm[0] = mm[1] = mm[2] = mm[3] = 0;
	mm[4] = mm[5] = mm[6] = mm[7] = 0;


	/* Confused -- 100% random */
	if (m_ptr->confused) {
		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}
	/* Normal movement */
	else {
		/* Logical moves */
		if (!get_moves_golem(Ind, m_idx, mm)) return;
	}


	/* Assume nothing */
	do_turn = FALSE;
	do_move = FALSE;
	//do_view = FALSE;

	/* Assume nothing */
	did_open_door = FALSE;
	did_bash_door = FALSE;
#ifdef OLD_MONSTER_LORE
	did_take_item = FALSE;
	did_kill_item = FALSE;
	did_move_body = FALSE;
	did_kill_body = FALSE;
	did_pass_wall = FALSE;
	did_kill_wall = FALSE;
#endif

	/* Take a zero-terminated array of "directions" */
	for (i = 0; mm[i]; i++) {
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
		//o_ptr = &o_list[c_ptr->o_idx];

		/* Access that cave grid's contents */
		y_ptr = &m_list[c_ptr->m_idx];


		/* Tavern entrance? */
//		if (c_ptr->feat == FEAT_SHOP_TAIL)
		if (c_ptr->feat == FEAT_SHOP) {
			/* Nothing */
		}
#if 0
		/* Floor is open? */
		else if (cave_floor_bold(zcave, ny, nx)) {
			/* Go ahead and move */
			do_move = TRUE;
		}
#else
		else if (creature_can_enter(r_ptr, c_ptr)) {
			/* Go ahead and move */
			do_move = TRUE;
		}
#endif
		/* Player ghost in wall XXX */
		else if (c_ptr->m_idx < 0) {
			/* Move into player */
			do_move = TRUE;
		}
		/* Permanent wall / permanently wanted structure */
		else if (((f_info[c_ptr->feat].flags1 & FF1_PERMANENT) ||
		    (f_info[c_ptr->feat].flags2 & FF2_BOUNDARY) ||
		    (c_ptr->feat == FEAT_WALL_HOUSE) ||
		    (c_ptr->feat == FEAT_HOME_HEAD) ||
		    (c_ptr->feat == FEAT_HOME_TAIL) ||
		    //(c_ptr->feat == FEAT_SHOP_HEAD) ||
		    //(c_ptr->feat == FEAT_SHOP_TAIL) ||
		    (c_ptr->feat == FEAT_HOME_OPEN) ||
		    (c_ptr->feat == FEAT_HOME) ||
		    (c_ptr->feat == FEAT_ALTAR_HEAD) ||
		    (c_ptr->feat == FEAT_ALTAR_TAIL))
		    && !(
		    (c_ptr->feat == FEAT_TREE) ||
		    (c_ptr->feat == FEAT_BUSH) ||
		    //(c_ptr->feat == FEAT_EVIL_TREE) ||
		    (c_ptr->feat == FEAT_DEAD_TREE)))
		{
			/* Nothing */
		}
		/* Monster moves through walls (and doors) */
		else if (r_ptr->flags2 & RF2_PASS_WALL) {
			/* Pass through walls/doors/rubble */
			do_move = TRUE;

#ifdef OLD_MONSTER_LORE
			/* Monster went through a wall */
			did_pass_wall = TRUE;
#endif
		}
		/* Monster destroys walls (and doors) */
		else if (r_ptr->flags2 & RF2_KILL_WALL) {
			/* Eat through walls/doors/rubble */
			do_move = TRUE;

#ifdef OLD_MONSTER_LORE
			/* Monster destroyed a wall */
			did_kill_wall = TRUE;
#endif

			/* Create floor */
//			c_ptr->feat = FEAT_FLOOR;
			cave_set_feat_live(wpos, ny, nx, twall_erosion(wpos, ny, nx));

			/* Forget the "field mark", if any */
			everyone_forget_spot(wpos, ny, nx);

			/* Notice */
			note_spot_depth(wpos, ny, nx);

			/* Redraw */
			everyone_lite_spot(wpos, ny, nx);

			/* Note changes to viewable region */
			//if (player_has_los_bold(Ind, ny, nx)) do_view = TRUE;
		}
		/* Handle doors and secret doors */
		else if (((c_ptr->feat >= FEAT_DOOR_HEAD) &&
		    (c_ptr->feat <= FEAT_DOOR_TAIL)) ||
		    (c_ptr->feat == FEAT_SECRET)) {
			bool may_bash = TRUE;

			/* Take a turn */
			do_turn = TRUE;

			/* Creature can open doors. */
			if (r_ptr->flags2 & RF2_OPEN_DOOR) {
				/* Closed doors and secret doors */
				if ((c_ptr->feat == FEAT_DOOR_HEAD) ||
				    (c_ptr->feat == FEAT_SECRET)) {
					/* The door is open */
					did_open_door = TRUE;

					/* Do not bash the door */
					may_bash = FALSE;
				}
				/* Locked doors (not jammed) */
				else if (c_ptr->feat < FEAT_DOOR_HEAD + 0x08) {
					int k;

					/* Door power */
					k = ((c_ptr->feat - FEAT_DOOR_HEAD) & 0x07);

#if 0
					/* XXX XXX XXX XXX Old test (pval 10 to 20) */
					if (randint((m_ptr->hp + 1) * (50 + o_ptr->pval)) <
					    40 * (m_ptr->hp - 10 - o_ptr->pval));
#endif

					/* Try to unlock it XXX XXX XXX */
					if (rand_int(m_ptr->hp / 10) > k) {
						/* Unlock the door */
						c_ptr->feat = FEAT_DOOR_HEAD + 0x00;

						/* Do not bash the door */
						may_bash = FALSE;
					}
				}
			}

			/* Stuck doors -- attempt to bash them down if allowed */
			if (may_bash && (r_ptr->flags2 & RF2_BASH_DOOR)) {
				int k;

				/* Door power */
				k = ((c_ptr->feat - FEAT_DOOR_HEAD) & 0x07);

#if 0
				/* XXX XXX XXX XXX Old test (pval 10 to 20) */
				if (randint((m_ptr->hp + 1) * (50 + o_ptr->pval)) <
				    40 * (m_ptr->hp - 10 - o_ptr->pval));
#endif

				/* Attempt to Bash XXX XXX XXX */
				if (rand_int(m_ptr->hp / 10) > k) {
					/* The door was bashed open */
					did_bash_door = TRUE;

					/* Hack -- fall into doorway */
					do_move = TRUE;
				}
			}


			/* Deal with doors in the way */
			if (did_open_door || did_bash_door) {
				/* Break down the door */
				if (did_bash_door && (rand_int(100) < 50))
					c_ptr->feat = FEAT_BROKEN;
				/* Open the door */
				else
					c_ptr->feat = FEAT_OPEN;

				/* Notice */
				note_spot_depth(wpos, ny, nx);

				/* Redraw */
				everyone_lite_spot(wpos, ny, nx);
			}
		}

		/* The player is in the way.  Attack him. */
		if (do_move && (c_ptr->m_idx < 0)
		    && Ind_owner && check_hostile(0-c_ptr->m_idx, Ind_owner)) {
			/* Do the attack */
			if (Players[0-c_ptr->m_idx]->id != m_ptr->owner) (void)make_attack_melee(0 - c_ptr->m_idx, m_idx);

			/* Do not move */
			do_move = FALSE;

			/* Took a turn */
			do_turn = TRUE;
		}

		/* A monster is in the way */
		if (do_move && c_ptr->m_idx > 0) {
			/* Attack it ! */
			if (m_ptr->owner != y_ptr->owner) monster_attack_normal(c_ptr->m_idx, m_idx);

			/* Assume no movement */
			do_move = FALSE;

			/* Take a turn */
			do_turn = TRUE;
		}


		/* Creature has been allowed move */
		if (do_move) {
			/* Take a turn */
			do_turn = TRUE;

			/* Hack -- Update the old location */
			zcave[oy][ox].m_idx = c_ptr->m_idx;

			/* Mega-Hack -- move the old monster, if any */
			if (c_ptr->m_idx > 0) {
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
cave_midx_debug(wpos, oy, ox, c_ptr->m_idx);
			/* Redraw the old grid */
			everyone_lite_spot(wpos, oy, ox);

			/* Redraw the new grid */
			everyone_lite_spot(wpos, ny, nx);
		}

		/* Stop when done */
		if (do_turn) {
			m_ptr->energy -= level_speed(&m_ptr->wpos);
			break;
		}
	}

#if 0
	/* Notice changes in view */
	if (do_view) {
		/* Update some things */
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
	}
#endif

	/* Hack -- get "bold" if out of options */
	if (!do_turn && !do_move && m_ptr->monfear) {
		/* No longer afraid */
		m_ptr->monfear = 0;
		//m_ptr->monfear_gone = 1;
	}
}

#ifdef ASTAR_DISTRIBUTE
void process_monsters_astar(void) {
	int		i;
	int		fx, fy;
	monster_type	*m_ptr;

	for (i = 0; i < ASTAR_MAX_INSTANCES; i++) {
		if (astar_info_open[i].m_idx == -1) continue;

		/* Access the monster */
		m_ptr = &m_list[astar_info_open[i].m_idx];

		/* Be careful, in case the player disconnected somehow
		   before the m_ptr data was properly updated. (Not sure..) */
		if (m_ptr->closest_player <= 0) continue;
		if (m_ptr->closest_player > NumPlayers) continue;
		//maybe todo: super para check via id:
		//if (m_ptr->closest_player_id != Players[m_ptr->closest_player].id) continue;

		/* Access the location */
		fx = m_ptr->fx;
		fy = m_ptr->fy;

		get_moves_astar(m_ptr->closest_player, astar_info_open[i].m_idx, &fy, &fx); //hack: own coordinates tell us that this isn't the real thing
	}
}
#endif



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

void process_monsters(void) {
	int		k, i, e, pl, tmp, j, n;
	int		fx, fy;

	bool		test;

	int		closest, dis_to_closest, lowhp;
	bool		blos, new_los;
	bool		interval = ((turn % cfg.fps) == 0); //assumes that cfg.fps is divisable by MONSTER_TURNS!

	monster_type	*m_ptr;
	monster_race	*r_ptr;
	player_type	*p_ptr;
	bool		reveal_cloaking, spot_cloaking;
	int		may_move_Ind, may_move_dis;
	char		m_name[MNAME_LEN];
	cave_type	**zcave;

	/* Local copies for speed - mikaelh */
	s16b *_m_fast = m_fast;
	monster_type *_m_list = m_list;
	player_type **_Players = Players;

	/* maybe better do in dungeon()?	- Jir - */
#ifdef PROJECTION_FLUSH_LIMIT
	count_project_times++;

	/* Reset projection counts */
	if (count_project_times >= PROJECTION_FLUSH_LIMIT_TURNS) {
 #if DEBUG_LEVEL > 2
		if (count_project > PROJECTION_FLUSH_LIMIT)
			s_printf("project() flush suppressed(%d)\n", count_project);
 #endif	// DEBUG_LEVEL
		count_project = 0;
		count_project_times = 0;
	}
#endif	// PROJECTION_FLUSH_LIMIT


	/* Process the monsters */
#ifdef PROCESS_MONSTERS_DISTRIBUTE
	for (k = m_top - 1 - turn % MONSTER_TURNS; k >= 0; k -= MONSTER_TURNS) {
#else
	for (k = m_top - 1; k >= 0; k--) {
#endif
		/*int closest = -1, dis_to_closest = 9999, lowhp = 9999;
		bool blos = FALSE, new_los;	*/

		/* Access the index */
		i = _m_fast[k];

		/* Access the monster */
		m_ptr = &_m_list[i];

		/* Excise "dead" monsters */
		if (!m_ptr->r_idx) {
			/* Excise the monster */
			_m_fast[k] = _m_fast[--m_top];

			/* Skip */
			continue;
		}

		r_ptr = race_inf(m_ptr);

		/* Access the location */
		fx = m_ptr->fx;
		fy = m_ptr->fy;

		/* Efficiency */
		//if (!(getcave(m_ptr->wpos))) return(FALSE);
		//if (!Players_on_depth(&m_ptr->wpos)) continue;

		/* Obtain the energy boost */
		e = extract_energy[m_ptr->mspeed];

#if 0 /* bugged here, moved downwards a couple of lines */
		/* Added for Valinor - C. Blue */
		if (r_ptr->flags7 & RF7_NEVER_ACT) m_ptr->energy = 0;
#endif

		/* Give this monster some energy */
		m_ptr->energy += e * MONSTER_TURNS;

		tmp = level_speed(&m_ptr->wpos);

		/* Added for Valinor; also used by target dummy - C. Blue */
		if (r_ptr->flags7 & RF7_NEVER_ACT) m_ptr->energy = 0;

		/* Target dummy "snowiness" hack, checked once per second */
		if (interval && !m_ptr->wpos.wz) {
			if (((m_ptr->r_idx == RI_TARGET_DUMMY1) || (m_ptr->r_idx == RI_TARGET_DUMMY2)) &&
			    (m_ptr->extra < 60) && (turn % cfg.fps == 0) &&
			    (wild_info[m_ptr->wpos.wy][m_ptr->wpos.wx].weather_type == 2)) {
				m_ptr->extra++;
				if ((m_ptr->r_idx == RI_TARGET_DUMMY1) && (m_ptr->extra == 30)) {
					m_ptr->r_idx = RI_TARGET_DUMMY2;
					everyone_lite_spot(&m_ptr->wpos, fy, fx);
				}
			}
			if (((m_ptr->r_idx == RI_TARGET_DUMMYA1) || (m_ptr->r_idx == RI_TARGET_DUMMYA2)) &&
			    (m_ptr->extra < 60) && (turn % cfg.fps == 0) &&
			    (wild_info[m_ptr->wpos.wy][m_ptr->wpos.wx].weather_type == 2)) {
				m_ptr->extra++;
				if ((m_ptr->r_idx == RI_TARGET_DUMMYA1) && (m_ptr->extra == 30)) {
					m_ptr->r_idx = RI_TARGET_DUMMYA2;
					everyone_lite_spot(&m_ptr->wpos, fy, fx);
				}
			}
		}

		/* Not enough energy to move */
		if (m_ptr->energy < tmp) continue;


		/* Make sure we don't store up too much energy */
		//if (m_ptr->energy > tmp) m_ptr->energy = tmp;
		m_ptr->energy = tmp;

		closest = -1;
		dis_to_closest = 9999;
		lowhp = 9999;
		blos = FALSE;

#ifdef C_BLUE_AI_MELEE
		/* save our previous melee target.
		   NOTE: This must be _after_ the energy-check. */
		m_ptr->last_target_melee_temp = m_ptr->last_target_melee;
		/* forget that target in case combat is interrupted,
		   depending on monster type: */
		if ((r_ptr->flags7 & RF7_MULTIPLY) || /* <- can't memorize */
		    (r_ptr->flags2 & RF2_SMART) || /* <- decides to re-evaluate targets */
		    ((r_ptr->flags2 & RF2_WEIRD_MIND) && magik(50)) || /* <- sometimes memorize */
		    (r_ptr->flags2 & RF2_EMPTY_MIND)) /* <- can't memorize */
			/* forget old target */
			m_ptr->last_target_melee = 0;
#endif

		/* is monster allowed to move although all targets are cloaked? */
		may_move_Ind = 0;
		may_move_dis = 9999;

#if 1
		/* Hack, Sandman's robin */
		if (m_ptr->r_idx == RI_ROBIN) {
			m_ptr->extra = 0;
			for (pl = 1; pl <= NumPlayers; pl++) {
				p_ptr = Players[pl];
				if (!strcmp(p_ptr->accountname, "The_sandman")) { //you can make it!~
					m_ptr->extra = 1;
					closest = pl;
					dis_to_closest = distance(p_ptr->py, p_ptr->px, fy, fx); //probably not needed
					break;
				}
			}
		}
		if (closest == -1)
#endif
		/* Find the closest player */
		for (pl = 1, n = NumPlayers; pl <= n; pl++)  {
			p_ptr = _Players[pl];
			reveal_cloaking = spot_cloaking = FALSE;

			/* Only check him if he is playing */
			if (p_ptr->conn == NOT_CONNECTED)
				continue;

			/* Make sure he's on the same dungeon level */
			if (!inarea(&p_ptr->wpos, &m_ptr->wpos))
				continue;

			/* Hack -- Skip him if he's shopping -
			   in a town, so dungeon stores aren't cheezy */
			if ((p_ptr->store_num != -1) && (p_ptr->wpos.wz == 0))
				continue;

			/* Hack -- make the dungeon master invisible to monsters */
//			if (p_ptr->admin_dm) continue;
			if (p_ptr->admin_dm && (!m_ptr->owner || (m_ptr->owner != p_ptr->id))) continue; /* for Dungeon Master GF_DOMINATE */

			/*
			 * Hack -- Ignore players that have died or left
			 * as suggested by PowerWyrm - mikaelh */
			if (!p_ptr->alive || p_ptr->death || p_ptr->new_level_flag) continue;

			/* Monsters serve a king on his land they dont attack him */
			// if (player_is_king(pl)) continue;

			/* Compute distance */
			j = distance(p_ptr->py, p_ptr->px, fy, fx);

			/* Change monster's highest player encounter - mode 3: monster is awake and player is within its area of awareness */
			if (cfg.henc_strictness == 3 && !m_ptr->csleep) {
				if (j <= r_ptr->aaf) {
					if (!in_bree(&m_ptr->wpos)) { /* not in Bree, because of Halloween :) */
						if (m_ptr->henc < p_ptr->max_lev) m_ptr->henc = p_ptr->max_lev;
						if (m_ptr->henc_top < (p_ptr->max_lev + p_ptr->max_plv) / 2) m_ptr->henc_top = (p_ptr->max_lev + p_ptr->max_plv) / 2;
						if (m_ptr->henc < p_ptr->supp) m_ptr->henc = p_ptr->supp;
						if (m_ptr->henc_top < (p_ptr->max_lev + p_ptr->supp_top) / 2) m_ptr->henc_top = (p_ptr->max_lev + p_ptr->supp_top) / 2;
					}
				}
			}

			/* Skip if the monster can't see the player */
//			if (player_invis(pl, m_ptr, j)) continue;	/* moved */

			/* Glaur. Check if monster has LOS to the player */
			new_los = los(&p_ptr->wpos, p_ptr->py, p_ptr->px, fy, fx);

#ifdef USE_SOUND_2010
			/* USE_SOUND_2010 -- temporary-boss-monster checks:
			   These are those (lesser) boss monsters that will only cause a
			   special music when we're actively engaging them. - C. Blue */
			if (new_los && p_ptr->music_monster != -2 && p_ptr->mon_vis[i]) { // && !m_ptr->csleep
				if (r_ptr->flags7 & RF7_NAZGUL) {
					//Nazgul; doesn't override Sauron or Halloween (The Great Pumpkin)
					if (p_ptr->music_monster != 43 && p_ptr->music_monster != 55) {
						Send_music(pl, (p_ptr->music_monster = 42), -1);
					}
				} else if (r_ptr->flags1 & RF1_UNIQUE) {
					if (m_ptr->r_idx == RI_SAURON) {
						//Sauron; overrides all others
 #ifdef GLOBAL_DUNGEON_KNOWLEDGE
						/* we now 'learned' who is the boss of this dungeon */
						if (p_ptr->wpos.wz > 0) wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].tower->known |= 0x8;
						else wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].dungeon->known |= 0x8;
 #endif
						Send_music(pl, (p_ptr->music_monster = 43), -1);
					}
					//Dungeon boss or special unique? (can't override Sauron, Nazgul or Halloween)
					else if (p_ptr->music_monster != 43 && p_ptr->music_monster != 42 && p_ptr->music_monster != 55) {
						//Dungeon boss?
						if (r_ptr->flags0 & RF0_FINAL_GUARDIAN) {
 #ifdef GLOBAL_DUNGEON_KNOWLEDGE
							/* we now 'learned' who is the boss of this dungeon */
							if (p_ptr->wpos.wz > 0) wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].tower->known |= 0x8;
							else wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].dungeon->known |= 0x8;
 #endif
							Send_music(pl, (p_ptr->music_monster = 41), -1);
						//Special Unique (non-respawning)? Can't override dungeon boss..
						} else if (r_ptr->level >= 98 && p_ptr->music_monster != 41) {
							//Any of em
							Send_music(pl, (p_ptr->music_monster = 40), -1);
						}
					}
				} else if ((m_ptr->r_idx == RI_PUMPKIN1 || m_ptr->r_idx == RI_PUMPKIN2 || m_ptr->r_idx == RI_PUMPKIN3)
				    && (p_ptr->music_monster != 43)) {
					//The Great Pumpkin; overrides Nazgul, Dungeon Bosses and even Special Uniques ^^
					Send_music(pl, (p_ptr->music_monster = 55), -1);
				} else if ((m_ptr->r_idx == RI_SANTA1 || m_ptr->r_idx == RI_SANTA2)
				    && (p_ptr->music_monster != 43)) {
					//Santa Claus
					Send_music(pl, (p_ptr->music_monster = 67), -1);
				}
			}
#endif

#ifdef DOUBLE_LOS_SAFETY
			/* los() may actually fail sometimes in both directions,
			   if monster stands right behind a corner and player is still further away.
			   In that case the player who doesn't check los() may still be able to cast
			   while the monster couldn't. So double check via projectable..() here: */
			new_los |= projectable_wall(&p_ptr->wpos, p_ptr->py, p_ptr->px, fy, fx, MAX_RANGE);
#endif

			/*	if (p_ptr->ghost)
				if (!new_los)
				j += 100; */

#if 0
			/* Hack: If monster can ignore walls, we don't need 'air los', but can instead
			   use a 'wall los' which just gets hindered by perma-walls ;) - C. Blue */
			if ((r_ptr->flags2 & (RF2_KILL_WALL | RF2_PASS_WALL)) &&
			    los_wall(&p_ptr->wpos, p_ptr->py, p_ptr->px, fy, fx))
	/* add a distance check here, or they'll have full level los */
				new_los = TRUE;
#endif

			/* Glaur. Check that the closest VISIBLE target gets selected,
			   if no visible one available just take the closest*/
			if (((blos >= new_los) && (j > dis_to_closest)) || (blos > new_los))
				continue;

			/* Glaur. Skip if same distance and stronger and same visibility*/
			if ((j == dis_to_closest) && (p_ptr->chp > lowhp) && (blos == new_los))
				continue;

			/* Skip if player wears amulet of invincibility - C. Blue */
			if ((p_ptr->admin_invinc
#ifdef TELEPORT_SURPRISES
			    || TELEPORT_SURPRISED(p_ptr, r_ptr)
#endif
			    ) && (!m_ptr->owner || (m_ptr->owner != p_ptr->id))) /* for Dungeon Master GF_DOMINATE */
				continue;

			/* For Arena Monster Challenge: Skip observers in the corners */
			if ((zcave = getcave(&p_ptr->wpos)) && zcave[p_ptr->py][p_ptr->px].feat == FEAT_HIGHLY_PROTECTED)
				continue;

			/* can spot/uncloak cloaked player? */
			if (p_ptr->cloaked == 1 && !p_ptr->cloak_neutralized) {
				if (strchr("eAN", r_ptr->d_char) ||
				    ((r_ptr->flags1 & RF1_UNIQUE) && (r_ptr->flags2 & RF2_SMART) && (r_ptr->flags2 & RF2_POWERFUL)) ||
				    (r_ptr->flags7 & RF7_NAZGUL)) {
#if 0 /* no reason to go this far I think, just attacking the player as usual should be enough - C. Blue */
					reveal_cloaking = TRUE;
#endif
				} else { /* can't see cloaked player? */
					/* hack: if monster moves highly randomly, we assume that it
					   doesn't really care about a player being nearby or not,
					   and hence keep up the random movement, except using attack
					   spells on the cloaked player! */
					if ((r_ptr->flags1 & RF1_RAND_50) ||
					    (r_ptr->flags5 & RF5_RAND_100)) {
						/* 'remember' closest cloaked player */
						if (j < may_move_dis) {
							may_move_Ind = pl;
							may_move_dis = j;
						}
					}

					/* Normally, cloaked players are ignored and monsters don't move/act: */
					continue;
				}
			}
#if 0 /* maybe doesn't make sense that spotting an action drops camouflage */
			else if (p_ptr->cloaked == 1 && new_los && !m_ptr->csleep && !strchr(",ijlmrsvwzFIQ", r_ptr->d_char) {
				spot_cloaking = TRUE;
			}
#endif

			/* Skip if under mindcrafter charm spell - C. Blue */
			if (m_ptr->charmedignore) {
				/* out of range? */
				if (j > 20 || j > r_ptr->aaf) {
					_Players[m_ptr->charmedignore]->mcharming--;
					m_ptr->charmedignore = 0;
				/* monster gets a sort of saving throw */
				} else if (test_charmedignore(pl, m_ptr->charmedignore, r_ptr))
					continue;
			}

			if (reveal_cloaking) {
				monster_desc(pl, m_name, i, 0);
				msg_format(pl, "\377r%^s reveals your presence!", m_name);
				break_cloaking(pl, 0);
			}
			else if (spot_cloaking) {
				monster_desc(pl, m_name, i, 0);
				msg_format(pl, "\377o%^s spots your actions!", m_name);
				break_cloaking(pl, 0); /* monster reveals our presence */
			}

			/* Remember this player */
			blos = new_los;
			dis_to_closest = j;
			closest = pl;
			lowhp = p_ptr->chp;
		}

		/* Paranoia -- Make sure we found a closest player */
		if (closest == -1) {
			/* hack: still move around randomly? */
			if (may_move_Ind) {
				closest = may_move_Ind;
				dis_to_closest = may_move_dis;
			}
			/* can't act at all - process next monster */
			else continue;
		} else {
			/* if we have a 'real' target, ignore any cloaked-target hacks */
			may_move_Ind = 0;
		}

		m_ptr->cdis = dis_to_closest;
		m_ptr->closest_player = closest;

		/* Hack -- Require proximity */
// TAKEN OUT EXPERIMENTALLY - C. BLUE
//		if (m_ptr->cdis >= 100) continue;
// alternatively try this, if needed at all:
//		if (m_ptr->cdis >= (r_ptr->aaf > 100 ? r_ptr->aaf : 100)) continue;

		p_ptr = _Players[closest];


		/* Assume no move */
		test = FALSE;

		/* Handle "sensing radius" */
		if (m_ptr->cdis <= r_ptr->aaf || (blos && !m_ptr->csleep)) {
			/* We can "sense" the player */
			test = TRUE;
		}

		/* Handle "sight" and "aggravation" */
#if 0
		else if ((m_ptr->cdis <= MAX_SIGHT) &&
		    (player_has_los_bold(closest, fy, fx) ||
		    p_ptr->aggravate))
#else
		else if (
//		    (player_has_los_bold(closest, fy, fx)) ||
 #ifndef REDUCED_AGGRAVATION
		    (p_ptr->aggravate && m_ptr->cdis <= MAX_SIGHT))
 #else
		    (p_ptr->aggravate && m_ptr->cdis <= 50))
 #endif

#endif	// 0
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
		    (cave[fy][fx].cost < r_ptr->aaf)) {
			/* We can "smell" the player */
			test = TRUE;
		}
#endif

		/* Do nothing */
		if (!test) continue;

		/* Change monster's highest player encounter (mode 1+ : monster actively targets a player) */
		if (!m_ptr->csleep && !in_bree(&m_ptr->wpos)) { /* not in Bree, because of Halloween & Christmas (Santa Claus) :) */
			if (m_ptr->henc < p_ptr->max_lev) m_ptr->henc = p_ptr->max_lev;
			if (m_ptr->henc_top < (p_ptr->max_lev + p_ptr->max_plv) / 2) m_ptr->henc_top = (p_ptr-> max_lev + p_ptr->max_plv) / 2;
			if (m_ptr->henc < p_ptr->supp) m_ptr->henc = p_ptr->supp;
			if (m_ptr->henc_top < (p_ptr->max_lev + p_ptr->supp_top) / 2) m_ptr->henc_top = (p_ptr->max_lev + p_ptr->supp_top) / 2;
		}

		/* Process the monster */
		if (!m_ptr->special
#ifdef RPG_SERVER
		    && !m_ptr->pet
#endif
		   )
		{
			/* Hack -- suppress messages */
			if (p_ptr->taciturn_messages) suppress_message = TRUE;

			process_monster(closest, i, (may_move_Ind != 0));

			/* for C_BLUE_AI (to remember if the player stood beside us and
			   then runs away from us to make us follow him): */
			if ((ABS(m_ptr->fy - p_ptr->py) <= 1) && (ABS(m_ptr->fx - p_ptr->px) <= 1))
				m_ptr->last_target = p_ptr->id;

			suppress_message = FALSE;
		}
#ifdef RPG_SERVER
		else if (m_ptr->pet) { //pet
			int p = find_player(m_ptr->owner);
			process_monster_pet(p, i);
		}
#endif
		else { //golem
			int p = find_player(m_ptr->owner);
			process_monster_golem(p, i);
		}

#ifdef SAURON_ANTI_MELEE
		/* Special Sauron enhancements */
		if (m_ptr->r_idx == RI_SAURON) {
			/* temporarily disable AI_ANNOY if the player who causes it is already adjacent to us!
			   otherwise we might waste too much time running instead of casting/attacking. */
			bool adj = ((p_ptr->py - m_ptr->fy <= 1) && (p_ptr->px - m_ptr->fx <= 1));

			/* is player a melee fighter mostly, or can intercept at least? */
			if (!adj &&
			    (p_ptr->s_info[SKILL_SWORD].value + p_ptr->s_info[SKILL_BLUNT].value +
			    p_ptr->s_info[SKILL_AXE].value + p_ptr->s_info[SKILL_POLEARM].value +
			    p_ptr->s_info[SKILL_MARTIAL_ARTS].value >= (p_ptr->max_plv * 2000L) / 3 ||
			    p_ptr->s_info[SKILL_INTERCEPT].value >= (p_ptr->max_plv * 3000L) / 4 ||
			    p_ptr->antimagic >= 40)) {
				if (!(r_info[m_ptr->r_idx].flags7 & RF7_AI_ANNOY)) {
					r_info[m_ptr->r_idx].flags7 |= RF7_AI_ANNOY;
					s_printf("SAURON: add ai_annoy.\n");
				}
			} else {
				if (r_info[m_ptr->r_idx].flags7 & RF7_AI_ANNOY) {
					r_info[m_ptr->r_idx].flags7 &= ~RF7_AI_ANNOY;
					s_printf("SAURON: rm ai_annoy%s\n", adj ? " (adj)." : ".");
				}
			}
		}
#endif
	}

	/* Only when needed, every five game turns */
	/* TODO: move this to the client side!!! */
//	if (scan_monsters && (!(turn%5)))
	if (scan_monsters && !(turn % (MULTI_HUED_UPDATE * MONSTER_TURNS))) {
		/* Shimmer multi-hued monsters */
		for (i = 1, n = m_max; i < n; i++) {
			m_ptr = &_m_list[i];

			/* Skip dead monsters */
			if (!m_ptr->r_idx) continue;

			/* Access the monster race */
			r_ptr = race_inf(m_ptr);

			/* Skip non-multi-hued monsters */
//			if (!(r_ptr->flags1 & RF1_ATTR_MULTI)) continue;
			if (!((r_ptr->flags1 & RF1_ATTR_MULTI) ||
#ifdef SLOW_ATTR_BNW
			    (r_ptr->flags7 & RF7_ATTR_BNW) ||
#endif
			    (r_ptr->flags2 & RF2_SHAPECHANGER) ||
			    (r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags2 & RF2_WEIRD_MIND) ||
			    m_ptr->ego))
				continue;

			if (r_ptr->flags2 & RF2_WEIRD_MIND) {
				update_mon_flicker(i);
				update_mon(i, FALSE);
			}
//			if (r_ptr->flags2 & RF2_WEIRD_MIND) update_mon(i, FALSE);

			/* Shimmer Multi-Hued Monsters */
			everyone_lite_spot(&m_ptr->wpos, m_ptr->fy, m_ptr->fx);
		}
	}
}



/* Due to incapability of adding new item flags,
 * this curse seems too soft.. pfft		- Jir -
 */
void curse_equipment(int Ind, int chance, int heavy_chance) {
	player_type *p_ptr = Players[Ind];
	bool changed = FALSE;
	u32b f1, f2, f3, f4, f5, f6, esp;
	object_type * o_ptr = &p_ptr->inventory[INVEN_WIELD + rand_int(12)];

	if (randint(100) > chance) return;

	if (!(o_ptr->k_idx)) return;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);


	/* Extra, biased saving throw for blessed items */
	if ((f3 & (TR3_BLESSED)) && (randint(888) > chance)) {
		char o_name[ONAME_LEN];
		object_desc(Ind, o_name, o_ptr, FALSE, 0);
		msg_format(Ind, "Your %s resist%s cursing!", o_name,
			((o_ptr->number > 1) ? "" : "s"));
		/* Hmmm -- can we wear multiple items? If not, this is unnecessary */
		return;
	}

#if 0	// l8r..
	if ((randint(100) <= heavy_chance) &&
	    (o_ptr->name1 || o_ptr->name2 || o_ptr->art_name)) {
		if (!(f3 & TR3_HEAVY_CURSE)) changed = TRUE;
		o_ptr->art_flags3 |= TR3_HEAVY_CURSE;
		o_ptr->art_flags3 |= TR3_CURSED;
		o_ptr->ident |= IDENT_CURSED;
	}
	else
#endif	// 0
	{
		if (!(o_ptr->ident & (ID_CURSED))) changed = TRUE;
		//o_ptr->art_flags3 |= TR3_CURSED;
		o_ptr->ident |= ID_CURSED;
	}

	if (changed) {
		msg_print(Ind, "There is a malignant black aura surrounding you...");
		if (o_ptr->note) {
			if (streq(quark_str(o_ptr->note), "uncursed")) {
				o_ptr->note = 0;
			}
		}
	}
}
