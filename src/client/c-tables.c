/* $Id$ */
/* File: tables.c */

/* Purpose: Angband Tables */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

/*
 * Only a little of the file is used for the client, the rest is commented out.
 */
#if 1	// Entire file!		- Jir -

#include "angband.h"



/*
 * Global array for looping through the "keypad directions"
 */
s16b ddd[9] =
{ 2, 8, 6, 4, 3, 1, 9, 7, 5 };

/*
 * Global arrays for converting "keypad direction" into offsets
 */
s16b ddx[10] =
{ 0, -1, 0, 1, -1, 0, 1, -1, 0, 1 };

s16b ddy[10] =
{ 0, 1, 1, 1, 0, 0, 0, -1, -1, -1 };

/*
 * Global arrays for optimizing "ddx[ddd[i]]" and "ddy[ddd[i]]"
 */
s16b ddx_ddd[9] =
{ 0, 0, 1, -1, 1, -1, 1, -1, 0 };

s16b ddy_ddd[9] =
{ 1, -1, 0, 0, 1, 1, -1, -1, 0 };


/*
 * Global array for converting numbers to uppercase hecidecimal digit
 * This array can also be used to convert a number to an octal digit
 */
char hexsym[16] =
{
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/*
 * Stat Table (INT/WIS) -- Minimum failure rate (percentage)
 */
byte adj_mag_fail[] =
{
	99	/* 3 */,
	99	/* 4 */,
	99	/* 5 */,
	99	/* 6 */,
	99	/* 7 */,
	50	/* 8 */,
	30	/* 9 */,
	20	/* 10 */,
	15	/* 11 */,
	12	/* 12 */,
	11	/* 13 */,
	10	/* 14 */,
	9	/* 15 */,
	8	/* 16 */,
	7	/* 17 */,
	6	/* 18/00-18/09 */,
	6	/* 18/10-18/19 */,
	5	/* 18/20-18/29 */,
	5	/* 18/30-18/39 */,
	5	/* 18/40-18/49 */,
	4	/* 18/50-18/59 */,
	4	/* 18/60-18/69 */,
	4	/* 18/70-18/79 */,
	4	/* 18/80-18/89 */,
	3	/* 18/90-18/99 */,
	3	/* 18/100-18/109 */,
	2	/* 18/110-18/119 */,
	2	/* 18/120-18/129 */,
	2	/* 18/130-18/139 */,
	2	/* 18/140-18/149 */,
	1	/* 18/150-18/159 */,
	1	/* 18/160-18/169 */,
	1	/* 18/170-18/179 */,
	1	/* 18/180-18/189 */,
	1	/* 18/190-18/199 */,
	0	/* 18/200-18/209 */,
	0	/* 18/210-18/219 */,
	0	/* 18/220+ */
};


/*
 * Stat Table (INT/WIS) -- Various things
 */
byte adj_mag_stat[] =
{
	0	/* 3 */,
	0	/* 4 */,
	0	/* 5 */,
	0	/* 6 */,
	0	/* 7 */,
	1	/* 8 */,
	1	/* 9 */,
	1	/* 10 */,
	1	/* 11 */,
	1	/* 12 */,
	1	/* 13 */,
	1	/* 14 */,
	2	/* 15 */,
	2	/* 16 */,
	2	/* 17 */,
	3	/* 18/00-18/09 */,
	3	/* 18/10-18/19 */,
	3	/* 18/20-18/29 */,
	3	/* 18/30-18/39 */,
	3	/* 18/40-18/49 */,
	4	/* 18/50-18/59 */,
	4	/* 18/60-18/69 */,
	5	/* 18/70-18/79 */,
	6	/* 18/80-18/89 */,
	7	/* 18/90-18/99 */,
	8	/* 18/100-18/109 */,
	9	/* 18/110-18/119 */,
	10	/* 18/120-18/129 */,
	11	/* 18/130-18/139 */,
	12	/* 18/140-18/149 */,
	13	/* 18/150-18/159 */,
	14	/* 18/160-18/169 */,
	15	/* 18/170-18/179 */,
	16	/* 18/180-18/189 */,
	17	/* 18/190-18/199 */,
	18	/* 18/200-18/209 */,
	19	/* 18/210-18/219 */,
	20	/* 18/220+ */
};

#if 0

/*
 * Stat Table (CHR) -- payment percentages
 */
byte adj_chr_gold[] =
{
	130	/* 3 */,
	125	/* 4 */,
	122	/* 5 */,
	120	/* 6 */,
	118	/* 7 */,
	116	/* 8 */,
	114	/* 9 */,
	112	/* 10 */,
	110	/* 11 */,
	108	/* 12 */,
	106	/* 13 */,
	104	/* 14 */,
	103	/* 15 */,
	102	/* 16 */,
	101	/* 17 */,
	100	/* 18/00-18/09 */,
	99	/* 18/10-18/19 */,
	98	/* 18/20-18/29 */,
	97	/* 18/30-18/39 */,
	96	/* 18/40-18/49 */,
	95	/* 18/50-18/59 */,
	94	/* 18/60-18/69 */,
	93	/* 18/70-18/79 */,
	92	/* 18/80-18/89 */,
	91	/* 18/90-18/99 */,
	90	/* 18/100-18/109 */,
	89	/* 18/110-18/119 */,
	88	/* 18/120-18/129 */,
	87	/* 18/130-18/139 */,
	86	/* 18/140-18/149 */,
	85	/* 18/150-18/159 */,
	84	/* 18/160-18/169 */,
	83	/* 18/170-18/179 */,
	82	/* 18/180-18/189 */,
	81	/* 18/190-18/199 */,
	80	/* 18/200-18/209 */,
	80	/* 18/210-18/219 */,
	80	/* 18/220+ */
};


/*
 * Stat Table (INT) -- Magic devices
 */
byte adj_int_dev[] =
{
	0	/* 3 */,
	0	/* 4 */,
	0	/* 5 */,
	0	/* 6 */,
	0	/* 7 */,
	1	/* 8 */,
	1	/* 9 */,
	1	/* 10 */,
	1	/* 11 */,
	1	/* 12 */,
	1	/* 13 */,
	1	/* 14 */,
	2	/* 15 */,
	2	/* 16 */,
	2	/* 17 */,
	3	/* 18/00-18/09 */,
	3	/* 18/10-18/19 */,
	4	/* 18/20-18/29 */,
	4	/* 18/30-18/39 */,
	5	/* 18/40-18/49 */,
	5	/* 18/50-18/59 */,
	6	/* 18/60-18/69 */,
	6	/* 18/70-18/79 */,
	7	/* 18/80-18/89 */,
	7	/* 18/90-18/99 */,
	8	/* 18/100-18/109 */,
	9	/* 18/110-18/119 */,
	10	/* 18/120-18/129 */,
	11	/* 18/130-18/139 */,
	12	/* 18/140-18/149 */,
	13	/* 18/150-18/159 */,
	14	/* 18/160-18/169 */,
	15	/* 18/170-18/179 */,
	16	/* 18/180-18/189 */,
	17	/* 18/190-18/199 */,
	18	/* 18/200-18/209 */,
	19	/* 18/210-18/219 */,
	20	/* 18/220+ */
};


/*
 * Stat Table (WIS) -- Saving throw
 */
byte adj_wis_sav[] =
{
	0	/* 3 */,
	0	/* 4 */,
	0	/* 5 */,
	0	/* 6 */,
	0	/* 7 */,
	1	/* 8 */,
	1	/* 9 */,
	1	/* 10 */,
	1	/* 11 */,
	1	/* 12 */,
	1	/* 13 */,
	1	/* 14 */,
	2	/* 15 */,
	2	/* 16 */,
	2	/* 17 */,
	3	/* 18/00-18/09 */,
	3	/* 18/10-18/19 */,
	3	/* 18/20-18/29 */,
	3	/* 18/30-18/39 */,
	3	/* 18/40-18/49 */,
	4	/* 18/50-18/59 */,
	4	/* 18/60-18/69 */,
	5	/* 18/70-18/79 */,
	5	/* 18/80-18/89 */,
	6	/* 18/90-18/99 */,
	7	/* 18/100-18/109 */,
	8	/* 18/110-18/119 */,
	9	/* 18/120-18/129 */,
	10	/* 18/130-18/139 */,
	11	/* 18/140-18/149 */,
	12	/* 18/150-18/159 */,
	13	/* 18/160-18/169 */,
	14	/* 18/170-18/179 */,
	15	/* 18/180-18/189 */,
	16	/* 18/190-18/199 */,
	17	/* 18/200-18/209 */,
	18	/* 18/210-18/219 */,
	19	/* 18/220+ */
};


/*
 * Stat Table (DEX) -- disarming
 */
byte adj_dex_dis[] =
{
	0	/* 3 */,
	0	/* 4 */,
	0	/* 5 */,
	0	/* 6 */,
	0	/* 7 */,
	0	/* 8 */,
	0	/* 9 */,
	0	/* 10 */,
	0	/* 11 */,
	0	/* 12 */,
	1	/* 13 */,
	1	/* 14 */,
	1	/* 15 */,
	2	/* 16 */,
	2	/* 17 */,
	4	/* 18/00-18/09 */,
	4	/* 18/10-18/19 */,
	4	/* 18/20-18/29 */,
	4	/* 18/30-18/39 */,
	5	/* 18/40-18/49 */,
	5	/* 18/50-18/59 */,
	5	/* 18/60-18/69 */,
	6	/* 18/70-18/79 */,
	6	/* 18/80-18/89 */,
	7	/* 18/90-18/99 */,
	8	/* 18/100-18/109 */,
	8	/* 18/110-18/119 */,
	8	/* 18/120-18/129 */,
	8	/* 18/130-18/139 */,
	8	/* 18/140-18/149 */,
	9	/* 18/150-18/159 */,
	9	/* 18/160-18/169 */,
	9	/* 18/170-18/179 */,
	9	/* 18/180-18/189 */,
	9	/* 18/190-18/199 */,
	10	/* 18/200-18/209 */,
	10	/* 18/210-18/219 */,
	10	/* 18/220+ */
};


/*
 * Stat Table (INT) -- disarming
 */
byte adj_int_dis[] =
{
	0	/* 3 */,
	0	/* 4 */,
	0	/* 5 */,
	0	/* 6 */,
	0	/* 7 */,
	1	/* 8 */,
	1	/* 9 */,
	1	/* 10 */,
	1	/* 11 */,
	1	/* 12 */,
	1	/* 13 */,
	1	/* 14 */,
	2	/* 15 */,
	2	/* 16 */,
	2	/* 17 */,
	3	/* 18/00-18/09 */,
	3	/* 18/10-18/19 */,
	3	/* 18/20-18/29 */,
	4	/* 18/30-18/39 */,
	4	/* 18/40-18/49 */,
	5	/* 18/50-18/59 */,
	6	/* 18/60-18/69 */,
	7	/* 18/70-18/79 */,
	8	/* 18/80-18/89 */,
	9	/* 18/90-18/99 */,
	10	/* 18/100-18/109 */,
	10	/* 18/110-18/119 */,
	11	/* 18/120-18/129 */,
	12	/* 18/130-18/139 */,
	13	/* 18/140-18/149 */,
	14	/* 18/150-18/159 */,
	15	/* 18/160-18/169 */,
	16	/* 18/170-18/179 */,
	17	/* 18/180-18/189 */,
	18	/* 18/190-18/199 */,
	19	/* 18/200-18/209 */,
	19	/* 18/210-18/219 */,
	19	/* 18/220+ */
};


/*
 * Stat Table (DEX) -- bonus to ac (plus 128)
 */
byte adj_dex_ta[] =
{
	128 + -4	/* 3 */,
	128 + -3	/* 4 */,
	128 + -2	/* 5 */,
	128 + -1	/* 6 */,
	128 + 0	/* 7 */,
	128 + 0	/* 8 */,
	128 + 0	/* 9 */,
	128 + 0	/* 10 */,
	128 + 0	/* 11 */,
	128 + 0	/* 12 */,
	128 + 0	/* 13 */,
	128 + 0	/* 14 */,
	128 + 1	/* 15 */,
	128 + 1	/* 16 */,
	128 + 1	/* 17 */,
	128 + 2	/* 18/00-18/09 */,
	128 + 2	/* 18/10-18/19 */,
	128 + 2	/* 18/20-18/29 */,
	128 + 2	/* 18/30-18/39 */,
	128 + 2	/* 18/40-18/49 */,
	128 + 3	/* 18/50-18/59 */,
	128 + 3	/* 18/60-18/69 */,
	128 + 3	/* 18/70-18/79 */,
	128 + 4	/* 18/80-18/89 */,
	128 + 5	/* 18/90-18/99 */,
	128 + 6	/* 18/100-18/109 */,
	128 + 7	/* 18/110-18/119 */,
	128 + 8	/* 18/120-18/129 */,
	128 + 9	/* 18/130-18/139 */,
	128 + 9	/* 18/140-18/149 */,
	128 + 10	/* 18/150-18/159 */,
	128 + 11	/* 18/160-18/169 */,
	128 + 12	/* 18/170-18/179 */,
	128 + 13	/* 18/180-18/189 */,
	128 + 14	/* 18/190-18/199 */,
	128 + 15	/* 18/200-18/209 */,
	128 + 15	/* 18/210-18/219 */,
	128 + 15	/* 18/220+ */
};


/*
 * Stat Table (STR) -- bonus to dam (plus 128)
 */
byte adj_str_td[] =
{
	128 + -2	/* 3 */,
	128 + -2	/* 4 */,
	128 + -1	/* 5 */,
	128 + -1	/* 6 */,
	128 + 0	/* 7 */,
	128 + 0	/* 8 */,
	128 + 0	/* 9 */,
	128 + 0	/* 10 */,
	128 + 0	/* 11 */,
	128 + 0	/* 12 */,
	128 + 0	/* 13 */,
	128 + 0	/* 14 */,
	128 + 0	/* 15 */,
	128 + 1	/* 16 */,
	128 + 2	/* 17 */,
	128 + 2	/* 18/00-18/09 */,
	128 + 2	/* 18/10-18/19 */,
	128 + 3	/* 18/20-18/29 */,
	128 + 3	/* 18/30-18/39 */,
	128 + 3	/* 18/40-18/49 */,
	128 + 3	/* 18/50-18/59 */,
	128 + 3	/* 18/60-18/69 */,
	128 + 4	/* 18/70-18/79 */,
	128 + 5	/* 18/80-18/89 */,
	128 + 5	/* 18/90-18/99 */,
	128 + 6	/* 18/100-18/109 */,
	128 + 7	/* 18/110-18/119 */,
	128 + 8	/* 18/120-18/129 */,
	128 + 9	/* 18/130-18/139 */,
	128 + 10	/* 18/140-18/149 */,
	128 + 11	/* 18/150-18/159 */,
	128 + 12	/* 18/160-18/169 */,
	128 + 13	/* 18/170-18/179 */,
	128 + 14	/* 18/180-18/189 */,
	128 + 15	/* 18/190-18/199 */,
	128 + 16	/* 18/200-18/209 */,
	128 + 18	/* 18/210-18/219 */,
	128 + 20	/* 18/220+ */
};


/*
 * Stat Table (DEX) -- bonus to hit (plus 128)
 */
byte adj_dex_th[] =
{
	128 + -3	/* 3 */,
	128 + -2	/* 4 */,
	128 + -2	/* 5 */,
	128 + -1	/* 6 */,
	128 + -1	/* 7 */,
	128 + 0	/* 8 */,
	128 + 0	/* 9 */,
	128 + 0	/* 10 */,
	128 + 0	/* 11 */,
	128 + 0	/* 12 */,
	128 + 0	/* 13 */,
	128 + 0	/* 14 */,
	128 + 0	/* 15 */,
	128 + 1	/* 16 */,
	128 + 2	/* 17 */,
	128 + 3	/* 18/00-18/09 */,
	128 + 3	/* 18/10-18/19 */,
	128 + 3	/* 18/20-18/29 */,
	128 + 3	/* 18/30-18/39 */,
	128 + 3	/* 18/40-18/49 */,
	128 + 4	/* 18/50-18/59 */,
	128 + 4	/* 18/60-18/69 */,
	128 + 4	/* 18/70-18/79 */,
	128 + 4	/* 18/80-18/89 */,
	128 + 5	/* 18/90-18/99 */,
	128 + 6	/* 18/100-18/109 */,
	128 + 7	/* 18/110-18/119 */,
	128 + 8	/* 18/120-18/129 */,
	128 + 9	/* 18/130-18/139 */,
	128 + 9	/* 18/140-18/149 */,
	128 + 10	/* 18/150-18/159 */,
	128 + 11	/* 18/160-18/169 */,
	128 + 12	/* 18/170-18/179 */,
	128 + 13	/* 18/180-18/189 */,
	128 + 14	/* 18/190-18/199 */,
	128 + 15	/* 18/200-18/209 */,
	128 + 15	/* 18/210-18/219 */,
	128 + 15	/* 18/220+ */
};


/*
 * Stat Table (STR) -- bonus to hit (plus 128)
 */
byte adj_str_th[] =
{
	128 + -3	/* 3 */,
	128 + -2	/* 4 */,
	128 + -1	/* 5 */,
	128 + -1	/* 6 */,
	128 + 0	/* 7 */,
	128 + 0	/* 8 */,
	128 + 0	/* 9 */,
	128 + 0	/* 10 */,
	128 + 0	/* 11 */,
	128 + 0	/* 12 */,
	128 + 0	/* 13 */,
	128 + 0	/* 14 */,
	128 + 0	/* 15 */,
	128 + 0	/* 16 */,
	128 + 0	/* 17 */,
	128 + 1	/* 18/00-18/09 */,
	128 + 1	/* 18/10-18/19 */,
	128 + 1	/* 18/20-18/29 */,
	128 + 1	/* 18/30-18/39 */,
	128 + 1	/* 18/40-18/49 */,
	128 + 1	/* 18/50-18/59 */,
	128 + 1	/* 18/60-18/69 */,
	128 + 2	/* 18/70-18/79 */,
	128 + 3	/* 18/80-18/89 */,
	128 + 4	/* 18/90-18/99 */,
	128 + 5	/* 18/100-18/109 */,
	128 + 6	/* 18/110-18/119 */,
	128 + 7	/* 18/120-18/129 */,
	128 + 8	/* 18/130-18/139 */,
	128 + 9	/* 18/140-18/149 */,
	128 + 10	/* 18/150-18/159 */,
	128 + 11	/* 18/160-18/169 */,
	128 + 12	/* 18/170-18/179 */,
	128 + 13	/* 18/180-18/189 */,
	128 + 14	/* 18/190-18/199 */,
	128 + 15	/* 18/200-18/209 */,
	128 + 15	/* 18/210-18/219 */,
	128 + 15	/* 18/220+ */
};


/*
 * Stat Table (STR) -- weight limit in deca-pounds
 */
byte adj_str_wgt[] =
{
	5	/* 3 */,
	6	/* 4 */,
	7	/* 5 */,
	8	/* 6 */,
	9	/* 7 */,
	10	/* 8 */,
	11	/* 9 */,
	12	/* 10 */,
	13	/* 11 */,
	14	/* 12 */,
	15	/* 13 */,
	16	/* 14 */,
	17	/* 15 */,
	18	/* 16 */,
	19	/* 17 */,
	20	/* 18/00-18/09 */,
	22	/* 18/10-18/19 */,
	24	/* 18/20-18/29 */,
	26	/* 18/30-18/39 */,
	28	/* 18/40-18/49 */,
	30	/* 18/50-18/59 */,
	30	/* 18/60-18/69 */,
	30	/* 18/70-18/79 */,
	30	/* 18/80-18/89 */,
	30	/* 18/90-18/99 */,
	30	/* 18/100-18/109 */,
	30	/* 18/110-18/119 */,
	30	/* 18/120-18/129 */,
	30	/* 18/130-18/139 */,
	30	/* 18/140-18/149 */,
	30	/* 18/150-18/159 */,
	30	/* 18/160-18/169 */,
	30	/* 18/170-18/179 */,
	30	/* 18/180-18/189 */,
	30	/* 18/190-18/199 */,
	30	/* 18/200-18/209 */,
	30	/* 18/210-18/219 */,
	30	/* 18/220+ */
};


/*
 * Stat Table (STR) -- weapon weight limit in pounds
 */
byte adj_str_hold[] =
{
	4	/* 3 */,
	5	/* 4 */,
	6	/* 5 */,
	7	/* 6 */,
	8	/* 7 */,
	10	/* 8 */,
	12	/* 9 */,
	14	/* 10 */,
	16	/* 11 */,
	18	/* 12 */,
	20	/* 13 */,
	22	/* 14 */,
	24	/* 15 */,
	26	/* 16 */,
	28	/* 17 */,
	30	/* 18/00-18/09 */,
	30	/* 18/10-18/19 */,
	35	/* 18/20-18/29 */,
	40	/* 18/30-18/39 */,
	45	/* 18/40-18/49 */,
	50	/* 18/50-18/59 */,
	55	/* 18/60-18/69 */,
	60	/* 18/70-18/79 */,
	65	/* 18/80-18/89 */,
	70	/* 18/90-18/99 */,
	80	/* 18/100-18/109 */,
	80	/* 18/110-18/119 */,
	80	/* 18/120-18/129 */,
	80	/* 18/130-18/139 */,
	80	/* 18/140-18/149 */,
	90	/* 18/150-18/159 */,
	90	/* 18/160-18/169 */,
	90	/* 18/170-18/179 */,
	90	/* 18/180-18/189 */,
	90	/* 18/190-18/199 */,
	100	/* 18/200-18/209 */,
	100	/* 18/210-18/219 */,
	100	/* 18/220+ */
};


/*
 * Stat Table (STR) -- digging value
 */
byte adj_str_dig[] =
{
	0	/* 3 */,
	0	/* 4 */,
	1	/* 5 */,
	2	/* 6 */,
	3	/* 7 */,
	4	/* 8 */,
	4	/* 9 */,
	5	/* 10 */,
	5	/* 11 */,
	6	/* 12 */,
	6	/* 13 */,
	7	/* 14 */,
	7	/* 15 */,
	8	/* 16 */,
	8	/* 17 */,
	9	/* 18/00-18/09 */,
	10	/* 18/10-18/19 */,
	12	/* 18/20-18/29 */,
	15	/* 18/30-18/39 */,
	20	/* 18/40-18/49 */,
	25	/* 18/50-18/59 */,
	30	/* 18/60-18/69 */,
	35	/* 18/70-18/79 */,
	40	/* 18/80-18/89 */,
	45	/* 18/90-18/99 */,
	50	/* 18/100-18/109 */,
	55	/* 18/110-18/119 */,
	60	/* 18/120-18/129 */,
	65	/* 18/130-18/139 */,
	70	/* 18/140-18/149 */,
	75	/* 18/150-18/159 */,
	80	/* 18/160-18/169 */,
	85	/* 18/170-18/179 */,
	90	/* 18/180-18/189 */,
	95	/* 18/190-18/199 */,
	100	/* 18/200-18/209 */,
	100	/* 18/210-18/219 */,
	100	/* 18/220+ */
};


/*
 * Stat Table (STR) -- help index into the "blow" table
 */
byte adj_str_blow[] =
{
	3	/* 3 */,
	4	/* 4 */,
	5	/* 5 */,
	6	/* 6 */,
	7	/* 7 */,
	8	/* 8 */,
	9	/* 9 */,
	10	/* 10 */,
	11	/* 11 */,
	12	/* 12 */,
	13	/* 13 */,
	14	/* 14 */,
	15	/* 15 */,
	16	/* 16 */,
	17	/* 17 */,
	20 /* 18/00-18/09 */,
	30 /* 18/10-18/19 */,
	40 /* 18/20-18/29 */,
	50 /* 18/30-18/39 */,
	60 /* 18/40-18/49 */,
	70 /* 18/50-18/59 */,
	80 /* 18/60-18/69 */,
	90 /* 18/70-18/79 */,
	100 /* 18/80-18/89 */,
	110 /* 18/90-18/99 */,
	120 /* 18/100-18/109 */,
	130 /* 18/110-18/119 */,
	140 /* 18/120-18/129 */,
	150 /* 18/130-18/139 */,
	160 /* 18/140-18/149 */,
	170 /* 18/150-18/159 */,
	180 /* 18/160-18/169 */,
	190 /* 18/170-18/179 */,
	200 /* 18/180-18/189 */,
	210 /* 18/190-18/199 */,
	220 /* 18/200-18/209 */,
	230 /* 18/210-18/219 */,
	240 /* 18/220+ */
};


/*
 * Stat Table (DEX) -- index into the "blow" table
 */
byte adj_dex_blow[] =
{
	0	/* 3 */,
	0	/* 4 */,
	0	/* 5 */,
	0	/* 6 */,
	0	/* 7 */,
	0	/* 8 */,
	0	/* 9 */,
	1	/* 10 */,
	1	/* 11 */,
	1	/* 12 */,
	1	/* 13 */,
	1	/* 14 */,
	1	/* 15 */,
	1	/* 16 */,
	1	/* 17 */,
	1	/* 18/00-18/09 */,
	2	/* 18/10-18/19 */,
	2	/* 18/20-18/29 */,
	2	/* 18/30-18/39 */,
	2	/* 18/40-18/49 */,
	3	/* 18/50-18/59 */,
	3	/* 18/60-18/69 */,
	4	/* 18/70-18/79 */,
	4	/* 18/80-18/89 */,
	5	/* 18/90-18/99 */,
	6	/* 18/100-18/109 */,
	7	/* 18/110-18/119 */,
	8	/* 18/120-18/129 */,
	9	/* 18/130-18/139 */,
	10	/* 18/140-18/149 */,
	11	/* 18/150-18/159 */,
	12	/* 18/160-18/169 */,
	14	/* 18/170-18/179 */,
	16	/* 18/180-18/189 */,
	18	/* 18/190-18/199 */,
	20	/* 18/200-18/209 */,
	20	/* 18/210-18/219 */,
	20	/* 18/220+ */
};


/*
 * Stat Table (DEX) -- chance of avoiding "theft" and "falling"
 */
byte adj_dex_safe[] =
{
	0	/* 3 */,
	1	/* 4 */,
	2	/* 5 */,
	3	/* 6 */,
	4	/* 7 */,
	5	/* 8 */,
	5	/* 9 */,
	6	/* 10 */,
	6	/* 11 */,
	7	/* 12 */,
	7	/* 13 */,
	8	/* 14 */,
	8	/* 15 */,
	9	/* 16 */,
	9	/* 17 */,
	10	/* 18/00-18/09 */,
	10	/* 18/10-18/19 */,
	15	/* 18/20-18/29 */,
	15	/* 18/30-18/39 */,
	20	/* 18/40-18/49 */,
	25	/* 18/50-18/59 */,
	30	/* 18/60-18/69 */,
	35	/* 18/70-18/79 */,
	40	/* 18/80-18/89 */,
	45	/* 18/90-18/99 */,
	50	/* 18/100-18/109 */,
	60	/* 18/110-18/119 */,
	70	/* 18/120-18/129 */,
	80	/* 18/130-18/139 */,
	90	/* 18/140-18/149 */,
	100	/* 18/150-18/159 */,
	100	/* 18/160-18/169 */,
	100	/* 18/170-18/179 */,
	100	/* 18/180-18/189 */,
	100	/* 18/190-18/199 */,
	100	/* 18/200-18/209 */,
	100	/* 18/210-18/219 */,
	100	/* 18/220+ */
};


/*
 * Stat Table (CON) -- base regeneration rate
 */
byte adj_con_fix[] =
{
	0	/* 3 */,
	0	/* 4 */,
	0	/* 5 */,
	0	/* 6 */,
	0	/* 7 */,
	0	/* 8 */,
	0	/* 9 */,
	0	/* 10 */,
	0	/* 11 */,
	0	/* 12 */,
	0	/* 13 */,
	1	/* 14 */,
	1	/* 15 */,
	1	/* 16 */,
	1	/* 17 */,
	2	/* 18/00-18/09 */,
	2	/* 18/10-18/19 */,
	2	/* 18/20-18/29 */,
	2	/* 18/30-18/39 */,
	2	/* 18/40-18/49 */,
	3	/* 18/50-18/59 */,
	3	/* 18/60-18/69 */,
	3	/* 18/70-18/79 */,
	3	/* 18/80-18/89 */,
	3	/* 18/90-18/99 */,
	4	/* 18/100-18/109 */,
	4	/* 18/110-18/119 */,
	5	/* 18/120-18/129 */,
	6	/* 18/130-18/139 */,
	6	/* 18/140-18/149 */,
	7	/* 18/150-18/159 */,
	7	/* 18/160-18/169 */,
	8	/* 18/170-18/179 */,
	8	/* 18/180-18/189 */,
	8	/* 18/190-18/199 */,
	9	/* 18/200-18/209 */,
	9	/* 18/210-18/219 */,
	9	/* 18/220+ */
};


/*
 * Stat Table (CON) -- extra half-hitpoints per level (plus 128)
 */
byte adj_con_mhp[] =
{
	128 + -5	/* 3 */,
	128 + -3	/* 4 */,
	128 + -2	/* 5 */,
	128 + -1	/* 6 */,
	128 + 0	/* 7 */,
	128 + 0	/* 8 */,
	128 + 0	/* 9 */,
	128 + 0	/* 10 */,
	128 + 0	/* 11 */,
	128 + 0	/* 12 */,
	128 + 0	/* 13 */,
	128 + 0	/* 14 */,
	128 + 1	/* 15 */,
	128 + 1	/* 16 */,
	128 + 2	/* 17 */,
	128 + 3	/* 18/00-18/09 */,
	128 + 4	/* 18/10-18/19 */,
	128 + 4	/* 18/20-18/29 */,
	128 + 4	/* 18/30-18/39 */,
	128 + 4	/* 18/40-18/49 */,
	128 + 5	/* 18/50-18/59 */,
	128 + 6	/* 18/60-18/69 */,
	128 + 7	/* 18/70-18/79 */,
	128 + 8	/* 18/80-18/89 */,
	128 + 9	/* 18/90-18/99 */,
	128 + 10	/* 18/100-18/109 */,
	128 + 11	/* 18/110-18/119 */,
	128 + 12	/* 18/120-18/129 */,
	128 + 13	/* 18/130-18/139 */,
	128 + 14	/* 18/140-18/149 */,
	128 + 15	/* 18/150-18/159 */,
	128 + 16	/* 18/160-18/169 */,
	128 + 18	/* 18/170-18/179 */,
	128 + 20	/* 18/180-18/189 */,
	128 + 22	/* 18/190-18/199 */,
	128 + 25	/* 18/200-18/209 */,
	128 + 25	/* 18/210-18/219 */,
	128 + 25	/* 18/220+ */
};


/*
 * This table is used to help calculate the number of blows the player can
 * make in a single round of attacks (one player turn) with a normal weapon.
 *
 * This number ranges from a single blow/round for weak players to up to six
 * blows/round for powerful warriors.
 *
 * Note that certain artifacts and ego-items give "bonus" blows/round.
 *
 * First, from the player class, we extract some values:
 *
 *    Warrior --> num = 6; mul = 5; div = MAX(30, weapon_weight);
 *    Mage    --> num = 4; mul = 2; div = MAX(40, weapon_weight);
 *    Priest  --> num = 5; mul = 3; div = MAX(35, weapon_weight);
 *    Rogue   --> num = 5; mul = 3; div = MAX(30, weapon_weight);
 *    Ranger  --> num = 5; mul = 4; div = MAX(35, weapon_weight);
 *    Paladin --> num = 5; mul = 4; div = MAX(30, weapon_weight);
 *
 * To get "P", we look up the relevant "adj_str_blow[]" (see above),
 * multiply it by "mul", and then divide it by "div", rounding down.
 *
 * To get "D", we look up the relevant "adj_dex_blow[]" (see above),
 * note especially column 6 (DEX 18/101) and 11 (DEX 18/150).
 *
 * The player gets "blows_table[P][D]" blows/round, as shown below,
 * up to a maximum of "num" blows/round, plus any "bonus" blows/round.
 */
byte blows_table[12][12] =
{
	/* P/D */
	/* 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11+ */

	/* 0  */
	{  1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   3 },

	/* 1  */
	{  1,   1,   1,   1,   2,   2,   3,   3,   3,   4,   4,   4 },

	/* 2  */
	{  1,   1,   2,   2,   3,   3,   4,   4,   4,   5,   5,   5 },

	/* 3  */
	{  1,   2,   2,   3,   3,   4,   4,   4,   5,   5,   5,   5 },

	/* 4  */
	{  1,   2,   2,   3,   3,   4,   4,   5,   5,   5,   5,   5 },

	/* 5  */
	{  2,   2,   3,   3,   4,   4,   5,   5,   5,   5,   5,   6 },

	/* 6  */
	{  2,   2,   3,   3,   4,   4,   5,   5,   5,   5,   5,   6 },

	/* 7  */
	{  2,   3,   3,   4,   4,   4,   5,   5,   5,   5,   5,   6 },

	/* 8  */
	{  3,   3,   3,   4,   4,   4,   5,   5,   5,   5,   6,   6 },

	/* 9  */
	{  3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6 },

	/* 10 */
	{  3,   3,   4,   4,   4,   4,   5,   5,   5,   6,   6,   6 },

	/* 11+ */
	{  3,   3,   4,   4,   4,   4,   5,   5,   6,   6,   6,   6 },
};


#if 0

/*
 * This is the "old" table used to calculate multiple blows.
 *
 * Note that this table used a different indexing scheme to determine "P"
 */

byte old_blows_table[11][12] =
{
	/* P/D */
	/* 3,  10, /01, /50, /90,/100,/101,/110,/120,/130,/140,/150 */

	/* 0+ */
	{  1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   3},

	/* 2+ */
	{  1,   1,   1,   1,   2,   2,   3,   3,   3,   3,   3,   4},

	/* 3+ */
	{  1,   1,   1,   2,   2,   3,   4,   4,   4,   4,   4,   5},

	/* 4+ */
	{  1,   1,   2,   2,   3,   3,   4,   4,   4,   5,   5,   5},

	/* 6+ */
	{  1,   2,   2,   3,   3,   4,   4,   4,   5,   5,   5,   5},

	/* 8+ */
	{  1,   2,   2,   3,   4,   4,   4,   5,   5,   5,   5,   5},

	/* 10+ */
	{  2,   2,   3,   3,   4,   4,   5,   5,   5,   5,   5,   6},

	/* 13+ */
	{  2,   3,   3,   3,   4,   4,   5,   5,   5,   5,   5,   6},

	/* 15+ */
	{  3,   3,   3,   4,   4,   4,   5,   5,   5,   5,   6,   6},

	/* 18+ */
	{  3,   3,   3,   4,   4,   4,   5,   5,   5,   5,   6,   6},

	/* 20+ */
	{  3,   3,   4,   4,   4,   4,   5,   5,   5,   6,   6,   6}
};

#endif

#endif

/*
 * Each chest has a certain set of traps, determined by pval
 * Each chest has a "pval" from 1 to the chest level (max 55)
 * If the "pval" is negative then the trap has been disarmed
 * The "pval" of a chest determines the quality of its treasure
 * Note that disarming a trap on a chest also removes the lock.
 */
byte chest_traps[64] =
{
	0,					/* 0 == empty */
	(CHEST_POISON),
	(CHEST_LOSE_STR),
	(CHEST_LOSE_CON),
	(CHEST_LOSE_STR),
	(CHEST_LOSE_CON),			/* 5 == best small wooden */
	0,
	(CHEST_POISON),
	(CHEST_POISON),
	(CHEST_LOSE_STR),
	(CHEST_LOSE_CON),
	(CHEST_POISON),
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_SUMMON),			/* 15 == best large wooden */
	0,
	(CHEST_LOSE_STR),
	(CHEST_LOSE_CON),
	(CHEST_PARALYZE),
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_SUMMON),
	(CHEST_PARALYZE),
	(CHEST_LOSE_STR),
	(CHEST_LOSE_CON),
	(CHEST_EXPLODE),			/* 25 == best small iron */
	0,
	(CHEST_POISON | CHEST_LOSE_STR),
	(CHEST_POISON | CHEST_LOSE_CON),
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_PARALYZE),
	(CHEST_POISON | CHEST_SUMMON),
	(CHEST_SUMMON),
	(CHEST_EXPLODE),
	(CHEST_EXPLODE | CHEST_SUMMON),	/* 35 == best large iron */
	0,
	(CHEST_SUMMON),
	(CHEST_EXPLODE),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_POISON | CHEST_PARALYZE),
	(CHEST_EXPLODE),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_POISON | CHEST_PARALYZE),	/* 45 == best small steel */
	0,
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_POISON | CHEST_PARALYZE | CHEST_LOSE_STR),
	(CHEST_POISON | CHEST_PARALYZE | CHEST_LOSE_CON),
	(CHEST_POISON | CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_POISON | CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_POISON | CHEST_PARALYZE | CHEST_LOSE_STR | CHEST_LOSE_CON),
	(CHEST_POISON | CHEST_PARALYZE),
	(CHEST_POISON | CHEST_PARALYZE),	/* 55 == best large steel */
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
	(CHEST_EXPLODE | CHEST_SUMMON),
};








