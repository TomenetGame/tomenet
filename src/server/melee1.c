/* $Id$ */
/* File: melee1.c */

/* Purpose: Monster attacks */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"


/* EXPERIMENTAL (huge difference): Attempted monster touch-melee attack already causes reactive shields/auras to trigger?
   Default is actually: No, the hit must also win vs AC/dodge/parry/block to actually connect.
   Damage dealt will be order(s) of magnitude greater, so the reactive spells might need damage adjustments;
   alternatively or additionally, the once-per-round define below might need to be considered. - C. Blue */
#define ATTEMPT_TRIGGERS_REACTIVE
#ifdef ATTEMPT_TRIGGERS_REACTIVE
 /* Only trigger reactive shields/auras once per monster round (instead of once per each single attack) */
 #define TRIGGERS_REACTIVE_ONCEPERROUND
#endif


/*
 * If defined, monsters stop to stun player and Mystics will stop using
 * their martial-arts..
 */
//#define SUPPRESS_MONSTER_STUN // HERESY !

/*
 * This one is more moderate; kick, punch, crush etc. still stuns, but
 * normal 'hit' doesn't.
 */
//#define NORMAL_HIT_NO_STUN

/* EXPERIMENTAL and actually not cool, breaking with the traditional system in
   which a hit is already 'mitigated' by armour simply by the fact that it
   misses. If enabled, the damage used for calculating monster critical hits
   is the one _before_ AC mitigation, making them much more common! This is
   required or Stormbringer would turn into a joke with this system.
   If this is disabled then we only allow this for base + poison + lite.
   Allow mitigating elemental damage by splitting it up into elemental and
   phsyical part if the attack was a melee attack? - C. Blue */
//#define EXPERIMENTAL_MITIGATION

/* NEW (and sort of experimental - it works well but it might make some
   monsters too easy compared to others).
   Allow additional damage mitigation by AC even if the effect is elemental,
   providing that the attack type indicates a phsyical attack? - C. Blue */
//#define ELEMENTAL_AC_MITIGATION

/* Let RBE_UN_POWER additionally drain MP from the player? */
//#define UNPOWER_DRAINS_MP


/*
 * Critical blow.  All hits that do 95% of total possible damage,
 * and which also do at least 20 damage, or, sometimes, N damage.
 * This is used only to determine "cuts" and "stuns".
 */
static int monster_critical(int dice, int sides, int dam) {
	int max = 0;
	int total = dice * sides;

	/* Must do at least 95% of perfect */
	if (dam < total * 19 / 20) return(0);

	/* Weak blows rarely work */
	if ((dam < 20) && (rand_int(100) >= dam)) return(0);

	/* Perfect damage */
	if (dam == total) max++;

	/* Super-charge */
	if (dam >= 40 && !rand_int(50)) max++;

	/* Critical damage */
	if (dam > 60) return(6 + max);
	if (dam > 45) return(5 + max);
	if (dam > 30) return(4 + max);
	if (dam > 18) return(3 + max);
	if (dam > 11) return(2 + max);
	return(1 + max);
}





/*
 * Determine if a monster attack against the player succeeds.
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match monster power against player armor.
 *
 * TODO: Add this kind of check for monsters' ranged physical attacks too (aka ARROW_x 'monster spells')!
 */
static int check_hit(int Ind, int power, int level, bool flag) {
	player_type *p_ptr = Players[Ind];
	int i, k, ac;

	/* Percentile dice */
	k = rand_int(100);
	/* Hack -- Always miss or hit */
	if (k < 10) return(k < 5);

	/* 'Blur' effect - miss more often */
	if ((i = get_skill_scale(p_ptr, SKILL_OSHADOW, 20)) && no_real_lite(Ind) && rand_int(level + 10) < i) return(FALSE);

	/* Calculate the "attack quality" */
	i = (power + (level * 3));
	/* Total armor */
	ac = (flag ? 0 : p_ptr->ac) + p_ptr->to_a;
	/* Power and Level compete against Armor */
#ifndef TO_AC_CAP_30
	if ((i > 0) && (randint(i) > ((ac * 3) / 4))) return(TRUE);
#else
	/* *3/4, *4/5, *5/6 and *6/7 are possible mainly:
	   3/4 would mean a slight nerf to AC values,
	   4/5 would mean a slight nerf to 'normal' AC values,
	   5/6 would mean a very slight buff to top-end AC values and very slight nerf to 'normal' values,
	   6/7 would mean a slight buff to top-end AC values.
	   However, note that *enchant* scrolls and shop enchantment service is indirectly buffed by TO_AC_CAP_30
	   and that low-ac players also look a bit better in comparison to higher-ac ones now.
	   All in all, 3/4 or 4/5 are the recommended values to use. */
	if ((i > 0) && (randint(i) > ((ac * 4) / 5))) return(TRUE);
#endif

	/* Assume miss */
	return(FALSE);
}



/*
 * Hack -- possible "insult" messages
 */
static cptr desc_insult[] =
{
	"insults you!",
	"insults your mother!",
	"gives you the finger!",
	"humiliates you!",
	"defiles you!",
	"dances around you!",
	"makes obscene gestures!",
	"moons you!!!"
};


/*
 * Hack -- possible "moan" messages
 */
static cptr desc_moan[] =
{
	"seems sad about something.",
	"asks if you have seen his dogs.",
	"tells you to get off his land.",
	"mumbles something about mushrooms."
};
/* for the Great Pumpkin on Halloween - C. Blue >:) */
cptr desc_moan_halloween[] =
{
	"screams: Trick or treat!",
	"says: Happy Halloween!",
	"moans loudly.",
	"says: Have you seen The Great Pumpkin?"
};

/* for the santa on christ^H^H^H^H^H^Hxmas */
static cptr desc_moan_xmas[] = {
	"wishes you a Merry Christmas!",
	"hopes that you will be less naughty next year!",
	"cheers \"Ho Ho ho!\"",
	"has a present for you!",
	"checks you against his cheeze list."
};

/*
 * Hack -- possible "seducing" messages
 */
#define MAX_WHISPERS	7
static cptr desc_whisper[] =
{
	"whispers something in your ear.",
	"asks if you have time to spare.",
	"gives you a charming wink.",
	"murmurs you sweetly.",
	"murmurs something about the night.",
	"woos you.",
	"courts you.",
};


/* returns 'TRUE' if the thief will blink away. */
static bool do_eat_gold(int Ind, int m_idx) {
	player_type *p_ptr = Players[Ind];
	monster_type    *m_ptr = &m_list[m_idx];
#if 0
	monster_race    *r_ptr = race_inf(m_ptr);
	int i, k;
#endif	// 0
	s32b            gold, available;

	if (safe_area(Ind)) return(TRUE);

	available = p_ptr->au - p_ptr->casino_wager;
	if (available < 0) return(TRUE); //paranoia

	gold = (available / 10) + randint(25);
	if (gold < 2) gold = 2;
	if (gold > 5000) gold = (available / 20) + randint(3000);
	if (gold > available) gold = available;


	p_ptr->au -= gold;
	if (gold <= 0) {
		msg_print(Ind, "Nothing was stolen.");
	} else if (p_ptr->au) {
		msg_print(Ind, "Your purse feels lighter.");
		msg_format(Ind, "\376\377o%d coins were stolen!", gold);
	} else {
		msg_print(Ind, "Your purse feels lighter.");
		msg_print(Ind, "\376\377oAll of your coins were stolen!");
	}

	/* Hack -- Consume some */
	if (magik(50)) gold = gold * rand_int(100) / 100;

	/* XXX simply one mass, in case player had huge amount of $ */
	while (gold > 0) {
		object_type forge, *j_ptr = &forge;

		/* Wipe the object */
		object_wipe(j_ptr);

		/* Prepare a gold object */
		invcopy(j_ptr, gold_colour(gold, FALSE, FALSE));

		/* Determine how much the treasure is "worth" */
#if 0
		j_ptr->pval = (gold >= 15000) ? 15000 : gold;
		gold -= 15000;
		/* fix gold 'colour' */
		j_ptr->k_idx = gold_amount(j_ptr->pval);
		j_ptr->sval = k_info[j_ptr->k_idx].sval;
#else
		j_ptr->pval = gold;
		gold = 0;
#endif

		monster_carry(m_ptr, m_idx, j_ptr);
	}

	/* Redraw gold */
	p_ptr->redraw |= (PR_GOLD);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Blink away */
	return(TRUE);
}


/* returns 'TRUE' if the thief will blink away. */
static bool do_eat_item(int Ind, int m_idx) {
	player_type *p_ptr = Players[Ind];
	monster_type    *m_ptr = &m_list[m_idx];
#if 0
	monster_race    *r_ptr = race_inf(m_ptr);
#endif	// 0
	object_type		*o_ptr;
	char			o_name[ONAME_LEN];
	int i, k;
#ifdef ENABLE_SUBINVEN
	int j = 0, l = 0; /* Init to kill (non-effective) compiler warning */
	object_type *os_ptr;
#endif

	/* Amulet of Immortality */
	//if (p_ptr->admin_invuln) return(FALSE);
	if (p_ptr->admin_invinc) return(FALSE);

	if (safe_area(Ind)) return(TRUE);

	/* Find an item */
	for (k = 0; k < 10; k++) {
		/* Pick an item */
		i = rand_int(INVEN_PACK);

		/* Obtain the item */
		o_ptr = &p_ptr->inventory[i];

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;

#ifdef ENABLE_SUBINVEN
		/* Don't steal subinventories, too annoying probably */
 #if 0
		if (o_ptr->tval == TV_SUBINVEN) continue;
 #else		/* Steal item from within a subinventory */
		if (o_ptr->tval == TV_SUBINVEN) {
			for (l = 0; l < 10; l++) {
				/* Pick an item */
				j = rand_int(o_ptr->bpval);

				/* Obtain the item */
				os_ptr = &p_ptr->subinventory[i][j];

				/* Skip empty slots */
				if (!os_ptr->k_idx) continue;

				o_ptr = os_ptr;
				l = i; /* Keep i just for easy access when displaying stealing message */
				i = (i + 1) * SUBINVEN_INVEN_MUL + j; /* Encode index for global inventory (inven+subinvens) */
				break;
			}
			/* Resume trying to steal from normal inventory */
			if (l == 10) continue;
		}
 #endif
#endif

		/* Don't steal artifacts  -CFT */
		if (artifact_p(o_ptr)) continue;

		/* Don't steal keys */
		if (!mon_allowed_pickup(o_ptr->tval)) continue;

		/* Get a description */
		object_desc(Ind, o_name, o_ptr, FALSE, 3);

		/* Message */
#ifdef ENABLE_SUBINVEN
		if (i >= SUBINVEN_INVEN_MUL) {
			char m_name[MNAME_LEN];

			msg_format(Ind, "\376\377o%sour %s (%c)(%c) was stolen!",
			    ((o_ptr->number > 1) ? "One of y" : "Y"),
			    o_name, index_to_label(l), index_to_label(j));

			monster_desc(Ind, m_name, m_idx, 0x1);
			s_printf("EAT_ITEM: %s from %s : %s (%c)(%c)\n",
			    m_name, p_ptr->name, o_name, index_to_label(l), index_to_label(j)); //may wrongly use o_name in plural but w/e
		} else
#endif
		{
			char m_name[MNAME_LEN];

			msg_format(Ind, "\376\377o%sour %s (%c) was stolen!",
			    ((o_ptr->number > 1) ? "One of y" : "Y"),
			    o_name, index_to_label(i));

			monster_desc(Ind, m_name, m_idx, 0x1);
			s_printf("EAT_ITEM: %s from %s : %s (%c)\n",
			    m_name, p_ptr->name, o_name, index_to_label(i)); //may wrongly use o_name in plural but w/e
		}

		/* Option */
#ifdef MONSTER_INVENTORY
		if (!o_ptr->questor) { /* questor items cannot be 'dropped', only destroyed! */
			int o_idx;

			/* Make an object */
			o_idx = o_pop();

			/* Success */
			if (o_idx) {
				object_type *j_ptr;

				/* Get new object */
				j_ptr = &o_list[o_idx];

				/* Copy object */
				object_copy(j_ptr, o_ptr);

				/* Modify number */
				j_ptr->number = 1;

				/* Hack -- If a wand, allocate total
				 * maximum timeouts or charges between those
				 * stolen and those missed. -LM-
				 */
				if (is_magic_device(o_ptr->tval)) divide_charged_item(j_ptr, o_ptr, 1);

				/* Forget mark */
				// j_ptr->marked = FALSE;

				/* Memorize monster */
				j_ptr->held_m_idx = m_idx;

				/* Build stack */
				j_ptr->next_o_idx = m_ptr->hold_o_idx;

				/* Build stack */
				m_ptr->hold_o_idx = o_idx;
			} else questitem_d(o_ptr, o_ptr->number); /* quest items are not immune to stealing :) */
		} else questitem_d(o_ptr, o_ptr->number); /* questors go poof */
#else
		if (is_magic_device(o_ptr->tval)) divide_charged_item(NULL, o_ptr, 1);
#endif	// MONSTER_INVENTORY

		/* Steal the items */
		inven_item_increase(Ind, i, -1);
		inven_item_optimize(Ind, i);

		/* Done */
		return(TRUE);
	}
	return(FALSE);
}

