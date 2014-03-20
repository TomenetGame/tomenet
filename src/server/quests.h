/* This is the header file for quests.c, please look there for more information.
   You may modify/use it freely as long as you give proper credit. - C. Blue
*/

#include "angband.h"

/* Notes:

   Stage changes given as negative numbers will instead add a random value
   of 0..ABS(stage) to current stage, allowing for random quest progression!
   Eg: kill_stage == -3 -> next_stage = current_stage + randint(3).

   codename 'none' is reserved, to indicate that it's not a follow-up quest.

   If a quest stage doesn't have any stage goals, nor any dialogue keywords,
   the quest will terminate after all automatic/timed actions of this stage
   have been done and all eligible rewards have been handed out.
   ++including: questor movement/teleport/revert-from-hostile and
   timed and instant stage-change effects.

   Items retrieved will be marked as 'quest items' for easy check in case the
   player also has to deliver them somewhere, whether they're the same items.


   Data structure:
   quest_info -> qi_stage   -> qi_goal   -> qi_kill
                                         -> qi_retrieve
                                         -> qi_deliver
                            -> qi_reward
              -> qi_keyword
              -> qi_kwreply
*/


/* Sub-structure: Mandatory questor information */
typedef struct qi_questor {
	/*-----  Fixed questor spawn information (from q_info.txt) ----- */

	/* quest initialisation and meta actions */
	bool accept_los, accept_interact;		/* player gets the quest just be being in LoS / interacting once with the questor (bump/read the parchment/pickup the item) */

	/* starting location restrictions */
	byte s_location_type;				/* flags setting elibible starting location types (QI_SLOC_xxx) */
	u16b s_towns_array;				/* QI_SLOC_TOWN: flags setting eligible starting towns (QI_STOWN_xxx) */
	u32b s_terrains;				/* QI_SLOC_SURFACE: flags setting eligible starting terrains (RF8_WILD_xxx, RF8_WILD_TOO_MASK for all) */
	/* exact questor starting location */
	struct worldpos start_wpos;			/* -1, -1 for random */
	s16b start_x, start_y;				/* -1, -1 for random */
	cptr questor_tpref;				/* filename of map to load, or empty for none */

	bool s_dungeon[MAX_D_IDX];			/* QI_SLOC_DUNGEON/TOWER: eligible starting dungeons/towers, or (for Wilderness dungeons): */
	u32b s_dungeon_must_flags1, s_dungeon_must_flags2, s_dungeon_must_flags3;	/*  eligible wilderness dungeon flags */
	u32b s_dungeon_mustnt_flags1, s_dungeon_mustnt_flags2, s_dungeon_mustnt_flags3;	/*  uneligible wilderness dungeon flags */
	bool s_dungeon_iddc;				/* is the Ironman Deep Dive Challenge an eligible starting point? */
	byte dlevmin, dlevmax;				/* eligible dungeon level or world sector level (0 for any) */

	/* type of questor */
	byte type;					/* QI_QUESTOR_xxx */

	s16b ridx;					/* QI_QUESTOR_NPC; 0 for any */
	char rchar;					/*  0 for any */
	byte rattr;					/*  0 for any */
	byte rlevmin, rlevmax;				/*  0 for any */

	s16b sval;					/* QI_QUESTOR_PARCHMENT */

	s16b ktval, ksval;				/* QI_QUESTOR_ITEM_xxx. No further stats/enchantments are displayed! */

	char name[MAX_CHARS];				/* optional pseudo-unique name that overrides the normal name */

	bool talkable;					/* questor accepts dialogue? (by bumping usually) */
	bool despawned;					/* questor starts despawned? */

	/* ..if killable ie not invincible: */
	bool drops_regular;				/* Drops regular loot (of his ridx type) instead of nothing? */
	bool drops_specific;				/* Drops a specific item (like DROP_CHOSEN) */
	s16b drops_tval;				/* hand over certain rewards to the player */
	s16b drops_sval;
	s16b drops_pval, drops_bpval;
	s16b drops_name1, drops_name2, drops_name2b;
	bool drops_good, drops_great;
	bool drops_reward	;			/*  use fitting-reward algo (from highlander etc)? */
	int drops_gold;
	int exp;

	/* ----- Dynamic questor information ----- */

	/* keep track of actual resulting questor location --
	   this data gets generated dynamically on quest activation from above template data */
	struct worldpos current_wpos;
	s16b current_x, current_y;
	s16b m_idx;
} qi_questor;

