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
struct birther
{
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
struct hist_type
{
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
static hist_type bg[] =
{
	{"You are the illegitimate and unacknowledged child ",	10, 1, 2, 25},
	{"You are the illegitimate but acknowledged child ",	20, 1, 2, 35},
	{"You are one of several children ",			95, 1, 2, 45},
	{"You are the first child ",				100, 1, 2, 50},

	{"of a Serf.  ",					40, 2, 3, 65},
	{"of a Yeoman.  ",					65, 2, 3, 80},
	{"of a Townsman.  ",					80, 2, 3, 90},
	{"of a Guildsman.  ",					90, 2, 3, 105},
	{"of a Landed Knight.  ",				96, 2, 3, 120},
	{"of a Titled Noble.  ",				99, 2, 3, 130},
	{"of a Royal Blood Line.  ",				100, 2, 3, 140},

	{"You are the black sheep of the family.  ",		20, 3, 50, 20},
	{"You are a credit to the family.  ",			80, 3, 50, 55},
	{"You are a well liked child.  ",			100, 3, 50, 60},

	{"Your mother was of the Teleri.  ",			40, 4, 1, 50},
	{"Your father was of the Teleri.  ",			75, 4, 1, 55},
	{"Your mother was of the Noldor.  ",			90, 4, 1, 55},
	{"Your father was of the Noldor.  ",			95, 4, 1, 60},
	{"Your mother was of the Vanyar.  ",			98, 4, 1, 65},
	{"Your father was of the Vanyar.  ",			100, 4, 1, 70},

	{"You are one of several children ",			60, 7, 8, 50},
	{"You are the only child ",				100, 7, 8, 55},

	{"of a Teleri ",					75, 8, 9, 50},
	{"of a Noldor ",					95, 8, 9, 55},
	{"of a Vanyar ",					100, 8, 9, 60},

	{"Ranger.  ",						40, 9, 54, 80},
	{"Archer.  ",						70, 9, 54, 90},
	{"Warrior.  ",						87, 9, 54, 110},
	{"Mage.  ",						95, 9, 54, 125},
	{"Prince.  ",						99, 9, 54, 140},
	{"King.  ",						100, 9, 54, 145},

	{"You are one of several children of a Hobbit ",                 85, 10, 11, 45},
	{"You are the only child of a Hobbit ",                 100, 10, 11, 55},

	{"Bum.  ",                                                       20, 11, 3, 55},
	{"Tavern Owner.  ",                                              30, 11, 3, 80},
	{"Miller.  ",                                            40, 11, 3, 90},
	{"Home Owner.  ",                                                50, 11, 3, 100},
	{"Burglar.  ",                                           80, 11, 3, 110},
	{"Warrior.  ",                                           95, 11, 3, 115},
	{"Mage.  ",                                                      99, 11, 3, 125},
	{"Clan Elder.  ",                                               100, 11, 3, 140},

	{"You are one of several children of a Gnome ",          85, 13, 14, 45},
	{"You are the only child of a Gnome ",                  100, 13, 14, 55},

	{"Beggar.  ",                                            20, 14, 3, 55},
	{"Braggart.  ",                                          50, 14, 3, 70},
	{"Prankster.  ",                                                 75, 14, 3, 85},
	{"Warrior.  ",                                           95, 14, 3, 100},
	{"Mage.  ",                                                     100, 14, 3, 125},

	{"You are one of two children of a Dwarven ",            25, 16, 17, 40},
	{"You are the only child of a Dwarven ",                        100, 16, 17, 50},

	{"Thief.  ",                                             10, 17, 18, 60},
	{"Prison Guard.  ",                                              25, 17, 18, 75},
	{"Miner.  ",                                             75, 17, 18, 90},
	{"Warrior.  ",                                           90, 17, 18, 110},
	{"Priest.  ",                                            99, 17, 18, 130},
	{"King.  ",                                                     100, 17, 18, 150},

	{"You are the black sheep of the family.  ",             15, 18, 57, 10},
	{"You are a credit to the family.  ",                    85, 18, 57, 50},
	{"You are a well liked child.  ",                               100, 18, 57, 55},

	{"Your mother was an Orc, but it is unacknowledged.  ",  25, 19, 20, 25},
	{"Your father was an Orc, but it is unacknowledged.  ", 100, 19, 20, 25},

	{"You are the adopted child ",                          100, 20, 2, 50},

	{"Your mother was a Cave-Troll ",                                30, 22, 23, 20},
	{"Your father was a Cave-Troll ",                                60, 22, 23, 25},
	{"Your mother was a Hill-Troll ",                                75, 22, 23, 30},
	{"Your father was a Hill-Troll ",                                90, 22, 23, 35},
	{"Your mother was a Water-Troll ",                               95, 22, 23, 40},
	{"Your father was a Water-Troll ",                              100, 22, 23, 45},

	{"Cook.  ",                                                       5, 23, 62, 60},
	{"Warrior.  ",                                           95, 23, 62, 55},
	{"Shaman.  ",                                            99, 23, 62, 65},
	{"Clan Chief.  ",                                               100, 23, 62, 80},

	{"You have dark brown eyes, ",                           20, 50, 51, 50},
	{"You have brown eyes, ",                                        60, 50, 51, 50},
	{"You have hazel eyes, ",                                        70, 50, 51, 50},
	{"You have green eyes, ",                                        80, 50, 51, 50},
	{"You have blue eyes, ",                                         90, 50, 51, 50},
	{"You have blue-gray eyes, ",                           100, 50, 51, 50},

	{"straight ",                                            70, 51, 52, 50},
	{"wavy ",                                                        90, 51, 52, 50},
	{"curly ",                                                      100, 51, 52, 50},

	{"black hair, ",                                                 30, 52, 53, 50},
	{"brown hair, ",                                                 70, 52, 53, 50},
	{"auburn hair, ",                                                80, 52, 53, 50},
	{"red hair, ",                                           90, 52, 53, 50},
	{"blond hair, ",                                                100, 52, 53, 50},

	{"and a very dark complexion.",                          10, 53, 0, 50},
	{"and a dark complexion.",                                       30, 53, 0, 50},
	{"and an average complexion.",                           80, 53, 0, 50},
	{"and a fair complexion.",                                       90, 53, 0, 50},
	{"and a very fair complexion.",                         100, 53, 0, 50},

	{"You have light grey eyes, ",                           85, 54, 55, 50},
	{"You have light blue eyes, ",                           95, 54, 55, 50},
	{"You have light green eyes, ",                         100, 54, 55, 50},

	{"straight ",                                            75, 55, 56, 50},
	{"wavy ",                                                       100, 55, 56, 50},

	{"black hair, and a fair complexion.",                   75, 56, 0, 50},
	{"brown hair, and a fair complexion.",                   85, 56, 0, 50},
	{"blond hair, and a fair complexion.",                   95, 56, 0, 50},
	{"silver hair, and a fair complexion.",                 100, 56, 0, 50},

	{"You have dark brown eyes, ",                           99, 57, 58, 50},
	{"You have glowing red eyes, ",                         100, 57, 58, 60},

	{"straight ",                                            90, 58, 59, 50},
	{"wavy ",                                                       100, 58, 59, 50},

	{"black hair, ",                                                 75, 59, 60, 50},
	{"brown hair, ",                                                100, 59, 60, 50},

	{"a one foot beard, ",                                   25, 60, 61, 50},
	{"a two foot beard, ",                                   60, 60, 61, 51},
	{"a three foot beard, ",                                         90, 60, 61, 53},
	{"a four foot beard, ",                                 100, 60, 61, 55},

	{"and a dark complexion.",                                      100, 61, 0, 50},

	{"You have slime green eyes, ",                          60, 62, 63, 50},
	{"You have puke yellow eyes, ",                          85, 62, 63, 50},
	{"You have blue-bloodshot eyes, ",                               99, 62, 63, 50},
	{"You have glowing red eyes, ",                         100, 62, 63, 55},

	{"dirty ",                                                       33, 63, 64, 50},
	{"mangy ",                                                       66, 63, 64, 50},
	{"oily ",                                                       100, 63, 64, 50},

	{"sea-weed green hair, ",                                        33, 64, 65, 50},
	{"bright red hair, ",                                    66, 64, 65, 50},
	{"dark purple hair, ",                                  100, 64, 65, 50},

	{"and green ",                                           25, 65, 66, 50},
	{"and blue ",                                            50, 65, 66, 50},
	{"and white ",                                           75, 65, 66, 50},
	{"and black ",                                          100, 65, 66, 50},

	{"ulcerous skin.",                                               33, 66, 0, 50},
	{"scabby skin.",                                                 66, 66, 0, 50},
	{"leprous skin.",                                               100, 66, 0, 50},


	{"You are one of several children of a DragonRider. ", 85, 89, 91, 50  },
	{"You are the only child of a DragonRider. ", 100, 89, 91, 60 },

	{"You have a Green Dragon.", 30, 91, 0, 40 },
	{"You have a Blue Dragon.", 55, 91, 0, 60 },
	{"You have a Brown Dragon.", 80, 91, 0, 80 },
	{"You have a Bronze Dragon.", 90, 91, 0, 100 },
	{"You have a Gold Dragon.", 100, 91, 0, 120},

	{"You are of an unknown generation of the Ents.",        30, 94, 95, 30},
	{"You are of the third generation of the Ents.",         40, 94, 95, 50},
	{"You are of the second generation of the Ents.",        60, 94, 95, 60},
	{"You are of the first beings who awoke on Arda.",      100, 94, 95, 80},

	{"You have green skin and unflexible members.",          50, 95, 0, 50},
	{"You have brown skin and unflexible members.",         100, 95, 0, 50},
};



/*
 * Current stats
 */
static s16b             stat_use[6];



/*
 * Returns adjusted stat -JK-
 * Algorithm by -JWT-
 *
 * auto_roll is boolean and states maximum changes should be used rather
 * than random ones to allow specification of higher values to wait for
 *
 * The "p_ptr->maximize" code is important      -BEN-
 */
static int adjust_stat(int Ind, int value, s16b amount, int auto_roll)
{
	player_type *p_ptr = Players[Ind];
	int i;

	/* Negative amounts */
	if (amount < 0)
	{
		/* Apply penalty */
		for (i = 0; i < (0 - amount); i++)
		{
			if (value >= 18+10)
			{
				value -= 10;
			}
			else if (value > 18)
			{
				value = 18;
			}
			else if (value > 3)
			{
				value--;
			}
		}
	}

	/* Positive amounts */
	else if (amount > 0)
	{
		/* Apply reward */
		for (i = 0; i < amount; i++)
		{
			if (value < 18)
			{
				value++;
			}
			else if (p_ptr->maximize)
			{
				value += 10;
			}
			else if (value < 18+70)
			{
				value += ((auto_roll ? 15 : randint(15)) + 5);
			}
			else if (value < 18+90)
			{
				value += ((auto_roll ? 6 : randint(6)) + 2);
			}
			else if (value < 18+100)
			{
				value++;
			}
		}
	}

	/* Return the result */
	return (value);
}




/*
 * Roll for a characters stats
 *
 * For efficiency, we include a chunk of "calc_bonuses()".
 */
static void get_stats(int Ind, int stat_order[6])
{
	player_type *p_ptr = Players[Ind];
	int             i, j, tries = 1000;

	int             bonus;

	int             dice[18];
	int             stats[6];

	/* Clear "stats" array */
	for (i = 0; i < 6; i++)
		stats[i] = 0;

	/* Check over the given stat order, to prevent cheating */
	for (i = 0; i < 6; i++)
	{
		/* Check range */
		if (stat_order[i] < 0 || stat_order[i] > 5)
		{
			stat_order[i] = 1;
		}

		/* Check for duplicated entries */
		if (stats[stat_order[i]] == 1)
		{
			/* Find a stat that hasn't been specified yet */
			for (j = 0; j < 6; j++)
			{
				if (stats[j])
					continue;

				stat_order[i] = j;
			}
		}

		/* Set flag */
		stats[stat_order[i]] = 1;
	}

	/* Roll and verify some stats */
	while (--tries)
	{
		/* Roll some dice */
		for (j = i = 0; i < 18; i++)
		{
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
	for (i = 0; i < 6; i++)
	{
		/* Extract 5 + 1d3 + 1d4 + 1d5 */
		j = 5 + dice[3*i] + dice[3*i+1] + dice[3*i+2];

		/* Save that value */
		stats[i] = j;
	}

	/* Now sort the stats */
	/* I use a bubble sort because I'm lazy at the moment */
	for (i = 0; i < 6; i++)
	{
		for (j = 0; j < 5; j++)
		{
			if (stats[j] < stats[j + 1])
			{
				int t;

				t = stats[j];
				stats[j] = stats[j + 1];
				stats[j + 1] = t;
			}
		}
	}

	/* Now, put them in the correct order */
	for (i = 0; i < 6; i++)
	{
		p_ptr->stat_max[stat_order[i]] = stats[i];
	}

	/* Adjust the stats */
	for (i = 0; i < 6; i++)
	{
		/* Obtain a "bonus" for "race" and "class" */
		bonus = p_ptr->rp_ptr->r_adj[i] + p_ptr->cp_ptr->c_adj[i];

		/* Variable stat maxes */
		if (p_ptr->maximize)
		{
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
		else
		{
			/* Apply the bonus to the stat (somewhat randomly) */
			stat_use[i] = adjust_stat(Ind, p_ptr->stat_max[i], bonus, FALSE);

			/* Save the resulting stat maximum */
			p_ptr->stat_cur[i] = p_ptr->stat_max[i] = stat_use[i];
		}
	}
}


/*
 * Roll for some info that the auto-roller ignores
 */
static void get_extra(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int             i, j, min_value, max_value;
	int tries = 300;

	/* Level one (never zero!) */
	p_ptr->lev = 1;

	/* Experience factor */
	p_ptr->expfact = p_ptr->rp_ptr->r_exp + p_ptr->cp_ptr->c_exp;

	/* Hitdice */
	p_ptr->hitdie = p_ptr->rp_ptr->r_mhp + p_ptr->cp_ptr->c_mhp;

	/* Assume base hitpoints (fully healed) */
	p_ptr->chp = p_ptr->mhp = p_ptr->hitdie;
	p_ptr->csane = p_ptr->msane =
		((int)(adj_con_mhp[p_ptr->stat_ind[A_WIS]]) - 128) / 2 + 10;

	/* Minimum hitpoints at highest level */
	min_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 3) / 8;
	min_value += PY_MAX_LEVEL;

	/* Maximum hitpoints at highest level */
	max_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 5) / 8;
	max_value += PY_MAX_LEVEL;

	/* Pre-calculate level 1 hitdice */
	p_ptr->player_hp[0] = p_ptr->hitdie;

	/* Roll out the hitpoints */
	while (--tries)
	{
		/* Roll the hitpoint values */
		for (i = 1; i < PY_MAX_LEVEL; i++)
		{
			j = randint(p_ptr->hitdie);
			p_ptr->player_hp[i] = p_ptr->player_hp[i-1] + j;
		}

		/* XXX Could also require acceptable "mid-level" hitpoints */

		/* Require "valid" hitpoints at highest level */
		if (p_ptr->player_hp[PY_MAX_LEVEL-1] < min_value) continue;
		if (p_ptr->player_hp[PY_MAX_LEVEL-1] > max_value) continue;

		/* Acceptable */
		break;
	}
}


/*
 * Get the racial history, and social class, using the "history charts".
 */
static void get_history(int Ind)
{
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
	switch (p_ptr->prace)
	{
		case RACE_HUMAN:
		case RACE_DUNADAN:
		case RACE_YEEK:
		{
			chart = 1;
			break;
		}

		case RACE_HALF_ELF:
		{
			chart = 4;
			break;
		}

		case RACE_ELF:
		case RACE_HIGH_ELF:
		{
			chart = 7;
			break;
		}

		case RACE_HOBBIT:
		{
			chart = 10;
			break;
		}

		case RACE_GNOME:
		{
			chart = 13;
			break;
		}

		case RACE_DWARF:
		{
			chart = 16;
			break;
		}

		case RACE_HALF_ORC:
		case RACE_GOBLIN:
		{
			chart = 19;
			break;
		}

		case RACE_HALF_TROLL:
		{
			chart = 22;
			break;
		}

		case RACE_ENT:
		{
			chart = 94;
			break;
		}

		case RACE_DRIDER:
		{
			chart = 89;
			break;
		}

		default:
		{
			chart = 0;
			break;
		}
	}


	/* Process the history */
	while (chart)
	{
		/* Start over */
		i = 0;

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
	while (--tries)
	{
		/* Extract remaining length */
		n = strlen(s);

		/* All done */
		if (n < 60)
		{
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
static void get_ahw(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Calculate the age */
	p_ptr->age = p_ptr->rp_ptr->b_age + randint(p_ptr->rp_ptr->m_age);

	/* Calculate the height/weight for males */
	if (p_ptr->male)
	{
		p_ptr->ht = randnor(p_ptr->rp_ptr->m_b_ht, p_ptr->rp_ptr->m_m_ht);
		p_ptr->wt = randnor(p_ptr->rp_ptr->m_b_wt, p_ptr->rp_ptr->m_m_wt);
	}

	/* Calculate the height/weight for females */
	else
	{
		p_ptr->ht = randnor(p_ptr->rp_ptr->f_b_ht, p_ptr->rp_ptr->f_m_ht);
		p_ptr->wt = randnor(p_ptr->rp_ptr->f_b_wt, p_ptr->rp_ptr->f_m_wt);
	}
}




/*
 * Get the player's starting money
 */
static void get_money(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int        i, gold;

	/* Social Class determines starting gold */
	gold = (p_ptr->sc * 6) + randint(100) + 300;

	/* Process the stats */
	for (i = 0; i < 6; i++)
	{
		/* Mega-Hack -- reduce gold for high stats */
		if (stat_use[i] >= 18+50) gold -= 300;
		else if (stat_use[i] >= 18+20) gold -= 200;
		else if (stat_use[i] > 18) gold -= 150;
		else gold -= (stat_use[i] - 8) * 10;
	}

	/* Minimum 100 gold */
	if (gold < 100) gold = 100;

	/* She charmed the banker into it! -CJS- */
	/* She slept with the banker.. :) -GDH-  */
	if (!p_ptr->male) gold += 50;

	/* Save the gold */
	p_ptr->au = gold;

	/* Since it's not a king/queen */
	p_ptr->own1.wx=p_ptr->own1.wy=p_ptr->own1.wz=0;
	wpcopy(&p_ptr->own2, &p_ptr->own1);
		
	if (p_ptr->admin_wiz)
	{
		/* the admin wizard can basically do what he wants */
		p_ptr->au = 50000000;
		p_ptr->lev = 100;
		p_ptr->exp = 999999999;
		p_ptr->noscore = 1;
		/* permanent invulnerability */
#if 0
		//p_ptr->invuln = -1;
		
		// This is a KING(or QUEEN :)
#endif
		p_ptr->total_winner = TRUE;
		p_ptr->max_dlv = 200;
		p_ptr->max_plv = 99;

		/* use res_uni instead; it messes the unique list */
//		for (i = 1; i < MAX_R_IDX; i++) p_ptr->r_killed[i] = r_info[i].level;
	}
	else if (p_ptr->admin_dm)
	{
		p_ptr->au = 50000000;
		p_ptr->lev = 100;
		p_ptr->exp = 999999999;
		p_ptr->invuln = -1;
		p_ptr->ghost = 1;
		p_ptr->noscore = 1;
		p_ptr->max_dlv = 200;
		p_ptr->max_plv = 99;
	}
	
}



/*
 * Clear all the global "character" data
 */
static void player_wipe(int Ind)
{
	player_type *p_ptr = Players[Ind];
	object_type *old_inven;
	int i, j;


	/* Hack -- save the inventory pointer */
	old_inven = p_ptr->inventory;

	/* Hack -- zero the struct */
	WIPE(p_ptr, player_type);

	/* Hack -- reset the inventory pointer */
	p_ptr->inventory = old_inven;

	/* Wipe the history */
	for (i = 0; i < 4; i++)
	{
		strcpy(p_ptr->history[i], "");
	}

	/* No weight */
	p_ptr->total_weight = 0;

	/* No items */
	p_ptr->inven_cnt = 0;
	p_ptr->equip_cnt = 0;

	/* Clear the inventory */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		invwipe(&p_ptr->inventory[i]);
	}


	/* Hack -- Well fed player */
	p_ptr->food = PY_FOOD_FULL - 1;

	/* Assume no winning game */
	p_ptr->total_winner = FALSE;

	/* Assume no panic save */
	panic_save = 0;

	/* Assume no cheating */
	p_ptr->noscore = 0;
	
	/* clear the wilderness map */
	for (i = 0; i < MAX_WILD_8; i++)
		p_ptr->wild_map[i] = 0;

	/* Hack -- assume the player has an initial knowledge of the area close to town */
	p_ptr->wild_map[(cfg.town_x+cfg.town_y*MAX_WILD_X)/8]|=1<<((cfg.town_x+cfg.town_y*MAX_WILD_X)%8);

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

static byte player_init[MAX_CLASS][3][2] =
{
	{
		/* Adventurer */
//		{ TV_RING, SV_RING_SEE_INVIS },
		{ TV_BOOK, 50 },
		{ TV_SWORD, SV_SABRE },
		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR }
	},

	{
		/* Warrior */
		{ TV_POTION, SV_POTION_BESERK_STRENGTH },
		{ TV_SWORD, SV_BROAD_SWORD },
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL }
	},

	{
		/* Mage */
		{ TV_MAGIC_BOOK, 0 },
		{ TV_SWORD, SV_DAGGER },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL }
	},

	{
		/* Priest */
		{ TV_PRAYER_BOOK, 0 },
		{ TV_HAFTED, SV_MACE },
		{ TV_POTION, SV_POTION_HEALING }
	},

	{
		/* Rogue */
#if 0
		{ TV_SHADOW_BOOK, 0 },	// this will be changed soon
		{ TV_SWORD, SV_SMALL_SWORD },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR }
#else	// 0
		{ TV_SHADOW_BOOK, 0 },	// this will be changed soon
		{ TV_SWORD, SV_MAIN_GAUCHE },
		{ TV_TRAPKIT, SV_TRAPKIT_SLING }
#endif	// 0
	},

	{
		/* Mimic */
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SWORD, SV_LONG_SWORD },
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL }
	},