/*
 * Class titles for the player.
 *
 * The player gets a new title every five levels, so each class
 * needs only ten titles total.
 */
cptr player_title[MAX_CLASS][PY_MAX_LEVEL/5] =
{
	/* Adventurer */
	{
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
		"Adventurer",
	},

	/* Warrior */
	{
		"Rookie",
		"Soldier",
		"Mercenary",
		"Veteran",
		"Swordsman",
		"Champion",
		"Hero",
		"Baron",
		"Duke",
		"Lord",
	},

	/* Warlock */
	{
		"Novice",
		"Apprentice",
		"Trickster",
		"Illusionist",
		"Spellbinder",
		"Evoker",
		"Conjurer",
		"Warlock",
		"Sorcerer",
		"Mage Lord",
	},

	/* Priest */
	{
		"Believer",
		"Acolyte",
		"Adept",
		"Curate",
		"Canon",
		"Lama",
		"Patriarch",
		"Priest",
		"High Priest",
		"Priest Lord",
	},

	/* Rogues */
	{
		"Vagabond",
		"Cutpurse",
		"Robber",
		"Burglar",
		"Filcher",
		"Sharper",
		"Low Thief",
		"High Thief",
		"Master Thief",
		"Assassin",
	},
	/* Mimic */
	{
                "Copier",
                "Copier",
                "Modifier",
                "Multiple",
                "Multiple",
                "Changer",
                "Metamorph",
                "Metamorph",
                "Shapeshifter",
                "Shapeshifter",
        },

        /* Archer */
	{
                "Rock Thrower",
                "Slinger",
                "Great Slinger",
                "Bowsen",
                "Bowsen",
                "Great Bowmen",
                "Great Bowmen",
                "Archer",
                "Archer",
                "Great Archer",
	},
};



