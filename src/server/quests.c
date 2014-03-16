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


/* Called every minute to check which quests to activate/deactivate depending on time/daytime */
void process_quests(void) {
	quest_info *q_ptr;
	int i;
	bool night = IS_NIGHT_RAW, day = !night, morning = IS_MORNING, forenoon = IS_FORENOON, noon = IS_NOON;
	bool afternoon = IS_AFTERNOON, evening = IS_EVENING, midnight = IS_MIDNIGHT, deepnight = IS_DEEPNIGHT;
	int minute = bst(MINUTE, turn);
	bool active;

	for (i = 0; i < max_q_idx; i++) {
		q_ptr = &q_info[i];

		if (q_ptr->cur_cooldown) q_ptr->cur_cooldown--;
		if (q_ptr->disabled || q_ptr->cur_cooldown) continue;

		/* check if quest should be active */
		active = FALSE;
		if (q_ptr->night && night) active = TRUE; /* don't include pseudo-night like for Halloween/Newyearseve */
		if (q_ptr->day && day) active = TRUE;
		if (q_ptr->morning && morning) active = TRUE;
		if (q_ptr->forenoon && forenoon) active = TRUE;
		if (q_ptr->noon && noon) active = TRUE;
		if (q_ptr->afternoon && afternoon) active = TRUE;
		if (q_ptr->evening && evening) active = TRUE;
		if (q_ptr->midnight && midnight) active = TRUE;
		if (q_ptr->deepnight && deepnight) active = TRUE;
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
		s_printf("QUEST_SPAWNED: Questor at %d,%d,%d - %d,%d.\n", wpos.wx, wpos.wy, wpos.wz, x, y);
#endif

		q_ptr->talk_focus[q] = 0;
	}
	
	/* Initialise with starting stage 0 */
	q_ptr->start_turn = turn;
	q_ptr->stage = -1;
	quest_stage(0, q_idx, 0, FALSE);
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

/* a quest has ended, clean up */
static void quest_terminate(int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, j;

	/* give players credit */
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (p_ptr->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

		if (p_ptr->quest_done[q_idx] < 10000) p_ptr->quest_done[q_idx]++;
	}

	/* don't respawn the questor *immediately* again, looks silyl */
	if (!q_ptr->cooldown) q_ptr->cur_cooldown = 5; /* wait 5 minutes */
	else q_ptr->cur_cooldown = q_ptr->cooldown;

	/* clean up */
	quest_deactivate(q_idx);
}

/* 1) we've entered a quest stage and it's pretty much 'empty' so we might terminate
      if nothing more is up. check for free rewards first and hand them out.
   2) goals were completed, before advancing the stage, hand out the proper rewards. */
static void quest_rewards(q_idx) {
}

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

/* Advance quest to a different stage (or start it out if stage is 0) */
void quest_stage(int pInd, int q_idx, int stage, bool quiet) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, j;
	bool anything = FALSE;

	/* dynamic info */
	//int stage_prev = quest_get_stage(pInd, q_idx);

	/* new stage is active.
	   for 'individual' quests this is just a temporary value used by quest_imprint_stage().
	   It is still used for the other stage-checking routines in this function too though:
	    quest_rewards(), quest_terminate() and the 'anything' check. */
	q_ptr->stage = stage;


	/* if a participating player is around the questor, entering a new stage..
	 *
	 * - qutomatically invokes the quest dialogue with him again (if any)
	 * - store information of the current stage in the p_ptr array,
	 *   eg the target location for easier lookup */
	for (i = 1; i <= NumPlayers; i++) {
		if (!inarea(&Players[i]->wpos, &q_ptr->current_wpos[0])) continue; //TODO: multiple current_wpos, one for each questor!!
		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

		quest_imprint_stage(i, q_idx, j);
		for (j = 0; j < q_ptr->questors; j++)
			if (!quiet) quest_dialogue(i, q_idx, j, FALSE);
	}


	/* check for handing out rewards! (in case they're free) */
	quest_rewards(q_idx);


	/* quest termination? */
	if (q_ptr->ending_stage && q_ptr->ending_stage == stage) quest_terminate(q_idx);

	/* auto-quest-termination? (actually redundant with ending_stage)
	   If a stage has no dialogue keywords, or stage goals, or timed/auto stage change
	   effects or questor-movement/tele/revert-from-hostile effects, THEN the quest will end. */
	   //TODO: implement all of that stuff :p
	/* optional goals play no role, obviously */
	for (i = 0; i < QI_GOALS; i++)
		if (q_ptr->kill[stage][i] || q_ptr->retrieve[stage][i] || q_ptr->deliver_pos[stage][i]) {
			anything = TRUE;
			break;
		}
	/* now check remaining dialogue options (keywords) */
	for (j = 0; j < q_ptr->questors; j++)
		for (i = 0; i < QI_MAX_KEYWORDS; i++)
			if (q_ptr->keyword[j][stage][i] &&
			    /* and it's not just a keyword-reply without a stage change? */
			    q_ptr->keyword_stage[j][stage][i] != -1) {
				anything = TRUE;
				break;
			}
	/* check auto/timed stage changes */
	if (q_ptr->change_stage[stage]) anything = TRUE;
	if (q_ptr->timed_stage_ingame[stage]) anything = TRUE;
	if (q_ptr->timed_stage_ingame_abs[stage]) anything = TRUE;
	if (q_ptr->timed_stage_real[stage]) anything = TRUE;

	/* really nothing left to do? */
	if (!anything) quest_terminate(q_idx);
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
void quest_imprint_stage(int Ind, int q_idx, int py_q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, stage;

	/* for 'individual' quests: imprint the individual stage for a player.
	   the globally set q_ptr->stage is in this case just a temporary value,
	   set by quest_stage() for us, that won't be of any further consequence. */
	stage = q_ptr->stage;
	if (q_ptr->individual) p_ptr->quest_stage[py_q_idx] = stage;

	/* find out if we are pursuing any sort of target locations */
	p_ptr->quest_target_pos[py_q_idx] = FALSE;
	p_ptr->quest_targetopt_pos[py_q_idx] = FALSE;
	p_ptr->quest_deliver_pos[py_q_idx] = FALSE;
	p_ptr->quest_deliveropt_pos[py_q_idx] = FALSE;

	for (i = 0; i < QI_GOALS; i++) {
		p_ptr->quest_target_pos[py_q_idx] = q_ptr->target_pos[stage][0];
		p_ptr->quest_deliver_pos[py_q_idx] = q_ptr->deliver_pos[stage][0];
	}
	for (i = 0; i < QI_OPTIONAL; i++) {
		p_ptr->quest_targetopt_pos[py_q_idx] = q_ptr->targetopt_pos[stage][0];
		p_ptr->quest_deliveropt_pos[py_q_idx] = q_ptr->deliveropt_pos[stage][0];
	}
}

cptr qi_msg_done = "\377oYou cannot acquire this quest again.";
cptr qi_msg_max = "\377yYou are already pursuing the maximum possible number of concurrent quests.";

/* Acquire a quest, WITHOUT CHECKING whether the quest actually allows this at this stage! */
bool quest_acquire(int Ind, int q_idx, bool quiet, cptr msg) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, j;

	msg = NULL;

	/* quests is only for admins or privileged accounts at the moment? (for testing purpose) */
	switch (q_ptr->privilege) {
	case 1: if (!p_ptr->privileged && !is_admin(p_ptr)) return FALSE;
		break;
	case 2: if (p_ptr->privileged != 2 && !is_admin(p_ptr)) return FALSE;
		break;
	case 3: if (!is_admin(p_ptr)) return FALSE;
	}

	/* has the player completed this quest already/too often? */
	if (p_ptr->quest_done[q_idx] > q_ptr->repeatable && q_ptr->repeatable != -1) {
		if (!quiet) msg = qi_msg_done;
		return FALSE;
	}

	/* does the player have capacity to pick up one more quest? */
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == -1) break;
	if (i == MAX_CONCURRENT_QUESTS) {
		if (!quiet) msg = qi_msg_max;
		return FALSE;
	}

	/* voila, player acquires this quest! */
	p_ptr->quest_idx[i] = q_idx;
	strcpy(p_ptr->quest_codename[i], q_ptr->codename);

	/* for 'individual' quests, reset temporary quest data or it might get carried over from previous quest */
	p_ptr->quest_stage[i] = 0; /* note that individual quests can ONLY start in stage 0, obviously */
	for (j = 0; j < QI_FLAGS; j++) p_ptr->quest_flags[i][j] = FALSE;
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
		msg_print(Ind, "\374 ");
		//msg_format(Ind, "\374\377U**\377u You have acquired the quest \"\377U%s\377u\" \377U**", q_name + q_ptr->name);
		switch (q_ptr->privilege) {
		case 0: msg_format(Ind, "\374\377uYou have acquired the quest \"\377U%s\377u\"", q_name + q_ptr->name);
			break;
		case 1: msg_format(Ind, "\374\377uYou have acquired the quest \"\377U%s\377u\" (\377yprivileged\377u)", q_name + q_ptr->name);
			break;
		case 2: msg_format(Ind, "\374\377uYou have acquired the quest \"\377U%s\377u\" (\377ohighly privileged\377u)", q_name + q_ptr->name);
			break;
		case 3: msg_format(Ind, "\374\377uYou have acquired the quest \"\377U%s\377u\" (\377radmins only\377u)", q_name + q_ptr->name);
		}
	}

	/* store information of the current stage in the p_ptr array,
	   eg the target location for easier lookup */
	quest_imprint_stage(Ind, q_idx, i);

	return TRUE;
}

