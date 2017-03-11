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

/* Distinguished light colour for flaming lites, magic lights, vampire light */
#define CAVE_LITE_COLOURS

/* Add extra character slot(s) dedicated to Ironman Deep Dive Challenge (MAX_DED_IDDC_CHARS) */
#define ALLOW_DED_IDDC_MODE
/* Add extra character slot(s) dedicated to PvP mode (MAX_DED_PVP_CHARS) */
#define ALLOW_DED_PVP_MODE

/* Use daily pre-generated IDDC lineups of dungeon types, with transitions, bosses, etc? */
#define IRONDEEPDIVE_MIXED_TYPES
/* Add fixed (and safe) towns to "Ironman Deep Dive Challenge"? (at depths 2k and 4k) */
#define IRONDEEPDIVE_FIXED_TOWNS
/* Allow to withdraw prematurely in fixed Ironman Deep Dive Challenge towns.
   (No entry to the leaderboard this way of course.) */
#define IRONDEEPDIVE_FIXED_TOWN_WITHDRAWAL
/* Fixed towns in "Ironman Deep Dive Challenge" are also static regarding objects and monsters? */
//#define IRONDEEPDIVE_STATIC_TOWNS
/* Add extra fixed towns to "Ironman Deep Dive Challenge"? (at depths 1k and 3k),
   these are 'wild' like random towns and not as privileged (no withdrawal, no item staticness) */
#define IRONDEEPDIVE_EXTRA_FIXED_TOWNS
/* Do artifacts time out especially quickly in the IDDC? */
#define IDDC_ARTIFACT_FAST_TIMEOUT
/* Can the first [two] speed rings be found especially easily in the IDDC? Or too much pampering? (0/[1]/2) */
#define IDDC_EASY_SPEED_RINGS 1

/* Do artifacts time out especially quickly for total_winners
   (and for once_winners if fallenkings_etiquette is set)? */
#define WINNER_ARTIFACT_FAST_TIMEOUT

/* Experimental: Dungeons rarely visited give exp bonus - C. Blue */
#define DUNGEON_VISIT_BONUS
//#define DUNGEON_VISIT_BONUS_DEPTHRANGE /* not yet implemented: enhance DUNGEON_VISIT_BONUS algorithm further (but seems inefficient atm) */

/* Filter swearwords (depends on client-side option too) and put offenders to jail?
   (Filer initialised from swearing.txt (vs nonswearing.txt) and init_swear_set().)
   NOTE: Censoring can still be disabled (and reenabled) on the fly via
         'censor_swearing' lua variable.*/
#define CENSOR_SWEARING

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
#define MAX_CLOUDS 1000

#define EXTRA_LEVEL_FEELINGS	/* enable extra level feelings, remotely angband-style, warning about dangers */
#define M_EGO_NEW_FLICKER	/* ego monsters flicker between base r_ptr and ego colour */
#define NEW_ANTIMAGIC_RATIO	/* new darksword-vs-skill ratio for antimagic: weapon up to 30%, skill up to 50% */

#define ANTI_SEVEN_EXPLOIT	/* prevent ball-spell/explosion casters from exploiting monster movement by using a 7-shaped corridor */
#define DOUBLE_LOS_SAFETY	/* prevent exploit of diagonal LoS that may result in stationary monsters unable to retaliate */
#define STEAL_CHEEZEREDUCTION	/* reduce cheeziness of stealing by giving more expensive items a chance to turn level 0 */

#define PLAYER_STORES		/* Enable player-run shops - C. Blue */
#define HOUSE_PAINTING		/* Allow players to paint their entrance area or house (for PLAYER_STORES) - C. Blue */
#define HOME_APPRAISAL		/* Displays player store price when inspecting an item at home */
#define EXPORT_PLAYER_STORE_OFFERS	60	/* Export all player store items to an external list every n minutes [60] */

#ifndef WIN32
 #define ENABLE_GO_GAME		/* Allows players to play vs CPU games of Go/Weiqi/Baduk. - C. Blue */
#endif
#define ENABLE_MAIA		/* enable RACE_MAIA (formerly 'DIVINE' race) */
#define ENABLE_KOBOLD		/* enable RACE_KOBOLD */


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


