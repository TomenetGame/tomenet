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

   Special quirk:
   Questors that are monsters use m_ptr->quest = q_idx.
   Questors that are objects, and quest items (that aren't questors) use o_ptr->quest = q_idx + 1.
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

/* Should we teleport item piles away to make exclusive room for questor objects,
   or should we just drop the questor object onto the pile? :) */
#define QUESTOR_OBJECT_EXCLUSIVE
/* If above is defined, should the questor item destroy all items on its grid
   when it spawns? Otherwise it'll just drop on top of them. */
#define QUESTOR_OBJECT_CRUSHES


static void quest_goal_check_reward(int pInd, int q_idx);
static bool quest_goal_check(int pInd, int q_idx, bool interacting);
static void quest_dialogue(int Ind, int q_idx, int questor_idx, bool repeat, bool interact_acquire, bool force_prompt);
static void quest_imprint_tracking_information(int Ind, int py_q_idx, bool target_flagging_only);
static void quest_check_goal_kr(int Ind, int q_idx, int py_q_idx, monster_type *m_ptr, object_type *o_ptr);
static void quest_remove_dungeons(int q_idx);


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


/* syllable generation for random passwords */
static cptr pw_consonant[] = {
     /* may occur everywhere (16) */
    "b", "c", "d", "f", "g",
    "k", "l", "m", "n", "p",
    "r", "s", "t", "w", "x", "z",

     /* may only occur as prefix in a syllable (29) */
    "h", "j", "v", /*"qu",*/ "kw", "bl",
    "br", "cl", "cr", "dr", "fl",
    "fr", "gl", "gr", "kl", /*"kn",*/ "kr",
    "pl", "pn", "pr", "ps", "sk",
    "sl", "sm", "sn", "sp", /*"sr",*/ "st",
    /*"th",*/ "tr", "tw", /*"vl", "vn",*/ "vr", /*"wl",*/ "wr",

    /* may not occur as both prefix and suffix in the same syllable (2) */
    "sh", "ch",

    /* may only occur as suffix in a syllable (1) */
    "ck"};
#define PW_ALL 16
#define PW_PRE 29
#define PW_XOR 2
#define PW_SUF 1
#define PWC_TOTAL (PW_ALL + PW_PRE + PW_XOR + PW_SUF)

static cptr pw_vowel[] = {
    "a", "e", "i", "o", "u", /*"y",*/

     /* may not be followed by 'ck'--actually not by any consonant (4) */
    "ay", "oy", "ee", "oo"};
#define PW_YV 4
#define PWV_TOTAL (5 + PW_YV)

/* Initialise randomised passwords */
static void qip_syllable(char *pw) {
	int q;
	bool p, v;

	q = rand_int(PWC_TOTAL - 1);
	strcat(pw, pw_consonant[q]);
	if (q >= PWC_TOTAL - PW_SUF - PW_XOR) p = TRUE;
	else p = FALSE;

	q = rand_int(PWV_TOTAL);
	strcat(pw, pw_vowel[q]);
	if (q >= PWV_TOTAL - PW_YV) v = TRUE;
	else v = FALSE;

#if 0 /* allow 'pseudo-russian'? ;) sound a bit odd though */
	if (rand_int(2)) {
#else
	if (!v && rand_int(2)) {
#endif
		if (p && v) q = rand_int(PWC_TOTAL - PW_SUF - PW_XOR - PW_PRE);
		else if (v) {
			q = rand_int(PWC_TOTAL - PW_YV - PW_PRE);
			if (q >= PW_ALL) q += PW_PRE;
		}
		else if (p) {
			q = rand_int(PWC_TOTAL - PW_SUF - PW_PRE);
			if (q >= PW_ALL) q += PW_PRE;
			if (q == PWC_TOTAL - PW_SUF - PW_XOR) q += PW_SUF;
		} else {
			q = rand_int(PWC_TOTAL - PW_PRE);
			if (q >= PW_ALL) q += PW_PRE;
		}
		strcat(pw, pw_consonant[q]);
	}
}
void quest_init_passwords(int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, tries = 100;

	for (i = 0; i < QI_PASSWORDS; i++) {
		q_ptr->password[i][0] = 0;

		/* syllable 1 */
		qip_syllable(q_ptr->password[i]);

		/* syllable 2 */
		qip_syllable(q_ptr->password[i]);
		if (strlen(q_ptr->password[i]) > QI_PASSWORD_LEN - 5) continue;

		/* syllable 3 */
		qip_syllable(q_ptr->password[i]);

		/* ^^' repeat? */
		if (handle_censor(q_ptr->password[i])) {
			tries--;
			if (tries) i--;
			else s_printf("Warning: Quest %d password #%d '%s' fails censor check, out of tries!\n", q_idx, i, q_ptr->password[i]);
		} else tries = 100;
	}
}

/* Called every minute to check which quests to activate/deactivate depending on time/daytime */
void process_quests(void) {
	quest_info *q_ptr;
	int i, j, k;
	bool night = IS_NIGHT_RAW, day = !night, morning = IS_MORNING, forenoon = IS_FORENOON, noon = IS_NOON;
	bool afternoon = IS_AFTERNOON, evening = IS_EVENING, midnight = IS_MIDNIGHT, deepnight = IS_DEEPNIGHT;
	int minute = bst(MINUTE, turn), hour = bst(HOUR, turn);
	bool active;
	qi_stage *q_stage;
	qi_questor_hostility *q_qhost;
	player_type *p_ptr;

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


		/* --------------- timers of quests in progress --------------- */
		if (!q_ptr->active) continue;


		/* handle individual cooldowns here too for now.
		   NOTE: this means player has to stay online for cooldown to expire, hmm */
		for (j = 1; j <= NumPlayers; j++) {
			p_ptr = Players[j];

			if (p_ptr->quest_cooldown[i])
				p_ptr->quest_cooldown[i]--;

			/* handle automatically timed stage actions for individual quests */
			for (k = 0; k < MAX_PQUESTS; k++)
				if (p_ptr->quest_idx[k] == i) break;
			if (k == MAX_PQUESTS) continue;

			if (p_ptr->quest_stage_timer[k] < 0) {
				if (hour == 1 - p_ptr->quest_stage_timer[k]) {
					q_stage = quest_qi_stage(i, p_ptr->quest_stage[k]);
					quest_set_stage(0, i, q_stage->change_stage, q_stage->quiet_change, NULL);
				}
			} else if (p_ptr->quest_stage_timer[k] > 0) {
				p_ptr->quest_stage_timer[k]--;
				if (!p_ptr->quest_stage_timer[k]) {
					q_stage = quest_qi_stage(i, p_ptr->quest_stage[k]);
					quest_set_stage(0, i, q_stage->change_stage, q_stage->quiet_change, NULL);
				}
			}
		}

		/* handle automatically timed stage actions for global quests */
		if (q_ptr->timed_countdown < 0) {
			if (hour == 1 - q_ptr->timed_countdown)
				quest_set_stage(0, i, q_ptr->timed_countdown_stage, q_ptr->timed_countdown_quiet, NULL);
		} else if (q_ptr->timed_countdown > 0) {
			q_ptr->timed_countdown--;
			if (!q_ptr->timed_countdown)
				quest_set_stage(0, i, q_ptr->timed_countdown_stage, q_ptr->timed_countdown_quiet, NULL);
		}

		/* handle automatic timed questor hostility */
		q_stage = quest_cur_qi_stage(i);
		for (j = 0; j < q_ptr->questors; j++) {
			if (!q_stage->questor_hostility[j]) continue;
			q_qhost = q_stage->questor_hostility[j];

			if (q_qhost->hostile_revert_timed_countdown < 0) {
				if (hour == 1 - q_qhost->hostile_revert_timed_countdown)
					quest_questor_reverts(i, j, &q_ptr->questor[j].current_wpos);
			} else if (q_qhost->hostile_revert_timed_countdown > 0) {
				q_qhost->hostile_revert_timed_countdown--;
				if (!q_qhost->hostile_revert_timed_countdown)
					quest_questor_reverts(i, j, &q_ptr->questor[j].current_wpos);
			}
		}
	}
}

/* Prepare a quest location by allocating/resetting/staticing/it or loading
   a dungeon file, just as needed. Returns zcave, just like getcave() does.
   'stat' will make the floor static. (If tpref is set, this is implied.)
   'tref', 'xstart' and 'ystart' load a map file.
   'dungeon' will prevent the map-file loading from fail if wpos is in a
   dungeon (or tower).
   NOTE: For now only supports world surface sectors safely, since it uses
         MAX_WID/MAX_HGT as fixed dimensions for tpref-loading. */
static cave_type **quest_prepare_zcave(struct worldpos *wpos, bool stat, cptr tpref, int xstart, int ystart, bool dungeon) {
	if (getcave(wpos)) {
		if (tpref) {
			/* In dungeons we cannot just unstatice a floor and
			   teleport people out, so we just 'fail' in that case..
			   - except if we're planning exactly this */
			if (wpos->wz && !dungeon) {
				s_printf("QUEST_PREPARE_ZCAVE: Already allocated for mapfile.\n");
				return NULL;
			}

			/* clear the floor and recreate it */
			teleport_players_level(wpos);
			dealloc_dungeon_level(wpos);
			alloc_dungeon_level(wpos);
			generate_cave(wpos, NULL);

#if 0
			wipe_m_list_admin(wpos);
			wipe_o_list_safely(wpos);
#endif

			process_dungeon_file(tpref, wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);
			/* loading a map file implies staticness! so it doesn't get reset */
			new_players_on_depth(wpos, 1, FALSE);
		} else if (stat) new_players_on_depth(wpos, 1, FALSE);
		return getcave(wpos);
	}

	/* not yet allocated, comfortable for us.. */
	alloc_dungeon_level(wpos);
	generate_cave(wpos, NULL);
	if (tpref) {
#if 0
		wipe_m_list_admin(wpos);
		wipe_o_list_safely(wpos);
#endif

		process_dungeon_file(tpref, wpos, &ystart, &xstart, MAX_HGT, MAX_WID, TRUE);
		/* loading a map file implies staticness! so it doesn't get reset */
		new_players_on_depth(wpos, 1, FALSE);
	} else if (stat) new_players_on_depth(wpos, 1, FALSE);
	return getcave(wpos);
}

/* Replace placeholders $$n/N, $$t/T, $$r/R, $$a/A and $$c/C in a string,
   thereby personalising it for dialogues and narrations. */
static void quest_text_replace(char *dest, cptr src, player_type *p_ptr) {
	char *textp, *textp2;
	int pos = 0;

	/* no placeholder detected? */
	if (!(textp = strstr(src, "$$"))) {
		strcpy(dest, src);
		return;
	}

	strncpy(dest, src, textp - src);
	pos = textp - src;
	dest[pos] = 0;
	while (TRUE) {
		switch (*(textp + 2)) {
		case 'N':
			strcat(dest, p_ptr->name);
			break;
		case 'T':
			strcat(dest, get_ptitle(p_ptr, FALSE));
			break;
		case 'R':
			strcat(dest, race_info[p_ptr->prace].title);
			break;
		case 'A':
			strcat(dest, get_prace(p_ptr));
			break;
		case 'C':
			strcat(dest, race_info[p_ptr->prace].title);
			break;
		case 'n':
			strcat(dest, p_ptr->name);
			dest[pos] = tolower(dest[pos]);
			break;
		case 't':
			strcat(dest, get_ptitle(p_ptr, FALSE));
			dest[pos] = tolower(dest[pos]);
			break;
		case 'r':
			strcat(dest, race_info[p_ptr->prace].title);
			dest[pos] = tolower(dest[pos]);
			break;
		case 'a':
			strcat(dest, get_prace(p_ptr));
			dest[pos] = tolower(dest[pos]);
			break;
		case 'c':
			strcat(dest, race_info[p_ptr->prace].title);
			dest[pos] = tolower(dest[pos]);
			break;
		}

		textp2 = strstr(textp + 3, "$$");
		if (!textp2) {
			strcat(dest, textp + 3);
			return;
		}

		pos += (textp2 - textp) - 3;
		strncat(dest, textp + 3, (textp2 - textp) - 3);
		dest[pos] = 0;
		textp = textp2;
	}
}

/* Helper function to pick one *set* flag at random,
   for determining questor spawn locations */
static u32b quest_pick_flag(u32b flagarray, int size) {
	int i;
	int flags = 0, choice;

	if (flagarray == 0x0) return 0x0;

	/* count flags that are set */
	for (i = 0; i < size; i++)
		if ((flagarray & (0x1 << i))) flags++;

	/* pick one */
	choice = randint(flags);

	/* translate back into flag and return it */
	for (i = 0; i < size; i++) {
		if (!(flagarray & (0x1 << i))) continue;
		if (--choice) continue;
		return (0x1 << i);
	}
	return 0x0; //paranoia
}

/* Tries to find a suitable wpos and x, y location (for questors or quest items),
   using qi_location as parameter.
   'stat' will also make the chosen location static.
   'dungeon' will prevent the location check from failing if it's inside a
   dungeon/tower and already allocated. */
static bool quest_special_spawn_location(struct worldpos *wpos, s16b *x_result, s16b *y_result, struct qi_location *q_loc, bool stat, bool dungeon) {
	int i, tries, min, max;
	cave_type **zcave;
	u32b choice, wild = RF8_WILD_TOO_MASK & ~(RF8_WILD_TOWN | RF8_WILD_EASY);
	int x, y, x2 = 63, y2 = 63; //compiler warning

	wpos->wx = 63; //default for cases of acute paranoia
	wpos->wy = 63;
	wpos->wz = 0;

	/* ---------- Try to pick a (random) spawn location ---------- */

	/* check 'L:' info in q_info.txt for details on starting location */
#if QDEBUG > 2
	s_printf(" SWPOS,XY: %d,%d,%d - %d,%d\n", q_loc->start_wpos.wx, q_loc->start_wpos.wy, q_loc->start_wpos.wz, q_loc->start_x, q_loc->start_y);
	s_printf(" SLOCT, STAR: %d,%d\n", q_loc->s_location_type, q_loc->s_towns_array);
#endif
	/* specified exact questor starting wpos? */
	if (q_loc->start_wpos.wx != -1) {
		wpos->wx = q_loc->start_wpos.wx;
		wpos->wy = q_loc->start_wpos.wy;
		wpos->wz = q_loc->start_wpos.wz;

		/* vary strict wpos a bit into adjacent terrain patches of same type? */
		if (q_loc->terrain_patch && !wpos->wz) {
			tries = 500;
			while (--tries) {
				x2 = wpos->wx - QI_TERRAIN_PATCH_RADIUS + rand_int(QI_TERRAIN_PATCH_RADIUS * 2 + 1);
				y2 = wpos->wy - QI_TERRAIN_PATCH_RADIUS + rand_int(QI_TERRAIN_PATCH_RADIUS * 2 + 1);
				if (distance(y2, x2, wpos->wy, wpos->wx) <= QI_TERRAIN_PATCH_RADIUS &&
				    wild_info[y2][x2].type == wild_info[wpos->wy][wpos->wx].type) break;

				/* specialty: avoid players, so we don't have to teleport
				   them around in case we have to deallocate the sector */
				if (!wpos->wz) {
					if (wild_info[y2][x2].ondepth && tries > 250) continue;
				} else if (wpos->wz < 0) {
					if (wild_info[y2][x2].dungeon->level[ABS(wpos->wz) - 1].ondepth && tries > 250) continue;
				} else {
					if (wild_info[y2][x2].tower->level[wpos->wz - 1].ondepth && tries > 250) continue;
				}
			}
			if (tries) { /* yay */
				wpos->wx = x2;
				wpos->wy = y2;
			}
		}
	}
	/* ok, pick one starting location randomly from all eligible ones */
	else {
		choice = quest_pick_flag(q_loc->s_location_type, 4);
		/* no or non-existing type specified */
		if (!choice) return FALSE;

		switch (choice) {
		case QI_SLOC_SURFACE:
			/* all terrains are possible? */
			if ((q_loc->s_terrains & RF8_WILD_TOO)) {
				choice = quest_pick_flag(wild, 32);
			}
			/* pick from specified list */
			else choice = quest_pick_flag(q_loc->s_terrains & wild, 32);
			/* no or non-existing type specified */
			if (!choice) return FALSE;

			/* pick one wpos location randomly, that matches our terrain */
			tries = 2000;
			while (--tries) {
				x = rand_int(MAX_WILD_X);
				y = rand_int(MAX_WILD_Y);

				/* specialty: avoid players, so we don't have to teleport
				   them around in case we have to deallocate the sector */
				if (wild_info[y][x].ondepth && tries > 1000) continue;

				switch (wild_info[y][x].type) {
				case WILD_OCEAN: if (choice == RF8_WILD_OCEAN) break;
				case WILD_LAKE: if (choice == RF8_WILD_LAKE) break;
				case WILD_GRASSLAND: if (choice == RF8_WILD_GRASS) break;
				case WILD_FOREST: if (choice == RF8_WILD_WOOD) break;
				case WILD_SWAMP: if (choice == RF8_WILD_SWAMP) break;
				case WILD_DENSEFOREST: if (choice == RF8_WILD_WOOD) break;
				case WILD_WASTELAND: if (choice == RF8_WILD_WASTE) break;
				case WILD_DESERT: if (choice == RF8_WILD_DESERT) break;
				case WILD_ICE: if (choice == RF8_WILD_ICE) break;
				}
			}
			if (!tries) /* engage emergency eloquence */
				for (x = 0; x < MAX_WILD_X; x++)
					for (y = 0; y < MAX_WILD_Y; y++) {
						switch (wild_info[y][x].type) {
						case WILD_OCEAN: if (choice == RF8_WILD_OCEAN) break;
						case WILD_LAKE: if (choice == RF8_WILD_LAKE) break;
						case WILD_GRASSLAND: if (choice == RF8_WILD_GRASS) break;
						case WILD_FOREST: if (choice == RF8_WILD_WOOD) break;
						case WILD_SWAMP: if (choice == RF8_WILD_SWAMP) break;
						case WILD_DENSEFOREST: if (choice == RF8_WILD_WOOD) break;
						case WILD_WASTELAND: if (choice == RF8_WILD_WASTE) break;
						case WILD_DESERT: if (choice == RF8_WILD_DESERT) break;
						case WILD_ICE: if (choice == RF8_WILD_ICE) break;
						}
					}
			/* paranoia.. */
			if (x == MAX_WILD_X && y == MAX_WILD_Y) {
				/* back to test server :-p */
				x = MAX_WILD_X - 1;
				y = MAX_WILD_Y - 1;
			}

			wpos->wx = x;
			wpos->wy = y;
			wpos->wz = 0;
			break;

		case QI_SLOC_TOWN:
			choice = quest_pick_flag(q_loc->s_towns_array, 5);
			/* no or non-existing type specified */
			if (!choice) return FALSE;

			/* assume non-dungeon town */
			wpos->wz = 0;


			switch (choice) { /* TODO: such hardcode. wow. */
			case QI_STOWN_BREE:
				for (i = 0; i < numtowns; i++)
					if (town[i].type == TOWN_BREE) {
						wpos->wx = town[i].x;
						wpos->wy = town[i].y;
						break;
					}
				break;
			case QI_STOWN_GONDOLIN:
				for (i = 0; i < numtowns; i++)
					if (town[i].type == TOWN_GONDOLIN) {
						wpos->wx = town[i].x;
						wpos->wy = town[i].y;
						break;
					}
				break;
			case QI_STOWN_MINASANOR:
				for (i = 0; i < numtowns; i++)
					if (town[i].type == TOWN_MINAS_ANOR) {
						wpos->wx = town[i].x;
						wpos->wy = town[i].y;
						break;
					}
				break;
			case QI_STOWN_LOTHLORIEN:
				for (i = 0; i < numtowns; i++)
					if (town[i].type == TOWN_LOTHLORIEN) {
						wpos->wx = town[i].x;
						wpos->wy = town[i].y;
						break;
					}
				break;
			case QI_STOWN_KHAZADDUM:
				for (i = 0; i < numtowns; i++)
					if (town[i].type == TOWN_KHAZADDUM) {
						wpos->wx = town[i].x;
						wpos->wy = town[i].y;
						break;
					}
				break;
			case QI_STOWN_WILD:
				return FALSE;
				break;
			case QI_STOWN_DUNGEON:
				return FALSE;
				break;
			case QI_STOWN_IDDC:
				return FALSE;
				break;
			case QI_STOWN_IDDC_FIXED:
				return FALSE;
				break;
			}
			break;

		case QI_SLOC_DUNGEON:
			if (!q_loc->s_dungeons) return FALSE;
			i = q_loc->s_dungeon[rand_int(q_loc->s_dungeons)];

			/* IDDC? */
			if (i == 255) {
#ifdef WPOS_IRONDEEPDIVE_X
				wpos->wx = WPOS_IRONDEEPDIVE_X;
				wpos->wy = WPOS_IRONDEEPDIVE_Y;
				wpos->wz = WPOS_IRONDEEPDIVE_Z * (rand_int(q_loc->dlevmax - q_loc->dlevmin + 1) + q_loc->dlevmin);
#else
				wpos->wx = 0;
				wpos->wy = 0;
				wpos->wz = 0;
#endif
			}
			/* Any wilderness dungeon? */
			else if (i == 0) {
				//todo: implement
			}
			/* Normal dungeon */
			else {
				choice = 0;//'found' marker
				for (x = 0; x < MAX_WILD_X; x++) {
					for (y = 0; y < MAX_WILD_Y; y++) {
						if (wild_info[y][x].tower && wild_info[y][x].tower->type == i) {
							wpos->wx = x;
							wpos->wy = y;
							min = wild_info[y][x].tower->baselevel;
							max = wild_info[y][x].tower->baselevel + wild_info[y][x].tower->maxdepth - 1;

							if (!(min > q_loc->dlevmax || max < q_loc->dlevmin)) {
								if (min <= q_loc->dlevmin) {
									if (max >= q_loc->dlevmax)
										i = q_loc->dlevmin + rand_int(q_loc->dlevmax - q_loc->dlevmin + 1) - min + 1;
									else
										i = q_loc->dlevmin + rand_int(max - q_loc->dlevmin + 1) - min + 1;
								} else {
									if (max >= q_loc->dlevmax)
										i = rand_int(q_loc->dlevmax - min + 1) + 1;
									else
										i = rand_int(max - min + 1) + 1;
								}

								wpos->wz = i;
								choice = 1;
								break;
							}
						}
						if (wild_info[y][x].dungeon && wild_info[y][x].dungeon->type == i) {
							wpos->wx = x;
							wpos->wy = y;
							min = wild_info[y][x].dungeon->baselevel;
							max = wild_info[y][x].dungeon->baselevel + wild_info[y][x].dungeon->maxdepth - 1;

							if (!(min > q_loc->dlevmax || max < q_loc->dlevmin)) {
								if (min <= q_loc->dlevmin) {
									if (max >= q_loc->dlevmax)
										i = q_loc->dlevmin + rand_int(q_loc->dlevmax - q_loc->dlevmin + 1) - min + 1;
									else
										i = q_loc->dlevmin + rand_int(max - q_loc->dlevmin + 1) - min + 1;
								} else {
									if (max >= q_loc->dlevmax)
										i = rand_int(q_loc->dlevmax - min + 1) + 1;
									else
										i = rand_int(max - min + 1) + 1;
								}

								wpos->wz = -i;
								choice = 1;
								break;
							}
						}
					}
					if (choice) break;
				}
				if (x == MAX_WILD_X && y == MAX_WILD_Y) {
					s_printf("QUEST_SPECIAL_SPAWN_LOCATION: Couldn't get suitable dungeon location.\n");
					return FALSE;
				}
			}
			break;

#if 0 /* included in QI_SLOC_DUNGEON atm */
		case QI_SLOC_TOWER:
			return FALSE;
			break;
#endif
		}
	}

	zcave = quest_prepare_zcave(wpos, stat, q_loc->tpref, q_loc->tpref_x, q_loc->tpref_y, dungeon);
	if (!zcave) return FALSE;

	/* Specified exact starting location within given wpos? */
	if (q_loc->start_x != -1) {
		x = q_loc->start_x;
		y = q_loc->start_y;

		/* vary strict starting loc a bit, within a radius? */
		if (q_loc->radius) {
			tries = 100;
			while (--tries) {
				x2 = q_loc->start_x - q_loc->radius + rand_int(q_loc->radius * 2 + 1);
				y2 = q_loc->start_y - q_loc->radius + rand_int(q_loc->radius * 2 + 1);
				if (!in_bounds(y2, x2)) continue;
				if (!cave_naked_bold(zcave, y2, x2)) continue;
				if (distance(y2, x2, y, x) <= q_loc->radius) break;
			}
			if (!tries) { /* no fun */
				x = q_loc->start_x;
				y = q_loc->start_y;
			}
		}
	} else {
		/* find a random spawn loc */
		i = 1000; //tries
		while (i) {
			i--;
			/* not too close to level borders maybe */
			x = rand_int(MAX_WID - 6) + 3;
			y = rand_int(MAX_HGT - 6) + 3;
			if (!in_bounds(y2, x2)) continue;
			if (!cave_naked_bold(zcave, y, x)) continue;
			break;
		}
		if (!i) {
			return FALSE;
			s_printf("QUEST_SPECIAL_SPAWN_LOCATION: No free spawning location.\n");
		}
	}

	*x_result = x;
	*y_result = y;

	return TRUE;
}

/* Attempt to spawn a monster-type questor */
static bool questor_monster(int q_idx, qi_questor *q_questor, int questor_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, m_idx;
	cave_type **zcave, *c_ptr;
	monster_race *r_ptr, *rbase_ptr;
	monster_type *m_ptr;

	/* data written back to q_info[] */
	struct worldpos wpos = {63, 63, 0}; //default for cases of acute paranoia
	int x, y;


	/* if it's supposed to be despawned, that's it
	   (for when an F-line dictates 'despawned' already in stage -1) */
	if (q_questor->despawned) {
		s_printf("QUESTOR_MONSTER: still despawned.\n");
		return TRUE;
	}

	wpos.wx = q_questor->current_wpos.wx;
	wpos.wy = q_questor->current_wpos.wy;
	wpos.wz = q_questor->current_wpos.wz;

	x = q_questor->current_x;
	y = q_questor->current_y;

	zcave = getcave(&wpos);
	c_ptr = &zcave[y][x];


	/* If no monster race index is set for the questor, don't spawn him. (paranoia) */
	if (!q_questor->ridx) return FALSE;

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

	m_ptr->questor = TRUE;
	m_ptr->questor_idx = questor_idx;
	m_ptr->quest = q_idx;

	m_ptr->r_idx = q_questor->ridx;
	m_ptr->ego = q_questor->reidx;
	MAKE(m_ptr->r_ptr, monster_race);
	r_ptr = m_ptr->r_ptr;
	if (m_ptr->ego) rbase_ptr = race_info_idx(m_ptr->r_idx, m_ptr->ego, 0);
	else rbase_ptr = &r_info[q_questor->ridx];

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
				m_ptr->astar_idx = i;
 #ifdef ASTAR_DISTRIBUTE
				astar_info_closed[i].nodes = 0;
 #endif
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
	m_ptr->questor_hostile = 0x0;
	m_ptr->questor_target = 0x0;
	q_questor->mo_idx = m_idx;

	update_mon(c_ptr->m_idx, TRUE);
#if QDEBUG > 1
	s_printf(" QUEST_SPAWNED: Questor '%s' (m_idx %d) at %d,%d,%d - %d,%d.\n", q_questor->name, m_idx, wpos.wx, wpos.wy, wpos.wz, x, y);
#endif

	/* bad hard-coded lite - TODO: treat like a real light source */
	if (q_questor->lite) {
		byte lite_rad = q_questor->lite % 100;
		u32b lite_type = CAVE_LITE;
		int cx, cy;

		//hack: cave_glow does the persistent lighting for now, until it's un-hardcoded..
#if 0 /* just allows white light */
		lite_type |= CAVE_GLOW;
#else /* mega hack added just to expand this hard-coding hack: allow TERM_LAMP light */
		if (q_questor->lite < 100) lite_type |= CAVE_GLOW_HACK_LAMP;
		else lite_type |= CAVE_GLOW_HACK;
#endif

		switch (q_questor->lite / 100) {
		case 0: break; //fire light, torch style
		case 1: lite_type |= CAVE_LITE_WHITE; break; //feanorian style
		case 2: lite_type |= CAVE_LITE_VAMP; break; //o_O
		}

		for (cx = x - lite_rad; cx <= x + lite_rad; cx++)
		for (cy = y - lite_rad; cy <= y + lite_rad; cy++) {
			if (distance(cy, cx, y, x) > lite_rad) continue;
			zcave[cy][cx].info |= lite_type;
			everyone_lite_spot(&wpos, cy, cx);
		}
	}

	q_questor->talk_focus = 0;
	return TRUE;
}

#ifdef QUESTOR_OBJECT_EXCLUSIVE
 #ifndef QUESTOR_OBJECT_CRUSHES
/* Helper function for questor_object() to teleport away a whole pile of items */
static void teleport_objects_away(struct worldpos *wpos, s16b x, s16b y, int dist) {
	u16b this_o_idx, next_o_idx = 0;
	int j;
	s16b cx, cy;

	cave_type **zcave = getcave(wpos), *c_ptr = &zcave[y][x];
	object_type tmp_obj;

  #if 1 /* scatter the pile? */
    for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
	tmp_obj = o_list[this_o_idx];
	for (j = 0; j < 10; j++) {
		cx = x + dist - rand_int(dist * 2);
		cy = y + dist - rand_int(dist * 2);
		if (cx == x || cy == y) continue;

		if (!in_bounds(cy, cx)) continue;
		if (!cave_floor_bold(zcave, cy, cx) ||
		    cave_perma_bold(zcave, cy, cx)) continue;

//		(void)floor_carry(cy, cx, &tmp_obj);
		drop_near(0, &tmp_obj, 0, wpos, cy, cx);
		delete_object_idx(this_o_idx, FALSE);
		break;
	}
    }
  #else /* 'teleport' the pile as a whole? --- UNTESTED!!! */
	for (j = 0; j < 10; j++) {
		cx = x + dist - rand_int(dist * 2);
		cy = y + dist - rand_int(dist * 2);
		if (cx == x || cy == y) continue;

		if (!in_bounds(cy, cx)) continue;
		if (!cave_floor_bold(zcave, cy, cx) ||
		    cave_perma_bold(zcave, cy, cx)) continue;

		/* no idea if this works */
		if (!cave_naked_bold(zcave, cy, cx)) continue;
		zcave[cy][cx].o_idx = zcave[y][x].o_idx;
		zcave[y][x].o_idx = 0;
		break;
	}
  #endif
}
 #endif
