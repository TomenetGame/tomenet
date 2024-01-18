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
/* --------------------------------------------------------------------------*/

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
/* Further add extra 'refuge' mini-vaultlike areas (wide open though) for a short respite that allow selling loot and very basic purchases */
#define IDDC_REFUGES
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

/* Server uses Send_weather() so the client draws the weather [NEW], instead of the server displaying its own weather animations for players [old]. */
#define CLIENT_SIDE_WEATHER
#ifdef CLIENT_SIDE_WEATHER
 //#define CLIENT_WEATHER_GLOBAL	/* All worldmap sectors have the same weather at any time, no 'clouds' exist */
#endif
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

#define WARNING_REST_TIMES	0	/* Warn this often about 'R'esting [0 = infinite, but still limited to newbie level range and stops for the current session once the player rests] */

#ifndef WIN32 /* no fork() */
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


/* Allow use of '!Q' inscription on any item to prevent earthquakes from TR5_IMPACT items?
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

/* Experimental and also silly ;) - reward players for wearing arts of similar name - C. Blue */
#define EQUIPMENT_SET_BONUS

/* Use an info screen for normal ID too, not just for *ID*, and display all
   guaranteed abilities (k_info and 100%-e_info flags/esp) in it. - C. Blue */
#define NEW_ID_SCREEN

/* Load 'D:' tags from k_info.txt file and display them on examining - C. Blue */
#define KIND_DIZ

/* Load 'D:' tags from r_info.txt file and display them on kill if enabled - C. Blue */
#define RACE_DIZ

/* Allow kings/queens/emperors/empresses to team up for Nether Realm.
   Must be at the worldmap sector of NR entrance, or inside NR.
   Note: Careful, that PVP-mode isn't allowed. Maybe all checks should just be moved into compat_*() functions. */
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

/* Add another set of 16 colours to the normal 16 colours the game uses,
   which are clones of those, but are only used for drawing the main (map_info aka game) screen.
   Purpose: Allow for palette animation, intended for smooth day/night lighting changes. - C. Blue
   Note: This changes TERM_BNW and TERM_PVP to be normal animated colours instead of masks,
         because otherwise there would not be enough colours available to accomodate for this addition.
   NOTE: Due to the hilite_player comeback this must always be on for server >= 4.7.3.0.0.0 or it breaks old clients. */
#define EXTENDED_COLOURS_PALANIM

/* Special extended colours that make use of background colouring - C. Blue
   Note: This is highly EXPERIMENTAL and not even implemented atm.,
         the only thing that works is proof of concept code that displays
         rain in alternating colours, TERM_ORANGE and (newly added for this) TERMX_BLUE.
         2022-03-10 - reenabling this, not for rain but for floor background 'test'.
   -- !!! CURRENTLY CAUSING A BUG: palette_animation no longer works (daylight-shading) -- */
#define EXTENDED_BG_COLOURS

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
 //#define XID_REPEAT
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

/* Allow a one-time reset of one skill at character level <RESET_SKILL>? ([35], No-define to disable) */
#define RESET_SKILL 35
#ifdef RESET_SKILL
 #define RESET_SKILL_LEVELS	5
 #define RESET_SKILL_FEE	3000000
#endif

/* Do vampires not suffer Black Breath at all? */
#define VAMPIRES_BB_IMMUNE
/* Allow vampires to polymorph into vampiric mist at 40, obtaining some special feats? */
#define VAMPIRIC_MIST

/* Specialty: Do vampire istari gain access to occult Shadow school? */
#define VAMP_ISTAR_SHADOW

/* Will negative boni on cursed items become (scaled) positive ones when wielded by true vampires (RACE_VAMPIRE)
   or hell knights (CLASS_HELLKNIGHT), provided the item is eligible (HEAVY_CURSE)? - C. Blue
   (0 = rather inconsistent method, 1 = recommended method) */
