/* $Id$ */
/* File: defines-features.h */

/* Purpose: global definitions of enabled features.
   I created created this file and split off all feature #defines
   to move them in here, so it can be included by the new .pre lua files,
   to allow for finer #ifdef checks there.
   So this file must ONLY contain #define <feature> basically! - C. Blue
 */



/* --------------------------------------------------------------------------*/
/* Features that are allowed in all build types, ie server-type independant: */

/* Add static (and safe) towns to "Ironman Deep Dive Challenge"? (2k, 4k) */
#define IRONDEEPDIVE_STATIC_TOWNS
/* Allow to withdraw prematurely in static Ironman Deep Dive Challenge towns.
   (No entry to the leaderboard this way of course.) */
#define IRONDEEPDIVE_STATIC_TOWN_WITHDRAWAL

#define DUNGEON_VISIT_BONUS	/* Experimental: Dungeons rarely visited give exp bonus - C. Blue */
//#define DUNGEON_VISIT_BONUS_DEPTHRANGE /* not yet implemented: enhance DUNGEON_VISIT_BONUS algorithm further (but seems inefficient atm) */

#define ENABLE_RUNEMASTER	/* enable runemaster class */
#define ENABLE_RCRAFT		/* New runecraft class - relsiet (toggles new alternative code for ENABLE_RUNEMASTER) */

#define ENABLE_NEW_MELEE	/* shields may block, weapons may parry */
#define DUAL_WIELD		/* rogues (and others too now) may dual-wield 1-hand weapons */
#define ENABLE_STANCES		/* combat stances for warriors */
#define ALLOW_SHIELDLESS_DEFENSIVE_STANCE	/* Always allow defensive stance (less effective than with shield though) */

#define ENABLE_CLOAKING		/* cloaking mode for rogues */
#define NEW_DODGING		/* reworked dodging formulas to allow more armour weight while aligning it to rogues, keeping your ideas though, Adam ;) - C. Blue */
#define ENABLE_TECHNIQUES
#define NEW_HISCORE		/* extended high score options for tomenet.cfg */

#define ENABLE_MCRAFT		/* 'Mindcrafter' class - C. Blue */
#define NEW_TOMES		/* customizable spellbooks */

#define CLIENT_SIDE_WEATHER	/* server uses Send_weather() instead of displaying own weather animation */
#define MAX_CLOUDS 2000

#define EXTRA_LEVEL_FEELINGS	/* enable extra level feelings, remotely angband-style, warning about dangers */
#define M_EGO_NEW_FLICKER	/* ego monsters flicker between base r_ptr and ego colour */
#define NEW_ANTIMAGIC_RATIO	/* new darksword-vs-skill ratio for antimagic: weapon up to 30%, skill up to 50% */

#define ANTI_SEVEN_EXPLOIT	/* prevent ball-spell/explosion casters from exploiting monster movement by using a 7-shaped corridor */
#define DOUBLE_LOS_SAFETY	/* prevent exploit of diagonal LoS that may result in stationary monsters unable to retaliate */
#define STEAL_CHEEZEREDUCTION	/* reduce cheeziness of stealing by giving more expensive items a chance to turn level 0 */

#define PLAYER_STORES		/* Enable player-run shops - C. Blue */
#define HOUSE_PAINTING		/* Allow players to paint their entrance area or house (for PLAYER_STORES) - C. Blue */

#ifndef WIN32
#define ENABLE_GO_GAME		/* Allows players to play vs CPU games of Go/Weiqi/Baduk. - C. Blue */
#endif
#define ENABLE_MAIA		/* enable RACE_MAIA (formerly 'DIVINE' race) */

/* Allow monsters with AI_ASTAR r_info flag to use A* pathfinding algorithm? - C. Blue */
#define MONSTER_ASTAR

/* C. Blue - Note about flow_by_sound/flow_by_smell:
   Smell is currently not distinct from sound, but code is kind of incomplete or mixed up in some parts.
   I added various limits for each sound and flow, although these aren't implemented yet they should be here,
   because they won't bring additional costs! So on fixing sound/smell they should definitely be added.
   Also, distinctions between sound/smell/ESP(new ;)) should be added, depending on monster races. */

