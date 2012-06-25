/* $Id$ */
/* File: tables.c */

/* Purpose: Angband Tables shared by both client and server.
 * originally added for runecraft. - C. Blue */

#include "angband.h"

#ifdef ENABLE_RCRAFT

r_element r_elements[RCRAFT_MAX_ELEMENTS] =
{
{ R_ACID, "Acid", 		SKILL_R_ACIDWATE,	 5, 10, 10,  0, 10,  0, 12 },
{ R_WATE, "Water",		SKILL_R_ACIDWATE,	15, 10, 10,  0, 10,  0, 12 },
{ R_ELEC, "Electricity",	SKILL_R_ELECEART,	 5, 10,  8,  0, 10,  0, 10 },
{ R_EART, "Earth", 		SKILL_R_ELECEART,	15, 10,  8,  0, 10,  0, 10 },
{ R_FIRE, "Fire", 		SKILL_R_FIRECHAO,	 5, 10, 12,  0, 12,  0, 10 },
{ R_CHAO, "Chaos", 		SKILL_R_FIRECHAO,	35, 10, 12,  0, 12,  0, 10 },
{ R_COLD, "Cold", 		SKILL_R_COLDNETH,	 5, 10, 10, -5, 10,  0, 10 },
{ R_NETH, "Nether", 		SKILL_R_COLDNETH,	30, 10, 10, -5, 10,  0, 10 },
{ R_POIS, "Poison", 		SKILL_R_POISNEXU,	10, 10, 10,  0, 10,  1, 10 },
{ R_NEXU, "Nexus", 		SKILL_R_POISNEXU,	25, 10, 10,  0, 10,  1, 10 },
{ R_FORC, "Force", 		SKILL_R_FORCTIME,	20, 10, 10,  0, 10,  1, 10 },
{ R_TIME, "Time", 		SKILL_R_FORCTIME,	40, 10, 10,  0, 10,  1, 10 },
};

r_imperative r_imperatives[RCRAFT_MAX_IMPERATIVES] =
{
{ I_MINI, "minimized",					-5, 10,  6,-10,  6, -1,  8 },
{ I_MODE, "moderate",					 0, 10, 10,  0, 10,  0, 10 },
{ I_MAXI, "maximized",					+5, 10, 15,+10, 15, +1, 13 },
{ I_COMP, "compressed",					-1, 10, 10, -5, 13, -2,  8 },
{ I_EXPA, "expanded",					+1, 10, 13, +5, 10, +2, 13 },
{ I_BRIE, "brief",					+3,  5, 10, -5,  8,  0,  6 },
{ I_LENG, "lengthened",					-3, 10, 13, +5, 10,  0, 15 },
{ I_CHAO, "chaotic",					 0, 10, 10,  0, 10,  0, 10 },
};

r_type r_types[RCRAFT_MAX_TYPES] =
{
{ T_MELE, "burst",					 0,  5,  5,  0,  5,  0,  0 },
{ T_SELF, "self",					 0, 10, 10,  0, 10,  2, 10 },
{ T_BOLT, "bolt",					 0, 10, 10,  0, 10,  0,  0 },
{ T_BEAM, "beam",					 2, 10,	14,  0, 10,  0,	 0 },
{ T_BALL, "ball",					 4, 10, 18,  0, 10,  2,	 0 },
{ T_WAVE, "wave",					 1, 10,	12,  0, 10,  0,	10 },
{ T_CLOU, "cloud",					 3, 10, 16,  0, 10,  1, 10 },
{ T_STOR, "storm",					 5, 10,	20,  0, 10,  1,	10 },
};