	{
		/* Archer */
		{ TV_HUNT_BOOK, 0 },
		{ TV_ARROW, SV_AMMO_MAGIC },
		{ TV_BOW, SV_LONG_BOW }
	},

	{
		/* Bard */
		{ TV_RING, SV_RING_SEE_INVIS },
		{ TV_SWORD, SV_SABRE },
		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR }
	},

};


/*
 * Give the cfg_admin_wizard some interesting stuff.
 */
void admin_outfit(int Ind, int realm)
{
	player_type *p_ptr = Players[Ind];
	int             i, tv, sv;

	object_type     forge;
	object_type     *o_ptr = &forge;

	int note = quark_add("!k");

	if (!is_admin(p_ptr)) return;


	/* Hack -- assume the player has an initial knowledge of the area close to town */
	for (i = 0; i < MAX_WILD_X*MAX_WILD_Y; i++)  p_ptr->wild_map[i/8] |= 1<<(i%8);

	invcopy(o_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_STAR_IDENTIFY));
	o_ptr->number = 99;
	o_ptr->discount = 100;
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	o_ptr->note = quark_add("@r8");
	object_known(o_ptr);
	object_aware(Ind, o_ptr);
	(void)inven_carry(Ind, o_ptr);
#if 1
	invcopy(o_ptr, lookup_kind(TV_POTION, SV_POTION_EXPERIENCE));
	o_ptr->number = 99;
	o_ptr->discount = 100;
	object_known(o_ptr);
	object_aware(Ind, o_ptr);
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	(void)inven_carry(Ind, o_ptr);
#endif
	invcopy(o_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_ARTIFACT_CREATION));
	o_ptr->number = 99;
	o_ptr->discount = 100;
	object_known(o_ptr);
	object_aware(Ind, o_ptr);
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	o_ptr->note = note;
	(void)inven_carry(Ind, o_ptr);
