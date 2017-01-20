/* $Id$ */
/* File: defines-player.h */

/* Purpose: global constants and macro definitions for player.pre lua file */

#define PW_PLAYER	8

#define PY_FOOD_MAX	15000	/* Food value (Bloated) */
#define PY_FOOD_FULL	10000	/* Food value (Normal) */

/*
 * Maximum number of "normal" pack slots, and the index of the "overflow"
 * slot, which can hold an item, but only temporarily, since it causes the
 * pack to "overflow", dropping the "last" item onto the ground.  Since this
 * value is used as an actual slot, it must be less than "INVEN_WIELD" (below).
 * Note that "INVEN_PACK" is probably hard-coded by its use in savefiles, and
 * by the fact that the screen can only show 23 items plus a one-line prompt.
 */
#define INVEN_PACK	23
/*
 * Indexes used for various "equipment" slots (hard-coded by savefiles, etc).
 */
#define INVEN_WIELD	24
/*
 * Total number of inventory slots (hard-coded).
 */
#define INVEN_TOTAL	38	/* since they start at 0, max slot index is INVEN_TOTAL - 1 (!) */


/*
 * Indexes of the various "stats" (hard-coded by savefiles, etc).
 */
#define A_STR	0
#define A_INT	1
#define A_WIS	2
#define A_DEX	3
#define A_CON	4
#define A_CHR	5


/*
 * Player race constants (hard-coded by save-files, arrays, etc)
 */
#define RACE_HUMAN	0
#define RACE_HALF_ELF	1
#define RACE_ELF	2
#define RACE_HOBBIT	3
#define RACE_GNOME	4
#define RACE_DWARF	5
#define RACE_HALF_ORC	6
#define RACE_HALF_TROLL	7
#define RACE_DUNADAN	8
#define RACE_HIGH_ELF	9
#define RACE_YEEK	10
#define RACE_GOBLIN	11
#define RACE_ENT	12
#define RACE_DRACONIAN	13
#ifdef ENABLE_KOBOLD
 #define RACE_KOBOLD	14
 #define RACE_DARK_ELF	15
 #define RACE_VAMPIRE	16
 //#ifdef ENABLE_MAIA
 #define RACE_MAIA	17
 //#endif
#else
 #define RACE_DARK_ELF	14
 #define RACE_VAMPIRE	15
 //#ifdef ENABLE_MAIA
 #define RACE_MAIA	16
 //#endif
#endif
/* (or simply replace all those defines with p_info.txt) */

/*
 * Player class constants (hard-coded by save-files, arrays, etc)
 */
#define CLASS_WARRIOR		0
#define CLASS_MAGE		1
#define CLASS_PRIEST		2
#define CLASS_ROGUE		3
#define CLASS_MIMIC		4
#define CLASS_ARCHER		5
#define CLASS_PALADIN		6
#define CLASS_RANGER		7
#define CLASS_ADVENTURER	8
//#define CLASS_BARD		9
#define CLASS_DRUID		9
#define CLASS_SHAMAN		10
#define CLASS_RUNEMASTER	11
#define CLASS_MINDCRAFTER	12
#ifdef ENABLE_DEATHKNIGHT
 #define CLASS_DEATHKNIGHT	13
#endif
#ifdef ENABLE_HELLKNIGHT
 #define CLASS_HELLKNIGHT	14
#endif
#ifdef ENABLE_CPRIEST
 #define CLASS_CPRIEST		15
#endif


/* for spell-casting */
#define TRAIT_ENLIGHTENED	14	/* Maiar */
#define TRAIT_CORRUPTED		15


/*
 * Skills
 */
#define SKILL_MAX               50000           /* Maximun skill value */
#define SKILL_STEP              1000            /* 1 skill point */

#define SKILL_EXCLUSIVE         9999            /* Flag to tell exclusive skills */

