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
	"XXX2X4",	//	"MULTIPLY",
	"S_ANIMAL",	// "XXX3X4",
	"ROCKET",	// "XXX4X4",
	"ARROW_1",	// former 1/2 (arrow)
	"ARROW_2",	// former 3/4 (missile)
	"XXX",		//	"ARROW_3",
	"XXX",		//	"ARROW_4",
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
	"BA_NUKE",	// "XXX5X4",
	"BR_NUKE",	// "XXX6X4",
	"BA_CHAO",	// "XXX7X4",
	"BR_DISI"	// "XXX8X4"
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
	"XXX",		//	"CAUSE_2",
	"XXX",		//	"CAUSE_3",
	"UNMAGIC",		//	"CAUSE_4",
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
	"ANIM_DEAD",	// "XXX3X6",
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

#if 0	// flags6
	"ANIM_DEAD", /* ToDo: Implement ANIM_DEAD */
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
	"XXX7X15",
	"XXX7X16",
	"XXX7X17",
	"XXX7X18",
	"XXX7X19",
	"XXX7X20",
	"XXX7X21",
	"XXX7X22",
	"XXX7X23",
	"XXX7X24",
	"XXX7X25",
	"XXX7X26",
	"XXX7X27",
	"XXX7X28",
	"XXX7X29",
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
	"XXX8X02",
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
	"XXX8X28",
	"XXX8X29",
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
        "IMPRESED",
        "SUSCEP_ACID",
        "SUSCEP_ELEC",
        "SUSCEP_POIS",
        "KILL_TREES",
        "WYRM_PROTECT",
        "DOPPLEGANGER",
        "ONLY_DEPTH",
        "SPECIAL_GENE",
        "NEVER_GENE",
	"XXX9X15",
	"XXX9X16",
	"XXX9X17",
	"XXX9X18",
	"XXX9X19",
	"XXX9X20",
	"XXX9X21",
	"XXX9X22",
	"XXX9X23",
	"XXX9X24",
	"XXX9X25",
	"XXX9X26",
	"XXX9X27",
	"XXX9X28",
	"XXX9X29",
	"IM_TELE",
	"IM_PSI",
	"RES_PSI",
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
   "XXX5",
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
   "XXX31",
   "XXX32"
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
	"VORPAL",	//"XXX5",		
	"IMPACT",
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

#if 0	// flags3
	"KNOWLEDGE",
	"XXX2",
	"XXX3",
	"XXX4",
	"XXX5",
	"XXX6",
	"XXX7",
	"XXX8",
#endif	// 0
#if 0
/*
 * Trap flags
 */
