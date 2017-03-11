/* $Id$ */
/* File: birth.c */

/* Purpose: create a player character */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"

/*
 * Limit the starting stats to max. of 18/40, so that some players
 * won't keep on suiciding for better stats(esp. 18/50+).
 * This option also bottom-up the starting stats somewhat to
 * compensate it.		- Jir -
 */
#if 0
#define STARTING_STAT_LIMIT
#endif

/*
 * Forward declare
 */
typedef struct birther birther;

/*
 * A structure to hold "rolled" information
 */
struct birther {
	s16b age;
	s16b wt;
	s16b ht;
	s16b sc;

	s32b au;

	s16b stat[6];

	char history[4][60];
};



/*
 * Forward declare
 */
typedef struct hist_type hist_type;

/*
 * Player background information
 */
struct hist_type {
	cptr info;			/* Textual History */

	byte roll;			/* Frequency of this entry */
	byte chart;			/* Chart index */
	byte next;			/* Next chart index */
	byte bonus;			/* Social Class Bonus + 50 */
};


/*
 * Background information (see below)
 *
 * Chart progression by race:
 *   Human/Dunadan -->  1 -->  2 -->  3 --> 50 --> 51 --> 52 --> 53
 *   Half-Elf      -->  4 -->  1 -->  2 -->  3 --> 50 --> 51 --> 52 --> 53
 *   Elf/High-Elf  -->  7 -->  8 -->  9 --> 54 --> 55 --> 56
 *   Hobbit        --> 10 --> 11 -->  3 --> 50 --> 51 --> 52 --> 53
 *   Gnome         --> 13 --> 14 -->  3 --> 50 --> 51 --> 52 --> 53
 *   Dwarf         --> 16 --> 17 --> 18 --> 57 --> 58 --> 59 --> 60 --> 61
 *   Half-Orc      --> 19 --> 20 -->  2 -->  3 --> 50 --> 51 --> 52 --> 53
 *   Half-Troll    --> 22 --> 23 --> 62 --> 63 --> 64 --> 65 --> 66
 *
 * XXX XXX XXX This table *must* be correct or drastic errors may occur!
 */
static hist_type bg[] = {
	{"You are the illegitimate and unacknowledged child ",	 10,  1,  2,  25},
	{"You are the illegitimate but acknowledged child ",	 20,  1,  2,  35},
	{"You are one of several children ",			 95,  1,  2,  45},
	{"You are the first child ",				100,  1,  2,  50},

	{"of a Serf. ",						 40,  2,  3,  65},
	{"of a Yeoman. ",					 65,  2,  3,  80},
	{"of a Townsman. ",					 80,  2,  3,  90},
	{"of a Guildsman. ",					 90,  2,  3, 105},
	{"of a Landed Knight. ",				 96,  2,  3, 120},
	{"of a Titled Noble. ",					 99,  2,  3, 130},
	{"of a Royal Blood Line. ",				100,  2,  3, 140},

	/* Human/Gnome/Halfling */
	{"You are the black sheep of the family. ",		 20,  3, 50, 20},
	{"You are a credit to the family. ",			 80,  3, 50, 55},
	{"You are a well liked child. ",			100,  3, 50, 60},

	{"Your mother was of the Teleri. ",			 40,  4,  1, 50},
	{"Your father was of the Teleri. ",			 75,  4,  1, 55},
	{"Your mother was of the Noldor. ",			 90,  4,  1, 55},
	{"Your father was of the Noldor. ",			 95,  4,  1, 60},
	{"Your mother was of the Vanyar. ",			 98,  4,  1, 65},
	{"Your father was of the Vanyar. ",			100,  4,  1, 70},

	{"You are one of several children ",			 60,  7,  8, 50},
	{"You are the only child ",				100,  7,  8, 55},

	{"of a Teleri ",					 75,  8,  9, 50},
	{"of a Noldor ",					 95,  8,  9, 55},
	{"of a Vanyar ",					100,  8,  9, 60},

	{"Ranger. ",						 40,  9, 54,  80},
	{"Archer. ",						 70,  9, 54,  90},
	{"Warrior. ",						 87,  9, 54, 110},
	{"Mage. ",						 95,  9, 54, 125},
	{"Prince. ",						 99,  9, 54, 140},
	{"King. ",						100,  9, 54, 145},

	{"You are one of several children of a Hobbit ",	 85, 10, 11, 45},
	{"You are the only child of a Hobbit ",			100, 10, 11, 55},

	{"Bum. ",						 20, 11,  3,  55},
	{"Tavern Owner. ",					 30, 11,  3,  80},
	{"Miller. ",						 40, 11,  3,  90},
	{"Home Owner. ",					 50, 11,  3, 100},
	{"Burglar. ",						 80, 11,  3, 110},
	{"Warrior. ",						 95, 11,  3, 115},
	{"Mage. ",						 99, 11,  3, 125},
	{"Clan Elder. ",					100, 11,  3, 140},

	/* Gnome */
	{"You are one of several children of a Gnome ",		 85, 13, 14, 45},
	{"You are the only child of a Gnome ",			100, 13, 14, 55},

	{"Beggar. ",						 20, 14,  3,  55},
	{"Braggart. ",						 50, 14,  3,  70},
	{"Prankster. ",						 75, 14,  3,  85},
	{"Warrior. ",						 95, 14,  3, 100},
	{"Mage. ",						100, 14,  3, 125},

	/* Dwarf */
	{"You are one of two children of a Dwarven ",		 25, 16, 17, 40},
	{"You are the only child of a Dwarven ",		100, 16, 17, 50},

	{"Thief. ",						 10, 17, 18,  60},
	{"Prison Guard. ",					 25, 17, 18,  75},
	{"Miner. ",						 75, 17, 18,  90},
	{"Warrior. ",						 90, 17, 18, 110},
	{"Priest. ",						 99, 17, 18, 130},
	{"King. ",						100, 17, 18, 150},

	{"You are the black sheep of the family. ",		 15, 18, 57, 10},
	{"You are a credit to the family. ",			 85, 18, 57, 50},
	{"You are a well liked child. ",			100, 18, 57, 55},

	/* Orc/Goblin --todo: more Goblin stuff */
	{"Your mother was an Orc, but it is unacknowledged. ",	 25, 19, 20, 25},
	{"Your father was an Orc, but it is unacknowledged. ",	100, 19, 20, 25},

	{"You are the adopted child ",				100, 20, 2, 50},

	/* Troll */
	{"Your mother was a Cave-Troll ",			 30, 22, 23, 20},
	{"Your father was a Cave-Troll ",			 60, 22, 23, 25},
	{"Your mother was a Hill-Troll ",			 75, 22, 23, 30},
	{"Your father was a Hill-Troll ",			 90, 22, 23, 35},
	{"Your mother was a Water-Troll ",			 95, 22, 23, 40},
	{"Your father was a Water-Troll ",			100, 22, 23, 45},

	{"Cook. ",						  5, 23, 62, 60},
	{"Warrior. ",						 95, 23, 62, 55},
	{"Shaman. ",						 99, 23, 62, 65},
	{"Clan Chief. ",					100, 23, 62, 80},

	/* Human features */
	{"You have dark brown eyes, ",				 20, 50, 51, 50},
	{"You have brown eyes, ",				 60, 50, 51, 50},
	{"You have hazel eyes, ",				 70, 50, 51, 50},
	{"You have green eyes, ",				 80, 50, 51, 50},
	{"You have blue eyes, ",				 90, 50, 51, 50},
	{"You have blue-gray eyes, ",				100, 50, 51, 50},

	{"straight ",						 70, 51, 52, 50},
	{"wavy ",						 90, 51, 52, 50},
	{"curly ",						100, 51, 52, 50},

	{"black hair, ",					 30, 52, 53, 50},
	{"brown hair, ",					 70, 52, 53, 50},
	{"auburn hair, ",					 80, 52, 53, 50},
	{"red hair, ",						 90, 52, 53, 50},
	{"blond hair, ",					100, 52, 53, 50},

	{"and a very dark complexion.",				 10, 53, 0, 50},
	{"and a dark complexion.",				 30, 53, 0, 50},
	{"and an average complexion.",				 80, 53, 0, 50},
	{"and a fair complexion.",				 90, 53, 0, 50},
	{"and a very fair complexion.",				100, 53, 0, 50},

	/* Elven features */
	{"You have light grey eyes, ",				 85, 54, 55, 50},
	{"You have light blue eyes, ",				 95, 54, 55, 50},
	{"You have light green eyes, ",				100, 54, 55, 50},

	{"straight ",						 75, 55, 56, 50},
	{"wavy ",						100, 55, 56, 50},

	{"black hair, and a fair complexion.",			 75, 56,  0, 50},
	{"brown hair, and a fair complexion.",			 85, 56,  0, 50},
	{"blond hair, and a fair complexion.",			 95, 56,  0, 50},
	{"silver hair, and a fair complexion.",			100, 56,  0, 50},

	/* Dwarven features */
	{"You have dark brown eyes, ",				 99, 57, 58, 50},
	{"You have glowing red eyes, ",				100, 57, 58, 60},

	{"straight ",						 90, 58, 59, 50},
	{"wavy ",						100, 58, 59, 50},

	{"black hair, ",					 75, 59, 60, 50},
	{"brown hair, ",					100, 59, 60, 50},

	{"a one foot beard, ",					 25, 60, 61, 50},
	{"a two foot beard, ",					 60, 60, 61, 51},
	{"a three foot beard, ",				 90, 60, 61, 53},
	{"a four foot beard, ",					100, 60, 61, 55},

	{"and a dark complexion.",				100, 61, 0, 50},

	/* Trollish features */
	{"You have slime green eyes, ",				 60, 62, 63, 50},
	{"You have puke yellow eyes, ",				 85, 62, 63, 50},
	{"You have blue-bloodshot eyes, ",			 99, 62, 63, 50},
	{"You have glowing red eyes, ",				100, 62, 63, 55},

	{"dirty ",						 33, 63, 64, 50},
	{"mangy ",						 66, 63, 64, 50},
	{"oily ",						100, 63, 64, 50},

	{"sea-weed green hair, ",				 33, 64, 65, 50},
	{"bright red hair, ",					 66, 64, 65, 50},
	{"dark purple hair, ",					100, 64, 65, 50},

	{"and green ",						 25, 65, 66, 50},
	{"and blue ",						 50, 65, 66, 50},
	{"and white ",						 75, 65, 66, 50},
	{"and black ",						100, 65, 66, 50},

	{"ulcerous skin.",					 33, 66,  0, 50},
	{"scabby skin.",					 66, 66,  0, 50},
	{"leprous skin.",					100, 66,  0, 50},


	/* Draconian (text inspired by Hengband -- hey, they had Draconians too?) */
	{"You are ",						100, 89, 90, 50 },

	{"the oldest child of a Draconian ",			 30, 90, 91, 55 },
	{"the youngest child of a Draconian ",			 50, 90, 91, 50 },
	{"the adopted child of a Draconian ",			 55, 90, 91, 50 },
	{"an orphaned child of a Draconian ",			 60, 90, 91, 45 },
	{"one of several children of a Draconian ",		 85, 90, 91, 50 },
	{"the only child of a Draconian ",			100, 90, 91, 55 },

	//{"Beggar. ", 10, 90, 91, 20 },
	{"Thief. ",						 16, 91, 92, 30 },
	{"Sailor. ",						 24, 91, 92, 45 },
	{"Mercenary. ",						 42, 91, 92, 45 },
	{"Warrior. ",						 73, 91, 92, 50 },
	{"Merchant. ",						 78, 91, 92, 50 },
	{"Artisan. ",						 86, 91, 92, 55 },
	{"Healer. ",						 91, 91, 92, 60 },
	{"Priest. ",						 96, 91, 92, 65 },
	{"Mage. ",						100, 91, 92, 70 },
	//{"Scholar. ", 99, 90, 91, 80 },
	//{"Noble. ", 100, 90, 91, 100 },

	{"You have ",						100, 92, 93, 50 },

#ifndef ENABLE_DRACONIAN_TRAITS
	{"charcoal wings, charcoal skin and a smoke-gray belly. ",11, 93, 94, 50 },
	{"bronze wings, bronze skin, and a copper belly. ",	 16, 93, 94, 50 },
	{"golden wings, and golden skin. ",			 24, 93, 94, 50 },
	{"white wings, and white skin. ",			 26, 93, 94, 60 },
	{"blue wings, blue skin, and a cyan belly. ",		 32, 93, 94, 50 },
	{"multi-hued wings, and multi-hued skin. ",		 33, 93, 94, 70 },
	{"brown wings, and brown skin. ",			 37, 93, 94, 45 },
	{"black wings, black skin, and a white belly. ",	 41, 93, 94, 50 },
	{"lavender wings, lavender skin, and a white belly. ",	 48, 93, 94, 50 },
	{"green wings, green skin and yellow belly. ",		 65, 93, 94, 50 },
	{"green wings, and green skin. ",			 75, 93, 94, 50 },
	{"red wings, and red skin. ",				 88, 93, 94, 50 },
	{"black wings, and black skin. ",			 94, 93, 94, 50 },
	{"metallic skin, and shining wings. ",			100, 93, 94, 55},
#else
	//changed social class values for draconian trait implementation
	{"blue wings, blue skin, and a cyan belly. ",		  6, 93, 94, 50 },
	{"white wings, and white skin. ",			  8, 93, 94, 50 },
	{"red wings, and red skin. ",				 21, 93, 94, 50 },
	{"charcoal wings, charcoal skin and a smoke-gray belly. ",32, 93, 94, 50 },
	{"black wings, black skin, and a white belly. ",	 36, 93, 94, 55 },
	{"black wings, and black skin. ",			 42, 93, 94, 50 },
	{"green wings, green skin and yellow belly. ",		 59, 93, 94, 50 },
	{"green wings, and green skin. ",			 69, 93, 94, 50 },
	{"multi-hued wings, and multi-hued skin. ",		 70, 93, 94, 60 },
	{"bronze wings, bronze skin, and a copper belly. ",	 75, 93, 94, 50 },
	{"metallic skin, and shining wings. ",			 81, 93, 94, 55 },
	{"golden wings, and golden skin. ",			 89, 93, 94, 60 },
	{"brown wings, and brown skin. ",			 93, 93, 94, 50 },
	{"lavender wings, lavender skin, and a white belly. ",	100, 93, 94, 60 },
#endif
#if 0
	{"You have a Green Eagle.",				 30, 94,  0, 40},
	{"You have a Blue Eagle.",				 55, 94,  0, 60},
	{"You have a Brown Eagle.",				 80, 94,  0, 80},
	{"You have a Bronze Eagle.",				 90, 94,  0, 100},
	{"You have a Golden Eagle.",				100, 94,  0, 120},
#else
	{"",							100, 94,  0, 50},
#endif


	/* Ent (text inspired by Hengband) */
	{"You are of an unknown generation of the Ents. ",	 30, 95, 96, 30},
	{"You are of the third generation of the Ents. ",	 40, 95, 96, 50},
	{"You are of the second generation of the Ents. ",	 60, 95, 96, 60},
	{"You are of the first beings who awoke on Arda. ",	100, 95, 96, 80},

	{"You have ",						100, 96, 98, 50},

	{"three fingers and toes, and are covered in ",		  5, 98, 99, 50},
	{"four fingers and toes, and are covered in ",		 20, 98, 99, 50},
	{"five fingers and toes, and are covered in ",		 40, 98, 99, 50},
	{"six fingers and toes, and are covered in ",		 60, 98, 99, 50},
	{"seven fingers and toes, and are covered in ",		 80, 98, 99, 50},
	{"eight fingers and toes, and are covered in ",		 95, 98, 99, 50},
	{"nine fingers and toes, and are covered in ",		100, 98, 99, 50},

	{"scaly brown skin.",					 10, 99,  0, 50},
	{"rough brown skin.",					 20, 99,  0, 50},
	{"smooth grey skin.",					 30, 99,  0, 50},
	{"dark green skin.",					 40, 99,  0, 50},
	{"mossy skin.",						 50, 99,  0, 50},
	{"deep brown skin.",					 60, 99,  0, 50},
	{"pale brown, flaky skin.",				 70, 99,  0, 50},
	{"rich chocolate-colored skin.",			 80, 99,  0, 50},
	{"ridged black skin.",					 90, 99,  0, 50},
	{"thick, almost corky skin.",				100, 99,  0, 50},


	/* Maiar background */
	{"You are a Maiar spirit, bound to ", 				100,100,101, 70},
#if 1
	{"Manwe Sulimo, King of the Valar. ",				 10,101,102,150},
	{"Ulmo, King of the Sea. ",					 20,101,102,100},
	{"Aule, the Smith. ",						 30,101,102,100},
	{"Orome Aldaron, the Great Rider and Hunter of Valinor. ",	 40,101,102,120},
	{"Mandos, Judge of the Dead. ",					 50,101,102,100},
	{"Irmo, Master of Dreams and Desires. ",			 60,101,102,100},
	{"Tulkas Astaldo, Champion of Valinor. ",			 65,101,102,130},
	{"Varda Elentari, Queen of the Stars, wife of Manwe. ",		 70,101,102,120},
	{"Yavanna Kementari, Giver of Fruits, wife of Aule. ",		 75,101,102, 90},
	{"Nienna, Lady of Mercy. ",					 80,101,102, 90},
	{"Este the Gentle. ",						 85,101,102, 90},
	{"Vaire the Weaver. ",						 90,101,102, 90},
	{"Vana the Ever-young. ",					 95,101,102, 90},
	{"Nessa the Dancer. ",						100,101,102, 90},

	{"You were sent to assist the Elves ",				 25,102,103,  0},
	{"You suspend your pilgrimage in Aman ",			 50,102,104,  0},
	{"Your ambition compels your physical manifestation in Arda. ",	 63,102, 50,  0},
	{"Your ambition compels your physical manifestation in Arda. ",	 75,102, 54,  0},
	{"You were ordered to assist the humans and elves ",		100,102,105,  0},
	{"that stayed in the East. ",					100,103, 54,  0},
	{"to spend time in the mortal land. ",				100,104, 50,  0},
	{"in the West. ",						 50,105, 50,  0},
	{"in the West. ",						100,105, 54,  0},
	//{"Your physical form is Elven in feature. ", 			 50,102, 54, 70},
	//{"You manifest as a Human to casual observers. ", 		100,102, 50, 70},
#else /* for testing max len */
	{"Orome Aldaron, the Great Rider and Hunter of Valinor. ",	100,101,102,120},
	{"You suspend your pilgrimage in Aman ",			100,102,104,  0},
	{"to spend time in the mortal land. ",				100,104, 50,  0},
#endif

	/* Kobold background */
	{"You are the runt of ",				 20, 110, 111, 40},
	{"You come from ",					 80, 110, 111, 50},
	{"You are the largest of ",				100, 110, 111, 55},

	{"a litter of 3 pups. ",				 15, 111, 112, 45},
	{"a litter of 4 pups. ",				 40, 111, 112, 45},
	{"a litter of 5 pups. ",				 70, 111, 112, 50},
	{"a litter of 6 pups. ",				 85, 111, 112, 50},
	{"a litter of 7 pups. ",				 95, 111, 112, 55},
	{"a litter of 8 pups. ",				100, 111, 112, 55},

	{"Your father was a fungus farmer, ",			 25, 112, 113, 40},
	{"Your father was a hunter, ",				 50, 112, 113, 45},
	{"Your father was a warrior, ",				 75, 112, 113, 50},
	{"Your father was a shaman, ",				 95, 112, 113, 55},
	{"Your father was the tribal chief, ",			100, 112, 113, 60},

	{"and your mother was a prisoner of war. ",		 20, 113, 114, 45},
	{"and your mother was a cook. ",			 95, 113, 114, 50},
	{"and your mother was one of the Chief's harem. ",	100, 113, 114, 55},

	{"You have black eyes, ",				 10, 114, 115, 50},
	{"You have dark brown eyes, ",				 40, 114, 115, 50},
	{"You have brown eyes, ",				 80, 114, 115, 50},
	{"You have light brown eyes, ",				 99, 114, 115, 50},
	{"You have glowing red eyes, ",				100, 114, 115, 50},

	{"a dark brown hide, ",					 40, 115, 116, 50},
	{"a reddish-brown hide, ",				 60, 115, 116, 50},
	{"an olive green hide, ",				 95, 115, 116, 50},
	{"a deep blue hide, ",					100, 115, 116, 50},

	{"and large, flat teeth.",				 10, 116,   0, 50},
	{"and small, sharp teeth.",				 90, 116,   0, 50},
	{"and large, sharp teeth.",				100, 116,   0, 50},


	/* Dark-Elf (text inspired by Hengband) */
	{"You are one of several children of a Dark Elven ",	 85, 120, 121, 45},
	{"You are the only child of a Dark Elven ",		100, 120, 121, 55},

	{"Warrior. ",						 50, 121, 122, 60},
	{"Warlock. ",						 80, 121, 122, 75},
	{"Noble. ",						100, 121, 122, 95},

	{"You have black eyes, ",				 60, 122, 123, 50},
	{"You have red eyes, ",					 85, 122, 123, 50},
	{"You have purple eyes, ",				100, 122, 123, 50},

	{"straight ",						 70, 123, 124, 50},
	{"wavy ",						 90, 123, 124, 50},
	{"curly ",						100, 123, 124, 50},

	{"black hair ",						 50, 124, 125, 50},
	{"white hair ",						100, 124, 125, 50},

	{"and a very dark complexion.",				100, 125,   0,  0},


	/* Vampire (text inspired by Hengband) */
	{"You arose from an unmarked grave. ", 			 20, 130, 132, 30},
	{"In life you were a simple peasant. ",			 50, 130, 132, 40},
	{"In life you were a decent citizen. ",			 70, 130, 132, 50},
	//{"the victim of a powerful Vampire Lord. ",		100, 131, 132,  0},
	{"In life you were a Vampire Hunter, but they got you. ",75, 130, 132, 55},
	{"In life you were a Necromancer. ",			 80, 130, 132, 60},
	{"In life you were a powerful noble. ",			 95, 130, 132, 70},
	{"In life you were a powerful and cruel tyrant. ",	100, 130, 132, 75},

	{"You were bitten by ",					100, 132, 133, 50},

	{"a renegade vampire. ",				 20, 133, 134, 40},
	{"a vampire serving an unknown house. ",		 50, 133, 134, 45},
	{"a vampire serving a major house. ",			 85, 133, 134, 50},
	{"an actual vampire noble. ",				 95, 133, 134, 55},
	{"a powerful vampire lord himself. ",			100, 133, 134, 60},

	{"You have ",						100, 134, 135, 50},

	{"jet-black hair, ",					 25, 135, 136, 50},
	{"matted brown hair, ",					 50, 135, 136, 50},
	{"white hair, ",					 75, 135, 136, 50},
	{"a hairless head, ",					100, 135, 136, 50},

	{"eyes like red coals, ",				 25, 136, 137, 50},
	{"blank white eyes, ",					 50, 136, 137, 50},
	{"feral yellow eyes, ",					 75, 136, 137, 50},
	{"bloodshot red eyes, ",				100, 136, 137, 50},

	{"and a deathly pale complexion.",			100, 137,   0, 50},
};