/* Sub-structure: Questor changes (or turns vulnerable) ('S') */
typedef struct qi_questor_morph {
	/* special questor behaviour during each stage */
	bool talkable;					/* questor accepts dialogue? (by bumping usually) */
	bool despawned;					/* questor vanishes during a quest stage? */

	bool invincible;				/* Is the questor invincible (if monster)/unpickable by monsters (if item) during a particular stage? */
	s16b death_fail;				/* If the questor dies, the quest goes to stage n? (->reset old stage goals/positions as if we just entered it, if that is possible? hm) */
	bool death_fail_all;				/* If the questor dies, the quest fails completely? */
	cptr name;					/* questor changes optional pseudo-unique name during this stage? */
	s16b ridx; 					/* questor changes to this base monster type */
	char rchar;
	byte rattr;
	byte rlev;
} qi_questor_morph;

/* Sub-structure: Questor moves himself or the player ('S'/'H') */
typedef struct qi_questor_hostility {
	struct worldpos teleport_wpos;			/* teleport participating player to a new position */
	s16b teleport_player_x, teleport_player_y;

	s16b hostile;					/* questor turns into a normal aggressor, and stage is changed */
	s16b hostile_revert_hp;				/* aggressor-questor turns back into a non-aggressive questor when falling to <= HP (death prevented!) and stage is changed */
	s16b hostile_revert_timed_ingame;		/* ..after ingame time (min).. */
	s16b hostile_revert_timed_ingame_abs;		/* ..at ingame time.. */
	s16b hostile_revert_timed_real;			/* ..after real time (s).. */

	bool turns_normal;				/* questor actually loses questor status and turns into a regular mob! */
} qi_questor_hostility;

/* Sub-structure: Questor moves himself or the player ('J') */
typedef struct qi_questor_act {
	byte walk_speed;				/* questor will actually move around during this stage? */
	s16b walk_destx, walk_desty;			/* target waypoint for questor to move to */
	s16b walk_stage;				/* stage will change when questor arrives at destination */

	struct worldpos teleport_wpos;			/* teleport questor to a new position */
	s16b teleport_x, teleport_y;
} qi_questor_act;

	/* quest goals, up to 10 per stage, with a multitude of different sub-goals (Note: of the subgoals 1 is randomly picked for the player, except if 'xxx_random_pick' is set, which allows the player to pick what he wants to do).
	   There are additionally up to 10 optional quest goals per stage.
	   --note: the #s of subgoals don't use #defines, because they vary too much anyway for each category, so they're just hard-coded numbers. */
//#define QI_GOALS 10 /* main goals to complete a stage */
//#define QI_OPTIONAL 10 /* optional goals in a stage */

/* Sub-structure: A single kill goal */
typedef struct qi_kill {
#if 0 /* too much, make it simpler for now */
	bool player_picks;				/* instead of picking one of the eligible monster criteria randomly, let the player decide which he wants to get */
#endif
	s16b ridx[10];					/* kill certain monster(s), 0 for none, -1 for any. */
	char rchar[5];					/*  ..certain types, 254 for any, 255 for none. AND's with attr/lev. */
	byte rattr[5];					/*  ..certain colours, 254 for any, 255 for none. AND's with char/lev. */
	byte rlevmin, rlevmax;				/* 0 for any. AND's with char/attr. */
	s16b number, number_left;
	byte spawn;					/* actually spawn the monster(s) nearby/in the target zone! (QI_SPAWN_xxx) */
	byte spawn_targets;				/* the spawned mobs go for 0=any players (normal monster AI) 1=the player who talked to the questor 2=questor */
} qi_kill;

/* Sub-structure: A single retrieval goal (main mem eater) */
typedef struct qi_retrieve {
#if 0 /* too much, make it simpler for now */
	bool player_picks;				/* instead of picking one subgoal randomly, let the player decide which he wants to get */
#endif
	s16b otval[10], osval[10];			/* retrieve certain item(s) (tval or sval == -1 -> any tval or sval, 0 = not checked) */
	s16b opval[5], obpval[5];			/* umm, let's say 9999 = not checked :-p, -9999 = any */
	byte oattr[5];					/*  ..certain colours (flavoured items only), 255 = not checked, 254 = any */
	s16b oname1[5], oname2[5], oname2b[5];		/* -3 = not checked, -2 == any except zero, -1 = any */
	int ovalue;					/* minimum value of the item (0 to disab..wait) */
	s16b number;					/* amount of fitting items to retrieve */
} qi_retrieve;

