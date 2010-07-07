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

/* Announce global events every 900 seconds */
#define GE_ANNOUNCE_INTERVAL 900

/* Allow ego monsters in Arena Challenge global event? */
#define GE_ARENA_ALLOW_EGO


/*
 * Converts stat num into a six-char (right justified) string
 */
void cnv_stat(int val, char *out_val)
{
	/* Above 18 */
	if (val > 18)
	{
		int bonus = (val - 18);

		if (bonus >= 220)
		{
			sprintf(out_val, "18/%3s", "***");
		}
		else if (bonus >= 100)
		{
			sprintf(out_val, "18/%03d", bonus);
		}
		else
		{
			sprintf(out_val, " 18/%02d", bonus);
		}
	}

	/* From 3 to 18 */
	else
	{
		sprintf(out_val, "    %2d", val);
	}
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
s16b modify_stat_value(int value, int amount)
{
	int i;

	/* Reward */
	if (amount > 0)
	{
		/* Apply each point */
		for (i = 0; i < amount; i++)
		{
			/* One point at a time */
			if (value < 18) value++;

			/* Ten "points" at a time */
			else value += 10;
		}
	}

	/* Penalty */
	else if (amount < 0)
	{
		/* Apply each point */
		for (i = 0; i < (0 - amount); i++)
		{
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
static void prt_stat(int Ind, int stat)
{	
	player_type *p_ptr = Players[Ind];
	Send_stat(Ind, stat, p_ptr->stat_top[stat], p_ptr->stat_use[stat], p_ptr->stat_ind[stat], p_ptr->stat_max[stat]);
}




/*
 * Prints "title", including "wizard" or "winner" as needed.
 */
static void prt_title(int Ind)
{
	player_type *p_ptr = Players[Ind];

	cptr p = "";

	/* Winner */
	if (p_ptr->total_winner || (p_ptr->lev > PY_MAX_LEVEL))
	{
		if (p_ptr->mode & (MODE_HARD | MODE_NO_GHOST))
			p = (p_ptr->male ? "**EMPEROR**" : "**EMPRESS**");
		else
			p = (p_ptr->male ? "**KING**" : "**QUEEN**");
	}

	/* Normal */
	else
	{
		if (p_ptr->lev < 60)
		p = player_title[p_ptr->pclass][((p_ptr->lev/5) < 10)? (p_ptr->lev/5) : 10][3 - p_ptr->male];
		else
		p = player_title_special[p_ptr->pclass][(p_ptr->lev < PY_MAX_PLAYER_LEVEL)? (p_ptr->lev - 60)/10 : 4][3 - p_ptr->male];
	}

	/* Ghost */
	if (p_ptr->ghost)
		p = "\377rGhost (dead)";

	Send_title(Ind, p);
}


/*
 * Prints level
 */
static void prt_level(int Ind)
{
	player_type *p_ptr = Players[Ind];

	s64b adv_exp;

	if (p_ptr->lev >= (is_admin(p_ptr) ? PY_MAX_LEVEL : PY_MAX_PLAYER_LEVEL))
		adv_exp = 0;
#ifndef ALT_EXPRATIO
	else adv_exp = (s64b)((s64b)player_exp[p_ptr->lev - 1] * (s64b)p_ptr->expfact / 100L);
#else
	else adv_exp = (s64b)player_exp[p_ptr->lev - 1];
#endif

	Send_experience(Ind, p_ptr->lev, p_ptr->max_exp, p_ptr->exp, adv_exp);
}


/*
 * Display the experience
 */
static void prt_exp(int Ind)
{
	player_type *p_ptr = Players[Ind];

	s64b adv_exp;

	if (p_ptr->lev >= (is_admin(p_ptr) ? PY_MAX_LEVEL : PY_MAX_PLAYER_LEVEL))
		adv_exp = 0;
#ifndef ALT_EXPRATIO
	else adv_exp = (s64b)((s64b)player_exp[p_ptr->lev - 1] * (s64b)p_ptr->expfact / 100L);
#else
	else adv_exp = (s64b)player_exp[p_ptr->lev - 1];
#endif

	Send_experience(Ind, p_ptr->lev, p_ptr->max_exp, p_ptr->exp, adv_exp);
}


/*
 * Prints current gold
 */
static void prt_gold(int Ind)
{
	player_type *p_ptr = Players[Ind];

	Send_gold(Ind, p_ptr->au, p_ptr->balance);
}



/*
 * Prints current AC
 */
static void prt_ac(int Ind)
{
	player_type *p_ptr = Players[Ind];

	Send_ac(Ind, p_ptr->dis_ac, p_ptr->dis_to_a);
}

static void prt_sanity(int Ind)
{
#ifdef SHOW_SANITY	// No.
	player_type *p_ptr=Players[Ind];
#if 0
	Send_sanity(Ind, p_ptr->msane, p_ptr->csane);
#else	// 0
	char buf[20];
	byte attr = TERM_L_GREEN;
	int skill = get_skill(p_ptr, SKILL_HEALTH);
	int ratio;
	ratio = p_ptr->msane ? (p_ptr->csane * 100) / p_ptr->msane : 100;

	/* Vague */
	if (ratio < 0)
	{
		/* This guy should be dead - for tombstone */
		attr = TERM_RED;
		strcpy(buf, "Vegetable");
	}
	else if (ratio < 10)
	{
//		attr = TERM_RED;
		attr = TERM_MULTI;
		strcpy(buf, "      MAD");
	}
	else if (ratio < 25)
	{
		attr = TERM_SHIELDI;
		strcpy(buf, "   Insane");
	}
	else if (ratio < 50)
	{
		attr = TERM_ORANGE;
		strcpy(buf, "   Crazy");
	}
	else if (ratio < 75)
	{
		attr = TERM_YELLOW;
		strcpy(buf, "    Weird");
	}
	else if (ratio < 90)
	{
		attr = TERM_GREEN;
		strcpy(buf, "     Sane");
	}
	else
	{
		attr = TERM_L_GREEN;
		strcpy(buf, "    Sound");
	}

	/* Full */
	if (skill >= 40)
	{
		snprintf(buf, sizeof(buf), "%4d/%4d", p_ptr->csane, p_ptr->msane);
	}
	/* Percentile */
	else if (skill >= 20)
	{
		snprintf(buf, sizeof(buf), "     %3d%%", ratio);
	}
	/* Sanity Bar */
	else if (skill >= 10)
	{
		int tmp = ratio / 11;
		strcpy(buf, "---------");
		if (tmp > 0) strncpy(buf, "*********", tmp);
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
static void prt_hp(int Ind)
{
	player_type *p_ptr = Players[Ind];

	Send_hp(Ind, p_ptr->mhp, p_ptr->chp);
}

/*
 * Prints Cur/Max stamina points
 */
static void prt_stamina(int Ind)
{
	player_type *p_ptr = Players[Ind];

	Send_stamina(Ind, p_ptr->mst, p_ptr->cst);
}

/*
 * Prints players max/cur spell points
 */
static void prt_sp(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Do not show mana unless it matters */
	Send_sp(Ind, p_ptr->msp, p_ptr->csp);
}


/*
 * Prints depth in stat area
 */
static void prt_depth(int Ind)
{
	player_type *p_ptr = Players[Ind];

	Send_depth(Ind, &p_ptr->wpos);
}


/*
 * Prints status of hunger
 */
static void prt_hunger(int Ind)
{
	player_type *p_ptr = Players[Ind];

	Send_food(Ind, p_ptr->food);
}


/*
 * Prints Blind status
 */
static void prt_blind(int Ind)
{
	player_type *p_ptr = Players[Ind];

	if (p_ptr->blind)
	{
		Send_blind(Ind, TRUE);
	}
	else
	{
		Send_blind(Ind, FALSE);
	}
}


/*
 * Prints Confusion status
 */
static void prt_confused(int Ind)
{
	player_type *p_ptr = Players[Ind];

	if (p_ptr->confused)
	{
		Send_confused(Ind, TRUE);
	}
	else
	{
		Send_confused(Ind, FALSE);
	}
}


/*
 * Prints Fear status
 */
static void prt_afraid(int Ind)
{
	player_type *p_ptr = Players[Ind];

	if (p_ptr->afraid)
	{
		Send_fear(Ind, TRUE);
	}
	else
	{
		Send_fear(Ind, FALSE);
	}
}


/*
 * Prints Poisoned status
 */
static void prt_poisoned(int Ind)
{
	player_type *p_ptr = Players[Ind];

	if (p_ptr->poisoned)
	{
		Send_poison(Ind, TRUE);
	}
	else
	{
		Send_poison(Ind, FALSE);
	}
}


/*
 * Prints Searching, Resting, Paralysis, or 'count' status
 * Display is always exactly 10 characters wide (see below)
 *
 * This function was a major bottleneck when resting, so a lot of
 * the text formatting code was optimized in place below.
 */
static void prt_state(int Ind)
{
	player_type *p_ptr = Players[Ind];

	bool p, s, r;

	/* Paralysis */
	if (p_ptr->paralyzed)
	{
		p = TRUE;
	}
	else
	{
		p = FALSE;
	}

	/* Searching */
	if (p_ptr->searching)
	{
		s = TRUE;
	}
	else
	{
		s = FALSE;
	}

	/* Resting */
	if (p_ptr->resting)
	{
		r = TRUE;
	}
	else
	{
		r = FALSE;
	}

	Send_state(Ind, p, s, r);
}


/*
 * Prints the speed of a character.			-CJS-
 */
static void prt_speed(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int i = p_ptr->pspeed;

#if 0	/* methinks we'd better tell it to players.. - Jir - */
	/* Hack -- Visually "undo" the Search Mode Slowdown */
	/* And this formula can be wrong for hellish */
//	if (p_ptr->searching) i+=(p_ptr->mode&MODE_HARD ? 5 : 10);
	if (p_ptr->searching) i += 10;
#endif	// 0

	Send_speed(Ind, i - 110);
}


static void prt_study(int Ind)
{
	player_type *p_ptr = Players[Ind];

	if (p_ptr->skill_points)
	{
		Send_study(Ind, TRUE);
	}
	else
	{
		Send_study(Ind, FALSE);
	}
}


static void prt_cut(int Ind)
{
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



static void prt_stun(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int s = p_ptr->stun;

	Send_stun(Ind, s);
}

static void prt_history(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int i;

	for (i = 0; i < 4; i++)
	{
		Send_history(Ind, i, p_ptr->history[i]);
	}
}

static void prt_various(int Ind)
{
	player_type *p_ptr = Players[Ind];

	Send_various(Ind, p_ptr->ht, p_ptr->wt, p_ptr->age, p_ptr->sc, r_name + r_info[p_ptr->body_monster].name);
}

static void prt_plusses(int Ind)
{
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
}

static void prt_skills(int Ind)
{
	Send_skills(Ind);
}

static void prt_AFK(int Ind)
{
	player_type *p_ptr = Players[Ind];

	byte afk = (p_ptr->afk ? 1 : 0);

	Send_AFK(Ind, afk);
}

static void prt_encumberment(int Ind)
{
	player_type *p_ptr = Players[Ind];

	byte cumber_armor = p_ptr->cumber_armor?1:0;
	byte awkward_armor = p_ptr->awkward_armor?1:0;
	/* Hack - For mindcrafters, it's the helmet, not the gloves,
	   but they fortunately use the same item symbol :) */
	byte cumber_glove = p_ptr->cumber_glove || p_ptr->cumber_helm?1:0;
	byte heavy_wield = p_ptr->heavy_wield?1:0;
	byte heavy_shield = p_ptr->heavy_shield?1:0; /* added in 4.4.0f */
	byte heavy_shoot = p_ptr->heavy_shoot?1:0;
	byte icky_wield = p_ptr->icky_wield?1:0;
	byte awkward_wield = p_ptr->awkward_wield?1:0;
	byte easy_wield = p_ptr->easy_wield?1:0;
	byte cumber_weight = p_ptr->cumber_weight?1:0;
	/* See next line. Also, we're already using all 12 spaces we have available for icons.
	byte rogue_heavy_armor = p_ptr->rogue_heavyarmor?1:0; */
	 /* Hack - MA also gives dodging, which relies on rogue_heavy_armor anyway. */
	byte monk_heavyarmor, rogue_heavyarmor = 0;
	byte awkward_shoot = p_ptr->awkward_shoot?1:0;
	if (!is_newer_than(&p_ptr->version, 4, 4, 2, 0, 0, 0)) {
		monk_heavyarmor = (p_ptr->monk_heavyarmor || p_ptr->rogue_heavyarmor)?1:0;
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
                icky_wield, awkward_wield, easy_wield, cumber_weight, monk_heavyarmor, rogue_heavyarmor, awkward_shoot);
}

static void prt_extra_status(int Ind)
{
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
 
 
static void health_redraw(int Ind)
{
	player_type *p_ptr = Players[Ind];

#ifdef DRS_SHOW_HEALTH_BAR

	/* Not tracking */
	if (p_ptr->health_who == 0)
	{
		/* Erase the health bar */
		Send_monster_health(Ind, 0, 0);
	}

	/* Tracking a hallucinatory monster */
	else if (p_ptr->image)
	{
		/* Indicate that the monster health is "unknown" */
		Send_monster_health(Ind, 0, TERM_WHITE);
	}

	/* Tracking a player */
	else if (p_ptr->health_who < 0)
	{
		player_type *q_ptr = Players[0 - p_ptr->health_who];

		if(Players[Ind]->conn == NOT_CONNECTED ) {
			/* Send_monster_health(Ind, 0, 0); */
			return;
		};

		if(0-p_ptr->health_who < NumPlayers) {
			if(Players[0-p_ptr->health_who]->conn == NOT_CONNECTED ) {
				Send_monster_health(Ind, 0, 0); 
				return;
			};
		} else {
			Send_monster_health(Ind, 0, 0); 
			return;
		}
			

		/* Tracking a bad player (?) */
		if (!q_ptr)
		{
			/* Erase the health bar */
			Send_monster_health(Ind, 0, 0);
		}

		/* Tracking an unseen player */
		else if (!p_ptr->play_vis[0 - p_ptr->health_who])
		{
			/* Indicate that the player health is "unknown" */
			Send_monster_health(Ind, 0, TERM_WHITE);
		}

		/* Tracking a visible player */
		else
		{
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

			/* Convert percent into "health" */
			len = (pct < 10) ? 1 : (pct < 90) ? (pct / 10 + 1) : 10;

			/* Send the health */
			Send_monster_health(Ind, len, attr);
		}
	}

	/* Tracking a bad monster (?) */
	else if (!m_list[p_ptr->health_who].r_idx)
	{
		/* Erase the health bar */
		Send_monster_health(Ind, 0, 0);
	}

	/* Tracking an unseen monster */
	else if (!p_ptr->mon_vis[p_ptr->health_who])
	{
		/* Indicate that the monster health is "unknown" */
		Send_monster_health(Ind, 0, TERM_WHITE);
	}

	/* Tracking a dead monster (???) */
	else if (!m_list[p_ptr->health_who].hp < 0)
	{
		/* Indicate that the monster health is "unknown" */
		Send_monster_health(Ind, 0, TERM_WHITE);
	}

	/* Tracking a visible monster */
	else
	{
		int pct, len;

		monster_type *m_ptr = &m_list[p_ptr->health_who];

		/* Default to almost dead */
		byte attr = TERM_RED;

		/* Crash once occurred here, m_ptr->hp -296, m_ptr->maxhp 0 - C. Blue */
		if (m_ptr->maxhp == 0) {
			Send_monster_health(Ind, 0, 0);
			return;
		} else

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
static void prt_frame_basic(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int i;

	/* Race and Class */
	Send_char_info(Ind, p_ptr->prace, p_ptr->pclass, p_ptr->male, p_ptr->mode);

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
static void prt_frame_extra(int Ind)
{
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

	/* Study spells */
	prt_study(Ind);
}


/*
 * Hack -- display inventory in sub-windows
 */
static void fix_inven(int Ind)
{
	/* Resend the inventory */
	display_inven(Ind);
}



/*
 * Hack -- display equipment in sub-windows
 */
static void fix_equip(int Ind)
{
	/* Resend the equipment */
	display_equip(Ind);
}

/*
 * Hack -- display character in sub-windows
 */
static void fix_player(int Ind)
{
}



/*
 * Hack -- display recent messages in sub-windows
 *
 * XXX XXX XXX Adjust for width and split messages
 */
static void fix_message(int Ind)
{
}


/*
 * Hack -- display overhead view in sub-windows
 *
 * Note that the "player" symbol does NOT appear on the map.
 */
static void fix_overhead(int Ind)
{
}


/*
 * Hack -- display monster recall in sub-windows
 */
static void fix_monster(int Ind)
{
}

/*
 * Calculate the player's sanity
 */

static void calc_sanity(int Ind)
{
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
				(void)strcpy(p_ptr->died_from, "Insanity");
				(void)strcpy(p_ptr->really_died_from, "Insanity");
		                if (!p_ptr->ghost) {
			                strcpy(p_ptr->died_from_list, "Insanity");
			                p_ptr->died_from_depth = getlevel(&p_ptr->wpos);
					/* Hack to remember total winning */
		                        if (p_ptr->total_winner) strcat(p_ptr->died_from_list, "\001");
				}
	            		/* No longer a winner */
//			        p_ptr->total_winner = FALSE;
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
void calc_mana(int Ind)
{
	player_type *p_ptr = Players[Ind];
	player_type *p_ptr2 = NULL; /* silence the warning */
	int Ind2;

	int levels, cur_wgt, max_wgt;
	s32b new_mana=0;

	object_type	*o_ptr;
	u32b f1, f2, f3, f4, f5, esp;

	if ((Ind2 = get_esp_link(Ind, LINKF_PAIN, &p_ptr2))) {
	}

	/* Extract "effective" player level */
	levels = p_ptr->lev;

	/* Hack -- no negative mana */
	if (levels < 0) levels = 0;

	/* Extract total mana */
	switch(p_ptr->pclass) {
	case CLASS_MAGE:
		/* much Int, few Wis */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 85 * levels / (300)) +
			    (adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 15 * levels / (300));
		break;
	case CLASS_RANGER:
//	case CLASS_DRUID: -- moved
		/* much Int, few Wis */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 85 * levels / (500)) +
			    (adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 15 * levels / (500));
		break;
	case CLASS_PRIEST:
	case CLASS_DRUID:
		/* few Int, much Wis */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 15 * levels / (400)) +
			    (adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 85 * levels / (400));
		break;
	case CLASS_PALADIN:
		/* few Int, much Wis */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 15 * levels / (500)) +
			    (adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 85 * levels / (500));
		break;
	case CLASS_ROGUE:
	case CLASS_MIMIC:
		/* much Int, few Wis */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 85 * levels / (550)) +
			    (adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 15 * levels / (550));
		break;
	case CLASS_ARCHER:
	case CLASS_WARRIOR:
		new_mana = 0;
		break;
	case CLASS_SHAMAN:
#if 0
		/* more Wis than Int */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 35 * levels / (400)) +
			    (adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 65 * levels / (400));
#else
		/* Depends on what's better, his WIS or INT */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    ((p_ptr->stat_ind[A_INT] > p_ptr->stat_ind[A_WIS]) ?
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 100 * levels / (400)) :
			    (adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 100 * levels / (400)));
#endif
		break; 
#ifndef ENABLE_RCRAFT
	/* Need a lot of SP for the spells. Reducing bonus to 1 tho. 2 seems excessive */
#define RUNEMASTER_SP_CALC_BONUS	1 /* 2 */
	case CLASS_RUNEMASTER:
		//2 SP per 1 point in SKILL_MAGIC
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 100) + 
		//Their spells are abit on the expensive side... Allow more max than mages.
		//A different system, really ... See runes.c
		  (RUNEMASTER_SP_CALC_BONUS + adj_mag_mana[p_ptr->stat_ind[A_INT]]) * levels / (2) +
		  (RUNEMASTER_SP_CALC_BONUS + adj_mag_mana[p_ptr->stat_ind[A_DEX]]) * levels / (2) ;
		break;
#else
	case CLASS_RUNEMASTER:
		//Spells are now much closer in cost to mage spells. Returning to a similar mode
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
		    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 65 * levels / (300)) +
		    (adj_mag_mana[p_ptr->stat_ind[A_DEX]] * 35 * levels / (300));
		break;
#endif
	case CLASS_MINDCRAFTER:
		/* much Int, some Chr (yeah!), little Wis */
#if 0 /* too much CHR drastically reduced the amount of viable races, basically only humans and elves remained */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) + /* <- disabled maybe even, in tables.c? */
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 75 * levels / (400)) +
			    (adj_mag_mana[p_ptr->stat_ind[A_CHR]] * 20 * levels / (400)) +
			    (adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 5 * levels / (400));
		break;
#else
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) + /* <- seems this might be important actually */
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 85 * levels / (400)) +
			    (adj_mag_mana[p_ptr->stat_ind[A_CHR]] * 10 * levels / (400)) +
			    (adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 5 * levels / (400));
		break;
#endif

	case CLASS_ADVENTURER:
//	case CLASS_BARD:	
	default:
	    	/* 50% Int, 50% Wis */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
		(adj_mag_mana[p_ptr->stat_ind[A_INT]] * 50 * levels / (550)) +
		(adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 50 * levels / (550));
		break;
	}

	/* Hack -- usually add one mana */
	if (new_mana) new_mana++;

	/* Get the gloves */
	o_ptr = &p_ptr->inventory[INVEN_HANDS];

	/* Examine the gloves */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	/* Only Sorcery/Magery users are affected */
	if (get_skill(p_ptr, SKILL_SORCERY) || get_skill(p_ptr, SKILL_MAGERY))
	{
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

#if 0 // C. Blue - Taken out again since polymorphing ability of DSMs was
// removed instead. Mimics and mimic-sorcs were punished too much.
	/* Forms that don't have proper 'hands' (arms) have mana penalty.
	   this will hopefully stop the masses of D form Istar :/ */
	if (p_ptr->body_monster)
	{
		monster_race *r_ptr = &r_info[p_ptr->body_monster];
		if (!r_ptr->body_parts[BODY_ARMS]) new_mana = (new_mana * 1) / 2;
	}
#endif

	if (new_mana <= 0) new_mana = 1;

	/* Sorcery helps mana */
#if 0 // C. Blue - If sorcery gives HP penalty, that will be nullified by
// mana bonus because of disruption shield. Instead, base mana for istari is raised.
	if (get_skill(p_ptr, SKILL_SORCERY))
	{
		new_mana += (new_mana * get_skill(p_ptr, SKILL_SORCERY)) / 200;
	}
#endif
#if 0 // DGDGDGDG
	/* Mimic really need that */
	if (p_ptr->pclass == CLASS_MIMIC)
	{
		new_mana = (new_mana * 7) / 10;
		if (new_mana < 1) new_mana = 1;
        }
#endif

	/* adjustment so paladins won't become OoD sentry guns and
	   rangers won't become invulnerable manashield tanks, and
	   priests won't become OoD wizards.. (C. Blue)
	   Removed ranger mana penalty here, added handicap in spells1.c
	   where disruption shield is calculated. (C. Blue) */
	switch(p_ptr->pclass) {
	case CLASS_MAGE:
	case CLASS_RANGER:
	case CLASS_DRUID:
		if (p_ptr->to_m) new_mana += new_mana * p_ptr->to_m / 100;
		break;
	/* in theory these actually don't use 'magic mana' at all?: */
	case CLASS_PRIEST:
	case CLASS_PALADIN:
	/* non-holy again: */
	case CLASS_MIMIC:
	case CLASS_ROGUE:
		if (p_ptr->to_m) new_mana += new_mana * p_ptr->to_m / 200;
		break;
	/* hybrids & more */
	case CLASS_SHAMAN:
	case CLASS_ADVENTURER:
	case CLASS_MINDCRAFTER:
	case CLASS_RUNEMASTER:
	default:
		if (p_ptr->to_m) new_mana += new_mana * p_ptr->to_m / 100;
		break;
	}

	/* Meditation increase mana at the cost of hp */
	if (p_ptr->tim_meditation)
	{
		new_mana += (new_mana * get_skill(p_ptr, SKILL_SORCERY)) / 100;
	}

	/* Disruption Shield now increases hp at the cost of mana */
	if (p_ptr->tim_manashield)
	{
	/* commented out (evileye for power) */
	/*	new_mana -= new_mana / 2; */
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
	case CLASS_RANGER: max_wgt = 220 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;
	case CLASS_PRIEST: max_wgt = 250 + get_skill_scale(p_ptr, SKILL_COMBAT, 150); break;
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
	if (((cur_wgt - max_wgt) / 10) > 0)
	{
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

	if (Ind2)
	{
		new_mana += p_ptr2->msp / 2;
	}

	/* Mana can never be negative */
	if (new_mana < 0) new_mana = 0;

	/* Some classes dont use mana */
	if ((p_ptr->pclass == CLASS_WARRIOR) ||
	    (p_ptr->pclass == CLASS_ARCHER))
	{
		new_mana = 0;
	}

#ifdef ARCADE_SERVER
        new_mana = 100;
#endif

	/* Maximum mana has changed */
	if (p_ptr->msp != new_mana)
	{
		/* Player has no mana now */
		if (!new_mana)
		{
			/* No mana left */
			p_ptr->csp = 0;
			p_ptr->csp_frac = 0;
		}

		/* Player had no mana, has some now */
		else if (!p_ptr->msp)
		{
			/* Reset mana */
#if 0 /* completely cheezable restoration */
			p_ptr->csp = new_mana;
#endif
			p_ptr->csp_frac = 0;
		}

		/* Player had some mana, adjust current mana */
		else
		{
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
	if (p_ptr->old_cumber_glove != p_ptr->cumber_glove)
	{
		/* Message */
		if (p_ptr->cumber_glove)
		{
			msg_print(Ind, "\377oYour covered hands feel unsuitable for spellcasting.");
		}
		else
		{
			msg_print(Ind, "\377gYour hands feel more suitable for spellcasting.");
		}

		/* Save it */
		p_ptr->old_cumber_glove = p_ptr->cumber_glove;
	}

	/* Take note when "helm state" changes */
	if (p_ptr->old_cumber_helm != p_ptr->cumber_helm)
	{
		/* Message */
		if (p_ptr->cumber_helm)
		{
			msg_print(Ind, "\377oYour heavy headgear feels unsuitable for mindcrafting.");
		}
		else
		{
			msg_print(Ind, "\377gYour headgear feels more suitable for mindcrafting.");
		}

		/* Save it */
		p_ptr->old_cumber_helm = p_ptr->cumber_helm;
	}


	/* Take note when "armor state" changes */
	if (p_ptr->old_awkward_armor != p_ptr->awkward_armor)
	{
	    if (p_ptr->pclass != CLASS_WARRIOR && p_ptr->pclass != CLASS_ARCHER) {
		/* Message */
		if (p_ptr->awkward_armor)
		{
			msg_print(Ind, "\377oThe weight of your armour strains your spellcasting.");
		}
		else
		{
			msg_print(Ind, "\377gYou feel able to cast more freely.");
		}
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

void calc_hitpoints(int Ind)
{
	player_type *p_ptr = Players[Ind], *p_ptr2 = NULL; /* silence the warning */

//	object_type *o_ptr;
//	u32b f1, f2, f3, f4, f5, esp;

	int bonus, Ind2 = 0, cr_mhp = p_ptr->cp_ptr->c_mhp + p_ptr->rp_ptr->r_mhp;
	long mhp, mhp_playerform, weakling_boost;
	u32b mHPLim, finalHP;
	int bonus_cap;

	if ((Ind2 = get_esp_link(Ind, LINKF_PAIN, &p_ptr2))) {
	}

	/* Un-inflate "half-hitpoint bonus per level" value */
	bonus = ((int)(adj_con_mhp[p_ptr->stat_ind[A_CON]]) - 128);

	/* Calculate hitpoints */
	if (!cfg.bonus_calc_type) {
		/* The traditional way.. */
/*		if (p_ptr->fruit_bat) mhp = (p_ptr->player_hp[p_ptr->lev-1] / 4) + (bonus * p_ptr->lev); //the_sandman: removed hp penalty
		else */ mhp = p_ptr->player_hp[p_ptr->lev-1] + (bonus * p_ptr->lev / 2);
	} else {
#if 1
		/* Don't exaggerate with HP on low levels (especially Ents) (bonus range is -5..+25) */
		bonus_cap = ((p_ptr->lev + 5) * (p_ptr->lev + 5)) / 100;
		if (bonus > bonus_cap) bonus = bonus_cap;
#endif

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
/*		if (p_ptr->fruit_bat)
			mhp = ((p_ptr->player_hp[p_ptr->lev-1] * 4) * (20 + bonus)) / (45 * 3);
		else */ // removed hp penalty -- the_sandman
			mhp = (p_ptr->player_hp[p_ptr->lev-1] * 2 * (20 + bonus)) / 45;

/* I think it might be better to dimish the boost for yeeks after level 50 slowly towards level 100 
   while granting the full boost at the critical level 50 instead of just a minor boost. Reason is:
   They don't have Morgoth's crown yet so they are unlikely to reach *** CON, but after winning
   they will be way stronger! On the other hand yeeks are already so weak that ultra-high Yeek
   Istari could still be instakilled (max. sorcery skill) if the boost is diminished,
   so only Yeek Mimics whose HP greatly reduces the low hitdice effect should be affected,
   as well as Adventurer-Mimics.. since it cannot be skill-based (might change anytime) nor
   class-based, we just punish all Yeeks, that's what they're for after all >;) */
#if 0 /* Reduced boost? */
		if (p_ptr->pclass == CLASS_MAGE || p_ptr->prace != RACE_YEEK)
			weakling_boost = ((p_ptr->lev < 50) ?
					(((p_ptr->lev * p_ptr->lev * p_ptr->lev) / 2500) * ((100 - p_ptr->cp_ptr->c_mhp * p_ptr->cp_ptr->c_mhp) + 20)) / 20 :
					(50 * ((100 - p_ptr->cp_ptr->c_mhp * p_ptr->cp_ptr->c_mhp) + 20)) / 20); /* Don't grow further above level 50 */
		else
			weakling_boost = ((p_ptr->lev < 50) ?
					((((p_ptr->lev * p_ptr->lev * p_ptr->lev) / 2500) * (6 - p_ptr->cp_ptr->c_mhp * 2)) / 3) :
					((50 * (6 - p_ptr->cp_ptr->c_mhp * 2)) / 3)); /* Don't grow further above level 50 */
#endif
#if 1 /* Slowly diminishing boost? */
    #if 0
		weakling_boost = ((p_ptr->lev < 50) ?
				(((p_ptr->lev * p_ptr->lev * p_ptr->lev) / 2500) * ((100 - p_ptr->cp_ptr->c_mhp * p_ptr->cp_ptr->c_mhp) + 20)) / 20 : /* <- full bonus */
/*					(p_ptr->pclass == CLASS_MAGE || p_ptr->prace != RACE_YEEK) ?*/
/*					(p_ptr->pclass != CLASS_MIMIC || p_ptr->prace != RACE_YEEK) ?*/
					((p_ptr->prace != RACE_YEEK) ?
					(50 * ((100 - p_ptr->cp_ptr->c_mhp * p_ptr->cp_ptr->c_mhp) + 20)) / 20 : /* <- full bonus */
					((100 - p_ptr->lev) * ((100 - p_ptr->cp_ptr->c_mhp * p_ptr->cp_ptr->c_mhp) + 20)) / 20)); /* <- full bonus, slowly diminishing */
					/* Note that the diminishing ends at level 100 ;) So it's not that bad */
    #else /* new (July 2008): take class+race hit dice into account, not just class hit dice */
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
    #endif
#endif

		/* Help very weak characters close to level 50 to avoid instakill on trying to win */
		mhp += weakling_boost;
	}

#if 0 // DGDGDGDG why ?
	/* Option : give mages a bonus hitpoint / lvl */
	if (cfg.mage_hp_bonus)
		if (p_ptr->pclass == CLASS_MAGE) mhp += p_ptr->lev;
#endif

#ifndef RPG_SERVER /* already greatly reduced Sorcery Skill ratio in tables.c */
#endif
#if 0 /* tables.c count for all servers now, not just RPG_SERVER */
	/* Sorcery reduces hp */
	if (get_skill(p_ptr, SKILL_SORCERY))
	{
		// mhp -= (mhp * get_skill_scale(p_ptr, SKILL_SORCERY, 20)) / 100;
		mhp -= (mhp * get_skill_scale(p_ptr, SKILL_SORCERY, 33)) / 100;
	}
#endif


        /* HP Bonus from SKILL_HEALTH */
#if 0
	/* it's a bit too much, and doesn't feel 'smooth' at all IMHO, balance-wise :/
	 Mainly because already strong chars can now get uber-tough - but.. see #else branch below..
	 Also, +LIFE is mostly to give non-mimics a chance to catch up a bit to mimics,
	 especially postking. Pre-king using a +LIFE weapon (only way) will be costly since it's
	 a whole slot used up, which could otherwise give resistances or deal more damage from an
	 artifact/ego weapon (archers have it easier than warriors though). */
        if (get_skill(p_ptr, SKILL_HEALTH) >= 30)
	        to_life++;
        if (get_skill(p_ptr, SKILL_HEALTH) >= 40)
	        to_life++;
        if (get_skill(p_ptr, SKILL_HEALTH) >= 50)
	        to_life++;
#else 
	/* instead let's make it an option to weak chars, instead of further buffing chars with don't
         need it at all (warriors, druids, mimics, etc). However, now chars can get this AND +LIFE items.
	 So in total it might be even more. A scale to 100 is hopefully ok. *experimental* */
        mhp += get_skill_scale(p_ptr, SKILL_HEALTH, 100);

 #ifdef ENABLE_DIVINE
	if (p_ptr->prace == RACE_DIVINE && (p_ptr->divinity == DIVINE_DEMON)) {
		mhp += (p_ptr->lev - 20) * 2;
	}
 #endif
#endif


	/* Now we calculated the base player form mhp. Save it for use with
	   +LIFE bonus. This will prevent mimics from total uber HP,
	   and giving them an excellent chance to compensate a form that
	   provides bad HP. - C. Blue */
	mhp_playerform = mhp;


	if (p_ptr->body_monster)
	{
	    long rhp = ((long)(r_info[p_ptr->body_monster].hdice)) * ((long)(r_info[p_ptr->body_monster].hside));

#if 0 /* this gives bad HP if your race has high HD and you don't use an uber-form */
	    /* pre-cap monster HP against ~3500 (5000) */
/*	    mHPLim = (100000 / ((100000 / rhp) + 18)); this was for non RPG_SERVER originally */
	    mHPLim = (50000 / ((50000 / rhp) + 20));/*18*/ /* for the new RPG_SERVER originally */
	    /* average with player HP */
	    finalHP = (mHPLim < mhp ) ? (((mhp * 4) + (mHPLim * 1)) / 5) : (((mHPLim * 2) + (mhp * 3)) / 5);
	    /* cap final HP against ~2300 */
//	    finalHP = (100000 / ((100000 / finalHP) + 20));
#endif
#if 0 /* fairer for using non-uber forms; still bad for races with high HD (thunderlord) */
	    mHPLim = (50000 / ((50000 / rhp) + 20));
	    if (mHPLim < mhp) {
		    levD = p_ptr->lev - r_ptr->level;
		    hpD = mhp - mHPLim;
		    if (levD < 0) levD = 0;
		    mHPLim = mhp - hpD * levD / 20; /* When your form is 20 or more levels below your charlevel,
						       you receive the full HP difference in the formula below. */
	    }
	    finalHP = (mHPLim < mhp ) ? (((mhp * 4) + (mHPLim * 1)) / 5) : (((mHPLim * 2) + (mhp * 3)) / 5);
#endif
#if 0 /* assume human mimic HP for calculation; afterwards, add up our 'extra' hit points if we're a stronger race */
	    long levD, hpD, raceHPbonus;
	    mHPLim = (50000 / ((50000 / rhp) + 20));

	    raceHPbonus = mhp - ((mhp * 15) / p_ptr->hitdie); /* 10 human + 5 mimic */
	    mhp -= raceHPbonus;
	    if (mHPLim < mhp) {
		    levD = p_ptr->lev - r_info[p_ptr->body_monster].level;
		    if (levD < 0) levD = 0;
		    if (levD > 20) levD = 20;
		    hpD = mhp - mHPLim;
		    mHPLim = mhp - (hpD * levD) / 20; /* When your form is 20 or more levels below your charlevel,
						       you receive the full HP difference in the formula below. */
	    }
	    finalHP = (mHPLim < mhp ) ? (((mhp * 4) + (mHPLim * 1)) / 5) : (((mHPLim * 2) + (mhp * 3)) / 5);
	    finalHP += raceHPbonus;
#endif
#if 1 /* assume human mimic HP for calculation; afterwards, add up our 'extra' hit points if we're a stronger race */
	/* additionally, scale form HP better with very high character levels */
	    long levD, hpD, raceHPbonus;
	    mHPLim = (50000 / ((50000 / rhp) + 20));

 #if 0 /* done below */
	    /* add flat bonus to maximum HP limit for char levels > 50, if form is powerful, to keep it useful */
	    mHPLim += p_ptr->lev > 50 ? (((p_ptr->lev - 50) * (r_info[p_ptr->body_monster].level + 30)) / 100) * 20 : 0;
 #endif

	    raceHPbonus = mhp - ((mhp * 15) / p_ptr->hitdie); /* 10 human + 5 mimic */
	    mhp -= raceHPbonus;
	    if (mHPLim < mhp) {
		    levD = p_ptr->lev - r_info[p_ptr->body_monster].level;
		    if (levD < 0) levD = 0;
		    if (levD > 20) levD = 20;
		    hpD = mhp - mHPLim;
		    mHPLim = mhp - (hpD * levD) / 20; /* When your form is 20 or more levels below your charlevel,
						       you receive the full HP difference in the formula below. */
	    }
	    finalHP = (mHPLim < mhp ) ? (((mhp * 4) + (mHPLim * 1)) / 5) : (((mHPLim * 2) + (mhp * 3)) / 5);
	    finalHP += raceHPbonus;
#endif

	    /* done */
	    mhp = finalHP;
	}

	/* Always have at least one hitpoint per level */
	if (mhp < p_ptr->lev + 1) mhp = p_ptr->lev + 1;

	/* for C_BLUE_AI */
	if (!p_ptr->body_monster) p_ptr->form_hp_ratio = 100;
	else p_ptr->form_hp_ratio = (mhp * 100) / mhp_playerform;

	/* Bonus from +LIFE items (should be weapons only -- not anymore, Bladeturner / Randarts etc.!).
	   Also, cap it at +3 (boomerang + weapon could result in +6) (Boomerangs can't have +LIFE anymore) */
	if (!is_admin(p_ptr) && p_ptr->to_l > 3) p_ptr->to_l = 3;

	/* Reduce use of +LIFE items for mimics while in monster-form */
	if (mhp > mhp_playerform) {
		if (p_ptr->to_l > 0)
			mhp += (mhp_playerform * p_ptr->to_l * mhp_playerform) / (10 * mhp);
		else
			mhp += (mhp_playerform * p_ptr->to_l) / 10;
	} else {
		mhp += (mhp * p_ptr->to_l) / 10;
	}
#if 1
	if (p_ptr->body_monster) {
		/* add flat bonus to maximum HP limit for char levels > 50, if form is powerful, to keep it useful */
		mhp += p_ptr->lev > 50 ? (((p_ptr->lev - 50) * ((r_info[p_ptr->body_monster].level > 80 ? 80 : r_info[p_ptr->body_monster].level) + 30)) / 100) * 8 : 0;
	}
#endif

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
	if (p_ptr->tim_meditation)
	{
		mhp = mhp * 3 / 5;
	}

	/* Disruption Shield */
	if (p_ptr->tim_manashield)
	{
	/* commented out (evileye for power) */
	/*	mhp += p_ptr->msp * 2 / 3; */
	}

	/* Factor in the hero / superhero settings */
	if (p_ptr->hero) mhp += 10;
	if (p_ptr->shero) mhp += 20;
#if 1 /* hmm */
	else if (p_ptr->fury) mhp += 20;
	else if (p_ptr->berserk) mhp += 20;
#endif
	

	/* New maximum hitpoints */
	if (mhp != p_ptr->mhp)
	{
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
static void calc_torch(int Ind)
{
	player_type *p_ptr = Players[Ind];
	
#if 0
	object_type *o_ptr = &p_ptr->inventory[INVEN_LITE];

	/* Base light radius */
	p_ptr->cur_lite = p_ptr->lite;

	/* Examine actual lites */
	if (o_ptr->tval == TV_LITE)
	{
		/* Torches (with fuel) provide some lite */
		if ((o_ptr->sval == SV_LITE_TORCH) && (o_ptr->pval > 0))
		{
			p_ptr->cur_lite += 1;
		}

		/* Lanterns (with fuel) provide more lite */
		if ((o_ptr->sval == SV_LITE_LANTERN) && (o_ptr->pval > 0))
		{
			p_ptr->cur_lite += 2;
		}

		/* Dwarven lanterns provide permanent radius 2 lite */
		if (o_ptr->sval == SV_LITE_DWARVEN)
		{
			p_ptr->cur_lite += 2;
		}

		/* Feanorian lanterns provide permanent, bright, lite */
		if (o_ptr->sval == SV_LITE_FEANOR)
		{
			p_ptr->cur_lite += 3;
		}

		/* Artifact Lites provide permanent, bright, lite */
		if (artifact_p(o_ptr)) p_ptr->cur_lite += 3;
	}
#endif	// 0

	/* Reduce lite when running if requested */
	if (p_ptr->running && p_ptr->view_reduce_lite)
	{
		/* Reduce the lite radius if needed */
		if (p_ptr->cur_lite > 1) p_ptr->cur_lite = 1;
		if (p_ptr->cur_vlite > 1) p_ptr->cur_vlite = 1;
	}

	/* Notice changes in the "lite radius" */
	if ((p_ptr->old_lite != p_ptr->cur_lite) || (p_ptr->old_vlite != p_ptr->cur_vlite))
	{
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
static int weight_limit(int Ind) /* max. 3000 atm */
{
	player_type *p_ptr = Players[Ind];

	int i;

	/* Weight limit based only on strength */
	i = adj_str_wgt[p_ptr->stat_ind[A_STR]] * 100;

	/* Return the result */
	return (i);
}


/* Should be called by every calc_bonus call */
static void calc_body_bonus(int Ind)
{
	player_type *p_ptr = Players[Ind];
	dun_level *l_ptr = getfloor(&p_ptr->wpos);
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return;

	int n, d, immunities = 0, immunity[7], immrand[3]; //, toac = 0, body = 0;
	int i, j;
	bool wepless = FALSE;
	monster_race *r_ptr = &r_info[p_ptr->body_monster];
	
	immunity[1] = 0; immunity[2] = 0; immunity[3] = 0;
	immunity[4] = 0; immunity[5] = 0; immunity[6] = 0;
	immrand[1] = 0; immrand[2] = 0;

	/* If in the player body nothing have to be done */
	if (!p_ptr->body_monster) return;

	/* keep random monster-body abilities static over quit/rejoin */
	Rand_value = p_ptr->mimic_seed;
	Rand_quick = TRUE;

	if (!r_ptr->body_parts[BODY_WEAPON])
	{
		wepless = TRUE;
//		p_ptr->num_blow = 0;
		p_ptr->num_blow = 1;
	}

	d = 0; n = 0;
	for (i = 0; i < 4; i++)
	{
		j = (r_ptr->blow[i].d_dice * r_ptr->blow[i].d_side);

		if (j) n++;
		
		switch (r_ptr->blow[i].effect)
		{
			case RBE_EXP_10:	case RBE_EXP_20:	case RBE_EXP_40:	case RBE_EXP_80:
			p_ptr->hold_life = TRUE;
		}

		/* Hack -- weaponless combat */
		if (wepless && j)
		{
//MA overrides this anyways:
//			p_ptr->num_blow++;
//			j *= 2;
		}

		d += (j * 2);
	}
	d /= 4;
	
	/* Apply STR bonus/malus, derived from form damage and race */
	if (!d) d = 1;
	/* 0..147 (greater titan) -> 0..5 -> -1..+4 */
	i = (((15000 / ((15000 / d) + 50)) / 29) - 1);
	if (strstr(r_name + r_ptr->name, "bear") && (r_ptr->flags3 & RF3_ANIMAL)) i++; /* Bears get +1 STR */
	if (r_ptr->flags3 & RF3_TROLL) i+=1;
	if (r_ptr->flags3 & RF3_GIANT) i+=1;
	if ((r_ptr->flags3 & RF3_DRAGON) && (strstr(r_name + r_ptr->name, "Mature ") ||
	    (r_ptr->d_char == 'D')))
		i+=1; /* only +1 since more bonus is coming from its damage already */
	p_ptr->stat_add[A_STR] += i;

#if 0
	if (n == 0) n = 1;
	/*d = (d / 2) / n;	// 8 // 7
	p_ptr->to_d += d;
	p_ptr->dis_to_d += d; - similar to HP: */
	d = d / n;
	if (d < (p_ptr->to_d + p_ptr->to_d_melee)) {
		p_ptr->to_d = ((p_ptr->to_d * 2) + (d * 1)) / 3;
		p_ptr->to_d_melee = (p_ptr->to_d_melee * 2) / 3;
		p_ptr->dis_to_d = ((p_ptr->dis_to_d * 2) + (d * 1)) / 3;
	} else {
		p_ptr->to_d = ((p_ptr->to_d * 1) + (d * 1)) / 2;
		p_ptr->to_d_melee = (p_ptr->to_d_melee * 1) / 2;
		p_ptr->dis_to_d = ((p_ptr->dis_to_d * 1) + (d * 1)) / 2;
	}
/*	p_ptr->dis_to_d = (d < p_ptr->dis_to_d) ?
		    (((p_ptr->dis_to_d * 2) + (d * 1)) / 3) :
		    (((p_ptr->dis_to_d * 1) + (d * 1)) / 2);*/
#endif
#if 0 /* moved to calc_boni() */
	/* Evaluate monster AC (if skin or armor etc) */
	body = (r_ptr->body_parts[BODY_HEAD] ? 1 : 0)
		+ (r_ptr->body_parts[BODY_TORSO] ? 3 : 0)
		+ (r_ptr->body_parts[BODY_ARMS] ? 2 : 0)
		+ (r_ptr->body_parts[BODY_LEGS] ? 1 : 0);

	toac = r_ptr->ac * 14 / (7 + body);
	/* p_ptr->ac += toac;
	p_ptr->dis_ac += toac; - similar to HP calculation: */
	if (toac < (p_ptr->ac + p_ptr->to_a)) {
		/* Vary between 3/4 + 1/4 (low monsters) to 2/3 + 1/3 (high monsters): */
		p_ptr->ac = (p_ptr->ac * (100 - r_ptr->level + 200)) / (100 - r_ptr->level + 300);
		p_ptr->to_a = ((p_ptr->to_a * (100 - r_ptr->level + 200)) + (toac * 100)) / (100 - r_ptr->level + 300);
		p_ptr->dis_ac = ((p_ptr->dis_ac * (100 - r_ptr->level + 200)) + (toac * 100)) / (100 - r_ptr->level + 300);
	} else {
		p_ptr->ac = (p_ptr->ac * 1) / 2;
		p_ptr->to_a = ((p_ptr->to_a * 1) + (toac * 1)) / 2;
		p_ptr->dis_ac = ((p_ptr->dis_ac * 1) + (toac * 1)) / 2;
	}
/*	p_ptr->dis_ac = (toac < p_ptr->dis_ac) ?
		    (((p_ptr->dis_ac * 2) + (toac * 1)) / 3) :
		    (((p_ptr->dis_ac * 1) + (toac * 1)) / 2);*/
#endif
	if (r_ptr->speed < 110)
	{
		/* let slowdown not be that large that players will never try that form */
		p_ptr->pspeed = 110 - (((110 - r_ptr->speed) * 20) / 100);
	}
	else
	{
		/* let speed bonus not be that high that players won't try any slower form */
		//p_ptr->pspeed = (((r_ptr->speed - 110) * 30) / 100) + 110;//was 50%, 30% for RPG_SERVER originally
		//Pfft, include the -speed into the calculation, too. Seems lame how -speed is counted for 100% but not + bonus.
		//But really, if physical-race-intrinsic bonuses/maluses are counted in mimicry, then dwarves 
		//should be able to keep their climbing ability past 30 when mimicked, TLs could fly, etc etc =/
		p_ptr->pspeed = (((r_ptr->speed - 110 - (p_ptr->prace == RACE_ENT ? 2 : 0) ) * 30) / 100) + 110;//was 50%, 30% for RPG_SERVER originally
	}

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
	if (r_ptr->weight <= 500) p_ptr->skill_stl += 2;
	else if (r_ptr->weight <= 500) p_ptr->skill_stl += 2;
	else if (r_ptr->weight <= 1000) p_ptr->skill_stl += 1;
	else if (r_ptr->weight <= 1500) p_ptr->skill_stl += 0;
	else if (r_ptr->weight <= 4500) p_ptr->skill_stl -= 1;
	else if (r_ptr->weight <= 20000) p_ptr->skill_stl -= 2;
	else if (r_ptr->weight <= 100000) p_ptr->skill_stl -= 3;
	else p_ptr->skill_stl -= 4;
	if (p_ptr->skill_stl < 0) p_ptr->skill_stl = 0;

	/* Extra fire if good archer */
	/*  1_IN_1  -none-
	    1_IN_2  Grandmaster Thief
	    1_IN_3  Halfling Slinger, Ranger Chieftain
	    1_IN_4  Ranger
	*/
	if ((r_ptr->flags4 & (RF4_ARROW_1 | RF4_ARROW_2 | RF4_ARROW_3)) &&	/* 1=BOW,2=XBOW,3=SLING; 4=generic missile */
	    (p_ptr->inventory[INVEN_BOW].k_idx) && (p_ptr->inventory[INVEN_BOW].tval == TV_BOW))
	{
#if 0 /* normal way to handle xbow <-> sling/bow shot frequency */
		if ((p_ptr->inventory[INVEN_BOW].sval == SV_SLING) ||
		    (p_ptr->inventory[INVEN_BOW].sval == SV_SHORT_BOW) ||
		    (p_ptr->inventory[INVEN_BOW].sval == SV_LONG_BOW)) p_ptr->num_fire++;
		if (r_ptr->freq_inate > 30) p_ptr->num_fire++; /* this time for crossbows too */
#else /* give xbow an advantage */
		if (r_ptr->freq_inate > 30) 
		if ((p_ptr->inventory[INVEN_BOW].sval == SV_SLING) ||
		    (p_ptr->inventory[INVEN_BOW].sval == SV_SHORT_BOW) ||
		    (p_ptr->inventory[INVEN_BOW].sval == SV_LONG_BOW)) p_ptr->num_fire++;
		p_ptr->num_fire++; /* this time for crossbows too */
#endif
	}

	/* Extra casting if good spellcaster */
	if ((r_ptr->flags4 & RF4_SPELLCASTER_MASK) ||
	    (r_ptr->flags5 & RF5_SPELLCASTER_MASK) ||
	    (r_ptr->flags6 & RF6_SPELLCASTER_MASK)) {
		if (r_ptr->freq_inate > 30) {
			p_ptr->num_spell++;	// 1_IN_3
			p_ptr->stat_add[A_INT] += 1;
			p_ptr->to_m += 10;
		}
		if (r_ptr->freq_inate >= 50) {
			p_ptr->num_spell++;	// 1_IN_2
			p_ptr->stat_add[A_INT] += 2;
			p_ptr->to_m += 20;
		}
		if (r_ptr->freq_inate == 100) { /* well, _which_ monster would that be? :) */
			p_ptr->num_spell++;	// 1_IN_1
			p_ptr->stat_add[A_INT] += 2;
			p_ptr->to_m += 10;
		}
	}


	/* Racial boni depending on the form's race */
	switch(p_ptr->body_monster)
	{
		/* Bats get feather falling */
		case 37:	case 114:	case 187:	case 235:	case 351:
		case 377:	case 391:	case 406:	case 484:	case 968:
		{
			p_ptr->feather_fall = TRUE;
			/* Vampire bats are vampiric */
			if (p_ptr->body_monster == 391) p_ptr->vampiric_melee = 100;
			break;
		}
		
		case 365: /* Vampiric mist is vampiric */
			p_ptr->vampiric_melee = 100;
			break;
		case 927: /* Vampiric ixitxachitl is vampiric */
			if (p_ptr->vampiric_melee < 50) p_ptr->vampiric_melee = 50;
			break;

		/* Elves get resist_lite, Dark-Elves get resist_dark */
		case 122:	case 400:	case 178:	case 182:	case 226:
		case 234:	case 348:	case 375:	case 564:	case 657:
		{
			p_ptr->resist_dark = TRUE;
			break;
		}
		case 864:
		{
			p_ptr->resist_lite = TRUE;
			break;
		}

		/* Hobbits/Halflings get the no-shoes-bonus */
		case 74:	case 539:
		{
			if (!p_ptr->inventory[INVEN_FEET].k_idx)
				p_ptr->stat_add[A_DEX] += 2;
			break;
		}
		
		/* Gnomes get free_act */
		case 258:	case 281:
		{
			p_ptr->free_act = TRUE;
			break;
		}

		/* Dwarves get res_blind & climbing ability */
		case 111:	case 865:
		{
			p_ptr->resist_blind = TRUE;
			if (p_ptr->lev >= 30) p_ptr->climb = TRUE;
			break;
		}
		
		/* High-elves resist_lite & see_inv */
		case 945:
		{
			p_ptr->resist_lite = TRUE;
			p_ptr->see_inv = TRUE;
			break;
		}

		/* Yeeks get feather_fall */
		case 52:	case 141:	case 179:	case 224:
		{
			p_ptr->feather_fall = TRUE;
			break;
		}

		/* Ents */
		case 708:
		{
			p_ptr->slow_digest = TRUE;
			//if (p_ptr->prace != RACE_ENT) 
			p_ptr->pspeed -= 2;
			p_ptr->suscep_fire = TRUE;
			p_ptr->resist_water = TRUE;
/* not form-dependant:			if (p_ptr->lev >= 4) p_ptr->see_inv = TRUE; */
			p_ptr->can_swim = TRUE; /* wood? */
			p_ptr->pass_trees = TRUE;
			break;
		}
		
		/* Ghosts get additional boni - undead see below */
		case 65:	case 100:	case 133:	case 152:	case 231:
		case 385:	case 394:	case 477:	case 507:	case 508:
		case 533:	case 534:	case 553:	case 630:	case 665:
		case 667:	case 690:	case 774:	case 895:
		case 929:	case 930:	case 931:	case 932:	case 933:
		case 967:	case 973:	case 974:
		{
			/* I'd prefer ghosts having a radius of awareness, like a 'pseudo-light source',
			since atm ghosts are completely blind in the dark :( -C. Blue */
			p_ptr->see_inv = TRUE;
			p_ptr->see_infra += 3;
	//		p_ptr->invis += 5; */ /* No. */
			break;
		}
		
		/* Vampires have VAMPIRIC attacks */
		case 432:	case 520:	case 521:	case 623:	case 989:
			if (p_ptr->vampiric_melee < 50) p_ptr->vampiric_melee = 50;
			break;
		
		/* Angels resist light, blindness and poison (usually immunity) */
		case 417:	case 456:	case 511:	case 605:	
		case 661:	case 1071:	case 1072:	case 1073:
			p_ptr->resist_lite = TRUE;
		/* Fallen Angel */
		case 652:
			p_ptr->resist_blind = TRUE;
			p_ptr->resist_pois = TRUE;
			break;
	}
	
	/* If monster has a lite source, but doesn't prove a torso (needed
	   to use a lite source, yellow light) or can breathe light (light
	   hound), add to the player's light radius! */
	if ((r_ptr->flags9 & RF9_HAS_LITE) &&
	     ((!r_ptr->body_parts[BODY_TORSO]) || (r_ptr->flags4 & RF4_BR_LITE)))
		p_ptr->cur_lite += 1;

	/* Forms that occur in the woods are able to pass them, so are animals */	
	if ((r_ptr->flags8 & RF8_WILD_WOOD) || (r_ptr->flags3 & RF3_ANIMAL))
		p_ptr->pass_trees = TRUE;

	/* Forms that occur in the mountains are able to pass them */
	if (r_ptr->flags8 & (RF8_WILD_MOUNTAIN | RF8_WILD_VOLCANO))
		p_ptr->climb = TRUE;
	/* Spiders can always climb */
	if (r_ptr->flags7 & RF7_SPIDER) p_ptr->climb = TRUE;

	/* Orcs get resist_dark */
	if(r_ptr->flags3 & RF3_ORC) p_ptr->resist_dark = TRUE;

	/* Trolls/Giants get sustain_str */
	if((r_ptr->flags3 & RF3_TROLL) || (r_ptr->flags3 & RF3_GIANT)) p_ptr->sustain_str = TRUE;

	/* Thunderlords get feather_fall, ESP_DRAGON */
	if(r_ptr->flags3 & RF3_DRAGONRIDER)
	{
	        p_ptr->feather_fall = TRUE;
		if (p_ptr->lev >= 5) p_ptr->telepathy |= ESP_DRAGON;
		if (p_ptr->lev >= 30) p_ptr->fly = TRUE;
	}

	/* Undead get lots of stuff similar to player ghosts */
	if(r_ptr->flags3 & RF3_UNDEAD)
	{
		/* p_ptr->see_inv = TRUE;
		p_ptr->resist_neth = TRUE;
		p_ptr->hold_life = TRUE;
		p_ptr->free_act = TRUE;
		p_ptr->see_infra += 3;
    		p_ptr->resist_fear = TRUE;*/
		/*p_ptr->resist_conf = TRUE;*/
		/* p_ptr->resist_pois = TRUE; */ /* instead of immune */
		/* p_ptr->resist_cold = TRUE; */

		p_ptr->resist_pois = TRUE;
		p_ptr->resist_dark = TRUE;
		p_ptr->resist_blind = TRUE;
		p_ptr->no_cut = TRUE;
		p_ptr->reduce_insanity = 1;
		p_ptr->see_infra += 1;
	}

	/* Non-living got a nice ability set too ;) */
	if(r_ptr->flags3 & RF3_NONLIVING)
	{
		p_ptr->resist_pois = TRUE;
		p_ptr->resist_fear = TRUE;
		p_ptr->reduce_insanity = 2;
	}

	/* Greater demons resist poison (monsters are immune) */
	if((r_ptr->d_char == 'U') && (r_ptr->flags3 & RF3_DEMON)) p_ptr->resist_pois = TRUE;

	/* Affect charisma by appearance */
	d = 0;
	if(r_ptr->flags3 & RF3_DRAGONRIDER) d = 1;
	else if(r_ptr->flags3 & RF3_DRAGON) d = 0;
	if(r_ptr->flags3 & RF3_ANIMAL) {
		if (r_ptr->weight <= 450 && strchr("bfqBCR", r_ptr->d_char)) d = 1; /* yes, I included NEWTS */
		else d = 0;
	}
	if(r_ptr->flags7 & RF7_SPIDER) d = -1;

	if(r_ptr->flags3 & RF3_ORC) d = -1;

	if(r_ptr->flags3 & RF3_NONLIVING) d = -1;
	if(r_ptr->flags3 & RF3_EVIL) d = -1;

	if(r_ptr->flags3 & RF3_TROLL) d = -2;
	if(r_ptr->flags3 & RF3_GIANT) d = -2;

	if(r_ptr->flags3 & RF3_UNDEAD) d = -3;
	if(r_ptr->flags3 & RF3_DEMON) d = -3;

	if(r_ptr->flags3 & RF3_GOOD) d += 2;

	p_ptr->stat_add[A_CHR] += d;


	//        if(r_ptr->flags1 & RF1_NEVER_MOVE) p_ptr->immovable = TRUE;
	if(r_ptr->flags2 & RF2_STUPID) p_ptr->stat_add[A_INT] -= 2;
	if(r_ptr->flags2 & RF2_SMART) p_ptr->stat_add[A_INT] += 2;
	if(r_ptr->flags2 & RF2_INVISIBLE){
		//		p_ptr->tim_invisibility = 100;
		p_ptr->tim_invis_power = p_ptr->lev * 4 / 5;
	}
	if(r_ptr->flags2 & RF2_REGENERATE) p_ptr->regenerate = TRUE;
	/* Immaterial forms (WRAITH / PASS_WALL) drain the mimic's HP! */
	if(r_ptr->flags2 & RF2_PASS_WALL) {
		if (!(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) &&
                    !(p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC)))
                {
//BAD!(recursion)			set_tim_wraith(Ind, 30000);
			p_ptr->tim_wraith = 30000;
		}
		p_ptr->drain_life++;
	}
	if(r_ptr->flags2 & RF2_KILL_WALL) p_ptr->auto_tunnel = TRUE;
	if(r_ptr->flags2 & RF2_AURA_FIRE) p_ptr->sh_fire = TRUE;
	if(r_ptr->flags2 & RF2_AURA_ELEC) p_ptr->sh_elec = TRUE;
	if(r_ptr->flags2 & RF3_AURA_COLD) p_ptr->sh_cold = TRUE;

	if(r_ptr->flags5 & RF5_MIND_BLAST) p_ptr->reduce_insanity = 1;
	if(r_ptr->flags5 & RF5_BRAIN_SMASH) p_ptr->reduce_insanity = 2;

	if(r_ptr->flags3 & RF3_SUSCEP_FIRE) p_ptr->suscep_fire = TRUE;
	if(r_ptr->flags3 & RF3_SUSCEP_COLD) p_ptr->suscep_cold = TRUE;
/* Imho, there should be only suspec fire and cold since these two are opposites.
Something which is fire-related will usually be suspectible to cold and vice versa.
Exceptions are rare, like Ent, who as a being of wood is suspectible to fire. (C. Blue) */
	if(r_ptr->flags9 & RF9_SUSCEP_ELEC) p_ptr->suscep_elec = TRUE;
	if(r_ptr->flags9 & RF9_SUSCEP_ACID) p_ptr->suscep_acid = TRUE;
	if(r_ptr->flags9 & RF9_SUSCEP_POIS) p_ptr->suscep_pois = TRUE;

	if(r_ptr->flags9 & RF9_RES_ACID) p_ptr->resist_acid = TRUE;
	if(r_ptr->flags9 & RF9_RES_ELEC) p_ptr->resist_elec = TRUE;
	if(r_ptr->flags9 & RF9_RES_FIRE) p_ptr->resist_fire = TRUE;
	if(r_ptr->flags9 & RF9_RES_COLD) p_ptr->resist_cold = TRUE;
	if(r_ptr->flags9 & RF9_RES_POIS) p_ptr->resist_pois = TRUE;
	
	if(r_ptr->flags3 & RF3_HURT_LITE) p_ptr->suscep_lite = TRUE;
#if 0 /* for now let's say EVIL is a state of mind, so the mimic isn't necessarily evil */
	if(r_ptr->flags3 & RF3_EVIL) p_ptr->suscep_good = TRUE;
#else /* the appearance is important for damage though - let's keep it restricted to demon/undead for now */
	if(r_ptr->flags3 & RF3_DEMON || r_ptr->flags3 & RF3_UNDEAD) p_ptr->suscep_good = TRUE;
#endif
	if(r_ptr->flags3 & RF3_UNDEAD) p_ptr->suscep_life = TRUE;

	/* Grant a mimic a maximum of 2 immunities for now. All further immunities
	are turned into resistances. Which ones is random. */
	if(r_ptr->flags3 & RF3_IM_ACID)
	{
	    immunities += 1;
	    immunity[immunities] = 1;
	    p_ptr->resist_acid = TRUE;
	}
	if(r_ptr->flags3 & RF3_IM_ELEC)
	{
	    immunities += 1;
	    immunity[immunities] = 2;
	    p_ptr->resist_elec = TRUE;
	}
	if(r_ptr->flags3 & RF3_IM_FIRE)
	{
	    immunities += 1;
	    immunity[immunities] = 3;
	    p_ptr->resist_fire = TRUE;
	}
	if(r_ptr->flags3 & RF3_IM_COLD)
	{
	    immunities += 1;
	    immunity[immunities] = 4;
	    p_ptr->resist_cold = TRUE;
	}
	if(r_ptr->flags3 & RF3_IM_POIS)
	{
	    immunities += 1;
	    immunity[immunities] = 5;
	    p_ptr->resist_pois = TRUE;
	}
	if(r_ptr->flags9 & RF9_IM_WATER)
	{
	    immunities += 1;
	    immunity[immunities] = 6;
	    p_ptr->resist_water = TRUE;
	}

	/* gain not more than 1 immunities at the same time from a form */
	if(immunities < 2)
	{
		if(r_ptr->flags3 & RF3_IM_ACID) p_ptr->immune_acid = TRUE;
		if(r_ptr->flags3 & RF3_IM_ELEC) p_ptr->immune_elec = TRUE;
		if(r_ptr->flags3 & RF3_IM_FIRE) p_ptr->immune_fire = TRUE;
		if(r_ptr->flags3 & RF3_IM_COLD) p_ptr->immune_cold = TRUE;
		if(r_ptr->flags3 & RF3_IM_POIS) p_ptr->immune_poison = TRUE;
		if(r_ptr->flags9 & RF9_IM_WATER) p_ptr->immune_water = TRUE;
	}
	else
	{
		immrand[1] = 1 + rand_int(immunities);
		immrand[2] = immrand[1]; /* now only get 1 immunity from form */
/*		immrand[2] = 1 + rand_int(immunities - 1);
		if(!(immrand[2] < immrand[1])) immrand[2]++;
*/
		if((immunity[immrand[1]] == 1) || (immunity[immrand[2]] == 1)) p_ptr->immune_acid = TRUE;
		if((immunity[immrand[1]] == 2) || (immunity[immrand[2]] == 2)) p_ptr->immune_elec = TRUE;
		if((immunity[immrand[1]] == 3) || (immunity[immrand[2]] == 3)) p_ptr->immune_fire = TRUE;
		if((immunity[immrand[1]] == 4) || (immunity[immrand[2]] == 4)) p_ptr->immune_cold = TRUE;
		if((immunity[immrand[1]] == 5) || (immunity[immrand[2]] == 5)) p_ptr->immune_poison = TRUE;
		if((immunity[immrand[1]] == 6) || (immunity[immrand[2]] == 6)) p_ptr->immune_water = TRUE;
	}
	
	if(r_ptr->flags9 & RF9_RES_LITE) p_ptr->resist_lite = TRUE;
	if(r_ptr->flags9 & RF9_RES_DARK) p_ptr->resist_dark = TRUE;
	if(r_ptr->flags9 & RF9_RES_BLIND) p_ptr->resist_blind = TRUE;
	if(r_ptr->flags9 & RF9_RES_SOUND) p_ptr->resist_sound = TRUE;
	if(r_ptr->flags3 & RF3_RES_PLAS) p_ptr->resist_plasma = TRUE;
	if(r_ptr->flags9 & RF9_RES_CHAOS) p_ptr->resist_chaos = TRUE;
	if(r_ptr->flags9 & RF9_RES_TIME) p_ptr->resist_time = TRUE;
	if(r_ptr->flags9 & RF9_RES_MANA) p_ptr->resist_mana = TRUE;
	if((r_ptr->flags9 & RF9_RES_SHARDS) || (r_ptr->flags3 & RF3_HURT_ROCK))
		p_ptr->resist_shard = TRUE;

	if(r_ptr->flags3 & RF3_RES_TELE) p_ptr->res_tele = TRUE;
	if(r_ptr->flags9 & RF9_IM_TELE) p_ptr->res_tele = TRUE;
	if(r_ptr->flags3 & RF3_RES_PLAS)
	{
	    p_ptr->resist_fire = TRUE;
	    /* p_ptr->oppose_fire = TRUE; */
	}
	if(r_ptr->flags3 & RF3_RES_WATE) p_ptr->resist_water = TRUE;
	if(r_ptr->flags3 & RF3_RES_NETH) p_ptr->resist_neth = TRUE;
	if(r_ptr->flags3 & RF3_RES_NEXU) p_ptr->resist_nexus = TRUE;
	if(r_ptr->flags3 & RF3_RES_DISE) p_ptr->resist_disen = TRUE;
	if(r_ptr->flags3 & RF3_NO_FEAR) p_ptr->resist_fear = TRUE;
	if(r_ptr->flags3 & RF3_NO_SLEEP) p_ptr->free_act = TRUE;
	if(r_ptr->flags3 & RF3_NO_CONF) p_ptr->resist_conf = TRUE;
	if(r_ptr->flags3 & RF3_NO_STUN) p_ptr->resist_sound = TRUE;
	if(r_ptr->flags8 & RF8_NO_CUT) p_ptr->no_cut = TRUE;
	if(r_ptr->flags7 & RF7_CAN_FLY) p_ptr->fly = TRUE;
	if(r_ptr->flags7 & RF7_CAN_SWIM) p_ptr->can_swim = TRUE;
	if(r_ptr->flags2 & RF2_REFLECTING) p_ptr->reflect = TRUE;
	if(r_ptr->flags7 & RF7_DISBELIEVE)
	{
#if 0
		p_ptr->antimagic += r_ptr->level / 2 + 20;
		p_ptr->antimagic_dis += r_ptr->level / 15 + 3;
#else /* a bit stricter for mimics.. */
		p_ptr->antimagic += r_ptr->level / 2 + 10;
		p_ptr->antimagic_dis += r_ptr->level / 50 + 2;
#endif
	}

	if((r_ptr->flags2 & RF2_WEIRD_MIND) ||
	    (r_ptr->flags9 & RF9_RES_PSI))
		p_ptr->reduce_insanity = 1;
	if((r_ptr->flags2 & RF2_EMPTY_MIND) ||
	    (r_ptr->flags9 & RF9_IM_PSI))
		p_ptr->reduce_insanity = 2;

	/* as long as not all resistances are implemented in r_info, use workaround via breaths */
	if(r_ptr->flags4 & RF4_BR_LITE) p_ptr->resist_lite = TRUE;
	if(r_ptr->flags4 & RF4_BR_DARK) p_ptr->resist_dark = TRUE;
	/* if(r_ptr->flags & RF__) p_ptr->resist_blind = TRUE; */
	if(r_ptr->flags4 & RF4_BR_SOUN) p_ptr->resist_sound = TRUE;
	if(r_ptr->flags4 & RF4_BR_SHAR) p_ptr->resist_shard = TRUE;
	if(r_ptr->flags4 & RF4_BR_CHAO) p_ptr->resist_chaos = TRUE;
	if(r_ptr->flags4 & RF4_BR_TIME) p_ptr->resist_time = TRUE;
	if(r_ptr->flags4 & RF4_BR_MANA) p_ptr->resist_mana = TRUE;
	if(r_ptr->flags4 & RF4_BR_PLAS)
	{
	    p_ptr->resist_fire = TRUE;
	    /* p_ptr->oppose_fire = TRUE; */
	}
	/* if((r_ptr->flags4 & RF4_BR_WATE) || <- does not exist */
	if (r_ptr->flags7 & RF7_AQUATIC) p_ptr->resist_water = TRUE;
	if (r_ptr->flags4 & RF4_BR_NETH) p_ptr->resist_neth = TRUE;
	/* res_neth_somewhat: (r_ptr->flags3 & RF3_EVIL) */
	if(r_ptr->flags4 & RF4_BR_NEXU) p_ptr->resist_nexus = TRUE;
	if(r_ptr->flags4 & RF4_BR_DISE) p_ptr->resist_disen = TRUE;

	/* The following BR-to-RES will be needed even with all of above RES implemented: */
	if(r_ptr->flags4 & RF4_BR_GRAV) p_ptr->feather_fall = TRUE;
	if(r_ptr->flags4 & RF4_BR_INER) p_ptr->free_act = TRUE;

	/* If not changed, spells didnt changed too, no need to send them */
	if (!p_ptr->body_changed)
	{
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
	if(!(r_ptr->flags2 & RF2_PASS_WALL) && p_ptr->tim_wraith)
		p_ptr->tim_wraith = 1;


	/* Update the innate spells */
	p_ptr->innate_spells[0] = r_ptr->flags4 & RF4_PLAYER_SPELLS;
	p_ptr->innate_spells[1] = r_ptr->flags5 & RF5_PLAYER_SPELLS;
	p_ptr->innate_spells[2] = r_ptr->flags6 & RF6_PLAYER_SPELLS;
	Send_spell_info(Ind, 0, 0, 0, "nothing");

	/* restore RNG */
	Rand_quick = FALSE;
}

#if 0	// moved to defines.h
bool monk_heavy_armor(int Ind)
{
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
int get_weaponmastery_skill(player_type *p_ptr, object_type *o_ptr)
{
	int i, skill = 0;
//	object_type *o_ptr = &p_ptr->inventory[slot];

	i = 0;

        if (!o_ptr->k_idx || o_ptr->tval == TV_SHIELD)
        {
                return -1;
        }
        switch (o_ptr->tval)
        {
        case TV_SWORD:
                if ((!skill) || (skill == SKILL_SWORD)) skill = SKILL_SWORD;
                else skill = -1;
                break;
	case TV_AXE:
		if ((!skill) || (skill == SKILL_AXE)) skill = SKILL_AXE;
		else skill = -1;
		break;
        case TV_BLUNT:
                if ((!skill) || (skill == SKILL_BLUNT)) skill = SKILL_BLUNT;
                else skill = -1;
                break;
//        case SKILL_POLEARM:
        case TV_POLEARM:
                if ((!skill) || (skill == SKILL_POLEARM)) skill = SKILL_POLEARM;
                else skill = -1;
                break;
	}

	/* Everything is ok */
	return skill;
}

/* Are all the ranged weapons wielded of the right type ? */
int get_archery_skill(player_type *p_ptr)
{
	int skill = 0;
	object_type *o_ptr;

	o_ptr = &p_ptr->inventory[INVEN_BOW];

	if (!o_ptr->k_idx) return -1;

	/* Hack -- Boomerang skill */
	if (o_ptr->tval == TV_BOOMERANG) return SKILL_BOOMERANG;

	switch (o_ptr->sval / 10)
	{
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


int calc_blows_obj(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];
	int str_index, dex_index, eff_weight = o_ptr->weight;
	u32b f1;
	artifact_type *a_ptr;

	int num = 0, wgt = 0, mul = 0, div = 0, num_blow = 0, str_adj, xblow = 0;

	/* Weapons which can be wielded 2-handed are easier to swing
	   than with one hand - experimental - C. Blue */
	if ((!p_ptr->inventory[INVEN_ARM].k_idx || /* don't forget dual-wield.. weapon might be in the other hand! */
	    (!p_ptr->inventory[INVEN_WIELD].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) &&
	    (k_info[o_ptr->k_idx].flags4 & (TR4_SHOULD2H | TR4_MUST2H | TR4_COULD2H)))
		eff_weight = (eff_weight * 2) / 3; /* probably more sensible, but..see the line below :/ */
//too easy!	eff_weight = (eff_weight * 1) / 2; /* get 2bpr with 18/30 str even with broad axe starting weapon */

	/* Analyze the class */
	switch (p_ptr->pclass)
	{
							/* Adevnturer */
		case CLASS_ADVENTURER: num = 4; wgt = 35; mul = 5; break;//wgt=35, but MA is too easy in comparison.
//was num = 5; ; mul = 6
							/* Warrior */
		case CLASS_WARRIOR: num = 6; wgt = 30; mul = 5; break;

							/* Mage */
		case CLASS_MAGE: num = 1; wgt = 40; mul = 2; break;
//was num = 3; ; 
							/* Priest */
		case CLASS_PRIEST: num = 4; wgt = 35; mul = 4; break;//mul3
//was num = 5; ; 
							/* Rogue */
		case CLASS_ROGUE: num = 5; wgt = 30; mul = 4; break; /* was mul = 3 - C. Blue - EXPERIMENTAL */

		/* I'm a rogue like :-o */
		case CLASS_RUNEMASTER: num = 4; wgt = 30; mul = 4; break;//was wgt = 40
							/* Mimic */
//trying 5bpr	case CLASS_MIMIC: num = 4; wgt = 30; mul = 4; break;//mul3
		case CLASS_MIMIC: num = 5; wgt = 35; mul = 4; break;//mul3

							/* Archer */
//		case CLASS_ARCHER: num = 3; wgt = 30; mul = 3; break;
		case CLASS_ARCHER: num = 3; wgt = 35; mul = 4; break;

							/* Paladin */
		case CLASS_PALADIN: num = 5; wgt = 35; mul = 5; break;//mul4

							/* Ranger */
		case CLASS_RANGER: num = 5; wgt = 35; mul = 4; break;//mul4


		case CLASS_DRUID: num = 4; wgt = 35; mul = 4; break; 

/* if he is to become a spellcaster, necro working on spell-kills:
		case CLASS_SHAMAN: num = 2; wgt = 40; mul = 3; break;
    however, then Martial Arts would require massive nerfing too for this class (or being removed even).
otherwise, let's compromise for now: */	case CLASS_SHAMAN: num = 4; wgt = 35; mul = 4; break;

		case CLASS_MINDCRAFTER: num = 5; wgt = 35; mul = 4; break;//was 4,30,4

/*		case CLASS_BARD: num = 4; wgt = 35; mul = 4; break; */
	}

	/* Enforce a minimum "weight" (tenth pounds) */
	div = ((eff_weight < wgt) ? wgt : eff_weight);


	/* Access the strength vs weight */
	str_adj = adj_str_blow[p_ptr->stat_ind[A_STR]];
	if (str_adj == 240) str_adj = 266; /* hack to reach 6 bpr with 400 lb weapons (max) at *** STR - C. Blue */
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
	if (get_weaponmastery_skill(p_ptr, o_ptr) != -1)
		num_blow += get_skill_scale(p_ptr, get_weaponmastery_skill(p_ptr, o_ptr), 2);

#if 0
	f1 = k_info[o_ptr->k_idx].flags1;
#else
	u32b f2, f3, f4, f5, esp;
	/* Extract the item flags */
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
#endif
	if (f1 & (TR1_BLOWS)) xblow = o_ptr->bpval;
	if (o_ptr->name2) {
		a_ptr = ego_make(o_ptr);
		f1 &= ~(k_info[o_ptr->k_idx].flags1 & TR1_PVAL_MASK & ~a_ptr->flags1);
	}
	if (o_ptr->name1 == ART_RANDART) {
		a_ptr = randart_make(o_ptr);
		f1 &= ~(k_info[o_ptr->k_idx].flags1 & TR1_PVAL_MASK & ~a_ptr->flags1);
	}
	if ((f1 & TR1_BLOWS)
#if 1 /* this appearently works */
	    && o_ptr->pval > xblow) xblow = o_ptr->pval;
#else /* this is how it's done in xtra1.c though */
      /* this allows shadow blade of ea (+2) (+n) to give +2+n EA */
		) xblow += o_ptr->pval;
#endif
	num_blow += xblow;

	return (num_blow);
}

int calc_blows_weapons(int Ind)
{
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
#else /* round up? (see bpr bonus below too) (encounrages crit gloves/bpr weapons) */
	if (p_ptr->dual_wield && p_ptr->dual_mode) num_blow = (blows1 + blows2 + 1) / 2;
#endif
	else num_blow = blows1;

	/* add dual-wield bonus if we wear light armour! */
	if (!p_ptr->rogue_heavyarmor && p_ptr->dual_wield && p_ptr->dual_mode)
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

int calc_crit_obj(int Ind, object_type *o_ptr)
{
	artifact_type *a_ptr;
	int xcrit = 0;
	u32b f1, f2, f3, f4, f5, esp;
	object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

	if (f5 & (TR5_CRIT)) xcrit = o_ptr->bpval;
	/* check for either.. */
	if (o_ptr->name2) {
		/* additional crits from an ego power, or.. */
		a_ptr = ego_make(o_ptr);
		f5 &= ~(k_info[o_ptr->k_idx].flags5 & TR5_PVAL_MASK & ~a_ptr->flags5);
	}
	if (o_ptr->name1 == ART_RANDART) {
		/* additional crits from a randart power. */
		a_ptr = randart_make(o_ptr);
		f5 &= ~(k_info[o_ptr->k_idx].flags5 & TR5_PVAL_MASK & ~a_ptr->flags5);
	}
	if ((f5 & TR5_CRIT)
#if 0 /* this appearently works */
	    && o_ptr->pval > xcrit) xcrit = o_ptr->pval;
#else /* this is how it's done in xtra1.c though */
		) xcrit += o_ptr->pval; /* no intrinsic +CRIT items in k_info.txt at the moment anyway, so it doesn't matter which way we choose */
#endif
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
void calc_boni(int Ind)
{
	cptr inscription = NULL;
	
//	char tmp[80];

	player_type *p_ptr = Players[Ind];
	dun_level *l_ptr = getfloor(&p_ptr->wpos);
        cave_type **zcave;
        if(!(zcave=getcave(&p_ptr->wpos))) return;

	int			j, hold, minus, am_bonus = 0, am_temp, to_life = 0;
	long			w, i;

	int			old_speed;
	int			old_num_blow;

	u32b			old_telepathy;
	int			old_see_inv;

	int			old_dis_ac;
	int			old_dis_to_a;

	int			old_dis_to_h;
	int			old_dis_to_d;

//	int			extra_blows;
	int			extra_shots;
	int			extra_spells;

	object_type		*o_ptr, *o2_ptr;
	object_kind		*k_ptr;

	long int n, d, toac = 0, body = 0;
	bool wepless = FALSE;
	monster_race *r_ptr = &r_info[p_ptr->body_monster];

	    u32b f1, f2, f3, f4, f5, esp;
		s16b pval;

	bool old_auto_id = p_ptr->auto_id;

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

	/* Clear extra blows/shots */
//	extra_blows = extra_shots = extra_spells = 0;
	extra_shots = extra_spells = 0;

	/* Clear the stat modifiers */
	for (i = 0; i < 6; i++) p_ptr->stat_add[i] = 0;

	/* Druidism bonuses */
	if (p_ptr->xtrastat > 0) {
		p_ptr->stat_add[A_STR] = p_ptr->xstr;
		p_ptr->stat_add[A_INT] = p_ptr->xint;
		p_ptr->stat_add[A_DEX] = p_ptr->xdex;
		p_ptr->stat_add[A_CON] = p_ptr->xcon;
		p_ptr->stat_add[A_CHR] = p_ptr->xchr;
	}
	
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
	p_ptr->resist_plasma = FALSE;
	p_ptr->immune_poison = FALSE;
	p_ptr->immune_water = FALSE;
	p_ptr->resist_water = FALSE;
	p_ptr->regen_mana = FALSE;
	p_ptr->feather_fall = FALSE;
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
	p_ptr->resist_shard = FALSE;
	p_ptr->resist_nexus = FALSE;
	p_ptr->resist_blind = FALSE;
	p_ptr->resist_neth = FALSE;
	p_ptr->resist_fear = FALSE;
	p_ptr->immune_acid = FALSE;
	p_ptr->immune_elec = FALSE;
	p_ptr->immune_fire = FALSE;
	p_ptr->immune_cold = FALSE;
	p_ptr->sh_fire = FALSE;
	p_ptr->sh_elec = FALSE;
	p_ptr->sh_cold = FALSE;
	p_ptr->auto_tunnel = FALSE;
	p_ptr->fly = FALSE;
	p_ptr->can_swim = FALSE;
	p_ptr->climb = FALSE;
	p_ptr->pass_trees = FALSE;
	p_ptr->luck_cur = 0;
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
//	p_ptr->to_s = 0;
	p_ptr->to_m = 0;
	p_ptr->to_l = 0;
	p_ptr->to_hp = 0;
	p_ptr->black_breath_tmp = FALSE;

	p_ptr->dual_wield = FALSE;
	if (p_ptr->inventory[INVEN_WIELD].k_idx &&
	    p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)
		p_ptr->dual_wield = TRUE;

	p_ptr->stormbringer = FALSE;

	/* Invisibility */
	p_ptr->invis = 0;
	if (!p_ptr->tim_invisibility) p_ptr->tim_invis_power = 0;

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


	/* Calc bonus body */
	if(p_ptr->body_monster)
	{
		calc_body_bonus(Ind);

	}
	else	// if if or switch to switch, that is the problem :)
			/* I vote for p_info ;) */
	{
        	/* Update the innate spells */
	        p_ptr->innate_spells[0] = 0x0;
        	p_ptr->innate_spells[1] = 0x0;
	        p_ptr->innate_spells[2] = 0x0;
		Send_spell_info(Ind, 0, 0, 0, "nothing");

		/* Start with "normal" speed */
		p_ptr->pspeed = 110;

#ifdef ARCADE_SERVER
		p_ptr->pspeed = 130;
		if(p_ptr->stun > 0) p_ptr->pspeed -= 3;
#endif

		/* Bats get +10 speed ... they need it!*/
		if (p_ptr->fruit_bat){
			if (p_ptr->fruit_bat == 1)
				p_ptr->pspeed += 10; //disabled due to bat-party-powerlevel-cheezing
//				p_ptr->pspeed += (3 + (p_ptr->lev > 49 ? 7 : p_ptr->lev / 7)); // +10 eventually.
//				p_ptr->pspeed += (3 + (p_ptr->lev > 42 ? 7 : p_ptr->lev / 6)); // +10 eventually.
//				p_ptr->pspeed += (3 + (p_ptr->lev > 35 ? 7 : p_ptr->lev / 5)); // +10 eventually.
			else
				p_ptr->pspeed += 3;
			p_ptr->fly = TRUE;
			p_ptr->feather_fall = TRUE;
		}
	/* Choosing a race just for its HP or low exp% shouldn't be what we want -C. Blue- */
	}

	/* Half-Elf */
	if (p_ptr->prace == RACE_HALF_ELF) {
		p_ptr->resist_lite = TRUE;
	}

	/* Elf */
	else if (p_ptr->prace == RACE_ELF) {
		p_ptr->see_inv = TRUE;
		p_ptr->resist_lite = TRUE;
	}

	/* Hobbit */
	else if (p_ptr->prace == RACE_HOBBIT)
	{
		p_ptr->sustain_dex = TRUE;

		/* DEX bonus for NOT wearing shoes */
		/* not while in mimicried form */
		if(!p_ptr->body_monster)
			if (!p_ptr->inventory[INVEN_FEET].k_idx)
				p_ptr->stat_add[A_DEX] += 2;
	}

	/* Gnome */
	else if (p_ptr->prace == RACE_GNOME) p_ptr->free_act = TRUE;

	/* Dwarf */
	else if (p_ptr->prace == RACE_DWARF)
	{
		p_ptr->resist_blind = TRUE;
		/* not while in mimicried form */
		if(!p_ptr->body_monster)
		    if (p_ptr->lev >= 30) p_ptr->climb = TRUE;
	}

	/* Half-Orc */
	else if (p_ptr->prace == RACE_HALF_ORC) p_ptr->resist_dark = TRUE;

	/* Half-Troll */
	else if (p_ptr->prace == RACE_HALF_TROLL) p_ptr->sustain_str = TRUE;

	/* Dunadan */
	else if (p_ptr->prace == RACE_DUNADAN) p_ptr->sustain_con = TRUE;

	/* High Elf */
	else if (p_ptr->prace == RACE_HIGH_ELF)
	{
		p_ptr->resist_lite = TRUE;
		p_ptr->see_inv = TRUE;
		p_ptr->resist_time = TRUE;
	}

	/* Yeek */
	else if (p_ptr->prace == RACE_YEEK) {
		p_ptr->feather_fall = TRUE;
		/* not while in mimicried form */
		if(!p_ptr->body_monster)
		{
			p_ptr->pass_trees = TRUE;
		}
	}

	/* Goblin */
	else if (p_ptr->prace == RACE_GOBLIN)
	{
		p_ptr->resist_dark = TRUE;
		/* not while in mimicried form */
		/*if(!p_ptr->body_monster) p_ptr->feather_fall = TRUE;*/
	}

	/* Ent */
	else if (p_ptr->prace == RACE_ENT)
	{
		/* always a bit slowish */
		p_ptr->slow_digest = TRUE;
		/* even while in different form? */
		p_ptr->suscep_fire = TRUE;
		p_ptr->resist_water = TRUE;

		/* not while in mimicried form */
		if(!p_ptr->body_monster)
		{
			p_ptr->pspeed -= 2;
			p_ptr->can_swim = TRUE; /* wood? */
			p_ptr->pass_trees = TRUE;
		} else p_ptr->pspeed -= 1; /* it's cost of ent's power, isn't it? */

		if (p_ptr->lev >= 4) p_ptr->see_inv = TRUE;

		if (p_ptr->lev >= 10) p_ptr->telepathy |= ESP_ANIMAL;
		if (p_ptr->lev >= 15) p_ptr->telepathy |= ESP_ORC;
		if (p_ptr->lev >= 20) p_ptr->telepathy |= ESP_TROLL;
		if (p_ptr->lev >= 25) p_ptr->telepathy |= ESP_GIANT;
		if (p_ptr->lev >= 30) p_ptr->telepathy |= ESP_DRAGON;
		if (p_ptr->lev >= 40) p_ptr->telepathy |= ESP_DEMON;
		if (p_ptr->lev >= 50) p_ptr->telepathy |= ESP_EVIL;
	}

	/* Thunderlord (former DragonRider) */
	else if (p_ptr->prace == RACE_DRIDER)
	{
		/* not while in mimicried form */
		if(!p_ptr->body_monster) p_ptr->feather_fall = TRUE;

		if (p_ptr->lev >= 5) p_ptr->telepathy |= ESP_DRAGON;
		if (p_ptr->lev >= 10) p_ptr->resist_fire = TRUE;
		if (p_ptr->lev >= 15) p_ptr->resist_cold = TRUE;
		if (p_ptr->lev >= 20) p_ptr->resist_acid = TRUE;
		if (p_ptr->lev >= 25) p_ptr->resist_elec = TRUE;
		/* not while in mimicried form */
		if (!p_ptr->body_monster)
		    if (p_ptr->lev >= 30) p_ptr->fly = TRUE;
	}

	/* Dark-Elves */
        else if (p_ptr->prace == RACE_DARK_ELF)
        {
		p_ptr->suscep_lite = TRUE;
//		p_ptr->suscep_good = TRUE;//maybe too harsh
                p_ptr->resist_dark = TRUE;
                if (p_ptr->lev >= 20) p_ptr->see_inv = TRUE;
        }

        /* Vampires */
        else if (p_ptr->prace == RACE_VAMPIRE)
        {
		p_ptr->suscep_lite = TRUE;
		p_ptr->suscep_good = TRUE;
		p_ptr->suscep_life = TRUE;

		p_ptr->resist_time = TRUE;
                p_ptr->resist_dark = TRUE;
                p_ptr->resist_neth = TRUE;
                p_ptr->resist_pois = TRUE;
                p_ptr->hold_life = TRUE;

                if (p_ptr->vampiric_melee < 50) p_ptr->vampiric_melee = 50; /* mimic forms give 50 (50% bite attacks) - 33 was actually pretty ok, for lower levels at least */
		/* sense surroundings without light source! (virtual lite / dark light) */
		p_ptr->cur_vlite = 1 + p_ptr->lev / 10;
//		if (p_ptr->lev >= 30) p_ptr->fly = TRUE; can poly into bat instead
        }
#ifdef ENABLE_DIVINE
	else if (p_ptr->prace == RACE_DIVINE) {
//		if (p_ptr->lev<20) {
			//Help em out a little..
			p_ptr->telepathy |= ESP_DEMON;
			p_ptr->telepathy |= ESP_GOOD;
//		}
		if (p_ptr->divinity==DIVINE_ANGEL) {
			p_ptr->cur_lite += 1 + (p_ptr->lev-20) / 4; //REAL light!
			if (p_ptr->lev>=20) {
				p_ptr->resist_lite = TRUE;
				p_ptr->to_a += (p_ptr->lev-20);
				p_ptr->dis_to_a += (p_ptr->lev-20);
			}
			if (p_ptr->lev>=50) {
				p_ptr->resist_pois = TRUE;
				p_ptr->sh_elec = TRUE;
				p_ptr->sh_cold = TRUE;
				p_ptr->fly = TRUE;
			}
		} else if (p_ptr->divinity==DIVINE_DEMON) {
			if (p_ptr->lev>=20) {
				p_ptr->resist_fire = TRUE;
				p_ptr->resist_dark = TRUE;
			}
			if (p_ptr->lev>=50) {
				p_ptr->immune_fire = TRUE; 
				p_ptr->resist_pois = TRUE;
				p_ptr->sh_fire = TRUE;
			}
		} 
	}
#endif


	if (p_ptr->pclass == CLASS_RANGER)
		if (p_ptr->lev >= 20) p_ptr->pass_trees = TRUE;

	if (p_ptr->pclass == CLASS_SHAMAN)
		if (p_ptr->lev >= 20) p_ptr->see_inv = TRUE;
	
	if (p_ptr->pclass == CLASS_DRUID)
		if (p_ptr->lev >= 10) p_ptr->pass_trees = TRUE;

	if (p_ptr->pclass == CLASS_MINDCRAFTER) {
		if (p_ptr->lev >= 20) p_ptr->reduce_insanity = 2;
		else if (p_ptr->lev >= 10) p_ptr->reduce_insanity = 1;
	}

	/* Check ability skills */
	if (get_skill(p_ptr, SKILL_CLIMB) >= 1) p_ptr->climb = TRUE;
	if (get_skill(p_ptr, SKILL_FLY) >= 1) p_ptr->fly = TRUE;
	if (get_skill(p_ptr, SKILL_FREEACT) >= 1) p_ptr->free_act = TRUE;
	if (get_skill(p_ptr, SKILL_RESCONF) >= 1) p_ptr->resist_conf = TRUE;

	/* Compute antimagic */
	if (get_skill(p_ptr, SKILL_ANTIMAGIC))
	{
//		p_ptr->anti_magic = TRUE;	/* it means 95% saving-throw!! */
#ifdef NEW_ANTIMAGIC_RATIO
		p_ptr->antimagic += get_skill_scale(p_ptr, SKILL_ANTIMAGIC, 50);
		p_ptr->antimagic_dis += 1 + (get_skill(p_ptr, SKILL_ANTIMAGIC) / 10); /* was /11, but let's reward max skill! */
#else
		p_ptr->antimagic += get_skill_scale(p_ptr, SKILL_ANTIMAGIC, 30);
		p_ptr->antimagic_dis += 1 + (get_skill(p_ptr, SKILL_ANTIMAGIC) / 10); /* was /11, but let's reward max skill! */
#endif
	}

	/* Ghost */
	if (p_ptr->ghost)
	{
		p_ptr->see_inv = TRUE;
		/* p_ptr->resist_neth = TRUE;
		p_ptr->hold_life = TRUE; */
		p_ptr->free_act = TRUE;
		p_ptr->see_infra += 3;
		p_ptr->resist_dark = TRUE;
		p_ptr->resist_blind = TRUE;
		p_ptr->immune_poison = TRUE;
		p_ptr->resist_cold = TRUE;
		p_ptr->resist_fear = TRUE;
		p_ptr->resist_conf = TRUE;
		p_ptr->no_cut = TRUE;
		p_ptr->reduce_insanity = 1;
		p_ptr->fly = TRUE; /* redundant */
		p_ptr->feather_fall = TRUE;
		/*p_ptr->tim_wraith = 30000; redundant*/
//		p_ptr->invis += 5; */ /* No. */
	}


	/* Hack -- apply racial/class stat maxes */
	if (p_ptr->maximize)
	{
		/* Apply the racial modifiers */
		for (i = 0; i < 6; i++)
		{
			/* Modify the stats for "race" */
			/* yeek mimic no longer rocks too much */
//			if (!p_ptr->body_monster) p_ptr->stat_add[i] += (p_ptr->rp_ptr->r_adj[i]);
			p_ptr->stat_add[i] += (p_ptr->rp_ptr->r_adj[i]) * 3 / (p_ptr->body_monster ? 4 : 3);
			p_ptr->stat_add[i] += (p_ptr->cp_ptr->c_adj[i]);
		}
	}

       	/* Apply the racial modifiers */
	if (p_ptr->mode & MODE_HARD)
	{
		for (i = 0; i < 6; i++)
		{
			/* Modify the stats for "race" */
			p_ptr->stat_add[i]--;
		}
	}
	

	/* Hack -- the dungeon master gets +50 speed. */
	if (p_ptr->admin_dm) 
	{
		p_ptr->pspeed += 50;
		p_ptr->telepathy |= ESP_ALL;
	}

	/* Hack -- recalculate inventory weight and count */
	p_ptr->total_weight = 0;
	p_ptr->inven_cnt = 0;

	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &p_ptr->inventory[i];

		/* Skip missing items */
		if (!o_ptr->k_idx) break;

		p_ptr->inven_cnt++;
		p_ptr->total_weight += o_ptr->weight * o_ptr->number;
	}
    
	/* Apply the bonus from Druidism */
	p_ptr->to_h += p_ptr->focus_val;
	p_ptr->dis_to_h += p_ptr->focus_val;

	p_ptr->to_h_ranged += p_ptr->focus_val; 
	p_ptr->dis_to_h_ranged += p_ptr->focus_val; 

	/* Scan the usable inventory */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		o_ptr = &p_ptr->inventory[i];
		k_ptr = &k_info[o_ptr->k_idx];
		pval = o_ptr->pval;

		/* Skip missing items */
		if (!o_ptr->k_idx) continue;

		p_ptr->total_weight += o_ptr->weight * o_ptr->number;

		/* Extract the item flags */
		object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

		/* Not-burning light source does nothing, good or bad */
		if ((f4 & TR4_FUEL_LITE) && (o_ptr->timeout < 1)) continue;

		/* MEGA ugly hack -- set spacetime distortion resistance */
		if (o_ptr->name1 == ART_ANCHOR) {
			p_ptr->resist_continuum = TRUE;
		}
		/* another bad hack, for when Morgoth's crown gets its pval
		   reduced experimentally, to keep massive +IV (for Q-quests^^): */
		if (o_ptr->name1 == ART_MORGOTH) {
			p_ptr->see_infra += 50;
		}

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
		if (k_ptr->flags1 & TR1_PVAL_MASK)
		{
			/* Affect stats */
			if (k_ptr->flags1 & TR1_STR) p_ptr->stat_add[A_STR] += o_ptr->bpval;
			if (k_ptr->flags1 & TR1_INT) p_ptr->stat_add[A_INT] += o_ptr->bpval;
			if (k_ptr->flags1 & TR1_WIS) p_ptr->stat_add[A_WIS] += o_ptr->bpval;
			if (k_ptr->flags1 & TR1_DEX) p_ptr->stat_add[A_DEX] += o_ptr->bpval;
			if (k_ptr->flags1 & TR1_CON) p_ptr->stat_add[A_CON] += o_ptr->bpval;
			if (k_ptr->flags1 & TR1_CHR) p_ptr->stat_add[A_CHR] += o_ptr->bpval;

			/* Affect stealth */
			if (k_ptr->flags1 & TR1_STEALTH) p_ptr->skill_stl += o_ptr->bpval;

			/* Affect searching ability (factor of five) */
			if (k_ptr->flags1 & TR1_SEARCH) p_ptr->skill_srh += (o_ptr->bpval * 5);

			/* Affect searching frequency (factor of five) */
			if (k_ptr->flags1 & TR1_SEARCH) p_ptr->skill_fos += (o_ptr->bpval * 3);

			/* Affect infravision */
			if (k_ptr->flags1 & TR1_INFRA) p_ptr->see_infra += o_ptr->bpval;

			/* Affect digging (factor of 20) */
			if (k_ptr->flags1 & TR1_TUNNEL) p_ptr->skill_dig += (o_ptr->bpval * 20);

			/* Affect speed */
			if (k_ptr->flags1 & TR1_SPEED) p_ptr->pspeed += o_ptr->bpval;

			/* Affect blows */
#if 1 /* for dual-wield this is too much, it's done in calc_blows_obj() now */
/* There are no known weapons so far that adds bpr intrinsically. Need this for e.g., ring of EA */
			if (k_ptr->flags1 & TR1_BLOWS) p_ptr->extra_blows += o_ptr->bpval;
#endif
			/* Affect spells */
			if (k_ptr->flags1 & TR1_SPELL) extra_spells += o_ptr->bpval;
//			if (k_ptr->flags1 & TR1_SPELL_SPEED) extra_spells += o_ptr->bpval;

	                /* Affect mana capacity */
    	        	if (f1 & (TR1_MANA)) {
				if ((f4 & TR4_SHOULD2H) &&
				    (p_ptr->inventory[INVEN_WIELD].k_idx && p_ptr->inventory[INVEN_ARM].k_idx))
					p_ptr->to_m += (o_ptr->bpval * 20) / 3;
				else
					p_ptr->to_m += o_ptr->bpval * 10;
			}

    	        	/* Affect life capacity */
        	        if (f1 & (TR1_LIFE))
				if ((o_ptr->name1 != ART_RANDART) || p_ptr->total_winner ||
#if 0 /* changed, didn't seem to make that much sense? - C. Blue */
				    (p_ptr->once_winner && cfg.fallenkings_etiquette && p_ptr->lev >= 50) ||
				    (p_ptr->lev >= o_ptr->level))
#else /* a bit different, hopefully doing better, also catching badly mutated +life arts after randart.c changes! */
				    (p_ptr->once_winner && cfg.fallenkings_etiquette))
#endif
					p_ptr->to_l += o_ptr->bpval;

		}

		if (k_ptr->flags5 & TR5_PVAL_MASK)
		{
			if (f5 & TR5_DISARM) p_ptr->skill_dis += (o_ptr->bpval) * 5;
			if (f5 & TR5_LUCK) p_ptr->luck_cur += o_ptr->bpval;
#if 1 /* this is too much for dual-wield, done in calc_blows_obj() now */
/* There are no known weapons so far that adds to crit intrinsically. */
			if (f5 & TR5_CRIT) p_ptr->xtra_crit += o_ptr->bpval;
#endif
		}

		/* bad hack, sorry. need redesign of boni - C. Blue
		   fixes the vampiric shadow blade bug / affects all +life +basestealth weapons:
		   (+2)(-2stl) -> life remains unchanged, stealth is increased by 2:
		   both mods affect the life! Correct mod affects the stealth, but it's displayed wrong.
		   pval should contain life bonus, bpval stealth. */
/*		if (o_ptr->name2 == EGO_VAMPIRIC || o_ptr->name2b == EGO_VAMPIRIC)*/
		if (f1 & (TR1_LIFE))
			if ((o_ptr->name1 != ART_RANDART) || p_ptr->total_winner ||
#if 0 /* changed, didn't seem to make that much sense? - C. Blue */
			    (p_ptr->once_winner && cfg.fallenkings_etiquette && p_ptr->lev >= 50) ||
			    (p_ptr->lev >= o_ptr->level))
#else /* a bit different, hopefully doing better, also catching badly mutated +life arts after randart.c changes! */
			    (p_ptr->once_winner && cfg.fallenkings_etiquette))
#endif
		{
/*			if ((o_ptr->pval < 0 && o_ptr->bpval > 0) ||
			    (o_ptr->pval > 0 && o_ptr->bpval < 0)) {*/
			if (o_ptr->pval != 0 && o_ptr->bpval != 0) {
				/* first we remove both effects on +LIFE */
				p_ptr->to_l -= o_ptr->pval + o_ptr->bpval;
				/* then we add the correct one, which is the wrong one ;) */
/*				if (o_ptr->pval < 0) p_ptr->to_l += o_ptr->pval;
				if (o_ptr->bpval < 0) p_ptr->to_l += o_ptr->bpval;*/
				p_ptr->to_l += o_ptr->pval;
			}
		}

		/* Next, add our ego bonuses */
		/* Hack -- clear out any pval bonuses that are in the base item
		 * bonus but not the ego bonus so we don't add them twice.
		*/
#if 1
//		if (o_ptr->name2 && o_ptr->tval!=TV_RING) // pls see apply_magic ;)
		if (o_ptr->name2)
		{
			artifact_type *a_ptr;
	 	
			a_ptr =	ego_make(o_ptr);
			f1 &= ~(k_ptr->flags1 & TR1_PVAL_MASK & ~a_ptr->flags1);
			f5 &= ~(k_ptr->flags5 & TR5_PVAL_MASK & ~a_ptr->flags5);

			/* Hack: Stormbringer! */
			if (o_ptr->name2 == EGO_STORMBRINGER)
			{
				p_ptr->stormbringer = TRUE;
				break_cloaking(Ind, 0);
				break_shadow_running(Ind);
				if (cfg.use_pk_rules == PK_RULES_DECLARE)
				{
					p_ptr->pkill|=PKILL_KILLABLE;
					if (!(p_ptr->pkill & PKILL_KILLER) &&
							!(p_ptr->pkill & PKILL_SET))
						set_pkill(Ind, 50);
				}
			}
		}

		if (o_ptr->name1 == ART_RANDART)
		{
			artifact_type *a_ptr;
	 	
			a_ptr =	randart_make(o_ptr);
			f1 &= ~(k_ptr->flags1 & TR1_PVAL_MASK & ~a_ptr->flags1);
			f5 &= ~(k_ptr->flags5 & TR5_PVAL_MASK & ~a_ptr->flags5);
		}
#endif


		/* Affect stats */
		if (f1 & TR1_STR) p_ptr->stat_add[A_STR] += pval;
		if (f1 & TR1_INT) p_ptr->stat_add[A_INT] += pval;
		if (f1 & TR1_WIS) p_ptr->stat_add[A_WIS] += pval;
		if (f1 & TR1_DEX) p_ptr->stat_add[A_DEX] += pval;
		if (f1 & TR1_CON) p_ptr->stat_add[A_CON] += pval;
		if (f1 & TR1_CHR) p_ptr->stat_add[A_CHR] += pval;

                /* Affect luck */
                if (f5 & (TR5_LUCK)) p_ptr->luck_cur += pval;

                /* Affect spell power */
//                if (f1 & (TR1_SPELL)) p_ptr->to_s += pval;

                /* Affect mana capacity */
                if (f1 & (TR1_MANA)) {
			if ((f4 & TR4_SHOULD2H) &&
			    (p_ptr->inventory[INVEN_WIELD].k_idx && p_ptr->inventory[INVEN_ARM].k_idx))
				p_ptr->to_m += (pval * 20) / 3;
			else
            			p_ptr->to_m += pval * 10;
		}

                /* Affect life capacity */
                if (f1 & (TR1_LIFE))
			if ((o_ptr->name1 != ART_RANDART) || p_ptr->total_winner ||
#if 0 /* changed, didn't seem to make that much sense? - C. Blue */
			    (p_ptr->once_winner && cfg.fallenkings_etiquette && p_ptr->lev >= 50) ||
			    (p_ptr->lev >= o_ptr->level))
#else /* a bit different, hopefully doing better, also catching badly mutated +life arts after randart.c changes! */
			    (p_ptr->once_winner && cfg.fallenkings_etiquette))
#endif
				p_ptr->to_l += o_ptr->pval;

		/* Affect stealth */
		if (f1 & TR1_STEALTH) p_ptr->skill_stl += pval;

		/* Affect searching ability (factor of five) */
		if (f1 & TR1_SEARCH) p_ptr->skill_srh += (pval * 5);

		/* Affect searching frequency (factor of five) */
		if (f1 & TR1_SEARCH) p_ptr->skill_fos += (pval * 3);

		/* Affect infravision */
		if (f1 & TR1_INFRA) p_ptr->see_infra += pval;

		/* Affect digging (factor of 20) */
		if (f1 & TR1_TUNNEL) p_ptr->skill_dig += (pval * 20);

		/* Affect speed */
		if (f1 & TR1_SPEED) p_ptr->pspeed += pval;

		/* Affect blows */
#if 0 /* for dual-wield this is too much, it's done in calc_blows_obj() now */
		if (f1 & TR1_BLOWS) p_ptr->extra_blows += pval;
                if (f5 & TR5_CRIT) p_ptr->xtra_crit += pval;
#endif

		/* Affect spellss */
//		if (f1 & TR1_SPELL_SPEED) extra_spells += pval;
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
//		if (f3 & TR3_KNOWLEDGE) p_ptr->auto_id = TRUE;
		if (f4 & TR4_AUTO_ID) p_ptr->auto_id = TRUE;

		/* Boost shots */
		if (f3 & TR3_XTRA_SHOTS) extra_shots++;

		/* Make the 'useless' curses interesting >:) - C. Blue */
		if ((f3 & TR3_TY_CURSE) && (o_ptr->ident & ID_CURSED)) p_ptr->ty_curse = TRUE;
		if ((f4 & TR4_DG_CURSE) && (o_ptr->ident & ID_CURSED)) p_ptr->dg_curse = TRUE;

		/* Various flags */
		if (f3 & TR3_AGGRAVATE) {
			p_ptr->aggravate = TRUE;
			break_cloaking(Ind, 0);
		}
//		if (f3 & TR3_TELEPORT) p_ptr->teleport = TRUE;
		if (f3 & TR3_TELEPORT){
			p_ptr->teleport = TRUE;
			//inscription = (unsigned char *) quark_str(o_ptr->note);
			inscription = quark_str(p_ptr->inventory[i].note);
			/* check for a valid inscription */
			if ((inscription != NULL) && (!(o_ptr->ident & ID_CURSED)))
			{
				/* scan the inscription for .. */
				while (*inscription != '\0')
				{
					if (*inscription == '.')
					{
						inscription++;
						/* a valid .. has been located */
						if (*inscription == '.')
						{
							inscription++;
							p_ptr->teleport = FALSE;
							break;
						}
					}
					inscription++;
				}
			}
		}
		if (f3 & TR3_DRAIN_EXP) p_ptr->drain_exp++;
                if (f5 & (TR5_DRAIN_MANA)) p_ptr->drain_mana++;
                if (f5 & (TR5_DRAIN_HP)) p_ptr->drain_life++;
		if (f5 & (TR5_INVIS)){
//			p_ptr->tim_invisibility = 100;
			p_ptr->tim_invis_power = p_ptr->lev * 4 / 5;
//			p_ptr->invis = p_ptr->tim_invis_power;
		}
		if (f3 & TR3_BLESSED) p_ptr->bless_blade = TRUE;
		if (f3 & TR3_XTRA_MIGHT) p_ptr->xtra_might++;
		if (f3 & TR3_SLOW_DIGEST) p_ptr->slow_digest = TRUE;
		if (f3 & TR3_REGEN) p_ptr->regenerate = TRUE;
		if (f5 & TR5_RES_TIME) p_ptr->resist_time = TRUE;
		if (f5 & TR5_RES_MANA) p_ptr->resist_mana = TRUE;
		if (f5 & TR5_IM_POISON) p_ptr->immune_poison = TRUE;
		if (f5 & TR5_IM_WATER) p_ptr->immune_water = TRUE;
		if (f5 & TR5_RES_WATER) p_ptr->resist_water = TRUE;
		if (f5 & TR5_PASS_WATER) p_ptr->can_swim = TRUE;
		if (f5 & TR5_REGEN_MANA) p_ptr->regen_mana = TRUE;
                if (esp) p_ptr->telepathy |= esp;
//		if (f3 & TR3_TELEPATHY) p_ptr->telepathy = TRUE;
//		if (f3 & TR3_LITE1) p_ptr->lite += 1;
		if (f3 & TR3_SEE_INVIS) p_ptr->see_inv = TRUE;
		if (f3 & TR3_FEATHER) p_ptr->feather_fall = TRUE;
		if (f2 & TR2_FREE_ACT) p_ptr->free_act = TRUE;
		if (f2 & TR2_HOLD_LIFE) p_ptr->hold_life = TRUE;

		/* Light(consider doing it on calc_torch) */
//		if (((f4 & TR4_FUEL_LITE) && (o_ptr->timeout > 0)) || (!(f4 & TR4_FUEL_LITE)))
		{
			j = 0;
			if (f3 & TR3_LITE1) j++;
			if (f4 & TR4_LITE2) j += 2;
			if (f4 & TR4_LITE3) j += 3;

			p_ptr->cur_lite += j;
			if (j && !(f4 & TR4_FUEL_LITE)) p_ptr->lite = TRUE;
		}

		/* powerful lights and anti-undead/evil items damage vampires */
#if 0
		if (p_ptr->prace == RACE_VAMPIRE && !cursed_p(o_ptr)) {
			if (j) { /* light sources, or other items that provide light */
				if ((j > 2) || o_ptr->name1 || (f3 & TR3_BLESSED) ||
				    (f1 & TR1_SLAY_EVIL) || (f1 & TR1_SLAY_UNDEAD) || (f1 & TR1_KILL_UNDEAD))
					p_ptr->drain_life++;
			} else {
				if ((f3 & TR3_BLESSED) || (f1 & TR1_KILL_UNDEAD))
					p_ptr->drain_life++;
			}
		}
#else
		if (p_ptr->prace == RACE_VAMPIRE && anti_undead(o_ptr)) p_ptr->drain_life++;
#endif

		/* Immunity flags */
		if (f2 & TR2_IM_FIRE) p_ptr->immune_fire = TRUE;
		if (f2 & TR2_IM_ACID) p_ptr->immune_acid = TRUE;
		if (f2 & TR2_IM_COLD) p_ptr->immune_cold = TRUE;
		if (f2 & TR2_IM_ELEC) p_ptr->immune_elec = TRUE;

                if (f2 & TR2_REDUC_FIRE) p_ptr->reduc_fire += 5 * o_ptr->to_a;
                if (f2 & TR2_REDUC_COLD) p_ptr->reduc_cold += 5 * o_ptr->to_a;
                if (f2 & TR2_REDUC_ELEC) p_ptr->reduc_elec += 5 * o_ptr->to_a;
                if (f2 & TR2_REDUC_ACID) p_ptr->reduc_acid += 5 * o_ptr->to_a;

		/* Resistance flags */
		if (f2 & TR2_RES_ACID) p_ptr->resist_acid = TRUE;
		if (f2 & TR2_RES_ELEC) p_ptr->resist_elec = TRUE;
		if (f2 & TR2_RES_FIRE) p_ptr->resist_fire = TRUE;
		if (f2 & TR2_RES_COLD) p_ptr->resist_cold = TRUE;
		if (f2 & TR2_RES_POIS) p_ptr->resist_pois = TRUE;
		if (f2 & TR2_RES_FEAR) p_ptr->resist_fear = TRUE;
		if (f2 & TR2_RES_CONF) p_ptr->resist_conf = TRUE;
		if (f2 & TR2_RES_SOUND) p_ptr->resist_sound = TRUE;
		if (f2 & TR2_RES_LITE) p_ptr->resist_lite = TRUE;
		if (f2 & TR2_RES_DARK) p_ptr->resist_dark = TRUE;
		if (f2 & TR2_RES_CHAOS) p_ptr->resist_chaos = TRUE;
		if (f2 & TR2_RES_DISEN) p_ptr->resist_disen = TRUE;
		if (f2 & TR2_RES_SHARDS) p_ptr->resist_shard = TRUE;
		if (f2 & TR2_RES_NEXUS) p_ptr->resist_nexus = TRUE;
		if (f5 & TR5_RES_TELE) p_ptr->res_tele = TRUE;
		if (f2 & TR2_RES_BLIND) p_ptr->resist_blind = TRUE;
		if (f2 & TR2_RES_NETHER) p_ptr->resist_neth = TRUE;
//		if (f2 & TR2_ANTI_MAGIC) p_ptr->anti_magic = TRUE;

		/* Sustain flags */
		if (f2 & TR2_SUST_STR) p_ptr->sustain_str = TRUE;
		if (f2 & TR2_SUST_INT) p_ptr->sustain_int = TRUE;
		if (f2 & TR2_SUST_WIS) p_ptr->sustain_wis = TRUE;
		if (f2 & TR2_SUST_DEX) p_ptr->sustain_dex = TRUE;
		if (f2 & TR2_SUST_CON) p_ptr->sustain_con = TRUE;
		if (f2 & TR2_SUST_CHR) p_ptr->sustain_chr = TRUE;

		/* PernA flags */
                if (f3 & (TR3_WRAITH))
		{
			//p_ptr->wraith_form = TRUE;
			if (!(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK) &&
			    !(p_ptr->wpos.wz && (l_ptr->flags1 & LF1_NO_MAGIC)))
			{
//BAD!(recursion)				set_tim_wraith(Ind, 30000);
				p_ptr->tim_wraith = 30000;
			}
		}
                if (f4 & (TR4_FLY)) p_ptr->fly = TRUE;
                if (f4 & (TR4_CLIMB)) {
			/* hack: climbing kit is made for humanoids, so it won't work for other forms! */
			if (o_ptr->tval != TV_TOOL || o_ptr->sval != SV_TOOL_CLIMB || !p_ptr->body_monster)
				p_ptr->climb = TRUE; /* item isn't a climbing kit or player is in normal @ form */
#if 0
			else if ((r_ptr->flags3 & (RF3_ORC | RF3_TROLL | RF3_DRAGONRIDER)) || (strchr("hkptyuVsLW", r_ptr->d_char)))
				p_ptr->climb = TRUE; /* only for normal-sized humanoids (well..trolls, would be unfair to half-trolls) */
#else
			else if (r_ptr->body_parts[BODY_FINGER] && r_ptr->body_parts[BODY_ARMS])
				p_ptr->climb = TRUE; /* everyone with arms and fingers may use it. ok? */
#endif
		}
                if (f4 & (TR4_IM_NETHER)) p_ptr->immune_neth = TRUE;
		if (f5 & (TR5_REFLECT))
			if (o_ptr->tval != TV_SHIELD || !p_ptr->heavy_shield)
				p_ptr->reflect = TRUE;
		if (f3 & (TR3_SH_FIRE)) p_ptr->sh_fire = TRUE;
		if (f5 & (TR5_SH_COLD)) p_ptr->sh_cold = TRUE;
		if (f3 & (TR3_SH_ELEC)) p_ptr->sh_elec = TRUE;
		if (f3 & (TR3_NO_MAGIC)) p_ptr->anti_magic = TRUE;
		if (f3 & (TR3_NO_TELE)) p_ptr->anti_tele = TRUE;

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
		if (am_temp > am_bonus) am_bonus = am_temp;

		if (f4 & (TR4_BLACK_BREATH)) p_ptr->black_breath_tmp = TRUE;

//		if (f5 & (TR5_IMMOVABLE)) p_ptr->immovable = TRUE;

		/* note: TR1_VAMPIRIC isn't directly applied for melee
		   weapons here, but instead in the py_attack... functions
		   in cmd1.c. Reason is that dual-wielding might hit with
		   a vampiric or a non-vampiric weapon randomly - C. Blue */



		/* Modify the base armor class */
#ifdef USE_NEW_SHIELDS
		if (o_ptr->tval != TV_SHIELD)
#else
		if (o_ptr->tval == TV_SHIELD && p_ptr->heavy_shield)
			p_ptr->ac += o_ptr->ac / 2;
		else
#endif
		p_ptr->ac += o_ptr->ac;

		/* The base armor class is always known */
#ifdef USE_NEW_SHIELDS
		if (o_ptr->tval != TV_SHIELD)
#else
		if (o_ptr->tval == TV_SHIELD && p_ptr->heavy_shield)
			p_ptr->dis_ac += o_ptr->ac / 2;
		else
#endif
		p_ptr->dis_ac += o_ptr->ac;

		/* Apply the bonuses to armor class */
		if (o_ptr->tval == TV_SHIELD && p_ptr->heavy_shield)
			p_ptr->to_a += o_ptr->to_a / 2;
		else
			p_ptr->to_a += o_ptr->to_a;

		/* Apply the mental bonuses to armor class, if known */
		if (object_known_p(Ind, o_ptr)) {
			if (o_ptr->tval == TV_SHIELD && p_ptr->heavy_shield)
				p_ptr->dis_to_a += o_ptr->to_a / 2;
			else
				p_ptr->dis_to_a += o_ptr->to_a;
		}



		/* Hack -- do not apply "weapon", "bow", "ammo", or "tool"  boni */
		if (i == INVEN_WIELD) continue;
		if (i == INVEN_ARM && o_ptr->tval != TV_SHIELD) continue; /*..and for dual-wield */
		if (i == INVEN_AMMO || i == INVEN_BOW) {
			if (f1 & TR1_VAMPIRIC) p_ptr->vampiric_ranged = 100;
			continue;
		}
		if (i == INVEN_TOOL) continue;



		if (f1 & TR1_BLOWS) p_ptr->extra_blows += pval;
                if (f5 & TR5_CRIT) p_ptr->xtra_crit += pval;

		/* Hack -- cause earthquakes */
		if (f5 & TR5_IMPACT) p_ptr->impact = TRUE;

		/* Generally vampiric? */
		if (f1 & TR1_VAMPIRIC) {
			if (p_ptr->vampiric_melee < NON_WEAPON_VAMPIRIC_CHANCE) p_ptr->vampiric_melee = NON_WEAPON_VAMPIRIC_CHANCE;
			if (p_ptr->vampiric_ranged < NON_WEAPON_VAMPIRIC_CHANCE_RANGED) p_ptr->vampiric_ranged = NON_WEAPON_VAMPIRIC_CHANCE_RANGED;
		}

		/* Hack MHDSM: */
		if (o_ptr->tval == TV_DRAG_ARMOR && o_ptr->sval == SV_DRAGON_MULTIHUED) {
			if (o_ptr->xtra2 & 0x01) p_ptr->immune_fire = TRUE;
			if (o_ptr->xtra2 & 0x02) p_ptr->immune_cold = TRUE;
			if (o_ptr->xtra2 & 0x04) p_ptr->immune_elec = TRUE;
			if (o_ptr->xtra2 & 0x08) p_ptr->immune_acid = TRUE;
			if (o_ptr->xtra2 & 0x10) p_ptr->immune_poison = TRUE;
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
	
	/* calculate higher to-life bonus from items / skills (non-stacking!) */
	if (p_ptr->to_l < 0) p_ptr->to_l += to_life;
	else if (p_ptr->to_l < to_life) p_ptr->to_l = to_life;

	/* Hard/Hellish mode also gives mana penalty */
	if (p_ptr->mode & MODE_HARD) p_ptr->to_m = (p_ptr->to_m * 2) / 3;

	/* Check for temporary blessings */
	if (p_ptr->bless_temp_luck) p_ptr->luck_cur += p_ptr->bless_temp_luck_power;

	if (p_ptr->antimagic > ANTIMAGIC_CAP) p_ptr->antimagic = ANTIMAGIC_CAP; /* AM cap */
	if (p_ptr->luck_cur < -10) p_ptr->luck_cur = -10; /* luck caps at -10 */
	if (p_ptr->luck_cur > 40) p_ptr->luck_cur = 40; /* luck caps at 40 */

	p_ptr->sun_burn = FALSE;
	if ((p_ptr->prace == RACE_VAMPIRE ||
	    (p_ptr->body_monster && r_info[p_ptr->body_monster].d_char == 'V'))
	    && !p_ptr->ghost) {
		/* damage from sun light */
		if (!p_ptr->wpos.wz && !night_surface && //!(zcave[p_ptr->py][p_ptr->px].info & CAVE_ICKY) &&
		    !p_ptr->resist_lite && (TOOL_EQUIPPED(p_ptr) != SV_TOOL_WRAPPING) &&
//		    !(p_ptr->inventory[INVEN_NECK].k_idx && p_ptr->inventory[INVEN_NECK].sval == SV_AMULET_HIGHLANDS2) &&
		    !(zcave[p_ptr->py][p_ptr->px].info & CAVE_PROT) &&
		    !(f_info[zcave[p_ptr->py][p_ptr->px].feat].flags1 & FF1_PROTECTED)) {
			p_ptr->drain_life++;
			p_ptr->sun_burn = TRUE;
		}
	}

	/* Calculate stats */
	for (i = 0; i < 6; i++)
	{
		int top, use, ind;


		/* Extract the new "stat_use" value for the stat */
		top = modify_stat_value(p_ptr->stat_max[i], p_ptr->stat_add[i]);

		/* Notice changes */
		if (p_ptr->stat_top[i] != top)
		{
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
		if (p_ptr->stat_use[i] != use)
		{
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
		else if (use <= 18+219) ind = (15 + (use - 18) / 10);

		/* Range: 18/220+ */
		else ind = (37);

		/* Notice changes */
		if (p_ptr->stat_ind[i] != ind)
		{
			/* Save the new index */
			p_ptr->stat_ind[i] = ind;

			/* Change in CON affects Hitpoints */
			if (i == A_CON)
			{
				p_ptr->update |= (PU_HP);
			}

			/* Change in INT may affect Mana/Spells */
			else if (i == A_INT)
			{
                                p_ptr->update |= (PU_MANA);
			}

			/* Change in WIS may affect Mana/Spells */
			else if (i == A_WIS)
			{
                                p_ptr->update |= (PU_MANA);
			}

			/* Window stuff */
			p_ptr->window |= (PW_PLAYER);
		}
	}


	/* Apply temporary "stun" */
	if (p_ptr->stun > 50)
	{
#if 0
		p_ptr->to_h -= 20;
		p_ptr->dis_to_h -= 20;
		p_ptr->to_d -= 20;
		p_ptr->dis_to_d -= 20;
#else
		p_ptr->to_h /= 2;
		p_ptr->dis_to_h /= 2;
		p_ptr->to_d /= 2;
		p_ptr->dis_to_d /= 2;
#endif
	}
	else if (p_ptr->stun)
	{
#if 0
		p_ptr->to_h -= 5;
		p_ptr->dis_to_h -= 5;
		p_ptr->to_d -= 5;
		p_ptr->dis_to_d -= 5;
#else
		p_ptr->to_h = (p_ptr->to_h * 2) / 3;
		p_ptr->dis_to_h = (p_ptr->dis_to_h * 2) / 3;
		p_ptr->to_d = (p_ptr->to_d * 2) / 3;
		p_ptr->dis_to_d = (p_ptr->dis_to_h * 2) / 3;
#endif
	}


        /* Adrenaline effects */
        if (p_ptr->adrenaline)
	{
	        int i;

		i = p_ptr->lev / 7;
		if (i > 5) i = 5;
		p_ptr->stat_add[A_CON] += i;
		p_ptr->stat_add[A_STR] += i;
		p_ptr->stat_add[A_DEX] += (i + 1) / 2;
		p_ptr->to_h += 12;
		p_ptr->dis_to_h += 12;
		if (p_ptr->adrenaline & 1)
		{
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

	/* Invulnerability */
	if (p_ptr->invuln)
	{
		p_ptr->to_a += 100;
		p_ptr->dis_to_a += 100;
	}

	/* Temporary "Levitation" */
	if (p_ptr->tim_ffall)
	{
		p_ptr->feather_fall = TRUE;
	}
	if (p_ptr->tim_fly)
	{
		p_ptr->fly = TRUE;
	}

	/* Temp ESP */
	if (p_ptr->tim_esp)
	{
//		p_ptr->telepathy = TRUE;
                p_ptr->telepathy |= ESP_ALL;
	}

	/* Temporary blessing */
	if (p_ptr->blessed)
	{
		p_ptr->to_a += p_ptr->blessed_power;
		p_ptr->dis_to_a += p_ptr->blessed_power;
		p_ptr->to_h += p_ptr->blessed_power / 2;
		p_ptr->dis_to_h += p_ptr->blessed_power / 2;
	}

	/* Temprory invisibility */
//	if (p_ptr->tim_invisibility)
	{
		p_ptr->invis = p_ptr->tim_invis_power;
	}

	/* Temprory shield */
	if (p_ptr->shield)
	{
		p_ptr->to_a += p_ptr->shield_power;
		p_ptr->dis_to_a += p_ptr->shield_power;
	}
	
	/* Temporary deflection */
	if(p_ptr->tim_deflect)
	{
		p_ptr->reflect = TRUE;
	}
	
	/* Temporary "Hero" */
	if (p_ptr->hero || (p_ptr->mindboost && p_ptr->mindboost_power >= 5))
	{
		p_ptr->to_h += 12;
		p_ptr->dis_to_h += 12;
	}

	/* Temporary "Berserk" */
	if (p_ptr->shero)
	{
		p_ptr->to_h += 5;//24
		p_ptr->dis_to_h += 5;//24
                p_ptr->to_d += 10;
                p_ptr->dis_to_d += 10;
		p_ptr->to_a -= 10;//10
		p_ptr->dis_to_a -= 10;//10
	}

	/* Temporary "Berserk" */
	if (p_ptr->berserk)
	{
		p_ptr->to_h -=5;
		p_ptr->dis_to_h -=5;
                p_ptr->to_d += 15;
                p_ptr->dis_to_d += 15;
		p_ptr->to_a -= 10;
		p_ptr->dis_to_a -= 10;
		p_ptr->extra_blows++;
	}

	/* Temporary "Fury" */
	if (p_ptr->fury)
	{
		p_ptr->to_h -= 10;
		p_ptr->dis_to_h -= 10;
                p_ptr->to_d += 20;
                p_ptr->dis_to_d += 20;
		p_ptr->pspeed += 5;
                p_ptr->to_a -= 30;
                p_ptr->dis_to_a -= 30;
	}

	if (p_ptr->combat_stance == 2) {
		p_ptr->to_a -= 30;
		p_ptr->dis_to_a -= 30;
	}

	/* Temporary "fast" */
	if (p_ptr->fast)
	{
		p_ptr->pspeed += p_ptr->fast_mod;
	}
	
	/* Extra speed bonus from Zeal prayer */
//	if (p_ptr->zeal) p_ptr->pspeed += (p_ptr->zeal_power / 6);

	/* Temporary "slow" */
	if (p_ptr->slow)
	{
		p_ptr->pspeed -= 10;
	}

	/* Temporary see invisible */
	if (p_ptr->tim_invis)
	{
		p_ptr->see_inv = TRUE;
	}

	/* Temporary infravision boost */
	if (p_ptr->tim_infra)
	{
//		p_ptr->see_infra++;
		p_ptr->see_infra += 5;
	}

	/* Temporary st-anchor */
	if (p_ptr->st_anchor)
	{
		p_ptr->resist_continuum = TRUE;
	}

	/* Hack -- Res Chaos -> Res Conf */
	if (p_ptr->resist_chaos)
	{
		p_ptr->resist_conf = TRUE;
	}

	/* Heart is boldened */
	if (p_ptr->res_fear_temp || p_ptr->hero || p_ptr->shero ||
	    p_ptr->fury || p_ptr->berserk || p_ptr->mindboost)
	{
		p_ptr->resist_fear = TRUE;
	}


	/* Hack -- Telepathy Change */
	if (p_ptr->telepathy != old_telepathy)
	{
		p_ptr->update |= (PU_MONSTERS);
	}

	/* Hack -- See Invis Change */
	if (p_ptr->see_inv != old_see_inv)
	{
		p_ptr->update |= (PU_MONSTERS);
	}

	/* Temporary space-time anchor */
	if (p_ptr->st_anchor)
	{
		p_ptr->anti_tele = TRUE;
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
#if 1
		if (stealth >= 10) {
//			p_ptr->skill_stl *= 3;
//			p_ptr->skill_stl = p_ptr->skill_stl * stealth / 10;
			p_ptr->skill_stl = p_ptr->skill_stl + stealth / 10;
		}
#endif
		if (sneakiness >= 0) {
			/* prevent TLs from getting glitched abysmal stealth here.. */
			if (p_ptr->skill_srh >= 0) p_ptr->skill_srh = (p_ptr->skill_srh * (sneakiness + 50)) / 50;
//			p_ptr->skill_srh = p_ptr->skill_srh + sneakiness / 3;
//			p_ptr->skill_fos = p_ptr->skill_fos * sneakiness / 10;
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
		p_ptr->skill_stl = p_ptr->skill_stl + 25;
		p_ptr->skill_srh = p_ptr->skill_srh + 10;
	}

	if (p_ptr->shadow_running) {
		p_ptr->pspeed += 10;
	}

        /* Assume player not encumbered by armor */
        p_ptr->cumber_armor = FALSE;
        /* Heavy armor penalizes to-hit and sneakiness speed */
        if (((armour_weight(p_ptr) - (100 + get_skill_scale(p_ptr, SKILL_COMBAT, 300) + adj_str_hold[p_ptr->stat_ind[A_STR]] * 6)) / 10) > 0)
        {
                /* Encumbered */
                p_ptr->cumber_armor = TRUE;
        }
	/* Take note when "armor state" changes */
	if (p_ptr->old_cumber_armor != p_ptr->cumber_armor)
	{
		/* Message */
		if (p_ptr->cumber_armor)
		{
			msg_print(Ind, "\377oThe weight of your armour encumbers your movement.");
		}
		else
		{
			msg_print(Ind, "\377gYou feel able to move more freely.");
		}

		/* Save it */
		p_ptr->old_cumber_armor = p_ptr->cumber_armor;
	}


	p_ptr->rogue_heavyarmor = rogue_heavy_armor(p_ptr);
	if (p_ptr->old_rogue_heavyarmor != p_ptr->rogue_heavyarmor) {
		if (p_ptr->rogue_heavyarmor) {
			msg_print(Ind, "\377oThe weight of your armour strains your flexibility and awareness.");
			break_cloaking(Ind, 0);
			break_shadow_running(Ind);
			if (p_ptr->dual_wield && !p_ptr->warning_dual && p_ptr->pclass != CLASS_ROGUE && p_ptr->max_plv <= 10) {
				p_ptr->warning_dual = 1;
				msg_print(Ind, "\374\377yHINT: You won't get any benefits from dual-wielding if your armour is");
				msg_print(Ind, "\374\377y      too heavy; in that case, consider using one weapon together with");
				msg_print(Ind, "\374\377y      a shield, or -if your STR is very high- try a two-handed weapon.");
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
		p_ptr->old_rogue_heavyarmor = p_ptr->rogue_heavyarmor;
	}

	if (p_ptr->stormbringer) {
		if (p_ptr->cloaked) {
			msg_print(Ind, "\377yYou cannot remain cloaked effectively while wielding the stormbringer!");
			break_cloaking(Ind, 0);
		}
		if (p_ptr->shadow_running) {
			msg_print(Ind, "\377yThe stormbringer hinders effective shadow running!");
			break_shadow_running(Ind);
		}
	}
	if (p_ptr->aggravate) {
		if (p_ptr->cloaked) {
			msg_print(Ind, "\377yYou cannot remain cloaked effectively while using aggravating items!");
			break_cloaking(Ind, 0);
		}
	}
	if (p_ptr->inventory[INVEN_WIELD].k_idx && (k_info[p_ptr->inventory[INVEN_WIELD].k_idx].flags4 & (TR4_MUST2H | TR4_SHOULD2H))) {
		if (p_ptr->cloaked && !instakills(Ind)) {
			msg_print(Ind, "\377yYour weapon is too large to remain cloaked effectively.");
			break_cloaking(Ind, 0);
		}
		if (p_ptr->shadow_running && !instakills(Ind)) {
			msg_print(Ind, "\377yYour weapon is too large for effective shadow running.");
			break_shadow_running(Ind);
		}
	}
	if (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD) {
		if (p_ptr->cloaked) {
			msg_print(Ind, "\377yYou cannot remain cloaked effectively while wielding a shield.");
			break_cloaking(Ind, 0);
		}
		if (p_ptr->shadow_running) {
			msg_print(Ind, "\377yYour shield hinders effective shadow running.");
			break_shadow_running(Ind);
		}
	}

#if 0 // DGDGDGDG -- skill powa !
	switch(p_ptr->pclass)
	{
		case CLASS_MONK:
			/* Unencumbered Monks become faster every 10 levels */
			if (!(monk_heavy_armor(Ind)))
				p_ptr->pspeed += (p_ptr->lev) / 10;

			/* Free action if unencumbered at level 25 */
			if  ((p_ptr->lev > 24) && !(monk_heavy_armor(Ind)))
				p_ptr->free_act = TRUE;
			break;

			/* -APD- Hack -- rogues +1 speed at 5,20,35,50.
			 * this may be out of place, but.... */
	
		case CLASS_ROGUE:
			p_ptr->pspeed += p_ptr->lev / 6;
			break;
	}
#else	/* 0 */
	p_ptr->monk_heavyarmor = monk_heavy_armor(p_ptr);
	if (p_ptr->old_monk_heavyarmor != p_ptr->monk_heavyarmor) {
		if (p_ptr->monk_heavyarmor)
		{
			msg_print(Ind, "\377oThe weight of your armour strains your martial arts performance.");
		}
		else
		{
			msg_print(Ind, "\377gYour armour is comfortable for using martial arts.");
		}
		p_ptr->old_monk_heavyarmor = p_ptr->monk_heavyarmor;
	}
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS) && !monk_heavy_armor(p_ptr))
	{
		long int k = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 5), w = 0;

		if (k)
		{
			/* Extract the current weight (in tenth pounds) */
			w = p_ptr->total_weight;

			/* Extract the "weight limit" (in tenth pounds) */
			i = weight_limit(Ind);

			/* XXX XXX XXX Apply "encumbrance" from weight */
			if (w> i/5) k -= ((w - (i/5)) / (i / 10));

			/* Assume unencumbered */
			p_ptr->cumber_weight = FALSE;

			if (k > 0)
			{
				/* Feather Falling if unencumbered at level 10 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 9)
					p_ptr->feather_fall = TRUE;

				/* Fear Resistance if unencumbered at level 15 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 14)
					p_ptr->resist_fear = TRUE;

				/* Confusion Resistance if unencumbered at level 20 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 19)
					p_ptr->resist_conf = TRUE;

				/* Free action if unencumbered at level 25 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 24)
					p_ptr->free_act = TRUE;

				/* Swimming if unencumbered at level 30 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 29)
					p_ptr->can_swim = TRUE;

				/* Climbing if unencumbered at level 40 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 39)
					p_ptr->climb = TRUE;

				/* Flying if unencumbered at level 50 */
				if  (get_skill(p_ptr, SKILL_MARTIAL_ARTS) > 49)
					p_ptr->fly = TRUE;

#if 0
				if (((!p_ptr->inventory[INVEN_ARM].k_idx) ||
				    (k_info[p_ptr->inventory[INVEN_ARM].k_idx].weight < 100)) &&
				    ((!p_ptr->inventory[INVEN_BOW].k_idx) ||
				    (k_info[p_ptr->inventory[INVEN_BOW].k_idx].weight < 150)) &&
				    ((!p_ptr->inventory[INVEN_WIELD].k_idx) ||
				    (k_info[p_ptr->inventory[INVEN_WIELD].k_idx].weight < 150)))
#else
				w = 0; 
				if (p_ptr->inventory[INVEN_ARM].k_idx) w += k_info[p_ptr->inventory[INVEN_ARM].k_idx].weight;
				if (p_ptr->inventory[INVEN_BOW].k_idx) w += k_info[p_ptr->inventory[INVEN_BOW].k_idx].weight;
				if (p_ptr->inventory[INVEN_WIELD].k_idx) w += k_info[p_ptr->inventory[INVEN_WIELD].k_idx].weight;
				if (w < 150)
#endif
				{
					/* give a speed bonus */
					p_ptr->pspeed += k;

					/* give a stealth bonus */
					p_ptr->skill_stl += k;
				}
			}
			else
			{
			    p_ptr->cumber_weight = TRUE;
			}
			if (p_ptr->old_cumber_weight != p_ptr->cumber_weight)
			{
				if (p_ptr->cumber_weight)
				{
					msg_print(Ind, "\377RYou can't move freely due to your backpack weight.");

				}
				else
				{
					msg_print(Ind, "\377gYour backpack is very comfortable to wear.");
				}
				p_ptr->old_cumber_weight = p_ptr->cumber_weight;
			}
		}

		/* Monks get extra ac for wearing very light or no armour at all */
		if (!p_ptr->inventory[INVEN_BOW].k_idx &&
			!p_ptr->inventory[INVEN_WIELD].k_idx &&
			(!p_ptr->inventory[INVEN_ARM].k_idx || p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD) && /* for dual-wielders */
			!p_ptr->cumber_weight)
		{
			int marts = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 60);
			int martsbonus, martsweight, martscapacity;
			
			martsbonus = (marts * 3) / 2 * MARTIAL_ARTS_AC_ADJUST / 100;
			martsweight = p_ptr->inventory[INVEN_BODY].weight;
			martscapacity = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 80);
			if (!(p_ptr->inventory[INVEN_BODY].k_idx))
			{
				p_ptr->to_a += martsbonus;
				p_ptr->dis_to_a += martsbonus;
			} else if (martsweight <= martscapacity){
				p_ptr->to_a += (martsbonus * (martscapacity - martsweight) / martscapacity);
				p_ptr->dis_to_a += (martsbonus * (martscapacity - martsweight) / martscapacity);
			}

			martsbonus = ((marts - 13) / 3) * MARTIAL_ARTS_AC_ADJUST / 100;
			martsweight = p_ptr->inventory[INVEN_OUTER].weight;
			martscapacity = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 40);
			if (!(p_ptr->inventory[INVEN_OUTER].k_idx) && (marts > 15))
			{
				p_ptr->to_a += martsbonus;
				p_ptr->dis_to_a += martsbonus;
			} else if ((martsweight <= martscapacity) && (marts > 15)) {
				p_ptr->to_a += (martsbonus * (martscapacity - martsweight) / martscapacity);
				p_ptr->dis_to_a += (martsbonus * (martscapacity - martsweight) / martscapacity);
			}

			martsbonus = ((marts - 8) / 3) * MARTIAL_ARTS_AC_ADJUST / 100;
			martsweight = p_ptr->inventory[INVEN_ARM].weight;
			martscapacity = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 30);
			if ((!p_ptr->inventory[INVEN_ARM].k_idx || p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) && (marts > 10)) /* for dual-wielders */
			{
				p_ptr->to_a += martsbonus;
				p_ptr->dis_to_a += martsbonus;
			} else if ((martsweight <= martscapacity) && (marts > 10)) {
				p_ptr->to_a += (martsbonus * (martscapacity - martsweight) / martscapacity);
				p_ptr->dis_to_a += (martsbonus * (martscapacity - martsweight) / martscapacity);
			}

			martsbonus = (marts - 2) / 3 * MARTIAL_ARTS_AC_ADJUST / 100;
			martsweight = p_ptr->inventory[INVEN_HEAD].weight;
			martscapacity = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 50);
			if (!(p_ptr->inventory[INVEN_HEAD].k_idx)&& (marts > 4))
			{
				p_ptr->to_a += martsbonus;
				p_ptr->dis_to_a += martsbonus;
			} else if ((martsweight <= martscapacity) && (marts > 4)) {
				p_ptr->to_a += (martsbonus * (martscapacity - martsweight) / martscapacity);
				p_ptr->dis_to_a += (martsbonus * (martscapacity - martsweight) / martscapacity);
			}

			martsbonus = (marts / 2) * MARTIAL_ARTS_AC_ADJUST / 100;
			martsweight = p_ptr->inventory[INVEN_HANDS].weight;
			martscapacity = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 20);
			if (!(p_ptr->inventory[INVEN_HANDS].k_idx))
			{
				p_ptr->to_a += martsbonus;
				p_ptr->dis_to_a += martsbonus;
			} else if (martsweight <= martscapacity) {
				p_ptr->to_a += (martsbonus * (martscapacity - martsweight) / martscapacity);
				p_ptr->dis_to_a += (martsbonus * (martscapacity - martsweight) / martscapacity);
			}

			martsbonus = (marts / 3) * MARTIAL_ARTS_AC_ADJUST / 100;
			martsweight = p_ptr->inventory[INVEN_FEET].weight;
			martscapacity = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 45);
			if (!(p_ptr->inventory[INVEN_FEET].k_idx))
			{
				p_ptr->to_a += martsbonus;
				p_ptr->dis_to_a += martsbonus;
			} else if (martsweight <= martscapacity) {
				p_ptr->to_a += (martsbonus * (martscapacity - martsweight) / martscapacity);
				p_ptr->dis_to_a += (martsbonus * (martscapacity - martsweight) / martscapacity);
			}
		}
	}

#endif	// 0

#if 0 // DGDGDGDGDG - no monks ffor the time being
	/* Monks get extra ac for armour _not worn_ */
//	if ((p_ptr->pclass == CLASS_MONK) && !(monk_heavy_armor(Ind)))
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS) && !(monk_heavy_armor(p_ptr)) &&
		!(p_ptr->inventory[INVEN_BOW].k_idx))
	{
		int marts = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 60);
		if (!(p_ptr->inventory[INVEN_BODY].k_idx))
		{
			p_ptr->to_a += (marts * 3) / 2 * MARTIAL_ARTS_AC_ADJUST / 100;
			p_ptr->dis_to_a += (marts * 3) / 2 * MARTIAL_ARTS_AC_ADJUST / 100;
		}
		if (!(p_ptr->inventory[INVEN_OUTER].k_idx) && (marts > 15))
		{
			p_ptr->to_a += ((marts - 13) / 3) * MARTIAL_ARTS_AC_ADJUST / 100;
			p_ptr->dis_to_a += ((marts - 13) / 3) * MARTIAL_ARTS_AC_ADJUST / 100;
		}
		if (!(p_ptr->inventory[INVEN_ARM].k_idx || p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) && (marts > 10)) /* for dual-wielders */
		{
			p_ptr->to_a += ((marts - 8) / 3) * MARTIAL_ARTS_AC_ADJUST / 100;
			p_ptr->dis_to_a += ((marts - 8) / 3) * MARTIAL_ARTS_AC_ADJUST / 100;
		}
		if (!(p_ptr->inventory[INVEN_HEAD].k_idx)&& (marts > 4))
		{
			p_ptr->to_a += (marts - 2) / 3 * MARTIAL_ARTS_AC_ADJUST / 100;
			p_ptr->dis_to_a += (marts -2) / 3 * MARTIAL_ARTS_AC_ADJUST / 100;
		}
		if (!(p_ptr->inventory[INVEN_HANDS].k_idx))
		{
			p_ptr->to_a += (marts / 2) * MARTIAL_ARTS_AC_ADJUST / 100;
			p_ptr->dis_to_a += (marts / 2) * MARTIAL_ARTS_AC_ADJUST / 100;
		}
		if (!(p_ptr->inventory[INVEN_FEET].k_idx))
		{
			p_ptr->to_a += (marts / 3) * MARTIAL_ARTS_AC_ADJUST / 100;
			p_ptr->dis_to_a += (marts / 3) * MARTIAL_ARTS_AC_ADJUST / 100;
		}
	}
#endif


	/* Evaluate monster AC (if skin or armor etc) */
	if (p_ptr->body_monster) {
		if (p_ptr->pclass == CLASS_DRUID)
			body = 1 + 3 + 0 + 0; /* 0 1 0 2 1 0 = typical ANIMAL pattern,
						need to hardcode it here to balance
						'Spectral tyrannosaur' form especially.
						(weap, tors, arms, finger, head, leg) */
		else if ((p_ptr->pclass == CLASS_SHAMAN) && strchr("EG", r_ptr->d_char))
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
#if 0
			/* Vary between 3/4 + 1/4 (low monsters) to 2/3 + 1/3 (high monsters): */
			p_ptr->ac = (p_ptr->ac * (100 - r_ptr->level + 200)) / (100 - r_ptr->level + 300);
			p_ptr->to_a = ((p_ptr->to_a * (100 - r_ptr->level + 200)) + (toac * 100)) / (100 - r_ptr->level + 300);
			p_ptr->dis_ac = (p_ptr->dis_ac * (100 - r_ptr->level + 200)) / (100 - r_ptr->level + 300);
			p_ptr->dis_to_a = ((p_ptr->dis_to_a * (100 - r_ptr->level + 200)) + (toac * 100)) / (100 - r_ptr->level + 300);
#else
			p_ptr->ac = (p_ptr->ac * 3) / 4;
			p_ptr->to_a = ((p_ptr->to_a * 3) + toac) / 4;
			p_ptr->dis_ac = (p_ptr->dis_ac * 3) / 4;
			p_ptr->dis_to_a = ((p_ptr->dis_to_a * 3) + toac) / 4;
#endif
		} else {
			p_ptr->ac = (p_ptr->ac * 1) / 2;
			p_ptr->to_a = ((p_ptr->to_a * 1) + (toac * 1)) / 2;
			p_ptr->dis_ac = (p_ptr->dis_ac * 1) / 2;
			p_ptr->dis_to_a = ((p_ptr->dis_to_a * 1) + (toac * 1)) / 2;
		}
		/*	p_ptr->dis_ac = (toac < p_ptr->dis_ac) ?
		(((p_ptr->dis_ac * 2) + (toac * 1)) / 3) :
		(((p_ptr->dis_ac * 1) + (toac * 1)) / 2);*/
	}

	if (p_ptr->mode & MODE_HARD)
	{
		if (p_ptr->dis_to_a > 0) p_ptr->dis_to_a = (p_ptr->dis_to_a * 2) / 3;
		if (p_ptr->to_a > 0) p_ptr->to_a = (p_ptr->to_a * 2) / 3;
	}


	/* Redraw armor (if needed) */
	if ((p_ptr->dis_ac != old_dis_ac) || (p_ptr->dis_to_a != old_dis_to_a))
	{
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
	if (o_ptr->k_idx && hold < o_ptr->weight / 10)
	{
		/* Hard to wield a heavy bow */
		p_ptr->to_h += 2 * (hold - o_ptr->weight / 10);
		p_ptr->dis_to_h += 2 * (hold - o_ptr->weight / 10);

		/* Heavy Bow */
		p_ptr->heavy_shoot = TRUE;
	}

	/* Compute "extra shots" if needed */
	if (o_ptr->k_idx && !p_ptr->heavy_shoot)
	{
		int archery = get_archery_skill(p_ptr);

		p_ptr->tval_ammo = 0;

		/* Take note of required "tval" for missiles */
		switch (o_ptr->sval)
		{
			case SV_SLING:
			{
				p_ptr->tval_ammo = TV_SHOT;
				break;
			}

			case SV_SHORT_BOW:
			case SV_LONG_BOW:
			{
				p_ptr->tval_ammo = TV_ARROW;
				break;
			}

			case SV_LIGHT_XBOW:
			case SV_HEAVY_XBOW:
			{
				p_ptr->tval_ammo = TV_BOLT;
				break;
			}
		}

		if (archery != -1)
		{
			p_ptr->to_h_ranged += get_skill_scale(p_ptr, archery, 25);
			/* the "xtra_might" adds damage for specifics */
			p_ptr->to_d_ranged += get_skill_scale(p_ptr, SKILL_ARCHERY, 10);
#if 0 /* new, since RPG_SERVER - C. Blue */
			/* Isn't 4 shots/turn too small? Not with art ammo */
			p_ptr->num_fire += (get_skill(p_ptr, archery) / 16);
//				+ get_skill_scale(p_ptr, SKILL_ARCHERY, 1);
#else
			switch(archery){
			case SKILL_SLING:
				p_ptr->num_fire += get_skill_scale(p_ptr, archery, 500) / 100;
				break;
			case SKILL_BOW:
				p_ptr->num_fire += get_skill_scale(p_ptr, archery, 500) / 125;
				break;
			case SKILL_XBOW:
				p_ptr->num_fire += get_skill_scale(p_ptr, archery, 500) / 250;
				p_ptr->xtra_might += get_skill_scale(p_ptr, archery, 500) / 250;
				break;
			case SKILL_BOOMERANG:
				if (get_skill_scale(p_ptr, archery, 500) >= 500) p_ptr->num_fire++;
				if (get_skill_scale(p_ptr, archery, 500) >= 333) p_ptr->num_fire++;
				if (get_skill_scale(p_ptr, archery, 500) >= 166) p_ptr->num_fire++;
				break;
			}
#endif
//			p_ptr->xtra_might += (get_skill(p_ptr, archery) / 50);
			p_ptr->xtra_might += (get_skill(p_ptr, SKILL_ARCHERY) / 50);
			/* Boomerang-mastery directly increases damage! - C. Blue */
			if (archery == SKILL_BOOMERANG)
				p_ptr->to_d_ranged += get_skill_scale(p_ptr, archery, 20);
#if 0	// not so meaningful (25,30,50)
			switch (archery)
			{
				case SKILL_SLING:
					if (p_ptr->tval_ammo == TV_SHOT)
						p_ptr->xtra_might += get_skill(p_ptr, archery) / 30;
					break;
				case SKILL_BOW:
					if (p_ptr->tval_ammo == TV_ARROW)
						p_ptr->xtra_might += get_skill(p_ptr, archery) / 30;
					break;
				case SKILL_XBOW:
					if (p_ptr->tval_ammo == TV_BOLT)
						p_ptr->xtra_might += get_skill(p_ptr, archery) / 30;
					break;
				case SKILL_BOOMERANG:
					if (!p_ptr->tval_ammo)
						p_ptr->xtra_might += get_skill(p_ptr, archery) / 30;
					break;
			}
#endif	// 0
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
	if(o_ptr->k_idx && o_ptr->tval == TV_DIGGING)
	{
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
	if (o_ptr->k_idx && o_ptr->tval == TV_SHIELD && (hold < (o_ptr->weight/10 - 4) * 7)) /* dual-wielder? */
		p_ptr->heavy_shield = TRUE;

#ifdef USE_BLOCKING
	if (o_ptr->k_idx && o_ptr->tval == TV_SHIELD) { /* might be dual-wielder */
		int malus;
 #ifdef USE_NEW_SHIELDS
		p_ptr->shield_deflect = o_ptr->ac * 10; /* add a figure by multiplying by _10_ for more accuracy */
 #else
		p_ptr->shield_deflect = 20 * (o_ptr->ac + 3); /* add a figure by multiplying by 2*_10_ for more accuracy */
 #endif
		malus = (o_ptr->weight/10 - 4) * 7 - hold;
		p_ptr->shield_deflect /= malus > 0 ? 10 + (malus + 4) / 3 : 10;

		/* adjust class-dependantly! */
		switch (p_ptr->pclass) {
		case CLASS_ADVENTURER: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 1) / 3; break;
		case CLASS_WARRIOR: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 0) / 2; break;
		case CLASS_PRIEST: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 2) / 4; break;
		case CLASS_MAGE: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 4) / 6; break;
		case CLASS_ARCHER: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 1) / 3; break;
		case CLASS_ROGUE: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 1) / 3; break;
//		case CLASS_MIMIC: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 0) / 2; break;
		case CLASS_MIMIC: p_ptr->shield_deflect = (p_ptr->shield_deflect * 3 + 1) / 4; break;
		case CLASS_RANGER: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 1) / 3; break;
//		case CLASS_PALADIN: p_ptr->shield_deflect = (p_ptr->shield_deflect * 3 + 1) / 4; break;
		case CLASS_PALADIN: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 0) / 2; break;
		case CLASS_SHAMAN: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 2) / 4; break;
		case CLASS_DRUID: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 1) / 3; break;
		case CLASS_RUNEMASTER: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 1) / 3; break;
		case CLASS_MINDCRAFTER: p_ptr->shield_deflect = (p_ptr->shield_deflect * 2 + 1) / 3; break;
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
 #if 0 /* instead of making it completely SKILL_MASTERY dependant,...*/
		if (k_info[o_ptr->k_idx].flags4 & TR4_MUST2H) {
			p_ptr->weapon_parry = get_skill_scale(p_ptr, SKILL_MASTERY, 30);
		} else if (k_info[o_ptr->k_idx].flags4 & TR4_SHOULD2H) {
			if (!p_ptr->inventory[INVEN_ARM].k_idx) p_ptr->weapon_parry = get_skill_scale(p_ptr, SKILL_MASTERY, 25);
			else p_ptr->weapon_parry = get_skill_scale(p_ptr, SKILL_MASTERY, 5);
		} else if (k_info[o_ptr->k_idx].flags4 & TR4_COULD2H) {
			if (!p_ptr->inventory[INVEN_ARM].k_idx) p_ptr->weapon_parry = get_skill_scale(p_ptr, SKILL_MASTERY, 20);
			else p_ptr->weapon_parry = get_skill_scale(p_ptr, SKILL_MASTERY, 10);
		} else {
			p_ptr->weapon_parry = get_skill_scale(p_ptr, SKILL_MASTERY, 10);
		}
		/* for dual-wielders: */
		if ((p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) &&
		    !p_ptr->rogue_heavyarmor)
			p_ptr->weapon_parry += get_skill_scale(p_ptr, SKILL_MASTERY, 10);
 #else /*..we add some base chance too (mainly for PvP!) */
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
		if (p_ptr->dual_wield && !p_ptr->rogue_heavyarmor)
			//p_ptr->weapon_parry += 5 + get_skill_scale(p_ptr, SKILL_MASTERY, 5);//was +0(+10)
			p_ptr->weapon_parry += 10;//pretty high, because independent of mastery skill^^
 #endif
		/* adjust class-dependantly! */
		switch (p_ptr->pclass) {
		case CLASS_ADVENTURER: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2 + 1) / 3; break;
		case CLASS_WARRIOR: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2 + 0) / 2; break;
		case CLASS_PRIEST: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2 + 2) / 4; break;
		case CLASS_MAGE: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2 + 4) / 6; break;
		case CLASS_ARCHER: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2 + 1) / 3; break;
		case CLASS_ROGUE: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2 + 0) / 2; break;
		case CLASS_MIMIC: p_ptr->weapon_parry = (p_ptr->weapon_parry * 3 + 1) / 4; break;
		case CLASS_RANGER: p_ptr->weapon_parry = (p_ptr->weapon_parry * 4 + 0) / 4; break;