/*
 * Hack -- the "basic" color names (see "TERM_xxx")
 */
cptr color_names[16] =
{
	"Dark",
	"White",
	"Slate",
	"Orange",
	"Red",
	"Green",
	"Blue",
	"Umber",
	"Light Dark",
	"Light Slate",
	"Violet",
	"Yellow",
	"Light Red",
	"Light Green",
	"Light Blue",
	"Light Umber",
};


/*
 * Hack -- the "basic" sound names (see "SOUND_xxx")
 */
cptr sound_names[SOUND_MAX] =
{
	"",
	"hit",
	"miss",
	"flee",
	"drop",
	"kill",
	"level",
	"death",
};



/*
 * Abbreviations of healthy stats
 */
cptr stat_names[6] =
{
	"STR: ", "INT: ", "WIS: ", "DEX: ", "CON: ", "CHR: "
};

/*
 * Abbreviations of damaged stats
 */
cptr stat_names_reduced[6] =
{
	"Str: ", "Int: ", "Wis: ", "Dex: ", "Con: ", "Chr: "
};


/*
 * Standard window names
 */
cptr ang_term_name[8] =
{
	"Angband",
	"Mirror",
	"Recall",
	"Choice",
	"Term-4",
	"Term-5",
	"Term-6",
	"Term-7"
};


