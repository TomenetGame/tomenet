/* $Id$ */
/* File: init1.c */

/* Purpose: Initialization (part 1) -BEN- */

#define SERVER

#include "angband.h"

/*
 * This file is used to initialize various variables and arrays for the
 * Angband game.  Note the use of "fd_read()" and "fd_write()" to bypass
 * the common limitation of "read()" and "write()" to only 32767 bytes
 * at a time.
 *
 * Several of the arrays for Angband are built from "template" files in
 * the "lib/file" directory, from which quick-load binary "image" files
 * are constructed whenever they are not present in the "lib/data"
 * directory, or if those files become obsolete, if we are allowed.
 *
 * Warning -- the "ascii" file parsers use a minor hack to collect the
 * name and text information in a single pass.  Thus, the game will not
 * be able to load any template file with more than 20K of names or 60K
 * of text, even though technically, up to 64K should be legal.
 *
 * Note that if "ALLOW_TEMPLATES" is not defined, then a lot of the code
 * in this file is compiled out, and the game will not run unless valid
 * "binary template files" already exist in "lib/data".  Thus, one can
 * compile Angband with ALLOW_TEMPLATES defined, run once to create the
 * "*.raw" files in "lib/data", and then quit, and recompile without
 * defining ALLOW_TEMPLATES, which will both save 20K and prevent people
 * from changing the ascii template files in potentially dangerous ways.
 *
 * The code could actually be removed and placed into a "stand-alone"
 * program, but that feels a little silly, especially considering some
 * of the platforms that we currently support.
 */


#ifdef ALLOW_TEMPLATES


/*
 * Hack -- error tracking
 */
extern s16b error_idx;
extern s16b error_line;


/*
 * Hack -- size of the "fake" arrays
 */
extern u32b fake_name_size;
extern u32b fake_text_size;



/*** Helper arrays for parsing ascii template files ***/

/*
 * Monster Blow Methods
 */
static cptr r_info_blow_method[] =
{
	"*",
	"HIT",
	"TOUCH",
	"PUNCH",
	"KICK",
	"CLAW",
	"BITE",
	"STING",
	"XXX1",
	"BUTT",
	"CRUSH",
	"ENGULF",
	"CHARGE",	// "XXX2",
	"CRAWL",
	"DROOL",
	"SPIT",
	"EXPLODE",	// "XXX3",
	"GAZE",
	"WAIL",
	"SPORE",
	"XXX4",
	"BEG",
	"INSULT",
	"MOAN",
	"SHOW",		// "XXX5",
	"WHISPER",
	NULL
};


/*
 * Monster Blow Effects
 */
static cptr r_info_blow_effect[] =
{
	"*",
	"HURT",
	"POISON",
	"UN_BONUS",
	"UN_POWER",
	"EAT_GOLD",
	"EAT_ITEM",
	"EAT_FOOD",
	"EAT_LITE",
	"ACID",
	"ELEC",
	"FIRE",
	"COLD",
	"BLIND",
	"CONFUSE",
	"TERRIFY",
	"PARALYZE",
	"LOSE_STR",
	"LOSE_INT",
	"LOSE_WIS",
	"LOSE_DEX",
	"LOSE_CON",
	"LOSE_CHR",
	"LOSE_ALL",
	"SHATTER",
	"EXP_10",
	"EXP_20",
	"EXP_40",
	"EXP_80",
	"DISEASE",	// below are PernA additions
	"TIME",
        "INSANITY",
        "HALLU",
        "PARASITE",
	"DISARM",	// ToME-NET ones
	"FAMINE",
	"SEDUCE",
	NULL
};

/*
   "DISEASE",
   "TIME",
   "INSANITY",
   "HALLU",
   "PARASITE",

   "FAMINE",
   NULL
*/

/*
 * Monster race flags
 */
static cptr r_info_flags1[] =
{
	"UNIQUE",
	"QUESTOR",
	"MALE",
	"FEMALE",
	"CHAR_CLEAR",
	"CHAR_MULTI",
	"ATTR_CLEAR",
	"ATTR_MULTI",
	"FORCE_DEPTH",
	"FORCE_MAXHP",
	"FORCE_SLEEP",
	"FORCE_EXTRA",
	"FRIEND",
	"FRIENDS",
	"ESCORT",
	"ESCORTS",
	"NEVER_BLOW",
	"NEVER_MOVE",
	"RAND_25",
	"RAND_50",
	"ONLY_GOLD",
	"ONLY_ITEM",
	"DROP_60",
	"DROP_90",
	"DROP_1D2",
	"DROP_2D2",
	"DROP_3D2",
	"DROP_4D2",
	"DROP_GOOD",
	"DROP_GREAT",
	"DROP_USEFUL",
	"DROP_CHOSEN"
};

/*
 * Monster race flags
 */
static cptr r_info_flags2[] =
{
	"STUPID",
	"SMART",
	"CAN_SPEAK",	// "XXX1X2",
	"REFLECTING",	// "XXX2X2",
	"INVISIBLE",
	"COLD_BLOOD",
	"EMPTY_MIND",
	"WEIRD_MIND",
	"DEATH_ORB",	// "MULTIPLY",
	"REGENERATE",
	"SHAPECHANGER",	// "XXX3X2",
	"ATTR_ANY",	// "XXX4X2",
	"POWERFUL",
	"ELDRITCH_HORROR", 		//"XXX5X2",
	"AURA_FIRE",	// "XXX7X2",
	"AURA_ELEC",	// "XXX6X2",
	"OPEN_DOOR",
	"BASH_DOOR",
	"PASS_WALL",
	"KILL_WALL",
	"MOVE_BODY",
	"KILL_BODY",
	"TAKE_ITEM",
	"KILL_ITEM",
	"BRAIN_1",
	"BRAIN_2",
	"BRAIN_3",
	"BRAIN_4",
	"BRAIN_5",
	"BRAIN_6",
	"BRAIN_7",
	"BRAIN_8"
};

/*
 * Monster race flags
 */
static cptr r_info_flags3[] =
{
	"ORC",
	"TROLL",
	"GIANT",
	"DRAGON",
	"DEMON",
	"UNDEAD",
	"EVIL",
	"ANIMAL",
	"DRAGONRIDER",	// "XXX1X3",
	"GOOD",	// "XXX2X3",
	"AURA_COLD",	// "XXX3X3",
	"NONLIVING",	// "IM_TELE",
	"HURT_LITE",
	"HURT_ROCK",
	"SUSCEP_FIRE",	// "HURT_FIRE",
	"SUSCEP_COLD",	// "HURT_COLD",
	"IM_ACID",
	"IM_ELEC",
	"IM_FIRE",
	"IM_COLD",
	"IM_POIS",
	"RES_TELE",	// "IM_PSI",
	"RES_NETH",
	"RES_WATE",
	"RES_PLAS",
	"RES_NEXU",
	"RES_DISE",
	"UNIQUE_4",	// "RES_PSI",
	"NO_FEAR",
	"NO_STUN",
	"NO_CONF",
	"NO_SLEEP"
};

/*
 * Monster race flags
 */
/* spells 96-127 (inate) */
static cptr r_info_flags4[] =
{
	"SHRIEK",
	"UNMAGIC",		//	"CAUSE_4",
//	"XXX2X4",	//	"MULTIPLY",
	"S_ANIMAL",	// "XXX3X4",
	"ROCKET",	// "XXX4X4",
	"ARROW_1",	// former 1/2 (arrow), now arrow/light
	"ARROW_2",	// former 3/4 (missile), now shot/heavy
	"ARROW_3",	//	"ARROW_3", now bolt/heavy
	"ARROW_4",	//	"ARROW_4", now missile/heavy
	"BR_ACID",
	"BR_ELEC",
	"BR_FIRE",
	"BR_COLD",
	"BR_POIS",
	"BR_NETH",
	"BR_LITE",
	"BR_DARK",
	"BR_CONF",
	"BR_SOUN",
	"BR_CHAO",
	"BR_DISE",
	"BR_NEXU",
	"BR_TIME",
	"BR_INER",
	"BR_GRAV",
	"BR_SHAR",
	"BR_PLAS",
	"BR_WALL",
	"BR_MANA",
	"BR_DISI",	// "XXX8X4"
	"BR_NUKE",	// "XXX6X4",
	"MOAN",		//"XXX",    <- MOAN is for Halloween event :) -C. Blue
	"BOULDER",	// "XXX",
};

/*
 * Monster race flags
 */
/* spells 128-159 (normal) */
static cptr r_info_flags5[] =
{
	"BA_ACID",
	"BA_ELEC",
	"BA_FIRE",
	"BA_COLD",
	"BA_POIS",
	"BA_NETH",
	"BA_WATE",
	"BA_MANA",
	"BA_DARK",
	"DRAIN_MANA",
	"MIND_BLAST",
	"BRAIN_SMASH",
	"CURSE",	//	"CAUSE_1",
	"RAND_100",		//	"CAUSE_2",
	"BA_NUKE",	// "XXX5X4",
	"BA_CHAO",	// "XXX7X4",
	"BO_ACID",
	"BO_ELEC",
	"BO_FIRE",
	"BO_COLD",
	"BO_POIS",
	"BO_NETH",
	"BO_WATE",
	"BO_MANA",
	"BO_PLAS",
	"BO_ICEE",
	"MISSILE",
	"SCARE",
	"BLIND",
	"CONF",
	"SLOW",
	"HOLD"
};

/*
 * Monster race flags
 */
/* spells 160-191 (bizarre) */
static cptr r_info_flags6[] =
{
	"HASTE",
	"HAND_DOOM",	// "XXX1X6",
	"HEAL",
	"S_ANIMALS",	// "XXX2X6",
	"BLINK",
	"TPORT",
	"RAISE_DEAD",	// "XXX3X6",
        "S_BUG",	// "XXX4X6",
	"TELE_TO",
	"TELE_AWAY",
	"TELE_LEVEL",
        "S_RNG",	// "XXX5",
	"DARKNESS",
	"TRAPS",
	"FORGET",
        "S_DRAGONRIDER",	// "XXX6X6",
	"S_KIN",	// "XXX7X6",
        "S_HI_DEMON",	// "XXX8X6",
	"S_MONSTER",
	"S_MONSTERS",
	"S_ANT",
	"S_SPIDER",
	"S_HOUND",
	"S_HYDRA",
	"S_ANGEL",
	"S_DEMON",
	"S_UNDEAD",
	"S_DRAGON",
	"S_HI_UNDEAD",
	"S_HI_DRAGON",
	"S_WRAITH",
	"S_UNIQUE"
};

#if 0	/* flags6 */
	"RAISE_DEAD", /* ToDo: Implement RAISE_DEAD */
        "S_BUG",
        "S_RNG",
        "S_DRAGONRIDER",  /* DG : Summon DragonRider */
	"S_KIN",
        "S_HI_DEMON",
#endif

/*
 * most of r_info_flags7-9 are not implemented;
 * for now, they're simply to 'deceive' parser.	- Jir -
 */

/*
 * Monster race flags
 */
static cptr r_info_flags7[] =
{
	"AQUATIC",
	"CAN_SWIM",
	"CAN_FLY",
	"FRIENDLY",
		"PET",
		"MORTAL",
	"SPIDER",
	"NAZGUL",
		"DG_CURSE",
		"POSSESSOR",
	"NO_DEATH",
	"NO_TARGET",
	"AI_ANNOY",
		"AI_SPECIAL",
		"NEUTRAL",
		"DROP_ART",
		"DROP_RANDART",
		"AI_PLAYER",
		"NO_THEFT",
	"NEVER_ACT",
	"NO_ESP",
	"ATTR_BASE",
	"VORTEX",
		"OOD_20",
		"OOD_15",
		"OOD_10",
	"XXX7X26",
	"S_LOWEXP", //"XXX7X27",
	"S_NOEXP", //"XXX7X28",
	"ATTR_BREATH",	//"XXX7X29",
	"MULTIPLY",		//"XXX7X30",
	"DISBELIEVE",	//"XXX7X31",
};

/*
 * Monster race flags
 */
static cptr r_info_flags8[] =
{
	"WILD_ONLY",
	"WILD_TOWN",
	"WILD_EASY",	/* ground without requirements: town/shore/waste/grass/swamp */
	"WILD_SHORE",
	"WILD_OCEAN",
	"WILD_WASTE",
	"WILD_WOOD",
	"WILD_VOLCANO",
	"XXX8X08",
	"WILD_MOUNTAIN",
	"WILD_GRASS",
        "NO_CUT",
        "CTHANGBAND",
        "PERNANGBAND",
        "ZANGBAND",
        "JOKEANGBAND",
        "BASEANGBAND",
	"BLUEBAND",
	"XXX8X18",
	"XXX8X19",
	"XXX8X20",
	"XXX8X21",
	"XXX8X22",
	"XXX8X23",
	"XXX8X24",
	"XXX8X25",
	"XXX8X26",
	"XXX8X27",
	"XXX8X28",
	"CLIMB",
	"WILD_SWAMP",	/* ToDo: Implement Swamp */
        "WILD_TOO",
};


/*
 * Monster race flags - Drops
 */
static cptr r_info_flags9[] =
{
	"DROP_CORPSE",
	"DROP_SKELETON",
        "HAS_LITE",
        "MIMIC",

        "HAS_EGG",
        "IMPRESSED",
        "SUSCEP_ACID",
        "SUSCEP_ELEC",

        "SUSCEP_POIS",
        "KILL_TREES",
        "WYRM_PROTECT",
        "DOPPLEGANGER",

        "ONLY_DEPTH",
        "SPECIAL_GENE",
        "NEVER_GENE",
	"RES_ACID",

	"RES_ELEC",
	"RES_FIRE",
	"RES_COLD",
	"RES_POIS",

	"RES_LITE",
	"RES_DARK",
	"RES_BLIND",
	"RES_SOUND",

	"RES_SHARDS",
	"RES_CHAOS",
	"RES_TIME",
	"RES_MANA",

	"IM_WATER",
	"IM_TELE",
	"IM_PSI",
	"RES_PSI",
};

/*
 * Monster race flags - Additional stuff - C. Blue
 */
static cptr r_info_flags0[] =
{
	"S_HI_MONSTER",
	"S_HI_MONSTERS",
	"S_HI_UNIQUE",
	"ASTAR",//4
	"NO_ESCORT",
	"NO_NEST",
	"X00000040",
	"X00000080",//8
	"X00000100",
	"X00000200",
	"X00000400",
	"X00000800",//12
	"X00001000",
	"X00002000",
	"X00004000",
	"X00008000",//16
	"X00010000",
	"X00020000",
	"X00040000",
	"X00080000",//20
	"X00100000",
	"X00200000",
	"X00400000",
	"X00800000",//24
	"X01000000",
	"X02000000",
	"X04000000",
	"X08000000",//28
	"X10000000",
	"X20000000",
	"X40000000",
	"X80000000",//32
};

/*
 * Trap flags (PernAngband)
 */
static cptr t_info_flags[] =
{
        "CHEST",
        "DOOR",
        "FLOOR",
        "CHANGE",	// "XXX4",
        "SPECIAL_GENE",
        "XXX6",
        "XXX7",
        "XXX8",
        "XXX9",
        "XXX10",
        "XXX11",
        "XXX12",
        "XXX13",
        "XXX14",
        "XXX15",
        "XXX16",
        "LEVEL1",
        "LEVEL2",
        "LEVEL3",
        "LEVEL4",
        "XXX21",
        "XXX22",
        "XXX23",
        "XXX24",
        "XXX25",
        "XXX26",
        "XXX27",
        "XXX28",
        "XXX29",
        "XXX30",
        "EASY_ID",
        "NO_ID"
};

/*
 * Object flags
 */
static cptr k_info_flags1[] =
{
	"STR",
	"INT",
	"WIS",
	"DEX",
	"CON",
	"CHR",
	"MANA",
	"SPELL",	// "SPELL_SPEED",
	"STEALTH",
	"SEARCH",
	"INFRA",
	"TUNNEL",
	"SPEED",
	"BLOWS",
	"LIFE",	// "LIFE",
	"VAMPIRIC",	// "XXX4",
	"SLAY_ANIMAL",
	"SLAY_EVIL",
	"SLAY_UNDEAD",
	"SLAY_DEMON",
	"SLAY_ORC",
	"SLAY_TROLL",
	"SLAY_GIANT",
	"SLAY_DRAGON",
	"KILL_DRAGON",
        "KILL_DEMON",
        "KILL_UNDEAD",
	"BRAND_POIS",	//"XXX6",
	"BRAND_ACID",
	"BRAND_ELEC",
	"BRAND_FIRE",
	"BRAND_COLD"
};

/*
 * Object flags
 */
static cptr k_info_flags2[] =
{
	"SUST_STR",
	"SUST_INT",
	"SUST_WIS",
	"SUST_DEX",
	"SUST_CON",
	"SUST_CHR",
        "REDUC_FIRE",	//"INVIS",
        "REDUC_COLD",	//"LIFE",
	"IM_ACID",
	"IM_ELEC",
	"IM_FIRE",
	"IM_COLD",
        "REDUC_ELEC",	//"SENS_FIRE",
        "REDUC_ACID",	//"REFLECT",
	"FREE_ACT",
	"HOLD_LIFE",
	"RES_ACID",
	"RES_ELEC",
	"RES_FIRE",
	"RES_COLD",
	"RES_POIS",
	"RES_FEAR",		// "ANTI_MAGIC",
	"RES_LITE",
	"RES_DARK",
	"RES_BLIND",
	"RES_CONF",
	"RES_SOUND",
	"RES_SHARDS",
	"RES_NETHER",
	"RES_NEXUS",
	"RES_CHAOS",
	"RES_DISEN"
};

/*
 * Object flags
 */
static cptr k_info_flags3[] =
{
	"SH_FIRE",
	"SH_ELEC",
        "AUTO_CURSE",
        "DECAY",
	"NO_TELE",
	"NO_MAGIC",
	"WRAITH",
	"TY_CURSE",		// ---
	"EASY_KNOW",
	"HIDE_TYPE",
	"SHOW_MODS",
	"INSTA_ART",
	"FEATHER",
        "LITE1",	// "LITE",
	"SEE_INVIS",
	"NORM_ART",		// "TELEPATHY"
	"SLOW_DIGEST",
	"REGEN",
	"XTRA_MIGHT",
	"XTRA_SHOTS",
	"IGNORE_ACID",
	"IGNORE_ELEC",
	"IGNORE_FIRE",
	"IGNORE_COLD",
	"ACTIVATE",
	"DRAIN_EXP",
	"TELEPORT",
	"AGGRAVATE",
	"BLESSED",
	"CURSED",
	"HEAVY_CURSE",
	"PERMA_CURSE"
};

#if 1	// under construction

#if 1	// vs.monst traps flags. implemented.
/*
 * Trap flags
 */
static cptr k_info_flags2_trap[] =
{
        "AUTOMATIC_5",
        "AUTOMATIC_99",
        "KILL_GHOST",
        "TELEPORT_TO",
        "ONLY_DRAGON",
        "ONLY_DEMON",
        "XXX3",
        "XXX3",
        "ONLY_ANIMAL",
        "ONLY_UNDEAD",
        "ONLY_EVIL",
        "XXX3",
	"XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
        "XXX3",
};
#endif	// 0

/*
 * Object flags
 */
static cptr k_info_flags4[] =
{
        "NEVER_BLOW",
        "PRECOGNITION",
        "BLACK_BREATH",
        "RECHARGE",
        "FLY",
        "DG_CURSE",
        "SHOULD2H",
        "MUST2H",
        "COULD2H",
        "CLONE",
        "SPECIAL_GENE",
        "CLIMB",
        "FAST_CAST",
        "CAPACITY",
        "CHARGING",
        "CHEAPNESS",
        "FOUNTAIN",
        "ANTIMAGIC_50",
        "ANTIMAGIC_30",
        "ANTIMAGIC_20",
        "ANTIMAGIC_10",
        "EASY_USE",
        "IM_NETHER",
        "RECHARGED",
        "ULTIMATE",
        "AUTO_ID",
        "LITE2",
        "LITE3",
        "FUEL_LITE",
        "ART_EXP",
        "CURSE_NO_DROP",
        "NO_RECHARGE"
};

/*
 * Object flags
 */
static cptr k_info_flags5[] =
{
        "TEMPORARY",
        "DRAIN_MANA",
        "DRAIN_HP",
	"VORPAL",	//"XXX5",		
	"IMPACT",
        "CRIT",
        "ATTR_MULTI",
        "WOUNDING",
        "FULL_NAME",
        "LUCK",
        "IMMOVABLE",
        "LEVELS",
	"FORCE_DEPTH",
	"XXX",
	"IGNORE_DISEN",
	"RES_TELE",
	"SH_COLD",
	"IGNORE_MANA",
	"IGNORE_WATER",
	"RES_TIME",
	"RES_MANA",
	"IM_POISON",
	"IM_WATER",
	"RES_WATER",
	"REGEN_MANA",	// "XXX8X24",
	"DISARM",	// "XXX8X25",
	"NO_ENCHANT",	// "XXX8X26",
	"CHAOTIC",	//"XXX8X27",
	"INVIS",	//"XXX8X28",
	"REFLECT",	//"XXX8X29",
	"PASS_WATER",	// "NORM_ART" is already in TR3
	"WINNERS_ONLY",
};

/*
 * ESP flags
 */
cptr esp_flags[] =
{
        "ESP_ORC",
        "ESP_TROLL",
        "ESP_DRAGON",
        "ESP_GIANT",
        "ESP_DEMON",
        "ESP_UNDEAD",
        "ESP_EVIL",
        "ESP_ANIMAL",
        "ESP_DRAGONRIDER",
        "ESP_GOOD",
        "ESP_NONLIVING",
        "ESP_UNIQUE",
        "ESP_SPIDER",
	"XXX8X02",
	"XXX8X02",
	"XXX8X02",
	"XXX8X02",
	"XXX8X17",
	"XXX8X18",
	"XXX8X19",
	"XXX8X20",
	"XXX8X21",
	"XXX8X22",
	"XXX8X23",
	"XXX8X24",
	"XXX8X25",
	"XXX8X26",
	"XXX8X27",
	"R_ESP_LOW",
	"R_ESP_HIGH", /* "XXX8X29", */
	"R_ESP_ANY", /* "XXX8X02", */
        "ESP_ALL",
};

