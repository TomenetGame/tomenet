/* File: files.c */

/* Purpose: code dealing with files (and death) */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"

#ifdef HANDLE_SIGNALS
#include <signal.h>
#endif

/*
 * You may or may not want to use the following "#undef".
 */
/* #undef _POSIX_SAVED_IDS */

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

#ifdef CHECK_TIME

/*
 * Operating hours for ANGBAND (defaults to non-work hours)
 */
static char days[7][29] =
{
	"SUN:XXXXXXXXXXXXXXXXXXXXXXXX",
	"MON:XXXXXXXX.........XXXXXXX",
	"TUE:XXXXXXXX.........XXXXXXX",
	"WED:XXXXXXXX.........XXXXXXX",
	"THU:XXXXXXXX.........XXXXXXX",
	"FRI:XXXXXXXX.........XXXXXXX",
	"SAT:XXXXXXXXXXXXXXXXXXXXXXXX"
};

/*
 * Restict usage (defaults to no restrictions)
 */
static bool check_time_flag = FALSE;

#endif


/*
 * Handle CHECK_TIME
 */
errr check_time(void)
{

#ifdef CHECK_TIME

	time_t              c;
	struct tm               *tp;

	/* No restrictions */
	if (!check_time_flag) return (0);

	/* Check for time violation */
	c = time((time_t *)0);
	tp = localtime(&c);

	/* Violation */
	if (days[tp->tm_wday][tp->tm_hour + 4] != 'X') return (1);

#endif

	/* Success */
	return (0);
}



/*
 * Initialize CHECK_TIME
 */
errr check_time_init(void)
{

#ifdef CHECK_TIME

	FILE        *fp;

	char    buf[1024];


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "time.txt");

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* No file, no restrictions */
	if (!fp) return (0);

	/* Assume restrictions */
	check_time_flag = TRUE;

	/* Parse the file */
	while (0 == my_fgets(fp, buf, 80))
	{
		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Chop the buffer */
		buf[29] = '\0';

		/* Extract the info */
		if (prefix(buf, "SUN:")) strcpy(days[0], buf);
		if (prefix(buf, "MON:")) strcpy(days[1], buf);
		if (prefix(buf, "TUE:")) strcpy(days[2], buf);
		if (prefix(buf, "WED:")) strcpy(days[3], buf);
		if (prefix(buf, "THU:")) strcpy(days[4], buf);
		if (prefix(buf, "FRI:")) strcpy(days[5], buf);
		if (prefix(buf, "SAT:")) strcpy(days[6], buf);
	}

	/* Close it */
	my_fclose(fp);

#endif

	/* Success */
	return (0);
}



#ifdef CHECK_LOAD

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN  64
#endif

typedef struct statstime statstime;

struct statstime
{
	int                 cp_time[4];
	int                 dk_xfer[4];
	unsigned int        v_pgpgin;
	unsigned int        v_pgpgout;
	unsigned int        v_pswpin;
	unsigned int        v_pswpout;
	unsigned int        v_intr;
	int                 if_ipackets;
	int                 if_ierrors;
	int                 if_opackets;
	int                 if_oerrors;
	int                 if_collisions;
	unsigned int        v_swtch;
	long                avenrun[3];
	struct timeval      boottime;
	struct timeval      curtime;
};

/*
 * Maximal load (if any).
 */
static int check_load_value = 0;

#endif


/*
 * Handle CHECK_LOAD
 */
errr check_load(void)
{

#ifdef CHECK_LOAD

	struct statstime    st;

	/* Success if not checking */
	if (!check_load_value) return (0);

	/* Check the load */
	if (0 == rstat("localhost", &st))
	{
		long val1 = (long)(st.avenrun[2]);
		long val2 = (long)(check_load_value) * FSCALE;

		/* Check for violation */
		if (val1 >= val2) return (1);
	}

#endif

	/* Success */
	return (0);
}


/*
 * Initialize CHECK_LOAD
 */
errr check_load_init(void)
{

#ifdef CHECK_LOAD

	FILE        *fp;

	char    buf[1024];

	char    temphost[MAXHOSTNAMELEN+1];
	char    thishost[MAXHOSTNAMELEN+1];


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_TEXT, "load.txt");

	/* Open the "load" file */
	fp = my_fopen(buf, "r");

	/* No file, no restrictions */
	if (!fp) return (0);

	/* Default load */
	check_load_value = 100;

	/* Get the host name */
	(void)gethostname(thishost, (sizeof thishost) - 1);

	/* Parse it */
	while (0 == my_fgets(fp, buf, 1024))
	{
		int value;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Parse, or ignore */
		if (sscanf(buf, "%s%d", temphost, &value) != 2) continue;

		/* Skip other hosts */
		if (!streq(temphost, thishost) &&
		    !streq(temphost, "localhost")) continue;

		/* Use that value */
		check_load_value = value;

		/* Done */
		break;
	}

	/* Close the file */
	my_fclose(fp);

#endif

	/* Success */
	return (0);
}





/*
 * Prints the following information on the screen.
 *
 * For this to look right, the following should be spaced the
 * same as in the prt_lnum code... -CFT
 *
 * This will send the info to the client now --KLJ--
 * 
 * Except that this (and display_player) are never called. --KLJ--
 */
