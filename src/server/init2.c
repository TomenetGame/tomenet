/* $Id$ */
/* File: init2.c */

/* Purpose: Initialization (part 2) -BEN- */

#define SERVER

//#define CHECK_MODIFICATION_TIME
#define CHECK_MODIFICATION_ALWAYS

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
		while (0 == my_fgets(fp, buf, 1024, FALSE))
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
 * Initialize the "s_info" array
 *
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_s_info(void)
{
#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	int fd;

	/* int i; */

	int mode = 0644;
#endif	// USE_RAW_FILES

	errr err = 0;

	FILE *fp;

	/* General buffer */
	char buf[1024];


	/*** Make the "header" ***/

	/* Allocate the "header" */
	MAKE(s_head, header);

	/* Save the "version" */
	s_head->v_major = VERSION_MAJOR;
	s_head->v_minor = VERSION_MINOR;
	s_head->v_patch = VERSION_PATCH;
	s_head->v_extra = 0;

	/* Save the "record" information */
//	s_head->info_num = max_s_idx;
	s_head->info_num = MAX_SKILLS;
	s_head->info_len = sizeof(skill_type);

	/* Save the size of "s_head" and "s_info" */
	s_head->head_size = sizeof(header);
	s_head->info_size = s_head->info_num * s_head->info_len;


#ifdef ALLOW_TEMPLATES
#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */

	/*** Load the binary image file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "s_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Attempt to open the "raw" file */
	fd = fd_open(buf, O_RDONLY);

	/* Drop permission */
	safe_setuid_drop();

	/* Process existing "raw" file */
	if (fd >= 0)
	{
#ifdef CHECK_MODIFICATION_TIME
		err = check_modification_date(fd, "s_info.txt");
#endif /* CHECK_MODIFICATION_TIME */
#ifdef CHECK_MODIFICATION_ALWAYS
		err = 0;
#endif
		/* Attempt to parse the "raw" file */
		if (!err)
			err = init_s_info_raw(fd);

		/* Close it */
		(void)fd_close(fd);

		/* Success */
		if (!err) return (0);

		/* Information */
		msg_print("Ignoring obsolete/defective 's_info.raw' file.");
		msg_print(NULL);
	}
#endif	// USE_RAW_FILES

	/*** Make the fake arrays ***/

	/* Fake the size of "a_name" and "a_text" */
#if 0
	fake_name_size = FAKE_NAME_SIZE;
	fake_text_size = FAKE_TEXT_SIZE;
#else	// 0
	fake_name_size = 20 * 1024L;
	fake_text_size = 60 * 1024L;
#endif	// 0

	/* Allocate the "s_info" array */
	C_MAKE(s_info, s_head->info_num, skill_type);

	/* Hack -- make "fake" arrays */
	C_MAKE(s_name, fake_name_size, char);
	C_MAKE(s_text, fake_text_size, char);

	/*** Load the ascii template file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "s_info.txt");

	/* Grab permission */
//	safe_setuid_grab();

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Drop permission */
//	safe_setuid_drop();

	/* Parse it */
	if (!fp) quit("Cannot open 's_info.txt' file.");

	/* Parse the file */
	err = init_s_info_txt(fp, buf);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
		s_printf("Error %d at line %d of 's_info.txt'.", err, error_line);
		s_printf("Record %d contains a '%s' error.", error_idx, oops);
		s_printf("Parsing '%s'.", buf);

		/* Quit */
		quit("Error in 's_info.txt' file.");
	}

#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	/*** Dump the binary image file ***/

	/* File type is "DATA" */
	FILE_TYPE(FILE_TYPE_DATA);

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "s_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Kill the old file */
	(void)fd_kill(buf);

	/* Attempt to create the raw file */
	fd = fd_make(buf, mode);

	/* Drop permission */
	safe_setuid_drop();

	/* Dump to the file */
	if (fd >= 0)
	{
		/* Dump it */
		fd_write(fd, (char*)(s_head), s_head->head_size);

		/* Dump the "s_info" array */
		fd_write(fd, (char*)(s_info), s_head->info_size);

		/* Dump the "s_name" array */
		fd_write(fd, (char*)(s_name), s_head->name_size);

		/* Dump the "s_text" array */
		fd_write(fd, (char*)(s_text), s_head->text_size);

		/* Close */
		(void)fd_close(fd);
	}


	/*** Kill the fake arrays ***/

	/* Free the "s_info" array */
	C_KILL(s_info, s_head->info_num, skill_type);

	/* Hack -- Free the "fake" arrays */
	C_KILL(s_name, fake_name_size, char);
	C_KILL(s_text, fake_text_size, char);

	/* Forget the array sizes */
	fake_name_size = 0;
	fake_text_size = 0;

