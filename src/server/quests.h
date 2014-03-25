/* This is the header file for quests.c, please look there for more information.
   You may modify/use it freely as long as you give proper credit. - C. Blue
*/

#include "angband.h"

/* Notes:

   Stage changes given as negative numbers will instead add a random value
   of 0..ABS(stage) to current stage, allowing for random quest progression!
   Eg: kill_stage == -3 -> next_stage = current_stage + randint(3).

   If a quest stage doesn't have any stage goals, nor any dialogue keywords
   that can change the stage,
   the quest will terminate after all automatic/timed actions of this stage
   have been done and all eligible rewards have been handed out.
   ++including: questor movement/teleport/revert-from-hostile and
   timed and instant stage-change effects.

   Items retrieved will be marked as 'quest items' for easy check in case the
   player also has to deliver them somewhere, whether they're the same items.
   Quest items cannot stack with other items.


   Data structure:
   quest_info -> qi_questor -> qi_location
              -> qi_stage   -> qi_goal              -> qi_kill
                                                    -> qi_retrieve
                                                    -> qi_deliver
                            -> qi_reward
                            -> qi_questitem         -> qi_location
                            -> qi_feature
                            -> qi_questor_morph
                            -> qi_questor_hostility
                            -> qi_questor_act
              -> qi_keyword
              -> qi_kwreply
*/


/* Sub-structure: Hold broad information about generating a specific spawn position randomly. */
typedef struct qi_location {
	/* spawn location info */
	byte s_location_type;				/* flags setting elibible starting location types (QI_SLOC_xxx) */
	u16b s_towns_array;				/* QI_SLOC_TOWN: flags setting eligible starting towns (QI_STOWN_xxx) */
	u32b s_terrains;				/* QI_SLOC_SURFACE: flags setting eligible starting terrains (RF8_WILD_xxx, RF8_WILD_TOO_MASK for all) */
	bool terrain_patch;				/* extend spawn location to nearby worldmap sectors if same terrain? */
	byte radius;					/* offset start_x, start_y loc randomly within a radius? */

	/* exact spawn location info */
	struct worldpos start_wpos;			/* -1, -1 for random */
	s16b start_x, start_y;				/* -1, -1 for random */

	/* dungeons eligible too? */
	bool s_dungeon[MAX_D_IDX];			/* QI_SLOC_DUNGEON/TOWER: eligible starting dungeons/towers, or (for Wilderness dungeons): */
	u32b s_dungeon_must_flags1, s_dungeon_must_flags2, s_dungeon_must_flags3;	/*  eligible wilderness dungeon flags */
	u32b s_dungeon_mustnt_flags1, s_dungeon_mustnt_flags2, s_dungeon_mustnt_flags3;	/*  uneligible wilderness dungeon flags */
	bool s_dungeon_iddc;				/* is the Ironman Deep Dive Challenge an eligible starting point? */
	byte dlevmin, dlevmax;				/* eligible dungeon level or world sector level (0 for any) */

	/* specific map design? */
	cptr tpref;					/* filename of map to load, or empty for none */
	int tpref_x, tpref_y;				/* x, y offset for loading small map parts */
} qi_location;

/* Sub-structure: Mandatory questor information.
   Quests usually need at least one questor to be acquirable by interacting with it.
   An exception are quests that are automatically spawned by other quests and auto-acquired. */