static void display_player_middle(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int show_tohit = p_ptr->dis_to_h;
	int show_todam = p_ptr->dis_to_d;

	s32b adv_exp;

	object_type *o_ptr = &p_ptr->inventory[INVEN_WIELD];

	/* Hack -- add in weapon info if known */
	if (object_known_p(Ind, o_ptr)) show_tohit += o_ptr->to_h;
	if (object_known_p(Ind, o_ptr)) show_todam += o_ptr->to_d;

	/* Dump the bonuses to hit/dam */
	Send_plusses(Ind, show_tohit, show_todam);

	/* Dump the armor class bonus */
	Send_ac(Ind, p_ptr->dis_ac, p_ptr->dis_to_a);

	if (p_ptr->lev >= PY_MAX_LEVEL)
		adv_exp = 0;
	else
	{
		s64b adv = ((s64b)player_exp[p_ptr->lev - 1] * (s64b)p_ptr->expfact / 100L);
		adv_exp = (s32b)(adv);
	}

	Send_experience(Ind, p_ptr->lev, p_ptr->max_exp, p_ptr->exp, adv_exp);

	Send_gold(Ind, p_ptr->au);

	Send_hp(Ind, p_ptr->mhp, p_ptr->chp);

	Send_sp(Ind, p_ptr->msp, p_ptr->csp);
}



/*
 * Display the character on the screen (with optional history)
 *
 * The top two and bottom two lines are left blank.
 */
void display_player(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int i;


	/* Send basic information */
	Send_char_info(Ind, p_ptr->prace, p_ptr->pclass, p_ptr->male);

	/* Age, Height, Weight, Social */
	Send_various(Ind, p_ptr->ht, p_ptr->wt, p_ptr->age, p_ptr->sc);

	/* Send all the stats */
	for (i = 0; i < 6; i++)
	{
		Send_stat(Ind, i, p_ptr->stat_top[i], p_ptr->stat_use[i]);
	}

	/* Extra info */
	display_player_middle(Ind);

	/* Display "history" info */
	Send_history(Ind, i, p_ptr->history[i]);
}

/*
 * Recursive "help file" perusal.  Return FALSE on "ESCAPE".
 *
 * XXX XXX XXX Consider using a temporary file.
 *
 */
static bool do_cmd_help_aux(int Ind, cptr name, cptr what, int line, int color)
{
	int             i, k;

	/* Number of "real" lines passed by */
	int             next = 0;

	/* Number of "real" lines in the file */
	int             size = 0;

	/* Backup value for "line" */
	int             back = 0;

	/* This screen has sub-screens */
	bool    menu = FALSE;

	/* Current help file */
	FILE    *fff = NULL;

	/* Find this string (if any) */
	cptr    find = NULL;

	/* Hold a string to find */
	char    finder[128];

	/* Hold a string to show */
	char    shower[128];

	/* Describe this thing */
	char    caption[128];

	/* Path buffer */
	char    path[1024];

	/* General buffer */
	char    buf[1024];

	/* Sub-menu information */
	char    hook[10][32];


	/* Wipe finder */
	strcpy(finder, "");

	/* Wipe shower */
	strcpy(shower, "");

	/* Wipe caption */
	strcpy(caption, "");

	/* Wipe the hooks */
	for (i = 0; i < 10; i++) hook[i][0] = '\0';


	/* Hack XXX XXX XXX */
	if (what)
	{
		/* Caption */
		strcpy(caption, what);

		/* Access the "file" */
		strcpy(path, name);

		/* Open */
		fff = my_fopen(path, "r");
	}

	/* Look in "help" */
	if (!fff)
	{
		/* Caption */
		sprintf(caption, "Help file '%s'", name);

		/* Build the filename */
		path_build(path, 1024, ANGBAND_DIR_TEXT, name);

		/* Open the file */
		fff = my_fopen(path, "r");
	}

	/* Oops */
	if (!fff)
	{
		/* Message */
		msg_format(Ind, "Cannot open '%s'.", name);
		msg_print(Ind, NULL);

		/* Oops */
		return (TRUE);
	}


	/* Pre-Parse the file */
	while (TRUE)
	{
		/* Read a line or stop */
		if (my_fgets(fff, buf, 1024)) break;

		/* XXX Parse "menu" items */
		if (prefix(buf, "***** "))
		{
			char b1 = '[', b2 = ']';

			/* Notice "menu" requests */
			if ((buf[6] == b1) && isdigit(buf[7]) &&
			    (buf[8] == b2) && (buf[9] == ' '))
			{
				/* This is a menu file */
				menu = TRUE;

				/* Extract the menu item */
				k = buf[7] - '0';

				/* Extract the menu item */
				strcpy(hook[k], buf + 10);
			}

			/* Skip this */
			continue;
		}

		/* Count the "real" lines */
		next++;
	}

	/* Save the number of "real" lines */
	size = next;



	/* Display the file */
		/* Restart when necessary */
		if (line >= size) line = 0;


		/* Re-open the file if needed */
		if (next > line)
		{
			/* Close it */
			my_fclose(fff);

			/* Hack -- Re-Open the file */
			fff = my_fopen(path, "r");

			/* Oops */
			if (!fff) return (FALSE);

			/* File has been restarted */
			next = 0;
		}

		/* Skip lines if needed */
		for (; next < line; next++)
		{
			/* Skip a line */
			if (my_fgets(fff, buf, 1024)) break;
		}


		/* Dump the next 20 lines of the file */
		for (i = 0; i < 20; )
		{
			byte attr = TERM_WHITE;

			/* Hack -- track the "first" line */
			if (!i) line = next;

			/* Get a line of the file or stop */
			if (my_fgets(fff, buf, 1024)) break;

			/* Hack -- skip "special" lines */
			if (prefix(buf, "***** ")) continue;

			/* Count the "real" lines */
			next++;

			/* Hack -- keep searching */
			if (find && !i && !strstr(buf, find)) continue;

			/* Hack -- stop searching */
			find = NULL;

			/* Extract color */
			if (color) attr = color_char_to_attr(buf[0]);

			/* Hack -- show matches */
			if (shower[0] && strstr(buf, shower)) attr = TERM_YELLOW;

			/* Dump the line */
			Send_special_line(Ind, size, i, attr, &buf[color]);

			/* Count the printed lines */
			i++;
		}

		/* Hack -- failed search */
		if (find)
		{
			bell();
			line = back;
			find = NULL;
			return (TRUE);
		}

	/* Close the file */
	my_fclose(fff);

	/* Escape */
	if (k == ESCAPE) return (FALSE);

	/* Normal return */
	return (TRUE);
}