/* Sub-structure: A single deliver goal */
typedef struct qi_deliver {
	struct worldpos wpos;				/* (after optionally killing/retrieving/or whatever) MOVE TO this world pos */
	bool terrain_patch;				/* extend valid target location over all connected world sectors whose terrain is of the same type (eg big forest)
							   max radius is QI_TERRAIN_PATCH_RADIUS. */
	s16b pos_x, pos_y;				/* -"- ..MOVE TO specifically this position */
	cptr tpref;					/* filename of map to load, or empty for none */
} qi_deliver;

/* Sub-structure: A single quest goal (which can be kill/retrieve/deliver) */
typedef struct qi_goal {
	/* Type of goal - exactly one of them must be non-NULL */
	qi_kill *kill;
	qi_retrieve *retrieve;
	qi_deliver *deliver;

	/* is this an optional goal? */
	bool optional;

	/* quest goals */
	bool goals;
	bool goals_nisi;				/* for goals set by kill/retrieve depending on deliver (for flag changes) */
	/* 'Z' lines: goals set/clear flags */
	u16b goal_setflags;
	u16b goal_clearflags;

	/* for kill/retrieve goals only (deliver goals have/need separate location info) */
	bool target_pos;				/* enable target pos? */
	struct worldpos target_wpos;			/* kill/retrieve specifically at this world pos */
	s16b target_pos_x, target_pos_y;		/* at specifically this position (even usable for kill/retrieve stuff?) */
	bool target_terrain_patch;			/* extend valid target location over all connected world sectors whose terrain is of the same type (eg big forest)
							   max radius is QI_TERRAIN_PATCH_RADIUS. */
	cptr target_tpref;				/* filename of map to load, or empty for none */

#if 0 /* just use a deliver_pos goal instead, for now */
	bool return_to_questor;				/* do we need to return to the questor first (bump), to get credit for particular main goals? */
#endif
} qi_goal;

/* Sub-structure: A single quest reward */
typedef struct qi_reward {
	s16b otval	;				/* hand over certain rewards to the player */
	s16b osval;
	s16b opval, obpval;
	s16b oname1, oname2, oname2b;
	bool ogood, ogreat, ovgreat;
	bool oreward;					/* use fitting-reward algo (from highlander etc)? */

	int gold;
	int exp;
	s16b statuseffect;				/* debuff (aka curse, maybe just for show)/un-debuff/tempbuff player? */
} qi_reward;

