/* $Id$ */
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

bool in_banlist(char *addr){
	struct ip_ban *ptr;
	for(ptr=banlist; ptr!=(struct ip_ban*)NULL; ptr=ptr->next){
		if(!strcmp(addr, ptr->ip)) return(TRUE);
	}
	return(FALSE);
}

void check_banlist(){
	struct ip_ban *ptr, *new, *old=(struct ip_ban*)NULL;
	ptr=banlist;
	while(ptr!=(struct ip_ban*)NULL){
		if(ptr->time){
			if(!(--ptr->time)){
				s_printf("Unbanning connections from %s\n", ptr->ip);
				if(!old){
					banlist=ptr->next;
					new=banlist;
				}
				else{
					old->next=ptr->next;
					new=old->next;
				}
				free(ptr);
				ptr=new;
				continue;
			}
		}
		ptr=ptr->next;
	}
}


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


#if 0
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
#endif

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
	    if (*ax==' ' || *ax=='@') {
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
bool suppress_message = FALSE;

void msg_print(int Ind, cptr msg)
{
	/* Pfft, sorry to bother you.... --JIR-- */
	if (suppress_message) return;

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

	if(p_ptr->admin_dm) return;

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

/* location-based */
void msg_print_near_site(int y, int x, worldpos *wpos, cptr msg)
{
	int i;
	player_type *p_ptr;

	/* Check each player */
	for (i = 1; i <= NumPlayers; i++)
	{
		/* Check this player */
		p_ptr = Players[i];

		/* Make sure this player is in the game */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Make sure this player is at this depth */
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Can (s)he see the site? */
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
void msg_format_near_site(int y, int x, worldpos *wpos, cptr fmt, ...)
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
	msg_print_near_site(y, x, wpos, buf);
}

/*
 * Send a message about a monster to everyone who can see it.
 * Monster name is appended at the beginning. (XXX kludgie!)	- Jir -
 *
 * Example: msg_print_near_monster(m_idx, "wakes up.");
 */
/*
 * TODO: allow format
 * TODO: distinguish 'witnessing' and 'hearing'
 */
void msg_print_near_monster(int m_idx, cptr msg)
{
	int i;
	player_type *p_ptr;
	cave_type **zcave;
	char m_name[80];

	monster_type	*m_ptr = &m_list[m_idx];
	worldpos *wpos=&m_ptr->wpos;

	if(!(zcave=getcave(wpos))) return;


	/* Check each player */
	for (i = 1; i <= NumPlayers; i++)
	{
		/* Check this player */
		p_ptr = Players[i];

		/* Make sure this player is in the game */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Make sure this player is at this depth */
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Skip if not visible */
		if (!p_ptr->mon_vis[m_idx]) continue;

		/* Can he see this player? */
//		if (!p_ptr->cave_flag[y][x] & CAVE_VIEW) continue;

		/* Acquire the monster name */
		monster_desc(i, m_name, m_idx, 0);

		msg_format(i, "%^s %s", m_name, msg);
	}
}




/* Takes a trap name and returns an index, or 0 if no such trap
 * was found. (NOT working!)
 */
#if 0
static int trap_index(char * name)
{
	int i;

	/* for each monster race */
	for (i = 1; i <= MAX_T_IDX; i++)
	{
		if (!strcmp(&t_name[t_info[i].name],name)) return i;
	}
	return 0;
}
#endif	// 0

/*
 * Dodge Chance Feedback.
 */
void use_ability_blade(int Ind)
{
	player_type *p_ptr = Players[Ind], *q_ptr;
	int dun_level = getlevel(&p_ptr->wpos);
	int chance = p_ptr->dodge_chance - (dun_level * 5 / 6);

	if (chance < 0) chance = 0;
	if (chance > 90) chance = 90;	// see DODGE_MAX_CHANCE in melee1.c
	if (is_admin(p_ptr))
	{
		msg_format(Ind, "You have exactly %d%% chances of dodging a level %d monster.", chance, dun_level);
	}

	if (chance < 5)
	{
		msg_format(Ind, "You have almost no chance of dodging a level %d monster.", dun_level);
	}
	else if (chance < 10)
	{
		msg_format(Ind, "You have a slight chance of dodging a level %d monster.", dun_level);
	}
	else if (chance < 20)
	{
		msg_format(Ind, "You have a significant chance of dodging a level %d monster.", dun_level);
	}
	else if (chance < 40)
	{
		msg_format(Ind, "You have a large chance of dodging a level %d monster.", dun_level);
	}
	else if (chance < 70)
	{
		msg_format(Ind, "You have a high chance of dodging a level %d monster.", dun_level);
	}
	else
	{
		msg_format(Ind, "You will usually dodge succesfully a level %d monster.", dun_level);
	}

	return;
}


static void do_cmd_refresh(int Ind)
{
	player_type *p_ptr = Players[Ind];
	object_type *o_ptr;
	int k;

	/* Hack -- fix the inventory count */
	p_ptr->inven_cnt = 0;
	for (k = 0; k < INVEN_PACK; k++)
	{
		o_ptr = &p_ptr->inventory[k];

		/* Skip empty items */
		if (!o_ptr->k_idx) continue;

		p_ptr->inven_cnt++;
	}

	/* Clear the target */
	p_ptr->target_who = 0;

	/* Update his view, light, bonuses, and torch radius */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_BONUS | PU_TORCH |
			PU_DISTANCE | PU_SPELLS | PU_SKILL_MOD);

	/* Recalculate mana */
	p_ptr->update |= (PU_MANA | PU_HP | PU_SANITY);

	/* Redraw */
	p_ptr->redraw |= PR_MAP | PR_EXTRA | PR_HISTORY | PR_VARIOUS;
	p_ptr->redraw |= (PR_HP | PR_GOLD | PR_BASIC | PR_PLUSSES);

	/* Notice */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	return;
}

static void do_slash_brief_help(int Ind)
{
	player_type *p_ptr = Players[Ind], *q_ptr;

#if 0	// let's not show obsolete ones
	msg_print(Ind, "Commands: afk at bed bug cast dis dress ex feel help house ignore less me");	// pet ?
	msg_print(Ind, "  monster news object pk quest rec ref rfe shout tag target untag ver;");
#endif	// 0
	msg_print(Ind, "Commands: afk at bed broadcast bug cast dis dress ex feel fill help ignore me");	// pet ?
	msg_print(Ind, "          pk quest rec ref rfe shout sip tag target untag;");

	if (is_admin(p_ptr))
	{
		msg_print(Ind, "  art cfg clv cp en eq geno id kick lua purge shutdown sta store");
		msg_print(Ind, "  trap unc unst wish");
	}
	else
	{
		msg_print(Ind, "  /dis \377rdestroys \377wall the uninscribed items in your inventory!");
	}
	msg_print(Ind, "  please type '/help' for detailed help.");
}


/*
 * Slash commands - huge hack function for command expansion.
 *
 * TODO: introduce sl_info.txt, like:
 * N:/lua:1
 * F:ADMIN
 * D:/lua (LUA script command)
 */

/* Allow to cast quickly when using '/cast'. (code by DEG) */
// #define FRIENDLY_SPELL_BOOST

static void do_slash_cmd(int Ind, char *message)
{
	int i;
	int k = 0, tk = 0;
	player_type *p_ptr = Players[Ind], *q_ptr;
 	/* cptr colon; */
 	char *colon;
	char *token[9];
	worldpos wp;
	bool admin = is_admin(p_ptr);

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
			/* evileye - a good lua programmer
			   will set refresh flags themself
			   its like downloading a huge file
			   each time i use lua ;( */
			/* do_cmd_refresh(Ind); */
		}
		else
		{
			msg_print(Ind, "\377oUsage: /lua (LUA script command)");
		}
		return;
	}
	else if ((prefix(message, "/rfe")) ||
			prefix(message, "/bug") ||
			prefix(message, "/cookie"))
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
	else if (prefix(message, "/shout") ||
			prefix(message, "/sh"))
	{
		aggravate_monsters(Ind, 1);
		if (colon)
		{
			msg_format_near(Ind, "\377B%^s shouts:%s", p_ptr->name, colon);
		}
		else
		{
			msg_format_near(Ind, "\377BYou hear %s shout!", p_ptr->name);
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
			bool nontag = FALSE;

			disturb(Ind, 1, 0);

			/* only tagged ones? */
			if (tk > 0 && prefix(token[1], "a")) nontag = TRUE;

			for(i = 0; i < INVEN_PACK; i++)
			{
				bool resist = FALSE;
				o_ptr = &(p_ptr->inventory[i]);
				if (!o_ptr->tval) break;

				object_flags(o_ptr, &f1, &f2, &f3, &f4, &f5, &esp);

				/* skip inscribed items */
				/* skip non-matching tags */
				if (o_ptr->note && 
					strcmp(quark_str(o_ptr->note), "terrible") &&
					strcmp(quark_str(o_ptr->note), "cursed") &&
					strcmp(quark_str(o_ptr->note), "uncursed") &&
					strcmp(quark_str(o_ptr->note), "broken") &&
					strcmp(quark_str(o_ptr->note), "average") &&
					strcmp(quark_str(o_ptr->note), "good") &&
//					strcmp(quark_str(o_ptr->note), "excellent"))
					strcmp(quark_str(o_ptr->note), "worthless"))
					continue;

				if (!nontag && !o_ptr->note && !k &&
					!(cursed_p(o_ptr) &&	/* Handle {cursed} */
						(object_known_p(Ind, o_ptr) ||
						 (o_ptr->ident & ID_SENSE))))
					continue;

				/* Player might wish to identify it first */
				if (k_info[o_ptr->k_idx].has_flavor &&
						!p_ptr->obj_aware[o_ptr->k_idx])
					continue;

				/* Hrm, this cannot be destroyed */
				if (((f4 & TR4_CURSE_NO_DROP) && cursed_p(o_ptr)) ||
						artifact_p(o_ptr))
					resist = TRUE;

				/* Hack -- filter by value */
				if (k && (!object_known_p(Ind, o_ptr) ||
							object_value_real(Ind, o_ptr) > k))
					continue;

				do_cmd_destroy(Ind, i, o_ptr->number);
				if (!resist) i--;

				/* Hack - Don't take a turn here */
				p_ptr->energy += level_speed(&p_ptr->wpos);
			}
			/* Take total of one turn */
			p_ptr->energy -= level_speed(&p_ptr->wpos);
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
			msg_print(Ind, "\377oSorry, /cast is not available for the time being.");
#if 0 // TODO: make that work without dependance on CLASS_
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
			}

//			msg_format(Ind,"Book = %ld, Spell = %ld, PlayerName = %s, PlayerID = %ld",book,whichspell,token[3],whichplayer); 
#endif
                        return;
		}
		/* Take everything off */
		else if ((prefix(message, "/bed")) ||
				prefix(message, "/naked"))
		{
			byte start = INVEN_WIELD, end = INVEN_TOTAL;
			object_type		*o_ptr;

			if (!tk)
			{
				start = INVEN_BODY;
				end = INVEN_FEET + 1;
			}

			disturb(Ind, 1, 0);

//			for (i=INVEN_WIELD;i<INVEN_TOTAL;i++)
			for (i = start; i < end; i++)
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
			bool gauche = FALSE;

			disturb(Ind, 1, 0);

			for (i=INVEN_WIELD;i<INVEN_TOTAL;i++)
			{
				if (!item_tester_hook_wear(Ind, i)) continue;

				o_ptr = &(p_ptr->inventory[i]);
				if (o_ptr->tval) continue;

				for(j = 0; j < INVEN_PACK; j++)
				{
					o_ptr = &(p_ptr->inventory[j]);
					if (!o_ptr->k_idx) break;

					/* skip unsuitable inscriptions */
					if (o_ptr->note &&
							(!strcmp(quark_str(o_ptr->note), "cursed") ||
							 !strcmp(quark_str(o_ptr->note), "terrible") ||
							 !strcmp(quark_str(o_ptr->note), "worthless") ||
							 check_guard_inscription(o_ptr->note, 'w')) )continue;

					if (!object_known_p(Ind, o_ptr)) continue;
					if (cursed_p(o_ptr)) continue;

					/* Already used? */
					if (!tk && wield_slot(Ind, o_ptr) != i) continue;

					/* Limit to items with specified strings, if any */
					if (tk && (!o_ptr->note ||
								!strstr(quark_str(o_ptr->note), token[1])))
						continue;

					do_cmd_wield(Ind, j);

					/* MEGAHACK -- tweak to handle rings right */
					if (o_ptr->tval == TV_RING && !gauche)
					{
						i -= 2;
						gauche = TRUE;
					}

					break;
				}
			}
			return;
		}
		/* Display extra information */
		else if (prefix(message, "/extra") ||
				prefix(message, "/examine") ||
				prefix(message, "/ex"))
		{
			if (admin) msg_format(Ind, "The game turn: %d", turn);
			do_cmd_time(Ind);

//			do_cmd_knowledge_dungeons(Ind);
			if (p_ptr->depth_in_feet) msg_format(Ind, "The deepest point you've reached: \377G-%d\377wft", p_ptr->max_dlv * 50);
			else msg_format(Ind, "The deepest point you've reached: Lev \377G-%d", p_ptr->max_dlv);

			msg_format(Ind, "You can move %d.%d times each turn.",
					extract_energy[p_ptr->pspeed] / 10,
					extract_energy[p_ptr->pspeed]
					- (extract_energy[p_ptr->pspeed] / 10) * 10);

			if (get_skill(p_ptr, SKILL_DODGE)) use_ability_blade(Ind);

			/* Insanity warning (better message needed!) */
			if (p_ptr->csane < p_ptr->msane / 8)
				msg_print(Ind, "\377rYou can hardly resist the temptation to cry out!");
			else if (p_ptr->csane < p_ptr->msane / 4)
				msg_print(Ind, "\377yYou feel insanity about to grasp your mind..");
			else if (p_ptr->csane < p_ptr->msane / 2)
				msg_print(Ind, "\377yYou feel insanity creep into your mind..");
			else
				msg_print(Ind, "\377wYou are sane.");

			if (p_ptr->body_monster)
			{
				monster_race *r_ptr = &r_info[p_ptr->body_monster];
				msg_format(Ind, "You %shave head.", r_ptr->body_parts[BODY_HEAD] ? "" : "don't ");
				msg_format(Ind, "You %shave arms.", r_ptr->body_parts[BODY_ARMS] ? "" : "don't ");
				msg_format(Ind, "You can%s use weapons.", r_ptr->body_parts[BODY_WEAPON] ? "" : "not");
				msg_format(Ind, "You can%s wear %s.", r_ptr->body_parts[BODY_FINGER] ? "" : "not", r_ptr->body_parts[BODY_FINGER] == 1 ? "a ring" : "rings");
				msg_format(Ind, "You %shave torso.", r_ptr->body_parts[BODY_TORSO] ? "" : "don't ");
				msg_format(Ind, "You %shave legs suitable for shoes.", r_ptr->body_parts[BODY_LEGS] ? "" : "don't ");
			}

			if (admin)
			{
				cave_type **zcave;
				cave_type *c_ptr;

				msg_format(Ind, "your sanity: %d/%d", p_ptr->csane, p_ptr->msane);
				msg_format(Ind, "server status: m_max(%d) o_max(%d)",
						m_max, o_max);

				msg_print(Ind, "Colour test - \377ddark \377wwhite \377sslate \377oorange \377rred \377ggreen \377bblue \377uumber");
				msg_print(Ind, "\377Dl_dark \377Wl_white \377vviolet \377yyellow \377Rl_red \377Gl_green \377Bl_blue \377Ul_umber");
				if(!(zcave=getcave(&wp)))
				{
					msg_print(Ind, "\377rOops, the cave's not allocated!!");
					return;
				}
				c_ptr=&zcave[p_ptr->py][p_ptr->px];

				msg_format(Ind, "(x:%d y:%d) info:%d feat:%d o_idx:%d m_idx:%d effect:%d",
						p_ptr->px, p_ptr->py,
						c_ptr->info, c_ptr->feat, c_ptr->o_idx, c_ptr->m_idx, c_ptr->effect);
			}

			return;
		}
		/* Please add here anything you think is needed.  */
		else if ((prefix(message, "/refresh")) ||
				prefix(message, "/ref"))
		{
			do_cmd_refresh(Ind);
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
				set_recall_timer(Ind, 1);
			}
			else
			{
				int item;
				object_type *o_ptr;
				bool found = FALSE;

				for(i = 0; i < INVEN_PACK; i++)
				{
					o_ptr = &(p_ptr->inventory[i]);
					if (!o_ptr->tval) break;

					if (find_inscription(o_ptr->note, "@R"))
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

				disturb(Ind, 1, 0);

				/* ALERT! Hard-coded! */
				switch (o_ptr->tval)
				{
					case TV_SCROLL:
						do_cmd_read_scroll(Ind, item);
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
					p_ptr->recall_pos.wz = k / (p_ptr->depth_in_feet ? 50 : 1);
					break;

				case 2:
					p_ptr->recall_pos.wx = k % MAX_WILD_X;
					p_ptr->recall_pos.wy = atoi(token[2]) % MAX_WILD_Y;
					p_ptr->recall_pos.wz = 0;
					break;

//				default:	/* follow the inscription */
					/* TODO: support tower */
//					p_ptr->recall_pos.wz = 0 - p_ptr->max_dlv;
//					p_ptr->recall_pos.wz = 0;
			}

			return;
		}
#if 1	/* TODO: remove &7 viewer commands */
		/* view RFE file or any other files in lib/data. */
		else if (prefix(message, "/less")) 
		{
			char    path[MAX_PATH_LENGTH];
			if (tk && is_admin(p_ptr))
			{
				//					if (strstr(token[1], "log") && is_admin(p_ptr))
				{
					//						path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_TEXT, "mangband.log");
					path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, token[1]);
					do_cmd_check_other_prepare(Ind, path);
					return;
				}
				//					else if (strstr(token[1], "rfe") &&

			}
			/* default is "mangband.rfe" */
			else if ((is_admin(p_ptr) || cfg.public_rfe))
			{
				path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_DATA, "tomenet.rfe");
				do_cmd_check_other_prepare(Ind, path);
				return;
			}
			else msg_print(Ind, "\377o/less is not opened for use...");
			return;
		}
		else if (prefix(message, "/news")) 
		{
			char    path[MAX_PATH_LENGTH];
			path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_TEXT, "news.txt");
			do_cmd_check_other_prepare(Ind, path);
			return;
		}
