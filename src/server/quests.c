/* This file is for providing a framework for quests,
   allowing easy addition/editing/removal of them.
   I hope I covered all sorts of quests, even though some quests might be a bit
   clunky to implement (quest needs to 'quietly' spawn a followup quest etc),
   otherwise let me know!
          - C. Blue

   Note: This is a new approach. Not to be confused with old
         q_list[], quest_type, quest[] and plots[] code.

   Quests can make use of Send_request_key/str/num/cfr/abort()
   functions to provide the possibility of dialogue with npcs.
   Quest npcs should be neutral and non-attackable (at least
   not on accident) and should probably not show up on ESP,
   neither should quest objects show up on object detection.


   Note about current mechanics:
   To make a quest spawn a new target questor ('Find dude X in the dungeon and talk to him')
   it is currently necessary to spawn a new quest via activate_quest[], have the player
   auto_accept_quiet it and silently terminate the current quest. The new quest should
   probably have almost the same title too in case the player can review the title so as to
   not confuse him..<--><------>maybe this could be improved on in the future :-p
   how to possibly improve: spawn sub questors with their own dialogues.
*/

//? #define SERVER
#include "angband.h"


/* set log level (0 to disable, 1 for normal logging, 2 for debug logging) */
#define QDEBUG 2

/* Disable a quest on error? */
//#define QERROR_DISABLE
/* Otherwise just put it on this cooldown (minutes) */
#define QERROR_COOLDOWN 30

/* Default cooldown in minutes. */
#define QI_COOLDOWN_DEFAULT 5


static void quest_goal_check_reward(int pInd, int q_idx);
static bool quest_goal_check(int pInd, int q_idx, bool interacting);
static void quest_imprint_stage(int Ind, int q_idx, int py_q_idx);
static void quest_dialogue(int Ind, int q_idx, int questor_idx, bool repeat, bool interact_acquire);


/* error messages for quest_acquire() */
static cptr qi_msg_deactivated = "\377sThis quest is currently unavailable.";
static cptr qi_msg_cooldown = "\377yYou cannot acquire this quest again just yet.";
static cptr qi_msg_prereq = "\377yYou have not completed the required prerequisite quests yet.";
static cptr qi_msg_minlev = "\377yYour level is too low to acquire this quest.";
static cptr qi_msg_maxlev = "\377oYour level is too high to acquire this quest.";
static cptr qi_msg_race = "\377oYour race is not eligible to acquire this quest.";
static cptr qi_msg_class = "\377oYour class is not eligible to acquire this quest.";
static cptr qi_msg_truebat = "\377oYou must be a born fruit bat to acquire this quest.";
static cptr qi_msg_bat = "\377yYou must be a fruit bat to acquire this quest.";
static cptr qi_msg_form = "\377yYou cannot acquire this quest in your current form.";
static cptr qi_msg_mode = "\377oYour character mode is not eligible for acquiring this quest.";
static cptr qi_msg_done = "\377oYou cannot acquire this quest again.";
static cptr qi_msg_max = "\377yYou are already pursuing the maximum possible number of concurrent quests.";


/* Called every minute to check which quests to activate/deactivate depending on time/daytime */
void process_quests(void) {
	quest_info *q_ptr;
	int i, j;
	bool night = IS_NIGHT_RAW, day = !night, morning = IS_MORNING, forenoon = IS_FORENOON, noon = IS_NOON;
	bool afternoon = IS_AFTERNOON, evening = IS_EVENING, midnight = IS_MIDNIGHT, deepnight = IS_DEEPNIGHT;
	int minute = bst(MINUTE, turn), hour = bst(HOUR, turn), stage;
	bool active;

	for (i = 0; i < max_q_idx; i++) {
		q_ptr = &q_info[i];

		if (q_ptr->cur_cooldown) q_ptr->cur_cooldown--;
		if (q_ptr->disabled || q_ptr->cur_cooldown) continue;

		stage = q_ptr->stage;

		/* check if quest should be active */
		active = FALSE;

		if (q_ptr->night && night) active = TRUE; /* don't include pseudo-night like for Halloween/Newyearseve */
		if (q_ptr->day && day) active = TRUE;

		/* specialty: if day is true, then these can falsify it again */
		if (!q_ptr->day) {
			if (q_ptr->morning && morning) active = TRUE;
			if (q_ptr->forenoon && forenoon) active = TRUE;
			if (q_ptr->noon && noon) active = TRUE;
			if (q_ptr->afternoon && afternoon) active = TRUE;
			if (q_ptr->evening && evening) active = TRUE;
		} else {
			if (!q_ptr->morning && morning) active = FALSE;
			if (!q_ptr->forenoon && forenoon) active = FALSE;
			if (!q_ptr->noon && noon) active = FALSE;
			if (!q_ptr->afternoon && afternoon) active = FALSE;
			if (!q_ptr->evening && evening) active = FALSE;
		}
		/* specialty: if night is true, then these can falsify it again */
		if (!q_ptr->night) {
			if (q_ptr->evening && evening) active = TRUE;
			if (q_ptr->midnight && midnight) active = TRUE;
			if (q_ptr->deepnight && deepnight) active = TRUE;
		} else {
			if (!q_ptr->evening && evening) active = FALSE;
			if (!q_ptr->midnight && midnight) active = FALSE;
			if (!q_ptr->deepnight && deepnight) active = FALSE;
		}

		/* very specific time? (in absolute daily minutes) */
		if (q_ptr->time_start != -1) {
			if (minute >= q_ptr->time_start) {
				if (q_ptr->time_stop == -1 ||
				    minute < q_ptr->time_stop)
					active = TRUE;
			}
		}

		/* quest is inactive and must be activated */
		if (!q_ptr->active && active)
			quest_activate(i);
		/* quest is active and must be deactivated */
		else if (q_ptr->active && !active)
			quest_deactivate(i);


		/* handle individual cooldowns here too for now.
		   NOTE: this means player has to stay online for cooldown to expire, hmm */
		for (j = 1; j <= NumPlayers; j++)
			if (Players[j]->quest_cooldown[i])
				Players[j]->quest_cooldown[i]--;


		/* handle automatically timed stage actions */
		if (q_ptr->timed_stage_countdown[stage] < 0) {
			if (hour == -q_ptr->timed_stage_countdown[stage])
				quest_set_stage(0, i, q_ptr->timed_stage_countdown_stage[stage], q_ptr->timed_stage_countdown_quiet[stage]);
		} else if (q_ptr->timed_stage_countdown[stage] > 0) {
			q_ptr->timed_stage_countdown[stage]--;
			if (!q_ptr->timed_stage_countdown[stage])
				quest_set_stage(0, i, q_ptr->timed_stage_countdown_stage[stage], q_ptr->timed_stage_countdown_quiet[stage]);
		}
	}
}

