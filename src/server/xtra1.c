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
	Send_stat(Ind, stat, p_ptr->stat_top[stat], p_ptr->stat_use[stat], p_ptr->stat_ind[stat], p_ptr->stat_cur[stat]);
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
		p = (p_ptr->male ? "**KING**" : "**QUEEN**");
	}

	/* Normal */
	else
	{
		if (p_ptr->lev < 60)
		p = player_title[p_ptr->pclass][((p_ptr->lev/5) < 10)? (p_ptr->lev/5) : 10][1 - p_ptr->male];
		else
		p = player_title_special[p_ptr->pclass][(p_ptr->lev < 99)? (p_ptr->lev - 60)/10 : 4][1 - p_ptr->male];
	}

	/* Ghost */
	if (p_ptr->ghost)
		p = "Ghost";

	Send_title(Ind, p);
}


/*
 * Prints level
 */
static void prt_level(int Ind)
{
	player_type *p_ptr = Players[Ind];

	s64b adv_exp;

	if (p_ptr->lev >= PY_MAX_LEVEL)
		adv_exp = 0;
	else adv_exp = (s64b)((s64b)player_exp[p_ptr->lev - 1] * (s64b)p_ptr->expfact / 100L);

	Send_experience(Ind, p_ptr->lev, p_ptr->max_exp, p_ptr->exp, adv_exp);
}


/*
 * Display the experience
 */
static void prt_exp(int Ind)
{
	player_type *p_ptr = Players[Ind];

	s64b adv_exp;

	if (p_ptr->lev >= PY_MAX_LEVEL)
		adv_exp = 0;
	else adv_exp = (s64b)((s64b)player_exp[p_ptr->lev - 1] * (s64b)p_ptr->expfact / 100L);

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
		strcpy(buf, "      Mad");
	}
	else if (ratio < 25)
	{
		attr = TERM_ORANGE;
		strcpy(buf, "   Insane");
	}
	else if (ratio < 50)
	{
		attr = TERM_YELLOW;
		strcpy(buf, "    Weird");
	}
	else if (ratio < 75)
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
		sprintf(buf, "%4d/%4d", p_ptr->csane, p_ptr->msane);
	}
	/* Percentile */
	else if (skill >= 20)
	{
		sprintf(buf, "     %3d%%", ratio);
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
//	if (p_ptr->searching) i+=(p_ptr->mode&MODE_HELL ? 5 : 10);
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

	int c = p_ptr->cut;

	Send_cut(Ind, c);
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

	int show_tohit_m = p_ptr->dis_to_h + p_ptr->to_h_melee;
	int show_todam_m = p_ptr->dis_to_d + p_ptr->to_d_melee;
/*	int show_tohit_m = p_ptr->to_h_melee;
	int show_todam_m = p_ptr->to_d_melee;
*/
	int show_tohit_r = p_ptr->dis_to_h + p_ptr->to_h_ranged;
	int show_todam_r = p_ptr->to_d_ranged;

	object_type *o_ptr = &p_ptr->inventory[INVEN_WIELD];
	object_type *o_ptr2 = &p_ptr->inventory[INVEN_BOW];
	object_type *o_ptr3 = &p_ptr->inventory[INVEN_AMMO];

	/* Hack -- add in weapon info if known */
        if (object_known_p(Ind, o_ptr)) show_tohit_m += o_ptr->to_h;
	if (object_known_p(Ind, o_ptr)) show_todam_m += o_ptr->to_d;
	if (object_known_p(Ind, o_ptr2)) show_tohit_r += o_ptr2->to_h;
	if (object_known_p(Ind, o_ptr2)) show_todam_r += o_ptr2->to_d;
	if (object_known_p(Ind, o_ptr3)) show_tohit_r += o_ptr3->to_h;
	if (object_known_p(Ind, o_ptr3)) show_todam_r += o_ptr3->to_d;

//	Send_plusses(Ind, show_tohit_m, show_todam_m, show_tohit_r, show_todam_r, p_ptr->to_h_melee, p_ptr->to_d_melee);
	Send_plusses(Ind, 0, 0, show_tohit_r, show_todam_r, show_tohit_m, show_todam_m);
}

static void prt_skills(int Ind)
{
	Send_skills(Ind);
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
	Send_char_info(Ind, p_ptr->prace, p_ptr->pclass, p_ptr->male);

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
	/* Cut/Stun */
	prt_cut(Ind);
	prt_stun(Ind);

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
	bonus = ((int)(adj_con_mhp[p_ptr->stat_ind[A_WIS]]) - 128);

	/* Hack -- assume 5 sanity points per level. */
	msane = 5*(lev+1) + (bonus * lev / 2);

	if (msane < lev + 1) msane = lev + 1;

	if (p_ptr->msane != msane) {

		/* Sanity carries over between levels. */
		p_ptr->csane += (msane - p_ptr->msane);
		/* If sanity just dropped to 0 or lower, die! */
                if (p_ptr->csane < 0) {
                        /* Sound */
                        sound(Ind, SOUND_DEATH);
                        /* Hack -- Note death */
                        msg_print(Ind, "\377vYou turn into an unthinking vegetable.");
			(void)strcpy(p_ptr->died_from, "Insanity");
	                if (!p_ptr->ghost) {
		                strcpy(p_ptr->died_from_list, "Insanity");
		                p_ptr->died_from_depth = getlevel(&p_ptr->wpos);
			}
            		/* No longer a winner */
		        p_ptr->total_winner = FALSE;
			/* Note death */
			p_ptr->death = TRUE;
	                p_ptr->deathblow = 0;
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
static void calc_mana(int Ind)
{
	player_type *p_ptr = Players[Ind];
	player_type *p_ptr2=(player_type*)NULL;

	int levels, cur_wgt, max_wgt;
	s32b new_mana=0;

	object_type	*o_ptr;

	u32b f1, f2, f3, f4, f5, esp;

	int Ind2 = 0;


	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_PAIN))
	  {
		Ind2 = find_player(p_ptr->esp_link);

		if (!Ind2)
			end_mind(Ind, FALSE);
		else
		{
			p_ptr2 = Players[Ind2];
		}
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
		/* much Int, few Wis */
		new_mana = get_skill_scale(p_ptr, SKILL_MAGIC, 200) +
			    (adj_mag_mana[p_ptr->stat_ind[A_INT]] * 85 * levels / (500)) +
			    (adj_mag_mana[p_ptr->stat_ind[A_WIS]] * 15 * levels / (500));
		break;
	case CLASS_PRIEST:
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
	case CLASS_ADVENTURER:
	case CLASS_BARD:
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
		    !(f2 & TR2_FREE_ACT) &&
		    !((f1 & TR1_DEX) && (o_ptr->pval > 0)))
		{
			/* Encumbered */
			p_ptr->cumber_glove = TRUE;

			/* Reduce mana */
			new_mana = (3 * new_mana) / 4;
		}
	}
#if 0 // C. Blue - Taken out again since polymorphing ability of DSMs was
// removed instead. Mimics and mimic-sorcs were punished too much.
	/* Forms that don't have proper 'hands' (arms) have mana penalty.
	   this will hopefully stop the masses of D form Istari :/ */
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
		if (p_ptr->to_m) new_mana += new_mana * p_ptr->to_m / 100;
		break;
	case CLASS_PRIEST:
	case CLASS_PALADIN:
	case CLASS_MIMIC:
		if (p_ptr->to_m) new_mana += new_mana * p_ptr->to_m / 200;
		break;
	case CLASS_RANGER:
	case CLASS_ADVENTURER:
	case CLASS_ROGUE:
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
	/* Assume player not encumbered by armor */
	p_ptr->cumber_armor = FALSE;

	/* Weigh the armor */
	cur_wgt = armour_weight(p_ptr);

	/* Determine the weight allowance */
	max_wgt = 200 + get_skill_scale(p_ptr, SKILL_COMBAT, 500);

	/* Heavy armor penalizes mana */
	if (((cur_wgt - max_wgt) / 10) > 0)
	{
		/* Encumbered */
		p_ptr->cumber_armor = TRUE;

		/* Reduce mana */
		new_mana -= ((cur_wgt - max_wgt) * 2 / 3);
	}

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
			msg_print(Ind, "Your covered hands feel unsuitable for spellcasting.");
		}
		else
		{
			msg_print(Ind, "Your hands feel more suitable for spellcasting.");
		}

		/* Save it */
		p_ptr->old_cumber_glove = p_ptr->cumber_glove;
	}


	/* Take note when "armor state" changes */
	if (p_ptr->old_cumber_armor != p_ptr->cumber_armor)
	{
		/* Message */
		if (p_ptr->cumber_armor)
		{
			msg_print(Ind, "The weight of your armour encumbers your movement.");
		}
		else
		{
			msg_print(Ind, "You feel able to move more freely.");
		}

		/* Save it */
		p_ptr->old_cumber_armor = p_ptr->cumber_armor;
	}
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