/*
 * Peruse the On-Line-Help, starting at the given file.
 *
 * Disabled --KLJ--
 */
void do_cmd_help(int Ind, int line)
{
	cptr name = "mangband.hlp";

	/* Peruse the main help file */
	(void)do_cmd_help_aux(Ind, name, NULL, line, FALSE);
}



/*
 * Hack -- display the contents of a file on the screen
 *
 * XXX XXX XXX Use this function for commands such as the
 * "examine object" command.
 */
errr show_file(int Ind, cptr name, cptr what, int line, int color)
{
	/* Peruse the requested file */
	(void)do_cmd_help_aux(Ind, name, what, line, color);

	/* Success */
	return (0);
}




/*
 * Process the player name.
 * Extract a clean "base name".
 * Build the savefile name if needed.
 */
bool process_player_name(int Ind, bool sf)
{
	player_type *p_ptr = Players[Ind];

	int i, k = 0;


	/* Cannot be too long */
	if (strlen(p_ptr->name) > 15)
	{
		/* Name too long */
		Destroy_connection(p_ptr->conn, "Your name is too long!");

		/* Abort */
		return FALSE;
	}

	/* Cannot contain "icky" characters */
	for (i = 0; p_ptr->name[i]; i++)
	{
		/* No control characters */
		if (iscntrl(p_ptr->name[i]))
		{
			/* Illegal characters */
			Destroy_connection(p_ptr->conn, "Your name contains control chars!");

			/* Abort */
			return FALSE;
		}
	}


#ifdef MACINTOSH

	/* Extract "useful" letters */
	for (i = 0; p_ptr->name[i]; i++)
	{
		char c = p_ptr->name[i];

		/* Convert "dot" to "underscore" */
		if (c == '.') c = '_';

		/* Accept all the letters */
		p_ptr->basename[k++] = c;
	}

#else

	/* Extract "useful" letters */
	for (i = 0; p_ptr->name[i]; i++)
	{
		char c = p_ptr->name[i];

		/* Accept some letters */
		if (isalpha(c) || isdigit(c)) p_ptr->basename[k++] = c;

		/* Convert space, dot, and underscore to underscore */
		else if (strchr(". _", c)) p_ptr->basename[k++] = '_';
	}

#endif


#if defined(WINDOWS) || defined(MSDOS)

	/* Hack -- max length */
	if (k > 8) k = 8;

#endif

	/* Terminate */
	p_ptr->basename[k] = '\0';

	/* Require a "base" name */
	if (!p_ptr->basename[0]) strcpy(p_ptr->basename, "PLAYER");


#ifdef SAVEFILE_MUTABLE

	/* Accept */
	sf = TRUE;

#endif

	/* Change the savefile name */
	if (sf)
	{
		char temp[128];

		/* Rename the savefile, using the player_base */
		(void)sprintf(temp, "%s", p_ptr->basename);

#ifdef VM
		/* Hack -- support "flat directory" usage on VM/ESA */
		(void)sprintf(temp, "%s.sv", player_base);
#endif /* VM */

		/* Build the filename */
		path_build(p_ptr->savefile, 1024, ANGBAND_DIR_SAVE, temp);
	}

	/* Success */
	return TRUE;
}


/*
 * Gets a name for the character, reacting to name changes.
 *
 * Assumes that "display_player()" has just been called
 * XXX Perhaps we should NOT ask for a name (at "birth()") on Unix?
 *
 * The name should be sent to us from the client, so this is unnecessary --KLJ--
 */
void get_name(int Ind)
{
}



/*
 * Hack -- commit suicide
 */
void do_cmd_suicide(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Mark as suicide */
	p_ptr->alive = FALSE;

	/* Hack -- set the cause of death */
	if (!p_ptr->ghost) 
	{
		strcpy(p_ptr->died_from_list, "self-inflicted wounds");
		p_ptr->died_from_depth = getlevel(&p_ptr->wpos);
	}

	/* Hack -- clear ghost */
	p_ptr->ghost = FALSE;

	if (p_ptr->total_winner) kingly(Ind);

	/* Kill him */
	player_death(Ind);
}



/*
 * Save a character
 */
void do_cmd_save_game(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Disturb the player */
	disturb(Ind, 1, 0);

	/* Clear messages */
	msg_print(Ind, NULL);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Message */
	msg_print(Ind, "Saving game...");

	/* The player is not dead */
	(void)strcpy(p_ptr->died_from, "(saved)");

	/* Forbid suspend */
	signals_ignore_tstp();

	/* Save the player */
	if (save_player(Ind))
	{
		msg_print(Ind, "Saving game... done.");
	}

	/* Save failed (oops) */
	else
	{
		msg_print(Ind, "Saving game... failed!");
	}

	/* Allow suspend again */
	signals_handle_tstp();

	/* Note that the player is not dead */
	(void)strcpy(p_ptr->died_from, "(alive and well)");
}



/*
 * Hack -- Calculates the total number of points earned         -JWT-
 */
