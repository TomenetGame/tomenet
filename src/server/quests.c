/* This file is for providing a framework for quests, allowing easy addition,
   editing and removal of them via a 'q_info.txt' file designed from scratch.
   You may modify/use it freely as long as you give proper credit. - C. Blue

   I hope I covered all sorts of quests, even though some quests might be a bit
   clunky to implement (quest needs to 'quietly' spawn a followup quest etc).
   Note: This is a new approach. Not to be confused with events, old xorders,
         or the sketches of quest_type, quest[] and plots[] code in bldg.c.

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
   not confuse him.. maybe this could be improved on in the future :-p
   how to possibly improve: spawn sub questors with their own dialogues.

   Code quirks:
   Currently instead of checking pInd against 0, all those functions additionally verify
   against p_ptr->individual to make double-sure it's a (non)individual quest.
   This is because some external functions that take 'Ind' instead of 'pInd' might just
   forward their Ind everywhere into the static functions. :-p
   (As a result, some final 'global quest' lines in set/get functions might never get
   called so they are actually obsolete.)

   Regarding party members, that could be done by: Scanning area for party members on
   questor interaction, ask them if they want to join the quest y/n, and then duplicating
   all quest message output to them. Further, any party member who is first to do so  can
   complete a goal or invoke a dialogue. Only the party member who INVOKES a dialogue can
   make a keyword-based decision, the others can just watch the dialogue passively.
   Stages cannot proceed until all party members are present at the questor, if the questor
   requires returning to him, or until all members are at the quest target sector, if any.
*/


#define SERVER
#include "angband.h"


/* set log level (0 to disable, 1 for normal logging, 2 for debug logging,
   3 for very verbose debug logging, 4 every single goal test is logged (deliver_xy -> every step)) */
#define QDEBUG 3

/* Disable a quest on error? */
//#define QERROR_DISABLE
/* Otherwise just put it on this cooldown (minutes) */
#define QERROR_COOLDOWN 30

/* Default cooldown in minutes. */
#define QI_COOLDOWN_DEFAULT 5


static void quest_goal_check_reward(int pInd, int q_idx);
static bool quest_goal_check(int pInd, int q_idx, bool interacting);
static void quest_dialogue(int Ind, int q_idx, int questor_idx, bool repeat, bool interact_acquire);
static void quest_imprint_tracking_information(int Ind, int py_q_idx);


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
	int minute = bst(MINUTE, turn), hour = bst(HOUR, turn);
	bool active;
	qi_stage *q_stage;

	for (i = 0; i < max_q_idx; i++) {
		q_ptr = &q_info[i];
		if (q_ptr->defined == FALSE) continue;

		if (q_ptr->cur_cooldown) q_ptr->cur_cooldown--;
		if (q_ptr->disabled || q_ptr->cur_cooldown) continue;

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


		q_stage = quest_cur_qi_stage(i);
		/* handle automatically timed stage actions */ //TODO: implement this for individual quests too
		if (q_stage->timed_countdown < 0) {
			if (hour == -q_stage->timed_countdown)
				quest_set_stage(0, i, q_stage->timed_countdown_stage, q_stage->timed_countdown_quiet);
		} else if (q_stage->timed_countdown > 0) {
			q_stage->timed_countdown--;
			if (!q_stage->timed_countdown)
				quest_set_stage(0, i, q_stage->timed_countdown_stage, q_stage->timed_countdown_quiet);
		}
	}
}

/* Spawn questor, prepare sector/floor, make things static if requested, etc. */
bool quest_activate(int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, q, m_idx;
	cave_type **zcave, *c_ptr;
	monster_race *r_ptr, *rbase_ptr;
	monster_type *m_ptr;
	qi_questor *q_questor;

	/* data written back to q_info[] */
	struct worldpos wpos = {63, 63, 0}; //default for cases of acute paranoia
	int x, y;

	/* catch bad mistakes */
	if (!q_ptr->defined) {
		s_printf("QUEST_UNDEFINED: Cancelled attempt to activate quest %d.\n", q_idx);
		return FALSE;
	}

	/* Quest is now 'active' */
#if QDEBUG > 0
 #if QDEBUG > 1
	for (i = 1;i <= NumPlayers; i++)
		if (is_admin(Players[i]))
			msg_format(i, "Quest '%s' (%d,%s) activated.", q_name + q_ptr->name, q_idx, q_ptr->codename);
 #endif
	s_printf("%s QUEST_ACTIVATE: '%s'(%d,%s) by %s\n", showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename, q_ptr->creator);
#endif
	q_ptr->active = TRUE;


	/* Generate questor 'monsters' */
	for (q = 0; q < q_ptr->questors; q++) {
		q_questor = &q_ptr->questor[q];

		/* Only treat NPC-questors (monsters) here for now (TODO: questor-items) */
		if (q_questor->type != QI_QUESTOR_NPC) continue;

		/* If no monster race index is set for the questor, don't spawn him. (paranoia) */
		if (!q_questor->ridx) continue;


		/* ---------- Try to choose questor spawn locations ---------- */

		/* check 'L:' info for starting location -
		   for now only support 1 starting location, ie use the first one that gets tested to be eligible */
#if QDEBUG > 2
		s_printf(" QIDX %d. SWPOS,XY: %d,%d,%d - %d,%d\n", q_idx, q_questor->start_wpos.wx, q_questor->start_wpos.wy, q_questor->start_wpos.wz, q_questor->start_x, q_questor->start_y);
		s_printf(" SLOCT, STAR: %d,%d\n", q_questor->s_location_type, q_questor->s_towns_array);
#endif
		if (q_questor->start_wpos.wx != -1) {
			/* exact questor starting wpos  */
			wpos.wx = q_questor->start_wpos.wx;
			wpos.wy = q_questor->start_wpos.wy;
			wpos.wz = q_questor->start_wpos.wz;
		} else if (!q_questor->s_location_type) return FALSE;
		else if ((q_questor->s_location_type & QI_SLOC_TOWN)) {
			if ((q_questor->s_towns_array & QI_STOWN_BREE)) {
				wpos.wx = 32;
				wpos.wy = 32;
			} else if ((q_questor->s_towns_array & QI_STOWN_GONDOLIN)) {
				wpos.wx = 27;
				wpos.wy = 13;
			} else if ((q_questor->s_towns_array & QI_STOWN_MINASANOR)) {
				wpos.wx = 25;
				wpos.wy = 58;
			} else if ((q_questor->s_towns_array & QI_STOWN_LOTHLORIEN)) {
				wpos.wx = 59;
				wpos.wy = 51;
			} else if ((q_questor->s_towns_array & QI_STOWN_KHAZADDUM)) {
				wpos.wx = 26;
				wpos.wy = 5;
			} else if ((q_questor->s_towns_array & QI_STOWN_WILD)) {
			} else if ((q_questor->s_towns_array & QI_STOWN_DUNGEON)) {
			} else if ((q_questor->s_towns_array & QI_STOWN_IDDC)) {
			} else if ((q_questor->s_towns_array & QI_STOWN_IDDC_FIXED)) {
			} else return FALSE; //paranoia
		} else if ((q_questor->s_location_type & QI_SLOC_SURFACE)) {
			if ((q_questor->s_terrains & RF8_WILD_TOO)) { /* all terrains are valid */
			} else if ((q_questor->s_terrains & RF8_WILD_SHORE)) {
			} else if ((q_questor->s_terrains & RF8_WILD_SHORE)) {
			} else if ((q_questor->s_terrains & RF8_WILD_OCEAN)) {
			} else if ((q_questor->s_terrains & RF8_WILD_WASTE)) {
			} else if ((q_questor->s_terrains & RF8_WILD_WOOD)) {
			} else if ((q_questor->s_terrains & RF8_WILD_VOLCANO)) {
			} else if ((q_questor->s_terrains & RF8_WILD_MOUNTAIN)) {
			} else if ((q_questor->s_terrains & RF8_WILD_GRASS)) {
			} else if ((q_questor->s_terrains & RF8_WILD_DESERT)) {
			} else if ((q_questor->s_terrains & RF8_WILD_ICE)) {
			} else if ((q_questor->s_terrains & RF8_WILD_SWAMP)) {
			} else return FALSE; //paranoia
		} else if ((q_questor->s_location_type & QI_SLOC_DUNGEON)) {
		} else if ((q_questor->s_location_type & QI_SLOC_TOWER)) {
		} else return FALSE; //paranoia

		q_questor->current_wpos.wx = wpos.wx;
		q_questor->current_wpos.wy = wpos.wy;
		q_questor->current_wpos.wz = wpos.wz;

		/* Allocate & generate cave */
		if (!(zcave = getcave(&wpos))) {
			//dealloc_dungeon_level(&wpos);
			alloc_dungeon_level(&wpos);
			generate_cave(&wpos, NULL);
			zcave = getcave(&wpos);
		}
		if (q_questor->static_floor) new_players_on_depth(&wpos, 1, TRUE);

		/* Specified exact starting location within given wpos? */
		if (q_questor->start_x != -1) {
			x = q_questor->start_x;
			y = q_questor->start_y;
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
				s_printf(" QUEST_CANCELLED: No free questor spawn location.\n");
				q_ptr->active = FALSE;
#ifdef QERROR_DISABLE
				q_ptr->disabled = TRUE;
#else
				q_ptr->cur_cooldown = QERROR_COOLDOWN;
#endif
				return FALSE;
			}
		}

		c_ptr = &zcave[y][x];

		q_questor->current_x = x;
		q_questor->current_y = y;


		/* ---------- Try to spawn the questor monster ---------- */

		m_idx = m_pop();
		if (!m_idx) {
			s_printf(" QUEST_CANCELLED: No free monster index to pop questor.\n");
			q_ptr->active = FALSE;
#ifdef QERROR_DISABLE
			q_ptr->disabled = TRUE;
#else
			q_ptr->cur_cooldown = QERROR_COOLDOWN;
#endif
			return FALSE;
		}

		/* make sure no other player/moster is occupying our spawning grid */
		if (c_ptr->m_idx < 0) {
			int Ind = -c_ptr->m_idx;
			player_type *p_ptr = Players[Ind];

			teleport_player(Ind, 1, TRUE);
			/* check again.. */
			if (c_ptr->m_idx < 0) teleport_player(Ind, 10, TRUE);
			/* and again.. (someone funny stone-walled the whole map?) */
			if (c_ptr->m_idx < 0) teleport_player(Ind, 200, TRUE);
			/* check again. If still here, just transport him to Bree for now -_- */
			if (c_ptr->m_idx < 0) {
				p_ptr->new_level_method = LEVEL_RAND;
				p_ptr->recall_pos.wx = cfg.town_x;
				p_ptr->recall_pos.wy = cfg.town_y;
				p_ptr->recall_pos.wz = 0;
				recall_player(Ind, "A strange force teleports you far away.");
			}
		} else if (c_ptr->m_idx > 0) {
			/* is it ANOTHER questor? uhoh */
			if (m_list[c_ptr->m_idx].questor) {
				/* we don't mess with questor locations for consistencies sake */
				s_printf(" QUEST_CANCELLED: Questor of quest %d occupies questor spawn location.\n", m_list[c_ptr->m_idx].quest);
				q_ptr->active = FALSE;
#ifdef QERROR_DISABLE
				q_ptr->disabled = TRUE;
#else
				q_ptr->cur_cooldown = QERROR_COOLDOWN;
#endif
				return FALSE;
			}

			teleport_away(c_ptr->m_idx, 1);
			/* check again.. */
			if (c_ptr->m_idx > 0) teleport_away(c_ptr->m_idx, 2);
			/* aaaand again.. */
			if (c_ptr->m_idx > 0) teleport_away(c_ptr->m_idx, 10);
			/* wow. this patience. */
			if (c_ptr->m_idx > 0) teleport_away(c_ptr->m_idx, 200);
			/* out of patience */
			if (c_ptr->m_idx > 0) delete_monster_idx(c_ptr->m_idx, TRUE);
		}
		c_ptr->m_idx = m_idx;

		m_ptr = &m_list[m_idx];
		MAKE(m_ptr->r_ptr, monster_race);
		r_ptr = m_ptr->r_ptr;
		rbase_ptr = &r_info[q_questor->ridx];

		m_ptr->questor = TRUE;
		m_ptr->questor_idx = q;
		m_ptr->quest = q_idx;
		m_ptr->r_idx = q_questor->ridx;
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
		if (q_questor->invincible) r_ptr->flags7 |= RF7_NO_DEATH; //for now we just use NO_DEATH flag for invincibility
		r_ptr->flags8 |= RF8_GENO_PERSIST | RF8_GENO_NO_THIN | RF8_ALLOW_RUNNING | RF8_NO_AUTORET;
		r_ptr->flags9 |= RF9_IM_TELE;

		r_ptr->text = 0;
		r_ptr->name = rbase_ptr->name;
		m_ptr->fx = x;
		m_ptr->fy = y;

		r_ptr->d_attr = q_questor->rattr;
		r_ptr->d_char = q_questor->rchar;
		r_ptr->x_attr = q_questor->rattr;
		r_ptr->x_char = q_questor->rchar;
	
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
					astar_info_open[i].m_idx = m_idx;
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
				s_printf(" QUEST_CANCELLED: No free A* index for questor.\n");
				q_ptr->active = FALSE;
 #ifdef QERROR_DISABLE
				q_ptr->disabled = TRUE;
 #else
				q_ptr->cur_cooldown = QERROR_COOLDOWN;
 #endif
				return FALSE;
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

		m_ptr->questor_invincible = q_questor->invincible; //for now handled by RF7_NO_DEATH
		m_ptr->questor_hostile = FALSE;
		q_questor->m_idx = m_idx;

		update_mon(c_ptr->m_idx, TRUE);
#if QDEBUG > 1
		s_printf(" QUEST_SPAWNED: Questor '%s' (m_idx %d) at %d,%d,%d - %d,%d.\n", q_questor->name, m_idx, wpos.wx, wpos.wy, wpos.wz, x, y);
#endif

		q_questor->talk_focus = 0;
	}

	/* Initialise with starting stage 0 */
	q_ptr->turn_activated = turn;
	q_ptr->cur_stage = -1;
	quest_set_stage(0, q_idx, 0, FALSE);
	return TRUE;
}