/* Allow use of '!E' inscription on any item to prevent earthquakes from TR5_IMPACT items?
   If disabled, it's still allowed to be used on Grond (exclusively)! */
//#define ALLOW_NO_QUAKE_INSCRIPTION

/* Allow only a limited amount of guild adders, in turn maintain an always
   accessible list of them, which also allows for removal while they aren't on? */
#define GUILD_ADDERS_LIST

/* Shorten party/guild name to (P) and (G) in party-/guild-chat for less clutter? */
#define GROUP_CHAT_NOCLUTTER

/* Load 'D:' tags from a_info.txt file and display them on examining - C. Blue */
#define ART_DIZ

/* Enable the instant resurrection feature (for everlasting players, in the temple) */
#define ENABLE_INSTANT_RES

/* Use new 4.4.9 wilderness features? WARNING: Changes wilderness and houses */
#define WILDERNESS_NEW_FEATURES

/* Allow a larger main window with a map bigger than 66x22 (usually 66x44) - C. Blue */
#define BIG_MAP

/* Use an info screen for normal ID too, not just for *ID*, and display all
   guaranteed abilities (k_info and 100%-e_info flags/esp) in it. - C. Blue */
#define NEW_ID_SCREEN

/* Load 'D:' tags from k_info.txt file and display them on examining - C. Blue */
#define KIND_DIZ

/* Allow kings/queens/emperors/empresses to team up for Nether Realm.
   Must be at the worldmap sector of NR entrance, or inside NR. */
#define ALLOW_NR_CROSS_PARTIES
/* Best to enable when ALLOW_NR_CROSS_PARTIES is enabled:
   Allow kings/queens/emperors/empresses to trade items that were found in the
   Nether Realm as long as the items weren't transported out of it once. */
#define ALLOW_NR_CROSS_ITEMS

/* expand TV_POTION svals to make TV_POTION2 obsolete? */
#define EXPAND_TV_POTION

/* Use newly reworked spell system: Discrete stages for each component of
   compound spells, and fixed mana cost to go with it. */
#define DISCRETE_SPELL_SYSTEM
//#define DSS_EXPANDED_SCROLLS /* spell scrolls to carry all versions of a spell (I,II,III..) instead of just one specific? (Implementation not yet done) */

/* Draconians get to pick a 'lineage trait' on birth,
   giving specific resistances and a breath weapon. - C. Blue */
#define ENABLE_DRACONIAN_TRAITS

/* Allow an OT_GUILD house for each guild. - C. Blue */
#define ENABLE_GUILD_HALL

/* like BONE_AND_TREASURE_CHAMBERS, just for IDDC */
#define IDDC_BONE_AND_TREASURE_CHAMBERS

/* Dedicated IDDC-only characters are (randomly) aware of flavoured items,
   with this chance per flavour, as soon as they enter the IDDC. [1..100] */
#define DED_IDDC_AWARE 33


/* New in 4.5.1.2: Support 100% client-side animated spellcasting - C. Blue
   Note: This must be on for clients >= 4.5.1.2.0.0 and off for those before.
   Same for servers. */
#define EXTENDED_TERM_COLOURS

/* Special extended colours that make use of background colouring - C. Blue
   Note: This is highly EXPERIMENTAL and not even implemented atm.,
         the only thing that works is proof of concept code that displays
         rain in alternating colours, TERM_ORANGE and (newly added for this) TERM2_BLUE. */
//#define EXTENDED_BG_COLOURS


/* better chance for non-low +hit,+dam on randart melee weapons and boomerangs */
#define RANDART_WEAPON_BUFF

/* Allow usage of /hilite command (todo: turn into client option) */
#define ENABLE_SELF_FLASHING


/* Allow !X pseudo-auto-identify inscription on spell books too - C. Blue
   Note that this badly abuses the 'item-on-floor' (negative current_item value) feature,
   which fortunately is unused in general.
   Currently there's a harmless message inconsistency: !X from spells will output an
   additional 'In your pack: xxxx (x)' message before the 'You have..' carry() message. */
#define ENABLE_XID_SPELL