/* Have fun */
/* returns 'TRUE' if the partner will blink away. */
static bool do_seduce(int Ind, int m_idx) {
	player_type *p_ptr = Players[Ind];
	monster_type    *m_ptr = &m_list[m_idx];
	monster_race    *r_ptr = race_inf(m_ptr);
	object_type		*o_ptr;
	char	    m_name[MNAME_LEN];
	char		o_name[ONAME_LEN];
	int d, i, j, ty, tx, chance, crowd = 0, piece = 0;
	bool done = FALSE;
	u32b f1, f2, f3, f4, f5, f6, esp;
	cave_type **zcave, *c_ptr;

	if (!(zcave = getcave(&p_ptr->wpos))) return(FALSE);

	for (d = 1; d <= 9; d++) {
		if (d == 5) continue;

		tx = p_ptr->px + ddx[d];
		ty = p_ptr->py + ddy[d];

		if (!in_bounds(ty, tx)) continue;

		c_ptr = &zcave[ty][tx];
		if (c_ptr->m_idx) crowd++;
	}

	if (crowd > 1) {
		msg_print(Ind, "Two of you feel disturbed.");
		return(TRUE);
	}

	/* Get the monster name (or "it") */
	monster_desc(Ind, m_name, m_idx, 0);

	/* Hack -- borrow 'dig' table for CHR check */
	chance = adj_str_dig[p_ptr->stat_ind[A_CHR]];

	/* Hack -- presume hetero */
	if ((p_ptr->male && (r_ptr->flags1 & RF1_MALE)) ||
			(!p_ptr->male && (r_ptr->flags1 & RF1_FEMALE)))
		chance *= 8;

	/* Give up */
	if (chance > 100) return(TRUE);

	/* From what you'll undress? */
	d = rand_int(6);

	for (i = 0; i < 6; i++) {
		j = INVEN_BODY + ((i + d) % 6);

		/* Get the item to take off */
		o_ptr = &(p_ptr->inventory[j]);
		if (!o_ptr->k_idx) continue;

		/* Extract the flags */
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		/* Hack -- cannot take off, not counted */
		if (cursed_p(o_ptr)) continue;
		//if (f3 & TR3_PERMA_CURSE) continue;

		if (!magik(chance)) {
			/* Describe the result */
			object_desc(Ind, o_name, o_ptr, FALSE, 0);
			msg_format(Ind, "\376\377r%^s seduces you and you take off your %s...", m_name, o_name);
			inven_takeoff(Ind, j, 255, FALSE, TRUE);
			break;
		}

		piece++;
	}

	if (piece || magik(chance)) return(FALSE);
	if (p_ptr->chp < 4) {
		msg_print(Ind, "You're too tired..");
		return(TRUE);
	}

	msg_format(Ind, "You yield yourself to %s...", m_name);

	/* let's not make it too abusable.. */
	while (!done) {
		d = randint(9);
		switch (d) {
			case 1:
				if (!p_ptr->cmp) continue;
				msg_print(Ind, "You feel drained of energy!");
				p_ptr->cmp /= 2;
				done = TRUE;
				p_ptr->redraw |= PR_MANA;
				break;

			case 2:
				if (set_diseased(Ind, p_ptr->diseased + rand_int(40) + 40, -m_idx))
					msg_print(Ind, "Darn, you've caught a disease!");
				done = TRUE;
				break;

			case 3:
				if (p_ptr->sustain_con) continue;
				msg_print(Ind, "You feel drained of vigor!");
				do_dec_stat(Ind, A_CON, STAT_DEC_NORMAL);
				done = TRUE;
				break;

			case 4:
				/* No sustainance */
				msg_print(Ind, "You lost your mind!");
				dec_stat(Ind, A_WIS, 10, STAT_DEC_NORMAL);
				done = TRUE;
				break;

			case 5:
				if (p_ptr->chp < 4) continue;
				msg_print(Ind, "You feel so tired after this...");
				p_ptr->chp /= 2;
				done = TRUE;
				p_ptr->redraw |= PR_HP;
				break;

			case 6:
				msg_print(Ind, "You feel it was not so bad an experience.");
				if (!(p_ptr->mode & MODE_PVP)) gain_exp(Ind, r_ptr->mexp / 2);
				done = TRUE;
				break;

			case 7:
				msg_print(Ind, "Grr, Bastard!!");
				(void)do_eat_item(Ind, m_idx);
				done = TRUE;
				break;

			case 8:
				if (p_ptr->au < 100) continue;
				msg_print(Ind, "They charged you for this..");
				(void)do_eat_gold(Ind, m_idx);
				done = TRUE;
				break;

			case 9:
				if (p_ptr->csane >= p_ptr->msane) continue;
				msg_print(Ind, "You feel somewhat quenched in a sense.");
				heal_insanity(Ind, damroll(4,8));
				done = TRUE;
				break;
		}
	}

	return(TRUE);
}


/*
 * Attack a player via physical attacks.
 */
#define DISARM_SCATTER /* disarmed items that dropped to the floor but failed to land properly, don't get erased but scattered around the floor instead */
bool make_attack_melee(int Ind, int m_idx) {
	player_type *p_ptr = Players[Ind];
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);

	int	ap_cnt;
	int	mon_acid = 0, mon_fire = 0, blows_total = 0;

	int	i, j, k, tmp, ac, rlev, r_idx = m_ptr->r_idx;
#ifndef NEW_DODGING /* actually 'chance' is used a lot in this function -> FIXME */
	int chance;