#endif	// 0
		else if (prefix(message, "/version") ||
				prefix(message, "/ver"))
		{
			if (tk) do_cmd_check_server_settings(Ind);
			else msg_print(Ind, longVersion);

			return;
		}
		else if (prefix(message, "/help") ||
				prefix(message, "/he") ||
				prefix(message, "/?"))
		{
			char    path[MAX_PATH_LENGTH];

			/* Build the filename */
			if (admin && !tk) path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_TEXT, "slash_ad.hlp");
			else path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_TEXT, "slash.hlp");

			do_cmd_check_other_prepare(Ind, path);
			return;
		}
		else if(prefix(message, "/pkill") ||
				prefix(message, "/pk"))
		{
			set_pkill(Ind, admin? 10 : 200);
			return;
		}
		else if(prefix(message, "/quest") ||
				prefix(message, "/que"))	/* /quIt */
		{
			int i, j=Ind, k=0;
			s16b r;
			int lev;
			u16b flags=(QUEST_MONSTER|QUEST_RANDOM);
	
			if(tk && !strcmp(token[1], "reset")){
				int qn;
				if(!admin) return;
				for(qn=0;qn<20;qn++){
					quests[qn].active=0;
					quests[qn].type=0;
					quests[qn].id=0;
				}
				msg_broadcast(0, "\377yQuests are reset");
				for(i=1; i<=NumPlayers; i++){
					if(Players[i]->conn==NOT_CONNECTED) continue;
					Players[i]->quest_id=0;
				}
				return;
			}
			if(tk && !strcmp(token[1], "guild")){
				if(!p_ptr->guild || guilds[p_ptr->guild].master != p_ptr->id){
					msg_print(Ind, "\377rYou are not a guildmaster");
					return;
				}
				if(tk==2){
					if((j=name_lookup_loose(Ind, token[2], FALSE))){
						if(Players[j]->quest_id){
							msg_format(Ind, "\377y%s has a quest already.", Players[j]->name);
							return;
						}
					}
					else{
						msg_format(Ind, "Player %s is not here", token[2]);
						return;
					}
				}
				else{
					msg_print(Ind, "Usage: /quest guild name");
					return;
				}
				flags|=QUEST_GUILD;
				lev=Players[j]->lev+5;
			}
			else if(admin && tk){
				if((j=name_lookup_loose(Ind, token[1], FALSE))){
					if(Players[j]->quest_id){
						msg_format(Ind, "\377y%s has a quest already.", Players[j]->name);
						return;
					}
					lev=Players[j]->lev;	/* for now */
				}
				else return;
			}
			else{
				flags|=QUEST_RACE;
				lev=Players[j]->lev;
			}
			if(Players[j]->quest_id){
				for(i=0; i<20; i++){
					if(quests[i].id==Players[j]->quest_id){
						if(j==Ind)
							msg_format(Ind, "\377oYour quest to kill \377y%d \377g%s \377ois not complete.%s", p_ptr->quest_num, r_name+r_info[quests[i].type].name, quests[i].flags&QUEST_GUILD?" (guild)":"");
						return;
					}
				}
			}
			get_mon_num_hook=dungeon_aux;
			get_mon_num_prep();
			i=2+randint(7);
			do{
				r=get_mon_num(lev);
				k++;
				if(k>100) lev--;
			} while(((lev-5) > r_info[r].level) || r_info[r].flags1 & RF1_UNIQUE);
			add_quest(Ind, j, r, i, flags);
			return;
		}
		else if (prefix(message, "/feeling") ||
				prefix(message, "/fe"))
		{
			if (!show_floor_feeling(Ind))
				msg_print(Ind, "You feel nothing special.");
			return;
		}
		else if (prefix(message, "/monster") ||
				prefix(message, "/mo"))
		{
			int r_idx, num;
			monster_race *r_ptr;
			if (!tk)
			{
				do_cmd_show_monster_killed_letter(Ind, NULL);
				return;
			}

			/* Handle specification like 'D', 'k' */
			if (strlen(token[1]) == 1)
			{
				do_cmd_show_monster_killed_letter(Ind, token[1]);
				return;
			}

			/* Directly specify a name (tho no1 would use it..) */
			r_idx = race_index(token[1]);
			if (!r_idx)
			{
				msg_print(Ind, "No such monster.");
				return;
			}

			r_ptr = &r_info[r_idx];
			num = p_ptr->r_killed[r_idx];

			if (get_skill(p_ptr, SKILL_MIMIC))
			{
				i = r_ptr->level - num;

				if (i > 0)
					msg_format(Ind, "%s : %d slain (%d more to go)",
							r_name + r_ptr->name, num, i);
				else
					msg_format(Ind, "%s : %d slain (learnt)", r_name + r_ptr->name, num);
			}
			else
			{
				msg_format(Ind, "%s : %d slain.", r_name + r_ptr->name, num);
			}

			/* TODO: show monster description */
			
			return;
		}
		/* add inscription to books */
		else if (prefix(message, "/autotag") ||
				prefix(message, "/at"))
		{
			object_type		*o_ptr;
			char c[] = "@m ";
			for(i = 0; i < INVEN_PACK; i++)
			{
				o_ptr = &(p_ptr->inventory[i]);
				auto_inscribe(Ind, o_ptr, tk);
			}
			/* Window stuff */
			p_ptr->window |= (PW_INVEN | PW_EQUIP);

			return;
		}
		else if (prefix(message, "/house") ||
				prefix(message, "/ho"))
		{
			do_cmd_show_houses(Ind);
			return;
		}
		else if (prefix(message, "/object") ||
				prefix(message, "/obj"))
		{
			if (!tk)
			{
				do_cmd_show_known_item_letter(Ind, NULL);
				return;
			}

			if (strlen(token[1]) == 1)
			{
				do_cmd_show_known_item_letter(Ind, token[1]);
				return;
			}

			return;
		}
		else if (prefix(message, "/sip"))
		{
			do_cmd_drink_fountain(Ind);
			return;
		}
		else if (prefix(message, "/fill"))
		{
			do_cmd_fill_bottle(Ind);
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
					/* depth in feet */
					wp.wz = (p_ptr->depth_in_feet ? k / 50 : k);
					break;
				case 3:
					/* depth in feet */
					wp.wx = k % MAX_WILD_X;
					wp.wy = atoi(token[2]) % MAX_WILD_Y;
					wp.wz = atoi(token[3]) / (p_ptr->depth_in_feet ? 50 : 1);
					break;
				case 2:
					wp.wx = k % MAX_WILD_X;
					wp.wy = atoi(token[2]) % MAX_WILD_Y;
					wp.wz = 0;
					break;
				default:
					break;
			}

			if (prefix(message, "/shutdown") ||
					prefix(message, "/quit"))
			{
				bool kick = (cfg.runlevel == 1024);
				set_runlevel(tk ? k :
						((cfg.runlevel < 6 || kick)? 6 : 5));
				msg_format(Ind, "Runlevel set to %d", cfg.runlevel);

				/* Hack -- character edit mode */
				if (k == 1024 || kick)
				{
					if (k == 1024) msg_print(Ind, "\377rEntering edit mode!");
					else msg_print(Ind, "\377rLeaving edit mode!");

					for (i = NumPlayers; i > 0; i--)
					{
						/* Check connection first */
						if (Players[i]->conn == NOT_CONNECTED)
							continue;

						/* Check for death */
						if (!is_admin(Players[i]))
							Destroy_connection(Players[i]->conn, "kicked out");
					}
				}
				time(&cfg.closetime);
				return;
			}
			else if (prefix(message, "/pet")){
#if 1
				if (tk && prefix(token[1], "force"))
				{
					summon_pet(Ind);
					msg_print(Ind, "You summon a pet");
				}
				else
				{
					msg_print(Ind, "Pet code is under working; summoning is bad for your server's health.");
					msg_print(Ind, "If you still want to summon one, type \377o/pet force\377w.");
				}
#endif
				return;
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
						add_banlist(j, (tk>1 ? atoi(token[2]) : 5));
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
				/* Wipe even if town/wilderness */
				wipe_o_list_safely(&wp);
				wipe_m_list(&wp);
				/* XXX trap clearance support dropped - reimplement! */
//				wipe_t_list(&wp);

				msg_format(Ind, "\377rItems/monsters on %s are cleared.", wpos_format(Ind, &wp));
				return;
			}
			else if(prefix(message, "/cp")){
				party_check(Ind);
				return;
			}
			else if (prefix(message, "/geno-level") ||
					prefix(message, "/geno"))
			{
				/* Wipe even if town/wilderness */
				wipe_m_list(&wp);

				msg_format(Ind, "\377rMonsters on %s are cleared.", wpos_format(Ind, &wp));
				return;
			}
			else if (prefix(message, "/unstatic-level") ||
					prefix(message, "/unst"))
			{
				/* no sanity check, so be warned! */
				master_level_specific(Ind, &wp, "u");
				//				msg_format(Ind, "\377rItems and monsters on %dft is cleared.", k * 50);
				return;
			}
			else if (prefix(message, "/static-level") ||
					prefix(message, "/sta"))
			{
				/* no sanity check, so be warned! */
				master_level_specific(Ind, &wp, "s");
				//				msg_format(Ind, "\377rItems and monsters on %dft is cleared.", k * 50);
				return;
			}
			/* TODO: make this player command (using spells, scrolls etc) */
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
				else if (tk > 0 && prefix(token[1], "show"))
				{
					int count = 0;
					for (i = 0; i < MAX_A_IDX ; i++)
					{
						if (!a_info[i].cur_num || a_info[i].known) continue;

						a_info[i].known = TRUE;
						count++;
					}
					msg_format(Ind, "%d 'found but unknown' artifacts are set as '\377Gknown\377w'.", count);
				}
				else if (tk > 0 && prefix(token[1], "fix"))
				{
					int count = 0;
					for (i = 0; i < MAX_A_IDX ; i++)
					{
						if (!a_info[i].cur_num || a_info[i].known) continue;

						a_info[i].cur_num = 0;
						count++;
					}
					msg_format(Ind, "%d 'found but unknown' artifacts are set as '\377rfindable\377w'.", count);
				}
