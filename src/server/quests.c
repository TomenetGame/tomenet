/* This file is for providing a framework for quests,
   allowing easy addition/editing/removal of them.
       - C. Blue

   Note: This is a new approach. Not to be confused with old
         q_list[], quest_type and quest[] code.

   Quests can make use of Send_request_key/str/num/cfr/abort()
   functions to provide the possibility of dialogue with npcs.
   Quest npcs should be neutral and non-attackable (at least
   not on accident) and should probably not show up on ESP,
   neither should quest objects show up on object detection.
*/


#define QI_MAX_STAGES		50	/* a quest can have these # of different stages */
#define QI_MAX_KEYWORDS		10	/* for dialogue with the questor */


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


struct quest_info {
    /* QUESTOR (quest giver) RESTRICTIONS: */
	/* player restrictions */
	int minlev, maxlev;				/* eligible player level range (0 for any) */
	u32b classes, races;				/* eligible player classes/races (CFx/RFx flags) */

	/* starting location restrictions */
	byte s_location_type				/* flags setting elibible starting location types (QI_SLOC_xxx) */
	u16b s_towns_array;				/* QI_SLOC_TOWN: flags setting eligible starting towns (QI_STOWN_xxx) */
	u32b s_terrains					/* QI_SLOC_SURFACE: flags setting eligible starting terrains (RF8_WILD_xxx, RF8_WILD_TOO_MASK for all) */

	bool s_dungeon[MAX_D_IDX];			/* QI_SLOC_DUNGEON/TOWER: eligible starting dungeons/towers, or (for Wilderness dungeons): */
	u32b s_dungeon_must_flags1, s_dungeon_must_flags2, s_dungeon_must_flags3;	/*  eligible wilderness dungeon flags */
	u32b s_dungeon_mustnt_flags1, s_dungeon_mustnt_flags2, s_dungeon_mustnt_flags3;	/*  uneligible wilderness dungeon flags */
	bool s_dungeon_iddc;				/*  is the Ironman Deep Dive Challenge an eligible starting point? */
	int dlevmin, dlevmax;				/*  eligible dungeon level or world sector level (0 for any) */

	bool night, day;				/* Only available at night/day in general? */
	bool morning, noon, afternoon, evening, midnight, deepnight; /*  Only available at very specific night/day times? */

	char t_pref[1024];				/* filename of map to load, or empty for none */

	/* exact questor starting location */
	struct worldpos start_wpos;			/* -1, -1 for random */
	int start_x, start_y;				/* -1, -1 for random */

	/* type of questor */
	int questor;					/* QI_QUESTOR_xxx */

	int questor_ridx;				/* QI_QUESTOR_NPC; 0 for any */
	char questor_rchar;				/*  0 for any */
	byte questor_rattr;				/*  0 for any */
	int questor_rlevmin, questor_rlevmax;		/*  0 for any */

	int questor_sval;				/* QI_QUESTOR_PARCHMENT */

	int questor_ktval, questor_ksval;		/* QI_QUESTOR_ITEM_xxx. No further stats/enchantments are displayed! */

    /* QUEST DURATION */
	bool active;					/* questor is currently spawned/active (depends on day/night constraints) */
	/* quest duration, after it was accepted, until it expires */
	int max_duration;				/* in seconds, 0 for never */
	int cooldown;					/* in seconds, minimum respawn time for the questor. 0 for 24h default. */
	bool per_player;				/* this quest isn't global but can be done by each player individually.
							   For example questors may spawn other questors -> must be global, not per_player. */
	bool non_static;				/* questor floor is not kept static, leaving it will lose the quest */

    //NOTE: these should instead be hard-coded, similar to global events, too much variety really.. (monster spawning etc):
    /* QUEST CONSEQUENCES (depend on quest stage the player is in) --
	stage action #0 is the initial one, when the QUESTOR spawns (invalid for action_talk[]),
	stage action #1 is, when a player comes in LoS,
	stage action #2 is, when a player accepts interacted with the questor. */

