/* $Id$ */
/*
 * Handle client-side files, such as the .mangrc configuration
 * file, and some various "pref files".
 */

#include "angband.h"

#include <sys/types.h>
#include <dirent.h>

#ifdef REGEX_SEARCH
 #include <regex.h>
#endif


/* Does WINDOWS client use the user's home folder instead of the TomeNET folder for 'scpt' and 'user' subfolders?
   This may be required in Windows 7 and higher, where access rights could cause problems when writing to these folders. - C. Blue */
#define WINDOWS_USER_HOME

/* Always map a custom font/tileset definition even if it's omitted, as omitted just means 'zero' aka 'use the standard colour' anyway.
   This is important for the specific case when a custom font AND a graphical tileset are picked at the same time,
   in which case the tileset should have the last say. Now if the font already defined an attr but the tileset wants to use the standard attr instead,
   then the attr must be cleared again to zero ie standard, when the omitted-attr line is read from the tileset definition prf file.

   So in other words the only effect this has is to sort out priority of graphical tileset definitions over font defininitions if both compete,
   in which case the tileset will win as it is loaded after the font. - C. Blue*/
#define CUSTOM_MAPPING_ALWAYS



static int MACRO_WAIT = 96; //hack: see c-util.c and keep consistent
static int MACRO_XWAIT = 30; //hack: see c-util.c and keep consistent

/*
 * Extract the first few "tokens" from a buffer
 *
 * This function uses "colon" and "slash" as the delimeter characters.
 *
 * We never extract more than "num" tokens.  The "last" token may include
 * "delimeter" characters, allowing the buffer to include a "string" token.
 *
 * We save pointers to the tokens in "tokens", and return the number found.
 *
 * Hack -- Attempt to handle the 'c' character formalism
 *
 * Hack -- An empty buffer, or a final delimeter, yields an "empty" token.
 *
 * Hack -- We will always extract at least one token
 */
static s16b tokenize(char *buf, s16b num, char **tokens) {
	int i = 0;
	char *s = buf;

	/* Process */
	while (i < num - 1) {
		char *t;

		/* Scan the string */
		for (t = s; *t; t++) {
			/* Found a delimiter */
			if ((*t == ':') || (*t == '/')) break;

			/* Handle single quotes */
			if (*t == '\'') {
				/* Advance */
				t++;

				/* Handle backslash */
				if (*t == '\\') t++;

				/* Require a character */
				if (!*t) break;

				/* Advance */
				t++;

				/* Hack -- Require a close quote */
				if (*t != '\'') *t = '\'';
			}

			/* Handle back-slash */
			if (*t == '\\') t++;
		}

		/* Nothing left */
		if (!*t) break;

		/* Nuke and advance */
		*t++ = '\0';

		/* Save the token */
		tokens[i++] = s;

		/* Advance */
		s = t;
	}

	/* Save the token */
	tokens[i++] = s;

	/* Number found */
	return(i);
}



/*
 * Convert a octal-digit into a decimal
 */
static int deoct(char c) {
	if (isdigit(c)) return(D2I(c));
	return(0);
}

/*
 * Convert a hexidecimal-digit into a decimal
 */
static int dehex(char c) {
	if (isdigit(c)) return(D2I(c));
	if (islower(c)) return(A2I(c) + 10);
	if (isupper(c)) return(A2I(tolower(c)) + 10);
	return(0);
}


/*
 * Hack -- convert a printable string into real ascii
 *
 * I have no clue if this function correctly handles, for example,
 * parsing "\xFF" into a (signed) char.  Whoever thought of making
 * the "sign" of a "char" undefined is a complete moron.  Oh well.
 */
void text_to_ascii(char *buf, cptr str) {
	char *s = buf;

	/* Analyze the "ascii" string */
	while (*str) {
		/* Backslash codes */
		if (*str == '\\') {
			/* Skip the backslash */
			str++;

			/* Hex-mode XXX */
			if (*str == 'x') {
				*s = 16 * dehex(*++str);
				*s++ += dehex(*++str);
			}

			/* Specialty: Asynchronous delay for usage in complex macros - C. Blue */
			else if (*str == 'w') *s++ = MACRO_WAIT;
			/* Specialty: Asynchronous delay for usage in complex macros - C. Blue */
			else if (*str == 'W') *s++ = MACRO_XWAIT;

			/* Hack -- simple way to specify "backslash" */
			else if (*str == '\\') *s++ = '\\';

			/* Hack -- simple way to specify "caret" */
			else if (*str == '^') *s++ = '^';

			/* Hack -- simple way to specify "space" */
			else if (*str == 's') *s++ = ' ';

			/* Hack -- simple way to specify Escape */
			else if (*str == 'e') *s++ = ESCAPE;

			/* Backspace */
			else if (*str == 'b') *s++ = '\b';

			/* Newline */
			else if (*str == 'n') *s++ = '\n';

			/* Return */
			else if (*str == 'r') *s++ = '\r';

			/* Tab */
			else if (*str == 't') *s++ = '\t';

			/* Octal-mode */
			else if (*str == '0') {
				*s = 8 * deoct(*++str);
				*s++ += deoct(*++str);
			}

			/* Octal-mode */
			else if (*str == '1') {
				*s = 64 + 8 * deoct(*++str);
				*s++ += deoct(*++str);
			}

			/* Octal-mode */
			else if (*str == '2') {
				*s = 64 * 2 + 8 * deoct(*++str);
				*s++ += deoct(*++str);
			}

			/* Octal-mode */
			else if (*str == '3') {
				*s = 64 * 3 + 8 * deoct(*++str);
				*s++ += deoct(*++str);
			}

			/* Skip the final char */
			str++;
		}

		/* Normal Control codes */
		else if (*str == '^') {
			str++;
			*s++ = (*str++ & 037);
		}

		/* Normal chars */
		else *s++ = *str++;
	}

	/* Terminate */
	*s = '\0';
}

/*
 * Extract a "parsed" path from an initial filename
 * Normally, we simply copy the filename into the buffer
 * But leading tilde symbols must be handled in a special way
 * Replace "~user/" by the home directory of the user named "user"
 * Replace "~/" by the home directory of the current user
 */
static errr path_parse(char *buf, cptr file) {
#ifndef WIN32
#ifndef AMIGA
	cptr	    u, s;
	struct passwd   *pw;
	char	    user[128];
#endif
#endif /* WIN32 */


	/* Assume no result */
	buf[0] = '\0';

	/* No file? */
	if (!file) return(-1);

	/* File needs no parsing */
	if (file[0] != '~') {
		strcpy(buf, file);
		return(0);
	}

	/* Windows should never have ~ in filename */
#ifndef WIN32
#ifndef AMIGA

	/* Point at the user */
	u = file + 1;

	/* Look for non-user portion of the file */
	s = strstr(u, PATH_SEP);

	/* Hack -- no long user names */
	if (s && (s >= u + sizeof(user))) return(1);

	/* Extract a user name */
	if (s) {
		int i;

		for (i = 0; u < s; ++i) user[i] = *u++;
		user[i] = '\0';
		u = user;
	}

	/* Look up the "current" user */
	if (u[0] == '\0') u = getlogin();

	/* Look up a user (or "current" user) */
	if (u) pw = getpwnam(u);
	else pw = getpwuid(getuid());

	/* Nothing found? */
	if (!pw) return(2);

	/* Make use of the info */
	(void)strcpy(buf, pw->pw_dir);

	/* Append the rest of the filename, if any */
	if (s) (void)strcat(buf, s);

#endif
#endif /* WIN32 */
	/* Success */
	return(0);
}



/*
 * Hack -- replacement for "fopen()"
 */
FILE *my_fopen(cptr file, cptr mode) {
	char		buf[1024];
	int err;

	/* Hack -- Try to parse the path */
	if ((err = path_parse(buf, file))) {
		errno = 900 + err;
		return(NULL);
	}

	/* Attempt to fopen the file anyway */
	return(fopen(buf, mode));
}


/*
 * Hack -- replacement for "fclose()"
 */
errr my_fclose(FILE *fff) {
	/* Require a file */
	if (!fff) return(-1);

	/* Close, check for error */
	if (fclose(fff) == EOF) return(1);

	/* Success */
	return(0);
}

/*
 * MetaHack -- check if the specified file already exists	- Jir -
 */
bool my_freadable(cptr file) {
	FILE *fff;
	fff = my_fopen(file, "rb");

	if (fff) return(FALSE);

	my_fclose(fff);
	return(TRUE);
}

/*
 * Hack -- replacement for "fgets()"
 *
 * Read a string, without a newline, to a file
 *
 * Process tabs, strip internal non-printables
 */
errr my_fgets(FILE *fff, char *buf, huge n) {
	huge i = 0;
	char *s;
	char tmp[1024];

	/* Read a line */
	if (fgets(tmp, 1024, fff)) {
		/* Convert weirdness */
		for (s = tmp; *s; s++) {
			/* Handle newline */
			if (*s == '\n') {
				/* Terminate */
				buf[i] = '\0';

				/* Success */
				return(0);
			}

			/* Handle tabs */
			else if (*s == '\t') {
				/* Hack -- require room */
				if (i + 8 >= n) break;

				/* Append a space */
				buf[i++] = ' ';

				/* Append some more spaces */
				while (!(i % 8)) buf[i++] = ' ';
			}

			/* Handle printables */
			else if (isprint(*s)) {
				/* Copy */
				buf[i++] = *s;

				/* Check length */
				if (i >= n) break;
			}
		}
	}

	/* Nothing */
	buf[0] = '\0';

	/* Failure */
	return(1);
}

/*
 * Custom replacement function for the dreadfully slow fgetc.
 * Can be used on only one file at a time. Files must be read to the end.
 * - mikaelh
 */
static char my_fgetc_buf[4096];
static FILE *my_fgetc_fp;
static long my_fgetc_pos = 4096, my_fgetc_len = 0;
static int my_fgetc(FILE *fff) {
	/* Check if the file has changed */
	if (my_fgetc_fp != fff) {
		if (my_fgetc_fp) {
			/* Rewind the old file a bit */
			fseek(my_fgetc_fp, my_fgetc_pos - my_fgetc_len, SEEK_CUR);
		}

		/* Reset */
		my_fgetc_pos = 4096;
		my_fgetc_len = 0;

		my_fgetc_fp = fff;
	}

	if (my_fgetc_pos >= 4096) {
		/* Fill the buffer */
		my_fgetc_len = fread(my_fgetc_buf, 1, 4096, fff);
		my_fgetc_pos = 0;
	}

	if (my_fgetc_pos < my_fgetc_len) {
		return(my_fgetc_buf[my_fgetc_pos++]);
	} else {
		/* Reset */
		my_fgetc_pos = 4096;
		my_fgetc_len = 0;
		my_fgetc_fp = NULL;

		/* Return EOF */
		return(EOF);
	}
}

/*
 * Return the next character without incrementing the internal counter.
 * - mikaelh
 */
static int my_fpeekc(FILE *fff) {
	if (my_fgetc_pos >= 4096) {
		/* Fill the buffer */
		my_fgetc_len = fread(my_fgetc_buf, 1, 4096, fff);
		my_fgetc_pos = 0;
	}

	if (my_fgetc_pos < my_fgetc_len) {
		return my_fgetc_buf[my_fgetc_pos];
	} else {
		/* Return EOF */
		return EOF;
	}
}

/*
 * A custom function for reading arbitarily long lines. The function allocates
 * memory as necessary to accommodate the line. *line will be set to point to
 * the buffer allocated for the line. The caller is responsible for freeing
 * the allocated memory.
 * - mikaelh
 */
errr my_fgets2(FILE *fff, char **line, int *n, byte *fmt) {
	int c;
	int done = FALSE;
	long len = 0;
	long alloc = 4096;
	char *buf, *tmp;

	*fmt = OS_UNKNOWN;

	/* Allocate some memory for the line */
	if ((tmp = buf = mem_alloc(alloc)) == NULL) {
		/* Set the pointer to NULL and count to zero */
		*line = NULL;
		*n = 0;

		/* Grave error */
		return 2;
	}

	while (TRUE) {
		c = my_fgetc(fff);

		switch (c) {
		/* Handle EOF */
		case EOF: {
			/* Terminate */
			buf[len] = '\0';

			/* Check if nothing has been read */
			if (len == 0) {
				/* Free the memory */
				mem_free(buf);

				/* Set the pointer to NULL and count to zero */
				*line = NULL;
				*n = 0;

				/* Return 1 */
				return(1);
			}

			/* Done */
			done = TRUE;
			break;
		}

		/* Handle newline */
		case '\n':
		case '\r': {
			int c2;

			/* Peek at the next character to eliminate a possible \n */
			c2 = my_fpeekc(fff);

			if (c2 == '\n') {
				/* Skip the \n */
				my_fgetc(fff);
				/* File seems to be in 'DOS format' */
				if (c == '\r') *fmt = OS_WIN32;
			}

			/* Terminate */
			buf[len] = '\0';

			/* Done */
			done = TRUE;
			break;
		}

		/* Handle tabs */
		case '\t': {
			int i;

			/* Make sure that we have enough space */
			if (len + 8 > alloc) {
				buf = mem_realloc(buf, alloc + 4096);
				alloc += 4096;

				if (buf == NULL) {
					/* Free the old memory */
					mem_free(tmp);

					/* Set the pointer to NULL and count to zero */
					*line = NULL;
					*n = 0;

					/* Grave error */
					return 2;
				}
				tmp = buf;
			}

			/* Add 8 spaces */
			for (i = 0; i < 8; i++) buf[len++] = ' ';

			break;
		}

		/* Handle printables */
		default:
			if (isprint(c)) buf[len++] = c;
			break;
		}

		if (done) break;

		/* Make sure we have enough space for at least one more character */
		if (len + 1 > alloc) {
			buf = mem_realloc(buf, alloc + 4096);
			alloc += 4096;

			if (buf == NULL) {
				/* Free the old memory */
				mem_free(tmp);

				/* Set the pointer to NULL and count to zero */
				*line = NULL;
				*n = 0;

				/* Grave error */
				return 2;
			}
			tmp = buf;
		}
	}

	/* Final result */
	*line = buf;
	*n = len + 1;

	/* Success */
	return(0);
}