#endif
	int	do_cut, do_stun, factor = 100;// blockchance, parrychance, malus;

	object_type *o_ptr;

	char	o_name[ONAME_LEN];
	char	m_name[MNAME_LEN], m_name_gen[MNAME_LEN];
	char	ddesc[MNAME_LEN];
	char	dam_msg[MAX_CHARS_WIDE] = { '\0' };

	bool	blinked;

	bool touched = FALSE, fear = FALSE, alive = TRUE;
	bool explode = FALSE, gone = FALSE;
	bool trigger_reactive = FALSE;

	bool player_vulnerable = FALSE;


	/* Not allowed to attack */
	if (r_ptr->flags2 & RF2_NEVER_BLOW) return(FALSE);

	/* Total armor */
	ac = p_ptr->ac + p_ptr->to_a;

	/* Extract the effective monster level */
	rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

	/* Extract the 'stun' factor */
	if (m_ptr->stunned > 50) factor -= 30;
	if (m_ptr->stunned) factor -= 20;

	/* Get the monster name (or "it") */
	monster_desc(Ind, m_name, m_idx, 0);
	monster_desc(Ind, m_name_gen, m_idx, 0x02);

	/* Get the "died from" information (i.e. "a kobold") */
	//hack: 'The' Horned Reaper from Dungeon Keeper ;)
	if (r_idx == RI_HORNED_REAPER_GE) monster_desc(Ind, ddesc, m_idx, 0x0180);
	else monster_desc(Ind, ddesc, m_idx, 0x0188);

	/* determine how much parrying or blocking would endanger our weapon/shield */
	for (i = 0; i < 4; i++) {
		if (r_ptr->blow[i].effect == RBE_ACID) mon_acid += 12;
		if (r_ptr->blow[i].effect == RBE_FIRE) mon_fire += 12;
		blows_total++;
	}
	if (blows_total) {
		mon_acid /= blows_total;
		mon_fire /= blows_total;
	}

	/* Assume no blink */
	blinked = FALSE;

	/* Scan through all four blows */
	for (ap_cnt = 0; ap_cnt < 4; ap_cnt++) {
#ifdef OLD_MONSTER_LORE
		bool visible = FALSE;
#endif
		bool obvious = FALSE;
		bool bypass_ac = FALSE;
		bool bypass_shield = FALSE;
		bool bypass_weapon = FALSE;

		bool bypass_protection = FALSE;
		bool invuln_applied = FALSE;

		int power = 0;
		int damage = 0, dam_ele = 0;
#ifdef EXPERIMENTAL_MITIGATION
		int damage_org = 0;
#endif

		bool attempt_block = FALSE, attempt_parry = FALSE;
		cptr act = NULL;

		/* Extract the attack infomation */
		int effect = m_ptr->blow[ap_cnt].effect;
		int method = m_ptr->blow[ap_cnt].method;
		int d_dice = m_ptr->blow[ap_cnt].d_dice;
		int d_side = m_ptr->blow[ap_cnt].d_side;


		/* Hack -- no more attacks */
		if (!method) break;

		/* Stop if player is dead or gone */
		if (p_ptr->suicided || p_ptr->death || p_ptr->new_level_flag) break;


#ifdef OLD_MONSTER_LORE
		/* Extract visibility (before blink) */
		if (p_ptr->mon_vis[m_idx]) visible = TRUE;
#endif

		touched = FALSE;
#ifndef TRIGGERS_REACTIVE_ONCEPERROUND
		trigger_reactive = FALSE;
#endif

		/* Extract the attack "power" */
		switch (effect) {
		case RBE_HURT:		power = 60; break;
		case RBE_POISON:	power =  5; break;
		case RBE_UN_BONUS:	power = 20; break;
		case RBE_UN_POWER:	power = 15; break;
		case RBE_EAT_GOLD:	power =  5; break;
		case RBE_EAT_ITEM:	power =  5; break;
		case RBE_EAT_FOOD:	power =  5; break;
		case RBE_EAT_LITE:	power =  5; break;
		case RBE_ACID:		power =  0; break;
		case RBE_ELEC:		power = 10; break;
		case RBE_FIRE:		power = 10; break;
		case RBE_COLD:		power = 10; break;
		case RBE_BLIND:		power =  2; break;
		case RBE_CONFUSE:	power = 10; break;
		case RBE_TERRIFY:	power = 10; break;
		case RBE_PARALYZE:	power =  2; break;
		case RBE_LOSE_STR:	power =  0; break;
		case RBE_LOSE_DEX:	power =  0; break;
		case RBE_LOSE_CON:	power =  0; break;
		case RBE_LOSE_INT:	power =  0; break;
		case RBE_LOSE_WIS:	power =  0; break;
		case RBE_LOSE_CHR:	power =  0; break;
		case RBE_LOSE_ALL:	power =  2; break;
		case RBE_SHATTER:	power = 60; break;
		case RBE_EXP_10:	power =  5; break;
		case RBE_EXP_20:	power =  5; break;
		case RBE_EXP_40:	power =  5; break;
		case RBE_EXP_80:	power =  5; break;
		case RBE_DISEASE:	power =  5; break;
		case RBE_TIME:		power =  5; break;
		case RBE_SANITY:	power = 60; break;
		case RBE_HALLU:		power = 10; break;
		case RBE_PARASITE:	power =  5; break;
		case RBE_DISARM:	power = 60; break;
		case RBE_FAMINE:	power = 20; break;
		case RBE_SEDUCE:	power = 80; break;
		case RBE_EAT_MANA:	power = 10; break;
		}

		/* Pre-check attack method for bypassing various means of defense */
		switch (method) {
		case RBM_GAZE://too crazy vs the big bosses..  if (p_ptr->blind) continue; /* :-D */
		case RBM_WAIL:
		case RBM_BEG:
		case RBM_INSULT:
		case RBM_MOAN:
		case RBM_SHOW:
		case RBM_WHISPER:
			bypass_protection = TRUE; /* mystic shield/shadow dispersion doesn't block light/sound waves */
			/* Fall through */
		case RBM_SPORE:
			bypass_shield = TRUE; /* can't be blocked */
			/* Fall through */
		case RBM_EXPLODE:
			bypass_ac = TRUE; /* means it always hits and can't be dodged or parried either.
					     Damage can still be reduced though (if RBE_HURT eg). */
			/* Fall through */
		case RBM_DROOL:
			bypass_weapon = TRUE; /* can't be parried */
			break;
		}

		/* Stair-GoI now also works on physical attacks! */
		if (p_ptr->invuln && (!bypass_invuln)) {// && !p_ptr->invuln_applied) {
			if (magik(40)) {
				msg_print(Ind, "The attack is fully deflected by a magic shield.");
				//if (who != PROJECTOR_TERRAIN) break_cloaking(Ind, 0);
				continue;
			}
			invuln_applied = TRUE;
		}

		if (!bypass_protection) {
			bool done = FALSE;

			/* Now here, taking precedence before almost anything else, even AC (check_hit()).
			   Only the 'outer' shields take precedence. */
			if (p_ptr->dispersion && p_ptr->cst) {
				msg_format(Ind, "\377%cYou disperse around %s attack!", COLOUR_DODGE_GOOD, m_name_gen);
				if (magik(p_ptr->dispersion)) use_stamina(p_ptr, 1);
				continue;
			}

			/* kinetic shield: (requires mana to work) */
			if (p_ptr->kinetic_shield
#ifdef ENABLE_OCCULT
			    || p_ptr->spirit_shield
#endif
			    ) {
				int cost = m_ptr->level / 10, clog, clog_dam;

				if (damage > 45) {
					clog_dam = (damage * 100) / 38;
					clog = 0;
					while ((clog_dam = (clog_dam * 10) / 12) >= 100) clog++;
					cost += clog;
					//adds +0 (up to 45 damage) ..+3 (70 dam) ..+7 (150 dam) ..+13 (500 dam) ..+17 (1000 dam, hypothetically)
				}
				if (p_ptr->cmp >= cost) {
					/* test if it repelled the blow */
					int chance = (p_ptr->lev * ((r_ptr->flags2 & RF2_POWERFUL) ? 67 : 100)) / r_ptr->level, max_chance;
					// ^= usually 50% chance vs all mobs you'd encounter at your level, 33% vs double-level POWERFUL

					/* drains mana on hit */
					p_ptr->cmp -= cost;
					p_ptr->redraw |= PR_MANA;

					/* spirit shield is usually weaker than kinetic shield */
#ifdef ENABLE_OCCULT
					if (p_ptr->kinetic_shield) max_chance = 50;
					else max_chance = p_ptr->spirit_shield_pow;
#else
					max_chance = 50;
#endif

					if (magik(chance > max_chance ? max_chance : chance)) {
						msg_format(Ind, "\377%c%^s is repelled.", COLOUR_DODGE_GOOD, m_name);
						continue;
					}

					/* assume that if kinetic shield failed, pfe will also fail: ... */
					if (!p_ptr->spirit_shield) done = TRUE;
				}
			}
			if (!done) {
				bool prot = FALSE;

				/* protection from evil, static and temporary: */
				if ((get_skill(p_ptr, SKILL_HDEFENSE) >= 30) && (r_ptr->flags3 & RF3_UNDEAD)) {
					if ((p_ptr->lev * 2 >= rlev) && (rand_int(100) + p_ptr->lev > 50 + rlev))
						prot = TRUE;
				} else if ((get_skill(p_ptr, SKILL_HDEFENSE) >= 40) && (r_ptr->flags3 & RF3_DEMON)) {
					if ((p_ptr->lev * 3 >= rlev * 2) && (rand_int(100) + p_ptr->lev > 50 + rlev))
						prot = TRUE;
				} else if ((get_skill(p_ptr, SKILL_HDEFENSE) >= 50) && (r_ptr->flags3 & RF3_EVIL)) {
					if ((p_ptr->lev * 3 >= rlev * 2) && (rand_int(100) + p_ptr->lev > 50 + rlev))
						prot = TRUE;
				}
				/* 'else' here to avoid crazy stacking */
				else if ((p_ptr->protevil > 0) && (r_ptr->flags3 & RF3_EVIL) &&
				    (((p_ptr->lev >= rlev) && ((rand_int(100) + p_ptr->lev) > 50)) || /* extra usefulness added (mostly for low levels): */
				     ((p_ptr->lev < rlev) && (p_ptr->lev + 10 >= rlev) && (rand_int(24) > 12 + rlev - p_ptr->lev)))) {
					prot = TRUE;
				}

				if (prot) {
#ifdef OLD_MONSTER_LORE
					/* Remember the Evil-ness */
					if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags3 |= RF3_EVIL;
#endif
					/* Message */
					msg_format(Ind, "\377%c%^s is repelled.", COLOUR_DODGE_GOOD, m_name);

					/* Hack -- Next attack */
					continue;
				}
			}
		}

		/* Monster hits player -- note: '!effect' check be Paranoia? */
		if (!effect || check_hit(Ind, power, rlev * factor / 100, bypass_ac)) {
			/* Always disturbing */
			disturb(Ind, 1, 0);
			/* Assume no cut or stun */
			do_cut = do_stun = 0;

			/* Describe the attack method */
			switch (method) {
			case RBM_HIT:
				act = "hits you";
#ifdef NORMAL_HIT_NO_STUN
				do_cut = 1;
#else
				do_cut = do_stun = 1;
#endif	// NORMAL_HIT_NO_STUN
				touched = TRUE;
				break;
			case RBM_TOUCH:
				act = "touches you";
				touched = TRUE;
				break;
			case RBM_PUNCH:
				act = "punches you";
				do_stun = 1;
				touched = TRUE;
				break;
			case RBM_KICK:
				act = "kicks you";
				do_stun = 1;
				touched = TRUE;
				break;
			case RBM_CLAW:
				act = "claws you";
				do_cut = 1;
				touched = TRUE;
				break;
			case RBM_BITE:
				act = "bites you";
				do_cut = 1;
				touched = TRUE;
				break;
			case RBM_STING:
				act = "stings you";
				touched = TRUE;
				break;
			case RBM_XXX1:
				act = "XXX1's you";
				break;
			case RBM_BUTT:
				act = "butts you";
				do_stun = 1;
				touched = TRUE;
				break;
			case RBM_CRUSH:
				act = "crushes you";
				do_stun = 1;
				touched = TRUE;
				break;
			case RBM_ENGULF:
				act = "engulfs you";
				touched = TRUE;
				break;
#if 0
			case RBM_XXX2:
				act = "XXX2's you";
				break;
#endif	// 0
			case RBM_CHARGE:
				act = "charges you";
				touched = TRUE;
#ifdef USE_SOUND_2010
#else
				//sound(SOUND_BUY); /* Note! This is "charges", not "charges at". */
#endif
				break;
			case RBM_CRAWL:
				act = "crawls on you";
				touched = TRUE;
				break;
			case RBM_DROOL:
				act = "drools on you";
				break;
			case RBM_SPIT:
				act = "spits on you";
				break;
#if 0
			case RBM_XXX3:
				act = "XXX3's on you";
				break;
#endif	// 0
			case RBM_EXPLODE:
				act = "explodes";
				explode = TRUE;
				break;
			case RBM_GAZE:
				act = "gazes at you";
				break;
			case RBM_WAIL:
				act = "wails at you";
				break;
			case RBM_SPORE:
				act = "releases spores at you";
				break;
			case RBM_XXX4:
				act = "projects XXX4's at you";
				break;
			case RBM_BEG:
				act = "begs you for money.";
				break;
			case RBM_INSULT:
				act = desc_insult[rand_int(8)];
				break;
			case RBM_MOAN:
				if (season_halloween)
					act = desc_moan_halloween[rand_int(4)];
				else if (season_xmas)
					act = desc_moan_xmas[rand_int(5)];
				else
					act = desc_moan[rand_int(4)];
				break;
#if 0
			case RBM_XXX5:
				act = "XXX5's you";
				break;
#endif	// 0
			case RBM_SHOW:
				if (randint(3) == 1)
					act = "sings 'We are a happy family'.";
				else
					act = "sings 'I love you, you love me'.";
#ifdef USE_SOUND_2010
#else
				//sound(SOUND_SHOW);
#endif
				break;
			case RBM_WHISPER:
				act = desc_whisper[rand_int(MAX_WHISPERS)];
				break;
			}

			/* Roll out the damage in advance, since it might be required in the avoidance-calcs below (Kinetic Shield) */
			damage = damroll(d_dice, d_side);
			if (r_idx == RI_MIRROR) damage = (damage * MIRROR_REDUCE_DAM_DEALT_MELEE + 99) / 100;
			if (invuln_applied) damage = (damage + 1) / 2;
#ifdef EXPERIMENTAL_MITIGATION
			damage_org = damage; /* as a special service, we allow the invuln reduction above to factor in first, to prevent getting ko'ed on stairs :-p */
#endif
#if 0
			/* to prevent cloaking mode from breaking (break_cloaking) if an attack didn't do damage */
			if (!d_dice && !d_side) no_dam = TRUE;
#endif

#ifndef NEW_DODGING
			chance = p_ptr->dodge_level - ((rlev * 5) / 6);
			//    - UNAWARENESS(p_ptr) * 2;
			/* always 10% chance to hit */
			if (chance > DODGE_CAP) chance = DODGE_CAP;
			if ((chance > 0) && !bypass_ac && magik(chance)) {
				msg_format(Ind, "\377%cYou dodge %s attack!", COLOUR_DODGE_GOOD, m_name_gen);
				continue;
			}
#else
			if (!bypass_ac && magik(apply_dodge_chance(Ind, rlev))) {
				msg_format(Ind, "\377%cYou dodge %s attack!", COLOUR_DODGE_GOOD, m_name_gen);
				continue;
			}
#endif

#if 0 /* let's make it more effective */
 #ifdef USE_BLOCKING
			/* Parry/Block - belongs to new-NR-viability changes */
			/* choose whether to attempt to block or to parry (can't do both at once),
			   50% chance each, except for if weapon is missing (anti-retaliate-inscription
			   has been left out, since if you want max block, you'll have to take off your weapon!) */
			if (p_ptr->shield_deflect && !bypass_shield &&
			    (!p_ptr->inventory[INVEN_WIELD].k_idx || magik(p_ptr->combat_stance == 1 ? 75 : 50))) {
				if (magik(apply_block_chance(p_ptr, p_ptr->shield_deflect))) {
					msg_format(Ind, "\377%cYou block %s attack.", COLOUR_BLOCK_GOOD, m_name_gen);
  #ifndef NEW_SHIELDS_NO_AC
					if (mon_acid + mon_fire) {
						if (randint(mon_acid + mon_fire) > mon_acid) {
							if (magik(5)) shield_takes_damage(Ind, GF_FIRE);
						} else {
							if (magik(10)) shield_takes_damage(Ind, GF_ACID);
						}
					}
  #endif
					continue;
				}
			}
 #endif
 #ifdef USE_PARRYING
			/* parrying */
			if (p_ptr->weapon_parry && !bypass_weapon) {
				if (magik(apply_parry_chance(p_ptr, p_ptr->weapon_parry))) {
					int slot = INVEN_WIELD;

					if (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD && magik(50)) /* dual-wield? */
						slot = INVEN_ARM;
					msg_format(Ind, "\377%cYou parry %s attack.", COLOUR_PARRY_GOOD, m_name_gen);
					if (mon_acid + mon_fire) {
						if (randint(mon_acid + mon_fire) > mon_acid) {
							if (magik(3)) weapon_takes_damage(Ind, GF_FIRE, slot);
						} else {
							if (magik(5)) weapon_takes_damage(Ind, GF_ACID, slot);
						}
					}
					continue;
				}
			}
 #endif
#else
			/* first, choose preferred defense: parry or block, what we can do better */
			if (p_ptr->shield_deflect && !bypass_shield && p_ptr->shield_deflect >= p_ptr->weapon_parry) {
				if (magik(75) || /* sometimes you're just not in position to use the preferred method */
				    !p_ptr->inventory[INVEN_WIELD].k_idx) /* if you don't wield a weapon, you get easier opportunity to swing the shield around you */
					attempt_block = TRUE;
				else if (p_ptr->weapon_parry && !bypass_weapon) attempt_parry = TRUE;
			} else if (p_ptr->weapon_parry && !bypass_weapon) {
				if (magik(75) || /* sometimes you're just not in position to use the preferred method */
				    !p_ptr->inventory[INVEN_ARM].k_idx || p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) /* if you don't wield a shield (1 weapon or dual-wield), you get easier opportunity to swing the shield around you */
					attempt_parry = TRUE;
				else if (p_ptr->shield_deflect && !bypass_shield) attempt_block = TRUE;
			}
 #ifdef USE_BLOCKING
			if (attempt_block && magik(apply_block_chance(p_ptr, p_ptr->shield_deflect))) {
				msg_format(Ind, "\377%cYou block %s attack.", COLOUR_BLOCK_GOOD, m_name_gen);
  #ifndef NEW_SHIELDS_NO_AC
				if (mon_acid + mon_fire) {
					if (randint(mon_acid + mon_fire) > mon_acid) {
						if (magik(5)) shield_takes_damage(Ind, GF_FIRE);
					} else {
						if (magik(10)) shield_takes_damage(Ind, GF_ACID);
					}
				}
  #endif
				continue;
			}
 #endif
 #ifdef USE_PARRYING
			if (attempt_parry && magik(apply_parry_chance(p_ptr, p_ptr->weapon_parry))) {
				int slot = INVEN_WIELD;

				if (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD && magik(50)) /* dual-wield? */
					slot = INVEN_ARM;
				msg_format(Ind, "\377%cYou parry %s attack.", COLOUR_PARRY_GOOD, m_name_gen);
				if (mon_acid + mon_fire) {
					if (randint(mon_acid + mon_fire) > mon_acid) {
						if (magik(3)) weapon_takes_damage(Ind, GF_FIRE, slot);
					} else {
						if (magik(5)) weapon_takes_damage(Ind, GF_ACID, slot);
					}
				}
				continue;
			}
 #endif
#endif

			/* Message */
			/* DEG Modified to give damage number */

			if ((act) && (damage < 1)) {
				if (method == RBM_MOAN) { /* no trailing dot */
					/* Colour change for ranged MOAN for Halloween event -C. Blue */
					if (season_halloween)
						msg_format(Ind, "\377o%^s %s", m_name, act);
					else
						msg_format(Ind, "%^s %s", m_name, act);
				} else if (method == RBM_SHOW || method == RBM_WHISPER || method == RBM_INSULT)
					msg_format(Ind, "%^s %s", m_name, act);
				else /* trailing dot */
					msg_format(Ind, "%^s %s.", m_name, act);
				strcpy(dam_msg, ""); /* suppress 'bla hits you for x dam' message! */
			} else if ((act) && (r_ptr->flags1 & (RF1_UNIQUE))) {
				//msg_format(Ind, "%^s %s for \377f%d \377wdamage.", m_name, act, damage);
				//sprintf(dam_msg, "%^s %s for \377f%%d \377wdamage.", m_name, act);
				sprintf(dam_msg, "%s %s for \377f%%d \377wdamage.", m_name, act);
			} else if (act) {
				//msg_format(Ind, "%^s %s for \377r%d \377wdamage.", m_name, act, damage);
				//sprintf(dam_msg, "%^s %s for \377r%%d \377wdamage.", m_name, act);
				sprintf(dam_msg, "%s %s for \377r%%d \377wdamage.", m_name, act);
			}
			dam_msg[0] = toupper(dam_msg[0]);


			/* The undead can give the player the Black Breath with
			 * a sucessful blow. Uniques have a better chance. -LM-
			 * Nazgul have a 25% chance
			 */
/* if (!p_ptr->protundead) {}		// more efficient way :)	*/
		/* I believe the previous version was wrong - evileye */

			/* If the player uses neither a weapon nor a shield
			   he is very defenseless against black breath infection
			   since he can neither block nor parry. */
			if (!p_ptr->inventory[INVEN_WIELD].k_idx &&
			    !p_ptr->inventory[INVEN_ARM].k_idx) player_vulnerable = TRUE;

			if (!p_ptr->black_breath &&
			    !safe_area(Ind) && /* just for Arena Monster Challenge! */
			    r_idx != RI_PUMPKIN && /* The Great Pumpkin of Halloween event shouldn't give BB, lol. -C. Blue */
			    (!p_ptr->suscep_life || !rand_int(3)) && (
			     ((r_ptr->flags7 & RF7_NAZGUL) && magik(player_vulnerable ? 10 : 25)) || (
			     (r_ptr->flags3 & RF3_UNDEAD) && m_ptr->ego != 32 /* not 'Purified' ego */
			     && (
			      (m_ptr->level >= 35 && (r_ptr->flags1 & RF1_UNIQUE) && randint(300 - m_ptr->level) == 1) ||
			      (m_ptr->level >= 40 && randint(400 - m_ptr->level - (player_vulnerable ? 75 : 0)) == 1)
			    ))))
				set_black_breath(Ind, -m_idx);

			/* Hack -- assume all attacks are obvious */
			obvious = TRUE;

			/* Set global melee-registration variable in case of MELEE_HIT_DRAINS_SHIELD (default: unused) */
			if (touched) melee_hit = TRUE;

				/* Apply appropriate damage */
			switch (effect) {
				case 0:
					/* Hack -- Assume obvious */
					obvious = TRUE;

					/* Hack -- No damage */
					damage = 0;

					break;

				case RBE_HURT:
					/* Obvious */
					obvious = TRUE;

					/* Hack -- Player armor reduces total damage */
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / AC_CAP_DIV);

					/* Take damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					break;

				case RBE_POISON:
					/* Special damage */
					dam_ele = damage;
					if (p_ptr->immune_poison) dam_ele = 0;
					if (p_ptr->suscep_pois) dam_ele = dam_ele * 2;
					if (p_ptr->resist_pois) dam_ele = (dam_ele + 2) / 3;
					if (p_ptr->oppose_pois) dam_ele = (dam_ele + 2) / 3;

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
#ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb (let's keep Osyluths etc dangerous!) */
#endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;

					/* Take some damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Take "poison" effect. Duration range scale: n->3n */
					if (!(p_ptr->resist_pois || p_ptr->oppose_pois || p_ptr->immune_poison)) {
						if (set_poisoned(Ind, p_ptr->poisoned + 3 + rlev / 4 + damroll(3, rlev / 6), -m_idx))
							obvious = TRUE;
					}

					/* Learn about the player */
					update_smart_learn(Ind, m_idx, DRS_POIS);

					break;

				case RBE_UN_BONUS:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage;
					if (p_ptr->resist_disen) dam_ele = (dam_ele * 6) / (randint(6) + 6);

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take some damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Allow complete resist */
					if (!p_ptr->resist_disen) {
						/* Apply disenchantment */
						if (apply_disenchant(Ind, 0)) obvious = TRUE;
					}

					/* Learn about the player */
					update_smart_learn(Ind, m_idx, DRS_DISEN);

					break;

				case RBE_UN_POWER:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take some damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					if (safe_area(Ind)) break;

#if POLY_RING_METHOD == 1
					/* discharge player's live form he attained from a
					   ring of polymorphing! (anti-cheeze for Morgoth) */
					if (!p_ptr->martyr
					    && p_ptr->tim_mimic && p_ptr->body_monster == p_ptr->tim_mimic_what /* only if you are currently using that form */
 #ifdef ENABLE_HELLKNIGHT
					    && p_ptr->pclass != CLASS_HELLKNIGHT
  #ifdef ENABLE_CPRIEST
					    && p_ptr->pclass != CLASS_CPRIEST
  #endif
 #endif
					    ) {
						if (p_ptr->tim_mimic >= 1000) {
							msg_print(Ind, "\377LThe magical force stabilizing your form fades *rapidly*...");
							set_mimic(Ind, (p_ptr->tim_mimic * 3) / 4, p_ptr->tim_mimic_what);
						}
						else {
							msg_print(Ind, "\377LThe magical force stabilizing your form fades *rapidly*...");
							set_mimic(Ind, p_ptr->tim_mimic - 200, p_ptr->tim_mimic_what);
						}
					}
#endif

					/* Saving throw from discharge resistance - anti-cheeze: for now applied _after_ above's tim_mimic discharge */
					if (p_ptr->resist_discharge && !magik(r_ptr->level > p_ptr->lev + 20 ? r_ptr->level - p_ptr->lev : 20)) break;

					/* Saving throw by Magic Device skill (9%..99%) */
					if (magik(117 - (2160 / (20 + get_skill_scale(p_ptr, SKILL_DEVICE, 100))))) break;

					/* Find an item */
					for (k = 0; k < 20; k++) {
						/* Pick an item */
#if POLY_RING_METHOD == 0
						i = rand_int(INVEN_PACK + 2);
						/* Inventory + the ring slots (timed polymorph rings) */
						if (i == INVEN_PACK) i = INVEN_LEFT;
						if (i == INVEN_PACK + 1) i = INVEN_RIGHT;
#else
						i = rand_int(INVEN_PACK);
#endif

						/* Obtain the item */
						o_ptr = &p_ptr->inventory[i];

						/* Drain charged wands/staffs/rods/polyrings -- what about activatable items? */
						if (((o_ptr->tval == TV_STAFF || o_ptr->tval == TV_WAND) && o_ptr->pval) ||
						    o_ptr->tval == TV_ROD ||
#ifdef MSTAFF_MDEV_COMBO
						    (o_ptr->tval == TV_MSTAFF &&
						    (((o_ptr->xtra1 || o_ptr->xtra2) && o_ptr->pval) || o_ptr->xtra3)) ||
#endif
						    (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_POLYMORPH && o_ptr->timeout_magic > 1)
						    ) {
							s16b chance = rand_int(get_skill_scale(p_ptr, SKILL_DEVICE, 80));

							/* Message */
							if (i < INVEN_PACK)
								msg_print(Ind, "Energy drains from your pack!");
							else
								msg_print(Ind, "Energy drains from your equipment!");

							/* Obvious */
							obvious = TRUE;

							/* Heal */
							j = rlev;
							if (o_ptr->tval == TV_RING)
								m_ptr->hp += j * (o_ptr->timeout_magic / 2000) * o_ptr->number;
							else if (o_ptr->tval == TV_ROD)
								m_ptr->hp += j * (k_info[o_ptr->k_idx].level >> 3) * o_ptr->number;
							else
								m_ptr->hp += j * o_ptr->pval * o_ptr->number;
							if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

							/* Redraw (later) if needed */
							update_health(m_idx);

							/* Uncharge */
							if (o_ptr->tval == TV_RING) {
								if (i < INVEN_PACK) {
									o_ptr->timeout_magic = 1; /* leave 0 to the dungeon.c routine.. */
								} else if (magik(chance)) {
									if (o_ptr->timeout_magic > 8000) o_ptr->timeout_magic -= (o_ptr->timeout_magic / 100);
									else if (o_ptr->timeout_magic > 2000) o_ptr->timeout_magic -= (o_ptr->timeout_magic / 50);
									else if (o_ptr->timeout_magic > 500) o_ptr->timeout_magic -= (o_ptr->timeout_magic / 25);
									else o_ptr->timeout_magic = 1;
								} else {
									if (o_ptr->timeout_magic > 8000) o_ptr->timeout_magic -= (o_ptr->timeout_magic / 20);
									else if (o_ptr->timeout_magic > 2000) o_ptr->timeout_magic -= (o_ptr->timeout_magic / 10);
									else if (o_ptr->timeout_magic > 500) o_ptr->timeout_magic -= (o_ptr->timeout_magic / 5);
									else o_ptr->timeout_magic = 1;
								}
							} else if (o_ptr->tval == TV_ROD) {
								if (magik(chance)) discharge_rod(o_ptr, 5 + rand_int(5));
								else discharge_rod(o_ptr, 15 + rand_int(10));
							} else {
								/* Pfft, this is really sucky... -,- MD should have something to
								 * do with it, at least... Will change it to 1_in_MDlev chance of
								 * total draining. Otherwise we will decrement. the_sandman
								 * - actual idea was to make ELEC IMM worth something btw =-p - C. Blue
								 ===> Note: MD skill and resist_discharge (from UN_POWER forms) indeed are preventing this now! */
								if (magik(chance)) o_ptr->pval--;
								else o_ptr->pval = 0;
							}

							/* Combine / Reorder the pack */
							p_ptr->notice |= (PN_COMBINE | PN_REORDER);

							/* Window stuff */
							p_ptr->window |= (PW_INVEN | PW_EQUIP);

							/* Done */
							break;
						}
					}

#ifdef UNPOWER_DRAINS_MP
					if (p_ptr->cmp) {
						p_ptr->cmp -= (p_ptr->mhp * (r_ptr->level + 30)) / 600;
						if (p_ptr->cmp < 0) p_ptr->cmp = 0;
						p_ptr->redraw |= PR_MANA;
						if (!obvious) msg_print(Ind, "Your psychic energy gets drained!");
					}
#endif

					break;

				case RBE_EAT_GOLD:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage -- we assume the 'elemental' part stands for the attack
					   being sort of a critical hit that doesn't allow AC mitigation! >;-D */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take some damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Obvious */
					obvious = TRUE;

					/* Saving throw (unless paralyzed) based on dex and level */
#if 0
					if (!p_ptr->paralyzed &&
					    (rand_int(100 + UNAWARENESS(p_ptr)) <
						 (adj_dex_safe[p_ptr->stat_ind[A_DEX]] + p_ptr->lev)))
#else	// 0
					if (!p_ptr->paralyzed &&
					    (rand_int(100 + UNAWARENESS(p_ptr)) <
						 (adj_dex_safe[p_ptr->stat_ind[A_DEX]] +
						  (p_ptr->rogue_heavyarmor ? 0 : get_skill(p_ptr, SKILL_STEALING))
 #ifdef ENABLE_STANCES
						  + (p_ptr->combat_stance == 1 ? 15 + p_ptr->combat_stance_power * 3 : 0)
 #endif
						  )))
#endif	// 0
					{
						/* Saving throw message */
						msg_print(Ind, "You quickly protect your money pouch!");

						/* Occasional blink anyway */
						if (rand_int(3)) blinked = TRUE;
					}
#ifndef TOOL_NOTHEFT_COMBO
					else if (TOOL_EQUIPPED(p_ptr) == SV_TOOL_MONEY_BELT && magik(100)) {
#else
					else if (TOOL_EQUIPPED(p_ptr) == SV_TOOL_THEFT_PREVENTION && magik(TOOL_SAFETY_CHANCE)) {
#endif
						/* Saving throw message */
						msg_print(Ind, "Your money was secured!");

						/* Occasional blink anyway */
						if (rand_int(3)) blinked = TRUE;
					}

					/* Eat gold */
					else blinked = do_eat_gold(Ind, m_idx);

					break;

				case RBE_EAT_ITEM:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage -- we assume the 'elemental' part stands for the attack
					   being sort of a critical hit that doesn't allow AC mitigation! >;-D */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take some damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Saving throw (unless paralyzed) based on dex and level */
#if 0
					if (!p_ptr->paralyzed &&
							(rand_int(100 + UNAWARENESS(p_ptr)) <
							 (adj_dex_safe[p_ptr->stat_ind[A_DEX]] + p_ptr->lev)))
#else	// 0
					if (!p_ptr->paralyzed &&
					    (rand_int(100 + UNAWARENESS(p_ptr)) <
						 (adj_dex_safe[p_ptr->stat_ind[A_DEX]] +
						  (p_ptr->rogue_heavyarmor ? 0 : get_skill(p_ptr, SKILL_STEALING))
 #ifdef ENABLE_STANCES
						  + (p_ptr->combat_stance == 1 ? 15 + p_ptr->combat_stance_power * 3 : 0)
 #endif
						  )))
#endif	// 0
					{
						/* Saving throw message */
						msg_print(Ind, "You grab hold of your backpack!");

						/* Occasional "blink" anyway */
						blinked = TRUE;

						/* Obvious */
						obvious = TRUE;

						/* Done */
						break;
					}

#ifndef TOOL_NOTHEFT_COMBO
					else if (TOOL_EQUIPPED(p_ptr) == SV_TOOL_THEFT_PREVENTION && magik(100)) { //80
#else
					else if (TOOL_EQUIPPED(p_ptr) == SV_TOOL_THEFT_PREVENTION && magik(TOOL_SAFETY_CHANCE)) {
#endif
						/* Saving throw message */
						msg_print(Ind, "Your backpack was secured!");

						/* Occasional "blink" anyway */
						blinked = TRUE;

						/* Obvious */
						obvious = TRUE;

						/* Done */
						break;
					}

					else blinked = obvious = do_eat_item(Ind, m_idx);
					break;

				case RBE_EAT_FOOD:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage -- we assume the 'elemental' part stands for the attack
					   being sort of a critical hit that doesn't allow AC mitigation! >;-D */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take some damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					if (safe_area(Ind)) break;

					/* Steal some food */
					for (k = 0; k < 10; k++) {
						/* Pick an item from the pack */
						i = rand_int(INVEN_PACK);

						/* Get the item */
						o_ptr = &p_ptr->inventory[i];

						/* Accept real items */
						if (!o_ptr->k_idx) continue;

						/* Only eat food */
						if (o_ptr->tval != TV_FOOD) continue;

						/* Get a description */
						object_desc(Ind, o_name, o_ptr, FALSE, 0);

						/* Message */
						msg_format(Ind, "%sour %s (%c) was eaten!",
							   ((o_ptr->number > 1) ? "One of y" : "Y"),
							   o_name, index_to_label(i));

						/* Steal the items */
						inven_item_increase(Ind, i, -1);
						inven_item_optimize(Ind, i);

						/* Obvious */
						obvious = TRUE;

						/* Done */
						break;
					}

					break;

				case RBE_EAT_LITE:
					/* Access the lite */
					o_ptr = &p_ptr->inventory[INVEN_LITE];

#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage -- we assume the 'elemental' part stands for the attack
					   being sort of a critical hit that doesn't allow AC mitigation! >;-D */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* Reduce the eat-lite damage part if we don't have (much) light */
					if (dam_ele > o_ptr->timeout / 250) dam_ele = p_ptr->cmp;

					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take some damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					if (safe_area(Ind)) break;

					/* Drain fuel */
					//if ((o_ptr->pval > 0) && (o_ptr->sval < SV_LITE_DWARVEN))
					if (o_ptr->timeout > 0) { // hope this won't cause trouble..
						/* Reduce fuel */
						o_ptr->timeout -= (250 + randint(250)) * damage;
						if (o_ptr->timeout < 1) o_ptr->timeout = 1;

						/* Notice */
						if (!p_ptr->blind) {
							msg_print(Ind, "Your light dims.");
							obvious = TRUE;
						}

						/* Window stuff */
						p_ptr->window |= (PW_INVEN | PW_EQUIP);
					}

					break;

				case RBE_ACID:
					/* Obvious */
					obvious = TRUE;

					/* Message */
					msg_print(Ind, "You are covered in acid!");

					/* Special damage */
					dam_ele = acid_dam(Ind, damage, ddesc, -m_idx); /* suffer strong elemental effects */
					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
#ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
#endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;

					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Learn about the player */
					update_smart_learn(Ind, m_idx, DRS_ACID);

					break;

				case RBE_ELEC:
					/* Obvious */
					obvious = TRUE;

					/* Message */
					msg_print(Ind, "You are struck by electricity!");

					/* Special damage */
					dam_ele = elec_dam(Ind, damage, ddesc, -m_idx); /* suffer strong elemental effects */
					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
#ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
#endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;

					if (apply_discharge(Ind, dam_ele)) obvious = TRUE;

					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);

					take_hit(Ind, damage, ddesc, -m_idx);

					/* Learn about the player */
					update_smart_learn(Ind, m_idx, DRS_ELEC);

					break;

				case RBE_FIRE:
					/* Obvious */
					obvious = TRUE;

					/* Message */
					msg_print(Ind, "You are enveloped in flames!");

					/* Special damage */
					dam_ele = fire_dam(Ind, damage, ddesc, -m_idx); /* suffer strong elemental effects */
					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
#ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
#endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;

					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Learn about the player */
					update_smart_learn(Ind, m_idx, DRS_FIRE);

					break;

				case RBE_COLD:
					/* Obvious */
					obvious = TRUE;

					/* Message */
					msg_print(Ind, "You are covered with frost!");

					/* Special damage */
					dam_ele = cold_dam(Ind, damage, ddesc, -m_idx); /* suffer strong elemental effects */
					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
#ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
#endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;

					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Learn about the player */
					update_smart_learn(Ind, m_idx, DRS_COLD);

					break;

				case RBE_BLIND:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage -- we assume the 'elemental' part stands for the attack
					   being sort of a critical hit that doesn't allow AC mitigation! >;-D */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Increase "blind" */
					if (!p_ptr->resist_blind) {
						if (set_blind(Ind, p_ptr->blind + 10 + randint(rlev)))
							obvious = TRUE;
					}

					/* Learn about the player */
					update_smart_learn(Ind, m_idx, DRS_BLIND);

					break;

				case RBE_CONFUSE:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage -- we assume the 'elemental' part stands for the attack
					   being sort of a critical hit that doesn't allow AC mitigation! >;-D */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Increase "confused" */
					if (!p_ptr->resist_conf) {
						if (set_confused(Ind, p_ptr->confused + 3 + randint(rlev)))
							obvious = TRUE;
					}

					/* Learn about the player */
					update_smart_learn(Ind, m_idx, DRS_CONF);

					break;

				case RBE_TERRIFY:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage -- we assume the 'elemental' part stands for the attack
					   being sort of a critical hit that doesn't allow AC mitigation! >;-D */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Increase "afraid" */
					if (p_ptr->resist_fear) {
						msg_print(Ind, "You stand your ground!");
						obvious = TRUE;
					} else if (rand_int(100) < p_ptr->skill_sav) {
						msg_print(Ind, "You stand your ground!");
						obvious = TRUE;
					} else {
						if (set_afraid(Ind, p_ptr->afraid + 3 + randint(rlev)))
							obvious = TRUE;
					}

					/* Learn about the player */
					update_smart_learn(Ind, m_idx, DRS_FEAR);

					break;

				case RBE_PARALYZE:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage -- we assume the 'elemental' part stands for the attack
					   being sort of a critical hit that doesn't allow AC mitigation! >;-D */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Increase "paralyzed" */
					if (p_ptr->free_act) {
						msg_print(Ind, "You are unaffected!");
						obvious = TRUE;
					} else if (rand_int(100) < p_ptr->skill_sav) {
						msg_print(Ind, "You resist the effects!");
						obvious = TRUE;
					} else {
						if (set_paralyzed(Ind, p_ptr->paralyzed + 3 + randint(rlev)))
							obvious = TRUE;
					}

					/* Learn about the player */
					update_smart_learn(Ind, m_idx, DRS_FREE);

					break;

				case RBE_LOSE_STR:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Damage (physical) */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Damage (stat) */
					if (do_dec_stat(Ind, A_STR, STAT_DEC_NORMAL)) obvious = TRUE;

					break;

				case RBE_LOSE_INT:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Damage (physical) */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Damage (stat) */
					if (do_dec_stat(Ind, A_INT, STAT_DEC_NORMAL)) obvious = TRUE;

					break;

				case RBE_LOSE_WIS:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Damage (physical) */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Damage (stat) */
					if (do_dec_stat(Ind, A_WIS, STAT_DEC_NORMAL)) obvious = TRUE;

					break;

				case RBE_LOSE_DEX:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Damage (physical) */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Damage (stat) */
					if (do_dec_stat(Ind, A_DEX, STAT_DEC_NORMAL)) obvious = TRUE;

					break;

				case RBE_LOSE_CON:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Damage (physical) */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Damage (stat) */
					if (do_dec_stat(Ind, A_CON, STAT_DEC_NORMAL)) obvious = TRUE;

					break;

				case RBE_LOSE_CHR:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Damage (physical) */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Damage (stat) */
					if (do_dec_stat(Ind, A_CHR, STAT_DEC_NORMAL)) obvious = TRUE;

					break;

				case RBE_LOSE_ALL:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //no resistance, and very nasty magic damage

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 33% physical, 66% 'elemental' */
						dam_ele = (dam_ele * 2) / 3;
						damage /= 3;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Damage (physical) */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Damage (stats) */
					if (do_dec_stat(Ind, A_STR, STAT_DEC_NORMAL)) obvious = TRUE;
					if (do_dec_stat(Ind, A_DEX, STAT_DEC_NORMAL)) obvious = TRUE;
					if (do_dec_stat(Ind, A_CON, STAT_DEC_NORMAL)) obvious = TRUE;
					if (do_dec_stat(Ind, A_INT, STAT_DEC_NORMAL)) obvious = TRUE;
					if (do_dec_stat(Ind, A_WIS, STAT_DEC_NORMAL)) obvious = TRUE;
					if (do_dec_stat(Ind, A_CHR, STAT_DEC_NORMAL)) obvious = TRUE;

					break;

				case RBE_SHATTER:
					/* Obvious */
					obvious = TRUE;

					/* Hack -- Reduce damage based on the player armor class */
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / AC_CAP_DIV);

					/* Take damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Radius 8 earthquake centered at the monster */
					/* Morgoth overrides LF1_NO_DESTROY. Note: This also works for Hru at these values. */
					if (damage > 23 || (damage && !rand_int(25 - damage))) {
						if (r_idx == RI_MORGOTH) override_LF1_NO_DESTROY = TRUE;
						earthquake(&p_ptr->wpos, m_ptr->fy, m_ptr->fx, 8);
					}
					break;

				case RBE_EXP_10:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //irresistible (could add hold-life and/or nether-res, but makes bloodletters too weak maybe?)

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Obvious */
					obvious = TRUE;

					/* Take damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					if (p_ptr->keep_life) msg_print(Ind, "You are unaffected!");
#ifdef NECROMANCY_HOLDS_LIFE
					else if (rand_int(100) < get_skill(p_ptr, SKILL_NECROMANCY))
						msg_print(Ind, "You keep hold of your life force!");
#endif
					else if (p_ptr->hold_life && (rand_int(100) < 95))
						msg_print(Ind, "You keep hold of your life force!");
					else {
						s32b d = damroll(10, 6) + (p_ptr->exp / 100) * MON_DRAIN_LIFE;

						if (p_ptr->hold_life) {
							msg_print(Ind, "You feel your life slipping away!");
							lose_exp(Ind, d / 10);
						} else {
							msg_print(Ind, "You feel your life draining away!");
							lose_exp(Ind, d);
						}
					}
					break;

				case RBE_EXP_20:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //irresistible (could add hold-life and/or nether-res, but makes bloodletters too weak maybe?)

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Obvious */
					obvious = TRUE;

					/* Take damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					if (p_ptr->keep_life) msg_print(Ind, "You are unaffected!");
#ifdef NECROMANCY_HOLDS_LIFE
					else if (rand_int(100) < get_skill(p_ptr, SKILL_NECROMANCY))
						msg_print(Ind, "You keep hold of your life force!");
#endif
					else if (p_ptr->hold_life && (rand_int(100) < 90))
						msg_print(Ind, "You keep hold of your life force!");
					else {
						s32b d = damroll(20, 6) + (p_ptr->exp / 100) * MON_DRAIN_LIFE;

						if (p_ptr->hold_life) {
							msg_print(Ind, "You feel your life slipping away!");
							lose_exp(Ind, d / 10);
						} else {
							msg_print(Ind, "You feel your life draining away!");
							lose_exp(Ind, d);
						}
					}
					break;

				case RBE_EXP_40:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //irresistible (could add hold-life and/or nether-res, but makes bloodletters too weak maybe?)

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Obvious */
					obvious = TRUE;

					/* Take damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					if (p_ptr->keep_life) msg_print(Ind, "You are unaffected!");
#ifdef NECROMANCY_HOLDS_LIFE
					else if (rand_int(100) < get_skill(p_ptr, SKILL_NECROMANCY))
						msg_print(Ind, "You keep hold of your life force!");
#endif
					else if (p_ptr->hold_life && (rand_int(100) < 75))
						msg_print(Ind, "You keep hold of your life force!");
					else {
						s32b d = damroll(40, 6) + (p_ptr->exp / 100) * MON_DRAIN_LIFE;

						if (p_ptr->hold_life) {
							msg_print(Ind, "You feel your life slipping away!");
							lose_exp(Ind, d / 10);
						} else {
							msg_print(Ind, "You feel your life draining away!");
							lose_exp(Ind, d);
						}
					}
					break;

				case RBE_EXP_80:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //irresistible (could add hold-life and/or nether-res, but makes bloodletters too weak maybe?)

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Obvious */
					obvious = TRUE;

					/* Take damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					if (p_ptr->keep_life) msg_print(Ind, "You are unaffected!");
#ifdef NECROMANCY_HOLDS_LIFE
					else if (rand_int(100) < get_skill(p_ptr, SKILL_NECROMANCY))
						msg_print(Ind, "You keep hold of your life force!");
#endif
					else if (p_ptr->hold_life && (rand_int(100) < 50))
						msg_print(Ind, "You keep hold of your life force!");
					else {
						s32b d = damroll(80, 6) + (p_ptr->exp / 100) * MON_DRAIN_LIFE;

						if (p_ptr->hold_life) {
							msg_print(Ind, "You feel your life slipping away!");
							lose_exp(Ind, d / 10);
						} else {
							msg_print(Ind, "You feel your life draining away!");
							lose_exp(Ind, d);
						}
					}
					break;

				/* Additisons from PernAngband	- Jir - */
				case RBE_DISEASE:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take some damage */
					//carried_monster_hit = TRUE;
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Duration range scale: n->3n */
					if (set_diseased(Ind, p_ptr->diseased + 3 + rlev / 4 + damroll(3, rlev / 6), -m_idx)) {
						msg_print(Ind, "You caught a disease!");
						obvious = TRUE;
					}

					/* Damage CON (10% chance)*/
					if (randint(100) < 11) {
						/* 1% chance for perm. damage (pernA one was buggie? */
						bool perm = (randint(10) == 1);

						if (dec_stat(Ind, A_CON, randint(10), perm + 1)) obvious = TRUE;
					}

					break;
				case RBE_PARASITE:
					/* Obvious */
					obvious = TRUE;

					//if (!p_ptr->parasite) set_parasite(damage, r_idx);

					break;
				case RBE_HALLU:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage -- we assume the 'elemental' part stands for the attack
					   being sort of a critical hit that doesn't allow AC mitigation! >;-D */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Increase "image" */
					if (!p_ptr->resist_chaos) {
						if (set_image(Ind, p_ptr->image + 3 + randint(rlev / 2)))
							obvious = TRUE;
					}
					break;

				case RBE_TIME:
					/* Special damage */
					dam_ele = damage;
					if (p_ptr->resist_time) dam_ele = (dam_ele * 6) / (randint(6) + 6);

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
#ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;

					switch (p_ptr->resist_time ? randint(9) : randint(10)) {
					case 1: case 2: case 3: case 4: case 5: {
						if (p_ptr->keep_life) msg_print(Ind, "You are unaffected!");
						else {
							msg_print(Ind, "You feel life has clocked back.");
							lose_exp(Ind, 100 + (p_ptr->exp / 100) * MON_DRAIN_LIFE);
						}
						break;
					}
					case 6: case 7: case 8: case 9: {
						int stat = rand_int(6);

						switch (stat) {
							case A_STR: act = "strong"; break;
							case A_INT: act = "bright"; break;
							case A_WIS: act = "wise"; break;
							case A_DEX: act = "agile"; break;
							case A_CON: act = "hardy"; break;
							case A_CHR: act = "beautiful"; break;
						}
						msg_format(Ind, "You're not as %s as you used to be...", act);
						if (safe_area(Ind)) break;
						p_ptr->stat_cur[stat] = (p_ptr->stat_cur[stat] * 3) / 4;
						if (p_ptr->stat_cur[stat] < 3) p_ptr->stat_cur[stat] = 3;
						p_ptr->update |= (PU_BONUS);
						break;
					}
					case 10: {
						msg_print(Ind, "You're not as powerful as you used to be...");
						if (safe_area(Ind)) break;
							for (k = 0; k < C_ATTRIBUTES; k++) {
							p_ptr->stat_cur[k] = (p_ptr->stat_cur[k] * 3) / 4;
							if (p_ptr->stat_cur[k] < 3) p_ptr->stat_cur[k] = 3;
						}
						p_ptr->update |= (PU_BONUS);
						break;
					}
					}
					//carried_monster_hit = TRUE;
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);
					update_smart_learn(Ind, m_idx, DRS_TIME);
					break;

				case RBE_SANITY:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //resistance is applied inside take_sanity_hit()

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 33% physical, 67% elemental */
						dam_ele = (dam_ele * 2) / 3;
						damage /= 3;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					obvious = TRUE;
					msg_print(Ind, "\377RYou shiver in madness..");

					/* Since sanity hits can become too powerful
					   by the normal melee-boosting formula for
					   monsters that gained levels, limit it - C. Blue */
//moving this to monster_exp for now (C. Blue)	if (damage > r_ptr->dd * r_ptr->ds) damage = r_ptr->dd * r_ptr->ds;
					/* (I left out an additional (totally averaging) '..+r_ptr->dd' to allow
					   tweaking the limit a bit by choosing appropriate values in r_info.txt */

					take_sanity_hit(Ind, damage, ddesc, -m_idx);
					break;

				case RBE_DISARM: /* Note: Shields cannot be disarmed, only weapons can */
					/* We assume it's a critical attack that does not get mitigated by armour. */
				{
					int slot = INVEN_WIELD;
					u32b f1, f2, f3, f4, f5, f6, esp;
					object_type *o_ptr;
					bool shield = FALSE, secondary = FALSE;
					bool dis_sec = FALSE;
					char o_name[ONAME_LEN];

					if (p_ptr->inventory[INVEN_ARM].k_idx) {
						shield = p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD ? TRUE : FALSE;
						secondary = !shield;
					}
					if (p_ptr->dual_wield) {
						if (p_ptr->dual_mode && magik(50)) slot = INVEN_ARM;
					} else if (secondary) slot = INVEN_ARM;
					o_ptr = &p_ptr->inventory[slot];

					/* Take damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, -m_idx);

					/* Bare-handed? oh.. */
					if (!o_ptr->k_idx) {
						msg_format(Ind, "\377o%^s cuts deep in your arms and hands", m_name);
						(void)set_cut(Ind, p_ptr->cut + 100, -m_idx, FALSE);
						break;
					}

					msg_format(Ind, "\377o%^s tries to disarm you.", m_name);
					if (safe_area(Ind)) break;

					object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

					/* object itself prevents getting separated? */
					/* can never take off permanently cursed stuff */
					if (f3 & TR3_PERMA_CURSE) break;
					else if ((f3 & TR3_HEAVY_CURSE) && magik(90)) break;
					else if (like_artifact_p(o_ptr) && magik(50)) break;

					/* do we hold one weapon with two hands? very safe */
					if (!p_ptr->heavy_wield && !shield && !p_ptr->dual_wield && (
					    ((f4 & TR4_MUST2H) && magik(90)) ||
					    ((f4 & TR4_SHOULD2H) && magik(85)) ||
					    ((f4 & TR4_COULD2H) && magik(75)) ||
					    (!(f4 & (TR4_MUST2H | TR4_SHOULD2H | TR4_COULD2H)) && magik(15)) /* we catch it with our spare hand ;) */
					    ))
						break;

					/* riposte */
					if (rand_int(p_ptr->skill_thn) * (p_ptr->heavy_wield ? 1 : 3)
					    < (rlev + damage + UNAWARENESS(p_ptr))) {
						if (!p_ptr->dual_wield || magik(90)) {
							/* For 'Arena Monster Challenge' event: */
							if (safe_area(Ind)) {
								msg_print(Ind, "\377wYou thought you'd lose the grip of your weapon but it didn't really happen.");
								break;
							}

							msg_print(Ind, "\376\377rYou lose the grip of your weapon!");
#ifdef USE_SOUND_2010
							sound(Ind, "disarm_weapon", "", SFX_TYPE_ATTACK, FALSE);
#endif
							//msg_format(Ind, "\377r%^s disarms you!", m_name);
							object_desc(0, o_name, o_ptr, FALSE, 3);
							if (cfg.anti_arts_hoard && true_artifact_p(o_ptr) && magik(98)) {
								inven_takeoff(Ind, slot, 1, FALSE, TRUE);
								s_printf("%s EFFECT: Disarmed (takeoff) %s: %s.\n", showtime(), p_ptr->name, o_name);
							} else {
#ifdef DISARM_SCATTER
								int o_idx;
								object_type forge = p_ptr->inventory[slot];
#endif

								s_printf("%s EFFECT: Disarmed (drop) %s: %s.\n", showtime(), p_ptr->name, o_name);
#ifndef DISARM_SCATTER
								inven_drop(TRUE, Ind, slot, 1, TRUE);
#else
								o_idx = inven_drop(FALSE, Ind, slot, 1, TRUE);
								if (o_idx == -1) s_printf("DISARM: ITEM DESTROYED.\n");
								else if (o_idx == -2) {
									int x1, y1, try = 500;
									cave_type **zcave;

									o_idx = 0;
									if ((zcave = getcave(&p_ptr->wpos))) /* this should never.. */
										while (o_idx <= 0 && try--) {
											x1 = rand_int(p_ptr->cur_wid);
											y1 = rand_int(p_ptr->cur_hgt);

											if (!cave_clean_bold(zcave, y1, x1)) continue;
											forge.marked2 = ITEM_REMOVAL_DEATH_WILD;
											o_idx = drop_near(try ? FALSE : TRUE, 0, &forge, 0, &p_ptr->wpos, y1, x1);
										}
									s_printf("DISARM: SCATTERING %s.\n", o_idx > 0 ? "succeeded" : "failed");
								}
#endif
								if (slot == INVEN_ARM) dis_sec = TRUE;
							}
						} else { /* dual-wield "feature".. get dual-disarmed :-p */
							/* For 'Arena Monster Challenge' event: */
							if (safe_area(Ind)) {
								msg_print(Ind, "\377wYou thought you'd lose the grip of your weapons but it didn't really happen.");
								break;
							}

							msg_print(Ind, "\376\377rYou lose the grip of your weapons!");
#ifdef USE_SOUND_2010
							sound(Ind, "disarm_weapon", "", SFX_TYPE_ATTACK, FALSE);
#endif
							//msg_format(Ind, "\377r%^s disarms you!", m_name);
							o_ptr = &p_ptr->inventory[INVEN_WIELD];
							object_desc(0, o_name, o_ptr, FALSE, 3);
							if (cfg.anti_arts_hoard && true_artifact_p(o_ptr) && magik(98)) {
								inven_takeoff(Ind, INVEN_WIELD, 1, FALSE, TRUE);
								s_printf("%s EFFECT: Disarmed (dual, takeoff) %s: %s.\n", showtime(), p_ptr->name, o_name);
							} else {
#ifdef DISARM_SCATTER
								int o_idx;
								object_type forge = p_ptr->inventory[INVEN_WIELD];
#endif

								s_printf("%s EFFECT: Disarmed (dual, drop) %s: %s.\n", showtime(), p_ptr->name, o_name);
#ifndef DISARM_SCATTER
								inven_drop(TRUE, Ind, INVEN_WIELD, 1, TRUE);
#else
								o_idx = inven_drop(FALSE, Ind, INVEN_WIELD, 1, TRUE);
								if (o_idx == -1) s_printf("DISARM: ITEM DESTROYED.\n");
								else if (o_idx == -2) {
									int x1, y1, try = 500;
									cave_type **zcave;

									o_idx = 0;
									if ((zcave = getcave(&p_ptr->wpos))) /* this should never.. */
										while (o_idx <= 0 && try--) {
											x1 = rand_int(p_ptr->cur_wid);
											y1 = rand_int(p_ptr->cur_hgt);

											if (!cave_clean_bold(zcave, y1, x1)) continue;
											forge.marked2 = ITEM_REMOVAL_DEATH_WILD;
											o_idx = drop_near(try ? FALSE : TRUE, 0, &forge, 0, &p_ptr->wpos, y1, x1);
										}
									s_printf("SCATTERING %s.\n", o_idx > 0 ? "succeeded" : "failed");
								}
#endif
							}

							o_ptr = &p_ptr->inventory[INVEN_ARM];
							object_desc(0, o_name, o_ptr, FALSE, 3);
							if (cfg.anti_arts_hoard && true_artifact_p(o_ptr) && magik(98)) {
								inven_takeoff(Ind, INVEN_ARM, 1, FALSE, TRUE);
								s_printf("%s EFFECT: Disarmed (dual, takeoff) %s: %s.\n", showtime(), p_ptr->name, o_name);
							} else {
#ifdef DISARM_SCATTER
								int o_idx;
								object_type forge = p_ptr->inventory[INVEN_ARM];
#endif

								s_printf("%s EFFECT: Disarmed (dual, drop) %s: %s.\n", showtime(), p_ptr->name, o_name);
#ifndef DISARM_SCATTER
								inven_drop(TRUE, Ind, slot, 1, TRUE);
#else
								o_idx = inven_drop(FALSE, Ind, INVEN_ARM, 1, TRUE);
								if (o_idx == -1) s_printf("DISARM: ITEM DESTROYED.\n");
								else if (o_idx == -2) {
									int x1, y1, try = 500;
									cave_type **zcave;

									o_idx = 0;
									if ((zcave = getcave(&p_ptr->wpos))) /* this should never.. */
										while (o_idx <= 0 && try--) {
											x1 = rand_int(p_ptr->cur_wid);
											y1 = rand_int(p_ptr->cur_hgt);

											if (!cave_clean_bold(zcave, y1, x1)) continue;
											forge.marked2 = ITEM_REMOVAL_DEATH_WILD;
											o_idx = drop_near(try ? FALSE : TRUE, 0, &forge, 0, &p_ptr->wpos, y1, x1);
										}
									s_printf("SCATTERING %s.\n", o_idx > 0 ? "succeeded" : "failed");
								}
#endif
							}
							dis_sec = TRUE;
						}

						obvious = TRUE;
#ifdef ALLOW_SHIELDLESS_DEFENSIVE_STANCE
						if ((p_ptr->combat_stance == 1 &&
						    dis_sec && !p_ptr->inventory[INVEN_WIELD].k_idx) ||
						    p_ptr->combat_stance == 2) {
#else
						if (p_ptr->combat_stance == 2) {
#endif
							msg_print(Ind, "\377sYou return to balanced combat stance.");
							p_ptr->combat_stance = 0;
							p_ptr->update |= (PU_BONUS);
							p_ptr->redraw |= (PR_PLUSSES | PR_STATE);
						}
					}
					break;
				}

				case RBE_FAMINE:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take some damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, 0);

					if (p_ptr->prace != RACE_VAMPIRE && p_ptr->prace != RACE_ENT && !(p_ptr->prace == RACE_MAIA && p_ptr->lev < 20)) {
						if (p_ptr->suscep_life) set_food(Ind, (p_ptr->food * 3) / 4);
						else set_food(Ind, p_ptr->food / 2);
						msg_print(Ind, "You have a sudden attack of hunger!");
					}
					obvious = TRUE;

					break;

				case RBE_SEDUCE:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take some damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, 0);

					gone = blinked = do_seduce(Ind, m_idx);

					/* Hack -- aquatic seducers won't blink */
					if (r_ptr->flags7 & RF7_AQUATIC) gone = blinked = FALSE;

					obvious = TRUE;

					break;

				case RBE_LITE:
					/* Special damage */
					dam_ele = damage;
					if (p_ptr->suscep_lite) dam_ele *= 2;
					if (p_ptr->resist_lite) dam_ele = (dam_ele * 4) / (randint(6) + 6);

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
#ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
#endif
					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;

					/* Take damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, 0);

					/* Increase "blind" */
					if (!p_ptr->resist_blind && !p_ptr->resist_lite)
						if (set_blind(Ind, p_ptr->blind + 10 + randint(rlev)))
							obvious = TRUE;

					/* Learn about the player */
					update_smart_learn(Ind, m_idx, DRS_BLIND);
					update_smart_learn(Ind, m_idx, DRS_LITE);
					break;

				case RBE_EAT_MANA:
#ifdef EXPERIMENTAL_MITIGATION
					/* Special damage -- we assume the 'elemental' part stands for the attack
					   being sort of a critical hit that doesn't allow AC mitigation! >;-D */
					dam_ele = damage; //no resistance

					switch (method) {
					case RBM_HIT: case RBM_PUNCH: case RBM_KICK:
					case RBM_CLAW: case RBM_BITE: case RBM_STING:
					case RBM_BUTT: case RBM_CRUSH:
						/* 50% physical, 50% elemental */
						dam_ele /= 2;
						damage /= 2;
						break;
					default:
						/* 100% elemental */
						damage = 0;
					}
 #ifdef ELEMENTAL_AC_MITIGATION
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / (AC_CAP_DIV + 100)); /* + 100: harder to absorb */
 #endif
					/* Reduce the eat-mana damage part if we don't have (much) mana */
					if (dam_ele > p_ptr->cmp) dam_ele = p_ptr->cmp;

					/* unify elemental and physical damage again: */
					damage = damage + dam_ele;
#endif

					/* Take some damage */
					if (dam_msg[0]) msg_format(Ind, dam_msg, damage);
					take_hit(Ind, damage, ddesc, 0);

					if (safe_area(Ind)) break;

					/* Drain mana */
					if (p_ptr->cmp > 0) {
#if 0
						p_ptr->cmp -= dam_ele; //drain fixed amount depending on damage
						p_ptr->cmp -= (p_ptr->mhp * (r_ptr->level + 50)) / 1000; //additionally drain a percentage
#else
						p_ptr->cmp -= (p_ptr->mhp * (r_ptr->level + 30)) / 600; //drain a percentage
#endif
						if (p_ptr->cmp < 0) p_ptr->cmp = 0;
						p_ptr->redraw |= PR_MANA;
						if (!obvious) msg_print(Ind, "Your psychic energy gets drained.");
						obvious = TRUE;

						update_smart_learn(Ind, m_idx, DRS_LITE);
					}
					break;
			}

			/* Reset global melee-variable to normal (in case of MELEE_HIT_DRAINS_SHIELD, default: unused) */
			melee_hit = FALSE;

			/* Hack -- only one of cut or stun */
			if (do_cut && do_stun) {
				/* Cancel cut */
				if (rand_int(100) < 50) do_cut = 0;
				/* Cancel stun */
				else do_stun = 0;
			}

			/* Handle cut */
			if (do_cut) {
				int k = 0;

				/* Critical hit (zero if non-critical) */
#ifndef EXPERIMENTAL_MITIGATION
				tmp = monster_critical(d_dice, d_side, damage);
#else
				tmp = monster_critical(d_dice, d_side, damage_org);
#endif

				/* Roll for damage */
				switch (tmp) {
				case 0: k = 0; break;
				case 1: k = randint(5); break;
				case 2: k = randint(5) + 5; break;
				case 3: k = randint(20) + 20; break;
				case 4: k = randint(50) + 50; break;
				case 5: k = randint(100) + 100; break;
				case 6: k = 300; break;
				default: k = 500; break;
				}

				/* Apply the cut */
				if (k) (void)set_cut(Ind, p_ptr->cut + k, -m_idx, FALSE);
			}
