/* File: util.c */

/* Purpose: Angband utilities -BEN- */


#define SERVER

#include "angband.h"




#ifndef HAS_MEMSET

/*
 * For those systems that don't have "memset()"
 *
 * Set the value of each of 'n' bytes starting at 's' to 'c', return 's'
 * If 'n' is negative, you will erase a whole lot of memory.
 */
char *memset(char *s, int c, huge n)
{
	char *t;
	for (t = s; len--; ) *t++ = c;
	return (s);
}

#endif



#ifndef HAS_STRICMP

/*
 * For those systems that don't have "stricmp()"
 *
 * Compare the two strings "a" and "b" ala "strcmp()" ignoring case.
 */
int stricmp(cptr a, cptr b)
{
	cptr s1, s2;
	char z1, z2;

	/* Scan the strings */
	for (s1 = a, s2 = b; TRUE; s1++, s2++)
	{
		z1 = FORCEUPPER(*s1);
		z2 = FORCEUPPER(*s2);
		if (z1 < z2) return (-1);
		if (z1 > z2) return (1);
		if (!z1) return (0);
	}
}

#endif


#ifdef SET_UID

# ifndef HAS_USLEEP

/*
 * For those systems that don't have "usleep()" but need it.
 *
 * Fake "usleep()" function grabbed from the inl netrek server -cba
 */
static int usleep(huge microSeconds)
{
	struct timeval      Timer;

	int                 nfds = 0;

#ifdef FD_SET
	fd_set          *no_fds = NULL;
#else
	int                     *no_fds = NULL;
#endif


	/* Was: int readfds, writefds, exceptfds; */
	/* Was: readfds = writefds = exceptfds = 0; */


	/* Paranoia -- No excessive sleeping */
	if (microSeconds > 4000000L) core("Illegal usleep() call");


	/* Wait for it */
	Timer.tv_sec = (microSeconds / 1000000L);
	Timer.tv_usec = (microSeconds % 1000000L);

	/* Wait for it */
	if (select(nfds, no_fds, no_fds, no_fds, &Timer) < 0)
	{
		/* Hack -- ignore interrupts */
		if (errno != EINTR) return -1;
	}

	/* Success */
	return 0;
}

# endif


/*
 * Hack -- External functions
 */
extern struct passwd *getpwuid();
extern struct passwd *getpwnam();


/*
 * Find a default user name from the system.
 */
void user_name(char *buf, int id)
{
	struct passwd *pw;

	/* Look up the user name */
	if ((pw = getpwuid(id)))
	{
		(void)strcpy(buf, pw->pw_name);
		buf[16] = '\0';

#ifdef CAPITALIZE_USER_NAME
		/* Hack -- capitalize the user name */
		if (islower(buf[0])) buf[0] = toupper(buf[0]);
#endif

		return;
	}

	/* Oops.  Hack -- default to "PLAYER" */
	strcpy(buf, "PLAYER");
}

#endif /* SET_UID */




/*
 * The concept of the "file" routines below (and elsewhere) is that all
 * file handling should be done using as few routines as possible, since
 * every machine is slightly different, but these routines always have the
 * same semantics.
 *
 * In fact, perhaps we should use the "path_parse()" routine below to convert
 * from "canonical" filenames (optional leading tilde's, internal wildcards,
 * slash as the path seperator, etc) to "system" filenames (no special symbols,
 * system-specific path seperator, etc).  This would allow the program itself
 * to assume that all filenames are "Unix" filenames, and explicitly "extract"
 * such filenames if needed (by "path_parse()", or perhaps "path_canon()").
 *
 * Note that "path_temp" should probably return a "canonical" filename.
 *
 * Note that "my_fopen()" and "my_open()" and "my_make()" and "my_kill()"
 * and "my_move()" and "my_copy()" should all take "canonical" filenames.
 *
 * Note that "canonical" filenames use a leading "slash" to indicate an absolute
 * path, and a leading "tilde" to indicate a special directory, and default to a
 * relative path, but MSDOS uses a leading "drivename plus colon" to indicate the
 * use of a "special drive", and then the rest of the path is parsed "normally",
 * and MACINTOSH uses a leading colon to indicate a relative path, and an embedded
 * colon to indicate a "drive plus absolute path", and finally defaults to a file
 * in the current working directory, which may or may not be defined.
 *
 * We should probably parse a leading "~~/" as referring to "ANGBAND_DIR". (?)
 */


#ifdef ACORN


/*
 * Most of the "file" routines for "ACORN" should be in "main-acn.c"
 */


#else /* ACORN */


#ifdef SET_UID

/*
 * Extract a "parsed" path from an initial filename
 * Normally, we simply copy the filename into the buffer
 * But leading tilde symbols must be handled in a special way
 * Replace "~user/" by the home directory of the user named "user"
 * Replace "~/" by the home directory of the current user
 */
errr path_parse(char *buf, int max, cptr file)
{
	cptr            u, s;
	struct passwd   *pw;
	char            user[128];


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

	/* Success */
	return (0);
}


#else /* SET_UID */


/*
 * Extract a "parsed" path from an initial filename
 *
 * This requires no special processing on simple machines,
 * except for verifying the size of the filename.
 */
errr path_parse(char *buf, int max, cptr file)
{
	/* Accept the filename */
	strnfmt(buf, max, "%s", file);

	/* Success */
	return (0);
}


#endif /* SET_UID */


/*
 * Hack -- acquire a "temporary" file name if possible
 *
 * This filename is always in "system-specific" form.
 */
