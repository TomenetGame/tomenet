/* $Id$ */
/* common.c - common functions */

#include "angband.h"

/*
 * XXX XXX XXX Important note about "colors" XXX XXX XXX
 *
 * The "TERM_*" color definitions list the "composition" of each
 * "Angband color" in terms of "quarters" of each of the three color
 * components (Red, Green, Blue), for example, TERM_UMBER is defined
 * as 2/4 Red, 1/4 Green, 0/4 Blue.
 *
 * The following info is from "Torbjorn Lindgren" (see "main-xaw.c").
 *
 * These values are NOT gamma-corrected.  On most machines (with the
 * Macintosh being an important exception), you must "gamma-correct"
 * the given values, that is, "correct for the intrinsic non-linearity
 * of the phosphor", by converting the given intensity levels based
 * on the "gamma" of the target screen, which is usually 1.7 (or 1.5).
 *
 * The actual formula for conversion is unknown to me at this time,
 * but you can use the table below for the most common gamma values.
 *
 * So, on most machines, simply convert the values based on the "gamma"
 * of the target screen, which is usually in the range 1.5 to 1.7, and
 * usually is closest to 1.7.  The converted value for each of the five
 * different "quarter" values is given below:
 *
 *  Given     Gamma 1.0       Gamma 1.5       Gamma 1.7     Hex 1.7
 *  -----       ----            ----            ----          ---
 *   0/4        0.00            0.00            0.00          #00
 *   1/4        0.25            0.27            0.28          #47
 *   2/4        0.50            0.55            0.56          #8f
 *   3/4        0.75            0.82            0.84          #d7
 *   4/4        1.00            1.00            1.00          #ff
 *
 * Note that some machines (i.e. most IBM machines) are limited to a
 * hard-coded set of colors, and so the information above is useless.
 *
 * Also, some machines are limited to a pre-determined set of colors,
 * for example, the IBM can only display 16 colors, and only 14 of
 * those colors resemble colors used by Angband, and then only when
 * you ignore the fact that "Slate" and "cyan" are not really matches,
 * so on the IBM, we use "orange" for both "Umber", and "Light Umber"
 * in addition to the obvious "Orange", since by combining all of the
 * "indeterminate" colors into a single color, the rest of the colors
 * are left with "meaningful" values.
 */

/*
 * Convert a "color letter" into an "actual" color
 * The colors are: dwsorgbuDWvyRGBU, as shown below
 */
int color_char_to_attr(char c) {
	switch (c) {
		case 'd': return (TERM_DARK);
		case 'w': return (TERM_WHITE);
		case 's': return (TERM_SLATE);
		case 'o': return (TERM_ORANGE);
		case 'r': return (TERM_RED);
		case 'g': return (TERM_GREEN);
		case 'b': return (TERM_BLUE);
		case 'u': return (TERM_UMBER);

		case 'D': return (TERM_L_DARK);
		case 'W': return (TERM_L_WHITE);
		case 'v': return (TERM_VIOLET);
		case 'y': return (TERM_YELLOW);
		case 'R': return (TERM_L_RED);
		case 'G': return (TERM_L_GREEN);
		case 'B': return (TERM_L_BLUE);
		case 'U': return (TERM_L_UMBER);

		case 'p': return (TERM_POIS);
		case 'f': return (TERM_FIRE);
		case 'a': return (TERM_ACID);
		case 'e': return (TERM_ELEC);
		case 'c': return (TERM_COLD);
		case 'h': return (TERM_HALF);
		case 'm': return (TERM_MULTI);
		case 'L': return (TERM_LITE);

		case 'C': return (TERM_CONF);
		case 'S': return (TERM_SOUN);
		case 'H': return (TERM_SHAR);
		case 'A': return (TERM_DARKNESS);
		case 'M': return (TERM_SHIELDM);
		case 'I': return (TERM_SHIELDI);

		/* maybe TODO: add EXTENDED_TERM_COLOURS here too */
		//case 'E': return TERM_CURSE; //like TERM_DARKNESS
		//case 'N': return TERM_ANNI; //like TERM_DARKNESS
		case 'P': return TERM_PSI;
		case 'x': return TERM_NEXU;
		case 'n': return TERM_NETH;
		case 'T': return TERM_DISE;
		case 'q': return TERM_INER;
		case 'F': return TERM_FORC;
		case 'V': return TERM_GRAV;
		case 't': return TERM_TIME;
		case 'E': return TERM_METEOR;
		case 'N': return TERM_MANA;
		case 'Q': return TERM_DISI;
		case 'Y': return TERM_WATE;
		case 'i': return TERM_ICE; //pretty similar to elec
		case 'l': return TERM_PLAS;
		case 'O': return TERM_DETO;
		case 'k': return TERM_NUKE;
		case 'K': return TERM_UNBREATH; //ugh --pretty similar to pois
		case 'j': return TERM_HOLYORB;
		case 'J': return TERM_HOLYFIRE;
		case 'X': return TERM_HELLFIRE;
		case 'z': return TERM_THUNDER;
		//case '': return TERM_LAMP; //this is just static yellow
		//case '': return TERM_LAMP_DARK; //this is just static l_umber
		case 'Z': return TERM_EMBER;
		case '0': return TERM_STARLITE;
		case '1': return TERM_HAVOC;
	}

	return (-1);
}