#endif	// USE_RAW_FILES
#endif	/* ALLOW_TEMPLATES */


#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	/*** Load the binary image file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "s_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Attempt to open the "raw" file */
	fd = fd_open(buf, O_RDONLY);

	/* Drop permission */
	safe_setuid_drop();

	/* Process existing "raw" file */
	if (fd < 0) quit("Cannot open 's_info.raw' file.");

	/* Attempt to parse the "raw" file */
	err = init_s_info_raw(fd);

	/* Close it */
	(void)fd_close(fd);

	/* Error */
	if (err) quit("Cannot parse 's_info.raw' file.");
#endif	// USE_RAW_FILES

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
	//int mode = 0644;

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
 * Initialize the "d_info" array
 *
 * Note that we let each entry have a unique "name" and "short name" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_d_info(void)
{
#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	int fd;

	int mode = 0644;
#endif	// USE_RAW_FILES

	errr err = 0;

	FILE *fp;

	/* General buffer */
	char buf[1024];


	/*** Make the header ***/

	/* Allocate the "header" */
	MAKE(d_head, header);

	/* Save the "version" */
	d_head->v_major = VERSION_MAJOR;
	d_head->v_minor = VERSION_MINOR;
	d_head->v_patch = VERSION_PATCH;
	d_head->v_extra = 0;

	/* Save the "record" information */
	d_head->info_num = MAX_D_IDX;
	d_head->info_len = sizeof(dungeon_info_type);

	/* Save the size of "d_head" and "d_info" */
	d_head->head_size = sizeof(header);
	d_head->info_size = d_head->info_num * d_head->info_len;


#ifdef ALLOW_TEMPLATES
#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */

	/*** Load the binary image file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "d_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Attempt to open the "raw" file */
	fd = fd_open(buf, O_RDONLY);

	/* Drop permission */
	safe_setuid_drop();

	/* Process existing "raw" file */
	if (fd >= 0)
	{
#ifdef CHECK_MODIFICATION_TIME
//#if 1
		err = check_modification_date(fd, "d_info.txt");
#endif /* CHECK_MODIFICATION_TIME */
#ifdef CHECK_MODIFICATION_ALWAYS
	        err = 0;
#endif

		/* Attempt to parse the "raw" file */
		if (!err)
			err = init_d_info_raw(fd);

		/* Close it */
		(void)fd_close(fd);

		/* Success */
		if (!err) return (0);

		/* Information */
		msg_print("Ignoring obsolete/defective 'd_info.raw' file.");
		msg_print(NULL);
	}
#endif	// USE_RAW_FILES


	/*** Make the fake arrays ***/

	/* Assume the size of "d_name" and "d_text" */
#if 0
	fake_name_size = FAKE_NAME_SIZE;
	fake_text_size = FAKE_TEXT_SIZE;
#else	// 0
	fake_name_size = 20 * 1024L;
	fake_text_size = 60 * 1024L;
#endif	// 0

	/* Allocate the "d_info" array */
	C_MAKE(d_info, d_head->info_num, dungeon_info_type);

	/* Hack -- make "fake" arrays */
	C_MAKE(d_name, fake_name_size, char);
	C_MAKE(d_text, fake_text_size, char);


	/*** Load the ascii template file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "d_info.txt");

	/* Grab permission */
	//safe_setuid_grab();

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Drop permission */
	//safe_setuid_drop();

	/* Parse it */
	if (!fp) quit("Cannot open 'd_info.txt' file.");

	/* Parse the file */
	err = init_d_info_txt(fp, buf);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
		s_printf("Error %d at line %d df 'd_info.txt'.\n", err, error_line);
		s_printf("Record %d contains a '%s' error.\n", error_idx, oops);
		s_printf("Parsing '%s'.\n", buf);
		//msg_print(NULL);

		/* Quit */
		quit("Error in 'd_info.txt' file.");
	}