#if 0
	invcopy(o_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_WORD_OF_RECALL));
	o_ptr->number = 98;
	o_ptr->discount = 100;
	object_known(o_ptr);
	object_aware(Ind, o_ptr);
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	//		o_ptr->note = quark_add("@R-3000");
	(void)inven_carry(Ind, o_ptr);
#endif
	invcopy(o_ptr, lookup_kind(TV_POTION, SV_POTION_AUGMENTATION));
	o_ptr->number = 99;
	o_ptr->discount = 100;
	object_known(o_ptr);
	object_aware(Ind, o_ptr);
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	(void)inven_carry(Ind, o_ptr);

	invcopy(o_ptr, lookup_kind(TV_POTION2, SV_POTION2_LEARNING));
	o_ptr->number = 99;
	o_ptr->discount = 100;
	object_known(o_ptr);
	object_aware(Ind, o_ptr);
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	(void)inven_carry(Ind, o_ptr);

	invcopy(o_ptr, lookup_kind(TV_AMULET, SV_AMULET_LIFE));
	o_ptr->number = 30;
	o_ptr->discount = 0;
	o_ptr->pval = 10;
	object_known(o_ptr);
	object_aware(Ind, o_ptr);
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	o_ptr->note = note;
	(void)inven_carry(Ind, o_ptr);