//		case CLASS_PALADIN: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2 + 0) / 2; break;
		case CLASS_PALADIN: p_ptr->weapon_parry = (p_ptr->weapon_parry * 3 + 1) / 4; break;
		case CLASS_SHAMAN: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2 + 2) / 4; break;
		case CLASS_DRUID: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2 + 1) / 3; break;
		case CLASS_RUNEMASTER: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2 + 1) / 3; break;
		case CLASS_MINDCRAFTER: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2 + 1) / 3; break;
		}
	}
#endif

	
	/* It is hard to hold a heavy weapon */
	o_ptr = &p_ptr->inventory[INVEN_WIELD];
	if (o_ptr->k_idx && hold < o_ptr->weight / 10)
	{
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
	if (o_ptr->k_idx && o_ptr->tval != TV_SHIELD && hold < o_ptr->weight / 10)
	{
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
	if ((o_ptr->k_idx || (o2_ptr->k_idx && o2_ptr->tval != TV_SHIELD)) && !p_ptr->heavy_wield)
	{
		int lev1 = -1, lev2 = -1;
		p_ptr->num_blow = calc_blows_weapons(Ind);
		p_ptr->num_blow += p_ptr->extra_blows;
		/* Boost blows with masteries */
		/* note for dual-wield: instead of using two different p_ptr->to_h/d_melee for each
		   weapon, we just average the mastery boni we'd receive on each - C. Blue */
		if (get_weaponmastery_skill(p_ptr, &p_ptr->inventory[INVEN_WIELD]) != -1)
			lev1 = get_skill(p_ptr, get_weaponmastery_skill(p_ptr, &p_ptr->inventory[INVEN_WIELD]));
		if (get_weaponmastery_skill(p_ptr, &p_ptr->inventory[INVEN_ARM]) != -1)
			lev2 = get_skill(p_ptr, get_weaponmastery_skill(p_ptr, &p_ptr->inventory[INVEN_ARM]));
		/* if we don't wear any weapon at all, we get 0 bonus */
		if (lev1 == -1 && lev2 == -1) lev2 = 0;
		/* if we don't dual-wield, we mustn't average things */
		if (lev1 == -1) lev1 = lev2;
		if (lev2 == -1) lev2 = lev1;
		/* average for dual-wield */
		p_ptr->to_h_melee += ((lev1 + lev2) / 2);
		p_ptr->to_d_melee += ((lev1 + lev2) / 4);
//		p_ptr->num_blow += get_skill_scale(p_ptr, get_weaponmastery_skill(p_ptr), 2);

	}


	/* Different calculation for monks with empty hands */
#if 1 // DGDGDGDG -- no more monks for the time being
//	if (p_ptr->pclass == CLASS_MONK)
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS) && !o_ptr->k_idx &&
		!(p_ptr->inventory[INVEN_BOW].k_idx))
	{
		int marts = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 50);
		p_ptr->num_blow = 0;

		/*the_sandman for the RPG server (might as well stay for all servers ^^ - C. Blue) */
		if (marts >  0) p_ptr->num_blow++;
		if (marts >  9) p_ptr->num_blow++;
		if (marts > 19) p_ptr->num_blow++;
		if (marts > 29) p_ptr->num_blow++;