/*
 * Certain "screens" always use the main screen, including News, Birth,
 * Dungeon, Tomb-stone, High-scores, Macros, Colors, Visuals, Options.
 *
 * Later, special flags may allow sub-windows to "steal" stuff from the
 * main window, including File dump (help), File dump (artifacts, uniques),
 * Character screen, Small scale map, Previous Messages, Store screen, etc.
 *
 * The "ctrl-i" (tab) command flips the "Display inven/equip" and "Display
 * equip/inven" flags for all windows.
 *
 * The "ctrl-g" command (or pseudo-command) should perhaps grab a snapshot
 * of the main screen into any interested windows.
 */
cptr window_flag_desc[32] =
{
	"Display inven/equip",
	"Display equip/inven",
	"Display spell list",
	"Display character",
	NULL,
	NULL,
	"Display messages",
	"Display overhead view",
	"Display monster recall",
	"Display object recall",
	NULL,
	"Display snap-shot",
	NULL,
	NULL,
	"Display borg messages",
	"Display borg status",
	NULL,
	NULL,
	"Display chat",
	"Display msgs except chat",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};


/*
 * Available Options
 *
 * Option Screen Sets:
 *
 *	Set 1: User Interface
 *	Set 2: Disturbance
 *	Set 3: Inventory
 *	Set 4: Game Play
 *
 * Note that bits 28-31 of set 0 are currently unused.
 */
