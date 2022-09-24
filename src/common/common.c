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
		case 'Z': return TERM_EMBER;
		case '0': return TERM_STARLITE;
		case '1': return TERM_HAVOC;
		case '2': return TERM_LAMP;
		case '3': return TERM_LAMP_DARK;
		case '4': return TERM_SELECTOR;
		case '5': return TERM_SMOOTHPAL;
		case '6': return TERM_SEL_RED;
		case '7': return TERM_SEL_BLUE;
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
		case TERM_EMBER: return 'Z'; //this was the final free letter ;) no more colour letters available now!
		case TERM_STARLITE: return '0'; //:-p
		case TERM_HAVOC: return '1';
		case TERM_LAMP: return '2';
		case TERM_LAMP_DARK: return '3';
		case TERM_SELECTOR: return '4';
		case TERM_SMOOTHPAL: return '5';
		case TERM_SEL_RED: return '6';
		case TERM_SEL_BLUE: return '7';
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
errr path_build(char *buf, int max, cptr path, cptr file) {
	/* Special file */
	if (file[0] == '~') {
		/* Use the file itself */
		strnfmt(buf, max, "%s", file);
	}

	/* Absolute file, on "normal" systems */
	else if (prefix(file, PATH_SEP) && !streq(PATH_SEP, "")) {
		/* Use the file itself */
		strnfmt(buf, max, "%s", file);
	}

	/* No path given */
	else if (!path[0]) {
		/* Use the file itself */
		strnfmt(buf, max, "%s", file);
	}

	/* Path and File */
	else {
		/* Build the new path */
		strnfmt(buf, max, "%s%s%s", path, PATH_SEP, file);
	}

	/* Success */
	return (0);
}

/*
 * version strings
 */
cptr longVersion, os_version;
cptr shortVersion;

void version_build() {
	char temp[256], buf[1024];

	int size;
	FILE *fff;

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
	strcat(temp, format(" %s (Compiled %s %s for %s)", is_client_side ? "client" : "server", __DATE__, __TIME__,
#ifdef WINDOWS
	    "WINDOWS"
#else /* Assume POSIX */
	    "POSIX"
#endif
	    ));

	longVersion = string_make(temp);

	/* Get OS version too */
	path_build(buf, 1024, ANGBAND_DIR_USER, "__temp");
#ifndef WINDOWS
	size = system(format("uname -a > %s", buf));
#else
	//(void)system("uname -a"); /* doesn't work on WINE */
	size = system(format("cmd /c ver > %s", buf)); /* safer to work everywhere? even works on WINE at least */
#endif
	(void)size;
	fff = my_fopen(buf, "r");
	if (fff) {
#ifndef WINDOWS
		if (fgets(temp, 256, fff) != temp) os_version = "<empty>";
#else /* ver returns 1 blank line first -_- */
		if (fgets(temp, 256, fff) != temp) os_version = "<empty>";
		else temp[strlen(temp) - 1] = 0; //trim newline
		/* If 1st line is empty, grab 2nd line instead */
		if (!temp[0] && fgets(temp, 256, fff) != temp) os_version = "<empty>";
#endif
		else {
			temp[strlen(temp) - 1] = 0; //trim newline
			os_version = string_make(temp);
		}
		fclose(fff);
	} else os_version = "<unavailable>";
	remove(buf);
}