typedef struct qi_questor {
	/*-----  Fixed questor spawn information (from q_info.txt) ----- */

	/* quest initialisation and meta actions */
	bool accept_los, accept_interact;		/* player gets the quest just be being in LoS / interacting once with the questor (bump/read the parchment/pickup the item) */

	bool static_floor;				/* questor floor will be static while the quest is active */
	bool quit_floor;				/* if player leaves the questor's floor, the quest will terminate and be lost */

	qi_location q_loc;				/* spawn location parameters */

	/* type of questor */
	byte type;					/* QI_QUESTOR_xxx */

	/* QI_QUESTOR_NPC */
	s16b ridx;
	char rchar;
	byte rattr;
	byte rlevmin, rlevmax;

	/* QI_QUESTOR_PARCHMENT */
	s16b psval, plev;

	/* QI_QUESTOR_ITEM_xxx. */
	/* No further stats/enchantments are displayed maybe? */
	s16b otval, osval, opval, obpval, oname1, oname2, oname2b;
	bool ogood, ogreat, overygreat;
	s16b olev;
	byte oattr;

	char name[MAX_CHARS];				/* optional pseudo-unique name that overrides the normal name */

	bool talkable;					/* questor initially starts accepting dialogue? (by bumping usually) */
	bool despawned;					/* questor initially starts despawned? */
	bool invincible;				/* questor initially starts invincible (if monster)/unpickable by monsters (if item) on spawn? */
	s16b death_fail;				/* If the questor dies, the quest goes to stage n? (->reset old stage goals/positions as if we just entered it, if that is possible? hm)
							   -1 = quest fails completely, 255 = questor death has no effect. */

	/* ..if killable ie not invincible: */
	byte drops;					/* 0=none, 1=Drops regular loot (of his ridx type) instead of nothing?, 2=specific, 3=1+2 */
	s16b drops_tval;				/* hand over certain rewards to the player */
	s16b drops_sval;
	s16b drops_pval, drops_bpval;
	s16b drops_name1, drops_name2, drops_name2b;
	bool drops_good, drops_great, drops_vgreat;
	byte drops_reward;				/*  use fitting-reward algo (from highlander etc)? - 0..5 */
	int drops_gold;
	int exp;

	/* ----- Dynamic questor information ----- */

	/* keep track of actual resulting questor location --
	   this data gets generated dynamically on quest activation from q_loc */
	struct worldpos current_wpos;
	s16b current_x, current_y;

	s16b mo_idx; /* union of m_idx and o_idx :-p */

	s16b talk_focus;				/* questor is focussed on this player and won't give others a chance to reply with keywords (non-individual quests only) */
} qi_questor;

/* Sub-structure: Questor changes (or turns vulnerable) ('S') */
typedef struct qi_questor_morph {
	/* special questor behaviour during each stage */
	bool talkable;					/* questor accepts dialogue? (by bumping usually) */
	bool despawned;					/* questor vanishes during a quest stage? */

	bool invincible;				/* Is the questor invincible (if monster)/unpickable by monsters (if item) during a particular stage? */
	s16b death_fail;				/* If the questor dies, the quest goes to stage n? (->reset old stage goals/positions as if we just entered it, if that is possible? hm)
							   -1 = quest fails completely, 255 = no effect */
	cptr name;					/* questor changes optional pseudo-unique name during this stage? */
	s16b ridx; 					/* questor changes to this base monster type */
	char rchar;
	byte rattr;
	byte rlev;
} qi_questor_morph;

/* Sub-structure: Questor moves himself or the player ('S'/'H') */
typedef struct qi_questor_hostility {
	bool unquestor;					/* questor actually loses questor status and turns into a regular mob!
							   NOTE: This will override all hostile_... options below.
							         Stage change may still happen, it'll be instantly. */

	bool hostile_player;				/* questor turns into a normal aggressor, and optionally, stage is changed when he is defeated or after a time */
	bool hostile_monster;				/* questor turns hostile to monsters, and optionally, stage is changed when he is defeated or after a time */

	s16b hostile_revert_hp;				/* aggressor-questor turns back into a non-aggressive questor when falling to <= HP (death prevented!) and stage is changed */
#if 0 /* currently not possible since we call the quest scheduler once a minute */
	s16b hostile_revert_timed_ingame;		/* ..after ingame time (min).. */
	s16b hostile_revert_timed_ingame_abs;		/* ..at ingame time.. */
#else
	s16b hostile_revert_timed_ingame_abs;		/* ..when a certain in-game time is reached (HOUR resolution! -1 to disable) */
#endif
	s16b hostile_revert_timed_real;			/* ..after real time (s).. */

	s16b change_stage;				/* new stage after hostility has ceased (255 for none) */
	bool quiet_change;				/* for the above stage-change: don't replay the stage's dialogue */

	/* dynamic timer helper data */
	s16b hostile_revert_timed_countdown;		/* dynamic, for countdown for above timings: negative value = ingame absolute, positive value = real-time counting down */
	s16b hostile_revert_timed_countdown_stage;
} qi_questor_hostility;