/* Specially handled properties for ego-items */

static cptr ego_flags[] =
{
        "SUSTAIN",
        "OLD_RESIST",
        "ABILITY",
        "R_ELEM",

        "R_LOW",
        "R_HIGH",
        "R_ANY",
        "R_DRAGON",

        "SLAY_WEAP",
        "DAM_DIE",
        "DAM_SIZE",
        "PVAL_M1",

        "PVAL_M2",
        "PVAL_M3",
        "PVAL_M5",
        "AC_M5",

        "XXX1",
        "XXX2",
        "XXX3",
        "XXXH1",

        "XXXH2",
        "XXXH3",
        "XXXH5",
        "XXXD3",

        "R_ESP",	// "TD_M1",
        "NO_SEED",	// "TD_M2",
        "LOW_ABILITY",
        "R_P_ABILITY",

        "R_STAT",
        "R_STAT_SUST",
        "R_IMMUNITY",
        "LIMIT_BLOWS"
};

#endif	// 0

/*
 * Feature flags
 */
static cptr f_info_flags1[] =
{
	"NO_WALK",
	"NO_VISION",
	"CAN_LEVITATE",
	"CAN_PASS",

	"FLOOR",
	"WALL",
	"PERMANENT",
	"CAN_FLY",

	"REMEMBER",
	"NOTICE",
	"DONT_NOTICE_RUNNING",
	"CAN_RUN",

	"DOOR",
	"SUPPORT_LIGHT",
	"CAN_CLIMB",
	"TUNNELABLE",

	"WEB",
	"ATTR_MULTI",
	"SLOW_RUNNING_1",
	"SLOW_RUNNING_2",

	"SLOW_FLYING_1",
	"SLOW_FLYING_2",
	"SLOW_CLIMBING_1",
	"SLOW_CLIMBING_2",

	"SLOW_WALKING_1",
	"SLOW_WALKING_2",
	"SLOW_SWIMMING_1",
	"SLOW_SWIMMING_2",

	"PROTECTED",	/* monsters cannot spawn on nor teleport to this grid. Addition for monster challenge arena: can't move onto it either! */
	"LOS",	/* cannot target/shoot/cast through this one, but may be able to walk through it ('easy door') */
	"BLOCK_LOS",	/* cannot target/shoot/cast through this one, but may be able to walk through it ('easy door') */
	"BLOCK_CONTACT"	/* like BLOCK_LOS, but allows the player to actually see across it */
};

#if 1	// flags from ToME, for next expansion

/*
 * Dungeon flags
 */
static cptr d_info_flags1[] =
{
	"PRINCIPAL",
	"MAZE",
	"SMALLEST",
	"SMALL",
	"BIG",
	"NO_DOORS",
	"WATER_RIVER",
	"LAVA_RIVER",
	"WATER_RIVERS",
	"LAVA_RIVERS",
	"CAVE",
	"CAVERN",
	"NO_UP",
	"HOT",
	"COLD",
	"FORCE_DOWN",
	"FORGET",
	"NO_DESTROY",
	"SAND_VEIN",
	"CIRCULAR_ROOMS",
	"EMPTY",
	"DAMAGE_FEAT",
	"FLAT",
	"TOWER",
	"RANDOM_TOWNS",
	"DOUBLE",
	"LIFE_LEVEL",
	"EVOLVE",
	"ADJUST_LEVEL_1",
	"ADJUST_LEVEL_2",
	"NO_RECALL",
	"NO_STREAMERS"
};

static cptr d_info_flags2[] =
{
	"RANDOM",
	"IRON",
	"HELL",
	"NO_RECALL_INTO",
//	"NO_MAP",	/* will be annexed to DF1_FORGET */
	"NO_MAGIC_MAP",
	"XXX1",
	"XXX1",
	"NO_DEATH",	/* 0x00000080L */
	"IRONFIX1",
	"IRONFIX2",
	"IRONFIX3",
	"IRONFIX4",
	"IRONRND1",
	"IRONRND2",
	"IRONRND3",
	"IRONRND4",
	"NO_ENTRY_STAIR",
	"NO_ENTRY_WOR",
	"NO_ENTRY_PROB",
	"NO_ENTRY_FLOAT",
	"NO_EXIT_STAIR",
	"NO_EXIT_WOR",
	"NO_EXIT_PROB",
	"NO_EXIT_FLOAT",
	"NO_STAIRS_UP",
	"NO_STAIRS_DOWN",
	"",
	"",
	"ADJUST_LEVEL_1_2",
	"NO_SHAFT",
	"ADJUST_LEVEL_PLAYER",
	"DELETED"	/* not likely */
};

#endif	// 0

/* Vault flags */
static cptr v_info_flags1[] =
{
	"FORCE_FLAGS",
	"NO_TELE",
	"NO_GENO",
	"NO_MAP",
	"NO_MAGIC_MAP",
	"NO_DESTROY",
	"NO_MAGIC",
	"XXX1",		// ASK_LEAVE
	"XXX1",
	"XXX1",
	"XXX1",
	"XXX1",		// DESC
	"XXX1",
	"XXX1",
	"XXX1",
	"XXX1",
	"XXX1",
	"XXX1",
	"XXX1",
	"XXX1",
	"NO_EASY_TRUEARTS",
	"NO_EASY_RANDARTS",
	"RARE_TRUEARTS",
	"RARE_RANDARTS",
	"NO_TRUEARTS",
	"NO_RANDARTS",
	"XXX1",
	"XXX1",
	"NO_PENETR",
	"HIVES",
	"NO_MIRROR",
	"NO_ROTATE"
};

/* Skill flags */
static cptr s_info_flags1[] =
{
        "HIDDEN",
        "AUTO_HIDE",
	"DUMMY",
	"MAX_1",

	"MAX_10",
	"MAX_20",
	"MAX_25",
	"XXX1",

	"XXX1",
	"XXX1",
	"XXX1",
	"XXX1",

	"XXX1",
	"XXX1",
	"XXX1",
	"XXX1",

	"XXX1",
	"XXX1",
	"XXX1",
	"XXX1",

	"XXX1",
	"XXX1",
	"XXX1",
	"XXX1",

	"XXX1",
	"XXX1",
	"MKEY_SCHOOL",		/* Below are Client flags */
	"MKEY_HARDCODE",

	"MKEY_SPELL",		/* Realm spells */
	"MKEY_TVAL",
	"MKEY_ITEM",
	"MKEY_DIRECTION"
};


/*
 * Dungeon effect types (used in E:damage:frequency:type entry in d_info.txt)
 */
static struct
{
	cptr name;
	int feat;
} d_info_dtypes[] =
{
	{"ELEC", GF_ELEC},
	{"POISON", GF_POIS},
	{"ACID", GF_ACID},
	{"COLD", GF_COLD},
	{"FIRE", GF_FIRE},
	{"MISSILE", GF_MISSILE},
	{"ARROW", GF_ARROW},
	{"PLASMA", GF_PLASMA},
	{"WATER", GF_WATER},
	{"LITE", GF_LITE},
	{"DARK", GF_DARK},
	{"LITE_WEAK", GF_LITE_WEAK},
	{"LITE_DARK", GF_DARK_WEAK},
	{"SHARDS", GF_SHARDS},
	{"SOUND", GF_SOUND},
	{"CONFUSION", GF_CONFUSION},
	{"FORCE", GF_FORCE},
	{"INERTIA", GF_INERTIA},
	{"MANA", GF_MANA},
	{"METEOR", GF_METEOR},
	{"ICE", GF_ICE},
	{"CHAOS", GF_CHAOS},
	{"NETHER", GF_NETHER},
	{"DISENCHANT", GF_DISENCHANT},
	{"NEXUS", GF_NEXUS},
	{"TIME", GF_TIME},
	{"GRAVITY", GF_GRAVITY},
	{"ROCKET", GF_ROCKET},
	{"NUKE", GF_NUKE},
	{"HOLY_FIRE", GF_HOLY_FIRE},
	{"HELL_FIRE", GF_HELL_FIRE},
	{"DISINTEGRATE", GF_DISINTEGRATE},
#if 0	// implement them first
	{"DESTRUCTION", GF_DESTRUCTION},
	{"RAISE", GF_RAISE},
#else	// 0
	{"DESTRUCTION", 0},
	{"RAISE", 0},
#endif	// 0
	{NULL, 0}
};

/*
 * Stores flags
 */
static cptr st_info_flags1[] =
{
	"DEPEND_LEVEL",
	"SHALLOW_LEVEL",
	"MEDIUM_LEVEL",
	"DEEP_LEVEL",
	"RARE",
	"VERY_RARE",
	"COMMON",
	"ALL_ITEM",
	"RANDOM",
	"FORCE_LEVEL",
	"MUSEUM",
	"NO_DISCOUNT",
	"NO_DISCOUNT2",
	"EGO",
	"RARE_EGO",
	"PRICE1",
	"PRICE2",
	"PRICE4",
	"PRICE16",
	"GOOD",
	"GREAT",
	"PRICY_ITEMS1",
	"PRICY_ITEMS2",
	"PRICY_ITEMS3",
	"PRICY_ITEMS4",
	"HARD_STEAL",
	"VHARD_STEAL",
	"SPECIAL",
	"BUY67",
	"BUY50",
	"NO_DISCOUNT3",
	"ZEROLEVEL"
};

/*** Initialize from ascii template files ***/



/* New: Parse for conditioned comments depending on compilation definitions - C. Blue
   Example: $RPG_SERVER$N:158:The Secret Weapon
   Usage: '$'/'%' <condition> '$'/'!'
	a leading '$' prepares a condition test
	a leading '%' ignores <condition> ("commented out")
	a final '$' tests for <condition>,
	a final '!' tests for NOT<condition> */
static bool invalid_server_conditions(char *buf)
{
	char cc[1024 + 1], m[20];
	int ccn = 1;
	bool invalid = FALSE;

	/* while loop to allow multiple macros appended to each other in the same line! */
	while (buf[0] == '$' || buf[0] == '%') {
		bool negation = FALSE, ignore = (buf[0] == '%');

		/* read the tag between $...$ symbols */
		while (buf[ccn] != '$' && buf[ccn] != '!' && buf[ccn] != '\0')
			{ cc[ccn - 1] = buf[ccn]; ccn++; }

		if (buf[ccn] == '\0') return TRUE; /* end of line reached, completely invalid */

		/* inverted rule? means that tag is between $...! symbols */
		if (buf[ccn] == '!') negation = TRUE;
		cc[ccn - 1] = '\0';
		/* remember the macro we found */
		strcpy(m, cc);

		/* In case the line passes and we will actually process it,
		   already prepare it by cutting away the leading $...$ tag. */
		strcpy(cc, buf + ccn + 1);
		strcpy(buf, cc);

		/* Test for leading '%' meaning this condition is currently commented out */
		if (ignore) continue;

		/* info:
		   'MAIN_SERVER' isn't defined in the actual program.
		   It's just a switch that means
		   "don't parse this line if any special macro is defined".
		   (negation: "parse this line if any special macro is defined".)
		   
		   Any other rule means
		   "only parse this line if that particular macro is defined".

		   And any negation of a rule means
		   "don't parse this line if that particular macro is defined".
		*/
#ifndef RPG_SERVER
		if (streq(m, "RPG_SERVER") && !negation) invalid = TRUE;
#else
		if (streq(m, "MAIN_SERVER") && !negation) invalid = TRUE;
		if (streq(m, "RPG_SERVER") && negation) invalid = TRUE;
#endif
#ifndef ARCADE_SERVER
		if (streq(m, "ARCADE_SERVER") && !negation) invalid = TRUE;
#else
		if (streq(m, "MAIN_SERVER") && !negation) invalid = TRUE;
		if (streq(m, "ARCADE_SERVER") && negation) invalid = TRUE;
#endif

		/* special flags that can occur additionally to server types */
//#ifndef HALLOWEEN
if (!season_halloween) {
		if (streq(m, "HALLOWEEN") && !negation) invalid = TRUE;
//#else
} else {
		if (streq(m, "HALLOWEEN") && negation) invalid = TRUE;
//#endif
}
//#ifndef WINTER_SEASON
if (season != SEASON_WINTER) {
		if (streq(m, "WINTER_SEASON") && !negation) invalid = TRUE;
//#else
} else {
		if (streq(m, "WINTER_SEASON") && negation) invalid = TRUE;
//#endif
}
if (!season_xmas) {
		if (streq(m, "XMAS") && !negation) invalid = TRUE;
} else {
		if (streq(m, "XMAS") && negation) invalid = TRUE;
}
//#ifndef NEW_YEARS_EVE
if (!season_newyearseve) {
		if (streq(m, "NEW_YEARS_EVE") && !negation) invalid = TRUE;
//#else
} else {
		if (streq(m, "NEW_YEARS_EVE") && negation) invalid = TRUE;
//#endif
}
#ifndef ENABLE_DIVINE
		if (streq(m, "ENABLE_DIVINE") && !negation) invalid = TRUE;
#else
		if (streq(m, "ENABLE_DIVINE") && negation) invalid = TRUE;
#endif
#ifndef USE_NEW_SHIELDS
		if (streq(m, "USE_NEW_SHIELDS") && !negation) invalid = TRUE;
#else
		if (streq(m, "USE_NEW_SHIELDS") && negation) invalid = TRUE;
#endif
#ifndef DUAL_WIELD
		if (streq(m, "DUAL_WIELD") && !negation) invalid = TRUE;
#else
		if (streq(m, "DUAL_WIELD") && negation) invalid = TRUE;
#endif
#ifndef ENABLE_STANCES
		if (streq(m, "ENABLE_STANCES") && !negation) invalid = TRUE;
#else
		if (streq(m, "ENABLE_STANCES") && negation) invalid = TRUE;
#endif
#ifndef ENABLE_MCRAFT
		if (streq(m, "ENABLE_MCRAFT") && !negation) invalid = TRUE;
#else
		if (streq(m, "ENABLE_MCRAFT") && negation) invalid = TRUE;
#endif
#ifndef NEW_TOMES
		if (streq(m, "NEW_TOMES") && !negation) invalid = TRUE;
#else
		if (streq(m, "NEW_TOMES") && negation) invalid = TRUE;
#endif
#ifndef ENABLE_RCRAFT
		if (streq(m, "ENABLE_RCRAFT") && !negation) invalid = TRUE;
#else
		if (streq(m, "ENABLE_RCRAFT") && negation) invalid = TRUE;
#endif
//#ifndef PRECIOUS_STONES
//		if (streq(m, "PRECIOUS_STONES") && !negation) invalid = TRUE;
//#else
//		if (streq(m, "PRECIOUS_STONES") && negation) invalid = TRUE;
//#endif

		/* List all known flags. If we hit an unknown flag, ignore the line by default! */
		if (strcmp(m, "MAIN_SERVER") &&
		    strcmp(m, "RPG_SERVER") &&
		    strcmp(m, "ARCADE_SERVER") &&
		    strcmp(m, "HALLOWEEN") &&
		    strcmp(m, "WINTER_SEASON") &&
		    strcmp(m, "NEW_YEARS_EVE") &&
		    strcmp(m, "ENABLE_DIVINE") &&
		    strcmp(m, "USE_NEW_SHIELDS") &&
		    strcmp(m, "DUAL_WIELD") &&
		    strcmp(m, "ENABLE_STANCES") &&
		    strcmp(m, "ENABLE_MCRAFT") &&
		    strcmp(m, "NEW_TOMES") &&
		    strcmp(m, "ENABLE_RCRAFT") &&
//		    strcmp(m, "PRECIOUS_STONES") &&
			TRUE)
			invalid = TRUE;
	}
	
	return invalid; /* only pass if none of our tests proves to be invalid */
}

/*
 * Grab one race flag from a textual string
 */