/*
 * XXX XXX hard-coded in sync_options!
 */
/*
   bool *o_var;

   byte	o_norm;

   byte	o_page;

   byte	o_set;	// not used at all
   byte	o_bit;	// not used at all

   cptr	o_text;
   cptr	o_desc;
*/
option_type option_info[OPT_MAX] =
{
	/*** User-Interface ***/

	{ &c_cfg.rogue_like_commands,	FALSE,	1,	0, 0,
	"rogue_like_commands",	"Rogue-like commands" },

	{ &c_cfg.quick_messages,	 	FALSE,	1,	0, 1,
	"quick_messages",		"Activate quick messages (skill etc)" },

	{ &c_cfg.other_query_flag,	FALSE,	1,	0, 2,	// XXX
	"other_query_flag",		"Prompt for various information (mimic polymorph)" },

	{ &c_cfg.carry_query_flag,	FALSE,	1,	0, 3,	// XXX
	"carry_query_flag",		"(broken) Prompt before picking things up" },

	{ &c_cfg.use_old_target,		FALSE,	1,	0, 4,
	"use_old_target",		"Use old target by default" },

	{ &c_cfg.always_pickup,		TRUE,	1,	0, 5,
	"always_pickup",		"Pick things up by default" },

	{ &c_cfg.always_repeat,		TRUE,	1,	0, 6,	// XXX
	"always_repeat",		"Repeat obvious commands" },

	{ &c_cfg.depth_in_feet,		TRUE,	1,	0, 7,
	"depth_in_feet",		"Show dungeon level in feet" },

	{ &c_cfg.stack_force_notes,	FALSE,	1,	0, 8,
	"stack_force_notes",	"Merge inscriptions when stacking" },

	{ &c_cfg.stack_force_costs,	FALSE,	1,	0, 9,
	"stack_force_costs",	"Merge discounts when stacking" },

	{ &c_cfg.show_labels,			TRUE,	1,	0, 10,	// XXX
	"show_labels",			"(broken) Show labels in object listings" },

	{ &c_cfg.show_weights,		FALSE,	1,	0, 11,
	"show_weights",			"Show weights in object listings" },

	{ &c_cfg.show_choices,		FALSE,	1,	0, 12,	// XXX
	"show_choices",			"(broken) Show choices in certain sub-windows" },

	{ &c_cfg.show_details,		FALSE,	1,	0, 13,	// XXX
	"show_details",			"(broken) Show details in certain sub-windows" },

	{ &c_cfg.ring_bell,			TRUE,	1,	0, 14,
	"ring_bell",			"Audible bell (on errors, etc)" },

	{ &c_cfg.use_color,			TRUE,	1,	0, 15,
	"use_color",			"Use color if possible (slow)" },


	/*** Disturbance ***/

	{ &c_cfg.find_ignore_stairs,	TRUE,	2,	0, 16,
	"find_ignore_stairs",	"Run past stairs" },

	{ &c_cfg.find_ignore_doors,	TRUE,	2,	0, 17,
	"find_ignore_doors",	"Run through open doors" },

	{ &c_cfg.find_cut,			TRUE,	2,	0, 18,
	"find_cut",				"Run past known corners" },

	{ &c_cfg.find_examine,		TRUE,	2,	0, 19,
	"find_examine",			"Run into potential corners" },

	{ &c_cfg.disturb_move,		TRUE,	2,	0, 20,
	"disturb_move",			"Disturb whenever any monster moves" },

	{ &c_cfg.disturb_near,		TRUE,	2,	0, 21,
	"disturb_near",			"Disturb whenever viewable monster moves" },

	{ &c_cfg.disturb_panel,		TRUE,	2,	0, 22,
	"disturb_panel",		"Disturb whenever map panel changes" },

	{ &c_cfg.disturb_state,		TRUE,	2,	0, 23,
	"disturb_state",		"Disturb whenever player state changes" },

	{ &c_cfg.disturb_minor,		TRUE,	2,	0, 24,
	"disturb_minor",		"Disturb whenever boring things happen" },

	{ &c_cfg.disturb_other,		TRUE,	2,	0, 25,
	"disturb_other",		"Disturb whenever various things happen" },

	{ &c_cfg.alert_hitpoint,		FALSE,	2,	0, 26,
	"alert_hitpoint",		"Alert user to critical hitpoints/sanity" },

	{ &c_cfg.alert_failure,		FALSE,	2,	0, 27,	// XXX
	"alert_failure",		"(broken) Alert user to various failures" },


	/*** Game-Play ***/

	{ &c_cfg.auto_afk,			FALSE,	3,	1, 0,	// former auto_haggle
//	"auto_afk",				"Allow automatic AFKing" },
	"auto_afk",				"Set 'AFK mode' automatically" },

	{ &c_cfg.newb_suicide,			FALSE,	3,	1, 1,	// former auto_scum
	"newb_suicide",			"Display newbie suicides" },

	{ &c_cfg.stack_allow_items,	TRUE,	3,	1, 2,
	"stack_allow_items",	"Allow weapons and armor to stack" },

	{ &c_cfg.stack_allow_wands,	TRUE,	3,	1, 3,
	"stack_allow_wands",	"Allow wands/staffs/rods to stack" },

	{ &c_cfg.expand_look,			FALSE,	3,	1, 4,	// XXX
	"expand_look",			"(broken) Expand the power of the look command" },

	{ &c_cfg.expand_list,			FALSE,	3,	1, 5,	// XXX
	"expand_list",			"(broken) Expand the power of the list commands" },

	{ &c_cfg.view_perma_grids,	TRUE,	3,	1, 6,
	"view_perma_grids",		"Map remembers all perma-lit grids" },

	{ &c_cfg.view_torch_grids,	FALSE,	3,	1, 7,
	"view_torch_grids",		"Map remembers all torch-lit grids" },

	{ &c_cfg.dungeon_align,		TRUE,	3,	1, 8,	// XXX
	"dungeon_align",		"(obsolete) Generate dungeons with aligned rooms" },

	{ &c_cfg.dungeon_stair,		TRUE,	3,	1, 9,	// XXX
	"dungeon_stair",		"(obsolete) Generate dungeons with connected stairs" },

	{ &c_cfg.recall_flicker,		FALSE,	3,	1, 10,	// XXX
	"recall_flicker",		"Flicker messages in recall" },

	{ &c_cfg.flow_by_smell,		FALSE,	3,	1, 11,	// XXX
	"flow_by_smell",		"(obsolete) Monsters chase recent locations (v.slow)" },

	{ &c_cfg.track_follow,		FALSE,	3,	1, 12,	// XXX
	"track_follow",			"(obsolete) Monsters follow the player (broken)" },

	{ &c_cfg.track_target,		FALSE,	3,	1, 13,	// XXX
	"track_target",			"(obsolete) Monsters target the player (broken)" },

	{ &c_cfg.smart_learn,			FALSE,	3,	1, 14,	// XXX
	"smart_learn",			"(obsolete) Monsters learn from their mistakes" },

	{ &c_cfg.smart_cheat,			FALSE,	3,	1, 15,	// XXX
	"smart_cheat",			"(obsolete) Monsters exploit players weaknesses" },


	/*** Efficiency ***/

	{ &c_cfg.view_reduce_lite,	FALSE,	4,	1, 16,	// (44)
	"view_reduce_lite",		"Reduce lite-radius when running" },

	{ &c_cfg.view_reduce_view,	FALSE,	4,	1, 17,
	"view_reduce_view",		"Reduce view-radius in town" },

	{ &c_cfg.avoid_abort,			FALSE,	4,	1, 18,	// XXX
	"avoid_abort",			"(obsolete) Avoid checking for user abort" },

	{ &c_cfg.avoid_other,			FALSE,	4,	1, 19,	// XXX
	"avoid_other",			"(broken) Avoid processing special colors" },

	{ &c_cfg.flush_failure,		TRUE,	4,	1, 20,	// XXX (resurrect me?)
	"flush_failure",		"(broken) Flush input on various failures" },

	{ &c_cfg.flush_disturb,		FALSE,	4,	1, 21,	// XXX
	"flush_disturb",		"(broken) Flush input whenever disturbed" },

	{ &c_cfg.flush_command,		FALSE,	4,	1, 22,	// XXX
	"flush_command",		"(obsolete) Flush input before every command" },

	{ &c_cfg.fresh_before,		TRUE,	4,	1, 23,	// XXX
	"fresh_before",			"(obsolete) Flush output before every command" },

	{ &c_cfg.fresh_after,			FALSE,	4,	1, 24,	// XXX
	"fresh_after",			"(obsolete) Flush output after every command" },

	{ &c_cfg.fresh_message,		FALSE,	4,	1, 25,	// XXX
	"fresh_message",		"(obsolete) Flush output after every message" },

	{ &c_cfg.compress_savefile,	TRUE,	4,	1, 26,	// XXX
	"compress_savefile",	"(broken) Compress messages in savefiles" },

	{ &c_cfg.hilite_player,		FALSE,	4,	1, 27,	// XXX (resurrect me)
	"hilite_player",		"(broken) Hilite the player with the cursor" },

	{ &c_cfg.view_yellow_lite,	FALSE,	4,	1, 28,
	"view_yellow_lite",		"Use special colors for torch-lit grids" },

	{ &c_cfg.view_bright_lite,	FALSE,	4,	1, 29,
	"view_bright_lite",		"Use special colors for 'viewable' grids" },

	{ &c_cfg.view_granite_lite,	FALSE,	4,	1, 30,
	"view_granite_lite",	"Use special colors for wall grids (slow)" },

	{ &c_cfg.view_special_lite,	FALSE,	4,	1, 31,	// (59)
	"view_special_lite",	"Use special colors for floor grids (slow)" },


	/*** TomeNET additions ***/

	{ &c_cfg.easy_open,			TRUE,	5,	9, 60,
	"easy_open",			"Automatically open doors" },

	{ &c_cfg.easy_disarm,			FALSE,	5,	9, 61,
	"easy_disarm",			"Automatically disarm traps" },

	{ &c_cfg.easy_tunnel,			FALSE,	5,	9, 62,
	"easy_tunnel",			"Automatically tunnel walls" },

	{ &c_cfg.auto_destroy,		FALSE,	5,	9, 63,
	"auto_destroy",			"(broken) No query to destroy known junks" },
//	"auto_destroy",			"(broken) No query to destroy known worthless items" },

	{ &c_cfg.auto_inscribe,		FALSE,	5,	9, 64,
	"auto_inscribe",		"Automatically inscribe books and so on" },

	{ &c_cfg.taciturn_messages,	FALSE,	5,	9, 65,
	"taciturn_messages",	"Suppress server messages as far as possible" },

	{ &c_cfg.last_words,			TRUE,	5,	9, 66,
	"last_words",			"Get last words when the character dies" },

	{ &c_cfg.limit_chat,			FALSE,	5,	9, 67,
	"limit_chat",			"Chat only with players on the same floor" },

	{ &c_cfg.thin_down_flush,		TRUE,	5,	9, 68,
	"thin_down_flush",		"Thin down screen flush signals to avoid freezing" },

	{ &c_cfg.auto_target,			FALSE,	5,	9, 69,
	"auto_target",			"Automatically set target to the nearest enemy" },

	{ &c_cfg.autooff_retaliator,	FALSE,	5,	9, 70,
	"autooff_retaliator",	"Stop the retaliator when protected by GoI etc" },

	{ &c_cfg.wide_scroll_margin,	FALSE,	5,	9, 71,
	"wide_scroll_margin",	"Scroll the screen more frequently" },

	{ &c_cfg.fail_no_melee,		FALSE,	5,	9, 72,
	"fail_no_melee",		"Stay still when item-retaliation fails" },

	{ &c_cfg.always_show_lists,		FALSE,	5,	9, 73,
	"always_show_lists",	"Always show lists in item/skill selection" },

	{ &c_cfg.target_history,		FALSE,	5,	9, 74,
	"target_history",		"Add target informations to the message history" },

	{ &c_cfg.linear_stats,			FALSE,	5,	9, 75,
	"linear_stats",			"Stats are represented in a linear way" },

	{ &c_cfg.exp_need,				FALSE,  5,  9, 76,
	"exp_need",				"Show the experience needed for next level" },

#if 0
	{ &c_cfg.speak_unique,                TRUE,   2,      13,
	"speak_unique",                 "Allow shopkeepers and uniques to speak" },
#endif	// 0

	/*** End of Table ***/

	{ NULL,			0, 0, 0, 0,
	NULL,			NULL }
};