/* Spawn questor, prepare sector/floor, make things static if requested, etc. */
void quest_activate(int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, q;
	cave_type **zcave, *c_ptr;
	monster_race *r_ptr, *rbase_ptr;
	monster_type *m_ptr;

	/* data written back to q_info[] */
	struct worldpos wpos = {63, 63, 0}; //default for cases of acute paranoia
	int x, y;


#if QDEBUG > 0
	for (i = 1;i <= NumPlayers; i++)
		if (is_admin(Players[i]))
			msg_format(i, "Quest '%s' (%d, '%s') activated.", q_name + q_ptr->name, q_idx, q_ptr->codename);
	s_printf("%s QUEST_ACTIVATE: '%s' ('%s' by '%s')\n", showtime(), q_name + q_ptr->name, q_ptr->codename, q_ptr->creator);
#endif
	q_ptr->active = TRUE;



	/* check 'L:' info for starting location -
	   for now only support 1 starting location, ie use the first one that gets tested to be eligible */
#if QDEBUG > 1
s_printf("QIDX %d. SWPOS,XY: %d,%d,%d - %d,%d\n", q_idx, q_ptr->start_wpos.wx, q_ptr->start_wpos.wy, q_ptr->start_wpos.wz, q_ptr->start_x, q_ptr->start_y);
s_printf("SLOCT, STAR: %d,%d\n", q_ptr->s_location_type, q_ptr->s_towns_array);
#endif
	if (q_ptr->start_wpos.wx != -1) {
		/* exact questor starting wpos  */
		wpos.wx = q_ptr->start_wpos.wx;
		wpos.wy = q_ptr->start_wpos.wy;
		wpos.wz = q_ptr->start_wpos.wz;
	} else if (!q_ptr->s_location_type) return;
	else if ((q_ptr->s_location_type & QI_SLOC_TOWN)) {
		if ((q_ptr->s_towns_array & QI_STOWN_BREE)) {
			wpos.wx = 32;
			wpos.wy = 32;
		} else if ((q_ptr->s_towns_array & QI_STOWN_GONDOLIN)) {
			wpos.wx = 27;
			wpos.wy = 13;
		} else if ((q_ptr->s_towns_array & QI_STOWN_MINASANOR)) {
			wpos.wx = 25;
			wpos.wy = 58;
		} else if ((q_ptr->s_towns_array & QI_STOWN_LOTHLORIEN)) {
			wpos.wx = 59;
			wpos.wy = 51;
		} else if ((q_ptr->s_towns_array & QI_STOWN_KHAZADDUM)) {
			wpos.wx = 26;
			wpos.wy = 5;
		} else if ((q_ptr->s_towns_array & QI_STOWN_WILD)) {
		} else if ((q_ptr->s_towns_array & QI_STOWN_DUNGEON)) {
		} else if ((q_ptr->s_towns_array & QI_STOWN_IDDC)) {
		} else if ((q_ptr->s_towns_array & QI_STOWN_IDDC_FIXED)) {
		} else return; //paranoia
	} else if ((q_ptr->s_location_type & QI_SLOC_SURFACE)) {
		if ((q_ptr->s_terrains & RF8_WILD_TOO)) { /* all terrains are valid */
		} else if ((q_ptr->s_terrains & RF8_WILD_SHORE)) {
		} else if ((q_ptr->s_terrains & RF8_WILD_SHORE)) {
		} else if ((q_ptr->s_terrains & RF8_WILD_OCEAN)) {
		} else if ((q_ptr->s_terrains & RF8_WILD_WASTE)) {
		} else if ((q_ptr->s_terrains & RF8_WILD_WOOD)) {
		} else if ((q_ptr->s_terrains & RF8_WILD_VOLCANO)) {
		} else if ((q_ptr->s_terrains & RF8_WILD_MOUNTAIN)) {
		} else if ((q_ptr->s_terrains & RF8_WILD_GRASS)) {
		} else if ((q_ptr->s_terrains & RF8_WILD_DESERT)) {
		} else if ((q_ptr->s_terrains & RF8_WILD_ICE)) {
		} else if ((q_ptr->s_terrains & RF8_WILD_SWAMP)) {
		} else return; //paranoia
	} else if ((q_ptr->s_location_type & QI_SLOC_DUNGEON)) {
	} else if ((q_ptr->s_location_type & QI_SLOC_TOWER)) {
	} else return; //paranoia

	/* TEMP: initialise questor starting locations all onto this one.
	   TODO: each questor gets its own pos */
	for (i = 0; i < QI_QUESTORS; i++) {
		q_ptr->current_wpos[i].wx = wpos.wx;
		q_ptr->current_wpos[i].wy = wpos.wy;
		q_ptr->current_wpos[i].wz = wpos.wz;
	}


	/* Allocate & generate cave */
	if (!(zcave = getcave(&wpos))) {
		//dealloc_dungeon_level(&wpos);
		alloc_dungeon_level(&wpos);
		generate_cave(&wpos, NULL);
		zcave = getcave(&wpos);
	}
	if (q_ptr->static_floor) new_players_on_depth(&wpos, 1, TRUE);


	/* Specified exact starting location within given wpos? */
	if (q_ptr->start_x != -1) {
		x = q_ptr->start_x;
		y = q_ptr->start_y;
	} else {
		/* find a random spawn loc */
		i = 1000; //tries
		while (i) {
			i--;
			/* not too close to level borders maybe */
			x = rand_int(MAX_WID - 6) + 3;
			y = rand_int(MAX_HGT - 6) + 3;
			if (!cave_naked_bold(zcave, y, x)) continue;
			break;
		}
		if (!i) {
			s_printf("QUEST_CANCELLED: No free questor spawn location.\n");
			q_ptr->active = FALSE;
#ifdef QERROR_DISABLE
			q_ptr->disabled = TRUE;
#else
			q_ptr->cur_cooldown = QERROR_COOLDOWN;
#endif
			return;
		}
	}

	c_ptr = &zcave[y][x];

	/* TEMP: initialise questor starting locations all onto this one.
	   TODO: each questor gets its own pos */
	for (i = 0; i < QI_QUESTORS; i++) {
		q_ptr->current_x[i] = x;
		q_ptr->current_y[i] = y;
	}


	/* generate questor 'monster' */
	for (q = 0; q < q_ptr->questors; q++) {
		if (!q_ptr->questor_ridx[q]) break;

		c_ptr->m_idx = m_pop();
		if (!c_ptr->m_idx) {
			s_printf("QUEST_CANCELLED: No free monster index to pop questor.\n");
			q_ptr->active = FALSE
#ifdef QERROR_DISABLE
			q_ptr->disabled = TRUE;
#else
			q_ptr->cur_cooldown = QERROR_COOLDOWN;
#endif
			return;
		}
		m_ptr = &m_list[c_ptr->m_idx];
		MAKE(m_ptr->r_ptr, monster_race);
		r_ptr = m_ptr->r_ptr;
		rbase_ptr = &r_info[q_ptr->questor_ridx[q]];

		m_ptr->questor = TRUE;
		m_ptr->questor_idx = 0; /* 1st */
		m_ptr->quest = q_idx;
		m_ptr->r_idx = q_ptr->questor_ridx[q];
		/* m_ptr->special = TRUE; --nope, this is unfortunately too much golem'ized.
		   Need code cleanup!! Maybe rename it to m_ptr->golem and add a new m_ptr->special. */
		r_ptr->extra = 0;
		m_ptr->mind = 0;
		m_ptr->owner = 0;

		r_ptr->flags1 = rbase_ptr->flags1;
		r_ptr->flags2 = rbase_ptr->flags2;
		r_ptr->flags3 = rbase_ptr->flags3;
		r_ptr->flags4 = rbase_ptr->flags4;
		r_ptr->flags5 = rbase_ptr->flags5;
		r_ptr->flags6 = rbase_ptr->flags6;
		r_ptr->flags7 = rbase_ptr->flags7;
		r_ptr->flags8 = rbase_ptr->flags8;
		r_ptr->flags9 = rbase_ptr->flags9;
		r_ptr->flags0 = rbase_ptr->flags0;

		r_ptr->flags1 |= RF1_FORCE_MAXHP;
		r_ptr->flags3 |= RF3_RES_TELE | RF3_RES_NEXU;
		r_ptr->flags7 |= RF7_NO_TARGET | RF7_NEVER_ACT;
		if (q_ptr->questor_invincible) r_ptr->flags7 |= RF7_NO_DEATH; //for now we just use NO_DEATH flag for invincibility
		r_ptr->flags8 |= RF8_GENO_PERSIST | RF8_GENO_NO_THIN | RF8_ALLOW_RUNNING | RF8_NO_AUTORET;
		r_ptr->flags9 |= RF9_IM_TELE;

		r_ptr->text = 0;
		r_ptr->name = rbase_ptr->name;
		m_ptr->fx = x;
		m_ptr->fy = y;

		r_ptr->d_attr = q_ptr->questor_rattr[q];
		r_ptr->d_char = q_ptr->questor_rchar[q];
		r_ptr->x_attr = q_ptr->questor_rattr[q];
		r_ptr->x_char = q_ptr->questor_rchar[q];
	
		r_ptr->aaf = rbase_ptr->aaf;
		r_ptr->mexp = rbase_ptr->mexp;
		r_ptr->hdice = rbase_ptr->hdice;
		r_ptr->hside = rbase_ptr->hside;
	
		m_ptr->maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
		m_ptr->hp = m_ptr->maxhp;
		m_ptr->org_maxhp = m_ptr->maxhp; /* CON */
		m_ptr->speed = rbase_ptr->speed;
		m_ptr->mspeed = m_ptr->speed;
		m_ptr->ac = rbase_ptr->ac;
		m_ptr->org_ac = m_ptr->ac; /* DEX */

		m_ptr->level = rbase_ptr->level;
		m_ptr->exp = MONSTER_EXP(m_ptr->level);

		for (i = 0; i < 4; i++) {
			m_ptr->blow[i].effect = rbase_ptr->blow[i].effect;
			m_ptr->blow[i].method = rbase_ptr->blow[i].method;
			m_ptr->blow[i].d_dice = rbase_ptr->blow[i].d_dice;
			m_ptr->blow[i].d_side = rbase_ptr->blow[i].d_side;

			m_ptr->blow[i].org_d_dice = rbase_ptr->blow[i].d_dice;
			m_ptr->blow[i].org_d_side = rbase_ptr->blow[i].d_side;
		}

		r_ptr->freq_innate = rbase_ptr->freq_innate;
		r_ptr->freq_spell = rbase_ptr->freq_spell;

#ifdef MONSTER_ASTAR
		if (r_ptr->flags0 & RF0_ASTAR) {
			/* search for an available A* table to use */
			for (i = 0; i < ASTAR_MAX_INSTANCES; i++) {
				/* found an available instance? */
				if (astar_info_open[i].m_idx == -1) {
					astar_info_open[i].m_idx = c_ptr->m_idx;
					astar_info_open[i].nodes = 0; /* init: start with empty set of nodes */
					astar_info_closed[i].nodes = 0; /* init: start with empty set of nodes */
					m_ptr->astar_idx = i;
					break;
				}
			}
			/* no instance available? Mark us (-1) to use normal movement instead */
			if (i == ASTAR_MAX_INSTANCES) {
				m_ptr->astar_idx = -1;
				/* cancel quest because of this? */
				s_printf("QUEST_CANCELLED: No free A* index for questor.\n");
				q_ptr->active = FALSE;
 #ifdef QERROR_DISABLE
				q_ptr->disabled = TRUE;
 #else
				q_ptr->cur_cooldown = QERROR_COOLDOWN;
 #endif
				return;
			}
		}
#endif

		m_ptr->clone = 0;
		m_ptr->cdis = 0;
		wpcopy(&m_ptr->wpos, &wpos);

		m_ptr->stunned = 0;
		m_ptr->confused = 0;
		m_ptr->monfear = 0;
		//r_ptr->sleep = rbase_ptr->sleep;
		r_ptr->sleep = 0;

		m_ptr->questor_invincible = q_ptr->questor_invincible[q]; //for now handled by RF7_NO_DEATH
		m_ptr->questor_hostile = FALSE;
		q_ptr->questor_m_idx[q] = c_ptr->m_idx;

		update_mon(c_ptr->m_idx, TRUE);
#if QDEBUG > 1
		s_printf("QUEST_SPAWNED: Questor '%s' at %d,%d,%d - %d,%d.\n", q_ptr->questor_name[0], wpos.wx, wpos.wy, wpos.wz, x, y);
#endif

		q_ptr->talk_focus[q] = 0;
	}
	
	/* Initialise with starting stage 0 */
	q_ptr->start_turn = turn;
	q_ptr->stage = -1;
	quest_set_stage(0, q_idx, 0, FALSE);
}

/* Despawn questor, unstatic sector/floor, etc. */
//TODO: implement multiple questors
void quest_deactivate(int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	//int i;
	cave_type **zcave, *c_ptr;
	//monster_race *r_ptr;
	//monster_type *m_ptr;

	/* data reread from q_info[] */
	struct worldpos wpos;
	//int qx, qy;

#if QDEBUG > 0
	s_printf("%s QUEST_DEACTIVATE: '%s' ('%s' by '%s')\n", showtime(), q_name + q_ptr->name, q_ptr->codename, q_ptr->creator);
#endif
	q_ptr->active = FALSE;


	/* get quest information */
	wpos.wx = q_ptr->current_wpos[0].wx;
	wpos.wy = q_ptr->current_wpos[0].wy;
	wpos.wz = q_ptr->current_wpos[0].wz;

	/* Allocate & generate cave */
	if (!(zcave = getcave(&wpos))) {
		//dealloc_dungeon_level(&wpos);
		alloc_dungeon_level(&wpos);
		generate_cave(&wpos, NULL);
		zcave = getcave(&wpos);
	}
	c_ptr = &zcave[q_ptr->current_y[0]][q_ptr->current_x[0]];

	/* unmake quest */
#if QDEBUG > 1
s_printf("deleting questor %d at %d,%d,%d - %d,%d\n", c_ptr->m_idx, wpos.wx, wpos.wy, wpos.wz, q_ptr->current_x[0], q_ptr->current_y[0]);
#endif
	if (c_ptr->m_idx) delete_monster_idx(c_ptr->m_idx, TRUE);
	if (q_ptr->static_floor) new_players_on_depth(&wpos, 0, FALSE);
}

/* return a current quest's cooldown. Either just uses q_ptr->cur_cooldown directly for global
   quests, or p_ptr->quest_cooldown for individual quests. */
s16b quest_get_cooldown(int pInd, int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];

	if (pInd && q_ptr->individual) return Players[pInd]->quest_cooldown[q_idx];
	return q_ptr->cur_cooldown;
}

/* set a current quest's cooldown. Either just uses q_ptr->cur_cooldown directly for global
   quests, or p_ptr->quest_cooldown for individual quests. */
void quest_set_cooldown(int pInd, int q_idx, s16b cooldown) {
	quest_info *q_ptr = &q_info[q_idx];

	if (pInd && q_ptr->individual) Players[pInd]->quest_cooldown[q_idx] = cooldown;
	else q_ptr->cur_cooldown = cooldown;
}

/* a quest has successfully ended, clean up */
static void quest_terminate(int pInd, int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, j;

	/* give players credit */
	if (pInd && q_ptr->individual) {
#if QDEBUG > 0
		s_printf("%s QUEST_TERMINATE_INDIVIDUAL: '%s' ('%s' by '%s')\n", showtime(), q_name + q_ptr->name, q_ptr->codename, q_ptr->creator);
#endif
		p_ptr = Players[pInd];

		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (p_ptr->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) return; //oops?

		if (p_ptr->quest_done[q_idx] < 10000) p_ptr->quest_done[q_idx]++;

		/* he is no longer on the quest, since the quest has finished */
		p_ptr->quest_idx[j] = -1;
		msg_format(pInd, "\374\377uYou have completed the quest \"\377U%s\377u\".", q_name + q_ptr->name);
		msg_print(pInd, "\374 ");

		/* don't respawn the questor *immediately* again, looks silyl */
		if (q_ptr->cooldown == -1) p_ptr->quest_cooldown[q_idx] = QI_COOLDOWN_DEFAULT;
		else p_ptr->quest_cooldown[q_idx] = q_ptr->cooldown;

		/* individual quests don't get cleaned up by deactivation */
		return;
	}

#if QDEBUG > 0
	s_printf("%s QUEST_TERMINATE_GLOBAL: '%s' ('%s' by '%s')\n", showtime(), q_name + q_ptr->name, q_ptr->codename, q_ptr->creator);
#endif
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (p_ptr->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

		if (p_ptr->quest_done[q_idx] < 10000) p_ptr->quest_done[q_idx]++;

		/* he is no longer on the quest, since the quest has finished */
		p_ptr->quest_idx[j] = -1;
		msg_format(i, "\374\377uYou have completed the quest \"\377U%s\377u\".", q_name + q_ptr->name);
		msg_print(i, "\374 ");
	}

	/* don't respawn the questor *immediately* again, looks silyl */
	if (q_ptr->cooldown == -1) q_ptr->cur_cooldown = QI_COOLDOWN_DEFAULT;
	else q_ptr->cur_cooldown = q_ptr->cooldown;

	/* clean up */
	quest_deactivate(q_idx);
}

/* return a current quest goal. Either just uses q_ptr->goals directly for global
   quests, or p_ptr->quest_goals for individual quests. */
static bool quest_get_goal(int pInd, int q_idx, int goal) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, stage = quest_get_stage(pInd, q_idx);

	if (!pInd) return q_ptr->goals[stage][goal]; /* no player? global goal */

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) return q_ptr->goals[stage][goal]; /* player isn't on this quest. return global goal. */

	if (q_ptr->individual) return p_ptr->quest_goals[i][goal]; /* individual quest */
	return q_ptr->goals[stage][goal]; /* global quest */
}

/* return an optional current quest goal. Either just uses q_ptr->goalsopt directly for global
   quests, or p_ptr->quest_goalsopt for individual quests. */
#if 0 /* compiler warning 'unused' */
static bool quest_get_goalopt(int pInd, int q_idx, int goalopt) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, stage = quest_get_stage(pInd, q_idx);

	if (!pInd) return q_ptr->goalsopt[stage][goalopt]; /* no player? global goal */

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) return q_ptr->goalsopt[stage][goalopt]; /* player isn't on this quest. return global goal. */

	if (q_ptr->individual) return p_ptr->quest_goalsopt[i][goalopt]; /* individual quest */
	return q_ptr->goalsopt[stage][goalopt]; /* global quest */
}
#endif

/* return a current quest flag. Either just uses q_ptr->flags directly for global
   quests, or p_ptr->quest_flags for individual quests. */