/*
 * Current stats
 */
static s16b stat_use[6];



/*
 * Returns adjusted stat -JK-
 * Algorithm by -JWT-
 *
 * auto_roll is boolean and states maximum changes should be used rather
 * than random ones to allow specification of higher values to wait for
 *
 * The "p_ptr->maximize" code is important      -BEN-
 */
static int adjust_stat(int Ind, int value, s16b amount, int auto_roll) {
	player_type *p_ptr = Players[Ind];
	int i;

	/* Negative amounts */
	if (amount < 0) {
		/* Apply penalty */
		for (i = 0; i < (0 - amount); i++) {
			if (value >= 18+10)
				value -= 10;
			else if (value > 18)
				value = 18;
			else if (value > 3)
				value--;
		}
	}

	/* Positive amounts */
	else if (amount > 0) {
		/* Apply reward */
		for (i = 0; i < amount; i++) {
			if (value < 18)
				value++;
			else if (p_ptr->maximize)
				value += 10;
			else if (value < 18+70)
				value += ((auto_roll ? 15 : randint(15)) + 5);
			else if (value < 18+90)
				value += ((auto_roll ? 6 : randint(6)) + 2);
			else if (value < 18+100)
				value++;
		}
	}

	/* Return the result */
	return (value);
}




/*
 * Roll for a characters stats
 *
 * For efficiency, we include a chunk of "calc_boni()".
 */
static bool get_stats(int Ind, int stat_order[6]) {
	player_type *p_ptr = Players[Ind];
	int i, j, tries = 1000;

	int bonus;

	int dice[18];
	int stats[6];

	int free_points = 30, maxed_stats = 0;

	/* Clear "stats" array */
	for (i = 0; i < 6; i++)
		stats[i] = 0;

	/* Traditional random stat rolling */
	if (CHAR_CREATION_FLAGS == 0) {

		/* Check over the given stat order, to prevent cheating */
		for (i = 0; i < 6; i++) {
			/* Check range */
			if (stat_order[i] < 0 || stat_order[i] > 5)
				stat_order[i] = 1;

			/* Check for duplicated entries */
			if (stats[stat_order[i]] == 1) {
				/* Find a stat that hasn't been specified yet */
				for (j = 0; j < 6; j++) {
					if (stats[j]) continue;
					stat_order[i] = j;
				}
			}

			/* Set flag */
			stats[stat_order[i]] = 1;
		}

		/* Roll and verify some stats */
		while (--tries) {
			/* Roll some dice */
			for (j = i = 0; i < 18; i++) {
				/* Roll the dice */
				dice[i] = randint(3 + i % 3);

				/* Collect the maximum */
				j += dice[i];
			}

			/* Verify totals */
#ifdef STARTING_STAT_LIMIT
			if ((j > 48) && (j < 58)) break;
#else
			if ((j > 42) && (j < 54)) break;
#endif
		}

		/* Acquire the stats */
		for (i = 0; i < 6; i++) {
			/* Extract 5 + 1d3 + 1d4 + 1d5 */
			j = 5 + dice[3*i] + dice[3*i+1] + dice[3*i+2];

			/* Save that value */
			stats[i] = j;
		}

		/* Now sort the stats */
		/* I use a bubble sort because I'm lazy at the moment */
		for (i = 0; i < 6; i++) {
			for (j = 0; j < 5; j++) {
				if (stats[j] < stats[j + 1]) {
					int t;

					t = stats[j];
					stats[j] = stats[j + 1];
					stats[j + 1] = t;
				}
			}
		}

		/* Now, put them in the correct order */
		for (i = 0; i < 6; i++)
			p_ptr->stat_max[stat_order[i]] = stats[i];

		/* Adjust the stats */
		for (i = 0; i < 6; i++) {
			/* Obtain a "bonus" for "race" and "class" */
			bonus = p_ptr->rp_ptr->r_adj[i] + p_ptr->cp_ptr->c_adj[i];

			/* Variable stat maxes */
			if (p_ptr->maximize) {
#ifdef STARTING_STAT_LIMIT
				if (!is_fighter(p_ptr))
					while (modify_stat_value(p_ptr->stat_max[i], bonus) > 18 + 40)
						p_ptr->stat_max[i]--;

#endif	//STARTING_STAT_LIMIT
				/* Start fully healed */
				p_ptr->stat_cur[i] = p_ptr->stat_max[i];

				/* Efficiency -- Apply the racial/class bonuses */
				stat_use[i] = modify_stat_value(p_ptr->stat_max[i], bonus);
			}

			/* Fixed stat maxes */
			else {
				/* Apply the bonus to the stat (somewhat randomly) */
				stat_use[i] = adjust_stat(Ind, p_ptr->stat_max[i], bonus, FALSE);

				/* Save the resulting stat maximum */
				p_ptr->stat_cur[i] = p_ptr->stat_max[i] = stat_use[i];
        		}
		}
	}

	/* Players may modify their stats manually */
	else {
		for (i = 0; i < 6; i++) {
			bonus = p_ptr->rp_ptr->r_adj[i] + p_ptr->cp_ptr->c_adj[i];

			/* Fix limits - all cases here cover malicious client-side cheating attempts :) */
			/* Stat may only be reduced by up to 2 points */
			if (stat_order[i] < 8) stat_order[i] = 8;
			/* Stat may not be raised by more than 7 points */
			if (stat_order[i] > 17) stat_order[i] = 17;
			/* Only one of the stats is allowed to be maximized */
			if (stat_order[i] == 17) {
				if (!maxed_stats) maxed_stats++;
				else stat_order[i] = 16;
			}
			/* Stat may only be decreased as long as it's > 3 (including the r/c bonus) */
			if (stat_order[i] + bonus < 3 && stat_order[i] < 10)
				stat_order[i] = (3 + bonus > 10) ? 10 : 3 + bonus;

			/* Count skill points needed to reach these stats */
			if (stat_order[i] <= 12) free_points -= (stat_order[i] - 10);
			else if (stat_order[i] <= 14) free_points -= (2 + (stat_order[i] - 12) * 2);
			else if (stat_order[i] <= 16) free_points -= (6 + (stat_order[i] - 14) * 3);
			else free_points -= 16; /* ouch */
		}

		/* If client has been hacked or a version desync error occured, quit. */
		if (free_points < 0) {
			s_printf("EXPLOIT: %s allocates too many (+%d) stat points.\n", p_ptr->name, -free_points);
			return FALSE;
		} else if (free_points) s_printf("STATPOINTS: %s allocates not all (-%d) stat points.\n", p_ptr->name, free_points);

		/* Apply selected stats */
		for (i = 0; i < 6; i++) {
			bonus = p_ptr->rp_ptr->r_adj[i] + p_ptr->cp_ptr->c_adj[i];
			p_ptr->stat_cur[i] = p_ptr->stat_max[i] = stat_order[i];
			stat_use[i] = modify_stat_value(p_ptr->stat_max[i], bonus);
		}
	}

	return TRUE;
}


/*
 * Roll for some info that the auto-roller ignores
 * NOTE: Keep lua_recalc_char() in sync with this if you use it.
 */
static void get_extra(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i, j, min_value, max_value, min_value_king = 0, max_value_king = 9999;
	int tries = 500;

	/* Experience factor */
/* This one is too harsh for TLs and too easy on yeeks
	p_ptr->expfact = p_ptr->rp_ptr->r_exp * (100 + p_ptr->cp_ptr->c_exp) / 100;
*/	p_ptr->expfact = p_ptr->rp_ptr->r_exp + p_ptr->cp_ptr->c_exp;

	/* Hitdice */
	p_ptr->hitdie = p_ptr->rp_ptr->r_mhp + p_ptr->cp_ptr->c_mhp;

	/* Assume base hitpoints (fully healed) */
	p_ptr->chp = p_ptr->mhp = p_ptr->hitdie;
	p_ptr->csane = p_ptr->msane =
		((int)(adj_con_mhp[p_ptr->stat_ind[A_WIS]]) - 128) / 2 + 10;

#if 0 /* experimental (not urgent probably): 0'ing this, see below */
	/* Minimum hitpoints at highest level */
	min_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 3) / 8;
	min_value += PY_MAX_LEVEL;

	/* Maximum hitpoints at highest level */
	max_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 5) / 8;
	max_value += PY_MAX_LEVEL;
#endif
#if 0 /* narrow HP range - 300 tries */
	/* Minimum hitpoints at kinging level */
	min_value_king = (50 * (p_ptr->hitdie - 1) * 15) / 32;
	min_value_king += 50;
	/* Maximum hitpoints at kinging level */
	max_value_king = (50 * (p_ptr->hitdie - 1) * 17) / 32;
	max_value_king += 50;

	/* Minimum hitpoints at highest level */
	min_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 15) / 32;
	min_value += PY_MAX_LEVEL;
	/* Maximum hitpoints at highest level */
	max_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 17) / 32;
	max_value += PY_MAX_LEVEL;
#endif
#if 1 /* narrower HP range */
	/* Minimum hitpoints at kinging level */
	min_value_king = (50 * (p_ptr->hitdie - 1) * 31) / 64;
	min_value_king += 50;
	/* Maximum hitpoints at kinging level */
	max_value_king = (50 * (p_ptr->hitdie - 1) * 33) / 64;
	max_value_king += 50;

	/* Minimum hitpoints at highest level */
	min_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 31) / 64;
	min_value += PY_MAX_LEVEL;
	/* Maximum hitpoints at highest level */
	max_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 33) / 64;
	max_value += PY_MAX_LEVEL;
#endif

	/* Pre-calculate level 1 hitdice */
	p_ptr->player_hp[0] = p_ptr->hitdie;

	/* Roll out the hitpoints */
	while (--tries) {
		/* Roll the hitpoint values */
		for (i = 1; i < PY_MAX_LEVEL; i++) {
			/* Maybe this is too random, let's give some
			   smaller random range - C. Blue
			j = randint(p_ptr->hitdie);*/
			j = 2 + randint(p_ptr->hitdie - 4);
			p_ptr->player_hp[i] = p_ptr->player_hp[i-1] + j;
		}

		/* Require "valid" hitpoints at kinging level */
		if (p_ptr->player_hp[50 - 1] < min_value_king) continue;
		if (p_ptr->player_hp[50 - 1] > max_value_king) continue;

		/* Require "valid" hitpoints at highest level */
		if (p_ptr->player_hp[PY_MAX_LEVEL - 1] < min_value) continue;
		if (p_ptr->player_hp[PY_MAX_LEVEL - 1] > max_value) continue;

		/* Acceptable */
		break;
	}

	/* warn in case char should be invalid.. */
	if (!tries) s_printf("CHAR_CREATION: %s exceeded 500 tries for HP rolling.\n", p_ptr->name);
}


/*
 * Get the racial history, and social class, using the "history charts".
 */
void get_history(int Ind) {
	player_type *p_ptr = Players[Ind];
	int             i, n, chart, roll, social_class;
	int tries = 500;
	char    *s, *t;
	char    buf[240];


	/* Clear the previous history strings */
	for (i = 0; i < 4; i++) p_ptr->history[i][0] = '\0';


	/* Clear the history text */
	buf[0] = '\0';

	/* Initial social class */
	social_class = randint(4);

	/* Starting place */
	switch (p_ptr->prace) {
	case RACE_HUMAN:
	case RACE_DUNADAN:
	case RACE_YEEK:
		chart = 1;
		break;
	case RACE_HALF_ELF:
		chart = 4;
		break;
	case RACE_ELF:
	case RACE_HIGH_ELF:
		chart = 7;
		break;
	case RACE_HOBBIT:
		chart = 10;
		break;
	case RACE_GNOME:
		chart = 13;
		break;
	case RACE_DWARF:
		chart = 16;
		break;
	case RACE_HALF_ORC:
	case RACE_GOBLIN:
		chart = 19;
		break;
	case RACE_HALF_TROLL:
		chart = 22;
		break;
	case RACE_ENT:
		chart = 95;
		break;
	case RACE_DRACONIAN:
		chart = 89;
		break;
	case RACE_DARK_ELF:
		chart = 120;
		break;
	case RACE_VAMPIRE:
		chart = 130;
		break;
#ifdef ENABLE_MAIA
	case RACE_MAIA:
		chart = 100;
		break;
#endif
#ifdef ENABLE_KOBOLD
	case RACE_KOBOLD:
		chart = 110;
		break;
#endif
	default:
		chart = 0;
		break;
	}


	/* Process the history */
	while (chart) {
		/* Start over */
		i = 0;

#ifdef ENABLE_DRACONIAN_TRAITS
		if (p_ptr->prace == RACE_DRACONIAN && chart == 93)
			/* Hack; Fixed nobility, based on trait choice */
			switch (p_ptr->ptrait) {
			case TRAIT_BLUE: roll = 6; break;
			case TRAIT_WHITE: roll = 8; break;
			case TRAIT_RED: roll = 21; break;
			case TRAIT_BLACK: roll = 22 + rand_int(21); break;
			case TRAIT_GREEN: roll = 43 + rand_int(27); break;
			case TRAIT_MULTI: roll = 70; break;
			case TRAIT_BRONZE: roll = 75; break;
			case TRAIT_SILVER: roll = 81; break;
			case TRAIT_GOLD: roll = 89; break;
			case TRAIT_LAW: roll = 93; break;
			case TRAIT_CHAOS: roll = 70; break;
			case TRAIT_BALANCE: roll = 100; break;
			default: roll = randint(100); break;
			}
		else
#endif

		/* Roll for nobility */
		roll = randint(100);

		/* Access the proper entry in the table */
		while ((chart != bg[i].chart) || (roll > bg[i].roll)) i++;

		/* Acquire the textual history */
		(void)strcat(buf, bg[i].info);

		/* Add in the social class */
		social_class += (int)(bg[i].bonus) - 50;

		/* Enter the next chart */
		chart = bg[i].next;
	}



	/* Verify social class */
	if (social_class > 100) social_class = 100;
	else if (social_class < 1) social_class = 1;

	/* Save the social class */
	p_ptr->sc = social_class;


	/* Skip leading spaces */
	for (s = buf; *s == ' '; s++) /* loop */;

	/* Get apparent length */
	n = strlen(s);

	/* Kill trailing spaces */
	while ((n > 0) && (s[n-1] == ' ')) s[--n] = '\0';


	/* Start at first line */
	i = 0;

	/* Collect the history */
	while (--tries) {
		/* Extract remaining length */
		n = strlen(s);

		/* All done */
		if (n < 60) {
			/* Save one line of history */
			strcpy(p_ptr->history[i++], s);

			/* All done */
			break;
		}

		/* Find a reasonable break-point */
		for (n = 60; ((n > 0) && (s[n-1] != ' ')); n--) /* loop */;

		/* Save next location */
		t = s + n;

		/* Wipe trailing spaces */
		while ((n > 0) && (s[n-1] == ' ')) s[--n] = '\0';

		/* Save one line of history */
		strcpy(p_ptr->history[i++], s);

		/* Start next line */
		for (s = t; *s == ' '; s++) /* loop */;
	}
}