/* Allow !X on magic devices too (rods/staves of perception). See ENABLE_XID_SPELL above, basically. */
#define ENABLE_XID_MDEV

#if defined(ENABLE_XID_SPELL) || defined(ENABLE_XID_MDEV)
 /* Repeat spell cast/device activation attempt until it succeeds. (Scrolls always succeed, so not needed for those.) */
 #define XID_REPEAT
#endif


/* Allow Martial Arts users to wield boomerangs? */
#define ENABLE_MA_BOOMERANG

/* Warning beep on taking damage while off-panel via locate command */
#define ALERT_OFFPANEL_DAM

/* Telekinesis uses server-side get_item() request instead of the
   client-side LUA script prompting for the item? (Recommended)
   Advantage: We only ask for the item after the spell has been cast
   successfully.
   Disadvantage: Macros need \wXX to wait for the server-side request.
   (Not a real 'feature', but needs to be in here to be recognized by player.pre.) */
#define TELEKINESIS_GETITEM_SERVERSIDE

/* Wilderness mapping scrolls are special: They're actually pieces of the world map. */
#define NEW_WILDERNESS_MAP_SCROLLS

/* Disable manual declaration of hostility/peace */
#define NO_PK

/* Allow players to solo-reking fallen winner characters without help of anyone else (experimental).
   If != 0, then it's enabled and both, the amount of pure money to be paid and the amount of shared money/time, to be 'paid' on top,
   so twice this has to be 'paid' in total. */
#define SOLO_REKING 5000000

/* Do vampires not suffer Black Breath at all? */
#define VAMPIRES_BB_IMMUNE
/* Will negative boni on cursed items become (scaled) positive ones when wielded by vampires?
   (0 = rather inconsistent method, 1 = recommended method) */
#define VAMPIRES_INV_CURSED 1
/* Allow vampires to polymorph into vampiric mist at 40, obtaining some special feats? */
#define VAMPIRIC_MIST

/* Allow ordering a specific item in a store */
#define ENABLE_ITEM_ORDER

/* Allow melting coins with high a magical fire-similar attack of high enough temperature into golem body pieces.
   The number is the cost factor for smelting coins into massive pieces; note: 7..17x is possible for default gold colour on player-dropped piles.
   Also allow melting items. */
#define SMELTING 8

/* Load 'D:' tags from e_info.txt file and display them on examining */
#define EGO_DIZ

/* For auto-retaliator's attack splitting: Switching target in the midst of combat costs 1 bpr of energy. Only affects melee attacking. */
#define TARGET_SWITCHING_COST
/* For shooting: Switching target in the midst of combat costs 1 spr of energy. */
#define TARGET_SWITCHING_COST_RANGED

/* Allow sending gold or an item to someone via merchants' guild */
#define ENABLE_MERCHANT_MAIL
#ifdef ENABLE_MERCHANT_MAIL
 //#define MERCHANT_MAIL_INFINITE /* If enabled, it'll bounce forever. [no] */
 #define MAX_MERCHANT_MAILS 100
 /* <this> x (MAX_MERCHANT_MAILS / cfg.fps) seconds  [36 -> 1 min] */
 #define MERCHANT_MAIL_DURATION 36
 #ifdef TEST_SERVER
  #define MERCHANT_MAIL_TIMEOUT 36
 #else
  #define MERCHANT_MAIL_TIMEOUT (36 * 60 * 24 * 7)
 #endif
#endif

/* Make staves stack the same way as wands; enable improved way of rod-stacking */
#define NEW_MDEV_STACKING

/* Anti-teleportation will cancel Word-of-recall instead of delaying it. */
#define ANTI_TELE_CHEEZE

/* Update item timeouts in realtime? (Torches/lanterns/Poly-rings/Blood-potions) */
#define LIVE_TIMEOUTS

/* Small fixes/improvements to lua_get_level() and spell-power (not required) */
#define FIX_LUA_GET_LEVEL

 /* limit to_ac to +30 instead of +35 -- Reason: Give base AC more weight!
    Can be adjusted by using different AC_CAP/AC_DIV values. */
#define TO_AC_CAP_30