#endif

/* Attempt to spawn an object-type questor */
static bool questor_object(int q_idx, qi_questor *q_questor, int questor_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	int o_idx, i;
	object_type *o_ptr;
	cave_type **zcave, *c_ptr;
	u32b resf = RESF_NOTRUEART;

	/* data written back to q_info[] */
	struct worldpos wpos;
	int x, y;


	wpos.wx = q_questor->current_wpos.wx;
	wpos.wy = q_questor->current_wpos.wy;
	wpos.wz = q_questor->current_wpos.wz;

	x = q_questor->current_x;
	y = q_questor->current_y;

	zcave = getcave(&wpos);
	c_ptr = &zcave[y][x];

	/* If no monster race index is set for the questor, don't spawn him. (paranoia) */
	if (!q_questor->otval || !q_questor->osval) return FALSE;


	/* ---------- Try to spawn the questor object ---------- */

	o_idx = o_pop();
	if (!o_idx) {
		s_printf(" QUEST_CANCELLED: No free object index to pop questor.\n");
		q_ptr->active = FALSE;
#ifdef QERROR_DISABLE
		q_ptr->disabled = TRUE;
#else
		q_ptr->cur_cooldown = QERROR_COOLDOWN;
#endif
		return FALSE;
	}

	/* make sure no other object is occupying our spawning grid */
	if (c_ptr->o_idx) {
		/* is it ANOTHER questor? uhoh */
		if (o_list[c_ptr->o_idx].questor) {
			/* we don't mess with questor locations for consistencies sake */
			s_printf(" QUEST_CANCELLED: Questor of quest %d occupies questor spawn location.\n", o_list[c_ptr->o_idx].quest - 1);
			q_ptr->active = FALSE;
#ifdef QERROR_DISABLE
			q_ptr->disabled = TRUE;
#else
			q_ptr->cur_cooldown = QERROR_COOLDOWN;
#endif
			return FALSE;
		}

#ifdef QUESTOR_OBJECT_EXCLUSIVE
 #ifdef QUESTOR_OBJECT_CRUSHES
		delete_object(&wpos, y, x, TRUE);
 #else
		/* teleport the whole pile of objects away */
		teleport_objects_away(&wpos, x, y, 1);
		/* check again.. */
		if (c_ptr->o_idx > 0) teleport_objects_away(&wpos, x, y, 2);
		/* aaaand again.. */
		if (c_ptr->o_idx > 0) teleport_objects_away(&wpos, x, y, 10);
		/* Illusionate is looting D pits again */
		if (c_ptr->o_idx > 0) teleport_objects_away(&wpos, x, y, 200);
		/* out of patience */
		if (c_ptr->o_idx > 0) delete_object_idx(c_ptr->o_idx, TRUE);
 #endif
#else
		/* just drop the questor onto the pile of stuff (if any).
		   -- fall through -- */
#endif
	}

	/* 'drop' it */
	o_ptr = &o_list[o_idx];
	invcopy(o_ptr, lookup_kind(q_questor->otval, q_questor->osval));
	o_ptr->ix = x;
	o_ptr->iy = y;
	wpcopy(&o_ptr->wpos, &wpos);

	o_ptr->name1 = q_questor->oname1;
	o_ptr->name2 = q_questor->oname2;
	o_ptr->name2b = q_questor->oname2b;
	apply_magic(&wpos, o_ptr, -2, FALSE, q_questor->ogood, q_questor->ogreat, q_questor->overygreat, resf);
	o_ptr->pval = q_questor->opval;
	o_ptr->bpval = q_questor->obpval;
	o_ptr->level = q_questor->olev;

	o_ptr->next_o_idx = c_ptr->o_idx; /* on top of pile, if any */
	o_ptr->stack_pos = 0;
	c_ptr->o_idx = o_idx;
	nothing_test2(c_ptr, x, y, &wpos, 3);
	q_ptr->objects_registered++;
	o_ptr->marked = 0;
	o_ptr->held_m_idx = 0;

	/* imprint questor status and details */
	o_ptr->questor = TRUE;
	o_ptr->questor_idx = questor_idx;
	o_ptr->quest = q_idx + 1;
	o_ptr->questor_invincible = q_questor->invincible;
	q_questor->mo_idx = o_idx;

	/* Clear visibility flags */
	for (i = 1; i <= NumPlayers; i++) Players[i]->obj_vis[o_idx] = FALSE;
	/* Note the spot */
	note_spot_depth(&wpos, y, x);
	/* Draw the spot */
	everyone_lite_spot(&wpos, y, x);

#if QDEBUG > 1
	s_printf(" QUEST_SPAWNED: Questor '%s' (o_idx %d) at %d,%d,%d - %d,%d.\n", q_questor->name, o_idx, wpos.wx, wpos.wy, wpos.wz, x, y);
#endif

	/* bad hard-coded lite - TODO: treat like a real light source */
	if (q_questor->lite) {
		byte lite_rad = q_questor->lite % 100;
		u32b lite_type = CAVE_GLOW | CAVE_LITE; //cave_glow does the persistent lighting for now, until it's un-hardcoded..
		int cx, cy;

		//hack: cave_glow does the persistent lighting for now, until it's un-hardcoded..
#if 0 /* just allows white light */
		lite_type |= CAVE_GLOW;
#else /* mega hack added just to expand this hard-coding hack: allow TERM_LAMP light */
		if (q_questor->lite < 100) lite_type |= CAVE_GLOW_HACK_LAMP;
		else lite_type |= CAVE_GLOW_HACK;
#endif

		switch (q_questor->lite / 100) {
		case 0: break; //fire light, torch style
		case 1: lite_type |= CAVE_LITE_WHITE; break; //feanorian style
		case 2: lite_type |= CAVE_LITE_VAMP; break; //o_O
		}

		for (cx = x - lite_rad; cx <= x + lite_rad; cx++)
		for (cy = y - lite_rad; cy <= y + lite_rad; cy++) {
			if (distance(cy, cx, y, x) > lite_rad) continue;
			zcave[cy][cx].info |= lite_type;
			everyone_lite_spot(&wpos, cy, cx);
		}
	}

	q_questor->talk_focus = 0;
	return TRUE;
}

/* Helper function: Initialise and spawn a questor.
   Returns TRUE if succeeded to spawn. */
static bool quest_spawn_questor(int q_idx, int questor_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_questor *q_questor = &q_ptr->questor[questor_idx];

	/* find a suitable spawn location for the questor */
	if (!quest_special_spawn_location(&q_questor->current_wpos, &q_questor->current_x, &q_questor->current_y, &q_questor->q_loc, q_questor->static_floor, FALSE)) {
		s_printf(" QUEST_CANCELLED: Couldn't obtain questor spawning location.\n");
		q_ptr->active = FALSE;
#ifdef QERROR_DISABLE
		q_ptr->disabled = TRUE;
#else
		q_ptr->cur_cooldown = QERROR_COOLDOWN;
#endif
		return FALSE;
	}

	/* generate and spawn the questor */
	switch (q_questor->type) {
	case QI_QUESTOR_NPC:
		if (!questor_monster(q_idx, q_questor, questor_idx)) return FALSE;
		break;
	case QI_QUESTOR_ITEM_PICKUP:
		if (!questor_object(q_idx, q_questor, questor_idx)) return FALSE;
		break;
	default: ;
		//TODO: other questor-types
		/* discard */
	}

	return TRUE;
}

/* Spawn questor, prepare sector/floor, make things static if requested, etc. */
bool quest_activate(int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, q;

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

	/* This needs to be initialised before spawning questors,
	   because object-type questors count for this too. */
	q_ptr->objects_registered = 0;

	/* Spawn questors (monsters/objects that players can
	   interact with to acquire this quest): */
	for (q = 0; q < q_ptr->questors; q++) {
		quest_spawn_questor(q_idx, q);
	}

	/* Initialise dynamic data */
	q_ptr->flags = 0x0000;
	q_ptr->timed_countdown = 0;
	for (i = 0; i < q_ptr->stages; i++) {
		for (q = 0; q < q_ptr->questors; q++)
			if (q_ptr->stage[i].questor_hostility[q])
				q_ptr->stage[i].questor_hostility[q]->hostile_revert_timed_countdown = 0;
		for (q = 0; q < q_ptr->stage[i].goals; q++) {
			q_ptr->stage[i].goal[q].cleared = FALSE;
			q_ptr->stage[i].goal[q].nisi = FALSE;
			if (q_ptr->stage[i].goal[q].kill)
				q_ptr->stage[i].goal[q].kill->number_left = 0;
		}
	}

	quest_init_passwords(q_idx);

	/* Initialise with starting stage 0 */
	q_ptr->turn_activated = turn;
	q_ptr->cur_stage = -1;
	quest_set_stage(0, q_idx, 0, FALSE, NULL);

	return TRUE;
}

/* Helper function for quest_deactivate().
   Search and destroy object-type questors and quest objects, similar to erase_guild_key().
   'p_id' is a player id. If 'individual' is != 0, this is additionally checked
   against the object owner. */
static void quest_erase_objects(int q_idx, byte individual, s32b p_id) {
	int i, j;
	object_type *o_ptr;

	int slot;
	hash_entry *ptr;
	player_type *p_ptr;

	/* Is the quest even missing any objects? */
	if (!q_info[q_idx].objects_registered) {
#if QDEBUG > 1
		s_printf(" No objects registered for erasure.\n");
#endif
		return;
	}

	/* Open world items */
#if QDEBUG > 1
	s_printf(" Erasing all object questors and quest objects..\n");
#endif
	for (j = 0; j < o_max; j++) {
		if (o_list[j].quest != q_idx + 1) continue;
		if (individual && o_list[j].owner != p_id) continue;
#if QDEBUG > 1
		s_printf("Erased one at %d.\n", j);
#endif
		o_list[j].questor = o_list[j].quest = 0; //prevent questitem_d() check from triggering when deleting it!..
		q_info[q_idx].objects_registered--; //..and count down manually
		delete_object_idx(j, TRUE);
	}

	if (q_info[q_idx].objects_registered < 0) {//paranoia
#if QDEBUG > 1
		s_printf(" Warning: Negative number of registered objects.\n");
#endif
		return;
	} else if (!q_info[q_idx].objects_registered) {
#if QDEBUG > 1
		s_printf(" Erased all registered objects.\n");
#endif
		return;
	}

	/* Players online */
	for (j = 1; j <= NumPlayers; j++) {
		p_ptr = Players[j];
		/* scan his inventory */
		for (i = 0; i < INVEN_TOTAL; i++) {
			o_ptr = &p_ptr->inventory[i];
			if (!o_ptr->k_idx) continue;
			if (o_ptr->quest != q_idx + 1) continue;
			if (individual && o_ptr->owner != p_id) continue;

#if QDEBUG > 1
			s_printf("QUEST_OBJECT: player '%s'\n", p_ptr->name);
#endif
			o_ptr->questor = o_ptr->quest = 0; //prevent questitem_d() check from triggering when deleting it!..
			q_info[q_idx].objects_registered--; //..and count down manually
			//questitem_d(o_ptr, o_ptr->number);
			inven_item_increase(j, i, -99);
			inven_item_describe(j, i);
			inven_item_optimize(j, i);
		}
	}

	if (q_info[q_idx].objects_registered < 0) {//paranoia
#if QDEBUG > 1
		s_printf(" Warning: Negative number of registered objects.\n");
#endif
		return;
	} else if (!q_info[q_idx].objects_registered) {
#if QDEBUG > 1
		s_printf(" Erased all registered objects.\n");
#endif
		return;
	}

	/* hack */
	NumPlayers++;
	MAKE(Players[NumPlayers], player_type);
	p_ptr = Players[NumPlayers];
	p_ptr->inventory = C_NEW(INVEN_TOTAL, object_type);
	for (slot = 0; slot < NUM_HASH_ENTRIES; slot++) {
		ptr = hash_table[slot];
		while (ptr) {
			/* clear his data (especially inventory) */
			o_ptr = p_ptr->inventory;
			WIPE(p_ptr, player_type);
			p_ptr->inventory = o_ptr;
			p_ptr->Ind = NumPlayers;
			C_WIPE(p_ptr->inventory, INVEN_TOTAL, object_type);
			/* set his supposed name */
			strcpy(p_ptr->name, ptr->name);
			/* generate savefile name */
			process_player_name(NumPlayers, TRUE);
			/* try to load him! */
			if (!load_player(NumPlayers)) {
				/* bad fail */
				s_printf("QUEST_OBJECT_ERASE: load_player '%s' failed\n", p_ptr->name);
				continue;
				/* unhack */
				C_FREE(p_ptr->inventory, INVEN_TOTAL, object_type);
				KILL(p_ptr, player_type);
				NumPlayers--;
				return;
			}
			/* scan his inventory */
			for (i = 0; i < INVEN_TOTAL; i++) {
				o_ptr = &p_ptr->inventory[i];
				if (!o_ptr->k_idx) continue;
				if (o_ptr->quest != q_idx + 1) continue;
				if (individual && o_ptr->owner != p_id) continue;

				s_printf("QUEST_OBJECT_ERASE: savegame '%s'\n", p_ptr->name);
				o_ptr->tval = o_ptr->sval = o_ptr->k_idx = 0;
				o_ptr->questor = o_ptr->quest = 0; //paranoia, just to make sure to prevent questitem_d() check from triggering when deleting it!
				q_info[q_idx].objects_registered--; //..and count down manually
				/* write savegame back */
				save_player(NumPlayers);
				continue;
				/* unhack */
				C_FREE(p_ptr->inventory, INVEN_TOTAL, object_type);
				KILL(p_ptr, player_type);
				NumPlayers--;
				return;
			}
			/* advance to next character */
			ptr = ptr->next;
		}
	}
	/* unhack */
	C_FREE(p_ptr->inventory, INVEN_TOTAL, object_type);
	KILL(p_ptr, player_type);
	NumPlayers--;

#if QDEBUG > 1
	s_printf("OBJECT_QUEST_ERASE: done. (registered: %d)\n", q_info[q_idx].objects_registered);
#endif
}

/* Respawn a previously despawned questor. Has no further consequences. */
static void quest_respawn_questor(int q_idx, int questor_idx) {
#if 0 /* values are retained from when the questor was despawned! */
	q_questor->current_wpos
	q_questor->current_x
	q_questor->current_y
#endif
	/* generate and spawn the questor */
	switch (q_info[q_idx].questor[questor_idx].type) {
	case QI_QUESTOR_NPC:
		if (!questor_monster(q_idx, &q_info[q_idx].questor[questor_idx], questor_idx))
			s_printf("QUEST_RESPAWN_QUESTOR: couldn't respawn NPC.\n");
		break;
	case QI_QUESTOR_ITEM_PICKUP:
		if (!questor_object(q_idx, &q_info[q_idx].questor[questor_idx], questor_idx))
			s_printf("QUEST_RESPAWN_QUESTOR: couldn't respawn ITEM_PICKUP.\n");
		break;
	default: ;
		//TODO: other questor-types
		/* discard */
	}
}

/* Helper function for quest_terminate(): Is a questor still at the original spawning location?
   If the questor has (been) moved, mark as 'tainted'! */
