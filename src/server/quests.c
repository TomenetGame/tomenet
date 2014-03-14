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
	bool active;

	for (i = 0; i < max_q_idx; i++) {
		q_ptr = &q_info[i];

		/* check if quest should be active */
		active = FALSE;
		if (q_ptr->night && IS_NIGHT_RAW) active = TRUE; /* don't include pseudo-night like for Halloween/Newyearseve */
		if (q_ptr->day && !IS_NIGHT_RAW) active = TRUE;
		if (q_ptr->morning && IS_MORNING) active = TRUE;
		if (q_ptr->forenoon && IS_FORENOON) active = TRUE;
		if (q_ptr->noon && IS_NOON) active = TRUE;
		if (q_ptr->afternoon && IS_AFTERNOON) active = TRUE;
		if (q_ptr->evening && IS_EVENING) active = TRUE;
		if (q_ptr->midnight && IS_MIDNIGHT) active = TRUE;
		if (q_ptr->deepnight && IS_DEEPNIGHT) active = TRUE;
		if (q_ptr->time_start != -1) {
			if (bst(MINUTE, turn) >= q_ptr->time_start) {
				if (q_ptr->time_stop == -1 ||
				    bst(MINUTE, turn) < q_ptr->time_stop)
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
