/* $Id$ */
/* File: init2.c */

/* Purpose: Initialization (part 2) -BEN- */

#define SERVER

#include "angband.h"


/*
 * This file is used to initialize various variables and arrays for the
 * Angband game.  Note the use of "fd_read()" and "fd_write()" to bypass
 * the common limitation of "read()" and "write()" to only 32767 bytes
 * at a time.
 *
 * Several of the arrays for Angband are built from "template" files in
 * the "lib/file" directory, from which quick-load binary "image" files
 * are constructed whenever they are not present in the "lib/data"
 * directory, or if those files become obsolete, if we are allowed.
 *
 * Warning -- the "ascii" file parsers use a minor hack to collect the
 * name and text information in a single pass.  Thus, the game will not
 * be able to load any template file with more than 20K of names or 60K
 * of text, even though technically, up to 64K should be legal.
 *
 * The "init1.c" file is used only to parse the ascii template files,
 * to create the binary image files.  If you include the binary image
 * files instead of the ascii template files, then you can undefine
 * "ALLOW_TEMPLATES", saving about 20K by removing "init1.c".  Note
 * that the binary image files are extremely system dependant.
 */



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
	string_free(ANGBAND_DIR_SCPT);
	string_free(ANGBAND_DIR_DATA);
	string_free(ANGBAND_DIR_GAME);
	string_free(ANGBAND_DIR_SAVE);
	string_free(ANGBAND_DIR_TEXT);
	string_free(ANGBAND_DIR_USER);


	/*** Prepare the "path" ***/

	/* Load in the tomenet.cfg file.  This is a file that holds many
	 * options thave have historically been #defined in config.h.
	 */

	/* Hack -- save the main directory */
	ANGBAND_DIR = string_make(path);

	/* Prepare to append to the Base Path */
	tail = path + strlen(path);


#ifdef VM


	/*** Use "flat" paths with VM/ESA ***/

	/* Use "blank" path names */
	ANGBAND_DIR_SCPT = string_make("");
	ANGBAND_DIR_DATA = string_make("");
	ANGBAND_DIR_GAME = string_make("");
	ANGBAND_DIR_SAVE = string_make("");
	ANGBAND_DIR_TEXT = string_make("");
	ANGBAND_DIR_USER = string_make("");


#else /* VM */


	/*** Build the sub-directory names ***/

	/* Build a path name */
	strcpy(tail, "scpt");
	ANGBAND_DIR_SCPT = string_make(path);

	/* Build a path name */
	strcpy(tail, "data");
	ANGBAND_DIR_DATA = string_make(path);

	/* Build a path name */
	strcpy(tail, "game");
	ANGBAND_DIR_GAME = string_make(path);

	/* Build a path name */
	strcpy(tail, "save");
	ANGBAND_DIR_SAVE = string_make(path);
	
	/* Build a path name */
	strcpy(tail, "text");
	ANGBAND_DIR_TEXT = string_make(path);

	/* Build a path name */
	strcpy(tail, "user");
	ANGBAND_DIR_USER = string_make(path);

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
 * Hack -- Explain a broken "lib" folder and quit (see below).
 *
 * XXX XXX XXX This function is "messy" because various things
 * may or may not be initialized, but the "plog()" and "quit()"
 * functions are "supposed" to work under any conditions.
 */
static void show_news_aux(cptr why)
{
	/* Why */
	plog(why);

	/* Explain */
	plog("The 'lib' directory is probably missing or broken.");

	/* More details */
	plog("Perhaps the archive was not extracted correctly.");

	/* Explain */
	plog("See the 'README' file for more information.");

	/* Quit with error */
	quit("Fatal Error.");
}


/*
 * Hack -- verify some files, and display the "news.txt" file
 *
 * This function is often called before "init_some_arrays()",
 * but after the "term.c" package has been initialized, so
 * be aware that many functions will not be "usable" yet.
 *
 * Note that this function attempts to verify the "news" file,
 * and the game aborts (cleanly) on failure.
 *
 * Note that this function attempts to verify (or create) the
 * "high score" file, and the game aborts (cleanly) on failure.
 *
 * Note that one of the most common "extraction" errors involves
 * failing to extract all sub-directories (even empty ones), such
 * as by failing to use the "-d" option of "pkunzip", or failing
 * to use the "save empty directories" option with "Compact Pro".
 * This error will often be caught by the "high score" creation
 * code below, since the "lib/apex" directory, being empty in the
 * standard distributions, is most likely to be "lost", making it
 * impossible to create the high score file.
 */
void show_news(void)
{
	int		fd = -1;

	int		mode = 0644;

	FILE        *fp;

	char	buf[1024];


	/*** Verify the "news" file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_TEXT, "news.txt");

	/* Attempt to open the file */
	fd = fd_open(buf, O_RDONLY);

	/* Failure */
	if (fd < 0)
	{
		char why[1024];

		/* Message */
		sprintf(why, "Cannot access the '%s' file!", buf);

		/* Crash and burn */
		show_news_aux(why);
	}

	/* Close it */
	(void)fd_close(fd);


	/*** Display the "news" file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_TEXT, "news.txt");

	/* Open the News file */
	fp = my_fopen(buf, "r");

	/* Dump */
	if (fp)
	{
		/* Dump the file to the screen */
		while (0 == my_fgets(fp, buf, 1024))
		{
			/* Display and advance */
			s_printf(buf);
			s_printf("\n");
		}

		/* Close */
		my_fclose(fp);
	}

	/*** Verify (or create) the "high score" file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "scores.raw");

	/* Attempt to open the high score file */
	fd = fd_open(buf, O_RDONLY);

	/* Failure */
	if (fd < 0)
	{
		/* File type is "DATA" */
		FILE_TYPE(FILE_TYPE_DATA);

		/* Create a new high score file */
		fd = fd_make(buf, mode);

		/* Failure */
		if (fd < 0)
		{
			char why[1024];

			/* Message */
			sprintf(why, "Cannot create the '%s' file!", buf);

			/* Crash and burn */
			show_news_aux(why);
		}
	}

	/* Close it */
	(void)fd_close(fd);
}

#ifdef ALLOW_TEMPLATES


/*
 * Hack -- help give useful error messages
 */
s16b error_idx;
s16b error_line;


/*
 * Hack -- help initialize the fake "name" and "text" arrays when
 * parsing an "ascii" template file.
 */
u32b fake_name_size;
u32b fake_text_size;