/*
 * Find the default paths to all of our important sub-directories.
 *
 * The purpose of each sub-directory is described in "variable.c".
 *
 * All of the sub-directories should, by default, be located inside
 * the main "lib" directory, whose location is very system dependant.
 *
 * This function takes a writable buffer, initially containing the
 * "path" to the "lib" directory, for example, "/pkg/lib/angband/",
 * or a system dependant string, for example, ":lib:".  The buffer
 * must be large enough to contain at least 32 more characters.
 *
 * Various command line options may allow some of the important
 * directories to be changed to user-specified directories, most
 * importantly, the "info" and "user" and "save" directories,
 * but this is done after this function, see "main.c".
 *
 * In general, the initial path should end in the appropriate "PATH_SEP"
 * string.  All of the "sub-directory" paths (created below or supplied
 * by the user) will NOT end in the "PATH_SEP" string, see the special
 * "path_build()" function in "util.c" for more information.
 *
 * Mega-Hack -- support fat raw files under NEXTSTEP, using special
 * "suffixed" directories for the "ANGBAND_DIR_DATA" directory, but
 * requiring the directories to be created by hand by the user.
 *
 * Hack -- first we free all the strings, since this is known
 * to succeed even if the strings have not been allocated yet,
 * as long as the variables start out as "NULL".
 */
void init_file_paths(char *path) {
	char *tail;

	/*** Free everything ***/

	/* Free the main path */
	string_free(ANGBAND_DIR);

	/* Free the sub-paths */
	string_free(ANGBAND_DIR_SCPT);
	string_free(ANGBAND_DIR_TEXT);
	string_free(ANGBAND_DIR_USER);
	string_free(ANGBAND_DIR_XTRA);
	string_free(ANGBAND_DIR_GAME);


	/*** Prepare the "path" ***/

	/* Hack -- save the main directory */
	ANGBAND_DIR = string_make(path);

	/* Prepare to append to the Base Path */
	tail = path + strlen(path);


#ifdef VM
	/*** Use "flat" paths with VM/ESA ***/

	/* Use "blank" path names */
	ANGBAND_DIR_SCPT = string_make("");
	ANGBAND_DIR_TEXT = string_make("");
	ANGBAND_DIR_USER = string_make("");
	ANGBAND_DIR_XTRA = string_make("");
	ANGBAND_DIR_GAME = string_make("");

#else /* VM */

	/*** Build the sub-directory names ***/

	/* Build a path name */
	strcpy(tail, "text");
	ANGBAND_DIR_TEXT = string_make(path);

	/* Build a path name */
	strcpy(tail, "xtra");
	ANGBAND_DIR_XTRA = string_make(path);

	/* Build a path name */
	strcpy(tail, "game");
	ANGBAND_DIR_GAME = string_make(path);

 /* On Windows 7 and higher, users should save their data to their designated user folder,
    or write access problems might occur, especially when overwriting a file! */
 #if !defined(WINDOWS) || !defined(WINDOWS_USER_HOME)
	/* Build a path name */
	strcpy(tail, "scpt");
	ANGBAND_DIR_SCPT = string_make(path);

	/* Build a path name */
	strcpy(tail, "user");
	ANGBAND_DIR_USER = string_make(path);
 #else
	if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
		char buf[1024], *btail, out_val[1024];

		strcpy(buf, getenv("HOMEDRIVE"));
		strcat(buf, getenv("HOMEPATH"));
		btail = buf + strlen(buf);

		/* Set 'scpt' folder */
		strcpy(btail, "\\TomeNET-scpt");
		ANGBAND_DIR_SCPT = string_make(buf);
		/* make sure it exists, just in case the installer didn't create it */
		if (!check_dir(ANGBAND_DIR_SCPT)) {
			mkdir(ANGBAND_DIR_SCPT);
			/* copy over the default files from the installation folder */
			sprintf(out_val, "xcopy /I /E /Q /Y /H /C %s%s \"%s\"", ANGBAND_DIR, "scpt", ANGBAND_DIR_SCPT);
			system(out_val);
		}

		/* Set 'user' folder */
		strcpy(btail, "\\TomeNET-user");
		ANGBAND_DIR_USER = string_make(buf);
		/* make sure it exists, just in case the installer didn't create it */
		if (!check_dir(ANGBAND_DIR_USER)) {
			mkdir(ANGBAND_DIR_USER);
			/* copy over the default files from the installation folder */
			sprintf(out_val, "xcopy /I /E /Q /Y /H /C %s%s \"%s\"", ANGBAND_DIR, "user", ANGBAND_DIR_USER);
			system(out_val);
		}
	} else { /* Fall back */
		strcpy(tail, "scpt");
		ANGBAND_DIR_SCPT = string_make(path);

		strcpy(tail, "user");
		ANGBAND_DIR_USER = string_make(path);
	}
 #endif

		/* terminate lib path again at it's actual location of the 'lib' folder */
		strcpy(tail, ""); /* -> this is ANGBAND_DIR */

#endif /* VM */
}

/* Convert a macro trigger key between Windows and Posix -- not implemented ie currently does nothing */
static void key_autoconvert(char *tmp, byte fmt) {
	//if (fmt == VERSION_OS || fmt == OS_UNKNOWN || VERSION_OS == OS_UNKNOWN) return;
	if (fmt == VERSION_OS || (fmt == OS_UNKNOWN && VERSION_OS != OS_WIN32)) return;

	/* DOS -> Unix */
	else if (fmt == OS_WIN32) {

//key_map_dos_unix[][][]

//c_msg_format("<%s>", tmp);
	}

	/* Unix -> DOS */
	else if (VERSION_OS == OS_WIN32) {

//key_map_dos_unix[][][]

//c_msg_format("<%s>", tmp);
	}
}


/*
 * Parse a sub-file of the "extra info" (format shown below)
 *
 * Each "action" line has an "action symbol" in the first column,
 * followed by a colon, followed by some command specific info,
 * usually in the form of "tokens" separated by colons or slashes.
 *
 * Blank lines, lines starting with white space, and lines starting
 * with pound signs ("#") are ignored (as comments).
 *
 * Note the use of "tokenize()" to allow the use of both colons and
 * slashes as delimeters, while still allowing final tokens which
 * may contain any characters including "delimiters".
 *
 * Note the use of "strtol()" to allow all "integers" to be encoded
 * in decimal, hexidecimal, or octal form.
 *
 * Note that "monster zero" is used for the "player" attr/char, "object
 * zero" will be used for the "stack" attr/char, and "feature zero" is
 * used for the "nothing" attr/char.
 *
 * Parse another file recursively, see below for details
 *   %:<filename>
 *
 * Specify the attr/char values for "monsters" by race index
 *   R:<num>:<a>:<c>
 *
 * Specify the attr/char values for "objects" by kind index
 *   K:<num>:<a>:<c>
 *
 * Specify the attr/char values for "features" by feature index
 *   F:<num>:<a>:<c>
 *
 * Specify the attr/char values for unaware "objects" by kind tval
 *   U:<tv>:<a>:<c>
 *
 * Specify the attr/char values for inventory "objects" by kind tval
 *   E:<tv>:<a>:<c>
 *
 * Define a macro action, given an encoded macro action
 *   A:<str>
 *
 * Create a normal macro, given an encoded macro trigger
 *   P:<str>
 *
 * Create a command macro, given an encoded macro trigger
 *   C:<str>
 *
 * Create a keyset mapping
 *   S:<key>:<key>:<dir>
 *
 * Turn an option off, given its name
 *   X:<str>
 *
 * Turn an option on, given its name
 *   Y:<str>
 *
 * Specify visual information, given an index, and some data
 *   V:<num>:<kv>:<rv>:<gv>:<bv>
 *
 * Specify a use for a subwindow
 *   W:<num>:<use>
 *
 *
 * During char redefinitions (actions RKFU), the char value is increased by
 * "char_map_offset" value. This is to make graphic redefinitions more easier.
 * Only during graphical file parsing the "char_map_offset" has MAX_FONT_CHAR + 1
 * value, otherwise is 0. This will be usefull, if the MAX_FONT_CHAR constant
 * changes, there will be no need to update the graphical .prf files.
 */
errr process_pref_file_aux_aux(char *buf, byte fmt, signed char subtileset) {
	int i, j, k;
	int n1, n2;

	char *zz[16], tmp[1024];

	/* We use our own macro__buf - mikaelh */
	static char *macro__buf = NULL;

	/* Skip "empty" lines */
	if (!buf[0]) return(0);

	/* Skip "blank" lines */
	if (isspace(buf[0])) return(0);

	/* Skip comments */
	if (buf[0] == '#') return(0);

	/* Require "?:*" format */
	if (buf[1] != ':') return(1);

	/* Process "%:<fname>" */
	if (buf[0] == '%') {
		/* Attempt to Process the given file */
		return(process_pref_file(buf + 2));
	}

	/* Necessary hack, otherwise 'options.prf' and 'window.prf' will also be
	   loaded by 'pref.prf' and mess up the options and window flags. */
	if (macro_processing_exclusive)
		switch (buf[0]) {
		case 'A': //macro action
		case 'P': case 'H': case 'C': //normal/hybrid/command macro
		case 'D': //delete macro
			break;
		default: return(0); //just discard all other lines
		}

	/* For all kinds of visual mappings (R/K/F/U/@/Z/r):
	   Allow specifying animation steps and within each (or for a static step w/o animation) allow randomization, eg
	   MAPCHAR:A-Anim1,Rchance1..m%:tile1..m,A-Anim2,Rchance1..n%:tile1..n% etc, or just
	   MAPCHAR:Rcance1..m%:tile1..m for a non-animated mapping that still picks a random tile out of several choices. */

	/* Process "R:<num>:<a>/<c>" -- attr/char for monster races (this should take precedence over 'r:' mappings),
	   todo for questors: "R:Q<qidx>C<cnt>:<a>/<c>" syntax based on rmapcnt and rcharidx, requires change to network protocol too. */
	switch(buf[0]) {
	case 'R':
		if (tokenize(buf + 2, 3, zz) == 3) {
			i = (huge)strtol(zz[0], NULL, 0);
			n1 = strtol(zz[1], NULL, 0);
			n2 = strtol(zz[2], NULL, 0) + char_map_offset;
			if (i >= MAX_R_IDX) return(1);
#ifdef USE_GRAPHICS
			if (!use_graphics)
#endif
				if (n2 > MAX_FONT_CHAR) return(1);
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n1)
#endif
				Client_setup.r_attr[i] = n1;
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n2)
#endif
			{
				Client_setup.r_char[i] = n2;
				monster_mapping_mod = u32b_char_dict_set(monster_mapping_mod, n2, monster_mapping_org[i]);
				c_subtileset[n2] = subtileset;
			}
			return(0);
		}
		break;

	/* Process "K:<num>:<a>/<c>"  -- attr/char for object kinds (should take preference over 'E:' mappings) */
	case 'K':
		if (tokenize(buf + 2, 3, zz) == 3) {
			i = (huge)strtol(zz[0], NULL, 0);
			n1 = strtol(zz[1], NULL, 0);
			n2 = strtol(zz[2], NULL, 0) + char_map_offset;
			if (i >= MAX_K_IDX) return(1);
#ifdef USE_GRAPHICS
			if (!use_graphics)
#endif
				if (n2 > MAX_FONT_CHAR) return(1);
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n1)
#endif
				Client_setup.k_attr[i] = n1;
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n2)
#endif
			{
				Client_setup.k_char[i] = n2;
				c_subtileset[n2] = subtileset;
			}
			return(0);
		}
		break;

	/* Process "F:<num>:<a>/<c>" -- attr/char for terrain features */
	case 'F':
		if (tokenize(buf + 2, 3, zz) == 3) {
			i = (huge)strtol(zz[0], NULL, 0);
			n1 = strtol(zz[1], NULL, 0);
			n2 = strtol(zz[2], NULL, 0) + char_map_offset;
			if (i >= MAX_F_IDX) return(1);
#ifdef USE_GRAPHICS
			if (!use_graphics)
#endif
				if (n2 > MAX_FONT_CHAR) return(1);
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n1)
#endif
				Client_setup.f_attr[i] = n1;
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n2)
#endif
			{
				Client_setup.f_char[i] = n2;
				floor_mapping_mod = u32b_char_dict_set(floor_mapping_mod, n2, floor_mapping_org[i]);
				c_subtileset[n2] = subtileset;

				/* Hacky: Try to catch if a custom font mapping puts anything besides walls on '2' or '127' ie the solid-bar/wall symbols,
				   in which case we assume the font is broken, as these symbols are reserved as 'solid blocks' for UI elements (mainly bars)!
				   Completely hard-coded unfortunately. */
				if ((n2 == FONT_MAP_SOLID_X11 || n2 == FONT_MAP_SOLID_WIN) &&
				    !(i == 26 || i == 28 || (i >= 50 && i <= 63) || (i >= 75 && i <= 78) || i == 95 || (i >= 98 && i <= 100) || i == 177 || (i >= 183 && i <= 185) || (i >= 188 && i <= 194)))
					bad_solid_mapping = TRUE;
			}
			return(0);
		}
		break;

	/* Process "U:<tv>:<a>/<c>" -- attr/char for unaware items */
	case 'U':
		if (tokenize(buf + 2, 3, zz) == 3) {
			j = (huge)strtol(zz[0], NULL, 0);
			n1 = strtol(zz[1], NULL, 0);
			n2 = strtol(zz[2], NULL, 0) + char_map_offset;
			if (j > 100) return(0);
#ifdef USE_GRAPHICS
			if (!use_graphics)
#endif
				if (n2 > MAX_FONT_CHAR) return(1);
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n1)
#endif
				Client_setup.u_attr[j] = n1;
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n2)
#endif
			{
				Client_setup.u_char[j] = n2;
				c_subtileset[n2] = subtileset;
			}
			return(0);
		}
		break;

	/* Process "I:<idx>:<x>:<y>:<width>:<height>" -- arbitrarily sized images. - This is purely client-side mapping, not transmitted to the server.
	   Problem: This prf is read for the first time on client startup _after_ graphical tilesets were loaded!
	            So the auto-scale code in the win/x11 tileset resize functions were unable to scale these yet, so we have to do it here.
	            (On subsequent font size changes after first startup, the auto-scaler code will then do any further rescaling of these.) - C. Blue
	   Note: For now, these images are cleared whenever the screen is refreshed, as they do not register in the character/attribute matrix (screen[]).
	         So they are mostly good for temporary effects eg client-side animations (Casino dice slots). */
	case 'I':