//				else if (tk > 0 && strchr(token[1],'*'))
				else if (tk > 0 && prefix(token[1], "reset!"))
				{
					for (i = 0; i < MAX_A_IDX ; i++)
					{
						a_info[i].cur_num = 0;
						a_info[i].known = FALSE;
					}
					msg_format(Ind, "All the artifacts are \377rfindable\377w!", k);
				}
				else if (tk > 0 && prefix(token[1], "ban!"))
				{
					for (i = 0; i < MAX_A_IDX ; i++)
					{
						a_info[i].cur_num = 1;
						a_info[i].known = TRUE;
					}
					msg_format(Ind, "All the artifacts are \377runfindable\377w!", k);
				}
				else
				{
					msg_print(Ind, "Usage: /artifact (No. | (show | fix | reset! | ban!)");
				}
				return;
			}
			else if (prefix(message, "/unique") ||
					prefix(message, "/uni"))
			{
				bool done = FALSE;
				monster_race *r_ptr;
				for (k = 1; k <= tk; k++)
				{
					if (prefix(token[k], "unseen"))
					{
						for (i = 0; i < MAX_R_IDX - 1 ; i++)
						{
							r_ptr = &r_info[i];
							if (!(r_ptr->flags1 & RF1_UNIQUE)) continue;

							r_ptr->r_sights = 0;
						}
						msg_print(Ind, "All the uniques are set as '\377onot seen\377'.");
						done = TRUE;
					}
					else if (prefix(token[k], "nonkill"))
					{
						monster_race *r_ptr;
						for (i = 0; i < MAX_R_IDX - 1 ; i++)
						{
							r_ptr = &r_info[i];
							if (!(r_ptr->flags1 & RF1_UNIQUE)) continue;

							r_ptr->r_tkills = r_ptr->r_pkills = 0;
						}
						msg_print(Ind, "All the uniques are set as '\377onever killed\377'.");
						done = TRUE;
					}
				}
				if (!done) msg_print(Ind, "Usage: /unique (unseen | nonkill)");
				return;
			}
			else if (prefix(message, "/reload-config") ||
					prefix(message, "/cfg"))
			{
				if (tk)
				{
					MANGBAND_CFG = string_make(token[1]);
				}

				//				msg_print(Ind, "Reloading server option(tomenet.cfg).");
				msg_format(Ind, "Reloading server option(%s).", MANGBAND_CFG);

				/* Reload the server preferences */
				load_server_cfg();
				return;
			}
			/* Admin wishing :)
			 * TODO: better parser like
			 * /wish 21 8 a117 d40
			 * for Aule {40% off}
			 */
			else if (prefix(message, "/wish"))
			{
				object_type	forge;
				object_type	*o_ptr = &forge;

//				if (tk < 2 || !k || !(l = atoi(token[2])))
				if (tk < 1 || !k)
				{
					msg_print(Ind, "\377oUsage: /wish (tval) (sval) [discount] [name] or /wish (o_idx)");
					return;
				}

				/* Move colon pointer forward to next word */
//				while (*arg2 && (isspace(*arg2))) arg2++;

				invcopy(o_ptr, tk > 1 ? lookup_kind(k, atoi(token[2])) : k);

				/* Wish arts out! */
//				if (token[3])
				if (tk > 3)
				{
					int nom = atoi(token[4]);
					o_ptr->number = 1;

					if (nom > 0) o_ptr->name1 = nom;
					else
					{
						/* It's ego or randarts */
						if (nom)
						{
							o_ptr->name2 = 0 - nom;
							if (tk > 3) o_ptr->name2b = 0 - atoi(token[4]);
						}
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
				if (tk > 2){
					o_ptr->discount = atoi(token[3]);
				}
				else{
					o_ptr->discount = 100;
				}
				object_known(o_ptr);
				o_ptr->owner = 0;
				//o_ptr->owner = p_ptr->id;
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
#if 0
				else if (tk)
				{
					i = trap_index(token[1]);
					if (i) wiz_place_trap(Ind, i);
					else msg_print(Ind, "\377oNo such traps.");
				}
#endif	// 0
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
				(void)detect_treasure(Ind, DEFAULT_RADIUS * 2);
				(void)detect_object(Ind, DEFAULT_RADIUS * 2);
				(void)detect_sdoor(Ind, DEFAULT_RADIUS * 2);
				(void)detect_trap(Ind, DEFAULT_RADIUS * 2);
				if (k)
				{
//					(void)detect_trap(Ind);
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
				if (tk) admin_outfit(Ind, k);
//				else admin_outfit(Ind, -1);
				else
				{
					msg_print(Ind, "usage: /eq (realm no.)");
					msg_print(Ind, "    Mage(0) Pray(1) sorc(2) fight(3) shad(4) hunt(5) psi(6) None(else)");
				}
				p_ptr->au = 50000000;
				p_ptr->skill_points = 9999;
				return;
			}
			else if (prefix(message, "/uncurse") ||
					prefix(message, "/unc"))
			{
				remove_all_curse(Ind);
				return;
			}
			/* do a wilderness cleanup */
			else if (prefix(message, "/purge")) 
			{
				msg_format(Ind, "previous server status: m_max(%d) o_max(%d)",
						m_max, o_max);
				compact_monsters(0, TRUE);
				compact_objects(0, TRUE);
//				compact_traps(0, TRUE);
				msg_format(Ind, "current server status: m_max(%d) o_max(%d)",
						m_max, o_max);

				return;
			}
			else if (prefix(message, "/store") ||
					prefix(message, "/sto"))
			{
				store_turnover();
				if (tk && prefix(token[1], "owner"))
				{
					for(i=0;i<numtowns;i++)
					{
//						for (k = 0; k < MAX_STORES - 2; k++)
						for (k = 0; k < max_st_idx; k++)
						{
							/* Shuffle a random shop (except home and auction house) */
							store_shuffle(&town[i].townstore[k]);
						}
					}
					msg_print(Ind, "\377oStore owners had been changed!");
				}
				else msg_print(Ind, "\377GStore inventory had been changed.");

				return;
			}
		}
	}

	do_slash_brief_help(Ind);
	return;
}