/* Allow monsters to use flowing pathfinding algorithm? */
//#define MONSTER_FLOW_BY_SOUND

/* Allow monsters to use smelling pathfinding algorithm? */
//#define MONSTER_FLOW_BY_SMELL

/* Allow monsters to use flow algorithm without range/radius (well as far as we have cpu powah..)/time restrictions?
   Let's just use same as for MONSTER_FLOW_BY_SOUND, so we don't need more fields for cave_type.. - C. Blue */
//#define MONSTER_FLOW_BY_ESP

/* Allow use of '!E' inscription to prevent earthquakes from TR5_IMPACT items? */
//#define ALLOW_NO_QUAKE_INSCRIPTION

/* Allow only a limited amount of guild adders, in turn maintain an always
   accessible list of them, which also allows for removal while they aren't on? */
#define GUILD_ADDERS_LIST

/* Shorten party/guild name to (P) and (G) in party-/guild-chat for less clutter? */
#define GROUP_CHAT_NOCLUTTER

/* Load 'D:' tags from a_info.txt file and display them on examining - C. Blue */
#define ART_DIZ



/* --------------------- Server-type dependant features -------------------- */

#ifdef RPG_SERVER
 #define BONE_AND_TREASURE_CHAMBERS	/* New experimental room types: Generate pits of bones or treasure - C. Blue */

 #define MUCHO_RUMOURS		/* print a rumour on day changes and unique kills (the_sandman) */
// #define PRECIOUS_STONES

 #define AUCTION_BETA		/* less restrictions while beta testing */
 #define AUCTION_SYSTEM
 #define AUCTION_DEBUG

 #define OPTIMIZED_ANIMATIONS	/* testing */
#endif

#ifdef TEST_SERVER
 #define ENABLE_ASSASSINATE	/* experimental fighting technique for rogues - devalues Backstabbing too much probably */

 #ifdef MAX_CLOUDS
  #undef MAX_CLOUDS
  #define MAX_CLOUDS 10		/* note that this number gets divided depending on season */
 #endif

 #define AUCTION_BETA		/* less restrictions while beta testing */
 #define AUCTION_SYSTEM
 #define AUCTION_DEBUG

 #define OPTIMIZED_ANIMATIONS	/* testing */
#endif



/* ------------------------ Client-side only features -----------------------*/

#ifdef CLIENT_SIDE
 /* compile hybrid clients that can do both, old and new runemastery */
 #ifndef ENABLE_RCRAFT
  #define ENABLE_RCRAFT		/* New runecraft class - relsiet (toggles new alternative code for ENABLE_RUNEMASTER) */
 #endif
#endif



/* ----------------- Misc flags induced by above definitions ----------------*/

/* Use new shields with block/deflect values instead of traditional ac/+ac ? */
#ifdef ENABLE_NEW_MELEE
 #ifndef USE_NEW_SHIELDS
  #define USE_NEW_SHIELDS
 #endif
/* Use blocking/parrying? if USE_NEW_SHIELDS is disabled, AC will be used to determine block chance */
 #ifndef USE_BLOCKING
  #define USE_BLOCKING
 #endif
 #ifndef USE_PARRYING
  #define USE_PARRYING
 #endif
#endif

/* Flow by sound/smell/esp requires MONSTER_FLOW */
#ifdef MONSTER_FLOW_BY_SOUND
 #define MONSTER_FLOW
#endif
#ifdef MONSTER_FLOW_BY_SMELL
 #ifndef MONSTER_FLOW
  #define MONSTER_FLOW
 #endif
#endif
#ifdef MONSTER_FLOW_BY_ESP
 #ifndef MONSTER_FLOW
  #define MONSTER_FLOW
 #endif
#endif


/* DUNGEON_VISIT_BONUS:
   Amount of time until a dungeon cannot be more frequented anymore (in minutes) [800].
   The time until a dungeon goes back down to 0 ('most unexplored') is that time * 10,
   so 800 would result in 8000 minutes aka ~6 days. */
#define VISIT_TIME_CAP 800