/* Despawn questor, unstatic sector/floor, etc. */
//TODO: implement multiple questors
void quest_deactivate(int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_questor *q_questor;
	int i, m_idx;
	cave_type **zcave, *c_ptr;
	//monster_race *r_ptr;
	//monster_type *m_ptr;

	/* data reread from q_info[] */
	struct worldpos wpos;
	//int qx, qy;

#if QDEBUG > 0
	s_printf("%s QUEST_DEACTIVATE: '%s' (%d,%s) by %s\n", showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename, q_ptr->creator);
#endif
	q_ptr->active = FALSE;


	for (i = 0; i < q_ptr->questors; i++) {
		q_questor = &q_ptr->questor[i];

		/* get quest information */
		wpos.wx = q_questor->current_wpos.wx;
		wpos.wy = q_questor->current_wpos.wy;
		wpos.wz = q_questor->current_wpos.wz;

		/* Allocate & generate cave */
		if (!(zcave = getcave(&wpos))) {
			//dealloc_dungeon_level(&wpos);
			alloc_dungeon_level(&wpos);
			generate_cave(&wpos, NULL);
			zcave = getcave(&wpos);
		}
		c_ptr = &zcave[q_questor->current_y][q_questor->current_x];

		/* unmake quest */
#if QDEBUG > 1
		s_printf(" deleting questor %d at %d,%d,%d - %d,%d\n", c_ptr->m_idx, wpos.wx, wpos.wy, wpos.wz, q_questor->current_x, q_questor->current_y);
#endif
		m_idx = c_ptr->m_idx;
		if (m_idx) {
			if (m_list[m_idx].questor && m_list[m_idx].quest == q_idx) {
#if QDEBUG > 1
				s_printf(" ..ok.\n");
#endif
				delete_monster_idx(c_ptr->m_idx, TRUE);
			} else
#if QDEBUG > 1
				s_printf(" ..failed: Monster is not a questor or has a different quest idx.\n");
#endif
		}
		if (q_questor->static_floor) new_players_on_depth(&wpos, 0, FALSE);
	}
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

/* Erase all temporary quest data of the player. */
static void quest_initialise_player_tracking(int Ind, int py_q_idx) {
	player_type *p_ptr = Players[Ind];
	int i;

	/* initialise the global info by deriving it from the other
	   concurrent quests we have _except_ for py_q_idx.. oO */
	p_ptr->quest_any_k = FALSE;
	p_ptr->quest_any_k_target = FALSE;
	p_ptr->quest_any_k_within_target = FALSE;

	p_ptr->quest_any_r = FALSE;
	p_ptr->quest_any_r_target = FALSE;
	p_ptr->quest_any_r_within_target = FALSE;

	p_ptr->quest_any_deliver_xy = FALSE;
	p_ptr->quest_any_deliver_xy_within_target = FALSE;

	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++) {
		if (i == py_q_idx) continue;
		/* expensive mechanism, sort of */
		quest_imprint_tracking_information(Ind, i);
	}

	/* clear direct data */
	p_ptr->quest_kill[py_q_idx] = FALSE;
	p_ptr->quest_retrieve[py_q_idx] = FALSE;

	p_ptr->quest_deliver_pos[py_q_idx] = FALSE;
	p_ptr->quest_deliver_xy[py_q_idx] = FALSE;

	/* for 'individual' quests, reset temporary quest data or it might get carried over from previous stage? */
	for (i = 0; i < QI_GOALS; i++) p_ptr->quest_goals[py_q_idx][i] = FALSE;
	for (i = 0; i < QI_OPTIONAL; i++) p_ptr->quest_goalsopt[py_q_idx][i] = FALSE;
}

/* a quest has successfully ended, clean up */
static void quest_terminate(int pInd, int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, j;

	/* give players credit */
	if (pInd && q_ptr->individual) {
		p_ptr = Players[pInd];
#if QDEBUG > 0
		s_printf("%s QUEST_TERMINATE_INDIVIDUAL: %s(%d) completes '%s'(%d,%s)\n", showtime(), p_ptr->name, pInd, q_name + q_ptr->name, q_idx, q_ptr->codename);
#endif

		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (p_ptr->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) return; //oops?

		if (p_ptr->quest_done[q_idx] < 10000) p_ptr->quest_done[q_idx]++;

		/* he is no longer on the quest, since the quest has finished */
		p_ptr->quest_idx[j] = -1;
		//good colours for '***': C-confusion (yay), q-inertia (pft, extended), E-meteor (mh, extended)
		msg_format(pInd, "\374\377C***\377u You have completed the quest \"\377U%s\377u\"! \377C***", q_name + q_ptr->name);
		//msg_print(pInd, "\374 ");

		/* don't respawn the questor *immediately* again, looks silyl */
		if (q_ptr->cooldown == -1) p_ptr->quest_cooldown[q_idx] = QI_COOLDOWN_DEFAULT;
		else p_ptr->quest_cooldown[q_idx] = q_ptr->cooldown;

		/* individual quests don't get cleaned up (aka completely reset)
		   by deactivation, except for this temporary tracking data,
		   or it would continue spamming quest checks eg on delivery_xy locs. */
		quest_initialise_player_tracking(pInd, j);
		return;
	}

#if QDEBUG > 0
	s_printf("%s QUEST_TERMINATE_GLOBAL: '%s'(%d,%s) completed by", showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename);
#endif
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (p_ptr->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

#if QDEBUG > 0
		s_printf(" %s(%d)", q_name + q_ptr->name, q_ptr->codename);
#endif
		if (p_ptr->quest_done[q_idx] < 10000) p_ptr->quest_done[q_idx]++;

		/* he is no longer on the quest, since the quest has finished */
		p_ptr->quest_idx[j] = -1;
		msg_format(i, "\374\377C***\377u You have completed the quest \"\377U%s\377u\"! \377C***", q_name + q_ptr->name);
		//msg_print(i, "\374 ");

		/* clean up temporary tracking data,
		   or it would continue spamming quest checks eg on delivery_xy locs. */
		quest_initialise_player_tracking(i, j);
	}
#if QDEBUG > 0
	s_printf(".\n");
#endif

	/* don't respawn the questor *immediately* again, looks silyl */
	if (q_ptr->cooldown == -1) q_ptr->cur_cooldown = QI_COOLDOWN_DEFAULT;
	else q_ptr->cur_cooldown = q_ptr->cooldown;

	/* clean up */
	quest_deactivate(q_idx);
}

/* return a current quest goal. Either just uses q_ptr->goals directly for global
   quests, or p_ptr->quest_goals for individual quests.
   'nisi' will cause returning FALSE if the quest goal is marked as 'nisi',
   even if it is also marked as 'completed'. */
static bool quest_get_goal(int pInd, int q_idx, int goal, bool nisi) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, stage = quest_get_stage(pInd, q_idx);
	qi_goal *q_goal = &quest_qi_stage(q_idx, stage)->goal[goal];

	if (!pInd || !q_ptr->individual) {
		if (nisi && q_goal->nisi) return FALSE;
		return q_goal->cleared; /* no player? global goal */
	}

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) {
		if (nisi && q_goal->nisi) return FALSE;
		return q_goal->cleared; /* player isn't on this quest. return global goal. */
	}

	if (q_ptr->individual) {
		if (nisi && p_ptr->quest_goals_nisi[i][goal]) return FALSE;
		return p_ptr->quest_goals[i][goal]; /* individual quest */
	}

	if (nisi && q_goal->nisi) return FALSE;
	return q_goal->cleared; /* global quest */
}

/* return an optional current quest goal. Either just uses q_ptr->goalsopt directly for global
   quests, or p_ptr->quest_goalsopt for individual quests. */
#if 0 /* compiler warning 'unused' */
static bool quest_get_goalopt(int pInd, int q_idx, int goalopt) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, stage = quest_get_stage(pInd, q_idx);

	if (!pInd || !q_ptr->individual) return q_ptr->goalsopt[stage][goalopt]; /* no player? global goal */

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) return q_ptr->goalsopt[stage][goalopt]; /* player isn't on this quest. return global goal. */

	if (q_ptr->individual) return p_ptr->quest_goalsopt[i][goalopt]; /* individual quest */
	return q_ptr->goalsopt[stage][goalopt]; /* global quest */
}
#endif

/* Returns the current quest->stage struct. */
qi_stage *quest_cur_qi_stage(int q_idx) {
	return &q_info[q_idx].stage[q_info[q_idx].cur_stage];
}
/* Returns a quest->stage struct to a 'stage' index used in q_info.txt. */
qi_stage *quest_qi_stage(int q_idx, int stage) {
	return &q_info[q_idx].stage[q_info[q_idx].stage_idx[stage]];
}
/* return the current quest stage. Either just uses q_ptr->cur_stage directly for global
   quests, or p_ptr->quest_stage for individual quests. */
s16b quest_get_stage(int pInd, int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i;

	if (!pInd || !q_ptr->individual) return q_ptr->cur_stage; /* no player? global stage */

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) return 0; /* player isn't on this quest: pick stage 0, the entry stage */

	if (q_ptr->individual) return p_ptr->quest_stage[i]; /* individual quest */
	return q_ptr->cur_stage; /* global quest */
}

/* return current quest flags. Either just uses q_ptr->flags directly for global
   quests, or p_ptr->quest_flags for individual quests. */
static u16b quest_get_flags(int pInd, int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i;

	if (!pInd || !q_ptr->individual) return q_ptr->flags; /* no player? global goal */

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) return q_ptr->flags; /* player isn't on this quest. return global goal. */

	if (q_ptr->individual) return p_ptr->quest_flags[i]; /* individual quest */
	return q_ptr->flags; /* global quest */
}
/* set/clear quest flags */
static void quest_set_flags(int pInd, int q_idx, u16b set_mask, u16b clear_mask) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i;

	if (!pInd || !q_ptr->individual) {
		q_ptr->flags |= set_mask; /* no player? global flags */
		q_ptr->flags &= ~clear_mask;
		return;
	}

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) {
		q_ptr->flags |= set_mask; /* player isn't on this quest. return global flags. */
		q_ptr->flags &= ~clear_mask;
		return;
	}

	if (q_ptr->individual) {
		p_ptr->quest_flags[i] |= set_mask; /* individual quest */
		p_ptr->quest_flags[i] &= ~clear_mask; 
		return;
	}

	/* global quest */
	q_ptr->flags |= set_mask;
	q_ptr->flags &= ~clear_mask;
}
/* according to Z lines, change flags when a quest goal has finally be resolved. */
//TODO for optional goals too..
static void quest_goal_changes_flags(int pInd, int q_idx, int stage, int goal) {
	qi_goal *q_goal = &quest_qi_stage(q_idx, stage)->goal[goal];

	quest_set_flags(pInd, q_idx, q_goal->setflags, q_goal->clearflags);
}

/* mark a quest goal as reached.
   Also check if we can now proceed to the next stage or set flags or hand out rewards.
   'nisi' is TRUE for kill/retrieve quests that depend on a delivery.
   This was added for handling flag changes induced by completing stage goals. */
static void quest_set_goal(int pInd, int q_idx, int goal, bool nisi) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, stage = quest_get_stage(pInd, q_idx);
	qi_goal *q_goal = &quest_qi_stage(q_idx, stage)->goal[goal];

#if QDEBUG > 2
	s_printf("QUEST_GOAL_SET: (%s,%d) goal %d%s by %d\n", q_ptr->codename, q_idx, goal, nisi ? "n" : "", pInd);