#if 0 /* compiler warning 'unused' */
static bool quest_get_flag(int pInd, int q_idx, int flag) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i;

	if (!pInd) return ((q_ptr->flags & (0x1 << flag)) != 0); /* no player? global goal */

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) return ((q_ptr->flags & (0x1 << flag)) != 0); /* player isn't on this quest. return global goal. */

	if (q_ptr->individual) return ((p_ptr->quest_flags[i] & (0x1 << flag)) != 0); /* individual quest */
	return ((q_ptr->flags & (0x1 << flag)) != 0); /* global quest */
}
#endif

/* return the current quest stage. Either just uses q_ptr->stage directly for global
   quests, or p_ptr->quest_stage for individual quests. */
s16b quest_get_stage(int pInd, int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i;

	if (!pInd) return q_ptr->stage; /* no player? global stage */

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) return 0; /* player isn't on this quest: pick stage 0, the entry stage */

	if (q_ptr->individual) return p_ptr->quest_stage[i]; /* individual quest */
	return q_ptr->stage; /* global quest */
}

/* according to Z lines, change flags when a quest goal has finally be resolved. */
//TODO for optional goals too..
static void quest_goal_changes_flags(int q_idx, int stage, int goal) {
	quest_info *q_ptr = &q_info[q_idx];

	q_ptr->flags |= (q_ptr->goal_setflags[stage][goal]);
	q_ptr->flags &= ~(q_ptr->goal_clearflags[stage][goal]);
}

/* mark a quest goal as reached.
   Also check if we can now proceed to the next stage or set flags or hand out rewards.
   'nisi' is TRUE for kill/retrieve quests that depend on a delivery.
   This was added for handling flag changes induced by completing stage goals. */
static void quest_set_goal(int pInd, int q_idx, int goal, bool nisi) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, stage = quest_get_stage(pInd, q_idx);

	if (!pInd) {
		if (!q_ptr->goals[stage][goal] || !nisi) q_ptr->goals_nisi[stage][goal] = nisi;
		q_ptr->goals[stage][goal] = TRUE; /* no player? global goal */

		/* change flags according to Z lines? */
		if (!q_ptr->goals_nisi[stage][goal]) quest_goal_changes_flags(q_idx, stage, goal);

		(void)quest_goal_check(0, q_idx, FALSE);
		return;
	}

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) {
		if (!q_ptr->goals[stage][goal] || !nisi) q_ptr->goals_nisi[stage][goal] = nisi;
		q_ptr->goals[stage][goal] = TRUE; /* player isn't on this quest. return global goal. */

		/* change flags according to Z lines? */
		if (!q_ptr->goals_nisi[stage][goal]) quest_goal_changes_flags(q_idx, stage, goal);

		(void)quest_goal_check(0, q_idx, FALSE);
		return;
	}

	if (q_ptr->individual) {
		if (!p_ptr->quest_goals[i][goal] || !nisi) p_ptr->quest_goals_nisi[i][goal] = nisi;
		p_ptr->quest_goals[i][goal] = TRUE; /* individual quest */

		/* change flags according to Z lines? */
		if (!q_ptr->goals_nisi[stage][goal]) quest_goal_changes_flags(q_idx, stage, goal);

		(void)quest_goal_check(pInd, q_idx, FALSE);
		return;
	}

	if (!q_ptr->goals[stage][goal] || !nisi) q_ptr->goals_nisi[stage][goal] = nisi;
	q_ptr->goals[stage][goal] = TRUE; /* global quest */

	/* change flags according to Z lines? */
	if (!q_ptr->goals_nisi[stage][goal]) quest_goal_changes_flags(q_idx, stage, goal);

	/* also check if we can now proceed to the next stage or set flags or hand out rewards */
	(void)quest_goal_check(0, q_idx, FALSE);
}
/* mark a quest goal as no longer reached. ouch. */
static void quest_unset_goal(int pInd, int q_idx, int goal) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, stage = quest_get_stage(pInd, q_idx);

	if (!pInd) {
		q_ptr->goals[stage][goal] = FALSE; /* no player? global goal */
		return;
	}

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) {
		q_ptr->goals[stage][goal] = FALSE; /* player isn't on this quest. return global goal. */
		return;
	}

	if (q_ptr->individual) {
		p_ptr->quest_goals[i][goal] = FALSE; /* individual quest */
		return;
	}

	q_ptr->goals[stage][goal] = FALSE; /* global quest */
}

/* mark an optional quest goal as reached */
#if 0 /* compiler warning 'unused' */
static void quest_set_goalopt(int pInd, int q_idx, int goalopt, bool nisi) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, stage = quest_get_stage(pInd, q_idx);

	if (!pInd) {
		if (!q_ptr->goalsopt[stage][goalopt] || !nisi) q_ptr->goalsopt_nisi[stage][goalopt] = nisi;
		q_ptr->goalsopt[stage][goalopt] = TRUE; /* no player? global goal */
		return;
	}

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) {
		if (!q_ptr->goalsopt[stage][goalopt] || !nisi) q_ptr->goalsopt_nisi[stage][goalopt] = nisi;
		q_ptr->goalsopt[stage][goalopt] = TRUE; /* player isn't on this quest. return global goal. */
		return;
	}

	if (q_ptr->individual) {
		if (!p_ptr->quest_goalsopt[i][goalopt] || !nisi) p_ptr->quest_goalsopt_nisi[i][goalopt] = nisi;
		p_ptr->quest_goalsopt[i][goalopt] = TRUE; /* individual quest */
		return;
	}

	if (!q_ptr->goalsopt[stage][goalopt] || !nisi) q_ptr->goalsopt_nisi[stage][goalopt] = nisi;
	q_ptr->goalsopt[stage][goalopt] = TRUE; /* global quest */

	/* also check if we can now set flags or hand out rewards */
	//TODO: for optional quests..
	//(void)quest_goal_check(pInd, q_idx, FALSE);
}
#endif
/* mark an optional quest goal as no longer reached. ouch. */
#if 0 /* compiler warning 'unused' */
static void quest_unset_goalopt(int pInd, int q_idx, int goalopt) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, stage = quest_get_stage(pInd, q_idx);

	if (!pInd) {
		q_ptr->goalsopt[stage][goalopt] = FALSE; /* no player? global goal */
		return;
	}

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) {
		q_ptr->goalsopt[stage][goalopt] = FALSE; /* player isn't on this quest. return global goal. */
		return;
	}

	if (q_ptr->individual) {
		p_ptr->quest_goalsopt[i][goalopt] = FALSE; /* individual quest */
		return;
	}

	q_ptr->goalsopt[stage][goalopt] = FALSE; /* global quest */
}
#endif

/* set/clear a quest flag ('set': TRUE -> set, FALSE -> clear) */
#if 0 /* compiler warning 'unused' */
static void quest_set_flag(int pInd, int q_idx, int flag, bool set) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i;

	if (!pInd) {
		if (set) q_ptr->flags |= (0x1 << flag); /* no player? global goal */
		else q_ptr->flags &= ~(0x1 << flag);
		return;
	}

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) {
		if (set) q_ptr->flags |= (0x1 << flag); /* player isn't on this quest. return global goal. */
		else q_ptr->flags &= ~(0x1 << flag);
		return;
	}

	if (q_ptr->individual) {
		if (set) p_ptr->quest_flags[i] |= (0x1 << flag);
		else p_ptr->quest_flags[i] &= ~(0x1 << flag); /* individual quest */
		return;
	}
	if (set) q_ptr->flags |= (0x1 << flag); /* global quest */
	else q_ptr->flags &= ~(0x1 << flag);
}
#endif

/* perform automatic things (quest spawn/stage change) in a stage */
static bool quest_stage_automatics(int pInd, int q_idx, int stage) {
	quest_info *q_ptr = &q_info[q_idx];

	/* auto-spawn (and acquire) new quest? */
	if (q_ptr->activate_quest[stage] != -1 && !q_info[q_ptr->activate_quest[stage]].disabled) {
		quest_activate(q_ptr->activate_quest[stage]);
#if QDEBUG > 0
			s_printf("%s QUEST_ACTIVATE_AUTO: '%s' ('%s' by '%s')\n",
			    showtime(), q_name + q_info[q_ptr->activate_quest[stage]].name,
			    q_info[q_ptr->activate_quest[stage]].codename, q_info[q_ptr->activate_quest[stage]].creator);
#endif
		if (q_ptr->auto_accept[stage])
			quest_acquire_confirmed(pInd, q_ptr->activate_quest[stage], q_ptr->auto_accept_quiet[stage]);
	}

	/* auto-change stage (timed)? */
	if (q_ptr->change_stage[stage] != -1) {
		/* not a timed change? instant then */
		if (//!q_ptr->timed_stage_ingame[stage] &&
		    !q_ptr->timed_stage_ingame_abs[stage] && !q_ptr->timed_stage_real[stage]) {
#if QDEBUG > 0
			s_printf("%s QUEST_STAGE_AUTO: '%s' %d->%d ('%s' by '%s')\n",
			    showtime(), q_name + q_ptr->name, quest_get_stage(pInd, q_idx),
			    q_ptr->change_stage[stage], q_ptr->codename, q_ptr->creator);
#endif
			quest_set_stage(pInd, q_idx, q_ptr->change_stage[stage], q_ptr->quiet_change_stage[stage]);
			/* don't imprint/play dialogue of this stage anymore, it's gone~ */
			return TRUE;
		}
		/* start the clock */
		/*cannot do this, cause quest scheduler is checking once per minute atm
		if (q_ptr->timed_stage_ingame[stage]) {
			q_ptr->timed_stage_countdown[stage] = q_ptr->timed_stage_ingame[stage];//todo: different resolution than real minutes
			q_ptr->timed_stage_countdown_stage[stage] = q_ptr->change_stage[stage];
			q_ptr->timed_stage_countdown_quiet[stage] = q_ptr->quiet_change_stage[stage];
		} else */
		if (q_ptr->timed_stage_ingame_abs[stage]) {
			q_ptr->timed_stage_countdown[stage] = -q_ptr->timed_stage_ingame_abs[stage];
			q_ptr->timed_stage_countdown_stage[stage] = q_ptr->change_stage[stage];
			q_ptr->timed_stage_countdown_quiet[stage] = q_ptr->quiet_change_stage[stage];
		} else if (q_ptr->timed_stage_real[stage]) {
			q_ptr->timed_stage_countdown[stage] = q_ptr->timed_stage_real[stage];
			q_ptr->timed_stage_countdown_stage[stage] = q_ptr->change_stage[stage];
			q_ptr->timed_stage_countdown_quiet[stage] = q_ptr->quiet_change_stage[stage];
		}
	}

	return FALSE;
}
/* Advance quest to a different stage (or start it out if stage is 0) */
void quest_set_stage(int pInd, int q_idx, int stage, bool quiet) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, j, k;
	bool anything;

#if QDEBUG > 0
	s_printf("%s QUEST_STAGE: '%s' %d->%d ('%s' by '%s')\n", showtime(), q_name + q_ptr->name, quest_get_stage(pInd, q_idx), stage, q_ptr->codename, q_ptr->creator);