/* Sub-structure: Questor moves himself or the player ('J') */
typedef struct qi_questor_act {
	struct worldpos tp_wpos;			/* teleport self to a new position */
	s16b tp_x, tp_y;

	struct worldpos tpy_wpos;			/* teleport participating players to a new position */
	s16b tpy_x, tpy_y;

	byte walk_speed;				/* questor will actually move around during this stage? */
	s16b walk_destx, walk_desty;			/* target waypoint for questor to move to */
	s16b change_stage;				/* stage will change when questor arrives at destination */
	bool quiet_change;				/* for the above stage-change: don't replay the stage's dialogue */

	struct worldpos teleport_wpos;			/* teleport questor to a new position */
	s16b teleport_x, teleport_y;
} qi_questor_act;

/* Quest goals, with a multitude of different sub-goals. Some AND with each other, some OR.
   There are three types of goals: Kill, Retrieve (obtain an object), Deliver (this is to
   travel to a specific location after finishing all kill/retrieve goals, or just to travel
   there if there are no kill/retrieve goals in this stage). */

/* Sub-structure: A single kill goal */
typedef struct qi_kill {
#if 0 /* too much, make it simpler for now */
	bool player_picks;				/* instead of picking one of the eligible monster criteria randomly, let the player decide which he wants to get */
#endif
	s16b ridx[10];					/* kill certain monster(s), 0 for none, -1 for any. */
	char rchar[5];					/*  ..certain types, 254 for any, 255 for none. AND's with attr/lev. */
	byte rattr[5];					/*  ..certain colours, 254 for any, 255 for none. AND's with char/lev. */
	byte rlevmin, rlevmax;				/* 0 for any. AND's with char/attr. */

	s16b number;

	byte spawn;					/* actually spawn the monster(s) nearby/in the target zone! (QI_SPAWN_xxx) */
	byte spawn_targets;				/* the spawned mobs go for 0=any players (normal monster AI) 1=the player who talked to the questor 2=questor */

	s16b number_left;	//dynamic data		/* keep track of how many are left to kill */
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
	byte radius;

	byte return_to_questor;				/* do we need to return to a questor (bump) to get credit? */

	cptr tpref;					/* filename of map to load, or empty for none */
	int tpref_x, tpref_y;				/* x, y offset for loading small map parts */
} qi_deliver;

/* Sub-structure: A single quest goal (which can be kill/retrieve/deliver) */
typedef struct qi_goal {
	/* Type of goal - exactly one of them must be non-NULL */
	qi_kill *kill;
	qi_retrieve *retrieve;
	qi_deliver *deliver;

	bool optional;					/* has no effect on stage completion */

	bool cleared;		//dynamic data		/* goal has been fulfilled! */
	bool nisi;		//dynamic data		/* for goals set by kill/retrieve depending on deliver (for flag changes) */

	/* 'Z' lines: goals set/clear flags */
	u16b setflags;
	u16b clearflags;

	/* for kill/retrieve goals only (deliver goals have/need separate location info) */
	bool target_pos;				/* enable target pos? */
	struct worldpos target_wpos;			/* kill/retrieve specifically at this world pos */
	bool target_terrain_patch;			/* extend valid target location over all connected world sectors whose terrain is of the same type (eg big forest)
							   max radius is QI_TERRAIN_PATCH_RADIUS. */
	s16b target_pos_x, target_pos_y;		/* at specifically this position (even usable for kill/retrieve stuff?) */
	byte target_pos_radius;

	cptr target_tpref;				/* filename of map to load, or empty for none */
	int target_tpref_x, target_tpref_y;		/* x, y offset for loading small map parts */
} qi_goal;