#if 0
	invcopy(o_ptr, lookup_kind(TV_STAFF, SV_STAFF_PROBING));
	o_ptr->number = 1;
	o_ptr->pval = 30000;
	o_ptr->discount = 0;
	object_known(o_ptr);
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	(void)inven_carry(Ind, o_ptr);

	invcopy(o_ptr, lookup_kind(TV_FOOD, SV_FOOD_FORTUNE_COOKIE));
	o_ptr->number = 99;
	o_ptr->discount = 0;
	object_known(o_ptr);
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	(void)inven_carry(Ind, o_ptr);
#endif	// 0

	invcopy(o_ptr, lookup_kind(TV_ARROW, SV_AMMO_MAGIC));
	o_ptr->note = quark_add("@f1");
	apply_magic_depth(1, o_ptr, -1, FALSE, FALSE, FALSE);
	o_ptr->number = 98;
	o_ptr->discount = 0;
	object_known(o_ptr);
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	(void)inven_carry(Ind, o_ptr);

#if 0
	invcopy(o_ptr, lookup_kind(TV_POTION, SV_POTION_INVIS));
	o_ptr->number = 9;
	o_ptr->discount = 100;
	object_known(o_ptr);
	object_aware(Ind, o_ptr);
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	(void)inven_carry(Ind, o_ptr);

	/* Ifrit bug report */
	invcopy(o_ptr, lookup_kind(TV_SWORD, SV_BROAD_SWORD));
	o_ptr->name1 = ART_GLAMDRING;
	apply_magic(1, o_ptr, -1, TRUE, TRUE, TRUE);
	o_ptr->number = 1;
	o_ptr->discount = 0;
	object_known(o_ptr);
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	(void)inven_carry(Ind, o_ptr);

	invcopy(o_ptr, lookup_kind(TV_GLOVES, SV_SET_OF_CESTI));
	o_ptr->name1 = ART_FINGOLFIN;
	apply_magic_depth(1, o_ptr, -1, TRUE, TRUE, TRUE);
	o_ptr->number = 1;
	o_ptr->discount = 0;
	object_known(o_ptr);
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	(void)inven_carry(Ind, o_ptr);
#endif

	invcopy(o_ptr, lookup_kind(TV_HAFTED, SV_GROND));
	o_ptr->name1 = ART_GROND;
	apply_magic_depth(1, o_ptr, -1, TRUE, TRUE, TRUE);
	o_ptr->number = 1;
	o_ptr->discount = 0;
	object_known(o_ptr);
	o_ptr->owner = p_ptr->id;
	o_ptr->level = 1;
	(void)inven_carry(Ind, o_ptr);

	/* gimme books :) */
	if (0 <= realm && realm < MAX_REALM)
	{
#if 0
		char c[] = "@m ";
		/* soon this will be obsolete tho */
		c[1] = ((realm == REALM_PRAYER)? 'p':'m');
		if (realm == REALM_FIGHTING) c[1] = 'n';
#endif	// 0
		for (i = 0; i < 9; i++)
		{
			int k = lookup_kind(magic_info[realm].spell_book, i);
			u32b f1, f2, f3, f4, f5, esp;
			if (!k) continue;
			invcopy(o_ptr, k);

			/* Hack^2 -- use this as a sign of being 'high books' */
			object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);
			o_ptr->number = (f3 & TR3_IGNORE_FIRE) ? 1 : 30;

#if 0
			c[2] = i +1 +48;
			o_ptr->note = quark_add(c);
#endif	// 0

			o_ptr->discount = 72;
			object_known(o_ptr);
			o_ptr->owner = p_ptr->id;
			o_ptr->level = 1;
			(void)inven_carry(Ind, o_ptr);
		}
	}
}