#endif

	/* dynamic info */
	//int stage_prev = quest_get_stage(pInd, q_idx);

	/* new stage is active.
	   for 'individual' quests this is just a temporary value used by quest_imprint_stage().
	   It is still used for the other stage-checking routines in this function too though:
	    quest_goal_check_reward(), quest_terminate() and the 'anything' check. */
	q_ptr->stage = stage;

	/* For non-'individual' quests,
	   if a participating player is around the questor, entering a new stage..
	   - qutomatically invokes the quest dialogue with him again (if any)
	   - store information of the current stage in the p_ptr array,
	     eg the target location for easier lookup */
	if (!q_ptr->individual || !pInd) { //the !pInd part is paranoia
		for (i = 1; i <= NumPlayers; i++) {
			if (!inarea(&Players[i]->wpos, &q_ptr->current_wpos[0])) continue; //TODO: multiple current_wpos, one for each questor!! to work with correct questor_idx in quest_dialogue call below!
			for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
				if (Players[i]->quest_idx[j] == q_idx) break;
			if (j == MAX_CONCURRENT_QUESTS) continue;

			/* play automatic narration if any */
			if (!quiet) {
				/* pre-scan narration if any line at all exists and passes the flag check */
				anything = FALSE;
				for (k = 0; k < QI_TALK_LINES; k++) {
					if (q_ptr->narration[stage][k] &&
					    ((q_ptr->narrationflags[stage][k] & q_ptr->flags) == q_ptr->narrationflags[stage][k])) {
						anything = TRUE;
						break;
					}
				}
				/* there is narration to display? */
				if (anything) {
					msg_print(i, "\374 ");
					msg_format(i, "\374\377u<\377U%s\377u>", q_name + q_ptr->name);
					for (k = 0; k < QI_TALK_LINES; k++) {
						if (!q_ptr->narration[stage][k]) break;
						if ((q_ptr->narrationflags[stage][k] & q_ptr->flags) != q_ptr->narrationflags[stage][k]) continue;
						msg_format(i, "\374\377U%s", q_ptr->narration[stage][k]);
					}
					//msg_print(i, "\374 ");
				}
			}

			/* perform automatic actions (spawn new quest, (timed) further stage change) */
			if (quest_stage_automatics(pInd, q_idx, stage)) return;

			/* update player's quest tracking data */
			quest_imprint_stage(i, q_idx, j);
			/* play questors' stage dialogue */
			for (j = 0; j < q_ptr->questors; j++)
				if (!quiet) quest_dialogue(i, q_idx, j, FALSE, FALSE);
		}
	} else { /* individual quest */
		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (Players[pInd]->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) return; //paranoia, shouldn't happen

		 //TODO: check against multiple current_wpos, one for each questor!! to work with correct questor_idx in quest_dialogue call below!

		/* play automatic narration if any */
		if (!quiet) {
			/* pre-scan narration if any line at all exists and passes the flag check */
			anything = FALSE;
			for (k = 0; k < QI_TALK_LINES; k++) {
				if (q_ptr->narration[stage][k] &&
				    ((q_ptr->narrationflags[stage][k] & q_ptr->flags) == q_ptr->narrationflags[stage][k])) {
					anything = TRUE;
					break;
				}
			}
			/* there is narration to display? */
			if (anything) {
				msg_print(pInd, "\374 ");
				msg_format(pInd, "\374\377u<\377U%s\377u>", q_name + q_ptr->name);
				for (k = 0; k < QI_TALK_LINES; k++) {
					if (!q_ptr->narration[stage][k]) break;
					msg_format(pInd, "\374\377U%s", q_ptr->narration[stage][k]);
				}
				//msg_print(pInd, "\374 ");
			}
		}

		/* perform automatic actions (spawn new quest, (timed) further stage change) */
		if (quest_stage_automatics(pInd, q_idx, stage)) return;

		/* update players' quest tracking data */
		quest_imprint_stage(pInd, q_idx, j);
		/* play questors' stage dialogue */
		for (j = 0; j < q_ptr->questors; j++)
			if (!quiet) quest_dialogue(pInd, q_idx, j, FALSE, FALSE);
	}


	/* check for handing out rewards! (in case they're free) */
	quest_goal_check_reward(pInd, q_idx);


	/* quest termination? */
	if (q_ptr->ending_stage && q_ptr->ending_stage == stage) {
#if QDEBUG > 0
		s_printf("%s QUEST_STAGE_ENDING: '%s' %d ('%s' by '%s')\n", showtime(), q_name + q_ptr->name, stage, q_ptr->codename, q_ptr->creator);
#endif
		quest_terminate(pInd, q_idx);
	}


	/* auto-quest-termination? (actually redundant with ending_stage)
	   If a stage has no dialogue keywords, or stage goals, or timed/auto stage change
	   effects or questor-movement/tele/revert-from-hostile effects, THEN the quest will end. */
	   //TODO: implement all of that stuff :p
	anything = FALSE;

	/* optional goals play no role, obviously */
	for (i = 0; i < QI_GOALS; i++)
		if (q_ptr->kill[stage][i] || q_ptr->retrieve[stage][i] || q_ptr->deliver_pos[stage][i]) {
			anything = TRUE;
			break;
		}
	/* now check remaining dialogue options (keywords) */
	for (j = 0; j < q_ptr->questors; j++) {
		for (i = 0; i < QI_MAX_KEYWORDS; i++)
			if (q_ptr->keyword[j][stage][i] &&
			    /* and it's not just a keyword-reply without a stage change? */
			    q_ptr->keyword_stage[j][stage][i] != -1) {
				anything = TRUE;
				break;
			}
		if (anything) break;
	}
	/* check auto/timed stage changes */
	if (q_ptr->change_stage[stage] != -1) anything = TRUE;
	//if (q_ptr->timed_stage_ingame[stage]) anything = TRUE;
	if (q_ptr->timed_stage_ingame_abs[stage] != -1) anything = TRUE;
	if (q_ptr->timed_stage_real[stage]) anything = TRUE;

	/* really nothing left to do? */
	if (!anything) {
#if QDEBUG > 0
		s_printf("%s QUEST_STAGE_EMPTY: '%s' %d ('%s' by '%s')\n", showtime(), q_name + q_ptr->name, stage, q_ptr->codename, q_ptr->creator);
#endif
		quest_terminate(pInd, q_idx);
	}
}

/* Store some information of the current stage in the p_ptr array,
   eg the target location for easier lookup. In theory we could make it work
   without this function, but then we'd for example have to check on EVERY
   step the player makes if he's doing any quest that has a target area and
   is there etc... */
/* TODO: currently only supports 1 target/delivery location, ie the one for the very first GOAL of that quest stage!
   this sucks, it's too complicated^^ what should probably be done is:
   only remember a p_ptr->bool_target_pos[max_conc], and then check on every wpos change.
   if positive check, imprint this on a p_ptr->bool_within_target_wpos[max_conc], and
   set p_ptr->bool_target_xypos[max_conc] if required.
   Then on every subsequent move/kill/itempickup we can check in detail. --OK, made it this way */
static void quest_imprint_stage(int Ind, int q_idx, int py_q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, stage;

	/* for 'individual' quests: imprint the individual stage for a player.
	   the globally set q_ptr->stage is in this case just a temporary value,
	   set by quest_set_stage() for us, that won't be of any further consequence. */
	stage = q_ptr->stage;
	if (q_ptr->individual) p_ptr->quest_stage[py_q_idx] = stage;

	/* find out if we are pursuing any sort of target locations */
	p_ptr->quest_target_pos[py_q_idx] = FALSE;
	p_ptr->quest_targetopt_pos[py_q_idx] = FALSE;
	p_ptr->quest_deliver_pos[py_q_idx] = FALSE;
	p_ptr->quest_deliveropt_pos[py_q_idx] = FALSE;

	/* set goal-dependant (temporary) quest info */
	for (i = 0; i < QI_GOALS; i++) {
		/* set target/deliver location helper info */
		if (q_ptr->target_pos[stage][i]) p_ptr->quest_target_pos[py_q_idx] = TRUE;
		if (q_ptr->deliver_pos[stage][i]) p_ptr->quest_deliver_pos[py_q_idx] = TRUE;

		/* set kill/retrieve tracking counter if we have such goals in this stage */
		if (q_ptr->kill[stage][i]) p_ptr->quest_kill_number[py_q_idx][i] = q_ptr->kill_number[stage][i];
		if (q_ptr->retrieve[stage][i]) p_ptr->quest_retrieve_number[py_q_idx][i] = q_ptr->retrieve_number[stage][i];
	}
	for (i = 0; i < QI_OPTIONAL; i++) {
		/* set target/deliver location helper info */
		if (q_ptr->targetopt_pos[stage][i]) p_ptr->quest_targetopt_pos[py_q_idx] = TRUE;
		if (q_ptr->deliveropt_pos[stage][i]) p_ptr->quest_deliveropt_pos[py_q_idx] = TRUE;

		/* set kill/retrieve tracking counter if we have such goals in this stage */
		if (q_ptr->killopt[stage][i]) p_ptr->quest_killopt_number[py_q_idx][i] = q_ptr->killopt_number[stage][i];
		if (q_ptr->retrieveopt[stage][i]) p_ptr->quest_retrieveopt_number[py_q_idx][i] = q_ptr->retrieveopt_number[stage][i];
	}
}

void quest_acquire_confirmed(int Ind, int q_idx, bool quiet) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, j;

	/* re-check (this was done in quest_acquire() before,
	   but we cannot carry over this information. :/
	   Also, it might have changed meanwhile by some very odd means that we don't want to think about --
	   does the player have capacity to pick up one more quest? */
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == -1) break;
	if (i == MAX_CONCURRENT_QUESTS) { /* paranoia - this should not be possible to happen */
		//if (!quiet)
			msg_print(Ind, "\377RYou cannot pick up this quest - please tell an admin about this!");
		return;
	}

	p_ptr->quest_idx[i] = q_idx;
	strcpy(p_ptr->quest_codename[i], q_ptr->codename);
#if QDEBUG > 1
s_printf("%s QUEST_ACQUIRED: (%d,%d,%d;%d,%d) %s (%d) has quest %d '%s'.\n", showtime(), p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, p_ptr->px, p_ptr->py, p_ptr->name, Ind, q_idx, q_ptr->codename);
#endif

	/* for 'individual' quests, reset temporary quest data or it might get carried over from previous quest */
	p_ptr->quest_stage[i] = 0; /* note that individual quests can ONLY start in stage 0, obviously */
	p_ptr->quest_flags[i] = 0x0000;
	for (j = 0; j < QI_GOALS; j++) p_ptr->quest_goals[i][j] = FALSE;
	for (j = 0; j < QI_OPTIONAL; j++) p_ptr->quest_goalsopt[i][j] = FALSE;

	/* reset temporary quest helper info */
	p_ptr->quest_target_pos[i] = FALSE;
	p_ptr->quest_within_target_wpos[i] = FALSE;
	p_ptr->quest_target_xy[i] = FALSE;
	p_ptr->quest_targetopt_pos[i] = FALSE;
	p_ptr->quest_within_targetopt_wpos[i] = FALSE;
	p_ptr->quest_targetopt_xy[i] = FALSE;
	p_ptr->quest_deliver_pos[i] = FALSE;
	p_ptr->quest_within_deliver_wpos[i] = FALSE;
	p_ptr->quest_deliver_xy[i] = FALSE;
	p_ptr->quest_deliveropt_pos[i] = FALSE;
	p_ptr->quest_within_deliveropt_wpos[i] = FALSE;
	p_ptr->quest_deliveropt_xy[i] = FALSE;

	/* let him know about just acquiring the quest? */
	if (!quiet) {
		//msg_print(Ind, "\374 "); /* results in double empty line, looking bad */
		//msg_format(Ind, "\374\377U**\377u You have acquired the quest \"\377U%s\377u\". \377U**", q_name + q_ptr->name);
		switch (q_ptr->privilege) {
		case 0: msg_format(Ind, "\374\377uYou have acquired the quest \"\377U%s\377u\".", q_name + q_ptr->name);
			break;
		case 1: msg_format(Ind, "\374\377uYou have acquired the quest \"\377U%s\377u\". (\377yprivileged\377u)", q_name + q_ptr->name);
			break;
		case 2: msg_format(Ind, "\374\377uYou have acquired the quest \"\377U%s\377u\". (\377ohighly privileged\377u)", q_name + q_ptr->name);
			break;
		case 3: msg_format(Ind, "\374\377uYou have acquired the quest \"\377U%s\377u\". (\377radmins only\377u)", q_name + q_ptr->name);
		}
		//msg_print(Ind, "\374 "); /* keep one line spacer to echoing our entered keyword */
	}

	/* hack: for 'individual' quests we use q_ptr->stage as temporary var to store the player's personal stage,
	   which is then transferred to the player again in quest_imprint_stage(). Yeah.. */
	if (q_ptr->individual) q_ptr->stage = 0; /* 'individual' quest entry stage is always 0 */
	/* store information of the current stage in the p_ptr array,
	   eg the target location for easier lookup */
	quest_imprint_stage(Ind, q_idx, i);

	/* re-prompt for keyword input, if any */
	quest_dialogue(Ind, q_idx, p_ptr->interact_questor_idx, TRUE, FALSE);
}

/* Acquire a quest, WITHOUT CHECKING whether the quest actually allows this at this stage!
   Returns TRUE if we are eligible for acquiring it (not just if we actually did acquire it). */