#ifdef USE_GRAPHICS
		if (!use_graphics) return(0); //"accept"

		if (tokenize(buf + 2, 5, zz) == 5) {
			j = (huge)strtol(zz[0], NULL, 0);
			if (j > MAX_TILES_RAWPICT) {
				logprint(format("Exceeding MAX_TILES_RAWPICT (%d).\n", MAX_TILES_RAWPICT));
				return(0);
			}
			tiles_rawpict_org[j].x = strtol(zz[1], NULL, 0);
			tiles_rawpict_org[j].y = strtol(zz[2], NULL, 0);
			tiles_rawpict_org[j].w = strtol(zz[3], NULL, 0);
			tiles_rawpict_org[j].h = strtol(zz[4], NULL, 0);
			tiles_rawpict_org[j].defined = TRUE;
			tiles_rawpict_subtileset[j] = subtileset;

			/* For first client startup: Since we're called here _after_ tileset loading, we have to scale here */
 #if defined(WINDOWS) || defined(USE_X11)
			tiles_rawpict_scale();
 #endif
			return(0);
		}
		break;
#else
		return(0); //just accept and ignore
#endif

	/* Process "r:<monstersymbol>:<a>/<c>" -- attr/char for all monsters whose race translates to a specific ASCII symbol in r_info.txt.
	   Eg: 'r:o:<a>/<c>' -> all orcs, snotlings, snagas etc will translate to specific mapping <a>/<c>. */
	case 'r':
#if 0 //todo: implement
		if (tokenize(buf + 2, 3, zz) == 3) {
			i = (huge)strtol(zz[0], NULL, 0);
			n1 = strtol(zz[1], NULL, 0);
			n2 = strtol(zz[2], NULL, 0) + char_map_offset;
			if (i >= MAX_R_IDX) return(1);
#ifdef USE_GRAPHICS
			if (!use_graphics)
#endif
				if (n2 > MAX_FONT_CHAR) return(1);
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n1)
#endif
				Client_setup.r_attr[i] = n1;
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n2)
#endif
			{
				Client_setup.r_char[i] = n2;
				monster_mapping_mod = u32b_char_dict_set(monster_mapping_mod, n2, monster_mapping_org[i]);
				c_subtileset[n2] = subtileset;
			}
			return(0);
		}
#endif
		break;

	/* Process "@:[<gender>],[<race>],[<class>],[<trait>],[<levelrange>],[<CHPrange>]:<a>/<c>" -- attr/char for player characters:
	   All parameters are optional (which is required as we'd hit 50k+ tiles ^^");
	   gender = m/f, race = RACE_xxx, class = CLASS_xxx, trait = TRAIT_xxx (draconian lineage or enlightened/corrupted maia),
	   levelranges: 0: 1-9, 1: 10-19, 2: 20-29, 3: 30-39, 4: 40-49, 5: 50-59, 6: 60-69, 7: 70-79, 8: 80-89, 9: 90-98, 10: 99+ (admin characters can be 100),
	   CHPrange: -: negative HP aka died, 0-7: same as when ascii '@' turns into a number, F: full HP aka '@' in ascii.
	   TODO maybe: Add a 2nd mask colour for representing the trait? */
	case '@':
#if 0 //todo: implement
		if (tokenize(buf + 2, 3, zz) == 3) {
			j = (huge)strtol(zz[0], NULL, 0);
			n1 = strtol(zz[1], NULL, 0);
			n2 = strtol(zz[2], NULL, 0) + char_map_offset;
			if (j > 100) return(0);
#ifdef USE_GRAPHICS
			if (!use_graphics)
#endif
				if (n2 > MAX_FONT_CHAR) return(1);
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n1)
#endif
				Client_setup.u_attr[j] = n1;
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n2)
#endif
			{
				Client_setup.u_char[j] = n2;
				c_subtileset[n2] = subtileset;
			}
			return(0);
		}
#endif
		break;

	/* Process "Z:<type>,[<subtype>]:<a>/<c>" -- attr/char for effects (both, client-side: weather, server-side: fireworks):
	   type: 0 = rain, 1 = snow, 2 = sandstorm (all client-side),
	         3 = fireworks launch, 4 = fireworks explosion (all server-side);
	   subtype (optional):
	         for types 0/1/2: 0 = no wind, 1 = west wind, 2 = east wind, 3 = strong west wind, 4 = strong east wind.
	NOTE: Currently weather particles and fireworks are implemented as 'K:' lines as a workaround. */
	case 'Z':
#if 0 //todo: implement
		if (tokenize(buf + 2, 3, zz) == 3) {
			j = (huge)strtol(zz[0], NULL, 0);
			n1 = strtol(zz[1], NULL, 0);
			n2 = strtol(zz[2], NULL, 0) + char_map_offset;
			if (j > 100) return(0);
#ifdef USE_GRAPHICS
			if (!use_graphics)
#endif
				if (n2 > MAX_FONT_CHAR) return(1);
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n1)
#endif
				Client_setup.u_attr[j] = n1;
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n2)
#endif
			{
				Client_setup.u_char[j] = n2;
				c_subtileset[n2] = subtileset;
			}
			return(0);
		}
#endif
		break;

	/* Process "E:<tv>:<a>/<c>" -- attr/char for equippy chars (purely tval-based base mapping) */
	case 'E':
		/* Do nothing */
		return(0);
#if 0
		if (tokenize(buf + 2, 3, zz) == 3) {
			j = (byte)strtol(zz[0], NULL, 0) % 128;
			n1 = strtol(zz[1], NULL, 0);
			n2 = strtol(zz[2], NULL, 0);
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n1)
#endif
				tval_to_attr[j] = n1;
#ifndef CUSTOM_MAPPING_ALWAYS
			if (n2)
#endif
			{
				tval_to_char[j] = n2;
				c_subtileset[n2] = subtileset;
			}
			return(0);
		}
#endif
		break;

	/* Process "V:<num>:<kv>:<rv>:<gv>:<bv>" -- visual info.
	   NOTE: Currently, colours are read from .rc/.ini file. (Also, colour index [i}[0] is only used for main-ami.c, where it is always 1). */
	case 'V':
		/* Do nothing */
		return(0);

		if (tokenize(buf + 2, 5, zz) == 5) {
			i = (byte)strtol(zz[0], NULL, 0);
			color_table[i][0] = (byte)strtol(zz[1], NULL, 0);
			color_table[i][1] = (byte)strtol(zz[2], NULL, 0);
			color_table[i][2] = (byte)strtol(zz[3], NULL, 0);
			color_table[i][3] = (byte)strtol(zz[4], NULL, 0);
			return(0);
		}
		break;

	/* Process "A:<str>" -- save an "action" for later */
	case 'A':
		/* Free the previous action */
		if (macro__buf) C_FREE(macro__buf, strlen(macro__buf) + 1, char);

		/* Allocate enough space for the ascii string - mikaelh */
		macro__buf = mem_alloc(strlen(buf));

		text_to_ascii(macro__buf, buf + 2);
		return(0);

	/* Process "P:<str>" -- create normal macro */
	case 'P':
		text_to_ascii(tmp, buf + 2);
		key_autoconvert(tmp, fmt);

		//hack
		if (macro_trigger_exclusive[0] && strcmp(macro_trigger_exclusive, tmp)) return(0);

		macro_add(tmp, macro__buf, FALSE, FALSE);
		return(0);

	/* Process "H:<str>" -- create hybrid macro */
	case 'H':
		text_to_ascii(tmp, buf + 2);
		key_autoconvert(tmp, fmt);

		//hack
		if (macro_trigger_exclusive[0] && strcmp(macro_trigger_exclusive, tmp)) return(0);

		macro_add(tmp, macro__buf, FALSE, TRUE);
		return(0);

	/* Process "C:<str>" -- create command macro */
	case 'C':
		text_to_ascii(tmp, buf + 2);
		key_autoconvert(tmp, fmt);

		//hack
		if (macro_trigger_exclusive[0] && strcmp(macro_trigger_exclusive, tmp)) return(0);

		macro_add(tmp, macro__buf, TRUE, FALSE);
		return(0);

	/* Process "D:<str>" -- delete a macro */
	case 'D':
		text_to_ascii(tmp, buf + 2);

		//hack
		if (macro_trigger_exclusive[0] && strcmp(macro_trigger_exclusive, tmp)) return(0);

		macro_del(tmp);
		return(0);

	/* Process "S:<key>:<key>:<dir>" -- keymap */
	case 'S':
		if (tokenize(buf + 2, 3, zz) == 3) {
			i = strtol(zz[0], NULL, 0) & 0x7F;
			j = strtol(zz[0], NULL, 0) & 0x7F;
			k = strtol(zz[0], NULL, 0) & 0x7F;
			if ((k > 9) || (k == 5)) k = 0;
			keymap_cmds[i] = j;
			keymap_dirs[i] = k;
			return(0);
		}
		break;

	/* Process "X:<str>" -- turn option off */
	case 'X':
		for (i = 0; option_info[i].o_desc; i++) {
			if (option_info[i].o_var &&
			    option_info[i].o_text &&
			    streq(option_info[i].o_text, buf + 2)) {
				(*option_info[i].o_var) = FALSE;
				Client_setup.options[i] = FALSE;
				check_immediate_options(i, FALSE, in_game);
				return(0);
			}
		}
		break;

	/* Process "Y:<str>" -- turn option on */
	case 'Y':
		for (i = 0; option_info[i].o_desc; i++) {
			if (option_info[i].o_var &&
			    option_info[i].o_text &&
			    streq(option_info[i].o_text, buf + 2)) {
				(*option_info[i].o_var) = TRUE;
				Client_setup.options[i] = TRUE;
				check_immediate_options(i, TRUE, in_game);
				return(0);
			}
		}
		break;

	/* Process "W:<num>:<use>" -- specify window action */
	case 'W':
		if (tokenize(buf + 2, 2, zz) == 2) {
			i = (byte)strtol(zz[0], NULL, 0);
			if (i != 0) { /* Main window [0] isn't allowed to display arbitrary stuff, it always shows the main screen */
				window_flag[i] = 1L << ((byte)strtol(zz[1], NULL, 0));
				check_for_playerlist();
			} else c_message_add("\377yInvalid attempt to set window_flag for main window.");
			return(0);
		}
		break;
	}

	/* Failure */
	return(1);
}