int checkallow(char *buff, int pos){
	if(!pos) return(0);
	if(pos==1) return(buff[0]==' ' ? 0 : 1); /* allow things like brass lantern */
	if(buff[pos-1]==' ' || buff[pos-2]=='\377') return(0);	/* swearing in colour */
	return(1);
}

int censor(char *line){
	int i, j;
	char *word;
	char lcopy[100];
	int level=0;
	strcpy(lcopy, line);
	for(i=0; lcopy[i]; i++){
		lcopy[i]=tolower(lcopy[i]);
	}
	for(i=0; strlen(swear[i].word); i++){
		if((word=strstr(lcopy, swear[i].word))){
			if(checkallow(lcopy, word-lcopy)) continue;
			word=(&line[(word-lcopy)]);
			for(j=0; j<strlen(swear[i].word); j++){
				word[j]='*';
			}
			level=MAX(level, swear[i].level);
		}
	}
	return(level);
}

/*
 * A message prefixed by a player name is sent only to that player.
 * Otherwise, it is sent to everyone.
 */
/*
 * This function is hacked *ALOT* to add extra-commands w/o
 * client change.		- Jir -
 */
static void player_talk_aux(int Ind, char *message)
{
 	int i, len, target = 0;
	char search[80], sender[80];
	player_type *p_ptr = Players[Ind], *q_ptr;
 	cptr colon, problem = "";
	bool me = FALSE;
	char c = 'B';
	int mycolor = 0;
	bool admin = is_admin(p_ptr);
	bool broadcast = FALSE;

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
					p_ptr->spam=1;
					player_death(Ind);
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
		else if(!strncmp(message, "/broadcast ", 11)) broadcast = TRUE;
		else{
			do_slash_cmd(Ind, message);	/* add check */
			return;
		}
	}

	process_hooks(HOOK_CHAT, "d", Ind);

	if(++p_ptr->talk>10){
		imprison(Ind, 30, "talking too much.");
		return;
	}

	for(i=1; i<=NumPlayers; i++){
		if(Players[i]->conn==NOT_CONNECTED) continue;
		Players[i]->talk=0;
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
		if(p_ptr->inval){
			msg_print(Ind, "Your account is not valid! Ask an admin to validate it.");
			return;
		}
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
		if(!stricmp(search, "Guild")){
			if(!p_ptr->guild){
				msg_print(Ind, "You are not in a guild");
			}
			else guild_msg_format(p_ptr->guild, "\377v[\377w%s\377v]\377y %s", p_ptr->name, colon+1);
			return;
		}
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
		if (p_ptr->mode & MODE_HELL) c = 'W';
		if (p_ptr->mode & MODE_NO_GHOST) c = 'D';
		if (p_ptr->total_winner) c = 'v';
		else if (p_ptr->ghost) c = 'r';
	}
	switch((i=censor(message))){
		case 0:
			break;
		default:
			imprison(Ind, i*20, "swearing");
		case 1:	msg_print(Ind, "Please do not swear");
	}

	/* Send to everyone */
	for (i = 1; i <= NumPlayers; i++)
	{
		q_ptr = Players[i];

		if (!admin)
		{
			if (check_ignore(i, Ind)) continue;
			if (!broadcast && (p_ptr->limit_chat || q_ptr->limit_chat) &&
					!inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;
		}

		/* Send message */
		if (broadcast)
			msg_format(i, "\377r[\377%c%s\377r] \377B%s", c, sender, message + 11);
		else if (!me)
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
		if (!p_ptr->admin_dm)
			msg_broadcast(Ind, format("\377o%s has returned from AFK.", p_ptr->name));
		p_ptr->afk = FALSE;
	}
	else
	{
		msg_print(Ind, "AFK mode is turned \377rON\377w.");
		if (!p_ptr->admin_dm)
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

			if (q_ptr->admin_dm) continue;

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
char *wpos_format(int Ind, worldpos *wpos)
{
	if (!Ind || Players[Ind]->depth_in_feet)
	return (format("%dft of (%d,%d)", wpos->wz * 50, wpos->wx, wpos->wy));
	else return (format("Lev %d of (%d,%d)", wpos->wz, wpos->wx, wpos->wy));
}


byte count_bits(u32b array)
{
	byte k = 0, i;        

	if(array)
		for(i = 0; i < 32; i++)
			if(array & (1 << i)) k++;

	return k;
}

/*
 * Find a player
 */
int get_playerind(char *name)
{
        int i;

        if (name == (char*)NULL)
                return(-1);
        for(i=1; i<=NumPlayers; i++)
        {
                if(Players[i]->conn==NOT_CONNECTED) continue;
                if(!stricmp(Players[i]->name, name)) return(i);
        }
        return(-1);
}

/*
 * Tell the player of the floor feeling.	- Jir -
 * NOTE: differs from traditional 'boring' etc feeling!
 */
bool show_floor_feeling(int Ind)
{
	player_type *p_ptr = Players[Ind];
	worldpos *wpos = &p_ptr->wpos;
	dun_level *l_ptr = getfloor(wpos);

	if (!l_ptr) return(FALSE);

	/* Hack^2 -- display the 'feeling' */
	if (l_ptr->flags1 & LF1_NO_TELEPORT)
		msg_print(Ind, "\377oYou feel the air is very stable...");
	if (l_ptr->flags1 & LF1_NO_MAGIC)
		msg_print(Ind, "\377oYou feel a suppressive air...");
	if (l_ptr->flags1 & LF1_NO_GENO)
		msg_print(Ind, "\377oYou have a feeling of peace...");
	if (l_ptr->flags1 & LF1_NOMAP)
		msg_print(Ind, "\377oYou feel yourself amnesiac...");
	if (l_ptr->flags1 & LF1_NO_MAGIC_MAP)
		msg_print(Ind, "\377oYou feel like a stranger...");
	if (l_ptr->flags1 & LF1_NO_DESTROY)
		msg_print(Ind, "\377oThe walls here seem very solid.");

	return(l_ptr->flags1 & LF1_FEELING_MASK ? TRUE : FALSE);
}

/*
 * Given item name as string, return the index in k_info array. Name
 * must exactly match (look out for commas and the like!), or else 0 is 
 * returned. Case doesn't matter. -DG-
 */

int test_item_name(cptr name)
{
       int i;

       /* Scan the items */
       for (i = 1; i < max_k_idx; i++)
       {
		object_kind *k_ptr = &k_info[i];
		cptr obj_name = k_name + k_ptr->name;

		/* If name matches, give us the number */
		if (stricmp(name, obj_name) == 0) return (i);
       }
       return (0);
}

/* 
 * Middle-Earth (Imladris) calendar code from ToME
 */
/*
 * Break scalar time
 */
s32b bst(s32b what, s32b t)
{
	s32b turns = t + (10 * DAY_START);

	switch (what)
	{
		case MINUTE:
			return ((turns / 10 / MINUTE) % 60);
		case HOUR:
			return (turns / 10 / (HOUR) % 24);
		case DAY:
			return (turns / 10 / (DAY) % 365);
		case YEAR:
			return (turns / 10 / (YEAR));
		default:
			return (0);
	}
}	    

cptr get_month_name(int day, bool full, bool compact)
{
	int i = 8;
	static char buf[40];

	/* Find the period name */
	while ((i > 0) && (day < month_day[i]))
	{
		i--;
	}

	switch (i)
	{
		/* Yestare/Mettare */
		case 0:
		case 8:
		{
			char buf2[20];

			sprintf(buf2, get_day(day + 1));
			if (full) sprintf(buf, "%s (%s day)", month_name[i], buf2);
			else sprintf(buf, "%s", month_name[i]);
			break;
		}
		/* 'Normal' months + Enderi */
		default:
		{
			char buf2[20];
			char buf3[20];

			sprintf(buf2, get_day(day + 1 - month_day[i]));
			sprintf(buf3, get_day(day + 1));

			if (full) sprintf(buf, "%s day of %s (%s day)", buf2, month_name[i], buf3);
			else if (compact) sprintf(buf, "%s day of %s", buf2, month_name[i]);
			else sprintf(buf, "%s %s", buf2, month_name[i]);
			break;
		}
	}

	return (buf);
}

cptr get_day(int day)
{
	static char buf[20];
	cptr p = "th";

	if ((day / 10) == 1) ;
	else if ((day % 10) == 1) p = "st";
	else if ((day % 10) == 2) p = "nd";
	else if ((day % 10) == 3) p = "rd";

	sprintf(buf, "%d%s", day, p);
	return (buf);
}

int gold_colour(int amt)
{
	int i, unit = 1;

	for (i = amt; i > 99 ; i >>= 1, unit++) /* naught */;
	if (unit > SV_GOLD_MAX) unit = SV_GOLD_MAX;
	return (lookup_kind(TV_GOLD, unit));
}

