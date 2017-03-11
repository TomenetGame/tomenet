/* $Id$ */
/* File: misc.c */

/* Purpose: misc code */

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
 * Modifier for martial-arts AC bonus; it's needed to balance martial-arts
 * and dodging skills. in percent. [50]
 */
#define MARTIAL_ARTS_AC_ADJUST	50

/* Hack: Martial arts to-damage bonus is partially counted as 'imaginary object +dam bonus' instead of purely 'skill bonus'?
   This is relevant for shapeshifters, because to_d_melee gets averaged, while to_d (from weapons) gets flat added!
   This is a pretty ugly hack. :( */
#define MARTIAL_TO_D_HACK
/* Mimic +dam bonus from forms takes a 'dent' in the slightly above-average region? (Recommended)
   This helps Jabberwock form to shine vs more "08/15" forms such as Maulotaur in terms of damage output! */
#ifdef MARTIAL_TO_D_HACK
 #define MIMIC_TO_D_DENTHACK /* optional */
#else
 #define MIMIC_TO_D_DENTHACK /* should always be on */
#endif

/* Announce global events every 900 seconds */
#define GE_ANNOUNCE_INTERVAL 900

/* Seconds left when to fire a final announcement */
#define GE_FINAL_ANNOUNCEMENT 300 /* 240 */

/* Allow ego monsters in Arena Challenge global event? */
#define GE_ARENA_ALLOW_EGO

/* Experimental and also silly ;) - reward players for wearing arts of similar name - C. Blue */
#define EQUIPMENT_SET_BONUS

/* Do not lower HP of mimics if the monster form has lower HP than their @ form. - C. Blue 
   Currently also works for to-dam, could be extended to even Speed maybe. Shouldn't be extended onto AC. */
#define MIMICRY_BOOST_WEAK_FORM

/* Enable to allow '..' inscription, for suppressing TELEPORT flag on non-cursed items.
   This is a bit ugly and there is no reason to have this enabled as long as the
   'Thunderlords' ego doesn't have TELEPORT flag (which was just removed). */
//#define TOGGLE_TELE



/*
 * Converts stat num into a six-char (right justified) string
 */
void cnv_stat(int val, char *out_val) {
	/* Above 18 */
	if (val > 18) {
		int bonus = (val - 18);

		if (bonus >= 220)
			sprintf(out_val, "18/%3s", "***");
		else if (bonus >= 100)
			sprintf(out_val, "18/%03d", bonus);
		else
			sprintf(out_val, " 18/%02d", bonus);
	}

	/* From 3 to 18 */
	else
		sprintf(out_val, "    %2d", val);
}



/*
 * Modify a stat value by a "modifier", return new value
 *
 * Stats go up: 3,4,...,17,18,18/10,18/20,...,18/220
 * Or even: 18/13, 18/23, 18/33, ..., 18/220
 *
 * Stats go down: 18/220, 18/210,..., 18/10, 18, 17, ..., 3
 * Or even: 18/13, 18/03, 18, 17, ..., 3
 */
s16b modify_stat_value(int value, int amount) {
	int i;

	/* Reward */
	if (amount > 0) {
		/* Apply each point */
		for (i = 0; i < amount; i++) {
			/* One point at a time */
			if (value < 18) value++;

			/* Ten "points" at a time */
			else value += 10;
		}
	}

	/* Penalty */
	else if (amount < 0) {
		/* Apply each point */
		for (i = 0; i < (0 - amount); i++) {
			/* Ten points at a time */
			if (value >= 18+10) value -= 10;

			/* Hack -- prevent weirdness */
			else if (value > 18) value = 18;

			/* One point at a time */
			else if (value > 3) value--;
		}
	}

	/* Return new value */
	return (value);
}





/*
 * Print character stat in given row, column
 */
static void prt_stat(int Ind, int stat) {
	player_type *p_ptr = Players[Ind];
	Send_stat(Ind, stat, p_ptr->stat_top[stat], p_ptr->stat_use[stat], p_ptr->stat_ind[stat], p_ptr->stat_max[stat]);
}




/*
 * Prints "title", including "wizard" or "winner" as needed.
 */
static void prt_title(int Ind) {
	player_type *p_ptr = Players[Ind];
	cptr p = "";

	/* Ghost */
	if (p_ptr->ghost) p = "\377rGhost (dead)";

	/* Winner */
#if 0
	else if (p_ptr->total_winner || (p_ptr->lev > PY_MAX_LEVEL)) {
		if (p_ptr->mode & (MODE_HARD | MODE_NO_GHOST))
			p = (p_ptr->male ? "**EMPEROR**" : "**EMPRESS**");
		else
			p = (p_ptr->male ? "**KING**" : "**QUEEN**");
	}
#else
	else if (p_ptr->total_winner || (p_ptr->lev > PY_MAX_LEVEL)) {
		char t[MAX_CHARS];
		strcpy(t, "\377v");
		strcat(t, get_ptitle(p_ptr, TRUE));
		Send_title(Ind, t);
		return;
	}
#endif
	/* Normal */
	else p = get_ptitle(p_ptr, TRUE);

	Send_title(Ind, p);
}


/*
 * Prints level
 */
static void prt_level(int Ind) {
	player_type *p_ptr = Players[Ind];
	s64b adv_exp, adv_exp_prev = 0;

	if (p_ptr->lev >= (is_admin(p_ptr) ? PY_MAX_LEVEL : PY_MAX_PLAYER_LEVEL))
		adv_exp = 0;
#ifndef ALT_EXPRATIO
	else adv_exp = (s64b)((s64b)player_exp[p_ptr->lev - 1] * (s64b)p_ptr->expfact / 100L);
#else
	else adv_exp = (s64b)player_exp[p_ptr->lev - 1];
#endif

	if (p_ptr->lev > 1)
#ifndef ALT_EXPRATIO
		adv_exp_prev = (s64b)((s64b)player_exp[p_ptr->lev - 2] * (s64b)p_ptr->expfact / 100L);
#else
		adv_exp_prev = (s64b)player_exp[p_ptr->lev - 2];
#endif

	Send_experience(Ind, p_ptr->lev, p_ptr->max_exp, p_ptr->exp, adv_exp, adv_exp_prev);
}


/*
 * Display the experience
 */
static void prt_exp(int Ind) {
	player_type *p_ptr = Players[Ind];
	s64b adv_exp, adv_exp_prev = 0;

	if (p_ptr->lev >= (is_admin(p_ptr) ? PY_MAX_LEVEL : PY_MAX_PLAYER_LEVEL))
		adv_exp = 0;
#ifndef ALT_EXPRATIO
	else adv_exp = (s64b)((s64b)player_exp[p_ptr->lev - 1] * (s64b)p_ptr->expfact / 100L);
#else
	else adv_exp = (s64b)player_exp[p_ptr->lev - 1];
#endif

	if (p_ptr->lev > 1)
#ifndef ALT_EXPRATIO
		adv_exp_prev = (s64b)((s64b)player_exp[p_ptr->lev - 2] * (s64b)p_ptr->expfact / 100L);
#else
		adv_exp_prev = (s64b)player_exp[p_ptr->lev - 2];
#endif

	Send_experience(Ind, p_ptr->lev, p_ptr->max_exp, p_ptr->exp, adv_exp, adv_exp_prev);
}


/*
 * Prints current gold
 */
static void prt_gold(int Ind) {
	player_type *p_ptr = Players[Ind];

	Send_gold(Ind, p_ptr->au, p_ptr->balance);
}



/*
 * Prints current AC
 */
static void prt_ac(int Ind) {
	player_type *p_ptr = Players[Ind];

	Send_ac(Ind, p_ptr->dis_ac, p_ptr->dis_to_a);
}

static void prt_sanity(int Ind) {
#ifdef SHOW_SANITY	// No.
	player_type *p_ptr = Players[Ind];
#if 0
	Send_sanity(Ind, p_ptr->msane, p_ptr->csane);
#else	// 0
	char buf[20];
	byte attr = TERM_L_GREEN;
	int skill = get_skill(p_ptr, SKILL_HEALTH);
	int ratio;
	ratio = p_ptr->msane ? (p_ptr->csane * 100) / p_ptr->msane : 100;

	/* Mindcrafters get better sanity display for free by levelling up */
	if (p_ptr->pclass == CLASS_MINDCRAFTER &&
	    p_ptr->lev >= skill)
		skill = p_ptr->lev;

	/* Vague */
	if (ratio < 0) {
		/* This guy should be dead - for tombstone */
		attr = TERM_RED;
		strcpy(buf, "Vegetable");
	} else if (ratio < 10) {
//		attr = TERM_RED;
		attr = TERM_MULTI;
		strcpy(buf, "      MAD");
	} else if (ratio < 25) {
		attr = TERM_SHIELDI;
		strcpy(buf, "   Insane");
	} else if (ratio < 50) {
		attr = TERM_ORANGE;
		strcpy(buf, "   Crazy");
	} else if (ratio < 75) {
		attr = TERM_YELLOW;
		strcpy(buf, "    Weird");
	} else if (ratio < 90) {
		attr = TERM_GREEN;
		strcpy(buf, "     Sane");
	} else {
		attr = TERM_L_GREEN;
		strcpy(buf, "    Sound");
	}

	switch (p_ptr->sanity_bar) {
	case 3: /* Full */
		snprintf(buf, sizeof(buf), "%4d/%4d", p_ptr->csane, p_ptr->msane);
		break;
	case 2: /* Percentile */
		snprintf(buf, sizeof(buf), "     %3d%%", ratio);
		break;
	case 1: /* Sanity Bar */
		{
			int tmp = ratio / 11;
			strcpy(buf, "---------");
			if (tmp > 0) strncpy(buf, "*********", tmp);
			break;
		}
	}
	/* Terminate */
	buf[9] = '\0';

	/* Send it */
	Send_sanity(Ind, attr, buf);

#endif	// 0
#endif	// SHOW_SANITY
}

/*
 * Prints Cur/Max hit points
 */
static void prt_hp(int Ind) {
	player_type *p_ptr = Players[Ind];

	Send_hp(Ind, p_ptr->mhp, p_ptr->chp);
}

/*
 * Prints Cur/Max stamina points
 */
static void prt_stamina(int Ind) {
	player_type *p_ptr = Players[Ind];

	Send_stamina(Ind, p_ptr->mst, p_ptr->cst);
}

/*
 * Prints players max/cur spell points
 */
static void prt_sp(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Do not show mana unless it matters */
	Send_sp(Ind, p_ptr->msp, p_ptr->csp);
}


/*
 * Prints depth in stat area
 */
static void prt_depth(int Ind) {
	player_type *p_ptr = Players[Ind];

	Send_depth(Ind, &p_ptr->wpos);
}


/*
 * Prints status of hunger
 */
static void prt_hunger(int Ind) {
	player_type *p_ptr = Players[Ind];

	Send_food(Ind, p_ptr->food);
}


/*
 * Prints Blind status
 */
static void prt_blind(int Ind) {
	player_type *p_ptr = Players[Ind];

	if (p_ptr->blind) Send_blind(Ind, TRUE);
	else Send_blind(Ind, FALSE);
}


/*
 * Prints Confusion status
 */
static void prt_confused(int Ind) {
	player_type *p_ptr = Players[Ind];

	if (p_ptr->confused) Send_confused(Ind, TRUE);
	else Send_confused(Ind, FALSE);
}


/*
 * Prints Fear status
 */
static void prt_afraid(int Ind) {
	player_type *p_ptr = Players[Ind];

	if (p_ptr->afraid) Send_fear(Ind, TRUE);
	else Send_fear(Ind, FALSE);
}


/*
 * Prints Poisoned status
 */
static void prt_poisoned(int Ind) {
	player_type *p_ptr = Players[Ind];

	if (p_ptr->poisoned) Send_poison(Ind, TRUE);
	else Send_poison(Ind, FALSE);
}


/*
 * Prints Searching, Resting, Paralysis, or 'count' status
 * Display is always exactly 10 characters wide (see below)
 *
 * This function was a major bottleneck when resting, so a lot of
 * the text formatting code was optimized in place below.
 */
static void prt_state(int Ind) {
	player_type *p_ptr = Players[Ind];

	bool p, s, r;

	/* Paralysis */
	if (p_ptr->paralyzed) p = TRUE;
	else p = FALSE;

	/* Searching */
	if (p_ptr->searching) s = TRUE;
	else s = FALSE;

	/* Resting */
	if (p_ptr->resting) r = TRUE;
	else r = FALSE;

	Send_state(Ind, p, s, r);
}


/*
 * Prints the speed of a character.			-CJS-
 */
static void prt_speed(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i = p_ptr->pspeed;

#if 0	/* methinks we'd better tell it to players.. - Jir - */
	/* Hack -- Visually "undo" the Search Mode Slowdown */
	/* And this formula can be wrong for hellish */
//	if (p_ptr->searching) i += (p_ptr->mode & MODE_HARD ? 5 : 10);
	if (p_ptr->searching) i += 10;
#endif	// 0

	Send_speed(Ind, i - 110);
}


static void prt_study(int Ind) {
	player_type *p_ptr = Players[Ind];

	if (p_ptr->skill_points) Send_study(Ind, TRUE);
	else Send_study(Ind, FALSE);
}


static void prt_bpr(int Ind) {
	player_type *p_ptr = Players[Ind];
	byte attr = p_ptr->num_blow ? TERM_L_GREEN : TERM_RED;

	if (p_ptr->tim_wraith) {
		Send_bpr(Ind, 255, TERM_WHITE);
		return;
	}

	switch (p_ptr->pclass) {
	case CLASS_WARRIOR:
	case CLASS_MIMIC:
#ifdef ENABLE_DEATHKNIGHT
	case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
	case CLASS_HELLKNIGHT:
#endif
	case CLASS_PALADIN:
	case CLASS_RANGER:
	case CLASS_ROGUE:
	case CLASS_MINDCRAFTER:
		if (p_ptr->num_blow == 1) attr = TERM_ORANGE;
		else if (p_ptr->num_blow == 2) attr = TERM_YELLOW;
		break;
	case CLASS_SHAMAN:
	case CLASS_ADVENTURER:
	case CLASS_RUNEMASTER:
#ifdef ENABLE_CPRIEST
	case CLASS_CPRIEST:
#endif
	case CLASS_PRIEST:
	case CLASS_DRUID:
		if (p_ptr->num_blow == 1) attr = TERM_YELLOW;
		break;
	case CLASS_MAGE:
	case CLASS_ARCHER:
		break;
	}

	Send_bpr(Ind, p_ptr->num_blow, attr);
}


static void prt_cut(int Ind) {
	player_type *p_ptr = Players[Ind];
//	cave_type **zcave;
	int c = p_ptr->cut;

#if 0 /* deprecated */
	/* hack: no-tele indicator takes priority. */
	if ((zcave = getcave(&p_ptr->wpos)))
		if (zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) return;
#endif
	Send_cut(Ind, c); /* need to send this always since the client doesn't clear the whole field */
}



static void prt_stun(int Ind) {
	player_type *p_ptr = Players[Ind];
	int s = p_ptr->stun;

	Send_stun(Ind, s);
}

static void prt_history(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i;

	for (i = 0; i < 4; i++)
		Send_history(Ind, i, p_ptr->history[i]);
}

static void prt_various(int Ind) {
	player_type *p_ptr = Players[Ind];

	Send_various(Ind, p_ptr->ht, p_ptr->wt, p_ptr->age, p_ptr->sc, r_name + r_info[p_ptr->body_monster].name);
}

static void prt_plusses(int Ind) {
	player_type *p_ptr = Players[Ind];
	int bmh = 0, bmd = 0;

	int show_tohit_m = p_ptr->dis_to_h + p_ptr->to_h_melee;
	int show_todam_m = p_ptr->dis_to_d + p_ptr->to_d_melee;
/*	int show_tohit_m = p_ptr->to_h_melee;
	int show_todam_m = p_ptr->to_d_melee;
*/
	int show_tohit_r = p_ptr->dis_to_h + p_ptr->to_h_ranged;
	int show_todam_r = p_ptr->to_d_ranged;

	/* well, about dual-wield.. we can only display the boni of one weapon or their average until we add another line
	   to squeeze info about the secondary weapon there too. so for now let's just stick with this. - C. Blue */
	object_type *o_ptr = &p_ptr->inventory[INVEN_WIELD];
	object_type *o_ptr2 = &p_ptr->inventory[INVEN_BOW];
	object_type *o_ptr3 = &p_ptr->inventory[INVEN_AMMO];
	object_type *o_ptr4 = &p_ptr->inventory[INVEN_ARM];

	/* Hack -- add in weapon info if known */
        if (object_known_p(Ind, o_ptr)) {
		bmh += o_ptr->to_h;
		bmd += o_ptr->to_d;
	}
	if (object_known_p(Ind, o_ptr2)) {
		show_tohit_r += o_ptr2->to_h;
		show_todam_r += o_ptr2->to_d;
	}
	if (object_known_p(Ind, o_ptr3)) {
		show_tohit_r += o_ptr3->to_h;
		show_todam_r += o_ptr3->to_d;
	}
	/* dual-wield..*/
	if (o_ptr4->k_idx && o_ptr4->tval != TV_SHIELD && p_ptr->dual_mode) {
		if (object_known_p(Ind, o_ptr4)) {
			bmh += o_ptr4->to_h;
			bmd += o_ptr4->to_d;
		}
		if (object_known_p(Ind, o_ptr) && object_known_p(Ind, o_ptr4)) {
			/* average of both */
			bmh /= 2;
			bmd /= 2;
		}
	}
	show_tohit_m += bmh;
	show_todam_m += bmd;

//	Send_plusses(Ind, show_tohit_m, show_todam_m, show_tohit_r, show_todam_r, p_ptr->to_h_melee, p_ptr->to_d_melee);
	Send_plusses(Ind, 0, 0, show_tohit_r, show_todam_r, show_tohit_m, show_todam_m);

	/* (not game-play relevant, just for easier handling in LUA scripts:) */
	p_ptr->overall_tohit_r = show_tohit_r;
	p_ptr->overall_todam_r = show_todam_r;
	p_ptr->overall_tohit_m = show_tohit_m;
	p_ptr->overall_todam_m = show_todam_m;
}

static void prt_skills(int Ind) {
	Send_skills(Ind);
}

static void prt_AFK(int Ind) {
	player_type *p_ptr = Players[Ind];
	byte afk = (p_ptr->afk ? 1 : 0);

	Send_AFK(Ind, afk);
}

static void prt_encumberment(int Ind) {
	player_type *p_ptr = Players[Ind];

	byte cumber_armor = p_ptr->cumber_armor ? 1 : 0;
	byte awkward_armor = p_ptr->awkward_armor ? 1 : 0;
	/* Hack - For mindcrafters, it's the helmet, not the gloves,
	   but they fortunately use the same item symbol :) */
	byte cumber_glove = p_ptr->cumber_glove || p_ptr->cumber_helm ? 1 : 0;
	byte heavy_wield = p_ptr->heavy_wield ? 1 : 0;
	byte heavy_shield = p_ptr->heavy_shield ? 1 : 0; /* added in 4.4.0f */
	byte heavy_shoot = p_ptr->heavy_shoot ? 1 : 0;
	byte icky_wield = p_ptr->icky_wield ? 1 : 0;
	byte awkward_wield = p_ptr->awkward_wield ? 1 : 0;
	byte easy_wield = p_ptr->easy_wield ? 1 : 0;
	byte cumber_weight = p_ptr->cumber_weight ? 1 : 0;
	/* See next line. Also, we're already using all 12 spaces we have available for icons.
	byte rogue_heavy_armor = p_ptr->rogue_heavyarmor ? 1 : 0; */
	 /* Hack - MA also gives dodging, which relies on rogue_heavy_armor anyway. */
	byte monk_heavyarmor, rogue_heavyarmor = 0;
	byte awkward_shoot = p_ptr->awkward_shoot ? 1 : 0;
	bool heavy_swim = p_ptr->heavy_swim ? 1 : 0;
	if (!is_newer_than(&p_ptr->version, 4, 4, 2, 0, 0, 0)) {
		monk_heavyarmor = (p_ptr->monk_heavyarmor || p_ptr->rogue_heavyarmor) ? 1 : 0;
	} else {
		monk_heavyarmor = p_ptr->monk_heavyarmor ? 1 : 0;
		rogue_heavyarmor = p_ptr->rogue_heavyarmor ? 1 : 0;
	}

	if (p_ptr->pclass == CLASS_WARRIOR || p_ptr->pclass == CLASS_ARCHER) {
		awkward_armor = 0; /* they don't use magic or SP */
/* todo: make cumber_glove and awkward_armor either both get checked here, or in xtra1.c,
   not one here, one in xtra1.c .. (cleaning-up measurements) */
	}

	Send_encumberment(Ind, cumber_armor, awkward_armor, cumber_glove, heavy_wield, heavy_shield, heavy_shoot,
	    icky_wield, awkward_wield, easy_wield, cumber_weight, monk_heavyarmor, rogue_heavyarmor, awkward_shoot, heavy_swim);
}

static void prt_extra_status(int Ind) {
	player_type *p_ptr = Players[Ind];
	char status[12 + 24]; /* 24 for potential colours */

	if (!is_newer_than(&p_ptr->version, 4, 4, 1, 5, 0, 0)) return;

	/* add combat stance indicator */
	if (get_skill(p_ptr, SKILL_STANCE)) {
		switch(p_ptr->combat_stance) {
		case 0: strcpy(status, "\377sBl ");
			break;
		case 1: strcpy(status, "\377uDf ");
			break;
		case 2: strcpy(status, "\377oOf ");
			break;
		}
	} else strcpy(status, "   ");

	/* add dual-wield mode indicator */
	if (get_skill(p_ptr, SKILL_DUAL) && p_ptr->dual_wield) {
		if (p_ptr->dual_mode)
			strcat(status, "\377sDH ");
		else
			strcat(status, "\377DMH ");
	} else strcat(status, "   ");

	/* add fire-till-kill indicator */
	if (p_ptr->shoot_till_kill)
		strcat(status, "\377sFK ");
	else
		strcat(status, "   ");

	/* add project-spells indicator */
	if (p_ptr->spell_project)
		strcat(status, "\377sPj ");
	else
		strcat(status, "   ");

	Send_extra_status(Ind, status);
}


/*
 * Redraw the "monster health bar"	-DRS-
 * Rather extensive modifications by	-BEN-
 *
 * The "monster health bar" provides visual feedback on the "health"
 * of the monster currently being "tracked".  There are several ways
 * to "track" a monster, including targetting it, attacking it, and
 * affecting it (and nobody else) with a ranged attack.
 *
 * Display the monster health bar (affectionately known as the
 * "health-o-meter").  Clear health bar if nothing is being tracked.
 * Auto-track current target monster when bored.  Note that the
 * health-bar stops tracking any monster that "disappears".
 */

static void health_redraw(int Ind) {
	player_type *p_ptr = Players[Ind];

#ifdef DRS_SHOW_HEALTH_BAR

	/* Not tracking */
	if (p_ptr->health_who == 0) {
		/* Erase the health bar */
		Send_monster_health(Ind, 0, 0);
	}

	/* Tracking a hallucinatory monster */
	else if (p_ptr->image) {
		/* Indicate that the monster health is "unknown" */
		Send_monster_health(Ind, 0, TERM_WHITE);
	}

	/* Tracking a player */
	else if (p_ptr->health_who < 0) {
		player_type *q_ptr = Players[0 - p_ptr->health_who];

		if (Players[Ind]->conn == NOT_CONNECTED) {
			/* Send_monster_health(Ind, 0, 0); */
			return;
		}

		if (0 - p_ptr->health_who <= NumPlayers) {
			if(Players[0-p_ptr->health_who]->conn == NOT_CONNECTED ) {
				Send_monster_health(Ind, 0, 0);
				return;
			};
		} else {
			Send_monster_health(Ind, 0, 0);
			return;
		}


		/* Tracking a bad player (?) */
		if (!q_ptr) {
			/* Erase the health bar */
			Send_monster_health(Ind, 0, 0);
		}

		/* Tracking an unseen player */
		else if (!p_ptr->play_vis[0 - p_ptr->health_who] && !is_admin(p_ptr)) {
			/* Indicate that the player health is "unknown" */
			Send_monster_health(Ind, 0, TERM_WHITE);
		}

		/* Tracking a visible player */
		else {
			int pct, len;

			/* Default to almost dead */
			byte attr = TERM_RED;

			/* Extract the "percent" of health */
			pct = 100L * q_ptr->chp / q_ptr->mhp;

			/* Badly wounded */
			if (pct >= 10) attr = TERM_L_RED;

			/* Wounded */
			if (pct >= 25) attr = TERM_ORANGE;

			/* Somewhat Wounded */
			if (pct >= 60) attr = TERM_YELLOW;

			/* Healthy */
			if (pct >= 100) attr = TERM_L_GREEN;

			/* Afraid */
			if (q_ptr->afraid) attr = TERM_VIOLET;

			/* Asleep (?) */
			if (q_ptr->paralyzed) attr = TERM_BLUE;

			/* Administrative invulnerability? */
			if (q_ptr->admin_invinc || q_ptr->admin_invuln) attr = TERM_L_UMBER;

			/* Convert percent into "health" */
			len = (pct < 10) ? 1 : (pct < 90) ? (pct / 10 + 1) : 10;

			/* Send the health */
			Send_monster_health(Ind, len, attr);
		}
	}

	/* Tracking a bad monster (?) */
	else if (!m_list[p_ptr->health_who].r_idx) {
		/* Erase the health bar */
		Send_monster_health(Ind, 0, 0);
	}

	/* Tracking an unseen monster */
	else if (!p_ptr->mon_vis[p_ptr->health_who] && !is_admin(p_ptr)) {
		/* Indicate that the monster health is "unknown" */
		Send_monster_health(Ind, 0, TERM_WHITE);
	}

	/* Tracking a dead monster (???) */
	else if (m_list[p_ptr->health_who].hp < 0) {
		/* Indicate that the monster health is "unknown" */
		Send_monster_health(Ind, 0, TERM_WHITE);
	}

	/* Tracking a visible monster */
	else {
		int pct, len;

		monster_type *m_ptr = &m_list[p_ptr->health_who];

		/* Default to almost dead */
		byte attr = TERM_RED;

		/* Crash once occurred here, m_ptr->hp -296, m_ptr->maxhp 0 - C. Blue
		   --also occurred in xtra2.c:8237, mon_take_hit() */
		if (m_ptr->maxhp == 0) {
			Send_monster_health(Ind, 0, 0);
			s_printf("DBG_MAXHP_4 %d,%d\n", m_ptr->r_idx, m_ptr->ego);
			return;
		}

		/* Extract the "percent" of health */
		pct = 100L * m_ptr->hp / m_ptr->maxhp;

		/* Badly wounded */
		if (pct >= 10) attr = TERM_L_RED;

		/* Wounded */
		if (pct >= 25) attr = TERM_ORANGE;

		/* Somewhat Wounded */
		if (pct >= 60) attr = TERM_YELLOW;

		/* Healthy */
		if (pct >= 100) attr = TERM_L_GREEN;

		/* Afraid */
		if (m_ptr->monfear) attr = TERM_VIOLET;

		/* Asleep */
		if (m_ptr->csleep) attr = TERM_BLUE;

		/* Monster never dies? */
		if ((r_info[m_ptr->r_idx].flags7 & RF7_NO_DEATH)) attr = TERM_L_UMBER;

		/* Convert percent into "health" */
		len = (pct < 10) ? 1 : (pct < 90) ? (pct / 10 + 1) : 10;

		/* Send the health */
		Send_monster_health(Ind, len, attr);
	}

#endif

}



/*
 * Display basic info (mostly left of map)
 */
static void prt_frame_basic(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i;

	/* Race and Class */
	Send_char_info(Ind, p_ptr->prace, p_ptr->pclass, p_ptr->ptrait, p_ptr->male, p_ptr->mode, p_ptr->name);

	/* Title */
	prt_title(Ind);

	/* Level/Experience */
	prt_level(Ind);
	prt_exp(Ind);

	/* All Stats */
	for (i = 0; i < 6; i++) prt_stat(Ind, i);

	/* Armor */
	prt_ac(Ind);

	/* Hitpoints */
	prt_hp(Ind);

	/* Sanity */
#ifdef SHOW_SANITY
	prt_sanity(Ind);
#endif

	/* Spellpoints */
	prt_sp(Ind);

	/* Stamina */
	prt_stamina(Ind);

	/* Gold */
	prt_gold(Ind);

	/* Current depth */
	prt_depth(Ind);

	/* Special */
	health_redraw(Ind);
}


/*
 * Display extra info (mostly below map)
 */
static void prt_frame_extra(int Ind) {
#if 0 /* deprecated */
	/* Mega-Hack : display AFK status in place of 'stun' status! */
#endif
	prt_AFK(Ind);

	/* Give monster health priority over AFK status display */
	health_redraw(Ind);

	/* Cut/Stun */
	prt_cut(Ind);
#if 0 /* deprecated */
	/* Mega-Hack : AFK and Stun share a field */
	if (Players[Ind]->stun || !Players[Ind]->afk) prt_stun(Ind);
#else
	prt_stun(Ind);
#endif

	/* Food */
	prt_hunger(Ind);

	/* Various */
	prt_blind(Ind);
	prt_confused(Ind);
	prt_afraid(Ind);
	prt_poisoned(Ind);

	/* State */
	prt_state(Ind);

	/* Speed */
	prt_speed(Ind);

	if (is_older_than(&Players[Ind]->version, 4, 4, 8, 5, 0, 0))
		/* Study spells */
		prt_study(Ind);
	else
		/* Blows/Round */
		prt_bpr(Ind);
}


/*
 * Hack -- display inventory in sub-windows
 */
static void fix_inven(int Ind) {
	/* Resend the inventory */
	display_inven(Ind);
}

/*
 * Hack -- display equipment in sub-windows
 */
static void fix_equip(int Ind) {
	/* Resend the equipment */
	display_equip(Ind);
}

/*
 * Hack -- display character in sub-windows
 */
static void fix_player(int Ind) {
}

static void fix_allitems(int Ind) {
	/* Resend the inventory */
	display_invenequip(Ind, FALSE);
}

static void fix_allitems_fwd(int Ind) {
	/* Resend the inventory */
	display_invenequip(Ind, TRUE);
}


/*
 * Hack -- display recent messages in sub-windows
 *
 * XXX XXX XXX Adjust for width and split messages
 */
static void fix_message(int Ind) {
}


/*
 * Hack -- display overhead view in sub-windows
 *
 * Note that the "player" symbol does NOT appear on the map.
 */
static void fix_overhead(int Ind) {
}


/*
 * Hack -- display monster recall in sub-windows
 */
static void fix_monster(int Ind) {
}

/*
 * Calculate the player's sanity
 */

static void calc_sanity(int Ind) {
	player_type *p_ptr = Players[Ind];
	int bonus, msane;
	/* Don't make the capacity too large */
	int lev = p_ptr->lev > 50 ? 50 : p_ptr->lev;

	/* Hack -- use the con/hp table for sanity/wis */
	bonus = ((int)(adj_wis_msane[p_ptr->stat_ind[A_WIS]]) - 128);

	/* Hack -- assume 5 sanity points per level. */
	msane = 5*(lev+1) + (bonus * lev / 2);

	if (msane < lev + 1) msane = lev + 1;

	if (p_ptr->msane != msane) {

		/* Sanity carries over between levels. */
		p_ptr->csane += (msane - p_ptr->msane);
		/* If sanity just dropped to 0 or lower, die! */
		if (p_ptr->csane < 0) {
			if (!p_ptr->safe_sane) {
				/* Hack -- Note death */
				msg_print(Ind, "\377vYou turn into an unthinking vegetable.");
				(void)strcpy(p_ptr->died_from, "insanity");
				(void)strcpy(p_ptr->really_died_from, "insanity");
				if (!p_ptr->ghost) {
					strcpy(p_ptr->died_from_list, "insanity");
					p_ptr->died_from_depth = getlevel(&p_ptr->wpos);
					/* Hack to remember total winning */
					if (p_ptr->total_winner) strcat(p_ptr->died_from_list, "\001");
				}
				/* No longer a winner */
//				p_ptr->total_winner = FALSE;
				/* Note death */
				p_ptr->death = TRUE;
				p_ptr->deathblow = 0;
			} else {
				p_ptr->csane = 0;
			}
		}
		
		p_ptr->msane = msane;

		if (p_ptr->csane >= msane) {
			p_ptr->csane = msane;
			p_ptr->csane_frac = 0;
		}

		p_ptr->redraw |= (PR_SANITY);
		p_ptr->window |= (PW_PLAYER);
	}
}


/*
 * Calculate maximum mana.  You do not need to know any spells.
 * Note that mana is lowered by heavy (or inappropriate) armor.
 *
 * This function induces status messages.
 */
//static void calc_mana(int Ind)
void calc_mana(int Ind) {
	player_type *p_ptr = Players[Ind];
	player_type *p_ptr2 = NULL; /* silence the warning */
	int Ind2 = get_esp_link(Ind, LINKF_PAIN, &p_ptr2);

	int levels, cur_wgt, max_wgt, tmp_lev;
	s32b new_mana = 0;

	object_type *o_ptr;
	u32b f1, f2, f3, f4, f5, f6, esp;


	/* Extract "effective" player level */
	tmp_lev = p_ptr->lev * 10;
	if (p_ptr->lev <= 50) levels = tmp_lev;
	/* Less additional mana gain for each further post-king level */
	else if (p_ptr->lev <= 70) levels = 500 + (tmp_lev - 500) / 2;
	else if (p_ptr->lev <= 85) levels = 500 + 100 + (tmp_lev - 700) / 3;
	else levels = 500 + 100 + 50 + (tmp_lev - 850) / 4;

	/* Hack -- no negative mana */
	if (levels < 0) levels = 0;

	/* Extract total mana */
	switch(p_ptr->pclass) {
	case CLASS_MAGE:
		/* much Int, few Wis */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 85 * levels  +
			    adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 15 * levels) / 3000;
		break;
	case CLASS_RANGER:
		/* much Int, few Wis --180 */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 85 * levels +
			    adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 15 * levels) / 5000;
		break;
#ifdef ENABLE_CPRIEST
	case CLASS_CPRIEST:
#endif
	case CLASS_PRIEST:
		/* few Int, much Wis --170 */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 15 * levels +
			    adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 85 * levels) / 3750;
		break;
	case CLASS_DRUID:
		/* few Int, much Wis --170 */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 15 * levels +
			    adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 85 * levels) / 4000;
		break;
#ifdef ENABLE_DEATHKNIGHT
	case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
	case CLASS_HELLKNIGHT:
#endif
	case CLASS_PALADIN:
		/* few Int, much Wis --140 */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 15 * levels +
			    adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 85 * levels) / 5500;
		break;
	case CLASS_ROGUE:
		/* much Int, few Wis --160 */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 85 * levels +
			    adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 15 * levels) / 5500;
	case CLASS_MIMIC:
		/* much Int, few Wis --160 */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 85 * levels +
			    adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 15 * levels) / 5000;
		break;
	case CLASS_ARCHER:
	case CLASS_WARRIOR:
		new_mana = 0;
		break;
	case CLASS_SHAMAN:
#if 0
		/* more Wis than Int --180 */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 35 * levels +
			    adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 65 * levels) / 4000;
#else
		/* Depends on what's better, his WIS or INT --180 */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    ((p_ptr->stat_ind[A_INT] > p_ptr->stat_ind[A_WIS]) ?
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 100 * levels) :
			    (adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 100 * levels)) / 4000;
#endif
		break; 
	case CLASS_RUNEMASTER:
		//Spells are now much closer in cost to mage spells. Returning to a similar mode
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
		    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 65 * levels +
		    adj_mag_mana[p_ptr->stat_ind[A_DEX]] * 35 * levels) / 3000;
		break;
	case CLASS_MINDCRAFTER:
		/* much Int, some Chr (yeah!), little Wis */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) + /* <- seems this might be important actually */
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 85 * levels +
			    adj_mag_mana[p_ptr->stat_ind[A_CHR]] * 10 * levels +
			    adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 5 * levels) / 4000;
		break;

	case CLASS_ADVENTURER:
//	case CLASS_BARD:
	default:
		/* 50% Int, 50% Wis --160 */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
		(adj_mag_mana[p_ptr->stat_ind[A_INT]] * 50 * levels +
		adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 50 * levels) / 5500;
		break;
	}


	/* --- apply +MANA --- */

	/* adjustment so paladins won't become OoD sentry guns and
	   rangers won't become invulnerable manashield tanks, and
	   priests won't become OoD wizards.. (C. Blue)
	   Removed ranger mana penalty here, added handicap in spells1.c
	   where disruption shield is calculated. (C. Blue) */
	switch(p_ptr->pclass) {
	case CLASS_MAGE:
	case CLASS_RANGER:
		if (p_ptr->to_m) new_mana += new_mana * p_ptr->to_m / 100;
		break;
	case CLASS_ADVENTURER:
	case CLASS_SHAMAN:
	case CLASS_DRUID:
	/* in theory these actually don't use 'magic mana' at all?: */
#ifdef ENABLE_CPRIEST
	case CLASS_CPRIEST:
#endif
	case CLASS_PRIEST: /* maybe Shamans are treated too good in comparison here */
		if (p_ptr->to_m) new_mana += new_mana * p_ptr->to_m / 130;
		break;
#ifdef ENABLE_DEATHKNIGHT
	case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
	case CLASS_HELLKNIGHT:
#endif
	case CLASS_PALADIN:
		if (p_ptr->to_m) new_mana += new_mana * p_ptr->to_m / 200;
		break;
	/* non-holy again: -- hm not sure if they still need to get reduced effect*/
	case CLASS_MIMIC:
	case CLASS_ROGUE:
#if 0
		if (p_ptr->to_m) new_mana += new_mana * p_ptr->to_m / 150;
#else /* why not.. */
		if (p_ptr->to_m) new_mana += new_mana * p_ptr->to_m / 100;
#endif
		break;
	/* hybrids & more */
	case CLASS_MINDCRAFTER:
	case CLASS_RUNEMASTER:
	default:
		if (p_ptr->to_m) new_mana += new_mana * p_ptr->to_m / 100;
		break;
	}


	/* --- apply flat MP boni --- */

	/* Hack -- usually add one mana */
	if (new_mana) new_mana++;

	/* EXPERIMENTAL: high mimicry skill further adds to mana pool */
	if (get_skill(p_ptr, SKILL_MIMIC) > 30)
		new_mana += (get_skill(p_ptr, SKILL_MIMIC) - 30) * 5;//1-pt resolution

	/* Meditation increase mana at the cost of hp */
	if (p_ptr->tim_meditation) new_mana += (new_mana * get_skill(p_ptr, SKILL_SORCERY)) / 100;


	/* --- apply flat mali --- */

	/* Disruption Shield now increases hp at the cost of mana */
	if (p_ptr->tim_manashield) {
		/* commented out (evileye for power) */
		/*new_mana -= new_mana / 2; */
	}


	/* --- apply mali (encumberment/awkward) --- */

	/* Get the gloves */
	o_ptr = &p_ptr->inventory[INVEN_HANDS];

	/* Examine the gloves */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	/* Only Sorcery/Magery users are affected */
	if (get_skill(p_ptr, SKILL_SORCERY) || get_skill(p_ptr, SKILL_MAGERY)) {
		/* Assume player is not encumbered by gloves */
		p_ptr->cumber_glove = FALSE;

		/* Normal gloves hurt mage-type spells */
		if (o_ptr->k_idx &&
		    !(f2 & TR2_FREE_ACT) && !(f1 & TR1_MANA) &&
		    !((f1 & TR1_DEX) && (o_ptr->pval > 0)) && 
		    !(o_ptr->tval == TV_GLOVES && o_ptr->sval == SV_SET_OF_ELVEN_GLOVES)) //Elven Gloves -> no penalty
		{
			/* Encumbered */
			p_ptr->cumber_glove = TRUE;

			/* Reduce mana */
			new_mana = (3 * new_mana) / 4;
		}
	}

	/* Mindcrafting is obstructed by heavy helmets (even non-tin foil) */
	/* Get the helm */
	o_ptr = &p_ptr->inventory[INVEN_HEAD];

	/* Only mindcraft-users are affected (CLASS_MINDCRAFTER) */
	if (get_skill(p_ptr, SKILL_PPOWER) ||
	    get_skill(p_ptr, SKILL_TCONTACT) ||
	    get_skill(p_ptr, SKILL_MINTRUSION))
	{
		/* Assume player is not encumbered by helm */
		p_ptr->cumber_helm = FALSE;

		/* too heavy helm? */
		if (o_ptr->weight > 40) {
			/* Encumbered */
			p_ptr->cumber_helm = TRUE;

			/* Reduce mana */
			new_mana = (3 * new_mana) / 4;
		}
	}


#if 1 /* now not anymore done in calc_boni (which is called before calc_mana) */
	/* Assume player not encumbered by armor */
	p_ptr->awkward_armor = FALSE;

	/* Weigh the armor */
	cur_wgt = worn_armour_weight(p_ptr);

	/* Determine the weight allowance */
//	max_wgt = 200 + get_skill_scale(p_ptr, SKILL_COMBAT, 250); break;
	switch (p_ptr->pclass) {
	case CLASS_MAGE: max_wgt = 150 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;
	case CLASS_RANGER: max_wgt = 240 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;
#ifdef ENABLE_CPRIEST
	case CLASS_CPRIEST:
#endif
	case CLASS_PRIEST: max_wgt = 250 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;
#ifdef ENABLE_DEATHKNIGHT
	case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
	case CLASS_HELLKNIGHT:
#endif
	case CLASS_PALADIN: max_wgt = 300 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;
	case CLASS_DRUID: max_wgt = 200 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;
	case CLASS_SHAMAN: max_wgt = 170 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;
	case CLASS_ROGUE: max_wgt = 200 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;
	case CLASS_RUNEMASTER: max_wgt = 230 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;/*was 270*/
	case CLASS_MIMIC: max_wgt = 280 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;
	case CLASS_ADVENTURER: max_wgt = 210 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;
//	case CLASS_MINDCRAFTER: max_wgt = 230 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;
	case CLASS_MINDCRAFTER: max_wgt = 260 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;
	case CLASS_WARRIOR:
	case CLASS_ARCHER:
	default: max_wgt = 1000; break;
	}

	/* Heavy armor penalizes mana */
//	if (((cur_wgt - max_wgt) / 10) > 0) {
	if ((cur_wgt - max_wgt) > 0) {
		/* Reduce mana */
//		new_mana -= ((cur_wgt - max_wgt) * 2 / 3);

		/* square root - C. Blue */
/*		long tmp, tmp2;
		tmp = (cur_wgt - max_wgt) * 1000;
		tmp2 = 1000;
		if (tmp > 1000) do {
			tmp *= 1000;
			tmp /= 1320;
			tmp2 *= 1149;
			tmp2 /= 1000;
		} while (tmp > 1000)
		tmp2 /= 1000;
		new_mana *= (10 - tmp2);
*/
		new_mana -= (new_mana * (cur_wgt - max_wgt > 100 ? 100 : cur_wgt - max_wgt)) / 100;

		/* Encumbered */
		p_ptr->awkward_armor = TRUE;
	}
#endif


	/* --- finalize --- */

	/* give at least 1 MP under normal circumstances */
	if (new_mana <= 0) new_mana = 1;

	if (Ind2) new_mana += p_ptr2->msp / 2;

	/* Istari being purely mana-based thanks to mana shield don't need @ form at all,
	   so vampire istari could get free permanent +5 speed from vampire bat form.
	   Prevent that here: */
	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster) new_mana /= 3; //for both, RI_VAMPIRE_BAT and RI_VAMPIRIC_MIST

	/* Some classes dont use mana */
	if ((p_ptr->pclass == CLASS_WARRIOR) ||
	    (p_ptr->pclass == CLASS_ARCHER))
		new_mana = 0;

#ifdef ARCADE_SERVER
	new_mana = 100;
#endif

	/* Maximum mana has changed */
	if (p_ptr->msp != new_mana) {
		/* Player has no mana now */
		if (!new_mana) {
			/* No mana left */
			p_ptr->csp = 0;
			p_ptr->csp_frac = 0;
		}

		/* Player had no mana, has some now */
		else if (!p_ptr->msp) {
			/* Reset mana */
#if 0 /* completely cheezable restoration */
			p_ptr->csp = new_mana;
#endif
			p_ptr->csp_frac = 0;
		}

		/* Player had some mana, adjust current mana */
		else {
			s32b value;

			/* change current mana proportionately to change of max mana, */
			/* divide first to avoid overflow, little loss of accuracy */
			value = ((((long)p_ptr->csp << 16) + p_ptr->csp_frac) /
				p_ptr->msp * new_mana);

			/* Extract mana components */
			p_ptr->csp = (value >> 16);
			p_ptr->csp_frac = (value & 0xFFFF);
		}

		/* Save new mana */
		p_ptr->msp = new_mana;

		/* Display mana later */
		p_ptr->redraw |= (PR_MANA);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);
	}

	/* Take note when "glove state" changes */
	if (p_ptr->old_cumber_glove != p_ptr->cumber_glove) {
		/* Message */
		if (p_ptr->cumber_glove)
			msg_print(Ind, "\377oYour covered hands feel unsuitable for spellcasting.");
		else
			msg_print(Ind, "\377gYour hands feel more suitable for spellcasting.");

		/* Save it */
		p_ptr->old_cumber_glove = p_ptr->cumber_glove;
	}

	/* Take note when "helm state" changes */
	if (p_ptr->old_cumber_helm != p_ptr->cumber_helm) {
		/* Message */
		if (p_ptr->cumber_helm)
			msg_print(Ind, "\377oYour heavy headgear feels unsuitable for mindcrafting.");
		else
			msg_print(Ind, "\377gYour headgear feels more suitable for mindcrafting.");

		/* Save it */
		p_ptr->old_cumber_helm = p_ptr->cumber_helm;
	}


	/* Take note when "armor state" changes */
	if (p_ptr->old_awkward_armor != p_ptr->awkward_armor) {
		if (p_ptr->pclass != CLASS_WARRIOR && p_ptr->pclass != CLASS_ARCHER) {
			/* Message */
			if (p_ptr->awkward_armor)
				msg_print(Ind, "\377oThe weight of your armour strains your spellcasting.");
			else
				msg_print(Ind, "\377gYou feel able to cast more freely.");
		}
		/* Save it */
		p_ptr->old_awkward_armor = p_ptr->awkward_armor;
	}

	/* refresh encumberment status line */
//	p_ptr->redraw |= PR_ENCUMBERMENT;// <- causes bad packet bugs when shopping
}



/*
 * Calculate the players (maximal) hit points
 * Adjust current hitpoints if necessary
 */
 
/* An option of giving mages an extra hit point per level has been added,
 * to hopefully facilitate them making it down to 1600ish and finding  
 * Constitution potions.  This should probably be changed to stop after level
 * 30.
 */

void calc_hitpoints(int Ind) {
	player_type *p_ptr = Players[Ind], *p_ptr2 = NULL; /* silence the warning */
	int player_hp_eff; /* replacement for accessing player_hp[] directly */

	//object_type *o_ptr;
	//u32b f1, f2, f3, f4, f5, f6, esp;

	int bonus, Ind2 = 0, cr_mhp = p_ptr->cp_ptr->c_mhp + p_ptr->rp_ptr->r_mhp;
	long mhp, mhp_playerform, weakling_boost;
	u32b mHPLim, finalHP;
	int bonus_cap, to_life;
	int rlev = r_info[p_ptr->body_monster].level;


	if ((Ind2 = get_esp_link(Ind, LINKF_PAIN, &p_ptr2))) {
	}

	/* do not increase a character's hit points too high post-king.
	   They will just become immortal and that's no fun either. */
	if (p_ptr->lev <= 50) player_hp_eff = p_ptr->player_hp[p_ptr->lev - 1]; /* the usual way */
	else {
		/* reduce post-king gain */
		player_hp_eff = p_ptr->player_hp[50 - 1];
		if (p_ptr->lev <= 70) player_hp_eff += (p_ptr->player_hp[p_ptr->lev - 1] - p_ptr->player_hp[50 - 1]) / 2;
		else {
			player_hp_eff += (p_ptr->player_hp[70 - 1] - p_ptr->player_hp[50 - 1]) / 2;
			if (p_ptr->lev <= 85) player_hp_eff += (p_ptr->player_hp[p_ptr->lev - 1] - p_ptr->player_hp[70 - 1]) / 3;
			else {
				player_hp_eff += (p_ptr->player_hp[85 - 1] - p_ptr->player_hp[70 - 1]) / 3;
				player_hp_eff += (p_ptr->player_hp[p_ptr->lev - 1] - p_ptr->player_hp[85 - 1]) / 4;
			}
		}
	}

	/* Un-inflate "half-hitpoint bonus per level" value */
	bonus = ((int)(adj_con_mhp[p_ptr->stat_ind[A_CON]]) - 128);

	/* Calculate hitpoints */
	if (!cfg.bonus_calc_type) {
		/* The traditional way.. */
		mhp = player_hp_eff + (bonus * p_ptr->lev / 2);
	} else {
		/* Don't exaggerate with HP on low levels (especially Ents) (bonus range is -5..+25) */
		bonus_cap = ((p_ptr->lev + 5) * (p_ptr->lev + 5)) / 100;
		if (bonus > bonus_cap) bonus = bonus_cap;

		/* And here I made the formula slightly more complex to fit better :> - C. Blue -
		   Explanation why I made If-clauses for Istari (Mage) and Yeeks:
		   - Istari are extremely low on HP compared to other classes,
		   but in order to reduce the insta-kill chance when trying to win
		   they get an out-of-line HP boost here when they come closer to
		   level 50, which starts to diminish again after they won and
		   further raise in levels!
		   - Yeeks do get the same boost as all the others, but since yeeks
		   hitdice ratio is extraordinarily bad it would nearly become
		   cancelled out by the added boost, so the boost is slightly reduced
		   for them (except for the ultra-weak Yeek Istari) to fit their
		   low exp% better. 
		   This can logically be done well by making IF-exceptions since the
		   help-kinging-boost is added and not multiplied by a character's
		   exp-ratio, and the race-hitpoint dice have no influence on it.
		   - The penalty is finely tuned to make Yeek Priests still a deal
		   tougher than Istari, even without sorcery, while keeping in account
		   the relation to all stronger classes as well. - C. Blue */
		mhp = (player_hp_eff * 2 * (20 + bonus)) / 45;

		/* I think it might be better to dimish the boost for yeeks after level 50 slowly towards level 100 
		   while granting the full boost at the critical level 50 instead of just a minor boost. Reason is:
		   They don't have Morgoth's crown yet so they are unlikely to reach *** CON, but after winning
		   they will be way stronger! On the other hand yeeks are already so weak that ultra-high Yeek
		   Istari could still be instakilled (max. sorcery skill) if the boost is diminished,
		   so only Yeek Mimics whose HP greatly reduces the low hitdice effect should be affected,
		   as well as Adventurer-Mimics.. since it cannot be skill-based (might change anytime) nor
		   class-based, we just punish all Yeeks, that's what they're for after all >;) */
		/* Slowly diminishing boost? */
		/* new (July 2008): take class+race hit dice into account, not just class hit dice */
		weakling_boost = (p_ptr->lev <= 50) ?
			(((p_ptr->lev * p_ptr->lev * p_ptr->lev) / 2500) * ((576 - cr_mhp * cr_mhp) + 105)) / 105 : /* <- full bonus */
#if 1
			/* this #if 1 is needed, because otherwise hdice differences would be too big in end-game :/
			In fact this is but a bad hack - what should be done is adjusting hdice tables for races/classes instead. */
			(50 * ((576 - cr_mhp * cr_mhp) + 105)) / 105; /* <- keep (!) full bonus for the rest of career up to level 99 */
#else
			/* currently discrepancies between yeek/human priest and ent warrior would be TOO big at end-game (>> level 50) */
			((100 - p_ptr->lev) * ((576 - cr_mhp * cr_mhp) + 105)) / 105; /* <- above 50, bonus is slowly diminishing again towards level 99 (PY_MAX_PLAYER_LEVEL) */
#endif

		/* Help very weak characters close to level 50 to avoid instakill on trying to win */
#if 0 /* not possible, because a weak character would LOSE HP when going from 50 -> 51 */
		mhp += (weakling_boost * 50) / ((p_ptr->pclass == CLASS_MAGE && p_ptr->lev > 50) ? p_ptr->lev : 50); /* Istari have disruption shield, they don't need HP for 'tanking' */
#else
		mhp += weakling_boost;
#endif
	}

	/* instead let's make it an option to weak chars, instead of further buffing chars with don't
	   need it at all (warriors, druids, mimics, etc). However, now chars can get this AND +LIFE items.
	   So in total it might be even more. A scale to 100 is hopefully ok. *experimental* */
	mhp += get_skill_scale(p_ptr, SKILL_HEALTH, 100);

#ifdef ENABLE_MAIA
	/* Extra bonus hp (2 per level) for the evil path */
	if (p_ptr->prace == RACE_MAIA && (p_ptr->ptrait == TRAIT_CORRUPTED) && p_ptr->lev >= 20) {
		mhp += (p_ptr->lev - 20) * 2;
	}
#endif

	/* Now we calculated the base player form mhp. Save it for use with
	   +LIFE bonus. This will prevent mimics from total uber HP,
	   and giving them an excellent chance to compensate a form that
	   provides bad HP. - C. Blue */
	mhp_playerform = mhp;


	if (p_ptr->body_monster) {
		long rhp = ((long)(r_info[p_ptr->body_monster].hdice)) * ((long)(r_info[p_ptr->body_monster].hside));

		/* assume human mimic HP for calculation; afterwards, add up our 'extra' hit points if we're a stronger race */
		/* additionally, scale form HP better with very high character levels */
		/* Reduce the effect of racial hit dice when in monster form? [3..6]
		   3 = no reduction, 6 = high reduction */
		#define FORM_REDUCES_RACE_DICE_INFLUENCE 6
		long raceHPbonus;
#ifndef MIMICRY_BOOST_WEAK_FORM
		long levD, hpD;
#endif

#if defined(ENABLE_OHERETICISM) && defined(ENABLE_HELLKNIGHT)
		/* Blood Sacrifice has special traits: Always useful form */
		if (p_ptr->body_monster == RI_BLOODTHIRSTER && (p_ptr->pclass == CLASS_HELLKNIGHT
 #ifdef CLASS_CPRIEST
		    || p_ptr->pclass == CLASS_CPRIEST
 #endif
		    )) rlev = 80;
#endif

		mHPLim = (50000 / ((50000 / rhp) + 20));

#if 0 /* done below */
		/* add flat bonus to maximum HP limit for char levels > 50, if form is powerful, to keep it useful */
		mHPLim += p_ptr->lev > 50 ? (((p_ptr->lev - 50) * (rlev + 30)) / 100) * 20 : 0;
#endif

		raceHPbonus = mhp - ((mhp * 16) / p_ptr->hitdie); /* 10 human + 6 mimic */
		mhp -= (raceHPbonus * 3) / FORM_REDUCES_RACE_DICE_INFLUENCE;
#if 0 /* nonsense^^ */
		/* compensate overall HP gain for values > 3 */
		mhp -= ((raceHPbonus - (raceHPbonus * 3) / FORM_REDUCES_RACE_DICE_INFLUENCE) * 5) / 3;
#endif
		if (mHPLim < mhp) {
#ifdef MIMICRY_BOOST_WEAK_FORM
			mHPLim = mhp;
#else
			levD = p_ptr->lev - rlev;
			if (levD < 0) levD = 0;
			if (levD > 20) levD = 20;
			hpD = mhp - mHPLim;
		        mHPLim = mhp - (hpD * levD) / 20; /* When your form is 20 or more levels below your charlevel,
							   you receive the full HP difference in the formula below. */
#endif
		}

		finalHP = (mHPLim < mhp) ? (((mhp * 4) + (mHPLim * 1)) / 5) : (((mHPLim * 2) + (mhp * 3)) / 5);
		finalHP += (raceHPbonus * 3) / FORM_REDUCES_RACE_DICE_INFLUENCE;

		/* done */
		mhp = finalHP;
	}

	/* Always have at least one hitpoint per level */
	if (mhp < p_ptr->lev + 1) mhp = p_ptr->lev + 1;

	/* for C_BLUE_AI */
	if (!p_ptr->body_monster) p_ptr->form_hp_ratio = 100;
	else p_ptr->form_hp_ratio = (mhp * 100) / mhp_playerform;

	/* calculate +LIFE bonus */
	to_life = p_ptr->to_l;

#ifdef ENABLE_MAIA
	/* Bonus from RACE_MAIA */
	if (p_ptr->divine_hp > 0) {
		/* count as if from item? */
		to_life += p_ptr->divine_hp_mod;
	}
#endif

	/* cap it at +30% HP and at -100% HP */
	if (to_life > 3 && !is_admin(p_ptr)) to_life = 3;
	if (to_life < -9) to_life = -9;

	/* new hack (see below): if life bonus is negative, dont apply player form hp */
	if (to_life > 0) {
		/* Reduce use of +LIFE items for mimics while in monster-form */
		if (mhp > mhp_playerform) {
			if (to_life > 0)
				mhp += (mhp_playerform * to_life * mhp_playerform) / (10 * mhp);
			else
				mhp += (mhp_playerform * to_life) / 10;
		} else {
			mhp += (mhp * to_life) / 10;
		}
	}

#if 1
	if (p_ptr->body_monster) {
		/* add flat bonus to maximum HP limit for char levels > 50, if form is powerful, to keep it useful */
		mhp += (p_ptr->lev > 50) ?
		    (((p_ptr->lev - 50) * ((rlev > 80 ? 80 :
 #if 0 /* for 15..17/32 hp birth calc */
		    rlev) + 30)) / 100) * 5 : 0;
 #else /* for 31..33/64 hp birth calc */
		    rlev) + 30)) / 25) : 0;
 #endif 
	}
#endif

	/* new hack (see above): player form hp doesn't matter if life bonus is negative */
	if (to_life < 0) mhp += (mhp * to_life) / 10;
	/* some places divide by mhp, so hack it for now */
	if (mhp == 0) mhp = 1;

	/* Factor in the hero / superhero settings.
	   Specialty: It's applied AFTER mimic form HP influence. */
	if (p_ptr->hero) mhp += 10;
	if (p_ptr->shero) mhp += 20;

#if 0 /* p_ptr->to_hp is unused atm! */
	/* Fixed Hit Point Bonus */
	if (!is_admin(p_ptr) && p_ptr->to_hp > 200) p_ptr->to_hp = 200;

	if (mhp > mhp_playerform) {
		/* Reduce the use for mimics (while in monster-form) */
		if (p_ptr->to_hp > 0)
			mhp += (mhp_playerform * p_ptr->to_hp) / mhp;
		else
			mhp += p_ptr->to_hp;
	} else {
		mhp += p_ptr->to_hp;
	}
#endif

	/* Meditation increase mana at the cost of hp */
	if (p_ptr->tim_meditation) mhp = mhp * 3 / 5;

	/* New maximum hitpoints */
	if (mhp != p_ptr->mhp) {
		s32b value;

		/* change current hit points proportionately to change of mhp */
		/* divide first to avoid overflow, little loss of accuracy */
		value = (((long)p_ptr->chp << 16) + p_ptr->chp_frac) / p_ptr->mhp;
		value = value * mhp;
		p_ptr->chp = (value >> 16);
		p_ptr->chp_frac = (value & 0xFFFF);

		/* Save the new max-hitpoints */
		p_ptr->mhp = mhp;
		
		/* Little fix (chp = mhp+1 sometimes) */
		if (p_ptr->chp > p_ptr->mhp) p_ptr->chp = p_ptr->mhp;

		/* Display hitpoints (later) */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);
	}
}



/*
 * Extract and set the current "lite radius"
 */
/*
 * XXX currently, this function does almost nothing; if lite radius
 * should be changed, call calc_boni too.
 */
static void calc_torch(int Ind) {
	player_type *p_ptr = Players[Ind];
	
#if 0
	object_type *o_ptr = &p_ptr->inventory[INVEN_LITE];

	/* Base light radius */
	p_ptr->cur_lite = p_ptr->lite;

	/* Examine actual lites */
	if (o_ptr->tval == TV_LITE) {
		/* Torches (with fuel) provide some lite */
		if ((o_ptr->sval == SV_LITE_TORCH) && (o_ptr->pval > 0))
			p_ptr->cur_lite += 1;

		/* Lanterns (with fuel) provide more lite */
		if ((o_ptr->sval == SV_LITE_LANTERN) && (o_ptr->pval > 0))
			p_ptr->cur_lite += 2;

		/* Dwarven lanterns provide permanent radius 2 lite */
		if (o_ptr->sval == SV_LITE_DWARVEN)
			p_ptr->cur_lite += 2;

		/* Feanorian lanterns provide permanent, bright, lite */
		if (o_ptr->sval == SV_LITE_FEANOR)
			p_ptr->cur_lite += 3;

		/* Artifact Lites provide permanent, bright, lite */
		if (artifact_p(o_ptr)) p_ptr->cur_lite += 3;
	}
#endif	// 0

	/* Reduce lite when running if requested */
	if (p_ptr->running && p_ptr->view_reduce_lite) {
		/* Reduce the lite radius if needed */
		if (p_ptr->cur_lite > 1) p_ptr->cur_lite = 1;
		if (p_ptr->cur_vlite > 1) p_ptr->cur_vlite = 1;
	}

	/* Notice changes in the "lite radius" */
	if ((p_ptr->old_lite != p_ptr->cur_lite) || (p_ptr->old_vlite != p_ptr->cur_vlite)) {
		/* Update the lite */
		p_ptr->update |= (PU_LITE);

		/* Update the monsters */
		p_ptr->update |= (PU_MONSTERS);

		/* Remember the old lite */
		p_ptr->old_lite = p_ptr->cur_lite;
		p_ptr->old_vlite = p_ptr->cur_vlite;
	}
}



/*
 * Computes current weight limit.
 */
static int weight_limit(int Ind) /* max. 3000 atm */ {
	player_type *p_ptr = Players[Ind];
	int i;

	/* Weight limit based only on strength */
	i = adj_str_wgt[p_ptr->stat_ind[A_STR]] * 100;

	/* Return the result */
	return (i);
}


/* Should be called by every calc_bonus call */
static void calc_body_bonus(int Ind, boni_col * csheet_boni) {
	player_type *p_ptr = Players[Ind];
	dun_level *l_ptr = getfloor(&p_ptr->wpos);
	cave_type **zcave;

	int d, immunities = 0, immunity[7], immrand, spellbonus = 0;
	int i, j;
	monster_race *r_ptr = &r_info[p_ptr->body_monster];
	char mname[MNAME_LEN];

	if (!(zcave = getcave(&p_ptr->wpos))) return;

	/* If in the player body nothing have to be done */
	if (!p_ptr->body_monster) return;

	immunity[1] = 0; immunity[2] = 0; immunity[3] = 0;
	immunity[4] = 0; immunity[5] = 0; immunity[6] = 0;
	immrand = 0;

	/* Prepare lower-case'd name for worry-free testing */
	strcpy(mname, r_name + r_ptr->name);
	mname[0] = tolower(mname[0]);

	/* keep random monster-body abilities static over quit/rejoin */
	Rand_value = p_ptr->mimic_seed;
	Rand_quick = TRUE;

	if (!r_ptr->body_parts[BODY_WEAPON]) p_ptr->num_blow = 1;

	d = 0;
	for (i = 0; i < 4; i++) {
		j = (r_ptr->blow[i].d_dice * r_ptr->blow[i].d_side);

		switch (r_ptr->blow[i].effect) {
		case RBE_EXP_10:
		case RBE_EXP_20:
		case RBE_EXP_40:
		case RBE_EXP_80:
			p_ptr->hold_life = TRUE;
			csheet_boni->cb[5] |= CB6_RLIFE;
			break;
		case RBE_SHATTER:
			p_ptr->stat_add[A_STR]++;
			csheet_boni->pstr++;
			break;
		case RBE_LOSE_STR:
			p_ptr->sustain_str = TRUE;
			csheet_boni->cb[11] |= CB12_RSSTR;
			break;
		case RBE_LOSE_INT:
			p_ptr->sustain_int = TRUE;
			csheet_boni->cb[11] |= CB12_RSINT;
			break;
		case RBE_LOSE_WIS:
			p_ptr->sustain_wis = TRUE;
			csheet_boni->cb[11] |= CB12_RSWIS;
			break;
		case RBE_LOSE_DEX:
			p_ptr->sustain_dex = TRUE;
			csheet_boni->cb[11] |= CB12_RSDEX;
			break;
		case RBE_LOSE_CON:
			p_ptr->sustain_con = TRUE;
			csheet_boni->cb[11] |= CB12_RSCON;
			break;
		case RBE_LOSE_CHR:
			p_ptr->sustain_chr = TRUE;
			csheet_boni->cb[11] |= CB12_RSCHR;
			break;
		case RBE_LOSE_ALL:
			p_ptr->sustain_str = TRUE;
			csheet_boni->cb[11] |= CB12_RSSTR;
			p_ptr->sustain_int = TRUE;
			csheet_boni->cb[11] |= CB12_RSINT;
			p_ptr->sustain_wis = TRUE;
			csheet_boni->cb[11] |= CB12_RSWIS;
			p_ptr->sustain_dex = TRUE;
			csheet_boni->cb[11] |= CB12_RSDEX;
			p_ptr->sustain_con = TRUE;
			csheet_boni->cb[11] |= CB12_RSCON;
			p_ptr->sustain_chr = TRUE;
			csheet_boni->cb[11] |= CB12_RSCHR;
			break;
		case RBE_UN_POWER:
			/* Specialty for (Lesser) Black Reaver form, buffing it for magic device usage:
			   Great casters who can also inflict power-drain are themselves resistant to it. */
			if (r_ptr->freq_innate >= 50) p_ptr->resist_discharge = TRUE; //unofficial resistance =p
			break;
		}

		d += (j * 2);
	}
	d /= 4;

	/* Apply STR bonus/malus, derived from form damage and race */
	/* <damage> */
	if (!d) d = 1;
	/* 0..147 (greater titan) -> 0..5 -> -1..+4 */
	i = (((15000 / ((15000 / d) + 50)) / 29) - 1);
	/* <corpse weight> : Non-light creatures won't get a STR malus,
	   even if they don't deal much damage. */
	if (r_ptr->weight >= 1500 && i < 0) i = 0;
	if (r_ptr->weight >= 4500 && i < 1) i++;
	if (r_ptr->weight >= 20000 && i < 2) i++;
	if (r_ptr->weight >= 100000 && i < 3) i++;
	/* <race> */
	if (strstr(mname, "bear") && (r_ptr->flags3 & RF3_ANIMAL)) i++; /* Bears get +1 STR */
	if (r_ptr->flags3 & RF3_TROLL) i += 1;
	if (r_ptr->flags3 & RF3_GIANT) i += 1;
	if ((r_ptr->flags3 & RF3_DRAGON) && (strstr(mname, "mature ") ||
	    (r_ptr->d_char == 'D')))
		i += 1; /* only +1 since more bonus is coming from its damage already */
	p_ptr->stat_add[A_STR] += i;
	csheet_boni->pstr += i;



	/* Very specific group/single monster boni for animals and people:
	   Note: Only STR/DEX/CHR (and Stealth) are considered for stats,
	         WIS/INT are considered mind-intrinsic and only modified by spell-casting frequency,
	         CON is already implied by the form's hit points.
	         (The only other stats that could potentially be added to be modified here are probably Saving Throw and Perception.) */

	/* Cats, rogues, martial artists (mystics/ninjas) and some warriors are very agile and most of them are stealthy */
	if (r_ptr->d_char == 'f') { /* Cats! */
		p_ptr->stat_add[A_DEX] += 2; csheet_boni->pdex += 2;
		/* don't stack with low weight stealth bonus too much (see further below) */
		if (r_ptr->weight <= 500) { p_ptr->skill_stl++; csheet_boni->slth++; }
		else { p_ptr->skill_stl = p_ptr->skill_stl + 2; csheet_boni->slth += 2; }
	}
	/* Rogues and master rogues */
	if (r_ptr->d_char == 'p' && r_ptr->d_attr == TERM_BLUE) {
		if (r_ptr->level >= 23) {
			p_ptr->stat_add[A_DEX] += 2; csheet_boni->pdex += 2;
			p_ptr->skill_stl = p_ptr->skill_stl + 2; csheet_boni->slth += 2;
		} else {
			p_ptr->stat_add[A_DEX]++; csheet_boni->pdex++;
			p_ptr->skill_stl++; csheet_boni->slth++;
		}
	}
	/* Warriors */
	if (r_ptr->d_char == 'p' && r_ptr->d_attr == TERM_UMBER && r_ptr->level >= 20) { p_ptr->stat_add[A_STR]++; csheet_boni->pstr++; } /* Skilled warriors */
	if (p_ptr->body_monster == 239) { p_ptr->stat_add[A_STR]++; csheet_boni->pstr++; } /* Berserker */
	if (p_ptr->body_monster == 1058 || p_ptr->body_monster == 1059) { p_ptr->stat_add[A_DEX]++; csheet_boni->pdex++; } /* (Grand) Swordsmaster */
	if (p_ptr->body_monster == 532) { /* Dagashi */
		p_ptr->stat_add[A_DEX]++; csheet_boni->pdex++;
		p_ptr->skill_stl++; csheet_boni->slth++;
	}
	if (p_ptr->body_monster == 485 || p_ptr->body_monster == 564) {/* Ninja, Nightblade */
		p_ptr->stat_add[A_DEX] += 2; csheet_boni->pdex += 2;
		p_ptr->skill_stl = p_ptr->skill_stl + 3; csheet_boni->slth += 3;
	}
	/* Mystics */
	if (r_ptr->d_char == 'p' && r_ptr->d_attr == TERM_ORANGE) {
		p_ptr->stat_add[A_DEX] += 2; csheet_boni->pdex += 2;
		p_ptr->skill_stl++; csheet_boni->slth++;
	}
	/* Monks */
	if (p_ptr->body_monster == 370) { p_ptr->stat_add[A_DEX]++; csheet_boni->pdex++; }/* Jade Monk */
	if (p_ptr->body_monster == 492) { p_ptr->stat_add[A_DEX]++; csheet_boni->pdex++; }/* Ivory Monk */



	if (r_ptr->speed < 110) {
		/* let slowdown not be that large that players will never try that form */
		p_ptr->pspeed = 110 - (((110 - r_ptr->speed) * 20) / 100);
	} else {
		/* let speed bonus not be that high that players won't try any slower form */
		//p_ptr->pspeed = (((r_ptr->speed - 110) * 30) / 100) + 110;//was 50%, 30% for RPG_SERVER originally
		//Pfft, include the -speed into the calculation, too. Seems lame how -speed is counted for 100% but not + bonus.
		//But really, if physical-race-intrinsic bonuses/maluses are counted in mimicry, then dwarves 
		//should be able to keep their climbing ability past 30 when mimicked, TLs could fly, etc etc =/
		p_ptr->pspeed = (((r_ptr->speed - 110 - (p_ptr->prace == RACE_ENT ? 2 : 0) ) * 30) / 100) + 110;//was 50%, 30% for RPG_SERVER originally
	}
	csheet_boni->spd = p_ptr->pspeed - 110;

#if 0 /* Should forms affect your searching/perception skills? Probably not. */
 #if 0
	/* Base skill -- searching ability */
	p_ptr->skill_srh /= 2;
	p_ptr->skill_srh += r_ptr->aaf / 10;

	/* Base skill -- searching frequency */
	p_ptr->skill_fos /= 2;
	p_ptr->skill_fos += r_ptr->aaf / 10;
 #else
	/* Base skill -- searching ability */
	p_ptr->skill_srh += r_ptr->aaf / 20 - 5;

	/* Base skill -- searching frequency */
	p_ptr->skill_fos += r_ptr->aaf / 20 - 5;
 #endif
#endif

	/* Stealth ability is influenced by weight (-> size) of the monster */
	if (r_ptr->weight <= 500) { p_ptr->skill_stl += 2; csheet_boni->slth += 2; }
	else if (r_ptr->weight <= 500) { p_ptr->skill_stl += 2; csheet_boni->slth += 2; }
	else if (r_ptr->weight <= 1000) { p_ptr->skill_stl += 1; csheet_boni->slth += 1; }
	else if (r_ptr->weight <= 1500) { p_ptr->skill_stl += 0; csheet_boni->slth += 0; }
	else if (r_ptr->weight <= 4500) { p_ptr->skill_stl -= 1; csheet_boni->slth -= 1; }
	else if (r_ptr->weight <= 20000) { p_ptr->skill_stl -= 2; csheet_boni->slth -= 2; }
	else if (r_ptr->weight <= 100000) { p_ptr->skill_stl -= 3; csheet_boni->slth -= 3; }
	else { p_ptr->skill_stl -= 4; csheet_boni->slth -= 4; }

	/* Extra fire if good archer */
	/*  1_IN_1  -none-
	    1_IN_2  Grandmaster Thief
	    1_IN_3  Halfling Slinger, Ranger Chieftain
	    1_IN_4  Ranger
	*/
	if ((r_ptr->flags4 & (RF4_ARROW_1 | RF4_ARROW_2 | RF4_ARROW_3)) &&	/* 1=BOW,2=XBOW,3=SLING; 4=generic missile */
	    (p_ptr->inventory[INVEN_BOW].k_idx) && (p_ptr->inventory[INVEN_BOW].tval == TV_BOW))
	{
#if 0
 #if 0 /* normal way to handle xbow <-> sling/bow shot frequency */
		if ((p_ptr->inventory[INVEN_BOW].sval == SV_SLING) ||
		    (p_ptr->inventory[INVEN_BOW].sval == SV_SHORT_BOW) ||
		    (p_ptr->inventory[INVEN_BOW].sval == SV_LONG_BOW)) p_ptr->num_fire++;
		if (r_ptr->freq_innate > 30) p_ptr->num_fire++; /* this time for crossbows too */
 #else /* give xbow an advantage */
		if (r_ptr->freq_innate > 30)
		if ((p_ptr->inventory[INVEN_BOW].sval == SV_SLING) ||
		    (p_ptr->inventory[INVEN_BOW].sval == SV_SHORT_BOW) ||
		    (p_ptr->inventory[INVEN_BOW].sval == SV_LONG_BOW)) p_ptr->num_fire++;
		p_ptr->num_fire++; /* this time for crossbows too */
 #endif
#else /* xbows only get +1ES from fast shooters, not already from slower ARROW_ shooters */
		if ((p_ptr->inventory[INVEN_BOW].sval == SV_SLING) ||
		    (p_ptr->inventory[INVEN_BOW].sval == SV_SHORT_BOW) ||
		    (p_ptr->inventory[INVEN_BOW].sval == SV_LONG_BOW)) { p_ptr->num_fire++; csheet_boni->shot++; }
		if (r_ptr->freq_innate > 30)
			{ p_ptr->num_fire++; csheet_boni->shot++; } /* this time for crossbows too */
#endif
	}

	/* Extra casting if good spellcaster */
	if ((r_ptr->flags4 & RF4_SPELLCASTER_MASK) ||
	    (r_ptr->flags5 & RF5_SPELLCASTER_MASK) ||
	    (r_ptr->flags6 & RF6_SPELLCASTER_MASK) ||
	    (r_ptr->flags0 & RF0_SPELLCASTER_MASK)) {
		if (r_ptr->freq_innate > 30) {
			p_ptr->num_spell++;	// 1_IN_3
			spellbonus += 1;
			p_ptr->to_m += 20; csheet_boni->mxmp += 2;
		}
		if (r_ptr->freq_innate >= 50) {
			p_ptr->num_spell++;	// 1_IN_2
			spellbonus += 2;
			p_ptr->to_m += 15; csheet_boni->mxmp += 1;
		}
		if (r_ptr->freq_innate == 100) { /* well, drujs and quylthulgs >_> */
			p_ptr->num_spell++;	// 1_IN_1
			spellbonus += 1;
			p_ptr->to_m += 15; csheet_boni->mxmp += 2;
		}
		/* holy/druidic forms get some WIS bonus too, others get INT only */
		if (r_ptr->d_char == 'h' || r_ptr->d_char == 'p') {
			switch (r_ptr->d_attr) {
			case TERM_GREEN:
			case TERM_L_GREEN:
			case TERM_WHITE:
				i = spellbonus / 2;
				j = spellbonus - i;
				p_ptr->stat_add[A_WIS] += i; csheet_boni->pwis += i;
				p_ptr->stat_add[A_INT] += j; csheet_boni->pint += j;
				break;
			default:
				p_ptr->stat_add[A_INT] += spellbonus; csheet_boni->pint += spellbonus;
			}
		} else p_ptr->stat_add[A_INT] += spellbonus; csheet_boni->pint += spellbonus;
	}


	/* Racial boni depending on the form's race */
	switch (p_ptr->body_monster) {
		/* Bats get feather falling */
		case 37:	case 114:	case 187:	case 235:	case 351:
		case 377:	case RI_VAMPIRE_BAT:	case 406:	case 484:	case 968:
			p_ptr->feather_fall = TRUE;
			csheet_boni->cb[5] |= CB6_RFFAL;
			/* Vampire bats are vampiric */
			if (p_ptr->body_monster == RI_VAMPIRE_BAT) { p_ptr->vampiric_melee = 100; csheet_boni->cb[6] |= CB7_RVAMP; }
#if 0 /* only real/chauvesouris ones for now, or spider/crow/wild cat forms would be obsolete! */
			/* Fruit bats get some life leech */
			if (p_ptr->body_monster == 37 && p_ptr->vampiric_melee < 50) { p_ptr->vampiric_melee = 50; csheet_boni->cb[6] |= CB7_RVAMP; }
#endif
			break;

		case RI_VAMPIRIC_MIST: /* Vampiric mist is vampiric */
			p_ptr->vampiric_melee = 100; csheet_boni->cb[6] |= CB7_RVAMP;

			/* as all mists/fumes: FF */
			p_ptr->feather_fall = TRUE; csheet_boni->cb[5] |= CB6_RFFAL;
			p_ptr->pass_trees = TRUE; csheet_boni->cb[12] |= CB13_XTREE;
			//note: we use ff+pt combo instead of lev, so they can't easily pass water!

			/* only for true vampires: obtain special form boni: */
			if (p_ptr->prace == RACE_VAMPIRE) {
				/* max stealth */
				p_ptr->skill_stl = p_ptr->skill_stl + 25;
				csheet_boni->slth += 25;
				/* acquire shivering aura, NOT cold aura, for now -- done via hack in melee2.c */
				//p_ptr->sh_cold = p_ptr->sh_cold_fix = TRUE; csheet_boni->cb[10] |= CB11_ACOLD;
			}
			break;
		case 927: /* Vampiric ixitxachitl is vampiric */
			if (p_ptr->vampiric_melee < 50) p_ptr->vampiric_melee = 50;
			csheet_boni->cb[6] |= CB7_RVAMP;
			break;

		/* Elves get resist_lite, Dark-Elves get resist_dark */
		case 122:	case 400:	case 178:	case 182:	case 226:
		case 234:	case 348:	case 375:	case 564:	case 657:
			p_ptr->resist_dark = TRUE; csheet_boni->cb[2] |= CB3_RDARK;
			break;
		case 864:
			p_ptr->resist_lite = TRUE; csheet_boni->cb[2] |= CB3_RLITE;
			break;

		/* Hobbits/Halflings get the no-shoes-bonus */
		case 74:	case 539:
			if (!p_ptr->inventory[INVEN_FEET].k_idx)
				{ p_ptr->stat_add[A_DEX] += 2; csheet_boni->pdex += 2; }
			break;

		/* Gnomes get free_act */
		case 258:	case 281:
			p_ptr->free_act = TRUE; csheet_boni->cb[4] |= CB5_RPARA;
			break;

		/* Dwarves get res_blind & climbing ability */
		case 111:	case 865:
			p_ptr->resist_blind = TRUE; csheet_boni->cb[1] |= CB2_RBLND;
			if (p_ptr->lev >= 30) { p_ptr->climb = TRUE; csheet_boni->cb[5] |= CB6_RCLMB; }
			break;

		/* High-elves resist_lite & see_inv */
		case 945:
			p_ptr->resist_lite = TRUE; csheet_boni->cb[2] |= CB3_RLITE;
			p_ptr->see_inv = TRUE; csheet_boni->cb[4] |= CB5_RSINV;
			break;

		/* Yeeks get feather_fall */
		case 52:	case 141:	case 179:	case 224:
			p_ptr->feather_fall = TRUE; csheet_boni->cb[5] |= CB6_RFFAL;
			break;

		/* Ents */
		case 708:
			p_ptr->slow_digest = TRUE; csheet_boni->cb[6] |= CB7_RFOOD;
			//if (p_ptr->prace != RACE_ENT)
			p_ptr->pspeed -= 2;
			p_ptr->suscep_fire = TRUE; csheet_boni->cb[0] |= CB1_SFIRE;
			p_ptr->resist_water = TRUE; csheet_boni->cb[3] |= CB4_RWATR;
/* not form-dependant:	if (p_ptr->lev >= 4) p_ptr->see_inv = TRUE; */
			p_ptr->can_swim = TRUE; csheet_boni->cb[12] |= CB13_XSWIM; /* wood? */
			p_ptr->pass_trees = TRUE; csheet_boni->cb[12] |= CB13_XTREE;
			break;

		/* Ghosts get additional boni - undead see below */
		case 65:	case 100:	case 133:	case 152:	case 231:
		case 385:	case 394:	case 477:	case 507:	case 508:
		case 533:	case 534:	case 553:	case 630:	case 665:
		case 667:	case 690:	case 774:	case 895:
		case 929:	case 930:	case 931:	case 932:	case 933:
		case 967:	case 973:	case 974:
			/* I'd prefer ghosts having a radius of awareness, like a 'pseudo-light source',
			since atm ghosts are completely blind in the dark :( -C. Blue */
			p_ptr->see_inv = TRUE; csheet_boni->cb[4] |= CB5_RSINV;
			//p_ptr->invis += 5; */ /* No. */
			break;

		/* Vampires have VAMPIRIC attacks */
		case 432:	case 520:	case 521:	case 623:	case 989:
			if (p_ptr->vampiric_melee < 50) { p_ptr->vampiric_melee = 50; csheet_boni->cb[6] |= CB7_RVAMP; }
			p_ptr->suscep_lite = TRUE; csheet_boni->cb[2] |= CB3_SLITE;
			break;

		/* Angels resist light, blindness and poison (usually immunity) */
		case 417:	case 456:	case 511:	case 605:
		case 661:	case 1071:	case 1072:	case 1073:
			p_ptr->see_inv = TRUE; csheet_boni->cb[4] |= CB5_RSINV;
		/* Fallen Angel */
		case 652:
			p_ptr->resist_blind = TRUE; csheet_boni->cb[1] |= CB2_RBLND;
			break;
		/* Mists/fumes have feather falling */
		//(note: mist giant and weird fume already have CAN_FLY even, covering FF)
		/* Dark mist */
		case 1064:
			p_ptr->feather_fall = TRUE; csheet_boni->cb[5] |= CB6_RFFAL;
			p_ptr->pass_trees = TRUE; csheet_boni->cb[12] |= CB13_XTREE;
			//note: we use ff+pt combo instead of lev, so they can't easily pass water!
			break;
	}

	/* If monster has a lite source, but doesn't prove a torso (needed
	   to use a lite source, yellow light) or can breathe light (light
	   hound), add to the player's light radius! */
	if ((r_ptr->flags9 & RF9_HAS_LITE) &&
	     ((!r_ptr->body_parts[BODY_TORSO]) || (r_ptr->flags4 & RF4_BR_LITE)))
		{ p_ptr->cur_lite += 1; csheet_boni->lite += 1; }

	/* Forms that occur in the woods are able to pass them, so are animals */	
	if ((r_ptr->flags8 & RF8_WILD_WOOD) || (r_ptr->flags3 & RF3_ANIMAL))
		{ p_ptr->pass_trees = TRUE; csheet_boni->cb[12] |= CB13_XTREE; }

	/* Forms that occur in the mountains are able to pass them */
	if ((r_ptr->flags8 & (RF8_WILD_MOUNTAIN | RF8_WILD_VOLCANO)) ||
	    (r_ptr->flags0 & RF0_CAN_CLIMB))
		{ p_ptr->climb = TRUE; csheet_boni->cb[5] |= CB6_RCLMB; }
	/* Spiders can always climb */
	if (r_ptr->flags7 & RF7_SPIDER) { p_ptr->climb = TRUE; csheet_boni->cb[5] |= CB6_RCLMB; }

	/* Orcs get resist_dark */
	if (r_ptr->flags3 & RF3_ORC) { p_ptr->resist_dark = TRUE; csheet_boni->cb[2] |= CB3_RDARK; }

	/* Trolls/Giants get sustain_str */
	if ((r_ptr->flags3 & RF3_TROLL) || (r_ptr->flags3 & RF3_GIANT)) { p_ptr->sustain_str = TRUE; csheet_boni->cb[11] |= CB12_RSSTR; }

	/* Draconian get feather_fall, ESP_DRAGON */
	if (r_ptr->flags3 & RF3_DRAGONRIDER) {
		p_ptr->feather_fall = TRUE; csheet_boni->cb[5] |= CB6_RFFAL;
		if (p_ptr->lev >= 5) { p_ptr->telepathy |= ESP_DRAGON; csheet_boni->cb[8] |= CB9_EDRGN; }
		if (p_ptr->lev >= 30) { p_ptr->levitate = TRUE; csheet_boni->cb[5] |= CB6_RLVTN; }
	}

	/* Undead get lots of stuff similar to player ghosts */
	if (r_ptr->flags3 & RF3_UNDEAD) {
		/* p_ptr->see_inv = TRUE;
		p_ptr->resist_neth = TRUE;
		p_ptr->hold_life = TRUE;
		p_ptr->free_act = TRUE;
		p_ptr->see_infra += 3;
		p_ptr->resist_fear = TRUE;*/
		/*p_ptr->resist_conf = TRUE;*/
		/* p_ptr->resist_pois = TRUE; */ /* instead of immune */
		/* p_ptr->resist_cold = TRUE; */

		p_ptr->resist_pois = TRUE; csheet_boni->cb[1] |= CB2_RPOIS;
		p_ptr->resist_dark = TRUE; csheet_boni->cb[2] |= CB3_RDARK;
		p_ptr->resist_blind = TRUE; csheet_boni->cb[1] |= CB2_RBLND;
		p_ptr->no_cut = TRUE; csheet_boni->cb[12] |= CB13_XNCUT;
		if (!p_ptr->reduce_insanity) { p_ptr->reduce_insanity = 1; csheet_boni->cb[3] |= CB4_RMIND; }
		p_ptr->see_infra += 1; csheet_boni->infr += 1;

		if (strchr("GWLV", r_ptr->d_char)) {
			p_ptr->see_infra += 4; csheet_boni->infr += 4;
		}
	}

	/* Non-living got a nice ability set too ;) */
	if (r_ptr->flags3 & RF3_NONLIVING) {
		p_ptr->resist_pois = TRUE; csheet_boni->cb[1] |= CB2_RPOIS;
		p_ptr->resist_fear = TRUE; csheet_boni->cb[4] |= CB5_RFEAR;
		p_ptr->reduce_insanity = 2; csheet_boni->cb[4] |= CB5_XMIND;
	}

	/* Greater demons resist poison (monsters are immune) */
	if ((r_ptr->d_char == 'U') && (r_ptr->flags3 & RF3_DEMON)) { p_ptr->resist_pois = TRUE; csheet_boni->cb[1] |= CB2_RPOIS; }

	/* Affect charisma by appearance */
	d = 0;
	if (r_ptr->flags3 & RF3_DRAGONRIDER) d = 1;
	else if (r_ptr->flags3 & RF3_DRAGON) d = 0;
	if (r_ptr->flags3 & RF3_ANIMAL) {
		if (r_ptr->weight <= 450 && strchr("bfqBCR", r_ptr->d_char)) d = 1; /* yes, I included NEWTS */
		else d = 0;
	}
	if (r_ptr->flags7 & RF7_SPIDER) d = -1;

	if (r_ptr->flags3 & RF3_ORC) d = -1;

	if (r_ptr->flags3 & RF3_NONLIVING) d = -1;
	if (r_ptr->flags3 & RF3_EVIL) d = -1;

	if (r_ptr->flags3 & RF3_TROLL) d = -2;
	if (r_ptr->flags3 & RF3_GIANT) d = -2;

	if (r_ptr->flags3 & RF3_UNDEAD) d = -3;
	if (r_ptr->flags3 & RF3_DEMON) d = -3;

	if (r_ptr->flags3 & RF3_GOOD) d += 2;

	p_ptr->stat_add[A_CHR] += d; csheet_boni->pchr += d;


	//if (r_ptr->flags1 & RF1_NEVER_MOVE) p_ptr->immovable = TRUE;
	if (r_ptr->flags2 & RF2_STUPID) { p_ptr->stat_add[A_INT] -= 2; csheet_boni->pint -= 2; }
	if (r_ptr->flags2 & RF2_SMART) { p_ptr->stat_add[A_INT] += 2; csheet_boni->pint += 2; }
	if (r_ptr->flags2 & RF2_INVISIBLE) {
		d = ((r_ptr->level > 100 ? 50 : r_ptr->level / 2) + (p_ptr->lev > 50 ? 50 : p_ptr->lev)) / 2;
		p_ptr->tim_invis_power = d * 4 / 5; csheet_boni->cb[4] |= CB5_RINVS;
	}
	if (r_ptr->flags2 & RF2_REGENERATE) { p_ptr->regenerate = TRUE; csheet_boni->cb[5] |= CB6_RRGHP; }
	/* Immaterial forms (WRAITH / PASS_WALL) drain the mimic's HP! */
	if (r_ptr->flags2 & RF2_PASS_WALL) {
		if (!(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) &&
		    !(p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC))) {
			//BAD!(recursion)	set_tim_wraith(Ind, 10000);
			if (!p_ptr->tim_wraith) p_ptr->redraw |= PR_BPR_WRAITH;
			p_ptr->tim_wraith = 10000; csheet_boni->cb[5] |= CB6_RWRTH;
		}
		p_ptr->drain_life++; csheet_boni->cb[5] |= CB6_SRGHP;
	}
	if (r_ptr->flags2 & RF2_KILL_WALL) { p_ptr->auto_tunnel = TRUE; csheet_boni->cb[12] |= CB13_XWALL; }
	if (r_ptr->flags2 & RF2_AURA_FIRE) { p_ptr->sh_fire = p_ptr->sh_fire_fix = TRUE; csheet_boni->cb[10] |= CB11_AFIRE; }
	if (r_ptr->flags2 & RF2_AURA_ELEC) { p_ptr->sh_elec = p_ptr->sh_elec_fix = TRUE; csheet_boni->cb[10] |= CB11_AELEC; }
	if (r_ptr->flags3 & RF3_AURA_COLD) { p_ptr->sh_cold = p_ptr->sh_cold_fix = TRUE; csheet_boni->cb[10] |= CB11_ACOLD; }

	if ((r_ptr->flags5 & RF5_MIND_BLAST) && !p_ptr->reduce_insanity) { p_ptr->reduce_insanity = 1; csheet_boni->cb[3] |= CB4_RMIND; }
	if (r_ptr->flags5 & RF5_BRAIN_SMASH) { p_ptr->reduce_insanity = 2; csheet_boni->cb[4] |= CB5_XMIND; }

	if (r_ptr->flags3 & RF3_SUSCEP_FIRE) { p_ptr->suscep_fire = TRUE; csheet_boni->cb[0] |= CB1_SFIRE; }
	if (r_ptr->flags3 & RF3_SUSCEP_COLD) { p_ptr->suscep_cold = TRUE; csheet_boni->cb[0] |= CB1_SCOLD; }
/* Imho, there should be only suspec fire and cold since these two are opposites.
Something which is fire-related will usually be suspectible to cold and vice versa.
Exceptions are rare, like Ent, who as a being of wood is suspectible to fire. (C. Blue) */
	if (r_ptr->flags9 & RF9_SUSCEP_ELEC) { p_ptr->suscep_elec = TRUE; csheet_boni->cb[0] |= CB1_SELEC; }
	if (r_ptr->flags9 & RF9_SUSCEP_ACID) { p_ptr->suscep_acid = TRUE; csheet_boni->cb[1] |= CB2_SACID; }
	if (r_ptr->flags9 & RF9_SUSCEP_POIS) { p_ptr->suscep_pois = TRUE; csheet_boni->cb[1] |= CB2_SPOIS; }

	if (r_ptr->flags9 & RF9_RES_ACID) { p_ptr->resist_acid = TRUE; csheet_boni->cb[1] |= CB2_RACID; }
	if (r_ptr->flags9 & RF9_RES_ELEC) { p_ptr->resist_elec = TRUE; csheet_boni->cb[0] |= CB1_RELEC; }
	if (r_ptr->flags9 & RF9_RES_FIRE) { p_ptr->resist_fire = TRUE; csheet_boni->cb[0] |= CB1_RFIRE; }
	if (r_ptr->flags9 & RF9_RES_COLD) { p_ptr->resist_cold = TRUE; csheet_boni->cb[0] |= CB1_RCOLD; }
	if (r_ptr->flags9 & RF9_RES_POIS) { p_ptr->resist_pois = TRUE; csheet_boni->cb[1] |= CB2_RPOIS; }

	if (r_ptr->flags3 & RF3_HURT_LITE) { p_ptr->suscep_lite = TRUE; csheet_boni->cb[2] |= CB3_SLITE; }
#if 0 /* for now let's say EVIL is a state of mind, so the mimic isn't necessarily evil */
	if (r_ptr->flags3 & RF3_EVIL) p_ptr->suscep_good = TRUE;
#else /* the appearance is important for damage though - let's keep it restricted to demon/undead for now */
	if (r_ptr->flags3 & RF3_DEMON || r_ptr->flags3 & RF3_UNDEAD) p_ptr->suscep_good = TRUE;
#endif
	if (r_ptr->flags3 & RF3_UNDEAD) p_ptr->suscep_life = TRUE;
	if (r_ptr->flags3 & RF3_GOOD) p_ptr->suscep_evil = TRUE;

	/* Grant a mimic a maximum of 2 immunities for now. All further immunities
	are turned into resistances. Which ones is random. */
	if (r_ptr->flags3 & RF3_IM_ACID) {
		immunities += 1;
		immunity[immunities] = 1;
		p_ptr->resist_acid = TRUE; csheet_boni->cb[1] |= CB2_RACID;
	}
	if (r_ptr->flags3 & RF3_IM_ELEC) {
		immunities += 1;
		immunity[immunities] = 2;
		p_ptr->resist_elec = TRUE; csheet_boni->cb[0] |= CB1_RELEC;
	}
	if (r_ptr->flags3 & RF3_IM_FIRE) {
		immunities += 1;
		immunity[immunities] = 3;
		p_ptr->resist_fire = TRUE; csheet_boni->cb[0] |= CB1_RFIRE;
	}
	if (r_ptr->flags3 & RF3_IM_COLD) {
		immunities += 1;
		immunity[immunities] = 4;
		p_ptr->resist_cold = TRUE; csheet_boni->cb[0] |= CB1_RCOLD;
	}
	if (r_ptr->flags3 & RF3_IM_POIS) {
		immunities += 1;
		immunity[immunities] = 5;
		p_ptr->resist_pois = TRUE; csheet_boni->cb[1] |= CB2_RPOIS;
	}
	if (r_ptr->flags9 & RF9_IM_WATER) {
		immunities += 1;
		immunity[immunities] = 6;
		p_ptr->resist_water = TRUE; csheet_boni->cb[3] |= CB4_RWATR;
	}
	/* (RF4_BR_PLAS implies IM_FIRE, done in init1.c) */

	/* gain not more than 1 immunities at the same time from a form */
	if (immunities == 1) {
		if (r_ptr->flags3 & RF3_IM_ACID) { p_ptr->immune_acid = TRUE; csheet_boni->cb[1] |= CB2_IACID; }
		if (r_ptr->flags3 & RF3_IM_ELEC) { p_ptr->immune_elec = TRUE; csheet_boni->cb[1] |= CB2_IELEC; }
		if (r_ptr->flags3 & RF3_IM_FIRE) { p_ptr->immune_fire = TRUE; csheet_boni->cb[0] |= CB1_IFIRE; }
		if (r_ptr->flags3 & RF3_IM_COLD) { p_ptr->immune_cold = TRUE; csheet_boni->cb[0] |= CB1_ICOLD; }
		if (r_ptr->flags3 & RF3_IM_POIS) { p_ptr->immune_poison = TRUE; csheet_boni->cb[1] |= CB2_IPOIS; }
		if (r_ptr->flags9 & RF9_IM_WATER) { p_ptr->immune_water = TRUE; csheet_boni->cb[3] |= CB4_IWATR; }
	} else if (immunities) {
		immrand = 1 + rand_int(immunities);

		switch (p_ptr->mimic_immunity) {
		case 1:
			if (r_ptr->flags3 & RF3_IM_ELEC) immunity[immrand] = 2;
			break;
		case 2:
			if (r_ptr->flags3 & RF3_IM_COLD) immunity[immrand] = 4;
			break;
		case 3:
			if (r_ptr->flags3 & RF3_IM_ACID) immunity[immrand] = 1;
			break;
		case 4:
			if (r_ptr->flags3 & RF3_IM_FIRE) immunity[immrand] = 3;
			break;
		case 5:
			if (r_ptr->flags3 & RF3_IM_POIS) immunity[immrand] = 5;
			break;
		case 6:
			if (r_ptr->flags9 & RF9_IM_WATER) immunity[immrand] = 6;
			break;
		}
//s_printf("MIMIC_IMMUNITY_CALC (%s): %s(%d) having %d sets imm[%d] to %d\n", showtime(), p_ptr->name, p_ptr->body_monster, p_ptr->mimic_immunity, immrand, immunity[immrand]);

		if (immunity[immrand] == 1) { p_ptr->immune_acid = TRUE; csheet_boni->cb[1] |= CB2_IACID; }
		if (immunity[immrand] == 2) { p_ptr->immune_elec = TRUE; csheet_boni->cb[1] |= CB2_IELEC; }
		if (immunity[immrand] == 3) { p_ptr->immune_fire = TRUE; csheet_boni->cb[0] |= CB1_IFIRE; }
		if (immunity[immrand] == 4) { p_ptr->immune_cold = TRUE; csheet_boni->cb[0] |= CB1_ICOLD; }
		if (immunity[immrand] == 5) { p_ptr->immune_poison = TRUE; csheet_boni->cb[1] |= CB2_IPOIS; }
		if (immunity[immrand] == 6) { p_ptr->immune_water = TRUE; csheet_boni->cb[3] |= CB4_IWATR; }
	}

#if defined(ENABLE_OHERETICISM) && defined(ENABLE_HELLKNIGHT)
	/* Hack: Blood Sacrifice form may give double-immunity!
	   Reason is that the player gains IM_FIRE at 50 intrinsically, so he'd like IM_POIS from the form, not a redundant IM_FIRE.
	   But the spell is already learnt at 45, when the player doesn't IM_FIRE yet. Now, it'd be weird if the form gave IM_POIS or even a choice instead,
	   especially since the player cannot use 'Select preferred immunity' function due to missing Mimicry skill.
	   So we'll just be generous at levels 45 to 49 and give IM_FIRE for free :-p as at 50 it'll not matter anymore. */
	if (p_ptr->body_monster == RI_BLOODTHIRSTER && (p_ptr->pclass == CLASS_HELLKNIGHT
 #ifdef CLASS_CPRIEST
	    || p_ptr->pclass == CLASS_CPRIEST
 #endif
	    )) {
		p_ptr->immune_fire = TRUE; csheet_boni->cb[0] |= CB1_IFIRE;
		p_ptr->immune_poison = TRUE; csheet_boni->cb[1] |= CB2_IPOIS;
	}
#endif

	if (r_ptr->flags9 & RF9_RES_LITE) { p_ptr->resist_lite = TRUE; csheet_boni->cb[2] |= CB3_RLITE; }
	if (r_ptr->flags9 & RF9_RES_DARK) { p_ptr->resist_dark = TRUE; csheet_boni->cb[2] |= CB3_RDARK; }
	if (r_ptr->flags9 & RF9_RES_BLIND) { p_ptr->resist_blind = TRUE; csheet_boni->cb[1] |= CB2_RBLND; }
	if (r_ptr->flags9 & RF9_RES_SOUND) { p_ptr->resist_sound = TRUE; csheet_boni->cb[2] |= CB3_RSOUN; }
	if (r_ptr->flags9 & RF9_RES_CHAOS) { p_ptr->resist_chaos = TRUE; csheet_boni->cb[3] |= CB4_RCHAO; }
	if (r_ptr->flags9 & RF9_RES_TIME) { p_ptr->resist_time = TRUE; csheet_boni->cb[3] |= CB4_RTIME; }
	if (r_ptr->flags9 & RF9_RES_MANA) { p_ptr->resist_mana = TRUE; csheet_boni->cb[3] |= CB4_RMANA; }
	if ((r_ptr->flags9 & RF9_RES_SHARDS) || (r_ptr->flags3 & RF3_HURT_ROCK))
		{ p_ptr->resist_shard = TRUE;  csheet_boni->cb[2] |= CB3_RSHRD; }
	if (r_ptr->flags3 & RF3_RES_TELE) { p_ptr->res_tele = TRUE; csheet_boni->cb[4] |= CB5_RTELE; }
	if (r_ptr->flags9 & RF9_IM_TELE) { p_ptr->res_tele = TRUE; csheet_boni->cb[4] |= CB5_RTELE; }
	if (r_ptr->flags3 & RF3_RES_WATE) { p_ptr->resist_water = TRUE; csheet_boni->cb[3] |= CB4_RWATR; }
	if (r_ptr->flags7 & RF7_AQUATIC) { p_ptr->resist_water = TRUE; csheet_boni->cb[3] |= CB4_RWATR; }
	if (r_ptr->flags3 & RF3_RES_NETH) { p_ptr->resist_neth = TRUE; csheet_boni->cb[2] |= CB3_RNETH; }
	if (r_ptr->flags3 & RF3_RES_NEXU) { p_ptr->resist_nexus = TRUE; csheet_boni->cb[2] |= CB3_RNEXU; }
	if (r_ptr->flags3 & RF3_RES_DISE) { p_ptr->resist_disen = TRUE; csheet_boni->cb[3] |= CB4_RDISE; }
	if (r_ptr->flags3 & RF3_NO_FEAR) { p_ptr->resist_fear = TRUE; csheet_boni->cb[4] |= CB5_RFEAR; }
	if (r_ptr->flags3 & RF3_NO_SLEEP) { p_ptr->free_act = TRUE; csheet_boni->cb[4] |= CB5_RPARA; }
	if (r_ptr->flags3 & RF3_NO_CONF) { p_ptr->resist_conf = TRUE; csheet_boni->cb[3] |= CB4_RCONF; }
	if (r_ptr->flags3 & RF3_NO_STUN) { p_ptr->resist_sound = TRUE; csheet_boni->cb[2] |= CB3_RSOUN; }
	if (r_ptr->flags8 & RF8_NO_CUT) { p_ptr->no_cut = TRUE; csheet_boni->cb[12] |= CB13_XNCUT; }
	if (r_ptr->flags7 & RF7_CAN_FLY) {
		p_ptr->levitate = TRUE; csheet_boni->cb[5] |= CB6_RLVTN;
		p_ptr->feather_fall = TRUE; csheet_boni->cb[5] |= CB6_RFFAL;
	}
	if (r_ptr->flags7 & RF7_CAN_SWIM) { p_ptr->can_swim = TRUE; csheet_boni->cb[12] |= CB13_XSWIM; }
	if (r_ptr->flags2 & RF2_REFLECTING) { p_ptr->reflect = TRUE; csheet_boni->cb[6] |= CB7_RREFL; }
	if (r_ptr->flags7 & RF7_DISBELIEVE) {
#if 0
		p_ptr->antimagic += r_ptr->level / 2 + 20; csheet_boni->amfi += r_ptr->level / 2 + 20;
		p_ptr->antimagic_dis += r_ptr->level / 15 + 3;
#else /* a bit stricter for mimics.. */
		p_ptr->antimagic += r_ptr->level / 2 + 10; csheet_boni->amfi += r_ptr->level / 2 + 10;
		p_ptr->antimagic_dis += r_ptr->level / 50 + 2;
#endif
	}

	if (((r_ptr->flags2 & RF2_WEIRD_MIND) ||
	    (r_ptr->flags9 & RF9_RES_PSI))
	    && !p_ptr->reduce_insanity)
		{ p_ptr->reduce_insanity = 1; csheet_boni->cb[3] |= CB4_RMIND; }
	if ((r_ptr->flags2 & RF2_EMPTY_MIND) ||
	    (r_ptr->flags9 & RF9_IM_PSI))
		{ p_ptr->reduce_insanity = 2; csheet_boni->cb[3] |= CB5_XMIND; }

	/* as long as not all resistances are implemented in r_info, use workaround via breaths */
	if (r_ptr->flags4 & RF4_BR_LITE) { p_ptr->resist_lite = TRUE; csheet_boni->cb[2] |= CB3_RLITE; }
	if (r_ptr->flags4 & RF4_BR_DARK) { p_ptr->resist_dark = TRUE; csheet_boni->cb[2] |= CB3_RDARK; }
	/* if (r_ptr->flags & RF__) p_ptr->resist_blind = TRUE; */
	if (r_ptr->flags4 & RF4_BR_SOUN) { p_ptr->resist_sound = TRUE; csheet_boni->cb[2] |= CB3_RSOUN; }
	if (r_ptr->flags4 & RF4_BR_SHAR) { p_ptr->resist_shard = TRUE; csheet_boni->cb[2] |= CB3_RSHRD; }
	if (r_ptr->flags4 & RF4_BR_CHAO) { p_ptr->resist_chaos = TRUE; csheet_boni->cb[3] |= CB4_RCHAO; }
	if (r_ptr->flags4 & RF4_BR_TIME) { p_ptr->resist_time = TRUE; csheet_boni->cb[3] |= CB4_RTIME; }
	if (r_ptr->flags4 & RF4_BR_MANA) { p_ptr->resist_mana = TRUE; csheet_boni->cb[3] |= CB4_RMANA; }
	if (r_ptr->flags4 & RF4_BR_PLAS) {
		p_ptr->resist_elec = TRUE; csheet_boni->cb[0] |= CB1_RELEC;
		p_ptr->resist_sound = TRUE; csheet_boni->cb[2] |= CB3_RSOUN;
		/* handled in the immunity selection code above!
		p_ptr->immune_fire = TRUE; csheet_boni->cb[0] |= CB1_IFIRE; */
	}
	if (r_ptr->flags4 & RF4_BR_NUKE) {
		p_ptr->resist_acid = TRUE; csheet_boni->cb[1] |= CB2_RACID;
		p_ptr->resist_pois = TRUE; csheet_boni->cb[1] |= CB2_RPOIS;
	}
	if (r_ptr->flags4 & RF4_BR_WALL) { p_ptr->resist_sound = TRUE; csheet_boni->cb[2] |= CB3_RSOUN; }
	/* if ((r_ptr->flags4 & RF4_BR_WATE) || <- does not exist */
	if (r_ptr->flags4 & RF4_BR_NETH) { p_ptr->resist_neth = TRUE; csheet_boni->cb[2] |= CB3_RNETH; }
	/* res_neth_somewhat: (r_ptr->flags3 & RF3_EVIL) */
	if (r_ptr->flags4 & RF4_BR_NEXU) { p_ptr->resist_nexus = TRUE; csheet_boni->cb[2] |= CB3_RNEXU; }
	if (r_ptr->flags4 & RF4_BR_DISE) { p_ptr->resist_disen = TRUE; csheet_boni->cb[3] |= CB4_RDISE; }

	/* The following BR-to-RES will be needed even with all of above RES implemented: */
	if (r_ptr->flags4 & RF4_BR_GRAV) { p_ptr->feather_fall = TRUE; csheet_boni->cb[5] |= CB6_RFFAL; }
	if (r_ptr->flags4 & RF4_BR_INER) { p_ptr->free_act = TRUE; csheet_boni->cb[4] |= CB5_RPARA; }

	/* If not changed, spells didnt changed too, no need to send them */
	if (!p_ptr->body_changed) {
		/* restore RNG */
		Rand_quick = FALSE;
		return;
	}
	p_ptr->body_changed = FALSE;

#if 0	/* moved so that 2 handed weapons etc can be checked for */
	/* Take off what is no more usable */
	do_takeoff_impossible(Ind);
#endif

	/* Hack -- cancel wraithform upon form change  */
	if (!(r_ptr->flags2 & RF2_PASS_WALL) && p_ptr->tim_wraith)
		p_ptr->tim_wraith = 1;

	/* Update the innate spells */
	calc_body_spells(Ind);

	/* restore RNG */
	Rand_quick = FALSE;
}

/* update innate spells */
void calc_body_spells(int Ind) {
	player_type *p_ptr = Players[Ind];
	monster_race *r_ptr;

	/* If in the player body nothing has to be done */
	if (!p_ptr->body_monster) return;

	r_ptr = &r_info[p_ptr->body_monster];

	p_ptr->innate_spells[0] = r_ptr->flags4 & RF4_PLAYER_SPELLS;
	p_ptr->innate_spells[1] = r_ptr->flags5 & RF5_PLAYER_SPELLS;
	p_ptr->innate_spells[2] = r_ptr->flags6 & RF6_PLAYER_SPELLS;
	Send_spell_info(Ind, 0, 0, 0, "nothing");
}

#if 0	// moved to defines.h
bool monk_heavy_armor(int Ind) {
 #if 1 // DGDGDGDG -- no more monks for the time being
	player_type *p_ptr = Players[Ind];
	u16b monk_arm_wgt = 0;

//	if (!(p_ptr->pclass == CLASS_MONK)) return FALSE;
	if (!get_skill(p_ptr, SKILL_MARTIAL_ARTS)) return FALSE;

	/* Weight the armor */
	monk_arm_wgt = armour_weight(p_ptr);
  #if 0
	monk_arm_wgt += p_ptr->inventory[INVEN_BODY].weight;
	monk_arm_wgt += p_ptr->inventory[INVEN_HEAD].weight;
	monk_arm_wgt += p_ptr->inventory[INVEN_ARM].weight;
	monk_arm_wgt += p_ptr->inventory[INVEN_OUTER].weight;
	monk_arm_wgt += p_ptr->inventory[INVEN_HANDS].weight;
	monk_arm_wgt += p_ptr->inventory[INVEN_FEET].weight;
  #endif	// 0

//	return (monk_arm_wgt > ( 100 + (p_ptr->lev * 4))) ;
	return (monk_arm_wgt > 50 + get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 200));
 #endif
}
#endif	// 0

/* Are all the weapons wielded of the right type ? */
int get_weaponmastery_skill(player_type *p_ptr, object_type *o_ptr) {
	/* no item */
	if (!o_ptr->k_idx) return -1;

#if 1
	/* EXPERIMENTAL - Hack for priests:
	   They always get full weapon skill if the weapon is BLESSED,
	   even if it's not a 'Blunt' type weapon: */
	if (p_ptr->pclass == CLASS_PRIEST && o_ptr->tval != TV_BLUNT && is_melee_weapon(o_ptr->tval)) {
		u32b dummy, f3;

		object_flags(o_ptr, &dummy, &dummy, &f3, &dummy, &dummy, &dummy, &dummy);
		if (f3 & TR3_BLESSED) return SKILL_BLUNT;
	}
#endif

	switch (o_ptr->tval) {
	/* known weapon types */
	case TV_SWORD:		return SKILL_SWORD;
	case TV_AXE:		return SKILL_AXE;
	case TV_BLUNT:		return SKILL_BLUNT;
	case TV_POLEARM:	return SKILL_POLEARM;
	/* not a weapon */
	case TV_SHIELD:		return -1;
	/* unknown weapon type */
	default:		return -1;
	}
}

/* Are all the ranged weapons wielded of the right type ? */
int get_archery_skill(player_type *p_ptr) {
	int skill = 0;
	object_type *o_ptr;

	o_ptr = &p_ptr->inventory[INVEN_BOW];

	if (!o_ptr->k_idx) return -1;

	/* Hack -- Boomerang skill */
	if (o_ptr->tval == TV_BOOMERANG) return SKILL_BOOMERANG;

	switch (o_ptr->sval / 10) {
		case 0:
			if ((!skill) || (skill == SKILL_SLING)) skill = SKILL_SLING;
			else skill = -1;
			break;
		case 1:
			if ((!skill) || (skill == SKILL_BOW)) skill = SKILL_BOW;
			else skill = -1;
			break;
		case 2:
			if ((!skill) || (skill == SKILL_XBOW)) skill = SKILL_XBOW;
			else skill = -1;
			break;
	}

	/* Everything is ok */
	return skill;
}


int calc_blows_obj(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];
	int str_index, dex_index, eff_weight = o_ptr->weight;
	u32b f1, f2, f3, f4, f5, f6, esp;
	int num = 0, wgt = 0, mul = 0, div = 0, num_blow = 0, str_adj;


	/* Extract the item flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

	if (f4 & TR4_NEVER_BLOW) return 0;

	/* cap for Grond. Heaviest normal weapon is MoD at 40.0, which Grond originally was, too. - C. Blue */
	if (eff_weight > 400) eff_weight = 400;

	/* Weapons which can be wielded 2-handed are easier to swing
	   than with one hand - experimental - C. Blue */
	if ((!p_ptr->inventory[INVEN_ARM].k_idx || /* don't forget dual-wield.. weapon might be in the other hand! */
	    (!p_ptr->inventory[INVEN_WIELD].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) &&
	    (k_info[o_ptr->k_idx].flags4 & (TR4_SHOULD2H | TR4_MUST2H | TR4_COULD2H)))
		eff_weight = (eff_weight * 2) / 3; /* probably more sensible, but..see the line below :/ */
		//too easy!	eff_weight = (eff_weight * 1) / 2; /* get 2bpr with 18/30 str even with broad axe starting weapon */

	/* Analyze the class */
	switch (p_ptr->pclass) {
		case CLASS_ADVENTURER: num = 4; wgt = 35; mul = 4; break;//wgt=35, but MA is too easy in comparison. //was num = 5; ; mul = 6
		case CLASS_WARRIOR: num = 6; wgt = 30; mul = 5; break;
		case CLASS_MAGE: num = 1; wgt = 40; mul = 2; break; //was num = 3; ;
#ifdef ENABLE_CPRIEST
		case CLASS_CPRIEST:
#endif
		case CLASS_PRIEST: num = 4; wgt = 35; mul = 4; break;//mul3 //was num = 5; ;
		case CLASS_ROGUE: num = 5; wgt = 30; mul = 4; break; /* was mul = 3 - C. Blue - EXPERIMENTAL */
		/* I'm a rogue like :-o */
		case CLASS_RUNEMASTER: num = 4; wgt = 30; mul = 4; break;//was wgt = 40
		//trying 5bpr	case CLASS_MIMIC: num = 4; wgt = 30; mul = 4; break;//mul3
		case CLASS_MIMIC: num = 5; wgt = 35; mul = 5; break;//mul3; mul4!
		//case CLASS_ARCHER: num = 3; wgt = 30; mul = 3; break;
		case CLASS_ARCHER: num = 3; wgt = 35; mul = 4; break;
#ifdef ENABLE_DEATHKNIGHT
		case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
		case CLASS_HELLKNIGHT:
#endif
		case CLASS_PALADIN: num = 5; wgt = 35; mul = 5; break;//mul4
		case CLASS_RANGER: num = 5; wgt = 35; mul = 4; break;//mul4
		case CLASS_DRUID: num = 4; wgt = 35; mul = 4; break;
		/* if he is to become a spellcaster, necro working on spell-kills:
		case CLASS_SHAMAN: num = 2; wgt = 40; mul = 3; break;
		    however, then Martial Arts would require massive nerfing too for this class (or being removed even).
		    otherwise, let's compromise for now: */
		case CLASS_SHAMAN: num = 4; wgt = 35; mul = 4; break;
		case CLASS_MINDCRAFTER: num = 5; wgt = 35; mul = 4; break;//was 4,30,4
		/*case CLASS_BARD: num = 4; wgt = 35; mul = 4; break; */
	}

	/* Enforce a minimum "weight" (tenth pounds) */
	div = ((eff_weight < wgt) ? wgt : eff_weight);

	/* Access the strength vs weight */
	str_adj = adj_str_blow[p_ptr->stat_ind[A_STR]];
	if (str_adj == 240) str_adj = 426; /* hack to reach 6 bpr with 400 lb weapons (max) at *** STR - C. Blue */
	str_index = ((str_adj * mul) / div);

	/* Maximal value */
	if (str_index > 11) str_index = 11;

	/* Index by dexterity */
	dex_index = (adj_dex_blow[p_ptr->stat_ind[A_DEX]]);

	/* Maximal value */
	if (dex_index > 11) dex_index = 11;

	/* Use the blows table */
	num_blow = blows_table[str_index][dex_index];

	/* Maximal value */
	if (num_blow > num) num_blow = num;

	/* Require at least one blow */
	if (num_blow < 1) num_blow = 1;


	/* Boost blows with masteries */
	if ((num = get_weaponmastery_skill(p_ptr, o_ptr)) != -1)
		num_blow += get_skill_scale(p_ptr, num, 2);


	/* Extra Attacks from item */
	if (f1 & TR1_BLOWS) {
		if (o_ptr->name1) num_blow += o_ptr->pval;
		else {
			if (k_info[o_ptr->k_idx].flags1 & TR1_BLOWS) num_blow += o_ptr->bpval;
			if (o_ptr->name2) {
				artifact_type *a_ptr = ego_make(o_ptr);

				f1 &= ~(k_info[o_ptr->k_idx].flags1 & TR1_PVAL_MASK & ~a_ptr->flags1);
				if (f1 & TR1_BLOWS) num_blow += o_ptr->pval;
			}
		}
	}

	return (num_blow);
}

int calc_blows_weapons(int Ind) {
	player_type *p_ptr = Players[Ind];
	int num_blow = 0, blows1 = 0, blows2 = 0;


	/* calculate blows with weapons (includes weaponmastery boni already) */

	if (p_ptr->inventory[INVEN_WIELD].k_idx) blows1 = calc_blows_obj(Ind, &p_ptr->inventory[INVEN_WIELD]);
	else if (p_ptr->inventory[INVEN_ARM].k_idx) blows1 = calc_blows_obj(Ind, &p_ptr->inventory[INVEN_ARM]);
	if (p_ptr->dual_wield) blows2 = calc_blows_obj(Ind, &p_ptr->inventory[INVEN_ARM]);


	/* apply dual-wield boni and calculations */

	/* mediate for dual-wield */
#if 0 /* round down? (see bpr bonus below too) (encourages bpr gloves/crit weapons - makes more sense?) */
	if (p_ptr->dual_wield && p_ptr->dual_mode) num_blow = (blows1 + blows2) / 2;
#elif 0 /* round up? (see bpr bonus below too) (encounrages crit gloves/bpr weapons) */
	if (p_ptr->dual_wield && p_ptr->dual_mode) num_blow = (blows1 + blows2 + 1) / 2;
#else /* round up, but only if we aren't anti-dual-wield-encumbered! */
	if (p_ptr->dual_wield && p_ptr->dual_mode) {
		if (!p_ptr->rogue_heavyarmor) num_blow = (blows1 + blows2 + 1) / 2;
		/* if encumbered, we cannot gain any dual-wield advantage! */
		else num_blow = (blows1 + blows2) / 2;
	}
#endif
	else num_blow = blows1;

	/* add dual-wield bonus if we wear light armour! */
	if (!p_ptr->rogue_heavyarmor && p_ptr->dual_wield && p_ptr->dual_mode
	    /* don't give dual-wield EA bonus if one of the weapons is NEVER_BLOW! */
	    && blows1 != 0 && blows2 != 0)
#if 0 /* if rounding down, add percentage bpr bonus maybe */
//		num_blow += (1 + (num_blow - 1) / 5);
		num_blow++;
#else /* if rounding up, add fixed (ie small) bpr bonus! */
//		num_blow += (1 + (num_blow - 1) / 5);  <- warriors could rightfully complain I think, mh.
		num_blow++;
#endif

	/* done */
	return num_blow;
}

int calc_crit_obj(int Ind, object_type *o_ptr) {
	int xcrit = 0;
	u32b f1, f2, f3, f4, f5, f6, esp;

	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
	if (f5 & TR5_CRIT) {
		if (o_ptr->name1) xcrit += o_ptr->pval;
		else {
			if (k_info[o_ptr->k_idx].flags5 & TR5_CRIT) xcrit += o_ptr->bpval;
			if (o_ptr->name2) {
				artifact_type *a_ptr = ego_make(o_ptr);

				f5 &= ~(k_info[o_ptr->k_idx].flags5 & TR5_PVAL_MASK & ~a_ptr->flags5);
				if (f5 & TR5_CRIT) xcrit += o_ptr->pval;
			}
		}
	}

	return (xcrit);
}


/*
 * Calculate the players current "state", taking into account
 * not only race/class intrinsics, but also objects being worn
 * and temporary spell effects.
 *
 * See also calc_mana() and calc_hitpoints().
 *
 * Take note of the new "speed code", in particular, a very strong
 * player will start slowing down as soon as he reaches 150 pounds,
 * but not until he reaches 450 pounds will he be half as fast as
 * a normal kobold.  This both hurts and helps the player, hurts
 * because in the old days a player could just avoid 300 pounds,
 * and helps because now carrying 300 pounds is not very painful.
 *
 * The "weapon" and "bow" do *not* add to the bonuses to hit or to
 * damage, since that would affect non-combat things.  These values
 * are actually added in later, at the appropriate place.
 *
 * This function induces various "status" messages.
 */
void calc_boni(int Ind) {
#ifdef TOGGLE_TELE
	cptr inscription = NULL;
#endif

	player_type *p_ptr = Players[Ind];
	dun_level *l_ptr = getfloor(&p_ptr->wpos);
	cave_type **zcave;

	/* Note: For vampires with auto-ID this is required, because of the 3
	   times that calc_boni() is called on logging in, for those vampires
	   (or V-form users) the first one will be in non-ready connection
	   state, while for other characters all three are actually in ready
	   connection state.
	   This will cause inventory/equipment to be 'broken' for vampires. */
	bool logged_in = get_conn_state_ok(Ind);

	if (!(zcave = getcave(&p_ptr->wpos))) {
		/* for stair-goi: try to repeat this failed calc_boni() call asap - C. Blue */
		p_ptr->update |= PU_BONUS;
		return;
	}
	
	int kk, jj;
	boni_col csheet_boni[15];
	/* Wipe the boni column data */
	for (kk = 0; kk < 15; kk++) {
		csheet_boni[kk].i = kk;
		csheet_boni[kk].spd = 0;
		csheet_boni[kk].slth = 0;
		csheet_boni[kk].srch = 0;
		csheet_boni[kk].infr = 0;
		csheet_boni[kk].lite = 0;
		csheet_boni[kk].dig = 0;
		csheet_boni[kk].blow = 0;
		csheet_boni[kk].crit = 0;
		csheet_boni[kk].shot = 0;
		csheet_boni[kk].migh = 0;
		csheet_boni[kk].mxhp = 0;
		csheet_boni[kk].mxmp = 0;
		csheet_boni[kk].luck = 0;
		csheet_boni[kk].pstr = 0;
		csheet_boni[kk].pint = 0;
		csheet_boni[kk].pwis = 0;
		csheet_boni[kk].pdex = 0;
		csheet_boni[kk].pcon = 0;
		csheet_boni[kk].pchr = 0;
		csheet_boni[kk].amfi = 0;
		csheet_boni[kk].sigl = 0;
		/* Clear the byte flags */
		for (jj = 0; jj < 16; jj++)
			csheet_boni[kk].cb[jj] = 0;
		csheet_boni[kk].color = TERM_DARK;
		csheet_boni[kk].symbol = ' '; //Empty item / form slot.
	}
	
	int j, hold, minus, am_bonus = 0, am_temp;
	long w, i;

	int old_speed;
	int old_num_blow;

	u32b old_telepathy;
	int old_see_inv;

	int old_dis_ac;
	int old_dis_to_a;

	int old_dis_to_h, old_to_h_melee;
	int old_dis_to_d, old_to_d_melee;

	//int extra_blows;
	int extra_shots;
	int extra_spells;
	byte never_blow = 0x0;
	bool never_blow_ranged = FALSE;

	object_type *o_ptr, *o2_ptr;
	object_kind *k_ptr;

	long int d, toac = 0, body = 0;
	monster_race *r_ptr = &r_info[p_ptr->body_monster];

	u32b f1, f2, f3, f4, f5, f6, esp;
	s16b pval;

	bool old_auto_id = p_ptr->auto_id;
	bool old_dual_wield = p_ptr->dual_wield;
	bool melee_weapon;

	bool old_sun_burn;

	int lite_inc_norm = 0, lite_inc_white = 0, old_lite_type;

	/* Equipment boni that might not be applied depending on encumberment */
#ifndef USE_NEW_SHIELDS
	int may_ac = 0;
#endif
#ifndef NEW_SHIELDS_NO_AC
	int may_to_a, may_dis_to_a;
#endif
	bool may_reflect = FALSE;


#ifdef EQUIPMENT_SET_BONUS
	/* for boni of artifact "sets" ie arts of (about) identical name - C. Blue */
	int equipment_set[INVEN_TOTAL - INVEN_WIELD], equipment_set_amount[INVEN_TOTAL - INVEN_WIELD];
	char equipment_set_name[INVEN_TOTAL - INVEN_WIELD][80];
	int equipment_set_bonus = 0;
	char tmp_name[ONAME_LEN], *tmp_name_ptr;

	for (i = 0; i < INVEN_TOTAL - INVEN_WIELD; i++) {
		equipment_set[i] = 0;
		equipment_set_name[i][0] = 0;
	}
#endif


	/* Save the old speed */
	old_speed = p_ptr->pspeed;

	/* Save the old vision stuff */
	old_telepathy = p_ptr->telepathy;
	old_see_inv = p_ptr->see_inv;

	/* Save the old armor class */
	old_dis_ac = p_ptr->dis_ac;
	old_dis_to_a = p_ptr->dis_to_a;

	/* Save the old hit/damage bonuses */
	old_dis_to_h = p_ptr->dis_to_h;
	old_dis_to_d = p_ptr->dis_to_d;

	old_to_h_melee = p_ptr->to_h_melee;
	old_to_d_melee = p_ptr->to_d_melee;

	/* Clear extra blows/shots */
	//extra_blows = extra_shots = extra_spells = 0;
	extra_shots = extra_spells = 0;

	/* Clear the stat modifiers */
	for (i = 0; i < 6; i++) p_ptr->stat_add[i] = 0;

	/* Originally Druidism bonuses */
	if (p_ptr->xtrastat_tim)
		switch (p_ptr->xtrastat_which) {
#ifdef ENABLE_HELLKNIGHT
		case 4: /* Hell Knight's Demonic Strength */
			p_ptr->stat_add[A_STR] = p_ptr->xtrastat_pow; csheet_boni[14].pstr += p_ptr->xtrastat_pow;
			p_ptr->stat_add[A_CON] = p_ptr->xtrastat_pow; csheet_boni[14].pcon += p_ptr->xtrastat_pow;
			p_ptr->sustain_str = TRUE; csheet_boni[14].cb[11] |= CB12_RSSTR;
			p_ptr->sustain_con = TRUE; csheet_boni[14].cb[11] |= CB12_RSCON;
			break;
#endif
		/* Druid's Extra Growth */
		case 3: p_ptr->stat_add[A_INT] = p_ptr->xtrastat_pow; csheet_boni[14].pint += p_ptr->xtrastat_pow;
		case 2: p_ptr->stat_add[A_CON] = p_ptr->xtrastat_pow; csheet_boni[14].pcon += p_ptr->xtrastat_pow;
		case 1: p_ptr->stat_add[A_DEX] = p_ptr->xtrastat_pow; csheet_boni[14].pdex += p_ptr->xtrastat_pow;
		case 0: p_ptr->stat_add[A_STR] = p_ptr->xtrastat_pow; csheet_boni[14].pstr += p_ptr->xtrastat_pow;
		}
#if defined(ENABLE_OHERETICISM) && defined(ENABLE_HELLKNIGHT)
	/* Hack: Blood Sacrifice form may give double-immunity!
	   Reason is that the player gains IM_FIRE at 50 intrinsically, so he'd like IM_POIS from the form, not a redundant IM_FIRE.
	   But the spell is already learnt at 45, when the player doesn't IM_FIRE yet. Now, it'd be weird if the form gave IM_POIS or even a choice instead,
	   especially since the player cannot use 'Select preferred immunity' function due to missing Mimicry skill.
	   So we'll just be generous at levels 45 to 49 and give IM_FIRE for free :-p as at 50 it'll not matter anymore. */
	else if (p_ptr->body_monster == RI_BLOODTHIRSTER
	    && (p_ptr->pclass == CLASS_HELLKNIGHT
 #ifdef CLASS_CPRIEST
	    || p_ptr->pclass == CLASS_CPRIEST
 #endif
	    )) {
		/* Bloodthirster form includes the benefits of 'Demonic Strength' (+max power of the spell) */
		p_ptr->stat_add[A_STR] = 4; csheet_boni[14].pstr += 4;
		p_ptr->stat_add[A_CON] = 4; csheet_boni[14].pcon += 4;
		p_ptr->sustain_str = TRUE; csheet_boni[14].cb[11] |= CB12_RSSTR;
		p_ptr->sustain_con = TRUE; csheet_boni[14].cb[11] |= CB12_RSCON;
	}
#endif


	/* Clear the Displayed/Real armor class */
	p_ptr->dis_ac = p_ptr->ac = 0;

	/* Clear the Displayed/Real Bonuses */
	p_ptr->dis_to_h = p_ptr->to_h = p_ptr->to_h_melee = p_ptr->to_h_ranged = 0;
	p_ptr->dis_to_d = p_ptr->to_d = p_ptr->to_d_melee = p_ptr->to_d_ranged = 0;
	p_ptr->dis_to_a = p_ptr->to_a = 0;


	/* Clear all the flags */
	p_ptr->aggravate = FALSE;
	p_ptr->teleport = FALSE;
	p_ptr->drain_exp = 0;
	p_ptr->drain_mana = 0;
	p_ptr->drain_life = 0;
	p_ptr->bless_blade = FALSE;
	p_ptr->xtra_might = 0;
	p_ptr->impact = FALSE;
	p_ptr->see_inv = FALSE;
	p_ptr->free_act = FALSE;
	p_ptr->slow_digest = FALSE;
	p_ptr->regenerate = FALSE;
	p_ptr->resist_time = FALSE;
	p_ptr->resist_mana = FALSE;
	p_ptr->immune_poison = FALSE;
	p_ptr->immune_water = FALSE;
	p_ptr->resist_water = FALSE;
	p_ptr->regen_mana = FALSE;
	p_ptr->feather_fall = FALSE;
	p_ptr->keep_life = FALSE;
	p_ptr->hold_life = FALSE;
	p_ptr->telepathy = 0;
	p_ptr->lite = FALSE;
	p_ptr->cur_lite = 0;
	p_ptr->cur_vlite = 0;
	p_ptr->sustain_str = FALSE;
	p_ptr->sustain_int = FALSE;
	p_ptr->sustain_wis = FALSE;
	p_ptr->sustain_con = FALSE;
	p_ptr->sustain_dex = FALSE;
	p_ptr->sustain_chr = FALSE;
	p_ptr->resist_acid = FALSE;
	p_ptr->resist_elec = FALSE;
	p_ptr->resist_fire = FALSE;
	p_ptr->resist_cold = FALSE;
	p_ptr->resist_pois = FALSE;
	p_ptr->resist_conf = FALSE;
	p_ptr->resist_sound = FALSE;
	p_ptr->resist_lite = FALSE;
	p_ptr->resist_dark = FALSE;
	p_ptr->resist_chaos = FALSE;
	p_ptr->resist_disen = FALSE;
	p_ptr->resist_discharge = FALSE;
	p_ptr->resist_shard = FALSE;
	p_ptr->resist_nexus = FALSE;
	p_ptr->resist_blind = FALSE;
	p_ptr->resist_neth = FALSE;
	p_ptr->resist_fear = FALSE;
	p_ptr->immune_acid = FALSE;
	p_ptr->immune_elec = FALSE;
	p_ptr->immune_fire = FALSE;
	p_ptr->immune_cold = FALSE;
	p_ptr->sh_fire = p_ptr->sh_fire_fix = FALSE;
	p_ptr->sh_elec = p_ptr->sh_elec_fix = FALSE;
	p_ptr->sh_cold = p_ptr->sh_cold_fix = FALSE;
	p_ptr->brand_fire = FALSE;
	p_ptr->brand_cold = FALSE;
	p_ptr->brand_elec = FALSE;
	p_ptr->brand_acid = FALSE;
	p_ptr->brand_pois = FALSE;
	p_ptr->auto_tunnel = FALSE;
	p_ptr->levitate = FALSE;
	p_ptr->can_swim = FALSE;
	p_ptr->climb = FALSE;
	p_ptr->pass_trees = FALSE;
	p_ptr->luck = 0;
	p_ptr->reduc_fire = 0;
	p_ptr->reduc_cold = 0;
	p_ptr->reduc_elec = 0;
	p_ptr->reduc_acid = 0;
	p_ptr->anti_magic = FALSE;
	p_ptr->auto_id = FALSE;
	p_ptr->reflect = FALSE;
	p_ptr->shield_deflect = 0;
	p_ptr->weapon_parry = 0;
	p_ptr->no_cut = FALSE;
	p_ptr->reduce_insanity = 0;
	//p_ptr->to_s = 0;
	p_ptr->to_m = 0;
	p_ptr->to_l = 0;
	p_ptr->to_hp = 0;
	p_ptr->black_breath_tmp = FALSE;

	p_ptr->dual_wield = FALSE;
	if (p_ptr->inventory[INVEN_WIELD].k_idx &&
	    p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) {
		p_ptr->dual_wield = TRUE;
		/* Don't kill warnings by inspecting weapons/armour in stores! */
		if (!suppress_message)
		if (p_ptr->warning_dual_mode == 0 && !old_dual_wield && !p_ptr->dual_mode) {
			msg_print(Ind, "\374\377yHINT: Dual-wield mode isn't enabled! Press '\377om\377y' to toggle it!");
			s_printf("warning_dual_mode: %s\n", p_ptr->name);
			p_ptr->warning_dual_mode = 1;
		}
	}

	p_ptr->stormbringer = FALSE;

	/* Invisibility */
	p_ptr->invis = 0;
	p_ptr->tim_invis_power = 0;

	p_ptr->immune_neth = FALSE;
	p_ptr->anti_tele = FALSE;
	p_ptr->res_tele = FALSE;
	p_ptr->antimagic = 0;
	p_ptr->antimagic_dis = 0;
	p_ptr->xtra_crit = 0;
	p_ptr->dodge_level = 1; /* everyone may get a lucky evasion :) - C. Blue */

	p_ptr->suscep_fire = FALSE;
	p_ptr->suscep_cold = FALSE;
	p_ptr->suscep_elec = FALSE;
	p_ptr->suscep_acid = FALSE;
	p_ptr->suscep_pois = FALSE;
	p_ptr->suscep_lite = FALSE;
	p_ptr->suscep_good = FALSE;
	p_ptr->suscep_evil = FALSE;
	p_ptr->suscep_life = FALSE;
	p_ptr->resist_continuum = FALSE;
	p_ptr->vampiric_melee = 0;
	p_ptr->vampiric_ranged = 0;

	/* nastiness */
	p_ptr->ty_curse = FALSE;
	p_ptr->dg_curse = FALSE;

	/* Start with a single blow per turn */
	old_num_blow = p_ptr->num_blow;
	p_ptr->num_blow = 1;
	p_ptr->extra_blows = 0;
	/* Start with a single shot per turn */
	p_ptr->num_fire = 1;
	/* Start with a single spell per turn */
	p_ptr->num_spell = 1;

	/* Reset the "xtra" tval */
	p_ptr->tval_xtra = 0;
	/* Reset the "ammo" tval */
	p_ptr->tval_ammo = 0;

	/* Base infravision (purely racial) */
	p_ptr->see_infra = p_ptr->rp_ptr->infra;
	csheet_boni[14].infr = p_ptr->rp_ptr->infra;
	//csheet_boni[14].infr = p_ptr->see_infra;

	/* Base skill -- disarming */
	p_ptr->skill_dis = p_ptr->rp_ptr->r_dis + p_ptr->cp_ptr->c_dis;
	/* Base skill -- magic devices */
	p_ptr->skill_dev = p_ptr->rp_ptr->r_dev + p_ptr->cp_ptr->c_dev;
	/* Base skill -- saving throw */
	p_ptr->skill_sav = p_ptr->rp_ptr->r_sav + p_ptr->cp_ptr->c_sav;
	/* Base skill -- stealth */
	p_ptr->skill_stl = p_ptr->rp_ptr->r_stl + p_ptr->cp_ptr->c_stl;
	/* Base skill -- searching ability */
	p_ptr->skill_srh = p_ptr->rp_ptr->r_srh + p_ptr->cp_ptr->c_srh;
	/* Base skill -- searching frequency */
	p_ptr->skill_fos = p_ptr->rp_ptr->r_fos + p_ptr->cp_ptr->c_fos;
	/* Base skill -- combat (normal) */
	p_ptr->skill_thn = p_ptr->rp_ptr->r_thn + p_ptr->cp_ptr->c_thn;
	/* Base skill -- combat (shooting) */
	p_ptr->skill_thb = p_ptr->rp_ptr->r_thb + p_ptr->cp_ptr->c_thb;
	/* Base skill -- combat (throwing) */
	p_ptr->skill_tht = p_ptr->rp_ptr->r_thb + p_ptr->cp_ptr->c_thb;

	/* Base skill -- digging */
	p_ptr->skill_dig = 0;

	/* Special admin items */
	p_ptr->admin_invuln = p_ptr->admin_invinc = FALSE;

	p_ptr->no_heal = FALSE;


	/* Not a limit, but good place maybe */
	if ((l_ptr && (l_ptr->flags2 & LF2_NO_RES_HEAL)) ||
	    (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_RES_HEAL)))
		p_ptr->no_heal = TRUE;


	/* Obtain ultimate life force hold at 99 */
	if (p_ptr->lev == 99) { p_ptr->keep_life = TRUE; csheet_boni[14].cb[13] |= CB14_ILIFE; }

	/* Calc bonus body */
	if (!p_ptr->body_monster) {
		/* Show the '@' with class colour for the boni page */
		csheet_boni[14].symbol = (p_ptr->fruit_bat) ? 'b' : '@';
		csheet_boni[14].color = class_info[p_ptr->pclass].color;
	} else {
		/* Show the monster form if the player is polymorphed */
		csheet_boni[14].symbol = r_info[p_ptr->body_monster].d_char;
		csheet_boni[14].color = r_info[p_ptr->body_monster].d_attr;
	}
	if (p_ptr->body_monster) calc_body_bonus(Ind, &csheet_boni[14]);
	else {	// if if or switch to switch, that is the problem :)
			/* I vote for p_info ;) */
		/* Update the innate spells */
		p_ptr->innate_spells[0] = 0x0;
		p_ptr->innate_spells[1] = 0x0;
		p_ptr->innate_spells[2] = 0x0;
		if (!suppress_boni && logged_in) Send_spell_info(Ind, 0, 0, 0, "nothing");

		/* Start with "normal" speed */
		p_ptr->pspeed = 110;

#ifdef ARCADE_SERVER
		p_ptr->pspeed = 130;
		if (p_ptr->stun > 0) p_ptr->pspeed -= 3;
#endif

		/* Bats get +10 speed ... they need it!*/
		if (p_ptr->fruit_bat) {
			if (p_ptr->fruit_bat == 1) {
				p_ptr->pspeed += 10; //disabled due to bat-party-powerlevel-cheezing
				//p_ptr->pspeed += (3 + (p_ptr->lev > 49 ? 7 : p_ptr->lev / 7)); // +10 eventually.
				//p_ptr->pspeed += (3 + (p_ptr->lev > 42 ? 7 : p_ptr->lev / 6)); // +10 eventually.
				//p_ptr->pspeed += (3 + (p_ptr->lev > 35 ? 7 : p_ptr->lev / 5)); // +10 eventually.
				if (p_ptr->vampiric_melee < 50) { p_ptr->vampiric_melee = 50; csheet_boni->cb[6] |= CB7_RVAMP; }
			} else {
				p_ptr->pspeed += 3;
				if (p_ptr->vampiric_melee < 33) { p_ptr->vampiric_melee = 33; csheet_boni->cb[6] |= CB7_RVAMP; }
			}
			p_ptr->levitate = TRUE; csheet_boni[14].cb[5] |= CB6_RLVTN;
			p_ptr->feather_fall = TRUE; csheet_boni[14].cb[5] |= CB6_RFFAL;
		}
		
		csheet_boni[14].spd = p_ptr->pspeed - 110;
	/* Choosing a race just for its HP or low exp% shouldn't be what we want -C. Blue- */
	}


	/* Half-Elf */
	if (p_ptr->prace == RACE_HALF_ELF) {
		p_ptr->resist_lite = TRUE; csheet_boni[14].cb[2] |= CB3_RLITE;
	}

	/* Elf */
	else if (p_ptr->prace == RACE_ELF) {
		p_ptr->see_inv = TRUE; csheet_boni[14].cb[4] |= CB5_RSINV;
		p_ptr->resist_lite = TRUE; csheet_boni[14].cb[2] |= CB3_RLITE;
	}

	/* Hobbit */
	else if (p_ptr->prace == RACE_HOBBIT) {
		p_ptr->sustain_dex = TRUE; csheet_boni[14].cb[11] |= CB12_RSDEX;

		/* DEX bonus for NOT wearing shoes */
		/* not while in mimicried form */
		if (!p_ptr->body_monster && !p_ptr->inventory[INVEN_FEET].k_idx)
			{ p_ptr->stat_add[A_DEX] += 2;  csheet_boni[14].pdex += 2; }
	}

	/* Gnome */
	else if (p_ptr->prace == RACE_GNOME) { p_ptr->free_act = TRUE; csheet_boni[14].cb[4] |= CB5_RPARA; }

	/* Dwarf */
	else if (p_ptr->prace == RACE_DWARF) {
		p_ptr->resist_blind = TRUE; csheet_boni[14].cb[1] |= CB2_RBLND;
		/* not while in mimicried form */
		if (!p_ptr->body_monster && p_ptr->lev >= 30) { p_ptr->climb = TRUE; csheet_boni[14].cb[5] |= CB6_RCLMB; }
	}

	/* Half-Orc */
	else if (p_ptr->prace == RACE_HALF_ORC) { p_ptr->resist_dark = TRUE; csheet_boni[14].cb[2] |= CB3_RDARK; }

	/* Half-Troll */
	else if (p_ptr->prace == RACE_HALF_TROLL) {
		p_ptr->sustain_str = TRUE; csheet_boni[14].cb[11] |= CB12_RSSTR;
		p_ptr->regenerate = TRUE; csheet_boni[14].cb[5] |= CB6_RRGHP;
		/* especially tough skin */
		p_ptr->to_a += 2;
		p_ptr->dis_to_a += 2;
	}

	/* Dunadan */
	else if (p_ptr->prace == RACE_DUNADAN) { p_ptr->sustain_con = TRUE;  csheet_boni[14].cb[11] |= CB12_RSCON; }

	/* High Elf */
	else if (p_ptr->prace == RACE_HIGH_ELF) {
		p_ptr->resist_lite = TRUE; csheet_boni[14].cb[2] |= CB3_RLITE;
		p_ptr->see_inv = TRUE; csheet_boni[14].cb[4] |= CB5_RSINV;
		p_ptr->resist_time = TRUE; csheet_boni[14].cb[3] |= CB4_RTIME;
	}

	/* Yeek */
	else if (p_ptr->prace == RACE_YEEK) {
		p_ptr->feather_fall = TRUE; csheet_boni[14].cb[5] |= CB6_RFFAL;
		/* not while in mimicried form */
		if (!p_ptr->body_monster) { p_ptr->pass_trees = TRUE; csheet_boni[14].cb[12] |= CB13_XTREE; }
	}

	/* Goblin */
	else if (p_ptr->prace == RACE_GOBLIN) {
		p_ptr->resist_dark = TRUE; csheet_boni[14].cb[2] |= CB3_RDARK;
		/* not while in mimicried form */
		/*if (!p_ptr->body_monster) p_ptr->feather_fall = TRUE;*/
	}

	/* Ent */
	else if (p_ptr->prace == RACE_ENT) {
		/* always a bit slowish */
		p_ptr->slow_digest = TRUE; csheet_boni[14].cb[6] |= CB7_RFOOD;
		/* even while in different form? */
		p_ptr->suscep_fire = TRUE; csheet_boni[14].cb[0] |= CB1_SFIRE;
		p_ptr->resist_water = TRUE; csheet_boni[14].cb[3] |= CB4_RWATR;

		/* not while in mimicried form */
		if (!p_ptr->body_monster) {
			p_ptr->pspeed -= 2; csheet_boni[14].spd -= 2;
			p_ptr->can_swim = TRUE; csheet_boni[14].cb[12] |= CB13_XSWIM; /* wood? */
			p_ptr->pass_trees = TRUE; csheet_boni[14].cb[12] |= CB13_XTREE;
			/* tree bark is  harder than skin */
			p_ptr->to_a += 5;
			p_ptr->dis_to_a += 5;
		} else { p_ptr->pspeed -= 1; csheet_boni[14].spd -= 1; } /* it's cost of ent's power, isn't it? */

		if (p_ptr->lev >= 4) { p_ptr->see_inv = TRUE; csheet_boni[14].cb[4] |= CB5_RSINV; }

		if (p_ptr->lev >= 10) { p_ptr->telepathy |= ESP_ANIMAL; csheet_boni[14].cb[7] |= CB8_EANIM; }
		if (p_ptr->lev >= 15) { p_ptr->telepathy |= ESP_ORC; csheet_boni[14].cb[7] |= CB8_EORCS; }
		if (p_ptr->lev >= 20) { p_ptr->telepathy |= ESP_TROLL; csheet_boni[14].cb[7] |= CB8_ETROL; }
		if (p_ptr->lev >= 25) { p_ptr->telepathy |= ESP_GIANT; csheet_boni[14].cb[7] |= CB8_EGIAN; }
		if (p_ptr->lev >= 30) { p_ptr->telepathy |= ESP_DRAGON; csheet_boni[14].cb[8] |= CB9_EDRGN; }
		if (p_ptr->lev >= 40) { p_ptr->telepathy |= ESP_DEMON; csheet_boni[14].cb[8] |= CB9_EDEMN; }
		if (p_ptr->lev >= 50) { p_ptr->telepathy |= ESP_EVIL; csheet_boni[14].cb[9] |= CB10_EEVIL; }
	}

	/* Draconian (former Dragonrider, Thunderlord) */
	else if (p_ptr->prace == RACE_DRACONIAN) {
		/* not while in mimicried form */
		if (!p_ptr->body_monster) { p_ptr->feather_fall = TRUE; csheet_boni[14].cb[5] |= CB6_RFFAL; }

		if (p_ptr->lev >= 5) { p_ptr->telepathy |= ESP_DRAGON; csheet_boni[14].cb[8] |= CB9_EDRGN; }
#ifndef ENABLE_DRACONIAN_TRAITS
		if (p_ptr->lev >= 10) { p_ptr->resist_fire = TRUE; csheet_boni[14].cb[0] |= CB1_RFIRE; }
		if (p_ptr->lev >= 15) { p_ptr->resist_cold = TRUE; csheet_boni[14].cb[0] |= CB1_RCOLD; }
		if (p_ptr->lev >= 20) { p_ptr->resist_acid = TRUE; csheet_boni[14].cb[1] |= CB2_RACID; }
		if (p_ptr->lev >= 25) { p_ptr->resist_elec = TRUE; csheet_boni[14].cb[0] |= CB1_RELEC; }
#endif
		/* not while in mimicried form */
		if (!p_ptr->body_monster) {
			if (p_ptr->lev >= 30) { p_ptr->levitate = TRUE; csheet_boni[14].cb[5] |= CB6_RLVTN; }
			/* scales are harder than skin */
			p_ptr->to_a += 4;
			p_ptr->dis_to_a += 4;
		}
	}

	/* Dark-Elves */
	else if (p_ptr->prace == RACE_DARK_ELF) {
		p_ptr->suscep_lite = TRUE; csheet_boni[14].cb[2] |= CB3_SLITE;
		//p_ptr->suscep_good = TRUE;//maybe too harsh
		p_ptr->resist_dark = TRUE; csheet_boni[14].cb[2] |= CB3_RDARK;
		if (p_ptr->lev >= 20) { p_ptr->see_inv = TRUE; csheet_boni[14].cb[4] |= CB5_RSINV; }
	}

	/* Vampires */
	else if (p_ptr->prace == RACE_VAMPIRE) {
		p_ptr->suscep_lite = TRUE; csheet_boni[14].cb[2] |= CB3_SLITE;
		p_ptr->suscep_good = TRUE;
		p_ptr->suscep_life = TRUE;

		p_ptr->resist_time = TRUE; csheet_boni[14].cb[3] |= CB4_RTIME;
		p_ptr->resist_dark = TRUE; csheet_boni[14].cb[2] |= CB3_RDARK;
		p_ptr->resist_cold = TRUE; csheet_boni[14].cb[0] |= CB1_RCOLD;
		p_ptr->resist_neth = TRUE; csheet_boni[14].cb[2] |= CB3_RNETH;
		p_ptr->immune_poison = TRUE; csheet_boni[14].cb[1] |= CB2_IPOIS;
		p_ptr->hold_life = TRUE; csheet_boni[14].cb[5] |= CB6_RLIFE;

		p_ptr->reduce_insanity = 1; csheet_boni[14].cb[3] |= CB4_RMIND;

		if (p_ptr->vampiric_melee < 50) p_ptr->vampiric_melee = 50; /* mimic forms give 50 (50% bite attacks) - 33 was actually pretty ok, for lower levels at least */
		csheet_boni[14].cb[6] |= CB7_RVAMP;
		/* Necro+Trauma combo intrinsic vamp boost for true vampires */
		if (get_skill(p_ptr, SKILL_NECROMANCY) >= 25 && get_skill(p_ptr, SKILL_TRAUMATURGY) >= 25
#if 0 /* Chh screen currently doesn't support a 'full vampirism' flag for '@' column, it's instead using some nasty hard-coded hacks.. */
		    && p_ptr->vampiric_melee <= get_skill(p_ptr, SKILL_NECROMANCY) + get_skill(p_ptr, SKILL_TRAUMATURGY)) { /* '<=' instead of '<' to trigger the csheet-boni */
			p_ptr->vampiric_melee = get_skill(p_ptr, SKILL_NECROMANCY) + get_skill(p_ptr, SKILL_TRAUMATURGY);
			if (p_ptr->vampiric_melee == 100) csheet_boni[14].cb[6] |= CB7_RVAMP;
		}
#else
		    && p_ptr->vampiric_melee < get_skill(p_ptr, SKILL_NECROMANCY) + get_skill(p_ptr, SKILL_TRAUMATURGY))
			p_ptr->vampiric_melee = get_skill(p_ptr, SKILL_NECROMANCY) + get_skill(p_ptr, SKILL_TRAUMATURGY);
#endif

		/* sense surroundings without light source! (virtual lite / dark light) */
		p_ptr->cur_vlite = 1 + p_ptr->lev / 10; csheet_boni[14].lite = 1 + p_ptr->lev / 10;
		csheet_boni[14].cb[12] |= CB13_XLITE;
		//if (p_ptr->lev >= 30) p_ptr->levitate = TRUE; can poly into bat instead

#ifdef ENABLE_DEATHKNIGHT
		if (p_ptr->pclass == CLASS_DEATHKNIGHT) p_ptr->resist_fear = TRUE;
#endif
	}
#ifdef ENABLE_MAIA
	else if (p_ptr->prace == RACE_MAIA) {
		//Help em out a little.. (to locate candlebearer/darkling)
		p_ptr->telepathy |= ESP_DEMON; csheet_boni[14].cb[8] |= CB9_EDEMN;
		p_ptr->telepathy |= ESP_GOOD; csheet_boni[14].cb[9] |= CB10_EGOOD;

		if (p_ptr->ptrait == TRAIT_ENLIGHTENED) {
			if (p_ptr->lev >= 20) csheet_boni[14].cb[6] |= CB7_IFOOD;
			if (p_ptr->lev >= 50) csheet_boni[14].cb[9] |= CB10_SEVIL;
			p_ptr->suscep_evil = TRUE;

			p_ptr->see_inv = TRUE; csheet_boni[14].cb[4] |= CB5_RSINV;
			p_ptr->resist_lite = TRUE; csheet_boni[14].cb[2] |= CB3_RLITE;
			if (p_ptr->lev >= 20) {
				int l = ((p_ptr->lev > 50 ? 50 : p_ptr->lev) - 20) / 2;

				p_ptr->cur_lite += 1 + (p_ptr->lev - 20) / 6; csheet_boni[14].lite = 1 + p_ptr->lev / 10; //REAL light!
				csheet_boni[14].cb[12] |= CB13_XLITE;
				lite_inc_white += 1 + (p_ptr->lev - 20) / 6;
				p_ptr->to_a += l;
				p_ptr->dis_to_a += l;
			}

			if (p_ptr->lev >= 50) {
				p_ptr->resist_pois = TRUE; csheet_boni[14].cb[1] |= CB2_RPOIS;
				p_ptr->resist_elec = TRUE; csheet_boni[14].cb[0] |= CB1_RELEC;
				p_ptr->sh_elec = TRUE; csheet_boni[14].cb[10] |= CB11_AELEC;
				p_ptr->resist_cold = TRUE; csheet_boni[14].cb[0] |= CB1_RCOLD;
				p_ptr->sh_cold = TRUE; csheet_boni[14].cb[10] |= CB11_ACOLD;
				p_ptr->levitate = TRUE; csheet_boni[14].cb[5] |= CB6_RLVTN;
				p_ptr->feather_fall = TRUE; csheet_boni[14].cb[5] |= CB6_RFFAL;
			}

			/* Bonus resistance for the good side */
			if (p_ptr->divine_xtra_res_time > 0)
				p_ptr->resist_time = TRUE;
		} else if (p_ptr->ptrait == TRAIT_CORRUPTED) {
			if (p_ptr->lev >= 20) csheet_boni[14].cb[6] |= CB7_IFOOD;
			p_ptr->suscep_good = TRUE;

			p_ptr->resist_fire = TRUE; csheet_boni[14].cb[0] |= CB1_RFIRE;
			p_ptr->resist_dark = TRUE; csheet_boni[14].cb[2] |= CB3_RDARK;

			if (p_ptr->lev >= 50) {
				p_ptr->resist_pois = TRUE; csheet_boni[14].cb[1] |= CB2_RPOIS;
				p_ptr->immune_fire = TRUE; csheet_boni[14].cb[0] |= CB1_IFIRE;
				p_ptr->sh_fire = p_ptr->sh_fire_fix = TRUE; csheet_boni[14].cb[10] |= CB11_AFIRE;
			}

			/* Bonus crit for the bad side */
			if (p_ptr->divine_crit > 0)
				p_ptr->xtra_crit = p_ptr->divine_crit_mod;
		} else { p_ptr->slow_digest = TRUE; csheet_boni[14].cb[6] |= CB7_RFOOD; }
	}
#endif
#ifdef ENABLE_KOBOLD
	/* Kobolds */
	else if (p_ptr->prace == RACE_KOBOLD)
		{ p_ptr->resist_pois = TRUE; csheet_boni[14].cb[1] |= CB2_RPOIS; }
#endif

	/* Apply boni from racial special traits */
	switch (p_ptr->ptrait) {
	case TRAIT_NONE: /* N/A */
	default:
		break;
#ifdef ENABLE_DRACONIAN_TRAITS
	case TRAIT_BLUE: /* Draconic Blue */
		if (p_ptr->lev >= 5) { p_ptr->brand_elec = TRUE; csheet_boni[14].cb[10] |= CB11_BELEC; }
		if (p_ptr->lev >= 15) { p_ptr->sh_elec = TRUE; csheet_boni[14].cb[10] |= CB11_AELEC; }
		if (p_ptr->lev < 25) { p_ptr->resist_elec = TRUE; csheet_boni[14].cb[0] |= CB1_RELEC; }
		else { p_ptr->immune_elec = TRUE; csheet_boni[14].cb[1] |= CB2_IELEC; }
		break;
	case TRAIT_WHITE: /* Draconic White */
		p_ptr->suscep_fire = TRUE; csheet_boni[14].cb[0] |= CB1_SFIRE;
		if (p_ptr->lev >= 15) { p_ptr->sh_cold = TRUE; csheet_boni[14].cb[10] |= CB11_ACOLD; }
		if (p_ptr->lev < 25) { p_ptr->resist_cold = TRUE; csheet_boni[14].cb[0] |= CB1_RCOLD; }
		else { p_ptr->immune_cold = TRUE; csheet_boni[14].cb[0] |= CB1_ICOLD; }
		break;
	case TRAIT_RED: /* Draconic Red */
		p_ptr->suscep_cold = TRUE; csheet_boni[14].cb[0] |= CB1_SCOLD;
		if (p_ptr->lev < 25) { p_ptr->resist_fire = TRUE; csheet_boni[14].cb[0] |= CB1_RFIRE; }
		else { p_ptr->immune_fire = TRUE; csheet_boni[14].cb[0] |= CB1_IFIRE; }
		break;
	case TRAIT_BLACK: /* Draconic Black */
		if (p_ptr->lev < 25) { p_ptr->resist_acid = TRUE; csheet_boni[14].cb[1] |= CB2_RACID; }
		else { p_ptr->immune_acid = TRUE; csheet_boni[14].cb[1] |= CB2_IACID; }
		break;
	case TRAIT_GREEN: /* Draconic Green */
		if (p_ptr->lev < 25) { p_ptr->resist_pois = TRUE; csheet_boni[14].cb[1] |= CB2_RPOIS; }
		else { p_ptr->immune_poison = TRUE; csheet_boni[14].cb[1] |= CB2_IPOIS; }
		break;
	case TRAIT_MULTI: /* Draconic Multi-hued */
		if (p_ptr->lev >= 5) { p_ptr->resist_elec = TRUE; csheet_boni[14].cb[0] |= CB1_RELEC; }
		if (p_ptr->lev >= 10) { p_ptr->resist_cold = TRUE; csheet_boni[14].cb[0] |= CB1_RCOLD; }
		if (p_ptr->lev >= 15) { p_ptr->resist_fire = TRUE; csheet_boni[14].cb[0] |= CB1_RFIRE; }
		if (p_ptr->lev >= 20) { p_ptr->resist_acid = TRUE; csheet_boni[14].cb[1] |= CB2_RACID; }
		if (p_ptr->lev >= 25) { p_ptr->resist_pois = TRUE; csheet_boni[14].cb[1] |= CB2_RPOIS; }
		break;
	case TRAIT_BRONZE: /* Draconic Bronze */
		p_ptr->to_a += 4;
		p_ptr->dis_to_a += 4;
		if (p_ptr->lev >= 5) { p_ptr->resist_conf = TRUE; csheet_boni[14].cb[3] |= CB4_RCONF; }
		if (p_ptr->lev >= 10) { p_ptr->resist_elec = TRUE; csheet_boni[14].cb[0] |= CB1_RELEC; }
		if (p_ptr->lev >= 10) { p_ptr->free_act = TRUE; csheet_boni[14].cb[4] |= CB5_RPARA; }
		if (p_ptr->lev >= 20) { p_ptr->reflect = TRUE; csheet_boni[14].cb[6] |= CB7_RREFL; }
		break;
	case TRAIT_SILVER: /* Draconic Silver */
		p_ptr->to_a += 4;
		p_ptr->dis_to_a += 4;
		if (p_ptr->lev >= 5) { p_ptr->resist_cold = TRUE; csheet_boni[14].cb[0] |= CB1_RCOLD; }
		if (p_ptr->lev >= 10) { p_ptr->resist_acid = TRUE; csheet_boni[14].cb[1] |= CB2_RACID; }
		if (p_ptr->lev >= 15) { p_ptr->resist_pois = TRUE; csheet_boni[14].cb[1] |= CB2_RPOIS; }
		if (p_ptr->lev >= 20) { p_ptr->reflect = TRUE; csheet_boni[14].cb[6] |= CB7_RREFL; }
		break;
	case TRAIT_GOLD: /* Draconic Gold */
		p_ptr->to_a += 4;
		p_ptr->dis_to_a += 4;
		if (p_ptr->lev >= 5) { p_ptr->resist_fire = TRUE; csheet_boni[14].cb[0] |= CB1_RFIRE; }
		if (p_ptr->lev >= 10) { p_ptr->resist_acid = TRUE; csheet_boni[14].cb[1] |= CB2_RACID; }
		if (p_ptr->lev >= 15) { p_ptr->resist_sound = TRUE; csheet_boni[14].cb[2] |= CB3_RSOUN; }
		if (p_ptr->lev >= 20) { p_ptr->reflect = TRUE; csheet_boni[14].cb[6] |= CB7_RREFL; }
		break;
	case TRAIT_LAW: /* Draconic Law */
		if (p_ptr->lev >= 5) { p_ptr->resist_shard = TRUE; csheet_boni[14].cb[2] |= CB3_RSHRD; }
		if (p_ptr->lev >= 10) { p_ptr->free_act = TRUE; csheet_boni[14].cb[4] |= CB5_RPARA; }
		if (p_ptr->lev >= 15) { p_ptr->resist_sound = TRUE; csheet_boni[14].cb[2] |= CB3_RSOUN; }
		break;
	case TRAIT_CHAOS: /* Draconic Chaos */
		if (p_ptr->lev >= 5) { p_ptr->resist_conf = TRUE; csheet_boni[14].cb[3] |= CB4_RCONF; }
		if (p_ptr->lev >= 15) { p_ptr->resist_chaos = TRUE; csheet_boni[14].cb[3] |= CB4_RCHAO; }
		if (p_ptr->lev >= 20) { p_ptr->resist_disen = TRUE; csheet_boni[14].cb[3] |= CB4_RDISE; }
		break;
	case TRAIT_BALANCE: /* Draconic Balance */
		if (p_ptr->lev >= 10) { p_ptr->resist_disen = TRUE; csheet_boni[14].cb[3] |= CB4_RDISE; }
		if (p_ptr->lev >= 20) { p_ptr->resist_sound = TRUE; csheet_boni[14].cb[2] |= CB3_RSOUN; }
		break;
	case TRAIT_POWER: /* Draconic Power */
		p_ptr->resist_fear = TRUE; csheet_boni[14].cb[4] |= CB5_RFEAR;
		if (p_ptr->lev >= 5) { p_ptr->resist_blind = TRUE; csheet_boni[14].cb[1] |= CB2_RBLND; }
		if (p_ptr->lev >= 20) { p_ptr->reflect = TRUE; csheet_boni[14].cb[6] |= CB7_RREFL; }
		break;
#endif
	}

	if (p_ptr->pclass == CLASS_RANGER) {
		if (p_ptr->lev >= 8) { p_ptr->pass_trees = TRUE; csheet_boni[14].cb[12] |= CB13_XTREE; }
		if (p_ptr->lev >= 15) { p_ptr->can_swim = TRUE; csheet_boni[14].cb[12] |= CB13_XSWIM; }
	}

	if (p_ptr->pclass == CLASS_SHAMAN)
		if (p_ptr->lev >= 20) { p_ptr->see_inv = TRUE; csheet_boni[14].cb[4] |= CB5_RSINV; }

	if (p_ptr->pclass == CLASS_DRUID)
		if (p_ptr->lev >= 10) { p_ptr->pass_trees = TRUE; csheet_boni[14].cb[12] |= CB13_XTREE; }

	if (p_ptr->pclass == CLASS_MINDCRAFTER) {
		if (p_ptr->lev >= 20) { p_ptr->reduce_insanity = 2; csheet_boni[14].cb[4] |= CB5_XMIND; }
		else if (p_ptr->lev >= 10 && !p_ptr->reduce_insanity) { p_ptr->reduce_insanity = 1; csheet_boni[14].cb[3] |= CB4_RMIND; }
	}


	/* Check ability skills */
	if (get_skill(p_ptr, SKILL_CLIMB) >= 1) { p_ptr->climb = TRUE; csheet_boni[14].cb[5] |= CB6_RCLMB; }
	if (get_skill(p_ptr, SKILL_LEVITATE) >= 1) {
		p_ptr->levitate = TRUE; csheet_boni[14].cb[5] |= CB6_RLVTN;
		p_ptr->feather_fall = TRUE; csheet_boni[14].cb[5] |= CB6_RFFAL;
	}
	if (get_skill(p_ptr, SKILL_FREEACT) >= 1) { p_ptr->free_act = TRUE; csheet_boni[14].cb[4] |= CB5_RPARA; }
	if (get_skill(p_ptr, SKILL_RESCONF) >= 1) { p_ptr->resist_conf = TRUE; csheet_boni[14].cb[3] |= CB4_RCONF; }


	/* Compute antimagic */
	if (get_skill(p_ptr, SKILL_ANTIMAGIC)) {
#ifdef NEW_ANTIMAGIC_RATIO
		p_ptr->antimagic += get_skill_scale(p_ptr, SKILL_ANTIMAGIC, 50); csheet_boni[14].amfi += get_skill_scale(p_ptr, SKILL_ANTIMAGIC, 50);
		p_ptr->antimagic_dis += 1 + (get_skill(p_ptr, SKILL_ANTIMAGIC) / 10); /* was /11, but let's reward max skill! */
#else
		p_ptr->antimagic += get_skill_scale(p_ptr, SKILL_ANTIMAGIC, 30); csheet_boni[14].amfi += get_skill_scale(p_ptr, SKILL_ANTIMAGIC, 30);
		p_ptr->antimagic_dis += 1 + (get_skill(p_ptr, SKILL_ANTIMAGIC) / 10); /* was /11, but let's reward max skill! */
#endif
	}

	/* Ghost */
	if (p_ptr->ghost) {
		p_ptr->see_inv = TRUE; csheet_boni[14].cb[4] |= CB5_RSINV;
		/* p_ptr->resist_neth = TRUE;
		p_ptr->hold_life = TRUE; */
		p_ptr->free_act = TRUE; csheet_boni[14].cb[4] |= CB5_RPARA;
		p_ptr->see_infra += 3; csheet_boni[14].infr += 3;
		p_ptr->resist_dark = TRUE; csheet_boni[14].cb[2] |= CB3_RDARK;
		p_ptr->resist_blind = TRUE; csheet_boni[14].cb[1] |= CB2_RBLND;
		p_ptr->immune_poison = TRUE; csheet_boni[14].cb[1] |= CB2_IPOIS;
		p_ptr->resist_cold = TRUE; csheet_boni[14].cb[0] |= CB1_RCOLD;
		p_ptr->resist_fear = TRUE; csheet_boni[14].cb[4] |= CB5_RFEAR;
		p_ptr->resist_conf = TRUE; csheet_boni[14].cb[3] |= CB4_RCONF;
		p_ptr->no_cut = TRUE; csheet_boni[14].cb[12] |= CB13_XNCUT;
		if (!p_ptr->reduce_insanity) { p_ptr->reduce_insanity = 1; csheet_boni[14].cb[3] |= CB4_RMIND; }
		p_ptr->levitate = TRUE; csheet_boni[14].cb[5] |= CB6_RLVTN; /* redundant */
		p_ptr->feather_fall = TRUE; csheet_boni[14].cb[5] |= CB6_RFFAL;
		/*p_ptr->tim_wraith = 10000; redundant*/
		//p_ptr->invis += 5; */ /* No. */
	}

	/* Hack -- apply racial/class stat maxes */
	if (p_ptr->maximize) {
		/* Apply the racial modifiers */
		for (i = 0; i < 6; i++) {
			/* Modify the stats for "race" */
			/* yeek mimic no longer rocks too much */
			//if (!p_ptr->body_monster) p_ptr->stat_add[i] += (p_ptr->rp_ptr->r_adj[i]);
//done in calc_body_bonus()!			p_ptr->stat_add[i] += (p_ptr->rp_ptr->r_adj[i]) * 3 / (p_ptr->body_monster ? 4 : 3);
			p_ptr->stat_add[i] += p_ptr->rp_ptr->r_adj[i];
			p_ptr->stat_add[i] += p_ptr->cp_ptr->c_adj[i];
		}
	}

	/* Apply the racial modifiers */
	if (p_ptr->mode & MODE_HARD) {
		for (i = 0; i < 6; i++) {
			/* Modify the stats for "race" */
			p_ptr->stat_add[i]--;
		}
	}


	/* Hack -- the dungeon master gets +50 speed. */
	if (p_ptr->admin_dm) {
		p_ptr->pspeed += 50; //csheet_boni[14].spd += 50; //Don't show these for the admin/tester
		p_ptr->telepathy |= ESP_ALL;
		/* Flag all the ESPs for ESP_ALL */
		//csheet_boni[14].cb[7] |= (CB8_ESPID | CB8_EANIM | CB8_EORCS | CB8_ETROL | CB8_EGIAN);
		//csheet_boni[14].cb[8] |= (CB9_EDRGN | CB9_EDEMN | CB9_EUNDD);
		//csheet_boni[14].cb[9] |= (CB10_EEVIL | CB10_EDGRI | CB10_EGOOD | CB10_ENONL | CB10_EUNIQ);
	}

	/* Hack -- recalculate inventory weight and count */
	p_ptr->total_weight = 0;
	p_ptr->inven_cnt = 0;

	for (i = 0; i < INVEN_PACK; i++) {
		o_ptr = &p_ptr->inventory[i];

		/* Skip missing items */
		if (!o_ptr->k_idx) break;

		p_ptr->inven_cnt++;
		p_ptr->total_weight += o_ptr->weight * o_ptr->number;
	}

	/* Apply the bonus from Druidism */
#if 0 /* focus _shot_ = ranged only (old way) */
	p_ptr->to_h_ranged += p_ptr->focus_val;
	p_ptr->dis_to_h_ranged += p_ptr->focus_val;
#else /* focus: apply to both, melee and ranged */
	p_ptr->to_h += p_ptr->focus_val;
	p_ptr->dis_to_h += p_ptr->focus_val;
#endif

	/* Scan the usable inventory */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];
		k_ptr = &k_info[o_ptr->k_idx];
		pval = o_ptr->pval;

		/* Skip missing items */
		if (!o_ptr->k_idx) {
			csheet_boni[i-INVEN_WIELD].cb[12] |= CB13_XNOEQ; //Nothing equipped, grey out the table column
			continue;
		}

		/* Set item display info */
		csheet_boni[i-INVEN_WIELD].color = k_info[o_ptr->k_idx].d_attr;
		csheet_boni[i-INVEN_WIELD].symbol = k_info[o_ptr->k_idx].d_char;

		/* Special admin items */
		if (o_ptr->tval == TV_AMULET) {
			switch (o_ptr->sval) {
			case SV_AMULET_INVINCIBILITY:
				p_ptr->admin_invinc = TRUE;
				/* fall through */
			case SV_AMULET_INVULNERABILITY:
				p_ptr->admin_invuln = TRUE;
			}
		}

#ifdef EQUIPMENT_SET_BONUS
		if (o_ptr->name1 == ART_RANDART) {
			/* paranoia maybe? Make sure name4 field has been set: */
			randart_name(o_ptr, NULL, tmp_name);
			/* compare */
			for (j = 0; j < i - INVEN_WIELD; j++)
				if (equipment_set[j] == o_ptr->name4 + 1) {
					equipment_set_amount[j]++;
					break;
				} else if (!strcmp(equipment_set_name[j], tmp_name)) {
					equipment_set_amount[j]++;
					/* for faster randart handling in future loop passes (pft) */
					equipment_set[j] = o_ptr->name4 + 1;
					break;
				}
			if (j == i - INVEN_WIELD) {
				equipment_set[j] = o_ptr->name4 + 1;
				equipment_set_amount[j] = 1;
				/* and for true arts: */
				strcpy(equipment_set_name[j], tmp_name);
			}
		} else if (o_ptr->name1) {
			strcpy(tmp_name, a_name + a_info[o_ptr->name1].name);
			tmp_name_ptr = tmp_name;
			/* trim leading and terminating ' characters */
			if (tmp_name[0] == '\'') {
				tmp_name_ptr++;
				tmp_name[strlen(tmp_name_ptr)] = 0;
			/* trim leading "of " */
			} else if (tmp_name[0] == 'o' && tmp_name[1] == 'f' && tmp_name[2] == ' ')
				tmp_name_ptr += 3;
#if 1
			/* maybe: strip 'the' too */
			if (tmp_name_ptr[0] == 't' && tmp_name_ptr[1] == 'h' && tmp_name_ptr[2] == 'e' && tmp_name_ptr[3] == ' ')
				tmp_name_ptr += 4;
#endif
			/* compare */
			for (j = 0; j < i - INVEN_WIELD; j++)
				if (!strcmp(equipment_set_name[j], tmp_name_ptr)) {
					equipment_set_amount[j]++;
					break;
				}
			if (j == i - INVEN_WIELD) {
				strcpy(equipment_set_name[j], tmp_name_ptr);
				equipment_set_amount[j] = 1;
				/* enable (use -1 since true arts don't use randart codes) */
				equipment_set[j] = -1;
			}
		}
#endif

		p_ptr->total_weight += o_ptr->weight * o_ptr->number;

		/* Extract the item flags */
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		/* Note cursed/hidden status... */
		if ((f3 & TR3_CURSED) || (f3 & TR3_HEAVY_CURSE) || (f3 & TR3_PERMA_CURSE)) csheet_boni[i-INVEN_WIELD].cb[12] |= CB13_XCRSE;
		if (!object_fully_known_p(Ind, o_ptr) && !(o_ptr->tval == TV_SWORD && o_ptr->sval == SV_DARK_SWORD)) csheet_boni[i-INVEN_WIELD].cb[11] |= CB12_XHIDD;
		
		/* Not-burning light source does nothing, good or bad */
		if ((f4 & TR4_FUEL_LITE) && (o_ptr->timeout < 1)) continue;

		/* Anti-Cheeze (DWing in MH mode on heavy armoured warriors):
		   Dual-wielding won't apply the second weapon if encumbered */
		if (i == INVEN_ARM && o_ptr->tval != TV_SHIELD && rogue_heavy_armor(p_ptr)) continue;

		if (f4 & TR4_NEVER_BLOW) {
			if (is_ranged_weapon(o_ptr->tval)) never_blow_ranged = TRUE; //disable shooting attacks
			else if (is_melee_weapon(o_ptr->tval)) {
				switch (i) {
				case INVEN_WIELD:
					never_blow |= 0x2; //disable melee for this weapon
					break;
				case INVEN_ARM:
					never_blow |= 0x4; //disable melee for this weapon
					break;
				}
			} else {
				never_blow = 0x1; //completely disable melee
				p_ptr->num_blow = 0;
			}
		}

		/* MEGA ugly hack -- set spacetime distortion resistance */
		if (o_ptr->name1 == ART_ANCHOR) {
			p_ptr->resist_continuum = TRUE;
		}
#if 0
		/* another bad hack, for when Morgoth's crown gets its pval
		   reduced experimentally, to keep massive +IV (for Q-quests^^): */
		if (o_ptr->name1 == ART_MORGOTH) {
			p_ptr->see_infra += 50;
		}
#endif

		/* Hack -- first add any "base bonuses" of the item.  A new
		 * feature in MAngband 0.7.0 is that the magnitude of the
		 * base bonuses is stored in bpval instead of pval, making the
		 * magnitude of "base bonuses" and "ego bonuses" independent
		 * from each other.
		 * An example of an item that uses this independency is an
		 * Orcish Shield of the Avari that gives +1 to STR and +3 to
		 * CON. (base bonus from the shield +1 STR,CON, ego bonus from
		 * the Avari +2 CON).
		 * Of course, the proper fix would be to redesign the object
		 * type so that each of the ego bonuses has its own independent
		 * parameter.
		 */
		/* If we have any base bonuses to add, add them */
		if (k_ptr->flags1 & TR1_PVAL_MASK) {
			/* Affect stats */
			if (k_ptr->flags1 & TR1_STR) { p_ptr->stat_add[A_STR] += o_ptr->bpval; csheet_boni[i-INVEN_WIELD].pstr += o_ptr->bpval; }
			if (k_ptr->flags1 & TR1_INT) { p_ptr->stat_add[A_INT] += o_ptr->bpval; csheet_boni[i-INVEN_WIELD].pint += o_ptr->bpval; }
			if (k_ptr->flags1 & TR1_WIS) { p_ptr->stat_add[A_WIS] += o_ptr->bpval; csheet_boni[i-INVEN_WIELD].pwis += o_ptr->bpval; }
			if (k_ptr->flags1 & TR1_DEX) { p_ptr->stat_add[A_DEX] += o_ptr->bpval; csheet_boni[i-INVEN_WIELD].pdex += o_ptr->bpval; }
			if (k_ptr->flags1 & TR1_CON) { p_ptr->stat_add[A_CON] += o_ptr->bpval; csheet_boni[i-INVEN_WIELD].pcon += o_ptr->bpval; }
			if (k_ptr->flags1 & TR1_CHR) { p_ptr->stat_add[A_CHR] += o_ptr->bpval; csheet_boni[i-INVEN_WIELD].pchr += o_ptr->bpval; }

			/* Affect stealth */
			if (k_ptr->flags1 & TR1_STEALTH) { p_ptr->skill_stl += o_ptr->bpval; csheet_boni[i-INVEN_WIELD].slth += o_ptr->bpval; }
			/* Affect searching ability (factor of five) */
			if (k_ptr->flags1 & TR1_SEARCH) { p_ptr->skill_srh += (o_ptr->bpval * 5); csheet_boni[i-INVEN_WIELD].srch += o_ptr->bpval; }
			/* Affect searching frequency (factor of five) */
			if (k_ptr->flags1 & TR1_SEARCH) p_ptr->skill_fos += (o_ptr->bpval * 3);
			/* Affect infravision */
			if (k_ptr->flags1 & TR1_INFRA) { p_ptr->see_infra += o_ptr->bpval; csheet_boni[i-INVEN_WIELD].infr += o_ptr->bpval; }

			/* Affect digging (factor of 20) */
			if (k_ptr->flags1 & TR1_TUNNEL) { p_ptr->skill_dig += (o_ptr->bpval * 20); csheet_boni[i-INVEN_WIELD].dig += o_ptr->bpval; }

			/* Affect speed */
			if (k_ptr->flags1 & TR1_SPEED) { p_ptr->pspeed += o_ptr->bpval; csheet_boni[i-INVEN_WIELD].spd += o_ptr->bpval; }

			/* Affect blows */
			/* There are no known weapons so far that adds bpr intrinsically. Need this only for ring of EA atm. */
			if (k_ptr->flags1 & TR1_BLOWS) { p_ptr->extra_blows += o_ptr->bpval; csheet_boni[i-INVEN_WIELD].blow += o_ptr->bpval; }

			/* Affect spells */
			if (k_ptr->flags1 & TR1_SPELL) extra_spells += o_ptr->bpval;

			/* Affect mana capacity -- currently doesn't exist as base bonus */
			if (k_ptr->flags1 & TR1_MANA) {
				if ((f4 & TR4_SHOULD2H) &&
				    (p_ptr->inventory[INVEN_WIELD].k_idx && p_ptr->inventory[INVEN_ARM].k_idx))
					{ p_ptr->to_m += (o_ptr->bpval * 20) / 3; csheet_boni[i-INVEN_WIELD].mxmp += o_ptr->bpval; }
				else
					{ p_ptr->to_m += o_ptr->bpval * 10; csheet_boni[i-INVEN_WIELD].mxmp += o_ptr->bpval; }
			}

			/* Affect life capacity -- currently doesn't exist as base bonus */
			if ((k_ptr->flags1 & TR1_LIFE) &&
			    ((o_ptr->name1 != ART_RANDART) ||
			    (make_resf(p_ptr) & RESF_LIFE) ||
			    o_ptr->bpval < 0)) //(hack: negative LIFE boni do get applied from randarts even for non-eligible characters)
				{ p_ptr->to_l += o_ptr->bpval; csheet_boni[i-INVEN_WIELD].mxhp += o_ptr->bpval; }
		}

		if (k_ptr->flags5 & TR5_PVAL_MASK) {
			if (k_ptr->flags5 & TR5_DISARM) p_ptr->skill_dis += (o_ptr->bpval) * 5;
			if (k_ptr->flags5 & TR5_LUCK) { p_ptr->luck += o_ptr->bpval; csheet_boni[i-INVEN_WIELD].luck += o_ptr->bpval; }
			/* There are no known weapons so far that add to crit intrinsically. */
			if (k_ptr->flags5 & TR5_CRIT) { p_ptr->xtra_crit += o_ptr->bpval; csheet_boni[i-INVEN_WIELD].crit += o_ptr->bpval; }
		}

		/* Next, add our ego bonuses */
		/* Hack -- clear out any pval bonuses that are in the base item
		 * bonus but not the ego bonus so we don't add them twice.
		 */
#if 1
		//if (o_ptr->name2 && o_ptr->tval != TV_RING) // pls see apply_magic ;)
		if (o_ptr->name2) {
			artifact_type *a_ptr;

			a_ptr = ego_make(o_ptr);
			f1 &= ~(k_ptr->flags1 & TR1_PVAL_MASK & ~a_ptr->flags1);
			f5 &= ~(k_ptr->flags5 & TR5_PVAL_MASK & ~a_ptr->flags5);

			/* Hack: Stormbringer! */
			if (o_ptr->name2 == EGO_STORMBRINGER) {
				p_ptr->stormbringer = TRUE;
				break_cloaking(Ind, 0);
				break_shadow_running(Ind);
				if (cfg.use_pk_rules == PK_RULES_DECLARE) {
#ifndef KURZEL_PK
					p_ptr->pkill |= PKILL_KILLABLE;
					if (!(p_ptr->pkill & PKILL_KILLER) &&
							!(p_ptr->pkill & PKILL_SET))
						set_pkill(Ind, 50);
#else
					p_ptr->pkill |= PKILL_SET; //Flag ON
#endif
				}
			}
		}

		if (o_ptr->name1 == ART_RANDART) {
			artifact_type *a_ptr;

			a_ptr =	randart_make(o_ptr);
			f1 &= ~(k_ptr->flags1 & TR1_PVAL_MASK & ~a_ptr->flags1);
			f5 &= ~(k_ptr->flags5 & TR5_PVAL_MASK & ~a_ptr->flags5);
		}
#endif


		/* Affect stats */
		if (f1 & TR1_STR) { p_ptr->stat_add[A_STR] += pval; csheet_boni[i-INVEN_WIELD].pstr += pval; }
		if (f1 & TR1_INT) { p_ptr->stat_add[A_INT] += pval; csheet_boni[i-INVEN_WIELD].pint += pval; }
		if (f1 & TR1_WIS) { p_ptr->stat_add[A_WIS] += pval; csheet_boni[i-INVEN_WIELD].pwis += pval; }
		if (f1 & TR1_DEX) { p_ptr->stat_add[A_DEX] += pval; csheet_boni[i-INVEN_WIELD].pdex += pval; }
		if (f1 & TR1_CON) { p_ptr->stat_add[A_CON] += pval; csheet_boni[i-INVEN_WIELD].pcon += pval; }
		if (f1 & TR1_CHR) { p_ptr->stat_add[A_CHR] += pval; csheet_boni[i-INVEN_WIELD].pchr += pval; }

		/* Affect luck */
		if (f5 & (TR5_LUCK)) { p_ptr->luck += pval; csheet_boni[i-INVEN_WIELD].luck += pval; }

		/* Affect spell power */
		//if (f1 & (TR1_SPELL)) p_ptr->to_s += pval;

		/* Affect mana capacity */
		if (f1 & (TR1_MANA)) {
			if ((f4 & TR4_SHOULD2H) &&
			    (p_ptr->inventory[INVEN_WIELD].k_idx && p_ptr->inventory[INVEN_ARM].k_idx))
				{ p_ptr->to_m += (pval * 20) / 3; csheet_boni[i-INVEN_WIELD].mxmp += pval; }
			else
				{ p_ptr->to_m += pval * 10; csheet_boni[i-INVEN_WIELD].mxmp += pval; }
		}

		/* Affect life capacity */
		if ((f1 & (TR1_LIFE)) &&
		    (o_ptr->name1 != ART_RANDART ||
		    (make_resf(p_ptr) & RESF_LIFE) ||
		    o_ptr->pval < 0))
			{ p_ptr->to_l += o_ptr->pval; csheet_boni[i-INVEN_WIELD].mxhp += o_ptr->pval; }

		/* Affect stealth */
		if (f1 & TR1_STEALTH) { p_ptr->skill_stl += pval; csheet_boni[i-INVEN_WIELD].slth += pval; }
#ifdef ART_WITAN_STEALTH
		else if (o_ptr->name1 && o_ptr->tval == TV_BOOTS && o_ptr->sval == SV_PAIR_OF_WITAN_BOOTS)
			{ p_ptr->skill_stl += -2; csheet_boni[i-INVEN_WIELD].slth += -2; }
#endif
		/* Affect searching ability (factor of five) */
		if (f1 & TR1_SEARCH) { p_ptr->skill_srh += (pval * 5); csheet_boni[i-INVEN_WIELD].srch += pval; }
		/* Affect searching frequency (factor of five) */
		if (f1 & TR1_SEARCH) p_ptr->skill_fos += (pval * 3);
		/* Affect infravision */
		if (f1 & TR1_INFRA) { p_ptr->see_infra += pval; csheet_boni[i-INVEN_WIELD].infr += pval; }

		/* Affect digging (factor of 20) */
		if (f1 & TR1_TUNNEL) { p_ptr->skill_dig += (pval * 20); csheet_boni[i-INVEN_WIELD].dig += pval; }

		/* Affect speed */
		if (f1 & TR1_SPEED) { p_ptr->pspeed += pval; csheet_boni[i-INVEN_WIELD].spd += pval; }

		/* Affect spellss */
		//if (f1 & TR1_SPELL_SPEED) extra_spells += pval;
		if (f1 & TR1_SPELL) extra_spells += pval;

		/* Affect disarming (factor of 20) */
		if (f5 & (TR5_DISARM)) p_ptr->skill_dis += pval * 5;

		/* Hack -- Sensible */
		/* not yet implemented
		if (f5 & (TR5_SENS_FIRE)) p_ptr->suscep_fire = TRUE;
		if (f5 & (TR6_SENS_COLD)) p_ptr->suscep_cold = TRUE;
		if (f5 & (TR6_SENS_ELEC)) p_ptr->suscep_elec = TRUE;
		if (f5 & (TR6_SENS_ACID)) p_ptr->suscep_acid = TRUE;
		if (f5 & (TR6_SENS_POIS)) p_ptr->suscep_acid = TRUE; */

		/* Boost shots */
		//if (f3 & TR3_KNOWLEDGE) p_ptr->auto_id = TRUE;
		if (f4 & TR4_AUTO_ID) { p_ptr->auto_id = TRUE; csheet_boni[i-INVEN_WIELD].cb[6] |= CB7_RAUID; }

		/* Boost shots */
		if (f3 & TR3_XTRA_SHOTS) { extra_shots++; csheet_boni[i-INVEN_WIELD].shot++; }

		/* Make the 'useless' curses interesting >:) - C. Blue */
		if ((f3 & TR3_TY_CURSE) && (o_ptr->ident & ID_CURSED)) p_ptr->ty_curse = TRUE;
		if ((f4 & TR4_DG_CURSE) && (o_ptr->ident & ID_CURSED)) p_ptr->dg_curse = TRUE;

		/* Various flags */
		if (f3 & TR3_AGGRAVATE) {
			p_ptr->aggravate = TRUE; csheet_boni[i-INVEN_WIELD].cb[6] |= CB7_RAGGR;
			break_cloaking(Ind, 0);
		}
		if (f3 & TR3_TELEPORT) {
			p_ptr->teleport = TRUE; csheet_boni[i-INVEN_WIELD].cb[4] |= CB5_STELE;
#ifdef TOGGLE_TELE /* disabled since TELEPORT flag was removed from Thunderlord ego, the only real reason to have this */
			//inscription = (unsigned char *) quark_str(o_ptr->note);
			inscription = quark_str(p_ptr->inventory[i].note);
			/* check for a valid inscription */
			if ((inscription != NULL) && (!(o_ptr->ident & ID_CURSED))) {
				/* scan the inscription for .. */
				while (*inscription != '\0') {
					if (*inscription == '.') {
						inscription++;
						/* a valid .. has been located */
						if (*inscription == '.') {
							inscription++;
							p_ptr->teleport = FALSE; csheet_boni[i-INVEN_WIELD].cb[4] &= (~CB5_STELE);
							break;
						}
					}
					inscription++;
				}
			}
#endif
		}
		if (f3 & TR3_DRAIN_EXP) p_ptr->drain_exp++;
		if (f5 & (TR5_DRAIN_MANA)) { p_ptr->drain_mana++; csheet_boni[i-INVEN_WIELD].cb[5] |= CB6_SRGMP; }
		if (f5 & (TR5_DRAIN_HP)) { p_ptr->drain_life++; csheet_boni[i-INVEN_WIELD].cb[5] |= CB6_SRGHP; }
		if (f5 & (TR5_INVIS)) {
			j = (p_ptr->lev > 50 ? 50 : p_ptr->lev) * 4 / 5;
			/* better than invis from monster form we're using? */
			if (j > p_ptr->tim_invis_power) p_ptr->tim_invis_power = j;
			csheet_boni[i-INVEN_WIELD].cb[4] |= CB5_RINVS;
		}
		if (f3 & TR3_BLESSED) p_ptr->bless_blade = TRUE;
		if (f3 & TR3_XTRA_MIGHT) { p_ptr->xtra_might++; csheet_boni[i-INVEN_WIELD].migh++; }
		if (f3 & TR3_SLOW_DIGEST) { p_ptr->slow_digest = TRUE; csheet_boni[i-INVEN_WIELD].cb[6] |= CB7_RFOOD; }
		if (f3 & TR3_REGEN) { p_ptr->regenerate = TRUE; csheet_boni[i-INVEN_WIELD].cb[5] |= CB6_RRGHP; }
		if (f5 & TR5_RES_TIME) { p_ptr->resist_time = TRUE; csheet_boni[i-INVEN_WIELD].cb[3] |= CB4_RTIME; }
		if (f5 & TR5_RES_MANA) { p_ptr->resist_mana = TRUE; csheet_boni[i-INVEN_WIELD].cb[3] |= CB4_RMANA; }
		if (f5 & TR5_IM_POISON) { p_ptr->immune_poison = TRUE; csheet_boni[i-INVEN_WIELD].cb[1] |= CB2_IPOIS; }
		if (f5 & TR5_IM_WATER) { p_ptr->immune_water = TRUE; csheet_boni[i-INVEN_WIELD].cb[3] |= CB4_IWATR; }
		if (f5 & TR5_RES_WATER) { p_ptr->resist_water = TRUE; csheet_boni[i-INVEN_WIELD].cb[3] |= CB4_RWATR; }
		if (f5 & TR5_PASS_WATER) { p_ptr->can_swim = TRUE; csheet_boni[i-INVEN_WIELD].cb[12] |= CB13_XSWIM; }
		if (f5 & TR5_REGEN_MANA) { p_ptr->regen_mana = TRUE; csheet_boni[i-INVEN_WIELD].cb[6] |= CB7_RRGMP; }
		if (esp) {
			p_ptr->telepathy |= esp;
			/* Flag all the ESPs for ESP_ALL */
			if (esp & ESP_ALL) {
				csheet_boni[i-INVEN_WIELD].cb[7] |= (CB8_ESPID | CB8_EANIM | CB8_EORCS | CB8_ETROL | CB8_EGIAN);
				csheet_boni[i-INVEN_WIELD].cb[8] |= (CB9_EDRGN | CB9_EDEMN | CB9_EUNDD);
				csheet_boni[i-INVEN_WIELD].cb[9] |= (CB10_EEVIL | CB10_EDGRI | CB10_EGOOD | CB10_ENONL | CB10_EUNIQ);
			} else {
				if (esp & ESP_SPIDER) csheet_boni[i-INVEN_WIELD].cb[7] |= CB8_ESPID;
				if (esp & ESP_ANIMAL) csheet_boni[i-INVEN_WIELD].cb[7] |= CB8_EANIM;
				if (esp & ESP_ORC) csheet_boni[i-INVEN_WIELD].cb[7] |= CB8_EORCS;
				if (esp & ESP_TROLL) csheet_boni[i-INVEN_WIELD].cb[7] |= CB8_ETROL;
				if (esp & ESP_GIANT) csheet_boni[i-INVEN_WIELD].cb[7] |= CB8_EGIAN;
				if (esp & ESP_DRAGON) csheet_boni[i-INVEN_WIELD].cb[8] |= CB9_EDRGN;
				if (esp & ESP_DEMON) csheet_boni[i-INVEN_WIELD].cb[8] |= CB9_EDEMN;
				if (esp & ESP_UNDEAD) csheet_boni[i-INVEN_WIELD].cb[8] |= CB9_EUNDD;
				if (esp & ESP_EVIL) csheet_boni[i-INVEN_WIELD].cb[9] |= CB10_EEVIL;
				if (esp & ESP_DRAGONRIDER) csheet_boni[i-INVEN_WIELD].cb[9] |= CB10_EDGRI;
				if (esp & ESP_GOOD) csheet_boni[i-INVEN_WIELD].cb[9] |= CB10_EGOOD;
				if (esp & ESP_NONLIVING) csheet_boni[i-INVEN_WIELD].cb[9] |= CB10_ENONL;
				if (esp & ESP_UNIQUE) csheet_boni[i-INVEN_WIELD].cb[9] |= CB10_EUNIQ;
			}
		}
		//if (f3 & TR3_TELEPATHY) p_ptr->telepathy = TRUE;
		//if (f3 & TR3_LITE1) p_ptr->lite += 1;
		if (f3 & TR3_SEE_INVIS) { p_ptr->see_inv = TRUE; csheet_boni[i-INVEN_WIELD].cb[4] |= CB5_RSINV; }
		if (f3 & TR3_FEATHER) { p_ptr->feather_fall = TRUE; csheet_boni[i-INVEN_WIELD].cb[5] |= CB6_RFFAL; }
		if (f2 & TR2_FREE_ACT) { p_ptr->free_act = TRUE; csheet_boni[i-INVEN_WIELD].cb[4] |= CB5_RPARA; }
		if (f2 & TR2_HOLD_LIFE) { p_ptr->hold_life = TRUE; csheet_boni[i-INVEN_WIELD].cb[5] |= CB6_RLIFE; }

		/* Light(consider doing it on calc_torch) */
		//if (((f4 & TR4_FUEL_LITE) && (o_ptr->timeout > 0)) || (!(f4 & TR4_FUEL_LITE)))
		{
			j = 0;
			if (f3 & TR3_LITE1) j++;
			if (f4 & TR4_LITE2) j += 2;
			if (f4 & TR4_LITE3) j += 3;

			p_ptr->cur_lite += j;
			if (f5 & TR5_WHITE_LIGHT) lite_inc_white += j;
			else lite_inc_norm += j;
			if (j && !(f4 & TR4_FUEL_LITE)) p_ptr->lite = TRUE;
			
			/* Permanent lite gets the 'sustain' colour */
			csheet_boni[i-INVEN_WIELD].lite += j;
			if (!(f4 & TR4_FUEL_LITE)) csheet_boni[i-INVEN_WIELD].cb[12] |= CB13_XLITE;
		}

		if (p_ptr->prace == RACE_VAMPIRE) {
			/* powerful lights and anti-undead/evil items damage vampires */
			if (anti_undead(o_ptr)) p_ptr->drain_life++;
			/* then again, spectral weapons don't hurt them */
			if (o_ptr->name2 == EGO_SPECTRAL || o_ptr->name2b == EGO_SPECTRAL) p_ptr->drain_life--; /* hack: cancel out the life-drain applied by spectral'ity */
		}
#ifdef ENABLE_HELLKNIGHT
		if (p_ptr->pclass == CLASS_HELLKNIGHT) {
			/* powerful lights and anti-demon/evil items damage hell knights */
			if (anti_demon(o_ptr)) p_ptr->drain_life++;
		}
#endif

		/* Immunity flags */
		if (f2 & TR2_IM_FIRE) { p_ptr->immune_fire = TRUE; csheet_boni[i-INVEN_WIELD].cb[0] |= CB1_IFIRE; }
		if (f2 & TR2_IM_ACID) { p_ptr->immune_acid = TRUE; csheet_boni[i-INVEN_WIELD].cb[1] |= CB2_IACID; }
		if (f2 & TR2_IM_COLD) { p_ptr->immune_cold = TRUE; csheet_boni[i-INVEN_WIELD].cb[0] |= CB1_ICOLD; }
		if (f2 & TR2_IM_ELEC) { p_ptr->immune_elec = TRUE; csheet_boni[i-INVEN_WIELD].cb[1] |= CB2_IELEC; }

		if (f2 & TR2_REDUC_FIRE) p_ptr->reduc_fire += 5 * o_ptr->to_a;
		if (f2 & TR2_REDUC_COLD) p_ptr->reduc_cold += 5 * o_ptr->to_a;
		if (f2 & TR2_REDUC_ELEC) p_ptr->reduc_elec += 5 * o_ptr->to_a;
		if (f2 & TR2_REDUC_ACID) p_ptr->reduc_acid += 5 * o_ptr->to_a;

		/* Resistance flags */
		if (f2 & TR2_RES_ACID) { p_ptr->resist_acid = TRUE; csheet_boni[i-INVEN_WIELD].cb[1] |= CB2_RACID; }
		if (f2 & TR2_RES_ELEC) { p_ptr->resist_elec = TRUE; csheet_boni[i-INVEN_WIELD].cb[0] |= CB1_RELEC; }
		if (f2 & TR2_RES_FIRE) { p_ptr->resist_fire = TRUE; csheet_boni[i-INVEN_WIELD].cb[0] |= CB1_RFIRE; }
		if (f2 & TR2_RES_COLD) { p_ptr->resist_cold = TRUE; csheet_boni[i-INVEN_WIELD].cb[0] |= CB1_RCOLD; }
		if (f2 & TR2_RES_POIS) { p_ptr->resist_pois = TRUE; csheet_boni[i-INVEN_WIELD].cb[1] |= CB2_RPOIS; }
		if (f2 & TR2_RES_FEAR) { p_ptr->resist_fear = TRUE; csheet_boni[i-INVEN_WIELD].cb[4] |= CB5_RFEAR; }
		if (f2 & TR2_RES_CONF) { p_ptr->resist_conf = TRUE; csheet_boni[i-INVEN_WIELD].cb[3] |= CB4_RCONF; }
		if (f2 & TR2_RES_SOUND) { p_ptr->resist_sound = TRUE; csheet_boni[i-INVEN_WIELD].cb[2] |= CB3_RSOUN; }
		if (f2 & TR2_RES_LITE) { p_ptr->resist_lite = TRUE; csheet_boni[i-INVEN_WIELD].cb[2] |= CB3_RLITE; }
		if (f2 & TR2_RES_DARK) { p_ptr->resist_dark = TRUE; csheet_boni[i-INVEN_WIELD].cb[2] |= CB3_RDARK; }
		if (f2 & TR2_RES_CHAOS) { p_ptr->resist_chaos = TRUE; csheet_boni[i-INVEN_WIELD].cb[3] |= CB4_RCHAO; }
		if (f2 & TR2_RES_DISEN) { p_ptr->resist_disen = TRUE; csheet_boni[i-INVEN_WIELD].cb[3] |= CB4_RDISE; }
		if (f2 & TR2_RES_SHARDS) { p_ptr->resist_shard = TRUE; csheet_boni[i-INVEN_WIELD].cb[2] |= CB3_RSHRD; }
		if (f2 & TR2_RES_NEXUS) { p_ptr->resist_nexus = TRUE; csheet_boni[i-INVEN_WIELD].cb[2] |= CB3_RNEXU; }
		if (f5 & TR5_RES_TELE) { p_ptr->res_tele = TRUE; csheet_boni[i-INVEN_WIELD].cb[4] |= CB5_RTELE; }
		if (f2 & TR2_RES_BLIND) { p_ptr->resist_blind = TRUE; csheet_boni[i-INVEN_WIELD].cb[1] |= CB2_RBLND; }
		if (f2 & TR2_RES_NETHER) { p_ptr->resist_neth = TRUE; csheet_boni[i-INVEN_WIELD].cb[2] |= CB3_RNETH; }

		/* Sustain flags */
		if (f2 & TR2_SUST_STR) { p_ptr->sustain_str = TRUE; csheet_boni[i-INVEN_WIELD].cb[11] |= CB12_RSSTR; }
		if (f2 & TR2_SUST_INT) { p_ptr->sustain_int = TRUE; csheet_boni[i-INVEN_WIELD].cb[11] |= CB12_RSINT; }
		if (f2 & TR2_SUST_WIS) { p_ptr->sustain_wis = TRUE; csheet_boni[i-INVEN_WIELD].cb[11] |= CB12_RSWIS; }
		if (f2 & TR2_SUST_DEX) { p_ptr->sustain_dex = TRUE; csheet_boni[i-INVEN_WIELD].cb[11] |= CB12_RSDEX; }
		if (f2 & TR2_SUST_CON) { p_ptr->sustain_con = TRUE; csheet_boni[i-INVEN_WIELD].cb[11] |= CB12_RSCON; }
		if (f2 & TR2_SUST_CHR) { p_ptr->sustain_chr = TRUE; csheet_boni[i-INVEN_WIELD].cb[11] |= CB12_RSCHR; }

		/* PernA flags */
		if (f3 & (TR3_WRAITH)) {
			//p_ptr->wraith_form = TRUE;
			if (!(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) &&
			    !(p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC))) {
				//BAD!(recursion)	set_tim_wraith(Ind, 10000);
				p_ptr->tim_wraith = 10000; csheet_boni[i-INVEN_WIELD].cb[5] |= CB6_RWRTH;
			}
		}
		if (f4 & (TR4_LEVITATE)) {
			p_ptr->levitate = TRUE; csheet_boni[i-INVEN_WIELD].cb[5] |= CB6_RLVTN;
			p_ptr->feather_fall = TRUE; csheet_boni[i-INVEN_WIELD].cb[5] |= CB6_RFFAL;
		}
		if (f4 & (TR4_CLIMB)) {
			/* hack: climbing kit is made for humanoids, so it won't work for other forms! (compare Send_equip()!) */
			if (o_ptr->tval != TV_TOOL || o_ptr->sval != SV_TOOL_CLIMB || !p_ptr->body_monster)
				{ p_ptr->climb = TRUE; csheet_boni[i-INVEN_WIELD].cb[5] |= CB6_RCLMB; } /* item isn't a climbing kit or player is in normal @ form */
#if 0
			else if ((r_ptr->flags3 & (RF3_ORC | RF3_TROLL | RF3_DRAGONRIDER)) || (strchr("hkptyuVsLW", r_ptr->d_char)))
				{ p_ptr->climb = TRUE;  csheet_boni[i-INVEN_WIELD].cb[5] |= CB6_RCLMB; } /* only for normal-sized humanoids (well..trolls, would be unfair to half-trolls) */
#else
			else if (r_ptr->body_parts[BODY_FINGER] && r_ptr->body_parts[BODY_ARMS])
				{ p_ptr->climb = TRUE; csheet_boni[i-INVEN_WIELD].cb[5] |= CB6_RCLMB; } /* everyone with arms and fingers may use it. ok? */
#endif
		}
		/* The Ring of Phasing, exclusively */
		if (f4 & (TR4_IM_NETHER)) { p_ptr->immune_neth = TRUE; csheet_boni[i-INVEN_WIELD].cb[2] |= CB3_INETH; }
		if (f5 & (TR5_REFLECT)) {
			if (o_ptr->tval != TV_SHIELD) { p_ptr->reflect = TRUE; csheet_boni[i-INVEN_WIELD].cb[6] |= CB7_RREFL; }
			else may_reflect = TRUE;
		}
		if (f3 & (TR3_SH_FIRE)) { p_ptr->sh_fire = p_ptr->sh_fire_fix = TRUE; csheet_boni[i-INVEN_WIELD].cb[10] |= CB11_AFIRE; }
		if (f5 & (TR5_SH_COLD)) { p_ptr->sh_cold = p_ptr->sh_cold_fix = TRUE; csheet_boni[i-INVEN_WIELD].cb[10] |= CB11_ACOLD; }
		if (f3 & (TR3_SH_ELEC)) { p_ptr->sh_elec = p_ptr->sh_elec_fix = TRUE; csheet_boni[i-INVEN_WIELD].cb[10] |= CB11_AELEC; }
		if (f3 & (TR3_NO_MAGIC)) {
			p_ptr->anti_magic = TRUE; csheet_boni[i-INVEN_WIELD].cb[6] |= CB7_RAMSH;
			/* turn off all magic auras */
			if (p_ptr->aura[0]) toggle_aura(Ind, 0);
			if (p_ptr->aura[1]) toggle_aura(Ind, 1);
			if (p_ptr->aura[2]) toggle_aura(Ind, 2);
		}
		if (f3 & (TR3_NO_TELE)) { p_ptr->anti_tele = TRUE; csheet_boni[i-INVEN_WIELD].cb[4] |= CB5_ITELE; }

		/* Additional flags from PernAngband */

		if (f4 & (TR4_IM_NETHER)) p_ptr->immune_neth = TRUE;

		/* Limit use of disenchanted DarkSword for non-unbe */
		minus = o_ptr->to_h + o_ptr->to_d; // + pval;// + (o_ptr->to_a / 4);
		if (minus < 0) minus = 0;
		am_temp = 0;
		if (f4 & TR4_ANTIMAGIC_50) am_temp += 50;
		if (f4 & TR4_ANTIMAGIC_30) am_temp += 30;
		if (f4 & TR4_ANTIMAGIC_20) am_temp += 20;
		if (f4 & TR4_ANTIMAGIC_10) am_temp += 10;
		am_temp -= minus;
		/* Weapons may not give more than 50% AM */
		if (am_temp > 50) am_temp = 50;
#ifdef NEW_ANTIMAGIC_RATIO
		am_temp = (am_temp * 3) / 5;
#endif
		/* Choose item with biggest AM field we find */
		if (am_temp > 0) csheet_boni[i-INVEN_WIELD].amfi = am_temp; //Track individual item boni, knowing they don't stack
		if (am_temp > am_bonus) am_bonus = am_temp;

		if ((f4 & (TR4_BLACK_BREATH))
#ifdef VAMPIRES_BB_IMMUNE
		    && p_ptr->prace != RACE_VAMPIRE
#endif
		    )
			p_ptr->black_breath_tmp = TRUE;

		//if (f5 & (TR5_IMMOVABLE)) p_ptr->immovable = TRUE;

		/* note: TR1_VAMPIRIC isn't directly applied for melee
		   weapons here, but instead in the py_attack... functions
		   in cmd1.c. Reason is that dual-wielding might hit with
		   a vampiric or a non-vampiric weapon randomly - C. Blue */



		/* Modify the base armor class, which is always known (dis_) */
#ifdef USE_NEW_SHIELDS
		if (o_ptr->tval != TV_SHIELD) {
			p_ptr->ac += o_ptr->ac;
			p_ptr->dis_ac += o_ptr->ac;
		}
#else
		if (o_ptr->tval == TV_SHIELD) may_ac += o_ptr->ac;
		else {
			p_ptr->ac += o_ptr->ac;
			p_ptr->dis_ac += o_ptr->ac;
		}
#endif

#ifndef NEW_SHIELDS_NO_AC
		/* Apply the bonuses to armor class */
		if (o_ptr->tval == TV_SHIELD) may_to_a += o_ptr->to_a;
		else p_ptr->to_a += o_ptr->to_a;
#else
		if (o_ptr->tval != TV_SHIELD) p_ptr->to_a += o_ptr->to_a;
#endif

		/* Apply the mental bonuses to armor class, if known */
		if (object_known_p(Ind, o_ptr)) {
#ifndef NEW_SHIELDS_NO_AC
			if (o_ptr->tval == TV_SHIELD) may_dis_to_a += o_ptr->to_a;
			else p_ptr->dis_to_a += o_ptr->to_a;
#else
			if (o_ptr->tval != TV_SHIELD) p_ptr->dis_to_a += o_ptr->to_a;
#endif
		}

		/* Brands/slays */
		if (f1 & TR1_SLAY_ANIMAL) csheet_boni[i-INVEN_WIELD].cb[7] |= CB8_SANIM;
		if (f1 & TR1_SLAY_EVIL) csheet_boni[i-INVEN_WIELD].cb[9] |= CB10_SEVIL;
		if (f1 & TR1_SLAY_UNDEAD) csheet_boni[i-INVEN_WIELD].cb[9] |= CB10_SUNDD;
		if (f1 & TR1_SLAY_DEMON) csheet_boni[i-INVEN_WIELD].cb[8] |= CB9_SDEMN;
		if (f1 & TR1_SLAY_ORC) csheet_boni[i-INVEN_WIELD].cb[7] |= CB8_SORCS;
		if (f1 & TR1_SLAY_TROLL) csheet_boni[i-INVEN_WIELD].cb[7] |= CB8_STROL;
		if (f1 & TR1_SLAY_GIANT) csheet_boni[i-INVEN_WIELD].cb[8] |= CB9_SGIAN;
		if (f1 & TR1_SLAY_DRAGON) csheet_boni[i-INVEN_WIELD].cb[8] |= CB9_SDRGN;
		if (f1 & TR1_KILL_DRAGON) csheet_boni[i-INVEN_WIELD].cb[8] |= CB9_KDRGN;
		if (f1 & TR1_KILL_DEMON) csheet_boni[i-INVEN_WIELD].cb[8] |= CB9_KDEMN;
		if (f1 & TR1_KILL_UNDEAD) csheet_boni[i-INVEN_WIELD].cb[9] |= CB10_KUNDD;
		if (f1 & TR1_BRAND_POIS) csheet_boni[i-INVEN_WIELD].cb[10] |= CB11_BPOIS;
		if (f1 & TR1_BRAND_ACID) csheet_boni[i-INVEN_WIELD].cb[10] |= CB11_BACID;
		if (f1 & TR1_BRAND_ELEC) csheet_boni[i-INVEN_WIELD].cb[10] |= CB11_BELEC;
		if (f1 & TR1_BRAND_FIRE) csheet_boni[i-INVEN_WIELD].cb[10] |= CB11_BFIRE;
		if (f1 & TR1_BRAND_COLD) csheet_boni[i-INVEN_WIELD].cb[10] |= CB11_BCOLD;
		if (f5 & TR5_VORPAL) csheet_boni[i-INVEN_WIELD].cb[11] |= CB12_BVORP;

		/* Hack -- do not apply "weapon", "bow", "ammo", or "tool"  boni */
		if ((i == INVEN_WIELD) || (i == INVEN_ARM && o_ptr->tval != TV_SHIELD)) { 
			if (f1 & TR1_BLOWS) csheet_boni[i-INVEN_WIELD].blow += pval;
			if (f5 & TR5_CRIT) csheet_boni[i-INVEN_WIELD].crit += pval;
			if (f1 & TR1_VAMPIRIC) csheet_boni[i-INVEN_WIELD].cb[6] |= CB7_RVAMP;
			continue;
		}

		if (i == INVEN_AMMO || i == INVEN_BOW) {
			if (f1 & TR1_VAMPIRIC) { p_ptr->vampiric_ranged = WEAPON_VAMPIRIC_CHANCE_RANGED; csheet_boni[i-INVEN_WIELD].cb[6] |= CB7_RVAMP; }
			continue;
		}
		if (i == INVEN_TOOL) continue;

		if (f1 & TR1_BLOWS) { p_ptr->extra_blows += pval; csheet_boni[i-INVEN_WIELD].blow += pval; }
		if (f5 & TR5_CRIT) { p_ptr->xtra_crit += pval; csheet_boni[i-INVEN_WIELD].crit += pval; }

		/* Hack -- cause earthquakes */
		if (f5 & TR5_IMPACT) p_ptr->impact = TRUE; /* non-weapons/tools, so this could be gloves or jewelry, but those don't exist at this time */
		if (f5 & TR5_VORPAL) csheet_boni[i-INVEN_WIELD].cb[11] |= CB12_BVORP;

		/* Generally vampiric? */
		if (f1 & TR1_VAMPIRIC) {
			csheet_boni[i-INVEN_WIELD].cb[6] |= CB7_RVAMP; //Hack the MA 100% to show * somehow for the client? Or add another cb[x] for level 2 vamp? - Kurzel
			/* specialty: martial artists benefit with 100% in melee if on gloves! And must not even wear a shield. */
			if (i == INVEN_HANDS && !p_ptr->inventory[INVEN_WIELD].k_idx && !p_ptr->inventory[INVEN_ARM].k_idx) {
				if (p_ptr->vampiric_melee < 100) p_ptr->vampiric_melee = 100;
			} else {
				if (p_ptr->vampiric_melee < NON_WEAPON_VAMPIRIC_CHANCE) p_ptr->vampiric_melee = NON_WEAPON_VAMPIRIC_CHANCE;
			}
			if (p_ptr->vampiric_ranged < NON_WEAPON_VAMPIRIC_CHANCE_RANGED) p_ptr->vampiric_ranged = NON_WEAPON_VAMPIRIC_CHANCE_RANGED;
		}
		
		/* Hack MHDSM: */
		if (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_MULTIHUED) {
			if (o_ptr->xtra2 & 0x01) { p_ptr->immune_fire = TRUE; csheet_boni[i-INVEN_WIELD].cb[0] |= CB1_IFIRE; }
			if (o_ptr->xtra2 & 0x02) { p_ptr->immune_cold = TRUE; csheet_boni[i-INVEN_WIELD].cb[0] |= CB1_ICOLD; }
			if (o_ptr->xtra2 & 0x04) { p_ptr->immune_elec = TRUE; csheet_boni[i-INVEN_WIELD].cb[1] |= CB2_IELEC; }
			if (o_ptr->xtra2 & 0x08) { p_ptr->immune_acid = TRUE; csheet_boni[i-INVEN_WIELD].cb[1] |= CB2_IACID; }
			if (o_ptr->xtra2 & 0x10) { p_ptr->immune_poison = TRUE; csheet_boni[i-INVEN_WIELD].cb[1] |= CB2_IPOIS; }
		}

		/* Apply the bonuses to hit/damage */
		p_ptr->to_h += o_ptr->to_h;
		p_ptr->to_d += o_ptr->to_d;

		/* Apply the mental bonuses tp hit/damage, if known */
		if (object_known_p(Ind, o_ptr)) p_ptr->dis_to_h += o_ptr->to_h;
		if (object_known_p(Ind, o_ptr)) p_ptr->dis_to_d += o_ptr->to_d;

	}

	p_ptr->antimagic += am_bonus;
#ifdef NEW_ANTIMAGIC_RATIO
	p_ptr->antimagic_dis += (am_bonus / 9);
#else
	p_ptr->antimagic_dis += (am_bonus / 15);
#endif

	/* Hard/Hellish mode also gives mana penalty */
	if (p_ptr->mode & MODE_HARD) p_ptr->to_m = (p_ptr->to_m * 2) / 3;

	/* Check for temporary blessings */
	if (p_ptr->bless_temp_luck) {
		p_ptr->luck += p_ptr->bless_temp_luck_power;
		/* abuse '@ form' luck column for this for now */
		csheet_boni[14].luck += p_ptr->bless_temp_luck_power;
	}

#ifdef EQUIPMENT_SET_BONUS
	/* Don't stack boni, instead, the highest counts */
	for (i = 0; i < INVEN_TOTAL - INVEN_WIELD; i++) {
		if (!equipment_set[i]) continue;
		if (equipment_set_amount[i] > equipment_set_bonus)
			equipment_set_bonus = equipment_set_amount[i];
	}
	if (equipment_set_bonus >= 2) {
		/* Display the luck boni on each involved item */
		//for (i = 0; i < INVEN_TOTAL - INVEN_WIELD; i++) //Mh, this doesn't really always work, needs better tracking in the luck calculation code?
			//if (equipment_set_amount[i]) csheet_boni[i].luck += equipment_set_bonus;
		//equipment_set_bonus = (equipment_set_bonus * equipment_set_bonus) / 2;
		//equipment_set_bonus = (equipment_set_bonus - 1) * 4;
		//equipment_set_bonus = (equipment_set_bonus * equipment_set_bonus);
		equipment_set_bonus = (equipment_set_bonus * (equipment_set_bonus + 1)) / 2;
		/* just prevent numerical overflows in p_ptr->luck (paranoiaish) */
		if (equipment_set_bonus > 40) equipment_set_bonus = 40;
		p_ptr->luck += equipment_set_bonus;
		/* abuse '@ form' luck column for this for now */
		csheet_boni[14].luck += equipment_set_bonus;
	}
#endif

	if (p_ptr->antimagic > ANTIMAGIC_CAP) p_ptr->antimagic = ANTIMAGIC_CAP; /* AM cap */
	if (p_ptr->antimagic_dis > ANTIMAGIC_DIS_CAP) p_ptr->antimagic_dis = ANTIMAGIC_DIS_CAP; /* AM radius cap */
	if (p_ptr->luck < -10) p_ptr->luck = -10; /* luck caps at -10 */
	if (p_ptr->luck > 40) p_ptr->luck = 40; /* luck caps at 40 */

	old_sun_burn = p_ptr->sun_burn;
	p_ptr->sun_burn = FALSE;
	if ((p_ptr->prace == RACE_VAMPIRE ||
	    (p_ptr->body_monster && r_info[p_ptr->body_monster].d_char == 'V'))
	    && !p_ptr->ghost && !(p_ptr->global_event_temp & PEVF_INDOORS_00)
	    && !(l_ptr && (l_ptr->flags2 & LF2_INDOORS))
	    && !(in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_INDOORS))) {
		/* damage from sunlight */
		if (!p_ptr->wpos.wz && !night_surface && //!(zcave[p_ptr->py][p_ptr->px].info & CAVE_ICKY) &&
		    !p_ptr->resist_lite && (TOOL_EQUIPPED(p_ptr) != SV_TOOL_WRAPPING) &&
//		    !(p_ptr->inventory[INVEN_NECK].k_idx && p_ptr->inventory[INVEN_NECK].sval == SV_AMULET_HIGHLANDS2) &&
		    !(zcave[p_ptr->py][p_ptr->px].info & (CAVE_PROT | CAVE_ICKY)) && /* don't burn inside inn nor in the player's house */
		    !(f_info[zcave[p_ptr->py][p_ptr->px].feat].flags1 & FF1_PROTECTED) &&
		    zcave[p_ptr->py][p_ptr->px].feat != FEAT_SHOP) {
			p_ptr->sun_burn = TRUE;
			if (!old_sun_burn) msg_print(Ind, "\377RYou burn in the sunlight!");
			/* vampire bats can stay longer under the sunlight than actual vampire form */
			if (p_ptr->body_monster != RI_VAMPIRE_BAT) {
				i = (turn % DAY) / HOUR;
				i = 5 - ABS(i - (SUNRISE + (NIGHTFALL - SUNRISE) / 2)) / 2; /* for calculate day time distance to noon -> max burn!: 2..5*/
				if (p_ptr->total_winner) i = (i + 1) / 2;//1..3
				p_ptr->drain_life += i;
			} else p_ptr->drain_life++; /* currently, vampiric mist suffers same as normal vampire form (VAMPIRIC_MIST) */
		}
	}


	/* Apply temporary "stun" */
	if (p_ptr->stun > 50) {
		p_ptr->to_h /= 2;
		p_ptr->dis_to_h /= 2;
		p_ptr->to_d /= 2;
		p_ptr->dis_to_d /= 2;
	} else if (p_ptr->stun) {
		p_ptr->to_h = (p_ptr->to_h * 2) / 3;
		p_ptr->dis_to_h = (p_ptr->dis_to_h * 2) / 3;
		p_ptr->to_d = (p_ptr->to_d * 2) / 3;
		p_ptr->dis_to_d = (p_ptr->dis_to_h * 2) / 3;
	}

	/* Adrenaline effects */
	if (p_ptr->adrenaline) {
		int i;

		i = p_ptr->lev / 7;
		if (i > 3) i = 3;//was 5, untested hmm
		p_ptr->stat_add[A_CON] += i;
		p_ptr->stat_add[A_STR] += i;
		p_ptr->stat_add[A_DEX] += (i + 1) / 2;
		p_ptr->to_h += 12;
		p_ptr->dis_to_h += 12;
		if (p_ptr->adrenaline & 1) {
			p_ptr->to_d += 8;
			p_ptr->dis_to_d += 8;
		}
		if (p_ptr->adrenaline & 2) p_ptr->extra_blows++;
		p_ptr->to_a -= 20;
		p_ptr->dis_to_a -= 20;
	}

	/* At least +1, max. +3 */
	if (p_ptr->zeal) p_ptr->extra_blows += p_ptr->zeal_power / 10 > 3 ? 3 : (p_ptr->zeal_power / 10 < 1 ? 1 : p_ptr->zeal_power / 10);
	else if (p_ptr->mindboost && p_ptr->mindboost_power >= 65) p_ptr->extra_blows++;

	if (p_ptr->mindboost) p_ptr->skill_sav += p_ptr->mindboost_power / 5;

#ifdef ENABLE_OCCULT
	if (p_ptr->temp_savingthrow) p_ptr->skill_sav = 95;
#endif

	/* Temporary "Levitation" */
	if (p_ptr->tim_ffall || p_ptr->tim_lev) p_ptr->feather_fall = TRUE;
	if (p_ptr->tim_lev) p_ptr->levitate = TRUE;

	/* Temp ESP */
	if (p_ptr->tim_esp) {
		//p_ptr->telepathy = TRUE;
		p_ptr->telepathy |= ESP_ALL;
	}

	/* Temporary invisibility */
	if (p_ptr->tim_invis_power > p_ptr->tim_invis_power2)
		p_ptr->invis = p_ptr->tim_invis_power;
	else
		p_ptr->invis = p_ptr->tim_invis_power2;

	/* Temporary deflection */
	if (p_ptr->tim_deflect) p_ptr->reflect = TRUE;

	/* Temporary "Hero" */
	if (p_ptr->hero || (p_ptr->mindboost && p_ptr->mindboost_power >= 5)) {
		p_ptr->to_h += 12;
		p_ptr->dis_to_h += 12;
	}

	/* Temporary "Berserk"/"Berserk Strength" */
	if (p_ptr->shero) {
		p_ptr->to_a -= 10;
		p_ptr->dis_to_a -= 10;
		/* may greatly increase +dam and +bpr, also helps bashing doors open and tunnelling */
		p_ptr->stat_add[A_STR] += 10;
	}

	/* Temporary "Fury" */
	if (p_ptr->fury) {
		p_ptr->to_h -= 10;
		p_ptr->dis_to_h -= 10;
		p_ptr->to_d += 20;
		p_ptr->dis_to_d += 20;
		p_ptr->pspeed += 5;
		p_ptr->to_a -= 30;
		p_ptr->dis_to_a -= 30;
	}

	/* Temporary "fast" */
	if (p_ptr->fast) p_ptr->pspeed += p_ptr->fast_mod;

	/* Temporary "slow" */
	if (p_ptr->slow) p_ptr->pspeed -= 10;

	/* Temporary see invisible */
	if (p_ptr->tim_invis) p_ptr->see_inv = TRUE;

	/* Temporary infravision boost */
	if (p_ptr->tim_infra) {
		//p_ptr->see_infra++;
		p_ptr->see_infra += 5;
	}

	/* Hack -- Res Chaos -> Res Conf */
	if (p_ptr->resist_chaos) p_ptr->resist_conf = TRUE;

	/* Heart is boldened */
	if (p_ptr->res_fear_temp || p_ptr->hero || p_ptr->shero ||
	    p_ptr->fury || p_ptr->berserk || p_ptr->mindboost)
		p_ptr->resist_fear = TRUE;


	/* Hack -- Telepathy Change */
	if (p_ptr->telepathy != old_telepathy) p_ptr->update |= (PU_MONSTERS);

	/* Hack -- See Invis Change */
	if (p_ptr->see_inv != old_see_inv) p_ptr->update |= (PU_MONSTERS);

	/* Temporary space-time anchor */
	if (p_ptr->st_anchor) {
		p_ptr->anti_tele = TRUE;
		p_ptr->resist_continuum = TRUE;
	}


	/* Bloating slows the player down (a little) */
	if (p_ptr->food >= PY_FOOD_MAX) p_ptr->pspeed -= 10;

	/* Searching slows the player down */
	/* -APD- adding "stealth mode" for rogues... will probably need to tweek this */
	/* XXX this can be out of place; maybe better done
	 * after skill bonuses are added?	- Jir - */
	if (p_ptr->searching) {
		int stealth = get_skill(p_ptr, SKILL_STEALTH);
		int sneakiness = get_skill(p_ptr, SKILL_SNEAKINESS);
		p_ptr->pspeed -= 10 - sneakiness / 7;
		csheet_boni[14].spd -= 10 - sneakiness / 7;
#if 1
		if (stealth >= 10) {
			p_ptr->skill_stl = p_ptr->skill_stl + stealth / 10;
		}
#endif
		if (sneakiness >= 0) {
			/* prevent TLs from getting glitched abysmal stealth here.. */
			if (p_ptr->skill_srh >= 0) p_ptr->skill_srh = (p_ptr->skill_srh * (sneakiness + 50)) / 50;
			//p_ptr->skill_srh = p_ptr->skill_srh + sneakiness / 3;
			//p_ptr->skill_fos = p_ptr->skill_fos * sneakiness / 10;
		}
	}

	if (p_ptr->cloaked) {
#if 0 /* for now, light is simply dimmed in cloaking mode, instead of breaking it. */
	/*  might need to give rogues a special ability to see in the dark like vampires. or not, pft. - C. Blue */
	/* cannot be cloaked if wielding a (real, not vampire vision) light source */
		if (p_ptr->cur_lite) break_cloaking(Ind, 0);
#else
		if (p_ptr->cur_lite) p_ptr->cur_lite = 1; /* dim it */
#endif
	}
	if (p_ptr->cloaked == 1) {
		p_ptr->pspeed -= 10 - get_skill_scale(p_ptr, SKILL_SNEAKINESS, 3);
		csheet_boni[14].spd -= 10 - get_skill_scale(p_ptr, SKILL_SNEAKINESS, 3);
		p_ptr->skill_stl = p_ptr->skill_stl + 25;
		csheet_boni[14].slth += 25;
		p_ptr->skill_srh = p_ptr->skill_srh + 10;
		csheet_boni[14].srch += 10;
	}

	if (p_ptr->shadow_running) { p_ptr->pspeed += 10; csheet_boni[14].spd += 10; }

	if (p_ptr->sh_fire_tim) p_ptr->sh_fire = TRUE;
	if (p_ptr->sh_cold_tim) p_ptr->sh_cold = TRUE;
	if (p_ptr->sh_elec_tim) p_ptr->sh_elec = TRUE;

	/* Bonus from +LIFE items (should be weapons only -- not anymore, Bladeturner / Randarts etc.!).
	   Also, cap it at +3 (boomerang + weapon could result in +6) (Boomerangs can't have +LIFE anymore) */
	if (!is_admin(p_ptr) && p_ptr->to_l > 3) p_ptr->to_l = 3;


	/* Calculate stats */
	for (i = 0; i < 6; i++) {
		int top, use, ind;

		/* Extract the new "stat_use" value for the stat */
		top = modify_stat_value(p_ptr->stat_max[i], p_ptr->stat_add[i]);

		/* Notice changes */
		if (p_ptr->stat_top[i] != top) {
			/* Save the new value */
			p_ptr->stat_top[i] = top;

			/* Redisplay the stats later */
			p_ptr->redraw |= (PR_STATS);
			/* Window stuff */
			p_ptr->window |= (PW_PLAYER);
		}


		/* Extract the new "stat_use" value for the stat */
		use = modify_stat_value(p_ptr->stat_cur[i], p_ptr->stat_add[i]);

		/* Notice changes */
		if (p_ptr->stat_use[i] != use) {
			/* Save the new value */
			p_ptr->stat_use[i] = use;

			/* Redisplay the stats later */
			p_ptr->redraw |= (PR_STATS);
			/* Window stuff */
			p_ptr->window |= (PW_PLAYER);
		}


		/* Values: 3, 4, ..., 17 */
		if (use <= 18) ind = (use - 3);
		/* Ranges: 18/00-18/09, ..., 18/210-18/219 */
		else if (use <= 18 + 219) ind = (15 + (use - 18) / 10);
		/* Range: 18/220+ */
		else ind = (37);

		/* Notice changes */
		if (p_ptr->stat_ind[i] != ind) {
			/* Save the new index */
			p_ptr->stat_ind[i] = ind;

			/* Change in CON affects Hitpoints */
			if (i == A_CON) p_ptr->update |= (PU_HP);

			/* Changes that may affect Mana/Spells (dex for runemaster, chr for mindcrafter) */
			else if (i == A_INT || i == A_WIS || i == A_DEX || i == A_CHR)
				p_ptr->update |= (PU_MANA);

			/* Window stuff */
			p_ptr->window |= (PW_PLAYER);
		}
	}


	/* Assume player not encumbered by armor */
	p_ptr->cumber_armor = FALSE;
	/* Heavy armor penalizes to-hit and sneakiness speed */
	if (((armour_weight(p_ptr) - (100 + get_skill_scale(p_ptr, SKILL_COMBAT, 300)
	    + adj_str_hold[p_ptr->stat_ind[A_STR]] * 6)) / 10) > 0) {
		/* Encumbered */
		p_ptr->cumber_armor = TRUE;
	}
	/* Take note when "armor state" changes */
	if (p_ptr->old_cumber_armor != p_ptr->cumber_armor) {
		/* Message */
		if (p_ptr->cumber_armor)
			msg_print(Ind, "\377oThe weight of your armour encumbers your movement.");
		else
			msg_print(Ind, "\377gYou feel able to move more freely.");

		/* Save it */
		p_ptr->old_cumber_armor = p_ptr->cumber_armor;
	}


	p_ptr->rogue_heavyarmor = rogue_heavy_armor(p_ptr);
	if (p_ptr->old_rogue_heavyarmor != p_ptr->rogue_heavyarmor) {
		/* Don't kill warnings by inspecting weapons/armour in stores! */
		if (!suppress_boni) {
			if (p_ptr->rogue_heavyarmor) {
				msg_print(Ind, "\377RThe weight of your armour strains your flexibility and awareness.");
				break_cloaking(Ind, 0);
				break_shadow_running(Ind);
				if (!p_ptr->warning_dual && p_ptr->dual_wield && p_ptr->pclass != CLASS_ROGUE) {
					p_ptr->warning_dual = 1;
					msg_print(Ind, "\374\377yHINT: You cannot dual-wield effectively if your armour is too heavy!");
					msg_print(Ind, "\374\377y      If you don't want to use lighter armour, consider using one weapon");
					msg_print(Ind, "\374\377y      and a shield, or (if your STRength is VERY high) a two-handed weapon.");
					s_printf("warning_dual: %s\n", p_ptr->name);
				}
			}
			/* do we still wear armour, and are still rogue or dual-wielding? */
			else if (armour_weight(p_ptr)) {
				if (p_ptr->pclass == CLASS_ROGUE || p_ptr->dual_wield) {
					msg_print(Ind, "\377gYour armour is comfortable and flexible.");
				} else { /* we took off a weapon, not armour.. */
					msg_print(Ind, "\377gYou feel comfortable again.");
				}
			}
			/* if not, give a slightly different msg */
			else if (p_ptr->pclass == CLASS_ROGUE || p_ptr->dual_wield) {
				msg_print(Ind, "\377gYou feel comfortable and flexible again.");
			} else { /* we took off a weapon, not armour.. */
				msg_print(Ind, "\377gYou feel comfortable again.");
			}
		}
		p_ptr->old_rogue_heavyarmor = p_ptr->rogue_heavyarmor;

		/* for 'greying out' the secondary weapon hack: */
		p_ptr->inventory[INVEN_ARM].changed = !p_ptr->inventory[INVEN_ARM].changed;
		p_ptr->window |= PW_EQUIP;
	}

	if (p_ptr->stormbringer && !suppress_boni) {
		if (p_ptr->cloaked) {
			msg_print(Ind, "\377yYou cannot remain cloaked effectively while wielding the stormbringer!");
			break_cloaking(Ind, 0);
		}
		if (p_ptr->shadow_running) {
			msg_print(Ind, "\377yThe stormbringer hinders effective shadow running!");
			break_shadow_running(Ind);
		}
	}
	if (p_ptr->aggravate && !suppress_boni) {
		if (p_ptr->cloaked) {
			msg_print(Ind, "\377yYou cannot remain cloaked effectively while using aggravating items!");
			break_cloaking(Ind, 0);
		}
	}
	if (p_ptr->inventory[INVEN_WIELD].k_idx &&
	    (k_info[p_ptr->inventory[INVEN_WIELD].k_idx].flags4 & (TR4_MUST2H | TR4_SHOULD2H))
	    && !suppress_boni) {
		if (p_ptr->cloaked && !instakills(Ind)) {
			msg_print(Ind, "\377yYour weapon is too large to remain cloaked effectively.");
			break_cloaking(Ind, 0);
		}
		if (p_ptr->shadow_running && !instakills(Ind)) {
			msg_print(Ind, "\377yYour weapon is too large for effective shadow running.");
			break_shadow_running(Ind);
		}
	}
	if (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD
	    && !suppress_boni) {
		if (p_ptr->cloaked) {
			msg_print(Ind, "\377yYou cannot remain cloaked effectively while wielding a shield.");
			break_cloaking(Ind, 0);
		}
		if (p_ptr->shadow_running) {
			msg_print(Ind, "\377yYour shield hinders effective shadow running.");
			break_shadow_running(Ind);
		}
	}

	p_ptr->monk_heavyarmor = monk_heavy_armor(p_ptr);
	if (p_ptr->old_monk_heavyarmor != p_ptr->monk_heavyarmor) {
		if (p_ptr->monk_heavyarmor)
			msg_print(Ind, "\377oThe weight of your armour strains your martial arts performance.");
		else
			msg_print(Ind, "\377gYour armour is comfortable for using martial arts.");
		p_ptr->old_monk_heavyarmor = p_ptr->monk_heavyarmor;
	}
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS) && !monk_heavy_armor(p_ptr)) {
		long int k = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 5), w = 0;

		if (k) {
			/* Extract the current weight (in tenth pounds) */
			w = p_ptr->total_weight;

			/* Extract the "weight limit" (in tenth pounds) */
			i = weight_limit(Ind) + 500;

			/* XXX XXX XXX Apply "encumbrance" from weight */
			if (w > i / 5) k -= ((w - (i / 5)) / (i / 10));

			/* Assume unencumbered */
			p_ptr->cumber_weight = FALSE;

			if (k > 0) {
				/* Feather Falling if unencumbered at level 10 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 9)
					{ p_ptr->feather_fall = TRUE; csheet_boni[14].cb[5] |= CB6_RFFAL; }

				/* Fear Resistance if unencumbered at level 15 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 14)
					{ p_ptr->resist_fear = TRUE; csheet_boni[14].cb[4] |= CB5_RFEAR; }

				/* Confusion Resistance if unencumbered at level 20 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 19)
					{ p_ptr->resist_conf = TRUE; csheet_boni[14].cb[3] |= CB4_RCONF; }

				/* Free action if unencumbered at level 25 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 24)
					{ p_ptr->free_act = TRUE; csheet_boni[14].cb[4] |= CB5_RPARA; }

				/* Swimming if unencumbered at level 30 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 29)
					{ p_ptr->can_swim = TRUE; csheet_boni[14].cb[12] |= CB13_XSWIM; }

				/* Climbing if unencumbered at level 40 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 39)
					{ p_ptr->climb = TRUE; csheet_boni[14].cb[5] |= CB6_RCLMB; }

				/* Levitating if unencumbered at level 50 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 49) {
					p_ptr->levitate = TRUE; csheet_boni[14].cb[5] |= CB6_RLVTN;
					p_ptr->feather_fall = TRUE; csheet_boni[14].cb[5] |= CB6_RFFAL;
				}

				w = 0;
				if (p_ptr->inventory[INVEN_ARM].k_idx) w += k_info[p_ptr->inventory[INVEN_ARM].k_idx].weight;
				if (p_ptr->inventory[INVEN_BOW].k_idx) w += k_info[p_ptr->inventory[INVEN_BOW].k_idx].weight;
				if (p_ptr->inventory[INVEN_WIELD].k_idx) w += k_info[p_ptr->inventory[INVEN_WIELD].k_idx].weight;
				if (w < 150) {
					/* give a speed bonus */
					p_ptr->pspeed += k; csheet_boni[14].spd += k;

					/* give a stealth bonus */
					p_ptr->skill_stl += k; csheet_boni[14].slth += k;
				}
			}
			else p_ptr->cumber_weight = TRUE;

			if (p_ptr->old_cumber_weight != p_ptr->cumber_weight) {
				if (p_ptr->cumber_weight)
					msg_print(Ind, "\377RYou can't move freely due to your backpack weight.");
				else
					msg_print(Ind, "\377gYour backpack is very comfortable to wear.");
				p_ptr->old_cumber_weight = p_ptr->cumber_weight;
			}
		}

		/* Monks get extra ac for wearing very light or no armour at all */
#ifndef ENABLE_MA_BOOMERANG
		if (!p_ptr->inventory[INVEN_BOW].k_idx &&
#else
		if (p_ptr->inventory[INVEN_BOW].tval != TV_BOW &&
#endif
		    !p_ptr->inventory[INVEN_WIELD].k_idx &&
		    (!p_ptr->inventory[INVEN_ARM].k_idx || p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD) && /* for dual-wielders */
		    !p_ptr->cumber_weight) {
			int marts = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 60);
			int martsbonus, martsweight, martscapacity;
			
			martsbonus = (marts * 3) / 2 * MARTIAL_ARTS_AC_ADJUST / 100;
			if (!(p_ptr->inventory[INVEN_BODY].k_idx)) martsweight = 0;
			else martsweight = p_ptr->inventory[INVEN_BODY].weight;
			martscapacity = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 80) + 20;//(90/100/110) wire fleece/hard leather armour = 100; rhino hide/hard studded leather = 110
			if (martsweight <= 20) { //allow most basic armour so we don't have to be naked ~~
				p_ptr->to_a += martsbonus;
				p_ptr->dis_to_a += martsbonus;
			} else if (martsweight <= martscapacity) {
				martsbonus /= 2;
				p_ptr->to_a += (martsbonus * (martscapacity - martsweight / 3) / (martscapacity + 40));
				p_ptr->dis_to_a += (martsbonus * (martscapacity - martsweight / 3) / (martscapacity + 40));
			}

			martsbonus = ((marts - 13) / 3) * MARTIAL_ARTS_AC_ADJUST / 100;
			martsweight = p_ptr->inventory[INVEN_OUTER].weight;
			martscapacity = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 40);
			if (!(p_ptr->inventory[INVEN_OUTER].k_idx) && (marts > 15)) {
				p_ptr->to_a += martsbonus;
				p_ptr->dis_to_a += martsbonus;
			} else if ((martsweight <= martscapacity) && (marts > 15)) {
				martsbonus /= 2;
				p_ptr->to_a += (martsbonus * (martscapacity - martsweight / 3) / (martscapacity + 15));
				p_ptr->dis_to_a += (martsbonus * (martscapacity - martsweight / 3) / (martscapacity + 15));
			}

			martsbonus = ((marts - 8) / 3) * MARTIAL_ARTS_AC_ADJUST / 100;
			martsweight = p_ptr->inventory[INVEN_ARM].weight;
			martscapacity = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 30);
			if ((!p_ptr->inventory[INVEN_ARM].k_idx ||
			    p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) && (marts > 10)) { /* for dual-wielders */
				p_ptr->to_a += martsbonus;
				p_ptr->dis_to_a += martsbonus;
			} else if ((martsweight <= martscapacity) && (marts > 10)) {
				martsbonus /= 2;
				p_ptr->to_a += (martsbonus * (martscapacity - martsweight) / (martscapacity + 0));
				p_ptr->dis_to_a += (martsbonus * (martscapacity - martsweight) / (martscapacity + 0));
			}

			martsbonus = (marts - 2) / 3 * MARTIAL_ARTS_AC_ADJUST / 100;
			martsweight = p_ptr->inventory[INVEN_HEAD].weight;
			martscapacity = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 50);
			if (!(p_ptr->inventory[INVEN_HEAD].k_idx)&& (marts > 4)) {
				p_ptr->to_a += martsbonus;
				p_ptr->dis_to_a += martsbonus;
			} else if ((martsweight <= martscapacity) && (marts > 4)) {
				martsbonus /= 2;
				p_ptr->to_a += (martsbonus * (martscapacity - martsweight / 3) / (martscapacity + 20));
				p_ptr->dis_to_a += (martsbonus * (martscapacity - martsweight / 3) / (martscapacity + 20));
			}

			martsbonus = (marts / 2) * MARTIAL_ARTS_AC_ADJUST / 100;
			martsweight = p_ptr->inventory[INVEN_HANDS].weight;
			martscapacity = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 20);
			if (!(p_ptr->inventory[INVEN_HANDS].k_idx)) {
				p_ptr->to_a += martsbonus;
				p_ptr->dis_to_a += martsbonus;
			} else if (martsweight <= martscapacity) {
				martsbonus /= 2;
				p_ptr->to_a += (martsbonus * (martscapacity - martsweight / 2) / (martscapacity + 10));
				p_ptr->dis_to_a += (martsbonus * (martscapacity - martsweight / 2) / (martscapacity + 10));
			}

			martsbonus = (marts / 3) * MARTIAL_ARTS_AC_ADJUST / 100;
			martsweight = p_ptr->inventory[INVEN_FEET].weight;
			martscapacity = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 45);
			if (!(p_ptr->inventory[INVEN_FEET].k_idx)) {
				p_ptr->to_a += martsbonus;
				p_ptr->dis_to_a += martsbonus;
			} else if (martsweight <= martscapacity) {
				martsbonus /= 2;
				p_ptr->to_a += (martsbonus * (martscapacity - martsweight / 3) / (martscapacity + 20));
				p_ptr->dis_to_a += (martsbonus * (martscapacity - martsweight / 3) / (martscapacity + 20));
			}
		}
	}

	/* Actual Modifier Bonuses (Un-inflate stat bonuses) */
	p_ptr->to_a += ((int)(adj_dex_ta[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->to_d_melee += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
#if 1 /* addition */
	p_ptr->to_h_melee += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
#else /* multiplication (percent) -- TODO FIRST!!: would need to be before all penalties but after marts/etc +hit boni! */
	p_ptr->to_h_melee = ((int)((p_ptr->to_h_melee * adj_dex_th_mul[p_ptr->stat_ind[A_DEX]]) / 100));
#endif
	//p_ptr->to_d += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
	//p_ptr->to_h += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
	//p_ptr->to_h += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);

	/* Displayed Modifier Bonuses (Un-inflate stat bonuses) */
	p_ptr->dis_to_a += ((int)(adj_dex_ta[p_ptr->stat_ind[A_DEX]]) - 128);
	//p_ptr->dis_to_d += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
	//p_ptr->dis_to_h += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
	//p_ptr->dis_to_h += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);

	/* Modify ranged weapon boni. DEX now very important for to_hit */
#if 1 /* addition */
	p_ptr->to_h_ranged += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
#else /* multiplication (percent) -- TODO FIRST!!: would need to be before all penalties but after marts/etc +hit boni! */
	if (p_ptr->to_h_ranged > 0) p_ptr->to_h_ranged = ((int)((p_ptr->to_h_ranged * adj_dex_th_mul[p_ptr->stat_ind[A_DEX]]) / 100));
	else p_ptr->to_h_ranged = ((int)((p_ptr->to_h_ranged * 100) / adj_dex_th_mul[p_ptr->stat_ind[A_DEX]]));
#endif


	/* Evaluate monster AC (if skin or armor etc) */
	if (p_ptr->body_monster) {
#if defined(ENABLE_OHERETICISM) && defined(ENABLE_HELLKNIGHT)
		/* Hack: Blood Sacrifice has special traits */
		if (p_ptr->body_monster == RI_BLOODTHIRSTER && (p_ptr->pclass == CLASS_HELLKNIGHT
 #ifdef CLASS_CPRIEST
		    || p_ptr->pclass == CLASS_CPRIEST
 #endif
		    )) {
			/* never reduce AC, only increase or leave as is! */
			if (toac > (p_ptr->ac + p_ptr->to_a)) {
				p_ptr->ac = (p_ptr->ac * 1) / 2;
				p_ptr->to_a = ((p_ptr->to_a * 1) + (toac * 1)) / 2;
				p_ptr->dis_ac = (p_ptr->dis_ac * 1) / 2;
				p_ptr->dis_to_a = ((p_ptr->dis_to_a * 1) + (toac * 1)) / 2;
			}
		} else {
#endif
		if (p_ptr->pclass == CLASS_DRUID)
			body = 1 + 3 + 0 + 0; /* 0 1 0 2 1 0 = typical ANIMAL pattern,
						need to hardcode it here to balance
						'Spectral tyrannosaur' form especially.
						(weap, tors, arms, finger, head, leg) */
		else if ((p_ptr->pclass == CLASS_SHAMAN) && mimic_shaman_fulleq(r_ptr->d_char))
			body = 1 + 3 + 2 + 1; /* they can wear all items even in these 000000 forms! */
		else /* normal mimicry */
			body = (r_ptr->body_parts[BODY_HEAD] ? 1 : 0)
				+ (r_ptr->body_parts[BODY_TORSO] ? 3 : 0)
				+ (r_ptr->body_parts[BODY_ARMS] ? 2 : 0)
				+ (r_ptr->body_parts[BODY_LEGS] ? 1 : 0);

		toac = r_ptr->ac * 14 / (7 + body);

		/* p_ptr->ac += toac;
		p_ptr->dis_ac += toac; - similar to HP calculation: */
		if (toac < (p_ptr->ac + p_ptr->to_a)) {
			p_ptr->ac = (p_ptr->ac * 3) / 4;
			p_ptr->to_a = ((p_ptr->to_a * 3) + toac) / 4;
			p_ptr->dis_ac = (p_ptr->dis_ac * 3) / 4;
			p_ptr->dis_to_a = ((p_ptr->dis_to_a * 3) + toac) / 4;
		} else {
			p_ptr->ac = (p_ptr->ac * 1) / 2;
			p_ptr->to_a = ((p_ptr->to_a * 1) + (toac * 1)) / 2;
			p_ptr->dis_ac = (p_ptr->dis_ac * 1) / 2;
			p_ptr->dis_to_a = ((p_ptr->dis_to_a * 1) + (toac * 1)) / 2;
		}
		/* p_ptr->dis_ac = (toac < p_ptr->dis_ac) ?
		(((p_ptr->dis_ac * 2) + (toac * 1)) / 3) :
		(((p_ptr->dis_ac * 1) + (toac * 1)) / 2);*/
#if defined(ENABLE_OHERETICISM) && defined(ENABLE_HELLKNIGHT)
		}
#endif
	}



	/* -------------------- AC mods unaffected by body_monster: -------------------- */

	/* Invulnerability */
	if (p_ptr->invuln) {
		p_ptr->to_a += 100;
		p_ptr->dis_to_a += 100;
	}

	/* Temporary blessing */
	if (p_ptr->blessed) {
		p_ptr->to_a += p_ptr->blessed_power;
		p_ptr->dis_to_a += p_ptr->blessed_power;
		p_ptr->to_h += p_ptr->blessed_power / 2;
		p_ptr->dis_to_h += p_ptr->blessed_power / 2;
	}

	/* Temporary shield */
	if (p_ptr->shield) {
		p_ptr->to_a += p_ptr->shield_power;
		p_ptr->dis_to_a += p_ptr->shield_power;
	}

	if (p_ptr->combat_stance == 2) {
		p_ptr->to_a -= 30;
		p_ptr->dis_to_a -= 30;
	}

	if (p_ptr->mode & MODE_HARD) {
		if (p_ptr->dis_to_a > 0) p_ptr->dis_to_a = (p_ptr->dis_to_a * 2) / 3;
		if (p_ptr->to_a > 0) p_ptr->to_a = (p_ptr->to_a * 2) / 3;
	}

	/* Redraw armor (if needed) */
	if ((p_ptr->dis_ac != old_dis_ac) || (p_ptr->dis_to_a != old_dis_to_a)) {
		/* Redraw */
		p_ptr->redraw |= (PR_ARMOR);
		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);
	}



	/* Obtain the "hold" value */
	hold = adj_str_hold[p_ptr->stat_ind[A_STR]];

	/* Examine the "current bow" */
	o_ptr = &p_ptr->inventory[INVEN_BOW];

	/* Assume not heavy */
	p_ptr->heavy_shoot = FALSE;

	/* It is hard to carholdry a heavy bow */
	if (o_ptr->k_idx && hold < o_ptr->weight / 10) {
		/* Hard to wield a heavy bow */
		p_ptr->to_h += 2 * (hold - o_ptr->weight / 10);
		p_ptr->dis_to_h += 2 * (hold - o_ptr->weight / 10);

		/* Heavy Bow */
		p_ptr->heavy_shoot = TRUE;
	}

	/* Compute "extra shots" if needed */
	if (o_ptr->k_idx && !p_ptr->heavy_shoot) {
		int archery = get_archery_skill(p_ptr);

		p_ptr->tval_ammo = 0;

		/* Take note of required "tval" for missiles */
		switch (o_ptr->sval) {
		case SV_SLING:
			p_ptr->tval_ammo = TV_SHOT;
			break;
		case SV_SHORT_BOW:
		case SV_LONG_BOW:
			p_ptr->tval_ammo = TV_ARROW;
			break;
		case SV_LIGHT_XBOW:
		case SV_HEAVY_XBOW:
			p_ptr->tval_ammo = TV_BOLT;
			break;
		}

		if (archery != -1) {
			p_ptr->to_h_ranged += get_skill_scale(p_ptr, archery, 25);
			switch (archery) {
			case SKILL_SLING:
				p_ptr->num_fire += get_skill_scale(p_ptr, archery, 500) / 100;
				csheet_boni[14].shot += get_skill_scale(p_ptr, archery, 500) / 100;
				break;
			case SKILL_BOW:
				p_ptr->num_fire += get_skill_scale(p_ptr, archery, 500) / 125;
				csheet_boni[14].shot += get_skill_scale(p_ptr, archery, 500) / 125;
				break;
			case SKILL_XBOW:
#if 1
				// Stagger the EM/ES instead of massive emphasis on 25/50 (Thanks, Vir)
				if (get_skill_scale(p_ptr, archery, 500) >= 125) { //EM
					p_ptr->xtra_might++;
					csheet_boni[14].migh++;
				}
				if (get_skill_scale(p_ptr, archery, 500) >= 250) { //ES
					p_ptr->num_fire++;
					csheet_boni[14].shot++;
				}
				if (get_skill_scale(p_ptr, archery, 500) >= 375) { //EM
					p_ptr->xtra_might++;
					csheet_boni[14].migh++;
				}
				if (get_skill_scale(p_ptr, archery, 500) == 500) { //ES
					p_ptr->num_fire++;
					csheet_boni[14].shot++;
				}
#else
				p_ptr->num_fire += get_skill_scale(p_ptr, archery, 500) / 250;
				csheet_boni[14].shot += get_skill_scale(p_ptr, archery, 500) / 250;
				p_ptr->xtra_might += get_skill_scale(p_ptr, archery, 500) / 250;
				csheet_boni[14].migh += get_skill_scale(p_ptr, archery, 500) / 250;
#endif
				break;
			case SKILL_BOOMERANG:
				if (get_skill_scale(p_ptr, archery, 500) >= 500) { p_ptr->num_fire++; csheet_boni[14].shot++; }
				if (get_skill_scale(p_ptr, archery, 500) >= 333) { p_ptr->num_fire++; csheet_boni[14].shot++; }
				if (get_skill_scale(p_ptr, archery, 500) >= 166) { p_ptr->num_fire++; csheet_boni[14].shot++; }

				/* Boomerang-mastery directly increases damage! - C. Blue */
				p_ptr->to_d_ranged += get_skill_scale(p_ptr, archery, 20);
				break;
			}
			if (archery != SKILL_BOOMERANG) {
#if 1//pfft, too disrupting vs slingers to disable this I guess.. need to think harder of a good way :|
/* temporarily disabled, need to talk first if we
   really want to boost the 50.000 skill this much,
   especially considering xbows who gain TWO xmight with this at once,
   but also with bows/slings which get ES _and_ EM at 50,
   rendering the power-scale of training the mastery skill
   very non-liner with this pull-up. */
#if 1
//Put less emphasis on ARCHERY (which only archers can get and more into the individual skill. 
//Make other archers more viable.
				p_ptr->xtra_might += get_skill_scale(p_ptr, archery, 1);
				csheet_boni[14].migh += get_skill_scale(p_ptr, archery, 1);
#else
 				p_ptr->xtra_might += (get_skill(p_ptr, SKILL_ARCHERY) / 50);
#endif
#endif

				p_ptr->to_d_ranged += get_skill_scale(p_ptr, SKILL_ARCHERY, 10);
			}
		}

		/* Add in the "bonus shots" */
		p_ptr->num_fire += extra_shots;

		/* Require at least one shot */
		if (p_ptr->num_fire < 1) p_ptr->num_fire = 1;

/* Turn off the spr cap in rpg? -the_sandman */
		/* Cap shots per round at 5 */
/*		if (p_ptr->num_fire > 5) p_ptr->num_fire = 5;
*/
		/* Other classes than archer or ranger have cap at 4 - the_sandman and mikaelh */
/*		if (p_ptr->pclass != CLASS_ARCHER && p_ptr->pclass != CLASS_RANGER && p_ptr->num_fire > 4) p_ptr->num_fire = 4;
*/
	}

	/* Add in the "bonus spells" */
	p_ptr->num_spell += extra_spells;

	/* Examine the "tool" */
	o_ptr = &p_ptr->inventory[INVEN_TOOL];

	/* Boost digging skill by tool weight */
	if (o_ptr->k_idx && o_ptr->tval == TV_DIGGING) {
		p_ptr->skill_dig += (o_ptr->weight / 10);

		/* Hack -- to_h/to_d added to digging (otherwise meanless) */
		p_ptr->skill_dig += o_ptr->to_h;
		p_ptr->skill_dig += o_ptr->to_d;

		p_ptr->skill_dig += p_ptr->skill_dig *
		    get_skill_scale(p_ptr, SKILL_DIG, 200) / 100;
	}

	/* Examine the "shield" */
	o_ptr = &p_ptr->inventory[INVEN_ARM];
	/* Assume not heavy */
	p_ptr->heavy_shield = FALSE;
	/* It is hard to hold a heavy shield */
	if (o_ptr->k_idx && o_ptr->tval == TV_SHIELD) {
		if (hold < (o_ptr->weight / 10 - 4) * 7) { /* dual-wielder? */
			p_ptr->heavy_shield = TRUE;
#ifndef USE_NEW_SHIELDS
			p_ptr->ac += may_ac / 2;
			p_ptr->dis_ac += may_ac / 2;
#endif
#ifndef NEW_SHIELDS_NO_AC
			p_ptr->to_a += may_to_a / 2;
			p_ptr->dis_to_a += may_dis_to_a / 2;
#endif
		} else {
			if (may_reflect) { p_ptr->reflect = TRUE; csheet_boni[INVEN_ARM-INVEN_WIELD].cb[6] |= CB7_RREFL; }
#ifndef USE_NEW_SHIELDS
			p_ptr->ac += may_ac;
			p_ptr->dis_ac += may_ac;
#endif
#ifndef NEW_SHIELDS_NO_AC
			p_ptr->to_a += may_to_a;
			p_ptr->dis_to_a += may_dis_to_a;
#endif
		}
	}

#ifdef USE_BLOCKING
	if (o_ptr->k_idx && o_ptr->tval == TV_SHIELD) { /* might be dual-wielder */
		int malus;

 #ifdef USE_NEW_SHIELDS
		p_ptr->shield_deflect = o_ptr->ac * 10; /* add a figure by multiplying by _10_ for more accuracy */
 #else
		p_ptr->shield_deflect = 20 * (o_ptr->ac + 3); /* add a figure by multiplying by 2*_10_ for more accuracy */
 #endif
		malus = (o_ptr->weight / 10 - 4) * 7 - hold;
		p_ptr->shield_deflect /= malus > 0 ? 10 + (malus + 4) / 3 : 10;

		/* adjust class-dependantly! */
		switch (p_ptr->pclass) {
		case CLASS_ADVENTURER: p_ptr->shield_deflect = (p_ptr->shield_deflect * 4 + 2) / 6; break;
		case CLASS_WARRIOR: p_ptr->shield_deflect = p_ptr->shield_deflect; break;
#ifdef ENABLE_CPRIEST
		case CLASS_CPRIEST:
#endif
		case CLASS_PRIEST: p_ptr->shield_deflect = (p_ptr->shield_deflect * 4 + 2) / 6; break;
		case CLASS_MAGE: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 4) / 6; break;
		case CLASS_ARCHER: p_ptr->shield_deflect = (p_ptr->shield_deflect * 4 + 2) / 6; break;
		case CLASS_ROGUE: p_ptr->shield_deflect = (p_ptr->shield_deflect * 4 + 2) / 6; break;
		case CLASS_MIMIC: p_ptr->shield_deflect = (p_ptr->shield_deflect * 5 + 1) / 6; break;
		case CLASS_RANGER: p_ptr->shield_deflect = (p_ptr->shield_deflect * 4 + 2) / 6; break;
 #ifdef ENABLE_DEATHKNIGHT
		case CLASS_DEATHKNIGHT:
 #endif
 #ifdef ENABLE_HELLKNIGHT
		case CLASS_HELLKNIGHT:
 #endif
		case CLASS_PALADIN: p_ptr->shield_deflect = p_ptr->shield_deflect; break;
		case CLASS_SHAMAN: p_ptr->shield_deflect = (p_ptr->shield_deflect * 3 + 3) / 6; break;
		case CLASS_DRUID: p_ptr->shield_deflect = (p_ptr->shield_deflect * 3 + 3) / 6; break;
		case CLASS_RUNEMASTER: p_ptr->shield_deflect = (p_ptr->shield_deflect * 3 + 3) / 6; break;
		case CLASS_MINDCRAFTER: p_ptr->shield_deflect = (p_ptr->shield_deflect * 4 + 2) / 6; break;
		}
	}
#endif

	/* Examine the "main weapon" */
	/* note- for dual-wield we don't need to run the main parry checks again on the secondary weapon,
	   because you are not allowed to hold 2h or 1.5h weapons in secondary slot. this leaves only one
	   possibility for dual-wield: 1h - which we can cover in a single test at the end - C. Blue */
	o_ptr = &p_ptr->inventory[INVEN_WIELD];
	/* if dual-wielder only has weapon in second hand, use that one! (assume left-handed person ;)) */
	if (!o_ptr->k_idx && p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)
		o_ptr = &p_ptr->inventory[INVEN_ARM];
	/* Assume not heavy */
	p_ptr->heavy_wield = FALSE;
#ifdef USE_PARRYING
	/* Do we have a weapon equipped at all? */
	if (o_ptr->k_idx) {
		if (k_info[o_ptr->k_idx].flags4 & TR4_MUST2H) {
			p_ptr->weapon_parry = 10 + get_skill_scale(p_ptr, SKILL_MASTERY, 20);
		} else if (k_info[o_ptr->k_idx].flags4 & TR4_SHOULD2H) {
			if (!p_ptr->inventory[INVEN_ARM].k_idx) p_ptr->weapon_parry = 10 + get_skill_scale(p_ptr, SKILL_MASTERY, 15);
			else p_ptr->weapon_parry = 3 + get_skill_scale(p_ptr, SKILL_MASTERY, 7);
		} else if (k_info[o_ptr->k_idx].flags4 & TR4_COULD2H) {
			if (!p_ptr->inventory[INVEN_ARM].k_idx || !p_ptr->inventory[INVEN_WIELD].k_idx)
				p_ptr->weapon_parry = 10 + get_skill_scale(p_ptr, SKILL_MASTERY, 10);
			else p_ptr->weapon_parry = 5 + get_skill_scale(p_ptr, SKILL_MASTERY, 10);
		} else {
			p_ptr->weapon_parry = 5 + get_skill_scale(p_ptr, SKILL_MASTERY, 10);
		}
		/* for dual-wielders, get a parry bonus for second weapon: */
		if (p_ptr->dual_wield && p_ptr->dual_mode && !p_ptr->rogue_heavyarmor
#if 0
		    /* don't give parry bonus if one of our weapons is NEVER_BLOW? */
		    && never_blow == 0x0
#endif
		    )
			//p_ptr->weapon_parry += 5 + get_skill_scale(p_ptr, SKILL_MASTERY, 5);//was +0(+10)
			p_ptr->weapon_parry += 10;//pretty high, because independent of mastery skill^^

		/* adjust class-dependantly! */
		switch (p_ptr->pclass) {
		case CLASS_ADVENTURER: p_ptr->weapon_parry = (p_ptr->weapon_parry * 4 + 2) / 6; break;
		case CLASS_WARRIOR: p_ptr->weapon_parry = p_ptr->weapon_parry; break;
 #ifdef ENABLE_CPRIEST
		case CLASS_CPRIEST:
 #endif
		case CLASS_PRIEST: p_ptr->weapon_parry = (p_ptr->weapon_parry * 4 + 2) / 6; break;
		case CLASS_MAGE: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2 + 4) / 6; break;
		case CLASS_ARCHER: p_ptr->weapon_parry = (p_ptr->weapon_parry * 4 + 2) / 6; break;
		case CLASS_ROGUE: p_ptr->weapon_parry = p_ptr->weapon_parry; break;
		case CLASS_MIMIC: p_ptr->weapon_parry = (p_ptr->weapon_parry * 5 + 1) / 6; break;
		case CLASS_RANGER: p_ptr->weapon_parry = p_ptr->weapon_parry; break;
 #ifdef ENABLE_DEATHKNIGHT
		case CLASS_DEATHKNIGHT:
 #endif
 #ifdef ENABLE_HELLKNIGHT
		case CLASS_HELLKNIGHT:
 #endif
		case CLASS_PALADIN: p_ptr->weapon_parry = (p_ptr->weapon_parry * 5 + 1) / 6; break;
		case CLASS_SHAMAN: p_ptr->weapon_parry = (p_ptr->weapon_parry * 3 + 3) / 6; break;
		case CLASS_DRUID: p_ptr->weapon_parry = (p_ptr->weapon_parry * 3 + 3) / 6; break;
		case CLASS_RUNEMASTER: p_ptr->weapon_parry = (p_ptr->weapon_parry * 4 + 2) / 6; break;
		case CLASS_MINDCRAFTER: p_ptr->weapon_parry = (p_ptr->weapon_parry * 5 + 1) / 6; break;
		}
	}
#endif

	/* It is hard to hold a heavy weapon */
	o_ptr = &p_ptr->inventory[INVEN_WIELD];
	if (o_ptr->k_idx && hold < o_ptr->weight / 10) {
		/* Hard to wield a heavy weapon */
		p_ptr->to_h += 2 * (hold - o_ptr->weight / 10);
		p_ptr->dis_to_h += 2 * (hold - o_ptr->weight / 10);

		if (p_ptr->weapon_parry > 0) p_ptr->weapon_parry /= 2; /* easy for now */

		/* Heavy weapon */
		p_ptr->heavy_wield = TRUE;
	}
	/* dual-wield */
	/* It is hard to hold a heavy weapon */
	o_ptr = &p_ptr->inventory[INVEN_ARM];
	if (o_ptr->k_idx && o_ptr->tval != TV_SHIELD && hold < o_ptr->weight / 10) {
		/* Hard to wield a heavy weapon */
		p_ptr->to_h += 2 * (hold - o_ptr->weight / 10);
		p_ptr->dis_to_h += 2 * (hold - o_ptr->weight / 10);

		if (p_ptr->weapon_parry > 0) p_ptr->weapon_parry /= 2; /* easy for now */

		/* Heavy weapon */
		p_ptr->heavy_wield = TRUE;
	}


	/* Normal weapons */
	o_ptr = &p_ptr->inventory[INVEN_WIELD];
	o2_ptr = &p_ptr->inventory[INVEN_ARM];
	melee_weapon = (o_ptr->k_idx || (o2_ptr->k_idx && o2_ptr->tval != TV_SHIELD));
	if (melee_weapon && !p_ptr->heavy_wield) {
		int lev1 = -1, lev2 = -1, chh_bpr = 0;

		p_ptr->num_blow = calc_blows_weapons(Ind);


		/* one of our two weapons is NEVER_BLOW? half total # of attacks (and half extra blows accordingly) */
		if ((never_blow == 0x2 || never_blow == 0x4) && p_ptr->dual_wield && p_ptr->dual_mode) {
			p_ptr->num_blow += (p_ptr->extra_blows + 1) / 2; //we're nice: rounding up! ('Character has control over weapon usaage'..)
			if (p_ptr->num_blow < 1) p_ptr->num_blow = 1;

			/* Boost blows with masteries */
			/* note for dual-wield: instead of using two different p_ptr->to_h/d_melee for each
			   weapon, we just average the mastery boni we'd receive on each - C. Blue */
			if (never_blow == 0x4 && (i = get_weaponmastery_skill(p_ptr, &p_ptr->inventory[INVEN_WIELD])) != -1) {
				lev1 = get_skill(p_ptr, i);
				/* Get intrinsic blow boni for column display (so everything adds up) */
				chh_bpr += get_skill_scale(p_ptr, i, 2);
			}
			if (never_blow == 0x2 && (i = get_weaponmastery_skill(p_ptr, &p_ptr->inventory[INVEN_ARM])) != -1) {
				lev2 = get_skill(p_ptr, i);
				/* Get intrinsic blow boni for column display (so everything adds up) */
				chh_bpr += get_skill_scale(p_ptr, i, 2);
			}
			csheet_boni[14].blow += (chh_bpr + 1) / 2; //rounding up..

			/* if we don't wear any weapon at all, we get 0 bonus */
			if (lev1 == -1 && lev2 == -1) lev2 = 0;
			/* if we don't dual-wield, we mustn't average things */
			if (lev1 == -1) lev1 = lev2;
			if (lev2 == -1) lev2 = lev1;
			/* average for dual-wield */
			if (p_ptr->dual_mode) {
				p_ptr->to_h_melee += ((lev1 + lev2) / 2);
				p_ptr->to_d_melee += ((lev1 + lev2) / 6);
			} else { /* treat main-hand mode basically like weapon+shield, in terms of accuracy/damage (ie no loss) */
				p_ptr->to_h_melee += lev1;
				p_ptr->to_d_melee += lev1 / 3;
			}
		}
		/* all our weapons are NEVER_BLOW? no melee then */
		else if (never_blow) {
			p_ptr->to_h_melee = p_ptr->to_d_melee = 0;
			p_ptr->num_blow = 0;
		}
		/* all weapons (1 or 2) are fine */
		else {
			p_ptr->num_blow += p_ptr->extra_blows;
			if (p_ptr->num_blow < 1) p_ptr->num_blow = 1;

			/* Boost blows with masteries */
			/* note for dual-wield: instead of using two different p_ptr->to_h/d_melee for each
			   weapon, we just average the mastery boni we'd receive on each - C. Blue */
			if ((i = get_weaponmastery_skill(p_ptr, &p_ptr->inventory[INVEN_WIELD])) != -1) {
				lev1 = get_skill(p_ptr, i);
				/* Get intrinsic blow boni for column display (so everything adds up) */
				chh_bpr += get_skill_scale(p_ptr, i, 2);
			}
			if ((i = get_weaponmastery_skill(p_ptr, &p_ptr->inventory[INVEN_ARM])) != -1) {
				lev2 = get_skill(p_ptr, i);
				/* Get intrinsic blow boni for column display (so everything adds up) */
				chh_bpr += get_skill_scale(p_ptr, i, 2);
			}
			if (lev1 != -1 && lev2 != -1) csheet_boni[14].blow += (chh_bpr + 1) / 2; //rounding up..
			else csheet_boni[14].blow += chh_bpr;

			/* if we don't wear any weapon at all, we get 0 bonus */
			if (lev1 == -1 && lev2 == -1) lev2 = 0;
			/* if we don't dual-wield, we mustn't average things */
			if (lev1 == -1) lev1 = lev2;
			if (lev2 == -1) lev2 = lev1;
			/* average for dual-wield */
			if (p_ptr->dual_mode) {
				p_ptr->to_h_melee += ((lev1 + lev2) / 2);
				p_ptr->to_d_melee += ((lev1 + lev2) / 6);
			} else { /* treat main-hand mode basically like weapon+shield, in terms of accuracy/damage (ie no loss) */
				p_ptr->to_h_melee += lev1;
				p_ptr->to_d_melee += lev1 / 3;
			}
		}
	}


	/* Different calculation for monks with empty hands */
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS) && !melee_weapon &&
	    never_blow != 0x1 &&
#ifndef ENABLE_MA_BOOMERANG
	    !(p_ptr->inventory[INVEN_BOW].k_idx)) {
#else
	    p_ptr->inventory[INVEN_BOW].tval != TV_BOW) {
#endif
		int marts = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 50);
		p_ptr->num_blow = 0;

		/*the_sandman for the RPG server (might as well stay for all servers ^^ - C. Blue) */
		if (marts >  0) { p_ptr->num_blow++; csheet_boni[14].blow++; }
		if (marts >  9) { p_ptr->num_blow++; csheet_boni[14].blow++; }
		if (marts > 19) { p_ptr->num_blow++; csheet_boni[14].blow++; }
		if (marts > 29) { p_ptr->num_blow++; csheet_boni[14].blow++; }
		//if (marts > 34) p_ptr->num_blow++; csheet_boni[14].blow++; } /* This _could_ be added if really required */
		if (marts > 39) { p_ptr->num_blow++; csheet_boni[14].blow++; }
		if (marts > 44) { p_ptr->num_blow++; csheet_boni[14].blow++; }
		if (marts > 49) { p_ptr->num_blow++; csheet_boni[14].blow++; }

		if (monk_heavy_armor(p_ptr)) p_ptr->num_blow /= 2;

		p_ptr->num_blow += 1 + p_ptr->extra_blows;
		if (p_ptr->num_blow < 1) p_ptr->num_blow = 1;

		if (!monk_heavy_armor(p_ptr)) {
			p_ptr->to_h_melee += (marts * 4) / 2;//was *3/2

#if 1 /* new boost */
			p_ptr->to_d_melee += ((marts * 3) / 5); /* new MA boost */
#elif 1 /* experimental */
			p_ptr->to_d_melee += ((marts * 2) / 5); /* for better form influence of MA mimics */
#else
			p_ptr->to_d_melee += (marts / 2); /* was 3 */
#endif

			/* Testing: added a new bonus. No more single digit dmg at lvl 20, I hope, esp as warrior. the_sandman */
			/* changed by C. Blue, so it's available to mimics too */
#ifndef MARTIAL_TO_D_HACK
			p_ptr->to_d_melee += 20 - (400 / (marts + 20)); /* get strong quickly and early - quite special ;-o */
#else /* Instead of to_d_melee, we use to_d here. The idea is that shapechanger shouldn't get too low +dam from high-power forms. ugly hack :( */
			/* actually we shouldn't do this. AC can no longer be reduced by weak forms,
			   so weapons shouldn't count as part of the form either, hence MA dmg fully counts as part of the form now.. */
			p_ptr->to_d += 20 - (400 / (marts + 20)); /* get strong quickly and early - quite special ;-o */
			p_ptr->dis_to_d += 20 - (400 / (marts + 20));
#endif
		}

	} else { /* make cumber_armor have effect on to-hit for non-martial artists too - C. Blue */
		if (p_ptr->cumber_armor && (p_ptr->to_h_melee > 0)) p_ptr->to_h_melee = (p_ptr->to_h_melee * 2) / 3;
	}

	//p_ptr->pspeed += get_skill_scale(p_ptr, SKILL_AGILITY, 10);
	if (p_ptr->cumber_armor) {
		p_ptr->pspeed += get_skill_scale(p_ptr, SKILL_SNEAKINESS, 4);
		csheet_boni[14].spd += get_skill_scale(p_ptr, SKILL_SNEAKINESS, 4);
	} else {
		p_ptr->pspeed += get_skill_scale(p_ptr, SKILL_SNEAKINESS, 7);
		csheet_boni[14].spd += get_skill_scale(p_ptr, SKILL_SNEAKINESS, 7);
	}

	p_ptr->redraw |= (PR_SPEED | PR_EXTRA) ;

	/* Hard mode */
	if ((p_ptr->mode & MODE_HARD)) {
		if (p_ptr->pspeed > 110) {
			int speed = p_ptr->pspeed - 110;
			speed = (speed + 1) / 2;
			p_ptr->pspeed = speed + 110;
		}
		if (p_ptr->num_blow > 1) p_ptr->num_blow--;
		if (p_ptr->num_fire > 1) p_ptr->num_fire--;
	}

	/* PvP mode */
	if ((p_ptr->mode & MODE_PVP)) {
		/* don't reduce +speed bonus from sneakiness skill */
		if (p_ptr->pspeed > 110) {
			int sneak = get_skill_scale(p_ptr, SKILL_SNEAKINESS, 7), speed = p_ptr->pspeed - 110 - sneak;
			speed = (speed + 1) / 2;
			p_ptr->pspeed = speed + 110 + sneak;
		}

		p_ptr->to_d = (p_ptr->to_d + 1) / 2;
		p_ptr->dis_to_d = (p_ptr->dis_to_d + 1) / 2;

		p_ptr->to_h = (p_ptr->to_h + 1) / 2;
		p_ptr->dis_to_h = (p_ptr->dis_to_h + 1) / 2;

#if 0
		p_ptr->to_a = (p_ptr->to_a + 1) / 2;
		p_ptr->dis_to_a = (p_ptr->dis_to_a + 1) / 2;
#endif

		p_ptr->xtra_crit = (p_ptr->xtra_crit + 2) / 3;

		/* extra_blows, extra_might, extra_shots */
	}

	/* A perma_cursed weapon stays even in weapon-less body form, reduce blows for that: */
	if (melee_weapon &
	    (!r_info[p_ptr->body_monster].body_parts[BODY_WEAPON]) &&
	    (p_ptr->num_blow > 1)) p_ptr->num_blow = 1;

	/* Weaponmastery bonus to hit and damage - not for MA!- C. Blue */
	if (get_skill(p_ptr, SKILL_MASTERY) && melee_weapon && p_ptr->num_blow) {
		int lev = get_skill(p_ptr, SKILL_MASTERY);
		p_ptr->to_h_melee += lev / 3;
		p_ptr->to_d_melee += lev / 10;
	}

	if (get_skill(p_ptr, SKILL_DODGE) && !p_ptr->rogue_heavyarmor)
	//if (!(r_ptr->flags1 & RF1_NEVER_MOVE)); // not for now
	{
#ifndef NEW_DODGING /* reworking dodge, see #else.. */
		/* use a long var temporarily to handle v.high total_weight */
		long temp_chance;
		/* Get the armor weight */
		int cur_wgt = armour_weight(p_ptr); /* change to worn_armour_weight if ever reactivated, and possibly adjust values */
		/* Base dodge chance */
		temp_chance = get_skill_scale(p_ptr, SKILL_DODGE, 150);
		/* Armor weight bonus/penalty */
		//p_ptr->dodge_level -= cur_wgt * 2;
		temp_chance -= cur_wgt;		/* XXX adjust me */
		/* Encumberance bonus/penalty */
		temp_chance -= p_ptr->total_weight / 100;
		/* Penalty for bad conditions */
		temp_chance -= UNAWARENESS(p_ptr);
		/* never below 0 */
		if (temp_chance < 0) temp_chance = 0;
		/* write long back to int */
		p_ptr->dodge_level = temp_chance;
#else /* reworking dodge here - C. Blue */
		/* I think it's cool to have a skill which for once doesn't depend on friggin armour_weight,
		   but on total weight for a change. Cool variation. - C. Blue */
		/* also see new helper function apply_dodge_chance() */

		/* ok, different idea: Treating inventory+equipment separately. Added 'equip_weight' */
		/* inven = 50.0 without loot/books, equip ~ 25.0 with light stuff, non-armour 15~30 (digger y/n) */
		/* rules here: backpack grows exponentially; non-weapons grow exponentially; weapons grow linear;
		               also special is that although backpack and non-weapons grow similar, they grow separately! */
		/* note: rings, amulet, light source and tool are simplifiedly counted to "weapons" here */
		long int e = equip_weight(p_ptr), w = e - armour_weight(p_ptr), i = p_ptr->total_weight - e; /* possibly change to worn_armour_weight, but not really much difference/needed imho - C. Blue */
		e = e - w;

		/* prevent any weird overflows if 'someone' collects gronds */
		if (i >= 5000) i = 1;
		else {
			//i = i / 5 + 10; /* 0:-0 25:-2 50:-8 75:-18 100:-31 125:-48 150:-68 */
			i = i / 7 + 10; /* 0:-0 25:-1 50:-4 75:-9 100:-16 125:-25 150:-35 */
			i = (i * i) / 1400;

			e = e / 8 + 10; /* -0...-6 (up to 15.0 lb armour weight; avg -3) */
			e = (e * e) / 100 - 1;

			w = w / 50; /* -0...-11 (avg -3; 2x heavy 1-h + mattock + misc items. note: ammo can get heavy too!) */

			i = 100 - i - e - w;
		}

		i = (i * 100) / (100 + UNAWARENESS(p_ptr) * 4);
		/* fix bottom limit (1) */
		if (i < 1) i = 1; /* minimum, everyone may dodge even if not skilled :) */
		/* fix top limit (100) - just paranoia: if above calculations were correct we can't go above 100. */
		if (i > 100) i = 100;
		/* write back to int */
		p_ptr->dodge_level = i;
#endif
	}

	/* Assume okay */
	p_ptr->icky_wield = FALSE;
	p_ptr->awkward_wield = FALSE;
	p_ptr->easy_wield = FALSE;
	p_ptr->awkward_shoot = FALSE;

	/* 2handed weapon and shield = less damage */
	//if (inventory[INVEN_WIELD + i].k_idx && inventory[INVEN_ARM + i].k_idx)
	if (p_ptr->inventory[INVEN_WIELD].k_idx && p_ptr->inventory[INVEN_ARM].k_idx) {
		/* Extract the item flags */
		object_flags(&p_ptr->inventory[INVEN_WIELD], &f1, &f2, &f3, &f4, &f5, &f6, &esp);

		if (f4 & TR4_SHOULD2H) {
			/* Reduce the real bonuses */
			if (p_ptr->to_h > 0) p_ptr->to_h = (2 * p_ptr->to_h) / 3;
			if (p_ptr->to_d > 0) p_ptr->to_d = (2 * p_ptr->to_d) / 3;

			/* Reduce the mental bonuses */
			if (p_ptr->dis_to_h > 0) p_ptr->dis_to_h = (2 * p_ptr->dis_to_h) / 3;
			if (p_ptr->dis_to_d > 0) p_ptr->dis_to_d = (2 * p_ptr->dis_to_d) / 3;

			/* Reduce the weaponmastery bonuses */
			if (p_ptr->to_h_melee > 0) p_ptr->to_h_melee = (2 * p_ptr->to_h_melee) / 3;
			if (p_ptr->to_d_melee > 0) p_ptr->to_d_melee = (2 * p_ptr->to_d_melee) / 3;

			p_ptr->awkward_wield = TRUE;
		}
	}

#ifdef USE_PARRYING
	if (p_ptr->heavy_wield) p_ptr->weapon_parry /= 3;
	else if (p_ptr->awkward_wield) p_ptr->weapon_parry /= 2;
#endif
#ifdef USE_BLOCKING
	if (p_ptr->heavy_shield) p_ptr->shield_deflect /= 3;
#endif

	if (p_ptr->inventory[INVEN_WIELD].k_idx && !p_ptr->inventory[INVEN_ARM].k_idx) {
		object_flags(&p_ptr->inventory[INVEN_WIELD], &f1, &f2, &f3, &f4, &f5, &f6, &esp);
		if (f4 & TR4_COULD2H) p_ptr->easy_wield = TRUE;
	}
	/* for dual-wield..*/
	if (!p_ptr->inventory[INVEN_WIELD].k_idx &&
	    p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) {
		object_flags(&p_ptr->inventory[INVEN_ARM], &f1, &f2, &f3, &f4, &f5, &f6, &esp);
		if (f4 & TR4_COULD2H) p_ptr->easy_wield = TRUE;
	}

	/* Equipment weight affects shooting */
	/* allow malus even */
	/* examples: 10.0: 0, 20.0: -4, 30.0: -13, 40.0: -32, 47.0: -50 cap */
	i = armour_weight(p_ptr) / 10;
	i = (i * i * i) / 2000;
	if (i > 50) i = 50; /* reached at 470.0 lb */
	p_ptr->to_h_ranged -= i;

	/* also: shield and ranged weapon = less accuracy for ranged weapon! */
	/* dual-wield currently does not affect shooting (!) */
	if (p_ptr->inventory[INVEN_BOW].k_idx && p_ptr->inventory[INVEN_ARM].k_idx
	    && p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD) {
		/* can't aim well while carrying a shield on the arm! */

		/* the following part punishes all +hit/+dam for ranged weapons;
		   trained fighters suffer more than untrained ones! - C. Blue */
		if (p_ptr->to_h_ranged >= 25) p_ptr->to_h_ranged = 0;
		/* note: it's to be interpreted as "larger shield -> harder to shoot", not "heavier", ie STR doesn't matter here: */
		else if (p_ptr->to_h_ranged >= 0) p_ptr->to_h_ranged -= 10 + (p_ptr->to_h_ranged * (160 + p_ptr->inventory[INVEN_ARM].weight) / 350); /* 3/5..4/5 depending on shield */
		else p_ptr->to_h_ranged -= 10;

		p_ptr->awkward_shoot = TRUE;
	}

	/* Priest weapon penalty for non-blessed edged weapons (assumes priests cannot dual-wield) */
	if (p_ptr->inventory[INVEN_WIELD].k_idx && (
	    (p_ptr->pclass == CLASS_PRIEST && !p_ptr->bless_blade &&
	    (o_ptr->tval == TV_SWORD || o_ptr->tval == TV_POLEARM || o_ptr->tval == TV_AXE))
#ifdef ENABLE_CPRIEST
	    || (p_ptr->pclass == CLASS_CPRIEST && p_ptr->bless_blade)
#endif
#ifdef ENABLE_HELLKNIGHT
	    || (p_ptr->pclass == CLASS_HELLKNIGHT && p_ptr->bless_blade)
#endif
	    )) {
		/* Reduce the real bonuses */
		/*p_ptr->to_h -= 2;
		p_ptr->to_d -= 2;*/
		p_ptr->to_h = p_ptr->to_h * 3 / 5;
		p_ptr->to_d = p_ptr->to_d * 3 / 5;
		p_ptr->to_h_melee = p_ptr->to_h_melee * 3 / 5;
		p_ptr->to_d_melee = p_ptr->to_d_melee * 3 / 5;

		/* Reduce the mental bonuses */
		/*p_ptr->dis_to_h -= 2;
		p_ptr->dis_to_d -= 2;*/
		p_ptr->dis_to_h = p_ptr->dis_to_h * 3 / 5;
		p_ptr->dis_to_d = p_ptr->dis_to_d * 3 / 5;

		/* Icky weapon */
		p_ptr->icky_wield = TRUE;
	}

	if (p_ptr->body_monster) {
		d = 0;
		for (i = 0; i < 4; i++) {
			j = (r_ptr->blow[i].d_dice * r_ptr->blow[i].d_side);
			j += r_ptr->blow[i].d_dice;
			j /= 2;

			d += j; /* adding up average monster damage per round */
		}

		/* GWoP: 472, GB: 270, Green DR: 96 */
		/* GWoP: 254, GT: 364, GB: 149, GDR: 56 */
		/* Quarter the damage and cap against 150 (unreachable though)
		- even The Destroyer form would reach just 138 ;) */
		d /= 4; /* average monster damage per blow */
		/* GWoP: 63, GT: 91, GB: 37, GDR: 14 */
#ifndef MIMIC_TO_D_DENTHACK /* a bit too little distinguishment for high-dam MA forms (Jabberwock vs Maulotaur for druids -> almost NO difference!) */
		//d = (3200 / ((900 / (d + 4)) + 22)) - 10; //still too high
		//d = (3200 / ((800 / (d + 4)) + 22)) - 20; //not enough, but on the way
		//d = (2850 / ((650 / (d + 4)) + 22)) - 20; //not quite there yet
		//d = (2500 / ((500 / (d + 4)) + 22)) - 20; //final target: Aim at +27 to-dam increase, which is still a lot --still too high ~~
		d = (2200 / ((250 / (d + 4)) + 22)) - 20;
		//d = (2000 / ((130 / (d + 4)) + 22)) - 20;//too little +dam increase
#else /* add a 'dent' for '08/15 forms' :-p to help Jabberwock shine moar vs Maulotaur */
		/* problem: these won't cut it - need cubic splines or something..
		   Goal: rise quickly, stay flat in mid-range, increase again in top range but cap quickly. */
		/* mhhh, instead let's try to subtract one polynomial from another, to add a "dent" (downwards) into the power curve
		   at 'common/boring' monster damage ranges (Maulotaur for example, although slightly better than most lame average^^) */
		//WRONG: this formula was for double values of d, oops (as if Maulo had 55 and Jabber 112)
		//d = (2500 / ((500 / (d + 4)) + 22)) - 20 - 10000 / ((d - 50) * (d - 50) + 490);
		//Maulo@25, Jabber@55, this seems fine:
		//d = (2500 / ((500 / (d + 4)) + 22)) - 20 - 700 / ((d - 25) * (d - 25) + 70);//geez :/ it's the Godzilla of the the mimic formulas I think..
		//d = (2500 / ((500 / (d + 4)) + 22)) - 20 - 1300 / ((d - 26) * (d - 26) + 90);//was ok
		d = (2200 / ((250 / (d + 4)) + 22)) - 20 - 950 / ((d - 25) * (d - 25) + 100);
		//d = (2000 / ((130 / (d + 4)) + 22)) - 20 - 650 / ((d - 25) * (d - 25) + 90);//too little
#endif

		/* Calculate new averaged to-dam bonus */
		if (d < p_ptr->to_d_melee)
#ifndef MIMICRY_BOOST_WEAK_FORM
			p_ptr->to_d_melee = (p_ptr->to_d_melee * 5 + d * 2) / 7;
		else
#else
			d = p_ptr->to_d_melee;
#endif
			p_ptr->to_d_melee = (p_ptr->to_d_melee + d) / 2;
	}


#if 0 /* disabled atm (it's already not easy to find two weapons that both deal great damage) */
	/* dual-wielding gets a slight to-hit malus */
	if (p_ptr->dual_wield && p_ptr->dual_mode) p_ptr->to_h_melee = (p_ptr->to_h_melee * 17) / 20; /* 85% */
#endif


	/* Note that stances currently factor in AFTER mimicry effect has been calculated!
	   Reason I chose this for now is that currently mimicry does not effect shield_deflect,
	   but does affect to_h and to_d. This would cause odd balance, so this way neither
	   shield_deflect nor to_h/to_d are affected. Might need fixing/adjusting later. - C. Blue */
#ifdef ENABLE_STANCES
 #ifdef USE_BLOCKING /* need blocking to make use of defensive stance */
		if ((p_ptr->combat_stance == 1) &&
		    (p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)) switch (p_ptr->combat_stance_power) {
			/* note that defensive stance also increases chance to actually prefer shield over parrying in melee.c */
			case 0: p_ptr->shield_deflect += 9;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 7) / 10;
				p_ptr->to_d = (p_ptr->to_d * 7) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 7) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
			case 1: p_ptr->shield_deflect += 11;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 7) / 10;
				p_ptr->to_d = (p_ptr->to_d * 7) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 7) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
			case 2: p_ptr->shield_deflect += 13;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 7) / 10;
				p_ptr->to_d = (p_ptr->to_d * 7) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 7) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
			case 3: p_ptr->shield_deflect += 15;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 8) / 10;
				p_ptr->to_d = (p_ptr->to_d * 8) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 8) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
		}
 #endif
 #ifdef USE_PARRYING /* need parrying to make use of offensive stance */
		if (p_ptr->combat_stance == 2) switch (p_ptr->combat_stance_power) {
			case 0: p_ptr->weapon_parry = (p_ptr->weapon_parry * 0) / 10;
				p_ptr->dodge_level = (p_ptr->dodge_level * 0) / 10;
				break;
			case 1: p_ptr->weapon_parry = (p_ptr->weapon_parry * 1) / 10;
				p_ptr->dodge_level = (p_ptr->dodge_level * 1) / 10;
				break;
			case 2: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2) / 10;
				p_ptr->dodge_level = (p_ptr->dodge_level * 2) / 10;
				break;
			case 3: p_ptr->weapon_parry = (p_ptr->weapon_parry * 3) / 10;
				p_ptr->dodge_level = (p_ptr->dodge_level * 3) / 10;
				break;
		}
  #ifdef ALLOW_SHIELDLESS_DEFENSIVE_STANCE
		else if ((p_ptr->combat_stance == 1) &&
		    (p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) switch (p_ptr->combat_stance_power) {
			case 0: p_ptr->weapon_parry = (p_ptr->weapon_parry * 13) / 10;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 6) / 10;
				p_ptr->to_d = (p_ptr->to_d * 6) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 6) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
			case 1: p_ptr->weapon_parry = (p_ptr->weapon_parry * 13) / 10;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 7) / 10;
				p_ptr->to_d = (p_ptr->to_d * 7) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 7) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
			case 2: p_ptr->weapon_parry = (p_ptr->weapon_parry * 14) / 10;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 7) / 10;
				p_ptr->to_d = (p_ptr->to_d * 7) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 7) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
			case 3: p_ptr->weapon_parry = (p_ptr->weapon_parry * 15) / 10;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 7) / 10;
				p_ptr->to_d = (p_ptr->to_d * 7) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 7) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
		}
  #endif
 #endif
#endif



	/* mali for blocking/parrying */
	if (p_ptr->paralyzed || p_ptr->stun > 100) {
		p_ptr->shield_deflect /= 4;
		p_ptr->weapon_parry = 0;
		/* also give a stealth bonus? (Sav's suggestion) */
	} else if (p_ptr->blind || p_ptr->confused) {
		p_ptr->shield_deflect /= 3;
		p_ptr->weapon_parry /= 3;
	} else if (p_ptr->stun) {
		p_ptr->shield_deflect /= 2;
		p_ptr->weapon_parry /= 2;
	}



	/* Redraw plusses to hit/damage if necessary */
	if (p_ptr->dis_to_h != old_dis_to_h || p_ptr->dis_to_d != old_dis_to_d ||
	    p_ptr->to_h_melee != old_to_h_melee || p_ptr->to_d_melee != old_to_d_melee) {
		/* Redraw plusses */
		p_ptr->redraw |= (PR_PLUSSES);
	}

	/* Affect Skill -- stealth (bonus one) */
	p_ptr->skill_stl += 1; /* <- ??? */

	/* Affect Skill -- disarming (DEX and INT) */
	p_ptr->skill_dis += adj_dex_dis[p_ptr->stat_ind[A_DEX]];
	p_ptr->skill_dis += adj_int_dis[p_ptr->stat_ind[A_INT]];

	/* Affect Skill -- magic devices (INT) */
	p_ptr->skill_dev += get_skill_scale(p_ptr, SKILL_DEVICE, 20);

	/* Affect Skill -- saving throw (WIS) */
	p_ptr->skill_sav += adj_wis_sav[p_ptr->stat_ind[A_WIS]];

	/* Affect Skill -- digging (STR) */
	p_ptr->skill_dig += adj_str_dig[p_ptr->stat_ind[A_STR]];

	/* Affect Skill -- disarming (Level, by Class) */
	//p_ptr->skill_dis += (p_ptr->cp_ptr->x_dis * get_skill(p_ptr, SKILL_DISARM) / 10);
	p_ptr->skill_dis += (p_ptr->cp_ptr->x_dis * get_skill(p_ptr, SKILL_TRAPPING) / 10);

	/* Affect Skill -- magic devices (Level, by Class) */
	p_ptr->skill_dev += (p_ptr->cp_ptr->x_dev * get_skill(p_ptr, SKILL_DEVICE) / 10);
	p_ptr->skill_dev += adj_int_dev[p_ptr->stat_ind[A_INT]];

	/* Affect Skill -- saving throw (Level, by Class) */
	p_ptr->skill_sav += (p_ptr->cp_ptr->x_sav * p_ptr->lev / 10);

	/* Affect Skill -- stealth (Level, by Class) */
	p_ptr->skill_stl += (get_skill_scale(p_ptr, SKILL_STEALTH, p_ptr->cp_ptr->x_stl * 5)) + get_skill_scale(p_ptr, SKILL_STEALTH, 25);
	csheet_boni[14].slth += (get_skill_scale(p_ptr, SKILL_STEALTH, p_ptr->cp_ptr->x_stl * 5)) + get_skill_scale(p_ptr, SKILL_STEALTH, 25);
	
	/* Affect Skill -- search ability (Level, by Class) */
	p_ptr->skill_srh += (get_skill_scale(p_ptr, SKILL_SNEAKINESS, p_ptr->cp_ptr->x_srh * 2));// + get_skill(p_ptr, SKILL_SNEAKINESS);

	/* Affect Skill -- search frequency (Level, by Class) */
	p_ptr->skill_fos += (get_skill_scale(p_ptr, SKILL_SNEAKINESS, p_ptr->cp_ptr->x_fos * 2));// + get_skill(p_ptr, SKILL_SNEAKINESS);

	/* Affect Skill -- combat (normal) (Level, by Class) */
        p_ptr->skill_thn += (p_ptr->cp_ptr->x_thn * (((2 * get_skill(p_ptr, SKILL_MASTERY)) + (1 * get_skill(p_ptr, SKILL_COMBAT))) / 10) / 10);

	/* Affect Skill -- combat (shooting) (Level, by Class) */
	//p_ptr->skill_thb += (p_ptr->cp_ptr->x_thb * (((2 * get_skill(p_ptr, SKILL_ARCHERY)) + (1 * get_skill(p_ptr, SKILL_COMBAT))) / 10) / 10);
	p_ptr->skill_thb += (p_ptr->cp_ptr->x_thb * (((get_skill(p_ptr, SKILL_ARCHERY) + get_skill(p_ptr, get_archery_skill(p_ptr))) + (1 * get_skill(p_ptr, SKILL_COMBAT))) / 10) / 10);

	/* Affect Skill -- combat (throwing) (Level, by Class) */
	p_ptr->skill_tht += (p_ptr->cp_ptr->x_thb * get_skill_scale(p_ptr, SKILL_COMBAT, 10));



	/* Knowledge in magic schools can give permanent extra boni */
	/* - SKILL_EARTH gives resistance in earthquake() */
	if (get_skill(p_ptr, SKILL_EARTH) >= 40) { p_ptr->resist_shard = TRUE; csheet_boni[14].cb[2] |= CB3_RSHRD; }
	if (get_skill(p_ptr, SKILL_AIR) >= 30) { p_ptr->feather_fall = TRUE; csheet_boni[14].cb[5] |= CB6_RFFAL; }
	if (get_skill(p_ptr, SKILL_AIR) >= 40) { p_ptr->resist_pois = TRUE; csheet_boni[14].cb[1] |= CB2_RPOIS; }
	if (get_skill(p_ptr, SKILL_AIR) >= 50) {
		p_ptr->levitate = TRUE; csheet_boni[14].cb[5] |= CB6_RLVTN;
		p_ptr->feather_fall = TRUE; csheet_boni[14].cb[5] |= CB6_RFFAL;
	}
	if (get_skill(p_ptr, SKILL_WATER) >= 30) { p_ptr->can_swim = TRUE; csheet_boni[14].cb[12] |= CB13_XSWIM; }
	if (get_skill(p_ptr, SKILL_WATER) >= 40) { p_ptr->resist_water = TRUE; csheet_boni[14].cb[3] |= CB4_RWATR; }
	if (get_skill(p_ptr, SKILL_WATER) >= 50) { p_ptr->immune_water = TRUE; csheet_boni[14].cb[3] |= CB4_IWATR; }
	if (get_skill(p_ptr, SKILL_FIRE) >= 30) { p_ptr->resist_fire = TRUE; csheet_boni[14].cb[0] |= CB1_RFIRE; }
	if (get_skill(p_ptr, SKILL_FIRE) >= 50) { p_ptr->immune_fire = TRUE; csheet_boni[14].cb[0] |= CB1_IFIRE; }
	if (get_skill(p_ptr, SKILL_MANA) >= 40) { p_ptr->resist_mana = TRUE; csheet_boni[14].cb[3] |= CB4_RMANA; }
	if (get_skill(p_ptr, SKILL_CONVEYANCE) >= 40) { p_ptr->res_tele = TRUE; csheet_boni[14].cb[4] |= CB5_RTELE; }
	if (get_skill(p_ptr, SKILL_DIVINATION) >= 50) { p_ptr->auto_id = TRUE; csheet_boni[14].cb[6] |= CB7_RAUID; }
	if (get_skill(p_ptr, SKILL_NATURE) >= 30) { p_ptr->regenerate = TRUE; csheet_boni[14].cb[5] |= CB6_RRGHP; }
	if (get_skill(p_ptr, SKILL_NATURE) >= 30) { p_ptr->pass_trees = TRUE; csheet_boni[14].cb[12] |= CB13_XTREE; }
	if (get_skill(p_ptr, SKILL_NATURE) >= 30) { p_ptr->can_swim = TRUE; csheet_boni[14].cb[12] |= CB13_XSWIM; }
	if (get_skill(p_ptr, SKILL_NATURE) >= 40) { p_ptr->resist_pois = TRUE; csheet_boni[14].cb[1] |= CB2_RPOIS; }
	/* - SKILL_MIND also helps to reduce hallucination time in set_image() */
	if (get_skill(p_ptr, SKILL_MIND) >= 40 && !p_ptr->reduce_insanity) { p_ptr->reduce_insanity = 1; csheet_boni[14].cb[3] |= CB4_RMIND; }
	if (get_skill(p_ptr, SKILL_MIND) >= 50) { p_ptr->reduce_insanity = 2; csheet_boni[14].cb[4] |= CB5_XMIND; }
	if (get_skill(p_ptr, SKILL_TEMPORAL) >= 50) { p_ptr->resist_time = TRUE; csheet_boni[14].cb[3] |= CB4_RTIME; }
	if (get_skill(p_ptr, SKILL_UDUN) >= 40) { p_ptr->hold_life = TRUE; csheet_boni[14].cb[5] |= CB6_RLIFE; }
	if (get_skill(p_ptr, SKILL_META) >= 20) p_ptr->skill_sav += get_skill(p_ptr, SKILL_META) - 20;
	/* - SKILL_HOFFENSE gives slay mods in brand/slay function tot_dam_aux() */
	/* - SKILL_HDEFENSE gives auto protection-from-evil */
	//if (get_skill(p_ptr, SKILL_HDEFENSE) >= 40) { p_ptr->resist_lite = TRUE; p_ptr->resist_dark = TRUE; }
	/* - SKILL_HCURING gives extra high regeneration in regen function, and reduces various effects */
	//if (get_skill(p_ptr, SKILL_HCURING) >= 50 && !p_ptr->reduce_insanity) p_ptr->reduce_insanity = 1;
	/* - SKILL_HSUPPORT renders DG/TY_CURSE effectless and prevents hunger */

	if (get_skill(p_ptr, SKILL_HSUPPORT) >= 50) csheet_boni[14].cb[6] |= CB7_IFOOD;

	/* slay/brand boni check here... */
	if (get_skill(p_ptr, SKILL_HOFFENSE) >= 50) csheet_boni[14].cb[9] |= CB10_SEVIL;
	if (get_skill(p_ptr, SKILL_HOFFENSE) >= 40) csheet_boni[14].cb[8] |= CB9_SDEMN;
	if (get_skill(p_ptr, SKILL_HOFFENSE) >= 30) csheet_boni[14].cb[9] |= CB10_SUNDD;
	//prob: it's melee only! if (get_skill(p_ptr, SKILL_HCURING) >= 50) csheet_boni[14].cb[9] |= CB10_SUNDD;

#ifdef ENABLE_OCCULT /* Occult */
	/* Should Occult schools really give boni? */
	if (get_skill(p_ptr, SKILL_OSPIRIT) >= 40) csheet_boni[14].cb[9] |= CB10_SUNDD;
	if (get_skill(p_ptr, SKILL_OSHADOW) >= 30) {
		p_ptr->resist_dark = TRUE; csheet_boni[14].cb[2] |= CB3_RDARK;
		/* Stealth bonus: */
		p_ptr->skill_stl += (get_skill(p_ptr, SKILL_OSHADOW) - 30) / 5;
		csheet_boni[14].slth += (get_skill(p_ptr, SKILL_OSHADOW) - 30) / 5;
	}
 #if 0
	if (get_skill(p_ptr, SKILL_OSHADOW) >= 45 && get_skill(p_ptr, SKILL_HDEFENSE) >= 45) {
		p_ptr->resist_chaos = TRUE; csheet_boni[14].cb[3] |= CB4_RCHAO;
	}
 #endif
	if (get_skill(p_ptr, SKILL_OSPIRIT) >= 30) { p_ptr->hold_life = TRUE; csheet_boni[14].cb[5] |= CB6_RLIFE; }
 #ifdef ENABLE_OHERETICISM
	if (get_skill(p_ptr, SKILL_OHERETICISM) >= 30) { p_ptr->resist_fire = TRUE; csheet_boni[14].cb[0] |= CB1_RFIRE; }
	if (get_skill(p_ptr, SKILL_OHERETICISM) >= 45) { p_ptr->resist_chaos = TRUE; csheet_boni[14].cb[3] |= CB4_RCHAO; }
 #endif
#endif

	if (get_skill(p_ptr, SKILL_NECROMANCY) >= 50) { p_ptr->keep_life = TRUE; csheet_boni[14].cb[13] |= CB14_ILIFE; }

	/* Fear Resistance from aura */
	if (get_skill(p_ptr, SKILL_AURA_FEAR) >= 20)
		{ p_ptr->resist_fear = TRUE; csheet_boni[14].cb[4] |= CB5_RFEAR; }



	/* Take note when "heavy bow" changes */
	if (p_ptr->old_heavy_shoot != p_ptr->heavy_shoot) {
		/* Message */
		if (p_ptr->heavy_shoot)
			msg_print(Ind, "\377oYou have trouble wielding such a heavy bow.");
		else if (p_ptr->inventory[INVEN_BOW].k_idx)
			msg_print(Ind, "\377gYou have no trouble wielding your bow.");
		else
			msg_print(Ind, "\377gYou feel relieved to put down your heavy bow.");

		/* Save it */
		p_ptr->old_heavy_shoot = p_ptr->heavy_shoot;
	}

	/* Take note when "heavy weapon" changes */
	if (p_ptr->old_heavy_wield != p_ptr->heavy_wield) {
		/* Message */
		if (p_ptr->heavy_wield)
			msg_print(Ind, "\377oYou have trouble wielding such a heavy weapon.");
		else if (p_ptr->inventory[INVEN_WIELD].k_idx || /* dual-wield */
		    (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD))
			msg_print(Ind, "\377gYou have no trouble wielding your weapon.");
		else
			msg_print(Ind, "\377gYou feel relieved to put down your heavy weapon.");

		/* Save it */
		p_ptr->old_heavy_wield = p_ptr->heavy_wield;
	}

	/* Take note when "heavy shield" changes */
	if (p_ptr->old_heavy_shield != p_ptr->heavy_shield) {
		/* Message */
		if (p_ptr->heavy_shield)
			msg_print(Ind, "\377oYou have trouble wielding such a heavy shield.");
		else if (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD) /* dual-wielders */
			msg_print(Ind, "\377gYou have no trouble wielding your shield.");
		else
			msg_print(Ind, "\377gYou feel relieved to put down your heavy shield.");

		/* Save it */
		p_ptr->old_heavy_shield = p_ptr->heavy_shield;
	}

	/* Take note when "illegal weapon" changes */
	if (p_ptr->old_icky_wield != p_ptr->icky_wield) {
		/* Message */
		if (p_ptr->icky_wield)
			msg_print(Ind, "\377oYou do not feel comfortable with your weapon.");
		else if (p_ptr->inventory[INVEN_WIELD].k_idx || /* dual-wield */
		    (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD))
			msg_print(Ind, "\377gYou feel comfortable with your weapon.");
		else
			msg_print(Ind, "\377gYou feel more comfortable after removing your weapon.");

		/* Save it */
		p_ptr->old_icky_wield = p_ptr->icky_wield;
	}

	/* Take note when "illegal weapon" changes */
	if (p_ptr->old_awkward_wield != p_ptr->awkward_wield) {
		/* Message */
		if (p_ptr->awkward_wield)
			msg_print(Ind, "\377yUsing this large weapon together with a shield makes fighting harder.");
		else if (p_ptr->inventory[INVEN_WIELD].k_idx)
			msg_print(Ind, "\377gYou feel comfortable with your weapon.");
		else if (!p_ptr->inventory[INVEN_ARM].k_idx)
			msg_print(Ind, "\377gYou feel more dexterous after removing your shield.");

		/* Save it */
		p_ptr->old_awkward_wield = p_ptr->awkward_wield;
	}

	if (p_ptr->old_easy_wield != p_ptr->easy_wield) {
		/* suppress message if we're heavy-wielding */
		if (!p_ptr->heavy_wield) {
			/* Message */
			if (p_ptr->easy_wield) {
				if (get_skill(p_ptr, SKILL_DUAL)) /* dual-wield */
					msg_print(Ind, "\377wWithout shield or secondary weapon, your weapon feels especially easy to swing.");
				else
					msg_print(Ind, "\377wWithout shield, your weapon feels especially easy to swing.");
			} else if (p_ptr->inventory[INVEN_WIELD].k_idx) {
				/*msg_print(Ind, "\377wWith shield, your weapon feels normally comfortable.");
				this above line does also show if you don't equip a shield but just switch from a
				may_2h to a normal weapon, hence confusing. */
				msg_print(Ind, "\377wYour weapon feels comfortable as usual.");
			}
		}
		/* Save it */
		p_ptr->old_easy_wield = p_ptr->easy_wield;
	}

	/* Take note when "illegal weapon" changes */
	if (p_ptr->old_awkward_shoot != p_ptr->awkward_shoot) {
		/* Message */
		if (p_ptr->awkward_shoot) {
			if (p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)
				msg_print(Ind, "\377yYou find it harder to aim your ranged weapon while wielding a shield.");
			else /* maybe leave awkward_shoot at FALSE if secondary slot isn't a shield! */
				msg_print(Ind, "\377yYou find it harder to aim your ranged weapon while dual-wielding weapons.");
		} else if (p_ptr->inventory[INVEN_BOW].k_idx)
			msg_print(Ind, "\377gYou find it easier to aim your ranged weapon.");

		/* Save it */
		p_ptr->old_awkward_shoot = p_ptr->awkward_shoot;
	}

#if 0 /* doesn't work well because it's mostly a continuous increase from hardly-noticing to massive-drowning */
	/* Swimming-indicator (maybe a bit too cheezy) */
	p_ptr->heavy_swim = FALSE;
	//if ((!p_ptr->tim_wraith) && (!p_ptr->levitate) && (!p_ptr->can_swim)) { --actually don't count these in
	if (!p_ptr->can_swim) {
		if (!(p_ptr->body_monster) || (
		    !(r_info[p_ptr->body_monster].flags7 &
		    (RF7_AQUATIC | RF7_CAN_SWIM)))) {
			int swim = get_skill_scale(p_ptr, SKILL_SWIM, 4500);

			/* temporary abs weight calc */
			if (p_ptr->wt + p_ptr->total_weight / 10 > 170 + swim * 2) {
				long factor = (p_ptr->wt + p_ptr->total_weight / 10) - 150 - swim * 2;

				if (factor >= 20) p_ptr->heavy_swim = TRUE;
			}
		}
	}
	if (p_ptr->old_heavy_swim != p_ptr->heavy_swim) {
		if (p_ptr->heavy_swim)
			msg_print(Ind, "\377yYou're too heavy to swim.");
		else
			msg_print(Ind, "\377gYou're light enough to swim.");
	}
	p_ptr->old_heavy_swim = p_ptr->heavy_swim;
#endif



	/* resistance to fire cancel sensibility to fire */
	if (p_ptr->resist_fire || p_ptr->oppose_fire || p_ptr->immune_fire)
		p_ptr->suscep_fire = FALSE;
	/* resistance to cold cancel sensibility to cold */
	if (p_ptr->resist_cold || p_ptr->oppose_cold || p_ptr->immune_cold)
		p_ptr->suscep_cold = FALSE;
	/* resistance to electricity cancel sensibility to fire */
	if (p_ptr->resist_elec || p_ptr->oppose_elec || p_ptr->immune_elec)
		p_ptr->suscep_elec = FALSE;
	/* resistance to acid cancel sensibility to fire */
	if (p_ptr->resist_acid || p_ptr->oppose_acid || p_ptr->immune_acid)
		p_ptr->suscep_acid = FALSE;
	/* resistance to light cancels sensibility to light */
	if (p_ptr->resist_lite) p_ptr->suscep_lite = FALSE;



#if 0 /* in the making.. */
	/* reduce speeds, so high-level players can duel each other even in Bree - C. Blue */
	if (p_ptr->blood_bond && (Ind2 = find_player(p_ptr->blood_bond))) {
		int factor1 = 10, factor2 = 10, reduction;
		if (p_ptr->pspeed < 110) {
			factor2 = (factor2 * (10 + (110 - p_ptr->pspeed))) / 10;
		} else {
			factor1 = (factor1 * (10 + (p_ptr->pspeed - 110))) / 10;
		}
		if (Players[Ind2]->pspeed < 110) {
			factor1 = (factor1 * (10 + (110 - Players[Ind2]->pspeed))) / 10;
		} else {
			factor2 = (factor2 * (10 + (Players[Ind2]->pspeed - 110))) / 10;
		}
		if (factor1 >= factor2) { /* player 1 is faster or equal speed */
			if (p_ptr->pspeed > 120) /* top (cur atm) speed for moving during blood bond */
				p_ptr->pspeed = 120;
				//reduction = p_ptr->pspeed - 120;
			if (factor1 >= (p_ptr->pspeed - 110) + 10) {
				factor1 = (factor * 10) / (p_ptr->pspeed - 110) + 10;
				Players[Ind2]->pspeed = 110 - factor1 + 10;
			}
		} else { /* player 2 is faster or equal speed */
			if (Players[Ind2]->pspeed > 120) /* top (cur atm) speed for moving during blood bond */
				Players[Ind2]->pspeed = 120;
				//reduction = Players[Ind2]->pspeed - 120;
		}
	}
#endif

	/* VAMPIRIC_MIST speed specialty: creeping (slowish) */
	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST && p_ptr->pspeed > 100)
		p_ptr->pspeed = 100 + ((p_ptr->pspeed - 100) * 2) / 3;

	/* Extract the current weight (in tenth pounds) */
	w = p_ptr->total_weight;

	/* Extract the "weight limit" (in tenth pounds) */
	i = weight_limit(Ind);

	/* XXX XXX XXX Apply "encumbrance" from weight */
	if (w > i / 2) {
		/* protect pspeed from uberflow O_o */
		//if (w > 61500) p_ptr->pspeed = 10; /* roughly ;-p */
		if (w > 70000) p_ptr->pspeed = 10; /* roughly ;-p */
		else p_ptr->pspeed -= ((w - (i / 2)) / (i / 10));
	}

	/* Display the speed (if needed) */
	if (p_ptr->pspeed != old_speed) p_ptr->redraw |= (PR_SPEED);



	/* swapping in AUTO_ID items will instantly ID inventory and equipment.
	   Careful, we're called from birth->player_setup->player_night maybe,
	   when we aren't READY|PLAYING yet: */
	if (p_ptr->auto_id && !old_auto_id && !suppress_boni && logged_in) {
#if 0 /* currently doesn't work ok with client-side auto-inscriptions */
		for (i = 0; i < INVEN_TOTAL; i++) {
			o_ptr = &p_ptr->inventory[i];
			object_aware(Ind, o_ptr);
			object_known(o_ptr);
		}
#else /* why not just this, simply.. :) */
		identify_pack(Ind);
#endif

#if 0 /* moved to client-side, clean! */
		/* hack: trigger client-side auto-inscriptions for convenience,
		   if it isn't due anyway.  */
		if (!p_ptr->inventory_changes) Send_inventory_revision(Ind);
#endif
	}



/* -------------------- Limits -------------------- */

	/* Make sure we don't get negative stealth from monster body malus */
	if (p_ptr->skill_stl < 0) p_ptr->skill_stl = 0;

	/* Antimagic characters cannot use magic devices at all.
	   Note: This isn't technically needed, it's just for visuals. */
	if (get_skill(p_ptr, SKILL_ANTIMAGIC)) p_ptr->skill_dev = 0;

#ifdef FUNSERVER
	/* Limit AC?.. */
	if (!is_admin(p_ptr)) {
		if ((p_ptr->to_a > 400) && (!p_ptr->total_winner)) p_ptr->to_a = 400; /* Lebohaum / Illuvatar bonus */
		if ((p_ptr->to_a > 300) && (p_ptr->total_winner)) p_ptr->to_a = 300;
		if (p_ptr->ac > 100) p_ptr->ac = 100;
	}
#endif

	if (((l_ptr && (l_ptr->flags2 & LF2_NO_SPEED)) ||
	    (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_SPEED)))
	    && p_ptr->pspeed > 110 && !p_ptr->admin_dm)
		p_ptr->pspeed = 110;

	if (((l_ptr && (l_ptr->flags2 & LF2_NO_RES_HEAL)) ||
	    (in_sector00(&p_ptr->wpos) && (sector00flags2 & LF2_NO_RES_HEAL)))
	    && !p_ptr->admin_dm) {
		p_ptr->resist_acid = FALSE;
		p_ptr->resist_elec = FALSE;
		p_ptr->resist_fire = FALSE;
		p_ptr->resist_cold = FALSE;
		p_ptr->resist_pois = FALSE;
		p_ptr->resist_conf = FALSE;
		p_ptr->resist_sound = FALSE;
		p_ptr->resist_lite = FALSE;
		p_ptr->resist_dark = FALSE;
		p_ptr->resist_chaos = FALSE;
		p_ptr->resist_disen = FALSE;
		p_ptr->resist_shard = FALSE;
		p_ptr->resist_nexus = FALSE;
		p_ptr->resist_neth = FALSE;
		p_ptr->resist_water = FALSE;
		p_ptr->resist_time = FALSE;
		p_ptr->resist_mana = FALSE;
		p_ptr->immune_acid = FALSE;
		p_ptr->immune_elec = FALSE;
		p_ptr->immune_fire = FALSE;
		p_ptr->immune_cold = FALSE;
		p_ptr->immune_poison = FALSE;
		p_ptr->immune_water = FALSE;
		p_ptr->immune_neth = FALSE;
	}

	/* Limit mana capacity bonus */
	if ((p_ptr->to_m > 300) && !is_admin(p_ptr)) p_ptr->to_m = 300;

	/* Limit Skill -- stealth from 0 to 30 */
	if (p_ptr->skill_stl > 30) p_ptr->skill_stl = 30;
	if (p_ptr->skill_stl < 0) p_ptr->skill_stl = 0;
	if (p_ptr->aggravate) p_ptr->skill_stl = 0;

	/* Limit Skill -- digging from 1 up */
	if (p_ptr->skill_dig < 1) p_ptr->skill_dig = 1;

	if ((p_ptr->anti_magic) && (p_ptr->skill_sav < 95)) p_ptr->skill_sav = 95;

	/* Limit Skill -- saving throw upto 95 */
	if (p_ptr->skill_sav > 95) p_ptr->skill_sav = 95;

	/* Limit critical hits bonus */
	if ((p_ptr->xtra_crit > 50) && !is_admin(p_ptr)) p_ptr->xtra_crit = 50;

	/* Limit speed penalty from total_weight */
	if ((p_ptr->pspeed < 10) && !is_admin(p_ptr)) p_ptr->pspeed = 10;
	if (p_ptr->pspeed < 100 && is_admin(p_ptr)) p_ptr->pspeed = 100;

	/* Limit speed */
	if ((p_ptr->pspeed > 210) && !is_admin(p_ptr)) p_ptr->pspeed = 210;

	/* Limit blows per round (just paranoia^^) */
	if (p_ptr->num_blow < 0) p_ptr->num_blow = 0;
	if ((p_ptr->num_blow > 20) && !is_admin(p_ptr)) p_ptr->num_blow = 20;
	/* ghost-dive limit to not destroy the real gameplay */
	if (p_ptr->ghost && !is_admin(p_ptr)) p_ptr->num_blow = (p_ptr->lev + 15) / 16;



	/* Determine colour of our light radius */
	old_lite_type = p_ptr->lite_type;
	if (p_ptr->cur_vlite > p_ptr->cur_lite) p_ptr->lite_type = 1; /* vampiric */
	else if (lite_inc_white > lite_inc_norm) p_ptr->lite_type = 2; /* artificial */
	else p_ptr->lite_type = 0; /* normal, fiery */
	if (old_lite_type != p_ptr->lite_type) {
		if (p_ptr->px /* calc_boni() is called once on birth, where player isn't positioned yet. */
		    && !suppress_boni) {
			forget_lite(Ind);
			update_lite(Ind);
		}
		old_lite_type = p_ptr->lite_type; /* erm, this isnt needed? */
	}



	/* XXX - Always resend skills */
	p_ptr->redraw |= (PR_SKILLS);
	/* also redraw encumberment status line */
	p_ptr->redraw |= (PR_ENCUMBERMENT);
	if (is_newer_than(&p_ptr->version, 4, 4, 8, 4, 0, 0))
		/* Redraw BpR */
		p_ptr->redraw |= PR_BPR_WRAITH;

	/* Send all the columns */
	if (is_newer_than(&p_ptr->version, 4, 5, 3, 2, 0, 0) && logged_in)
		for (i = 0; i < 15; i++) {
			f1 = f2 = f3 = f4 = f5 = f6 = esp = 0x0;
			if (csheet_boni[i].cb[11] & CB12_XHIDD) {
				/* Wipe the boni column data */
				csheet_boni[i].i = i;
				csheet_boni[i].spd = 0;
				csheet_boni[i].slth = 0;
				csheet_boni[i].srch = 0;
				csheet_boni[i].infr = 0;
				csheet_boni[i].lite = 0;
				csheet_boni[i].dig = 0;
				csheet_boni[i].blow = 0;
				csheet_boni[i].crit = 0;
				csheet_boni[i].shot = 0;
				csheet_boni[i].migh = 0;
				csheet_boni[i].mxhp = 0;
				csheet_boni[i].mxmp = 0;
				csheet_boni[i].luck = 0;
				csheet_boni[i].pstr = 0;
				csheet_boni[i].pint = 0;
				csheet_boni[i].pwis = 0;
				csheet_boni[i].pdex = 0;
				csheet_boni[i].pcon = 0;
				csheet_boni[i].pchr = 0;
				csheet_boni[i].amfi = 0;
				csheet_boni[i].sigl = 0;
				/* Clear the byte flags */
				for (jj = 0; jj < 13; jj++)
					csheet_boni[i].cb[jj] = 0;
				//Actually, keep the item representation :)
				//csheet_boni[i].color = TERM_DARK;
				//csheet_boni[i].symbol = ' '; //Empty item / form slot.
				
				 //Actually give the basic item data instead of hiding it all. See object1.cf
				bool can_have_hidden_powers = TRUE;
#ifdef NEW_ID_SCREEN
				can_have_hidden_powers = FALSE;
//				bool not_identified_at_all = TRUE;
				if (i != 14) {
					o_ptr = &p_ptr->inventory[i + INVEN_WIELD];
					k_ptr = &k_info[o_ptr->k_idx];
					/* Build the flags */
					//object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
					if (object_aware_p(Ind, o_ptr)) {
						f1 = k_info[o_ptr->k_idx].flags1;
						f2 = k_info[o_ptr->k_idx].flags2;
						f3 = k_info[o_ptr->k_idx].flags3;
						f4 = k_info[o_ptr->k_idx].flags4;
						f5 = k_info[o_ptr->k_idx].flags5;
						f6 = k_info[o_ptr->k_idx].flags6;
						esp = k_info[o_ptr->k_idx].esp;
						/* hack: granted pval-abilities */
						if (o_ptr->tval == TV_MSTAFF && o_ptr->pval) f1 |= TR1_MANA;
						if (o_ptr->name1) can_have_hidden_powers = TRUE; //artifact
					} else can_have_hidden_powers = TRUE; //unknown jewelry type
					/* Assume we must *id* (just once) to learn sigil powers */
					if (o_ptr->sigil && !object_fully_known_p(Ind, o_ptr)) can_have_hidden_powers = TRUE;
					ego_item_type *e_ptr;
					if (object_known_p(Ind, o_ptr)) {
						if (o_ptr->name2) {
							e_ptr = &e_info[o_ptr->name2];
							for (j = 0; j < 5; j++) {
								if (e_ptr->rar[j] == 0) continue;
								/* hack: can *identifying* actually make a difference at all? */

								/* any non-trivial (on the object name itself visible) abilities? */
								if ((e_ptr->fego1[j] & ETR1_EASYKNOW_MASK) ||
								    (e_ptr->fego2[j] & ETR2_EASYKNOW_MASK) ||
								    /* random ego mods (R_xxx)? */
								    (e_ptr->esp[j] & ESP_R_MASK)) {
									can_have_hidden_powers = TRUE;
								}

								/* random base mods? */
								if (e_ptr->rar[j] != 100) {
									if (e_ptr->flags1[j] | e_ptr->flags2[j] | e_ptr->flags3[j] |
									    e_ptr->flags4[j] | e_ptr->flags5[j] | e_ptr->flags6[j] |
									    e_ptr->esp[j]) {
										can_have_hidden_powers = TRUE;
										continue;
									}
								}

								/* fixed base mods, ie which we absolutely know will be on the item even without *id*ing */
								f1 |= e_ptr->flags1[j];
								f2 |= e_ptr->flags2[j];
								f3 |= e_ptr->flags3[j];
								f4 |= e_ptr->flags4[j];
								f5 |= e_ptr->flags5[j];
								f6 |= e_ptr->flags6[j];
								esp |= e_ptr->esp[j]; /* & ~ESP_R_MASK -- not required */
							}
						}
						if (o_ptr->name2b) {
							e_ptr = &e_info[o_ptr->name2b];
							for (j = 0; j < 5; j++) {
								if (e_ptr->rar[j] == 0) continue;
								/* hack: can *identifying* actually make a difference at all? */

								/* any non-trivial (on the object name itself visible) abilities? */
								if ((e_ptr->fego1[j] & ETR1_EASYKNOW_MASK) ||
								    (e_ptr->fego2[j] & ETR2_EASYKNOW_MASK) ||
								    /* random ego mods (R_xxx)? */
								    (e_ptr->esp[j] & ESP_R_MASK)) {
									can_have_hidden_powers = TRUE;
								}

								/* random base mods? */
								if (e_ptr->rar[j] != 100) {
									if (e_ptr->flags1[j] | e_ptr->flags2[j] | e_ptr->flags3[j] |
									    e_ptr->flags4[j] | e_ptr->flags5[j] | e_ptr->flags6[j] |
									    e_ptr->esp[j]) {
										can_have_hidden_powers = TRUE;
										continue;
									}
								}

								/* fixed base mods, ie which we absolutely know will be on the item even without *id*ing */
								f1 |= e_ptr->flags1[j];
								f2 |= e_ptr->flags2[j];
								f3 |= e_ptr->flags3[j];
								f4 |= e_ptr->flags4[j];
								f5 |= e_ptr->flags5[j];
								f6 |= e_ptr->flags6[j];
								esp |= e_ptr->esp[j]; /* & ~ESP_R_MASK -- not required */
							}
						}
					} else can_have_hidden_powers = TRUE; //not identified

					//Translate item flags to PKT data
					if (object_aware_p(Ind, o_ptr)) { //must know base item type to see anything
						/* Table A - Skip impossible flags for items... */
						if (f2 & TR2_RES_FIRE) csheet_boni[i].cb[0] |= CB1_RFIRE;
						if (f2 & TR2_IM_FIRE) csheet_boni[i].cb[0] |= CB1_IFIRE;
						if (f2 & TR2_RES_COLD) csheet_boni[i].cb[0] |= CB1_RCOLD;
						if (f2 & TR2_IM_COLD) csheet_boni[i].cb[0] |= CB1_ICOLD;
						if (f2 & TR2_RES_ELEC) csheet_boni[i].cb[0] |= CB1_RELEC;
						if (f2 & TR2_IM_ELEC) csheet_boni[i].cb[1] |= CB2_IELEC;
						if (f2 & TR2_RES_ACID) csheet_boni[i].cb[1] |= CB2_RACID;
						if (f2 & TR2_IM_ACID) csheet_boni[i].cb[1] |= CB2_IACID;
						if (f2 & TR2_RES_POIS) csheet_boni[i].cb[1] |= CB2_RPOIS;
						if (f5 & TR5_IM_POISON) csheet_boni[i].cb[1] |= CB2_IPOIS;
						if (f2 & TR2_RES_BLIND) csheet_boni[i].cb[1] |= CB2_RBLND;
						if (f2 & TR2_RES_LITE) csheet_boni[i].cb[2] |= CB3_RLITE;
						if (f2 & TR2_RES_DARK) csheet_boni[i].cb[2] |= CB3_RDARK;
						if (f2 & TR2_RES_SOUND) csheet_boni[i].cb[2] |= CB3_RSOUN;
						if (f2 & TR2_RES_SHARDS) csheet_boni[i].cb[2] |= CB3_RSHRD;
						if (f2 & TR2_RES_NEXUS) csheet_boni[i].cb[2] |= CB3_RNEXU;
						if (f2 & TR2_RES_NETHER) csheet_boni[i].cb[2] |= CB3_RNETH;
						if (f4 & TR4_IM_NETHER) csheet_boni[i].cb[2] |= CB3_INETH; //ring of phasing
						if (f2 & TR2_RES_CONF) csheet_boni[i].cb[3] |= CB4_RCONF;
						if (f2 & TR2_RES_CHAOS) csheet_boni[i].cb[3] |= CB4_RCHAO;
						if (f2 & TR2_RES_DISEN) csheet_boni[i].cb[3] |= CB4_RDISE;
						if (f5 & TR5_RES_WATER) csheet_boni[i].cb[3] |= CB4_RWATR;
						if (f5 & TR5_IM_WATER) csheet_boni[i].cb[3] |= CB4_IWATR; //ocean soul
						if (f5 & TR5_RES_TIME) csheet_boni[i].cb[3] |= CB4_RTIME;
						if (f5 & TR5_RES_MANA) csheet_boni[i].cb[3] |= CB4_RMANA;
						
						/* Table B */
						if (f2 & TR2_RES_FEAR) csheet_boni[i].cb[4] |= CB5_RFEAR;
						if (f2 & TR2_FREE_ACT) csheet_boni[i].cb[4] |= CB5_RPARA;
						if (f3 & TR3_TELEPORT) {
							csheet_boni[i].cb[4] |= CB5_STELE;
#ifdef TOGGLE_TELE /* disabled since TELEPORT flag was removed from Thunderlord ego, the only real reason to have this */
							//inscription = (unsigned char *) quark_str(o_ptr->note);
							inscription = quark_str(p_ptr->inventory[i].note);
							/* check for a valid inscription */
							if ((inscription != NULL) && (!(o_ptr->ident & ID_CURSED))) {
								/* scan the inscription for .. */
								while (*inscription != '\0') {
									if (*inscription == '.') {
										inscription++;
										/* a valid .. has been located */
										if (*inscription == '.') {
											inscription++;
											csheet_boni[i].cb[4] &= (~CB5_STELE);
											break;
										}
									}
									inscription++;
								}
							}
#endif
						}
						if (f5 & TR5_RES_TELE) csheet_boni[i].cb[4] |= CB5_RTELE;
						if (f3 & TR3_NO_TELE) csheet_boni[i].cb[4] |= CB5_ITELE;
						if (f3 & TR3_SEE_INVIS) csheet_boni[i].cb[4] |= CB5_RSINV;
						if (f5 & TR5_INVIS) csheet_boni[i].cb[4] |= CB5_RINVS;
						if (f3 & TR3_FEATHER) csheet_boni[i].cb[5] |= CB6_RFFAL;
						if (f4 & TR4_LEVITATE) csheet_boni[i].cb[5] |= CB6_RLVTN;
						if (f4 & TR4_CLIMB) csheet_boni[i].cb[5] |= CB6_RCLMB; //climbing kit
						if (f2 & TR2_HOLD_LIFE) csheet_boni[i].cb[5] |= CB6_RLIFE;
						if (f3 & (TR3_WRAITH)) csheet_boni[i].cb[5] |= CB6_RWRTH;
						if (f5 & TR5_DRAIN_HP) csheet_boni[i].cb[5] |= CB6_SRGHP;
						if (f3 & TR3_REGEN) csheet_boni[i].cb[5] |= CB6_RRGHP;
						if (f5 & TR5_DRAIN_MANA) csheet_boni[i].cb[5] |= CB6_SRGMP;
						if (f5 & TR5_REGEN_MANA) csheet_boni[i].cb[6] |= CB7_RRGMP;
						if (f3 & TR3_SLOW_DIGEST) csheet_boni[i].cb[6] |= CB7_RFOOD;
						if (f1 & TR1_VAMPIRIC) csheet_boni[i].cb[6] |= CB7_RVAMP;
						if (f4 & TR4_AUTO_ID) csheet_boni[i].cb[6] |= CB7_RAUID;
						if (f5 & TR5_REFLECT) csheet_boni[i].cb[6] |= CB7_RREFL;
						if (f3 & TR3_NO_MAGIC) csheet_boni[i].cb[6] |= CB7_RAMSH;
						if (f3 & TR3_AGGRAVATE) csheet_boni[i].cb[6] |= CB7_RAGGR;
						
						/* Table C */
						if (object_known_p(Ind, o_ptr)) { //must be identified to see PVAL
							pval = o_ptr->pval;

							/*todo: build a_ptr for ego/art powers and mask out k1/k5 pval mask from it,
							  to get the -real- pvals (no more witans +8 stealth if +10 speed..) */
							if (o_ptr->name2) {
								artifact_type *a_ptr = ego_make(o_ptr);
								f1 &= ~(k_ptr->flags1 & TR1_PVAL_MASK & ~a_ptr->flags1);
								f5 &= ~(k_ptr->flags5 & TR5_PVAL_MASK & ~a_ptr->flags5);
							} else if (o_ptr->name1 == ART_RANDART) {
								artifact_type *a_ptr = randart_make(o_ptr);
								f1 &= ~(k_ptr->flags1 & TR1_PVAL_MASK & ~a_ptr->flags1);
								f5 &= ~(k_ptr->flags5 & TR5_PVAL_MASK & ~a_ptr->flags5);
							}

							if (f1 & TR1_SPEED) csheet_boni[i].spd += pval;
							if (f1 & TR1_STEALTH) csheet_boni[i].slth += pval;
							if (f1 & TR1_SEARCH) csheet_boni[i].srch += pval;
							if (f1 & TR1_INFRA) csheet_boni[i].infr += pval;
							if (f1 & TR1_TUNNEL) csheet_boni[i].dig += pval;
							if (f1 & TR1_BLOWS) csheet_boni[i].blow += pval;
							if (f5 & TR5_CRIT) csheet_boni[i].crit += pval;
							if (f5 & TR5_LUCK) csheet_boni[i].luck += pval;

							if (f1 & TR1_STR) csheet_boni[i].pstr += pval;
							if (f1 & TR1_INT) csheet_boni[i].pint += pval;
							if (f1 & TR1_WIS) csheet_boni[i].pwis += pval;
							if (f1 & TR1_DEX) csheet_boni[i].pdex += pval;
							if (f1 & TR1_CON) csheet_boni[i].pcon += pval;
							if (f1 & TR1_CHR) csheet_boni[i].pchr += pval;

							if (f1 & TR1_MANA) csheet_boni[i].mxmp += pval;
							if (f1 & TR1_LIFE) csheet_boni[i].mxhp += pval;

							{ //lite
								j = 0;
								if (f3 & TR3_LITE1) j++;
								if (f4 & TR4_LITE2) j += 2;
								if (f4 & TR4_LITE3) j += 3;
								csheet_boni[i].lite += j;
								if (!(f4 & TR4_FUEL_LITE)) csheet_boni[i].cb[12] |= CB13_XLITE;
							}

							if (f3 & TR3_XTRA_SHOTS) csheet_boni[i].shot++;
							if (f3 & TR3_XTRA_MIGHT) csheet_boni[i].migh++;
							if (f2 & TR2_SUST_STR) csheet_boni[i].cb[11] |= CB12_RSSTR;
							if (f2 & TR2_SUST_INT) csheet_boni[i].cb[11] |= CB12_RSINT;
							if (f2 & TR2_SUST_WIS) csheet_boni[i].cb[11] |= CB12_RSWIS;
							if (f2 & TR2_SUST_DEX) csheet_boni[i].cb[11] |= CB12_RSDEX;
							if (f2 & TR2_SUST_CON) csheet_boni[i].cb[11] |= CB12_RSCON;
							if (f2 & TR2_SUST_CHR) csheet_boni[i].cb[11] |= CB12_RSCHR;
							
							/* And now the base PVAL of the item... */
							if (k_ptr->flags1 & TR1_PVAL_MASK) {
								if (k_ptr->flags1 & TR1_STR) csheet_boni[i].pstr += o_ptr->bpval;
								if (k_ptr->flags1 & TR1_INT) csheet_boni[i].pint += o_ptr->bpval;
								if (k_ptr->flags1 & TR1_WIS) csheet_boni[i].pwis += o_ptr->bpval;
								if (k_ptr->flags1 & TR1_DEX) csheet_boni[i].pdex += o_ptr->bpval;
								if (k_ptr->flags1 & TR1_CON) csheet_boni[i].pcon += o_ptr->bpval;
								if (k_ptr->flags1 & TR1_CHR) csheet_boni[i].pchr += o_ptr->bpval;

								if (k_ptr->flags1 & TR1_STEALTH) csheet_boni[i].slth += o_ptr->bpval;
								if (k_ptr->flags1 & TR1_SEARCH) csheet_boni[i].srch += o_ptr->bpval;
								if (k_ptr->flags1 & TR1_INFRA) csheet_boni[i].infr += o_ptr->bpval;
								if (k_ptr->flags1 & TR1_TUNNEL) csheet_boni[i].dig += o_ptr->bpval;
								if (k_ptr->flags1 & TR1_SPEED) csheet_boni[i].spd += o_ptr->bpval;
								if (k_ptr->flags1 & TR1_BLOWS) csheet_boni[i].blow += o_ptr->bpval;
							}
							if (k_ptr->flags5 & TR5_PVAL_MASK) {
								if (f5 & TR5_LUCK) csheet_boni[i].luck += o_ptr->bpval;
								if (f5 & TR5_CRIT) csheet_boni[i].crit += o_ptr->bpval;
							}
						}
						
						/* Table D */
						if (f1 & TR1_SLAY_ANIMAL) csheet_boni[i].cb[7] |= CB8_SANIM;
						if (f1 & TR1_SLAY_EVIL) csheet_boni[i].cb[9] |= CB10_SEVIL;
						if (f1 & TR1_SLAY_UNDEAD) csheet_boni[i].cb[9] |= CB10_SUNDD;
						if (f1 & TR1_SLAY_DEMON) csheet_boni[i].cb[8] |= CB9_SDEMN;
						if (f1 & TR1_SLAY_ORC) csheet_boni[i].cb[7] |= CB8_SORCS;
						if (f1 & TR1_SLAY_TROLL) csheet_boni[i].cb[7] |= CB8_STROL;
						if (f1 & TR1_SLAY_GIANT) csheet_boni[i].cb[8] |= CB9_SGIAN;
						if (f1 & TR1_SLAY_DRAGON) csheet_boni[i].cb[8] |= CB9_SDRGN;
						if (f1 & TR1_KILL_DRAGON) csheet_boni[i].cb[8] |= CB9_KDRGN;
						if (f1 & TR1_KILL_DEMON) csheet_boni[i].cb[8] |= CB9_KDEMN;
						if (f1 & TR1_KILL_UNDEAD) csheet_boni[i].cb[9] |= CB10_KUNDD;
						if (esp & ESP_ALL) {
							csheet_boni[i].cb[7] |= (CB8_ESPID | CB8_EANIM | CB8_EORCS | CB8_ETROL | CB8_EGIAN);
							csheet_boni[i].cb[8] |= (CB9_EDRGN | CB9_EDEMN | CB9_EUNDD);
							csheet_boni[i].cb[9] |= (CB10_EEVIL | CB10_EDGRI | CB10_EGOOD | CB10_ENONL | CB10_EUNIQ);
						} else {
							if (esp & ESP_SPIDER) csheet_boni[i].cb[7] |= CB8_ESPID;
							if (esp & ESP_ANIMAL) csheet_boni[i].cb[7] |= CB8_EANIM;
							if (esp & ESP_ORC) csheet_boni[i].cb[7] |= CB8_EORCS;
							if (esp & ESP_TROLL) csheet_boni[i].cb[7] |= CB8_ETROL;
							if (esp & ESP_GIANT) csheet_boni[i].cb[7] |= CB8_EGIAN;
							if (esp & ESP_DRAGON) csheet_boni[i].cb[8] |= CB9_EDRGN;
							if (esp & ESP_DEMON) csheet_boni[i].cb[8] |= CB9_EDEMN;
							if (esp & ESP_UNDEAD) csheet_boni[i].cb[8] |= CB9_EUNDD;
							if (esp & ESP_EVIL) csheet_boni[i].cb[9] |= CB10_EEVIL;
							if (esp & ESP_DRAGONRIDER) csheet_boni[i].cb[9] |= CB10_EDGRI;
							if (esp & ESP_GOOD) csheet_boni[i].cb[9] |= CB10_EGOOD;
							if (esp & ESP_NONLIVING) csheet_boni[i].cb[9] |= CB10_ENONL;
							if (esp & ESP_UNIQUE) csheet_boni[i].cb[9] |= CB10_EUNIQ;
						}
						if (f1 & TR1_BRAND_FIRE) csheet_boni[i].cb[10] |= CB11_BFIRE;
						if (f3 & TR3_SH_FIRE) csheet_boni[i].cb[10] |= CB11_AFIRE;
						if (f1 & TR1_BRAND_COLD) csheet_boni[i].cb[10] |= CB11_BCOLD;
						if (f5 & TR5_SH_COLD) csheet_boni[i].cb[10] |= CB11_ACOLD;
						if (f1 & TR1_BRAND_ELEC) csheet_boni[i].cb[10] |= CB11_BELEC;
						if (f3 & TR3_SH_ELEC) csheet_boni[i].cb[10] |= CB11_AELEC;
						if (f1 & TR1_BRAND_ACID) csheet_boni[i].cb[10] |= CB11_BACID;
						if (f1 & TR1_BRAND_POIS) csheet_boni[i].cb[10] |= CB11_BPOIS;
						if (f5 & TR5_VORPAL) csheet_boni[i].cb[11] |= CB12_BVORP;
					}
				}
#endif
				/* conclude hack: can *identifying* actually make a difference at all? */
				if (can_have_hidden_powers) csheet_boni[i].cb[11] |= CB12_XHIDD;
			}
			Send_boni_col(Ind, csheet_boni[i]);
		}



	/* Don't kill warnings by inspecting weapons/armour in stores! */
	if (!suppress_message && !p_ptr->ghost) {
		/* warning messages, mostly for newbies */
		if (p_ptr->warning_bpr == 0 && /* limit, so it won't annoy priests anymore who use zeal spell */
		    p_ptr->num_blow == 1 && old_num_blow > 1 &&
		    p_ptr->inventory[INVEN_WIELD].k_idx && is_melee_weapon(p_ptr->inventory[INVEN_WIELD].tval)) {
			p_ptr->warning_bpr = 1;
			msg_print(Ind, "\374\377yWARNING! Your number of melee attacks per round has just dropped to ONE.");
			msg_print(Ind, "\374\377y    If you rely on melee combat, it is strongly advised to try and");
			msg_print(Ind, "\374\377y    get AT LEAST TWO blows/round (shown in the bottom status line).");
			msg_print(Ind, "\374\377yPossible reasons are: Weapon is too heavy; too little STR or DEX; You");
			msg_print(Ind, "\374\377y    just equipped too heavy armour or a shield - depending on your class.");
			msg_print(Ind, "\374\377y    Also, some classes can dual-wield to get an extra blow/round.");
			s_printf("warning_bpr: %s\n", p_ptr->name);
		}
		if (p_ptr->warning_bpr3 == 2 &&
		    p_ptr->num_blow == 1 && old_num_blow == 1 &&
		    /* and don't spam Martial Arts users or mage-staff wielders ;) */
		    p_ptr->inventory[INVEN_WIELD].k_idx && is_melee_weapon(p_ptr->inventory[INVEN_WIELD].tval)) {
			p_ptr->warning_bpr2 = p_ptr->warning_bpr3 = 1;
			msg_print(Ind, "\374\377yWARNING! You can currently perform only ONE 'blow per round' (attack).");
			msg_print(Ind, "\374\377y    If you rely on close combat, you should get at least 2 BpR!");
			msg_print(Ind, "\374\377y    Possible reasons: Weapon is too heavy or your strength is too low.");
			if (p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD) {
				if (p_ptr->rogue_like_commands)
					msg_print(Ind, "\374\377y    Try taking off your shield ('\377oSHIFT+t\377y') and see if that helps.");
				else
					msg_print(Ind, "\374\377y    Try taking off your shield ('\377ot\377y' key) and see if that helps.");
			}
			switch (p_ptr->pclass) {
			case CLASS_WARRIOR:
				msg_print(Ind, "\374\377y    Warriors should try either a dagger, whip, spear or cleaver.");
				break;
			case CLASS_PALADIN:
				msg_print(Ind, "\374\377y    Paladins should try either a dagger, whip, spear or cleaver.");
				break;
#ifdef ENABLE_DEATHKNIGHT
			case CLASS_DEATHKNIGHT:
				msg_print(Ind, "\374\377y    Death Knights should try either a dagger, spear or cleaver.");
				break;
#endif
#ifdef ENABLE_HELLKNIGHT
			case CLASS_HELLKNIGHT:
				msg_print(Ind, "\374\377y    Hell Knights should try either a dagger, spear or cleaver.");
				break;
#endif
			case CLASS_MIMIC:
				msg_print(Ind, "\374\377y    Mimics should try either a dagger, whip, spear or cleaver.");
				break;
			case CLASS_ROGUE:
				msg_print(Ind, "\374\377y    Rogues should try dual-wielding two daggers or main gauches.");
				break;
			case CLASS_RANGER:
				msg_print(Ind, "\374\377y    Rangers should try dual-wielding two daggers or sword & dagger.");
				break;
			}
			s_printf("warning_bpr23: %s\n", p_ptr->name);
		}
	} /* suppress_message */



	/* hack: no physical attacks */
	if ((p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST) ||
	    (p_ptr->body_monster && (r_info[p_ptr->body_monster].flags1 & RF1_NEVER_BLOW)) ||
	    never_blow == 0x1 || ((never_blow == 0x2 || never_blow == 0x4) && !(p_ptr->dual_wield && p_ptr->dual_mode)) || never_blow == 0x6)
		p_ptr->num_blow = 0;
	if ((p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST) ||
	    never_blow_ranged)
		p_ptr->num_fire = 0;
}



/*
 * Handle "p_ptr->notice"
 */
void notice_stuff(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Notice stuff */
	if (!p_ptr->notice) return;

	/* Combine the pack */
	if (p_ptr->notice & PN_COMBINE) {
		p_ptr->notice &= ~(PN_COMBINE);
		combine_pack(Ind);
	}

	/* Reorder the pack */
	if (p_ptr->notice & PN_REORDER) {
		p_ptr->notice &= ~(PN_REORDER);
		reorder_pack(Ind);
	}
}


/*
 * Handle "p_ptr->update"
 */
void update_stuff(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Update stuff */
	if (!p_ptr->update) return;

	/* This should only be sent once. This data
	   does not change in runtime */
	if (p_ptr->update & PU_SKILL_INFO) {
		int i;

		calc_techniques(Ind);

		p_ptr->update &= ~(PU_SKILL_INFO);
		for (i = 0; i < MAX_SKILLS; i++) {
			if (s_info[i].name) {
				Send_skill_init(Ind, i);
			}
		}
	}

	if (p_ptr->update & PU_SKILL_MOD) {
		int i;

		p_ptr->update &= ~(PU_SKILL_MOD);
		for (i = 0; i < MAX_SKILLS; i++) {
			if (s_info[i].name && p_ptr->s_info[i].touched) {
				Send_skill_info(Ind, i, FALSE);
				p_ptr->s_info[i].touched = FALSE;
			}
		}
		Send_skill_points(Ind);
	}

	if (p_ptr->update & PU_BONUS) {
		p_ptr->update &= ~(PU_BONUS);
		/* Take off what is no more usable, BEFORE calculating boni */
		do_takeoff_impossible(Ind);
		calc_boni(Ind);
	}

	if (p_ptr->update & PU_TORCH) {
		p_ptr->update &= ~(PU_TORCH);
		calc_torch(Ind);
	}

	if (p_ptr->update & PU_HP) {
		p_ptr->update &= ~(PU_HP);
		calc_hitpoints(Ind);
	}

	if (p_ptr->update & (PU_SANITY)) {
		p_ptr->update &= ~(PU_SANITY);
		calc_sanity(Ind);
	}

	if (p_ptr->update & PU_MANA) {
		p_ptr->update &= ~(PU_MANA);
		calc_mana(Ind);
	}

	/* Character is not ready yet, no screen updates */
	/*if (!character_generated) return;*/

	/* Character has changed depth very recently, no screen updates */
	if (p_ptr->new_level_flag) return;

	if (p_ptr->update & PU_UN_LITE) {
		p_ptr->update &= ~(PU_UN_LITE);
		forget_lite(Ind);
	}

	if (p_ptr->update & PU_UN_VIEW) {
		p_ptr->update &= ~(PU_UN_VIEW);
		forget_view(Ind);
	}


	if (p_ptr->update & PU_VIEW) {
		p_ptr->update &= ~(PU_VIEW);
		update_view(Ind);
	}

	if (p_ptr->update & PU_LITE) {
		p_ptr->update &= ~(PU_LITE);
		update_lite(Ind);
	}


	if (p_ptr->update & PU_FLOW) {
		p_ptr->update &= ~(PU_FLOW);
		update_flow();
	}


	if (p_ptr->update & PU_DISTANCE) {
		p_ptr->update &= ~(PU_DISTANCE);
		p_ptr->update &= ~(PU_MONSTERS);
		update_monsters(TRUE);
		update_players();
	}

	if (p_ptr->update & PU_MONSTERS) {
		p_ptr->update &= ~(PU_MONSTERS);
		update_monsters(FALSE);
		update_players();
	}


#ifdef USE_SOUND_2010
	/* Have we freshly forged or broken a mind link? */
	if (p_ptr->update & PU_MUSIC) {
		p_ptr->update &= ~PU_MUSIC;
		/* hack: update music, if we're mind-linked to someone */
		int Ind2;
		player_type *p_ptr2;
		u32b f;

		/* we just forged a link (and we are sender) */
		if ((Ind2 = get_esp_link(Ind, LINKF_VIEW, &p_ptr2))) {
			/* hack: temporarily remove the dedicated mind-link flag, to allow Send_music() */
			f = Players[Ind2]->esp_link_flags;
			Players[Ind2]->esp_link_flags &= ~LINKF_VIEW_DEDICATED;
			Send_music(Ind2, Players[Ind]->music_current, Players[Ind]->musicalt_current);
			/* ultra hack-- abuse this for ambient sfx too ^^ */
			Send_sfx_ambient(Ind2, Players[Ind]->sound_ambient, FALSE);
			Players[Ind2]->esp_link_flags = f;
		}
		/* we just broke a link (and we were receiver) */
		else {
			cave_type **zcave = getcave(&p_ptr->wpos);
			handle_music(Ind); /* restore music after a mind-link has been broken */
			/* ultra hack-- abuse this for ambient sfx too ^^ */
			if (zcave) handle_ambient_sfx(Ind, &(zcave[p_ptr->py][p_ptr->px]), &p_ptr->wpos, FALSE);
			else {
				/* Rare crash, induced by clearing a doppelganger of the mindlink receiver at
				   his actual location, putting him at some odd location where he not really is,
				   causing getcave() to find no allocated cave at 'his' coordinates. */
				s_printf("UPATE_STUFF: zcave failed (%s)\n", p_ptr->name);
				/* retry */
				p_ptr->update |= PU_MUSIC;
			}
		}
	}
#endif


	if (p_ptr->update & PU_LUA) {
		/* update the client files */
		p_ptr->update &= ~(PU_LUA);
		p_ptr->warning_lua_update = p_ptr->warning_lua_count = 0;
		exec_lua(Ind, "update_client()");
	}
}


/*
 * Handle "p_ptr->redraw"
 */
void redraw_stuff(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Redraw stuff */
	if (!p_ptr->redraw) return;

	/* Character is not ready yet, no screen updates */
	/*if (!character_generated) return;*/

	/* Hack -- clear the screen */
	if (p_ptr->redraw & PR_WIPE) {
		p_ptr->redraw &= ~PR_WIPE;
		msg_print(Ind, NULL);
	}


	if (p_ptr->redraw & PR_MAP) {
		p_ptr->redraw &= ~(PR_MAP);
		prt_map(Ind, FALSE);

#ifdef CLIENT_SIDE_WEATHER
		/* hack: update weather if it was just a panel-change
		   (ie level-sector) and not a level-change.
		   Note: this could become like PR_WEATHER_PANEL,
		   however, PR_ flags are already in use completely atm. */
		if (p_ptr->panel_changed) player_weather(Ind, FALSE, FALSE, TRUE);
#endif
		p_ptr->panel_changed = FALSE;
	}


	if (p_ptr->redraw & PR_BASIC) {
		p_ptr->redraw &= ~(PR_BASIC);
		p_ptr->redraw &= ~(PR_MISC | PR_TITLE | PR_STATS);
		p_ptr->redraw &= ~(PR_LEV | PR_EXP | PR_GOLD);
		p_ptr->redraw &= ~(PR_ARMOR | PR_HP | PR_MANA | PR_STAMINA);
		p_ptr->redraw &= ~(PR_DEPTH | PR_HEALTH);
		prt_frame_basic(Ind);
	}

	if (p_ptr->redraw & PR_ENCUMBERMENT) {
		p_ptr->redraw &= ~(PR_ENCUMBERMENT);
		prt_encumberment(Ind);
	}

	if (p_ptr->redraw & PR_MISC) {
		p_ptr->redraw &= ~(PR_MISC);
		Send_char_info(Ind, p_ptr->prace, p_ptr->pclass, p_ptr->ptrait, p_ptr->male, p_ptr->mode, p_ptr->name);
	}

	if (p_ptr->redraw & PR_TITLE) {
		p_ptr->redraw &= ~(PR_TITLE);
		prt_title(Ind);
	}

	if (p_ptr->redraw & PR_LEV) {
		p_ptr->redraw &= ~(PR_LEV);
		prt_level(Ind);
	}

	if (p_ptr->redraw & PR_EXP) {
		p_ptr->redraw &= ~(PR_EXP);
		prt_exp(Ind);
	}

	if (p_ptr->redraw & PR_STATS) {
		p_ptr->redraw &= ~(PR_STATS);
		prt_stat(Ind, A_STR);
		prt_stat(Ind, A_INT);
		prt_stat(Ind, A_WIS);
		prt_stat(Ind, A_DEX);
		prt_stat(Ind, A_CON);
		prt_stat(Ind, A_CHR);
	}

	if (p_ptr->redraw & PR_ARMOR) {
		p_ptr->redraw &= ~(PR_ARMOR);
		prt_ac(Ind);
	}

	if (p_ptr->redraw & PR_HP) {
		p_ptr->redraw &= ~(PR_HP);
		prt_hp(Ind);
	}

	if (p_ptr->redraw & PR_STAMINA) {
		p_ptr->redraw &= ~(PR_STAMINA);
		prt_stamina(Ind);
	}

	if (p_ptr->redraw & PR_SANITY) {
		p_ptr->redraw &= ~(PR_SANITY);
#ifdef SHOW_SANITY
		prt_sanity(Ind);
#endif
	}

	if (p_ptr->redraw & PR_MANA) {
		p_ptr->redraw &= ~(PR_MANA);
		prt_sp(Ind);
	}

	if (p_ptr->redraw & PR_GOLD) {
		p_ptr->redraw &= ~(PR_GOLD);
		prt_gold(Ind);
	}

	if (p_ptr->redraw & PR_DEPTH) {
		p_ptr->redraw &= ~(PR_DEPTH);
		prt_depth(Ind);
	}

	if (p_ptr->redraw & PR_HEALTH) {
		p_ptr->redraw &= ~(PR_HEALTH);
		health_redraw(Ind);
	}

	if (p_ptr->redraw & PR_HISTORY) {
		p_ptr->redraw &= ~(PR_HISTORY);
		prt_history(Ind);
	}

	if (p_ptr->redraw & PR_VARIOUS) {
		p_ptr->redraw &= ~(PR_VARIOUS);
		prt_various(Ind);
	}

	if (p_ptr->redraw & PR_PLUSSES) {
		p_ptr->redraw &= ~(PR_PLUSSES);
		prt_plusses(Ind);
	}

	if (p_ptr->redraw & PR_SKILLS) {
		p_ptr->redraw &= ~(PR_SKILLS);
		prt_skills(Ind);
	}

	if (p_ptr->redraw & PR_EXTRA) {
		p_ptr->redraw &= ~(PR_EXTRA);
		p_ptr->redraw &= ~(PR_CUT | PR_STUN);
		p_ptr->redraw &= ~(PR_HUNGER);
		p_ptr->redraw &= ~(PR_BLIND | PR_CONFUSED);
		p_ptr->redraw &= ~(PR_AFRAID | PR_POISONED);
		p_ptr->redraw &= ~(PR_STATE | PR_SPEED);
		if (is_older_than(&p_ptr->version, 4, 4, 8, 5, 0, 0)) p_ptr->redraw &= ~PR_STUDY;
		prt_frame_extra(Ind);
		prt_extra_status(Ind);
	}

	if (p_ptr->redraw & PR_CUT) {
		p_ptr->redraw &= ~(PR_CUT);
		prt_cut(Ind);
	}

	if (p_ptr->redraw & PR_STUN) {
		p_ptr->redraw &= ~(PR_STUN);
		prt_stun(Ind);
	}

	if (p_ptr->redraw & PR_HUNGER) {
		p_ptr->redraw &= ~(PR_HUNGER);
		prt_hunger(Ind);
	}

	if (p_ptr->redraw & PR_BLIND) {
		p_ptr->redraw &= ~(PR_BLIND);
		prt_blind(Ind);
	}

	if (p_ptr->redraw & PR_CONFUSED) {
		p_ptr->redraw &= ~(PR_CONFUSED);
		prt_confused(Ind);
	}

	if (p_ptr->redraw & PR_AFRAID) {
		p_ptr->redraw &= ~(PR_AFRAID);
		prt_afraid(Ind);
	}

	if (p_ptr->redraw & PR_POISONED) {
		p_ptr->redraw &= ~(PR_POISONED);
		prt_poisoned(Ind);
	}

	if (p_ptr->redraw & PR_STATE) {
		p_ptr->redraw &= ~(PR_STATE);
		prt_state(Ind);
		prt_extra_status(Ind);
	}

	if (p_ptr->redraw & PR_SPEED) {
		p_ptr->redraw &= ~(PR_SPEED);
		prt_speed(Ind);
	}

	if (is_older_than(&p_ptr->version, 4, 4, 8, 5, 0, 0)) {
		if (p_ptr->redraw & PR_STUDY) {
			p_ptr->redraw &= ~(PR_STUDY);
			prt_study(Ind);
		}
	} else {
		if (p_ptr->redraw & PR_BPR_WRAITH) {
			p_ptr->redraw &= ~(PR_BPR_WRAITH);
			prt_bpr(Ind);
		}
	}
}
/* Note that this should be called before redraw_stuff() for BIGMAP_MINDLINK_HACK to work. */
void redraw2_stuff(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Redraw stuff */
	if (!p_ptr->redraw2) return;

	if (p_ptr->redraw2 & PR2_MAP_FWD) {
		p_ptr->redraw2 &= ~(PR2_MAP_FWD);
		prt_map_forward(Ind);
	}

	if (p_ptr->redraw2 & PR2_MAP_SCR) {
		p_ptr->redraw2 &= ~(PR2_MAP_SCR);
		prt_map(Ind, TRUE);
	}
}

/*
 * Handle "p_ptr->window"
 */
void window_stuff(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Window stuff */
	if (!p_ptr->window) return;

	/* Display inventory */
	if (p_ptr->window & PW_INVEN) {
		p_ptr->window &= ~(PW_INVEN);
		fix_inven(Ind);
	}

	/* Display equipment */
	if (p_ptr->window & PW_EQUIP) {
		p_ptr->window &= ~(PW_EQUIP);
		fix_equip(Ind);
	}

	/* Display player */
	if (p_ptr->window & PW_PLAYER) {
		p_ptr->window &= ~(PW_PLAYER);
		fix_player(Ind);
	}

	/* Display overhead view */
	if (p_ptr->window & PW_MESSAGE) {
		p_ptr->window &= ~(PW_MESSAGE);
		fix_message(Ind);
	}

	/* Display overhead view */
	if (p_ptr->window & PW_OVERHEAD) {
		p_ptr->window &= ~(PW_OVERHEAD);
		fix_overhead(Ind);
	}

	/* Display monster recall */
	if (p_ptr->window & PW_MONSTER) {
		p_ptr->window &= ~(PW_MONSTER);
		fix_monster(Ind);
	}

	/* Display all inventory and equipment, forced */
	if (p_ptr->window & PW_ALLITEMS) {
		p_ptr->window &= ~(PW_ALLITEMS);
		fix_allitems(Ind);
	}

	/* Display all inventory and equipment, forced, to mindlinker */
	if (p_ptr->window & PW_ALLITEMS_FWD) {
		p_ptr->window &= ~(PW_ALLITEMS_FWD);
		fix_allitems_fwd(Ind);
	}
}


/*
 * Handle "p_ptr->update" and "p_ptr->redraw" and "p_ptr->window"
 */
void handle_stuff(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Hack -- delay updating */
	if (p_ptr->new_level_flag) return;

	/* Update stuff */
	if (p_ptr->update) update_stuff(Ind);

	/* Redraw stuff */
	if (p_ptr->redraw2) redraw2_stuff(Ind);
	if (p_ptr->redraw) redraw_stuff(Ind);

	/* Window stuff */
	if (p_ptr->window) window_stuff(Ind);
}


/*
 * Start a global_event (highlander tournament etc.) - C. Blue
 * IMPORTANT: Ind may be 0 if this is called from custom.lua !
 */
int start_global_event(int Ind, int getype, char *parm) {
	int n, i;
	player_type *p_ptr = NULL;
	global_event_type *ge;
	if (Ind) p_ptr = Players[Ind];
#if 1 /* randomize the global_event's ID? (makes sense if you have 'hidden' events) */
	int f = 0, c;
	for (n = 0; n < MAX_GLOBAL_EVENTS; n++) if (global_event[n].getype == GE_NONE) f++;
	if (!f) return(1); /* error, no more global events */
	c = randint(f);
	f = 0;
	for (n = 0; n < MAX_GLOBAL_EVENTS; n++) if (global_event[n].getype == GE_NONE) {
		f++;
		if (f == c) break;
	}
#else
	for (n = 0; n < MAX_GLOBAL_EVENTS; n++) if (global_event[n].getype == GE_NONE) break;/* success */
	if (n == MAX_GLOBAL_EVENTS) return(1); /* error, no more global events */
#endif

	ge = &global_event[n];
	ge->getype = getype;
	ge->creator = 0;
	if (Ind) ge->creator = p_ptr->id;
	ge->announcement_time = 1800; /* time until event finally starts, announced every 15 mins */
	ge->signup_time = 0; /* 0 = during announcement time */
	ge->first_announcement = TRUE; /* first announcement will also display available commands */
	ge->start_turn = turn;
	//ge->start_turn = turn + 1; /* +1 is a small hack, to prevent double-announcement */
	/* hack - synch start_turn to cfg.fps, since process_global_events is only called every
	   (turn % cfg.fps == 0), and its announcement timer checks will fail otherwise */
	ge->start_turn += (cfg.fps - (turn % cfg.fps));
	time(&ge->started);
	ge->paused = FALSE;
	ge->paused_turns = 0; /* counter for real turns the event "missed" while being paused */
	for (i = 0; i < 64; i++) {/* yeah I could use WIPE.. */
		ge->state[i] = 0;
		ge->participant[i] = 0;
		ge->extra[i] = 0;
	}
	ge->end_turn = 0;
	ge->ending = 0;
	strcpy(ge->title, "(UNTITLED EVENT)");
	for (i = 0; i < 10; i++) strcpy(ge->description[i], "");
	strcpy(ge->description[0], "(NO DESCRIPTION AVAILABLE)");
	ge->hidden = FALSE;
	ge->min_participants = 0; /* no minimum */
	ge->limited = 0; /* no maximum */
	ge->cleanup = 0; /* no cleaning up needed so far (for when the event ends) */
	ge->noghost = FALSE;

	/* IMPORTANT: state[0] == 255 is used as indicator that cleaning-up must be done, event has ended. */
	switch(getype) {
	case GE_HIGHLANDER:	/* 'Highlander Tournament' */
		/* parameters:
		[<int != 0>]	set announcement time to this (seconds)
		['>']		create staircases leading back into the dungeon from surface */

		strcpy(ge->title, "Highlander Tournament");
		strcpy(ge->description[0], " Create a new level 1 character, then sign him on for this deathmatch! ");
		strcpy(ge->description[1], " You're teleported into a dungeon and you have 10 minutes to level up. ");
		strcpy(ge->description[2], " After that, everyone will meet under the sky for a bloody slaughter.  ");
		strcpy(ge->description[3], " Add amulets of defeated opponents to yours to increase its power!     ");
		strcpy(ge->description[4], " Hints: When it starts you receive an amulet, don't forget to wear it! ");
		strcpy(ge->description[5], "        Buy your equipment BEFORE it starts. Or you'll go naked.       ");
		strcpy(ge->description[6], " Rules: Make sure that you don't gain ANY experience until it starts.  ");
		strcpy(ge->description[7], "        Also, you aren't allowed to pick up ANY gold/items from another");
		strcpy(ge->description[8], "        player before the tournament begins!                           ");
		strcpy(ge->description[9], "");
		ge->noghost = TRUE;
		ge->end_turn = ge->start_turn + cfg.fps * 60 * 90 ; /* 90 minutes max. duration,
								most of the time is just for announcing it
								so players will sign on via /evsign <n> */
		switch(rand_int(2)) { /* Determine terrain type! */
		case 0: ge->extra[2] = WILD_WASTELAND; break;
		//case 1: ge->extra[2] = WILD_SWAMP; break; swamp maybe too annoying
		case 1: ge->extra[2] = WILD_GRASSLAND; break;
		}
		switch(rand_int(3)) { /* Load premade layout? (Arenas) */
		case 1: ge->extra[4] = 1; break; //mountain region
		//case 2: ge->extra[4] = 2; break; //city ruins
		default: ge->extra[4] = 0; break; //no premade layout, just plain wilderness
		}
		if (!ge->extra[0]) ge->extra[0] = 95; /* there are no objects of lvl 96..99 anyways */
		if (atoi(parm)) ge->announcement_time = atoi(parm);
		ge->min_participants = 2;
		ge->extra[5] = 0; /* 0 = don't create staircases into the dungeon, 1 = do create */
		if (strstr(parm, ">")) ge->extra[5] = 1;
		break;
	case GE_ARENA_MONSTER:	/* 'Arena Monster Challenge' */
		/* parameters: none */

		strcpy(ge->title, "Arena Monster Challenge");
		strcpy(ge->description[0], " During the duration of Bree's Arena Monster Challenge, you just type  ");
		strcpy(ge->description[1], format(" '\377U/evsign %d <Monster Name>\377w' and you'll have a chance to challenge  ", n+1));
		strcpy(ge->description[2], " it for an illusion death match in Bree's upper training tower floor.  ");
		strcpy(ge->description[3], " Neither the monster nor you will really die in person, just illusions ");
		strcpy(ge->description[4], " of you, created by the wizards of 'Arena Monster Challenge (tm)' will ");
		strcpy(ge->description[5], " actually do the fighting. For the duration of the spell it will seem  ");
		strcpy(ge->description[6], " completely real to you though, and you can even use and consume items!");
		//strcpy(ge->description[7], " (Note: Some creatures might be beyond the wizards' abilities.)");
		strcpy(ge->description[7], format(" (Example: '\377U/evsign %d black orc vet\377w' gets you a veteran archer!)", n+1));
		strcpy(ge->description[8], "");
		strcpy(ge->description[9], "");
		ge->end_turn = ge->start_turn + cfg.fps * 60 * 30 ; /* 30 minutes max. duration, insta-start */
#if 0
		switch(rand_int(2)) { /* Determine terrain type! */
		case 0: ge->extra[2] = WILD_WASTELAND; break;
		//case 1: ge->extra[2] = WILD_SWAMP; break; swamp maybe too annoying
		case 1: ge->extra[2] = WILD_GRASSLAND; break;
		}
		switch(rand_int(3)) { /* Load premade layout? (Arenas) */
		case 0: ge->extra[4] = 1; break;
		}
#endif
		ge->announcement_time = 0;
		ge->signup_time = 60 * 30;
		ge->min_participants = 0;
		break;
	case GE_DUNGEON_KEEPER:	/* 'Dungeon Keeper' Labyrinth */
		strcpy(ge->title, "Dungeon Keeper");
		strcpy(ge->description[0], " Characters up to level 14 are eligible to sign up for this race for   ");
		strcpy(ge->description[1], " your life, through a labyrinth that is guarded by the Horned Reaper!  ");
		strcpy(ge->description[2], " Rules: Running, teleporting, healing, detection, maps and speed boni  ");
		strcpy(ge->description[3], "        do NOT work. You don't need any items for this event, except   ");
		strcpy(ge->description[4], "        for a brass lantern which you can buy from town store '1'.     ");
		strcpy(ge->description[5], " Your goal is to find one of the escape beacons (light green '>') in   ");
		strcpy(ge->description[6], " time, before the horned reaper finds you or the dungeon is flooded    ");
		strcpy(ge->description[7], " with lava (begins after 5 minutes, after 8 minutes it is submerged    ");
		strcpy(ge->description[8], " which will mean certain death, even if you are immune to fire).       ");
		strcpy(ge->description[9], " The escape beacons are self-illuminating so you won't miss them.");
		ge->noghost = TRUE;
		ge->end_turn = ge->start_turn + cfg.fps * 60 * 60 ; /* 60 minutes max. duration,
								most of the time is just for announcing it
								so players will sign on via /evsign <n> */
		ge->extra[0] = 95; /* there are no objects of lvl 96..99 anyways */
		ge->min_participants = 1; /* EXPERIMENTAL */
		if (atoi(parm)) ge->announcement_time = atoi(parm);
		break;
	}

	/* Fix limits */
	if (ge->limited > MAX_GE_PARTICIPANTS) ge->limited = MAX_GE_PARTICIPANTS;

#if 0
	/* give feedback to the creator; tell all admins if this was called from a script */
/*	if (Ind) {
		msg_format(Ind, "Reward item level = %d.", ge->extra[0]);
		msg_format(Ind, "Created new event #%d of type %d parms='%s'.", n + 1, getype, parm);
	} else */{
		for (i = 1;i <= NumPlayers; i++)
		    if (is_admin(Players[i])) {
			msg_format(i, "Reward item level = %d.", ge->extra[0]);
			msg_format(i, "Created new event #%d of type %d parms='%s'.", n + 1, getype, parm);
		}
	}
#endif
	s_printf("%s EVENT_CREATE: #%d of type %d parms='%s'\n", showtime(), n + 1, getype, parm);

	/* extra announcement if announcement time isn't the usual multiple of announcement intervals */
#if 1 /* this is ok, and leaving it for now */
	if ((ge->announcement_time % GE_ANNOUNCE_INTERVAL) && (ge->announcement_time % GE_FINAL_ANNOUNCEMENT)) announce_global_event(n);
#else /* this would be even finer, but actually it's not needed as long as start_global_event() is called with sensible announcement_time :) */
/* note that this else-branch is missing the GE_FINAL_ANNOUNCEMENT-check atm */
	if (ge->announcement_time >= 120 && !(GE_ANNOUNCE_INTERVAL % 60)) { /* if we announce in x-minute-steps, and have at least 2 minutes left.. */
		if ((ge->announcement_time / 60) % (GE_ANNOUNCE_INTERVAL / 60)) announce_global_event(n); /* ..then don't double-announce for this whole minute */
	} else { /* if we announce in second-steps or weird fractions of minutes, or if we display the remaining time in seconds anyway because it's <120s.. */
		if (ge->announcement_time % GE_ANNOUNCE_INTERVAL) announce_global_event(n);/* ..then don't double-announce just for this second */
	}
#endif
	return(0);
}

/*
 * Stop a global event :(
 */
void stop_global_event(int Ind, int n) {
	global_event_type *ge = &global_event[n];
	msg_format(Ind, "Wiping event #%d of type %d.", n+1, ge->getype);
	s_printf("%s EVENT_STOP: #%d of type %d\n", showtime(), n+1, ge->getype);
	if (ge->getype) msg_broadcast_format(0, "\377y[Event '%s' (%d) was cancelled.]", ge->title, n+1);
#if 0
	ge->getype = GE_NONE;
	for (i = 1; i <= NumPlayers; i++) Players[i]->global_event_type[n] = GE_NONE;
#else /* we really need to call the clean-up instead of just GE_NONEing it: */
 #if 0 /* of for normal cases */
	ge->announcement_time = -1; /* enter the processing phase, */
 #else /* for turn overflow situations */
	ge->paused_turns = 0; /* potentially fix turn counter */
	ge->start_turn = turn;
	ge->announcement_time = -1; /* enter the processing phase, */
 #endif
	ge->state[0] = 255; /* ..and process clean-up! */
#endif
	return;
}

void announce_global_event(int ge_id) {
	global_event_type *ge = &global_event[ge_id];
	int time_left = ge->announcement_time - ((turn - ge->start_turn) / cfg.fps);

	/* display minutes, if at least 120s left */
	//if (time_left >= 120) msg_broadcast_format(0, "\374\377W[%s (%d) starts in %d minutes. See \377s/evinfo\377W]", ge->title, ge_id + 1, time_left / 60);
	//if (time_left >= 120) msg_broadcast_format(0, "\374\377U[\377W%s (\377U%d\377W) starts in %d minutes - see /evinfo\377U]", ge->title, ge_id + 1, time_left / 60);
	//if (time_left >= 120) msg_broadcast_format(0, "\374\377U[\377W%s (\377U%d\377W) starts in %d minutes\377U]", ge->title, ge_id + 1, time_left / 60);
	if (time_left >= 120) msg_broadcast_format(0, "\374\377W[%s (\377U%d\377W) starts in %d minutes]", ge->title, ge_id + 1, time_left / 60);
	/* otherwise just seconds */
	else msg_broadcast_format(0, "\374\377W[%s (%d) starts in %d seconds!]", ge->title, ge_id + 1, time_left);

	/* display additional commands on first advertisement */
	if (ge->first_announcement) {
		msg_broadcast_format(0, " \377WType '\377U/evinfo %d\377W' to learn more and '\377U/evsign %d\377W' to sign up.", ge_id + 1, ge_id + 1);
		ge->first_announcement = FALSE;
	}
}

/*
 * A player signs on for a global_event - C. Blue
 */
void global_event_signup(int Ind, int n, cptr parm) {
	int i, p, max_p;
	bool fake_signup = FALSE;
	global_event_type *ge = &global_event[n];
	player_type *p_ptr = Players[Ind];

	char parm_log[60] = "";

	/* for monster type: */
	char c[80], parm2[80], *cp, *p2p;
	int r_found = 0;

#ifdef GE_ARENA_ALLOW_EGO
	/* for ego monsters: */
	int re_found = 0, re_r = 0;//compiler warning
	bool re_impossible = FALSE, no_ego = FALSE, perfect_ego = FALSE;
	char ce[80], *cep, parm2e[80], *p2ep;
	char *separator;

	int r_found_tmp = 0, re_found_tmp = 0, re_r_tmp = 0;
	int r_found_tmp_len = 0, r_found_len = 0;
#endif

	/* Still room for more participants? */
	max_p = MAX_GE_PARTICIPANTS;
	if (ge->limited) max_p = ge->limited; /* restricted number of participants? */
	for (p = 0; p < max_p; p++) if (!ge->participant[p]) break;/* success */
	if (p == max_p) {
		msg_print(Ind, "\377ySorry, maximum number of possible participants has been reached.");
		return;
	}

	for (i = 0; i < max_p; i++) if (ge->participant[i] == p_ptr->id) {
		msg_print(Ind, "\377sYou have already signed up!");
		return;
	}

	if (p_ptr->inval) {
		msg_print(Ind, "\377ySorry, only validated accounts may participate.");
		return;
	}

	/* check individual event restrictions against player */
	switch (ge->getype) {
	case GE_HIGHLANDER:	/* Highlander Tournament */
		if (p_ptr->mode & MODE_DED_IDDC) {
			msg_print(Ind, "\377ySorry, as a dedicated ironman deep diver you may not participate.");
			if (!is_admin(p_ptr)) return;
		}
		if (p_ptr->mode & MODE_PVP) {
			msg_print(Ind, "\377ySorry, PvP characters may not participate.");
			if (!is_admin(p_ptr)) return;
		}
		if (p_ptr->global_event_participated[ge->getype]) {
			msg_print(Ind, "\377ySorry, a character may participate only once in this event.");
			if (!is_admin(p_ptr)) return;
		}
		if (p_ptr->max_exp > 0 || p_ptr->max_plv > 1) {
			msg_print(Ind, "\377ySorry, only newly created characters may sign up for this event.");
			if (!is_admin(p_ptr)) return;
		}
		if (p_ptr->fruit_bat == 1) { /* 1 = native bat, 2 = from chauve-souris */
			msg_print(Ind, "\377ySorry, native fruit bats are not eligible to join this event.");
			if (!is_admin(p_ptr)) return;
		}
		break;
	case GE_ARENA_MONSTER:	/* Arena Monster Challenge */
		if (ge->state[0] != 1) { /* in case we do add an announcement time.. */
			msg_print(Ind, "\377yYou have to wait until it starts to sign up for this event!");
			return;
		}
#if 0 /* need to be in Bree or in the arena? */
		if (!in_bree(&p_ptr->wpos) &&
		    !in_arena(&p_ptr->wpos)) {
			msg_print(Ind, "\377yYou have to be in Bree or in the arena to sign up for this event!");
#else /* need to be in Bree or in the training tower? */
		if (!in_bree(&p_ptr->wpos) && !in_trainingtower(&p_ptr->wpos)
		    && !is_admin(p_ptr)) {
			msg_print(Ind, "\377yYou have to be in Bree or in the training tower to sign up for this event!");
#endif
			return;
		}
		if ((parm == NULL) || !strlen(parm)) {
			msg_format(Ind, "\377yYou have to specify a monster name:  /evsign %d monstername", n+1);
			return;
		}

		/* lower case */
		strcpy(parm2, parm);
		p2p = parm2;
		while (*p2p) {*p2p = tolower(*p2p); p2p++;}

		/* trim spaces .. and also quotes */
		p2p = parm2;
		while (*p2p == ' ' || *p2p == '"') p2p++;
		while (p2p[strlen(p2p) - 1] == ' ' || p2p[strlen(p2p) - 1] == '"') p2p[strlen(p2p) - 1] = 0;

		/* Scan the monster races */
		for (i = 1; i < MAX_R_IDX - 1; i++) {
			/* get monster race name */
			strcpy(c, r_info[i].name + r_name);
			if (!strlen(c)) continue;

			/* lower case */
			cp = c;
			while (*cp) {*cp = tolower(*cp); cp++;}


			/* exact match? */
			if (!strcmp(p2p, c)) {
				r_found = i;
				no_ego = TRUE;
				re_found = 0;

				/* done. No room for ego power. */
				break;
			}

#ifndef GE_ARENA_ALLOW_EGO
			if (strstr(p2p, c)) {
				r_found = i;
				r_found_len = strlen(c);
			}
#else
			/* partial match? could have ego power too. */
			if ((separator = strstr(p2p, c))) {
				/* test for ego power for the rest of the string */
				if (separator == p2p) { /* monster name begins with race name? */
					strcpy(parm2e, p2p + strlen(c)); /* then ego power begins afterwards and goes till the end of the monster name */
				} else if (strlen(separator) < strlen(c)) { /* error: race name has chars before AND afterwards */
					continue;
				} else { /* monster name ends with race name? */
					strncpy(parm2e, p2p, strlen(p2p) - strlen(c)); /* then ego power starts at the beginning of the monster name */
					parm2e[strlen(p2p) - strlen(c)] = 0;
				}

				/* if we already found both, a matching monster + ego, be wary of this new result!
				   Otherwise, 'black orc vet' will result in a plain 'Black' getting spawned.. */
				if (r_found && re_found) {
					r_found_tmp = r_found;
					re_found_tmp = re_found;
					re_r_tmp = re_r;
				} else if (r_found) {
					r_found_tmp = r_found;
					r_found_tmp_len = r_found_len;
				}

				/* remember choice in case we don't find anything better */
				r_found = i;
				r_found_len = strlen(c);

				/* check ego power - if exact match then we're done */
				/* trim spaces just to be sure */
				p2ep = parm2e;
				while (*p2ep == ' ') p2ep++;
				while (p2ep[strlen(p2ep) - 1] == ' ') p2ep[strlen(p2ep) - 1] = 0;

				/* IMPOSSIBLE-- no ego power specified, it was just spaces? */
				//if (!strlen(parm2e)) 

			        for (p = 1; p < MAX_RE_IDX; p++) {
					/* get monster ego name */
					strcpy(ce, re_info[p].name + re_name);

					/* lower case */
					cep = ce;
					while (*cep) {*cep = tolower(*cep); cep++;}

					/* exact match? */
					if (!strcmp(p2ep, ce)) {
						if (mego_ok(i, p)) {
							/* done, success */
							re_impossible = FALSE;
							re_found = p;
							re_r = i;
							perfect_ego = TRUE;

#if 1
							/* special hack: check all remaining races and prefer them if EXACT match - added for 'Fallen Angel'! */
							//actually, with ne addition of 'r_found_len' this should now be obsolete?
							while (++i < MAX_R_IDX - 1) {
								/* get monster race name */
								strcpy(c, r_info[i].name + r_name);
								if (!strlen(c)) continue;

								/* lower case */
								cp = c;
								while (*cp) {*cp = tolower(*cp); cp++;}

								/* exact match? */
								if (!strcmp(p2p, c)) {
									if (!is_admin(p_ptr) &&
									    ((r_info[i].flags1 & RF1_UNIQUE) ||
									    (r_info[i].flags1 & RF1_QUESTOR) ||
									    (r_info[i].flags7 & RF7_NEVER_ACT) ||
									    (r_info[i].flags7 & RF7_PET) ||
									    (r_info[i].flags7 & RF7_NEUTRAL) ||
									    (r_info[i].flags7 & RF7_FRIENDLY) ||
									    (r_info[i].flags8 & RF8_JOKEANGBAND) ||
									    (r_info[i].rarity == 255)))
										continue;

									/* done. Discard perfect 'ego power+base race' in favour for just plain perfect base race,
									   to allow 'Fallen Angel' base monster instead of 'Fallen' ego type on 'Angel' base monster. */
									r_found = i;
									r_found_len = strlen(c);
									no_ego = TRUE;
									re_found = 0;
									break;
								}
							}
#endif

							break;
						}
						/* impossible ego power -- keep searching in case we find something better */
						re_impossible = TRUE;
					} else if (re_impossible) continue; /* don't allow partial matches if an exact match already failed us */

					/* partial match? */
					//else if (strstr(p2ep, ce)) {
					else if (strstr(ce, p2ep)) {
						if (mego_ok(i, p)) {
							/* remember choice in case we don't find anything better */
							re_found = p;
							re_r = i;
						}
					}
				}
				if (perfect_ego) break;
#endif
			}

			/* if we already found both, a matching monster + ego, be wary of this new result!
			   Otherwise, 'black orc vet' will result in a plain 'Black' getting spawned.. */
			if (r_found_tmp && re_found_tmp && !re_found) {
				r_found = r_found_tmp;
				re_found = re_found_tmp;
				re_r = re_r_tmp;
			}
			/* if we still found no ego power, at least choose the base type that matches a longer string.
			   Otherwise 'black orc vet' will result in a Black spawning.. */
			else if (r_found_tmp && !re_found_tmp && !re_found && r_found_tmp_len > r_found_len)
				r_found = r_found_tmp;
		}

		if (!r_found) {
			msg_print(Ind, "\377yCouldn't find base monster (punctuation and name must be exact).");
			return;
		}
#ifdef GE_ARENA_ALLOW_EGO
		if (re_impossible) {
			msg_print(Ind, "\377ySorry, that ego creature is beyond the wizards' abilities.");
			return;
		}
		/* if we didn't find a race+ego combo, we at least found a raw race, use it */
		if (!re_found && !no_ego) {
			msg_print(Ind, "\377yCouldn't find matching ego power, going with base version..");
		}
#endif

		/* if we found a race and ego that somewhat fit, prefer that over just a race without ego, that somewhat fits */
		if (re_found) r_found = re_r;

		/* base monster type is not allowed? */
		if (!is_admin(p_ptr) &&
		    ((r_info[r_found].flags1 & (RF1_UNIQUE | RF1_QUESTOR)) ||
		    (r_info[r_found].flags7 & (RF7_NEVER_ACT | RF7_NO_DEATH | RF7_PET | RF7_NEUTRAL | RF7_FRIENDLY)) ||
		    (r_info[r_found].flags8 & (RF8_GENO_NO_THIN | RF8_JOKEANGBAND)) ||
		    (r_info[r_found].rarity == 255))) {
			msg_print(Ind, "\377ySorry, that creature is beyond the wizards' abilities.");
			return;
		}

		ge->extra[1] = r_found;
#ifndef GE_ARENA_ALLOW_EGO
		monster_race_desc(0, c, r_found_race_only, 0x188);
		msg_broadcast_format(0, "\376\377c** %s challenges %s! **", p_ptr->name, c);
		strcpy(parm_log, r_name + r_info[r_found].name);
#else
		ge->extra[3] = re_found;
		ge->extra[5] = Ind;

		/* rebuild parm from definite ego+monster name */
		if (re_found) {
			strcpy(parm_log, re_name + re_info[re_found].name);
			strcat(parm_log, " ");
			strcat(parm_log, r_name + r_info[r_found].name);
		} else {
			strcpy(parm_log, r_name + r_info[r_found].name);
		}
#endif

		fake_signup = TRUE;
		break;
	case GE_DUNGEON_KEEPER:	/* 'Dungeon Keeper' labyrinth race */
		if (p_ptr->mode & MODE_DED_IDDC) {
			msg_print(Ind, "\377ySorry, as a dedicated ironman deep diver you may not participate.");
			if (!is_admin(p_ptr)) return;
		}
		if (p_ptr->mode & MODE_PVP) {
			msg_print(Ind, "\377ySorry, PvP characters may not participate.");
			if (!is_admin(p_ptr)) return;
		}
		if (p_ptr->max_plv > 14) {
			msg_print(Ind, "\377ySorry, you must be at most level 14 to sign up for this event.");
			if (!is_admin(p_ptr)) return;
		}
#if 0 /* fine now since speed is limited to +0 via floor flag */
		if (p_ptr->fruit_bat == 1) { /* 1 = native bat, 2 = from chauve-souris */
			msg_print(Ind, "\377ySorry, native fruit bats are not eligible to join this event.");
			if (!is_admin(p_ptr)) return;
		}
#endif
		if (p_ptr->ghost) {
			msg_print(Ind, "\377ySorry, ghosts may not participate in this event.");
			if (!is_admin(p_ptr)) return;
		}
		if (p_ptr->global_event_participated[ge->getype]) {
			msg_print(Ind, "\377ySorry, a character may participate only once in this event.");
			if (!is_admin(p_ptr)) return;
		}
		break;
	}

	if (parm_log[0]) s_printf("%s EVENT_SIGNUP: %d (%s): %s (%s).\n", showtime(), n + 1, ge->title, p_ptr->name, parm_log);
	else s_printf("%s EVENT_SIGNUP: %d (%s): %s.\n", showtime(), n + 1, ge->title, p_ptr->name);
	if (fake_signup) return;

	/* currently no warning/error/solution if you try to sign on for multiple events at the same time :|
	   However, player_type has MAX_GLOBAL_EVENTS sized data arrays for each event, so a player *could*
	   theoretically participate in all events at once at this time.. */
	msg_format(Ind, "\374\377c>>You signed up for %s!<<", ge->title);
	msg_broadcast_format(Ind, "\374\377c%s signed up for %s.", p_ptr->name, ge->title);
	ge->participant[p] = p_ptr->id;
	p_ptr->global_event_type[n] = ge->getype;
	time(&p_ptr->global_event_signup[n]);
	p_ptr->global_event_started[n] = ge->started;
	for (i = 0; i < 4; i++) p_ptr->global_event_progress[n][i] = 0; /* WIPE :p */
}

/*
 * Process a global event - C. Blue
 */
static void process_global_event(int ge_id) {
	global_event_type *ge = &global_event[ge_id];
	player_type *p_ptr;
	object_type forge, *o_ptr = &forge; /* for creating a reward, for example */
	worldpos wpos;
	struct wilderness_type *wild;
	struct dungeon_type *d_ptr;
	int participants = 0;
	int i, j = 0, n, k, x, y; /* misc variables, used by the events */
	cave_type **zcave, *c_ptr;
	int xstart = 0, ystart = 0; /* for arena generation */
	long elapsed, elapsed_turns; /* amounts of seconds elapsed since the event was started (created) */
	char m_name[MNAME_LEN], buf[MSG_LEN];
	int m_idx, tries = 0;
	time_t now;

	time(&now);
	/* real timer will sometimes fail when using in a function which isn't sync'ed against it but instead relies
	    on the game's FPS for timining ..strange tho :s */
	elapsed_turns = turn - ge->start_turn - ge->paused_turns;
	elapsed = elapsed_turns / cfg.fps;

	wpos.wx = WPOS_SECTOR00_X; /* sector 0,0 by default, for 'sector00separation' */
	wpos.wy = WPOS_SECTOR00_Y;
	wpos.wz = WPOS_SECTOR00_Z;

	/* catch absurdities (happens on turn overflow) */
	if (elapsed_turns > 100000 * cfg.fps) {
		ge->paused_turns = 0;
		ge->start_turn = turn; /* fix turn counter */
		elapsed_turns = 0;
		ge->announcement_time = -1; /* enter the processing phase, */
		ge->state[0] = 255; /* ..and process clean-up! */
	}

	/* extra warning at T - x min for last minute subscribers */
	if (ge->announcement_time * cfg.fps - elapsed_turns == GE_FINAL_ANNOUNCEMENT * cfg.fps) {
		announce_global_event(ge_id);
		return; /* not yet >:) */

	/* Start/announce event */
	} else if (ge->announcement_time * cfg.fps - elapsed_turns >= 0) {
//debug		msg_format(1, ".%d.%d", ge->announcement_time * cfg.fps - elapsed_turns, elapsed_turns);

		/* Start/announce event, take two */
		if (!((ge->announcement_time * cfg.fps - elapsed_turns) % (GE_ANNOUNCE_INTERVAL * cfg.fps))) {

			/* we're still just announcing the event -- nothing more to do for now */
			if (ge->announcement_time * cfg.fps - elapsed_turns > 0L) {
				announce_global_event(ge_id);
				return; /* not yet >:) */

			/* prepare to start the event! */
			} else {
				dungeon_type *d_ptr;

				/* count participants first, to see if there are enough to start the event */
				for (i = 0; i < MAX_GE_PARTICIPANTS; i++) {
					if (!ge->participant[i]) continue;

					/* Check that the player really is here - mikaelh */
					for (j = 1; j <= NumPlayers; j++)
						if (Players[j]->id == ge->participant[i]) break;
					if (j > NumPlayers) {
						s_printf("EVENT_CHECK_PARTICIPANTS: ID %d no longer available.\n", ge->participant[i]);
						continue;
					}

					p_ptr = Players[j];
					d_ptr = getdungeon(&p_ptr->wpos);

					/* Check that he still fulfils requirements, if any */
					switch (ge->getype) {
					case GE_HIGHLANDER:
						if ((p_ptr->max_exp || p_ptr->max_plv > 1) && !is_admin(p_ptr)) {
							s_printf("EVENT_CHECK_PARTICIPANTS: Player '%s' no longer eligible.\n", p_ptr->name);
							msg_print(j, "\377oCharacters need to have 0 experience to be eligible.");
							msg_broadcast_format(j, "\377s%s has gained experience and is no longer eligible.", p_ptr->name);
#ifdef USE_SOUND_2010
							sound(j, "failure", NULL, SFX_TYPE_MISC, FALSE);
#endif
							p_ptr->global_event_type[ge_id] = GE_NONE;
							ge->participant[i] = 0;
							continue;
						}
						if (d_ptr && ((d_ptr->flags1 & (DF1_FORCE_DOWN | DF1_NO_RECALL)) || (d_ptr->flags2 & (DF2_IRON | DF2_NO_EXIT_WOR)))) {
							s_printf("EVENT_CHECK_PARTICIPANTS: Player '%s' stuck in dungeon.\n", p_ptr->name);
							msg_print(j, "\377oEvent participation failed because your dungeon doesn't allow recalling.");
							msg_broadcast_format(j, "\377s%s isn't allowed to leave the dungeon to participate.", p_ptr->name);
#ifdef USE_SOUND_2010
							sound(j, "failure", NULL, SFX_TYPE_MISC, FALSE);
#endif
							p_ptr->global_event_type[ge_id] = GE_NONE;
							ge->participant[i] = 0;
							continue;
						}
						break;
					case GE_DUNGEON_KEEPER:
						if ((p_ptr->max_plv > 14) && !is_admin(p_ptr)) {
							s_printf("EVENT_CHECK_PARTICIPANTS: Player '%s' no longer eligible.\n", p_ptr->name);
							msg_print(j, "\377oCharacters need to have 0 experience to be eligible.");
							msg_broadcast_format(j, "\377s%s is no longer eligible due to character level.", p_ptr->name);
#ifdef USE_SOUND_2010
							sound(j, "failure", NULL, SFX_TYPE_MISC, FALSE);
#endif
							p_ptr->global_event_type[ge_id] = GE_NONE;
							ge->participant[i] = 0;
							continue;
						}
						if (d_ptr && ((d_ptr->flags1 & (DF1_FORCE_DOWN | DF1_NO_RECALL)) || (d_ptr->flags2 & (DF2_IRON | DF2_NO_EXIT_WOR)))) {
							s_printf("EVENT_CHECK_PARTICIPANTS: Player '%s' stuck in dungeon.\n", p_ptr->name);
							msg_print(j, "\377oEvent participation failed because your dungeon doesn't allow recalling.");
							msg_broadcast_format(j, "\377s%s isn't allowed to leave the dungeon to participate.", p_ptr->name);
#ifdef USE_SOUND_2010
							sound(j, "failure", NULL, SFX_TYPE_MISC, FALSE);
#endif
							p_ptr->global_event_type[ge_id] = GE_NONE;
							ge->participant[i] = 0;
							continue;
						}
						break;
					}

					/* Player is valid for entering */
					participants++;
				}

				/* not enough participants? Don't hand out reward for free to someone. */
				if (ge->min_participants && (participants < ge->min_participants)) {
					msg_broadcast_format(0, "\377y%s needs at least %d participant%s.", ge->title, ge->min_participants, ge->min_participants == 1 ? "" : "s");
					s_printf("%s EVENT_NOPLAYERS: %d (%s) has only %d/%d participants.\n", showtime(), ge_id + 1, ge->title, participants, ge->min_participants);
					/* remove players who DID sign up from being 'participants' */
					for (j = 1; j <= NumPlayers; j++)
						if (Players[j]->global_event_type[ge_id] == ge->getype) {
							Players[j]->global_event_type[ge_id] = GE_NONE;
#ifdef USE_SOUND_2010
							sound(j, "failure", NULL, SFX_TYPE_MISC, FALSE);
#endif
						}
					ge->getype = GE_NONE;

				/* Participants are ok, event now starts! */
				} else {
					s_printf("%s EVENT_STARTS: %d (%s) has %d participants.\n", showtime(), ge_id + 1, ge->title, participants);
					msg_broadcast_format(0, "\374\377U[>>\377C%s (\377U%d\377C) starts now!\377U<<]", ge->title, ge_id + 1);

					/* memorize each character's participation */
					for (j = 0; j < MAX_GE_PARTICIPANTS; j++) {
						if (!ge->participant[j]) continue;

						for (i = 1; i <= NumPlayers; i++) {
							if (Players[i]->id != ge->participant[j]) continue;
							Players[i]->global_event_participated[ge->getype]++;
							/* play warning sfx in case they were afk waiting for it to begin? ;) */
#ifdef USE_SOUND_2010
							sound(i, "gong", "bell", SFX_TYPE_MISC, FALSE);
#endif
						}
					}
				}
			}

		/* we're still just announcing the event -- nothing more to do for now */
		} else {
			return; /* still announcing */
		}
	}

	/* Event starts immediately without announcement time? Still display a hint message. */
	if (!ge->announcement_time && !ge->state[0])
		msg_broadcast_format(0, " \377WType '\377U/evinfo %d\377W' to learn more and '\377U/evsign %d\377W' to sign up.", ge_id + 1, ge_id + 1);

	/* if event is not yet over, check if it could be.. */
	if (ge->state[0] != 255) {
		/* Time Over? :( */
		if ((ge->end_turn && turn >= ge->end_turn) ||
		    (ge->ending && now >= ge->ending)) {
			ge->state[0] = 255; /* state[0] is used as indicator for clean-up phase of any event */
			msg_broadcast_format(0, "\377y>>%s ends due to time limit!<<", ge->title);
			s_printf("%s EVENT_TIMEOUT: %d - %s.\n", showtime(), ge_id + 1, ge->title);
		}
		/* Time warning at T-5minutes! (only if the whole event lasts MORE THAN 10 minutes) */
		/* Note: paused turns will be added to the running time, IF the event end is given in "turns" */
		if ((ge->end_turn && ge->end_turn - ge->start_turn - ge->paused_turns > 600 * cfg.fps && turn - ge->paused_turns == ge->end_turn - 300 * cfg.fps) ||
		    /* However, paused turns will be ignored if the event end is given as absolute time! */
		    (!ge->end_turn && ge->ending && ge->ending - ge->started > 600 && now == ge->ending - 360)) {
			msg_broadcast_format(0, "\377y[%s (%d) comes to an end in 6 more minutes!]", ge->title, ge_id + 1);
		}
	}

	/* Event is running! Process its stages... */
	switch (ge->getype) {
	/* Highlander Tournament */
	case GE_HIGHLANDER:
		switch (ge->state[0]) {
		case 0: /* prepare level, gather everyone, start exp'ing */
			ge->cleanup = 1;
			sector00separation++; /* separate sector 0,0 from the worldmap - participants have access ONLY */
			sector00flags1 = sector00flags2 = 0x0;
			wipe_m_list(&wpos); /* clear any (powerful) spawns */
			wipe_o_list_safely(&wpos); /* and objects too */
			unstatic_level(&wpos);/* get rid of any other person, by unstaticing ;) */

			if (!getcave(&wpos)) alloc_dungeon_level(&wpos);
			/* generate solid battleground, not oceans and stuff */
			ge->extra[1] = wild_info[wpos.wy][wpos.wx].type;
			s_printf("EVENT_LAYOUT: Generating wild %d at %d,%d,%d\n", ge->extra[2], wpos.wx, wpos.wy, wpos.wz);
			wild_info[wpos.wy][wpos.wx].type = ge->extra[2];
			wilderness_gen(&wpos);
			/* make it static */
//			static_level(&wpos); /* preserve layout while players dwell in dungeon (for arena layouts) */
			new_players_on_depth(&wpos, 1, FALSE);
			/* wipe obstacles away so ranged chars vs melee chars won't end in people waiting inside trees */
			zcave = getcave(&wpos);
			for (x = 1; x < MAX_WID - 1; x++)
			for (y = 1; y < MAX_HGT - 1; y++) {
				c_ptr = &zcave[y][x];
				if (c_ptr->feat == FEAT_IVY ||
				    c_ptr->feat == FEAT_BUSH ||
				    c_ptr->feat == FEAT_DEAD_TREE ||
				    c_ptr->feat == FEAT_TREE)
					switch (ge->extra[2]) {
					case WILD_WASTELAND: c_ptr->feat = FEAT_DIRT; break;
					case WILD_GRASSLAND:
					default: c_ptr->feat = FEAT_GRASS; break;
					}
			}
			if (ge->extra[4]) {
				/* Generate the level from fixed arena layout */
				s_printf("EVENT_LAYOUT: Generating arena %d at %d,%d,%d\n", ge->extra[4], wpos.wx, wpos.wy, wpos.wz);
				process_dungeon_file(format("t_arena%d.txt", ge->extra[4]), &wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);
			}

			/* actually create temporary Highlander dungeon! */
			if (!wild_info[wpos.wy][wpos.wx].dungeon) {
				/* add staircase downwards into the dungeon? */
				if (!ge->extra[5]) {
					s_printf("EVENT_LAYOUT: Adding dungeon (no entry).\n");
					add_dungeon(&wpos, 1, 50, DF1_NO_RECALL, DF2_IRON | DF2_NO_EXIT_MASK |
					    DF2_NO_ENTRY_MASK,
					    DF3_NO_SIMPLE_STORES | DF3_NO_DUNGEON_BONUS | DF3_EXP_20, FALSE, 0, 0, 0, 0);
				} else {
					s_printf("EVENT_LAYOUT: Adding dungeon (entry ok).\n");
					add_dungeon(&wpos, 1, 50, DF1_NO_RECALL, DF2_IRON | DF2_NO_EXIT_MASK |
					    DF2_NO_ENTRY_WOR | DF2_NO_ENTRY_PROB | DF2_NO_ENTRY_FLOAT,
					    DF3_NO_SIMPLE_STORES | DF3_NO_DUNGEON_BONUS | DF3_EXP_20, FALSE, 0, 0, 0, 0);

					/* place staircase on an empty accessible grid */
					do {
						y = rand_int((MAX_HGT) - 3) + 1;
						x = rand_int((MAX_WID) - 3) + 1;
					} while (!cave_floor_bold(zcave, y, x)
					    && (++tries < 1000));
					zcave[y][x].feat = FEAT_MORE;
					/* remember for dungeon removal at the end */
					ge->extra[6] = x;
					ge->extra[7] = y;

					sector00downstairs++;
				}
			} else {
				s_printf("EVENT_LAYOUT: Dungeon already in place.\n");
			}

			sector00music_dun = 62;
			sector00musicalt_dun = 0;

			/* teleport the participants into the dungeon */
			for (j = 0; j < MAX_GE_PARTICIPANTS; j++) {
				if (!ge->participant[j]) continue;

				for (i = 1; i <= NumPlayers; i++) {
					if (Players[i]->id != ge->participant[j]) continue;

					p_ptr = Players[i];

					if (p_ptr->wpos.wx != WPOS_HIGHLANDER_X || p_ptr->wpos.wy != WPOS_HIGHLANDER_Y) {
						p_ptr->recall_pos.wx = WPOS_HIGHLANDER_X;
						p_ptr->recall_pos.wy = WPOS_HIGHLANDER_Y;
						p_ptr->recall_pos.wz = WPOS_HIGHLANDER_DUN_Z;
						p_ptr->global_event_temp = PEVF_PASS_00 | PEVF_NOGHOST_00 |
						    PEVF_SAFEDUN_00 | PEVF_SEPDUN_00;
						p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
						recall_player(i, "");
					}
					/* Give him the amulet of the highlands */
					invcopy(o_ptr, lookup_kind(TV_AMULET, SV_AMULET_HIGHLANDS));
					o_ptr->number = 1;
					o_ptr->level = 0;
					o_ptr->discount = 0;
					o_ptr->ident |= ID_MENTAL;
					o_ptr->owner = p_ptr->id;
					o_ptr->mode = p_ptr->mode;
					object_aware(i, o_ptr);
					object_known(o_ptr);
					inven_carry(i, o_ptr);
					/* may only take part in one tournament per char */
					gain_exp(i, 1);
					/* give some safe time for exp'ing */
#ifndef KURZEL_PK
					if (cfg.use_pk_rules == PK_RULES_DECLARE) {
						p_ptr->pkill &= ~PKILL_KILLABLE;
					}
#endif
					p_ptr->global_event_progress[ge_id][0] = 1; /* now in 0,0,0-dungeon! */
				}
			}

			ge->state[0] = 1;
			break;
		case 1: /* exp phase - end prematurely if all players pseudo-died in dungeon */
			n = 0;
			k = 0;
			for (i = 1; i <= NumPlayers; i++)
				if (!Players[i]->admin_dm && Players[i]->wpos.wx == WPOS_SECTOR00_X && Players[i]->wpos.wy == WPOS_SECTOR00_Y) {
					n++;
					j = i;
					/* count players who have already been kicked out of the dungeon by pseudo-dying */
					if (!Players[i]->wpos.wz) k++;
				}

			if (!n) ge->state[0] = 255; /* double kill by monsters or something? ew. */
			else if (n == 1) { /* early ending, everyone died to monsters in the dungeon */
				ge->state[0] = 6;
				ge->extra[3] = j;
			}
			else if ((n == k) && !ge->extra[5]) ge->state[0] = 3; /* all players are already at the surface,
										because all of them were defeated by monsters,
										and there's no staircase to re-enter the dungeon.. */
			else if (elapsed - ge->announcement_time >= 600 - 45) { /* give a warning, peace ends soon */
				for (i = 1; i <= NumPlayers; i++)
					if (Players[i]->wpos.wx == WPOS_SECTOR00_X && Players[i]->wpos.wy == WPOS_SECTOR00_Y)
						msg_print(i, "\377f[The slaughter will begin soon!]");
				ge->state[0] = 2;
			}
			break;
		case 2: /* final exp phase after the warning has been issued - end prematurely if needed (see above) */
			sector00music_dun = 63;
			sector00musicalt_dun = 0;

			n = 0;
			k = 0;
			for (i = 1; i <= NumPlayers; i++)
				if (!Players[i]->admin_dm && Players[i]->wpos.wx == WPOS_SECTOR00_X && Players[i]->wpos.wy == WPOS_SECTOR00_Y) {
					n++;
					j = i;
					if (!Players[i]->wpos.wz) k++;
					handle_music(i);
				}

			if (!n) ge->state[0] = 255; /* double kill or something? ew. */
			else if (n == 1) { /* early ending, everyone died to monsters in the dungeon */
				ge->state[0] = 6;
				ge->extra[3] = j;
			}
			else if ((n == k) && !ge->extra[5]) ge->state[0] = 3; /* start deathmatch already (see above, state 1) */
			else if (elapsed - ge->announcement_time >= 600) ge->state[0] = 3; /* start deathmatch phase */
			break;
		case 3: /* get people out of the dungeon (if not all pseudo-died already) and make them fight each other properly */
			/* remember time stamp when we entered deathmatch phase (for spawning a baddy) */
			ge->state[2] = turn - ge->start_turn; /* this keeps it /gefforward friendly */
			ge->state[3] = 0;
			sector00music = 47; /* death match theme */
			sector00musicalt = 0;

			/* got a staircase to remove? */
			if (ge->extra[5]) {
				zcave = getcave(&wpos);
				zcave[ge->extra[7]][ge->extra[6]].feat = FEAT_DIRT;
				everyone_lite_spot(&wpos, ge->extra[7], ge->extra[6]);
				sector00downstairs--;
				ge->extra[5] = 0;
			}

			for (i = 1; i <= NumPlayers; i++) {
				p_ptr = Players[i];
				if (p_ptr->admin_dm || p_ptr->wpos.wx != WPOS_SECTOR00_X || p_ptr->wpos.wy != WPOS_SECTOR00_Y) continue;

				if (p_ptr->party) {
					for (j = 1; j <= NumPlayers; j++) {
						if (j == i) continue;
						if (Players[j]->wpos.wx != WPOS_SECTOR00_X || Players[j]->wpos.wy != WPOS_SECTOR00_Y) continue;
						/* leave party */
						if (Players[j]->party == p_ptr->party) party_leave(i, FALSE);
					}
				}

				/* change "normal" Highlands amulet to v2 with ESP? */
				for (j = INVEN_TOTAL - 1; j >= 0; j--)
					if (p_ptr->inventory[j].tval == TV_AMULET && p_ptr->inventory[j].sval == SV_AMULET_HIGHLANDS) {
						invcopy(&p_ptr->inventory[j], lookup_kind(TV_AMULET, SV_AMULET_HIGHLANDS2));
						p_ptr->inventory[j].number = 1;
						p_ptr->inventory[j].level = 0;
						p_ptr->inventory[j].discount = 0;
						p_ptr->inventory[j].ident |= ID_MENTAL;
						p_ptr->inventory[j].owner = p_ptr->id;
						p_ptr->inventory[j].mode = p_ptr->mode;
						object_aware(i, &p_ptr->inventory[j]);
						object_known(&p_ptr->inventory[j]);
					}
				p_ptr->update |= (PU_BONUS | PU_VIEW);
				p_ptr->window |= (PW_INVEN | PW_EQUIP);
				handle_stuff(i);

				p_ptr->global_event_temp &= ~PEVF_SAFEDUN_00; /* no longer safe from death */
				p_ptr->global_event_temp |= PEVF_AUTOPVP_00;

				if (Players[i]->wpos.wz) {
					p_ptr->recall_pos.wx = WPOS_SECTOR00_X;
					p_ptr->recall_pos.wy = WPOS_SECTOR00_Y;
					p_ptr->recall_pos.wz = 0;
					p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
					recall_player(i, "");
				}

				p_ptr->global_event_progress[ge_id][0] = 4; /* now before deathmatch */
				msg_print(i, "\377fThe bloodshed begins!");
				handle_music(i);
			}

			ge->state[0] = 4;
			break;
		case 4: /* teleport them around first */
			/* NOTE: the no-tele vault stuff might just need fixing, ie ignoring the no-tele when auto-recalling */
			for (i = 1; i <= NumPlayers; i++)
				if (inarea(&Players[i]->wpos, &wpos)) {
					p_ptr = Players[i];
					wiz_lite(i); /* no tourneys at night, chars with low IV lose */
					teleport_player(i, 200, TRUE);
					/* in case some player waited in a NO_TELE vault..!: */
					if (p_ptr->wpos.wz && !p_ptr->admin_dm) {
						msg_print(i, "\377rThe whole dungeon suddenly COLLAPSES!");
						strcpy(p_ptr->died_from,"a mysterious accident");
						p_ptr->global_event_temp = PEVF_NONE; /* clear no-WoR/perma-death/no-death flags */
						p_ptr->deathblow = 0;
						player_death(i);
					}
					p_ptr->global_event_progress[ge_id][0] = 5; /* now in deathmatch */
				}
			ge->state[0] = 5;
			break;
		case 5: /* deathmatch phase -- might add some random teleportation for more fun */
			/* NOTE: the mysterious-accident is deprecated, since we use extra[5] now */
			n = 0;
			for (i = 1; i <= NumPlayers; i++) {
				if (Players[i]->admin_dm) continue;
				/* in case some player tries to go > again^^ */
				if (!Players[i]->wpos.wx && !Players[i]->wpos.wy && Players[i]->wpos.wz
				    && Players[i]->global_event_type[ge_id] == GE_HIGHLANDER) {
					msg_print(i, "\377rThe whole dungeon suddenly COLLAPSES!");
					strcpy(Players[i]->died_from,"a mysterious accident");
					Players[i]->global_event_temp = PEVF_NONE; /* clear no-WoR/perma-death/no-death flags */
					Players[i]->deathblow = 0;
					player_death(i);
				}
				if (inarea(&Players[i]->wpos, &wpos)) {
					n++;
					j = i;
				}
			}
			if (!n) ge->state[0] = 255; /* double kill or something? ew. */
			if (n == 1) { /* We have a winner! (not a total_winner but anyways..) */
				ge->state[0] = 6;
				ge->extra[3] = j;
			}

			/* if tournament runs for too long without result, spice it up by
			   throwing in some nasty baddy (Bad Luck Bat from Hell): */
			/* TODO: also spawn something if a player is AFK (or highlandering himself on friend's account -_-) */
//			if (elapsed_turns - (ge->announcement_time * cfg.fps) - 600 == 600) { /* after 10 minutes of deathmatch phase */
			if ((!ge->state[3]) && ((turn - ge->start_turn) - ge->state[2] >= 600 * cfg.fps)) {
				msg_broadcast(0, "\377aThe gods of highlands are displeased by the lack of blood flowing.");
				summon_override_checks = SO_ALL & ~(SO_GRID_EMPTY);
				while (!(summon_detailed_one_somewhere(&wpos, RI_BAD_LUCK_BAT, 0, FALSE, 101)) && (++tries < 1000));
				summon_override_checks = SO_NONE;
				ge->state[3] = 1; /* remember that we already spawned one so we don't keep spawning */
					/* this actually serves if an admin /gefforward's too far, beyond the spawning
					time, so we can still spawn one without the admin taking too much care.. */
			}

			break;
		case 6: /* we have a winner! */
			j = ge->extra[3];
			if (j > NumPlayers) { /* Make sure the winner didn't die in the 1 turn that just passed! */
				ge->state[0] = 255; /* no winner, d'oh */
				break;
			}

			p_ptr = Players[j];
			if (!in_sector00(&p_ptr->wpos)) { /* not ok.. */
				ge->state[0] = 255; /* no winner, d'oh */
				break;
			}

			sprintf(buf, "\374\377a>>%s wins %s!<<", p_ptr->name, ge->title);
			msg_broadcast(0, buf);
#ifdef TOMENET_WORLDS
			if (cfg.worldd_events) world_msg(buf);
#endif
#ifdef USE_SOUND_2010
			sound(j, "success", NULL, SFX_TYPE_MISC, FALSE);
#endif
			if (!p_ptr->max_exp) gain_exp(j, 1); /* may only take part in one tournament per char */

			/* don't create a actual reward here, but just a signed deed that can be turned in (at mayor's office)! */
			k = lookup_kind(TV_PARCHMENT, SV_DEED_HIGHLANDER);
			invcopy(o_ptr, k);
			o_ptr->number = 1;
			object_aware(j, o_ptr);
			object_known(o_ptr);
			o_ptr->discount = 0;
			o_ptr->level = 0;
			o_ptr->ident |= ID_MENTAL;
			//o_ptr->note = quark_add("Tournament reward");
			inven_carry(j, o_ptr);

			s_printf("%s EVENT_WON: %s wins %d (%s)\n", showtime(), p_ptr->name, ge_id + 1, ge->title);
			l_printf("%s \\{s%s has won %s\n", showdate(), p_ptr->name, ge->title);

			/* avoid him dying */
			set_poisoned(j, 0, 0);
			set_cut(j, 0, 0);
			set_food(j, PY_FOOD_FULL);
			hp_player_quiet(j, 5000, TRUE);

			ge->state[0] = 7;
			ge->state[1] = elapsed;
			break;
		case 7: /* chill out for a few seconds (or get killed -- but now there aren't monster spawns anymore) */
			if (elapsed - ge->state[1] >= 5) ge->state[0] = 255;
			break;
		case 255: /* clean-up */
			if (!ge->cleanup) {
				ge->getype = GE_NONE; /* end of event */
				break;
			}

			for (i = 1; i <= NumPlayers; i++) {
				p_ptr = Players[i];
				if (in_sector00(&p_ptr->wpos)) {
					for (j = INVEN_TOTAL; j >= 0; j--) /* Erase the highlander amulets */
						if (p_ptr->inventory[j].tval == TV_AMULET &&
						    ((p_ptr->inventory[j].sval == SV_AMULET_HIGHLANDS) ||
						    (p_ptr->inventory[j].sval == SV_AMULET_HIGHLANDS2))) {
							inven_item_increase(i, j, -p_ptr->inventory[j].number);
							inven_item_optimize(i, j);
						}
					p_ptr->global_event_type[ge_id] = GE_NONE; /* no longer participant */
					p_ptr->recall_pos.wx = cfg.town_x;
					p_ptr->recall_pos.wy = cfg.town_y;
					p_ptr->recall_pos.wz = 0;
					p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
					p_ptr->global_event_temp = PEVF_PASS_00; /* clear all other flags, allow a final recall out */
					recall_player(i, "");
					/* required, or the double-wpos-change in below's 'unstatic_level()' call will cause
					   panic save due to old wpos possibly still being in non-existant highlander dungeon */
					process_player_change_wpos(i);
				}
			}

			sector00flags1 = sector00flags2 = 0x0;
			sector00separation--;

			/* still got a staircase to remove? */
			if (ge->extra[5]) {
				zcave = getcave(&wpos);
				zcave[ge->extra[7]][ge->extra[6]].feat = FEAT_DIRT;
				everyone_lite_spot(&wpos, ge->extra[7], ge->extra[6]);
				sector00downstairs--;
				ge->extra[5] = 0;
			}

			/* remove temporary Highlander dungeon! */
			if (wild_info[wpos.wy][wpos.wx].dungeon)
				rem_dungeon(&wpos, FALSE);

			wild_info[wpos.wy][wpos.wx].type = ge->extra[1];
			wipe_m_list(&wpos); /* clear any (powerful) spawns */
			wipe_o_list_safely(&wpos); /* and objects too */
			unstatic_level(&wpos);/* get rid of any other person, by unstaticing ;) */

			ge->getype = GE_NONE; /* end of event */
			break;
		}
		break;

	/* Arena Monster Challenge */
	case GE_ARENA_MONSTER:
		wpos.wx = cfg.town_x;
		wpos.wy = cfg.town_y;
		wild = &wild_info[wpos.wy][wpos.wx];
		d_ptr = wild->tower;
		if (!d_ptr) {
			s_printf("FATAL_ERROR: global_event 'Arena Monster Challenge': Bree has no Training Tower.\n");
			return; /* paranoia, Bree should *always* have 'The Training Tower' */
		}
		wpos.wz = d_ptr->maxdepth;

		switch(ge->state[0]){
		case 0: /* prepare */
#if 0 /* disabled unstaticing for now since it might unstatice the whole 32,32 sector on all depths? dunno */
			unstatic_level(&wpos);/* get rid of any other person, by unstaticing ;) */
#else
			for (i = 1; i <= NumPlayers; i++)
				if (inarea(&Players[i]->wpos, &wpos)) {
					/* Give him his hit points back */
					set_cut(i, 0, 0);
					Players[i]->chp = Players[i]->mhp;
					Players[i]->chp_frac = 0;
					Players[i]->redraw |= PR_HP;

					Players[i]->new_level_method = (Players[i]->wpos.wz > 0 ? LEVEL_RECALL_DOWN : LEVEL_RECALL_UP);
					Players[i]->recall_pos.wx = wpos.wx;
					Players[i]->recall_pos.wy = wpos.wy;
					Players[i]->recall_pos.wz = 0;
					recall_player(i, "\377yThe arena wizards teleport you out of here!");
				}
			if (getcave(&wpos)) { /* check that the level is allocated - mikaelh */
				dealloc_dungeon_level(&wpos);
			}
#endif

			ge_special_sector++;

			if (!getcave(&wpos)) {
				alloc_dungeon_level(&wpos);
				generate_cave(&wpos, NULL); /* should work =p (see make_resf) */
			}

			new_players_on_depth(&wpos, 1, TRUE); /* make it static */
			s_printf("EVENT_LAYOUT: Generating arena_tt at %d,%d,%d\n", wpos.wx, wpos.wy, wpos.wz);
			process_dungeon_file("t_arena_tt.txt", &wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);

			wipe_m_list(&wpos); /* clear any (powerful) spawns */
			wipe_o_list_safely(&wpos); /* and objects too */
			ge->state[0] = 1;
			break;
		case 1: /* running - not much to do here actually :) it's all handled by global_event_signup */
			if (ge->extra[1]) { /* new challenge to process? */
				if (!getcave(&wpos)) { /* in case nobody was there for a long enough time to unstatice, no idea when/if though.. */
					alloc_dungeon_level(&wpos);
					generate_cave(&wpos, NULL); /* should work =p (see make_resf) */
					new_players_on_depth(&wpos, 1, FALSE); /* make it static */
				}

				wipe_m_list(&wpos); /* get rid of previous monster */
				summon_override_checks = SO_ALL & ~(SO_PROTECTED | SO_GRID_EMPTY);
#ifndef GE_ARENA_ALLOW_EGO
				while (!summon_specific_race_somewhere(&wpos, ge->extra[1], 100, 1) /* summon new monster */
				    && (++tries < 1000));
#else
				while (!(m_idx = summon_detailed_one_somewhere(&wpos, ge->extra[1], ge->extra[3], FALSE, 101))
				    && (++tries < 1000));
				monster_desc(0, m_name, m_idx, 0x08);
				msg_broadcast_format(0, "\376\377c** %s challenges %s! **", Players[ge->extra[5]]->name, m_name);
#endif
				summon_override_checks = SO_NONE;

				ge->extra[2] = ge->extra[1]; /* remember it for result announcement later */
#ifdef GE_ARENA_ALLOW_EGO
				ge->extra[4] = ge->extra[3]; /* remember it for result announcement later */
#endif
				ge->extra[1] = 0;
			}
			break;
		case 255: /* clean-up */
			if (getcave(&wpos)) {
				wipe_m_list(&wpos); /* clear any (powerful) spawns */
				wipe_o_list_safely(&wpos); /* and objects too */
#if 0 /* disabled unstaticing for now since it might unstatice the whole 32,32 sector on all depths? dunno */
				unstatic_level(&wpos);/* get rid of any other person, by unstaticing ;) */
#else
				new_players_on_depth(&wpos, -1, TRUE); /* remove forced staticness */

				for (i = 1; i <= NumPlayers; i++)
					if (inarea(&Players[i]->wpos, &wpos)) {
						Players[i]->new_level_method = (Players[i]->wpos.wz > 0 ? LEVEL_RECALL_DOWN : LEVEL_RECALL_UP);
						Players[i]->recall_pos.wx = wpos.wx;
						Players[i]->recall_pos.wy = wpos.wy;
						Players[i]->recall_pos.wz = 0;
						recall_player(i, "\377yThe arena wizards teleport you out of here!");
#ifdef USE_SOUND_2010
						sound(i, "failure", NULL, SFX_TYPE_MISC, FALSE);
#endif
					}
				if (getcave(&wpos)) { /* check that the level is allocated - mikaelh */
					dealloc_dungeon_level(&wpos);
				}
#endif /* so if if0'ed, we just have to wait for normal unstaticing routine to take care of stale level :/ */
			}
			ge->getype = GE_NONE; /* end of event */
			ge_special_sector--;
			break;
		}
		break;

	/* Dungeon Keeper labyrinth race */
	case GE_DUNGEON_KEEPER:
		switch (ge->state[0]) {
		case 0: { /* prepare level, gather everyone, begin */
			int bx[3], by[3];

			ge->state[1] = 0;
			ge->cleanup = 1;
			sector00separation++; /* separate sector 0,0 from the worldmap - participants have access ONLY */
			sector00music = 64;
			sector00musicalt = 46; /* terrifying (notele) music */
			sector00flags1 = LF1_NO_MAGIC_MAP;
			sector00flags2 = LF2_NO_RUN | LF2_NO_TELE | LF2_NO_DETECT | LF2_NO_ESP | LF2_NO_SPEED | LF2_NO_RES_HEAL | LF2_FAIR_TERRAIN_DAM | LF2_INDOORS;
			sector00wall = FEAT_PERM_INNER; //FEAT_PERM_SOLID gets shaded to slate :/
			wipe_m_list(&wpos); /* clear any (powerful) spawns */
			wipe_o_list_safely(&wpos); /* and objects too */
			unstatic_level(&wpos);/* get rid of any other person, by unstaticing ;) */

			if (!getcave(&wpos)) alloc_dungeon_level(&wpos);
			s_printf("EVENT_LAYOUT: Generating labyrinth at %d,%d,%d\n", wpos.wx, wpos.wy, wpos.wz);
			/* make it static */
			new_players_on_depth(&wpos, 1, FALSE);
			zcave = getcave(&wpos);

			/* wipe level with floor tiles */
			for (x = 0; x < MAX_WID; x++)
			for (y = 0; y < MAX_HGT; y++) {
				zcave[y][x].feat = FEAT_ASH; /* scary ;) */
				zcave[y][x].info |= CAVE_STCK;//| CAVE_GLOW;
				zcave[y][x].info &= ~CAVE_GLOW; /* scarier! */
			}

			/* add perma wall borders and basic labyrinth grid (rooms) */
			for (x = 0; x < MAX_WID; x++) {
				for (y = 4; y <= MAX_HGT - 4; y += 4) {
					if (x < MAX_WID - 1) zcave[y][x].feat = FEAT_PERM_INNER;
				}
				/* comply with wilderness generation scheme (or crash when approaching) */
				zcave[0][x].feat = FEAT_PERM_CLEAR;
				zcave[MAX_HGT - 1][x].feat = FEAT_PERM_CLEAR;
			}
			for (y = 0; y < MAX_HGT; y++) {
				for (x = 4; x <= MAX_WID - 4; x += 4) {
					if (y < MAX_HGT - 1) zcave[y][x].feat = FEAT_PERM_INNER;
				}
				/* comply with wilderness generation scheme (or crash when approaching) */
				zcave[y][0].feat = FEAT_PERM_CLEAR;
				zcave[y][MAX_WID - 1].feat = FEAT_PERM_CLEAR;
			}

			/* generate max # of doors, then remove some randomly */

			/* pass 1: add all and remove some doors */

			/* add all possible doors, vertically and horizontally */
			for (x = 2; x < MAX_WID - 1; x += 4)
			for (y = 4; y <= MAX_HGT - 4; y += 4) {
//				if (zcave[y][x].feat != FEAT_PERM_INNER) continue;
				zcave[y][x].feat = FEAT_DOOR_HEAD;
			}
			for (y = 2; y < MAX_HGT - 1; y += 4)
			for (x = 4; x <= MAX_WID - 4; x += 4) {
//				if (zcave[y][x].feat != FEAT_PERM_INNER) continue;
				zcave[y][x].feat = FEAT_DOOR_HEAD;
			}

			/* remove 0..1. Maybe even ..2? */
			for (x = 2; x < MAX_WID - 1; x += 4)
			for (y = 2; y < MAX_HGT - 1; y += 4) {
				int door_pos[4], doors = 0, doors_exist = 0; /* door_pos are same as dd arrays, ie numpad dirs */

				/* we're at room centre. Process N/E/S/W doors. */
				if (y > 2) { /* skip N door when at top border */
					/* check for already existing door */
					if (zcave[y + 2 * ddy[8]][x + 2 * ddx[8]].feat == FEAT_DOOR_HEAD) {
						door_pos[doors_exist] = 8;
						doors_exist++;
					} else doors++;
				}
				if (x < MAX_WID - 1 - (MAX_WID - 1) % 4 - 3) { /* skip E door when at right border */
					/* check for already existing door */
					if (zcave[y + 2 * ddy[6]][x + 2 * ddx[6]].feat == FEAT_DOOR_HEAD) {
						door_pos[doors_exist] = 6;
						doors_exist++;
					} else doors++;
				}
				if (y < MAX_HGT - (MAX_HGT - 1) % 4 - 3) { /* skip S door when at bottom border */
					/* check for already existing door */
					if (zcave[y + 2 * ddy[2]][x + 2 * ddx[2]].feat == FEAT_DOOR_HEAD) {
						door_pos[doors_exist] = 2;
						doors_exist++;
					} else doors++;
				}
				if (x > 2) { /* skip W door when at left border */
					/* check for already existing door */
					if (zcave[y + 2 * ddy[4]][x + 2 * ddx[4]].feat == FEAT_DOOR_HEAD) {
						door_pos[doors_exist] = 4;
						doors_exist++;
					} else doors++;
				}

				/* we already have all non-doors we need? (created by the adjacent rooms) */
				if (doors_exist <= 2 || /* 1 or 2 non-doors exist, or it's a corner */
				    (doors_exist == 3 && doors == 1) || /* sufficient # of non-doors for a centre room (1 out of 4) */
				    (doors_exist == 3 && !rand_int(2))) /* edge rooms don't need non-doors, but sometimes add one anyway (-> 1 out of 3) */
					continue;

				/* create sufficient number of missing doors */
				if (doors_exist == 3) {
					/* scenario (doors == 0): edge room with 3 doors, we want to remove 1 */
					k = rand_int(3);
					zcave[y + 2 * ddy[door_pos[k]]][x + 2 * ddx[door_pos[k]]].feat = FEAT_PERM_INNER;
				} else {
					/* scenario: centre room with 4 doors, we need to remove 1 */
					k = rand_int(4);
					zcave[y + 2 * ddy[door_pos[k]]][x + 2 * ddx[door_pos[k]]].feat = FEAT_PERM_INNER;
				}
			}

			/* pass 2 (sanity): make sure each room has at very least ONE door */

			for (x = 2; x < MAX_WID - 1; x += 4)
			for (y = 2; y < MAX_HGT - 1; y += 4) {
				int door_pos[4], doors = 0; /* door_pos are same as dd arrays, ie numpad dirs */

				/* we're at room centre. Process N/E/S/W doors. */
				if (y > 2) { /* skip N door when at top border */
					/* check for already existing door */
					if (zcave[y + 2 * ddy[8]][x + 2 * ddx[8]].feat == FEAT_DOOR_HEAD) continue;
					else {
						door_pos[doors] = 8;
						doors++;
					}
				}
				if (x < MAX_WID - 1 - (MAX_WID - 1) % 4 - 3) { /* skip E door when at right border */
					/* check for already existing door */
					if (zcave[y + 2 * ddy[6]][x + 2 * ddx[6]].feat == FEAT_DOOR_HEAD) continue;
					else {
						door_pos[doors] = 6;
						doors++;
					}
				}
				if (y < MAX_HGT - (MAX_HGT - 1) % 4 - 3) { /* skip S door when at bottom border */
					/* check for already existing door */
					if (zcave[y + 2 * ddy[2]][x + 2 * ddx[2]].feat == FEAT_DOOR_HEAD) continue;
					else {
						door_pos[doors] = 2;
						doors++;
					}
				}
				if (x > 2) { /* skip W door when at left border */
					/* check for already existing door */
					if (zcave[y + 2 * ddy[4]][x + 2 * ddx[4]].feat == FEAT_DOOR_HEAD) continue;
					else {
						door_pos[doors] = 4;
						doors++;
					}
				}

				if (!doors) continue; /* paranoia */
				/* room already has NO door! */

				/* randomly create one */
				k = rand_int(doors);
				zcave[y + 2 * ddy[door_pos[k]]][x + 2 * ddx[door_pos[k]]].feat = FEAT_DOOR_HEAD;
			}

#if 1
			/* maybe - randomly add void gate pairs */
			k = 0;
			for (i = 0; i < 6; i++) {
				n = 1000;
				while (--n) {
					x = rand_int(MAX_WID - 1) + 1;
					y = rand_int(MAX_HGT - 1) + 1;
					if ((f_info[zcave[y][x].feat].flags1 & FF1_FLOOR) &&
					    !(f_info[zcave[y][x].feat].flags1 & FF1_DOOR))
						break;
				}
				if (!n) continue;
				place_between_ext(&wpos, y, x, MAX_HGT, MAX_WID);
				k++;
			}
			if (!k) s_printf("..COULDN'T PLACE void jump gates\n");
			else s_printf("..placed %d/6 void jump gates\n", k);
#endif

			/* place exit beacons [3 -> ~25% win?] */
			k = 0;
			for (i = 0; i < 4; i++) {
				n = 10000;
				while (--n) {
#if 0 /* place them anywhere in a chamber */
					x = rand_int(MAX_WID - 1) + 1;
					y = rand_int(MAX_HGT - 1) + 1;
#else /* place them in the center of a chamber */
					x = rand_int(MAX_WID / 4) * 4 + 2;
					y = rand_int(MAX_HGT / 4) * 4 + 2;
#endif
					/* only place it on floor (that hasn't got a gate/beacon on it yet) */
					if ((f_info[zcave[y][x].feat].flags1 & FF1_FLOOR) &&
					    !(f_info[zcave[y][x].feat].flags1 & FF1_DOOR) &&
					    !(f_info[zcave[y][x].feat].flags1 & FF1_PERMANENT))
						break;
				}
				if (!n) continue;
				cave_set_feat_live(&wpos, y, x, FEAT_BEACON);
				zcave[y][x].info |= CAVE_GLOW; /* make it 100% non-annoying ^^ */
				bx[k] = x; by[k] = y;
				k++;
			}
			if (!k) s_printf("..COULDN'T PLACE exit beacons\n");
			else s_printf("..placed %d/4 exit beacons\n", k);

			/* place Horned Reaper :D */
			n = 10000;
			while (--n) {
				x = rand_int(MAX_WID - 1) + 1;
				y = rand_int(MAX_HGT - 1) + 1;
				if ((f_info[zcave[y][x].feat].flags1 & FF1_FLOOR) &&
				    !(f_info[zcave[y][x].feat].flags1 & FF1_DOOR))
					break;
			}
			if (!n) s_printf("..COULDN'T PLACE Horned Reaper\n");
			else {
				summon_override_checks = SO_ALL;
				place_monster_one(&wpos, y, x, RI_HORNED_REAPER_GE, FALSE, FALSE, FALSE, 0, 0);
				summon_override_checks = SO_NONE;
				s_printf("..placed Horned Reaper\n");
			}

			/* teleport the players in */
			for (j = 0; j < MAX_GE_PARTICIPANTS; j++) {
				if (!ge->participant[j]) continue;

				for (i = 1; i <= NumPlayers; i++) {
					if (Players[i]->id != ge->participant[j]) continue;
					p_ptr = Players[i];

					if (p_ptr->wpos.wx != WPOS_SECTOR00_X || p_ptr->wpos.wy != WPOS_SECTOR00_Y
					    || p_ptr->wpos.wz != WPOS_SECTOR00_Z) {
						p_ptr->recall_pos.wx = WPOS_SECTOR00_X;
						p_ptr->recall_pos.wy = WPOS_SECTOR00_Y;
						p_ptr->recall_pos.wz = WPOS_SECTOR00_Z;
						p_ptr->global_event_temp = PEVF_PASS_00;
						p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
						/* don't spawn them too close to a beacon */
						p_ptr->avoid_loc = k;
						p_ptr->avoid_loc_x = bx;
						p_ptr->avoid_loc_y = by;
						recall_player(i, "");
						p_ptr->avoid_loc = 0;
					}
					p_ptr->global_event_temp |= PEVF_NOGHOST_00 | PEVF_NO_RUN_00 | PEVF_NOTELE_00 | PEVF_INDOORS_00 | PEVF_STCK_OK;
					p_ptr->update |= PU_BONUS;
					p_ptr->global_event_progress[ge_id][0] = 1; /* now in 0,0,0 sector */

					/* make sure they stop running (not really needed though..) */
					disturb(i, 0, 0);
					handle_music(i);
				}
			}

			ge->state[0] = 1;
			break;
			}
		case 1: /* hunt/race phase - end if all players escape or die or after a timeout */
			/* everyone has escaped or died? */
			n = 0;
			for (i = 1; i <= NumPlayers; i++)
				if (!Players[i]->admin_dm && in_sector00(&Players[i]->wpos))
					n++;
			if (!n) {
				ge->state[0] = 255;
				break;
			}

			/* timeout not yet reached? proceed normally */
			if (elapsed - ge->announcement_time < 300) break;//start after 300s

			sector00music = 65;
			sector00musicalt = 47; /* death match music */
			for (i = 1; i <= NumPlayers; i++)
				if (!Players[i]->admin_dm && in_sector00(&Players[i]->wpos))
					handle_music(i);

			ge->state[0] = 2;
			break;
		case 2: /* fill the labyrinth with more and more lava ^^- */
			/* everyone has escaped or died? */
			n = 0;
			for (i = 1; i <= NumPlayers; i++)
				if (!Players[i]->admin_dm && in_sector00(&Players[i]->wpos))
					n++;
			if (!n) {
				ge->state[0] = 255;
				break;
			}

			n = elapsed; /* for how long is the event already up? */
			n -= ge->announcement_time; /* dont factor in the announcement time */
			n -= 300; /* don't factor in the duration of 1st phase */
			n /= 5; /* from here on, act in 5 second intervals */
			if (ge->state[1] >= n || /* next 5s interval is not yet up? */
			    n > 180 / 5) /* we're already 3 min into this phase and termination is imminent */
				break;
			ge->state[1] = n; /* advance into another lava-filling step (lock time interval semaphore) */

			zcave = getcave(&wpos);
			/* after 3 min in pase 2, fill EVERYTHING -> everyone will die <(* *)> */
			if (n == 180 / 5) {
				for (x = 1; x < MAX_WID - 1; x++)
				for (y = 1; y < MAX_HGT - 1; y++)
					if (zcave[y][x].feat != FEAT_PERM_INNER) {
						//cave_set_feat_live(&wpos, y, x, FEAT_DEEP_LAVA);
						zcave[y][x].feat = FEAT_DEEP_LAVA;
						everyone_lite_spot(&wpos, y, x);
					}
				break;
			}
			/* if it's not that late yet, just fill some lava.. */
			for (i = 0; i < 50 + n * 5; i++) {
				j = 100;
				while (--j) {
					x = rand_int(MAX_WID - 1) + 1;
					y = rand_int(MAX_HGT - 1) + 1;
					if ((f_info[zcave[y][x].feat].flags1 & FF1_FLOOR) &&
					    !(f_info[zcave[y][x].feat].flags1 & FF1_DOOR) &&
					    zcave[y][x].feat != FEAT_BEACON &&
					    zcave[y][x].feat != FEAT_DEEP_LAVA)
						break;
				}
				if (!j) continue;
				//cave_set_feat_live(&wpos, y, x, FEAT_DEEP_LAVA);
				zcave[y][x].feat = FEAT_DEEP_LAVA;
				everyone_lite_spot(&wpos, y, x);
			}
			break;
		case 255: /* clean-up */
			if (!ge->cleanup) {
				ge->getype = GE_NONE; /* end of event */
				break;
			}

			sector00flags1 = sector00flags2 = 0x0;
			sector00separation--;

			/* cleanly teleport all lingering admins out instead of displacing them into (non-generated) pvp-dungeon ^^ */
			for (i = 1; i <= NumPlayers; i++)
				if (inarea(&Players[i]->wpos, &wpos)) {
					Players[i]->new_level_method = LEVEL_OUTSIDE_RAND;
					Players[i]->recall_pos.wx = cfg.town_x;
					Players[i]->recall_pos.wy = cfg.town_y;
					Players[i]->recall_pos.wz = 0;
					recall_player(i, "");
				}

			wipe_m_list(&wpos); /* clear any (powerful) spawns */
			wipe_o_list_safely(&wpos); /* and objects too */
			unstatic_level(&wpos);/* get rid of any other person, by unstaticing ;) */

			ge->getype = GE_NONE; /* end of event */
			break;
		}
		break;

	default: /* generic clean-up routine for untitled events */
		switch (ge->state[0]) {
		case 255: /* remove an untitled event that has been stopped */
			ge->getype = GE_NONE;
			break;
		}
	}
	
	/* Check for end of event */
	if (ge->getype == GE_NONE) {
		msg_broadcast_format(0, "\374\377W[%s has ended]", ge->title);
		s_printf("%s EVENT_END: %d - '%s'.\n", showtime(), ge_id + 1, ge->title);
	}
}

/*
 * Process running global_events - C. Blue
 */
void process_global_events(void) {
	int n;
	for (n = 0; n < MAX_GLOBAL_EVENTS; n++)
		if (global_event[n].getype != GE_NONE) {
			if (!global_event[n].paused) process_global_event(n);
			else global_event[n].paused_turns++;
		}
}

/*
 * Update the 'tomenet.check' file
 * Currently used by the hangcheck script
 *  - mikaelh
 */
void update_check_file(void) {
	FILE *fp;
	char buf[1024];
	path_build(buf, 1024, ANGBAND_DIR_DATA, "tomenet.check");
	fp = fopen(buf, "wb");
	if (fp) {
		/* print the current timestamp into the file */
		fprintf(fp, "%d\n", (int)time(NULL));
		fclose(fp);
	}
}


/*
 * Clear all current_* variables used by Handle_item() - mikaelh
 */
void clear_current(int Ind) {
	player_type *p_ptr = Players[Ind];

	p_ptr->using_up_item = -1;

	p_ptr->current_enchant_h = 0;
	p_ptr->current_enchant_d = 0;
	p_ptr->current_enchant_a = 0;

	p_ptr->current_identify = 0;
	p_ptr->current_star_identify = 0;
	p_ptr->current_recharge = 0;
	p_ptr->current_artifact = 0;
	p_ptr->current_artifact_nolife = FALSE;
	p_ptr->current_curse = 0;
	p_ptr->current_tome_creation = 0;
	p_ptr->current_rune = 0;
	p_ptr->current_telekinesis = NULL;
}

void calc_techniques(int Ind) {
	player_type *p_ptr = Players[Ind];

	p_ptr->melee_techniques = MT_NONE;
	p_ptr->ranged_techniques = RT_NONE;

	if (mtech_lev[p_ptr->pclass][0] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][0])
		p_ptr->melee_techniques |= MT_SPRINT;
	if (mtech_lev[p_ptr->pclass][1] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][1])
		p_ptr->melee_techniques |= MT_TAUNT;
	if (mtech_lev[p_ptr->pclass][2] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][2])
		p_ptr->melee_techniques |= MT_JUMP;
	if (mtech_lev[p_ptr->pclass][3] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][3])
		p_ptr->melee_techniques |= MT_DISTRACT;
	if (mtech_lev[p_ptr->pclass][4] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][4])
		p_ptr->melee_techniques |= MT_POISON;
	if (mtech_lev[p_ptr->pclass][5] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][5])
		p_ptr->melee_techniques |= MT_KNOCK;
	if (mtech_lev[p_ptr->pclass][6] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][6])
		p_ptr->melee_techniques |= MT_TRACKANIM;
	if (mtech_lev[p_ptr->pclass][7] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][7])
		p_ptr->melee_techniques |= MT_DETNOISE;
	if (mtech_lev[p_ptr->pclass][8] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][8])
		p_ptr->melee_techniques |= MT_FLASH;
	if (mtech_lev[p_ptr->pclass][9] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][9])
		p_ptr->melee_techniques |= MT_CLOAK;
	if (mtech_lev[p_ptr->pclass][10] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][10])
		p_ptr->melee_techniques |= MT_SPIN;
	if (mtech_lev[p_ptr->pclass][11] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][11])
		p_ptr->melee_techniques |= MT_ASSA;
	if (mtech_lev[p_ptr->pclass][12] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][12])
		p_ptr->melee_techniques |= MT_BERSERK;
	if (mtech_lev[p_ptr->pclass][13] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][13])
		p_ptr->melee_techniques |= MT_SJUMP;
	if (mtech_lev[p_ptr->pclass][14] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][14])
		p_ptr->melee_techniques |= MT_SRUN;
	if (mtech_lev[p_ptr->pclass][15] &&
	    p_ptr->lev >= mtech_lev[p_ptr->pclass][15])
		p_ptr->melee_techniques |= MT_ICLOAK;

	if (get_skill(p_ptr, SKILL_ARCHERY) >= 4) p_ptr->ranged_techniques |= RT_FLARE; /* Flare missile */
	if (get_skill(p_ptr, SKILL_ARCHERY) >= 8) p_ptr->ranged_techniques |= RT_PRECS; /* Precision shot */
	if (get_skill(p_ptr, SKILL_ARCHERY) >= 10) p_ptr->ranged_techniques |= RT_CRAFT; /* Craft some ammunition */
	if (get_skill(p_ptr, SKILL_ARCHERY) >= 16) p_ptr->ranged_techniques |= RT_DOUBLE; /* Double-shot */
	if (get_skill(p_ptr, SKILL_ARCHERY) >= 25) p_ptr->ranged_techniques |= RT_BARRAGE; /* Barrage */

	Send_technique_info(Ind);
}

/* helper function to provide shortcut for checking for mind-linked player.
   flags == 0x0 means 'accept all flags'. */
int get_esp_link(int Ind, u32b flags, player_type **p2_ptr) {
	player_type *p_ptr = Players[Ind];
	int Ind2 = 0;
	(*p2_ptr) = NULL;

	if (p_ptr->esp_link_type &&
	    p_ptr->esp_link &&
	    ((p_ptr->esp_link_flags & flags) || flags == 0x0)) {
		Ind2 = find_player(p_ptr->esp_link);
		if (!Ind2) end_mind(Ind, FALSE);
		else (*p2_ptr) = Players[Ind2];
	}
	return Ind2;
}
/* helper function to provide shortcut for controlling mind-linked player.
   flags == 0x0 means 'accept all flags'. */
//void use_esp_link(int *Ind, u32b flags, player_type *p_ptr) {
void use_esp_link(int *Ind, u32b flags) {
	int Ind2;
//	p_ptr = Players[*Ind];
	player_type *p_ptr = Players[(*Ind)];

	if (p_ptr->esp_link_type &&
	    p_ptr->esp_link &&
	    ((p_ptr->esp_link_flags & flags) || flags == 0x0)) {
		Ind2 = find_player(p_ptr->esp_link);
		if (!Ind2) end_mind((*Ind), FALSE);
		else {
//			p_ptr = Players[Ind2];
			(*Ind) = Ind2;
		}
	}
}

/* Handle string input request replies */
void handle_request_return_str(int Ind, int id, char *str) {
	player_type *p_ptr = Players[Ind];

	/* verify that the ID is actually valid (maybe clear p_ptr->request_id here too) */
	if (id != p_ptr->request_id) return;

	/* request done */
	p_ptr->request_id = RID_NONE;

	/* verify that a string had been requested */
	if (RTYPE_STR != p_ptr->request_type) return;

	/* quests occupy an id broadband */
	if (id >= RID_QUEST) {
		//DEBUG
		if (str[0] == '\e')
			s_printf("RID_QUEST: (%d) '%s' replied <ESC>'%s'\n", id, p_ptr->name, str + 1);
		else
			s_printf("RID_QUEST: (%d) '%s' replied '%s'\n", id, p_ptr->name, str);

		str[30] = '\0'; /* arbitrary buffer limit */
		quest_reply(Ind, id - RID_QUEST, str);
		return;
	}

	switch (id) {
#ifdef ENABLE_GO_GAME
	case RID_GO_MOVE:
		if (p_ptr->store_num == -1) return; /* Discard if we left the building */
		str[160] = '\0'; /* prevent possile buffer overflow */
		go_engine_move_human(Ind, str);
		return;
#endif
	case RID_GUILD_RENAME:
		str[40] = '\0'; /* prevent possile buffer overflow */
		if (str[0] == '\e' || !str[0]) return; /* user ESCaped */
		guild_rename(Ind, str);
		return;
#ifdef ENABLE_ITEM_ORDER
	case RID_ITEM_ORDER: {
		int i, j, num, extra = -1;
		char str2[40], *str2ptr = str2;
		store_type *st_ptr;
		owner_type *ot_ptr;
		s64b price;
		object_type forge;
		char o_name[ONAME_LEN];
		bool old_rand;
		u32b tmp_seed;

		if (p_ptr->item_order_store != 0) {
			if (p_ptr->item_order_store - 1 == p_ptr->store_num && p_ptr->item_order_town == gettown(Ind))
				msg_print(Ind, "\377yYou still have an order open at this store!");
			else
				msg_format(Ind, "\377yYou still have an order open at the %s in %s!",
				    st_name + st_info[p_ptr->item_order_store - 1].name, town_profile[town[p_ptr->item_order_town].type].name);
			return;
		}

		str[40] = '\0';

		/* trim */
		while (*str == ' ') str++;
		while (str[strlen(str - 1)] == ' ') str[strlen(str - 1)] = 0;
		if (!(*str)) return;
		for (i = 0; i < strlen(str) - 1; i++) {
			if (str[i] == ' ' && str[i + 1] == ' ') continue;
			*str2ptr = str[i];
			str2ptr++;
		}
		*str2ptr = str[i];
		*(str2ptr + 1) = 0;

		/* allow specifying a number */
		str2ptr = str2;
		if (str2[0] == 'a') {
			if (str2[1] == ' ') {
				num = 1;
				str2ptr += 2;
			} else if (str2[1] == 'n' && str2[2] == ' ') {
				num = 1;
				str2ptr += 3;
			} else num = 1;
		} else if ((str2[0] == 'n' && str2[1] == 'o' && str2[2] == ' ') ||
		    (str2[0] == '0' && str2[1] == ' ') ||
		    (str2[0] == '0' && !i)) {
			msg_print(Ind, "Very funny, friend. Now stop wasting my time!");
			return;
		} else if ((i = atoi(str2))) {
			num = i;
			while (*str2ptr >= '0' && *str2ptr <= '9') str2ptr++;
			if (*str2ptr == ' ') str2ptr++;
		} else num = 1;
		if (num > 99) {
			msg_print(Ind, "Sorry, you can order a stack of up to 99 items at most.");
			return;
		}

		strcpy(str, str2ptr);

		/* failure, not in an npc-store */
		if (p_ptr->store_num < 0) {
			msg_print(Ind, "Sorry, I don't take orders.");
			return;
		}
		/* failure, not a simple town store */
		if (p_ptr->store_num > STORE_MAGIC && p_ptr->store_num != STORE_BOOK && p_ptr->store_num != STORE_RUNE) {
			msg_print(Ind, "Sorry, I don't take orders.");
			return;
		}

		/* hack: allow ordering very specific spell scrolls */
		strncpy(str2, str, 40);
		if (!strncasecmp(str2, "spell crystals", 14)) {
			/* extract spell name, error if not speficied */
			if (strlen(str) > 18) {
				strcpy(str2, str + 18);
				strcpy(str, "Spell Crystals");
			} else {
				msg_print(Ind, "I need to know which spell you want in the spell crystals.");
				return;
				//*str2 = 0;
			}
		} else if (!strncasecmp(str2, "spell scrolls", 13)) {
			/* extract spell name, error if not speficied */
			if (strlen(str) > 17) {
				strcpy(str2, str + 17);
				strcpy(str, "Spell Scrolls");
			} else {
				msg_print(Ind, "I need to know which spell you want in the spell scrolls.");
				return;
				//*str2 = 0;
			}
		} else if (!strncasecmp(str2, "spell crystal", 13)) {
			/* extract spell name, error if not speficied */
			if (strlen(str) > 17) {
				strcpy(str2, str + 17);
				strcpy(str, "Spell Crystal");
			} else {
				msg_print(Ind, "I need to know which spell you want in the spell crystal.");
				return;
				//*str2 = 0;
			}
		} else if (!strncasecmp(str2, "spell scroll", 12)) {
			/* extract spell name, error if not speficied */
			if (strlen(str) > 16) {
				strcpy(str2, str + 16);
				strcpy(str, "Spell Scroll");
			} else {
				msg_print(Ind, "I need to know which spell you want in the spell scroll.");
				return;
				//*str2 = 0;
			}
		} else *str2 = 0;
		if (*str2) {
			for (i = 0; i < max_spells; i++) {
				/* Check for full name */
				if (!strcasecmp(school_spells[i].name, str2)) break;

				/* Additionally check for '... I' tier 1 spell version, assuming the player omitted the 'I' */
				j = strlen(school_spells[i].name) - 2;
				//assume spell names are at least 3 characters long(!)
				if (school_spells[i].name[j] == ' ' //assume the final character must be 'I'
				    && j == strlen(str2)
				    && !strncasecmp(school_spells[i].name, str2, j)) break;
			}
			if (i == max_spells) {
				msg_print(Ind, "Sorry, I have never heard of such a spell.");
				return;
			}
			extra = i;
		}

		/* failure, item does not exist in the game */
		for (i = 1; i < max_k_idx; i++) {
			invcopy(&forge, i);
			forge.number = num; //hack: make item name match player input for plural (num > 1)
			object_desc(0, o_name, &forge, FALSE, 256);
			if (!o_name[0] || o_name[0] == ' ') continue;
			if (!strcmp(o_name, "(nothing)")) continue;
			if (!strcasecmp(o_name, str)) break;
			/* generated scrolls always have pval 0 (MANATHRUST), so special check is needed.
			   Also note that we omit the 'Spell scroll' part because we're not sure if it's singular or plural. */
			if (strstr(o_name, " of Manathrust") && extra != -1) break;
		}
		if (i == max_k_idx) {
#if 1 /* quality of life: for specific stores, no need to type the whole (always same) item name. */
			/* assume player maybe just entered a spell name, if this is the bookstore. */
			if (p_ptr->store_num == STORE_BOOK ||
			    p_ptr->store_num == STORE_BOOK_DUN ||
			    p_ptr->store_num == STORE_LIBRARY ||
			    p_ptr->store_num == STORE_HIDDENLIBRARY ||
			    p_ptr->store_num == STORE_FORBIDDENLIBRARY) {
				for (i = 0; i < max_spells; i++) {
					if (!strcasecmp(school_spells[i].name, str)) break;

					/* Additionally check for '... I' tier 1 spell version, assuming the player omitted the 'I' */
					j = strlen(school_spells[i].name) - 2;
					//assume spell names are at least 3 characters long(!)
					if (school_spells[i].name[j] == ' ' //assume the final character must be 'I'
					    && j == strlen(str)
					    && !strncasecmp(school_spells[i].name, str, j)) break;
				}
				if (i < max_spells) {
					extra = i;
					i = lookup_kind(TV_BOOK, SV_SPELLBOOK);
					invcopy(&forge, i);
				}
				/* still failure */
				if (i == max_k_idx) {
					msg_print(Ind, "Sorry, I don't know of such an item or spell.");
					return;
				}
			}
			/* assume player maybe just entered a rune element, if this is the rune repository. */
			if (p_ptr->store_num == STORE_RUNE ||
			    p_ptr->store_num == STORE_RUNE_DUN) {
				for (i = 0; i < max_k_idx; i++) {
					if (k_info[i].tval != TV_RUNE) continue;
					if (!strcasecmp(k_name + k_info[i].name, str)) break;
				}
				if (i < max_k_idx) invcopy(&forge, i);
				/* still failure */
				if (i == max_k_idx) {
					msg_print(Ind, "Sorry, I don't know of such an item or rune.");
					return;
				}
			}
			/* still failure */
			if (i == max_k_idx) {
				msg_print(Ind, "Sorry, I don't know of such an item.");
				return;
			}
#else
			msg_print(Ind, "Sorry, I don't know of such an item.");
			return;
#endif
		}
		forge.number = 1;

		/* check store's template for this item */
		st_ptr = &town[0].townstore[p_ptr->store_num];
		for (j = 0; j < st_info[st_ptr->st_idx].table_num; j++) {
			/* specific k_idx? */
			if (i == st_info[st_ptr->st_idx].table[j][0]) break;
			/* tval-hack? (all svals are allowed) */
			else if (st_info[st_ptr->st_idx].table[j][0] > 10000 && forge.tval == st_info[st_ptr->st_idx].table[j][0] - 10000) break;
		}
		/* failure, this item is not part of this store's stock template */
		if (j == st_info[st_ptr->st_idx].table_num) {
			msg_print(Ind, "Sorry, I don't sell that kind of item.");
			return;
		}

		/* extract item rarity, hack: any-tval causes greater rarity, and so do spell scrolls (any pval) */
		if (st_info[st_ptr->st_idx].table[j][0] > 10000) /* any tval */
			j = st_info[st_ptr->st_idx].table[j][1] / 2;
		else if (forge.tval == TV_BOOK && forge.sval == SV_SPELLBOOK) /* any pval (book stores only) */
			j = (st_info[st_ptr->st_idx].table[j][1] * 2) / 3;
		else
			j = st_info[st_ptr->st_idx].table[j][1];

		/* create item and calculate price based on item rarity */
		ot_ptr = &ow_info[st_ptr->owner];

		/* use fixed seed depending on town and in-game hour,
		   so people won't spam offers for 'ring of protection'
		   until they get the desired result */
		old_rand = Rand_quick;
		tmp_seed = Rand_value;
		Rand_quick = TRUE;
		Rand_value = (3623 * p_ptr->wpos.wy + 29753) * (2843 * p_ptr->wpos.wx + 48869) + p_ptr->wpos.wz + (379 * (turn / HOUR) + 41);
		apply_magic(&p_ptr->wpos, &forge, 0, FALSE, FALSE, FALSE, FALSE, RESF_NO_ENCHANT);
		Rand_quick = old_rand;
		Rand_value = tmp_seed;

		if (extra != -1) forge.pval = extra; //spellbooks
		object_desc(0, o_name, &forge, FALSE, 256); //for checking if it's a scroll of crystal, namewise

		forge.number = num;
		price = price_item(Ind, &forge, ot_ptr->min_inflate, FALSE);
		/* make sure price doesn't beat BM for *id* / teleport (shop 5), *rc* (shop 4), which are all at 2% rarity */
		price = (price * (180 - j) * (180 - j)) / 6400; //1x (100%) .. 5x (1%) -- so it's still above BM
		price *= num;

		/* Hack -- Charge lite uniformly (occurance 2 of 2, keep in sync) */
		if (forge.tval == TV_LITE) {
			u32b f1, f2, f3, f4, f5, f6, esp;
			object_flags(&forge, &f1, &f2, &f3, &f4, &f5, &f6, &esp);

			/* Only fuelable ones! */
			if (f4 & TR4_FUEL_LITE) {
				if (forge.sval == SV_LITE_TORCH) forge.timeout = FUEL_TORCH / 2;
				if (forge.sval == SV_LITE_LANTERN) forge.timeout = FUEL_LAMP / 2;
			}
		}

		/* If wands, update the # of charges. stack size can be set by force_num or mass_produce (occurance 2 of 2, keep in sync) */
		if ((forge.tval == TV_WAND
#ifdef NEW_MDEV_STACKING
		    || forge.tval == TV_STAFF
#endif
		    ) && forge.number > 1)
			forge.pval *= forge.number;

		p_ptr->item_order_forge = forge;
		p_ptr->item_order_cost = price;
		p_ptr->item_order_rarity = j;

		if (num == 1) {
			/* special hack: spell scrolls get a unique message */
			if (extra != -1) {
				if (strstr(o_name, "Scroll"))
					Send_request_cfr(Ind, RID_ITEM_ORDER, format("I can keep that particular scroll for you, that will be %d Au!", price), 2);
				else
					Send_request_cfr(Ind, RID_ITEM_ORDER, format("I can keep that particular crystal for you, that will be %d Au!", price), 2);
			}
			else if (j >= 90) Send_request_cfr(Ind, RID_ITEM_ORDER, format("That will be %d gold pieces!", price), 2);
			else if (j >= 50) Send_request_cfr(Ind, RID_ITEM_ORDER, format("That item is somewhat less common, that will be %d Au!", price), 2);
			else if (j >= 20) Send_request_cfr(Ind, RID_ITEM_ORDER, format("That item is uncommon, I could promise you delivery for %d Au!", price), 2);
			else if (j >= 5) Send_request_cfr(Ind, RID_ITEM_ORDER, format("That item is rare, I'll try to get it for you for %d Au!", price), 2);
			else Send_request_cfr(Ind, RID_ITEM_ORDER, format("That's very rare, I might be able to get hold of one for %d Au!", price), 2);
		} else {
			/* special hack: spell scrolls get a unique message */
			if (extra != -1) {
				if (strstr(o_name, "Scroll"))
					Send_request_cfr(Ind, RID_ITEM_ORDER, format("I can keep those specific scrolls for you, that will be %d Au!", price), 2);
				else
					Send_request_cfr(Ind, RID_ITEM_ORDER, format("I can keep those specific crystals for you, that will be %d Au!", price), 2);
			}
			else if (j >= 90) Send_request_cfr(Ind, RID_ITEM_ORDER, format("That will be %d gold pieces!", price), 2);
			else if (j >= 50) Send_request_cfr(Ind, RID_ITEM_ORDER, format("Those are somewhat less common, that will be %d Au!", price), 2);
			else if (j >= 20) Send_request_cfr(Ind, RID_ITEM_ORDER, format("Those are uncommon, I could promise you delivery for %d Au!", price), 2);
			else if (j >= 5) Send_request_cfr(Ind, RID_ITEM_ORDER, format("Those are rare, I'll try to get those for you for %d Au!", price), 2);
			else Send_request_cfr(Ind, RID_ITEM_ORDER, format("Those are very rare, I might be able to obtain them for %d Au!", price), 2);
		}
		return;
		}
#endif
#ifdef ENABLE_MERCHANT_MAIL
	case RID_SEND_ITEM: {
		int i;
		object_type *o_ptr = &p_ptr->inventory[p_ptr->mail_item];
		char accname[ACCOUNTNAME_LEN];
		cptr comp;
		cptr acc;
		u32b pid;
		byte w;
		bool total_winner, once_winner;
		char o_name[ONAME_LEN];
		dungeon_type *d_ptr;
		struct worldpos wpos;

		if (!o_ptr->k_idx) {
			msg_print(Ind, "Invalid item.");
			return;
		}

		str[0] = toupper(str[0]);
		pid = lookup_player_id(str);
		if (!pid) {
			msg_format(Ind, "\377ySorry, there is no character named %s.", str);
			return;
		}
		acc = lookup_accountname(pid);
		if (acc) strcpy(accname, acc);
		else { //paranoia
			msg_format(Ind, "\377ySorry, that character does not have an account.");
			return;
		}
		w = lookup_player_winner(pid);
		total_winner = (w & 0x1);
		once_winner = (w & 0x2);

		if (p_ptr->au < p_ptr->mail_fee && !p_ptr->mail_COD) {
			msg_format(Ind, "\377yYou do not carry enough money to pay the fee of %d Au!", p_ptr->mail_fee);
			return;
		}

		comp = compat_mode(p_ptr->mode, lookup_player_mode(pid));
		if (comp) {
			msg_format(Ind, "\377yYou cannot send anything to %s characters.", comp);
			return;
		}

		/* These anti-ironman checks take care of possible IDDC_IRON_COOP too */
		d_ptr = getdungeon(&p_ptr->wpos);
		if (d_ptr && ((d_ptr->flags2 & (DF2_IRON | DF2_NO_RECALL_INTO)) || (d_ptr->flags1 & DF1_NO_RECALL))) {
			msg_print(Ind, "Sorry, we are unable to ship from this location.");
			return;
		}
		wpos = lookup_player_wpos(pid);
		d_ptr = getdungeon(&wpos);
		if (d_ptr && ((d_ptr->flags2 & (DF2_IRON | DF2_NO_RECALL_INTO)) || (d_ptr->flags1 & DF1_NO_RECALL))) {
			msg_print(Ind, "Sorry, we are unable to ship to that player's location.");
			return;
		}

#ifdef IRON_IRON_TEAM
		if ((i = lookup_player_party(pid)) && (parties[i].mode & PA_IRONTEAM) && o_ptr->owner != pid
		    //&& o_ptr->iron_trade != lookup_player_iron_trade(pid) {  --currently no lookup_player_iron_trade() function :-p just use this for now:
		    && p_ptr->party != i) {
			msg_print(Ind, "\377yUnfortunately we cannot ship to that person.");
			return;
		}
#endif

		/* true artifact restrictions */
		if (true_artifact_p(o_ptr)) {
			if (cfg.anti_arts_hoard || p_ptr->total_winner) {
				msg_print(Ind, "\377yAs a royalty, you cannot hand over true artifacts.");
				return;
			}
			if (!winner_artifact_p(o_ptr) && cfg.kings_etiquette && total_winner) {
				msg_print(Ind, "\377yRoyalties may not own true artifacts.");
				return;
			}
			if (cfg.anti_arts_pickup && o_ptr->level < lookup_player_level(pid)) {
				msg_print(Ind, "\377yThe receipient must match the level of a true artifact.");
				return;
			}
		}
		/* WINNERS_ONLY item restrictions */
		if ((k_info[o_ptr->k_idx].flags5 & TR5_WINNERS_ONLY) &&
#ifdef FALLEN_WINNERSONLY
		    !once_winner
#else
		    !total_winner
#endif
		    ) {
			msg_print(Ind, "That item may only be sent to royalties!");
			return;
		}

		/* le paranoid double-check */
		for (i = 0; i < MAX_MERCHANT_MAILS; i++)
			if (!mail_sender[i][0]) break;
		if (i == MAX_MERCHANT_MAILS) {
			msg_print(Ind, "\377yWe're very sorry, our service is currently overloaded! Please try again later.");
			return;
		}

		msg_format(Ind, "Thanks, %s! We have sent a package on its way.", p_ptr->male ? "sir" : "ma'am");
		if (!p_ptr->mail_COD) {
			p_ptr->au -= p_ptr->mail_fee;
			p_ptr->redraw |= PR_GOLD;
 #ifdef USE_SOUND_2010
			sound(Ind, "pickup_gold", NULL, SFX_TYPE_COMMAND, FALSE);
 #endif
		}

 #ifdef USE_SOUND_2010
		sound_item(Ind, o_ptr->tval, o_ptr->sval, "pickup_");
 #endif

		mail_forge[i] = *o_ptr; // we accept a full item stack actually
		strcpy(mail_sender[i], p_ptr->name);
		strcpy(mail_target[i], str);
		strcpy(mail_target_acc[i], acc);
		mail_duration[i] = MERCHANT_MAIL_DURATION;
		mail_COD[i] = p_ptr->mail_COD;
		mail_xfee[i] = p_ptr->mail_xfee;

		inven_item_increase(Ind, p_ptr->mail_item, -o_ptr->number);
		inven_item_describe(Ind, p_ptr->mail_item);
		inven_item_optimize(Ind, p_ptr->mail_item);

		object_desc(0, o_name, &mail_forge[i], TRUE, 3);
		s_printf("MERCHANT_MAIL:(SENT) <%s> to <%s> sent: %s.\n", p_ptr->name, mail_target[i], o_name);
		return; }
	case RID_SEND_GOLD: {
		int i;
		char accname[ACCOUNTNAME_LEN];
		cptr comp;
		cptr acc;
		u32b pid, total = p_ptr->mail_gold;
		dungeon_type *d_ptr;
		struct worldpos wpos;

		if (total > p_ptr->au) {
			//someone steal from us while we're in the store dialogue? =P
			msg_print(Ind, "\377yYou do not have that much money!");
			return;
		}
		if (!p_ptr->mail_COD) total += p_ptr->mail_fee;
		if (p_ptr->au < total) {
			msg_format(Ind, "\377yYou do not carry enough money to pay the total amount of %d Au, including the fee of %d Au!", total, p_ptr->mail_fee);
			return;
		}

		str[0] = toupper(str[0]);
		pid = lookup_player_id(str);
		if (!pid) {
			msg_format(Ind, "\377ySorry, there is no character named %s.", str);
			return;
		}
		acc = lookup_accountname(pid);
		if (acc) strcpy(accname, acc);
		else { //paranoia
			msg_format(Ind, "\377ySorry, that character does not have an account.");
			return;
		}

		comp = compat_mode(p_ptr->mode, lookup_player_mode(pid));
		if (comp) {
			msg_format(Ind, "\377yYou cannot send anything to %s characters.", comp);
			return;
		}

		/* These anti-ironman checks take care of possible IDDC_IRON_COOP too */
		d_ptr = getdungeon(&p_ptr->wpos);
		if (d_ptr && ((d_ptr->flags2 & (DF2_IRON | DF2_NO_RECALL_INTO)) || (d_ptr->flags1 & DF1_NO_RECALL))) {
			msg_print(Ind, "Sorry, we are unable to transfer from this location.");
			return;
		}
		wpos = lookup_player_wpos(pid);
		d_ptr = getdungeon(&wpos);
		if (d_ptr && ((d_ptr->flags2 & (DF2_IRON | DF2_NO_RECALL_INTO)) || (d_ptr->flags1 & DF1_NO_RECALL))) {
			msg_print(Ind, "Sorry, we are unable to transfer to that player's location.");
			return;
		}

#ifdef IRON_IRON_TEAM
		if ((i = lookup_player_party(pid)) && (parties[i].mode & PA_IRONTEAM) && p_ptr->party != i) {
			msg_print(Ind, "\377yUnfortunately we cannot transfer to that person.");
			return;
		}
#endif

		/* le paranoid double-check */
		for (i = 0; i < MAX_MERCHANT_MAILS; i++)
			if (!mail_sender[i][0]) break;
		if (i == MAX_MERCHANT_MAILS) {
			msg_print(Ind, "\377yWe're very sorry, our service is currently overloaded! Please try again later.");
			return;
		}

		msg_format(Ind, "Thanks, %s! We have sent %d Au on its way.", p_ptr->male ? "sir" : "ma'am", p_ptr->mail_gold);
		p_ptr->au -= total;
		p_ptr->redraw |= PR_GOLD;

 #ifdef USE_SOUND_2010
		sound(Ind, "pickup_gold", NULL, SFX_TYPE_COMMAND, FALSE);
 #endif

		invcopy(&mail_forge[i], lookup_kind(TV_GOLD, 1));
		mail_forge[i].pval = p_ptr->mail_gold;
		strcpy(mail_sender[i], p_ptr->name);
		strcpy(mail_target[i], str);
		strcpy(mail_target_acc[i], acc);
		mail_duration[i] = MERCHANT_MAIL_DURATION;
		mail_COD[i] = p_ptr->mail_COD;
		mail_xfee[i] = 0;

		s_printf("MERCHANT_MAIL:(SENT) <%s> to <%s> sent: %d Au.\n", p_ptr->name, mail_target[i], mail_forge[i].pval);
		return; }
#endif
	default:;
	}
}

/* Handle number input request replies */
void handle_request_return_num(int Ind, int id, int num) {
	player_type *p_ptr = Players[Ind];

	/* verify that the ID is actually valid (maybe clear p_ptr->request_id here too) */
	if (id != p_ptr->request_id) return;

	/* request done */
	p_ptr->request_id = RID_NONE;

	/* verify that a number had been requested */
	if (RTYPE_NUM != p_ptr->request_type) return;

	switch (id) {
#ifdef SOLO_REKING
	case RID_SR_DONATE: /* added for SOLO_REKING */
		if (num > p_ptr->au) {
			msg_print(Ind, "You do not have that much gold with you!");
			return;
		}
		if (num <= 0) {
			msg_print(Ind, "You decide not to donate at this time.");
			msg_print(Ind, "You pray to the gods.");
			return;
		}

		p_ptr->au -= num;
		p_ptr->redraw |= PR_GOLD;
		if (num == 1)
			msg_format(Ind, "Your donation of %d gold piece is well received.", num);
		else
			msg_format(Ind, "Your donation of %d gold pieces is well received.", num);
		msg_print(Ind, "You pray to the gods.");

		/* no fallen winner yet or already donated enough and changed our fate? */
		if (!p_ptr->once_winner || p_ptr->total_winner) return;

		/* count donation, enough to change fate? */
		p_ptr->solo_reking_au -= num;
		if (p_ptr->solo_reking_au < 0) {
			p_ptr->solo_reking += p_ptr->solo_reking_au;
			p_ptr->solo_reking_au = 0;
			if (p_ptr->solo_reking < 0) p_ptr->solo_reking = 0;
		}
		if (p_ptr->solo_reking + p_ptr->solo_reking_au == 0) {
			msg_print(Ind, "\377GYou feel your fate has been changed!");
			// .. revive Sauron too? =P .. */
			p_ptr->once_winner = 0;
			p_ptr->r_killed[RI_MORGOTH] = 0;
		}
		return;
#endif
#ifdef ENABLE_MERCHANT_MAIL
	case RID_SEND_ITEM_PAY:
		if (num < 1) {
			msg_print(Ind, "\377yThe requested payment must be at least 1 Au.");
			return;
		}
		if (num > 2000000000) {
			msg_print(Ind, "\377yThat is too much, the maximum allowed amount is 2 000 000 000 Au.");
			return;
		}
		p_ptr->mail_xfee = num;
		Send_request_str(Ind, RID_SEND_ITEM, "Who is the addressee, please? ", "");
		return;
#endif
	default:;
	}
}

/* Handle key input request replies */
void handle_request_return_key(int Ind, int id, char c) {
	player_type *p_ptr = Players[Ind];

	/* verify that the ID is actually valid (maybe clear p_ptr->request_id here too) */
	if (id != p_ptr->request_id) return;

	/* request done */
	p_ptr->request_id = RID_NONE;

	/* verify that a key had been requested */
	if (RTYPE_KEY != p_ptr->request_type) return;

	switch (id) {
#ifdef ENABLE_GO_GAME
	case RID_GO:
		if (p_ptr->store_num == -1) return; /* Discard if we left the building */
		go_challenge_accept(Ind, FALSE);
		return;
	case RID_GO_START:
		if (p_ptr->store_num == -1) return; /* Discard if we left the building */
		go_challenge_start(Ind);
		return;
#endif
	default:;
	}
}

/* Handle confirmation request replies */
void handle_request_return_cfr(int Ind, int id, bool cfr) {
	player_type *p_ptr = Players[Ind];

	/* verify that the ID is actually valid (maybe clear p_ptr->request_id here too) */
	if (id != p_ptr->request_id) return;

	/* request done */
	p_ptr->request_id = RID_NONE;

	/* verify that a y/n confirmation had been requested */
	if (RTYPE_CFR != p_ptr->request_type) return;

	/* quests occupy an id broadband */
	if (id >= RID_QUEST_ACQUIRE) {
		if (cfr) quest_acquire_confirmed(Ind, id - RID_QUEST_ACQUIRE, FALSE);
		return;
	}
	else if (id >= RID_QUEST) {
		char str[2];

		if (cfr) str[0] = 'y';
		else str[0] = 'n';
		str[1] = 0;

		//DEBUG
		s_printf("RID_QUEST_CFR: (%d) '%s' replied '%s'\n", id, p_ptr->name, str);

		quest_reply(Ind, id - RID_QUEST, str);
		return;
	}

	switch (id) {
#ifdef ENABLE_GO_GAME
	case RID_GO:
		if (p_ptr->store_num == -1) return; /* Discard if we left the building */
		if (!cfr) {
			Send_store_special_clr(Ind, 5, 18);
			Send_store_special_str(Ind, 8, 8, TERM_ORANGE, "Now you're chickening out huh!");
			p_ptr->store_action = 0;
			return;
		}
		go_challenge_accept(Ind, TRUE);
		return;
#endif
	case RID_GUILD_RENAME:
		if (cfr) Send_request_str(Ind, RID_GUILD_RENAME, "Enter a new guild name: ", "");
		return;
	case RID_GUILD_CREATE:
		if (cfr) guild_create(Ind, p_ptr->cur_file_title);
		p_ptr->cur_file_title[0] = 0;//not really needed
		return;
#ifdef ENABLE_ITEM_ORDER
	case RID_ITEM_ORDER: {
		s32b dur;
		char o_name[ONAME_LEN];

		if (p_ptr->store_num == -1) return;
		if (!cfr) return;
		if (p_ptr->item_order_cost > p_ptr->au) {
			msg_print(Ind, "You do not have that much gold with you!");
			return;
		}
		p_ptr->au -= p_ptr->item_order_cost;
		p_ptr->redraw |= PR_GOLD;
		p_ptr->item_order_store = p_ptr->store_num + 1;
		p_ptr->item_order_town = gettown(Ind);

		object_desc(0, o_name, &p_ptr->item_order_forge, FALSE, 256);
		s_printf("RID_ITEM_ORDER: <%s> st %d, to %d: %d %s (%" PRId64 " Au)\n", p_ptr->name, p_ptr->item_order_store, p_ptr->item_order_town, p_ptr->item_order_forge.number, o_name, p_ptr->item_order_cost);

		/* calculate time, real-time minutes depending on store template rarity */
		//dur = (cfg.fps * 60 * 5 * (113 - p_ptr->item_order_rarity)) / 13; //5..43 min
		//dur = (cfg.fps * 60 * 5 * (115 - p_ptr->item_order_rarity)) / 15; //5..38 min
		dur = (cfg.fps * 60 * 5 * (120 - p_ptr->item_order_rarity)) / 20; //5..30 min
		/* piles increase duration */
		//dur = (dur * (97 + p_ptr->item_order_forge.number)) / 98; //x1..x2 linear
		dur = (dur * (400 - (15000 / (49 + p_ptr->item_order_forge.number)))) / 100; //x1..x3 tang
 #ifdef TEST_SERVER
		p_ptr->item_order_turn = turn; //instant delivery for testing purpose
 #else
		p_ptr->item_order_turn = turn + dur;
 #endif
		/* give in-game-time message */
		if (dur <= HOUR / 2) msg_format(Ind, "It should arrive shortly.");
		else if (dur <= (HOUR * 3) / 2) msg_format(Ind, "Check back with me in an hour, give or take.");
		else if (dur <= HOUR * 6) msg_format(Ind, "Deal, this will take a few hours though.");
		else if (dur <= HOUR * 18) msg_format(Ind, "Deal, maybe it'll take half a day, give or take a couple hours.");
		else if (dur <= DAY) msg_format(Ind, "Deal. I don't expect it to take longer than a day to arrive.");
		else msg_format(Ind, "Deal. But this might take a long time to arrive, don't expect it today.");
		return;
		}
#endif
#ifdef ENABLE_MERCHANT_MAIL
	case RID_SEND_ITEM:
		if (cfr) Send_request_cfr(Ind, RID_SEND_ITEM2, "Will you be paying the fee? Otherwise we'll charge the addressee.", 2);
		return;
	case RID_SEND_GOLD:
		if (cfr) Send_request_cfr(Ind, RID_SEND_GOLD2, "Will you be paying the fee? Otherwise we'll charge the addressee.", 1);
		return;
	case RID_SEND_ITEM2:
		p_ptr->mail_COD = !cfr;
		Send_request_str(Ind, RID_SEND_ITEM, "Who is the addressee, please? ", "");
		return;
	case RID_SEND_GOLD2:
		p_ptr->mail_COD = !cfr;
		Send_request_str(Ind, RID_SEND_GOLD, "Who is the addressee, please? ", "");
		return;
	case RID_SEND_FEE:
	case RID_SEND_FEE_PAY: {
		int i = p_ptr->mail_item;

		/* silyl case: the package expired while we were looking at the "do you accept the fee?" prompt */
		if (mail_duration[i]) {
			msg_print(Ind, "\377ySorry, the package has already been returned.");
			return;
		}

		if (cfr) {
			char sender[NAME_LEN];

			if (p_ptr->au < p_ptr->mail_fee) {
				msg_format(Ind, "\377yYou do not carry enough money to pay the fee of %d Au!", p_ptr->mail_fee);
				return;
			}

			/* we accepted the COD fee */
			strcpy(sender, mail_sender[i]); //save it before it gets 0'ed in the next line
			if (merchant_mail_carry(Ind, i)) {
				p_ptr->au -= p_ptr->mail_fee;
				p_ptr->redraw |= PR_GOLD;
 #ifdef USE_SOUND_2010
				sound(Ind, "pickup_gold", NULL, SFX_TYPE_COMMAND, FALSE);
 #endif

				/* mail the payment to the sender -- reuse the current mail slot 'i' for that */
				if (id == RID_SEND_FEE_PAY) {
					u32b pid;
					cptr acc;

					invcopy(&mail_forge[i], lookup_kind(TV_GOLD, 1));
					mail_forge[i].pval = mail_xfee[i];

					strcpy(mail_sender[i], p_ptr->name);
					strcpy(mail_target[i], sender);

					pid = lookup_player_id(sender);
					if (pid) {
						acc = lookup_accountname(pid);
						if (acc) strcpy(mail_target_acc[i], acc);
						else s_printf("MAIL_ERROR: payment acc '%s'\n", sender);
					} else s_printf("MAIL_ERROR: payment id '%s'\n", sender);

					mail_duration[i] = MERCHANT_MAIL_DURATION;
					mail_COD[i] = FALSE;
					mail_xfee[i] = 0;
				}

				/* check if there's more waiting for us */
				merchant_mail_delivery(Ind);
			}
		} else {
			/* return to sender */
			msg_format(Ind, "Alright, %s. We're sending it back.", p_ptr->male ? "sir" : "ma'am");
			mail_timeout[i] = -2;
		}
		return; }
	case RID_SEND_ITEM_PAY:
		if (cfr) Send_request_cfr(Ind, RID_SEND_ITEM_PAY2, "Will you be paying the fee? Otherwise we'll charge the addressee.", 2);
		return;
	case RID_SEND_ITEM_PAY2:
		p_ptr->mail_COD = !cfr;
		Send_request_num(Ind, RID_SEND_ITEM_PAY, "How much gold does the receipient have to pay you? ", -1);
		return;
#endif
	default: ;
	}
}

/* Cap a player's energy:
   Make sure they don't have too much, but let them store up some extra.
   Storing up extra energy lets us perform actions while we are running */
void limit_energy(player_type *p_ptr) {
	//if (p_ptr->energy > (level_speed(p_ptr->dun_depth) * 6) / 5)
	//	p_ptr->energy = (level_speed(p_ptr->dun_depth) * 6) / 5;
	if (p_ptr->energy > (level_speed(&p_ptr->wpos) * 2) - 1)
		p_ptr->energy = (level_speed(&p_ptr->wpos) * 2) - 1;
}
