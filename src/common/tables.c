/* $Id$ */
/* File: tables.c */

/* Purpose: Angband Tables shared by both client and server.
 * originally added for runecraft. - C. Blue */

#include "angband.h"


#ifdef ENABLE_RCRAFT

/* Table of valid runespell elements, their flags, and the sylables for their casting. */
r_element r_elements[RCRAFT_MAX_ELEMENTS] =
{
	{ 0, "Acid", 		"Delibro",		1, SKILL_R_ACIDWATE, R_ACID,},
	{ 1, "Electricity",	"Fulmin", 		1, SKILL_R_ELECEART, R_ELEC,},
	{ 2, "Fire", 		"Aestus", 		1, SKILL_R_FIRECHAO, R_FIRE,},
	{ 3, "Cold", 		"Gelum",		1, SKILL_R_COLDNETH, R_COLD,},
	{ 4, "Poison", 		"Lepis", 		1, SKILL_R_POISNEXU, R_POIS,},
	{ 5, "Force", 		"Fero",			1, SKILL_R_FORCTIME, R_FORC,},
	{ 6, "Water",		"Mio",	 		1, SKILL_R_ACIDWATE, R_WATE,},
	{ 7, "Earth", 		"Ostes", 		1, SKILL_R_ELECEART, R_EART,},
	{ 8, "Chaos", 		"Emuto", 		1, SKILL_R_FIRECHAO, R_CHAO,},
	{ 9, "Nether", 		"Elido", 		1, SKILL_R_COLDNETH, R_NETH,},
	{10, "Nexus", 		"Vicis", 		1, SKILL_R_POISNEXU, R_NEXU,},
	{11, "Time", 		"Emero",	 	1, SKILL_R_FORCTIME, R_TIME,},
};


/*
	Table of spell potencies.
	
	id, name, level +#, cost *10%, fail +%, damage *10%, cast_time *10%, radius +#, duration *10%
	cost, damage, cast_time and duration are /10 multipliers; fail is a percentage bonus; level, radius are +/- value
 
 */
r_imper r_imperatives [RCRAFT_MAX_IMPERATIVES] =
{
	//i	n	 +l, c%, f+, d%, p%, r+, t%
	{ 0, "minimized",	-1,  5,-10,  6, 10, -1,  5 },
	{ 1, "moderate",	 0, 10,  0, 10, 10,  0, 10 },
	{ 2, "maximized",	 1, 15, 20, 15, 10, +1, 13 },
	{ 3, "compressed",	 2, 15,-10, 13, 10, -2,  8 },
	{ 4, "expanded",	 2, 13,  0, 10, 10, +2, 13 },
	{ 5, "brief",		 3, 13, 15,  6,  5,  0,  5 },
	{ 6, "lengthened",	 3, 15, 10,  6, 10,  0, 16 },
	{ 7, "chaotic",		 0,  0,  0,  0, 10,  5,  0 },
};

/*
	Table of spell augmentations, similar to imperatives. (Uses third rune in permutation!)
	Modifiers have more benefits / less drawbacks scaling with skill level.
	Modifiers will scale from 0% of the benefit / 200% of the drawback, to 100% of each.
 
	Rune, level req, cost *10%, fail +%, damage *10%, cast_time *10%, radius +#, duration *10%
	cost, damage, cast_time and duration are /10 multipliers; fail is a percentage bonus; level, radius are +/- value 
 
	struct r_augment
	{
		u32b rune; //Rune flag
		s16b level; //Level required
		s16b cost; //Cost multiplier
		s16b fail; //Fail multiplier
		s16b dam; //Damage multipler
		s16b time; //Time to cast
		s16b radius; //Radius +/-
		s16b duration; //Duration multiplier
	};
 */
r_augment r_augments[RCRAFT_MAX_ELEMENTS] = 
{
	//R_FLAG,	+l$, *c%, +f%, *d%, *p%, r+, *t%
	{ R_ACID,   1,  10,   0,  10,  10,   0, 13}, //30% duration increase
	{ R_ELEC,   1,   7,   0,  10,  10,   0, 10}, //30% cost reduction
	{ R_FIRE,   1,  12,   0,  12,  10,   0, 10}, //20% damage/cost increase
	{ R_COLD,   1,  10, -10,  10,  10,   0, 10}, //10% failure reduction
	{ R_POIS,   5,  10,   0,  10,   8,   0, 10}, //20% energy decrease
	{ R_FORC,  15,  10,   0,  10,  10,   2, 10}, // +2 radius increase
	{ R_WATE,  15,  10,  -5,   8,   8,   0, 10}, //20% energy/damage decrease, 5% failure decrease
	{ R_EART,  15,  11,   0,  11,  10,   0, 11}, //10% damage/cost increase, 10% duration increase
	{ R_CHAO,  25,  14, +20,  14,  10,   0, 10}, //40% damage/cost increase, 20% failure increase
	{ R_NETH,  25,  14,   0,  14,  10,  -2, 10}, //40% damage/cost increase, -2 radius decrease
	{ R_NEXU,  25,  10,   0,  10,  10,   3,  7}, // +3 radius increase, 30% duration decrease
	{ R_TIME,  30,  10, +10,   6,   6,   0, 12}, //40% energy/damage decrease, 20% duration increase
};

/*
Runespell types.

	int id;
	unsigned long type; //Flag
	char * title; //Name
	int cost;	//+ skill level to overall spell
	byte pen; 	//MP multiplier
*/
r_type runespell_types[RCRAFT_MAX_TYPES] =
{
	{ 0, R_MELE, "burst",	-3,  5 },
	{ 1, R_SELF, "self",	 0, 10 },
	{ 2, R_BOLT, "bolt",	 0, 10 },
	{ 3, R_BEAM, "beam",	 1, 11 },
	{ 4, R_BALL, "ball",	 3, 12 },
	{ 5, R_WAVE, "wave",	 3, 11 },
	{ 6, R_CLOU, "cloud",	 5, 13 },
	{ 7, R_STOR, "storm",	 7, 20 },
};