static void quest_verify_questor_location(int q_idx, int questor_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_questor *q_questor;
	cave_type **zcave, *c_ptr;

	/* data reread from q_info[] */
	struct worldpos wpos;

	q_questor = &q_ptr->questor[questor_idx];

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
	switch (q_questor->type) {
	case QI_QUESTOR_NPC:
		if (c_ptr->m_idx != q_questor->mo_idx) q_questor->tainted = TRUE;
		break;
	case QI_QUESTOR_ITEM_PICKUP:
		if (c_ptr->o_idx != q_questor->mo_idx) q_questor->tainted = TRUE;
		break;
	default: ;
		/* discard */
	}
}

/* Despawn a spawned questor. Has no further consequences.
   NOTE: Despawning an object-type questor might fail here, depending on where
         it is (player inventory in a save game etc).
         For this reason, when a quest gets deactivated, quest_erase_objects()
         is called to make sure the items are all gone.
         For stage-dependant questor-despawning however, this function only
         works reliable for monster-type questors! */
static void quest_despawn_questor(int q_idx, int questor_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_questor *q_questor;
	int j, m_idx, o_idx;
	cave_type **zcave, *c_ptr;
	bool fail = FALSE;

	/* data reread from q_info[] */
	struct worldpos wpos;

	q_questor = &q_ptr->questor[questor_idx];

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
	switch (q_questor->type) {
	case QI_QUESTOR_NPC:
#if QDEBUG > 1
		s_printf(" deleting m-questor %d at %d,%d,%d - %d,%d\n", c_ptr->m_idx, wpos.wx, wpos.wy, wpos.wz, q_questor->current_x, q_questor->current_y);
#endif
		m_idx = c_ptr->m_idx;
		if (m_idx == q_questor->mo_idx) {
			if (m_list[m_idx].questor && m_list[m_idx].quest == q_idx) {
#if QDEBUG > 1
				s_printf(" ..ok.\n");
#endif
				delete_monster_idx(c_ptr->m_idx, TRUE);
			} else {
#if QDEBUG > 1
				s_printf(" ..failed: Monster is not a questor or has a different quest idx.\n");
#endif
				fail = TRUE;
			}
		} else if (!m_idx) {
#if QDEBUG > 1
			s_printf(" ..failed: No monster there.\n");
#endif
			fail = TRUE;
		} else {
#if QDEBUG > 1
			s_printf(" ..failed: Monster has different idx than the questor.\n");
#endif
			fail = TRUE;
		}
		/* scan the entire monster list to catch the questor */
		if (fail) {
#if QDEBUG > 1
			s_printf(" Scanning entire monster list..\n");
#endif
			for (j = 1; j < m_max; j++) {
				if (!m_list[j].questor) continue;
				if (m_list[j].quest != q_idx) continue;
				if (m_list[j].questor_idx != questor_idx) continue;
				s_printf(" found it at %d!\n", j);
				delete_monster_idx(j, TRUE);
				//:-p break;
			}
		}
		break;
	case QI_QUESTOR_ITEM_PICKUP:
#if QDEBUG > 1
		s_printf(" deleting o-questor %d at %d,%d,%d - %d,%d\n", c_ptr->o_idx, wpos.wx, wpos.wy, wpos.wz, q_questor->current_x, q_questor->current_y);
#endif
		o_idx = c_ptr->o_idx;
		if (o_idx == q_questor->mo_idx) {
			if (o_list[o_idx].questor && o_list[o_idx].quest - 1 == q_idx) {
#if QDEBUG > 1
				s_printf(" ..ok.\n");
#endif
				o_list[o_idx].questor = o_list[o_idx].quest = 0; //prevent questitem_d() check from triggering when deleting it!..
				q_info[q_idx].objects_registered--; //..and count down manually
				delete_object_idx(c_ptr->o_idx, TRUE);
			} else
#if QDEBUG > 1
				s_printf(" ..failed: Object is not a questor or has a different quest idx.\n");
#endif
				fail = TRUE;
		} else if (!o_idx) {
#if QDEBUG > 1
			s_printf(" ..failed: No object there.\n");
#endif
			fail = TRUE;
		} else {
#if QDEBUG > 1
			s_printf(" ..failed: Object has different idx than the questor.\n");
#endif
			fail = TRUE;
		}
		/* scan the entire object list to catch the questor */
		if (fail)  {
#if QDEBUG > 1
			s_printf(" Scanning entire object list..\n");
#endif
			for (j = 0; j < o_max; j++) {
				if (!o_list[j].questor) continue;
				if (o_list[j].quest != q_idx + 1) continue;
				if (o_list[j].questor_idx != questor_idx) continue;
				s_printf(" found it at %d!\n", j);
				o_list[j].questor = o_list[j].quest = 0; //prevent questitem_d() check from triggering when deleting it!..
				q_info[q_idx].objects_registered--; //..and count down manually
				delete_object_idx(j, TRUE);
				//:-p break;
			}
#if 0 /* keep quest items in inventory. And questors are now unique and undroppable, so n.p.! */
			/* last resort, like for trueart/guildkey erasure, scan everyone's inventory -_- */
			quest_erase_object_questor(q_idx, i);
#endif
		}
		break;
	default: ;
		/* discard */
	}
}

/* Despawn questors, unstatic sectors/floors, etc. */
void quest_deactivate(int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	int i;

	/* data reread from q_info[] */
	struct worldpos wpos;

#if QDEBUG > 0
	s_printf("%s QUEST_DEACTIVATE: '%s' (%d,%s) by %s\n", showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename, q_ptr->creator);
#endif
	q_ptr->active = FALSE;

	/* despawn questors and unstatice their floors */
	for (i = 0; i < q_ptr->questors; i++) {
		quest_despawn_questor(q_idx, i);
		if (q_ptr->questor[i].static_floor) new_players_on_depth(&wpos, 0, FALSE);
	}

#if 0 /* no, allow players to keep the quest in their quest list */
	/* Erase all object questors and quest objects (duh) */
	quest_erase_objects(q_idx, FALSE, 0);
#endif

	/* remove quest dungeons */
	quest_remove_dungeons(q_idx);
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

	for (i = 0; i < MAX_PQUESTS; i++) {
		/* skip this quest (and unused quests, checked in quest_imprint_tracking_information()) */
		if (i == py_q_idx) continue;
		/* expensive mechanism, sort of */
		quest_imprint_tracking_information(Ind, i, TRUE);
	}

	/* clear direct data */
	p_ptr->quest_kill[py_q_idx] = FALSE;
	p_ptr->quest_retrieve[py_q_idx] = FALSE;

	p_ptr->quest_deliver_pos[py_q_idx] = FALSE;
	p_ptr->quest_deliver_xy[py_q_idx] = FALSE;

	/* for 'individual' quests, reset temporary quest data or it might get carried over from previous stage? */
	for (i = 0; i < QI_GOALS; i++) p_ptr->quest_goals[py_q_idx][i] = FALSE;

	/* restore correct target/deliver situational tracker ('..within_target' flags!) */
	quest_check_player_location(Ind);
}

/* Helper function for quest_terminate() */
static void quest_terminate_individual(int Ind, int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int j;

#if QDEBUG > 0
	s_printf("%s QUEST_TERMINATE_INDIVIDUAL: %s(%d) completes '%s'(%d,%s)\n", showtime(), p_ptr->name, Ind, q_name + q_ptr->name, q_idx, q_ptr->codename);
#endif

	for (j = 0; j < MAX_PQUESTS; j++)
		if (p_ptr->quest_idx[j] == q_idx) break;
	if (j == MAX_PQUESTS) return; //oops?

	if (p_ptr->quest_done[q_idx] < 10000) p_ptr->quest_done[q_idx]++;

	/* he is no longer on the quest, since the quest has finished */
	p_ptr->quest_idx[j] = -1;
	//good colours for '***': C-confusion (yay), q-inertia (pft, extended), E-meteor (mh, extended)
	if (q_ptr->auto_accept != 3) {
		msg_format(Ind, "\374\377C***\377u You have completed the quest \"\377U%s\377u\"! \377C***", q_name + q_ptr->name);
		//msg_print(Ind, "\374 ");

#ifdef USE_SOUND_2010
		sound(Ind, "success", NULL, SFX_TYPE_MISC, FALSE);
#endif
	}

	/* erase all of this player's quest items for this quest */
	quest_erase_objects(q_idx, TRUE, p_ptr->id);

	/* set personal quest cooldown */
	if (q_ptr->cooldown == -1) p_ptr->quest_cooldown[q_idx] = QI_COOLDOWN_DEFAULT;
	else p_ptr->quest_cooldown[q_idx] = q_ptr->cooldown;

	/* individual quests don't get cleaned up (aka completely reset)
	   by deactivation, except for this temporary tracking data,
	   or it would continue spamming quest checks eg on delivery_xy locs. */
	quest_initialise_player_tracking(Ind, j);
}
/* a quest has (successfully?) ended, clean up.
   Added wpos for reattaching temporarily global quests (questor moved etc)
   to players within the area, if the quest is actually individual. */
static void quest_terminate(int pInd, int q_idx, struct worldpos *wpos) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, j;

	q_ptr->dirty = TRUE;

	/* give player credit (individual quest) */
	if (q_ptr->individual) {
		if (pInd) {
#if QDEBUG > 0
			s_printf("%s QUEST_TERMINATE (pInd): %s(%d) completes '%s'(%d,%s)\n", showtime(), Players[pInd]->name, pInd, q_name + q_ptr->name, q_idx, q_ptr->codename);
#endif
			/* business as usual */
			quest_terminate_individual(pInd, q_idx);
		} else {
#if QDEBUG > 0
			s_printf("%s QUEST_TERMINATE (!pInd): completing '%s'(%d,%s)\n", showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename);
#endif
			for (i = 1; i <= NumPlayers; i++) {
				if (wpos != NULL && !inarea(&Players[i]->wpos, wpos)) continue;
				quest_terminate_individual(i, q_idx);
			}
		}

		/* If quest has been 'tainted' we need a hard reset like for global quests.. */
		if (q_ptr->tainted) {
			/* don't respawn the questor *immediately* again, looks silyl */
			if (q_ptr->cooldown == -1) q_ptr->cur_cooldown = QI_COOLDOWN_DEFAULT;
			else q_ptr->cur_cooldown = q_ptr->cooldown;

			/* clean up */
			quest_deactivate(q_idx); //includes quest_remove_dungeons()
			return;
		}

		/* remove quest dungeons -- a bit quirky to do this for personal quests,
		   but it might be required, so that the objectives in the dungeon can
		   reset properly. :/ */
		quest_remove_dungeons(q_idx);

		/* check for tainted questors and respawn them, aka soft reset ^^ */
		for (i = 0; i < q_ptr->questors; i++) {
			/* extra check to mark as tainted if (been) moved away from original spawn loc */
			quest_verify_questor_location(q_idx, i);

			if (q_ptr->questor[i].tainted) {
#if QDEBUG > 1
				s_printf("QUEST_TERMINATE (individual): Trying to de+respawn tainted questor %d.\n", i);
#endif
				quest_despawn_questor(q_idx, i);
				/* Note: not respawn, but spawn (complete re-init) */
				if (!quest_spawn_questor(q_idx, i)) {
					//oops?
					s_printf("QUEST_TERMINATE: quest %d, cannot spawn tainted questor %d.\n", q_idx, i);
					quest_deactivate(q_idx);//cold boot -_-
					return;
				}
			}
		}

		/* individual quests don't get cleaned up (aka completely reset)
		   by deactivation, unlike global quests. */
		return;
	}

	/* give players credit (global quest) */
#if QDEBUG > 0
	s_printf("%s QUEST_TERMINATE: (global) '%s'(%d,%s) completed by", showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename);
#endif
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		for (j = 0; j < MAX_PQUESTS; j++)
			if (p_ptr->quest_idx[j] == q_idx) break;
		if (j == MAX_PQUESTS) continue;

#if QDEBUG > 0
		s_printf(" %s(%s)", q_name + q_ptr->name, q_ptr->codename);
#endif
		if (p_ptr->quest_done[q_idx] < 10000) p_ptr->quest_done[q_idx]++;

		/* he is no longer on the quest, since the quest has finished */
		p_ptr->quest_idx[j] = -1;
		if (q_ptr->auto_accept != 3) {
			msg_format(i, "\374\377C***\377u You have completed the quest \"\377U%s\377u\"! \377C***", q_name + q_ptr->name);
			//msg_print(i, "\374 ");

#ifdef USE_SOUND_2010
			sound(i, "success", NULL, SFX_TYPE_MISC, FALSE);
#endif
		}

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
	quest_deactivate(q_idx); //includes quest_remove_dungeons()
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
	for (i = 0; i < MAX_PQUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_PQUESTS) {
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
/* just return the 'nisi' state of a quest goal (for ungoal_r) */
static bool quest_get_goal_nisi(int pInd, int q_idx, int goal) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr;
	int i, stage = quest_get_stage(pInd, q_idx);
	qi_goal *q_goal = &quest_qi_stage(q_idx, stage)->goal[goal];

	if (!pInd || !q_ptr->individual) return q_goal->nisi; /* global quest */

	p_ptr = Players[pInd];
	for (i = 0; i < MAX_PQUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_PQUESTS) return q_goal->nisi;  /* player isn't on this quest. return global goal. */

	if (q_ptr->individual) return p_ptr->quest_goals_nisi[i][goal]; /* individual quest */

	return q_goal->nisi; /* global quest */
}

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
	for (i = 0; i < MAX_PQUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_PQUESTS) return 0; /* player isn't on this quest: pick stage 0, the entry stage */

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
	for (i = 0; i < MAX_PQUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_PQUESTS) return q_ptr->flags; /* player isn't on this quest. return global goal. */

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
	for (i = 0; i < MAX_PQUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_PQUESTS) {
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
	for (i = 0; i < MAX_PQUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_PQUESTS) {
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
	for (i = 0; i < MAX_PQUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	if (i == MAX_PQUESTS) {
		q_goal->cleared = FALSE; /* player isn't on this quest. return global goal. */
		return;
	}

	if (q_ptr->individual) {
		p_ptr->quest_goals[i][goal] = FALSE; /* individual quest */
		return;
	}

	q_goal->cleared = FALSE; /* global quest */
}

/* spawn/hand out special quest items on stage start.
   Note: We don't need pInd/individual checks here, because quest items that
         get handed out will always be given to the player who has the
         questors's 'talk_focus', be it a global or individual quest. */
static void quest_spawn_questitems(int q_idx, int stage) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);
	qi_questitem *q_qitem;
	object_type forge, *o_ptr = &forge;
	int i, j, py;
	char name[MAX_CHARS];//ONAME_LEN?

	struct worldpos wpos;
	s16b x, y, o_idx;
	cave_type **zcave, *c_ptr;

	for (i = 0; i < q_stage->qitems; i++) {
		q_qitem = &q_stage->qitem[i];

		/* generate the object */
		invcopy(o_ptr, lookup_kind(TV_SPECIAL, SV_QUEST));
		o_ptr->pval = q_qitem->opval;
		o_ptr->xtra1 = q_qitem->ochar;
		o_ptr->xtra2 = q_qitem->oattr;
		o_ptr->xtra3 = i;
		o_ptr->xtra4 = TRUE;
		o_ptr->weight = q_qitem->oweight;
		o_ptr->number = 1;
		o_ptr->level = q_qitem->olev;

		o_ptr->quest = q_idx + 1;
		o_ptr->quest_stage = stage;

		/* just have a questor hand it over? */
		if (q_qitem->questor_gives != 255) {
			/* omit despawned-check, w/e */
			/* check for questor's validly focussed player */
			py = q_ptr->questor[q_qitem->questor_gives].talk_focus;
			if (!py) return; /* oops? */
			object_desc(0, name, o_ptr, TRUE, 256);
			msg_format(py, "\374\377GYou have received %s.", name); //for now. This might need some fine tuning
			/* own it */
			o_ptr->owner = Players[py]->id;
			o_ptr->mode = Players[py]->mode;
			o_ptr->iron_trade = Players[py]->iron_trade;
			inven_carry(py, o_ptr);
			q_ptr->objects_registered++;
			continue;
		}

		/* spawn it 80 days away~ */
		if (!quest_special_spawn_location(&wpos, &x, &y, &q_qitem->q_loc, FALSE, FALSE)) {
			s_printf("QUEST_SPAWN_QUESTITEMS: Couldn't obtain spawning location.\n");
			return;
		}

		/* create a new world object from forge */
		o_idx = o_pop();
		if (!o_idx) {
			s_printf("QUEST_SPAWN_QUESTITEMS: No free object index to pop item.\n");
			return;
		}
		o_list[o_idx] = *o_ptr; /* transfer forged object */
		o_ptr = &o_list[o_idx];
		o_ptr->ix = x;
		o_ptr->iy = y;
		wpcopy(&o_ptr->wpos, &wpos);

		wpcopy(&q_qitem->result_wpos, &wpos);
		q_qitem->result_x = x;
		q_qitem->result_y = x;

		/* 'drop' it */
		zcave = quest_prepare_zcave(&wpos, FALSE, q_qitem->q_loc.tpref, q_qitem->q_loc.tpref_x, q_qitem->q_loc.tpref_y, FALSE);

		c_ptr = &zcave[y][x];
		o_ptr->next_o_idx = c_ptr->o_idx; /* on top of pile, if any */
		o_ptr->stack_pos = 0;
		c_ptr->o_idx = o_idx;
		nothing_test2(c_ptr, x, y, &wpos, 4);
		q_ptr->objects_registered++;

		o_ptr->marked = 0;
		o_ptr->held_m_idx = 0;

		/* Clear visibility flags */
		for (j = 1; j <= NumPlayers; j++) Players[j]->obj_vis[o_idx] = FALSE;
		/* Note the spot */
		note_spot_depth(&wpos, y, x);
		/* Draw the spot */
		everyone_lite_spot(&wpos, y, x);
	}
}

/* Change questor visuals, intrinsics and communication when a quest stage starts */
static void quest_questor_morph(int q_idx, int stage, int questor_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);
	qi_questor *q_questor = &q_ptr->questor[questor_idx];
	qi_questor_morph *q_qmorph = q_stage->questor_morph[questor_idx];

	/* just assume that morphing taints. We don't know if the questor gets
	   morphed back to perfectly fine state later, and to check that is
	   more of a hassle than to just respawning him anyway. */
	q_questor->tainted = TRUE;

	q_questor->talkable = q_qmorph->talkable;
	if (q_questor->despawned != q_qmorph->despawned) {
#if QDEBUG > 1
		s_printf("QUEST_QUESTOR_MORPH: %d despawned-change %s,%d\n", questor_idx, q_ptr->codename, q_idx);
#endif
		q_questor->despawned = q_qmorph->despawned;
		if (q_questor->despawned) {
			/* despawn questor */
			quest_despawn_questor(q_idx, questor_idx);
		} else {
			/* respawn questor */
			quest_respawn_questor(q_idx, questor_idx);
		}
	}
	q_questor->invincible = q_qmorph->invincible;
	q_questor->death_fail = q_qmorph->death_fail;
	if (q_qmorph->name) strcpy(q_questor->name, q_qmorph->name);
	if (q_qmorph->ridx) q_questor->ridx = q_qmorph->ridx;
	if (q_qmorph->reidx != -1) q_questor->reidx = q_qmorph->reidx;
	if (q_qmorph->rchar != 127) q_questor->rchar = q_qmorph->rchar;
	if (q_qmorph->rattr != 255) q_questor->rattr = q_qmorph->rattr;
	if (q_qmorph->rlev) m_list[q_questor->mo_idx].level = q_qmorph->rlev;
#if 0
	/* Note the spot */
	note_spot_depth(&q_questor->current_wpos, q_questor->current_y, q_questor->current_x);
	/* Draw the spot */
	everyone_lite_spot(&q_questor->current_wpos, q_questor->current_y, q_questor->current_x);
#else
	/* Note the spot */
	note_spot_depth(&m_list[q_questor->mo_idx].wpos, m_list[q_questor->mo_idx].fy, m_list[q_questor->mo_idx].fx);
	/* Draw the spot */
	everyone_lite_spot(&m_list[q_questor->mo_idx].wpos, m_list[q_questor->mo_idx].fy, m_list[q_questor->mo_idx].fx);
#endif
}
/* Change questor aggressive behaviour towards players or monsters when a stage
   starts. This can revert later on and then trigger a stage change too. */
static void quest_questor_hostility(int q_idx, int stage, int questor_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);
	qi_questor *q_questor = &q_ptr->questor[questor_idx];
	qi_questor_hostility *q_qhost = q_stage->questor_hostility[questor_idx];
	monster_type *m_ptr = &m_list[q_questor->mo_idx];
	monster_race *r_ptr = m_ptr->r_ptr;

	/* questor turns into a regular monster? (irreversible, implies hostile-to-players) */
	if (q_qhost->unquestor) {
		m_ptr->questor = 0;
		m_ptr->quest = 0;
	}

	/* questor turns hostile to players? */
	if (q_qhost->hostile_player) {
		r_ptr->flags7 &= ~(RF7_NO_TARGET | RF7_NEVER_ACT);//| RF7_NO_DEATH; --done in quest_questor_morph() actually
		m_ptr->questor_hostile |= 0x1;
	}

	/* questor turns hostile to monsters? */
	if (q_qhost->hostile_monster)
		m_ptr->questor_hostile |= 0x2;

	/* time hostility? (absolute in-game time) */
	if (q_qhost->hostile_revert_timed_ingame_abs != -1)
		q_qhost->hostile_revert_timed_countdown = -1 - q_qhost->hostile_revert_timed_ingame_abs;

	/* time hostility? (relative real time) */
	if (q_qhost->hostile_revert_timed_real)
		q_qhost->hostile_revert_timed_countdown = q_qhost->hostile_revert_timed_real;
}
/* Have a questor perform actions (possibly onto the player(s) too) or start
   moving when a quest stage starts */
