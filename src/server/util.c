/* $Id$ */
/* File: util.c */

/* Purpose: Angband utilities -BEN- */


#define SERVER

#include "angband.h"
#ifdef TOMENET_WORLDS
#include "../world/world.h"
#endif

/* For gettimeofday */
#include <sys/time.h>

static void console_talk_aux(char *message);


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
	for (t = s; n--; ) *t++ = c;
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

int in_banlist(char *acc, char *addr, int *time, char *reason) {
	struct combo_ban *ptr;
	int found = 0x0;

	for (ptr = banlist; ptr != (struct combo_ban*)NULL; ptr = ptr->next) {
		if (ptr->ip[0] && addr && !strcmp(addr, ptr->ip)) found |= 0x1;
		if (ptr->acc[0] && acc && !strcmp(acc, ptr->acc)) found |= 0x2;

		if (reason) strcpy(reason, ptr->reason);
		if (time) *time = ptr->time;

		return found;
	}
	return 0x0;
}

void check_banlist() {
	struct combo_ban *ptr, *new, *old = (struct combo_ban*)NULL;
	ptr = banlist;
	while (ptr != (struct combo_ban*)NULL) {
		if (ptr->time) {
			if (!(--ptr->time)) {
				if (ptr->reason[0]) s_printf("Unbanning due to ban timeout (ban reason was '%s'):\n", ptr->reason);
				else s_printf("Unbanning due to ban timeout:\n");
				if (ptr->ip[0]) s_printf(" Connections from %s\n", ptr->ip);
				if (ptr->acc[0]) s_printf(" Connections for %s\n", ptr->acc);

				if (!old) {
					banlist = ptr->next;
					new = banlist;
				} else {
					old->next = ptr->next;
					new = old->next;
				}
				free(ptr);
				ptr = new;
				continue;
			}
		}
		ptr = ptr->next;
	}
}


#ifdef SET_UID

# ifndef HAS_USLEEP

/*
 * For those systems that don't have "usleep()" but need it.
 *
 * Fake "usleep()" function grabbed from the inl netrek server -cba
 */
static int usleep(huge microSeconds) {
	struct timeval Timer;
	int nfds = 0;
#ifdef FD_SET
	fd_set *no_fds = NULL;
#else
	int *no_fds = NULL;
#endif


	/* Was: int readfds, writefds, exceptfds; */
	/* Was: readfds = writefds = exceptfds = 0; */


	/* Paranoia -- No excessive sleeping */
	if (microSeconds > 4000000L) core("Illegal usleep() call");


	/* Wait for it */
	Timer.tv_sec = (microSeconds / 1000000L);
	Timer.tv_usec = (microSeconds % 1000000L);

	/* Wait for it */
	if (select(nfds, no_fds, no_fds, no_fds, &Timer) < 0) {
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
errr path_parse(char *buf, int max, cptr file) {
	cptr u, s;
	struct passwd *pw;
	char user[128];


	/* Assume no result */
	buf[0] = '\0';

	/* No file? */
	if (!file) return (-1);

	/* File needs no parsing */
	if (file[0] != '~') {
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
#ifdef WINDOWS
	static u32b tmp_counter;
	static char valid_characters[] =
			"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char rand_ext[4];

	rand_ext[0] = valid_characters[rand_int(sizeof (valid_characters))];
	rand_ext[1] = valid_characters[rand_int(sizeof (valid_characters))];
	rand_ext[2] = valid_characters[rand_int(sizeof (valid_characters))];
	rand_ext[3] = '\0';
	strnfmt(buf, max, "%s/server_%ud.%s", ANGBAND_DIR_DATA, tmp_counter, rand_ext);
	tmp_counter++;
#else 
	cptr s;

	/* Temp file */
	s = tmpnam(NULL);

	/* Oops */
	if (!s) return (-1);

	/* Format to length */
	strnfmt(buf, max, "%s", s);
#endif
	/* Success */
	return (0);
}

/*
 * Hack -- replacement for "fopen()"
 */
FILE *my_fopen(cptr file, cptr mode) {
	char buf[1024];

	/* Hack -- Try to parse the path */
	if (path_parse(buf, 1024, file)) return (NULL);

	/* Attempt to fopen the file anyway */
	return (fopen(buf, mode));
}


/*
 * Hack -- replacement for "fclose()"
 */
errr my_fclose(FILE *fff) {
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
errr my_fgets(FILE *fff, char *buf, huge n, bool conv)
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
			else if (isprint(*s) || *s == '\377')
			{
				/* easier to edit perma files */
				if (conv && *s == '{' && *(s + 1) != '{')
					*s = '\377';
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
errr fd_kill(cptr file) {
	char buf[1024];

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
errr fd_move(cptr file, cptr what) {
	char buf[1024];
	char aux[1024];

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
errr fd_copy(cptr file, cptr what) {
	char buf[1024];
	char aux[1024];

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
int fd_make(cptr file, int mode) {
	char buf[1024];

	/* Hack -- Try to parse the path */
	if (path_parse(buf, 1024, file)) return (-1);

#ifdef BEN_HACK

	/* Check for existence */
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
int fd_open(cptr file, int flags) {
	char buf[1024];

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
errr fd_lock(int fd, int what) {
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
	huge p;

	/* Verify fd */
	if (fd < 0) return (-1);

	/* Seek to the given position */
	p = lseek(fd, n, SEEK_SET);

	/* Failure */
	if (p == (huge) -1) return (1);

	/* Failure */
	if (p != n) return (1);

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
	if ((huge) read(fd, buf, n) != n) return (1);

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
	if ((huge) write(fd, buf, n) != n) return (1);

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
#ifndef USE_SOUND_2010
void sound(int Ind, int val) {
	//Send_sound(Ind, val);
	Send_sound(Ind, val, 0, 0, 100, 0);
}
#else
/* 'type' is used client-side, for efficiency options concerning near-simultaneous sounds
   'nearby' means if other players nearby would be able to also hear the sound. - C. Blue */
void sound(int Ind, cptr name, cptr alternative, int type, bool nearby) {
#if 0 /* non-optimized way (causes LUA errors if sound() is called in fire_ball() which is in turn called from LUA - C. Blue */
	int val, val2;

	val = exec_lua(0, format("return get_sound_index(\"%s\")", name));

	if (alternative) {
		val2 = exec_lua(0, format("return get_sound_index(\"%s\")", alternative));
	} else {
		val2 = -1;
	}

#else /* optimized way... */
	player_type *p_ptr = Players[Ind];
	int val = -1, val2 = -1, i, d;

	/* backward compatibility */
	if (type == SFX_TYPE_STOP && !is_newer_than(&p_ptr->version, 4, 6, 1, 1, 0, 0)) return;

	if (name) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], name)) {
			val = i;
			break;
		}
	}

	if (alternative) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], alternative)) {
			val2 = i;
			break;
		}
	}

#endif

	//if (is_admin(p_ptr)) s_printf("USE_SOUND_2010: looking up sound %s -> %d.\n", name, val);

	if (val == -1) {
		if (val2 != -1) {
			/* Use the alternative instead */
			val = val2;
			val2 = -1;
		} else {
			/* hack for SFX_TYPE_STOP */
			if (name || alternative)

			return;
		}
	}

	/* also send sounds to nearby players, depending on sound or sound type */
	if (nearby) {
		for (i = 1; i <= NumPlayers; i++) {
			if (Players[i]->conn == NOT_CONNECTED) continue;
			if (!inarea(&Players[i]->wpos, &p_ptr->wpos)) continue;
			if (Ind == i) continue;

			/* backward compatibility */
			if (type == SFX_TYPE_STOP && !is_newer_than(&Players[i]->version, 4, 6, 1, 1, 0, 0)) continue;

			d = distance(p_ptr->py, p_ptr->px, Players[i]->py, Players[i]->px);
#if 0
			if (d > 10) continue;
			if (d == 0) d = 1; //paranoia oO

			Send_sound(i, val, val2, type, 100 / d, p_ptr->id);
			//Send_sound(i, val, val2, type, (6 - d) * 20, p_ptr->id);  hm or this?
#else
			if (d > MAX_SIGHT) continue;
			d += 3;
			d /= 2;

			Send_sound(i, val, val2, type, 100 / d, p_ptr->id);
#endif
		}
	}

	Send_sound(Ind, val, val2, type, 100, p_ptr->id);
}
void sound_vol(int Ind, cptr name, cptr alternative, int type, bool nearby, int vol) {
	int val = -1, val2 = -1, i, d;

	if (name) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], name)) {
			val = i;
			break;
		}
	}

	if (alternative) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], alternative)) {
			val2 = i;
			break;
		}
	}


	//if (is_admin(Players[Ind])) s_printf("USE_SOUND_2010: looking up sound %s -> %d.\n", name, val);

	if (val == -1) {
		if (val2 != -1) {
			/* Use the alternative instead */
			val = val2;
			val2 = -1;
		} else {
			return;
		}
	}

	/* also send sounds to nearby players, depending on sound or sound type */
	if (nearby) {
		for (i = 1; i <= NumPlayers; i++) {
			if (Players[i]->conn == NOT_CONNECTED) continue;
			if (!inarea(&Players[i]->wpos, &Players[Ind]->wpos)) continue;
			if (Ind == i) continue;

			d = distance(Players[Ind]->py, Players[Ind]->px, Players[i]->py, Players[i]->px);
#if 0
			if (d > 10) continue;
			if (d == 0) d = 1; //paranoia oO

			Send_sound(i, val, val2, type, vol / d, Players[Ind]->id);
			//Send_sound(i, val, val2, type, vol... (6 - d) * 20, Players[Ind]->id);  hm or this?
#else
			if (d > MAX_SIGHT) continue;
			d += 3;
			d /= 2;

			Send_sound(i, val, val2, type, vol / d, Players[Ind]->id);
#endif
		}
	}
	Send_sound(Ind, val, val2, type, vol, Players[Ind]->id);
}
/* Send sound to origin player and destination player,
   with destination player getting reduced volume depending on distance */
void sound_pair(int Ind_org, int Ind_dest, cptr name, cptr alternative, int type) {
	int val = -1, val2 = -1, i, d;

	if (name) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], name)) {
			val = i;
			break;
		}
	}

	if (alternative) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], alternative)) {
			val2 = i;
			break;
		}
	}

	if (val == -1) {
		if (val2 != -1) {
			/* Use the alternative instead */
			val = val2;
			val2 = -1;
		} else {
			return;
		}
	}

	if (Players[Ind_dest]->conn != NOT_CONNECTED &&
	    inarea(&Players[Ind_dest]->wpos, &Players[Ind_org]->wpos) &&
	    Ind_dest != Ind_org) {
		d = distance(Players[Ind_org]->py, Players[Ind_org]->px, Players[Ind_dest]->py, Players[Ind_dest]->px);
		if (d <= MAX_SIGHT) {
			d += 3;
			d /= 2;
			Send_sound(Ind_dest, val, val2, type, 100 / d, Players[Ind_org]->id);
		}
	}

	Send_sound(Ind_org, val, val2, type, 100, Players[Ind_org]->id);
}
/* Send sound to everyone on a particular wpos sector/dungeon floor */
void sound_floor_vol(struct worldpos *wpos, cptr name, cptr alternative, int type, int vol) {
	int val = -1, val2 = -1, i;

	if (name) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], name)) {
			val = i;
			break;
		}
	}

	if (alternative) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], alternative)) {
			val2 = i;
			break;
		}
	}

	if (val == -1) {
		if (val2 != -1) {
			/* Use the alternative instead */
			val = val2;
			val2 = -1;
		} else {
			return;
		}
	}

	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (!inarea(&Players[i]->wpos, wpos)) continue;
		Send_sound(i, val, val2, type, vol, Players[i]->id);
	}
}
/* send sound to player and everyone nearby at full volume, similar to
   msg_..._near(), except it also sends to the player himself.
   This is used for highly important and *loud* sounds such as 'shriek' - C. Blue */
void sound_near(int Ind, cptr name, cptr alternative, int type) {
	int i, d;
	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (!inarea(&Players[i]->wpos, &Players[Ind]->wpos)) continue;

		/* Can player see the target player? */
		//if (!(Players[i]->cave_flag[Players[Ind]->py][Players[Ind]->px] & CAVE_VIEW)) continue;

		/* within audible range? */
		d = distance(Players[Ind]->py, Players[Ind]->px, Players[i]->py, Players[i]->px);
		if (d > MAX_SIGHT) continue;

#ifndef SFX_SHRIEK_VOLUME
		if (strcmp(name, "shriek") || Players[i]->sfx_shriek)
			sound(i, name, alternative, type, FALSE);
#else
		if (strcmp(name, "shriek"))
			sound(i, name, alternative, type, FALSE);
		else if (Players[i]->sfx_shriek)
			sound_vol(i, name, alternative, type, FALSE, SFX_SHRIEK_VOLUME);
#endif
	}
}
/* send sound to all players nearby a certain location, and allow to specify
   a player to exclude, same as msg_print_near_site() for messages. - C. Blue */
void sound_near_site(int y, int x, worldpos *wpos, int Ind, cptr name, cptr alternative, int type, bool viewable) {
	int i, d;
	player_type *p_ptr;
	int val = -1, val2 = -1;

	if (name) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], name)) {
			val = i;
			break;
		}
	}

	if (alternative) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], alternative)) {
			val2 = i;
			break;
		}
	}

	if (val == -1) {
		if (val2 != -1) {
			/* Use the alternative instead */
			val = val2;
			val2 = -1;
		} else {
			return;
		}
	}

	/* Check each player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this player */
		p_ptr = Players[i];

		/* Make sure this player is in the game */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Skip specified player, if any */
		if (i == Ind) continue;

		/* Make sure this player is at this depth */
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Can player see the site via LOS? */
		if (viewable && !(p_ptr->cave_flag[y][x] & CAVE_VIEW)) continue;

		/* within audible range? */
		d = distance(y, x, Players[i]->py, Players[i]->px);
		/* NOTE: should be consistent with msg_print_near_site() */
		if (d > MAX_SIGHT) continue;

#if 0
		/* limit for volume calc */
		if (d > 20) d = 20;
		d += 3;
		d /= 3;
		Send_sound(i, val, val2, type, 100 / d, 0);
#else
		/* limit for volume calc */
		Send_sound(i, val, val2, type, 100 - (d * 50) / 11, 0);
#endif
	}
}
/* send sound to all players nearby a certain location, and allow to specify
   a player to exclude, same as msg_print_near_site() for messages. - C. Blue */
void sound_near_site_vol(int y, int x, worldpos *wpos, int Ind, cptr name, cptr alternative, int type, bool viewable, int vol) {
	int i, d;
	player_type *p_ptr;
	int val = -1, val2 = -1;

	if (name) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], name)) {
			val = i;
			break;
		}
	}

	if (alternative) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], alternative)) {
			val2 = i;
			break;
		}
	}

	if (val == -1) {
		if (val2 != -1) {
			/* Use the alternative instead */
			val = val2;
			val2 = -1;
		} else {
			return;
		}
	}

	/* Check each player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this player */
		p_ptr = Players[i];

		/* Make sure this player is in the game */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Skip specified player, if any */
		if (i == Ind) continue;

		/* Make sure this player is at this depth */
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Can player see the site via LOS? */
		if (viewable && !(p_ptr->cave_flag[y][x] & CAVE_VIEW)) continue;

		/* within audible range? */
		d = distance(y, x, Players[i]->py, Players[i]->px);
		/* NOTE: should be consistent with msg_print_near_site() */
		if (d > MAX_SIGHT) continue;

#if 0
		/* limit for volume calc */
		if (d > 20) d = 20;
		d += 3;
		d /= 3;
		Send_sound(i, val, val2, type, ((100 / d) * vol) / 100, 0);
#else
		/* limit for volume calc */
		Send_sound(i, val, val2, type, ((100 - (d * 50) / 11) * vol) / 100, 0);
#endif
	}
}
/* Play sfx at full volume to everyone in a house, and at normal-distance volume to
   everyone near the door (as sound_near_site() would). */
void sound_house_knock(int h_idx, int dx, int dy) {
	house_type *h_ptr = &houses[h_idx];

	fill_house(h_ptr, FILL_SFX_KNOCK, NULL);

	/* basically sound_near_site(), except we skip players on CAVE_ICKY grids, ie those who are in either this or other houses! */
	int i, d;
	player_type *p_ptr;
	int val = -1, val2 = -1;

	const char *name = (houses[h_idx].flags & HF_MOAT) ? "knock_castle" : "knock";
	const char *alternative = "block_shield_projectile";
	cave_type **zcave;

	if (name) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], name)) {
			val = i;
			break;
		}
	}

	if (alternative) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], alternative)) {
			val2 = i;
			break;
		}
	}

	if (val == -1) {
		if (val2 != -1) {
			/* Use the alternative instead */
			val = val2;
			val2 = -1;
		} else {
			return;
		}
	}

	/* Check each player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this player */
		p_ptr = Players[i];

		/* Make sure this player is in the game */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Make sure this player is at this depth */
		if (!inarea(&p_ptr->wpos, &h_ptr->wpos)) continue;

		zcave = getcave(&p_ptr->wpos);
		if (!zcave) continue;//paranoia
		/* Specialty for sound_house(): Is player NOT in a house? */
		if ((zcave[p_ptr->py][p_ptr->px].info & CAVE_ICKY)
		    /* however, exempt the moat! */
		    && zcave[p_ptr->py][p_ptr->px].feat != FEAT_DRAWBRIDGE
		    && zcave[p_ptr->py][p_ptr->px].feat != FEAT_DEEP_WATER)
			continue;

		/* within audible range? */
		d = distance(dy, dx, Players[i]->py, Players[i]->px);
		/* hack: if we knock on the door, assume distance 0 between us and door */
		if (d > 0) d--;
		/* NOTE: should be consistent with msg_print_near_site() */
		if (d > MAX_SIGHT) continue;

#if 0
		/* limit for volume calc */
		if (d > 20) d = 20;
		d += 3;
		d /= 3;
		Send_sound(i, val, val2, SFX_TYPE_COMMAND, 100 / d, 0);
#else
		/* limit for volume calc */
		Send_sound(i, val, val2, SFX_TYPE_COMMAND, 100 - (d * 50) / 11, 0);
#endif
	}

}
/* like msg_print_near_monster() just for sounds,
   basically same as sound_near_site() except no player can be exempt - C. Blue */
void sound_near_monster(int m_idx, cptr name, cptr alternative, int type) {
	int i, d;
	player_type *p_ptr;
	cave_type **zcave;

	monster_type *m_ptr = &m_list[m_idx];
	worldpos *wpos = &m_ptr->wpos;
	int val = -1, val2 = -1;

	if (!(zcave = getcave(wpos))) return;

	if (name) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], name)) {
			val = i;
			break;
		}
	}

	if (alternative) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], alternative)) {
			val2 = i;
			break;
		}
	}

	if (val == -1) {
		if (val2 != -1) {
			/* Use the alternative instead */
			val = val2;
			val2 = -1;
		} else {
			return;
		}
	}

	/* Check each player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this player */
		p_ptr = Players[i];

		/* Make sure this player is in the game */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Make sure this player is at this depth */
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Skip if not visible */
		//if (!p_ptr->mon_vis[m_idx]) continue;

		/* Can player see this monster via LOS? */
		//if (!(p_ptr->cave_flag[m_ptr->fy][m_ptr->fx] & CAVE_VIEW)) continue;

		/* within audible range? */
		d = distance(m_ptr->fy, m_ptr->fx, Players[i]->py, Players[i]->px);

		/* NOTE: should be consistent with msg_print_near_site() */
		if (d > MAX_SIGHT) continue;

#if 0
		/* limit for volume calc */
		if (d > 20) d = 20;
		d += 3;
		d /= 3;
		Send_sound(i, val, val2, type, 100 / d, 0);
#else
		/* limit for volume calc */
		Send_sound(i, val, val2, type, 100 - (d * 50) / 11, 0);
#endif
	}
}
/* like msg_print_near_monster() just for sounds,
   basically same as sound_near_site(). - C. Blue
   NOTE: This function assumes that it's playing a MONSTER ATTACK type sound,
         because it's checking p_ptr->sfx_monsterattack to mute it. */
void sound_near_monster_atk(int m_idx, int Ind, cptr name, cptr alternative, int type) {
	int i, d;
	player_type *p_ptr;
	cave_type **zcave;

	monster_type *m_ptr = &m_list[m_idx];
	worldpos *wpos = &m_ptr->wpos;
	int val = -1, val2 = -1;

	if (!(zcave = getcave(wpos))) return;

	if (name) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], name)) {
			val = i;
			break;
		}
	}

	if (alternative) for (i = 0; i < SOUND_MAX_2010; i++) {
		if (!audio_sfx[i][0]) break;
		if (!strcmp(audio_sfx[i], alternative)) {
			val2 = i;
			break;
		}
	}

	if (val == -1) {
		if (val2 != -1) {
			/* Use the alternative instead */
			val = val2;
			val2 = -1;
		} else {
			return;
		}
	}

	/* Check each player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this player */
		p_ptr = Players[i];

		/* Make sure this player is in the game */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Skip specified player, if any */
		if (i == Ind) continue;

		/* Skip players who don't want to hear attack sounds */
		if (!p_ptr->sfx_monsterattack) continue;

		/* Make sure this player is at this depth */
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Skip if not visible */
		//if (!p_ptr->mon_vis[m_idx]) continue;

		/* Can player see this monster via LOS? */
		//if (!(p_ptr->cave_flag[m_ptr->fy][m_ptr->fx] & CAVE_VIEW)) continue;

		/* within audible range? */
		d = distance(m_ptr->fy, m_ptr->fx, Players[i]->py, Players[i]->px);

		/* NOTE: should be consistent with msg_print_near_site() */
		if (d > MAX_SIGHT) continue;

#if 0
		/* limit for volume calc */
		if (d > 20) d = 20;
		d += 3;
		d /= 3;
		Send_sound(i, val, val2, type, 100 / d, 0);
#else
		/* limit for volume calc */
		Send_sound(i, val, val2, type, 100 - (d * 50) / 11, 0);
#endif
	}
}

/* Find correct music for the player based on his current location - C. Blue
 * Note - rarely played music:
 *     dungeons - generic/ironman/forcedownhellish
 *     towns - generic day/night (used for Menegroth/Nargothrond at times)
 * Note - dun-gen-iron and dun-gen-fdhell are currently swapped. */
void handle_music(int Ind) {
	player_type *p_ptr = Players[Ind];
	dun_level *l_ptr = NULL;
	int i = -1, dlev = getlevel(&p_ptr->wpos);
	dungeon_type *d_ptr = getdungeon(&p_ptr->wpos);
	cave_type **zcave = getcave(&p_ptr->wpos);

#ifdef ARCADE_SERVER
	/* Special music for Arcade Server stuff */
	if (p_ptr->wpos.wx == WPOS_ARCADE_X && p_ptr->wpos.wy == WPOS_ARCADE_Y
	//    && p_ptr->wpos.wz == WPOS_ARCADE_Z
	    ) {
		p_ptr->music_monster = -2;
 #if 0
		if (p_ptr->wpos.wz == 0) Send_music(Ind, 1, -1); /* 'generic town' music instead of Bree default */
		else {
			//47 and 48 are actually pieces used in other arena events
			if (rand_int(2)) Send_music(Ind, 47, 1);
			else Send_music(Ind, 48, 1);
		}
 #else
		if (p_ptr->wpos.wz == 0) Send_music(Ind, 48, 1); /* 'arena' ;) sounds a bit like Unreal Tournament menu music hehe */
		else Send_music(Ind, 47, 1); /* 'death match' music (pvp arena) */
 #endif
		return;
	}
#endif

#ifdef IRONDEEPDIVE_MIXED_TYPES
	if (in_irondeepdive(&p_ptr->wpos)) {
		l_ptr = getfloor(&p_ptr->wpos);
 #if 0 /* kinda annoying to have 2 floors of 'unspecified' music - C. Blue */
		if (!iddc[ABS(p_ptr->wpos.wz)].step) i = iddc[ABS(p_ptr->wpos.wz)].type;
		else i = 0; //Transition floors
 #else
		if (iddc[ABS(p_ptr->wpos.wz)].step < 2) i = iddc[ABS(p_ptr->wpos.wz)].type;
		else i = iddc[ABS(p_ptr->wpos.wz)].next;
 #endif
	} else
#endif
	if (p_ptr->wpos.wz != 0) {
		l_ptr = getfloor(&p_ptr->wpos);
		if (p_ptr->wpos.wz < 0) {
			i = wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].dungeon->type;
#if 1			/* allow 'themed' music too? (or keep music as 'original dungeon theme' exclusively) */
			if (!i && //except if this dungeon has particular flags (because flags are NOT affected by theme):
			    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].dungeon->theme &&
			    !(d_ptr->flags1 & DF1_FORCE_DOWN) && !(d_ptr->flags2 & (DF2_NO_DEATH | DF2_IRON | DF2_HELL)))
				i = wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].dungeon->theme;
#endif
		} else {
			i = wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].tower->type;
#if 1			/* allow 'themed' music too? (or keep music as 'original dungeon theme' exclusively) */
			if (!i && //except if this dungeon has particular flags (because flags are NOT affected by theme):
			    wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].tower->theme &&
			    !(d_ptr->flags1 & DF1_FORCE_DOWN) && !(d_ptr->flags2 & (DF2_NO_DEATH | DF2_IRON | DF2_HELL)))
				i = wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].tower->theme;
#endif
		}
	}

	if (in_netherrealm(&p_ptr->wpos) && dlev == netherrealm_end) {
		//Zu-Aon
		//hack: init music as 'higher priority than boss-specific':
		p_ptr->music_monster = -2;
		Send_music(Ind, 45, 14);
		return;
	} else if ((i != -1) && (l_ptr->flags1 & LF1_NO_GHOST)) { /* Assuming that only Morgoth's floor has LF1_NO_GHOST */
		//Morgoth
		//hack: init music as 'higher priority than boss-specific':
		p_ptr->music_monster = -2;
		if (p_ptr->total_winner) Send_music(Ind, 88, 44);
		else Send_music(Ind, 44, 14);
		return;
	}

	if (in_valinor(&p_ptr->wpos)) {
		//hack: init music as 'higher priority than boss-specific':
		p_ptr->music_monster = -2;
		Send_music(Ind, 8, 1); //Valinor
		return;
	} else if (in_pvparena(&p_ptr->wpos)) {
		//hack: init music as 'higher priority than boss-specific':
		p_ptr->music_monster = -2;
		Send_music(Ind, 66, 47); //PvP Arena (Highlander Deathmatch as alternative)
		return;
	} else if (ge_special_sector && in_arena(&p_ptr->wpos)) {
		//hack: init music as 'higher priority than boss-specific':
		p_ptr->music_monster = -2;
		Send_music(Ind, 48, 0); //Monster Arena Challenge
		return;
	} else if (in_sector00(&p_ptr->wpos)) {
		//hack: init music as 'higher priority than boss-specific':
		p_ptr->music_monster = -2;
		Send_music(Ind, sector00music, sector00musicalt);
		return;
	} else if (in_sector00_dun(&p_ptr->wpos)) {
		//hack: init music as 'higher priority than boss-specific':
		p_ptr->music_monster = -2;
		Send_music(Ind, sector00music_dun, sector00musicalt_dun);
		return;
	}

	/* No-tele grid: Re-use 'terrifying' bgm for this */
	if (p_ptr->music_monster != -5 && //hack: jails are also no-tele
	    zcave && (zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK)) {
#if 0 /* hack: init music as 'higher priority than boss-specific': */
		p_ptr->music_monster = -2;
#endif
		Send_music(Ind, 46, 14); //No-Tele vault
		return;
	}

	/* rest of the music has lower priority than already running, boss-specific music */
	if (p_ptr->music_monster != -1
	    /* hacks for sickbay, tavern and jail music */
	    && p_ptr->music_monster != -3 && p_ptr->music_monster != -4 && p_ptr->music_monster != -5)
		return;


	/* on world surface */
	if (p_ptr->wpos.wz == 0) {
		/* Jail hack */
		if (p_ptr->music_monster == -5) {
			Send_music(Ind, 87, 46);
			return;
		}

		/* play town music also in its surrounding area of houses, if we're coming from the town? */
		if (istownarea(&p_ptr->wpos, MAX_TOWNAREA)) {
			int tmus = 0, tmus_inverse = 0;
			int tmus_reserve = 1; //generic town day music

			i = wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].town_idx;

			if (night_surface) switch (town[i].type) { //nightly music
			default:
			case TOWN_VANILLA: tmus = 49; tmus_inverse = 1; break; //default town
			case TOWN_BREE: tmus = 50; tmus_inverse = 3; break; //Bree
			case TOWN_GONDOLIN: tmus = 51; tmus_inverse = 4; break; //Gondo
			case TOWN_MINAS_ANOR: tmus = 52; tmus_inverse = 5; break; //Minas
			case TOWN_LOTHLORIEN: tmus = 53; tmus_inverse = 6; break; //Loth
			case TOWN_KHAZADDUM: tmus = 54; tmus_inverse = 7; break; //Khaz
			}
			else switch (town[i].type) { //daily music
			default:
			case TOWN_VANILLA: tmus = 1; tmus_inverse = 49; break; //default town
			case TOWN_BREE: tmus = 3; tmus_inverse = 50; break; //Bree
			case TOWN_GONDOLIN: tmus = 4; tmus_inverse = 51; break; //Gondo
			case TOWN_MINAS_ANOR: tmus = 5; tmus_inverse = 52; break; //Minas
			case TOWN_LOTHLORIEN: tmus = 6; tmus_inverse = 53; break; //Loth
			case TOWN_KHAZADDUM: tmus = 7; tmus_inverse = 54; break; //Khaz
			}

			/* Sickbay hack */
			if (p_ptr->music_monster == -3) {
				Send_music(Ind, 86, tmus);
				return;
			}
			/* Tavern hack */
			if (p_ptr->music_monster == -4) {
				//abuse tmus_inverse
				if (night_surface) switch (town[i].type) { //nightly music
				default:
				case TOWN_VANILLA: tmus_inverse = 69; break; //default town
				case TOWN_BREE: tmus_inverse = 71; break; //Bree
				case TOWN_GONDOLIN: tmus_inverse = 73; break; //Gondo
				case TOWN_MINAS_ANOR: tmus_inverse = 75; break; //Minas
				case TOWN_LOTHLORIEN: tmus_inverse = 77; break; //Loth
				case TOWN_KHAZADDUM: tmus_inverse = 79; break; //Khaz
				}
				else switch (town[i].type) { //daily music
				default:
				case TOWN_VANILLA: tmus_inverse = 68; break; //default town
				case TOWN_BREE: tmus_inverse = 70; break; //Bree
				case TOWN_GONDOLIN: tmus_inverse = 72; break; //Gondo
				case TOWN_MINAS_ANOR: tmus_inverse = 74; break; //Minas
				case TOWN_LOTHLORIEN: tmus_inverse = 76; break; //Loth
				case TOWN_KHAZADDUM: tmus_inverse = 78; break; //Khaz
				}
				Send_music(Ind, tmus_inverse, tmus);
				return;
			}

			/* Seasonal music, overrides the music of the place (usually Bree) where it mainly takes place */
			if (season_halloween) {
				/* Designated place: Bree */
				if (town[i].type == TOWN_BREE) {
					tmus_reserve = tmus_inverse; //correctly counter-switch day vs night music again, as replacement for dedicated event music
					tmus_inverse = tmus;
					tmus = 83;
				}
			}
			else if (season_xmas) {
				/* Designated place: Bree */
				if (town[i].type == TOWN_BREE) {
					tmus_reserve = tmus_inverse; //correctly counter-switch day vs night music again, as replacement for dedicated event music
					tmus_inverse = tmus;
					tmus = 84;
				}
			}
			else if (season_newyearseve) {
				/* Designated place: Bree */
				if (town[i].type == TOWN_BREE) {
					tmus_reserve = tmus_inverse; //correctly counter-switch day vs night music again, as replacement for dedicated event music
					tmus_inverse = tmus;
					tmus = 85;
				}
			}

			/* now the specialty: If we're coming from elsewhere,
			   we only switch to town music when we enter the town.
			   If we're coming from the town, however, we keep the
			   music while being in its surrounding area of houses. */
			if (istown(&p_ptr->wpos) || p_ptr->music_current == tmus
			    /* don't switch from town area music to wild music on day/night change: */
			    || p_ptr->music_current == tmus_inverse)
				Send_music(Ind, tmus, night_surface ? tmus_inverse : tmus_reserve);
			/* Wilderness music */
			else if (night_surface) Send_music(Ind, 10, 0);
			else Send_music(Ind, 9, 0);
			return;
		} else {
			if (night_surface) Send_music(Ind, 10, 0);
			else Send_music(Ind, 9, 0);
			return;
		}
	/* in the dungeon */
	} else {
		/* Dungeon towns have their own music to bolster the player's motivation ;) */
		if (isdungeontown(&p_ptr->wpos)) {
			if (is_fixed_irondeepdive_town(&p_ptr->wpos, dlev)) {
				if (dlev == 40) {
					/* Tavern hack */
					if (p_ptr->music_monster == -4) {
						Send_music(Ind, 81, 57);
						return;
					}
					Send_music(Ind, 57, 1); /* Menegroth: own music, fallback to generic town */
				} else {
					/* Tavern hack */
					if (p_ptr->music_monster == -4) {
						Send_music(Ind, 82, 58);
						return;
					}
					Send_music(Ind, 58, 49); /* Nargothrond: own music, fallback to generic town night */
				}
			} else {
				/* Tavern hack */
				if (p_ptr->music_monster == -4) {
					Send_music(Ind, 80, 2);
					return;
				}
				Send_music(Ind, 2, 1); /* the usual music for this case */
			}
			return;
		}

		/* Floor-specific music (monster/boss-independant)? */
		if ((i != DI_NETHER_REALM) /*not in Nether Realm, really ^^*/
		    && (!(d_ptr->flags2 & DF2_NO_DEATH)) /* nor in easy dungeons */
		    && !(p_ptr->wpos.wx == WPOS_PVPARENA_X /* and not in pvp-arena */
		    && p_ptr->wpos.wy == WPOS_PVPARENA_Y && p_ptr->wpos.wz == WPOS_PVPARENA_Z))
		{
			if (p_ptr->distinct_floor_feeling || is_admin(p_ptr)) {
				if (l_ptr->flags2 & LF2_OOD_HI) {
					Send_music(Ind, 46, 14); //what a terrifying place
					return;
				}
			}
		}

		//we could just look through audio.lua, by querying get_music_name() I guess..
		switch (i) {
		default:
		case 0:
			if (d_ptr->flags2 & DF2_NO_DEATH) Send_music(Ind, 12, 11);//note: music file (dungeon_generic_nodeath) is identical to the one of the Training Tower
			else if (d_ptr->flags2 & DF2_IRON) Send_music(Ind, 13, 11);
			else if ((d_ptr->flags2 & DF2_HELL) || (d_ptr->flags1 & DF1_FORCE_DOWN)) Send_music(Ind, 14, 11);
			else Send_music(Ind, 11, 0); //dungeon, generic
			return;
		case 1: Send_music(Ind, 32, 11); return; //Mirkwood
		case 2: Send_music(Ind, 17, 11); return; //Mordor
		case 3: Send_music(Ind, 19, 11); return; //Angband
		case 4: Send_music(Ind, 16, 11); return; //Barrow-Downs
		case 5: Send_music(Ind, 21, 11); return; //Mount Doom
		case 6: Send_music(Ind, 22, 13); return; //Nether Realm
		case 7: Send_music(Ind, 35, 11); return; //Submerged Ruins
		case 8: Send_music(Ind, 26, 11); return; //Halls of Mandos
		case 9: Send_music(Ind, 30, 11); return; //Cirith Ungol
		case 10: Send_music(Ind, 28, 11); return; //The Heart of the Earth
		case 16: Send_music(Ind, 18, 11); return; //The Paths of the Dead
		case 17: Send_music(Ind, 37, 11); return; //The Illusory Castle
		case 18: Send_music(Ind, 39, 11); return; //The Maze
		case 19: Send_music(Ind, 20, 11); return; //The Orc Cave
		case 20: Send_music(Ind, 36, 11); return; //Erebor
		case 21: Send_music(Ind, 27, 11); return; //The Old Forest
		case 22: Send_music(Ind, 29, 11); return; //The Mines of Moria
		case 23: Send_music(Ind, 34, 11); return; //Dol Guldur
		case 24: Send_music(Ind, 31, 11); return; //The Small Water Cave
		case 25: Send_music(Ind, 38, 11); return; //The Sacred Land of Mountains
		case 26: Send_music(Ind, 24, 11); return; //The Land of Rhun
		case 27: Send_music(Ind, 25, 11); return; //The Sandworm Lair
		case 28: Send_music(Ind, 33, 11); return; //Death Fate
		case 29: Send_music(Ind, 23, 11); return; //The Helcaraxe
		case 30: Send_music(Ind, 15, 12); return; //The Training Tower
		//31 is handled above by in_valinor() check
		case 32:
			if (is_newer_than(&p_ptr->version, 4, 5, 6, 0, 0, 1)) Send_music(Ind, 56, 13); //The Cloud Planes
			else {
				if (p_ptr->audio_mus == 56) Send_music(Ind, 13, 0); /* outdated music pack? (use ironman music for now (forcedown/hellish doesn't fit)) */
				else Send_music(Ind, 56, 13); /* the actual specific music for this dungeon */
			}
			return;
		}
	}

	/* Shouldn't happen - send default (dungeon) music */
	Send_music(Ind, 0, 0);
}
void handle_seasonal_music(void) {
	player_type *p_ptr;
	int i;

	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		/* Currently all seasonal events take place in Bree */
		if (p_ptr->wpos.wz == 0 &&
		    istownarea(&p_ptr->wpos, MAX_TOWNAREA) &&
		    town[wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].town_idx].type == TOWN_BREE)
			handle_music(i);
	}
}