/* Table of valid runespell types and their meta information.

GF_TYPES are zero if they represent a new/special spell, dealt with case by case in cast_runespell() (usually self-spells, or spells with more than one effect)

	int id;
	char * description; //Used in the projection description
	byte dam; //Damage/power multiplier
	byte pen; // Bonus MP penalty for a particular spell. (/10)
	byte level; //  Average skill level for success 
	byte fail; // fail rate multiplier (how much more difficult is it than something else?)
	int gf_type; //0 for special cases (handled by cast_runespell)
 
	int gf_explode; //0 for no explosion
	int r_augment; //0 for no augment
 
	Number of occurances at the right, mostly due to nexus/NULL combinations, could be reduced if NULL are changed.
	NULL occurances are also risky due to accessing RT_TIME for instance, probably okay if adventurers are limited at 2 runes.
	Might be fine actually, allows weaker and versatile runies w/ basic spells; 'haste' effect for full casters, even if not trained in time explicitly.
	WARNING: Don't re-order this table without changing <<#define RT_BLAH index>> to match! - Kurzel
*/
r_spell runespell_list[RT_MAX] =
{
{ RT_NONE,			"nothing",		10,  0,  0,  0, 0, 0,			0,		0 }, //0 (Use RT_NULL for elements that 'cancel', RT_NONE for code failure)
{ RT_ACID,			"acid",			11, 10,  5,  0, 1, GF_ACID,		0,		0 }, //3
{ RT_ELEC,			"lightning",		11, 10,  5,  0, 1, GF_ELEC,		0,		0 }, //3
{ RT_FIRE,			"fire",			11, 10,  5,  0, 1, GF_FIRE,		0,		0 }, //4
{ RT_COLD,			"frost",		11, 10,  5,  0, 1, GF_COLD,		0,		0 }, //2
{ RT_POISON,			"gas",			12, 10, 10,  5, 1, GF_POIS,		0,		0 }, //7 (due to NULL combinations, make this more interesting? or are runies just BAD at this element)
{ RT_FORCE,			"force",		 9, 12, 20, 15, 1, GF_FORCE,		0,		0 }, //3
{ RT_WATER,			"water",		10, 12, 15, 10, 1, GF_WATER,		0,		0 }, //4
{ RT_SHARDS,			"shards",		11, 12, 15, 10, 1, GF_SHARDS,		0,		0 }, //4
{ RT_CHAOS,			"chaos",		13, 15, 25, 15, 1, GF_CHAOS,		0,		0 }, //4
{ RT_NETHER,			"nether",		13, 15, 25, 15, 1, GF_NETHER,		0,		0 }, //5
{ RT_NEXUS,			"nexus",		12, 14, 25, 10, 1, GF_NEXUS,		0,		0 }, //3
{ RT_TIME,			"time",			10, 20, 30, 10, 1, GF_TIME,		0,		0 }, //5

{ RT_POWER,			"dissolution",		10, 20, 30, 15, 2, GF_DISP_ALL,		0,		0 }, //1
{ RT_DISINTEGRATE_ELEC,		"disintegration",	12, 25, 40, 20, 3, GF_DISINTEGRATE,	0,		R_ELEC }, //1
{ RT_DISINTEGRATE_SHARDS,	"disintegration",	12, 25, 40, 20, 3, GF_DISINTEGRATE,	0,		R_EART }, //1
{ RT_DISINTEGRATE_FIRE,		"disintegration",	12, 25, 40, 20, 3, GF_DISINTEGRATE,	0,		R_FIRE }, //1
{ RT_DISINTEGRATE_CHAOS,	"disintegration",	12, 25, 40, 20, 3, GF_DISINTEGRATE,	0,		R_CHAO }, //1
{ RT_DISINTEGRATE_COLD,		"disintegration",	12, 25, 40, 20, 3, GF_DISINTEGRATE,	0,		R_COLD }, //1
{ RT_DISINTEGRATE_NETHER,	"disintegration",	12, 25, 40, 20, 3, GF_DISINTEGRATE,	0,		R_NETH }, //1
{ RT_DISINTEGRATE_POISON,	"disintegration",	12, 25, 40, 20, 3, GF_DISINTEGRATE,	0,		R_POIS }, //1
{ RT_DISINTEGRATE_NEXUS,	"disintegration",	12, 25, 40, 20, 3, GF_DISINTEGRATE,	0,		R_NEXU }, //1
{ RT_DISINTEGRATE_FORCE,	"disintegration",	12, 25, 40, 20, 3, GF_DISINTEGRATE,	0,		R_FORC }, //1
{ RT_DISINTEGRATE_TIME,		"disintegration",	12, 25, 40, 20, 3, GF_DISINTEGRATE,	0,		R_TIME }, //1

{ RT_HI_ELEC,			"charge",		15, 15, 20, 10, 2, GF_ELEC,		GF_ELEC,	R_ELEC }, //1 (bolt effect goes 'through' enemies, like MC-bolt?)
{ RT_STARLIGHT_ACID,		"starlight",		12, 15, 35, 10, 3, GF_LITE,		GF_LITE,	R_ACID }, //1
{ RT_STARLIGHT_WATER,		"starlight",		12, 15, 35, 10, 3, GF_LITE,		GF_LITE,	R_WATE }, //1
{ RT_STARLIGHT_FIRE,		"starlight",		12, 15, 35, 10, 3, GF_LITE,		GF_LITE,	R_FIRE }, //1
{ RT_STARLIGHT_CHAOS,		"starlight",		12, 15, 35, 10, 3, GF_LITE,		GF_LITE,	R_CHAO }, //1
{ RT_STARLIGHT_COLD,		"starlight",		12, 15, 35, 10, 3, GF_LITE,		GF_LITE,	R_COLD }, //1
{ RT_STARLIGHT_NETHER,		"starlight",		12, 15, 35, 10, 3, GF_LITE,		GF_LITE,	R_NETH }, //1
{ RT_STARLIGHT_POISON,		"starlight",		12, 15, 35, 10, 3, GF_LITE,		GF_LITE,	R_POIS }, //1
{ RT_STARLIGHT_NEXUS,		"starlight",		12, 15, 35, 10, 3, GF_LITE,		GF_LITE,	R_NEXU }, //1
{ RT_STARLIGHT_FORCE,		"starlight",		12, 15, 35, 10, 3, GF_LITE,		GF_LITE,	R_FORC }, //1
{ RT_STARLIGHT_TIME,		"starlight",		12, 15, 35, 10, 3, GF_LITE,		GF_LITE,	R_TIME }, //1

{ RT_HELL_FIRE,			"hellfire",		13, 13, 25, 15, 2, GF_HELL_FIRE,	0,		0 }, //1
{ RT_DETONATION_ACID,		"detonations",		14, 15, 35, 15, 3, GF_DETONATION,	0,		R_ACID }, //1
{ RT_DETONATION_WATER,		"detonations",		14, 15, 35, 15, 3, GF_DETONATION,	0,		R_WATE }, //1
{ RT_DETONATION_ELEC,		"detonations",		14, 15, 35, 15, 3, GF_DETONATION,	0,		R_ELEC }, //1
{ RT_DETONATION_SHARDS,		"detonations",		14, 15, 35, 15, 3, GF_DETONATION,	0,		R_EART }, //1
{ RT_DETONATION_COLD,		"detonations",		14, 15, 35, 15, 3, GF_DETONATION,	0,		R_COLD }, //1
{ RT_DETONATION_NETHER,		"detonations",		14, 15, 35, 15, 3, GF_DETONATION,	0,		R_NETH }, //1
{ RT_DETONATION_POISON,		"detonations",		14, 15, 35, 15, 3, GF_DETONATION,	0,		R_POIS }, //1
{ RT_DETONATION_NEXUS,		"detonations",		14, 15, 35, 15, 3, GF_DETONATION,	0,		R_NEXU }, //1
{ RT_DETONATION_FORCE,		"detonations",		14, 15, 35, 15, 3, GF_DETONATION,	0,		R_FORC }, //1
{ RT_DETONATION_TIME,		"detonations",		14, 15, 35, 15, 3, GF_DETONATION,	0,		R_TIME }, //1

{ RT_ANNIHILATION,		"annihilation",		 5, 20, 30, 15, 2, GF_ANNIHILATION,	0,		0 }, //1
{ RT_STASIS_ACID,		"stasis",		10, 10, 30, 10, 3, GF_STASIS,		0,		R_ACID }, //1 (needs a colour scheme? power check)
{ RT_STASIS_WATER,		"stasis",		10, 10, 30, 10, 3, GF_STASIS,		0,		R_WATE }, //1
{ RT_STASIS_ELEC,		"stasis",		10, 10, 30, 10, 3, GF_STASIS,		0,		R_ELEC }, //1
{ RT_STASIS_SHARDS,		"stasis",		10, 10, 30, 10, 3, GF_STASIS,		0,		R_EART }, //1
{ RT_STASIS_FIRE,		"stasis",		10, 10, 30, 10, 3, GF_STASIS,		0,		R_FIRE }, //1
{ RT_STASIS_CHAOS,		"stasis",		10, 10, 30, 10, 3, GF_STASIS,		0,		R_CHAO }, //1
{ RT_STASIS_POISON,		"stasis",		10, 10, 30, 10, 3, GF_STASIS,		0,		R_POIS }, //1
{ RT_STASIS_NEXUS,		"stasis",		10, 10, 30, 10, 3, GF_STASIS,		0,		R_NEXU }, //1
{ RT_STASIS_FORCE,		"stasis",		10, 10, 30, 10, 3, GF_STASIS,		0,		R_FORC }, //1
{ RT_STASIS_TIME,		"stasis",		10, 10, 30, 10, 3, GF_STASIS,		0,		R_TIME }, //1

{ RT_UNBREATH,			"noxious unbreath",	16, 15, 25, 10, 2, GF_UNBREATH,		GF_POIS,	R_POIS }, //2
{ RT_DRAIN_ACID,		"drain",		 3, 25, 35, 20, 3, GF_OLD_DRAIN,	0,		R_ACID }, //1 (lifesteal fix)
{ RT_DRAIN_WATER,		"drain",		 3, 25, 35, 20, 3, GF_OLD_DRAIN,	0,		R_WATE }, //1
{ RT_DRAIN_ELEC,		"drain",		 3, 25, 35, 20, 3, GF_OLD_DRAIN,	0,		R_ELEC }, //1
{ RT_DRAIN_SHARDS,		"drain",		 3, 25, 35, 20, 3, GF_OLD_DRAIN,	0,		R_EART }, //1
{ RT_DRAIN_FIRE,		"drain",		 3, 25, 35, 20, 3, GF_OLD_DRAIN,	0,		R_FIRE }, //1
{ RT_DRAIN_CHAOS,		"drain",		 3, 25, 35, 20, 3, GF_OLD_DRAIN,	0,		R_CHAO }, //1
{ RT_DRAIN_COLD,		"drain",		 3, 25, 35, 20, 3, GF_OLD_DRAIN,	0,		R_COLD }, //1
{ RT_DRAIN_NETHER,		"drain",		 3, 25, 35, 20, 3, GF_OLD_DRAIN,	0,		R_NETH }, //1
{ RT_DRAIN_FORCE,		"drain",		 3, 25, 35, 20, 3, GF_OLD_DRAIN,	0,		R_FORC }, //1
{ RT_DRAIN_TIME,		"drain",		 3, 25, 35, 20, 3, GF_OLD_DRAIN,	0,		R_TIME }, //1

{ RT_INERTIA,			"inertia",		12, 12, 20, 10, 2, GF_INERTIA,		0,		0 }, //1
{ RT_GRAVITY_ACID,		"gravity",		13, 13, 30, 15, 3, GF_GRAVITY,		0,		R_ACID }, //1
{ RT_GRAVITY_WATER,		"gravity",		13, 13, 30, 15, 3, GF_GRAVITY,		0,		R_WATE }, //1
{ RT_GRAVITY_ELEC,		"gravity",		13, 13, 30, 15, 3, GF_GRAVITY,		0,		R_ELEC }, //1
{ RT_GRAVITY_SHARDS,		"gravity",		13, 13, 30, 15, 3, GF_GRAVITY,		0,		R_EART }, //1
{ RT_GRAVITY_FIRE,		"gravity",		13, 13, 30, 15, 3, GF_GRAVITY,		0,		R_FIRE }, //1
{ RT_GRAVITY_CHAOS,		"gravity",		13, 13, 30, 15, 3, GF_GRAVITY,		0,		R_CHAO }, //1
{ RT_GRAVITY_COLD,		"gravity",		13, 13, 30, 15, 3, GF_GRAVITY,		0,		R_COLD }, //1
{ RT_GRAVITY_NETHER,		"gravity",		13, 13, 30, 15, 3, GF_GRAVITY,		0,		R_NETH }, //1
{ RT_GRAVITY_POISON,		"gravity",		13, 13, 30, 15, 3, GF_GRAVITY,		0,		R_POIS }, //1
{ RT_GRAVITY_NEXUS,		"gravity",		13, 13, 30, 15, 3, GF_GRAVITY,		0,		R_NEXU }, //1

/* Gestalts (names should be somewhat mystic/arcane/esoteric, but also scientific) - Kurzel */
{ RT_ACID_ELEC,			"ionization",		12, 12, 10,  5, 2, GF_ACID_ELEC,	0,		0 }, //1 (opposing nexus boost)
{ RT_ACID_FIRE,			"bile",			12, 12, 10,  5, 2, GF_ACID_FIRE,	0,		0 }, //2 (name -> one word? different than 'bile' U)
{ RT_ACID_COLD,			"rime",			12, 12, 10,  5, 2, GF_ACID_COLD,	0,		0 }, //2
{ RT_ACID_POISON,		"venom",		13, 12, 15,  5, 2, GF_ACID_POISON,	GF_BLIND,	0 }, //1 (not GF_CONFUSION to avoid extra dmg, same effect)
{ RT_ACID_TIME,			"temporal acid",	14, 12, 20, 10, 2, GF_ACID,		GF_TIME,	R_TIME }, //2 (+20% damage, +20% cost/level on 'temporal' versions)
{ RT_PLASMA,			"plasma",		13, 13, 20,  5, 2, GF_PLASMA,		0,		0 }, //2 (RT_ELEC_FIRE)
{ RT_ELEC_COLD,			"static",		12, 12, 10,  5, 2, GF_ELEC_COLD,	0,		0 }, //2
{ RT_ELEC_POISON,		"jolting",		13, 12, 15,  5, 2, GF_ELEC_POISON,	GF_STUN,	0 }, //1
{ RT_ELEC_TIME,			"temporal lightning",	14, 12, 20, 10, 2, GF_ELEC,		GF_TIME,	R_TIME }, //2 (name too long?)
/* RT_FIRE_COLD - defines RT_NULL - IMPORTANT! */
{ RT_NULL,			"nothing",		10,  0,  0,  0, 0, 0,			0,		0 }, //5
/* Non-engendered overlap:
 * R_EART | R_COLD | R_POIS
 */
{ RT_FIRE_POISON,		"consumption",		13, 12, 15,  5, 2, GF_FIRE_POISON,	GF_OLD_SLEEP,	0 }, //1 (poison effect rule -> fever-fainting?)
{ RT_FIRE_TIME,			"temporal fire",	14, 12, 20, 10, 2, GF_FIRE,		GF_TIME,	R_TIME }, //2
{ RT_COLD_POISON,		"hypothermia",		13, 12, 10,  5, 2, GF_COLD_POISON,	GF_OLD_SLOW,	0 }, //1
{ RT_COLD_TIME,			"temporal frost",	14, 12, 20, 10, 2, GF_COLD,		GF_TIME,	R_TIME }, //2
{ RT_SLOW,			"mire",			15, 15, 30, 10, 2, GF_OLD_SLOW,		0,		0 }, //1 (RT_POISON_TIME)

/* Acid */
{ RT_DISARM_ACID,		"corrosion",		 5, 10, 15,  5, 2, GF_CORRODE,		0,		0 }, //2
{ RT_NUKE,			"toxine",		15, 15, 25, 15, 2, GF_NUKE,		0,		0 }, //1 (explode with poly?, name -> toxic waste?)
{ RT_DARKNESS_ACID,		"hungry darkness",	12, 12, 15,  5, 2, GF_DARK,		GF_ACID,	0 }, //1
{ RT_HI_ACID,			"ablation",		15, 15, 20, 10, 2, GF_ACID,		GF_ACID,	R_ACID }, //2
{ RT_ACID_NEXUS,		"electrolysis",		13, 12, 10,  0, 2, GF_ELEC,		0,		R_NEXU }, //1 (name -> reduction?)
/* Electricity */
{ RT_ELEC_WATER,		"ionized water",	13, 12, 10,  0, 2, GF_ELEC,		GF_WATER,	0 }, //3 (this might do something for the self spell ^^) (self-shock? projection is mostly elec, +20% dmg, water difficulty)
{ RT_BRILLIANCE_ELEC,		"shining brilliance",	 8, 12, 15,  5, 2, GF_LITE,		GF_ELEC,	0 }, //1
{ RT_TELEPORT_ELEC,		"displacement",		13, 10, 15,  5, 2, GF_AWAY_ALL,		0	,	0 }, //1
{ RT_THUNDER,			"thunder",		13, 13, 20,  5, 2, GF_SOUND,		0,		0 }, //2
{ RT_ELEC_NEXUS,		"oxidation",		13, 12, 10,  5, 2, GF_ACID,		0,		R_NEXU }, //1
/* Fire */
{ RT_FIRE_WATER,		"steam",		10,  0,  0,  0, 0, 0,			0,		0 }, //0 (this might do something for the self spell ^^)
{ RT_DIG_FIRE,			"eroding fire",		10, 12, 15,  5, 2, GF_DIG_FIRE,		0,		0 }, //2 (also burn trees?)
{ RT_DARKNESS_FIRE,		"burning darkness",	12, 12, 15,  5, 2, GF_DARK,		GF_FIRE,	0 }, //1
{ RT_HI_FIRE,			"blazing fire",		15, 15, 20, 10, 2, GF_FIRE,		GF_FIRE,	R_FIRE }, //2
{ RT_FIRE_NEXUS,		"wicking fire",		13, 12, 10,  5, 2, GF_COLD,		0,		R_NEXU }, //1 (nomenclature for nexus+base? unique atm)
/* Cold */
{ RT_ICE,			"ice",			13, 13, 20,  5, 2, GF_ICE,		0,		0 }, //2
{ RT_DISARM_COLD,		"shattering",		 5, 10, 15,  5, 2, GF_SHATTER,		0,		0 }, //2
{ RT_BRILLIANCE_COLD,		"grim brilliance",	 8, 12, 15,  5, 2, GF_LITE,		GF_COLD,	0 }, //1
{ RT_HI_COLD,			"hoarfrost",		15, 15, 20, 10, 2, GF_COLD,		GF_COLD,	R_COLD }, //2
{ RT_COLD_NEXUS,		"exothermy",		13, 12, 10,  5, 2, GF_FIRE,		0,		R_NEXU }, //1
/* Poison */
{ RT_WATERPOISON,		"miasma",		14, 12, 20, 10, 2, GF_WATERPOISON,	0,		0 }, //1
{ RT_ICEPOISON,			"frozen miasma",	14, 12, 20, 10, 2, GF_ICEPOISON,	0,		0 }, //1 (misnomer -> shards != ice)
{ RT_CONFUSION,			"confusion",		12, 12, 20, 10, 2, GF_CONFUSION,	0,		0 }, //1 (should deal damage, AND apply effect! (NOT psi!) >.> GWoP)
{ RT_BLINDNESS,			"blindness",		 4, 10, 10, 10, 2, GF_BLIND,		0,		0 }, //1
{ RT_STUN,			"concussion",		 2, 10, 15, 10, 2, GF_STUN,		0,		0 }, //1
/* Force */
{ RT_WAVE,			"pressure",		15, 20, 35, 10, 2, GF_WAVE,		0,		0 }, //2
{ RT_MISSILE,			"magic missiles",	12, 20, 30,  5, 2, GF_MISSILE,		0,		0 }, //2
{ RT_LIGHT,			"light",		 6, 10, 10,  0, 2, GF_LITE,		0,		0 }, //1 (NOT GF_LITE_WEAK)
{ RT_SHADOW,			"shadow",		10, 10, 10,  0, 2, GF_DARK,		0,		0 }, //1
{ RT_TELEPORT_NEXUS,		"displacement",		13, 15, 15,  5, 2, GF_AWAY_ALL,		0,		0 }, //2
/* Water */
{ RT_DIG,			"erosion",		10, 12, 15,  0, 2, GF_KILL_WALL,	0,		0 }, //1 (should have a *good* self-spell version)
{ RT_POLYMORPH,			"transformation",	10, 10, 25, 15, 2, GF_OLD_POLY,		0,		0 }, //1
{ RT_CLONE,			"duplication",		10, 10, 25, 15, 2, GF_OLD_CLONE,	0,		0 },  //1
{ RT_WATER_NEXUS,		"crystal",		13, 14, 20, 10, 2, GF_SHARDS,		0,		R_NEXU }, //1 (name -> solidification?)
{ RT_DIG_TELEPORT,		"erosion",		10, 12, 15,  5, 2, GF_KILL_WALL,	0,		0 }, //2 (self teleport)
/* Earth */
{ RT_INFERNO,			"raging fire",		13, 15, 20, 15, 2, GF_INFERNO,		0,		0 }, //1 (naming needs change?)
{ RT_GENOCIDE,			"genocide",		 1, 20, 30, 15, 2, 0,			0,		0 }, //1 (GF_GENOCIDE - Add effect!)
{ RT_DIG_MEMORY,		"erosion",		10, 12, 15,  5, 2, GF_KILL_WALL,	0,		0 }, //2 (self memory)
{ RT_EARTH_NEXUS,		"mud",			12, 14, 20, 10, 2, GF_WATER,		0,		R_NEXU }, //1 (name -> liquidation?)
/* Chaos */
{ RT_CHAOS_NETHER,		"nexus",		12, 14, 20, 10, 2, GF_NEXUS,		0,		0 }, //0 (exactly RT_nexus -> chaos+nether+3rd invert
{ RT_DISENCHANT,		"disenchantment",	12, 12, 30, 15, 2, GF_DISENCHANT,	0,		0 }, //1 (Balance this! ..with mana..)
{ RT_WONDER,			"wonder",		10, 10, 10,  0, 2, GF_WONDER,		0,		0 }, //1 (GF_WONDER - Add effect!)
/* Nether */
{ RT_MANA,			"mana",			12, 12, 30, 15, 2, GF_MANA,		0,		0 }, //1
{ RT_NETHER_TIME,		"oblivion",		10,  0,  0,  0, 2, 0,			0,		0 }, //0 (another NULL combination? should be something else, maybe just nether augmented w/ time? balance w/ chaos+time 'death' effect might be good here ^^)
/* Time */
{ RT_SLEEP,			"slumber",		 8, 10, 25, 10, 2, GF_OLD_SLEEP,	0,		0 }, //2

//New 3-rune (triple school) RT_EFFECTs here! Don't forget to organize by index... - Kurzel
//Minimum effect level is the maximum level of composition runes
//Minimum failure malus is usually the maximum penalty of composistion runes
//Damage is boosted more for low runes, low runes usually take precidence in combination
//Powerful combination elements always 'augment' rather than explode, but may do both

//RESISTANCE self utility (should be removed, more 'unique' spells or families should be added)
{ RT_WONDER_RESIST,		"wonder",		10, 10, 10,  0, 2, 0,			0,		0 }, //7 (self spell -> 5 element resist from 3 base/poison CHECK for this on other projections like PLASMA)

//GLYPH self utility combinations, use light/dark+explosion to project, elec/earth school absent due to starlite/self -> circle (minimized circle should be a single glyph!)
//Level fixed to 30, starlite 35 (for usability =.=) +20% damage to correct level malus (balance, will be /weaker/ than brilliance/darkness)
//Descriptors listed as light and shadow, rather than brilliance / darkness, as above; this helps w/ identification
{ RT_GLYPH_LITE_ACID,		"hungry light",		 7, 12, 30, 15, 3, GF_LITE,		GF_ACID,	0 }, //1
{ RT_GLYPH_LITE_COLD,		"grim brilliance",	10, 12, 30, 15, 3, GF_LITE,		GF_COLD,	0 }, //1
{ RT_GLYPH_LITE_POISON,		"radiation",		 7, 12, 30, 15, 3, GF_LITE,		GF_POIS,	0 }, //1
{ RT_GLYPH_DARK_ACID,		"hungry darkness",	14, 12, 30, 15, 3, GF_DARK,		GF_ACID,	0 }, //1
{ RT_GLYPH_DARK_FIRE,		"burning darkness",	14, 12, 30, 15, 3, GF_DARK,		GF_FIRE,	0 }, //1
{ RT_GLYPH_DARK_POISON,		"noxious darkness",	12, 12, 30, 15, 3, GF_DARK,		GF_POIS,	0 }, //1

//LITE/DARK + SHARDS , balanced at loss of fire+water combinations
//Powerful or weak for level? - perhaps needs a power check
{ RT_BRILLIANCE_SHARDS,		"slicing brilliance",	11, 12, 35,  5, 3, GF_LITE,		GF_SHARDS,	0 }, //1
{ RT_DARKNESS_SHARDS,		"slicing darkness",	13, 12, 35,  5, 3, GF_DARK,		GF_SHARDS,	0 }, //1

//R_CHAO dominates base elements, explodes w/ random base/poison element, +20% power, augments w/ chaos
{ RT_CHAOS_BASE,		"limbic chaos",		15, 15, 35, 15, 3, GF_CHAOS,		0,		R_CHAO }, //6 (replace 0 with GF_BASE_RANDOM hack <.< 'limbic' latin root)

//add R_TIME to get time augment, wonder explosion, base elements get +25 level +20% dmg, include poison/shards, check power (might need boost?)
{ RT_ACID_WONDER,		"enchanted acid",	13, 10, 25,  5, 3, GF_ACID,		GF_WONDER,	R_TIME }, //1
{ RT_ELEC_WONDER,		"enchanted lightning",	13, 10, 25,  5, 3, GF_ELEC,		GF_WONDER,	R_TIME }, //1
{ RT_COLD_WONDER,		"enchanted frost",	13, 10, 25,  5, 3, GF_COLD,		GF_WONDER,	R_TIME }, //1
{ RT_POISON_WONDER,		"enchanted confusion",	14, 12, 35, 10, 3, GF_CONFUSION,	GF_WONDER,	R_TIME }, //1 (exception with poison/chaos, poison rule)
{ RT_WATER_WONDER,		"enchanted water",	12, 12, 40, 10, 3, GF_WATER,		GF_WONDER,	R_TIME }, //1
{ RT_SHARDS_WONDER,		"enchanted shards",	13, 12, 40, 10, 3, GF_SHARDS,		GF_WONDER,	R_TIME }, //1

//also augment these special ones, confusion (damaging) explosion, sort w/ nether
{ RT_WATERPOISON_CHAOS,		"chaotic miasma",	16, 12, 30, 10, 3, GF_WATERPOISON,	GF_CONFUSION,	R_CHAO }, //1
{ RT_ICEPOISON_CHAOS,		"chaotic frozen miasma",16, 12, 30, 10, 3, GF_ICEPOISON,	GF_CONFUSION,	R_CHAO }, //1
//water/icepoison_neth at end for now, sort this!
//new water/icepoison_neth here!

//R_NETH generally explodes AND augments with nether (blindness w/ poison)
//For base elements/gestalt, +10 level, +10% damage
{ RT_ACID_ELEC_NETHER,		"corrupted ionization",	13, 12, 25, 10, 3, GF_ACID_ELEC,	GF_NETHER,	R_NETH }, //1
{ RT_ACID_FIRE_NETHER,		"corrupted bile",	13, 12, 25, 10, 3, GF_ACID_FIRE,	GF_NETHER,	R_NETH }, //1
{ RT_ACID_POISON_NETHER,	"corrupted venom",	14, 12, 20, 10, 3, GF_ACID_POISON,	GF_BLIND,	R_NETH }, //1
{ RT_ELEC_POISON_NETHER,	"corrupted jolting",	14, 12, 20, 10, 3, GF_ELEC_POISON,	GF_BLIND,	R_NETH }, //1
{ RT_FIRE_POISON_NETHER,	"corrupted consumption",14, 12, 20, 10, 3, GF_FIRE_POISON,	GF_BLIND,	R_NETH }, //1

//R_FORC generally explodes with force (stun w/ poison)
//For base elements/gestalt, +10 level, +10% damage
//Note: no elec due to thunder family, poison should stun more, less dmg
{ RT_ACID_FIRE_FORCE,		"forceful bile",	13, 12, 30, 10, 3, GF_ACID_FIRE,	GF_FORCE,	0 }, //1 (name is too awesome? ^^)
{ RT_ACID_COLD_FORCE,		"forceful rime",	13, 12, 30, 10, 3, GF_ACID_COLD,	GF_FORCE,	0 }, //1
{ RT_ACID_POISON_FORCE,		"forceful venom",	14, 12, 25, 10, 3, GF_ACID_POISON,	GF_STUN,	0 }, //1
{ RT_FIRE_POISON_FORCE,		"forceful consumption",	14, 12, 25, 10, 3, GF_FIRE_POISON,	GF_STUN,	0 }, //1
{ RT_COLD_POISON_FORCE,		"forceful hypothermia",	14, 12, 25, 10, 3, GF_COLD_POISON,	GF_STUN,	0 }, //1

//R_TIME should almost *always* augment with time
//Lesser elements also explode with time (slow w/ poison)
//Explodes with base elements/gestalt, +25 level, +20% damage (slightly weaker)
{ RT_ACID_ELEC_TIME,		"temporal ionization",	14, 12, 25, 10, 3, GF_ACID_ELEC,	GF_TIME,	R_TIME }, //1
{ RT_ACID_FIRE_TIME,		"temporal bile",	14, 12, 25, 10, 3, GF_ACID_FIRE,	GF_TIME,	R_TIME }, //1
{ RT_ACID_COLD_TIME,		"temporal rime",	14, 12, 25, 10, 3, GF_ACID_COLD,	GF_TIME,	R_TIME }, //1
{ RT_ACID_POISON_TIME,		"temporal venom",	15, 12, 30, 10, 3, GF_ACID_POISON,	GF_OLD_SLOW,	R_TIME }, //1
{ RT_ELEC_COLD_TIME,		"temporal static",	14, 12, 25, 10, 3, GF_ELEC_COLD,	GF_TIME,	R_TIME }, //1
{ RT_ELEC_POISON_TIME,		"temporal jolting",	15, 12, 30, 10, 3, GF_ELEC_POISON,	GF_OLD_SLOW,	R_TIME }, //1
{ RT_FIRE_POISON_TIME,		"temporal consumption",	15, 12, 30, 10, 3, GF_FIRE_POISON,	GF_OLD_SLOW,	R_TIME }, //1
{ RT_COLD_POISON_TIME,		"temporal hypothermia",	15, 12, 30, 10, 3, GF_COLD_POISON,	GF_OLD_SLOW,	R_TIME }, //1

/* sort all these water/icePoison things */
{ RT_WATERPOISON_TIME,		"temporal miasma",	16, 12, 35, 10, 3, GF_WATERPOISON,	GF_TIME,	R_TIME }, //1
{ RT_ICEPOISON_TIME,		"temporal frozen miasma",16, 12, 35, 10, 3, GF_ICEPOISON,	GF_TIME,	R_TIME }, //1
/* sort this! caused the 3-rune bug =\ */
{ RT_ROCKET,			"rockets",		14, 16, 40, 10, 3, GF_ROCKET,		0,		0 }, //1 (power check?)

{ RT_DIG_FIRE_TIME,		"eroding heat",		10, 12, 15,  5, 3, GF_DIG_FIRE,		0,		R_TIME }, //1
/* power check here and normal ones too! */
{ RT_DISARM_COLD_TIME,		"shattering",		 5, 10, 15,  5, 3, GF_SHATTER,		0,		R_TIME }, //1
{ RT_DISARM_ACID_TIME,		"corrosion",		 5, 10, 15,  5, 3, GF_CORRODE,		0,		R_TIME }, //1

//R_NEXU adds nexus burst, damage, to diametrically opposing base runes
//Only 2 due to R_FIRE | R_COLD -> RT_NULL
{ RT_ACID_ELEC_NEXUS,		"warped ionization",	14, 12, 30, 10, 3, GF_ACID_ELEC,	GF_NEXUS,	0 }, //1 (+20% damage boost)
{ RT_DIG_NEXUS,			"warped erosion",	14, 14, 35, 10, 3, GF_NEXUS,		GF_KILL_WALL,	0 }, //1 (+20% damage boost ..silly.. make it explode in kill_wall, project nexus instead!)

//The 'Trinity' of PLASMA, THUNDER, ICE, use augments
//Also explode with 'powerful' elements (2nd elements from base schools)
//Exclude force explicitly! (nvm this for GF_ICE, for now..)
//Note that nexus doesn't augment these, but allows access from other schools
//Exceptions:
//Plasma/ice has no cold/fire augment due to R_FIRE | R_COLD -> RT_NULL
//Thunder has no water augment due to R_ELEC | R_WATE -> RT_NULL
//Thunder also has no force augment, plasma gets a special one, giving trinity some more unique-pattern aspects within itself when augmented 

//Fire/Elec/Force -> hi_plasma
{ RT_HI_PLASMA,			"flaring plasma",	15, 13, 30,  5, 3, GF_PLASMA,		GF_LITE,	R_FORC }, //1 (+20% dmg, explodes w/ Lite, Force augment, name unique)
//PLASMA uses augments
{ RT_PLASMA_ACID,		"plasma",		13, 13, 20,  5, 3, GF_PLASMA,		0,		R_ACID }, //1
{ RT_PLASMA_WATER,		"liquid plasma",	13, 13, 20,  5, 3, GF_PLASMA,		GF_WATER,	R_WATE }, //1
{ RT_PLASMA_NETHER,		"dark plasma",		13, 13, 20,  5, 3, GF_PLASMA,		GF_DARK,	R_NETH }, //1 (nether -> dark to counter chaos in ice)
{ RT_PLASMA_POISON,		"fainting plasma",	13, 13, 20,  5, 3, GF_PLASMA,		GF_OLD_SLEEP,	R_POIS }, //1 (name? poison rule)
{ RT_PLASMA_TIME,		"temporal plasma",	15, 13, 30,  5, 3, GF_PLASMA,		GF_TIME,	R_TIME }, //1 (gestalt explodes w/ Time)
//THUNDER uses augments (could use better names? death tolls, wails, etc? ^^ schematic naming for now..)
{ RT_THUNDER_ACID,		"thunder",		13, 13, 20,  5, 3, GF_SOUND,		0,		R_ACID }, //1
{ RT_THUNDER_CHAOS,		"chaotic thunder",	13, 13, 20,  5, 3, GF_SOUND,		GF_CONFUSION,	R_CHAO }, //1 (exception, chaos shouldn't explode with itself? see others for pattern.)
{ RT_THUNDER_COLD,		"thunder",		13, 13, 20,  5, 3, GF_SOUND,		0,		R_COLD }, //1
{ RT_THUNDER_NETHER,		"corrupted thunder",	13, 13, 20,  5, 3, GF_SOUND,		GF_NETHER,	R_NETH }, //1
{ RT_THUNDER_POISON,		"stunning thunder",	13, 13, 20,  5, 3, GF_SOUND,		GF_STUN,	R_POIS }, //1 (name? poison rule)
//ICE uses augments
{ RT_ICE_ELEC,			"ice",			13, 13, 20,  5, 3, GF_ICE,		0,		R_ELEC }, //1
{ RT_ICE_SHARDS,		"impaling ice",		13, 13, 20,  5, 3, GF_ICE,		GF_SHARDS,	R_EART }, //1 (name, slicing ice -> silly?)
{ RT_ICE_CHAOS,			"shining ice",		13, 13, 20,  5, 3, GF_ICE,		GF_LITE,	R_CHAO }, //1 (exception, chaos does this in place of fire 'brilliance')
{ RT_ICE_POISON,		"slowing ice",		13, 13, 20,  5, 3, GF_ICE,		GF_OLD_SLOW,	R_POIS }, //1 (name? poison rule)
//Water/Cold/Force -> hi_ice
{ RT_HI_ICE,			"crackling ice",	15, 13, 30,  5, 3, GF_ICE,		GF_FORCE,	R_FORC }, //1 (exception, ice gets the full force benefit! name unique)
{ RT_ICE_TIME,			"temporal ice",		15, 13, 30, 10, 3, GF_ICE,		GF_TIME,	R_TIME }, //1

//WAVE and MISSILE
//Water/Earth/force -> hi_force
{ RT_HI_FORCE,			"concussive force",	13, 22, 40, 20, 3, GF_FORCE,		GF_STUN,	R_FORC }, //1 (+40% dmg, +10 cost/level , explodes with stun, power check!)
//WAVE uses augments (but only 3/6! ^^,)
//No fire/cold augment for this due to ice and fire/water
//No elec augment for this due to elec water
{ RT_WAVE_CHAOS,		"pressure",		15, 20, 35, 10, 3, GF_WAVE,		0,		R_CHAO }, //1
{ RT_WAVE_NETHER,		"pressure",		15, 20, 35, 10, 3, GF_WAVE,		0,		R_NETH }, //1
{ RT_WAVE_POISON,		"pressure",		15, 20, 35, 10, 3, GF_WAVE,		0,		R_POIS }, //1
//MISSILE uses explosions
{ RT_MISSILE_ACID,		"acid missiles",	12, 20, 30,  5, 3, GF_MISSILE,		GF_ACID,	0 }, //1
{ RT_MISSILE_FIRE,		"fire missiles",	12, 20, 30,  5, 3, GF_MISSILE,		GF_FIRE,	0 }, //1
{ RT_MISSILE_CHAOS,		"chaos missiles",	12, 20, 30,  5, 3, GF_MISSILE,		GF_CHAOS,	0 }, //1 (the ONLY exploding chaos combination, name change?)
{ RT_MISSILE_COLD,		"frost missiles",	12, 20, 30,  5, 3, GF_MISSILE,		GF_COLD,	0 }, //1
{ RT_MISSILE_NETHER,		"nether missiles",	12, 20, 30,  5, 3, GF_MISSILE,		GF_NETHER,	0 }, //1
{ RT_MISSILE_POISON,		"noxious missiles",	12, 20, 30,  5, 3, GF_MISSILE,		GF_POIS,	0 }, //1

//MANA and DISENCHANT
//DISENCHANT+MANA -> THIS! (pretty special combination, looking at how many patterns support this! ^^ - Kurzel
//(As disenchant/mana are diametricly opposing, and this is a double nexus effect too, let's make it hi_nexus! ..boosted difficulty +5..)
{ RT_HI_NEXUS,			"nexus tendrils",	16, 14, 40, 20, 3, GF_NEXUS,	GF_NEXUS,	R_NEXU }, //1 (chaos/nether/nexus, also explodes w/ nexus, self -> alter reality? ^^,)
//DISENCHANT uses augments
{ RT_DISENCHANT_ACID,		"disenchantment",	12, 12, 30, 15, 3, GF_DISENCHANT,	0,		R_ACID }, //1
{ RT_DISENCHANT_WATER,		"disenchantment",	12, 12, 30, 15, 3, GF_DISENCHANT,	0,		R_WATE }, //1
{ RT_DISENCHANT_ELEC,		"disenchantment",	12, 12, 30, 15, 3, GF_DISENCHANT,	0,		R_ELEC }, //1
{ RT_DISENCHANT_SHARDS,		"disenchantment",	12, 12, 30, 15, 3, GF_DISENCHANT,	0,		R_EART }, //1
{ RT_DISENCHANT_COLD,		"disenchantment",	12, 12, 30, 15, 3, GF_DISENCHANT,	0,		R_COLD }, //1
{ RT_DISENCHANT_FORCE,		"disenchantment",	12, 12, 30, 15, 3, GF_DISENCHANT,	0,		R_FORC }, //1
{ RT_DISENCHANT_TIME,		"disenchantment",	12, 12, 30, 15, 3, GF_DISENCHANT,	0,		R_TIME }, //1
//MANA uses augments
{ RT_MANA_ACID,			"mana",			12, 12, 30, 15, 3, GF_MANA,		0,		R_ACID }, //1
{ RT_MANA_WATER,		"mana",			12, 12, 30, 15, 3, GF_MANA,		0,		R_WATE }, //1
{ RT_MANA_ELEC,			"mana",			12, 12, 30, 15, 3, GF_MANA,		0,		R_ELEC }, //1
{ RT_MANA_SHARDS,		"mana",			12, 12, 30, 15, 3, GF_MANA,		0,		R_EART }, //1
{ RT_MANA_FIRE,			"mana",			12, 12, 30, 15, 3, GF_MANA,		0,		R_FIRE }, //1
{ RT_MANA_FORCE,		"mana",			12, 12, 30, 15, 3, GF_MANA,		0,		R_FORC }, //1
{ RT_MANA_TIME,			"mana",			12, 12, 30, 15, 3, GF_MANA,		0,		R_TIME }, //1

/* Nether/Chaos + water/icePoison use augments, sort this! */
{ RT_WATERPOISON_NETHER,	"corrupted miasma",	16, 12, 30, 10, 3, GF_WATERPOISON,	GF_NETHER,	R_NETH }, //1
{ RT_ICEPOISON_NETHER,		"corrupted frozen miasma",16, 12, 30, 10, 3, GF_ICEPOISON,	GF_NETHER,	R_NETH }, //1

/* Previous Version; Preserved for Comparison */
/*
{ RT_NONE, "nothing", 10, 0, 0, 0, 0, 0 },
{ RT_SHADOW, "shadow", 9, 8, 1, 0, 3, GF_DARK },
{ RT_COLD, "cold", 10, 10, 1, 0, 2, GF_COLD },
{ RT_ACID, "acid", 10, 10, 1, 0, 2, GF_ACID },
{ RT_ELEC, "electricity", 10, 10, 1, 0, 2, GF_ELEC },
{ RT_CLONE, "repetition", 9, 9, 1, 0, 1, GF_OLD_CLONE },
{ RT_FIRE, "fire", 10, 10, 1, 0, 2, GF_FIRE },
{ RT_LIGHT, "light", 5, 10, 1, 0, 3, GF_LITE },
{ RT_SATIATION, "cold", 8, 10, 2, 0, 1, GF_COLD },
{ RT_WIND_BLINK, "wind", 8, 10, 3, 20, 1, GF_SHARDS },
{ RT_DETECTION_BLIND, "blindness", 4, 10, 3, 0, 2, GF_BLIND },
{ RT_POISON, "poison", 8, 10, 3, 5, 2, GF_POIS },
{ RT_SEE_INVISIBLE, "concussion", 1, 10, 3, 5, 1, GF_STUN },
{ RT_FORCE, "force", 10, 11, 5, 15, 1, GF_FORCE },
{ RT_DETECT_TRAP, "concussion", 2, 10, 5, 0, 2, GF_STUN },
{ RT_STASIS_DISARM, "stasis", 8, 10, 5, 5, 1, GF_STASIS },
{ RT_SHARDS, "shards", 10, 12, 5, 10, 2, GF_SHARDS },
{ RT_DARK_SLOW, "mire", 10, 12, 5, 0, 2, GF_OLD_SLOW },
{ RT_BESERK, "confusion", 4, 12, 5, 0, 2, GF_CONFUSION },
{ RT_TELEPORT, "displacement", 10, 14, 10, 0, 1, GF_AWAY_ALL },
{ RT_STUN, "concussion", 2, 14, 10, 0, 2, GF_STUN },
{ RT_HEALING, "stasis", 6, 14, 10, 0, 3, GF_STASIS },
{ RT_MISSILE, "magic missiles", 11, 12, 10, 0, 2, GF_MISSILE },
{ RT_DETECT_STAIR, "blindness", 1, 12, 10, 0, 2, GF_BLIND },
{ RT_DIG, "erosion", 10, 12, 10, 0, 1, GF_KILL_WALL },
{ RT_POLYMORPH, "polymorph", 10, 10, 10, 0, 2, GF_OLD_POLY },
{ RT_FURY, "stasis", 10, 10, 10, 0, 2, GF_STASIS },
{ RT_BRILLIANCE, "brilliance", 8, 12, 12, 0, 1, GF_LITE },
{ RT_OBSCURITY, "obscurity", 12, 12, 12, 0, 1, GF_DARK },
{ RT_WATER, "water", 11, 12, 15, 0, 3, GF_WATER },
{ RT_MANA, "mana", 14, 17, 15, 30, 1, GF_MANA },
{ RT_ANNIHILATION, "stasis", 11, 13, 15, 0, 1, GF_STASIS },
{ RT_PLASMA, "plasma", 13, 13, 15, 0, 2, GF_PLASMA },
{ RT_FLY, "wind", 12, 12, 15, 5, 2, GF_SHARDS },
{ RT_AURA, "hell fire", 12, 12, 15, 0, 2, GF_HELL_FIRE },
{ RT_VISION, "blindness", 10, 15, 18, 0, 1, GF_BLIND },
{ RT_WATERPOISON, "water and poison", 11, 11, 18, 2, 3, GF_WATERPOISON },
{ RT_DISENCHANT, "disenchantment", 11, 12, 20, 0, 2, GF_DISENCHANT },
{ RT_QUICKEN, "mire", 4, 15, 20, 5, 2, GF_OLD_SLOW },
{ RT_ANCHOR, "stasis", 10, 15, 22, 5, 2, GF_STASIS },
{ RT_CHAOS, "chaos", 11, 10, 25, 6, 1, GF_CHAOS },
{ RT_INERTIA, "inertia", 12, 12, 25, 6, 1, GF_INERTIA },
{ RT_NEXUS, "nexus", 11, 11, 25, 6, 2, GF_NEXUS },
{ RT_PSI_ESP, "psi", 11, 12, 25, 8, 1, GF_PSI },
{ RT_GRAVITY, "gravity", 12, 14, 25, 10, 2, GF_GRAVITY },
{ RT_TIME, "time", 13, 15, 25, 10, 1, GF_TIME },
{ RT_MEMORY, "disenchantment", 10, 25, 5, 11, 2, GF_DISENCHANT },
{ RT_SUMMON, "displacment", 12, 12, 25, 0, 2, GF_AWAY_ALL },
{ RT_RESISTANCE, "disenchantment", 10, 15, 25, 10, 2, GF_DISENCHANT },
{ RT_ICE, "ice", 12, 12, 25, 0, 1, GF_ICE },
{ RT_THUNDER, "thunder", 13, 13, 25, 5, 2, GF_SOUND },
{ RT_TELEPORT_TO, "displacement", 18, 10, 26, 8, 2, GF_AWAY_ALL },
{ RT_UNBREATH, "unbreath", 13, 14, 30, 6, 3, GF_UNBREATH },
{ RT_WALLS, "magic missiles", 12, 12, 30, 8, 2, GF_MISSILE },
{ RT_HELL_FIRE, "hell fire", 13, 13, 30, 5, 2, GF_HELL_FIRE },
{ RT_ICEPOISON, "ice and poison", 13, 13, 30, 5, 3, GF_ICEPOISON },
{ RT_DISINTEGRATE, "disintegration", 15, 15, 30, 10, 1, GF_DISINTEGRATE },
{ RT_MYSTIC_SHIELD, "force", 13, 14, 30, 5, 2, GF_FORCE },
{ RT_MAGIC_WARD, "light", 10, 14, 30, 5, 1, GF_LITE },
{ RT_RECALL, "nexus", 13, 14, 30, 5, 2, GF_NEXUS },
{ RT_NETHER, "nether", 10, 14, 30, 5, 2, GF_NETHER },
{ RT_TELEPORT_LEVEL, "displacement", 20, 12, 32, 10, 2, GF_AWAY_ALL },
{ RT_EARTHQUAKE, "meteorites", 14, 14, 35, 15, 3, GF_METEOR },
{ RT_NUKE, "toxine", 13, 16, 40, 10, 3, GF_NUKE },
{ RT_ROCKET, "detonations", 14, 16, 40, 10, 1, GF_ROCKET },
{ RT_MAGIC_CIRCLE, "light", 11, 20, 40, 30, 1, GF_LITE },
*/
};