/*
 * Standard error message text
 */
static cptr err_str[8] =
{
	NULL,
	"parse error",
	"obsolete file",
	"missing record header",
	"non-sequential records",
	"invalid flag specification",
	"undefined directive",
	"out of memory"
};


#endif




/*
 * Initialize the "f_info" array
 *
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_f_info(void)
{
	errr err;

	FILE *fp;

	/* General buffer */
	char buf[1024];


	/*** Make the header ***/

	/* Allocate the "header" */
	MAKE(f_head, header);

	/* Save the "version" */
	f_head->v_major = VERSION_MAJOR;
	f_head->v_minor = VERSION_MINOR;
	f_head->v_patch = VERSION_PATCH;
	f_head->v_extra = 0;

	/* Save the "record" information */
	f_head->info_num = MAX_F_IDX;
	f_head->info_len = sizeof(feature_type);

	/* Save the size of "f_head" and "f_info" */
	f_head->head_size = sizeof(header);
	f_head->info_size = f_head->info_num * f_head->info_len;


	/*** Make the fake arrays ***/

	/* Fake the size of "f_name" and "f_text" */
	fake_name_size = 20 * 1024L;
	fake_text_size = 60 * 1024L;

	/* Allocate the "f_info" array */
	C_MAKE(f_info, f_head->info_num, feature_type);

	/* Hack -- make "fake" arrays */
	C_MAKE(f_name, fake_name_size, char);
	C_MAKE(f_text, fake_text_size, char);


	/*** Load the ascii template file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "f_info.txt");

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Parse it */
	if (!fp) quit("Cannot open 'f_info.txt' file.");

	/* Parse the file */
	err = init_f_info_txt(fp, buf);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
		s_printf("Error %d at line %d of 'f_info.txt'.", err, error_line);
		s_printf("Record %d contains a '%s' error.", error_idx, oops);
		s_printf("Parsing '%s'.", buf);

		/* Quit */
		quit("Error in 'f_info.txt' file.");
	}

	/* Success */
	return (0);
}




/*
 * Initialize the "k_info" array
 *
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_k_info(void)
{
	errr err;

	FILE *fp;

	/* General buffer */
	char buf[1024];

	/*** Make the header ***/

	/* Allocate the "header" */
	MAKE(k_head, header);

	/* Save the "version" */
	k_head->v_major = VERSION_MAJOR;
	k_head->v_minor = VERSION_MINOR;
	k_head->v_patch = VERSION_PATCH;
	k_head->v_extra = 0;

	/* Save the "record" information */
	k_head->info_num = MAX_K_IDX;
	k_head->info_len = sizeof(object_kind);

	/* Save the size of "k_head" and "k_info" */
	k_head->head_size = sizeof(header);
	k_head->info_size = k_head->info_num * k_head->info_len;

	/*** Make the farrays ***/

	/* Fake the size of "k_name" and "k_text" */
	fake_name_size = 20 * 1024L;
	fake_text_size = 60 * 1024L;

	/* Allocate the "k_info" array */
	C_MAKE(k_info, k_head->info_num, object_kind);

	/* Hack -- make "fake" arrays */
	C_MAKE(k_name, fake_name_size, char);
	C_MAKE(k_text, fake_text_size, char);


	/*** Load the ascii template file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "k_info.txt");

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Parse it */
	if (!fp) quit("Cannot open 'k_info.txt' file.");

	/* Parse the file */
	err = init_k_info_txt(fp, buf);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
		s_printf("Error %d at line %d of 'k_info.txt'.", err, error_line);
		s_printf("Record %d contains a '%s' error.", error_idx, oops);
		s_printf("Parsing '%s'.", buf);

		/* Quit */
		quit("Error in 'k_info.txt' file.");
	}

	/* Success */
	return (0);
}




/*
 * Initialize the "a_info" array
 *
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_a_info(void)
{
	errr err;

	FILE *fp;

	/* General buffer */
	char buf[1024];


	/*** Make the "header" ***/

	/* Allocate the "header" */
	MAKE(a_head, header);

	/* Save the "version" */
	a_head->v_major = VERSION_MAJOR;
	a_head->v_minor = VERSION_MINOR;
	a_head->v_patch = VERSION_PATCH;
	a_head->v_extra = 0;

	/* Save the "record" information */
	a_head->info_num = MAX_A_IDX;
	a_head->info_len = sizeof(artifact_type);

	/* Save the size of "a_head" and "a_info" */
	a_head->head_size = sizeof(header);
	a_head->info_size = a_head->info_num * a_head->info_len;

	/*** Make the fake arrays ***/

	/* Fake the size of "a_name" and "a_text" */
	fake_name_size = 20 * 1024L;
	fake_text_size = 60 * 1024L;

	/* Allocate the "a_info" array */
	C_MAKE(a_info, a_head->info_num, artifact_type);

	/* Hack -- make "fake" arrays */
	C_MAKE(a_name, fake_name_size, char);
	C_MAKE(a_text, fake_text_size, char);


	/*** Load the ascii template file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "a_info.txt");

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Parse it */
	if (!fp) quit("Cannot open 'a_info.txt' file.");

	/* Parse the file */
	err = init_a_info_txt(fp, buf);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
		s_printf("Error %d at line %d of 'a_info.txt'.", err, error_line);
		s_printf("Record %d contains a '%s' error.", error_idx, oops);
		s_printf("Parsing '%s'.", buf);

		/* Quit */
		quit("Error in 'a_info.txt' file.");
	}

	/* Success */
	return (0);
}



/*
 * Initialize the "e_info" array
 *
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_e_info(void)
{
	errr err;

	FILE *fp;

	/* General buffer */
	char buf[1024];


	/*** Make the "header" ***/

	/* Allocate the "header" */
	MAKE(e_head, header);

	/* Save the "version" */
	e_head->v_major = VERSION_MAJOR;
	e_head->v_minor = VERSION_MINOR;
	e_head->v_patch = VERSION_PATCH;
	e_head->v_extra = 0;

	/* Save the "record" information */
	e_head->info_num = MAX_E_IDX;
	e_head->info_len = sizeof(ego_item_type);

	/* Save the size of "e_head" and "e_info" */
	e_head->head_size = sizeof(header);
	e_head->info_size = e_head->info_num * e_head->info_len;


	/*** Make the fake arrays ***/

	/* Fake the size of "e_name" and "e_text" */
	fake_name_size = 20 * 1024L;
	fake_text_size = 60 * 1024L;

	/* Allocate the "e_info" array */
	C_MAKE(e_info, e_head->info_num, ego_item_type);

	/* Hack -- make "fake" arrays */
	C_MAKE(e_name, fake_name_size, char);
	C_MAKE(e_text, fake_text_size, char);


	/*** Load the ascii template file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "e_info.txt");

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Parse it */
	if (!fp) quit("Cannot open 'e_info.txt' file.");

	/* Parse the file */
	err = init_e_info_txt(fp, buf);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
		s_printf("Error %d at line %d of 'e_info.txt'.", err, error_line);
		s_printf("Record %d contains a '%s' error.", error_idx, oops);
		s_printf("Parsing '%s'.", buf);

		/* Quit */
		quit("Error in 'e_info.txt' file.");
	}

	/* Success */
	return (0);
}