void handle_ambient_sfx(int Ind, cave_type *c_ptr, struct worldpos *wpos, bool smooth) {
	player_type *p_ptr = Players[Ind];
	dun_level *l_ptr = getfloor(wpos);

	/* sounds that guaranteedly override everthing */
	if (in_valinor(wpos)) {
		Send_sfx_ambient(Ind, SFX_AMBIENT_SHORE, TRUE);
		return;
	}

	/* don't play outdoor (or any other) ambient sfx if we're in a special pseudo-indoors sector */
	if ((l_ptr && (l_ptr->flags2 & LF2_INDOORS)) ||
	    (in_sector00(wpos) && (sector00flags2 & LF2_INDOORS))) {
		Send_sfx_ambient(Ind, SFX_AMBIENT_NONE, FALSE);
		return;
	}

	/* disable certain ambient sounds if they shouldn't be up here */
	if (p_ptr->sound_ambient == SFX_AMBIENT_FIREPLACE && !inside_inn(p_ptr, c_ptr)) {
		Send_sfx_ambient(Ind, SFX_AMBIENT_NONE, smooth);
	} else if (p_ptr->sound_ambient == SFX_AMBIENT_SHORE && (wpos->wz != 0 || (wild_info[wpos->wy][wpos->wx].type != WILD_OCEAN && wild_info[wpos->wy][wpos->wx].bled != WILD_OCEAN))) {
		Send_sfx_ambient(Ind, SFX_AMBIENT_NONE, smooth);
	} else if (p_ptr->sound_ambient == SFX_AMBIENT_LAKE && (wpos->wz != 0 ||
	    (wild_info[wpos->wy][wpos->wx].type != WILD_LAKE && wild_info[wpos->wy][wpos->wx].bled != WILD_LAKE &&
	    wild_info[wpos->wy][wpos->wx].type != WILD_RIVER && wild_info[wpos->wy][wpos->wx].bled != WILD_RIVER &&
	    wild_info[wpos->wy][wpos->wx].type != WILD_SWAMP && wild_info[wpos->wy][wpos->wx].bled != WILD_SWAMP))) {
		Send_sfx_ambient(Ind, SFX_AMBIENT_NONE, smooth);
	}

	/* enable/switch to certain ambient loops */
#if 0 /* buggy: enable no_house_sfx while inside a house, and the sfx will stay looping even when leaving the house */
	if (p_ptr->sound_ambient != SFX_AMBIENT_FIREPLACE && (f_info[c_ptr->feat].flags1 & FF1_PROTECTED) && istown(wpos) && p_ptr->sfx_house) { /* sfx_house check is redundant with grid_affects_player() */
#elif 0 /* still buggy :-p */
	if (p_ptr->sound_ambient != SFX_AMBIENT_FIREPLACE && (f_info[c_ptr->feat].flags1 & FF1_PROTECTED) && istown(wpos)) {
#else /* dunno */
	if (p_ptr->sound_ambient != SFX_AMBIENT_FIREPLACE && inside_inn(p_ptr, c_ptr)) {
#endif
		Send_sfx_ambient(Ind, SFX_AMBIENT_FIREPLACE, smooth);
	} else if (p_ptr->sound_ambient != SFX_AMBIENT_FIREPLACE &&
	    p_ptr->sound_ambient != SFX_AMBIENT_SHORE && wpos->wz == 0 && (wild_info[wpos->wy][wpos->wx].type == WILD_OCEAN || wild_info[wpos->wy][wpos->wx].bled == WILD_OCEAN)) {
		Send_sfx_ambient(Ind, SFX_AMBIENT_SHORE, smooth);
	} else if (p_ptr->sound_ambient != SFX_AMBIENT_FIREPLACE && p_ptr->sound_ambient != SFX_AMBIENT_SHORE && 
	    p_ptr->sound_ambient != SFX_AMBIENT_LAKE && wpos->wz == 0 &&
	    (wild_info[wpos->wy][wpos->wx].type == WILD_LAKE || wild_info[wpos->wy][wpos->wx].bled == WILD_LAKE ||
	    wild_info[wpos->wy][wpos->wx].type == WILD_RIVER || wild_info[wpos->wy][wpos->wx].bled == WILD_RIVER ||
	    wild_info[wpos->wy][wpos->wx].type == WILD_SWAMP || wild_info[wpos->wy][wpos->wx].bled == WILD_SWAMP)) {
		Send_sfx_ambient(Ind, SFX_AMBIENT_LAKE, smooth);
	}
}

/* play single ambient sfx, synched for all players, depending on worldmap terrain - C. Blue */
void process_ambient_sfx(void) {
	int i, vol;
	player_type *p_ptr;
	wilderness_type *w_ptr;

	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];
		if (p_ptr->conn == NOT_CONNECTED) continue;
		if (p_ptr->wpos.wz) continue;

		w_ptr = &wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx];
		if (w_ptr->ambient_sfx_counteddown) continue;
		if (w_ptr->ambient_sfx) {
			p_ptr->ambient_sfx_timer = 0;
			continue;
		}
		if (w_ptr->ambient_sfx_timer) {
			if (!w_ptr->ambient_sfx_counteddown) {
				w_ptr->ambient_sfx_timer--;
				w_ptr->ambient_sfx_counteddown = TRUE; //semaphore
			}
			continue;
		}

		vol = 25 + rand_int(75);
		//if (!rand_int(6)) vol = 100;

		switch (w_ptr->type) { /* ---- ensure consistency with alloc_dungeon_level() ---- */
		case WILD_RIVER:
		case WILD_LAKE:
		case WILD_SWAMP:
			if (season == SEASON_WINTER) break;
			sound_floor_vol(&p_ptr->wpos, "animal_toad", NULL, SFX_TYPE_AMBIENT, vol);
			w_ptr->ambient_sfx_timer = 4 + rand_int(4);
			break;
		case WILD_ICE:
		case WILD_MOUNTAIN:
		case WILD_WASTELAND:
			if (IS_NIGHT) sound_floor_vol(&p_ptr->wpos, "animal_wolf", NULL, SFX_TYPE_AMBIENT, vol);
			w_ptr->ambient_sfx_timer = 30 + rand_int(60);
			break;
		//case WILD_SHORE:
		case WILD_OCEAN:
			if (IS_DAY) sound_floor_vol(&p_ptr->wpos, "animal_seagull", NULL, SFX_TYPE_AMBIENT, vol);
			w_ptr->ambient_sfx_timer = 30 + rand_int(60);
			break;
		case WILD_FOREST:
		case WILD_DENSEFOREST:
			if (IS_DAY) {
				if (season == SEASON_WINTER) {
					sound_floor_vol(&p_ptr->wpos, "animal_wolf", NULL, SFX_TYPE_AMBIENT, vol);
					w_ptr->ambient_sfx_timer = 30 + rand_int(60);
				} else {
					sound_floor_vol(&p_ptr->wpos, "animal_bird", NULL, SFX_TYPE_AMBIENT, vol);
					w_ptr->ambient_sfx_timer = 10 + rand_int(20);
				}
			} else {
				if (!rand_int(3)) {
					sound_floor_vol(&p_ptr->wpos, "animal_wolf", NULL, SFX_TYPE_AMBIENT, vol);
					w_ptr->ambient_sfx_timer = 30 + rand_int(60);
				} else {
					sound_floor_vol(&p_ptr->wpos, "animal_owl", NULL, SFX_TYPE_AMBIENT, vol);
					w_ptr->ambient_sfx_timer = 20 + rand_int(40);
				}
			}
			break;
		/* Note: Default terrain that doesn't have ambient sfx will automatically clear people's ambient_sfx_timer too.
		   This is ok for towns but not cool for fast wilderness travel where terrain is mixed a lot.
		   For that reason we give default terrain a "pseudo-timeout" to compromise a bit. */
		default:
			w_ptr->ambient_sfx_timer = 30 + rand_int(5); //should be > than time required for travelling across 1 wilderness sector
			break;
		}

		w_ptr->ambient_sfx = TRUE;
		p_ptr->ambient_sfx_timer = 0;
	}

	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (Players[i]->wpos.wz) continue;
		wild_info[Players[i]->wpos.wy][Players[i]->wpos.wx].ambient_sfx = FALSE;
		wild_info[Players[i]->wpos.wy][Players[i]->wpos.wx].ambient_sfx_counteddown = FALSE;
	}
}

/* generate an item-type specific sound, depending on the action applied to it
   action: 0 = pickup, 1 = drop, 2 = wear/wield, 3 = takeoff, 4 = throw, 5 = destroy */
void sound_item(int Ind, int tval, int sval, cptr action) {
	char sound_name[30];
	cptr item = NULL;

	/* action hack: re-use sounds! */
	action = "item_";

	/* choose sound */
	if (is_melee_weapon(tval)) switch(tval) {
		case TV_SWORD: item = "sword"; break;
		case TV_BLUNT:
			if (sval == SV_WHIP) item = "whip"; else item = "blunt";
			break;
		case TV_AXE: item = "axe"; break;
		case TV_POLEARM: item = "polearm"; break;
	} else if (is_armour(tval)) {
		if (is_textile_armour(tval, sval))
			item = "armour_light";
		else
			item = "armour_heavy";
	} else switch(tval) {
		/* equippable stuff */
		case TV_LITE: item = "lightsource"; break;
		case TV_RING: item = "ring"; break;
		case TV_AMULET: item = "amulet"; break;
		case TV_TOOL: item = "tool"; break;
		case TV_DIGGING: item = "tool_digger"; break;
		case TV_MSTAFF: item = "magestaff"; break;
		case TV_BOOMERANG: item = "boomerang"; break;
		case TV_BOW: item = "bow"; break;
		case TV_SHOT: item = "shot"; break;
		case TV_ARROW: item = "arrow"; break;
		case TV_BOLT: item = "bolt"; break;
		/* other items */
		case TV_BOOK: item = "book"; break;
		case TV_SCROLL: case TV_PARCHMENT:
			item = "scroll"; break;
		case TV_BOTTLE: item = "potion"; break;
		case TV_POTION: case TV_POTION2: case TV_FLASK:
			item = "potion"; break;
		case TV_RUNE: item = "rune"; break;
		case TV_SKELETON: item = "skeleton"; break;
		case TV_FIRESTONE: item = "firestone"; break;
		case TV_SPIKE: item = "spike"; break;
		case TV_CHEST: item = "chest"; break;
		case TV_JUNK: item = "junk"; break;
		case TV_GAME:
			if (sval == SV_GAME_BALL) {
				item = "ball_pass"; break; }
			else if (sval <= SV_BLACK_PIECE) {
				item = "go_stone"; break; }
			item = "game_piece"; break;
		case TV_TRAPKIT: item = "trapkit"; break;
		case TV_STAFF: item = "staff"; break;
		case TV_WAND: item = "wand"; break;
		case TV_ROD: item = "rod"; break;
		case TV_ROD_MAIN: item = "rod"; break;
		case TV_FOOD: item = "food"; break;
		case TV_KEY: item = "key"; break;
		case TV_GOLEM:
			if (sval == SV_GOLEM_WOOD) {
				item = "golem_wood"; break; }
			else if (sval <= SV_GOLEM_ADAM) {
				item = "golem_metal"; break; }
			else if (sval >= SV_GOLEM_ATTACK) {
				item = "scroll"; break; }
			item = "golem_misc"; break;
		case TV_SPECIAL:
			switch (sval) {
			case SV_SEAL: item = "seal"; break;
			}
			break;
	}

	/* no sound effect available? */
	if (item == NULL) return;

	/* build sound name from action and item */
	strcpy(sound_name, action);
	strcat(sound_name, item);
	/* off we go */
	sound(Ind, sound_name, NULL, SFX_TYPE_COMMAND, FALSE);
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
	const char *ax = quark_str(quark);

	if (ax == NULL) return FALSE;

	while ((ax = strchr(ax, '!')) != NULL) {
		while (ax++ != NULL) {
			if (*ax == 0)  {
				return FALSE; /* end of quark, stop */
			}
			if (*ax == ' ' || *ax == '@' || *ax == '#' || *ax == '-') {
				break; /* end of segment, stop */
			}
			if (*ax == what) {
				return TRUE; /* exact match, stop */
			}
			if (*ax == '*') {
				/* why so much hassle? * = all, that's it */
/*				return TRUE; -- well, !'B'ash if it's on the ground sucks ;) */

				switch (what) { /* check for paranoid tags */
				case 'd': /* no drop */
				case 'h': /* (obsolete) no house ( sell a a key ) */
				case 'k': /* no destroy */
				case 's': /* no sell */
				case 'v': /* no thowing */
				case '=': /* force pickup */
#if 0
				case 'w': /* no wear/wield */
				case 't': /* no take off */
#endif
					return TRUE;
				}
				//return FALSE;
			}
			if (*ax == '+') {
				/* why so much hassle? * = all, that's it */
/*				return TRUE; -- well, !'B'ash if it's on the ground sucks ;) */

				switch (what) { /* check for paranoid tags */
				case 'h': /* (obsolete) no house ( sell a a key ) */
				case 'k': /* no destroy */
				case 's': /* no sell */
				case 'v': /* no thowing */
				case '=': /* force pickup */
#if 0
				case 'w': /* no wear/wield */
				case 't': /* no take off */
#endif
					return TRUE;
				}
				//return FALSE;
			}
		}
	}
	return FALSE;
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
bool suppress_message = FALSE, censor_message = FALSE, suppress_boni = FALSE;
int censor_length = 0, censor_punish = 0;

void msg_print(int Ind, cptr msg_raw) {
	char msg_dup[MSG_LEN], *msg = msg_dup;
	int line_len = 80; /* maximum length of a text line to be displayed;
		     this is client-dependant, compare c_msg_print (c-util.c) */
	char msg_buf[line_len + 2 + 2 * 80]; /* buffer for 1 line. + 2 bytes for colour code (+2*80 bytes for colour codeeeezz) */
	char msg_minibuf[3]; /* temp buffer for adding characters */
	int text_len, msg_scan = 0, space_scan, tab_spacer = 0, tmp;
	char colour_code = 'w', prev_colour_code = 'w';
	bool no_colour_code = FALSE;
	bool first_character = TRUE;
	//bool is_chat = ((msg_raw != NULL) && (strlen(msg_raw) > 2) && (msg_raw[2] == '['));
	bool client_ctrlo = FALSE, client_chat = FALSE, client_all = FALSE;

	/* for {- feature */
	char first_colour_code = 'w';
	bool first_colour_code_set = FALSE;

	/* backward msg window width hack for windows clients (non-x11 clients rather) */
	if (!is_newer_than(&Players[Ind]->version, 4, 4, 5, 3, 0, 0) && !strcmp(Players[Ind]->realname, "PLAYER")) line_len = 72;

	/* Pfft, sorry to bother you.... --JIR-- */
	if (suppress_message) return;

	/* no message? */
	if (msg_raw == NULL) return;

	strcpy(msg_dup, msg_raw); /* in case msg_raw was constant */
	/* censor swear words? */
	if (censor_message) {
		/* skip the name of the sender, etc. */
		censor_punish = handle_censor(msg + strlen(msg) - censor_length);
		/* we just needed to get censor_punish at least once, above.
		   Now don't really censor the string if the player doesn't want to.. */
		if (!Players[Ind]->censor_swearing) strcpy(msg_dup, msg_raw);
	}

	/* marker for client: add message to 'chat-only buffer', not to 'nochat buffer' */
	if (msg[0] == '\375') {
		client_chat = TRUE;
		msg++;
		/* hack: imply that chat-only messages are also always important messages */
		client_ctrlo = TRUE;
	}
	/* marker for client: add message to 'nochat buffer' AND to 'chat-only buffer' */
	else if (msg[0] == '\374') {
		client_all = TRUE;
		msg++;
		/* hack: imply that messages that go to chat-buffer are also always important messages */
		client_ctrlo = TRUE;
	}
	/* ADDITIONAL marker for client: add message to CTRL+O 'important scrollback buffer' */
	if (msg[0] == '\376') {
		client_ctrlo = TRUE;
		msg++;
	}
	/* note: neither \375 nor \374 means:
	   add message to 'nochat buffer', but not to 'chat-only buffer' (default) */

	/* neutralize markers if client version too old */
	if (!is_newer_than(&Players[Ind]->version, 4, 4, 2, 0, 0, 0))
		client_ctrlo = client_chat = client_all = FALSE;

#if 1	/* String longer than 1 line? -> Split it up! --C. Blue-- */
	while (msg != NULL && msg[msg_scan] != '\0') {
		/* Start a new line */
		strcpy(msg_buf, "");
		text_len = 0;

		/* Tabbing the line? */
		msg_minibuf[0] = ' ';
		msg_minibuf[1] = '\0';
#if 0 /* 0'ed to remove backward compatibility via '~' character. We want to switch to \374..6 codes exlusively */
		if (is_chat && tab_spacer) {
			/* Start the padding for chat messages with '~' */
			strcat(msg_buf, "~");
			tmp = tab_spacer - 1;
		} else {
			tmp = tab_spacer;
		}
#else
		tmp = tab_spacer;
#endif
		while (tmp--) {
			text_len++;
			strcat(msg_buf, msg_minibuf);
		}

		/* Prefixing colour code? */
		if (colour_code) {
			msg_minibuf[0] = '\377';
			msg_minibuf[1] = colour_code;
			msg_minibuf[2] = '\0';
			strcat(msg_buf, msg_minibuf);
			///colour_code = 0;
		}

		/* Process the string... */
		while (text_len < line_len) {
			switch (msg[msg_scan]) {
			case '\0': /* String ends! */
				text_len = line_len;
				continue;
			case '\377': /* Colour code! Text length does not increase. */
				if (!no_colour_code) {
					/* broken \377 at the end of the text? ignore */
					if (color_char_to_attr(msg[msg_scan + 1]) == -1
					    && msg[msg_scan + 1] != '-' /* {-, {. and {{ are handled (further) below */
					    && msg[msg_scan + 1] != '.'
					    && msg[msg_scan + 1] != '\377') {
						msg_scan++;
						continue;
					}

					/* Capture double \377 which stand for a normal { char instead of a colour code: */
					if (msg[msg_scan + 1] != '\377') {
						msg_minibuf[0] = msg[msg_scan];
						msg_scan++;

						/* needed for rune sigil on items, pasted to chat */
						if (msg[msg_scan] == '.') {
							msg[msg_scan] = colour_code = prev_colour_code;
						/* needed for new '\377-' feature in multi-line messages: resolve it to actual colour */
						} else if (msg[msg_scan] == '-') {
							msg[msg_scan] = colour_code = first_colour_code;
						} else {
							prev_colour_code = colour_code;
							colour_code = msg[msg_scan];
							if (!first_colour_code_set) {
								first_colour_code_set = TRUE;
								first_colour_code = colour_code;
							}
						}
						msg_scan++;

						msg_minibuf[1] = colour_code;
						msg_minibuf[2] = '\0';
						strcat(msg_buf, msg_minibuf);
						break;
					}
					no_colour_code = TRUE;
				} else no_colour_code = FALSE;
				/* fall through if it's a '{' character */
			default: /* Text length increases by another character.. */
				/* Depending on message type, remember to tab the following
				   lines accordingly to make it look better ^^
				   depending on the first character of this line. */
				if (first_character) {
					switch (msg[msg_scan]) {
					case '*': tab_spacer = 2; break; /* Kill message */
					case '[': /* Chat message */
#if 0
						tab_spacer = 1;
#else
						{
							const char *bracket = strchr(&msg[msg_scan], ']');

							if (bracket) {
								const char *ptr = &msg[msg_scan];

								/* Pad lines according to how long the name is - mikaelh */
								tab_spacer = bracket - &msg[msg_scan] + 2;

								/* Ignore space reserved for colour codes:
								   Guild chat has coloured [ ] brackets. - C. Blue */
								while ((ptr = strchr(ptr, '\377')) && ptr < bracket) {
									ptr++;
									tab_spacer -= 2; /* colour code consists of two chars
									    (we can guarantee that here, since the server
									    generated this colour code) */
								}

								/* Hack: multiline /me emotes */
								if (tab_spacer > 70) tab_spacer = 1;

								/* Paranoia */
								if (tab_spacer < 1) tab_spacer = 1;
								if (tab_spacer > 30) tab_spacer = 30;
							} else {
								/* No ']' */
								tab_spacer = 1;
							}
						}
#endif
						break;
					default: tab_spacer = 1;
					}
				}

				/* remember first actual chat colour (guild chat changes
				   colour a few times for [..] brackets and name).
				   This is for {- etc feature.
				   However, if there is no new colour specified before
				   beginning of chat text then use the one we had, or {-
				   wouldn't work correctly in private chat anymore. - C. Blue */
				if (msg[msg_scan] == ']') {
#if 0  /* this is wrong, because the colour code COULD already be from \\a feature! */
				    ((msg[msg_scan + 1] == ' ' && msg[msg_scan + 2] == '\377') ||
#endif
					if (msg[msg_scan + 1] == '\377') first_colour_code_set = FALSE;
				}

				/* Process text.. */
				first_character = FALSE;
				msg_minibuf[0] = msg[msg_scan];
				msg_minibuf[1] = '\0';
				strcat(msg_buf, msg_minibuf);
				msg_scan++;
				text_len++;

				/* Avoid cutting words in two */
				if ((text_len == line_len) && (msg[msg_scan] != '\0')
				    && (

				    (msg[msg_scan - 1] >= 'A' && msg[msg_scan - 1] <= 'Z') ||
				    (msg[msg_scan - 1] >= 'a' && msg[msg_scan - 1] <= 'z') ||
				    (msg[msg_scan - 1] >= '0' && msg[msg_scan - 1] <= '9') ||
				    msg[msg_scan - 1] == '_' || /* for pasting lore-flags to chat! */
				    msg[msg_scan - 1] == '(' ||
				    msg[msg_scan - 1] == '[' ||
				    msg[msg_scan - 1] == '{' ||
#if 1 /* don't break smileys? */
				    msg[msg_scan - 1] == ':' || msg[msg_scan - 1] == ';' ||
				    msg[msg_scan - 1] == '^' || msg[msg_scan - 1] == '-' ||
#endif
#if 1 /* don't break quotes right at the start */
				    msg[msg_scan - 1] == '"' || msg[msg_scan - 1] == '\'' ||
#endif
#if 0
				    msg[msg_scan - 1] == ')' ||
				    msg[msg_scan - 1] == ']' ||
				    msg[msg_scan - 1] == '}' ||
#endif
				    /* (maybe too much) for pasting items to chat, (+1) or (-2,0) : */
				    msg[msg_scan - 1] == '+' || msg[msg_scan - 1] == '-' ||
				    /* Don't break colour codes */
				    msg[msg_scan - 1] == '\377'

				    ) && (

				    (msg[msg_scan] >= 'A' && msg[msg_scan] <= 'Z') ||
				    (msg[msg_scan] >= 'a' && msg[msg_scan] <= 'z') ||
				    (msg[msg_scan] >= '0' && msg[msg_scan] <= '9') ||
#if 1 /* specialty: don't break quotes */
				    //(msg[msg_scan] != ' ' && msg[msg_scan - 1] == '"') ||
				    msg[msg_scan - 1] == '"' || msg[msg_scan - 1] == '\'' ||
#endif
#if 1 /* don't break moar smileys? */
				    msg[msg_scan] == '(' ||
				    msg[msg_scan] == '[' ||
				    msg[msg_scan] == '{' ||
#endif
#if 1 /* don't break smileys? */
				    msg[msg_scan] == '-' ||
				    msg[msg_scan] == '^' ||
#endif
				    msg[msg_scan] == '_' ||/* for pasting lore-flags to chat! */
				    msg[msg_scan] == ')' ||
				    msg[msg_scan] == ']' ||
				    msg[msg_scan] == '}' ||
				    /* (maybe too much) for pasting items to chat, (+1) or (-2,0) : */
				    msg[msg_scan] == '+' || msg[msg_scan] == '-' ||
				    /* interpunction at the end of a sentence */
				    msg[msg_scan] == '.' || msg[msg_scan] == ',' ||
				    msg[msg_scan] == ';' || msg[msg_scan] == ':' ||
				    msg[msg_scan] == '!' || msg[msg_scan] == '?' ||
				    /* Don't break colour codes */
				    msg[msg_scan] == '\377'

				    )) {
					space_scan = msg_scan;
					do {
						space_scan--;
					} while (((msg[space_scan - 1] >= 'A' && msg[space_scan - 1] <= 'Z') ||
						(msg[space_scan - 1] >= 'a' && msg[space_scan - 1] <= 'z') ||
						(msg[space_scan - 1] >= '0' && msg[space_scan - 1] <= '9') ||
#if 1 /* don't break smileys? */
						msg[space_scan - 1] == ':' || msg[space_scan - 1] == ';' ||
#endif
#if 0
						msg[space_scan - 1] == ')' ||
						msg[space_scan - 1] == ']' ||
						msg[space_scan - 1] == '}' ||
#endif
#if 0
						msg[space_scan - 1] == '_' ||/* for pasting lore-flags to chat! */
#endif
						msg[space_scan - 1] == '(' ||
						msg[space_scan - 1] == '[' ||
						msg[space_scan - 1] == '{' ||
						/* (maybe too much) for pasting items to chat, (+1) or (-2,0) : */
						msg[space_scan - 1] == '+' || msg[space_scan - 1] == '-' ||
						/* pasting flags to chat ("SLAY_EVIL") */
						msg[space_scan - 1] == '_' ||
						msg[space_scan - 1] == '\377'
						) && space_scan > 0);

					/* Simply cut words that are very long - mikaelh */
					if (msg_scan - space_scan > 36) {
						space_scan = msg_scan;
					}

					if (space_scan) {
						msg_buf[strlen(msg_buf) - msg_scan + space_scan] = '\0';
						msg_scan = space_scan;
					}
				}
			}
		}
		Send_message(Ind, format("%s%s%s",
		    client_chat ? "\375" : (client_all ? "\374" : ""),
		    client_ctrlo ? "\376" : "",
		    msg_buf));
		/* hack: avoid trailing space in the next sub-line */
		//if (msg[msg_scan] == ' ') msg_scan++;
		while (msg[msg_scan] == ' ') msg_scan++;//avoid all trailing spaces in the next sub-line
	}

	if (msg == NULL) Send_message(Ind, msg);

	return;
#endif // enable line breaks?

	Send_message(Ind, format("%s%s%s",
	    client_chat ? "\375" : (client_all ? "\374" : ""),
	    client_ctrlo ? "\376" : "",
	    msg));
}

void msg_broadcast(int Ind, cptr msg) {
	int i;

	/* Tell every player */
	for (i = 1; i <= NumPlayers; i++) {
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

void msg_admins(int Ind, cptr msg) {
	int i;

	/* Tell every player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Skip disconnected players */
		if (Players[i]->conn == NOT_CONNECTED) 
			continue;

		/* Skip the specified player */
		if (i == Ind)
			continue;

		/* Skip non-admins */
		if (!is_admin(Players[i]))
			continue;

		/* Tell this one */
		msg_print(i, msg);
	 }
}

void msg_broadcast_format(int Ind, cptr fmt, ...) {
	//int i;
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
	for (i = 1; i <= NumPlayers; i++) {
		p_ptr = Players[i];

		/* Skip disconnected players */
		if (p_ptr->conn == NOT_CONNECTED) 
			continue;
			

		/* Tell Mama */
		if (is_admin(p_ptr))
			msg_print(i, buf);
	 }
}



/*
 * Display a formatted message, using "vstrnfmt()" and "msg_print()".
 */
void msg_format(int Ind, cptr fmt, ...) {
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
 * Send a message to everyone on a floor.
 */
static void floor_msg(struct worldpos *wpos, cptr msg) {
	int i;

//system-msg, currently unused anyway-	if(cfg.log_u) s_printf("[%s] %s\n", Players[sender]->name, msg);
	/* Check for this guy */
	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		/* Check this guy */
		if (inarea(wpos, &Players[i]->wpos)) msg_print(i, msg);
	}
}
/*
 * Send a formatted message to everyone on a floor. (currently unused)
 */
void floor_msg_format(struct worldpos *wpos, cptr fmt, ...) {
	va_list vp;
	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);
	/* Format the args, save the length */
	(void)vstrnfmt(buf, 1024, fmt, vp);
	/* End the Varargs Stuff */
	va_end(vp);
	/* Display */
	floor_msg(wpos, buf);
}
/*
 * Send a message to everyone on a floor, considering ignorance.
 */
static void floor_msg_ignoring(int sender, struct worldpos *wpos, cptr msg) {
	int i;

	if(cfg.log_u) s_printf("(%d,%d,%d)%s\n", wpos->wx, wpos->wy, wpos->wz, msg + 2);// Players[sender]->name, msg);
	/* Check for this guy */
	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (check_ignore(i, sender)) continue;
		/* Check this guy */
		if (inarea(wpos, &Players[i]->wpos)) msg_print(i, msg);
	}
}
/*
 * Send a formatted message to everyone on a floor, considering ignorance.
 */
static void floor_msg_format_ignoring(int sender, struct worldpos *wpos, cptr fmt, ...) {
	va_list vp;
	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);
	/* Format the args, save the length */
	(void)vstrnfmt(buf, 1024, fmt, vp);
	/* End the Varargs Stuff */
	va_end(vp);
	/* Display */
	floor_msg_ignoring(sender, wpos, buf);
}