#endif
	if (!pInd || !q_ptr->individual) {
		if (!q_goal->cleared || !nisi) q_goal->nisi = nisi;
		q_goal->cleared = TRUE; /* no player? global goal */

		/* change flags according to Z lines? */
		if (!q_goal->nisi) quest_goal_changes_flags(0, q_idx, stage, goal);

		(void)quest_goal_check(0, q_idx, FALSE);
		return;
	}

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) {
		if (!q_goal->cleared || !nisi) q_goal->nisi = nisi;
		q_goal->cleared = TRUE; /* player isn't on this quest. return global goal. */

		/* change flags according to Z lines? */
		if (!q_goal->nisi) quest_goal_changes_flags(0, q_idx, stage, goal);

		(void)quest_goal_check(0, q_idx, FALSE);
		return;
	}

	if (q_ptr->individual) {
		if (!p_ptr->quest_goals[i][goal] || !nisi) p_ptr->quest_goals_nisi[i][goal] = nisi;
		p_ptr->quest_goals[i][goal] = TRUE; /* individual quest */

		/* change flags according to Z lines? */
		if (!p_ptr->quest_goals_nisi[i][goal]) quest_goal_changes_flags(pInd, q_idx, stage, goal);

		(void)quest_goal_check(pInd, q_idx, FALSE);
		return;
	}

	if (!q_goal->cleared || !nisi) q_goal->nisi = nisi;
	q_goal->cleared = TRUE; /* global quest */
	/* change flags according to Z lines? */

	if (!q_goal->nisi) quest_goal_changes_flags(0, q_idx, stage, goal);

	/* also check if we can now proceed to the next stage or set flags or hand out rewards */
	(void)quest_goal_check(0, q_idx, FALSE);
}
/* mark a quest goal as no longer reached. ouch. */
static void quest_unset_goal(int pInd, int q_idx, int goal) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, stage = quest_get_stage(pInd, q_idx);
	qi_goal *q_goal = &quest_qi_stage(q_idx, stage)->goal[goal];

#if QDEBUG > 2
	s_printf("QUEST_GOAL_UNSET: (%s,%d) goal %d by %d\n", q_ptr->codename, q_idx, goal, pInd);
#endif
	if (!pInd || !q_ptr->individual) {
		q_goal->cleared = FALSE; /* no player? global goal */
		return;
	}

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_CONCURRENT_QUESTS) {
		q_goal->cleared = FALSE; /* player isn't on this quest. return global goal. */
		return;
	}

	if (q_ptr->individual) {
		p_ptr->quest_goals[i][goal] = FALSE; /* individual quest */
		return;
	}

	q_goal->cleared = FALSE; /* global quest */
}

/* mark an optional quest goal as reached */
#if 0 /* compiler warning 'unused' */
static void quest_set_goalopt(int pInd, int q_idx, int goalopt, bool nisi) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, stage = quest_get_stage(pInd, q_idx);

	if (!pInd || !q_ptr->individual) {
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

	if (!pInd || !q_ptr->individual) {
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

/* perform automatic things (quest spawn/stage change) in a stage */
static bool quest_stage_automatics(int pInd, int q_idx, int stage) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);

	/* auto-spawn (and acquire) new quest? */
	if (q_stage->activate_quest != -1) {
		if (!q_info[q_stage->activate_quest].disabled &&
		    q_info[q_stage->activate_quest].defined) {
#if QDEBUG > 0
			s_printf("%s QUEST_ACTIVATE_AUTO: '%s'(%d,%s) stage %d\n",
			    showtime(), q_name + q_info[q_stage->activate_quest].name,
			    q_stage->activate_quest, q_info[q_stage->activate_quest].codename, stage);
#endif
			quest_activate(q_stage->activate_quest);
			if (q_stage->auto_accept)
				quest_acquire_confirmed(pInd, q_stage->activate_quest, q_stage->auto_accept_quiet);
		}
#if QDEBUG > 0
		else s_printf("%s QUEST_ACTIVATE_AUTO --failed--: '%s'(%d)\n",
		    showtime(), q_name + q_info[q_stage->activate_quest].name,
		    q_stage->activate_quest);
#endif
	}

	/* auto-change stage (timed)? */
	if (q_stage->change_stage != -1) {
		/* not a timed change? instant then */
		if (//!q_ptr->timed_ingame &&
		    q_stage->timed_ingame_abs != -1 && !q_stage->timed_real) {
#if QDEBUG > 0
			s_printf("%s QUEST_STAGE_AUTO: '%s'(%d,%s) %d->%d\n",
			    showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename, quest_get_stage(pInd, q_idx), q_stage->change_stage);
#endif
			quest_set_stage(pInd, q_idx, q_stage->change_stage, q_stage->quiet_change);
			/* don't imprint/play dialogue of this stage anymore, it's gone~ */
			return TRUE;
		}
		/* start the clock */
		/*cannot do this, cause quest scheduler is checking once per minute atm
		if (q_stage->timed_ingame) {
			q_stage->timed_countdown = q_stage->timed_ingame;//todo: different resolution than real minutes
			q_stage->timed_countdown_stage = q_stage->change_stage;
			q_stage->timed_countdown_quiet = q_stage->quiet_change;
		} else */
		if (q_stage->timed_ingame_abs != -1) {
			q_stage->timed_countdown = -q_stage->timed_ingame_abs;
			q_stage->timed_countdown_stage = q_stage->change_stage;
			q_stage->timed_countdown_quiet = q_stage->quiet_change;
		} else if (q_stage->timed_real) {
			q_stage->timed_countdown = q_stage->timed_real;
			q_stage->timed_countdown_stage = q_stage->change_stage;
			q_stage->timed_countdown_quiet = q_stage->quiet_change;
		}
	}

	return FALSE;
}
static void quest_imprint_tracking_information(int Ind, int py_q_idx) {
	player_type *p_ptr = Players[Ind];
	quest_info *q_ptr;
	int i, stage, q_idx;
	qi_stage *q_stage;
	qi_goal *q_goal;

	/* no quest pursued in this slot? */
	if (p_ptr->quest_idx[py_q_idx] == -1) return;
	q_idx = p_ptr->quest_idx[py_q_idx];
	q_ptr = &q_info[q_idx];

	/* our quest is unavailable for some reason? */
	if (!q_ptr->defined || !q_ptr->active) return;
	stage = quest_get_stage(Ind, q_idx);
	q_stage = quest_qi_stage(q_idx, stage);

	/* now set goal-dependant (temporary) quest info again */
	for (i = 0; i < q_stage->goals; i++) {
		q_goal = &q_stage->goal[i];

		/* set deliver location helper info */
		if (q_goal->deliver) {
			p_ptr->quest_deliver_pos[py_q_idx] = TRUE;
			/* note: deliver has no 'basic wpos check', since the whole essence of
			   "delivering" is to actually move somewhere. */
			/* finer check? */
			if (q_goal->deliver->pos_x != -1) p_ptr->quest_any_deliver_xy = TRUE;
		}

		/* set kill/retrieve tracking counter if we have such goals in this stage */
		if (q_goal->kill) {
			p_ptr->quest_kill_number[py_q_idx][i] = q_goal->kill->number;
			/* set target location helper info */
			p_ptr->quest_kill[py_q_idx] = TRUE;
			/* assume it's a restricted target "at least" */
			p_ptr->quest_any_k_target = TRUE;
			/* if it's not a restricted target, it's active everywhere */
			if (!q_goal->target_pos) {
				p_ptr->quest_any_k = TRUE;
				p_ptr->quest_any_k_within_target = TRUE;
			}
		}
		if (q_goal->retrieve) {
			p_ptr->quest_retrieve_number[py_q_idx][i] = q_goal->retrieve->number;
			/* set target location helper info */
			p_ptr->quest_retrieve[py_q_idx] = TRUE;
			/* assume it's a restricted target "at least" */
			p_ptr->quest_any_r_target = TRUE;
			/* if it's not a restricted target, it's active everywhere */
			if (!q_goal->target_pos) {
				p_ptr->quest_any_r = TRUE;
				p_ptr->quest_any_r_within_target = TRUE;
			}
		}
	}
#if 0//TODO
	for (i = 0; i < QI_OPTIONAL; i++) {
		/* set deliver location helper info */
		if (q_ptr->deliveropt_pos[stage][i]) {
			p_ptr->quest_deliver_pos[py_q_idx] = TRUE;
			/* finer check? */
			if (q_ptr->deliveropt_pos_x[stage][i] != -1) p_ptr->quest_any_deliver_xy = TRUE;
		}

		/* set kill/retrieve tracking counter if we have such goals in this stage */
		if (q_ptr->killopt[stage][i]) {
			p_ptr->quest_killopt_number[py_q_idx][i] = q_ptr->killopt_number[stage][i];
			/* set target location helper info */
			p_ptr->quest_kill[py_q_idx] = TRUE;
			/* assume it's a restricted target "at least" */
			p_ptr->quest_any_k_target = TRUE;
			/* if it's not a restricted target, it's active everywhere */
			if (!q_ptr->targetopt_pos[stage][i]) {
				p_ptr->quest_any_k = TRUE;
				p_ptr->quest_any_k_within_target = TRUE;
			}
		}
		if (q_ptr->retrieveopt[stage][i]) {
			p_ptr->quest_retrieveopt_number[py_q_idx][i] = q_ptr->retrieveopt_number[stage][i];
			/* set target location helper info */
			p_ptr->quest_retrieve[py_q_idx] = TRUE;
			/* assume it's a restricted target "at least" */
			p_ptr->quest_any_r_target = TRUE;
			/* if it's not a restricted target, it's active everywhere */
			if (!q_ptr->targetopt_pos[stage][i]) {
				p_ptr->quest_any_r = TRUE;
				p_ptr->quest_any_r_within_target = TRUE;
			}
		}
	}
#endif
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
	int stage;

	/* for 'individual' quests: imprint the individual stage for a player.
	   the globally set q_ptr->cur_stage is in this case just a temporary value,
	   set by quest_set_stage() for us, that won't be of any further consequence. */
	stage = q_ptr->cur_stage;
	if (q_ptr->individual) p_ptr->quest_stage[py_q_idx] = stage;


	/* find out if we are pursuing any sort of target locations */

	/* first, initialise all temporary info */
	quest_initialise_player_tracking(Ind, py_q_idx);

	/* now set/add our new stage's info */
	quest_imprint_tracking_information(Ind, py_q_idx);
}
/* Advance quest to a different stage (or start it out if stage is 0) */
void quest_set_stage(int pInd, int q_idx, int stage, bool quiet) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_stage *q_stage;
	int i, j, k;
	bool anything;

#if QDEBUG > 0
	s_printf("%s QUEST_STAGE: '%s'(%d,%s) %d->%d\n", showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename, quest_get_stage(pInd, q_idx), stage);
	s_printf(" actq %d, autoac %d, cstage %d\n", quest_qi_stage(q_idx, stage)->activate_quest, quest_qi_stage(q_idx, stage)->auto_accept, quest_qi_stage(q_idx, stage)->change_stage);