//		if (marts > 34) p_ptr->num_blow++; /* this is 'marts > 0' now */
		if (marts > 39) p_ptr->num_blow++;
		if (marts > 44) p_ptr->num_blow++;
		if (marts > 49) p_ptr->num_blow++;	/* Do we want 9ea from MA? certainly not. */

		if (monk_heavy_armor(p_ptr)) p_ptr->num_blow /= 2;

		p_ptr->num_blow += 1 + p_ptr->extra_blows;

		if (!monk_heavy_armor(p_ptr))
		{
/*			p_ptr->to_h += (marts / 3) * 2;
			p_ptr->to_d += (marts / 3);

			p_ptr->dis_to_h += (marts / 3) * 2;
			p_ptr->dis_to_d += (marts / 3);
*/
			p_ptr->to_h_melee += marts*3/2; 	
			/* the_sandman: needs to be played out properly.. */
			/* the new addition. max dex (40) -> 90 to_h base.. decent imo */
			/* DEX influence the to hit. lets make every 5 dex -> +1 toHit */
			p_ptr->to_h_melee += (int)((p_ptr->stat_cur[A_DEX])/5);
			p_ptr->to_d_melee += (marts / 3);/* was 3, experimental 
							    was 4, too low w/o mimicry for decent forms -,- reverted back to 3. the_sandman */
			/* Testing: added a new bonus. No more single digit dmg at lvl 20, I hope, esp as warrior. the_sandman */
			/* Lowered to marts/2 */
			/* changed by C. Blue, so it's available to mimics too */
			p_ptr->to_d_melee += 20 - (400 / (marts + 20)); /* get strong quickly and early - quite special ;-o */
		}