#ifndef SUPPRESS_MONSTER_STUN // HERESY !
			/* That's overdone; \
			 * let's remove do_stun from RBM_HIT instead		- Jir - */

			/* Handle stun */
			if (do_stun) {
				int k = 0;

				/* Critical hit (zero if non-critical) */
#ifndef EXPERIMENTAL_MITIGATION
				tmp = monster_critical(d_dice, d_side, damage);
#else
				tmp = monster_critical(d_dice, d_side, damage_org);
#endif

				/* Simulate Martial Arts techniques - unfair advantage though as we're RF3_NO_STUN. */
#if 0
				if (r_idx == RI_MIRROR && !tmp && !rand_int(4)) tmp = 1 + rand_int(2);
#else
				/* So tone it down to a minimum at least */
				if (r_idx == RI_MIRROR && !tmp && !rand_int(5)) tmp = 1;
#endif

				/* Roll for damage */
				switch (tmp) {
				case 0: k = 0; break;
				case 1: k = randint(5); break;
				case 2: k = randint(10) + 5; break;
				case 3: k = randint(15) + 10; break;
				case 4: k = randint(15) + 20; break;
				case 5: k = randint(15) + 30; break;
				case 6: k = randint(15) + 40; break;
				case 7: k = randint(10) + 55; break;
				case 8: k = randint(5) + 70; break;
				default: k = 100; break; /* currently 8 is max, so this cannot happen */
				}

				/* Apply the stun */
				if (k) (void)set_stun(Ind, p_ptr->stun + k);
			}