/*
 * Computes character's age, height, and weight
 */
static void get_ahw(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Calculate the age */
	p_ptr->age = p_ptr->rp_ptr->b_age + randint(p_ptr->rp_ptr->m_age);

	/* Calculate the height/weight for males */
	if (p_ptr->male) {
		p_ptr->ht = randnor(p_ptr->rp_ptr->m_b_ht, p_ptr->rp_ptr->m_m_ht);
		p_ptr->wt = randnor(p_ptr->rp_ptr->m_b_wt, p_ptr->rp_ptr->m_m_wt);
	}

	/* Calculate the height/weight for females */
	else {
		p_ptr->ht = randnor(p_ptr->rp_ptr->f_b_ht, p_ptr->rp_ptr->f_m_ht);
		p_ptr->wt = randnor(p_ptr->rp_ptr->f_b_wt, p_ptr->rp_ptr->f_m_wt);
	}
}




/*
 * Get the player's starting money
 */
static void get_money(int Ind) {
	player_type *p_ptr = Players[Ind];
	int        i, gold;

	/* Social Class determines starting gold */
/*	gold = (p_ptr->sc * 6) + randint(100) + 300; - the suicide spam to get more gold was annoying.. */
	gold = randint(20) + 350;

	/* Process the stats */
	for (i = 0; i < 6; i++) {
		/* Mega-Hack -- reduce gold for high stats */
		if (stat_use[i] >= 18+50) gold -= 150;
		else if (stat_use[i] >= 18+20) gold -= 100;
		else if (stat_use[i] > 18) gold -= 50;
		else gold -= (stat_use[i] - 8) * 5;
	}

	/* Minimum 100 gold */
	if (gold < 100) gold = 100;

	/* Save the gold */
	p_ptr->au = gold;
	p_ptr->balance = 0;
	p_ptr->tim_blacklist = 0;
	p_ptr->tim_watchlist = 0;
	p_ptr->pstealing = 0;

#ifdef RPG_SERVER /* casters need to buy a decent book, warriors don't */
 #if 0
	p_ptr->au = 1000;
 #else
  #if STARTEQ_TREATMENT < 3
	switch(p_ptr->pclass){
	case CLASS_MAGE:        p_ptr->au += 850; break;
 #ifdef ENABLE_CPRIEST
	case CLASS_CPRIEST:
 #endif
	case CLASS_PRIEST:      p_ptr->au += 600; break;
	case CLASS_SHAMAN:	p_ptr->au += 550; break;
	case CLASS_RUNEMASTER:  p_ptr->au += 500; break;
	case CLASS_MINDCRAFTER:	p_ptr->au += 500; break;
	case CLASS_ROGUE:	p_ptr->au += 400; break;
	case CLASS_ARCHER:	p_ptr->au += 400; break;
	case CLASS_ADVENTURER:	p_ptr->au += 300; break;
	case CLASS_RANGER:      p_ptr->au += 200; break;
   #ifdef ENABLE_DEATHKNIGHT
	case CLASS_DEATHKNIGHT:
   #endif
   #ifdef ENABLE_HELLKNIGHT
	case CLASS_HELLKNIGHT:	//cannot happen here
   #endif
	case CLASS_PALADIN:     p_ptr->au += 200; break;
	case CLASS_DRUID:	p_ptr->au += 200; break;
	case CLASS_MIMIC:	p_ptr->au += 100; break;
	case CLASS_WARRIOR:	p_ptr->au += 0; break;// they can sell their eq.
	default:                ;
	}
  #else
	switch(p_ptr->pclass){
	case CLASS_MAGE:        p_ptr->au += 1000; break;
	case CLASS_SHAMAN:	p_ptr->au += 1000; break;
	case CLASS_MINDCRAFTER:	p_ptr->au += 900; break;
 #ifdef ENABLE_CPRIEST
	case CLASS_CPRIEST:
 #endif
	case CLASS_PRIEST:      p_ptr->au += 800; break;
	case CLASS_RUNEMASTER:  p_ptr->au += 800; break;
	case CLASS_ADVENTURER:	p_ptr->au += 700; break;
	case CLASS_DRUID:	p_ptr->au += 600; break;
	case CLASS_RANGER:      p_ptr->au += 600; break;
   #ifdef ENABLE_DEATHKNIGHT
	case CLASS_DEATHKNIGHT:
   #endif
   #ifdef ENABLE_HELLKNIGHT
	case CLASS_HELLKNIGHT:	//cannot happen here
   #endif
	case CLASS_PALADIN:     p_ptr->au += 600; break;
	case CLASS_ROGUE:	p_ptr->au += 600; break;
	case CLASS_MIMIC:	p_ptr->au += 600; break;
	case CLASS_WARRIOR:	p_ptr->au += 600; break;
	case CLASS_ARCHER:	p_ptr->au += 400; break; /* gets ammo, bow, lantern, extra phases! */
	default:                ;
	}
  #endif
 #endif
#else
 #if STARTEQ_TREATMENT == 3
	p_ptr->au += 300;
 #else
	if ((p_ptr->pclass != CLASS_WARRIOR) && (p_ptr->pclass != CLASS_MIMIC))
		p_ptr->au += 200; /* their startup eq sells for most */
 #endif
#endif

	/* Since it's not a king/queen */
	p_ptr->own1.wx = p_ptr->own1.wy = p_ptr->own1.wz = 0;
	wpcopy(&p_ptr->own2, &p_ptr->own1);
}



/*
 * Clear all the global "character" data
 */
static void player_wipe(int Ind)
{
	player_type *p_ptr = Players[Ind];
	object_type *old_inven;
	object_type *old_inven_copy;
	int i;


	/* Hack -- save the inventory pointer */
	old_inven = p_ptr->inventory;
	old_inven_copy = p_ptr->inventory_copy;

	/* Hack -- zero the struct */
	WIPE(p_ptr, player_type);

	/* Hack -- reset the inventory pointer */
	p_ptr->inventory = old_inven;
	p_ptr->inventory_copy = old_inven_copy;

	/* Wipe the history */
	for (i = 0; i < 4; i++)
		strcpy(p_ptr->history[i], "");

	/* No weight */
	p_ptr->total_weight = 0;

	/* No items */
	p_ptr->inven_cnt = 0;
	p_ptr->equip_cnt = 0;

	/* Clear the inventory */
#if 0
	for (i = 0; i < INVEN_TOTAL; i++)
		invwipe(&p_ptr->inventory[i]);
#else
	C_WIPE(p_ptr->inventory, INVEN_TOTAL, object_type);
#endif

	/* Hack -- Fill the copy with 0xFF - mikaelh */
	memset(p_ptr->inventory_copy, 0xFF, INVEN_TOTAL * sizeof(object_type));

	/* Hack -- Well fed player */
	p_ptr->food = PY_FOOD_FULL - 1;

	/* Assume no winning game */
	p_ptr->total_winner = FALSE;
	p_ptr->once_winner = FALSE;

	/* Assume no panic save */
	panic_save = 0;

	/* Assume no cheating */
	p_ptr->noscore = 0;

	/* clear the wilderness map */
	for (i = 0; i < MAX_WILD_8; i++)
		p_ptr->wild_map[i] = 0;

	/* Hack -- assume the player has an initial knowledge of the area close to town */
	p_ptr->wild_map[(cfg.town_x + cfg.town_y * MAX_WILD_X) / 8] |= 1U << ((cfg.town_x + cfg.town_y * MAX_WILD_X) % 8);

	/* Esp link */
	p_ptr->esp_link_end = 0;

	/* Player don't have the black breath from the beginning !*/
	p_ptr->black_breath = FALSE;

	/* Assume not admin */
	p_ptr->admin_wiz = p_ptr->admin_dm = FALSE;
}




/*
 * Each player starts out with a few items, given as tval/sval pairs.
 * In addition, he always has some food and a few torches.
 */

/*
 * If { 0, 0} or other 'illigal' item, one random item from bard_init
 * will be given instead.	- Jir -
 */
static byte player_init[2][MAX_CLASS][5][3] = {
    { /* Normal body */
	{
		/* Warrior */
		{ TV_SWORD, SV_BROAD_SWORD, 0 },
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL, 0 },
		{ TV_POTION, SV_POTION_BERSERK_STRENGTH, 0 },
		{ 255, 255, 0 },
		{ 255, 255, 0 },
	},

	{
		/* Mage */
		{ TV_MSTAFF, SV_MSTAFF, 0 },
		{ TV_SOFT_ARMOR, SV_ROBE, 0 },
		{ TV_BOOK, 50, 0 },
		{ TV_WAND, SV_WAND_MAGIC_MISSILE , 10 },
		{ 255, 255, 0 },
	},

	{
		/* Priest */
		{ TV_BLUNT, SV_MACE, 0 },
		{ TV_POTION, SV_POTION_HEALING, 0 },
//		{ TV_BOOK, SV_SPELLBOOK, -1 }, /* __lua_HHEALING */
		{ TV_BOOK, 56, 0 },
//		{ TV_SOFT_ARMOR, SV_ROBE, 0},
		{ TV_SOFT_ARMOR, SV_FROCK, 0},
		{ 255, 255, 0 },
	},

	{
		/* Rogue */
		{ TV_SWORD, SV_MAIN_GAUCHE, 0 },
		{ TV_SWORD, SV_DAGGER, 0 },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR, 0 },
		{ TV_TRAPKIT, SV_TRAPKIT_SLING, 0 },
		{ TV_TRAPKIT, SV_TRAPKIT_POTION, 0 },
//		{ TV_BOOK, SV_SPELLBOOK, 21 }, /* Spellbook of Phase Door */
	},

	{
		/* Mimic */
		{ TV_SWORD, SV_TULWAR, 0 },
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL, 0 },
		{ TV_POTION, SV_POTION_CURE_SERIOUS, 0 },
		{ TV_POTION, SV_POTION_SELF_KNOWLEDGE, 0},
		{ 255, 255, 0 },
//		{ TV_RING, SV_RING_POLYMORPH, 0 },
	},

	{
		/* Archer */
		{ TV_ARROW, SV_AMMO_MAGIC, 0 },
		{ TV_SHOT, SV_AMMO_MAGIC, 0 },
		{ TV_BOLT, SV_AMMO_MAGIC, 0 },
		{ TV_BOW, SV_LONG_BOW, 0 },
		{ TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL, 0 },
	},

	{
		/* Paladin */
		{ TV_BLUNT, SV_WAR_HAMMER, 0 },
		{ TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL, 0 },
		{ TV_SCROLL, SV_SCROLL_PROTECTION_FROM_EVIL, 0 },
		{ TV_BOOK, SV_SPELLBOOK, -1 }, /* __lua_HBLESSING */
		{ 255, 255, 0 },
	},

	{
		/* Ranger */
		{ TV_SWORD, SV_LONG_SWORD, 0 },
		{ TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL, 0 },
		{ TV_BOOK, 50, 0 },
		{ TV_BOW, SV_LONG_BOW, 0 },
		{ TV_TRAPKIT, SV_TRAPKIT_SLING, 0 },
	},

	{
		/* Adventurer */
		{ TV_SWORD, SV_SHORT_SWORD, 0 },
		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR, 0 },
		{ TV_SCROLL, SV_SCROLL_MAPPING, 0 },
		{ TV_BOW, SV_SLING, 0 },
		{ 255, 255, 0 },
	},

	{
		/* Druid */
		{ TV_POTION, SV_POTION_HEROISM, 0 },
		{ TV_POTION, SV_POTION_CURE_CRITICAL, 0 },
		{ TV_POTION, SV_POTION_INVIS, 0 },
		{ TV_AMULET, SV_AMULET_SLOW_DIGEST, 0 },
		{ TV_SOFT_ARMOR, SV_GOWN, 0 },
	},

	{
		/* Shaman */
		{ TV_BOOK, 61, 0 },
		{ TV_SOFT_ARMOR, SV_ROBE, 0 },
		{ TV_AMULET, SV_AMULET_INFRA, 3 },
		{ TV_POTION, SV_POTION_CURE_POISON, 0 },
		{ TV_SWORD, SV_SHADOW_BLADE, 0 }, /* just a placeholder! */
	},
	{
		/* Runemaster */
		{ TV_SWORD, SV_DAGGER, 0 },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR, 0 },
		{ TV_STAFF, SV_STAFF_DETECT_ITEM, 20 },
		{ TV_DIGGING, SV_PICK, 0 },
		{ TV_BOOMERANG, SV_BOOM_S_METAL, 0 },
	},
	{
		/* Mindcrafter */
//		{ TV_BOOK, 50, 0 },
		{ TV_BOOK, SV_SPELLBOOK, -1 },/* __lua_MSCARE */
//		{ TV_SWORD, SV_SHORT_SWORD, 0 },
		{ TV_SWORD, SV_RAPIER, 0 },//TV_SABRE didn't give 2 bpr w/ min recomm stats, and they are so similar, so what gives
		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR, 0 },
		{ TV_SCROLL, SV_SCROLL_TELEPORT, 0 },
		{ 255, 255, 0 },
	},

#ifdef ENABLE_DEATHKNIGHT
	{
		/* Death Knight (Vampire Paladin) */
		{ TV_SWORD, SV_LONG_SWORD, 0 },
		{ TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL, 0 },
		//{ TV_SCROLL, SV_SCROLL_ICE, 0 },
		{ TV_SCROLL, SV_SCROLL_DARKNESS, 0 },
		{ TV_BOOK, SV_SPELLBOOK, -1 }, /* __lua_OFEAR */
		{ 255, 255, 0 },
	},
#endif
#ifdef ENABLE_HELLKNIGHT
	{
		/* Hell Knight (Corrupted Paladin) */
		{ TV_SWORD, SV_LONG_SWORD, 0 },
		{ TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL, 0 },
		//{ TV_SCROLL, SV_SCROLL_ICE, 0 },
		{ TV_AMULET, SV_AMULET_DOOM, -1 },
		{ TV_BOOK, SV_SPELLBOOK, -1 }, /* __lua_TERROR */
		{ 255, 255, 0 },
	},
#endif
#ifdef ENABLE_CPRIEST
	{
		/* Corrupted Priest */
		{ TV_BLUNT, SV_MACE, 0 },
		{ TV_POTION, SV_POTION_HEALING, 0 },
		{ TV_BOOK, SV_SPELLBOOK, -1 }, /* __lua_TERROR?.. */
		{ TV_SOFT_ARMOR, SV_FROCK, 0},
		{ 255, 255, 0 },
	},
#endif
    },
    { /* Fruit bat body */
	{
		/* Warrior */
		{ TV_HELM, SV_METAL_CAP, 0 },
		{ TV_CLOAK, SV_CLOAK, 0 },
		{ TV_POTION, SV_POTION_BERSERK_STRENGTH, 0 },
		{ 255, 255, 0 },
		{ 255, 255, 0 },
	},

	{
		/* Mage */
		{ TV_HELM, SV_CLOTH_CAP, 0 },
		{ TV_CLOAK, SV_CLOAK, 0 },
		{ TV_BOOK, 50, 0 },
		{ TV_WAND, SV_WAND_MAGIC_MISSILE , 10 },
		{ 255, 255, 0 },
	},

	{
		/* Priest */
		{ TV_HELM, SV_CLOTH_CAP, 0 },
		{ TV_POTION, SV_POTION_HEALING, 0 },
//		{ TV_BOOK, SV_SPELLBOOK, -1 }, /* __lua_HHEALING */
		{ TV_BOOK, 56, 0 },
		{ TV_CLOAK, SV_CLOAK, 0 },
		{ 255, 255, 0 },
	},

	{
		/* Rogue */
		{ TV_HELM, SV_HARD_LEATHER_CAP, 0 },
		{ 255, 255, 0 },
		{ TV_CLOAK, SV_CLOAK, 0 },
		{ TV_TRAPKIT, SV_TRAPKIT_SLING, 0 },
		{ TV_TRAPKIT, SV_TRAPKIT_POTION, 0 },
//		{ TV_BOOK, SV_SPELLBOOK, 21 }, /* Spellbook of Phase Door */
	},

	{
		/* Mimic */
		{ TV_HELM, SV_METAL_CAP, 0 },
		{ TV_CLOAK, SV_CLOAK, 0 },
		{ TV_POTION, SV_POTION_CURE_SERIOUS, 0 },
		{ TV_POTION, SV_POTION_SELF_KNOWLEDGE, 0},
		{ 255, 255, 0 },
//		{ TV_RING, SV_RING_POLYMORPH, 0 },
	},

	{
		/* Archer */
		{ TV_ARROW, SV_AMMO_MAGIC, 0 },
		{ TV_SHOT, SV_AMMO_MAGIC, 0 },
		{ TV_BOLT, SV_AMMO_MAGIC, 0 },
		{ TV_BOW, SV_LONG_BOW, 0 },//just doesn't work as fruit bat
		{ TV_HELM, SV_METAL_CAP, 0 },
	},

	{
		/* Paladin */
		{ TV_HELM, SV_METAL_CAP, 0 },
		{ TV_CLOAK, SV_CLOAK, 0 },
		{ TV_SCROLL, SV_SCROLL_PROTECTION_FROM_EVIL, 0 },
		{ TV_BOOK, SV_SPELLBOOK, -1 }, /* __lua_HBLESSING */
		{ 255, 255, 0 },
	},

	{
		/* Ranger */
		{ TV_HELM, SV_HARD_LEATHER_CAP, 0 },
		{ TV_CLOAK, SV_CLOAK, 0 },
		{ TV_BOOK, 50, 0 },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL, 0 },//instead of unusable bow. alternatives: invis-pot, id-all-scroll?, mapping, rll, csw/ccw?
		{ TV_TRAPKIT, SV_TRAPKIT_SLING, 0 },
	},

	{
		/* Adventurer */
		{ TV_HELM, SV_HARD_LEATHER_CAP, 0 },
		{ TV_CLOAK, SV_CLOAK, 0 },
		{ TV_SCROLL, SV_SCROLL_MAPPING, 0 },
		{ 255, 255, 0 },
		{ 255, 255, 0 },
	},

	{
		/* Druid */
		{ TV_POTION, SV_POTION_HEROISM, 0 },
		{ TV_POTION, SV_POTION_CURE_CRITICAL, 0 },
		{ TV_POTION, SV_POTION_INVIS, 0 },
		{ TV_AMULET, SV_AMULET_SLOW_DIGEST, 0 },
		{ 255, 255, 0 },
	},

	{
		/* Shaman */
		{ TV_BOOK, 61, 0 },
		{ TV_CLOAK, SV_CLOAK, 0 },
		{ TV_AMULET, SV_AMULET_INFRA, 3 },
		{ TV_POTION, SV_POTION_CURE_POISON, 0 },
		{ TV_HELM, SV_CLOTH_CAP, 0 }, /* just a placeholder! */
	},
	{
		/* Runemaster */
		{ TV_HELM, SV_HARD_LEATHER_CAP, 0 },
		{ TV_CLOAK, SV_CLOAK, 0 },
		{ TV_STAFF, SV_STAFF_DETECT_ITEM, 20 },
		{ TV_DIGGING, SV_PICK, 0 },
		{ 255, 255, 0 },
	},
	{
		/* Mindcrafter */
//		{ TV_BOOK, 50, 0 },
		{ TV_BOOK, SV_SPELLBOOK, -1 },/* __lua_MSCARE */
		{ TV_HELM, SV_METAL_CAP, 0 },
		{ TV_CLOAK, SV_CLOAK, 0 },
		{ TV_SCROLL, SV_SCROLL_TELEPORT, 0 },
		{ 255, 255, 0 },
	},