#define do_player_outfit()	\
	object_aware(Ind, o_ptr); \
	object_known(o_ptr); \
	o_ptr->ident |= ID_MENTAL; \
	o_ptr->owner = p_ptr->id; \
	o_ptr->level = 0; \
	(void)inven_carry(Ind, o_ptr);

/*
 * Init players with some belongings
 *
 * Having an item makes the player "aware" of its purpose.
 */
static void player_outfit(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int             i, tv, sv;

	object_type     forge;

	object_type     *o_ptr = &forge;


	/* Hack -- Give the player some food */
	if (p_ptr->prace == RACE_ENT)
	{
		invcopy(o_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_SATISFY_HUNGER));
		o_ptr->number = rand_range(3, 7);
	}
	/* XXX problem is that Lembas sell dear.. */
	else if (p_ptr->prace == RACE_HALF_ELF ||
			p_ptr->prace == RACE_ELF || p_ptr->prace == RACE_HIGH_ELF)
	{
		invcopy(o_ptr, lookup_kind(TV_FOOD, SV_FOOD_WAYBREAD));
		o_ptr->number = rand_range(2, 4);
	}
	else
	{
		invcopy(o_ptr, lookup_kind(TV_FOOD, SV_FOOD_RATION));
		o_ptr->number = rand_range(2, 4);
	}
	do_player_outfit();

	/* Hack -- Give the player some torches */
	invcopy(o_ptr, lookup_kind(TV_LITE, SV_LITE_TORCH));
	o_ptr->number = rand_range(3, 7);
	o_ptr->timeout = rand_range(3, 7) * 500;
	do_player_outfit();

	if (!strcmp(p_ptr->name, "Moltor"))
	{
		invcopy(o_ptr, lookup_kind(TV_FOOD, SV_FOOD_PINT_OF_ALE));
		o_ptr->name2 = 188;	// Bud ;)
		o_ptr->number = 9;
		apply_magic_depth(1, o_ptr, -1, TRUE, TRUE, TRUE);
		o_ptr->discount = 72;
		o_ptr->owner = p_ptr->id;
		o_ptr->level = 1;
		object_known(o_ptr);
		object_aware(Ind, o_ptr);
		o_ptr->ident |= ID_MENTAL;
		(void)inven_carry(Ind, o_ptr);
	}

	//admin_outfit(Ind);

	/* Hack -- Give the player three useful objects */
	for (i = 0; i < 3; i++)
	{
		tv = player_init[p_ptr->pclass][i][0];
		sv = player_init[p_ptr->pclass][i][1];
		invcopy(o_ptr, lookup_kind(tv, sv));
		do_player_outfit();
	}
	
	if (p_ptr->prace == RACE_DRIDER)
	{
		invcopy(o_ptr, lookup_kind(TV_FIRESTONE, SV_FIRE_SMALL));
		o_ptr->number = rand_range(3, 7);
		do_player_outfit();
	}

	/* Hack -- Give the player newbie guide Parchment */
	invcopy(o_ptr, lookup_kind(TV_PARCHEMENT, SV_PARCHMENT_NEWBIE));
	do_player_outfit();
}

