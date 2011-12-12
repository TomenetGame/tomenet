/* $Id$ */
/* File: util.c */

/* Purpose: Angband utilities -BEN- */


#define SERVER

#include "angband.h"
#ifdef TOMENET_WORLDS
#include "../world/world.h"
#endif

#ifdef MINGW
/* For gettimeofday */
#include <sys/time.h>
#endif

//static int name_lookup_loose_quiet(int Ind, cptr name, u16b party);

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
			else if (isprint(*s) || *s=='\377')
			{
				/* easier to edit perma files */
				if(conv && *s=='{' && *(s+1)!='{')
					*s='\377';
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
//	Send_sound(Ind, val);
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

#endif

//	if (is_admin(Players[Ind])) s_printf("USE_SOUND_2010: looking up sound %s -> %d.\n", name, val);

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

			Send_sound(i, val, val2, type, 100 / d, Players[Ind]->id);
//			Send_sound(i, val, val2, type, (6 - d) * 20, Players[Ind]->id);  hm or this?
#else
			if (d > MAX_SIGHT) continue;
			d += 3;
			d /= 2;

			Send_sound(i, val, val2, type, 100 / d, Players[Ind]->id);
#endif
		}
	}

	Send_sound(Ind, val, val2, type, 100, Players[Ind]->id);
}
/* send sound to player and everyone nearby at full volume, similar to
   msg_..._near(), except it also sends to the player himself.
   This is used for highly important and *loud* sounds such as 'shriek' - C. Blue */
void sound_near(int Ind, cptr name, cptr alternative, int type) {
	int i, d;
	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (!inarea(&Players[i]->wpos, &Players[Ind]->wpos)) continue;

		/* Can he see this player? */
//		if (!(Players[i]->cave_flag[Players[Ind]->py][Players[Ind]->px] & CAVE_VIEW)) continue;

		/* within audible range? */
		d = distance(Players[Ind]->py, Players[Ind]->px, Players[i]->py, Players[i]->px);
		if (d > MAX_SIGHT) continue;

		sound(i, name, alternative, type, FALSE);
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

		/* Can (s)he see the site? */
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
/* like msg_print_near_monster() just for sounds,
   basically same as sound_near_site() - C. Blue */
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
//		if (!p_ptr->mon_vis[m_idx]) continue;

		/* Can he see this player? */
//		if (!p_ptr->cave_flag[y][x] & CAVE_VIEW) continue;

		/* Can (s)he see the site? */
//		if (!(p_ptr->cave_flag[y][x] & CAVE_VIEW)) continue;

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

/* find correct music for the player based on his current location - C. Blue */
void handle_music(int Ind) {
	player_type *p_ptr = Players[Ind];
	dun_level *l_ptr = NULL;
	int i = -1;
	cave_type **zcave = getcave(&p_ptr->wpos);

#ifdef ARCADE_SERVER
	/* Special music for Arcade Server stuff */
	if (p_ptr->wpos.wx == WPOS_ARCADE_X && p_ptr->wpos.wy == WPOS_ARCADE_Y
//	    && p_ptr->wpos.wz == WPOS_ARCADE_Z
	    ) {
		p_ptr->music_monster = -2;
		if (p_ptr->wpos.wz == 0) Send_music(Ind, 1); /* 'generic town' music instead of Bree default */
		else {
			//47 and 48 are actually pieces used in other arena events
			if (rand_int(2)) Send_music(Ind, 47);
			else Send_music(Ind, 48);
		}
		return;
	}
#endif

	if (p_ptr->wpos.wz != 0) {
		l_ptr = getfloor(&p_ptr->wpos);
		if (p_ptr->wpos.wz < 0)
			i = wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].dungeon->type;
		else
			i = wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].tower->type;
	}


	if (getlevel(&p_ptr->wpos) == 196) {
		//Zu-Aon
		//hack: init music as 'higher priority than boss-specific':
		p_ptr->music_monster = -2;
		Send_music(Ind, 45);
		return;
	} else if ((i != -1) && (l_ptr->flags1 & LF1_NO_GHOST)) {
		//Morgoth
		//hack: init music as 'higher priority than boss-specific':
		p_ptr->music_monster = -2;
		Send_music(Ind, 44);
		return;
	}

	if (getlevel(&p_ptr->wpos) == 200) {
		//hack: init music as 'higher priority than boss-specific':
		p_ptr->music_monster = -2;
		Send_music(Ind, 8); //Valinor
		return;
	} else if (p_ptr->wpos.wx == WPOS_PVPARENA_X &&
	    p_ptr->wpos.wy == WPOS_PVPARENA_Y && p_ptr->wpos.wz == WPOS_PVPARENA_Z) {
		//hack: init music as 'higher priority than boss-specific':
		p_ptr->music_monster = -2;
		Send_music(Ind, 47); //PvP Arena
		return;
	} else if (ge_special_sector && p_ptr->wpos.wx == WPOS_ARENA_X &&
	    p_ptr->wpos.wy == WPOS_ARENA_Y && p_ptr->wpos.wz == WPOS_ARENA_Z) {
		//hack: init music as 'higher priority than boss-specific':
		p_ptr->music_monster = -2;
		Send_music(Ind, 48); //Monster Arena Challenge
		return;
	} else if (sector00separation && p_ptr->wpos.wx == WPOS_HIGHLANDER_X &&
	    p_ptr->wpos.wy == WPOS_HIGHLANDER_Y && p_ptr->wpos.wz == WPOS_HIGHLANDER_Z) {
		//hack: init music as 'higher priority than boss-specific':
		p_ptr->music_monster = -2;
		Send_music(Ind, 47); //Highlander Tournament (death match phase)
		return;
	}

	/* No-tele grid: Re-use 'terrifying' bgm for this */
	if (zcave && (zcave[p_ptr->py][p_ptr->px].info & CAVE_STCK)) {
#if 0 /* hack: init music as 'higher priority than boss-specific': */
		p_ptr->music_monster = -2;
#endif
		Send_music(Ind, 46); //No-Tele vault
		return;
	}

	/* rest of the music has lower priority than already running, boss-specific music */
	if (p_ptr->music_monster != -1) return;


	/* on world surface */
	if (p_ptr->wpos.wz == 0) {
		/* play town music also in its surrounding area of houses, if we're coming from the town? */
		if (istownarea(&p_ptr->wpos, 2)) {
			worldpos tpos = {0, 0, 0};
			int x, y, tmus = 0;

			for (x = p_ptr->wpos.wx - 2; x <= p_ptr->wpos.wx + 2; x++) {
				for (y = p_ptr->wpos.wy - 2; y <= p_ptr->wpos.wy + 2; y++) {
					if (x < 0 || x > 63 || y < 0 || y > 63) continue;
					tpos.wx = x; tpos.wy = y;
					if (istown(&tpos)) break;
				}
				if (istown(&tpos)) break;
			}

			for (i = 0; i < numtowns; i++)
				if (town[i].x == tpos.wx && town[i].y == tpos.wy) {
					switch (town[i].type) {
					default:
					case 0: tmus = 1; break; //default town
					case 1: tmus = 3; break; //Bree
					case 2: tmus = 4; break;
					case 3: tmus = 5; break;
					case 4: tmus = 6; break;
					case 5: tmus = 7; break;
					}
				}

			/* now the specialty: If we're coming from elsewhere,
			   we only switch to town music when we enter the town.
			   If we're coming from the town, however, we keep the
			   music while being in its surrounding area of houses. */
			if (istown(&p_ptr->wpos) || p_ptr->music_current == tmus)
				Send_music(Ind, tmus);
			else if (night_surface) Send_music(Ind, 10);
			else Send_music(Ind, 9);
			return;
		} else {
			if (night_surface) Send_music(Ind, 10);
			else Send_music(Ind, 9);
			return;
		}
	/* in the dungeon */
	} else {
		/* Dungeon towns have their own music to bolster the player's motivation ;) */
		if (isdungeontown(&p_ptr->wpos)) {
			Send_music(Ind, 2);
			return;
		}

		/* Floor-specific music (monster/boss-independant)? */
		if ((i != 6) /*not in Nether Realm, really ^^*/
		    && (!(d_info[i].flags2 & DF2_NO_DEATH)) /* nor in easy dungeons */
		    && !(p_ptr->wpos.wx == WPOS_PVPARENA_X /* and not in pvp-arena */
		    && p_ptr->wpos.wy == WPOS_PVPARENA_Y && p_ptr->wpos.wz == WPOS_PVPARENA_Z))
		{
			if (p_ptr->distinct_floor_feeling || is_admin(p_ptr)) {
				if (l_ptr->flags2 & LF2_OOD_HI) {
					Send_music(Ind, 46);
					return;
				}
			}
		}

		//we could just look through audio.lua, by querying get_music_name() I guess..
		switch (i) {
		default:
		case 0:
			if (d_info[i].flags2 & DF2_NO_DEATH) Send_music(Ind, 12);
			else if (d_info[i].flags2 & DF2_IRON) Send_music(Ind, 13);
			else if ((d_info[i].flags2 & DF2_HELL) || (d_info[i].flags1 & DF1_FORCE_DOWN)) Send_music(Ind, 14);
			else Send_music(Ind, 11);
			return;
		case 1: Send_music(Ind, 32); return; //Mirkwood
		case 2: Send_music(Ind, 17); return; //Mordor
		case 3: Send_music(Ind, 19); return; //Angband
		case 4: Send_music(Ind, 16); return; //Barrow-Downs
		case 5: Send_music(Ind, 21); return; //Mount Doom
		case 6: Send_music(Ind, 22); return; //Nether Realm
		case 7: Send_music(Ind, 35); return; //Submerged Ruins
		case 8: Send_music(Ind, 26); return; //Halls of Mandos
		case 9: Send_music(Ind, 30); return; //Cirith Ungol
		case 10: Send_music(Ind, 28); return; //The Heart of the Earth
		case 16: Send_music(Ind, 18); return; //The Paths of the Dead
		case 17: Send_music(Ind, 37); return; //The Illusory Castle
		case 18: Send_music(Ind, 39); return; //The Maze
		case 19: Send_music(Ind, 20); return; //The Orc Cave
		case 20: Send_music(Ind, 36); return; //Erebor
		case 21: Send_music(Ind, 27); return; //The Old Forest
		case 22: Send_music(Ind, 29); return; //The Mines of Moria
		case 23: Send_music(Ind, 34); return; //Dol Guldur
		case 24: Send_music(Ind, 31); return; //The Small Water Cave
		case 25: Send_music(Ind, 38); return; //The Sacred Land of Mountains
		case 26: Send_music(Ind, 24); return; //The Land of Rhun
		case 27: Send_music(Ind, 25); return; //The Sandworm Lair
		case 28: Send_music(Ind, 33); return; //Death Fate
		case 29: Send_music(Ind, 23); return; //The Helcaraxe
		case 30: Send_music(Ind, 15); return; //The Training Tower
		}
	}

	/* Shouldn't happen - send default (dungeon) music */
	Send_music(Ind, 0);
}