/*
 * Send a message to everyone on the world surface. (for season change)
 */
void world_surface_msg(cptr msg) {
	int i;

//system-msg, currently unused anyway-	if(cfg.log_u) s_printf("[%s] %s\n", Players[sender]->name, msg);
	/* Check for this guy */
	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		/* Check this guy */
		if (Players[i]->wpos.wz == 0) msg_print(i, msg);
	}
}


/*
 * Display a message to everyone who is in sight on another player.
 *
 * This is mainly used to keep other players advised of actions done
 * by a player.  The message is not sent to the player who performed
 * the action.
 */
void msg_print_near(int Ind, cptr msg) {
	player_type *p_ptr = Players[Ind];
	int y, x, i;
	struct worldpos *wpos;

	wpos = &p_ptr->wpos;
	if (p_ptr->admin_dm) return;

	y = p_ptr->py;
	x = p_ptr->px;

	/* Check each player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this player */
		p_ptr = Players[i];

		/* Make sure this player is in the game */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Don't send the message to the player who caused it */
		if (Ind == i) continue;

		/* Make sure this player is at this depth */
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Can he see this player? */
		if (p_ptr->cave_flag[y][x] & CAVE_VIEW) {
			/* Send the message */
			msg_print(i, msg);
		}
	}
}

/* Whispering: Send message to adjacent players */
void msg_print_verynear(int Ind, cptr msg) {
	player_type *p_ptr = Players[Ind];
	int y, x, i;
	struct worldpos *wpos;
	wpos = &p_ptr->wpos;

	//if(p_ptr->admin_dm) return;

	y = p_ptr->py;
	x = p_ptr->px;

	/* Check each player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this player */
		p_ptr = Players[i];

		/* Make sure this player is in the game */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Don't send the message to the player who caused it */
		if (Ind == i) continue;

		/* Make sure this player is at this depth */
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Is he in range? */
		if (abs(p_ptr->py - y) <= 1 && abs(p_ptr->px - x) <= 1) {
			/* Send the message */
			msg_print(i, msg);
		}
	}
}

/* Take into account player hallucination.
   In theory, msg_print_near_monster() could be used, but that one doesn't support
   starting on colour codes, and this function is used in all places where messages
   ought to follow the game's message colour scheme.. this all seems not too efficient :/.
   m_idx == -1: assume seen. */
void msg_print_near_monvar(int Ind, int m_idx, cptr msg, cptr msg_garbled, cptr msg_unseen) {
	player_type *p_ptr = Players[Ind];
	int y, x, i;
	struct worldpos *wpos;
	wpos = &p_ptr->wpos;

	if (p_ptr->admin_dm) return;

	y = p_ptr->py;
	x = p_ptr->px;

	/* Check each player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this player */
		p_ptr = Players[i];

		/* Make sure this player is in the game */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Don't send the message to the player who caused it */
		if (Ind == i) continue;

		/* Make sure this player is at this depth */
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Can he see this player? */
		if (p_ptr->cave_flag[y][x] & CAVE_VIEW) {
			/* Send the message */
			if (m_idx != -1 && !p_ptr->mon_vis[m_idx]) msg_print(i, msg_unseen);
			else if (p_ptr->image) msg_print(i, msg_garbled);
			else msg_print(i, msg);
		}
	}
}

/*
 * Same as above, except send a formatted message.
 */
void msg_format_near(int Ind, cptr fmt, ...) {
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

/* for whispering */
void msg_format_verynear(int Ind, cptr fmt, ...) {
	va_list vp;
	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Format the args, save the length */
	(void)vstrnfmt(buf, 1024, fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Display */
	msg_print_verynear(Ind, buf);
}

/* location-based - also, skip player Ind if non-zero */
void msg_print_near_site(int y, int x, worldpos *wpos, int Ind, bool view, cptr msg) {
	int i, d;
	player_type *p_ptr;

	/* Check each player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this player */
		p_ptr = Players[i];

		/* Make sure this player is in the game */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Skip specified player, if any */
		if (i == Ind) continue;

		/* Make sure this player is at this depth */
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Can (s)he see the site? */
		if (view) {
			if (!(p_ptr->cave_flag[y][x] & CAVE_VIEW)) continue;
		} else { /* can (s)he hear it? */
			/* within audible range? */
			d = distance(y, x, Players[i]->py, Players[i]->px);
			/* NOTE: should be consistent with sound_near_site() */
			if (d > MAX_SIGHT) continue;
		}

		/* Send the message */
		msg_print(i, msg);
	}
}

/*
 * Same as above, except send a formatted message.
 */
void msg_format_near_site(int y, int x, worldpos *wpos, int Ind, bool view, cptr fmt, ...) {
	va_list vp;
	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Format the args, save the length */
	(void)vstrnfmt(buf, 1024, fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Display */
	msg_print_near_site(y, x, wpos, Ind, view, buf);
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
void msg_print_near_monster(int m_idx, cptr msg) {
	int i;
	player_type *p_ptr;
	cave_type **zcave;
	char m_name[MNAME_LEN];

	monster_type	*m_ptr = &m_list[m_idx];
	worldpos *wpos = &m_ptr->wpos;

	if (!(zcave = getcave(wpos))) return;


	/* Check each player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Check this player */
		p_ptr = Players[i];

		/* Make sure this player is in the game */
		if (p_ptr->conn == NOT_CONNECTED) continue;

		/* Make sure this player is at this depth */
		if (!inarea(&p_ptr->wpos, wpos)) continue;

		/* Skip if not visible */
		if (!p_ptr->mon_vis[m_idx]) continue;

		/* Can he see this monster? */
		if (!(p_ptr->cave_flag[m_ptr->fy][m_ptr->fx] & CAVE_VIEW)) continue;

		/* Acquire the monster name */
		monster_desc(i, m_name, m_idx, 0);

		msg_format(i, "%^s %s", m_name, msg);
	}
}

/* send a message to all online party members */
void msg_party_format(int Ind, cptr fmt, ...) {
	int i;
	char buf[1024];
	va_list vp;

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);
	/* Format the args, save the length */
	(void)vstrnfmt(buf, 1024, fmt, vp);
	/* End the Varargs Stuff */
	va_end(vp);

	/* Tell every player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Skip disconnected players */
		if (Players[i]->conn == NOT_CONNECTED) 
			continue;

#if 0
		/* Skip the specified player */
		if (i == Ind)
			continue;
#endif

		/* skip players not in his party */
		if (Players[i]->party != Players[Ind]->party)
			continue;

		/* Tell this one */
		msg_print(i, buf);
	 }
}

/* send a message to all online guild members */
void msg_guild_format(int Ind, cptr fmt, ...) {
	int i;
	char buf[1024];
	va_list vp;

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);
	/* Format the args, save the length */
	(void)vstrnfmt(buf, 1024, fmt, vp);
	/* End the Varargs Stuff */
	va_end(vp);

	/* Tell every player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Skip disconnected players */
		if (Players[i]->conn == NOT_CONNECTED) 
			continue;

#if 0
		/* Skip the specified player */
		if (i == Ind)
			continue;
#endif

		/* skip players not in his guild */
		if (Players[i]->guild != Players[Ind]->guild)
			continue;

		/* Tell this one */
		msg_print(i, buf);
	 }
}

#if 0 /* unused atm */
static char* dodge_diz(int chance) {
	if (chance < 5)
		return "almost no";
	else if (chance < 14)
		return "a slight";
	else if (chance < 23)
		return "a significant";
	else if (chance < 30)
		return "a good";
	else if (chance < 40)
		return "a very good";
	else
		return "a high";
}
#endif

/*
 * Dodge Chance Feedback.
 */
void use_ability_blade(int Ind) {
	player_type *p_ptr = Players[Ind];
#ifndef NEW_DODGING
	int dun_level = getlevel(&p_ptr->wpos);
	int chance = p_ptr->dodge_level - (dun_level * 5 / 6);

	if (chance < 0) chance = 0;
	if (chance > DODGE_CAP) chance = DODGE_CAP;	// see DODGE_CAP in melee1.c
	//if (is_admin(p_ptr))
		msg_format(Ind, "You have a %d%% chance of dodging a level %d monster.", chance, dun_level);
 #if 0
	if (chance < 5)
		msg_format(Ind, "You have almost no chance of dodging a level %d monster.", dun_level);
	else if (chance < 10)
		msg_format(Ind, "You have a slight chance of dodging a level %d monster.", dun_level);
	else if (chance < 20)
		msg_format(Ind, "You have a significant chance of dodging a level %d monster.", dun_level);
	else if (chance < 40)
		msg_format(Ind, "You have a large chance of dodging a level %d monster.", dun_level);
	else if (chance < 70)
		msg_format(Ind, "You have a high chance of dodging a level %d monster.", dun_level);
	else
		msg_format(Ind, "You will usually dodge a level %d monster.", dun_level);
 #endif
#else
	int lev;
	lev = p_ptr->lev * 2 < 127 ? p_ptr->lev * 2 : 127;

	if (is_admin(p_ptr))
		msg_format(Ind, "You have a %d%%/%d%% chance of dodging a level %d/%d monster.",
		    apply_dodge_chance(Ind, p_ptr->lev), apply_dodge_chance(Ind, lev), p_ptr->lev, lev);

/*
	msg_format(Ind, "You have %s/%s chance of dodging a level %d/%d monster.",
		dodge_diz(apply_dodge_chance(Ind, p_ptr->lev)), dodge_diz(apply_dodge_chance(Ind, lev)),
		p_ptr->lev, lev);

	msg_format(Ind, "You have %s chance of dodging a level %d monster.",
	    dodge_diz(apply_dodge_chance(Ind, p_ptr->lev)), p_ptr->lev);
*/

	else msg_format(Ind, "You have a %d%% chance of dodging a level %d monster's attack.",
	    apply_dodge_chance(Ind, p_ptr->lev), p_ptr->lev);
#endif
	return;
}



void check_parryblock(int Ind) {
#ifdef ENABLE_NEW_MELEE
	char msg[MAX_CHARS];
	player_type *p_ptr = Players[Ind];
	int apc = apply_parry_chance(p_ptr, p_ptr->weapon_parry), abc = apply_block_chance(p_ptr, p_ptr->shield_deflect);

	if (is_admin(p_ptr)) {
		msg_format(Ind, "You have exactly %d%%/%d%% base chance of parrying/blocking.", 
			p_ptr->weapon_parry, p_ptr->shield_deflect);
		msg_format(Ind, "You have exactly %d%%/%d%% real chance of parrying/blocking.", 
			apc, abc);
	} else {
		if (!apc)
			strcpy(msg, "You cannot parry at the moment. ");
			//msg_print(Ind, "You cannot parry at the moment.");
 #if 0
		else if (apc < 5)
			msg_print(Ind, "You have almost no chance of parrying.");
		else if (apc < 10)
			msg_print(Ind, "You have a slight chance of parrying.");
		else if (apc < 20)
			msg_print(Ind, "You have a significant chance of parrying.");
		else if (apc < 30)
			msg_print(Ind, "You have a good chance of parrying.");
		else if (apc < 40)
			msg_print(Ind, "You have a very good chance of parrying.");
		else if (apc < 50)
			msg_print(Ind, "You have an excellent chance of parrying.");
		else
			msg_print(Ind, "You have a superb chance of parrying.");
 #else
		else strcpy(msg, format("You have a %d%% chance of parrying. ",
			apc));
 #endif

		if (!abc)
			strcat(msg, "You cannot block at the moment.");
			//msg_print(Ind, "You cannot block at the moment.");
 #if 0
		else if (abc < 5)
			msg_print(Ind, "You have almost no chance of blocking.");
		else if (abc < 14)
			msg_print(Ind, "You have a slight chance of blocking.");
		else if (abc < 23)
			msg_print(Ind, "You have a significant chance of blocking.");
		else if (abc < 33)
			msg_print(Ind, "You have a good chance of blocking.");
		else if (abc < 43)
			msg_print(Ind, "You have a very good chance of blocking.");
		else if (abc < 48)
			msg_print(Ind, "You have an excellent chance of blocking.");
		else
			msg_print(Ind, "You have a superb chance of blocking.");
 #else
		else strcat(msg, format("You have a %d%% chance of blocking.",
			abc));
 #endif
		msg_print(Ind, msg);
	}
#endif
	return;
}



void toggle_shoot_till_kill(int Ind) {
	player_type *p_ptr = Players[Ind];
	if (p_ptr->shoot_till_kill) {
		msg_print(Ind, "\377wFire-till-kill mode now off.");
		p_ptr->shooting_till_kill = FALSE;
	} else {
		msg_print(Ind, "\377wFire-till-kill mode now on!");
	}
	p_ptr->shoot_till_kill = !p_ptr->shoot_till_kill;
s_printf("SHOOT_TILL_KILL: Player %s toggles %s.\n", p_ptr->name, p_ptr->shoot_till_kill ? "true" : "false");
	p_ptr->redraw |= PR_STATE;
	return;
}

void toggle_dual_mode(int Ind) {
	player_type *p_ptr = Players[Ind];

	if (!get_skill(p_ptr, SKILL_DUAL)) {
		msg_print(Ind, "You are not proficient in dual-wielding.");
		return;
	}

	if (p_ptr->dual_mode) {
		if (cursed_p(&p_ptr->inventory[INVEN_WIELD]) || (p_ptr->dual_wield && cursed_p(&p_ptr->inventory[INVEN_ARM]))) {
			msg_print(Ind, "\377yYou cannot switch to main-hand mode while wielding a cursed weapon!");
			return;
		}
		msg_print(Ind, "\377wDual-wield mode: Main-hand. (This disables all dual-wield boni.)");
	} else msg_print(Ind, "\377wDual-wield mode: Dual-hand.");

	p_ptr->dual_mode = !p_ptr->dual_mode;
s_printf("DUAL_MODE: Player %s toggles %s.\n", p_ptr->name, p_ptr->dual_mode ? "true" : "false");
	p_ptr->redraw |= PR_STATE | PR_PLUSSES;
	calc_boni(Ind);
	return;
}

#ifdef CENSOR_SWEARING
/* similar to strstr(), but catches char repetitions and swap-arounds.
   TODO: current implementation is pretty naive, need to use more effective algo when less lazy. */
#define GET_MOST_DELAYED_MATCH /* must be enabled */
static char* censor_strstr(char *line, char *word, int *eff_len) {
	char bufl[MSG_LEN], bufs[NAME_LEN], *best = NULL;
	int i, j, add;

	if (line[0] == '\0') return (char*)NULL;
	if (word[0] == '\0') return (char*)NULL;//or line, I guess.

	strcpy(bufl, line);
	strcpy(bufs, word);
	*eff_len = 0;

	/* (Note: Since we're returning line[] without any char-mapping, we
	   can only do replacements here, not shortening/expanding. */
	/* replace 'ck' or 'kc' by just 'kk' */
	i = 0;
	while (bufl[i] && bufl[i + 1]) {
		if (bufl[i] == 'c' && bufl[i + 1] == 'k') bufl[i] = 'k';
		else if (bufl[i] == 'k' && bufl[i + 1] == 'c') bufl[i + 1] = 'k';
		i++;
	}
	i = 0;
	while (bufs[i] && bufs[i + 1]) {
		if (bufs[i] == 'c' && bufs[i + 1] == 'k') bufs[i] = 'k';
		else if (bufs[i] == 'k' && bufs[i + 1] == 'c') bufs[i + 1] = 'k';
		i++;
	}

	/* search while allowing repetitions/swapping of characters.
	   Important: Get the furthest possible match for the starting char of
	   the search term, required for 'preceeding/separated' check which is
	   executed directly on 'line' further below in censor_aux(). Eg:
	   sshit -> match the 2nd s and return it as position, not the 1st.
	   Otherwise, "this shit" -> "thisshit" -> the first s and the h would
	   be separated by a space in 'line' and therefore trigger the nonswear
	   special check. */
	i = 0;
	while (bufl[i]) {
		j = 0;
		add = 0; /* track duplicte chars */

#ifdef GET_MOST_DELAYED_MATCH
		/* if we already got a match and the test of the upfollowing
		   position is negative, take that match, since we still want
		   to get all occurances eventually, not just the last one. */
		if (bufs[j] != bufl[i + j + add] && best) return best;
#endif

		while (bufs[j] == bufl[i + j + add]) {
			if (bufs[j + 1] == '\0') {
				*eff_len = j + add + 1;
#ifndef GET_MOST_DELAYED_MATCH
				return &line[i]; /* FOUND */
#else
				best = &line[i];
				break;
#endif
			}
			j++;

			/* end of line? */
			if (bufl[i + j + add] == '\0')
#ifndef GET_MOST_DELAYED_MATCH
				return (char*)NULL; /* NOT FOUND */
#else
				return best;
#endif

			/* reduce duplicate chars */
			if (bufs[j] != bufl[i + j + add]) {
				if (bufs[j - 1] == bufl[i + j + add])
					add++;
			}
		}
		/* NOT FOUND so far */
		i++;
	}
#ifndef GET_MOST_DELAYED_MATCH
	return (char*)NULL; /* NOT FOUND */
#else
	return best;
#endif
}

/* Censor swear words while keeping good words, and determining punishment level */
//NOTE: EXEMPT_BROKEN_SWEARWORDS and HIGHLY_EFFECTIVE_CENSOR shouldn't really work togehter (because latter one kills spaces etc.)
#define EXEMPT_BROKEN_SWEARWORDS	/* don't 'recognize' swear words that are broken up into 'innocent' parts */
#define HIGHLY_EFFECTIVE_CENSOR		/* strip all kinds of non-alpha chars too? ([disabled])*/
#define HEC_RESTRICTION			/* actually restrict HIGHLY_EFFECTIVE_CENSOR so it does _not_ strip a couple specific \
					   non-alpha chars which probably weren't used for masking swearing - eg: \
					   "as---s" obviously means "ass", but "as '?' symbol" shouldn't recognize "ass". */
#define CENSOR_PH_TO_F			/* (slightly picky) convert ph to f ?*/
#define REDUCE_DUPLICATE_H		/* (slightly picky) reduce multiple h to just one? */
#define REDUCE_H_CONSONANT		/* (slightly picky) drop h before consonants? */
#define CENSOR_LEET			/* 433+ $p34k: Try to translate certain numbers and symbols to letters? ([disabled]) */
#define EXEMPT_VSHORT_COMBINED		/* Exempt very short swear words if they're part of a bigger 'word'?
					   //(Problem: They could just be preceeded by 'the' or 'you' w/o a space) -
					   //practically unfeasible (~1250+ words!) to utilize nonswear list for this ([enabled]) */
#ifdef EXEMPT_VSHORT_COMBINED
 #define VSHORT_STEALTH_CHECK		/* Perform a check if the swear word is masked by things like 'the', 'you', 'a(n)'.. */
#endif
#define SMARTER_NONSWEARING		/* Match single spaces inside nonswearing words with any amount of consecutive insignificant characters in the chat */
static int censor_aux(char *buf, char *lcopy, int *c, bool leet, bool max_reduce) {
	int i, j, k, offset, cc[MSG_LEN], pos, eff_len;
	char line[MSG_LEN];
	char *word, l0, l1, l2, l3;
	int level = 0;
#ifdef VSHORT_STEALTH_CHECK
	bool masked;
#endif
#ifdef HEC_RESTRICTION
	char repeated = 0;
	bool hec_seq = FALSE, ok1 = FALSE, ok2 = FALSE, ok3 = FALSE;
#endif

	/* create working copies */
	strcpy(line, buf);
	for (i = 0; !i || c[i]; i++) cc[i] = c[i];
	cc[i] = 0;

	/* replace certain non-alpha chars by alpha chars (leet speak detection)? */
	if (leet) {
#ifdef HIGHLY_EFFECTIVE_CENSOR
		bool is_leet = FALSE, was_leet;
#endif
		i = 0;
		while (lcopy[i]) {
#ifdef HIGHLY_EFFECTIVE_CENSOR
			was_leet = is_leet;
			is_leet = TRUE;//(i != 0);//meaning 'i > 0', for efficiency
#endif

			/* keep table of leet chars consistent with the one in censor() */
			switch (lcopy[i]) {
			case '@': lcopy[i] = 'a'; break;
			case '<': lcopy[i] = 'c'; break;
			case '!': lcopy[i] = 'i'; break;
			case '|': lcopy[i] = 'l'; break;//hm, could be i too :/
			case '$': lcopy[i] = 's'; break;
			case '+': lcopy[i] = 't'; break;
			case '1': lcopy[i] = 'i'; break;
			case '3': lcopy[i] = 'e'; break;
			case '4': lcopy[i] = 'a'; break;
			case '5': lcopy[i] = 's'; break;
			case '6': lcopy[i] = 'g'; break;
			case '7': lcopy[i] = 't'; break;
			case '8': lcopy[i] = 'b'; break;
			case '9': lcopy[i] = 'g'; break;
			case '0': lcopy[i] = 'o'; break;

			/* important characters that are usually not leet speek but must be turned
			   into characters that will not get filtered out later, because these
			   characters listed here could actually break words and thereby cause
			   false positives. Eg 'headless (h, l27, 427)' would otherwise wrongly turn
			   into 'headlesshltxaxtx' and trigger false 'shit' positive. */
			case '(': lcopy[i] = 'c'; break; //all of these could be c,i,l I guess.. pick the least problematic one (i)
			case ')': lcopy[i] = 'i'; break; // l prob: "mana/" -> mANAL :-p
			case '/': lcopy[i] = 'i'; break;
			case '\\': lcopy[i] = 'i'; break;

#ifdef HIGHLY_EFFECTIVE_CENSOR
			/* hack: Actually _counter_ the capabilities of highly-effective
			   censoring being done further below after this. Reason:
			   Catch numerical expressions that probably aren't l33t sp34k,
			   such as "4.5", "4,5" or "4/5" but also '4} (55'!
			   Exact rule: <leet char><non-leet non-alphanum char> = allowed. */
			default:
				is_leet = FALSE; //this character wasn't one of the leet cases above
				/* inconspicuous non-alphanum char? */
				if (was_leet &&
				    (lcopy[i] < 'a' || lcopy[i] > 'z') && (lcopy[i] < '0' || lcopy[i] > '9'))
					/* replace by, say 'x' (harmless^^) (Note: Capital character doesn't work, gets filtered out futher below */
					lcopy[i] = 'x';
				break;
#else
			default:
				lcopy[i] = ' ';
 #if 0 /* nonsense, because nonswearing isn't applied after this anymore, but was long before in censor() already! */
				lcopy[i] = '~'; /* ' ' is usually in use in nonswearing.txt,
				    so it would need to be removed there whenever we turn off HIGHLY_EFFECTIVE_CENSOR
				    and put back in when we reenable it. Using a non-used character instead of SPACE
				    is therefore a much more comfortable way. */
 #endif
				break;
#endif
			}

			line[cc[i]] = lcopy[i];
			i++;
		}
	}

#ifdef REDUCE_H_CONSONANT
	/* reduce 'h' before consonant */
	//TODO: do HIGHLY_EFFECTIVE_CENSOR first probably
	i = j = 0;
	while (lcopy[j]) {
		if (lcopy[j] == 'h' &&
		    lcopy[j + 1] != '\0' &&
		    lcopy[j + 1] != 'a' &&
		    lcopy[j + 1] != 'e' &&
		    lcopy[j + 1] != 'i' &&
		    lcopy[j + 1] != 'o' &&
		    lcopy[j + 1] != 'u' &&
		    lcopy[j + 1] != 'y'
		    && lcopy[j + 1] >= 'a' && lcopy[j + 1] <= 'z' /*TODO drop this, see 'TODO' above*/
		    /* detect 'shlt' as masked form of 'shit', without reducing it to 'slt' */
		    && (!j || (lcopy[j - 1] != 'c' && lcopy[j - 1] != 's'))
		    ) {
			j++;
		}

		/* build index map for stripped string */
		cc[i] = cc[j];
		lcopy[i] = lcopy[j];

		i++;
		j++;
	}
	/* ensure the reduced string is possibly terminated earlier */
	lcopy[i] = '\0';
	cc[i] = 0;

	/* reduce 'h' after consonant except for c or s */
	//TODO: do HIGHLY_EFFECTIVE_CENSOR first probably
	i = j = 1;
	if (lcopy[0])
	while (lcopy[j]) {
		if (lcopy[j] == 'h' &&
		    lcopy[j - 1] != 'c' &&
		    lcopy[j - 1] != 's' &&
		    lcopy[j - 1] != 'a' &&
		    lcopy[j - 1] != 'e' &&
		    lcopy[j - 1] != 'i' &&
		    lcopy[j - 1] != 'o' &&
		    lcopy[j - 1] != 'u'
		    && lcopy[j - 1] >= 'a' && lcopy[j - 1] <= 'z' /*TODO drop this, see 'TODO' above*/
		    ) {
			j++;
			continue;
		}

		/* build index map for stripped string */
		cc[i] = cc[j];
		lcopy[i] = lcopy[j];

		i++;
		j++;
	}
	/* ensure the reduced string is possibly terminated earlier */
	lcopy[i] = '\0';
	cc[i] = 0;
#endif

#ifdef HIGHLY_EFFECTIVE_CENSOR
	/* check for non-alpha chars and _drop_ them (!) */
	i = j = 0;
	while (lcopy[j]) {
		/* non-alphanum symbol? */
		if ((lcopy[j] < 'a' || lcopy[j] > 'z') &&
		    lcopy[j] != 'Z') {
 #ifdef HEC_RESTRICTION
			/* new sequence to look ahead into? */
			if (!hec_seq) {
				hec_seq = TRUE;
				k = j;
				while (lcopy[k] && (lcopy[k] < 'a' || lcopy[k] > 'z') && lcopy[k] != 'Z') {
					/* don't allow these 'bad' symbols to be the only ones occuring. Note: spaces don't count as bad symbols */
					if (lcopy[k] != '.' && lcopy[k] != ',' && lcopy[k] != '-') ok3 = TRUE;

					/* make sure it's not just a single symbol getting repeated */
					if (!repeated) {
						repeated = lcopy[k];
						k++;
						continue;
					}
					if (lcopy[k] != repeated) ok1 = TRUE; /* different symbols = rather inconspicuous */

					/* at least 3 non-alphanum symbols for the sequence to have any "meaning" */
					if (k - j >= 2) ok2 = TRUE;

					if (ok1 && ok2 && ok3) break;
					k++;
				}
			}
			if (!ok1 || !ok2 || !ok3) { /* conspicuous sequence? (possibly intended to mask swearing) */
				/* cut them out */
				j++;
				continue;
			}
 #else
			/* cut them out */
			j++;
			continue;
 #endif
		}
 #ifdef HEC_RESTRICTION
		else hec_seq = FALSE;
 #endif

		/* modify index map for stripped string */
		cc[i] = cc[j];
		lcopy[i] = lcopy[j];
		i++;
		j++;
	}
	/* ensure the reduced string is possibly terminated earlier */
	lcopy[i] = '\0';
	cc[i] = 0;
#endif

	/* reduce repeated chars (>3 for consonants, >2 for vowel) */
	i = j = 0;
	while (lcopy[j]) {
		/* paranoia (if HIGHLY_EFFECTIVE_CENSOR is enabled) */
		if (lcopy[j] < 'a' || lcopy[j] > 'z') {
			cc[i] = cc[j];
			lcopy[i] = lcopy[j];
			i++;
			j++;
			continue;
		}

		/* reduce any char repetition and re-check for swear words */
		if (max_reduce) {
			if (lcopy[j + 1] == lcopy[j]) {
				j++;
				continue;
			}
		} else switch (lcopy[j]) {
		case 'a': case 'e': case 'i': case 'o': case 'u': case 'y':
			if (lcopy[j + 1] == lcopy[j] &&
			    lcopy[j + 2] == lcopy[j]) {
				j++;
				continue;
			}
		default:
			if (lcopy[j + 1] == lcopy[j] &&
			    lcopy[j + 2] == lcopy[j] &&
			    lcopy[j + 3] == lcopy[j]) {
				j++;
				continue;
			}
			break;
		}

		/* modify index map for reduced string */
		cc[i] = cc[j];
		lcopy[i] = lcopy[j];
		i++;
		j++;
	}
	/* ensure the reduced string is possibly terminated earlier */
	lcopy[i] = '\0';
	cc[i] = 0;

	/* check for swear words and censor them */
	for (i = 0; swear[i].word[0]; i++) {
		offset = 0;

		/* check for multiple occurrances of this swear word */
		while ((word = censor_strstr(lcopy + offset, swear[i].word, &eff_len))) {
			pos = word - lcopy;
			l0 = tolower(line[cc[pos]]);
			l1 = l2 = l3 = 0; //kill compiler warnings
			if (cc[pos] >= 1) l1 = tolower(line[cc[pos] - 1]);
			if (cc[pos] >= 2) {
				l2 = tolower(line[cc[pos] - 2]);
				/* catch & dismiss colour codes */
				if (l2 == '\377') l1 = 'Y'; /* 'disable' char */
			}
			if (cc[pos] >= 3) {
				l3 = tolower(line[cc[pos] - 3]);
				/* catch & dismiss colour codes */
				if (l3 == '\377') l2 = 'Y'; /* 'disable' char */
			}

			/* prevent checking the same occurance repeatedly */
			offset = pos + 1;

#ifdef EXEMPT_BROKEN_SWEARWORDS
			/* hm, maybe don't track swear words if proceeded and postceeded by some other letters
			   and separated by spaces inside it -- and if those nearby letters aren't the same letter,
			   if someone just duplicates it like 'sshitt' */
			/* check for swear-word-preceding non-duplicate alpha char */
			if (cc[pos] > 0 &&
			    l1 >= 'a' && l1 <= 'z' &&
			    (l1 != l0 || strlen(swear[i].word) <= 3)) {
#ifdef EXEMPT_VSHORT_COMBINED
				/* special treatment for swear words of <= 3 chars length: */
				if (strlen(swear[i].word) <= 3) {
 #ifdef VSHORT_STEALTH_CHECK
					/* check for masking by 'the', 'you', 'a(n)' etc. */
					masked = TRUE;
					if (cc[pos] == 1 || cc[pos] == 2) {
						/* if there's only 1 or 2 chars before the swear word, it depends on the first
						   char of the swear word: vowel is ok, consonant is not. */
						switch (l0) {
						case 'a': case 'e': case 'i': case 'o': case 'u': case 'y':
							masked = FALSE;
							break;
						}
					}
					/* if there are more chars before the swear word, it's maybe ok if
					   the swear word starts on a vowel ('XXXXass' is very common!) */
					else {
						/* ..to be specific, if the char before the swear word is NOT a vowel (well, duh, true for most cases).
						   Yeah yeah, just trying to fix 'ass' detection here, and a 'y' or 'u' in front of it remains swearing.. */
						switch (l0) {
						case 'a': case 'e': case 'i': case 'o': case 'u': case 'y':
							switch (l1) {
							case 'a': case 'e': case 'i': case 'o': case 'u': case 'y':
								break;
							default:
								masked = FALSE;
								break;
							}
							break;
						}
					}
					/* if swear word is followed by a vowel, it's ok. */
					switch (tolower(line[cc[pos] + strlen(swear[i].word)])) {
					case 'a': case 'e': case 'i': case 'o': case 'u': case 'y':
						masked = FALSE;
						break;
					}
				    /* so if it is already determined by now to be a masked swear word, skip the vshort-exemption-checks */
				    if (!masked) {
 #endif

 #if 0 /* softened this up, see below */
					/* if there's UP TO 2 other chars before it or exactly 1 non-duplicate char, it's exempt.
					   (for more leading chars, nonswear has to be used.) */
					if (cc[pos] == 1 && l1 >= 'a' && l1 <= 'z' && l1 != l0) continue;
					if (cc[pos] == 2) {
						if (l1 >= 'a' && l1 <= 'z' && l2 >= 'a' && l2 <= 'z' &&
						    (l0 != l1 || l0 != l2 || l1 != l2)) continue;
						/* also test for only 1 leading alpha char */
						if ((l2 < 'a' || l2 > 'z') &&
						    l1 >= 'a' && l1 <= 'z' && l1 != l0) continue;
					}
					if (cc[pos] >= 3) {
						 if ((l3 < 'a' || l3 > 'z') &&
						    l1 >= 'a' && l1 <= 'z' && l2 >= 'a' && l2 <= 'z' &&
						    (l0 != l1 || l0 != l2 || l1 != l2)) continue;
						/* also test for only 1 leading alpha char */
						if ((l2 < 'a' || l2 > 'z') &&
						    l1 >= 'a' && l1 <= 'z' && l1 != l0) continue;
					}
					/* if there's no char before it but 2 other chars after it or 1 non-dup after it, it's exempt. */
					//TODO maybe - or just use nonswear for that
 #else
					/* if there's at least 2 other chars before it, which aren't both duplicates, or
					   if there's exactly 1 non-duplicate char before it, it's exempt. */
					if (cc[pos] == 1 && l1 >= 'a' && l1 <= 'z' && l1 != l0) continue;
					if (cc[pos] >= 2) {
						if (l1 >= 'a' && l1 <= 'z' && l2 >= 'a' && l2 <= 'z' &&
						    (l0 != l1 || l0 != l2 || l1 != l2)) continue;
						/* also test for only 1 leading alpha char */
						if ((l2 < 'a' || l2 > 'z') &&
						    l1 >= 'a' && l1 <= 'z' && l1 != l0) continue;
					}
					/* if there's no char before it but 2 other chars after it or 1 non-dup after it, it's exempt. */
					//TODO maybe - or just use nonswear for that
 #endif
 #ifdef VSHORT_STEALTH_CHECK
				    }
 #endif
				}
#endif

				/* check that the swear word occurance was originally non-continuous, ie separated by space etc.
				   (if this is FALSE then this is rather a case for nonswearwords.txt instead.) */
				for (j = cc[pos]; j < cc[pos + strlen(swear[i].word) - 1]; j++) {
					if (tolower(line[j]) < 'a' || tolower(line[j] > 'z')) {
						/* heuristical non-swear word found! */
						break;
					}
				}
				/* found? */
				if (j < cc[pos + strlen(swear[i].word) - 1]) {
					/* prevent it from getting tested for swear words */
					continue;
				}
			}
#endif

			/* we can skip ahead */
			offset = pos + strlen(swear[i].word);

#if 0
			/* censor it! */
			for (j = 0; j < eff_len); j++) {
				line[cc[pos + j]] = buf[cc[pos + j]] = '*';
				lcopy[pos + j] = '*';
			}
#else
			/* actually censor separator chars too, just so it looks better ;) */
			for (j = 0; j < eff_len - 1; j++) {
				for (k = cc[pos + j]; k <= cc[pos + j + 1]; k++)
					line[k] = buf[k] = '*';

				/* MUST be disabled or interferes with detection overlaps: */
				/* for processing lcopy in a while loop instead of just 'if'
				   -- OBSOLETE ACTUALLY (thanks to 'offset')? */
				//lcopy[pos + j] = '*';
			}
			/* see right above - MUST be disabled: */
			//lcopy[pos + j] = '*';
#endif
			level = MAX(level, swear[i].level);
		}
	}
	return(level);
}

