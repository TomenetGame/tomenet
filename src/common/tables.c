/* $Id$ */
/* File: tables.c */

/* Purpose: Angband Tables shared by both client and server.
 * originally added for runecraft. - C. Blue */

#include "angband.h"



/* Spell failure rate calculations ----------------------------------------- */

/*
 * Stat Table (INT/WIS) -- Minimum failure rate (percentage)
 */
byte adj_mag_fail[] = {
        99      /* 3 */,
        99      /* 4 */,
        99      /* 5 */,
        99      /* 6 */,
        99      /* 7 */,
        50      /* 8 */,
        30      /* 9 */,
        20      /* 10 */,
        15      /* 11 */,
        12      /* 12 */,
        11      /* 13 */,
        10      /* 14 */,
        9       /* 15 */,
        8       /* 16 */,
        7       /* 17 */,
        6       /* 18/00-18/09 */,
        6       /* 18/10-18/19 */,
        5       /* 18/20-18/29 */,
        5       /* 18/30-18/39 */,
        5       /* 18/40-18/49 */,
        4       /* 18/50-18/59 */,
        4       /* 18/60-18/69 */,
        4       /* 18/70-18/79 */,
        4       /* 18/80-18/89 */,
        3       /* 18/90-18/99 */,
        3       /* 18/100-18/109 */,
        2       /* 18/110-18/119 */,
        2       /* 18/120-18/129 */,
        2       /* 18/130-18/139 */,
        2       /* 18/140-18/149 */,
        1       /* 18/150-18/159 */,
        1       /* 18/160-18/169 */,
        1       /* 18/170-18/179 */,
        1       /* 18/180-18/189 */,
        1       /* 18/190-18/199 */,
        0       /* 18/200-18/209 */,
        0       /* 18/210-18/219 */,
        0       /* 18/220+ */
};

/*
 * Stat Table (INT/WIS) -- Various things
 */
byte adj_mag_stat[] = {
        0       /* 3 */,
        0       /* 4 */,
        0       /* 5 */,
        1       /* 6 */,
        1       /* 7 */,
        1       /* 8 */,
        2       /* 9 */,
        2       /* 10 */,
        3       /* 11 */,
        3       /* 12 */,
        4       /* 13 */,
        4       /* 14 */,
        5       /* 15 */,
        6       /* 16 */,
        7       /* 17 */,
        8       /* 18/00-18/09 */,
        9       /* 18/10-18/19 */,
        10      /* 18/20-18/29 */,
        11      /* 18/30-18/39 */,
        12      /* 18/40-18/49 */,
        13      /* 18/50-18/59 */,
        14      /* 18/60-18/69 */,
        16      /* 18/70-18/79 */,
        18      /* 18/80-18/89 */,
        21      /* 18/90-18/99 */,
        24      /* 18/100-18/109 */,
        27      /* 18/110-18/119 */,
        30      /* 18/120-18/129 */,
        33      /* 18/130-18/139 */,
        36      /* 18/140-18/149 */,
        39      /* 18/150-18/159 */,
        42      /* 18/160-18/169 */,
        45      /* 18/170-18/179 */,
        48      /* 18/180-18/189 */,
        51      /* 18/190-18/199 */,
        54      /* 18/200-18/209 */,
        57      /* 18/210-18/219 */,
        60      /* 18/220+ */
};

/*
 * Stat Table (INT) for mimicry powers fail rate modifier (%)
 */
byte adj_int_pow[] = {
        250     /* 3 */,
        210     /* 4 */,
        190     /* 5 */,
        160     /* 6 */,
        140     /* 7 */,
        120     /* 8 */,
        110     /* 9 */,
        100     /* 10 */,
        100     /* 11 */,
        95      /* 12 */,
        90      /* 13 */,
        85      /* 14 */,
        80      /* 15 */,
        75      /* 16 */,
        70      /* 17 */,
        65      /* 18/00-18/09 */,
        61      /* 18/10-18/19 */,
        57      /* 18/20-18/29 */,
        53      /* 18/30-18/39 */,
        49      /* 18/40-18/49 */,
        45      /* 18/50-18/59 */,
        41      /* 18/60-18/69 */,
        38      /* 18/70-18/79 */,
        35      /* 18/80-18/89 */,
        32      /* 18/90-18/99 */,
        29      /* 18/100-18/109 */,
        27      /* 18/110-18/119 */,
        25      /* 18/120-18/129 */,
        23      /* 18/130-18/139 */,
        21      /* 18/140-18/149 */,
        19      /* 18/150-18/159 */,
        17      /* 18/160-18/169 */,
        15      /* 18/170-18/179 */,
        14      /* 18/180-18/189 */,
        13      /* 18/190-18/199 */,
        12      /* 18/200-18/209 */,
        11      /* 18/210-18/219 */,
        10      /* 18/220+ */
};