/* generate an item-type specific sound, depending on the action applied to it
   action: 0 = pickup, 1 = drop, 2 = wear/wield, 3 = takeoff, 4 = throw, 5 = destroy */
void sound_item(int Ind, int tval, int sval, cptr action) {
	char sound_name[30];
	cptr item = NULL;

	/* action hack: re-use sounds! */
	action = "item_";

	/* choose sound */
	if (is_weapon(tval)) switch(tval) {
		case TV_SWORD: item = "sword"; break;
		case TV_BLUNT:
			if (sval == SV_WHIP) item = "whip"; else item = "blunt";
			break;
		case TV_AXE: item = "axe"; break;
		case TV_POLEARM: item = "polearm"; break;
	} else if (is_armour(tval)) {
		if (is_textile_armour(tval, sval))
			item = "armor_light";
		else
			item = "armor_heavy";
	} else switch(tval) {
		/* equippable stuff */
		case TV_LITE: item = "lightsource"; break;
		case TV_RING: item = "ring"; break;
		case TV_AMULET: item = "amulet"; break;
		case TV_TOOL: item = "tool"; break;
		case TV_DIGGING: item = "tool_digger"; break;
		case TV_MSTAFF: item = "magestaff"; break;
//		case TV_BOOMERANG: item = ""; break;
//		case TV_BOW: item = ""; break;
//		case TV_SHOT: item = ""; break;
//		case TV_ARROW: item = ""; break;
//		case TV_BOLT: item = ""; break;
		/* other items */
		case TV_SCROLL: case TV_PARCHMENT:
			item = "scroll"; break;
/*		case TV_BOTTLE: item = "potion"; break;
		case TV_POTION: case TV_POTION2: case TV_FLASK:
			item = "potion"; break;*/
		case TV_RUNE1: case TV_RUNE2:
			item = "rune"; break;
//		case TV_SKELETON: item = ""; break;
		case TV_FIRESTONE: item = "firestone"; break;
/*		case TV_SPIKE: item = ""; break;
		case TV_CHEST: item = ""; break;
		case TV_JUNK: item = ""; break;
		case TV_TRAPKIT: item = ""; break;
		case TV_STAFF: item = ""; break;
		case TV_WAND: item = ""; break;
		case TV_ROD: item = ""; break;
		case TV_ROD_MAIN: item = ""; break;
		case TV_FOOD: item = ""; break;
		case TV_KEY: item = ""; break;
		case TV_GOLEM: item = ""; break;
		case TV_SPECIAL: item = ""; break;
*/	}

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
	const char *ax;
	ax = quark_str(quark);
	if (ax == NULL) { return FALSE; };
	while ((ax=strchr(ax, '!')) != NULL) {
		while (ax++ != NULL) {
			if (*ax == 0)  {
				return FALSE; /* end of quark, stop */
			}
			if (*ax == ' ' || *ax == '@' || *ax == '#') {
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
				case 'h': /* no house ( sell a a key ) */
				case 'k': /* no destroy */
				case 's': /* no sell */
				case 'v': /* no thowing */
				case '=': /* force pickup */
#if 0
				case 'w': /* no wear/wield */
				case 't': /* no take off */
#endif
					return TRUE;
				};
			};
		};
	};
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
bool suppress_message = FALSE, censor_message = FALSE;
int censor_length = 0;