static void quest_questor_act(int pInd, int q_idx, int stage, int questor_idx) {
	int j, k;
	s16b ox, oy, nx = 0, ny = 0;
	quest_info *q_ptr = &q_info[q_idx];
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);
	qi_questor *q_questor = &q_ptr->questor[questor_idx];
	qi_questor_act *q_qact = q_stage->questor_act[questor_idx];
	cave_type **zcave;
	monster_type *m_ptr = &m_list[q_questor->mo_idx];
	player_type *p_ptr;

	if (pInd && q_ptr->individual) {
		/* individual quest */
		p_ptr = Players[pInd];

		/* tele players to new location? */
		if (q_qact->tppy_wpos.wx != -1) {
			p_ptr->recall_pos.wx = q_qact->tppy_wpos.wx;
			p_ptr->recall_pos.wy = q_qact->tppy_wpos.wy;
			p_ptr->recall_pos.wz = q_qact->tppy_wpos.wz;
			//p_ptr->new_level_method = (q_qact->tppy_wpos.wz > 0 ? LEVEL_RECALL_DOWN : LEVEL_RECALL_UP);
			p_ptr->new_level_method = LEVEL_RAND;
			recall_player(pInd, "");
		}

		/* and move him to a specific grid */
		if (q_qact->tppy_x != -1) {
			zcave = getcave(&p_ptr->wpos);
			ox = p_ptr->px;
			oy = p_ptr->py;
			nx = q_qact->tppy_x;
			ny = q_qact->tppy_y;
			p_ptr->py = ny;
			p_ptr->px = nx;
			grid_affects_player(pInd, ox, oy);
			zcave[oy][ox].m_idx = 0;
			zcave[ny][nx].m_idx = 0 - pInd;
			cave_midx_debug(&q_qact->tppy_wpos, ny, nx, -pInd);
			everyone_lite_spot(&q_qact->tppy_wpos, oy, ox);
			everyone_lite_spot(&q_qact->tppy_wpos, ny, nx);
			verify_panel(pInd);
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
			p_ptr->update |= (PU_DISTANCE);
			p_ptr->window |= (PW_OVERHEAD);
			handle_stuff(pInd);
		}
	} else {
		/* global quest - teleport all players who are here and on the quest */
		for (j = 1; j <= NumPlayers; j++) {
			p_ptr = Players[j];
			if (!inarea(&p_ptr->wpos, &m_list[q_questor->mo_idx].wpos)) continue;
			for (k = 0; k < MAX_PQUESTS; k++)
				if (p_ptr->quest_idx[k] == q_idx) break;
			if (k == MAX_PQUESTS) continue;

			/* tele players to new location? */
			if (q_qact->tppy_wpos.wx != -1) {
				p_ptr->recall_pos.wx = q_qact->tppy_wpos.wx;
				p_ptr->recall_pos.wy = q_qact->tppy_wpos.wy;
				p_ptr->recall_pos.wz = q_qact->tppy_wpos.wz;
				//p_ptr->new_level_method = (q_qact->tppy_wpos.wz > 0 ? LEVEL_RECALL_DOWN : LEVEL_RECALL_UP);
				p_ptr->new_level_method = LEVEL_RAND;
				recall_player(j, "");
			}

			/* and move him to a specific grid */
			if (q_qact->tppy_x != -1) {
				zcave = getcave(&p_ptr->wpos);
				oy = p_ptr->py;
				ox = p_ptr->px;
				ny = q_qact->tppy_y;
				nx = q_qact->tppy_x;
				p_ptr->py = ny;
				p_ptr->px = nx;
				grid_affects_player(j, ox, oy);
				zcave[oy][ox].m_idx = 0;
				zcave[ny][nx].m_idx = 0 - j;
				cave_midx_debug(&q_qact->tppy_wpos, ny, nx, -j);
				everyone_lite_spot(&q_qact->tppy_wpos, oy, ox);
				everyone_lite_spot(&q_qact->tppy_wpos, ny, nx);
				verify_panel(j);
				p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
				p_ptr->update |= (PU_DISTANCE);
				p_ptr->window |= (PW_OVERHEAD);
				handle_stuff(j);
			}
		}
	}

	/* tele self to new location? */
	if (q_qact->tp_wpos.wx != -1) {
		/* to wpos */
		/* delete monster visibly from the old wpos */
		if ((zcave = getcave(&m_ptr->wpos))) {
			ox = m_ptr->fx;
			oy = m_ptr->fy;
			zcave[oy][ox].m_idx = 0;
			everyone_lite_spot(&m_ptr->wpos, oy, ox);
		}

		/* place it on the new wpos */
		m_ptr->wpos.wx = q_qact->tp_wpos.wx;
		m_ptr->wpos.wy = q_qact->tp_wpos.wy;
		m_ptr->wpos.wz = q_qact->tp_wpos.wz;
	}
	/* to specific grid */
	if (q_qact->tp_x != -1) {
		nx = q_qact->tp_x;
		ny = q_qact->tp_y;
		m_ptr->fy = ny;
		m_ptr->fx = nx;
	}
	/* finish moving, for both cases */
	if (q_qact->tp_wpos.wx != -1 || q_qact->tp_x != -1) {
		/* place monster visibly on the new wpos */
		if ((zcave = getcave(&m_ptr->wpos))) {
			zcave[ny][nx].m_idx = q_questor->mo_idx;
			cave_midx_debug(&q_qact->tp_wpos, ny, nx, q_questor->mo_idx);
		}
		update_mon(q_questor->mo_idx, TRUE);
		if (zcave) everyone_lite_spot(&q_qact->tp_wpos, ny, nx);

		q_questor->tainted = TRUE; //a location change warrants need for respawn later
	}

	/* walk somewhere? */
	if (q_qact->walk_speed) {
		m_ptr->speed = q_qact->walk_speed;
		m_ptr->destx = q_qact->walk_destx;
		m_ptr->desty = q_qact->walk_desty;

		q_questor->tainted = TRUE; //a location change warrants need for respawn later
	}
}
/* Add a quest-specific dungeon when a dungeon stage starts */
static void quest_add_dungeon(int q_idx, int stage) {
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);
	u32b flags1 = q_stage->dun_flags1, flags2 = q_stage->dun_flags2, flags3 = q_stage->dun_flags3;
	cave_type **zcave;
	int tries = 0;

	if (!quest_special_spawn_location(&q_stage->dun_wpos, &q_stage->dun_x, &q_stage->dun_y, &q_stage->dun_loc, FALSE, FALSE)) {
		s_printf("QUEST_ADD_DUNGEON: Couldn't get spawn location.\n");
		return;
	}

	if (!(zcave = getcave(&q_stage->dun_wpos))) {
		alloc_dungeon_level(&q_stage->dun_wpos);
		zcave = getcave(&q_stage->dun_wpos);
	}
	wilderness_gen(&q_stage->dun_wpos);
	if (q_stage->dun_tower) {
		if (wild_info[q_stage->dun_wpos.wy][q_stage->dun_wpos.wx].tower) {
			s_printf("QUEST_ADD_TOWER: Already exists.\n");
			return;
		}
	} else {
		if (wild_info[q_stage->dun_wpos.wy][q_stage->dun_wpos.wx].dungeon) {
			s_printf("QUEST_ADD_DUNGEON: Already exists.\n");
			return;
		}
	}

	/* add staircase downwards into the dungeon? */
	switch (q_stage->dun_hard) {
	case 0: break;
	case 1: flags1 |= DF1_FORCE_DOWN; break;
	case 2: flags2 |= DF2_IRON; break;
	}
	add_dungeon(&q_stage->dun_wpos, q_stage->dun_base, q_stage->dun_max, flags1, flags2, flags3, q_stage->dun_tower, 0, q_stage->dun_theme, q_idx + 1, stage);

	/* place staircase */
	do {
		q_stage->dun_y = rand_int((MAX_HGT) - 4) + 2;
		q_stage->dun_x = rand_int((MAX_WID) - 4) + 2;
	} while (!cave_floor_bold(zcave, q_stage->dun_y, q_stage->dun_x)
	    && (++tries < 1000));
	zcave[q_stage->dun_y][q_stage->dun_x].feat = q_stage->dun_tower ? FEAT_LESS : FEAT_MORE;
}
/* Remove all quest-specific dungeons (called on quest deactivation) */
static void quest_remove_dungeons(int q_idx) {
	quest_info *q_ptr = &q_info[q_idx];
	int i;

	for (i = 0; i < q_ptr->stages; i++) {
		if (!q_ptr->stage[i].dun_base) continue;
		rem_dungeon(&q_ptr->stage[i].dun_wpos, FALSE);
	}
}
/* Remove the dungeon of a specific quest stage */
static void quest_remove_dungeon(int q_idx, int stage) {
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);

	if (!q_stage->dun_base || q_stage->dun_keep) return;
	rem_dungeon(&q_stage->dun_wpos, FALSE);
}
/* Helper vars for quest_aux(), sigh.. TODO: some other way? ^^ */
static cptr quest_aux_name = NULL;
static char quest_aux_char = 127;
static byte quest_aux_attr = 255;
/* Helper function for quest_mspawn_pick, well more specifically for get_mon_num_hook. */
static bool quest_aux(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags1 & RF1_UNIQUE) return FALSE;
	if (quest_aux_name && !strstr(r_name + r_ptr->name, quest_aux_name)) return FALSE;
	if (quest_aux_attr != 255 && r_ptr->d_attr != quest_aux_attr) return FALSE;
	if (quest_aux_char != 127 && r_ptr->d_char != quest_aux_char) return FALSE;

	return TRUE;
}
/* Helper function for quest_spawn_monsters().
   It picks a specific monster (r_idx) from the specifications. */
static s16b quest_mspawn_pick(qi_monsterspawn *q_mspawn) {
	int lev = q_mspawn->rlevmin + randint(q_mspawn->rlevmax - q_mspawn->rlevmin);

	/* exact spec? */
	if (q_mspawn->ridx) return q_mspawn->ridx;

	quest_aux_name = q_mspawn->name;
	quest_aux_char = q_mspawn->rchar;
	quest_aux_attr = q_mspawn->rattr;

	get_mon_num_hook = quest_aux;
	get_mon_num2_hook = NULL;
	get_mon_num_prep(0, NULL);//reject_uniques done in quest_aux()
	return get_mon_num(lev, lev - 10);//-10 : reduce OOD;
}
/* Spawn monsters on stage startup */
static void quest_spawn_monsters(int q_idx, int stage) {
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);
	qi_monsterspawn *q_mspawn;
	struct worldpos wpos;
	int i, j, tries;
	s16b x, y, r_idx;
	cave_type **zcave;

	for (i = 0; i < q_stage->mspawns; i++) {
		q_mspawn = &q_stage->mspawn[i];

		if (!quest_special_spawn_location(&wpos, &x, &y, &q_mspawn->loc, FALSE, FALSE)) {
			s_printf("QUEST_SPAWN_MONSTERS: Couldn't get spawn location for spawn #%d.\n", i);
			continue;
		}
		if (!(zcave = getcave(&wpos))) {
			s_printf("QUEST_SPAWN_MONSTERS: Couldn't get zcave for spawn #%d.\n", i);
			continue;
		}

		//:ridx:char:attr:minlev:maxlev:partial name to match
		for (j = 1; j <= q_mspawn->amount; j++) {
			if (q_mspawn->scatter) {
				/* Find a legal, distant, unoccupied, space */
				tries = 0;
				while (++tries < 50) {
					y = rand_int(getlevel(&wpos) ? MAX_HGT : MAX_HGT);
					x = rand_int(getlevel(&wpos) ? MAX_WID : MAX_WID);
					if (cave_naked_bold(zcave, y, x)) break;
				}
				if (tries == 50) continue;

				if (!(r_idx = quest_mspawn_pick(q_mspawn))) {
					s_printf("QUEST_MSPAWN_PICK: Monster criteria not matchable for spawn #%d.\n", i);
					continue;
				}
				//todo: implement reidx specification
#ifdef USE_SOUND_2010
				if (summon_specific_race(&wpos, y, x, r_idx, q_mspawn->clones, q_mspawn->groups ? rand_int(5) + 5 : 1))
					sound_near_site(y, x, &wpos, 0, "summon", NULL, SFX_TYPE_MISC, FALSE);
#else
				(void)summon_specific_race(&wpos, y, x, r_idx, q_mspawn->clones, q_mspawn->groups ? rand_int(5) + 5 : 1);
#endif
			} else {
				if (!(r_idx = quest_mspawn_pick(q_mspawn))) {
					s_printf("QUEST_MSPAWN_PICK: Monster criteria not matchable for spawn #%d.\n", i);
					continue;
				}
				//todo: implement reidx specification
#ifdef USE_SOUND_2010
				if (summon_specific_race(&wpos, y, x, r_idx, q_mspawn->clones, q_mspawn->groups ? rand_int(5) + 5 : 1))
					sound_near_site(y, x, &wpos, 0, "summon", NULL, SFX_TYPE_MISC, FALSE);
#else
				(void)summon_specific_race(&wpos, y, x, r_idx, q_mspawn->clones, q_mspawn->groups ? rand_int(5) + 5 : 1);
#endif
			}
		}
	}
}
/* perform automatic things (quest spawn/stage change) in a stage.
   If 'individual_only' is set, only things that do not affect other players
   are done. This was added for reattaching a 'temporarily global' quest stage
   (ie one where questors moved around, things that cannot be done per player
   individually because there aren't personal instances of each questor for
   every player) back to every eligible player nearby. */
static bool quest_stage_automatics(int pInd, int q_idx, int py_q_idx, int stage, bool individual_only) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);

	int i;
	struct worldpos wpos;
	qi_feature *q_feat;

	/* auto-genocide all monsters at a wpos? */
	if (q_stage->geno_wpos.wx != -1)
		wipe_m_list_admin(&q_stage->geno_wpos);

	/* ---------- hijacking this function for questor and dungeon manipulation ---------- */
	if (!individual_only) {
		//first dungeon..
		if (q_stage->dun_base) quest_add_dungeon(q_idx, stage);
		//..then the questor could teleport there ;)
		for (i = 0; i < q_ptr->questors; i++) {
			if (q_stage->questor_morph[i]) quest_questor_morph(q_idx, stage, i);
			if (q_stage->questor_hostility[i]) quest_questor_hostility(q_idx, stage, i);
			if (q_stage->questor_act[i]) quest_questor_act(pInd, q_idx, stage, i);
		}
		//..and monsters get added on top
		quest_spawn_monsters(q_idx, stage);
	}
	/* ---------------------------------------------------------------------------------- */

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

	/* auto-set/clear flags? */
	quest_set_flags(pInd, q_idx, q_stage->setflags, q_stage->clearflags);

	if (!individual_only) {
		/* auto-build cave grid features? */
		for (i = 0; i < q_stage->feats; i++) {
			q_feat = &q_stage->feat[i];
			if (q_feat->wpos_questor != 255)
				wpos = q_ptr->questor[q_feat->wpos_questor].current_wpos;
			else if (q_feat->wpos_questitem != 255)
				wpos = q_stage->qitem[q_feat->wpos_questitem].result_wpos;
			else {
				wpos.wx = q_feat->wpos.wx;
				wpos.wy = q_feat->wpos.wy;
				wpos.wz = q_feat->wpos.wz;
			}
			/* note that this already contains a getcave() check: */
			cave_set_feat_live(&wpos, q_feat->y, q_feat->x, q_feat->feat);
		}
	}

	/* auto-change stage (timed)? */
	if (q_stage->change_stage != 255) {
		/* not a timed change? instant then */
		if (q_stage->timed_ingame_abs == -1 && !q_stage->timed_real) {
#if QDEBUG > 0
			s_printf("%s QUEST_STAGE_AUTO: '%s'(%d,%s) %d->%d\n",
			    showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename, quest_get_stage(pInd, q_idx), q_stage->change_stage);
#endif

			/* quickly, before it's too late! hand out/spawn any special quest items */
			quest_spawn_questitems(q_idx, stage);
			/* ..and hand out auto-rewards too! (aka free rewards) */
			quest_goal_check_reward(pInd, q_idx);

			quest_set_stage(pInd, q_idx, q_stage->change_stage, q_stage->quiet_change, NULL);
			/* don't imprint/play dialogue of this stage anymore, it's gone~ */
			return TRUE;
		}

		/* start the clock */
		if (q_ptr->individual) {
			/*cannot do this, cause quest scheduler is checking once per minute atm
			if (q_stage->timed_ingame) {
				p_ptr->quest_stage_timer[i] = q_stage->timed_ingame;//todo: different resolution than real minutes
			} else */
			if (q_stage->timed_ingame_abs != -1) {
				Players[pInd]->quest_stage_timer[py_q_idx] = -1 - q_stage->timed_ingame_abs;
			} else if (q_stage->timed_real) {
				Players[pInd]->quest_stage_timer[py_q_idx] = q_stage->timed_real;
			}
		} else {
			/*cannot do this, cause quest scheduler is checking once per minute atm
			if (q_stage->timed_ingame) {
				q_ptr->timed_countdown = q_stage->timed_ingame;//todo: different resolution than real minutes
				q_ptr->timed_countdown_stage = q_stage->change_stage;
				q_ptr->timed_countdown_quiet = q_stage->quiet_change;
			} else */
			if (q_stage->timed_ingame_abs != -1) {
				q_ptr->timed_countdown = -1 - q_stage->timed_ingame_abs;
				q_ptr->timed_countdown_stage = q_stage->change_stage;
				q_ptr->timed_countdown_quiet = q_stage->quiet_change;
			} else if (q_stage->timed_real) {
				q_ptr->timed_countdown = q_stage->timed_real;
				q_ptr->timed_countdown_stage = q_stage->change_stage;
				q_ptr->timed_countdown_quiet = q_stage->quiet_change;
			}
		}
	}

	return FALSE;
}
/* Imprint the current stage's tracking information of one of our pursued quests
   into our temporary quest data. This means target-flagging and kill/retrieve counting.
   If 'target_flagging_only' is set, the counters aren't touched. */
static void quest_imprint_tracking_information(int Ind, int py_q_idx, bool target_flagging_only) {
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
			if (!target_flagging_only)
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
			if (!target_flagging_only)
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
}
/* Actually, for retrieval quests, check right away whether the player
   carries any matching items and credit them!
   (Otherwise either dropping them would reduce his counter, or picking
   them up again would wrongly increase it or w/e depending on what
   other approach is taken and how complicated you want it to be.) */
void quest_precheck_retrieval(int Ind, int q_idx, int py_q_idx) {
	player_type *p_ptr = Players[Ind];
	int i;

#if 0	/* done in qcg_kr() anyway */
	quest_info *q_ptr = &q_info[q_idx];
	int stage = quest_get_stage(Ind, q_idx);
	qi_stage *q_stage = quest_qi_stage(Ind, stage);

	for (i = 0; i < q_stage->goals; i++) {
		if (!q_stage->goal[i].retrieve) continue;
		...
	}*/
#endif
	for (i = 0; i < INVEN_PACK; i++) {
		if (!p_ptr->inventory[i].k_idx) continue;
#if 1
		/* Do not allow pre-gathering of quest items?
		   Except for special quest items because those can be handed out
		   by the questor and hence are already in our inventory. */
		if (p_ptr->inventory[i].tval != TV_SPECIAL ||
		    p_ptr->inventory[i].sval != SV_QUEST)
			continue;
#endif
		quest_check_goal_kr(Ind, q_idx, py_q_idx, NULL, &p_ptr->inventory[i]);
	}
}
/* Update our current target location requirements and check them right away.
   This function is to be called right after a stage change.
   Its purpose is to check if the player already carries all required items
   for a retrieval quest and is even at the correct deliver position already.
   A stage change will occur then, induced from the goal-checks called in the
   functions called in this function eventually.
   Without this function, the player would have to..
   -drop the eligible items he's already carrying and pick them up again and..
   -change wpos once, then go back, to be able to deliver.
   Let this function also be known as...badumtsh.. <INSTAEND HACK>! */
static void quest_instacheck_retrieval(int Ind, int q_idx, int py_q_idx) {
	/* Check if our current wpos/location is already an important target
	   for retrieval quests that we just acquired */
	quest_check_player_location(Ind);
	/* If it isn't (for retrieval quests), we're done. */
	if (!Players[Ind]->quest_any_r_target) return;

	/* check for items already in our inventory that fulfil any
	   retrieval goals we just acquired right away */
	quest_precheck_retrieval(Ind, q_idx, py_q_idx); /* sets goals as 'nisi'.. */
	quest_check_goal_deliver(Ind); /* ..turns them in! */
}
/* Store some information of the current stage in the p_ptr array to track his
   progress more easily in the game code:
   -How many kills/items he has to retrieve and
   -whether he is currently within a target location to do so.
   Additionally it checks right away if the player already carries any requested
   items and credits him for them right away. */
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
	quest_imprint_tracking_information(Ind, py_q_idx, FALSE);
}

/* Helper function for quest_set_stage(), to re-individualise quests that just
   finished a timer/walk sequence and hence were 'temporarily global' of some sort.
   'final_player' must be TRUE for the last player that this function gets called
   for, since then some 'global' stage things will be done such as calling the
   stage automatics.
   Returns 'py_q_idx' aka the player's personal quest index. */
static byte quest_set_stage_individual(int Ind, int q_idx, int stage, bool quiet, bool final_player) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);
	player_type *p_ptr = Players[Ind];
	int i, j, k;
	char text[MAX_CHARS * 2];
	qi_goal *q_goal;
	qi_deliver *q_del;

	for (j = 0; j < MAX_PQUESTS; j++)
		if (p_ptr->quest_idx[j] == q_idx) break;
	if (j == MAX_PQUESTS) return j; //paranoia, shouldn't happen

	/* clear any pending RIDs */
	if (p_ptr->request_id >= RID_QUEST && p_ptr->request_id <= RID_QUEST_ACQUIRE + MAX_Q_IDX - 1) {
		Send_request_abort(Ind);
		p_ptr->request_id = RID_NONE;
	}

	/* play automatic narration if any */
	if (!quiet) {
		/* pre-scan narration if any line at all exists and passes the flag check */
		bool anything = FALSE;
		for (k = 0; k < QI_TALK_LINES; k++) {
			if (q_stage->narration[k] &&
			    ((q_stage->narration_flags[k] & quest_get_flags(Ind, q_idx)) == q_stage->narration_flags[k])) {
				anything = TRUE;
				break;
			}
		}
		/* there is narration to display? */
		if (anything) {
			msg_print(Ind, "\374 ");
			msg_format(Ind, "\374\377u<\377U%s\377u>", q_name + q_ptr->name);
			for (k = 0; k < QI_TALK_LINES; k++) {
				if (!q_stage->narration[k]) break;
				if ((q_stage->narration_flags[k] & quest_get_flags(Ind, q_idx)) == q_stage->narration_flags[k]) continue;

#if 0 /* simple way */
				msg_format(Ind, "\374\377U%s", q_stage->narration[k]);
#else /* allow placeholders */
				quest_text_replace(text, q_stage->narration[k], p_ptr);
				msg_format(Ind, "\374\377U%s", text);
#endif
			}
			//msg_print(Ind, "\374 ");
		}
	}

	/* Do a 'minimal' quest_imprint_stage() on the player here!
	   This is important to update his stage, because although quest_stage_automatics()
	   takes the correct current stage as an argument, it will in turn call
	   quest_goal_check_reward() which does NOT take a stage argument and tries to
	   determine the stage from the player! So the player must carry the correct stage. */
	p_ptr->quest_stage[j] = stage;

	/* perform automatic actions (spawn new quest, (timed) further stage change) */
	if (quest_stage_automatics(Ind, q_idx, j, stage, !final_player)) return -1;

	/* update players' quest tracking data */
	quest_imprint_stage(Ind, q_idx, j);

	/* Optionally load map files for goal-target / goal-deliver locations */
	if (final_player)
		for (i = 0; i < q_stage->goals; i++) {
			q_goal = &q_stage->goal[i];
			q_del = q_goal->deliver;
			if (q_goal->target_tpref)
				(void)quest_prepare_zcave(&q_goal->target_wpos, FALSE, q_goal->target_tpref, q_goal->target_tpref_x, q_goal->target_tpref_y, FALSE);
			if (q_del && q_del->tpref)
				(void)quest_prepare_zcave(&q_del->wpos, FALSE, q_del->tpref, q_del->tpref_x, q_del->tpref_y, FALSE);
		}

	/* play questors' stage dialogue */
	if (!quiet)
		for (k = 0; k < q_ptr->questors; k++) {
			if (!inarea(&p_ptr->wpos, &q_ptr->questor[k].current_wpos)) continue;
			quest_dialogue(Ind, q_idx, k, FALSE, FALSE, FALSE);
		}

	/* hand out/spawn any special quest items */
	if (final_player)
		quest_spawn_questitems(q_idx, stage);

	return j;
}
/* Advance quest to a different stage (or start it out if stage is 0).

   Note: Individual quests usually give us a pInd != 0. HOWEVER..
         If the stage was set by timers or questor-walk, ie by completion of
         aspects that is not directly induced by a participating player, then
         we have a pInd of 0, so we have to double-check to be sure.

         Therefore it is especially important, that the 'global' var
         q_ptr->cur_stage is on stage change of an individual quest updated too
         instead of just p_ptr->quest_stage vars, for when a timed/walk event
         is started in that stage.

         Also, while timed/walking, questors shouldn't accept interaction with
         players on different stages of the quest! Basically, a quest that is
         currently in timed/walk state, is sort of 'temporarily global'.

         TODO: The timers could at least be put into p_ptr->quest_timer vars,
         since they do not affect any positioning, unlike actual movement of
         the questors.
         Also, questor-self-teleportation is a problem for individual quests,
         same as questor walking around.

         To solve this matters I added 'wpos' and quest_set_stage_individual():
         wpos is the location where the stage change was triggered, and hence
         can be used to reattach all players present with their individual
         quest stages. */