#ifdef ENABLE_DEATHKNIGHT
	{
		/* Death Knight (Vampire Paladin) */
		{ TV_HELM, SV_METAL_CAP, 0 },
		{ TV_CLOAK, SV_CLOAK, 0 },
		//{ TV_SCROLL, SV_SCROLL_ICE, 0 },
		{ TV_SCROLL, SV_SCROLL_DARKNESS, 0 },
		{ TV_BOOK, SV_SPELLBOOK, -1 }, /* __lua_OFEAR */
		{ 255, 255, 0 },
	},
#endif
#ifdef ENABLE_HELLKNIGHT
	{
		/* Hell Knight (Corrupted Paladin) */
		{ TV_HELM, SV_METAL_CAP, 0 },
		{ TV_CLOAK, SV_CLOAK, 0 },
		//{ TV_SCROLL, SV_SCROLL_ICE, 0 },
		{ TV_AMULET, SV_AMULET_DOOM, -1 },
		{ TV_BOOK, SV_SPELLBOOK, -1 }, /* __lua_TERROR */
		{ 255, 255, 0 },
	},
#endif
#ifdef ENABLE_CPRIEST
	{
		/* Corrupted Priest */
		{ TV_HELM, SV_CLOTH_CAP, 0 },
		{ TV_POTION, SV_POTION_HEALING, 0 },
		{ TV_BOOK, SV_SPELLBOOK, -1 }, /* __lua_TERROR?.. */
		{ TV_CLOAK, SV_CLOAK, 0 },
		{ 255, 255, 0 },
	},
#endif
    }
};
void init_player_outfits(void) {
	/* hack: make sure spellbook constants are correct.
	   actually this could be done once in init_lua,
	   but since the array player_init is static, we
	   just do it here. - C. Blue */

	   //player_init[0][CLASS_PRIEST][2][2] = __lua_HHEALING;
	   player_init[0][CLASS_PALADIN][3][2] = __lua_HBLESSING;
#ifdef ENABLE_DEATHKNIGHT
	   player_init[0][CLASS_DEATHKNIGHT][3][2] = __lua_OFEAR;
#endif
#ifdef ENABLE_HELLKNIGHT
	   player_init[0][CLASS_HELLKNIGHT][3][2] = __lua_OFEAR; //just placeholder, since class can't be "created"
#endif
#ifdef ENABLE_CPRIEST
	   player_init[0][CLASS_CPRIEST][2][2] = __lua_OFEAR; //just placeholder, since class can't be "created"
#endif
	   player_init[0][CLASS_MINDCRAFTER][0][2] = __lua_MSCARE;

	   //player_init[1][CLASS_PRIEST][2][2] = __lua_HHEALING;
	   player_init[1][CLASS_PALADIN][3][2] = __lua_HBLESSING;
#ifdef ENABLE_DEATHKNIGHT
	   player_init[1][CLASS_DEATHKNIGHT][3][2] = __lua_OFEAR;
#endif
#ifdef ENABLE_HELLKNIGHT
	   player_init[1][CLASS_HELLKNIGHT][3][2] = __lua_OFEAR; //just placeholder, since class can't be "created"
#endif
#ifdef ENABLE_CPRIEST
	   player_init[1][CLASS_CPRIEST][2][2] = __lua_OFEAR; //just placeholder, since class can't be "created"
#endif
	   player_init[1][CLASS_MINDCRAFTER][0][2] = __lua_MSCARE;
}

#define BARD_INIT_NUM	15
static byte bard_init[BARD_INIT_NUM][2] = {
	{ TV_RING, SV_RING_SEE_INVIS },
	{ TV_BOOK, 50 },
	{ TV_BOW, SV_LONG_BOW },
	{ TV_TRAPKIT, SV_TRAPKIT_SLING },
	{ TV_POTION, SV_POTION_BERSERK_STRENGTH },

	{ TV_HARD_ARMOR, SV_CHAIN_MAIL },
	{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
	{ TV_BLUNT, SV_MACE },
	{ TV_POTION, SV_POTION_HEALING },
	{ TV_SCROLL, SV_SCROLL_PROTECTION_FROM_EVIL },

	{ TV_SWORD, SV_LONG_SWORD },
	{ TV_ARROW, SV_AMMO_MAGIC },
	{ TV_BLUNT, SV_WHIP },
	{ TV_LITE, SV_LITE_LANTERN },
	{ TV_TOOL, SV_TOOL_FLINT },
};

#define do_admin_outfit()	\
	object_aware(Ind, o_ptr); \
	object_known(o_ptr); \
/*	o_ptr->discount = 100; */ /* was annoying text-wise */ \
	o_ptr->ident |= ID_MENTAL; \
/*	o_ptr->owner = p_ptr->id; */ \
/*	o_ptr->mode = p_ptr->mode; */ \
	o_ptr->level = 1; \
	(void)inven_carry(Ind, o_ptr);

/*
 * Give the cfg_admin_wizard some interesting stuff.
 */
/* XXX 'realm' is not used */
void admin_outfit(int Ind, int realm) {
	player_type *p_ptr = Players[Ind];
	int             i;

	object_type     forge;
	object_type     *o_ptr = &forge;

	//int note = quark_add("!k");

	if (!is_admin(p_ptr)) return;


	/* Hack -- assume the player has an initial knowledge of the area close to town */
	for (i = 0; i < MAX_WILD_X*MAX_WILD_Y; i++)  p_ptr->wild_map[i/8] |= 1<<(i%8);

#if 0 /* book spam stopped for now -_-  -C. Blue */
	for (i = 0; i < 255; i++) {
		int k_idx = lookup_kind(TV_BOOK, i);

		if (!k_idx) continue;
		invcopy(o_ptr, k_idx);
		o_ptr->number = 1;
		apply_magic(&p_ptr->wpos, o_ptr, 1, TRUE, FALSE, FALSE, FALSE, RESF_NONE);
		do_admin_outfit();
	}
#endif

	invcopy(o_ptr, lookup_kind(TV_BOOK, 58)); /* Handbook of Dungeon Keeping */
	o_ptr->number = 1;
	do_admin_outfit();

	//invcopy(o_ptr, lookup_kind(TV_LITE, SV_LITE_FEANORIAN));
	invcopy(o_ptr, lookup_kind(TV_LITE, SV_LITE_DWARVEN)); /* more subtile ;) */
	apply_magic_depth(0, o_ptr, -1, TRUE, TRUE, TRUE, FALSE, RESF_NONE);
	o_ptr->number = 1;
	do_admin_outfit();

	invcopy(o_ptr, lookup_kind(TV_AMULET, SV_AMULET_INVINCIBILITY));
	apply_magic_depth(0, o_ptr, -1, TRUE, TRUE, TRUE, FALSE, RESF_NONE);
	o_ptr->number = 1;
	do_admin_outfit();

	invcopy(o_ptr, lookup_kind(TV_AMULET, SV_AMULET_INVULNERABILITY));
	apply_magic_depth(0, o_ptr, -1, TRUE, TRUE, TRUE, FALSE, RESF_NONE);
	o_ptr->number = 1;
	do_admin_outfit();

	invcopy(o_ptr, lookup_kind(TV_CLOAK, SV_SHADOW_CLOAK));
	o_ptr->name1 = ART_CLOAK_DM;
	apply_magic_depth(0, o_ptr, -1, TRUE, TRUE, TRUE, FALSE, RESF_NONE);
	o_ptr->number = 1;
	do_admin_outfit();

	invcopy(o_ptr, lookup_kind(TV_ROD, SV_ROD_PROBING));
	o_ptr->number = 3;
	o_ptr->name2 = EGO_RISTARI;
	o_ptr->pval = 0;
	do_admin_outfit();

	invcopy(o_ptr, lookup_kind(TV_HELM, SV_GOGGLES_DM));
	o_ptr->name1 = ART_GOGGLES_DM;
	apply_magic_depth(0, o_ptr, -1, TRUE, TRUE, TRUE, FALSE, RESF_NONE);
	o_ptr->number = 1;
	do_admin_outfit();

	if (!p_ptr->fruit_bat) {
		invcopy(o_ptr, lookup_kind(TV_BOW, SV_LONG_BOW));
		apply_magic_depth(0, o_ptr, -1, TRUE, TRUE, TRUE, FALSE, RESF_NONE);
		o_ptr->to_h = 200;
		o_ptr->to_d = 200;
		o_ptr->number = 1;
		do_admin_outfit();

		invcopy(o_ptr, lookup_kind(TV_ARROW, SV_AMMO_MAGIC));
		apply_magic_depth(0, o_ptr, -1, TRUE, TRUE, TRUE, FALSE, RESF_NONE);
		o_ptr->number = 1;
		do_admin_outfit();

		/* either to inscribe @Ox or to one-hit-kill */
		invcopy(o_ptr, lookup_kind(TV_POLEARM, SV_SCYTHE));
		o_ptr->name1 = ART_SCYTHE_DM;
		apply_magic_depth(0, o_ptr, -1, TRUE, TRUE, TRUE, FALSE, RESF_NONE);
		o_ptr->number = 1;
		do_admin_outfit();
	}

#if 0
#ifdef TEST_SERVER
	invcopy(o_ptr, lookup_kind(TV_POTION, SV_POTION_DEATH));
	o_ptr->number = 1;
	do_admin_outfit();
#endif
#endif
}

#if STARTEQ_TREATMENT == 0
    #define do_player_outfit()	\
	object_aware(Ind, o_ptr); \
	object_known(o_ptr); \
	o_ptr->ident |= ID_MENTAL; \
	o_ptr->owner = p_ptr->id; \
	o_ptr->mode = p_ptr->mode; \
	o_ptr->level = 1; \
	(void)inven_carry_equip(Ind, o_ptr);
#else
 #if STARTEQ_TREATMENT == 3
    #define do_player_outfit()	\
	object_aware(Ind, o_ptr); \
	object_known(o_ptr); \
	o_ptr->ident |= ID_MENTAL; \
	o_ptr->owner = p_ptr->id; \
	o_ptr->mode = p_ptr->mode; \
	o_ptr->level = 0; \
	o_ptr->discount = 100; /* <- replaced this by making level-0-items unsellable in general */ \
	if (!o_ptr->note) o_ptr->note = quark_add(""); /* hack to hide '100% off' tag */ \
	(void)inven_carry_equip(Ind, o_ptr);
 #else
    #define do_player_outfit()	\
	object_aware(Ind, o_ptr); \
	object_known(o_ptr); \
	o_ptr->ident |= ID_MENTAL; \
	o_ptr->owner = p_ptr->id; \
	o_ptr->mode = p_ptr->mode; \
	o_ptr->level = 0; \
	(void)inven_carry_equip(Ind, o_ptr);
 #endif
#endif

/*
 * Init players with some belongings
 *
 * Having an item makes the player "aware" of its purpose.
 */
static void player_outfit(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i, j, tv, sv, pv, k_idx, body;

	object_type     forge;
	object_type     *o_ptr = &forge;

	body = (p_ptr->mode & MODE_FRUIT_BAT) ? 1 : 0;

	/* Hack -- Give the player some water */
	if (p_ptr->prace == RACE_ENT) {
		invcopy(o_ptr, lookup_kind(TV_POTION, SV_POTION_WATER));
		o_ptr->number = rand_range(3, 7);
		do_player_outfit();
	}
	/* XXX problem is that Lembas sell dear.. */
	else if (p_ptr->prace == RACE_HALF_ELF ||
	    p_ptr->prace == RACE_ELF || p_ptr->prace == RACE_HIGH_ELF) {
		invcopy(o_ptr, lookup_kind(TV_FOOD, SV_FOOD_WAYBREAD));
		o_ptr->number = rand_range(3, 4);
		do_player_outfit();
	}
	/* Firestones for Dragonriders */
	else if (p_ptr->prace == RACE_DRACONIAN) {
		invcopy(o_ptr, lookup_kind(TV_FIRESTONE, SV_FIRE_SMALL));
		o_ptr->number = rand_range(3, 5);
		do_player_outfit();
	}
	/* Dwarves like to collect treasures */
	else if (p_ptr->prace == RACE_DWARF) {
		invcopy(o_ptr, lookup_kind(TV_STAFF, SV_STAFF_DETECT_GOLD));
		o_ptr->number = 1;
		o_ptr->pval = 30;
		do_player_outfit();
	}

	/* vampires get mummy wrapping against the burning sunlight */
	if (p_ptr->prace == RACE_VAMPIRE) {
		invcopy(o_ptr, lookup_kind(TV_TOOL, SV_TOOL_WRAPPING));
		o_ptr->number = 1;
		do_player_outfit();
	}
	/* vampires feed off living prey, using their vampiric life leech *exclusively* */
	else if (p_ptr->prace != RACE_ELF && p_ptr->prace != RACE_HALF_ELF && p_ptr->prace != RACE_HIGH_ELF
	    && p_ptr->prace != RACE_ENT) {
		invcopy(o_ptr, lookup_kind(TV_FOOD, SV_FOOD_RATION));
		o_ptr->number = rand_range(3, 7);
		do_player_outfit();
	}

	/* Hack -- Give the player some torches */
	if (p_ptr->prace != RACE_VAMPIRE && p_ptr->pclass != CLASS_ARCHER && p_ptr->pclass != CLASS_RUNEMASTER) {
		invcopy(o_ptr, lookup_kind(TV_LITE, SV_LITE_TORCH));
		o_ptr->number = rand_range(3, 7);
		o_ptr->timeout = rand_range(3, 7) * 500;
		do_player_outfit();
	}

	if (!strcmp(p_ptr->name, "Moltor")) {
		invcopy(o_ptr, lookup_kind(TV_FOOD, SV_FOOD_PINT_OF_ALE));
		o_ptr->name2 = 188;	// Bud ;)
		o_ptr->number = 9;
		apply_magic_depth(0, o_ptr, -1, TRUE, TRUE, TRUE, FALSE, make_resf(p_ptr));
		o_ptr->discount = 72;
		o_ptr->owner = p_ptr->id;
                o_ptr->mode = p_ptr->mode;
		o_ptr->level = 1;
		object_known(o_ptr);
		object_aware(Ind, o_ptr);
		o_ptr->ident |= ID_MENTAL;
		(void)inven_carry(Ind, o_ptr);
	}

#ifdef TEST_SERVER
	invcopy(o_ptr, lookup_kind(TV_POTION, SV_POTION_DEATH));
	o_ptr->discount = 100;
	o_ptr->owner = p_ptr->id;
        o_ptr->mode = p_ptr->mode;
	o_ptr->level = 0;
	object_known(o_ptr);
	object_aware(Ind, o_ptr);
	o_ptr->ident |= ID_MENTAL;
	(void)inven_carry(Ind, o_ptr);
#endif

	//admin_outfit(Ind);

	/* Hack -- Give the player useful objects */
	for (i = 0; i < 5; i++) {
		tv = player_init[body][p_ptr->pclass][i][0];
		sv = player_init[body][p_ptr->pclass][i][1];
		pv = player_init[body][p_ptr->pclass][i][2];

		/* Give players some racially flavoured starter weapon */
		if (tv == TV_SWORD) {
			switch (sv) {
			case SV_BROAD_SWORD:/* warrior */
				switch (p_ptr->prace) {
				case RACE_HALF_TROLL:
				case RACE_ENT:
					tv = TV_BLUNT; sv = SV_MACE; break;
				case RACE_DWARF:
					tv = TV_AXE; sv = SV_BROAD_AXE; break;
				case RACE_DRACONIAN:
					tv = TV_POLEARM; sv = SV_TRIFURCATE_SPEAR; break;
				} break;
			case SV_TULWAR:/* mimic */
				switch (p_ptr->prace) {
				case RACE_HALF_TROLL:
				case RACE_ENT:
					tv = TV_BLUNT; sv = SV_BALL_AND_CHAIN; break;
				case RACE_DWARF:
					tv = TV_AXE; sv = SV_HATCHET; break;
				case RACE_DRACONIAN:
					tv = TV_POLEARM; sv = SV_RHOMPHAIA; break;
				} break;
			case SV_LONG_SWORD:/* ranger */
				switch (p_ptr->prace) {
				case RACE_HALF_TROLL:
				case RACE_ENT:
					tv = TV_BLUNT; sv = SV_BALL_AND_CHAIN; break;
				case RACE_DWARF:
					tv = TV_AXE; sv = SV_LIGHT_WAR_AXE; break;
				case RACE_DRACONIAN:
					tv = TV_POLEARM; sv = SV_SICKLE; break;
				} break;
			case SV_SHORT_SWORD:/* adventurer */
				switch (p_ptr->prace) {
				case RACE_HALF_TROLL:
				case RACE_ENT:
					tv = TV_BLUNT; sv = SV_CLUB; break;
				case RACE_DWARF:
					tv = TV_AXE; sv = SV_CLEAVER; break;
				case RACE_DRACONIAN:
					tv = TV_POLEARM; sv = SV_SPEAR; break;
				} break;
			case SV_SHADOW_BLADE:/* shaman, shadow blade is just a placeholder */
				tv = TV_AXE; sv = SV_CLEAVER;
				switch (p_ptr->prace) {
				case RACE_HALF_TROLL:
				case RACE_ENT:
					tv = TV_BLUNT; sv = SV_QUARTERSTAFF; break;
				case RACE_DRACONIAN:
					tv = TV_POLEARM; sv = SV_BROAD_SPEAR; break;
				} break;
			case SV_RAPIER:/* mindcrafter */
				switch (p_ptr->prace) {
				case RACE_DRACONIAN:
					tv = TV_POLEARM; sv = SV_TRIDENT; break;
				} break;
			}
		}

		/* If someone uses too low STR/DEX values, "downgrade"
		   his starter weapon to a lighter version to ensure at least 2 bpr. */
		if (is_melee_weapon(tv)) {
			int wgt, sv2 = sv;
			int sv_best = sv, bpr_best = 0, bpr;
			int tv2 = tv, tv_best = tv;

			/* stat_ind has not been set so we have to hack it for calc_blows_obj().
			   Side-effect: We ignore Hobbits' +2 DEX bonus for not wearing shoes */
			//abuse 'wgt'
			for (j = 0; j < 6; j++) {
				/* Values: 3, 4, ..., 17 */
				if (stat_use[j] <= 18) wgt = (stat_use[j] - 3);
				/* Ranges: 18/00-18/09, ..., 18/210-18/219 */
				else if (stat_use[j] <= 18 + 219) wgt = (15 + (stat_use[j] - 18) / 10);
				/* Range: 18/220+ */
				else wgt = 37;

				p_ptr->stat_ind[j] = wgt;
			}

			switch (p_ptr->pclass) {
			case CLASS_WARRIOR:
				/* maybe 3, to aim for 3 bpr at first? -
				   changed it to 2, or all warriors would start with silly 'small' weapons.. */
				j = 2;
				break;
			case CLASS_MIMIC:
			case CLASS_PALADIN:
#ifdef ENABLE_DEATHKNIGHT
			case CLASS_DEATHKNIGHT:
#endif
#ifdef ENABLE_HELLKNIGHT
			case CLASS_HELLKNIGHT:	//cannot happen here
#endif
			case CLASS_MINDCRAFTER:
			//case CLASS_PRIEST:
				j = 2;
				break;
			default:
				j = 0;
			}

			while (TRUE) {
				invcopy(o_ptr, lookup_kind(tv2, sv2));
				bpr = calc_blows_obj(Ind, o_ptr);

				/* megahack: avoid weapons that are too heavy to wield them at all */
				if (adj_str_hold[p_ptr->stat_ind[A_STR]] < o_ptr->weight / 10) bpr = j - 1;

				/* weapon is fine? */
				if (bpr >= j) {
					tv = tv2;
					sv = sv2;
					break;
				}

				/* remember heaviest weapon that reached best bpr */
				if (bpr > bpr_best) {
					bpr_best = bpr;
					tv_best = tv2;
					sv_best = sv2;
				}

				/* find a weapon that is slightly lighter than our current one */
				wgt = 0;
				for (k_idx = 0; k_idx < max_k_idx; k_idx++) {
					/* must be same weapon class - hack: make blunt+axe interchangable! */
					if (k_info[k_idx].tval != tv &&
					    !((k_info[k_idx].tval == TV_AXE && tv == TV_BLUNT) ||
					    (k_info[k_idx].tval == TV_BLUNT && tv == TV_AXE))) continue;
					/* forbid weapons that are > level 15 or have slay/brand specialties or are 'broken',
					   except if explicitely marked as startup weapon (scourge) */
					if (((k_info[k_idx].flags1 & TR1_MULTMASK) || k_info[k_idx].level > 15 || k_info[k_idx].to_d < 0)
					    && !(k_info[k_idx].flags6 & TR6_STARTUP)) continue;
					/* must be lighter than current weapon */
					if (k_info[k_idx].weight >= o_ptr->weight) continue;

					/* must be as heavy as possible, since we're trying to go lower gradually */
					if (wgt > k_info[k_idx].weight) continue;
					if (wgt == k_info[k_idx].weight &&
					    (rand_int(2) || k_info[k_idx].tval != tv)) continue;

					wgt = k_info[k_idx].weight;
					sv2 = k_info[k_idx].sval;
					tv2 = k_info[k_idx].tval;
				}

				/* no lighter weapon found? */
				if (tv2 == o_ptr->tval && sv2 == o_ptr->sval) {
					/* for failing 3 bpr target (warriors):
					   did we at least reach 2 bpr though? -> accept new weapon */
					if (calc_blows_obj(Ind, o_ptr) >= 2) {
						sv = sv_best;
						tv = tv_best;
					}
					/* otherwise keep (heavy) starter weapon */
					break;
				}
			}
		}

#if 0
		if (tv == TV_BOOK && sv == SV_SPELLBOOK) { /* hack - correct book orders */
			if (pv == 60) pv = __lua_HBLESSING;
//			if (pv != 255) pv = 60;
		}
#endif

		if (tv == 255 && sv == 255) {
			/* nothing */
		} else {
			k_idx = lookup_kind(tv, sv);
			if (k_idx < 2) { /* '1' is 'something' */
				j = rand_int(BARD_INIT_NUM);
				k_idx = lookup_kind(bard_init[j][0], bard_init[j][1]);
			} else {
				invcopy(o_ptr, k_idx);
				o_ptr->pval = pv;
				o_ptr->number = 1;

#if 1 /* use check in do_cmd_ranged_technique() instead? */
				/* hack: prevent newbie archers from wasting their
				   only arrow by a flare missile technique */
				if (is_ammo(tv)) o_ptr->note = quark_add("!k");
#endif

#ifdef ENABLE_HELLKNIGHT
				if (tv == TV_AMULET && sv == SV_AMULET_DOOM) {
					o_ptr->pval = 0;
					o_ptr->bpval = pv;
					o_ptr->to_a = pv;
					o_ptr->ident |= (ID_CURSED);
				}
#endif

				do_player_outfit();
			}
		}
	}

	/* Lantern of Brightness for Archers */
	if (p_ptr->pclass == CLASS_ARCHER && p_ptr->prace != RACE_VAMPIRE) {
		u32b f1, f2, f3, f4, f5, f6, esp;
		do {
			invcopy(o_ptr, lookup_kind(TV_LITE, SV_LITE_LANTERN));
			o_ptr->number = 1;
			o_ptr->discount = 100;
			o_ptr->name2 = 139;
			apply_magic(&p_ptr->wpos, o_ptr, -1, FALSE, FALSE, FALSE, FALSE, RESF_NONE);
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &f6, &esp);
		} while (f2 & TR2_RES_DARK);
		do_player_outfit();

		invcopy(o_ptr, lookup_kind(TV_FLASK, SV_FLASK_OIL));
		o_ptr->number = 5;
		do_player_outfit();
	}

	/* Normal Lantern for Runemasters */
	if (p_ptr->pclass == CLASS_RUNEMASTER) {
		invcopy(o_ptr, lookup_kind(TV_LITE, SV_LITE_LANTERN));
		o_ptr->number = 1;
		o_ptr->timeout = rand_range(3, 7) * 500;
		do_player_outfit();

		invcopy(o_ptr, lookup_kind(TV_FLASK, SV_FLASK_OIL));
		o_ptr->number = rand_range(3, 7);
		do_player_outfit();
	}

	/* hack for mimics: pick a type of poly ring - C. Blue */
#if 0 /* disabled for now */
	if (p_ptr->pclass == CLASS_MIMIC) {
		invcopy(o_ptr, lookup_kind(TV_RING, SV_RING_POLYMORPH));
		o_ptr->number = 1;
		o_ptr->discount = 100;
 #if 0 /* random */
		apply_magic(&p_ptr->wpos, o_ptr, -1, FALSE, FALSE, FALSE, FALSE, RESF_NONE);
 #else /* predefined, or else people might reroll like crazy.... */
		switch (randint(7)) {
		case 1: pv = 125; break;//z (rotting corpse)
		case 2: pv = 194; break;//u (tengu)
		case 3: pv = 313; break;//o (uruk)
		case 4: pv = 288; break;//P (fire giant)
		case 5: pv = 343; break;//Y (sasquatch)
		case 6: pv = 178; break;//h (dark elven mage)
		case 7: pv = 370; break;//p (jade monk)
		}
		o_ptr->pval = pv;
                if (r_info[pv].level > 0) {
			o_ptr->level = 10 + (1000 / ((2000 / r_info[pv].level) + 10));
		} else {
                        o_ptr->level = 10;
		}
		/* Make the ring last only a certain period of time >:) - C. Blue */ 
		o_ptr->timeout_magic = 3000 + rand_int(3001);
 #endif
		do_player_outfit();
	}
#endif

#ifdef RPG_SERVER /* give extra startup survival kit - so unaffected by very low CHR! */
	invcopy(o_ptr, lookup_kind(TV_POTION, SV_POTION_CURE_SERIOUS));
	o_ptr->number = 5;
	o_ptr->discount = 100;
	do_player_outfit();
/* replacing phase scrolls with more $. Stacking issues annoy me. -Molt */
#if 0
	invcopy(o_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_PHASE_DOOR));
	o_ptr->number = (p_ptr->pclass == CLASS_ARCHER) ? 10 : 5;
	o_ptr->discount = 100;
	do_player_outfit();