#define SKILL_COMBAT            1
#define SKILL_MASTERY           2
#define SKILL_SWORD             3
#define SKILL_CRITS             4
#define SKILL_POLEARM           5
#define SKILL_BLUNT		6
#define SKILL_ARCHERY           7
#define SKILL_SLING             8
#define SKILL_BOW               9
#define SKILL_XBOW              10
#define SKILL_BACKSTAB          11
#define SKILL_MAGIC             12
//#define SKILL_CASTSPEED         13
#define SKILL_SHOOT_TILL_KILL	13
#define SKILL_SORCERY           14
#define SKILL_MAGERY            15
#define SKILL_MIMIC             16
#define SKILL_DEVICE            17
#define SKILL_SHADOW            18
#define SKILL_PRAY              19
#define SKILL_SPELLLENGTH       20
#define SKILL_SNEAKINESS        21
#define SKILL_DISARM            22
#define SKILL_STEALTH           23
#define SKILL_STEALING          24
#define SKILL_NECROMANCY        25
#define SKILL_ANTIMAGIC         26
/* #define SKILL_AURA_POWER       27 */
#define SKILL_TRAUMATURGY       27
#define SKILL_AURA_FEAR         28
#define SKILL_AURA_SHIVER       29
#define SKILL_AURA_DEATH        30
#define SKILL_HUNTING		31
#define SKILL_TECHNIQUE		32
#define SKILL_MISC              33
#define SKILL_AGILITY		34
#define SKILL_CALMNESS		35
#define SKILL_SWIM		36
#define SKILL_MARTIAL_ARTS	37
#define SKILL_RICOCHET		38
#define SKILL_BOOMERANG		39
#define SKILL_TRAINING		40
#define SKILL_INTERCEPT		41
#define SKILL_DODGE		42
#define SKILL_HEALTH		43
#define SKILL_DIG		44
#define SKILL_SPELLRAD		45
#define SKILL_TRAPPING          46
#define SKILL_AXE		47	/* hrm, bad order */

/* School skills */
#define SKILL_CONVEYANCE        48
#define SKILL_SPELL		49
#define SKILL_MANA              50
#define SKILL_FIRE              51
#define SKILL_AIR               52
#define SKILL_WATER             53
#define SKILL_NATURE            54
#define SKILL_EARTH             55
#define SKILL_DIVINATION        56
#define SKILL_TEMPORAL          57
#define SKILL_META              58
#define SKILL_MIND              59
#define SKILL_UDUN              60

/* for future use, if we ever can get these balanced in a sensible manner (seems unlikely without heavy changes to all other game elements oO) */
#define SKILL_SCHOOL_CRAFTING	61	/* dummy skill for clustering */
#define SKILL_WEAPONSMITHING	62
#define SKILL_ARMOURSMITHING	63
#define SKILL_FLETCHING		64
#define SKILL_JEWELCRAFT	65
#define SKILL_TOOLCRAFT		66
#define SKILL_TAILORING		67
#define SKILL_ALCHEMY		68
#define SKILL_ENCHANTING	69

#define SKILL_HOFFENSE          70
#define SKILL_HDEFENSE          71
#define SKILL_HCURING           72
#define SKILL_HSUPPORT          73

#define SKILL_DRUID_ARCANE	74
#define SKILL_DRUID_PHYSICAL	75

#define SKILL_ASTRAL		77

#define SKILL_DUAL		78 /* dual-wield for rogues */
#define SKILL_STANCE		79 /* combat stances for warriors */

#define SKILL_PPOWER		80 /* the new mindcrafter skills */
#define SKILL_TCONTACT		81 /* the new mindcrafter skills */
#define SKILL_MINTRUSION	82 /* the new mindcrafter skills */

/* Dummy skills - just to make the mass of schools appear more ordered - C. Blue */
#define SKILL_SCHOOL_MAGIC	83
#define SKILL_SCHOOL_PRAYING	84
#define SKILL_SCHOOL_DRUIDISM	85
#define SKILL_SCHOOL_MINDCRAFT	86
//#ifdef ENABLE_OCCULT /* Occult */
 #define SKILL_OSHADOW		87
 #define SKILL_OSPIRIT		88
 /* and the dummy skill for sorting: */
 #define SKILL_SCHOOL_OCCULT	89
 //#ifdef ENABLE_OHERETICISM
  #define SKILL_OHERETICISM	94
 //#endif
//#endif

/* additional ones */
#define SKILL_CLIMB		90
#define SKILL_LEVITATE		91
#define SKILL_FREEACT		92
#define SKILL_RESCONF		93
//94 defined above
#if 0	/* skills to come	- Jir - */
 #define SKILL_INNATE_POWER	/* in mimicry tree */
 #define SKILL_EGO_POWER
#endif	/* 0 */

#define SKILL_SCHOOL_RUNECRAFT	95
#define SKILL_R_LITE		96
#define SKILL_R_DARK		97
#define SKILL_R_NEXU		98
#define SKILL_R_NETH		99
#define SKILL_R_CHAO		100
#define SKILL_R_MANA		101

/* for future use, so no client update will be required */
#define SKILL_SOULFEASTING	102	/* could switch with SKILL_NECROMANCY if ever needed */
#define SKILL_SUMMONING		103
#define SKILL_TAMING		104

#define SKILL_BLOOD_MAGIC	109 /* dummy */

/* For Draconians */
#define SKILL_BREATH		110

#define MAX_SKILLS              128