void quest_set_stage(int pInd, int q_idx, int stage, bool quiet, struct worldpos *wpos) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_stage *q_stage;
	player_type *p_ptr;
	int i, j, k, py_q_idx = -1;
	bool anything;
	char text[MAX_CHARS * 2];
	qi_goal *q_goal;
	qi_deliver *q_del;
	s16b py = 0, py_Ind[MAX_PLAYERS], py_qidx[MAX_PLAYERS];

	int stage_prev = quest_get_stage(pInd, q_idx);

	/* stage randomizer! */
	if (stage < 0) stage = stage_prev + randint(-stage);

#if QDEBUG > 0
	s_printf("%s QUEST_STAGE: '%s'(%d,%s) %d->%d\n", showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename, quest_get_stage(pInd, q_idx), stage);
	s_printf(" actq %d, autoac %d, cstage %d\n", quest_qi_stage(q_idx, stage)->activate_quest, quest_qi_stage(q_idx, stage)->auto_accept, quest_qi_stage(q_idx, stage)->change_stage);
#endif
	/* do some clean-up of remains of the previous stage */
	if (stage_prev != -1) {
		/* unstatice static locations of previous stage, possibly from
		   quest items, targetted goals, deliver goals. */
		q_stage = quest_qi_stage(q_idx, stage_prev);
		for (i = 0; i < q_stage->goals; i++) {
			q_goal = &q_stage->goal[i];
			q_del = q_goal->deliver;
			if (q_goal->target_tpref) new_players_on_depth(&q_goal->target_wpos, -1, TRUE);
			if (q_del && q_del->tpref) new_players_on_depth(&q_del->wpos, -1, TRUE);
			for (j = 0; j < q_stage->qitems; j++)
				if (q_stage->qitem[j].q_loc.tpref) new_players_on_depth(&q_stage->qitem[j].result_wpos, -1, TRUE);
		}

		/* remove dungeons that only lasted for a stage */
		quest_remove_dungeon(q_idx, stage_prev);

		/* clear any timers */
		q_ptr->timed_countdown = 0;
		for (i = 0; i < q_ptr->questors; i++)
			if (q_stage->questor_hostility[i])
				q_stage->questor_hostility[i]->hostile_revert_timed_countdown = 0;
	}


	/* new stage is active.
	   for 'individual' quests this is just a temporary value used by quest_imprint_stage().
	   It is still used for the other stage-checking routines in this function too though:
	    quest_goal_check_reward(), quest_terminate() and the 'anything' check. */
	q_ptr->cur_stage = stage;
	q_ptr->dirty = TRUE;
	q_stage = quest_qi_stage(q_idx, stage);

	/* For non-'individual' quests,
	   if a participating player is around the questor, entering a new stage..
	   - qutomatically invokes the quest dialogue with him again (if any)
	   - store information of the current stage in the p_ptr array,
	     eg the target location for easier lookup */
	if (!q_ptr->individual) {
		for (i = 1; i <= NumPlayers; i++) {
			p_ptr = Players[i];
			p_ptr->quest_eligible = 0;
			for (j = 0; j < MAX_PQUESTS; j++)
				if (p_ptr->quest_idx[j] == q_idx) break;
			if (j == MAX_PQUESTS) continue;
			p_ptr->quest_eligible = j + 1;

			/* clear any pending RIDs */
			if (p_ptr->request_id >= RID_QUEST && p_ptr->request_id <= RID_QUEST_ACQUIRE + MAX_Q_IDX - 1) {
				Send_request_abort(i);
				p_ptr->request_id = RID_NONE;
			}

			/* play automatic narration if any */
			if (!quiet) {
				/* pre-scan narration if any line at all exists and passes the flag check */
				anything = FALSE;
				for (k = 0; k < QI_TALK_LINES; k++) {
					if (q_stage->narration[k] &&
					    ((q_stage->narration_flags[k] & quest_get_flags(0, q_idx)) == q_stage->narration_flags[k])) {
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
						if ((q_stage->narration_flags[k] & quest_get_flags(0, q_idx)) != q_stage->narration_flags[k]) continue;
#if 0 /* simple way */
						msg_format(i, "\374\377U%s", q_stage->narration[k]);
#else /* allow placeholders */
						quest_text_replace(text, q_stage->narration[k], p_ptr);
						msg_format(i, "\374\377U%s", text);
#endif
					}
					//msg_print(i, "\374 ");
				}
			}
		}

		for (i = 1; i <= NumPlayers; i++) {
			if (!Players[i]->quest_eligible) continue;

			/* update player's quest tracking data */
			quest_imprint_stage(i, q_idx, Players[i]->quest_eligible - 1);
		}

		/* Optionally load map files for goal-target / goal-deliver locations */
		for (i = 0; i < q_stage->goals; i++) {
			q_goal = &q_stage->goal[i];
			q_del = q_goal->deliver;
			if (q_goal->target_tpref)
				(void)quest_prepare_zcave(&q_goal->target_wpos, FALSE, q_goal->target_tpref, q_goal->target_tpref_x, q_goal->target_tpref_y, FALSE);
			if (q_del && q_del->tpref)
				(void)quest_prepare_zcave(&q_del->wpos, FALSE, q_del->tpref, q_del->tpref_x, q_del->tpref_y, FALSE);
		}

		/* perform automatic actions (spawn new quest, (timed) further stage change)
		   Note: Should imprint correct stage on player before calling this,
		         otherwise automatic stage change will "skip" a stage in between ^^.
		         This should be rather cosmetic though. */
		if (quest_stage_automatics(0, q_idx, -1, stage, FALSE)) return;

		if (!quiet)
			for (i = 1; i <= NumPlayers; i++) {
				if (!Players[i]->quest_eligible) continue;

				/* play questors' stage dialogue */
				for (k = 0; k < q_ptr->questors; k++) {
					if (wpos == NULL || !inarea(&Players[i]->wpos, &q_ptr->questor[k].current_wpos)) continue;
					quest_dialogue(i, q_idx, k, FALSE, FALSE, FALSE);
				}
			}

		/* hand out/spawn any special quest items */
		quest_spawn_questitems(q_idx, stage);
	} else { /* individual quest */
		/* Do we have to re-individualise the quest,
		   coming from a pseudo-global stage (aka questor walking/teleport)? */
		if (!pInd) {
			/* build list of players eligible to resume */
			for (i = 1; i <= NumPlayers; i++) {
				if (wpos != NULL && !inarea(&Players[i]->wpos, wpos)) continue;

				for (j = 0; j < MAX_PQUESTS; j++)
					if (Players[i]->quest_idx[j] == q_idx) break;
				if (j == MAX_PQUESTS) continue;

				py_Ind[++py - 1] = i;
				py_qidx[py - 1] = j;
			}

			/* reattach player to quest progress */
			for (i = 0; i < py; i++)
				(void)quest_set_stage_individual(py_Ind[i], q_idx, stage, quiet, i == py - 1);

			py_q_idx = -2; //hack: mark
		} else
			/* Business as usual */
			py_q_idx = quest_set_stage_individual(pInd, q_idx, stage, quiet, TRUE);

		/* Stage automatics kicked in and changed stage? */
		if (py_q_idx == -1) return;
	}

	/* check for handing out rewards! (in case they're free) */
	quest_goal_check_reward(pInd, q_idx);


	/* quest termination? */
	if (q_ptr->ending_stage && q_ptr->ending_stage == stage) {
#if QDEBUG > 0
		s_printf("%s QUEST_STAGE_ENDING: '%s'(%d,%s) %d\n", showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename, stage);
#endif
		quest_terminate(pInd, q_idx, wpos);
	}


	/* auto-quest-termination? (actually redundant with ending_stage, just for convenience:)
	   If a stage has no dialogue keywords, or stage goals, or timed/auto stage change
	   effects or questor-movement/tele/revert-from-hostile effects, THEN the quest will end. */
	anything = FALSE;

	for (i = 0; i < q_ptr->questors; i++) {
		/* a questor still has a revert-hostility-timer for stage change pending? */
		if (q_stage->questor_hostility[i] &&
		    q_stage->questor_hostility[i]->hostile_revert_timed_countdown && //redundant- just checking change_stage should suffice
		    q_stage->questor_hostility[i]->change_stage != 255) {
			anything = TRUE;
			break;
		}
		/* a questor still has an act-walk for stage change pending? */
		if (q_stage->questor_act[i] &&
		    q_stage->questor_act[i]->walk_speed && //redundant- just checking change_stage should suffice
		    q_stage->questor_act[i]->change_stage != 255) {
			anything = TRUE;
			break;
		}
	}

	/* optional goals play no role, obviously */
	for (i = 0; i < q_stage->goals; i++)
		if (q_stage->goal[i].kill || q_stage->goal[i].retrieve || q_stage->goal[i].deliver) {
			anything = TRUE;
			break;
		}
	/* now check remaining dialogue options (keywords) */
	for (i = 0; i < q_ptr->keywords; i++)
		if (q_ptr->keyword[i].stage_ok[stage] && /* keyword is available in this stage */
		    q_ptr->keyword[i].stage != 255 && /* and it's not just a keyword that doesn't trigger a stage change? */
		    !q_ptr->keyword[i].any_stage) { /* and it's not valid in ANY stage? that's not sufficient */
			anything = TRUE;
			break;
		}
	/* check auto/timed stage changes */
	if (q_stage->change_stage != 255) anything = TRUE;
	//if (q_ptr->timed_ingame) anything = TRUE;
	if (q_stage->timed_ingame_abs != -1) anything = TRUE;
	if (q_stage->timed_real) anything = TRUE;

	/* really nothing left to do? */
	if (!anything) {
#if QDEBUG > 0
		s_printf("%s QUEST_STAGE_EMPTY: '%s'(%d,%s) %d\n", showtime(), q_name + q_ptr->name, q_idx, q_ptr->codename, stage);
#endif
		quest_terminate(pInd, q_idx, wpos);
	}

#if 1	/* INSTAEND HACK */
	/* hack - we couldn't do this in quest_imprint_stage(), see the note there:
	   check if we already auto-completed the quest stage this quickly just by standing there! */
	if (py_q_idx == -1) { //global quest
		for (i = 1; i <= NumPlayers; i++) {
			for (j = 0; j < MAX_PQUESTS; j++)
				if (Players[i]->quest_idx[j] == q_idx) break;
			if (j == MAX_PQUESTS) continue;

			quest_instacheck_retrieval(i, q_idx, j);
		}
	} else { /* individual quest */
		if (py_q_idx == -2)
			/* reattach all eligible players.. */
			for (i = 0; i < py; i++)
				quest_instacheck_retrieval(py_Ind[i], q_idx, py_qidx[i]);
		else if (py_q_idx != MAX_PQUESTS) //paranoia
			quest_instacheck_retrieval(pInd, q_idx, py_q_idx);
	}
#endif
}

void quest_acquire_confirmed(int Ind, int q_idx, bool quiet) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, j;

	/* specialty: 'local' quest. This goes into a special slot that isn't displayed in /quest list */
	if (q_ptr->local) i = LOCAL_QUEST;
	else {
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
	}

	p_ptr->quest_idx[i] = q_idx;
	strcpy(p_ptr->quest_codename[i], q_ptr->codename);
#if QDEBUG > 1
	s_printf("%s QUEST_ACQUIRED: (%d,%d,%d;%d,%d) %s (%d) has quest '%s'(%d,%s) (slot %d).\n",
	    showtime(), p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, p_ptr->px, p_ptr->py, p_ptr->name, Ind, q_name + q_ptr->name, q_idx, q_ptr->codename, i);
#endif

	/* for 'individual' quests, reset temporary quest data or it might get carried over from previous quest */
	p_ptr->quest_stage[i] = 0; /* note that individual quests can ONLY start in stage 0, obviously */
	p_ptr->quest_flags[i] = 0x0000;
	for (j = 0; j < QI_GOALS; j++) p_ptr->quest_goals[i][j] = FALSE;

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

#if 1 /* INSTAEND HACK */
	/* hack - we couldn't do this in quest_imprint_stage(), see the note there:
	   check if we already auto-completed the quest stage this quickly just by standing there! */
	q_ptr->dirty = FALSE;
	quest_instacheck_retrieval(Ind, q_idx, i);
	/* stage advanced or quest even terminated? */
	//if (q_ptr->cur_stage != 0 || p_ptr->quest_idx[i] == -1) return;
	if (q_ptr->dirty) return;
#endif

	/* re-prompt for keyword input, if any */
	quest_dialogue(Ind, q_idx, p_ptr->interact_questor_idx, TRUE, FALSE, FALSE);
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
	if (q_ptr->local) i = LOCAL_QUEST;
	else {
		for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
			if (p_ptr->quest_idx[i] == -1) break;
		if (i == MAX_CONCURRENT_QUESTS) {
			if (!quiet) msg_print(Ind, qi_msg_max);
			return FALSE;
		}
	}

	/* voila, player may acquire this quest! */
#if QDEBUG > 1
	s_printf("%s QUEST_MAY_ACQUIRE: (%d,%d,%d;%d,%d) %s (%d) may acquire quest '%s'(%d,%s).\n",
	    showtime(), p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz, p_ptr->px, p_ptr->py, p_ptr->name, Ind, q_name + q_ptr->name, q_idx, q_ptr->codename);
#endif
	return TRUE;
}