static void calc_hitpoints(int Ind)
{
	player_type *p_ptr = Players[Ind], *p_ptr2;

//	object_type *o_ptr;
//	u32b f1, f2, f3, f4, f5, esp;

	int bonus, Ind2 = 0;
	long mhp, mhp_playerform;
	u32b mHPLim, finalHP;

	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_PAIN))
	{
	    Ind2 = find_player(p_ptr->esp_link);

	    if (!Ind2)
	    	end_mind(Ind, FALSE);
	    else
		p_ptr2 = Players[Ind2];
	}

	/* Un-inflate "half-hitpoint bonus per level" value */
	bonus = ((int)(adj_con_mhp[p_ptr->stat_ind[A_CON]]) - 128);

	/* Calculate hitpoints */
	if (p_ptr->fruit_bat) mhp = (p_ptr->player_hp[p_ptr->lev-1] / 4) + (bonus * p_ptr->lev);
	else mhp = p_ptr->player_hp[p_ptr->lev-1] + (bonus * p_ptr->lev / 2);

#if 0 // DGDGDGDG why ?
	/* Option : give mages a bonus hitpoint / lvl */
	if (cfg.mage_hp_bonus)
		if (p_ptr->pclass == CLASS_MAGE) mhp += p_ptr->lev;
#endif

	/* Sorcery reduces hp */
	if (get_skill(p_ptr, SKILL_SORCERY))
	{
		// mhp -= (mhp * get_skill_scale(p_ptr, SKILL_SORCERY, 20)) / 100;
		mhp -= (mhp * get_skill_scale(p_ptr, SKILL_SORCERY, 33)) / 100;
	}

	/* Now we calculated the base player form mhp. Save it for use with
	   +LIFE bonus. This will prevent mimics from total uber HP,
	   and giving them an excellent chance to compensate a form that
	   provides bad HP. - C. Blue */
	mhp_playerform = mhp;

	if (p_ptr->body_monster)
	{
	    long rhp = ((long)(r_info[p_ptr->body_monster].hdice)) * ((long)(r_info[p_ptr->body_monster].hside));

	    /* pre-cap monster HP against ~3500 (5000) */
	    mHPLim = (100000 / ((100000 / rhp) + 18));
	    /* average with player HP */
	    finalHP = (mHPLim < mhp ) ? (((mhp * 5) + (mHPLim * 2)) / 7) : ((mHPLim + mhp) / 2);
	    /* cap final HP against ~2300 */
//	    finalHP = (100000 / ((100000 / finalHP) + 20));
	    /* done */
	    mhp = finalHP;
	}

	/* Always have at least one hitpoint per level */
	if (mhp < p_ptr->lev + 1) mhp = p_ptr->lev + 1;

	/* Factor in the hero / superhero settings */
	if (p_ptr->hero) mhp += 10;
	if (p_ptr->shero) mhp += 30;

	if (p_ptr->fury) mhp += 40;
	
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

	/* Bonus from +LIFE items (should be weapons only) */
	mhp += mhp_playerform * p_ptr->to_l / 10;

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
 * should be changed, call calc_bonuses too.
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
	}

	/* Notice changes in the "lite radius" */
	if (p_ptr->old_lite != p_ptr->cur_lite)
	{
		/* Update the lite */
		p_ptr->update |= (PU_LITE);

		/* Update the monsters */
		p_ptr->update |= (PU_MONSTERS);

		/* Remember the old lite */
		p_ptr->old_lite = p_ptr->cur_lite;
	}
}



/*
 * Computes current weight limit.
 */
static int weight_limit(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int i;

	/* Weight limit based only on strength */
	i = adj_str_wgt[p_ptr->stat_ind[A_STR]] * 100;

	/* Return the result */
	return (i);
}