long total_points(int Ind)
{
	player_type *p_ptr = Players[Ind];
	/* Maggot bonus.. beware, r_idx is hard-coded! */
	int i = p_ptr->r_killed[8]? 1 : 2;

	if (p_ptr->mode == MODE_HELL) return (((p_ptr->max_exp + (100 * p_ptr->max_dlv)) * 3 / 2)*i);
	else return ((p_ptr->max_exp + (100 * p_ptr->max_dlv) + p_ptr->au)*i);
}

/*
 * Semi-Portable High Score List Entry (128 bytes) -- BEN
 *
 * All fields listed below are null terminated ascii strings.
 *
 * In addition, the "number" fields are right justified, and
 * space padded, to the full available length (minus the "null").
 *
 * Note that "string comparisons" are thus valid on "pts".
 */

typedef struct high_score high_score;

struct high_score
{
	char what[8];           /* Version info (string) */

	char pts[10];           /* Total Score (number) */

	char gold[10];          /* Total Gold (number) */

	char turns[10];         /* Turns Taken (number) */

	char day[10];           /* Time stamp (string) */

	char who[16];           /* Player Name (string) */

	char sex[2];            /* Player Sex (string) */
	char p_r[3];            /* Player Race (number) */
	char p_c[3];            /* Player Class (number) */

	char cur_lev[4];                /* Current Player Level (number) */
	char cur_dun[4];                /* Current Dungeon Level (number) */
	char max_lev[4];                /* Max Player Level (number) */
	char max_dun[4];                /* Max Dungeon Level (number) */

	char how[32];           /* Method of death (string) */
};



/*
 * The "highscore" file descriptor, if available.
 */
static int highscore_fd = -1;


/*
 * Seek score 'i' in the highscore file
 */
static int highscore_seek(int i)
{
	/* Seek for the requested record */
	return (fd_seek(highscore_fd, (huge)(i) * sizeof(high_score)));
}


/*
 * Read one score from the highscore file
 */
static errr highscore_read(high_score *score)
{
	/* Read the record, note failure */
	return (fd_read(highscore_fd, (char*)(score), sizeof(high_score)));
}


/*
 * Write one score to the highscore file
 */
static int highscore_write(high_score *score)
{
	/* Write the record, note failure */
	return (fd_write(highscore_fd, (char*)(score), sizeof(high_score)));
}




/*
 * Just determine where a new score *would* be placed
 * Return the location (0 is best) or -1 on failure
 */
static int highscore_where(high_score *score)
{
	int                     i;

	high_score              the_score;

	/* Paranoia -- it may not have opened */
	if (highscore_fd < 0) return (-1);

	/* Go to the start of the highscore file */
	if (highscore_seek(0)) return (-1);

	/* Read until we get to a higher score */
	for (i = 0; i < MAX_HISCORES; i++)
	{
		if (highscore_read(&the_score)) return (i);
		if (strcmp(the_score.pts, score->pts) < 0) return (i);
	}

	/* The "last" entry is always usable */
	return (MAX_HISCORES - 1);
}


/*
 * Actually place an entry into the high score file
 * Return the location (0 is best) or -1 on "failure"
 */
static int highscore_add(high_score *score)
{
	int                     i, slot;
	bool            done = FALSE;

	high_score              the_score, tmpscore;


	/* Paranoia -- it may not have opened */
	if (highscore_fd < 0) return (-1);

	/* Determine where the score should go */
	slot = highscore_where(score);

	/* Hack -- Not on the list */
	if (slot < 0) return (-1);

	/* Hack -- prepare to dump the new score */
	the_score = (*score);

	/* Slide all the scores down one */
	for (i = slot; !done && (i < MAX_HISCORES); i++)
	{
		/* Read the old guy, note errors */
		if (highscore_seek(i)) return (-1);
		if (highscore_read(&tmpscore)) done = TRUE;

		/* Back up and dump the score we were holding */
		if (highscore_seek(i)) return (-1);
		if (highscore_write(&the_score)) return (-1);

		/* Hack -- Save the old score, for the next pass */
		the_score = tmpscore;
	}

	/* Return location used */
	return (slot);
}



/*
 * Display the scores in a given range.
 * Assumes the high score list is already open.
 * Only five entries per line, too much info.
 *
 * Mega-Hack -- allow "fake" entry at the given position.
 */