static int censor(char *line) {
	int i, j, im, jm, cc[MSG_LEN], offset;
#ifdef CENSOR_LEET
	int j_pre = 0, cc_pre[MSG_LEN];
#endif
	char lcopy[MSG_LEN], lcopy2[MSG_LEN];
	char tmp1[MSG_LEN], tmp2[MSG_LEN], tmp3[MSG_LEN], tmp4[MSG_LEN];
	char *word;

#ifdef SMARTER_NONSWEARING
	/* non-swearing extra check (reduces insignificant characters) */
	bool reduce;
	char ccns[MSG_LEN];
#endif


	strcpy(lcopy, line);

	/* special expressions to exempt: '@S...<.>blabla': '@S-.shards' -> '*****hards' :-p */
	if ((word = strstr(lcopy, "@S"))) {
		char *c = word + 2;

		while (*c) {
			switch (*c) {
			/* still within @S expression */
			case 'k': case 'K': case 'm': case 'M':
			case '-': case '0': case '1': case '2':
			case '3': case '4': case '5': case '6':
			case '7': case '8': case '9':
				c++;
				continue;
			/* end of @S expression */
			case '.':
				/* prevent the whole expression from getting tested for swear words */
				while (word <= c) {
					//lcopy2[(word - lcopy)] = 'Z';
					*word = 'Z';
					word++;
				}
				c = lcopy + strlen(lcopy); //to exit outer while loop
				break;
			/* not a valid @S expression or not followed up by any further text */
			default:
				c = lcopy + strlen(lcopy); //to exit outer while loop
				break;
			}
		}
	}

	/* convert to lower case */
	i = 0;
	while (lcopy[i]) {
		lcopy[i] = tolower(lcopy[i]);
		i++;
	}

	/* strip colour codes and initialize index map (cc[]) at that */
	i = j = 0;
	while (lcopy[j]) {
		if (lcopy[j] == '\377') {
			j++;
			if (lcopy[j]) j++;
			continue;
		}

		/* initially build index map for stripped string */
		cc[i] = j;
		lcopy[i] = lcopy[j];

		i++;
		j++;
	}
	lcopy[i] = '\0';
	cc[i] = 0;

#ifdef CENSOR_PH_TO_F
	/* reduce ph to f */
	i = j = 0;
	while (lcopy[j]) {
		if (lcopy[j] == 'p' && lcopy[j + 1] == 'h') {
			cc[i] = cc[j];
			lcopy[i] = 'f';
			i++;
			j += 2;
			continue;
		}

		/* build index map for stripped string */
		cc[i] = cc[j];
		lcopy[i] = lcopy[j];

		i++;
		j++;
	}
	lcopy[i] = '\0';
	cc[i] = 0;
#endif

#ifdef REDUCE_DUPLICATE_H
	/* reduce hh to h */
	i = j = 0;
	while (lcopy[j]) {
		if (lcopy[j] == 'h' && lcopy[j + 1] == 'h') {
			j++;
			continue;
		}

		/* build index map for stripped string */
		cc[i] = cc[j];
		lcopy[i] = lcopy[j];

		i++;
		j++;
	}
	lcopy[i] = '\0';
	cc[i] = 0;
#endif

#ifdef CENSOR_LEET
	/* special hardcoded case: variants of '455hole' leet-speak */
	j = 0;
	for (i = 0; i < strlen(lcopy); i++) {
		/* reduce duplicate chars (must do it this way instead of i==i+1, because we need full cc-mapped string) */
		if (i && lcopy[i] == lcopy[i - 1]) continue;
		/* build un-leeted, mapped string */
		if ((lcopy[i] >= '0' && lcopy[i] <= '9') || (lcopy[i] >= 'a' && lcopy[i] <= 'z')
		    || lcopy[i] == '@' || lcopy[i] == '$' || lcopy[i] == '!' || lcopy[i] == '|' || lcopy[i] == '+'
		    ) {
			/* keep table of leet chars consistent with the one in censor_aux() */
			switch (lcopy[i]) {
			case '@': lcopy2[j] = 'a'; break;
			case '<': lcopy2[j] = 'c'; break;
			case '!': lcopy2[j] = 'i'; break;
			case '|': lcopy2[j] = 'l'; break;//hm, could be i too :/
			case '$': lcopy2[j] = 's'; break;
			case '+': lcopy2[j] = 't'; break;
			case '1': lcopy2[j] = 'i'; break;
			case '3': lcopy2[j] = 'e'; break;
			case '4': lcopy2[j] = 'a'; break;
			case '5': lcopy2[j] = 's'; break;
			case '6': lcopy2[j] = 'g'; break;
			case '7': lcopy2[j] = 't'; break;
			case '8': lcopy2[j] = 'b'; break;
			case '9': lcopy2[j] = 'g'; break;
			case '0': lcopy2[j] = 'o'; break;
			default: lcopy2[j] = lcopy[i];
			}
			cc_pre[j] = i;
			j++;
		}
	}
	lcopy2[j] = 0;
	if ((word = strstr(lcopy2, "ashole")) ||
	    (word = strstr(lcopy2, "ashoie")))
		/* use severity level of 'ashole' (condensed) for 'asshole' */
		for (i = 0; swear[i].word[0]; i++) {
			if (!strcmp(swear[i].word, "ashole")) {
				j_pre = swear[i].level;
				/* if it was to get censored, do so */
				if (j_pre) {
 #if 0
					for (offset = cc_pre[word - lcopy2]; offset <= cc_pre[(word - lcopy2) + 5]; offset++)
						lcopy[offset] = '*';
 #endif
					for (offset = cc[cc_pre[word - lcopy2]]; offset <= cc[cc_pre[(word - lcopy2) + 5]]; offset++)
						line[offset] = '*';
				}
				break;
			}
		}
#endif

	/* check for legal words first */
	//TODO: could be moved into censor_aux and after leet speek conversion, to allow leet speeking of non-swear words (eg "c00k")
	strcpy(lcopy2, lcopy); /* use a 'working copy' to allow _overlapping_ nonswear words --- since we copy it below anyway, we don't need to create a working copy here */
	/*TODO!!! non-split swear words should take precedence over split-up non-swear words: "was shit" -> "as sh" is 'fine'.. BEEP!
	  however, this can be done by disabling HIGHLY_EFFECTIVE_CENSORING and enabling EXEMPT_BROKEN_SWEARWORDS as a workaround. */
#ifdef SMARTER_NONSWEARING
	/* Replace insignificant symbols by spaces and reduce consecutive spaces to one */
	reduce = FALSE;
	j = 0;
	for (i = 0; i < strlen(lcopy2); i++) {
		ccns[j] = i;
		switch (lcopy2[i]) {
		case ' ':
			if (reduce) continue;
			reduce = TRUE;
			lcopy[j] = lcopy2[i];
			j++;
			continue;
		/* insignificant characters only, that won't break leet speak detection in censor_aux() if we filter them out here */
		case '"':
		case '\'':
 #if 1 /* need better way, but perfect solution impossible? ^^ -- for now benefit of doubt (these COULD be part of swearing): */
		case '(':
		case ')':
		case '/':
		case '\\':
 #endif
			if (reduce) continue;
			reduce = TRUE;
			lcopy[j] = ' ';
			j++;
			continue;
		/* normal characters */
		default:
			lcopy[j] = lcopy2[i];
			ccns[j] = i;
			j++;
			reduce = FALSE;
		}
	}
	lcopy[j] = 0;
#endif
//s_printf("ns: '%s'  '%s'\n", lcopy, lcopy2); //DEBUG
	/* Maybe todo: Also apply nonswear to lcopy2 (the non-reduced version of lcopy) afterwards */
	for (i = 0; nonswear[i][0]; i++) {
#ifndef HIGHLY_EFFECTIVE_CENSOR
		/* hack! If HIGHLY_EFFECTIVE_CENSOR is NOT enabled, skip all nonswearing-words that contain spaces!
		   This is done because those nonswearing-words are usually supposed to counter wrong positives
		   from HIGHLY_EFFECTIVE_CENSOR, and will lead to false negatives when it is disabled xD
		   (eg: 'was shit' chat with "as sh" non-swear)  - C. Blue */
		if (strchr(nonswear[i], ' ')) continue;
#endif

		offset = 0;
		/* check for multiple occurrances of this nonswear word */
		while ((word = strstr(lcopy + offset, nonswear[i]))) {
			/* prevent checking the same occurance repeatedly */
			offset = word - lcopy + strlen(nonswear[i]);

			if ((nonswear_affix[i] & 0x1)) {
				if (word == lcopy) continue; //beginning of lcopy string?
				if (!((*(word - 1) >= 'a' && *(word - 1) <= 'z') ||
				    (*(word - 1) >= '0' && *(word - 1) <= '1') ||
				    *(word - 1) == '\''))
					continue;
			}
			if ((nonswear_affix[i] & 0x2)) {
				if (!word[strlen(nonswear[i])]) continue; //end of lcopy string?
				if (!((lcopy[offset] >= 'a' && lcopy[offset] <= 'z') ||
				    (lcopy[offset] >= '0' && lcopy[offset] <= '1') ||
				    lcopy[offset] == '\''))
					continue;
			}

			/* prevent it from getting tested for swear words */
#ifdef SMARTER_NONSWEARING
			im = ccns[word - lcopy];
			jm = ccns[word - lcopy + strlen(nonswear[i]) - 1];
			for (j = im; j <= jm; j++)
				lcopy2[j] = 'Z';
#else
			for (j = 0; j < strlen(nonswear[i]); j++)
				lcopy2[(word - lcopy) + j] = 'Z';
#endif
		}
	}

	/* perform 2x2 more 'parallel' tests  */
	strcpy(tmp1, line);
	strcpy(tmp2, line);
	strcpy(tmp3, line);
	strcpy(tmp4, line);
	strcpy(lcopy, lcopy2);
	i = censor_aux(tmp1, lcopy, cc, FALSE, FALSE); /* continue with normal check */
	strcpy(lcopy, lcopy2);
#ifdef CENSOR_LEET
	j = censor_aux(tmp2, lcopy, cc, TRUE, FALSE); /* continue with leet speek check */
	if (j_pre > j) j = j_pre; //pre-calculated special case
	strcpy(lcopy, lcopy2);
#else
	j = 0;
#endif
	im = censor_aux(tmp3, lcopy, cc, FALSE, TRUE); /* continue with normal check */
	strcpy(lcopy, lcopy2);
#ifdef CENSOR_LEET
	jm = censor_aux(tmp4, lcopy, cc, TRUE, TRUE); /* continue with leet speek check */
#else
	jm = 0;
#endif

	/* combine all censored characters */
	for (offset = 0; tmp1[offset]; offset++)
		if (tmp1[offset] == '*') line[offset] = '*';
#ifdef CENSOR_LEET
	for (offset = 0; tmp2[offset]; offset++)
		if (tmp2[offset] == '*') line[offset] = '*';
#endif
	for (offset = 0; tmp3[offset]; offset++)
		if (tmp3[offset] == '*') line[offset] = '*';
#ifdef CENSOR_LEET
	for (offset = 0; tmp4[offset]; offset++)
		if (tmp4[offset] == '*') line[offset] = '*';
#endif

	/* return 'worst' result */
	if (j > i) i = j;
	if (im > i) i = im;
	if (jm > i) i = jm;
	return i;
}
#endif

/*
 * A message prefixed by a player name is sent only to that player.
 * Otherwise, it is sent to everyone.
 */
/*
 * This function is hacked *ALOT* to add extra-commands w/o
 * client change.		- Jir -
 *
 * Yeah. totally.
 *
 * Muted players can't talk now (player_type.muted-- can be enabled
 * through /mute <name> and disabled through /unmute <name>).
 *				- the_sandman
 */
static void player_talk_aux(int Ind, char *message) {
	int i, len, target = 0, target_raw_len = 0;
	char search[MSG_LEN], sender[MAX_CHARS];
	char message2[MSG_LEN];
	player_type *p_ptr = NULL, *q_ptr;
	char *colon;
	bool me = FALSE, me_gen = FALSE, log = TRUE, nocolon = FALSE;
	char c_n = 'B'; /* colours of sender name and of brackets (unused atm) around this name */
#ifdef KURZEL_PK
	char c_b = 'B';
#endif
	int mycolor = 0;
	bool admin = FALSE;
	bool broadcast = FALSE;

#ifdef TOMENET_WORLDS
	char tmessage[MSG_LEN];		/* TEMPORARY! We will not send the name soon */
#endif

	if (!Ind) {
		console_talk_aux(message);
		return;
	}

	/* Get sender's name */
	p_ptr = Players[Ind];
	/* Get player name */
	strcpy(sender, p_ptr->name);
	admin = is_admin(p_ptr);

	/* Default to no search string */
	strcpy(search, "");

	/* this ain't a proper colon (must have a receipient or chat code on the left side!) */
	if (message[0] == ':') nocolon = TRUE;


	/* Look for a player's name followed by a colon */

	/* tBot's stuff */
	/* moved here to allow tbot to see fake pvt messages. -Molt */
	strncpy(last_chat_owner, sender, NAME_LEN);
	strncpy(last_chat_line, message, MSG_LEN);
	/* do exec_lua() not here instead of in dungeon.c - mikaelh */

	/* '-:' at beginning of message sends to normal chat, cancelling special chat modes - C. Blue */
	if (strlen(message) >= 4 && message[1] == ':' && message[2] == '-' && message[3] == ':')
		message += 4;
	/* Catch this case too anyway, just for comfort if someone uses fixed macros for -: */
	else if (strlen(message) >= 2 && message[0] == '-' && message[1] == ':')
		message += 2;


	/* hack: preparse for private chat target, and if found
	   then strip chat mode prefixes to switch to private chat mode - C. Blue */
	/* Form a search string if we found a colon */
	if (strlen(message) > 4 && message[1] == ':' && message[2] != ':' &&
	    (message[0] == '!' || message[0] == '#' || message[0] == '$')
	    && (colon = strchr(message + 3, ':'))) {
		/* Copy everything up to the colon to the search string */
		strncpy(search, message + 2, colon - message - 2);
		search[colon - message - 2] = '\0';
		/* Acquire length of search string */
		len = strlen(search);
		/* Look for a recipient who matches the search string */
		if (len) {
#ifdef TOMENET_WORLDS
			struct rplist *w_player;
#endif
			/* NAME_LOOKUP_LOOSE DESPERATELY NEEDS WORK */
			target = name_lookup_loose(Ind, search, TRUE, TRUE, TRUE);

			if (target) {
				/* strip leading chat mode prefix,
				   pretend that it's a private message instead */
				message += 2;
			}
#ifdef TOMENET_WORLDS
			else if (cfg.worldd_privchat) {
				w_player = world_find_player(search, 0);
				if (w_player) {
					/* strip leading chat mode prefix,
					   pretend that it's a private message instead */
					message += 2;
				}
			}
#endif
		}

		/* erase our traces */
		search[0] = '\0';
		len = 0;
	}


	colon = strchr(message, ':');
	/* only assume targetted chat if the 'name' would acually be of acceptable length */
	if (colon && (colon - message) > NAME_LEN) {
		/* in case the player actually used double-colon, not knowing of this clever mechanism here, fix it for him ;) */
		if (colon[1] == ':') {
			i = (int) (colon - message);
			do message[i] = message[i + 1];
			while (message[i++] != '\0');
		}

		colon = NULL;
	}

	/* Ignore "smileys" or URL */
	//if (colon && strchr(")(-/:", colon[1]))
	/* (C. Blue) changing colon parsing. :: becomes
	    textual :  - otherwise : stays control char */
	if (colon) {
		bool smiley = FALSE;
		/* no double-colon '::' smileys inside possible item inscriptions */
		char *bracer = strchr(message, '{');
		bool maybe_inside_inscription = bracer != NULL && bracer < colon;

		target_raw_len = colon - message;

		/* if another colon followed this one then
		   it was not meant to be a control char */
		switch (colon[1]) {
		/* accept these chars for smileys, but only if the colon is either first in the line or stands after a SPACE,
		   otherwise it is to expect that someone tried to write to party/privmsg */
		case '(':	case ')':
		case '[':	case ']':
		case '{':	case '}':
/* staircases, so cannot be used for smileys here ->		case '<':	case '>': */
		case '\\':	case '|':
		case 'p': case 'P': case 'o': case 'O':
		case 'D': case '3':
			if (message == colon || colon[-1] == ' ' || colon[-1] == '>' || /* >:) -> evil smiley */
			    ((message == colon - 1) && (colon[-1] != '!') && (colon[-1] != '#') && (colon[-1] != '%') && (colon[-1] != '$') && (colon[-1] != '+'))) /* <- party names must be at least 2 chars then */
				colon = NULL; /* the check is mostly important for '(' */
			break;
		case '-':
			if (message == colon || colon[-1] == ' ' || colon[-1] == '>' || /* here especially important: '-' often is for numbers/recall depth */
			    ((message == colon - 1) && (colon[-1] != '!') && (colon[-1] != '#') && (colon[-1] != '%') && (colon[-1] != '$') && (colon[-1] != '+'))) /* <- party names must be at least 2 chars then */
				if (!strchr("123456789", *(colon + 2))) colon = NULL;
			break;
		case '/':
			/* only accept / at the end of a chat message */
			if ('\0' == *(colon + 2)) colon = NULL;
			/* or it might be a http:// style link */
			else if ('/' == *(colon + 2)) colon = NULL;
			break;
		case ':':
			/* remove the 1st colon found, .. */

			/* always if there's no chat-target in front of the double-colon: */
			if (message == colon || colon[-1] == ' ') {
				i = (int) (colon - message);
				do message[i] = message[i + 1];
				while (message[i++] != '\0');
				colon = NULL;
				break;
			}

			/* new hack: ..but only if the previous two chars aren't  !:  (party chat),
			   and if it's appearently meant to be a smiley. */
			if ((colon - message == 1) && (colon[-1] == '!' || colon[-1] == '#'
			    || colon[-1] == '%' || colon[-1] == '$' || colon[-1] == '+'))
			switch (*(colon + 2)) {
			case '(': case ')':
			case '[': case ']':
			case '{': case '}':
			case '<': case '>':
			case '-': case '|':
			case 'p': case 'P': case 'o': case 'O':
			case 'D': case '3':
			smiley = !maybe_inside_inscription; break; }

			/* check for smiley at end of the line */
			if ((message + strlen(message) - colon >= 3) &&
			    (message + strlen(message) - colon <= 4)) // == 3 for 2-letter-smileys only.
			//if ((*(colon + 2)) && !(*(colon + 3)))
			switch (*(colon + 2)) {
			case '(': case ')':
			case '[': case ']':
			case '{': case '}':
			case '<': case '>':
			case '-': case '/':
			case '\\': case '|':
			case 'p': case 'P': case 'o': case 'O':
			case 'D': case '3':
			smiley = !maybe_inside_inscription; break; }

			if (smiley) break;

			/* Pretend colon wasn't there */
			i = (int) (colon - message);
			do message[i] = message[i + 1];
			while (message[i++] != '\0');
			colon = NULL;
			break;
		case '\0':
			/* if colon is last char in the line, it's not a priv/party msg. */
#if 0 /* disabled this 'feature', might be more convenient - C. Blue */
			colon = NULL;
#else
			if (colon[-1] != '!' && colon[-1] != '#' && colon[-1] != '%' && colon[-1] != '$' && colon[-1] != '+')
				colon = NULL;
#endif
			break;
		}
	}

	/* don't log spammy slash commands */
	if (prefix(message, "/untag ")
	    || prefix(message, "/dis"))
		log = FALSE;

	/* no big brother */
	//if(cfg.log_u && (!colon || message[0] != '#' || message[0] != '/')){ /* message[0] != '#' || message[0] != '/' is always true -> big brother mode - mikaelh */
	if (cfg.log_u && log &&
	    (!colon || nocolon || (message[0] == '/' && strncmp(message, "/note", 5)))) {
		/* Shorten multiple identical messages to prevent log file spam,
		   eg recall rod activation attempts. - Exclude admins. */
		if (admin)
			s_printf("[%s] %s\n", sender, message);
		else if (strcmp(message, p_ptr->last_chat_line)) {
			if (p_ptr->last_chat_line_cnt) {
				s_printf("[%s (x%d)]\n", sender, p_ptr->last_chat_line_cnt + 1);
				p_ptr->last_chat_line_cnt = 0;
			}
			s_printf("[%s] %s\n", sender, message);

			/* Keep track of repeating chat lines to avoid log file spam (slash commands like '/rec' mostly) */
			strncpy(p_ptr->last_chat_line, message, MSG_LEN - 1);
			p_ptr->last_chat_line[MSG_LEN - 1] = 0;
		} else p_ptr->last_chat_line_cnt++;
	}

	/* Special - shutdown command (for compatibility) */
	if (prefix(message, "@!shutdown") && admin) {
		/*world_reboot();*/
		shutdown_server();
		return;
	}

	if (message[0] == '/' ){
		if (!strncmp(message, "/me ", 4)) me = TRUE;
		else if (!strncmp(message, "/me's", 4)) me = me_gen = TRUE;
		else if (!strncmp(message, "/broadcast ", 11)) broadcast = TRUE;
		else {
			do_slash_cmd(Ind, message);	/* add check */
			return;
		}
	}

#ifndef ARCADE_SERVER
	if (!colon) p_ptr->msgcnt++; /* !colon -> only prevent spam if not in party/private chat */
	if (p_ptr->msgcnt > 12) {
		time_t last = p_ptr->msg;
		time(&p_ptr->msg);
		if (p_ptr->msg - last < 6) {
			p_ptr->spam++;
			switch (p_ptr->spam) {
			case 1:
				msg_print(Ind, "\377yPlease don't spam the server");
				break;
			case 3:
			case 4:
				msg_print(Ind, "\374\377rWarning! this behaviour is unacceptable!");
				break;
			case 5:
				p_ptr->chp = -3;
				strcpy(p_ptr->died_from, "hypoxia");
				p_ptr->spam = 1;
				p_ptr->deathblow = 0;
				player_death(Ind);
				return;
			}
		}
		if (p_ptr->msg - last > 240 && p_ptr->spam) p_ptr->spam--;
		p_ptr->msgcnt = 0;
	}
	if (p_ptr->spam > 1 || p_ptr->muted) return;
#endif
	process_hooks(HOOK_CHAT, "d", Ind);

	if (++p_ptr->talk > 10) {
		imprison(Ind, JAIL_SPAM, "talking too much.");
		return;
	}

	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		Players[i]->talk = 0;
	}



	/* Special function '!:' at beginning of message sends to own party - sorry for hack, C. Blue */
	//if ((strlen(message) >= 1) && (message[0] == ':') && (!colon) && (p_ptr->party))
	if ((strlen(message) >= 2) && (message[0] == '!') && (message[1] == ':') && (colon) && (p_ptr->party)) {
		target = p_ptr->party;

#if 1 /* No private chat for invalid accounts ? */
		if (p_ptr->inval) {
			msg_print(Ind, "\377yYour account is not valid, wait for an admin to validate it.");
			return;
		}
#endif

		/* Send message to target party */
		if (p_ptr->mutedchat < 2) {
#ifdef GROUP_CHAT_NOCLUTTER
			/* prevent buffer overflow */
			message[MSG_LEN - 1 - 10 - strlen(sender)] = 0;
			party_msg_format_ignoring(Ind, target, "\375\377%c[(P) %s] %s", COLOUR_CHAT_PARTY, sender, message + 2);
#else
			/* prevent buffer overflow */
			message[MSG_LEN - 1 - 7 - strlen(sender) - strlen(parties[target].name)] = 0;
			party_msg_format_ignoring(Ind, target, "\375\377%c[%s:%s] %s", COLOUR_CHAT_PARTY, parties[target].name, sender, message + 2);
#endif
			//party_msg_format_ignoring(Ind, target, "\375\377%c[%s:%s] %s", COLOUR_CHAT_PARTY, parties[target].name, sender, message + 1);
		}

		/* Done */
		return;
	}

	/* '#:' at beginning of message sends to dungeon level - C. Blue */
	if ((strlen(message) >= 2) && (message[0] == '#') && (message[1] == ':') && (colon)) {
#if 1 /* No private chat for invalid accounts ? */
		if (p_ptr->inval) {
			msg_print(Ind, "\377yYour account is not valid, wait for an admin to validate it.");
			return;
		}
#endif

		if (!p_ptr->wpos.wz) {
#if 0
			msg_print(Ind, "You aren't in a dungeon or tower.");
#else /* Darkie's idea: */
			if (*(message + 2)) {
				censor_message = TRUE;
				censor_length = strlen(message + 2);

				/* prevent buffer overflow */
				message[MSG_LEN - 1 + 2 - strlen(p_ptr->name) - 9] = 0;
				msg_format_near(Ind, "\377%c%^s says: %s", COLOUR_CHAT, p_ptr->name, message + 2);
				msg_format(Ind, "\377%cYou say: %s", COLOUR_CHAT, message + 2);

				censor_message = FALSE;
				handle_punish(Ind, censor_punish);
			} else {
				msg_format_near(Ind, "\377%c%s clears %s throat.", COLOUR_CHAT, p_ptr->name, p_ptr->male ? "his" : "her");
				msg_format(Ind, "\377%cYou clear your throat.", COLOUR_CHAT);
			}
#endif
			return;
		}

		/* Send message to target floor */
		if (p_ptr->mutedchat < 2) {
			censor_message = TRUE;
			censor_length = strlen(message + 2);

			/* prevent buffer overflow */
			message[MSG_LEN - 1 + 2 - strlen(sender) - 6] = 0;
			floor_msg_format_ignoring(Ind, &p_ptr->wpos, "\375\377%c[%s] %s", COLOUR_CHAT_LEVEL, sender, message + 2);

			censor_message = FALSE;
			handle_punish(Ind, censor_punish);
		}

		/* Done */
		return;
	}

	/* '%:' at the beginning sends to self - mikaelh */
	if ((strlen(message) >= 2) && (message[0] == '%') && (message[1] == ':') && (colon)) {
		/* prevent buffer overflow */
		message[MSG_LEN - 1 + 2 - 8] = 0;
		/* Send message to self */
		msg_format(Ind, "\377o<%%>\377w %s", message + 2);

		/* Done */
		return;
	}

	/* '+:' at beginning of message sends reply to last player who sent us a private msg  - C. Blue */
	if ((strlen(message) >= 2) && (message[0] == '+') && (message[1] == ':') && colon) {
#if 1 /* No private chat for invalid accounts ? */
		if (p_ptr->inval) {
			msg_print(Ind, "\377yYour account is not valid, wait for an admin to validate it.");
			return;
		}
#endif

		/* Haven't received an initial private msg yet? */
		if (!strlen(p_ptr->reply_name)) {
			msg_print(Ind, "You haven't received any private message to reply to yet.");
			return;
		}

		if (p_ptr->mutedchat == 2) return;

		/* Forge private message */
		strcpy(message2, p_ptr->reply_name);
		strcat(message2, message + 1);
		strcpy(message, message2);
		colon = message + strlen(p_ptr->reply_name);

		/* Continue with forged private message */
	}

	/* '$:' at beginning of message sends to guild - C. Blue */
	if ((strlen(message) >= 2) && (message[0] == '$') && (message[1] == ':') && (colon) && (p_ptr->guild)) {
#if 1 /* No private chat for invalid accounts ? */
		if (p_ptr->inval) {
			msg_print(Ind, "\377yYour account is not valid, wait for an admin to validate it.");
			return;
		}
#endif

		/* Send message to guild party */
		if (p_ptr->mutedchat < 2) {
#ifdef GROUP_CHAT_NOCLUTTER
			/* prevent buffer overflow */
			message[MSG_LEN - 1 + 2 - strlen(sender) - 16] = 0;
			guild_msg_format(p_ptr->guild, "\375\377y[\377%c(G) %s\377y]\377%c %s", COLOUR_CHAT_GUILD, sender, COLOUR_CHAT_GUILD, message + 2);
#else
			/* prevent buffer overflow */
			message[MSG_LEN - 1 + 2 - strlen(sender) - strlen(guilds[p_ptr->guild].name) - 18] = 0;
			guild_msg_format(p_ptr->guild, "\375\377y[\377%c%s\377y:\377%c%s\377y]\377%c %s", COLOUR_CHAT_GUILD, guilds[p_ptr->guild].name, COLOUR_CHAT_GUILD, sender, COLOUR_CHAT_GUILD, message + 2);
#endif
		}

		/* Done */
		return;
	}


	/* Form a search string if we found a colon */
	if (colon) {
		if (p_ptr->mutedchat == 2) return;
#if 1 /* No private chat for invalid accounts ? */
		if (p_ptr->inval) {
			msg_print(Ind, "\377yYour account is not valid, wait for an admin to validate it.");
			return;
		}
#endif
		/* Copy everything up to the colon to the search string */
		strncpy(search, message, colon - message);

		/* Add a trailing NULL */
		search[colon - message] = '\0';
	} else if (p_ptr->mutedchat) return;

	/* Acquire length of search string */
	len = strlen(search);

	/* Look for a recipient who matches the search string */
	if (len) {
#ifdef TOMENET_WORLDS
		struct rplist *w_player;
#endif

		/* NAME_LOOKUP_LOOSE DESPERATELY NEEDS WORK */
		target = name_lookup_loose(Ind, search, TRUE, TRUE, TRUE);
#ifdef TOMENET_WORLDS
		if (!target && cfg.worldd_privchat) {
			w_player = world_find_player(search, 0);
			if (w_player) {
				/* prevent buffer overflow */
				message[MSG_LEN - strlen(p_ptr->name) - 7 - 8 - strlen(w_player->name) + target_raw_len] = 0;//8 are world server tax
				world_pmsg_send(p_ptr->id, p_ptr->name, w_player->name, colon + 1);
				msg_format(Ind, "\375\377s[%s:%s] %s", p_ptr->name, w_player->name, colon + 1);

				/* hack: assume that the target player will become the
				   one we want to 'reply' to, afterwards, if we don't
				   have a reply-to target yet. */
				if (!strlen(p_ptr->reply_name))
					strcpy(p_ptr->reply_name, w_player->name);

				return;
			}
		}
#endif

		/* Move colon pointer forward to next word */
/* no, this isn't needed and actually has annoying effects if you try to write smileys:
		while (*colon && (isspace(*colon) || *colon == ':')) colon++; */
/* instead, this is sufficient: */
		if (colon) colon++;

		/* lookup failed */
		if (!target) {
			/* Bounce message to player who sent it */
			//msg_format(Ind, "(no receipient for: %s)", colon);
#ifndef ARCADE_SERVER
			msg_print(Ind, "(Cannot match desired recipient)");
#endif
			/* Allow fake private msgs to be intercepted - Molt */
			exec_lua(0, "chat_handler()");

			/* Give up */
			return;
		}
	}

	/* Send to appropriate player */
	if (len && target > 0) {
		/* Set target player */
		q_ptr = Players[target];

		if (q_ptr->ignoring_chat == 2 && (!q_ptr->party || q_ptr->party != p_ptr->party)) {
			msg_print(Ind, "(That player is currently in *private* mode.)");
			return;
		}
		else if (!check_ignore(target, Ind)) {
			/* Remember sender, to be able to reply to him quickly */
#if 0
			strcpy(q_ptr->reply_name, sender);
#else /* use his account name instead, since it's possible now */
			if (!strcmp(sender, p_ptr->name)) strcpy(q_ptr->reply_name, p_ptr->accountname);
			else strcpy(q_ptr->reply_name, sender);
#endif

			/* prevent buffer overflow */
			message[MSG_LEN - strlen(sender) - 7 - strlen(q_ptr->name) + target_raw_len] = 0;

			/* Send message to target */
			msg_format(target, "\375\377g[%s:%s] %s", sender, q_ptr->name, colon);
			if ((q_ptr->page_on_privmsg ||
			    (q_ptr->page_on_afk_privmsg && q_ptr->afk)) &&
			    q_ptr->paging == 0)
				q_ptr->paging = 1;

			/* Also send back to sender */
			if (target != Ind)
				msg_format(Ind, "\375\377g[%s:%s] %s", sender, q_ptr->name, colon);

			/* Only display this message once now - mikaelh */
			if (q_ptr->afk && !player_list_find(p_ptr->afk_noticed, q_ptr->id)) {
				msg_format(Ind, "\377o%s seems to be Away From Keyboard.", q_ptr->name);
				player_list_add(&p_ptr->afk_noticed, q_ptr->id);
			}

#if 0
			/* hack: assume that the target player will become the
			   one we want to 'reply' to, afterwards, if we don't
			   have a reply-to target yet. */
			if (!p_ptr->reply_name || !strlen(p_ptr->reply_name))
				strcpy(p_ptr->reply_name, q_ptr->name);
#else
			/* hack: assume that the target player will become the
			   one we want to 'reply' to, afterwards.
			   This might get somewhat messy if we're privchatting
			   to two players at the same time, but so would
			   probably the other variant above. That one stays
			   true to the '+:' definition given in the guide though,
			   while this one diverges a bit. */
 #if 0
			strcpy(p_ptr->reply_name, q_ptr->name);
 #else /* use his account name instead, since it's possible now */
			strcpy(p_ptr->reply_name, q_ptr->accountname);
 #endif
#endif

			exec_lua(0, "chat_handler()");
			return;
		} else {
#if 1 /* keep consistent with paging */
			/* Tell the sender */
			msg_print(Ind, "(That player is currently ignoring you.)");
#endif
			return;
		}
	}

	/* Send to appropriate party */
	if (len && target < 0) {
		/* Can't send msg to party from 'outside' as non-admin */
		if (p_ptr->party != 0 - target && !admin) {
			msg_print(Ind, "You aren't in that party.");
			return;
		}

#ifdef GROUP_CHAT_NOCLUTTER
		/* prevent buffer overflow */
		message[MSG_LEN - strlen(sender) - 10] = 0;
		/* Send message to target party */
		party_msg_format_ignoring(Ind, 0 - target, "\375\377%c[(P) %s] %s", COLOUR_CHAT_PARTY, sender, colon);
#else
		/* prevent buffer overflow */
		message[MSG_LEN - 7 - strlen(sender) - strlen(parties[0 - target].name) + target_raw_len] = 0;
		party_msg_format_ignoring(Ind, 0 - target, "\375\377%c[%s:%s] %s", COLOUR_CHAT_PARTY, parties[target].name, sender, colon);
#endif

		//party_msg_format_ignoring(Ind, 0 - target, "\375\377%c[%s:%s] %s", COLOUR_CHAT_PARTY, parties[0 - target].name, sender, colon);

		/* Also send back to sender if not in that party */
		if (!player_in_party(0 - target, Ind)) {
			msg_format(Ind, "\375\377%c[%s:%s] %s",
			    COLOUR_CHAT_PARTY, parties[0 - target].name, sender, colon);
		}

		exec_lua(0, "chat_handler()");

		/* Done */
		return;
	}

	if (strlen(message) > 1) mycolor = (prefix(message, "}") && (color_char_to_attr(*(message + 1)) != -1)) ? 2 : 0;

	if (!Ind) c_n = 'y';
	/* Disabled this for now to avoid confusion. */
	else if (mycolor) c_n = *(message + 1);
	else {
		/* Dungeon Master / Dungeon Wizard have their own colour now :) */
		if (is_admin(p_ptr)) c_n = 'b';
		else if (p_ptr->total_winner) c_n = 'v';
		else if (p_ptr->ghost) c_n = 'r';
		else if (p_ptr->mode & MODE_EVERLASTING) c_n = 'B';
		else if (p_ptr->mode & MODE_PVP) c_n = COLOUR_MODE_PVP;
		else if (p_ptr->mode & MODE_NO_GHOST) c_n = 'D';
		else if (p_ptr->mode & MODE_HARD) c_n = 's';
		else c_n = 'W'; /* normal mode */
		
		/* Color the brackets of some players... (Enabled for PK) */
#ifdef KURZEL_PK
		if ((p_ptr->mode & MODE_HARD) && (p_ptr->mode & MODE_NO_GHOST))
			c_b = 'r'; /* hellish mode */
		else if (p_ptr->pkill & PKILL_SET) c_b = 'R'; //KURZEL_PK
		else c_b = c_n;
#endif
	}

	/* Admins have exclusive colour - the_sandman */
	if (c_n == 'b' && !admin) return;
