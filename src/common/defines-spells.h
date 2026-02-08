/* $Id$ */
/* File: defines-spells.h */

/* Purpose: global constants and macro definitions for spells.pre lua file */

#define PR_MANA		0x00000080U	/* Display Mana */

#define PR_STATE	0x00100000U	/* Display Extra (State) */

/*
 * Number of effects
 */
#define EFF_WAVE	0x00000001U	/* A circle whose radius increase */
#define EFF_STORM	0x00000004U	/* The area follows the player */
#define EFF_THINWAVE	0x00000080U	/* Same as wave, but thickness 1 instead of 3 (hits each target only once instead of three times) */
#define EFF_VORTEX	0x00000100U	/* The area follows the target - Kurzel*/

/* Generic temporary weapon branding, currently only used for melee weapons */
#define TBRAND_ELEC		0x0001
#define TBRAND_COLD		0x0002
#define TBRAND_FIRE		0x0004
#define TBRAND_ACID		0x0008
#define TBRAND_POIS		0x0010
//unused/not fully implemented:
#define TBRAND_BASE		(TBRAND_ELEC | TBRAND_COLD | TBRAND_FIRE | TBRAND_ACID)
#define TBRAND_CHAO		0x0020
#define TBRAND_VORP		0x0040
#define TBRAND_BALL_FIRE	0x0080
#define TBRAND_BALL_COLD	0x0100
#define TBRAND_BALL_ELEC	0x0200
#define TBRAND_BALL_ACID	0x0400
#define TBRAND_BALL_SOUN	0x0800
//hereticism:
#define TBRAND_HELLFIRE		0x1000
//unlife:
#define TBRAND_VAMPIRIC		0x2000
//aura of death:
#define TBRAND_ICE		0x4000
#define TBRAND_PLASMA		0x8000

/* Temporary weapon branding - flags */
#define TBRAND_F_EXTERN			0x01	/* Brand wasn't applied by ourselves but an external source, ie via GF_TBRAND_POIS projection; this is just used to prevent message spam */
#define TBRAND_F_POTION_MUSHROOM	0x02	/* Brand type isn't one of the flags above but actually a potion sval (for 'Apply Poison' technique extending) */
#define TBRAND_F_MAINHAND		0x04	/* Brand is applied to equip slot a) */
#define TBRAND_F_OFFHAND		0x08	/* Brand is applied to equip slot b) */
#define TBRAND_F_DUAL			(TBRAND_F_MAINHAND | TBRAND_F_OFFHAND)


/*
 * Shield effect options
 */
#define SHIELD_NONE		0x0000
#define SHIELD_COUNTER		0x0001
#define SHIELD_FIRE		0x0002


/*
 * Spell types used by project(), and related functions.
 */
#define GF_ELEC			1
#define GF_POIS			2
#define GF_ACID			3
#define GF_COLD			4
#define GF_FIRE			5
#define GF_SHOT			6
#define GF_ARROW		7
#define GF_BOLT			8
#define GF_BOULDER		9
#define GF_MISSILE		10
#define GF_ACID_BLIND		11
#define GF_PLASMA		12
#define GF_HOLY_ORB		13
#define GF_WATER		14
#define GF_LITE			15
#define GF_DARK			16
#define GF_LITE_WEAK		17
#define GF_DARK_WEAK		18
#define GF_PROTEVIL_PLAYER	19
#define GF_SHARDS		20
#define GF_SOUND		21
#define GF_CONFUSION		22
#define GF_FORCE		23
#define GF_INERTIA		24
#define GF_MANA			26
#define GF_METEOR		27
#define GF_ICE			28
#define GF_CHAOS		30
#define GF_NETHER		31
#define GF_DISENCHANT		32
#define GF_NEXUS		33
#define GF_TIME			34
#define GF_GRAVITY		35
#define GF_KILL_WALL		40
#define GF_KILL_DOOR		41
#define GF_KILL_TRAP		42
#define GF_KILL_TRAP_DOOR	43
#define GF_MAKE_WALL		45
#define GF_MAKE_DOOR		46
#define GF_MAKE_TRAP		47
#define GF_OLD_CLONE		51
#define GF_OLD_POLY		52
#define GF_OLD_HEAL		53
#define GF_OLD_SPEED		54
#define GF_OLD_SLOW		55
#define GF_OLD_CONF		56
#define GF_OLD_SLEEP		57
#define GF_OLD_DRAIN		58
#define GF_AWAY_UNDEAD		61
#define GF_AWAY_EVIL		62
#define GF_AWAY_ALL		63
#define GF_TURN_UNDEAD		64
#define GF_TURN_EVIL		65
#define GF_TURN_ALL		66
#define GF_DISP_UNDEAD		67
#define GF_DISP_EVIL		68
#define GF_DISP_ALL		69

#define	GF_HEAL_PLAYER		70
#define	GF_STONE_WALL		71
#define	GF_EARTHQUAKE		72
#define	GF_WRAITH_PLAYER	73
#define	GF_SPEED_PLAYER		74
#define	GF_SHIELD_PLAYER	75
#define GF_RECALL_PLAYER	76
#define GF_STUN			77
#define GF_IDENTIFY		78
#define GF_PSI			79
#define GF_HOLY_FIRE		80
#define GF_DISINTEGRATE		81
#define GF_HELLFIRE		82 /* was HOLY_ORB */
#define GF_NETHER_WEAK		83 /* special version of GF_NETHER, solely for Vampires smashing Potions of Death */
#define GF_REMCURSE_PLAYER	84
#define GF_KILL_GLYPH		85
#define GF_STARLITE		86
#define GF_TERROR		87
#define GF_HAVOC		88
#define GF_INFERNO		89 /* damage-wise like GF_ROCKET, but no special sfx and doesn't hurt terrain (could be changed, dunno) */
#define GF_DETONATION		90 /* damage-wise like GF_ROCKET, but different sfx */
#define GF_ROCKET		91