/* Mimicry powers ---------------------------------------------------------- */

/*
 * Mimic 'spells' -- they should be integrated to the realm spells
 * (slevel, smana, sfail, sexp, ftk)
 */
magic_type innate_powers[96] = {
/* 0(lev), mana, fail, 0(exp), ftk */
// RF4_SHRIEK                   0x00000001      /* Shriek for help */
  {0, 2, 0, 0, 0},
// RF4_UNMAGIC                  0x00000002      /* (?) */
  {0, 0, 0, 0, 0},
// (S_ANIMAL) RF4_XXX3                  0x00000004      /* (?) */
  {0, 0, 0, 0, 0},
// RF4_ROCKET                   0x00000008      /* (?) */
  {0, 60, 70, 0, 2},
// RF4_ARROW_1                  0x00000010      /* Fire an arrow (light) */
  {0, 2, 5, 0, 1},
// RF4_ARROW_2                  0x00000020      /* Fire an shot (heavy) */
  {0, 2, 6, 0, 1},
// XXX (RF4_ARROW_3)                    0x00000040      /* Fire bolt (heavy) */
  {0, 2, 7, 0, 1},
// XXX (RF4_ARROW_4)                    0x00000080      /* Fire missiles (heavy) */
  {0, 3, 9, 0, 1},
// RF4_BR_ACID                  0x00000100      /* Breathe Acid */
  {0, 10, 20, 0, 2},
// RF4_BR_ELEC                  0x00000200      /* Breathe Elec */
  {0, 10, 20, 0, 2},
// RF4_BR_FIRE                  0x00000400      /* Breathe Fire */
  {0, 10, 20, 0, 2},
// RF4_BR_COLD                  0x00000800      /* Breathe Cold */
  {0, 10, 20, 0, 2},
// RF4_BR_POIS                  0x00001000      /* Breathe Poison */
  {0, 13, 25, 0, 2},
// RF4_BR_NETH                  0x00002000      /* Breathe Nether */
  {0, 22, 35, 0, 2},
// RF4_BR_LITE                  0x00004000      /* Breathe Lite */
  {0, 10, 20, 0, 2},
// RF4_BR_DARK                  0x00008000      /* Breathe Dark */
  {0, 10, 20, 0, 2},
// RF4_BR_CONF                  0x00010000      /* Breathe Confusion */
  {0, 10, 25, 0, 2},
// RF4_BR_SOUN                  0x00020000      /* Breathe Sound */
  {0, 14, 30, 0, 2},
// RF4_BR_CHAO                  0x00040000      /* Breathe Chaos */
  {0, 25, 40, 0, 2},
// RF4_BR_DISE                  0x00080000      /* Breathe Disenchant */
  {0, 28, 45, 0, 2},
// RF4_BR_NEXU                  0x00100000      /* Breathe Nexus */
  {0, 20, 45, 0, 2},
// RF4_BR_TIME                  0x00200000      /* Breathe Time */
  {0, 30, 50, 0, 2},
// RF4_BR_INER                  0x00400000      /* Breathe Inertia */
  {0, 25, 35, 0, 2},
// RF4_BR_GRAV                  0x00800000      /* Breathe Gravity */
  {0, 25, 35, 0, 2},
// RF4_BR_SHAR                  0x01000000      /* Breathe Shards */
  {0, 15, 25, 0, 2},
// RF4_BR_PLAS                  0x02000000      /* Breathe Plasma */
  {0, 25, 30, 0, 2},
// RF4_BR_WALL                  0x04000000      /* Breathe Force */
  {0, 30, 40, 0, 2},
// RF4_BR_MANA                  0x08000000      /* Breathe Mana */
  {0, 35, 45, 0, 2},
// RF4_BR_DISI                  0x10000000
  {0, 50, 70, 0, 2},
// RF4_BR_NUKE                  0x20000000
  {0, 27, 40, 0, 2},
// 0x40000000
  {0, 0, 0, 0, 0},
// RF4_BOULDER
  {0, 2, 15, 0, 1},

/*
 * New monster race bit flags
 */
// RF5_BA_ACID                  0x00000001      /* Acid Ball */
  {0, 8, 10, 0, 2},
// RF5_BA_ELEC                  0x00000002      /* Elec Ball */
  {0, 8, 10, 0, 2},
// RF5_BA_FIRE                  0x00000004      /* Fire Ball */
  {0, 8, 10, 0, 2},
// RF5_BA_COLD                  0x00000008      /* Cold Ball */
  {0, 8, 10, 0, 2},
// RF5_BA_POIS                  0x00000010      /* Poison Ball */
  {0, 11, 20, 0, 2},
// RF5_BA_NETH                  0x00000020      /* Nether Ball */
  {0, 25, 40, 0, 2},
// RF5_BA_WATE                  0x00000040      /* Water Ball */
  {0, 17, 30, 0, 2},
// RF5_BA_MANA                  0x00000080      /* Mana Storm */
  {0, 45, 50, 0, 2},
// RF5_BA_DARK                  0x00000100      /* Darkness Storm */
  {0, 20, 0, 0, 2},
// RF5_DRAIN_MANA               0x00000200      /* Drain Mana */
  {0, 0, 0, 0, 0},
// RF5_MIND_BLAST               0x00000400      /* Blast Mind */
  {0, 15, 13, 0, 2},
// RF5_BRAIN_SMASH              0x00000800      /* Smash Brain */
  {0, 25, 15, 0, 2},
// RF5_CAUSE_1                  0x00001000      /* Cause Light Wound */
  {0, 3, 15, 0, 1},
// RF5_CAUSE_2                  0x00002000      /* Cause Serious Wound */
  {0, 6, 20, 0, 1},
// RF5_BA_NUKE                  0x00004000      /* Toxic Ball */
  {0, 30, 30, 0, 2},
// RF5_BA_CHAO                  0x00008000      /* Chaos Ball */
  {0, 45, 40, 0, 2},
// RF5_BO_ACID                  0x00010000      /* Acid Bolt */
  {0, 4, 13, 0, 1},
// RF5_BO_ELEC                  0x00020000      /* Elec Bolt (unused) */
  {0, 3, 13, 0, 1},
// RF5_BO_FIRE                  0x00040000      /* Fire Bolt */
  {0, 4, 13, 0, 1},
// RF5_BO_COLD                  0x00080000      /* Cold Bolt */
  {0, 3, 13, 0, 1},
// RF5_BO_POIS                  0x00100000      /* Poison Bolt (unused) */
  {0, 5, 16, 0, 1},
// RF5_BO_NETH                  0x00200000      /* Nether Bolt */
  {0, 15, 20, 0, 1},
// RF5_BO_WATE                  0x00400000      /* Water Bolt */
  {0, 13, 18, 0, 1},
// RF5_BO_MANA                  0x00800000      /* Mana Bolt */
  {0, 20, 25, 0, 1},
// RF5_BO_PLAS                  0x01000000      /* Plasma Bolt */
  {0, 15, 18, 0, 1},
// RF5_BO_ICEE                  0x02000000      /* Ice Bolt */
  {0, 10, 15, 0, 1},
// RF5_MISSILE                  0x04000000      /* Magic Missile */
  {0, 2, 5, 0, 1},
// RF5_SCARE                    0x08000000      /* Frighten Player */
  {0, 3, 8, 0, 0},
// RF5_BLIND                    0x10000000      /* Blind Player */
  {0, 5, 8, 0, 0},
// RF5_CONF                     0x20000000      /* Confuse Player */
  {0, 5, 8, 0, 0},
// RF5_SLOW                     0x40000000      /* Slow Player */
  {0, 7, 10, 0, 0},
// RF5_HOLD                     0x80000000      /* Paralyze Player (translates into forced monsleep) */
  {0, 5, 10, 0, 0},

/*
 * New monster race bit flags
 */
// RF6_HASTE                    0x00000001      /* Speed self */
  {0, 10, 20, 0, 0},
// RF6_HAND_DOOM                0x00000002      /* Speed a lot (?) */
  {0, 100, 80, 0, 0},
// RF6_HEAL                     0x00000004      /* Heal self */
  {0, 10, 20, 0, 0},
// RF6_XXX2                     0x00000008      /* Heal a lot (?) */
  {0, 0, 0, 0, 0},
// RF6_BLINK                    0x00000010      /* Teleport Short */
  {0, 5, 15, 0, 0},
// RF6_TPORT                    0x00000020      /* Teleport Long */
  {0, 20, 30, 0, 0},
// RF6_XXX3                     0x00000040      /* Move to Player (?) */
  {0, 0, 0, 0, 0},
// RF6_XXX4                     0x00000080      /* Move to Monster (?) */
  {0, 0, 0, 0, 0},
// RF6_TELE_TO                  0x00000100      /* Move player to monster */
  {0, 20, 30, 0, 0},
// RF6_TELE_AWAY                0x00000200      /* Move player far away */
  {0, 25, 30, 0, 0},
// RF6_TELE_LEVEL               0x00000400      /* Move player vertically */
  {0, 30, 60, 0, 0},
// RF6_XXX5                     0x00000800      /* Move player (?) */
  {0, 0, 0, 0, 0},
// RF6_DARKNESS         0x00001000      /* Create Darkness */
  {0, 6, 8, 0, 0},
// RF6_TRAPS                    0x00002000      /* Create Traps */
  {0, 15, 25, 0, 0},
// RF6_FORGET                   0x00004000      /* Cause amnesia */
  {0, 25, 35, 0, 0},

};

