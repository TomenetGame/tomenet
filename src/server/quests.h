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
   not confuse him..		maybe this could be improved on in the future :-p
   how to possibly improve: spawn sub questors with their own dialogues.
*/


//? #define SERVER
#include "angband.h"


#define QI_CODENAME_LEN		10	/* short, internal quest code name */
#define QI_MAX_STAGES		50	/* a quest can have these # of different stages */
#define QI_MAX_KEYWORDS		10	/* for dialogue with the questor */
#define QI_MAX_STAGE_REWARDS 	10	/* max # of rewards handed out per completed stage */
#define QI_GOALS		10	/* main goals to complete a stage */
#define QI_OPTIONAL		10	/* optional goals in a stage */
#define QI_REWARD_GOALS		5	/* up to 5 different main/optional goals that have to be completed for a specific reward */
#define QI_STAGE_GOALS		5	/* up to 5 different main/optional goals that have to be completed for changing to a specific next stage */
#define QI_FOLLOWUP_STAGES	5	/* the # of possible follow-up stages of which one is picked depending on the completed stage goals */


#define QI_SLOC_TOWN		0x1
#define QI_SLOC_SURFACE		0x2
#define QI_SLOC_DUNGEON		0x4
#define QI_SLOC_TOWER		0x8


#define QI_STOWN_BREE		0x001
#define QI_STOWN_GONDOLIN	0x002
#define QI_STOWN_MINASANOR	0x004
#define QI_STOWN_LOTHLORIEN	0x008
#define QI_STOWN_KHAZADDUM	0x010

#define QI_STOWN_WILD		0x020
#define QI_STOWN_DUNGEON	0x040
#define QI_STOWN_IDDC		0x080
#define QI_STOWN_IDDC_FIXED	0x100


#define QI_QUESTOR_RUMOUR	0
#define QI_QUESTOR_NPC		1	/* neutral/friendly monster */
#define QI_QUESTOR_PARCHMENT	2	/* message in a bottle ^^ (read) */
#define QI_QUESTOR_ITEM_PICKUP	3	/* item (pick up) */
#define QI_QUESTOR_ITEM_TOUCH	4	/* item (walk over it) */


#define QI_SPAWN_RING		0	/* spawn around questor */
#define QI_SPAWN_RING_P		1	/* spawn around player */
#define QI_SPAWN_RING_WIDE	2	/* spawn around questor like siege of malice */
#define QI_SPAWN_RING_WIDE_P	3	/* spawn around player like a siege of malice */
#define QI_SPAWN_PACK_NEAR	4	/* spawn as pack nearby questor */
#define QI_SPAWN_PACK_NEAR_P	5	/* spawn as pack nearby player */
#define QI_SPAWN_PACK		6	/* spawn as pack further away */
#define QI_SPAWN_PACK_FAR_P	7	/* spawn as pack far away */


/* Notes:

   Stage changes given as negative numbers will instead add a random value
   of 0..ABS(stage) to current stage, allowing for random quest progression!
   Eg: kill_stage == -3 -> next_stage = current_stage + randint(3).

   codename 'none' is reserved, to indicate that it's not a follow-up quest.

   If a quest stage doesn't have any stage goals, nor any dialogue keywords,
   the quest will terminate after all automatic/timed actions of this stage
   have been done and all eligible rewards have been handed out.

   Items retrieved will be marked as 'quest items' for easy check in case the
   player also has to deliver them somewhere, whether they're the same items.
*/