#else
	if (p_ptr->pclass == CLASS_ARCHER)
	p_ptr->au += 100;
	else
	p_ptr->au += 50;
#endif
#endif

	/* Hack -- Give the player newbie guide Parchment */
	invcopy(o_ptr, lookup_kind(TV_PARCHMENT, SV_PARCHMENT_NEWBIE));
	//invcopy(o_ptr, lookup_kind(TV_PARCHMENT, SV_PARCHMENT_NEWS));
	o_ptr->number = 1;
	do_player_outfit();
}

static void player_create_tmpfile(int Ind) {
	player_type *p_ptr = Players[Ind];
	//FILE *fff;
	char file_name[MAX_PATH_LENGTH];

	/* Temporary file */
	if (path_temp(file_name, MAX_PATH_LENGTH)) {
		s_printf("failed to generate tmpfile for %s!\n", p_ptr->name);
		return;
	}

	strcpy(p_ptr->infofile, file_name);
}

static void player_setup(int Ind, bool new) {
	player_type *p_ptr = Players[Ind];
	int y, x, i, d, count = 0;
	dun_level *l_ptr = NULL;

	//bool unstaticed = FALSE;
	bool panic = p_ptr->panic;

	struct worldpos *wpos = &p_ptr->wpos;
	cave_type **zcave;

	/* initialise "not in a store" very early on up here,
	  for some teleport_player() calls that may follow soon */
	p_ptr->store_num = -1;

	/* Catch bad player coordinates,
	   either corrupted ones (insane values)
	   or invalid ones if dungeon locations were changed meanwhile - C. Blue */
        /* Ultra-hack bugfix for recall-crash, thanks to Chris for the idea :) */
	if ((wpos->wx >= MAX_WILD_X) || (wpos->wy >= MAX_WILD_Y) || (wpos->wz > MAX_DEPTH) ||
	    (wpos->wx < 0) || (wpos->wy < 0) || (wpos->wz < -MAX_DEPTH)) {
		s_printf("Ultra-hack executed for %s. wx %d wy %d wz %d\n", p_ptr->name, wpos->wx, wpos->wy, wpos->wz);
		wpos->wx = cfg.town_x;
                wpos->wy = cfg.town_y;
		wpos->wz = 0;
	}
	/* If dungeon existences changed, restore players who saved
	   within a now-invalid dungeon - C. Blue */
	if (((wpos->wz > 0) && (wild_info[wpos->wy][wpos->wx].tower == NULL)) ||
	    ((wpos->wz < 0) && (wild_info[wpos->wy][wpos->wx].dungeon == NULL))) {
		s_printf("Ultra-hack #2 executed for %s. wx %d wy %d wz %d\n", p_ptr->name, wpos->wx, wpos->wy, wpos->wz);
		wpos->wx = cfg.town_x;
                wpos->wy = cfg.town_y;
		wpos->wz = 0;

#ifdef ALLOW_NR_CROSS_ITEMS
		for (i = 1; i < INVEN_TOTAL; i++)
			p_ptr->inventory[i].NR_tradable = FALSE;
#endif
#ifdef ALLOW_NR_CROSS_PARTIES
		if (p_ptr->party && !p_ptr->admin_dm &&
		    compat_mode(p_ptr->mode, parties[p_ptr->party].cmode)) {
			/* need to leave party, since we might be teamed up with incompatible char mode players! */
			/* party_leave(Ind, FALSE); */
			if (streq(p_ptr->name, parties[p_ptr->party].owner)) {
				/* impossible, because the owner always has the same mode as the party's cmode */
				/* party_remove(Ind, p_ptr->name); */
			} else {
				parties[p_ptr->party].members--;
				Send_party(Ind, TRUE, FALSE);
				p_ptr->party = 0;
				clockin(Ind, 2);
			}
		}
#endif
	}

	/* hack for sector00separation (Highlander Tournament): Players mustn't enter sector 0,0 */
	/* note that this hack will also prevent players who got disconnected during Highlander Tourney
	   to continue it, but that must be accepted. Otherwise players could exploit it and just join
	   with a certain character during highlander tourneys and continue to level it up
	   infinitely! :) So, who gets disconnected will be removed from the event! */
	//if (sector00separation && ...) {
	if (wpos->wx == WPOS_SECTOR00_X && wpos->wy == WPOS_SECTOR00_Y) {
		/* Teleport him out of the event area */
#if 0
		switch rand_int(3){
		case 0:	wpos->wx = WPOS_SECTOR00_ADJAC_X;
		case 1:	wpos->wy = WPOS_SECTOR00_ADJAC_Y; break;
		case 2: wpos->wx = WPOS_SECTOR00_ADJAC_X;
		}
#else
		wpos->wx = cfg.town_x;
                wpos->wy = cfg.town_y;
#endif
		wpos->wz = 0;

		/* Remove him from the event and strip quest items off him */
		for (d = 0; d < MAX_GLOBAL_EVENTS; d++) {
			switch (p_ptr->global_event_type[d]) {
			case GE_NONE:
				/* everything is fine */
				break;
			case GE_HIGHLANDER:
				p_ptr->global_event_temp = PEVF_NONE;
				for (i = 0; i < INVEN_TOTAL; i++) /* Erase the highlander amulets */
					if (p_ptr->inventory[i].tval == TV_AMULET && 
					    (p_ptr->inventory[i].sval == SV_AMULET_HIGHLANDS || p_ptr->inventory[i].sval == SV_AMULET_HIGHLANDS2)) {
					    inven_item_increase(Ind, i, -p_ptr->inventory[i].number);
						inven_item_optimize(Ind, i);
					}
				break;
			}
			p_ptr->global_event_type[d] = GE_NONE;
		}
	}

#if 0 /* not really useful? */
	/* If he's in the training tower of Bree, check for running global events accordingly */
	if (in_arena(wpos)) {
		for (d = 0; d < MAX_GLOBAL_EVENTS; d++)
		if (p_ptr->global_event_type[d] != GE_NONE)
		switch (p_ptr->global_event_type[d]) {
		case GE_ARENA_MONSTER:
			wpos->wx = cfg.town_x;
			wpos->wy = cfg.town_y;
			wpos->wz = 0;
			break;
		}
	}
#endif

	/* If the player encountered a server crash last time he was saved (panic save),
	   carry him out of that dungeon to make sure he doesn't die to the crash. */
	if (p_ptr->panic) {
#ifndef RPG_SERVER    /* Not in RPG */
		if (wpos->wz && !in_irondeepdive(wpos)) {
			s_printf("Auto-recalled panic-saved player %s.\n", p_ptr->name);

 #ifdef ALLOW_NR_CROSS_PARTIES
			if (p_ptr->party && !p_ptr->admin_dm &&
			    at_netherrealm(wpos) && compat_mode(p_ptr->mode, parties[p_ptr->party].cmode)) {
				/* need to leave party, since we might be teamed up with incompatible char mode players! */
				/* party_leave(Ind, FALSE); */
				if (streq(p_ptr->name, parties[p_ptr->party].owner)) {
					/* impossible, because the owner always has the same mode as the party's cmode */
					/* party_remove(Ind, p_ptr->name); */
				} else {
					parties[p_ptr->party].members--;
					Send_party(Ind, TRUE, FALSE);
					p_ptr->party = 0;
					clockin(Ind, 2);
				}
			}
 #endif
 #ifdef ALLOW_NR_CROSS_ITEMS
			if (in_netherrealm(wpos))
				for (i = 1; i < INVEN_TOTAL; i++)
					p_ptr->inventory[i].NR_tradable = FALSE;
 #endif

#if 0
			wpos->wz = 0;

			/* Avoid landing in permanent rock or trees or mountains etc.
			   after an auto-recall, caused by a previous panic save. */
			p_ptr->auto_transport = AT_BLINK;
#endif
		}
#endif
		/* Avoid critical border spots which might lead to segfaults.. (don't ask) */
		if (p_ptr->px < 2) p_ptr->px = 2;
		if (p_ptr->px > MAX_WID - 2) p_ptr->px = MAX_WID - 2;
		if (p_ptr->py < 2) p_ptr->py = 2;
		if (p_ptr->py > MAX_HGT - 2) p_ptr->py = MAX_HGT - 2;
		p_ptr->panic = FALSE;
	}

#ifndef VAMPIRIC_MIST
	if (p_ptr->prace == RACE_VAMPIRE && p_ptr->body_monster == RI_VAMPIRIC_MIST)
		do_mimic_change(Ind, 0, TRUE);
#endif

	/* anti spammer code */
	p_ptr->msgcnt = 0;
	p_ptr->msg = 0;
	p_ptr->spam = 0;

	/* Default location if just starting */
	//if (wpos->wz == 0 && wpos->wy == 0 && wpos->wx == 0 && p_ptr->py == 0 && p_ptr->px == 0) {
	if (new) {
		p_ptr->wpos.wx = cfg.town_x;
		p_ptr->wpos.wy = cfg.town_y;
		p_ptr->wpos.wz = 0;
#if 0	// moved afterwards (since town can be non-allocated here)
		p_ptr->py = level_down_y(wpos);
		p_ptr->px = level_down_x(wpos);
#endif	// 0
		p_ptr->town_x = p_ptr->wpos.wx;
		p_ptr->town_y = p_ptr->wpos.wy;
	}

	/* Count players on this depth */
	for (i = 1; i <= NumPlayers; i++) {
		/* Skip this player */
		if (i == Ind) continue;

		/* Skip disconnected players */
		if (Players[i]->conn == NOT_CONNECTED)
			continue;

		/* Count */
		if (inarea(wpos, &Players[i]->wpos))
			count++;
	}

	/* Make sure he's supposed to be here -- if not, then the level has
	 * been unstaticed and so he should forget his memory of the old level.
	 */
	/* Problem: Player could have logged out, level unstaticed, someone else
	   entered it, logged out, still static, first player logs in and game
	   thinks it's HIS view and allows him to see the new level.. Solution
	   would probably be to add a sort of unique 'floor id' like item seeds - C. Blue */
	if (count >= players_on_depth(wpos)) {
#if 0 /* done below now - C. Blue */
		/* Clear the "marked" and "lit" flags for each cave grid */
		for (y = 0; y < MAX_HGT; y++) {
			for (x = 0; x < MAX_WID; x++) {
				p_ptr->cave_flag[y][x] = 0;
			}
		}
#endif

		/* He is now on the level, so add him to the player_on_depth list */
		new_players_on_depth(wpos, 1, TRUE);

		/* Set the unstaticed variable to true so we know to do a non-LOS requiring
		 * scatter when we place the player.  See below.
		 */
		//unstaticed = TRUE; /* seems unused atm! */
	}

	/* Rebuild the level if necessary */
	if (!(zcave = getcave(wpos))) {
		if (p_ptr->wpos.wz) {
			/* Clear the "marked" and "lit" flags for each cave grid */
			for (y = 0; y < MAX_HGT; y++) {
				for (x = 0; x < MAX_WID; x++) {
					p_ptr->cave_flag[y][x] = 0;
				}
			}

			alloc_dungeon_level(wpos);
			/* hack: Prevent generating a random dungeon town via relogging+unstaticing cheeze */
			p_ptr->dummy_option_8 = TRUE;
			generate_cave(wpos, p_ptr);
			p_ptr->dummy_option_8 = FALSE;
			l_ptr = getfloor(wpos);

			/* Player now starts mapping this dungeon (as far as its flags allow) */
			p_ptr->dlev_id = l_ptr->id;

			if ((l_ptr->wid < p_ptr->px) || (l_ptr->hgt < p_ptr->py)){
				p_ptr->px = l_ptr->wid / 2;
				p_ptr->py = l_ptr->hgt / 2;
				NumPlayers++; // hack for cave_midx_debug - mikaelh
				teleport_player_force(Ind, 1);
				NumPlayers--;
			}
		} else {
#if 0 /* if we assume that surface levels are mostly the same, we can keep view info - C. Blue */
			/* Clear the "marked" and "lit" flags for each cave grid */
			for (y = 0; y < MAX_HGT; y++) {
				for (x = 0; x < MAX_WID; x++) {
					p_ptr->cave_flag[y][x] = 0;
				}
			}
#endif

			alloc_dungeon_level(wpos);
			generate_cave(wpos, p_ptr);
			l_ptr = getfloor(wpos);
			if (!players_on_depth(wpos)) new_players_on_depth(wpos, 1, FALSE);
#ifndef NEW_DUNGEON
			/* paranoia, update the players wilderness map. */
			p_ptr->wild_map[(-p_ptr->dun_depth) / 8] |= (1U << ((-p_ptr->dun_depth) % 8));
#endif
			p_ptr->dlev_id = 0;
		}

		zcave = getcave(wpos);

		/* teleport players who logged into a non-existing/changed
		   floor, so they don't get stuck in walls - C. Blue */
		if (!player_can_enter(Ind, zcave[p_ptr->py][p_ptr->px].feat, TRUE)
		    /* max level limit to make players learn? or more comfort instead?
		      -- it's important for ppl who log back into IDDC to have this teleport! */
		    //&& p_ptr->lev < 10
		    ) {
			NumPlayers++; // hack for cave_midx_debug - mikaelh
			/* not a '_force'd teleport, so won't get out of NO_TELE vaults! */
			if (!teleport_player(Ind, 5, TRUE)) teleport_player(Ind, 200, TRUE);
			NumPlayers--;
		}

		/* for IDDC: We might be trying to log-scum here! In dubio pro duriore =P */
		if (in_irondeepdive(&p_ptr->wpos) && !panic
		    //&& !is_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos)))
		    && (!l_ptr || !(l_ptr->flags1 & LF1_DUNGEON_TOWN))) /* !l_ptr check just to silence the compiler.. */
			p_ptr->IDDC_logscum = TRUE;

#if 1
		/* fix ancient chars that had logscum flag set in their savegame, before the town was added */
		if (l_ptr && (l_ptr->flags1 & LF1_DUNGEON_TOWN)) p_ptr->IDDC_logscum = FALSE;
#endif
	} else if (p_ptr->wpos.wz) {
		bool unknown = FALSE;
		l_ptr = getfloor(wpos);

		/* If player doesn't know this level.. */
		if (l_ptr->id != p_ptr->dlev_id) {
			unknown = TRUE;

			/* Clear the "marked" and "lit" flags for each cave grid */
			for (y = 0; y < MAX_HGT; y++) {
				for (x = 0; x < MAX_WID; x++) {
					p_ptr->cave_flag[y][x] = 0;
				}
			}

			/* Player now starts mapping this dungeon (as far as its flags allow) */
			p_ptr->dlev_id = l_ptr->id;

			/* for IDDC: We might be trying to log-scum here! In dubio pro duriore =P */
			if (in_irondeepdive(&p_ptr->wpos) && !panic
			    //&& !is_fixed_irondeepdive_town(&p_ptr->wpos, getlevel(&p_ptr->wpos)))
			    && !(l_ptr->flags1 & LF1_DUNGEON_TOWN))
				p_ptr->IDDC_logscum = TRUE;

#if 1
			/* fix ancient chars that had logscum flag set in their savegame, before the town was added */
			if (l_ptr && (l_ptr->flags1 & LF1_DUNGEON_TOWN)) p_ptr->IDDC_logscum = FALSE;
#endif
		}
		/* <else> It's still the same level we left [a moment ago], np.
		   Here we just keep the IDDC_logscum that we read in load.c. */

		/* teleport players who logged into a non-existing/changed
		   floor, so they don't get stuck in walls - C. Blue */
		if (!player_can_enter(Ind, zcave[p_ptr->py][p_ptr->px].feat, TRUE)
		    /* max level limit? Otherwise it could be exploited
		       by taking off levitation ring/climbing set to phase around */
		    && (p_ptr->lev < 10 || unknown) /* added the 'unknown' hack for IDDC, otherwise it may suck at times */
		    ) {
			NumPlayers++; // hack for cave_midx_debug - mikaelh
			/* not a '_force'd teleport, so won't get out of NO_TELE vaults! */
			if (!teleport_player(Ind, 5, TRUE)) teleport_player(Ind, 200, TRUE);
			NumPlayers--;
		}
	}
	/* could add an 'else' branch to teleport players on surface,
	   see 'if player_can_enter()' stuff above. - C. Blue */

	/* hack -- update night/day on world surface */
	if (!wpos->wz) {
		/* hack: temporarily allow the player to be called in wild
		   view update routines (player_day/player_night) */
		//NumPlayers++;

		/* hack #2: assume his (so far not loaded!) options are
		   set the way that he remembers all town features */
		i = p_ptr->view_perma_grids;
		p_ptr->view_perma_grids = TRUE;

		//if (IS_DAY) world_surface_day(wpos);
		//else world_surface_night(wpos);
		if (IS_DAY) player_day(Ind);
		else player_night(Ind);

		p_ptr->view_perma_grids = i;
		//NumPlayers--;

		/* Don't allow players to save in houses they don't own -> teleport them - C. Blue */
		/* Assumption: wpos.wz==0 and CAVE_ICKY -> we're inside a house*/
		if ((zcave[p_ptr->py][p_ptr->px].info & CAVE_ICKY) &&
		    /* slightly paranoid hack, in case we really are in a no-tele wilderness vault oO */
		    !(zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK)) {
			/* house here which we are allowed to enter? */
			if (!wraith_access(Ind)) {
				teleport_player_force(Ind, 1);
				s_printf("Out-of-House-blinked %s at wx %d wy %d wz %d\n", p_ptr->name, wpos->wx, wpos->wy, wpos->wz);
			}
		}
	} else if (isdungeontown(wpos)) {
		/* make whole dungeon town visible like a 'normal town at night' */
		player_dungeontown(Ind);
	}

	/* blink by 1 if standing on a shop grid (in town) */
	if (zcave[p_ptr->py][p_ptr->px].feat == FEAT_SHOP)
		teleport_player_force(Ind, 1);

	if (new) {
#if 0
		p_ptr->py = level_down_y(wpos);
		p_ptr->px = level_down_x(wpos);
#else	// 0
		y = level_rand_y(wpos);
		x = level_rand_x(wpos);
		p_ptr->py = y;
		p_ptr->px = x;

#endif	// 0
	}

	/* Hack be sure the player is inbounds */
	if (p_ptr->px < 1) p_ptr->px = 1;
	if (p_ptr->py < 1) p_ptr->py = 1;
	if (p_ptr->px >= MAX_WID) p_ptr->px = MAX_WID - 1;
	if (p_ptr->py >= MAX_HGT) p_ptr->py = MAX_HGT - 1;

	x = p_ptr->px;
	y = p_ptr->py;

	/* Don't stack with another player */
	if (zcave[y][x].m_idx && zcave[y][x].m_idx != 0 - Ind) {
		for (i = 0; i < 3000; i++) {
			d = (i + 9) / 10;
			//scatter(wpos, &y, &x, p_ptr->py, p_ptr->px, d, 0);
			/* Don't require LOS anymore - mikaelh */
			scatter(wpos, &y, &x, p_ptr->py, p_ptr->px, d, 1);

			if (!in_bounds(y, x) || !cave_empty_bold(zcave, y, x)) continue;
			else {
				p_ptr->px = x;
				p_ptr->py = y;
			}

			break;
		}
	}

	/* Update the location's player index */
	zcave[y][x].m_idx = 0 - Ind;
	NumPlayers++; // increase temporarily to get rid of the out of range warnings from cave_midx_debug - mikaelh
	cave_midx_debug(wpos, y, x, -Ind);
	NumPlayers--;

	/* Show him to everybody */
	everyone_lite_spot(wpos, y, x);
	/* Hack -- Give him "awareness" of certain objects */
	for (i = 1; i < max_k_idx; i++) {
		object_kind *k_ptr = &k_info[i];

		/* Skip "empty" objects */
		if (!k_ptr->name) continue;

		/* No flavor yields aware */
		if (!k_ptr->has_flavor) p_ptr->obj_aware[i] = TRUE;
	}

	/* Add him to the player name database, if he is not already there */
	if (!lookup_player_name(p_ptr->id)) {
		byte w;
		time_t ttime;
		/* Add */
		w = (p_ptr->total_winner ? 1 : 0) + (p_ptr->once_winner ? 2 : 0) + (p_ptr->iron_winner ? 4 : 0) + (p_ptr->iron_winner_ded ? 8 : 0);
		add_player_name(p_ptr->name, p_ptr->id, p_ptr->account, p_ptr->prace, p_ptr->pclass, p_ptr->mode, 1, 0, 0, 0, 0, time(&ttime), p_ptr->admin_dm ? 1 : (p_ptr->admin_wiz ? 2 : 0), *wpos, p_ptr->houses_owned, w);
	} else {
	/* Verify his data - only needed for 4.2.0 -> 4.2.2 savegame conversion :) - C. Blue */
	/* Now also needed for 4.5.2 -> 4.5.3 again ^^ To stamp guild info into hash table, for self-adding */
	/* And for 4.6.2, winner status for item-sending (ENABLE_MERCHANT_MAIL) */
		time_t ttime;
		byte w;
		/* Verify mode */
		w = (p_ptr->total_winner ? 1 : 0) + (p_ptr->once_winner ? 2 : 0) + (p_ptr->iron_winner ? 4 : 0) + (p_ptr->iron_winner_ded ? 8 : 0);
		verify_player(p_ptr->name, p_ptr->id, p_ptr->account, p_ptr->prace, p_ptr->pclass, p_ptr->mode, p_ptr->lev, p_ptr->party, p_ptr->guild, p_ptr->guild_flags, 0, time(&ttime), p_ptr->admin_dm ? 1 : (p_ptr->admin_wiz ? 2 : 0), *wpos, (char)p_ptr->houses_owned, w);
	}

	/* Set his "current activities" variables */
	p_ptr->current_char = 0;
	p_ptr->current_spell = p_ptr->current_rod = p_ptr->current_wand = p_ptr->current_activation = -1;
	p_ptr->current_mind = -1;
	p_ptr->current_selling = -1;
	p_ptr->current_rcraft = -1;
#ifdef AUCTION_SYSTEM
	p_ptr->current_auction = 0;
#endif
#ifdef PLAYER_STORES
	p_ptr->ps_house_x = p_ptr->ps_house_y = -1;
	p_ptr->ps_mcheque_x = p_ptr->ps_mcheque_y = -1;
#endif
	p_ptr->current_item = -1;
	p_ptr->current_book = -1;
	p_ptr->current_aux = -1;
	p_ptr->current_realm = -1;
	p_ptr->current_fire = -1;
	p_ptr->current_throw = -1;
	p_ptr->current_breath = 0;

#if 0 //it's zero'ed anyway..
	/* Runecraft */
	p_ptr->shoot_till_kill_rcraft = FALSE;
	/* disable other ftk types */
	p_ptr->shoot_till_kill_mimic = 0;
	p_ptr->shoot_till_kill_spell = 0;
	p_ptr->shoot_till_kill_wand = 0;
	p_ptr->shoot_till_kill_rod = 0;
	/* totally obsolete and redundant */
	p_ptr->IDDC_logscum = FALSE;
#endif

	/* No item being used up */
	p_ptr->using_up_item = -1;

	/* Drain-HP hack for client recognition when to warn */
	p_ptr->hp_drained = TRUE;

	/* Set the player's "panel" information */
	l_ptr = getfloor(wpos);
	if (l_ptr) {
		/* Hack -- tricky formula, but needed */
		p_ptr->max_panel_rows = MAX_PANEL_ROWS_L;
		p_ptr->max_panel_cols = MAX_PANEL_COLS_L;

		p_ptr->max_tradpanel_rows = MAX_TRADPANEL_ROWS_L;
		p_ptr->max_tradpanel_cols = MAX_TRADPANEL_COLS_L;

		p_ptr->cur_hgt = l_ptr->hgt;
		p_ptr->cur_wid = l_ptr->wid;
	} else {
		p_ptr->max_panel_rows = MAX_PANEL_ROWS;
		p_ptr->max_panel_cols = MAX_PANEL_COLS;

		p_ptr->max_tradpanel_rows = MAX_TRADPANEL_ROWS;
		p_ptr->max_tradpanel_cols = MAX_TRADPANEL_COLS;

		p_ptr->cur_hgt = MAX_HGT;
		p_ptr->cur_wid = MAX_WID;
	}

#ifdef BIG_MAP
	if (p_ptr->max_panel_rows < 0) p_ptr->max_panel_rows = 0;
	if (p_ptr->max_panel_cols < 0) p_ptr->max_panel_cols = 0;
#endif

	panel_calculate(Ind);


#if defined(ALERT_OFFPANEL_DAM) || defined(LOCATE_KEEPS_OVL)
	/* For alert-beeps on damage: Init remembered panel */
	p_ptr->panel_row_old = p_ptr->panel_row;
	p_ptr->panel_col_old = p_ptr->panel_col;
#endif

	/* Make sure his party still exists */
	if (p_ptr->party && parties[p_ptr->party].members == 0) {
		/* Reset to neutral */
		p_ptr->party = 0;
		clockin(Ind, 2);
	}

	/* Make sure his guild still exists */
	if (p_ptr->guild && guilds[p_ptr->guild].members == 0 && !strlen(guilds[p_ptr->guild].name)) {
		/* Reset to neutral */
		p_ptr->guild = 0;
		clockin(Ind, 3);
	}


#if 1 /* fix problem that player logging on on regenerated level cant see himself at the beginning */
 #if 0
	/* allow to instantly determine the terrain type we start _in_ (could be walls/trees) */
  #if 0 /* panic saves at this stage, if this code happens in if(alloc).. stuff above already -- DOESNT WORK? */
	note_spot(Ind, p_ptr->py, p_ptr->px);
  #else /* so we'd have to do it manually if this code is up there instead (sigh) */
	p_ptr->cave_flag[p_ptr->py][p_ptr->px] |= CAVE_MARK;
  #endif
 #endif
	/* make sure he sees himself on the (possibly dark) map */
	everyone_lite_spot(wpos, p_ptr->py, p_ptr->px);
#endif


	/* Tell the server to redraw the player's display */
	p_ptr->redraw |= PR_MAP | PR_EXTRA | PR_BASIC | PR_HISTORY | PR_VARIOUS;
	p_ptr->redraw |= PR_PLUSSES | PR_STATE;

	/* Update his view, light, bonuses, and torch radius */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_BONUS | PU_TORCH | PU_DISTANCE | PU_SKILL_INFO | PU_SKILL_MOD | PU_LUA);

	/* Update his inventory, equipment, and spell info */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);


	p_ptr->master_move_hook = NULL; /* just in case its not */

	/* This guy is alive now */
	p_ptr->alive = TRUE;

	player_create_tmpfile(Ind);
}