#ifdef KURZEL_PK
	if (admin) c_b = 'b';
#endif


	censor_message = TRUE;

#ifdef TOMENET_WORLDS
	if (broadcast) {
		/* prevent buffer overflow */
		message[MSG_LEN - 1 - strlen(sender) - 12 + 11] = 0;

		snprintf(tmessage, sizeof(tmessage), "\375\377r[\377%c%s\377r]\377%c %s", c_n, sender, COLOUR_CHAT, message + 11);
		censor_length = strlen(message + 11);
	} else if (!me) {
		/* prevent buffer overflow */
		message[MSG_LEN - 1 - strlen(sender) - 8 + mycolor] = 0;

 #ifndef KURZEL_PK
		snprintf(tmessage, sizeof(tmessage), "\375\377%c[%s]\377%c %s", c_n, sender, COLOUR_CHAT, message + mycolor);
 #else
		snprintf(tmessage, sizeof(tmessage), "\375\377%c[\377%c%s\377%c]\377%c %s", c_b, c_n, sender, c_b, COLOUR_CHAT, message + mycolor);
 #endif
		censor_length = strlen(message + mycolor);
	} else {
		/* Why not... */
		if (strlen(message) > 4) mycolor = (prefix(&message[4], "}") && (color_char_to_attr(*(message + 5)) != -1)) ? 2 : 0;
		else {
			censor_message = FALSE;
			return;
		}
		if (mycolor) c_n = message[5];

		/* prevent buffer overflow */
		message[MSG_LEN - 1 - strlen(sender) - 10 + 4 + mycolor] = 0;
		if (me_gen) {
 #ifndef KURZEL_PK
			snprintf(tmessage, sizeof(tmessage), "\375\377%c[%s%s]", c_n, sender, message + 3 + mycolor);
 #else
			snprintf(tmessage, sizeof(tmessage), "\375\377%c[\377%c%s%s\377%c]", c_b, c_n, sender, message + 3 + mycolor, c_b);
 #endif
		} else {
 #ifndef KURZEL_PK
			snprintf(tmessage, sizeof(tmessage), "\375\377%c[%s %s]", c_n, sender, message + 4 + mycolor);
 #else
			snprintf(tmessage, sizeof(tmessage), "\375\377%c[\377%c%s %s\377%c]", c_b, c_n, sender, message + 4 + mycolor, c_b);
 #endif
			censor_length = strlen(message + 4 + mycolor) + 1;
		}
	}

 #if 0
  #if 0
	if ((broadcast && cfg.worldd_broadcast) || (!broadcast && cfg.worldd_pubchat))
	    && !(len && (target != 0) && !cfg.worldd_privchat)) /* privchat = to party or to person */
		world_chat(p_ptr->id, tmessage);	/* no ignores... */
  #else
	/* Incoming chat is now filtered instead of outgoing which allows IRC relay to get public chat messages from worldd - mikaelh */
	world_chat(p_ptr->id, tmessage);	/* no ignores... */
  #endif
 #else /* Remove current redundancy. Make worldd_pubchat decide if we broadcast all our chat out there or not. */
	/* in case privchat wasn't handled above (because it's disabled),
	   exempt it here so we only process real chat/broadcasts */
	if (!(!cfg.worldd_privchat && len && target != 0)) {
		if (broadcast && cfg.worldd_broadcast) {
			world_chat(0, tmessage); /* can't ignore */
		} else if (!broadcast && cfg.worldd_pubchat) {
			world_chat(p_ptr->id, tmessage);
		}
	}
 #endif

	for(i = 1; i <= NumPlayers; i++) {
		q_ptr = Players[i];

		if (!admin) {
			if (check_ignore(i, Ind)) continue;
			if (q_ptr->ignoring_chat) continue;
			if (!broadcast && (p_ptr->limit_chat || q_ptr->limit_chat) &&
			    !inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;
		}
		msg_print(i, tmessage);
	}
#else
	/* Send to everyone */
	for (i = 1; i <= NumPlayers; i++) {
		q_ptr = Players[i];

		if (!admin) {
			if (check_ignore(i, Ind)) continue;
			if (q_ptr->ignoring_chat) continue;
			if (!broadcast && (p_ptr->limit_chat || q_ptr->limit_chat) &&
			    !inarea(&p_ptr->wpos, &q_ptr->wpos)) continue;
		}

		/* Send message */
		if (broadcast) {
			/* prevent buffer overflow */
			message[MSG_LEN - 1 - strlen(sender) - 12 + 11] = 0;

			censor_length = strlen(message + 11);
			msg_format(i, "\375\377r[\377%c%s\377r]\377%c %s", c_n, sender, COLOUR_CHAT, message + 11);
		} else if (!me) {
			/* prevent buffer overflow */
			message[MSG_LEN - 1 - strlen(sender) - 12 + mycolor] = 0;

			censor_length = strlen(message + mycolor);
 #ifndef KURZEL_PK
			msg_format(i, "\375\377%c[%s]\377%c %s", c_n, sender, COLOUR_CHAT, message + mycolor);
 #else
			msg_format(i, "\375\377%c[\377%c%s\377%c]\377%c %s", c_b, c_n, sender, c_b, COLOUR_CHAT, message + mycolor);
 #endif
			/* msg_format(i, "\375\377%c[%s] %s", Ind ? 'B' : 'y', sender, message); */
		}
		else {
			/* prevent buffer overflow */
			message[MSG_LEN - 1 - strlen(sender) - 1 + 4] = 0;

			censor_length = strlen(message + 4);
			if (me_gen)
				msg_format(i, "%s%s", sender, message + 3);
			else
				msg_format(i, "%s %s", sender, message + 4);
		}
	}
#endif

	censor_message = FALSE;
	p_ptr->warning_chat = 1;
	handle_punish(Ind, censor_punish);

	exec_lua(0, "chat_handler()");

}
/* Console talk is automatically sent by 'Server Admin' which is treated as an admin */
static void console_talk_aux(char *message) {
 	int i;
	cptr sender = "Server Admin";
	bool me = FALSE, log = TRUE;
	char c_n = 'y'; /* colours of sender name and of brackets (unused atm) around this name */
#ifdef KURZEL_PK
	char c_b = 'y';
#endif
	bool broadcast = FALSE;

#ifdef TOMENET_WORLDS
	char tmessage[MSG_LEN];		/* TEMPORARY! We will not send the name soon */
#else
	player_type *q_ptr;
#endif

	/* tBot's stuff */
	/* moved here to allow tbot to see fake pvt messages. -Molt */
	strncpy(last_chat_owner, sender, NAME_LEN);
	strncpy(last_chat_line, message, MSG_LEN);

	/* no big brother */
	if (cfg.log_u && log) s_printf("[%s] %s\n", sender, message);

	/* Special - shutdown command (for compatibility) */
	if (prefix(message, "@!shutdown")) {
		/*world_reboot();*/
		shutdown_server();
		return;
	}

	if (message[0] == '/' ) {
		if (!strncmp(message, "/me ", 4)) me = TRUE;
		else if (!strncmp(message, "/broadcast ", 11)) broadcast = TRUE;
		else return;
	}

	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		Players[i]->talk = 0;
	}

	censor_message = TRUE;

#ifdef TOMENET_WORLDS
	if (broadcast) {
		snprintf(tmessage, sizeof(tmessage), "\375\377r[\377%c%s\377r]\377%c %s", c_n, sender, COLOUR_CHAT, message + 11);
		censor_length = strlen(message + 11);
	} else if (!me) {
#ifndef KURZEL_PK
		snprintf(tmessage, sizeof(tmessage), "\375\377%c[%s]\377%c %s", c_n, sender, COLOUR_CHAT, message);
#else
		snprintf(tmessage, sizeof(tmessage), "\375\377%c[\377%c%s\377%c]\377%c %s", c_b, c_n, sender, c_b, COLOUR_CHAT, message);
#endif
		censor_length = strlen(message);
	} else {
#ifndef KURZEL_PK
		snprintf(tmessage, sizeof(tmessage), "\375\377%c[%s %s]", c_n, sender, message + 4);
#else
		snprintf(tmessage, sizeof(tmessage), "\375\377%c[\377%c%s %s\377%c]", c_b, c_n, sender, message + 4, c_b);
#endif
		censor_length = strlen(message + 4);
	}

#else
	/* Send to everyone */
	for (i = 1; i <= NumPlayers; i++) {
		q_ptr = Players[i];

		/* Send message */
		if (broadcast) {
			censor_length = strlen(message + 11);
			msg_format(i, "\375\377r[\377%c%s\377r]\377%c %s", c_n, sender, COLOUR_CHAT, message + 11);
		} else if (!me) {
			censor_length = strlen(message);
#ifndef KURZEL_PK
			msg_format(i, "\375\377%c[%s]\377%c %s", c_n, sender, COLOUR_CHAT, message);
#else
			msg_format(i, "\375\377%c[\377%c%s\377%c]\377%c %s", c_b, c_n, sender, c_b, COLOUR_CHAT, message);
#endif
		}
		else {
			censor_length = strlen(message + 4);
			msg_format(i, "%s %s", sender, message + 4);
		}
	}
#endif

	censor_message = FALSE;

	exec_lua(0, "chat_handler()");
}

/* Check for swear words, censor + punish */
int handle_censor(char *message) {
#ifdef CENSOR_SWEARING
	if (censor_swearing) return censor(message);
#endif
	return 0;
}
void handle_punish(int Ind, int level) {
	switch (level) {
	case 0:
		break;
	case 1:
		msg_print(Ind, "Please do not swear.");
		break;
	default:
		imprison(Ind, level * JAIL_SWEARING, "swearing");
	}
}

/* toggle AFK mode off if it's currently on, also reset idle time counter for in-game character.
   required for cloaking mode! - C. Blue */
void un_afk_idle(int Ind) {
	Players[Ind]->idle_char = 0;
	if (Players[Ind]->afk && !(Players[Ind]->admin_dm && cfg.secret_dungeon_master)) toggle_afk(Ind, "");
	stop_cloaking(Ind);
}

/*
 * toggle AFK mode on/off.	- Jir -
 */