static bool quest_acquire(int Ind, int q_idx, bool quiet) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, j;
	bool ok;

	/* quest is deactivated? -- paranoia here */
	if (!q_ptr->active) {
		msg_print(Ind, qi_msg_deactivated);
		return FALSE;
	}
	/* quest is on [individual] cooldown? */
	if (quest_get_cooldown(Ind, q_idx)) {
		msg_print(Ind, qi_msg_cooldown);
		return FALSE;
	}

	/* quests is only for admins or privileged accounts at the moment? (for testing purpose) */
	switch (q_ptr->privilege) {
	case 1: if (!p_ptr->privileged && !is_admin(p_ptr)) return FALSE;
		break;
	case 2: if (p_ptr->privileged != 2 && !is_admin(p_ptr)) return FALSE;
		break;
	case 3: if (!is_admin(p_ptr)) return FALSE;
	}

	/* matches prerequisite quests? (ie this is a 'follow-up' quest) */
	for (i = 0; i < QI_PREREQUISITES; i++) {
		if (!q_ptr->prerequisites[i][0]) continue;
		for (j = 0; j < MAX_Q_IDX; j++) {
			if (strcmp(q_info[j].codename, q_ptr->prerequisites[i])) continue;
			if (!p_ptr->quest_done[j]) {
				msg_print(Ind, qi_msg_prereq);
				return FALSE;
			}
		}
	}
	/* level / race / class / mode / body restrictions */
	if (q_ptr->minlev && q_ptr->minlev > p_ptr->lev) {
		msg_print(Ind, qi_msg_minlev);
		return FALSE;
	}
	if (q_ptr->maxlev && q_ptr->maxlev < p_ptr->max_plv) {
		msg_print(Ind, qi_msg_maxlev);
		return FALSE;
	}
	if (!(q_ptr->races & (0x1 << p_ptr->prace))) {
		msg_print(Ind, qi_msg_race);
		return FALSE;
	}
	if (!(q_ptr->classes & (0x1 << p_ptr->pclass))) {
		msg_print(Ind, qi_msg_class);
		return FALSE;
	}
	ok = FALSE;
	if (!q_ptr->must_be_fruitbat && !q_ptr->must_be_monster) ok = TRUE;
	if (q_ptr->must_be_fruitbat && p_ptr->fruit_bat <= q_ptr->must_be_fruitbat) ok = TRUE;
	if (q_ptr->must_be_monster && p_ptr->body_monster == q_ptr->must_be_monster
	    && q_ptr->must_be_fruitbat != 1) /* hack: disable must_be_monster if must be a TRUE fruit bat.. */
		ok = TRUE;
	if (!ok) {
		if (q_ptr->must_be_fruitbat == 1) msg_print(Ind, qi_msg_truebat);
		else if (q_ptr->must_be_fruitbat || q_ptr->must_be_monster == 37) msg_print(Ind, qi_msg_bat);
		else msg_print(Ind, qi_msg_form);
		return FALSE;
	}
	ok = FALSE;
	if (q_ptr->mode_norm && !(p_ptr->mode & (MODE_EVERLASTING | MODE_PVP))) ok = TRUE;
	if (q_ptr->mode_el && (p_ptr->mode & MODE_EVERLASTING)) ok = TRUE;
	if (q_ptr->mode_pvp && (p_ptr->mode & MODE_PVP)) ok = TRUE;
	if (!ok) {
		msg_print(Ind, qi_msg_mode);
		return FALSE;
	}

	/* has the player completed this quest already/too often? */
	if (p_ptr->quest_done[q_idx] > q_ptr->repeatable && q_ptr->repeatable != -1) {
		if (!quiet) msg_print(Ind, qi_msg_done);
		return FALSE;
	}

	/* does the player have capacity to pick up one more quest? */
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == -1) break;
	if (i == MAX_CONCURRENT_QUESTS) {
		if (!quiet) msg_print(Ind, qi_msg_max);
		return FALSE;
	}

	/* voila, player may acquire this quest! */
#if QDEBUG > 1
s_printf("%s QUEST_MAY_ACQUIRE: (%d,%d,%d;%d,%d) %s (%d) may acquire quest %d '%s'.\n", showtime(), p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, p_ptr->px, p_ptr->py, p_ptr->name, Ind, q_idx, q_ptr->codename);
#endif
	return TRUE;
}

/* A player interacts with the questor (bumps him if a creature :-p).
   This displays quest talk/narration, turns in goals, or may acquire it. */
#define ALWAYS_DISPLAY_QUEST_TEXT
void quest_interact(int Ind, int q_idx, int questor_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, stage = quest_get_stage(Ind, q_idx);
	bool may_acquire = FALSE;

	/* quest is deactivated? */
	if (!q_ptr->active) return;

	/* quests is only for admins or privileged accounts at the moment? (for testing purpose) */
	switch (q_ptr->privilege) {
	case 1: if (!p_ptr->privileged && !is_admin(p_ptr)) return;
		break;
	case 2: if (p_ptr->privileged != 2 && !is_admin(p_ptr)) return;
		break;
	case 3: if (!is_admin(p_ptr)) return;
	}

	/* cannot interact with the questor during this stage? */
	if (!q_ptr->questor_talkable[questor_idx]) return;


	/* questor interaction may (automatically) acquire the quest */
	/* has the player not yet acquired this quest? */
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	/* nope - test if he's eligible for picking it up!
	   Otherwise, the questor will remain silent for him. */
	if (i == MAX_CONCURRENT_QUESTS) {
		/* do we accept players by questor interaction at all? */
		if (!q_ptr->accept_interact[questor_idx]) return;
		/* do we accept players to acquire this quest in the current quest stage? */
		if (!q_ptr->accepts[stage]) return;

		/* if player cannot pick up this quest, questor remains silent */
		if (!(may_acquire = quest_acquire(Ind, q_idx, FALSE))) return;
	}


	/* if we're not here for quest acquirement, just check for quest goals
	   that have been completed and just required delivery back here */
	if (!may_acquire && quest_goal_check(Ind, q_idx, TRUE)) return;

	/* questor interaction qutomatically invokes the quest dialogue, if any */
	q_ptr->talk_focus[questor_idx] = Ind; /* only this player can actually respond with keywords -- TODO: ensure this happens for non 'individual' quests only */
	quest_dialogue(Ind, q_idx, questor_idx, FALSE, may_acquire);

	/* prompt him to acquire this quest if he hasn't yet */
	if (may_acquire) Send_request_cfr(Ind, RID_QUEST_ACQUIRE + q_idx, format("Accept the quest \"%s\"?", q_name + q_ptr->name), TRUE);
}

/* Talk vs keyword dialogue between questor and player.
   This is initiated either by bumping into the questor or by entering a new
   stage while in the area of the questor.
   Note that if the questor is focussed, only that player may respond with keywords.

   'repeat' will repeat requesting an input and skip the usual dialogue. Used for
   keyword input when a keyword wasn't recognized.

   'interact_acquire' must be set if this dialogue spawns from someone interacting
   initially with the questor who is eligible to acquire the quest.
   In that case, the player won't get the enter-a-keyword-prompt. Instead, our
   caller function quest_interact() will prompt him to acquire the quest first. */
static void quest_dialogue(int Ind, int q_idx, int questor_idx, bool repeat, bool interact_acquire) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, k, stage = quest_get_stage(Ind, q_idx);
	bool anything;

	if (!repeat) {
		/* pre-scan talk if any line at all passes the flag check */
		anything = FALSE;
		for (k = 0; k < QI_TALK_LINES; k++) {
			if (q_ptr->talk[questor_idx][stage][k] &&
			    ((q_ptr->talkflags[questor_idx][stage][k] & q_ptr->flags) == q_ptr->talkflags[questor_idx][stage][k])) {
				anything = TRUE;
				break;
			}
		}
		/* there is NPC talk to display? */
		if (anything) {
			p_ptr->interact_questor_idx = questor_idx;
			msg_print(Ind, "\374 ");
			msg_format(Ind, "\374\377u<\377B%s\377u> speaks to you:", q_ptr->questor_name[questor_idx]);
			for (i = 0; i < QI_TALK_LINES; i++) {
				if (!q_ptr->talk[questor_idx][stage][i]) break;
				if ((q_ptr->talkflags[questor_idx][stage][k] & q_ptr->flags) != q_ptr->talkflags[questor_idx][stage][k]) continue;
				msg_format(Ind, "\374\377U%s", q_ptr->talk[questor_idx][stage][i]);
			}
			//msg_print(Ind, "\374 ");
		}
	}

	/* No keyword-interaction possible if we haven't acquired the quest yet. */
	if (interact_acquire) return;

	/* If there are any keywords in this stage, prompt the player for a reply.
	   If the questor is focussed on one player, only he can give a reply,
	   otherwise the first player to reply can advance the stage. */
	if (!q_ptr->individual && q_ptr->talk_focus[questor_idx] && q_ptr->talk_focus[questor_idx] != Ind) return;

	p_ptr->interact_questor_idx = questor_idx;
	if (q_ptr->keyword[questor_idx][stage][0]) { /* at least 1 keyword available? */
		/* hack: if 1st keyword is empty string "", display a "more" prompt */
		if (q_ptr->keyword[questor_idx][stage][0][0] == 0)
			Send_request_str(Ind, RID_QUEST + q_idx, "Your reply (or ENTER for more)> ", "");
		else {
			/* hack: if 1st keyword is "y" just give a yes/no choice instead of an input prompt?
			   we assume that 2nd keyword is a "n" then. */
			if (q_ptr->keyword[questor_idx][stage][0][0] == 'y' &&
			    q_ptr->keyword[questor_idx][stage][0][1] == 0)
				Send_request_cfr(Ind, RID_QUEST + q_idx, "Your reply, yes or no?>", FALSE);
			else /* normal prompt for keyword input */
				Send_request_str(Ind, RID_QUEST + q_idx, "Your reply> ", "");
		}
	}
}

/* Player replied in a questor dialogue by entering a keyword. */
void quest_reply(int Ind, int q_idx, char *str) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, j, stage = quest_get_stage(Ind, q_idx), questor_idx = p_ptr->interact_questor_idx;
	char *c;

#if 0
	if (!str[0] || str[0] == '\e') return; /* player hit the ESC key.. */
#else /* distinguish RETURN key, to allow empty "" keyword (to 'continue' a really long talk for example) */
	if (str[0] == '\e') return; /* player hit the ESC key.. */
#endif

	/* convert input to all lower-case */
	c = str;
	while (*c) {
		*c = tolower(*c);
		c++;
	}

	/* echo own reply for convenience */
	msg_print(Ind, "\374 ");
	if (!strcmp(str, "y")) msg_print(Ind, "\374\377u>\377UYes");
	else if (!strcmp(str, "n")) msg_print(Ind, "\374\377u>\377UNo");
	else msg_format(Ind, "\374\377u>\377U%s", str);

	/* scan keywords for match */
	for (i = 0; i < QI_MAX_KEYWORDS; i++) {
		if (!q_ptr->keyword[questor_idx][stage][i]) break; /* no more keywords? */
		if (strcmp(q_ptr->keyword[questor_idx][stage][i], str)) continue; /* not matching? */
		/* check if required flags match to enable this keyword */
		if ((q_ptr->keywordflags[questor_idx][stage][i] & q_ptr->flags) != q_ptr->keywordflags[questor_idx][stage][i]) continue;

		/* reply? */
		if (q_ptr->keyword_reply[questor_idx][stage][i] &&
		    q_ptr->keyword_reply[questor_idx][stage][i][0]) {
			msg_print(Ind, "\374 ");
			msg_format(Ind, "\374\377u<\377B%s\377u> speaks to you:", q_ptr->questor_name[questor_idx]);
			for (j = 0; j < QI_TALK_LINES; j++) {
				if (!q_ptr->keyword_reply[questor_idx][stage][i][j]) break;
				msg_format(Ind, "\374\377U%s", q_ptr->keyword_reply[questor_idx][stage][i][j]);
			}
			//msg_print(Ind, "\374 ");
		}

		/* flags change? */
		q_ptr->flags |= (q_ptr->keyword_setflags[questor_idx][stage][i]);
		q_ptr->flags &= ~(q_ptr->keyword_clearflags[questor_idx][stage][i]);

		/* stage change? */
		if (q_ptr->keyword_stage[questor_idx][stage][i] != -1)
			quest_set_stage(Ind, q_idx, q_ptr->keyword_stage[questor_idx][stage][i], FALSE);

		return;
	}
	/* it was nice small-talking to you, dude */
#if 1
	/* if keyword wasn't recognised, repeat input prompt instead of just 'dropping' the convo */
	quest_dialogue(Ind, q_idx, questor_idx, TRUE, FALSE);
	/* don't give 'wassup?' style msg if we just hit RETURN.. silyl */
	if (str[0]) {
		msg_print(Ind, "\374 ");
		msg_format(Ind, "\374\377u<\377B%s\377u> has nothing to say about that.", q_ptr->questor_name[questor_idx]);
		//msg_print(Ind, "\374 ");
	}
#endif
	return;
}

/* Test kill quest goal criteria vs an actually killed monster, for a match.
   Main criteria (r_idx vs char+attr+level) are OR'ed.
   (Unlike for retrieve-object matches where they are actually AND'ed.) */
static bool quest_goal_matches_kill(int q_idx, int stage, int goal, monster_type *m_ptr) {
	quest_info *q_ptr = &q_info[q_idx];
	int i;
	monster_race *r_ptr = race_inf(m_ptr);

	/* check for race index */
	for (i = 0; i < 10; i++) {
		/* no monster specified? */
		if (q_ptr->kill_ridx[stage][goal][i] == 0) continue;
		/* accept any monster? */
		if (q_ptr->kill_ridx[stage][goal][i] == -1) return TRUE;
		/* specified an r_idx */
		if (q_ptr->kill_ridx[stage][goal][i] == m_ptr->r_idx) return TRUE;
	}

	/* check for char/attr/level combination - treated in pairs (AND'ed) over the index */
	for (i = 0; i < 5; i++) {
		/* no char specified? */
		if (q_ptr->kill_rchar[stage][goal][i] == 255) continue;
		 /* accept any char? */
		if (q_ptr->kill_rchar[stage][goal][i] != 254 &&
		    /* specified a char? */
		    q_ptr->kill_rchar[stage][goal][i] != r_ptr->d_char) continue;

		/* no attr specified? */
		if (q_ptr->kill_rattr[stage][goal][i] == 255) continue;
		 /* accept any attr? */
		if (q_ptr->kill_rattr[stage][goal][i] != 254 &&
		    /* specified an attr? */
		    q_ptr->kill_rattr[stage][goal][i] != r_ptr->d_attr) continue;

		/* min/max level check -- note that we use m-level, not r-level :-o */
		if ((!q_ptr->kill_rlevmin[stage][goal] || q_ptr->kill_rlevmin[stage][goal] <= m_ptr->level) &&
		    (!q_ptr->kill_rlevmax[stage][goal] || q_ptr->kill_rlevmax[stage][goal] >= m_ptr->level))
			return TRUE;
	}

	/* pft */
	return FALSE;
}