#endif

	/* dynamic info */
	//int stage_prev = quest_get_stage(pInd, q_idx);

	/* new stage is active.
	   for 'individual' quests this is just a temporary value used by quest_imprint_stage().
	   It is still used for the other stage-checking routines in this function too though:
	    quest_goal_check_reward(), quest_terminate() and the 'anything' check. */
	q_ptr->cur_stage = stage;
	q_stage = quest_qi_stage(q_idx, stage);

	/* For non-'individual' quests,
	   if a participating player is around the questor, entering a new stage..
	   - qutomatically invokes the quest dialogue with him again (if any)
	   - store information of the current stage in the p_ptr array,
	     eg the target location for easier lookup */
	if (!q_ptr->individual || !pInd) { //the !pInd part is paranoia
		for (i = 1; i <= NumPlayers; i++) {
			//if (!inarea(&Players[i]->wpos, &q_ptr->current_wpos[0])) continue; //seems nonsensical for narrations
			for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
				if (Players[i]->quest_idx[j] == q_idx) break;
			if (j == MAX_CONCURRENT_QUESTS) continue;

			/* play automatic narration if any */
			if (!quiet) {
				/* pre-scan narration if any line at all exists and passes the flag check */
				anything = FALSE;
				for (k = 0; k < QI_TALK_LINES; k++) {
					if (q_stage->narration[k] &&
					    ((q_stage->narration_flags[k] & quest_get_flags(pInd, q_idx)) == q_stage->narration_flags[k])) {
						anything = TRUE;
						break;
					}
				}
				/* there is narration to display? */
				if (anything) {
					msg_print(i, "\374 ");
					msg_format(i, "\374\377u<\377U%s\377u>", q_name + q_ptr->name);
					for (k = 0; k < QI_TALK_LINES; k++) {
						if (!q_stage->narration[k]) break;
						if ((q_stage->narration_flags[k] & quest_get_flags(pInd, q_idx)) != q_stage->narration_flags[k]) continue;
						msg_format(i, "\374\377U%s", q_stage->narration[k]);
					}
					//msg_print(i, "\374 ");
				}
			}

			/* update player's quest tracking data */
			quest_imprint_stage(i, q_idx, j);

			/* perform automatic actions (spawn new quest, (timed) further stage change)
			   Note: Should imprint correct stage on player before calling this,
			         otherwise automatic stage change will "skip" a stage in between ^^.
			         This should be rather cosmetic though. */
			if (quest_stage_automatics(pInd, q_idx, stage)) return;

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
				if (q_stage->narration[k] &&
				    ((q_stage->narration_flags[k] & quest_get_flags(pInd, q_idx)) == q_stage->narration_flags[k])) {
					anything = TRUE;
					break;
				}
			}
			/* there is narration to display? */
			if (anything) {
				msg_print(pInd, "\374 ");
				msg_format(pInd, "\374\377u<\377U%s\377u>", q_name + q_ptr->name);
				for (k = 0; k < QI_TALK_LINES; k++) {
					if (!q_stage->narration[k]) break;
					msg_format(pInd, "\374\377U%s", q_stage->narration[k]);
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
		s_printf("%s QUEST_STAGE_ENDING: '%s'(%d,%s) %d\n", showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename, stage);
#endif
		quest_terminate(pInd, q_idx);
	}


	/* auto-quest-termination? (actually redundant with ending_stage)
	   If a stage has no dialogue keywords, or stage goals, or timed/auto stage change
	   effects or questor-movement/tele/revert-from-hostile effects, THEN the quest will end. */
	   //TODO: implement all of that stuff :p
	anything = FALSE;

	/* optional goals play no role, obviously */
	for (i = 0; i < q_stage->goals; i++)
		if (q_stage->goal[i].kill || q_stage->goal[i].retrieve || q_stage->goal[i].deliver) {
			anything = TRUE;
			break;
		}
	/* now check remaining dialogue options (keywords) */
	for (i = 0; i < q_ptr->keywords; i++)
		if (q_ptr->keyword[i].stage_ok[stage] && /* keyword is available in this stage */
		    q_ptr->keyword[i].stage != -1) { /* and it's not just a keyword-reply without a stage change? */
			anything = TRUE;
			break;
		}
	/* check auto/timed stage changes */
	if (q_stage->change_stage != -1) anything = TRUE;
	//if (q_ptr->timed_ingame) anything = TRUE;
	if (q_stage->timed_ingame_abs != -1) anything = TRUE;
	if (q_stage->timed_real) anything = TRUE;

	/* really nothing left to do? */
	if (!anything) {
#if QDEBUG > 0
		s_printf("%s QUEST_STAGE_EMPTY: '%s'(%d,%s) %d\n", showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename, stage);
#endif
		quest_terminate(pInd, q_idx);
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
	s_printf("%s QUEST_ACQUIRED: (%d,%d,%d;%d,%d) %s (%d) has quest '%s'(%d,%s).\n",
	    showtime(), p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, p_ptr->px, p_ptr->py, p_ptr->name, Ind, q_name + q_ptr->name, q_idx, q_ptr->codename);
#endif

	/* for 'individual' quests, reset temporary quest data or it might get carried over from previous quest */
	p_ptr->quest_stage[i] = 0; /* note that individual quests can ONLY start in stage 0, obviously */
	p_ptr->quest_flags[i] = 0x0000;
	for (j = 0; j < QI_GOALS; j++) p_ptr->quest_goals[i][j] = FALSE;
	for (j = 0; j < QI_OPTIONAL; j++) p_ptr->quest_goalsopt[i][j] = FALSE;

	/* reset temporary quest helper info */
	quest_initialise_player_tracking(Ind, i);

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

	/* hack: for 'individual' quests we use q_ptr->cur_stage as temporary var to store the player's personal stage,
	   which is then transferred to the player again in quest_imprint_stage(). Yeah.. */
	if (q_ptr->individual) q_ptr->cur_stage = 0; /* 'individual' quest entry stage is always 0 */
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
	s_printf("%s QUEST_MAY_ACQUIRE: (%d,%d,%d;%d,%d) %s (%d) may acquire quest '%s'(%d,%s).\n",
	    showtime(), p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, p_ptr->px, p_ptr->py, p_ptr->name, Ind, q_name + q_ptr->name, q_idx, q_ptr->codename);
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
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);
	qi_questor *q_questor = &q_ptr->questor[questor_idx];

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
	if (!q_questor->talkable) return;


	/* questor interaction may (automatically) acquire the quest */
	/* has the player not yet acquired this quest? */
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	/* nope - test if he's eligible for picking it up!
	   Otherwise, the questor will remain silent for him. */
	if (i == MAX_CONCURRENT_QUESTS) {
		/* do we accept players by questor interaction at all? */
		if (!q_questor->accept_interact) return;
		/* do we accept players to acquire this quest in the current quest stage? */
		if (!q_stage->accepts) return;

		/* if player cannot pick up this quest, questor remains silent */
		if (!(may_acquire = quest_acquire(Ind, q_idx, FALSE))) return;
	}


	/* if we're not here for quest acquirement, just check for quest goals
	   that have been completed and just required delivery back here */
	if (!may_acquire && quest_goal_check(Ind, q_idx, TRUE)) return;

	/* questor interaction qutomatically invokes the quest dialogue, if any */
	q_questor->talk_focus = Ind; /* only this player can actually respond with keywords -- TODO: ensure this happens for non 'individual' quests only */
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
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);
	bool anything;

	if (!repeat) {
		/* pre-scan talk if any line at all passes the flag check */
		anything = FALSE;
		for (k = 0; k < QI_TALK_LINES; k++) {
			if (q_stage->talk[questor_idx][k] &&
			    ((q_stage->talk_flags[questor_idx][k] & quest_get_flags(Ind, q_idx)) == q_stage->talk_flags[questor_idx][k])) {
				anything = TRUE;
				break;
			}
		}
		/* there is NPC talk to display? */
		if (anything) {
			p_ptr->interact_questor_idx = questor_idx;
			msg_print(Ind, "\374 ");
			msg_format(Ind, "\374\377u<\377B%s\377u> speaks to you:", q_ptr->questor[questor_idx].name);
			for (i = 0; i < QI_TALK_LINES; i++) {
				if (!q_stage->talk[questor_idx][i]) break;
				if ((q_stage->talk_flags[questor_idx][k] & quest_get_flags(Ind, q_idx)) != q_stage->talk_flags[questor_idx][k]) continue;
				msg_format(Ind, "\374\377U%s", q_stage->talk[questor_idx][i]);
			}
			//msg_print(Ind, "\374 ");
		}
	}

	/* No keyword-interaction possible if we haven't acquired the quest yet. */
	if (interact_acquire) return;

	/* If there are any keywords in this stage, prompt the player for a reply.
	   If the questor is focussed on one player, only he can give a reply,
	   otherwise the first player to reply can advance the stage. */
	if (!q_ptr->individual && q_ptr->questor[questor_idx].talk_focus && q_ptr->questor[questor_idx].talk_focus != Ind) return;

	p_ptr->interact_questor_idx = questor_idx;
	/* check if there's at least 1 keyword available in our stage/from our questor */
	anything = FALSE;
	for (i = 0; i < q_ptr->keywords; i++)
		if (q_ptr->keyword[i].stage_ok[stage] &&
		    q_ptr->keyword[i].questor_ok[questor_idx]) {
			anything = TRUE;
			break;
		}
	if (anything) { /* at least 1 keyword available? */
		/* hack: if 1st keyword is empty string "", display a "more" prompt */
		if (q_ptr->keyword[i].keyword[0] == 0)
			Send_request_str(Ind, RID_QUEST + q_idx, "Your reply (or ENTER for more)> ", "");
		else {
			/* hack: if 1st keyword is "y" just give a yes/no choice instead of an input prompt?
			   we assume that 2nd keyword is a "n" then. */
			if (q_ptr->keyword[i].keyword[0] == 'y' &&
			    q_ptr->keyword[i].keyword[1] == 0)
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
	int i, j, k, stage = quest_get_stage(Ind, q_idx), questor_idx = p_ptr->interact_questor_idx;
	char *c;
	qi_keyword *q_key;
	qi_kwreply *q_kwr;

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
	for (i = 0; i < q_ptr->keywords; i++) {
		q_key = &q_ptr->keyword[i];

		if (!q_key->stage_ok[stage] || /* no more keywords? */
		    !q_key->questor_ok[questor_idx]) continue;

		/* check if required flags match to enable this keyword */
		if ((q_key->flags & quest_get_flags(Ind, q_idx)) != q_key->flags) continue;

		if (strcmp(q_key->keyword, str)) continue; /* not matching? */

		/* reply? */
		for (j = 0; j < q_ptr->kwreplies; j++) {
			q_kwr = &q_ptr->kwreply[j];

			if (!q_kwr->stage_ok[stage] || /* no more keywords? */
			    !q_kwr->questor_ok[questor_idx]) continue;

			/* check if required flags match to enable this keyword */
			if ((q_kwr->flags & quest_get_flags(Ind, q_idx)) != q_kwr->flags) continue;

			for (k = 0; k < QI_KEYWORDS_PER_REPLY; k++) {
				if (q_kwr->keyword_idx[k] == i) {
					msg_print(Ind, "\374 ");
					msg_format(Ind, "\374\377u<\377B%s\377u> speaks to you:", q_ptr->questor[questor_idx].name);
					/* we can re-use j here ;) */
					for (j = 0; j < q_kwr->lines; j++) {
						if ((q_kwr->replyflags[j] & quest_get_flags(Ind, q_idx)) != q_kwr->replyflags[j]) continue;
						msg_format(Ind, "\374\377U%s", q_kwr->reply[j]);
					}
					//msg_print(Ind, "\374 ");

					j = q_ptr->kwreplies; //hack->break! -- 1 reply found is enough
					break;
				}
			}
		}

		/* flags change? */
		quest_set_flags(Ind, q_idx, q_key->setflags, q_key->clearflags);

		/* stage change? */
		if (q_key->stage != -1) {
			quest_set_stage(Ind, q_idx, q_key->stage, FALSE);
			return;
		}
		/* Instead of return'ing, re-prompt for dialogue
		   if we only changed flags or simply got a keyword-reply. */
		str[0] = 0; /* don't give the 'nothing to say' line, our keyword already matched */
		break;
	}
	/* it was nice small-talking to you, dude */
#if 1
	/* if keyword wasn't recognised, repeat input prompt instead of just 'dropping' the convo */
	quest_dialogue(Ind, q_idx, questor_idx, TRUE, FALSE);
	/* don't give 'wassup?' style msg if we just hit RETURN.. silyl */
	if (str[0]) {
		msg_print(Ind, "\374 ");
		msg_format(Ind, "\374\377u<\377B%s\377u> has nothing to say about that.", q_ptr->questor[questor_idx].name);
		//msg_print(Ind, "\374 ");
	}
#endif
	return;
}

/* Test kill quest goal criteria vs an actually killed monster, for a match.
   Main criteria (r_idx vs char+attr+level) are OR'ed.
   (Unlike for retrieve-object matches where they are actually AND'ed.) */
static bool quest_goal_matches_kill(int q_idx, int stage, int goal, monster_type *m_ptr) {
	int i;
	monster_race *r_ptr = race_inf(m_ptr);
	qi_kill *q_kill = quest_qi_stage(q_idx, stage)->goal[goal].kill;

	/* check for race index */
	for (i = 0; i < 10; i++) {
		/* no monster specified? */
		if (q_kill->ridx[i] == 0) continue;
		/* accept any monster? */
		if (q_kill->ridx[i] == -1) return TRUE;
		/* specified an r_idx */
		if (q_kill->ridx[i] == m_ptr->r_idx) return TRUE;
	}

	/* check for char/attr/level combination - treated in pairs (AND'ed) over the index */
	for (i = 0; i < 5; i++) {
		/* no char specified? */
		if (q_kill->rchar[i] == 255) continue;
		 /* accept any char? */
		if (q_kill->rchar[i] != 254 &&
		    /* specified a char? */
		    q_kill->rchar[i] != r_ptr->d_char) continue;

		/* no attr specified? */
		if (q_kill->rattr[i] == 255) continue;
		 /* accept any attr? */
		if (q_kill->rattr[i] != 254 &&
		    /* specified an attr? */
		    q_kill->rattr[i] != r_ptr->d_attr) continue;

		/* min/max level check -- note that we use m-level, not r-level :-o */
		if ((!q_kill->rlevmin || q_kill->rlevmin <= m_ptr->level) &&
		    (!q_kill->rlevmax || q_kill->rlevmax >= m_ptr->level))
			return TRUE;
	}

	/* pft */
	return FALSE;
}

/* Test retrieve-item quest goal criteria vs an actually retrieved object, for a match.
   (Note that the for-blocks are AND'ed unlike for kill-matches where they are OR'ed.) */
static bool quest_goal_matches_object(int q_idx, int stage, int goal, object_type *o_ptr) {
	int i;
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	byte attr;
	qi_retrieve *q_ret = quest_qi_stage(q_idx, stage)->goal[goal].retrieve;

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
		if (q_ret->otval[i] == 0) continue;
		/* accept any tval? */
		if (q_ret->otval[i] != -1 &&
		    /* specified a tval */
		    q_ret->otval[i] != o_ptr->tval) continue;;

		/* no sval specified? */
		if (q_ret->osval[i] == 0) continue;
		/* accept any sval? */
		if (q_ret->osval[i] != -1 &&
		    /* specified a sval */
		    q_ret->osval[i] != o_ptr->sval) continue;;

		break;
	}
	if (i == 10) return FALSE;

	/* check for pval/bpval/attr/name1/name2/name2b
	   note: let's treat pval/bpval as minimum values instead of exact values for now. */
	for (i = 0; i < 5; i++) {
		/* no pval specified? */
		if (q_ret->opval[i] == 9999) continue;
		/* accept any pval? */
		if (q_ret->opval[i] != -9999 &&
		    /* specified a pval? */
		    q_ret->opval[i] < o_ptr->pval) continue;

		/* no bpval specified? */
		if (q_ret->obpval[i] == 9999) continue;
		/* accept any bpval? */
		if (q_ret->obpval[i] != -9999 &&
		    /* specified a bpval? */
		    q_ret->obpval[i] < o_ptr->bpval) continue;

		/* no attr specified? */
		if (q_ret->oattr[i] == 255) continue;
		/* accept any attr? */
		if (q_ret->oattr[i] != -254 &&
		    /* specified a attr? */
		    q_ret->oattr[i] != attr) continue;

		/* no name1 specified? */
		if (q_ret->oname1[i] == -3) continue;
		 /* accept any name1? */
		if (q_ret->oname1[i] != -1 &&
		 /* accept any name1, but MUST be != 0? */
		    (q_ret->oname1[i] != -2 || !o_ptr->name1) &&
		    /* specified a name1? */
		    q_ret->oname1[i] != o_ptr->name1) continue;

		/* no name2 specified? */
		if (q_ret->oname2[i] == -3) continue;
		 /* accept any name2? */
		if (q_ret->oname2[i] != -1 &&
		 /* accept any name2, but MUST be != 0? */
		    (q_ret->oname2[i] != -2 || !o_ptr->name2) &&
		    /* specified a name2? */
		    q_ret->oname2[i] != o_ptr->name2) continue;

		/* no name2b specified? */
		if (q_ret->oname2b[i] == -3) continue;
		 /* accept any name2b? */
		if (q_ret->oname2b[i] != -1 &&
		 /* accept any name2b, but MUST be != 0? */
		    (q_ret->oname2b[i] != -2 || !o_ptr->name2b) &&
		    /* specified a name2b? */
		    q_ret->oname2b[i] != o_ptr->name2b) continue;
	}
	if (i == 5) return FALSE;

	/* finally, test against minimum value */
	if (q_ret->ovalue <= object_value_real(0, o_ptr)) return TRUE;

	/* pft */
	return FALSE;
}

/* Check if player completed a kill or item-retrieve goal.
   The monster slain or item retrieved must therefore be given to this function for examination.
   Note: The mechanics for retrieving quest items at a specific target position are a bit tricky:
         If n items have to be retrieved, each one has to be a different item that gets picked at
         the target pos. When an item is lost from the player's inventory again however, it mayb be
         retrieved anywhere, ignoring the target location specification. This requires the quest
         items to be marked when they get picked up at the target location, to free those marked
         ones from same target loc restrictions for re-pickup. */
static void quest_check_goal_kr(int Ind, int q_idx, int py_q_idx, monster_type *m_ptr, object_type *o_ptr) {
	quest_info *q_ptr = &q_info[q_idx];;
	player_type *p_ptr = Players[Ind];
	int j, k, stage;
	bool nisi;
	qi_stage *q_stage;
	qi_goal *q_goal;

	/* quest is deactivated? */
	if (!q_ptr->active) return;

	stage = quest_get_stage(Ind, q_idx);
	q_stage = quest_qi_stage(q_idx, stage);

	/* For handling Z-lines: flags changed depending on goals completed:
	   pre-check if we have any pending deliver goal in this stage.
	   If so then we can only set the quest goal 'nisi' (provisorily),
	   and hence flags won't get changed yet until it is eventually resolved
	   when we turn in the delivery. */
	nisi = FALSE;
	for (j = 0; j < q_stage->goals; j++)
		if (q_stage->goal[j].deliver) {
			nisi = TRUE;
			break;
		}

#if QDEBUG > 2
	s_printf(" CHECKING k/r-GOAL IN QUEST (%s,%d) stage %d.\n", q_ptr->codename, q_idx, stage);
#endif
	/* check the quest goals, whether any of them wants a target to this location */
	for (j = 0; j < q_stage->goals; j++) {
		q_goal = &q_stage->goal[j];

		/* no k/r goal? */
		if (!q_goal->kill && !q_goal->retrieve) continue;
#if QDEBUG > 2
		s_printf(" FOUND kr GOAL %d (k=%d,r=%d).\n", j, q_goal->kill, q_goal->retrieve);
#endif

		/* location-restricted?
		   Exempt already retrieved items that were just lost temporarily on the way! */
		if (q_goal->target_pos &&
		    !(o_ptr->quest == q_idx + 1 && o_ptr->quest_stage == stage)) {
			/* extend target terrain over a wide patch? */
			if (q_goal->target_terrain_patch) {
				/* different z-coordinate = instant fail */
				if (p_ptr->wpos.wz != q_goal->target_wpos.wz) continue;
				/* are we within range and have same terrain type? */
				if (distance(p_ptr->wpos.wx, p_ptr->wpos.wy,
				    q_goal->target_wpos.wx, q_goal->target_wpos.wy) > QI_TERRAIN_PATCH_RADIUS ||
				    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type !=
				    wild_info[q_goal->target_wpos.wy][q_goal->target_wpos.wx].type)
					continue;
			}
			/* just check a single, specific wpos? */
			else if (!inarea(&q_goal->target_wpos, &p_ptr->wpos)) continue;

			/* check for exact x,y location? */
			if (q_goal->target_pos_x != -1 &&
			    q_goal->target_pos_x != p_ptr->px &&
			    q_goal->target_pos_y != p_ptr->py)
				continue;
		}
#if QDEBUG > 2
		s_printf(" PASSED/NO LOCATION CHECK.\n");
#endif

	///TODO: implement for global quests too!
		/* check for kill goal here */
		if (m_ptr && q_goal->kill) {
			if (!quest_goal_matches_kill(q_idx, stage, j, m_ptr)) continue;
			/* decrease the player's kill counter, if we got all, goal is completed! */
			p_ptr->quest_kill_number[py_q_idx][j]--;
			if (p_ptr->quest_kill_number[py_q_idx][j]) continue; /* not yet */

			/* we have completed a target-to-xy goal! */
			quest_set_goal(Ind, q_idx, j, nisi);
			continue;
		}

	///TODO: implement for global quests too!
		/* check for retrieve-item goal here */
		if (o_ptr && q_goal->retrieve) {
			if (!quest_goal_matches_object(q_idx, stage, j, o_ptr)) continue;

			/* discard old items from another quest or quest stage that just look similar!
			   Those are 'tainted' and cannot be reused. */
			if (o_ptr->quest != q_idx + 1 || o_ptr->quest_stage != stage) continue;

			/* mark the item as quest item, so we know we found it at the designated target loc (if any) */
			o_ptr->quest = q_idx + 1;
			o_ptr->quest_stage = stage;

			/* decrease the player's retrieve counter, if we got all, goal is completed! */
			p_ptr->quest_retrieve_number[py_q_idx][j] -= o_ptr->number;
			if (p_ptr->quest_retrieve_number[py_q_idx][j] > 0) continue; /* not yet */

			/* we have completed a target-to-xy goal! */
			quest_set_goal(Ind, q_idx, j, nisi);

			/* if we don't have to deliver in this stage,
			   we can just remove all the quest items right away now,
			   since they've fulfilled their purpose of setting the quest goal. */
			for (k = 0; k < q_stage->goals; k++)
				if (q_stage->goal[k].deliver) break;
#if QDEBUG > 2
			s_printf(" REMOVE RETRIEVED ITEMS.\n");
#endif
			if (k == q_stage->goals) {
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
/* Small intermediate function to reduce workload.. (1. for k-goals) */
void quest_check_goal_k(int Ind, monster_type *m_ptr) {
	player_type *p_ptr = Players[Ind];
	int i;

#if QDEBUG > 3
	s_printf("QUEST_CHECK_GOAL_r: by %d,%s\n", Ind, p_ptr->name);
#endif
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++) {
#if 0
		/* player actually pursuing a quest? -- paranoia (quest_kill should be FALSE then) */
		if (p_ptr->quest_idx[i] == -1) continue;
#endif
		/* reduce workload */
		if (!p_ptr->quest_kill[i]) continue;

		quest_check_goal_kr(Ind, p_ptr->quest_idx[i], i, m_ptr, NULL);
	}
}
/* Small intermediate function to reduce workload.. (2. for r-goals) */
void quest_check_goal_r(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];
	int i;

#if QDEBUG > 3
	s_printf("QUEST_CHECK_GOAL_k: by %d,%s\n", Ind, p_ptr->name);
#endif
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++) {
#if 0
		/* player actually pursuing a quest? -- paranoia (quest_retrieve should be FALSE then) */
		if (p_ptr->quest_idx[i] == -1) continue;
#endif
		/* reduce workload */
		if (!p_ptr->quest_retrieve[i]) continue;

		quest_check_goal_kr(Ind, p_ptr->quest_idx[i], i, NULL, o_ptr);
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
	qi_stage *q_stage;

#if QDEBUG > 3
	s_printf("QUEST_CHECK_UNGOAL_r: by %d,%s\n", Ind, p_ptr->name);
#endif
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++) {
#if 0
		/* player actually pursuing a quest? -- paranoia (quest_retrieve should be FALSE then) */
		if (p_ptr->quest_idx[i] == -1) continue;
#endif
		/* reduce workload */
		if (!p_ptr->quest_retrieve[i]) continue;

		q_idx = p_ptr->quest_idx[i];
		q_ptr = &q_info[q_idx];

		/* quest is deactivated? */
		if (!q_ptr->active) continue;

		stage = quest_get_stage(Ind, q_idx);
		q_stage = quest_qi_stage(q_idx, stage);
#if QDEBUG > 2
		s_printf(" CHECKING FOR QUEST (%s,%d) stage %d.\n", q_ptr->codename, q_idx, stage);
#endif

		/* check the quest goals, whether any of them wants a retrieval */
		for (j = 0; j < q_stage->goals; j++) {
			/* no r goal? */
			if (!q_stage->goal[j].retrieve) continue;

			/* phew, item has nothing to do with this quest goal? */
			if (!quest_goal_matches_object(q_idx, stage, j, o_ptr)) continue;
#if QDEBUG > 2
			s_printf(" FOUND r GOAL %d.\n", j);
#endif

			/* increase the player's retrieve counter again */
			p_ptr->quest_retrieve_number[i][j] += num;
			if (p_ptr->quest_retrieve_number[i][j] <= 0) continue; /* still cool */

			/* we have un-completed a target-to-xy goal, ow */
			quest_unset_goal(Ind, q_idx, j);
			continue;
		}
	}
}

/* When we're called then we know that this player just arrived at a fitting
   wpos for a deliver quest. We can now complete the corresponding quest goal accordingly. */
static void quest_handle_goal_deliver_wpos(int Ind, int py_q_idx, int q_idx, int stage) {
	player_type *p_ptr = Players[Ind];
	quest_info *q_ptr = &q_info[q_idx];
	int j, k;
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);
	qi_goal *q_goal;
	qi_deliver *q_del;

#if QDEBUG > 2
	s_printf("QUEST_HANDLE_GOAL_DELIVER_WPOS: by %d,%s - quest (%s,%d) stage %d\n",
	    Ind, p_ptr->name, q_ptr->codename, q_idx, stage);
#endif

	/* pre-check if we have completed all kill/retrieve goals of this stage,
	   because we can only complete any deliver goal if we have fetched all
	   the stuff (bodies, or kill count rather, and objects) that we ought to
	   'deliver', duh */
	for (j = 0; j < q_stage->goals; j++) {
		q_goal = &q_stage->goal[j];
#if QDEBUG > 2
		if (q_goal->kill || q_goal->retrieve) {
			s_printf(" --FOUND GOAL %d (k%d,m%d)..", j, q_goal->kill, q_goal->retrieve);
			if (!quest_get_goal(Ind, q_idx, j, FALSE)) {
				s_printf("MISSING.\n");
				break;
			} else s_printf("DONE.\n");

		}
#else
		if ((q_goal->kill || q_goal->retrieve)
		    && !quest_get_goal(Ind, q_idx, j, FALSE))
			break;
#endif
	}
	if (j != QI_GOALS) {
#if QDEBUG > 2
		s_printf(" MISSING kr GOAL\n");
#endif
		return;
	}

	/* check the quest goals, whether any of them wants a delivery to this location */
	for (j = 0; j < q_stage->goals; j++) {
		q_goal = &q_stage->goal[j];

		if (!q_goal->deliver) continue;
		q_del = q_goal->deliver;

		/* handle only non-specific x,y goals here */
		if (q_del->pos_x != -1) continue;
#if QDEBUG > 2
		s_printf(" FOUND deliver_wpos GOAL %d.\n", j);
#endif

		/* extend target terrain over a wide patch? */
		if (q_del->terrain_patch) {
			/* different z-coordinate = instant fail */
			if (p_ptr->wpos.wz != q_del->wpos.wz) continue;
			/* are we within range and have same terrain type? */
			if (distance(p_ptr->wpos.wx, p_ptr->wpos.wy,
			    q_del->wpos.wx, q_del->wpos.wy) > QI_TERRAIN_PATCH_RADIUS ||
			    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type !=
			    wild_info[q_del->wpos.wy][q_del->wpos.wx].type)
				continue;
		}
		/* just check a single, specific wpos? */
		else if (!inarea(&q_del->wpos, &p_ptr->wpos)) continue;
#if QDEBUG > 2
		s_printf(" PASSED LOCATION CHECK.\n");
#endif

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
		for (k = 0; k < q_stage->goals; k++)
			if (quest_get_goal(Ind, q_idx, k, TRUE)) {
				quest_set_goal(Ind, q_idx, k, FALSE);
				quest_goal_changes_flags(Ind, q_idx, stage, k);
			}

		/* we have completed a delivery-to-wpos goal! */
		quest_set_goal(Ind, q_idx, j, FALSE);
	}
}
/* Check if player completed a deliver goal to a wpos and specific x,y.
   Hack for now: added 'py_q_idx' to directly specify the player's quest index,
                 since we already know it. No need to do the same work multiple times.
                 In the code this is marked by '//++' */
static void quest_check_goal_deliver_xy(int Ind, int q_idx, int py_q_idx) {
	player_type *p_ptr = Players[Ind];
	int j, k, stage;
	quest_info *q_ptr = &q_info[q_idx];;
	qi_stage *q_stage;
	qi_goal *q_goal;
	qi_deliver *q_del;

#if QDEBUG > 3
	s_printf("QUEST_CHECK_GOAL_DELIVER_XY: by %d,%s - quest (%s,%d) stage %d\n",
	    Ind, p_ptr->name, q_ptr->codename, q_idx, quest_get_stage(Ind, q_idx));
#endif

	/* quest is deactivated? */
	if (!q_ptr->active) return;

	stage = quest_get_stage(Ind, q_idx);
	q_stage = quest_qi_stage(q_idx, stage);

	/* pre-check if we have completed all kill/retrieve goals of this stage,
	   because we can only complete any deliver goal if we have fetched all
	   the stuff (bodies, or kill count rather, and objects) that we ought to
	   'deliver', duh */
	for (j = 0; j < q_stage->goals; j++) {
		q_goal = &q_stage->goal[j];
#if QDEBUG > 3
		if (q_goal->kill || q_goal->retrieve) {
			s_printf(" --FOUND GOAL %d (k%d,m%d)..", j, q_ptr->kill[j], q_ptr->retrieve[j]);
			if (!quest_get_goal(Ind, q_idx, j, FALSE)) {
				s_printf("MISSING.\n");
				break;
			} else s_printf("DONE.\n");

		}
#else
		if ((q_goal->kill || q_goal->retrieve)
		    && !quest_get_goal(Ind, q_idx, j, FALSE))
			break;
#endif
	}
	if (j != q_stage->goals) {
#if QDEBUG > 3
		s_printf(" MISSING kr GOAL\n");
#endif
		return;
	}

	/* check the quest goals, whether any of them wants a delivery to this location */
	for (j = 0; j < q_stage->goals; j++) {
		q_goal = &q_stage->goal[j];
		if (!q_goal->deliver) continue;
		q_del = q_goal->deliver;

		/* handle only specific x,y goals here */
		if (q_goal->deliver->pos_x == -1) continue;
#if QDEBUG > 3
		s_printf(" FOUND deliver_xy GOAL %d.\n", j);
#endif

		/* extend target terrain over a wide patch? */
		if (q_del->terrain_patch) {
			/* different z-coordinate = instant fail */
			if (p_ptr->wpos.wz != q_del->wpos.wz) continue;
			/* are we within range and have same terrain type? */
			if (distance(p_ptr->wpos.wx, p_ptr->wpos.wy,
			    q_del->wpos.wx, q_del->wpos.wy) > QI_TERRAIN_PATCH_RADIUS ||
			    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type !=
			    wild_info[q_del->wpos.wy][q_del->wpos.wx].type)
				continue;
		}
		/* just check a single, specific wpos? */
		else if (!inarea(&q_del->wpos, &p_ptr->wpos)) continue;

		/* check for exact x,y location */
		if (q_del->pos_x != p_ptr->px ||
		    q_del->pos_y != p_ptr->py)
			continue;
#if QDEBUG > 3
		s_printf(" PASSED LOCATION CHECK.\n");
#endif

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
		for (k = 0; k < q_stage->goals; k++)
			if (quest_get_goal(Ind, q_idx, k, TRUE)) {
				quest_set_goal(Ind, q_idx, k, FALSE);
				quest_goal_changes_flags(Ind, q_idx, stage, k);
			}

		/* we have completed a delivery-to-xy goal! */
		quest_set_goal(Ind, q_idx, j, FALSE);
	}
}
/* Small intermediate function to reduce workload.. (3. for deliver-goals) */
void quest_check_goal_deliver(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i;

	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++) {
#if 0
		/* player actually pursuing a quest? -- paranoia (quest_retrieve should be FALSE then) */
		if (p_ptr->quest_idx[i] == -1) continue;
#endif
		/* reduce workload */
		if (!p_ptr->quest_deliver_xy[i]) continue;

		quest_check_goal_deliver_xy(Ind, p_ptr->quest_idx[i], i);
	}
}