#if 0 /* already done further above! */		
		/* At least +1, max. +3 */
		if (p_ptr->zeal) p_ptr->num_blow += p_ptr->zeal_power / 10 > 3 ? 3 : (p_ptr->zeal_power / 10 < 1 ? 1 : p_ptr->zeal_power / 10);
#endif
	}
	else /* make cumber_armor have effect on to-hit for non-martial artists too - C. Blue */
	{
		if (p_ptr->cumber_armor && (p_ptr->to_h_melee > 0)) p_ptr->to_h_melee = (p_ptr->to_h_melee * 2) / 3;
	}
#endif

//	p_ptr->pspeed += get_skill_scale(p_ptr, SKILL_AGILITY, 10);
	if (p_ptr->cumber_armor) p_ptr->pspeed += get_skill_scale(p_ptr, SKILL_SNEAKINESS, 4);
	else p_ptr->pspeed += get_skill_scale(p_ptr, SKILL_SNEAKINESS, 7);

	/* Add the buffs bonuses here */
	p_ptr->pspeed += p_ptr->rune_speed; 
	p_ptr->skill_stl += p_ptr->rune_stealth;
	p_ptr->see_infra += p_ptr->rune_IV;
	p_ptr->redraw |= (PR_SPEED|PR_EXTRA) ;

	if (p_ptr->mode & MODE_HARD && p_ptr->pspeed > 110)
	{
		int speed = p_ptr->pspeed - 110;

		speed /= 2;

		p_ptr->pspeed = speed + 110;
	}


	/* Hell mode is HARD */
	if ((p_ptr->mode & MODE_HARD) && (p_ptr->num_blow > 1)) p_ptr->num_blow--;

	/* A perma_cursed weapon stays even in weapon-less body form, reduce blows for that: */
	if ((p_ptr->inventory[INVEN_WIELD].k_idx ||
	    (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) &&
	    (!r_info[p_ptr->body_monster].body_parts[BODY_WEAPON]) &&
	    (p_ptr->num_blow > 1)) p_ptr->num_blow = 1;

	/* Combat bonus to damage */
	if (get_skill(p_ptr, SKILL_COMBAT))
	{
		int lev = get_skill_scale(p_ptr, SKILL_COMBAT, 10);

#if 0
                p_ptr->dis_to_h += lev;
		p_ptr->to_h += lev;
#else /* reason for this is that DEX has influence on combat +hit bonus! (only affects h_melee and h_ranged, not to_h */
                p_ptr->to_h_melee += lev;
                p_ptr->to_h_ranged += lev;
#endif

/*		if (o_ptr->k_idx) {
			p_ptr->to_d += lev;
			p_ptr->dis_to_d += lev;*/
			p_ptr->to_d_melee += lev;
/*		}*/
	}

	/* Weaponmastery bonus to damage - not for MA!- C. Blue */
	if (get_skill(p_ptr, SKILL_MASTERY) && o_ptr->k_idx)
	{
		int lev = get_skill(p_ptr, SKILL_MASTERY);

/*		p_ptr->to_h += lev / 5;
		p_ptr->dis_to_h += lev / 5;
		p_ptr->to_d += lev / 5;
		p_ptr->dis_to_d += lev / 5;
*/		p_ptr->to_h_melee += lev / 3;
		p_ptr->to_d_melee += lev / 5;
	}

	if (get_skill(p_ptr, SKILL_DODGE) && !p_ptr->rogue_heavyarmor)
//	if (!(r_ptr->flags1 & RF1_NEVER_MOVE));		// not for now
	{
#ifndef NEW_DODGING /* reworking dodge, see #else.. */
		/* use a long var temporarily to handle v.high total_weight */
		long temp_chance;
		/* Get the armor weight */
		int cur_wgt = armour_weight(p_ptr); /* change to worn_armour_weight if ever reactivated, and possibly adjust values */
		/* Base dodge chance */
		temp_chance = get_skill_scale(p_ptr, SKILL_DODGE, 150);
		/* Armor weight bonus/penalty */
//		p_ptr->dodge_level -= cur_wgt * 2;
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
 #if 0
//			/* 25..30 lb equip, 50..90 lb inven if lightly equipped (50..80 difference depending on books/potions/scrolls (ew)) */
//			i = (p_ptr->total_weight / 10) + 50; /* 25>96% 50>92% 75>87% 100>80% 125>72% 150>62% */
//			i = 102 - ((i * i) / 1000);

//			i = (p_ptr->total_weight / 8) + 50; /* 25>95% 50>89% 75>81% 100>71% 125>59% 150>45% */
//			i = 101 - ((i * i) / 1000);

//			i = i / 8 + 50; /* 0:-0 25:-4 50:-10 75:-18 100:-28 125:-40 150:-54 */
//			i = 2 - (i * i) / 1000;

//			i = i / 7 + 50; /* 0:-0 25:-5 50:-12 75:-22 100:-34 125:-49 150:-67 */
//			i = 2 - (i * i) / 1000;

//			i = i / 10 + 0; /* 0:-0 25:-0 50:-3 75:-16 100:-40 125:-78 150:-135 */
//			i = (i * i * i) / 25000;

//			i = i / 20 + 0; /* 0:-0 25:-0 50:-3 75:-16 100:-40 125:-78 150:-135 */
//			i = (i * i * i) / 25000;

//			i = i / 7 + 20; /* 0:-0 25:-2 50:-7 75:-15 100:-25 125:-38 150:-53 */
//			i = 1 - (i * i) / 1000;
 #endif
			i = i / 5 + 10; /* 0:-0 25:-2 50:-8 75:-18 100:-31 125:-48 150:-68 */
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
 #if 0 /* done in apply_dodge_chance()! */
		i = (i * DODGE_MAX_CHANCE) / 100; /* Note how it's scaled, instead of capped! >:) */
		/* reduce player's dodge level if (s)he neglected to train it alongside character level */
		j = get_skill(p_ptr, SKILL_DODGE); /* note: training dodge skill +2 ahead (like other skills usually) won't help for dodging, sorry. See next line: */
		i = (i * (j > p_ptr->lev ? p_ptr->lev : j)) / (p_ptr->lev >= 50 ? 50 : p_ptr->lev);
 #endif
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
//	if (inventory[INVEN_WIELD + i].k_idx && inventory[INVEN_ARM + i].k_idx)
	if (p_ptr->inventory[INVEN_WIELD].k_idx && p_ptr->inventory[INVEN_ARM].k_idx)
	{
		/* Extract the item flags */
		object_flags(&p_ptr->inventory[INVEN_WIELD], &f1, &f2, &f3, &f4, &f5, &esp);

		if (f4 & TR4_SHOULD2H)
		{                
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
	if (p_ptr->inventory[INVEN_WIELD].k_idx && !p_ptr->inventory[INVEN_ARM].k_idx) {
		object_flags(&p_ptr->inventory[INVEN_WIELD], &f1, &f2, &f3, &f4, &f5, &esp);
		if (f4 & TR4_COULD2H) {
			p_ptr->easy_wield = TRUE;
		}
	}
	/* for dual-wield..*/
	if (!p_ptr->inventory[INVEN_WIELD].k_idx &&
	    p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD) {
		object_flags(&p_ptr->inventory[INVEN_ARM], &f1, &f2, &f3, &f4, &f5, &esp);
		if (f4 & TR4_COULD2H) {
			p_ptr->easy_wield = TRUE;
		}
	}

	/* Equipment weight affects shooting */
#if 0 /* adj_weight_tohr not yet implemented and maybe too distinct steps */
	i = armour_weight(p_ptr) / 200;
	if (i > 25) i = 25;
	p_ptr->to_h_ranged = (p_ptr->to_h_ranged * adj_weight_tohr[i]) / 100;
#endif
#if 0
	i = armour_weight(p_ptr) / 100;
	i = (i * i * i) / 1000; /* 1500 */
	if (i > 100) i = 100;
	p_ptr->to_h_ranged = (p_ptr->to_h_ranged * (100 - i)) / 100;
#endif
#if 0 /* was used a lot, but somehow allows way too high weight? x10 factor? */
	/* allow malus even */
	/* examples: 200.0: -4, 300.0: -13, 400.0: -32 */
	i = armour_weight(p_ptr) / 100;
	i = (i * i * i) / 2000;
	if (i > 50) i = 50; /* reached at 470.0 lbs */
	p_ptr->to_h_ranged -= i;
#endif
#if 1
	/* allow malus even */
	/* examples: 10.0: 0, 20.0: -4, 30.0: -13, 40.0: -32, 47.0: -50 cap */
	i = armour_weight(p_ptr) / 10;
	i = (i * i * i) / 2000;
	if (i > 50) i = 50; /* reached at 470.0 lbs */
	p_ptr->to_h_ranged -= i;
#endif
#if 0 /* or.. other possibility (somewhat older): */
	/* Apply ranged to-hit penalty depending on armour weight */
	/* examples: 10.0: -10%, 20.0: -20%, 30.0: -30%, 40.0: -40%, 50.0: -50% */
	p_ptr->to_h_ranged = (p_ptr->to_h_ranged * 100) / (100 + armour_weight(p_ptr) / 10);
#endif


	/* also: shield and ranged weapon = less accuracy for ranged weapon! */
	/* dual-wield currently does not affect shooting (!) */
	if (p_ptr->inventory[INVEN_BOW].k_idx && p_ptr->inventory[INVEN_ARM].k_idx
	    && p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)
	{
		/* can't aim well while carrying a shield on the arm! */
#if 0 /* the following part only punishes the +hit/+dam from trained skill - C. Blue */
		p_ptr->to_h_ranged = 0;/* /= 4; need more severeness, it was nothing */
		p_ptr->to_d_ranged = 0;/* bam */
#else
		/* the following part punishes all +hit/+dam for ranged weapons;
        	   trained fighters suffer more than untrained ones! - C. Blue */
		if (p_ptr->to_h_ranged >= 25) p_ptr->to_h_ranged = 0;
		else if (p_ptr->to_h_ranged >= 0) p_ptr->to_h_ranged -= 10 + (p_ptr->to_h_ranged * 3) / 5;
		else p_ptr->to_h_ranged -= 10;

 #if 0 /* Moltor agrees it's a bit too harsh probably */
		if (p_ptr->to_d_ranged >= 15) p_ptr->to_d_ranged = 0;
		else if (p_ptr->to_d_ranged >= 0) p_ptr->to_d_ranged -= 5 + (p_ptr->to_d_ranged * 2) / 3;
		else p_ptr->to_d_ranged -= 5;
 #endif
#endif
		p_ptr->awkward_shoot = TRUE;
	}

	/* Priest weapon penalty for non-blessed edged weapons */
//	if ((p_ptr->pclass == 2) && (!p_ptr->bless_blade) &&
//	if ((get_skill(p_ptr, SKILL_PRAY)) && (!p_ptr->bless_blade) &&
	if (p_ptr->inventory[INVEN_WIELD].k_idx &&
	    (p_ptr->pclass == CLASS_PRIEST) && (!p_ptr->bless_blade) &&
	    ((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM) ||
	    (o_ptr->tval == TV_AXE)))
	{
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
		if (!r_ptr->body_parts[BODY_WEAPON])
		{
			wepless = TRUE;
	    	}

		d = 0; n = 0;
		for (i = 0; i < 4; i++)
		{
			j = (r_ptr->blow[i].d_dice * r_ptr->blow[i].d_side);
			j += r_ptr->blow[i].d_dice;
			j /= 2;
			if (j) n++;

			/* Hack -- weaponless combat */
/*			if (wepless && j)
			{
				j *= 2;
			}
*/
			d += j; /* adding up average monster damage per round */
		}
		/* At least have 1 blow (although this line is not needed anymore) */
		if (n == 0) n = 1;

		/*d = (d / 2) / n;	// 8 // 7
		p_ptr->to_d += d;
		p_ptr->dis_to_d += d; - similar to HP: */

		/* Divide by player blow number instead of
		monster blow number :
		//d /= n;*/
		//d /= ((p_ptr->num_blow > 0) ? p_ptr->num_blow : 1);

		/* GWoP: 472, GB: 270, Green DR: 96 */
		/* Quarter the damage and cap against 150 (unreachable though)
		- even The Destroyer form would reach just 138 ;) */
		d /= 4; /* average monster damage per blow */

		/* Cap the to-dam if it's too great */
#if 0 /* OLD: */
//too lame:	if (d > 0) d = (15000 / ((10000 / d) + 100)) + 1;
//		if (d > 0) d = (15000 / ((15000 / d) + 65)) + 1;
//perfect if this were FINAL +dam, but it'll be averaged: if (d > 0) d = (2500 / ((1000 / d) + 20)) + 1;
		if (d > 0) d = (4000 / ((1500 / d) + 20)) + 1;
//		if (d > 0) d = (12500 / ((12500 / d) + 65)) + 1;
/*too powerful:	if (d > 0) d = (25000 / ((10000 / d) + 100)) + 1;
		if (d > 0) d = (20000 / ((10000 / d) + 100)) + 1;*/
#else /* super-low forms got too much +dam bonus! */
//slightly too lame:		if (d > 0) d = (4160 / ((2000 / (d + 4)) + 20)) - 8;
//slightly too powerful:	if (d > 0) d = (4000 / ((1500 / (d + 4)) + 20)) - 10;
		if (d > 0) d = (4000 / ((1500 / (d + 4)) + 22)) - 10;
#endif

		/* Calculate new averaged to-dam bonus */
		if (d < (p_ptr->to_d + p_ptr->to_d_melee)) {
			p_ptr->to_d = ((p_ptr->to_d * 5) + (d * 2)) / 7;
			p_ptr->to_d_melee = (p_ptr->to_d_melee * 5) / 7;
			p_ptr->dis_to_d = ((p_ptr->dis_to_d * 5) + (d * 2)) / 7;
		} else {
			p_ptr->to_d = ((p_ptr->to_d * 1) + (d * 1)) / 2;
			p_ptr->to_d_melee = (p_ptr->to_d_melee * 1) / 2;
			p_ptr->dis_to_d = ((p_ptr->dis_to_d * 1) + (d * 1)) / 2;
		}
	/*	p_ptr->dis_to_d = (d < p_ptr->dis_to_d) ?
		(((p_ptr->dis_to_d * 2) + (d * 1)) / 3) :
		(((p_ptr->dis_to_d * 1) + (d * 1)) / 2);	*/
	}

	/* dual-wielding gets a slight to-hit malus */
	if (p_ptr->dual_wield && p_ptr->dual_mode) p_ptr->to_h_melee = (p_ptr->to_h_melee * 17) / 20; /* 85% */



	/* Actual Modifier Bonuses (Un-inflate stat bonuses) */
	p_ptr->to_a += ((int)(adj_dex_ta[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->to_d_melee += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
#if 0 /* addition */
	p_ptr->to_h_melee += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
#else /* multiplication (percent) */
	p_ptr->to_h_melee = ((int)((p_ptr->to_h_melee * adj_dex_th[p_ptr->stat_ind[A_DEX]]) / 100));
#endif
//	p_ptr->to_h_melee += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128) / 2;
//	p_ptr->to_d += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
//	p_ptr->to_h += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
//	p_ptr->to_h += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);

	/* Displayed Modifier Bonuses (Un-inflate stat bonuses) */
	p_ptr->dis_to_a += ((int)(adj_dex_ta[p_ptr->stat_ind[A_DEX]]) - 128);
//	p_ptr->dis_to_d += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
//	p_ptr->dis_to_h += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
//	p_ptr->dis_to_h += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);

	/* Modify ranged weapon boni. DEX now very important for to_hit */
//	p_ptr->to_h_ranged += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128) / 2;
#if 0 /* addition */
	p_ptr->to_h_ranged += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
#else /* multiplication (percent) */
	if (p_ptr->to_h_ranged > 0) p_ptr->to_h_ranged = ((int)((p_ptr->to_h_ranged * adj_dex_th[p_ptr->stat_ind[A_DEX]]) / 100));
	else p_ptr->to_h_ranged = ((int)((p_ptr->to_h_ranged * 100) / adj_dex_th[p_ptr->stat_ind[A_DEX]]));
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
			case 0: p_ptr->shield_deflect = (p_ptr->shield_deflect * 11) / 10;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 5) / 10;
				p_ptr->to_d = (p_ptr->to_d * 5) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 5) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
			case 1: p_ptr->shield_deflect = (p_ptr->shield_deflect * 12) / 10;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 6) / 10;
				p_ptr->to_d = (p_ptr->to_d * 6) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 6) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
			case 2: p_ptr->shield_deflect = (p_ptr->shield_deflect * 13) / 10;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 7) / 10;
				p_ptr->to_d = (p_ptr->to_d * 7) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 7) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
			case 3: p_ptr->shield_deflect = (p_ptr->shield_deflect * 20) / 10;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 8) / 10;
				p_ptr->to_d = (p_ptr->to_d * 8) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 8) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
		}
 #endif
 #ifdef USE_PARRYING /* need parrying to make use of offensive stance */
		if (p_ptr->combat_stance == 2) switch (p_ptr->combat_stance_power) {
			case 0: p_ptr->weapon_parry = (p_ptr->weapon_parry * 2) / 10;
				p_ptr->dodge_level = (p_ptr->dodge_level * 2) / 10;
				break;
			case 1: p_ptr->weapon_parry = (p_ptr->weapon_parry * 3) / 10;
				p_ptr->dodge_level = (p_ptr->dodge_level * 3) / 10;
				break;
			case 2: p_ptr->weapon_parry = (p_ptr->weapon_parry * 4) / 10;
				p_ptr->dodge_level = (p_ptr->dodge_level * 4) / 10;
				break;
			case 3: p_ptr->weapon_parry = (p_ptr->weapon_parry * 5) / 10;
				p_ptr->dodge_level = (p_ptr->dodge_level * 5) / 10;
				break;
		}
  #ifdef ALLOW_SHIELDLESS_DEFENSIVE_STANCE
		else if ((p_ptr->combat_stance == 1) &&
		    (p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD)) switch (p_ptr->combat_stance_power) {
			case 0: p_ptr->weapon_parry = (p_ptr->weapon_parry * 11) / 10;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 5) / 10;
				p_ptr->to_d = (p_ptr->to_d * 5) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 5) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
			case 1: p_ptr->weapon_parry = (p_ptr->weapon_parry * 12) / 10;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 5) / 10;
				p_ptr->to_d = (p_ptr->to_d * 5) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 5) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
			case 2: p_ptr->weapon_parry = (p_ptr->weapon_parry * 13) / 10;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 5) / 10;
				p_ptr->to_d = (p_ptr->to_d * 5) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 5) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
			case 3: p_ptr->weapon_parry = (p_ptr->weapon_parry * 17) / 10;
				p_ptr->dis_to_d = (p_ptr->dis_to_d * 5) / 10;
				p_ptr->to_d = (p_ptr->to_d * 5) / 10;
				p_ptr->to_d_melee = (p_ptr->to_d_melee * 5) / 10;
				p_ptr->to_d_ranged = (p_ptr->to_d_ranged * 5) / 10;
				break;
		}
  #endif
 #endif