/* for traps.h :) - C. Blue */
#define GF_REMFEAR		92
#define GF_HERO_MONSTER		93
#define GF_LIFEHEAL		94
#define GF_DEC_STR		95
#define GF_DEC_DEX		96
#define GF_DEC_CON		97
#define GF_RES_STR		98
#define GF_RES_DEX		99
#define GF_RES_CON		100
#define GF_INC_STR		101
#define GF_INC_DEX		102
#define GF_INC_CON		103
#define GF_AUGMENTATION		104
#define GF_RUINATION		105
#define GF_EXP			106
#define GF_MIND_SLOW		107
#define GF_TBRAND_POIS		108
#define GF_CODE			109
#define GF_NUKE			110
#define GF_BLIND		111
#define GF_HOLD			112	/* hold */
#define GF_DOMINATE		113	/* dominate */
#define GF_BLESS_PLAYER		114
#define GF_REMFEAR_PLAYER	115
#define GF_SATHUNGER_PLAYER	116
#define GF_RESFIRE_PLAYER	117
#define GF_RESCOLD_PLAYER	118
#define GF_CUREPOISON_PLAYER	119
#define GF_SEEINVIS_PLAYER	120
#define GF_SEEMAP_PLAYER	121
#define GF_CURECUT_PLAYER	122
#define GF_CURESTUN_PLAYER	123
#define GF_DETECTCREATURE_PLAYER	124
#define GF_DETECTDOOR_PLAYER	125
#define GF_DETECTTRAP_PLAYER	126
#define GF_TELEPORTLVL_PLAYER	127
#define GF_RESPOIS_PLAYER	128
#define GF_RESELEC_PLAYER	129
#define GF_RESACID_PLAYER	130
#define GF_HPINCREASE_PLAYER	131
#define GF_HERO_PLAYER		132
#define GF_SHERO_PLAYER		133

#define GF_UNBREATH		134
#define GF_WAVE			135

#define GF_TELEPORT_PLAYER	136	/* UNUSED actually: only s_convey used it once */

#define GF_RESTORE_PLAYER	137	/* C. Blue changes */
#define GF_VAPOUR		138	/* This is same as GF_WATER, just looks differently */
#define GF_CURE_PLAYER		139
#define GF_RESURRECT_PLAYER	140
#define GF_SANITY_PLAYER	141
#define GF_ZEAL_PLAYER		142
#define GF_DISP_DEMON		143
#define GF_SOULCURE_PLAYER	144
#define GF_MINDBOOST_PLAYER	145
#define GF_REMCONF_PLAYER	146
#define GF_REMIMAGE_PLAYER	147
#define GF_SLOWPOISON_PLAYER	148
#define GF_CURING		149

/* Zangband changes */
#define GF_TELE_TO		150
#define GF_HAND_DOOM		151
#define GF_STASIS		152

/* For the new priest spell I'm conjuring - the_sandman */
#define GF_CURSE		153
/* Here comes the druid items - the_sandman */
#define GF_HEALINGCLOUD		154
#define GF_WATERPOISON		155
#define GF_ICEPOISON		156
#define GF_EXTRA_STATS		157
#define GF_EXTRA_TOHIT		158

#define GF_PUSH			159 /* Moltor */
#define GF_SILENCE		160 /* for new mindcrafters */
#define GF_CHARMIGNORE		161
#define GF_STOP			162 /* special fx: scroll of rune of protection in a monster trap - C. Blue */
#define GF_CAUSE		163 /* 'Curse' actually, the monster spell */
#define GF_FLARE		164 /* Combination of LITE_WEAK and FIRE damage, for Flare Missile */
#define GF_LIFE_SLOW		165
#define GF_NO_REGEN		166
#define GF_THUNDER		167 /* To replace the hacky 'triple-bolt' of the thunderstorm spell */
#define GF_ANNIHILATION		168 /* To differentiate drain effect from hacky non-drain effect for wands */
#define GF_VINE_SLOW		169 /* Same as GF_OLD_SLOW but causes terrain fluff */
#define GF_DISP_UNDEAD_DEMON	170
#define	GF_RIFT			171
#define GF_RESTORING		172
#define GF_UNHEALTH		173
#define GF_DISEASE		174

/* For snowflakes on WINTER_SEASON. Could use 0 for type, but let's complete it. -C. Blue */
#define GF_SNOWFLAKE		200
/* For fireworks on NEW_YEARS_EVE - C. Blue */
#define GF_FW_FIRE		201
#define GF_FW_ELEC		202
#define GF_FW_POIS		203
#define GF_FW_LITE		204
#define GF_FW_YCLD		205
#define GF_FW_SHDM		206
#define GF_FW_MULT		207
/* well, let's try to bring weather and seasons? */
#define GF_RAINDROP		208
#define GF_LEAF			209 /* unused, just added here for inspiration - C. Blue */
/* full-screen warnings or other important notifications that players oughtn't overlook - C. Blue */
//ugly though, since they are wpos-bound -..
// #define GF_TEXT_UPDATE	210 /* 'your game version is outdated..' */
#define GF_SHOW_LIGHTNING	211
#define GF_THUNDER_VISUAL	212
