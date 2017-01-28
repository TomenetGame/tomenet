/* $Id$ */
/* File: defines-util.h */

/* Purpose: global constants and macro definitions for util.pre lua file */


/*
 * Bit flags for the "c_get_item" function
 */
#define USE_EQUIP	0x01	/* Allow equip items */
#define USE_INVEN	0x02	/* Allow inven items */


#define TERM_DARK	0	/* 'd' */	/* 0,0,0 */
#define TERM_WHITE	1	/* 'w' */	/* 4,4,4 */
#define TERM_SLATE	2	/* 's' */	/* 2,2,2 */
#define TERM_ORANGE	3	/* 'o' */	/* 4,2,0 */
#define TERM_RED	4	/* 'r' */	/* 3,0,0 */
#define TERM_GREEN	5	/* 'g' */	/* 0,2,1 */
#define TERM_BLUE	6	/* 'b' */	/* 0,0,4 */
#define TERM_UMBER	7	/* 'u' */	/* 2,1,0 */
#define TERM_L_DARK	8	/* 'D' */	/* 1,1,1 */
#define TERM_L_WHITE	9	/* 'W' */	/* 3,3,3 */
#define TERM_VIOLET	10	/* 'v' */	/* 4,0,4 */
#define TERM_YELLOW	11	/* 'y' */	/* 4,4,0 */
#define TERM_L_RED	12	/* 'R' */	/* 4,0,0 */
#define TERM_L_GREEN	13	/* 'G' */	/* 0,4,0 */
#define TERM_L_BLUE	14	/* 'B' */	/* 0,4,4 */
#define TERM_L_UMBER	15	/* 'U' */	/* 3,2,1 */

/* Non encoded shimmer attributes */
#define TERM_MULTI	16	/* all the main colours */
#define TERM_POIS	17	/* I Love this ;) */
#define TERM_FIRE	18	/* fireball */
#define TERM_COLD	19	/* cold */
#define TERM_ACID	20	/* acid, similar to darkness */
#define TERM_ELEC	21	/* elec */
#define TERM_CONF	22	/* umber/lumber */
#define TERM_SOUN	23	/* similar to lite */
#define TERM_SHAR	24	/* umber/slate */
#define TERM_LITE	25	/* similar to sound */
#define TERM_DARKNESS	26	/* similar to acid */

#define TERM_SHIELDM	27	/* mana shield */
#define TERM_SHIELDI	28	/* invulnerability */

#ifdef EXTENDED_TERM_COLOURS
 #define TERM_CURSE	29
 #define TERM_ANNI	30
#endif

#define TERM_HALF	31	/* only the brighter colours */

#ifdef EXTENDED_TERM_COLOURS
 #define TERM_OLD_BNW	0x20	/* 32: black & white MASK, for admin wizards */
 #define TERM_OLD_PVP	0x40	/* 64: black & red MASK, for active PvP-hostility (or stormbringer) */

 #define TERM_PSI	32
 #define TERM_NEXU	33
 #define TERM_NETH	34
 #define TERM_DISE	35
 #define TERM_INER	36
 #define TERM_FORC	37
 #define TERM_GRAV	38
 #define TERM_TIME	39
 #define TERM_METEOR	40
 #define TERM_MANA	41
 #define TERM_DISI	42
 #define TERM_WATE	43
 #define TERM_ICE	44
 #define TERM_PLAS	45
 #define TERM_DETO	46
 #define TERM_NUKE	47
 #define TERM_UNBREATH	48
 #define TERM_HOLYORB	49
 #define TERM_HOLYFIRE	50
 #define TERM_HELLFIRE	51
 #define TERM_THUNDER	52

 #define TERM_LAMP	53
 #define TERM_LAMP_DARK	54

 #define TERM_EMBER	55

 #ifdef ATMOSPHERIC_INTRO
  #define TERM_FIRETHIN	56
 #endif

 #define TERM_STARLITE	57
 #define TERM_HAVOC	58

 #ifdef EXTENDED_BG_COLOURS
  #define TERM2_BLUE	63
 #endif

 #define TERM_BNW	0x40	/* 64: black & white MASK, for admin wizards */
 #define TERM_PVP	0x80	/* 128: black & red MASK, for active PvP-hostility (or stormbringer) */
#else
 #define TERM_BNW	0x20	/* 32: black & white MASK, for admin wizards */
 #define TERM_PVP	0x40	/* 64: black & red MASK, for active PvP-hostility (or stormbringer) */

 /* Reserved attr values - do not exceed */
 #define TERM_RESERVED	0x80	/* 128 */
#endif



/* Hooks, scripts  (currently not accessed from LUA actually) */
#define HOOK_MONSTER_DEATH      0
#define HOOK_OPEN               1
#define HOOK_GEN_QUEST          2
#define HOOK_END_TURN           3
#define HOOK_FEELING            4
#define HOOK_NEW_MONSTER        5
#define HOOK_GEN_LEVEL          6
#define HOOK_BUILD_ROOM1        7
#define HOOK_NEW_LEVEL          8
#define HOOK_QUEST_FINISH       9
#define HOOK_QUEST_FAIL         10
#define HOOK_GIVE               11
#define HOOK_CHAR_DUMP          12
#define HOOK_INIT_QUEST         13
#define HOOK_WILD_GEN           14
#define HOOK_DROP               15
#define HOOK_IDENTIFY           16
#define HOOK_MOVE               17
#define HOOK_STAIR              18
#define HOOK_MONSTER_AI         19
#define HOOK_PLAYER_LEVEL       20
#define HOOK_WIELD              21
#define HOOK_INIT               22
#define HOOK_QUAFF              23
#define HOOK_AIM                24
#define HOOK_USE                25
#define HOOK_ACTIVATE           26
#define HOOK_ZAP                27
#define HOOK_READ               28
#define HOOK_CALC_BONUS         29
#define HOOK_PLAYER_FLAGS       30
#define HOOK_KEYPRESS           31
#define HOOK_CHAT               32
#define HOOK_MON_SPEAK          33
#define HOOK_MKEY               34
#define HOOK_BIRTH_OBJECTS      35
#define HOOK_ACTIVATE_DESC      36
#define HOOK_INIT_GAME          37
#define HOOK_ACTIVATE_POWER     38
#define HOOK_ITEM_NAME          39
#define HOOK_SAVE_GAME          40
#define HOOK_LOAD_GAME          41
#define HOOK_LEVEL_REGEN        42
#define HOOK_LEVEL_END_GEN      43
#define HOOK_BUILDING_ACTION    44
#define HOOK_PROCESS_WORLD      45
#define HOOK_WIELD_SLOT         46
#define HOOK_STORE_STOCK        47
#define HOOK_STORE_BUY          48
#define HOOK_GEN_LEVEL_BEGIN    49
#define HOOK_GET                50
#define HOOK_NPCTEST            51
#define MAX_HOOKS               52