#endif
			if (explode) {
#ifdef USE_SOUND_2010
#else
//				sound(SOUND_EXPLODE);
#endif
				if (mon_take_hit(Ind, m_idx, m_ptr->hp + 1, &fear, NULL)) {
					blinked = FALSE;
					alive = FALSE;
				}
			}

			/* Apply our retaliation-auras */
			if (touched && alive) {
				trigger_reactive = TRUE;

				/* Apply monster-vampirism (2023! just for mirror fight ~_~ - C. Blue) */
				if ((r_ptr->flags9 & RF9_VAMPIRIC) && m_ptr->hp < m_ptr->maxhp) {
					/* Heal */
					m_ptr->hp += damage / 3;
					if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

					/* Redraw (later) if needed */
					update_health(m_idx);
				}
			}
		}

		/* Monster missed player */
		else {
			/* Analyze failed attacks -- all attacks that would have 'touched' -- give 'miss' message for most */
			switch (method) {
			case RBM_HIT:
			case RBM_TOUCH:
			case RBM_PUNCH:
			case RBM_KICK:
			case RBM_CLAW:
			case RBM_BITE:
			case RBM_STING:
			case RBM_XXX1:
			case RBM_BUTT:
			case RBM_CRUSH:
			//case RBM_XXX2:
			case RBM_CHARGE:
				/* Visible monsters */
				if (p_ptr->mon_vis[m_idx]) {
					/* Disturbing */
					disturb(Ind, 1, 0);

					/* Message */
					msg_format(Ind, "%^s misses you.", m_name);
				}
#ifdef ATTEMPT_TRIGGERS_REACTIVE
				trigger_reactive = TRUE;
#endif
				/* Fall through */
			/* Note: No 'miss' message here, these are 'pseudo-attacks' ie not actively 'seeking' the player (worms crawling, vortices engulfing): */
			case RBM_CRAWL:
			case RBM_ENGULF:
				break;
			}
		}