#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	/*** Dump the binary image file ***/

	/* File type is "DATA" */
	FILE_TYPE(FILE_TYPE_DATA);

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "d_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Kill the old file */
	(void)fd_kill(buf);

	/* Attempt to create the raw file */
	fd = fd_make(buf, mode);

	/* Drop permission */
	safe_setuid_drop();

	/* Dump to the file */
	if (fd >= 0)
	{
		/* Dump it */
		fd_write(fd, (char*)(d_head), d_head->head_size);

		/* Dump the "r_info" array */
		fd_write(fd, (char*)(d_info), d_head->info_size);

		/* Dump the "r_name" array */
		fd_write(fd, (char*)(d_name), d_head->name_size);

		/* Dump the "r_text" array */
		fd_write(fd, (char*)(d_text), d_head->text_size);

		/* Close */
		(void)fd_close(fd);
	}


	/*** Kill the fake arrays ***/

	/* Free the "d_info" array */
	C_KILL(d_info, d_head->info_num, dungeon_info_type);

	/* Hack -- Free the "fake" arrays */
	C_KILL(d_name, fake_name_size, char);
	C_KILL(d_text, fake_text_size, char);

	/* Forget the array sizes */
	fake_name_size = 0;
	fake_text_size = 0;

#endif	// USE_RAW_FILES
#endif	/* ALLOW_TEMPLATES */


#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	/*** Load the binary image file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "d_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Attempt to open the "raw" file */
	fd = fd_open(buf, O_RDONLY);

	/* Drop permission */
	safe_setuid_drop();

	/* Process existing "raw" file */
	if (fd < 0) quit("Cannot load 'd_info.raw' file.");

	/* Attempt to parse the "raw" file */
	err = init_d_info_raw(fd);

	/* Close it */
	(void)fd_close(fd);

	/* Error */
	if (err) quit("Cannot parse 'd_info.raw' file.");
#endif	// USE_RAW_FILES

	/* Success */
	return (0);
}


/*
 * Initialize the "t_info" array
 *
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_t_info(void)
{
#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	int mode = 0644;
#endif	// USE_RAW_FILES

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


/*
 * Initialize the "st_info" array
 *
 * Note that we let each entry have a unique "name" and "short name" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_st_info(void)
{
#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	int fd;

	int mode = 0644;
#endif	// USE_RAW_FILES

	errr err = 0;

	FILE *fp;

	/* General buffer */
	char buf[1024];


	/*** Make the header ***/

	/* Allocate the "header" */
	MAKE(st_head, header);

	/* Save the "version" */
	st_head->v_major = VERSION_MAJOR;
	st_head->v_minor = VERSION_MINOR;
	st_head->v_patch = VERSION_PATCH;
	st_head->v_extra = 0;

	/* Save the "record" information */
	st_head->info_num = MAX_ST_IDX;
	st_head->info_len = sizeof(store_info_type);

	/* Save the size of "st_head" and "st_info" */
	st_head->head_size = sizeof(header);
	st_head->info_size = st_head->info_num * st_head->info_len;


#ifdef ALLOW_TEMPLATES
#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */

	/*** Load the binary image file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "st_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Attempt to open the "raw" file */
	fd = fd_open(buf, O_RDONLY);

	/* Drop permission */
	safe_setuid_drop();

	/* Process existing "raw" file */
	if (fd >= 0)
	{
#ifdef CHECK_MODIFICATION_TIME
		err = check_modification_date(fd, "st_info.txt");
#endif /* CHECK_MODIFICATION_TIME */
#ifdef CHECK_MODIFICATION_ALWAYS
	        err = 0;
#endif

		/* Attempt to parse the "raw" file */
		if (!err)
			err = init_st_info_raw(fd);

		/* Close it */
		(void)fd_close(fd);

		/* Success */
		if (!err) return (0);

		/* Information */
		msg_print("Ignoring obsolete/defective 'st_info.raw' file.");
		msg_print(NULL);
	}
#endif	// USE_RAW_FILES


	/*** Make the fake arrays ***/

	/* Assume the size of "st_name" and "st_text" */