/* Check if player completed a deliver goal to a questor. */
static void quest_check_goal_deliver_questor(int Ind, int q_idx, int py_q_idx) {
	player_type *p_ptr = Players[Ind];
	int j, k, stage;
	quest_info *q_ptr = &q_info[q_idx];;
	qi_stage *q_stage;
	qi_goal *q_goal;

#if QDEBUG > 3
	s_printf("QUEST_CHECK_GOAL_DELIVER_QUESTOR: by %d,%s - quest (%s,%d) stage %d\n",
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

		/* handle only specific to-questor goals here */
		if (q_goal->deliver->return_to_questor == 255) continue;
#if QDEBUG > 3
		s_printf(" FOUND return-to-questor GOAL %d.\n", j);
#endif

		/* First mark all 'nisi' quest goals as finally resolved,
		   to change flags accordingly if defined by a Z-line.
		   It's important to do this before removing retieved items,
		   just in case UNGOAL will kick in, in inven_item_increase()
		   and unset our quest goal :-p. */
		for (k = 0; k < q_stage->goals; k++)
			if (quest_get_goal(Ind, q_idx, k, TRUE)) {
				quest_set_goal(Ind, q_idx, k, FALSE);
				quest_goal_changes_flags(Ind, q_idx, stage, k);
			}

		/* we have completed a delivery-return goal!
		   We need to un-nisi it here before UNGOAL is called in inven_item_increase(). */
		quest_set_goal(Ind, q_idx, j, FALSE);

		/* for item retrieval goals therefore linked to this deliver goal,
		   remove all quest items now finally that we 'delivered' them.. */
		if (q_ptr->individual) {
			for (k = INVEN_PACK - 1; k >= 0; k--) {
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
}

/* A player interacts with the questor (bumps him if a creature :-p).
   This displays quest talk/narration, turns in goals, or may acquire it.
   The file pointer is for object questors who get examined by the player. */
#define ALWAYS_DISPLAY_QUEST_TEXT
void quest_interact(int Ind, int q_idx, int questor_idx, FILE *fff) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, j, stage = quest_get_stage(Ind, q_idx);
	bool not_acquired_yet = FALSE, may_acquire = FALSE;
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);
	qi_questor *q_questor = &q_ptr->questor[questor_idx];
	qi_goal *q_goal; //for return_to_questor check

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

	/* questor interaction may (automatically) acquire the quest */
	/* has the player not yet acquired this quest? */
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++)
		if (p_ptr->quest_idx[i] == q_idx) break;
	/* nope - test if he's eligible for picking it up!
	   Otherwise, the questor will remain silent for him. */
	if (i == MAX_CONCURRENT_QUESTS) not_acquired_yet = TRUE;

	/* check for deliver quest goals that require to return to a questor */
	if (!not_acquired_yet)
		for (j = 0; j < q_stage->goals; j++) {
			q_goal = &q_stage->goal[j];
			if (!q_goal->deliver || q_goal->deliver->return_to_questor == 255) continue;

			q_ptr->dirty = FALSE;
			quest_check_goal_deliver_questor(Ind, q_idx, j);
			/* check for stage change/termination */
			if (q_ptr->dirty) return;
		}


	/* cannot interact with the questor during this stage? */
	if (!q_questor->talkable) return;

	if (not_acquired_yet) {
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

	/* Special hack - object questor.
	   That means we're in the middle of the item examination screen. Add to it! */
	if (q_questor->type == QI_QUESTOR_ITEM_PICKUP) {
		if (q_stage->talk_examine[questor_idx] > 1) {
			qi_stage *q_stage2 = quest_qi_stage(q_idx, q_stage->talk_examine[questor_idx] - 2);
			if (q_stage2->talk_examine[questor_idx])
				fprintf(fff, " \377GYou notice something about this item! (See your chat window.)\n");
			else /* '>_> */
				fprintf(fff, " \377GIt seems the item itself is speaking to you! (See your chat window.)\n");
		} else {
			if (q_stage->talk_examine[questor_idx])
				fprintf(fff, " \377GYou notice something about this item! (See your chat window.)\n");
			else /* '>_> */
				fprintf(fff, " \377GIt seems the item itself is speaking to you! (See your chat window.)\n");
		}
	}

	/* questor interaction qutomatically invokes the quest dialogue, if any */
	q_questor->talk_focus = Ind; /* only this player can actually respond with keywords -- TODO: ensure this happens for non 'individual' quests only */
	quest_dialogue(Ind, q_idx, questor_idx, FALSE, may_acquire, TRUE);

	/* prompt him to acquire this quest if he hasn't yet */
	if (may_acquire) {
		/* auto-acquire? */
		if (q_ptr->auto_accept) quest_acquire_confirmed(Ind, q_idx, q_ptr->auto_accept >= 2);
		else Send_delayed_request_cfr(Ind, RID_QUEST_ACQUIRE + q_idx, format("Accept the quest \"%s\"?", q_name + q_ptr->name), 1);
	}
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
   caller function quest_interact() will prompt him to acquire the quest first.

   'force_prompt' is set if we're called from quest_interact(), and will always
   give us a keyword-prompt even though there are no valid ones.
   Assumption is that if we're called from quest_interact() we bumped on purpose
   into the questor because we wanted to try out a keyword (as a player, we don't
   know that there maybe are none.. >:D). */
static void quest_dialogue(int Ind, int q_idx, int questor_idx, bool repeat, bool interact_acquire, bool force_prompt) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, k, stage = quest_get_stage(Ind, q_idx);
	qi_stage *q_stage = quest_qi_stage(q_idx, stage), *q_stage_talk = q_stage;
	bool anything, obvious_keyword = FALSE, more_hack = FALSE, yn_hack = FALSE;
	char text[MAX_CHARS * 2];

	/* hack: reuse another stage's dialogue for this stage too? */
	if (q_stage->talk_examine[questor_idx] > 1) q_stage_talk = quest_qi_stage(q_idx, q_stage->talk_examine[questor_idx] - 2);

	if (!repeat) {
		/* pre-scan talk if any line at all passes the flag check */
		anything = FALSE;
		for (k = 0; k < q_stage_talk->talk_lines[questor_idx]; k++) {
			if (q_stage_talk->talk[questor_idx][k] &&
			    ((q_stage_talk->talk_flags[questor_idx][k] & quest_get_flags(Ind, q_idx)) == q_stage_talk->talk_flags[questor_idx][k])) {
				anything = TRUE;
				break;
			}
		}
		/* there is NPC talk to display? */
		if (anything) {
			p_ptr->interact_questor_idx = questor_idx;
			msg_print(Ind, "\374 ");
			if (q_stage_talk->talk_examine[questor_idx])
				msg_format(Ind, "\374\377uYou examine <\377B%s\377u>:", q_ptr->questor[questor_idx].name);
			else
				msg_format(Ind, "\374\377u<\377B%s\377u> speaks to you:", q_ptr->questor[questor_idx].name);
			for (i = 0; i < q_stage_talk->talk_lines[questor_idx]; i++) {
				if (!q_stage_talk->talk[questor_idx][i]) break;
				if ((q_stage_talk->talk_flags[questor_idx][i] & quest_get_flags(Ind, q_idx)) != q_stage_talk->talk_flags[questor_idx][i]) continue;

#if 0 /* simple way */
				msg_format(Ind, "\374\377U%s", q_stage_talk->talk[questor_idx][i]);
#else /* allow placeholders */
				quest_text_replace(text, q_stage_talk->talk[questor_idx][i], p_ptr);
				msg_format(Ind, "\374\377U%s", text);
#endif

				/* lines that contain obvious keywords, ie those marked by '[[..]]',
				   will always force a reply-prompt for convenience! */
				if (strchr(q_stage_talk->talk[questor_idx][i], '\377')) obvious_keyword = TRUE;
			}
			//msg_print(Ind, "\374 ");
		}
		if (obvious_keyword) force_prompt = TRUE;
	} else force_prompt = TRUE; /* repeating what? to talk of course, what else oO -> force_prompt! */

	/* No keyword-interaction possible if we haven't acquired the quest yet. */
	if (interact_acquire) return;

	/* If there are any keywords in this stage, prompt the player for a reply.
	   If the questor is focussed on one player, only he can give a reply,
	   otherwise the first player to reply can advance the stage. */
	if (!q_ptr->individual && q_ptr->questor[questor_idx].talk_focus && q_ptr->questor[questor_idx].talk_focus != Ind) return;

	p_ptr->interact_questor_idx = questor_idx;
	/* check if there's at least 1 keyword available in our stage/from our questor */
	/* NEW: however.. >:) that keyword must be 'obvious'! Or prompt must be forced.
	        We won't disclose 'secret' keywords easily by prompting without any 'apparent'
	        reason for the prompt, and we don't really want to go the route of prompting
	        _everytime_ because it's a bit annoying to have to close the prompt everytime.
	        So we leave it to the player to bump into the questor (->force_prompt) if he
	        wants to try and find out secret keywords.. */

	/* Also we scan here for "~" hack keywords. */
	for (i = 0; i < q_ptr->keywords; i++)
		if (q_ptr->keyword[i].stage_ok[stage] &&
		    q_ptr->keyword[i].questor_ok[questor_idx]
		    && !q_ptr->keyword[i].any_stage/* and it mustn't use a wildcard stage. that's not sufficient. */
		    )
			break;
	if (q_ptr->keyword[i].keyword[0] == 'y' &&
	    q_ptr->keyword[i].keyword[1] == 0)
		yn_hack = TRUE;

	/* ..continue scanning, now for press-SPACEBAR-for-more hack */
	for (; i < q_ptr->keywords; i++)
		if (q_ptr->keyword[i].stage_ok[stage] &&
		    q_ptr->keyword[i].questor_ok[questor_idx]
		    && !q_ptr->keyword[i].any_stage/* and it mustn't use a wildcard stage. that's not sufficient. */
		    && q_ptr->keyword[i].keyword[0] == 0) {
			more_hack = TRUE;
			break;
		}

	if (force_prompt) { /* at least 1 keyword available? */
		/* hack: if 1st keyword is "~", display a "more" prompt */
		if (more_hack)
			Send_delayed_request_str(Ind, RID_QUEST + q_idx, "? (blank for more)> ", "");
		else {
			/* hack: if 1st keyword is "y" just give a yes/no choice instead of an input prompt?
			   we assume that 2nd keyword is a "n" then. */
			if (yn_hack)
				Send_delayed_request_cfr(Ind, RID_QUEST + q_idx, "? (choose yes or no)>", 0);
			else /* normal prompt for keyword input */
				Send_delayed_request_str(Ind, RID_QUEST + q_idx, "?> ", "");
		}

		/* for cancelling the dialogue in case player gets moved away from the questor: */
		p_ptr->questor_dialogue_hack_xy = p_ptr->px + (p_ptr->py << 8);
		p_ptr->questor_dialogue_hack_wpos = p_ptr->wpos.wx + (p_ptr->wpos.wy << 8) + (p_ptr->wpos.wz << 16);
	}
}

/* Custom strcmp() routine for checking input for quest keywords,
   allowing wild cards: '*' */
static bool qstrcmp(char *keyword, char *input) {
	char *ck = keyword, *ci = input, *res, subkeyword[30];

	while (*ck && ci) {
		if (*ck == '*') {
			ck++;

			/* keyword ends on wildcard? done! */
			if (!(*ck)) return FALSE; //matched successfully

			/* do we have to terminate with exact ending (ie no wildcard at the end)? */
			res = strchr(ck, '*');

			/* yes, no further wild card coming up after this */
			if (!res) {
				res = strstr(ci, ck);
				if (!res) return TRUE;
				ci = res + strlen(ck);
				if (*ci) return TRUE;
				return FALSE; //matched successfully
			}
			/* no, we have another wildcard coming up after this */
			else {
				strncpy(subkeyword, ck, res - ck);
				subkeyword[res - ck] = 0;
				ck = res;
				res = strstr(ci, subkeyword);
				if (!res) return TRUE;
				ci = res + strlen(subkeyword);
			}
		} else {
			/* do we have to terminate with exact ending (ie no wildcard at the end)? */
			res = strchr(ck, '*');

			/* yes, no further wild card coming up after this */
			if (!res) {
				if (strlen(ci) > strlen(ck)) return TRUE;
				if (!strncmp(ci, ck, strlen(ck))) return FALSE; //matched successfully
				return TRUE;
			}
			/* no, we have another wildcard coming up after this */
			else {
				strncpy(subkeyword, ck, res - ck);
				subkeyword[res - ck] = 0;
				ck = res;
				if (strncmp(ci, subkeyword, strlen(subkeyword))) return TRUE;
				ci = res + strlen(subkeyword);
			}
		}
	}
	return FALSE; //matched successfully
}

/* Player replied in a questor dialogue by entering a keyword. */
void quest_reply(int Ind, int q_idx, char *str) {
	quest_info *q_ptr = &q_info[q_idx];
	player_type *p_ptr = Players[Ind];
	int i, j, k, stage = quest_get_stage(Ind, q_idx), questor_idx = p_ptr->interact_questor_idx;
	char *c, text[MAX_CHARS * 2], *ct;
	qi_keyword *q_key;
	qi_kwreply *q_kwr;
	int password_hack;
	bool help = FALSE;

	/* if player got teleported/moved (switched places? :-p)
	   away from the questor, stop the dialogue */
	if (p_ptr->questor_dialogue_hack_xy != p_ptr->px + (p_ptr->py << 8)) return;
	if (p_ptr->questor_dialogue_hack_wpos != p_ptr->wpos.wx + (p_ptr->wpos.wy << 8) + (p_ptr->wpos.wz << 16)) return;

	/* check for pure '?': trim leading/trailing spaces */
	while (*str == ' ') str++;
	while (str[strlen(str) - 1] == ' ') str[strlen(str) - 1] = 0;
	/* check for '?': */
	if (!strcmp(str, "?")) help = TRUE;

	/* treat non-alphanum as spaces */
	c = str;
	while (*c) {
		if (!isalpha(*c) && !isdigit(*c) && *c >= 32) //don't accidentally mask ESC character..
			*c = ' ';
		c++;
	}
	/* trim leading/trailing spaces */
	while (*str == ' ') str++;
	while (str[strlen(str) - 1] == ' ') str[strlen(str) - 1] = 0;
	/* reduce multi-spaces */
	c = str;
	ct = text;
	while (*c) {
		if (*c == ' ' && *(c + 1) == ' ') {
			c++;
			continue;
		}
		*ct = *c;
		ct++;
		c++;
	}
	*ct = *c;
	strcpy(str, text);

	/* hack: restore '?' special keyword */
	if (!str[0] && help) strcpy(str, "?");

#if 0
	if ((!str[0] || str[0] == '\e') && !help) return; /* player hit the ESC key.. */
#else /* distinguish RETURN key, to allow empty "" keyword (to 'continue' a really long talk for example) */
	if (str[0] == '\e') {
		msg_print(Ind, " "); //format it up a little bit?
		return; /* player hit the ESC key.. */
	}
#endif

	/* convert input to all lower-case */
	c = str;
	while (*c) {
		*c = tolower(*c);
		c++;
	}

	/* hacks: player obviously wants to get out -- simulate ESC.
	   todo: only allow these words if they're not used as keywords at the same time */
	if (streq(str, "quit") || streq(str, "exit") || streq(str, "leave")) {
		msg_print(Ind, " ");
		return;
	}

	/* echo own reply for convenience */
	msg_print(Ind, "\374 ");
	if (!strcmp(str, "y")) msg_print(Ind, "\374\377u>\377UYes");
	else if (!strcmp(str, "n")) msg_print(Ind, "\374\377u>\377UNo");
	else msg_format(Ind, "\374\377u>\377U%s", str);

	/* default "goodbye" phrases - same as ESC but at least give an answer since they are more polite :D */
	if (streq(str, "bye") || streq(str, "goodbye") || streq(str, "good bye") || streq(str, "farewell") || streq(str, "fare well")
	    || streq(str, "seeyou") || streq(str, "see you") || streq(str, "seeya") || streq(str, "see ya") || streq(str, "take care")) {
		/* questor can actually speak? */
		if (!q_ptr->stage[stage].talk_examine[questor_idx]) {
			msg_print(Ind, "\374 ");
			msg_format(Ind, "\374\377u<\377B%s\377u> speaks to you:", q_ptr->questor[questor_idx].name);
			switch (rand_int(3)) {
			case 0:
				msg_print(Ind, "\374\377UGoodbye.");
				break;
			case 1:
				msg_print(Ind, "\374\377UFarewell.");
				break;
			case 2:
				msg_print(Ind, "\374\377UTake care.");
				break;
			}
		}
		msg_print(Ind, " ");
		return;
	}

	/* scan keywords for match */
	for (i = 0; i < q_ptr->keywords; i++) {
		q_key = &q_ptr->keyword[i];

		if (!q_key->stage_ok[stage] || /* no more keywords? */
		    !q_key->questor_ok[questor_idx]) continue;

		/* check if required flags match to enable this keyword */
		if ((q_key->flags & quest_get_flags(Ind, q_idx)) != q_key->flags) continue;

		/* Scan for $$p randomised password hack */
		if (q_key->keyword[0] == '$' &&
		    q_key->keyword[1] == '$' &&
		    q_key->keyword[2] == 'p') {
			password_hack = q_key->keyword[3] - '0';
			/* boundary checks */
			if (password_hack <= 0 || password_hack > QI_PASSWORDS) continue;
			/* password correct? */
			if (strcmp(q_ptr->password[password_hack - 1], str)) continue;
		}
		/* proceed normally (no password hack) */
		else if (qstrcmp(q_key->keyword, str)) continue; /* not matching? */

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
					if (q_ptr->stage[stage].talk_examine[questor_idx])
						msg_format(Ind, "\374\377uYou examine <\377B%s\377u>:", q_ptr->questor[questor_idx].name);
					else
						msg_format(Ind, "\374\377u<\377B%s\377u> speaks to you:", q_ptr->questor[questor_idx].name);
					/* we can re-use j here ;) */
					for (j = 0; j < q_kwr->lines; j++) {
						if ((q_kwr->replyflags[j] & quest_get_flags(Ind, q_idx)) != q_kwr->replyflags[j]) continue;
#if 0 /* simple way */
						msg_format(Ind, "\374\377U%s", q_kwr->reply[j]);
#else /* allow placeholders */
						quest_text_replace(text, q_kwr->reply[j], p_ptr);
						msg_format(Ind, "\374\377U%s", text);
#endif
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
		if (q_key->stage != 255) {
			quest_set_stage(Ind, q_idx, q_key->stage, FALSE, NULL);
			return;
		}
		/* Instead of return'ing, re-prompt for dialogue
		   if we only changed flags or simply got a keyword-reply. */
		str[0] = 0; /* don't give the 'nothing to say' line, our keyword already matched */
		break;
	}
	/* it was nice small-talking to you, dude */

	/* generic help */
	if (help && str[0]) { //check that there was no '?' keyword implicitely defined in this quest
		msg_print(Ind, "\374 ");
		msg_format(Ind, "\374\377uType any of the keywords that are highlighted in the questor's text,");
		msg_format(Ind, "\374\377uor press ESC key or type \377%cbye\377u to exit this quest dialogue.", QUEST_KEYWORD_HIGHLIGHT);
		str[0] = 0; /* don't give the 'nothing to say' line */
	}

#if 1
	/* if keyword wasn't recognised, repeat input prompt instead of just 'dropping' the convo */
	quest_dialogue(Ind, q_idx, questor_idx, TRUE, FALSE, FALSE);
	/* don't give 'wassup?' style msg if we just hit RETURN.. silyl */
	if (str[0]) {
		msg_print(Ind, "\374 ");
		if (!q_ptr->stage[stage].default_reply[questor_idx]) {
			if (q_ptr->stage[stage].talk_examine[questor_idx])
				msg_format(Ind, "\374\377uThere is no indication about that on <\377B%s\377u>.", q_ptr->questor[questor_idx].name);
			else
				msg_format(Ind, "\374\377u<\377B%s\377u> has nothing to say about that.", q_ptr->questor[questor_idx].name);
		} else {
			char text2[MAX_CHARS * 2];
			bool pasv = TRUE;

			if ((c = strstr(q_ptr->stage[stage].default_reply[questor_idx], "$$Q"))) {
				pasv = FALSE;
				i = (int)(c - q_ptr->stage[stage].default_reply[questor_idx]);
				strncpy(text, q_ptr->stage[stage].default_reply[questor_idx], i);
				text[i] = 0;
				strcat(text, "<\377B");
				strcat(text, q_ptr->questor[questor_idx].name);
				strcat(text, "\377U>");
				strcat(text, c + 3);
			} else if ((c = strstr(q_ptr->stage[stage].default_reply[questor_idx], "$$q"))) {
				i = (int)(c - q_ptr->stage[stage].default_reply[questor_idx]);
				strncpy(text, q_ptr->stage[stage].default_reply[questor_idx], i);
				text[i] = 0;
				strcat(text, "<\377B");
				strcat(text, q_ptr->questor[questor_idx].name);
				strcat(text, "\377u>");
				strcat(text, c + 3);
			} else strcpy(text, q_ptr->stage[stage].default_reply[questor_idx]);

			//if (q_ptr->stage[stage].talk_examine[questor_idx])  --no effect, since it's completely user-defined text now

#if 0 /* simple way */
			msg_format(Ind, "\374\377%c%s", pasv ? 'u' : 'U', text);
#else /* allow placeholders */
			quest_text_replace(text2, text, p_ptr);
			msg_format(Ind, "\374\377%c%s", pasv ? 'u' : 'U', text2);
#endif
		}
		//msg_print(Ind, "\374 ");
	}
#endif
}

/* Test kill quest goal criteria vs an actually killed monster, for a match.
   Main criteria (r_idx vs char+attr+level) are OR'ed.
   (Unlike for retrieve-object matches where they are actually AND'ed.) */
static bool quest_goal_matches_kill(int q_idx, int stage, int goal, monster_type *m_ptr) {
	int i;
	monster_race *r_ptr = race_inf(m_ptr);
	qi_kill *q_kill = quest_qi_stage(q_idx, stage)->goal[goal].kill;
	char mname[MNAME_LEN];

	/* check for race index */
	for (i = 0; i < 10; i++) {
		/* no monster specified? */
		if (q_kill->ridx[i] == 0) continue;
		/* accept any monster? */
		else if (q_kill->ridx[i] == -1) {
			if (q_kill->reidx[i] == -1) return TRUE;
			else if (q_kill->reidx[i] == m_ptr->ego) return TRUE;
		}
		/* specified an r_idx */
		else if (q_kill->ridx[i] == m_ptr->r_idx) {
			if (q_kill->reidx[i] == -1) return TRUE;
			else if (q_kill->reidx[i] == m_ptr->ego) return TRUE;
		}
	}

	/* check for char/attr/level combination - treated in pairs (AND'ed) over the index */
	monster_desc2(mname, m_ptr, 0);
	for (i = 0; i < 5; i++) {
		/* name specified but doesnt match? */
		if (q_kill->name[i] && !strstr(mname, q_kill->name[i])) continue;

		/* no char specified? */
		if (q_kill->rchar[i] == 127) continue;
		 /* accept any char? */
		if (q_kill->rchar[i] != 126 &&
		    /* specified a char? */
		    q_kill->rchar[i] != r_ptr->d_char) continue;

		/* no attr specified? */
		if (q_kill->rattr[i] == 255) continue;//redundant with rchar==127 check
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
	byte attr;
	qi_retrieve *q_ret = quest_qi_stage(q_idx, stage)->goal[goal].retrieve;
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	char oname[ONAME_LEN];

	/* First let's find out the object's attr..which is uggh not so cool (from cave.c).
	   Note that d_attr has the correct get base colour, especially for flavoured items! */
#if 0
	attr = k_ptr->k_attr;
#else /* this is correct */
	attr = k_ptr->d_attr;
#endif
	if (o_ptr->tval == TV_BOOK && is_custom_tome(o_ptr->sval))
		attr = get_book_name_color(o_ptr);
	/* hack: colour of fancy shirts or custom objects can vary  */
	if ((o_ptr->tval == TV_SOFT_ARMOR && o_ptr->sval == SV_SHIRT) ||
	    (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_CUSTOM_OBJECT)) {
		if (!o_ptr->xtra1) o_ptr->xtra1 = (s16b)attr; //wut.. remove this hack? should be superfluous anyway
			attr = (byte)o_ptr->xtra1;
	}
	if (o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_QUEST)
		attr = (byte)o_ptr->xtra2; //^^
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
		if (q_ret->osval[i] == 0) continue;//redundant with otval==0 check
		/* accept any sval? */
		if (q_ret->osval[i] != -1 &&
		    /* specified a sval */
		    q_ret->osval[i] != o_ptr->sval) continue;;

		break;
	}
	if (i == 10) return FALSE;

	/* check for pval/bpval/attr/name1/name2/name2b
	   note: let's treat pval/bpval as minimum values instead of exact values for now.
	   important: object names must not contain inscriptions >;). */
	//object_desc(0, oname, o_ptr, FALSE, 256); //raw text name only?
	object_desc(0, oname, o_ptr, FALSE, 16); //at least mask owner name, or it'd be exploitable again :-p
	for (i = 0; i < 5; i++) {
		/* name specified but doesnt match? */
		if (q_ret->name[i] && !strstr(oname, q_ret->name[i])) continue;

		/* no pval specified? */
		if (q_ret->opval[i] == 9999) continue;
		/* accept any pval? */
		if (q_ret->opval[i] != -9999 &&
		    /* specified a pval? */
		    q_ret->opval[i] < o_ptr->pval) continue;

		/* no bpval specified? */
		if (q_ret->obpval[i] == 9999) continue;//redundant with opval==9999 check
		/* accept any bpval? */
		if (q_ret->obpval[i] != -9999 &&
		    /* specified a bpval? */
		    q_ret->obpval[i] < o_ptr->bpval) continue;

		/* no attr specified? */
		if (q_ret->oattr[i] == 255) continue;//redundant with opval==9999 check
		/* accept any attr? */
		if (q_ret->oattr[i] != 254 &&
		    /* specified a attr? */
		    q_ret->oattr[i] != attr) continue;

		/* no name1 specified? */
		if (q_ret->oname1[i] == -3) continue;//redundant with opval==9999 check
		 /* accept any name1? */
		if (q_ret->oname1[i] != -1 &&
		 /* accept any name1, but MUST be != 0? */
		    (q_ret->oname1[i] != -2 || !o_ptr->name1) &&
		    /* specified a name1? */
		    q_ret->oname1[i] != o_ptr->name1) continue;

		/* no name2 specified? */
		if (q_ret->oname2[i] == -3) continue;//redundant with opval==9999 check
		 /* accept any name2? */
		if (q_ret->oname2[i] != -1 &&
		 /* accept any name2, but MUST be != 0? */
		    (q_ret->oname2[i] != -2 || !o_ptr->name2) &&
		    /* specified a name2? */
		    q_ret->oname2[i] != o_ptr->name2) continue;

		/* no name2b specified? */
		if (q_ret->oname2b[i] == -3) continue;//redundant with opval==9999 check
		 /* accept any name2b? */
		if (q_ret->oname2b[i] != -1 &&
		 /* accept any name2b, but MUST be != 0? */
		    (q_ret->oname2b[i] != -2 || !o_ptr->name2b) &&
		    /* specified a name2b? */
		    q_ret->oname2b[i] != o_ptr->name2b) continue;

		break;
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
	bool nisi, was_credited;
	qi_stage *q_stage;
	qi_goal *q_goal;

	/* quest is deactivated? */
	if (!q_ptr->active) return;

	stage = quest_get_stage(Ind, q_idx);
	q_stage = quest_qi_stage(q_idx, stage);

	/* Exception for retrieved items. Check if an item that is
	   1) already owned or
	   2) 'found' at the wrong target location
	   originally WAS a valid quest item, then it is still valid! */
	was_credited = (o_ptr && (o_ptr->quest == q_idx + 1 && o_ptr->quest_stage == stage));

	/* For handling Z-lines: flags changed depending on goals completed:
	   pre-check if we have any pending deliver goal in this stage.
	   If so then we can only set the quest goal 'nisi' (provisorily),
	   and hence flags won't get changed yet until it is eventually resolved
	   when we turn in the delivery.
	   Now also used for limiting/fixing ungoal_r: only nisi-goals can be unset. */
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
		s_printf(" FOUND kr GOAL %d (k=%d,r=%d).\n", j, q_goal->kill ? TRUE : FALSE, q_goal->retrieve ? TRUE : FALSE);
#endif

		/* location-restricted?
		   Exempt already retrieved items that were just lost temporarily on the way! */
		if (q_goal->target_pos && !was_credited) {
			/* extend target terrain over a wide patch? */
			if (q_goal->target_terrain_patch) {
				/* different z-coordinate = instant fail */
				if (p_ptr->wpos.wz != q_goal->target_wpos.wz) continue;
				/* are we within range and have same terrain type? */
				if (distance(p_ptr->wpos.wy, p_ptr->wpos.wx,
				    q_goal->target_wpos.wy, q_goal->target_wpos.wx) > QI_TERRAIN_PATCH_RADIUS ||
				    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type !=
				    wild_info[q_goal->target_wpos.wy][q_goal->target_wpos.wx].type)
					continue;
			}
			/* just check a single, specific wpos? */
			else if (!inarea(&q_goal->target_wpos, &p_ptr->wpos)) continue;

			/* check for exact x,y location? */
			if (q_goal->target_pos_x != -1 &&
			    distance(q_goal->target_pos_y, q_goal->target_pos_x, p_ptr->py, p_ptr->px) > q_goal->target_pos_radius)
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
#if QDEBUG > 2
			s_printf("  COUNTED_k down.\n");
#endif
			p_ptr->quest_kill_number[py_q_idx][j]--;
			if (p_ptr->quest_kill_number[py_q_idx][j]) continue; /* not yet */

			/* we have completed a target-to-xy goal! */
			quest_set_goal(Ind, q_idx, j, nisi);
			continue;
		}

	///TODO: implement for global quests too!
		/* check for retrieve-item goal here */
		if (o_ptr && q_goal->retrieve) {
			/* All retrieval items must be unowned so you can't just
			   keep stacks of them for each quest. */
			/* Allow owned items if they're already marked as quest items! */
			if (!q_goal->retrieve->allow_owned && o_ptr->owner && (!was_credited ||
			    (q_ptr->individual == 2 && o_ptr->owner != p_ptr->id)))
				continue;

			if (!quest_goal_matches_object(q_idx, stage, j, o_ptr)) continue;

			/* discard (old) items from another quest or quest stage that just look similar!
			   Those cannot simply be reused. */
			if (o_ptr->quest && (o_ptr->quest != q_idx + 1 || o_ptr->quest_stage != stage)) continue;

			/* mark the item as quest item, so we know we found it at the designated target loc (if any) */
			o_ptr->quest = q_idx + 1;
			o_ptr->quest_stage = stage;

			/* If quest is strictly individual ('solo'), make sure we cannot
			   give the quest items to someone else for him getting credit. */
			if (q_ptr->individual == 2) o_ptr->level = 0;

			/* decrease the player's retrieve counter, if we got all, goal is completed! */
#if QDEBUG > 2
			s_printf("  COUNTED_r down by %d.\n", o_ptr->number);
#endif
			p_ptr->quest_retrieve_number[py_q_idx][j] -= o_ptr->number;
			if (p_ptr->quest_retrieve_number[py_q_idx][j] > 0) continue; /* not yet */

			/* we have completed a target-to-xy goal! */
			quest_set_goal(Ind, q_idx, j, nisi);

			/* if we don't have to deliver in this stage,
			   we can just remove all the quest items right away now,
			   since they've fulfilled their purpose of setting the quest goal. */
			for (k = 0; k < q_stage->goals; k++)
				if (q_stage->goal[k].deliver) {
#if QDEBUG > 2
					s_printf(" CANNOT REMOVE RETRIEVED ITEMS: awaiting delivery.\n");
#endif
					break;
				}
			if (k == q_stage->goals) {
#if QDEBUG > 2
				s_printf(" REMOVE RETRIEVED ITEMS.\n");
#endif
				if (q_ptr->individual) {
					for (k = INVEN_PACK - 1; k >= 0; k--) {
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
	player_type *p_ptr = Players[Ind], *p2_ptr;
	int i, j, k;

#if QDEBUG > 3
	s_printf("QUEST_CHECK_GOAL_k: by %d,%s\n", Ind, p_ptr->name);
#endif
	for (i = 0; i < MAX_PQUESTS; i++) {
#if 0
		/* player actually pursuing a quest? -- paranoia (quest_kill should be FALSE then) */
		if (p_ptr->quest_idx[i] == -1) continue;
#endif
		/* reduce workload */
		if (!p_ptr->quest_kill[i]) continue;

		quest_check_goal_kr(Ind, p_ptr->quest_idx[i], i, m_ptr, NULL);

		/* if it's a party-individual quest, also credit all present
		   party mebers who are on this quest & stage too. */
		if (q_info[p_ptr->quest_idx[i]].individual == 1 && p_ptr->party)
			for (j = 1; j <= NumPlayers; j++) {
				p2_ptr = Players[j];
				/* must be in the same sector and in the same party */
				if (!inarea(&p_ptr->wpos, &p2_ptr->wpos)) continue;
				if (p_ptr->party != p2_ptr->party) continue;
				/* only give credit for exactly this quest, even if the monster
				   matches other quests of his too. This is covered by the first
				   for loop over the original player's further quests. */
				if (!p2_ptr->quest_any_k_within_target) continue;//uh, efficiency maybe? (this check is redundant with next for loop)
				for (k = 0; k < MAX_PQUESTS; k++)
					if (p_ptr->quest_idx[i] == p2_ptr->quest_idx[k]) {
						quest_check_goal_kr(j, p2_ptr->quest_idx[k], k, m_ptr, NULL);
						break;
					}
			}
	}
}
/* Small intermediate function to reduce workload.. (2. for r-goals) */
void quest_check_goal_r(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];
	int i;

#if QDEBUG > 3
	s_printf("QUEST_CHECK_GOAL_r: by %d,%s\n", Ind, p_ptr->name);
#endif
	for (i = 0; i < MAX_PQUESTS; i++) {
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

	/* Only check items that have already been credited in some way.
	   (Just efficiency. It's redundant with detailed checks below.) */
	if (!o_ptr->quest) return;

#if QDEBUG > 3
	s_printf("QUEST_CHECK_UNGOAL_r: by %d,%s\n", Ind, p_ptr->name);
#endif
	for (i = 0; i < MAX_PQUESTS; i++) {
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

		/* only check items that have previously been credited
		   in this quest and stage! */
		if (o_ptr->quest - 1 != q_idx || o_ptr->quest_stage != stage) continue;

#if QDEBUG > 2
		s_printf(" (UNGOAL) CHECKING FOR QUEST (%s,%d) stage %d.\n", q_ptr->codename, q_idx, stage);
#endif

		/* check the quest goals, whether any of them wants a retrieval */
		for (j = 0; j < q_stage->goals; j++) {
			/* no r goal? */
			if (!q_stage->goal[j].retrieve) continue;

			/* only nisi-goals: we can lose quest items on our way
			   to deliver them, but if we don't have to deliver
			   them and we already collected all, then the goal is
			   set in stone...uh or cleared in stone? */
			if (!quest_get_goal_nisi(Ind, q_idx, j)) continue;

			/* phew, item has nothing to do with this quest goal? */
			if (!quest_goal_matches_object(q_idx, stage, j, o_ptr)) continue;
#if QDEBUG > 2
			s_printf(" (UNGOAL) FOUND r GOAL %d by %d,%s\n", j, Ind, p_ptr->name);
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
			s_printf(" --FOUND GOAL %d (k%d,m%d)..", j, q_goal->kill->number, q_goal->retrieve->number);
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
			if (distance(p_ptr->wpos.wy, p_ptr->wpos.wx,
			    q_del->wpos.wy, q_del->wpos.wx) > QI_TERRAIN_PATCH_RADIUS ||
			    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type !=
			    wild_info[q_del->wpos.wy][q_del->wpos.wx].type)
				continue;
		}
		/* just check a single, specific wpos? */
		else if (!inarea(&q_del->wpos, &p_ptr->wpos)) continue;
#if QDEBUG > 2
		s_printf(" (DELIVER) PASSED LOCATION CHECK.\n");
#endif

		/* First mark all 'nisi' quest goals as finally resolved,
		   to change flags accordingly if defined by a Z-line.
		   It's important to do this before removing retieved items,
		   just in case UNGOAL will kick in, in inven_item_increase()
		   and unset our quest goal :-p. */
		for (k = 0; k < q_stage->goals; k++)
			if (quest_get_goal(Ind, q_idx, k, TRUE)) {
				quest_set_goal(Ind, q_idx, k, FALSE);
				quest_goal_changes_flags(Ind, q_idx, stage, k);
			}

		/* we have completed a delivery-to-wpos goal!
		   We need to un-nisi it here before UNGOAL is called in inven_item_increase(). */
		quest_set_goal(Ind, q_idx, j, FALSE);

		/* for item retrieval goals therefore linked to this deliver goal,
		   remove all quest items now finally that we 'delivered' them.. */
		if (q_ptr->individual) {
			for (k = INVEN_PACK - 1; k >= 0; k--) {
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
}
/* Check if player completed a deliver goal to a wpos and specific x,y. */
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
			if (distance(p_ptr->wpos.wy, p_ptr->wpos.wx,
			    q_del->wpos.wy, q_del->wpos.wx) > QI_TERRAIN_PATCH_RADIUS ||
			    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].type !=
			    wild_info[q_del->wpos.wy][q_del->wpos.wx].type)
				continue;
		}
		/* just check a single, specific wpos? */
		else if (!inarea(&q_del->wpos, &p_ptr->wpos)) continue;

		/* check for exact x,y location, plus radius */
		if (distance(q_del->pos_y, q_del->pos_x, p_ptr->py, p_ptr->px) > q_del->radius)
			continue;
#if QDEBUG > 2
		s_printf(" (DELIVER_XY) PASSED LOCATION CHECK.\n");
#endif

		/* First mark all 'nisi' quest goals as finally resolved,
		   to change flags accordingly if defined by a Z-line.
		   It's important to do this before removing retieved items,
		   just in case UNGOAL will kick in, in inven_item_increase()
		   and unset our quest goal :-p. */
		for (k = 0; k < q_stage->goals; k++)
			if (quest_get_goal(Ind, q_idx, k, TRUE)) {
				quest_set_goal(Ind, q_idx, k, FALSE);
				quest_goal_changes_flags(Ind, q_idx, stage, k);
			}

		/* we have completed a delivery-to-xy goal!
		   We need to un-nisi it here before UNGOAL is called in inven_item_increase(). */
		quest_set_goal(Ind, q_idx, j, FALSE);

		/* for item retrieval goals therefore linked to this deliver goal,
		   remove all quest items now finally that we 'delivered' them.. */
		if (q_ptr->individual) {
			for (k = INVEN_PACK - 1; k >= 0; k--) {
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
}
/* Small intermediate function to reduce workload.. (3. for deliver-goals) */
void quest_check_goal_deliver(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i;

	for (i = 0; i < MAX_PQUESTS; i++) {
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
		if (q_stage->next_stage_from_goals[j] == 255) continue;

		/* scan through all goals required to be fulfilled to enter this stage */
		for (i = 0; i < QI_STAGE_GOALS; i++) {
			if (q_stage->goals_for_stage[j][i] == -1) continue;

			/* If even one goal is missing, we cannot advance. */
			if (!quest_get_goal(pInd, q_idx, q_stage->goals_for_stage[j][i], FALSE)) break;
		}
		/* we may proceed to another stage? */
		if (i == QI_STAGE_GOALS) return q_stage->next_stage_from_goals[j];
	}
	return 255; /* goals are not complete yet */
}

/* Apply status effect(s) to a specific player.
   This is an attempt at an interesting hack:
   We take a k_info.txt N-index and apply that item directly to the player!
   Eligible items are: Potions, scrolls, staves, NON-DIRECTIONAL rods.
   (Maybe in the future rings, amulets or other items too.)
   The non-consumable item types are instead attempted to be activated. */
//static
void quest_statuseffect(int Ind, int fx) {
	int k_idx = k_info_num[fx];
	int tv = k_info[k_idx].tval, sv = k_info[k_idx].sval;
	bool dummy;

	/* special effects, not in k_info */
	if (fx < 0) switch (-fx) {
	case 1: /* luck, short */
		bless_temp_luck(Ind, 1, 1000);
		return;
	case 2: /* luck, medium */
		bless_temp_luck(Ind, 1, 2000);
		return;
	case 3: /* luck, long */
		bless_temp_luck(Ind, 1, 3000);
		return;
	default:
		s_printf("Warning: Unusable quest status effect %d.\n", fx);
		return;
	}

	if (tv == TV_FOOD) (void)eat_food(Ind, sv, NULL, &dummy);
	else if (tv == TV_POTION || tv == TV_POTION2) (void)quaff_potion(Ind, tv, sv, -257);
	else if (tv == TV_SCROLL) (void)read_scroll(Ind, tv, sv, NULL, 0, &dummy, &dummy);
	else if (tv == TV_ROD) (void)zap_rod(Ind, sv, DEFAULT_RADIUS, NULL, &dummy);
	else if (tv == TV_STAFF) (void)use_staff(Ind, sv, DEFAULT_RADIUS, FALSE, &dummy);
	//not implemented--
	//else if ((k_info[k_idx].flags3 & TR3_ACTIVATE)) (void)activate_item(Ind, tv, sv, DEFAULT_RADIUS, FALSE);
	else s_printf("Warning: Unusable quest status effect %d.\n", fx);
}

/* hand out a reward object to player (if individual quest)
   or to all involved players in the area (if non-individual quest) */
static void quest_reward_object(int pInd, int q_idx, object_type *o_ptr) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, j;

	if (pInd && q_ptr->individual) { //we should never get an individual quest without a pInd here..
		o_ptr->iron_trade = Players[pInd]->iron_trade;
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
		for (j = 0; j < MAX_PQUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_PQUESTS) continue;

		/* must be around a questor to receive the rewards,
		   so you can't afk-quest in a pack of people with one person doing it all. */
		for (j = 0; j < q_ptr->questors; j++)
			if (inarea(&Players[i]->wpos, &q_ptr->questor[j].current_wpos)) break;
		if (j == q_ptr->questors) continue;

		/* hand him out the reward too */
		o_ptr->iron_trade = Players[i]->iron_trade;
		inven_carry(i, o_ptr);
	}
}

/* hand out a reward object created by create_reward() to player (if individual quest)
   or to all involved players in the area (if non-individual quest) */
static void quest_reward_create(int pInd, int q_idx, u32b resf) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, j;

	if (pInd && q_ptr->individual) { //we should never get an individual quest without a pInd here..
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
		for (j = 0; j < MAX_PQUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_PQUESTS) continue;

		/* must be around a questor to receive the rewards,
		   so you can't afk-quest in a pack of people with one person doing it all. */
		for (j = 0; j < q_ptr->questors; j++)
			if (inarea(&Players[i]->wpos, &q_ptr->questor[j].current_wpos)) break;
		if (j == q_ptr->questors) continue;

		/* hand him out the reward too */
		give_reward(i, resf, q_name + q_ptr->name, 0, 0);
	}
}

/* hand out gold to player (if individual quest)
   or to all involved players in the area (if non-individual quest) */
static void quest_reward_gold(int pInd, int q_idx, int au) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, j;

	if (pInd && q_ptr->individual) { //we should never get an individual quest without a pInd here..
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
		for (j = 0; j < MAX_PQUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_PQUESTS) continue;

		/* must be around a questor to receive the rewards,
		   so you can't afk-quest in a pack of people with one person doing it all. */
		for (j = 0; j < q_ptr->questors; j++)
			if (inarea(&Players[i]->wpos, &q_ptr->questor[j].current_wpos)) break;
		if (j == q_ptr->questors) continue;

		/* hand him out the reward too */
		gain_au(i, au, FALSE, FALSE);
	}
}

/* provide experience to player (if individual quest)
   or to all involved players in the area (if non-individual quest) */
static void quest_reward_exp(int pInd, int q_idx, int exp) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, j;

	if (pInd && q_ptr->individual) { //we should never get an individual quest without a pInd here..
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
		for (j = 0; j < MAX_PQUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_PQUESTS) continue;

		/* must be around a questor to receive the rewards,
		   so you can't afk-quest in a pack of people with one person doing it all. */
		for (j = 0; j < q_ptr->questors; j++)
			if (inarea(&Players[i]->wpos, &q_ptr->questor[j].current_wpos)) break;
		if (j == q_ptr->questors) continue;

		/* hand him out the reward too */
		gain_exp(i, exp);
	}
}

/* provide status effect to player (if individual quest)
   or to all involved players in the area (if non-individual quest) */
static void quest_reward_statuseffect(int pInd, int q_idx, int fx) {
	quest_info *q_ptr = &q_info[q_idx];
	int i, j;

	if (pInd && q_ptr->individual) { //we should never get an individual quest without a pInd here..
		quest_statuseffect(pInd, fx);
		return;
	}

	if (!q_ptr->individual) {
		s_printf(" QUEST_REWARD_STATUSEFFECT: not individual, but no pInd either.\n");
		return;//catch paranoid bugs--maybe obsolete
	}

	/* global quest (or for some reason missing pInd..<-paranoia)  */
	for (i = 1; i <= NumPlayers; i++) {
		/* is the player on this quest? */
		for (j = 0; j < MAX_PQUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_PQUESTS) continue;

		/* must be around a questor to receive the rewards,
		   so you can't afk-quest in a pack of people with one person doing it all. */
		for (j = 0; j < q_ptr->questors; j++)
			if (inarea(&Players[i]->wpos, &q_ptr->questor[j].current_wpos)) break;
		if (j == q_ptr->questors) continue;

		/* hand him out the reward too */
		quest_statuseffect(i, fx);
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
	struct worldpos wpos = { cfg.town_x, cfg.town_y, 0};

	/* Since the rewards are fixed, independantly of where the questors are,
	   we can just pick either one questor at random to use his wpos or just
	   use an arbitrarily set wpos for apply_magic() calls further down. */
	if (q_ptr->questors) { /* extreme paranoia - could be false with auto-acquired quests? */
#if 0
		for (j = 0; j < q_ptr->questors; j++) {
			q_questor = &q_ptr->questor[j];
			if (q_questor->despawned) continue;
			/* just pick the first good questor for now.. w/e */
			wpos = q_questor->current_wpos;
			break;
		}
#else
		/* just pick the first questor for now.. w/e */
		q_questor = &q_ptr->questor[0];
		wpos = q_questor->current_wpos;
#endif
	}

#if 0 /* we're called when stage 0 starts too, and maybe it's some sort of globally determined goal->reward? */
	if (!pInd || !q_ptr->individual) {
		s_printf(" QUEST_GOAL_CHECK_REWARD: returned! oops\n");
		return; //paranoia?
	}
#endif

	/* scan through all rewards that may be handed out in this stage */
	for (j = 0; j < q_stage->rewards; j++) {
		q_rew = &q_stage->reward[j];

		/* no reward? */
		if (!q_rew->otval &&
		    !q_rew->oreward &&
		    !q_rew->gold &&
		    !q_rew->exp &&
		    !q_rew->statuseffect)
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
				if (!q_rew->ogood && !q_rew->ogreat && !q_rew->ovgreat) {
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
					apply_magic(&wpos, o_ptr, -2, FALSE, FALSE, FALSE, FALSE, resf);
					o_ptr->pval = q_rew->opval;
					o_ptr->bpval = q_rew->obpval;
					o_ptr->note = quark_add(format("%s", q_name + q_ptr->name));
					o_ptr->note_utag = 0;
#ifdef PRE_OWN_DROP_CHOSEN
					o_ptr->level = 0;
					if (pInd) {
						o_ptr->owner = Players[pInd]->id;
						o_ptr->mode = Players[pInd]->mode;
						o_ptr->iron_trade = Players[pInd]->iron_trade;
						if (true_artifact_p(o_ptr)) determine_artifact_timeout(o_ptr->name1, &wpos);
					}
#endif
				} else {
					o_ptr = &forge;
					object_wipe(o_ptr);
					invcopy(o_ptr, lookup_kind(q_rew->otval, q_rew->osval));
					o_ptr->number = 1;
					apply_magic(&wpos, o_ptr, -2, q_rew->ogood, q_rew->ogreat, q_rew->ovgreat, FALSE, resf);
					o_ptr->note = quark_add(format("%s", q_name + q_ptr->name));
					o_ptr->note_utag = 0;
#ifdef PRE_OWN_DROP_CHOSEN
					o_ptr->level = 0;
					if (pInd) {
						o_ptr->owner = Players[pInd]->id;
						o_ptr->mode = Players[pInd]->mode;
						o_ptr->iron_trade = Players[pInd]->iron_trade;
						if (true_artifact_p(o_ptr)) determine_artifact_timeout(o_ptr->name1, &wpos);
					}
#endif
				}

				/* hand it out */
				quest_reward_object(pInd, q_idx, o_ptr);
				r_obj++;
			}
			/* instead use create_reward() like for events? */
			else if (q_rew->oreward) {
				switch (q_rew->oreward) {
				case 1: resf |= RESF_LOW; break;
				case 2: resf |= RESF_LOW2; break;
				case 3: resf |= RESF_MID; break;
				case 4: resf |= RESF_HIGH; break;
				case 5: break; /* 'allow randarts' */
				}
				quest_reward_create(pInd, q_idx, resf);
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
			if (q_rew->statuseffect)
				quest_reward_statuseffect(pInd, q_idx, q_rew->statuseffect);
		}
	}

	/* give one unified message per reward type that was handed out */
	if (pInd && q_ptr->individual) {
		if (r_obj == 1) msg_print(pInd, "You have received an item.");
		else if (r_obj) msg_format(pInd, "You have received %d items.", r_obj);
		if (r_gold) msg_format(pInd, "You have received %d gold piece%s.", r_gold, r_gold == 1 ? "" : "s");
		if (r_exp) msg_format(pInd, "You have received %d experience point%s.", r_exp, r_exp == 1 ? "" : "s");
#ifdef USE_SOUND_2010
		if (r_obj || r_gold || r_exp) sound(pInd, "success", NULL, SFX_TYPE_MISC, FALSE);
#endif
	} else for (i = 1; i <= NumPlayers; i++) {
		/* is the player on this quest? */
		for (j = 0; j < MAX_PQUESTS; j++)
			if (Players[i]->quest_idx[j] == q_idx) break;
		if (j == MAX_PQUESTS) continue;

		/* must be around a questor to receive the rewards,
		   so you can't afk-quest in a pack of people with one person doing it all. */
		for (j = 0; j < q_ptr->questors; j++)
			if (inarea(&Players[i]->wpos, &q_ptr->questor[j].current_wpos)) break;
		if (j == q_ptr->questors) continue;

		if (r_obj == 1) msg_print(i, "You have received an item.");
		else if (r_obj) msg_format(i, "You have received %d items.", r_obj);
		if (r_gold) msg_format(i, "You have received %d gold piece%s.", r_gold, r_gold == 1 ? "" : "s");
		if (r_exp) msg_format(i, "You have received %d experience point%s.", r_exp, r_exp == 1 ? "" : "s");

#ifdef USE_SOUND_2010
		if (r_obj || r_gold || r_exp) sound(i, "success", NULL, SFX_TYPE_MISC, FALSE);
#endif
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
	if (stage == 255) return FALSE; /* stage not yet completed */

	/* check goals for rewards */
	quest_goal_check_reward(pInd, q_idx);

	/* advance stage! */
	quest_set_stage(pInd, q_idx, stage, FALSE, NULL);
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
	for (i = 0; i < MAX_PQUESTS; i++) {
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
				/* skip 'to-questor' deliveries */
				if (q_del->return_to_questor != 255) continue;

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

/* A player decides to abandon one of his concurrent quests. */
void quest_abandon(int Ind, int py_q_idx) {
	player_type *p_ptr = Players[Ind];
	int q_idx = p_ptr->quest_idx[py_q_idx];

	/* erase all of his quest items */
	quest_erase_objects(q_idx, TRUE, p_ptr->id);

	/* no longer on it */
	p_ptr->quest_idx[py_q_idx] = -1;

	/* give him 'quest done' credit if he cancelled it too late (ie after rewards were handed out)? */
	if (q_info[q_idx].quest_done_credit_stage <= quest_get_stage(Ind, q_idx)
	    && p_ptr->quest_done[q_idx] < 10000) //limit
		p_ptr->quest_done[q_idx]++;

#ifdef USE_SOUND_2010
	sound(Ind, "failure", NULL, SFX_TYPE_MISC, FALSE);
#endif
}

/* Display quest status log of a current quest stage to a player. */
void quest_log(int Ind, int py_q_idx) {
	player_type *p_ptr = Players[Ind];
	int q_idx = p_ptr->quest_idx[py_q_idx];
	quest_info *q_ptr = &q_info[q_idx];
	int pInd = q_ptr->individual ? Ind : 0;
	int k, stage = quest_get_stage(Ind, q_idx);
	qi_stage *q_stage = quest_qi_stage(q_idx, stage);
	char text[MAX_CHARS * 2];
	bool anything = FALSE;

	/* pre-scan narration if any line at all exists and passes the flag check */
	for (k = 0; k < QI_LOG_LINES; k++) {
		if (q_stage->log[k] &&
		    ((q_stage->log_flags[k] & quest_get_flags(pInd, q_idx)) == q_stage->log_flags[k])) {
			anything = TRUE;
			break;
		}
	}

	if (!anything) {
		msg_print(Ind, " ");
		msg_format(Ind, "\377uThere is currently no log for quest <\377U%s\377u>", q_name + q_ptr->name);
		msg_print(Ind, " ");
		return;
	}

	msg_print(Ind, " ");
	msg_format(Ind, "\377uLog for quest <\377U%s\377u>:", q_name + q_ptr->name);
	for (k = 0; k < QI_LOG_LINES; k++) {
		if (!q_stage->log[k]) break;
		if ((q_stage->log_flags[k] & quest_get_flags(pInd, q_idx)) != q_stage->log_flags[k]) continue;
#if 0 /* simple way */
		msg_format(Ind, "\377U%s", q_stage->log[k]);
#else /* allow placeholders */
		quest_text_replace(text, q_stage->log[k], p_ptr);
		msg_format(Ind, "\377U%s", text);
#endif
	}
	msg_print(Ind, " ");
}


/* ---------- Helper functions for initialisation of q_info[] from q_info.txt in init.c ---------- */

/* Allocate/initialise a questor, or return it if already existing. */
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
	p->death_fail = -1; /* quest fails if this questor dies */
	p->q_loc.tpref = NULL;

	/* done, return the new, requested one */
	return &q_ptr->questor[num];
}

/* Allocate/initialise a questor-morph, or return it if already existing. */
qi_questor_morph *init_quest_qmorph(int q_idx, int stage, int questor) {
	qi_stage *q_stage = init_quest_stage(q_idx, stage);
	qi_questor_morph *p;

	/* we already have this existing one */
	if (q_stage->questor_morph[questor]) return q_stage->questor_morph[questor];

	/* allocate a new one */
	p = (qi_questor_morph*)malloc(sizeof(qi_questor_morph));
	if (!p) {
		s_printf("init_quest_qmorph() Couldn't allocate memory!\n");
		exit(0);
	}

	/* attach it to its parent structure */
	q_stage->questor_morph[questor] = p;

	/* default: no name change */
	p->name = NULL;

	/* done, return the new, requested one */
	return p;
}
/* Allocate/initialise a questor-hostility, or return it if already existing. */
qi_questor_hostility *init_quest_qhostility(int q_idx, int stage, int questor) {
	qi_stage *q_stage = init_quest_stage(q_idx, stage);
	qi_questor_hostility *p;

	/* we already have this existing one */
	if (q_stage->questor_hostility[questor]) return q_stage->questor_hostility[questor];

	/* allocate a new one */
	p = (qi_questor_hostility*)malloc(sizeof(qi_questor_hostility));
	if (!p) {
		s_printf("init_quest_qhostility() Couldn't allocate memory!\n");
		exit(0);
	}

	/* default (off) */
	p->hostile_revert_timed_ingame_abs = -1;

	/* attach it to its parent structure */
	q_stage->questor_hostility[questor] = p;

	/* done, return the new, requested one */
	return p;
}
/* Allocate/initialise a questor-act, or return it if already existing. */
qi_questor_act *init_quest_qact(int q_idx, int stage, int questor) {
	qi_stage *q_stage = init_quest_stage(q_idx, stage);
	qi_questor_act *p;

	/* we already have this existing one */
	if (q_stage->questor_act[questor]) return q_stage->questor_act[questor];

	/* allocate a new one */
	p = (qi_questor_act*)malloc(sizeof(qi_questor_act));
	if (!p) {
		s_printf("init_quest_qact() Couldn't allocate memory!\n");
		exit(0);
	}

	/* attach it to its parent structure */
	q_stage->questor_act[questor] = p;

	/* done, return the new, requested one */
	return p;
}

/* Allocate/initialise a quest stage, or return it if already existing. */
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
		p->next_stage_from_goals[i] = 255; /* no next stages set, end quest by default if nothing specified */
		for (j = 0; j < QI_STAGE_GOALS; j++)
			p->goals_for_stage[i][j] = -1; /* no next stages set, end quest by default if nothing specified */
	}
	for (i = 0; i < QI_STAGE_REWARDS; i++) {
		for (j = 0; j < QI_REWARD_GOALS; j++)
			p->goals_for_reward[i][j] = -1; /* no next stages set, end quest by default if nothing specified */
	}
	p->activate_quest = -1;
	p->change_stage = 255;
	p->timed_ingame_abs = -1;
	p->timed_real = 0;
	if (num == 0) p->accepts = TRUE;
	p->reward = NULL;
	p->goal = NULL;
	p->geno_wpos.wx = -1; //disable

	/* done, return the new one */
	return &q_ptr->stage[q_ptr->stages - 1];
}

/* Allocate/initialise a quest keyword, or return it if already existing. */
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

/* Allocate/initialise a quest keyword reply, or return it if already existing. */
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

/* Allocate/initialise a kill goal, or return it if already existing.
   NOTE: This function takes a 'goal' that is 1 higher than the internal goal index!
         That's a hack to allow specifying 'optional' goals in q_info.txt. */
qi_kill *init_quest_kill(int q_idx, int stage, int q_info_goal) {
	qi_goal *q_goal = init_quest_goal(q_idx, stage, q_info_goal);
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
		p->name[i] = NULL;

		p->rchar[i] = 254;
		p->rattr[i] = 254;

	}

	for (i = 0; i < 10; i++)
		p->reidx[i] = -1;//accept all egos (incl. 0)

	/* done, return the new, requested one */
	return p;
}

/* Allocate/initialise a kill goal, or return it if already existing.
   NOTE: This function takes a 'q_info_goal' that is 1 higher than the internal goal index!
         That's a hack to allow specifying 'optional' goals in q_info.txt. */
qi_retrieve *init_quest_retrieve(int q_idx, int stage, int q_info_goal) {
	qi_goal *q_goal = init_quest_goal(q_idx, stage, q_info_goal);
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
		p->otval[i] = -1; // 'any'
		p->osval[i] = -1; // 'any'

		p->name[i] = NULL;

		p->opval[i] = -9999;
		p->obpval[i] = -9999;
		p->oattr[i] = 254;
		p->oname1[i] = -1;
		p->oname2[i] = -1;
		p->oname2b[i] = -1;
	}

	/* done, return the new, requested one */
	return p;
}

/* Allocate/initialise a kill goal, or return it if already existing.
   NOTE: This function takes a 'q_info_goal' that is 1 higher than the internal goal index!
         That's a hack to allow specifying 'optional' goals in q_info.txt. */
qi_deliver *init_quest_deliver(int q_idx, int stage, int q_info_goal) {
	qi_goal *q_goal = init_quest_goal(q_idx, stage, q_info_goal);
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

	/* default: not a return-to-questor goal */
	p->return_to_questor = 255;

	/* done, return the new, requested one */
	return p;
}

/* Allocate/initialise a kill goal, or return it if already existing.
   NOTE: This function takes a 'q_info_goal' that is 1 higher than the internal goal index!
         That's a hack to allow specifying 'optional' goals in q_info.txt. */
qi_goal *init_quest_goal(int q_idx, int stage, int q_info_goal) {
	qi_stage *q_stage = init_quest_stage(q_idx, stage);
	qi_goal *p;
	bool opt = FALSE;

	/* hack: negative number means optional goal.
	   Once a goal is initialised as 'optional' in here,
	   it will stay optional, even if it's referred to by positive index after that.
	   NOTE: Goal 0 can obviously never be optional^^ but you don't have to use it! */
	if (q_info_goal < 0) {
		q_info_goal = -q_info_goal;
		opt = TRUE;
	}
	q_info_goal--; /* unhack it to represent internal goal index now */

	/* we already have this existing one */
	if (q_stage->goals > q_info_goal) return &q_stage->goal[q_info_goal];

	/* allocate all missing instances up to the requested index */
	p = (qi_goal*)realloc(q_stage->goal, sizeof(qi_goal) * (q_info_goal + 1));
	if (!p) {
		s_printf("init_quest_goal() Couldn't allocate memory!\n");
		exit(0);
	}
	/* initialise the ADDED memory */
	//memset(p + q_stage->goals * sizeof(qi_goal), 0, (q_info_goal + 1 - q_stage->goals) * sizeof(qi_goal));
	memset(&p[q_stage->goals], 0, (q_info_goal + 1 - q_stage->goals) * sizeof(qi_goal));

	/* attach it to its parent structure */
	q_stage->goal = p;
	q_stage->goals = q_info_goal + 1;

	/* initialise with correct defaults (paranoia) */
	p = &q_stage->goal[q_stage->goals - 1];
	p->kill = NULL;
	p->retrieve = NULL;
	p->deliver = NULL;
	p->optional = opt; /* permanent forever! */

	/* done, return the new, requested one */
	return &q_stage->goal[q_info_goal];
}

/* Allocate/initialise a quest reward, or return it if already existing. */
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

/* Allocate/initialise a custom quest item, or return it if already existing. */
qi_questitem *init_quest_questitem(int q_idx, int stage, int num) {
	qi_stage *q_stage = init_quest_stage(q_idx, stage);
	qi_questitem *p;

	/* we already have this existing one */
	if (q_stage->qitems > num) return &q_stage->qitem[num];

	/* allocate a new one */
	p = (qi_questitem*)realloc(q_stage->qitem, sizeof(qi_questitem) * (q_stage->qitems + 1));
	if (!p) {
		s_printf("init_quest_questitem() Couldn't allocate memory!\n");
		exit(0);
	}
	/* initialise the ADDED memory */
	memset(&p[q_stage->qitems], 0, sizeof(qi_questitem));

	/* set defaults */
	p->questor_gives = 255; /* 255 = disabled */
	p->q_loc.tpref = NULL;

	/* attach it to its parent structure */
	q_stage->qitem = p;
	q_stage->qitems++;

	/* done, return the new one */
	return &q_stage->qitem[q_stage->qitems - 1];
}

/* Allocate/initialise a cave feature to be built in a stage,
   or return it if already existing. */
qi_feature *init_quest_feature(int q_idx, int stage, int num) {
	qi_stage *q_stage = init_quest_stage(q_idx, stage);
	qi_feature *p;


	/* we already have this existing one */
	if (q_stage->feats > num) return &q_stage->feat[num];

	/* allocate a new one */
	p = (qi_feature*)realloc(q_stage->feat, sizeof(qi_feature) * (q_stage->feats + 1));
	if (!p) {
		s_printf("init_quest_feature() Couldn't allocate memory!\n");
		exit(0);
	}
	/* initialise the ADDED memory */
	memset(&p[q_stage->feats], 0, sizeof(qi_feature));

	/* attach it to its parent structure */
	q_stage->feat = p;
	q_stage->feats++;

	/* done, return the new one */
	return &q_stage->feat[q_stage->feats - 1];
}

/* Allocate/initialise a monster spawn event in a stage,
   or return it if already existing. */
qi_monsterspawn *init_quest_monsterspawn(int q_idx, int stage, int num) {
	qi_stage *q_stage = init_quest_stage(q_idx, stage);
	qi_monsterspawn *p;


	/* we already have this existing one */
	if (q_stage->mspawns > num) return &q_stage->mspawn[num];

	/* allocate a new one */
	p = (qi_monsterspawn*)realloc(q_stage->mspawn, sizeof(qi_monsterspawn) * (q_stage->mspawns + 1));
	if (!p) {
		s_printf("init_quest_monsterspawn() Couldn't allocate memory!\n");
		exit(0);
	}
	/* initialise the ADDED memory */
	memset(&p[q_stage->mspawns], 0, sizeof(qi_monsterspawn));

	/* attach it to its parent structure */
	q_stage->mspawn = p;
	q_stage->mspawns++;

	/* done, return the new one */
	return &q_stage->mspawn[q_stage->mspawns - 1];
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
			switch (q_ptr->questor[j].type) {
			case QI_QUESTOR_NPC:
				questor = m_list[q_ptr->questor[j].mo_idx].questor;
				k = m_list[q_ptr->questor[j].mo_idx].quest;
				s_printf(" m_idx %d of q_idx %d (questor=%d)\n", q_ptr->questor[j].mo_idx, k, questor);
				if (k == i && questor) {
					s_printf("..ok.\n");
					delete_monster_idx(q_ptr->questor[j].mo_idx, TRUE);
				} else s_printf("..failed: Questor does not exist.\n");
				break;
			case QI_QUESTOR_ITEM_PICKUP:
				questor = o_list[q_ptr->questor[j].mo_idx].questor;
				k = o_list[q_ptr->questor[j].mo_idx].quest - 1;
				s_printf(" o_idx %d of q_idx %d (questor=%d)\n", q_ptr->questor[j].mo_idx, k, questor);
				if (k == i && questor) {
					s_printf("..ok.\n");
					o_list[q_ptr->questor[j].mo_idx].questor = o_list[q_ptr->questor[j].mo_idx].quest = 0; //prevent questitem_d() check from triggering when deleting it!..
					q_ptr->objects_registered--; //..and count down manually
					delete_object_idx(q_ptr->questor[j].mo_idx, TRUE);
				} else s_printf("..failed: Questor does not exist.\n");
				break;
			}
		}
	}
}

/* To call after server has been loaded up, to reattach questors and their quests,
   index-wise. The indices get shuffled over server restart. */
void fix_questors_on_startup(void) {
	quest_info *q_ptr;
	monster_type *m_ptr;
	object_type *o_ptr;
	int i;

	for (i = 1; i < m_max; i++) {
		m_ptr = &m_list[i];
		if (!m_ptr->questor) continue;

		q_ptr = &q_info[m_ptr->quest];
		if (!q_ptr->defined || /* this quest no longer exists in q_info.txt? */
		    !q_ptr->active || /* or it's not supposed to be enabled atm? */
		    q_ptr->questors <= m_ptr->questor_idx) { /* ew */
			s_printf("QUESTOR DEPRECATED (on load) m_idx %d, q_idx %d.\n", i, m_ptr->quest);
			m_ptr->questor = FALSE;
			/* delete him too, maybe? */
		} else q_ptr->questor[m_ptr->questor_idx].mo_idx = i;
	}

	for (i = 1; i < o_max; i++) {
		o_ptr = &o_list[i];
		if (!o_ptr->questor) continue;

		q_ptr = &q_info[o_ptr->quest - 1];
		if (!q_ptr->defined || /* this quest no longer exists in q_info.txt? */
		    !q_ptr->active || /* or it's not supposed to be enabled atm? */
		    q_ptr->questors <= o_ptr->questor_idx) { /* ew */
			s_printf("QUESTOR DEPRECATED (on load) o_idx %d, q_idx %d.\n", i, o_ptr->quest - 1);
			o_ptr->questor = FALSE;
			/* delete him too, maybe? */
		} else q_ptr->questor[o_ptr->questor_idx].mo_idx = i;
	}
}

/* Keep track of losing quest-related items we spawned,
   so we don't try to remove them later on if they're already gone,
   saving CPU and I/O.
   NOTE: We do not track normal items that just happen to be needed for a
         retrieval quest goal, or that are normal quest rewards, since we
         don't want to erase those when the quest is done. */
void questitem_d(object_type *o_ptr, int num) {
	if (!o_ptr->questor && /* it's neither a questor */
	    !(o_ptr->tval == TV_SPECIAL && o_ptr->sval == SV_QUEST && o_ptr->quest)) /* nor a special quest item? */
		return;

	/* paranoia */
	if (!q_info[o_ptr->quest - 1].objects_registered) {
		s_printf("QUESTITEM_D: t%d,s%d,p%d is already zero.\n", o_ptr->tval, o_ptr->sval, o_ptr->pval);
		return;
	}

	q_info[o_ptr->quest - 1].objects_registered -= num;

	/* react to object questor 'death' */
	if (o_ptr->questor) {
		if (q_info[o_ptr->quest - 1].defined && q_info[o_ptr->quest - 1].questors > o_ptr->questor_idx) {
			/* Quest progression/fail effect? */
			questor_death(o_ptr->quest - 1, o_ptr->questor_idx, &o_ptr->wpos, o_ptr->owner);
		} else {
			s_printf("QUESTOR DEPRECATED (object_death)\n");
		}
	}
}

/* ---------- Questor actions/reactions to 'external' effects in the game world ---------- */

/* Questor was killed and drops any loot? */
void questor_drop_specific(int Ind, int q_idx, int questor_idx, struct worldpos *wpos, int x, int y) {
	qi_questor *q_questor = &q_info[q_idx].questor[questor_idx];
	object_type forge, *o_ptr = &forge;
	u32b resf = RESF_NOTRUEART;

	/* specific reward */
	if (q_questor->drops_tval) {
		/* very specific reward */
		if (!q_questor->drops_good && !q_questor->drops_great && !q_questor->drops_vgreat) {
			o_ptr = &forge;
			object_wipe(o_ptr);
			invcopy(o_ptr, lookup_kind(q_questor->drops_tval, q_questor->drops_sval));
			o_ptr->number = 1;
			o_ptr->name1 = q_questor->drops_name1;
			o_ptr->name2 = q_questor->drops_name2;
			o_ptr->name2b = q_questor->drops_name2b;
			if (o_ptr->name1) {
				o_ptr->name1 = ART_RANDART; //hack: disallow true arts!
				o_ptr->name2 = o_ptr->name2b = 0;
			}
			apply_magic(wpos, o_ptr, -2, FALSE, FALSE, FALSE, FALSE, resf);
			o_ptr->pval = q_questor->drops_pval;
			o_ptr->bpval = q_questor->drops_bpval;
			o_ptr->note = 0;
			o_ptr->note_utag = 0;
#ifdef PRE_OWN_DROP_CHOSEN
			o_ptr->level = 0;
			o_ptr->owner = Players[Ind]->id;
			o_ptr->mode = Players[Ind]->mode;
			o_ptr->iron_trade = Players[Ind]->iron_trade;
			if (true_artifact_p(o_ptr)) determine_artifact_timeout(o_ptr->name1, wpos);
#endif
		} else {
			o_ptr = &forge;
			object_wipe(o_ptr);
			invcopy(o_ptr, lookup_kind(q_questor->drops_tval, q_questor->drops_sval));
			o_ptr->number = 1;
			apply_magic(wpos, o_ptr, -2, q_questor->drops_good, q_questor->drops_great, q_questor->drops_vgreat, FALSE, resf);
			o_ptr->note = 0;
			o_ptr->note_utag = 0;
#ifdef PRE_OWN_DROP_CHOSEN
			o_ptr->level = 0;
			o_ptr->owner = Players[Ind]->id;
			o_ptr->mode = Players[Ind]->mode;
			o_ptr->iron_trade = Players[Ind]->iron_trade;
			if (true_artifact_p(o_ptr)) determine_artifact_timeout(o_ptr->name1, wpos);
#endif
		}
		drop_near(0, o_ptr, 0, wpos, y, x);
	}
	/* instead use create_reward() like for events? */
	else if (q_questor->drops_reward) {
		switch (q_questor->drops_reward) {
		case 1: resf |= RESF_LOW; break;
		case 2: resf |= RESF_LOW2; break;
		case 3: resf |= RESF_MID; break;
		case 4: resf |= RESF_HIGH; break;
		case 5: break; /* 'allow randarts' */
		}
		create_reward(Ind, o_ptr, 95, 95, TRUE, TRUE, resf, 3000);
		drop_near(0, o_ptr, 0, wpos, y, x);
	}

	/* drop gold too? */
	if (q_questor->drops_gold) {
		invcopy(o_ptr, gold_colour(q_questor->drops_gold, TRUE, FALSE));
		o_ptr->pval = q_questor->drops_gold;
		drop_near(0, o_ptr, 0, wpos, y, x);
	}
}

/* Questor was killed (npc) or destroyed (object). Quest progression/fail effect?
   Ind can be 0 if object questor got destroyed/erased by other means.
   'owner' is for object questors, of which we assume that they are unique per
   player and not droppable, so the player who loses it will get his individual
   quest terminated if death_fail == -1. */
void questor_death(int q_idx, int questor_idx, struct worldpos *wpos, s32b owner) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_questor *q_questor = &q_ptr->questor[questor_idx];

	int i, j, stage = quest_get_stage(0, q_idx), pInd = 0;
	qi_stage *q_stage;
	qi_goal *q_goal;
	qi_deliver *q_del;

	q_questor->tainted = TRUE; //a "location change" (aka death) warrants need for respawn later

	/* for (object-type) questors, deactivate the quest
	   so it will be activated anew after a cooldown? */

	switch (q_questor->type) {
	case QI_QUESTOR_NPC:
		break;

	case QI_QUESTOR_ITEM_PICKUP:
		/* Find the player whose object questor just broke */
		for (i = 1; i <= NumPlayers; i++)
			if (Players[i]->id == owner) {
				pInd = i;
				break;
			}
		break;
	}

	/* questor death either changes stage or even terminates quest? */
	if (q_questor->death_fail != 255) {
		/* unstatice static locations of previous stage, possibly from
		   quest items, targetted goals, deliver goals. */
		q_stage = quest_qi_stage(q_idx, stage);
		for (i = 0; i < q_stage->goals; i++) {
			q_goal = &q_stage->goal[i];
			q_del = q_goal->deliver;
			if (q_goal->target_tpref)
				new_players_on_depth(&q_goal->target_wpos, -1, TRUE);
			if (q_del && q_del->tpref)
				new_players_on_depth(&q_del->wpos, -1, TRUE);
			for (j = 0; j < q_stage->qitems; j++)
				if (q_stage->qitem[j].q_loc.tpref)
					new_players_on_depth(&q_stage->qitem[j].result_wpos, -1, TRUE);
		}
	}

	/* (Ind can be 0 too, if questor got destroyed by something else..) */
	if (q_questor->death_fail == -1) quest_terminate(pInd, q_idx, wpos);
	else if (q_questor->death_fail != 255) quest_set_stage(0, q_idx, q_questor->death_fail, FALSE, wpos);
}