/* Sub-structure: A single quest stage */
typedef struct qi_stage {
	/* quest acceptance */
	bool accepts;					/* player can acquire the quest during a stage */

	s16b activate_quest;				/* spawn a certain new quest of this index (and thereby another questor) (if not already existing) */
	bool auto_accept;				/* player will automatically acquire the newly spawned quest (from activate_quest[]) */
	bool auto_accept_quiet;				/* player will automatically acquire the newly spawned quest (from activate_quest[]) but not get a quest-accept-notification type of message about it */


	/* stage-change automatics */
	s16b change_stage;				/* automatically change to a different stage after handling everything that was to do in the current stage (-1 = disable) */
#if 0 /* currently not possible since we call the quest scheduler once a minute */
	s16b timed_stage_ingame;			/* automatically change to a different stage after a certain amount of in-game minutes passed */
	s16b timed_stage_ingame_abs;			/* automatically change to a different stage when a certain in-game time is reached (minute resolution) */
#else
	s16b timed_stage_ingame_abs;			/* automatically change to a different stage when a certain in-game time is reached (HOUR resolution! -1 to disable) */
#endif
	s16b timed_stage_real;				/* automatically change to a different stage after a certain amount of real minutes passed */
	bool quiet_change_stage;			/* for the above auto-changes: don't replay the stage's dialogue */
	/* dynamic timer helper data */
	s16b timed_stage_countdown;			/* dynamic, for countdown for above timings: negative value = ingame absolute, positive value = real-time counting down */
	s16b timed_stage_countdown_stage;
	bool timed_stage_countdown_quiet;


	/* create a dungeon/tower for a quest stage? completely static? predefined layouts? */
	struct worldpos add_dungeon_wpos;		/* create it at this world pos */
	bool add_dungeon_terrain_patch;			/* extend valid location over all connected world sectors whose terrain is of the same type (eg big forest) */
	char *add_dungeon_parms;			/* same as for master_level() maybe */
	char **add_dungeon_t_pref;			/* table of predefined template filenames for each dungeon floor (or NULL for random floors inbetween) */
	bool add_dungeon_fullystatic;			/* all floors are static */
	bool add_dungeon_keep;				/* keep dungeon until quest ends instead of erasing it when this stage is completed */


	/* Questor going bonkers? (optional/advanced) */
	qi_questor_morph *questor_morph;
	qi_questor_hostility *questor_hostility;
	qi_questor_act *questor_act;


	/* quest dialogues and responses/consequences (stage 0 means player loses the quest again) */
	//NOTE: '$RPM' in dialogue will be substituted by xxx_random_pick'ed monster criteria
	//NOTE: '$OPM' in dialogue will be substituted by xxx_random_pick'ed object criteria
	s16b talk_focus[QI_QUESTORS];			/* questor is focussed on this player and won't give others a chance to reply with keywords (non-individual quests only) */
	byte talk_lines[QI_QUESTORS];
	cptr talk[QI_QUESTORS][QI_TALK_LINES]; 		/* n conversations a 10 lines a 79 characters */
	u16b talkflags[QI_QUESTORS][QI_TALK_LINES];	/* required flags configuration for a convo line to get displayed  */

	byte narration_lines;
	cptr narration[QI_TALK_LINES];			/* display a quest-progress narration when this stage starts, a 10 lines a 79 characters, aka "You have arrived at the lake!" */
	u16b narrationflags[QI_TALK_LINES];		/* required flags configuration to display this narrative line */


	/* the rewards for this stage, if any */
	byte rewards;
	qi_reward *reward;

	/* contains the indices of up to QI_REWARD_GOALS different QI_GOALS/QI_OPTIONAL goals which are AND'ed;
	   hack: 'optional' indices start after main goals, so if QI_GOALS is 10, the first QI_OPTIONAL would have index 11. */
	char goals_for_reward[QI_STAGE_REWARDS][QI_REWARD_GOALS]; /* char to save space, only 1 byte instead of int: returns the goal's index (or -1 if none) */


	/* the goals for this stage */
	byte goals;
	qi_goal *goal;

	/* determine if a new stage should begin depending on which goals we have completed */
	/* contains the indices of up to QI_STAGE_GOALS different QI_GOALS/QI_OPTIONAL goals which are AND'ed;
	   hack: 'optional' indices start after main goals, so if QI_GOALS is 10 (indices 0..9), the first QI_OPTIONAL would have index 10. */
	char goals_for_stage[QI_FOLLOWUP_STAGES][QI_STAGE_GOALS]; /* char to save space, only 1 byte instead of int: returns the goal's index (or -1 if none) */
	s16b next_stage_from_goals[QI_FOLLOWUP_STAGES]; /* <stage> index of the possible follow-up stages */
} qi_stage;

/* Sub-structure: A single quest keyword */
typedef struct qi_keyword {
	char keyword[QI_KEYWORD_LEN];			/* each convo may allow the player to reply with up to m keywords a 30 chars; 'Y' as 1st keyword and 'N' as 2nd trigger a yes/no hack */
	bool questor_ok[QI_QUESTORS];			/* this keyword is valid for which questor(s) ? */
	bool stage_ok[QI_STAGES];			/* this keyword is valid during which stage(s) ? */
	u16b flags;					/* required flags configuration for a keyword to be enabled */

	u16b setflags;					/* ..and the keyword will change flags to these */
	u16b clearflags;				/* ..and the keyword will change flags to these */

	s16b stage;					/* entering this keyword will bring the player to a different quest stage (or -1) */
} qi_keyword;

/* Sub-structure: A single quest keyword-reply (main mem eater) */
typedef struct qi_kwreply {
	byte keyword_idx[QI_KEYWORDS_PER_REPLY];	/* which keyword(s) will prompt this reply from the current questor? */

	byte lines;
	cptr reply[QI_TALK_LINES];			/* give a reply to the keyword (cptr table contains [QI_TALK_LINES])*/

	u16b replyflags[QI_TALK_LINES];			/* only print this particular text line if these flags are matching the quest flags */
} qi_kwreply;

