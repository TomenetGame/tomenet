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
 * Flush the screen, make a noise
 */
void bell(void)
{
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
}  

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

void do_slash_cmd(int Ind, cptr message){
	int i;
	int k = 0, tk = 0;
	player_type *p_ptr = Players[Ind], *q_ptr;
 	cptr colon;
	char *token[9];
	worldpos wp;
	bool admin = p_ptr->admin_wiz || p_ptr->admin_dm;

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
	else if ((prefix(message, "/rfe")) ||
			prefix(message, "/bug"))
	{
		if (colon)
		{
			rfe_printf("[%s]%s\n", p_ptr->name, colon);
			msg_print(Ind, "\377GThank you for sending us a message!");
		}
		else
		{
			msg_print(Ind, "\377oUsage: /rfe (message)");
		}
		return;
	}

	else
	{
		/* cut tokens off (thx Ascrep(DEG)) */
		if ((token[0]=strtok(message," ")))
		{
//			s_printf("%d : %s", tk, token[0]);
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
//					strcmp(quark_str(o_ptr->note), "excellent"))
					strcmp(quark_str(o_ptr->note), "worthless"))
					continue;

				if ((f4 & TR4_CURSE_NO_DROP) && cursed_p(o_ptr))
					continue;

				do_cmd_destroy(Ind, i, o_ptr->number);
				i--;

				/* Hack - Don't take a turn here */
				p_ptr->energy += level_speed(&p_ptr->wpos);
//				p_ptr->energy += level_speed(p_ptr->dun_depth);
			}
			/* Take total of one turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);
//			p_ptr->energy -= level_speed(p_ptr->dun_depth);
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
//			cptr	*ax = token[1] ? token[1] : "!k";
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
//					msg_format(Ind,"Book = %ld, Spell = %ld, PlayerName = %s, PlayerID = %ld",book,whichspell,token[3],whichplayer); 
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
			int book;

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
				msg_print(Ind, "\377rYou can hardly resist the temptation to cry out!");
			else if (p_ptr->csane < p_ptr->msane / 4)
				msg_print(Ind, "\377yYou feel insanity about to grasp your mind..");
			else if (p_ptr->csane < p_ptr->msane / 2)
				msg_print(Ind, "\377yYou feel insanity creep into your mind..");
			else
				msg_print(Ind, "\377wYou are sane.");

			if (admin)
			{
				msg_format(Ind, "your sanity: %d/%d", p_ptr->csane, p_ptr->msane);
				msg_format(Ind, "server status - m_max(%d) o_max(%d) t_max(%d)",
						m_max, o_max, t_max);
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
		/* Now this command is opened for everyone */
		else if (prefix(message, "/recall") ||
				prefix(message, "/rec"))
		{
				if (admin)
				{
					p_ptr->word_recall = 1;
//					msg_print(Ind, "\377oOmnipresent you are...");
//					msg_format(Ind, "\377oTrying to recall to %s...",wpos_format(&p_ptr->recall_pos));
				}
				else
				{
					int item;
					object_type *o_ptr;
					char c[3] = "@R";
					bool found = FALSE;

					for(i = 0; i < INVEN_PACK; i++)
					{
						o_ptr = &(p_ptr->inventory[i]);
						if (!o_ptr->tval) break;

						if (find_inscription(o_ptr->note, c))
						{
							item = i;
							found = TRUE;
							break;
						}
					}

					if (!found)
					{
						msg_print(Ind, "\377oInscription {@R} not found.");
						return;
					}

					/* ALERT! Hard-coded! */
					switch (o_ptr->tval)
					{
						case TV_SCROLL:
							do_cmd_read_scroll(Ind, item);
							break;
						case TV_MAGIC_BOOK:		// 6e
							do_cmd_cast(Ind, item, 4);
							break;
						case TV_SORCERY_BOOK:	// 5f
							do_cmd_sorc(Ind, item, 5);
							break;
						case TV_PRAYER_BOOK:	// 5e
							do_cmd_pray(Ind, item, 4);
							break;
						case TV_SHADOW_BOOK:	// 5c,5d
							if (spell_okay(Ind, 35, 1) && p_ptr->csp >= 40) do_cmd_shad(Ind, item, 3);
							else do_cmd_shad(Ind, item, 2);
							break;
						case TV_PSI_BOOK:	// 2d
							do_cmd_psi(Ind, item, 3);
							break;
						case TV_ROD:
							do_cmd_zap_rod(Ind, item);
							break;
						default:
							do_cmd_activate(Ind, item);
//							msg_print(Ind, "\377oYou cannot recall with that.");
							break;
					}

				}

				switch (tk)
				{
					case 1:
						/* depth in feet */
						p_ptr->recall_pos.wz = k / 50;
						break;

					case 2:
						p_ptr->recall_pos.wx = k % MAX_WILD_X;
						p_ptr->recall_pos.wy = atoi(token[2]) % MAX_WILD_Y;
						break;

					default:
						p_ptr->recall_pos.wz = 0;
				}
#if 0
				/* depth in feet */
				k = tk ? k / 50 : 0;

				if (-MAX_WILD >= k || k >= MAX_DEPTH)
				{
						msg_print(Ind, "\377oIllegal depth.  Usage: /recall [depth in feet]");
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
				set_runlevel((cfg.runlevel<6 ? 6 : 5));
				msg_format(Ind, "Runlevel set to %d", cfg.runlevel);
				time(&cfg.closetime);
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
					msg_print(Ind, "\377oIllegal depth.  Usage: /clv [depth in feet]");
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
					msg_print(Ind, "\377oIllegal depth.  Usage: /geno [depth in feet]");
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
					msg_print(Ind, "\377oIllegal depth.  Usage: /unst [depth in feet]");
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
					msg_print(Ind, "\377oIllegal depth.  Usage: /sta [depth in feet]");
					return;
				}
#endif // 0

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
			else if (prefix(message, "/wish"))
			{
				object_type	forge;
				object_type	*o_ptr = &forge;

//				if (tk < 2 || !k || !(l = atoi(token[2])))
				if (tk < 1 || !k)
				{
					msg_print(Ind, "\377oUsage: /wish (tval) (sval) [name1] or /wish (o_idx)");
					return;
				}

				/* Move colon pointer forward to next word */
//				while (*arg2 && (isspace(*arg2))) arg2++;

				invcopy(o_ptr, tk > 1 ? lookup_kind(k, atoi(token[2])) : k);

				/* Wish arts out! */
//				if (token[3])
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
			else if (prefix(message, "/enlight") ||
					prefix(message, "/en"))
			{
				wiz_lite(Ind);
				(void)detect_treasure(Ind);
				(void)detect_object(Ind);
				(void)detect_sdoor(Ind);
				(void)detect_trap(Ind);
				if (k)
				{
					(void)detect_trap(Ind);
					identify_pack(Ind);
					self_knowledge(Ind);

					/* Combine the pack */
					p_ptr->notice |= (PN_COMBINE);

					/* Window stuff */
					p_ptr->window |= (PW_INVEN | PW_EQUIP);
				}

				return;
			}
			else if (prefix(message, "/equip") ||
					prefix(message, "/eq"))
			{
				admin_outfit(Ind);
				return;
			}
			else if (prefix(message, "/uncurse") ||
					prefix(message, "/unc"))
			{
				remove_all_curse(Ind);
				return;
			}
#if 0	// pfft, it was not suitable for slash-commands
			/* view RFE file. this should be able to handle log file also. */
			else if (prefix(message, "/less")) 
			{
				do_cmd_view_rfe(Ind);
				return;
			}
#endif	// 0
			else
			{
				msg_print(Ind, "Commands: afk bed cast dis dress ex ignore me rec ref rfe tag target untag;");
				msg_print(Ind, "  art cfg clv en eq geno id kick lua shutdown sta trap unc unst wish");
				return;
			}
		}
		else
		{
			msg_print(Ind, "Commands: afk bed cast dis dress ex ignore me rec ref rfe tag target untag;");
//			msg_print(Ind, "  /quaff is also available for old client users :)");
			msg_print(Ind, "  /dis \377rdestroys \377wall the uninscribed items in your inventory!");
			return;
		}
	}
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

	if(message[0]=='/'){
		if(!strncmp(message, "/me", 3)) me=TRUE;
		else{
			do_slash_cmd(Ind, message);	/* add check */
			return;
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
						target = 0 - i;
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