/* check goals for their current completion state, for entering the next stage if ok */
static int quest_goal_check_stage(int pInd, int q_idx) {
	int i, j, stage = quest_get_stage(pInd, q_idx);
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);

	/* scan through all possible follow-up stages */
	for (j = 0; j < QI_FOLLOWUP_STAGES; j++) {
		/* no follow-up stage? */
		if (q_stage->next_stage_from_goals[j] == -1) continue;

		/* scan through all goals required to be fulfilled to enter this stage */
		for (i = 0; i < QI_STAGE_GOALS; i++) {
			if (q_stage->goals_for_stage[j][i] == -1) continue;

			/* If even one goal is missing, we cannot advance. */
			if (!quest_get_goal(pInd, q_idx, q_stage->goals_for_stage[j][i], FALSE)) break;
		}
		/* we may proceed to another stage? */
		if (i == QI_STAGE_GOALS) return q_stage->next_stage_from_goals[j];
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
		s_printf(" QUEST_REWARD_OBJECT: not individual, but no pInd either.\n");
		return;//catch paranoid bugs--maybe obsolete
	}

	/* global quest (or for some reason missing pInd..<-paranoia)  */
	for (i = 1; i <= NumPlayers; i++) {
		/* is the player on this quest? */
		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

		/* must be around a questor to receive the rewards.
		   TODO: why? can just stand around somewhere, so.. */
		for (j = 0; j < q_ptr->questors; j++)
			if (inarea(&Players[i]->wpos, &q_ptr->questor[j].current_wpos)) break;
		if (j == q_ptr->questors) continue;

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
		s_printf(" QUEST_REWARD_CREATE: not individual, but no pInd either.\n");
		return;//catch paranoid bugs--maybe obsolete
	}

	/* global quest (or for some reason missing pInd..<-paranoia)  */
	for (i = 1; i <= NumPlayers; i++) {
		/* is the player on this quest? */
		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

		/*TODO: why does the player need to be around the questor anyway hmm */
		for (j = 0; j < q_ptr->questors; j++)
			if (inarea(&Players[i]->wpos, &q_ptr->questor[j].current_wpos)) break;
		if (j == q_ptr->questors) continue;

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
		s_printf(" QUEST_REWARD_GOLD: not individual, but no pInd either.\n");
		return;//catch paranoid bugs--maybe obsolete
	}

	/* global quest (or for some reason missing pInd..<-paranoia)  */
	for (i = 1; i <= NumPlayers; i++) {
		/* is the player on this quest? */
		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

		/*TODO: why does the player need to be around the questor anyway hmm */
		for (j = 0; j < q_ptr->questors; j++)
			if (inarea(&Players[i]->wpos, &q_ptr->questor[j].current_wpos)) break;
		if (j == q_ptr->questors) continue;

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
		s_printf(" QUEST_REWARD_EXP: not individual, but no pInd either.\n");
		return;//catch paranoid bugs--maybe obsolete
	}

	/* global quest (or for some reason missing pInd..<-paranoia)  */
	for (i = 1; i <= NumPlayers; i++) {
		/* is the player on this quest? */
		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

		/*TODO: why does the player need to be around the questor anyway hmm */
		for (j = 0; j < q_ptr->questors; j++)
			if (inarea(&Players[i]->wpos, &q_ptr->questor[j].current_wpos)) break;
		if (j == q_ptr->questors) continue;

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
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);
	qi_reward *q_rew;
	qi_questor *q_questor;

	/* TODO: use sensible questor location for generating rewards (apply_magic()) */
	if (!q_ptr->questors) return; //extreme paranoia
	q_questor = &q_ptr->questor[0]; //compiler warning
	for (j = 0; j < q_ptr->questors; j++) {
		q_questor = &q_ptr->questor[j];
		if (q_questor->despawned) continue;
		break;
	} /* just pick the first good questor for now.. w/e */


