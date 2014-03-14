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


static void quest_activate(int q_idx);
static void quest_deactivate(int q_idx);


/* enable debug log output */
#define QDEBUG

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

		if (q_ptr->disabled) continue;

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
static void quest_activate(int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
#ifdef QDEBUG
	s_printf("%s QUEST_ACTIVATE: '%s' ('%s' by '%s')\n", showtime(), q_name + q_ptr->name, q_ptr->codename, q_ptr->creator);
#endif
	q_ptr->active = TRUE;


#if 0
	c_ptr = &zcave[y][x];
	c_ptr->m_idx = m_pop();
	if (!c_ptr->m_idx) return;
	m_ptr = &m_list[c_ptr->m_idx];
	MAKE(m_ptr->r_ptr, monster_race);
	m_ptr->special = TRUE;
	m_ptr->fx = x;
	m_ptr->fy = y;
	r_ptr = m_ptr->r_ptr;
	r_ptr->flags1 = 0;
	r_ptr->flags2 = 0;
	r_ptr->flags3 = 0;
	r_ptr->flags4 = 0;
	r_ptr->flags5 = 0;
	r_ptr->flags6 = 0;
	delete_monster_idx(c_ptr->m_idx, TRUE);
	r_ptr->text = 0;
	r_ptr->name = 0;
	r_ptr->sleep = 0;
	r_ptr->aaf = 20;
	r_ptr->speed = 110;
	r_ptr->mexp = 1;
	r_ptr->d_attr = TERM_YELLOW;
	r_ptr->d_char = 'g';
	r_ptr->x_attr = TERM_YELLOW;
	r_ptr->x_char = 'g';
	r_ptr->freq_innate = 0;
	r_ptr->freq_spell = 0;
	r_ptr->flags1 |= RF1_FORCE_MAXHP;
	r_ptr->flags2 |= RF2_STUPID | RF2_EMPTY_MIND | RF2_REGENERATE | RF2_POWERFUL | RF2_BASH_DOOR | RF2_MOVE_BODY;
	r_ptr->flags3 |= RF3_HURT_ROCK | RF3_IM_COLD | RF3_IM_ELEC | RF3_IM_POIS | RF3_NO_FEAR | RF3_NO_CONF | RF3_NO_SLEEP | RF9_IM_TELE;
			r_ptr->hdice = 10;
			r_ptr->hside = 10;
			r_ptr->ac = 20;
	r_ptr->extra = golem_flags;
	m_ptr->speed = r_ptr->speed;
	m_ptr->mspeed = m_ptr->speed;
	m_ptr->ac = r_ptr->ac;
	m_ptr->maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
	m_ptr->hp = maxroll(r_ptr->hdice, r_ptr->hside);
	m_ptr->clone = 100;
	for (i = 0; i < golem_m_arms; i++) {
		m_ptr->blow[i].method = r_ptr->blow[i].method = RBM_HIT;
		m_ptr->blow[i].effect = r_ptr->blow[i].effect = RBE_HURT;
		m_ptr->blow[i].d_dice = r_ptr->blow[i].d_dice = (golem_type + 1) * 3;
		m_ptr->blow[i].d_side = r_ptr->blow[i].d_side = 3 + golem_arms[i];
	}
	m_ptr->owner = p_ptr->id;
	m_ptr->r_idx = 1 + golem_type;
	m_ptr->level = p_ptr->lev;
	m_ptr->exp = MONSTER_EXP(m_ptr->level);
	m_ptr->csleep = 0;
	wpcopy(&m_ptr->wpos, &p_ptr->wpos);
	m_ptr->stunned = 0;
	m_ptr->confused = 0;
	m_ptr->monfear = 0;
	m_ptr->cdis = 0;
	m_ptr->mind = GOLEM_NONE;
	r_ptr->flags8 |= RF8_NO_AUTORET | RF8_GENO_PERSIST | RF8_GENO_NO_THIN;
	r_ptr->flags7 |= RF7_NO_TARGET;
	update_mon(c_ptr->m_idx, TRUE);
#endif
//    bool questor, quest_invincible, quest_aggressive; /* further quest_info flags are referred to when required, no need to copy all of them here */
//    int quest;
}

/* Despawn questor, unstatic sector/floor, etc. */
static void quest_deactivate(int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
#ifdef QDEBUG
	s_printf("%s QUEST_DEACTIVATE: '%s' ('%s' by '%s')\n", showtime(), q_name + q_ptr->name, q_ptr->codename, q_ptr->creator);
#endif
	q_ptr->active = FALSE;
}