monster_spell_type monster_spells4[32] = {
  {"Shriek", FALSE},
  {"Negate magic", FALSE},
  {"XXX", TRUE},
  {"Fire Rocket", TRUE},

  {"Arrow", TRUE},
  {"Shot", TRUE},
  {"Bolt", TRUE},
  {"Missile", TRUE},

  {"Breathe Acid", TRUE},
  {"Breathe Lightning", TRUE},
  {"Breathe Fire", TRUE},
  {"Breathe Cold", TRUE},

  {"Breathe Poison", TRUE},
  {"Breathe Nether", TRUE},
  {"Breathe Lite", TRUE},
  {"Breathe Darkness", TRUE},

  {"Breathe Confusion", TRUE},
  {"Breathe Sound", TRUE},
  {"Breathe Chaos", TRUE},
  {"Breathe Disenchantment", TRUE},

  {"Breathe Nexus", TRUE},
  {"Breathe Time", TRUE},
  {"Breathe Inertia", TRUE},
  {"Breathe Gravity", TRUE},

  {"Breathe Shards", TRUE},
  {"Breathe Plasma", TRUE},
  {"Breathe Force", TRUE},
  {"Breathe Mana", TRUE},

  {"Breathe Disintegration", TRUE},
  {"Breathe Toxic Waste", TRUE},
  {"Ghastly Moan", FALSE},
  {"Throw Boulder", TRUE},      /* "XXX", */
};