//#define ALLOW_AFK_COLOURCODES
#ifdef ALLOW_AFK_COLOURCODES
void toggle_afk(int Ind, char *msg0)
#else
void toggle_afk(int Ind, char *msg)
#endif
{
	player_type *p_ptr = Players[Ind];
	char afk[MAX_CHARS];
	int i = 0;
#ifdef ALLOW_AFK_COLOURCODES
	char msg[MAX_CHARS], *m = msg0;

	/* strip colour codes and cap message at len 60 */
	while ((*m) && i < 60) {
		if (*m == '\377') {
			m++;
			if (*m == '\377') {
				msg[i++] = '{';
			}
		} else msg[i++] = *m;
		m++;
	}
	msg[i] = 0;
#endif

	/* don't go un-AFK from auto-retaliation */
	if (p_ptr->afk && p_ptr->auto_retaliaty) return;

	/* don't go AFK while shooting-till-kill (target dummy, maybe ironwing ;)) */
	if (!p_ptr->afk && p_ptr->shooting_till_kill) return;

	strcpy(afk, "");

	if (p_ptr->afk && !msg[0]) {
		if (strlen(p_ptr->afk_msg) == 0)
			msg_print(Ind, "AFK mode is turned \377GOFF\377w.");
		else
			msg_format(Ind, "AFK mode is turned \377GOFF\377w. (%s\377w)", p_ptr->afk_msg);
		if (!p_ptr->admin_dm) {
			if (strlen(p_ptr->afk_msg) == 0)
				snprintf(afk, sizeof(afk), "\377%c%s has returned from AFK.", COLOUR_AFK, p_ptr->name);
			else
				snprintf(afk, sizeof(afk), "\377%c%s has returned from AFK. (%s\377%c)", COLOUR_AFK, p_ptr->name, p_ptr->afk_msg, COLOUR_AFK);
		}
		p_ptr->afk = FALSE;

		/* Clear everyone's afk_noticed */
		for (i = 1; i <= NumPlayers; i++)
			player_list_del(&Players[i]->afk_noticed, p_ptr->id);
	} else {
		/* hack: lose health tracker so we actually get to see the 'AFK'
		   (for example we might've attacked the target dummy before). */
		health_track(Ind, 0);

		/* stop every major action */
		disturb(Ind, 1, 1); /* ,1) = keep resting! */

		strncpy(p_ptr->afk_msg, msg, MAX_CHARS - 1);
		p_ptr->afk_msg[MAX_CHARS - 1] = '\0';

		if (strlen(p_ptr->afk_msg) == 0)
			msg_print(Ind, "AFK mode is turned \377rON\377w.");
		else
			msg_format(Ind, "AFK mode is turned \377rON\377w. (%s\377w)", p_ptr->afk_msg);
		if (!p_ptr->admin_dm) {
			if (strlen(p_ptr->afk_msg) == 0)
				snprintf(afk, sizeof(afk), "\377%c%s seems to be AFK now.", COLOUR_AFK, p_ptr->name);
			else
				snprintf(afk, sizeof(afk), "\377%c%s seems to be AFK now. (%s\377%c)", COLOUR_AFK, p_ptr->name, p_ptr->afk_msg, COLOUR_AFK);
		}
		p_ptr->afk = TRUE;

#if 0 /* not anymore */
		/* actually a hint for newbie rogues couldn't hurt */
		if (p_ptr->tim_blacklist)
			msg_print(Ind, "\376\377yNote: Your blacklist timer won't decrease while AFK.");
#endif

		/* still too many starvations, so give a warning - C. Blue */
		if (p_ptr->food < PY_FOOD_ALERT) {
			p_ptr->paging = 6; /* add some beeps, too - mikaelh */
			msg_print(Ind, "\374\377RWARNING: Going AFK while hungry or weak can result in starvation! Eat first!");
		}

		/* Since Mark's mimic died in front of Minas XBM due to this.. - C. Blue */
		if (p_ptr->tim_wraith) {
			p_ptr->paging = 6;
			msg_print(Ind, "\374\377RWARNING: Going AFK in wraithform is very dangerous because wraithform impairs auto-retaliation!");
		}
	}

	/* Replaced msg_broadcast by this, to allow /ignore and /ic */
	/* Tell every player */
	for (i = 1; i <= NumPlayers; i++) {
		/* Skip disconnected players */
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (check_ignore(i, Ind)) continue;
		if (Players[i]->no_afk_msg) continue;
		if (Players[i]->ignoring_chat && !(p_ptr->party && player_in_party(p_ptr->party, i))) continue;

		/* Skip himself */
		if (i == Ind) continue;

		/* Tell this one */
		msg_print(i, afk);
	}

	p_ptr->redraw |= PR_EXTRA;
	redraw_stuff(Ind);
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
bool is_a_vowel(int ch) {
	switch (ch) {
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
int name_lookup_loose(int Ind, cptr name, u16b party, bool include_account_names, bool quiet) {
	int i, j, len, target = 0;
	player_type *q_ptr, *p_ptr;
	cptr problem = "";
	bool party_online;

	p_ptr = Players[Ind];

	/* don't waste time */
	if (p_ptr == (player_type*)NULL) return(0);

	/* Acquire length of search string */
	len = strlen(name);

	/* Look for a recipient who matches the search string */
	if (len) {
		/* Check players */
		for (i = 1; i <= NumPlayers; i++) {
			/* Check this one */
			q_ptr = Players[i];

			/* Skip if disconnected */
			if (q_ptr->conn == NOT_CONNECTED) continue;

			/* let admins chat */
			if (q_ptr->admin_dm && !q_ptr->admin_dm_chat && !is_admin(p_ptr)
			    /* Hack: allow the following accounts nasty stuff (e.g., spam the DMs!) */
			    && strcasecmp(p_ptr->accountname, "kurzel")
			    && strcasecmp(p_ptr->accountname, "moltor")
			    && strcasecmp(p_ptr->accountname, "the_sandman")
			    && strcasecmp(p_ptr->accountname, "faith")
			    && strcasecmp(p_ptr->accountname, "mikaelh")
			    && strcasecmp(p_ptr->accountname, "c. blue")) continue;

			/* Check name */
			if (!strncasecmp(q_ptr->name, name, len)) {
				/* Set target if not set already */
				if (!target) {
					target = i;
				} else {
					/* Matching too many people */
					problem = "players";
				}

				/* Check for exact match */
				if (len == (int)strlen(q_ptr->name)) {
					/* Never a problem */
					target = i;
					problem = "";

					/* Finished looking */
					break;
				}
			}
		}

		/* Then check accounts */
		if (include_account_names && !target) for (i = 1; i <= NumPlayers; i++) {
			/* Check this one */
			q_ptr = Players[i];

			/* Skip if disconnected */
			if (q_ptr->conn == NOT_CONNECTED) continue;

			/* let admins chat */
			if (q_ptr->admin_dm && !q_ptr->admin_dm_chat && !is_admin(p_ptr)
			    /* Hack: allow the following accounts nasty stuff (e.g., spam the DMs!) */
			    && strcasecmp(p_ptr->accountname, "kurzel")
			    && strcasecmp(p_ptr->accountname, "moltor")
			    && strcasecmp(p_ptr->accountname, "the_sandman")
			    && strcasecmp(p_ptr->accountname, "faith")
			    && strcasecmp(p_ptr->accountname, "mikaelh")
			    && strcasecmp(p_ptr->accountname, "c. blue")) continue;

			/* Check name */
			if (!strncasecmp(q_ptr->accountname, name, len)) {
				/* Set target if not set already */
				if (!target) {
					target = i;
				} else {
					/* Matching too many people */
					problem = "players";
				}

				/* Check for exact match */
				if (len == (int)strlen(q_ptr->accountname)) {
					/* Never a problem */
					target = i;
					problem = "";

					/* Finished looking */
					break;
				}
			}
		}

		/* Check parties */
		if (party && !target) {
			for (i = 1; i < MAX_PARTIES; i++) {
				/* Skip if empty */
				if (!parties[i].members) continue;

				/* Check that the party has players online - mikaelh */
				party_online = FALSE;
				for (j = 1; j <= NumPlayers; j++) {
					/* Check this one */
					q_ptr = Players[j];

					/* Skip if disconnected */
					if (q_ptr->conn == NOT_CONNECTED) continue;

					/* Check if the player belongs to this party */
					if (q_ptr->party == i) {
						party_online = TRUE;
						break;
					}
				}
				if (!party_online) continue;

				/* Check name */
				if (!strncasecmp(parties[i].name, name, len)) {
					/* Set target if not set already */
					if (!target) {
						target = 0 - i;
					} else {
						/* Matching too many parties */
						problem = "parties";
					}

					/* Check for exact match */
					if (len == (int)strlen(parties[i].name)) {
						/* Never a problem */
						target = 0 - i;
						problem = "";

						/* Finished looking */
						break;
					}
				}
			}
		}
	}

	/* Check for recipient set but no match found */
	if ((len && !target) || (target < 0 && !party)) {
		/* Send an error message */
		if (!quiet) msg_format(Ind, "Could not match name '%s'.", name);

		/* Give up */
		return 0;
	}

	/* Check for multiple recipients found */
	if (strlen(problem)) {
		/* Send an error message */
		if (!quiet) msg_format(Ind, "'%s' matches too many %s.", name, problem);

		/* Give up */
		return 0;
	}

	/* Success */
	return target;
}

/* copy/pasted from name_lookup_loose(), just without being loose.. */
int name_lookup(int Ind, cptr name, u16b party, bool include_account_names, bool quiet) {
	int i, j, len, target = 0;
	player_type *q_ptr, *p_ptr;
	bool party_online;

	p_ptr = Players[Ind];

	/* don't waste time */
	if (p_ptr == (player_type*)NULL) return(0);

	/* Acquire length of search string */
	len = strlen(name);

	/* Look for a recipient who matches the search string */
	if (len) {
		/* Check players */
		for (i = 1; i <= NumPlayers; i++) {
			/* Check this one */
			q_ptr = Players[i];

			/* Skip if disconnected */
			if (q_ptr->conn == NOT_CONNECTED) continue;

			/* let admins chat */
			if (q_ptr->admin_dm && !q_ptr->admin_dm_chat && !is_admin(p_ptr)
			    /* Hack: allow the following accounts nasty stuff (e.g., spam the DMs!) */
			    && strcasecmp(p_ptr->accountname, "kurzel")
			    && strcasecmp(p_ptr->accountname, "moltor")
			    && strcasecmp(p_ptr->accountname, "the_sandman")
			    && strcasecmp(p_ptr->accountname, "faith")
			    && strcasecmp(p_ptr->accountname, "mikaelh")
			    && strcasecmp(p_ptr->accountname, "c. blue")) continue;

			/* Check name */
			if (!strcasecmp(q_ptr->name, name)) {
				/* Never a problem */
				target = i;

				/* Finished looking */
				break;
			}
		}

		/* Then check accounts */
		if (include_account_names && !target) for (i = 1; i <= NumPlayers; i++) {
			/* Check this one */
			q_ptr = Players[i];

			/* Skip if disconnected */
			if (q_ptr->conn == NOT_CONNECTED) continue;

			/* let admins chat */
			if (q_ptr->admin_dm && !q_ptr->admin_dm_chat && !is_admin(p_ptr)
			    /* Hack: allow the following accounts nasty stuff (e.g., spam the DMs!) */
			    && strcasecmp(p_ptr->accountname, "kurzel")
			    && strcasecmp(p_ptr->accountname, "moltor")
			    && strcasecmp(p_ptr->accountname, "the_sandman")
			    && strcasecmp(p_ptr->accountname, "faith")
			    && strcasecmp(p_ptr->accountname, "mikaelh")
			    && strcasecmp(p_ptr->accountname, "c. blue")) continue;

			/* Check name */
			if (!strcasecmp(q_ptr->accountname, name)) {
				/* Never a problem */
				target = i;

				/* Finished looking */
				break;
			}
		}

		/* Check parties */
		if (party && !target) {
			for (i = 1; i < MAX_PARTIES; i++) {
				/* Skip if empty */
				if (!parties[i].members) continue;

				/* Check that the party has players online - mikaelh */
				party_online = FALSE;
				for (j = 1; j <= NumPlayers; j++) {
					/* Check this one */
					q_ptr = Players[j];

					/* Skip if disconnected */
					if (q_ptr->conn == NOT_CONNECTED) continue;

					/* Check if the player belongs to this party */
					if (q_ptr->party == i) {
						party_online = TRUE;
						break;
					}
				}
				if (!party_online) continue;

				/* Check name */
				if (!strcasecmp(parties[i].name, name)) {
					/* Never a problem */
					target = 0 - i;

					/* Finished looking */
					break;
				}
			}
		}
	}

	/* Check for recipient set but no match found */
	if ((len && !target) || (target < 0 && !party)) {
		/* Send an error message */
		if (!quiet) msg_format(Ind, "Could not match name '%s'.", name);

		/* Give up */
		return 0;
	}

	/* Success */
	return target;
}

/*
 * Convert bracer '{' into '\377'
 */
void bracer_ff(char *buf) {
	int i, len = strlen(buf);

	for (i = 0; i < len; i++) {
		if (buf[i] == '{') buf[i] = '\377';
	}
}

/*
 * make strings from worldpos '-1550ft of (17,15)'	- Jir -
 */
char *wpos_format(int Ind, worldpos *wpos) {
	int i = Ind, n, d = 0;
	cptr desc = "";
	bool ville = istown(wpos) && !isdungeontown(wpos);
	dungeon_type *d_ptr = NULL;

	/* Hack for Valinor originally */
	if (i < 0) i = -i;
	if (wpos->wz > 0 && (d_ptr = wild_info[wpos->wy][wpos->wx].tower))
		d = d_ptr->type;
	if (wpos->wz < 0 && (d_ptr = wild_info[wpos->wy][wpos->wx].dungeon))
		d = d_ptr->type;
	if (!wpos->wz && ville)
		for (n = 0; n < numtowns; n++)
			if (town[n].x == wpos->wx && town[n].y == wpos->wy) {
				desc = town_profile[town[n].type].name;
				break;
			}
	if (!strcmp(desc, "")) ville = FALSE;

	if (!i || Players[i]->depth_in_feet) {
		if (Ind >= 0 || (!d && !ville)) {
			return (format("%dft of (%d,%d)", wpos->wz * 50, wpos->wx, wpos->wy));
		} else
			if (!ville)
				return (format("%dft %s", wpos->wz * 50, get_dun_name(wpos->wx, wpos->wy, (wpos->wz > 0), d_ptr, 0, FALSE)));
			else
				return (format("%s", desc));
	} else {
		if (Ind >= 0 || (!d && !ville)) {
			return (format("Lv %d of (%d,%d)", wpos->wz, wpos->wx, wpos->wy));
		} else
			if (!ville)
				return (format("Lv %d %s", wpos->wz, get_dun_name(wpos->wx, wpos->wy, (wpos->wz > 0), d_ptr, 0, FALSE)));
			else
				return (format("%s", desc));
	}
}
char *wpos_format_compact(int Ind, worldpos *wpos) {
	int n;
	cptr desc = "";
	bool ville = istown(wpos) && !isdungeontown(wpos);

	if (!wpos->wz && ville)
		for (n = 0; n < numtowns; n++)
			if (town[n].x == wpos->wx && town[n].y == wpos->wy) {
				desc = town_profile[town[n].type].name;
				break;
			}
	if (!strcmp(desc, "")) ville = FALSE;

	if (ville) return (format("%s", desc));
	else {
		if (Players[Ind]->depth_in_feet)
			return (format("%dft (%d,%d)", wpos->wz * 50, wpos->wx, wpos->wy));
		else
			return (format("Lv %d (%d,%d)", wpos->wz, wpos->wx, wpos->wy));
	}
}


byte count_bits(u32b array) {
	byte k = 0, i;

	if (array)
		for (i = 0; i < 32; i++)
			if (array & (1U << i)) k++;

	return k;
}

/*
 * Find a player
 */
int get_playerind(char *name) {
	int i;

	if (name == (char*)NULL) return(-1);
	for(i = 1; i <= NumPlayers; i++) {
		if(Players[i]->conn == NOT_CONNECTED) continue;
		if(!stricmp(Players[i]->name, name)) return(i);
	}
	return(-1);
}
int get_playerind_loose(char *name) {
	int i, len = strlen(name);

	if (len == 0) return(-1);
	if (name == (char*)NULL) return(-1);
	for(i = 1; i <= NumPlayers; i++) {
		if(Players[i]->conn == NOT_CONNECTED) continue;
		if (!strncasecmp(Players[i]->name, name, len)) return(i);
	}
	return(-1);
}

int get_playerslot_loose(int Ind, char *iname) {
	int i, j;
	char o_name[ONAME_LEN], i_name[ONAME_LEN];

	if (iname == (char*)NULL) return(-1);
	if ((*iname) == 0) return(-1);

	for (j = 0; iname[j]; j++) i_name[j] = tolower(iname[j]);
	i_name[j] = 0;

	for (i = 0; i < INVEN_TOTAL; i++) {
		if (!Players[Ind]->inventory[i].k_idx) continue;

		object_desc(0, o_name, &Players[Ind]->inventory[i], FALSE, 3+16+32);
		for (j = 0; o_name[j]; j++) o_name[j] = tolower(o_name[j]);

		if (strstr(o_name, i_name)) return(i);
	}

	return(-1);
}

/*
 * Tell the player of the floor feeling.	- Jir -
 * NOTE: differs from traditional 'boring' etc feeling!
 * NOTE: added traditional feelings, to warn of dangers - C. Blue
 */
bool show_floor_feeling(int Ind, bool dungeon_feeling) {
	player_type *p_ptr = Players[Ind];
	worldpos *wpos = &p_ptr->wpos;
	struct dungeon_type *d_ptr = getdungeon(wpos);
	dun_level *l_ptr = getfloor(wpos);
	bool felt = FALSE;

	/* Hack for Valinor - C. Blue */
	if (in_valinor(wpos)) {
		msg_print(Ind, "\374\377gYou have a wonderful feeling of peace...");
		return TRUE;
	}

	/* XXX devise a better formula */
	if (!in_irondeepdive(&p_ptr->wpos) && (p_ptr->lev * ((p_ptr->lev >= 40) ? 3 : 2) + 5 < getlevel(wpos))) {
		msg_print(Ind, "\374\377rYou feel an imminent danger!");
		felt = TRUE;
	}

#ifdef DUNGEON_VISIT_BONUS
	if (dungeon_feeling && d_ptr && dungeon_bonus[d_ptr->id]
	    && !(d_ptr->flags3 & DF3_NO_DUNGEON_BONUS)) {
		felt = TRUE;
		switch (dungeon_bonus[d_ptr->id]) {
		case 3: msg_print(Ind, "\377UThis place has not been explored in ages."); break;
		case 2: msg_print(Ind, "\377UThis place has not been explored in a long time."); break;
		case 1: msg_print(Ind, "\377UThis place has not been explored recently."); break;
		}
	}
#endif

	if (!l_ptr) return(felt);

#ifdef EXTRA_LEVEL_FEELINGS
	/* Display extra traditional feeling to warn of dangers. - C. Blue
	   Note: Only display ONE feeling, thereby setting priorities here.
	   Note: Don't display feelings in Training Tower (NO_DEATH). */
	if ((!p_ptr->distinct_floor_feeling && !is_admin(p_ptr)) || (d_ptr->flags2 & DF2_NO_DEATH) ||
	    in_pvparena(wpos) || isdungeontown(wpos)) {
		//msg_print(Ind, "\376\377yLooks like any other level..");
		//msg_print(Ind, "\377ypfft");
	}
	else if (l_ptr->flags2 & LF2_OOD_HI) {
		msg_print(Ind, "\374\377yWhat a terrifying place..");
		felt = TRUE;
	} else if ((l_ptr->flags2 & LF2_VAULT_HI) &&
		(l_ptr->flags2 & LF2_OOD)) {
		msg_print(Ind, "\374\377yWhat a terrifying place..");
		felt = TRUE;
	} else if ((l_ptr->flags2 & LF2_VAULT_OPEN) || // <- TODO: implement :/
		 ((l_ptr->flags2 & LF2_VAULT) && (l_ptr->flags2 & LF2_OOD_FREE))) {
		msg_print(Ind, "\374\377yYou sense an air of danger.."); //this is pretty rare actually, maybe not worth it?
		felt = TRUE;
	} else if (l_ptr->flags2 & LF2_VAULT) {
		msg_print(Ind, "\374\377yFeels somewhat dangerous around here..");
		felt = TRUE;
	} else if (l_ptr->flags2 & LF2_PITNEST_HI) {
		msg_print(Ind, "\374\377yFeels somewhat dangerous around here..");
		felt = TRUE;
	} else if (l_ptr->flags2 & LF2_OOD_FREE) {
		msg_print(Ind, "\374\377yThere's a sensation of challenge..");
		felt = TRUE;
	} /*	else if (l_ptr->flags2 & LF2_PITNEST) { //maybe enable, maybe too cheezy
		msg_print(Ind, "\374\377yYou feel your luck is turning..");
		felt = TRUE;
	} */ else if (l_ptr->flags2 & LF2_UNIQUE) {
		msg_print(Ind, "\374\377yThere's a special feeling about this place..");
		felt = TRUE;
	} /* else if (l_ptr->flags2 & LF2_ARTIFACT) { //probably too cheezy, if not then might need combination with threat feelings above
		msg_print(Ind, "\374\377y");
		felt = TRUE;
	} *//*	else if (l_ptr->flags2 & LF2_ITEM_OOD) { //probably too cheezy, if not then might need combination with threat feelings above
		msg_print(Ind, "\374\377y");
		felt = TRUE;
	} */ else {
		msg_print(Ind, "\374\377yWhat a boring place..");
		felt = TRUE;
	}
#endif

	/* Special feeling for dungeon bosses in IDDC */
	if ((p_ptr->distinct_floor_feeling || is_admin(p_ptr)) &&
	    in_irondeepdive(wpos) && (l_ptr->flags2 & LF2_DUN_BOSS))
		msg_print(Ind, "\374\377UYou feel a commanding presence..");


	/* Hack^2 -- display the 'feeling' */
	if (l_ptr->flags1 & LF1_NO_MAGIC) {
		msg_print(Ind, "\377oYou feel a suppressive air.");
		/* Automatically dis/re-enable wraith form */
		p_ptr->update |= PU_BONUS;
	}
	if (l_ptr->flags1 & LF1_NO_GENO)
//sounds as if it's good for the player		msg_print(Ind, "\377oYou have a feeling of peace...");
		msg_print(Ind, "\377oThe air seems distorted.");
	if (l_ptr->flags1 & LF1_NO_MAP)
		msg_print(Ind, "\377oYou lose all sense of direction...");
	if (l_ptr->flags1 & LF1_NO_MAGIC_MAP)
		msg_print(Ind, "\377oYou feel like a stranger...");
	if (l_ptr->flags1 & LF1_NO_DESTROY)
		msg_print(Ind, "\377oThe walls here seem very solid.");
	if (l_ptr->flags1 & LF1_NO_GHOST)
		msg_print(Ind, "\377oYou feel that your life hangs in the balance!"); //credits to Moltor actually, ha!:)
#if 0
	if (l_ptr->flags1 & DF1_NO_RECALL)
		msg_print(Ind, "\377oThere is strong magic enclosing this dungeon.");
#endif
	/* Can leave IRONMAN? */
	if ((l_ptr->flags1 & LF1_IRON_RECALL || ((d_ptr->flags1 & DF1_FORCE_DOWN) && d_ptr->maxdepth == ABS(p_ptr->wpos.wz)))
	    && !(d_ptr->flags2 & DF2_NO_EXIT_WOR))
		msg_print(Ind, "\374\377gYou don't sense a magic barrier here!");

	return(l_ptr->flags1 & LF1_FEELING_MASK ? TRUE : felt);
}

/*
 * Given item name as string, return the index in k_info array. Name
 * must exactly match (look out for commas and the like!), or else 0 is 
 * returned. Case doesn't matter. -DG-
 */

int test_item_name(cptr name) {
	int i;

	/* Scan the items */
	for (i = 1; i < max_k_idx; i++) {
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
s32b bst(s32b what, s32b t) {
#if 0 /* TIMEREWRITE */
	s32b turns = t + (10 * DAY_START);

	switch (what) {
		case MINUTE:
			return ((turns / 10 / MINUTE) % 60);
		case HOUR:
			return ((turns / 10 / HOUR) % 24);
		case DAY:
			return ((turns / 10 / DAY) % 365);
		case YEAR:
			return ((turns / 10 / YEAR));
		default:
			return (0);
	}
#else
	s32b turns = t;

	if (what == MINUTE)
		return ((turns / MINUTE) % 60);
	else if (what == HOUR)
		return ((turns / HOUR) % 24);
	else if (what == DAY)
		return ((((turns / DAY) + START_DAY) % 365));
	else if (what == YEAR)
		return ((turns / YEAR) + START_YEAR);
	else
		return (0);
#endif
}

cptr get_month_name(int day, bool full, bool compact) {
	int i = 8;
	static char buf[40];

	/* Find the period name */
	while ((i > 0) && (day < month_day[i]))
		i--;

	switch (i) {
	/* Yestare/Mettare */
	case 0:
	case 8: {
		char buf2[20];

		snprintf(buf2, 20, "%s", get_day(day + 1));
		if (full) snprintf(buf, 40, "%s (%s day)", month_name[i], buf2);
		else snprintf(buf, 40, "%s", month_name[i]);
		break;
	}
	/* 'Normal' months + Enderi */
	default: {
		char buf2[20];
		char buf3[20];

		snprintf(buf2, 20, "%s", get_day(day + 1 - month_day[i]));
		snprintf(buf3, 20, "%s", get_day(day + 1));

		if (full) snprintf(buf, 40, "%s day of %s (%s day)", buf2, month_name[i], buf3);
		else if (compact) snprintf(buf, 40, "%s day of %s", buf2, month_name[i]);
		else snprintf(buf, 40, "%s %s", buf2, month_name[i]);
		break;
		}
	}

	return (buf);
}

cptr get_day(int day) {
	static char buf[20];
	cptr p = "th";

	if ((day / 10) == 1) ;
	else if ((day % 10) == 1) p = "st";
	else if ((day % 10) == 2) p = "nd";
	else if ((day % 10) == 3) p = "rd";

	snprintf(buf, 20, "%d%s", day, p);
	return (buf);
}

/* Fuzzy: Will allow one-off colour,
   Compact: Will increase stack size by factor [100] for all particular colour ranges. */
int gold_colour(int amt, bool fuzzy, bool compact) {
	int i, unit = 1;

	//for (i = amt; i > 99; i >>= 1, unit++) /* naught */; --old

#if 1
	/* special: perform pre-rounding, to achieve smoother looking transitions */
	for (i = 1; i * 10 <= amt; i *= 10);
	/* keep first 2 digits, discard the rest */
	if (i >= 10) amt = (amt / (i / 10)) * (i / 10);
#endif

	if (compact)
#if 0 /* scale? */
		for (i = amt / 100; i >= 40; i = (i * 2) / 3, unit++) /* naught */;
#else /* stretch? */
 #if 0 /* was ok, but due to natural income flow many lower tier colours are sort of skipped */
		//for (i = amt; i >= 60; i = (i * 2) / 4, unit++) /* naught */;
 #else /* seems to scale better, allowing all colours to shine */
		for (i = amt; i >= 60; i = (i * 4) / 9, unit++) /* naught */;
 #endif
#endif
	else
		for (i = amt; i >= 40; i = (i * 2) / 3, unit++) /* naught */;
	if (fuzzy)
		//unit = unit - 1 + rand_int(3);
		//unit = unit - 1 + (rand_int(5) + 2) / 3;
		unit = unit - 1 + (rand_int(7) + 4) / 5;
	if (unit < 1) unit = 1;
	if (unit > SV_GOLD_MAX) unit = SV_GOLD_MAX;

	return (lookup_kind(TV_GOLD, unit));
}

/* Catching bad players who hack their client.. nasty! */
void lua_intrusion(int Ind, char *problem_diz) {
	s_printf("LUA INTRUSION: %s : %s\n", Players[Ind]->name, problem_diz);

#if 0 /* 4.4.8 client had a bug, mass-near-killing people. Let's turn this silly stuff off. -C. Blue */
	take_hit(Ind, Players[Ind]->chp - 1, "", 0);
	msg_print(Ind, "\377rThat was close huh?!");
#else
	if (!strcmp(problem_diz, "bad spell level")) {
		msg_print(Ind, "\376\377RERROR: You need higher skill to cast this spell. However, your book shows");
		msg_print(Ind, "\376\377R       that you may cast it because your LUA spell scripts are out of date!");
		msg_print(Ind, "\376\377R       Please update your client (or at least the LUA files in lib/scpt).");
		// (and don't use '-u' command-line option)
	} else {
		msg_print(Ind, "\376\377RERROR: Your LUA spell scripts seem to be out of date!");
		msg_print(Ind, "\376\377R       Please update your client (or at least the LUA files in lib/scpt).");
		// (and don't use '-u' command-line option)
	}
#endif
}

void bbs_add_line(cptr textline) {
	int i, j;
	/* either find an empty bbs entry (store its position in j) */
	for (i = 0; i < BBS_LINES; i++)
		if(!strcmp(bbs_line[i], "")) break;
	j = i;
	/* or scroll up by one line, discarding the first line */
	if (i == BBS_LINES)
		for (j = 0; j < BBS_LINES - 1; j++)
			strcpy(bbs_line[j], bbs_line[j + 1]);
	/* write the line to the bbs */
	strncpy(bbs_line[j], textline, MAX_CHARS_WIDE - 3); /* lines get one leading spaces on outputting, so it's 78-1  //  was 77 */
}

void bbs_del_line(int entry) {
	int j;
	if (entry < 0) return;
	if (entry >= BBS_LINES) return;
	for (j = entry; j < BBS_LINES - 1; j++)
		strcpy(bbs_line[j], bbs_line[j + 1]);
	/* erase last line */
	strcpy(bbs_line[BBS_LINES - 1], "");
}

void bbs_erase(void) {
	int i;
	for (i = 0; i < BBS_LINES; i++)
		strcpy(bbs_line[i], "");
}

void pbbs_add_line(u16b party, cptr textline) {
	int i, j;
	/* either find an empty bbs entry (store its position in j) */
	for (i = 0; i < BBS_LINES; i++)
		if(!strcmp(pbbs_line[party][i], "")) break;
	j = i;
	/* or scroll up by one line, discarding the first line */
	if (i == BBS_LINES)
		for (j = 0; j < BBS_LINES - 1; j++)
			strcpy(pbbs_line[party][j], pbbs_line[party][j + 1]);
	/* write the line to the bbs */
	strncpy(pbbs_line[party][j], textline, MAX_CHARS_WIDE - 3);
}

void gbbs_add_line(byte guild, cptr textline) {
	int i, j;
	/* either find an empty bbs entry (store its position in j) */
	for (i = 0; i < BBS_LINES; i++)
		if(!strcmp(gbbs_line[guild][i], "")) break;
	j = i;
	/* or scroll up by one line, discarding the first line */
	if (i == BBS_LINES)
		for (j = 0; j < BBS_LINES - 1; j++)
			strcpy(gbbs_line[guild][j], gbbs_line[guild][j + 1]);
	/* write the line to the bbs */
	strncpy(gbbs_line[guild][j], textline, MAX_CHARS_WIDE - 3);
}


/*
 * Add a player id to a linked list.
 * Doesn't check for duplicates
 * Takes a double pointer to the list
 */
void player_list_add(player_list_type **list, s32b player) {
	player_list_type *pl_ptr;

	MAKE(pl_ptr, player_list_type);

	pl_ptr->id = player;
	pl_ptr->next = *list;
	*list = pl_ptr;
}

/*
 * Check if a list contains an id.
 */
bool player_list_find(player_list_type *list, s32b player) {
	player_list_type *pl_ptr;

	pl_ptr = list;

	while (pl_ptr) {
		if (pl_ptr->id == player) return TRUE;
		pl_ptr = pl_ptr->next;
	}

	return FALSE;
}

/*
 * Delete an id from a list.
 * Takes a double pointer to the list
 */
bool player_list_del(player_list_type **list, s32b player) {
	player_list_type *pl_ptr, *prev;

	if (*list == NULL) return FALSE;

	/* Check the first node */
	if ((*list)->id == player) {
		*list = (*list)->next;
		return TRUE;
	}

	pl_ptr = (*list)->next;
	prev = *list;

	/* Check the rest of the nodes */
	while (pl_ptr) {
		if (pl_ptr->id == player) {
			prev->next = pl_ptr->next;
			FREE(pl_ptr, player_list_type);
			return TRUE;
		}

		prev = pl_ptr;
		pl_ptr = pl_ptr->next;
	}

	return FALSE;
}

/*
 * Free an entire list.
 */
void player_list_free(player_list_type *list) {
	player_list_type *pl_ptr, *tmp;

	pl_ptr = list;

	while (pl_ptr) {
		tmp = pl_ptr;
		pl_ptr = pl_ptr->next;
		FREE(tmp, player_list_type);
	}
}

/*
 * Check if the client version fills the requirements.
 *
 * Branch has to be an exact match.
 */
bool is_newer_than(version_type *version, int major, int minor, int patch, int extra, int branch, int build) {
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
	else if (version->branch == branch) {
		/* Now check the build */
		if (version->build < build)
			return FALSE;
		else if (version->build > build)
			return TRUE;
	}

	/* Default */
	return FALSE;
}

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
 * Since only GNU libc has memfrob, we use our own.
 */
void my_memfrob(void *s, int n) {
	int i;
	char *str;

	str = (char*) s;

	for (i = 0; i < n; i++) {
		/* XOR every byte with 42 */
		str[i] ^= 42;
	}
}

/* compare player mode compatibility - C. Blue
   Note: returns NULL if compatible.
   strict: Ignore IRONDEEPDIVE_ALLOW_INCOMPAT. */
cptr compat_pmode(int Ind1, int Ind2, bool strict) {
	player_type *p1_ptr = Players[Ind1], *p2_ptr = Players[Ind2];

#ifdef IRONDEEPDIVE_ALLOW_INCOMPAT
	/* EXPERIMENTAL */
	if (!strict &&
	    in_irondeepdive(&p1_ptr->wpos) &&
	    in_irondeepdive(&p2_ptr->wpos))
		return NULL;
#endif

	if (p1_ptr->mode & MODE_PVP) {
		if (!(p2_ptr->mode & MODE_PVP)) {
			return "non-pvp";
		}
	} else if (p1_ptr->mode & MODE_EVERLASTING) {
		if (!(p2_ptr->mode & MODE_EVERLASTING)) {
			return "non-everlasting";
		}
	} else if (p2_ptr->mode & MODE_PVP) {
		return "pvp";
	} else if (p2_ptr->mode & MODE_EVERLASTING) {
		return "everlasting";
	}
	return NULL; /* means "is compatible" */
}

/* compare object and player mode compatibility - C. Blue
   Note: returns NULL if compatible. */
cptr compat_pomode(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];

#ifdef IRONDEEPDIVE_ALLOW_INCOMPAT
	/* EXPERIMENTAL */
	if (in_irondeepdive(&p_ptr->wpos) &&
	    in_irondeepdive(&o_ptr->wpos))
		return NULL;
#endif

#ifdef ALLOW_NR_CROSS_ITEMS
	if (o_ptr->NR_tradable &&
	    in_netherrealm(&p_ptr->wpos))
		return NULL;
#endif

	if (!o_ptr->owner || is_admin(p_ptr)) return NULL; /* always compatible */
	if (p_ptr->mode & MODE_PVP) {
		if (!(o_ptr->mode & MODE_PVP)) {
			if (o_ptr->mode & MODE_EVERLASTING) {
				if (!(cfg.charmode_trading_restrictions & 2)) {
					return "non-pvp";
				}
			} else if (!(cfg.charmode_trading_restrictions & 4)) {
				return "non-pvp";
			}
		}
	} else if (p_ptr->mode & MODE_EVERLASTING) {
		if (o_ptr->mode & MODE_PVP) {
			return "pvp";
		} else if (!(o_ptr->mode & MODE_EVERLASTING)) {
			if (!(cfg.charmode_trading_restrictions & 1)) return "non-everlasting";
		}
	} else if (o_ptr->mode & MODE_PVP) {
		return "pvp";
	} else if (o_ptr->mode & MODE_EVERLASTING) {
		return "everlasting";
	}
	return NULL; /* means "is compatible" */
}

/* compare two objects' mode compatibility for stacking/absorbing - C. Blue
   Note: returns NULL if compatible. */
cptr compat_omode(object_type *o1_ptr, object_type *o2_ptr) {
#ifdef IRONDEEPDIVE_ALLOW_INCOMPAT
	/* EXPERIMENTAL */
	if ((in_irondeepdive(&o1_ptr->wpos)) &&
	    in_irondeepdive(&o2_ptr->wpos))
		return NULL;
#endif

	/* ownership given for both items? */
	if (!o1_ptr->owner) {
		if (!o2_ptr->owner) return NULL; /* always compatible */
		else return "owned";
	} else if (!o2_ptr->owner) return "owned";

	/* both are owned. so compare actual modes */
	if (o1_ptr->mode & MODE_PVP) {
		if (!(o2_ptr->mode & MODE_PVP)) {
			return "non-pvp";
		}
	} else if (o1_ptr->mode & MODE_EVERLASTING) {
		if (!(o2_ptr->mode & MODE_EVERLASTING)) {
			return "non-everlasting";
		}
	} else if (o2_ptr->mode & MODE_PVP) {
		return "pvp";
	} else if (o2_ptr->mode & MODE_EVERLASTING) {
		return "everlasting";
	}
	return NULL; /* means "is compatible" */
}

cptr compat_mode(byte mode1, byte mode2) {
	if (mode1 & MODE_PVP) {
		if (!(mode2 & MODE_PVP)) {
			return "non-pvp";
		}
	} else if (mode1 & MODE_EVERLASTING) {
		if (!(mode2 & MODE_EVERLASTING)) {
			return "non-everlasting";
		}
	} else if (mode2 & MODE_PVP) {
		return "pvp";
	} else if (mode2 & MODE_EVERLASTING) {
		return "everlasting";
	}
	return NULL; /* means "is compatible" */
}

/* cut out pseudo-id inscriptions from a string (a note ie inscription usually),
   save resulting string in s2,
   save highest found pseudo-id string in psid. - C. Blue */
void note_crop_pseudoid(char *s2, char *psid, cptr s) {
	char *p, s0[ONAME_LEN];
	int id = -1; /* assume no pseudo-id inscription */

	if (s == NULL) return;
	strcpy(s2, s);

	while (TRUE) {
		strcpy(s0, s2);
		strcpy(s2, "");

		if ((p = strstr(s0, "empty-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 6);
			if (id < 0) id = 0;
		} else if ((p = strstr(s0, "empty"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 5);
			if (id < 0) id = 0;
		} else if ((p = strstr(s0, "uncursed-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 9);
			if (id < 0) id = 1;
		} else if ((p = strstr(s0, "uncursed"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 8);
			if (id < 0) id = 1;
		} else if ((p = strstr(s0, "terrible-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 9);
			if (id < 1) id = 2;
		} else if ((p = strstr(s0, "terrible"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 8);
			if (id < 1) id = 2;
		} else if ((p = strstr(s0, "cursed-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 7);
			if (id < 2) id = 3;
		} else if ((p = strstr(s0, "cursed"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 6);
			if (id < 2) id = 3;
		} else if ((p = strstr(s0, "bad-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 4);
			if (id < 2) id = 4;
		} else if ((p = strstr(s0, "bad"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 3);
			if (id < 2) id = 4;
		} else if ((p = strstr(s0, "worthless-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 10);
			if (id < 4) id = 5;
		} else if ((p = strstr(s0, "worthless"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 9);
			if (id < 4) id = 5;
		} else if ((p = strstr(s0, "broken-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 7);
			if (id < 5) id = 6;
		} else if ((p = strstr(s0, "broken"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 6);
			if (id < 5) id = 6;
		} else if ((p = strstr(s0, "average-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 8);
			if (id < 6) id = 7;
		} else if ((p = strstr(s0, "average"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 7);
			if (id < 6) id = 7;
		} else if ((p = strstr(s0, "good-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 5);
			if (id < 7) id = 8;
		} else if ((p = strstr(s0, "good"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 4);
			if (id < 7) id = 8;
		} else if ((p = strstr(s0, "excellent-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 10);
			if (id < 8) id = 9;
		} else if ((p = strstr(s0, "excellent"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 9);
			if (id < 8) id = 9;
		} else if ((p = strstr(s0, "special-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 8);
			if (id < 9) id = 10;
		} else if ((p = strstr(s0, "special"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 7);
			if (id < 9) id = 10;
		} else {
			/* no further replacements to make */
			break;
		}
	}

	strcpy(s2, s0);

	switch (id) {
	case 0: strcpy(psid, "empty"); break;
	case 1: strcpy(psid, "uncursed"); break;
	case 2: strcpy(psid, "terrible"); break;
	case 3: strcpy(psid, "cursed"); break;
	case 4: strcpy(psid, "bad"); break;
	case 5: strcpy(psid, "worthless"); break;
	case 6: strcpy(psid, "broken"); break;
	case 7: strcpy(psid, "average"); break;
	case 8: strcpy(psid, "good"); break;
	case 9: strcpy(psid, "excellent"); break;
	case 10: strcpy(psid, "special"); break;
	default:
		strcpy(psid, "");
	}
}

/* For when an item re-curses itself on equipping:
   Remove any 'uncursed' part in its inscription. */
void note_toggle_cursed(object_type *o_ptr, bool cursed) {
	char *cn, note2[ONAME_LEN], *cnp;

	if (!o_ptr->note) {
		if (cursed) o_ptr->note = quark_add("cursed");
		else o_ptr->note = quark_add("uncursed");
		return;
	}

	strcpy(note2, quark_str(o_ptr->note));

	/* remove old 'uncursed' inscriptions */
	if ((cn = strstr(note2, "uncursed"))) {
		while (note2[0] && (cn = strstr(note2, "uncursed"))) {
			cnp = cn + 7;
			if (cn > note2 && //the 'uncursed' does not start on the first character of note2?
			    *(cn - 1) == '-') cn--; /* cut out leading '-' delimiter before "uncursed" */

			/* strip formerly trailing delimiter if it'd end up on first position in the new inscription */
			if (cn == note2 && *(cnp + 1) == '-') cnp++;

			do {
				cnp++;
				*cn = *cnp;
				cn++;
			} while (*cnp);
		}
	}

	/* remove old 'cursed' inscription */
	if ((cn = strstr(note2, "cursed"))) {
		while (note2[0] && (cn = strstr(note2, "cursed"))) {
			cnp = cn + 5;
			if (cn > note2 && //the 'cursed' does not start on the first character of note2?
			    *(cn - 1) == '-') cn--; /* cut out leading '-' delimiter before "cursed" */

			/* strip formerly trailing delimiter if it'd end up on first position in the new inscription */
			if (cn == note2 && *(cnp + 1) == '-') cnp++;

			do {
				cnp++;
				*cn = *cnp;
				cn++;
			} while (*cnp);
		}
	}

	/* append the new cursed-state indicator */
	if (note2[0]) strcat(note2, "-");
	if (cursed) {
		strcat(note2, "cursed");
		o_ptr->note = quark_add(note2);
	} else {
		strcat(note2, "uncursed");
		o_ptr->note = quark_add(note2);
	}
}

/* Wands/staves: Add/crop 'empty' inscription. */
void note_toggle_empty(object_type *o_ptr, bool empty) {
	char *cn, note2[ONAME_LEN], *cnp;

	/* If no inscription, object_desc() will handle this by adding a pseudo-inscription 'empty'. */
	if (!o_ptr->note) return;

	strcpy(note2, quark_str(o_ptr->note));

	if (!empty) {
		/* remove old 'empty' inscription */
		if ((cn = strstr(note2, "empty"))) {
			while (note2[0] && (cn = strstr(note2, "empty"))) {
				cnp = cn + 4;
				if (cn > note2 && //the 'empty' does not start on the first character of note2?
				    *(cn - 1) == '-') cn--; /* cut out leading '-' delimiter before "empty" */

				/* strip formerly trailing delimiter if it'd end up on first position in the new inscription */
				if (cn == note2 && *(cnp + 1) == '-') cnp++;

				do {
					cnp++;
					*cn = *cnp;
					cn++;
				} while (*cnp);
			}
		}
		o_ptr->note = quark_add(note2);
		return;
	}

	/* inscription already exists? */
	if (strstr(note2, "empty")) return;

	/* don't add 'empty' inscription to items that show "(0 charges)" already, aka identified items */
	if (o_ptr->ident & ID_KNOWN) return;

	/* append the new empty-state indicator */
	if (note2[0]) strcat(note2, "-");
	if (empty) {
		strcat(note2, "empty");
		o_ptr->note = quark_add(note2);
	}
}

/* Convert certain characters into HTML entities
* These characters are <, >, & and "
* NOTE: The returned string is dynamically allocated
*/
char *html_escape(const char *str) {
	int i, new_len = 0;
	const char *tmp;
	char *result;
	
	if (!str) {
		/* Return an empty string */
		result = malloc(1);
		*result = '\0';
		return result;
	}
	
	/* Calculate the resulting length */
	tmp = str;
	while (*tmp) {
		switch (*tmp) {
			case '<': case '>':
				new_len += 3;
				break;
			case '&':
				new_len += 4;
				break;
			case '"':
				new_len += 5;
				break;
		}
		new_len++;
		tmp++;
	}
	
	result = malloc(new_len + 1);
	i = 0;
	tmp = str;
	
	while (*tmp) {
		switch (*tmp) {
			case '<':
				result[i++] = '&';
				result[i++] = 'l';
				result[i++] = 't';
				result[i++] = ';';
				break;
			case '>':
				result[i++] = '&';
				result[i++] = 'g';
				result[i++] = 't';
				result[i++] = ';';
				break;
			case '&':
				result[i++] = '&';
				result[i++] = 'a';
				result[i++] = 'm';
				result[i++] = 'p';
				result[i++] = ';';
				break;
			case '"':
				result[i++] = '&';
				result[i++] = 'q';
				result[i++] = 'u';
				result[i++] = 'o';
				result[i++] = 't';
				result[i++] = ';';
				break;
			default:
				result[i++] = *tmp;
		}
		tmp++;
	}
	
	/* Terminate */
	result[new_len] = '\0';
	
	return result;
}

#define JSON_ESCAPE_ARRAY_SIZE 93
const char *json_escape_chars[JSON_ESCAPE_ARRAY_SIZE] = {
	"\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005", "\\u0006", "\\u0007",
	"\\b",     "\\t",     "\\n",     "\\u000b", "\\f",     "\\r",     "\\u000e", "\\u000f",
	"\\u0010", "\\u0011", "\\u0012", "\\u0013", "\\u0014", "\\u0015", "\\u0016", "\\u0017",
	"\\u0018", "\\u0019", "\\u001a", "\\u001b", "\\u001c", "\\u001d", "\\u001e", "\\u001f",
	NULL, NULL, "\\\"", NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, "\\\\"
};

const int json_escape_len[JSON_ESCAPE_ARRAY_SIZE] = {
	5, 5, 5, 5, 5, 5, 5, 5,
	2, 2, 2, 5, 2, 2, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5,
	0, 0, 2, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 2
};

/*
 * JSON spec requires that all control characters, ", and \ must be escaped.
 * 'dest' is the output buffer, 'src' is the input buffer and 'n' is the size of the output buffer.
 * The resulting string is always NULL-terminated even if the buffer is not big enough.
 */
char *json_escape_str(char *dest, const char *src, size_t n) {
	size_t destpos = 0;
	const char *outstr = NULL;
	int outlen = 0;
	int c = 0;
	while ((c = *src++)) {
		// Special characters need to be escaped
		if (c >= 0 && c < JSON_ESCAPE_ARRAY_SIZE) {
			outstr = json_escape_chars[c];
			outlen = json_escape_len[c];
		} else {
			outstr = NULL;
			outlen = 0;
		}
		if (outstr) {
			if (destpos + outlen + 1 < n) {
				char c2;
				while ((c2 = *outstr++)) {
					dest[destpos++] = c2;
				}
			}
		} else {
			// Make sure output buffer has enough room
			if (destpos + 1 < n) {
				dest[destpos++] = c;
			}
		}
	}
	dest[destpos] = '\0';
	return dest;
}

/* level generation benchmark */
void do_benchmark(int Ind) {
	int i, n = 100;
	player_type *p_ptr = Players[Ind];
	struct timeval begin, end;
	int sec, usec;

#ifndef WINDOWS
	block_timer();
#endif

	/* Use gettimeofday to get precise time */
	gettimeofday(&begin, NULL);

	for (i = 0; i < n; i++) {
		generate_cave(&p_ptr->wpos, p_ptr);
	}

	gettimeofday(&end, NULL);

#ifndef WINDOWS
	allow_timer();
#endif

	/* Calculate the time */
	sec = end.tv_sec - begin.tv_sec;
	usec = end.tv_usec - begin.tv_usec;
	if (usec < 0) {
		usec += 1000000;
		sec--;
	}

	/* Print the result */
	msg_format(Ind, "Generated %d levels in %d.%06d seconds.", n, sec, usec);

	/* Log it too */
	s_printf("BENCHMARK: Generated %d levels in %d.%06d seconds.\n", n, sec, usec);
}

/*
 * Get the difference between two timevals as a string.
 */
cptr timediff(struct timeval *begin, struct timeval *end) {
	static char buf[20];
	int sec, msec, usec;

	sec = end->tv_sec - begin->tv_sec;
	usec = end->tv_usec - begin->tv_usec;

	if (usec < 0) {
		usec += 1000000;
		sec--;
	}

	msec = usec / 1000;

	snprintf(buf, 20, "%d.%03d", sec, msec);
	return buf;
}

/* Strip special chars off player input 's', to prevent any problems/colours */
void strip_control_codes(char *ss, char *s) {
	char *sp = s, *ssp = ss;
	bool skip = FALSE;
	while(TRUE) {
		if (*sp == '\0') { /* end of string has top priority */
			*ssp = '\0';
			break;
		} else if (skip) skip = FALSE; /* a 'code parameter', needs to be stripped too */
		else if ((*sp >= 32) && (*sp <= 127)) { /* normal characters */
			*ssp = *sp;
			ssp++;
		} else if (*sp == '\377') {
			if (*(sp + 1) == '\377') { /* make two '{' become one real '{' */
				*ssp = '{';
				ssp++;
			}
			skip = TRUE; /* strip colour codes */
		}
		sp++;
	}
}

/* return a string displaying the flag state - C. Blue */
cptr flags_str(u32b flags) {
#if 0 /* disfunctional atm */
	char *fsp = malloc(40 * sizeof(char));
	int b;

	for (b = 0; b < 32; b++) {
		*fsp++ = (flags & (1U << b)) ? '1' : '0';
		if (b % 4 == 3) *fsp++ = ' ';
	}
	*fsp++ = '\0';

	return (fsp - 40);
#else
	return (NULL);
#endif
}

/* get player's racial attribute */
cptr get_prace(player_type *p_ptr) {
#ifdef ENABLE_MAIA
	if (p_ptr->prace == RACE_MAIA && p_ptr->ptrait) {
		if (p_ptr->ptrait == TRAIT_ENLIGHTENED)
			return "Enlightened";
		else if (p_ptr->ptrait == TRAIT_CORRUPTED)
			return "Corrupted";
		else
			return special_prace_lookup[p_ptr->prace];
	} else
#endif
	return special_prace_lookup[p_ptr->prace];
}

/* get player's title */
cptr get_ptitle(player_type *p_ptr, bool short_form) {
	if (p_ptr->lev < 60) return player_title[p_ptr->pclass][((p_ptr->lev / 5) < 10)? (p_ptr->lev / 5) : 10][(short_form ? 3 : 1) - p_ptr->male];
	return player_title_special[p_ptr->pclass][(p_ptr->lev < PY_MAX_PLAYER_LEVEL) ? (p_ptr->lev - 60) / 10 : 4][(short_form ? 3 : 1) - p_ptr->male];
}

#ifdef DUNGEON_VISIT_BONUS
void reindex_dungeons() {
# ifdef DUNGEON_VISIT_BONUS_DEPTHRANGE
	int i;
# endif
	int x, y;
	wilderness_type *w_ptr;
	struct dungeon_type *d_ptr;

	dungeon_id_max = 0;

	for (y = 0; y < MAX_WILD_Y; y++) {
		for (x = 0; x < MAX_WILD_X; x++) {
			w_ptr = &wild_info[y][x];
			if (w_ptr->flags & WILD_F_UP) {
				d_ptr = w_ptr->tower;
				d_ptr->id = ++dungeon_id_max;

				dungeon_visit_frequency[dungeon_id_max] = 0;
				dungeon_x[dungeon_id_max] = x;
				dungeon_y[dungeon_id_max] = y;
				dungeon_tower[dungeon_id_max] = TRUE;

				/* Initialize all dungeons at 'low rest bonus' */
				set_dungeon_bonus(dungeon_id_max, TRUE);

				s_printf("  indexed tower   at %d,%d: id = %d\n", x, y, dungeon_id_max);
			}
			if (w_ptr->flags & WILD_F_DOWN) {
				d_ptr = w_ptr->dungeon;
				d_ptr->id = ++dungeon_id_max;

				dungeon_visit_frequency[dungeon_id_max] = 0;
				dungeon_x[dungeon_id_max] = x;
				dungeon_y[dungeon_id_max] = y;
				dungeon_tower[dungeon_id_max] = FALSE;

				/* Initialize all dungeons at 'low rest bonus' */
				set_dungeon_bonus(dungeon_id_max, TRUE);

				s_printf("  indexed dungeon at %d,%d: id = %d\n", x, y, dungeon_id_max);
			}
		}
	}

# ifdef DUNGEON_VISIT_BONUS_DEPTHRANGE
	for (i = 0; i < 20; i++)
		depthrange_visited[i] = 0;
# endif

	s_printf("Reindexed %d dungeons/towers.\n", dungeon_id_max);
}

void set_dungeon_bonus(int id, bool reset) {
	if (reset) {
		/* Initialize dungeon at 'low rest bonus' */
		dungeon_visit_frequency[id] = ((VISIT_TIME_CAP * 17) / 20) - 1; /* somewhat below the threshold */
		dungeon_bonus[id] = 1;
		return;
	}

	if (dungeon_visit_frequency[id] < VISIT_TIME_CAP / 10)
		dungeon_bonus[id] = 3;
	else if (dungeon_visit_frequency[id] < VISIT_TIME_CAP / 2)
		dungeon_bonus[id] = 2;
	else if (dungeon_visit_frequency[id] < (VISIT_TIME_CAP * 19) / 20)
		dungeon_bonus[id] = 1;
	else
		dungeon_bonus[id] = 0;
}
#endif

/*
 * Shuffle an array of integers using the Fisher-Yates algorithm.
 */
void intshuffle(int *array, int size) {
	int i, j, tmp;

	for (i = size - 1; i > 0; i--) {
		j = rand_int(i + 1);
		tmp = array[i];
		array[i] = array[j];
		array[j] = tmp;
	}
}

/* for all the dungeons/towers that are special, yet use the type 0 dungeon template - C. Blue
   todo: actually create own types for these. would also make DF3_JAIL_DUNGEON obsolete.
   If 'extra' is set, special info is added: Town name, to keep the two Angbands apart. */
char *get_dun_name(int x, int y, bool tower, dungeon_type *d_ptr, int type, bool extra) {
	static char *jail = "Jail Dungeon";
	static char *pvp_arena = "PvP Arena";
	static char *highlander = "Highlands";
	static char *irondeepdive = "Ironman Deep Dive Challenge";

	/* hacks for 'extra' info, ugh */
	static char *angband_lothlorien = "Angband (Lothlorien)";
	static char *angband_khazaddum = "Angband (Khazad-dum)";

	if (d_ptr) type = d_ptr->type;

	/* normal dungeon */
	if (type) {
		/* hack for the two Angbands - so much hardcoding.... */
		if (extra && !strcmp(d_name + d_info[type].name, "Angband")) {
			if (town[TIDX_LORIEN].x == x && town[TIDX_LORIEN].y == y)
				return angband_lothlorien;
			else return angband_khazaddum;
		} else /* default */
			return (d_name + d_info[type].name);
	}

	/* special type 0 (yet) dungeons */
	if (x == WPOS_PVPARENA_X &&
	    y == WPOS_PVPARENA_Y &&
	    tower == (WPOS_PVPARENA_Z > 0))
		return pvp_arena;

	if (x == WPOS_HIGHLANDER_X &&
	    y == WPOS_HIGHLANDER_Y &&
	    tower == (WPOS_HIGHLANDER_DUN_Z > 0))
		return highlander;

	if (x == WPOS_IRONDEEPDIVE_X &&
	    y == WPOS_IRONDEEPDIVE_Y &&
	    tower == (WPOS_IRONDEEPDIVE_Z > 0))
		return irondeepdive;

	if (d_ptr && (
#if 0 /* obsolete fortunately (/fixjaildun) */
	    /* ughhh */
	    (d_ptr->baselevel == 30 && d_ptr->maxdepth == 30 &&
	    (d_ptr->flags1 & DF1_FORGET) &&
	    (d_ptr->flags2 & DF2_IRON))
	    ||
#endif
	    /* yay */
	    (d_ptr->flags3 & DF3_JAIL_DUNGEON) ))
		return jail;

	/* really just "Wilderness" */
	return (d_name + d_info[type].name);
}

/* Add gold to the player's gold, for easier handling. - C. Blue
   Returns FALSE if we already own 2E9 Au. */
bool gain_au(int Ind, u32b amt, bool quiet, bool exempt) {
	player_type *p_ptr = Players[Ind];

	/* hack: prevent s32b overflow */
	if (2000000000 - amt < p_ptr->au) {
		if (!quiet) msg_format(Ind, "\377yYou cannot carry more than 2 billion worth of gold!");
		return FALSE;
	} else {
#ifdef EVENT_TOWNIE_GOLD_LIMIT
		if ((p_ptr->mode & MODE_DED_IDDC) && !in_irondeepdive(&p_ptr->wpos)
		    && p_ptr->gold_picked_up == EVENT_TOWNIE_GOLD_LIMIT) {
			msg_print(Ind, "\377yYou cannot collect any more cash or your life would be forfeit.");
			return FALSE;
		}
#endif

		/* Collect the gold */
		p_ptr->au += amt;
		p_ptr->redraw |= (PR_GOLD);
	}

	if (exempt) return TRUE;
#ifdef EVENT_TOWNIE_GOLD_LIMIT
	/* if EVENT_TOWNIE_GOLD_LIMIT is 0 then nothing happens */
	if (p_ptr->gold_picked_up <= EVENT_TOWNIE_GOLD_LIMIT) {
		p_ptr->gold_picked_up += (amt > EVENT_TOWNIE_GOLD_LIMIT) ? EVENT_TOWNIE_GOLD_LIMIT : amt;
		if (p_ptr->gold_picked_up > EVENT_TOWNIE_GOLD_LIMIT
		    && !p_ptr->max_exp) {
			msg_print(Ind, "You gain a tiny bit of experience from collecting cash.");
			gain_exp(Ind, 1);
		}
	}
#endif
	return TRUE;
}

/* backup all house prices and contents for all players to lib/save/estate/.
   partial: don't stop with failure if a character files can't be read. */
#define ESTATE_BACKUP_VERSION "v2"
bool backup_estate(bool partial) {
	FILE *fp;
	char buf[MAX_PATH_LENGTH], buf2[MAX_PATH_LENGTH], savefile[CHARACTERNAME_LEN], c;
	cptr name;
	int i, j, k;
	int sy, sx, ey,ex , x, y;
	cave_type **zcave, *c_ptr;
	bool newly_created, allocated;
	u32b au;
	house_type *h_ptr;
	struct dna_type *dna;
	struct worldpos *wpos;
	object_type *o_ptr;

	s_printf("Backing up all real estate...\n");
	path_build(buf2, MAX_PATH_LENGTH, ANGBAND_DIR_SAVE, "estate");

	/* create folder lib/save/estate if not existing */
#ifdef WINDOWS
	mkdir(buf2);
#else
	mkdir(buf2, 0770);
#endif

	/* scan all houses */
	for (i = 0; i < num_houses; i++) {
		h_ptr = &houses[i];
		if (!h_ptr->dna->owner) continue;

		wpos = &h_ptr->wpos;
		dna = h_ptr->dna;
		/* assume worst possible charisma (3) */
		au = dna->price / 100 * adj_chr_gold[0];
		if (au < 100) au = 100;
		//h_ptr->dy,dx

		/* WARNING: For now unable to save guild halls */
		if (h_ptr->dna->owner_type != OT_PLAYER) continue;

		name = lookup_player_name(h_ptr->dna->owner);
		if (!name) {
			s_printf("  warning: house %d's owner %d doesn't have a name.\n", i, h_ptr->dna->owner);
			continue;
		}

		/* create backup file if required, or append to it */
		/* create actual filename from character name (same as used for sf_delete or process_player_name) */
		k = 0;
		for (j = 0; name[j]; j++) {
			c = name[j];
			/* Accept some letters */
			if (isalpha(c) || isdigit(c)) savefile[k++] = c;
			/* Convert space, dot, and underscore to underscore */
			else if (strchr(SF_BAD_CHARS, c)) savefile[k++] = '_';
		}
		savefile[k] = '\0';
		/* build path name and try to create/append to player's backup file */
		path_build(buf, MAX_PATH_LENGTH, buf2, savefile);
		if ((fp = fopen(buf, "rb")) == NULL)
			newly_created = TRUE;
		else {
			newly_created = FALSE;
			fclose(fp);
		}
		if ((fp = fopen(buf, "ab")) == NULL) {
			s_printf("  error: cannot open file '%s'.\n", buf);
			if (partial) continue;
			return FALSE;
		} else if (newly_created) {
			newly_created = FALSE;
			/* begin with a version tag */
			fprintf(fp, "%s\n", ESTATE_BACKUP_VERSION);

#if 0 /* guild info is no longer tied to map reset! */
			/* add 2M Au if he's a guild master, since guilds will be erased if the server
			   savefile gets deleted (which is the sole purpose of calling this function..) */
			for (j = 0; j < MAX_GUILDS; j++)
				if (guilds[j].master == h_ptr->dna->owner) {
					fprintf(fp, "AU:%d\n", GUILD_PRICE);
					s_printf("  guild master: '%s'.\n", name);
					break;
				}
#endif
		}

		/* add house price to his backup file */
		fprintf(fp, "AU:%d\n", au);

		/* scan house contents and add them to his backup file */
		/* traditional house? */
		if (h_ptr->flags & HF_TRAD) {
			for (j = 0; j < h_ptr->stock_num; j++) {
				o_ptr = &h_ptr->stock[j];
				/* add object to backup file */
				fprintf(fp, "OB:");
				(void)fwrite(o_ptr, sizeof(object_type), 1, fp);
				/* store inscription too! */
				if (o_ptr->note) {
					fprintf(fp, "%d\n", (int)strlen(quark_str(o_ptr->note)));
					(void)fwrite(quark_str(o_ptr->note), sizeof(char), strlen(quark_str(o_ptr->note)), fp);
				} else
					fprintf(fp, "%d\n", -1);
			}
		}
		/* mang-style house? */
		else {
			/* allocate sector of the house to access objects */
			if (!(zcave = getcave(wpos))) {
				alloc_dungeon_level(wpos);
				wilderness_gen(wpos);
				if (!(zcave = getcave(wpos))) {
					s_printf("  error: cannot getcave(%d,%d,%d).\nfailed.\n", wpos->wx, wpos->wy, wpos->wz);
					fclose(fp);
					return FALSE;
				}
				allocated = TRUE;
			} else allocated = FALSE;

			if (h_ptr->flags & HF_RECT) {
				sy = h_ptr->y + 1;
				sx = h_ptr->x + 1;
				ey = h_ptr->y + h_ptr->coords.rect.height - 1;
				ex = h_ptr->x + h_ptr->coords.rect.width - 1;
				for (y = sy; y < ey; y++) {
					for (x = sx; x < ex; x++) {
						c_ptr = &zcave[y][x];
						if (c_ptr->o_idx) {
							o_ptr = &o_list[c_ptr->o_idx];
							/* add object to backup file */
							fprintf(fp, "OB:");
							(void)fwrite(o_ptr, sizeof(object_type), 1, fp);
							/* store inscription too! */
							if (o_ptr->note) {
								fprintf(fp, "%d\n", (int)strlen(quark_str(o_ptr->note)));
								(void)fwrite(quark_str(o_ptr->note), sizeof(char), strlen(quark_str(o_ptr->note)), fp);
							} else
								fprintf(fp, "%d\n", -1);
						}
					}
				}
			} else {
				/* Polygonal house */
				//fill_house(h_ptr, FILL_CLEAR, NULL);
			}

			if (allocated) dealloc_dungeon_level(wpos);
		}

		/* done with this particular house */
		fclose(fp);
	}

	s_printf("done.\n");
	return TRUE;
}

/* helper function for restore_estate():
   copy all remaining data from our save file to the temporary file
   and turn the temporary file into the new save file. */
void relay_estate(char *buf, char *buf2, FILE *fp, FILE *fp_tmp) {
	int c;

	/* relay the remaining data, if any */
	while ((c = fgetc(fp)) != EOF) fputc(c, fp_tmp);

	/* done */
	fclose(fp);
	fclose(fp_tmp);

	/* erase old save file, make temporary file the new save file */
	remove(buf);
	rename(buf2, buf);
}

/* get back the backed-up real estate from files */
void restore_estate(int Ind) {
	player_type *p_ptr = Players[Ind];
	FILE *fp, *fp_tmp;
	char buf[MAX_PATH_LENGTH], buf2[MAX_PATH_LENGTH], version[MAX_CHARS];
	char data[4], data_note[MSG_LEN];//MAX_OLEN?
	char o_name[MSG_LEN], *rc;
	unsigned long au;
	int data_len, r;
	object_type forge, *o_ptr = &forge;
	bool gained_anything = FALSE;

	s_printf("Restoring real estate for %s...\n", p_ptr->name);

	/* create folder lib/save/estate if not existing */
	path_build(buf2, MAX_PATH_LENGTH, ANGBAND_DIR_SAVE, "estate");

	/* build path name and try to create/append to player's backup file */
	path_build(buf, MAX_PATH_LENGTH, buf2, p_ptr->basename);
	if ((fp = fopen(buf, "rb")) == NULL) {
		s_printf("  error: No file '%s' exists to request from.\nfailed.\n", buf);
		msg_print(Ind, "\377yNo goods or money stored to request.");
		return;
	}
	s_printf("  opened file '%s'.\n", buf);

	/* Try to read version string */
	if (fgets(version, MAX_CHARS, fp) == NULL) {
		s_printf("  error: File is empty.\nfailed.\n");
		msg_print(Ind, "\377oAn error occurred, please contact an administrator.");
		fclose(fp);
		return;
	}
	version[strlen(version) - 1] = '\0';
	s_printf("  reading a version '%s' estate file.\n", version);

	/* open temporary file for writing the stuff the player left over */
	strcpy(buf2, buf);
	strcat(buf2, ".$$$");
	if ((fp_tmp = fopen(buf2, "wb")) == NULL) {
		s_printf("  error: cannot open temporary file '%s'.\nfailed.\n", buf2);
		msg_print(Ind, "\377oAn error occurred, please contact an administrator.");
		fclose(fp);
		return;
	}

	/* relay to temporary file */
	fprintf(fp_tmp, "%s\n", version);

	msg_print(Ind, "\377yChecking for money and items from estate stored for you...");
	while (TRUE) {
		/* scan file for either house price (AU:) or object (OB:) */
		if (!fread(data, sizeof(char), 3, fp)) {
			if (!gained_anything) {
				s_printf("  nothing to request.\ndone.\n");
				msg_print(Ind, "\377yNo goods or money left to request.");
			} else {
				s_printf("done.\n");
				msg_print(Ind, "\377yYou received everything that was stored for you.");
			}
			relay_estate(buf, buf2, fp, fp_tmp);
			return;
		}
		data[3] = '\0';

		/* get house price from backup file */
		if (!strcmp(data, "AU:")) {
			au = 0;
			r = fscanf(fp, "%lu\n", &au);
			if (r == EOF || r == 0 || !au) {
				s_printf("  error: Corrupted AU: line.\n");
				msg_print(Ind, "\377oAn error occurred, please contact an administrator.");
				relay_estate(buf, buf2, fp, fp_tmp);
				return;
			}

			/* give gold to player if it doesn't overflow,
			   otherwise relay rest to temporary file, swap and exit */
			if (!gain_au(Ind, au, FALSE, FALSE)) {
				msg_print(Ind, "\377yDrop/deposite some gold to be able to receive more gold.");

				/* write failed gold gain back into new buffer file */
				fprintf(fp_tmp, "AU:%lu\n", au);

				relay_estate(buf, buf2, fp, fp_tmp);
				return;
			}
			gained_anything = TRUE;
			s_printf("  gained %ld Au.\n", au);
			msg_format(Ind, "You receive %ld gold pieces.", au);
			continue;
		}
		/* get object from backup file */
		else if (!strcmp(data, "OB:")) {
			r = fread(o_ptr, sizeof(object_type), 1, fp);
			if (r == 0) {
				s_printf("  error: Failed to read object.\n");
				msg_print(Ind, "\377oAn error occurred, please contact an administrator.");
				fprintf(fp_tmp, "OB:");
				relay_estate(buf, buf2, fp, fp_tmp);
				return;
			}
			/* Update item's kind-index in case k_info.txt has been modified */
			o_ptr->k_idx = lookup_kind(o_ptr->tval, o_ptr->sval);
#ifdef SEAL_INVALID_OBJECTS
			if (!seal_or_unseal_object(o_ptr)) continue;
#endif

			/* also read inscription */
			o_ptr->note = 0;
			data_len = -2;
#if 0 /* scanf() sucks a bit */
			data_note[0] = '\0';
			r = fscanf(fp, "%d[^\n]", &data_len);
			r = fread(data_note, 1, 1, fp); //strip the \n that fscanf had to miss
#else
			rc = fgets(data_note, 4, fp);
			data_len = atoi(data_note);
			data_note[0] = '\0';
#endif
			if (rc == NULL || data_len == -2) {
				object_desc(Ind, o_name, o_ptr, TRUE, 3);
				s_printf("  error: Corrupted note line (item '%s').\n", o_name);
				msg_print(Ind, "\377oAn error occurred, please contact an administrator.");
				relay_estate(buf, buf2, fp, fp_tmp);
				return;
			}
			if (data_len != -1) {
				r = fread(data_note, sizeof(char), data_len, fp);
				if (r == data_len) {
					data_note[data_len] = '\0';
					o_ptr->note = quark_add(data_note);
				} else {
					s_printf("  error: Failed to read note line (item '%s').\n", o_name);
					msg_print(Ind, "\377oAn error occurred, please contact an administrator.");
					relay_estate(buf, buf2, fp, fp_tmp);
					return;
				}
			}

			/* is it a pile of gold? */
			if (o_ptr->tval == TV_GOLD) {
				/* give gold to player if it doesn't overflow,
				   otherwise relay rest to temporary file, swap and exit */
				au = o_ptr->pval;
				if (!gain_au(Ind, au, FALSE, FALSE)) {
					msg_print(Ind, "\377yDrop/deposite some gold to be able to receive more gold.");

					/* write failed gold gain back into new buffer file */
					fprintf(fp_tmp, "OB:");
					(void)fwrite(o_ptr, sizeof(object_type), 1, fp_tmp);

					/* paranoia: should always be inscriptionless of course */
					/* ..and its inscription */
					if (o_ptr->note) {
						fprintf(fp_tmp, "%d\n", (int)strlen(quark_str(o_ptr->note)));
						(void)fwrite(quark_str(o_ptr->note), sizeof(char), strlen(quark_str(o_ptr->note)), fp_tmp);
					} else
						fprintf(fp_tmp, "%d\n", -1);

					relay_estate(buf, buf2, fp, fp_tmp);
					return;
				}
				gained_anything = TRUE;
				s_printf("  gained %ld Au.\n", au);
				msg_format(Ind, "You receive %ld gold pieces.", au);
				continue;
			}

			/* give item to player if he has space left,
			   otherwise relay rest to temporary file, swap and exit */
			if (!inven_carry_okay(Ind, o_ptr, 0x0)) {
				msg_print(Ind, "\377yYour inventory is full, make some space to receive more items.");

				/* write failed item back into new buffer file */
				fprintf(fp_tmp, "OB:");
				(void)fwrite(o_ptr, sizeof(object_type), 1, fp_tmp);

				/* ..and its inscription */
				if (o_ptr->note) {
					fprintf(fp_tmp, "%d\n", (int)strlen(quark_str(o_ptr->note)));
					(void)fwrite(quark_str(o_ptr->note), sizeof(char), strlen(quark_str(o_ptr->note)), fp_tmp);
				} else
					fprintf(fp_tmp, "%d\n", -1);

				relay_estate(buf, buf2, fp, fp_tmp);
				return;
			}

			gained_anything = TRUE;
			inven_carry(Ind, o_ptr);
			object_desc(Ind, o_name, o_ptr, TRUE, 3);
			msg_format(Ind, "You receive %s.", o_name);
			s_printf("  gained %s.\n", o_name);
			continue;
		} else {
			s_printf("  invalid data '%s'.\n", data);
			msg_print(Ind, "\377oAn error occurred, please contact an administrator.");
			relay_estate(buf, buf2, fp, fp_tmp);
			return;
		}
	}
}

/* For gathering statistical Ironman Deep Dive Challenge data on players clearing out monsters */
void log_floor_coverage(dun_level *l_ptr, struct worldpos *wpos) {
	cptr feel;

	if (!l_ptr) return;

	if (l_ptr->flags2 & LF2_OOD_HI) feel = "terrifying";
	else if ((l_ptr->flags2 & LF2_VAULT_HI) &&
	    (l_ptr->flags2 & LF2_OOD)) feel = "terrifying";
	else if ((l_ptr->flags2 & LF2_VAULT_OPEN) || // <- TODO: implement :/
	     ((l_ptr->flags2 & LF2_VAULT) && (l_ptr->flags2 & LF2_OOD_FREE))) feel = "danger";
	else if (l_ptr->flags2 & LF2_VAULT) feel = "dangerous";
	else if (l_ptr->flags2 & LF2_PITNEST_HI) feel = "dangerous";
	else if (l_ptr->flags2 & LF2_OOD_FREE) feel = "challenge";
	else if (l_ptr->flags2 & LF2_UNIQUE) feel = "special";
	else feel = "boring";

	if (l_ptr->monsters_generated + l_ptr->monsters_spawned == 0)
		s_printf("CVRG-IDDC: %3d, g:--- s:--- k:---, --%% (%s)\n", wpos->wz, feel);
	else
		s_printf("CVRG-IDDC: %3d, g:%3d s:%3d k:%3d, %2d%% (%s)\n", wpos->wz,
		    l_ptr->monsters_generated, l_ptr->monsters_spawned, l_ptr->monsters_killed,
		    (l_ptr->monsters_killed * 100) / (l_ptr->monsters_generated + l_ptr->monsters_spawned),
		    feel);
}

/* whenever the player enters a new grid (walk, teleport..)
   check whether he is affected in certain ways.. */
void grid_affects_player(int Ind, int ox, int oy) {
	player_type *p_ptr = Players[Ind];
	int x = p_ptr->px, y = p_ptr->py;
	cave_type **zcave;
	cave_type *c_ptr;
	bool inn = FALSE, music = FALSE;

	if (!(zcave = getcave(&p_ptr->wpos))) return;
	c_ptr = &zcave[p_ptr->py][p_ptr->px];
	inn = inside_inn(p_ptr, c_ptr);

	if (!p_ptr->wpos.wz && !night_surface && !(c_ptr->info & CAVE_PROT) &&
	    !(f_info[c_ptr->feat].flags1 & FF1_PROTECTED) &&
	    c_ptr->feat != FEAT_SHOP) {
		if (!p_ptr->grid_sunlit) {
			p_ptr->grid_sunlit = TRUE;
			calc_boni(Ind);
		}
	} else if (p_ptr->grid_sunlit) {
		p_ptr->grid_sunlit = FALSE;
		calc_boni(Ind);
	}

	/* Handle entering/leaving no-teleport area */
	if (zcave[y][x].info & CAVE_STCK && (ox == -1 || !(zcave[oy][ox].info & CAVE_STCK))) {
		msg_print(Ind, "\377DThe air in here feels very still.");
		p_ptr->redraw |= PR_DEPTH; /* hack: depth colour indicates no-tele */
#ifdef USE_SOUND_2010
		/* New: Have bgm indicate no-tele too! */
		handle_music(Ind);
		music = TRUE;
#endif

		msg_format_near(Ind, "%s loses %s wraith powers.", p_ptr->name, p_ptr->male ? "his" : "her");
		msg_print(Ind, "You lose your wraith powers.");

		/* Automatically disable permanent wraith form (set_tim_wraith) */
		p_ptr->tim_wraith = 0; //avoid duplicate message
		p_ptr->update |= PU_BONUS;
		p_ptr->redraw |= PR_BPR_WRAITH;
	}
	if (ox != -1 && (zcave[oy][ox].info & CAVE_STCK) && !(zcave[y][x].info & CAVE_STCK)) {
		msg_print(Ind, "\377sFresh air greets you as you leave the vault.");
		p_ptr->redraw |= PR_DEPTH; /* hack: depth colour indicates no-tele */
		p_ptr->redraw |= PR_BPR_WRAITH;
#ifdef USE_SOUND_2010
		/* New: Have bgm indicate no-tele too! */
		handle_music(Ind);
		music = TRUE;
#endif

		/* Automatically re-enable permanent wraith form (set_tim_wraith) */
		p_ptr->update |= PU_BONUS;
	}

#ifdef USE_SOUND_2010
	/* Handle entering a sickbay area (only possible via logging on into one) */
	if (p_ptr->music_monster != -3 && (c_ptr->feat == FEAT_SICKBAY_AREA || c_ptr->feat == FEAT_SICKBAY_DOOR)) {
		p_ptr->music_monster = -3; //hack sickbay music
		handle_music(Ind);
		music = TRUE;
	}

	/* Handle leaving a sickbay area */
	if (p_ptr->music_monster == -3 && c_ptr->feat != FEAT_SICKBAY_AREA && c_ptr->feat != FEAT_SICKBAY_DOOR) {
		p_ptr->music_monster = -1; //unhack sickbay music
		handle_music(Ind);
		music = TRUE;
	}

	/* Handle entering/leaving taverns. (-3 check is for distinguishing inn from sickbay) */
	if (p_ptr->music_monster != -3 && p_ptr->music_monster != -4 && inn) {
		p_ptr->music_monster = -4; //hack inn music
		handle_music(Ind);
		music = TRUE;

		/* Also take care of inn ambient sfx (fireplace) */
		handle_ambient_sfx(Ind, c_ptr, &p_ptr->wpos, TRUE);
	}
	if (p_ptr->music_monster == -4 && !inn) {
		p_ptr->music_monster = -1; //unhack inn music
		handle_music(Ind);
		music = TRUE;

		/* Also take care of inn ambient sfx (fireplace) */
		handle_ambient_sfx(Ind, c_ptr, &p_ptr->wpos, TRUE);
	}

	/* Handle entering a jail */
	if (p_ptr->music_monster != -5 && (c_ptr->info & CAVE_JAIL)) {
		p_ptr->music_monster = -5; //hack jail music
		handle_music(Ind);
		music = TRUE;
	}

	/* Handle leaving a jail */
	if (p_ptr->music_monster == -5 && !(c_ptr->info & CAVE_JAIL)) {
		p_ptr->music_monster = -1; //unhack jail music
		handle_music(Ind);
		music = TRUE;
	}
#endif

	/* Hack: Inns count as houses too */
	if (inside_house(&p_ptr->wpos, p_ptr->px, p_ptr->py) || inn || p_ptr->store_num != -1) {
		if (!p_ptr->grid_house) {
			p_ptr->grid_house = TRUE;
			if (!p_ptr->sfx_house) Send_sfx_volume(Ind, 0, 0);
			else if (p_ptr->sfx_house_quiet) Send_sfx_volume(Ind, p_ptr->sound_ambient == SFX_AMBIENT_FIREPLACE ? 100 : GRID_SFX_REDUCTION, GRID_SFX_REDUCTION);
		}
	} else if (p_ptr->grid_house) {
		p_ptr->grid_house = FALSE;
		if (p_ptr->sfx_house_quiet || !p_ptr->sfx_house) Send_sfx_volume(Ind, 100, 100);
	}

	/* Renew music too? */
	if (!music && ox == -1) handle_music(Ind);

	/* quests - check if he has arrived at a designated exact x,y target location */
	if (p_ptr->quest_any_deliver_xy_within_target) quest_check_goal_deliver(Ind);
}

/* Items that can be shared even between incompatible character modes or if level 0! */
bool exceptionally_shareable_item(object_type *o_ptr) {
	if (o_ptr->name1 || ((o_ptr->name2 || o_ptr->name2b) && o_ptr->tval != TV_FOOD)) return FALSE;

	if ((o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_WORD_OF_RECALL) ||
	    (o_ptr->tval == TV_LITE && o_ptr->sval == SV_LITE_TORCH) ||
	    (o_ptr->tval == TV_FLASK && o_ptr->sval == SV_FLASK_OIL) ||
	    (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_SATISFY_HUNGER) ||
	    // "Why not share ale? -Molt" <- good idea, here too!
	    (o_ptr->tval == TV_FOOD && o_ptr->sval >= SV_FOOD_MIN_FOOD && o_ptr->sval <= SV_FOOD_MAX_FOOD))
		return TRUE;
	return FALSE;
}
/* Starter items that can be shared despite being level 0! */
bool shareable_starter_item(object_type *o_ptr) {
	if (o_ptr->name1 || ((o_ptr->name2 || o_ptr->name2b) && o_ptr->tval != TV_FOOD)) return FALSE;

	if ((o_ptr->tval == TV_LITE && o_ptr->sval == SV_LITE_TORCH) ||
	    (o_ptr->tval == TV_SCROLL && o_ptr->sval == SV_SCROLL_SATISFY_HUNGER) ||
	    // "Why not share ale? -Molt" <- good idea, here too!
	    (o_ptr->tval == TV_FOOD && o_ptr->sval >= SV_FOOD_MIN_FOOD && o_ptr->sval <= SV_FOOD_MAX_FOOD))
		return TRUE;
	return FALSE;
}

void kick_char(int Ind_kicker, int Ind_kickee, char *reason) {
	char kickmsg[MSG_LEN];

	if (reason) {
		msg_format(Ind_kicker, "Kicking %s out (%s).", Players[Ind_kickee]->name, reason);
		snprintf(kickmsg, MSG_LEN, "Kicked out (%s)", reason);
		Destroy_connection(Players[Ind_kickee]->conn, kickmsg);
	} else {
		msg_format(Ind_kicker, "Kicking %s out.", Players[Ind_kickee]->name);
		Destroy_connection(Players[Ind_kickee]->conn, "Kicked out");
	}
}

void kick_ip(int Ind_kicker, char *ip_kickee, char *reason, bool msg) {
	int i;
	char kickmsg[MSG_LEN];
	bool found = FALSE;

	if (reason) {
		if (msg) msg_format(Ind_kicker, "Kicking out connections from %s (%s).", ip_kickee, reason);
		snprintf(kickmsg, MSG_LEN, "Kicked out - %s", reason);
	} else {
		if (msg) msg_format(Ind_kicker, "Kicking out connections from %s.", ip_kickee);
		snprintf(kickmsg, MSG_LEN, "Kicked out");
	}

	/* Kick him out (note, this could affect multiple people at once if sharing an IP) */
	for (i = 1; i <= NumPlayers; i++) {
		if (!Players[i]) continue;
		if (!strcmp(get_player_ip(i), ip_kickee)) {
			found = TRUE;
			if (reason) s_printf("IP-kicked '%s' (%s).\n", Players[i]->name, reason);
			else s_printf("IP-kicked '%s'.\n", Players[i]->name);
			Destroy_connection(Players[i]->conn, kickmsg);
			i--;
		}
	}
	if (msg && !found) msg_print(Ind_kicker, "No matching player online to kick.");
}

/* Calculate basic success chance for magic devices for a player,
   before any "minimum granted chance" is applied. */
static int magic_device_base_chance(int Ind, object_type *o_ptr) {
	u32b dummy, f4;
	player_type *p_ptr = Players[Ind];

	/* Extract the item level */
	int lev = k_info[o_ptr->k_idx].level;

#if 0 /* not needed anymore since x_dev and skill-ratios have been adjusted in tables.c */
	/* Reduce very high levels */
	lev = (400 - ((200 - lev) * (200 - lev)) / 100) / 4;//1..75
#endif

	/* Base chance of success */
	int chance = p_ptr->skill_dev;

	/* Hack -- use artifact level instead */
	if (true_artifact_p(o_ptr)) lev = a_info[o_ptr->name1].level;

	/* Extract object flags */
	object_flags(o_ptr, &dummy, &dummy, &dummy, &f4, &dummy, &dummy, &dummy);

	/* Is it simple to use ? */
	if (f4 & TR4_EASY_USE) {
		chance += USE_DEVICE;

		/* High level objects are harder */
		chance = chance - (lev * 4) / 5;
	} else {
		/* High level objects are harder */
		//chance = chance - ((lev > 50) ? 50 : lev) - (p_ptr->antimagic * 2);
		chance = chance - lev;
	}

	/* Hacks: Certain items are easier/harder to use in general: */

	/* Runes */
	if (o_ptr->tval == TV_RUNE) {
		chance = chance / 10 - 20;
		if (o_ptr->sval >= RCRAFT_MAX_ELEMENTS) chance -= 10;
		switch (o_ptr->sval) {
		/* basic tier */
		case SV_R_LITE:
			if (get_skill(p_ptr, SKILL_R_LITE))
				chance += get_skill(p_ptr, SKILL_R_LITE) + 15;
			break;
		case SV_R_DARK:
			if (get_skill(p_ptr, SKILL_R_DARK))
				chance += get_skill(p_ptr, SKILL_R_DARK) + 15;
			break;
		case SV_R_NEXU:
			if (get_skill(p_ptr, SKILL_R_NEXU))
				chance += get_skill(p_ptr, SKILL_R_NEXU) + 15;
			break;
		case SV_R_NETH:
			if (get_skill(p_ptr, SKILL_R_NETH))
				chance += get_skill(p_ptr, SKILL_R_NETH) + 15;
			break;
		case SV_R_CHAO:
			if (get_skill(p_ptr, SKILL_R_CHAO))
				chance += get_skill(p_ptr, SKILL_R_CHAO) + 15;
			break;
		case SV_R_MANA:
			if (get_skill(p_ptr, SKILL_R_MANA))
				chance += get_skill(p_ptr, SKILL_R_MANA) + 15;
			break;
		/* high tier - Ew, hardcoded (but nice job). - Kurzel */
		case SV_R_CONF:
			if (get_skill(p_ptr, SKILL_R_LITE) && get_skill(p_ptr, SKILL_R_DARK))
				chance += (get_skill(p_ptr, SKILL_R_LITE) + get_skill(p_ptr, SKILL_R_DARK)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_LITE))
				chance += get_skill(p_ptr, SKILL_R_LITE) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_DARK))
				chance += get_skill(p_ptr, SKILL_R_DARK) / 2 + 10;
		break;
		case SV_R_INER:
			if (get_skill(p_ptr, SKILL_R_LITE) && get_skill(p_ptr, SKILL_R_NEXU))
				chance += (get_skill(p_ptr, SKILL_R_LITE) + get_skill(p_ptr, SKILL_R_NEXU)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_LITE))
				chance += get_skill(p_ptr, SKILL_R_LITE) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_NEXU))
				chance += get_skill(p_ptr, SKILL_R_NEXU) / 2 + 10;
		break;
		case SV_R_ELEC:
			if (get_skill(p_ptr, SKILL_R_LITE) && get_skill(p_ptr, SKILL_R_NETH))
				chance += (get_skill(p_ptr, SKILL_R_LITE) + get_skill(p_ptr, SKILL_R_NETH)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_LITE))
				chance += get_skill(p_ptr, SKILL_R_LITE) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_NETH))
				chance += get_skill(p_ptr, SKILL_R_NETH) / 2 + 10;
		break;
		case SV_R_FIRE:
			if (get_skill(p_ptr, SKILL_R_LITE) && get_skill(p_ptr, SKILL_R_CHAO))
				chance += (get_skill(p_ptr, SKILL_R_LITE) + get_skill(p_ptr, SKILL_R_CHAO)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_LITE))
				chance += get_skill(p_ptr, SKILL_R_LITE) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_CHAO))
				chance += get_skill(p_ptr, SKILL_R_CHAO) / 2 + 10;
		break;
		case SV_R_WATE:
			if (get_skill(p_ptr, SKILL_R_LITE) && get_skill(p_ptr, SKILL_R_MANA))
				chance += (get_skill(p_ptr, SKILL_R_LITE) + get_skill(p_ptr, SKILL_R_MANA)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_LITE))
				chance += get_skill(p_ptr, SKILL_R_LITE) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_MANA))
				chance += get_skill(p_ptr, SKILL_R_MANA) / 2 + 10;
		break;
		case SV_R_GRAV:
			if (get_skill(p_ptr, SKILL_R_DARK) && get_skill(p_ptr, SKILL_R_NEXU))
				chance += (get_skill(p_ptr, SKILL_R_DARK) + get_skill(p_ptr, SKILL_R_NEXU)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_DARK))
				chance += get_skill(p_ptr, SKILL_R_DARK) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_NEXU))
				chance += get_skill(p_ptr, SKILL_R_NEXU) / 2 + 10;
		break;
		case SV_R_COLD:
			if (get_skill(p_ptr, SKILL_R_DARK) && get_skill(p_ptr, SKILL_R_NETH))
				chance += (get_skill(p_ptr, SKILL_R_DARK) + get_skill(p_ptr, SKILL_R_NETH)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_DARK))
				chance += get_skill(p_ptr, SKILL_R_DARK) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_NETH))
				chance += get_skill(p_ptr, SKILL_R_NETH) / 2 + 10;
		break;
		case SV_R_ACID:
			if (get_skill(p_ptr, SKILL_R_DARK) && get_skill(p_ptr, SKILL_R_CHAO))
				chance += (get_skill(p_ptr, SKILL_R_DARK) + get_skill(p_ptr, SKILL_R_CHAO)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_DARK))
				chance += get_skill(p_ptr, SKILL_R_DARK) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_CHAO))
				chance += get_skill(p_ptr, SKILL_R_CHAO) / 2 + 10;
		break;
		case SV_R_POIS:
			if (get_skill(p_ptr, SKILL_R_DARK) && get_skill(p_ptr, SKILL_R_MANA))
				chance += (get_skill(p_ptr, SKILL_R_DARK) + get_skill(p_ptr, SKILL_R_MANA)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_DARK))
				chance += get_skill(p_ptr, SKILL_R_DARK) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_MANA))
				chance += get_skill(p_ptr, SKILL_R_MANA) / 2 + 10;
		break;
		case SV_R_SHAR:
			if (get_skill(p_ptr, SKILL_R_NEXU) && get_skill(p_ptr, SKILL_R_NETH))
				chance += (get_skill(p_ptr, SKILL_R_NEXU) + get_skill(p_ptr, SKILL_R_NETH)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_NEXU))
				chance += get_skill(p_ptr, SKILL_R_NEXU) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_NETH))
				chance += get_skill(p_ptr, SKILL_R_NETH) / 2 + 10;
		break;
		case SV_R_SOUN:
			if (get_skill(p_ptr, SKILL_R_NEXU) && get_skill(p_ptr, SKILL_R_CHAO))
				chance += (get_skill(p_ptr, SKILL_R_NEXU) + get_skill(p_ptr, SKILL_R_CHAO)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_NEXU))
				chance += get_skill(p_ptr, SKILL_R_NEXU) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_CHAO))
				chance += get_skill(p_ptr, SKILL_R_CHAO) / 2 + 10;
		break;
		case SV_R_TIME:
			if (get_skill(p_ptr, SKILL_R_NEXU) && get_skill(p_ptr, SKILL_R_MANA))
				chance += (get_skill(p_ptr, SKILL_R_NEXU) + get_skill(p_ptr, SKILL_R_MANA)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_NEXU))
				chance += get_skill(p_ptr, SKILL_R_NEXU) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_MANA))
				chance += get_skill(p_ptr, SKILL_R_MANA) / 2 + 10;
		break;
		case SV_R_DISE:
			if (get_skill(p_ptr, SKILL_R_NETH) && get_skill(p_ptr, SKILL_R_CHAO))
				chance += (get_skill(p_ptr, SKILL_R_NETH) + get_skill(p_ptr, SKILL_R_CHAO)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_NETH))
				chance += get_skill(p_ptr, SKILL_R_NETH) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_CHAO))
				chance += get_skill(p_ptr, SKILL_R_CHAO) / 2 + 10;
		break;
		case SV_R_ICEE:
			if (get_skill(p_ptr, SKILL_R_NETH) && get_skill(p_ptr, SKILL_R_MANA))
				chance += (get_skill(p_ptr, SKILL_R_NETH) + get_skill(p_ptr, SKILL_R_MANA)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_NETH))
				chance += get_skill(p_ptr, SKILL_R_NETH) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_MANA))
				chance += get_skill(p_ptr, SKILL_R_MANA) / 2 + 10;
		break;
		case SV_R_PLAS:
			if (get_skill(p_ptr, SKILL_R_CHAO) && get_skill(p_ptr, SKILL_R_MANA))
				chance += (get_skill(p_ptr, SKILL_R_CHAO) + get_skill(p_ptr, SKILL_R_MANA)) / 2 + 25;
			else if (get_skill(p_ptr, SKILL_R_CHAO))
				chance += get_skill(p_ptr, SKILL_R_CHAO) / 2 + 10;
			else if (get_skill(p_ptr, SKILL_R_MANA))
				chance += get_skill(p_ptr, SKILL_R_MANA) / 2 + 10;
		break;
		}
	}