/*
 * Initialize the "r_info" array
 *
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_r_info(void)
{
	errr err;

	FILE *fp;

	/* General buffer */
	char buf[1024];


	/*** Make the header ***/

	/* Allocate the "header" */
	MAKE(r_head, header);

	/* Save the "version" */
	r_head->v_major = VERSION_MAJOR;
	r_head->v_minor = VERSION_MINOR;
	r_head->v_patch = VERSION_PATCH;
	r_head->v_extra = 0;

	/* Save the "record" information */
	r_head->info_num = MAX_R_IDX;
	r_head->info_len = sizeof(monster_race);

	/* Save the size of "r_head" and "r_info" */
	r_head->head_size = sizeof(header);
	r_head->info_size = r_head->info_num * r_head->info_len;


	/*** Make the fake arrays ***/

	/* Assume the size of "r_name" and "r_text" */
	fake_name_size = 20 * 1024L;
	fake_text_size = 60 * 1024L;

	/* Allocate the "r_info" array */
	C_MAKE(r_info, r_head->info_num, monster_race);

	/* Hack -- make "fake" arrays */
	C_MAKE(r_name, fake_name_size, char);
	C_MAKE(r_text, fake_text_size, char);

	/*** Load the ascii template file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "r_info.txt");

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Parse it */
	if (!fp) quit("Cannot open 'r_info.txt' file.");

	/* Parse the file */
	err = init_r_info_txt(fp, buf);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
		s_printf("Error %d at line %d of 'r_info.txt'.", err, error_line);
		s_printf("Record %d contains a '%s' error.", error_idx, oops);
		s_printf("Parsing '%s'.", buf);

		/* Quit */
		quit("Error in 'r_info.txt' file.");
	}

	/* Success */
	return (0);
}


#ifdef RANDUNIS
/*
 * Initialize the "re_info" array
 *
 * Note that we let each entry have a unique "name" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_re_info(void)
{
	int mode = 0644;

	errr err = 0;

	FILE *fp;

	/* General buffer */
	char buf[1024];


	/*** Make the header ***/

	/* Allocate the "header" */
        MAKE(re_head, header);

	/* Save the "version" */
        re_head->v_major = VERSION_MAJOR;
        re_head->v_minor = VERSION_MINOR;
        re_head->v_patch = VERSION_PATCH;
        re_head->v_extra = 0;

	/* Save the "record" information */
        re_head->info_num = MAX_RE_IDX;
        re_head->info_len = sizeof(monster_ego);

        /* Save the size of "re_head" and "re_info" */
        re_head->head_size = sizeof(header);
        re_head->info_size = re_head->info_num * re_head->info_len;

#ifdef ALLOW_TEMPLATES

	/*** Make the fake arrays ***/

        /* Assume the size of "re_name" */
//	fake_name_size = FAKE_NAME_SIZE;
	fake_name_size = 20 * 1024L;

        /* Allocate the "re_info" array */
        C_MAKE(re_info, re_head->info_num, monster_ego);

	/* Hack -- make "fake" arrays */
        C_MAKE(re_name, fake_name_size, char);


	/*** Load the ascii template file ***/

	/* Build the filename */
        path_build(buf, 1024, ANGBAND_DIR_GAME, "re_info.txt");

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Parse it */
        if (!fp) quit("Cannot open 're_info.txt' file.");

	/* Parse the file */
        err = init_re_info_txt(fp, buf);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
                s_printf("Error %d at line %d of 're_info.txt'.", err, error_line);
		s_printf("Record %d contains a '%s' error.", error_idx, oops);
		s_printf("Parsing '%s'.", buf);
//		s_printf(NULL);

		/* Quit */
                quit("Error in 're_info.txt' file.");
	}

#endif	/* ALLOW_TEMPLATES */

	/* Success */
	return (0);
}
#endif	// RANDUNIS


/*
 * Initialize the "t_info" array
 *
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_t_info(void)
{
	int mode = 0644;

	errr err = 0;

	FILE *fp;

	/* General buffer */
	char buf[1024];


	/*** Make the header ***/

	/* Allocate the "header" */
	MAKE(t_head, header);

	/* Save the "version" */
	t_head->v_major = VERSION_MAJOR;
	t_head->v_minor = VERSION_MINOR;
	t_head->v_patch = VERSION_PATCH;
	t_head->v_extra = 0;

	/* Save the "record" information */
	t_head->info_num = MAX_T_IDX;
	t_head->info_len = sizeof(trap_kind);

	/* Save the size of "t_head" and "t_info" */
	t_head->head_size = sizeof(header);
	t_head->info_size = t_head->info_num * t_head->info_len;


#ifdef ALLOW_TEMPLATES

	/*** Make the fake arrays ***/

	/* Fake the size of "t_name" and "t_text" */
	fake_name_size = 20 * 1024L;
	fake_text_size = 60 * 1024L;
//	fake_name_size = FAKE_NAME_SIZE;
//	fake_text_size = FAKE_TEXT_SIZE;

	/* Allocate the "t_info" array */
	C_MAKE(t_info, t_head->info_num, trap_kind);

	/* Hack -- make "fake" arrays */
	C_MAKE(t_name, fake_name_size, char);
	C_MAKE(t_text, fake_text_size, char);


	/*** Load the ascii template file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "tr_info.txt");

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Parse it */
	if (!fp) quit("Cannot open 'tr_info.txt' file.");

	/* Parse the file */
	err = init_t_info_txt(fp, buf);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
		s_printf("Error %d at line %d of 'tr_info.txt'.", err, error_line);
		s_printf("Record %d contains a '%s' error.", error_idx, oops);
		s_printf("Parsing '%s'.", buf);

		/* Quit */
		quit("Error in 'tr_info.txt' file.");
	}