/* Should be called by every calc_bonus call */
/* TODO: allow ego form */
static void calc_body_bonus(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int n, d, i, j, toac = 0, body = 0, immunities = 0, immunity[7], immrand[3];
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
	
	/* Apply STR bonus/malus, derived from form damage */
	if (d == 0) d = 1;
	/* 0..147 (greater titan) -> 0..5 -> -1..+4 */
	p_ptr->stat_add[A_STR] += (((15000 / ((15000 / d) + 50)) / 29) - 1);

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
#if 0
	/* Evaluate monster AC (if skin or armor etc) */
	body = (r_ptr->body_parts[BODY_HEAD] ? 1 : 0)
		+ (r_ptr->body_parts[BODY_TORSO] ? 3 : 0)
		+ (r_ptr->body_parts[BODY_ARMS] ? 2 : 0)
		+ (r_ptr->body_parts[BODY_LEGS] ? 1 : 0);

	toac = r_ptr->ac * 14 / (7 + body);
	/* p_ptr->ac += toac;
	p_ptr->dis_ac += toac; - similar to HP calculation: */
	if (toac < (p_ptr->ac + p_ptr->to_a)) {
		p_ptr->ac = (p_ptr->ac * 3) / 4;
		p_ptr->to_a = ((p_ptr->to_a * 3) + (toac * 1)) / 4;
		p_ptr->dis_ac = ((p_ptr->dis_ac * 3) + (toac * 1)) / 4;
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
		p_ptr->pspeed = 110 - (((110 - r_ptr->speed) * 30) / 100);
	}
	else
	{
		/* let speed bonus not be that high that players won't try any slower form */
		p_ptr->pspeed = (((r_ptr->speed - 110) * 50) / 100) + 110;
	}

	/* Base skill -- searching ability */
	p_ptr->skill_srh /= 2;
	p_ptr->skill_srh += r_ptr->aaf / 10;

	/* Base skill -- searching frequency */
	p_ptr->skill_fos /= 2;
	p_ptr->skill_fos += r_ptr->aaf / 10;

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
	if (r_ptr->flags4 & RF4_ARROW_1)
	{
		p_ptr->num_fire++;	// Free
		if (r_ptr->freq_inate > 30) p_ptr->num_fire++;	// 1_IN_3
		if (r_ptr->freq_inate > 60) p_ptr->num_fire++;	// 1_IN_1
	}
	else
	/* Extra casting if good spellcaster */
	{
		if (r_ptr->freq_inate > 30) p_ptr->num_spell++;	// 1_IN_3
		if (r_ptr->freq_inate > 60) p_ptr->num_spell++;	// 1_IN_1
	}


	/* Racial boni depending on the form's race */
	switch(p_ptr->body_monster)
	{
		/* Bats get feather falling */
		case 37:	case 114:	case 187:	case 235:	case 351:
		case 377:	case 391:	case 406:	case 484:	case 968:
		{
			p_ptr->feather_fall = TRUE;
			break;
		}

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
			p_ptr->sensible_fire = TRUE;
			p_ptr->resist_water = TRUE;

			if (p_ptr->lev >= 4) p_ptr->see_inv = TRUE;

			p_ptr->can_swim = TRUE; /* wood? */
			p_ptr->pass_trees = TRUE;
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

	/* Affect charisma by appearance */
	if(r_ptr->flags3 & RF3_DRAGON) d = 0;
	if(r_ptr->flags3 & RF3_DRAGONRIDER) d = 0;
	if(r_ptr->flags3 & RF3_ANIMAL) d = 0;

	if(r_ptr->flags3 & RF3_EVIL) d = -1;

	if(r_ptr->flags3 & RF3_ORC) d = -1;
	if(r_ptr->flags3 & RF3_DEMON) d = -1;
	if(r_ptr->flags3 & RF3_NONLIVING) d = -1;

	if(r_ptr->flags3 & RF3_TROLL) d = -2;
	if(r_ptr->flags3 & RF3_GIANT) d = -2;
	if(r_ptr->flags3 & RF3_UNDEAD) d = -2;
	if(r_ptr->flags7 & RF7_SPIDER) d = -2;

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
	if(r_ptr->flags2 & RF2_PASS_WALL) p_ptr->tim_wraith = 30000;
	//        if(r_ptr->flags2 & RF2_KILL_WALL) p_ptr->auto_tunnel = 100;
	/* quick hack */
	if(r_ptr->flags2 & RF2_KILL_WALL) p_ptr->skill_dig = 20000;
	if(r_ptr->flags2 & RF2_AURA_FIRE) p_ptr->sh_fire = TRUE;
	if(r_ptr->flags2 & RF2_AURA_ELEC) p_ptr->sh_elec = TRUE;
	if(r_ptr->flags2 & RF3_AURA_COLD) p_ptr->sh_cold = TRUE;

	if(r_ptr->flags5 & RF5_MIND_BLAST) p_ptr->reduce_insanity = 1;
	if(r_ptr->flags5 & RF5_BRAIN_SMASH) p_ptr->reduce_insanity = 2;

	if(r_ptr->flags3 & RF3_SUSCEP_FIRE) p_ptr->sensible_fire = TRUE;
	if(r_ptr->flags3 & RF3_SUSCEP_COLD) p_ptr->sensible_cold = TRUE;
/* Imho, there should be only suspec fire and cold since these two are opposites.
Something which is fire-related will usually be suspectible to cold and vice versa.
Exceptions are rare, like Ent, who as a being of wood is suspectible to fire. (C. Blue) */
	if(r_ptr->flags9 & RF9_SUSCEP_ELEC) p_ptr->sensible_elec = TRUE;
	if(r_ptr->flags9 & RF9_SUSCEP_ACID) p_ptr->sensible_acid = TRUE;
	if(r_ptr->flags9 & RF9_SUSCEP_POIS) p_ptr->sensible_pois = TRUE;

	if(r_ptr->flags9 & RF9_RES_ACID) p_ptr->resist_acid = TRUE;
	if(r_ptr->flags9 & RF9_RES_ELEC) p_ptr->resist_elec = TRUE;
	if(r_ptr->flags9 & RF9_RES_FIRE) p_ptr->resist_fire = TRUE;
	if(r_ptr->flags9 & RF9_RES_COLD) p_ptr->resist_cold = TRUE;
	if(r_ptr->flags9 & RF9_RES_POIS) p_ptr->resist_pois = TRUE;

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
	if(r_ptr->flags9 & RF9_RES_SHARDS) p_ptr->resist_shard = TRUE;

	if(r_ptr->flags3 & RF3_RES_TELE) p_ptr->res_tele = TRUE;
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
		p_ptr->antimagic += r_ptr->level / 2 + 20;
		p_ptr->antimagic_dis += r_ptr->level / 15 + 3;
	}

	if(r_ptr->flags2 & RF2_WEIRD_MIND) p_ptr->reduce_insanity = 1;
	if(r_ptr->flags2 & RF2_EMPTY_MIND) p_ptr->reduce_insanity = 2;

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
	return (monk_arm_wgt > 100 + get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 200));
#endif
}
#endif	// 0