/* Hack -- give the bard(or whatever randskill class) random skills	- Jir - */
#if 0
static void do_bard_skill(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int i, j;
	int value, mod;

	for (i = 0; i < MAX_SKILLS; i++)
	{
		/* Receives most of 'father' skills for free */
		if (i == SKILL_COMBAT || i == SKILL_MASTERY ||
				i == SKILL_ARCHERY || i == SKILL_MAGIC ||
				i == SKILL_SNEAKINESS || i == SKILL_HEALTH ||
//				i == SKILL_NECROMANCY ||
				i == SKILL_SPELL ||
				i == SKILL_AURA_POWER || i == SKILL_DODGE) continue;

		if (magik(67))
		{
			bool resist = FALSE;
			/* Racial skill 'resist' */
			for (j = 0; j < MAX_SKILLS; j++)
			{
				/* Efficiency */
				if (!p_ptr->rp_ptr->skills[j].skill) break;

				if (p_ptr->rp_ptr->skills[j].skill == i)
				{
					value = p_ptr->rp_ptr->skills[j].value;
					mod = p_ptr->rp_ptr->skills[j].mod;

					if (!value && !mod) continue;
					if (p_ptr->rp_ptr->skills[j].vmod == '=')
					{
						resist = TRUE;
						break;
					}
					if (p_ptr->rp_ptr->skills[j].vmod == '+')
					{
						if (!resist)
						{
							p_ptr->s_info[i].value = value;
							p_ptr->s_info[i].mod = mod;
							resist = TRUE;
						}
						/* Two plusses? bad manner, but we'll handle it.. */
						else
						{
							p_ptr->s_info[i].value += value;
							p_ptr->s_info[i].mod += mod;
						}
						continue;
					}
				}
			}

			if (resist) continue;

			p_ptr->s_info[i].value = 0;
			p_ptr->s_info[i].mod = 0;
		}
		else
		{
			p_ptr->s_info[i].value *= 2;
			p_ptr->s_info[i].mod = (p_ptr->s_info[i].mod * 3) / 2;
		}
	}

	/* Father zero, child zero */
	for (i = 0; i < MAX_SKILLS; i++)
	{
		//s32b value = 0, mod = 0;

		/* Develop only revelant branches */
		if (p_ptr->s_info[i].value || p_ptr->s_info[i].mod)
		{
			int z = s_info[i].father;

			if (z == -1 || z == SKILL_NECROMANCY || z == SKILL_MISC) continue;

			if (!p_ptr->s_info[z].value && !p_ptr->s_info[z].mod)
			{
				p_ptr->s_info[z].dev = FALSE;
				p_ptr->s_info[i].value = p_ptr->s_info[i].mod = 0;
			}
		}
	}
}
#endif /*0*/