#endif	/* ALLOW_TEMPLATES */

	/* Success */
	return (0);
}



/*
 * Initialize the "v_info" array
 *
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_v_info(void)
{
	errr err;

	FILE *fp;

	/* General buffer */
	char buf[1024];


	/*** Make the header ***/

	/* Allocate the "header" */
	MAKE(v_head, header);

	/* Save the "version" */
	v_head->v_major = VERSION_MAJOR;
	v_head->v_minor = VERSION_MINOR;
	v_head->v_patch = VERSION_PATCH;
	v_head->v_extra = 0;

	/* Save the "record" information */
	v_head->info_num = MAX_V_IDX;
	v_head->info_len = sizeof(vault_type);

	/* Save the size of "v_head" and "v_info" */
	v_head->head_size = sizeof(header);
	v_head->info_size = v_head->info_num * v_head->info_len;

	/*** Make the fake arrays ***/

	/* Fake the size of "v_name" and "v_text" */
	fake_name_size = 20 * 1024L;
	fake_text_size = 170 * 1024L;
//	fake_text_size = 120 * 1024L;

	/* Allocate the "k_info" array */
	C_MAKE(v_info, v_head->info_num, vault_type);

	/* Hack -- make "fake" arrays */
	C_MAKE(v_name, fake_name_size, char);
	C_MAKE(v_text, fake_text_size, char);


	/*** Load the ascii template file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "v_info.txt");

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Parse it */
	if (!fp) quit("Cannot open 'v_info.txt' file.");

	/* Parse the file */
	err = init_v_info_txt(fp, buf);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
		s_printf("Error %d at line %d of 'v_info.txt'.", err, error_line);
		s_printf("Record %d contains a '%s' error.", error_idx, oops);
		s_printf("Parsing '%s'.", buf);

		/* Quit */
		quit("Error in 'v_info.txt' file.");
	}

	/* Success */
	return (0);
}




/*** Initialize others ***/


#if 0
/*
 * Hack -- Objects sold in the stores -- by tval/sval pair.
 */