static void display_scores_aux(int Ind, int line, int note, high_score *score)
{
	int             i, j, from, to, attr, place;

	high_score      the_score;

	char    out_val[256];

	FILE *fff;
	char file_name[1024];

	/* Paranoia -- it may not have opened */
	if (highscore_fd < 0) return;
	
	/* Temporary file */
	if (path_temp(file_name, 1024)) return;

	/* Open the temp file */
	fff = my_fopen(file_name, "w");

	/* Assume we will show the first 20 */
	from = 0;
	to = 20;
	if (to > MAX_HISCORES) to = MAX_HISCORES;


	/* Seek to the beginning */
	if (highscore_seek(0)) return;

	/* Hack -- Count the high scores */
	for (i = 0; i < MAX_HISCORES; i++)
	{
		if (highscore_read(&the_score)) break;
	}

	/* Hack -- allow "fake" entry to be last */
	if ((note == i) && score) i++;

	/* Forget about the last entries */
	if (i > to) i = to;


	/* Show 5 per page, until "done" */
	for (j = from, place = j+1; j < i; j++, place++)
	{
		int pr, pc, clev, mlev, cdun, mdun;

		cptr gold, when, aged;


		/* Hack -- indicate death in yellow */
		attr = (j == note) ? TERM_YELLOW : TERM_WHITE;


		/* Mega-Hack -- insert a "fake" record */
		if ((note == j) && score)
		{
			the_score = (*score);
			attr = TERM_L_GREEN;
			score = NULL;
			note = -1;
			j--;
			i--;
		}

		/* Read a normal record */
		else
		{
			/* Read the proper record */
			if (highscore_seek(j)) break;
			if (highscore_read(&the_score)) break;
		}

		/* Extract the race/class */
		pr = atoi(the_score.p_r);
		pc = atoi(the_score.p_c);

		/* Extract the level info */
		clev = atoi(the_score.cur_lev);
		mlev = atoi(the_score.max_lev);
		cdun = atoi(the_score.cur_dun);
		mdun = atoi(the_score.max_dun);

		/* Hack -- extract the gold and such */
		for (when = the_score.day; isspace(*when); when++) /* loop */;
		for (gold = the_score.gold; isspace(*gold); gold++) /* loop */;
		for (aged = the_score.turns; isspace(*aged); aged++) /* loop */;

		/* Dump some info */
		sprintf(out_val, "%3d.%9s  %s the %s %s, Level %d",
			place, the_score.pts, the_score.who,
			race_info[pr].title, class_info[pc].title,
			clev);

		/* Append a "maximum level" */
		if (mlev > clev) strcat(out_val, format(" (Max %d)", mlev));

		/* Dump the first line */
		fprintf(fff, "%s\n", out_val);

		/* Another line of info */
		if (strcmp(the_score.how, "winner"))
			sprintf(out_val, "               Killed by %s on %s %d",
			the_score.how, "Dungeon Level", cdun);
		else
			sprintf(out_val, "               Retired after a legendary career");

		/* Hack -- some people die in the town */
		if ((!cdun) && (strcmp(the_score.how, "winner")))
		{
			sprintf(out_val, "               Killed by %s in the Town",
				the_score.how);
		}

		/* Append a "maximum level" */
		if (mdun > cdun) strcat(out_val, format(" (Max %d)", mdun));

		/* Dump the info */
		fprintf(fff, "%s\n", out_val);

		/* And still another line of info */
		sprintf(out_val,
			"               (Date %s, Gold %s, Turn %s).",
			when, gold, aged);
		fprintf(fff, "%s\n", out_val);

		/* Print newline if this isn't the last one */
		if (j < i - 1)
			fprintf(fff, "\n");
	}

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(Ind, file_name, "High Scores", line, 0);

	/* Remove the file */
	fd_kill(file_name);
}




/*
 * Enters a players name on a hi-score table, if "legal", and in any
 * case, displays some relevant portion of the high score list.
 *
 * Assumes "signals_ignore_tstp()" has been called.
 */