/* Disable various warnings, if the player chose a class that isn't affected primarily: */
void disable_specific_warnings(player_type *p_ptr) {
	/* disable ALL warnings? (client-side option) */
	if (!p_ptr->newbie_hints) {
		p_ptr->warning_bpr = 1;
		p_ptr->warning_bpr2 = 1;
		p_ptr->warning_bpr3 = 1;
		p_ptr->warning_run = 3;//p_ptr->warning_run_steps = ;
		p_ptr->warning_run_monlos = 1;
		p_ptr->warning_run_lite = 10;
		p_ptr->warning_wield = 1;
		p_ptr->warning_chat = 1;
		p_ptr->warning_lite = 1;
		p_ptr->warning_lite_refill = 1;
		p_ptr->warning_wield_combat = 1;
		p_ptr->warning_rest = 3;
		p_ptr->warning_mimic = 1;
		p_ptr->warning_dual = 1;
		p_ptr->warning_dual_mode = 1;
		p_ptr->warning_potions = 1;
		p_ptr->warning_wor = 1;
		p_ptr->warning_ghost = 1;
		p_ptr->warning_instares = 1;
		p_ptr->warning_autoret = 99;//p_ptr->warning_autoret_ok = ;
		//p_ptr->warning_ma_weapon = 1; leave enabled
		//p_ptr->warning_ma_shield = 1; leave enabled
		p_ptr->warning_technique_melee = 1;
		p_ptr->warning_technique_ranged = 1;
		p_ptr->warning_hungry = 2;
		p_ptr->warning_autopickup = PY_MAX_LEVEL;
		p_ptr->warning_ranged_autoret = 1;
		p_ptr->warning_cloak = 1;
		p_ptr->warning_macros = 1;
		p_ptr->warning_numpadmove = 1;
		p_ptr->warning_ammotype = 1;
		p_ptr->warning_ai_annoy = 1;
		p_ptr->warning_fountain = 1;
		p_ptr->warning_voidjumpgate = 1;
		p_ptr->warning_staircase = 1;
		p_ptr->warning_worldmap = 1;
		p_ptr->warning_dungeon = 1;
		p_ptr->warning_tunnel = 1;
		p_ptr->warning_tunnel2 = 1;
		p_ptr->warning_tunnel3 = 1;
		p_ptr->warning_trap = 1;
		p_ptr->warning_tele = 1;
		p_ptr->warning_fracexp = 1;
		p_ptr->warning_death = 1;
		p_ptr->warning_drained = 1;
		p_ptr->warning_boomerang = 1;
		p_ptr->warning_bash = 1;
		p_ptr->warning_inspect = 1;
		return;
	}

	if (p_ptr->pclass == CLASS_ARCHER ||
	    p_ptr->pclass == CLASS_ADVENTURER ||
	    p_ptr->pclass == CLASS_SHAMAN ||
	    p_ptr->pclass == CLASS_MAGE) {
		p_ptr->warning_autoret = 99;
	}

	if (p_ptr->pclass == CLASS_ARCHER ||
	    p_ptr->pclass == CLASS_ADVENTURER || p_ptr->pclass == CLASS_DRUID ||
	    p_ptr->pclass == CLASS_MAGE || p_ptr->pclass == CLASS_SHAMAN) {
		p_ptr->warning_wield = 1;
	}

	/* classes that may go without using any [ranged] weapons don't need a wield-warning */
	if (p_ptr->pclass == CLASS_ADVENTURER ||
	    p_ptr->pclass == CLASS_DRUID ||
	    p_ptr->pclass == CLASS_SHAMAN ||
	    p_ptr->pclass == CLASS_MAGE)
		p_ptr->warning_wield_combat = 1;

	if (p_ptr->pclass == CLASS_MAGE ||
//	    p_ptr->pclass == CLASS_RUNEMASTER ||
	    p_ptr->pclass == CLASS_ARCHER) {
		p_ptr->warning_bpr = 1;
	}

	if (!(p_ptr->pclass == CLASS_WARRIOR || p_ptr->pclass == CLASS_PALADIN
#ifdef ENABLE_DEATHKNIGHT
	    || p_ptr->pclass == CLASS_DEATHKNIGHT
#endif
#ifdef ENABLE_HELLKNIGHT
	    || p_ptr->pclass == CLASS_HELLKNIGHT
#endif
	    || p_ptr->pclass == CLASS_ROGUE || p_ptr->pclass == CLASS_MIMIC
	    //|| p_ptr->pclass == CLASS_RUNEMASTER
	    //|| p_ptr->pclass == CLASS_RANGER || p_ptr->pclass == CLASS_MINDCRAFTER
	    )) {
		p_ptr->warning_bpr2 = 1;
	}

	if (!(p_ptr->pclass == CLASS_WARRIOR || p_ptr->pclass == CLASS_PALADIN
#ifdef ENABLE_DEATHKNIGHT
	    || p_ptr->pclass == CLASS_DEATHKNIGHT
#endif
#ifdef ENABLE_HELLKNIGHT
	    || p_ptr->pclass == CLASS_HELLKNIGHT
#endif
	    || p_ptr->pclass == CLASS_ROGUE || p_ptr->pclass == CLASS_MIMIC
	    //|| p_ptr->pclass == CLASS_RUNEMASTER
	    //|| p_ptr->pclass == CLASS_RANGER || p_ptr->pclass == CLASS_MINDCRAFTER
	    )) {
		p_ptr->warning_bpr3 = 1;
	}

	/* fruit bat mode chars can't wield weapons */
	if (p_ptr->fruit_bat == 1) {
		p_ptr->warning_wield = 1;
		p_ptr->warning_bpr = p_ptr->warning_bpr2 = p_ptr->warning_bpr3 = 1;
		p_ptr->warning_wield_combat = 1;
	}

	/* cannot use melee techniques? */
	if (!(p_ptr->pclass == CLASS_WARRIOR ||
	    p_ptr->pclass == CLASS_RANGER || p_ptr->pclass == CLASS_PALADIN ||
#ifdef ENABLE_DEATHKNIGHT
	    p_ptr->pclass == CLASS_DEATHKNIGHT ||
#endif
#ifdef ENABLE_HELLKNIGHT
	    p_ptr->pclass == CLASS_HELLKNIGHT ||
#endif
	    p_ptr->pclass == CLASS_MIMIC || p_ptr->pclass == CLASS_ROGUE ||
	    p_ptr->pclass == CLASS_ADVENTURER || p_ptr->pclass == CLASS_RUNEMASTER ||
	    p_ptr->pclass == CLASS_MINDCRAFTER))
		p_ptr->warning_technique_melee = 1;

	/* cannot use ranged techniques? */
	if (!(p_ptr->pclass == CLASS_ARCHER || p_ptr->pclass == CLASS_RANGER))
		p_ptr->warning_technique_ranged = 1;

	/* Classes other than archer might want to perform melee auto-retaliation */
	if (p_ptr->pclass != CLASS_ARCHER) p_ptr->warning_ranged_autoret = 1;

	/* Vampires don't use normal light sources */
	if (p_ptr->prace == RACE_VAMPIRE) p_ptr->warning_lite = 1;

	/* Martial artists shouldn't get a weapon-wield warning */
	if (get_skill(p_ptr, SKILL_MARTIAL_ARTS)) {
		p_ptr->warning_wield = 1;
		/* also don't send any weapon-bpr related warnings since their
		   suggested remedies don't work for us as MA user */
		p_ptr->warning_bpr = 1;
		p_ptr->warning_bpr2 = 1;
		p_ptr->warning_bpr3 = 1;
	}

	if (p_ptr->pclass != CLASS_ROGUE)
		p_ptr->warning_cloak = 1;

#ifdef RPG_SERVER
	/* No need for WoR hints in an all-Ironman environment */
	p_ptr->warning_wor = 1;
#endif

	if (!(p_ptr->mode & MODE_EVERLASTING))
		p_ptr->warning_instares = 1;

	if ((p_ptr->mode & (MODE_DED_IDDC | MODE_PVP)))
		p_ptr->warning_dungeon = 1;

	if ((p_ptr->mode & MODE_DED_IDDC))
		p_ptr->warning_wor = 1;

	if ((p_ptr->mode & (MODE_NO_GHOST | MODE_PVP))) {
		p_ptr->warning_ghost = 1;
		p_ptr->warning_death = 1;
	}

	/* Some warnings cease at certain levels */
	disable_lowlevel_warnings(p_ptr);
}

/* Disable warnings when player reaches certain levels */
void disable_lowlevel_warnings(player_type *p_ptr) {
	if (p_ptr->max_plv > 1) {
		p_ptr->warning_bpr2 = 1;
		p_ptr->warning_bpr3 = 1;
		p_ptr->warning_ammotype = 1;
		p_ptr->warning_wield = 1;
	}
	if (p_ptr->max_plv > 2) {
		p_ptr->warning_wield_combat = 1;
		p_ptr->warning_fracexp = 1;
	}
	if (p_ptr->max_plv > 3) {
		p_ptr->warning_worldmap = 1;
		p_ptr->warning_dungeon = 1;
		p_ptr->warning_run_monlos = 1;
	}
	if (p_ptr->max_plv > 4) {
		p_ptr->warning_voidjumpgate = 1;
		p_ptr->warning_staircase = 1;
		p_ptr->warning_autoret = 99;//p_ptr->warning_autoret_ok = ;
	}
	if (p_ptr->max_plv > 5) {
		p_ptr->warning_run = 3;
		p_ptr->warning_lite = 1;
	}
	if (p_ptr->max_plv > 7) {
		p_ptr->warning_trap = 1;
		p_ptr->warning_fountain = 1;
	}
	if (p_ptr->max_plv > 10) {
		p_ptr->warning_ghost = 1;
		p_ptr->warning_dual = 1;
		p_ptr->warning_run_lite = 10;
		p_ptr->warning_ranged_autoret = 1;
		p_ptr->warning_mimic = 1;
		p_ptr->warning_tunnel = 1;
		p_ptr->warning_tunnel2 = 1;
		p_ptr->warning_tunnel3 = 1;
		p_ptr->warning_bash = 1;
	}
	if (p_ptr->max_plv > 15) {
		p_ptr->warning_bpr = 1;
		p_ptr->warning_rest = 3;
		p_ptr->warning_hungry = 1;
		p_ptr->warning_macros = 1;
		p_ptr->warning_boomerang = 1;
		p_ptr->warning_inspect = 1;
	}
	if (p_ptr->max_plv > 20) {
		p_ptr->warning_dual_mode = 1;
		p_ptr->warning_hungry = 2;
		p_ptr->warning_lite_refill = 1;
	}
	if (p_ptr->max_plv >= 25) {
		/* mimics, as the latest learners, learn sprint at 15 and taunt at 20 */
		p_ptr->warning_ai_annoy = 1;
	}
	if (p_ptr->max_plv > 30) {
		p_ptr->warning_instares = 1;
		p_ptr->warning_death = 1;
		p_ptr->warning_drained = 1;
	}
}

#if 0
/* See do_Maia_skill() - Kurzel */
/* Set up trait-dependant skill boni (at birth) */
static void do_trait_skill(int Ind, int s, int m) {
	int tmp_val = Players[Ind]->s_info[s].mod;
	/* Modify skill, avoiding overflow (mod is u16b) */
	tmp_val = (tmp_val * m) / 100;
	/* Cap to 2.0 */
	if (tmp_val > 2000) tmp_val = 2000;
	Players[Ind]->s_info[s].mod = tmp_val;
	/* Update it after the re-increasing has been finished */
	Send_skill_info(Ind, s, FALSE);
}
#endif

/*
 * Create a character.  Then wait for a moment.
 *
 * The delay may be reduced, but is recommended to keep players
 * from continuously rolling up characters, which can be VERY
 * expensive CPU wise.
 *
 * Note that we may be called with "junk" leftover in the various
 * fields, so we must be sure to clear them first.
 */