/* Helper function to process the "user pref file" with the given complete path+filename 'buf' and filenam 'name' */
/* Warn not for every bad mapping line, but once after all mapping lines were loaded: */
#define BAD_MAPPING_BUNDLE
errr process_pref_file_aux(char *buf, cptr name, bool quiet) {
	FILE *fp;
	int n, err;
	char *buf2, *c, *c2;
	byte fmt;
	//bool errors = FALSE;
	int subtileset = -1;

	fp = my_fopen(buf, "r");
	if (!fp) {
		if (!quiet) {
			if (rl_connection_state == 1) c_message_add(format("\377yCould not open file %s", buf));
			if (strcmp(ANGBAND_SYS, "gcu")) logprint(format("Could not open file %s.\n", buf));
		}
		return(-1);
	}

	/* Recognize from filename part (basename suffix) '#...'.bmp when we're a sub-tileset file
	   (this is somewhat better than passing a parm, because it will also catch user loading it manually).
	   Then remember desired character-specific subtileset file accordingly. */
	/* Are we reading a graphical tileset file? Check whether file name starts on "graphics-" */
	c = buf + strlen(buf);
#ifdef WINDOWS
	while (c >= buf && *c != '\\') c--;
#else
	while (c >= buf && *c != '/') c--;
#endif
	c++;
	if (prefix(c, "graphics-")) {
		/* (We do not check for .bmp extension, not really needed if we already checked for 'graphics' filename prefix) */
		/* check whether it's a valid subtile file */
		if ((c2 = strchr(c, '#')) && atoi(c2 + 1) >= 0 && atoi(c2 + 1) < MAX_SUBFONTS) subtileset = atoi(c2 + 1);
	}

	/* Process the file */
	while (0 == (err = my_fgets2(fp, &buf2, &n, &fmt))) {
		/* Process the line */
		if (process_pref_file_aux_aux(buf2, fmt, subtileset)) {
			/* Useful error message */
			if (rl_connection_state == 1) c_msg_format("\377yError in '%s' parsing '%s'.", buf2, name);
			if (strcmp(ANGBAND_SYS, "gcu")) logprint(format("Error in '%s' parsing '%s'.\n", buf2, name));
			//else if (rl_connection_state != 1) plog_fmt("Error in '%s' parsing '%s'.\n", buf2, name); //too annoying if prf file contains a bunch of outdated options as residue from older game versions
			//errors = TRUE;
		}
#ifndef BAD_MAPPING_BUNDLE /* Warn for every bad mapping-line? */
		if (bad_solid_mapping) {
			if (rl_connection_state == 1) {
				c_msg_format("\377yThe mapping '%s' in '%s' maps non-wall feat", buf2, name);
				c_msg_print("\377y to solid wall symbols (either 2 or 127), indicating that the mapping is broken!");
			}
			if (strcmp(ANGBAND_SYS, "gcu")) logprint(format("The mapping '%s' in '%s' maps non-wall feats to solid wall symbols (either 2 or 127), indicating that the mapping is broken!\n", buf2, name));
			else if (rl_connection_state != 1) plog_fmt("The mapping '%s' in '%s' maps non-wall feats to solid wall symbols (either 2 or 127), indicating that the mapping is broken!\n", buf2, name);
			bad_solid_mapping = FALSE;
		}
#endif
		mem_free(buf2);
	}
#ifdef BAD_MAPPING_BUNDLE /* Warn for every bad mapping-line? */
	if (bad_solid_mapping) {
		if (rl_connection_state == 1) {
			c_msg_format("\377yThe mapping in '%s' maps non-wall feats to", name);
			c_msg_print("\377y solid wall symbols (either 2 or 127), indicating that the mapping is broken!");
		}
		if (strcmp(ANGBAND_SYS, "gcu")) logprint(format("The mapping in '%s' maps non-wall feats to solid wall symbols (either 2 or 127), indicating that the mapping is broken!\n", name));
		else if (rl_connection_state != 1) plog_fmt("The mapping in '%s' maps non-wall feats to solid wall symbols (either 2 or 127), indicating that the mapping is broken!\n", name);
		bad_solid_mapping = FALSE;
	}
#endif
	if (err == 2) {
		if (strcmp(ANGBAND_SYS, "gcu")) logprint(format("Grave error: Couldn't allocate memory when parsing '%s'.\n", name));
		//plog(format("!!! GRAVE ERROR: Couldn't allocate memory when parsing file '%s' !!!\n", name)); //might be deadly if it happens in live game ^^' so instead just:
		c_msg_format("\377R!!! GRAVE ERROR: Couldn't allocate memory when parsing file '%s' !!!", name);
	} else err = 0;

#if 0 /* actually don't do this as it would mess up separate .prf files by putting all currently loaded stuff from any of them into this one file */
	/* Try to auto-fix errors simply by re-saving the file */
	if (errors) {
		char bufb[1024];

		strcpy(bufb, buf);
		strcat(bufb, ".bak");
		rename(buf, bufb);
		//options_dump(buf);
		macros_dump(buf);
		c_msg_print("\377yTried to auto-fix the errors by re-saving the file. Backup made to ");
	}
#endif

	/* Close the file */
	my_fclose(fp);

	/* Success */
	return(err);
}
/*
 * Process the "user pref file" with the given name
 *
 * See the function above for a list of legal "commands".
 */
errr process_pref_file(cptr name) {
	char buf[1024];

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, name);
	if (strcmp(ANGBAND_SYS, "gcu")) logprint(format("Processing prf file '%s'.\n", name)); //in GCU-only client this lands across the curses terminals instead of the console, pointless
	return(process_pref_file_aux(buf, name, TRUE));
}
errr process_pref_file_manual(cptr name) {
	char buf[1024];

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, name);
	if (strcmp(ANGBAND_SYS, "gcu")) logprint(format("Processing prf file '%s'.\n", name)); //in GCU-only client this lands across the curses terminals instead of the console, pointless
	return(process_pref_file_aux(buf, name, FALSE));
}

/* Wrapper to try and load both, the character name based macros
   and the character name + monster form specific macros (if form-specific macros are disabled). */
errr load_charspec_macros(cptr cname) {
	char tmp[MAX_CHARS];
	errr error1;

	sprintf(tmp, "%s.prf", cname);
	error1 = process_pref_file(tmp);

	if (c_cfg.load_form_macros && strcmp(c_p_ptr->body_name, "Player")) {

		sprintf(tmp, "%s%c%s.prf", cname, PRF_BODY_SEPARATOR, c_p_ptr->body_name);
		(void)process_pref_file(tmp);
	}

	return(error1);
}

/*
 * Show the Message of the Day.
 *
 * It is given in the "Setup" info sent by the server.
 */
#ifdef WIN32
extern int usleep(long microSeconds);
#endif
void show_motd(int delay) {
	int i;

	/* Save the old screen */
	Term_save();

	/* Clear the screen */
	Term_clear();

	for (i = 0; i < 23; i++) {
		/* Show each line */
		/* Term_putstr(0, i, -1, TERM_WHITE, &Setup.motd[i * 80]); */
		Term_putstr(0, i, -1, TERM_WHITE, &Setup.motd[i * 120]);
	}

	/* Show it */
	Term_fresh();

	/* Wait a few seconds if needed */
	usleep(delay * 10000L);

	/* Wait for a keypress */
	while (!inkey());	/* nothing -- it saves us from getting disconnected */

	/* Reload the old screen */
	Term_load();
}

/*
 * Peruse a file sent by the server.
 *
 * This is used by the artifact list, the unique list, the player
 * list, *Identify*, and Self Knowledge.
 *
 * It may eventually be used for help file perusal as well.
 */
void peruse_file(void) {
	char k = 0;
	bool guide_hack = FALSE;
	char tmp[80];
	static char srcstr[80] = { 0 };
	bool searching = FALSE, reverse = FALSE;
#ifdef REGEX_SEARCH
	static bool regexp = FALSE;
	bool regexp_ok = is_atleast(&server_version, 4, 9, 0, 0, 0, 0);
#endif
	bool old_inkey_msg, old_inkey_interact_macros = inkey_interact_macros;

	/* Initialize */
	cur_line = 0;
	cur_col = 0;
	special_page_size = 20 + HGT_PLUS; /* assume 'non-odd_line' aka normal page size (vs 21 or --2) */

	inkey_interact_macros = FALSE; //needed because we might be called here while we're waiting inside inkey() call in c-cmd.c, which just had this TRUE'd

	/* Save the old screen */
	Term_save();
	perusing = TRUE;

	/* Clear the terminal now */
	Term_clear();

	/* Show the stuff */
	while (TRUE) {
		/* Clear the screen */
		/* hack: no need for full tearm clearing if we only updated 'max_line'.
		   purpose: avoid the extra flickering when 'max_line' arrives over the network. */
		/* This causes flickering so I removed it - mikaelh */
		/* if (k != 1) Term_clear(); */

		/* Send the command */
		line_searching = searching;
		if (searching) {
			if (reverse) {
				strcpy(tmp, "\373"); /* Hack: Marker for 'reverse' */
				strcat(tmp, srcstr);
				reverse = FALSE;
			} else strcpy(tmp, srcstr);
		}
#ifdef REGEX_SEARCH
		Send_special_line(special_line_type, cur_line + ((regexp && regexp_ok) ? 1000000000 : 0), searching ? tmp : ""); //+1000000000: not backwards compatible, requires 4.9.0+ server
#else
		Send_special_line(special_line_type, cur_line, searching ? tmp : "");
#endif
		searching = FALSE;

		/* Show a general "title" */
#if 0
	/* Don't just print the version as a title, better keep the line free in \
	 that case - ideal for the new 21-lines feature (odd_line / div-3 lines). \
	 This should be kept in sync with Receive_special_line() in nclient.c! */
		//prt(format("[%s]", shortVersion), 0, 0);
		prt(format("[TomeNET %d.%d.%d%s]",
			VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, CLIENT_VERSION_TAG), 0, 0);
#endif

		/* Prompt. (Consistent with prompt in Receive_special_line() in nclient.c.) */
		/* indicate EOF by different status line colour */
		if (cur_line + special_page_size >= max_line)
			c_prt(TERM_ORANGE, format("[Space/p/Enter/BkSpc/g/G/#%s navigate,%s ESC exit.] (%d-%d/%d)",
			    //(p_ptr->admin_dm || p_ptr->admin_wiz) ? "/s/d/D" : "",
#ifdef REGEX_SEARCH
			    regexp_ok ? "/s/d/D/r" : "/s/d/D",
#else
			    "/s/d/D",
#endif
			    my_strcasestr(special_line_title, "unique monster") ? " ! best," : "",
			    cur_line + 1, max_line , max_line), 23 + HGT_PLUS, 0);
		else
			c_prt(TERM_L_WHITE, format("[Space/p/Enter/BkSpc/g/G/#%s navigate,%s ESC exit.] (%d-%d/%d)",
			    //(p_ptr->admin_dm || p_ptr->admin_wiz) ? "/s/d/D" : "",
#ifdef REGEX_SEARCH
			    regexp_ok ? "/s/d/D/r" : "/s/d/D",
#else
			    "/s/d/D",
#endif
			    my_strcasestr(special_line_title, "unique monster") ? " ! best," : "",
			    cur_line + 1, cur_line + special_page_size, max_line), 23 + HGT_PLUS, 0);
		/* Get a keypress -
		   hack: update max_line to its real value as soon as possible */
		if (!max_line) inkey_max_line = TRUE;

		/* Are we in the players list (cmd_players())? Special hack to
		   auto-update the list if players join/leave meanwhile.. */
		if (special_line_type == SPECIAL_FILE_PLAYER) {
			within_cmd_player = TRUE;
			within_cmd_player_ticks = ticks;
			k = inkey();
			within_cmd_player = FALSE;
			if (k == -1) continue; //hack
		} else

		/* Enable macros, so navigation via arrow keys works. */
		k = inkey();
		line_searching = FALSE;

		/* Hack -- go to a specific line */
		if (k == '#') {
			prt(format("Goto Line(max %d): ", max_line), 23 + HGT_PLUS, 0);
			strcpy(tmp, "0");
			old_inkey_msg = inkey_msg;
			inkey_msg = TRUE;
			if (askfor_aux(tmp, 10, 0)) cur_line = atoi(tmp);
			inkey_msg = old_inkey_msg;
		}
		/* Hack -- search for string */
		if (k == 's') {
			prt(format("Search for text: ", max_line), 23 + HGT_PLUS, 0);
			//tmp[0] = 0;
			strcpy(tmp, srcstr);
			old_inkey_msg = inkey_msg;
			inkey_msg = TRUE;
			if (askfor_aux(tmp, 60, 0)) {
				searching = TRUE;
				strcpy(srcstr, tmp);
			}
			inkey_msg = old_inkey_msg;
#ifdef REGEX_SEARCH
			regexp = FALSE;
#endif
		}
#ifdef REGEX_SEARCH
		/* Hack -- search for regexp string */
		if (k == 'r' && regexp_ok) {
			prt(format("Search for regexp: ", max_line), 23 + HGT_PLUS, 0);
			//tmp[0] = 0;
			strcpy(tmp, srcstr);
			old_inkey_msg = inkey_msg;
			inkey_msg = TRUE;
			if (askfor_aux(tmp, 60, 0)) {
				searching = TRUE;
				regexp = TRUE;
				strcpy(srcstr, tmp);
			}
			inkey_msg = old_inkey_msg;
			regexp = TRUE;
		}
#endif
		/* Search for next occurance */
		if (k == 'd' && srcstr[0]) searching = TRUE;
		/* Search for previous occurance */
		if (k == 'D' && srcstr[0]) searching = reverse = TRUE;

		/* Hack -- Allow backing up */
		if (k == '-') {
			cur_line -= 10;
#if 1 /* take a break at beginning of list before wrapping around */
			if (cur_line < 0 &&
			    cur_line > -10)
				cur_line = 0;
#endif
		}

		/* Hack -- Allow backing up ala 'less' */
		if (k == 'b' || k == 'p' || k == KTRL('U')) {
			cur_line -= special_page_size;
#if 1 /* take a break at beginning of list before wrapping around */
			if (cur_line < 0 &&
			    cur_line > -special_page_size)
				cur_line = 0;
#endif
		}

		/* Hack -- Advance one line */
		if ((k == '\n') || (k == '\r') || (k == 'j') || k == '2')
			cur_line++;

		/* Hack -- backing up one line (octal 010 = BACKSPACE) */
		if (k == 'k' || k == '8' || k == '\010')
			cur_line--;

		/* Advance one page */
		if (k == ' ' || k == KTRL('D')) {
			cur_line += special_page_size;
#if 1
	/* take a break at end of list before wrapping around \
	 (consistent with behavior in nclient.c: Receive_special_line() \
	 and with do_cmd_help_aux() in files.c.) */
			if (cur_line > max_line - special_page_size &&
			    cur_line < max_line) {
				cur_line = max_line - special_page_size;
				if (cur_line < 0) cur_line = 0;
			}
#endif
		}

		/* Hack -- back to the top */
		if (k == 'g') cur_line = 0;

		/* Hack -- go to the bottom (it's vi, u know ;) */
		if (k == 'G') cur_line = max_line - special_page_size;

		/* Take a screenshot */
		if (k == KTRL('T')) {
			/* Hack: Screenshots of item inspections are only as long as needed, line-wise */
			if (special_line_type == SPECIAL_FILE_OTHER && cur_line == 0 &&
			    (strstr(special_line_title, "Item Details") || strstr(special_line_title, "Basic Item Information")))
				xhtml_screenshot("screenshot????", 1);
			else
			/* Normal */
			xhtml_screenshot("screenshot????", FALSE);
		}

		/* allow chatting, as it's now also possible within stores */
		if (k == ':') cmd_message();
#ifdef USE_SOUND_2010
		/* allow toggling music/master too */
		else if (k == KTRL('C')) toggle_music(FALSE);
		else if (k == KTRL('N')) toggle_master(FALSE);
		else if (k == KTRL('X')) toggle_music(FALSE);//rl
		else if (k == KTRL('V')) toggle_master(FALSE);//rl
#endif
		/* and very handy for *ID*ing: inscribe this item */
		if (k == '{') cmd_inscribe(USE_INVEN | USE_EQUIP);
		if (k == '}') cmd_uninscribe(USE_INVEN | USE_EQUIP);

		/* Exit on escape */
		if (k == ESCAPE || k == KTRL('Q')) break;

		/* Special functionality depending on information type */
		if (k == '!') {
			/* Unique monster list: Jump to strongest we slayed. */
			if (my_strcasestr(special_line_title, "unique monster")) {
				searching = TRUE;
				strcpy(srcstr, "strongest unique monster");
			}
		}

		/* Horizontal scroll */
		if (k == '4' || k == '<' || k == 'h') {
			/* Scroll left */
			cur_col = (cur_col >= 40) ? (cur_col - 40) : 0;

			/* Success */
			continue;
		}

		/* Horizontal scroll */
		if (k == '6' || k == '>' || k == 'l') {
			/* Scroll right */
			cur_col = cur_col + 40;
			if (cur_col >= ONAME_LEN) { /* Prevent buffer overflow: buf[] in Receive_special_line(). */
				cur_col -= 40;
				if (cur_col < 0) cur_col = 0; /* Edge case, paranoia. */
			}

			/* Success */
			continue;
		}

		/* Hack: Pressing ? while already in ? invokes the guide */
		if (k == '?' && special_line_type == SPECIAL_FILE_HELP) {
			guide_hack = TRUE;
			break;
		}

#if 0 /* not useful :/ only very few items are practicably checkable this way */
		/* Supahack: Pressing ? while looking at an item invokes the guide for that item's details if available */
		if (k == '?' && special_line_type == SPECIAL_FILE_OTHER && cur_line == 0 &&
		    (strstr(special_line_title, "Item Details") || strstr(special_line_title, "Basic Item Information"))) {
			char uppercase[ONAME_LEN];
			int i = 0, j = 4;

			while (special_line_first[j] && special_line_first[j] != '{') {
				uppercase[i] = toupper(special_line_first[j]);
				i++;
				j++;
			}
			uppercase[i] = 0;
			if (uppercase[i - 1] == ' ') uppercase[i - 1] = 0;
			if (!strncmp(uppercase, "AN ", 2)) cmd_the_guide(3, 0, uppercase + 3);
			else if (!strncmp(uppercase, "A ", 2)) cmd_the_guide(3, 0, uppercase + 2);
			else cmd_the_guide(3, 0, uppercase);
			continue;
		}
#endif

		/* Check maximum line */
		/* don't allow 'empty lines' at end of list but wrap around immediately */
		if (cur_line >= max_line) cur_line = 0;
		/* Except when searching, or getting subsequent matches would become annoying.. */
		else if (!searching && cur_line > max_line - special_page_size) cur_line = max_line - special_page_size;
		/* ..and wrap around backwards too */
		else if (cur_line < 0) {
			cur_line = max_line - special_page_size;
			if (cur_line < 0) cur_line = 0;
		}
	}

	/* Tell the server we're done looking */
	Send_special_line(SPECIAL_FILE_NONE, 0, "");

	/* No longer using file perusal */
	special_line_type = 0;

	/* Forget the length of this file, since the next file
	   we peruse might be completely different */
	max_line = 0;

	/* Reload the old screen */
	Term_load();
	perusing = FALSE;

	/* Flush any events that came in */
	Flush_queue();

	/* Hack: ? + ? = Guide */
	if (guide_hack) {
		cmd_the_guide(0, 0, NULL);
		Flush_queue();//needed?
	}

	inkey_interact_macros = old_inkey_interact_macros;
}