//	fake_name_size = FAKE_NAME_SIZE;
	fake_name_size = 20 * 1024L;

	/* Allocate the "st_info" array */
	C_MAKE(st_info, st_head->info_num, store_info_type);

	/* Hack -- make "fake" arrays */
	C_MAKE(st_name, fake_name_size, char);


	/*** Load the ascii template file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "st_info.txt");

	/* Grab permission */
//	safe_setuid_grab();

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Drop permission */
//	safe_setuid_drop();

	/* Parse it */
	if (!fp) quit("Cannot open 'st_info.txt' file.");

	/* Parse the file */
	err = init_st_info_txt(fp, buf);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
		s_printf("Error %d at line %d of 'st_info.txt'.\n", err, error_line);
		s_printf("Record %d contains a '%s' error.\n", error_idx, oops);
		s_printf("Parsing '%s'.\n", buf);

		/* Quit */
		quit("Error in 'st_info.txt' file.");
	}


#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	/*** Dump the binary image file ***/

	/* File type is "DATA" */
	FILE_TYPE(FILE_TYPE_DATA);

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "st_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Kill the old file */
	(void)fd_kill(buf);

	/* Attempt to create the raw file */
	fd = fd_make(buf, mode);

	/* Drop permission */
	safe_setuid_drop();

	/* Dump to the file */
	if (fd >= 0)
	{
		/* Dump it */
		fd_write(fd, (char*)(st_head), st_head->head_size);

		/* Dump the "r_info" array */
		fd_write(fd, (char*)(st_info), st_head->info_size);

		/* Dump the "r_name" array */
		fd_write(fd, (char*)(st_name), st_head->name_size);

		/* Close */
		(void)fd_close(fd);
	}


	/*** Kill the fake arrays ***/

	/* Free the "st_info" array */
	C_KILL(st_info, st_head->info_num, store_info_type);

	/* Hack -- Free the "fake" arrays */
	C_KILL(st_name, fake_name_size, char);

	/* Forget the array sizes */
	fake_name_size = 0;
	fake_text_size = 0;

#endif	// USE_RAW_FILES
#endif	/* ALLOW_TEMPLATES */


#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	/*** Load the binary image file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "st_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Attempt to open the "raw" file */
	fd = fd_open(buf, O_RDONLY);

	/* Drop permission */
	safe_setuid_drop();

	/* Process existing "raw" file */
	if (fd < 0) quit("Cannot load 'st_info.raw' file.");

	/* Attempt to parse the "raw" file */
	err = init_st_info_raw(fd);

	/* Close it */
	(void)fd_close(fd);

	/* Error */
	if (err) quit("Cannot parse 'st_info.raw' file.");
#endif	// USE_RAW_FILES

	/* Success */
	return (0);
}


/*
 * Initialize the "ow_info" array
 *
 * Note that we let each entry have a unique "name" and "short name" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_ow_info(void)
{
#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	int fd;

	int mode = 0644;
#endif	// USE_RAW_FILES

	errr err = 0;

	FILE *fp;

	/* General buffer */
	char buf[1024];


	/*** Make the header ***/

	/* Allocate the "header" */
	MAKE(ow_head, header);

	/* Save the "version" */
	ow_head->v_major = VERSION_MAJOR;
	ow_head->v_minor = VERSION_MINOR;
	ow_head->v_patch = VERSION_PATCH;
	ow_head->v_extra = 0;

	/* Save the "record" information */
	ow_head->info_num = MAX_OW_IDX;
	ow_head->info_len = sizeof(owner_type);

	/* Save the size of "head" and "ow_info" */
	ow_head->head_size = sizeof(header);
	ow_head->info_size = ow_head->info_num * ow_head->info_len;


#ifdef ALLOW_TEMPLATES
#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */

	/*** Load the binary image file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "ow_info.raw");

	/* Grab permission */
//	safe_setuid_grab();

	/* Attempt to open the "raw" file */
	fd = fd_open(buf, O_RDONLY);

	/* Drop permission */
//	safe_setuid_drop();

	/* Process existing "raw" file */
	if (fd >= 0)
	{
#ifdef CHECK_MODIFICATION_TIME
		err = check_modification_date(fd, "ow_info.txt");
#endif /* CHECK_MODIFICATION_TIME */
#ifdef CHECK_MODIFICATION_ALWAYS
	        err = 0;
#endif

		/* Attempt to parse the "raw" file */
		if (!err)
			err = init_ow_info_raw(fd);

		/* Close it */
		(void)fd_close(fd);

		/* Success */
		if (!err) return (0);

		/* Information */
		msg_print("Ignoring obsolete/defective 'ow_info.raw' file.");
		msg_print(NULL);
	}