/* Sub-structure: A single quest reward.
   It can actually be a combination of the three things 'item', 'gold' and 'exp' in one, if desired.
   The fourth type 'statuseffect' requires special implementation. (Could be some sort of buff/curse.) */
typedef struct qi_reward {
	s16b otval;					/* hand over certain rewards to the player */
	s16b osval;
	s16b opval, obpval;
	s16b oname1, oname2, oname2b;
	bool ogood, ogreat, ovgreat;
	byte oreward;					/* use fitting-reward algo (from highlander etc)? - 0..5 */

	int gold;
	int exp;
	s16b statuseffect;				/* debuff (aka curse, maybe just for show)/un-debuff/tempbuff player? */
} qi_reward;

/* Sub-structure: A special quest item <TV_SPECIAL, SV_QUEST>.
   that will be generated on a specific target location somewhere in the world.
   Since the item is completely blank, all these values are mandatory to
   specify. Note that its location info ('Bl' line) is actually the same as for
   questors ('L' line).
   Its pval has no effect, except for allowing us to distinguish between
   multiple quest items in retrieval quest goals. */
typedef struct qi_questitem {
	s16b opval;					/* only used to distinguish between them (by retrieve-goals) */
	char ochar;
	byte oattr;
	s16b oweight;
	byte olev;
	char name[MAX_CHARS]; //could use ONAME_LEN

	byte questor_gives;				/* Do not spawn it anywhere but just hand it over on interaction with this questor */

	qi_location q_loc;				/* spawn location parameters */

	/* ----- Dynamic data ----- */

	/* keep track of actual resulting quest item spawn location --
	   this data gets generated dynamically on quest item generation from q_loc */
	struct worldpos result_wpos;
	s16b result_x, result_y;
} qi_questitem;

/* Sub-structure: A single grid feature that is imprinted ('built') onto the
   map at a specific wpos (either directly specified or derived from a questor)
   and a specific x,y loc, automatically on stage startup. */
typedef struct qi_feature {
	byte wpos_questor;				/* use the current wpos of one of our questors? (overrides) */
	byte wpos_questitem;				/* use the wpos of one of our quest items from this stage? (overrides) */
	struct worldpos wpos;				/* use a specific wpos */

	s16b x, y;					/* the cave pos to change */

	byte feat;					/* the f_info.txt feat to build */
} qi_feature;

/* Sub-structure: A single quest stage.
   The central unit of a quest, providing goals, rewards and dialogue.
   A quest consists of a series of stages that switch between each other
   depending on the player fulfilling their goals or reacting to their
   dialogue, until an ending stage is reached and the quest terminates. */