monster_spell_type monster_spells5[32] = {
  {"Acid Ball", TRUE},
  {"Lightning Ball", TRUE},
  {"Fire Ball", TRUE},
  {"Cold Ball", TRUE},

  {"Poison Ball", TRUE},
  {"Nether Ball", TRUE},
  {"Water Ball", TRUE},
  {"Mana Storm", TRUE},

  {"Darkness Storm", TRUE},
  {"Drain Mana", TRUE},
  {"Mind Blast", TRUE},
  {"Brain Smash", TRUE},

  {"Cause Wounds", TRUE},
  {"XXX", TRUE},
  {"Ball Toxic Waste", TRUE},
  {"Raw Chaos", TRUE},

  {"Acid Bolt", TRUE},
  {"Lightning Bolt", TRUE},
  {"Fire Bolt", TRUE},
  {"Cold Bolt", TRUE},

  {"Poison Bolt", TRUE},
  {"Nether Bolt", TRUE},
  {"Water Bolt", TRUE},
  {"Mana Bolt", TRUE},

  {"Plasma Bolt", TRUE},
  {"Ice Bolt", TRUE},
  {"Magic Missile", TRUE},
  {"Scare", TRUE},

  {"Blind", TRUE},
  {"Confusion", TRUE},
  {"Slow", TRUE},
  {"Paralyze", TRUE},
};