typedef struct quest_info {
	/* QUEST IS CURRENTLY ACTIVE (aka questor is currently spawned - depends on time (day/night/specific) constraints) */
	bool active;

//#define QI_CODENAME_LEN 10
	char codename[QI_CODENAME_LEN + 1];	/* short, unique, internal name for checking prerequisite quests for follow-up quests */
	char creator[NAME_LEN];		/* credits -- who thought up and created this quest :) */
	//char name[MAX_CHARS];		/* readable title of this quest */
	u16b name;			/* readable title of this quest - offset */


    /* QUESTOR (quest giver) RESTRICTIONS: */
	/* player restrictions */
	int minlev, maxlev;			/* eligible player level range (0 for any) */
	u32b races, classes;			/* eligible player classes/races (CFx/RFx flags) */
	/* matrix of codename(s) of prerequisite quests needed to accept this 'follow-up' quest.
	   x-direction: OR, y-direction: AND */
	char followup_matrix[10][10][QI_CODENAME_LEN + 1];


	/* starting location restrictions */
	byte s_location_type;			/* flags setting elibible starting location types (QI_SLOC_xxx) */
	u16b s_towns_array;			/* QI_SLOC_TOWN: flags setting eligible starting towns (QI_STOWN_xxx) */
	u32b s_terrains;			/* QI_SLOC_SURFACE: flags setting eligible starting terrains (RF8_WILD_xxx, RF8_WILD_TOO_MASK for all) */
	/* exact questor starting location */
	struct worldpos start_wpos;		/* -1, -1 for random */
	int start_x, start_y;			/* -1, -1 for random */

	bool s_dungeon[MAX_D_IDX];		/* QI_SLOC_DUNGEON/TOWER: eligible starting dungeons/towers, or (for Wilderness dungeons): */
	u32b s_dungeon_must_flags1, s_dungeon_must_flags2, s_dungeon_must_flags3;	/*  eligible wilderness dungeon flags */
	u32b s_dungeon_mustnt_flags1, s_dungeon_mustnt_flags2, s_dungeon_mustnt_flags3;	/*  uneligible wilderness dungeon flags */
	bool s_dungeon_iddc;			/*  is the Ironman Deep Dive Challenge an eligible starting point? */
	int dlevmin, dlevmax;			/*  eligible dungeon level or world sector level (0 for any) */

	/* keep track of actual resulting questor location */
	struct worldpos current_wpos;
	int current_x, current_y;

	/* eligible time for quest/questor to spawn and be active */
	bool night, day;			/* Only available at night/day in general? */
	bool morning, forenoon, noon, afternoon, evening, midnight, deepnight; /*  Only available at very specific night/day times? */
	int time_start, time_stop;		/* Only available during very specific time interval? */

	cptr t_pref;				/* filename of map to load, or empty for none */


	/* type of questor */
	int questor;				/* QI_QUESTOR_xxx */

	int questor_ridx;			/* QI_QUESTOR_NPC; 0 for any */
	char questor_rchar;			/*  0 for any */
	byte questor_rattr;			/*  0 for any */
	int questor_rlevmin, questor_rlevmax;	/*  0 for any */

	int questor_sval;			/* QI_QUESTOR_PARCHMENT */

	int questor_ktval, questor_ksval;	/* QI_QUESTOR_ITEM_xxx. No further stats/enchantments are displayed! */

	bool questor_invincible;		/* Is the questor invincible (if monster)/unpickable by monsters (if item)? */
	char questor_name[MAX_CHARS];		/* optional pseudo-unique name that overrides the normal name */


    /* QUEST DURATION */
	/* quest duration, after it was accepted, until it expires */
	int max_duration;			/* in seconds, 0 for never */
	int cooldown;				/* in seconds, minimum respawn time for the questor. 0 for 24h default. */
	bool per_player;			/* this quest isn't global but can be done by each player individually.
						   For example questors may spawn other questors -> must be global, not per_player. */
	bool static_floor;			/* questor floor will be static while the quest is active */
	bool quit_floor;			/* if player leaves the questor's floor, the quest will terminate and be lost */


    //NOTE: these should instead be hard-coded, similar to global events, too much variety really.. (monster spawning etc):
    /* QUEST CONSEQUENCES (depend on quest stage the player is in) --
	stage action #0 is the initial one, when the QUESTOR spawns (invalid for action_talk[]),
	stage action #1 is, when a player comes in LoS,
	stage action #2 is, when a player accepts interacted with the questor. */

	/* quest initialisation and meta actions */
	bool accept_los, accept_interact;	/* player gets the quest just be being in LoS / interacting once with the questor (bump/read the parchment/pickup the item) */

	int activate_quest[QI_MAX_STAGES];	/* spawn a certain new quest of this index (and thereby another questor) (if not already existing) */
	bool auto_accept[QI_MAX_STAGES];	/* player will automatically acquire the newly spawned quest (from activate_quest[]) */
	bool auto_accept_quiet[QI_MAX_STAGES];	/* player will automatically acquire the newly spawned quest (from activate_quest[]) but not get a quest-accept-notification type of message about it */

	int change_stage[QI_MAX_STAGES];		/* automatically change to a different stage after handling everything that was to do in the current stage */
	int timed_stage_ingame[QI_MAX_STAGES];		/* automatically change to a different stage after a certain amount of in-game minutes passed */
	int timed_stage_ingame_abs[QI_MAX_STAGES];	/* automatically change to a different stage after a certain in-game time is reached */
	int timed_stage_real[QI_MAX_STAGES];		/* automatically change to a different stage after a certain amount of real seconds passed */


	/* special questor behaviour during each stage */
	bool questor_invincible_new[QI_MAX_STAGES];	/* Is the questor invincible (if monster)/unpickable by monsters (if item) during a particular stage? */
	bool questor_death_fail[QI_MAX_STAGES];		/* If the questor dies, the quest state fails? (->reset stage goals/positions as if we just entered it, if that is possible? hm) */
	bool questor_death_fail_all[QI_MAX_STAGES];	/* If the questor dies, the quest fails completely? */
	cptr questor_name_new[QI_MAX_STAGES];		/* questor changes optional pseudo-unique name during this stage? */
	int questor_ridx_new[QI_MAX_STAGES]; 		/* questor changes to this base monster type */
	char questor_rchar_new[QI_MAX_STAGES];
	byte questor_rattr_new[QI_MAX_STAGES];
	int questor_rlev_new[QI_MAX_STAGES];

	bool questor_talkable[QI_MAX_STAGES];		/* questor accepts dialogue? (by bumping usually) */
	bool questor_despawned[QI_MAX_STAGES];		/* questor vanishes during a quest stage? */

	int questor_walk_speed[QI_MAX_STAGES];		/* questor will actually move around during this stage? */
	int questor_walk_destx[QI_MAX_STAGES], questor_walk_desty[QI_MAX_STAGES]; /* target waypoint for questor to move to */
	int questor_walk_stage[QI_MAX_STAGES];		/* stage will change when questor arrives at destination */

	struct worldpos teleport_questor_wpos[QI_MAX_STAGES];	/* teleport questor to a new position */
	int teleport_questor_x[QI_MAX_STAGES], teleport_questor_y[QI_MAX_STAGES];
	struct worldpos teleport_wpos[QI_MAX_STAGES];		/* teleport participating player to a new position */
	int teleport_player_x[QI_MAX_STAGES], teleport_player_y[QI_MAX_STAGES];

	int questor_hostile[QI_MAX_STAGES];		/* questor turns into a normal aggressor, and stage is changed */
	int questor_hostile_revert_hp[QI_MAX_STAGES];	/* aggressor-questor turns back into a non-aggressive questor when falling to <= HP (death prevented!) and stage is changed */
	int questor_hostile_revert_timed_ingame[QI_MAX_STAGES]; /* ..after ingame time (min).. */
	int questor_hostile_revert_timed_ingame_abs[QI_MAX_STAGES]; /* ..at ingame time.. */
	int questor_hostile_revert_timed_real[QI_MAX_STAGES]; /* ..after real time (s).. */


	/* quest dialogues and responses/consequences (stage 0 means player loses the quest again) */
	//NOTE: '$RPM' in dialogue will be substituted by xxx_random_pick'ed monster criteria
	//NOTE: '$OPM' in dialogue will be substituted by xxx_random_pick'ed object criteria
	cptr talk[QI_MAX_STAGES][10];				/* n conversations a 10 lines a 79 characters */
	cptr keywords[QI_MAX_STAGES][QI_MAX_KEYWORDS];		/* each convo may allow the player to reply with up to m keywords a 30 chars */
	int keywords_stage[QI_MAX_STAGES][QI_MAX_KEYWORDS];	/*  ..which will bring the player to a different quest stage */
	char yn[QI_MAX_STAGES];					/* each convo may allow the player to reply with yes or no (NOTE: could just be done with keywords too, actually..) */
	int y_stage[QI_MAX_STAGES], n_stage[QI_MAX_STAGES];	/*  ..which will bring the player to a different quest stage */

	cptr narration[QI_MAX_STAGES][10];			/* display a quest-progress narration when this stage starts, a 10 lines a 79 characters, aka "You have arrived at the lake!" */


	/* create a dungeon/tower for a quest stage? completely static? predefined layouts? */
	struct worldpos add_dungeon_wpos[QI_MAX_STAGES];	/* create it at this world pos */
	bool add_dungeon_terrain_patch[QI_MAX_STAGES][QI_GOALS];/* extend valid location over all connected world sectors whose terrain is of the same type (eg big forest) */
	char *add_dungeon_parms[QI_MAX_STAGES];			/* same as for master_level() maybe */
	char **add_dungeon_t_pref[QI_MAX_STAGES];		/* table of predefined template filenames for each dungeon floor (or NULL for random floors inbetween) */
	bool add_dungeon_fullystatic[QI_MAX_STAGES];		/* all floors are static */
	bool add_dungeon_keep[QI_MAX_STAGES];			/* keep dungeon until quest ends instead of erasing it when this stage is completed */


	/* quest goals, up to 10 per stage, with a multitude of different sub-goals (Note: of the subgoals 1 is randomly picked for the player, except if 'xxx_random_pick' is set, which allows the player to pick what he wants to do).
	   There are additionally up to 10 optional quest goals per stage.
	   --note: the #s of subgoals don't use #defines, because they vary too much anyway for each category, so they're just hard-coded numbers. */
//#define QI_GOALS 10 /* main goals to complete a stage */
//#define QI_OPTIONAL 10 /* optional goals in a stage */
	bool kill_player_picks[QI_MAX_STAGES][QI_GOALS];		/* instead of picking one of the eligible monster criteria randomly, let the player decide which he wants to get */
	int kill_ridx[QI_MAX_STAGES][QI_GOALS][20];			/* kill certain monster(s) */
	char kill_rchar[QI_MAX_STAGES][QI_GOALS][5];			/*  ..certain types */
	byte kill_rattr[QI_MAX_STAGES][QI_GOALS][5];			/*  ..certain colours */
	int kill_rlevmin[QI_MAX_STAGES], kill_rlevmax[QI_MAX_STAGES][QI_GOALS];	/* 0 for any */
	int kill_number[QI_MAX_STAGES][QI_GOALS];
	int kill_spawn[QI_MAX_STAGES][QI_GOALS], kill_spawn_loc[QI_MAX_STAGES][QI_GOALS];	/* actually spawn the monster(s) nearby! (QI_SPAWN_xxx) */
	bool kill_spawn_targets_questor[QI_MAX_STAGES][QI_GOALS];	/* the spawned mobs go for the questor primarily */
	int kill_stage[QI_MAX_STAGES][QI_GOALS];			/* switch to a different quest stage on defeating the monsters */

	bool retrieve_player_picks[QI_MAX_STAGES][QI_GOALS];		/* instead of picking one subgoal randomly, let the player decide which he wants to get */
	int retrieve_otval[QI_MAX_STAGES][QI_GOALS][20], retrieve_osval[QI_MAX_STAGES][QI_GOALS][20];	/* retrieve certain item(s) */
	int retrieve_opval[QI_MAX_STAGES][QI_GOALS][5], retrieve_obpval[QI_MAX_STAGES][QI_GOALS][5];
	byte retrieve_oattr[QI_MAX_STAGES][QI_GOALS][5];		/*  ..certain colours (flavoured items only) */
	int retrieve_oname1[QI_MAX_STAGES][QI_GOALS][20], retrieve_oname2[QI_MAX_STAGES][QI_GOALS][20], retrieve_oname2b[QI_MAX_STAGES][QI_GOALS][20];
	int retrieve_ovalue[QI_MAX_STAGES][QI_GOALS][20];
	int retrieve_number[QI_MAX_STAGES][QI_GOALS][20];
	int retrieve_stage[QI_MAX_STAGES][QI_GOALS];			/* switch to a different quest stage on retrieving the items */

	struct worldpos target_wpos[QI_MAX_STAGES][QI_GOALS];		/* kill/retrieve specifically at this world pos */
	int target_pos_x[QI_MAX_STAGES][QI_GOALS], target_pos_y[QI_MAX_STAGES][QI_GOALS]; /* at specifically this position (even usable for kill/retrieve stuff?) */
	bool target_terrain_patch[QI_MAX_STAGES][QI_GOALS];		/* extend valid target location over all connected world sectors whose terrain is of the same type (eg big forest) */

	struct worldpos deliver_wpos[QI_MAX_STAGES][QI_GOALS];		/* (after optionally killing/retrieving/or whatever) MOVE TO this world pos */
	int deliver_pos_x[QI_MAX_STAGES][QI_GOALS], deliver_pos_y[QI_MAX_STAGES][QI_GOALS]; /* -"- ..MOVE TO specifically this position */
	bool deliver_terrain_patch[QI_MAX_STAGES][QI_GOALS];		/* extend valid target location over all connected world sectors whose terrain is of the same type (eg big forest) */


	bool killopt_player_picks[QI_MAX_STAGES][QI_OPTIONAL];		/* instead of picking one of the eligible monster criteria randomly, let the player decide which he wants to get */
	int killopt_ridx[QI_MAX_STAGES][QI_OPTIONAL][20];		/* kill certain monster(s) */
	char killopt_rchar[QI_MAX_STAGES][QI_OPTIONAL][5];		/*  ..certain types */
	byte killopt_rattr[QI_MAX_STAGES][QI_OPTIONAL][5];		/*  ..certain colours */
	int killopt_rlevmin[QI_MAX_STAGES][QI_OPTIONAL], killopt_rlevmax[QI_MAX_STAGES][QI_OPTIONAL];	/* 0 for any */
	int killopt_number[QI_MAX_STAGES][QI_OPTIONAL];
	int killopt_spawn[QI_MAX_STAGES][QI_OPTIONAL], killopt_spawn_loc[QI_MAX_STAGES][QI_OPTIONAL];	/* actually spawn the monster(s) nearby! (QI_SPAWN_xxx) */
	bool killopt_spawn_targets_questor[QI_MAX_STAGES][QI_OPTIONAL];	/* the spawned mobs go for the questor primarily */
	int killopt_stage[QI_MAX_STAGES][QI_OPTIONAL];			/* switch to a different quest stage on defeating the monsters */

	bool retrieveopt_player_picks[QI_MAX_STAGES][QI_OPTIONAL];	/* instead of picking one subgoal randomly, let the player decide which he wants to get */
	int retrieveopt_otval[QI_MAX_STAGES][QI_OPTIONAL][20], retrieveopt_osval[QI_MAX_STAGES][QI_OPTIONAL][20];	/* retrieve certain item(s) */
	int retrieveopt_opval[QI_MAX_STAGES][QI_OPTIONAL][5], retrieveopt_obpval[QI_MAX_STAGES][QI_OPTIONAL][5];
	byte retrieveopt_oattr[QI_MAX_STAGES][QI_OPTIONAL][5];		/*  ..certain colours (flavoured items only) */
	int retrieveopt_oname1[QI_MAX_STAGES][QI_OPTIONAL][20], retrieveopt_oname2[QI_MAX_STAGES][QI_OPTIONAL][20], retrieveopt_oname2b[QI_MAX_STAGES][QI_OPTIONAL][20];
	int retrieveopt_ovalue[QI_MAX_STAGES][QI_OPTIONAL][20];
	int retrieveopt_number[QI_MAX_STAGES][QI_OPTIONAL][20];
	int retrieveopt_stage[QI_MAX_STAGES][QI_OPTIONAL];		/* switch to a different quest stage on retrieving the items */

	struct worldpos targetopt_wpos[QI_MAX_STAGES][QI_OPTIONAL];	/* kill/retrieve specifically at this world pos */
	int targetopt_pos_x[QI_MAX_STAGES][QI_OPTIONAL], targetopt_pos_y[QI_MAX_STAGES][QI_OPTIONAL]; /* at specifically this position (hm does this work for kill/retrieve stuff? pft) */
	bool targetopt_terrain_patch[QI_MAX_STAGES][QI_OPTIONAL];	/* extend valid target location over all connected world sectors whose terrain is of the same type (eg big forest) */

	struct worldpos deliveropt_wpos[QI_MAX_STAGES][QI_GOALS];	/* (after optionally killing/retrieving/or whatever) MOVE TO this world pos */
	int deliveropt_pos_x[QI_MAX_STAGES][QI_GOALS], deliveropt_pos_y[QI_MAX_STAGES][QI_GOALS]; /* -"- ..MOVE TO specifically this position */
	bool deliveropt_terrain_patch[QI_MAX_STAGES][QI_GOALS];		/* extend valid target location over all connected world sectors whose terrain is of the same type (eg big forest) */


#if 0 /* too large! */
	/* quest rewards - multiple items per stage possible,
	   each determined by the 'quest goals & optional quest goals' matrix:
	   Up to 10 optional quest goals per stage possible. */
//#define QI_MAX_STAGE_REWARDS 10
	bool reward_optional_matrix[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS][QI_GOALS + QI_OPTIONAL][QI_GOALS + QI_OPTIONAL]; /* main/optional quest goals needed for this reward, x-direction=OR, y-direction=AND */
#else
	/* contains the indices of up to QI_REWARD_GOALS different QI_GOALS/QI_OPTIONAL goals which are AND'ed;
	   hack: 'optional' indices start after main goals, so if QI_GOALS is 10, the first QI_OPTIONAL would have index 11. */
//#define QI_REWARD_GOALS 5 /* up to 5 different main/optional goals that have to be completed for a specific reward */
	char goals_for_reward[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS][QI_REWARD_GOALS];	/* char to save space, only 1 byte instead of int: returns the goal's index (or -1 if none) */
#endif

	int reward_otval[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS];		/* hand over certain rewards to the player */
	int reward_osval[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS];
	int reward_opval[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS], reward_obpval[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS];
	int reward_oname1[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS], reward_oname2[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS], reward_oname2b[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS];
	bool reward_ogood[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS], reward_ogreat[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS];
	bool reward_oreward[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS];	/*  use fitting-reward algo (from highlander etc)? */

	int reward_gold[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS];
	int reward_exp[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS];
	int reward_statuseffect[QI_MAX_STAGES][QI_MAX_STAGE_REWARDS];	/* debuff (aka curse, maybe just for show)/un-debuff/tempbuff player? */


#if 0 /* too large! also we want maybe different goals -> different next stages */
	/* determine how the main goals have to be completed to advance to next stage,
	   x-direction: OR, y-direction: AND */
	bool stage_complete_matrix[QI_MAX_STAGES][QI_GOALS][QI_GOALS];	/* quest goals needed to complete a stage, x-direction=OR, y-direction=AND */
#else
	bool return_to_questor[QI_MAX_STAGES][QI_GOALS];		/* do we need to return to the questor first (bump), to get credit for particular main goals? */
	bool return_to_questor_opt[QI_MAX_STAGES][QI_OPTIONAL];		/* do we need to return to the questor first (bump), to get credit for particular optional goals? */
	/* determine if a new stage should begin depending on which goals we have completed */
	/* contains the indices of up to QI_STAGE_GOALS different QI_GOALS/QI_OPTIONAL goals which are AND'ed;
	   hack: 'optional' indices start after main goals, so if QI_GOALS is 10 (indices 0..9), the first QI_OPTIONAL would have index 10. */
//#define QI_STAGE_GOALS 5 /* up to 5 different main/optional goals that have to be completed for changing to a specific next stage */
//#define QI_FOLLOWUP_STAGES 5 /* the # of possible follow-up stages of which one is picked depending on the completed stage goals */
	char goals_for_stage[QI_MAX_STAGES][QI_FOLLOWUP_STAGES][QI_STAGE_GOALS];	/* char to save space, only 1 byte instead of int: returns the goal's index (or -1 if none) */
	int next_stage_from_goals[QI_MAX_STAGES][QI_FOLLOWUP_STAGES]; 			/* <stage> index of the possible follow-up stages */
#endif
} quest_info;