#endif	// USE_RAW_FILES


	/*** Make the fake arrays ***/

	/* Assume the size of "ow_name" and "ow_text" */
#if 0
	fake_name_size = FAKE_NAME_SIZE;
#else	// 0
	fake_name_size = 20 * 1024L;
#endif	// 0


	/* Allocate the "ow_info" array */
	C_MAKE(ow_info, ow_head->info_num, owner_type);

	/* Hack -- make "fake" arrays */
	C_MAKE(ow_name, fake_name_size, char);


	/*** Load the ascii template file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "ow_info.txt");

	/* Grab permission */
//	safe_setuid_grab();

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Drop permission */
//	safe_setuid_drop();

	/* Parse it */
	if (!fp) quit("Cannot open 'ow_info.txt' file.");

	/* Parse the file */
	err = init_ow_info_txt(fp, buf);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
		s_printf("Error %d at line %d of 'ow_info.txt'.\n", err, error_line);
		s_printf("Record %d contains a '%s' error.\n", error_idx, oops);
		s_printf("Parsing '%s'.\n", buf);

		/* Quit */
		quit("Error in 'ow_info.txt' file.");
	}


#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	/*** Dump the binary image file ***/

	/* File type is "DATA" */
	FILE_TYPE(FILE_TYPE_DATA);

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "ow_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Kill the old file */
	(void)fd_kill(buf);

	/* Attempt to create the raw file */
	fd = fd_make(buf, mode);

	/* Drop permission */
	safe_setuid_drop();

	/* Dump to the file */
	if (fd >= 0)
	{
		/* Dump it */
		fd_write(fd, (char*)(ow_head), ow_head->head_size);

		/* Dump the "r_info" array */
		fd_write(fd, (char*)(ow_info), ow_head->info_size);

		/* Dump the "r_name" array */
		fd_write(fd, (char*)(ow_name), ow_head->name_size);

		/* Close */
		(void)fd_close(fd);
	}


	/*** Kill the fake arrays ***/

	/* Free the "ow_info" array */
	C_KILL(ow_info, ow_head->info_num, owner_type);

	/* Hack -- Free the "fake" arrays */
	C_KILL(ow_name, fake_name_size, char);

	/* Forget the array sizes */
	fake_name_size = 0;
	fake_text_size = 0;

#endif	// USE_RAW_FILES
#endif	/* ALLOW_TEMPLATES */


#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	/*** Load the binary image file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "ow_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Attempt to open the "raw" file */
	fd = fd_open(buf, O_RDONLY);

	/* Drop permission */
	safe_setuid_drop();

	/* Process existing "raw" file */
	if (fd < 0) quit("Cannot load 'ow_info.raw' file.");

	/* Attempt to parse the "raw" file */
	err = init_ow_info_raw(fd);

	/* Close it */
	(void)fd_close(fd);

	/* Error */
	if (err) quit("Cannot parse 'ow_info.raw' file.");
#endif	// USE_RAW_FILES

	/* Success */
	return (0);
}

#if 1
/*
 * Initialize the "ba_info" array
 *
 * Note that we let each entry have a unique "name" and "short name" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_ba_info(void)
{
#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	int fd;

	int mode = 0644;
#endif	// USE_RAW_FILES

	errr err = 0;

	FILE *fp;

	/* General buffer */
	char buf[1024];


	/*** Make the header ***/

	/* Allocate the "header" */
	MAKE(ba_head, header);

	/* Save the "version" */
	ba_head->v_major = VERSION_MAJOR;
	ba_head->v_minor = VERSION_MINOR;
	ba_head->v_patch = VERSION_PATCH;
	ba_head->v_extra = 0;

	/* Save the "record" information */
	ba_head->info_num = MAX_BA_IDX;
	ba_head->info_len = sizeof(store_action_type);

	/* Save the size of "head" and "ba_info" */
	ba_head->head_size = sizeof(header);
	ba_head->info_size = ba_head->info_num * ba_head->info_len;