/* Test retrieve-item quest goal criteria vs an actually retrieved object, for a match.
   (Note that the for-blocks are AND'ed unlike for kill-matches where they are OR'ed.) */
static bool quest_goal_matches_object(int q_idx, int stage, int goal, object_type *o_ptr) {
	quest_info *q_ptr = &q_info[q_idx];
	int i;
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	byte attr;

	/* first let's find out the object's attr..which is uggh not so cool (from cave.c) */
	attr = k_ptr->k_attr;
	if (o_ptr->tval == TV_BOOK && is_custom_tome(o_ptr->sval))
		attr = get_book_name_color(0, o_ptr);
	/* hack: colour of fancy shirts or custom objects can vary  */
	if ((o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_SHIRT) ||
	    (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT)) {
		if (!o_ptr->xtra1) o_ptr->xtra1 = attr; //wut.. remove this hack? should be superfluous anyway
			attr = o_ptr->xtra1;
	}
	if ((k_info[o_ptr->k_idx].flags5 & TR5_ATTR_MULTI))
	    //#ifdef CLIENT_SHIMMER whatever..
		attr = TERM_HALF;

	/* check for tval/sval */
	for (i = 0; i < 10; i++) {
		/* no tval specified? */
		if (q_ptr->retrieve_otval[stage][goal][i] == 0) continue;
		/* accept any tval? */
		if (q_ptr->retrieve_otval[stage][goal][i] != -1 &&
		    /* specified a tval */
		    q_ptr->retrieve_otval[stage][goal][i] != o_ptr->tval) continue;;

		/* no sval specified? */
		if (q_ptr->retrieve_osval[stage][goal][i] == 0) continue;
		/* accept any sval? */
		if (q_ptr->retrieve_osval[stage][goal][i] != -1 &&
		    /* specified a sval */
		    q_ptr->retrieve_osval[stage][goal][i] != o_ptr->sval) continue;;

		break;
	}
	if (i == 10) return FALSE;

	/* check for pval/bpval/attr/name1/name2/name2b
	   note: let's treat pval/bpval as minimum values instead of exact values for now. */
	for (i = 0; i < 5; i++) {
		/* no pval specified? */
		if (q_ptr->retrieve_opval[stage][goal][i] == 9999) continue;
		/* accept any pval? */
		if (q_ptr->retrieve_opval[stage][goal][i] != -9999 &&
		    /* specified a pval? */
		    q_ptr->retrieve_opval[stage][goal][i] < o_ptr->pval) continue;

		/* no bpval specified? */
		if (q_ptr->retrieve_obpval[stage][goal][i] == 9999) continue;
		/* accept any bpval? */
		if (q_ptr->retrieve_obpval[stage][goal][i] != -9999 &&
		    /* specified a bpval? */
		    q_ptr->retrieve_obpval[stage][goal][i] < o_ptr->bpval) continue;

		/* no attr specified? */
		if (q_ptr->retrieve_oattr[stage][goal][i] == 255) continue;
		/* accept any attr? */
		if (q_ptr->retrieve_oattr[stage][goal][i] != -254 &&
		    /* specified a attr? */
		    q_ptr->retrieve_oattr[stage][goal][i] != attr) continue;

		/* no name1 specified? */
		if (q_ptr->retrieve_oname1[stage][goal][i] == -3) continue;
		 /* accept any name1? */
		if (q_ptr->retrieve_oname1[stage][goal][i] != -1 &&
		 /* accept any name1, but MUST be != 0? */
		    (q_ptr->retrieve_oname1[stage][goal][i] != -2 || !o_ptr->name1) &&
		    /* specified a name1? */
		    q_ptr->retrieve_oname1[stage][goal][i] != o_ptr->name1) continue;

		/* no name2 specified? */
		if (q_ptr->retrieve_oname2[stage][goal][i] == -3) continue;
		 /* accept any name2? */
		if (q_ptr->retrieve_oname2[stage][goal][i] != -1 &&
		 /* accept any name2, but MUST be != 0? */
		    (q_ptr->retrieve_oname2[stage][goal][i] != -2 || !o_ptr->name2) &&
		    /* specified a name2? */
		    q_ptr->retrieve_oname2[stage][goal][i] != o_ptr->name2) continue;

		/* no name2b specified? */
		if (q_ptr->retrieve_oname2b[stage][goal][i] == -3) continue;
		 /* accept any name2b? */
		if (q_ptr->retrieve_oname2b[stage][goal][i] != -1 &&
		 /* accept any name2b, but MUST be != 0? */
		    (q_ptr->retrieve_oname2b[stage][goal][i] != -2 || !o_ptr->name2b) &&
		    /* specified a name2b? */
		    q_ptr->retrieve_oname2b[stage][goal][i] != o_ptr->name2b) continue;
	}
	if (i == 5) return FALSE;

	/* finally, test against minimum value */
	if (q_ptr->retrieve_ovalue[stage][goal] <= object_value_real(0, o_ptr)) return TRUE;

	/* pft */
	return FALSE;
}

/* Check if player completed a kill or item-retrieve goal.
   The monster slain or item retrieved must therefore be given to this function for examination.
   NOTE/TODO: These checks here actually make p_ptr->quest_target_wpos.. helper information obsolete..
   Note: The mechanics for retrieving quest items at a specific target position are a bit trick:
         If n items have to be retrieved, each one has to be a different item that gets picked at
         the target pos. When an item is lost from the player's inventory again however, it mayb be
         retrieved anywhere, ignoring the target location specification. This requires the quest
         items to be marked when they get picked up at the target location, to free those marked
         ones from same target loc restrictions for re-pickup. */
void quest_check_goal_kr(int Ind, monster_type *m_ptr, object_type *o_ptr) {
	quest_info *q_ptr;
	player_type *p_ptr = Players[Ind];
	int i, j, k, q_idx, stage;
	bool nisi;

	/* paranoia -- neither a kill has been made nor an item picked up */
	if (!m_ptr && !o_ptr) return;

	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++) {
		/* player actually pursuing a quest? */
		if (p_ptr->quest_idx[i] == -1) continue;

		if (!p_ptr->quest_target_pos[i]) continue;//redundant with quest_within_target_pos?
		if (!p_ptr->quest_within_target_wpos[i]) continue;

		q_idx = p_ptr->quest_idx[i];
		q_ptr = &q_info[q_idx];

		/* quest is deactivated? */
		if (!q_ptr->active) continue;

		stage = quest_get_stage(Ind, q_idx);

		/* For handling Z-lines: flags changed depending on goals completed:
		   pre-check if we have any pending deliver goal in this stage.
		   If so then we can only set the quest goal 'nisi' (provisorily),
		   and hence flags won't get changed yet until it is eventually resolved
		   when we turn in the delivery. */
		nisi = FALSE
		for (j = 0; j < QI_GOALS; j++)
			if (q_ptr->deliver_pos[stage][j]) {
				nisi = TRUE;
				break;
			}

		/* check the quest goals, whether any of them wants a target to this location */
		for (j = 0; j < QI_GOALS; j++) {
			/* no k/r goal? */
			if (!q_ptr->kill[stage][j] && !q_ptr->retrieve[stage][j]) continue;

			/* location-restricted?
			   Exempt already retrieved items that were just lost temporarily on the way! */
			if (q_ptr->target_pos[stage][j] &&
			    !(o_ptr->quest == q_idx + 1 && o_ptr->quest_stage == stage)) {
				/* extend target terrain over a wide patch? */
				if (q_ptr->target_terrain_patch[stage][j]) {
					/* different z-coordinate = instant fail */
					if (p_ptr->wpos.wz != q_ptr->target_wpos[stage][j].wz) continue;
					/* are we within range and have same terrain type? */
					if (distance(p_ptr->wpos.wx, p_ptr->wpos.wy, q_ptr->target_wpos[stage][j].wx,
					    q_ptr->target_wpos[stage][j].wy) > QI_TERRAIN_PATCH_RADIUS ||
					    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type !=
					    wild_info[q_ptr->target_wpos[stage][j].wy][q_ptr->target_wpos[stage][j].wx].type)
						continue;
				}
				/* just check a single, specific wpos? */
				else if (!inarea(&q_ptr->target_wpos[stage][j], &p_ptr->wpos)) continue;

				/* check for exact x,y location? */
				if (q_ptr->target_pos_x[stage][j] != -1 &&
				    q_ptr->target_pos_x[stage][j] != p_ptr->px &&
				    q_ptr->target_pos_y[stage][j] != p_ptr->py)
					continue;
			}

			/* check for kill goal here */
			if (m_ptr && q_ptr->kill[stage][j]) {
				if (!quest_goal_matches_kill(q_idx, stage, j, m_ptr)) continue;
				/* decrease the player's kill counter, if we got all, goal is completed! */
				p_ptr->quest_kill_number[i][j]--;
				if (p_ptr->quest_kill_number[i][j]) continue; /* not yet */

				/* we have completed a target-to-xy goal! */
				quest_set_goal(Ind, q_idx, j, nisi);
				continue;
			}

			/* check for retrieve-item goal here */
			if (o_ptr && q_ptr->retrieve[stage][j]) {
				if (!quest_goal_matches_object(q_idx, stage, j, o_ptr)) continue;

				/* discard old items from another quest or quest stage that just look similar!
				   Those are 'tainted' and cannot be reused. */
				if (o_ptr->quest != q_idx + 1 || o_ptr->quest_stage != stage) continue;

				/* mark the item as quest item, so we know we found it at the designated target loc (if any) */
				o_ptr->quest = q_idx + 1;
				o_ptr->quest_stage = stage;

				/* decrease the player's retrieve counter, if we got all, goal is completed! */
				p_ptr->quest_retrieve_number[i][j] -= o_ptr->number;
				if (p_ptr->quest_retrieve_number[i][j] > 0) continue; /* not yet */

				/* we have completed a target-to-xy goal! */
				quest_set_goal(Ind, q_idx, j, nisi);

				/* if we don't have to deliver in this stage,
				   we can just remove all the quest items right away now,
				   since they've fulfilled their purpose of setting the quest goal. */
				for (k = 0; k < QI_GOALS; k++)
					if (q_ptr->deliver_pos[stage][k]) break;
				if (k == QI_GOALS) {
					if (q_ptr->individual) {
						for (k = 0; k < INVEN_PACK; k++) {
							if (p_ptr->inventory[k].quest == q_idx + 1 &&
							    p_ptr->inventory[k].quest_stage == stage) {
								inven_item_increase(Ind, k, -99);
								//inven_item_describe(Ind, k);
								inven_item_optimize(Ind, k);
							}
						}
					} else {
						//TODO (not just here): implement global retrieval quests..
					}
				}

				continue;
			}
		}
	}
}
/* Check if we have to un-set an item-retrieval quest goal because we lost <num> items!
   Note: If we restricted quest stages to request no overlapping item types, we could also add
         'quest_goal' to object_type so we won't need lookups in here, but currently there is no
         such restriction and I don't think we need it. */
void quest_check_ungoal_r(int Ind, object_type *o_ptr, int num) {
	quest_info *q_ptr;
	player_type *p_ptr = Players[Ind];
	int i, j, q_idx, stage;

	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++) {
		/* player actually pursuing a quest? */
		if (p_ptr->quest_idx[i] == -1) continue;

		if (!p_ptr->quest_target_pos[i]) continue;//redundant with quest_within_target_pos?
		if (!p_ptr->quest_within_target_wpos[i]) continue;

		q_idx = p_ptr->quest_idx[i];
		q_ptr = &q_info[q_idx];

		/* quest is deactivated? */
		if (!q_ptr->active) continue;

		stage = quest_get_stage(Ind, q_idx);

		/* check the quest goals, whether any of them wants a target to this location */
		for (j = 0; j < QI_GOALS; j++) {
			/* no r goal? */
			if (!q_ptr->retrieve[stage][j]) continue;

			/* phew, item has nothing to do with this quest goal? */
			if (!quest_goal_matches_object(q_idx, stage, j, o_ptr)) continue;

			/* increase the player's retrieve counter again */
			p_ptr->quest_retrieve_number[i][j] += num;
			if (p_ptr->quest_retrieve_number[i][j] <= 0) continue; /* still cool */

			/* we have un-completed a target-to-xy goal, ow */
			quest_unset_goal(Ind, q_idx, j);
			continue;
		}
	}
}