/*
 * Convert a color to a color letter.
 * The colors are: dwsorgbuDWvyRGBU, as shown below
 */
char color_attr_to_char(int a) {
	switch (a) {
		case TERM_DARK: return 'd';
		case TERM_WHITE: return 'w';
		case TERM_SLATE: return 's';
		case TERM_ORANGE: return 'o';
		case TERM_RED: return 'r';
		case TERM_GREEN: return 'g';
		case TERM_BLUE: return 'b';
		case TERM_UMBER: return 'u';

		case TERM_L_DARK: return 'D';
		case TERM_L_WHITE: return 'W';
		case TERM_VIOLET: return 'v';
		case TERM_YELLOW: return 'y';
		case TERM_L_RED: return 'R';
		case TERM_L_GREEN: return 'G';
		case TERM_L_BLUE: return 'B';
		case TERM_L_UMBER: return 'U';

		case TERM_HALF: return 'h';
		case TERM_MULTI: return 'm';
		case TERM_POIS: return 'p';
		case TERM_FIRE: return 'f';
		case TERM_ACID: return 'a';
		case TERM_ELEC: return 'e';
		case TERM_COLD: return 'c';
		case TERM_LITE: return 'L';

		case TERM_CONF: return 'C';
		case TERM_SOUN: return 'S';
		case TERM_SHAR: return 'H';
		case TERM_DARKNESS: return 'A';
		case TERM_SHIELDM: return 'M';
		case TERM_SHIELDI: return 'I';

		/* maybe TODO: add EXTENDED_TERM_COLOURS here too */
		//case TERM_CURSE: return 'E'; //like TERM_DARKNESS
		//case TERM_ANNI: return 'N'; //like TERM_DARKNESS
		case TERM_PSI: return 'P';
		case TERM_NEXU: return 'x';
		case TERM_NETH: return 'n';
		case TERM_DISE: return 'T';
		case TERM_INER: return 'q';
		case TERM_FORC: return 'F';
		case TERM_GRAV: return 'V';
		case TERM_TIME: return 't';
		case TERM_METEOR: return 'E';
		case TERM_MANA: return 'N';
		case TERM_DISI: return 'Q';
		case TERM_WATE: return 'Y';
		case TERM_ICE: return 'i';//pretty similar to elec
		case TERM_PLAS: return 'l';
		case TERM_DETO: return 'O';
		case TERM_NUKE: return 'k';
		case TERM_UNBREATH: return 'K';//ugh --pretty similar to pois
		case TERM_HOLYORB: return 'j';
		case TERM_HOLYFIRE: return 'J';
		case TERM_HELLFIRE: return 'X';
		case TERM_THUNDER: return 'z';
		//case TERM_LAMP: return ''; //this is just static yellow
		//case TERM_LAMP_DARK: return ''; //this is just static l_umber
		case TERM_EMBER: return 'Z'; //this was the final free letter ;) no more colour letters available now!
		case TERM_STARLITE: return '0'; //:-p
		case TERM_HAVOC: return '1';
	}

	return 'w';
}