typedef struct qi_stage {
	/* quest acceptance */
	bool accepts;					/* player can acquire the quest during a stage */

	s16b activate_quest;				/* spawn a certain new quest of this index (and thereby another questor) (if not already existing) -1 for disabled */
	bool auto_accept;				/* player will automatically acquire the newly spawned quest (from activate_quest[]) */
	bool auto_accept_quiet;				/* player will automatically acquire the newly spawned quest (from activate_quest[]) but not get a quest-accept-notification type of message about it */


	/* stage-change automatics */
	s16b change_stage;				/* automatically change to a different stage after handling everything that was to do in the current stage (-1 = disable) */
#if 0 /* currently not possible since we call the quest scheduler once a minute */
	s16b timed_ingame;				/* automatically change to a different stage after a certain amount of in-game minutes passed */
	s16b timed_ingame_abs;				/* automatically change to a different stage when a certain in-game time is reached (minute resolution) */
#else
	s16b timed_ingame_abs;				/* automatically change to a different stage when a certain in-game time is reached (HOUR resolution! -1 to disable) */
#endif
	s16b timed_real;				/* automatically change to a different stage after a certain amount of real minutes passed */
	bool quiet_change;				/* for the above auto-changes: don't replay the stage's dialogue */

	/* dynamic timer helper data */
	s16b timed_countdown;				/* dynamic, for countdown for above timings: negative value = ingame absolute, positive value = real-time counting down */
	s16b timed_countdown_stage;
	bool timed_countdown_quiet;

	u16b setflags;					/* these flags will automatically be set on stage start */
	u16b clearflags;				/* these flags will automatically be cleared on stage start */

	/* cave grid features to be built automatically on stage start */
	byte feats;
	qi_feature *feat;


	/* create a dungeon/tower for a quest stage? completely static? predefined layouts? */
	struct worldpos add_dungeon_wpos;		/* create it at this world pos */
	bool add_dungeon_terrain_patch;			/* extend valid location over all connected world sectors whose terrain is of the same type (eg big forest) */
	char *add_dungeon_parms;			/* same as for master_level() maybe */
	char **add_dungeon_t_pref;			/* table of predefined template filenames for each dungeon floor (or NULL for random floors inbetween) */
	bool add_dungeon_fullystatic;			/* all floors are static */
	bool add_dungeon_keep;				/* keep dungeon until quest ends instead of erasing it when this stage is completed */


	/* Questor going bonkers? (optional/advanced) */
	qi_questor_morph *questor_morph[QI_QUESTORS];
	qi_questor_hostility *questor_hostility[QI_QUESTORS];
	qi_questor_act *questor_act[QI_QUESTORS];


	/* quest dialogues and responses/consequences (stage 0 means player loses the quest again) */
	//NOTE: '$RPM' in dialogue will be substituted by xxx_random_pick'ed monster criteria
	//NOTE: '$OPM' in dialogue will be substituted by xxx_random_pick'ed object criteria
	bool talk_examine[QI_QUESTORS];			/* questor doesn't "talk" but rather the text claims that "you are examining <questor>" (for item questors or "dead" questors) */
	byte talk_lines[QI_QUESTORS];
	cptr talk[QI_QUESTORS][QI_TALK_LINES]; 		/* n conversations a 10 lines a 79 characters */
	u16b talk_flags[QI_QUESTORS][QI_TALK_LINES];	/* required flags configuration for a convo line to get displayed  */

	byte narration_lines;
	cptr narration[QI_TALK_LINES];			/* display a quest-progress narration when this stage starts, a 10 lines a 79 characters, aka "You have arrived at the lake!" */
	u16b narration_flags[QI_TALK_LINES];		/* required flags configuration to display this narrative line */


	/* the rewards for this stage, if any */
	byte rewards; /* up to QI_STAGE_REWARDS */
	qi_reward *reward;

	/* contains the indices of up to QI_REWARD_GOALS different QI_GOALS goals which are AND'ed */
	s16b goals_for_reward[QI_STAGE_REWARDS][QI_REWARD_GOALS]; /* returns the goal's index (or -1 if none) */


	/* the goals for this stage */
	byte goals; /* up to QI_STAGE_GOALS */
	qi_goal *goal;

	/* determine if a new stage should begin depending on which goals we have completed */
	/* contains the indices of up to QI_STAGE_GOALS different QI_GOALS goals which are AND'ed; 'optional' goals are skipped in the check! */
	s16b goals_for_stage[QI_FOLLOWUP_STAGES][QI_STAGE_GOALS]; /* returns the goal's index (or -1 if none) */
	s16b next_stage_from_goals[QI_FOLLOWUP_STAGES]; /* <stage> index of the possible follow-up stages */

	/* amout of special quest items to spawn */
	byte qitems;
	qi_questitem *qitem;
} qi_stage;