#if 1
	/* equippable magic devices are especially easy to use? (ie no wands/staves/rods)
	   eg tele rings, serpent amulets, true artifacts */
	else if (!is_magic_device(o_ptr->tval)) {
		chance += 30;
		chance = chance - lev / 10;
	}
#else
	/* Rings of polymorphing and the Ring of Phasing shouldn't require magic device skill really */
	else if ((o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_POLYMORPH) ||
	    (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_WRAITH)) {
		if (chance < USE_DEVICE * 2) chance = USE_DEVICE * 2;
	}
#endif

	/* Confusion makes it much harder (maybe TODO: blind/stun?) */
	if (p_ptr->confused) chance = chance / 2;

	return chance;
}

/* just for display purpose, return an actual average percentage value */
int activate_magic_device_chance(int Ind, object_type *o_ptr) {
	int chance = magic_device_base_chance(Ind, o_ptr);

	/* Give everyone a (slight) chance */
	if (chance < USE_DEVICE)
		return ((100 / (USE_DEVICE - chance + 1)) * ((100 * (USE_DEVICE - 1)) / USE_DEVICE)) / 100;

	/* Normal chance */
	return 100 - ((USE_DEVICE - 1) * 100) / chance;
}

bool activate_magic_device(int Ind, object_type *o_ptr) {
	player_type *p_ptr = Players[Ind];

	int chance = magic_device_base_chance(Ind, o_ptr);

	/* Certain items are heavily restricted (todo: use WINNERS_ONLY flag instead for cleanliness) */
	if (o_ptr->name1 == ART_PHASING && !p_ptr->total_winner) {
		msg_print(Ind, "Only royalties may activate this Ring!");
		if (!is_admin(p_ptr)) return FALSE;
	}

	/* Give everyone a (slight) chance */
	if ((chance < USE_DEVICE) && (rand_int(USE_DEVICE - chance + 1) == 0))
		chance = USE_DEVICE;

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint(chance) < USE_DEVICE)) return FALSE;
	return TRUE;
}