cptr k_info_flags2_trap[] =
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
cptr k_info_flags4[] =
{
        "NEVER_BLOW",
        "PRECOGNITION",
        "BLACK_BREATH",
        "RECHARGE",
        "FLY",
        "DG_CURSE",
        "COULD2H",
        "MUST2H",
        "LEVELS",
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
cptr k_info_flags5[] =
{
        "TEMPORARY",
        "DRAIN_MANA",
        "DRAIN_HP",
        "KILL_DEMON",
        "KILL_UNDEAD",
        "CRIT",
        "ATTR_MULTI",
        "WOUNDING",
        "FULL_NAME",
        "LUCK",
        "IMMOVABLE",
	"XXX8X22",
	"XXX8X22",
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
	"XXX8X25",
//	"LIFE",		//"XXX8X27",
	"CHAOTIC",	//"XXX8X26",
	"INVIS",	//"XXX8X28",
	"SENS_FIRE",	//"XXX8X25",
	"REFLECT",	//"XXX8X29",
	"XXX8X22",	// "NORM_ART"
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
	"XXX8X28",
	"XXX8X29",
	"XXX8X02",
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
        "AC_M1",
        "AC_M2",
        "AC_M3",
        "AC_M5",
        "TH_M1",
        "TH_M2",
        "TH_M3",
        "TH_M5",
        "R_ESP",	// "TD_M1",
        "TD_M2",
        "TD_M3",
        "TD_M5",
        "R_P_ABILITY",
        "R_STAT",
        "R_STAT_SUST",
        "R_IMMUNITY",
        "LIMIT_BLOWS"
};

#endif	// 0


/*** Initialize from ascii template files ***/


/*
 * Initialize the "v_info" array, by parsing an ascii "template" file
 */
errr init_v_info_txt(FILE *fp, char *buf)
{
	int i;

	char *s;

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
	while (0 == my_fgets(fp, buf, 1024))
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
			s = strchr(buf+2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf+2);

			/* Verify information */
			if (i <= error_idx) return (4);

			/* Verify information */
			if (i >= v_head->info_num) return (2);

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
			s = buf+2;

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
			if (4 != sscanf(buf+2, "%d:%d:%d:%d",
			                &typ, &rat, &hgt, &wid)) return (1);

			/* Save the values */
			v_ptr->typ = typ;
			v_ptr->rat = rat;
			v_ptr->hgt = hgt;
			v_ptr->wid = wid;

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


#if 1
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
	while (0 == my_fgets(fp, buf, 1024))
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
			if ((3 != sscanf(buf+2, "%d.%d.%d", &v1, &v2, &v3)) ||
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
			s = strchr(buf+2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf+2);

			/* Verify information */
			if (i <= error_idx) return (4);

			/* Verify information */
			if (i >= f_head->info_num) return (2);

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
			s = buf+2;

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
			if (1 != sscanf(buf+2, "%d",
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
	s_printf("Unknown object flag '%s'.", what);

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
	while (0 == my_fgets(fp, buf, 1024))
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
			if ((3 != sscanf(buf+2, "%d.%d.%d", &v1, &v2, &v3)) ||
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
			s = strchr(buf+2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf+2);

			/* Verify information */
			if (i <= error_idx) return (4);

			/* Verify information */
			if (i >= f_head->info_num) return (2);

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
			if (1 != sscanf(buf+2, "%d",
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
			if (7 != sscanf(buf+2, "%c:%c:%c:%c:%c:%c:%c",
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
			f_ptr->d_attr = tmp;
			f_ptr->d_char = buf[2];

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
			if (4 != sscanf(buf+2, "%dd%d:%d:%d",
						&dice, &side, &freq, &type))
			{
				int j;

				if (3 != sscanf(buf+2, "%dd%d:%d",
							&dice, &side, &freq)) return (1);

							tmp = buf+2;
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

			freq *= 10;
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

#if 0
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
	s_printf("Unknown object flag '%s'.", what);

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
	while (0 == my_fgets(fp, buf, 1024))
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
			if ((3 != sscanf(buf+2, "%d.%d.%d", &v1, &v2, &v3)) ||
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
			s = strchr(buf+2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
//			i = atoi(buf+2);

			/* Count it up */
			i = ++idx;

			/* Verify information */
			if (i <= error_idx) return (4);

			/* Verify information */
			if (i >= k_head->info_num) return (2);

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
			s = buf+2;

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
			if (3 != sscanf(buf+2, "%d:%d:%d",
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
			if (4 != sscanf(buf+2, "%d:%d:%d:%ld",
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
			if (6 != sscanf(buf+2, "%d:%dd%d:%d:%d:%d",
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

	/* Debug -- print total no. */
	s_printf("k_info total: %d\n", idx);

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
#if 0

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
	s_printf("Unknown artifact flag '%s'.", what);

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
	while (0 == my_fgets(fp, buf, 1024))
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
			if ((3 != sscanf(buf+2, "%d.%d.%d", &v1, &v2, &v3)) ||
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
			s = strchr(buf+2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf+2);

			/* Verify information */
			if (i < error_idx) return (4);

			/* Verify information */
			if (i >= a_head->info_num) return (2);

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

			/* Next... */
			continue;
		}

		/* There better be a current a_ptr */
		if (!a_ptr) return (3);



		/* Process 'D' for "Description" */
		if (buf[0] == 'D')
		{
#if 0	// Hope they'll be handled in client-side someday
			/* Acquire the text */
			s = buf+2;

			/* Hack -- Verify space */
			if (a_head->text_size + strlen(s) + 8 > fake_text_size) return (7);

			/* Advance and Save the text index */
			if (!a_ptr->text) a_ptr->text = ++a_head->text_size;

			/* Append chars to the name */
			strcpy(a_text + a_head->text_size, s);

			/* Advance the index */
			a_head->text_size += strlen(s);

#endif
			/* Next... */
			continue;
		}


		/* Process 'I' for "Info" (one line only) */
		if (buf[0] == 'I')
		{
			int tval, sval, pval;

			/* Scan for the values */
			if (3 != sscanf(buf+2, "%d:%d:%d",
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
			if (4 != sscanf(buf+2, "%d:%d:%d:%ld",
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
			if (6 != sscanf(buf+2, "%d:%dd%d:%d:%d:%d",
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
			s = buf+2;

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
 * Grab one flag in a ego-item_type from a textual string
 */
#if 0
static bool grab_one_ego_item_flag(ego_item_type *e_ptr, cptr what)
{
	int i;

	/* Check flags1 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, k_info_flags1[i]))
		{
			e_ptr->flags1 |= (1L << i);
			return (0);
		}
	}

	/* Check flags2 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, k_info_flags2[i]))
		{
			e_ptr->flags2 |= (1L << i);
			return (0);
		}
	}

	/* Check flags3 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, k_info_flags3[i]))
		{
			e_ptr->flags3 |= (1L << i);
			return (0);
		}
	}

	/* Oops */
	s_printf("Unknown ego-item flag '%s'.", what);

	/* Error */
	return (1);
}
#endif	// 0

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
#if 0
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
	s_printf("Unknown ego-item flag '%s'.", what);

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
	while (0 == my_fgets(fp, buf, 1024))
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
			if ((3 != sscanf(buf+2, "%d.%d.%d", &v1, &v2, &v3)) ||
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
			s = strchr(buf+2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf+2);

			/* Verify information */
			if (i < error_idx) return (4);

			/* Verify information */
			if (i >= e_head->info_num) return (2);

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

                        for (j = 0; j < 6; j++)
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
			s = buf+2;

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

			/* Scan for the values */
                        if (3 != sscanf(buf+2, "%d:%d:%d",
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
                        if (1 != sscanf(buf+2, "%d",
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
			if (2 != sscanf(buf+2, "%d:%d",
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
                        if (3 != sscanf(buf+2, "%c:%d:%d",
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
			if (4 != sscanf(buf+2, "%d:%d:%d:%ld",
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
			if (4 != sscanf(buf+2, "%d:%d:%d:%d",
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
			s = buf+2;

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

	/* Scan flags3 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags3[i]))
		{
			r_ptr->flags3 |= (1L << i);
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

	/* Scan flags8 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags8[i]))
		{
			r_ptr->flags8 |= (1L << i);
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

	/* Oops */
	s_printf("Unknown monster flag '%s'.", what);

	/* Failure */
	return (1);
}


/*
 * Grab one (spell) flag in a monster_race from a textual string
 */
static errr grab_one_spell_flag(monster_race *r_ptr, cptr what)
{
	int i;

	/* Scan flags4 */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, r_info_flags4[i]))
		{
			r_ptr->flags4 |= (1L << i);
			return (0);
		}
	}

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

	/* Oops */
	s_printf("Unknown monster flag '%s'.", what);

	/* Failure */
	return (1);
}




/*
 * Initialize the "r_info" array, by parsing an ascii "template" file
 */
errr init_r_info_txt(FILE *fp, char *buf)
{
	int i;

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
	while (0 == my_fgets(fp, buf, 1024))
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
			if ((3 != sscanf(buf+2, "%d.%d.%d", &v1, &v2, &v3)) ||
			    (v1 != r_head->v_major) ||
			    (v2 != r_head->v_minor) ||
			    (v3 != r_head->v_patch))
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
			s = strchr(buf+2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf+2);

			/* Verify information */
			if (i < error_idx) return (4);

			/* Verify information */
			if (i >= r_head->info_num) return (2);

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

#if 0	// pernA hack -- someday.
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
		if (buf[0] == 'D')
		{
#if 0	// I've never seen this used :)		- Jir -
			/* Acquire the text */
			s = buf+2;

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
			r_ptr->d_char = sym;
			r_ptr->d_attr = tmp;

			/* Next... */
			continue;
		}

		/* Process 'I' for "Info" (one line only) */
		if (buf[0] == 'I')
		{
			int spd, hp1, hp2, aaf, ac, slp;

			/* Scan for the other values */
			if (6 != sscanf(buf+2, "%d:%dd%d:%d:%d:%d",
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
                if (buf[0] == 'E')
		{
#if 0
                        int weap, tors, fing, head, arms, legs;

			/* Scan for the other values */
                        if (BODY_MAX != sscanf(buf+2, "%d:%d:%d:%d:%d:%d",
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
                if (buf[0] == 'O')
		{
#if 0
                        int treasure, combat, magic, tools;

			/* Scan for the values */
                        if (4 != sscanf(buf+2, "%d:%d:%d:%d",
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
		if (buf[0] == 'W')
		{
			int lev, rar, pad;
			long exp;

			/* Scan for the values */
			if (4 != sscanf(buf+2, "%d:%d:%d:%ld",
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
		if (buf[0] == 'B')
		{
			int n1, n2;

			/* Find the next empty blow slot (if any) */
			for (i = 0; i < 4; i++) if (!r_ptr->blow[i].method) break;

			/* Oops, no more slots */
			if (i == 4) return (1);

			/* Analyze the first field */
			for (s = t = buf+2; *t && (*t != ':'); t++) /* loop */;

			/* Terminate the field (if necessary) */
			if (*t == ':') *t++ = '\0';

			/* Analyze the method */
			for (n1 = 0; r_info_blow_method[n1]; n1++)
			{
				if (streq(s, r_info_blow_method[n1])) break;
			}

			/* Invalid method */
			if (!r_info_blow_method[n1]) return (1);

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
				if (0 != grab_one_basic_flag(r_ptr, s)) return (5);

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
	s_printf("Unknown monster flag '%s'.", what);

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

	/* Oops */
	s_printf("Unknown monster flag '%s'.", what);

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
                        s_printf("Unknown monster ego value modifier %c.", c);
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
	s_printf("Unknown monster flag '%s'.", what);

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
	while (0 == my_fgets(fp, buf, 1024))
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
			if ((3 != sscanf(buf+2, "%d.%d.%d", &v1, &v2, &v3)) ||
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
			s = strchr(buf+2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf+2);

			/* Verify information */
			if (i < error_idx) return (4);

			/* Verify information */
                        if (i >= re_head->info_num) return (2);

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
                        if (12 != sscanf(buf+2, "%c%d:%c%dd%c%d:%c%d:%c%d:%c%d",
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
                        if (8 != sscanf(buf+2, "%c%d:%d:%c%d:%c%ld:%c",
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
							s_printf("no more slots!");
							return (1);
						}

			/* Analyze the first field */
			for (s = t = buf+2; *t && (*t != ':'); t++) /* loop */;

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
				s_printf("invalid method!");
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
				s_printf("invalid effect!");
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
							s_printf("strange dice!");
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
	s_printf("Unknown trap_type flag '%s'.", what);

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
	while (0 == my_fgets(fp, buf, 1024))
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
			if ((3 != sscanf(buf+2, "%d.%d.%d", &v1, &v2, &v3)) ||
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
			s = strchr(buf+2, ':');

			/* Verify that colon */
			if (!s) return (1);

			/* Nuke the colon, advance to the name */
			*s++ = '\0';

			/* Paranoia -- require a name */
			if (!*s) return (1);

			/* Get the index */
			i = atoi(buf+2);

			/* Verify information */
			if (i <= error_idx) return (4);

			/* Verify information */
			if (i >= t_head->info_num) return (2);

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
			int dd,ds;
			char color;

			/* Scan for the values */
			if (8 != sscanf(buf+2, "%d:%d:%d:%d:%d:%dd%d:%c",
					&difficulty, &probability, &another,
					&p1valinc, &minlevel, &dd, &ds,
					&color)) return (1);

			t_ptr->difficulty  = (byte)difficulty;
			t_ptr->probability = (s16b)probability;
			t_ptr->another     = (s16b)another;
			t_ptr->p1valinc     = (s16b)p1valinc;
			t_ptr->minlevel    = (byte)minlevel;
			t_ptr->dd          = (s16b)dd;
			t_ptr->ds          = (s16b)ds;
			t_ptr->color       = color_char_to_attr(color);

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
			s = buf+2;

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



#else	/* ALLOW_TEMPLATES */

#ifdef MACINTOSH
static int i = 0;
#endif

#endif	/* ALLOW_TEMPLATES */

