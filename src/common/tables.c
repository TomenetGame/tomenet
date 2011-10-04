/* $Id$ */
/* File: tables.c */

/* Purpose: Angband Tables shared by both client and server.
 * originally added for runecraft. - C. Blue */

#include "angband.h"


#ifdef ENABLE_RCRAFT

/* Table of valid runespell elements, their flags, and the sylables for their casting. */
r_element r_elements[RCRAFT_MAX_ELEMENTS] =
{
	{ 0, "Acid", 		"Delibro",	1, SKILL_R_ACIDWATE, R_ACID,},
	{ 1, "Electricity",	"Fulmin", 	1, SKILL_R_ELECEART, R_ELEC,},
	{ 2, "Fire", 		"Aestus", 	1, SKILL_R_FIRECHAO, R_FIRE,},
	{ 3, "Cold", 		"Gelum",		1, SKILL_R_COLDNETH, R_COLD,},
	{ 4, "Poison", 		"Lepis", 		1, SKILL_R_POISNEXU, R_POIS,},
	{ 5, "Force", 		"Fero",		1, SKILL_R_FORCTIME, R_FORC,},
	{ 6, "Water",		"Mio",	 	1, SKILL_R_ACIDWATE, R_WATE,},
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
	{ 0, "minimized",	-3,  5,-10,  6, 10, -1,  5 },
	{ 1, "moderate",	 0, 10,  0, 10, 10,  0, 10 },
	{ 2, "maximized",	 3, 15, 15, 15, 10, +1, 13 },
	{ 3, "compressed",	-2, 15,-10, 13, 10, -2,  8 },
	{ 4, "expanded",	 2, 13,  0, 10, 10, +2, 13 },
	{ 5, "brief",		-1, 13, 10,  6,  5,  0,  5 },
	{ 6, "lengthened",	 1, 15,  5,  6, 10,  0, 16 },
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
	//R_FLAG, +l$, *c%, +f%, *d%, *p%,  r+,*t%
	{ R_ACID,   1,  10,   0,  10,  10,   0, 13}, //30% duration increase
	{ R_ELEC,   1,   7,   0,  10,  10,   0, 10}, //30% cost reduction
	{ R_FIRE,   1,  12,   0,  12,  10,   0, 10}, //20% damage/cost increase
	{ R_COLD,   1,  10, -10,  10,  10,   0, 10}, //10% failure reduction
	{ R_POIS,   1,  10,   0,  10,   8,   0, 10}, //20% energy decrease
	{ R_FORC,  15,  10,   0,  10,  10,   2, 10}, // +2 radius increase
	{ R_WATE,  15,  10,  -5,   8,   8,   0, 10}, //20% energy/damage decrease, 5% failure decrease
	{ R_EART,  15,  11,   0,  11,  10,   0, 11}, //10% damage/cost increase, 10% duration increase
	{ R_CHAO,  25,  14, +20,  14,  10,   0, 10}, //40% damage/cost increase, 20% failure increase
	{ R_NETH,  25,  14,   0,  14,  10,  -2, 10}, //40% damage/cost increase, -2 radius decrease
	{ R_NEXU,  25,  10,   0,  10,  10,   4,  6}, // +4 radius increase, 40% duration decrease
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
	{ 0, R_MELE, "burst",	-1,  4 },
	{ 1, R_SELF, "self",	 0, 10 },
	{ 2, R_BOLT, "bolt",	 0,  6 },
	{ 3, R_BEAM, "beam",	 2, 12 },
	{ 4, R_BALL, "ball",	 5, 18 },
	{ 5, R_WAVE, "wave",	 1, 11 },
	{ 6, R_CLOU, "cloud",	 3, 15 },
	{ 7, R_STOR, "storm",	 4, 13 },
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

/*
	Value Assignment Rules:
	1. Rune-Damage:
		1. In General: (True-Damage)/100 + (Rune-Level/5) [Round Up]
		2. Base elements (and plasma) with a HI version have a -2 dam in the low version. Other HI elements (eg. wave/starlight/nuke) gain a +2 dam in the high version.
		3. Composite elements that retain their contributing element resistances (eg. ice/unbreath/hellfire) deal the larger damage of their contribiting elements, before level mod.
		4. Elements with multiple immunities (eg. gestalts) gain +1 dam.
		5. Elements with routine susceptabilities (eg. lite) get -1 dam, elements with routine resistances (eg. dark) do double this damage -2.
		6. Elements (eg. dissolution/disintegration) with no non-unique resistances have a -3 dam.
		7. Utility-projection elements (eg. disarm_acid) deal 50% damage, or effect specific (eg. slow/drain/genocide) have their own values.
	2. Rune-Cost:
		1. In General: (Damage-10)/2 + 10 [Round Towards 10] (eg. 9/10/11 dam -> 10 cost)
		2. Utility-projection elements (eg. disarm_acid) and effect specific (eg. slow/drain/genocide) have their own values.
		3. Self-spells modify this.
	3. Rune-Levels:
		1. In General: Access levels compare below.
		2. Composite elements take the highest level of their contributing elements.
		3. Chaos/Nether average the level of the contributing elements.
		4. Time takes the lower level of contributing elements.
		5. Some weak-utility (eg. lite/shadow) take the lowest (True-level = 5).
		
		Level  5: +1 dam
		--------
		Base [Fire/Frost/Lightning/Acid]: 1200
		Poison: 800 (poison)
		Lite: 400 (blind)
		Dark: 400 (blind)
		
		Level 10: +2 dam
		--------
		Confusion: 400 (confusion)
		
		Level 15: +3 dam
		--------
		Shard: 400 (cut)
		Water: ??? -> 200 (stun/confusion)
		Nexus: 250 (warp)
		
		Level 20: +4 dam
		--------
		Force: 200 (stun)
		Plasma: 150 (stun)
		Ice: ??? -> Plasma (stun/cut)
		Sound: 400 (stun)
		
		Level 25: +5 dam
		--------
		Inertia: 200
		
		Level 30: +6 dam
		--------
		Disenchantment: 500
		Mana: 250 -> 500
		Power: 500
		
		Level 35: +7 dam
		--------
		Nether: 550
		Chaos: 600
		Gravity: 150
		
		Level 40: +8 dam
		--------
		Time: 150
		Disintegration : 300
		Rocket/detonations: 800
	4. Rune-Fail
		1. 1-rune -> 0
		2. 2-rune -> 5
		3. 3-rune -> 10
		4. Self-spells modify this.
	5. Rune-Difficulty
		1. This generally equals the # of runes.
*/

r_spell runespell_list[RT_MAX] =
{
{ RT_NONE,		"nothing",	10,  0,  0,  0, 0, 0,			0,	0 }, //0
{ RT_ACID,		"acid",		11, 10,  5,  0, 1, GF_ACID,		0,	0 }, //3
{ RT_ELEC,		"lightning",	11, 10,  5,  0, 1, GF_ELEC,		0,	0 }, //3
{ RT_FIRE,		"fire",		11, 10,  5,  0, 1, GF_FIRE,		0,	0 }, //4
{ RT_COLD,		"frost",	11, 10,  5,  0, 1, GF_COLD,		0,	0 }, //2
{ RT_POISON,		"gas",		 9, 10,  5,  0, 1, GF_POIS,		0,	0 }, //6
{ RT_FORCE,		"force",	 6,  8, 20,  0, 1, GF_FORCE,		0,	0 }, //3
{ RT_WATER,		"water",	 5,  8, 15,  0, 1, GF_WATER,		0,	0 }, //4
{ RT_SHARDS,		"shards",	 7,  9, 15,  0, 1, GF_SHARDS,		0,	0 }, //4
{ RT_CHAOS,		"chaos",	13, 11, 35,  0, 1, GF_CHAOS,		0,	0 }, //4
{ RT_NETHER,		"nether",	13, 11, 35,  0, 1, GF_NETHER,		0,	0 }, //4
{ RT_NEXUS,		"nexus",	 6,  8, 15,  0, 1, GF_NEXUS,		0,	0 }, //3
{ RT_TIME,		"time",		10, 10, 40,  0, 1, GF_TIME,		0,	0 }, //5

{ RT_POWER,			"dissolution",		 8,  9, 30,  5, 2, GF_DISP_ALL,		0,	0 }, //1
{ RT_DISINTEGRATE_ELEC,		"disintegration",	 8,  9, 40, 10, 3, GF_DISINTEGRATE,	0,	R_ELEC }, //1
{ RT_DISINTEGRATE_SHARDS,	"disintegration",	 8,  9, 40, 10, 3, GF_DISINTEGRATE,	0,	R_EART }, //1
{ RT_DISINTEGRATE_FIRE,		"disintegration",	 8,  9, 40, 10, 3, GF_DISINTEGRATE,	0,	R_FIRE }, //1
{ RT_DISINTEGRATE_CHAOS,	"disintegration",	 8,  9, 40, 10, 3, GF_DISINTEGRATE,	0,	R_CHAO }, //1
{ RT_DISINTEGRATE_COLD,		"disintegration",	 8,  9, 40, 10, 3, GF_DISINTEGRATE,	0,	R_COLD }, //1
{ RT_DISINTEGRATE_NETHER,	"disintegration",	 8,  9, 40, 10, 3, GF_DISINTEGRATE,	0,	R_NETH }, //1
{ RT_DISINTEGRATE_POISON,	"disintegration",	 8,  9, 40, 10, 3, GF_DISINTEGRATE,	0,	R_POIS }, //1
{ RT_DISINTEGRATE_NEXUS,	"disintegration",	 8,  9, 40, 10, 3, GF_DISINTEGRATE,	0,	R_NEXU }, //1
{ RT_DISINTEGRATE_FORCE,	"disintegration",	 8,  9, 40, 10, 3, GF_DISINTEGRATE,	0,	R_FORC }, //1
{ RT_DISINTEGRATE_TIME,		"disintegration",	 8,  9, 40, 10, 3, GF_DISINTEGRATE,	0,	R_TIME }, //1

{ RT_HI_ELEC,		"charge",	15, 12, 15,  5, 2, GF_ELEC,		GF_ELEC,	R_ELEC }, //1
{ RT_STARLIGHT_ACID,	"starlight",	13, 11, 35, 10, 3, GF_LITE,		GF_LITE,	R_ACID }, //1
{ RT_STARLIGHT_WATER,	"starlight",	13, 11, 35, 10, 3, GF_LITE,		GF_LITE,	R_WATE }, //1
{ RT_STARLIGHT_FIRE,	"starlight",	13, 11, 35, 10, 3, GF_LITE,		GF_LITE,	R_FIRE }, //1
{ RT_STARLIGHT_CHAOS,	"starlight",	13, 11, 35, 10, 3, GF_LITE,		GF_LITE,	R_CHAO }, //1
{ RT_STARLIGHT_COLD,	"starlight",	13, 11, 35, 10, 3, GF_LITE,		GF_LITE,	R_COLD }, //1
{ RT_STARLIGHT_NETHER,	"starlight",	13, 11, 35, 10, 3, GF_LITE,		GF_LITE,	R_NETH }, //1
{ RT_STARLIGHT_POISON,	"starlight",	13, 11, 35, 10, 3, GF_LITE,		GF_LITE,	R_POIS }, //1
{ RT_STARLIGHT_NEXUS,	"starlight",	13, 11, 35, 10, 3, GF_LITE,		GF_LITE,	R_NEXU }, //1
{ RT_STARLIGHT_FORCE,	"starlight",	13, 11, 35, 10, 3, GF_LITE,		GF_LITE,	R_FORC }, //1
{ RT_STARLIGHT_TIME,	"starlight",	13, 11, 35, 10, 3, GF_LITE,		GF_LITE,	R_TIME }, //1

{ RT_HELL_FIRE,		"hellfire",	17, 13, 35,  5, 2, GF_HELL_FIRE,	0,	0 }, //1
{ RT_DETONATION_ACID,	"detonations",	16, 13, 40, 10, 3, GF_DETONATION,	0,	R_ACID }, //1
{ RT_DETONATION_WATER,	"detonations",	16, 13, 40, 10, 3, GF_DETONATION,	0,	R_WATE }, //1
{ RT_DETONATION_ELEC,	"detonations",	16, 13, 40, 10, 3, GF_DETONATION,	0,	R_ELEC }, //1
{ RT_DETONATION_SHARDS,	"detonations",	16, 13, 40, 10, 3, GF_DETONATION,	0,	R_EART }, //1
{ RT_DETONATION_COLD,	"detonations",	16, 13, 40, 10, 3, GF_DETONATION,	0,	R_COLD }, //1
{ RT_DETONATION_NETHER,	"detonations",	16, 13, 40, 10, 3, GF_DETONATION,	0,	R_NETH }, //1
{ RT_DETONATION_POISON,	"detonations",	16, 13, 40, 10, 3, GF_DETONATION,	0,	R_POIS }, //1
{ RT_DETONATION_NEXUS,	"detonations",	16, 13, 40, 10, 3, GF_DETONATION,	0,	R_NEXU }, //1
{ RT_DETONATION_FORCE,	"detonations",	16, 13, 40, 10, 3, GF_DETONATION,	0,	R_FORC }, //1
{ RT_DETONATION_TIME,	"detonations",	16, 13, 40, 10, 3, GF_DETONATION,	0,	R_TIME }, //1

{ RT_ANNIHILATION,	"annihilation",	 4, 10, 30,  5, 2, GF_ANNIHILATION,	0,	0 }, //1
{ RT_STASIS_ACID,	"stasis",	10, 10, 35, 10, 3, GF_STASIS,		0,	R_ACID }, //1
{ RT_STASIS_WATER,	"stasis",	10, 10, 35, 10, 3, GF_STASIS,		0,	R_WATE }, //1
{ RT_STASIS_ELEC,	"stasis",	10, 10, 35, 10, 3, GF_STASIS,		0,	R_ELEC }, //1
{ RT_STASIS_SHARDS,	"stasis",	10, 10, 35, 10, 3, GF_STASIS,		0,	R_EART }, //1
{ RT_STASIS_FIRE,	"stasis",	10, 10, 35, 10, 3, GF_STASIS,		0,	R_FIRE }, //1
{ RT_STASIS_CHAOS,	"stasis",	10, 10, 35, 10, 3, GF_STASIS,		0,	R_CHAO }, //1
{ RT_STASIS_POISON,	"stasis",	10, 10, 35, 10, 3, GF_STASIS,		0,	R_POIS }, //1
{ RT_STASIS_NEXUS,	"stasis",	10, 10, 35, 10, 3, GF_STASIS,		0,	R_NEXU }, //1
{ RT_STASIS_FORCE,	"stasis",	10, 10, 35, 10, 3, GF_STASIS,		0,	R_FORC }, //1
{ RT_STASIS_TIME,	"stasis",	10, 10, 35, 10, 3, GF_STASIS,		0,	R_TIME }, //1

{ RT_UNBREATH,		"unbreath",	13, 11, 15,  5, 2, GF_UNBREATH,		0,	0 }, //2
{ RT_DRAIN_ACID,	"drain",	 2,  0, 35, 10, 3, GF_OLD_DRAIN,	0,	R_ACID }, //1 (lifesteal fix)
{ RT_DRAIN_WATER,	"drain",	 2, 10, 35, 10, 3, GF_OLD_DRAIN,	0,	R_WATE }, //1
{ RT_DRAIN_ELEC,	"drain",	 2, 10, 35, 10, 3, GF_OLD_DRAIN,	0,	R_ELEC }, //1
{ RT_DRAIN_SHARDS,	"drain",	 2, 10, 35, 10, 3, GF_OLD_DRAIN,	0,	R_EART }, //1
{ RT_DRAIN_FIRE,	"drain",	 2, 10, 35, 10, 3, GF_OLD_DRAIN,	0,	R_FIRE }, //1
{ RT_DRAIN_CHAOS,	"drain",	 2, 10, 35, 10, 3, GF_OLD_DRAIN,	0,	R_CHAO }, //1
{ RT_DRAIN_COLD,	"drain",	 2, 10, 35, 10, 3, GF_OLD_DRAIN,	0,	R_COLD }, //1
{ RT_DRAIN_NETHER,	"drain",	 2, 10, 35, 10, 3, GF_OLD_DRAIN,	0,	R_NETH }, //1
{ RT_DRAIN_FORCE,	"drain",	 2, 10, 35, 10, 3, GF_OLD_DRAIN,	0,	R_FORC }, //1
{ RT_DRAIN_TIME,	"drain",	 2, 10, 35, 10, 3, GF_OLD_DRAIN,	0,	R_TIME }, //1

{ RT_INERTIA,		"inertia",	 7,  9, 30,  5, 2, GF_INERTIA,		0,	0 }, //1
{ RT_GRAVITY_ACID,	"gravity",	 9, 10, 35, 10, 3, GF_GRAVITY,		0,	R_ACID }, //1
{ RT_GRAVITY_WATER,	"gravity",	 9, 10, 35, 10, 3, GF_GRAVITY,		0,	R_WATE }, //1
{ RT_GRAVITY_ELEC,	"gravity",	 9, 10, 35, 10, 3, GF_GRAVITY,		0,	R_ELEC }, //1
{ RT_GRAVITY_SHARDS,	"gravity",	 9, 10, 35, 10, 3, GF_GRAVITY,		0,	R_EART }, //1
{ RT_GRAVITY_FIRE,	"gravity",	 9, 10, 35, 10, 3, GF_GRAVITY,		0,	R_FIRE }, //1
{ RT_GRAVITY_CHAOS,	"gravity",	 9, 10, 35, 10, 3, GF_GRAVITY,		0,	R_CHAO }, //1
{ RT_GRAVITY_COLD,	"gravity",	 9, 10, 35, 10, 3, GF_GRAVITY,		0,	R_COLD }, //1
{ RT_GRAVITY_NETHER,	"gravity",	 9, 10, 35, 10, 3, GF_GRAVITY,		0,	R_NETH }, //1
{ RT_GRAVITY_POISON,	"gravity",	 9, 10, 35, 10, 3, GF_GRAVITY,		0,	R_POIS }, //1
{ RT_GRAVITY_NEXUS,	"gravity",	 9, 10, 35, 10, 3, GF_GRAVITY,		0,	R_NEXU }, //1

/* Gestalts (names should be somewhat mystic/arcane/esoteric, but also scientific) - Kurzel */
{ RT_ACID_ELEC,		"ionization",		12, 11,  5,  5, 2, GF_ACID_ELEC,	0,		0 }, //1
{ RT_ACID_FIRE,		"bile",			12, 11,  5,  5, 2, GF_ACID_FIRE,	0,		0 }, //2
{ RT_ACID_COLD,		"rime",			12, 11,  5,  5, 2, GF_ACID_COLD,	0,		0 }, //2
{ RT_ACID_POISON,	"venom",		12, 11,  5,  5, 2, GF_ACID_POISON,	GF_BLIND,	0 }, //1
{ RT_ACID_TIME,		"temporal acid",	11, 10,  5,  5, 2, GF_ACID,		GF_TIME,	R_TIME }, //2
{ RT_PLASMA,		"plasma",		14, 10, 20,  5, 2, GF_PLASMA,		0,		0 }, //2 (RT_ELEC_FIRE)
{ RT_ELEC_COLD,		"static",		12, 11,  5,  5, 2, GF_ELEC_COLD,	0,		0 }, //2
{ RT_ELEC_POISON,	"jolting",		12, 11,  5,  5, 2, GF_ELEC_POISON,	GF_STUN,	0 }, //1
{ RT_ELEC_TIME,		"temporal lightning",	11, 10,  5,  5, 2, GF_ELEC,		GF_TIME,	R_TIME }, //2
{ RT_NULL,		"nothing",		 0,  0,  0,  5, 2, 0,			0,		0 }, //5 /* RT_FIRE_COLD - defines RT_NULL - IMPORTANT! */
{ RT_FIRE_POISON,		"consumption",	12, 11,  5,  5, 2, GF_FIRE_POISON,	GF_OLD_SLEEP,	0 }, //1 (poison effect rule -> fever-fainting?)
{ RT_FIRE_TIME,		"temporal fire",	11, 10,  5,  5, 2, GF_FIRE,		GF_TIME,	R_TIME }, //2
{ RT_COLD_POISON,	"hypothermia",		12, 11,  5,  5, 2, GF_COLD_POISON,	GF_OLD_SLOW,	0 }, //1
{ RT_COLD_TIME,		"temporal frost",	11, 10,  5,  5, 2, GF_COLD,		GF_TIME,	R_TIME }, //2
{ RT_SLOW,		"mire",			10, 10,  5,  5, 2, GF_OLD_SLOW,		0,		0 }, //1 (RT_POISON_TIME)

/* Acid */
{ RT_DISARM_ACID,	"corrosion",		 5,  8, 15,  5, 2, GF_CORRODE,		0,		0 }, //2
{ RT_NUKE,		"toxine",		17, 13, 35,  5, 2, GF_NUKE,		GF_POIS,	R_POIS }, //1
{ RT_DARKNESS_ACID,	"hungry darkness",	12, 11, 20,  5, 2, GF_DARK,		GF_ACID,	0 }, //1
{ RT_HI_ACID,		"ablation",		16, 13, 20,  5, 2, GF_ACID,		GF_ACID,	R_ACID }, //2
{ RT_ACID_NEXUS,	"reduction",		13, 11, 10,  5, 2, GF_ELEC,		0,		R_NEXU }, //1
/* Electricity */
{ RT_ELEC_WATER,	"conducting water",	13, 11, 10,  5, 2, GF_ELEC,		GF_WATER,	R_ELEC }, //3 
{ RT_BRILLIANCE_ELEC,	"shining brilliance",	7,  9, 20,  5, 2, GF_LITE,		GF_ELEC,	0 }, //1
{ RT_TELEPORT_ELEC,	"displacement",		14, 12, 20,  5, 2, GF_AWAY_ALL,		GF_ELEC,	0 }, //1
{ RT_THUNDER,		"thunder",		14, 12, 20,  5, 2, GF_SOUND,		0,		0 }, //2
{ RT_ELEC_NEXUS,	"oxidation",		13, 11, 10,  5, 2, GF_ACID,		0,		R_NEXU }, //1
/* Fire */
{ RT_FIRE_WATER,	"steam",		 0,  0,  0,  5, 2, 0,			0,		0 }, //0 (this might do something for the self spell ^^)
{ RT_DIG_FIRE,		"eroding heat",		10,  5, 15,  5, 2, GF_DIG_FIRE,		0,		0 }, //2
{ RT_DARKNESS_FIRE,	"burning darkness",	12, 11, 20,  5, 2, GF_DARK,		GF_FIRE,	0 }, //1
{ RT_HI_FIRE,		"blazing fire",		16, 13, 20,  5, 2, GF_FIRE,		GF_FIRE,	R_FIRE }, //2
{ RT_FIRE_NEXUS,	"endothermy",		13, 11, 10,  5, 2, GF_COLD,		0,		R_NEXU }, //1
/* Cold */
{ RT_ICE,		"ice",			13, 11, 20,  5, 2, GF_ICE,		0,		0 }, //2
{ RT_DISARM_COLD,	"shattering",		 5,  8, 15,  5, 2, GF_SHATTER,		0,		0 }, //2
{ RT_BRILLIANCE_COLD,	"grim brilliance",	 7,  9, 20,  5, 2, GF_LITE,		GF_COLD,	0 }, //1
{ RT_HI_COLD,		"wicking frost",	16, 13, 20,  5, 2, GF_COLD,		GF_COLD,	R_COLD }, //2
{ RT_COLD_NEXUS,		"exothermy",	13, 11, 10,  5, 2, GF_FIRE,		0,		R_NEXU }, //1
/* Poison */
{ RT_WATERPOISON,	"miasma",		11, 10, 15,  5, 2, GF_WATERPOISON,	0,		0 }, //1
{ RT_ICEPOISON,		"nettles",		11, 10, 15,  5, 2, GF_ICEPOISON,	0,		0 }, //1
{ RT_CONFUSION,		"confusion",		 8,  9, 20,  5, 2, GF_CONFUSION,	0,		0 }, //1 (should deal damage, AND apply effect! (NOT psi!) >.> GWoP)
{ RT_BLINDNESS,		"blindness",		10, 10, 10,  5, 2, GF_BLIND,		0,		0 }, //1
{ RT_STUN,		"concussion",		12, 11, 20,  5, 2, GF_STUN,		0,		0 }, //1 (stun based on dmg?)
/* Force */
{ RT_WAVE,		"pressure",		 8,  9, 20,  5, 2, GF_WAVE,		0,		0 }, //2
{ RT_MISSILE,		"magic missiles",	 4,  7, 20,  5, 2, GF_MISSILE,		0,		0 }, //2
{ RT_LIGHT,		"light",		 4,  7,  5,  5, 2, GF_LITE,		0,		0 }, //1
{ RT_SHADOW,		"shadow",		 6,  8,  5,  5, 2, GF_DARK,		0,		0 }, //1
{ RT_TELEPORT_NEXUS,	"displacement",		14, 12, 20,  5, 2, GF_AWAY_ALL,	GF_NEXUS,		0 }, //2 (dam should be 7? keep equal to elec though for main projection)
/* Water */
{ RT_DIG,		"eroding water",	10, 10, 15,  5, 2, GF_KILL_WALL,	0,		0 }, //1
{ RT_POLYMORPH,		"transformation",	10, 10, 25,  5, 2, GF_OLD_POLY,		0,		0 }, //1
{ RT_CLONE,		"duplication",		10, 10, 25,  5, 2, GF_OLD_CLONE,	0,		0 },  //1
{ RT_WATER_NEXUS,	"crystal",		 7,  9, 15,  5, 2, GF_SHARDS,		0,		R_NEXU }, //1
{ RT_DIG_TELEPORT,	"temporal erosion",	10, 10, 15,  5, 2, GF_KILL_WALL,	0,		R_TIME }, //2
/* Earth */
{ RT_INFERNO,		"raging fire",		13, 11, 25,  5, 2, GF_INFERNO,		0,		0 }, //1 (naming needs change?)
{ RT_GENOCIDE,		"nothing",		 1, 10, 30,  5, 2, 0,			0,		0 }, //1 (GF_GENOCIDE - Add effect! 'nothing' for now)
{ RT_DIG_MEMORY,	"temporal erosion",	10, 10, 15,  5, 2, GF_KILL_WALL,	0,		R_TIME }, //2
{ RT_EARTH_NEXUS,	"mud",			 5,  8, 15,  5, 2, GF_WATER,		0,		R_NEXU }, //1
/* Chaos */
{ RT_CHAOS_NETHER,	"artificial nexus",	10, 10, 35,  5, 2, GF_NEXUS,		0,		0 }, //0
{ RT_DISENCHANT,	"disenchantment",	11, 10, 30,  5, 2, GF_DISENCHANT,	0,		0 }, //1
{ RT_WONDER,		"wonder",		13, 11, 35,  5, 2, GF_WONDER,		0,		0 }, //1
/* Nether */
{ RT_MANA,		"mana",			11, 10, 30,  5, 2, GF_MANA,		0,		0 }, //1
{ RT_NETHER_TIME,	"oblivion",		 9, 10, 35,  5, 2, GF_TIME,		GF_ANNIHILATION,	0 }, //1
/* Time */
{ RT_SLEEP,		"slumber",		10, 10, 15,  5, 2, GF_OLD_SLEEP,	0,		0 }, //2

//New 3-rune (triple school) RT_EFFECTs here! Don't forget to organize by index... - Kurzel
//Minimum effect level is the maximum level of composition runes
//Minimum failure malus is usually the maximum penalty of composistion runes
//Damage is boosted more for low runes, low runes usually take precidence in combination
//Powerful combination elements always 'augment' rather than explode, but may do both

//RESISTANCE self utility (should be removed, more 'unique' spells or families should be added)
{ RT_WONDER_RESIST,	"elements",		11, 10,  5, 10, 3, GF_BASE,		0,		0 }, //8

//GLYPH self utility combinations, use light/dark+explosion to project, elec/earth school absent due to starlite/self -> circle (minimized circle should be a single glyph!)
//Level fixed to 30, starlite 35 (for usability =.=) +20% damage to correct level malus (balance, will be /weaker/ than brilliance/darkness)
//Descriptors listed as light and shadow, rather than brilliance / darkness, as above; this helps w/ identification
{ RT_GLYPH_LITE_ACID,	"hungry light",		 9, 10, 30, 10, 3, GF_LITE,		GF_ACID,	0 }, //1
{ RT_GLYPH_LITE_COLD,	"grim brilliance",	 9, 10, 30, 10, 3, GF_LITE,		GF_COLD,	0 }, //1
{ RT_GLYPH_LITE_POISON,	"radiation",		 9, 10, 30, 10, 3, GF_LITE,		GF_NUKE,	0 }, //1
{ RT_GLYPH_DARK_ACID,	"hungry darkness",	16, 13, 30, 10, 3, GF_DARK,		GF_ACID,	0 }, //1
{ RT_GLYPH_DARK_FIRE,	"burning darkness",	16, 13, 30, 10, 3, GF_DARK,		GF_FIRE,	0 }, //1
{ RT_GLYPH_DARK_POISON,	"nightmares",		16, 13, 30, 10, 3, GF_DARK,		GF_TURN_ALL,	0 }, //1

//LITE/DARK + SHARDS , balanced at loss of fire+water combinations
{ RT_BRILLIANCE_SHARDS,"slicing brilliance",	 8,  9, 25, 10, 3, GF_LITE,		GF_SHARDS,	0 }, //1
{ RT_DARKNESS_SHARDS,	"slicing darkness",	14, 12, 25, 10, 3, GF_DARK,		GF_SHARDS,	0 }, //2

//R_CHAO dominates base elements.
{ RT_CHAOS_BASE,	"limbic chaos",		11, 10, 20, 10, 3, GF_CHAOS,		GF_BASE,	0 }, //6

//add R_TIME to get time augment, wonder explosion
{ RT_ACID_WONDER,	"enchanted acid",	14, 12, 20, 10, 3, GF_ACID,		GF_WONDER,	R_TIME }, //1
{ RT_ELEC_WONDER,	"enchanted lightning",	14, 12, 20, 10, 3, GF_ELEC,		GF_WONDER,	R_TIME }, //1
{ RT_COLD_WONDER,	"enchanted frost",	14, 12, 20, 10, 3, GF_COLD,		GF_WONDER,	R_TIME }, //1
{ RT_POISON_WONDER,	"enchanted gas",	12, 11, 20, 10, 3, GF_POIS,		GF_WONDER,	R_TIME }, //1

{ RT_WATER_WONDER,	"enchanted water",	 7,  9, 25, 10, 3, GF_WATER,		GF_WONDER,	R_TIME }, //1
{ RT_SHARDS_WONDER,	"enchanted shards",	 9, 10, 25, 10, 3, GF_SHARDS,		GF_WONDER,	R_TIME }, //1

//also augment these special ones, sort w/ nether
{ RT_WATERPOISON_CHAOS,	"addling miasma",	13, 11, 25, 10, 3, GF_WATERPOISON,	GF_CONFUSION,	R_CHAO }, //1
{ RT_ICEPOISON_CHAOS,	"addling nettles",	13, 11, 25, 10, 3, GF_ICEPOISON,	GF_CONFUSION,	R_CHAO }, //1
//water/icepoison_neth at end for now, sort this!
//new water/icepoison_neth here!

//R_NETH generally explodes
{ RT_ACID_ELEC_NETHER,	"corrupted ionization",	16, 13, 25, 10, 3, GF_ACID_ELEC,	GF_NETHER,	0 }, //1
{ RT_ACID_FIRE_NETHER,	"corrupted bile",	16, 13, 25, 10, 3, GF_ACID_FIRE,	GF_NETHER,	0 }, //1
{ RT_ACID_POISON_NETHER,"corrupted venom",	14, 12, 25, 10, 3, GF_ACID_POISON,	GF_BLIND,	0 }, //1
{ RT_ELEC_POISON_NETHER,"corrupted jolting",	14, 12, 25, 10, 3, GF_ELEC_POISON,	GF_STUN,	0 }, //1
{ RT_FIRE_POISON_NETHER,"corrupted consumption",14, 12, 25, 10, 3, GF_FIRE_POISON,	GF_OLD_SLEEP,	0 }, //1

//R_FORC generally explodes with force
{ RT_ACID_FIRE_FORCE,	"forceful bile",	15, 12, 20, 10, 3, GF_ACID_FIRE,	GF_FORCE,	0 }, //1
{ RT_ACID_COLD_FORCE,	"forceful rime",	15, 12, 20, 10, 3, GF_ACID_COLD,	GF_FORCE,	0 }, //1
//These could be improved upon...conflict w/ explosion elements.
{ RT_ACID_POISON_FORCE,	"forceful venom",	13, 11, 20, 10, 3, GF_ACID_POISON,	GF_BLIND,	0 }, //1
{ RT_FIRE_POISON_FORCE,	"forceful consumption",	13, 11, 20, 10, 3, GF_FIRE_POISON,	GF_OLD_SLEEP,	0 }, //1
{ RT_COLD_POISON_FORCE,	"forceful hypothermia",	13, 11, 20, 10, 3, GF_COLD_POISON,	GF_OLD_SLOW,	0 }, //1

//R_TIME should almost *always* augment AND explode with time
{ RT_ACID_ELEC_TIME,	"temporal ionization",	12, 11,  5, 10, 3, GF_ACID_ELEC,	GF_TIME,	R_TIME }, //1
{ RT_ACID_FIRE_TIME,	"temporal bile",	12, 11,  5, 10, 3, GF_ACID_FIRE,	GF_TIME,	R_TIME }, //1
{ RT_ACID_COLD_TIME,	"temporal rime",	12, 11,  5, 10, 3, GF_ACID_COLD,	GF_TIME,	R_TIME }, //1
{ RT_ACID_POISON_TIME,	"temporal venom",	10, 10,  5, 10, 3, GF_ACID_POISON,	GF_BLIND,	R_TIME }, //1
{ RT_ELEC_COLD_TIME,	"temporal static",	12, 11,  5, 10, 3, GF_ELEC_COLD,	GF_TIME,	R_TIME }, //1
{ RT_ELEC_POISON_TIME,	"temporal jolting",	10, 10,  5, 10, 3, GF_ELEC_POISON,	GF_STUN,	R_TIME }, //1
{ RT_FIRE_POISON_TIME,	"temporal consumption",	10, 10,  5, 10, 3, GF_FIRE_POISON,	GF_OLD_SLEEP,	R_TIME }, //1
{ RT_COLD_POISON_TIME,	"temporal hypothermia",	10, 10,  5, 10, 3, GF_COLD_POISON,	GF_OLD_SLOW,	R_TIME }, //1

/* sort all these water/icePoison things */
{ RT_WATERPOISON_TIME,	"temporal miasma",	11, 10, 15, 10, 3, GF_WATERPOISON,	GF_TIME,	R_TIME }, //1
{ RT_ICEPOISON_TIME,	"temporal nettles",	11, 10, 15, 10, 3, GF_ICEPOISON,	GF_TIME,	R_TIME }, //1
/* sort this! keep in-order to avoid bugs! */
{ RT_ROCKET,		"mini-rockets",		11, 10, 15, 10, 3, GF_ROCKET,		0,		0 }, //1

{ RT_DIG_FIRE_TIME,	"temporal eroding heat",10, 10, 15, 10, 3, GF_DIG_FIRE,		0,		R_TIME }, //1
/* power check here and normal ones too! */
{ RT_DISARM_COLD_TIME,	"temporal shattering",	 5,  8, 15, 10, 3, GF_SHATTER,		0,		R_TIME }, //1
{ RT_DISARM_ACID_TIME,	"temporal corrosion",	 5,  8, 15, 10, 3, GF_CORRODE,		0,		R_TIME }, //1

//R_NEXU adds nexus burst, damage, to diametrically opposing base runes
//Only 2 due to R_FIRE | R_COLD -> RT_NULL
{ RT_ACID_ELEC_NEXUS,	"warped ionization",	14, 12, 15, 10, 3, GF_ACID_ELEC,	GF_NEXUS,	0 }, //1
{ RT_DIG_NEXUS,		"warped erosion",	10, 10, 15, 10, 3, GF_NEXUS,		GF_KILL_WALL,	0 }, //1

//The 'Trinity' of PLASMA, THUNDER, ICE, use augments
//Also explode with 'powerful' elements (2nd elements from base schools)
//Exclude force explicitly! (nvm this for GF_ICE, for now..)
//Note that nexus doesn't augment these, but allows access from other schools
//Exceptions:
//Plasma/ice has no cold/fire augment due to R_FIRE | R_COLD -> RT_NULL
//Thunder has no water augment due to R_ELEC | R_WATE -> RT_NULL

//Fire/Elec/Force -> hi_plasma
{ RT_HI_PLASMA,		"flaring plasma",	16, 13, 20, 10, 3, GF_PLASMA,		GF_LITE,	R_FORC }, //1
//PLASMA uses augments
{ RT_PLASMA_ACID,	"plasma",		14, 10, 20, 10, 3, GF_PLASMA,		0,		R_ACID }, //1
{ RT_PLASMA_WATER,	"liquid plasma",	14, 10, 20, 10, 3, GF_PLASMA,		GF_WATER,	R_WATE }, //1
{ RT_PLASMA_NETHER,	"dark plasma",		14, 10, 20, 10, 3, GF_PLASMA,		GF_DARK,	R_NETH }, //1 (nether -> dark to counter chaos in ice)
{ RT_PLASMA_POISON,	"fainting plasma",	14, 10, 20, 10, 3, GF_PLASMA,		GF_OLD_SLEEP,	R_POIS }, //1 (name? poison rule)
{ RT_PLASMA_TIME,	"temporal plasma",	14, 10, 20, 10, 3, GF_PLASMA,		GF_TIME,	R_TIME }, //1 
//THUNDER uses augments (power may be too weak?)
{ RT_THUNDER_ACID,	"thunder",		 8, 12, 20,  5, 3, GF_SOUND,		0,		R_ACID }, //1
{ RT_THUNDER_CHAOS,	"addling thunder",	 8, 12, 20,  5, 3, GF_SOUND,		GF_CONFUSION,	R_CHAO }, //1 (exception, chaos shouldn't explode with itself? see others for pattern.)
{ RT_THUNDER_COLD,	"thunder",		 8, 12, 20,  5, 3, GF_SOUND,		0,		R_COLD }, //1
{ RT_THUNDER_NETHER,	"corrupted thunder",	 8, 12, 20,  5, 3, GF_SOUND,		GF_NETHER,	R_NETH }, //1
{ RT_THUNDER_POISON,	"stunning thunder",	 8, 12, 20,  5, 3, GF_SOUND,		GF_STUN,	R_POIS }, //1
//ICE uses augments
{ RT_ICE_ELEC,		"ice",			14, 12, 20,  5, 3, GF_ICE,		0,		R_ELEC }, //1
{ RT_ICE_SHARDS,	"impaling ice",		14, 12, 20,  5, 3, GF_ICE,		GF_SHARDS,	R_EART }, //1 (name, slicing ice -> silly?)
{ RT_ICE_CHAOS,		"shining ice",		14, 12, 20,  5, 3, GF_ICE,		GF_LITE,	R_CHAO }, //1 (exception, chaos does this in place of fire 'brilliance')
{ RT_ICE_POISON,	"slowing ice",		14, 12, 20,  5, 3, GF_ICE,		GF_OLD_SLOW,	R_POIS }, //1 (name? poison rule)
//Water/Cold/Force -> hi_ice
{ RT_HI_ICE,		"crackling ice",	16, 12, 20,  5, 3, GF_ICE,		GF_FORCE,	R_FORC }, //1 (exception, ice gets the full force benefit! name unique)
{ RT_ICE_TIME,		"temporal ice",		14, 12, 20, 10, 3, GF_ICE,		GF_TIME,	R_TIME }, //1

//WAVE and MISSILE
//Water/Earth/force -> hi_force
{ RT_HI_FORCE,		"meteorites",		12, 11, 40, 10, 3, GF_METEOR,		GF_FORCE,	R_FORC }, //1
//WAVE uses augments (but only 3/6! ^^,)
//No fire/cold augment for this due to ice and fire/water
//No elec augment for this due to elec+water?
{ RT_WAVE_CHAOS,	"pressure",		 8,  9, 20, 10, 3, GF_WAVE,		0,		R_CHAO }, //1
{ RT_WAVE_NETHER,	"pressure",		 8,  9, 20, 10, 3, GF_WAVE,		0,		R_NETH }, //1
{ RT_WAVE_POISON,	"pressure",		 8,  9, 20, 10, 3, GF_WAVE,		0,		R_POIS }, //1
//MISSILE uses explosions, so higher levels
{ RT_MISSILE_ACID,	"acid missiles",	 4,  7, 20,  5, 3, GF_MISSILE,		GF_ACID,	R_ACID }, //1
{ RT_MISSILE_FIRE,	"fire missiles",	 4,  7, 20,  5, 3, GF_MISSILE,		GF_FIRE,	R_FIRE }, //1
{ RT_MISSILE_CHAOS,	"chaotic missiles",	 6,  8, 30,  5, 3, GF_MISSILE,		GF_CHAOS,	R_CHAO }, //1
{ RT_MISSILE_COLD,	"frost missiles",	 4,  7, 20,  5, 3, GF_MISSILE,		GF_COLD,	R_COLD }, //1
{ RT_MISSILE_NETHER,	"corrupted missiles",	 6,  8, 30,  5, 3, GF_MISSILE,		GF_NETHER,	R_NETH }, //1
{ RT_MISSILE_POISON,	"noxious missiles",	 4,  7, 20,  5, 3, GF_MISSILE,		GF_POIS,	R_POIS }, //1

//MANA and DISENCHANT
//DISENCHANT+MANA -> THIS! (pretty special combination, looking at how many patterns support this! ^^ - Kurzel
//(As disenchant/mana are diametricly opposing, and this is a double nexus effect too, let's make it hi_nexus! ..boosted difficulty +5..)
{ RT_HI_NEXUS,		"nexus tendrils",	 9, 10, 30, 10, 3, GF_NEXUS,		GF_NEXUS,	R_NEXU }, //1 (chaos/nether/nexus, also explodes w/ nexus, self -> alter reality? ^^,)
//DISENCHANT uses augments
{ RT_DISENCHANT_ACID,	"disenchantment",	11, 10, 30, 10, 3, GF_DISENCHANT,	0,		R_ACID }, //1
{ RT_DISENCHANT_WATER,	"disenchantment",	11, 10, 30, 10, 3, GF_DISENCHANT,	0,		R_WATE }, //1
{ RT_DISENCHANT_ELEC,	"disenchantment",	11, 10, 30, 10, 3, GF_DISENCHANT,	0,		R_ELEC }, //1
{ RT_DISENCHANT_SHARDS,	"disenchantment",	11, 10, 30, 10, 3, GF_DISENCHANT,	0,		R_EART }, //1
{ RT_DISENCHANT_COLD,	"disenchantment",	11, 10, 30, 10, 3, GF_DISENCHANT,	0,		R_COLD }, //1
{ RT_DISENCHANT_FORCE,	"disenchantment",	11, 10, 30, 10, 3, GF_DISENCHANT,	0,		R_FORC }, //1
{ RT_DISENCHANT_TIME,	"disenchantment",	11, 10, 30, 10, 3, GF_DISENCHANT,	0,		R_TIME }, //1
//MANA uses augments
{ RT_MANA_ACID,		"mana",			11, 10, 30, 10, 3, GF_MANA,		0,		R_ACID }, //1
{ RT_MANA_WATER,	"mana",			11, 10, 30, 10, 3, GF_MANA,		0,		R_WATE }, //1
{ RT_MANA_ELEC,		"mana",			11, 10, 30, 10, 3, GF_MANA,		0,		R_ELEC }, //1
{ RT_MANA_SHARDS,	"mana",			11, 10, 30, 10, 3, GF_MANA,		0,		R_EART }, //1
{ RT_MANA_FIRE,		"mana",			11, 10, 30, 10, 3, GF_MANA,		0,		R_FIRE }, //1
{ RT_MANA_FORCE,	"mana",			11, 10, 30, 10, 3, GF_MANA,		0,		R_FORC }, //1
{ RT_MANA_TIME,		"mana",			11, 10, 30, 10, 3, GF_MANA,		0,		R_TIME }, //1

/* Nether/Chaos + water/icePoison use augments, sort this! */
{ RT_WATERPOISON_NETHER,"dark miasma",		13, 11, 25, 10, 3, GF_WATERPOISON,	GF_DARK,	R_NETH }, //1
{ RT_ICEPOISON_NETHER,	"dark nettles",		13, 11, 25, 10, 3, GF_ICEPOISON,	GF_DARK,	R_NETH }, //1
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

{ R_ACID | R_EART | R_FIRE, RT_FIRE },
{ R_ACID | R_EART | R_CHAO, RT_CHAOS },
{ R_ACID | R_EART | R_COLD, RT_COLD },
{ R_ACID | R_EART | R_NETH, RT_DARKNESS_SHARDS },
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
{ R_WATE | R_ELEC | R_FORC, RT_ELEC_WATER },
{ R_WATE | R_ELEC | R_TIME, RT_TIME },

{ R_WATE | R_EART | R_FIRE, RT_FIRE },
{ R_WATE | R_EART | R_CHAO, RT_CHAOS },
{ R_WATE | R_EART | R_COLD, RT_ICE_SHARDS },
{ R_WATE | R_EART | R_NETH, RT_NETHER },
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

{ R_WATE | R_COLD | R_POIS, RT_ICE_POISON },
{ R_WATE | R_COLD | R_NEXU, RT_DIG_FIRE },
{ R_WATE | R_COLD | R_FORC, RT_HI_ICE },
{ R_WATE | R_COLD | R_TIME, RT_ICE_TIME },

{ R_WATE | R_NETH | R_POIS, RT_NULL },
{ R_WATE | R_NETH | R_NEXU, RT_MANA_WATER },
{ R_WATE | R_NETH | R_FORC, RT_WAVE_NETHER },
{ R_WATE | R_NETH | R_TIME, RT_WATER },

{ R_WATE | R_POIS | R_FORC, RT_WAVE_POISON },
{ R_WATE | R_POIS | R_TIME, RT_WATERPOISON_TIME },

{ R_WATE | R_NEXU | R_FORC, RT_MISSILE },
{ R_WATE | R_NEXU | R_TIME, RT_DIG_MEMORY },

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

{ R_EART | R_COLD | R_POIS, RT_NULL },
{ R_EART | R_COLD | R_NEXU, RT_NULL },
{ R_EART | R_COLD | R_FORC, RT_MISSILE_COLD },
{ R_EART | R_COLD | R_TIME, RT_DISARM_COLD_TIME },

{ R_EART | R_NETH | R_POIS, RT_NULL },
{ R_EART | R_NETH | R_NEXU, RT_MANA_SHARDS },
{ R_EART | R_NETH | R_FORC, RT_MISSILE_NETHER },
{ R_EART | R_NETH | R_TIME, RT_SHARDS },

{ R_EART | R_POIS | R_FORC, RT_MISSILE_POISON },
{ R_EART | R_POIS | R_TIME, RT_ICEPOISON_TIME },

{ R_EART | R_NEXU | R_FORC, RT_WAVE },
{ R_EART | R_NEXU | R_TIME, RT_DIG_TELEPORT }, 

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

{ R_FIRE | R_COLD | R_POIS, RT_WONDER_RESIST },
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
{ R_CHAO | R_NEXU | R_TIME, RT_DISENCHANT_TIME },

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
{ R_NETH | R_NEXU | R_TIME, RT_MANA_TIME },

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
{ R_NETH | R_TIME, RT_NETHER_TIME },

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