#define VAMPIRES_INV_CURSED 1
#ifdef VAMPIRES_INV_CURSED
 /* New specialty (super-experimental): Change cursed randarts too (into something useful)? */
 #define INVERSE_CURSED_RANDARTS

 /* Will randarts retain their positive abilities on flipping? (mostly for auto-id) */
 #define INVERSE_CURSED_RETAIN
#endif

/* Allow ordering a specific item in a store */
#define ENABLE_ITEM_ORDER

/* Allow melting coins with high a magical fire-similar attack of high enough temperature into golem body pieces.
   The number is the cost factor for smelting coins into massive pieces; note: 7..17x is possible for default gold colour on player-dropped piles.
   Also allow melting items. */
#define SMELTING 8
/* Divide 'real' temperature * 10 by this to allow more easygoing usage if we just can't push the required numbers for technical reasons. */
#define SMELTING_DIV 20

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
 /* <this> x (MAX_MERCHANT_MAILS / cfg.fps) seconds  [36 -> 1 min, ie 36*100/60] */
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
/* ..Additionally cancel Word-of-recall inside space-time-anchor fields, instead of delaying it? */
#ifdef ANTI_TELE_CHEEZE
 //#define ANTI_TELE_CHEEZE_ANCHOR
#endif

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

#define DEATH_FATE_SPECIAL	/* Death Fate special */

/* Enable o_*.lua 'Occult' magic schools (shamans, rogues, adventurers) */
#define ENABLE_OCCULT

/* Allow 'Vampire Paladins' aka Death Knights. Requires ENABLE_OCCULT. */
#ifdef ENABLE_OCCULT
 #define ENABLE_DEATHKNIGHT

  #define ENABLE_HELLKNIGHT	/* Allow 'Corrupted Paladins' aka Hell Knights. Requires ENABLE_OCCULT. */
  #ifdef ENABLE_HELLKNIGHT
   #define ENABLE_OHERETICISM	/* Enable 'Hereticism' occult school for Hell Knights */
   #define ENABLE_CPRIEST	/* Allow 'Corrupted Priest', keeping its normal class name. Should assume ENABLE_HELLKNIGHT. */
  #endif

 #define ENABLE_OUNLIFE	/* Enable 'Nether' occult school for Death Knights */
 #ifdef ENABLE_OUNLIFE /* forced implication code-wise! */
  #define ENABLE_OHERETICISM
 #endif
#endif

/* 'Necromancy' skill gives an additional chance to keep hold of your life force */
#define NECROMANCY_HOLDS_LIFE

/* Allow creation of demolition charges for 'Digging' skill.
   Consider renaming 'Digging' to 'Excavation'.
   Also designates minimum required 'Digging' skill to be active [5]. */
#define ENABLE_DEMOLITIONIST 5
/* Restrict finding ENABLE_DEMOLITIONIST items to within the IDDC. Usage of found items is not restricted however. */
//#define DEMOLITIONIST_IDDC_ONLY
/* Restrict placing blast charges to within the IDDC, for debugging/testing purpose */
//#define DEMOLITIONIST_BLAST_IDDC_ONLY

/* Allow wielding spell books? (count as 2-handed item, count as bare-handed aka 'no item equipped and no martial arts applied' for attacking)
   Spells cast from that book get boni:
    -20% MP cost,
    -5% fail rate,
    +1 spell level (does not affect fail rate but does affect MP cost, but nowadays MP cost is fixed anyway, so no effect;
                   does not affect spells that were not yet castable, similar to how the Spell-power skill works). */
#define WIELD_BOOKS
/* Allow wielding magic devices: Wands, Staves, Rods, in the manner of WIELD_BOOKS?
   Boni:
    30% reduced fail chance, flat on top,
    20% chance to retain the charge/energy at the cost of MP (depending on the device level). */
#define WIELD_DEVICES



/* ------------------------------------------------------------------------------------------------- */
/* --------------------- TESTING/EXPERIMENTAL - This stuff is hot alpha/beta.. --------------------- */
/* ------------------------------------------------------------------------------------------------- */