#if 0 /* we're called when stage 0 starts too, and maybe it's some sort of globally determined goal->reward? */
	if (!pInd || !q_ptr->individual) {
		s_printf(" QUEST_GOAL_CHECK_REWARD: returned! oops\n");
		return; //paranoia?
	}
#endif

	/* scan through all possible follow-up stages */
	for (j = 0; j < q_stage->rewards; j++) {
		q_rew = &q_stage->reward[j];

		/* no reward? */
		if (!q_rew->otval &&
		    !q_rew->oreward &&
		    !q_rew->gold &&
		    !q_rew->exp) //TODO: reward_statuseffect
			continue;

		/* scan through all goals required to be fulfilled to get this reward */
		for (i = 0; i < QI_REWARD_GOALS; i++) {
			if (q_stage->goals_for_reward[j][i] == -1) continue;

			/* if even one goal is missing, we cannot get the reward */
			if (!quest_get_goal(pInd, q_idx, q_stage->goals_for_reward[j][i], FALSE)) break;
		}
		/* we may get rewards? */
		if (i == QI_REWARD_GOALS) {
			/* create and hand over this reward! */
#if QDEBUG > 0
			s_printf("%s QUEST_GOAL_CHECK_REWARD: Rewarded in stage %d of '%s'(%d,%s)\n", showtime(), stage, q_name + q_ptr->name, q_idx, q_ptr->codename);
#endif

			/* specific reward */
			if (q_rew->otval) {
				/* very specific reward */
				if (!q_rew->ogood && !q_rew->ogreat) {
					o_ptr = &forge;
					object_wipe(o_ptr);
					invcopy(o_ptr, lookup_kind(q_rew->otval, q_rew->osval));
					o_ptr->number = 1;
					o_ptr->name1 = q_rew->oname1;
					o_ptr->name2 = q_rew->oname2;
					o_ptr->name2b = q_rew->oname2b;
					if (o_ptr->name1) {
						o_ptr->name1 = ART_RANDART; //hack: disallow true arts!
						o_ptr->name2 = o_ptr->name2b = 0;
					}
					apply_magic(&q_questor->current_wpos, o_ptr, -2, FALSE, FALSE, FALSE, FALSE, resf);
					o_ptr->pval = q_rew->opval;
					o_ptr->bpval = q_rew->obpval;
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
					invcopy(o_ptr, lookup_kind(q_rew->otval, q_rew->osval));
					o_ptr->number = 1;
					apply_magic(&q_questor->current_wpos, o_ptr, -2, q_rew->ogood, q_rew->ogreat, q_rew->ovgreat, FALSE, resf);
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
			else if (q_rew->oreward) {
				quest_reward_create(pInd, q_idx);
				r_obj++;
			}
			/* hand out gold? */
			if (q_rew->gold) {
				quest_reward_gold(pInd, q_idx, q_rew->gold);
				r_gold += q_rew->gold;
			}
			/* provide exp? */
			if (q_rew->exp) {
				quest_reward_exp(pInd, q_idx, q_rew->exp);
				r_exp += q_rew->exp;
			}
			//TODO: s16b reward_statuseffect[QI_STAGES][QI_STAGE_REWARDS];
		}
	}

	/* give one unified message per reward type that was handed out */
	if (pInd && q_ptr->individual) {
		if (r_obj == 1) msg_print(pInd, "You have received an item.");
		else if (r_obj) msg_format(pInd, "You have received %d items.", r_obj);
		if (r_gold) msg_format(pInd, "You have received %d gold piece%s.", r_gold, r_gold == 1 ? "" : "s");
		if (r_exp) msg_format(pInd, "You have received %d experience point%s.", r_exp, r_exp == 1 ? "" : "s");
	} else for (i = 1; i <= NumPlayers; i++) {
		/* is the player on this quest? */
		for (j = 0; j < MAX_CONCURRENT_QUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_CONCURRENT_QUESTS) continue;

		/*TODO: why does the player need to be around the questor anyway hmm */
		for (j = 0; j < q_ptr->questors; j++)
			if (inarea(&Players[i]->wpos, &q_ptr->questor[j].current_wpos)) break;
		if (j == q_ptr->questors) continue;

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
	qi_stage *q_stage;
	qi_goal *q_goal;
	qi_deliver *q_del;

#if QDEBUG > 3
	s_printf("%s CHECK_PLAYER_LOCATION: %d,%s has anyk,anyr=%d,%d.\n", showtime(), Ind, p_ptr->name, p_ptr->quest_any_k, p_ptr->quest_any_r);
#endif

	/* reset local tracking info? only if we don't need to check it anyway: */
	if (!p_ptr->quest_any_k) p_ptr->quest_any_k_within_target = FALSE;
	if (!p_ptr->quest_any_r) p_ptr->quest_any_r_within_target = FALSE;
	p_ptr->quest_any_deliver_xy_within_target = FALSE; /* deliveries are of course intrinsically not 'global' but have a target location^^ */

	/* Quests: Is this wpos a target location or a delivery location for any of our quests? */
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++) {
		/* player actually pursuing a quest? */
		if (p_ptr->quest_idx[i] == -1) continue;

		/* check for target location for kill goals.
		   We only need to do that if we aren't already in 'always checking' mode,
		   because we have quest goals that aren't restricted to a specific target location. */
		if (p_ptr->quest_kill[i] && !p_ptr->quest_any_k && p_ptr->quest_any_k_target) {
			q_idx = p_ptr->quest_idx[i];
			q_ptr = &q_info[q_idx];

			/* quest is deactivated? */
			if (!q_ptr->active) continue;

			stage = quest_get_stage(Ind, q_idx);
			q_stage = quest_qi_stage(q_idx, stage);

			/* check the quest goals, whether any of them wants a target to this location */
			for (j = 0; j < q_stage->goals; j++) {
				q_goal = &q_stage->goal[j];

				if (!q_goal->kill) continue;
				if (!q_goal->target_pos) continue;

				/* extend target terrain over a wide patch? */
				if (q_goal->target_terrain_patch) {
					/* different z-coordinate = instant fail */
					if (p_ptr->wpos.wz != q_goal->target_wpos.wz) continue;
					/* are we within range and have same terrain type? */
					if (distance(p_ptr->wpos.wx, p_ptr->wpos.wy,
					    q_goal->target_wpos.wx, q_goal->target_wpos.wy) <= QI_TERRAIN_PATCH_RADIUS &&
					    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type ==
					    wild_info[q_goal->target_wpos.wy][q_goal->target_wpos.wx].type) {
						p_ptr->quest_any_k_within_target = TRUE;
						break;
					}
				}
				/* just check a single, specific wpos? */
				else if (inarea(&q_goal->target_wpos, &p_ptr->wpos)) {
					p_ptr->quest_any_k_within_target = TRUE;
					break;
				}
			}
		}

		/* check for target location for retrieve goals.
		   We only need to do that if we aren't already in 'always checking' mode,
		   because we have quest goals that aren't restricted to a specific target location. */
		if (p_ptr->quest_retrieve[i] && !p_ptr->quest_any_r && p_ptr->quest_any_r_target) {
			q_idx = p_ptr->quest_idx[i];
			q_ptr = &q_info[q_idx];

			/* quest is deactivated? */
			if (!q_ptr->active) continue;

			stage = quest_get_stage(Ind, q_idx);
			q_stage = quest_qi_stage(q_idx, stage);

			/* check the quest goals, whether any of them wants a target to this location */
			for (j = 0; j < q_stage->goals; j++) {
				q_goal = &q_stage->goal[j];

				if (!q_goal->retrieve) continue;
				if (!q_goal->target_pos) continue;

				/* extend target terrain over a wide patch? */
				if (q_goal->target_terrain_patch) {
					/* different z-coordinate = instant fail */
					if (p_ptr->wpos.wz != q_goal->target_wpos.wz) continue;
					/* are we within range and have same terrain type? */
					if (distance(p_ptr->wpos.wx, p_ptr->wpos.wy,
					    q_goal->target_wpos.wx, q_goal->target_wpos.wy) <= QI_TERRAIN_PATCH_RADIUS &&
					    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type ==
					    wild_info[q_goal->target_wpos.wy][q_goal->target_wpos.wx].type) {
						p_ptr->quest_any_r_within_target = TRUE;
						break;
					}
				}
				/* just check a single, specific wpos? */
				else if (inarea(&q_goal->target_wpos, &p_ptr->wpos)) {
					p_ptr->quest_any_r_within_target = TRUE;
					break;
				}
			}
		}

		/* check for deliver location ('move' goals) */
		if (p_ptr->quest_deliver_pos[i]) {
			q_idx = p_ptr->quest_idx[i];
			q_ptr = &q_info[q_idx];

			/* quest is deactivated? */
			if (!q_ptr->active) continue;

			stage = quest_get_stage(Ind, q_idx);
			q_stage = quest_qi_stage(q_idx, stage);

			/* first clear old wpos' delivery state */
			p_ptr->quest_deliver_xy[i] = FALSE;

			/* check the quest goals, whether any of them wants a delivery to this location */
			for (j = 0; j < q_stage->goals; j++) {
				q_goal = &q_stage->goal[j];
				if (!q_goal->deliver) continue;
				q_del = q_goal->deliver;

				/* extend target terrain over a wide patch? */
				if (q_del->terrain_patch) {
					/* different z-coordinate = instant fail */
					if (p_ptr->wpos.wz != q_del->wpos.wz) continue;
					/* are we within range and have same terrain type? */
					if (distance(p_ptr->wpos.wx, p_ptr->wpos.wy,
					    q_del->wpos.wx, q_del->wpos.wy) <= QI_TERRAIN_PATCH_RADIUS &&
					    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type ==
					    wild_info[q_del->wpos.wy][q_del->wpos.wx].type) {
						/* imprint new temporary destination location information */
						/* specific x,y loc? */
						if (q_del->pos_x != -1) {
							p_ptr->quest_deliver_xy[i] = TRUE;
							p_ptr->quest_any_deliver_xy_within_target = TRUE;
							/* and check right away if we're already on the correct x,y location */
							quest_check_goal_deliver_xy(Ind, q_idx, i);
						} else {
							/* we are at the requested deliver location (for at least this quest)! (wpos) */
							quest_handle_goal_deliver_wpos(Ind, i, q_idx, stage);
						}
						break;
					}
				}
				/* just check a single, specific wpos? */
				else if (inarea(&q_del->wpos, &p_ptr->wpos)) {
					/* imprint new temporary destination location information */
					/* specific x,y loc? */
					if (q_del->pos_x != -1) {
						p_ptr->quest_deliver_xy[i] = TRUE;
						p_ptr->quest_any_deliver_xy_within_target = TRUE;
						/* and check right away if we're already on the correct x,y location */
						quest_check_goal_deliver_xy(Ind, q_idx, i);
					} else {
						/* we are at the requested deliver location (for at least this quest)! (wpos) */
						quest_handle_goal_deliver_wpos(Ind, i, q_idx, stage);
					}
					break;
				}
			}
		}
	}
}

/* Hack: if a quest was disabled in q_info, this will have set the
   'disabled_on_load' flag of that quest, which tells us that we have to handle
   deleting its remaining questor(s) here, before the server finally starts up. */
void quest_handle_disabled_on_startup() {
	int i, j, k;
	quest_info *q_ptr;
	bool questor;

	for (i = 0; i < MAX_Q_IDX; i++) {
		q_ptr = &q_info[i];
		if (!q_ptr->defined) continue;
		if (!q_ptr->disabled_on_load) continue;

		s_printf("QUEST_DISABLED_ON_LOAD: Deleting %d questor(s) from quest %d\n", q_ptr->questors, i);
		for (j = 0; j < q_ptr->questors; j++) {
			questor = m_list[q_ptr->questor[j].m_idx].questor;
			k = m_list[q_ptr->questor[j].m_idx].quest;

			s_printf(" m_idx %d of q_idx %d (questor=%d)\n", q_ptr->questor[j].m_idx, k, questor);

			if (k == i && questor) {
				s_printf("..ok.\n");
				delete_monster_idx(q_ptr->questor[j].m_idx, TRUE);
			} else s_printf("..failed: Questor does not exist.\n");
		}
	}
}


/* ---------- Helper functions for initialisation of q_info[] from q_info.txt in init.c ---------- */

qi_questor *init_quest_questor(int q_idx, int num) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_questor *p;

	/* we already have this existing one */
	if (q_ptr->questors > num) return &q_ptr->questor[num];

	/* allocate all missing instances up to the requested index */
	p = (qi_questor*)realloc(q_ptr->questor, sizeof(qi_questor) * (num + 1));
	if (!p) {
		s_printf("init_quest_questor() Couldn't allocate memory!\n");
		exit(0);
	}
	/* initialise the ADDED memory */
	//memset(p + q_ptr->questors * sizeof(qi_questor), 0, (num + 1 - q_ptr->questors) * sizeof(qi_questor));
	memset(&p[q_ptr->questors], 0, (num + 1 - q_ptr->questors) * sizeof(qi_questor));

	/* attach it to its parent structure */
	q_ptr->questor = p;
	q_ptr->questors = num + 1;

	/* initialise with correct defaults (paranoia) */
	p = &q_ptr->questor[q_ptr->questors - 1];
	p->accept_los = FALSE;
	p->accept_interact = TRUE;
	p->talkable = TRUE;
	p->despawned = FALSE;
	p->invincible = TRUE;
	p->tpref = NULL;

	/* done, return the new, requested one */
	return &q_ptr->questor[num];
}