void msg_print(int Ind, cptr msg_raw)
{
	char msg_dup[MSG_LEN], *msg = msg_dup;
	int line_len = 80; /* maximum length of a text line to be displayed;
		     this is client-dependant, compare c_msg_print (c-util.c) */
	char msg_buf[line_len + 2 + 2 * 80]; /* buffer for 1 line. + 2 bytes for colour code (+2*80 bytes for colour codeeeezz) */
	char msg_minibuf[3]; /* temp buffer for adding characters */
	int text_len, msg_scan = 0, space_scan, tab_spacer = 0, tmp;
	char colour_code = 0;
	bool no_colour_code = FALSE;
	bool first_character = TRUE;
//	bool is_chat = ((msg_raw != NULL) && (strlen(msg_raw) > 2) && (msg_raw[2] == '['));
	bool client_ctrlo = FALSE, client_chat = FALSE, client_all = FALSE;

	/* backward msg window width hack for windows clients (non-x11 clients rather) */
	if (!is_newer_than(&Players[Ind]->version, 4, 4, 5, 3, 0, 0) && !strcmp(Players[Ind]->realname, "PLAYER")) line_len = 72;

	/* Pfft, sorry to bother you.... --JIR-- */
	if (suppress_message) return;

	/* no message? */
	if (msg_raw == NULL) return;

	strcpy(msg_dup, msg_raw); /* in case msg_raw was constant */
	/* censor swear words? */
	if (censor_message && Players[Ind]->censor_swearing)
		/* skip the name of the sender, etc. */
		handle_censor(msg + strlen(msg) - censor_length);

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
///			colour_code = 0;
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
					if (msg[msg_scan + 1] == '\0') {
						msg_scan++;
						continue;
					}
					/* Capture double \377 which stand for a normal { char instead of a colour code: */
					if (msg[msg_scan + 1] != '\377') {
						msg_minibuf[0] = msg[msg_scan];
						msg_scan++;
						msg_minibuf[1] = msg[msg_scan];
						colour_code = msg[msg_scan];
						msg_scan++;
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

				/* Process text.. */
				first_character = FALSE;
				msg_minibuf[0] = msg[msg_scan];
				msg_minibuf[1] = '\0';
				strcat(msg_buf, msg_minibuf);
				msg_scan++;
				text_len++;
				/* Avoid cutting words in two */
				if ((text_len == line_len) && (msg[msg_scan] != '\0') &&
				    ((msg[msg_scan - 1] >= 'A' && msg[msg_scan - 1] <= 'Z') ||
				    (msg[msg_scan - 1] >= '0' && msg[msg_scan - 1] <= '9') ||
#if 0
				    (msg[msg_scan - 1] == '(' || msg[msg_scan - 1] == ')') ||
				    (msg[msg_scan - 1] == '[' || msg[msg_scan - 1] == ']') ||
				    (msg[msg_scan - 1] == '{' || msg[msg_scan - 1] == '}') ||
#else
				    (msg[msg_scan - 1] == '(') ||
				    (msg[msg_scan - 1] == '[') ||
				    (msg[msg_scan - 1] == '{') ||
#endif
				    (msg[msg_scan - 1] >= 'a' && msg[msg_scan - 1] <= 'z') ||
				    /* (maybe too much) for pasting items to chat, (+1) or (-2,0) : */
				    (msg[msg_scan - 1] == '+' || msg[msg_scan - 1] == '-') ||
				    (msg[msg_scan - 1] == '\377')) &&
				    ((msg[msg_scan] >= 'A' && msg[msg_scan] <= 'Z') ||
#if 0
				    (msg[msg_scan] == '(' || msg[msg_scan] == ')') ||
				    (msg[msg_scan] == '[' || msg[msg_scan] == ']') ||
				    (msg[msg_scan] == '{' || msg[msg_scan] == '}') ||
#else
				    (msg[msg_scan] == ')') ||
				    (msg[msg_scan] == ']') ||
				    (msg[msg_scan] == '}') ||
#endif
				    (msg[msg_scan] >= '0' && msg[msg_scan] <= '9') ||
				    (msg[msg_scan] >= 'a' && msg[msg_scan] <= 'z') ||
				    /* (maybe too much) for pasting items to chat, (+1) or (-2,0) : */
				    (msg[msg_scan] == '+' || msg[msg_scan] == '-') ||
				    (msg[msg_scan] == '\377'))) {
					space_scan = msg_scan;
					do {
						space_scan--;
					} while (((msg[space_scan - 1] >= 'A' && msg[space_scan - 1] <= 'Z') ||
						(msg[space_scan - 1] >= '0' && msg[space_scan - 1] <= '9') ||
						(msg[space_scan - 1] >= 'a' && msg[space_scan - 1] <= 'z') ||
#if 0
						(msg[space_scan - 1] == '(' || msg[space_scan - 1] == ')') ||
						(msg[space_scan - 1] == '[' || msg[space_scan - 1] == ']') ||
						(msg[space_scan - 1] == '{' || msg[space_scan - 1] == '}') ||
#else
						(msg[space_scan - 1] == '(') ||
						(msg[space_scan - 1] == '[') ||
						(msg[space_scan - 1] == '{') ||
#endif
						/* (maybe too much) for pasting items to chat, (+1) or (-2,0) : */
						(msg[space_scan - 1] == '+' || msg[space_scan - 1] == '-') ||
						(msg[space_scan - 1] == '\377')) &&
						space_scan > 0);

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
	}

	if (msg == NULL) Send_message(Ind, msg);

	return;
#endif // enable line breaks?

	Send_message(Ind, format("%s%s%s",
	    client_chat ? "\375" : (client_all ? "\374" : ""),
	    client_ctrlo ? "\376" : "",
	    msg));
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

void msg_admins(int Ind, cptr msg)
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
		
		/* Skip non-admins */
		if (!is_admin(Players[i]))
			continue;
			
		/* Tell this one */
		msg_print(i, msg);
	 }
}