static bool unknown_shut_up = FALSE;
static errr grab_one_class_flag(s32b *choice, cptr what)
{
	int i;
	cptr s;

	/* Scan classes flags */
//	for (i = 0; i < max_c_idx && (s = class_info[i].title + c_name); i++)
	for (i = 0; i < MAX_CLASS && (s = class_info[i].title); i++)
	{
		if (streq(what, s))
		{
			(choice[i / 32]) |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	if (!unknown_shut_up) s_printf("Unknown class flag '%s'.\n", what);

	/* Failure */
	return (1);
}
static errr grab_one_race_allow_flag(s32b *choice, cptr what)
{
	int i;
	cptr s;

#ifndef ENABLE_DIVINE
	/* Hack, so ow_info.txt works */
	if (streq(what, "Maia")) return 0;
#endif

	/* Scan classes flags */
//	for (i = 0; i < max_rp_idx && (s = race_info[i].title + rp_name); i++)
	for (i = 0; i < MAX_RACE && (s = race_info[i].title); i++) {
		if (streq(what, s)) {
			(choice[i / 32]) |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	if (!unknown_shut_up) s_printf("(1)Unknown race flag '%s'.\n", what);

	/* Failure */
	return (1);
}

/*
 * Grab one realm from a textual string
 */
static errr grab_one_player_realm_flag(s32b *realms, cptr what)
{
#if 0	// basically, always fail
	int i;

	/* Check flags1 */
	for (i = 1; i < MAX_REALM; i++)
	{
		if (streq(what, realm_names[i][0]))
		{
			(realms[(i - 1) / 32]) |= (1L << (i - 1));
			return (0);
		}
	}
#endif	// 0

	/* Oops */
	if (!unknown_shut_up) s_printf("Unknown realm name '%s'.\n", what);

	/* Error */
	return (1);
}

/*
 * Grab one flag in a vault_type from a textual string
 */
static errr grab_one_vault_flag(vault_type *v_ptr, cptr what)
{
	int i;

	/* Check flags1 */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, v_info_flags1[i]))
		{
                        v_ptr->flags1 |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("Unknown vault flag '%s'.\n", what);

	/* Error */
	return (1);
}


/*
 * Initialize the "v_info" array, by parsing an ascii "template" file
 */
errr init_v_info_txt(FILE *fp, char *buf)
{
	int i;

	char *s, *t;

	/* Not ready yet */
	bool okay = FALSE;

	/* Current entry */
	vault_type *v_ptr = NULL;


	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Prepare the "fake" stuff */
	v_head->name_size = 0;
	v_head->text_size = 0;

	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE))
	{
		/* Advance the line number */
		error_line++;

		/* parse server conditions */
		if (invalid_server_conditions(buf)) continue;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			int v1, v2, v3;

			/* Scan for the values */
			if ((3 != sscanf(buf, "V:%d.%d.%d", &v1, &v2, &v3)) ||
			    (v1 != v_head->v_major) ||
			    (v2 != v_head->v_minor) ||
			    (v3 != v_head->v_patch))
			{
				/* It only annoying -- DG */
//				return (2);
//				s_printf("Warning: different version file(%d.%d.%d)\n", v1, v2, v3);
			}

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf + 2);

			/* Verify information */
			if (i <= error_idx) return (4);

			/* Verify information */
			if (i >= (int) v_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
			v_ptr = &v_info[i];

			/* Hack -- Verify space */
			if (v_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
			if (!v_ptr->name) v_ptr->name = ++v_head->name_size;

			/* Append chars to the name */
			strcpy(v_name + v_head->name_size, s);

			/* Advance the index */
			v_head->name_size += strlen(s);

			/* Next... */
			continue;
		}

		/* There better be a current v_ptr */
		if (!v_ptr) return (3);


		/* Process 'D' for "Description" */
		if (buf[0] == 'D')
		{
			/* Acquire the text */
			s = buf + 2;

			/* Hack -- Verify space */
			if (v_head->text_size + strlen(s) + 8 > fake_text_size) return (7);

			/* Advance and Save the text index */
			if (!v_ptr->text) v_ptr->text = ++v_head->text_size;

			/* Append chars to the name */
			strcpy(v_text + v_head->text_size, s);

			/* Advance the index */
			v_head->text_size += strlen(s);

			/* Next... */
			continue;
		}


		/* Process 'X' for "Extra info" (one line only) */
		if (buf[0] == 'X')
		{
			int typ, rat, hgt, wid;

			/* Scan for the values */
			if (4 != sscanf(buf + 2, "%d:%d:%d:%d",
			                &typ, &rat, &hgt, &wid)) return (1);

			/* Save the values */
			v_ptr->typ = typ;
			v_ptr->rat = rat;
			v_ptr->hgt = hgt;
			v_ptr->wid = wid;

			/* Next... */
			continue;
		}

		/* Hack -- Process 'F' for flags */
		if (buf[0] == 'F')
		{
			/* Parse every entry textually */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

				/* Parse this entry */
				if (0 != grab_one_vault_flag(v_ptr, s)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
	++v_head->name_size;
	++v_head->text_size;


	/* No version yet */
	if (!okay) return (2);


	/* Success */
	return (0);
}


#if 0
/*
 * Initialize the "f_info" array, by parsing an ascii "template" file
 */
errr init_f_info_txt(FILE *fp, char *buf)
{
	int i;

	char *s;

	/* Not ready yet */
	bool okay = FALSE;

	/* Current entry */
	feature_type *f_ptr = NULL;


	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Prepare the "fake" stuff */
	f_head->name_size = 0;
	f_head->text_size = 0;

	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE))
	{
		/* Advance the line number */
		error_line++;

		/* parse server conditions */
		if (invalid_server_conditions(buf)) continue;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			int v1, v2, v3;

			/* Scan for the values */
			if ((3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) ||
			    (v1 != f_head->v_major) ||
			    (v2 != f_head->v_minor) ||
			    (v3 != f_head->v_patch))
			{
				/* It only annoying -- DG */
//				return (2);
//				s_printf("Warning: different version file(%d.%d.%d)\n", v1, v2, v3);
			}

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf + 2);

			/* Verify information */
			if (i <= error_idx) return (4);

			/* Verify information */
			if (i >= (int) f_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
			f_ptr = &f_info[i];

			/* Hack -- Verify space */
			if (f_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
			if (!f_ptr->name) f_ptr->name = ++f_head->name_size;

			/* Append chars to the name */
			strcpy(f_name + f_head->name_size, s);

			/* Advance the index */
			f_head->name_size += strlen(s);

			/* Default "mimic" */
			f_ptr->mimic = i;

			/* Next... */
			continue;
		}

		/* There better be a current f_ptr */
		if (!f_ptr) return (3);


#if 0

		/* Process 'D' for "Description" */
		if (buf[0] == 'D')
		{
			/* Acquire the text */
			s = buf + 2;

			/* Hack -- Verify space */
			if (f_head->text_size + strlen(s) + 8 > fake_text_size) return (7);

			/* Advance and Save the text index */
			if (!f_ptr->text) f_ptr->text = ++f_head->text_size;

			/* Append chars to the name */
			strcpy(f_text + f_head->text_size, s);

			/* Advance the index */
			f_head->text_size += strlen(s);

			/* Next... */
			continue;
		}

#endif


		/* Process 'M' for "Mimic" (one line only) */
		if (buf[0] == 'M')
		{
			int mimic;

			/* Scan for the values */
			if (1 != sscanf(buf + 2, "%d",
			                &mimic)) return (1);

			/* Save the values */
			f_ptr->mimic = mimic;

			/* Next... */
			continue;
		}


		/* Process 'G' for "Graphics" (one line only) */
		if (buf[0] == 'G')
		{
			int tmp;

			/* Paranoia */
			if (!buf[2]) return (1);
			if (!buf[3]) return (1);
			if (!buf[4]) return (1);

			/* Extract the color */
			tmp = color_char_to_attr(buf[4]);
			if (tmp < 0) return (1);

			/* Save the values */
			f_ptr->f_char = buf[2];
			f_ptr->f_attr = tmp;

			/* Next... */
			continue;
		}


		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
	++f_head->name_size;
	++f_head->text_size;


	/* No version yet */
	if (!okay) return (2);


	/* Success */
	return (0);
}
#else // 0

/*
 * Grab one flag in an feature_type from a textual string
 */
static errr grab_one_feature_flag(feature_type *f_ptr, cptr what)
{
	int i;

	/* Check flags1 */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, f_info_flags1[i]))
		{
                        f_ptr->flags1 |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("Unknown object flag '%s'.\n", what);

	/* Error */
	return (1);
}


/*
 * Initialize the "f_info" array, by parsing an ascii "template" file
 */
errr init_f_info_txt(FILE *fp, char *buf)
{
	int i;

	char *s, *t;

	/* Not ready yet */
	bool okay = FALSE;
	u32b default_desc = 0, default_tunnel = 0, default_block = 0;

	/* Current entry */
	feature_type *f_ptr = NULL;


	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Prepare the "fake" stuff */
	f_head->name_size = 0;
	f_head->text_size = 0;

	/* Add some fake descs */
	default_desc = ++f_head->text_size;
	strcpy(f_text + f_head->text_size, "a wall blocking your way");
	f_head->text_size += strlen("a wall blocking your way");

	default_tunnel = ++f_head->text_size;
	strcpy(f_text + f_head->text_size, "You cannot tunnel through that.");
	f_head->text_size += strlen("You cannot tunnel through that.");

	default_block = ++f_head->text_size;
	strcpy(f_text + f_head->text_size, "a wall blocking your way");
	f_head->text_size += strlen("a wall blocking your way");

	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE))
	{
		/* Advance the line number */
		error_line++;

		/* parse server conditions */
		if (invalid_server_conditions(buf)) continue;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			int v1, v2, v3;

			/* Scan for the values */
			if ((3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) ||
					(v1 != f_head->v_major) ||
					(v2 != f_head->v_minor) ||
					(v3 != f_head->v_patch))
			{
				/* It only annoying -- DG */
				//				return (2);
				//				s_printf("Warning: different version file(%d.%d.%d)\n", v1, v2, v3);
			}

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf + 2);

			/* Verify information */
			if (i <= error_idx) return (4);

			/* Verify information */
			if (i >= (int) f_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
			f_ptr = &f_info[i];

			/* Hack -- Verify space */
			if (f_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
			if (!f_ptr->name) f_ptr->name = ++f_head->name_size;

			/* Append chars to the name */
			strcpy(f_name + f_head->name_size, s);

			/* Advance the index */
			f_head->name_size += strlen(s);

			/* Default "mimic" */
			f_ptr->mimic = i;
			f_ptr->text = default_desc;
			f_ptr->block = default_desc;
			f_ptr->tunnel = default_tunnel;
			f_ptr->block = default_block;

			/* Next... */
			continue;
		}

		/* There better be a current f_ptr */
		if (!f_ptr) return (3);


		/* Process 'D' for "Descriptions" */
		if (buf[0] == 'D')
		{
			/* Acquire the text */
			s = buf+4;

			/* Hack -- Verify space */
			if (f_head->text_size + strlen(s) + 8 > fake_text_size) return (7);

			switch (buf[2])
			{
				case '0':
					/* Advance and Save the text index */
					f_ptr->text = ++f_head->text_size;
					break;
				case '1':
					/* Advance and Save the text index */
					f_ptr->tunnel = ++f_head->text_size;
					break;
				case '2':
					/* Advance and Save the text index */
					f_ptr->block = ++f_head->text_size;
					break;
				default:
					return (6);
					break;
			}

			/* Append chars to the name */
			strcpy(f_text + f_head->text_size, s);

			/* Advance the index */
			f_head->text_size += strlen(s);

			/* Next... */
			continue;
		}


		/* Process 'M' for "Mimic" (one line only) */
		if (buf[0] == 'M')
		{
			int mimic;

			/* Scan for the values */
			if (1 != sscanf(buf + 2, "%d",
						&mimic)) return (1);

						/* Save the values */
						f_ptr->mimic = mimic;

						/* Next... */
						continue;
		}

		/* Process 'S' for "Shimmer" (one line only) */
		if (buf[0] == 'S')
		{
			char s0, s1, s2, s3, s4, s5, s6;

			/* Scan for the values */
			if (7 != sscanf(buf + 2, "%c:%c:%c:%c:%c:%c:%c",
						&s0, &s1, &s2, &s3, &s4, &s5, &s6)) return (1);

						/* Save the values */
						f_ptr->shimmer[0] = color_char_to_attr(s0);
						f_ptr->shimmer[1] = color_char_to_attr(s1);
						f_ptr->shimmer[2] = color_char_to_attr(s2);
						f_ptr->shimmer[3] = color_char_to_attr(s3);
						f_ptr->shimmer[4] = color_char_to_attr(s4);
						f_ptr->shimmer[5] = color_char_to_attr(s5);
						f_ptr->shimmer[6] = color_char_to_attr(s6);

						/* Next... */
						continue;
		}


		/* Process 'G' for "Graphics" (one line only) */
		if (buf[0] == 'G')
		{
			int tmp;

			/* Paranoia */
			if (!buf[2]) return (1);
			if (!buf[3]) return (1);
			if (!buf[4]) return (1);

			/* Extract the color */
			tmp = color_char_to_attr(buf[4]);

			/* Paranoia */
			if (tmp < 0) return (1);

			/* Save the values */
#if 0
			f_ptr->d_attr = tmp;
			f_ptr->d_char = buf[2];
#else	// 0
			f_ptr->f_attr = tmp;
			f_ptr->f_char = buf[2];
#endif	// 0

			/* Next... */
			continue;
		}

		/* Process 'E' for "Effects" (up to four lines) -SC- */
		if (buf[0] == 'E')
		{
			int side, dice, freq, type;
			cptr tmp;

			/* Find the next empty blow slot (if any) */
			for (i = 0; i < 4; i++) if ((!f_ptr->d_side[i]) &&
					(!f_ptr->d_dice[i])) break;

			/* Oops, no more slots */
			if (i == 4) return (1);

			/* Scan for the values */
			if (4 != sscanf(buf + 2, "%dd%d:%d:%d",
						&dice, &side, &freq, &type))
			{
				int j;

				if (3 != sscanf(buf + 2, "%dd%d:%d",
							&dice, &side, &freq)) return (1);

							tmp = buf + 2;
							for (j = 0; j < 2; j++)
							{
								tmp = strchr(tmp, ':');
								if (tmp == NULL) return(1);
								tmp++;
							}

							j = 0;

							while (d_info_dtypes[j].name != NULL)
								if (strcmp(d_info_dtypes[j].name, tmp) == 0)
								{
									f_ptr->d_type[i] = d_info_dtypes[j].feat;
									break;
								}
								else j++;

								if (d_info_dtypes[j].name == NULL) return(1);
			}
			else
				f_ptr->d_type[i] = type;

			freq *= 1;//was 10
			/* Save the values */
			f_ptr->d_side[i] = side;
			f_ptr->d_dice[i] = dice;
			f_ptr->d_frequency[i] = freq;

			/* Next... */
			continue;
		}

		/* Hack -- Process 'F' for flags */
		if (buf[0] == 'F')
		{
			/* Parse every entry textually */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

				/* Parse this entry */
				if (0 != grab_one_feature_flag(f_ptr, s)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}



		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
	++f_head->name_size;
	++f_head->text_size;


	/* No version yet */
	if (!okay) return (2);


	/* Success */
	return (0);
}
#endif	// 0


/*
 * Grab one flag in an object_kind from a textual string
 */
static errr grab_one_kind_flag(object_kind *k_ptr, cptr what)
{
	int i;

	/* Check flags1 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, k_info_flags1[i]))
		{
			k_ptr->flags1 |= (1L << i);
			return (0);
		}
	}

	/* Check flags2 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, k_info_flags2[i]))
		{
			k_ptr->flags2 |= (1L << i);
			return (0);
		}
	}

#if 1
        /* Check flags2 -- traps*/
	for (i = 0; i < 32; i++)
	{
                if (streq(what, k_info_flags2_trap[i]))
		{
			k_ptr->flags2 |= (1L << i);
			return (0);
		}
	}
#endif	// 0

	/* Check flags3 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, k_info_flags3[i]))
		{
			k_ptr->flags3 |= (1L << i);
			return (0);
		}
	}


        /* Check flags4 */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, k_info_flags4[i]))
		{
                        k_ptr->flags4 |= (1L << i);
			return (0);
		}
	}

        /* Check flags5 */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, k_info_flags5[i]))
		{
                        k_ptr->flags5 |= (1L << i);
			return (0);
		}
	}

        /* Check esp_flags */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, esp_flags[i]))
		{
                        k_ptr->esp |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("Unknown object flag '%s'.\n", what);

	/* Error */
	return (1);
}



/*
 * Initialize the "k_info" array, by parsing an ascii "template" file
 */
errr init_k_info_txt(FILE *fp, char *buf)
{
	int i, idx = 0;

	char *s, *t;

	/* Not ready yet */
	bool okay = FALSE;

	/* Current entry */
	object_kind *k_ptr = NULL;


	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Prepare the "fake" stuff */
	k_head->name_size = 0;
	k_head->text_size = 0;

	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE))
	{
		/* Advance the line number */
		error_line++;

		/* parse server conditions */
		if (invalid_server_conditions(buf)) continue;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;
		
		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			int v1, v2, v3;

			/* Scan for the values */
			if ((3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) ||
			    (v1 != k_head->v_major) ||
			    (v2 != k_head->v_minor) ||
			    (v3 != k_head->v_patch))
			{
				/* It only annoying -- DG */
//				return (2);
//				s_printf("Warning: different version file(%d.%d.%d)\n", v1, v2, v3);
			}

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
//			i = atoi(buf + 2);

			/* Count it up */
			i = ++idx;

			/* Verify information */
			if (i <= error_idx) return (4);

			/* Verify information */
			if (i >= (int) k_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
			k_ptr = &k_info[i];

			/* Hack -- Verify space */
			if (k_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
			if (!k_ptr->name) k_ptr->name = ++k_head->name_size;

			/* Append chars to the name */
			strcpy(k_name + k_head->name_size, s);

			/* Advance the index */
			k_head->name_size += strlen(s);

			/* Next... */
			continue;
		}

		/* There better be a current k_ptr */
		if (!k_ptr) return (3);



		/* Process 'D' for "Description" */
		if (buf[0] == 'D')
		{
#if 0	// not used anyway
			/* Acquire the text */
			s = buf + 2;

			/* Hack -- Verify space */
			if (k_head->text_size + strlen(s) + 8 > fake_text_size) return (7);

			/* Advance and Save the text index */
			if (!k_ptr->text) k_ptr->text = ++k_head->text_size;

			/* Append chars to the name */
			strcpy(k_text + k_head->text_size, s);

			/* Advance the index */
			k_head->text_size += strlen(s);

#endif
			/* Next... */
			continue;
		}



		/* Process 'G' for "Graphics" (one line only) */
		if (buf[0] == 'G')
		{
			char sym;
			int tmp;

			/* Paranoia */
			if (!buf[2]) return (1);
			if (!buf[3]) return (1);
			if (!buf[4]) return (1);

			/* Extract the char */
			sym = buf[2];

			/* Extract the attr */
			tmp = color_char_to_attr(buf[4]);

			/* Paranoia */
			if (tmp < 0) return (1);

			/* Save the values */
			k_ptr->k_char = sym;
			k_ptr->k_attr = tmp;

			/* Next... */
			continue;
		}

		/* Process 'I' for "Info" (one line only) */
		if (buf[0] == 'I')
		{
			int tval, sval, pval;

			/* Scan for the values */
			if (3 != sscanf(buf + 2, "%d:%d:%d",
			                &tval, &sval, &pval)) return (1);

			/* Save the values */
			k_ptr->tval = tval;
			k_ptr->sval = sval;
			k_ptr->pval = pval;

			/* Next... */
			continue;
		}

		/* Process 'W' for "More Info" (one line only) */
		if (buf[0] == 'W')
		{
			int level, extra, wgt;
			long cost;

			/* Scan for the values */
			if (4 != sscanf(buf + 2, "%d:%d:%d:%ld",
			                &level, &extra, &wgt, &cost)) return (1);

			/* Save the values */
			k_ptr->level = level;
			k_ptr->extra = extra;
			k_ptr->weight = wgt;
			k_ptr->cost = cost;

			/* Next... */
			continue;
		}

		/* Process 'A' for "Allocation" (one line only) */
		if (buf[0] == 'A')
		{
			int i;

			/* XXX XXX XXX Simply read each number following a colon */
			for (i = 0, s = buf+1; s && (s[0] == ':') && s[1]; ++i)
			{
				/* Default chance */
				k_ptr->chance[i] = 1;

				/* Store the attack damage index */
				k_ptr->locale[i] = atoi(s+1);

				/* Find the slash */
				t = strchr(s+1, '/');

				/* Find the next colon */
				s = strchr(s+1, ':');

				/* If the slash is "nearby", use it */
				if (t && (!s || t < s))
				{
					int chance = atoi(t+1);
					if (chance > 0) k_ptr->chance[i] = chance;
				}
			}

			/* Next... */
			continue;
		}

		/* Hack -- Process 'P' for "power" and such */
		if (buf[0] == 'P')
		{
			int ac, hd1, hd2, th, td, ta;

			/* Scan for the values */
			if (6 != sscanf(buf + 2, "%d:%dd%d:%d:%d:%d",
			                &ac, &hd1, &hd2, &th, &td, &ta)) return (1);

			k_ptr->ac = ac;
			k_ptr->dd = hd1;
			k_ptr->ds = hd2;
			k_ptr->to_h = th;
			k_ptr->to_d = td;
			k_ptr->to_a =  ta;

			/* Next... */
			continue;
		}

		/* Hack -- Process 'F' for flags */
		if (buf[0] == 'F')
		{
			/* Parse every entry textually */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

				/* Parse this entry */
				if (0 != grab_one_kind_flag(k_ptr, s)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}


		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
	++k_head->name_size;
	++k_head->text_size;


	/* No version yet */
	if (!okay) return (2);

#if DEBUG_LEVEL > 2
	/* Debug -- print total no. */
	s_printf("k_info total: %d\n", idx);
#endif	// DEBUG_LEVEL

	max_k_idx = ++idx;

	/* Success */
	return (0);
}


/*
 * Grab one flag in an artifact_type from a textual string
 */
static errr grab_one_artifact_flag(artifact_type *a_ptr, cptr what)
{
	int i;

	/* Check flags1 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, k_info_flags1[i]))
		{
			a_ptr->flags1 |= (1L << i);
			return (0);
		}
	}

	/* Check flags2 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, k_info_flags2[i]))
		{
			a_ptr->flags2 |= (1L << i);
			return (0);
		}
	}

	/* Check flags3 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, k_info_flags3[i]))
		{
			a_ptr->flags3 |= (1L << i);
			return (0);
		}
	}
#if 1

        /* Check flags2 -- traps (huh? - Jir -) */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, k_info_flags2_trap[i]))
		{
                        a_ptr->flags2 |= (1L << i);
			return (0);
		}
	}
#endif	// 0

        /* Check flags4 */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, k_info_flags4[i]))
		{
                        a_ptr->flags4 |= (1L << i);
			return (0);
		}
	}

        /* Check flags5 */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, k_info_flags5[i]))
		{
                        a_ptr->flags5 |= (1L << i);
			return (0);
		}
	}

        /* Check esp_flags */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, esp_flags[i]))
		{
                        a_ptr->esp |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("Unknown artifact flag '%s'.\n", what);

	/* Error */
	return (1);
}




/*
 * Initialize the "a_info" array, by parsing an ascii "template" file
 */
errr init_a_info_txt(FILE *fp, char *buf)
{
	int i;

	char *s, *t;

	/* Not ready yet */
	bool okay = FALSE;

	/* Current entry */
	artifact_type *a_ptr = NULL;


	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE))
	{
		/* Advance the line number */
		error_line++;

		/* parse server conditions */
		if (invalid_server_conditions(buf)) continue;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			int v1, v2, v3;

			/* Scan for the values */
			if ((3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) ||
			    (v1 != a_head->v_major) ||
			    (v2 != a_head->v_minor) ||
			    (v3 != a_head->v_patch))
			{
				/* It only annoying -- DG */
//				return (2);
//				s_printf("Warning: different version file(%d.%d.%d)\n", v1, v2, v3);
			}

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf + 2);

			/* Verify information */
			if (i < error_idx) return (4);

			/* Verify information */
			if (i >= (int) a_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
			a_ptr = &a_info[i];

			/* Hack -- Verify space */
			if (a_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
			if (!a_ptr->name) a_ptr->name = ++a_head->name_size;

			/* Append chars to the name */
			strcpy(a_name + a_head->name_size, s);

			/* Advance the index */
			a_head->name_size += strlen(s);

			/* Ignore everything */
			a_ptr->flags3 |= (TR3_IGNORE_ACID);
			a_ptr->flags3 |= (TR3_IGNORE_ELEC);
			a_ptr->flags3 |= (TR3_IGNORE_FIRE);
			a_ptr->flags3 |= (TR3_IGNORE_COLD);
			a_ptr->flags5 |= (TR5_IGNORE_WATER);

			/* Next... */
			continue;
		}

		/* There better be a current a_ptr */
		if (!a_ptr) return (3);



		/* Process 'D' for "Description" */
		if (buf[0] == 'D')
		{
#ifdef ART_DIZ
#if 1	// Hope they'll be handled in client-side someday
			/* Acquire the text */
			s = buf + 2;

			/* Hack -- Verify space */
			if (a_head->text_size + strlen(s) + 8 > fake_text_size) return (7);

			/* Advance and Save the text index */
			if (!a_ptr->text) a_ptr->text = ++a_head->text_size;

			/* Append chars to the name */
			strcpy(a_text + a_head->text_size, s);

			/* Advance the index */
			a_head->text_size += strlen(s);
#endif
#endif
			/* Next... */
			continue;
		}


		/* Process 'I' for "Info" (one line only) */
		if (buf[0] == 'I')
		{
			int tval, sval, pval;

			/* Scan for the values */
			if (3 != sscanf(buf + 2, "%d:%d:%d",
			                &tval, &sval, &pval)) return (1);

			/* Save the values */
			a_ptr->tval = tval;
			a_ptr->sval = sval;
			a_ptr->pval = pval;

			/* Next... */
			continue;
		}

		/* Process 'W' for "More Info" (one line only) */
		if (buf[0] == 'W')
		{
			int level, rarity, wgt;
			long cost;

			/* Scan for the values */
			if (4 != sscanf(buf + 2, "%d:%d:%d:%ld",
			                &level, &rarity, &wgt, &cost)) return (1);

			/* Save the values */
			a_ptr->level = level;
			a_ptr->rarity = rarity;
			a_ptr->weight = wgt;
			a_ptr->cost = cost;

			/* Next... */
			continue;
		}

		/* Hack -- Process 'P' for "power" and such */
		if (buf[0] == 'P')
		{
			int ac, hd1, hd2, th, td, ta;

			/* Scan for the values */
			if (6 != sscanf(buf + 2, "%d:%dd%d:%d:%d:%d",
			                &ac, &hd1, &hd2, &th, &td, &ta)) return (1);

			a_ptr->ac = ac;
			a_ptr->dd = hd1;
			a_ptr->ds = hd2;
			a_ptr->to_h = th;
			a_ptr->to_d = td;
			a_ptr->to_a =  ta;

			/* Next... */
			continue;
		}

                /* Process 'Z' for "Granted power" */
                if (buf[0] == 'Z')
		{
#if 0
                        int i;

			/* Acquire the text */
			s = buf + 2;

                        /* Find it in the list */
                        for (i = 0; i < power_max; i++)
                        {
                                if (!stricmp(s, powers_type[i].name)) break;
                        }

                        if (i == power_max) return (6);

                        a_ptr->power = i;

#endif	// 0
			/* Next... */
			continue;
		}

		/* Hack -- Process 'F' for flags */
		if (buf[0] == 'F')
		{
			/* Parse every entry textually */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while ((*t == ' ') || (*t == '|')) t++;
				}

				/* Parse this entry */
				if (0 != grab_one_artifact_flag(a_ptr, s)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}


		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
	++a_head->name_size;
	++a_head->text_size;


	/* No version yet */
	if (!okay) return (2);


	/* Success */
	return (0);
}