#endif




	/* Redraw plusses to hit/damage if necessary */
	if ((p_ptr->dis_to_h != old_dis_to_h) || (p_ptr->dis_to_d != old_dis_to_d))
	{
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
	p_ptr->skill_dis += (p_ptr->cp_ptr->x_dis * get_skill(p_ptr, SKILL_DISARM) / 10);

	/* Affect Skill -- magic devices (Level, by Class) */
	p_ptr->skill_dev += (p_ptr->cp_ptr->x_dev * get_skill(p_ptr, SKILL_DEVICE) / 10);
	p_ptr->skill_dev += adj_int_dev[p_ptr->stat_ind[A_INT]];

	/* Affect Skill -- saving throw (Level, by Class) */
	p_ptr->skill_sav += (p_ptr->cp_ptr->x_sav * p_ptr->lev / 10);

#if 0	// doh, it's all zero!!
	/* Affect Skill -- stealth (Level, by Class) */
	p_ptr->skill_stl += (p_ptr->cp_ptr->x_stl * get_skill(p_ptr, SKILL_STEALTH) / 10);

	/* Affect Skill -- search ability (Level, by Class) */
	p_ptr->skill_srh += (p_ptr->cp_ptr->x_srh * get_skill(p_ptr, SKILL_SNEAKINESS) / 10);

	/* Affect Skill -- search frequency (Level, by Class) */
	p_ptr->skill_fos += (p_ptr->cp_ptr->x_fos * get_skill(p_ptr, SKILL_SNEAKINESS) / 10);
#else
	/* Affect Skill -- stealth (Level, by Class) */
	p_ptr->skill_stl += (get_skill_scale(p_ptr, SKILL_STEALTH, p_ptr->cp_ptr->x_stl * 5)) + get_skill_scale(p_ptr, SKILL_STEALTH, 25);

	/* Affect Skill -- search ability (Level, by Class) */
	p_ptr->skill_srh += (get_skill_scale(p_ptr, SKILL_SNEAKINESS, p_ptr->cp_ptr->x_srh * 2));// + get_skill(p_ptr, SKILL_SNEAKINESS);

	/* Affect Skill -- search frequency (Level, by Class) */
	p_ptr->skill_fos += (get_skill_scale(p_ptr, SKILL_SNEAKINESS, p_ptr->cp_ptr->x_fos * 2));// + get_skill(p_ptr, SKILL_SNEAKINESS);


#endif	// 0

	/* Affect Skill -- combat (normal) (Level, by Class) */
        p_ptr->skill_thn += (p_ptr->cp_ptr->x_thn * (((7 * get_skill(p_ptr, SKILL_MASTERY)) + (3 * get_skill(p_ptr, SKILL_COMBAT))) / 10) / 10);

	/* Affect Skill -- combat (shooting) (Level, by Class) */
	p_ptr->skill_thb += (p_ptr->cp_ptr->x_thb * (((7 * get_skill(p_ptr, SKILL_ARCHERY)) + (3 * get_skill(p_ptr, SKILL_COMBAT))) / 10) / 10);

	/* Affect Skill -- combat (throwing) (Level, by Class) */
	p_ptr->skill_tht += (p_ptr->cp_ptr->x_thb * get_skill_scale(p_ptr, SKILL_COMBAT, 10));




	/* Knowledge in magic schools can give permanent extra boni */
	/* - SKILL_EARTH gives resistance in earthquake() */
	if (get_skill(p_ptr, SKILL_EARTH) >= 30) p_ptr->resist_shard = TRUE;
	if (get_skill(p_ptr, SKILL_AIR) >= 30) p_ptr->feather_fall = TRUE;
	if (get_skill(p_ptr, SKILL_AIR) >= 40) p_ptr->resist_pois = TRUE;
	if (get_skill(p_ptr, SKILL_AIR) >= 50) p_ptr->fly = TRUE;
	if (get_skill(p_ptr, SKILL_WATER) >= 30) p_ptr->resist_water = TRUE;
	if (get_skill(p_ptr, SKILL_WATER) >= 40) p_ptr->can_swim = TRUE;
	if (get_skill(p_ptr, SKILL_WATER) >= 50) p_ptr->immune_water = TRUE;
	if (get_skill(p_ptr, SKILL_FIRE) >= 30) p_ptr->resist_fire = TRUE;
	if (get_skill(p_ptr, SKILL_FIRE) >= 50) p_ptr->immune_fire = TRUE;
	if (get_skill(p_ptr, SKILL_MANA) >= 40) p_ptr->resist_mana = TRUE;
	if (get_skill(p_ptr, SKILL_CONVEYANCE) >= 30) p_ptr->res_tele = TRUE;
	if (get_skill(p_ptr, SKILL_DIVINATION) >= 50) p_ptr->auto_id = TRUE;
	if (get_skill(p_ptr, SKILL_NATURE) >= 30) p_ptr->regenerate = TRUE;
	if (get_skill(p_ptr, SKILL_NATURE) >= 30) p_ptr->pass_trees = TRUE;
	if (get_skill(p_ptr, SKILL_NATURE) >= 40) p_ptr->can_swim = TRUE;
	/* - SKILL_MIND also helps to reduce hallucination time in set_image() */
	if (get_skill(p_ptr, SKILL_MIND) >= 40 && !p_ptr->reduce_insanity) p_ptr->reduce_insanity = 1;
	if (get_skill(p_ptr, SKILL_MIND) >= 50) p_ptr->reduce_insanity = 2;
	if (get_skill(p_ptr, SKILL_TEMPORAL) >= 50) p_ptr->resist_time = TRUE;
	if (get_skill(p_ptr, SKILL_UDUN) >= 30) p_ptr->res_tele = TRUE;
	if (get_skill(p_ptr, SKILL_UDUN) >= 40) p_ptr->hold_life = TRUE;
	if (get_skill(p_ptr, SKILL_META) >= 20) p_ptr->skill_sav += get_skill(p_ptr, SKILL_META) - 20;
	/* - SKILL_HOFFENSE gives slay mods in brand/slay function tot_dam_aux() */
	/* - SKILL_HDEFENSE gives auto protection-from-evil */
//	if (get_skill(p_ptr, SKILL_HDEFENSE) >= 40) { p_ptr->resist_lite = TRUE; p_ptr->resist_dark = TRUE; }
	/* - SKILL_HCURING gives extra high regeneration in regen function, and reduces various effects */
//	if (get_skill(p_ptr, SKILL_HCURING) >= 50) p_ptr->reduce_insanity = 1;
	/* - SKILL_HSUPPORT renders DG/TY_CURSE effectless and prevents hunger */



	/* Take note when "heavy bow" changes */
	if (p_ptr->old_heavy_shoot != p_ptr->heavy_shoot)
	{
		/* Message */
		if (p_ptr->heavy_shoot)
		{
			msg_print(Ind, "\377oYou have trouble wielding such a heavy bow.");
		}
		else if (p_ptr->inventory[INVEN_BOW].k_idx)
		{
			msg_print(Ind, "\377gYou have no trouble wielding your bow.");
		}
		else
		{
			msg_print(Ind, "\377gYou feel relieved to put down your heavy bow.");
		}

		/* Save it */
		p_ptr->old_heavy_shoot = p_ptr->heavy_shoot;
	}


	/* Take note when "heavy weapon" changes */
	if (p_ptr->old_heavy_wield != p_ptr->heavy_wield)
	{
		/* Message */
		if (p_ptr->heavy_wield)
		{
			msg_print(Ind, "\377oYou have trouble wielding such a heavy weapon.");
		}
		else if (p_ptr->inventory[INVEN_WIELD].k_idx || /* dual-wield */
		    (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD))
		{
			msg_print(Ind, "\377gYou have no trouble wielding your weapon.");
		}
		else
		{
			msg_print(Ind, "\377gYou feel relieved to put down your heavy weapon.");
		}

		/* Save it */
		p_ptr->old_heavy_wield = p_ptr->heavy_wield;
	}

	/* Take note when "heavy shield" changes */
	if (p_ptr->old_heavy_shield != p_ptr->heavy_shield)
	{
		/* Message */
		if (p_ptr->heavy_shield)
		{
			msg_print(Ind, "\377oYou have trouble wielding such a heavy shield.");
		}
		else if (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD) /* dual-wielders */
		{
			msg_print(Ind, "\377gYou have no trouble wielding your shield.");
		}
		else
		{
			msg_print(Ind, "\377gYou feel relieved to put down your heavy shield.");
		}

		/* Save it */
		p_ptr->old_heavy_shield = p_ptr->heavy_shield;
	}

	/* Take note when "illegal weapon" changes */
	if (p_ptr->old_icky_wield != p_ptr->icky_wield)
	{
		/* Message */
		if (p_ptr->icky_wield)
		{
			msg_print(Ind, "\377oYou do not feel comfortable with your weapon.");
		}
		else if (p_ptr->inventory[INVEN_WIELD].k_idx || /* dual-wield */
		    (p_ptr->inventory[INVEN_ARM].k_idx && p_ptr->inventory[INVEN_ARM].tval != TV_SHIELD))
		{
			msg_print(Ind, "\377gYou feel comfortable with your weapon.");
		}
		else
		{
			msg_print(Ind, "\377gYou feel more comfortable after removing your weapon.");
		}

		/* Save it */
		p_ptr->old_icky_wield = p_ptr->icky_wield;
	}

	/* Take note when "illegal weapon" changes */
	if (p_ptr->old_awkward_wield != p_ptr->awkward_wield)
	{
		/* Message */
		if (p_ptr->awkward_wield)
		{
			msg_print(Ind, "\377oYou find it hard to fight with your weapon and shield.");
		}
		else if (p_ptr->inventory[INVEN_WIELD].k_idx)
		{
			msg_print(Ind, "\377gYou feel comfortable with your weapon.");
		}
		else if (!p_ptr->inventory[INVEN_ARM].k_idx)
		{
			msg_print(Ind, "\377gYou feel more dexterous after removing your shield.");
		}

		/* Save it */
		p_ptr->old_awkward_wield = p_ptr->awkward_wield;
	}

	if (p_ptr->old_easy_wield != p_ptr->easy_wield)
	{
		/* suppress message if we're heavy-wielding */
		if (!p_ptr->heavy_wield) {
			/* Message */
			if (p_ptr->easy_wield)
			{
				if (get_skill(p_ptr, SKILL_DUAL)) /* dual-wield */
					msg_print(Ind, "\377wWithout shield or secondary weapon, your weapon feels especially easy to swing.");
				else
					msg_print(Ind, "\377wWithout shield, your weapon feels especially easy to swing.");
			}
			else if (p_ptr->inventory[INVEN_WIELD].k_idx)
			{
/*					msg_print(Ind, "\377wWith shield, your weapon feels normally comfortable.");
				this above line does also show if you don't equip a shield but just switch from a
				may_2h to a normal weapon, hence confusing. */
				msg_print(Ind, "\377wYour weapon feels comfortable as usual.");
			}
		}
		/* Save it */
		p_ptr->old_easy_wield = p_ptr->easy_wield;
	}

	/* Take note when "illegal weapon" changes */
	if (p_ptr->old_awkward_shoot != p_ptr->awkward_shoot)
	{
		/* Message */
		if (p_ptr->awkward_shoot)
		{
			if (p_ptr->inventory[INVEN_ARM].tval == TV_SHIELD)
				msg_print(Ind, "\377oYou find it hard to aim while carrying your shield.");
			else /* maybe leave awkward_shoot at FALSE if secondary slot isn't a shield! */
				msg_print(Ind, "\377oYou find it hard to aim while dual-wielding weapons.");
		}
		else if (p_ptr->inventory[INVEN_BOW].k_idx)
		{
			msg_print(Ind, "\377gYou find it easy to aim.");
		}

		/* Save it */
		p_ptr->old_awkward_shoot = p_ptr->awkward_shoot;
	}




	/* resistance to fire cancel sensibility to fire */
	if(p_ptr->resist_fire || p_ptr->oppose_fire || p_ptr->immune_fire)
		p_ptr->suscep_fire = FALSE;
	/* resistance to cold cancel sensibility to cold */
	if(p_ptr->resist_cold || p_ptr->oppose_cold || p_ptr->immune_cold)
		p_ptr->suscep_cold = FALSE;
	/* resistance to electricity cancel sensibility to fire */
	if(p_ptr->resist_elec || p_ptr->oppose_elec || p_ptr->immune_elec)
		p_ptr->suscep_elec = FALSE;
	/* resistance to acid cancel sensibility to fire */
	if(p_ptr->resist_acid || p_ptr->oppose_acid || p_ptr->immune_acid)
		p_ptr->suscep_acid = FALSE;
	/* resistance to light cancels sensibility to light */
	if(p_ptr->resist_lite) p_ptr->suscep_lite = FALSE;


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
//				reduction = p_ptr->pspeed - 120;
			if (factor1 >= (p_ptr->pspeed - 110) + 10) {
				factor1 = (factor * 10) / (p_ptr->pspeed - 110) + 10;
				Players[Ind2]->pspeed = 110 - factor1 + 10;
			}
		} else { /* player 2 is faster or equal speed */
			if (Players[Ind2]->pspeed > 120) /* top (cur atm) speed for moving during blood bond */
				Players[Ind2]->pspeed = 120;
//				reduction = Players[Ind2]->pspeed - 120;
		}
	}