/* Main structure: The complete quest data */
typedef struct quest_info {
	/* -------------------------------- MANDATORY GLOBAL QUEST DATA -------------------------------- */

	/* ----- Dynamic quest state information ----- */

	bool defined;					/* Quest has been loaded from q_info.txt and therefore exists. */
	bool active;					/* QUEST IS CURRENTLY ACTIVE (aka questor is currently spawned - \
							   depends on time (day/night/specific) constraints) */
	bool disabled; 					/* quest has been temporarily disabled, is hence deactivated and cannot \
							   be activated until enabled again (eg for when something breaks during quest progression) */
	bool disabled_on_load;				/* dynamic info for quests disabled via q_info */

	s16b cur_cooldown;				/* in seconds, minimum respawn time for the questor. 0 for 24h default. */
	s32b turn_activated;				/* the turn when the quest became activated */
	s32b turn_acquired;				/* for global quests: the turn when it was acquired */

	/* the current quest stage (-1 is the init stage, which progresses to 0
	   automatically, during which quests are usually acquired by players) */
	s16b cur_stage;					/* the current stage in the quest progress */

	/* global quest flags (a-p to clear, A-P to set) -- note that these are stage-independant! */
	u16b flags;

	/*-----  Fixed quest data (from q_info.txt) ----- */

//#define QI_CODENAME_LEN 10
	char codename[QI_CODENAME_LEN + 1];		/* short, unique, internal name for checking prerequisite quests for follow-up quests */
	char creator[NAME_LEN];				/* credits -- who thought up and created this quest :) */
	//char name[MAX_CHARS];				/* readable title of this quest */
	u16b name;					/* readable title of this quest - offset */

    /* QUESTOR (quest giver) RESTRICTIONS: */
	/* player restrictions */
	byte privilege;					/* quest can only be acquired by admins (for testing them etc) */
	byte minlev, maxlev;				/* eligible player level range (0 for any) */
	u32b races, classes;				/* eligible player classes/races (CFx/RFx flags) */
	bool mode_norm, mode_el, mode_pvp;		/* are these character modes eligible to join? (normal = normal/uw/hell) */
	byte must_be_fruitbat;				/* must be a true fruit bat? (OR's with body_monster!) */
	s16b must_be_monster;				/* must be polymorphed into this form? (OR's with body_fruitbat!) */
	char prerequisites[QI_PREREQUISITES][QI_CODENAME_LEN + 1]; /* prerequisite quests the player must have completed to acquire this quest */

	/* eligible time for quest to become active and thereby spawn the questors */
	bool night, day;				/* Only available at night/day in general? */
	bool morning, forenoon, noon, afternoon, evening, midnight, deepnight; /*  Only available at very specific night/day times? */
	s16b time_start, time_stop;			/* Only available during very specific time interval? */

    /* QUEST DURATION */
	/* quest duration, after it was accepted, until it expires */
	bool individual;				/* quest isn't global, but stage/flags/goals are stored individually for each player,
							   allowing everyone to have his own personal 'instance' of the quest running simultaneusly.
							   For example questors may spawn other questors -> must be global, not individual. */
	s16b repeatable;				/* player may repeat this quest n times (0 = can only do this quest once) */
	s16b cooldown;					/* in seconds, minimum respawn time for the questor. 0 for 24h default. */
	int max_duration;				/* in seconds, 0 for never */
	bool static_floor;				/* questor floor will be static while the quest is active */
	bool quit_floor;				/* if player leaves the questor's floor, the quest will terminate and be lost */
	s16b ending_stage;				/* if this stage is reached, the quest will terminate */
	s16b quest_done_credit_stage;			/* minimum stage that will increase the quest_done counter of players who are pursuing the quest */

	/* -------------------------------- (OPTIONAL) QUESTOR-SPECIFIC DATA -------------------------------- */
	/* Note that each 'normal' quest needs at least one questor,
	   for players being able to interact with to acquire the quest.
	   The exception are quests that are automatically acquired during a stage of another quest. */

	/* ----- Fixed quest data (from q_info.txt) ----- */

	/* questor restrictions (locations etc..): */
	byte questors;					/* how many questors were actually defined in q_info */
	qi_questor *questor;

	/* -------------------------------- OPTIONAL SUB-STRUCTURED DATA -------------------------------- */

	/* ----- Fixed quest data (from q_info.txt) ----- */

	/* amount of different quest stages */
	byte stages;
	byte stage_idx[QI_STAGES];			/* map a stage to a stage[]-index, for example there could be 3 stages in a quest: 0, 1 and 7 :-p */
	qi_stage *stage;

	/* amount of different keywords for player-npc-dialogue */
	int keywords;
	qi_keyword *keyword;

	/* amount of different replies to keywords in player-npc-dialogue */
	int kwreplies;
	qi_kwreply *kwreply;
} quest_info;