rspell_sel rspell_selector[MAX_RSPELL_SEL] =
{
{ R_ACID | R_WATE | R_ELEC, RT_DISINTEGRATE_ELEC },
{ R_ACID | R_WATE | R_FIRE, RT_DISINTEGRATE_FIRE },
{ R_ACID | R_WATE | R_COLD, RT_DISINTEGRATE_COLD },
{ R_ACID | R_WATE | R_POIS, RT_DISINTEGRATE_POISON },
{ R_ACID | R_WATE | R_FORC, RT_DISINTEGRATE_FORCE },
{ R_ACID | R_WATE | R_EART, RT_DISINTEGRATE_SHARDS },
{ R_ACID | R_WATE | R_CHAO, RT_DISINTEGRATE_CHAOS },
{ R_ACID | R_WATE | R_NETH, RT_DISINTEGRATE_NETHER },
{ R_ACID | R_WATE | R_NEXU, RT_DISINTEGRATE_NEXUS },
{ R_ACID | R_WATE | R_TIME, RT_DISINTEGRATE_TIME },

{ R_ACID | R_ELEC | R_FIRE, RT_PLASMA_ACID },
{ R_ACID | R_ELEC | R_CHAO, RT_CHAOS_BASE },
{ R_ACID | R_ELEC | R_COLD, RT_WONDER_RESIST },
{ R_ACID | R_ELEC | R_NETH, RT_ACID_ELEC_NETHER },
{ R_ACID | R_ELEC | R_POIS, RT_WONDER_RESIST },
{ R_ACID | R_ELEC | R_NEXU, RT_ACID_ELEC_NEXUS },
{ R_ACID | R_ELEC | R_FORC, RT_THUNDER_ACID },
{ R_ACID | R_ELEC | R_TIME, RT_ACID_ELEC_TIME },

{ R_ACID | R_EART | R_FIRE, RT_FIRE }, //acid + earth combinations are opposed? (for now, yes - similar to elec+water, some work)
{ R_ACID | R_EART | R_CHAO, RT_CHAOS },
{ R_ACID | R_EART | R_COLD, RT_COLD },
{ R_ACID | R_EART | R_NETH, RT_NETHER },
{ R_ACID | R_EART | R_POIS, RT_POISON },
{ R_ACID | R_EART | R_NEXU, RT_ELEC_WATER },
{ R_ACID | R_EART | R_FORC, RT_MISSILE_ACID },
{ R_ACID | R_EART | R_TIME, RT_DISARM_ACID_TIME },

{ R_ACID | R_FIRE | R_COLD, RT_WONDER_RESIST },
{ R_ACID | R_FIRE | R_NETH, RT_ACID_FIRE_NETHER },
{ R_ACID | R_FIRE | R_POIS, RT_WONDER_RESIST },
{ R_ACID | R_FIRE | R_NEXU, RT_ELEC_COLD },
{ R_ACID | R_FIRE | R_FORC, RT_ACID_FIRE_FORCE },
{ R_ACID | R_FIRE | R_TIME, RT_ACID_FIRE_TIME },

{ R_ACID | R_CHAO | R_COLD, RT_CHAOS_BASE },
{ R_ACID | R_CHAO | R_NETH, RT_ELEC },
{ R_ACID | R_CHAO | R_POIS, RT_CHAOS_BASE },
{ R_ACID | R_CHAO | R_NEXU, RT_DISENCHANT_ACID },
{ R_ACID | R_CHAO | R_FORC, RT_GLYPH_LITE_ACID },
{ R_ACID | R_CHAO | R_TIME, RT_ACID_WONDER },

{ R_ACID | R_COLD | R_POIS, RT_WONDER_RESIST },
{ R_ACID | R_COLD | R_NEXU, RT_PLASMA },
{ R_ACID | R_COLD | R_FORC, RT_ACID_COLD_FORCE },
{ R_ACID | R_COLD | R_TIME, RT_ACID_COLD_TIME },

{ R_ACID | R_NETH | R_POIS, RT_ACID_POISON_NETHER },
{ R_ACID | R_NETH | R_NEXU, RT_MANA_ACID },
{ R_ACID | R_NETH | R_FORC, RT_GLYPH_DARK_ACID },
{ R_ACID | R_NETH | R_TIME, RT_ACID },

{ R_ACID | R_POIS | R_FORC, RT_ACID_POISON_FORCE },
{ R_ACID | R_POIS | R_TIME, RT_ACID_POISON_TIME },

{ R_ACID | R_NEXU | R_FORC, RT_THUNDER },
{ R_ACID | R_NEXU | R_TIME, RT_ELEC_TIME },

{ R_WATE | R_ELEC | R_FIRE, RT_PLASMA_WATER },
{ R_WATE | R_ELEC | R_CHAO, RT_CHAOS },
{ R_WATE | R_ELEC | R_COLD, RT_ICE_ELEC },
{ R_WATE | R_ELEC | R_NETH, RT_NETHER },
{ R_WATE | R_ELEC | R_POIS, RT_POISON },
{ R_WATE | R_ELEC | R_NEXU, RT_DISARM_ACID },
{ R_WATE | R_ELEC | R_FORC, RT_ELEC_WATER }, //RT_WAVE+elec will shock you!
{ R_WATE | R_ELEC | R_TIME, RT_TIME },

{ R_WATE | R_EART | R_FIRE, RT_FIRE },
{ R_WATE | R_EART | R_CHAO, RT_CHAOS },
{ R_WATE | R_EART | R_COLD, RT_ICE_SHARDS },
{ R_WATE | R_EART | R_NETH, RT_NETHER }, //self does nothing? heal+anti-heal
{ R_WATE | R_EART | R_POIS, RT_POISON },
{ R_WATE | R_EART | R_NEXU, RT_DIG_NEXUS },
{ R_WATE | R_EART | R_FORC, RT_HI_FORCE },
{ R_WATE | R_EART | R_TIME, RT_TIME },

{ R_WATE | R_FIRE | R_COLD, RT_WATER },
{ R_WATE | R_FIRE | R_NETH, RT_NETHER },
{ R_WATE | R_FIRE | R_POIS, RT_POISON },
{ R_WATE | R_FIRE | R_NEXU, RT_DISARM_COLD },
{ R_WATE | R_FIRE | R_FORC, RT_FORCE },
{ R_WATE | R_FIRE | R_TIME, RT_TIME },

{ R_WATE | R_CHAO | R_COLD, RT_ICE_CHAOS },
{ R_WATE | R_CHAO | R_NETH, RT_SHARDS },
{ R_WATE | R_CHAO | R_POIS, RT_WATERPOISON_CHAOS },
{ R_WATE | R_CHAO | R_NEXU, RT_DISENCHANT_WATER },
{ R_WATE | R_CHAO | R_FORC, RT_WAVE_CHAOS },
{ R_WATE | R_CHAO | R_TIME, RT_WATER_WONDER },

{ R_WATE | R_COLD | R_POIS, RT_ICE_POISON }, //this is NOT == RT_ICEPOISON
{ R_WATE | R_COLD | R_NEXU, RT_DIG_FIRE },
{ R_WATE | R_COLD | R_FORC, RT_HI_ICE },
{ R_WATE | R_COLD | R_TIME, RT_ICE_TIME },

{ R_WATE | R_NETH | R_POIS, RT_NULL }, //something to kill undead? cure+blind+anti-heal (disable atm)
{ R_WATE | R_NETH | R_NEXU, RT_MANA_WATER },
{ R_WATE | R_NETH | R_FORC, RT_WAVE_NETHER },
{ R_WATE | R_NETH | R_TIME, RT_WATER },

{ R_WATE | R_POIS | R_FORC, RT_WAVE_POISON },
{ R_WATE | R_POIS | R_TIME, RT_WATERPOISON_TIME },

{ R_WATE | R_NEXU | R_FORC, RT_MISSILE },
{ R_WATE | R_NEXU | R_TIME, RT_DIG_MEMORY }, //80

{ R_ELEC | R_EART | R_ACID, RT_STARLIGHT_ACID },
{ R_ELEC | R_EART | R_FIRE, RT_STARLIGHT_FIRE },
{ R_ELEC | R_EART | R_COLD, RT_STARLIGHT_COLD },
{ R_ELEC | R_EART | R_POIS, RT_STARLIGHT_POISON },
{ R_ELEC | R_EART | R_FORC, RT_STARLIGHT_FORCE },
{ R_ELEC | R_EART | R_WATE, RT_STARLIGHT_WATER },
{ R_ELEC | R_EART | R_CHAO, RT_STARLIGHT_CHAOS },
{ R_ELEC | R_EART | R_NETH, RT_STARLIGHT_NETHER },
{ R_ELEC | R_EART | R_NEXU, RT_STARLIGHT_NEXUS },
{ R_ELEC | R_EART | R_TIME, RT_STARLIGHT_TIME },

{ R_ELEC | R_FIRE | R_COLD, RT_WONDER_RESIST },
{ R_ELEC | R_FIRE | R_NETH, RT_PLASMA_NETHER },
{ R_ELEC | R_FIRE | R_POIS, RT_PLASMA_POISON },
{ R_ELEC | R_FIRE | R_NEXU, RT_ACID_COLD },
{ R_ELEC | R_FIRE | R_FORC, RT_HI_PLASMA },
{ R_ELEC | R_FIRE | R_TIME, RT_PLASMA_TIME },

{ R_ELEC | R_CHAO | R_COLD, RT_CHAOS_BASE },
{ R_ELEC | R_CHAO | R_NETH, RT_ACID },
{ R_ELEC | R_CHAO | R_POIS, RT_CHAOS_BASE },
{ R_ELEC | R_CHAO | R_NEXU, RT_DISENCHANT_ELEC },
{ R_ELEC | R_CHAO | R_FORC, RT_THUNDER_CHAOS },
{ R_ELEC | R_CHAO | R_TIME, RT_ELEC_WONDER },

{ R_ELEC | R_COLD | R_POIS, RT_WONDER_RESIST },
{ R_ELEC | R_COLD | R_NEXU, RT_ACID_FIRE },
{ R_ELEC | R_COLD | R_FORC, RT_THUNDER_COLD },
{ R_ELEC | R_COLD | R_TIME, RT_ELEC_COLD_TIME },

{ R_ELEC | R_NETH | R_POIS, RT_ELEC_POISON_NETHER },
{ R_ELEC | R_NETH | R_NEXU, RT_MANA_ELEC },
{ R_ELEC | R_NETH | R_FORC, RT_THUNDER_NETHER },
{ R_ELEC | R_NETH | R_TIME, RT_ELEC },

{ R_ELEC | R_POIS | R_FORC, RT_THUNDER_POISON },
{ R_ELEC | R_POIS | R_TIME, RT_ELEC_POISON_TIME },

{ R_ELEC | R_NEXU | R_FORC, RT_HI_ACID },
{ R_ELEC | R_NEXU | R_TIME, RT_ACID_TIME },

{ R_EART | R_FIRE | R_COLD, RT_SHARDS },
{ R_EART | R_FIRE | R_NETH, RT_DARKNESS_SHARDS },
{ R_EART | R_FIRE | R_POIS, RT_ROCKET },
{ R_EART | R_FIRE | R_NEXU, RT_ICE },
{ R_EART | R_FIRE | R_FORC, RT_MISSILE_FIRE },
{ R_EART | R_FIRE | R_TIME, RT_DIG_FIRE_TIME },

{ R_EART | R_CHAO | R_COLD, RT_BRILLIANCE_SHARDS },
{ R_EART | R_CHAO | R_NETH, RT_WATER },
{ R_EART | R_CHAO | R_POIS, RT_ICEPOISON_CHAOS },
{ R_EART | R_CHAO | R_NEXU, RT_DISENCHANT_SHARDS },
{ R_EART | R_CHAO | R_FORC, RT_MISSILE_CHAOS },
{ R_EART | R_CHAO | R_TIME, RT_SHARDS_WONDER },

{ R_EART | R_COLD | R_POIS, RT_NULL }, //nothing at the moment, maybe that vermin control spell?
{ R_EART | R_COLD | R_NEXU, RT_NULL }, //R_WATE | R_FIRE
{ R_EART | R_COLD | R_FORC, RT_MISSILE_COLD },
{ R_EART | R_COLD | R_TIME, RT_DISARM_COLD_TIME },

{ R_EART | R_NETH | R_POIS, RT_NULL }, //nothing at the moment, maybe that vermin control spell?
{ R_EART | R_NETH | R_NEXU, RT_MANA_SHARDS },
{ R_EART | R_NETH | R_FORC, RT_MISSILE_NETHER },
{ R_EART | R_NETH | R_TIME, RT_SHARDS },

{ R_EART | R_POIS | R_FORC, RT_MISSILE_POISON },
{ R_EART | R_POIS | R_TIME, RT_ICEPOISON_TIME },

{ R_EART | R_NEXU | R_FORC, RT_WAVE },
{ R_EART | R_NEXU | R_TIME, RT_DIG_TELEPORT }, //48

{ R_FIRE | R_CHAO | R_ACID, RT_DETONATION_ACID },
{ R_FIRE | R_CHAO | R_ELEC, RT_DETONATION_ELEC },
{ R_FIRE | R_CHAO | R_COLD, RT_DETONATION_COLD },
{ R_FIRE | R_CHAO | R_POIS, RT_DETONATION_POISON },
{ R_FIRE | R_CHAO | R_FORC, RT_DETONATION_FORCE },
{ R_FIRE | R_CHAO | R_WATE, RT_DETONATION_WATER },
{ R_FIRE | R_CHAO | R_EART, RT_DETONATION_SHARDS },
{ R_FIRE | R_CHAO | R_NETH, RT_DETONATION_NETHER },
{ R_FIRE | R_CHAO | R_NEXU, RT_DETONATION_NEXUS },
{ R_FIRE | R_CHAO | R_TIME, RT_DETONATION_TIME },

{ R_FIRE | R_COLD | R_POIS, RT_POISON },
{ R_FIRE | R_COLD | R_NEXU, RT_NEXUS },
{ R_FIRE | R_COLD | R_FORC, RT_FORCE },
{ R_FIRE | R_COLD | R_TIME, RT_TIME },

{ R_FIRE | R_NETH | R_POIS, RT_FIRE_POISON_NETHER },
{ R_FIRE | R_NETH | R_NEXU, RT_MANA_FIRE },
{ R_FIRE | R_NETH | R_FORC, RT_GLYPH_DARK_FIRE },
{ R_FIRE | R_NETH | R_TIME, RT_FIRE },

{ R_FIRE | R_POIS | R_FORC, RT_FIRE_POISON_FORCE },
{ R_FIRE | R_POIS | R_TIME, RT_FIRE_POISON_TIME },

{ R_FIRE | R_NEXU | R_FORC, RT_HI_COLD },
{ R_FIRE | R_NEXU | R_TIME, RT_COLD_TIME },

{ R_CHAO | R_COLD | R_POIS, RT_CHAOS_BASE },
{ R_CHAO | R_COLD | R_NEXU, RT_DISENCHANT_COLD },
{ R_CHAO | R_COLD | R_FORC, RT_GLYPH_LITE_COLD },
{ R_CHAO | R_COLD | R_TIME, RT_COLD_WONDER },

{ R_CHAO | R_NETH | R_POIS, RT_UNBREATH },
{ R_CHAO | R_NETH | R_NEXU, RT_HI_NEXUS },
{ R_CHAO | R_NETH | R_FORC, RT_TELEPORT_NEXUS },
{ R_CHAO | R_NETH | R_TIME, RT_SLEEP },

{ R_CHAO | R_POIS | R_FORC, RT_GLYPH_LITE_POISON },
{ R_CHAO | R_POIS | R_TIME, RT_POISON_WONDER },

{ R_CHAO | R_NEXU | R_FORC, RT_DISENCHANT_FORCE },
{ R_CHAO | R_NEXU | R_TIME, RT_DISENCHANT_TIME }, //24

{ R_COLD | R_NETH | R_ACID, RT_STASIS_ACID },
{ R_COLD | R_NETH | R_ELEC, RT_STASIS_ELEC },
{ R_COLD | R_NETH | R_FIRE, RT_STASIS_FIRE },
{ R_COLD | R_NETH | R_POIS, RT_STASIS_POISON },
{ R_COLD | R_NETH | R_FORC, RT_STASIS_FORCE },
{ R_COLD | R_NETH | R_WATE, RT_STASIS_WATER },
{ R_COLD | R_NETH | R_EART, RT_STASIS_SHARDS },
{ R_COLD | R_NETH | R_CHAO, RT_STASIS_CHAOS },
{ R_COLD | R_NETH | R_NEXU, RT_STASIS_NEXUS },
{ R_COLD | R_NETH | R_TIME, RT_STASIS_TIME },

{ R_COLD | R_POIS | R_FORC, RT_COLD_POISON_FORCE },
{ R_COLD | R_POIS | R_TIME, RT_COLD_POISON_TIME },

{ R_COLD | R_NEXU | R_FORC, RT_HI_FIRE },
{ R_COLD | R_NEXU | R_TIME, RT_FIRE_TIME },

{ R_NETH | R_POIS | R_FORC, RT_GLYPH_DARK_POISON },
{ R_NETH | R_POIS | R_TIME, RT_POISON },

{ R_NETH | R_NEXU | R_FORC, RT_MANA_FORCE },
{ R_NETH | R_NEXU | R_TIME, RT_MANA_TIME }, //8

{ R_POIS | R_NEXU | R_ACID, RT_DRAIN_ACID },
{ R_POIS | R_NEXU | R_ELEC, RT_DRAIN_ELEC },
{ R_POIS | R_NEXU | R_FIRE, RT_DRAIN_FIRE },
{ R_POIS | R_NEXU | R_COLD, RT_DRAIN_COLD },
{ R_POIS | R_NEXU | R_FORC, RT_DRAIN_FORCE },
{ R_POIS | R_NEXU | R_WATE, RT_DRAIN_WATER },
{ R_POIS | R_NEXU | R_EART, RT_DRAIN_SHARDS },
{ R_POIS | R_NEXU | R_CHAO, RT_DRAIN_CHAOS },
{ R_POIS | R_NEXU | R_NETH, RT_DRAIN_NETHER },
{ R_POIS | R_NEXU | R_TIME, RT_DRAIN_TIME },

{ R_FORC | R_TIME | R_ACID, RT_GRAVITY_ACID },
{ R_FORC | R_TIME | R_ELEC, RT_GRAVITY_ELEC },
{ R_FORC | R_TIME | R_FIRE, RT_GRAVITY_FIRE },
{ R_FORC | R_TIME | R_COLD, RT_GRAVITY_COLD },
{ R_FORC | R_TIME | R_POIS, RT_GRAVITY_POISON },
{ R_FORC | R_TIME | R_WATE, RT_GRAVITY_WATER },
{ R_FORC | R_TIME | R_EART, RT_GRAVITY_SHARDS },
{ R_FORC | R_TIME | R_CHAO, RT_GRAVITY_CHAOS },
{ R_FORC | R_TIME | R_NETH, RT_GRAVITY_NETHER },
{ R_FORC | R_TIME | R_NEXU, RT_GRAVITY_NEXUS },

{ R_ACID | R_WATE, RT_POWER },
{ R_ACID | R_ELEC, RT_ACID_ELEC },
{ R_ACID | R_EART, RT_DISARM_ACID },
{ R_ACID | R_FIRE, RT_ACID_FIRE },
{ R_ACID | R_CHAO, RT_NUKE },
{ R_ACID | R_COLD, RT_ACID_COLD },
{ R_ACID | R_NETH, RT_DARKNESS_ACID },
{ R_ACID | R_POIS, RT_ACID_POISON },
{ R_ACID | R_NEXU, RT_ACID_NEXUS },
{ R_ACID | R_FORC, RT_HI_ACID },
{ R_ACID | R_TIME, RT_ACID_TIME },

{ R_WATE | R_ELEC, RT_ELEC_WATER },
{ R_WATE | R_EART, RT_DIG },
{ R_WATE | R_FIRE, RT_NULL },
{ R_WATE | R_CHAO, RT_POLYMORPH },
{ R_WATE | R_COLD, RT_ICE },
{ R_WATE | R_NETH, RT_CLONE },
{ R_WATE | R_POIS, RT_WATERPOISON },
{ R_WATE | R_NEXU, RT_WATER_NEXUS },
{ R_WATE | R_FORC, RT_WAVE },
{ R_WATE | R_TIME, RT_DIG_TELEPORT },

{ R_ELEC | R_EART, RT_HI_ELEC },
{ R_ELEC | R_FIRE, RT_PLASMA },
{ R_ELEC | R_CHAO, RT_BRILLIANCE_ELEC },
{ R_ELEC | R_COLD, RT_ELEC_COLD },
{ R_ELEC | R_NETH, RT_TELEPORT_ELEC },
{ R_ELEC | R_POIS, RT_ELEC_POISON },
{ R_ELEC | R_NEXU, RT_ELEC_NEXUS },
{ R_ELEC | R_FORC, RT_THUNDER },
{ R_ELEC | R_TIME, RT_ELEC_TIME },

{ R_EART | R_FIRE, RT_DIG_FIRE },
{ R_EART | R_CHAO, RT_INFERNO },
{ R_EART | R_COLD, RT_DISARM_COLD },
{ R_EART | R_NETH, RT_GENOCIDE },
{ R_EART | R_POIS, RT_ICEPOISON },
{ R_EART | R_NEXU, RT_EARTH_NEXUS },
{ R_EART | R_FORC, RT_MISSILE },
{ R_EART | R_TIME, RT_DIG_MEMORY },

{ R_FIRE | R_CHAO, RT_HELL_FIRE },
{ R_FIRE | R_COLD, RT_NULL },
{ R_FIRE | R_NETH, RT_DARKNESS_FIRE },
{ R_FIRE | R_POIS, RT_FIRE_POISON },
{ R_FIRE | R_NEXU, RT_FIRE_NEXUS },
{ R_FIRE | R_FORC, RT_HI_FIRE },
{ R_FIRE | R_TIME, RT_FIRE_TIME },

{ R_CHAO | R_COLD, RT_BRILLIANCE_COLD },
{ R_CHAO | R_NETH, RT_NEXUS },
{ R_CHAO | R_POIS, RT_CONFUSION },
{ R_CHAO | R_NEXU, RT_DISENCHANT },
{ R_CHAO | R_FORC, RT_LIGHT },
{ R_CHAO | R_TIME, RT_WONDER },

{ R_COLD | R_NETH, RT_ANNIHILATION },
{ R_COLD | R_POIS, RT_COLD_POISON },
{ R_COLD | R_NEXU, RT_COLD_NEXUS },
{ R_COLD | R_FORC, RT_HI_COLD },
{ R_COLD | R_TIME, RT_COLD_TIME },

{ R_NETH | R_POIS, RT_BLINDNESS },
{ R_NETH | R_NEXU, RT_MANA },
{ R_NETH | R_FORC, RT_SHADOW },
{ R_NETH | R_TIME, RT_NULL },

{ R_POIS | R_NEXU, RT_UNBREATH },
{ R_POIS | R_FORC, RT_STUN },
{ R_POIS | R_TIME, RT_SLOW },

{ R_NEXU | R_FORC, RT_TELEPORT_NEXUS },
{ R_NEXU | R_TIME, RT_SLEEP },

{ R_FORC | R_TIME, RT_INERTIA },

{ R_ACID, RT_ACID },
{ R_ELEC, RT_ELEC },
{ R_FIRE, RT_FIRE },
{ R_COLD, RT_COLD },
{ R_POIS, RT_POISON },
{ R_FORC, RT_FORCE },
{ R_WATE, RT_WATER },
{ R_EART, RT_SHARDS },
{ R_CHAO, RT_CHAOS },
{ R_NETH, RT_NETHER },
{ R_NEXU, RT_NEXUS },
{ R_TIME, RT_TIME },
};

#endif