void msg_broadcast_format(int Ind, cptr fmt, ...)
{
//	int i;
	
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
		if (is_admin(p_ptr))
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

/* Whispering: Send message to adjacent players */
void msg_print_verynear(int Ind, cptr msg)
{
	player_type *p_ptr = Players[Ind];
	int y, x, i;
	struct worldpos *wpos;
	wpos=&p_ptr->wpos;

//	if(p_ptr->admin_dm) return;

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

		/* Is he in range? */
		if (abs(p_ptr->py - y) <= 1 && abs(p_ptr->px - x) <= 1) {
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

/* for whispering */
void msg_format_verynear(int Ind, cptr fmt, ...)
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
	msg_print_verynear(Ind, buf);
}

/* location-based - also, skip player Ind if non-zero */
void msg_print_near_site(int y, int x, worldpos *wpos, int Ind, bool view, cptr msg)
{
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
void msg_format_near_site(int y, int x, worldpos *wpos, int Ind, bool view, cptr fmt, ...)
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
void msg_print_near_monster(int m_idx, cptr msg)
{
	int i;
	player_type *p_ptr;
	cave_type **zcave;
	char m_name[MNAME_LEN];

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

/*
 * Dodge Chance Feedback.
 */
void use_ability_blade(int Ind)
{
	player_type *p_ptr = Players[Ind];
#ifndef NEW_DODGING
	int dun_level = getlevel(&p_ptr->wpos);
	int chance = p_ptr->dodge_level - (dun_level * 5 / 6);

	if (chance < 0) chance = 0;
	if (chance > DODGE_MAX_CHANCE) chance = DODGE_MAX_CHANCE;	// see DODGE_MAX_CHANCE in melee1.c
	if (is_admin(p_ptr))
		msg_format(Ind, "You have exactly %d%% chances of dodging a level %d monster.", chance, dun_level);

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
#else
	int lev;
	lev = p_ptr->lev * 2 < 127 ? p_ptr->lev * 2 : 127;
	if (is_admin(p_ptr))
		msg_format(Ind, "You have exactly %d%%/%d%% chance of dodging a level %d/%d monster.",
		    apply_dodge_chance(Ind, p_ptr->lev), apply_dodge_chance(Ind, lev), p_ptr->lev, lev);

/*	msg_format(Ind, "You have %s/%s chance of dodging a level %d/%d monster.",
		dodge_diz(apply_dodge_chance(Ind, p_ptr->lev)), dodge_diz(apply_dodge_chance(Ind, lev)),
		p_ptr->lev, lev);
*/
	msg_format(Ind, "You have %s chance of dodging a level %d monster.",
	    dodge_diz(apply_dodge_chance(Ind, p_ptr->lev)), p_ptr->lev);
#endif
	return;
}



void check_parryblock(int Ind)
{
#ifdef ENABLE_NEW_MELEE
	player_type *p_ptr = Players[Ind];
	if (is_admin(p_ptr)) {
		msg_format(Ind, "You have exactly %d%%/%d%% base chance of parrying/blocking.", 
			p_ptr->weapon_parry, p_ptr->shield_deflect);
		msg_format(Ind, "You have exactly %d%%/%d%% real chance of parrying/blocking.", 
			apply_parry_chance(p_ptr, p_ptr->weapon_parry), apply_block_chance(p_ptr, p_ptr->shield_deflect));
	} else {
		if (!apply_parry_chance(p_ptr, p_ptr->weapon_parry))
			msg_print(Ind, "You cannot parry at the moment.");
		else if (apply_parry_chance(p_ptr, p_ptr->weapon_parry) < 5)
			msg_print(Ind, "You have almost no chance of parrying.");
		else if (apply_parry_chance(p_ptr, p_ptr->weapon_parry) < 10)
			msg_print(Ind, "You have a slight chance of parrying.");
		else if (apply_parry_chance(p_ptr, p_ptr->weapon_parry) < 20)
			msg_print(Ind, "You have a significant chance of parrying.");
		else if (apply_parry_chance(p_ptr, p_ptr->weapon_parry) < 30)
			msg_print(Ind, "You have a good chance of parrying.");
		else if (apply_parry_chance(p_ptr, p_ptr->weapon_parry) < 40)
			msg_print(Ind, "You have a very good chance of parrying.");
		else if (apply_parry_chance(p_ptr, p_ptr->weapon_parry) < 50)
			msg_print(Ind, "You have an excellent chance of parrying.");
		else
			msg_print(Ind, "You have a superb chance of parrying.");

		if (!apply_block_chance(p_ptr, p_ptr->shield_deflect))
			msg_print(Ind, "You cannot block at the moment.");
		else if (apply_block_chance(p_ptr, p_ptr->shield_deflect) < 5)
			msg_print(Ind, "You have almost no chance of blocking.");
		else if (apply_block_chance(p_ptr, p_ptr->shield_deflect) < 14)
			msg_print(Ind, "You have a slight chance of blocking.");
		else if (apply_block_chance(p_ptr, p_ptr->shield_deflect) < 23)
			msg_print(Ind, "You have a significant chance of blocking.");
		else if (apply_block_chance(p_ptr, p_ptr->shield_deflect) < 33)
			msg_print(Ind, "You have a good chance of blocking.");
		else if (apply_block_chance(p_ptr, p_ptr->shield_deflect) < 44)
			msg_print(Ind, "You have a very good chance of blocking.");
		else if (apply_block_chance(p_ptr, p_ptr->shield_deflect) < 55)
			msg_print(Ind, "You have an excellent chance of blocking.");
		else
			msg_print(Ind, "You have a superb chance of blocking.");
	}
#endif
	return;
}



void toggle_shoot_till_kill(int Ind)
{
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

void toggle_dual_mode(int Ind)
{
	player_type *p_ptr = Players[Ind];
	if (p_ptr->dual_mode)
		msg_print(Ind, "\377wDual-wield mode: Main-hand. (This disables all dual-wield boni.)");
	else
		msg_print(Ind, "\377wDual-wield mode: Dual-hand.");
	p_ptr->dual_mode = !p_ptr->dual_mode;
s_printf("DUAL_MODE: Player %s toggles %s.\n", p_ptr->name, p_ptr->dual_mode ? "true" : "false");
	p_ptr->redraw |= PR_STATE | PR_PLUSSES;
	calc_boni(Ind);
	return;
}

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
#define EXEMPT_BROKEN_SWEARWORDS	/* don't 'recognize' swear words that are broken up into 'innocent' parts */
#define HIGHLY_EFFECTIVE_CENSOR		/* strip all kinds of non-alpha chars too? */
#define CENSOR_PH_TO_F			/* (slightly picky) convert ph to f ?*/
#define REDUCE_DUPLICATE_H		/* (slightly picky) reduce multiple h to just one? */
#define REDUCE_H_CONSONANT		/* (slightly picky) drop h before consonants? */
static int censor_aux(char *buf, char *lcopy, int *c, bool leet) {
	int i, j, k, offset, cc[MSG_LEN], pos, eff_len;
	char line[MSG_LEN];
	char *word, l0, l1, l2, l3;
	int level = 0;

	/* create working copies */
	strcpy(line, buf);
	for (i = 0; !i || c[i]; i++) cc[i] = c[i];
	cc[i] = 0;

	/* replace certain non-alpha chars by alpha chars (leet speak detection)? */
	if (leet) {
		i = 0;
		while (lcopy[i]) {
			switch (lcopy[i]) {
			case '!': lcopy[i] = 'i'; break;
			case '$': lcopy[i] = 's'; break;
			case '|': lcopy[i] = 'i'; break;
			case '(': lcopy[i] = 'i'; break;
			case ')': lcopy[i] = 'i'; break;
			case '/': lcopy[i] = 'i'; break;
			case '\\': lcopy[i] = 'i'; break;
			case '+': lcopy[i] = 't'; break;
			case '1': lcopy[i] = 'i'; break;
			case '3': lcopy[i] = 'e'; break;
			case '4': lcopy[i] = 'a'; break;
			case '5': lcopy[i] = 's'; break;
			case '7': lcopy[i] = 't'; break;
			case '8': lcopy[i] = 'b'; break;
			case '0': lcopy[i] = 'o'; break;
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
		    ) {
			j++;
		}

		/* build index map for stripped string */
		cc[i] = cc[j];
		lcopy[i] = lcopy[j];

		i++;
		j++;
	}
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
	lcopy[i] = '\0';
	cc[i] = 0;
#endif

#ifdef HIGHLY_EFFECTIVE_CENSOR
	/* check for non-alpha chars and _drop_ them (!) */
	i = j = 0;
	while (lcopy[j]) {
		if ((lcopy[j] < 'a' || lcopy[j] > 'z') &&
		    lcopy[j] != 'Z') {
			j++;
			continue;
		}

		/* modify index map for stripped string */
		cc[i] = cc[j];
		lcopy[i] = lcopy[j];
		i++;
		j++;
	}
	lcopy[i] = '\0';
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

		switch (lcopy[j]) {
		case 'a': case 'e': case 'i': case 'o': case 'u': case 'y':
			if (lcopy[j + 1] == lcopy[j] &&
			    lcopy[j + 2] == lcopy[j]) {
				j++;
				continue;
			}
			break;
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

	/* check for swear words and censor them */
	for (i = 0; swear[i].word[0]; i++) {
		offset = 0;

		/* check for multiple occurrances of this swear word */
		while ((word = censor_strstr(lcopy + offset, swear[i].word, &eff_len))) {
			pos = word - lcopy;
			l0 = tolower(line[cc[pos]]);
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
			offset = pos + strlen(swear[i].word);

#ifdef EXEMPT_BROKEN_SWEARWORDS
			/* hm, maybe don't track swear words if proceeded and postceeded by some other letters
			   and separated by spaces inside it -- and if those nearby letters aren't the same letter,
			   if someone just duplicates it like 'sshitt' */
			/* check for swear-word-preceding non-duplicate alpha char */
			if (cc[pos] > 0 &&
			    l1 >= 'a' && l1 <= 'z' &&
			    l1 != l0) {
				/* special treatment for swear words of <= 3 chars length: */
				if (strlen(swear[i].word) <= 3) {
					/* if there's UP TO 2 other chars before it or exactly 1 non-duplicate char, it's exempt.
					   (for more leading chars, nonswear has to be used.) */
					if (cc[pos] == 1 && l1 >= 'a' && l1 <= 'z' && l1 != l0) break;
					if (cc[pos] == 2) {
						if (l1 >= 'a' && l1 <= 'z' && l2 >= 'a' && l2 <= 'z' &&
						    (l0 != l1 || l0 != l2 || l1 != l2)) break;
						/* also test for only 1 leading alpha char */
						if ((l2 < 'a' || l2 > 'z') &&
						    l1 >= 'a' && l1 <= 'z' && l1 != l0) break;
					}
					if (cc[pos] >= 3) {
						 if ((l3 < 'a' || l3 > 'z') &&
						    l1 >= 'a' && l1 <= 'z' && l2 >= 'a' && l2 <= 'z' &&
						    (l0 != l1 || l0 != l2 || l1 != l2)) break;
						/* also test for only 1 leading alpha char */
						if ((l2 < 'a' || l2 > 'z') &&
						    l1 >= 'a' && l1 <= 'z' && l1 != l0) break;
					}
					/* if there's no char before it but 2 other chars after it or 1 non-dup after it, it's exempt. */
					//TODO maybe - or just use nonswear for that
				}

				/* check that the swear word occurance was originally non-continuous, ie separated by space etc.
				   (if this is FALSE then this is rather a case for nonswearwords.txt instead.) */
				for (j = cc[pos]; j < cc[pos + strlen(swear[i].word) - 1]; j++) {
					if (tolower(line[j]) < 'a' || tolower(line[j] > 'z')) {
						/* heuristical non-swear word found! */
						break;
					}
				}
				/* found? */
				if (j < cc[pos + strlen(swear[i].word) - 1]) continue;
			}
#endif

#if 0
			/* censor it! */
			for (j = 0; j < eff_len); j++) {
				line[cc[pos + j]] = buf[cc[pos + j]] = '*';
#else
			/* actually censor separator chars too, just so it looks better ;) */
			for (j = 0; j < eff_len - 1; j++) {
				for (k = cc[pos + j]; k <= cc[pos + j + 1]; k++)
					line[k] = buf[k] = '*';
#endif

				/* for processing lcopy in a while loop instead of just 'if'
				   -- OBSOLETE ACTUALLY (thanks to 'offset')? */
				lcopy[pos + j] = '*';
			}
			level = MAX(level, swear[i].level);
		}
	}
	return(level);
}
static int censor(char *line) {
	int i, j, cc[MSG_LEN], offset;
	char lcopy[MSG_LEN], lcopy2[MSG_LEN], tmp[MSG_LEN];
	char *word;

	strcpy(lcopy, line);

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

	/* check for legal words first */
	strcpy(lcopy2, lcopy); /* use a 'working copy' to allow _overlapping_ nonswear words */
	for (i = 0; nonswear[i][0]; i++) {
		offset = 0;
		/* check for multiple occurrances of this nonswear word */
		while ((word = strstr(lcopy + offset, nonswear[i]))) {
			/* prevent checking the same occurance repeatedly */
			offset = word - lcopy + strlen(nonswear[i]);

			/* prevent it from getting tested for swear words */
			for (j = 0; j < strlen(nonswear[i]); j++)
				lcopy2[(word - lcopy) + j] = 'Z';
		}
	}
	strcpy(lcopy, lcopy2);

	/* perform two more 'parallel' tests  */
	strcpy(tmp, line);
	i = censor_aux(line, lcopy, cc, FALSE); /* continue with normal check  */
	j = censor_aux(tmp, lcopy2, cc, TRUE); /* continue with leet speek check */

	if (j > i) {
		strcpy(line, tmp);
		return j;
	}
	return i;
}

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
static void player_talk_aux(int Ind, char *message)
{
 	int i, j, len, target = 0;
	char search[MSG_LEN], sender[MAX_CHARS];
	char message2[MSG_LEN];
	player_type *p_ptr = Players[Ind], *q_ptr;
 	char *colon;
	bool me = FALSE, log = TRUE;
	char c_n = 'B', c_b = 'B'; /* colours of sender name and of brackets (unused atm) around this name */
	int mycolor = 0;
	bool admin = FALSE;
	bool broadcast = FALSE;

#ifdef TOMENET_WORLDS
	char tmessage[MSG_LEN];		/* TEMPORARY! We will not send the name soon */
#endif

	/* Get sender's name */
	if (Ind) {
		/* Get player name */
		strcpy(sender, p_ptr->name);
		admin = is_admin(p_ptr);
	} else {
		/* Default name */
		strcpy(sender, "Server Admin");
	}

#if 0
	if (p_ptr->muted && !admin) return;		/* moved down there - the_sandman */
#endif

	/* Default to no search string */
	strcpy(search, "");

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

	colon = strchr(message, ':');

	/* Ignore "smileys" or URL */
//	if (colon && strchr(")(-/:", colon[1]))
	/* (C. Blue) changing colon parsing. :: becomes
	    textual :  - otherwise : stays control char */
	if (colon) {
		bool smiley = FALSE;
		/* if another colon followed this one then
		   it was not meant to be a control char */
		switch(colon[1]){
		/* accept these chars for smileys, but only if the colon is either first in the line or stands after a SPACE,
		   otherwise it is to expect that someone tried to write to party/privmsg */
		case '(':	case ')':
		case '[':	case ']':
		case '{':	case '}':
/* staircases, so cannot be used for smileys here ->		case '<':	case '>': */
		case '\\':	case '|':
		case 'p': case 'P': case 'o': case 'O':
			if (message == colon || colon[-1] == ' ' || colon[-1] == '>' || /* >:) -> evil smiley */
			    ((message == colon - 1) && (colon[-1] != '!') && (colon[-1] != '#') && (colon[-1] != '%') && (colon[-1] != '$'))) /* <- party names must be at least 2 chars then */
				colon = NULL; /* the check is mostly important for '(' */
			break;
		case '-':
			if (message == colon || colon[-1] == ' ' || colon[-1] == '>' || /* here especially important: '-' often is for numbers/recall depth */
			    ((message == colon - 1) && (colon[-1] != '!') && (colon[-1] != '#') && (colon[-1] != '%') && (colon[-1] != '$'))) /* <- party names must be at least 2 chars then */
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
				do message[i] = message[i+1];
				while(message[i++]!='\0');
				colon = NULL;
				break;
			}

			/* new hack: ..but only if the previous two chars aren't  !:  (party chat),
			   and if it's appearently meant to be a smiley. */
			if ((colon - message == 1) && (colon[-1]=='!' || colon[-1]=='#' || colon[-1]=='%' || colon[-1]=='$'))
			switch (*(colon + 2)) {
			case '(': case ')':
			case '[': case ']':
			case '{': case '}':
			case '<': case '>':
			case '-': case '|':
			case 'p': case 'P': case 'o': case 'O': case 'D':
			smiley = TRUE; break; }
			
			/* check for smiley at end of the line */
			if ((message + strlen(message) - colon >= 3) &&
			    (message + strlen(message) - colon <= 4)) // == 3 for 2-letter-smileys only.
//			if ((*(colon + 2)) && !(*(colon + 3)))
			switch (*(colon + 2)) {
			case '(': case ')':
			case '[': case ']':
			case '{': case '}':
			case '<': case '>':
			case '-': case '/':
			case '\\': case '|':
			case 'p': case 'P': case 'o': case 'O': case 'D':
			smiley = TRUE; break; }

			if (smiley) break;

			/* Pretend colon wasn't there */
			i = (int) (colon - message);
			do message[i] = message[i+1];
			while(message[i++]!='\0');
			colon = NULL;
			break;
		case '\0':
			/* if colon is last char in the line, it's not a priv/party msg. */
#if 0 /* disabled this 'feature', might be more convenient - C. Blue */
			colon = NULL;
#endif
			break;
		}
	}

	/* don't log spammy slash commands */
	if (prefix(message, "/untag ")) log = FALSE;

	/* no big brother */
//	if(cfg.log_u && (!colon || message[0] != '#' || message[0] != '/')){ /* message[0] != '#' || message[0] != '/' is always true -> big brother mode - mikaelh */
	if (cfg.log_u && (!colon) && log) {
		/* Shorten multiple identical messages to prevent log file spam,
		   eg recall rod activation attempts. - Exclude admins. */
		if (is_admin(p_ptr))
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
		if (p_ptr->msg-last < 6) {
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
		if (p_ptr->msg-last > 240 && p_ptr->spam) p_ptr->spam--;
		p_ptr->msgcnt = 0;
	}
	if (p_ptr->spam > 1 || p_ptr->muted) return;
#endif
	process_hooks(HOOK_CHAT, "d", Ind);

	if (++p_ptr->talk > 10) {
		imprison(Ind, 30, "talking too much.");
		return;
	}

	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		Players[i]->talk = 0;
	}

	/* Special function '!:' at beginning of message sends to own party - sorry for hack, C. Blue */
//	if ((strlen(message) >= 1) && (message[0] == ':') && (!colon) && (p_ptr->party))
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
			party_msg_format_ignoring(Ind, target, "\375\377%c[(P) %s] %s", COLOUR_CHAT_PARTY, sender, message + 2);
#else
			party_msg_format_ignoring(Ind, target, "\375\377%c[%s:%s] %s", COLOUR_CHAT_PARTY, parties[target].name, sender, message + 2);
#endif
//			party_msg_format_ignoring(Ind, target, "\375\377%c[%s:%s] %s", COLOUR_CHAT_PARTY, parties[target].name, sender, message + 1);
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
				msg_format_near(Ind, "\377%c%^s says: %s", COLOUR_CHAT, p_ptr->name, message + 2);
				msg_format(Ind, "\377%cYou say: %s", COLOUR_CHAT, message + 2);
			} else {
				msg_format_near(Ind, "\377%c%s clears %s throat.", COLOUR_CHAT, p_ptr->name, p_ptr->male ? "his" : "her");
				msg_format(Ind, "\377%cYou clear your throat.", COLOUR_CHAT);
			}
#endif
			return;
		}

		/* Send message to target floor */
		if (p_ptr->mutedchat < 2) {
			floor_msg_format_ignoring(Ind, &p_ptr->wpos, "\375\377%c[%s] %s", COLOUR_CHAT_LEVEL, sender, message + 2);
		}

		/* Done */
		return;
	}

	/* '%:' at the beginning sends to self - mikaelh */
	if ((strlen(message) >= 2) && (message[0] == '%') && (message[1] == ':') && (colon)) {
		/* Send message to self */
		msg_format(Ind, "\377o<%%> \377w%s", message + 2);

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
		if (!p_ptr->reply_name || !strlen(p_ptr->reply_name)) {
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
			guild_msg_format(p_ptr->guild, "\375\377y[\377%c(G) %s\377y]\377%c %s", COLOUR_CHAT_GUILD, sender, COLOUR_CHAT_GUILD, message + 2);
#else
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
		struct rplist *w_player;

		/* NAME_LOOKUP_LOOSE DESPERATELY NEEDS WORK */
		target = name_lookup_loose_quiet(Ind, search, TRUE);
#ifdef TOMENET_WORLDS
		if (!target && cfg.worldd_privchat) {
			w_player = world_find_player(search, 0);
			if (w_player) {
				world_pmsg_send(p_ptr->id, p_ptr->name, w_player->name, colon + 1);
				msg_format(Ind, "\375\377s[%s:%s] %s", p_ptr->name, w_player->name, colon + 1);

				/* hack: assume that the target player will become the
				   one we want to 'reply' to, afterwards, if we don't
				   have a reply-to target yet. */
				if (!p_ptr->reply_name || !strlen(p_ptr->reply_name))
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
//			msg_format(Ind, "(no receipient for: %s)", colon);
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
		if (!check_ignore(target, Ind)) {
			/* Set target player */
			q_ptr = Players[target];
			/* Remember sender, to be able to reply to him quickly */
			strcpy(q_ptr->reply_name, sender);

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
			strcpy(p_ptr->reply_name, q_ptr->name);
#endif

			exec_lua(0, "chat_handler()");
			return;
		} else {
			/* Tell the sender */
			msg_print(Ind, "(That player has ignored you)");
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

		/* Send message to target party */
		party_msg_format_ignoring(Ind, 0 - target, "\375\377%c[(P) %s] %s", COLOUR_CHAT_PARTY, sender, colon);
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

	if (strlen(message) > 1) mycolor = (prefix(message, "}") && (color_char_to_attr(*(message + 1)) != -1))?2:0;

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
		if ((p_ptr->mode & MODE_HARD) && (p_ptr->mode & MODE_NO_GHOST))
			c_b = 'r'; /* hellish mode */
		else c_b = c_n;
	}

	/* Admins have exclusive colour - the_sandman */
	if (c_n == 'b' && !is_admin(p_ptr)) return;

	strcpy(message2, message);
	j = handle_censor(message2);
	censor_message = TRUE;

#ifdef TOMENET_WORLDS
	if (broadcast) {
		snprintf(tmessage, sizeof(tmessage), "\375\377r[\377%c%s\377r] \377%c%s", c_n, sender, COLOUR_CHAT, message + 11);
		censor_length = strlen(message + 11);
	} else if (!me) {
		snprintf(tmessage, sizeof(tmessage), "\375\377%c[%s] \377%c%s", c_n, sender, COLOUR_CHAT, message + mycolor);
		censor_length = strlen(message + mycolor);
	} else {
		/* Why not... */
		if (strlen(message) > 4) mycolor = (prefix(&message[4], "}") && (color_char_to_attr(*(message + 5)) != -1)) ? 2 : 0;
		else {
			censor_message = FALSE;
			return;
		}
		if (mycolor) c_n = message[5];
		snprintf(tmessage, sizeof(tmessage), "\375\377%c[%s %s]", c_n, sender, message + 4 + mycolor);
		censor_length = strlen(message + 4 + mycolor);
	}
 #if 0
	if (((broadcast && cfg.worldd_broadcast) || (!broadcast && cfg.worldd_pubchat))
	    && !(len && (target != 0) && !cfg.worldd_privchat)) /* privchat = to party or to person */
		world_chat(p_ptr->id, tmessage);	/* no ignores... */
 #else
		/* Incoming chat is now filtered instead of outgoing which allows IRC relay to get public chat messages from worldd - mikaelh */
		world_chat(p_ptr->id, tmessage);	/* no ignores... */
 #endif
	for(i = 1; i <= NumPlayers; i++){
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
	censor_length = strlen(message + 4);
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
			censor_length = strlen(message + 11);
			msg_format(i, "\375\377r[\377%c%s\377r] \377%c%s", c_n, sender, COLOUR_CHAT, message + 11);
		} else if (!me) {
			censor_length = strlen(message + mycolor);
			msg_format(i, "\375\377%c[%s] \377%c%s", c_n, sender, COLOUR_CHAT, message + mycolor);
			/* msg_format(i, "\375\377%c[%s] %s", Ind ? 'B' : 'y', sender, message); */
		}
		else {
			censor_length = strlen(message + 4);
			msg_format(i, "%s %s", sender, message + 4);
		}
	}
#endif
	p_ptr->warning_chat = 1;
	censor_message = FALSE;
	handle_punish(Ind, j);

	exec_lua(0, "chat_handler()");

}

/* Check for swear words, censor + punish */
int handle_censor(char *message) {
	return censor(message);
}
void handle_punish(int Ind, int level) {
	switch (level) {
	case 0:
		break;
	case 1:
		msg_print(Ind, "Please do not swear.");
		break;
	default:
		imprison(Ind, level * 20, "swearing");
	}
}

/* toggle AFK mode off if it's currently on, also reset idle time counter for in-game character.
   required for cloaking mode! - C. Blue */
void un_afk_idle(int Ind) {
	Players[Ind]->idle_char = 0;
	if (Players[Ind]->afk) toggle_afk(Ind, "");
	stop_cloaking(Ind);
}

/*
 * toggle AFK mode on/off.	- Jir -
 */
void toggle_afk(int Ind, char *msg)
{
	player_type *p_ptr = Players[Ind];
	char afk[MAX_CHARS];
	int i;

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
				snprintf(afk, sizeof(afk), "\374\377%c%s has returned from AFK.", COLOUR_AFK, p_ptr->name);
			else
				snprintf(afk, sizeof(afk), "\374\377%c%s has returned from AFK. (%s\377%c)", COLOUR_AFK, p_ptr->name, p_ptr->afk_msg, COLOUR_AFK);
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
				snprintf(afk, sizeof(afk), "\374\377%c%s seems to be AFK now.", COLOUR_AFK, p_ptr->name);
			else
				snprintf(afk, sizeof(afk), "\374\377%c%s seems to be AFK now. (%s\377%c)", COLOUR_AFK, p_ptr->name, p_ptr->afk_msg, COLOUR_AFK);
		}
		p_ptr->afk = TRUE;

		/* actually a hint for newbie rogues couldn't hurt */
		if (p_ptr->tim_blacklist)
			msg_print(Ind, "\377yNote: Your blacklist timer won't decrease while AFK.");

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
int name_lookup_loose(int Ind, cptr name, u16b party)
{
	int i, j, len, target = 0;
	player_type *q_ptr, *p_ptr;
	cptr problem = "";
	bool party_online;

	p_ptr=Players[Ind];

	/* don't waste time */
	if(p_ptr==(player_type*)NULL) return(0);

	/* Acquire length of search string */
	len = strlen(name);

	/* Look for a recipient who matches the search string */
	if (len) {
		if (party) {
			/* First check parties */
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

		/* Then check players */
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
	}

	/* Check for recipient set but no match found */
	if ((len && !target) || (target < 0 && !party)) {
		/* Send an error message */
		msg_format(Ind, "Could not match name '%s'.", name);

		/* Give up */
		return 0;
	}

	/* Check for multiple recipients found */
	if (strlen(problem)) {
		/* Send an error message */
		msg_format(Ind, "'%s' matches too many %s.", name, problem);

		/* Give up */
		return 0;
	}

	/* Success */
	return target;
}

/* same as name_lookup_loose, but without warning message if no name was found */
int name_lookup_loose_quiet(int Ind, cptr name, u16b party)
{
	int i, j, len, target = 0;
	player_type *q_ptr, *p_ptr;
	cptr problem = "";
	bool party_online;

	p_ptr = Players[Ind];

	/* don't waste time */
	if(p_ptr == (player_type*)NULL) return(0);

	/* Acquire length of search string */
	len = strlen(name);

	/* Look for a recipient who matches the search string */
	if (len) {
		if (party) {
			/* First check parties */
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

		/* Then check players */
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
	}

	/* Check for recipient set but no match found */
	if ((len && !target) || (target < 0 && !party)) {
		/* Give up */
		return 0;
	}

	/* Check for multiple recipients found */
	if (strlen(problem)) {
		/* Send an error message */
		msg_format(Ind, "'%s' matches too many %s.", name, problem);

		/* Give up */
		return 0;
	}

	/* Success */
	return target;
}

/* copy/pasted from name_lookup_loose(), just without being loose.. */
int name_lookup(int Ind, cptr name, u16b party)
{
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
		if (party) {
			/* First check parties */
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

		/* Then check players */
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
	}

	/* Check for recipient set but no match found */
	if ((len && !target) || (target < 0 && !party)) {
		/* Send an error message */
		msg_format(Ind, "Could not match name '%s'.", name);

		/* Give up */
		return 0;
	}

	/* Success */
	return target;
}

/* copy/pasted from name_lookup_loose(), just without being loose.. but with quiet */
int name_lookup_quiet(int Ind, cptr name, u16b party)
{
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
		if (party) {
			/* First check parties */
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

		/* Then check players */
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
	}

	/* Check for recipient set but no match found */
	if ((len && !target) || (target < 0 && !party)) {
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
	int i = Ind, d = 0, n;
	cptr desc = "";
	bool ville = istown(wpos) && !isdungeontown(wpos);
	dungeon_type *d_ptr;

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
				return (format("%dft %s", wpos->wz * 50, d_info[d].name + d_name));
			else
				return (format("%s", desc));
	} else {
		if (Ind >= 0 || (!d && !ville)) {
			return (format("Lv %d of (%d,%d)", wpos->wz, wpos->wx, wpos->wy));
		} else
			if (!ville)
				return (format("Lv %d %s", wpos->wz, d_info[d].name + d_name));
			else
				return (format("%s", desc));
	}
}
char *wpos_format_compact(int Ind, worldpos *wpos)
{
	int d = 0, n;
	cptr desc = "";
	bool ville = istown(wpos) && !isdungeontown(wpos);
	dungeon_type *d_ptr;

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

	if (ville) return (format("%s", desc));
	else {
		if (Players[Ind]->depth_in_feet)
			return (format("%dft (%d,%d)", wpos->wz * 50, wpos->wx, wpos->wy));
		else
			return (format("Lv %d (%d,%d)", wpos->wz, wpos->wx, wpos->wy));
	}
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

	if (name == (char*)NULL) return(-1);
	for(i = 1; i <= NumPlayers; i++) {
		if(Players[i]->conn == NOT_CONNECTED) continue;
		if(!stricmp(Players[i]->name, name)) return(i);
	}
	return(-1);
}
int get_playerind_loose(char *name)
{
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
bool show_floor_feeling(int Ind)
{
	player_type *p_ptr = Players[Ind];
	worldpos *wpos = &p_ptr->wpos;
	struct dungeon_type *d_ptr = getdungeon(wpos);
	dun_level *l_ptr = getfloor(wpos);
	bool felt = FALSE;

	/* Hack for Valinor - C. Blue */
	if (getlevel(wpos) == 200) {
		msg_print(Ind, "\374\377gYou have a wonderful feeling of peace...");
		return TRUE;
	}

	/* XXX devise a better formula */
	if (p_ptr->lev * ((p_ptr->lev >= 40) ? 3 : 2) + 5 < getlevel(wpos))
	{
		msg_print(Ind, "\374\377oYou feel an imminent danger!");
		felt = TRUE;
	}

	if (!l_ptr) return(felt);

#ifdef EXTRA_LEVEL_FEELINGS
	/* Display extra traditional feeling to warn of dangers. - C. Blue
	   Note: Only display ONE feeling, thereby setting priorities here.
	   Note: Don't display feelings in Training Tower (NO_DEATH). */
	if ((!p_ptr->distinct_floor_feeling && !is_admin(p_ptr)) || (d_ptr->flags2 & DF2_NO_DEATH) ||
	    (wpos->wx == WPOS_PVPARENA_X && wpos->wy == WPOS_PVPARENA_Y && wpos->wz == WPOS_PVPARENA_Z)) {
//		msg_print(Ind, "\376\377yLooks like any other level..");
//		msg_print(Ind, "\377ypfft");
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
		msg_print(Ind, "\374\377yYou sense an air of danger..");
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
	if(l_ptr->flags1 & LF1_IRON_RECALL || ((d_ptr->flags1 & DF1_FORCE_DOWN) && d_ptr->maxdepth == ABS(p_ptr->wpos.wz)))
		msg_print(Ind, "\377gYou don't sense a magic barrier here!");

	return(l_ptr->flags1 & LF1_FEELING_MASK ? TRUE : felt);
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
#if 0 /* TIMEREWRITE */
	s32b turns = t + (10 * DAY_START);

	switch (what)
	{
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

			snprintf(buf2, 20, "%s", get_day(day + 1));
			if (full) snprintf(buf, 40, "%s (%s day)", month_name[i], buf2);
			else snprintf(buf, 40, "%s", month_name[i]);
			break;
		}
		/* 'Normal' months + Enderi */
		default:
		{
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

cptr get_day(int day)
{
	static char buf[20];
	cptr p = "th";

	if ((day / 10) == 1) ;
	else if ((day % 10) == 1) p = "st";
	else if ((day % 10) == 2) p = "nd";
	else if ((day % 10) == 3) p = "rd";

	snprintf(buf, 20, "%d%s", day, p);
	return (buf);
}

int gold_colour(int amt)
{
	int i, unit = 1;

	for (i = amt; i > 99 ; i >>= 1, unit++) /* naught */;
	if (unit > SV_GOLD_MAX) unit = SV_GOLD_MAX;
	return (lookup_kind(TV_GOLD, unit));
}

/* Catching bad players who hack their client.. nasty! */
void lua_intrusion(int Ind, char *problem_diz)
{
	s_printf(format("LUA INTRUSION: %s : %s\n", Players[Ind]->name, problem_diz));

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

void bbs_add_line(cptr textline)
{
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

void bbs_del_line(int entry)
{
	int j;
	if (entry < 0) return;
	if (entry >= BBS_LINES) return;
        for (j = entry; j < BBS_LINES - 1; j++)
                strcpy(bbs_line[j], bbs_line[j + 1]);
	/* erase last line */
	strcpy(bbs_line[BBS_LINES - 1], "");
}

void bbs_erase(void)
{
	int i;
        for (i = 0; i < BBS_LINES; i++)
                strcpy(bbs_line[i], "");
}

void pbbs_add_line(u16b party, cptr textline)
{
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

void gbbs_add_line(byte guild, cptr textline)
{
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
void player_list_add(player_list_type **list, s32b player)
{
	player_list_type *pl_ptr;

	MAKE(pl_ptr, player_list_type);

	pl_ptr->id = player;
	pl_ptr->next = *list;
	*list = pl_ptr;
}

/*
 * Check if a list contains an id.
 */
bool player_list_find(player_list_type *list, s32b player)
{
	player_list_type *pl_ptr;

	pl_ptr = list;

	while (pl_ptr)
	{
		if (pl_ptr->id == player)
		{
			return TRUE;
		}
		pl_ptr = pl_ptr->next;
	}

	return FALSE;
}

/*
 * Delete an id from a list.
 * Takes a double pointer to the list
 */
bool player_list_del(player_list_type **list, s32b player)
{
	player_list_type *pl_ptr, *prev;

	if (*list == NULL) return FALSE;

	/* Check the first node */
	if ((*list)->id == player)
	{
		*list = (*list)->next;
		return TRUE;
	}

	pl_ptr = (*list)->next;
	prev = *list;

	/* Check the rest of the nodes */
	while (pl_ptr)
	{
		if (pl_ptr->id == player)
		{
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
void player_list_free(player_list_type *list)
{
	player_list_type *pl_ptr, *tmp;

	pl_ptr = list;

	while (pl_ptr)
	{
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
bool is_newer_than(version_type *version, int major, int minor, int patch, int extra, int branch, int build)
{
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

bool is_older_than(version_type *version, int major, int minor, int patch, int extra, int branch, int build)
{
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
	else if (version->branch == branch)
	{
		/* Now check the build */
		if (version->build > build)
			return FALSE;
		else if (version->build < build)
			return TRUE;
	}

	/* Default */
	return FALSE;
}

bool is_same_as(version_type *version, int major, int minor, int patch, int extra, int branch, int build)
{
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
void my_memfrob(void *s, int n)
{
	int i;
	char *str;

	str = (char*) s;

	for (i = 0; i < n; i++)
	{
		/* XOR every byte with 42 */
		str[i] ^= 42;
	}
}

/* compare player mode compatibility - C. Blue
   Note: returns NULL if compatible. */
cptr compat_pmode(int Ind1, int Ind2) {
	if (Players[Ind1]->mode & MODE_PVP) {
		if (!(Players[Ind2]->mode & MODE_PVP)) {
			return "non-pvp";
		}
	} else if (Players[Ind1]->mode & MODE_EVERLASTING) {
		if (!(Players[Ind2]->mode & MODE_EVERLASTING)) {
			return "non-everlasting";
		}
	} else if (Players[Ind2]->mode & MODE_PVP) {
		return "pvp";
	} else if (Players[Ind2]->mode & MODE_EVERLASTING) {
		return "everlasting";
	}
	return NULL; /* means "is compatible" */
}

/* compare object and player mode compatibility - C. Blue
   Note: returns NULL if compatible. */
cptr compat_pomode(int Ind, object_type *o_ptr) {
	if (!o_ptr->owner || is_admin(Players[Ind])) return NULL; /* always compatible */
	if (Players[Ind]->mode & MODE_PVP) {
		if (!(o_ptr->mode & MODE_PVP)) {
			if (o_ptr->mode & MODE_EVERLASTING) {
				if (!(cfg.charmode_trading_restrictions & 2)) {
					return "non-pvp";
				}
			} else if (!(cfg.charmode_trading_restrictions & 4)) {
				return "non-pvp";
			}
		}
	} else if (Players[Ind]->mode & MODE_EVERLASTING) {
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

/* cut out pseudo-id inscriptions from a string (a note ie inscription usually),
   save resulting string in s2,
   save highest found pseudo-id string in psid. - C. Blue */
void note_crop_pseudoid(char *s2, char *psid, cptr s) {
	char *p, s0[80];
	int id = 0; /* assume no pseudo-id inscription */

	if (s == NULL) return;
	strcpy(s2, s);

	while (TRUE) {
		strcpy(s0, s2);
		strcpy(s2, "");

		if ((p = strstr(s0, "terrible-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 9);
			if (id < 1) id = 1;
		} else if ((p = strstr(s0, "terrible"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 8);
			if (id < 1) id = 1;
		} else if ((p = strstr(s0, "cursed-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 7);
			if (id < 2) id = 2;
		} else if ((p = strstr(s0, "cursed"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 6);
			if (id < 2) id = 2;
		} else if ((p = strstr(s0, "worthless-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 10);
			if (id < 4) id = 4;
		} else if ((p = strstr(s0, "worthless"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 9);
			if (id < 4) id = 4;
		} else if ((p = strstr(s0, "broken-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 7);
			if (id < 5) id = 5;
		} else if ((p = strstr(s0, "broken"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 6);
			if (id < 5) id = 5;
		} else if ((p = strstr(s0, "average-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 8);
			if (id < 6) id = 6;
		} else if ((p = strstr(s0, "average"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 7);
			if (id < 6) id = 6;
		} else if ((p = strstr(s0, "good-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 5);
			if (id < 7) id = 7;
		} else if ((p = strstr(s0, "good"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 4);
			if (id < 7) id = 7;
		} else if ((p = strstr(s0, "excellent-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 10);
			if (id < 8) id = 8;
		} else if ((p = strstr(s0, "excellent"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 9);
			if (id < 8) id = 8;
		} else if ((p = strstr(s0, "special-"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 8);
			if (id < 9) id = 9;
		} else if ((p = strstr(s0, "special"))) {
			strncpy(s2, s0, p - s0);
			s2[p - s0] = '\0';
			strcat(s2, p + 7);
			if (id < 9) id = 9;
		} else {
			/* no further replacements to make */
			break;
		}
	}

	strcpy(s2, s0);

	switch (id) {
	case 1: strcpy(psid, "terrible"); break;
	case 2: strcpy(psid, "cursed"); break;
	case 3: strcpy(psid, "bad"); break;
	case 4: strcpy(psid, "worthless"); break;
	case 5: strcpy(psid, "broken"); break;
	case 6: strcpy(psid, "average"); break;
	case 7: strcpy(psid, "good"); break;
	case 8: strcpy(psid, "excellent"); break;
	case 9: strcpy(psid, "special"); break;
	default:
		strcpy(psid, "");
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
		*fsp++ = (flags & (1 << b)) ? '1' : '0';
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