#ifndef TRIGGERS_REACTIVE_ONCEPERROUND
		if (trigger_reactive && alive) do_trigger_reactive(Ind, m_idx, m_name, &fear, &alive);
#endif

#ifdef OLD_MONSTER_LORE
		/* Analyze "visible" monsters only */
		if (visible) {
			/* Count "obvious" attacks (and ones that cause damage) */
			if (obvious || damage || (r_ptr->r_blows[ap_cnt] > 10)) {
				/* Count attacks of this type */
				if (r_ptr->r_blows[ap_cnt] < MAX_UCHAR)
					r_ptr->r_blows[ap_cnt]++;
			}
		}
#endif

		/* Hack -- blinked monster doesn't attack any more */
		if (gone || !alive) break;
	}
#ifdef TRIGGERS_REACTIVE_ONCEPERROUND
	if (trigger_reactive && alive) do_trigger_reactive(Ind, m_idx, m_name, &fear, &alive);
#endif


	/* Blink away */
	if (blinked && alive) {
		if (teleport_away(m_idx, MAX_SIGHT * 2 + 5)) {
			msg_print(Ind, "There is a puff of smoke!");
#ifdef USE_SOUND_2010
//redundant: teleport_away() already calls sound() :/	sound(Ind, "puff", NULL, SFX_TYPE_MON_SPELL, TRUE);
#endif
		}
	}


	/* manage warning_autoret for melee-dependant chars, which gives newbies
	   a hint that they don't need to try and "run into" a monster to attack
	   it, but should rather stand still to conserve energy. - C. Blue */
	if (p_ptr->warning_autoret != 99 && p_ptr->warning_autoret_ok == 0 &&
	    !(r_ptr->flags8 & RF8_NO_AUTORET) &&
	    !p_ptr->paralyzed && p_ptr->stun <= 100 &&
	    !p_ptr->blind && !p_ptr->confused && !p_ptr->afraid) {
		p_ptr->warning_autoret++;
		p_ptr->warning_autoret_ok = 1;
		if (p_ptr->warning_autoret == 3) {
			p_ptr->warning_autoret = 0;
			msg_print(Ind, "\374\377oHINT: Your character will AUTOMATICALLY attack any monster that comes close!");
			msg_print(Ind, "\374\377o      Don't try to \"run into\" a monster, because you might end up missing it.");
			s_printf("warning_autoret: %s\n", p_ptr->name);
		}
	}

	if (p_ptr->warning_wield_combat == 0 &&
	    p_ptr->inventory[INVEN_WIELD].k_idx == 0 &&
	    p_ptr->inventory[INVEN_ARM].k_idx == 0 &&
	    p_ptr->inventory[INVEN_BOW].k_idx == 0
	    && !get_skill(p_ptr, SKILL_MARTIAL_ARTS)) {
		p_ptr->warning_wield_combat = 1;
		msg_print(Ind, "\374\377RWARNING: You aren't wielding a weapon! Press \377yw\377R to equip one!");
		s_printf("warning_wield_combat: %s\n", p_ptr->name);
	}

	/* Always notice cause of death */
	if (p_ptr->death) {
		if (!r_ptr->r_deaths && (r_ptr->flags1 & RF1_UNIQUE)) s_printf("Unique 1st death: %d (%s) killed %s (%s).\n", r_idx, r_name, p_ptr->name, p_ptr->accountname);
		r_ptr->r_deaths++;
	}

	/* Assume we attacked */
	return(TRUE);
}
void do_trigger_reactive(int Ind, int m_idx, cptr m_name, bool *fear, bool *alive) {
	player_type *p_ptr = Players[Ind];
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);
	int r_idx = m_ptr->r_idx;

	/* Check if our 'intrinsic' (Blood Magic, not from item/external spell) auras were suppressed by our own antimagic field. */
	bool aura_ok = !magik((p_ptr->antimagic * 8) / 5);
	int auras_failed = 0, player_aura_dam;


	/*
	 * Apply non-skill auras
	 */

	/* Alternate contradicting auras, same as with brands */
	if (p_ptr->sh_fire && p_ptr->sh_cold && *alive) {
		/* Immolation / fire aura */
		if (rand_int(2)) {
			if (!(r_ptr->flags3 & RF3_IM_FIRE)) {
				player_aura_dam = damroll(2,6);
				if (r_ptr->flags9 & RF9_RES_FIRE) player_aura_dam /= 3;
				if (r_ptr->flags3 & RF3_SUSCEP_FIRE) player_aura_dam *= 2;
				if (r_idx == RI_MIRROR) player_aura_dam = (player_aura_dam * MIRROR_REDUCE_DAM_TAKEN_AURA + 99) / 100;
				if (mon_take_hit(Ind, m_idx, player_aura_dam, fear, " turns into a pile of ashes")) *alive = FALSE;
				else {
					msg_format(Ind, "%^s gets burned for \377%c%d\377w damage!", m_name, (r_ptr->flags1 & RF1_UNIQUE) ? 'e' : 'g', player_aura_dam);
					if (*fear && !(m_ptr->csleep || m_ptr->stunned > 100)) {
						if (m_ptr->r_idx != RI_MORGOTH) msg_format(Ind, "%^s flees in terror!", m_name);
						else msg_format(Ind, "%^s retreats!", m_name);
					}
				}
			}
#ifdef OLD_MONSTER_LORE
			else {
				if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags3 |= RF3_IM_FIRE;
			}
#endif
		}
		/* Frostweaving / cold aura */
		else {
			if (!(r_ptr->flags3 & RF3_IM_COLD)) {
				player_aura_dam = damroll(2,6);
				if (r_ptr->flags9 & RF9_RES_COLD) player_aura_dam /= 3;
				if (r_ptr->flags3 & RF3_SUSCEP_COLD) player_aura_dam *= 2;
				if (r_idx == RI_MIRROR) player_aura_dam = (player_aura_dam * MIRROR_REDUCE_DAM_TAKEN_AURA + 99) / 100;
				if (mon_take_hit(Ind, m_idx, player_aura_dam, fear, " freezes and shatters")) *alive = FALSE;
				else {
					msg_format(Ind, "%^s freezes for \377%c%d\377w damage!", m_name, (r_ptr->flags1 & RF1_UNIQUE) ? 'e' : 'g', player_aura_dam);
					if (*fear && !(m_ptr->csleep || m_ptr->stunned > 100)) {
						if (m_ptr->r_idx != RI_MORGOTH) msg_format(Ind, "%^s flees in terror!", m_name);
						else msg_format(Ind, "%^s retreats!", m_name);
					}
				}
			}
#ifdef OLD_MONSTER_LORE
			else {
				if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags3 |= RF3_IM_COLD;
			}
#endif
		}
	} else {
		/* Immolation / fire aura */
		if (p_ptr->sh_fire && *alive) {
			if (!(r_ptr->flags3 & RF3_IM_FIRE)) {
				player_aura_dam = damroll(2,6);
				if (r_ptr->flags9 & RF9_RES_FIRE) player_aura_dam /= 3;
				if (r_ptr->flags3 & RF3_SUSCEP_FIRE) player_aura_dam *= 2;
				if (r_idx == RI_MIRROR) player_aura_dam = (player_aura_dam * MIRROR_REDUCE_DAM_TAKEN_AURA + 99) / 100;
				if (mon_take_hit(Ind, m_idx, player_aura_dam, fear, " turns into a pile of ashes")) *alive = FALSE;
				else {
					msg_format(Ind, "%^s gets burned for \377%c%d\377w damage!", m_name, (r_ptr->flags1 & RF1_UNIQUE) ? 'e' : 'g', player_aura_dam);
					if (*fear && !(m_ptr->csleep || m_ptr->stunned > 100)) {
						if (m_ptr->r_idx != RI_MORGOTH) msg_format(Ind, "%^s flees in terror!", m_name);
						else msg_format(Ind, "%^s retreats!", m_name);
					}
				}
			}
#ifdef OLD_MONSTER_LORE
			else {
				if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags3 |= RF3_IM_FIRE;
			}
#endif
		}
		/* Frostweaving / cold aura */
		if (p_ptr->sh_cold && *alive) {
			if (!(r_ptr->flags3 & RF3_IM_COLD)) {
				player_aura_dam = damroll(2,6);
				if (r_ptr->flags9 & RF9_RES_COLD) player_aura_dam /= 3;
				if (r_ptr->flags3 & RF3_SUSCEP_COLD) player_aura_dam *= 2;
				if (r_idx == RI_MIRROR) player_aura_dam = (player_aura_dam * MIRROR_REDUCE_DAM_TAKEN_AURA + 99) / 100;
				if (mon_take_hit(Ind, m_idx, player_aura_dam, fear, " freezes and shatters")) *alive = FALSE;
				else {
					msg_format(Ind, "%^s freezes for \377%c%d\377w damage!", m_name, (r_ptr->flags1 & RF1_UNIQUE) ? 'e' : 'g', player_aura_dam);
					if (*fear && !(m_ptr->csleep || m_ptr->stunned > 100)) {
						if (m_ptr->r_idx != RI_MORGOTH) msg_format(Ind, "%^s flees in terror!", m_name);
						else msg_format(Ind, "%^s retreats!", m_name);
					}
				}
			}
#ifdef OLD_MONSTER_LORE
			else {
				if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags3 |= RF3_IM_COLD;
			}
#endif
		}
	}
	/* Electricity / lightning aura */
	if (p_ptr->sh_elec && *alive) {
		if (!(r_ptr->flags3 & RF3_IM_ELEC)) {
			player_aura_dam = damroll(2,6);
			if (r_ptr->flags9 & RF9_RES_ELEC) player_aura_dam /= 3;
			if (r_ptr->flags9 & RF9_SUSCEP_ELEC) player_aura_dam *= 2;
			if (r_idx == RI_MIRROR) player_aura_dam = (player_aura_dam * MIRROR_REDUCE_DAM_TAKEN_AURA + 99) / 100;
			if (mon_take_hit(Ind, m_idx, player_aura_dam, fear," turns into a pile of cinder")) *alive = FALSE;
			else {
				msg_format(Ind, "%^s gets zapped for \377%c%d\377w damage!", m_name, (r_ptr->flags1 & RF1_UNIQUE) ? 'e' : 'g', player_aura_dam);
				if (*fear && !(m_ptr->csleep || m_ptr->stunned > 100)) {
					if (m_ptr->r_idx != RI_MORGOTH) msg_format(Ind, "%^s flees in terror!", m_name);
					else msg_format(Ind, "%^s retreats!", m_name);
				}
			}
		}
#ifdef OLD_MONSTER_LORE
		else if (p_ptr->mon_vis[m_idx]) r_ptr->r_flags3 |= RF3_IM_ELEC;
#endif
	}

	/* Nimbus - Kurzel */
	if (p_ptr->nimbus && *alive) {
		msg_format(Ind, "%^s breaches your aura of power!", m_name);
		do_nimbus(Ind, m_ptr->fy, m_ptr->fx);
		if (!m_ptr->r_idx) *alive = FALSE; //hack: Monster was deleted meanwhile if it died, so m_ptr was wiped. We test for r_idx as it should never be zero unless wiped.
	}

	/*
	 *Apply the 'shield auras'
	 */
	/* force shield */
	if (p_ptr->shield && (p_ptr->shield_opt & SHIELD_COUNTER) && *alive) {
		int d = damroll(p_ptr->shield_power_opt, p_ptr->shield_power_opt2);

		if (r_idx == RI_MIRROR) d = (d * MIRROR_REDUCE_DAM_TAKEN_AURA + 99) / 100;
		if (mon_take_hit(Ind, m_idx, d, fear, " got bashed by your mystc shield")) *alive = FALSE;
		else {
			msg_format(Ind, "Your mystic shield bashes %s for \377%c%d\377w damage!", m_name, (r_ptr->flags1 & RF1_UNIQUE) ? 'e' : 'g', d);
			if (*fear && !(m_ptr->csleep || m_ptr->stunned > 100)) {
				if (m_ptr->r_idx != RI_MORGOTH) msg_format(Ind, "%^s flees in terror!", m_name);
				else msg_format(Ind, "%^s retreats!", m_name);
			}
		}
	}
	/* fire shield */
	if (p_ptr->shield && (p_ptr->shield_opt & SHIELD_FIRE) && *alive) {
		if (!(r_ptr->flags3 & RF3_IM_FIRE)) {
			int d = damroll(p_ptr->shield_power_opt, p_ptr->shield_power_opt2);

			if (r_ptr->flags9 & RF9_RES_FIRE) d /= 3;
			if (r_ptr->flags3 & RF3_SUSCEP_FIRE) d *= 2;
			if (r_idx == RI_MIRROR) d = (d * MIRROR_REDUCE_DAM_TAKEN_AURA + 99) / 100;
			if (mon_take_hit(Ind, m_idx, d, fear, " turns into a pile of ashes")) *alive = FALSE;
			else {
				msg_format(Ind, "Your fiery shield burns %^s for \377%c%d\377w damage!", m_name, (r_ptr->flags1 & RF1_UNIQUE) ? 'e' : 'g', d);
				if (*fear && !(m_ptr->csleep || m_ptr->stunned > 100)) {
					if (m_ptr->r_idx != RI_MORGOTH) msg_format(Ind, "%^s flees in terror!", m_name);
					else msg_format(Ind, "%^s retreats!", m_name);
				}
			}
		}
	}