/* A questor who was walking towards a destination waypoint has just arrived.
   The Ind is just the player the questor 'had targetted', would he be a normal
   monster. It might as well just be omitted, but maybe it'll turn out convenient. */
void quest_questor_arrived(int q_idx, int questor_idx, struct worldpos *wpos) {
	qi_stage *q_stage = quest_cur_qi_stage(q_idx);
	qi_questor_act *q_qact = q_stage->questor_act[questor_idx];

	q_qact->walk_speed = 0;

	if (q_qact->change_stage)
		quest_set_stage(0, q_idx, q_qact->change_stage, q_qact->quiet_change, wpos);
}

/* An aggressive questor reverts to non-aggressive */
void quest_questor_reverts(int q_idx, int questor_idx, struct worldpos *wpos) {
	quest_info *q_ptr = &q_info[q_idx];
	qi_stage *q_stage = quest_cur_qi_stage(q_idx);
	qi_questor *q_questor = &q_ptr->questor[questor_idx];
	qi_questor_hostility *q_qhost = q_stage->questor_hostility[questor_idx];
	monster_type *m_ptr = &m_list[q_questor->mo_idx];
	monster_race *r_ptr = m_ptr->r_ptr;

	r_ptr->flags7 |= (RF7_NO_TARGET | RF7_NEVER_ACT);//| RF7_NO_DEATH; --done in quest_questor_morph() actually
	m_ptr->questor_hostile = 0x0;

	/* change stage? */
	if (q_qhost->change_stage != 255)
		quest_set_stage(0, q_idx, q_qhost->change_stage, q_qhost->quiet_change, wpos);
}