bool player_birth(int Ind, int conn, connection_t *connp) {
	player_type *p_ptr;
	int i;
	struct account acc;
	bool acc_banned = FALSE;
	char acc_houses = 0;

	cptr accname = connp->nick, name = connp->c_name;
	int race = connp->race, class = connp->class, trait = connp->trait, sex = connp->sex;
	int stat_order[6];
	stat_order[0] = connp->stat_order[0];
	stat_order[1] = connp->stat_order[1];
	stat_order[2] = connp->stat_order[2];
	stat_order[3] = connp->stat_order[3];
	stat_order[4] = connp->stat_order[4];
	stat_order[5] = connp->stat_order[5];

#ifdef ENABLE_DEATHKNIGHT
	/* Unhack the artificial duplicate slot usage of Paladin/Death Knight class */
	if (class == CLASS_PALADIN && race == RACE_VAMPIRE) class = CLASS_DEATHKNIGHT;
#endif
#ifdef ENABLE_HELLKNIGHT
	/* Unhack the artificial duplicate slot usage of Paladin/Hell Knight class */
	if (class == CLASS_PALADIN && trait == TRAIT_CORRUPTED) class = CLASS_HELLKNIGHT; //cannot actually happen here
#endif
#ifdef ENABLE_CPRIEST
	/* Unhack the artificial duplicate slot usage of Priest/Corrupted Priest class */
	if (class == CLASS_PRIEST && trait == TRAIT_CORRUPTED) class = CLASS_CPRIEST; //cannot actually happen here
#endif

	/* To find out which characters crash the server */
	s_printf("Trying to log in with character %s...\n", name);

	/* Do some consistency checks */
	if (race < 0 || race >= MAX_RACE) race = 0;
	if (class < 0 || class >= MAX_CLASS) class = 0;
	if (trait < 0 || trait >= MAX_TRAIT) trait = 0;
//	if (sex < 0 || sex > 7) sex = 0;
	/* Check for legal class */
	if ((race_info[race].choice & BITS(class)) == 0) {
		s_printf("%s EXPLOIT_CLASS_CHOICE: %s(%s) chose race %d ; class %d ; trait %d.\n", showtime(), accname, name, race, class, trait);
		return FALSE;
	}
	/* If we have no traits available at all for this race then any trait choice is illegal*/
	if (trait && (trait_info[0].choice & BITS(race))) {
		s_printf("%s EXPLOIT_TRAIT_N/A: %s(%s) chose race %d ; class %d ; trait %d.\n", showtime(), accname, name, race, class, trait);
		return FALSE;
	}
	/* Check for legal trait */
	if ((trait_info[trait].choice & BITS(race)) == 0) {
		s_printf("%s EXPLOIT_TRAIT_CHOICE: %s(%s) chose race %d ; class %d ; trait %d.\n", showtime(), accname, name, race, class, trait);
		return FALSE;
	}

	/* Allocate memory for him */
	MAKE(Players[Ind], player_type);

	/* Allocate memory for his inventory */
	Players[Ind]->inventory = C_NEW(INVEN_TOTAL, object_type);

	/* Allocate memory for the copy */
	Players[Ind]->inventory_copy = C_NEW(INVEN_TOTAL, object_type);

	/* Set pointer */
	p_ptr = Players[Ind];

	/* Clear old information */
	player_wipe(Ind);

	/* Receive info found in PKT_SCREEN_DIM otherwise */
	p_ptr->screen_wid = connp->Client_setup.screen_wid;
	p_ptr->screen_hgt = connp->Client_setup.screen_hgt;

	/* Copy his name and connection info */
	strcpy(p_ptr->name, name);
	p_ptr->conn = conn;

	/* Verify his name and create a savefile name */
	if (!process_player_name(Ind, TRUE)) return FALSE;

	if (GetAccount(&acc, accname, NULL, FALSE)) {
		p_ptr->account = acc.id;
		p_ptr->noscore = (acc.flags & ACC_NOSCORE);
		p_ptr->inval = (acc.flags & ACC_TRIAL);
		/* more flags - C. Blue */
		p_ptr->restricted = (acc.flags & ACC_VRESTRICTED) ? 2 : (acc.flags & ACC_RESTRICTED) ? 1 : 0;
		p_ptr->privileged = (acc.flags & ACC_VPRIVILEGED) ? 2 : (acc.flags & ACC_PRIVILEGED) ? 1 : 0;
		p_ptr->pvpexception = (acc.flags & ACC_PVP) ? 1 : (acc.flags & ACC_NOPVP) ? 2 : (acc.flags & ACC_ANOPVP) ? 3 : 0;
		p_ptr->mutedchat = (acc.flags & ACC_VQUIET) ? 2 : (acc.flags & ACC_QUIET) ? 1 : 0;
		acc_banned = (acc.flags & ACC_BANNED) ? TRUE : FALSE;
		s_printf("(%s) ACC1:Player %s has flags %d\n", showtime(), accname, acc.flags);
		acc_houses = acc.houses;
	}

	/* handle banned player 1/2 */
	if (acc_banned) {
		msg_print(Ind, "\377R*** Your account is temporarily suspended ***");
		s_printf("Refused ACC_BANNED account %s (character %s)\n.", accname, name);
		return FALSE;
	}

	/* Attempt to load from a savefile */
	character_loaded = FALSE;

	confirm_admin(Ind);

	/* Try to load */
	if (!load_player(Ind)) {
		/* Loading failed badly */
		return FALSE;
	}

	/* init account-wide houses limit? // ACC_HOUSE_LIMIT */
	if (acc_houses == -1) {
		if (GetAccount(&acc, accname, NULL, FALSE)) {
			/* grab sum from hash table entries */
			acc_houses = acc_sum_houses(&acc);

			acc_set_houses(accname, acc_houses);
			s_printf("ACC_HOUSE_LIMIT_INIT: initialised %s(%s) with %d.\n", p_ptr->name, accname, acc_houses);
		} else s_printf("ACC_HOUSE_LIMIT_INIT_ERROR: couldn't get account %s.\n", accname);
	} else s_printf("ACC_HOUSE_LIMIT: read %s(%s) with %d.\n", p_ptr->name, accname, acc_houses);

	/* Did loading succeed? */
	if (character_loaded) {
		/* Log for freeze bug */
		s_printf("Loaded character %s on %d %d %d\n", name, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);
		/* Loading succeeded */
		player_setup(Ind, FALSE);
		clockin(Ind, 0);	/* Timestamp the player */
		clockin(Ind, 1);	/* Set player level */
		clockin(Ind, 2);	/* Set player party */
		if (p_ptr->xorder_id){
			for (i = 0; i < MAX_XORDERS; i++){
				if (xorders[i].active && xorders[i].id == p_ptr->xorder_id) break;
			}
			if (i == MAX_XORDERS) p_ptr->xorder_id = 0;
		}

		/* hack: if he's in town, get him pseudo tree-passing */
		if (istown(&p_ptr->wpos) || isdungeontown(&p_ptr->wpos)) p_ptr->town_pass_trees = TRUE;

#if 0
		/* Don't allow players to save in houses they don't own -> teleport them */
		if ((!p_ptr->wpos.wz) && (cave_info..  [wpos->wy][wpos->wx]. & CAVE_ICKY)) {
			s_printf("Out-of-House-blinked %s at wx %d wy %d wz %d\n", p_ptr->name, wpos->wx, wpos->wy, wpos->wz);
			teleport_player(Ind, 1);
		}
#endif

		p_ptr->newbie_hints = TRUE;
		disable_specific_warnings(p_ptr);

		return TRUE;
	}

	/* Else, loading failed, but we just create a new character */

	/* Hack -- rewipe the player info if load failed */
	player_wipe(Ind);

	/* Receive info found in PKT_SCREEN_DIM otherwise */
	p_ptr->screen_wid = connp->Client_setup.screen_wid;
	p_ptr->screen_hgt = connp->Client_setup.screen_hgt;

	/* Copy his name and connection info */
	strcpy(p_ptr->name, name);
	p_ptr->conn = conn;

	/* again ;( */
	if (GetAccount(&acc, accname, NULL, FALSE)) {
		p_ptr->account = acc.id;
		p_ptr->noscore = (acc.flags & ACC_NOSCORE);
		p_ptr->inval = (acc.flags & ACC_TRIAL);
		/* more flags - C. Blue */
		p_ptr->restricted = (acc.flags & ACC_VRESTRICTED) ? 2 : (acc.flags & ACC_RESTRICTED) ? 1 : 0;
		p_ptr->privileged = (acc.flags & ACC_VPRIVILEGED) ? 2 : (acc.flags & ACC_PRIVILEGED) ? 1 : 0;
		p_ptr->pvpexception = (acc.flags & ACC_PVP) ? 1 : (acc.flags & ACC_NOPVP) ? 2 : (acc.flags & ACC_ANOPVP) ? 3 : 0;
		p_ptr->mutedchat = (acc.flags & ACC_VQUIET) ? 2 : (acc.flags & ACC_QUIET) ? 1 : 0;
		acc_banned = (acc.flags & ACC_BANNED) ? TRUE : FALSE;
//		s_printf("(%s) ACC2:Player %s has flags %d\n", showtime(), accname, acc.flags);
		s_printf("(%s) ACC2:Player %s has flags %d (%s)\n", showtime(), accname, acc.flags, get_conn_userhost(conn));
//		s_printf("(%s) ACC2:Player %s has flags %d (%s@%s)\n", showtime(), accname, acc.flags, Conn[conn]->real, Conn[conn]->host);
	}
	/* handle banned player 2/2 */
	if (acc_banned) {
		msg_print(Ind, "\377R*** Your account is temporarily suspended ***");
		s_printf("Refused ACC_BANNED account %s (character %s)\n.", accname, name);
		return FALSE;
	}

	p_ptr->turn = turn; /* Birth time (for info) */

	/* Reprocess his name */
	if (!process_player_name(Ind, TRUE)) return FALSE;

	confirm_admin(Ind);

	/* player is not in a game */
	p_ptr->team = 0;

	/* paranoia? */
	p_ptr->skill_points = 0;

	/* Set info */
	p_ptr->mode |= sex & ~MODE_MALE;

#if 1 /* keep for now to stay compatible, doesn't hurt us */
	if (sex > 511) {
		sex -= 512;
		p_ptr->mode |= MODE_FRUIT_BAT;
	}
#endif

#if 0
	/* hack? disallow 'pvp mode' for new players? */
	if (p_ptr->inval && (p_ptr->mode & MODE_PVP)) {
		p_ptr->mode &= ~MODE_PVP;
		msg_print(Ind, "Because you're a new, your mode has been changed from 'pvp' to 'normal'.");
	}
#endif

	/* no more stand-alone 'hard mode' */
	if ((p_ptr->mode & MODE_HARD) &&
		!(p_ptr->mode & MODE_NO_GHOST)) {
			p_ptr->mode &= ~MODE_HARD;
			/* note- cannot msg_print() to player at this point yet, actually */
			msg_print(Ind, "Hard mode is no longer supported - choose hellish mode instead.");
			msg_print(Ind, "Your character has automatically been converted to normal mode.");
	}

	/* fix potential exploits */
	if (p_ptr->mode & MODE_EVERLASTING) p_ptr->mode &= ~(MODE_HARD | MODE_NO_GHOST);
	if (p_ptr->mode & MODE_PVP) p_ptr->mode &= ~(MODE_EVERLASTING | MODE_HARD | MODE_NO_GHOST | MODE_FRUIT_BAT);

	if (p_ptr->mode & MODE_FRUIT_BAT) p_ptr->fruit_bat = 1;
	p_ptr->dna = ((class & 0xff) | ((race & 0xff) << 8) );
	p_ptr->dna |= (randint(65535) << 16);
	p_ptr->male = sex & 1;
	p_ptr->pclass = class;
	p_ptr->align_good = 0x7fff;	/* start neutral */
	p_ptr->align_law = 0x7fff;
	p_ptr->prace = race;
	p_ptr->ptrait = trait;
#ifndef KURZEL_PK
	p_ptr->pkill = (PKILL_KILLABLE);
#else
	p_ptr->pkill = 0; //Flag default OFF
#endif

	s_printf("CHARACTER_CREATION: race=%s ; class=%s ; trait=%s ; mode=%u\n", race_info[p_ptr->prace].title, class_info[p_ptr->pclass].title, trait_info[p_ptr->ptrait].title, p_ptr->mode);

	/* Set pointers */
	p_ptr->rp_ptr = &race_info[race];
	p_ptr->cp_ptr = &class_info[class];
	p_ptr->tp_ptr = &trait_info[trait];

#ifdef RPG_SERVER /* Make characters always NO_GHOST */
	p_ptr->mode |= MODE_NO_GHOST;
	p_ptr->mode &= ~MODE_EVERLASTING;
	p_ptr->mode &= ~MODE_PVP;
#endif

	/* Set his ID */
	p_ptr->id = newid();

	/* Level one (never zero!) */
	p_ptr->lev = 1;

	/* Actually Generate */
	p_ptr->maximize = cfg.maximize ? TRUE : FALSE;

	/* No autoroller */
	if (!get_stats(Ind, stat_order)) return FALSE;

	/* Roll for base hitpoints */
	get_extra(Ind);

	/* Roll for age/height/weight */
	get_ahw(Ind);

	/* Roll for social class */
	get_history(Ind);

	/* Roll for gold */
	get_money(Ind);

	/* for PVP-mode chars: set level and skill, execute hacks */
	if (p_ptr->mode & MODE_PVP) {
		object_type forge, *o_ptr = &forge;

		p_ptr->lev = MIN_PVP_LEVEL;
#ifndef ALT_EXPRATIO
		p_ptr->exp = ((s64b)player_exp[p_ptr->lev - 2] * (s64b)p_ptr->expfact) / 100L;
#else
		p_ptr->exp = (s64b)player_exp[p_ptr->lev - 2];
#endif
		p_ptr->skill_points = (p_ptr->lev - 1) * SKILL_NB_BASE;
		p_ptr->au = 9950 + rand_int(101);

		/* give her/him a free mimic transformation for starting out */
		if ((p_ptr->pclass == CLASS_ADVENTURER) ||
		    (p_ptr->pclass == CLASS_MIMIC) ||
		    (p_ptr->pclass == CLASS_SHAMAN))
			p_ptr->free_mimic = 1;

		/* a good starter item since we're not going from level 1 */
#if 0
		give_reward(Ind, RESF_LOW2, "", 1, 0);
#else
		i = lookup_kind(TV_PARCHMENT, SV_DEED_PVP_START);
		invcopy(o_ptr, i);
		o_ptr->number = 1;
		object_aware(Ind, o_ptr);
		object_known(o_ptr);
		o_ptr->discount = 0;
		o_ptr->level = 0;
		o_ptr->ident |= ID_MENTAL;
		inven_carry(Ind, o_ptr);
#endif

		/* Give him sufficient CCW potions to save him from the hassle */
		i = lookup_kind(TV_POTION, SV_POTION_CURE_CRITICAL);
		invcopy(o_ptr, i);
		o_ptr->number = 10;
		object_aware(Ind, o_ptr);
		object_known(o_ptr);
		o_ptr->discount = 0;
		o_ptr->level = 0;
		o_ptr->ident |= ID_MENTAL;
		inven_carry(Ind, o_ptr);
	}

#ifdef EVENT_TOWNIE_GOLD_LIMIT
	if ((p_ptr->mode & MODE_DED_IDDC) && EVENT_TOWNIE_GOLD_LIMIT != -1) {
		p_ptr->au += EVENT_TOWNIE_GOLD_LIMIT;
		p_ptr->gold_picked_up = EVENT_TOWNIE_GOLD_LIMIT;
	}
#endif

#if 1 /* note that this allows use of WoR to get to IDDC :) */
	if ((p_ptr->mode & MODE_DED_IDDC)) {
		/* automatically know the location of IDDC dungeon */
		p_ptr->wild_map[(WPOS_IRONDEEPDIVE_X + WPOS_IRONDEEPDIVE_Y * MAX_WILD_X) / 8] |=
		    (1U << ((WPOS_IRONDEEPDIVE_X + WPOS_IRONDEEPDIVE_Y * MAX_WILD_X) % 8));
	}
#endif

	/* admin hacks */
	if (p_ptr->admin_wiz) {
		/* the admin wizard can basically do what he wants */

		/* use res_uni instead; it messes the unique list */
//		for (i = 1; i < MAX_R_IDX; i++) p_ptr->r_killed[i] = r_info[i].level;
	} else if (p_ptr->admin_dm) {
		p_ptr->invuln = -1;
		p_ptr->ghost = 1;
//		p_ptr->noscore = 1;
	}

	p_ptr->max_lev = p_ptr->max_plv = p_ptr->lev;
	p_ptr->max_exp = p_ptr->exp;

	/* Set his location, panel, etc. */
	player_setup(Ind, TRUE);

	/* Set up the skills */
//	p_ptr->skill_last_level = 1;	/* max_plv will do maybe..? */
	for (i = 0; i < MAX_SKILLS; i++)
		p_ptr->s_info[i].dev = FALSE;
	for (i = 0; i < MAX_SKILLS; i++) {
		s32b value = 0, mod = 0;

		/* Make sure all are touched */
		p_ptr->s_info[i].touched = TRUE;
		compute_skills(p_ptr, &value, &mod, i);

		init_skill(p_ptr, value, mod, i);

		/* Develop only revelant branches */
		if (p_ptr->s_info[i].value || p_ptr->s_info[i].mod) {
			int z = s_info[i].father;

			while (z != -1) {
				p_ptr->s_info[z].dev = TRUE;
				z = s_info[z].father;
				if (z == 0) break;
			}
		}
	}

#if 0
	/* Adjust birth trait based skills (only draconians currently) - Kurzel */
	switch (p_ptr->ptrait) {
		case TRAIT_BLUE: {
			do_trait_skill(Ind, SKILL_R_LITE, 105);
			do_trait_skill(Ind, SKILL_R_NETH, 105);
		break; }
		case TRAIT_WHITE: {
			do_trait_skill(Ind, SKILL_R_LITE,  97);
			do_trait_skill(Ind, SKILL_R_DARK, 108);
			do_trait_skill(Ind, SKILL_R_NETH, 108);
			do_trait_skill(Ind, SKILL_R_CHAO,  97);
		break; }
		case TRAIT_RED: {
			do_trait_skill(Ind, SKILL_R_LITE, 108);
			do_trait_skill(Ind, SKILL_R_DARK,  97);
			do_trait_skill(Ind, SKILL_R_NETH,  97);
			do_trait_skill(Ind, SKILL_R_CHAO, 108);
		break; }
		case TRAIT_BLACK: {
			do_trait_skill(Ind, SKILL_R_DARK, 105);
			do_trait_skill(Ind, SKILL_R_CHAO, 105);
		break; }
		case TRAIT_GREEN: {
			do_trait_skill(Ind, SKILL_R_DARK, 105);
			do_trait_skill(Ind, SKILL_R_MANA, 105);
		break; }
		case TRAIT_MULTI: {
			do_trait_skill(Ind, SKILL_R_LITE, 103);
			do_trait_skill(Ind, SKILL_R_DARK, 103);
			do_trait_skill(Ind, SKILL_R_NETH, 103);
			do_trait_skill(Ind, SKILL_R_CHAO, 103);
		break; }
		case TRAIT_BRONZE: {
			do_trait_skill(Ind, SKILL_R_LITE, 105);
			do_trait_skill(Ind, SKILL_R_DARK, 105);
		break; }
		case TRAIT_SILVER: {
			do_trait_skill(Ind, SKILL_R_LITE, 105);
			do_trait_skill(Ind, SKILL_R_NEXU, 105);
		break; }
		case TRAIT_GOLD: {
			do_trait_skill(Ind, SKILL_R_NEXU, 105);
			do_trait_skill(Ind, SKILL_R_CHAO, 105);
		break; }
		case TRAIT_LAW: {
			do_trait_skill(Ind, SKILL_R_NEXU, 105);
			do_trait_skill(Ind, SKILL_R_CHAO, 103);
			do_trait_skill(Ind, SKILL_R_MANA, 103);
		break; }
		case TRAIT_CHAOS: {
			do_trait_skill(Ind, SKILL_R_NETH, 103);
			do_trait_skill(Ind, SKILL_R_CHAO, 108);
		break; }
		case TRAIT_BALANCE: {
			do_trait_skill(Ind, SKILL_R_NEXU, 103);
			do_trait_skill(Ind, SKILL_R_NETH, 103);
			do_trait_skill(Ind, SKILL_R_CHAO, 103);
			do_trait_skill(Ind, SKILL_R_MANA, 103);
		break; }
		default:
		break;
	}
#endif

	/* Bards receive really random skills */
#if 0
	if (p_ptr->pclass == CLASS_BARD) {
		do_bard_skill(Ind);
	}
#endif

	/* Fruit bats get some skills nulled that they cannot put to use anyway,
	   just to clean up and avoid newbies accidentally putting points into them. */
	if (p_ptr->mode & MODE_FRUIT_BAT) fruit_bat_skills(p_ptr);

	/* special outfits for admin (pack overflows!) */
	if (is_admin(p_ptr)) {
		admin_outfit(Ind, 0);
		p_ptr->au = 50000000;
		p_ptr->lev = 99;
		p_ptr->exp = 999999999;
		p_ptr->skill_points = 9999;
//		p_ptr->noscore = 1;
		/* permanent invulnerability */
		p_ptr->total_winner = TRUE;
		p_ptr->once_winner = TRUE; //just for consistency, eg when picking up WINNERS_ONLY items
		p_ptr->max_dlv = 200;
#ifdef SEPARATE_RECALL_DEPTHS
		for (i = 0; i < MAX_D_IDX * 2; i++)
			p_ptr->max_depth[i] = 200;
#endif
	}
	/* Hack -- outfit the player */
	else player_outfit(Ind);

	/* Give the player some resurrections */
	p_ptr->lives = cfg.lifes + 1;

	/* msp is only calculated in calc_mana which is only in update_stuff though.. */
	calc_mana(Ind);
	p_ptr->csp = p_ptr->msp;

	/* Start with full stamina */
	p_ptr->cst = 10;
	p_ptr->mst = 10;

	/* start with dual-wield mode 'dual-handed' */
	p_ptr->dual_mode = TRUE;

	/* disabled flash_self by default */
	p_ptr->flash_self = -1;

	/* hack: allow to get extra level feeling immediately */
	p_ptr->turns_on_floor = TURNS_FOR_EXTRA_FEELING;

	/* hack: if he's in town, get him pseudo tree-passing */
	if (istown(&p_ptr->wpos) || isdungeontown(&p_ptr->wpos)) p_ptr->town_pass_trees = TRUE;

	/* HACK - avoid misleading 'updated' messages and routines - C. Blue
	   (Can be used for different purpose, usually in conjuction with custom.lua) */
	/* An artifact reset can be done by changing just custom.lua. */
	/* Set updated_savegame_birth via lua (server_startup()). */
	p_ptr->updated_savegame = updated_savegame_birth;

	/* for automatic artifact reset */
	p_ptr->artifact_reset = ABS(artifact_reset);
	p_ptr->fluent_artifact_reset = FALSE;

	/* Prepare newbie-aiding warnings that ought to occur only
	   once (not necessarily implemented like that atm) - C. Blue */
	p_ptr->newbie_hints = TRUE;
	disable_specific_warnings(p_ptr);

	/* No active quests */
	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++) {
		p_ptr->quest_codename[i][0] = 0;
		p_ptr->quest_idx[i] = -1;
	}

	/* To find out which characters crash the server */
	s_printf("Logged in with character %s.\n", name);

	/* for "warning_pvp" */
	p_ptr->newly_created = TRUE;

	/* Success */
	return TRUE;
}

/* Disallow non-authorized admin (improvement needed!!) */
/* returns FALSE if bogus admin - Jir - */
bool confirm_admin(int Ind)
{
	struct account acc;
	player_type *p_ptr = Players[Ind];
	bool admin = FALSE;

	if (!GetAccountID(&acc, p_ptr->account, FALSE)) return(FALSE);
	if (acc.flags & ACC_ADMIN) admin = TRUE;
	/* sucks, but allows an admin wizard. i'll change - evileye */
	/* one DM is enough - jir :) */
//	if (!strcmp(p_ptr->name, acc.name)) p_ptr->admin_wiz = admin;
	if (strcmp(p_ptr->name, acc.name)) p_ptr->admin_wiz = admin;
	else p_ptr->admin_dm = admin;
	return(admin);
}


/*
 * We are starting a "brand new" server.  We need to initialze the unique
 * info, so that they will be created.  This function is only called if the
 * server state savefile could not be loaded.
 */
void server_birth(void) {
	int i;

	/* Initialize uniques */
	for (i = 0; i < MAX_R_IDX; i++) {
		/* Make sure we have a unique */
		if (!(r_info[i].flags1 & RF1_UNIQUE))
			continue;

		/* Set his maximum creation number */
		r_info[i].max_num = 1;
	}

	/* Set party zero's name to "Neutral" */
	strcpy(parties[0].name, "Neutral");

	/* First player's ID should be 1 */
	account_id = 1;
	player_id = 1;
}