#if 0
	/* lightning shield */
	if (p_ptr->shield && (p_ptr->shield_opt & SHIELD_ELEC) && *alive) {
		if (!(r_ptr->flags3 & RF3_IM_ELEC)) {
			int d = damroll(p_ptr->shield_power_opt, p_ptr->shield_power_opt2);

			if (r_ptr->flags9 & RF9_RES_ELEC) d /= 3;
			if (r_ptr->flags9 & RF9_SUSCEP_ELEC) d *= 2;
			if (r_idx == RI_MIRROR) d = (d * MIRROR_REDUCE_DAM_TAKEN_AURA + 99) / 100;
			if (mon_take_hit(Ind, m_idx, d, fear, " turns into a pile of cinder")) *alive = FALSE;
			else {
				msg_format(Ind, "Your lightning shield zaps %^s for \377%c%d\377w damage!", m_name, (r_ptr->flags1 & RF1_UNIQUE) ? 'e' : 'g', d);
				if (*fear && !(m_ptr->csleep || m_ptr->stunned > 100)) {
					if (m_ptr->r_idx != RI_MORGOTH) msg_format(Ind, "%^s flees in terror!", m_name);
					else msg_format(Ind, "%^s retreats!", m_name);
				}
			}
		}
	}
	/* fear shield */
	if (p_ptr->shield && (p_ptr->shield_opt & SHIELD_FEAR) && *alive
	    && !(r_ptr->flags3 & RF3_NO_FEAR)
	    && !(r_ptr->flags2 & RF2_POWERFUL)
	    && !(r_ptr->flags1 & RF1_UNIQUE)) {
		int tmp, d = damroll(p_ptr->shield_power_opt, p_ptr->shield_power_opt2) - m_ptr->level;

		if (d > 0) {
			msg_format(Ind, "%^s gets scared away!", m_name);
			/* Increase fear */
			tmp = m_ptr->monfear + p_ptr->shield_power_opt;
			*fear = TRUE;
			/* Set fear */
			m_ptr->monfear = (tmp < 200) ? tmp : 200;
			m_ptr->monfear_gone = 0;
		}
	}
#endif

	/* ice shield, functionally similar to aura of death - Kurzel */
	if (p_ptr->shield && (p_ptr->shield_opt & SHIELD_ICE) && *alive) {
		if (magik(25)) {
			//Note - Maybe not fully consinstent?
			//       Either it's an aura, which gets breached and as a result causes an outburst ie projection,
			//       or it's a shield which works the same as the other shields (SHIELD_FIRE most prominently), not causing a projection but just retaliation damage.
			//msg_format(Ind, "%^s breaches your ice shield!", m_name);
			sprintf(p_ptr->attacker, " is enveloped in ice for");
			fire_ball(Ind, GF_ICE, 0, damroll(p_ptr->shield_power_opt, p_ptr->shield_power_opt2), 1, p_ptr->attacker);
			if (!m_ptr->r_idx) *alive = FALSE; //hack: Monster was deleted meanwhile if it died, so m_ptr was wiped. We test for r_idx as it should never be zero unless wiped.
		}
	}
	/* plasma shield, functionally similar to aura of death - Kurzel */
	if (p_ptr->shield && (p_ptr->shield_opt & SHIELD_PLASMA) && *alive) {
		if (magik(25)) {
			//Note - Maybe not fully consinstent?
			//       Either it's an aura, which gets breached and as a result causes an outburst ie projection,
			//       or it's a shield which works the same as the other shields (SHIELD_FIRE most prominently), not causing a projection but just retaliation damage.
			//msg_format(Ind, "%^s breaches your plasma shield!", m_name);
			sprintf(p_ptr->attacker, " is enveloped in plasma for");
			fire_ball(Ind, GF_PLASMA, 0, damroll(p_ptr->shield_power_opt, p_ptr->shield_power_opt2), 1, p_ptr->attacker);
			if (!m_ptr->r_idx) *alive = FALSE; //hack: Monster was deleted meanwhile if it died, so m_ptr was wiped. We test for r_idx as it should never be zero unless wiped.
		}
	}

	/*
	 * Apply the blood magic auras
	 */
	/* Aura of fear is now affected by the monster level too */
	if (get_skill(p_ptr, SKILL_AURA_FEAR) && p_ptr->aura[AURA_FEAR] &&
	    !(r_ptr->flags3 & (RF3_UNDEAD | RF3_NONLIVING))
	    && !(r_ptr->flags3 & RF3_NO_FEAR) && *alive) {
		int mod = ((r_ptr->flags2 & RF2_POWERFUL) ? 10 : 0) + ((r_ptr->flags1 & RF1_UNIQUE) ? 10 : 0);

		if (magik(get_skill_scale(p_ptr, SKILL_AURA_FEAR, 30) + 5) &&
		    r_ptr->level + mod < get_skill_scale(p_ptr, SKILL_AURA_FEAR, 100)) {
			if (aura_ok) {
				msg_format(Ind, "%^s breaches your aura of fear and appears afraid.", m_name);
				//short 'shock' effect ;) it's cooler than just running away
				m_ptr->monfear = 2 + get_skill_scale(p_ptr, SKILL_AURA_FEAR, 2);
				m_ptr->monfear_gone = 0;
			} else auras_failed++;
		}
	}
	/* Shivering Aura is affected by the monster level */
	if (((get_skill(p_ptr, SKILL_AURA_SHIVER) && p_ptr->aura[AURA_SHIVER]) ||
	    (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST))
	    && !(r_ptr->flags3 & RF3_NO_STUN) && !(r_ptr->flags3 & RF3_IM_COLD) && *alive) {
		int mod = ((r_ptr->flags1 & RF1_UNIQUE) ? 10 : 0);
		int chance_trigger = get_skill_scale(p_ptr, SKILL_AURA_SHIVER, 25);
		int threshold_effect = get_skill_scale(p_ptr, SKILL_AURA_SHIVER, 100);

		if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST) {
			chance_trigger = 25; //max
			threshold_effect = (p_ptr->lev < 50) ? p_ptr->lev * 2 : 100; //80..100 (max)
		}

		chance_trigger += 25; //generic boost

		if (magik(chance_trigger) && (r_ptr->level + mod < threshold_effect)) {
			if (aura_ok) {
				m_ptr->stunned += 10;
				if (m_ptr->stunned > 100) msg_format(Ind, "%^s breaches your shivering aura and appears frozen.", m_name);
				else if (m_ptr->stunned > 50) msg_format(Ind, "%^s breaches your shivering aura and shivers heavily.", m_name);
				else msg_format(Ind, "%^s breaches your shivering aura and shivers.", m_name);
			} else auras_failed++;
		}
	}
	/* Aura of death is NOT affected by monster level*/
	if (get_skill(p_ptr, SKILL_AURA_DEATH) && p_ptr->aura[AURA_DEATH] && *alive) {
		int chance = get_skill_scale(p_ptr, SKILL_AURA_DEATH, 50);

		if (magik(chance)) {
			if (aura_ok) {
				int dam = 5 + chance * 3;

				if (magik(50)) {
					msg_format(Ind, "%^s breaches your death aura, causing a plasma outburst!", m_name);
					sprintf(p_ptr->attacker, " eradiates a wave of plasma for");
					fire_ball(Ind, GF_PLASMA, 0, dam, 1, p_ptr->attacker);
				} else {
					msg_format(Ind, "%^s breaches your death aura, causing an ice explosion!", m_name);
					sprintf(p_ptr->attacker, " eradiates a wave of ice for");
					fire_ball(Ind, GF_ICE, 0, dam, 1, p_ptr->attacker);
				}
				if (!m_ptr->r_idx) *alive = FALSE; //hack: Monster was deleted meanwhile if it died, so m_ptr was wiped. We test for r_idx as it should never be zero unless wiped.
			} else auras_failed++;
		}
	}
	/* Notify if our 'intrinsic' (Blood Magic, not from item/external spell) auras failed.. */
	if (auras_failed) {
#ifdef USE_SOUND_2010
		sound(Ind, "am_field", NULL, SFX_TYPE_MISC, FALSE);
#endif
		msg_format(Ind, "\377%cYour anti-magic field disrupts your aura%s.", COLOUR_AM_OWN, auras_failed == 1 ? "" : "s");
	}
}

/*
 * Determine if a monster attack against the monster succeeds.
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match monster power against player armor.
 */
static int mon_check_hit(int m_idx, int power, int level) {
	monster_type *m_ptr = &m_list[m_idx];

	int i, k, ac;

	/* Percentile dice */
	k = rand_int(100);

	/* Hack -- Always miss or hit */
	if (k < 10) return(k < 5);

	/* Calculate the "attack quality" */
	i = (power + (level * 3));

	/* Total armor */
	ac = m_ptr->ac;

	/* Power and Level compete against Armor */
	if ((i > 0) && (randint(i) > ((ac * 3) / 4))) return(TRUE);

	/* Assume miss */
	return(FALSE);
}


/*
 * Attack a monster via physical attacks.
 */
bool monster_attack_normal(int tm_idx, int m_idx) {
	/* Targer */
	monster_type *tm_ptr = &m_list[tm_idx];
#ifdef RPG_SERVER
	monster_race *tr_ptr = race_inf(tm_ptr);
	int exp_gain;
#endif
	/* Attacker */
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = race_inf(m_ptr);

	int ap_cnt;

	int ac, rlev; // ,i, j, k, tmp;
	int do_cut, do_stun;

	bool dead = FALSE;


	/* Not allowed to attack */
	if (r_ptr->flags2 & RF2_NEVER_BLOW) return(FALSE);

	/* Total armor */
	ac = tm_ptr->ac;

	/* Extract the effective monster level */
	rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
#ifdef RPG_SERVER
	exp_gain = tr_ptr->mexp / 2;
	//exp_gain = race_inf(tm_ptr)->level;
#endif

	/* Scan through all four blows */
	for (ap_cnt = 0; ap_cnt < 4; ap_cnt++) {
		//bool obvious = FALSE; //currently not used

		int power = 0;
		int damage = 0;
//#ifndef EXPERIMENTAL_MITIGATION
		//int damage_org = 0;
//#endif

		/* Extract the attack infomation */
		int effect = m_ptr->blow[ap_cnt].effect;
		int method = m_ptr->blow[ap_cnt].method;
		int d_dice = m_ptr->blow[ap_cnt].d_dice;
		int d_side = m_ptr->blow[ap_cnt].d_side;

		/* Hack -- no more attacks */
		if (!method) break;

		/* Stop if monster is dead or gone */
		if (dead) break;

		/* Extract the attack "power" */
		switch (effect) {
			case RBE_HURT:  power = 60; break;
		}

		/* Monster hits player */
		if (!effect || mon_check_hit(tm_idx, power, rlev)) {
			/* Assume no cut or stun */
			do_cut = do_stun = 0;

			/* Describe the attack method */
			switch (method) {
				case RBM_HIT: {
#ifdef NORMAL_HIT_NO_STUN
					do_cut = 1;
#else
					do_cut = do_stun = 1;
#endif	// NORMAL_HIT_NO_STUN
					break;
				}
			}

			/* Hack -- assume all attacks are obvious */
			//obvious = TRUE;

			/* Roll out the damage */
			damage = damroll(d_dice, d_side);
//#ifndef EXPERIMENTAL_MITIGATION
			//damage_org = damage;
//#endif

			/* Apply appropriate damage */
			switch (effect) {
				case 0:
					/* Hack -- Assume obvious */
					//obvious = TRUE;

					/* Hack -- No damage */
					damage = 0;

					break;

				case RBE_HURT:
				{
					bool fear;

					/* Obvious */
					//obvious = TRUE;

					/* Hack -- Monster armor reduces total damage */
					damage -= (damage * ((ac < AC_CAP) ? ac : AC_CAP) / AC_CAP_DIV);
					/* Take damage */
					dead = mon_take_hit_mon(m_idx, tm_idx, damage, &fear, NULL);

					break;
				}
			}


			/* Hack -- only one of cut or stun */
			if (do_cut && do_stun) {
				/* Cancel cut */
				if (rand_int(100) < 50) do_cut = 0;
				/* Cancel stun */
				else do_stun = 0;
			}
#ifdef RPG_SERVER
			if (dead && m_ptr->pet) {
				char monster_name[MNAME_LEN];
				int Ind = find_player(m_ptr->owner);

				if (Ind) {
					monster_desc(Ind, monster_name, tm_idx, 0x04 | 0x08);
					if (!(Players[Ind]->mode & MODE_PVP)) {
 #ifdef SHOW_XP_GAIN
						gain_exp_onhold(Ind, (unsigned int)(exp_gain));
						msg_format(Ind, "\377yYour pet killed %s. (%d XP)", monster_name, Players[Ind]->gain_exp);
						apply_exp(Ind);
 #else
						msg_format(Ind, "\377yYour pet killed %s.", monster_name);
						gain_exp(Ind, (unsigned int)(exp_gain));
 #endif
					} else msg_format(Ind, "\377yYour pet killed %s.", monster_name);
				}
				if (monster_gain_exp(m_idx,(unsigned int)(exp_gain), FALSE) > 0 && Ind)
					msg_print(Ind, "\377GYour pet looks more experienced!");
			}
#endif
#if 0//todo:implement
			/* Handle cut */
			if (do_cut) {
				int k = 0;

				/* Critical hit (zero if non-critical) */
//#ifndef EXPERIMENTAL_MITIGATION
				tmp = monster_critical(d_dice, d_side, damage);
//#else
				tmp = monster_critical(d_dice, d_side, damage_org);
//#endif

				/* Roll for damage */
				switch (tmp) {
				case 0: k = 0; break;
				case 1: k = randint(5); break;
				case 2: k = randint(5) + 5; break;
				case 3: k = randint(20) + 20; break;
				case 4: k = randint(50) + 50; break;
				case 5: k = randint(100) + 100; break;
				case 6: k = 300; break;
				default: k = 500; break;
				}
 #if 0//todo:implement
				/* Apply the cut */
				if (k) (void)set_cut(Ind, p_ptr->cut + k, FALSE);
 #endif
			}
#endif
#if 0//todo:implement
			/* Handle stun */
			if (do_stun) {
				int k = 0;

				/* Critical hit (zero if non-critical) */
//#ifndef EXPERIMENTAL_MITIGATION
				tmp = monster_critical(d_dice, d_side, damage);
//#else
				tmp = monster_critical(d_dice, d_side, damage_org);
//#endif
				/* Roll for damage */
				switch (tmp) {
				case 0: k = 0; break;
				case 1: k = randint(5); break;
				case 2: k = randint(10) + 10; break;
				case 3: k = randint(20) + 20; break;
				case 4: k = randint(30) + 30; break;
				case 5: k = randint(40) + 40; break;
				case 6: k = 80; break;
				default: k = 100; break;
				}

				/* Apply the stun */
				if (k) (void)set_stun(Ind, p_ptr->stun + k);

#if 0//todo:implement
				if (!(r_ptr->flags3 & RF3_NO_STUN)) {
					if (!m_ptr->stunned) msg_format(Ind, "\377y%^s is stunned.", m_name);
					else msg_format(Ind, "\377y%^s appears more stunned.", m_name);
					m_ptr->stunned = m_ptr->stunned + 20 + get_skill_scale(p_ptr, SKILL_COMBAT, 5);
				} else {
					msg_format(Ind, "\377o%^s resists the effect.", m_name);
				}
#endif
			}
#endif
		}
	}

	/* Assume we attacked */
	return(TRUE);
}
