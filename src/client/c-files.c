/* $Id$ */
/*
 * Handle client-side files, such as the .mangrc configuration
 * file, and some various "pref files".
 */

#include "angband.h"

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
s16b tokenize(char *buf, s16b num, char **tokens)
{
        int i = 0;

        char *s = buf;


        /* Process */
        while (i < num - 1)
        {
                char *t;

                /* Scan the string */
                for (t = s; *t; t++)
                {
                        /* Found a delimiter */
                        if ((*t == ':') || (*t == '/')) break;

                        /* Handle single quotes */
                        if (*t == '\'')
                        {
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
        return (i);
}



/*
 * Convert a octal-digit into a decimal
 */
static int deoct(char c)
{
        if (isdigit(c)) return (D2I(c));
        return (0);
}

/*
 * Convert a hexidecimal-digit into a decimal
 */
static int dehex(char c)
{
        if (isdigit(c)) return (D2I(c));
        if (islower(c)) return (A2I(c) + 10);
        if (isupper(c)) return (A2I(tolower(c)) + 10);
        return (0);
}


/*
 * Hack -- convert a printable string into real ascii
 *
 * I have no clue if this function correctly handles, for example,
 * parsing "\xFF" into a (signed) char.  Whoever thought of making
 * the "sign" of a "char" undefined is a complete moron.  Oh well.
 */
void text_to_ascii(char *buf, cptr str)
{
        char *s = buf;

        /* Analyze the "ascii" string */
        while (*str)
        {
                /* Backslash codes */
                if (*str == '\\')
                {
                        /* Skip the backslash */
                        str++;

                        /* Hex-mode XXX */
                        if (*str == 'x')
                        {
                                *s = 16 * dehex(*++str);
                                *s++ += dehex(*++str);
                        }

                        /* Hack -- simple way to specify "backslash" */
                        else if (*str == '\\')
                        {
                                *s++ = '\\';
                        }

                        /* Hack -- simple way to specify "caret" */
                        else if (*str == '^')
                        {
                                *s++ = '^';
                        }

                        /* Hack -- simple way to specify "space" */
                        else if (*str == 's')
                        {
                                *s++ = ' ';
                        }

                        /* Hack -- simple way to specify Escape */
                        else if (*str == 'e')
                        {
                                *s++ = ESCAPE;
                        }

                        /* Backspace */
                        else if (*str == 'b')
                        {
                                *s++ = '\b';
                        }

                        /* Newline */
                        else if (*str == 'n')
                        {
                                *s++ = '\n';
                        }

                        /* Return */
                        else if (*str == 'r')
                        {
                                *s++ = '\r';
                        }

                        /* Tab */
                        else if (*str == 't')
                        {
                                *s++ = '\t';
                        }

                        /* Octal-mode */
                        else if (*str == '0')
                        {
                                *s = 8 * deoct(*++str);
                                *s++ += deoct(*++str);
                        }

                        /* Octal-mode */
                        else if (*str == '1')
                        {
                                *s = 64 + 8 * deoct(*++str);
                                *s++ += deoct(*++str);
                        }

                        /* Octal-mode */
                        else if (*str == '2')
                        {
                                *s = 64 * 2 + 8 * deoct(*++str);
                                *s++ += deoct(*++str);
                        }

                        /* Octal-mode */
                        else if (*str == '3')
                        {
                                *s = 64 * 3 + 8 * deoct(*++str);
                                *s++ += deoct(*++str);
                        }

                        /* Skip the final char */
                        str++;
                }

                /* Normal Control codes */
                else if (*str == '^')
                {
                        str++;
                        *s++ = (*str++ & 037);
                }

                /* Normal chars */
                else
                {
                        *s++ = *str++;
                }
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
errr path_parse(char *buf, cptr file)
{
#ifndef WIN32
#ifndef AMIGA
        cptr            u, s;
        struct passwd   *pw;
        char            user[128];
#endif
#endif /* WIN32 */


        /* Assume no result */
        buf[0] = '\0';

        /* No file? */
        if (!file) return (-1);

        /* File needs no parsing */
        if (file[0] != '~')
        {
                strcpy(buf, file);
                return (0);
        }

	/* Windows should never have ~ in filename */
#ifndef WIN32
#ifndef AMIGA

        /* Point at the user */
        u = file+1;

        /* Look for non-user portion of the file */
        s = strstr(u, PATH_SEP);

        /* Hack -- no long user names */
        if (s && (s >= u + sizeof(user))) return (1);

        /* Extract a user name */
        if (s)
        {
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
        if (!pw) return (1);

        /* Make use of the info */
        (void)strcpy(buf, pw->pw_dir);

        /* Append the rest of the filename, if any */
        if (s) (void)strcat(buf, s);

#endif
#endif /* WIN32 */
        /* Success */
        return (0);
}



/*
 * Hack -- replacement for "fopen()"
 */
FILE *my_fopen(cptr file, cptr mode)
{
        char                buf[1024];

        /* Hack -- Try to parse the path */
        if (path_parse(buf, file)) return (NULL);

        /* Attempt to fopen the file anyway */
        return (fopen(buf, mode));
}


/*
 * Hack -- replacement for "fclose()"
 */
errr my_fclose(FILE *fff)
{
        /* Require a file */
        if (!fff) return (-1);

        /* Close, check for error */
        if (fclose(fff) == EOF) return (1);

        /* Success */
        return (0);
}

/*
 * MetaHack -- check if the specified file already exists	- Jir -
 */
bool my_freadable(cptr file)
{
	FILE *fff;
	fff = my_fopen(file, "r");

	if (fff) return (FALSE);
	
	my_fclose(fff);
	return (TRUE);
}

/*
 * Get a safe filename from given strings.	- Jir -
 * eg. "./lib/text/Jir" => "./lib/text/Jir0004.txt"
 *
 * TODO: handle extender neatly. REWRITE ME
 */
errr get_safe_file(char *buf, cptr file)
{
	int i;
	char tmp[1024];

	/* Assume no result */
	buf[0] = '\0';

	/* No file? */
	if (!file) return (-1);

	for (i = 1; i <= 9999; i++)
	{
		sprintf(tmp, "%s%#4d.txt", file, i);
		if (my_freadable(tmp)) continue;

		strcpy(buf, tmp);
		return (0);
	}
	
	/* Failure */
	return (-1);
}



/*
 * Hack -- replacement for "fgets()"
 *
 * Read a string, without a newline, to a file
 *
 * Process tabs, strip internal non-printables
 */
errr my_fgets(FILE *fff, char *buf, huge n)
{
        huge i = 0;

        char *s;

        char tmp[1024];

        /* Read a line */
        if (fgets(tmp, 1024, fff))
        {
                /* Convert weirdness */
                for (s = tmp; *s; s++)
                {
                        /* Handle newline */
                        if (*s == '\n')
                        {
                                /* Terminate */
                                buf[i] = '\0';

                                /* Success */
                                return (0);
                        }

                        /* Handle tabs */
                        else if (*s == '\t')
                        {
                                /* Hack -- require room */
                                if (i + 8 >= n) break;

                                /* Append a space */
                                buf[i++] = ' ';

                                /* Append some more spaces */
                                while (!(i % 8)) buf[i++] = ' ';
                        }

                        /* Handle printables */
                        else if (isprint(*s))
                        {
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
        return (1);
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
void init_file_paths(char *path)
{
        char *tail;


        /*** Free everything ***/

        /* Free the main path */
        string_free(ANGBAND_DIR);

        /* Free the sub-paths */
        string_free(ANGBAND_DIR_APEX);
        string_free(ANGBAND_DIR_BONE);
        string_free(ANGBAND_DIR_DATA);
        string_free(ANGBAND_DIR_EDIT);
        string_free(ANGBAND_DIR_FILE);
        string_free(ANGBAND_DIR_HELP);
        string_free(ANGBAND_DIR_INFO);
        string_free(ANGBAND_DIR_SAVE);
        string_free(ANGBAND_DIR_USER);
        string_free(ANGBAND_DIR_XTRA);
        string_free(ANGBAND_DIR_SCPT);


        /*** Prepare the "path" ***/

        /* Hack -- save the main directory */
        ANGBAND_DIR = string_make(path);

        /* Prepare to append to the Base Path */
        tail = path + strlen(path);


#ifdef VM


        /*** Use "flat" paths with VM/ESA ***/

        /* Use "blank" path names */
        ANGBAND_DIR_APEX = string_make("");
        ANGBAND_DIR_BONE = string_make("");
        ANGBAND_DIR_DATA = string_make("");
        ANGBAND_DIR_EDIT = string_make("");
        ANGBAND_DIR_FILE = string_make("");
        ANGBAND_DIR_HELP = string_make("");
        ANGBAND_DIR_INFO = string_make("");
        ANGBAND_DIR_SAVE = string_make("");
        ANGBAND_DIR_USER = string_make("");
        ANGBAND_DIR_SCPT = string_make("");
        ANGBAND_DIR_XTRA = string_make("");


#else /* VM */


        /*** Build the sub-directory names ***/

        /* Build a path name */
        strcpy(tail, "apex");
        ANGBAND_DIR_APEX = string_make(path);

        /* Build a path name */
        strcpy(tail, "scpt");
        ANGBAND_DIR_SCPT = string_make(path);

        /* Build a path name */
        strcpy(tail, "bone");
        ANGBAND_DIR_BONE = string_make(path);

        /* Build a path name */
        strcpy(tail, "data");
        ANGBAND_DIR_DATA = string_make(path);

        /* Build a path name */
        strcpy(tail, "edit");
        ANGBAND_DIR_EDIT = string_make(path);

        /* Build a path name */
        strcpy(tail, "file");
        ANGBAND_DIR_FILE = string_make(path);

        /* Build a path name */
//        strcpy(tail, "help");
        strcpy(tail, "text");
        ANGBAND_DIR_HELP = string_make(path);

        /* Build a path name */
        strcpy(tail, "info");
        ANGBAND_DIR_INFO = string_make(path);

        /* Build a path name */
        strcpy(tail, "save");
        ANGBAND_DIR_SAVE = string_make(path);

        /* Build a path name */
        strcpy(tail, "user");
        ANGBAND_DIR_USER = string_make(path);

        /* Build a path name */
        strcpy(tail, "xtra");
        ANGBAND_DIR_XTRA = string_make(path);

#endif /* VM */


#ifdef NeXT

        /* Allow "fat binary" usage with NeXT */
        if (TRUE)
        {
                cptr next = NULL;

# if defined(m68k)
                next = "m68k";
# endif

# if defined(i386)
                next = "i386";
# endif

# if defined(sparc)
                next = "sparc";
# endif

# if defined(hppa)
                next = "hppa";
# endif

                /* Use special directory */
                if (next)
                {
                        /* Forget the old path name */
                        string_free(ANGBAND_DIR_DATA);

                        /* Build a new path name */
                        sprintf(tail, "data-%s", next);
                        ANGBAND_DIR_DATA = string_make(path);
                }
        }

#endif /* NeXT */

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
 */
errr process_pref_file_aux(char *buf)
{
        int i, j, k;
	int n1, n2;

        char *zz[16];


        /* Skip "empty" lines */
        if (!buf[0]) return (0);

        /* Skip "blank" lines */
        if (isspace(buf[0])) return (0);

        /* Skip comments */
        if (buf[0] == '#') return (0);


        /* Require "?:*" format */
        if (buf[1] != ':') return (1);


        /* Process "%:<fname>" */
        if (buf[0] == '%')
        {
                /* Attempt to Process the given file */
                return (process_pref_file(buf + 2));
        }


        /* Process "R:<num>:<a>/<c>" -- attr/char for monster races */
        if (buf[0] == 'R')
        {
                if (tokenize(buf+2, 3, zz) == 3)
                {
                        i = (huge)strtol(zz[0], NULL, 0);
						i += 12;	/* gfx-fix by Tanix */
                        n1 = strtol(zz[1], NULL, 0);
                        n2 = strtol(zz[2], NULL, 0);
                        if (i >= MAX_R_IDX) return (1);
                        if (n1) Client_setup.r_attr[i] = n1;
                        if (n2) Client_setup.r_char[i] = n2;
                        return (0);
                }
        }


        /* Process "K:<num>:<a>/<c>"  -- attr/char for object kinds */
        else if (buf[0] == 'K')
        {
                if (tokenize(buf+2, 3, zz) == 3)
                {
                        i = (huge)strtol(zz[0], NULL, 0);
                        n1 = strtol(zz[1], NULL, 0);
                        n2 = strtol(zz[2], NULL, 0);
                        if (i >= MAX_K_IDX) return (1);
                        if (n1) Client_setup.k_attr[i] = n1;
                        if (n2) Client_setup.k_char[i] = n2;
                        return (0);
                }
        }


        /* Process "F:<num>:<a>/<c>" -- attr/char for terrain features */
        else if (buf[0] == 'F')
        {
                if (tokenize(buf+2, 3, zz) == 3)
                {
                        i = (huge)strtol(zz[0], NULL, 0);
                        n1 = strtol(zz[1], NULL, 0);
                        n2 = strtol(zz[2], NULL, 0);
                        // if (i >= MAX_F_IDX) return (1);
                        if (n1) Client_setup.f_attr[i] = n1;
                        if (n2) Client_setup.f_char[i] = n2;
                        return (0);
                }
        }


        /* Process "U:<tv>:<a>/<c>" -- attr/char for unaware items */
        else if (buf[0] == 'U')
        {
                if (tokenize(buf+2, 3, zz) == 3)
                {
                        j = (huge)strtol(zz[0], NULL, 0);
                        n1 = strtol(zz[1], NULL, 0);
                        n2 = strtol(zz[2], NULL, 0);
			if (j > 100) return 0;
			if (n1) Client_setup.u_attr[j] = n1;
			if (n2) Client_setup.u_char[j] = n2;
                        return (0);
                }
        }


        /* Process "E:<tv>:<a>/<c>" -- attr/char for equippy chars */
        else if (buf[0] == 'E')
        {
		/* Do nothing */
		return (0);

#if 0
                if (tokenize(buf+2, 3, zz) == 3)
                {
                        j = (byte)strtol(zz[0], NULL, 0) % 128;
                        n1 = strtol(zz[1], NULL, 0);
                        n2 = strtol(zz[2], NULL, 0);
                        if (n1) tval_to_attr[j] = n1;
                        if (n2) tval_to_char[j] = n2;
                        return (0);
                }
#endif
        }

        /* Process "A:<str>" -- save an "action" for later */
        else if (buf[0] == 'A')
        {
                text_to_ascii(macro__buf, buf+2);
                return (0);
        }

        /* Process "P:<str>" -- create normal macro */
        else if (buf[0] == 'P')
        {
                char tmp[1024];
                text_to_ascii(tmp, buf+2);
                macro_add(tmp, macro__buf, FALSE);
                return (0);
        }

        /* Process "C:<str>" -- create command macro */
        else if (buf[0] == 'C')
        {
                char tmp[1024];
                text_to_ascii(tmp, buf+2);
                macro_add(tmp, macro__buf, TRUE);
                return (0);
        }


        /* Process "S:<key>:<key>:<dir>" -- keymap */
        else if (buf[0] == 'S')
        {
                if (tokenize(buf+2, 3, zz) == 3)
                {
                        i = strtol(zz[0], NULL, 0) & 0x7F;
                        j = strtol(zz[0], NULL, 0) & 0x7F;
                        k = strtol(zz[0], NULL, 0) & 0x7F;
                        if ((k > 9) || (k == 5)) k = 0;
                        keymap_cmds[i] = j;
                        keymap_dirs[i] = k;
                        return (0);
                }
        }


        /* Process "V:<num>:<kv>:<rv>:<gv>:<bv>" -- visual info */
        else if (buf[0] == 'V')
        {
		/* Do nothing */
		return (0);

                if (tokenize(buf+2, 5, zz) == 5)
                {
                        i = (byte)strtol(zz[0], NULL, 0);
                        color_table[i][0] = (byte)strtol(zz[1], NULL, 0);
                        color_table[i][1] = (byte)strtol(zz[2], NULL, 0);
                        color_table[i][2] = (byte)strtol(zz[3], NULL, 0);
                        color_table[i][3] = (byte)strtol(zz[4], NULL, 0);
                        return (0);
                }
        }


        /* Process "X:<str>" -- turn option off */
        else if (buf[0] == 'X')
        {
                for (i = 0; option_info[i].o_desc; i++)
                {
                        if (option_info[i].o_var &&
                            option_info[i].o_text &&
                            streq(option_info[i].o_text, buf + 2))
                        {
                                (*option_info[i].o_var) = FALSE;
				Client_setup.options[i] = FALSE;
                                return (0);
                        }
                }
        }

        /* Process "Y:<str>" -- turn option on */
        else if (buf[0] == 'Y')
        {
                for (i = 0; option_info[i].o_desc; i++)
                {
                        if (option_info[i].o_var &&
                            option_info[i].o_text &&
                            streq(option_info[i].o_text, buf + 2))
                        {
                                (*option_info[i].o_var) = TRUE;
				Client_setup.options[i] = TRUE;
                                return (0);
                        }
                }
        }

	/* Process "W:<num>:<use>" -- specify window action */
	else if (buf[0] == 'W')
	{
		if (tokenize(buf+2, 2, zz) == 2)
		{
			i = (byte)strtol(zz[0], NULL, 0);
			window_flag[i] = 1L << ((byte)strtol(zz[1], NULL, 0));
			return (0);
		}
	}


        /* Failure */
        return (1);
}


/*
 * Process the "user pref file" with the given name
 *
 * See the function above for a list of legal "commands".
 */
errr process_pref_file(cptr name)
{
        FILE *fp;

        char buf[1024];


        /* Build the filename */
        path_build(buf, 1024, ANGBAND_DIR_USER, name);

        /* Open the file */
        fp = my_fopen(buf, "r");

        /* Catch errors */
        if (!fp) return (-1);

        /* Process the file */
        while (0 == my_fgets(fp, buf, 1024))
        {
                /* Process the line */
                if (process_pref_file_aux(buf))
                {
                        /* Useful error message */
                        printf("Error in '%s' parsing '%s'.", buf, name);
                }
        }

        /* Close the file */
        my_fclose(fp);

        /* Success */
        return (0);
}


/*
 * Show the Message of the Day.
 *
 * It is given in the "Setup" info sent by the server.
 */
void show_motd(void)
{
	int i;
	char ch;

	/* Save the old screen */
	Term_save();

	/* Clear the screen */
	Term_clear();

	for (i = 0; i < 23; i++)
	{
		/* Show each line */
		Term_putstr(0, i, -1, TERM_WHITE, &Setup.motd[i * 80]);
	}

	/* Show it */
	Term_fresh();

	/* Wait for a keypress */
//	Term_inkey(&ch, TRUE, TRUE);
	while (!inkey()) /* nothing -- it saves us from getting disconnected */;

	/* Reload the old screen */
	Term_load();

	/* Clear the screen again */
//	Term_clear();
}

/*
 * Peruse a file sent by the server.
 *
 * This is used by the artifact list, the unique list, the player
 * list, *Identify*, and Self Knowledge.
 *
 * It may eventually be used for help file perusal as well.
 */
void peruse_file(void)
{
	char k; 

	/* Initialize */
	cur_line = 0;

	/* Save the old screen */
	Term_save();

	/* Show the stuff */
	while (TRUE)
	{
		/* Clear the screen */
		Term_clear();

		/* Send the command */
		Send_special_line(special_line_type, cur_line);

		/* Show a general "title" */
		prt(format("[%s]", shortVersion), 0, 0);
/*		prt(format("[TomeNET %d.%d.%d]",
			VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH), 0, 0);	*/


		/* Prompt */
//		prt("[Press Return, Space, -, b, or ESC to exit.]", 23, 0);
		prt(format("[Press Return, Space, -, b, or ESC to exit.] (%d/%d)",
					cur_line, max_line), 23, 0);

		/* Get a keypress */
		k = inkey();

		/* Hack -- go to a specific line */
		if (k == '#')
		{
			char tmp[80];
//			prt("Goto Line: ", 23, 0);
			prt(format("Goto Line(max %d): ", max_line), 23, 0);
			strcpy(tmp, "0");
			if (askfor_aux(tmp, 10, 0))
			{
				cur_line = atoi(tmp);
			}
		}

		/* Hack -- Allow backing up */
		if (k == '-')
		{
			cur_line -= 10;
		}

		/* Hack -- Allow backing up ala 'less' */
		if (k == 'b' || k == 'p' || k == KTRL('U'))
		{
			cur_line -= 20;
		}

		/* Hack -- Advance one line */
		if ((k == '\n') || (k == '\r') || (k == 'j') || k == '2')
		{
			cur_line++;
		}

		/* Hack -- backing up one line */
		if (k == 'k' || k == '8')
		{
			cur_line--;
		}

		/* Advance one page */
		if (k == ' ' || k == KTRL('D'))
		{
			cur_line += 20;
		}

		/* Hack -- back to the top */
		if (k == 'g')
		{
			cur_line = 0;
		}

		/* Hack -- go to the bottom (it's vi, u know ;) */
		if (k == 'G')
		{
			cur_line = max_line - 20;
		}

		/* Exit on escape */
		if (k == ESCAPE || k == KTRL('X')) break;

		/* Check maximum line */
		if (cur_line > max_line || cur_line < 0)
			cur_line = 0;
	}

	/* Tell the server we're done looking */
	Send_special_line(SPECIAL_FILE_NONE, 0);

	/* No longer using file perusal */
	special_line_type = 0;

	/* Reload the old screen */
	Term_load();

	/* Flush any events that came in */
	Flush_queue();
}

/*
 * Hack -- Dump a character description file
 *
 * XXX XXX XXX Allow the "full" flag to dump additional info,
 * and trigger its usage from various places in the code.
 */
errr file_character(cptr name, bool full)
{
	int			i, j, x, y;
	byte		a;
	char		c;
	cptr		paren = ")";
	int			fd = -1;
	FILE		*fff = NULL;
	store_type  *st_ptr;
	char		o_name[80];
	char		buf[1024];
	bool            spell_first = FALSE;


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, name);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the non-existing file */
	if (fd < 0) fff = my_fopen(buf, "w");


	/* Invalid file */
	if (!fff)
	{
		/* Message */
		c_msg_print("Character dump failed!");
		c_msg_print(NULL);

		/* Error */
		return (-1);
	}


	/* Begin dump */
	fprintf(fff, "  [TomeNET %d.%d.%d @ %s Character Dump]\n\n",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, server_name);


	/* Display player */
	display_player(0);

	/* Dump part of the screen */
	for (y = 2; y < 22; y++)
	{
		/* Dump each row */
		for (x = 0; x < 79; x++)
		{
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));

			/* Dump it */
			buf[x] = c;
		}

		/* Terminate */
		buf[x] = '\0';

		/* End the row */
		fprintf(fff, "%s\n", buf);
	}

	/* Display history */
	display_player(1);

	/* Dump part of the screen */
	for (y = 15; y < 20; y++)
	{
		/* Dump each row */
		for (x = 0; x < 79; x++)
		{
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));

			/* Dump it */
			buf[x] = c;
		}

		/* Terminate */
		buf[x] = '\0';

		/* End the row */
		fprintf(fff, "%s\n", buf);
	}

#if 0 // DGDGDGDG -- make me work
	/* body_monster is shown in Character screen instead.	- Jir - */

	{
		char desc[80];
		monster_race_desc(desc, p_ptr->body_monster, 0);
		fprintf(fff, "\n Your body was %s.", desc);

		if (p_ptr->tim_mimic) fprintf(fff, "\n Your were disguised into a %s.", p_ptr->mimic_name);
	}
#endif
	/* Dump skills */
	dump_skills(fff);

#if 0 // DGDGDG - make me work
	/* Well, sorry, that'd be hard. We should put new 'connection-status'
	 * for tombscreen maybe..	- Jir */

	/* Monsters slain */
	{
		int k;
		s32b Total = 0;

		for (k = 1; k < max_r_idx-1; k++)
		{
			monster_race *r_ptr = &r_info[k];

			if (r_ptr->flags1 & (RF1_UNIQUE))
			{
				bool dead = (r_ptr->max_num == 0);
				if (dead)
				{
					Total++;
				}
			}
			else
			{
				s16b This = r_ptr->r_pkills;
				if (This > 0)
				{
					Total += This;
				}
			}
		}

		if (Total < 1)
			fprintf(fff,"\n You have defeated no enemies yet.\n");
		else if (Total == 1)
			fprintf(fff,"\n You have defeated one enemy.\n");
		else
			fprintf(fff,"\n You have defeated %lu enemies.\n", Total);
	}
#endif

	/* Skip some lines */
	fprintf(fff, "\n\n");


	/* Dump the equipment */
	fprintf(fff, "  [Character Equipment]\n\n");
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		fprintf(fff, "%c%s %s\n",
				index_to_label(i), paren, inventory_name[i]);
	}
	fprintf(fff, "\n\n");

	/* Dump the inventory */
	fprintf(fff, "  [Character Inventory]\n\n");
	for (i = 0; i < INVEN_PACK; i++)
	{
		if (!strncmp(inventory_name[i], "(nothing)", 9)) continue;
//		if (!strlen(inventory_name[i])) continue;

		fprintf(fff, "%c%s %s\n",
				index_to_label(i), paren, inventory_name[i]);
	}
	fprintf(fff, "\n\n");

	/* Dump the last messages */
	fprintf(fff, "  [Last Messages]\n\n");
	dump_messages(fff, 50);
	fprintf(fff, "\n\n");

	/* Close it */
	my_fclose(fff);


	/* Message */
	c_msg_print("Character dump successful.");
	c_msg_print(NULL);

	/* Success */
	return (0);
}