monster_spell_type monster_spells6[32] = {
  {"Haste Self", FALSE},
  {"Hand of Doom", TRUE},
  {"Heal", FALSE},
  {"XXX", TRUE},

  {"Blink", FALSE},
  {"Teleport", FALSE},
  {"XXX", TRUE},
  {"XXX", TRUE},

  {"Teleport To", TRUE},
  {"Teleport Away", TRUE},
  {"Teleport Level", FALSE},
  {"XXX", TRUE},

  {"Darkness", FALSE},
  {"Trap Creation", FALSE},
  {"Cause Amnesia", TRUE},
  /* Summons follow, but players can't summon */
  {"XXX", TRUE},

  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},

  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},

  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},

  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},
};


/* Runecraft --------------------------------------------------------------- */

r_element r_elements[RCRAFT_MAX_ELEMENTS] = {
{ R_LITE, "Light",    SKILL_R_LITE },
{ R_DARK, "Darkness", SKILL_R_DARK },
{ R_NEXU, "Nexus",    SKILL_R_NEXU },
{ R_NETH, "Nether",   SKILL_R_NETH },
{ R_CHAO, "Chaos",    SKILL_R_CHAO },
{ R_MANA, "Mana",     SKILL_R_MANA },
};

r_imperative r_imperatives[RCRAFT_MAX_IMPERATIVES] =   // |  EFF |  DPS |  AOE |  DUR |
{                                                      // +------+------+------+------+
{ I_MINI, "minimized",   0,  4, -12,  6, -1,  8, 10 }, // | +50% | -40% | POOR | POOR |
{ I_LENG, "lengthened",  2,  6,  -6,  8,  0, 14, 10 }, // | +33% | -20% |      | BEST | Best efficiency (duration*area) for cloud.
{ I_COMP, "compressed",  3,  8,  -3, 10, -2, 12, 10 }, // | +25% |   0% | PFFT | GOOD | Best dps efficiency; bolt. Ball for reflecting targets.
{ I_MODE, "moderate",    5, 10,   0, 10,  0, 10, 10 }, // |      |      |      |      | Not inefficient in more situations, the macro/mana saver.
{ I_EXPA, "expanded",    7, 12,  +3,  9, +2,  8, 10 }, // | -25% | -10% | BEST | POOR | Best aoe efficiency; burst. Cloud for dense population.
{ I_BRIE, "brief",       8,  9,  +6,  6,  0,  6,  5 }, // | -33% | +20% |      | PFFT | 
{ I_MAXI, "maximized",  10, 28, +12, 14, +1, 12, 10 }, // | -50% | +40% | GOOD | GOOD | Best outright dps, except situational storm, swarm, flare.
{ I_ENHA, "enhanced",   10, 20,   0, 10,  0, 10, 10 }, // |      |      |      |      |
};                                                     // +------+------+------+------+