#endif


	/* Extract the current weight (in tenth pounds) */
	w = p_ptr->total_weight;

	/* Extract the "weight limit" (in tenth pounds) */
	i = weight_limit(Ind);

	/* XXX XXX XXX Apply "encumbrance" from weight */
	if (w > i/2) {
		/* protect pspeed from uberflow O_o */
//		if (w > 61500) p_ptr->pspeed = 10; /* roughly ;-p */
		if (w > 70000) p_ptr->pspeed = 10; /* roughly ;-p */
		else p_ptr->pspeed -= ((w - (i/2)) / (i / 10));
	}

	/* Display the speed (if needed) */
	if (p_ptr->pspeed != old_speed) p_ptr->redraw |= (PR_SPEED);


	/* swapping in AUTO_ID items will instantly ID inventory and equipment */
	if (p_ptr->auto_id && !old_auto_id)
		for (i = 0; i < INVEN_TOTAL; i++) {
			o_ptr = &p_ptr->inventory[i];
			object_aware(Ind, o_ptr);
			object_known(o_ptr);
		}


/* -------------------- Limits -------------------- */

#ifdef FUNSERVER
	/* Limit AC?.. */
	if (!is_admin(p_ptr)) {
		if ((p_ptr->to_a > 400) && (!p_ptr->total_winner)) p_ptr->to_a = 400; /* Lebohaum / Illuvatar bonus */
		if ((p_ptr->to_a > 300) && (p_ptr->total_winner)) p_ptr->to_a = 300;
		if (p_ptr->ac > 100) p_ptr->ac = 100;
	}