byte mh_attr(int max) {
        switch (randint(max)) {
                case  1: return (TERM_RED);
                case  2: return (TERM_GREEN);
                case  3: return (TERM_BLUE);
                case  4: return (TERM_YELLOW);
                case  5: return (TERM_ORANGE);
                case  6: return (TERM_VIOLET);
                case  7: return (TERM_L_RED);
                case  8: return (TERM_L_GREEN);
                case  9: return (TERM_L_BLUE);
                case 10: return (TERM_UMBER);
                case 11: return (TERM_L_UMBER);
                case 12: return (TERM_SLATE);
                case 13: return (TERM_WHITE);
                case 14: return (TERM_L_WHITE);
                case 15: return (TERM_L_DARK);
        }
        return (TERM_WHITE);
}

/*
 * Create a new path by appending a file (or directory) to a path
 *
 * This requires no special processing on simple machines, except
 * for verifying the size of the filename, but note the ability to
 * bypass the given "path" with certain special file-names.
 *
 * Note that the "file" may actually be a "sub-path", including
 * a path and a file.
 *
 * Note that this function yields a path which must be "parsed"
 * using the "parse" function above.
 */
errr path_build(char *buf, int max, cptr path, cptr file)
{
        /* Special file */
        if (file[0] == '~')
        {
                /* Use the file itself */
                strnfmt(buf, max, "%s", file);
        }

        /* Absolute file, on "normal" systems */
        else if (prefix(file, PATH_SEP) && !streq(PATH_SEP, ""))
        {
                /* Use the file itself */
                strnfmt(buf, max, "%s", file);
        }

        /* No path given */
        else if (!path[0])
        {
                /* Use the file itself */
                strnfmt(buf, max, "%s", file);
        }

        /* Path and File */
        else
        {
                /* Build the new path */
                strnfmt(buf, max, "%s%s%s", path, PATH_SEP, file);
        }

        /* Success */
        return (0);
}

/*
 * version strings
 */
cptr longVersion;
cptr shortVersion;

void version_build()
{
	char temp[256];

	/* Append the version number */
	sprintf(temp, "%s %d.%d.%d.%d%s", TOMENET_VERSION_SHORT, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_EXTRA, SERVER_VERSION_TAG);

#if 0
	/* Append the additional version info */
	if (VERSION_EXTRA == 1)
		strcat(temp, "-alpha");
	else if (VERSION_EXTRA == 2)
		strcat(temp, "-beta");
	else if (VERSION_EXTRA == 3)
		strcat(temp, "-development");
	else if (VERSION_EXTRA)
		strcat(temp, format(".%d", VERSION_EXTRA));
#endif

	shortVersion = string_make(temp);

	/* Append the date of build */
	strcat(temp, format(" (Compiled %s %s)", __DATE__, __TIME__));

	longVersion = string_make(temp);
}


/* Hrm, maybe we'd better prepare common/skills.c or sth? */
/* Find the realm, given a book(tval) */
int find_realm(int book)
{
        switch (book)
        {
        case TV_MAGIC_BOOK:
                return REALM_MAGERY;
        case TV_PRAYER_BOOK:
                return REALM_PRAYER;
        case TV_SORCERY_BOOK:
                return REALM_SORCERY;
        case TV_FIGHT_BOOK:
                return REALM_FIGHTING;
        case TV_SHADOW_BOOK:
                return REALM_SHADOW;
        case TV_PSI_BOOK:
                return REALM_PSI;
        case TV_HUNT_BOOK:
                return REALM_HUNT;
        };
        return -1;
}

/* strcasestr() is only defined in _GNU_SOURCE, so we need our own implementation to be safe */
const char *my_strcasestr(const char *big, const char *little) {
	const char *ret = NULL;
	int cnt = 0, cnt2 = 0;
	int L = strlen(little), l;

	if (little == NULL) return big;

	do {
		cnt2 = 0;
		l = 0;
		while (little[cnt2] != 0) {
			if (big[cnt + cnt2] == little[cnt2] || big[cnt + cnt2] == tolower(little[cnt2]) || big[cnt + cnt2] == toupper(little[cnt2])) l++;
			else break;

			if (l == 1) ret = big + cnt;

			cnt2++;
		}

		if (L == l) return ret;
		cnt++;
	} while (big[cnt] != '\0');
	return NULL;
}