r_type r_types[RCRAFT_MAX_TYPES] =                                      // |   Moderate  |  Maximized  |  Enhanced*  | Table generated for 0.25 weight (base elements, +20% dps from high elements) Higher weight ruins the bolt advantage vs aoe spells for high elements. :/
{                                                                       // +------+------+------+------+------+------+--------------------------------------------------------------------------------------------------------------------------------------------------------
{ T_BOLT, "bolt",   5,  1, 15, 2,  1, 73, 20,  0,   0, 0,  0,  0,  0 }, // |  766 |  630 [ 1058 ]  870 |  766 |  630 | Bolt; +50% dps from ball. Beam; ~ball dps but more efficient, -25% compared to Swarm (but less restrictive). Base dmg matches istari manabolt damage, mana dmg is -100 dmg by comparison.
{ T_CLOU, "cloud", 10,  3, 20, 0,  0,  0,  0,  1, 163, 2,  2,  6, 14 }, // |  163 |  135 |  228 |  190 |  244 |  203 | Storm is +50% dps for much lower cost than a maximized cloud, with 2.0x duration and a fixed radius of 1. Might see more use by hybrids; more applicable than frost barrier for most elements.
{ T_SIGN, "sign",  15,  5, 20, 7, 10, 13, 20, 10, 100, 2,  2, 20, 45 }, // |      |      |      |      |      |      | Radius for Disarm is 1-4, Illumination is 5-11 (identical to Istari). Bolt damage matches stone-to-mud. Annihilation is -50% dps compared to the wand, 720 to Morgy/uniques, 2400 cap to PowerD/N/etc (better than flare if it hits).
{ T_BALL, "ball",  20,  5, 15, 0,  0,  0,  0, 21, 519, 2,  3,  0,  0 }, // |  519 [  432 ]  726 [  605 ]  951 [  792 ] Fireflash(r4): 430vs432 pre SP, 594vs605 post SP, 792(usually less)vs792(inefficient) to Morgy, 1188vs605*2 suscept cap. OrbofDraining(r3): 367, 734 to Morgy
{ T_WAVE, "wave",  25, 10, 30, 0,  0,  0,  0, 20, 200, 8, 12,  0,  0 }, // |  200 |  166 |  280 |  233 |  400 |  333 | -30% dps for mana surge compared to power staff, base elements are equivalent.
{ T_SIGL, "sigil", 30, 25, 25, 0,  0,  0,  0, 10, 100, 0,  0,  0,  0 }, // |      |      |      |      |      |      |
{ T_BURS, "burst", 35,  3, 15, 1,  1, 32, 32, 21, 519, 1,  2,  0,  0 }, // |  519 |  432 |  726 |  605 [ 1056 ]  870 | Flare; +10% dps over swarm (and not area restricted), but -33% HP efficiency. oO Intended to mimic average bolt damage.
};                                                                      // +------+------+------+------+------+------+
                                                                        // | 1200 |  400 | 1200 |  400 | 1200 |  400 |
r_projection r_projections[RCRAFT_MAX_PROJECTIONS] = {
{ R_LITE,          GF_LITE,       400, "light",           TR2_RES_LITE },
{ R_DARK,          GF_DARK,       550, "darkness",        TR2_RES_DARK },
{ R_NEXU,          GF_NEXUS,      250, "nexus",           TR2_RES_NEXUS },
{ R_NETH,          GF_NETHER,     550, "nether",          TR2_RES_NETHER },
{ R_CHAO,          GF_CHAOS,      600, "chaos",           TR2_RES_CHAOS },
{ R_MANA,          GF_MANA,       600, "mana",            TR5_RES_MANA },

{ R_LITE | R_DARK, GF_CONFUSION,  400, "confusion",       TR2_RES_CONF },
{ R_LITE | R_NEXU, GF_INERTIA,    200, "inertia",         TR2_FREE_ACT },
{ R_LITE | R_NETH, GF_ELEC,      1200, "lightning",       TR2_RES_ELEC },
{ R_LITE | R_CHAO, GF_FIRE,      1200, "fire",            TR2_RES_FIRE },
{ R_LITE | R_MANA, GF_WAVE,       300, "water",           TR5_RES_WATER },

{ R_DARK | R_NEXU, GF_GRAVITY,    150, "gravity",         TR3_FEATHER },
{ R_DARK | R_NETH, GF_COLD,      1200, "frost",           TR2_RES_COLD },
{ R_DARK | R_CHAO, GF_ACID,      1200, "acid",            TR2_RES_ACID },
{ R_DARK | R_MANA, GF_POIS,       800, "poison",          TR2_RES_POIS },

{ R_NEXU | R_NETH, GF_SHARDS,     400, "shards",          TR2_RES_SHARDS },
{ R_NEXU | R_CHAO, GF_SOUND,      400, "sound",           TR2_RES_SOUND },
{ R_NEXU | R_MANA, GF_TIME,       150, "time",            TR5_RES_TIME },

{ R_NETH | R_CHAO, GF_DISENCHANT, 500, "disenchantment",  TR2_RES_DISEN },
{ R_NETH | R_MANA, GF_ICE,        933, "ice",             TR2_RES_COLD | TR2_RES_SHARDS },

{ R_CHAO | R_MANA, GF_PLASMA,     933, "plasma",          TR2_RES_ELEC | TR2_RES_FIRE | TR2_RES_SOUND },
};