#endif

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



	/* XXX - Always resend skills */
	p_ptr->redraw |= (PR_SKILLS);
	/* also redraw encumberment status line */
	p_ptr->redraw |= (PR_ENCUMBERMENT);


	/* warning messages, mostly for newbies */
	if (p_ptr->num_blow == 1 && old_num_blow > 1 && p_ptr->warning_bpr == 0 &&
	    p_ptr->inventory[INVEN_WIELD].k_idx) {
		p_ptr->warning_bpr = 1;
		msg_print(Ind, "\374\377yWARNING! Your number of melee attacks per round has just dropped to ONE.");
		msg_print(Ind, "\374\377y    If you rely on melee combat, it is strongly advised to try and");
		msg_print(Ind, "\374\377y    get AT LEAST TWO blows/round (press shift+c to check the #).");
		msg_print(Ind, "\374\377yPossible reasons are: Weapon is too heavy; too little STR or DEX; You");
		msg_print(Ind, "\374\377y    just equipped too heavy armour or a shield - depending on your class.");
		msg_print(Ind, "\374\377y    Also, some classes can dual-wield to get an extra blow/round.");
	}
	if (p_ptr->max_plv == 1 &&
	    p_ptr->num_blow == 1 && old_num_blow == 1 && p_ptr->warning_bpr3 == 2 &&
	    /* and don't spam Martial Arts users ;) */
	    p_ptr->inventory[INVEN_WIELD].k_idx) {
		p_ptr->warning_bpr2 = p_ptr->warning_bpr3 = 1;
		msg_print(Ind, "\374\377yWARNING! You can currently perform only ONE melee attack per round.");
		msg_print(Ind, "\374\377y    If you rely on melee combat, it is strongly advised to try and");
		msg_print(Ind, "\374\377y    get AT LEAST TWO blows/round (press shift+c to check the #).");
		msg_print(Ind, "\374\377yPossible reasons are: Weapon is too heavy; too little STR or DEX; You");
		msg_print(Ind, "\374\377y    just equipped too heavy armour or a shield - depending on your class.");
		msg_print(Ind, "\374\377y    Also, some classes can dual-wield to get an extra blow/round.");
	}
}



/*
 * Handle "p_ptr->notice"
 */
void notice_stuff(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Notice stuff */
	if (!p_ptr->notice) return;


	/* Combine the pack */
	if (p_ptr->notice & PN_COMBINE)
	{
		p_ptr->notice &= ~(PN_COMBINE);
		combine_pack(Ind);
	}

	/* Reorder the pack */
	if (p_ptr->notice & PN_REORDER)
	{
		p_ptr->notice &= ~(PN_REORDER);
		reorder_pack(Ind);
	}
}


/*
 * Handle "p_ptr->update"
 */
void update_stuff(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Update stuff */
	if (!p_ptr->update) return;

	/* This should only be sent once. This data
	   does not change in runtime */
	if (p_ptr->update & PU_SKILL_INFO)
        {
                int i;

		calc_techniques(Ind);

		p_ptr->update &= ~(PU_SKILL_INFO);
                for (i = 1; i < MAX_SKILLS; i++)
                {
                        if (s_info[i].name)
                        {
				Send_skill_init(Ind, i);
                        }
                }
	}

	if (p_ptr->update & PU_SKILL_MOD)
        {
                int i;

		p_ptr->update &= ~(PU_SKILL_MOD);
                for (i = 1; i < MAX_SKILLS; i++)
                {
                        if (s_info[i].name && p_ptr->s_info[i].touched)
                        {
                                Send_skill_info(Ind, i);
				p_ptr->s_info[i].touched = FALSE;
                        }
                }
		Send_skill_points(Ind);
	}

	if (p_ptr->update & PU_BONUS)
	{
		p_ptr->update &= ~(PU_BONUS);
		/* Take off what is no more usable, BEFORE calculating boni */
		do_takeoff_impossible(Ind);
		calc_boni(Ind);
	}

	if (p_ptr->update & PU_TORCH)
	{
		p_ptr->update &= ~(PU_TORCH);
		calc_torch(Ind);
	}

	if (p_ptr->update & PU_HP)
	{
		p_ptr->update &= ~(PU_HP);
		calc_hitpoints(Ind);
	}

	if (p_ptr->update & (PU_SANITY))
	{
		p_ptr->update &= ~(PU_SANITY);
		calc_sanity(Ind);
	}

	if (p_ptr->update & PU_MANA)
	{
		p_ptr->update &= ~(PU_MANA);
		calc_mana(Ind);
	}

	/* Character is not ready yet, no screen updates */
	/*if (!character_generated) return;*/

	/* Character has changed depth very recently, no screen updates */
	if (p_ptr->new_level_flag) return;

	if (p_ptr->update & PU_UN_LITE)
	{
		p_ptr->update &= ~(PU_UN_LITE);
		forget_lite(Ind);
	}

	if (p_ptr->update & PU_UN_VIEW)
	{
		p_ptr->update &= ~(PU_UN_VIEW);
		forget_view(Ind);
	}


	if (p_ptr->update & PU_VIEW)
	{
		p_ptr->update &= ~(PU_VIEW);
		update_view(Ind);
	}

	if (p_ptr->update & PU_LITE)
	{
		p_ptr->update &= ~(PU_LITE);
		update_lite(Ind);
	}


	if (p_ptr->update & PU_FLOW)
	{
		p_ptr->update &= ~(PU_FLOW);
		update_flow();
	}


	if (p_ptr->update & PU_DISTANCE)
	{
		p_ptr->update &= ~(PU_DISTANCE);
		p_ptr->update &= ~(PU_MONSTERS);
		update_monsters(TRUE);
		update_players();
	}

	if (p_ptr->update & PU_MONSTERS)
	{
		p_ptr->update &= ~(PU_MONSTERS);
		update_monsters(FALSE);
		update_players();
	}

	if(p_ptr->update & PU_LUA){
		/* update the client files */
		p_ptr->update &= ~(PU_LUA);
                exec_lua(Ind, "update_client()");
	}
}


/*
 * Handle "p_ptr->redraw"
 */
void redraw_stuff(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Redraw stuff */
	if (!p_ptr->redraw) return;


	/* Character is not ready yet, no screen updates */
	/*if (!character_generated) return;*/

	/* Hack -- clear the screen */
	if (p_ptr->redraw & PR_WIPE)
	{
		p_ptr->redraw &= ~PR_WIPE;
		msg_print(Ind, NULL);
	}


	if (p_ptr->redraw & PR_MAP)
	{
#ifdef USE_SOUND_2010
		/* hack: update music, if we're mind-linked to someone */
		int Ind2;
		player_type *p_ptr2;
		u32b f;

		if ((Ind2 = get_esp_link(Ind, LINKF_VIEW, &p_ptr2))) {
			/* hack: temporarily remove the dedicated mind-link flag, to allow Send_music() */
			f = Players[Ind2]->esp_link_flags;
			Players[Ind2]->esp_link_flags &= ~LINKF_VIEW_DEDICATED;
//s_printf("xtra1-hack_music (%s, %s)\n", Players[Ind2]->name, Players[Ind]->name);
			Send_music(Ind2, Players[Ind]->music_current);
			Players[Ind2]->esp_link_flags = f;
		} else handle_music(Ind); /* restore music after a mind-link has been broken */
#endif

		p_ptr->redraw &= ~(PR_MAP);
		prt_map(Ind);

#ifdef CLIENT_SIDE_WEATHER
		/* hack: update weather if it was just a panel-change
		   (ie level-sector) and not a level-change.
		   Note: this could become like PR_WEATHER_PANEL,
		   however, PR_ flags are already in use completely atm. */
		if (p_ptr->panel_changed) player_weather(Ind, FALSE, FALSE, TRUE);
#endif
		p_ptr->panel_changed = FALSE;
	}


	if (p_ptr->redraw & PR_BASIC)
	{
		p_ptr->redraw &= ~(PR_BASIC);
		p_ptr->redraw &= ~(PR_MISC | PR_TITLE | PR_STATS);
		p_ptr->redraw &= ~(PR_LEV | PR_EXP | PR_GOLD);
		p_ptr->redraw &= ~(PR_ARMOR | PR_HP | PR_MANA | PR_STAMINA);
		p_ptr->redraw &= ~(PR_DEPTH | PR_HEALTH);
		prt_frame_basic(Ind);
	}

	if (p_ptr->redraw & PR_ENCUMBERMENT)
	{
		p_ptr->redraw &= ~(PR_ENCUMBERMENT);
		prt_encumberment(Ind);
	}

	if (p_ptr->redraw & PR_MISC)
	{
		p_ptr->redraw &= ~(PR_MISC);
		Send_char_info(Ind, p_ptr->prace, p_ptr->pclass, p_ptr->male, p_ptr->mode);
	}

	if (p_ptr->redraw & PR_TITLE)
	{
		p_ptr->redraw &= ~(PR_TITLE);
		prt_title(Ind);
	}

	if (p_ptr->redraw & PR_LEV)
	{
		p_ptr->redraw &= ~(PR_LEV);
		prt_level(Ind);
	}

	if (p_ptr->redraw & PR_EXP)
	{
		p_ptr->redraw &= ~(PR_EXP);
		prt_exp(Ind);
	}

	if (p_ptr->redraw & PR_STATS)
	{
		p_ptr->redraw &= ~(PR_STATS);
		prt_stat(Ind, A_STR);
		prt_stat(Ind, A_INT);
		prt_stat(Ind, A_WIS);
		prt_stat(Ind, A_DEX);
		prt_stat(Ind, A_CON);
		prt_stat(Ind, A_CHR);
	}

	if (p_ptr->redraw & PR_ARMOR)
	{
		p_ptr->redraw &= ~(PR_ARMOR);
		prt_ac(Ind);
	}

	if (p_ptr->redraw & PR_HP)
	{
		p_ptr->redraw &= ~(PR_HP);
		prt_hp(Ind);
	}

	if (p_ptr->redraw & PR_STAMINA)
	{
		p_ptr->redraw &= ~(PR_STAMINA);
		prt_stamina(Ind);
	}

	if(p_ptr->redraw & PR_SANITY)
	{
		p_ptr->redraw &= ~(PR_SANITY);
#ifdef SHOW_SANITY
		prt_sanity(Ind);
#endif
	}

	if (p_ptr->redraw & PR_MANA)
	{
		p_ptr->redraw &= ~(PR_MANA);
		prt_sp(Ind);
	}

	if (p_ptr->redraw & PR_GOLD)
	{
		p_ptr->redraw &= ~(PR_GOLD);
		prt_gold(Ind);
	}

	if (p_ptr->redraw & PR_DEPTH)
	{
		p_ptr->redraw &= ~(PR_DEPTH);
		prt_depth(Ind);
	}

	if (p_ptr->redraw & PR_HEALTH)
	{
		p_ptr->redraw &= ~(PR_HEALTH);
		health_redraw(Ind);
	}

	if (p_ptr->redraw & PR_HISTORY)
	{
		p_ptr->redraw &= ~(PR_HISTORY);
		prt_history(Ind);
	}

	if (p_ptr->redraw & PR_VARIOUS)
	{
		p_ptr->redraw &= ~(PR_VARIOUS);
		prt_various(Ind);
	}

	if (p_ptr->redraw & PR_PLUSSES)
	{
		p_ptr->redraw &= ~(PR_PLUSSES);
		prt_plusses(Ind);
	}

	if (p_ptr->redraw & PR_SKILLS)
	{
		p_ptr->redraw &= ~(PR_SKILLS);
		prt_skills(Ind);
	}

	if (p_ptr->redraw & PR_EXTRA)
	{
		p_ptr->redraw &= ~(PR_EXTRA);
		p_ptr->redraw &= ~(PR_CUT | PR_STUN);
		p_ptr->redraw &= ~(PR_HUNGER);
		p_ptr->redraw &= ~(PR_BLIND | PR_CONFUSED);
		p_ptr->redraw &= ~(PR_AFRAID | PR_POISONED);
		p_ptr->redraw &= ~(PR_STATE | PR_SPEED | PR_STUDY);
		prt_frame_extra(Ind);
		prt_extra_status(Ind);
	}

	if (p_ptr->redraw & PR_CUT)
	{
		p_ptr->redraw &= ~(PR_CUT);
		prt_cut(Ind);
	}

	if (p_ptr->redraw & PR_STUN)
	{
		p_ptr->redraw &= ~(PR_STUN);
		prt_stun(Ind);
	}

	if (p_ptr->redraw & PR_HUNGER)
	{
		p_ptr->redraw &= ~(PR_HUNGER);
		prt_hunger(Ind);
	}

	if (p_ptr->redraw & PR_BLIND)
	{
		p_ptr->redraw &= ~(PR_BLIND);
		prt_blind(Ind);
	}

	if (p_ptr->redraw & PR_CONFUSED)
	{
		p_ptr->redraw &= ~(PR_CONFUSED);
		prt_confused(Ind);
	}

	if (p_ptr->redraw & PR_AFRAID)
	{
		p_ptr->redraw &= ~(PR_AFRAID);
		prt_afraid(Ind);
	}

	if (p_ptr->redraw & PR_POISONED)
	{
		p_ptr->redraw &= ~(PR_POISONED);
		prt_poisoned(Ind);
	}

	if (p_ptr->redraw & PR_STATE)
	{
		p_ptr->redraw &= ~(PR_STATE);
		prt_state(Ind);
		prt_extra_status(Ind);
	}

	if (p_ptr->redraw & PR_SPEED)
	{
		p_ptr->redraw &= ~(PR_SPEED);
		prt_speed(Ind);
	}

	if (p_ptr->redraw & PR_STUDY)
	{
		p_ptr->redraw &= ~(PR_STUDY);
		prt_study(Ind);
	}
}


/*
 * Handle "p_ptr->window"
 */
void window_stuff(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Window stuff */
	if (!p_ptr->window) return;

	/* Display inventory */
	if (p_ptr->window & PW_INVEN)
	{
		p_ptr->window &= ~(PW_INVEN);
		fix_inven(Ind);
	}

	/* Display equipment */
	if (p_ptr->window & PW_EQUIP)
	{
		p_ptr->window &= ~(PW_EQUIP);
		fix_equip(Ind);
	}

	/* Display player */
	if (p_ptr->window & PW_PLAYER)
	{
		p_ptr->window &= ~(PW_PLAYER);
		fix_player(Ind);
	}

	/* Display overhead view */
	if (p_ptr->window & PW_MESSAGE)
	{
		p_ptr->window &= ~(PW_MESSAGE);
		fix_message(Ind);
	}

	/* Display overhead view */
	if (p_ptr->window & PW_OVERHEAD)
	{
		p_ptr->window &= ~(PW_OVERHEAD);
		fix_overhead(Ind);
	}

	/* Display monster recall */
	if (p_ptr->window & PW_MONSTER)
	{
		p_ptr->window &= ~(PW_MONSTER);
		fix_monster(Ind);
	}
}