qi_stage *init_quest_stage(int q_idx, int num) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_stage *p;
	int i, j;

	/* pointless, but w/e */
	if (num == -1) {
#if QDEBUG > 1
		s_printf("QUESTS: referred to stage -1.\n");
#endif
		return NULL;
	}

	/* we already have this existing one */
	if (q_ptr->stage_idx[num] != -1) return &q_ptr->stage[q_ptr->stage_idx[num]];

	/* allocate a new one */
	p = (qi_stage*)realloc(q_ptr->stage, sizeof(qi_stage) * (q_ptr->stages + 1));
	if (!p) {
		s_printf("init_quest_stage() Couldn't allocate memory!\n");
		exit(0);
	}
	/* initialise the ADDED memory */
	//memset(p + q_ptr->stages * sizeof(qi_stage), 0, sizeof(qi_stage));
	memset(&p[q_ptr->stages], 0, sizeof(qi_stage));

	/* attach it to its parent structure */
	q_ptr->stage = p;
	q_ptr->stage_idx[num] = q_ptr->stages;
	q_ptr->stages++;

	/* initialise with correct defaults */
	p = &q_ptr->stage[q_ptr->stages - 1];
	for (i = 0; i < QI_FOLLOWUP_STAGES; i++) {
		p->next_stage_from_goals[i] = -1; /* no next stages set, end quest by default if nothing specified */
		for (j = 0; j < QI_STAGE_GOALS; j++)
			p->goals_for_stage[i][j] = -1; /* no next stages set, end quest by default if nothing specified */
	}
	for (i = 0; i < QI_STAGE_REWARDS; i++) {
		for (j = 0; j < QI_REWARD_GOALS; j++)
			p->goals_for_reward[i][j] = -1; /* no next stages set, end quest by default if nothing specified */
	}
	p->activate_quest = -1;
	p->change_stage = -1;
	p->timed_ingame_abs = -1;
	p->timed_real = 0;
	if (num == 0) p->accepts = TRUE;
	p->reward = NULL;
	p->goal = NULL;

	/* done, return the new one */
	return &q_ptr->stage[q_ptr->stages - 1];
}

qi_keyword *init_quest_keyword(int q_idx, int num) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_keyword *p;

	/* we already have this existing one */
	if (q_ptr->keywords > num) return &q_ptr->keyword[num];

	/* allocate all missing instances up to the requested index */
	p = (qi_keyword*)realloc(q_ptr->keyword, sizeof(qi_keyword) * (num + 1));
	if (!p) {
		s_printf("init_quest_keyword() Couldn't allocate memory!\n");
		exit(0);
	}
	/* initialise the ADDED memory */
	//memset(p + q_ptr->keywords * sizeof(qi_keyword), 0, (num + 1 - q_ptr->keywords) * sizeof(qi_keyword));
	memset(&p[q_ptr->keywords], 0, (num + 1 - q_ptr->keywords) * sizeof(qi_keyword));

	/* attach it to its parent structure */
	q_ptr->keyword = p;
	q_ptr->keywords = num + 1;

	/* done, return the new, requested one */
	return &q_ptr->keyword[num];
}

