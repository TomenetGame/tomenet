/* $Id$ */
/* File: tables.c */

/* Purpose: Angband Tables shared by both client and server.
 * originally added for runecraft. - C. Blue */

#include "angband.h"

r_element r_elements[RCRAFT_MAX_ELEMENTS] =
{
{ R_LITE, "Light", 	SKILL_R_LITE },
{ R_DARK, "Darkness",	SKILL_R_DARK },
{ R_NEXU, "Nexus", 	SKILL_R_NEXU },
{ R_NETH, "Nether", 	SKILL_R_NETH },
{ R_CHAO, "Chaos", 	SKILL_R_CHAO },
{ R_MANA, "Mana", 	SKILL_R_MANA },
};

r_imperative r_imperatives[RCRAFT_MAX_IMPERATIVES+1] =
{
{ I_MINI, "minimized",	 0,  6, -10,  6, -1,  8, 10 },
{ I_LENG, "lengthened",	 2,  8,  +5, 10,  0, 15, 10 },
{ I_COMP, "compressed",	 3, 13,  -5, 13, -2,  8, 10 },
{ I_MODE, "moderate",	 5, 10,   0, 10,  0, 10, 10 },
{ I_EXPA, "expanded",	 7, 13,  +5, 10, +2, 13, 10 },
{ I_BRIE, "brief",	 8,  8,  -5,  8,  0,  6,  5 },
{ I_MAXI, "maximized",	10, 15, +10, 15, +1, 13, 10 },
{ I_ENHA, "enhanced",	10, 15,   0, 10,  0, 10, 10 },
};

r_type r_types[RCRAFT_MAX_TYPES+1] =
{
{ T_BOLT, "bolt",	 1,  1, 15, 3,  4, 53, 27,  0,   0, 0, 0,  0,  0 },
{ T_BEAM, "beam",	 5,  2, 25, 3,  9, 53, 32,  9, 169, 0, 0,  6, 14 },
{ T_CLOU, "cloud",	10,  3, 30, 0,  0,  0,  0,  9, 169, 1, 5,  6, 14 },
{ T_BALL, "ball",	15,  5, 30, 0,  0, 28, 28, 25, 784, 2, 7,  0,  0 },
{ T_WAVE, "wave",	20,  4, 40, 0,  0,  0,  0, 25, 441, 0, 0,  6, 10 },
{ T_STOR, "storm",	25, 10, 40, 5, 10, 25, 25, 16, 256, 1, 3, 10, 35 },
{ T_RUNE, "rune",	30, 20, 20, 0,  0,  0,  0, 16, 625, 1, 2,  3,  7 },
{ T_ENCH, "augment",	35, 50, 50, 0,  0,  0,  0,  0,   0, 0, 0,  0,  0 },
};

r_projection r_projections[RCRAFT_MAX_PROJECTIONS] =
{
{ R_LITE,		GF_LITE,	400,	"light" },
{ R_DARK,		GF_DARK,	900,	"darkness" },
{ R_NEXU,		GF_NEXUS,	350,	"lightning" },
{ R_NETH,		GF_NETHER,	550,	"nether" },
{ R_CHAO,		GF_CHAOS,	600,	"chaos" },
{ R_MANA,		GF_MANA,	550,	"mana" },

{ R_LITE | R_DARK,	GF_CONFUSION,	400,	"confusion" },
{ R_LITE | R_NEXU,	GF_INERTIA,	350,	"inertia" },
{ R_LITE | R_NETH,	GF_ELEC,	1200,	"lightning" },
{ R_LITE | R_CHAO,	GF_FIRE,	1200,	"fire" },
{ R_LITE | R_MANA,	GF_WAVE,	250,	"water" },

{ R_DARK | R_NEXU,	GF_GRAVITY,	300,	"gravity" },
{ R_DARK | R_NETH,	GF_COLD,	1200,	"frost" },
{ R_DARK | R_CHAO,	GF_ACID,	600,	"acid" },
{ R_DARK | R_MANA,	GF_POIS,	800,	"poison" },

{ R_NEXU | R_NETH,	GF_TIME,	150,	"time" },
{ R_NEXU | R_CHAO,	GF_SOUND,	400,	"sound" },
{ R_NEXU | R_MANA,	GF_SHARDS,	400,	"shards" },

{ R_NETH | R_CHAO,	GF_DISENCHANT,	500,	"disenchantment" },
{ R_NETH | R_MANA,	GF_FORCE,	400,	"force"},

{ R_CHAO | R_MANA,	GF_PLASMA,	700,	"plasma" },
};