/*
 * Hack -- Dump a character description file
 *
 * XXX XXX XXX Allow the "full" flag to dump additional info,
 * and trigger its usage from various places in the code.
 */
#define DUMP_MAX_X 80	/* Increased to 80 from originally 79, or Chh gets its last column cut off (and potentially other stuff too) */
errr file_character(cptr name, bool quiet) {
	int		i, x, y;
	byte		a;
	char32_t		c;
	cptr		paren = ")";
	int		fd = -1;
	FILE		*fff = NULL;
	char		buf[1024], *cp, linebuf[1024];

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, name);
	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the non-existing file */
	if (fd < 0) fff = my_fopen(buf, "w");
	/* Invalid file */
	if (!fff) {
		/* Message */
		c_msg_print("Character dump failed!");
		clear_topline_forced();

		/* Error */
		return(-1);
	}

	/* Save the old screen */
	Term_save();

	/* Begin dump */
	fprintf(fff, "  [TomeNET %d.%d.%d%s @ %s Character Dump]\n\n",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, CLIENT_VERSION_TAG, server_name);

	/* Display player */
	display_player(0);

	/* Dump part of the screen */
	for (y = 1; y < 22; y++) {
		/* Dump each row */
#if 0 /* this is actually correct */
		for (x = 0; x < Term->wid; x++) {
#else /* bad hack actually, just to avoid spacer lines on oook.cz */
		for (x = 0; x < DUMP_MAX_X; x++) {
#endif
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));
			/* Characters above MAX_FONT_CHAR are graphical and will reported as MAX_FONT_CHAR. */
			if (c > MAX_FONT_CHAR) c = MAX_FONT_CHAR;
			/* Dump it */
			buf[x] = (char)c;
		}
		/* Terminate */
		buf[x] = '\0';
		/* End the row */
		fprintf(fff, "%s\n", buf);
	}

	/* Display history */
	display_player(1);

	/* Dump part of the screen */
	for (y = 14; y < 19; y++) {
		/* Dump each row */
#if 0 /* this is actually correct */
		for (x = 0; x < Term->wid; x++) {
#else /* bad hack actually, just to avoid spacer lines on oook.cz */
		for (x = 0; x < DUMP_MAX_X; x++) {
#endif
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));
			/* Characters above MAX_FONT_CHAR are graphical and will reported as MAX_FONT_CHAR. */
			if (c > MAX_FONT_CHAR) c = MAX_FONT_CHAR;
			/* Dump it */
			buf[x] = c;
		}
		/* Terminate */
		buf[x] = '\0';
		/* End the row */
		fprintf(fff, "%s\n", buf);
	}

	/* Display mods/flags ('Chh' screen) */
	display_player(2);

	/* Dump part of the screen */
	for (y = 1; y < 22; y++) {
		/* Dump each row */
#if 0 /* this is actually correct */
		for (x = 0; x < Term->wid; x++) {
#else /* bad hack actually, just to avoid spacer lines on oook.cz */
		for (x = 0; x < DUMP_MAX_X; x++) {
#endif
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));
			/* Characters above MAX_FONT_CHAR are graphical and will reported as MAX_FONT_CHAR. */
			if (c > MAX_FONT_CHAR) c = MAX_FONT_CHAR;
			/* Dump it */
			buf[x] = (char)c;
		}
		/* Terminate */
		buf[x] = '\0';
		/* End the row */
		fprintf(fff, "%s\n", buf);
	}

	/* Dump skills */
	fprintf(fff, "\n  [Skill Chart]\n"); /* one less \n, dump_skills() adds one */
	dump_skills(fff);

	/* Skip some lines */
	fprintf(fff, "\n\n");

	/* Dump the equipment */
	fprintf(fff, "  [Character Equipment]\n\n");
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		/* For coloured rune sigils in item names: Strip colour codes! */
		cp = inventory_name[i];
		x = 0;
		while (*cp) {
			if (*cp == '\377') cp++;
			else buf[x++] = *cp;
			cp++;
		}
		buf[x] = 0;

		/* trim lines to 80 chars */
		sprintf(linebuf, "%c%s %s\n", index_to_label(i), paren, buf);
		//linebuf[80] = 0; --not for now
		fprintf(fff, "%s", linebuf);
	}
	fprintf(fff, "\n\n");

	/* Dump the inventory */
	fprintf(fff, "  [Character Inventory]\n\n");
	for (i = 0; i < INVEN_PACK; i++) {
		if (!strncmp(inventory_name[i], "(nothing)", 9)) continue;

		/* trim lines to 80 chars */
		sprintf(linebuf, "%c%s %s\n", index_to_label(i), paren, inventory_name[i]);
		//linebuf[80] = 0; --not for now
		fprintf(fff, "%s", linebuf);
	}
	fprintf(fff, "\n\n");

#ifdef ENABLE_SUBINVEN
	/* Dump the inventory */
	fprintf(fff, "  [Character Subinventory]\n\n");
	for (i = 0; i < INVEN_PACK; i++) {
		if (inventory[i].tval != TV_SUBINVEN) continue;

		/* trim lines to 80 chars */
		sprintf(linebuf, "[%c] %s\n", index_to_label(i), inventory_name[i]);
		//linebuf[80] = 0; --not for now
		fprintf(fff, "%s", linebuf);

		for (x = 0; x < SUBINVEN_PACK; x++) {
			if (!subinventory[i][x].tval) continue;

			/* trim lines to 80 chars */
			sprintf(linebuf, "  %c%s %s\n", index_to_label(i), paren, subinventory_name[i][x]);
			//linebuf[80] = 0; --not for now
			fprintf(fff, "%s", linebuf);
		}
	}
	fprintf(fff, "\n\n");
#endif

	/* Dump the last messages */
	fprintf(fff, "  [Last Messages]\n\n");
	dump_messages_aux(fff, 100, 0, TRUE);
	fprintf(fff, "\n"); /* one less newline, since dump_messages_aux already adds */

	/* Dump a simple monochrome 'screenshot' of the level too, in
	   addition to the xhtml real screenshot output (on death dump) */
	fprintf(fff, "  [Surroundings]\n\n");
	if (screen_icky) Term_switch(0);
	/* skip top line, already in 'last messages' if any at all */
	for (y = 1; y < Term->hgt; y++) {
		for (x = 0; x < Term->wid - 1; x++) { /* -1: Hack for angband.oook.cz ladder: 80 chars would cause extra linebreaks there :/ So we just discard the final column.. */
			(void)(Term_what(x, y, &a, &c));
			/* Characters above MAX_FONT_CHAR are graphical and will reported as MAX_FONT_CHAR. */
			if (c > MAX_FONT_CHAR) c = MAX_FONT_CHAR;

			switch (c) {
			/* Windows client uses ASCII char 31 for paths */
			case 31:
				c = '.';
				break;
			/* revert special characters from font_map_solid_walls */
			case FONT_MAP_SOLID_WIN: case FONT_MAP_SOLID_X11:
				c = '#';
				break;
			case FONT_MAP_VEIN_WIN: //case FONT_MAP_VEIN_X11: --duplicate case atm (both are 1)
				c = '$';
				break;
			default:
				/* catch possible custom fonts */
				if (c < 32 || c > 126) c = '~';
			}

			/* dump it */
			buf[x] = c;
		}
		buf[x] = '\0';
		fprintf(fff, "%s\n", buf);
	}
	if (screen_icky) Term_switch(0);
	fprintf(fff, "\n\n");

 	/* Unique monsters slain/helped with - C. Blue */
	fprintf(fff, "  [Unique Monsters]\n\n");
	/* 1st pass - determine category existence: kill vs assist */
	x = 0; /* count killing blows */
	for (i = 0; i < MAX_UNIQUES; i++) if (r_unique[i] == 1) x++;
	if (!x) fprintf(fff, "You have not slain any unique monsters yourself.\n");
	/* 2nd pass - list killed and assisted with monsters */
	for (i = 0; i < MAX_UNIQUES; i++) {
		if (r_unique[i] == 1)
			fprintf(fff, "You have slain %s.\n", r_unique_name[i]);
		else if (r_unique[i] == 2)
			fprintf(fff, "You have assisted in slaying %s.\n", r_unique_name[i]);
	}
	fprintf(fff, "\n\n");

	/* Close it */
	my_fclose(fff);

	/* Reload the old screen */
	Term_load();

	/* Message */
	if (!quiet) c_msg_format("Character dump to file '%s' successful.", name);
	clear_topline_forced();

	/* Success */
	return(0);
}