static errr top_twenty(int Ind)
{
	player_type *p_ptr = Players[Ind];
	int          j;

	high_score   the_score;

	time_t ct = time((time_t*)0);

	/* No score file */
	if (highscore_fd < 0)
	{
		s_printf("Score file unavailable.\n");
		return (0);
	}

	/* Interupted */
	if (!p_ptr->total_winner && streq(p_ptr->died_from, "Interrupting"))
	{
		msg_print(Ind, "Score not registered due to interruption.");
		/* display_scores_aux(0, 10, -1, NULL); */
		return (0);
	}

	/* Quitter */
	if (!p_ptr->total_winner && streq(p_ptr->died_from, "Quitting"))
	{
		msg_print(Ind, "Score not registered due to quitting.");
		/* display_scores_aux(0, 10, -1, NULL); */
		return (0);
	}


	/* Clear the record */
	WIPE(&the_score, high_score);

	/* Save the version */
	sprintf(the_score.what, "%u.%u.%u",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

	/* Calculate and save the points */
	sprintf(the_score.pts, "%9lu", (long)total_points(Ind));
	the_score.pts[9] = '\0';

	/* Save the current gold */
	sprintf(the_score.gold, "%9lu", (long)p_ptr->au);
	the_score.gold[9] = '\0';

	/* Save the current turn */
	sprintf(the_score.turns, "%9lu", (long)turn);
	the_score.turns[9] = '\0';

#ifdef HIGHSCORE_DATE_HACK
	/* Save the date in a hacked up form (9 chars) */
	sprintf(the_score.day, "%-.6s %-.2s", ctime(&ct) + 4, ctime(&ct) + 22);
#else
	/* Save the date in standard form (8 chars) */
	strftime(the_score.day, 9, "%m/%d/%y", localtime(&ct));
#endif

	/* Save the player name (15 chars) */
	sprintf(the_score.who, "%-.15s", p_ptr->name);

	/* Save the player info */
	sprintf(the_score.sex, "%c", (p_ptr->male ? 'm' : 'f'));
	sprintf(the_score.p_r, "%2d", p_ptr->prace);
	sprintf(the_score.p_c, "%2d", p_ptr->pclass);

	/* Save the level and such */
	sprintf(the_score.cur_lev, "%3d", p_ptr->lev);
	sprintf(the_score.cur_dun, "%3d", p_ptr->died_from_depth);
	sprintf(the_score.max_lev, "%3d", p_ptr->max_plv);
	sprintf(the_score.max_dun, "%3d", p_ptr->max_dlv);

	/* Save the cause of death (31 chars) */
	/* HACKED to take the saved cause of death of the character, not the ghost */
	sprintf(the_score.how, "%-.31s", p_ptr->died_from_list);


	/* Lock (for writing) the highscore file, or fail */
	if (fd_lock(highscore_fd, F_WRLCK)) return (1);

	/* Add a new entry to the score list, see where it went */
	j = highscore_add(&the_score);

	/* Unlock the highscore file, or fail */
	if (fd_lock(highscore_fd, F_UNLCK)) return (1);


#if 0
	/* Hack -- Display the top fifteen scores */
	if (j < 10)
	{
		display_scores_aux(0, 15, j, NULL);
	}

	/* Display the scores surrounding the player */
	else
	{
		display_scores_aux(0, 5, j, NULL);
		display_scores_aux(j - 2, j + 7, j, NULL);
	}
#endif


	/* Success */
	return (0);
}


/*
 * Predict the players location, and display it.
 */
static errr predict_score(int Ind, int line)
{
	player_type *p_ptr = Players[Ind];
	int          j;

	high_score   the_score;


	/* No score file */
	if (highscore_fd < 0)
	{
		s_printf("Score file unavailable.\n");
		return (0);
	}


	/* Save the version */
	sprintf(the_score.what, "%u.%u.%u",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

	/* Calculate and save the points */
	sprintf(the_score.pts, "%9lu", (long)total_points(Ind));

	/* Save the current gold */
	sprintf(the_score.gold, "%9lu", (long)p_ptr->au);

	/* Save the current turn */
	sprintf(the_score.turns, "%9lu", (long)turn);

	/* Hack -- no time needed */
	strcpy(the_score.day, "TODAY");

	/* Save the player name (15 chars) */
	sprintf(the_score.who, "%-.15s", p_ptr->name);

	/* Save the player info */
	sprintf(the_score.sex, "%c", (p_ptr->male ? 'm' : 'f'));
	sprintf(the_score.p_r, "%2d", p_ptr->prace);
	sprintf(the_score.p_c, "%2d", p_ptr->pclass);

	/* Save the level and such */
	sprintf(the_score.cur_lev, "%3d", p_ptr->lev);
	sprintf(the_score.cur_dun, "%3d", getlevel(&p_ptr->wpos));
	sprintf(the_score.max_lev, "%3d", p_ptr->max_plv);
	sprintf(the_score.max_dun, "%3d", p_ptr->max_dlv);

	/* Hack -- no cause of death */
	strcpy(the_score.how, "nobody (yet!)");


	/* See where the entry would be placed */
	j = highscore_where(&the_score);


	/* Hack -- Display the top fifteen scores */
	if (j < 19)  /* 10 */
	{
		display_scores_aux(Ind, line, j, &the_score);
	}

	/* Display some "useful" scores */
	else
	{
		display_scores_aux(Ind, line, 19, &the_score);  /* -1, NULL */
	}


	/* Success */
	return (0);
}




/*
 * Change a player into a King!                 -RAK-
 */
void kingly(int Ind)
{
	player_type *p_ptr = Players[Ind];

#if 0	// No, this makes Delete_player fail!
	/* Hack -- retire in town */
	p_ptr->wpos.wx=0;	// pfft, not 0 maybe
	p_ptr->wpos.wy=0;
	p_ptr->wpos.wz=0;
#endif	// 0

	/* Fake death */
	//(void)strcpy(p_ptr->died_from_list, "Ripe Old Age");
	(void)strcpy(p_ptr->died_from_list, "winner");

	/* Restore the experience */
	p_ptr->exp = p_ptr->max_exp;

	/* Restore the level */
	p_ptr->lev = p_ptr->max_plv;

	/* Hack -- Player gets an XP bonus for beating the game */
	p_ptr->exp = p_ptr->max_exp += 10000000L;
}


/*
 * Add a player to the high score list.
 */
void add_high_score(int Ind)
{
	char buf[1024];

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "scores.raw");

	/* Open the high score file, for reading/writing */
	highscore_fd = fd_open(buf, O_RDWR);

	/* Add them */
	top_twenty(Ind);

	/* Shut the high score file */
	(void)fd_close(highscore_fd);

	/* Forget the high score fd */
	highscore_fd = -1;
}


/*
 * Close up the current game (player may or may not be dead)
 *
 * This function is called only from "main.c" and "signals.c".
 *
 * In here we try to save everybody's game, as well as save the server state.
 */
void close_game(void)
{
	int i;

	/* No suspending now */
	signals_ignore_tstp();

	for (i = 0; i < NumPlayers; i++)
	{
		player_type *p_ptr = Players[i];

		/* Make sure the player is connected */
		if (p_ptr->conn == NOT_CONNECTED)
			continue;

		/* Handle stuff */
		handle_stuff(i);

		/* Flush the messages */
		msg_print(i, NULL);

		/* Build the filename */
		/*path_build(buf, 1024, ANGBAND_DIR_APEX, "scores.raw");*/

		/* Open the high score file, for reading/writing */
		/*highscore_fd = fd_open(buf, O_RDWR);*/


		/* Handle death */
		if (p_ptr->death)
		{
			/* Handle retirement */
			if (p_ptr->total_winner) kingly(i);

			/* Save memories */
			if (!save_player(i)) msg_print(i, "death save failed!");

			/* Handle score, show Top scores */
			top_twenty(i);
		}

		/* Still alive */
		else
		{
			/* Save the game */
			do_cmd_save_game(i);

			/* Prompt for scores XXX XXX XXX */
			/*prt("Press Return (or Escape).", 0, 40);*/
		}


		/* Shut the high score file */
		/*(void)fd_close(highscore_fd);*/

		/* Forget the high score fd */
		/*highscore_fd = -1;*/
	}

	/* Stop the timer */
	teardown_timer();

	/* Try to save the server information */
	save_server_info();

	/* Allow suspending now */
	signals_handle_tstp();
}


/*
 * Hack -- Display the scores in a given range and quit.
 *
 * This function is only called from "main.c" when the user asks
 * to see the "high scores".
 */
void display_scores(int Ind, int line)
{
	char buf[1024];

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "scores.raw");

	/* Open the binary high score file, for reading */
	highscore_fd = fd_open(buf, O_RDONLY);

	/* Paranoia -- No score file */
	if (highscore_fd < 0)
	{
		/* Message to server admin */
		s_printf("Score file unavailable.\n");

		/* Quit */
		return;
	}

	/* Display the scores */
	predict_score(Ind, line);

	/* Shut the high score file */
	(void)fd_close(highscore_fd);

	/* Forget the high score fd */
	highscore_fd = -1;

	/* Quit */
	/* quit(NULL); */
}