void player_create_tmpfile(int Ind)
{
	player_type *p_ptr = Players[Ind];

	FILE *fff;

	char file_name[MAX_PATH_LENGTH];

	/* Temporary file */
	if (path_temp(file_name, MAX_PATH_LENGTH))
	{
		s_printf("failed to generate tmpfile for %s!\n", p_ptr->name);
		return;
	}

	strcpy(p_ptr->infofile, file_name);
}

static void player_setup(int Ind, bool new)
{
	player_type *p_ptr = Players[Ind];
	int y, x, i, d, count = 0;
	cave_type *c_ptr;
			dun_level *l_ptr;

//	bool dawn = ((turn % (10L * TOWN_DAWN)) < (10L * TOWN_DAWN / 2)), unstaticed = 0; 
//	bool dawn = IS_DAY, unstaticed = 0; 
	bool unstaticed = 0; 

	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;

	/* anti spammer code */
	p_ptr->msgcnt=0;
	p_ptr->msg=0;
	p_ptr->spam=0;

	/* Default location if just starting */
//	if(wpos->wz==0 && wpos->wy==0 && wpos->wx==0 && p_ptr->py==0 && p_ptr->px==0){
	if (new) {
		p_ptr->wpos.wx=cfg.town_x;
		p_ptr->wpos.wy=cfg.town_y;
		p_ptr->wpos.wz=0;
#if 0	// moved afterwards (since town can be non-allocated here)
		p_ptr->py=level_down_y(wpos);
		p_ptr->px=level_down_x(wpos);
#endif	// 0
		p_ptr->town_x=p_ptr->wpos.wx;
		p_ptr->town_y=p_ptr->wpos.wy;
	}
	/* Count players on this depth */
	for (i = 1; i <= NumPlayers; i++)
	{
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
	if (count >= players_on_depth(wpos))
	{
		/* Clear the "marked" and "lit" flags for each cave grid */
		for (y = 0; y < MAX_HGT; y++)
		{
			for (x = 0; x < MAX_WID; x++)
			{
				p_ptr->cave_flag[y][x] = 0;
			}
		}
		/* He is now on the level, so add him to the player_on_depth list */
		new_players_on_depth(wpos, 1, TRUE);

		/* Set the unstaticed variable to true so we know to do a non-LOS requiring
		 * scatter when we place the player.  See below.
		 */
		unstaticed = 1;
	}

	/* Rebuild the level if necessary */
	if(!(zcave=getcave(wpos))){
		if(p_ptr->wpos.wz){
			alloc_dungeon_level(wpos);
			generate_cave(wpos);
			if(p_ptr->lev<=5){
				teleport_player(Ind, 5);
			}
		}
		else{
			alloc_dungeon_level(wpos);
			generate_cave(wpos);
			if(!players_on_depth(wpos))
				new_players_on_depth(wpos,1,FALSE);
#ifndef NEW_DUNGEON
			/* paranoia, update the players wilderness map. */
			p_ptr->wild_map[(-p_ptr->dun_depth)/8] |= (1<<((-p_ptr->dun_depth)%8));
#endif
		}
		zcave=getcave(wpos);
	}
			
	/* Memorize town */
	if (istown(wpos))
	{
		/* Memorize the town if it's daytime */
		for (y = 0; y < MAX_HGT; y++)
		{
			for (x = 0; x < MAX_WID; x++)
			{
				byte *w_ptr = &p_ptr->cave_flag[y][x];

				/* Acquire pointer */
				c_ptr = &zcave[y][x];

				/* If day or interesting, memorize */
//				if (IS_DAY || c_ptr->feat > FEAT_INVIS || c_ptr->info & CAVE_ROOM)
				if (IS_DAY || !cave_plain_floor_grid(c_ptr) || c_ptr->info & CAVE_ROOM)
					*w_ptr |= CAVE_MARK;
			}
		}
	}
	
	/* hack -- update night/day in wilderness levels */
	if ((!wpos->wz) && (IS_DAY)) wild_apply_day(wpos); 
	if ((!wpos->wz) && (IS_NIGHT)) wild_apply_night(wpos);

	if (new) {
#if 0
		p_ptr->py=level_down_y(wpos);
		p_ptr->px=level_down_x(wpos);
#else	// 0
		y=level_rand_y(wpos);
		x=level_rand_x(wpos);
		p_ptr->py=y;
		p_ptr->px=x;

#endif	// 0
	}

	/* Hack be sure the player is inbounds */
	if (p_ptr->px < 1) p_ptr->px = 1;
	if (p_ptr->py < 1) p_ptr->py = 1;
	if (p_ptr->px >= MAX_WID) p_ptr->px = MAX_WID - 1;
	if (p_ptr->py >= MAX_HGT) p_ptr->py = MAX_HGT - 1;

#if 0	// nonsense
	/* Re-Place the player correctly */
	for (i = 0; i < 3000; i++)
	{
		d = (i + 4) / 10;

		/* Pick a location */
		/* Hack -- ghosts&wraithly Sorcerors do not scatter, as they may not be in a line of sight
		   with a valid region */
#if 0
		//              if (!p_ptr->ghost && !p_ptr->wraith_in_wall)
		//{
			// Hack -- require line of sight if the level has not been unstaticed
			// since the player was last on it.  If the player was on it, then
			// don't require LOS since he might be stuck in rock and this will hang
			// the game.
		//scatter(Depth, &y, &x, p_ptr->py, p_ptr->px, d, unstaticed);

		//if (!in_bounds(Depth, y, x) || !cave_empty_bold(Depth, y, x)) continue;
		//}
		//else
#endif
		{
			/* ghosts can be on top of stuff */
			x = p_ptr->px;
			y = p_ptr->py;
		}

		break;
	}
	/* Set the player's location */
	p_ptr->py = y;
	p_ptr->px = x;
#else
	x = p_ptr->px;
	y = p_ptr->py;

	/* Don't stack with another player */
	if (zcave[y][x].m_idx)
	{
		for (i = 0; i < 3000; i++)
		{
			d = (i + 4) / 10;
			scatter(wpos, &y, &x, p_ptr->py, p_ptr->px, d, 0);

			if (!in_bounds(y, x) || !cave_empty_bold(zcave, y, x)) continue;
			else
			{
				p_ptr->px = x;
				p_ptr->py = y;
			}

			break;
		}
	}
#endif	// 0

	/* Update the location's player index */
	zcave[y][x].m_idx = 0 - Ind;

	/* Show him to everybody */
	everyone_lite_spot(wpos, y, x);
	/* Hack -- Give him "awareness" of certain objects */
	for (i = 1; i < max_k_idx; i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Skip "empty" objects */
		if (!k_ptr->name) continue;

		/* No flavor yields aware */
		if (!k_ptr->has_flavor) p_ptr->obj_aware[i] = TRUE;
	}

	/* Add him to the player name database, if he is not already there */
	if (!lookup_player_name(p_ptr->id))
	{
		time_t ttime;
		/* Add */
		add_player_name(p_ptr->name, p_ptr->id, p_ptr->account, p_ptr->prace, p_ptr->pclass, 1, 0, 0, 0, time(&ttime));
	}

	/* Set his "current activities" variables */
	p_ptr->current_spell = p_ptr->current_rod = p_ptr->current_activation = -1;
	p_ptr->current_mind = -1;
	p_ptr->current_selling = p_ptr->store_num = -1;
	p_ptr->current_char = 0;

	/* Set the player's "panel" information */
	l_ptr = getfloor(wpos);
	if (l_ptr)
	{
		/* Hack -- tricky formula, but needed */
		p_ptr->max_panel_rows = ((l_ptr->hgt + SCREEN_HGT / 2) / SCREEN_HGT) * 2 - 2;
		p_ptr->max_panel_cols = ((l_ptr->wid + SCREEN_WID / 2) / SCREEN_WID ) * 2 - 2;

		p_ptr->cur_hgt = l_ptr->hgt;
		p_ptr->cur_wid = l_ptr->wid;
	}
	else
	{
		p_ptr->max_panel_rows = (MAX_HGT / SCREEN_HGT) * 2 - 2;
		p_ptr->max_panel_cols = (MAX_WID / SCREEN_WID) * 2 - 2;

		p_ptr->cur_hgt = MAX_HGT;
		p_ptr->cur_wid = MAX_WID;
	}

	p_ptr->panel_row = ((p_ptr->py - SCREEN_HGT / 4) / (SCREEN_HGT / 2));
	if (p_ptr->panel_row > p_ptr->max_panel_rows) p_ptr->panel_row = p_ptr->max_panel_rows;
	else if (p_ptr->panel_row < 0) p_ptr->panel_row = 0;

	p_ptr->panel_col = ((p_ptr->px - SCREEN_WID / 4) / (SCREEN_WID / 2));
	if (p_ptr->panel_col > p_ptr->max_panel_cols) p_ptr->panel_col = p_ptr->max_panel_cols;
	else if (p_ptr->panel_col < 0) p_ptr->panel_col = 0;

	/* Set the rest of the panel information */
	panel_bounds(Ind);

	/* Make sure his party still exists */
	if (p_ptr->party && parties[p_ptr->party].num == 0)
	{
		/* Reset to neutral */
		p_ptr->party = 0;
	}

	/* Tell the server to redraw the player's display */
	p_ptr->redraw |= PR_MAP | PR_EXTRA | PR_BASIC | PR_HISTORY | PR_VARIOUS;
	p_ptr->redraw |= PR_PLUSSES;

	/* Update his view, light, bonuses, and torch radius */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_BONUS | PU_TORCH | PU_DISTANCE | PU_SKILL_INFO | PU_SKILL_MOD | PU_LUA);

	/* Update his inventory, equipment, and spell info */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);


	p_ptr->master_move_hook=NULL; /* just in case its not */

	/* This guy is alive now */
	p_ptr->alive = TRUE;

	player_create_tmpfile(Ind);
}