#ifdef WINDOWS
/* On Windows async screenshot process: Test if it finished meanwhile. */
void screenshot_result_check(void) {
	FILE *fp;
	char resbuf[5];
	int res = -1;
	bool missing;

	/* Since this is an async process spawn, wait for a limited time for it to complete */
	if ((missing = !my_fexists("screenCapture.res")) && screenshotting) return;
	//had one report that on an i5/Win10 screenshotting took somewhat over 2s sometimes o_O so this might time out a lot.

	/* Screenshot process didn't complete in time? */
	if (missing) {
		if (!screenshot_silent_dump) {
			c_msg_print("Warning: screenCapture exceeded time limit, but might still have worked fine.");
			c_msg_format(" If so, it saved to %spng", screenshotting_filename);
		} else {
			screenshot_silent_dump = FALSE;
			c_msg_print("Warning: screenCapture.bat exceeded time limit.");
		}
		return;
	}

	/* try to read result code */
	fp = fopen("screenCapture.res", "r");
	if (fp) {
		if (fgets(resbuf, 5, fp)) res = atoi(resbuf);
		else c_msg_print("Error: Unable to read result value from screenCapture.res."); //paranoia
		fclose(fp);
	} else c_msg_print("Error: Cannot open screenCapture.res."); //paranoia
	remove("screenCapture.res");
	if (res == -1) {
		screenshot_silent_dump = FALSE;
		return;
	}

	/* evaluate result code. '10' indicates no .NET framework, it's unlikely any other error gets thrown */
	if (res == 10) c_msg_print("Error: .NET framework must be installed to take PNG image screenshots.");
	else if (res != 0) c_msg_format("Error: Unknown error %d.", atoi(resbuf));
	//res = 0 aka no error:
	else if (!screenshot_silent_dump) c_msg_format("Screenshot saved to %spng", screenshotting_filename);
	screenshot_silent_dump = FALSE;
}
#endif

/*
 * Make an xhtml screenshot - mikaelh
 * Some code borrowed from ToME
 * Mode 'redux' added (C. Blue):
 * 0/FALSE:	normal,
 * 1:		for item inspection, stop screenshot when two empty lines occur subsequently,
 * 2:		use non-bigmap screen in any case,
 * 3:		for monster/artifact lore aux - display only the relevant paragraph about the actual monster/item.
 */
void xhtml_screenshot(cptr name, byte redux) {
	static cptr color_table[16 + 1] = {
		"#000000",	/* BLACK */
		"#ffffff",	/* WHITE */
		"#9d9d9d",	/* GRAY */
		"#ff8d00",	/* ORANGE */
		"#b70000",	/* RED */
		"#009d44",	/* GREEN */
//#ifndef READABILITY_BLUE
#if 0
		"#0000ff",	/* BLUE */
#else
		"#0033ff",	/* BLUE */
#endif
		"#8d6600",	/* BROWN */
//#ifndef DISTINCT_DARK
#if 0
		"#747474",	/* DARKGRAY */
#else
		//"#585858",	/* DARKGRAY */
		"#666666",	/* DARKGRAY */
#endif
		"#cdcdcd",	/* LIGHTGRAY */
		"#af00ff",	/* PURPLE */
		"#ffff00",	/* YELLOW */
		"#ff3030",	/* PINK */
		"#00ff00",	/* LIGHTGREEN */
		"#00ffff",	/* LIGHTBLUE */
		"#c79d55",	/* LIGHTBROWN */
		"#f0f0f0",	/* Invalid color */
	};
#ifdef EXTENDED_BG_COLOURS
//todo: implement
#endif

	FILE *fp;
	byte *scr_aa;
	char32_t *scr_cc;
	char unm_cc;
	char *unm_cc_ptr;
	char32_t unm_cc_idx;
	byte cur_attr, prt_attr;
	int i, x, y, max;
	int y_start, y_end, ss_lines = 0;
	char buf[1024];
	char file_name[256];


	x = strlen(name) - 4;

	/* Replace "????" in the end */
	if (!strcmp("????", &name[x])) {
		/* Paranoia */
		if (x > 200) x = 200;

		if (!c_cfg.screenshot_format) {
			/* Replace '????' with a continuously increasing number */
#ifdef WINDOWS
			/* Windows implementation */
			WIN32_FIND_DATA findFileData;
			HANDLE hFind;
			char buf2[1024];

			/* Search for numbered screenshot files */
			strncpy(buf2, name, x);
			buf2[x] = '\0';
			strcat(buf2, "*.xhtml");
			path_build(buf, 1024, ANGBAND_DIR_USER, buf2);

			max = 0;

			hFind = FindFirstFile(buf, &findFileData);

			if (hFind) {
				if (isdigit(findFileData.cFileName[x])) {
					i = atoi(&findFileData.cFileName[x]);
					if (i > max) max = i;
				}

				while (FindNextFile(hFind, &findFileData)) {
					if (isdigit(findFileData.cFileName[x])) {
						i = atoi(&findFileData.cFileName[x]);
						if (i > max) max = i;
					}
				}

				FindClose(hFind);
			}
#else
			/* UNIX implementation based on opendir */
			DIR *dp;
			struct dirent *entry;

			dp = opendir(ANGBAND_DIR_USER);

			if (!dp) {
				c_msg_print("Couldn't open the user directory.");
				silent_dump = FALSE;
				return;
			}

			max = 0;

			/* Find the file with the biggest number */
			while ((entry = readdir(dp))) {
				/* Check that the name matches the pattern */
				if (strncmp(name, entry->d_name, x) == 0 && isdigit(entry->d_name[x])) {
					i = atoi(&entry->d_name[x]);
					if (i > max) max = i;
				}
			}

			closedir(dp);
#endif
			/* Use the next number in the name */
			strncpy(buf, name, x);
			buf[x] = '\0';
			snprintf(file_name, 256, "%s%04d.xhtml", buf, max + 1);
		} else {
			time_t ct = time(NULL);
			struct tm* ctl = localtime(&ct);

			/* Replace '????' with the current date and time */
			strncpy(buf, name, x);
			buf[x] = '\0';

			//snprintf(file_name, 256, "%s%-.24s", buf, ctime(&ct));
			snprintf(file_name, 256, "%s_%04d-%02d-%02d_%02d-%02d-%02d.xhtml", buf,
			    1900 + ctl->tm_year, ctl->tm_mon + 1, ctl->tm_mday,
			    ctl->tm_hour, ctl->tm_min, ctl->tm_sec);
		}

		path_build(buf, 1024, ANGBAND_DIR_USER, file_name);
	} else {
		strncpy(file_name, name, 249);
		file_name[249] = '\0';
		strcat(file_name, ".xhtml");
		path_build(buf, 1024, ANGBAND_DIR_USER, file_name);
		fp = fopen(buf, "rb");
		if (fp) {
			char buf2[1028];

			fclose(fp);
			strcpy(buf2, buf);
			strcat(buf2, ".bak");
			/* Make a backup of an already existing file */
			rename(buf, buf2);
		}
	}

#ifdef ENABLE_SHIFT_SPECIALKEYS
	/* Hack: SHIFT+CTRL+T makes a real (PNG) screenshot instead of an xhtml screenshot - C. Blue */
	if ((!c_cfg.screenshot_keys && inkey_shift_special == 3) ||
	    (c_cfg.screenshot_keys && inkey_shift_special != 3)) {
 #if defined(USE_X11)
		/* On X11, use ImageMagick's 'import' if available */
		if (x11_win_term_main) {
			char buf2[1028];

			strcpy(buf2, buf);
			buf2[strlen(buf2) - 5] = 0;
			if (!system(format("import -window %d %spng", x11_win_term_main, buf2))) {
				strcpy(buf2, file_name);
				buf2[strlen(buf2) - 5] = 0;
				if (!silent_dump) c_msg_format("Screenshot saved to %spng", buf2);
			} else c_msg_format("Error: Failed to call imagemagick's 'import'. ('%s')", buf2);
			silent_dump = FALSE;
			return;
		}
 #elif defined(WINDOWS)
		/* On Windows, use the .NET framework if available, via batch file */
		char buf2[1028];

		/* Generate filename with path (from xhtml filename) */
		strcpy(buf2, buf);
		buf2[strlen(buf2) - 5] = 0;

		/* Spawn async process to take screenshot */
		remove("screenCapture.res");
		if (WinExec(format("screenCapture.bat \"%spng\" \"\"", buf2), SW_HIDE) > 31) {
			screenshotting = 500; // 500 * 10ms interval -> wait for 5s max to give feedback msg

			/* Generate filename without path (from xhtml filename) */
			strcpy(buf2, file_name);
			buf2[strlen(buf2) - 5] = 0;
			strcpy(screenshotting_filename, buf2);
			screenshot_silent_dump = silent_dump;
		} else c_msg_format("Error: Failed to call screenCapture.bat (%lu).", GetLastError());
		silent_dump = FALSE;
		return;
 #endif
	}
#endif

	fp = fopen(buf, "wb");
	if (!fp) {
		/* Couldn't write */
		silent_dump = FALSE;
		return;
	}

	x = strlen(file_name);
	file_name[x - 6] = '\0'; /* Kill the ".xhtml" from the end */

	fprintf(fp, "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
		     "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"DTD/xhtml1-strict.dtd\">\n"
		     "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
		     "<head>\n");
	fprintf(fp, "<meta name=\"GENERATOR\" content=\"TomeNET %d.%d.%d%s\"/>\n",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, CLIENT_VERSION_TAG);
	fprintf(fp, "<title>%s</title>\n", file_name);
	fprintf(fp, "</head>\n"
		     "<body>\n"
		     "<pre style=\"color: #ffffff; background-color: #000000; font-family: monospace\">\n");

	cur_attr = Term->scr->a[0][0];
#ifdef EXTENDED_COLOURS_PALANIM
	if (cur_attr >= TERMA_OFFSET && cur_attr < TERMA_OFFSET + BASE_PALETTE_SIZE) cur_attr -= TERMA_OFFSET; /* Use the basic colours instead of the palette-animated ones */
#endif
	prt_attr = term2attr(cur_attr);
	/* safe-fail: can happen if an EXTENDED_BG_COLOUR is used but not defined here (see color_table[] above): */
	if (prt_attr > N_ELEMENTS(color_table) - 1) prt_attr = N_ELEMENTS(color_table) - 1;
	fprintf(fp, "<span style=\"color: %s\">", color_table[prt_attr]);

	size_t bytes = 0;
	y_start = (redux == 1) ? 2 : ((redux == 3) ? 5 : 0);
	y_end = (redux == 2) ? DEFAULT_TERM_HGT : Term->hgt;
	for (y = y_start; y < y_end; y++) {
		scr_aa = Term->scr->a[y];
		scr_cc = Term->scr->c[y];

		if (redux == 1) {
			bool more = FALSE;

			/* Preparse two lines, if both are blank we stop right before the first one and are done, assuming that no more content will follow. */
			for (x = 0; x < Term->wid; x++)
				if (scr_cc[x] != ' ') {
					more = TRUE;
					break;
				}
			/* Try next line? */
			if (!more) {
				scr_cc = Term->scr->c[y + 1];
				for (x = 0; x < Term->wid; x++)
					if (scr_cc[x] != ' ') {
						more = TRUE;
						break;
					}
				/* Restore pointer for actual screenshot */
				scr_cc = Term->scr->c[y];
			}
			if (!more) {
				y = Term->hgt - 1;
				continue;
			}
		}
		if (redux == 3 && y >= 7) {
			bool more = FALSE;

			/* Preparse one lines, if it's blank we stop right before t and are done, assuming that no more relevant content will follow. */
			for (x = 0; x < Term->wid; x++)
				if (scr_cc[x] != ' ') {
					more = TRUE;
					break;
				}
			if (!more) {
				y = Term->hgt - 1;
				continue;
			}
		}
		ss_lines++;

		for (x = 0; x < Term->wid; x++) {
			if (scr_aa[x] != cur_attr) {
				cur_attr = scr_aa[x];
#ifdef EXTENDED_COLOURS_PALANIM
				if (cur_attr >= TERMA_OFFSET && cur_attr < TERMA_OFFSET + BASE_PALETTE_SIZE) cur_attr -= TERMA_OFFSET; /* Use the basic colours instead of the palette-animated ones */
#endif

				strcpy(&buf[bytes], "</span><span style=\"color: ");
				bytes += 27;

				/* right now just pick a random colour for flickering colours
				 * maybe add some javascript for real flicker later */
				prt_attr = term2attr(cur_attr);
				/* safe-fail: can happen if an EXTENDED_BG_COLOUR is used but not defined here (see color_table[] above): */
				if (prt_attr > N_ELEMENTS(color_table) - 1) prt_attr = N_ELEMENTS(color_table) - 1;
				strcpy(&buf[bytes], color_table[prt_attr]);
				bytes += 7;

				strcpy(&buf[bytes], "\">");
				bytes += 2;
			}

			/* unmap custom fonts, so screenshot becomes readable */
			unm_cc_idx = scr_cc[x];
			if (NULL != (unm_cc_ptr = u32b_char_dict_get(monster_mapping_mod, unm_cc_idx))) unm_cc = *unm_cc_ptr;
			else if (NULL != (unm_cc_ptr = u32b_char_dict_get(floor_mapping_mod, unm_cc_idx))) unm_cc = *unm_cc_ptr;
			else unm_cc = (char) scr_cc[x]; /* no custom mapping found, take as is */

			switch (unm_cc) {
			/* revert special characters from font_map_solid_walls */
			case FONT_MAP_SOLID_WIN: case FONT_MAP_SOLID_X11:
				buf[bytes++] = '#';
				break;
			case FONT_MAP_VEIN_WIN: //case FONT_MAP_VEIN_X11: --duplicate case atm (both are 1)
				buf[bytes++] = '$';
				break;
			/* Windows client uses ASCII char 31 for paths */
			case 31:
				buf[bytes++] = '.';
				break;
			case '&':
				buf[bytes++] = '&';
				buf[bytes++] = 'a';
				buf[bytes++] = 'm';
				buf[bytes++] = 'p';
				buf[bytes++] = ';';
				break;
			case '<':
				buf[bytes++] = '&';
				buf[bytes++] = 'l';
				buf[bytes++] = 't';
				buf[bytes++] = ';';
				break;
			case '>':
				buf[bytes++] = '&';
				buf[bytes++] = 'g';
				buf[bytes++] = 't';
				buf[bytes++] = ';';
				break;
			default:
				/* catch possible custom fonts */
				if (unm_cc < 32 || unm_cc > 126)
					buf[bytes++] = '~';
				else
					/* proceed normally */
					buf[bytes++] = unm_cc;
			}
			/* Write data out before the buffer gets full */
			if (bytes > 512) {
				if (fwrite(buf, 1, bytes, fp) < bytes) {
					fprintf(stderr, "fwrite failed\n");
					c_msg_print("\377rScreenshot could not be written!");
					fclose(fp);
					silent_dump = FALSE;
					return;
				}

				bytes = 0;
			}
		}
		buf[bytes++] = '\n';
	}

	/* Write what remains in the buffer */
	if (bytes) {
		if (fwrite(buf, 1, bytes, fp) < bytes) {
			fprintf(stderr, "fwrite failed\n");
			c_msg_print("\377rScreenshot could not be written!");
			fclose(fp);
			silent_dump = FALSE;
			return;
		}
	}

	fprintf(fp, "</span>\n"
		    "</pre>\n"
		    "</body>\n"
		    "</html>\n");

	fclose(fp);

	if (!silent_dump) c_msg_format("Screenshot saved to %s.xhtml", file_name);
	else silent_dump = FALSE;

	/* For PNG screenshot */
	strcpy(screenshot_filename, file_name);
	screenshot_height = ss_lines;
}