/* New experimental room types: Generate pits of bones or treasure - C. Blue */
#define BONE_AND_TREASURE_CHAMBERS

/* Don't erase ovl_info overlay displaying detected monsters (mostly) when
   using cmd_locate(). */
#define LOCATE_KEEPS_OVL

/* Enable o_*.lua 'Occult' magic schools (shamans, rogues, adventurers) */
#define ENABLE_OCCULT

/* Allow 'Vampire Paladins' aka Death Knights. Requires ENABLE_OCCULT. */
#ifdef ENABLE_OCCULT
 #define ENABLE_DEATHKNIGHT

 //#if defined(TEST_SERVER) || defined(TEST_CLIENT)
  #define ENABLE_HELLKNIGHT	/* Allow 'Corrupted Paladins' aka Hell Knights. Requires ENABLE_OCCULT. */
  #ifdef ENABLE_HELLKNIGHT
   #define ENABLE_OHERETICISM	/* Enable 'Hereticism' occult school for Hell Knights */
   #define ENABLE_CPRIEST	/* Allow 'Corrupted Priest', keeping its normal class name. Should assume ENABLE_HELLKNIGHT. */
  #endif
 //#endif
#endif


/* --------------------- Server-type dependant features -------------------- */

#ifdef RPG_SERVER
 /* Do we want to use Kurzel's PvE/P when mode 1 PK is configured? */
//#define KURZEL_PK --disabled because it breaks chat highlighting

 #define MUCHO_RUMOURS		/* print a rumour on day changes and unique kills (the_sandman) */
// #define PRECIOUS_STONES

 #define AUCTION_BETA		/* less restrictions while beta testing */
 #define AUCTION_SYSTEM
 #define AUCTION_DEBUG

 #define OPTIMIZED_ANIMATIONS	/* testing */
#endif

#ifdef TEST_SERVER
 #define NEW_REMOVE_CURSE	/* rc has fail chance; allow projecting rc spell on others */

 #define ENABLE_ASSASSINATE	/* experimental fighting technique for rogues - devalues Backstabbing too much probably */

 #ifdef MAX_CLOUDS
  #undef MAX_CLOUDS
  #define MAX_CLOUDS 10		/* note that this number gets divided depending on season */
 #endif

 #define AUCTION_BETA		/* less restrictions while beta testing */
 #define AUCTION_SYSTEM
 #define AUCTION_DEBUG

 #define OPTIMIZED_ANIMATIONS	/* testing */

 #define TELEPORT_SURPRISES	/* monsters are surprised for a short moment if a player long-range teleported next to them */

 #define LIMIT_SPELLS		/* Allow player to limit the level of spells he casts */
#endif

#ifdef ARCADE_SERVER
#endif



/* ------------------------ Client-side only features -----------------------*/

#ifdef CLIENT_SIDE
//Originally for hybrid clients to do both old and new runemastery, now unused.

 /* Use a somewhat lighter 'dark blue' for TERM_BLUE to improve readability
    on some screens / under certain circumstances? */
 //#define READABILITY_BLUE --has been turned into a config file option!

 /* Use a somewhat darker 'light dark' for TERM_L_DARK to improve distinction
    from slate tone? */
 #define DISTINCT_DARK

 /* Remove some hard-coding in the client options */
 #define CO_BIGMAP	7

 /* Blacken lower part of screen if we're mindlinked to a non-bigmap-target but
    are actually using bigmap-screen. */
 #ifdef TEST_CLIENT
  #define BIGMAP_MINDLINK_HACK
 #endif

 /* Atmospheric login screens, with animation, sound and music? */
 #define ATMOSPHERIC_INTRO
#endif

/* ----------------- Misc flags induced by above definitions ----------------*/

/* Use new shields with block/deflect values instead of traditional ac/+ac ? */
#ifdef ENABLE_NEW_MELEE
 #ifndef USE_NEW_SHIELDS
  #define USE_NEW_SHIELDS
  /* Shields do not have an +AC modifier anymore (aka it's zeroed)? - Note that shields still count as is_armour() though */
  #define NEW_SHIELDS_NO_AC
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


/* Enable/undefine specific features locally */
#include "defines-features-local.h"