/*
 * Handle "p_ptr->update" and "p_ptr->redraw" and "p_ptr->window"
 */
void handle_stuff(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Hack -- delay updating */
	if (p_ptr->new_level_flag) return;

	/* Update stuff */
	if (p_ptr->update) update_stuff(Ind);

	/* Redraw stuff */
	if (p_ptr->redraw) redraw_stuff(Ind);

	/* Window stuff */
	if (p_ptr->window) window_stuff(Ind);
}


/*
 * Start a global_event (highlander tournament etc.) - C. Blue
 * IMPORTANT: Ind may be 0 if this is called from custom.lua !
 */
int start_global_event(int Ind, int getype, char *parm)
{
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
//	ge->start_turn = turn + 1; /* +1 is a small hack, to prevent double-announcement */
	/* hack - synch start_turn to cfg.fps, since process_global_events is only called every
	   (turn % cfg.fps == 0), and its announcement timer checks will fail otherwise */
	ge->start_turn += (cfg.fps - (turn % cfg.fps));
	time(&ge->started);
	ge->paused = FALSE;
	ge->paused_turns = 0; /* counter for real turns the event "missed" while being paused */
	for (i = 0; i < 64; i++) {/* yeah I could use WIPE.. */
	        ge->state[i]=0;
		ge->participant[i]=0;
		ge->extra[i]=0;
	}
	ge->end_turn=0;
	ge->ending=0;
	strcpy(ge->title, "(UNTITLED EVENT)");
	for (i = 0; i<6; i++) strcpy(ge->description[i], "");
	strcpy(ge->description[0], "(NO DESCRIPTION AVAILABLE)");
	ge->hidden=FALSE;
	ge->min_participants=0; /* no minimum */
	ge->limited=0; /* no maximum */
	ge->cleanup=0; /* no cleaning up needed so far (for when the event ends) */

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
		ge->end_turn = ge->start_turn + cfg.fps * 60 * 90 ; /* 90 minutes max. duration,
								most of the time is just for announcing it
								so players will sign on via /evsign <n> */
		switch(rand_int(2)) { /* Determine terrain type! */
		case 0: ge->extra[2] = WILD_WASTELAND; break;
//		case 1: ge->extra[2] = WILD_SWAMP; break; swamp maybe too annoying
		case 1: ge->extra[2] = WILD_GRASSLAND; break;
		}
		switch(rand_int(3)) { /* Load premade layout? (Arenas) */
		case 0: ge->extra[4] = 1; break;
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
		strcpy(ge->description[1], format(" '/evsign %d <Monster Name>' and you'll have a chance to challenge  ", n+1));
		strcpy(ge->description[2], " it for an illusion death match in Bree's upper training tower floor.  ");
		strcpy(ge->description[3], " Neither the monster nor you will really die in person, just illusions ");
		strcpy(ge->description[4], " of you, created by the wizards of 'Arena Monster Challenge (tm)' will ");
		strcpy(ge->description[5], " actually do the fighting. For the duration of the spell it will seem  ");
		strcpy(ge->description[6], " completely real to you though, and you can even use and consume items!");
//		strcpy(ge->description[7], " (Note: Some creatures might be beyond the wizards' abilities.)");
		strcpy(ge->description[7], format(" (Example: '/evsign %d black orc vet' gets you a veteran archer!)", n+1));
		ge->end_turn = ge->start_turn + cfg.fps * 60 * 30 ; /* 30 minutes max. duration, insta-start */
#if 0
		switch(rand_int(2)) { /* Determine terrain type! */
		case 0: ge->extra[2] = WILD_WASTELAND; break;
//		case 1: ge->extra[2] = WILD_SWAMP; break; swamp maybe too annoying
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
	}

	/* Fix limits */
	if (ge->limited > MAX_GE_PARTICIPANTS) ge->limited = MAX_GE_PARTICIPANTS;

	/* give feedback to the creator; tell all admins if this was called from a script */
/*	if (Ind) {
		msg_format(Ind, "Reward item level = %d.", ge->extra[0]);
		msg_format(Ind, "Created new event #%d of type %d parms='%s'.", n+1, getype, parm);
	} else */{
		for (i = 1;i <= NumPlayers; i++)
		    if (is_admin(Players[i])) {
			msg_format(i, "Reward item level = %d.", ge->extra[0]);
			msg_format(i, "Created new event #%d of type %d parms='%s'.", n+1, getype, parm);
		}
	}
	s_printf("%s EVENT_CREATE: #%d of type %d parms='%s'\n", showtime(), n+1, getype, parm);

	/* extra announcement if announcement time isn't the usual multiple of announcement intervals */
#if 1 /* this is ok, and leaving it for now */
	if ((ge->announcement_time % GE_ANNOUNCE_INTERVAL) && (ge->announcement_time % 240)) announce_global_event(n);
#else /* this would be even finer, but actually it's not needed as long as start_global_event() is called with sensible announcement_time :) */
/* note that this else-branch is missing the 240-check atm */
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
void stop_global_event(int Ind, int n){
	global_event_type *ge = &global_event[n];
	msg_format(Ind, "Wiping event #%d of type %d.", n+1, ge->getype);
	s_printf("%s EVENT_STOP: #%d of type %d\n", showtime(), n+1, ge->getype);
	if (ge->getype) msg_broadcast_format(0, "\377y[Event '%s' (%d) was cancelled.]", ge->title, n+1);
#if 0
	ge->getype = GE_NONE;
	for (i = 1; i <= NumPlayers; i++) Players[i]->global_event_type[n] = GE_NONE;
#else /* we really need to call the clean-up instead of just GE_NONEing it: */
	ge->announcement_time = -1; /* enter the processing phase, */
	ge->state[0] = 255; /* ..and process clean-up! */
#endif
	return;
}

void announce_global_event(int ge_id) {
	global_event_type *ge = &global_event[ge_id];
	int time_left = ge->announcement_time - ((turn - ge->start_turn) / cfg.fps);

	/* display minutes, if at least 120s left */
	if (time_left >= 120) msg_broadcast_format(0, "\377W[%s (%d) starts in %d minutes]", ge->title, ge_id+1, time_left / 60);
	/* otherwise just seconds */
	else msg_broadcast_format(0, "\377W[%s (%d) starts in %d seconds!]", ge->title, ge_id+1, time_left);

	/* display additional commands on first advertisement */
	if (ge->first_announcement) {
		msg_broadcast_format(0, "\377WType '/evinfo %d' and '/evsign %d' to learn more or to sign up.", ge_id+1, ge_id+1);
		ge->first_announcement = FALSE;
	}
}

/*
 * A player signs on for a global_event - C. Blue
 */
void global_event_signup(int Ind, int n, cptr parm){
	int i, p, max_p, r_found = 0, r_found_len = 99;
	bool r_impossible = FALSE, fake_signup = FALSE;
	global_event_type *ge = &global_event[n];
	player_type *p_ptr = Players[Ind];
	char c[80], parm2[80], *cp, *p2p = parm2, parm2e[80];
	/* for ego monsters: */
	int re_found = 0, re_found_len = 99;
	bool re_impossible = FALSE;
	char ce[80], *cep;
	char parm_log[60] = "";
#ifdef GE_ARENA_ALLOW_EGO
	r_found_len = 0; /* check must go the other way round */
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
		if ((p_ptr->wpos.wx != cfg.town_x || p_ptr->wpos.wy != cfg.town_y || p_ptr->wpos.wz != 0) &&
		    (p_ptr->wpos.wx != WPOS_ARENA_X || p_ptr->wpos.wy != WPOS_ARENA_Y || p_ptr->wpos.wz != WPOS_ARENA_Z)) {
			msg_print(Ind, "\377yYou have to be in Bree or in the arena to sign up for this event!");
			return;
		}
		if ((parm == NULL) || !strlen(parm)) {
			msg_format(Ind, "\377yYou have to specify a monster name:  /evsign %d monstername", n+1);
			return;
		}
		strcpy(parm2, parm);
		p2p = parm2;
		while (*p2p) {*p2p = tolower(*p2p); p2p++;}

	        /* Scan the monster races */
	        for (i = 1; i < MAX_R_IDX; i++) {
			strcpy(c, r_info[i].name + r_name);
			cp = c;
			while (*cp) {*cp = tolower(*cp); cp++;}
			if (!strlen(c)) continue;

#ifdef GE_ARENA_ALLOW_EGO /* parse-strategy: egos w/ non pre-scanning require exact monster name input */
	                if (strstr(parm2, c)) {
#else /* old tolerant way, without allowing egos */
	                if (strstr(c, parm2)) {
#endif
				if (!((r_info[i].flags1 & RF1_UNIQUE) ||
				    (r_info[i].flags7 & RF7_NEVER_ACT) ||
				    (r_info[i].flags7 & RF7_PET) ||
				    (r_info[i].flags7 & RF7_NEUTRAL) ||
				    (r_info[i].flags7 & RF7_FRIENDLY) ||
				    (r_info[i].flags8 & RF8_JOKEANGBAND) ||
				    (r_info[i].rarity == 255))) {
#ifdef GE_ARENA_ALLOW_EGO
					if (r_found_len < (int)strlen(c)) {
#else
					if (r_found_len > (int)strlen(c)) {
#endif
						r_found_len = strlen(c);
						r_found = i;
					}
				} else r_impossible = TRUE;
			}
		}

		if (r_found) {
			/* also scan for ego monster? */
#ifdef GE_ARENA_ALLOW_EGO
			/* cut out the monster race name before ego processing (eg 'sorceror' -> potential probs) */
			strcpy(c, r_info[r_found].name + r_name);
			cp = c;
			while (*cp) {*cp = tolower(*cp); cp++;}
			strncpy(parm2e, parm2, strstr(parm2, c) - parm2);
			/* weirdness, required to fix string termination like this: - C. Blue */
			parm2e[strstr(parm2, c) - parm2] = 0;
			strcat(parm2e, strstr(parm2, c) + strlen(c));
			/* trim spaces just to be sure */
			while (parm2e[0] == 32) strcpy(parm2e, parm2e + 1); /* at beginning (left side) */
			while (parm2e[strlen(parm2e) - 1] == 32) parm2e[strlen(parm2e) - 1] = 0; /* at the end */

			if (strlen(parm2e))
		        for (i = 1; i < MAX_RE_IDX; i++) {
				strcpy(ce, re_info[i].name + re_name);
				cep = ce;
				while (*cep) {*cep = tolower(*cep); cep++;}
#if 1
		                if (strstr(ce, parm2e)) {
#else /* make it more precise instead? */
/* note that this needs you to inverse re_found_len < > 0/99 check! */
		                if (strstr(parm2e, ce)) {
#endif
					if (mego_ok(r_found, i)) {
						if (re_found_len > (int)strlen(ce)) {
							re_found_len = strlen(ce);
							re_found = i;
						}
					} else re_impossible = TRUE;
				}
			}

			if (re_found) {
				/* do nothing (announcing will be done in actual event processing) */
			} else if (re_impossible) {
				msg_print(Ind, "\377ySorry, that ego creature is beyond the wizards' abilities.");
				return;
			}
			if (!re_found && strlen(parm2e)) {
				msg_print(Ind, "\377yCouldn't find matching ego power, going with base version..");
			}
#endif

			ge->extra[1] = r_found;
#ifndef GE_ARENA_ALLOW_EGO
			monster_race_desc(0, c, r_found, 0x88);
			msg_broadcast_format(0, "\376\377c** %s challenges %s! **", p_ptr->name, c);
#else
			ge->extra[3] = re_found;
			ge->extra[5] = Ind;
#endif

			/* rebuild parm from definite ego+monster name */
			if (re_found) {
				strcpy(parm_log, re_name + re_info[re_found].name);
				strcat(parm_log, " ");
				strcat(parm_log, r_name + r_info[r_found].name);
			} else {
				strcpy(parm_log, r_name + r_info[r_found].name);
			}

			fake_signup = TRUE;
			break;
		} else if (r_impossible) {
			msg_print(Ind, "\377ySorry, that creature is beyond the wizards' abilities.");
			return;
		}

//		msg_format(Ind, "\377yCouldn't find that monster, do upper/lowercase letters match?", n);
		msg_format(Ind, "\377yCouldn't find base monster (punctuation and name must be exact).", n);
		return;
	}

	if (parm_log[0]) s_printf("%s EVENT_SIGNUP: %d (%s): %s (%s).\n", showtime(), n + 1, ge->title, p_ptr->name, parm_log);
	else s_printf("%s EVENT_SIGNUP: %d (%s): %s.\n", showtime(), n + 1, ge->title, p_ptr->name);
	if (fake_signup) return;

	/* currently no warning/error/solution if you try to sign on for multiple events at the same time :|
	   However, player_type has MAX_GLOBAL_EVENTS sized data arrays for each event, so a player *could*
	   theoretically participate in all events at once at this time.. */
	msg_format(Ind, "\377c>>You signed up for %s!<<", ge->title);
	msg_broadcast_format(Ind, "\377s%s signed up for %s", p_ptr->name, ge->title);
	ge->participant[p] = p_ptr->id;
	p_ptr->global_event_type[n] = ge->getype;
	time(&p_ptr->global_event_signup[n]);
	p_ptr->global_event_started[n] = ge->started;
	for (i = 0; i < 4; i++) p_ptr->global_event_progress[n][i] = 0; /* WIPE :p */
}

/*
 * Process a global event - C. Blue
 */
static void process_global_event(int ge_id)
{
	global_event_type *ge = &global_event[ge_id];
	player_type *p_ptr;
	object_type forge, *o_ptr = &forge; /* for creating a reward, for example */
	bool timeout = FALSE;
	worldpos wpos;
        struct wilderness_type *wild;
	struct dungeon_type *d_ptr;
	int participants = 0;
	int i, j = 0, n, k, x, y; /* misc variables, used by the events */
	cave_type **zcave, *c_ptr;
	int xstart = 0, ystart = 0; /* for arena generation */
	long elapsed, elapsed_turns; /* amounts of seconds elapsed since the event was started (created) */
	char m_name[80];
	int m_idx, tries = 0;
	time_t now;

	time(&now);
        /* real timer will sometimes fail when using in a function which isn't sync'ed against it but instead relies
	    on the game's FPS for timining ..strange tho :s */
	elapsed_turns = turn - ge->start_turn - ge->paused_turns;
	elapsed = elapsed_turns / cfg.fps;
	wpos.wx = 0; wpos.wy = 0; wpos.wz = 0; /* sector 0,0 by default, for 'sector00separation' */

	if (ge->announcement_time * cfg.fps - elapsed_turns == 240L * cfg.fps) { /* extra warning at T-4min for last minute subscribers */
		announce_global_event(ge_id);
		return; /* not yet >:) */
	} else if (ge->announcement_time * cfg.fps - elapsed_turns >= 0) {
//debug		msg_format(1, ".%d.%d", ge->announcement_time * cfg.fps - elapsed_turns, elapsed_turns);
		if (!((ge->announcement_time * cfg.fps - elapsed_turns) % (GE_ANNOUNCE_INTERVAL * cfg.fps))) {
			if (ge->announcement_time * cfg.fps - elapsed_turns > 0L) {
				announce_global_event(ge_id);
				return; /* not yet >:) */
			} else {
				/* count participants */
				for (i = 0; i < MAX_GE_PARTICIPANTS; i++) {
					if (ge->participant[i]) {
						/* Check that the player really is here - mikaelh */
						for (j = 1; j <= NumPlayers; j++) {
							if (Players[j]->id == ge->participant[i]) {
								participants++;
								break;
							}
						}
					}
				}
				/* enough participants? Don't hand out reward for free to someone. */
				if (ge->min_participants && (participants < ge->min_participants)) {
					msg_broadcast_format(0, "\377y%s needs at least %d participants.", ge->title, ge->min_participants);
					s_printf("%s EVENT_NOPLAYERS: %d (%s) has only %d/%d participants.\n", showtime(), ge_id+1, ge->title, participants, ge->min_participants);
					/* remove players who DID sign up from being 'participants' */
					for (j = 1; j <= NumPlayers; j++)
						if (Players[j]->global_event_type[ge_id] == ge->getype)
							Players[j]->global_event_type[ge_id] = GE_NONE;
					ge->getype = GE_NONE;
				} else {
					msg_broadcast_format(0, "\377C[>>%s starts now!<<]", ge->title);
				}
			}
		} else {
			return; /* still announcing */
		}
	}

	/* Time Over? :( */
	if ((ge->end_turn && turn >= ge->end_turn) ||
	    (ge->ending && now >= ge->ending)) {
		timeout = TRUE; /* d'oh */
		ge->state[0] = 255; /* state[0] is used as indicator for clean-up phase of any event */
		msg_broadcast_format(0, "\377y>>%s ends due to time limit!<<", ge->title);
		s_printf("%s EVENT_TIMEOUT: %d - %s.\n", showtime(), ge_id+1, ge->title);
	}
	/* Time warning at T-5minutes! (only if the whole event lasts MORE THAN 10 minutes) */
	/* Note: paused turns will be added to the running time, IF the event end is given in "turns" */
	if ((ge->end_turn && ge->end_turn - ge->start_turn - ge->paused_turns > 600 * cfg.fps && turn - ge->paused_turns == ge->end_turn - 300 * cfg.fps) ||
	    /* However, paused turns will be ignored if the event end is given as absolute time! */
	    (!ge->end_turn && ge->ending && ge->ending - ge->started > 600 && now == ge->ending - 360)) {
		msg_broadcast_format(0, "\377y[%s comes to an end in 6 more minutes!]", ge->title);
	}

	/* Event is running! Process its stages... */
	switch(ge->getype){
	/* Highlander Tournament */
	case GE_HIGHLANDER:
		switch(ge->state[0]){
		case 0: /* prepare level, gather everyone, start exp'ing */
			ge->cleanup = 1;
			sector00separation++; /* separate sector 0,0 from the worldmap - participants have access ONLY */
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
					adddungeon(&wpos, 1, 50, DF1_NO_RECALL, DF2_IRON |
					    DF2_NO_ENTRY_MASK | DF2_NO_EXIT_MASK, FALSE, 0);
				} else {
					adddungeon(&wpos, 1, 50, DF1_NO_RECALL, DF2_IRON |
					    DF2_NO_ENTRY_WOR | DF2_NO_ENTRY_PROB | DF2_NO_ENTRY_FLOAT | DF2_NO_EXIT_MASK, FALSE, 0);

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
			}

			for (i = 1; i <= NumPlayers; i++)
			for (j = 0; j < MAX_GE_PARTICIPANTS; j++)
			if (ge->participant[j] == Players[i]->id) {
				p_ptr = Players[i];
				if ((p_ptr->max_exp || p_ptr->max_plv > 1) && !is_admin(p_ptr)) {
					msg_print(i, "\377oCharacters need to have 0 experience to be eligible.");
					ge->participant[j] = 0;
					p_ptr->global_event_type[ge_id] = GE_NONE;
					continue;
				}
				if (p_ptr->wpos.wx || p_ptr->wpos.wy) {
					p_ptr->recall_pos.wx = 0;
					p_ptr->recall_pos.wy = 0;
					p_ptr->recall_pos.wz = -1;
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
			        o_ptr->owner_mode = p_ptr->mode;
			        object_aware(i, o_ptr);
		        	object_known(o_ptr);
				inven_carry(i, o_ptr);
				/* give some safe time for exp'ing */
				if (cfg.use_pk_rules == PK_RULES_DECLARE) {
					p_ptr->pkill &= ~PKILL_KILLABLE;
				}
				p_ptr->global_event_progress[ge_id][0] = 1; /* now in 0,0,0-dungeon! */
			}

			ge->state[0] = 1;
			break;
		case 1: /* exp phase - end prematurely if all players pseudo-died in dungeon */
			n = 0;
			k = 0;
			for (i = 1; i <= NumPlayers; i++)
				if (!is_admin(Players[i]) && !Players[i]->wpos.wx && !Players[i]->wpos.wy) {
					n++;
					j = i;
					/* count players who have already been kicked out of the dungeon by pseudo-dieing */
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
					if (!Players[i]->wpos.wx && !Players[i]->wpos.wy) msg_print(i, "\377f[The slaughter will begin soon!]");
				ge->state[0] = 2;
			}
			break;
		case 2: /* final exp phase after the warning has been issued - end prematurely if needed (see above) */
			n = 0;
			k = 0;
			for (i = 1; i <= NumPlayers; i++)
				if (!is_admin(Players[i]) && !Players[i]->wpos.wx && !Players[i]->wpos.wy) {
					n++;
					j = i;
					if (!Players[i]->wpos.wz) k++;
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
				if (is_admin(p_ptr) || p_ptr->wpos.wx || p_ptr->wpos.wy) continue;

				if (p_ptr->party) {
					for (j = 1; j <= NumPlayers; j++) {
						if (j == i) continue;
						if (Players[j]->wpos.wx || Players[j]->wpos.wy) continue;
						/* leave party */
						if (Players[j]->party == p_ptr->party) party_leave(i);
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
					        p_ptr->inventory[j].owner_mode = p_ptr->mode;
					        object_aware(i, &p_ptr->inventory[j]);
				        	object_known(&p_ptr->inventory[j]);
					}
			        p_ptr->update |= (PU_BONUS | PU_VIEW);
				p_ptr->window |= (PW_INVEN | PW_EQUIP);
				handle_stuff(i);

				p_ptr->global_event_temp &= ~PEVF_SAFEDUN_00; /* no longer safe from death */
				p_ptr->global_event_temp |= PEVF_AUTOPVP_00;

				if (Players[i]->wpos.wz) {
					p_ptr->recall_pos.wx = 0;
					p_ptr->recall_pos.wy = 0;
					p_ptr->recall_pos.wz = 0;
	                    		p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
				        recall_player(i, "");
				}

				p_ptr->global_event_progress[ge_id][0] = 4; /* now before deathmatch */
				msg_print(i, "\377fThe bloodshed begins!");
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
					if (p_ptr->wpos.wz && !is_admin(p_ptr)) {
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
				if (is_admin(Players[i])) continue;
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
				while (!(summon_detailed_one_somewhere(&wpos, 1114, 0, FALSE, 101)) && (++tries < 1000));
				summon_override_checks = SO_NONE;
				ge->state[3] = 1; /* remember that we already spawned one so we don't keep spawning */
					/* this actually serves if an admin /gefforward's too far, beyond the spawning
					time, so we can still spawn one without the admin taking too much care.. */
			}

			break;
		case 6: /* we have a winner! */
			j = ge->extra[3];
			if (j <= NumPlayers) { /* Make sure the winner didn't die in the 1 turn that just passed! */
			    p_ptr = Players[j];
			    if (!p_ptr->wpos.wx && !p_ptr->wpos.wy) { /* ok then.. */
				msg_broadcast_format(0, "\374\377a>>%s wins %s!<<", p_ptr->name, ge->title);
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
				o_ptr->note = quark_add("Tournament reward");
				inven_carry(j, o_ptr);

				s_printf("%s EVENT_WON: %s wins %d (%s)\n", showtime(), p_ptr->name, ge_id+1, ge->title);

				/* avoid him dying */
				set_poisoned(j, 0, 0);
				set_cut(j, 0, 0);
				set_food(j, PY_FOOD_FULL);
				hp_player_quiet(j, 5000, TRUE);

				ge->state[0] = 7;
				ge->state[1] = elapsed;
			    } else {
				ge->state[0] = 255; /* no winner, d'oh */
			    }
			} else {
				ge->state[0] = 255; /* no winner, d'oh */
			}
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
				if (!p_ptr->wpos.wx && !p_ptr->wpos.wy) {
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
				}
			}

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
				remdungeon(&wpos, FALSE);

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
		wild=&wild_info[wpos.wy][wpos.wx];
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
			msg_broadcast_format(0, "\377WType '/evinfo %d' and '/evsign %d' to learn more or to sign up.", ge_id+1, ge_id+1);
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
						Players[i]->recall_pos.wx = wpos.wx;
						Players[i]->recall_pos.wy = wpos.wy;
						Players[i]->recall_pos.wz = 0;
						recall_player(i, "\377yThe arena wizards teleport you out of here!");
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

	default: /* generic clean-up routine for untitled events */
		switch (ge->state[0]) {
		case 255: /* remove an untitled event that has been stopped */
			ge->getype = GE_NONE;
			break;
		}
	}
	
	/* Check for end of event */
	if (ge->getype == GE_NONE) {
		msg_broadcast_format(0, "\377s[%s has ended]", ge->title);
		s_printf("%s EVENT_END: %d - '%s'.\n", showtime(), ge_id+1, ge->title);
	}
}

/*
 * Process running global_events - C. Blue
 */
void process_global_events(void)
{
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
void update_check_file(void)
{
	FILE *fp;
	char buf[1024];
	path_build(buf, 1024, ANGBAND_DIR_DATA, "tomenet.check");
	fp = fopen(buf, "w");
	if (fp) {
		/* print the current timestamp into the file */
		fprintf(fp, "%d\n", (int)time(NULL));
		fclose(fp);
	}
}


/*
 * Clear all current_* variables used by Handle_item() - mikaelh
 */
void clear_current(int Ind)
{
	player_type *p_ptr = Players[Ind];

	p_ptr->using_up_item = -1;

	p_ptr->current_enchant_h = 0;
	p_ptr->current_enchant_d = 0;
	p_ptr->current_enchant_a = 0;

	p_ptr->current_identify = 0;
	p_ptr->current_star_identify = 0;
	p_ptr->current_recharge = 0;
	p_ptr->current_artifact = 0;
	p_ptr->current_curse = 0;
	p_ptr->current_tome_creation = 0;

	p_ptr->current_telekinesis = NULL;
}

void calc_techniques(int Ind) {
	player_type *p_ptr = Players[Ind];
	int m = 20; /* default, actually unused since no other classes have SKILL_STANCE */

	p_ptr->melee_techniques = 0x0000;
	p_ptr->ranged_techniques = 0x0000;

	/* Send accessible melee & ranged techniques (hard-coded on client-side, so no defines here :/) */
	switch (p_ptr->pclass) {

	case CLASS_ADVENTURER:
		if (p_ptr->lev >= 6) p_ptr->melee_techniques |= 0x0001; /* Sprint */
		if (p_ptr->lev >= 15) p_ptr->melee_techniques |= 0x0002; /* Taunt */
//		if (p_ptr->lev >= 22) p_ptr->melee_techniques |= 0x0004; /* Jump */
		Send_technique_info(Ind);
		return;	
	case CLASS_ROGUE:
		if (p_ptr->lev >= 3) p_ptr->melee_techniques |= 0x0001; /* Sprint - Rogues know how to get away! */
		if (p_ptr->lev >= 6) p_ptr->melee_techniques |= 0x0002; /* Taunt - Rogues are bad-mouthed ;) */
		if (p_ptr->lev >= 9) p_ptr->melee_techniques |= 0x0008; /* Distract */
		if (p_ptr->lev >= 12) p_ptr->melee_techniques |= 0x0080; /* Flash bomb */
//		if (p_ptr->lev >= 18) p_ptr->melee_techniques |= 0x0004; /* Jump */
		if (p_ptr->lev >= 50 && p_ptr->total_winner) p_ptr->melee_techniques |= 0x4000; /* Shadow run */
		Send_technique_info(Ind);
		return;	
	case CLASS_RUNEMASTER:
		if (p_ptr->lev >= 4) p_ptr->melee_techniques |= 0x0001; /* Sprint - Rogues know how to get away! */
		if (p_ptr->lev >= 9) p_ptr->melee_techniques |= 0x0002; /* Taunt - Rogues are bad-mouthed ;) */
		Send_technique_info(Ind);
		return;	
	case CLASS_MINDCRAFTER:
		if (p_ptr->lev >= 8) p_ptr->melee_techniques |= 0x0002; /* Taunt */
		if (p_ptr->lev >= 12) p_ptr->melee_techniques |= 0x0008; /* Distract */
		Send_technique_info(Ind);
		return;	

	case CLASS_WARRIOR:
//		if (p_ptr->lev >= 24) p_ptr->melee_techniques |= 0x0004; /* Jump */
		m = 0; break;
	case CLASS_RANGER:
//		if (p_ptr->lev >= 24) p_ptr->melee_techniques |= 0x0004; /* Jump */
		m = 4; break;
	case CLASS_PALADIN: m = 7; break;
	case CLASS_MIMIC: m = 11; break;
	}

	if (get_skill(p_ptr, SKILL_STANCE) >= 4 + m) p_ptr->melee_techniques |= 0x0001; /* Sprint */
	if (get_skill(p_ptr, SKILL_STANCE) >= 9 + m) p_ptr->melee_techniques |= 0x0002; /* Taunt */
	if (get_skill(p_ptr, SKILL_STANCE) >= 16 + m) p_ptr->melee_techniques |= 0x0200; /* Spin */
	if (get_skill(p_ptr, SKILL_STANCE) >= 33 + m) p_ptr->melee_techniques |= 0x0800; /* Berserk */

	if (get_skill(p_ptr, SKILL_ARCHERY) >= 4) p_ptr->ranged_techniques |= 0x0001; /* Flare missile */
	if (get_skill(p_ptr, SKILL_ARCHERY) >= 8) p_ptr->ranged_techniques |= 0x0002; /* Precision shot */
	if (get_skill(p_ptr, SKILL_ARCHERY) >= 10) p_ptr->ranged_techniques |= 0x0004; /* Craft some ammunition */
	if (get_skill(p_ptr, SKILL_ARCHERY) >= 16) p_ptr->ranged_techniques |= 0x0008; /* Double-shot */
	if (get_skill(p_ptr, SKILL_ARCHERY) >= 25) p_ptr->ranged_techniques |= 0x0010; /* Barrage */

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
		if (!Ind2) {
			end_mind(Ind, FALSE);
		} else {
			(*p2_ptr) = Players[Ind2];
		}
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
