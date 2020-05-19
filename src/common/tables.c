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
magic_type innate_powers[128] = {
/* 0(lev), mana, fail, 0(exp), ftk */
// RF4_SHRIEK                   0x00000001      /* Shriek for help */
  {0, 2, 0, 0, 0},
// RF4_UNMAGIC                  0x00000002      /* (?) */
  {0, 0, 0, 0, 0},
// (S_ANIMAL) RF4_XXX3          0x00000004      /* (?) */
  {0, 0, 0, 0, 0},
// RF4_ROCKET                   0x00000008      /* (?) */
  {0, 60, 70, 0, 2},
// RF4_ARROW_1                  0x00000010      /* Fire an arrow (light) */
  {0, 2, 5, 0, 1},
// RF4_ARROW_2                  0x00000020      /* Fire an shot (heavy) */
  {0, 2, 6, 0, 1},
// XXX (RF4_ARROW_3)            0x00000040      /* Fire bolt (heavy) */
  {0, 2, 7, 0, 1},
// XXX (RF4_ARROW_4)            0x00000080      /* Fire missiles (heavy) */
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
// 0x40000000                   0x40000000
  {0, 0, 0, 0, 0},
// RF4_BOULDER                  0x80000000
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
// RF6_DARKNESS                 0x00001000      /* Create Darkness */
  {0, 6, 8, 0, 0},
// RF6_TRAPS                    0x00002000      /* Create Traps */
  {0, 15, 25, 0, 0},
// RF6_FORGET                   0x00004000      /* Cause amnesia */
  {0, 25, 35, 0, 0},
// RF6_XXX5                     0x00008000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x00010000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x00020000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x00040000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x00080000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x00100000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x00200000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x00400000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x00800000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x01000000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x02000000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x04000000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x08000000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x10000000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x20000000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x40000000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF6_XXX5                     0x80000000      /* (unavailable) */
  {0, 0, 0, 0, 0},

/*
 * New monster race bit flags
 */
// RF0_XXX3                     0x00000001      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00000002      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00000004      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00000008      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00000010      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00000020      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00000040      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_BO_DISE			0x00000080
  {0, 15, 20, 0, 1},
// RF0_BA_DISE			0x00000100
  {0, 30, 40, 0, 1},
// RF0_XXX5                     0x00000200      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00000400      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00000800      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00001000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00002000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00004000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00008000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00010000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00020000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_BR_ICE			0x00040000
  {0, 20, 27, 0, 2},
// RF0_BR_WATER			0x00080000
  {0, 25, 30, 0, 2},

// RF0_XXX3                     0x00100000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00200000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00400000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x00800000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x01000000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x02000000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x04000000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_BO_DISE			0x08000000
  {0, 15, 20, 0, 1},
// RF0_BA_DISE			0x10000000
  {0, 30, 40, 0, 1},
// RF0_XXX5                     0x20000000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x40000000      /* (unavailable) */
  {0, 0, 0, 0, 0},
// RF0_XXX5                     0x80000000      /* (unavailable) */
  {0, 0, 0, 0, 0},
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
  {"Toxic Waste Ball", TRUE},
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

monster_spell_type monster_spells0[32] = {
  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},

  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},
  {"Disenchantment Bolt", TRUE},

  {"Disenchantment Ball", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},

  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},
  {"XXX", TRUE},

  {"XXX", TRUE},
  {"XXX", TRUE},
  {"Breathe Ice", TRUE},
  {"Breathe Water", TRUE},

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