#ifdef ALLOW_TEMPLATES
#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */

	/*** Load the binary image file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "ba_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Attempt to open the "raw" file */
	fd = fd_open(buf, O_RDONLY);

	/* Drop permission */
	safe_setuid_drop();

	/* Process existing "raw" file */
	if (fd >= 0)
	{
#ifdef CHECK_MODIFICATION_TIME
		err = check_modification_date(fd, "ba_info.txt");
#endif /* CHECK_MODIFICATION_TIME */
#ifdef CHECK_MODIFICATION_ALWAYS
	        err = 0;
#endif

		/* Attempt to parse the "raw" file */
		if (!err)
			err = init_ba_info_raw(fd);

		/* Close it */
		(void)fd_close(fd);

		/* Success */
		if (!err) return (0);

		/* Information */
		msg_print("Ignoring obsolete/defective 'ba_info.raw' file.");
		msg_print(NULL);
	}
#endif	// USE_RAW_FILES


	/*** Make the fake arrays ***/

	/* Assume the size of "ba_name" and "ba_text" */
//	fake_name_size = FAKE_NAME_SIZE;
	fake_name_size = 20 * 1024L;

	/* Allocate the "ba_info" array */
	C_MAKE(ba_info, ba_head->info_num, store_action_type);

	/* Hack -- make "fake" arrays */
	C_MAKE(ba_name, fake_name_size, char);


	/*** Load the ascii template file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "ba_info.txt");

	/* Grab permission */
//	safe_setuid_grab();

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Drop permission */
//	safe_setuid_drop();

	/* Parse it */
	if (!fp) quit("Cannot open 'ba_info.txt' file.");

	/* Parse the file */
	err = init_ba_info_txt(fp, buf);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
		s_printf("Error %d at line %d df 'ba_info.txt'.\n", err, error_line);
		s_printf("Record %d contains a '%s' error.\n", error_idx, oops);
		s_printf("Parsing '%s'.\n", buf);

		/* Quit */
		quit("Error in 'ba_info.txt' file.");
	}


#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	/*** Dump the binary image file ***/

	/* File type is "DATA" */
	FILE_TYPE(FILE_TYPE_DATA);

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "ba_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Kill the old file */
	(void)fd_kill(buf);

	/* Attempt to create the raw file */
	fd = fd_make(buf, mode);

	/* Drop permission */
	safe_setuid_drop();

	/* Dump to the file */
	if (fd >= 0)
	{
		/* Dump it */
		fd_write(fd, (char*)(ba_head), ba_head->head_size);

		/* Dump the "r_info" array */
		fd_write(fd, (char*)(ba_info), ba_head->info_size);

		/* Dump the "r_name" array */
		fd_write(fd, (char*)(ba_name), ba_head->name_size);

		/* Close */
		(void)fd_close(fd);
	}


	/*** Kill the fake arrays ***/

	/* Free the "ba_info" array */
	C_KILL(ba_info, ba_head->info_num, store_action_type);

	/* Hack -- Free the "fake" arrays */
	C_KILL(ba_name, fake_name_size, char);

	/* Forget the array sizes */
	fake_name_size = 0;
	fake_text_size = 0;

#endif	// USE_RAW_FILES
#endif	/* ALLOW_TEMPLATES */


#ifdef USE_RAW_FILES	/* Don't delete it or I'LL SCORCH YOU!	- Jir - */
	/*** Load the binary image file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, "ba_info.raw");

	/* Grab permission */
	safe_setuid_grab();

	/* Attempt to open the "raw" file */
	fd = fd_open(buf, O_RDONLY);

	/* Drop permission */
	safe_setuid_drop();

	/* Process existing "raw" file */
	if (fd < 0) quit("Cannot load 'ba_info.raw' file.");

	/* Attempt to parse the "raw" file */
	err = init_ba_info_raw(fd);

	/* Close it */
	(void)fd_close(fd);

	/* Error */
	if (err) quit("Cannot parse 'ba_info.raw' file.");
#endif	// USE_RAW_FILES

	/* Success */
	return (0);
}
#endif	// 0



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
	s_printf("prepare_distance(): last count = %d\n", count);
#endif
}


void init_schools(s16b new_size)
{
	/* allocate the extra memory */
	C_MAKE(schools, new_size, school_type);
	max_schools = new_size;
}