/* A player interacts with the questor (bumps him if a creature :-p) */
#define ALWAYS_DISPLAY_QUEST_TEXT
void quest_interact(int Ind, int q_idx, int questor_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, stage = quest_get_stage(Ind, q_idx);
	cptr msg = NULL;//compiler warning

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

	/* questor interaction may automatically acquire the quest */
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

		if (!quest_acquire(Ind, q_idx, FALSE, msg)) {
#ifdef ALWAYS_DISPLAY_QUEST_TEXT /* still display the initial quest text even if we cannot acquire the quest? */
			if (q_ptr->talk[questor_idx][stage][0]) { /* there is NPC talk to display? */
				p_ptr->interact_questor_idx = questor_idx;
				msg_print(Ind, "\374 ");
				msg_format(Ind, "\374\377u<\377B%s\377u> speaks to you:", q_ptr->questor_name[questor_idx]);
				for (i = 0; i < QI_TALK_LINES; i++) {
					if (!q_ptr->talk[questor_idx][stage][i]) break;
					msg_format(Ind, "\374\377U%s", q_ptr->talk[questor_idx][stage][i]);
				}
				msg_print(Ind, "\374 ");
			}
#endif
		}
		if (msg) msg_print(Ind, msg);
	}


	/* check for quest goals that have been completed and just required delivery back here */
	if (quest_goal_check(Ind, q_idx, TRUE)) return;


	/* questor interaction qutomatically invokes the quest dialogue, if any */
	q_ptr->talk_focus[questor_idx] = Ind; /* only this player can actually respond with keywords */
	quest_dialogue(Ind, q_idx, questor_idx, FALSE);
}