static byte store_table[MAX_STORES-3][STORE_CHOICES][2] =
{
	{
		/* General Store */

		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_BISCUIT },
		{ TV_FOOD, SV_FOOD_JERKY },
		{ TV_FOOD, SV_FOOD_JERKY },

		{ TV_FOOD, SV_FOOD_PINT_OF_WINE },
		{ TV_FOOD, SV_FOOD_PINT_OF_ALE },
		{ TV_LITE, SV_LITE_TORCH },
		{ TV_LITE, SV_LITE_TORCH },
		{ TV_LITE, SV_LITE_TORCH },
		{ TV_LITE, SV_LITE_TORCH },
		{ TV_LITE, SV_LITE_LANTERN },
		{ TV_LITE, SV_LITE_LANTERN },

		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },
		{ TV_ARROW, SV_AMMO_NORMAL },
		{ TV_ARROW, SV_AMMO_NORMAL },

		{ TV_SHOT, SV_AMMO_NORMAL },
		{ TV_BOLT, SV_AMMO_NORMAL },
		{ TV_BOLT, SV_AMMO_NORMAL },
		{ TV_DIGGING, SV_SHOVEL },
		{ TV_DIGGING, SV_PICK },
		{ TV_CLOAK, SV_CLOAK },
		{ TV_CLOAK, SV_CLOAK },
		{ TV_CLOAK, SV_CLOAK }
	},

	{
		/* Armoury */

		{ TV_BOOTS, SV_PAIR_OF_SOFT_LEATHER_BOOTS },
		{ TV_BOOTS, SV_PAIR_OF_SOFT_LEATHER_BOOTS },
		{ TV_BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },
		{ TV_BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },
		{ TV_HELM, SV_HARD_LEATHER_CAP },
		{ TV_HELM, SV_HARD_LEATHER_CAP },
		{ TV_HELM, SV_METAL_CAP },
		{ TV_HELM, SV_IRON_HELM },

		{ TV_SOFT_ARMOR, SV_ROBE },
		{ TV_SOFT_ARMOR, SV_ROBE },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },
		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },
		{ TV_SOFT_ARMOR, SV_HARD_STUDDED_LEATHER },
		{ TV_SOFT_ARMOR, SV_HARD_STUDDED_LEATHER },

		{ TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL },
		{ TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL },
		{ TV_HARD_ARMOR, SV_METAL_SCALE_MAIL },
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL },
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL },
		{ TV_HARD_ARMOR, SV_AUGMENTED_CHAIN_MAIL },
		{ TV_HARD_ARMOR, SV_BAR_CHAIN_MAIL },
		{ TV_HARD_ARMOR, SV_DOUBLE_CHAIN_MAIL },

		{ TV_HARD_ARMOR, SV_METAL_BRIGANDINE_ARMOUR },
		{ TV_GLOVES, SV_SET_OF_LEATHER_GLOVES },
		{ TV_GLOVES, SV_SET_OF_LEATHER_GLOVES },
		{ TV_GLOVES, SV_SET_OF_GAUNTLETS },
		{ TV_SHIELD, SV_SMALL_LEATHER_SHIELD },
		{ TV_SHIELD, SV_SMALL_LEATHER_SHIELD },
		{ TV_SHIELD, SV_LARGE_LEATHER_SHIELD },
		{ TV_SHIELD, SV_SMALL_METAL_SHIELD }
	},

	{
		/* Weaponsmith */

		{ TV_SWORD, SV_DAGGER },
		{ TV_SWORD, SV_MAIN_GAUCHE },
		{ TV_SWORD, SV_RAPIER },
		{ TV_SWORD, SV_SMALL_SWORD },
		{ TV_SWORD, SV_SHORT_SWORD },
		{ TV_SWORD, SV_SABRE },
		{ TV_SWORD, SV_CUTLASS },
		{ TV_SWORD, SV_TULWAR },

		{ TV_SWORD, SV_BROAD_SWORD },
		{ TV_SWORD, SV_LONG_SWORD },
		{ TV_SWORD, SV_SCIMITAR },
		{ TV_SWORD, SV_KATANA },
		{ TV_SWORD, SV_BASTARD_SWORD },
		{ TV_POLEARM, SV_SPEAR },
		{ TV_POLEARM, SV_AWL_PIKE },
		{ TV_POLEARM, SV_TRIDENT },

		{ TV_POLEARM, SV_PIKE },
		{ TV_POLEARM, SV_BEAKED_AXE },
		{ TV_POLEARM, SV_BROAD_AXE },
		{ TV_POLEARM, SV_LANCE },
		{ TV_POLEARM, SV_BATTLE_AXE },
		{ TV_HAFTED, SV_WHIP },
		{ TV_BOW, SV_SLING },
		{ TV_BOW, SV_SHORT_BOW },

		{ TV_BOW, SV_LONG_BOW },
		{ TV_BOW, SV_LIGHT_XBOW },
		{ TV_SHOT, SV_AMMO_NORMAL },
		{ TV_SHOT, SV_AMMO_NORMAL },
		{ TV_ARROW, SV_AMMO_NORMAL },
		{ TV_ARROW, SV_AMMO_NORMAL },
		{ TV_BOLT, SV_AMMO_NORMAL },
		
		{ TV_FIGHT_BOOK, 0 },
	},

	{
		/* Temple */

		{ TV_HAFTED, SV_WHIP },
		{ TV_HAFTED, SV_QUARTERSTAFF },
		{ TV_HAFTED, SV_MACE },
		{ TV_HAFTED, SV_MACE },
		{ TV_HAFTED, SV_BALL_AND_CHAIN },
		{ TV_HAFTED, SV_WAR_HAMMER },
		{ TV_HAFTED, SV_LUCERN_HAMMER },
		{ TV_HAFTED, SV_MORNING_STAR },

		{ TV_HAFTED, SV_FLAIL },
	//	{ TV_HAFTED, SV_FLAIL },    for life.
		{ TV_HAFTED, SV_LEAD_FILLED_MACE },
		{ TV_SCROLL, SV_SCROLL_REMOVE_CURSE },
		{ TV_SCROLL, SV_SCROLL_BLESSING },
		{ TV_SCROLL, SV_SCROLL_HOLY_CHANT },
		{ TV_SCROLL, SV_SCROLL_LIFE	},
		{ TV_SCROLL, SV_SCROLL_LIFE	},
		{ TV_POTION, SV_POTION_BOLDNESS },
		{ TV_POTION, SV_POTION_HEROISM },

		{ TV_POTION, SV_POTION_CURE_LIGHT },
		{ TV_POTION, SV_POTION_CURE_SERIOUS },
		{ TV_POTION, SV_POTION_CURE_SERIOUS },
		{ TV_POTION, SV_POTION_CURE_CRITICAL },
		{ TV_POTION, SV_POTION_CURE_CRITICAL },
		{ TV_POTION, SV_POTION_RESTORE_EXP },
		{ TV_POTION, SV_POTION_RESTORE_EXP },
	//	{ TV_POTION, SV_POTION_RESTORE_EXP },

		{ TV_PRAYER_BOOK, 0 },
		{ TV_PRAYER_BOOK, 0 },
		{ TV_PRAYER_BOOK, 0 },
		{ TV_PRAYER_BOOK, 1 },
		{ TV_PRAYER_BOOK, 1 },
		{ TV_PRAYER_BOOK, 2 },
		{ TV_PRAYER_BOOK, 2 },
		{ TV_PRAYER_BOOK, 3 }
	},

	{
		/* Alchemy shop */

		{ TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_HIT },
		{ TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_DAM },
		{ TV_SCROLL, SV_SCROLL_ENCHANT_ARMOR },
		{ TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ TV_SCROLL, SV_SCROLL_LIGHT },

		{ TV_SCROLL, SV_SCROLL_PHASE_DOOR },
		{ TV_SCROLL, SV_SCROLL_PHASE_DOOR },
		{ TV_SCROLL, SV_SCROLL_PHASE_DOOR },
		{ TV_SCROLL, SV_SCROLL_MONSTER_CONFUSION },
		{ TV_SCROLL, SV_SCROLL_MAPPING },
		{ TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_HIT },
		{ TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_DAM },
		{ TV_SCROLL, SV_SCROLL_ENCHANT_ARMOR },

		{ TV_SCROLL, SV_SCROLL_DETECT_DOOR },
		{ TV_SCROLL, SV_SCROLL_DETECT_INVIS },
		{ TV_SCROLL, SV_SCROLL_RECHARGING },
		{ TV_SCROLL, SV_SCROLL_SATISFY_HUNGER },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },

		{ TV_POTION, SV_POTION_RESIST_HEAT },
		{ TV_POTION, SV_POTION_RESIST_COLD },
		{ TV_POTION, SV_POTION_RES_STR },
		{ TV_POTION, SV_POTION_RES_INT },
		{ TV_POTION, SV_POTION_RES_WIS },
		{ TV_POTION, SV_POTION_RES_DEX },
		{ TV_POTION, SV_POTION_RES_CON },
		{ TV_POTION, SV_POTION_RES_CHR }
	},

	{
		/* Magic-User store */

/* 		{ TV_RING, SV_RING_SEARCHING }, */
		{ TV_RING, SV_RING_FEATHER_FALL },
		{ TV_RING, SV_RING_PROTECTION },
		{ TV_AMULET, SV_AMULET_CHARISMA },
		{ TV_AMULET, SV_AMULET_SLOW_DIGEST },
		{ TV_AMULET, SV_AMULET_RESIST_ACID },
		{ TV_WAND, SV_WAND_SLOW_MONSTER },
		{ TV_WAND, SV_WAND_CONFUSE_MONSTER },

		{ TV_WAND, SV_WAND_SLEEP_MONSTER },
		{ TV_WAND, SV_WAND_MAGIC_MISSILE },
		{ TV_WAND, SV_WAND_STINKING_CLOUD },
/*		{ TV_WAND, SV_WAND_WONDER },   */
		{ TV_STAFF, SV_STAFF_LITE },
		{ TV_STAFF, SV_STAFF_MAPPING },
		{ TV_STAFF, SV_STAFF_DETECT_TRAP },
		{ TV_STAFF, SV_STAFF_DETECT_DOOR },

/*		{ TV_STAFF, SV_STAFF_DETECT_GOLD }, */
		{ TV_STAFF, SV_STAFF_DETECT_ITEM },
		{ TV_STAFF, SV_STAFF_DETECT_INVIS },
		{ TV_STAFF, SV_STAFF_DETECT_EVIL },
		{ TV_STAFF, SV_STAFF_TELEPORTATION },
		{ TV_STAFF, SV_STAFF_TELEPORTATION },
		{ TV_STAFF, SV_STAFF_IDENTIFY },
		{ TV_STAFF, SV_STAFF_IDENTIFY },

		{ TV_MAGIC_BOOK, 0 },
		{ TV_MAGIC_BOOK, 0 },
		{ TV_MAGIC_BOOK, 1 },
		{ TV_MAGIC_BOOK, 1 },
		{ TV_MAGIC_BOOK, 2 },
		{ TV_MAGIC_BOOK, 3 },
	
		{ TV_SORCERY_BOOK, 0 },
		{ TV_SORCERY_BOOK, 0 },
		{ TV_SORCERY_BOOK, 1 },
		{ TV_SORCERY_BOOK, 2 },
		{ TV_SORCERY_BOOK, 3 },
	}
};
#endif