	/* quest initialisation and meta actions */
	bool auto_accept_los, auto_accept;	/* player gets the quest just be being in LoS / interacting once with the questor (bump/read the parchment/pickup the item) */
	int activate_quest[QI_MAX_STAGES];	/* spawn a certain new quest of this index (and thereby another questor) (if not already existing) */
	int change_stage[QI_MAX_STAGES];	/* automatically change to a different stage after handling everything that was to do in the current stage */

	/* quest dialogues and responses/consequences (stage 0 means player loses the quest again) */
	//NOTE: '$RPM' in dialogue will be substituted by xxx_random_pick'ed monster criteria
	//NOTE: '$OPM' in dialogue will be substituted by xxx_random_pick'ed object criteria
	char talk[QI_MAX_STAGES][10][50];			/* n conversations a 10 lines a 50 characters */
	char keywords[QI_MAX_STAGES][QI_MAX_KEYWORDS][30];	/* each convo may allow the player to reply with up to m keywords */
	int keywords_stage[QI_MAX_STAGES][QI_MAX_KEYWORDS];	/*  ..which will bring the player to a different quest stage */
	char yn[QI_MAX_STAGES];					/* each convo may allow the player to reply with yes or no (NOTE: could just be done with keywords too, actually..) */
	int y_stage[QI_MAX_STAGES], n_stage[QI_MAX_STAGES]	/*  ..which will bring the player to a different quest stage */

	/* quest goals (Note: Usually OR'ed, except if 'xxx_random_pick' is set) */
	bool kill_random_pick;						/* instead of demanding any of the eligible monster criteria, pick ONE randomly */
	int kill_ridx[QI_MAX_STAGES][20];				/* kill certain monster(s) */
	char kill_rchar[QI_MAX_STAGES][5];				/*  ..certain types */
	byte kill_rattr[QI_MAX_STAGES][5];				/*  ..certain colours */
	int kill_rlevmin[QI_MAX_STAGES], kill_rlevmax[QI_MAX_STAGES];	/* 0 for any */
	int kill_number[QI_MAX_STAGES];
	int kill_spawn[QI_MAX_STAGES], kill_spawn_loc[QI_MAX_STAGES];	/* actually spawn the monster(s) nearby! (QI_SPAWN_xxx) */
	int kill_stage[QI_MAX_STAGES];					/* switch to a different quest stage on defeating the monsters */

	bool retrieve_random_pick;							/* instead of demanding any of the eligible item criteria, pick ONE randomly */
	int retrieve_otval[QI_MAX_STAGES][20], retrieve_osval[QI_MAX_STAGES][20];	/* retrieve certain item(s) */
	int retrieve_opval[QI_MAX_STAGES][5], retrieve_obpval[QI_MAX_STAGES][5];
	byte retrieve_oattr[QI_MAX_STAGES][5];						/*  ..certain colours (flavoured items only) */
	int retrieve_oname1[QI_MAX_STAGES][20], retrieve_oname2[QI_MAX_STAGES][20], retrieve_oname2b[QI_MAX_STAGES][20];
	int retrieve_ovalue[QI_MAX_STAGES][20];
	int retrieve_number[QI_MAX_STAGE][20];
	int retrieve_stage[QI_MAX_STAGES];						/* switch to a different quest stage on retrieving the items */

	struct worldpos target_wpos;		/* kill/retrieve specifically at this world pos */
	bool target_terrain_patch;		/* extend valid target location over all connected world sectors whose terrain is of the same type (eg big forest) */

	/* quest rewards */
	int reward_otval[QI_MAX_STAGES];	/* hand over certain rewards to the player */
	int reward_osval[QI_MAX_STAGES];
	int reward_opval[QI_MAX_STAGES], reward_obpval[QI_MAX_STAGES];
	int reward_oname1[QI_MAX_STAGES], reward_spawn_oname2[QI_MAX_STAGES], reward_oname2b[QI_MAX_STAGES];
	bool reward_ogood[QI_MAX_STAGES], reward_ogreat[QI_MAX_STAGES];
	bool reward_oreward[QI_MAX_STAGES];	/*  use fitting-reward algo (from highlander etc)? */
	int reward_gold[QI_MAX_STAGES];
	int reward_exp[QI_MAX_STAGES];
} quest_info;