/* Talk vs keyword dialogue between questor and player.
   This is initiated either by bumping into the questor or by entering a new
   stage while in the area of the questor.
   Note that if the questor is focussed, only that player may respond with keywords.
   'repeat' will repeat requesting an input and skip the usual dialogue. Used for
   keyword input when a keyword wasn't recognized. */
void quest_dialogue(int Ind, int q_idx, int questor_idx, bool repeat) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, stage = quest_get_stage(Ind, q_idx);

	if (!repeat && q_ptr->talk[questor_idx][stage][0]) { /* there is NPC talk to display? */
		p_ptr->interact_questor_idx = questor_idx;
		msg_print(Ind, "\374 ");
		msg_format(Ind, "\374\377u<\377B%s\377u> speaks to you:", q_ptr->questor_name[questor_idx]);
		for (i = 0; i < QI_TALK_LINES; i++) {
			if (!q_ptr->talk[questor_idx][stage][i]) break;
			msg_format(Ind, "\374\377U%s", q_ptr->talk[questor_idx][stage][i]);
		}
		msg_print(Ind, "\374 ");
	}

	/* If there are any keywords in this stage, prompt the player for a reply.
	   If the questor is focussed on one player, only he can give a reply,
	   otherwise the first player to reply can advance the stage. */
	if (q_ptr->talk_focus[questor_idx] && q_ptr->talk_focus[questor_idx] != Ind) return;
	p_ptr->interact_questor_idx = questor_idx;
	if (q_ptr->keyword[questor_idx][stage][0]) { /* at least 1 keyword available? */
		/* hack: if 1st keyword is empty string "", display a "more" prompt */
		if (q_ptr->keyword[questor_idx][stage][0][0] == 0)
			Send_request_str(Ind, RID_QUEST + q_idx, "Your reply (or ENTER for more)> ", "");
		else
			Send_request_str(Ind, RID_QUEST + q_idx, "Your reply> ", "");
	}
}