/* Sub-structure: A single quest keyword.
   A possible player response to a quest dialogue, which may cause
   a change in quest stage. */
typedef struct qi_keyword {
	char keyword[QI_KEYWORD_LEN];			/* each convo may allow the player to reply with up to m keywords a 30 chars; 'Y' as 1st keyword and 'N' as 2nd trigger a yes/no hack */
	bool questor_ok[QI_QUESTORS];			/* this keyword is valid for which questor(s) ? */
	bool stage_ok[QI_STAGES], any_stage;		/* this keyword is valid during which stage(s) ? any_stage is an extra marker for quest_dialogue() */
	u16b flags;					/* required flags configuration for a keyword to be enabled */

	u16b setflags;					/* ..and the keyword will change flags to these */
	u16b clearflags;				/* ..and the keyword will change flags to these */

	s16b stage;					/* entering this keyword will bring the player to a different quest stage (or -1) */
} qi_keyword;

/* Sub-structure: A single quest keyword-reply (main mem eater).
   An automatic reply the player gets when he enters a certain keyword
   (replies can be valid for multiple keywords each, actually). */
typedef struct qi_kwreply {
	byte keyword_idx[QI_KEYWORDS_PER_REPLY];	/* which keyword(s) will prompt this reply from the current questor? */
	bool questor_ok[QI_QUESTORS];			/* this keyword reply is valid for which questor(s) ? */
	bool stage_ok[QI_STAGES];			/* this keyword reply is valid during which stage(s) ? */
	u16b flags;					/* this keyword reply is only valid if the set flags match? */

	byte lines;
	cptr reply[QI_TALK_LINES];			/* give a reply to the keyword (cptr table contains [QI_TALK_LINES])*/
	u16b replyflags[QI_TALK_LINES];			/* only print this particular text line if these flags are matching the quest flags */
} qi_kwreply;

/* Main structure: The complete quest data.
   Each quest contains info
   -when/where to activate, scheduled repeatedly over the in-game day/night
    cycle and what requirements players must meet to acquire it,
   -how many questors (quest monsters/objects that the player can interact
    with to acquire the quest or to proceed through quest stages) it has,
   -how many quest stages it has for the player to beat,
   -how many keywords the quest provides (possibly used by each of its stages)
    and the automatic reply dialogue that each keyword results in. */
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
	   automatically, during which quests are usually acquired by players).
	   IT MUST NEVER BE -1 AFTER THE QUEST ACTIVATION HAS FINISHED or init_quest_stage() will do the segfault dance. */
	s16b cur_stage;					/* the current stage in the quest progress */
	bool dirty;					/* dirty flag, set whenever a stage completes ;) (including quest termination)
							   there is no need for players to have their local instances of this,
							   because it's used atomically in succeeding code parts that know they
							   can depend on each other. (uh or something) */

	s16b objects_registered;			/* Track all objects the quest spawns except for quest items.
							   So this keeps track of object-type questors and of special quest items. */

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
	byte individual;				/* quest isn't global, but stage/flags/goals are stored individually for each player,
							   allowing everyone to have his own personal 'instance' of the quest running simultaneusly.
							   For example questors may spawn other questors -> must be global, not individual.
							   0: global, 1: individual and sharing goals with party members, 2: solo-individual. */
	s16b repeatable;				/* player may repeat this quest n times (0 = can only do this quest once) */
	s16b cooldown;					/* in seconds, minimum respawn time for the quest. 0 for 24h default. */
	int max_duration;				/* in seconds, 0 for never */
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
	s16b stage_idx[QI_STAGES];			/* map a stage to a stage[]-index, for example there could be 3 stages in a quest: 0, 1 and 7 :-p */
	qi_stage *stage;

	/* amount of different keywords for player-npc-dialogue */
	int keywords;
	qi_keyword *keyword;

	/* amount of different replies to keywords in player-npc-dialogue */
	int kwreplies;
	qi_kwreply *kwreply;
} quest_info;