/* Hack -- give the bard random skills	- Jir - */
/* TODO: racial skills should be preserved */
static void do_bard_skill(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int i, j, k;
	int value, mod;

	for (i = 1; i < MAX_SKILLS; i++)
	{
		if (i == SKILL_COMBAT || i == SKILL_MASTERY ||
				i == SKILL_ARCHERY || i == SKILL_MAGIC ||
				i == SKILL_SNEAKINESS || i == SKILL_HEALTH ||
//				i == SKILL_NECROMANCY ||
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
			p_ptr->s_info[i].mod *= 1.5;	/* oops floatpoint */
		}
	}

	/* Father zero, child zero */
	for (i = 1; i < MAX_SKILLS; i++)
	{
		s32b value = 0, mod = 0;

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
bool player_birth(int Ind, cptr accname, cptr name, int conn, int race, int class, int sex, int stat_order[6])
{
	player_type *p_ptr;
	int i;
	struct account *c_acc;

	/* Do some consistency checks */
	if (race < 0 || race >= MAX_RACES) race = 0;
	if (class < 0 || class >= MAX_CLASS) class = 0;
//	if (sex < 0 || sex > 7) sex = 0;

	/* Allocate memory for him */
	MAKE(Players[Ind], player_type);

	/* Allocate memory for his inventory */
	C_MAKE(Players[Ind]->inventory, INVEN_TOTAL, object_type);

	/* Set pointer */
	p_ptr = Players[Ind];

	/* Clear old information */
	player_wipe(Ind);

	/* Copy his name and connection info */
	strcpy(p_ptr->name, name);
	p_ptr->conn = conn;

	c_acc=GetAccount(accname, NULL);
	if(c_acc){
		p_ptr->account=c_acc->id;
		p_ptr->noscore=(c_acc->flags & ACC_NOSCORE);
		p_ptr->inval=(c_acc->flags & ACC_TRIAL);
		KILL(c_acc, struct account);
	}

	/* Verify his name and create a savefile name */
	if (!process_player_name(Ind, TRUE)) return FALSE;

	/* Attempt to load from a savefile */
	character_loaded = FALSE;

	confirm_admin(Ind);

	/* Try to load */
	if (!load_player(Ind))
	{
		/* Loading failed badly */
		return FALSE;
	}

	/* Did loading succeed? */
	if (character_loaded)
	{
		/* Loading succeeded */         
		player_setup(Ind, FALSE);
		clockin(Ind, 0);	/* Timestamp the player */
		clockin(Ind, 1);	/* Set player level */
		clockin(Ind, 2);	/* Set player party */
		if(p_ptr->quest_id){
			int i;
			for(i=0; i<20; i++){
				if(quests[i].active && quests[i].id==p_ptr->quest_id) break;
			}
			if(i==20) p_ptr->quest_id=0;
		}
		return TRUE;            
	}

	/* Else, loading failed, but we just create a new character */

	/* Hack -- rewipe the player info if load failed */
	player_wipe(Ind);

	/* Copy his name and connection info */
	strcpy(p_ptr->name, name);
	p_ptr->conn = conn;
	
	/* again ;( */
	c_acc=GetAccount(accname, NULL);
	if(c_acc){
		p_ptr->account=c_acc->id;
		p_ptr->noscore=(c_acc->flags & ACC_NOSCORE);
		p_ptr->inval=(c_acc->flags & ACC_TRIAL);
		KILL(c_acc, struct account);
	}

	/* Reprocess his name */
	if (!process_player_name(Ind, TRUE)) return FALSE;
	
	confirm_admin(Ind);

	/* Set info */
	if (sex > 511)
	{
		sex -= 512;
		p_ptr->fruit_bat = 1;
	}
#if 0
	if (sex > 1)
	{
		sex -= 2;
		p_ptr->mode = MODE_HELL;
	}
	else
	{
		p_ptr->mode = MODE_NORMAL;
	}
#else	// 0
	p_ptr->mode = sex & ~MODE_MALE;

#endif	// 0
	p_ptr->dna = ((class & 0xff) | ((race & 0xff) << 8) );
	p_ptr->dna |= (randint(65535) << 16);
	p_ptr->male = sex & 1;
        p_ptr->pclass = class;
	p_ptr->align_good = 0x7fff;	/* start neutral */
	p_ptr->align_law = 0x7fff;
#if 0
        /* Mega hack but it's fun :) */
        if (!strcmp(name, "Stitch"))
                p_ptr->prace = RACE_EXP626;
        else
#endif
                p_ptr->prace = race;
	p_ptr->pkill=(PKILL_KILLABLE);

	/* Set pointers */
	p_ptr->rp_ptr = &race_info[race];
	p_ptr->cp_ptr = &class_info[class];

	/* Set his ID */
	p_ptr->id = newid();

	/* Actually Generate */
	p_ptr->maximize = cfg.maximize?TRUE:FALSE;

	/* No autoroller */
	get_stats(Ind, stat_order);

	/* Roll for base hitpoints */
	get_extra(Ind);

	/* Roll for age/height/weight */
	get_ahw(Ind);

	/* Roll for social class */
	get_history(Ind);

	/* Roll for gold */
	get_money(Ind);

	/* Hack -- outfit the player */
	player_outfit(Ind);

	/* Set his location, panel, etc. */
	player_setup(Ind, TRUE);

	/* Set up the skills */
	p_ptr->skill_points = 0;
//	p_ptr->skill_last_level = 1;	/* max_plv will do maybe..? */
	for (i = 1; i < MAX_SKILLS; i++)
		p_ptr->s_info[i].dev = FALSE;
	for (i = 1; i < MAX_SKILLS; i++)
	{
		s32b value = 0, mod = 0;

		compute_skills(p_ptr, &value, &mod, i);

		init_skill(p_ptr, value, mod, i);

		/* Develop only revelant branches */
		if (p_ptr->s_info[i].value || p_ptr->s_info[i].mod)
		{
			int z = s_info[i].father;

			while (z != -1)
			{
				p_ptr->s_info[z].dev = TRUE;
				z = s_info[z].father;
				if (z == 0)
					break;
			}
		}
	}

	/* Bards receive really random skills */
	if (p_ptr->pclass == CLASS_BARD)
	{
		do_bard_skill(Ind);
	}

	/* Success */
	return TRUE;
}

/* Disallow non-authorized admin (improvement needed!!) */
/* returns FALSE if bogus admin - Jir - */
bool confirm_admin(int Ind)
{
	struct account *c_acc;
	player_type *p_ptr = Players[Ind];
	bool admin=FALSE;

	c_acc=GetAccountID(p_ptr->account);
	if(!c_acc) return(FALSE);
	if(c_acc->flags&ACC_ADMIN) admin=TRUE;
	/* sucks, but allows an admin wizard. i'll change - evileye */
	/* one DM is enough - jir :) */
//	if(!strcmp(p_ptr->name, c_acc->name)) p_ptr->admin_wiz=admin;
	if(strcmp(p_ptr->name, c_acc->name)) p_ptr->admin_wiz=admin;
	else p_ptr->admin_dm=admin;
	KILL(c_acc, struct account);
	return(admin);
}


/*
 * We are starting a "brand new" server.  We need to initialze the unique
 * info, so that they will be created.  This function is only called if the
 * server state savefile could not be loaded.
 */
void server_birth(void)
{
	int i;

	/* Initialize uniques */
	for (i = 0; i < MAX_R_IDX; i++)
	{
		/* Make sure we have a unique */
		if (!(r_info[i].flags1 & RF1_UNIQUE))
			continue;

		/* Set his maximum creation number */
		r_info[i].max_num = 1;
		
		/* Number of minutes until he respawns */
		/* -1 is used to denote an undefined respawn time.  This should
		 * be set when the unique is killed.
		 */
		r_info[i].respawn_timer = -1;
	}

	/* Set party zero's name to "Neutral" */
	strcpy(parties[0].name, "Neutral");

	/* First player's ID should be 1 */
	account_id = 1;
	player_id = 1;
}