/* Enable DM "adventure" modules, including save/load of entire cave floor files from the ] menu client-side.
   Multiple modules could be loaded with the quest/event frameworks for detailed adventure sites. - Kurzel */
#define DM_MODULES
#ifdef DM_MODULES
 #define DM_MODULES_DUNGEON_SIZE 9	/* Number of dungeon floors required to host all the modules */
#endif

/* Allow to press alt-wield (shift+W) to equip a digging tool into the weapon slot! (4.7.4b+ test).
   Note that digging tools must all receive MUST2H flag for this. - C. Blue */
#define EQUIPPABLE_DIGGERS

/* Alchemy Satchels (and chests, for now) as inventory extension - C. Blue */
#define ENABLE_SUBINVEN

/* Finally create some usage for the so far functionless altars - C. Blue
   Ideas: Pray (1) or Sacrifice item (2).
   1) Gain extra quirks/buffs, but apply a behavioral restriction too? (Eg +luck, but mustn't kill GOOD monsters)
   2) Gain extra item type drop chance/quality/both. Gain +luck for itemvalue^.5 / 100 (fraction-fine scale), maybe add rarity bonus to luck too.
      Or determine duration by item rarity.
   Keep the effect for specific time, and even prolong it indefinitely while player doesn't change staircase/float direction and doesn't log out.
   Problem: lotr.fandom.com/wiki/Religion -> Ilúvatarism and Melkorism. But even if vamp/hk pray to Melkor, they still will kill him -_-. */
//#define ALTARS_2021

/* One of the bigger things - add actual crafting to the game? - C. Blue
    Goals:
	Allow to gain basically any possible item in the game, therefore exchanging targetted grind vs some RNG result.
	Depend on each other, with certain crafting results being ingredients in turn for some other item/craft.

    Problems:
	Loot drops are already fitting very well, crafting could dilute things too much.
	    -> ppl can dismantle 'useless' loot they find, to obtain raw materials for crafting,
	       giving every item a (small) purpose perhaps

	Ingredient spam, mixed into loot, requiring extra space in inventory/houses etc.
	    -> add a profession bag^^,
	    -> keep # of ingredients strongly in check, but offer a decent variery ot intermediary-tier items that are used up in subsequent crafts maybe

	Require stationary tools eg forge? Which symbol for those? & and % are interesting, but already overused, so just ':' left maybe.
	    -> could be cool to require very special/hidden forges etc perhaps.
	    -> not sure how this is solved in towns - every profession has its station? a bit cluttery. Maybe not all of them need one?

	Everyone becomes a crafter? Need to spend skill points? Need to create an extra crafting char? (Same as for Stealing atm.)
	Otoh it'd also not be good if everyone could be a crafter 'for free' anyway, diluting crafting a lot, instead of it being a perk of choice, to stand out.
	    -> some items could be too hard for anyone to use except the crafter, or w/e, just get level 0'ed if really necessary.
	    -> one char must be limited to one crafting profession
	    -> profession skill goes from 0.000 to 50.000 so it aligns with charlevel, but needs a bigger ratio to require maybe just ~5 levels worth of skill points (25)
	    -> specialty: while skill does increase your potential crafting level, there is a cLev that needs to be brought up to the skill-unlocked mLev of crafting,
	       by actually performing crafting actions! Eg 20.000 blacksmithing, but your cur-skill is at 3.418 because you have only created 3 helmets so far ^^-
	       The amount of work to put into it should increase polynomially, basically comparable to XP gain from monster-slaying, for the parallel 'game of crafting' vs 'game of monster-slaying'.

    Crafts:
	Blacksmith		requires Fletcher for hafted weapons,
				Textile Worker for heavy armour and shields,
				crafts swords etc w/o any supply chain items (just pure metal).

	Textile Worker		cloth and leather armour,
				inlays for heavy armour

	Fletcher		ranged weapons,
				staves (anything w/o metal warheads),
				traps,
				handles for heavy weapons

	* maybe combine Textule and Fletcher? Seems not much to do @ cloth and leather armour really.. except if it includes DSMs maybe..
	  could become weaver or plaiter..

	Alchemist		crafts potions,
				can create volatile runes,
				uses volatile runes to craft scrolls and spells,
				uses engraved runes to craft devices.

	Rune scribe		crafts runes, both volatile and engraved,
				can create scrolls and spells from volatile runes,
				can create runemaster-usable rune stones (aka 'runes' in the game right now),
				uses engraved runes to enchant items permanently

	2 types of runes:
		1) volatile runes (paper runes^^)
			these are for scrolls, spells
		2) engraved runes (on solid materials, rock, metal, wood)
			these are for enchantments (items) and magic devices, and for runecraft

    In-game symbols:
	Much like for demolition: * for ingredients probably.
	':' for stationary tools such as a forge perhaps. '%' and especially '&' would be nice but are overused,
	or we move/switch those around a bit..

    Tool items (pocketed, stationary), Ingredients/material items:
	Blacksmithing
	    hammer, forge
	    metal bars, mithril bars, adamantite bars
	Textileworking
	    needle/cutter, loom
	    cloth, leather
	Fletchery
	    slicer, workbench
	    wood, leather
	Alchemy
	    mixing flask (potions), pen (scrolls/spells), laboratory
	    paper/ink, various tinctures/essences
	Runescript
	    pen/chisel
	    ink, paper/rocks, - (?)
*/
//#define CRAFTING_2023