static void prepare_distance()
{
	int d, y, x, count = 0;

	/* Start with adjacent locations, spread further */
	for (d = 0; d < PREPARE_RADIUS ; d++)
	{
		/* Check nearby locations */
		for (y = - d; y <= d; y++)
		{
			for (x = - d; x <= d; x++)
			{
				/* Check distance */
				if (distance(y, x, 0, 0) != d) continue;

				tdy[count] = y;
				tdx[count] = x;
				count++;
			}
		}
		tdi[d] = count;
	}
	/* Hack -- terminate */
	tdi[PREPARE_RADIUS] = count + 1;
	
#if DEBUG_LEVEL > 2
	s_printf("last count: %d\n", count);
#endif
}


/*
 * Initialize some other arrays
 */
static errr init_other(void)
{
	int i, k, n;


	/*** Prepare the "dungeon" information ***/

	/* Allocate and Wipe the object list */
	C_MAKE(o_list, MAX_O_IDX, object_type);

	/* Allocate and Wipe the monster list */
	C_MAKE(m_list, MAX_M_IDX, monster_type);

	/* Allocate and Wipe the traps list */
//	C_MAKE(t_list, MAX_TR_IDX, trap_type);

	/*** Init the wild_info array... for more information see wilderness.c ***/
	init_wild_info();

	/*** Prepare the various "bizarre" arrays ***/

	/* Quark variables */
	C_MAKE(quark__str, QUARK_MAX, cptr);

	/*** Prepare the Player inventory ***/

	/* Allocate it */
	/* This is done on player initialization --KLJ-- */
	/*C_MAKE(inventory, INVEN_TOTAL, object_type);*/


	/*** alloc for the houses ***/
	C_MAKE(houses, 1024, house_type);
	house_alloc=1024;

	/*** Pre-allocate the basic "auto-inscriptions" ***/

	/* The "basic" feelings */
	(void)quark_add("cursed");
	(void)quark_add("broken");
	(void)quark_add("average");
	(void)quark_add("good");

	/* The "extra" feelings */
	(void)quark_add("excellent");
	(void)quark_add("worthless");
	(void)quark_add("special");
	(void)quark_add("terrible");

	/* Some extra strings */
	(void)quark_add("uncursed");
	(void)quark_add("on sale");

	/* Turn on color */
	use_color = TRUE;

	/*** Pre-allocate space for the "format()" buffer ***/

	/* Hack -- Just call the "format()" function */
	(void)format("%s (%s).", "Ben Harrison", MAINTAINER);

	prepare_distance();

	/* Success */
	return (0);
}


void init_swearing(){
	int i=0;
	FILE *fp;
	fp=fopen("swearing.txt", "r");
	if(fp==(FILE*)NULL) return;
	do{
		fscanf(fp, "%s%d\n", swear[i].word, &swear[i].level);
		//printf("%d %s %d\n", i, swear[i].word, swear[i].level);
		i++;
	}while(!feof(fp));
	swear[i].word[0]='\0';
	fclose(fp);
}

/*
 * Initialize some other arrays
 */
static errr init_alloc(void)
{
	int i, j;

	object_kind *k_ptr;

	monster_race *r_ptr;

	alloc_entry *table;

	s16b num[256];

	s16b aux[256];

	/*** Analyze object allocation info ***/

	/* Clear the "aux" array */
	C_WIPE(&aux, 256, s16b);

	/* Clear the "num" array */
	C_WIPE(&num, 256, s16b);

	/* Size of "alloc_kind_table" */
	alloc_kind_size = 0;

	/* Scan the objects */
	for (i = 1; i < MAX_K_IDX; i++)
	{
		k_ptr = &k_info[i];

		/* Scan allocation pairs */
		for (j = 0; j < 4; j++)
		{
			/* Count the "legal" entries */
			if (k_ptr->chance[j])
			{
				/* Count the entries */
				alloc_kind_size++;

				/* Group by level */
				num[k_ptr->locale[j]]++;
			}
		}
	}

	/* Collect the level indexes */
	for (i = 1; i < 256; i++)
	{
		/* Group by level */
		num[i] += num[i-1];
	}

	/* Paranoia */
	if (!num[0]) quit("No town objects!");


	/*** Initialize object allocation info ***/

	/* Allocate the alloc_kind_table */
	C_MAKE(alloc_kind_table, alloc_kind_size, alloc_entry);

	/* Access the table entry */
	table = alloc_kind_table;

	/* Scan the objects */
	for (i = 1; i < MAX_K_IDX; i++)
	{
		k_ptr = &k_info[i];

		/* Scan allocation pairs */
		for (j = 0; j < 4; j++)
		{
			/* Count the "legal" entries */
			if (k_ptr->chance[j])
			{
				int p, x, y, z;

				/* Extract the base level */
				x = k_ptr->locale[j];

				/* Extract the base probability */
				p = (100 / k_ptr->chance[j]);

				/* Skip entries preceding our locale */
				y = (x > 0) ? num[x-1] : 0;

				/* Skip previous entries at this locale */
				z = y + aux[x];

				/* Load the entry */
				table[z].index = i;
				table[z].level = x;
				table[z].prob1 = p;
				table[z].prob2 = p;
				table[z].prob3 = p;

				/* Another entry complete for this locale */
				aux[x]++;
			}
		}
	}


	/*** Analyze monster allocation info ***/

	/* Clear the "aux" array */
	C_WIPE(&aux, 256, s16b);

	/* Clear the "num" array */
	C_WIPE(&num, 256, s16b);

	/* Size of "alloc_race_table" */
	alloc_race_size = 0;

	/* Scan the monsters (not the ghost) */
	for (i = 1; i < MAX_R_IDX - 1; i++)
	{
		/* Get the i'th race */
		r_ptr = &r_info[i];

		/* Legal monsters */
		if (r_ptr->rarity)
		{
			/* Count the entries */
			alloc_race_size++;

			/* Group by level */
			num[r_ptr->level]++;
		}
	}

	/* Collect the level indexes */
	for (i = 1; i < 256; i++)
	{
		/* Group by level */
		num[i] += num[i-1];
	}

	/* Paranoia */
	if (!num[0]) quit("No town monsters!");


	/*** Initialize monster allocation info ***/

	/* Allocate the alloc_race_table */
	C_MAKE(alloc_race_table, alloc_race_size, alloc_entry);

	/* Access the table entry */
	table = alloc_race_table;

	/* Scan the monsters (not the ghost) */
	for (i = 1; i < MAX_R_IDX - 1; i++)
	{
		/* Get the i'th race */
		r_ptr = &r_info[i];

		/* Count valid pairs */
		if (r_ptr->rarity)
		{
			int p, x, y, z;

			/* Extract the base level */
			x = r_ptr->level;

			/* Extract the base probability */
			p = (100 / r_ptr->rarity);

			/* Skip entries preceding our locale */
			y = (x > 0) ? num[x-1] : 0;

			/* Skip previous entries at this locale */
			z = y + aux[x];

			/* Load the entry */
			table[z].index = i;
			table[z].level = x;
			table[z].prob1 = p;
			table[z].prob2 = p;
			table[z].prob3 = p;

			/* Another entry complete for this locale */
			aux[x]++;
		}
	}


	/* Success */
	return (0);
}