/* Save Auto-Inscription file (*.ins) - C. Blue */
void save_auto_inscriptions(cptr name) {
	FILE *fp;
	char buf[1024];
	char file_name[256];
	int i;

	strncpy(file_name, name, 249);
	file_name[249] = '\0';

	/* add '.ins' extension if not already existing */
	if (strlen(name) > 4) {
		if (name[strlen(name) - 1] == 's' &&
		    name[strlen(name) - 2] == 'n' &&
		    name[strlen(name) - 3] == 'i' &&
		    name[strlen(name) - 4] == '.') {
			/* fine */
		} else strcat(file_name, ".ins");
	} else strcat(file_name, ".ins");

	path_build(buf, 1024, ANGBAND_DIR_USER, file_name);

	fp = fopen(buf, "w");
	if (!fp) {
		/* Couldn't write */
		c_msg_print("Saving Auto-inscriptions failed!");
		return;
	}

	/* write header (1 line) */
	fprintf(fp, "Auto-Inscriptions file for TomeNET v%d.%d.%d%s\n",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, CLIENT_VERSION_TAG);

	fprintf(fp, "### This file may contain comment lines, these, must start on '##' but will NOT be saved.\n");
	fprintf(fp, "### Note that empty lines do count as data lines and hence cannot be inserted for the purpose of visual formatting.\n");

	/* write inscriptions (2 lines each) */
	for (i = 0; i < MAX_AUTO_INSCRIPTIONS; i++) {
		fprintf(fp, "%s\n", auto_inscription_match[i]);
		fprintf(fp, "%s\n", auto_inscription_tag[i]);
		fprintf(fp, "  %c,", auto_inscription_force[i] ? 'F' : '-');
		fprintf(fp, "%c,", auto_inscription_ignore[i] ? 'i' : (auto_inscription_autopickup[i] ? 'a' : (auto_inscription_autodestroy[i] ? 'A' : '-')));
		fprintf(fp, "%c,", auto_inscription_subinven[i] ? 'b' : '-'); //ENABLE_SUBINVEN
		fprintf(fp, "%c\n", auto_inscription_disabled[i] ? 'X' : '-');
	}

	fclose(fp);

	c_msg_format("Auto-inscriptions saved to file '%s'.", file_name);
}

/* Similar to fgets() reads at most len-1 characters and 0-terminates the string (as usual at position len-1 aka the len'th char). - C. Blue */
static char *ai_fgets(char *buf, int len, FILE *fp) {
	char tmp[1024];

	while (TRUE) {
		if (fgets(tmp, 1024, fp) == NULL) return(NULL);
		/* allow comments: Ignore all lines starting on '###' */
		if (tmp[0] == '#' && tmp[1] == '#' && tmp[2] == '#') continue;
		/* trim trailing newline from fgets() */
		if (*tmp && tmp[strlen(tmp) - 1] == '\n') tmp[strlen(tmp) - 1] = 0;
		break;
	}

	/* Paranoia edge case */
	if (len <= 0) {
		*buf = 0;
		return(buf);
	}

	strncpy(buf, tmp, len);
	/* Ensure proper string termination */
	if (len <= strlen(buf)) buf[len - 1] = 0;
	return(buf);
}

/* Load Auto-Inscription file (*.ins) - C. Blue
   Returns TRUE if filename was found and loaded at least one inscription entry
   (even partially, if some of it was corrupt/missing) from it, else FALSE. */
bool load_auto_inscriptions(cptr name) {
	FILE *fp;
	char buf[1024], *bufptr;
	char file_name[256], vtag[7];
	int i, c, j, c_eff, version, vmaj, vmin, vpatch, tmp;
	bool replaced, force, loaded_something = FALSE;
#ifdef REGEX_SEARCH
	int ires = -999;
	regex_t re_src;
	char *regptr;
#endif

	strncpy(file_name, name, 249);
	file_name[249] = '\0';

	/* add '.ins' extension if not already existing */
	if (strlen(name) > 4) {
		if (name[strlen(name) - 1] == 's' &&
		    name[strlen(name) - 2] == 'n' &&
		    name[strlen(name) - 3] == 'i' &&
		    name[strlen(name) - 4] == '.') {
			/* fine */
		} else strcat(file_name, ".ins");
	} else strcat(file_name, ".ins");

	path_build(buf, 1024, ANGBAND_DIR_USER, file_name);

	fp = fopen(buf, "r");
	if (!fp) {
		/* Couldn't open */
		return(FALSE);
	}

	/* load header (1 line) */
	if (ai_fgets(buf, 80, fp) == NULL) {
		fclose(fp);
		return(FALSE);
	}

	/* extract version */
	i = sscanf(buf, "Auto-Inscriptions file for TomeNET v%d.%d.%d%s\n", &vmaj, &vmin, &vpatch, vtag);
	if (i < 4) vtag[0] = 0;

	if (vmaj < 4 || /* at most 4.7.1a */
	    (vmaj == 4 && (vmin < 7 ||
	    (vmin == 7 && (vpatch < 1 ||
	    (vpatch == 1 && (!vtag[0] || vtag[0] == 'a')))))))
		version = 1;
	else if (vmaj == 4 && vmin == 7 && vpatch < 4) /* older than 4.7.4 */
		version = 2;
	else if (vmaj == 4 && (vmin < 9 ||
	    (vmin == 9 && (vpatch < 1 || (vpatch == 1 && (!vtag[0] || vtag[0] == '-')))))) // '-Test' client tag; at most 4.9.1-test
		version = 3;
	else if (vmaj == 4 && vmin == 9 && (vpatch < 3 || (vpatch == 3 && !vtag[0]))) /* at most 4.9.3 */
		version = 4;
	else //starting at 4.9.3a/test
		version = 5;

#ifdef TEST_CLIENT
	logprint(format("Read a v%d.%d.%d <%s><%c%c> .ins file, version %d.\n", vmaj, vmin, vpatch, vtag, vtag[0], vtag[1], version));
	c_msg_format("Read a v%d.%d.%d <%s><%c%c> .ins file, version %d.", vmaj, vmin, vpatch, vtag, vtag[0], vtag[1], version);
#endif

	/* attempt to merge current auto-inscriptions, and give priority to those we want to load here */

	/* load inscriptions (2 lines each) */
	c = 0; /* current internal auto-inscription slot to set */
	for (i = 0; i < MAX_AUTO_INSCRIPTIONS; i++) {
		replaced = FALSE;
		force = FALSE;

		/* try to read a match */
		if (version < 5) {
			if (ai_fgets(buf, AUTOINS_MATCH_LEN + 1, fp) == NULL) break; /* +1 to accomodate for prefixed '!' in older versions */
			bufptr = buf;
			if (*bufptr == '!') {
				force = TRUE;
				bufptr++;
			} else bufptr[AUTOINS_MATCH_LEN - 1] = 0;
		} else {
			if (ai_fgets(buf, AUTOINS_MATCH_LEN, fp) == NULL) break;
			bufptr = buf;
		}

		/* skip empty matches */
		if (*bufptr == 0) {
			/* try to read according tag */
			if (ai_fgets(buf, AUTOINS_TAG_LEN, fp) == NULL) break;
			if (version < 5) {
				/* try to read automation flags */
				if (version >= 3) {
					if (ai_fgets(buf, 5, fp) == NULL) break;
					if (ai_fgets(buf, 5, fp) == NULL) break;
				}
				/* try to read 'bags-only' and 'disabled' flags */
				if (version >= 4) {
					if (ai_fgets(buf, 5, fp) == NULL) break;
					if (ai_fgets(buf, 5, fp) == NULL) break;
				}
			} else {
				char aif, aiidp, ais, aid;
				int res;

				if (ai_fgets(buf, MAX_CHARS, fp) == NULL) break;
				res = sscanf(buf, "  %c,%c,%c,%c\n", &aif, &aiidp, &ais, &aid);
				if (res != 4) break;
			}
			continue;
		}

		/* Old version (v1, before version tag was introduced): Convert '?' wildcard to new '*' wildcard automatically: */
		if (version == 1) {
			char *wc;

			while ((wc = strchr(buf, '?'))) *wc = '#';
		}

		/* check for duplicate entry (if it already exists)
		   and replace older entry simply */
		for (j = 0; j < MAX_AUTO_INSCRIPTIONS; j++) {
			if (strcmp(bufptr, auto_inscription_match[j])) continue;

			/* try to read according tag */
			if (ai_fgets(buf, AUTOINS_TAG_LEN, fp) == NULL) break;
			strcpy(auto_inscription_tag[j], buf);
			auto_inscription_force[j] = force;

			if (version < 5) {
				auto_inscription_ignore[j] = FALSE;

				/* try to read automation flags */
				if (version >= 3) {
					if (ai_fgets(buf, 5, fp) == NULL) break;
					tmp = atoi(buf);
					if (tmp >= 2) {
						auto_inscription_ignore[j] = TRUE;
						tmp -= 2;
					}
					auto_inscription_autopickup[j] = tmp;

					if (ai_fgets(buf, 5, fp) == NULL) break;
					auto_inscription_autodestroy[j] = atoi(buf);
				}

				/* try to read 'bags-only' and 'disabled' flags */
				if (version >= 4) {
					if (ai_fgets(buf, 5, fp) == NULL) break;
					auto_inscription_subinven[j] = atoi(buf);
					if (ai_fgets(buf, 5, fp) == NULL) break;
					auto_inscription_disabled[j] = atoi(buf);
				}
			} else {
				char aif, aiidp, ais, aid;
				int res;

				if (ai_fgets(buf, MAX_CHARS, fp) == NULL) break;
				res = sscanf(buf, "  %c,%c,%c,%c\n", &aif, &aiidp, &ais, &aid);
				if (res != 4) break;

				auto_inscription_force[j] = force = (aif == 'F');
				auto_inscription_ignore[j] = auto_inscription_autopickup[j] = auto_inscription_autodestroy[j] = FALSE;
				switch (aiidp) {
				case 'i': auto_inscription_ignore[j] = TRUE; break;
				case 'a': auto_inscription_autopickup[j] = TRUE; break;
				case 'A': auto_inscription_autodestroy[j] = TRUE; break;
				}
				auto_inscription_subinven[j] = (ais == 'b');
				auto_inscription_disabled[j] = (aid == 'X');
			}

			replaced = TRUE;
			loaded_something = TRUE;
			break;
		}
		if (replaced) continue;

		if (j < MAX_AUTO_INSCRIPTIONS) break; //premature ending -> broken .ins file

		/* search for free match-slot */
		if (c >= 0) {
			while (auto_inscription_match[c][0] && c < MAX_AUTO_INSCRIPTIONS) c++;
			if (c == MAX_AUTO_INSCRIPTIONS) {
				c = -1;
				c_eff = 0;
			} else c_eff = c;
		} else {
			/* if all slots are full, overwrite them starting with the first slot */
			c--;
			c_eff = -c - 1;
		}
		/* set slot */
		loaded_something = TRUE;
		strcpy(auto_inscription_match[c_eff], bufptr);
		auto_inscription_force[c_eff] = force;
#ifdef REGEX_SEARCH
		/* Actually test regexp for validity right away, so we can avoid spam/annoyance/searching later. */
		/* Check for '$' prefix, forcing regexp interpretation */
		regptr = auto_inscription_match[c_eff];
		if (regptr[0] == '$') {
			regptr++;
			ires = regcomp(&re_src, regptr, REG_EXTENDED | REG_ICASE);
			if (ires != 0) {
				auto_inscription_invalid[c_eff] = TRUE;
				c_msg_format("\377oInvalid regular expression in auto-inscription #%d.", c_eff + 1);
			} else auto_inscription_invalid[c_eff] = FALSE;
			regfree(&re_src);
		}
#endif

		/* try to read according tag */
		if (ai_fgets(buf, AUTOINS_TAG_LEN, fp) == NULL) break;
		strcpy(auto_inscription_tag[c_eff], buf);

		if (version < 5) {
			auto_inscription_ignore[c_eff] = FALSE;

			/* try to read automation flags */
			if (version >= 3) {
				if (ai_fgets(buf, 5, fp) == NULL) break;
				tmp = atoi(buf);
				if (tmp >= 2) {
					auto_inscription_ignore[c_eff] = TRUE;
					tmp -= 2;
				}
				auto_inscription_autopickup[c_eff] = tmp;

				if (ai_fgets(buf, 5, fp) == NULL) break;
				auto_inscription_autodestroy[c_eff] = atoi(buf);
			}

			/* try to read 'bags-only' and 'disabled' flags */
			if (version >= 4) {
				if (ai_fgets(buf, 5, fp) == NULL) break;
				auto_inscription_subinven[c_eff] = atoi(buf);
				if (ai_fgets(buf, 5, fp) == NULL) break;
				auto_inscription_disabled[c_eff] = atoi(buf);
			}
		} else {
			char aif, aiidp, ais, aid;
			int res;

			if (ai_fgets(buf, MAX_CHARS, fp) == NULL) break;
			res = sscanf(buf, "  %c,%c,%c,%c\n", &aif, &aiidp, &ais, &aid);
			if (res != 4) break;

			auto_inscription_force[c_eff] = (aif == 'F');
			auto_inscription_ignore[c_eff] = auto_inscription_autopickup[c_eff] = auto_inscription_autodestroy[c_eff] = FALSE;
			switch (aiidp) {
			case 'i': auto_inscription_ignore[c_eff] = TRUE; break;
			case 'a': auto_inscription_autopickup[c_eff] = TRUE; break;
			case 'A': auto_inscription_autodestroy[c_eff] = TRUE; break;
			}
			auto_inscription_subinven[c_eff] = (ais == 'b');
			auto_inscription_disabled[c_eff] = (aid == 'X');
		}

		if (c >= 0) c++;
	}
	//c_msg_print("Auto-inscriptions loaded/merged.");
	fclose(fp);

	/* Auto-convert old version files: Write new version back to disk. */
	switch (version) {
	case 1:
		c_message_add("Old auto-inscription wildcards ('?') updated to new version ('#').");
		/* fall through */
	case 2:
		c_message_add("Old auto-inscriptions updated to support auto-pickup/-destroy."); //welllll, not exactly :-s
		/* fall through */
	case 3:
		c_message_add("Old auto-inscriptions updated to support bags-only/disabled flags."); //welllll, not exactly :-s
		/* fall through */
	case 4:
		c_message_add("Old auto-inscriptions updated to new file format."); //welllll, not exactly :-s
		/* fall through */

		/* Always re-save the converted inscriptions to update them to the latest version */
		save_auto_inscriptions(name);
	}

	return(loaded_something);
}