/* Work in progress // debug code - do not enable this!
   cmd_map(): Support initiating '2'/'8' map scrolling  while in 's'elector mode,
   by moving the X vertically out of the map
   (ie onto screen line 0 or 45 in 46-lines mode, aka big_map)?
   Note that retrieval from 'minimap_yoff' from server-side is just needed for this. - C. Blue */
//#define WILDMAP_ALLOW_SELECTOR_SCROLLING



/* ------------------------------------------------------------------------- */
/* --------------------- Server-type dependant features -------------------- */
/* ------------------------------------------------------------------------- */

/* Specific settings for rpg-server ("ironman server") only */
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

/* Specific settings for test-server only */
#ifdef TEST_SERVER
 /* Allow subclassing ie. planned access to skills from a secondary class!
   Enables role customization while preserving unique features of base classes.
   Currently applies a big +200% XP penalty for 2/3 ratios from both classes. */
 #define ENABLE_SUBCLASS
 #define ENABLE_SUBCLASS_TITLE // show secondary title when long format is called for (eg. @ screen)
 #define ENABLE_SUBCLASS_COLOR // alternate color in cave.c but not in @ screen (where title is seen)

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

 #define TELEPORT_SURPRISES 5	/* monsters are surprised for a short moment (0.1s * n) if a player long-range teleported next to them */

 #define LIMIT_SPELLS		/* Allow player to limit the level of spells he casts */

 /* Just for debugging - unbind savegames from accounts */
 #define IGNORE_SAVEGAME_MISMATCH

 /* Biggest can of worms evah: Inter-server portals */
 #define SERVER_PORTALS

 /* Harsh weather gives us trouble of some sort? */
 #define IRRITATING_WEATHER /* TODO: Fix weather code, see pos_in_weather() and two related code parts commented about there */
#endif

/* Specific settings for Arcade server only */
#ifdef ARCADE_SERVER
#endif

/* Specific settings for main-server only */
#if !defined(RPG_SERVER) && !defined(TEST_SERVER) && !defined(ARCADE_SERVER)
#endif



/* -------------------------------------------------------------------------- */
/* ------------------------ Client-side only features ----------------------- */
/* -------------------------------------------------------------------------- */