void init_spells(s16b new_size)
{
	/* allocate the extra memory */
	C_MAKE(school_spells, new_size, spell_type);
	max_spells = new_size;
}


/*
 * Initialize some other arrays
 */
static errr init_other(void)
{
	//int i, k, n;


	/*** Prepare the "dungeon" information ***/

#ifndef USE_OLD_UPDATE_VIEW
	/* Used by "update_view()" */
	(void)vinfo_init();
#endif	/* USE_OLD_UPDATE_VIEW */

	/* Allocate and Wipe the object list */
	C_MAKE(o_list, MAX_O_IDX, object_type);

	/* Allocate and Wipe the monster list */
	C_MAKE(m_list, MAX_M_IDX, monster_type);

	/* Allocate and Wipe the traps list */
//	C_MAKE(t_list, MAX_TR_IDX, trap_type);

	/*** Init the wild_info array... for more information see wilderness.c ***/
	// init_wild_info();

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
	(void)quark_add("Handmade");

	/* Turn on color */
	use_color = TRUE;

	/*** Pre-allocate space for the "format()" buffer ***/

	/* Hack -- Just call the "format()" function */
	(void)format("%s (%s).", "Ben Harrison", MAINTAINER);

	prepare_distance();

	/* Success */
	return (0);
}


static void init_swearing(){
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
	for (i = 1; i < max_k_idx; i++)
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
	for (i = 1; i < max_k_idx; i++)
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

#if 0
/* 
 * Mark guardians and their artifacts with SPECIAL_GENE flag
 */
static void init_guardians(void)
{
	int i;

	/* Scan dungeons */
	for (i = 0; i < max_d_idx; i++)
	{
		dungeon_info_type *d_ptr = &d_info[i];

		/* Mark the guadian monster */
		if (d_ptr->final_guardian)
		{
			monster_race *r_ptr = &r_info[d_ptr->final_guardian];

			r_ptr->flags9 |= RF9_SPECIAL_GENE;

			/* Mark the final artifact */
			if (d_ptr->final_artifact)
			{
				artifact_type *a_ptr = &a_info[d_ptr->final_artifact];

				a_ptr->flags4 |= TR4_SPECIAL_GENE;
			}

			/* Mark the final object */
			if (d_ptr->final_object)
			{
				object_kind *k_ptr = &k_info[d_ptr->final_object];

				k_ptr->flags4 |= TR4_SPECIAL_GENE;
			}

			/* Give randart if there are no final artifacts */
			if (!(d_ptr->final_artifact) && !(d_ptr->final_object))
			{
				r_ptr->flags7 |= RF7_DROP_RANDART;
			}
		}
	}
}
#endif	// 0

static bool str_to_boolean(char * str)
{
	/* false by default */
	return !(strcasecmp(str, "true"));
}

/* Try to set a server option.  This is handled very sloppily right now,
 * since server options are manually mapped to global variables.  Maybe
 * the handeling of this will be unified in the future with some sort of 
 * options structure.
 */
static void set_server_option(char * option, char * value)
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
	else if (!strcmp(option,"META_PORT"))
	{
                cfg.meta_port = atoi(value);
	}
	else if (!strcmp(option,"WORLDSERVER")){
		cfg.wserver=strdup(value);
	}
	else if (!strcmp(option,"WORLDPASS")){
		cfg.pass=strdup(value);
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
	else if (!strcmp(option,"MAX_LIFES"))
	{
		cfg.lifes = atoi(value);
	}
	else if (!strcmp(option,"DOOR_BUMP_OPEN"))
	{
		cfg.door_bump_open = atoi(value);
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
	else if (!strcmp(option,"GW_PORT"))
	{
		cfg.gw_port = atoi(value);
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
	else if (!strcmp(option,"ANTI_ARTS_HOARD"))
	{
		cfg.anti_arts_hoard = str_to_boolean(value);
	}
	else if (!strcmp(option,"ANTI_ARTS_HOUSE"))
	{
		cfg.anti_arts_house = str_to_boolean(value);
	}
	else if (!strcmp(option,"ANTI_ARTS_SHOP"))
	{
		cfg.anti_arts_shop = str_to_boolean(value);
	}
	else if (!strcmp(option,"ANTI_ARTS_PICKUP"))
	{
		cfg.anti_arts_pickup = str_to_boolean(value);
	}
	else if (!strcmp(option,"ANTI_ARTS_SEND"))
	{
		cfg.anti_arts_send = str_to_boolean(value);
	}
	else if (!strcmp(option,"ANTI_CHEEZE_PICKUP"))
	{
		cfg.anti_cheeze_pickup = str_to_boolean(value);
	}
	else if (!strcmp(option,"SURFACE_ITEM_REMOVAL"))
	{
		cfg.surface_item_removal = atoi(value);
	}
	else if (!strcmp(option,"DUNGEON_SHOP_CHANCE"))
	{
		cfg.dungeon_shop_chance = atoi(value);
	}
	else if (!strcmp(option,"DUNGEON_SHOP_TYPE"))
	{
		cfg.dungeon_shop_type = atoi(value);
	}
	else if (!strcmp(option,"DUNGEON_SHOP_TIMEOUT"))
	{
		cfg.dungeon_shop_timeout = atoi(value);
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
	else if (!strcmp(option,"CBLUE_MONSTERS"))
	{
		cfg.cblue_monsters = atoi(value);
	}
	else if (!strcmp(option,"VANILLA_MONSTERS"))
	{
		cfg.vanilla_monsters = atoi(value);
	}
	else if (!strcmp(option,"PET_MONSTERS"))
	{
		cfg.pet_monsters = atoi(value);
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
	else if (!strcmp(option, "USE_PK_RULES"))
	{
		cfg.use_pk_rules = atoi(value);
	}
	else if (!strcmp(option, "QUIT_BAN_MODE"))
	{
		cfg.quit_ban_mode = atoi(value);
	}
	else if (!strcmp(option,"LOG_U"))
	{
		cfg.log_u = str_to_boolean(value);
	}
	else if (!strcmp(option,"REPLACE_HISCORE"))
	{
		cfg.replace_hiscore = atoi(value);
	}
	else if (!strcmp(option,"UNIKILL_FORMAT"))
	{
		cfg.unikill_format = atoi(value);
	}
	else if (!strcmp(option,"SERVER_NOTES"))
	{
		cfg.server_notes = strdup(value);
	}
	else if (!strcmp(option,"ARTS_DISABLED"))
	{
		cfg.arts_disabled = str_to_boolean(value);
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
static void load_server_cfg_aux(FILE * cfg)
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
	FILE *cfg_file;
	
	/* Attempt to open the file */
//	cfg = fopen("tomenet.cfg", "r");
	cfg_file = fopen(MANGBAND_CFG, "r");

	/* Failure */
	if (cfg_file == (FILE*)NULL)
	{
//		printf("Error : cannot open file tomenet.cfg\n");
		printf("Error : cannot open file '%s'\n", MANGBAND_CFG);
		return (FALSE);
	}

        /* Default value */
        cfg.bind_name = NULL;

	/* Actually parse the file */
	load_server_cfg_aux(cfg_file);

	/* Close it */
	fclose(cfg_file);

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

	/* Initialize skill info */
	s_printf("[Initializing arrays... (skills)]\n");
	if (init_s_info()) quit("Cannot initialize skills");

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

	/* Initialize dungeon type info */
	s_printf("[Initializing arrays... (dungeon types)]\n");
	if (init_d_info()) quit("Cannot initialize dungeon types");
	//init_guardians();

	/* Initialize feature info */
	s_printf("[Initializing arrays... (vaults)]\n");
	if (init_v_info()) quit("Cannot initialize vaults");

	/* Initialize trap info */
	s_printf("[Initializing arrays... (traps)]\n");
	if (init_t_info()) quit("Cannot initialize traps");

#if 1
	/* Initialize actions type info */
	s_printf("[Initializing arrays... (action types)]\n");
	if (init_ba_info()) quit("Cannot initialize action types");
#endif	// 0

	/* Initialize owners type info */
	s_printf("[Initializing arrays... (owners types)]\n");
	if (init_ow_info()) quit("Cannot initialize owners types");

#if 1
	/* Initialize stores type info */
	s_printf("[Initializing arrays... (stores types)]\n");
	if (init_st_info()) quit("Cannot initialize stores types");
#endif	// 0

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


