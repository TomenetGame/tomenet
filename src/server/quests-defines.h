/* This is the definitions  file for quests.c, please look there for more information.
   You may modify/use it freely as long as you give proper credit. - C. Blue
*/


#define QI_CODENAME_LEN		10	/* short, internal quest code name */
#define QI_PREREQUISITES	5	/* max # of prerequisite quests for a follow-up quest */
#define QI_QUESTORS		30	/* amount of questor(NPC)s, there can be more than one! */
#define QI_STAGES		50	/* a quest can have these # of different stages */
#define QI_TALK_LINES		15	/* amount of text lines per talk dialogue */
#define QI_LOG_LINES		15	/* amount of text lines shown in a "quest log" screen */
#define QI_KEYWORDS		100	/* for dialogue with the questor */
#define QI_KEYWORD_LEN		30	/* length of a keyword, for dialogue with the questor */
#define QI_PASSWORDS		5	/* special keywords that are actually passwords that are randomly regenerated each session */
#define QI_PASSWORD_LEN		12	/* length of a password-keyword, for dialogue with the questor */
#define QI_KEYWORD_REPLIES	50	/* replies from for a questor, depending on keyword entered */
#define QI_KEYWORDS_PER_REPLY	5	/* so many different keywords may trigger the same keyword-reply text */
#define QI_STAGE_REWARDS 	10	/* max # of rewards handed out per completed stage */
#define QI_GOALS		5	/* main goals to complete a stage */
#define QI_REWARD_GOALS		5	/* up to 5 different main/optional goals that have to be completed for a specific reward */
#define QI_STAGE_GOALS		5	/* up to 5 different main/optional goals that have to be completed for changing to a specific next stage */
#define QI_FOLLOWUP_STAGES	5	/* the # of possible follow-up stages of which one is picked depending on the completed stage goals */
#define QI_FLAGS		16	/* global flags that range from 'a' to 'p' and can be set via uppercase letter, erased via lowercase letter. */
#define QI_TERRAIN_PATCH_RADIUS	5	/* max radius for valid terrain of same type as target terrain (terrain patch extension for quest goals) --note: this uses distance() */
#define QI_STAGE_AUTO_FEATS	15	/* max # of cave features to build on a stage start */
#define QI_STAGE_AUTO_MSPAWNS	10	/* max # of monster spawning on a stage start */


#define QI_SLOC_SURFACE		0x1
#define QI_SLOC_TOWN		0x2
#define QI_SLOC_DUNGEON		0x4
//#define QI_SLOC_TOWER		0x8

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
#define QI_SPAWN_PACK_FAR	7	/* spawn as pack far away */


/* calculate an internal quest goal index from a quest goal number specified in q_info.txt */
//deprecated. #define QUEST_GOAL_IDX(j)	((j > 0) ? j - 1 /* main goal */ : (j < 0 ? QI_GOALS - j - 1 /* optional goal */ : -1 /* invalid */))