/* Hrm, maybe we'd better prepare common/skills.c or sth? */
/* Find the realm, given a book(tval) */
int find_realm(int book) {
	switch (book) {
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
char *my_strcasestr(const char *big, const char *little) {
	const char *ret = NULL;
	int cnt = 0, cnt2 = 0;
	int L = strlen(little), l;

	/* para stuff that is necessary */
	if (big == NULL) return NULL; //cannot find anything in a place that doesn't exist
	if (little == NULL) return NULL; //something that doesn't exist, cannot be found.. (was 'return big')
	if (*little == 0) return (char*)big;
	if (*big == 0) return NULL; //at least this one is required, was glitching in-game guide search! oops..

	do {
		cnt2 = 0;
		l = 0;
		while (little[cnt2] != 0) {
			if (big[cnt + cnt2] == little[cnt2] || big[cnt + cnt2] == tolower(little[cnt2]) || big[cnt + cnt2] == toupper(little[cnt2])) l++;
			else break;

			if (l == 1) ret = big + cnt;

			cnt2++;
		}

		if (L == l) return (char*)ret;
		cnt++;
	} while (big[cnt] != '\0');
	return NULL;
}
/* Same as my_strcasestr() but skips colour codes (added for guide search).
   strict:
    0 - not strict
    1 - search for occurances only at the beginning of a line (tolerating colour codes and spaces)
    2 - same as (1) and it must not start on a lower-case letter (to ensure it's not just inside some random text).
    3 - same as (2) and also search only for all-caps (for item flags)
    4 - same as (3) and it must be at the beginning of the line without tolerating spaces, to rule out that we're inside some paragraph already.
   $$: Additionally, ending 'littlex' on "$$" will force it to be matched at the end of a line only. */
char *my_strcasestr_skipcol(const char *big, const char *littlex, byte strict) {
	const char *ret = NULL;
	int cnt = 0, cnt2 = 0, cnt_offset;
	int L, l;
	bool end_of_line;
	char little[MSG_LEN];

	/* para stuff that is necessary */
	if (big == NULL) return NULL; //cannot find anything in a place that doesn't exist
	if (littlex == NULL) return NULL; //something that doesn't exist, cannot be found.. (was 'return big')
	if (*littlex == 0) return (char*)big;
	if (*big == 0) return NULL; //at least this one is required, was glitching in-game guide search! oops..

	strcpy(little, littlex);
	/* Check for '$$' end-of-line marker */
	end_of_line = strlen(little) >= 4 && little[strlen(little) - 1] == '$' && little[strlen(little) - 2] == '$';
	if (end_of_line) little[strlen(little) - 2] = 0;
	L = strlen(little);

	if (strict) { /* switch to strict mode */
		bool just_spaces = (strict == 4 ? FALSE : TRUE);
		do {
			/* Skip colour codes */
			while (big[cnt] == '\377') {
				cnt++;
				if (big[cnt] != 0) cnt++; //paranoia: broken colour code
			}
			if (!big[cnt]) return NULL;
			if (big[cnt] != ' ') just_spaces = FALSE;

			/* Should not start on a lower-case letter, so we know we're not just in the middle of some random text.. */
			if (strict >= 2 && isalpha(big[cnt]) && big[cnt] == tolower(big[cnt])) return NULL;

			cnt2 = cnt_offset = 0;
			l = 0;
			while (little[cnt2] != 0) {
				/* Skip colour codes */
				while (big[cnt + cnt2 + cnt_offset] == '\377') {
					cnt_offset++;
					if (big[cnt + cnt2 + cnt_offset] != 0) cnt_offset++; //paranoia: broken colour code
				}
				if (!big[cnt + cnt2 + cnt_offset]) return NULL;

				if (strict >= 3) { /* Case-sensitive: Caps only (the needle is actually all-caps) */
					if (big[cnt + cnt2 + cnt_offset] == little[cnt2]) l++;
					else break;
				} else {
					if (big[cnt + cnt2 + cnt_offset] == little[cnt2] || big[cnt + cnt2 + cnt_offset] == tolower(little[cnt2]) || big[cnt + cnt2 + cnt_offset] == toupper(little[cnt2])) l++;
					else break;
				}

				if (l == 1) ret = big + cnt;
				cnt2++;
			}

			if (L == l) {
#if 1 /* Enable '$' force-end-of-line marker */
				if (end_of_line && big[cnt + cnt2 + cnt_offset]) {
					cnt++;
					if (!big[cnt]) return NULL;
					continue;
				}
#endif
				return (char*)ret;
			}
			if (!just_spaces) return NULL; /* failure: not at the beginning of the line (tolerating colour codes) */
			cnt++;
		} while (big[cnt] != '\0');
		return NULL;
	} else {
		do {
			/* Skip colour codes */
			while (big[cnt] == '\377') {
				cnt++;
				if (big[cnt] != 0) cnt++; //paranoia: broken colour code
			}

			cnt2 = cnt_offset = 0;
			l = 0;
			while (little[cnt2] != 0) {
				/* Skip colour codes */
				while (big[cnt + cnt2 + cnt_offset] == '\377') {
					cnt_offset++;
					if (big[cnt + cnt2 + cnt_offset] != 0) cnt_offset++; //paranoia: broken colour code
				}

				if (big[cnt + cnt2 + cnt_offset] == little[cnt2] || big[cnt + cnt2 + cnt_offset] == tolower(little[cnt2]) || big[cnt + cnt2 + cnt_offset] == toupper(little[cnt2])) l++;
				else break;

				if (l == 1) ret = big + cnt;
				cnt2++;
			}

			if (L == l) return (char*)ret;
			cnt++;
		} while (big[cnt] != '\0');
		return NULL;
	}
}

/* Provide extra convenience for character names that end on roman numbers to count their reincarnations,
   for highlighting/beeping on personal chat messages, specifically allowing the sender to omit the roman number
   at the end of our character name, assuming it is separated by at least one space;
   also for 'similar names' checks regarding choosing a character name on character creation!
   Note that this function also verifies the actual validity of the roman numbers.. :-p - C. Blue */
char *roman_suffix(char* cname) {
	char *p, *p0;
	int arabic = 0, rome_prev = 0;
	bool maybe_prefix = FALSE;

	/* Not long enough to contain roman number? */
	if (strlen(cname) < 3) return NULL;
	/* No separator between name and number? */
	if (!strchr(cname, ' ')) return NULL;

	/* Locate begin of the roman number */
	p = cname + strlen(cname) - 1;
	while (*p != ' ') p--;
	p0 = p + 1;

	/* Extract and validate roman number */
	while (*(++p)) {
		switch (*p) {
		case 'I':
			if (arabic % 5 >= 3) return NULL; //this digit may never occur more than 3 times in a row

			if (maybe_prefix) arabic += rome_prev; //prefix was not a prefix but a normal digit ('I' cannot get a prefix)
			if (arabic % 5 == 0) maybe_prefix = TRUE;
			else {
				arabic++;
				maybe_prefix = FALSE;
			}
			rome_prev = 1;
			break;
		case 'V':
			if (rome_prev) {
				if (rome_prev == 5) return NULL; //this specific digit is never allowed to repeat
				if (rome_prev < 5 && !maybe_prefix) return NULL; //smaller digits before us are only allowed if they can be a subtractive prefix to us
			}

			if (maybe_prefix) { //was prefix a prefix or a normal digit?
				if (rome_prev == 1) arabic--;
				else arabic += rome_prev;
			}
			arabic += 5;
			maybe_prefix = FALSE;
			rome_prev = 5;
			break;
		case 'X':
			if (arabic % 50 >= 30) return NULL; //this digit may never occur more than 3 times in a row
			if (rome_prev) {
				if (rome_prev < 10 && !maybe_prefix) return NULL; //smaller digits before us are only allowed if they can be a subtractive prefix to us
			}

			if (maybe_prefix) { //was prefix a prefix or a normal digit?
				if (rome_prev == 1) arabic--;
				else arabic += rome_prev;
			}
			if (arabic % 50 == 0) maybe_prefix = TRUE;
			else {
				arabic += 10;
				maybe_prefix = FALSE;
			}
			rome_prev = 10;
			break;
		case 'L':
			if (rome_prev) {
				if (rome_prev == 50) return NULL; //this specific digit is never allowed to repeat
				if (rome_prev < 50 && !maybe_prefix) return NULL; //smaller digits before us are only allowed if they can be a subtractive prefix to us
				if (rome_prev < 10) return NULL; //catch previous digits that can be prefix but are too small to work as prefix for the current digit
			}

			if (maybe_prefix) { //was prefix a prefix or a normal digit?
				if (rome_prev == 10) arabic -= 10;
				else arabic += rome_prev;
			}
			arabic += 50;
			maybe_prefix = FALSE;
			rome_prev = 50;
			break;
		case 'C':
			if (arabic % 500 >= 300) return NULL; //this digit may never occur more than 3 times in a row
			if (rome_prev) {
				if (rome_prev < 100 && !maybe_prefix) return NULL; //smaller digits before us are only allowed if they can be a subtractive prefix to us
				if (rome_prev < 10) return NULL; //catch previous digits that can be prefix but are too small to work as prefix for the current digit
			}

			if (maybe_prefix) { //was prefix a prefix or a normal digit?
				if (rome_prev == 10) arabic -= 10;
				else arabic += rome_prev;
			}
			if (arabic % 500 == 0) maybe_prefix = TRUE;
			else {
				arabic += 100;
				maybe_prefix = FALSE;
			}
			rome_prev = 100;
			break;
		case 'D':
			if (rome_prev) {
				if (rome_prev == 500) return NULL; //this specific digit is never allowed to repeat
				if (rome_prev < 500 && !maybe_prefix) return NULL; //smaller digits before us are only allowed if they can be a subtractive prefix to us
				if (rome_prev < 100) return NULL; //catch previous digits that can be prefix but are too small to work as prefix for the current digit
			}

			if (maybe_prefix) { //was prefix a prefix or a normal digit?
				if (rome_prev == 100) arabic -= 100;
				else arabic += rome_prev;
			}
			arabic += 500;
			maybe_prefix = FALSE;
			rome_prev = 500;
			break;
		case 'M':
			// (no limit for amount of Ms occurring in a row..)
			if (rome_prev) {
				if (rome_prev < 1000 && !maybe_prefix) return NULL; //smaller digits before us are only allowed if they can be a subtractive prefix to us
				if (rome_prev < 100) return NULL; //catch previous digits that can be prefix but are too small to work as prefix for the current digit
			}

			if (maybe_prefix) { //was prefix a prefix or a normal digit?
				if (rome_prev == 100) arabic -= 100;
				//else arabic += rome_prev; //not needed because of 'irregular' below: maybe_prefix can never be set to true from an 'M', so we don't need to catch that case <-here.
			}
			arabic += 1000;
			maybe_prefix = FALSE; //irregular ;) simply as it's the highest roman number..
			rome_prev = 1000;
			break;
		default:
			/* Other letters do not belong into roman numbers */
			return NULL;
		}
	}

	/* Success, return starting position of the roman number */
	return p0;
}

/*
 * Check if the server/client version fills the requirements.
 *
 * Branch has to be an exact match.
 */
bool is_older_than(version_type *version, int major, int minor, int patch, int extra, int branch, int build) {
	if (version->major > major)
		return FALSE; /* very new */
	else if (version->major < major)
		return TRUE; /* very old */
	else if (version->minor > minor)
		return FALSE; /* pretty new */
	else if (version->minor < minor)
		return TRUE; /* pretty old */
	else if (version->patch > patch)
		return FALSE; /* somewhat new */
	else if (version->patch < patch)
		return TRUE; /* somewhat old */
	else if (version->extra > extra)
		return FALSE; /* a little newer */
	else if (version->extra < extra)
		return TRUE; /* a little older */
	/* Check that the branch is an exact match */
	else if (version->branch == branch) {
		/* Now check the build */
		if (version->build > build)
			return FALSE;
		else if (version->build < build)
			return TRUE;
	}

	/* Default */
	return FALSE;
}
bool is_newer_than(version_type *version, int major, int minor, int patch, int extra, int branch, int build) {
//#ifdef ATMOSPHERIC_INTRO /* only defined client-side, so commented out is simpler for now */
	/* hack for animating colours before we even have network contact to server */
	if (!version->major) return TRUE;
//#endif

	if (version->major < major)
		return FALSE; /* very old */
	else if (version->major > major)
		return TRUE; /* very new */
	else if (version->minor < minor)
		return FALSE; /* pretty old */
	else if (version->minor > minor)
		return TRUE; /* pretty new */
	else if (version->patch < patch)
		return FALSE; /* somewhat old */
	else if (version->patch > patch)
		return TRUE; /* somewhat new */
	else if (version->extra < extra)
		return FALSE; /* a little older */
	else if (version->extra > extra)
		return TRUE; /* a little newer */
	/* Check that the branch is an exact match */
	else if (version->branch == branch)
	{
		/* Now check the build */
		if (version->build < build)
			return FALSE;
		else if (version->build > build)
			return TRUE;
	}

	/* Default */
	return FALSE;
}
bool is_same_as(version_type *version, int major, int minor, int patch, int extra, int branch, int build) {
	if (version->major == major
	    && version->minor == minor
	    && version->patch == patch
	    && version->extra == extra
	    && version->branch == branch
	    && version->build == build)
		return TRUE;

	return FALSE;
}

/*
 * Hack -- determine if an item is "wearable" (or a missile)
 */
bool wearable_p(object_type *o_ptr) {
	/* Valid "tval" codes */
	switch (o_ptr->tval) {
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
	case TV_BOOMERANG:
	case TV_DIGGING:
	case TV_BLUNT:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
	case TV_LITE:
	case TV_AMULET:
	case TV_RING:
	case TV_AXE:
	case TV_MSTAFF:
	case TV_TOOL:
		return (TRUE);
	}

	/* Nope */
	return (FALSE);
}

/* Very simple linked list for storing character redefinition mapping information. */
/* Replace with better dictionary implementation (hashed, tsearch, ...) if speed is what you want. ;) */

/* Inserts key, value pair into dictionary or overwrites previous key if found. */
struct u32b_char_dict_t *u32b_char_dict_set(struct u32b_char_dict_t *start, uint32_t key, char value) {
	if (start == NULL) {
		start=(struct u32b_char_dict_t *)malloc(sizeof(struct u32b_char_dict_t));
		start->key=key;
		start->value=value;
		start->next=NULL;
		return start;
	}

	for (struct u32b_char_dict_t *cur = start;; cur=cur->next) {
		if (key == cur->key) {
			cur->value = value;
			return start;
		}
		if (cur->next == NULL) {
			cur->next=(struct u32b_char_dict_t *)malloc(sizeof(struct u32b_char_dict_t));
			cur->next->key=key;
			cur->next->value=value;
			cur->next->next=NULL;
			return start;
		}
	}
}

/* Returns not NULL pointer to char that is found uder key, NULL if not found. */
char *u32b_char_dict_get(struct u32b_char_dict_t *start, uint32_t key) {
	if (start == NULL) return NULL;

	for (struct u32b_char_dict_t *cur=start;cur!=NULL;cur=cur->next) {
		if (key == cur->key) {
			return &cur->value;
		}
	}
	return NULL;
}

/* Removes key, value pair from dictionary found under key. */
struct u32b_char_dict_t *u32b_char_dict_unset(struct u32b_char_dict_t *start, uint32_t key) {
	if (start == NULL) return start;

	struct u32b_char_dict_t *prev = NULL;
	struct u32b_char_dict_t *curr = start;
	for (; curr != NULL; curr = curr->next) {
		if (key == curr->key) break;
		prev = curr;
	}

	if (curr == NULL) return start;

	/* Found key */
	struct u32b_char_dict_t *next = curr->next;
	free(curr);

	/* If the key was at start, return next. */
	if (prev == NULL) return next;

	/* The key was is between prev & next, chain them. */
	prev->next = next;
	return start;
}

/* Remove all entries at once. */
struct u32b_char_dict_t *u32b_char_dict_free(struct u32b_char_dict_t *start) {
	while (start != NULL) {
		struct u32b_char_dict_t* next = start->next;
		free(start);
		start=next;
	}
	return NULL;
}

/* Validates provided screen dimensions. If the input dimensions are invalid, they will be changed to valid dimensions.
 * In this case the 'width' and 'height' is set to nearest lower valid value and if there is no such one, than to nearest higher valid value. */
void validate_screen_dimensions(s16b *width, s16b *height) {
	s16b wid = *width, hgt = *height;
#ifdef BIG_MAP
	if (wid > MAX_SCREEN_WID) wid = MAX_SCREEN_WID;
	if (wid < MIN_SCREEN_WID) wid = MIN_SCREEN_WID;
	if (hgt > MAX_SCREEN_HGT) hgt = MAX_SCREEN_HGT;
	if (hgt < MIN_SCREEN_HGT) hgt = MIN_SCREEN_HGT;
	/* for now until resolved: avoid dimensions whose half values aren't divisors of MAX_WID/HGT */
	if (MAX_WID % (wid / 2)) wid = SCREEN_WID;
	if (MAX_HGT % (hgt / 2)) hgt = SCREEN_HGT;
#else
	wid = SCREEN_WID;
	hgt = SCREEN_HGT;
#endif
	(*width) = wid;
	(*height) = hgt;
}

/*
 * Approximate Distance between two points.
 *
 * When either the X or Y component dwarfs the other component,
 * this function is almost perfect, and otherwise, it tends to
 * over-estimate about one grid per fifteen grids of distance.
 *
 * Algorithm: hypot(dy,dx) = max(dy,dx) + min(dy,dx) / 2
 */
/*
 * For radius-affecting things, consider using tdx[], tdy[], tdi[] instead,
 * which contain pre-calculated results of this function.		- Jir -
 * (Please see prepare_distance() )
 */
int distance(int y1, int x1, int y2, int x2) { /* Note: This is currently only used server-side and did reside in cave.c. Could be moved back. - C. Blue */
	int dy, dx, d;

	/* Find the absolute y/x distance components */
	dy = (y1 > y2) ? (y1 - y2) : (y2 - y1);
	dx = (x1 > x2) ? (x1 - x2) : (x2 - x1);

	/* Hack -- approximate the distance */
	d = (dy > dx) ? (dy + (dx>>1)) : (dx + (dy>>1));

	/* Return the distance */
	return (d);
}
