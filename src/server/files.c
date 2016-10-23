/* $Id$ */
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
#ifndef WINDOWS
#include <sys/wait.h>
#endif
#endif

/* The first x highscore entries that are displayed to players: */
#define SCORES_SHOWN 28

/*
 * You may or may not want to use the following "#undef".
 */
/* #undef _POSIX_SAVED_IDS */

/* Use first line of file as stationary title caption? If disabled,
   'what' parm of do_cmd_help_aux() will be used instead (normal). - C. Blue */
//#define HELP_AUX_GRABS_TITLE


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
#if 0
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
#else	// 0
s16b tokenize(char *buf, s16b num, char **tokens, char delim1, char delim2)
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
			if ((*t == delim1) || (*t == delim2)) break;

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
#endif	// 0

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
	while (0 == my_fgets(fp, buf, 80, FALSE))
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
	while (0 == my_fgets(fp, buf, 1024, FALSE))
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




#if 0
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
static void display_player_middle(int Ind) {
	player_type *p_ptr = Players[Ind];
	int bmh = 0, bmd = 0;
	s32b adv_exp, adv_exp_prev;

	int show_tohit_m = p_ptr->dis_to_h + p_ptr->to_h_melee;
	int show_todam_m = p_ptr->dis_to_d + p_ptr->to_d_melee;
/*	int show_tohit_m = p_ptr->to_h_melee;
	int show_todam_m = p_ptr->to_d_melee;
*/
	int show_tohit_r = p_ptr->dis_to_h + p_ptr->to_h_ranged;
	int show_todam_r = p_ptr->to_d_ranged;

        /* well, about dual-wield.. we can only display the boni of one weapon or their average until we add another line
           to squeeze info about the secondary weapon there too. so for now let's just stick with this. - C. Blue */
	object_type *o_ptr = &p_ptr->inventory[INVEN_WIELD];
	object_type *o_ptr2 = &p_ptr->inventory[INVEN_BOW];
	object_type *o_ptr3 = &p_ptr->inventory[INVEN_AMMO];
	object_type *o_ptr4 = &p_ptr->inventory[INVEN_ARM];

	/* Hack -- add in weapon info if known */
	if (object_known_p(Ind, o_ptr)) {
		bmh += o_ptr->to_h;
		bmd += o_ptr->to_d;
	}
	if (object_known_p(Ind, o_ptr2)) {
		show_tohit_r += o_ptr2->to_h;
		show_todam_r += o_ptr2->to_d;
	}
	if (object_known_p(Ind, o_ptr3)) {
		show_tohit_r += o_ptr3->to_h;
		show_todam_r += o_ptr3->to_d;
	}
	/* dual-wield..*/
	if (o_ptr4->k_idx && o_ptr4->tval != TV_SHIELD) {
		if (object_known_p(Ind, o_ptr4)) {
			bmh += o_ptr4->to_h;
			bmd += o_ptr4->to_d;
		}
		if (object_known_p(Ind, o_ptr) && object_known_p(Ind, o_ptr4)) {
			/* average of both */
			bmh /= 2;
			bmd /= 2;
		}
	}

	show_tohit_m += bmh;
	show_todam_m += bmd;

	/* Dump the bonuses to hit/dam */
//	Send_plusses(Ind, show_tohit_m, show_todam_m, show_tohit_r, show_todam_r, p_ptr->to_h_melee, p_ptr->to_d_melee);
	Send_plusses(Ind, 0, 0, show_tohit_r, show_todam_r, show_tohit_m, show_todam_m);

	/* Dump the armor class bonus */
	Send_ac(Ind, p_ptr->dis_ac, p_ptr->dis_to_a);

	if (p_ptr->lev >= (is_admin(p_ptr) ? PY_MAX_LEVEL : PY_MAX_PLAYER_LEVEL))
		adv_exp = 0;
	else {
		s64b adv_prev = 0;
#ifndef ALT_EXPRATIO
		s64b adv = ((s64b)player_exp[p_ptr->lev - 1] * (s64b)p_ptr->expfact / 100L);
		if (p_ptr->lev > 1) adv_prev = ((s64b)player_exp[p_ptr->lev - 2] * (s64b)p_ptr->expfact / 100L);
#else
		s64b adv = (s64b)player_exp[p_ptr->lev - 1];
		if (p_ptr->lev > 1) adv_prev = (s64b)player_exp[p_ptr->lev - 2];
#endif
		adv_exp = (s32b)(adv);
		adv_exp_prev = (s32b)(adv_prev);
	}

	Send_experience(Ind, p_ptr->lev, p_ptr->max_exp, p_ptr->exp, adv_exp, adv_exp_prev);
	Send_gold(Ind, p_ptr->au);
	Send_hp(Ind, p_ptr->mhp, p_ptr->chp);
	Send_sp(Ind, p_ptr->msp, p_ptr->csp);
	Send_stamina(Ind, p_ptr->mst, p_ptr->cst);
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
	Send_char_info(Ind, p_ptr->prace, p_ptr->pclass, p_ptr->ptrait, p_ptr->male, p_ptr->mode, p_ptr->name);

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
#endif	// 0

/*
 * Recursive "help file" perusal.  Return FALSE on "ESCAPE".
 *
 * XXX XXX XXX Consider using a temporary file.
 *
 * Added 'odd_line' flag: Usually FALSE -> 20 lines available per 'page'.
 *                        If TRUE -> 21 lines available! Added this for
 *                        @-screen w/ COMPACT_PLAYERLIST - C. Blue
 *             *changed it to 'div3_line', for cleaner addition of more of this type.
 *             *changed it to 'divl' to be most flexible.
 */
static bool do_cmd_help_aux(int Ind, cptr name, cptr what, s32b line, int color, int divl)
{
	int lines_per_page = 20 + HGT_PLUS;
	int i, k = 0;
	/* Number of "real" lines passed by */
	s32b next = 0;
	/* Number of "real" lines in the file */
	s32b size = 0;
	/* Backup value for "line" */
	s32b back = 0;
	/* This screen has sub-screens */
	//bool menu = FALSE;
	/* Current help file */
	FILE *fff = NULL;
	/* Find this string (if any) */
	cptr find = NULL;
	/* Hold a string to find */
	char finder[128];
	/* Hold a string to show */
	char shower[128];
	/* Describe this thing */
	char caption[128];
	/* Path buffer */
	char path[MAX_PATH_LENGTH];
	/* General buffer */
	char buf[1024];
	/* Sub-menu information */
	char hook[10][32];
	/* Stationary title bar, derived from 1st line of the file or from 'what' parm. */
#ifdef HELP_AUX_GRABS_TITLE
	bool use_title = FALSE;
#endif

	if (is_newer_than(&Players[Ind]->version, 4, 4, 7, 0, 0, 0)) {
		/* use first line in the file as stationary title */
#ifdef HELP_AUX_GRABS_TITLE
		use_title = TRUE;
#endif

		if (divl) {
			if (is_older_than(&Players[Ind]->version, 4, 4, 9, 4, 0, 0)) {
				/* 21-lines-per-page feature requires client 4.4.7.1 aka 4.4.7a */
				if (divl == 3) {
					lines_per_page = 21 + HGT_PLUS - ((21 + HGT_PLUS) % 3);
					/* hack: tell client we'd like a mod3 # of lines per page */
					Send_special_line(Ind, 0, 21 + HGT_PLUS, 0, "");
				}
			} else {
				lines_per_page = 21 + HGT_PLUS - ((21 + HGT_PLUS) % divl);
				/* hack: tell client we'd like a mod4 # of lines per page */
				Send_special_line(Ind, 0, 20 + divl + HGT_PLUS, 0, "");
			}
		}
	}

	/* Wipe finder */
	strcpy(finder, "");
	/* Wipe shower */
	strcpy(shower, "");
	/* Wipe caption */
	strcpy(caption, "");

	/* Wipe the hooks */
	for (i = 0; i < 10; i++) hook[i][0] = '\0';


	/* Hack XXX XXX XXX */
	if (what) {
		/* Caption */
		strcpy(caption, what);
		/* Access the "file" */
		strcpy(path, name);
		/* Open */
		fff = my_fopen(path, "rb");
	}

	/* Look in "help" */
	if (!fff) {
#if 0 /* will overwrite legens-rev.log's title if the file doesn't exist yet */
		/* Caption */
		snprintf(caption, sizeof(caption), "Help file '%s'", name);
#endif
		/* Build the filename */
		path_build(path, MAX_PATH_LENGTH, ANGBAND_DIR_TEXT, name);
		/* Open the file */
		fff = my_fopen(path, "rb");
	}

#ifndef HELP_AUX_GRABS_TITLE
	if (strlen(caption)) {
		k = (72 - strlen(caption)) / 2;
		strcpy(buf, "");
		for (i = 0; i < k; i++) strcat(buf, " ");
		strcat(buf, "\377W- [\377w");
		strcat(buf, caption);
		strcat(buf, "\377W] -");
		Send_special_line(Ind, size, -1, TERM_WHITE, buf);
		k = 0;
	}
#endif

	/* Oops */
	if (!fff) {
#if 0 /* no spam -  the msg is written many times at once and also not needed */
		/* Message */
		msg_format(Ind, "Cannot open '%s'.", name);
		msg_print(Ind, NULL);
#endif

		/* Oops */
		return (TRUE);
	}


	/* Pre-Parse the file */
	while (TRUE) {
		/* Read a line or stop */
		if (my_fgets(fff, buf, 1024, FALSE)) break;

		/* XXX Parse "menu" items */
		if (prefix(buf, "***** ")) {
			char b1 = '[', b2 = ']';

			/* Notice "menu" requests */
			if ((buf[6] == b1) && isdigit(buf[7]) &&
			    (buf[8] == b2) && (buf[9] == ' '))
			{
				/* This is a menu file */
				//menu = TRUE;

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
#ifdef HELP_AUX_GRABS_TITLE
	if (use_title) size--;
#endif


	/* Display the file */
	/* Restart when necessary */
	if (line >= size) line = 0;

	/* (Consistent behavior with peruse_file() in c-files.c
	   and Receive_special_line() in nclient.c.) */
	if (line > size - lines_per_page) {
		line = size - lines_per_page;
		if (line < 0) line = 0;
	}


	/* Re-open the file if needed */
	if (next > line) {
		/* Close it */
		my_fclose(fff);

		/* Hack -- Re-Open the file */
		fff = my_fopen(path, "rb");

		/* Oops */
		if (!fff) return (FALSE);

		/* File has been restarted */
		next = 0;

#ifdef HELP_AUX_GRABS_TITLE
		/* Use special 'title' line? -> Skip in regular use. */
		if (use_title)
			if (!my_fgets(fff, buf, 1024, FALSE))
				Send_special_line(Ind, size, -1, TERM_WHITE, buf);
#endif
	}

	/* Skip lines if needed */
	for (; next < line; next++) {
		/* Skip a line */
		if (my_fgets(fff, buf, 1024, FALSE)) break;
	}


	/* Dump the next 20 lines of the file */
	for (i = 0; i < lines_per_page; ) {
		byte attr = TERM_WHITE;

		/* Hack -- track the "first" line */
		if (!i) line = next;

		/* Get a line of the file or stop */
		if (my_fgets(fff, buf, 1024, FALSE)) break;

		/* Hack -- skip "special" lines */
		if (prefix(buf, "***** ")) continue;

		/* Count the "real" lines */
		next++;

		/* Hack -- keep searching */
		if (find && !i && !strstr(buf, find)) continue;

		/* Hack -- stop searching */
		find = NULL;

		if (buf[0] == '\n') continue;

#if 0	// This will now be done by \377? codes! - C. Blue
		/* Extract color */
		if (color) attr = color_char_to_attr(buf[0]);
#endif

		/* Hack -- show matches */
		if (shower[0] && strstr(buf, shower)) attr = TERM_YELLOW;

		/* Dump the line */
#if 0	// see above
		Send_special_line(Ind, size, i, attr, &buf[color]);
#else
		Send_special_line(Ind, size, i, attr, buf);
#endif
		/* Count the printed lines */
		i++;
	}

	/* Hack -- failed search */
	if (find) {
		bell();
		line = back;
		find = NULL;
		my_fclose(fff);	/* The evil file that waited open? */
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
void do_cmd_help(int Ind, int line) {
	/* Peruse the main help file */
	(void)do_cmd_help_aux(Ind, Players[Ind]->rogue_like_commands ? "tomenet-rl.hlp" : "tomenet.hlp", "Welcome to TomeNET", line, FALSE, 0);
}





/*
 * Hack -- display the contents of a file on the screen
 *
 * XXX XXX XXX Use this function for commands such as the
 * "examine object" command.
 */
errr show_file(int Ind, cptr name, cptr what, s32b line, int color, int divl)
{
	/* Peruse the requested file */
	(void)do_cmd_help_aux(Ind, name, what, line, color, divl);

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
	if (strlen(p_ptr->name) > 15) //was 20 once?
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
		else if (strchr(SF_BAD_CHARS, c)) p_ptr->basename[k++] = '_';
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
		(void)snprintf(temp, sizeof(temp), "%s", p_ptr->basename);

#ifdef VM
		/* Hack -- support "flat directory" usage on VM/ESA */
		(void)snprintf(temp, sizeof(temp), "%s.sv", player_base);
#endif /* VM */

		/* Build the filename */
		path_build(p_ptr->savefile, MAX_PATH_LENGTH, ANGBAND_DIR_SAVE, temp);
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
void do_cmd_suicide(int Ind) {
	player_type *p_ptr = Players[Ind];

	/* Don't allow PvP characters this way to deny their opponent the kill credit */
	if (in_pvparena(&p_ptr->wpos)) {
		msg_print(Ind, "\377yGladiators never suicide in the arena!");
		return;
	}

	/* Mark as suicide */
	p_ptr->alive = FALSE;

	/* Hack -- set the cause of death */
	if (!p_ptr->ghost) {
		//strcpy(p_ptr->died_from, "");
		strcpy(p_ptr->died_from_list, "self-inflicted wounds");
		p_ptr->died_from_depth = getlevel(&p_ptr->wpos);
	}

	/* Hack -- clear ghost */
	p_ptr->ghost = 0;

	if (p_ptr->total_winner) {
		if (p_ptr->iron_winner) kingly(Ind, 4);
		else kingly(Ind, 1);
	} else if (p_ptr->iron_winner) kingly(Ind, 3);
	/* Retirement in Valinor? - C. Blue :) */
	if (in_valinor(&p_ptr->wpos)) kingly(Ind, 2);

	/* Kill him */
	p_ptr->deathblow = 0;
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
/* FIXME: this function returns bad value when max_exp is stupidly large
 * (usually admin chars) */
int total_points(int Ind) {
	u32b points, tmp_base, tmp1, tmp2, tmp3, tmp3a, bonusm, bonusd;
	u32b lev_factoring;
	player_type *p_ptr = Players[Ind];

	/* kill maggot for 100% bonus on total score? -> no
	   why a little bonus for HELL mode? the honour for the player
	   who chooses hell mode on his own is far greater without it. -> no
	   add cash to exp? what if the player collected cool gear instead?
	   make cash allow the player to skip lots of levels on the ladder?  -> no
	   max_dlv? just enter the staircase in lothlorien and back up. -> no
	   For now let's calc it basing on pure progress in gameplay!: */
	//exp counts mainly, level factors in
//	return (p_ptr->max_exp * (p_ptr->max_plv + 30) / 30);

	/* Bonus */
	bonusm = 100;
	bonusd = 100;
	if (p_ptr->mode & MODE_NO_GHOST) bonusm += 25;
	if (p_ptr->mode & MODE_HARD) bonusm += 25;

#ifndef ALT_EXPRATIO
	/* Bonus might cause overflow at lvl 94+ - so maybe compensate */
	tmp_base = (p_ptr->max_exp * 3) / 3; //leaving this, changing lev_factoring instead..
	lev_factoring = 58; //was 33 -> overflow at 94+

	/* split the number against overflow bug */
	tmp1 = p_ptr->lev + lev_factoring;
	tmp2 = lev_factoring;
	tmp3a = tmp_base % 10000000;
	tmp3 = (tmp_base - tmp3a) / 10000000;
	points = (((tmp3a * bonusm) / bonusd) * tmp1) / tmp2;
	points += ((((10000000 * bonusm) / bonusd) * tmp1) / tmp2) * tmp3;
#else
	tmp_base = p_ptr->max_exp;
	lev_factoring = 75; /* (yeek warrior -> tl mimic : *3) */
	lev_factoring = 230; /* (yeek warrior -> tl mimic : *2) */

	/* split the number against overflow bug; divide by 10 to avoid overflow again */
	tmp1 = (p_ptr->expfact + lev_factoring) / 10;
	tmp2 = 310 / 10; /* (230 + 80 (yeek warrior)) */
	tmp3a = tmp_base % 10000000;
	tmp3 = (tmp_base - tmp3a) / 10000000;
	points = (((tmp3a * bonusm) / bonusd) * tmp1) / tmp2;
	points += ((((10000000 * bonusm) / bonusd) * tmp1) / tmp2) * tmp3;
#endif
	return points;

	//level counts mainly, exp factors in at higher levels
	//return (p_ptr->max_plv * (300 + (p_ptr->max_exp / 100000)) / 300);
#if 0
	/* Maggot bonus.. beware, r_idx is hard-coded! */
	int i = p_ptr->r_killed[8]? 50 : 100;
	if (p_ptr->mode & MODE_HARD) i = i * 5 / 4;

	if (p_ptr->mode & MODE_NO_GHOST) return (((p_ptr->max_exp + (100 * p_ptr->max_dlv)) * 4 / 3)*i/100);
	else return ((p_ptr->max_exp + (100 * p_ptr->max_dlv) + p_ptr->au)*i/100);
#endif //0
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

	char pts[11];           /* Total Score (number) */

	char gold[11];          /* Total Gold (number) */

	char turns[11];         /* Turns Taken (number) */

	char day[11];           /* Time stamp (string) */

	char who[16];           /* Player Name (string) */
	char whose[16];		/* Account Name (string) */

	char sex[2];            /* Player Sex (string) */
	char p_r[3];            /* Player Race (number) */
	char p_c[3];            /* Player Class (number) */

	char cur_lev[4];                /* Current Player Level (number) */
	char cur_dun[4];                /* Current Dungeon Level (number) */
	char max_lev[4];                /* Max Player Level (number) */
	char max_dun[4];                /* Max Dungeon Level (number) */

	char how[50];           /* Method of death (string) */

	char mode[1];		/* Difficulty/character mode */
};

#if 0 /* different format? */
typedef struct high_score_old high_score_old;
struct high_score_old
{
	char what[8];           /* Version info (string) */

	char pts[11];           /* Total Score (number) */

	char gold[11];          /* Total Gold (number) */

	char turns[11];         /* Turns Taken (number) */

	char day[10];           /* Time stamp (string) */

	char who[16];           /* Player Name (string) */
	char whose[16];		/* Account Name (string) */

	char sex[2];            /* Player Sex (string) */
	char p_r[3];            /* Player Race (number) */
	char p_c[3];            /* Player Class (number) */

	char cur_lev[4];                /* Current Player Level (number) */
	char cur_dun[4];                /* Current Dungeon Level (number) */
	char max_lev[4];                /* Max Player Level (number) */
	char max_dun[4];                /* Max Dungeon Level (number) */

	char how[50];           /* Method of death (string) */

	char mode[1];		/* Difficulty/character mode */
};
#else /* same format actually */
typedef struct high_score high_score_old;
#endif


int highscore_send(char *buffer, int max) {
	int len = 0, len2;
	FILE *hsp;
	char buf[1024];
	struct high_score score;
	cptr mode, race, class;

	path_build(buf, 1024, ANGBAND_DIR_DATA, "scores.raw");

	hsp = fopen(buf, "rb");
	if (!hsp) return(0);

	while (fread(&score, sizeof(struct high_score), 1, hsp)) {
		switch (score.mode[0] & MODE_MASK) {
			case MODE_NORMAL:
				mode = "Normal";
				break;
			case MODE_HARD:
				mode = "Hard";
				break;
			case MODE_NO_GHOST:
				mode = "Unworldly";
				break;
			case (MODE_HARD | MODE_NO_GHOST):
				mode = "Hellish";
				break;
			case MODE_EVERLASTING:
				mode = "Everlasting";
				break;
			case MODE_PVP:
				mode = "PvP";
				break;
			default:
				mode = "Unknown";
				break;
		}

		race = race_info[atoi(score.p_r)].title;
		class = class_info[atoi(score.p_c)].title;

		len2 = snprintf(buf, 1024, "pts=%s\ngold=%s\nturns=%s\nday=%s\nwho=%s\nwhose=%s\nsex=%s\nrace=%s\nclass=%s\ncur_lev=%s\ncur_dun=%s\nmax_lev=%s\nmax_dun=%s\nhow=%s\nmode=%s\n",
			score.pts, score.gold, score.turns, score.day, score.who, score.whose, score.sex, race, class, score.cur_lev, score.cur_dun, score.max_lev, score.max_dun, score.how, mode);

		if (len2 + len < max) {
			memcpy(&buffer[len], buf, len2);
			len += len2;
			buffer[len] = '\0';
		}
	}

	fclose(hsp);

	return len;
}

int houses_send(char *buffer, int max) {
	int i, len = 0, len2, screen_x, screen_y;
	house_type *h_ptr;
	struct dna_type *dna;
	char buf[1024];
	s32b price;
	cptr owner, wpos;

	for (i = 0; i < num_houses; i++) {
		h_ptr = &houses[i];
		dna = h_ptr->dna;

		screen_y = h_ptr->dy * 5 / MAX_HGT;
		screen_x = h_ptr->dx * 5 / MAX_WID;
		wpos = wpos_format(0, &h_ptr->wpos);

		price = dna->price;

		if (houses[i].dna->owner_type == OT_PLAYER)
			owner = lookup_player_name(houses[i].dna->owner);
		else if (houses[i].dna->owner_type == OT_GUILD)
			owner = guilds[houses[i].dna->owner].name;
		else owner = "";

		if (!owner) owner = "";

		if (houses[i].dna->owner_type == OT_GUILD)
			len2 = snprintf(buf, 1024, "location=[%d,%d] in %s\nprice=%d\nguild=%s\n",
			    screen_y, screen_x, wpos, price, owner);
		else
			len2 = snprintf(buf, 1024, "location=[%d,%d] in %s\nprice=%d\nowner=%s\n",
			    screen_y, screen_x, wpos, price, owner);

		if (len + len2 < max) {
			memcpy(&buffer[len], buf, len2);
			len += len2;
			buffer[len] = '\0';
		}
	}

	return len;
}

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

static int highscore_seek_old(int i)
{
	/* Seek for the requested record */
	return (fd_seek(highscore_fd, (huge)(i) * sizeof(high_score_old)));
}
static errr highscore_read_old(high_score_old *score)
{
	/* Read the record, note failure */
	return (fd_read(highscore_fd, (char*)(score), sizeof(high_score_old)));
}



/*
 * Just determine where a new score *would* be placed
 * Return the location (0 is best) or -1 on failure
 */
#ifndef NEW_HISCORE
static int highscore_where(high_score *score, bool *move_up, bool *move_down)
#else
static int highscore_where(high_score *score, int *erased_slot)
#endif
{
	int             i, slot_ret = -1;
	high_score      the_score;
#ifndef NEW_HISCORE
	int		slot_pts = -1, slot_name = -1, slot_account = -1;
#else
	bool		flag_char, flag_acc, flag_c, flag_r;
	int		slot_old = -1;
	int		entries_from_same_account = 0;
#endif

	/* Paranoia -- it may not have opened */
	if (highscore_fd < 0) return (-1);

	/* Go to the start of the highscore file */
	if (highscore_seek(0)) return (-1);


#ifndef NEW_HISCORE /*restructuring with new hex flags, see #else branch - C. Blue */
	/* Read until we get to a higher score
	   or appropriate rules are met -C. Blue */
	for (i = 0; i < MAX_HISCORES; i++)
	{
		if (highscore_read(&the_score))
		{
			if (slot_pts == -1) slot_pts = i;
			break;
		}
		if ((strcmp(the_score.pts, score->pts) < 0) && (slot_pts == -1))
    			slot_pts = i;
		if ((!strcmp(the_score.who, score->who)) && (slot_name == -1))
			slot_name = i;
		if ((!strcmp(the_score.whose, score->whose)) && (slot_account == -1))
			slot_account = i;
	}

	/* Insert score right before any higher score */
	(*move_up) = FALSE; (*move_down) = FALSE;
	if (slot_pts != -1) slot_ret = slot_pts;

	/* If this char is already on high-score list,
	   and replace_hiscore is set to 1 then replace it! */
	if ((cfg.replace_hiscore == 1) && (slot_name != -1)) {
		/* Replace old score of this character */
		if (slot_pts != -1) {
			if (slot_name > slot_pts) (*move_up) = TRUE;
			if (slot_name < slot_pts) (*move_down) = TRUE;
		} else {
			if (slot_name < MAX_HISCORES - 1) (*move_down) = TRUE;
		}
	}
	/* If this char is already on high-score list,
	   and the new score is better than the old one
	   and replace_hiscore is set to 2 then replace it! */
	else if ((cfg.replace_hiscore == 2) && (slot_name != -1)) {
		if ((slot_pts > slot_name) || (slot_pts == -1)) {
			/* ignore the new score */
			slot_ret = -2; /* hack */
		} else {
			/* If we just have to replace a score with a
			   better one at the _same_ position, we still
			   set move_up = TRUE since this will prevent
			   the previous entry from sliding down and have
			   it been overwritten instead */
			(*move_up) = TRUE;
		}
	}

	if (slot_ret != -1) return (slot_ret);

	/* The "last" entry is always usable */
	return (MAX_HISCORES - 1);
#else
	/* Read entries until we find a slot with lower score or arrive at the end */
	for (i = 0; i < MAX_HISCORES; i++) {
		if (highscore_read(&the_score)) {
			/* arrived at the end of entries -> take this one then */
			slot_ret = i;
			break;
		}
		/* always valid rule: sort in after next-highest score */
		if (strcmp(the_score.pts, score->pts) < 0) {
			slot_ret = i;
			break;
		}
	}
	/* hack: last entry is always 'available' (effectively unused though!)  */
	if (slot_ret == -1) slot_ret = MAX_HISCORES - 1;

 #if 0 /* don't start after the appropriate slot, but check all slots */
	/* Go to next position in highscore file, if available. Otherwise, skip the for loop: */
	if (!highscore_seek(slot_ret + 1))
	/* now check previous entries before adding this one, if required
	   according to our custom extended rule set from tomenet.cfg */
	for (i = slot_ret + 1; i < MAX_HISCORES; i++) {
 #else
	/* Go to the start of the highscore file */
	if (highscore_seek(0)) return (-1);
	/* now check previous entries before adding this one, if required
	   according to our custom extended rule set from tomenet.cfg,
	   whether we find an entry we have to actually replace with the
	   new one.. */
	for (i = 0; i < MAX_HISCORES; i++) {
 #endif
		if (highscore_read(&the_score)) {
			/* arrived at the end of entries -> nothing to replace */
			break;
		}
		/* first, prepare flag test results */
		flag_char = flag_acc = flag_c = flag_r = TRUE;
		/* check for same char name */
		if (cfg.replace_hiscore & 0x08) {
			if (strcmp(the_score.who, score->who)) flag_char = FALSE;
		}
		/* check for same acc name */
		if (strcmp(the_score.whose, score->whose)) {
			if (cfg.replace_hiscore & 0x10) flag_acc = FALSE;
		} else { /* for cfg.replace_hiscore & 0x7 > 2 */
			entries_from_same_account++;
		}
		/* check for same class */
		if (cfg.replace_hiscore & 0x20) {
			if (strcmp(the_score.p_c, score->p_c)) flag_c = FALSE;
		}
		/* check for same race */
		if (cfg.replace_hiscore & 0x40) {
			if (strcmp(the_score.p_r, score->p_r)) flag_r = FALSE;
		}

		/* now test special modes in combination with flag results */
		if ((cfg.replace_hiscore & 0x7) == 1) {
			/* replace older entries by newer entries */
			if (flag_char && flag_acc && flag_c && flag_r) {
				slot_old = i;
				break;
			}
		}
		if ((cfg.replace_hiscore & 0x7) == 2) {
			/* replace lower entries by higher entries */
			if (flag_char && flag_acc && flag_c && flag_r) {
				if (strcmp(the_score.pts, score->pts) < 0) {
					slot_old = i;
					break;
				} else {
					/* hack for display_scores_aux later:
					   show that this entry is our 'goal' (use another colour) */
					slot_old = -2 -i; /* <= -2 -> 'there exists a slot, but we can't reach it yet' */
				}
			}
		}
		if ((cfg.replace_hiscore & 0x7) == 3) {
			/* replace lowest entry of this account which matches the flags */
			if (flag_char && flag_acc && flag_c && flag_r) {
				if (strcmp(the_score.pts, score->pts) < 0) {
					slot_old = i;
				} else {
					/* hack for display_scores_aux later:
					   show that this entry is our 'goal' (use another colour) */
					slot_old = -2 -i; /* <= -2 -> 'there exists a slot, but we can't reach it yet' */
				}
			/* replace last entry of this account anyway due to limitation of this mode? */
			} else if (entries_from_same_account == 2) {
				if (strcmp(the_score.pts, score->pts) < 0) {
					slot_old = i;
					break;
				} else {
					/* hack for display_scores_aux later:
					   show that this entry is our 'goal' (use another colour) */
					slot_old = -2 -i; /* <= -2 -> 'there exists a slot, but we can't reach it yet' */
				}
			}
		}
		if ((cfg.replace_hiscore & 0x7) == 4) {
			/* replace lowest entry of this account which matches the flags */
			if (flag_char && flag_acc && flag_c && flag_r) {
				if (strcmp(the_score.pts, score->pts) < 0) {
					slot_old = i;
				} else {
					/* hack for display_scores_aux later:
					   show that this entry is our 'goal' (use another colour) */
					slot_old = -2 -i; /* <= -2 -> 'there exists a slot, but we can't reach it yet' */
				}
			/* replace last entry of this account anyway due to limitation of this mode? */
			} else if (entries_from_same_account == 3) {
				if (strcmp(the_score.pts, score->pts) < 0) {
					slot_old = i;
					break;
				} else {
					/* hack for display_scores_aux later:
					   show that this entry is our 'goal' (use another colour) */
					slot_old = -2 -i; /* <= -2 -> 'there exists a slot, but we can't reach it yet' */
				}
			}
		}
	}

 #if 0 /* basic version without -2 -i slot_old hack for display_scores_aux */
	/* If this char is already on high-score list,
	   and replace_hiscore is set to values other than 0,
	   - resulting in 'slot_old != -1' - then replace it! */
	(*erased_slot) = -1;
	/* found an old entry to remove? */
	if (slot_old > -1) {
		/* if the old entry is on last position, it'll be overwritten anyway */
		if (slot_old < MAX_HISCORES - 1) {
			(*erased_slot) = slot_old;
		}
	}
	/* Nothing to replace, although we're in replace mode,
	   ie we theoretically made it into the high score, but
	   our previous entry was too good to be replaced, and
	   thereby our new entry is to be discarded? */
	else if ((cfg.replace_hiscore & 0x7) != 0)
		slot_ret = -1; /* hack: 'ignore the new score' */
 #else /* hacked for display_scores_aux best possible display */
	/* If this char is already on high-score list,
	   and replace_hiscore is set to values other than 0,
	   - resulting in 'slot_old > -1' - then replace it! */
	(*erased_slot) = slot_old;
	/* Nothing to replace, although we're in replace mode
	   AND we found a previous matching entry (slot_old < -1):
	   ie we theoretically made it into the high score, but
	   our previous entry was too good to be replaced, and
	   thereby our new entry is to be discarded? */
	if (((cfg.replace_hiscore & 0x7) != 0) && slot_old < -1)
		slot_ret = -2 - slot_ret; /* hack: 'ignore the new score'
					     hack: useful display for display_scores_aux */
 #endif

	return (slot_ret);
#endif
}


/*
 * Actually place an entry into the high score file
 * Return the location (0 is best) or -1 on "failure"
 */
static int highscore_add(high_score *score) {
	int             i, slot;
	high_score      the_score, tmpscore;
	bool            done = FALSE;
#ifndef NEW_HISCORE
	bool            move_up, move_down;
#else
	int		erased_slot, cur_slots = MAX_HISCORES;
#endif

	/* Paranoia -- it may not have opened */
	if (highscore_fd < 0) return (-1);

	/* Determine where the score should go */
#ifndef NEW_HISCORE
	slot = highscore_where(score, &move_up, &move_down);
#else
	slot = highscore_where(score, &erased_slot);
#endif
	/* Hack -- Not on the list
	   (ie didn't match conditions to replace previous score) */
	if (slot < 0) return (-1);

	/* Hack -- prepare to dump the new score */
	the_score = (*score);

#ifndef NEW_HISCORE
	if (!move_down) {
		/* Slide all the scores down one */
		for (i = slot; !done && (i < MAX_HISCORES); i++) {
			/* Read the old guy, note errors */
			if (highscore_seek(i)) return (-1);
			if (highscore_read(&tmpscore)) done = TRUE;

			/* Back up and dump the score we were holding */
			if (highscore_seek(i)) return (-1);
			if (highscore_write(&the_score)) return (-1);

			if (move_up && !strcmp(score->who, tmpscore.who)) {
				/* If older score is to be removed, we can stop here */
				return(slot);
			}

			/* Hack -- Save the old score, for the next pass */
			the_score = tmpscore;
		}
	} else {
		/* Move upwards through the score board */
		for (i = slot; !done && (i >= 0); i--) {
			/* Read the old guy, note errors */
			if (highscore_seek(i)) return (-1);
			if (highscore_read(&tmpscore)) done = TRUE;

			/* Back up and dump the score we were holding */
			if (highscore_seek(i)) return (-1);
			if (highscore_write(&the_score)) return (-1);

			if (!strcmp(score->who, tmpscore.who)) {
				/* If older score is to be removed, we can stop here */
				return(slot);
			}

			/* Hack -- Save the old score, for the next pass */
			the_score = tmpscore;
		}
	}
#else
	/* Erase one old entry first? */
	if (erased_slot > -1) {
		cur_slots = erased_slot + 1; /* how many used slots does the high score table currently have? */
		for (i = erased_slot; i < MAX_HISCORES; i++) {
			/* Read the following entry, if any */
			if (highscore_seek(i + 1)) break;
			if (highscore_read(&tmpscore)) break;
			/* do log file entry: */
//			if (i == erased_slot) s_printf("HISCORE_DEL: #%d %s\n", i, tmpscore.who);
			/* hack: count high score table slots in use: */
			cur_slots++;
			/* Overwrite current entry with it */
			if (highscore_seek(i)) return(-1);
			if (highscore_write(&tmpscore)) return(-1);
		}
		/* take note if the newly inserted slot has to be moved
		   because its actually below the erased slot */
		if (slot > erased_slot) slot--;
	}
	/* Insert new entry - Just slide all the scores down one */
	/* note ('for' destination: if we previously erased one entry,
	   then it means the last entry has become obsolete, so we
	   skip it when we shift downwards again, otherwise we'd
	   duplicate it. */
//	for (i = slot; !done && (i < (erased_slot > -1 ? MAX_HISCORES - 1 : MAX_HISCORES)); i++) {
	for (i = slot; !done && (i < cur_slots); i++) {
		/* Read the old guy, note errors */
		if (highscore_seek(i)) return (-1);
		if (highscore_read(&tmpscore)) done = TRUE;
		/* Back up and dump the score we were holding */
		if (highscore_seek(i)) return (-1);
		if (highscore_write(&the_score)) return (-1);
		/* Hack -- Save the old score, for the next pass */
		the_score = tmpscore;
	}
#endif

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
#ifndef NEW_HISCORE
static void display_scores_aux(int Ind, int line, int note, high_score *score)
#else
static void display_scores_aux(int Ind, int line, int note, int erased_slot, high_score *score)
#endif
{
	int             i, j, from, to, place;//, attr
	char attrc[3];

	high_score      the_score;

	char    out_val[256];

	FILE *fff;
	char file_name[MAX_PATH_LENGTH];

	/* Hack - distinguish between wilderness and dungeon */
	bool wilderness;

	/* Hack - display if the player was a former winner */
	char extra_info[40];

	/* Paranoia -- it may not have opened */
	if (highscore_fd < 0) return;

	/* Temporary file */
	if (path_temp(file_name, MAX_PATH_LENGTH)) return;

	/* Open the temp file */
	fff = my_fopen(file_name, "wb");
	if (fff == (FILE*)NULL) return;

	/* Assume we will show the first 20 */
	from = 0;
	to = SCORES_SHOWN;
	if (to > MAX_HISCORES) to = MAX_HISCORES;


	/* Seek to the beginning */
	if (highscore_seek(0)){
		my_fclose(fff);
		return;
	}

	/* Hack -- Count the high scores */
	for (i = 0; i < MAX_HISCORES; i++)
		if (highscore_read(&the_score)) break;

#ifndef NEW_HISCORE
	/* Hack -- allow "fake" entry to be last */
	if ((note == i) && score) i++;
#else
	if (note < -1) note = -(note + 2); /* unwrap display hack */
	if (score) i++; /* hack: insert fake entry */
#endif

	/* Forget about the last entries */
	if (i > to) i = to;

	/* Show 5 per page, until "done" */
	for (j = from, place = j+1; j < i; j++, place++) {
		int pr, pc, clev, mlev, cdun, mdun;
		byte modebuf;
		char modestr[20], modecol[5];
		cptr gold, when, aged;


		/* Hack -- indicate death in yellow */
		//attr = (j == note) ? TERM_YELLOW : TERM_WHITE;


		/* Mega-Hack -- insert a "fake" record */
		if ((note == j) && score) {
			sprintf(attrc, "\377G");
			the_score = (*score);
			//attr = TERM_L_GREEN;
			score = NULL;
			note = -1;
			j--;
			i--;
		}

		/* Read a normal record */
		else {
			sprintf(attrc, "\377w");
#ifdef NEW_HISCORE
			/* unwrap display hack part 2 */
			if (erased_slot < -1 && -(erased_slot + 2) == j)
				/* indicate our 'goal' character we're chasing, score-wise, for replacing him */
				sprintf(attrc, "\377B");
			else if (erased_slot > -1 && erased_slot == j)
				/* indicate our ex-goal which we now seem to have reached, just for fun */
				sprintf(attrc, "\377B");
#endif
			/* Read the proper record */
			if (highscore_seek(j)) break;
			if (highscore_read(&the_score)) break;
		}


		strcpy(extra_info, ".");
		wilderness = FALSE;

		/* Extract the race/class */
		pr = atoi(the_score.p_r);
		pc = atoi(the_score.p_c);

		/* Extract the level info */
		clev = atoi(the_score.cur_lev);
		mlev = atoi(the_score.max_lev);
#if 0
		if (the_score.cur_dun[strlen(the_score.cur_dun) - 1] == '\001') {
			wilderness = TRUE;
			the_score.cur_dun[strlen(the_score.cur_dun) - 1] = '\0';
		}
#endif
		cdun = atoi(the_score.cur_dun);
		mdun = atoi(the_score.max_dun);

		/* Hack -- extract the gold and such */
		for (when = the_score.day; isspace(*when); when++) /* loop */;
		for (gold = the_score.gold; isspace(*gold); gold++) /* loop */;
		for (aged = the_score.turns; isspace(*aged); aged++) /* loop */;

		modebuf = (the_score.mode[0] & MODE_MASK);
		strcpy(modestr, "");
		strcpy(modecol, "");
		switch (modebuf) {
                case MODE_HARD:
			strcpy(modestr, "purgatorial ");
//			strcpy(modecol, "\377s");
	    	        break;
                case MODE_NO_GHOST:
			strcpy(modestr, "unworldly ");
			strcpy(modecol, "\377D");
	                break;
		case (MODE_HARD + MODE_NO_GHOST):
			strcpy(modestr, "hellish ");
			strcpy(modecol, "\377D");
			break;
                case MODE_NORMAL:
//			strcpy(modecol, "\377w");
                        break;
		}

		/* Hack ;) Remember if the player was a former winner */
		if (the_score.how[strlen(the_score.how) - 1] == '\001')
		{
			strcpy(extra_info, ". (Defeated Morgoth)");
			the_score.how[strlen(the_score.how) - 1] = '\0';
		}

		/* Dump some info */
		snprintf(out_val, sizeof(out_val), "%2s%3d.%10s %s%s the %s%s %s, Lv.%d",
			attrc, place, the_score.pts, modecol, the_score.who, modestr, 
			race_info[pr].title, class_info[pc].title,
			clev);

		/* Append a "maximum level" */
		if (mlev > clev) strcat(out_val, format(" (max %d)", mlev));

		/* Dump the first line */
		fprintf(fff, "%s\n", out_val);

		/* Another line of info */
		if (strcmp(the_score.how, "winner") && strcmp(the_score.how, "*winner*") && strcmp(the_score.how, "iron champion") && strcmp(the_score.how, "iron emperor"))
			snprintf(out_val, sizeof(out_val),
				"               Killed by %s\n"
				"               on %s %d%s%s",
				the_score.how, wilderness ? "wilderness level" : "dungeon level", cdun, mdun > cdun ? format(" (max %d)", mdun) : "", extra_info);
		else if (!strcmp(the_score.how, "winner"))
			snprintf(out_val, sizeof(out_val),
				"               \377vRetired after a legendary career\n"
				"               on %s %d%s%s", wilderness ? "wilderness level" : "dungeon level", cdun, mdun > cdun ? format(" (max %d)", mdun) : "", extra_info);
		else if (!strcmp(the_score.how, "*winner*"))
			snprintf(out_val, sizeof(out_val),
				"               \377vRetired on the shores of Valinor\n"
				"               on %s %d%s%s", wilderness ? "wilderness level" : "dungeon level", cdun, mdun > cdun ? format(" (max %d)", mdun) : "", extra_info);
		else if (!strcmp(the_score.how, "iron champion"))
			snprintf(out_val, sizeof(out_val),
				"               \377sRetired iron champion\n"
				"               on %s %d%s%s", wilderness ? "wilderness level" : "dungeon level", cdun, mdun > cdun ? format(" (max %d)", mdun) : "", extra_info);
		else if (!strcmp(the_score.how, "iron emperor"))
			snprintf(out_val, sizeof(out_val),
				"               \377vRetired from the iron throne\n"
				"               on %s %d%s%s", wilderness ? "wilderness level" : "dungeon level", cdun, mdun > cdun ? format(" (max %d)", mdun) : "", extra_info);

		/* Hack -- some people die in the town */
		if (!cdun)
		{
			/* (can't be in Valinor while we're in town, can we) */
			if (strcmp(the_score.how, "winner"))
				snprintf(out_val, sizeof(out_val),
					"               Killed by %s\n"
					"               in town%s",
					the_score.how, mdun > cdun ? format(" (max %d)", mdun) : "");
			else
				snprintf(out_val, sizeof(out_val),
					"               \377vRetired after a legendary career\n"
					"               in town%s", mdun > cdun ? format(" (max %d)", mdun) : "");
		}

		/* Append a "maximum level" */
//		if (mdun > cdun) strcat(out_val, format(" (max %d)", mdun));

		/* Dump the info */
		fprintf(fff, "%s\n", out_val);

		/* And still another line of info */
		sprintf(out_val,
			"               (Date %s, Gold %s, Turn %s).",
			when, gold, aged);
		fprintf(fff, "%s\n", out_val);

#if 0 /* bad formatting in > 4.4.1.7, better without this */
		/* Print newline if this isn't the last one */
		if (j < i - 1)
#endif
			fprintf(fff, "\n");
	}

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(Ind, file_name, "High Scores", line, 0, 5);

	/* Remove the file */
	fd_kill(file_name);
}




/*
 * Enters a players name on a hi-score table, if "legal", and in any
 * case, displays some relevant portion of the high score list.
 *
 * Assumes "signals_ignore_tstp()" has been called.
 */
static errr top_twenty(int Ind) {
	player_type *p_ptr = Players[Ind];
	//int          j;

	high_score   the_score;

	time_t ct = time((time_t*)0);

	if (is_admin(p_ptr)) return 0;

	/* No score file */
	if (highscore_fd < 0) {
		s_printf("Score file unavailable.\n");
		return (0);
	}

	/* Interupted */
	if (!p_ptr->total_winner && streq(p_ptr->died_from, "Interrupting")) {
		msg_print(Ind, "Score not registered due to interruption.");
		/* display_scores_aux(0, 10, -1, NULL); */
		return (0);
	}

	/* Quitter */
	if (!p_ptr->total_winner && streq(p_ptr->died_from, "Quitting")) {
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
	sprintf(the_score.pts, "%10u", total_points(Ind));
	the_score.pts[10] = '\0';

	/* Save the current gold */
	sprintf(the_score.gold, "%10u", p_ptr->au);
	the_score.gold[10] = '\0';

	/* Save the current turn */
	sprintf(the_score.turns, "%10u", turn);
	the_score.turns[10] = '\0';

#ifdef HIGHSCORE_DATE_HACK
	/* Save the date in a hacked up form (9 chars) */
	sprintf(the_score.day, "%-.6s %-.2s", ctime(&ct) + 4, ctime(&ct) + 22);
#else
	/* Save the date in standard form (8 chars) */
//	strftime(the_score.day, 9, "%m/%d/%y", localtime(&ct));
	/* Save the date in real standard form (10 chars) */
	strftime(the_score.day, 11, "%Y/%m/%d", localtime(&ct));
#endif

	/* Save the player name (15 chars) */
	sprintf(the_score.who, "%-.15s", p_ptr->name);
	/* Save the player account name (15 chars) */
	sprintf(the_score.whose, "%-.15s", p_ptr->accountname);

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
	sprintf(the_score.how, "%-.49s", p_ptr->died_from_list);

	/* Save the modus (hellish, bat..) */
//	sprintf(the_score.mode, "%c", p_ptr->mode);
	the_score.mode[0] = p_ptr->mode;

	/* Lock (for writing) the highscore file, or fail */
	if (fd_lock(highscore_fd, F_WRLCK)) return (1);

	/* Add a new entry to the score list, see where it went */
	//j = highscore_add(&the_score);
	highscore_add(&the_score);

	/* Unlock the highscore file, or fail */
	if (fd_lock(highscore_fd, F_UNLCK)) return (1);


#if 0
	/* Hack -- Display the top fifteen scores */
	if (j < 10) display_scores_aux(0, 15, j, NULL);
	/* Display the scores surrounding the player */
	else {
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
static errr predict_score(int Ind, int line) {
	player_type *p_ptr = Players[Ind];
	int		j;
	high_score	the_score;
#ifndef NEW_HISCORE
	bool		move_up, move_down;
#else
	int		erased_slot;
#endif

	/* No score file */
	if (highscore_fd < 0) {
		s_printf("Score file unavailable.\n");
		return (0);
	}


	/* Save the version */
	sprintf(the_score.what, "%u.%u.%u",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

	/* Calculate and save the points */
	sprintf(the_score.pts, "%10u", total_points(Ind));

	/* Save the current gold */
	sprintf(the_score.gold, "%10u", p_ptr->au);

	/* Save the current turn */
	sprintf(the_score.turns, "%10u", turn);

	/* Hack -- no time needed */
	strcpy(the_score.day, "TODAY");

	/* Save the player name (15 chars) */
	sprintf(the_score.who, "%-.15s", p_ptr->name);
	/* Save the player name (15 chars) */
	sprintf(the_score.whose, "%-.15s", p_ptr->accountname);

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

	/* Modus.. */
	the_score.mode[0] = p_ptr->mode;

	/* See where the entry would be placed */
#ifndef NEW_HISCORE
	j = highscore_where(&the_score, &move_up, &move_down);
	/* Hack -- Display the top n scores */
	if (is_admin(p_ptr))
		display_scores_aux(Ind, line, -1, NULL);
	else if (j < (SCORES_SHOWN - 1))
		display_scores_aux(Ind, line, j, &the_score);
	/* Display some "useful" scores */
	else
		display_scores_aux(Ind, line, SCORES_SHOWN - 1, &the_score);  /* -1, NULL */
#else
	j = highscore_where(&the_score, &erased_slot);
	if (is_admin(p_ptr))
		display_scores_aux(Ind, line, -1, -1, NULL);
	else
		display_scores_aux(Ind, line, j, erased_slot, &the_score);
#endif


	/* Success */
	return (0);
}




/*
 * Change a player into a King!                 -RAK-
 */
void kingly(int Ind, int type) {
	player_type *p_ptr = Players[Ind];

#if 0	// No, this makes Delete_player fail!
	/* Hack -- retire in town */
	p_ptr->wpos.wx = 0;	// pfft, not 0 maybe
	p_ptr->wpos.wy = 0;
	p_ptr->wpos.wz = 0;
#endif	// 0

	/* Fake death */
	//(void)strcpy(p_ptr->died_from_list, "Ripe Old Age");
	switch (type) {
	case 1: strcpy(p_ptr->died_from_list, "winner"); break; //retirement
	case 2: strcpy(p_ptr->died_from_list, "*winner*"); break; //valinor retirement
	case 3: strcpy(p_ptr->died_from_list, "iron champion"); break; //made it through IDDC
	case 4: strcpy(p_ptr->died_from_list, "iron emperor"); break; //made it through IDDC and killed Morgoth
	}

	/* Restore the experience */
	p_ptr->exp = p_ptr->max_exp;

	/* Restore the level */
	p_ptr->lev = p_ptr->max_plv;

	/* Hack -- Player gets an XP bonus for beating the game */
	/* p_ptr->exp = p_ptr->max_exp += 10000000L; */
}


/*
 * Add a player to the high score list.
 */
void add_high_score(int Ind) {
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
void close_game(void) {
	int i;

	/* No suspending now */
	signals_ignore_tstp();

	for (i = 1; i <= NumPlayers; i++) {
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
		if (p_ptr->death) {
			/* Handle retirement */
		        /* Retirement in Valinor? - C. Blue :) */
	    		if (in_valinor(&p_ptr->wpos)) kingly(i, 2);
			else if (p_ptr->total_winner) kingly(i, 1);

			/* Save memories */
			if (!save_player(i)) msg_print(i, "death save failed!");

			/* Handle score, show Top scores */
			top_twenty(i);
		}

		/* Still alive */
		else {
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

	/* Save list of banned players */
	save_banlist();

	/* Save dynamic quest info */
	save_quests();

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
void display_scores(int Ind, int line) {
	char buf[1024];

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "scores.raw");

	/* Open the binary high score file, for reading */
	highscore_fd = fd_open(buf, O_RDONLY);

	/* Paranoia -- No score file */
	if (highscore_fd < 0) {
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
errr get_rnd_line(cptr file_name, int entry, char *output, int max_len) {
	FILE    *fp;
	char    buf[1024];
	int     line = 0, counter, test, numentries;
	int     line_num = 0;
	//bool    found = FALSE;


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_TEXT, file_name);

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Failed */
	if (!fp) return (-1);

	/* Find the entry of the monster */
	while (TRUE) {
		/* Get a line from the file */
		if (my_fgets(fp, buf, 1024, FALSE) == 0) {
			/* Count the lines */
			line_num++;

			/* Look for lines starting with 'N:' */
			if ((buf[0] == 'N') && (buf[1] == ':')) {
				/* Allow default lines */
				if (buf[2] == '*') {
					/* Default lines */
					//found = TRUE;
					break;
				}
				/* Get the monster number */
				else if (sscanf(&(buf[2]), "%d", &test) != EOF) {
					/* Is it the right monster? */
					if (test == entry) {
						//found = TRUE;
						break;
					}
				} else {
					my_fclose(fp);
					return (-1);
				}
			}
		} else {
			/* Reached end of file */
			my_fclose(fp);
			return (-1);
		}

	}

	/* Get the number of entries */
	while (TRUE) {
		/* Get the line */
		if (my_fgets(fp, buf, 1024, FALSE) == 0) {
			/* Count the lines */
			line_num++;

			/* Look for the number of entries */
			if (isdigit(buf[0])) {
				/* Get the number of entries */
				numentries = atoi(buf);
				break;
			}
		} else {
			/* Count the lines */
			line_num++;

			my_fclose(fp);
			return (-1);
		}
	}

	if (numentries > 0) {
		/* Grab an appropriate line number */
		line = rand_int(numentries);

		/* Get the random line */
		for (counter = 0; counter <= line; counter++) {
			/* Count the lines */
			line_num++;

			/* Try to read the line */
			if (my_fgets(fp, buf, 1024, FALSE) == 0) {
				/* Found the line */
				if (counter == line) break;
			} else {
				my_fclose(fp);
				return (-1);
			}
		}

		/* Copy the line */
		strncpy(output, buf, max_len);
	}

	/* Close the file */
	my_fclose(fp);

	/* Success */
	return (line);
}

/*
 * Read lines from a file to memory.
 * 
 * Adapted from get_rnd_line(). This function assumes similar format for files.
 */
errr read_lines_to_memory(cptr file_name, char ***lines_out, int *num_lines_out) {
	char    **lines = NULL;
	FILE    *fp = NULL;
	char    buf[1024];
	int     counter = 0, numentries = 0;
	int     line_num = 0;
	//bool    found = FALSE;


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_TEXT, file_name);

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Failed */
	if (!fp) return (-1);

	/* Find the entry of the monster */
	while (TRUE) {
		/* Get a line from the file */
		if (my_fgets(fp, buf, 1024, FALSE) == 0) {
			/* Count the lines */
			line_num++;

			/* Look for lines starting with 'N:' */
			if ((buf[0] == 'N') && (buf[1] == ':')) {
				break;
			}
		} else {
			/* Reached end of file */
			my_fclose(fp);
			return (-1);
		}

	}

	/* Get the number of entries */
	while (TRUE) {
		/* Get the line */
		if (my_fgets(fp, buf, 1024, FALSE) == 0) {
			/* Count the lines */
			line_num++;

			/* Look for the number of entries */
			if (isdigit(buf[0])) {
				/* Get the number of entries */
				numentries = atoi(buf);
				break;
			}
		} else {
			/* Count the lines */
			line_num++;

			my_fclose(fp);
			return (-1);
		}
	}
	
	/* Allocate array of pointers */
	C_MAKE(lines, numentries, char *);

	/* Read all entries */
	for (counter = 0; counter < numentries; counter++) {
		/* Count the lines */
		line_num++;

		/* Try to read the line */
		if (my_fgets(fp, buf, 1024, FALSE) == 0) {
			/* Store the line in memory */
			lines[counter] = strdup(buf);
		} else {
			my_fclose(fp);
			return (-1);
		}
	}

	/* Close the file */
	my_fclose(fp);

	/* Use the two parameters to return the array and its size */
	*lines_out = lines;
	*num_lines_out = numentries;

	/* Success */
	return (0);
}

/*
 * Get a random line from an array stored in memory.
 * 
 * Adapted from get_rnd_line().
 */
errr get_rnd_line_from_memory(char **lines, int numentries, char *output, int max_len) {
	int line = 0;

	if (numentries > 0) {
		/* Grab an appropriate line number */
		line = rand_int(numentries);

		/* Copy the line */
		strncpy(output, lines[line], max_len);
		
		/* Make sure the string is terminated */
		output[max_len - 1] = '\0';
	} else {
		/* Make output an empty string */
		output[0] = '\0';
	}

	/* Success */
	return (line);
}

/* Clear objects so that artifacts get saved.
 * This probably isn't neccecary, as all the objects on each of
 * these levels should have been cleared by now. However, paranoia
 * can't hurt all that much... -APD
 */
/* totally inefficient replacement - sorry. */
/* it also doesnt respect non existent world positions */
/* rewrite */
/* k rewritten.. but not so better?	- Jir - */
void wipeout_needless_objects() {
#if 0	// exit_game_panic version
	int i = 1;
	int j,k;
	struct worldpos wpos;
	struct wilderness_type *wild;

	for (i = 0; i < MAX_WILD_X; i++) {
		wpos.wx = i;
		for (j = 0; j < MAX_WILD_Y; j++) {
			wpos.wy = j;
			wild = &wild_info[j][i];

			wpos.wz = 0;
			if(getcave(&wpos) && !players_on_depth(&wpos)) wipe_o_list(&wpos);

			if (wild->flags&WILD_F_UP)
				for (k = 0;k < wild->tower->maxdepth; k++) {
					wpos.wz = k;
					if((getcave(&wpos)) && (!players_on_depth(&wpos))) wipe_o_list(&wpos);
				}

			if (wild->flags&WILD_F_DOWN)
				for (k = 0;k < wild->dungeon->maxdepth; k++) {
					wpos.wz = -k;
					if((getcave(&wpos)) && (!players_on_depth(&wpos))) wipe_o_list(&wpos);
				}
		}
	}
#else	// 0
	struct worldpos cwpos;
	struct dungeon_type *d_ptr;
	wilderness_type *w_ptr;
	int x,y,z;

	for (y = 0; y < MAX_WILD_Y; y++) {
		cwpos.wy = y;
		for (x = 0; x < MAX_WILD_X; x++) {
			cwpos.wx = x;
			cwpos.wz = 0;
			w_ptr = &wild_info[y][x];
//			if(getcave(&cwpos) && !players_on_depth(&cwpos)) wipe_o_list(&cwpos);
			if(w_ptr->flags & WILD_F_DOWN){
				d_ptr = w_ptr->dungeon;
				for (z = 1; z <= d_ptr->maxdepth; z++) {
					cwpos.wz = -z;
					if(d_ptr->level[z-1].ondepth && d_ptr->level[z-1].cave)
						wipe_o_list(&cwpos);
				}
			}
			if(w_ptr->flags & WILD_F_UP){
				d_ptr = w_ptr->tower;
				for (z = 1; z <= d_ptr->maxdepth; z++) {
					cwpos.wz = z;
					if(d_ptr->level[z-1].ondepth && d_ptr->level[z-1].cave)
						wipe_o_list(&cwpos);
				}
			}
		}
	}
#endif	// 0
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
void exit_game_panic(void) {
	int i = 1;

#ifndef WINDOWS
	int dumppid, dumpstatus;

	/* fork() a child process that will abort() - mikaelh */
	dumppid = fork();
	if (dumppid == 0) {
		/* child process */

#ifdef HANDLE_SIGNALS
		signal(SIGABRT, SIG_IGN);
#endif

		/* dump core */
		abort();
	} else if (dumppid != -1) {
		/* wait for the child */
		waitpid(dumppid, &dumpstatus, 0);
	}
#endif

	/* If nothing important has happened, just quit */
	if (!server_generated || server_saved) quit("panic");

	while (NumPlayers > (i - 1)) {
		player_type *p_ptr = Players[i];

		/* Don't dereference bad pointers */
		if (!p_ptr) {
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

		/* Remember depth in the log files */
		s_printf("Trying panic saving %s on %d %d %d..\n", p_ptr->name, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);

		/* Panic save, or get worried */
		if (!save_player(i)) {
			if (!Destroy_connection(p_ptr->conn, "panic save failed!")) {
				/* Something extremely bad is going on, try to recover anyway */
				i++;
			}
		}

		/* Successful panic save */
		if (!Destroy_connection(p_ptr->conn, "panic save succeeded!")) {
			/* Something very bad happened, skip to next player */
			i++;
		}
	}

//	wipeout_needless_objects();

	/* Save dynamic quest info */
	save_quests();

	/* Save list of banned players */
	save_banlist();

	if (!save_server_info()) quit("server panic info save failed!");

#if 0 /* abort() done above in a child process */
	/* make a core dump using abort() - mikaelh */
# ifdef HANDLE_SIGNALS
	signal(SIGABRT, SIG_IGN);
# endif
	abort();
#endif
	
	/* Successful panic save of server info */
	quit("server panic info save succeeded!");
}


/* Save all characters and server info with 'panic save' flag set,
   to prevent non-autorecalled players after a flush error restart - C. Blue */
void save_game_panic(void) {
	int i = 1;

	/* If nothing important has happened, just quit */
//	if (!server_generated || server_saved) return;

	while (NumPlayers > (i - 1)) {
		player_type *p_ptr = Players[i];

		/* Don't dereference bad pointers */
		if (!p_ptr) {
			/* Skip to next player */
			i++;

			continue;
		}

		/* Hack -- turn off some things */
		disturb(i, 1, 0);

		/* Mega-Hack -- Delay death */
//		if (p_ptr->chp < 0) p_ptr->death = FALSE;

		/* Hardcode panic save */
		panic_save = 1;

		/* Forbid suspend */
//		signals_ignore_tstp();

		/* Indicate panic save */
//		(void)strcpy(p_ptr->died_from, "(panic save)");

		/* Remember depth in the log files */
		s_printf("Trying panic saving %s on %d %d %d..\n", p_ptr->name, p_ptr->wpos.wx, p_ptr->wpos.wy, p_ptr->wpos.wz);

		/* Panic save */
		save_player(i);
	    	i++;
	}

//	wipeout_needless_objects();

	/* Save dynamic quest info */
	save_quests();

	/* Save list of banned players */
	save_banlist();

	save_server_info();

	/* No more panicking */
	panic_save = 0;
}


/*
 * Windows specific replacement for signal handling [grk]
 */
#ifdef WINDOWS
#ifndef HANDLE_SIGNALS

LPTOP_LEVEL_EXCEPTION_FILTER old_handler;

/* Callback to be called by Windows when our term closes, the user 
 * logs off, the system is shutdown, etc.
 */
BOOL ctrl_handler( DWORD fdwCtrlType ) {
	/* Save everything and quit the game */
	shutdown_server();

	return TRUE;
}

/* Global unhandled exception handler */
/* If the server crashes under Windows, this is where we end up */
LONG WINAPI myUnhandledExceptionFilter(
  struct _EXCEPTION_POINTERS* ExceptionInfo) {
	/* We don't report to the meta server in this case, the meta
	 * server will detect that we've gone anyway 
	 */

	/* Call the previous exception handler, which we are assuming
	 * is the MinGW exception handler which should have been implicitly
	 * setup when we loaded the exchndl.dll library.
	 */
	if(old_handler != NULL)
	{
	  old_handler(ExceptionInfo);
	}

	/* Save everything and quit the game */
	exit_game_panic();

	/* We don't expect to ever get here... but for what it's worth... */
	return(EXCEPTION_EXECUTE_HANDLER); 
		
}


void setup_exit_handler(void) {
	/* Trap CTRL+C, Logoff, Shutdown, etc */
	if( SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ctrl_handler, TRUE ) ) 
	{
		plog("Initialised exit save handler.");
	}else{
		plog("ERROR: Could not set panic save handler!");
	}
	/* Trap unhandled exceptions, i.e. server crashes */
	old_handler = SetUnhandledExceptionFilter( myUnhandledExceptionFilter );
}
#endif
#endif


#ifdef HANDLE_SIGNALS


/*
 * Handle signals -- suspend
 *
 * Actually suspend the game, and then resume cleanly
 *
 * This will probably inflict much anger upon the suspender, but it is still
 * allowed (for now) --KLJ--
 */
#ifdef SIGTSTP
static void handle_signal_suspend(int sig) {
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
#endif


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
static void handle_signal_simple(int sig) {
	/* Disable handler */
	(void)signal(sig, SIG_IGN);


	/* Nothing to save, just quit */
	if (!server_generated || server_saved) quit(NULL);


	/* Count the signals */
	signal_count++;


	/* Allow suicide (after 5) */
	if (signal_count >= 5) {
		/* Tell the metaserver that we've quit */
		Report_to_meta(META_DIE);

		/* Save everything and quit the game */
//              exit_game_panic();
		shutdown_server();
	}

	/* Give warning (after 4) */
	else if (signal_count >= 4) {
		s_printf("Warning: Next signal kills server!\n");
	}

	/* Restore handler */
	(void)signal(sig, handle_signal_simple);
}

#ifdef SIGPIPE
static void handle_signal_bpipe(int sig){
	(void)signal(sig, SIG_IGN);	/* This should not happen, but for the sake of convention... */
	s_printf("SIGPIPE received\n");
	(void)signal(sig, handle_signal_bpipe);
}
#endif

/*
 * Handle signal -- abort, kill, etc
 *
 * This one also calls exit_game_panic() --KLJ--
 */
static void handle_signal_abort(int sig) {
	/* Disable handler */
	(void)signal(sig, SIG_IGN);

	s_printf("Received signal %d.\n", sig);

	/* Nothing to save, just quit */
	if (!server_generated || server_saved) quit(NULL);

	/* Tell the metaserver that we're going down */
	Report_to_meta(META_DIE);

	/* Save everybody and quit */
	exit_game_panic();
}




/*
 * Ignore SIGTSTP signals (keyboard suspend)
 */
void signals_ignore_tstp(void) {
#ifdef SIGTSTP
	(void)signal(SIGTSTP, SIG_IGN);
#endif

}

/*
 * Handle SIGTSTP signals (keyboard suspend)
 */
void signals_handle_tstp(void) {
#ifdef SIGTSTP
	(void)signal(SIGTSTP, handle_signal_suspend);
#endif

}


/*
 * Prepare to handle the relevant signals
 */
void signals_init(void) {
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

#ifdef SIGPIPE
	(void)signal(SIGPIPE, handle_signal_bpipe);
#endif

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
void signals_ignore_tstp(void) { }

/*
 * Do nothing
 */
void signals_handle_tstp(void) { }

/*
 * Do nothing
 */
void signals_init(void) { }


#endif  /* HANDLE_SIGNALS */


#if 0

/*
 * New v* functions for accessing files in memory
 * Uses non-blocking fds to read the files
 *  - mikaelh
 */

/* Same as in nserver.c */
#define MAX_FD 1023

#define VFILE_INPUT_INSTALLED	1

struct vfile {
	char *name;
	char *data;
	size_t alloc;
	size_t len;
	off_t pos;
	int flags;
	int vflags;
	int (*callback)(int, int);
};

typedef struct vfile vfile;

struct vfile vfiles[MAX_FD];

void vfile_receive_input(int fd, int arg);
int vopen(const char *pathname, int flags, int (*callback)(int, int));
int vclose(int fd);
ssize_t vread(int fd, char *buf, size_t len);
off_t vseek(int fd, off_t offset, int whence);

void vfile_receive_input(int fd, int arg) {
	vfile *vf = &vfiles[fd];
	ssize_t read_len;

	if (vf->alloc - vf->len < 4096) {
		vf->alloc += 4096;
		vf->data = realloc(vf->data, vf->alloc);
	}	

	read_len = read(fd, vf->data + vf->len, vf->alloc - vf->len);
	if (read_len > 0) {
		vf->len += read_len;
	}
	else if (read_len == 0) { /* end of file */
		remove_input(fd);
		vf->vflags &= ~VFILE_INPUT_INSTALLED;
		(*vf->callback)(fd, 0);
	}
	else if (errno != EAGAIN) {
		/* fatal error, close the file and notify callback */
		vclose(fd);
		(*vf->callback)(fd, -1);
	}
}

int vopen(const char *pathname, int flags, int (*callback)(int, int)) {
	int fd;
	size_t len;
	vfile *vf;

	flags |= O_NONBLOCK;

	fd = open(pathname, flags);

	if (fd >= 0) {
		vf = &vfiles[fd];
		memset(vf, 0, sizeof(struct vfile));
		len = strlen(pathname);
		vf->name = malloc(len + 1);
		strcpy(vf->name, pathname);
		vf->flags = flags;
		vf->callback = callback;
		vf->alloc = 4096;
		vf->data = malloc(vf->alloc);

		install_input(vfile_receive_input, fd, 0);
		vf->vflags |= VFILE_INPUT_INSTALLED;
	}

	return fd;
}

int vclose(int fd) {
	vfile *vf = &vfiles[fd];
	int n;

	if (vf->vflags & VFILE_INPUT_INSTALLED) {
		remove_input(fd);
		vf->vflags &= ~VFILE_INPUT_INSTALLED;
	}

	n = close(fd);

	if (!n) {
		free(vf->name);
		free(vf->data);
		memset(vf, 0, sizeof(struct vfile));
	}

	return n;
}

ssize_t vread(int fd, char *buf, size_t len) {
	vfile *vf = &vfiles[fd];

	if (!vf->data) {
		return -1;
	}

	if (vf->pos + len > vf->len) {
		len = vf->len - vf->pos;
	}

	if (len < 0) return 0;

	memcpy(buf, &vf->data[vf->pos], len);
	vf->pos += len;

	return len;
}

off_t vseek(int fd, off_t offset, int whence) {
	vfile *vf = &vfiles[fd];

	if (whence == SEEK_SET) {
		vf->pos = offset;
	} else if (whence == SEEK_CUR) {
		vf->pos += offset;
	} else if (whence == SEEK_END) {
		vf->pos = vf->len + offset;
	} else {
		return -1;
	}

	return vf->pos;
}

#endif // 0

/* Erase the current highscore completely - C. Blue */
bool highscore_reset(int Ind) {
        char buf[1024];

        /* Build the filename */
        path_build(buf, 1024, ANGBAND_DIR_DATA, "scores.raw");

	/* bam (delete file, simply) */
        highscore_fd = fd_open(buf, O_TRUNC);
        (void)fd_close(highscore_fd);

        /* Forget the high score fd */
	highscore_fd = -1;
        return(TRUE);
}

/* remove one specific entry from the highscore - C. Blue */
bool highscore_remove(int Ind, int slot) {
	int	i;
	high_score	tmpscore;
        char buf[1024];

        /* Build the filename */
        path_build(buf, 1024, ANGBAND_DIR_DATA, "scores.raw");

        /* Open the binary high score file, for reading */
        highscore_fd = fd_open(buf, O_RDWR);
        /* Paranoia -- No score file */
        if (highscore_fd < 0) {
                if (Ind) msg_print(Ind, "Score file unavailable!");
                return(FALSE);
        }
        /* Lock (for writing) the highscore file, or fail */
        if (fd_lock(highscore_fd, F_WRLCK)) {
                if (Ind) msg_print(Ind, "Couldn't lock highscore file for writing!");
                return(FALSE);
        }

	for (i = slot; i < MAX_HISCORES; i++) {
		/* Read the following entry, if any */
		if (highscore_seek(i + 1)) break;
		if (highscore_read(&tmpscore)) break;
		/* Overwrite current entry with it */
		if (highscore_seek(i)) return(FALSE);
		if (highscore_write(&tmpscore)) return(FALSE);
	}

	/* zero final entry */
	WIPE(&tmpscore, sizeof(high_score));
	strcpy(tmpscore.who, "(nobody)"); /* hack: erase name instead of 0 */
	if (highscore_seek(i)) return(FALSE);
	if (highscore_write(&tmpscore)) return(FALSE);

        /* Unlock the highscore file, or fail */
        if (fd_lock(highscore_fd, F_UNLCK)) {
                if (Ind) msg_print(Ind, "Couldn't unlock highscore file from writing!");
                return(FALSE);
        }
        /* Shut the high score file */
        (void)fd_close(highscore_fd);

        /* Forget the high score fd */
	highscore_fd = -1;

	return(TRUE);
}

/* Hack: Update old high score file to new format - C. Blue */
bool highscore_file_convert(int Ind) {
	int	i, entries;
	high_score_old	oldscore[MAX_HISCORES];
	high_score	newscore[MAX_HISCORES];
        char buf[1024];

        /* Build the filename */
        path_build(buf, 1024, ANGBAND_DIR_DATA, "scores.raw");

        /* Open the binary high score file, for reading */
        highscore_fd = fd_open(buf, O_RDWR);
        /* Paranoia -- No score file */
        if (highscore_fd < 0) {
                if (Ind) msg_print(Ind, "Score file unavailable!");
                return(FALSE);
        }

	for (i = 0; i < MAX_HISCORES; i++) {
		/* Read old entries */
		if (highscore_seek_old(i)) break;
		if (highscore_read_old(&oldscore[i])) break;
	}
	entries = i;

        /* Shut the high score file */
        (void)fd_close(highscore_fd);

#if 0 /* old conversion done once, for example */
	/* convert entries */
	for (i = 0; i < entries; i++) {
		char mod[80];

		strcpy(newscore[i].what, oldscore[i].what);
		strcpy(newscore[i].pts, oldscore[i].pts);
		strcpy(newscore[i].gold, oldscore[i].gold);
		strcpy(newscore[i].turns, oldscore[i].turns);

		/* convert date format */
		mod[0] = '2';
		mod[1] = '0';
		mod[2] = oldscore[i].day[6];
		mod[3] = oldscore[i].day[7];
		mod[4] = '/';
		mod[5] = oldscore[i].day[0];
		mod[6] = oldscore[i].day[1];
		mod[7] = '/';
		mod[8] = oldscore[i].day[3];
		mod[9] = oldscore[i].day[4];
		mod[10] = '\0';

		strcpy(newscore[i].day, mod);

		strcpy(newscore[i].who, oldscore[i].who);
		strcpy(newscore[i].whose, oldscore[i].whose);

		strcpy(newscore[i].sex, oldscore[i].sex);
		strcpy(newscore[i].p_r, oldscore[i].p_r);
		strcpy(newscore[i].p_c, oldscore[i].p_c);

		strcpy(newscore[i].cur_lev, oldscore[i].cur_lev);
		strcpy(newscore[i].cur_dun, oldscore[i].cur_dun);
		strcpy(newscore[i].max_lev, oldscore[i].max_lev);
		strcpy(newscore[i].max_dun, oldscore[i].max_dun);

		strcpy(newscore[i].how, oldscore[i].how);
		strcpy(newscore[i].mode, oldscore[i].mode);
	}
#else /* new conversion: inserting RACE_KOBOLD */
	for (i = 0; i < entries; i++) {
		int r;
		strcpy(newscore[i].what, oldscore[i].what);
		strcpy(newscore[i].pts, oldscore[i].pts);
		strcpy(newscore[i].gold, oldscore[i].gold);
		strcpy(newscore[i].turns, oldscore[i].turns);

		strcpy(newscore[i].day, oldscore[i].day);

		strcpy(newscore[i].who, oldscore[i].who);
		strcpy(newscore[i].whose, oldscore[i].whose);

		strcpy(newscore[i].sex, oldscore[i].sex);

		r = atoi(oldscore[i].p_r);
		if (r >= RACE_KOBOLD) {
			r++;
			sprintf(newscore[i].p_r, "%2d", r);
		} else strcpy(newscore[i].p_r, oldscore[i].p_r);

		strcpy(newscore[i].p_c, oldscore[i].p_c);

		strcpy(newscore[i].cur_lev, oldscore[i].cur_lev);
		strcpy(newscore[i].cur_dun, oldscore[i].cur_dun);
		strcpy(newscore[i].max_lev, oldscore[i].max_lev);
		strcpy(newscore[i].max_dun, oldscore[i].max_dun);

		strcpy(newscore[i].how, oldscore[i].how);
		strcpy(newscore[i].mode, oldscore[i].mode);
	}
#endif
	/* bam (delete file, simply) */
        highscore_fd = fd_open(buf, O_TRUNC);
        (void)fd_close(highscore_fd);

        /* Open the binary high score file, for writing */
        highscore_fd = fd_open(buf, O_RDWR);
        /* Paranoia -- No score file */
        if (highscore_fd < 0) {
                if (Ind) msg_print(Ind, "Score file unavailable!");
                return(FALSE);
        }

        /* Lock (for writing) the highscore file, or fail */
        if (fd_lock(highscore_fd, F_WRLCK)) {
                if (Ind) msg_print(Ind, "Couldn't lock highscore file for writing!");
                return(FALSE);
        }

	for (i = 0; i < entries; i++) {
		/* Skip to end */
		if (highscore_seek(i)) return (-1);
		/* add new entry */
		if (highscore_write(&newscore[i])) return (-1);
	}

        /* Unlock the highscore file, or fail */
        if (fd_lock(highscore_fd, F_UNLCK)) {
                if (Ind) msg_print(Ind, "Couldn't unlock highscore file from writing!");
                return(FALSE);
        }
        /* Shut the high score file */
        (void)fd_close(highscore_fd);

        /* Forget the high score fd */
	highscore_fd = -1;

	return(TRUE);
}