/* Are all the weapons wielded of the right type ? */
static int get_weaponmastery_skill(player_type *p_ptr)
{
	int i, skill = 0;
	object_type *o_ptr = &p_ptr->inventory[INVEN_WIELD];

	i = 0;

        if (!o_ptr->k_idx)
        {
                return 0;
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
        case TV_HAFTED:
                if ((!skill) || (skill == SKILL_HAFTED)) skill = SKILL_HAFTED;
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


int calc_blows(int Ind, object_type *o_ptr)
{
	player_type *p_ptr = Players[Ind];
	int str_index, dex_index;

	int num = 0, wgt = 0, mul = 0, div = 0, num_blow = 0;

	/* Analyze the class */
	switch (p_ptr->pclass)
	{
							/* Adevnturer */
		case CLASS_ADVENTURER: num = 4; wgt = 35; mul = 5; break;
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
		case CLASS_ROGUE: num = 5; wgt = 30; mul = 3; break;

							/* Mimic */
		case CLASS_MIMIC: num = 4; wgt = 30; mul = 4; break;//mul3

							/* Archer */
		case CLASS_ARCHER: num = 3; wgt = 30; mul = 3; break;

							/* Paladin */
		case CLASS_PALADIN: num = 5; wgt = 35; mul = 5; break;//mul4

							/* Ranger */
		case CLASS_RANGER: num = 5; wgt = 35; mul = 5; break;//mul4


		case CLASS_BARD: num = 4; wgt = 35; mul = 4; break;
	}

	/* Enforce a minimum "weight" (tenth pounds) */
	div = ((o_ptr->weight < wgt) ? wgt : o_ptr->weight);

	/* Access the strength vs weight */
	str_index = (adj_str_blow[p_ptr->stat_ind[A_STR]] * mul / div);

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

	/* Add in the "bonus blows" */
//	num_blow += p_ptr->extra_blows;

	/* Require at least one blow */
	if (num_blow < 1) num_blow = 1;

	/* Boost blows with masteries */
	if (get_weaponmastery_skill(p_ptr) != -1)
	{
		num_blow += get_skill_scale(p_ptr, get_weaponmastery_skill(p_ptr), 2);
	}

	if (p_ptr->zeal) num_blow += (p_ptr->zeal_power / 10);

	return (num_blow);
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
void calc_bonuses(int Ind)
{
	cptr inscription = NULL;
	
	char tmp[80];

	player_type *p_ptr = Players[Ind];

	int			i, j, hold, minus, am;
	long			w;

	int			old_speed;

	int			old_telepathy;
	int			old_see_inv;

	int			old_dis_ac;
	int			old_dis_to_a;

	int			old_dis_to_h;
	int			old_dis_to_d;

//	int			extra_blows;
	int			extra_shots;
	int			extra_spells;

	object_type		*o_ptr;
	object_kind		*k_ptr;

	long int n, d, toac = 0, body = 0;
	bool wepless = FALSE;
	monster_race *r_ptr = &r_info[p_ptr->body_monster];

	    u32b f1, f2, f3, f4, f5, esp;
		s16b pval;


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


	/* Clear the Displayed/Real armor class */
	p_ptr->dis_ac = p_ptr->ac = 0;

	/* Clear the Displayed/Real Bonuses */
	p_ptr->dis_to_h = p_ptr->to_h = p_ptr->to_h_melee = p_ptr->to_h_ranged = 0;
	p_ptr->dis_to_d = p_ptr->to_d = p_ptr->to_d_melee = p_ptr->to_d_ranged = 0;
	p_ptr->dis_to_a = p_ptr->to_a = 0;


	/* Clear all the flags */
	p_ptr->aggravate = FALSE;
	p_ptr->teleport = FALSE;
	p_ptr->exp_drain = FALSE;
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
	p_ptr->no_cut = FALSE;
	p_ptr->reduce_insanity = 0;
//		p_ptr->to_s = 0;
		p_ptr->to_m = 0;
		p_ptr->to_l = 0;
		p_ptr->black_breath_tmp = FALSE;
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
	p_ptr->dodge_chance = 0;

	p_ptr->sensible_fire = FALSE;
	p_ptr->sensible_cold = FALSE;
	p_ptr->sensible_elec = FALSE;
	p_ptr->sensible_acid = FALSE;
	p_ptr->sensible_pois = FALSE;
	p_ptr->resist_continuum = FALSE;

	/* Start with a single blow per turn */
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
		/* Start with "normal" speed */
		p_ptr->pspeed = 110;

		/* Bats get +10 speed ... they need it!*/
		if (p_ptr->fruit_bat){
			p_ptr->pspeed += 10;
			p_ptr->fly = TRUE;
			p_ptr->feather_fall = TRUE;
		}
	/* Choosing a race just for its HP or low exp% shouldn't be what we want -C. Blue- */
	}

	/* Elf */
	if (p_ptr->prace == RACE_ELF) p_ptr->resist_lite = TRUE;

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
	}

	/* Yeek */
	else if (p_ptr->prace == RACE_YEEK) p_ptr->feather_fall = TRUE;

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
		p_ptr->slow_digest = TRUE;
		p_ptr->sensible_fire = TRUE;
		p_ptr->resist_water = TRUE;

		/* not while in mimicried form */
		if(!p_ptr->body_monster)
		{
			p_ptr->can_swim = TRUE; /* wood? */
			p_ptr->pass_trees = TRUE;
			p_ptr->pspeed -= 2;
		}

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

	if (p_ptr->pclass == CLASS_RANGER)
	    if (p_ptr->lev >= 20) p_ptr->pass_trees = TRUE;

	/* Check ability skills */
	if (get_skill(p_ptr, SKILL_CLIMB) >= 1) p_ptr->climb = TRUE;
	if (get_skill(p_ptr, SKILL_FLY) >= 1) p_ptr->fly = TRUE;
	if (get_skill(p_ptr, SKILL_FREEACT) >= 1) p_ptr->free_act = TRUE;
	if (get_skill(p_ptr, SKILL_RESCONF) >= 1) p_ptr->resist_conf = TRUE;

	/* Compute antimagic */
	if (get_skill(p_ptr, SKILL_ANTIMAGIC))
	{
//		p_ptr->anti_magic = TRUE;	/* it means 95% saving-throw!! */
		p_ptr->antimagic += get_skill(p_ptr, SKILL_ANTIMAGIC);
		p_ptr->antimagic_dis += 1 + (get_skill(p_ptr, SKILL_ANTIMAGIC) / 11);
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
		p_ptr->resist_pois = TRUE; /* instead of immune_poison */
		p_ptr->resist_cold = TRUE;
		p_ptr->resist_fear = TRUE;
		p_ptr->resist_conf = TRUE;
		p_ptr->no_cut = TRUE;
		p_ptr->reduce_insanity = 1;
		/*p_ptr->fly = TRUE; redundant*/
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
	if (p_ptr->mode & MODE_HELL)
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
		if(o_ptr->name1 == ART_ANCHOR)
		{
			p_ptr->resist_continuum = TRUE;
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
			if (k_ptr->flags1 & TR1_SEARCH) p_ptr->skill_fos += (o_ptr->bpval * 5);

			/* Affect infravision */
			if (k_ptr->flags1 & TR1_INFRA) p_ptr->see_infra += o_ptr->bpval;

			/* Affect digging (factor of 20) */
			if (k_ptr->flags1 & TR1_TUNNEL) p_ptr->skill_dig += (o_ptr->bpval * 20);

			/* Affect speed */
			if (k_ptr->flags1 & TR1_SPEED) p_ptr->pspeed += o_ptr->bpval;

			/* Affect blows */
			if (k_ptr->flags1 & TR1_BLOWS) p_ptr->extra_blows += o_ptr->bpval;

			/* Affect spells */
			if (k_ptr->flags1 & TR1_SPELL) extra_spells += o_ptr->bpval;
//			if (k_ptr->flags1 & TR1_SPELL_SPEED) extra_spells += o_ptr->bpval;

                /* Affect mana capacity */
                if (f1 & (TR1_MANA)) {
			if ((f4 & TR4_COULD2H) &&
			    (p_ptr->inventory[INVEN_WIELD].k_idx && p_ptr->inventory[INVEN_ARM].k_idx))
				p_ptr->to_m += (o_ptr->bpval * 20) / 3;
			else
				p_ptr->to_m += o_ptr->bpval * 10;
		}

                /* Affect life capacity */
                if (f1 & (TR1_LIFE)) p_ptr->to_l += o_ptr->bpval;

		}

		if (k_ptr->flags5 & TR5_PVAL_MASK)
		{
			if (f5 & (TR5_CRIT)) p_ptr->xtra_crit += o_ptr->bpval;
			if (f5 & (TR5_DISARM)) p_ptr->skill_dis += (o_ptr->bpval) * 10;
			if (f5 & (TR5_LUCK)) p_ptr->luck_cur += o_ptr->bpval;
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
			if ((f4 & TR4_COULD2H) &&
			    (p_ptr->inventory[INVEN_WIELD].k_idx && p_ptr->inventory[INVEN_ARM].k_idx))
				p_ptr->to_m += (pval * 20) / 3;
			else
            			p_ptr->to_m += pval * 10;
		}

                /* Affect life capacity */
                if (f1 & (TR1_LIFE)) p_ptr->to_l += pval;

		/* Affect stealth */
		if (f1 & TR1_STEALTH) p_ptr->skill_stl += pval;

		/* Affect searching ability (factor of five) */
		if (f1 & TR1_SEARCH) p_ptr->skill_srh += (pval * 5);

		/* Affect searching frequency (factor of five) */
		if (f1 & TR1_SEARCH) p_ptr->skill_fos += (pval * 5);

		/* Affect infravision */
		if (f1 & TR1_INFRA) p_ptr->see_infra += pval;

		/* Affect digging (factor of 20) */
		if (f1 & TR1_TUNNEL) p_ptr->skill_dig += (pval * 20);

		/* Affect speed */
		if (f1 & TR1_SPEED) p_ptr->pspeed += pval;

		/* Affect blows */
		if (f1 & TR1_BLOWS) p_ptr->extra_blows += pval;
                if (f5 & (TR5_CRIT)) p_ptr->xtra_crit += pval;

		/* Affect spellss */
//		if (f1 & TR1_SPELL_SPEED) extra_spells += pval;
		if (f1 & TR1_SPELL) extra_spells += pval;

		/* Affect disarming (factor of 20) */
		if (f5 & (TR5_DISARM)) p_ptr->skill_dis += pval * 10;

		/* Hack -- Sensible */
		if (f5 & (TR5_SENS_FIRE)) p_ptr->sensible_fire = TRUE;
/* not yet implemented
		if (f5 & (TR6_SENS_COLD)) p_ptr->sensible_cold = TRUE;
		if (f5 & (TR6_SENS_ELEC)) p_ptr->sensible_elec = TRUE;
		if (f5 & (TR6_SENS_ACID)) p_ptr->sensible_acid = TRUE;
		if (f5 & (TR6_SENS_POIS)) p_ptr->sensible_acid = TRUE; */

		/* Boost shots */
//		if (f3 & TR3_KNOWLEDGE) p_ptr->auto_id = TRUE;
		if (f4 & TR4_AUTO_ID) p_ptr->auto_id = TRUE;

		/* Boost shots */
		if (f3 & TR3_XTRA_SHOTS) extra_shots++;

		/* Various flags */
		if (f3 & TR3_AGGRAVATE) p_ptr->aggravate = TRUE;
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
		if (f3 & TR3_DRAIN_EXP) p_ptr->exp_drain = TRUE;
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
			p_ptr->tim_wraith = 30000;
		}
                if (f4 & (TR4_FLY)) p_ptr->fly = TRUE;
                if (f4 & (TR4_CLIMB)) p_ptr->climb = TRUE;
                if (f4 & (TR4_IM_NETHER)) p_ptr->immune_neth = TRUE;
		if (f5 & (TR5_REFLECT)) p_ptr->reflect = TRUE;
		if (f3 & (TR3_SH_FIRE)) p_ptr->sh_fire = TRUE;
		if (f3 & (TR3_SH_ELEC)) p_ptr->sh_elec = TRUE;
/*not implemented yet:	if (f3 & (TR3_SH_COLD)) p_ptr->sh_cold = TRUE;*/
		if (f3 & (TR3_NO_MAGIC)) p_ptr->anti_magic = TRUE;
		if (f3 & (TR3_NO_TELE)) p_ptr->anti_tele = TRUE;

		/* Additional flags from PernAngband */

		if (f4 & (TR4_IM_NETHER)) p_ptr->immune_neth = TRUE;

		/* Limit use of disenchanted DarkSword for non-unbe */
		minus = o_ptr->to_h + o_ptr->to_d; // + pval;// + (o_ptr->to_a / 4);
		if (minus < 0) minus = 0;
		am = 0;
		if (f4 & TR4_ANTIMAGIC_50) am += 50;
		if (f4 & TR4_ANTIMAGIC_30) am += 30;
		if (f4 & TR4_ANTIMAGIC_20) am += 20;
		if (f4 & TR4_ANTIMAGIC_10) am += 10;
		am -= minus;
		/* Weapons may not give more than 50% AM */
		if (am > 50) am = 50;
		if (am > 0)
		{
			p_ptr->antimagic += am;
			p_ptr->antimagic_dis += (am / 15);
		}

		if (f4 & (TR4_BLACK_BREATH)) p_ptr->black_breath_tmp = TRUE;

//		if (f5 & (TR5_IMMOVABLE)) p_ptr->immovable = TRUE;



		/* Modify the base armor class */
		p_ptr->ac += o_ptr->ac;

		/* The base armor class is always known */
		p_ptr->dis_ac += o_ptr->ac;

		/* Apply the bonuses to armor class */
		p_ptr->to_a += o_ptr->to_a;

		/* Apply the mental bonuses to armor class, if known */
		if (object_known_p(Ind, o_ptr)) p_ptr->dis_to_a += o_ptr->to_a;

		/* Hack -- do not apply "weapon" bonuses */
		if (i == INVEN_WIELD) continue;

		/* Hack -- do not apply "bow" bonuses */
		if (i == INVEN_BOW) continue;

		if (i == INVEN_AMMO || i == INVEN_TOOL) continue;

		/* Hack -- cause earthquakes */
		if (f5 & TR5_IMPACT) p_ptr->impact = TRUE;

		/* Apply the bonuses to hit/damage */
		p_ptr->to_h += o_ptr->to_h;
		p_ptr->to_d += o_ptr->to_d;

		/* Apply the mental bonuses tp hit/damage, if known */
		if (object_known_p(Ind, o_ptr)) p_ptr->dis_to_h += o_ptr->to_h;
		if (object_known_p(Ind, o_ptr)) p_ptr->dis_to_d += o_ptr->to_d;

	}

	if (p_ptr->antimagic > 90) p_ptr->antimagic = 90; /* AM cap */
	if (p_ptr->luck_cur > 40) p_ptr->luck_cur = 40; /* luck caps at 40 */


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
#else	// 0
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS) && !monk_heavy_armor(p_ptr))
	{
		int k = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 6);

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

				if (((!p_ptr->inventory[INVEN_ARM].k_idx) ||
				    (k_info[p_ptr->inventory[INVEN_ARM].k_idx].weight < 100)) &&
				    ((!p_ptr->inventory[INVEN_BOW].k_idx) ||
				    (k_info[p_ptr->inventory[INVEN_BOW].k_idx].weight < 150)) &&
				    ((!p_ptr->inventory[INVEN_WIELD].k_idx) ||
				    (k_info[p_ptr->inventory[INVEN_WIELD].k_idx].weight < 150)))
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
			if (!(p_ptr->inventory[INVEN_ARM].k_idx) && (marts > 10))
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

	p_ptr->pspeed += get_skill_scale(p_ptr, SKILL_AGILITY, 10);
	p_ptr->pspeed += get_skill_scale(p_ptr, SKILL_SNEAKINESS, 7);
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
		if (!(p_ptr->inventory[INVEN_ARM].k_idx) && (marts > 10))
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

	/* Apply temporary "stun" */
	if (p_ptr->stun > 50)
	{
		p_ptr->to_h -= 20;
		p_ptr->dis_to_h -= 20;
		p_ptr->to_d -= 20;
		p_ptr->dis_to_d -= 20;
	}
	else if (p_ptr->stun)
	{
		p_ptr->to_h -= 5;
		p_ptr->dis_to_h -= 5;
		p_ptr->to_d -= 5;
		p_ptr->dis_to_d -= 5;
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
		p_ptr->dis_to_a -= 10;
	}

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
		p_ptr->to_h += p_ptr->blessed_power;
		p_ptr->dis_to_h += p_ptr->blessed_power;
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

	/* Temporary "Hero" */
	if (p_ptr->hero)
	{
		p_ptr->to_h += 12;
		p_ptr->dis_to_h += 12;
	}

	/* Heart is boldened */
	if (p_ptr->res_fear_temp) p_ptr->resist_fear = TRUE;

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

	/* Temporary "Fury" */
	if (p_ptr->fury)
	{
		p_ptr->to_h -= 10;
		p_ptr->dis_to_h -= 10;
                p_ptr->to_d += 20;
                p_ptr->dis_to_d += 20;
		p_ptr->pspeed += 10;
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

	/* Hack -- Hero/Shero -> Res fear */
	if (p_ptr->hero || p_ptr->shero)
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


	/* Extract the current weight (in tenth pounds) */
	w = p_ptr->total_weight;

	/* Extract the "weight limit" (in tenth pounds) */
	i = weight_limit(Ind);

	/* XXX XXX XXX Apply "encumbrance" from weight */
	if (w > i/2) p_ptr->pspeed -= ((w - (i/2)) / (i / 10));

	/* Bloating slows the player down (a little) */
	if (p_ptr->food >= PY_FOOD_MAX) p_ptr->pspeed -= 10;

	/* Searching slows the player down */
	/* -APD- adding "stealth mode" for rogues... will probably need to tweek this */
	/* XXX this can be out of place; maybe better done
	 * after skill bonuses are added?	- Jir - */
	if (p_ptr->searching) 
	{
		int stealth = get_skill(p_ptr, SKILL_STEALTH);
		int sneakiness = get_skill(p_ptr, SKILL_SNEAKINESS);
//		p_ptr->pspeed -= 10;
		p_ptr->pspeed -= 10 - sneakiness / 7;

		if (stealth >= 10)
		{
//			p_ptr->skill_stl *= 3;
			p_ptr->skill_stl = p_ptr->skill_stl * stealth / 10;
		}
		if (sneakiness >= 10)
		{
			p_ptr->skill_srh = p_ptr->skill_srh * sneakiness / 10;
//			p_ptr->skill_fos = p_ptr->skill_fos * sneakiness / 10;
		}
	}

	if (p_ptr->mode & MODE_HELL && p_ptr->pspeed > 110)
	{
		int speed = p_ptr->pspeed - 110;

		speed /= 2;

		p_ptr->pspeed = speed + 110;
	}

	/* Display the speed (if needed) */
	if (p_ptr->pspeed != old_speed) p_ptr->redraw |= (PR_SPEED);


	/* Actual Modifier Bonuses (Un-inflate stat bonuses) */
	p_ptr->to_a += ((int)(adj_dex_ta[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->to_d += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
	p_ptr->to_h += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->to_h += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);

	/* Displayed Modifier Bonuses (Un-inflate stat bonuses) */
	p_ptr->dis_to_a += ((int)(adj_dex_ta[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->dis_to_d += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
	p_ptr->dis_to_h += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->dis_to_h += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);

	/* Evaluate monster AC (if skin or armor etc) */
	if (p_ptr->body_monster) {
		body = (r_ptr->body_parts[BODY_HEAD] ? 1 : 0)
			+ (r_ptr->body_parts[BODY_TORSO] ? 3 : 0)
			+ (r_ptr->body_parts[BODY_ARMS] ? 2 : 0)
			+ (r_ptr->body_parts[BODY_LEGS] ? 1 : 0);

		toac = r_ptr->ac * 14 / (7 + body);
		/* p_ptr->ac += toac;
		p_ptr->dis_ac += toac; - similar to HP calculation: */
		if (toac < (p_ptr->ac + p_ptr->to_a)) {
			p_ptr->ac = (p_ptr->ac * 3) / 4;
			p_ptr->to_a = ((p_ptr->to_a * 3) + (toac * 1)) / 4;
			p_ptr->dis_ac = (p_ptr->dis_ac * 3) / 4;
			p_ptr->dis_to_a = ((p_ptr->dis_to_a * 3) + (toac * 1)) / 4;
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

	if (p_ptr->mode & MODE_HELL)
	{
		if (p_ptr->dis_to_a > 0) p_ptr->dis_to_a /= 2;
		if (p_ptr->to_a > 0) p_ptr->to_a /= 2;
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
	if (hold < o_ptr->weight / 10)
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
			/* Isn't 4 shots/turn too small? Not with art ammo */
			p_ptr->num_fire += (get_skill(p_ptr, archery) / 16);
//				+ get_skill_scale(p_ptr, SKILL_ARCHERY, 1);
			p_ptr->xtra_might += (get_skill(p_ptr, archery) / 50);
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
			get_skill_scale(p_ptr, SKILL_DIG, 300) / 100;
	}


	/* Examine the "main weapon" */
	o_ptr = &p_ptr->inventory[INVEN_WIELD];

	/* Assume not heavy */
	p_ptr->heavy_wield = FALSE;

	/* It is hard to hold a heavy weapon */
	if (hold < o_ptr->weight / 10)
	{
		/* Hard to wield a heavy weapon */
		p_ptr->to_h += 2 * (hold - o_ptr->weight / 10);
		p_ptr->dis_to_h += 2 * (hold - o_ptr->weight / 10);

		/* Heavy weapon */
		p_ptr->heavy_wield = TRUE;
	}


	/* Normal weapons */
	if (o_ptr->k_idx && !p_ptr->heavy_wield)
	{
		p_ptr->num_blow = calc_blows(Ind, o_ptr);
		p_ptr->num_blow += p_ptr->extra_blows;
		/* Boost blows with masteries */
		if (get_weaponmastery_skill(p_ptr) != -1)
		{
			int lev = get_skill(p_ptr, get_weaponmastery_skill(p_ptr));

			p_ptr->to_h_melee += lev;
			p_ptr->to_d_melee += lev / 2;
//			p_ptr->num_blow += get_skill_scale(p_ptr, get_weaponmastery_skill(p_ptr), 2);

		}
	}

	/* Different calculation for monks with empty hands */
#if 1 // DGDGDGDG -- no more monks for the time being
//	if (p_ptr->pclass == CLASS_MONK)
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS) && !o_ptr->k_idx &&
		!(p_ptr->inventory[INVEN_BOW].k_idx))
	{
		int marts = get_skill_scale(p_ptr, SKILL_MARTIAL_ARTS, 50);
		p_ptr->num_blow = 0;

		if (marts >  9) p_ptr->num_blow++;
		if (marts > 19) p_ptr->num_blow++;
		if (marts > 29) p_ptr->num_blow++;
		if (marts > 34) p_ptr->num_blow++;
		if (marts > 39) p_ptr->num_blow++;
		if (marts > 44) p_ptr->num_blow++;
		if (marts > 49) p_ptr->num_blow++;

		if (monk_heavy_armor(p_ptr)) p_ptr->num_blow /= 2;

		p_ptr->num_blow += 1 + p_ptr->extra_blows;

		if (!monk_heavy_armor(p_ptr))
		{
/*			p_ptr->to_h += (marts / 3) * 2;
			p_ptr->to_d += (marts / 3);

			p_ptr->dis_to_h += (marts / 3) * 2;
			p_ptr->dis_to_d += (marts / 3);
*/
			p_ptr->to_h_melee += (marts / 3) * 2;
			p_ptr->to_d_melee += (marts / 3);
		}
	}
#endif

	/* Hell mode is HARD */
	if ((p_ptr->mode & MODE_HELL) && (p_ptr->num_blow > 1)) p_ptr->num_blow--;

	/* A perma_cursed weapon stays even in weapon-less body form, reduce blows for that: */
	if ((p_ptr->inventory[INVEN_WIELD].k_idx) &&
	    (!r_info[p_ptr->body_monster].body_parts[BODY_WEAPON]) &&
	    (p_ptr->num_blow > 1)) p_ptr->num_blow = 1;

	/* Combat bonus to damage */
	if (get_skill(p_ptr, SKILL_COMBAT))
	{
		int lev = get_skill_scale(p_ptr, SKILL_COMBAT, 10);

		p_ptr->to_h += lev;
		p_ptr->dis_to_h += lev;
		p_ptr->to_d += lev;
		p_ptr->dis_to_d += lev;
	}

	/* Combat bonus to damage */
	if (get_skill(p_ptr, SKILL_MASTERY))
	{
		int lev = get_skill(p_ptr, SKILL_MASTERY);

/*		p_ptr->to_h += lev / 5;
		p_ptr->dis_to_h += lev / 5;
		p_ptr->to_d += lev / 5;
		p_ptr->dis_to_d += lev / 5;
*/		p_ptr->to_h_melee += lev / 3;
		p_ptr->to_d_melee += lev / 5;
	}

	if (get_skill(p_ptr, SKILL_DODGE))
//	if (!(r_ptr->flags1 & RF1_NEVER_MOVE));		// not for now
	{
		/* use a long var temporarily to handle v.high total_weight */
		long temp_chance;

		/* Get the armor weight */
		int cur_wgt = armour_weight(p_ptr);

		/* Base dodge chance */
		temp_chance = get_skill_scale(p_ptr, SKILL_DODGE, 150);

		/* Armor weight bonus/penalty */
//		p_ptr->dodge_chance -= cur_wgt * 2;
		temp_chance -= cur_wgt;		/* XXX adjust me */

		/* Encumberance bonus/penalty */
		temp_chance -= p_ptr->total_weight / 100;

		/* Penalty for bad conditions */
		temp_chance -= UNAWARENESS(p_ptr);

		/* write long back to int */
		p_ptr->dodge_chance = temp_chance;

		/* Never below 0 */
		if (p_ptr->dodge_chance < 0) p_ptr->dodge_chance = 0;
	}

	/* Assume okay */
	p_ptr->icky_wield = FALSE;
	p_ptr->awkward_wield = FALSE;

	/* 2handed weapon and shield = less damage */
//	if (inventory[INVEN_WIELD + i].k_idx && inventory[INVEN_ARM + i].k_idx)
	if (p_ptr->inventory[INVEN_WIELD].k_idx && p_ptr->inventory[INVEN_ARM].k_idx)
	{
		/* Extract the item flags */
		object_flags(&p_ptr->inventory[INVEN_WIELD], &f1, &f2, &f3, &f4, &f5, &esp);

		if (f4 & TR4_COULD2H)
		{                
			/* Reduce the real bonuses */
			if (p_ptr->to_h > 0) p_ptr->to_h = (3 * p_ptr->to_h) / 5;
			if (p_ptr->to_d > 0) p_ptr->to_d = (3 * p_ptr->to_d) / 5;

			/* Reduce the mental bonuses */
			if (p_ptr->dis_to_h > 0) p_ptr->dis_to_h = (3 * p_ptr->dis_to_h) / 5;
			if (p_ptr->dis_to_d > 0) p_ptr->dis_to_d = (3 * p_ptr->dis_to_d) / 5;
			
			/* Reduce the weaponmastery bonuses */
			if (p_ptr->to_h_melee > 0) p_ptr->to_h_melee = (3 * p_ptr->to_h_melee) / 5;
			if (p_ptr->to_d_melee > 0) p_ptr->to_d_melee = (3 * p_ptr->to_d_melee) / 5;

			p_ptr->awkward_wield = TRUE;
		}
	}

	/* Priest weapon penalty for non-blessed edged weapons */
//	if ((p_ptr->pclass == 2) && (!p_ptr->bless_blade) &&
//	if ((get_skill(p_ptr, SKILL_PRAY)) && (!p_ptr->bless_blade) &&
	if ((p_ptr->pclass == CLASS_PRIEST) && (!p_ptr->bless_blade) &&
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
			if (j) n++;

			/* Hack -- weaponless combat */
/*			if (wepless && j)
			{
				j *= 2;
			}
*/			d += (j * 2);
		}
		/* At least have 1 blow (although this line is not needed anymore) */
		if (n == 0) n = 1;

		/*d = (d / 2) / n;	// 8 // 7
		p_ptr->to_d += d;
		p_ptr->dis_to_d += d; - similar to HP: */

		/* Divide by player blow number instead of
		monster blow number :
		//d /= n;*/
		//d /= ((p_ptr->num_blows > 0) ? p_ptr->num_blows : 1);

		/* GWoP: 472, GB: 270, Green DR: 96 */
		/* Quarter the damage and cap against 150 (unreachable though)
		- even The Destroyer form would reach just 138 ;) */
		d /= 4;

		/* Cap the to-dam if it's too great */
//too lame:	if (d > 0) d = (15000 / ((10000 / d) + 100)) + 1;
		if (d > 0) d = (15000 / ((15000 / d) + 65)) + 1;
/*too powerful:	if (d > 0) d = (25000 / ((10000 / d) + 100)) + 1;
		if (d > 0) d = (20000 / ((10000 / d) + 100)) + 1;*/

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

	/* Redraw plusses to hit/damage if necessary */
	if ((p_ptr->dis_to_h != old_dis_to_h) || (p_ptr->dis_to_d != old_dis_to_d))
	{
		/* Redraw plusses */
		p_ptr->redraw |= (PR_PLUSSES);
	}

	/* Affect Skill -- stealth (bonus one) */
	p_ptr->skill_stl += 1;

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
	p_ptr->skill_srh += (get_skill_scale(p_ptr, SKILL_SNEAKINESS, p_ptr->cp_ptr->x_srh * 5)) + get_skill(p_ptr, SKILL_SNEAKINESS);

	/* Affect Skill -- search frequency (Level, by Class) */
	p_ptr->skill_fos += (get_skill_scale(p_ptr, SKILL_SNEAKINESS, p_ptr->cp_ptr->x_fos * 5)) + get_skill(p_ptr, SKILL_SNEAKINESS);


#endif	// 0

	/* Affect Skill -- combat (normal) (Level, by Class) */
        p_ptr->skill_thn += (p_ptr->cp_ptr->x_thn * (((7 * get_skill(p_ptr, SKILL_MASTERY)) + (3 * get_skill(p_ptr, SKILL_COMBAT))) / 10) / 10);

	/* Affect Skill -- combat (shooting) (Level, by Class) */
	p_ptr->skill_thb += (p_ptr->cp_ptr->x_thb * (((7 * get_skill(p_ptr, SKILL_ARCHERY)) + (3 * get_skill(p_ptr, SKILL_COMBAT))) / 10) / 10);

	/* Affect Skill -- combat (throwing) (Level, by Class) */
	p_ptr->skill_tht += (p_ptr->cp_ptr->x_thb * get_skill_scale(p_ptr, SKILL_COMBAT, 10));

#if 0	// moved to player_invis
	if (p_ptr->aggravate)
	{
		if (p_ptr->tim_invisibility || magik(1))	/* don't be too noisy */
			msg_print(Ind, "You somewhat feel your presence is known.");
		p_ptr->invis = 0;
		p_ptr->tim_invisibility = 0;
		p_ptr->tim_invis_power = 0;
	}
	else
	{
#if 0 // DGDGDGDG -- no more monks for the time being
                if (p_ptr->pclass == CLASS_MONK)
		{
			p_ptr->skill_stl += (p_ptr->lev/10); /* give a stealth bonus */
                }
#endif
	}
#endif	// 0


	/* Limit Skill -- stealth from 0 to 30 */
	if (p_ptr->skill_stl > 30) p_ptr->skill_stl = 30;
	if (p_ptr->skill_stl < 0) p_ptr->skill_stl = 0;

	/* Limit Skill -- digging from 1 up */
	if (p_ptr->skill_dig < 1) p_ptr->skill_dig = 1;

	if ((p_ptr->anti_magic) && (p_ptr->skill_sav < 95)) p_ptr->skill_sav = 95;

	/* Limit Skill -- saving throw upto 95 */
	if (p_ptr->skill_sav > 95) p_ptr->skill_sav = 95;


	/* Take note when "heavy bow" changes */
	if (p_ptr->old_heavy_shoot != p_ptr->heavy_shoot)
	{
		/* Message */
		if (p_ptr->heavy_shoot)
		{
			msg_print(Ind, "You have trouble wielding such a heavy bow.");
		}
		else if (p_ptr->inventory[INVEN_BOW].k_idx)
		{
			msg_print(Ind, "You have no trouble wielding your bow.");
		}
		else
		{
			msg_print(Ind, "You feel relieved to put down your heavy bow.");
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
			msg_print(Ind, "You have trouble wielding such a heavy weapon.");
		}
		else if (p_ptr->inventory[INVEN_WIELD].k_idx)
		{
			msg_print(Ind, "You have no trouble wielding your weapon.");
		}
		else
		{
			msg_print(Ind, "You feel relieved to put down your heavy weapon.");
		}

		/* Save it */
		p_ptr->old_heavy_wield = p_ptr->heavy_wield;
	}


	/* Take note when "illegal weapon" changes */
	if (p_ptr->old_icky_wield != p_ptr->icky_wield)
	{
		/* Message */
		if (p_ptr->icky_wield)
		{
			msg_print(Ind, "You do not feel comfortable with your weapon.");
		}
		else if (p_ptr->inventory[INVEN_WIELD].k_idx)
		{
			msg_print(Ind, "You feel comfortable with your weapon.");
		}
		else
		{
			msg_print(Ind, "You feel more comfortable after removing your weapon.");
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
			msg_print(Ind, "You find it hard to fight with your weapon and shield.");
		}
		else if (p_ptr->inventory[INVEN_WIELD].k_idx)
		{
			msg_print(Ind, "You feel comfortable with your weapon.");
		}
		else if (!p_ptr->inventory[INVEN_ARM].k_idx)
		{
			msg_print(Ind, "You feel more dexterous after removing your shield.");
		}

		/* Save it */
		p_ptr->old_awkward_wield = p_ptr->awkward_wield;
	}

	/* resistance to fire cancel sensibility to fire */
	if(p_ptr->resist_fire || p_ptr->oppose_fire || p_ptr->immune_fire)
		p_ptr->sensible_fire=FALSE;
	/* resistance to cold cancel sensibility to cold */
	if(p_ptr->resist_cold || p_ptr->oppose_cold || p_ptr->immune_cold)
		p_ptr->sensible_cold=FALSE;
	/* resistance to electricity cancel sensibility to fire */
	if(p_ptr->resist_elec || p_ptr->oppose_elec || p_ptr->immune_elec)
		p_ptr->sensible_elec=FALSE;
	/* resistance to acid cancel sensibility to fire */
	if(p_ptr->resist_acid || p_ptr->oppose_acid || p_ptr->immune_acid)
		p_ptr->sensible_acid=FALSE;

	/* Limit speed penalty from total_weight */
	if (p_ptr->pspeed < 10) p_ptr->pspeed = 10;

	/* XXX - Always resend skills */
	p_ptr->redraw |= (PR_SKILLS);
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
				p_ptr->s_info[i].touched=FALSE;
                        }
                }
		Send_skill_points(Ind);
	}

	if (p_ptr->update & PU_BONUS)
	{
		p_ptr->update &= ~(PU_BONUS);
		calc_bonuses(Ind);
		/* Take off what is no more usable */
		do_takeoff_impossible(Ind);
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
		p_ptr->redraw &= ~(PR_MAP);
		prt_map(Ind);
	}


	if (p_ptr->redraw & PR_BASIC)
	{
		p_ptr->redraw &= ~(PR_BASIC);
		p_ptr->redraw &= ~(PR_MISC | PR_TITLE | PR_STATS);
		p_ptr->redraw &= ~(PR_LEV | PR_EXP | PR_GOLD);
		p_ptr->redraw &= ~(PR_ARMOR | PR_HP | PR_MANA);
		p_ptr->redraw &= ~(PR_DEPTH | PR_HEALTH);
		prt_frame_basic(Ind);
	}

	if (p_ptr->redraw & PR_MISC)
	{
		p_ptr->redraw &= ~(PR_MISC);
		Send_char_info(Ind, p_ptr->prace, p_ptr->pclass, p_ptr->male);
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