/*
 * Get a random line from a file
 * Based on the monster speech patch by Matt Graham,
 */
errr get_rnd_line(cptr file_name, int entry, char *output)
{
	FILE    *fp;
	char    buf[1024];
	int     line, counter, test, numentries;
	int     line_num = 0;
	bool    found = FALSE;


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_TEXT, file_name);

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Failed */
	if (!fp) return (-1);

	/* Find the entry of the monster */
	while (TRUE)
	{
		/* Get a line from the file */
		if (my_fgets(fp, buf, 1024) == 0)
		{
			/* Count the lines */
			line_num++;

			/* Look for lines starting with 'N:' */
			if ((buf[0] == 'N') && (buf[1] == ':'))
			{
				/* Allow default lines */
				if (buf[2] == '*')
				{
					/* Default lines */
					found = TRUE;
					break;
				}
				/* Get the monster number */
				else if (sscanf(&(buf[2]), "%d", &test) != EOF)
				{
					/* Is it the right monster? */
					if (test == entry)
					{
						found = TRUE;
						break;
					}
				}
				else
				{
					my_fclose(fp);
					return (-1);
				}
			}
		}
		else
		{
			/* Reached end of file */
			my_fclose(fp);
			return (-1);
		}

	}

	/* Get the number of entries */
	while (TRUE)
	{
		/* Get the line */
		if (my_fgets(fp, buf, 1024) == 0)
		{
			/* Count the lines */
			line_num++;

			/* Look for the number of entries */
			if (isdigit(buf[0]))
			{
				/* Get the number of entries */
				numentries = atoi(buf);
				break;
			}
		}
		else
		{
			/* Count the lines */
			line_num++;

			my_fclose(fp);
			return (-1);
		}
	}

	if (numentries > 0)
	{
		/* Grab an appropriate line number */
		line = rand_int(numentries);

		/* Get the random line */
		for (counter = 0; counter <= line; counter++)
		{
			/* Count the lines */
			line_num++;

			/* Try to read the line */
			if (my_fgets(fp, buf, 1024) == 0)
			{
				/* Found the line */
				if (counter == line) break;
			}
			else
			{
				my_fclose(fp);
				return (-1);
			}
		}

		/* Copy the line */
		strcpy(output, buf);
	}

	/* Close the file */
	my_fclose(fp);

	/* Success */
	return (0);
}


/*
 * Handle a fatal crash.
 *
 * Here we try to save every player's state, and the state of the server
 * in general.  Note that we must be extremely careful not to have a crash
 * in this function, or some things may not get saved.  Also, this function
 * may get called because some data structures are not in a "correct" state.
 * For this reason many paranoia checks are done to prevent bad pointer
 * dereferences.
 *
 * Note that this function would not be needed at all if there were no bugs.
 */
void exit_game_panic(void)
{
	int i = 1;
	int j,k;
	struct worldpos wpos;
	struct wilderness_type *wild;

	/* If nothing important has happened, just quit */
	if (!server_generated || server_saved) quit("panic");

	while (NumPlayers > (i - 1))
	{
		player_type *p_ptr = Players[i];

		/* Don't dereference bad pointers */
		if (!p_ptr)
		{
			/* Skip to next player */
			i++;

			continue;
		}

		/* Hack -- turn off some things */
		disturb(i, 1, 0);

		/* Mega-Hack -- Delay death */
		if (p_ptr->chp < 0) p_ptr->death = FALSE;

		/* Hardcode panic save */
		panic_save = 1;

		/* Forbid suspend */
		signals_ignore_tstp();

		/* Indicate panic save */
		(void)strcpy(p_ptr->died_from, "(panic save)");

		/* Panic save, or get worried */
		if (!save_player(i))
		{
			if (!Destroy_connection(p_ptr->conn, "panic save failed!"))
			{
				/* Something extremely bad is going on, try to recover anyway */
				i++;
			}
		}

		/* Successful panic save */
		if (!Destroy_connection(p_ptr->conn, "panic save succeeded!"))
		{
			/* Something very bad happened, skip to next player */
			i++;
		}
	}

	/* Clear objects so that artifacts get saved.
	 * This probably isn't neccecary, as all the objects on each of
	 * these levels should have been cleared by now. However, paranoia
	 * can't hurt all that much... -APD
	 */
/* totally inefficient replacement - sorry. */
/* it also doesnt respect non existent world positions */
/* rewrite */
/* k rewritten.. but not so better? */
	for(i=0;i<MAX_WILD_X;i++){
		wpos.wx=i;
		for(j=0;j<MAX_WILD_Y;j++){
			wpos.wy=j;
			wild=&wild_info[j][i];

			if (wild->flags&WILD_F_UP)
				for (k = 0;k < wild->tower->maxdepth; k++)
				{
					wpos.wz=k;
					if((getcave(&wpos)) && (!players_on_depth(&wpos))) wipe_o_list(&wpos);
				}

			if (wild->flags&WILD_F_DOWN)
				for (k = 0;k < wild->dungeon->maxdepth; k++)
				{
					wpos.wz=-k;
					if((getcave(&wpos)) && (!players_on_depth(&wpos))) wipe_o_list(&wpos);
				}
		}
	}
	/* Stop the timer */
	teardown_timer();

	if (!save_server_info()) quit("server panic info save failed!");


	/* Dump a nice core - Chris */
#ifdef HANDLE_SIGNALS
	signal(11, 0);
#ifndef WINDOWS
	kill(getpid(), 11);
#endif
#endif
	
	/* Successful panic save of server info */
	quit("server panic info save succeeded!");
}