errr path_temp(char *buf, int max)
{
	cptr s;

	/* Temp file */
	s = tmpnam(NULL);

	/* Oops */
	if (!s) return (-1);

	/* Format to length */
	strnfmt(buf, max, "%s", s);

	/* Success */
	return (0);
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
 * Hack -- replacement for "fopen()"
 */
FILE *my_fopen(cptr file, cptr mode)
{
	char                buf[1024];

	/* Hack -- Try to parse the path */
	if (path_parse(buf, 1024, file)) return (NULL);

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


#endif /* ACORN */


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
 * Hack -- replacement for "fputs()"
 *
 * Dump a string, plus a newline, to a file
 *
 * XXX XXX XXX Process internal weirdness?
 */
errr my_fputs(FILE *fff, cptr buf, huge n)
{
	/* XXX XXX */
	n = n ? n : 0;

	/* Dump, ignore errors */
	(void)fprintf(fff, "%s\n", buf);

	/* Success */
	return (0);
}


#ifdef ACORN


/*
 * Most of the "file" routines for "ACORN" should be in "main-acn.c"
 *
 * Many of them can be rewritten now that only "fd_open()" and "fd_make()"
 * and "my_fopen()" should ever create files.
 */


#else /* ACORN */


/*
 * Code Warrior is a little weird about some functions
 */
#ifdef BEN_HACK
extern int open(const char *, int, ...);
extern int close(int);
extern int read(int, void *, unsigned int);
extern int write(int, const void *, unsigned int);
extern long lseek(int, long, int);
#endif /* BEN_HACK */


/*
 * The Macintosh is a little bit brain-dead sometimes
 */
#ifdef MACINTOSH
# define open(N,F,M) open((char*)(N),F)
# define write(F,B,S) write(F,(char*)(B),S)
#endif /* MACINTOSH */


/*
 * Several systems have no "O_BINARY" flag
 */
#ifndef O_BINARY
# define O_BINARY 0
#endif /* O_BINARY */


/*
 * Hack -- attempt to delete a file
 */
errr fd_kill(cptr file)
{
	char                buf[1024];

	/* Hack -- Try to parse the path */
	if (path_parse(buf, 1024, file)) return (-1);

	/* Remove */
	(void)remove(buf);

	/* XXX XXX XXX */
	return (0);
}


/*
 * Hack -- attempt to move a file
 */
errr fd_move(cptr file, cptr what)
{
	char                buf[1024];
	char                aux[1024];

	/* Hack -- Try to parse the path */
	if (path_parse(buf, 1024, file)) return (-1);

	/* Hack -- Try to parse the path */
	if (path_parse(aux, 1024, what)) return (-1);

	/* Rename */
	(void)rename(buf, aux);

	/* XXX XXX XXX */
	return (0);
}


/*
 * Hack -- attempt to copy a file
 */
errr fd_copy(cptr file, cptr what)
{
	char                buf[1024];
	char                aux[1024];

	/* Hack -- Try to parse the path */
	if (path_parse(buf, 1024, file)) return (-1);

	/* Hack -- Try to parse the path */
	if (path_parse(aux, 1024, what)) return (-1);

	/* Copy XXX XXX XXX */
	/* (void)rename(buf, aux); */

	/* XXX XXX XXX */
	return (1);
}


/*
 * Hack -- attempt to open a file descriptor (create file)
 *
 * This function should fail if the file already exists
 *
 * Note that we assume that the file should be "binary"
 *
 * XXX XXX XXX The horrible "BEN_HACK" code is for compiling under
 * the CodeWarrior compiler, in which case, for some reason, none
 * of the "O_*" flags are defined, and we must fake the definition
 * of "O_RDONLY", "O_WRONLY", and "O_RDWR" in "A-win-h", and then
 * we must simulate the effect of the proper "open()" call below.
 */
int fd_make(cptr file, int mode)
{
	char                buf[1024];

	/* Hack -- Try to parse the path */
	if (path_parse(buf, 1024, file)) return (-1);

#ifdef BEN_HACK

	/* Check for existance */
	/* if (fd_close(fd_open(file, O_RDONLY | O_BINARY))) return (1); */

	/* Mega-Hack -- Create the file */
	(void)my_fclose(my_fopen(file, "wb"));

	/* Re-open the file for writing */
	return (open(buf, O_WRONLY | O_BINARY, mode));

#else /* BEN_HACK */

	/* Create the file, fail if exists, write-only, binary */
	return (open(buf, O_CREAT | O_EXCL | O_WRONLY | O_BINARY, mode));

#endif /* BEN_HACK */

}


/*
 * Hack -- attempt to open a file descriptor (existing file)
 *
 * Note that we assume that the file should be "binary"
 */
int fd_open(cptr file, int flags)
{
	char                buf[1024];

	/* Hack -- Try to parse the path */
	if (path_parse(buf, 1024, file)) return (-1);

	/* Attempt to open the file */
	return (open(buf, flags | O_BINARY, 0));
}


/*
 * Hack -- attempt to lock a file descriptor
 *
 * Legal lock types -- F_UNLCK, F_RDLCK, F_WRLCK
 */
errr fd_lock(int fd, int what)
{
	/* XXX XXX */
	what = what ? what : 0;

	/* Verify the fd */
	if (fd < 0) return (-1);

#ifdef SET_UID

# ifdef USG

	/* Un-Lock */
	if (what == F_UNLCK)
	{
		/* Unlock it, Ignore errors */
		lockf(fd, F_ULOCK, 0);
	}

	/* Lock */
	else
	{
		/* Lock the score file */
		if (lockf(fd, F_LOCK, 0) != 0) return (1);
	}

#else

	/* Un-Lock */
	if (what == F_UNLCK)
	{
		/* Unlock it, Ignore errors */
		(void)flock(fd, LOCK_UN);
	}

	/* Lock */
	else
	{
		/* Lock the score file */
		if (flock(fd, LOCK_EX) != 0) return (1);
	}

# endif

#endif

	/* Success */
	return (0);
}


/*
 * Hack -- attempt to seek on a file descriptor
 */
errr fd_seek(int fd, huge n)
{
	long p;

	/* Verify fd */
	if (fd < 0) return (-1);

	/* Seek to the given position */
	p = lseek(fd, n, SEEK_SET);

	/* Failure */
	if (p < 0) return (1);

	/* Failure */
	if (p != n) return (1);

	/* Success */
	return (0);
}


/*
 * Hack -- attempt to truncate a file descriptor
 */
errr fd_chop(int fd, huge n)
{
	/* XXX XXX */
	n = n ? n : 0;

	/* Verify the fd */
	if (fd < 0) return (-1);

#if defined(sun) || defined(ultrix) || defined(NeXT)
	/* Truncate */
	ftruncate(fd, n);
#endif

	/* Success */
	return (0);
}


/*
 * Hack -- attempt to read data from a file descriptor
 */
errr fd_read(int fd, char *buf, huge n)
{
	/* Verify the fd */
	if (fd < 0) return (-1);

#ifndef SET_UID

	/* Read pieces */
	while (n >= 16384)
	{
		/* Read a piece */
		if (read(fd, buf, 16384) != 16384) return (1);

		/* Shorten the task */
		buf += 16384;

		/* Shorten the task */
		n -= 16384;
	}

#endif

	/* Read the final piece */
	if (read(fd, buf, n) != n) return (1);

	/* Success */
	return (0);
}


/*
 * Hack -- Attempt to write data to a file descriptor
 */
errr fd_write(int fd, cptr buf, huge n)
{
	/* Verify the fd */
	if (fd < 0) return (-1);

#ifndef SET_UID

	/* Write pieces */
	while (n >= 16384)
	{
		/* Write a piece */
		if (write(fd, buf, 16384) != 16384) return (1);

		/* Shorten the task */
		buf += 16384;

		/* Shorten the task */
		n -= 16384;
	}

#endif

	/* Write the final piece */
	if (write(fd, buf, n) != n) return (1);

	/* Success */
	return (0);
}


/*
 * Hack -- attempt to close a file descriptor
 */
errr fd_close(int fd)
{
	/* Verify the fd */
	if (fd < 0) return (-1);

	/* Close */
	(void)close(fd);

	/* XXX XXX XXX */
	return (0);
}


#endif /* ACORN */


/*
 * Move the cursor
 */
#if 0
void move_cursor(int row, int col)
{
	Term_gotoxy(col, row);
}
#endif



/*
 * Convert a decimal to a single digit octal number
 */
static char octify(uint i)
{
	return (hexsym[i%8]);
}

/*
 * Convert a decimal to a single digit hex number
 */
static char hexify(uint i)
{
	return (hexsym[i%16]);
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
 * Hack -- convert a string into a printable form
 */
void ascii_to_text(char *buf, cptr str)
{
	char *s = buf;

	/* Analyze the "ascii" string */
	while (*str)
	{
		byte i = (byte)(*str++);

		if (i == ESCAPE)
		{
			*s++ = '\\';
			*s++ = 'e';
		}
		else if (i == ' ')
		{
			*s++ = '\\';
			*s++ = 's';
		}
		else if (i == '\b')
		{
			*s++ = '\\';
			*s++ = 'b';
		}
		else if (i == '\t')
		{
			*s++ = '\\';
			*s++ = 't';
		}
		else if (i == '\n')
		{
			*s++ = '\\';
			*s++ = 'n';
		}
		else if (i == '\r')
		{
			*s++ = '\\';
			*s++ = 'r';
		}
		else if (i == '^')
		{
			*s++ = '\\';
			*s++ = '^';
		}
		else if (i == '\\')
		{
			*s++ = '\\';
			*s++ = '\\';
		}
		else if (i < 32)
		{
			*s++ = '^';
			*s++ = i + 64;
		}
		else if (i < 127)
		{
			*s++ = i;
		}
		else if (i < 64)
		{
			*s++ = '\\';
			*s++ = '0';
			*s++ = octify(i / 8);
			*s++ = octify(i % 8);
		}
		else
		{
			*s++ = '\\';
			*s++ = 'x';
			*s++ = hexify(i / 16);
			*s++ = hexify(i % 16);
		}
	}

	/* Terminate */
	*s = '\0';
}



/*
 * Variable used by the functions below
 */
static int hack_dir = 0;


/*
 * Convert a "Rogue" keypress into an "Angband" keypress
 * Pass extra information as needed via "hack_dir"
 *
 * Note that many "Rogue" keypresses encode a direction.
 */
static char roguelike_commands(char command)
{
	/* Process the command */
	switch (command)
	{
		/* Movement (rogue keys) */
		case 'b': hack_dir = 1; return (';');
		case 'j': hack_dir = 2; return (';');
		case 'n': hack_dir = 3; return (';');
		case 'h': hack_dir = 4; return (';');
		case 'l': hack_dir = 6; return (';');
		case 'y': hack_dir = 7; return (';');
		case 'k': hack_dir = 8; return (';');
		case 'u': hack_dir = 9; return (';');

		/* Running (shift + rogue keys) */
		case 'B': hack_dir = 1; return ('.');
		case 'J': hack_dir = 2; return ('.');
		case 'N': hack_dir = 3; return ('.');
		case 'H': hack_dir = 4; return ('.');
		case 'L': hack_dir = 6; return ('.');
		case 'Y': hack_dir = 7; return ('.');
		case 'K': hack_dir = 8; return ('.');
		case 'U': hack_dir = 9; return ('.');

		/* Tunnelling (control + rogue keys) */
		case KTRL('B'): hack_dir = 1; return ('+');
		case KTRL('J'): hack_dir = 2; return ('+');
		case KTRL('N'): hack_dir = 3; return ('+');
		case KTRL('H'): hack_dir = 4; return ('+');
		case KTRL('L'): hack_dir = 6; return ('+');
		case KTRL('Y'): hack_dir = 7; return ('+');
		case KTRL('K'): hack_dir = 8; return ('+');
		case KTRL('U'): hack_dir = 9; return ('+');

		/* Hack -- White-space */
		case KTRL('M'): return ('\r');

		/* Allow use of the "destroy" command */
		case KTRL('D'): return ('k');

		/* Hack -- Commit suicide */
		case KTRL('C'): return ('Q');

		/* Locate player on map */
		case 'W': return ('L');

		/* Browse a book (Peruse) */
		case 'P': return ('b');

		/* Jam a door (Spike) */
		case 'S': return ('j');

		/* Toggle search mode */
		case '#': return ('S');

		/* Use a staff (Zap) */
		case 'Z': return ('u');

		/* Take off equipment */
		case 'T': return ('t');

		/* Fire an item */
		case 't': return ('f');

		/* Bash a door (Force) */
		case 'f': return ('B');

		/* Look around (examine) */
		case 'x': return ('l');

		/* Aim a wand (Zap) */
		case 'z': return ('a');

		/* Zap a rod (Activate) */
		case 'a': return ('z');

		/* Run */
		case ',': return ('.');

		/* Stay still (fake direction) */
		case '.': hack_dir = 5; return (',');

		/* Stay still (fake direction) */
		case '5': hack_dir = 5; return (',');

		/* Standard walking */
		case '1': hack_dir = 1; return (';');
		case '2': hack_dir = 2; return (';');
		case '3': hack_dir = 3; return (';');
		case '4': hack_dir = 4; return (';');
		case '6': hack_dir = 6; return (';');
		case '7': hack_dir = 7; return (';');
		case '8': hack_dir = 8; return (';');
		case '9': hack_dir = 9; return (';');
	}

	/* Default */
	return (command);
}


/*
 * Convert an "Original" keypress into an "Angband" keypress
 * Pass direction information back via "hack_dir".
 *
 * Note that "Original" and "Angband" are very similar.
 */
static char original_commands(char command)
{
	/* Process the command */
	switch (command)
	{
		/* Hack -- White space */
		case KTRL('J'): return ('\r');
		case KTRL('M'): return ('\r');

		/* Tunnel */
		case 'T': return ('+');

		/* Run */
		case '.': return ('.');

		/* Stay still (fake direction) */
		case ',': hack_dir = 5; return (',');

		/* Stay still (fake direction) */
		case '5': hack_dir = 5; return (',');

		/* Standard walking */
		case '1': hack_dir = 1; return (';');
		case '2': hack_dir = 2; return (';');
		case '3': hack_dir = 3; return (';');
		case '4': hack_dir = 4; return (';');
		case '6': hack_dir = 6; return (';');
		case '7': hack_dir = 7; return (';');
		case '8': hack_dir = 8; return (';');
		case '9': hack_dir = 9; return (';');

		/* Hack -- Commit suicide */
		case KTRL('K'): return ('Q');
		case KTRL('C'): return ('Q');
	}

	/* Default */
	return (command);
}


/*
 * React to new value of "rogue_like_commands".
 *
 * Initialize the "keymap" arrays based on the current value of
 * "rogue_like_commands".  Note that all "undefined" keypresses
 * by default map to themselves with no direction.  This allows
 * "standard" commands to use the same keys in both keysets.
 *
 * To reset the keymap, simply set "rogue_like_commands" to -1,
 * call this function, restore its value, call this function.
 *
 * The keymap arrays map keys to "command_cmd" and "command_dir".
 *
 * It is illegal for keymap_cmds[N] to be zero, except for
 * keymaps_cmds[0], which is unused.
 *
 * You can map a key to "tab" to make it "non-functional".
 */
void keymap_init(void)
{
	int i, k;

	/* Notice changes in the "rogue_like_commands" flag */
	static bool old_rogue_like = -1;

	/* Hack -- notice changes in "rogue_like_commands" */
	if (old_rogue_like == rogue_like_commands) return;

	/* Initialize every entry */
	for (i = 0; i < 128; i++)
	{
		/* Default to "no direction" */
		hack_dir = 0;

		/* Attempt to translate */
		if (rogue_like_commands)
		{
			k = roguelike_commands(i);
		}
		else
		{
			k = original_commands(i);
		}

		/* Save the keypress */
		keymap_cmds[i] = k;

		/* Save the direction */
		keymap_dirs[i] = hack_dir;
	}

	/* Save the "rogue_like_commands" setting */
	old_rogue_like = rogue_like_commands;
}








/*
 * Legal bit-flags for macro__use[X]
 */
#define MACRO_USE_CMD   0x01    /* X triggers a command macro */
#define MACRO_USE_STD   0x02    /* X triggers a standard macro */

/*
 * Fast check for trigger of any macros
 */
static byte macro__use[256];



/*
 * Hack -- add a macro definition (or redefinition).
 *
 * If "cmd_flag" is set then this macro is only active when
 * the user is being asked for a command (see below).
 */
void macro_add(cptr pat, cptr act, bool cmd_flag)
{
	int n;


	/* Paranoia -- require data */
	if (!pat || !act) return;


	/* Look for a re-usable slot */
	for (n = 0; n < macro__num; n++)
	{
		/* Notice macro redefinition */
		if (streq(macro__pat[n], pat))
		{
			/* Free the old macro action */
			string_free(macro__act[n]);

			/* Save the macro action */
			macro__act[n] = string_make(act);

			/* Save the "cmd_flag" */
			macro__cmd[n] = cmd_flag;

			/* All done */
			return;
		}
	}


	/* Save the pattern */
	macro__pat[macro__num] = string_make(pat);

	/* Save the macro action */
	macro__act[macro__num] = string_make(act);

	/* Save the "cmd_flag" */
	macro__cmd[macro__num] = cmd_flag;

	/* One more macro */
	macro__num++;


	/* Hack -- Note the "trigger" char */
	macro__use[(byte)(pat[0])] |= MACRO_USE_STD;

	/* Hack -- Note the "trigger" char of command macros */
	if (cmd_flag) macro__use[(byte)(pat[0])] |= MACRO_USE_CMD;
}



/*
 * Check for possibly pending macros
 */
#if 0
static int macro_maybe(cptr buf, int n)
{
	int i;

	/* Scan the macros */
	for (i = n; i < macro__num; i++)
	{
		/* Skip inactive macros */
		if (macro__cmd[i] && !inkey_flag) continue;

		/* Check for "prefix" */
		if (prefix(macro__pat[i], buf))
		{
			/* Ignore complete macros */
			if (!streq(macro__pat[i], buf)) return (i);
		}
	}

	/* No matches */
	return (-1);
}
#endif


/*
 * Find the longest completed macro
 */
#if 0
static int macro_ready(cptr buf)
{
	int i, t, n = -1, s = -1;

	/* Scan the macros */
	for (i = 0; i < macro__num; i++)
	{
		/* Skip inactive macros */
		if (macro__cmd[i] && !inkey_flag) continue;

		/* Check for "prefix" */
		if (!prefix(buf, macro__pat[i])) continue;

		/* Check the length of this entry */
		t = strlen(macro__pat[i]);

		/* Find the "longest" entry */
		if ((n >= 0) && (s > t)) continue;

		/* Track the entry */
		n = i;
		s = t;
	}

	/* Return the result */
	return (n);
}
#endif



/*
 * Local "need flush" variable
 */
static bool flush_later = FALSE;


/*
 * Local variable -- we just finished a macro action
 */
/*static bool after_macro = FALSE;*/

/*
 * Local variable -- we are inside a macro action
 */
/*static bool parse_macro = FALSE;*/

/*
 * Local variable -- we are inside a "control-underscore" sequence
 */
/*static bool parse_under = FALSE;*/

/*
 * Local variable -- we are inside a "control-backslash" sequence
 */
/*static bool parse_slash = FALSE;*/

/*
 * Local variable -- we are stripping symbols for a while
 */
/*static bool strip_chars = FALSE;*/



/*
 * Flush all input chars.  Actually, remember the flush,
 * and do a "special flush" before the next "inkey()".
 *
 * This is not only more efficient, but also necessary to make sure
 * that various "inkey()" codes are not "lost" along the way.
 */
void flush(void)
{
	/* Do it later */
	flush_later = TRUE;
}


/*
 * Flush the screen, make a noise
 */
void bell(void)
{
#if 0
	/* Mega-Hack -- Flush the output */
	Term_fresh();

	/* Make a bell noise (if allowed) */
	if (ring_bell) Term_xtra(TERM_XTRA_NOISE, 0);

	/* Flush the input (later!) */
	flush();
#endif
}


/*
 * Mega-Hack -- Make a (relevant?) sound
 */
void sound(int Ind, int val)
{
	/* Make a sound */
	Send_sound(Ind, val);
}




/*
 * Helper function called only from "inkey()"
 *
 * This function does most of the "macro" processing.
 *
 * We use the "Term_key_push()" function to handle "failed" macros,
 * as well as "extra" keys read in while choosing a macro, and the
 * actual action for the macro.
 *
 * Embedded macros are illegal, although "clever" use of special
 * control chars may bypass this restriction.  Be very careful.
 *
 * The user only gets 500 (1+2+...+29+30) milliseconds for the macro.
 *
 * Note the annoying special processing to "correctly" handle the
 * special "control-backslash" codes following a "control-underscore"
 * macro sequence.  See "main-x11.c" and "main-xaw.c" for details.
 */
#if 0
static char inkey_aux(void)
{
	int             k = 0, n, p = 0, w = 0;

	char    ch;

	cptr    pat, act;

	char    buf[1024];


	/* Wait for keypress */
	(void)(Term_inkey(&ch, TRUE, TRUE));


	/* End of internal macro */
	if (ch == 29) parse_macro = FALSE;


	/* Do not check "ascii 28" */
	if (ch == 28) return (ch);

	/* Do not check "ascii 29" */
	if (ch == 29) return (ch);


	/* Do not check macro actions */
	if (parse_macro) return (ch);

	/* Do not check "control-underscore" sequences */
	if (parse_under) return (ch);

	/* Do not check "control-backslash" sequences */
	if (parse_slash) return (ch);


	/* Efficiency -- Ignore impossible macros */
	if (!macro__use[(byte)(ch)]) return (ch);

	/* Efficiency -- Ignore inactive macros */
	if (!inkey_flag && (macro__use[(byte)(ch)] == MACRO_USE_CMD)) return (ch);


	/* Save the first key, advance */
	buf[p++] = ch;
	buf[p] = '\0';


	/* Wait for a macro, or a timeout */
	while (TRUE)
	{
		/* Check for possible macros */
		k = macro_maybe(buf, k);

		/* Nothing matches */
		if (k < 0) break;

		/* Check for (and remove) a pending key */
		if (0 == Term_inkey(&ch, FALSE, TRUE))
		{
			/* Append the key */
			buf[p++] = ch;
			buf[p] = '\0';

			/* Restart wait */
			w = 0;
		}

		/* No key ready */
		else
		{
			/* Increase "wait" */
			w += 10;

			/* Excessive delay */
			if (w >= 100) break;

			/* Delay */
			Term_xtra(TERM_XTRA_DELAY, w);
		}
	}


	/* Check for a successful macro */
	k = macro_ready(buf);

	/* No macro available */
	if (k < 0)
	{
		/* Push all the keys back on the queue */
		while (p > 0)
		{
			/* Push the key, notice over-flow */
			if (Term_key_push(buf[--p])) return (0);
		}

		/* Wait for (and remove) a pending key */
		(void)Term_inkey(&ch, TRUE, TRUE);

		/* Return the key */
		return (ch);
	}


	/* Access the macro pattern */
	pat = macro__pat[k];

	/* Get the length of the pattern */
	n = strlen(pat);

	/* Push the "extra" keys back on the queue */
	while (p > n)
	{
		/* Push the key, notice over-flow */
		if (Term_key_push(buf[--p])) return (0);
	}


	/* We are now inside a macro */
	parse_macro = TRUE;

	/* Push the "macro complete" key */
	if (Term_key_push(29)) return (0);


	/* Access the macro action */
	act = macro__act[k];

	/* Get the length of the action */
	n = strlen(act);

	/* Push the macro "action" onto the key queue */
	while (n > 0)
	{
		/* Push the key, notice over-flow */
		if (Term_key_push(act[--n])) return (0);
	}


	/* Force "inkey()" to call us again */
	return (0);
}
#endif




/*
 * Get a keypress from the user.
 *
 * This function recognizes a few "global parameters".  These are variables
 * which, if set to TRUE before calling this function, will have an effect
 * on this function, and which are always reset to FALSE by this function
 * before this function returns.  Thus they function just like normal
 * parameters, except that most calls to this function can ignore them.
 *
 * Normally, this function will process "macros", but if "inkey_base" is
 * TRUE, then we will bypass all "macro" processing.  This allows direct
 * usage of the "Term_inkey()" function.
 *
 * Normally, this function will do something, but if "inkey_xtra" is TRUE,
 * then something else will happen.
 *
 * Normally, this function will wait until a "real" key is ready, but if
 * "inkey_scan" is TRUE, then we will return zero if no keys are ready.
 *
 * Normally, this function will show the cursor, and will process all normal
 * macros, but if "inkey_flag" is TRUE, then we will only show the cursor if
 * "hilite_player" is TRUE, and also, we will only process "command" macros.
 *
 * Note that the "flush()" function does not actually flush the input queue,
 * but waits until "inkey()" is called to perform the "flush".
 *
 * Refresh the screen if waiting for a keypress and no key is ready.
 *
 * Note that "back-quote" is automatically converted into "escape" for
 * convenience on machines with no "escape" key.  This is done after the
 * macro matching, so the user can still make a macro for "backquote".
 *
 * Note the special handling of a few "special" control-keys, which
 * are reserved to simplify the use of various "main-xxx.c" files,
 * or used by the "macro" code above.
 *
 * Ascii 27 is "control left bracket" -- normal "Escape" key
 * Ascii 28 is "control backslash" -- special macro delimiter
 * Ascii 29 is "control right bracket" -- end of macro action
 * Ascii 30 is "control caret" -- indicates "keypad" key
 * Ascii 31 is "control underscore" -- begin macro-trigger
 *
 * Hack -- Make sure to allow calls to "inkey()" even if "term_screen"
 * is not the active Term, this allows the various "main-xxx.c" files
 * to only handle input when "term_screen" is "active".
 *
 * Note the nasty code used to process the "inkey_base" flag, which allows
 * various "macro triggers" to be entered as normal key-sequences, with the
 * appropriate timing constraints, but without actually matching against any
 * macro sequences.  Most of the nastiness is to handle "ascii 28" (see below).
 *
 * The "ascii 28" code is a complete hack, used to allow "default actions"
 * to be associated with a given keypress, and used only by the X11 module,
 * it may or may not actually work.  The theory is that a keypress can send
 * a special sequence, consisting of a "macro trigger" plus a "default action",
 * with the "default action" surrounded by "ascii 28" symbols.  Then, when that
 * key is pressed, if the trigger matches any macro, the correct action will be
 * executed, and the "strip default action" code will remove the "default action"
 * from the keypress queue, while if it does not match, the trigger itself will
 * be stripped, and then the "ascii 28" symbols will be stripped as well, leaving
 * the "default action" keys in the "key queue".  Again, this may not work.
 */
#if 0
char inkey(int Ind)
{
	int v;

	char kk, ch;

	bool done = FALSE;

	term *old = Term;

	int w = 0;

	int skipping = FALSE;


	/* Hack -- handle delayed "flush()" */
	if (flush_later)
	{
		/* Done */
		flush_later = FALSE;

		/* Cancel "macro" info */
		parse_macro = after_macro = FALSE;

		/* Cancel "sequence" info */
		parse_under = parse_slash = FALSE;

		/* Cancel "strip" mode */
		strip_chars = FALSE;

		/* Forget old keypresses */
		Term_flush();
	}


	/* Access cursor state */
	(void)Term_get_cursor(&v);

	/* Show the cursor if waiting, except sometimes in "command" mode */
	if (!inkey_scan && (!inkey_flag || hilite_player || character_icky))
	{
		/* Show the cursor */
		(void)Term_set_cursor(1);
	}


	/* Hack -- Activate the screen */
	Term_activate(term_screen);


	/* Get a (non-zero) keypress */
	for (ch = 0; !ch; )
	{
		/* Nothing ready, not waiting, and not doing "inkey_base" */
		if (!inkey_base && inkey_scan && (0 != Term_inkey(&ch, FALSE, FALSE))) break;


		/* Hack -- flush output once when no key ready */
		if (!done && (0 != Term_inkey(&ch, FALSE, FALSE)))
		{
			/* Hack -- activate proper term */
			Term_activate(old);

			/* Flush output */
			Term_fresh();

			/* Hack -- activate the screen */
			Term_activate(term_screen);

			/* Mega-Hack -- reset saved flag */
			character_saved = FALSE;

			/* Mega-Hack -- reset signal counter */
			signal_count = 0;

			/* Only once */
			done = TRUE;
		}


		/* Hack */
		if (inkey_base)
		{
			char xh;

			/* Check for keypress, optional wait */
			(void)Term_inkey(&xh, !inkey_scan, TRUE);

			/* Key ready */
			if (xh)
			{
				/* Reset delay */
				w = 0;

				/* Mega-Hack */
				if (xh == 28)
				{
					/* Toggle "skipping" */
					skipping = !skipping;
				}

				/* Use normal keys */
				else if (!skipping)
				{
					/* Use it */
					ch = xh;
				}
			}

			/* No key ready */
			else
			{
				/* Increase "wait" */
				w += 10;

				/* Excessive delay */
				if (w >= 100) break;

				/* Delay */
				Term_xtra(TERM_XTRA_DELAY, w);
			}

			/* Continue */
			continue;
		}


		/* Get a key (see above) */
		kk = ch = inkey_aux();


		/* Finished a "control-underscore" sequence */
		if (parse_under && (ch <= 32))
		{
			/* Found the edge */
			parse_under = FALSE;

			/* Stop stripping */
			strip_chars = FALSE;

			/* Strip this key */
			ch = 0;
		}


		/* Finished a "control-backslash" sequence */
		if (parse_slash && (ch == 28))
		{
			/* Found the edge */
			parse_slash = FALSE;

			/* Stop stripping */
			strip_chars = FALSE;

			/* Strip this key */
			ch = 0;
		}


		/* Handle some special keys */
		switch (ch)
		{
			/* Hack -- convert back-quote into escape */
			case '`':

			/* Convert to "Escape" */
			ch = ESCAPE;

			/* Done */
			break;

			/* Hack -- strip "control-right-bracket" end-of-macro-action */
			case 29:

			/* Strip this key */
			ch = 0;

			/* Done */
			break;

			/* Hack -- strip "control-caret" special-keypad-indicator */
			case 30:

			/* Strip this key */
			ch = 0;

			/* Done */
			break;

			/* Hack -- strip "control-underscore" special-macro-triggers */
			case 31:

			/* Strip this key */
			ch = 0;

			/* Inside a "underscore" sequence */
			parse_under = TRUE;

			/* Strip chars (always) */
			strip_chars = TRUE;

			/* Done */
			break;

			/* Hack -- strip "control-backslash" special-fallback-strings */
			case 28:

			/* Strip this key */
			ch = 0;

			/* Inside a "control-backslash" sequence */
			parse_slash = TRUE;

			/* Strip chars (sometimes) */
			strip_chars = after_macro;

			/* Done */
			break;
		}


		/* Hack -- Set "after_macro" code */
		after_macro = ((kk == 29) ? TRUE : FALSE);


		/* Hack -- strip chars */
		if (strip_chars) ch = 0;
	}


	/* Hack -- restore the term */
	Term_activate(old);


	/* Restore the cursor */
	Term_set_cursor(v);


	/* Cancel the various "global parameters" */
	inkey_base = inkey_xtra = inkey_flag = inkey_scan = FALSE;


	/* Return the keypress */
	return (ch);
}
#endif





/*
 * We use a global array for all inscriptions to reduce the memory
 * spent maintaining inscriptions.  Of course, it is still possible
 * to run out of inscription memory, especially if too many different
 * inscriptions are used, but hopefully this will be rare.
 *
 * We use dynamic string allocation because otherwise it is necessary
 * to pre-guess the amount of quark activity.  We limit the total
 * number of quarks, but this is much easier to "expand" as needed.
 *
 * Any two items with the same inscription will have the same "quark"
 * index, which should greatly reduce the need for inscription space.
 *
 * Note that "quark zero" is NULL and should not be "dereferenced".
 */

/*
 * Add a new "quark" to the set of quarks.
 */
s16b quark_add(cptr str)
{
	int i;

	/* Look for an existing quark */
	for (i = 1; i < quark__num; i++)
	{
		/* Check for equality */
		if (streq(quark__str[i], str)) return (i);
	}

	/* Paranoia -- Require room */
	if (quark__num == QUARK_MAX) return (0);

	/* New maximal quark */
	quark__num = i + 1;

	/* Add a new quark */
	quark__str[i] = string_make(str);

	/* Return the index */
	return (i);
}


/*
 * This function looks up a quark
 */
cptr quark_str(s16b i)
{
	cptr q;

	/* Verify */
	if ((i < 0) || (i >= quark__num)) i = 0;

	/* Access the quark */
	q = quark__str[i];

	/* Return the quark */
	return (q);
}

/*
 * Check to make sure they haven't inscribed an item against what
 * they are trying to do -Crimson
 * look for "!*Erm" type, and "!* !A !f" type.
 */

bool check_guard_inscription( s16b quark, char what ) {
    const char  *   ax;     
    ax=quark_str(quark);
    if( ax == NULL ) { return FALSE; };
    while( (ax=strchr(ax,'!')) != NULL ) {
	while( ax++ != NULL ) {
	    if (*ax==0)  {
		 return FALSE; /* end of quark, stop */
	    }
	    if (*ax==' ') {
		 break; /* end of segment, stop */
	    }
	    if (*ax==what) {
		return TRUE; /* exact match, stop */
	    }
	    if(*ax =='*') {
		switch( what ) { /* check for paranoid tags */
		    case 'd': /* no drop */
		    case 'h': /* no house ( sell a a key ) */
		    case 'k': /* no destroy */
		    case 's': /* no sell */
		    case 'v': /* no thowing */
			case '=': /* force pickup */
		      return TRUE;
		};
	    };  
	};  
    };  
    return FALSE;
};  

/*
 * Litterally.	- Jir -
 */

char *find_inscription(s16b quark, char *what)
{
    const char  *ax = quark_str(quark);
    if( ax == NULL || !what) { return FALSE; };
	
	return (strstr(ax, what));
}  

/*
 * Second try for the "message" handling routines.
 *
 * Each call to "message_add(s)" will add a new "most recent" message
 * to the "message recall list", using the contents of the string "s".
 *
 * The messages will be stored in such a way as to maximize "efficiency",
 * that is, we attempt to maximize the number of sequential messages that
 * can be retrieved, given a limited amount of storage space.
 *
 * We keep a buffer of chars to hold the "text" of the messages, not
 * necessarily in "order", and an array of offsets into that buffer,
 * representing the actual messages.  This is made more complicated
 * by the fact that both the array of indexes, and the buffer itself,
 * are both treated as "circular arrays" for efficiency purposes, but
 * the strings may not be "broken" across the ends of the array.
 *
 * The "message_add()" function is rather "complex", because it must be
 * extremely efficient, both in space and time, for use with the Borg.
 */



/*
 * How many messages are "available"?
 */
s16b message_num(void)
{
	int last, next, n;

	/* Extract the indexes */
	last = message__last;
	next = message__next;

	/* Handle "wrap" */
	if (next < last) next += MESSAGE_MAX;

	/* Extract the space */
	n = (next - last);

	/* Return the result */
	return (n);
}



/*
 * Recall the "text" of a saved message
 */
cptr message_str(s16b age)
{
	s16b x;
	s16b o;
	cptr s;

	/* Forgotten messages have no text */
	if ((age < 0) || (age >= message_num())) return ("");

	/* Acquire the "logical" index */
	x = (message__next + MESSAGE_MAX - (age + 1)) % MESSAGE_MAX;

	/* Get the "offset" for the message */
	o = message__ptr[x];

	/* Access the message text */
	s = &message__buf[o];

	/* Return the message text */
	return (s);
}



/*
 * Add a new message, with great efficiency
 */
void message_add(cptr str)
{
#if 0
	int i, k, x, n;


	/*** Step 1 -- Analyze the message ***/

	/* Hack -- Ignore "non-messages" */
	if (!str) return;

	/* Message length */
	n = strlen(str);

	/* Important Hack -- Ignore "long" messages */
	if (n >= MESSAGE_BUF / 4) return;


	/*** Step 2 -- Attempt to optimize ***/

	/* Limit number of messages to check */
	k = message_num() / 4;

	/* Limit number of messages to check */
	if (k > MESSAGE_MAX / 32) k = MESSAGE_MAX / 32;

	/* Check the last few messages (if any to count) */
	for (i = message__next; k; k--)
	{
		u16b q;

		cptr old;

		/* Back up and wrap if needed */
		if (i-- == 0) i = MESSAGE_MAX - 1;

		/* Stop before oldest message */
		if (i == message__last) break;

		/* Extract "distance" from "head" */
		q = (message__head + MESSAGE_BUF - message__ptr[i]) % MESSAGE_BUF;

		/* Do not optimize over large distance */
		if (q > MESSAGE_BUF / 2) continue;

		/* Access the old string */
		old = &message__buf[message__ptr[i]];

		/* Compare */
		if (!streq(old, str)) continue;

		/* Get the next message index, advance */
		x = message__next++;

		/* Handle wrap */
		if (message__next == MESSAGE_MAX) message__next = 0;

		/* Kill last message if needed */
		if (message__next == message__last) message__last++;

		/* Handle wrap */
		if (message__last == MESSAGE_MAX) message__last = 0;

		/* Assign the starting address */
		message__ptr[x] = message__ptr[i];

		/* Success */
		return;
	}


	/*** Step 3 -- Ensure space before end of buffer ***/

	/* Kill messages and Wrap if needed */
	if (message__head + n + 1 >= MESSAGE_BUF)
	{
		/* Kill all "dead" messages */
		for (i = message__last; TRUE; i++)
		{
			/* Wrap if needed */
			if (i == MESSAGE_MAX) i = 0;

			/* Stop before the new message */
			if (i == message__next) break;

			/* Kill "dead" messages */
			if (message__ptr[i] >= message__head)
			{
				/* Track oldest message */
				message__last = i + 1;
			}
		}

		/* Wrap "tail" if needed */
		if (message__tail >= message__head) message__tail = 0;

		/* Start over */
		message__head = 0;
	}


	/*** Step 4 -- Ensure space before next message ***/

	/* Kill messages if needed */
	if (message__head + n + 1 > message__tail)
	{
		/* Grab new "tail" */
		message__tail = message__head + n + 1;

		/* Advance tail while possible past first "nul" */
		while (message__buf[message__tail-1]) message__tail++;

		/* Kill all "dead" messages */
		for (i = message__last; TRUE; i++)
		{
			/* Wrap if needed */
			if (i == MESSAGE_MAX) i = 0;

			/* Stop before the new message */
			if (i == message__next) break;

			/* Kill "dead" messages */
			if ((message__ptr[i] >= message__head) &&
			    (message__ptr[i] < message__tail))
			{
				/* Track oldest message */
				message__last = i + 1;
			}
		}
	}


	/*** Step 5 -- Grab a new message index ***/

	/* Get the next message index, advance */
	x = message__next++;

	/* Handle wrap */
	if (message__next == MESSAGE_MAX) message__next = 0;

	/* Kill last message if needed */
	if (message__next == message__last) message__last++;

	/* Handle wrap */
	if (message__last == MESSAGE_MAX) message__last = 0;



	/*** Step 6 -- Insert the message text ***/

	/* Assign the starting address */
	message__ptr[x] = message__head;

	/* Append the new part of the message */
	for (i = 0; i < n; i++)
	{
		/* Copy the message */
		message__buf[message__head + i] = str[i];
	}

	/* Terminate */
	message__buf[message__head + i] = '\0';

	/* Advance the "head" pointer */
	message__head += n + 1;
#endif
}



/*
 * Hack -- flush
 */
#if 0
static void msg_flush(int x)
{
	byte a = TERM_L_BLUE;

	/* Hack -- fake monochrome */
	if (!use_color) a = TERM_WHITE;

	/* Pause for response */
	Term_putstr(x, 0, -1, a, "-more-");

	/* Get an acceptable keypress */
	while (1)
	{
		int cmd = inkey();
		if (quick_messages) break;
		if ((cmd == ESCAPE) || (cmd == ' ')) break;
		if ((cmd == '\n') || (cmd == '\r')) break;
		bell();
	}

	/* Clear the line */
	Term_erase(0, 0, 255);
}
#endif


/*
 * Output a message to the top line of the screen.
 *
 * Break long messages into multiple pieces (40-72 chars).
 *
 * Allow multiple short messages to "share" the top line.
 *
 * Prompt the user to make sure he has a chance to read them.
 *
 * These messages are memorized for later reference (see above).
 *
 * We could do "Term_fresh()" to provide "flicker" if needed.
 *
 * The global "msg_flag" variable can be cleared to tell us to
 * "erase" any "pending" messages still on the screen.
 *
 * XXX XXX XXX Note that we must be very careful about using the
 * "msg_print()" functions without explicitly calling the special
 * "msg_print(NULL)" function, since this may result in the loss
 * of information if the screen is cleared, or if anything is
 * displayed on the top line.
 *
 * XXX XXX XXX Note that "msg_print(NULL)" will clear the top line
 * even if no messages are pending.  This is probably a hack.
 */
void msg_print(int Ind, cptr msg)
{
	/* Ahh, the beautiful simplicity of it.... --KLJ-- */
	Send_message(Ind, msg);
}

void msg_broadcast(int Ind, cptr msg)
{
	int i;
	
	/* Tell every player */
	for (i = 1; i <= NumPlayers; i++)
	{
		/* Skip disconnected players */
		if (Players[i]->conn == NOT_CONNECTED) 
			continue;
			
		/* Skip the specified player */
		if (i == Ind)
			continue;       
			
		/* Tell this one */
		msg_print(i, msg);
	 }
}

void msg_broadcast_format(int Ind, cptr fmt, ...)
{
	int i;
	
	va_list vp;

	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);
	
	/* Format the args, save the length */
	(void)vstrnfmt(buf, 1024, fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	msg_broadcast(Ind, buf);
}

/* Send a formatted message only to admin chars.	-Jir- */
void msg_admin(cptr fmt, ...)
//void msg_admin(int Ind, cptr fmt, ...)
{
	int i;
	player_type *p_ptr;
	
	va_list vp;

	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);
	
	/* Format the args, save the length */
	(void)vstrnfmt(buf, 1024, fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Tell every admin */
	for (i = 1; i <= NumPlayers; i++)
	{
		p_ptr = Players[i];

		/* Skip disconnected players */
		if (p_ptr->conn == NOT_CONNECTED) 
			continue;
			

		/* Tell Mama */
		if (p_ptr->admin_dm || p_ptr->admin_wiz)
			msg_print(i, buf);
	 }
}



/*
 * Display a formatted message, using "vstrnfmt()" and "msg_print()".
 */
void msg_format(int Ind, cptr fmt, ...)
{
	va_list vp;

	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);
	
	/* Format the args, save the length */
	(void)vstrnfmt(buf, 1024, fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Display */
	msg_print(Ind, buf);
}


/*
 * Display a message to everyone who is in sight on another player.
 *
 * This is mainly used to keep other players advised of actions done
 * by a player.  The message is not sent to the player who performed
 * the action.
 */
void msg_print_near(int Ind, cptr msg)
{
	player_type *p_ptr = Players[Ind];
	int y, x, i;
	struct worldpos *wpos;
	wpos=&p_ptr->wpos;

	y = p_ptr->py;
	x = p_ptr->px;

	/* Check each player */
	for (i = 1; i <= NumPlayers; i++)
	{
		/* Check this player */
		p_ptr = Players[i];

		/* Make sure this player is in the game */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Don't send the message to the player who caused it */
		if (Ind == i) continue;

		/* Make sure this player is at this depth */
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Can he see this player? */
		if (p_ptr->cave_flag[y][x] & CAVE_VIEW)
		{
			/* Send the message */
			msg_print(i, msg);
		}
	}
}


/*
 * Same as above, except send a formatted message.
 */
void msg_format_near(int Ind, cptr fmt, ...)
{
	va_list vp;

	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Format the args, save the length */
	(void)vstrnfmt(buf, 1024, fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Display */
	msg_print_near(Ind, buf);
}


/*
 * Allow to cast quickly when using '/cast'. (code by DEG)
 */
// #define FRIENDLY_SPELL_BOOST

/*
 * A message prefixed by a player name is sent only to that player.
 * Otherwise, it is sent to everyone.
 */
/*
 * This function is hacked *ALOT* to add extra-commands w/o
 * client change.		- Jir -
 */
void player_talk_aux(int Ind, cptr message)
{
 	int i, len, target = 0;
	char search[80], sender[80];
	player_type *p_ptr = Players[Ind], *q_ptr;
 	cptr colon, problem = "";
	bool me = FALSE;
	char c = 'B';
	int mycolor = 0;
	bool admin = p_ptr->admin_wiz || p_ptr->admin_dm;

	p_ptr->msgcnt++;
	if(p_ptr->msgcnt>12){
		time_t last=p_ptr->msg;
		time(&p_ptr->msg);
		if(p_ptr->msg-last < 6){
			p_ptr->spam++;
			switch(p_ptr->spam){
				case 1:
					msg_print(Ind, "\377yPlease don't spam the server");
					break;
				case 3:
				case 4:
					msg_print(Ind, "\377rWarning! this behaviour is unacceptable!");
					break;
				case 5:
					p_ptr->chp=-3;
					strcpy(p_ptr->died_from, "hypoxia");
					player_death(Ind);
					p_ptr->spam=1;
					return;
			}
		}
		if(p_ptr->msg-last > 240 && p_ptr->spam) p_ptr->spam--;
		p_ptr->msgcnt=0;
	}
	if(p_ptr->spam > 1) return;

	/* Get sender's name */
	if (Ind)
	{
		/* Get player name */
		strcpy(sender, p_ptr->name);
	}
	else
	{
		/* Default name */
		strcpy(sender, "Server Admin");
	}

	/* Special - shutdown command (for compatibility) */
	if (prefix(message, "@!shutdown") && admin)
	{
		shutdown_server();
		return;
	}

	/* Special - '/' commands */
	if (prefix(message, "/"))
	{
		int k = 0, tk = 0;
		char *token[9];
//		cptr token[9];
		worldpos wp;

		wpcopy(&wp, &p_ptr->wpos);

		/* Look for a player's name followed by a colon */
		colon = strchr(message, ' ');

		/* hack -- non-token ones first */
		if ((prefix(message, "/script") ||
					prefix(message, "/scr") ||
					prefix(message, "/ ") ||	// use with care!
					prefix(message, "//") ||	// use with care!
					prefix(message, "/lua")) && admin)
		{
			if (colon)
			{
				master_script_exec(Ind, colon);
			}
			else
			{
				msg_print(Ind, "\377oUsage: /lua (LUA script command)");
			}
			return;
		}
		else if (prefix(message, "/me "))
		{
			me = TRUE;
		}
		else
		{

			/* cut tokens off (thx Ascrep(DEG)) */
			if ((token[0]=strtok(message," ")))
			{
//				s_printf("%d : %s", tk, token[0]);
				for (i=1;i<=9;i++)
				{
					token[i]=strtok(NULL," ");
					if (token[i]==NULL)
						break;
					tk = i;
				}
			}

			/* Default to no search string */
			//		strcpy(search, "");

			/* Form a search string if we found a colon */
			if (tk)
			{
				k = atoi(token[1]);
#if 0
				/* Copy everything up to the colon to the search string */
				strncpy(search, message, colon - message);

				/* Add a trailing NULL */
				search[colon - message] = '\0';

				/* Move colon pointer forward to next word */
				while (*colon && (isspace(*colon))) colon++;
#endif
			}

			/* User commands */
			if (prefix(message, "/ignore") ||
					prefix(message, "/ig"))
			{
				add_ignore(Ind, token[1]);
				return;
			}
			else if (prefix(message, "/afk"))
			{
				toggle_afk(Ind);
				return;
			}

			/* Semi-auto item destroyer */
			else if ((prefix(message, "/dispose")) ||
					prefix(message, "/dis"))
			{
				object_type		*o_ptr;
                u32b f1, f2, f3, f4, f5, esp;
				for(i = 0; i < INVEN_PACK; i++)
				{
					o_ptr = &(p_ptr->inventory[i]);
					if (!o_ptr->tval) break;

					object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

					/* skip inscribed items */
					/* skip non-matching tags */
					if (o_ptr->note && 
						strcmp(quark_str(o_ptr->note), "cursed") &&
						strcmp(quark_str(o_ptr->note), "uncursed") &&
						strcmp(quark_str(o_ptr->note), "broken") &&
						strcmp(quark_str(o_ptr->note), "average") &&
						strcmp(quark_str(o_ptr->note), "good") &&
//						strcmp(quark_str(o_ptr->note), "excellent"))
						strcmp(quark_str(o_ptr->note), "worthless"))
							continue;

					if ((f4 & TR4_CURSE_NO_DROP) && cursed_p(o_ptr))
						continue;

					do_cmd_destroy(Ind, i, o_ptr->number);
					i--;

					/* Hack - Don't take a turn here */
					p_ptr->energy += level_speed(&p_ptr->wpos);
//					p_ptr->energy += level_speed(p_ptr->dun_depth);
				}
				/* Take total of one turn */
				p_ptr->energy -= level_speed(&p_ptr->wpos);
//				p_ptr->energy -= level_speed(p_ptr->dun_depth);
				return;
			}

			/* add inscription to everything */
			else if (prefix(message, "/tag"))
			{
				object_type		*o_ptr;
				for(i = 0; i < INVEN_PACK; i++)
				{
					o_ptr = &(p_ptr->inventory[i]);
					if (!o_ptr->tval) break;

					/* skip inscribed items */
					if (o_ptr->note) continue;

					o_ptr->note = quark_add(token[1] ? token[1] : "!k");
				}
				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP);

				return;
			}
			/* remove specific inscription */
			else if (prefix(message, "/untag"))
			{
				object_type		*o_ptr;
//				cptr	*ax = token[1] ? token[1] : "!k";
				cptr	ax = token[1] ? token[1] : "!k";

				for(i = 0; i < INVEN_PACK; i++)
				{
					o_ptr = &(p_ptr->inventory[i]);
					if (!o_ptr->tval) break;

					/* skip inscribed items */
					if (!o_ptr->note) continue;

					/* skip non-matching tags */
					if (strcmp(quark_str(o_ptr->note), ax)) continue;

					o_ptr->note = 0;
				}

				/* Combine the pack */
				p_ptr->notice |= (PN_COMBINE);

				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP);

				return;
			}
			/* '/cast' code is written by Ascrep(DEG). thx! */
			else if (prefix(message, "/cast"))
			{
				int book, whichplayer, whichspell;
				bool ami = FALSE;
#if 0
				token[0]=strtok(message," ");
				if (token[0]==NULL)
				{
					msg_print(Ind, "\377oUsage: /cast (Book) (Spell) [Playername]");
					return;
				}

				for (i=1;i<50;i++)
				{
					token[i]=strtok(NULL," ");
					if (token[i]==NULL)
						break;
				}
#endif

				/* at least 2 arguments required */
				if (tk < 2)
				{
					msg_print(Ind, "\377oUsage: /cast (Book) (Spell) [Player name]");
					return;
				}

				if(*token[1]>='1' && *token[1]<='9')
				{	
					object_type *o_ptr;
					char c[4] = "@";
					bool found = FALSE;

					c[1] = ((p_ptr->pclass == CLASS_PRIEST) ||
							(p_ptr->pclass == CLASS_PALADIN)? 'p':'m');
					if (p_ptr->pclass == CLASS_WARRIOR) c[1] = 'n';
					c[2] = *token[1];
					c[3] = '\0';

					for(i = 0; i < INVEN_PACK; i++)
					{
						o_ptr = &(p_ptr->inventory[i]);
						if (!o_ptr->tval) break;

						if (find_inscription(o_ptr->note, c))
						{
							book = i;
							found = TRUE;
							break;
						}
					}

					if (!found)
					{
						msg_format(Ind, "\377oInscription {%s} not found.", c);
						return;
					}
					//					book = atoi(token[1])-1;
				}	
				else
				{	
					*token[1] &= ~(0x20);
					if(*token[1]>='A' && *token[1]<='W')
					{	
						book = (int)(*token[1]-'A');
					}		
					else 
					{
						msg_print(Ind,"\377oBook variable was out of range (a-i) or (1-9)");
						return;
					}	
				}

				if(*token[2]>='1' && *token[2]<='9')
				{	
					//					whichspell = atoi(token[2]+'A'-1);
					whichspell = atoi(token[2]) - 1;
				}	
				else if(*token[2]>='a' && *token[2]<='i')
				{	
					whichspell = (int)(*token[2]-'a');
				}		
				/* if Capital letter, it's for friends */
				else if(*token[2]>='A' && *token[2]<='I')
				{
					whichspell = (int)(*token[2]-'A');
					//					*token[2] &= 0xdf;
					//					whichspell = *token[2]-1;
					ami = TRUE;
				}
				else 
				{
					msg_print(Ind,"\377oSpell out of range [A-I].");
					return;
				}	

				if (token[3])
				{
					if (!(whichplayer = name_lookup_loose(Ind, token[3], TRUE)))
						return;

					if (whichplayer == Ind)
					{
						msg_print(Ind,"You feel lonely.");
					}
					/* Ignore "unreasonable" players */
					else if (!target_able(Ind, 0 - whichplayer))
					{
						msg_print(Ind,"\377oThat player is out of your sight.");
						return;
					}
					else
					{
						//			msg_format(Ind,"Book = %ld, Spell = %ld, PlayerName = %s, PlayerID = %ld",book,whichspell,token[3],whichplayer); 
						target_set_friendly(Ind,5,whichplayer);
						whichspell += 64;
					}
				}
				else if (ami)
				{
					target_set_friendly(Ind, 5);
					whichspell += 64;
				}
				else
				{
					target_set(Ind, 5);
				}

				switch (p_ptr->pclass)
				{
					case CLASS_SORCERER:
						{
							do_cmd_sorc(Ind, book, whichspell);
#ifdef FRIENDLY_SPELL_BOOST
							p_ptr->energy += level_speed(p_ptr->dun_depth);
#endif
							break;
						}

					case CLASS_TELEPATH:
						{
							do_cmd_psi(Ind, book, whichspell);
#ifdef FRIENDLY_SPELL_BOOST
							p_ptr->energy += level_speed(p_ptr->dun_depth);
#endif
							break;
						}
					case CLASS_ROGUE:
						{
							do_cmd_shad(Ind, book, whichspell);
#ifdef FRIENDLY_SPELL_BOOST
							p_ptr->energy += level_speed(p_ptr->dun_depth);
#endif
							break;
						}
					case CLASS_ARCHER:
						{
							do_cmd_hunt(Ind, book, whichspell);
#ifdef FRIENDLY_SPELL_BOOST
							p_ptr->energy += level_speed(p_ptr->dun_depth);
#endif
							break;
						}
					case CLASS_MAGE:
					case CLASS_RANGER:
						{
							do_cmd_cast(Ind, book, whichspell);
#ifdef FRIENDLY_SPELL_BOOST
							p_ptr->energy += level_speed(p_ptr->dun_depth);
#endif
							break;
						}
					case CLASS_PRIEST:
					case CLASS_PALADIN:
						{
							do_cmd_pray(Ind, book, whichspell);
#ifdef FRIENDLY_SPELL_BOOST
							p_ptr->energy += level_speed(p_ptr->dun_depth);
#endif
							break;
						}
				}

				//			msg_format(Ind,"Book = %ld, Spell = %ld, PlayerName = %s, PlayerID = %ld",book,whichspell,token[3],whichplayer); 
				return;
			}
			/* Take everything off */
			else if ((prefix(message, "/bed")) ||
					prefix(message, "/naked"))
			{
				object_type		*o_ptr;
				for (i=INVEN_WIELD;i<INVEN_TOTAL;i++)
				{
					o_ptr = &(p_ptr->inventory[i]);
					if (!o_ptr->tval) continue;

					/* skip inscribed items */
					/* skip non-matching tags */
					if ((check_guard_inscription(o_ptr->note, 't')) ||
						(check_guard_inscription(o_ptr->note, 'T')) ||
						(cursed_p(o_ptr))) continue;

					inven_takeoff(Ind, i, 255);
					p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;
				}
				return;
			}

			/* Try to wield everything */
			else if ((prefix(message, "/dress")) ||
					prefix(message, "/dr"))
			{
				object_type		*o_ptr;
				int j;

				for (i=INVEN_WIELD;i<INVEN_TOTAL;i++)
				{
					o_ptr = &(p_ptr->inventory[i]);
					if (o_ptr->tval) continue;

					for(j = 0; j < INVEN_PACK; j++)
					{
						o_ptr = &(p_ptr->inventory[j]);
						if (!o_ptr->tval) break;

						/* skip unsuitable inscriptions */
						if (o_ptr->note &&
								(!strcmp(quark_str(o_ptr->note), "cursed") ||
								 check_guard_inscription(o_ptr->note, 'w')) )continue;

						if (wield_slot(Ind, o_ptr) != i) continue;

						do_cmd_wield(Ind, j);
						break;
					}

				}
				return;
			}
			/* '/cast' code is written by Ascrep(DEG). thx! */
			else if (prefix(message, "/quaff"))
			{
				int book, whichplayer, whichspell;
				bool ami = FALSE;

				/* at least 1 argument required */
				if (tk < 1)
				{
					msg_print(Ind, "\377oUsage: /quaff (item index or no.)");
					return;
				}

				if(*token[1]>='1' && *token[1]<='9')
				{	
					object_type *o_ptr;
					char c[4] = "@q";
					bool found = FALSE;

					c[2] = *token[1];
					c[3] = '\0';

					for(i = 0; i < INVEN_PACK; i++)
					{
						o_ptr = &(p_ptr->inventory[i]);
						if (!o_ptr->tval) break;

						if (find_inscription(o_ptr->note, c))
						{
							book = i;
							found = TRUE;
							break;
						}
					}

					if (!found)
					{
						msg_format(Ind, "\377oInscription {%s} not found.", c);
						return;
					}
					//					book = atoi(token[1])-1;
				}	
				else
				{	
					*token[1] &= ~(0x20);
					if(*token[1]>='A' && *token[1]<='W')
					{	
						book = (int)(*token[1]-'A');
					}		
					else 
					{
						msg_print(Ind,"\377oPotion variable was out of range (a-w) or (1-9)");
						return;
					}	
				}

				do_cmd_quaff_potion(Ind, book);
				return;
			}
			/* Display extra information */
			else if (prefix(message, "/extra") ||
					prefix(message, "/examine") ||
					prefix(message, "/ex"))
			{
				/* Insanity warning (better message needed!) */
				if (p_ptr->csane < p_ptr->msane / 8)
				{
					/* Message */
					msg_print(Ind, "\377rYou can hardly resist the temptation to cry out!");
				}
				else if (p_ptr->csane < p_ptr->msane / 4)
				{
					/* Message */
					msg_print(Ind, "\377yYou feel insanity about to grasp your mind..");
				}
				else if (p_ptr->csane < p_ptr->msane / 2)
				{
					/* Message */
					msg_print(Ind, "\377yYou feel insanity creep into your mind..");
				}
				else
				{
					/* Message */
					msg_print(Ind, "\377wYou are sane.");
				}

				return;
			}
			/* Please add here anything you think is needed.  */
			else if ((prefix(message, "/refresh")) ||
					prefix(message, "/ref"))
			{
				/* Clear the target */
				p_ptr->target_who = 0;

				/* Recalculate bonuses */
				p_ptr->update |= (PU_BONUS);

				/* Recalculate torch */
				p_ptr->update |= (PU_TORCH);

				/* Recalculate mana */
				p_ptr->update |= (PU_MANA | PU_HP | PU_SANITY);

				/* Redraw */
				p_ptr->redraw |= (PR_HP | PR_GOLD | PR_BASIC | PR_PLUSSES);

				/* Notice */
				p_ptr->notice |= (PN_COMBINE | PN_REORDER);

				/* Window stuff */
				p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER | PW_SPELL);

				return;
			}
			else if ((prefix(message, "/target")) ||
					prefix(message, "/tar"))
			{
				int tx, ty;

				/* Clear the target */
				p_ptr->target_who = 0;

				/* at least 2 arguments required */
				if (tk < 2)
				{
					msg_print(Ind, "\377oUsage: /target (X) (Y) <from your position>");
					return;
				}

				tx = p_ptr->px + k;
				ty = p_ptr->py + atoi(token[2]);
				
				if (!in_bounds(ty,tx))
				{
					msg_print(Ind, "\377oIllegal position!");
					return;
				}

				/* Set the target */
				p_ptr->target_row = ty;
				p_ptr->target_col = tx;

				/* Set 'stationary' target */
				p_ptr->target_who = 0 - MAX_PLAYERS - 2;
				
				return;
			}

			/*
			 * Admin commands
			 *
			 * These commands should be replaced by LUA scripts in the future.
			 */
			else if (admin)
			{
				/* presume worldpos */
				switch (tk)
				{
					case 1:
					{
						/* depth in feet */
						wp.wz = k / 50;
						break;
					}

					case 3:
					{
						/* depth in feet */
						wp.wz = atoi(token[3]) / 50;
					}

					case 2:
					{
						wp.wx = k % MAX_WILD_X;
						wp.wy = atoi(token[2]) % MAX_WILD_Y;
						wp.wz = 0;
						break;
					}

					default:
					{
						break;
					}
				}

				if (prefix(message, "/shutdown") ||
						prefix(message, "/quit"))
				{
					shutdown_server();
				}
				else if (prefix(message, "/kick"))
				{
					if (tk)
					{
						int j = name_lookup_loose(Ind, token[1], FALSE);
						if (j)
						{
							/* Success maybe :) */
							msg_format(Ind, "Kicking %s out...", Players[j]->name);

							/* Kick him */
							Destroy_connection(Players[j]->conn, "kicked out");
						}
						return;
					}

					msg_print(Ind, "\377oUsage: /kick [Player name]");
					return;
				}
				/* erase items and monsters */
				else if (prefix(message, "/clear-level") ||
						prefix(message, "/clv"))
				{
#if 0
					/* depth in feet */
					k = k / 50;
					if (!tk) k = p_ptr->dun_depth;

					if (-MAX_WILD >= k || k >= MAX_DEPTH)
					{
						msg_print(Ind, "\377oIlligal depth.  Usage: /clv [depth in feet]");
						return;
					}
#endif

					/* Wipe even if town/wilderness */
					wipe_o_list_safely(&wp);
					wipe_m_list(&wp);
					wipe_t_list(&wp);

					msg_format(Ind, "\377rItems/monsters/traps on %s are cleared.", wpos_format(&wp));
					return;
				}
				else if (prefix(message, "/geno-level") ||
						prefix(message, "/geno"))
				{
#if 0
					/* depth in feet */
					k = k / 50;
					if (!tk) k = p_ptr->dun_depth;

					if (-MAX_WILD >= k || k >= MAX_DEPTH)
					{
						msg_print(Ind, "\377oIlligal depth.  Usage: /geno [depth in feet]");
						return;
					}
#endif	// 0

					/* Wipe even if town/wilderness */
					wipe_m_list(&wp);

					msg_format(Ind, "\377rMonsters on %s are cleared.", wpos_format(&wp));
					return;
				}
				else if (prefix(message, "/unstatic-level") ||
						prefix(message, "/unst"))
				{
#if 0
					/* depth in feet */
					k = k / 50;
					if (!tk) k = p_ptr->dun_depth;

					if (-MAX_WILD >= k || k >= MAX_DEPTH)
					{
						msg_print(Ind, "\377oIlligal depth.  Usage: /unst [depth in feet]");
						return;
					}
#endif
					/* no sanity check, so be warned! */
					master_level_specific(Ind, &wp, "u");
					//				msg_format(Ind, "\377rItems and monsters on %dft is cleared.", k * 50);
					return;
				}
				else if (prefix(message, "/static-level") ||
						prefix(message, "/sta"))
				{
#if 0
					/* depth in feet */
					k = k / 50;
					if (!tk) k = p_ptr->dun_depth;

					if (-MAX_WILD >= k || k >= MAX_DEPTH)
					{
						msg_print(Ind, "\377oIlligal depth.  Usage: /sta [depth in feet]");
						return;
					}
#endif	// 0

					/* no sanity check, so be warned! */
					master_level_specific(Ind, &wp, "s");
					//				msg_format(Ind, "\377rItems and monsters on %dft is cleared.", k * 50);
					return;
				}
				else if (prefix(message, "/identify") ||
						prefix(message, "/id"))
				{
					identify_pack(Ind);

					/* Combine the pack */
					p_ptr->notice |= (PN_COMBINE);

					/* Window stuff */
					p_ptr->window |= (PW_INVEN | PW_EQUIP);

					return;
				}
				else if (prefix(message, "/artifact") ||
						prefix(message, "/art"))
				{
					if (k)
					{
						if (a_info[k].cur_num)
						{
							a_info[k].cur_num = 0;
							a_info[k].known = FALSE;
							msg_format(Ind, "Artifact %d is now \377Gfindable\377w.", k);
						}
						else
						{
							a_info[k].cur_num = 1;
							a_info[k].known = TRUE;
							msg_format(Ind, "Artifact %d is now \377runfindable\377w.", k);
						}
					}
					else if (tk > 0 && strchr(token[1],'*'))
					{
						for (i = 0; i < MAX_A_IDX ; i++)
							a_info[i].cur_num = 0;
							a_info[k].known = FALSE;
							msg_format(Ind, "All the artifacts are \377rfindable\377w!", k);
					}
					else
					{
						msg_print(Ind, "Usage: /artifact No.");
					}
					return;
				}
				else if (prefix(message, "/reload-config") ||
						prefix(message, "/cfg"))
				{
					if (tk)
					{
						MANGBAND_CFG = string_make(token[1]);
					}

					//				msg_print(Ind, "Reloading server option(mangband.cfg).");
					msg_format(Ind, "Reloading server option(%s).", MANGBAND_CFG);

					/* Reload the server preferences */
					load_server_cfg();
					return;
				}
				else if (prefix(message, "/recall") ||
						prefix(message, "/rec"))
				{
					switch (tk)
					{
						case 1:
						{
							/* depth in feet */
							p_ptr->recall_pos.wz = k / 50;
							break;
						}

						case 2:
						{
							p_ptr->recall_pos.wx = k % MAX_WILD_X;
							p_ptr->recall_pos.wy = atoi(token[2]) % MAX_WILD_Y;
							break;
						}

						default:
						{
							p_ptr->recall_pos.wz = 0;
						}
					}
#if 0
					/* depth in feet */
					k = tk ? k / 50 : 0;

					if (-MAX_WILD >= k || k >= MAX_DEPTH)
					{
						msg_print(Ind, "\377oIlligal depth.  Usage: /recall [depth in feet]");
					}
					else
					{
						p_ptr->recall_depth = k;
						p_ptr->word_recall = 1;

						msg_print(Ind, "\377oOmnipresent you are...");
					}
					/* do it on your own risk.. pfft */
					p_ptr->recall_pos.wz = k;
#endif
					p_ptr->word_recall = 1;
//					msg_print(Ind, "\377oOmnipresent you are...");
					msg_format(Ind, "\377oTrying to recall to %s...",wpos_format(&p_ptr->recall_pos));
					return;
				}
				else if (prefix(message, "/wish"))
				{
					object_type	forge;
					object_type	*o_ptr = &forge;

//					if (tk < 2 || !k || !(l = atoi(token[2])))
					if (tk < 1 || !k)
					{
						msg_print(Ind, "\377oUsage: /wish (tval) (sval) [name1] or /wish (o_idx)");
						return;
					}

					/* Move colon pointer forward to next word */
//					while (*arg2 && (isspace(*arg2))) arg2++;

					invcopy(o_ptr, tk > 1 ? lookup_kind(k, atoi(token[2])) : k);

					/* Wish arts out! */
//					if (token[3])
					if (tk > 2)
					{
						int nom = atoi(token[3]);
						o_ptr->number = 1;

						if (nom > 0) o_ptr->name1 = nom;
						else
						{
							/* It's ego or randarts */
							if (nom) o_ptr->name2 = 0 - nom;
							else o_ptr->name1 = ART_RANDART;

							/* Piece together a 32-bit random seed */
							o_ptr->name3 = rand_int(0xFFFF) << 16;
							o_ptr->name3 += rand_int(0xFFFF);
						}
					}
					else
					{
						o_ptr->number = o_ptr->weight > 100 ? 2 : 99;
					}

					apply_magic(&p_ptr->wpos, o_ptr, -1, TRUE, TRUE, TRUE);
					o_ptr->discount = 99;
					object_known(o_ptr);
					o_ptr->owner = p_ptr->id;
					o_ptr->level = 1;
					(void)inven_carry(Ind, o_ptr);

					return;
				}
				else if (prefix(message, "/trap") ||
						prefix(message, "/tr"))
				{
					if (k)
					{
						wiz_place_trap(Ind, k);
					}
					else 
					{
						wiz_place_trap(Ind, TRAP_OF_FILLING);
					}
					return;
				}
				else
				{
					msg_print(Ind, "Commands: afk bed cast dis dress ex ignore me ref tag target untag;");
					msg_print(Ind, "  art cfg clv geno id kick lua recall shutdown sta trap unst wish");
					return;
				}
			}
			else
			{
				msg_print(Ind, "Commands: afk bed cast dis dress ex ignore me ref tag target untag;");
				msg_print(Ind, "  /quaff is also available for old client users :)");
				msg_print(Ind, "  /dis \377rdestroys \377wevery uninscribed items in your inventory!");
				return;
			}
		}
	}

	/* Default to no search string */
	strcpy(search, "");


	/* Look for a player's name followed by a colon */
	colon = strchr(message, ':');

	/* Ignore "smileys" or URL */
	if (colon && strchr(")(-/:", *(colon + 1)))
	{
		/* Pretend colon wasn't there */
		colon = NULL;
	}

	/* Form a search string if we found a colon */
	if (colon)
	{
		/* Copy everything up to the colon to the search string */
		strncpy(search, message, colon - message);

		/* Add a trailing NULL */
		search[colon - message] = '\0';
	}

	/* Acquire length of search string */
	len = strlen(search);

	/* Look for a recipient who matches the search string */
	if (len)
	{
		target = name_lookup_loose(Ind, search, TRUE);
#if 0
		/* First check parties */
		for (i = 1; i < MAX_PARTIES; i++)
		{
			/* Skip if empty */
			if (!parties[i].num) continue;

			/* Check name */
			if (!strncasecmp(parties[i].name, search, len))
			{
				/* Set target if not set already */
				if (!target)
				{
					target = 0 - i;
				}
				else
				{
					/* Matching too many parties */
					problem = "parties";
				}

				/* Check for exact match */
				if (len == strlen(parties[i].name))
				{
					/* Never a problem */
					problem = "";

					/* Finished looking */
					break;
				}
			}
		}

		/* Then check players */
		for (i = 1; i <= NumPlayers; i++)
		{
			/* Check this one */
			q_ptr = Players[i];

			/* Skip if disconnected */
			if (q_ptr->conn == NOT_CONNECTED) continue;

			/* Check name */
			if (!strncasecmp(q_ptr->name, search, len))
			{
				/* Set target if not set already */
				if (!target)
				{
					target = i;
				}
				else
				{
					/* Matching too many people */
					problem = "players";
				}

				/* Check for exact match */
				if (len == strlen(q_ptr->name))
				{
					/* Never a problem */
					problem = "";

					/* Finished looking */
					break;
				}
			}
		}
#endif /* 0 */
		/* Move colon pointer forward to next word */
		while (*colon && (isspace(*colon) || *colon == ':')) colon++;

		/* lookup failed */
		if (!target)
		{
			/* Bounce message to player who sent it */
			msg_format(Ind, "[%s] %s", p_ptr->name, colon);

			/* Give up */
			return;
		}
	}
#if 0
	/* Check for recipient set but no match found */
	if (len && !target)
	{
		/* Bounce message to player who sent it */
		msg_format(Ind, "[%s] %s", p_ptr->name, colon);

		/* Send an error message */
		msg_format(Ind, "Could not match name '%s'.", search);

		/* Give up */
		return;
	}

	/* Check for multiple recipients found */
	if (strlen(problem))
	{
		/* Bounce message to player who sent it */
		msg_format(Ind, "[%s] %s", p_ptr->name, colon);

		/* Send an error message */
		msg_format(Ind, "'%s' matches too many %s.", search, problem);

		/* Give up */
		return;
	}
#endif /* 0 */

	/* Send to appropriate player */
	if ((len && target > 0) && (!check_ignore(target, Ind)))
	{
		/* Set target player */
		q_ptr = Players[target];

		/* Send message to target */
		msg_format(target, "\377g[%s:%s] %s", q_ptr->name, sender, colon);

		/* Also send back to sender */
		msg_format(Ind, "\377g[%s:%s] %s", q_ptr->name, sender, colon);

		if (q_ptr->afk) msg_format(Ind, "\377o%s seems to be Away From Keyboard.", q_ptr->name);

		/* Done */
		return;
	}

	/* Send to appropriate party */
	if (len && target < 0)
	{
		/* Send message to target party */
		party_msg_format_ignoring(Ind, 0 - target, "\377G[%s:%s] %s",
				 parties[0 - target].name, sender, colon);

		/* Also send back to sender if not in that party */
		if(!player_in_party(0-target, Ind)){
			msg_format(Ind, "\377G[%s:%s] %s",
			   parties[0 - target].name, sender, colon);
		}

		/* Done */
		return;
	}

	/* Look for leading ':', but ignore "smileys" */
	/* -APD- I don't like this new fangled talking system, reverting to the old way */
	/*if (!strncasecmp(message, "All:", 4) ||
	      (message[0] == ':' && !strchr(")(-", message[1])))
	  { */
//              me = prefix(message, "/me ");
		if (strlen(message) > 1) mycolor = (prefix(message, "}") && (color_char_to_attr(*(message + 1)) != -1))?2:0;

		if (!Ind) c = 'y';
		else if (mycolor) c = *(message + 1);
		else
		{
			if (p_ptr->mode == MODE_HELL) c = 'D';
			if (p_ptr->total_winner) c = 'v';
			else if (p_ptr->ghost) c = 'r';
		}

		/* Send to everyone */
		for (i = 1; i <= NumPlayers; i++)
		{
			if (check_ignore(i, Ind)) continue;

			/* Send message */
			if (!me)
			{
				msg_format(i, "\377%c[%s] \377B%s", c, sender, message + mycolor);
				/* msg_format(i, "\377%c[%s] %s", Ind ? 'B' : 'y', sender, message); */
			} 
			else msg_format(i, "%s %s", sender, message + 4);
		}
	/*}
	 else
	{
		 Send to everyone at this depth 
		for (i = 1; i <= NumPlayers; i++)
		{
			if (check_ignore(i, Ind)) continue;

			q_ptr = Players[i];

			 Check depth 
			if (p_ptr->dun_depth == q_ptr->dun_depth)
			{
				 Send message 
				msg_format(i, "\377U[%s] %s", sender, message);
			}
		}
	}
	*/
}

/*
 * toggle AFK mode on/off.	- Jir -
 */
void toggle_afk(int Ind)
{
	player_type *p_ptr = Players[Ind];

	if (p_ptr->afk)
	{
		msg_print(Ind, "AFK mode is turned \377GOFF\377w.");
		msg_broadcast(Ind, format("\377o%s has returned from AFK.", p_ptr->name));
		p_ptr->afk = FALSE;
	}
	else
	{
		msg_print(Ind, "AFK mode is turned \377rON\377w.");
		msg_broadcast(Ind, format("\377o%s seems to be AFK now.", p_ptr->name));
		p_ptr->afk = TRUE;
	}
	return;
}

/*
 * A player has sent a message to the rest of the world.
 *
 * Parse it and send to everyone or to only the person(s) he requested.
 *
 * Note that more than one message may get sent at once, seperated by
 * tabs ('\t').  Thus, this function splits them and calls
 * "player_talk_aux" to do the dirty work.
 */
void player_talk(int Ind, char *message)
{
	char *cur, *next;

	/* Start at the beginning */
	cur = message;

	/* Process until out of messages */
	while (cur)
	{
		/* Find the next tab */
		next = strchr(cur, '\t');

		/* Stop out the tab */
		if (next)
		{
			/* Replace with \0 */
			*next = '\0';
		}

		/* Process this message */
		player_talk_aux(Ind, cur);

		/* Move to the next one */
		if (next)
		{
			/* One step past the \0 */
			cur = next + 1;
		}
		else
		{
			/* No more message */
			cur = NULL;
		}
	}
}
	

/*
 * Check a char for "vowel-hood"
 */
bool is_a_vowel(int ch)
{
	switch (ch)
	{
		case 'a':
		case 'e':
		case 'i':
		case 'o':
		case 'u':
		case 'A':
		case 'E':
		case 'I':
		case 'O':
		case 'U':
		return (TRUE);
	}

	return (FALSE);
}

/*
 * Look up a player/party name, allowing abbreviations.  - Jir -
 * (looking up party name this way can be rather annoying though)
 *
 * returns 0 if not found/error(, minus value if party.)
 *
 * if 'party' is TRUE, party name is also looked up.
 */
int name_lookup_loose(int Ind, cptr name, bool party)
{
	int i, len, target = 0;
	player_type *q_ptr;
	cptr problem = "";

	/* Acquire length of search string */
	len = strlen(name);

	/* Look for a recipient who matches the search string */
	if (len)
	{
		if (party)
		{
			/* First check parties */
			for (i = 1; i < MAX_PARTIES; i++)
			{
				/* Skip if empty */
				if (!parties[i].num) continue;

				/* Check name */
				if (!strncasecmp(parties[i].name, name, len))
				{
					/* Set target if not set already */
					if (!target)
					{
						target = 0 - i;
					}
					else
					{
						/* Matching too many parties */
						problem = "parties";
					}

					/* Check for exact match */
					if (len == strlen(parties[i].name))
					{
						/* Never a problem */
						target = i;
						problem = "";

						/* Finished looking */
						break;
					}
				}
			}
		}

		/* Then check players */
		for (i = 1; i <= NumPlayers; i++)
		{
			/* Check this one */
			q_ptr = Players[i];

			/* Skip if disconnected */
			if (q_ptr->conn == NOT_CONNECTED) continue;

			/* Check name */
			if (!strncasecmp(q_ptr->name, name, len))
			{
				/* Set target if not set already */
				if (!target)
				{
					target = i;
				}
				else
				{
					/* Matching too many people */
					problem = "players";
				}

				/* Check for exact match */
				if (len == strlen(q_ptr->name))
				{
					/* Never a problem */
					target = i;
					problem = "";

					/* Finished looking */
					break;
				}
			}
		}
	}

	/* Check for recipient set but no match found */
	if ((len && !target) || (target < 0 && !party))
	{
		/* Send an error message */
		msg_format(Ind, "Could not match name '%s'.", name);

		/* Give up */
		return 0;
	}

	/* Check for multiple recipients found */
	if (strlen(problem))
	{
		/* Send an error message */
		msg_format(Ind, "'%s' matches too many %s.", name, problem);

		/* Give up */
		return 0;
	}

	/* Success */
	return target;
}

/*
 * Convert bracer '{' into '\377'
 */
void bracer_ff(char *buf)
{
	int i, len = strlen(buf);

	for(i=0;i<len;i++){
		if(buf[i]=='{') buf[i]='\377';
	}
}

/*
 * make strings from worldpos '-1550ft of (17,15)'	- Jir -
 */
char *wpos_format(worldpos *wpos)
{
	return (format("%dft of (%d,%d)", wpos->wz * 50, wpos->wx, wpos->wy));
}


byte count_bits(u32b array)
{
	byte k = 0, i;        

	if(array)
		for(i = 0; i < 32; i++)
			if(array & (1 << i)) k++;

	return k;
}