/* Save Character-Birth file (*.dna).
 * 'touch': Keep saved values dna_sex (Sex/Body/Mode), dna_class, dna_race, dna_trait instead of updating them to newly chosen values.
 */
void save_birth_file(cptr cname, bool touch) {
	FILE *fp;
	char buf[1024];
	char file_name[256];
	int i;
	char name[CNAME_LEN + 4];
#ifdef CHARNAME_ROMAN
	char *ptr;
#endif

	strcpy(name, cname);
#ifdef CHARNAME_ROMAN
	/* Ignore roman number at the end, for players who increment after each death :) */
	if ((ptr = roman_suffix(name))) *(ptr - 1) = 0;
#endif

	strncpy(file_name, name, 249);
	file_name[249] = '\0';

	/* add '.dna' extension if not already existing */
	if (strlen(name) > 4) {
		if (name[strlen(name) - 1] == 'a' &&
		    name[strlen(name) - 2] == 'n' &&
		    name[strlen(name) - 3] == 'd' &&
		    name[strlen(name) - 4] == '.') {
			/* fine */
		} else strcat(file_name, ".dna");
	} else strcat(file_name, ".dna");

	path_build(buf, 1024, ANGBAND_DIR_USER, file_name);

	fp = fopen(buf, "w");
	if (!fp) {
		/* Couldn't write */
		c_msg_print("Saving character birth dna failed!");
		return;
	}

	/* Header (1 line) */
	fprintf(fp, "Character birth file for TomeNET v%d.%d.%d%s\n",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, CLIENT_VERSION_TAG);

	if (!touch) {
		/* Info */
		fprintf(fp, "%d\n", sex); //Sex/Body/Mode
		fprintf(fp, "%d\n", class); //Class
		fprintf(fp, "%d\n", race); //Race
		fprintf(fp, "%d\n", trait); //Trait

		/* Stats */
		for (i = 0; i < C_ATTRIBUTES; i++)
			fprintf(fp, "%d\n", stat_order[i]);
	} else {
		/* Info */
		fprintf(fp, "%d\n", dna_sex); //Sex/Body/Mode
		fprintf(fp, "%d\n", dna_class); //Class
		fprintf(fp, "%d\n", dna_race); //Race
		fprintf(fp, "%d\n", dna_trait); //Trait

		/* Stats */
		for (i = 0; i < C_ATTRIBUTES; i++)
			fprintf(fp, "%d\n", dna_stat_order[i]);
	}

	/* Done */
	fclose(fp);
}

/* Load Character-Birth file (*.dna) */
void load_birth_file(cptr cname) {
	FILE *fp;
	char buf[1024];
	char file_name[256];
	char name[CNAME_LEN + 4];
#ifdef CHARNAME_ROMAN
	char *ptr;
#endif

	int vm = 0, vi = 0, vp = 0;
	char vt = '%', *p;
	bool update = FALSE;
	int tmp, i, r;


	/* Assume invalid until we completely loaded it successfully */
	valid_dna = FALSE;

	strcpy(name, cname);
#ifdef CHARNAME_ROMAN
	/* Ignore roman number at the end, for players who increment after each death :) */
	if ((ptr = roman_suffix(name))) *(ptr - 1) = 0;
#endif

	strncpy(file_name, name, 249);
	file_name[249] = '\0';

	/* add '.dna' extension if not already existing */
	if (strlen(name) > 4) {
		if (name[strlen(name) - 1] == 'a' &&
		    name[strlen(name) - 2] == 'n' &&
		    name[strlen(name) - 3] == 'd' &&
		    name[strlen(name) - 4] == '.') {
			/* fine */
		} else strcat(file_name, ".dna");
	} else strcat(file_name, ".dna");

	path_build(buf, 1024, ANGBAND_DIR_USER, file_name);

	fp = fopen(buf, "r");
	if (!fp) {
		/* Couldn't open */
		return;
	}

	/* Header (1 line) */
	if (fgets(buf, 80, fp) == NULL) {
		fclose(fp);
		return;
	}

	/* scan header for version */
	if ((p = strstr(buf, "TomeNET v"))) {
		vm = atoi(p + 9);
		vi = atoi(p + 11);
		vp = atoi(p + 13);
		vt = *(p + 14);
		if (!vt) vt = '-';//note: '-' > '%'
	} else {
		fclose(fp);
		return; //error reading file format
	}

	/* Info */
	r = fscanf(fp, "\n%d", &tmp); //Sex/Body/Mode
	if (r == EOF || r == 0) {
		fclose(fp);
		return; // Failed to read from file
	}
	dna_sex = (s16b)tmp;

	/* Translate outdated flags */
	if (vm < 4 || (vm == 4 && vi < 7) || (vm == 4 && vi == 7 && vp < 1) || (vm == 4 && vi == 7 && vp == 1 && vt < 'b')) { //note that '-' and '%' are also < 'b'
		update = TRUE;
		if (dna_sex & MODE_MALE_OLD) {
			dna_sex &= ~MODE_MALE_OLD;
			dna_sex |= MODE_MALE;
		}
		if (dna_sex & MODE_FRUIT_BAT_OLD) {
			dna_sex &= ~MODE_FRUIT_BAT_OLD;
			dna_sex |= MODE_FRUIT_BAT;
		}
	}

	r = fscanf(fp, "\n%d", &tmp); //Class
	if (r == EOF || r == 0) {
		fclose(fp);
		return; // Failed to read from file
	}
	dna_class = (s16b)tmp;

	dna_class_title = class_info[dna_class].title;
	/* Hack class names of 'hidden' classes to reflect their base class from which they stem */
	if (!is_newer_than(&server_version, 4, 7, 0, 2, 0, 0)) { //old way: hardcoded
		/* Unhack for ENABLE_DEATHKNIGHT: Share slot with Paladin class choice */
		if (dna_class == CLASS_DEATHKNIGHT) dna_class = CLASS_PALADIN;
	} else if (class_info[dna_class].hidden) dna_class = class_info[dna_class].base_class;

	r = fscanf(fp, "\n%d", &tmp); //Race
	if (r == EOF || r == 0) {
		fclose(fp);
		return; // Failed to read from file
	}
	dna_race = (s16b)tmp;
	r = fscanf(fp, "\n%d", &tmp); //Trait
	if (r == EOF || r == 0) {
		fclose(fp);
		return; // Failed to read from file
	}
	dna_trait = (s16b)tmp;

	/* Stats */
	for (i = 0; i < C_ATTRIBUTES; i++) {
		r = fscanf(fp, "\n%d", &tmp);
		if (r == EOF || r == 0) {
			fclose(fp);
			return; // Failed to read from file
		}
		dna_stat_order[i] = (s16b)tmp;
	}

#if 0 /* for future enhancements */
	/* > 4.5.8? */
	if (vm > 4 || (vm == 4 && (vi > 5 || (vi == 5 && (vp > 8 || (vp == 8 && vt > '-')))))) {
	}
#endif

	/* Validate */
	valid_dna = TRUE; //Safety for mis-hacked dna files in future? - Kurzel

	/* Done */
	fclose(fp);

	/* Update an outdated dna file version? */
	if (update) save_birth_file(name, TRUE);
}

/* Check if our Guide is outdated -- only do this once on initial client startup, not on every relog (retry_contact).
   Also do this when explicitely requested (eg via =U or /reinit_guide). */
#ifdef WINDOWS
 #include <process.h> /* for _spawnl() */
#endif
int check_guide_checksums(bool forced) {
	FILE *fp;
	char buf[MAX_CHARS_WIDE], buf2[MAX_CHARS_WIDE], *c;

	/* TODO: Make this a .tomenetrc / tomenet.ini switch */
#ifdef WINDOWS
 #if 1	/* 1: Don't check guide checksums on client startup. \
	      Disabled now as nobody has the required sha256sum.exe/bat files in their 'updater' folder anyway. Reenable on next release. */
	/* (guide_outdated remains FALSE) */
	if (!forced) return(0);
 #endif
#else /* Assume POSIX - We assume that sha256sum is probably available on POSIX, so this could be worth using already. */
 #if 1	/* 1: Don't check guide checksums on client startup. We have = C now, so this isn't that important anymore. */
	/* (guide_outdated remains FALSE) */
	if (!forced) return(0);
 #endif
#endif

	/* Do we have sha256sum tool? */
#ifdef WINDOWS
	if (!my_fexists("updater\\sha256sum.bat"))
	//if (access("updater\\sha256sum.bat", F_OK))
#else /* assume POSIX */
	if (system("sha256sum --version"))
#endif
	{
		//logprint("Warning: No sha256sum found, cannot auto-check guide for outdatedness.\n"); --could be spammy
		guide_outdated = FALSE;
		c_msg_print("\377yError verifying Guide version: 'sha256sum' is not installed.");
		return(1);
	}

	buf2[0] = buf[0] = 0;
#ifdef WINDOWS
	(void)_spawnl(_P_WAIT, "updater\\sha256sum.bat", "updater\\sha256sum.bat", NULL);
#else /* assume POSIX */
	(void)system("sha256sum TomeNET-Guide.txt > TomeNET-Guide.sha256.local");
#endif
	fp = fopen("TomeNET-Guide.sha256.local", "r");
	if (fp) {
		if (fgets(buf2, MAX_CHARS_WIDE - 1, fp) == NULL) {
			c_msg_print("\377yError verifying Guide version: 'TomeNET-Guide.sha256.local' is empty.");
			fclose(fp);
			return(4);
		}
		buf[MAX_CHARS_WIDE - 1] = 0; //paranoia
		fclose(fp);
	} else {
		c_msg_print("\377yError verifying Guide version: Cannot read 'TomeNET-Guide.sha256.local'.");
		return(2);
	}
	remove("TomeNET-Guide.sha256.local");
	if ((c = strchr(buf2, ' '))) *c = 0; //cut off file name, only keep the actual hash
#ifdef WINDOWS
	(void)_spawnl(_P_WAIT, "updater\\wget.exe", "wget.exe", "--dot-style=mega", "https://www.tomenet.eu/TomeNET-Guide.sha256", NULL);
#else /* assume POSIX */
	(void)system("wget --timeout=3 https://www.tomenet.eu/TomeNET-Guide.sha256");
#endif
	fp = fopen("TomeNET-Guide.sha256", "r");
	if (fp) {
		if (fgets(buf, MAX_CHARS_WIDE - 1, fp) == NULL) {
			c_msg_print("\377yError verifying Guide version: 'TomeNET-Guide.sha256' is empty.");
			fclose(fp);
			return(5);
		}
		buf[MAX_CHARS_WIDE - 1] = 0; //paranoia
		fclose(fp);
	} else {
		c_msg_print("\377yError verifying Guide version: Cannot download 'TomeNET-Guide.sha256'.");
		return(3);
	}
	remove("TomeNET-Guide.sha256");
	if ((c = strchr(buf, ' '))) *c = 0; //cut off file name, only keep the actual hash
	//printf("old <%s>, new <%s>\n", buf2, buf);
	guide_outdated = strcmp(buf2, buf);

	//Must be disabled if we ever call check_guide_checksums() before the window system is initialized (eg early on in main()), or segfault:
	//if (guide_outdated) c_msg_print("\377yYour Guide is outdated. You can update it in-game now by pressing: \377s= U");

	return(0);
}