qi_kwreply *init_quest_kwreply(int q_idx, int num) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_kwreply *p;

	/* we already have this existing one */
	if (q_ptr->kwreplies > num) return &q_ptr->kwreply[num];

	/* allocate all missing instances up to the requested index */
	p = (qi_kwreply*)realloc(q_ptr->kwreply, sizeof(qi_kwreply) * (num + 1));
	if (!p) {
		s_printf("init_quest_kwreply() Couldn't allocate memory!\n");
		exit(0);
	}
	/* initialise the ADDED memory */
	//memset(p + q_ptr->keywords * sizeof(qi_keyword), 0, (num + 1 - q_ptr->keywords) * sizeof(qi_keyword));
	memset(&p[q_ptr->kwreplies], 0, (num + 1 - q_ptr->kwreplies) * sizeof(qi_kwreply));

	/* attach it to its parent structure */
	q_ptr->kwreply = p;
	q_ptr->kwreplies = num + 1;

	/* done, return the new, requested one */
	return &q_ptr->kwreply[num];
}

qi_kill *init_quest_kill(int q_idx, int stage, int goal) {
	qi_goal *q_goal = init_quest_goal(q_idx, stage, goal);
	qi_kill *p;
	int i;

	/* we already have this existing one */
	if (q_goal->kill) return q_goal->kill;

	/* allocate a new one */
	p = (qi_kill*)malloc(sizeof(qi_kill));
	if (!p) {
		s_printf("init_quest_kill() Couldn't allocate memory!\n");
		exit(0);
	}

	/* attach it to its parent structure */
	q_goal->kill = p;

	/* initialise with defaults */
	for (i = 0; i < 5; i++) {
		p->rchar[i] = 255;
		p->rattr[i] = 255;
	}

	/* done, return the new, requested one */
	return p;
}

qi_retrieve *init_quest_retrieve(int q_idx, int stage, int goal) {
	qi_goal *q_goal = init_quest_goal(q_idx, stage, goal);
	qi_retrieve *p;
	int i;

	/* we already have this existing one */
	if (q_goal->retrieve) return q_goal->retrieve;

	/* allocate a new one */
	p = (qi_retrieve*)malloc(sizeof(qi_retrieve));
	if (!p) {
		s_printf("init_quest_retrieve() Couldn't allocate memory!\n");
		exit(0);
	}

	/* attach it to its parent structure */
	q_goal->retrieve = p;

	/* initialise with defaults */
	for (i = 0; i < 5; i++) {
		p->opval[i] = 9999;
		p->obpval[i] = 9999;
		p->oattr[i] = 255;
		p->oname1[i] = -3;
		p->oname2[i] = -3;
		p->oname2b[i] = -3;
	}

	/* done, return the new, requested one */
	return p;
}

qi_deliver *init_quest_deliver(int q_idx, int stage, int goal) {
	qi_goal *q_goal = init_quest_goal(q_idx, stage, goal);
	qi_deliver *p;

	/* we already have this existing one */
	if (q_goal->deliver) return q_goal->deliver;

	/* allocate a new one */
	p = (qi_deliver*)malloc(sizeof(qi_deliver));
	if (!p) {
		s_printf("init_quest_deliver() Couldn't allocate memory!\n");
		exit(0);
	}

	/* attach it to its parent structure */
	q_goal->deliver = p;

	/* done, return the new, requested one */
	return p;
}

qi_goal *init_quest_goal(int q_idx, int stage, int num) {
	qi_stage *q_stage = init_quest_stage(q_idx, stage);
	qi_goal *p;

	/* we already have this existing one */
	if (q_stage->goals > num) return &q_stage->goal[num];

	/* allocate all missing instances up to the requested index */
	p = (qi_goal*)realloc(q_stage->goal, sizeof(qi_goal) * (num + 1));
	if (!p) {
		s_printf("init_quest_goal() Couldn't allocate memory!\n");
		exit(0);
	}
	/* initialise the ADDED memory */
	//memset(p + q_stage->goals * sizeof(qi_goal), 0, (num + 1 - q_stage->goals) * sizeof(qi_goal));
	memset(&p[q_stage->goals], 0, (num + 1 - q_stage->goals) * sizeof(qi_goal));

	/* attach it to its parent structure */
	q_stage->goal = p;
	q_stage->goals = num + 1;

	/* initialise with correct defaults (paranoia) */
	p = &q_stage->goal[q_stage->goals - 1];
	p->kill = NULL;
	p->retrieve = NULL;
	p->deliver = NULL;

	/* done, return the new, requested one */
	return &q_stage->goal[num];
}

qi_reward *init_quest_reward(int q_idx, int stage, int num) {
	qi_stage *q_stage = init_quest_stage(q_idx, stage);
	qi_reward *p;

	/* we already have this existing one */
	if (q_stage->rewards > num) return &q_stage->reward[num];

	/* allocate all missing instances up to the requested index */
	p = (qi_reward*)realloc(q_stage->reward, sizeof(qi_reward) * (num + 1));
	if (!p) {
		s_printf("init_quest_reward() Couldn't allocate memory!\n");
		exit(0);
	}
	/* initialise the ADDED memory */
	//memset(p + q_stage->rewards * sizeof(qi_reward), 0, (num + 1 - q_stage->rewards) * sizeof(qi_reward));
	memset(&p[q_stage->rewards], 0, (num + 1 - q_stage->rewards) * sizeof(qi_reward));

	/* attach it to its parent structure */
	q_stage->reward = p;
	q_stage->rewards = num + 1;

	/* done, return the new, requested one */
	return &q_stage->reward[num];
}


/* ---------- Load/save dynamic quest data.  ---------- */
/* This cannot be done in the server savefile, because it gets read before all
   ?_info.txt files are read from lib/data. So for example the remaining number
   of kills in a kill-quest cannot be stored anywhere, since the stage and
   stage goals are not yet initialised. Oops.
   However, saving this quest data of random/varying lenght is a mess anyway,
   so it's good that we keep it far away from the server savefile. */

/* To call after server has been loaded up, to reattach questors and their quests,
   index-wise. The m-indices get shuffled over server restart. */
void fix_questors_on_startup(void) {
	quest_info *q_ptr;
	monster_type *m_ptr;
	int i;

	for (i = 1; i < m_max; i++) {
		m_ptr = &m_list[i];
		if (!m_ptr->questor) continue;

		q_ptr = &q_info[m_ptr->quest];
		if (!q_ptr->defined || /* this quest no longer exists in q_info.txt? */
		    !q_ptr->active || /* or it's not supposed to be enabled atm? */
		    q_ptr->questors <= m_ptr->questor_idx) { /* ew */
			s_printf("QUESTOR DEPRECATED (on load) midx %d, qidx %d.\n", i, m_ptr->quest);
			m_ptr->questor = FALSE;
			/* delete him too? */
		} else q_ptr->questor[m_ptr->questor_idx].m_idx = i;
	}
}

void quests_load(void) {
#if 0
    int i, j, k;
    s16b max, questors, stages, goals;
    byte flags = QI_FLAGS, tmpbyte, q_questors;
    int dummysize1 = sizeof(byte) * 7 + sizeof(s16b) * 3 + sizeof(s32b);
    monster_type *m_ptr;
    quest_info *q_ptr;

    rd_s16b(&max);
    rd_s16b(&questors);
    if (max > max_q_idx) s_printf("Warning: Read more quest info than available quests.\n");
    if (questors > QI_QUESTORS) s_printf("Warning: Read more questors than allowed.\n");
    if (!s_older_than(4, 5, 20)) {
	rd_byte(&flags);
	if (flags > QI_FLAGS) s_printf("Warning: Read more flags than available.\n");
    }

    for (i = 0; i < max; i++) {
	if (max > max_q_idx) {
	    //WARNING-- THIS WONT WORK WITH VERSIONS STARTING SOMEWHERE BETWEEN 4.5.19
	    //AND 4.5.26 ANYMORE, cause we have different/dynamic data size now +_+
	    //so just gotta add quests, never remove :-p
#if 0
	    strip_bytes(dummysize1);
	    continue;
#else
	    s_printf("<Loading quests: Critical Error - number of saved quests is greater than number of defined quests!>\n");
	    exit(0);
#endif
	}

	/* read 'active' and 'disabled' state */
	rd_byte((byte *) &tmpbyte);
	/* in case it was already disabled in q_info.txt */
	if (!q_info[i].disabled) {
	    q_info[i].active = (tmpbyte != 0);
	    rd_byte((byte *) &q_info[i].disabled);
	} else {
	    rd_byte((byte *) &tmpbyte); /* strip as dummy info */
	    q_info[i].disabled_on_load = TRUE;
	}

	rd_s16b(&q_info[i].cur_cooldown);
	rd_s16b(&q_info[i].cur_stage);
	rd_s32b(&q_info[i].turn_activated);
	if (!older_than(4, 5, 26)) {
	    rd_s32b(&q_info[i].turn_acquired);

	    rd_byte(&q_questors);
	}

	for (j = 0; j < questors && j < q_questors; j++) {
	    if (j >= QI_QUESTORS) {
		strip_bytes(7);
		continue;
	    }

	    rd_byte((byte *) &q_info[i].questor[j].current_wpos.wx);
	    rd_byte((byte *) &q_info[i].questor[j].current_wpos.wy);
	    rd_byte((byte *) &q_info[i].questor[j].current_wpos.wz);
	    rd_byte((byte *) &q_info[i].questor[j].current_x);
	    rd_byte((byte *) &q_info[i].questor[j].current_y);

	    rd_s16b(&q_info[i].questor[j].m_idx);
	}

	if (!older_than(4, 5, 25))
	    rd_u16b(&q_info[i].flags);
	else if (!s_older_than(4, 5, 20)) {
	    for (j = 0; j < flags; j++) {
		if (j >= QI_FLAGS) {
		    strip_bytes(1);
		    continue;
		}
		rd_byte(&tmpbyte);
		if (tmpbyte) q_info[i].flags |= (0x1 << j);
	    }
	}

	if (!older_than(4, 5, 26)) {
	    rd_byte(&stages);
	    for (k = 0; k < stages; k++) {
		rd_byte(&goals);
		for (j = 0; j < goals; j++) {
		    rd_byte((byte *) &q_info[i].stage[k].goal[j].cleared);
		    rd_byte((byte *) &q_info[i].stage[k].goal[j].nisi);
		    if (q_info[i].stage[k].goal[j].kill)
			rd_s16b(&q_info[i].kill_number_left[k][j]);
		    else
			strip_bytes(2);
		}
	    }
	} else if (!older_than(4, 5, 24)) { /* discard the old crap */
	    for (k = 0; k < QI_STAGES; k++) {
		for (j = 0; j < QI_GOALS; j++)
		    strip_bytes(3);
		for (j = 0; j < QI_OPTIONAL; j++)
		    strip_bytes(3);
	    }
	}
    }
    s_printf("Read %d/%d saved quests states (discarded %d).\n", max, max_q_idx, max > max_q_idx ? max - max_q_idx : 0);

    /* Now fix questors:
       Their m_idx is different now from what it was when the server shut down.
       We need to reattach the correct m_idx to the quests. */
    for (i = 1; i < m_max; i++) {
	m_ptr = &m_list[i];
	if (!m_ptr->questor) continue;

	q_ptr = &q_info[m_ptr->quest];
	q_ptr->questor[m_ptr->questor_idx].m_idx = i;
    }
#endif
}

void quests_save(void) {
#if 0
    int i, j, k;

    wr_s16b(max_q_idx);
    wr_s16b(QI_QUESTORS);
    wr_byte(QI_FLAGS);

    for (i = 0; i < max_q_idx; i++) {
	wr_byte(q_info[i].active);
#if 0 /* actually don't write this, it's more comfortable to use q_info '-2 repeatable' entry instead */
	wr_byte(q_info[i].disabled);
#else
	wr_byte(0);
#endif
	wr_s16b(q_info[i].cur_cooldown);
	wr_s16b(q_info[i].cur_stage);
	wr_s32b(q_info[i].turn_activated);
	wr_s32b(q_info[i].turn_acquired);

	for (j = 0; j < QI_QUESTORS; j++) {
#if 0//restructure
	    wr_byte(q_info[i].current_wpos[j].wx);
	    wr_byte(q_info[i].current_wpos[j].wy);
	    wr_byte(q_info[i].current_wpos[j].wz);
	    wr_byte(q_info[i].current_x[j]);
	    wr_byte(q_info[i].current_y[j]);

	    wr_s16b(q_info[i].questor_m_idx[j]);
#else
	    wr_byte(0);wr_byte(0);wr_byte(0);wr_byte(0);wr_byte(0);wr_byte(0);wr_byte(0);
#endif
	}

	wr_u16b(q_info[i].flags);

	wr_byte(q_info[i].stages);
	for (k = 0; k < q_info[i].stages; k++) {
	    wr_byte(q_info[i].stage[k].goals);
	    for (j = 0; j < q_info[i].stage[k].goals; j++) {
		wr_byte(q_info[i].stage[k].goal[j].cleared);
		wr_byte(q_info[i].stage[k].goal[j].nisi);
		if (q_info[i].stage[k].goal[j].kill)
		    wr_s16b(q_info[i].stage[k].goal[j].kill.number_left);
		else
		    wr_s16b(0);
	    }
	}
    }
#endif
}