bool str_to_boolean(char * str)
{
	/* false by default */
	return !(strcasecmp(str, "true"));
}

/* Try to set a server option.  This is handled very sloppily right now,
 * since server options are manually mapped to global variables.  Maybe
 * the handeling of this will be unified in the future with some sort of 
 * options structure.
 */
void set_server_option(char * option, char * value)
{
	/* Due to the lame way that C handles strings, we can't use a switch statement */
	if (!strcmp(option,"REPORT_TO_METASERVER"))
	{
		cfg.report_to_meta = str_to_boolean(value);
	}
	else if (!strcmp(option,"META_ADDRESS"))
	{
		cfg.meta_address = strdup(value);
	}
	else if (!strcmp(option,"BIND_NAME"))
	{
		cfg.bind_name = strdup(value);
	}
	else if (!strcmp(option,"CONSOLE_PASSWORD"))
	{
		cfg.console_password = strdup(value);
	}
	else if (!strcmp(option,"ADMIN_WIZARD_NAME"))
	{
		cfg.admin_wizard = strdup(value);
	}
	else if (!strcmp(option,"DUNGEON_MASTER_NAME"))
	{
		cfg.dungeon_master = strdup(value);
	}
	else if (!strcmp(option,"SECRET_DUNGEON_MASTER"))
	{
		cfg.secret_dungeon_master = str_to_boolean(value);
	}
	else if (!strcmp(option,"FPS"))
	{
		cfg.fps = atoi(value);
		/* Hack -- reinstall the timer handler to match the new FPS */
		install_timer_tick(dungeon, cfg.fps);
	}
	else if (!strcmp(option,"MAGE_HITPOINT_BONUS"))
	{
		cfg.mage_hp_bonus = str_to_boolean(value);
	}
	else if (!strcmp(option,"NEWBIES_CANNOT_DROP"))
	{
		cfg.newbies_cannot_drop = atoi(value);
	}
	else if (!strcmp(option,"PRESERVE_DEATH_LEVEL"))
	{
		cfg.preserve_death_level = atoi(value);
	}
	else if (!strcmp(option,"NO_GHOST"))
	{
		cfg.no_ghost = str_to_boolean(value);
	}
	else if (!strcmp(option,"DOOR_BUMP_OPEN"))
	{
		cfg.door_bump_open = str_to_boolean(value);
	}
	else if (!strcmp(option,"BASE_UNIQUE_RESPAWN_TIME"))
	{
		cfg.unique_respawn_time = atoi(value);
	}
	else if (!strcmp(option,"MAX_UNIQUE_RESPAWN_TIME"))
	{
		cfg.unique_max_respawn_time = atoi(value);
	}
	else if (!strcmp(option,"LEVEL_UNSTATIC_CHANCE"))
	{
		cfg.level_unstatic_chance = atoi(value);
	}
	else if (!strcmp(option,"RETIRE_TIMER"))
	{
		cfg.retire_timer = atoi(value);
	}
	else if (!strcmp(option,"MAXIMIZE"))
	{
		cfg.maximize = str_to_boolean(value);
	}
	else if (!strcmp(option,"GAME_PORT"))
	{
		cfg.game_port = atoi(value);
	}
	else if (!strcmp(option,"MIN_UNSTATIC_LEVEL"))
	{
		cfg.min_unstatic_level = atoi(value);
	}
	else if (!strcmp(option,"SPELL_INTERFERE"))
	{
		cfg.spell_interfere = atoi(value);
	}
	else if (!strcmp(option,"CONSOLE_PORT"))
	{
		cfg.console_port = atoi(value);
	}
	else if (!strcmp(option,"ANTI_ARTS_HORDE"))
	{
		cfg.anti_arts_horde = str_to_boolean(value);
	}
	else if (!strcmp(option,"SPELL_STACK_LIMIT"))
	{
		cfg.spell_stack_limit = atoi(value);
	}
	else if (!strcmp(option,"KINGS_ETIQUETTE"))
	{
		cfg.kings_etiquette = str_to_boolean(value);
	}
	else if (!strcmp(option,"ZANG_MONSTERS"))
	{
		cfg.zang_monsters = atoi(value);
	}
	else if (!strcmp(option,"PERN_MONSTERS"))
	{
		cfg.pern_monsters = atoi(value);
	}
	else if (!strcmp(option,"CTH_MONSTERS"))
	{
		cfg.cth_monsters = atoi(value);
	}
	else if (!strcmp(option,"JOKE_MONSTERS"))
	{
		cfg.joke_monsters = atoi(value);
	}
	else if (!strcmp(option,"VANILLA_MONSTERS"))
	{
		cfg.vanilla_monsters = atoi(value);
	}
	else if (!strcmp(option,"RUNNING_SPEED"))
	{
		cfg.running_speed = atoi(value);
	}
	else if (!strcmp(option,"ANTI_SCUM"))
	{
		cfg.anti_scum = atoi(value);
	}
	else if (!strcmp(option,"DUN_UNUSUAL"))
	{
		cfg.dun_unusual = atoi(value);
	}
	else if (!strcmp(option, "TOWN_X"))
	{
		cfg.town_x = atoi(value);
	}
	else if (!strcmp(option, "TOWN_Y"))
	{
		cfg.town_y = atoi(value);
	}
	else if (!strcmp(option, "TOWN_BASE"))
	{
		cfg.town_base = atoi(value);
	}
	else if (!strcmp(option, "DUNGEON_MAX"))
	{
		cfg.dun_max = atoi(value);
	}
	else if (!strcmp(option, "DUNGEON_BASE"))
	{
		cfg.dun_base = atoi(value);
	}
	else if (!strcmp(option, "STORE_TURNS"))
	{
		cfg.store_turns = atoi(value);
	}
	else if (!strcmp(option,"PUBLIC_RFE"))
	{
		cfg.public_rfe = str_to_boolean(value);
	}
	else if (!strcmp(option,"AUTO_PURGE"))
	{
		cfg.auto_purge = str_to_boolean(value);
	}
	else if (!strcmp(option, "RESTING_RATE"))
	{
		cfg.resting_rate = atoi(value);
	}
	else if (!strcmp(option, "PARTY_XP_BOOST"))
	{
		cfg.party_xp_boost = atoi(value);
	}
	else printf("Error : unrecognized tomenet.cfg option %s\n", option);
}