/* Check if player completed a deliver goal to a wpos.
   Actually when we're called then we already know that we're at a fitting wpos
   for at least one quest. :-p Just have to check for all concurrent quests to
   make sure we set all their goals too. */
static void quest_check_goal_deliver_wpos(int Ind) {
	quest_info *q_ptr;
	player_type *p_ptr = Players[Ind];
	int i, j, k, q_idx, stage;

	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++) {
		/* player actually pursuing a quest? */
		if (p_ptr->quest_idx[i] == -1) continue;

		if (!p_ptr->quest_deliver_pos[i]) continue;//redundant with quest_within_deliver_pos?
		if (!p_ptr->quest_within_deliver_wpos[i]) continue;

		q_idx = p_ptr->quest_idx[i];
		q_ptr = &q_info[q_idx];

		/* quest is deactivated? */
		if (!q_ptr->active) continue;

		stage = quest_get_stage(Ind, q_idx);

		/* pre-check if we have completed all kill/retrieve goals of this stage,
		   because we can only complete any deliver goal if we have fetched all
		   the stuff (bodies, or kill count rather, and objects) that we ought to
		   'deliver', duh */
		for (j = 0; j < QI_GOALS; j++)
			if ((q_ptr->kill[stage][j] || q_ptr->retrieve[stage][j])
			    && !q_ptr->goals[stage][j])
				break;
		if (j != QI_GOALS) continue;

		/* check the quest goals, whether any of them wants a delivery to this location */
		for (j = 0; j < QI_GOALS; j++) {
			if (!q_ptr->deliver_pos[stage][j]) continue;

			/* handle only non-specific x,y goals here */
			if (q_ptr->deliver_pos_x[stage][j] != -1) continue;

			/* extend target terrain over a wide patch? */
			if (q_ptr->deliver_terrain_patch[stage][j]) {
				/* different z-coordinate = instant fail */
				if (p_ptr->wpos.wz != q_ptr->deliver_wpos[stage][j].wz) continue;
				/* are we within range and have same terrain type? */
				if (distance(p_ptr->wpos.wx, p_ptr->wpos.wy, q_ptr->deliver_wpos[stage][j].wx,
				    q_ptr->deliver_wpos[stage][j].wy) > QI_TERRAIN_PATCH_RADIUS ||
				    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type !=
				    wild_info[q_ptr->deliver_wpos[stage][j].wy][q_ptr->deliver_wpos[stage][j].wx].type)
					continue;
			}
			/* just check a single, specific wpos? */
			else if (!inarea(&q_ptr->deliver_wpos[stage][j], &p_ptr->wpos)) continue;

			/* for item retrieval goals therefore linked to this deliver goal,
			   remove all quest items now finally that we 'delivered' them.. */
			if (q_ptr->individual) {
				for (k = 0; k < INVEN_PACK; k++) {
					if (p_ptr->inventory[k].quest == q_idx + 1 &&
					    p_ptr->inventory[k].quest_stage == stage) {
						inven_item_increase(Ind, k, -99);
						//inven_item_describe(Ind, k);
						inven_item_optimize(Ind, k);
					}
				}
			} else {
				//TODO (not just here): implement global retrieval quests..
			}
			/* ..and also mark all 'nisi' quest goals as finally resolved,
			   to change flags accordingly if defined by a Z-line. */
			for (k = 0; k < QI_GOALS; k++)
				if (q_ptr->goals_nisi[stage][k]) {
					q_ptr->goals_nisi[stage][k] = FALSE;
					quest_goal_changes_flags(q_idx, stage, k);
				}

			/* we have completed a delivery-to-wpos goal! */
			quest_set_goal(Ind, q_idx, j, FALSE);
		}
	}
}
/* Check if player completed a deliver goal to a wpos and specific x,y */
void quest_check_goal_deliver_xy(int Ind) {
	quest_info *q_ptr;
	player_type *p_ptr = Players[Ind];
	int i, j, k, q_idx, stage;

	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++) {
		/* player actually pursuing a quest? */
		if (p_ptr->quest_idx[i] == -1) continue;

		if (!p_ptr->quest_deliver_pos[i]) continue;//redundant with quest_within_deliver_pos?
		if (!p_ptr->quest_within_deliver_wpos[i]) continue;

		q_idx = p_ptr->quest_idx[i];
		q_ptr = &q_info[q_idx];

		/* quest is deactivated? */
		if (!q_ptr->active) continue;

		stage = quest_get_stage(Ind, q_idx);

		/* pre-check if we have completed all kill/retrieve goals of this stage,
		   because we can only complete any deliver goal if we have fetched all
		   the stuff (bodies, or kill count rather, and objects) that we ought to
		   'deliver', duh */
		for (j = 0; j < QI_GOALS; j++)
			if ((q_ptr->kill[stage][j] || q_ptr->retrieve[stage][j])
			    && !q_ptr->goals[stage][j])
				break;
		if (j != QI_GOALS) continue;

		/* check the quest goals, whether any of them wants a delivery to this location */
		for (j = 0; j < QI_GOALS; j++) {
			if (!q_ptr->deliver_pos[stage][j]) continue;

			/* handle only specific x,y goals here */
			if (q_ptr->deliver_pos_x[stage][j] == -1) continue;

			/* extend target terrain over a wide patch? */
			if (q_ptr->deliver_terrain_patch[stage][j]) {
				/* different z-coordinate = instant fail */
				if (p_ptr->wpos.wz != q_ptr->deliver_wpos[stage][j].wz) continue;
				/* are we within range and have same terrain type? */
				if (distance(p_ptr->wpos.wx, p_ptr->wpos.wy, q_ptr->deliver_wpos[stage][j].wx,
				    q_ptr->deliver_wpos[stage][j].wy) > QI_TERRAIN_PATCH_RADIUS ||
				    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type !=
				    wild_info[q_ptr->deliver_wpos[stage][j].wy][q_ptr->deliver_wpos[stage][j].wx].type)
					continue;
			}
			/* just check a single, specific wpos? */
			else if (!inarea(&q_ptr->deliver_wpos[stage][j], &p_ptr->wpos)) continue;

			/* check for exact x,y location */
			if (q_ptr->deliver_pos_x[stage][j] != p_ptr->px ||
			    q_ptr->deliver_pos_y[stage][j] != p_ptr->py)
				continue;

			/* for item retrieval goals therefore linked to this deliver goal,
			   remove all quest items now finally that we 'delivered' them.. */
			if (q_ptr->individual) {
				for (k = 0; k < INVEN_PACK; k++) {
					if (p_ptr->inventory[k].quest == q_idx + 1 &&
					    p_ptr->inventory[k].quest_stage == stage) {
						inven_item_increase(Ind, k, -99);
						//inven_item_describe(Ind, k);
						inven_item_optimize(Ind, k);
					}
				}
			} else {
				//TODO (not just here): implement global retrieval quests..
			}
			/* ..and also mark all 'nisi' quest goals as finally resolved,
			   to change flags accordingly if defined by a Z-line. */
			for (k = 0; k < QI_GOALS; k++)
				if (q_ptr->goals_nisi[stage][k]) {
					q_ptr->goals_nisi[stage][k] = FALSE;
					quest_goal_changes_flags(q_idx, stage, k);
				}

			/* we have completed a delivery-to-xy goal! */
			quest_set_goal(Ind, q_idx, j, FALSE);
		}
	}
}

/* check goals for their current completion state, for entering the next stage if ok */
static int quest_goal_check_stage(int pInd, int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, j, stage = quest_get_stage(pInd, q_idx);

	/* scan through all possible follow-up stages */
	for (j = 0; j < QI_FOLLOWUP_STAGES; j++) {
		/* no follow-up stage? */
		if (q_ptr->next_stage_from_goals[stage][j] == -1) continue;

		/* scan through all goals required to be fulfilled to enter this stage */
		for (i = 0; i < QI_STAGE_GOALS; i++) {
			if (q_ptr->goals_for_stage[stage][j][i] == -1) continue;

			/* if even one goal is missing, we cannot advance */
			if (!quest_get_goal(pInd, q_idx, q_ptr->goals_for_stage[stage][j][i])) break;
		}
		/* we may proceed to another stage? */
		if (i == QI_STAGE_GOALS) return q_ptr->next_stage_from_goals[stage][j];
	}
	return -1; /* goals are not complete yet */
}

/* hand out a reward object to player (if individual quest)
   or to all involved players in the area (if non-individual quest) */
static void quest_reward_object(int pInd, int q_idx, object_type *o_ptr) {
	quest_info *q_ptr = &q_info[q_idx];
	//player_type *p_ptr;
	int i, j;

	if (pInd && q_ptr->individual) { //we should never get an individual quest without a pInd here..
		/*p_ptr = Players[pInd];
		drop_near(o_ptr, -1, &p_ptr->wpos, &p_ptr->py, &p_ptr->px);*/
		//msg_print(pInd, "You have received an item."); --give just ONE message for ALL items, less spammy
		inven_carry(pInd, o_ptr);
		return;
	}

	if (!q_ptr->individual) {
		s_printf("QUEST_REWARD_OBJECT: not individual, but no pInd either.\n");
		return;//catch paranoid bugs--maybe obsolete
	}

	/* global quest (or for some reason missing pInd..<-paranoia)  */
	for (i = 1; i <= NumPlayers; i++) {
		if (!inarea(&Players[i]->wpos, &q_ptr->current_wpos[0])) continue; //TODO: multiple current_wpos, one for each questor!!
		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

		/* hand him out the reward too */
		/*p_ptr = Players[i];
		drop_near(o_ptr, -1, &p_ptr->wpos, &p_ptr->py, &p_ptr->px);*/
		//msg_print(i, "You have received an item."); --give just ONE message for ALL items, less spammy
		inven_carry(i, o_ptr);
	}
}

/* hand out a reward object created by create_reward() to player (if individual quest)
   or to all involved players in the area (if non-individual quest) */
static void quest_reward_create(int pInd, int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	u32b resf = RESF_NOTRUEART;
	int i, j;

	if (pInd && q_ptr->individual) { //we should never get an individual quest without a pInd here..
		//msg_print(pInd, "You have received an item."); --give just ONE message for ALL items, less spammy
		give_reward(pInd, resf, q_name + q_ptr->name, 0, 0);
		return;
	}

	if (!q_ptr->individual) {
		s_printf("QUEST_REWARD_CREATE: not individual, but no pInd either.\n");
		return;//catch paranoid bugs--maybe obsolete
	}

	/* global quest (or for some reason missing pInd..<-paranoia)  */
	for (i = 1; i <= NumPlayers; i++) {
		if (!inarea(&Players[i]->wpos, &q_ptr->current_wpos[0])) continue; //TODO: multiple current_wpos, one for each questor!!
		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

		/* hand him out the reward too */
		//msg_print(i, "You have received an item."); --give just ONE message for ALL items, less spammy
		give_reward(i, resf, q_name + q_ptr->name, 0, 0);
	}
}

/* hand out gold to player (if individual quest)
   or to all involved players in the area (if non-individual quest) */
static void quest_reward_gold(int pInd, int q_idx, int au) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, j;

	if (pInd && q_ptr->individual) { //we should never get an individual quest without a pInd here..
		//msg_format(pInd, "You have received %d gold pieces.", au); --give just ONE message for ALL gold, less spammy
		gain_au(pInd, au, FALSE, FALSE);
		return;
	}

	if (!q_ptr->individual) {
		s_printf("QUEST_REWARD_GOLD: not individual, but no pInd either.\n");
		return;//catch paranoid bugs--maybe obsolete
	}

	/* global quest (or for some reason missing pInd..<-paranoia)  */
	for (i = 1; i <= NumPlayers; i++) {
		if (!inarea(&Players[i]->wpos, &q_ptr->current_wpos[0])) continue; //TODO: multiple current_wpos, one for each questor!!
		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

		/* hand him out the reward too */
		//msg_format(i, "You have received %d gold pieces.", au); --give just ONE message for ALL gold, less spammy
		gain_au(i, au, FALSE, FALSE);
	}
}

/* provide experience to player (if individual quest)
   or to all involved players in the area (if non-individual quest) */