r_projection r_projections[RCRAFT_MAX_PROJECTIONS + 1] =
{
{ R_ACID,		GF_ACID,		DT_DIRECT,	1200,	"acidic ",		"acid" },
{ R_WATE,		GF_WATER,		DT_DIRECT,	250,	"damp ",		"water" },
{ R_ELEC,		GF_ELEC,		DT_DIRECT,	1200,	"shocking ",		"lightning" },
{ R_EART,		GF_SHARDS,		DT_DIRECT,	400,	"cutting ",		"shards" },
{ R_FIRE,		GF_FIRE,		DT_DIRECT,	1200,	"fiery ",		"fire" },
{ R_CHAO,		GF_CHAOS,		DT_DIRECT,	600,	"chaotic ",		"chaos" },
{ R_COLD,		GF_COLD,		DT_DIRECT,	1200,	"frozen ",		"frost" },
{ R_NETH,		GF_NETHER,		DT_DIRECT,	550,	"corrupted ",		"nether" },
{ R_POIS,		GF_POIS,		DT_DIRECT,	800,	"venemous ",		"poison" },
{ R_NEXU,		GF_NEXUS,		DT_DIRECT,	350,	"twisting ",		"nexus" },
{ R_FORC,		GF_FORCE,		DT_DIRECT,	200,	"impacting ",		"force" },
{ R_TIME,		GF_TIME,		DT_DIRECT,	150,	"temporal ",		"time" },

{ R_ACID | R_WATE,	GF_DISP_ALL,		DT_DIRECT,	100,	"dissolving ",		"dissolution" },
{ R_ACID | R_ELEC,	GF_ACID_ELEC,		DT_DIRECT,	1000,	"ionizing ",		"ions" },
{ R_ACID | R_EART,	GF_ACID_DISARM,		DT_DIRECT,	600,	"corroding ",		"corrosion" },
{ R_ACID | R_FIRE,	GF_ACID_FIRE,		DT_DIRECT,	1000,	"bilious ",		"bile" },
{ R_ACID | R_CHAO,	GF_CONFUSION,		DT_DIRECT,	400,	"perplexing ",		"perplexity" },
{ R_ACID | R_COLD,	GF_ACID_COLD,		DT_DIRECT,	1000,	"hoary ",		"rime" },
{ R_ACID | R_NETH,	GF_ANNIHILATION,	DT_INDIRECT,	20,	"annihilating ",	"annihilation" },
{ R_ACID | R_POIS,	GF_ACID_POISON,		DT_DIRECT,	700,	"sickening ",		"venom" },
{ R_ACID | R_NEXU,	GF_ELEC,		DT_DIRECT,	1200,	"shocking ",		"lightning" },
{ R_ACID | R_FORC,	GF_HI_ACID,		DT_DIRECT,	1000,	"ablating ",		"ablation" },
{ R_ACID | R_TIME,	GF_ACID,		DT_DIRECT,	1200,	"acidic ",		"acid" },

{ R_WATE | R_ELEC,	GF_ELEC_DISARM,		DT_DIRECT,	600,	"discharging ",		"discharge" },
{ R_WATE | R_EART,	GF_KILL_WALL,		DT_HACK,	10,	"eroding ",		"erosion" },
{ R_WATE | R_FIRE,	GF_FIRE_DISARM,		DT_DIRECT,	600,	"evaporating ",		"evaporation" },
{ R_WATE | R_CHAO,	GF_OLD_POLY,		DT_EFFECT,	50,	"mutating ",		"polymorphing" },
{ R_WATE | R_COLD,	GF_ICE,			DT_DIRECT,	700,	"crackling ",		"ice" },
{ R_WATE | R_NETH,	GF_OLD_CLONE,		DT_EFFECT,	50,	"duplicating ",		"duplication" },
{ R_WATE | R_POIS,	GF_OLD_SLEEP,		DT_EFFECT,	25,	"paralyzing ",		"sleep" },
{ R_WATE | R_NEXU,	GF_TELE_TO,		DT_EFFECT,	100,	"returning ",		"calling" },
{ R_WATE | R_FORC,	GF_WAVE,		DT_DIRECT,	400,	"pressurized ",		"pressure" },
{ R_WATE | R_TIME,	GF_WATER,		DT_DIRECT,	250,	"damp ",		"water" },

{ R_ELEC | R_EART,	GF_THUNDER,		DT_DIRECT,	700,	"thunderous ",		"thunder" },
{ R_ELEC | R_FIRE,	GF_ELEC_FIRE,		DT_DIRECT,	1000,	"searing ",		"flux" },
{ R_ELEC | R_CHAO,	GF_LITE,		DT_DIRECT,	400,	"luminous ",		"light" },
{ R_ELEC | R_COLD,	GF_ELEC_COLD,		DT_DIRECT,	1000,	"conductive ",		"static" },
{ R_ELEC | R_NETH,	GF_DARK,		DT_DIRECT,	900,	"dim ",			"darkness" },
{ R_ELEC | R_POIS,	GF_ELEC_POISON,		DT_DIRECT,	700,	"jolting ",		"jolting" },
{ R_ELEC | R_NEXU,	GF_ACID,		DT_DIRECT,	1200,	"acidic ",		"acid" },
{ R_ELEC | R_FORC,	GF_HI_ELEC,		DT_DIRECT,	1000,	"zapping ",		"energy" },
{ R_ELEC | R_TIME,	GF_ELEC,		DT_DIRECT,	1200,	"shocking ",		"lightning" },

{ R_EART | R_FIRE,	GF_DETONATION,		DT_DIRECT,	800,	"detonating ",		"detonations" },
{ R_EART | R_CHAO,	GF_EARTHQUAKE,		DT_HACK,	2,	"quaking ",		"turmoil" },
{ R_EART | R_COLD,	GF_COLD_DISARM,		DT_DIRECT,	600,	"shattering ",		"shattering" },
//{ R_EART | R_NETH,	GF_GENOCIDE,		DT_EFFECT,	50,	"recoiling ",		"recoil" },
{ R_EART | R_NETH,	GF_RUINATION,		DT_EFFECT,	50,	"wracking ",		"ruination" }, //perhaps DT_HACK is more appropriate?
{ R_EART | R_POIS,	GF_STUN,		DT_EFFECT,	100,	"concussive ",		"concussion" }, //ignores damage?? as above (should FIX this - urgent!) - Kurzel!!
{ R_EART | R_NEXU,	GF_GRAVITY,		DT_DIRECT,	300,	"heavy ",		"gravity" },
{ R_EART | R_FORC,	GF_SOUND,		DT_DIRECT,	400,	"cacophonous ",		"sound" },
{ R_EART | R_TIME,	GF_SHARDS,		DT_DIRECT,	400,	"cutting ",		"shards" },

{ R_FIRE | R_CHAO,	GF_HELL_FIRE,		DT_DIRECT,	600,	"hellish ",		"hellfire" },
{ R_FIRE | R_COLD,	GF_FIRE_COLD,		DT_DIRECT,	1000,	"disparate ",		"whitefire" },
{ R_FIRE | R_NETH,	GF_LIFE_FIRE,		DT_DIRECT,	600,	"matyring ",		"lifefire" },
{ R_FIRE | R_POIS,	GF_FIRE_POISON,		DT_DIRECT,	700,	"smoking ",		"ash" },
{ R_FIRE | R_NEXU,	GF_COLD,		DT_DIRECT,	1200,	"frozen ",		"frost" },
{ R_FIRE | R_FORC,	GF_HI_FIRE,		DT_DIRECT,	1000,	"incinerating ",	"incineration" },
{ R_FIRE | R_TIME,	GF_FIRE,		DT_DIRECT,	1200,	"fiery ",		"fire" },

{ R_CHAO | R_COLD,	GF_WONDER,		DT_DIRECT,	0,	"wonderous ",		"wonder" },
{ R_CHAO | R_NETH,	GF_DISENCHANT,		DT_DIRECT,	500,	"disenchanting ",	"disenchantment" },
//{ R_CHAO | R_POIS,	GF_AFFLICT,		DT_EFFECT,	250,	"afflicting ",		"affliction" },
{ R_CHAO | R_POIS,	GF_BLIND,		DT_EFFECT,	50,	"confusing ",		"confusion" }, //'confusion' effect, no damage (use blind? for now..) - Kurzel!!
{ R_CHAO | R_NEXU,	GF_MISSILE,		DT_DIRECT,	150,	"harmonious ",		"order" },
{ R_CHAO | R_FORC,	GF_METEOR,		DT_DIRECT,	200,	"entropic ",		"entropy" },
{ R_CHAO | R_TIME,	GF_CHAOS,		DT_DIRECT,	600,	"chaotic ",		"chaos" },

{ R_COLD | R_NETH,	GF_BLIGHT,		DT_DIRECT,	600,	"deathly ",		"blight" },
{ R_COLD | R_POIS,	GF_COLD_POISON,		DT_DIRECT,	700,	"chilling ",		"frostbite" },
{ R_COLD | R_NEXU,	GF_FIRE,		DT_DIRECT,	1200,	"fiery ",		"fire" },
{ R_COLD | R_FORC,	GF_HI_COLD,		DT_DIRECT,	1000,	"snapping ",		"snapping" },
{ R_COLD | R_TIME,	GF_COLD,		DT_DIRECT,	1200,	"frozen ",		"frost" },

{ R_NETH | R_POIS,	GF_OLD_DRAIN,		DT_INDIRECT,	15,	"devouring ",		"miasma" },
{ R_NETH | R_NEXU,	GF_MANA,		DT_DIRECT,	550,	"destructive ",		"mana" },
{ R_NETH | R_FORC,	GF_DISINTEGRATE,	DT_DIRECT,	300,	"obliterating ",		"disintegration" },
{ R_NETH | R_TIME,	GF_NETHER,		DT_DIRECT,	550,	"corrupted ",		"nether" },

{ R_POIS | R_NEXU,	GF_UNBREATH,		DT_DIRECT,	600,	"choking ",		"unbreath" },
{ R_POIS | R_FORC,	GF_HI_POISON,		DT_DIRECT,	1000,	"toxic ",		"toxins" },
{ R_POIS | R_TIME,	GF_STOP,		DT_EFFECT,	75,	"disjoining ",		"disjunction" }, //experimental - Kurzel!!

{ R_NEXU | R_FORC,	GF_AWAY_ALL,		DT_EFFECT,	200,	"warping ",		"teleportation" },
{ R_NEXU | R_TIME,	GF_STASIS,		DT_EFFECT,	25,	"entrapping ",		"stasis" },

{ R_FORC | R_TIME,	GF_INERTIA,		DT_DIRECT,	350,	"inertial ",		"inertia" },

//Begin the extra projections
{ 0,			GF_PLASMA,		DT_DIRECT,	700,	"searing ",		"plasma" }, //plasma and flux both 'sear'
};

#endif