#ifdef CLIENT_SIDE
//Originally for hybrid clients to do both old and new runemastery, now unused.

 /* Use a somewhat lighter 'dark blue' for TERM_BLUE to improve readability
    on some screens / under certain circumstances? */
 //#define READABILITY_BLUE --has been turned into a config file option!

 /* Use a somewhat darker 'light dark' for TERM_L_DARK to improve distinction
    from slate tone? */
 #define DISTINCT_DARK

 /* Remove some hard-coding in the client options */
 #define CO_BIGMAP		7
 #define CO_PALETTE_ANIMATION	124

 /* Blacken lower part of screen if we're mindlinked to a non-bigmap-target but
    are actually using bigmap-screen. */
 #ifdef TEST_CLIENT
  #define BIGMAP_MINDLINK_HACK
 #endif

 /* Atmospheric login screens, with animation, sound and music? */
 #define ATMOSPHERIC_INTRO

/* 4.6.2: Allow to retry login, for re-entering invalid account/character names or after death. */
 #define RETRY_LOGIN

 /* Buffer guide in RAM, to reduce searching times (especially on Windows OS, not really bad on Linux) */
 #define BUFFER_GUIDE
 #ifdef BUFFER_GUIDE
  #define GUIDE_LINES_MAX 35000 //note: the guide is currently 25074 lines long
 #endif
 #define BUFFER_LOCAL_FILE
 #ifdef BUFFER_LOCAL_FILE
  #define LOCAL_FILE_LINES_MAX 35000 //actually, r_info.txt is already 20640 lines long :o
 #endif

 /* Use regex.h to offer regexp in-game guide searching; and now also auto-inscription regexp matching */
 #define REGEX_SEARCH

 /* Enable bookmarking feature? */
 #define GUIDE_BOOKMARKS 20

 /* Disable c_cfg.big_map option and make it a client-global setting instead that spans over any login choice. */
 #define GLOBAL_BIG_MAP

 /* experimental: Allow things like SHIFT+ENTER in menus - C. Blue */
 #ifdef TEST_CLIENT
  #define ENABLE_SHIFT_SPECIALKEYS
 #endif

 /* Allow redefining black colour (#0) too, and allow redefining any colour to #000000 (aka allow redefining all colours completely and freely)? */
 #define CUSTOMIZE_COLOURS_FREELY

 /* Enable hack in inkey() to allow using right/left arrow keys and pos1/end inside a text input prompt.
   Note that this hack is only active while inkey_interact_macros = TRUE or net_fd = -1 (client startup phase).
   - net_fd is -1 in the TomeNET startup login screen and account overview screen.
   And inkey_interact_macros is TRUE in...
   - the macro menu, when prompted for file names.
   - cmd_the_guide() and browse_local_file() - may currently be commented out or not, see comment there, change anytime if desired.
   - cmd_spoilers().
   - cmd_check_misc() (the knowledge menu).
   - auto_inscriptions().
   It is FALSE especially when typing in a chat message, so panic macros will still work if player is unexpectedly attacked by a monster while typing.
  */
 #define ALLOW_NAVI_KEYS_IN_PROMPT
 #ifdef ALLOW_NAVI_KEYS_IN_PROMPT
  /* During string input (chat messages!) navigational keys will actually override any macros put on them, not even normal macros on them will work.
     Only some navigational keys will do this: Those that actually have a real function in string input.  */
  #define SOME_NAVI_KEYS_DISABLE_MACROS_IN_PROMPTS
 #endif
#endif



/* -------------------------------------------------------------------------- */
/* ----------------- Misc flags induced by above definitions ---------------- */
/* -------------------------------------------------------------------------- */

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
 #ifdef USE_PARRYING
  /* Chance +AC enchantment on weapons to a bonus in parry chance? Requires USE_PARRYING to be enabled. Also specifies the divisor*10 to translate +AC to +parry. [10]
     A drawback currently: The player can always know his true parry chance via checking in 'm' menu, even if the weapon hasn't been identified yet. */
  #define WEAPONS_NO_AC 10
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



/* --------------------------------------------------------------------------*/

/* Enable/undefine specific features locally */
#include "defines-features-local.h"