/* Parse the loaded tomenet.cfg file, and if a valid expression was found
 * try to set it using set_server_option.
 *
 * Note that this function uses strsep. I don't think this is an ANSI C function.
 * If you have any problems compiling this, please let me know and I will change this.
 * -Alex 
 *
 * Seemingly it caused trouble in win32;
 * Yakina reverted this to strtok.
 */
void load_server_cfg_aux(FILE * cfg)
{
	char line[256];
#if 0
	char * lineofs;
#else
	bool first_token;
#endif	// 0

	char * newword;
	char * option;
	char * value;

	/* Read in lines until we hit EOF */
	while (fgets(line, 256, cfg) != NULL)
	{
		// Chomp off the end of line character
		line[strlen(line)-1] = '\0';

		/* Parse the line that has been read in */
		// If the line begins with a # or is empty, ignore it
		if ((line[0] == '#') || (line[0] == '\0')) continue;

		// Reset option and value
		option = NULL;
		value = NULL;
		
		// Split the line up into words
		// strsep is a really cool function... its neat, we don't have
		// to dynamically allocate any memory because we apply our null
		// terminations directly to line.
#if 0
		lineofs = line;
		while ((newword = strsep(&lineofs, " ")))
#else
		first_token = 1;
		while ((newword = strtok(first_token ? line : NULL, " ")))
#endif	// 0
		{
			first_token = 0;

			/* Set the option or value */
			if (!option) option = newword;
			else if ((!value) && (newword[0] != '=')) 
			{
				value = newword;
				/* Hack -- ignore "" around values */
				if (value[0] == '"') value++;
				if (value[strlen(value)-1] == '"') value[strlen(value)-1] = '\0';
			}

			// If we have a completed option and value, then try to set it
			if (option && value)
			{
				set_server_option(option, value);
				break;
			}
		}
	}
}

/* Load in the tomenet.cfg file.  This is a file that holds many
 * options thave have historically been #defined in config.h.
 */

bool load_server_cfg(void)
{
	FILE * cfg;
	
	/* Attempt to open the file */
//	cfg = fopen("tomenet.cfg", "r");
	cfg = fopen(MANGBAND_CFG, "r");

	/* Failure */
	if (cfg == (FILE*)NULL)
	{
//		printf("Error : cannot open file tomenet.cfg\n");
		printf("Error : cannot open file %s\n", MANGBAND_CFG);
		return (FALSE);
	}

	/* Actually parse the file */
	load_server_cfg_aux(cfg);

	/* Close it */
	fclose(cfg);

	return (TRUE);
}


/*
 * Initialize various Angband variables and arrays.
 *
 * This initialization involves the parsing of special files
 * in the "lib/data" and sometimes the "lib/edit" directories.
 *
 * Note that the "template" files are initialized first, since they
 * often contain errors.  This means that macros and message recall
 * and things like that are not available until after they are done.
 */
void init_some_arrays(void)
{
        /* Init lua */
	s_printf("[Initializing lua... (scripts)]\n");
        init_lua();

	/* Initialize feature info */
	s_printf("[Initializing arrays... (features)]\n");
	if (init_f_info()) quit("Cannot initialize features");

	/* Initialize object info */
	s_printf("[Initializing arrays... (objects)]\n");
	if (init_k_info()) quit("Cannot initialize objects");

	/* Initialize artifact info */
	s_printf("[Initializing arrays... (artifacts)]\n");
	if (init_a_info()) quit("Cannot initialize artifacts");

	/* Initialize ego-item info */
	s_printf("[Initializing arrays... (ego-items)]\n");
	if (init_e_info()) quit("Cannot initialize ego-items");

	/* Initialize monster info */
	s_printf("[Initializing arrays... (monsters)]\n");
	if (init_r_info()) quit("Cannot initialize monsters");

#ifdef RANDUNIS
	/* Initialize ego monster info */
	s_printf("[Initializing arrays... (ego-monsters)]\n");
	if (init_re_info()) quit("Cannot initialize ego monsters");
#endif	// RANDUNIS

	/* Initialize feature info */
	s_printf("[Initializing arrays... (vaults)]\n");
	if (init_v_info()) quit("Cannot initialize vaults");

	/* Initialize trap info */
	s_printf("[Initializing arrays... (traps)]\n");
	if (init_t_info()) quit("Cannot initialize traps");

	/* Initialize some other arrays */
	s_printf("[Initializing arrays... (other)]\n");
	if (init_other()) quit("Cannot initialize other stuff");

	/* Initialize some other arrays */
	s_printf("[Initializing arrays... (alloc)]\n");
	if (init_alloc()) quit("Cannot initialize alloc stuff");

	init_swearing();

	/* Hack -- all done */
	s_printf("[Initializing arrays... done]\n");
}