/* Condense an (account) name into a 'normalised' version, used to prevent
   new players from creating account names too similar to existing ones. - C. Blue */
void condense_name(char *condensed, const char *name) {
	char *bufptr = condensed, current, multiple = 0, *ptr;
	bool space = TRUE;

	for (ptr = (char*)name; *ptr; ptr++) {
		/* skip spaces in the beginning */
		if (space && *ptr == ' ') continue;
		space = FALSE;

		/* ignore lower/upper case */
		current = tolower(*ptr);

		//discard non-alphanumeric characters
		if (!isalpha(current) && !isdigit(current)) continue;

		//condense multiples of the same character
		if (multiple == current) continue;
		multiple = current;

		//finally add the character
		*bufptr++ = current;
	}
	*bufptr = 0; //terminate

	//discard spaces at the end of the name
	for (ptr = bufptr - 1; ptr >= condensed; ptr--)
		if (*ptr == ' ') {
			*ptr = 0;
			bufptr--;
		}
		else break;

	//extra strict: discard digits at the end of the name
	for (ptr = bufptr - 1; ptr >= condensed; ptr--)
		if (isdigit(*ptr))
			*ptr = 0;
		else break;
}

/* Helper function to check account/character/guild.. names for too great similarity:
   Super-strict mode: Disallow (non-trivial) names that only have 1+ letter inserted somewhere. */
int similar_names(const char *name1, const char *name2) {
	char tmpname[MAX_CHARS];
	const char *ptr, *ptr2;
	char *ptr3;
	int diff, min, diff_bonus;
	int diff_loc, diff2;
	bool words = FALSE;

	if (strlen(name1) < 5 || strlen(name2) < 5) return 0; //trivial length, it's ok to be similar

	/* don't exaggerate */
	if (strlen(name1) > strlen(name2)) min = strlen(name2);
	else min = strlen(name1);

	/* '->' */
	diff = 0;
	ptr2 = name1;
	diff_loc = -1;
	for (ptr = name2; *ptr && *ptr2; ) {
		if (tolower(*ptr) != tolower(*ptr2)) {
			diff++;
			if (diff_loc == -1) diff_loc = ptr - name2;//remember where they started to be different
		}
		else ptr++;
		ptr2++;
	}
	//all remaining characters that couldn't be matched are also "different"
	if (diff_loc == -1 && (*ptr || *ptr2)) diff_loc = ptr - name2;//remember where they started to be different
	while (*ptr++) diff++;
	while (*ptr2++) diff++;
	//too little difference between names? forbidden!
	if (diff <= (min - 5) / 2 + 1) {
		//special check: if they differ early on, it weighs slightly more :)
		if (diff_loc < min / 2 - 1) {
			//loosened up slightly
			if (diff <= (min - 6) / 2 + 1) {
				s_printf("similar_names (1a): name1 '%s', name2 '%s' (tmp '%s')\n", name1, name2, tmpname);
				return 1;
			}
		} else { //normal case
			s_printf("similar_names (1): name1 '%s', name2 '%s' (tmp '%s')\n", name1, name2, tmpname);
			return 1;
		}
	}

	/* '<-' */
	diff = 0;
	ptr2 = name2;
	diff_loc = -1;
	for (ptr = name1; *ptr && *ptr2; ) {
		if (tolower(*ptr) != tolower(*ptr2)) {
			diff++;
			if (diff_loc == -1) diff_loc = ptr - name1;//remember where they started to be different
		}
		else ptr++;
		ptr2++;
	}
	//all remaining characters that couldn't be matched are also "different"
	if (diff_loc == -1 && (*ptr || *ptr2)) diff_loc = ptr - name1;//remember where they started to be different
	while (*ptr++) diff++;
	while (*ptr2++) diff++;
	//too little difference between names? forbidden!
	if (diff <= (min - 5) / 2 + 1) {
		//special check: if they differ early on, it weighs slightly more :)
		if (diff_loc < min / 2 - 1) {
			//loosened up slightly
			if (diff <= (min - 6) / 2 + 1) {
				s_printf("similar_names (2a): name1 '%s', name2 '%s' (tmp '%s')\n", name1, name2, tmpname);
				return 2;
			}
		} else { //normal case
			s_printf("similar_names (2): name1 '%s', name2 '%s' (tmp '%s')\n", name1, name2, tmpname);
			return 2;
		}
	}

	/* '='  -- note: weakness here is, the first 2 methods don't combine with this one ;).
	   So the checks could be tricked by combining one 'replaced char' with one 'too many char'
	   to circumvent the limitations for longer names that usually would require a 3+ difference.. =P */
	diff = 0;
	ptr2 = name2;
	diff_loc = -1;
	diff_bonus = 0;
	for (ptr = name1; *ptr && *ptr2; ) {
		if (tolower(*ptr) != tolower(*ptr2)) {
			diff++;
			if (diff_loc == -1) diff_loc = ptr - name1;//remember where they started to be different
			//consonant vs vowel = big difference
			if (diff_loc < min / 2 - 1) /* see below (*) */
				if (is_a_vowel(tolower(*ptr)) != is_a_vowel(tolower(*ptr2))) diff_bonus++;
		}
		ptr++;
		ptr2++;
	}
	//all remaining characters that couldn't be matched are also "different"
	if (diff_loc == -1 && (*ptr || *ptr2)) diff_loc = ptr - name1;//remember where they started to be different
	while (*ptr++) diff++;
	while (*ptr2++) diff++;
	//too little difference between names? forbidden!
	if (diff <= (min - 5) / 2 + 1) {
		//special check: if they differ early on, it weighs slightly more :)
		if (diff_loc < min / 2 - 1) { /* see above (*) */
			//loosened up slightly
			if (diff <= (min - 6 - (diff_bonus ? 1 : 0)) / 2 + 1) {
				s_printf("similar_names (3a): name1 '%s', name2 '%s' (tmp '%s')\n", name1, name2, tmpname);
				return 3;
			}
		} else { //normal case
			s_printf("similar_names (3): name1 '%s', name2 '%s' (tmp '%s')\n", name1, name2, tmpname);
			return 3;
		}
	}

#if 0	/* Check for anagrams */
	diff = 0;
	strcpy(tmpname, name2);
	for (ptr = name1; *ptr; ptr++) {
		for (ptr3 = tmpname; *ptr3; ptr3++) {
			if (tolower(*ptr) == tolower(*ptr3)) {
				*ptr3 = '*'; //'disable' this character
				break;
			}
		}
		if (!(*ptr3)) diff++;
	}
	diff2 = 0;
	strcpy(tmpname, name1);
	for (ptr = name2; *ptr; ptr++) {
		for (ptr3 = tmpname; *ptr3; ptr3++) {
			if (tolower(*ptr) == tolower(*ptr3)) {
				*ptr3 = '*'; //'disable' this character
				break;
			}
		}
		if (!(*ptr3)) diff2++;
	}
	if (diff2 > diff) diff = diff2; //max()
	//too little difference between names? forbidden!
	if (diff <= (min - 6) / 2 + 1) { //must use the 'loosened up' version used further above, since it'd override otherwise
		s_printf("similar_names (4): name1 '%s', name2 '%s' (tmp '%s')\n", name1, name2, tmpname);
		return 4;
	}
#endif
#if 1	/* Check for exact anagrams */
	diff = 0;
	strcpy(tmpname, name2);
	for (ptr = name1; *ptr; ptr++) {
		for (ptr3 = tmpname; *ptr3; ptr3++) {
			if (tolower(*ptr) == tolower(*ptr3)) {
				*ptr3 = '*'; //'disable' this character
				break;
			}
		}
		if (!(*ptr3)) {
			diff = 1;
			break;
		}
	}
	strcpy(tmpname, name1);
	for (ptr = name2; *ptr; ptr++) {
		for (ptr3 = tmpname; *ptr3; ptr3++) {
			if (tolower(*ptr) == tolower(*ptr3)) {
				*ptr3 = '*'; //'disable' this character
				break;
			}
		}
		if (!(*ptr3)) {
			diff = 1;
			break;
		}
	}
	if (!diff) { //perfect anagrams?
		s_printf("similar_names (5): name1 '%s', name2 '%s' (tmp '%s')\n", name1, name2, tmpname);
		return 5;
	}
#endif

	/* Check for prefix */
	diff = 0;
	if (strlen(name1) <= strlen(name2)) {
		ptr = name1;
		ptr2 = name2;
		diff2 = strlen(name2);
	} else {
		ptr = name2;
		ptr2 = name1;
		diff2 = strlen(name1);
	}
	while (*ptr) {
		if (tolower(*ptr) != tolower(*ptr2)) break;
		if (!isalpha(*ptr) && !isdigit(*ptr)) words = TRUE;
		ptr++;
		ptr2++;
		//REVERSE diff here
		diff++;
	}
	//too little difference between names? forbidden!
	if ((diff >= 5 && diff > (diff2 * (words ? 6 : 5)) / 10) || (diff < 5 && !words && diff2 < diff + 2)) {
		s_printf("similar_names (6): name1 '%s', name2 '%s'\n", name1, name2);
		return 6;
	}

	return 0; //ok!
}