/*
 * Grab one flag from a textual string
 */
static errr grab_one_skill_flag(u32b *f1, cptr what)
{
	int i;

	/* Check flags1 */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, s_info_flags1[i]))
		{
			(*f1) |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("(2)Unknown skill flag '%s'.\n", what);

	/* Error */
	return (1);
}


/*
 * Initialize the "s_info" array, by parsing an ascii "template" file
 */
errr init_s_info_txt(FILE *fp, char *buf)
{
	int i, z, order = 1;

	char *s;

	/* Not ready yet */
	bool okay = FALSE;

	/* Current entry */
	skill_type *s_ptr = NULL;


	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE))
	{
		/* Advance the line number */
		error_line++;

		/* parse server conditions */
		if (invalid_server_conditions(buf)) continue;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			int v1, v2, v3;

#ifdef VERIFY_VERSION_STAMP

			/* Scan for the values */
			if ((3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) ||
			    (v1 != s_head->v_major) ||
			    (v2 != s_head->v_minor) ||
			    (v3 != s_head->v_patch))
			{
				return (2);
			}

#else /* VERIFY_VERSION_STAMP */

			/* Scan for the values */
			if (3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) return (2);

#endif /* VERIFY_VERSION_STAMP */

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'T' for "skill Tree" */
		if (buf[0] == 'T')
		{
			char *sec;
			s16b s1, s2;

			/* Scan for the values */
			if (NULL == (sec = strchr(buf + 2, ':')))
			{
				return (1);
			}
			*sec = '\0';
			sec++;
			if (!*sec) return (1);

			s1 = find_skill(buf + 2);
			s2 = find_skill(sec);
			if (s2 == -1) return (1);

			s_info[s2].father = s1;
			s_info[s2].order = order++;

			/* Next... */
			continue;
		}

		/* Process 'E' for "Exclusive" */
		if (buf[0] == 'E')
		{
			char *sec;
			s16b s1, s2;

			/* Scan for the values */
			if (NULL == (sec = strchr(buf + 2, ':')))
			{
				return (1);
			}
			*sec = '\0';
			sec++;
			if (!*sec) return (1);

			s1 = find_skill(buf + 2);
			s2 = find_skill(sec);
			if ((s1 == -1) || (s2 == -1)) return (1);

			s_info[s1].action[s2] = SKILL_EXCLUSIVE;
			s_info[s2].action[s1] = SKILL_EXCLUSIVE;

			/* Next... */
			continue;
		}


		/* Process 'O' for "Opposite" */
		if (buf[0] == 'O')
		{
			char *sec, *cval;
			s16b s1, s2;

			/* Scan for the values */
			if (NULL == (sec = strchr(buf + 2, ':')))
			{
				return (1);
			}
			*sec = '\0';
			sec++;
			if (!*sec) return (1);
			if (NULL == (cval = strchr(sec, '%')))
			{
				return (1);
			}
			*cval = '\0';
			cval++;
			if (!*cval) return (1);

			s1 = find_skill(buf + 2);
			s2 = find_skill(sec);
			if ((s1 == -1) || (s2 == -1)) return (1);

			s_info[s1].action[s2] = -atoi(cval);

			/* Next... */
			continue;
		}

                /* Process 'A' for "Amical/friendly" */
                if (buf[0] == 'f')
		{
			char *sec, *cval;
			s16b s1, s2;

			/* Scan for the values */
			if (NULL == (sec = strchr(buf + 2, ':')))
			{
				return (1);
			}
			*sec = '\0';
			sec++;
			if (!*sec) return (1);
			if (NULL == (cval = strchr(sec, '%')))
			{
				return (1);
			}
			*cval = '\0';
			cval++;
			if (!*cval) return (1);

			s1 = find_skill(buf + 2);
			s2 = find_skill(sec);
			if ((s1 == -1) || (s2 == -1)) return (1);

			s_info[s1].action[s2] = atoi(cval);

			/* Next... */
			continue;
		}

		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf + 2);

			/* Verify information */
			if (i >= (int) s_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
			s_ptr = &s_info[i];

			/* Hack -- Verify space */
			if (s_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
			if (!s_ptr->name) s_ptr->name = ++s_head->name_size;

			/* Append chars to the name */
			strcpy(s_name + s_head->name_size, s);

			/* Advance the index */
			s_head->name_size += strlen(s);

			/* Init */
			s_ptr->action_mkey = 0;
//			s_ptr->dev = FALSE;
			for (z = 0; z < MAX_SKILLS; z++)
//			for (z = 0; z < max_s_idx; z++)
			{
				s_ptr->action[z] = 0;                                
			}

			/* Next... */
			continue;
		}

		/* There better be a current s_ptr */
		if (!s_ptr) return (3);

		/* Process 'D' for "Description" */
		if (buf[0] == 'D')
		{
			/* Acquire the text */
			s = buf + 2;

			/* Hack -- Verify space */
			if (s_head->text_size + strlen(s) + 8 > fake_text_size) return (7);

			/* Advance and Save the text index */
			if (!s_ptr->desc)
			{
				s_ptr->desc = ++s_head->text_size;

				/* Append chars to the name */
				strcpy(s_text + s_head->text_size, s);

				/* Advance the index */
				s_head->text_size += strlen(s);
			}
			else
			{
				/* Append chars to the name */
				strcpy(s_text + s_head->text_size, format("\n%s", s));

				/* Advance the index */
				s_head->text_size += strlen(s) + 1;
			}

			/* Next... */
			continue;
		}

		/* Process 'A' for "Activation Description" */
		if (buf[0] == 'A')
		{
			char *txt;

			/* Acquire the text */
			s = buf + 2;

			if (NULL == (txt = strchr(s, ':'))) return (1);
			*txt = '\0';
			txt++;

			/* Hack -- Verify space */
			if (s_head->text_size + strlen(txt) + 8 > fake_text_size) return (7);

			/* Advance and Save the text index */
			if (!s_ptr->action_desc) s_ptr->action_desc = ++s_head->text_size;

			/* Append chars to the name */
			strcpy(s_text + s_head->text_size, txt);
			s_ptr->action_mkey = atoi(s);

			/* Advance the index */
			s_head->text_size += strlen(txt);

			/* Next... */
			continue;
		}

		/* Process 'I' for "Info" (one line only) */
		if (buf[0] == 'I')
		{
			int rate, tval;

			/* Scan for the values */
			if (2 != sscanf(buf + 2, "%d:%d", &rate, &tval))
			{
				return (1);
			}

			/* Save the values */
			s_ptr->rate = rate;
			s_ptr->tval = tval;

			/* Next... */
			continue;
		}

                /* Process 'F' for flags */
		if (buf[0] == 'F')
		{
                        char *t;

			/* Parse every entry textually */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while ((*t == ' ') || (*t == '|')) t++;
				}

				/* Parse this entry */
                                if (0 != grab_one_skill_flag(&(s_ptr->flags1), s)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
	++s_head->name_size;
	++s_head->text_size;


	/* No version yet */
	if (!okay) return (2);


	/* Success */
	return (0);
}


/*
 * Grab one flag in a ego-item_type from a textual string
 */
static bool grab_one_ego_item_flag(ego_item_type *e_ptr, cptr what, int n)
{
	int i;

	/* Check flags1 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, k_info_flags1[i]))
		{
                        e_ptr->flags1[n] |= (1L << i);
			return (0);
		}
	}

	/* Check flags2 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, k_info_flags2[i]))
		{
                        e_ptr->flags2[n] |= (1L << i);
			return (0);
		}
	}
#if 1
        /* Check flags2 -- traps */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, k_info_flags2_trap[i]))
		{
                        e_ptr->flags2[n] |= (1L << i);
			return (0);
		}
	}
#endif	// 0

	/* Check flags3 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, k_info_flags3[i]))
		{
                        e_ptr->flags3[n] |= (1L << i);
			return (0);
		}
	}
#if 1
        /* Check flags4 */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, k_info_flags4[i]))
		{
                        e_ptr->flags4[n] |= (1L << i);
			return (0);
		}
	}

        /* Check flags5 */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, k_info_flags5[i]))
		{
                        e_ptr->flags5[n] |= (1L << i);
			return (0);
		}
	}

        /* Check esp_flags */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, esp_flags[i]))
		{
                        e_ptr->esp[n] |= (1L << i);
			return (0);
		}
	}

        /* Check ego_flags */
	for (i = 0; i < 32; i++)
	{
                if (streq(what, ego_flags[i]))
		{
                        e_ptr->fego[n] |= (1L << i);
			return (0);
		}
	}
#endif	// 0
	/* Oops */
	s_printf("Unknown ego-item flag '%s'.\n", what);

	/* Error */
	return (1);
}




/*
 * Initialize the "e_info" array, by parsing an ascii "template" file
 */
errr init_e_info_txt(FILE *fp, char *buf)
{
//	int i;
        int i, cur_r = -1, cur_t = 0, j;

	char *s, *t;

	/* Not ready yet */
	bool okay = FALSE;

	/* Current entry */
	ego_item_type *e_ptr = NULL;


	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE))
	{
		/* Advance the line number */
		error_line++;

		/* parse server conditions */
		if (invalid_server_conditions(buf)) continue;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			int v1, v2, v3;

			/* Scan for the values */
			if ((3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) ||
			    (v1 != e_head->v_major) ||
			    (v2 != e_head->v_minor) ||
			    (v3 != e_head->v_patch))
			{
				/* It only annoying -- DG */
//				return (2);
//				s_printf("Warning: different version file(%d.%d.%d)\n", v1, v2, v3);
			}

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

#if 0 /* removed for Amulet of Telepathic Awareness */
			/* Paranoia -- require a name */
			if (!*s) return (1);
#else
			if (!*s) s--;
#endif

			/* Get the index */
			i = atoi(buf + 2);

			/* Verify information */
			if (i < error_idx) return (4);

			/* Verify information */
			if (i >= (int) e_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
			e_ptr = &e_info[i];

			/* Hack -- Verify space */
			if (e_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
			if (!e_ptr->name) e_ptr->name = ++e_head->name_size;
			
			/* Append chars to the name */
			strcpy(e_name + e_head->name_size, s);

			/* Advance the index */
			e_head->name_size += strlen(s);

                        /* Needed hack ported */
//                        e_ptr->power = -1;
                        cur_r = -1;
                        cur_t = 0;

                        for (j = 0; j < MAX_EGO_BASETYPES; j++)
                        {
                                e_ptr->tval[j] = 255;
                        }
                        for (j = 0; j < 5; j++)
                        {
                                e_ptr->rar[j] = 0;
                                e_ptr->flags1[j] = 0;
                                e_ptr->flags2[j] = 0;
                                e_ptr->flags3[j] = 0;
                                e_ptr->flags4[j] = 0;
                                e_ptr->flags5[j] = 0;
                                e_ptr->esp[j] = 0;
                        }

			/* Next... */
			continue;
		}

		/* There better be a current e_ptr */
		if (!e_ptr) return (3);



		/* Process 'D' for "Description" */
		if (buf[0] == 'D')
		{
#if 0
			/* Acquire the text */
			s = buf + 2;

			/* Hack -- Verify space */
			if (e_head->text_size + strlen(s) + 8 > fake_text_size) return (7);

			/* Advance and Save the text index */
			if (!e_ptr->text) e_ptr->text = ++e_head->text_size;

			/* Append chars to the name */
			strcpy(e_text + e_head->text_size, s);

			/* Advance the index */
			e_head->text_size += strlen(s);

#endif
			/* Next... */
			continue;
		}


		/* PernA flags	- Jir - */
                /* Process 'T' for "Tval/Sval" (up to 5 lines) */
                if (buf[0] == 'T')
		{
                        int tv, minsv, maxsv;
                        if (cur_t == MAX_EGO_BASETYPES) {
				s_printf("ERROR: Exceeded MAX_EGO_BASETYPES in ego index %d.\n", error_idx);
                        	return (1);
                        }

			/* Scan for the values */
                        if (3 != sscanf(buf + 2, "%d:%d:%d",
                                &tv, &minsv, &maxsv)) return (1);

			/* Save the values */
                        e_ptr->tval[cur_t] = tv;
                        e_ptr->min_sval[cur_t] = minsv;
                        e_ptr->max_sval[cur_t] = maxsv;

                        cur_t++;

			/* Next... */
			continue;
		}

                /* Process 'R' for "flags rarity" (up to 5 lines) */
                if (buf[0] == 'R')
		{
                        int rar;

			/* Scan for the values */
                        if (1 != sscanf(buf + 2, "%d",
                                &rar)) return (1);

                        cur_r++;

			/* Save the values */
                        e_ptr->rar[cur_r] = rar;

			/* Next... */
			continue;
		}

#if 0
		/* Process 'X' for "Xtra" (one line only) */
		if (buf[0] == 'X')
		{
			int slot, rating;
                        char pos;	// actually it's boolean

			/* Scan for the values */
			if (2 != sscanf(buf + 2, "%d:%d",
			                &slot, &rating)) return (1);

			/* Save the values */
			e_ptr->slot = slot;
			e_ptr->rating = rating;

			/* Next... */
			continue;
		}
#endif	// 0

		/* Process 'X' for "Xtra" (one line only) */
		if (buf[0] == 'X')
		{
			int slot, rating;
                        char pos;

			/* Scan for the values */
                        if (3 != sscanf(buf + 2, "%c:%d:%d",
                                &pos, &slot, &rating)) return (1);

			/* Save the values */
                        /* e_ptr->slot = slot; */
			e_ptr->rating = rating;
                        e_ptr->before = (pos == 'B')?TRUE:FALSE;

			/* Next... */
			continue;
		}

		/* Process 'W' for "More Info" (one line only) */
		if (buf[0] == 'W')
		{
			int level, rarity, pad2;	// rarity2
			long cost;

			/* Scan for the values */
			if (4 != sscanf(buf + 2, "%d:%d:%d:%ld",
			                &level, &rarity, &pad2, &cost)) return (1);

			/* Save the values */
			e_ptr->level = level;
			e_ptr->rarity = rarity;
			/* e_ptr->weight = wgt; */
			e_ptr->mrarity = pad2;
			e_ptr->cost = cost;

			/* Next... */
			continue;
		}

		/* Hack -- Process 'C' for "creation" */
		if (buf[0] == 'C')
		{
			int th, td, ta, pv;

			/* Scan for the values */
			if (4 != sscanf(buf + 2, "%d:%d:%d:%d",
			                &th, &td, &ta, &pv)) return (1);

			e_ptr->max_to_h = th;
			e_ptr->max_to_d = td;
			e_ptr->max_to_a = ta;
			e_ptr->max_pval = pv;

			/* Next... */
			continue;
		}
                /* Process 'Z' for "Granted power" */
                if (buf[0] == 'Z')
		{
#if 0
                        int i;

			/* Acquire the text */
			s = buf + 2;

                        /* Find it in the list */
                        for (i = 0; i < power_max; i++)
                        {
                                if (!stricmp(s, powers_type[i].name)) break;
                        }

                        if (i == power_max) return (6);

                        e_ptr->power = i;

#endif	// 0
			/* Next... */
			continue;
		}

		/* Hack -- Process 'F' for flags */
		if (buf[0] == 'F')
		{
                        if (cur_r == -1) return (6);

			/* Parse every entry textually */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while ((*t == ' ') || (*t == '|')) t++;
				}

				/* Parse this entry */
				if (0 != grab_one_ego_item_flag(e_ptr, s, cur_r)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
	++e_head->name_size;
	++e_head->text_size;


	/* No version yet */
	if (!okay) return (2);


	/* Success */
	return (0);
}


/*
 * Grab one (basic) flag in a monster_race from a textual string
 */
static errr grab_one_basic_flag(monster_race *r_ptr, cptr what)
{
	int i;

	/* Most common flags first - mikaelh */

	/* Scan flags3 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags3[i]))
		{
			r_ptr->flags3 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags1 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags1[i]))
		{
			r_ptr->flags1 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags2 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags2[i]))
		{
			r_ptr->flags2 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags8 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags8[i]))
		{
			r_ptr->flags8 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags7 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags7[i]))
		{
			r_ptr->flags7 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags9 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags9[i]))
		{
			r_ptr->flags9 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags0 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags0[i]))
		{
			r_ptr->flags0 |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("Unknown monster flag '%s'.\n", what);

	/* Failure */
	return (1);
}


/*
 * Grab one (spell) flag in a monster_race from a textual string
 */
static errr grab_one_spell_flag(monster_race *r_ptr, cptr what)
{
	int i;

	/* Scan flags5 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags5[i]))
		{
			r_ptr->flags5 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags6 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags6[i]))
		{
			r_ptr->flags6 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags4 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags4[i]))
		{
			r_ptr->flags4 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags0 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags0[i]))
		{
			r_ptr->flags0 |= (1L << i);
			return (0);
		}
	}

	/* For Halloween Event we need new MOAN in RF8 -C. Blue */
	/* Scan flags8 */
if (season_halloween) {
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags8[i]))
		{
			r_ptr->flags8 |= (1L << i);
			return (0);
		}
	}
}

	/* Oops */
	s_printf("Unknown monster flag '%s'.\n", what);

	/* Failure */
	return (1);
}




/*
 * Initialize the "r_info" array, by parsing an ascii "template" file
 */