/* XXX XXX they should be sent from server like other spells! */
cptr monster_spells4[32] =
{
  "Shriek",
  "Negate magic",
  "XXX3",
  "Rocket",
  "Arrow",
  "Bolt",
  "XXX7",
  "XXX8",
  "Breath Acid",
  "Breath Lightning",
  "Breath Fire",
  "Breath Cold",
  "Breath Poison",
  "Breath Nether",
  "Breath Lite",
  "Breath Darkness",
  "Breath Confusion",
  "Breath Sound",
  "Breath Chaos",
  "Breath Disenchantment",
  "Breath Nexus",
  "Breath Time",
  "Breath Inertia",
  "Breath Gravity",
  "Breath Shards",
  "Breath Plasma",
  "Breath Force",
  "Breath Mana",
  "Breath Disintegration",
  "Breath Toxic Waste",
  "XXX",
  "Boulder",	// "XXX",
};

/*
 * New monster race bit flags
 */
cptr monster_spells5[32] =
{
  "Ball of Acid",
  "Ball of Lightning",
  "Ball of Fire",
  "Ball of Cold",
  "Ball of Poison",
  "Ball of Nether",
  "Ball of Water",
  "Ball of Mana",
  "Ball of Darkness",
  "Drain Mana",
  "Mind Blast",
  "Brain Smash",
  //"Cause Light Wounds",
  "Cause Wounds",
  "XXX",
  "Ball of Toxic Waste",
  "Ball of Chaos",
  "Bolt of Acid",
  "Bolt of Lightning",
  "Bolt of Fire",
  "Bolt of Cold",
  "Bolt of Poison",
  "Bolt of Nether",
  "Bolt of Water",
  "Bolt of Mana",
  "Bolt of Plasma",
  "Bolt of Ice",
  "Magic Missile",
  "Scare",
  "Blind",
  "Confusion",
  "Slow",
  "Paralyze",
};

/*
 * New monster race bit flags
 */
cptr monster_spells6[32] =
{
  "Speed",
  "Hand of Doom",	// "XXX",
  "Heal",
  "XXX",
  "Blink",
  "Teleport",
  "XXX",
  "XXX",
  "Teleport To",
  "Teleport Away",
  "Teleport Level",
  "XXX",
  "Darkness",
  "Traps",
  "Forget",
  "XXX",
  "XXX",
  "XXX",
  /* Summons follow, but players can't summon */
};

#endif	// 0