/* Player replied in a questor dialogue by entering a keyword */
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

	/* scan keywords for match */
	for (i = 0; i < QI_MAX_KEYWORDS; i++) {
		if (!q_ptr->keyword[questor_idx][stage][i]) break; /* no more keywords? */
		if (strcmp(q_ptr->keyword[questor_idx][stage][i], str)) continue; /* not matching? */

		/* reply? */
		if (q_ptr->keyword_reply[questor_idx][stage][i] &&
		    q_ptr->keyword_reply[questor_idx][stage][i][0]) {
			msg_print(Ind, "\374 ");
			msg_format(Ind, "\374\377u<\377B%s\377u> speaks to you:", q_ptr->questor_name[questor_idx]);
			for (j = 0; j < QI_TALK_LINES; j++) {
				if (!q_ptr->keyword_reply[questor_idx][stage][i][j]) break;
				msg_format(Ind, "\374\377U%s", q_ptr->keyword_reply[questor_idx][stage][i][j]);
			}
			msg_print(Ind, "\374 ");
		}

		/* stage change? */
		if (q_ptr->keyword_stage[questor_idx][stage][i] != -1)
			quest_stage(Ind, q_idx, q_ptr->keyword_stage[questor_idx][stage][i], FALSE);

		return;
	}
	/* it was nice small-talking to you, dude */
#if 1
	/* if keyword wasn't recognised, repeat input prompt instead of just 'dropping' the convo */
	quest_dialogue(Ind, q_idx, questor_idx, TRUE);
	/* don't give 'wassup?' style msg if we just hit RETURN.. silyl */
	if (str[0]) {
		msg_print(Ind, "\374 ");
		msg_format(Ind, "\374\377u<\377B%s\377u> has nothing to say about that.", q_ptr->questor_name[questor_idx]);
		msg_print(Ind, "\374 ");
	}
#endif
	return;
}

/* Check if quest goals of the current stage have been completed and accordingly
   call quest_reward() and/or quest_stage() to advance.
   Goals can only be completed by players who are pursuing that quest.
   'interacting' is TRUE if a player is interacting with the questor. */
bool quest_goal_check(int pInd, int q_idx, bool interacting) {
//	quest_info *q_ptr = &q_info[q_idx];
//	player_type *p_ptr = Players[Ind];
//	int i, stage = quest_get_stage(pInd, q_idx);
	return FALSE; /* stage not yet completed */


	//quest_reward(q_idx);
	//quest_stage(pInd, q_idx, , FALSE);
	return TRUE; /* stage has been completed and changed to the next stage */
}