static void quest_reward_exp(int pInd, int q_idx, int exp) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, j;

	if (pInd && q_ptr->individual) { //we should never get an individual quest without a pInd here..
		//msg_format(pInd, "You have received %d experience points.", exp); --give just ONE message for ALL gold, less spammy
		gain_exp(pInd, exp);
		return;
	}

	if (!q_ptr->individual) {
		s_printf("QUEST_REWARD_EXP: not individual, but no pInd either.\n");
		return;//catch paranoid bugs--maybe obsolete
	}

	/* global quest (or for some reason missing pInd..<-paranoia)  */
	for (i = 1; i <= NumPlayers; i++) {
		if (!inarea(&Players[i]->wpos, &q_ptr->current_wpos[0])) continue; //TODO: multiple current_wpos, one for each questor!!
		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

		/* hand him out the reward too */
		//msg_format(i, "You have received %d experience points.", exp); --give just ONE message for ALL gold, less spammy
		gain_exp(i, exp);
	}
}

/* check current stage of quest stage goals for handing out rewards:
   1) we've entered a quest stage and it's pretty much 'empty' so we might terminate
      if nothing more is up. check for free rewards first and hand them out.
   2) goals were completed. Before advancing the stage, hand out the proper rewards. */
//TODO: test for proper implementation for non-'individual' quests
static void quest_goal_check_reward(int pInd, int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, j, stage = quest_get_stage(pInd, q_idx);
	object_type forge, *o_ptr;
	u32b resf = RESF_NOTRUEART;
	/* count rewards */
	int r_obj = 0, r_gold = 0, r_exp = 0;

#if 0 /* we're called when stage 0 starts too, and maybe it's some sort of globally determined goal->reward? */
	if (!pInd) {
		s_printf("QUEST_GOAL_CHECK_REWARD: returned! oops\n");
		return; //paranoia?
	}
#endif

	/* scan through all possible follow-up stages */
	for (j = 0; j < QI_MAX_STAGE_REWARDS; j++) {
		/* no reward? */
		if (!q_ptr->reward_otval[stage][j] &&
		    !q_ptr->reward_oreward[stage][j] &&
		    !q_ptr->reward_gold[stage][j] &&
		    !q_ptr->reward_exp[stage][j]) //TODO: reward_statuseffect
			continue;

		/* scan through all goals required to be fulfilled to get this reward */
		for (i = 0; i < QI_REWARD_GOALS; i++) {
			if (q_ptr->goals_for_reward[stage][j][i] == -1) continue;

			/* if even one goal is missing, we cannot get the reward */
			if (!quest_get_goal(pInd, q_idx, q_ptr->goals_for_reward[stage][j][i])) break;
		}
		/* we may get rewards? */
		if (i == QI_REWARD_GOALS) {
			/* create and hand over this reward! */
#if QDEBUG > 0
			s_printf("%s QUEST_GOAL_CHECK_REWARD: Rewarded in '%s' %d ('%s' by '%s')\n", showtime(), q_name + q_ptr->name, stage, q_ptr->codename, q_ptr->creator);
#endif

			/* specific reward */
			if (q_ptr->reward_otval[stage][j]) {
				/* very specific reward */
				if (!q_ptr->reward_ogood && !q_ptr->reward_ogreat) {
					o_ptr = &forge;
					object_wipe(o_ptr);
					invcopy(o_ptr, lookup_kind(q_ptr->reward_otval[stage][j], q_ptr->reward_osval[stage][j]));
					o_ptr->number = 1;
					o_ptr->name1 = q_ptr->reward_oname1[stage][j];
					o_ptr->name2 = q_ptr->reward_oname2[stage][j];
					o_ptr->name2b = q_ptr->reward_oname2b[stage][j];
					if (o_ptr->name1) {
						o_ptr->name1 = ART_RANDART; //hack: disallow true arts!
						o_ptr->name2 = o_ptr->name2b = 0;
					}
					apply_magic(&q_ptr->current_wpos[0], o_ptr, -2, FALSE, FALSE, FALSE, FALSE, resf);
					o_ptr->pval = q_ptr->reward_opval[stage][j];
					o_ptr->bpval = q_ptr->reward_obpval[stage][j];
					o_ptr->note = quark_add(format("%s", q_name + q_ptr->name));
					o_ptr->note_utag = 0;
#ifdef PRE_OWN_DROP_CHOSEN
					o_ptr->level = 0;
					o_ptr->owner = p_ptr->id;
					o_ptr->mode = p_ptr->mode;
					if (o_ptr->name1) determine_artifact_timeout(o_ptr->name1);
#endif
				} else {
					o_ptr = &forge;
					object_wipe(o_ptr);
					invcopy(o_ptr, lookup_kind(q_ptr->reward_otval[stage][j], q_ptr->reward_osval[stage][j]));
					o_ptr->number = 1;
					apply_magic(&q_ptr->current_wpos[0], o_ptr, -2, q_ptr->reward_ogood[stage][j], q_ptr->reward_ogreat[stage][j], q_ptr->reward_ovgreat[stage][j], FALSE, resf);
					o_ptr->note = quark_add(format("%s", q_name + q_ptr->name));
					o_ptr->note_utag = 0;
#ifdef PRE_OWN_DROP_CHOSEN
					o_ptr->level = 0;
					o_ptr->owner = p_ptr->id;
					o_ptr->mode = p_ptr->mode;
					if (o_ptr->name1) determine_artifact_timeout(o_ptr->name1);
#endif
				}

				/* hand it out */
				quest_reward_object(pInd, q_idx, o_ptr);
				r_obj++;
			}
			/* instead use create_reward() like for events? */
			else if (q_ptr->reward_oreward[stage][j]) {
				quest_reward_create(pInd, q_idx);
				r_obj++;
			}
			/* hand out gold? */
			if (q_ptr->reward_gold[stage][j]) {
				quest_reward_gold(pInd, q_idx, q_ptr->reward_gold[stage][j]);
				r_gold += q_ptr->reward_gold[stage][j];
			}
			/* provide exp? */
			if (q_ptr->reward_exp[stage][j]) {
				quest_reward_exp(pInd, q_idx, q_ptr->reward_exp[stage][j]);
				r_exp += q_ptr->reward_exp[stage][j];
			}
			//TODO: s16b reward_statuseffect[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS];
		}
	}

	/* give one unified message per reward type that was handed out */
	if (pInd && q_ptr->individual) {
		if (r_obj == 1) msg_print(pInd, "You have received an item.");
		else if (r_obj) msg_format(pInd, "You have received %d items.", r_obj);
		if (r_gold) msg_format(pInd, "You have received %d gold piece%s.", r_gold, r_gold == 1 ? "" : "s");
		if (r_exp) msg_format(pInd, "You have received %d experience point%s.", r_exp, r_exp == 1 ? "" : "s");
	} else for (i = 1; i <= NumPlayers; i++) {
		if (!inarea(&Players[i]->wpos, &q_ptr->current_wpos[0])) continue; //TODO: multiple current_wpos, one for each questor!!
		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

		if (r_obj == 1) msg_print(i, "You have received an item.");
		else if (r_obj) msg_format(i, "You have received %d items.", r_obj);
		if (r_gold) msg_format(i, "You have received %d gold piece%s.", r_gold, r_gold == 1 ? "" : "s");
		if (r_exp) msg_format(i, "You have received %d experience point%s.", r_exp, r_exp == 1 ? "" : "s");
	}

	return; /* goals are not complete yet */
}

/* Check if quest goals of the current stage have been completed and accordingly
   call quest_goal_check_reward() and/or quest_set_stage() to advance.
   Goals can only be completed by players who are pursuing that quest.
   'interacting' is TRUE if a player is interacting with the questor. */
static bool quest_goal_check(int pInd, int q_idx, bool interacting) {
	int stage;

	/* check goals for stage advancement */
	stage = quest_goal_check_stage(pInd, q_idx);
	if (stage == -1) return FALSE; /* stage not yet completed */

	/* check goals for rewards */
	quest_goal_check_reward(pInd, q_idx);

	/* advance stage! */
	quest_set_stage(pInd, q_idx, stage, FALSE);
	return TRUE; /* stage has been completed and changed to the next stage */
}

/* Check if a player's location is around any of his quest destinations (target/delivery). */
void quest_check_player_location(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i, j, q_idx, stage;
	quest_info *q_ptr;
	bool found = FALSE;

	/* Quests: Is this wpos a target location or a delivery location for any of our quests? */
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++) {
		/* player actually pursuing a quest? */
		if (p_ptr->quest_idx[i] == -1) continue;

		/* check for deliver location ('move' goals) */
		if (p_ptr->quest_deliver_pos[i]) {
			q_idx = p_ptr->quest_idx[i];
			q_ptr = &q_info[q_idx];

			/* quest is deactivated? */
			if (!q_ptr->active) continue;

			stage = quest_get_stage(Ind, q_idx);

			/* first clear old wpos' delivery state */
			p_ptr->quest_within_deliver_wpos[i] = FALSE;
			p_ptr->quest_deliver_xy[i] = FALSE;

			/* check the quest goals, whether any of them wants a delivery to this location */
			for (j = 0; j < QI_GOALS; j++) {
				if (!q_ptr->deliver_pos[stage][j]) continue;

				/* extend target terrain over a wide patch? */
				if (q_ptr->deliver_terrain_patch[stage][j]) {
					/* different z-coordinate = instant fail */
					if (p_ptr->wpos.wz != q_ptr->deliver_wpos[stage][j].wz) continue;
					/* are we within range and have same terrain type? */
					if (distance(p_ptr->wpos.wx, p_ptr->wpos.wy, q_ptr->deliver_wpos[stage][j].wx,
					    q_ptr->deliver_wpos[stage][j].wy) <= QI_TERRAIN_PATCH_RADIUS &&
					    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type ==
					    wild_info[q_ptr->deliver_wpos[stage][j].wy][q_ptr->deliver_wpos[stage][j].wx].type) {
						/* imprint new temporary destination location information */
						p_ptr->quest_within_deliver_wpos[i] = TRUE;
						/* specific x,y loc? */
						if (q_ptr->deliver_pos_x[stage][j] != -1) {
							p_ptr->quest_deliver_xy[i] = TRUE;
							/* and check right away if we're already on the correct x,y location */
							quest_check_goal_deliver_xy(Ind);
						} else {
							/* we are at the requested deliver location (for at least this quest)! (wpos) */
							quest_check_goal_deliver_wpos(Ind);
						}
						found = TRUE;
						break;
					}
				}
				/* just check a single, specific wpos? */
				else if (inarea(&q_ptr->deliver_wpos[stage][j], &p_ptr->wpos)) {
					/* imprint new temporary destination location information */
					p_ptr->quest_within_deliver_wpos[i] = TRUE;
					/* specific x,y loc? */
					if (q_ptr->deliver_pos_x[stage][j] != -1) {
						p_ptr->quest_deliver_xy[i] = TRUE;
						/* and check right away if we're already on the correct x,y location */
						quest_check_goal_deliver_xy(Ind);
					} else {
						/* we are at the requested deliver location (for at least this quest)! (wpos) */
						quest_check_goal_deliver_wpos(Ind);
					}
					found = TRUE;
					break;
				}
			}
		}
		/* check for target location (kill/retrieve goals) */
		if (!found && p_ptr->quest_target_pos[i]) {
			q_idx = p_ptr->quest_idx[i];
			q_ptr = &q_info[q_idx];

			/* quest is deactivated? */
			if (!q_ptr->active) continue;

			stage = quest_get_stage(Ind, q_idx);

			/* first clear old wpos' target state */
			p_ptr->quest_within_target_wpos[i] = FALSE;
			p_ptr->quest_target_xy[i] = FALSE;

			/* check the quest goals, whether any of them wants a target to this location */
			for (j = 0; j < QI_GOALS; j++) {
				if (!q_ptr->target_pos[stage][j]) continue;

				/* extend target terrain over a wide patch? */
				if (q_ptr->target_terrain_patch[stage][j]) {
					/* different z-coordinate = instant fail */
					if (p_ptr->wpos.wz != q_ptr->target_wpos[stage][j].wz) continue;
					/* are we within range and have same terrain type? */
					if (distance(p_ptr->wpos.wx, p_ptr->wpos.wy, q_ptr->target_wpos[stage][j].wx,
					    q_ptr->target_wpos[stage][j].wy) <= QI_TERRAIN_PATCH_RADIUS &&
					    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type ==
					    wild_info[q_ptr->target_wpos[stage][j].wy][q_ptr->target_wpos[stage][j].wx].type) {
						/* imprint new temporary destination location information */
						p_ptr->quest_within_target_wpos[i] = TRUE;
						/* specific x,y loc? */
						if (q_ptr->target_pos_x[stage][j] != -1) p_ptr->quest_target_xy[i] = TRUE;
						found = TRUE;
						break;
					}
				}
				/* just check a single, specific wpos? */
				else if (inarea(&q_ptr->target_wpos[stage][j], &p_ptr->wpos)) {
					/* imprint new temporary destination location information */
					p_ptr->quest_within_target_wpos[i] = TRUE;
					/* specific x,y loc? */
					if (q_ptr->target_pos_x[stage][j] != -1) p_ptr->quest_target_xy[i] = TRUE;
					found = TRUE;
					break;
				}
			}
		}
		if (found) break;
	}
}