#ifdef HANDLE_SIGNALS




/*
 * Handle signals -- suspend
 *
 * Actually suspend the game, and then resume cleanly
 *
 * This will probably inflict much anger upon the suspender, but it is still
 * allowed (for now) --KLJ--
 */
static void handle_signal_suspend(int sig)
{
	/* Disable handler */
	(void)signal(sig, SIG_IGN);

#ifdef SIGSTOP

	/* Suspend ourself */
#ifndef WINDOWS
	(void)kill(0, SIGSTOP);
#else
	raise(SIGSTOP);
#endif

#endif

	/* Restore handler */
	(void)signal(sig, handle_signal_suspend);
}


/*
 * Handle signals -- simple (interrupt and quit)
 *
 * This function was causing a *huge* number of problems, so it has
 * been simplified greatly.  We keep a global variable which counts
 * the number of times the user attempts to kill the process, and
 * we commit suicide if the user does this a certain number of times.
 *
 * We attempt to give "feedback" to the user as he approaches the
 * suicide thresh-hold, but without penalizing accidental keypresses.
 *
 * To prevent messy accidents, we should reset this global variable
 * whenever the user enters a keypress, or something like that.
 *
 * This simply calls "exit_game_panic()", which should try to save
 * everyone's character and the server info, which is probably nicer
 * than killing everybody. --KLJ--
 */
static void handle_signal_simple(int sig)
{
	/* Disable handler */
	(void)signal(sig, SIG_IGN);


	/* Nothing to save, just quit */
	if (!server_generated || server_saved) quit(NULL);


	/* Count the signals */
	signal_count++;


	/* Allow suicide (after 5) */
	if (signal_count >= 5)
	{
		/* Tell the metaserver that we've quit */
		Report_to_meta(META_DIE);

		/* Save everything and quit the game */
//              exit_game_panic();
		shutdown_server();
	}

	/* Give warning (after 4) */
	else if (signal_count >= 4)
	{
		s_printf("Warning: Next signal kills server!\n");
	}

	/* Restore handler */
	(void)signal(sig, handle_signal_simple);
}


/*
 * Handle signal -- abort, kill, etc
 *
 * This one also calls exit_game_panic() --KLJ--
 */
static void handle_signal_abort(int sig)
{
	/* Disable handler */
	(void)signal(sig, SIG_IGN);


	/* Nothing to save, just quit */
	if (!server_generated || server_saved) quit(NULL);

	/* Save everybody and quit */
	exit_game_panic();
}




/*
 * Ignore SIGTSTP signals (keyboard suspend)
 */
void signals_ignore_tstp(void)
{

#ifdef SIGTSTP
	(void)signal(SIGTSTP, SIG_IGN);
#endif

}

/*
 * Handle SIGTSTP signals (keyboard suspend)
 */
void signals_handle_tstp(void)
{

#ifdef SIGTSTP
	(void)signal(SIGTSTP, handle_signal_suspend);
#endif

}


/*
 * Prepare to handle the relevant signals
 */
void signals_init(void)
{

#ifdef SIGHUP
	(void)signal(SIGHUP, SIG_IGN);
#endif


#ifdef SIGTSTP
	(void)signal(SIGTSTP, handle_signal_suspend);
#endif


#ifdef SIGINT
	(void)signal(SIGINT, handle_signal_simple);
#endif

#ifdef SIGQUIT
	(void)signal(SIGQUIT, handle_signal_simple);
#endif


#ifdef SIGFPE
	(void)signal(SIGFPE, handle_signal_abort);
#endif

#ifdef SIGILL
	(void)signal(SIGILL, handle_signal_abort);
#endif

#ifdef SIGTRAP
	(void)signal(SIGTRAP, handle_signal_abort);
#endif

#ifdef SIGIOT
	(void)signal(SIGIOT, handle_signal_abort);
#endif

#ifdef SIGKILL
	(void)signal(SIGKILL, handle_signal_abort);
#endif

#ifdef SIGBUS
	(void)signal(SIGBUS, handle_signal_abort);
#endif

#ifdef SIGSEGV
	(void)signal(SIGSEGV, handle_signal_abort);
#endif

#ifdef SIGTERM
	(void)signal(SIGTERM, handle_signal_abort);
#endif

	/*
	 * This happens naturaly when clients disconnect.
#ifdef SIGPIPE
	(void)signal(SIGPIPE, handle_signal_abort);
#endif
*/

#ifdef SIGEMT
	(void)signal(SIGEMT, handle_signal_abort);
#endif

#ifdef SIGDANGER
	(void)signal(SIGDANGER, handle_signal_abort);
#endif

#ifdef SIGSYS
	(void)signal(SIGSYS, handle_signal_abort);
#endif

#ifdef SIGXCPU
	(void)signal(SIGXCPU, handle_signal_abort);
#endif

#ifdef SIGPWR
	(void)signal(SIGPWR, handle_signal_abort);
#endif

}


#else   /* HANDLE_SIGNALS */


/*
 * Do nothing
 */
void signals_ignore_tstp(void)
{
}

/*
 * Do nothing
 */
void signals_handle_tstp(void)
{
}

/*
 * Do nothing
 */
void signals_init(void)
{
}


#endif  /* HANDLE_SIGNALS */