errr init_r_info_txt(FILE *fp, char *buf)
{
	int i, j;
	char *s, *t;
	/* Not ready yet */
	bool okay = FALSE;
	/* Current entry */
	monster_race *r_ptr = NULL;


	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Start the "fake" stuff */
	r_head->name_size = 0;
	r_head->text_size = 0;

	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE)) {
		/* Advance the line number */
		error_line++;

		/* parse server conditions */
		if (invalid_server_conditions(buf)) continue;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V') {
			int v1, v2, v3;

			/* Scan for the values */
			if ((3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) ||
			    (v1 != r_head->v_major) ||
			    (v2 != r_head->v_minor) ||
			    (v3 != r_head->v_patch)) {
				/* It only annoying -- DG */
//				return (2);
//				s_printf("Warning: different version file(%d.%d.%d)\n", v1, v2, v3);
			}

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N') {
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf + 2);

			/* Verify information */
			if (i < error_idx) return (4);

			/* Verify information */
			if (i >= (int) r_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
			r_ptr = &r_info[i];

			/* Hack -- Verify space */
			if (r_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
			if (!r_ptr->name) r_ptr->name = ++r_head->name_size;

			/* Append chars to the name */
			strcpy(r_name + r_head->name_size, s);

			/* Advance the index */
			r_head->name_size += strlen(s);

			/* Check for duplicate version of this monster that just
			   differs by FRIENDS flag, to make mimicry more consistent. - C. Blue */
			for (j = 1; j < i; j++) {
				if (strcmp(r_info[j].name + r_name, s)) continue;
				r_ptr->dup_idx = j;
				break;
			}

#if 1	// pernA hack -- someday.
			/* HACK -- Those ones HAVE to have a set default value */
			r_ptr->drops.treasure = OBJ_GENE_TREASURE;
			r_ptr->drops.combat = OBJ_GENE_COMBAT;
			r_ptr->drops.magic = OBJ_GENE_MAGIC;
			r_ptr->drops.tools = OBJ_GENE_TOOL;
			r_ptr->freq_inate = r_ptr->freq_spell = 0;
#endif	// 0

			/* Next... */
			continue;
		}

		/* There better be a current r_ptr */
		if (!r_ptr) return (3);


		/* Process 'D' for "Description" */
		if (buf[0] == 'D') {
#if 0	// I've never seen this used :)		- Jir -
			/* Acquire the text */
			s = buf + 2;

			/* Hack -- Verify space */
			if (r_head->text_size + strlen(s) + 8 > fake_text_size) return (7);

			/* Advance and Save the text index */
			if (!r_ptr->text) r_ptr->text = ++r_head->text_size;

			/* Append chars to the name */
			strcpy(r_text + r_head->text_size, s);

			/* Advance the index */
			r_head->text_size += strlen(s);
#endif

			/* Next... */
			continue;
		}

		/* Process 'G' for "Graphics" (one line only) */
		if (buf[0] == 'G') {
			char sym;
			int tmp;

			/* Paranoia */
			if (!buf[2]) return (1);
			if (!buf[3]) return (1);
			if (!buf[4]) return (1);

			/* Extract the char */
			sym = buf[2];

			/* Extract the attr */
			tmp = color_char_to_attr(buf[4]);

			/* Paranoia */
			if (tmp < 0) return (1);

			/* Save the values */
			r_ptr->d_char = sym;
			r_ptr->d_attr = tmp;

			/* Next... */
			continue;
		}

		/* Process 'I' for "Info" (one line only) */
		if (buf[0] == 'I') {
			int spd, hp1, hp2, aaf, ac, slp;

			/* Scan for the other values */
			if (6 != sscanf(buf + 2, "%d:%dd%d:%d:%d:%d",
			                &spd, &hp1, &hp2, &aaf, &ac, &slp)) return (1);

			/* Save the values */
			r_ptr->speed = spd;
			r_ptr->hdice = hp1;
			r_ptr->hside = hp2;
			r_ptr->aaf = aaf;
			r_ptr->ac = ac;
			r_ptr->sleep = slp;

			/* Next... */
			continue;
		}

		/* Process 'E' for "Body Parts" (one line only) */
		if (buf[0] == 'E') {
#if 1
                        int weap, tors, fing, head, arms, legs;

			/* Scan for the other values */
                        if (BODY_MAX != sscanf(buf + 2, "%d:%d:%d:%d:%d:%d",
                                &weap, &tors, &arms, &fing, &head, &legs)) return (1);

			/* Save the values */
                        r_ptr->body_parts[BODY_WEAPON] = weap;
                        r_ptr->body_parts[BODY_TORSO] = tors;
                        r_ptr->body_parts[BODY_ARMS] = arms;
                        r_ptr->body_parts[BODY_FINGER] = fing;
                        r_ptr->body_parts[BODY_HEAD] = head;
                        r_ptr->body_parts[BODY_LEGS] = legs;

                        /* Mega debugging hack */
                        if (weap > arms) quit(format("monster %d, %d weapon(s), %d arm(s) !", error_idx, weap, arms));

#endif	// 0
			/* Next... */
			continue;
		}

		/* Process 'O' for "Object type" (one line only) */
		if (buf[0] == 'O') {
#if 1
                        int treasure, combat, magic, tools;

			/* Scan for the values */
                        if (4 != sscanf(buf + 2, "%d:%d:%d:%d",
                                &treasure, &combat, &magic, &tools)) return (1);

			/* Save the values */
                        r_ptr->drops.treasure = treasure;
                        r_ptr->drops.combat = combat;
                        r_ptr->drops.magic = magic;
                        r_ptr->drops.tools = tools;

#endif	// 0
			/* Next... */
			continue;
		}

		/* Process 'W' for "More Info" (one line only) */
		if (buf[0] == 'W') {
			int lev, rar, pad;
			long exp;

			/* Scan for the values */
			if (4 != sscanf(buf + 2, "%d:%d:%d:%ld",
			                &lev, &rar, &pad, &exp)) return (1);

			/* Save the values */
			r_ptr->level = lev;
			r_ptr->rarity = rar;
//			r_ptr->extra = pad;
#if 1	//
			r_ptr->extra = 0;
                        /* MEGA HACK */
                        if(!pad) pad = 100;
                        r_ptr->weight = pad;
#endif
			r_ptr->mexp = exp;

			/* Next... */
			continue;
		}

		/* Process 'B' for "Blows" (up to four lines) */
		if (buf[0] == 'B') {
			int n1, n2;

			/* Find the next empty blow slot (if any) */
			for (i = 0; i < 4; i++) if (!r_ptr->blow[i].method) break;

			/* Oops, no more slots */
			if (i == 4) return (1);

			/* Analyze the first field */
			for (s = t = buf + 2; *t && (*t != ':'); t++) /* loop */;

			/* Terminate the field (if necessary) */
			if (*t == ':') *t++ = '\0';

			/* Analyze the method */
			for (n1 = 0; r_info_blow_method[n1]; n1++)
				if (streq(s, r_info_blow_method[n1])) break;

			/* Invalid method */
			if (!r_info_blow_method[n1]) return (1);

			/* Analyze the second field */
			for (s = t; *t && (*t != ':'); t++) /* loop */;

			/* Terminate the field (if necessary) */
			if (*t == ':') *t++ = '\0';

			/* Analyze effect */
			for (n2 = 0; r_info_blow_effect[n2]; n2++)
				if (streq(s, r_info_blow_effect[n2])) break;

			/* Invalid effect */
			if (!r_info_blow_effect[n2]) return (1);

			/* Analyze the third field */
			for (s = t; *t && (*t != 'd'); t++) /* loop */;

			/* Terminate the field (if necessary) */
			if (*t == 'd') *t++ = '\0';

			/* Save the method */
			r_ptr->blow[i].method = n1;

			/* Save the effect */
			r_ptr->blow[i].effect = n2;

			/* Extract the damage dice and sides */
			r_ptr->blow[i].d_dice = atoi(s);
			r_ptr->blow[i].d_side = atoi(t);

			/* Next... */
			continue;
		}

		/* Process 'F' for "Basic Flags" (multiple lines) */
		if (buf[0] == 'F') {
			/* Parse every entry */
			for (s = buf + 2; *s; ) {
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t) {
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

				/* Parse this entry */
				if (0 != grab_one_basic_flag(r_ptr, s)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

		/* Process 'S' for "Spell Flags" (multiple lines) */
		if (buf[0] == 'S') {
			/* Parse every entry */
			for (s = buf + 2; *s; ) {
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t) {
					*t++ = '\0';
					while ((*t == ' ') || (*t == '|')) t++;
				}

				/* XXX XXX XXX Hack -- Read spell frequency */
				if (1 == sscanf(s, "1_IN_%d", &i)) {
					/* Extract a "frequency" */
					r_ptr->freq_spell = r_ptr->freq_inate = 100 / i;

					/* Start at next entry */
					s = t;

					/* Continue */
					continue;
				}

				/* Parse this entry */
				if (0 != grab_one_spell_flag(r_ptr, s)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
	++r_head->name_size;
	++r_head->text_size;


	/* XXX XXX XXX XXX The ghost is unused */

	/* Mega-Hack -- acquire "ghost" */
	r_ptr = &r_info[MAX_R_IDX-1];

	/* Acquire the next index */
	r_ptr->name = r_head->name_size;
	r_ptr->text = r_head->text_size;

	/* Save some space for the ghost info */
	r_head->name_size += 64;
	r_head->text_size += 64;

	/* Hack -- Default name/text for the ghost */
	strcpy(r_name + r_ptr->name, "Nobody, the Undefined Ghost");
	strcpy(r_text + r_ptr->text, "It seems strangely familiar...");

	/* Hack -- set the char/attr info */
	r_ptr->d_attr = r_ptr->x_attr = TERM_WHITE;
	r_ptr->d_char = r_ptr->x_char = 'G';

	/* Hack -- Try to prevent a few "potential" bugs */
	r_ptr->flags1 |= (RF1_UNIQUE);

	/* Hack -- Try to prevent a few "potential" bugs */
	r_ptr->flags1 |= (RF1_NEVER_MOVE | RF1_NEVER_BLOW);

	/* Hack -- Try to prevent a few "potential" bugs */
	r_ptr->hdice = r_ptr->hside = 1;

	/* Hack -- Try to prevent a few "potential" bugs */
	r_ptr->mexp = 1L;

	for (i = 1; i < MAX_R_IDX; i++) {
		/* Invert flag WILD_ONLY <-> RF8_DUNGEON */
		r_info[i].flags8 ^= 1L;

		/* WILD_TOO without any other wilderness flags enables all flags */
#if 0
		if ((r_info[i].flags8 & RF8_WILD_TOO) && !(r_info[i].flags8 & 0x7FFFFFFE))
			r_info[i].flags8 = 0x0463;
#else	// 0
		if ((r_info[i].flags8 & RF8_WILD_TOO) &&
				!(r_info[i].flags8 & RF8_WILD_TOO_MASK))
			r_info[i].flags8 |= RF8_WILD_TOO_MASK;
#endif	// 0

		/* WILD_EASY without any other wilderness flags enables all flags */
		if ((r_info[i].flags8 & RF8_WILD_EASY) &&
				!(r_info[i].flags8 & RF8_WILD_EASY_MASK))
			r_info[i].flags8 |= RF8_WILD_EASY_MASK;

		/* Implied flags */
		if (r_info[i].flags1 & RF1_UNIQUE)
			r_info[i].flags7 |= RF7_OOD_20;

		/* clear flags that we want to be 'disabled' in defines.h for the time being,
		   for example RF6_RAISE_DEAD isn't implemented fully! - C. Blue */
		r_info[i].flags1 &= ~RF1_DISABLE_MASK;
		r_info[i].flags2 &= ~RF2_DISABLE_MASK;
		r_info[i].flags3 &= ~RF3_DISABLE_MASK;
		r_info[i].flags4 &= ~RF4_DISABLE_MASK;
		r_info[i].flags5 &= ~RF5_DISABLE_MASK;
		r_info[i].flags6 &= ~RF6_DISABLE_MASK;
		r_info[i].flags7 &= ~RF7_DISABLE_MASK;
		r_info[i].flags8 &= ~RF8_DISABLE_MASK;
		r_info[i].flags9 &= ~RF9_DISABLE_MASK;
		r_info[i].flags0 &= ~RF0_DISABLE_MASK;
	}


	/* No version yet */
	if (!okay) return (2);


	/* Success */
	return (0);
}

#ifdef RANDUNIS
/*
 * Grab one (basic) flag in a monster_race from a textual string
 */
static errr grab_one_basic_ego_flag(monster_ego *re_ptr, cptr what, bool add)
{
	int i;

	/* Scan flags1 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags1[i]))
		{
                        if (add)
                                re_ptr->mflags1 |= (1L << i);
                        else
                                re_ptr->nflags1 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags2 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags2[i]))
		{
                        if (add)
                                re_ptr->mflags2 |= (1L << i);
                        else
                                re_ptr->nflags2 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags3 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags3[i]))
		{
                        if (add)
                                re_ptr->mflags3 |= (1L << i);
                        else
                                re_ptr->nflags3 |= (1L << i);
			return (0);
		}
	}
	/* Scan flags7 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags7[i]))
		{
                        if (add)
                                re_ptr->mflags7 |= (1L << i);
                        else
                                re_ptr->nflags7 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags8 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags8[i]))
		{
                        if (add)
                                re_ptr->mflags8 |= (1L << i);
                        else
                                re_ptr->nflags8 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags9 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags9[i]))
		{
                        if (add)
                                re_ptr->mflags9 |= (1L << i);
                        else
                                re_ptr->nflags9 |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("Unknown monster flag '%s'.\n", what);

	/* Failure */
	return (1);
}


/*
 * Grab one (spell) flag in a monster_race from a textual string
 */
static errr grab_one_spell_ego_flag(monster_ego *re_ptr, cptr what, bool add)
{
	int i;

	/* Scan flags4 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags4[i]))
		{
                        if (add)
                                re_ptr->mflags4 |= (1L << i);
                        else
                                re_ptr->nflags4 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags5 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags5[i]))
		{
                        if (add)
                                re_ptr->mflags5 |= (1L << i);
                        else
                                re_ptr->nflags5 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags6 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags6[i]))
		{
                        if (add)
                                re_ptr->mflags6 |= (1L << i);
                        else
                                re_ptr->nflags6 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags0 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags0[i]))
		{
                        if (add)
                                re_ptr->mflags0 |= (1L << i);
                        else
                                re_ptr->nflags0 |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("Unknown monster flag '%s'.\n", what);

	/* Failure */
	return (1);
}


/* Values in re_info can be fixed, added, substracted or percented */
static byte monster_ego_modify(char c)
{
        switch (c)
        {
                case '+': return MEGO_ADD;
                case '-': return MEGO_SUB;
                case '=': return MEGO_FIX;
                case '%': return MEGO_PRC;
                default:
                {
                        s_printf("Unknown monster ego value modifier %c.\n", c);
                        return MEGO_ADD;
                }
        }
}

/*
 * Grab one (basic) flag in a monster_race from a textual string
 */
static errr grab_one_ego_flag(monster_ego *re_ptr, cptr what, bool must)
{
	int i;

	/* Scan flags1 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags1[i]))
		{
                        if (must) re_ptr->flags1 |= (1L << i);
                        else re_ptr->hflags1 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags2 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags2[i]))
		{
                        if (must) re_ptr->flags2 |= (1L << i);
                        else re_ptr->hflags2 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags3 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags3[i]))
		{
                        if (must) re_ptr->flags3 |= (1L << i);
                        else re_ptr->hflags3 |= (1L << i);
			return (0);
		}
	}
	/* Scan flags7 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags7[i]))
		{
                        if (must) re_ptr->flags7 |= (1L << i);
                        else re_ptr->hflags7 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags8 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags8[i]))
		{
                        if (must) re_ptr->flags8 |= (1L << i);
                        else re_ptr->hflags8 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags9 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags9[i]))
		{
                        if (must) re_ptr->flags9 |= (1L << i);
			else re_ptr->hflags9 |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("Unknown monster flag '%s'.\n", what);

	/* Failure */
	return (1);
}

/*
 * Initialize the "re_info" array, by parsing an ascii "template" file
 */
errr init_re_info_txt(FILE *fp, char *buf)
{
        int i, j;

        byte blow_num = 0;
        int r_char_number = 0, nr_char_number = 0;

	char *s, *t;

	/* Not ready yet */
	bool okay = FALSE;

	/* Current entry */
        monster_ego *re_ptr = NULL;


	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Start the "fake" stuff */
        re_head->name_size = 0;
        re_head->text_size = 0;

	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE))
	{
		/* Advance the line number */
		error_line++;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			int v1, v2, v3;

			/* Scan for the values */
			if ((3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) ||
                            (v1 != re_head->v_major) ||
                            (v2 != re_head->v_minor) ||
                            (v3 != re_head->v_patch))
			{
//				return (2);
//				s_printf("Warning: different version file(%d.%d.%d)\n", v1, v2, v3);
			}

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf + 2);

			/* Verify information */
			if (i < error_idx) return (4);

			/* Verify information */
                        if (i >= (int) re_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
                        re_ptr = &re_info[i];

			/* Hack -- Verify space */
                        if (re_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
                        if (!re_ptr->name) re_ptr->name = ++re_head->name_size;

			/* Append chars to the name */
                        strcpy(re_name + re_head->name_size, s);

			/* Advance the index */
                        re_head->name_size += strlen(s);

                        /* Some inits */
                        blow_num = 0;
                        r_char_number = 0;
                        nr_char_number = 0;
                        for (j = 0; j < 10; j++) re_ptr->r_char[j] = 0;
                        for (j = 0; j < 10; j++) re_ptr->nr_char[j] = 0;
                        for (j = 0; j < 4; j++)
                        {
                                re_ptr->blow[j].method = 0;
                                re_ptr->blow[j].effect = 0;
                                re_ptr->blow[j].d_dice = 0;
                                re_ptr->blow[j].d_side = 0;
                                re_ptr->blowm[j][0] = MEGO_ADD;
                                re_ptr->blowm[j][1] = MEGO_ADD;
                        }

			/* Next... */
			continue;
		}

                /* There better be a current re_ptr */
                if (!re_ptr) return (3);

		/* Process 'G' for "Graphics" (one line only) */
		if (buf[0] == 'G')
		{
			char sym;
			int tmp;

			/* Paranoia */
			if (!buf[2]) return (1);
			if (!buf[3]) return (1);
			if (!buf[4]) return (1);

			/* Extract the char */
                        if (buf[2] != '*') sym = buf[2];
                        else sym = MEGO_CHAR_ANY;

			/* Extract the attr */
                        if (buf[4] != '*') tmp = color_char_to_attr(buf[4]);
                        else tmp = MEGO_CHAR_ANY;

			/* Paranoia */
			if (tmp < 0) return (1);

			/* Save the values */
                        re_ptr->d_char = sym;
                        re_ptr->d_attr = tmp;

			/* Next... */
			continue;
		}

		/* Process 'I' for "Info" (one line only) */
		if (buf[0] == 'I')
		{
			int spd, hp1, hp2, aaf, ac, slp;
                        char mspd, mhp1, mhp2, maaf, mac, mslp;

			/* Scan for the other values */
                        if (12 != sscanf(buf + 2, "%c%d:%c%dd%c%d:%c%d:%c%d:%c%d",
                                &mspd, &spd, &mhp1, &hp1, &mhp2, &hp2, &maaf, &aaf, &mac, &ac, &mslp, &slp)) return (1);

			/* Save the values */
                        re_ptr->speed = (spd << 2) + monster_ego_modify(mspd);
                        re_ptr->hdice = (hp1 << 2) + monster_ego_modify(mhp1);
                        re_ptr->hside = (hp2 << 2) + monster_ego_modify(mhp2);
                        re_ptr->aaf = (aaf << 2) + monster_ego_modify(maaf);
                        re_ptr->ac = (ac << 2) + monster_ego_modify(mac);
                        re_ptr->sleep = (slp << 2) + monster_ego_modify(mslp);

			/* Next... */
			continue;
		}

		/* Process 'W' for "More Info" (one line only) */
		if (buf[0] == 'W')
		{
                        int lev, rar, wt;
                        char mlev, mwt, mexp, pos;
			long exp;

			/* Scan for the values */
                        if (8 != sscanf(buf + 2, "%c%d:%d:%c%d:%c%ld:%c",
                                &mlev, &lev, &rar, &mwt, &wt, &mexp, &exp, &pos)) return (1);

			/* Save the values */
                        re_ptr->level = (lev << 2) + monster_ego_modify(mlev);
                        re_ptr->rarity = rar;
                        re_ptr->weight = (wt << 2) + monster_ego_modify(mwt);
                        re_ptr->mexp = (exp << 2) + monster_ego_modify(mexp);
                        re_ptr->before = (pos == 'B')?TRUE:FALSE;

			/* Next... */
			continue;
		}

		/* Process 'B' for "Blows" (up to four lines) */
		if (buf[0] == 'B')
		{
                        int n1, n2, dice, side;
                        char mdice, mside;

			/* Oops, no more slots */
                        if (blow_num == 4)
						{
							s_printf("no more slots!\n");
							return (1);
						}

			/* Analyze the first field */
			for (s = t = buf + 2; *t && (*t != ':'); t++) /* loop */;

			/* Terminate the field (if necessary) */
			if (*t == ':') *t++ = '\0';

			/* Analyze the method */
			for (n1 = 0; r_info_blow_method[n1]; n1++)
			{
				if (streq(s, r_info_blow_method[n1])) break;
			}

			/* Invalid method */
			if (!r_info_blow_method[n1])
			{
				s_printf("invalid method!\n");
				return (1);
			}

			/* Analyze the second field */
			for (s = t; *t && (*t != ':'); t++) /* loop */;

			/* Terminate the field (if necessary) */
			if (*t == ':') *t++ = '\0';

			/* Analyze effect */
			for (n2 = 0; r_info_blow_effect[n2]; n2++)
			{
				if (streq(s, r_info_blow_effect[n2])) break;
			}

			/* Invalid effect */
			if (!r_info_blow_effect[n2])
			{
				s_printf("invalid effect!\n");
				return (1);
			}

			/* Save the method */
                        re_ptr->blow[blow_num].method = n1;

			/* Save the effect */
                        re_ptr->blow[blow_num].effect = n2;

			/* Extract the damage dice and sides */
                        if (4 != sscanf(t, "%c%dd%c%d",
                                &mdice, &dice, &mside, &side))
						{
							s_printf("strange dice!\n");
							return (1);
						}

                        re_ptr->blow[blow_num].d_dice = dice;
                        re_ptr->blow[blow_num].d_side = side;
                        re_ptr->blowm[blow_num][0] = monster_ego_modify(mdice);
                        re_ptr->blowm[blow_num][1] = monster_ego_modify(mside);
                        blow_num++;

			/* Next... */
			continue;
		}

                /* Process 'F' for "Flags monster must have" (multiple lines) */
                if (buf[0] == 'F')
		{
                        char r_char;

			/* Parse every entry */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

                                /* XXX XXX XXX Hack -- Read monster symbols */
                                if (1 == sscanf(s, "R_CHAR_%c", &r_char))
                                {
                                        /* Limited to 5+5 races */
                                        if(r_char_number >= 10) continue;

					/* Extract a "frequency" */
                                        re_ptr->r_char[r_char_number++] = r_char;

					/* Start at next entry */
					s = t;

					/* Continue */
					continue;
				}

				/* Parse this entry */
                                if (0 != grab_one_ego_flag(re_ptr, s, TRUE)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

                /* Process 'H' for "Flags monster must not have" (multiple lines) */
                if (buf[0] == 'H')
		{
                        char r_char;

			/* Parse every entry */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

                                /* XXX XXX XXX Hack -- Read monster symbols */
                                if (1 == sscanf(s, "R_CHAR_%c", &r_char))
                                {
                                        /* Limited to 5 races */
                                        if(nr_char_number >= 10) continue;

					/* Extract a "frequency" */
                                        re_ptr->nr_char[nr_char_number++] = r_char;

					/* Start at next entry */
					s = t;

					/* Continue */
					continue;
				}

				/* Parse this entry */
                                if (0 != grab_one_ego_flag(re_ptr, s, FALSE)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

                /* Process 'M' for "Basic Monster Flags" (multiple lines) */
                if (buf[0] == 'M')
		{
			/* Parse every entry */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

				/* Parse this entry */
                                if (0 != grab_one_basic_ego_flag(re_ptr, s, TRUE)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

                /* Process 'O' for "Basic Monster -Flags" (multiple lines) */
                if (buf[0] == 'O')
		{
			/* Parse every entry */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

                                /* XXX XXX XXX Hack -- Read no flags */
                                if (!strcmp(s, "MF_ALL"))
				{
                                        /* No flags */
                                        re_ptr->nflags1 = re_ptr->nflags2 = re_ptr->nflags3 = re_ptr->nflags7 = re_ptr->nflags8 = re_ptr->nflags9 = 0xFFFFFFFF;

					/* Start at next entry */
					s = t;

					/* Continue */
					continue;
				}

				/* Parse this entry */
                                if (0 != grab_one_basic_ego_flag(re_ptr, s, FALSE)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

                /* Process 'S' for "Spell Flags" (multiple lines) */
                if (buf[0] == 'S')
		{
			/* Parse every entry */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while ((*t == ' ') || (*t == '|')) t++;
				}

				/* XXX XXX XXX Hack -- Read spell frequency */
				if (1 == sscanf(s, "1_IN_%d", &i))
				{
					/* Extract a "frequency" */
                                        re_ptr->freq_spell = re_ptr->freq_inate = 100 / i;

					/* Start at next entry */
					s = t;

					/* Continue */
					continue;
				}

				/* Parse this entry */
                                if (0 != grab_one_spell_ego_flag(re_ptr, s, TRUE)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
                }

                /* Process 'T' for "Spell -Flags" (multiple lines) */
                if (buf[0] == 'T')
		{
			/* Parse every entry */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while ((*t == ' ') || (*t == '|')) t++;
				}

                                /* XXX XXX XXX Hack -- Read no flags */
                                if (!strcmp(s, "MF_ALL"))
				{
                                        /* No flags */
                                        re_ptr->nflags4 = re_ptr->nflags5 = re_ptr->nflags6 = 0xFFFFFFFF;

					/* Start at next entry */
					s = t;

					/* Continue */
					continue;
				}

				/* Parse this entry */
                                if (0 != grab_one_spell_ego_flag(re_ptr, s, FALSE)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
        ++re_head->name_size;

	/* No version yet */
	if (!okay) return (2);

	/* Success */
	return (0);
}

#endif	//	RANDUNIS

/*
 * Yummie traps borrowed from PernAngband	- Jir -
 */

/*
 * Grab one flag in an trap_kind from a textual string
 */
static errr grab_one_trap_type_flag(trap_kind *t_ptr, cptr what)
{
	s16b i;

	/* Check flags1 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, t_info_flags[i]))
		{
			t_ptr->flags |= (1L << i);
			return (0);
		}
	}
	/* Oops */
	s_printf("Unknown trap_type flag '%s'.\n", what);

	/* Error */
	return (1);
}


/*
 * Initialize the "tr_info" array, by parsing an ascii "template" file
 */
errr init_t_info_txt(FILE *fp, char *buf)
{
	int i;

	char *s, *t;

	/* Not ready yet */
	bool okay = FALSE;

	/* Current entry */
	trap_kind *t_ptr = NULL;


	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Prepare the "fake" stuff */
	t_head->name_size = 0;
	t_head->text_size = 0;

	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE))
	{
		/* Advance the line number */
		error_line++;

		/* parse server conditions */
		if (invalid_server_conditions(buf)) continue;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			int v1, v2, v3;

			/* Scan for the values */
			if ((3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) ||
			    (v1 != t_head->v_major) ||
			    (v2 != t_head->v_minor) ||
			    (v3 != t_head->v_patch))
			{
//				return (2);
//				s_printf("Warning: different version file(%d.%d.%d)\n", v1, v2, v3);
			}

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf + 2);

			/* Verify information */
			if (i <= error_idx) return (4);

			/* Verify information */
			if (i >= (int) t_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
			t_ptr = &t_info[i];

			/* Hack -- Verify space */
			if (t_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
			if (!t_ptr->name) t_ptr->name = ++t_head->name_size;

			/* Append chars to the name */
			strcpy(t_name + t_head->name_size, s);

			/* Advance the index */
			t_head->name_size += strlen(s);

			/* Next... */
			continue;
		}

		/* There better be a current t_ptr */
		if (!t_ptr) return (3);


		/* Process 'I' for "Information" */
		if (buf[0] == 'I')
		{
			int probability, another, p1valinc, difficulty;
			int minlevel;
			int dd, ds, vanish;
			char color;

			/* Scan for the values */
			if (9 != sscanf(buf + 2, "%d:%d:%d:%d:%d:%dd%d:%c:%d",
					&difficulty, &probability, &another,
					&p1valinc, &minlevel, &dd, &ds,
					&color, &vanish)) return (1);

			t_ptr->difficulty  = (byte)difficulty;
			t_ptr->probability = (s16b)probability;
			t_ptr->another     = (s16b)another;
			t_ptr->p1valinc     = (s16b)p1valinc;
			t_ptr->minlevel    = (byte)minlevel;
			t_ptr->dd          = (s16b)dd;
			t_ptr->ds          = (s16b)ds;
			t_ptr->color       = color_char_to_attr(color);
			t_ptr->vanish      = (byte)vanish;

			/* Megahack -- move pernA-oriented instakill traps deeper */
			if (19 < error_idx && error_idx < 134)
				t_ptr->minlevel += 5;

			/* Next... */
			continue;
		}


		/* Process 'D' for "Description" */
		if (buf[0] == 'D')
		{
			/* Acquire the text */
			s = buf + 2;

			/* Hack -- Verify space */
			if (t_head->text_size + strlen(s) + 8 > fake_text_size) return (7);

			/* Advance and Save the text index */
			if (!t_ptr->text) t_ptr->text = ++t_head->text_size;

			/* Append chars to the name */
			strcpy(t_text + t_head->text_size, s);

			/* Advance the index */
			t_head->text_size += strlen(s);

			/* Next... */
			continue;
		}


		/* Hack -- Process 'F' for flags */
		if (buf[0] == 'F')
		{

                        t_ptr->flags = 0;

			/* Parse every entry textually */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

				/* Parse this entry */
                                if (0 != grab_one_trap_type_flag(t_ptr, s)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}


		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
	++t_head->name_size;
	++t_head->text_size;


	/* No version yet */
	if (!okay) return (2);


	/* Success */
	return (0);
}


/*
 * Grab one flag for a dungeon type from a textual string
 */
static errr grab_one_dungeon_flag(dungeon_info_type *d_ptr, cptr what)
{
	int i;

	/* Scan flags1 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, d_info_flags1[i]))
		{
			d_ptr->flags1 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags2 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, d_info_flags2[i]))
		{
			d_ptr->flags2 |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("Unknown dungeon type flag '%s'.\n", what);

	/* Failure */
	return (1);
}

/*
 * Grab one (basic) flag in a monster_race from a textual string
 */
static errr grab_one_basic_monster_flag(dungeon_info_type *d_ptr, cptr what, byte rule)
{
	int i;

	/* Scan flags1 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags1[i]))
		{
			d_ptr->rules[rule].mflags1 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags2 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags2[i]))
		{
			d_ptr->rules[rule].mflags2 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags3 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags3[i]))
		{
			d_ptr->rules[rule].mflags3 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags7 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags7[i]))
		{
			d_ptr->rules[rule].mflags7 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags8 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags8[i]))
		{
			d_ptr->rules[rule].mflags8 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags9 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags9[i]))
		{
			d_ptr->rules[rule].mflags9 |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("Unknown monster flag '%s'.\n", what);

	/* Failure */
	return (1);
}


/*
 * Grab one (spell) flag in a monster_race from a textual string
 */
static errr grab_one_spell_monster_flag(dungeon_info_type *d_ptr, cptr what, byte rule)
{
	int i;

	/* Scan flags4 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags4[i]))
		{
			d_ptr->rules[rule].mflags4 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags5 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags5[i]))
		{
			d_ptr->rules[rule].mflags5 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags6 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags6[i]))
		{
			d_ptr->rules[rule].mflags6 |= (1L << i);
			return (0);
		}
	}

	/* Scan flags0 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags0[i]))
		{
			d_ptr->rules[rule].mflags0 |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("Unknown monster flag '%s'.\n", what);

	/* Failure */
	return (1);
}

/*
 * Initialize the "d_info" array, by parsing an ascii "template" file
 */
errr init_d_info_txt(FILE *fp, char *buf)
{
	int i, j;

	byte rule_num = 0;

	byte r_char_number = 0;

	char *s, *t;

	/* Not ready yet */
	bool okay = FALSE;

	/* Current entry */
	dungeon_info_type *d_ptr = NULL;

	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Start the "fake" stuff */
	d_head->name_size = 0;
	d_head->text_size = 0;

	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE))
	{
		/* Advance the line number */
		error_line++;

		/* parse server conditions */
		if (invalid_server_conditions(buf)) continue;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			int v1, v2, v3;

#ifdef VERIFY_VERSION_STAMP

			/* Scan for the values */
			if ((3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) ||
			    (v1 != d_head->v_major) ||
			    (v2 != d_head->v_minor) ||
			    (v3 != d_head->v_patch))
			{
				return (2);
			}

#else /* VERIFY_VERSION_STAMP */

			/* Scan for the values */
			if (3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) return (2);

#endif /* VERIFY_VERSION_STAMP */

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf + 2);

			/* Verify information */
			if (i < error_idx) return (4);

			/* Verify information */
			if (i >= (int) d_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
			d_ptr = &d_info[i];

			/* New (for fountains of blood) - remember own index */
//			d_ptr->idx = error_idx;

			/* Hack -- Verify space */
			if (d_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
			if (!d_ptr->name) d_ptr->name = ++d_head->name_size;

			/* Append chars to the name */
			strcpy(d_name + d_head->name_size, s);

			/* Advance the index */
			d_head->name_size += strlen(s);

			/* HACK -- Those ones HAVE to have a set default value */
			d_ptr->ix = -1;
			d_ptr->iy = -1;
			d_ptr->ox = -1;
			d_ptr->oy = -1;
			d_ptr->fill_method = 1;
			rule_num = -1;
			r_char_number = 0;
			for (j = 0; j < 5; j++)
			{
				int k;

				d_ptr->rules[j].mode = DUNGEON_MODE_NONE;
				d_ptr->rules[j].percent = 0;

				for (k = 0; k < 5; k++) d_ptr->rules[j].r_char[k] = 0;
			}

			/* HACK -- Those ones HAVE to have a set default value */
			d_ptr->objs.treasure = OBJ_GENE_TREASURE;
			d_ptr->objs.combat = OBJ_GENE_COMBAT;
			d_ptr->objs.magic = OBJ_GENE_MAGIC;
			d_ptr->objs.tools = OBJ_GENE_TOOL;

			/* Next... */
			continue;
		}

		/* There better be a current d_ptr */
		if (!d_ptr) return (3);

		/* Process 'D' for "Description */
		if (buf[0] == 'D')
		{
			/* Acquire short name */
			d_ptr->short_name[0] = buf[2];
			d_ptr->short_name[1] = buf[3];
			d_ptr->short_name[2] = buf[4];

			/* Acquire the text */
			s = buf + 6;

			/* Hack -- Verify space */
			if (d_head->text_size + strlen(s) + 8 > fake_text_size) return (7);

			/* Advance and Save the text index */
			if (!d_ptr->text) d_ptr->text = ++d_head->text_size;

			/* Append chars to the name */
			strcpy(d_text + d_head->text_size, s);

			/* Advance the index */
			d_head->text_size += strlen(s);

			/* Next... */
			continue;
		}

		/* Process 'W' for "More Info" (one line only) */
		if (buf[0] == 'W')
		{
			int min_lev, max_lev;
			int min_plev, next;
			int min_alloc, max_chance;

			/* Scan for the values */
			if (6 != sscanf(buf + 2, "%d:%d:%d:%d:%d:%d",
				&min_lev, &max_lev, &min_plev, &next, &min_alloc, &max_chance)) return (1);

			/* Save the values */
			d_ptr->mindepth = min_lev;
			d_ptr->maxdepth = max_lev;
			d_ptr->min_plev = min_plev;
			d_ptr->next = next;
			d_ptr->min_m_alloc_level = min_alloc;
			d_ptr->max_m_alloc_chance = max_chance;

			/* Next... */
			continue;
		}

		/* Process 'L' for "fLoor type" (one line only) */
		if (buf[0] == 'L')
		{
			int f[5], p[5];
			int i, j;

			/* Scan for the values */
			if (10 != sscanf(buf + 2, "%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
				&f[0], &p[0], &f[1], &p[1], &f[2], &p[2], &f[3], &p[3], &f[4], &p[4])) {
				/* Scan for the values - part ii*/
				if (5 != sscanf(buf + 2, "%d:%d:%d:%d:%d", &p[0], &p[1],
						&p[2], &p[3], &p[4])) return (1);

				/* Save the values */
				for (i = 0; i < 5; i++) d_ptr->floor_percent[i][1] = p[i];

				continue;
			}

			/* Save the values */
			for (i = 0; i < 5; i++) {
				d_ptr->floor[i] = f[i];
				for (j = 0; j <= 1; j++) d_ptr->floor_percent[i][j] = p[i];
			}

			/* Next... */
			continue;
		}

		/* Process 'O' for "Object type" (one line only) */
		if (buf[0] == 'O')
		{
			int treasure, combat, magic, tools;

			/* Scan for the values */
			if (4 != sscanf(buf + 2, "%d:%d:%d:%d",
				&treasure, &combat, &magic, &tools)) return (1);

			/* Save the values */
			d_ptr->objs.treasure = treasure;
			d_ptr->objs.combat = combat;
			d_ptr->objs.magic = magic;
			d_ptr->objs.tools = tools;

			/* Next... */
			continue;
		}

		/* Process 'A' for "wAll type" (one line only) */
		if (buf[0] == 'A')
		{
			int outer, inner;
			int w[5], p[5];
			int i, j;

			/* Scan for the values */
			if (12 != sscanf(buf + 2, "%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
				&w[0], &p[0], &w[1], &p[1], &w[2], &p[2], &w[3], &p[3], &w[4], &p[4], &outer, &inner))
			{
				/* Scan for the values - part ii*/
				if (5 != sscanf(buf + 2, "%d:%d:%d:%d:%d", &p[0], &p[1],
				    &p[2], &p[3], &p[4])) return (1);

				/* Save the values */
				for (i = 0; i < 5; i++) d_ptr->fill_percent[i][1] = p[i];

				continue;
			}

			/* Save the values */
			for (i = 0; i < 5; i++) {
				d_ptr->fill_type[i] = w[i];
				for (j = 0; j <= 1; j++) d_ptr->fill_percent[i][j] = p[i];
			}

			d_ptr->outer_wall = outer;
			d_ptr->inner_wall = inner;

			/* Next... */
			continue;
		}

		/* Process 'E' for "Effects" (up to four lines) -SC- */
		if (buf[0] == 'E')
		{
			int side, dice, freq, type;
			cptr tmp;

			/* Find the next empty blow slot (if any) */
			for (i = 0; i < 4; i++) if ((!d_ptr->d_side[i]) &&
						    (!d_ptr->d_dice[i])) break;

			/* Oops, no more slots */
			if (i == 4) return (1);

			/* Scan for the values */
			  if (4 != sscanf(buf + 2, "%dd%d:%d:%d",
				  &dice, &side, &freq, &type))
			{
				int j;

				if (3 != sscanf(buf + 2, "%dd%d:%d",
						&dice, &side, &freq)) return (1);

				tmp = buf + 2;
				for (j = 0; j < 2; j++)
				{
					tmp = strchr(tmp, ':');
					if (tmp == NULL) return(1);
					tmp++;
				}

				j = 0;

				while (d_info_dtypes[j].name != NULL)
					if (strcmp(d_info_dtypes[j].name, tmp) == 0)
					{
						d_ptr->d_type[i] = d_info_dtypes[j].feat;
						break;
					}
					else j++;

				if (d_info_dtypes[j].name == NULL) return(1);
			}
			else
				d_ptr->d_type[i] = type;

			freq *= 10;
			/* Save the values */
			d_ptr->d_side[i] = side;
			d_ptr->d_dice[i] = dice;
			d_ptr->d_frequency[i] = freq;

			/* Next... */
			continue;
		}

		/* Process 'F' for "Dungeon Flags" (multiple lines) */
		if (buf[0] == 'F')
		{
			int artif = 0, monst = 0, obj = 0;
			int ix = -1, iy = -1, ox = -1, oy = -1;
			int fill_method;

			/* Parse every entry */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

				/* XXX XXX XXX Hack -- Read dungeon in/out coords */
				if (4 == sscanf(s, "WILD_%d_%d__%d_%d", &ix, &iy, &ox, &oy))
				{
					d_ptr->ix = ix;
					d_ptr->iy = iy;
					d_ptr->ox = ox;
					d_ptr->oy = oy;

					/* Start at next entry */
					s = t;

					/* Continue */
					continue;
				}

				/* XXX XXX XXX Hack -- Read dungeon fill method */
				if (1 == sscanf(s, "FILL_METHOD_%d", &fill_method))
				{
					d_ptr->fill_method = fill_method;

					/* Start at next entry */
					s = t;

					/* Continue */
					continue;
				}

				/* XXX XXX XXX Hack -- Read Final Object */
				if (1 == sscanf(s, "FINAL_OBJECT_%d", &obj))
				{
					/* Extract a "Final Artifact" */
					d_ptr->final_object = obj;

					/* Start at next entry */
					s = t;

					/* Continue */
					continue;
				}

				/* XXX XXX XXX Hack -- Read Final Artifact */
				if (1 == sscanf(s, "FINAL_ARTIFACT_%d", &artif))
				{
					/* Extract a "Final Artifact" */
					d_ptr->final_artifact = artif;

					/* Start at next entry */
					s = t;

					/* Continue */
					continue;
				}

				/* XXX XXX XXX Hack -- Read Artifact Guardian */
				if (1 == sscanf(s, "FINAL_GUARDIAN_%d", &monst))
				{
					/* Extract a "Artifact Guardian" */
					d_ptr->final_guardian = monst;

					/* Start at next entry */
					s = t;

					/* Continue */
					continue;
				}

				/* Parse this entry */
				if (0 != grab_one_dungeon_flag(d_ptr, s)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

		/* Process 'R' for "monster generation Rule" (up to 5 lines) */
		if (buf[0] == 'R')
		{
			int percent, mode;
			int z, y, lims[5];

			/* Scan for the values */
			if (2 != sscanf(buf + 2, "%d:%d",
				&percent, &mode)) return (1);

			/* Save the values */
			r_char_number = 0;
			rule_num++;

			d_ptr->rules[rule_num].percent = percent;
			d_ptr->rules[rule_num].mode = mode;

			/* Lets remap the flat percents */
			lims[0] = d_ptr->rules[0].percent;
			for (y = 1; y <= rule_num; y++)
			{
				lims[y] = lims[y - 1] + d_ptr->rules[y].percent;
			}
			for (z = 0; z < 100; z++)
			{
				for (y = rule_num; y >= 0; y--)
				{
					if (z < lims[y]) d_ptr->rule_percents[z] = y;
				}
			}

			/* Next... */
			continue;
		}

		/* Process 'M' for "Basic Flags" (multiple lines) */
		if (buf[0] == 'M')
		{
			byte r_char;

			/* Parse every entry */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

				/* XXX XXX XXX Hack -- Read monster symbols */
				if (1 == sscanf(s, "R_CHAR_%c", &r_char))
				{
					/* Limited to 5 races */
					if(r_char_number >= 5) continue;

					/* Extract a "frequency" */
					d_ptr->rules[rule_num].r_char[r_char_number++] = r_char;

					/* Start at next entry */
					s = t;

					/* Continue */
					continue;
				}

				/* Parse this entry */
				if (0 != grab_one_basic_monster_flag(d_ptr, s, rule_num)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

		/* Process 'S' for "Spell Flags" (multiple lines) */
		if (buf[0] == 'S')
		{
			/* Parse every entry */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while ((*t == ' ') || (*t == '|')) t++;
				}

				/* Parse this entry */
				if (0 != grab_one_spell_monster_flag(d_ptr, s, rule_num)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
	++d_head->name_size;
	++d_head->text_size;

	/* No version yet */
	if (!okay) return (2);

	/* Hack -- acquire total number */
	max_d_idx = ++error_idx;

#if DEBUG_LEVEL > 2
	/* Debug -- print total no. */
	s_printf("d_info total: %d\n", max_d_idx);
#endif	// DEBUG_LEVEL

	/* Success */
	return (0);
}



/*
 * Grab one race flag from a textual string
 */
static errr grab_one_race_flag(owner_type *ow_ptr, int state, cptr what)
{
	/* int i;
	cptr s; */

	/* Scan race flags */
	unknown_shut_up = TRUE;
	if (!grab_one_race_allow_flag(ow_ptr->races[state], what))
	{
		unknown_shut_up = FALSE;
		return (0);
	}

	/* Scan classes flags */
	if (!grab_one_class_flag(ow_ptr->classes[state], what))
	{
		unknown_shut_up = FALSE;
		return (0);
	}

	/* Scan realms flags */
	if (!grab_one_player_realm_flag(ow_ptr->realms[state], what))
	{
		unknown_shut_up = FALSE;
		return (0);
	}

	/* Oops */
	unknown_shut_up = FALSE;
	s_printf("Unknown race/class/realm flag '%s'.\n", what);

	/* Failure */
	return (1);
}

/*
 * Grab one store flag from a textual string
 */
static errr grab_one_store_flag(store_info_type *st_ptr, cptr what)
{
	int i;

	/* Scan store flags */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, st_info_flags1[i]))
		{
			st_ptr->flags1 |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("Unknown store flag '%s'.\n", what);

	/* Failure */
	return (1);
}

/*
 * Initialize the "st_info" array, by parsing an ascii "template" file
 */
errr init_st_info_txt(FILE *fp, char *buf)
{
	int i = 0, item_idx = 0/*, cnt = 0*/;

	char *s, *t;

	/* Not ready yet */
	bool okay = FALSE;

	/* Current entry */
	store_info_type *st_ptr = NULL;


	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Start the "fake" stuff */
	st_head->name_size = 0;
	st_head->text_size = 0;

	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE))
	{
		/* Advance the line number */
		error_line++;

		/* parse server conditions */
		if (invalid_server_conditions(buf)) continue;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			int v1, v2, v3;

#ifdef VERIFY_VERSION_STAMP

			/* Scan for the values */
			if ((3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) ||
			    (v1 != st_head->v_major) ||
			    (v2 != st_head->v_minor) ||
			    (v3 != st_head->v_patch))
			{
				return (2);
			}

#else /* VERIFY_VERSION_STAMP */

			/* Scan for the values */
			if (3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) return (2);

#endif /* VERIFY_VERSION_STAMP */

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf + 2);

//			++cnt;

			/* Verify information */
			if (i < error_idx) return (4);

			/* Verify information */
			if (i >= (int) st_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
			st_ptr = &st_info[i];

			/* Hack -- Verify space */
			if (st_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
			if (!st_ptr->name) st_ptr->name = ++st_head->name_size;

			/* Append chars to the name */
			strcpy(st_name + st_head->name_size, s);

			/* Advance the index */
			st_head->name_size += strlen(s);

			/* We are ready for a new set of objects */
			item_idx = 0;

			/* Next... */
			continue;
		}

		/* There better be a current st_ptr */
		if (!st_ptr) return (3);

		/* Process 'I' for "Items" (multiple lines) */
		if (buf[0] == 'I')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			st_ptr->table[item_idx][1] = atoi(buf + 2);

			/* Append chars to the name */
			st_ptr->table[item_idx++][0] = test_item_name(s);

			st_ptr->table_num = item_idx;

			/* Next... */
			continue;
		}

		/* Process 'T' for "Tval/sval" */
		if (buf[0] == 'T')
		{
			int tv1, sv1, rar1;

			/* Scan for the values */
			if (3 != sscanf(buf + 2, "%d:%d:%d",
				&rar1, &tv1, &sv1)) return (1);

			/* Get the index */
			st_ptr->table[item_idx][1] = rar1;
			/* Hack -- 256 as a sval means all possible items */
			st_ptr->table[item_idx++][0] = (sv1 < 256)?lookup_kind(tv1, sv1):tv1 + 10000;

			st_ptr->table_num = item_idx;

			/* Next... */
			continue;
		}

		/* Process 'G' for "Graphics" one line only) */
		if (buf[0] == 'G')
		{
			char c, a;
			int attr;

			/* Scan for the values */
			if (2 != sscanf(buf + 2, "%c:%c",
				&c, &a)) return (1);

			/* Extract the color */
			attr = color_char_to_attr(a);

			/* Paranoia */
			if (attr < 0) return (1);

			/* Save the values */
			st_ptr->d_char = c;
			st_ptr->d_attr = attr;

			/* Next... */
			continue;
		}

		/* Process 'A' for "Actions" (one line only) */
		if (buf[0] == 'A')
		{
			int a1, a2, a3, a4, a5, a6;

			/* Scan for the values */
			if (6 != sscanf(buf + 2, "%d:%d:%d:%d:%d:%d",
				&a1, &a2, &a3, &a4, &a5, &a6)) return (1);

			/* Save the values */
			st_ptr->actions[0] = a1;
			st_ptr->actions[1] = a2;
			st_ptr->actions[2] = a3;
			st_ptr->actions[3] = a4;
			st_ptr->actions[4] = a5;
			st_ptr->actions[5] = a6;

			/* Next... */
			continue;
		}

		/* Process 'F' for "store Flags" (multiple lines) */
		if (buf[0] == 'F')
		{
			/* Parse every entry */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

				/* Parse this entry */
				if (0 != grab_one_store_flag(st_ptr, s)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

		/* Process 'O' for "Owners" (one line only) */
		if (buf[0] == 'O')
		{
			int a1, a2, a3, a4, a5, a6;

			/* Scan for the values */
			if (MAX_STORE_OWNERS != sscanf(buf + 2, "%d:%d:%d:%d:%d:%d",
				&a1, &a2, &a3, &a4, &a5, &a6)) return (1);

			/* Save the values */
			st_ptr->owners[0] = a1;
			st_ptr->owners[1] = a2;
			st_ptr->owners[2] = a3;
			st_ptr->owners[3] = a4;
			st_ptr->owners[4] = a5;
			st_ptr->owners[5] = a6; /* MAX_STORE_OWNERS */

			/* Next... */
			continue;
		}

		/* Process 'W' for "Extra info" (one line only) */
		if (buf[0] == 'W')
		{
			int max_obj;

			/* Scan for the values */
			if (1 != sscanf(buf + 2, "%d",
				&max_obj)) return (1);

			/* Save the values */
			if (max_obj > STORE_INVEN_MAX) max_obj = STORE_INVEN_MAX;
			st_ptr->max_obj = max_obj;

			/* Next... */
			continue;
		}

		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
	++st_head->name_size;
	++st_head->text_size;

	/* No version yet */
	if (!okay) return (2);

	/* Hack -- acquire total number */
	max_st_idx = ++error_idx;

#if DEBUG_LEVEL > 2
	/* Debug -- print total no. */
	s_printf("st_info total: %d\n", max_st_idx);
#endif	// DEBUG_LEVEL

	/* Success */
	return (0);
}

/*
 * Initialize the "ba_info" array, by parsing an ascii "template" file
 */
errr init_ba_info_txt(FILE *fp, char *buf)
{
	int i = 0;

	char *s;

	/* Not ready yet */
	bool okay = FALSE;

	/* Current entry */
	store_action_type *ba_ptr = NULL;


	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Start the "fake" stuff */
	ba_head->name_size = 0;
	ba_head->text_size = 0;

	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE))
	{
		/* Advance the line number */
		error_line++;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			int v1, v2, v3;

#ifdef VERIFY_VERSION_STAMP

			/* Scan for the values */
			if ((3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) ||
			    (v1 != ba_head->v_major) ||
			    (v2 != ba_head->v_minor) ||
			    (v3 != ba_head->v_patch))
			{
				return (2);
			}

#else /* VERIFY_VERSION_STAMP */

			/* Scan for the values */
			if (3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) return (2);

#endif /* VERIFY_VERSION_STAMP */

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf + 2);

			/* Verify information */
			if (i < error_idx) return (4);

			/* Verify information */
			if (i >= (int) ba_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
			ba_ptr = &ba_info[i];

			/* Hack -- Verify space */
			if (ba_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
			if (!ba_ptr->name) ba_ptr->name = ++ba_head->name_size;

			/* Append chars to the name */
			strcpy(ba_name + ba_head->name_size, s);

			/* Advance the index */
			ba_head->name_size += strlen(s);

			/* Next... */
			continue;
		}

		/* There better be a current ba_ptr */
		if (!ba_ptr) return (3);

		/* Process 'C' for "Costs" (one line only) */
		if (buf[0] == 'C')
		{
			int ch, cn, cl;

			/* Scan for the values */
			if (3 != sscanf(buf + 2, "%d:%d:%d",
				&ch, &cn, &cl)) return (1);

			/* Save the values */
			ba_ptr->costs[STORE_HATED] = ch;
			ba_ptr->costs[STORE_NORMAL] = cn;
			ba_ptr->costs[STORE_LIKED] = cl;

			/* Next... */
			continue;
		}

		/* Process 'I' for "Infos" (one line only) */
		if (buf[0] == 'I')
		{
			int act, act_res;
			char letter;
			unsigned int flags;

			/* Scan for the values */
			if (4 != sscanf(buf + 2, "%d:%d:%c:%u",
				&act, &act_res, &letter, &flags)) return (1);

			/* Save the values */
			ba_ptr->action = act;
			ba_ptr->action_restr = act_res;
			ba_ptr->letter = letter;
			ba_ptr->flags = (byte)flags;

			/* Next... */
			continue;
		}

		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
	++ba_head->name_size;
	++ba_head->text_size;

	/* No version yet */
	if (!okay) return (2);

	/* Success */
	return (0);
}

/*
 * Initialize the "ow_info" array, by parsing an ascii "template" file
 */
errr init_ow_info_txt(FILE *fp, char *buf)
{
	int i;

	char *s, *t;

	/* Not ready yet */
	bool okay = FALSE;

	/* Current entry */
	owner_type *ow_ptr = NULL;


	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;


	/* Start the "fake" stuff */
	ow_head->name_size = 0;
	ow_head->text_size = 0;

	/* Parse */
	while (0 == my_fgets(fp, buf, 1024, FALSE))
	{
		/* Advance the line number */
		error_line++;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (1);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			int v1, v2, v3;

#ifdef VERIFY_VERSION_STAMP

			/* Scan for the values */
			if ((3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) ||
			    (v1 != ow_head->v_major) ||
			    (v2 != ow_head->v_minor) ||
			    (v3 != ow_head->v_patch))
			{
				return (2);
			}

#else /* VERIFY_VERSION_STAMP */

			/* Scan for the values */
			if (3 != sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3)) return (2);

#endif /* VERIFY_VERSION_STAMP */

			/* Okay to proceed */
			okay = TRUE;

			/* Continue */
			continue;
		}

		/* No version yet */
		if (!okay) return (2);


		/* Process 'N' for "New/Number/Name" */
		if (buf[0] == 'N')
		{
			/* Find the colon before the name */
			s = strchr(buf + 2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf + 2);

			/* Verify information */
			if (i < error_idx) return (4);

			/* Verify information */
			if (i >= (int) ow_head->info_num) return (2);

			/* Save the index */
			error_idx = i;

			/* Point at the "info" */
			ow_ptr = &ow_info[i];

			/* Hack -- Verify space */
			if (ow_head->name_size + strlen(s) + 8 > fake_name_size) return (7);

			/* Advance and Save the name index */
			if (!ow_ptr->name) ow_ptr->name = ++ow_head->name_size;

			/* Append chars to the name */
			strcpy(ow_name + ow_head->name_size, s);

			/* Advance the index */
			ow_head->name_size += strlen(s);

			/* Next... */
			continue;
		}

		/* There better be a current ow_ptr */
		if (!ow_ptr) return (3);


		/* Process 'C' for "Costs" (one line only) */
		if (buf[0] == 'C')
		{
			int ch, cn, cl;

			/* Scan for the values */
			if (3 != sscanf(buf + 2, "%d:%d:%d",
				&ch, &cn, &cl)) return (1);

			/* Save the values */
			ow_ptr->costs[STORE_HATED] = ch;
			ow_ptr->costs[STORE_NORMAL] = cn;
			ow_ptr->costs[STORE_LIKED] = cl;

			/* Next... */
			continue;
		}

		/* Process 'I' for "Info" (multiple lines line only) */
		if (buf[0] == 'I')
		{
			int cost, max_inf, min_inf, haggle, insult;

			/* Scan for the values */
			if (5 != sscanf(buf + 2, "%d:%d:%d:%d:%d",
				&cost, &max_inf, &min_inf, &haggle, &insult)) return (1);

			/* Save the values */
//			ow_ptr->max_cost = cost;
			ow_ptr->max_cost = cost * STORE_PURSE_BOOST;
			ow_ptr->max_inflate = max_inf;
			ow_ptr->min_inflate = min_inf;
			ow_ptr->haggle_per = haggle;
			ow_ptr->insult_max = insult;

			/* Next... */
			continue;
		}

		/* Process 'L' for "Liked races/classes/realms" (multiple lines) */
		if (buf[0] == 'L')
		{
			/* Parse every entry */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

				/* Parse this entry */
				if (0 != grab_one_race_flag(ow_ptr, STORE_LIKED, s)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}
		/* Process 'H' for "Hated races/classes/realms" (multiple lines) */
		if (buf[0] == 'H')
		{
			/* Parse every entry */
			for (s = buf + 2; *s; )
			{
				/* Find the end of this entry */
				for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

				/* Nuke and skip any dividers */
				if (*t)
				{
					*t++ = '\0';
					while (*t == ' ' || *t == '|') t++;
				}

				/* Parse this entry */
				if (0 != grab_one_race_flag(ow_ptr, STORE_HATED, s)) return (5);

				/* Start the next entry */
				s = t;
			}

			/* Next... */
			continue;
		}

		/* Oops */
		return (6);
	}


	/* Complete the "name" and "text" sizes */
	++ow_head->name_size;
	++ow_head->text_size;

	/* No version yet */
	if (!okay) return (2);

	/* Success */
	return (0);
}



#else	/* ALLOW_TEMPLATES */

#ifdef MACINTOSH
static int i = 0;
#endif

#endif	/* ALLOW_TEMPLATES */


/*
 * Another big lump of ToME parts, for loading maps/quests
 * Most part is undone yet, but at least it can read town definition files.
 * - Jir -
 */

/* Random dungeon grid effects */
#define RANDOM_NONE         0x00
#define RANDOM_FEATURE      0x01
#define RANDOM_MONSTER      0x02
#define RANDOM_OBJECT       0x04
#define RANDOM_EGO          0x08
#define RANDOM_ARTIFACT     0x10
#define RANDOM_TRAP         0x20


typedef struct dungeon_grid dungeon_grid;

struct dungeon_grid
{
	int		feature;		/* Terrain feature */
	int		monster;		/* Monster */
	int		object;			/* Object */
	int		ego;			/* Ego-Item */
	int		artifact;		/* Artifact */
	int		trap;			/* Trap */
	int		cave_info;		/* Flags for CAVE_MARK, CAVE_GLOW, CAVE_ICKY, CAVE_ROOM */
	int		special;		/* Reserved for special terrain info */
	int		random;			/* Number of the random effect */
	int             bx, by;                 /* For between gates */
	int             mimic;                  /* Mimiced features */
	bool            ok;
	bool            defined;
};
static bool meta_sleep = TRUE;

static dungeon_grid letter[255];

/*
 * Parse a sub-file of the "extra info"
 */
bool process_dungeon_file_full = FALSE;
//static errr process_dungeon_file_aux(char *buf, int *yval, int *xval, int xvalstart, int ymax, int xmax)
static errr process_dungeon_file_aux(char *buf, worldpos *wpos, int *yval, int *xval, int xvalstart, int ymax, int xmax)
{
	int i;
	char *zz[33]; /* was 33 */

	int dun_level = getlevel(wpos);
	c_special *cs_ptr;
	cave_type **zcave;
	zcave=getcave(wpos);

	if (!zcave) return (-1);	/* maybe SIGSEGV soon anyway */


	/* Skip "empty" lines */
	if (!buf[0]) return (0);

	/* Skip "blank" lines */
	if (isspace(buf[0])) return (0);

	/* Skip comments */
	if (buf[0] == '#') return (0);

	/* Require "?:*" format */
	if (buf[1] != ':') return (1);


	/* Process "%:<fname>" */
	if (buf[0] == '%')
	{
		/* Attempt to Process the given file */
		return (process_dungeon_file(buf + 2, wpos, yval, xval, ymax, xmax, FALSE));
	}

	/* Process "N:<sleep>" */
	if (buf[0] == 'N')
	{
		int num;

		if ((num = tokenize(buf + 2, 1, zz, ':', '/')) > 0)
		{
			meta_sleep = atoi(zz[0]);
		}

		return (0);
	}

	/* Process "F:<letter>:<terrain>:<cave_info>:<monster>:<object>:<ego>:<artifact>:<trap>:<special>:<mimic>" -- info for dungeon grid */
	if (buf[0] == 'F')
	{
		int num;

		if ((num = tokenize(buf + 2, 10, zz, ':', '/')) > 1)
		{
			int index = zz[0][0];

			/* Reset the feature */
			letter[index].feature = 0;
			letter[index].monster = 0;
			letter[index].object = 0;
			letter[index].ego = 0;
			letter[index].artifact = 0;
			letter[index].trap = 0;
			letter[index].cave_info = 0;
			letter[index].special = 0;
			letter[index].random = 0;
			letter[index].mimic = 0;
			letter[index].ok = TRUE;
			letter[index].defined = TRUE;

			if (num > 1)
			{
				if (zz[1][0] == '*')
				{
					letter[index].random |= RANDOM_FEATURE;
					if (zz[1][1])
					{
						zz[1]++;
						letter[index].feature = atoi(zz[1]);
					}
				}
				else
				{
					letter[index].feature = atoi(zz[1]);
				}
			}

			if (num > 2)
				letter[index].cave_info = atoi(zz[2]);

			/* Monster */
			if (num > 3)
			{
				if (zz[3][0] == '*')
				{
					letter[index].random |= RANDOM_MONSTER;
					if (zz[3][1])
					{
						zz[3]++;
						letter[index].monster = atoi(zz[3]);
					}
				}
				else
				{
					letter[index].monster = atoi(zz[3]);
				}
			}

			/* Object */
			if (num > 4)
			{
				if (zz[4][0] == '*')
				{
					letter[index].random |= RANDOM_OBJECT;

					if (zz[4][1])
					{
						zz[4]++;
						letter[index].object = atoi(zz[4]);
					}
				}
				else
				{
					letter[index].object = atoi(zz[4]);
				}
			}

			/* Ego-Item */
			if (num > 5)
			{
				if (zz[5][0] == '*')
				{
					letter[index].random |= RANDOM_EGO;

					if (zz[5][1])
					{
						zz[5]++;
						letter[index].ego = atoi(zz[5]);
					}
				}
				else
				{
					letter[index].ego = atoi(zz[5]);
				}
			}

			/* Artifact */
			if (num > 6)
			{
				if (zz[6][0] == '*')
				{
					letter[index].random |= RANDOM_ARTIFACT;

					if (zz[6][1])
					{
						zz[6]++;
						letter[index].artifact = atoi(zz[6]);
					}
				}
				else
				{
					letter[index].artifact = atoi(zz[6]);
				}
			}

			if (num > 7)
			{
				if (zz[7][0] == '*')
				{
					letter[index].random |= RANDOM_TRAP;

					if (zz[7][1])
					{
						zz[7]++;
						letter[index].trap = atoi(zz[7]);
					}
				}
				else
					letter[index].trap = atoi(zz[7]);
			}

#if 0
			if (num > 8)
			{
				/* Quests can be defined by name only */
				if (zz[8][0] == '"')
				{
					int i;

					/* Hunt & shoot the ending " */
					i = strlen(zz[8]) - 1;
					if (zz[8][i] == '"') zz[8][i] = '\0';
					letter[index].special = 0;
					for (i = 0; i < max_q_idx; i++)
					{
						if (!strcmp(&zz[8][1], quest[i].name))
						{
							letter[index].special = i;
							break;
						}
					}
				}
				else
					letter[index].special = atoi(zz[8]);
			}
#else	// 0
			if (num > 8)
			{
				/* Quests can be defined by name only */
				if (zz[8][0] == '"')
				{
#if 0	// later for quest
					int i;

					/* Hunt & shoot the ending " */
					i = strlen(zz[8]) - 1;
					if (zz[8][i] == '"') zz[8][i] = '\0';
					letter[index].special = 0;
					for (i = 0; i < max_q_idx; i++)
					{
						if (!strcmp(&zz[8][1], quest[i].name))
						{
							letter[index].special = i;
							break;
						}
					}
#endif	// 0
				}
				else
					letter[index].special = atoi(zz[8]);
			}
#endif	// 0

			if (num > 9)
			{
				letter[index].mimic = atoi(zz[9]);
			}

			return (0);
		}
	}

	/* Process "D:<dungeon>" -- info for the cave grids */
	else if (buf[0] == 'D')
	{
		int x;

//		object_type object_type_body;

		/* Acquire the text */
		char *s = buf + 2;

		/* Length of the text */
		int len = strlen(s);

		int y = *yval;
		*xval = xvalstart;
		for (x = *xval, i = 0; ((x < xmax) && (i < len)); x++, s++, i++)
		{
			/* Access the grid */
			cave_type *c_ptr = &zcave[y][x];

			int idx = s[0];

			int object_index = letter[idx].object;
#if 0
			int monster_index = letter[idx].monster;
#else /* rudimentary support till actual code (see further below what i mean) has been looked at, too lazy atm - C. Blue */
			int monster_index = letter[idx].monster;
#endif // 0
			int random = letter[idx].random;
			int artifact_index = letter[idx].artifact;

			if (!letter[idx].ok) s_printf("Warning '%c' not defined but used.\n", idx);

//			if (init_flags & INIT_GET_SIZE) continue;

			/* use the plasma generator wilderness */
//			if (((!dun_level) || (!letter[idx].defined)) && (idx == ' ')) continue;
			if (((!wpos->wz) || (!letter[idx].defined)) && (idx == ' ')) continue;

			/* Clear some info */
			c_ptr->info = 0;

			/* Lay down a floor */
//			c_ptr->mimic = letter[idx].mimic;

			/* seasons hack: replace trees/bushes on world surface according to season! - C. Blue */
			if ((letter[idx].feature == FEAT_TREE || letter[idx].feature == FEAT_BUSH) &&
			    !wpos->wz) {
				cave_set_feat(wpos, y, x, get_seasonal_tree());
//				c_ptr->feat = get_seasonal_tree();
			} else {
				cave_set_feat(wpos, y, x, letter[idx].feature);
//				c_ptr->feat = letter[idx].feature;
			}

			/* TERAHACK -- memorize stair locations XXX XXX */
			if (c_ptr->feat == FEAT_LESS)	// '<'
			{
				new_level_down_y(wpos, y);
				new_level_down_x(wpos, x);
			} 
			else if (c_ptr->feat == FEAT_MORE)
			{
				new_level_up_y(wpos, y);
				new_level_up_x(wpos, x);
			}

			/* Cave info */
			c_ptr->info |= letter[idx].cave_info;

			/* Create a monster */
			if (random & RANDOM_MONSTER)
			{
				int level = monster_level;

//				monster_level = quest[p_ptr->inside_quest].level + monster_index;
				monster_level = getlevel(wpos) + monster_level;

				place_monster(wpos, y, x, meta_sleep, FALSE);

				monster_level = level;
			}
#if 0
			else if (monster_index)
			{
				/* Place it */
				m_allow_special[monster_index] = TRUE;
				place_monster_aux(y, x, monster_index, meta_sleep, FALSE, MSTATUS_ENEMY, 0);
				m_allow_special[monster_index] = FALSE;
			}
#else /* rudimentary support till above code has been looked at, too lazy atm - C. Blue */
			else if (monster_index)
			{
				summon_override_checks = SO_ALL; /* disable all checks */
				place_monster_aux(wpos, y, x, monster_index, FALSE, FALSE, 0, 0);
//				place_monster_one(wpos, y, x, monster_index, 0, 0, FALSE, 0, 0);
				summon_override_checks = SO_NONE; /* re-enable default */
			}
#endif	// 0

			/* Object (and possible trap) */
			if ((random & RANDOM_OBJECT) && (random & RANDOM_TRAP))
			{
				int level = object_level;

//				object_level = quest[p_ptr->inside_quest].level;
				object_level = dun_level;

				/*
				 * Random trap and random treasure defined
				 * 25% chance for trap and 75% chance for object
				 */
				if (rand_int(100) < 75)
				{
					place_object(wpos, y, x, FALSE, FALSE, FALSE, RESF_NONE, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
				}
				// else
				if (rand_int(100) < 25)
				{
					place_trap(wpos, y, x, 0);
				}

				object_level = level;
			}
			else if (random & RANDOM_OBJECT)
			{
				/* Create an out of deep object */
				if (object_index)
				{
					int level = object_level;

//					object_level = quest[p_ptr->inside_quest].level + object_index;
					object_level = getlevel(wpos) + object_index;
					if (rand_int(100) < 75)
						place_object(wpos, y, x, FALSE, FALSE, FALSE, RESF_NONE, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
					else if (rand_int(100) < 80)
						place_object(wpos, y, x, TRUE, FALSE, FALSE, RESF_NONE, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
					else
						place_object(wpos, y, x, TRUE, TRUE, FALSE, RESF_NONE, default_obj_theme, 0, ITEM_REMOVAL_NEVER);

					object_level = level;
				}
				else if (rand_int(100) < 75)
				{
					place_object(wpos, y, x, FALSE, FALSE, FALSE, RESF_NONE, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
				}
				else if (rand_int(100) < 80)
				{
					place_object(wpos, y, x, TRUE, FALSE, FALSE, RESF_NONE, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
				}
				else
				{
					place_object(wpos, y, x, TRUE, TRUE, FALSE, RESF_NONE, default_obj_theme, 0, ITEM_REMOVAL_NEVER);
				}
			}
			/* Random trap */
			else if (random & RANDOM_TRAP)
			{
				place_trap(wpos, y, x, 0);
			}
#if 0
			else if (object_index)
			{
				/* Get local object */
				object_type *o_ptr = &object_type_body;

				k_allow_special[object_index] = TRUE;

				/* Create the item */
				object_prep(o_ptr, object_index);

				/* Apply magic (no messages, no artifacts) */
				apply_magic(wpos, o_ptr, dun_level, FALSE, TRUE, FALSE, FALSE, TRUE);

				k_allow_special[object_index] = FALSE;

				drop_near(o_ptr, -1, wpos, y, x);
			}
#endif	// 0

			/* Artifact */
			if (artifact_index)
			{
#if 0
				int I_kind = 0;

				artifact_type *a_ptr = &a_info[artifact_index];

				object_type forge;

				/* Get local object */
				object_type *q_ptr = &forge;

				a_allow_special[artifact_index] = TRUE;

				/* Wipe the object */
				object_wipe(q_ptr);

				/* Acquire the "kind" index */
				I_kind = lookup_kind(a_ptr->tval, a_ptr->sval);

				/* Create the artifact */
				object_prep(q_ptr, I_kind);

				/* Save the name */
				q_ptr->name1 = artifact_index;

				/* Extract the fields */
				q_ptr->pval = a_ptr->pval;
				q_ptr->ac = a_ptr->ac;
				q_ptr->dd = a_ptr->dd;
				q_ptr->ds = a_ptr->ds;
				q_ptr->to_a = a_ptr->to_a;
				q_ptr->to_h = a_ptr->to_h;
				q_ptr->to_d = a_ptr->to_d;
				q_ptr->weight = a_ptr->weight;

				random_artifact_resistance(q_ptr);

				handle_art_inum(artifact_index);

				a_allow_special[artifact_index] = FALSE;

				/* Drop the artifact */
				drop_near(q_ptr, -1, y, x);
#endif	// 0
			}

#if 0
			/* Terrain special */
			if (letter[idx].special == -1)
			{
				if (!letter[idx].bx)
				{
					letter[idx].bx = x;
					letter[idx].by = y;
				}
				else
				{
					c_ptr->special = (letter[idx].by << 8) + letter[idx].bx;
					cave[letter[idx].by][letter[idx].bx].special = (y << 8) + x;
				}
			}
			else
			{
				c_ptr->special = letter[idx].special;
			}
#else	// 0
			/* Terrain special */
			if (letter[idx].special == -1)
			{
#if 0
				if (!letter[idx].bx)
				{
					letter[idx].bx = x;
					letter[idx].by = y;
				}
				else
				{
					c_ptr->special = (letter[idx].by << 8) + letter[idx].bx;
					cave[letter[idx].by][letter[idx].bx].special = (y << 8) + x;
				}
#endif	// 0
			}
			else
			{
				/* MEGAHACK -- let's just make stores available */
				if (letter[idx].feature == FEAT_SHOP)
				{
					if((cs_ptr=AddCS(c_ptr, CS_SHOP)))
					{
						/* MEGAHACK till st_info is implemented */
						// int y1, x1;
						int store = letter[idx].special;
						// if (store > 8) store = 8;

						cs_ptr->sc.omni = store;

#if 0	// not here
						for (y1 = y - 1; y1 <= y + 1; y1++)
						{
							for (x1 = x - 1; x1 <= x + 1; x1++)
							{
								/* Get the grid */
								c_ptr = &zcave[y1][x1];

								/* Illuminate the store */
								c_ptr->info |= CAVE_ROOM | CAVE_GLOW;
							}
						}
#endif	// 0
					}
				}
//				c_ptr->special = letter[idx].special;
			}
#endif	// 0
		}
		if ((process_dungeon_file_full) && (*xval < x)) *xval = x;
		(*yval)++;

		return (0);
	}

	/* Process "W:<command>: ..." -- info for the wilderness */
	else if (buf[0] == 'W')
	{
		return (0);
#if 0
		/* Process "W:D:<layout> */
		/* Layout of the wilderness */
		if (buf[2] == 'D')
		{
			int x;
			char i;

			/* Acquire the text */
			char *s = buf+4;

			int y = *yval;

			for(x = 0; x < max_wild_x; x++)
			{
				if (1 != sscanf(s + x, "%c", &i)) return (1);
				wild_map[y][x].feat = wildc2i[(int)i];
			}

			(*yval)++;

			return (0);
		}
		/* Process "M:<plus>:<line>" -- move line lines */
		else if (buf[2] == 'M')
		{
			if (tokenize(buf+4, 2, zz, ':', '/') == 2)
			{
				if (atoi(zz[0]))
				{
					(*yval) += atoi(zz[1]);
				}
				else
				{
					(*yval) -= atoi(zz[1]);
				}
			}
			else
			{
				return (1);
			}
			return (0);
		}
		/* Process "W:P:<x>:<y> - starting position in the wilderness */
		else if (buf[2] == 'P')
		{
			if ((p_ptr->wilderness_x == 0) &&
			    (p_ptr->wilderness_y == 0))
			{
				if (tokenize(buf+4, 2, zz, ':', '/') == 2)
				{
					p_ptr->wilderness_x = atoi(zz[0]);
					p_ptr->wilderness_y = atoi(zz[1]);
				}
				else
				{
					return (1);
				}
			}

			return (0);
		}
		/* Process "W:E:<dungeon>:<y>:<x> - entrance to the dungeon <dungeon> */
		else if (buf[2] == 'E')
		{
			if (tokenize(buf+4, 3, zz, ':', '/') == 3)
			{
				wild_map[atoi(zz[1])][atoi(zz[2])].entrance = 1000 + atoi(zz[0]);
			}
			else
			{
				return (1);
			}

			return (0);
		}
#endif	// 0
	}

	/* Process "P:<x>:<y>" -- player position */
	else if (buf[0] == 'P')
	{
#if 0	// It'll be needed very soon maybe
		if (init_flags & INIT_CREATE_DUNGEON)
		{
			if (tokenize(buf + 2, 2, zz, ':', '/') == 2)
			{
				/* Place player in a quest level */
				if (p_ptr->inside_quest || (init_flags & INIT_POSITION))
				{
					py = atoi(zz[0]);
					px = atoi(zz[1]); 
				}
				/* Place player in the town */
				else if ((p_ptr->oldpx == 0) && (p_ptr->oldpy == 0))
				{
					p_ptr->oldpy = atoi(zz[0]);
					p_ptr->oldpx = atoi(zz[1]); 
				}
			}
		}
#else	// 0.. quick Hack
		if (tokenize(buf + 2, 2, zz, ':', '/') == 2)
		{
			int yy = atoi(zz[0]);
			int xx = atoi(zz[1]);
			new_level_rand_y(wpos, yy);
			new_level_rand_x(wpos, xx);
#if 0
			new_level_down_y(wpos, yy);
			new_level_down_x(wpos, xx);
			new_level_up_y(wpos, yy);
			new_level_up_x(wpos, xx);
#endif	// 0
		}


#endif	// 0
		return (0);
	}

	/* Process "M:<type>:<maximum>" -- set maximum values */
	else if (buf[0] == 'M')
	{
		return (0);

#if	0	// It's very nice code - this should be transmitted to the client, tho
		if (tokenize(buf + 2, 3, zz, ':', '/') >= 2)
		{
			/* Maximum towns */
			if (zz[0][0] == 'T')
			{
				max_towns = atoi(zz[1]); 
			}

			/* Maximum real towns */
			if (zz[0][0] == 't')
			{
				max_real_towns = atoi(zz[1]); 
			}

			/* Maximum r_idx */
			else if (zz[0][0] == 'R')
			{
				max_r_idx = atoi(zz[1]); 
			}

			/* Maximum re_idx */
			else if (zz[0][0] == 'r')
			{
				max_re_idx = atoi(zz[1]); 
			}

			/* Maximum s_idx */
			else if (zz[0][0] == 'k')
			{
				max_s_idx = atoi(zz[1]);
				if (max_s_idx > MAX_SKILLS) return (1);
			}

			/* Maximum k_idx */
			else if (zz[0][0] == 'K')
			{
				max_k_idx = atoi(zz[1]); 
			}

			/* Maximum v_idx */
			else if (zz[0][0] == 'V')
			{
				max_v_idx = atoi(zz[1]); 
			}

			/* Maximum f_idx */
			else if (zz[0][0] == 'F')
			{
				max_f_idx = atoi(zz[1]); 
			}

			/* Maximum a_idx */
			else if (zz[0][0] == 'A')
			{
				max_a_idx = atoi(zz[1]); 
			}

			/* Maximum e_idx */
			else if (zz[0][0] == 'E')
			{
				max_e_idx = atoi(zz[1]); 
			}

			/* Maximum ra_idx */
			else if (zz[0][0] == 'Z')
			{
				max_ra_idx = atoi(zz[1]); 
			}

			/* Maximum o_idx */
			else if (zz[0][0] == 'O')
			{
				max_o_idx = atoi(zz[1]); 
			}

			/* Maximum player types */
			else if (zz[0][0] == 'P')
			{
				if (zz[1][0] == 'R')
				{
					max_rp_idx = atoi(zz[2]); 
				}
				else if (zz[1][0] == 'S')
				{
					max_rmp_idx = atoi(zz[2]); 
				}
				else if (zz[1][0] == 'C')
				{
					max_c_idx = atoi(zz[2]);
				}
				else if (zz[1][0] == 'M')
				{
					max_mc_idx = atoi(zz[2]);
				}
				else if (zz[1][0] == 'H')
				{
					max_bg_idx = atoi(zz[2]); 
				}
			}

			/* Maximum m_idx */
			else if (zz[0][0] == 'M')
			{
				max_m_idx = atoi(zz[1]); 
			}

			/* Maximum tr_idx */
			else if (zz[0][0] == 'U')
			{
				max_t_idx = atoi(zz[1]); 
			}

			/* Maximum wf_idx */
			else if (zz[0][0] == 'W')
			{
				max_wf_idx = atoi(zz[1]); 
			}

			/* Maximum ba_idx */
			else if (zz[0][0] == 'B')
			{
				max_ba_idx = atoi(zz[1]); 
			}

			/* Maximum st_idx */
			else if (zz[0][0] == 'S')
			{
				max_st_idx = atoi(zz[1]); 
			}

			/* Maximum set_idx */
			else if (zz[0][0] == 's')
			{
				max_set_idx = atoi(zz[1]);
			}

			/* Maximum ow_idx */
			else if (zz[0][0] == 'N')
			{
				max_ow_idx = atoi(zz[1]); 
			}

			/* Maximum wilderness x size */
			else if (zz[0][0] == 'X')
			{
				max_wild_x = atoi(zz[1]); 
			}

			/* Maximum wilderness y size */
			else if (zz[0][0] == 'Y')
			{
				max_wild_y = atoi(zz[1]); 
			}

			/* Maximum d_idx */
			else if (zz[0][0] == 'D')
			{
				max_d_idx = atoi(zz[1]); 
			}

			return (0);
		}
#endif	// 0
	}

	/* Failure */
	return (1);
}




/*
 * Helper function for "process_dungeon_file()"
 */
#if 0
static cptr process_dungeon_file_expr(char **sp, char *fp)
{
	cptr v;

	char *b;
	char *s;

	char b1 = '[';
	char b2 = ']';

	char f = ' ';

	/* Initial */
	s = (*sp);

	/* Skip spaces */
	while (isspace(*s)) s++;

	/* Save start */
	b = s;

	/* Default */
	v = "?o?o?";

	/* Analyze */
	if (*s == b1)
	{
		const char *p;
		const char *t;

		/* Skip b1 */
		s++;

		/* First */
		t = process_dungeon_file_expr(&s, &f);

		/* Oops */
		if (!*t)
		{
			/* Nothing */
		}

		/* Function: IOR */
		else if (streq(t, "IOR"))
		{
			v = "0";
			while (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
				if (*t && !streq(t, "0")) v = "1";
			}
		}

		/* Function: AND */
		else if (streq(t, "AND"))
		{
			v = "1";
			while (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
				if (*t && streq(t, "0")) v = "0";
			}
		}

		/* Function: NOT */
		else if (streq(t, "NOT"))
		{
			v = "1";
			while (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
				if (*t && streq(t, "1")) v = "0";
			}
		}

		/* Function: EQU */
		else if (streq(t, "EQU"))
		{
			v = "1";
			if (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
			}
			while (*s && (f != b2))
			{
				p = t;
				t = process_dungeon_file_expr(&s, &f);
				if (*t && !streq(p, t)) v = "0";
			}
		}

		/* Function: LEQ */
		else if (streq(t, "LEQ"))
		{
			v = "1";
			if (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
			}
			while (*s && (f != b2))
			{
				p = t;
				t = process_dungeon_file_expr(&s, &f);
				if (*t && (strcmp(p, t) > 0)) v = "0";
			}
		}

		/* Function: GEQ */
		else if (streq(t, "GEQ"))
		{
			v = "1";
			if (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
			}
			while (*s && (f != b2))
			{
				p = t;
				t = process_dungeon_file_expr(&s, &f);
				if (*t && (strcmp(p, t) < 0)) v = "0";
			}
		}

		/* Oops */
		else
		{
			while (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
			}
		}

		/* Verify ending */
		if (f != b2) v = "?x?x?";

		/* Extract final and Terminate */
		if ((f = *s) != '\0') *s++ = '\0';
	}

	/* Other */
	else
        {
                bool text_mode = FALSE;

		/* Accept all printables except spaces and brackets */
                while (isprint(*s))
                {
                        if (*s == '"') text_mode = !text_mode;
                        if (!text_mode)
                        {
                                if (strchr(" []", *s))
                                        break;
                        }
                        else
                        {
                                if (strchr("[]", *s))
                                        break;
                        }

                        ++s;
                }

		/* Extract final and Terminate */
		if ((f = *s) != '\0') *s++ = '\0';

		/* Variable */
		if (*b == '$')
		{
			/* System */
			if (streq(b+1, "SYS"))
			{
				v = ANGBAND_SYS;
			}

			/* Graphics */
			else if (streq(b+1, "GRAF"))
			{
				v = ANGBAND_GRAF;
			}

			/* Race */
			else if (streq(b+1, "RACE"))
			{
				v = rp_ptr->title + rp_name;
			}

			/* Race Mod */
			else if (streq(b+1, "RACEMOD"))
			{
				v = rmp_ptr->title + rmp_name;
			}

			/* Class */
			else if (streq(b+1, "CLASS"))
			{
				v = cp_ptr->title + c_name;
			}

			/* Player */
			else if (streq(b+1, "PLAYER"))
			{
				v = player_base;
			}

			/* Town */
			else if (streq(b+1, "TOWN"))
			{
				strnfmt(pref_tmp_value, 8, "%d", p_ptr->town_num);
				v = pref_tmp_value;
			}

			/* Town destroyed */
			else if (prefix(b+1, "TOWN_DESTROY"))
                        {                               
				strnfmt(pref_tmp_value, 8, "%d", town_info[atoi(b + 13)].destroyed);
				v = pref_tmp_value;
			}

			/* Current quest number */
			else if (streq(b+1, "QUEST_NUMBER"))
			{
				strnfmt(pref_tmp_value, 8, "%d", p_ptr->inside_quest);
				v = pref_tmp_value;
			}

			/* Number of last quest */
			else if (streq(b+1, "LEAVING_QUEST"))
			{
				strnfmt(pref_tmp_value, 8, "%d", leaving_quest);
				v = pref_tmp_value;
			}

			/* DAYTIME status */
			else if (prefix(b+1, "DAYTIME"))
			{
				if ((bst(HOUR, turn) >= SUNRISE) && (bst(HOUR, turn) < NIGHTFALL))
					v = "1";
				else
					v = "0";
			}

			/* Quest status */
			else if (prefix(b+1, "QUEST"))
			{
				/* "QUEST" uses a special parameter to determine the number of the quest */
				if (*(b + 6) != '"')
					strnfmt(pref_tmp_value, 8, "%d", quest[atoi(b+6)].status);
				else
				{
					char c[80];
					int i;

					/* Copy it to temp array, so that we can modify it safely */
					strcpy(c, b + 7);

                                        /* Hunt & shoot the ending " */
					for (i = 0; (c[i] != '"') && (c[i] != '\0'); i++);
					if (c[i] == '"') c[i] = '\0';
					strcpy(pref_tmp_value, "-1");
					for (i = 0; i < max_q_idx; i++)
					{
                                                if (streq(c, quest[i].name))
                                                {
							strnfmt(pref_tmp_value, 8, "%d", quest[i].status);
							break;
						}
                                        }
				}
				v = pref_tmp_value;
			}

			/* Variant name */
			else if (streq(b+1, "VARIANT"))
			{
				v = "ToME";
			}

			/* Wilderness */
			else if (streq(b+1, "WILDERNESS"))
			{
				if (vanilla_town) v = "NONE";
				else v = "NORMAL";
			}
		}

		/* Constant */
		else
		{
			v = b;
		}
	}

	/* Save */
	(*fp) = f;

	/* Save */
	(*sp) = s;

	/* Result */
	return (v);
}
#endif	// 0


//errr process_dungeon_file(cptr name, int *yval, int *xval, int ymax, int xmax, bool init)
errr process_dungeon_file(cptr name, worldpos *wpos, int *yval, int *xval, int ymax, int xmax, bool init)
{
	FILE *fp;
	char buf[1024];
	int num = -1, i;
	errr err = 0;
	bool bypass = FALSE;

	/* Save the start since it ought to be modified */
	int xmin = *xval;

	cave_type **zcave;
	zcave = getcave(wpos);
	if (!zcave) return (-1);	/* maybe SIGSEGV soon anyway */

	if (init) {
		meta_sleep = TRUE;
		for (i = 0; i < 255; i++) {
			letter[i].defined = FALSE;
			if (i == ' ') letter[i].ok = TRUE;
			else letter[i].ok = FALSE;
			letter[i].bx = 0;
			letter[i].by = 0;
		}
	}

	/* Build the filename */
//	path_build(buf, 1024, ANGBAND_DIR_EDIT, name);
	path_build(buf, 1024, ANGBAND_DIR_GAME, name);

	/* Grab permission */
//	safe_setuid_grab();

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Drop permission */
//	safe_setuid_drop();

	/* No such file */
	if (!fp) {
		s_printf("Cannot find file %s at %s\n", name, buf);
		return (-1);
	}

	/* Process the file */
	while (0 == my_fgets(fp, buf, 1024, FALSE)) {
		/* Count lines */
		num++;


		/* Skip "empty" lines */
		if (!buf[0]) continue;

		/* Skip "blank" lines */
		if (isspace(buf[0])) continue;

		/* Skip comments */
		if (buf[0] == '#') continue;


		/* Process "?:<expr>" */
		if ((buf[0] == '?') && (buf[1] == ':'))
		{
#if 0	// later
			char f;
			cptr v;
			char *s;

			/* Start */
			s = buf + 2;

			/* Parse the expr */
			v = process_dungeon_file_expr(&s, &f);

			/* Set flag */
			bypass = (streq(v, "0") ? TRUE : FALSE);
#endif // 0

			/* Continue */
			continue;
		}

		/* Apply conditionals */
		if (bypass) continue;


		/* Process "%:<file>" */
		if (buf[0] == '%')
		{
			/* Process that file if allowed */
			(void)process_dungeon_file(buf + 2, wpos, yval, xval, ymax, xmax, FALSE);

			/* Continue */
			continue;
		}


		/* Process the line */
		err = process_dungeon_file_aux(buf, wpos, yval, xval, xmin, ymax, xmax);

		/* Oops */
		if (err) break;
	}


	/* Error */
	if (err)
	{
		/* Useful error message */
		s_printf("Error %d in line %d of file '%s'.\n", err, num, name);
		s_printf("Parsing '%s'\n", buf);
	}

	/* Close the file */
	my_fclose(fp);

	/* update player maps */
	for (i = 1; i <= NumPlayers; i++) {
		/* Only for players on the level */
		if (inarea(wpos, &Players[i]->wpos)) Players[i]->redraw |= PR_MAP;
	}

	/* Result */
	return (err);
}
